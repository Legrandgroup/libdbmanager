/**
 *
 * \file dbmanager.hpp
 *
 * \brief Wrapper around SQL commands presenting C++ methods requests instead
 */

#ifndef _DBMANAGER_HPP_
#define _DBMANAGER_HPP_

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
#include "sqltable.hpp"

/**
 * \interface DBManager
 *
 * \brief Interface for managing requests to a database.
 *
 * This interface offers methods to deal with SQL tables without having to care about SQL language.
 *
 */
class DBManager
{
public:
	//Get a table records, with possibility to specify some field value (name - value expected) Should have used default parameters but it doesn't exsisit un DBus.
	/**
	 * \brief table content getter
	 *
	 * This method allows to obtain the content of a SQL table with some parameters.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \param distinct Set to true to remove duplicated records from the result.
	 * \return vector< map<string, string> > The recordd list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	virtual std::vector< std::map<string, string> > get(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false, const bool& isAtomic = true) noexcept = 0;

	//Insert a new record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to insert a record in a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \return bool The success or failure of the operation.
	 */
	bool insert(const std::string& table, const std::map<std::string , std::string>& values = std::map<std::string , std::string >(), const bool& isAtomic = true) {
		return this->insert(table, std::vector<std::map<std::string,std::string>>({values}), isAtomic);
	}
	/**
	 * \brief table record setter
	 *
	 * Allows to insert a record in a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \return bool The success or failure of the operation.
	 */
	virtual bool insert(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>(), const bool& isAtomic = true) = 0;

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
	virtual bool modify(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence = true, const bool& isAtomic = true) noexcept = 0;

	//Delete a record from the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to delete a record from a table. Can be called over DBus.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refField The reference fields values to identify the record to update in the table.
	 * \return bool The success or failure of the operation.
	 */
	virtual bool remove(const std::string& table, const std::map<std::string, std::string>& refFields, const bool& isAtomic = true) = 0;

	/**
	 * \brief table dump method
	 *
	 * Dumps all table infos and contents of the database in a visually formated string.
	 *
	 * \return string The visually formated string containing infos and contents of tables of the database.
	 */
	//std::string dumpTables();
	/**
	 * \brief table dump method
	 *
	 * Dumps all table infos and contents of the database in a HTML formated string.
	 *
	 * \return string The HTML formated string containing infos and contents of tables of the database.
	 */
	//std::string dumpTablesAsHtml();
};

#endif //_DBMANAGER_HPP_
