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
#define TIXML_USE_STL	/* Fix possibly badly packaged tinyxml headers */
#include <tinyxml.h>
#undef TIXML_USE_STL

//SQLiteCpp includes
#include "SQLiteC++.h"

//Project includes
#include "dbmanager.hpp"
#include "sqltable.hpp"


/**
 * \class SQLiteDBManager
 *
 * \brief Class for managing requests to a sqlite3 database.
 *
 * This class is an implementation of the DBManager interface to use a sqlite SQL database.
 *
 */
class SQLiteDBManager : public DBManager {

public:
	/**
	 * \brief Constructor.
	 *
	 * Only constructor of the class.
	 *
	 * \param filename The SQLite database file path.
	 * \param configurationDescriptionFile The configuration file for database migration.
	 */
	SQLiteDBManager(const std::string& filename, const std::string& configurationDescriptionFile = "");
	
	/**
	 * \brief Destructor.
	 */
	~SQLiteDBManager() noexcept;
	
	/**
	 * \brief Copy constructor.
	 *
	 * Copy construction is not allowed... because we hold a mutex
	 */
	SQLiteDBManager(const SQLiteDBManager& other) = delete;

	/**
	 * \brief Assignment operator.
	 *
	 * Assignment is not allowed... because we would also duplicate a mutex, which is not allowed
	 *
	 * \param other The object assigned to us
	 * \return Ourselves, with our new identity
	 */
	SQLiteDBManager& operator=(const SQLiteDBManager other) = delete;

	/**
	 * \brief table content getter
	 *
	 * This method is the implementation of the DBManager interface get method.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \param distinct Set to true to remove duplicated records from the result.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	std::vector< std::map<std::string, std::string> > get(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false, const bool& isAtomic = true) const noexcept;

	/**
	 * \brief table record setter
	 *
	 * This method is the implementation of the DBManager interface insert method.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool insert(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>(), const bool& isAtomic = true);

	/**
	 * \brief table record setter
	 *
	 * This method is the implementation of the DBManager interface modify method.
	 * \param table The name of the SQL table in which the record will be updated.
	 * \param refFields The reference fields values to identify the record to update in the table.
	 * \param values The new record values to update in the table.
	 * \param insertIfNotExists If set to true, the record will be inserted if it does not exist yer, if false, the method will only modify an existing record or fail if it does not exist
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool modify(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& insertIfNotExists = true, const bool& isAtomic = true) noexcept;

	/**
	 * \brief table record setter
	 *
	 * This method is the implementation of the DBManager interface remove method.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refFields The reference fields values to identify the record to update in the table.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool remove(const std::string& table, const std::map<std::string, std::string>& refFields, const bool& isAtomic = true);

	/**
	 * \brief table record setter
	 *
	 * This method is the implementation of the DBManager interface linkRecords method.
	 * \param table1 The name of the first SQL table that contains the first record to link.
	 * \param record1 The first record in table1 to link.
	 * \param table2 The name of the second SQL table that contains the second record to link.
	 * \param record2 The second record in table2 to link.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool linkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool& isAtomic = true);
	/**
	 * \brief table record setter
	 *
	 * This method is the implementation of the DBManager interface unlinkRecords method.
	 * \param table1 The name of the first SQL table that contains the first record to link.
	 * \param record1 The first record in table1 to link.
	 * \param table2 The name of the second SQL table that contains the second record to link.
	 * \param record2 The second record in table2 to link.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool unlinkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool& isAtomic = true);

	/**
	 * \brief table record getter
	 *
	 * This method is the implementation of the DBManager interface getLinkedRecords method.
	 * \param table The name of the SQL table that contains the record to take as reference.
	 * \param record The record in table to find.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return map<string, vector<map<string,string>>> All the records linked to the specified record organized by tables.
	 */
	std::map<std::string, std::vector<std::map<std::string,std::string>>> getLinkedRecords(const std::string& table, const std::map<std::string, std::string>& record, const bool & isAtomic = true) const;

