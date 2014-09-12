#include "dbmanager.hpp"

DBManager* DBManager::instance = NULL;

DBManager::DBManager(Connection &connection, string filename) : ObjectAdaptor(connection, "/org/Legrand/Conductor/DBManager") {
	this->filename = filename;
	//this->db = *(new Database(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE));
}

DBManager::~DBManager() {
	/*if(this->db != NULL) {
		delete db;
		db = NULL;
	}//*/
}


DBManager* DBManager::GetInstance() {
	if(instance == NULL) {
		Connection bus = Connection::SystemBus();
		bus.request_name("org.Legrand.Conductor.DBManager");

		instance = new DBManager(bus);//, "testdb.sql");
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
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		stringstream ss;
		ss << "SELECT ";

		if(distinct)
			ss << "DISTINCT ";

		vector<basic_string<char> > newColumns;
		if(columns.empty()) {
			ss << "*";
			Statement query(db, "PRAGMA table_info(\"" + table + "\");");
			while(query.executeStep())
				newColumns.push_back(query.getColumn(1).getText());
		}
		else if(columns.size() == 1) {
			if(columns.at(0) == "*") {
				ss << "*";
				Statement query(db, "PRAGMA table_info(\"" + table + "\");");
				while(query.executeStep())
					newColumns.push_back(query.getColumn(1).getText());
			}
			else {
				for(vector<basic_string<char> >::const_iterator it = columns.begin(); it != columns.end(); it++) {
					ss << "\"" << *it << "\"";
					if((it+1) != columns.end())
						ss <<  ",";
					newColumns.push_back(*it);
				}
			}
		}
		else {
			for(vector<basic_string<char> >::const_iterator it = columns.begin(); it != columns.end(); it++) {
				ss << "\"" << *it << "\"";
				if((it+1) != columns.end())
					ss <<  ",";
				newColumns.push_back(*it);
			}
		}


		ss << " FROM \"" << table << "\"";

		Statement query(db, ss.str());

		vector<map<string, string> > result;

		while(query.executeStep()) {
			map<string, string> record;

			for(int i = 0; i < query.getColumnCount(); i++) {
				if(query.getColumn(i).isNull()) {
					record.insert(pair<string, string>(newColumns.at(i), ""));
				}
				else {
					record.insert(pair<string, string>(newColumns.at(i), query.getColumn(i).getText()));
				}
			}

			result.push_back(record);
		}

		this->mut.unlock();
		return result;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		this->mut.unlock();
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
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);
		stringstream ss;

		ss << "INSERT INTO \"" << table << "\" ";

		if(values.size() != 0) {
			stringstream columnsName;
			stringstream columnsValue;
			columnsName << "(";
			columnsValue << "(";
			map<string, string> newValues(values);
			for(map<string, string>::iterator it = newValues.begin(); it != newValues.end(); it++) {
				map<string, string>::iterator tmp = it;
				tmp++;
				bool testok = (tmp != newValues.end());
				columnsName << "\"" << it->first << "\"";
				if(testok)
					columnsName << ",";
				columnsValue << "\"" << it->second << "\"";
				if(testok)
					columnsValue << ",";
			}
			columnsName << ")";
			columnsValue << ")";

			ss << columnsName.str() << " VALUES " << columnsValue.str() << ";";
		}
		else {
			ss << "DEFAULT VALUES";
		}

		//cout << "Request: " << ss.str() << endl;

		bool result= db.exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		this->mut.unlock();
		return result;
	}
	catch(const Exception &e) {
		this->mut.unlock();
		cerr  << e.what() << endl;
		return false;
	}
}

