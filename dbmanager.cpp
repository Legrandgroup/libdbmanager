#include "dbmanager.hpp"

DBManager* DBManager::instance = NULL;

DBManager::DBManager(string filename) {
	this->filename = filename;
	this->db = new Database(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE);
}

DBManager::~DBManager() {
	if(this->instance != NULL) {
		delete instance;
		instance = NULL;
	}

	if(this->db != NULL) {
		delete db;
		db = NULL;
	}
}


DBManager* DBManager::GetInstance() {
	if(instance == NULL) {
		instance = new DBManager("testdb.sql");
	}

	return instance;
}

//Get a table records, with possibility to specify some field value (name - value expected)
vector< map<string, string> > DBManager::get(string table, vector<string> columns, bool distinct) {
	stringstream ss;
	ss << "SELECT ";
	if(columns.empty()) {
		ss << "*";
		Statement query(*(this->db), "PRAGMA table_info(" + table + ");");
		while(query.executeStep())
			columns.push_back(query.getColumn(1));
	}
	else {
		for(vector<string>::iterator it = columns.begin(); it != columns.end(); it++) {
			ss << *it;
			if((it+1) != columns.end())
				ss <<  ",";
		}
	}
	
	if(distinct)
		ss << " DISTINCT";

	ss << " FROM " << table;

	Statement query(*(this->db), ss.str()); 	

	vector<map<string, string> > result;

	while(query.executeStep()) {
		map<string, string> record;

		for(int i = 0; i < query.getColumnCount(); i++) {
			record.insert(pair<string, string>(columns.at(i), query.getColumn(i).getText()));
		}

		result.push_back(record);
	}

	return result;
}

//Insert a new record in the specified table
bool DBManager::insertRecord(string table, map<string,string> values) {
	stringstream ss;

	ss << "INSERT INTO " << table << " ";

	stringstream columnsName;
	stringstream columnsValue;
	columnsName << "(";
	columnsValue << "(";
	for(map<string, string>::iterator it = values.begin(); it != values.end(); it++) {
		map<string, string>::iterator tmp = it;
		tmp++;
		bool testok = (tmp != values.end());
		columnsName << it->first;
		if(testok)
			columnsName << ",";
		columnsValue << it->second;
		if(testok)
			columnsValue << ",";
	}
	columnsName << ")";
	columnsValue << ")";

	ss << columnsName.str() << " VALUES " << columnsValue.str() << ";";

	cout << "Request: " << ss.str() << endl;

	Statement query(*(this->db), ss.str());

	return (query.exec() > 0);
}

//Update a record in the specified table
bool DBManager::modifyRecord(string table, string recordId, map<string,string> values) {
	return true;
}

//Delete a record from the specified table
bool DBManager::deleteRecord(string table, string recordId) {
	return true;
}
