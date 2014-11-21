/**
 *
 * \file dbmanager.hpp
 *
 * \brief Wrapper around SQL commands (to a sqlite3 database) presenting C++ methods requests instead
 */

#ifndef _SQLITE_DBMANAGER_HPP_
#define _SQLITE_DBMANAGER_HPP_

//STL includes
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <utility>
#include <map>
#include <exception>
#include <mutex>

//tinyxml includes
#include <tinyxml.h>

//SQLiteCpp includes
#include "SQLiteC++.h"

//Project includes
#include "dbmanager.hpp"
#include "sqltable.hpp"


/**
 * \class SqliteDBManager
 *
 * \brief Class for managing requests to a sqlite3 database.
 *
 * This class offers methods to deal with SQL tables without having to care about SQL specifications. It uses SQLiteC++ wrapper.
 *
 * It allows some methods to be called on DBus for testing purposes.
 *
 * It implements a Singleton design pattern.
 */
class SQLiteDBManager : public DBManager
{
public:
	/**
	 * \brief Constructor.
	 *
	 * Only constructor of the class.
	 *
	 * \param filename The SQLite database file path.
	 */
	SQLiteDBManager(const std::string& filename, const std::string& configurationDescriptionFile = "");
	/**
	 * \brief Destructor.
	 *
	 */
	~SQLiteDBManager() noexcept;
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
	std::vector< std::map<string, string> > get(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false, const bool& isAtomic = true) noexcept;


	//Insert a new record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to insert a record in a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool insert(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>(), const bool& isAtomic = true);

	//Update a record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to update a record of a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be updated.
	 * \param refField The reference fields values to identify the record to update in the table.
	 * \param values The new record values to update in the table.
	 * \param checkExistence A flag to set in order to check existence of records in the base. If it doesn't, it should be inserted.
	 * \return bool The success or failure of the operation.
	 */
	bool modify(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence = true, const bool& isAtomic = true) noexcept;

	//Delete a record from the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to delete a record from a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refField The reference fields values to identify the record to update in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool remove(const std::string& table, const std::map<std::string, std::string>& refFields, const bool& isAtomic = true);
	bool linkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool& isAtomic = true);
	bool unlinkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool& isAtomic = true);
	/**
	 * \brief table dump method
	 *
	 * Dumps all table infos and contents of the database in a visually formated string.
	 *
	 * \return string The visually formated string containing infos and contents of tables of the database.
	 */
	std::string dumpTables();
	/**
	 * \brief table dump method
	 *
	 * Dumps all table infos and contents of the database in a HTML formated string.
	 *
	 * \return string The HTML formated string containing infos and contents of tables of the database.
	 */
	std::string dumpTablesAsHtml();

private :
	std::vector< std::map<string, string> > getCore(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false) noexcept;

	bool insertCore(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>());

	bool modifyCore(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence = true) noexcept;

	bool removeCore(const std::string& table, const std::map<std::string, std::string>& refFields);
	//Check presence of default tables (name and columns) and corrects absence of table of wrong columns.
	/**
	 * \brief table check method
	 *
	 * Allows to check the presence of default tables in the database according to specifics models.
	 *
	 * If tables are missing, it builds them. If tables are present but don't match models, it modifies them to make them match models.
	 *
	 */
	void checkDefaultTables(const bool& isAtomic = true);
	bool checkDefaultTablesCore();

	/**
	 * \brief table check method
	 *
	 * Allows to check the presence and the state of a table in the database according to a model.
	 *
	 * If table is missing, it builds it. If table is present but don't match the model, it modifies it to make it match the model.
	 *
	 */
	void checkTableInDatabaseMatchesModel(const SQLTable &model, const bool& isAtomic = true) noexcept;
	bool checkTableInDatabaseMatchesModelCore(const SQLTable &model) noexcept;

	/**
	 * \brief table creation method
	 *
	 * Allows to create a table in the database with a set of fields name and default value.
	 *
	 * \param table The table name.
	 * \param values The fields name and default values of the table.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const std::string& table, const std::map<std::string, std::string>& values, const bool& isAtomic = true);
	//Create a table that matches the parameter
	/**
	 * \brief table setter
	 *
	 * Allows to create a table in the database.
	 * \param table The SQLTable instance that modelizes the table to be created.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const SQLTable& table, const bool& isAtomic = true) noexcept;
	bool createTableCore(const SQLTable& table) noexcept;
	//Alter a table to match the parameter
	/**
	 * \brief table setter
	 *
	 * Allows to add fields to a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to add to the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool addFieldsToTable(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields, const bool& isAtomic = true) noexcept;
	bool addFieldsToTableCore(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept;
	/**
	 * \brief table setter
	 *
	 * Allows to remove fields of a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to remove of the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool removeFieldsFromTable(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields, const bool& isAtomic = true) noexcept;
	bool removeFieldsFromTableCore(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept;
	//Delete a table
	/**
	 * \brief table setter
	 *
	 * Allows to delete a table from the database.
	 * \param table The table name to delete.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteTable(const std::string& table, const bool& isAtomic = true) noexcept;
	bool deleteTableCore(const std::string& table) noexcept;
	/**
	 * \brief table listing method
	 *
	 * Lists all table names of the database.
	 *
	 * \return vector<string> The list of table names of the database.
	 */
	std::vector< std::string > listTables(const bool& isAtomic = true);
	std::vector< std::string > listTablesCore();
	bool isReferenced(const std::string& name, const bool& isAtomic = true);
	bool isReferencedCore(const std::string& name);
	std::set<std::string> getPrimaryKeys(const std::string& name, const bool& isAtomic = true);
	std::set<std::string> getPrimaryKeysCore(const std::string& name);
	std::set<std::string> getFieldNames(const std::string& name, const bool& isAtomic = true);
	std::set<std::string> getFieldNamesCore(const std::string& name);
	std::map<std::string, std::string> getDefaultValues(const std::string& name, const bool& isAtomic = true);
	std::map<std::string, std::string> getDefaultValuesCore(const std::string& name);
	std::map<std::string, bool> getNotNullFlags(const std::string& name, const bool& isAtomic = true);
	std::map<std::string, bool> getNotNullFlagsCore(const std::string& name);
	std::map<std::string, bool> getUniqueness(const std::string& name, const bool& isAtomic = true);
	std::map<std::string, bool> getUniquenessCore(const std::string& name);
	std::string createRelation(const std::string &kind, const std::vector<std::string> &tables, const bool& isAtomic = true);
	std::string createRelationCore(const std::string &kind, const std::vector<std::string> &tables);
	SQLTable getTableFromDatabase(const string& table, const bool& isAtomic = true);
	SQLTable getTableFromDatabaseCore(const string& table);
	bool linkRecordsCore(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2);
	bool unlinkRecordsCore(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2);

	bool applyPolicy(const string& relationshipName, const string& relationshipPolicy, const vector<string>& linkedTables, const bool& isAtomic = true);
	bool applyPolicyCore(const string& relationshipName, const string& relationshipPolicy, const vector<string>& linkedTables);

	std::string filename;			/*!< The SQLite database file path.*/
	std::string configurationDescriptionFile;			/*!< The SQLite database file path.*/
	std::mutex mut;					/*!< The mutex to lock access to the base.*/
	SQLite::Database *db;				/*!< The database object (actually points to a SQLite::Database underneath but we hide it so that code using this library does not also have to include SQLiteC++.h */
};

#endif //_SQLITE_DBMANAGER_HPP_