	/**
	 * \brief table listing method
	 *
	 * This method is the implementation of the DBManager interface listTables method.
	 *
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return vector<string> The list of table names of the database.
	 */
	std::vector<std::string> listTables(const bool& isAtomic = true) const;

	/**
	 * \brief database configuration file setter
	 *
	 * This method is the implementation of the DBManager interface setDatabaseConfigurationFile method.
	 *
	 * \param databaseConfigurationFile The new database configuration file value.
	 */
	void setDatabaseConfigurationFile(const std::string& databaseConfigurationFile = "");

	/**
	 * \brief table dump method
	 *
	 * Dumps the whole database to a visually formated string.
	 *
	 * \param dumpTableName A specific table to dump (if specified, we will only dump that table, otherwise, we will dump the whole database)
	 *
	 * \return A string representing (visually) the requested data.
	 */
	std::string to_string(std::string dumpTableName) const;
	
	/**
	 * \brief table dump method
	 *
	 * Dumps all table infos and contents of the database in a HTML formated string.
	 *
	 * \return The HTML formated string containing infos and contents of tables of the database.
	 */
	std::string dumpTablesAsHtml() const;

private :
	/**
	 * \brief table/column string escaping function for SQL commands
	 *
	 * This method will escape a table/column name in order to pass it between double quotes (") inside an SQL statement
	 * It will repeat twice all double quotes in string
	 *
	 * \param in The input table/column name
	 * \return The corresponding escaped name
	 */
	const std::string escDQ(const std::string& in) const;

	/**
	 * \brief table dump method
	 *
	 * Dumps one table of the database to a visually formated string.
	 *
	 * \param dumpTableName The name of the table to dump
	 *
	 * \return string A string representing (visually) the requested table.
	 */
	std::string tableToString(const std::string& tableName) const;

	/**
	 * \brief table content getter
	 *
	 * The 'core' of the get method, which contains all the SQL statements.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \param distinct Set to true to remove duplicated records from the result.
	 * \return vector< map<string, string> > The record list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	std::vector< std::map<std::string, std::string> > getCore(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false) const noexcept;

	/**
	 * \brief table record setter
	 *
	 * The 'core' of the insert method, which contains all the SQL statements.
	 *
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool insertCore(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>());

	/**
	 * \brief table record setter
	 *
	 * The 'core' of the modify method, which contains all the SQL statements.
	 *
	 * \param table The name of the SQL table in which the record will be updated.
	 * \param refFields The reference fields values to identify the record to update in the table.
	 * \param values The new record values to update in the table.
	 * \param insertIfNotExists If set to true, the record will be inserted if it does not exist yer, if false, the method will only modify an existing record or fail if it does not exist
	 * \return bool The success or failure of the operation.
	 */
	bool modifyCore(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence = true) noexcept;

	/**
	 * \brief table record setter
	 *
	 * The 'core' of the remove method, which contains all the SQL statements.
	 *
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refFields The reference fields values to identify the record to update in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool removeCore(const std::string& table, const std::map<std::string, std::string>& refFields);

	/**
	 * \brief table record setter
	 *
	 * The 'core' of the linkRecords method, which contains all the SQL statements.
	 * \param table1 The name of the first SQL table that contains the first record to link.
	 * \param record1 The first record in table1 to link.
	 * \param table2 The name of the second SQL table that contains the second record to link.
	 * \param record2 The second record in table2 to link.
	 * \return bool The success or failure of the operation.
	 */
	bool linkRecordsCore(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2);

	/**
	 * \brief table record setter
	 *
	 * The 'core' of the unlinkRecords method, which contains all the SQL statements.
	 * \param table1 The name of the first SQL table that contains the first record to link.
	 * \param record1 The first record in table1 to link.
	 * \param table2 The name of the second SQL table that contains the second record to link.
	 * \param record2 The second record in table2 to link.
	 * \return bool The success or failure of the operation.
	 */
	bool unlinkRecordsCore(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2);

