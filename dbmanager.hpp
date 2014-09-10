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
#include <mutex>

#include <dbus-c++/dbus.h>
#include "dbmanageradaptor.hpp"

#include <sqlitecpp/SQLiteC++.h>

#include "dbconfig.hpp"
#include "sqltable.hpp"


using namespace std;
using namespace DBus;
using namespace SQLite;

/**
 * \class DBManager
 *
 * \brief Class for managing requests to a sqlite3 database.
 *
 * This class offers methods to deal with SQL tables without having to care about SQL specifications. It uses SQLiteC++ wrapper.
 *
 * It allows some methods to be called on DBus for testing purposes.
 *
 * It implements a Singleton design pattern.
 */
class DBManager :
public org::Legrand::Conductor::DBmanager_adaptor,
public IntrospectableAdaptor,
public ObjectAdaptor
{
public:
	/**
	 * \brief instance getter
	 *
	 * This method allows to obtain the pointer to the unique instance of this class. It is part of the Singleton design pattern.
	 *
	 * \return DBManager* The pointer to the unique instance of this class.
	 */
	static DBManager* GetInstance();
	/**
	 * \brief instance setter
	 *
	 * This method allows to free the pointer to the unique instance of this class. It is part of the Singleton design pattern.
	 *
	 */
	static void FreeInstance();

	//Get a table records, with possibility to specify some field value (name - value expected) Should have used default parameters bu it doesn't exsisit un DBus.
	/**
	 * \brief table content getter
	 *
	 * This method allows to obtain the content of a SQL table with some parameters.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \param distinct Set to true to remove duplicated records from the result.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	vector< map<string, string> > get(const string& table, const vector<basic_string<char> >& columns = vector<basic_string<char> >(), const bool& distinct = false);

	//DBus methods
	/**
	 * \brief DBus table content getter
	 *
	 * This method allows to obtain the content of a SQL table with columns filtering and duplicates removal.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	vector< map<string, string> > getPartialTableWithoutDuplicates(const string& table, const vector<basic_string<char> >& columns);
	/**
	 * \brief DBus table content getter
	 *
	 * This method allows to obtain the content of a SQL table with columns filtering.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	vector< map<string, string> > getPartialTable(const string& table, const vector<basic_string<char> >& columns);
	/**
	 * \brief DBus table content getter
	 *
	 * This method allows to obtain the content of a SQL table with duplicates removal. All columns are selected.
	 *
	 * \param table The name of the SQL table.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	vector< map<string, string> > getFullTableWithoutDuplicates(const string& table);
	/**
	 * \brief DBus table content getter
	 *
	 * This method allows to obtain the content of a SQL table. All columns are selected.
	 *
	 * \param table The name of the SQL table.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	vector< map<string, string> > getFullTable(const string& table);
	
	//Insert a new record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to insert a record in a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool insertRecord(const string& table, const map<basic_string<char>, basic_string<char> >& values);

	//Update a record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to update a record of a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be updated.
	 * \param refField The reference fields values to identify the record to update in the table.
	 * \param values The new record values to update in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool modifyRecord(const string& table, const map<string, string>& refFields, const map<basic_string<char>, basic_string<char> >& values);

	//Delete a record from the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to delete a record from a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refField The reference fields values to identify the record to update in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteRecord(const string& table, const map<string, string>& refFields);

	//A replacer dans private après tests
	//Check presence of default tables (name and columns) and corrects absence of table of wrong columns.
	/**
	 * \brief table check method
	 *
	 * Allows to check the presence of default tables in the database according to specifics models.
	 *
	 * If tables are missing, it builds them. If tables are present but don't match models, it modifies them to make them match models.
	 *
	 */
	void checkDefaultTables();
	bool createTable(const string& table, const map< string, string >& values);
	vector< string > listTables();

private :
	/**
	 * \brief Constructor.
	 *
	 * Only constructor of the class.
	 *
	 * As this class inherits DBus::ObjectAdaptor, which needs a DBus connection, this class need one too.
	 *
	 * \param connection The DBus connection to a bus.
	 * \param filename The SQLite database file path.
	 */
	DBManager(Connection &connection, string filename = PATH_DB);
	/**
	 * \brief Destructor.
	 *
	 */
	~DBManager();


	/**
	 * \brief table check method
	 *
	 * Allows to check the presence and the state of a table in the database according to a model.
	 *
	 * If table is missing, it builds it. If table is present but don't match the model, it modifies it to make it match the model.
	 *
	 */
	void checkTableInDatabaseMatchesModel(const SQLTable &model);

	//Create a table that matches the parameter
	/**
	 * \brief table setter
	 *
	 * Allows to create a table in the database.
	 * \param table The SQLTable instance that modelizes the table to be created.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const SQLTable& table);
	//Alter a table to match the parameter
	/**
	 * \brief table setter
	 *
	 * Allows to add fields to a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to add to the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool addFieldsToTable(const string& table, const vector<tuple<string, string, bool, bool> > fields);
	/**
	 * \brief table setter
	 *
	 * Allows to remove fields of a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to remove of the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool removeFieldsFromTable(const string& table, const vector<tuple<string, string, bool, bool> > fields);
	//Delete a table
	/**
	 * \brief table setter
	 *
	 * Allows to delete a table from the database.
	 * \param table The table name to delete.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteTable(const string& table);


	static DBManager* instance;	/*!< The pointer to the unique instance of the class. It is part of the Singleton design pattern.*/
	
	string filename;			/*!< The SQLite database file path.*/
	Database *db;				/*!< The pointer to SQLiteCpp wrapper Database class.*/
	mutex mut;					/*!< The mutex to lock access to the base.*/
};

#endif //_DBMANAGER_HPP_