//Update a record in the specified table
bool DBManager::modifyRecord(const string& table, const map<string, string>& refFields, const map<basic_string<char>, basic_string<char> >& values) {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);
		stringstream ss;
		ss << "UPDATE \"" << table << "\" SET ";

		map<string, string> newValues(values);
		for(map<string, string>::iterator it = newValues.begin(); it != newValues.end(); it++) {
			map<string, string>::iterator tmp = it;
			tmp++;
			bool testok = (tmp != newValues.end());

			ss << "\"" << it->first << "\" = \"" << it->second << "\"";
			if(testok)
				ss << ", ";
			else
				ss << " ";
		}

		if(refFields.size() > 0) {
			ss << "WHERE ";
			map<string, string> tmp = refFields;
			for(map<string, string>::iterator it = tmp.begin(); it != tmp.end(); it++) {
				map<string, string>::iterator tester = it;
				tester++;
				bool testOk = (tester != tmp.end());
				ss << "\"" << it->first << "\" = \"" << it->second << "\"";
				if(testOk)
					ss << " AND ";
			}
		}
	
		bool result= db.exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		this->mut.unlock();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return false;
	}
}

//Delete a record from the specified table
bool DBManager::deleteRecord(const string& table, const map<string, string>& refFields) {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);
		stringstream ss;

		ss << "DELETE FROM \"" << table << "\"";
			if(refFields.size() > 0) {
			ss << " WHERE ";

			map<string, string> tmp = refFields;
			for(map<string, string>::iterator it = tmp.begin(); it != tmp.end(); it++) {
				map<string, string>::iterator tester = it;
				tester++;
				bool testOk = (tester != tmp.end());
				ss << "\"" << it->first << "\" = \"" << it->second << "\"";
				if(testOk)
					ss << " AND ";
			}
		}
	
		bool result= db.exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		this->mut.unlock();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return false;
	}
}

void DBManager::checkDefaultTables() {
	Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
	//Table "global"
	SQLTable tableGlobal("global");
	tableGlobal.addField(tuple<string,string,bool,bool>("admin-password", "", true, false));
	tableGlobal.addField(tuple<string,string,bool,bool>("switch-name", "WiFi-SOHO", true, false ));
	this->checkTableInDatabaseMatchesModel(tableGlobal);

	
	//Table "management-interface"
	SQLTable tableMI("management-interface");
	tableMI.addField(tuple<string,string,bool,bool>("IP-address-type", "autoip", true, false));
	tableMI.addField(tuple<string,string,bool,bool>("IP-address", "", true, false));
	tableMI.addField(tuple<string,string,bool,bool>("IP-netmask", "", true, false));
	tableMI.addField(tuple<string,string,bool,bool>("default-gateway", "", true, false));
	tableMI.addField(tuple<string,string,bool,bool>("dns-server", "", true, false));
	tableMI.addField(tuple<string,string,bool,bool>("vlan", "", true, false));
	tableMI.addField(tuple<string,string,bool,bool>("remote-support-server", "", true, false));
	this->checkTableInDatabaseMatchesModel(tableMI);

	Statement query(db, "SELECT name FROM sqlite_master WHERE type='table'");
	vector<string> tablesInDb;
	while(query.executeStep()) {
		tablesInDb.push_back(query.getColumn(0).getText());
	}

	for(vector<string>::iterator it = tablesInDb.begin(); it != tablesInDb.end(); it++) {
		if(*it != tableGlobal.getName() && *it != tableMI.getName()) {
			this->deleteTable(*it);
		}
	}
}

