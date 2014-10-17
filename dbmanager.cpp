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
	db->exec("PRAGMA foreign_keys = ON");
}

DBManager::~DBManager() noexcept {
	if(this->db != NULL) {
		delete this->db;
		this->db = NULL;
	}
}


DBManager* DBManager::GetInstance() noexcept {
	//Singleton design pattern
	if(instance == NULL) {
		Connection bus = Connection::SystemBus();
		bus.request_name("org.Legrand.Conductor.DBManager");

		instance = new DBManager(bus);
		instance->checkDefaultTables();
	}

	return instance;
}

void DBManager::FreeInstance() noexcept {
	if(instance != NULL) {
		delete instance;
		instance = NULL;
	}
}

//Get a table records, with possibility to specify some field value (name - value expected)
vector< map<string, string> > DBManager::get(const string& table, const vector<basic_string<char> >& columns, const bool& distinct) noexcept {
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
	return this->modifyRecord(table, refFields, values, true);
}

bool DBManager::modifyRecord(const string& table, const map<string, string>& refFields, const map<basic_string<char>, basic_string<char> >& values, const bool& checkExistence) noexcept {
	if(values.empty()) return false;
	if(checkExistence && this->get(table).empty()) { 	//It's okay to call get there, mutex isn't locked yet.
		return this->insertRecord(table, values);
	}
	//In case of check existence and empty table, won't reach there.
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
		//We check first "basics" tables
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
							bool isNotNull = (string(fieldElem->Attribute("is-not-null")) == "true");
							bool isUnique  = (string(fieldElem->Attribute("is-unique")) == "true");
							table.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isUnique));
						}
						fieldElem = fieldElem->NextSiblingElement();
					}
					tables.push_back(table);
				}
				tableElem = tableElem->NextSiblingElement();
			}
		}

		//Then we check relation in order to add foreign keys and create tables for m:n relationships.
		//*
		dbElem = doc.FirstChildElement();
		set<string> relationShipTables;	//Tables creation for relationship purpose.
		if(dbElem && (string(dbElem->Value()) == "database")) {
			TiXmlElement *relationElem = dbElem->FirstChildElement();
			while(relationElem) {
				if(string(relationElem->Value()) == "relationship") {
							string kind = relationElem->Attribute("kind");
							if(kind == "m:n") {
								string firstTableName = relationElem->Attribute("first-table");
								string secondTableName = relationElem->Attribute("second-table");

								for(auto &it : tables) {
									if(it.getName() == firstTableName || it.getName() == secondTableName) {
										it.markReferenced();
									}
								}
								vector<string> linkedtables;
								linkedtables.push_back(firstTableName);
								linkedtables.push_back(secondTableName);
								relationShipTables.emplace(this->createRelation(relationElem->Attribute("kind"), relationElem->Attribute("policy"), linkedtables));
							}
				}
				relationElem = relationElem->NextSiblingElement();
			}
		}//*/

		//cout << endl << "Starting the checking." << endl;
		//Check if tables in db match models
		for(auto &table : tables) {
			this->checkTableInDatabaseMatchesModel(table);
		}

		//Remove tables that are present in db but not in model
		set<string> sqliteSpecificTables;	//Tables not to delete if they exist for internal sqlite behavior.
		sqliteSpecificTables.emplace("sqlite_sequence");

		vector<string> tablesInDbTmp = listTables();
		set<string> tablesInDb;
		for(auto &it :tablesInDbTmp) {
			tablesInDb.emplace(it);
		}

		for(auto &table : tables) {
			cout << "Checking model table " << table.getName() << endl;
			if(tablesInDb.find(table.getName()) != tablesInDb.end()) {
				cout << "Shouldn't be there" << endl;
				tablesInDb.erase(tablesInDb.find(table.getName()));
			}
		}
		for(auto &it : sqliteSpecificTables) {
			cout << "Checking sqlite table " << it << endl;
			if(tablesInDb.find(it) != tablesInDb.end()) {
				cout << "Shouldn't be there" << endl;
				tablesInDb.erase(tablesInDb.find(it));
			}
		}
		for(auto &it : relationShipTables) {
			cout << "Checking relationship table " << it << endl;
			if(tablesInDb.find(it) != tablesInDb.end()) {
				cout << "Shouldn't be there" << endl;
				tablesInDb.erase(tablesInDb.find(it));
			}
		}
		for(auto &it : tablesInDb) {
			cout << "Deleting " << it << endl;
			this->deleteTable(it);
		}

		/*
		for(auto &it : listTables()) {
			bool deleteIt = true;
			for(auto &table : tables) {
				bool isntInModel = (it != table.getName());
				bool isntASQLiteInternTable = (sqliteSpecificTables.find(it) != sqliteSpecificTables.end());
				bool isntARelationShipTable = (relationShipTables.find(it) != relationShipTables.end());
				cout << table.getName() << ": " << endl;
				cout << "MOdel: " << isntInModel << endl;
				cout << "SQLIte Intern: " << isntASQLiteInternTable << endl;
				cout << "Rel Tab: " <<  isntARelationShipTable << endl;
				deleteIt = deleteIt && isntInModel && isntASQLiteInternTable;
			}
			if(deleteIt)
				this->deleteTable(it);
		}//*/
	}
}

