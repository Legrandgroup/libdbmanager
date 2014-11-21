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
//#include "sqltable.hpp"

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
	 * \param isAtomic A flag to do the operations in an atomic way.
	 * \return vector< map<string, string> > The records list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	virtual std::vector< std::map<std::string, std::string> > get(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false, const bool& isAtomic = true) noexcept = 0;

	//Insert a new record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to insert only one record in a table.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \param isAtomic A flag to do the operations in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	bool insert(const std::string& table, const std::map<std::string , std::string>& values = std::map<std::string , std::string >(), const bool& isAtomic = true) {
		return this->insert(table, std::vector<std::map<std::string,std::string>>({values}), isAtomic);
	}
	/**
	 * \brief table record setter
	 *
	 * Allows to insert some records in a table.
	 * \param table The name of the SQL table in which the records will be inserted.
	 * \param values The records to insert in the table.
	 * \param isAtomic A flag to do the operations in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	virtual bool insert(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>(), const bool& isAtomic = true) = 0;

	//Update a record in the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to update a record of a table.
	 * \param table The name of the SQL table in which the record will be updated.
	 * \param refField The reference fields values to identify the record to update in the table. If empty, will update all records of the table.
	 * \param values The new record values to update in the table.
	 * \param checkExistence A flag to set in order to check existence of the record in the table. If the record existence must be checked and the record does not exist, it will be inserted into the table.
	 * \param isAtomic A flag to do the operations in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	virtual bool modify(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence = true, const bool& isAtomic = true) noexcept = 0;

	//Delete a record from the specified table
	/**
	 * \brief table record setter
	 *
	 * Allows to delete a record from a table.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refField The reference fields values to identify the record to update in the table. If empty, all records in the table will be deleted;
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return bool The success or failure of the operation.
	 */
	virtual bool remove(const std::string& table, const std::map<std::string, std::string>& refFields, const bool& isAtomic = true) = 0;
	virtual bool linkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool & isAtomic = true) = 0;
	virtual bool unlinkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool & isAtomic = true) = 0;
protected:
	/**
	 * \brief database status check
	 *
	 * Allows to check the status of a database. It could be used with a file that describes the schemas of the database. This methods, if properly implemented, may allow to have a migration mechasnism.
	 * \param isAtomic A boolean to indicates that the operation should be done in an atomic way.
	 */
	virtual void checkDefaultTables(const bool& isAtomic = true) = 0;
};

#endif //_DBMANAGER_HPP_