void DBManager::checkTableInDatabaseMatchesModel(const SQLTable &model) {
	Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
	if(!db.tableExists(model.getName())) {
		this->createTable(model);
	}
	else {	
		//cout "Trying table_info on " << model.getName() << endl;
		Statement query(db, "PRAGMA table_info(\""+ model.getName()  + "\")");
		SQLTable tableInDb(model.getName());
		while(query.executeStep()) {
			string name = query.getColumn(1).getText();
			string defaultValue = query.getColumn(5).getText();
			bool isNotNull = (query.getColumn(3).getInt() == 1);
			bool isPk = (query.getColumn(5).getInt() == 1);
			tableInDb.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isPk));
			//cout "Added field " << name << " while building tableInDb " << endl;
		}
		//cout tableInDb.getFields().size()  << " fields extracted from table_info" << endl;

		if(model == tableInDb) {
			//cout "Table exist and matches the model." << endl;
		}		
		else {
			//cout "Table exists but is different." << endl;
			/*if(model.getFields().size() < tableInDb.getFields().size()) { // Too many columns in DB
				//cout "More columns in DB." << endl;
				this->removeFieldsFromTable(tableInDb.getName(), tableInDb.diff(model));
			}
			else if(model.getFields().size() > tableInDb.getFields().size()) { // Missing columns in DB
				//cout "Less columns in DB." << endl;
				this->addFieldsToTable(tableInDb.getName(), model.diff(tableInDb));
			}
			else {	 // Same number of columns but columsn are different.
				//cout "Same columns in DB." << endl;
				this->removeFieldsFromTable(tableInDb.getName(), tableInDb.diff(model));
				this->addFieldsToTable(tableInDb.getName(), model.diff(tableInDb));
			}//*/
				//cout "Will exec add." << endl;
				this->addFieldsToTable(tableInDb.getName(), model.diff(tableInDb));
				//cout "Execed add." << endl;
				//cout "Will exec remove." << endl;
				this->removeFieldsFromTable(tableInDb.getName(), tableInDb.diff(model));
				//cout "Execed remove." << endl;
		}
		//Checker si la table existante est différente du modèle. Si c'est le cas, ajouter ou supprimer les colonnes en conséquence.
	}
}

bool DBManager::createTable(const SQLTable& table) {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);
		stringstream ss;
		ss << "CREATE TABLE \"" << table.getName() << "\" (";
		vector< tuple<string,string,bool,bool> > fields = table.getFields();

		for(vector< tuple<string,string,bool,bool> >::iterator it = fields.begin(); it != fields.end(); it++) {
			vector< tuple<string,string,bool,bool> >::iterator tmp = it;
			tmp++;
			bool testOk = (tmp != fields.end());
			//0 -> field name
			//1 -> field default value
			//2 -> field is not null
			//3 -> field is primarykey
			ss << "\"" << std::get<0>(*it) << "\" TEXT ";
			if(std::get<2>(*it))
				ss << "NOT NULL ";
			if(std::get<3>(*it))
				ss << "PRIMARY KEY ";
			ss << "DEFAULT \"" << std::get<1>(*it) << "\"";

			if(testOk)
				ss << ", ";
		}

		ss << ")";

		//cout "Query: " << ss.str() << endl;

		db.exec(ss.str());
		transaction.commit();
		this->mut.unlock();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return false;
	}
}

bool DBManager::addFieldsToTable(const string& table, const vector<tuple<string, string, bool, bool> > fields) {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);
		stringstream ss;
		bool result = true;

		if(!fields.empty()) {
			for(vector<tuple<string, string, bool, bool> >::const_iterator it = fields.begin(); it != fields.end(); it++) {
				ss.str("");
				ss << "ALTER TABLE \"" << table << "\" ADD \"" << std::get<0>(*it) << "\" TEXT";
				if(std::get<2>(*it))
					ss << "NOT NULL ";
				if(std::get<3>(*it))
					ss << "PRIMARY KEY ";
				ss << "DEFAULT \"" << std::get<1>(*it) << "\"";
				//cout "THEQUERYADD: " << ss.str() << endl;
				db.exec(ss.str());
			}
		}

		if(result)
			transaction.commit();
		this->mut.unlock();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return false;
	}
}