void DBManager::checkTableInDatabaseMatchesModel(const SQLTable &model) noexcept {
	cout << "Checking table " << model.getName() << endl;
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
			map<string, bool> uniqueFields = this->getUniqueness(model.getName());
			bool isUnique = uniqueFields[name];
			tableInDb.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isUnique));
		}

		if(this->isReferenced(model.getName())) {
			tableInDb.markReferenced();
		}
		else {
			tableInDb.unmarkReferenced();
		}

		if(model != tableInDb) {
				//cout << "Model n db don't match." << endl;
				this->addFieldsToTable(tableInDb.getName(), model.diff(tableInDb));
				this->removeFieldsFromTable(tableInDb.getName(), tableInDb.diff(model));
		}
	}
}

bool DBManager::createTable(const SQLTable& table) noexcept {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "CREATE TABLE \"" << table.getName() << "\" (";
		vector< tuple<string,string,bool,bool> > fields = table.getFields();

		if(table.isReferenced()) {
			ss << "\"id\" INTEGER PRIMARY KEY AUTOINCREMENT, ";
		}

		for(auto &it : fields) {
			//0 -> field name
			//1 -> field default value
			//2 -> field is not null
			//3 -> field is unique
			ss << "\"" << std::get<0>(it) << "\" TEXT ";
			if(std::get<2>(it))
				ss << "NOT NULL ";
			if(std::get<3>(it))
				ss << "UNIQUE ";
			ss << "DEFAULT \"" << std::get<1>(it) << "\", ";
		}


		ss.str(ss.str().substr(0, ss.str().size()-2));
		ss << ")";

		//cout << ss.str() << endl;
		db->exec(ss.str());
		transaction.commit();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::addFieldsToTable(const string& table, const vector<tuple<string, string, bool, bool> >& fields) noexcept {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Transaction transaction(*db);
		//Ate flag to move cursor at the end of the string when we do ss.str("blabla");
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		bool result = true;

		if(!fields.empty()) {/*
			//(1) We save feidl names, primary keys, default values and not null flags
			Statement query(*db, "PRAGMA table_info(\"" + table + "\")");
			vector<string> fieldNames;
			set<string> primaryKeys;
			map<string, string> defaultValues;
			map<string, bool> notNull;
			while(query.executeStep()) {
				//+1 Because of behavior of the pragma.
				//The pk column is equal to 0 if the field isn't part of the primary key.
				//If the field is part of the primary key, it is equal to the index of the record +1
				//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
				if(query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
					primaryKeys.emplace(query.getColumn(1).getText());
				}
				defaultValues.emplace(query.getColumn(1).getText(), query.getColumn(4).getText());
				notNull.emplace(query.getColumn(1).getText(), (query.getColumn(3).getInt() == 1));
				fieldNames.push_back(query.getColumn(1).getText());
			}

			//(2) Then we save table's records
			vector<map<string, string>> records;
			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			ss << "SELECT ";

			vector<basic_string<char> > newColumns;
			ss << "*";
			//We fetch the names of table's columns in order to populate the map correctly
			//(With only * as columns name, we are notable to match field names to field values in order to build the map)
			cout << ss.str() << endl;
			Statement query2(*db, "PRAGMA table_info(\"" + table + "\");");
			while(query2.executeStep())
				newColumns.push_back(query2.getColumn(1).getText());

			ss << " FROM \"" << table << "\"";
			cout << ss.str() << endl;
			Statement query3(*db, ss.str());

			while(query3.executeStep()) {
				map<string, string> record;
				for(int i = 0; i < query3.getColumnCount(); ++i) {
					if(query3.getColumn(i).isNull()) {
						record.emplace(newColumns.at(i), "");
					}
					else {
						record.emplace(newColumns.at(i), query3.getColumn(i).getText());
					}
				}
				records.push_back(record);
			}

			//(3) Now that everything is saved, we can drop the "old" table
			ss.str("");
			ss << "DROP TABLE \"" << table << "\"";
			cout << ss.str() << endl;
			db->exec(ss.str());

			//(4) We can add the new fields informations
			for(auto &it : fields) {
				string name = std::get<0>(it);
				string dv = std::get<1>(it);
				bool nn = std::get<2>(it);
				bool pk = std::get<3>(it);
				fieldNames.push_back(name);
				defaultValues.emplace(name, dv);
				notNull.emplace(name, nn);
				if(pk)
					primaryKeys.emplace(name);
			}

			//(5) We can recreate the table
			ss.str("");
			ss << "CREATE TABLE \"" << table << "\" (";

			for(auto &it : fieldNames) {
				ss << "\"" << it << "\" TEXT ";
				if(notNull[it])
					ss << "NOT NULL ";
				ss << "DEFAULT \"" << defaultValues[it] << "\", ";
			}

			if(!primaryKeys.empty()) {
				ss << "PRIMARY KEY (";
				for(auto &it : primaryKeys) {
					ss << "\"" << it << "\", ";
				}
				ss.str(ss.str().substr(0, ss.str().size()-2));
				ss << "))";
			}
			else {
				ss.str(ss.str().substr(0, ss.str().size()-2));
				ss << ")";
			}
			db->exec(ss.str());

			//(6) The new table is created, populate with olds records (new fields will have default value)
			for(auto &vectIt : records) {
				ss.str("");
				ss << "INSERT INTO \"" << table << "\" ";

				if(!vectIt.empty()) {
					stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
					stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
					columnsName << "(";
					columnsValue << "(";
					for(const auto &mapIt : vectIt) {
						columnsName << "\"" << mapIt.first << "\",";
						columnsValue << "\"" << mapIt.second << "\",";
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

				cout << ss.str() << endl;
				result = result && db->exec(ss.str()) > 0;
			}
//*/
			//*
			for(const auto &it : fields) {
				ss.str("");
				ss << "ALTER TABLE \"" << table << "\" ADD \"" << std::get<0>(it) << "\" TEXT ";
				if(std::get<2>(it))
					ss << "NOT NULL ";
				ss << "DEFAULT \"" << std::get<1>(it) << "\"";
				db->exec(ss.str());
			}//*/
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

bool DBManager::removeFieldsFromTable(const string & table, const vector<tuple<string, string, bool, bool> >& fields) noexcept {
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


bool DBManager::deleteTable(const string& table) noexcept {
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

	bool foreignKeysEnabled = false;
	Statement query(*db, "PRAGMA foreign_keys");
	while(query.executeStep()) {
		if(string(query.getColumn(0).getText()) == "1") {
			foreignKeysEnabled = true;
		}
		else {
			cout << "Text of pragma is " << query.getColumn(0).getText() << endl;
		}
	}
	if(foreignKeysEnabled) {
		htmlDump << "<p> Foreign Keys are enabled </p>";
	}
	else {
		htmlDump << "<p> Foreign Keys are disabled </p>";
	}

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
				htmlDump << "<th>" << mapIt->first;
				set<string> primarykeys = this->getPrimaryKeys(*tableName);
				if(primarykeys.find(mapIt->first) != primarykeys.end()) {
					htmlDump << " [PK] ";
				}

				map<string, bool> uniqueness = this->getUniqueness(*tableName);
				if(uniqueness[mapIt->first]) {
					htmlDump << " [U] ";
				}
				htmlDump << "</th>";
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

			vector<string> fields;
			Statement query(*db, "PRAGMA table_info(\"" + *tableName + "\")");
			while(query.executeStep()) {
				fields.push_back(query.getColumn(1).getText());
			}

			htmlDump << "<table class=\"table table-striped table-bordered table-hover\">";
			htmlDump << "<thead>";
			htmlDump << "<tr>";
			//Première ligne : nom des colonnes
			for(auto &it : fields) {
				htmlDump << "<th>" << it;
				set<string> primarykeys = this->getPrimaryKeys(*tableName);
				if(primarykeys.find(it) != primarykeys.end()) {
					htmlDump << " [PK] ";
				}
				htmlDump << "</th>";
			}
			htmlDump << "</tr>";
			htmlDump << "</thead>";
			htmlDump << "</table>";
		}
	}
	htmlDump << "</body>";

	return htmlDump.str();
}


bool DBManager::isReferenced(string name) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	bool result = false;
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		while(query.executeStep()) {
			//cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
			//+1 Because of behavior of the pragma.
			//The pk column is equal to 0 if the field isn't part of the primary key.
			//If the field is part of the primary key, it is equal to the index of the record +1
			//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
			if(query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
				result = true;
			}
		}
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
	}
	return result;
}

