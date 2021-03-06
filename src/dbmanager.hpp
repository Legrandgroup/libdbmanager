/*
This file is part of libdbmanager
(see the file COPYING in the root of the sources for a link to the
homepage of libdbmanager)

libdbmanager is a C++ library providing methods for reading/modifying a
database using only C++ methods & objects and no SQL
Copyright (C) 2016 Legrand SA

libdbmanager is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License version 3
(dated 29 June 2007) as published by the Free Software Foundation.

libdbmanager is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libdbmanager (in the source code, it is enclosed in
the file named "lgpl-3.0.txt" in the root of the sources).
If not, see <http://www.gnu.org/licenses/>.
*/
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

#include "dbmanagerapi.hpp"	// For LIBDBMANAGER_API

/**
 * \interface DBManager
 *
 * \brief Interface for managing requests to a database.
 *
 * This interface offers methods to deal with SQL tables without having to care about SQL language.
 *
 */
class LIBDBMANAGER_API DBManager {

protected:
	/**
	 * \brief Base class destructor
	 *
	 * This destructor is protected to force calling the derived class destructor... never the base class destructor
	 */
	~DBManager() { } // Lionel: FIXME: -Weffc++ will still complain because derived class do not have virtual destructors

public:
	/**
	 * \brief table content getter
	 *
	 * This method allows to obtain the content of a SQL table with some parameters.
	 *
	 * \param table The name of the SQL table.
	 * \param columns The columns name to obtain from the table. Leave empty for all columns.
	 * \param distinct Set to true to remove duplicated records from the result.
	 * \param isAtomic A flag to do the operations in an atomic way.
	 * \return The records list obtained from the SQL table. A record is a pair "field name"-"field value".
	 */
	virtual std::vector< std::map<std::string, std::string> > get(const std::string& table, const std::vector<std::string >& columns = std::vector<std::string >(), const bool& distinct = false, const bool& isAtomic = true) const noexcept = 0;

	/**
	 * \brief table record setter
	 *
	 * Allows to insert only one record in a table.
	 * \param table The name of the SQL table in which the record will be inserted.
	 * \param values The record to insert in the table.
	 * \param isAtomic A flag to do the operations in an atomic way.
	 * \return The success or failure of the operation.
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
	 * \return The success or failure of the operation.
	 */
	virtual bool insert(const std::string& table, const std::vector<std::map<std::string , std::string>>& values = std::vector<std::map<std::string , std::string >>(), const bool& isAtomic = true) = 0;

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
	virtual bool modify(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& insertIfNotExists = true, const bool& isAtomic = true) noexcept = 0;

	/**
	 * \brief table record setter
	 *
	 * Allows to delete a record from a table.
	 * \param table The name of the SQL table in which the record will be removed.
	 * \param refFields The reference fields values to identify the record to update in the table. If empty, all records in the table will be deleted;
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return The success or failure of the operation.
	 */
	virtual bool remove(const std::string& table, const std::map<std::string, std::string>& refFields, const bool& isAtomic = true) = 0;

	/**
	 * \brief table record setter
	 *
	 * Allows to link 2 records from 2 different tables. A relationship must exists between those tables. If given records do not exist in tables, they are inserted.
	 * \param table1 The name of the first SQL table that contains the first record to link.
	 * \param record1 The first record in table1 to link.
	 * \param table2 The name of the second SQL table that contains the second record to link.
	 * \param record2 The second record in table2 to link.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return The success or failure of the operation.
	 */
	virtual bool linkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool & isAtomic = true) = 0;

	/**
	 * \brief table record setter
	 *
	 * Allows to unlink 2 records from 2 different tables. A relationship must exists between those tables. If given records do not exist in tables, nothing is done since there is nothing to unlink.
	 * \param table1 The name of the first SQL table that contains the first record to unlink.
	 * \param record1 The first record in table1 to link.
	 * \param table2 The name of the second SQL table that contains the second record to unlink.
	 * \param record2 The second record in table2 to link.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return The success or failure of the operation.
	 */
	virtual bool unlinkRecords(const std::string& table1, const std::map<std::string, std::string>& record1, const std::string& table2, const std::map<std::string, std::string>& record2, const bool & isAtomic = true) = 0;

	/**
	 * \brief table record getter
	 *
	 * Allows to obtain all records linked to a specific record.
	 * \param table The name of the SQL table that contains the record to take as reference.
	 * \param record The record in table to find.
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return All the records linked to the specified record organized by tables.
	 */
	virtual std::map<std::string, std::vector<std::map<std::string,std::string>>> getLinkedRecords(const std::string& table, const std::map<std::string, std::string>& record, const bool & isAtomic = true) const = 0;

	/**
	 * \brief database status check
	 *
	 * Allows to check the status of a database. It could be used with a file that describes the schemas of the database. This methods, if properly implemented, may allow to have a migration mechasnism.
	 * \param isAtomic A boolean to indicates that the operation should be done in an atomic way.
	 * \return The success or failure of the operation.
	 */
	virtual bool checkDefaultTables(const bool& isAtomic = true) { return true; };

	/**
	 * \brief table listing method
	 *
	 * Lists all table names of the database.
	 *
	 * \param isAtomic A flag to operates the modifications in an atomic way.
	 * \return The list of table names of the database.
	 */
	virtual std::vector< std::string > listTables(const bool& isAtomic = true) const = 0;

	/**
	 * \brief database configuration file setter
	 *
	 * Sets the database configuration file.
	 *
	 * \param databaseConfigurationFile The new database configuration file value.
	 */
	virtual void setDatabaseConfigurationFile(const std::string& databaseConfigurationFile = "") = 0;

	/**
	 * \brief table dump method
	 *
	 * Dumps the whole database to a visually formated string.
	 *
	 * \param dumpTableName A specific table to dump (if specified, we will only dump that table, otherwise, we will dump the whole database)
	 *
	 * \return A string representing (visually) the requested data.
	 */
	virtual std::string to_string(const std::string& dumpTableName = "") const = 0;
	
	/**
	 * \brief table dump method
	 *
	 * Overload the operator<< to allow dumping table to a stream
	 */
	friend std::ostream& operator<< (std::ostream& out, const DBManager& dbManager) {
		out << dbManager.to_string();
		return out;
        }
	
	/**
	 * \brief table dump method
	 *
	 * Dumps all table infos and contents of the database in a HTML formated string.
	 *
	 * \return string The HTML formated string containing infos and contents of tables of the database.
	 */
	virtual std::string dumpTablesAsHtml() const = 0;
	
};

#endif //_DBMANAGER_HPP_
