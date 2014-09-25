#include "dbmanager.hpp"

DBManager* DBManager::instance = NULL;

/* ### Useful note ###
 *
 * Methods that affect the database are built this way :
 * Step  1: Create a MutexUnlocker object that lock the mutex on construction and unlock it on destruction
 * Step  2: Build the SQL Query thanks to stringstream (it is a STL stream that allows to easily put variables values in a string)
 * Step  3: Execute the built Query on the database
 * Step 4a: In case of success, return true for modifying operations or return values requested for reading operations
 * Step 4b: In case of failure, catch any exception, return false for modifying operations or return empty values for reading operations.
 */

DBManager::DBManager(Connection &connection, string filename) : filename(filename), ObjectAdaptor(connection, "/org/Legrand/Conductor/DBManager") {
	this->db = new Database(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
}

DBManager::~DBManager() {
	if(this->db != NULL) {
		delete this->db;
		this->db = NULL;
	}
}


DBManager* DBManager::GetInstance() {
	//Singleton design pattern
	if(instance == NULL) {
		Connection bus = Connection::SystemBus();
		bus.request_name("org.Legrand.Conductor.DBManager");

		instance = new DBManager(bus);
		instance->checkDefaultTables();
	}

	return instance;
}

void DBManager::FreeInstance() {
	if(instance != NULL) {
		delete instance;
		instance = NULL;
	}
}

//Get a table records, with possibility to specify some field value (name - value expected)
vector< map<string, string> > DBManager::get(const string& table, const vector<basic_string<char> >& columns, const bool& distinct) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "SELECT ";

		if(distinct)
			ss << "DISTINCT ";

		vector<basic_string<char> > newColumns;
		if(columns.empty()) {
			ss << "*";
			//We fetch the names of table's columns in order to populate the map correctly
			//(With only * as columns name, we are notable to match field names to field values in order to build the map)
			Statement query(*db, "PRAGMA table_info(\"" + table + "\");");
			while(query.executeStep())
				newColumns.push_back(query.getColumn(1).getText());
		}
		else if(columns.size() == 1) {
			if(columns.at(0) == "*") {
				ss << "*";
				//We fetch the names of table's columns in order to populate the map correctly
				//(With only * as columns name, we are notable to match field names to field values in order to build the map)
				Statement query(*db, "PRAGMA table_info(\"" + table + "\");");
				while(query.executeStep())
					newColumns.push_back(query.getColumn(1).getText());
			}
			else {
				ss << "\"" << columns.at(0) << "\"";
				newColumns.push_back(columns.at(0));
			}
		}
		else {
			for(const auto &it : columns) {
				ss << "\"" << it << "\"";
				ss << ", ";
				newColumns.push_back(it);
			}
			ss.str(ss.str().substr(0, ss.str().size()-2)); //Solution that remove last ", "

		}

		ss << " FROM \"" << table << "\"";

		Statement query(*db, ss.str());

		vector<map<string, string> > result;

		while(query.executeStep()) {
			map<string, string> record;

			for(int i = 0; i < query.getColumnCount(); ++i) {
				if(query.getColumn(i).isNull()) {
					record.emplace(newColumns.at(i), "");
				}
				else {
					record.emplace(newColumns.at(i), query.getColumn(i).getText());
				}
			}

			result.push_back(record);
		}

		return result;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		return vector< map<string, string> >();
	}
}
vector< map<string, string> > DBManager::getPartialTableWithoutDuplicates(const string& table, const vector<basic_string<char> >& columns) {
	return this->get(table, columns, true);
}

vector< map<string, string> > DBManager::getPartialTable(const string& table, const vector<basic_string<char> >& columns) {
	return this->get(table, columns, false);
}

vector< map<string, string> > DBManager::getFullTableWithoutDuplicates(const string& table) {
	return this->get(table, vector<basic_string<char> >(), true);
}

vector< map<string, string> > DBManager::getFullTable(const string& table) {
	return this->get(table, vector<basic_string<char> >(), false);
}