set<string> DBManager::getPrimaryKeys(string name) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	set<string> result;
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		while(query.executeStep()) {
			//cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
			//+1 Because of behavior of the pragma.
			//The pk column is equal to 0 if the field isn't part of the primary key.
			//If the field is part of the primary key, it is equal to the index of the record +1
			//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
			if(query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
				result.emplace(query.getColumn(1).getText());
			}
		}
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
	}
	return result;
}

map<string, string> DBManager::getDefaultValues(string name) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		map<string, string> defaultValues;
		while(query.executeStep()) {
			defaultValues.emplace(query.getColumn(1).getText(),query.getColumn(3).getText());
		}

		return defaultValues;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return map<string, string>();
	}
}

map<string, bool> DBManager::getNotNullFlags(string name) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		map<string, bool> notNullFlags;
		while(query.executeStep()) {
			notNullFlags.emplace(query.getColumn(1).getText(),(query.getColumn(4).getInt() == 1));
		}

		return notNullFlags;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return map<string, bool>();
	}
}

map<string, bool> DBManager::getUniqueness(string name) {
	MutexUnlocker mu(this->mut); // Class That lock the mutex and unlock it when destroyed.
	try {
		//(1) We get the field list
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		set<string> notUniqueFields;
		while(query.executeStep()) {
			notUniqueFields.emplace(query.getColumn(1).getText());
		}

		//(2) We obtain the unique indexes
		set<string> uniqueFields;
		Statement query2(*db, "PRAGMA index_list(\"" + name + "\")");
		while(query2.executeStep()) {
			if(query2.getColumn(2).getInt() == 1) {
				Statement query3(*db, "PRAGMA index_info(\"" + string(query2.getColumn(1).getText()) + "\")");
				while(query3.executeStep()) {
					uniqueFields.emplace(query3.getColumn(2).getText());
				}
			}
		}

		map<string, bool> uniqueness;
		for(auto &it : uniqueFields) {
			notUniqueFields.erase(notUniqueFields.find(it));
			uniqueness.emplace(it, true);
		}

		for(auto &it : notUniqueFields) {
			uniqueness.emplace(it, false);
		}

		return uniqueness;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return map<string, bool>();
	}
}

string DBManager::createRelation(const string &kind, const string &policy, const vector<string> &tables) {
	if(kind == "m:n" && tables.size() == 2) {
		vector<string> tablesInDb = listTables();
		Transaction transaction(*db);
		string table1 = tables.at(0);
		string table2 = tables.at(1);
		string relationName = table1 + "_" + table2;
		bool addIt = true;
		for(auto &it : tablesInDb) {
			if(it == relationName) {
				addIt = false;
			}
		}
		if(addIt) {
			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			ss << "CREATE TABLE \"" << relationName << "\" (";

			ss << "\"" << table1 << "#" << PK_FIELD_NAME << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
			ss << "\"" << table2 << "#" << PK_FIELD_NAME << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
			ss << "PRIMARY KEY (\"" << table1 << "#" << PK_FIELD_NAME << "\", \"" << table2 << "#" << PK_FIELD_NAME << "\"))";

			cout << ss.str() << endl;
			db->exec(ss.str());

			transaction.commit();
		}
		return relationName;
	}
	else
		return string();
}