	/**
	 * \brief table check method
	 *
	 * Allows to check the presence of default tables in the database according to specifics models.
	 *
	 * If tables are missing, it builds them. If tables are present but don't match models, it modifies them to make them match models.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool checkDefaultTables(const bool& isAtomic = true);

	/**
	 * \brief table check method
	 *
	 * The 'core' of the checkDefaultTables method, which contains all the SQL statements.
	 * \return bool The success or failure of the operation.
	 */
	bool checkDefaultTablesCore();

	/**
	 * \brief table check method
	 *
	 * Allows to check the presence and the state of a table in the database according to a model.
	 *
	 * If table is missing, it builds it. If table is present but don't match the model, it modifies it to make it match the model.
	 * \param model A SQLTable instance that modelizes a SQL table.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 */
	void checkTableInDatabaseMatchesModel(const SQLTable &model, const bool& isAtomic = true) noexcept;

	/**
	 * \brief table check method
	 *
	 * The 'core' of the checkTableInDatabaseMatchesModel method, which contains all the SQL statements.
	 * \param model A SQLTable instance that modelizes a SQL table.
	 */
	bool checkTableInDatabaseMatchesModelCore(const SQLTable &model) noexcept;

	/**
	 * \brief table creation method
	 *
	 * Allows to create a table in the database with a set of fields name and default value.
	 *
	 * \param table The table name.
	 * \param values The fields name and default values of the table.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const std::string& table, const std::map<std::string, std::string>& values, const bool& isAtomic = true);

	/**
	 * \brief table creation method
	 *
	 * Allows to create a table in the database  according to a SQL table instance modelizing the table.
	 * \param table The SQLTable instance that modelizes the table to be created.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool createTable(const SQLTable& table, const bool& isAtomic = true) noexcept;

	/**
	 * \brief table creation method
	 *
	 * The 'core' of the createTable method, which contains all the SQL statements.
	 * \param table The SQLTable instance that modelizes the table to be created.
	 * \return bool The success or failure of the operation.
	 */
	bool createTableCore(const SQLTable& table) noexcept;