//Insert a new record in the specified table
bool DBManager::insertRecord(const string& table, const map<basic_string<char>,basic_string<char> >& values) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "INSERT INTO \"" << table << "\" ";

		if(!values.empty()) {
			stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
			stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
			columnsName << "(";
			columnsValue << "(";
			for(const auto &it : values) {
				columnsName << "\"" << it.first << "\",";
				columnsValue << "\"" << it.second << "\",";
			}
			columnsName.str(columnsName.str().substr(0, columnsName.str().size()-1));
			columnsValue.str(columnsValue.str().substr(0, columnsValue.str().size()-1));
			columnsName << ")";
			columnsValue << ")";

			ss << columnsName.str() << " VALUES " << columnsValue.str() << ";";
		}
		else {
			ss << "DEFAULT VALUES";
		}

		bool result= db->exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr  << e.what() << endl;
		return false;
	}
}

//Update a record in the specified table
bool DBManager::modifyRecord(const string& table, const map<string, string>& refFields, const map<basic_string<char>, basic_string<char> >& values) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "UPDATE \"" << table << "\" SET ";

		for(const auto &it : values) {
			ss << "\"" << it.first << "\" = \"" << it.second << "\", ";
		}
		ss.str(ss.str().substr(0, ss.str().size()-2));
		ss << " ";

		if(!refFields.empty()) {
			ss << "WHERE ";
			for(const auto &it : refFields) {
				ss << "\"" << it.first << "\" = \"" << it.second << "\" AND ";
			}
			ss.str(ss.str().substr(0, ss.str().size()-5));
		}
	
		bool result= db->exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}

//Delete a record from the specified table
bool DBManager::deleteRecord(const string& table, const map<string, string>& refFields) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DELETE FROM \"" << table << "\"";
			if(!refFields.empty()) {
			ss << " WHERE ";

			for(const auto &it : refFields) {
				ss << "\"" << it.first << "\" = \"" << it.second << "\" AND ";
			}
			ss.str(ss.str().substr(0, ss.str().size()-5));
		}
	
		bool result= db->exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}

void DBManager::checkDefaultTables() {
	//Loading of default table model thanks to XML definition file.
	TiXmlDocument doc("/tmp/conductor_db.conf");
	if(doc.LoadFile() || doc.LoadFile("/etc/conductor_db.conf")) { // Will try to load file in tmp first then the one in etc
		vector<SQLTable> tables;
		TiXmlElement *dbElem = doc.FirstChildElement();
		if(dbElem && (string(dbElem->Value()) == "database")) {
			TiXmlElement *tableElem = dbElem->FirstChildElement();
			while(tableElem) {
				if(string(tableElem->Value()) == "table") {
					SQLTable table(tableElem->Attribute("name"));
					TiXmlElement *fieldElem = tableElem->FirstChildElement();
					while(fieldElem) {
						if(string(fieldElem->Value()) == "field") {
							string name = fieldElem->Attribute("name");
							string defaultValue = fieldElem->Attribute("default-value");
							bool isNotNull = (fieldElem->Attribute("is-not-null") == "true");
							bool isPrimaryKey  = (fieldElem->Attribute("is-primary-key") == "true");
							table.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isPrimaryKey));
						}
						fieldElem = fieldElem->NextSiblingElement();
					}
					tables.push_back(table);
				}
				tableElem = tableElem->NextSiblingElement();
			}
		}

		//Check if tables in db match models
		for(auto &table : tables) {
			this->checkTableInDatabaseMatchesModel(table);
		}

		//Remove tables that are present in db but not in model
		for(auto &it : listTables()) {
			bool deleteIt = true;
			for(auto &table : tables) {
				deleteIt = deleteIt && (it != table.getName());
			}
			if(deleteIt)
				this->deleteTable(it);
		}
	}
}

void DBManager::checkTableInDatabaseMatchesModel(const SQLTable &model) {
	if(!db->tableExists(model.getName())) {
		this->createTable(model);
	}
	else {
		Statement query(*db, "PRAGMA table_info(\""+ model.getName()  + "\")");
		SQLTable tableInDb(model.getName());
		while(query.executeStep()) {
			string name = query.getColumn(1).getText();
			string defaultValue = query.getColumn(5).getText();
			bool isNotNull = (query.getColumn(3).getInt() == 1);
			bool isPk = (query.getColumn(5).getInt() == 1);
			tableInDb.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isPk));
		}

		if(model != tableInDb) {
				this->addFieldsToTable(tableInDb.getName(), model.diff(tableInDb));
				this->removeFieldsFromTable(tableInDb.getName(), tableInDb.diff(model));
		}
	}
}