bool DBManager::removeFieldsFromTable(const string & table, const vector<tuple<string, string, bool, bool> > fields) {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);
		bool result = true;
		//cout "Fields size in ramoveFieldsFT: " << fields.size() << endl;
		if(!fields.empty()) {
			//cout "Not empty." << endl;
			vector<string> remainingFields;

			Statement query(db, "PRAGMA table_info(\""+ table  + "\")");
			SQLTable tableInDb("global");
			while(query.executeStep()) {
				string name = query.getColumn(1).getText();
				bool isRemaining = true;
				for(vector<tuple<string, string, bool, bool> >::const_iterator it = fields.begin(); it != fields.end(); it++) {
					if(std::get<0>(*it) == name)
						isRemaining = false;
				}
				if(isRemaining)
					remainingFields.push_back(name);
			}
	
			if(!remainingFields.empty()) {
				//cout "Fields to remove not empty." << endl;
				stringstream ss;
				ss << "ALTER TABLE \"" << table << "\" RENAME TO \"" << table << "OLD\"";

				Statement query2(db, ss.str());
				result = (result && (query2.exec() > 0));
				ss.str("");
				ss << "CREATE TABLE \"" << table << "\" AS SELECT ";
				for(vector<string>::iterator it = remainingFields.begin(); it != remainingFields.end(); it++) {
					vector<string>::iterator tmp = it;
					tmp++;
					bool testOk = (tmp != remainingFields.end());

					ss << "\"" << *it << "\"";
					//ss << *it ;
					if(testOk)
						ss << ",";
				}

				ss << " FROM \"" << table << "\"OLD";
		
				//cout "THEQUERY: " << ss.str() << endl;
				db.exec(ss.str());
			
				ss.str("");
				ss << "DROP TABLE \"" << table << "\"OLD";
				db.exec(ss.str());
			}
		}
	
		if(result)
			transaction.commit();
		this->mut.unlock();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return false;
	}
}


bool DBManager::deleteTable(const string& table) {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Transaction transaction(db);

		stringstream ss;

		ss << "DROP TABLE \"" << table << "\"";

		db.exec(ss.str());
		transaction.commit();
		this->mut.unlock();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return false;
	}
}

bool DBManager::createTable(const string& table, const map< string, string >& values) {
	SQLTable tab(table);
	for(map< string, string >::const_iterator it = values.begin(); it != values.end(); it++) {
		tab.addField(tuple<string, string, bool, bool>(it->first, it->second, true, false));
	}
	return this->createTable(tab);
}

vector< string > DBManager::listTables() {
	this->mut.lock();
	try {
		Database db(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
		Statement query(db, "SELECT name FROM sqlite_master WHERE type='table'");
		vector<string> tablesInDb;
		while(query.executeStep()) {
			tablesInDb.push_back(query.getColumn(0).getText());
		}

		this->mut.unlock();
		return tablesInDb;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		this->mut.unlock();
		return vector<string>();
	}
}

string DBManager::dumpTables() {
	stringstream dump;

	vector< string > tables = this->listTables();

	for(vector<string>::iterator tableName = tables.begin(); tableName != tables.end(); tableName++) {
		//Récupération des valeurs
		vector<map<string, string> > records = this->get(*tableName);

		//Calcul du plus long mot
		map<string, unsigned int> longests;
		for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); vectIt++) {
			for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); mapIt++) {
				//Init of values
				if(longests.find(mapIt->first) == longests.end()) {
					longests.insert(pair<string, unsigned int>(mapIt->first, 0));
				}

				//First is column name.
				if(mapIt->first.size() > longests[mapIt->first]) {
					//cout << "Name checked and is bigger" << endl;
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
		for(map<string, string>::iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); mapIt++) {
			//First is column name.
			for(unsigned int i = 0; i < longests[mapIt->first]; i++) {
				Hsep << "-";
			}
			headers << mapIt->first;

			for(unsigned int i = 0; i < (longests[mapIt->first]-mapIt->first.size()); i++) {
				headers << " ";
			}

			//Check if iterator is the last one with data
			map<string, string>::iterator tmp = records.at(0).end();
			tmp--;
			if(tmp != mapIt) {
				Hsep << "-+-";
				headers << " | ";
			}
		}
		Hsep << "-+";
		headers << " |";

		stringstream values;
		for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); vectIt++) {
			values << "| ";
			for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); mapIt++) {
				values << mapIt->second;

				for(unsigned int i = 0; i < (longests[mapIt->first]-mapIt->second.size()); i++) {
					values << " ";
				}

				//Check if iterator is the last one with data
				map<string, string>::iterator tmp = vectIt->end();
				tmp--;
				if(tmp != mapIt) {

					values << " | ";
				}
			}
			values << " |";

			//Check if iterator is the last one with data
			vector<map<string, string> >::iterator tmp = records.end();
			tmp--;
			if(tmp != vectIt) {
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
		vector<string>::iterator tmp = tables.end();
		tmp--;
		if(tmp != tableName) {
			dump << endl;
		}
	}

	return dump.str();
}