	/**
	 * \brief table setter
	 *
	 * Allows to add fields to a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to add to the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool addFieldsToTable(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields, const bool& isAtomic = true) noexcept;

	/**
	 * \brief table setter
	 *
	 * The 'core' of the addFieldsToTable method, which contains all the SQL statements.
	 * \param table The table name to be modified.
	 * \param fields The fields to add to the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool addFieldsToTableCore(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept;

	/**
	 * \brief table setter
	 *
	 * Allows to remove fields of a table in the database.
	 * \param table The table name to be modified.
	 * \param fields The fields to remove of the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool removeFieldsFromTable(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields, const bool& isAtomic = true) noexcept;

	/**
	 * \brief table setter
	 *
	 * The 'core' of the removeFieldsFromTable method, which contains all the SQL statements.
	 * \param table The table name to be modified.
	 * \param fields The fields to remove of the table. A field is modelized by a tuple of 4 elements in this order : the field name [string], the field default value [string], the ability of the field to have NULL value [bool](false = can have NULL value) and the ability of the field to be in the primary key of the table[bool].
	 * \return bool The success or failure of the operation.
	 */
	bool removeFieldsFromTableCore(const std::string& table, const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept;

	/**
	 * \brief table deleter
	 *
	 * Allows to delete a table from the database.
	 * \param table The table name to delete.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteTable(const std::string& table, const bool& isAtomic = true) noexcept;

	/**
	 * \brief table deleter
	 *
	 * Allows to delete a table from the database.
	 * \param table The table name to delete.
	 * \return bool The success or failure of the operation.
	 */
	bool deleteTableCore(const std::string& table) noexcept;

	/**
	 * \brief table record getter
	 *
	 * The 'core' of the removeFieldsFromTable method, which contains all the SQL statements.
	 * \param table The name of the SQL table that contains the record to take as reference.
	 * \param record The record in table to find.
	 * \return map<string, vepctor<map<string,string>>> All the records linked to the specified record organized by tables.
	 */
	std::map<std::string, std::vector<std::map<std::string,std::string>>> getLinkedRecordsCore(const std::string& table, const std::map<std::string, std::string>& record) const;

	/**
	 * \brief db info getter
	 *
	 * Check if the database has foreign keys enabled
	 * \return true if foreign keys are enabled
	 */
	bool areForeignKeysEnabled() const;
	
	/**
	 * \brief table listing method
	 *
	 * The 'core' of the listTables method, which contains all the SQL statements.
	 *
	 * \return vector<string> The list of table names of the database.
	 */
	std::vector< std::string > listTablesCore() const;

	/**
	 * \brief table status checking method
	 *
	 * Checks if a table is referenced in the database (it is considered referenced if the table has a primary key).
	 *
	 * \param name The name of the table to check in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The referenced status of the table.
	 */
	bool isReferenced(const std::string& name, const bool& isAtomic = true) const;

	/**
	 * \brief table status checking method
	 *
	 * The 'core' of the isReferenced method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to check in the database.
	 * \return bool The referenced status of the table.
	 */
	bool isReferencedCore(const std::string& name) const;

	/**
	 * \brief table status setter method
	 *
	 * Mark a table as referenced in the database (aka giving it a primary key).
	 *
	 * \param name The name of the table to mark in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The result of the operation.
	 */
	bool markReferenced(const std::string& name, const bool& isAtomic = true);

	/**
	 * \brief table status setter method
	 *
	 * The 'core' of the markReferenced method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to mark in the database.
	 * \return bool The result of the operation.
	 */
	bool markReferencedCore(const std::string& name);

	/**
	 * \brief table status setter method
	 *
	 * Mark a table as not referenced in the database (aka removing its primary key).
	 *
	 * \param name The name of the table to mark in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The result of the operation.
	 */
	bool unmarkReferenced(const std::string& name, const bool& isAtomic = true);

	/**
	 * \brief table status setter method
	 *
	 * The 'core' of the unmarkReferenced method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to mark in the database.
	 * \return bool The result of the operation.
	 */
	bool unmarkReferencedCore(const std::string& name);

	/**
	 * \brief table information getter method
	 *
	 * Get all the field names that composes the primary key.
	 *
	 * \param name The name of the table to check in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return set<string> The list of table names of the database.
	 */
	std::set<std::string> getPrimaryKeys(const std::string& name, const bool& isAtomic = true) const;

	/**
	 * \brief table information getter method
	 *
	 * The 'core' of the getPrimarykeys method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to check in the database.
	 * \return set<string> The list of table names of the database.
	 */
	std::set<std::string> getPrimaryKeysCore(const std::string& name) const;

	/**
	 * \brief table information getter method
	 *
	 * Is the column name \p column a primary key in table \p table
	 *
	 * \param table The name of the table to check in the database.
	 * \param column The name of the column to check in \p table
	 * \return true if \p column is a primary key in table \p table
	 */
	inline bool isPrimaryKeyCore(const std::string& table, const std::string& column) const {
		std::set<std::string> primarykeys = this->getPrimaryKeys(table);
		return (primarykeys.find(column) != primarykeys.end());
	}
	
	/**
	 * \brief table information getter method
	 *
	 * Get all the field names of the table.
	 *
	 * \param name The name of the table to check in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return The list of fields names in table \p name (set)
	 */
	std::set<std::string> getFieldNames(const std::string& name, const bool& isAtomic = true) const;

	/**
	 * \brief table information getter method
	 *
	 * The 'core' of the getFieldNames method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to check in the database.
	 * \return The list of fields names in table \p name (set)
	 */
	std::set<std::string> getFieldNamesCore(const std::string& name) const;

	/**
	 * \brief table information getter method
	 *
	 * Get all the default values of fields that have a default value in the table.
	 *
	 * \param name The name of the table to check in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return map<string, string> The default values of each field of the table that has a default value.
	 */
	std::map<std::string, std::string> getDefaultValues(const std::string& name, const bool& isAtomic = true) const;

	/**
	 * \brief table information getter method
	 *
	 * The 'core' of the getDefaultValues method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to check in the database.
	 * \return map<string, string> The default values of each field of the table that has a default value.
	 */
	std::map<std::string, std::string> getDefaultValuesCore(const std::string& name) const;

	/**
	 * \brief table information getter method
	 *
	 * Get all the not null status of fields that have a default value in the table.
	 *
	 * \param name The name of the table to check in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return map<string, bool> The not null status of each field of the table that has a default value.
	 */
	std::map<std::string, bool> getNotNullFlags(const std::string& name, const bool& isAtomic = true) const;

	/**
	 * \brief table information getter method
	 *
	 * The 'core' of the getNotNullFlags method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to check in the database.
	 * \return map<string, bool> The not null status of each field of the table that has a default value.
	 */
	std::map<std::string, bool> getNotNullFlagsCore(const std::string& name) const;

	/**
	 * \brief table information getter method
	 *
	 * Get all the uniqueness status of fields that have a default value in the table.
	 *
	 * \param name The name of the table to check in the database.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return map<string, bool> The uniqueness status of each field of the table that has a default value.
	 */
	std::map<std::string, bool> getUniqueness(const std::string& name, const bool& isAtomic = true) const;

	/**
	 * \brief table information getter method
	 *
	 * The 'core' of the getUniqueness method, which contains all the SQL statements.
	 *
	 * \param name The name of the table to check in the database.
	 * \return map<string, bool> The uniqueness status of each field of the table that has a default value.
	 */
	std::map<std::string, bool> getUniquenessCore(const std::string& name) const;

	/**
	 * \brief table creation method
	 *
	 * Method to create a relationship between tables. Only m:n relationship are handled for the moment.
	 *
	 * \param kind The kind of relationship.
	 * \param tables The tables to link.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return string The name of the created table.
	 */
	std::string createRelation(const std::string &kind, const std::vector<std::string> &tables, const bool& isAtomic = true);

	/**
	 * \brief table creation method
	 *
	 * The 'core' of the createRelation method, which contains all the SQL statements.
	 *
	 * \param kind The kind of relationship.
	 * \param tables The tables to link.
	 * \return string The name of the created table.
	 */
	std::string createRelationCore(const std::string &kind, const std::vector<std::string> &tables);

	/**
	 * \brief table creation method
	 *
	 * A method to initialize a SQLTable instance from information obtained in database.
	 *
	 * \param table The table name to modelize.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return The table
	 */
	SQLTable getTableFromDatabase(const std::string& table, const bool& isAtomic = true) const;

	/**
	 * \brief table creation method
	 *
	 * The 'core' of the getTableFromDatabase method, which contains all the SQL statements.
	 *
	 * \param table The table name to modelize.
	 * \return The table
	 */
	SQLTable getTableFromDatabaseCore(const std::string& table) const;

	/**
	 * \brief relationship parametering method
	 *
	 * A method to apply the policy defined in database configuration file for a relationship.
	 *
	 * \param relationshipName The relationship table name on which policy must be applied.
	 * \param relationshipPolicy The policy to apply.
	 * \param linkedTables The tables that are concerned by this relationship.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return string The name of the created table.
	 */
	bool applyPolicy(const std::string& relationshipName, const std::string& relationshipPolicy, const std::vector<std::string>& linkedTables, const bool& isAtomic = true);

	/**
	 * \brief relationship parametering method
	 *
	 * The 'core' of the applyPolicy method, which contains all the SQL statements.
	 *
	 * \param relationshipName The relationship table name on which policy must be applied.
	 * \param relationshipPolicy The policy to apply.
	 * \param linkedTables The tables that are concerned by this relationship.
	 * \return string The name of the created table.
	 */
	bool applyPolicyCore(const std::string& relationshipName, const std::string& relationshipPolicy, const std::vector<std::string>& linkedTables);

	std::string filename;						/*!< The SQLite database file path.*/
	std::string configurationDescriptionFile;	/*!< The configuration file path or the content of this file.*/
	mutable std::mutex mut;								/*!< The mutex to lock access to the base (mutable... so changes to this attribute can be done even on a const object (locking is not changing the db) */
	SQLite::Database* db;						/*!< The database object (actually points to a SQLite::Database underneath but we hide it so that code using this library does not also have to include SQLiteC++.h */
};

#endif //_SQLITE_DBMANAGER_HPP_
