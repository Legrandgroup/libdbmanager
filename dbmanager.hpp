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

#include <dbus-c++/dbus.h>
#include "dbmanageradaptor.hpp"

#include <sqlitecpp/SQLiteC++.h>

#include "dbconfig.hpp"

using namespace std;
using namespace DBus;
using namespace SQLite;

class DBManager :
public org::Legrand::Conductor::DBmanager_adaptor,
public IntrospectableAdaptor,
public ObjectAdaptor
{
public:
	static DBManager* GetInstance();

	//Get a table records, with possibility to specify some field value (name - value expected) Should have used default parameters bu it doesn't exsisit un DBus.
	vector< map<string, string> > get(const string& table, const vector<basic_string<char> >& columns = vector<basic_string<char> >(), const bool& distinct = false);
	vector< map<string, string> > getPartialTableWithoutDuplicates(const string& table, const vector<basic_string<char> >& columns);
	vector< map<string, string> > getPartialTable(const string& table, const vector<basic_string<char> >& columns);
	vector< map<string, string> > getFullTableWithoutDuplicates(const string& table);
	vector< map<string, string> > getFullTable(const string& table);
	
	//Insert a new record in the specified table
	bool insertRecord(const string& table, const map<basic_string<char>, basic_string<char> >& values);

	//Update a record in the specified table
	bool modifyRecord(const string& table, const string& recordId, const map<basic_string<char>, basic_string<char> >& values);

	//Delete a record from the specified table
	bool deleteRecord(const string& table, const string& recordId);

private :
	DBManager(Connection &connection, string filename = PATH_DB);
	~DBManager();

	bool createTable(const string& name, const vector<string>& columns);

	static DBManager* instance;
	
	string filename;
	Database *db;
};

#endif //_DBMANAGER_HPP_