bool DBManager::createTable(const SQLTable& table) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "CREATE TABLE \"" << table.getName() << "\" (";
		vector< tuple<string,string,bool,bool> > fields = table.getFields();

		for(auto &it : fields) {
			//0 -> field name
			//1 -> field default value
			//2 -> field is not null
			//3 -> field is primarykey
			ss << "\"" << std::get<0>(it) << "\" TEXT ";
			if(std::get<2>(it))
				ss << "NOT NULL ";
			if(std::get<3>(it))
				ss << "PRIMARY KEY ";
			ss << "DEFAULT \"" << std::get<1>(it) << "\", ";
		}
		ss.str(ss.str().substr(0, ss.str().size()-2));
		ss << ")";
		db->exec(ss.str());
		transaction.commit();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::addFieldsToTable(const string& table, const vector<tuple<string, string, bool, bool> >& fields) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		//Ate flag to move cursor at the end of the string when we do ss.str("blabla");
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		bool result = true;

		if(!fields.empty()) {
			for(const auto &it : fields) {
				ss.str("");
				ss << "ALTER TABLE \"" << table << "\" ADD \"" << std::get<0>(it) << "\" TEXT ";
				if(std::get<2>(it))
					ss << "NOT NULL ";
				if(std::get<3>(it))
					ss << "PRIMARY KEY ";
				ss << "DEFAULT \"" << std::get<1>(it) << "\"";
				db->exec(ss.str());
			}
		}

		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::removeFieldsFromTable(const string & table, const vector<tuple<string, string, bool, bool> >& fields) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		bool result = true;
		if(!fields.empty()) {
			vector<string> remainingFields;

			Statement query(*db, "PRAGMA table_info(\""+ table  + "\")");
			SQLTable tableInDb(table);
			while(query.executeStep()) {
				string name = query.getColumn(1).getText();
				bool isRemaining = true;
				for(const auto &it : fields) {
					if(std::get<0>(it) == name)
						isRemaining = false;
				}
				if(isRemaining)
					remainingFields.push_back(name);
			}
	
			if(!remainingFields.empty()) {
				stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
				ss << "ALTER TABLE \"" << table << "\" RENAME TO \"" << table << "OLD\"";

				db->exec(ss.str());
				ss.str("");
				ss << "CREATE TABLE \"" << table << "\" AS SELECT ";
				for(auto &it : remainingFields) {
					ss << "\"" << it << "\",";
				}
				ss.str(ss.str().substr(0, ss.str().size()-1));

				ss << " FROM \"" << table << "OLD\"";

				db->exec(ss.str());
				ss.str("");
				ss << "DROP TABLE \"" << table << "OLD\"";
				db->exec(ss.str());
			}
		}
	
		if(result) {
			transaction.commit();
		}
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}


bool DBManager::deleteTable(const string& table) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);

		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DROP TABLE \"" << table << "\"";

		db->exec(ss.str());
		transaction.commit();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::createTable(const string& table, const map< string, string >& values) {
	SQLTable tab(table);
	for(const auto &it : values) {
		tab.addField(tuple<string, string, bool, bool>(it.first, it.second, true, false));
	}
	return this->createTable(tab);
}

vector< string > DBManager::listTables() {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Statement query(*db, "SELECT name FROM sqlite_master WHERE type='table'");
		vector<string> tablesInDb;
		while(query.executeStep()) {
			tablesInDb.push_back(query.getColumn(0).getText());
		}

		return tablesInDb;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return vector<string>();
	}
}

