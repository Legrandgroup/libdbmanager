/**
 *
 * * \file dbmanager.hpp
 *
 * * \brief Class for managing requests to a sqlite3 database
 *
 * */

#ifndef _DBMANAGER_HPP_
#define _DBMANAGER_HPP_


#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <utility>
#include <map>
#include <exception>

#include <sqlitecpp/SQLiteC++.h>

#include "dbconfig.hpp"

using namespace std;
using namespace SQLite;

class DBManager {
public:
	static DBManager* GetInstance();

	//Get a table records, with possibility to specify some field value (name - value expected)
	vector< map<string, string> > get(string table, vector<string> columns = vector<string>(), bool distinct = false);
	
	//Insert a new record in the specified table
	bool insertRecord(string table, map<string,string> values);

	//Update a record in the specified table
	bool modifyRecord(string table, string recordId, map<string,string> values);

	//Delete a record from the specified table
	bool deleteRecord(string table, string recordId);

private :
	DBManager(string filename = PATH_DB);
	~DBManager();

	static DBManager* instance;
	
	string filename;
	Database *db;
};

#endif //_DBMANAGER_HPP_
