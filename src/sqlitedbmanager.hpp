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
	SQLiteDBManager(std::string filename, std::string configurationDescriptionFile = "");
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
	std::vector< std::map<string, string> > get(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false) noexcept;

	//Insert a new record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to insert a record in a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool insertRecord(const std::string& table, const std::map<std::string , std::string >& values = std::map<std::string , std::string >());

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
	bool modifyRecord(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence = true) noexcept;

	//Delete a record from the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to delete a record from a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refField The reference fields values to identify the record to update in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteRecord(const std::string& table, const std::map<std::string, std::string>& refFields);

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
	/**
	 * \brief table creation method
	 *
	 * Allows to create a table in the database with a set of fields name and default value.
	 *
	 * \param table The table name.
	 * \param values The fields name and default values of the table.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const std::string& table, const std::map<std::string, std::string>& values);
	/**
	 * \brief table listing method
	 *
	 * Lists all table names of the database.
	 *
	 * \return vector<string> The list of table names of the database.
	 */
	std::vector< std::string > listTables();
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
	/**
	 * \brief table check method
	 *
	 * Allows to check the presence and the state of a table in the database according to a model.
	 *
	 * If table is missing, it builds it. If table is present but don't match the model, it modifies it to make it match the model.
	 *
	 */
	void checkTableInDatabaseMatchesModel(const SQLTable &model) noexcept;

	//Create a table that matches the parameter
	/**
	 * \brief table setter
	 *
	 * Allows to create a table in the database.
	 * \param table The SQLTable instance that modelizes the table to be created.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const SQLTable& table) noexcept;
	//Alter a table to match the parameter
	/**
	 * \brief table setter
	 *
	 * Allows to add fields to a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to add to the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool addFieldsToTable(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept;
	/**
	 * \brief table setter
	 *
	 * Allows to remove fields of a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to remove of the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool removeFieldsFromTable(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept;
	//Delete a table
	/**
	 * \brief table setter
	 *
	 * Allows to delete a table from the database.
	 * \param table The table name to delete.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteTable(const std::string& table) noexcept;
	bool isReferenced(std::string name);
	std::set<std::string> getPrimaryKeys(std::string name);
	std::map<std::string, std::string> getDefaultValues(std::string name);
	std::map<std::string, bool> getNotNullFlags(std::string name);
	std::map<std::string, bool> getUniqueness(std::string name);
	std::string createRelation(const std::string &kind, const std::string &policy, const std::vector<std::string> &tables);

	std::string filename;			/*!< The SQLite database file path.*/
	std::string configurationDescriptionFile;			/*!< The SQLite database file path.*/
	std::mutex mut;					/*!< The mutex to lock access to the base.*/
	void *db;				/*!< The database object (actually points to a SQLite::Database underneath but we hide it so that code using this library does not also have to include SQLiteC++.h */
};

#endif //_SQLITE_DBMANAGER_HPP_