string DBManager::dumpTables() {
	stringstream dump;
	vector< string > tables = this->listTables();

	for(vector<string>::iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		//Récupération des valeurs
		vector<map<string, string> > records = this->get(*tableName);

		if(!records.empty()) {
			//Calcul du plus long mot
			map<string, unsigned int> longests;
			for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					//Init of values
					if(longests.find(mapIt->first) == longests.end()) {
						longests.emplace(mapIt->first, 0);
					}

					//First is column name.
					if(mapIt->first.size() > longests[mapIt->first]) {
						longests[mapIt->first] = mapIt->first.size();
					}
					if(mapIt->second.size() > longests[mapIt->first]) {
						longests[mapIt->first] = mapIt->second.size();
					}
				}
			}

			//Première ligne : nom des colonnes
			stringstream Hsep;
			stringstream headers;
			Hsep << "+-";
			headers << "| ";
			for(map<string, string>::iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); ++mapIt) {
				//First is column name.
				for(unsigned int i = 0; i < longests[mapIt->first]; ++i) {
					Hsep << "-";
				}
				headers << mapIt->first;

				for(unsigned int i = 0; i < (longests[mapIt->first]-mapIt->first.size()); ++i) {
					headers << " ";
				}

				//Check if iterator is the last one with data
				if(next(mapIt) != records.at(0).end()) {
					Hsep << "-+-";
					headers << " | ";
				}
			}
			Hsep << "-+";
			headers << " |";

			stringstream values;
			for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				values << "| ";
				for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					values << mapIt->second;

					for(unsigned int i = 0; i < (longests[mapIt->first]-mapIt->second.size()); ++i) {
						values << " ";
					}

					//Check if iterator is the last one with data
					if(next(mapIt) != vectIt->end()) {
						values << " | ";
					}
				}
				values << " |";

				//Check if iterator is the last one with data
				if(next(vectIt) != records.end()) {
					values << endl;
				}
			}

			stringstream tableDump;
			tableDump << "Table: " << *tableName << endl;
			tableDump << Hsep.str() << endl;
			tableDump << headers.str() << endl;
			tableDump << Hsep.str() << endl;
			tableDump << values.str() << endl;
			tableDump << Hsep.str() << endl;

			dump << tableDump.str();
			//Check if iterator is the last one with data
			if(next(tableName) != tables.end()) {
				dump << endl;
			}
		}
		else {
			dump << "Table " << *tableName << " is empty." << endl;
		}
	}

	return dump.str();
}

string DBManager::dumpTablesAsHtml() {
	stringstream htmlDump;
	htmlDump << "<!DOCTYPE html>";
	htmlDump << "<head>";
	htmlDump << "<title>Condutor tables dump</title>";
	htmlDump <<	"<link rel=\"icon\" href=\"../favicon.ico\">";
	htmlDump <<	"<!-- Bootstrap core CSS -->";
	htmlDump <<	"<link href=\"../css/bootstrap.css\" rel=\"stylesheet\">";
	htmlDump <<	"<!-- HTML5 shim and Respond.js IE8 support of HTML5 elements and media queries -->";
	htmlDump <<	"<!--[if lt IE 9]>";
	htmlDump <<	"<script src=\"../js/html5shiv.min.js\"></script>";
	htmlDump <<	"<script src=\"../js/respond.min.js\"></script>";
	htmlDump <<	"<![endif]-->";
	htmlDump <<	"<!-- [if (It IE 9) & (!IEMobile)]>";
	htmlDump <<	"<script src=\"../js/css3-mediaqueries.js\"></script>";
	htmlDump <<	"<![endif]-->";
	htmlDump << "</head>";
	htmlDump << "<body>";
	htmlDump << "<h1> Dump of Conductor Tables </h1>";

	vector< string > tables = this->listTables();

	for(vector<string>::iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		htmlDump << "<h3> Table : " << *tableName << "</h3>";
		//Récupération des valeurs
		vector<map<string, string> > records = this->get(*tableName);

		if(!records.empty()) {
			htmlDump << "<table class=\"table table-striped table-bordered table-hover\">";
			htmlDump << "<thead>";
			htmlDump << "<tr>";
			//Première ligne : nom des colonnes
			for(map<string, string>::iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); ++mapIt) {
				//First is column name.
				htmlDump << "<th>" << mapIt->first << "</th>";
			}
			htmlDump << "</tr>";
			htmlDump << "</thead>";

			htmlDump << "<tbody>";
			for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				htmlDump << "<tr>";
				for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					htmlDump << "<td>" << mapIt->second << "</td>";
				}
				htmlDump << "</tr>";
			}
			htmlDump << "</tbody>";
			htmlDump << "</table>";
		}
		else {
			htmlDump << "<p>Table " << *tableName << " is empty.</p>";
		}
	}
	htmlDump << "</body>";

	return htmlDump.str();
}
