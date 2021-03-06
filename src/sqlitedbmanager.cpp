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
#include "sqlitedbmanager.hpp"
#include <fstream>
#include <unistd.h>	/* For access() */
#include <cstdlib>	/* For strtol() */

using namespace SQLite;
using namespace std;

/**
 * \brief Tests if a file is readable
 *
 * \param filename The name (PATH) of the file to test
 * \return true if the file is readable, false otherwise
 */
inline bool fileIsReadable(const std::string& filename) {
	return (access(filename.c_str(), R_OK) == 0);
}

/* ### Useful note ###
 *
 * Methods that affect the database are built this way : they are separated in 2 methods.
 * One with the regular name and the other one suffixed with "Core".
 * The 'core' method contains all SQL statements while the other method manage atomicity
 * of the operations thanks to transactions and mutex.
 *
 * The atomicity of each operation follows the same pattern :
 * Step 1  : Is the flag set ?
 * Step 2a : The flag is set so we try to take the mutex. If the operation shall modify the database, we also init a transaction.
 * Step 3a : If the return of the 'core' method is true, we commit the transaction.
 * Step 4a : We release the mutex.
 * Step 5a : We return the result of the 'core' method.
 * Step 2b : We just return the result of the 'core' method without considering any mutex nor transaction.
 */

/* Fix for sqlitecpp v2.0.0 and above */
#ifndef SQLITE_OPEN_READWRITE
#define SQLITE_OPEN_READWRITE OPEN_READWRITE
#endif
#ifndef SQLITE_OPEN_CREATE
#define SQLITE_OPEN_CREATE OPEN_CREATE
#endif
SQLiteDBManager::SQLiteDBManager(const std::string& filename,
                                 const std::string& configurationDescriptionFile) :
			filename(filename),
			configurationDescriptionFile(configurationDescriptionFile),
			mut(),
			db(new Database(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE)) {

	this->db->exec("PRAGMA foreign_keys = ON");	/*Activation of foreign key support in SQLite database */
	if (!this->checkDefaultTables()) {			  /* Will proceed migration if some changes are detected between configuration file and database state */
		if (this->db != NULL) {	/* Release memory... we are failing at construction */
			delete this->db;
			this->db = NULL;
		}
		throw invalid_argument("Badly-formatted XML configuration description");	/* An error occured during database migration , throw and exception*/
	}
}

SQLiteDBManager::~SQLiteDBManager() noexcept {
	if (this->db != NULL) {
		delete this->db;
		this->db = NULL;
	}
}

const string SQLiteDBManager::escDQ(const std::string& in) const {
	std::string escaped(in);
	std::string::size_type n = 0;
	
	while (( n = escaped.find('"', n)) != std::string::npos ) {
		/* Found one occurence */
		escaped.replace(n,
		                1, /* Replace one character (") */
		                "\"\"");	/* Replace with two double quotes characters */
		n += 2;	/* 2 characters added, skip them for next round */
	}
	return escaped;
}

bool SQLiteDBManager::checkDefaultTables(const bool& isAtomic) {
	if (isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*(this->db));
		if (this->checkDefaultTablesCore()) {
			transaction.commit();
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return this->checkDefaultTablesCore();
	}
}

bool SQLiteDBManager::checkDefaultTablesCore() {

	try {
		bool validXmlContent = false;	/* Do we consider the input XML as valid ? */
		bool result = false;
		TiXmlDocument doc;
		/* Load the default table model base on the provided input XML definition
		 * This XML can be provided inside a file (this->configurationDescriptionFile then contains the PATH to this file)
		 * or it can be provided directly as a string buffer (this->configurationDescriptionFile then stores the actual XML content)
		 */

		if (fileIsReadable(this->configurationDescriptionFile)) { /* We first check if this->configurationDescriptionFile is an existing file... */
#ifdef DEBUG
			cout << "Reading XML database description from file " + this->configurationDescriptionFile << endl;
#endif
			doc.LoadFile(this->configurationDescriptionFile.c_str());
			validXmlContent = true;
		}
		else { /* ...as a second chance, we try to parse this->configurationDescriptionFile directly as XML */
			validXmlContent = (doc.Parse(this->configurationDescriptionFile.data()) == NULL);
#ifdef DEBUG
			if (validXmlContent) {
				cout << "Read XML database description directly from provided buffer" << endl;
			}
#endif
		}

		if (validXmlContent) {
			vector<SQLTable> tables;
			map<string, vector<map<string, string>>> defaultRecords;
			/*
			 * The expect structure the configuration file is :
			 * <database>
			 * 	<table name="...">
			 * 		<field name="..." default-value="..." is-not-null="..." is-unique="..." />
			 * 		<field name="..." default-value="..." is-not-null="..." is-unique="..." />
			 * 		<default-records>
			 * 			<record>
			 * 				<field name="..." value="..." />
			 * 				<field name="..." value="..." />
			 * 			</record>
			 * 			<record>
			 * 				<field name="..." value="..." />
			 * 				<field name="..." value="..." />
			 * 			</record>
			 * 		</default-records>
			 * 	</table>
			 * 	<table name="...">
			 * 		<field name="..." default-value="..." is-not-null="..." is-unique="..." />
			 * 		<field name="..." default-value="..." is-not-null="..." is-unique="..." />
			 * 		<default-records>
			 * 			<record>
			 * 				<field name="..." value="..." />
			 * 				<field name="..." value="..." />
			 * 			</record>
			 * 			<record>
			 * 				<field name="..." value="..." />
			 * 				<field name="..." value="..." />
			 * 			</record>
			 * 		</default-records>
			 * 	</table>
			 * 	<!-- kind possible value : m:n -->
			 * 	<!-- policy possible value : none, link-all -->
			 * 	<relationship kind="..." policy="..." first-table="..." second-table="..." />
			 * 	<relationship kind="..." policy="..." first-table="..." second-table="..." />
			 * </database>
			 */
			TiXmlElement *dbElem = doc.FirstChildElement();
			//We check first "basics" tables
			if(dbElem && (string(dbElem->Value()) == "database")) {
				TiXmlElement *tableElem = dbElem->FirstChildElement();
				while(tableElem) {
					if(string(tableElem->Value()) == "table") {
						SQLTable table(tableElem->Attribute("name"));
						TiXmlElement *fieldElem = tableElem->FirstChildElement();
						while(fieldElem) {
							if(string(fieldElem->Value()) == "field") {
								string name = fieldElem->Attribute("name");
								string defaultValue = fieldElem->Attribute("default-value");
								bool isNotNull = (string(fieldElem->Attribute("is-not-null")) == "true");
								bool isUnique  = (string(fieldElem->Attribute("is-unique")) == "true");
								table.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isUnique));
							}
							else if(string(fieldElem->Value()) == "default-records") {
								TiXmlElement *recordElem = fieldElem->FirstChildElement();
								while(recordElem) {
									if(string(recordElem->Value()) == "record") {
										map<string, string> record;
										TiXmlElement *fieldValueElem = recordElem->FirstChildElement();
										while(fieldValueElem) {
											if(string(fieldValueElem->Value()) == "field") {
												string name = fieldValueElem->Attribute("name");
												string value = fieldValueElem->Attribute("value");
												record.emplace(name, value);
											}
											fieldValueElem = fieldValueElem->NextSiblingElement();
										}
										if(defaultRecords.find(table.getName()) != defaultRecords.end()) {
											defaultRecords[table.getName()].push_back(record);
										}
										else {
											defaultRecords.emplace(table.getName(), vector<map<string, string>>({record}));
										}
									}
									recordElem = recordElem->NextSiblingElement();
								}
							}
							fieldElem = fieldElem->NextSiblingElement();
						}
						tables.push_back(table);
					}
					tableElem = tableElem->NextSiblingElement();
				}
			}

			//Then we check relations in order to add foreign keys and create tables for m:n relationships.
			dbElem = doc.FirstChildElement();
			set<string> relationShipTables;	//Tables creation for relationship purpose.
			map<string, string> relationshipPolicies;
			map<string, vector<string>> relationshipLinkedTables;
			set<string> referencedTables;
			if(dbElem && (string(dbElem->Value()) == "database")) {
				TiXmlElement *relationElem = dbElem->FirstChildElement();
				while(relationElem) {
					if(string(relationElem->Value()) == "relationship") {
								string kind = relationElem->Attribute("kind");
								if(kind == "m:n") {
									string firstTableName = relationElem->Attribute("first-table");
									string secondTableName = relationElem->Attribute("second-table");

									for(auto &it : tables) {
										if(it.getName() == firstTableName || it.getName() == secondTableName) {
											it.markReferenced();
										}
									}
									vector<string> linkedtables;
									linkedtables.push_back(firstTableName);
									linkedtables.push_back(secondTableName);
									string relationshipTableName = this->createRelationCore(relationElem->Attribute("kind"), linkedtables);
									relationShipTables.emplace(relationshipTableName);
									relationshipPolicies.emplace(relationshipTableName, relationElem->Attribute("policy"));
									relationshipLinkedTables.emplace(relationshipTableName, linkedtables);
									referencedTables.emplace(firstTableName);
									referencedTables.emplace(secondTableName);
								}
					}
					relationElem = relationElem->NextSiblingElement();
				}
			}
			result = true;

			//Check if tables in database match parsed models
			for(auto &table : tables) {
				result = result && this->checkTableInDatabaseMatchesModelCore(table);
			}

			//Insert the default record specified in the configuration file if the concerned table is empty.
			for(auto &it : defaultRecords) {
				if(this->getCore(it.first).empty()) {
					result = result && this->insertCore(it.first, it.second);
				}
			}

			//Policy application for all relationship tables.
			for(auto &it : relationShipTables) {
				result = result && this->applyPolicyCore(it, relationshipPolicies[it], relationshipLinkedTables[it]);
			}

			if(tables.empty()) {
				cerr << "WARNING: Be careful there is no table in the database configuration file." << endl;
			}

			//Remove tables that are present in database but not in models
			set<string> sqliteSpecificTables;		//Tables not to delete if they exist because they are necessary for internal sqlite behavior.
			sqliteSpecificTables.emplace("sqlite_sequence");

			//We get all tables in database.
			vector<string> tablesInDbTmp = listTablesCore();
			set<string> tablesInDb;
			for(auto &it :tablesInDbTmp) {
				tablesInDb.emplace(it);
			}

			//We remove from this list the modelized tables
			for(auto &table : tables) {
				if(tablesInDb.find(table.getName()) != tablesInDb.end()) {
					tablesInDb.erase(tablesInDb.find(table.getName()));
				}
			}

			//We remove from this list the internal sqlite tables tables
			for(auto &it : sqliteSpecificTables) {
				if(tablesInDb.find(it) != tablesInDb.end()) {
					tablesInDb.erase(tablesInDb.find(it));
				}
			}

			//We remove from this list the relationship tables
			for(auto &it : relationShipTables) {
				if(tablesInDb.find(it) != tablesInDb.end()) {
					tablesInDb.erase(tablesInDb.find(it));
				}
			}

			//We remove the unnecessary relationship tables and we list the tables that are referenced so we can remove them properly after removed relationship tables.
			set<string> tablesToDeleteInSecond;
			for(auto &it : tablesInDb) {
				if(this->isReferencedCore(it) && this->getPrimaryKeysCore(it).size() == 1) {
					tablesToDeleteInSecond.emplace(it);
				}
				else {
					result = result &&  this->deleteTableCore(it);
				}
			}

			//We remove those referenced tables.
			for(auto &it : tablesToDeleteInSecond) {
				result = result &&  this->deleteTableCore(it);
			}

			//Unmarking referenced tables that should't be referenced anymore.
			for(auto &it : this->listTablesCore()) {
				if(this->isReferencedCore(it) && (referencedTables.find(it) == referencedTables.end()) && (relationShipTables.find(it) == relationShipTables.end())) {
					this->unmarkReferencedCore(it);
				}
			}
		}
		else {
			throw string("Unable to load any configuration file.");
		}

		return result;
	}
	catch(const string &e) {
		cerr << "Exception caught while reading database description file :" << endl << e << endl;
		return false;
	}
	catch(const exception &e) {
		cerr << "Exception caught while reading database description file :" << endl << e.what() << endl;
		return false;
	}
}

void SQLiteDBManager::checkTableInDatabaseMatchesModel(const SQLTable& model,
                                                       const bool& isAtomic) noexcept {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*(this->db));
		if(this->checkTableInDatabaseMatchesModelCore(model))
			transaction.commit();
	}
	else {
		this->checkTableInDatabaseMatchesModelCore(model);
	}
}

bool SQLiteDBManager::checkTableInDatabaseMatchesModelCore(const SQLTable& model) noexcept {
	bool result = true;
	//Create table if it doesn't exist in the database.
	if (!this->db->tableExists(model.getName())) {
		result = result && this->createTableCore(model);
	}
	else {
		SQLTable tableInDb = this->getTableFromDatabaseCore(model.getName());

		//Add new fields and remove useless ones from the existing table in database if it differ from model.
		if(model != tableInDb) {
			result = result && this->addFieldsToTableCore(tableInDb.getName(), model.diff(tableInDb));
			result = result && this->removeFieldsFromTableCore(tableInDb.getName(), tableInDb.diff(model));
		}
	}
	return result;
}

bool SQLiteDBManager::createTable(const SQLTable& table,
                                  const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*(this->db));

		bool result = this->createTableCore(table);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->createTableCore(table);
	}
}

bool SQLiteDBManager::createTableCore(const SQLTable& table) noexcept {
	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "CREATE TABLE \"" << this->escDQ(table.getName()) << "\" (";
		const vector< tuple<string,string,bool,bool> > fields = table.getFields();

		if (table.isReferenced()) {
			ss << "\"id\" INTEGER PRIMARY KEY AUTOINCREMENT, ";
		}

		for (vector< tuple<string,string,bool,bool> >::const_iterator it = fields.begin(); it != fields.end(); ++it) {
			/* Check if iterator is on the first element of the list, and add a separator otherwise */
			if (it != fields.begin()) {
				ss << ", ";
			}
			const string& fieldName = std::get<0>(*it);
			const string& defaultValue = std::get<1>(*it);
			const bool& notNullProperty = std::get<2>(*it);
			const bool& uniqueProperty = std::get<3>(*it);
			//0 -> field name
			//1 -> field default value
			//2 -> field is not null
			//3 -> field is unique
			ss << "\"" << this->escDQ(fieldName) << "\" TEXT ";
			if (notNullProperty)
				ss << "NOT NULL ";
			if (uniqueProperty)
				ss << "UNIQUE ON CONFLICT ABORT ";
			ss << "DEFAULT \"" << this->escDQ(defaultValue) << "\"";
		}


		ss << ")";

		this->db->exec(ss.str());
		return true;
	}
	catch(const Exception & e) {
		cerr << "createTableCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::addFieldsToTable(const std::string& table,
                                       const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields,
                                       const bool& isAtomic) noexcept {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));

		bool result = this->addFieldsToTableCore(table, fields);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->addFieldsToTableCore(table, fields);
	}
}

bool SQLiteDBManager::addFieldsToTableCore(const std::string& table,
                                           const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept {

	/* The logical steps to follow in this methods :
	 *                     [case where table is not referenced] -> (4) -> (5) -> (6) -------------------------------------
	 * 					  /                                                                                               \
	 * (1) -> (2) -> (3)--																								   --> result
	 * 					  \																								  /
	 * 					   [case where table is referenced] -> (7) -> (8) -> (9) -> (10) -> (11) -> (12) -> (13) -> (14) -
	 */
	try {
		bool result = true;

		if(!fields.empty()) {
			//(1) We save the current table
			SQLTable newTable = this->getTableFromDatabaseCore(table);
			//(2) Then we save table's records
			vector<map<string, string>> records = this->getCore(table);

			//(3) We can add the new fields informations
			for(auto &it : fields) {
				string name = std::get<0>(it);
				string dv = std::get<1>(it);
				bool nn = std::get<2>(it);
				bool unique = std::get<3>(it);
				newTable.addField(tuple<string,string,bool,bool>(name, dv, nn, unique));
			}

			if(!newTable.isReferenced()) {
				//(4) Now that everything is saved, we can drop the "old" table
				result = result && this->deleteTableCore(table);
				if(!result)
					return result;

				//(5) We can recreate the table
				result = result &&this->createTableCore(newTable);
				if(!result)
					return result;

				//(6) The new table is created, populate with old records (new fields will have default value)
				result = result &&this->insertCore(newTable.getName(), records);
			}
			else {
				//(7) We'll search if a join table (or more) exists. If so, the table is referenced for a m:n relationship, otherwise it's a 1:n or a 1:1 relationship.
				set<string> linkingTables;
				for(auto &name : this->listTablesCore()) {
					if(name.find(table + "_") != string::npos || name.find("_" + table) != string::npos) {
						linkingTables.emplace(name);
					}
				}
				//No need to check field properties of linking tables as these tables fit a specific model : 2 integer column noted as primary keys and referencing the primary keys of 2 tables.

				if(!linkingTables.empty()) {	//TODO: Handle the 1:1 and 1:n relationships cases.
					//(8) Now we have all the linker tables names, we can fetch their records.
					map<string, vector<map<string, string>>> recordsByTable;
					for(auto &name : linkingTables) {
						recordsByTable.emplace(name, this->getCore(name));
					}
					//(9) Now the linking Tables are saved, we can drop them
					for(auto &name : linkingTables) {
						result = result && this->deleteTableCore(name);
						if(!result)
							return result;
					}
					//(10) We can now drop our referenced table
					result = result && this->deleteTableCore(newTable.getName());
					if(!result)
						return result;
					//(11) We can recreate our referenced table
					result = result && this->createTableCore(newTable);
					if(!result)
						return result;
					//(12) We can populate it
					result = result && this->insertCore(newTable.getName(), records);
					if(!result)
						return result;
					//(13) Now that the table is recreated we can recreate the linking tables
					for(auto &name : linkingTables) {
						string nameOfOthertable;
						vector<string> tables;
						if(name.find(table + string("_")) != string::npos) {
							string temp = table + "_";
							size_t start = temp.length();
							size_t length = name.length()-temp.length();
							nameOfOthertable = name.substr(start, length);
							tables.push_back(table);
							tables.push_back(nameOfOthertable);
						}
						else {
							string temp = "_" + table;
							size_t start = 0;
							size_t length = name.length()-temp.length();
							nameOfOthertable = name.substr(start, length);
							tables.push_back(nameOfOthertable);
							tables.push_back(table);
						}
						result = result && (name == this->createRelationCore("m:n", tables));
						if(!result)
							return result;
					}

					//(14) Now the linking tables are recreated we can populate them
					for(auto &name : linkingTables) {
						result = result && this->insertCore(name, recordsByTable[name]);
						if(!result)
							return result;
					}
				}
			}
		}

		return result;
	}
	catch(const Exception &e) {
		cerr << "addFieldsToTableCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::removeFieldsFromTable(const std::string& table,
                                            const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields,
                                            const bool& isAtomic) noexcept {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		bool result = this->removeFieldsFromTableCore(table, fields);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->removeFieldsFromTableCore(table, fields);
	}
}

bool SQLiteDBManager::removeFieldsFromTableCore(const std::string& table,
                                                const std::vector<std::tuple<std::string, std::string, bool, bool> >& fields) noexcept {

	/* The logical steps to follow in this methods :
	 *                     [case where table is not referenced] -> (4) -> (5) -> (6) -------------------------------------
	 * 					  /                                                                                               \
	 * (1) -> (2) -> (3)--																								   --> result
	 * 					  \																								  /
	 * 					   [case where table is referenced] -> (7) -> (8) -> (9) -> (10) -> (11) -> (12) -> (13) -> (14) -
	 */
	try {
		bool result = true;
		if(!fields.empty()) {
			//(1) We save the current table
			SQLTable newTable = this->getTableFromDatabaseCore(table);

			//(2) We can remove the fields
			for(auto &current : newTable.getFields()) {
				for(auto &toDelete : fields) {
					if(toDelete == current) {
						cout << "Removing field " << std::get<0>(current) << endl;
						newTable.removeField(std::get<0>(current));
					}
				}
			}

			//(3) Then we save table's records
			vector<string> columns;
			for(auto &it : newTable.getFields()) {
				columns.push_back(std::get<0>(it));
			}
			vector<map<string, string>> records = this->getCore(table, columns);

			if(!newTable.isReferenced()) {
				//(4) Now that everything is saved, we can drop the "old" table
				result = result && this->deleteTableCore(table);
				if(!result)
					return result;

				//(5) We can recreate the table
				result = result &&this->createTableCore(newTable);
				if(!result)
					return result;
	
				//(6) The new table is created, populate with old records (new fields will have default value)
				result = result &&this->insertCore(newTable.getName(), records);
			}
			else {
				//(7) We'll search if a join table (or more) exists. If so, the table is referenced for a m:n relationship, otherwise it's a 1:n or a 1:1 relationship.
				set<string> linkingTables;
				for(auto &name : this->listTablesCore()) {
					if(name.find(table + "_") != string::npos || name.find("_" + table) != string::npos) {
						linkingTables.emplace(name);
					}
				}
				//No need to check field properties of linking tables as these tables fit a specific model : 2 integer column noted as primary keys and referencing the primary keys of 2 tables.

				if(!linkingTables.empty()) {	//TODO: Handle the 1:1 and 1:n relationships cases.
					//(8) Now we have all the linker tables names, we can fetch their records.
					map<string, vector<map<string, string>>> recordsByTable;
					for(auto &name : linkingTables) {
						recordsByTable.emplace(name, this->getCore(name));
					}
					//(9) Now the linking Tables are saved, we can drop them
					for(auto &name : linkingTables) {
						result = result && this->deleteTableCore(name);
						if(!result)
							return result;
					}
					//(10) We can now drop our referenced table
					result = result && this->deleteTableCore(newTable.getName());
					if(!result)
						return result;
					//(11) We can recreate our referenced table
					result = result && this->createTableCore(newTable);
					if(!result)
						return result;
					//(12) We can populate it
					result = result && this->insertCore(newTable.getName(), records);
					if(!result)
						return result;
					//(13) Now that the table is recreated we can recreate the linking tables
					for(auto &name : linkingTables) {
						string nameOfOthertable;
						vector<string> tables;
						if(name.find(table + string("_")) != string::npos) {
							string temp = table + "_";
							size_t start = temp.length();
							size_t length = name.length()-temp.length();
							nameOfOthertable = name.substr(start, length);
							tables.push_back(table);
							tables.push_back(nameOfOthertable);
						}
						else {
							string temp = "_" + table;
							size_t start = 0;
							size_t length = name.length()-temp.length();
							nameOfOthertable = name.substr(start, length);
							tables.push_back(nameOfOthertable);
							tables.push_back(table);
						}
						result = result && (name == this->createRelationCore("m:n", tables));
						if(!result)
							return result;
					}

					//(14) Now the linking tables are recreated we can populate them
					for(auto &name : linkingTables) {
						result = result && this->insertCore(name, recordsByTable[name]);
						if(!result)
							return result;
					}
				}
			}
		}

		return result;
	}
	catch(const Exception &e) {
		cerr << "removeFieldsFromTableCore: " << e.what() << endl;
		return false;
	}
}


bool SQLiteDBManager::deleteTable(const std::string& table,
                                  const bool& isAtomic) noexcept {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*(this->db));

		bool result = this->deleteTableCore(table);
		if (result)
			transaction.commit();
		return result;
	}
	else {
		return this->deleteTableCore(table);
	}
}

bool SQLiteDBManager::deleteTableCore(const std::string& table) noexcept {
	string ss;

	ss = "DROP TABLE \"" + this->escDQ(table) + "\"";

	try {
		this->db->exec(ss);
		return true;
	}
	catch(const Exception & e) {
		cerr << __func__ << "(): exception while running SQL cmd \"" << ss << "\": " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::createTable(const std::string& table,
                                  const std::map< std::string, std::string >& values,
                                  const bool& isAtomic) {

	SQLTable tab(table);
	for(const auto &it : values) {
		tab.addField(tuple<string, string, bool, bool>(it.first, it.second, true, false));
	}
	return this->createTable(tab, isAtomic);
}

std::vector< std::string > SQLiteDBManager::listTables(const bool& isAtomic) const {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return listTablesCore();
	}
	else {
		return listTablesCore();
	}
}

std::vector< std::string > SQLiteDBManager::listTablesCore() const {
	//All the tables names are in the sqlite_master table.
	try {
		vector<string> tablesInDb;
		for(auto &it : getCore("sqlite_master")) {
			if(it["type"] == "table")
				tablesInDb.push_back(it["name"]);
		}

		return tablesInDb;
	}
	catch(const Exception &e) {
		cerr << "listTablesCore: " << e.what() << endl;
		return vector<string>();
	}
}

bool SQLiteDBManager::areForeignKeysEnabled() const {
	Statement query(*(this->db), "PRAGMA foreign_keys");
	while (query.executeStep()) {
		if (string(query.getColumn(0).getText()) == "1") {
			return true;
		}
		else {
			//cout << "Text of pragma is " << query.getColumn(0).getText() << endl;
		}
	}
	return false;
}

std::string SQLiteDBManager::tableToString(const std::string& tableName) const {

	stringstream dump;

	const vector<map<string, string> > records = this->get(tableName); // Get all records
	map<string, bool> uniqueness = this->getUniqueness(tableName);	// Get the list of unique records for this table

	if (!records.empty()) {
		/* Compute the longest name for each column (between column names and values) */
		map<string, unsigned int> longests;	/* string is the column name, unsigned int is the column width (in characters) */
		for (vector<map<string, string> >::const_iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
			for (map<string, string>::const_iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
				/* Initialise count... if column is not known yet, set the length to fit the column name */
				const string& columnName = mapIt->first;
				const string& recordValue = mapIt->second;

				if (longests.find(columnName) == longests.end()) {	/* New column (absent from map "longests" */
					unsigned int columnHeaderSz = columnName.size();	/* Start from the size of the column */
					if (this->isPrimaryKeyCore(tableName, columnName)) {	/* This column stores primary keys */
						columnHeaderSz+=5;	/* Get more room for suffix " [PK]" (5 chars) */
					}
					if (uniqueness[columnName]) {	/* This column stores unique entries */
						columnHeaderSz+=4;	/* Get more room for suffix " [U]" (4 chars) */
					}
					longests.emplace(columnName, columnHeaderSz);
				}

				/* Only if we find a value that is larger than the column name, increase the max length */
				if (recordValue.size() > longests[columnName]) {
					longests[columnName] = recordValue.size();
				}
			}
		}

		/* Write the first line: column names */
		stringstream Hsep;	/* Horizontal separator between rows (records) */
		stringstream headers;	/* Headers line */
		Hsep << "+-";	/* Start with left border */
		headers << "| ";
		for (map<string, string>::const_iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); ++mapIt) {
			/* Check if iterator is on the first column, and add a separator otherwise */
			if (mapIt != records.at(0).begin()) {	/* If not the first column of the table */
				Hsep << "-+-";	/* Add separators */
				headers << " | ";
			}

			const string& columnName = mapIt->first;

			Hsep << string(longests[columnName], '-');	/* Fill hypens */

			string columnHeader(columnName);
			if (this->isPrimaryKeyCore(tableName, columnName)) {	/* This column stores primary keys */
				columnHeader += " [PK]";
			}
			if (uniqueness[columnName]) {	/* This column stores unique entries */
				columnHeader += " [U]";
			}

			headers << columnHeader << string(longests[columnName]-columnHeader.size(), ' ');	/* Write header and pad with spaces */
		}
		Hsep << "-+";	/* Add right border */
		headers << " |";

		/* Once here, we have created the headers line and a Hsep separator to use between all records */
		stringstream values;
		for (vector<map<string, string> >::const_iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
			values << "| ";	/* Start with left border */
			for (map<string, string>::const_iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
				/* Check if iterator is on the first column, and add a separator otherwise */
				if (mapIt != vectIt->begin()) {	/* If not the first column of the table */
					values << " | ";	/* Add separator */
				}
				const string& columnName = mapIt->first;
				const string& recordValue = mapIt->second;
				values << recordValue << string(longests[columnName]-recordValue.size(), ' ');	/* Write value for this record (row) and pad with spaces */
			}
			values << " |" << endl; /* Add right border */
		}

		stringstream tableDump;
		tableDump << "Table: " << tableName << endl;
		tableDump << Hsep.str() << endl;
		tableDump << headers.str() << endl;
		tableDump << Hsep.str() << endl;
		tableDump << values.str();	/* Note: values already terminates with endl */
		tableDump << Hsep.str() << endl;

		dump << tableDump.str();
	}
	else {	/* If we reach here, records.empty() == true */
		set<string> columnNames = this->getFieldNames(tableName);

		dump << "Table: " << tableName << " is empty" << endl;
		dump << "Columns are: ";

		for (set<string>::const_iterator setIt = columnNames.begin(); setIt != columnNames.end(); ++setIt) {
			if (setIt != columnNames.begin()) {
				dump << ", ";
			}
			dump << *setIt;
			if (this->isPrimaryKeyCore(tableName, *setIt)) {	/* This column stores primary keys */
				dump << " [PK]";
			}
			if (uniqueness[*setIt]) {	/* This column stores unique entries */
				dump << " [U]";
			}
		}
		dump << endl;
	}
	return dump.str();
}

std::string SQLiteDBManager::to_string(const std::string& dumpTableName) const {
	stringstream dump;

#ifdef DEBUG
	cout << __func__ << "(): Starting parsing database" << endl;
#endif
	const vector< string > tables = this->listTables();
	
	bool specificTableFound = false;	/* Did we find a specific table we were asked to dump? */
	if (this->areForeignKeysEnabled()) {
		dump << "Foreign Keys are enabled" << endl;
	}
	else {
		dump << "Foreign Keys are disabled" << endl;
	}
	dump << "Database stored in " << this->filename << endl << "List of tables (" << tables.size() << "): ";
	for (vector<string>::const_iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		if (tableName != tables.begin()) {
			dump << ", ";
		}
		if (*tableName == dumpTableName) {	/* We have found the table name that we were requested to dump */
			specificTableFound = true;
		}
		dump << "\"" << *tableName << "\"";
	}
	dump << endl;

	if (dumpTableName != "") {	/* A table name was specified as argument */
		if (specificTableFound) {
			/* The specified table was found... dump only this table (not the whole db) */
			return this->tableToString(dumpTableName);
		}
		else {	/* The specified table was not found */
#ifdef DEBUG
			cerr << __func__ << "(): The table specified (\"" << dumpTableName << "\") does not exist. Dumping all database instead" << endl;
#endif
		}
	}

	for (vector<string>::const_iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		/* tableName will loop through every table name in the database */
		dump << this->tableToString(*tableName);
	}
#ifdef DEBUG
	cout << __func__ << "(): Finished parsing database" << endl;
#endif
	return dump.str();
}

std::string SQLiteDBManager::dumpTablesAsHtml() const {
	stringstream htmlDump;
	htmlDump << "<!DOCTYPE html>";
	htmlDump << "<head>";
	htmlDump << "<title>Condutor tables dump</title>";
	htmlDump <<	"<link rel=\"icon\" href=\"../favicon.ico\">";
	htmlDump <<	"<!-- Bootstrap core CSS -->";
	htmlDump <<	"<link href=\"../css/bootstrap.css\" rel=\"stylesheet\">";
	htmlDump <<	"<!-- HTML5 shim and Respond.js IE8 support of HTML5 elements and media queries -->";
	htmlDump <<	"<!--[if lt IE 9]>";
	htmlDump <<	"<script src=\"../js/html5shiv.min.js\"></script>";
	htmlDump <<	"<script src=\"../js/respond.min.js\"></script>";
	htmlDump <<	"<![endif]-->";
	htmlDump <<	"<!-- [if (It IE 9) & (!IEMobile)]>";
	htmlDump <<	"<script src=\"../js/css3-mediaqueries.js\"></script>";
	htmlDump <<	"<![endif]-->";
	htmlDump << "</head>";
	htmlDump << "<body>";
	htmlDump << "<h1> Dump of Conductor Tables </h1>";
	
	if (this->areForeignKeysEnabled()) {
		htmlDump << "<p> Foreign Keys are enabled </p>";
	}
	else {
		htmlDump << "<p> Foreign Keys are disabled </p>";
	}

	const vector< string > tables = this->listTables();

	for (vector<string>::const_iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		/* tableName will loop through every table name in the database */
		htmlDump << "<h3> Table : " << *tableName << "</h3>";
		vector<map<string, string> > records = this->get(*tableName); // Get all records
		map<string, bool> uniqueness = this->getUniqueness(*tableName);

		if (!records.empty()) {
			htmlDump << "<table class=\"table table-striped table-bordered table-hover\">";
			htmlDump << "<thead>";
			htmlDump << "<tr>";
			/* Write the first line: column names */
			for (map<string, string>::iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); ++mapIt) {
				const string& columnName = mapIt->first;
				
				htmlDump << "<th>" << columnName;
				if (this->isPrimaryKeyCore(*tableName, columnName)) {
					htmlDump << " [PK] ";
				}
				if (uniqueness[columnName]) {
					htmlDump << " [U] ";
				}
				htmlDump << "</th>";
			}
			htmlDump << "</tr>";
			htmlDump << "</thead>";
			
			/* Write all records, one record per table row, one field in each table cell */
			htmlDump << "<tbody>";
			for (vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				htmlDump << "<tr>";
				for (map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					const string& recordValue = mapIt->second;
					htmlDump << "<td>" << recordValue << "</td>";
				}
				htmlDump << "</tr>";
			}
			htmlDump << "</tbody>";
			htmlDump << "</table>";
		}
		else {
			set<string> columnNames = this->getFieldNames(*tableName);
			
			htmlDump << "<table class=\"table table-striped table-bordered table-hover\">";
			htmlDump << "<thead>";
			htmlDump << "<tr>";
			/* Write the first line: column names */
			for (auto &it : columnNames) {
				htmlDump << "<th>" << it;
				if (this->isPrimaryKeyCore(*tableName, it)) {
					htmlDump << " [PK] ";
				}
				if (uniqueness[it]) {
					htmlDump << " [U] ";
				}
				htmlDump << "</th>";
			}
			htmlDump << "</tr>";
			htmlDump << "</thead>";
			htmlDump << "</table>";
		}
	}
	htmlDump << "</body>";

	return htmlDump.str();
}

bool SQLiteDBManager::isReferenced(const std::string& name,
                                   const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->isReferencedCore(name);
	}
	else {
		return this->isReferencedCore(name);
	}
}

bool SQLiteDBManager::isReferencedCore(const std::string& name) const {
	bool result = false;
	try {
		Statement query(*(this->db), "PRAGMA table_info(\"" + this->escDQ(name) + "\")");
		while(query.executeStep()) {
			////cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
			//+1 Because of behavior of the pragma.
			//The pk column is equal to 0 if the field isn't part of the primary key.
			//If the field is part of the primary key, it is equal to the index of the record +1
			//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
			if(query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
				result = true;
			}
		}
	}
	catch(const Exception &e) {
		cerr << "isReferencedCore: " << e.what() << endl;
	}
	return result;
}

std::set<std::string> SQLiteDBManager::getPrimaryKeys(const std::string& name,
                                                      const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getPrimaryKeysCore(name);
	}
	else {
		return this->getPrimaryKeysCore(name);
	}
}

std::set<std::string> SQLiteDBManager::getPrimaryKeysCore(const std::string& name) const {

	set<string> result;
	try {
		Statement query(*(this->db), "PRAGMA table_info(\"" + this->escDQ(name) + "\")");
		while (query.executeStep()) {
			////cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
			//+1 Because of behavior of the pragma.
			//The pk column is equal to 0 if the field isn't part of the primary key.
			//If the field is part of the primary key, it is equal to the index of the record +1
			//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
			if (query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
				result.emplace(query.getColumn(1).getText());
			}
		}
	}
	catch (const Exception &e) {
		cerr << "getPrimaryKeysCore: " << e.what() << endl;
	}
	return result;
}

std::map<std::string, std::string> SQLiteDBManager::getDefaultValues(const std::string& name,
                                                                     const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getDefaultValuesCore(name);
	}
	else {
		return this->getDefaultValuesCore(name);
	}
}

std::map<std::string, std::string> SQLiteDBManager::getDefaultValuesCore(const std::string& name) const {

	try {
		bool referenced = this->isReferencedCore(name);
		Statement query(*(this->db), "PRAGMA table_info(\"" + this->escDQ(name) + "\")");
		map<string, string> defaultValues;
		while(query.executeStep()) {
			string fieldName = query.getColumn(1).getText();
			if(!(referenced && (fieldName == PK_FIELD_NAME))) {
				string dv = query.getColumn(4).getText();
				if(dv.front() == '"' && dv.back() == '"') {
					dv = dv.substr(1, dv.length()-2);
				}
				defaultValues.emplace(fieldName, dv);
			}
		}

		return defaultValues;
	}
	catch(const Exception &e) {
		cerr << "getDefaultValues: " << e.what() << endl;
		return map<string, string>();
	}
}

std::map<std::string, bool> SQLiteDBManager::getNotNullFlags(const std::string& name,
                                                             const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getNotNullFlagsCore(name);
	}
	else {
		return this->getNotNullFlagsCore(name);
	}
}

std::map<std::string, bool> SQLiteDBManager::getNotNullFlagsCore(const std::string& name) const {

	try {
		bool referenced = this->isReferencedCore(name);
		Statement query(*(this->db), "PRAGMA table_info(\"" + this->escDQ(name) + "\")");
		map<string, bool> notNullFlags;
		while(query.executeStep()) {
			string fieldName = query.getColumn(1).getText();
			if(!(referenced && (fieldName == PK_FIELD_NAME)))
				notNullFlags.emplace(fieldName,(query.getColumn(3).getInt() == 1));
		}

		return notNullFlags;
	}
	catch(const Exception &e) {
		cerr << "getNotNullFlagsCore: " << e.what() << endl;
		return map<string, bool>();
	}
}

std::map<std::string, bool> SQLiteDBManager::getUniqueness(const std::string& name,
                                                           const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getUniquenessCore(name);
	}
	else {
		return this->getUniquenessCore(name);
	}
}

std::map<std::string, bool> SQLiteDBManager::getUniquenessCore(const std::string& name) const {

	try {
		bool referenced = this->isReferencedCore(name);
		//(1) We get the field list
		set<string> notUniqueFields = this->getFieldNamesCore(name);

		//(2) We obtain the unique indexes
		set<string> uniqueFields;
		Statement query2(*(this->db), "PRAGMA index_list(\"" + this->escDQ(name) + "\")");
		while(query2.executeStep()) {
			string fieldName = query2.getColumn(1).getText();
			if(!(referenced && (fieldName == PK_FIELD_NAME))) {
				if(query2.getColumn(2).getInt() == 1) {
					Statement query3(*(this->db), "PRAGMA index_info(\"" + this->escDQ(fieldName) + "\")");
					while(query3.executeStep()) {
						uniqueFields.emplace(query3.getColumn(2).getText());
					}
				}
			}
		}

		map<string, bool> uniqueness;
		for(auto &it : uniqueFields) {
			notUniqueFields.erase(notUniqueFields.find(it));
			uniqueness.emplace(it, true);
		}

		for(auto &it : notUniqueFields) {
			uniqueness.emplace(it, false);
		}

		return uniqueness;
	}
	catch(const Exception &e) {
		cerr << "getUniquenessCore: " << e.what() << endl;
		return map<string, bool>();
	}
}

std::string SQLiteDBManager::createRelation(const std::string& kind,
                                            const std::vector<std::string>& tables,
                                            const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		string result = this->createRelationCore(kind, tables);
		if(!result.empty())
			transaction.commit();
		return result;
	}
	else {
		return this->createRelationCore(kind, tables);
	}
}

std::string SQLiteDBManager::createRelationCore(const std::string& kind,
                                                const std::vector<std::string>& tables) {

	//m:n relationships
	if(kind == "m:n" && tables.size() == 2) {
		string table1 = tables.at(0);
		string table2 = tables.at(1);
		string relationName = table1 + "_" + table2;
		bool addIt = true;
		// We check that the joining table does not already exists in the database.
		for(auto &it : listTablesCore()) {
			if(it == relationName) {
				addIt = false;
			}
		}

		stringstream fieldName1;
		fieldName1 << table1 << "#" << PK_FIELD_NAME;
		stringstream fieldName2;
		fieldName2 << table2 << "#" << PK_FIELD_NAME;

		if(addIt) {
			//If the joining table needs to be created, we first check that referenced table have primary keys.
			//If they do not, we give them a primary key field.
			if(!this->isReferencedCore(table1)) {
				this->markReferencedCore(table1);
			}
			if(!this->isReferencedCore(table2)) {
				this->markReferencedCore(table2);
			}

			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			ss << "CREATE TABLE \"" << this->escDQ(relationName) << "\" (";

			ss << "\"" << this->escDQ(fieldName1.str())  << "\" INTEGER REFERENCES \"" << this->escDQ(table1) << "\"(\"" << this->escDQ(PK_FIELD_NAME) << "\"), ";
			ss << "\"" << this->escDQ(fieldName2.str()) << "\" INTEGER REFERENCES \"" << this->escDQ(table2) << "\"(\"" << this->escDQ(PK_FIELD_NAME) << "\"), ";
			ss << "PRIMARY KEY (\"" << this->escDQ(table1) << "#" << this->escDQ(PK_FIELD_NAME) << "\", \"" << this->escDQ(table2) << "#" << this->escDQ(PK_FIELD_NAME) << "\"))";

			//We use the referenced table primary keys as foreign keys (see m:n relationship theory if it bugs you).
			this->db->exec(ss.str());
		}

		return relationName;
	}
	else {
		return string();
	}
}

std::vector< std::map<std::string, std::string> > SQLiteDBManager::get(const std::string& table,
                                                                       const std::vector<std::string >& columns,
                                                                       const bool& distinct,
                                                                       const bool& isAtomic) const noexcept {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getCore(table, columns, distinct);
	}
	else {
		return this->getCore(table, columns, distinct);
	}
}

bool SQLiteDBManager::insert(const std::string& table,
                             const std::vector<std::map<std::string, std::string> >& values,
							 const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));

		bool result = this->insertCore(table, values);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->insertCore(table, values);
	}
}

bool SQLiteDBManager::modify(const std::string& table,
                             const std::map<std::string, std::string>& refFields,
                             const std::map<std::string, std::string>& values,
                             const bool& insertIfNotExists,
                             const bool& isAtomic) noexcept {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));

		bool result = this->modifyCore(table, refFields, values, insertIfNotExists);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->modifyCore(table, refFields, values, insertIfNotExists);
	}
}

bool SQLiteDBManager::remove(const std::string& table,
                             const std::map<std::string, std::string>& refFields,
                             const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));

		bool result = this->removeCore(table, refFields);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->removeCore(table, refFields);
	}
}

std::vector< std::map<std::string, std::string> > SQLiteDBManager::getCore(const std::string& table,
                                                                           const std::vector<std::string >& columns,
                                                                           const bool& distinct) const noexcept {

	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "SELECT ";

		if(distinct)
			ss << "DISTINCT ";

		vector<string > newColumns;
		bool getAllFields = (columns.empty() || (columns.size() == 1 && columns.at(0) == "*"));	/* Shall be process all fields of the table? (there no filter) */
		if (getAllFields) {
			ss << "*";
			//We fetch the names of table's columns in order to populate the map correctly
			//(With only * as columns name, we are notable to match field names to field values in order to build the map)
			Statement query(*(this->db), "PRAGMA table_info(\"" + this->escDQ(table) + "\");");
			while(query.executeStep())
				newColumns.push_back(query.getColumn(1).getText());
		}
		else {	/* There is a filter... insert the field names in the SQL command */
			for (vector<std::string >::const_iterator it = columns.begin(); it != columns.end(); ++it) {
				/* Check if iterator is on the first element of the list, and add a separator otherwise */
				if (it != columns.begin()) {
					ss << ", ";
				}
				ss << "\"" << this->escDQ(*it) << "\"";
				newColumns.push_back(*it);
			}
		}

		ss << " FROM \"" << this->escDQ(table) << "\"";
		
#ifdef DEBUG
		cout << __func__ << "(): running SQL query \"" << ss.str() << "\"" << endl;
#endif
		Statement query(*(this->db), ss.str());
		
		vector<map<string, string> > result;

		while(query.executeStep()) {
			map<string, string> record;

			for(int i = 0; i < query.getColumnCount(); ++i) {
				if(query.getColumn(i).isNull()) {
					record.emplace(newColumns.at(i), "");
				}
				else {
					record.emplace(newColumns.at(i), query.getColumn(i).getText());
				}
			}

			result.push_back(record);
		}

		return result;
	}
	catch(const Exception & e) {
		cerr << "getCore: " << e.what() << endl;
		return vector< map<string, string> >();
	}
}

bool SQLiteDBManager::insertCore(const std::string& table,
                                 const std::vector<std::map<std::string, string> >& values) {

	try {
		bool result = true;
		for (vector<map<string, string> >::const_iterator vectIt = values.begin(); vectIt != values.end(); ++vectIt) {
			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			//Only one request is used. We could have use one request for each record, but it would have been slower.
			ss << "INSERT INTO \"" << this->escDQ(table) << "\" ";
			if (!vectIt->empty()) {
				stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
				stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
				columnsName << "(";
				columnsValue << "(";
				for (map<string, string>::const_iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					/* Check if iterator is on the first element of the list, and add a separator otherwise */
					if (mapIt != vectIt->begin()) {
						columnsName << ",";
						columnsValue << ",";
					}
					columnsName << "\"" << this->escDQ(mapIt->first) << "\"";
					columnsValue << "\"" << this->escDQ(mapIt->second) << "\"";
				}
				columnsName << ")";
				columnsValue << ")";

				ss << columnsName.str() << " VALUES " << columnsValue.str() << ";";
			}
			else {
				ss << "DEFAULT VALUES";
			}
#ifdef DEBUG
			cout << __func__ << "(): running SQL query \"" << ss.str() << "\"" << endl;
#endif
			result = result && (this->db->exec(ss.str()) > 0);
		}

		return result;
	}
	catch (const Exception &e) {
		cerr  << __func__ << "(): " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::modifyCore(const std::string& table,
                                 const std::map<std::string, std::string>& refFields,
                                 const std::map<std::string, std::string>& values,
                                 const bool& insertIfNotExists) noexcept {

	if (values.empty()) return false;

	stringstream sql_where(ios_base::in | ios_base::out | ios_base::ate);
	if (!refFields.empty()) {
		sql_where << "WHERE ";
		for (map<string, string>::const_iterator it = refFields.begin(); it != refFields.end(); ++it) {
			/* Check if iterator is on the first element of the list, and add a separator otherwise */
			if (it != refFields.begin()) {
				sql_where << " AND ";
			}
			sql_where << "\"" << this->escDQ(it->first) << "\" = \"" << this->escDQ(it->second) << "\"";
		}
	}

	if (insertIfNotExists) {
		/* Count the number of records matching refFields filter */
		stringstream sql_cmd(ios_base::in | ios_base::out | ios_base::ate);
		sql_cmd << "SELECT COUNT(*) AS RecordCount FROM \"" << this->escDQ(table) << "\" " << sql_where.str();

		long int recordCount = 0;

#ifdef DEBUG
		cout << __func__ << "(): running SQL query \"" <<  sql_cmd.str() << "\"" << endl;
#endif
		Statement query(*(this->db), sql_cmd.str());

		while (query.executeStep()) {
			if (query.getColumnCount() != 1) {
				cerr  << "modifyCore: COUNT query returned more than one line" << endl;
			}
			if (query.getColumn(0).isNull()) {
				cerr  << "modifyCore: COUNT query returned empty line" << endl;
			}
			else {
				recordCount = strtol(query.getColumn(0).getText(), NULL, 0);
			}
		}
		//cout << "Got record count " << recordCount << "\n";

		if (recordCount == 0) {	/* No matching field exist in the database... we can't modify... try insertion instead */
			vector<map<string,string>> vals;
			map<string,string> insertedValues(values);	/* Initialise the values to insert with the values provided for modification */

			for (map<string, string>::const_iterator mapIt = refFields.begin(); mapIt != refFields.end(); ++mapIt) {	/* Parse the whole refFields map */
				const string& refColumnName = mapIt->first;
				const string& refRecordValue = mapIt->second;

				if (insertedValues.find(refColumnName) == insertedValues.end()) {	/* refColumnName was not found inside the insertedValue map */
					/* If this column was not part of the values to set, use the reference value (that we should have matched against) as the column+value pair to insert */
					/* This will make sure that we build a record as close as possible to the supposed result we would have after a successful modify (instead of insert) */
					insertedValues.emplace(std::make_pair(refColumnName, refRecordValue));
				}
			}

			vals.push_back(insertedValues);
#ifdef DEBUG
			cout << __func__ << "(): Inserting rather than modifying (no pre-existing record)\n";
#endif
			return this->insertCore(table, vals);	/* Insert rather than modifying */
		}
	}
	/* If we reach here, we will modify, not insert */
	try {
		stringstream sql_cmd(ios_base::in | ios_base::out | ios_base::ate);
		sql_cmd << "UPDATE \"" << this->escDQ(table) << "\" SET ";

		for (std::map<std::string, std::string>::const_iterator it = values.begin(); it != values.end(); ++it) {
			/* Check if iterator is on the first element of the list, and add a separator otherwise */
			if (it != values.begin()) {
				sql_cmd << ", ";
			}
			const string& fieldName = it->first;
			const string& newValue = it->second;
			sql_cmd << "\"" << this->escDQ(fieldName) << "\" = \"" << this->escDQ(newValue) << "\"";
		}
		sql_cmd << " " << sql_where.str();

#ifdef DEBUG
		cout << __func__ << "(): running SQL query \"" << sql_cmd.str() << "\"" << endl;
#endif
		return this->db->exec(sql_cmd.str()) > 0;
	}
	catch (const Exception &e) {
		cerr << "modifyCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::removeCore(const std::string& table,
                                 const std::map<std::string, std::string>& refFields) {

	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DELETE FROM \"" << this->escDQ(table) << "\"";
			if (!refFields.empty()) {
			ss << " WHERE ";

			for (map<string, string>::const_iterator it = refFields.begin(); it != refFields.end(); ++it) {
				/* Check if iterator is on the first element of the list, and add a separator otherwise */
				if (it != refFields.begin()) {
					ss << " AND ";
				}
				const string& fieldName = it->first;
				const string& value = it->second;
				ss << "\"" << this->escDQ(fieldName) << "\" = \"" << this->escDQ(value) << "\"";
			}
		}
		
#ifdef DEBUG
		cout << __func__ << "(): running SQL query \"" << ss.str() << "\"" << endl;
#endif
		int rowsDeleted = this->db->exec(ss.str());
		return (refFields.empty() || rowsDeleted>0);	/* If refFields is empty, we wanted to erase all, only in that case, even 0 rows affected would mean success */
	}
	catch (const Exception &e) {
		cerr << __func__ << "(): Exception while running query: " << e.what() << endl;
		return false;
	}
}

std::set<std::string> SQLiteDBManager::getFieldNames(const std::string& name,
                                                     const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getFieldNamesCore(name);
	}
	else {
		return this->getFieldNamesCore(name);
	}
}

std::set<std::string> SQLiteDBManager::getFieldNamesCore(const std::string& name) const {

	try {
		bool referenced = this->isReferencedCore(name);
#ifdef DEBUG
		cout << __func__ << "(): running SQL query \"PRAGMA table_info(\"" + this->escDQ(name) + "\")\"" << endl;
#endif
		Statement query(*(this->db), "PRAGMA table_info(\"" + this->escDQ(name) + "\")");
		set<string> fieldNamesSet;
		while(query.executeStep()) {
			string fieldName = query.getColumn(1).getText();
			if(!(referenced && (fieldName == PK_FIELD_NAME))) {
				fieldNamesSet.emplace(fieldName);
			}
		}

		return fieldNamesSet;
	}
	catch(const Exception &e) {
		cerr << "getFieldsNameCore: " << e.what() << endl;
		return set<string>();
	}
}

SQLTable SQLiteDBManager::getTableFromDatabaseCore(const std::string& table) const {

	SQLTable tableInDb(table);
	if(this->isReferencedCore(tableInDb.getName())) {
		tableInDb.markReferenced();
	}
	else {
		tableInDb.unmarkReferenced();
	}

	set<string> fieldNames = this->getFieldNamesCore(tableInDb.getName());
	map<string, string> defaultValues = this->getDefaultValuesCore(tableInDb.getName());
	map<string, bool> notNullFlags = this->getNotNullFlagsCore(tableInDb.getName());
	map<string, bool> uniqueness = this->getUniquenessCore(tableInDb.getName());

	for(auto &name : fieldNames) {
		tableInDb.addField(tuple<string,string,bool,bool>(name, defaultValues[name], notNullFlags[name], uniqueness[name]));
	}

	return tableInDb;
}

bool SQLiteDBManager::linkRecords(const std::string& table1,
                                  const std::map<std::string, std::string>& record1,
                                  const std::string& table2,
                                  const std::map<std::string, std::string>& record2,
                                  const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		bool result = this->linkRecordsCore(table1, record1, table2, record2);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->linkRecordsCore(table1, record1, table2, record2);
	}
}

bool SQLiteDBManager::linkRecordsCore(const std::string& table1,
                                      const std::map<std::string, std::string>& record1,
                                      const std::string& table2,
                                      const std::map<std::string, std::string>& record2) {

	//(1) We check if the records to link exists. If not we create them.
	bool result = true;
	bool record1Exist = false;
	for(auto &it : this->getCore(table1)) {
		it.erase(PK_FIELD_NAME);
		if(it == record1)
			record1Exist = true;
	}
	if(!record1Exist) {
		result = result && this->insertCore(table1, vector<map<string,string>>({record1}));
	}
	bool record2Exist = false;
	for(auto &it : this->getCore(table2)) {
		it.erase(PK_FIELD_NAME);
		if(it == record2)
			record2Exist = true;
	}
	if(!record2Exist) {
		result = result && this->insertCore(table2, vector<map<string,string>>({record2}));
	}

	if(!result)
		return result;
	//(2) We get their ids.
	set<string> record1Ids;
	for(auto &it : this->getCore(table1)) {
		string temp = it[PK_FIELD_NAME];
		it.erase(PK_FIELD_NAME);
		if(it == record1) {
			record1Ids.emplace(temp);
		}
	}
	set<string> record2Ids;
	for(auto &it : this->getCore(table2)) {
		string temp = it[PK_FIELD_NAME];
		it.erase(PK_FIELD_NAME);
		if(it == record2) {
			record2Ids.emplace(temp);
		}
	}

	if(!result)
			return result;
	//(3) We get the joining table name
	string joiningTable;
	for(auto &it : this->listTablesCore()) {
		string case1 = table1 + "_" + table2;
		string case2 = table2 + "_" + table1;
		if(it == case1)
			joiningTable = case1;
		else if(it == case2)
			joiningTable = case2;
	}

	result = result && (!joiningTable.empty());

	if(!result)
		return result;
	//(4) We check those records are not already linked
	string ref1FieldName = table1 + "#" + PK_FIELD_NAME;
	string ref2FieldName = table2 + "#" + PK_FIELD_NAME;
	map<map<string, string>, bool> linkingRecordLinked;
	bool allAlreadyLinked = true;
	for(auto &itRecord1Ids : record1Ids) {
		for(auto &itRecord2Ids : record2Ids) {
			map<string, string> linkingRecord;
			linkingRecord.emplace(ref1FieldName, itRecord1Ids);
			linkingRecord.emplace(ref2FieldName, itRecord2Ids);
			bool alreadyLinked = false;
			for(auto &it : this->getCore(joiningTable)) {
				if(it == linkingRecord)
					alreadyLinked = true;
			}
			allAlreadyLinked = allAlreadyLinked && alreadyLinked;
			linkingRecordLinked.emplace(linkingRecord, alreadyLinked);
		}
	}
	result = result && !allAlreadyLinked;
	if(!result)
		return result;
	//(5) We link those records.
	for(auto &it : linkingRecordLinked) {
		if(!it.second)
		result = result && this->insertCore(joiningTable, vector<map<string,string>>({it.first}));
	}

	return result;
}

bool SQLiteDBManager::applyPolicy(const std::string& relationshipName,
                                  const std::string& relationshipPolicy,
                                  const std::vector<std::string>& linkedTables,
                                  const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		bool result = this->applyPolicyCore(relationshipName, relationshipPolicy, linkedTables);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->applyPolicyCore(relationshipName, relationshipPolicy, linkedTables);
	}
}

bool SQLiteDBManager::applyPolicyCore(const std::string& relationshipName,
                                      const std::string& relationshipPolicy,
                                      const std::vector<std::string>& linkedTables) {

	bool result = true;
	if(this->getCore(relationshipName).empty() && linkedTables.size() == 2) {
		if(relationshipPolicy == "link-all") {
			vector<map<string, string>> recordsToInsert;
			for(auto &itRecordsTable1 : this->getCore(linkedTables.at(0))) {
				for(auto &itRecordsTable2 : this->getCore(linkedTables.at(1))) {
					map<string, string> record;
					record.emplace(linkedTables.at(0) + "#" + PK_FIELD_NAME, itRecordsTable1[PK_FIELD_NAME]);
					record.emplace(linkedTables.at(1) + "#" + PK_FIELD_NAME, itRecordsTable2[PK_FIELD_NAME]);
					recordsToInsert.push_back(record);
				}
			}
			result = result && this->insertCore(relationshipName, recordsToInsert);
		}
	}
	return result;
}

bool SQLiteDBManager::unlinkRecords(const std::string& table1,
                                    const std::map<std::string, std::string>& record1,
                                    const std::string& table2,
                                    const std::map<std::string, std::string>& record2,
                                    const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		bool result = this->unlinkRecordsCore(table1, record1, table2, record2);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->unlinkRecordsCore(table1, record1, table2, record2);
	}
}

bool SQLiteDBManager::unlinkRecordsCore(const std::string& table1,
                                        const std::map<std::string, std::string>& record1,
                                        const std::string& table2,
                                        const std::map<std::string, std::string>& record2) {

	//(1) We check if the records to link exists. If not we create them.
	bool result = true;
	bool record1Exist = false;
	for(auto &it : this->getCore(table1)) {
		it.erase(PK_FIELD_NAME);
		if(it == record1)
			record1Exist = true;
	}

	bool record2Exist = false;
	for(auto &it : this->getCore(table2)) {
		it.erase(PK_FIELD_NAME);
		if(it == record2)
			record2Exist = true;
	}

	result = result && record1Exist && record2Exist;

	if(!result)
		return result;
	//(2) We get their ids.
	set<string> record1Ids;
	for(auto &it : this->getCore(table1)) {
		string temp = it[PK_FIELD_NAME];
		it.erase(PK_FIELD_NAME);
		if(it == record1) {
			record1Ids.emplace(temp);
		}
	}
	set<string> record2Ids;
	for(auto &it : this->getCore(table2)) {
		string temp = it[PK_FIELD_NAME];
		it.erase(PK_FIELD_NAME);
		if(it == record2) {
			record2Ids.emplace(temp);
		}
	}

	if(!result)
			return result;
	//(3) We get the joining table name
	string joiningTable;
	for(auto &it : this->listTablesCore()) {
		string case1 = table1 + "_" + table2;
		string case2 = table2 + "_" + table1;
		if(it == case1)
			joiningTable = case1;
		else if(it == case2)
			joiningTable = case2;
	}

	result = result && (!joiningTable.empty());

	if(!result)
		return result;
	//(4) We check those records are not already linked
	string ref1FieldName = table1 + "#" + PK_FIELD_NAME;
	string ref2FieldName = table2 + "#" + PK_FIELD_NAME;
	vector<map<string, string>> recordsToDelete;
	for(auto &itRecord1Ids : record1Ids) {
		for(auto &itRecord2Ids : record2Ids) {
			map<string, string> linkingRecord;
			linkingRecord.emplace(ref1FieldName, itRecord1Ids);
			linkingRecord.emplace(ref2FieldName, itRecord2Ids);
			for(auto &it : this->getCore(joiningTable)) {
				if(it == linkingRecord)
					recordsToDelete.push_back(it);
			}
		}
	}
	result = result && !recordsToDelete.empty();
	if(!result)
		return result;
	//(5) We delete those records.
	for(auto &it : recordsToDelete) {
		result = result && this->removeCore(joiningTable, it);
	}

	return result;
}

std::map<std::string, std::vector<std::map<std::string,std::string> > > SQLiteDBManager::getLinkedRecords(const std::string& table,
                                                                                                          const std::map<std::string, std::string>& record,
                                                                                                          const bool& isAtomic) const {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		return this->getLinkedRecordsCore(table, record);
	}
	else {
		return this->getLinkedRecordsCore(table, record);
	}
}

std::map<std::string, std::vector<std::map<std::string,std::string> > > SQLiteDBManager::getLinkedRecordsCore(const std::string& table,
                                                                                                              const std::map<std::string, std::string>& record) const {

	// (1) We get all record ids for records whose values matches the record given in parameter.
	set<string> referenceRecordIds;
	for(auto &it : this->getCore(table)) {
		string id = it[PK_FIELD_NAME];
		it.erase(PK_FIELD_NAME);
		if(it == record) {
			referenceRecordIds.emplace(id);
		}
	}

	// (2) We find all the joining table names associated with this table and the relatedTables names
	set<string> linkingTables;
	map<string, string> relatedTables;
	for(auto &name : this->listTablesCore()) {
		if(name.find(table + "_") != string::npos || name.find("_" + table) != string::npos) {
			linkingTables.emplace(name);
			if(name.find(table + "_") != string::npos) {
				string otherTableName = name.substr(string(table+"_").length(), name.length()-string(table+"_").length());
				relatedTables.emplace(name, otherTableName);
			}
			else {
				string otherTableName = name.substr(0, name.length()-string("_"+table).length());
				relatedTables.emplace(name, otherTableName);
			}
		}
	}

	// (3) We fetch the related records
	map<string, vector<map<string,string>>> result;
	for(auto &linkingTable : linkingTables) {
		for(auto &it : this->getCore(linkingTable)) {
			if(referenceRecordIds.find(it[table + "#" + PK_FIELD_NAME]) != referenceRecordIds.end()) {
				string relatedId = it[relatedTables[linkingTable] + "#" + PK_FIELD_NAME];
				for(auto &relatedRecord : this->getCore(relatedTables[linkingTable])) {
					if(relatedRecord[PK_FIELD_NAME] == relatedId) {
						if(result.find(relatedTables[linkingTable]) != result.end()) {
							vector<map<string, string>> newVect;
							result.emplace(relatedTables[linkingTable], newVect);
						}
						result[relatedTables[linkingTable]].push_back(relatedRecord);
					}
				}
			}
		}
	}

	return result;
}

bool SQLiteDBManager::markReferenced(const std::string& name,
                                     const bool& isAtomic) {

	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		bool result = this->markReferencedCore(name);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->markReferencedCore(name);
	}
}

bool SQLiteDBManager::markReferencedCore(const std::string& name) {

	bool result = false;

	for(auto &it : this->listTablesCore()) {
		if(it == name) {
			result = result || true;
		}
	}

	if(!result)
		return result;

	result = !this->isReferencedCore(name);

	if(result) {
		// (1) Save table fields
		SQLTable table = this->getTableFromDatabaseCore(name);
		// (2) Save table content
		vector<map<string, string>> records = this->getCore(name);
		// (3) Drop the current table
		result = result && this->deleteTableCore(name);
		// (4) Mark the table referenced
		table.markReferenced();
		// (5) We recreate the table
		result = result && this->createTableCore(table);
		// (6) We populate the table
		result = result && this->insertCore(name, records);
	}

	return result;
}

bool SQLiteDBManager::unmarkReferenced(const std::string& name,
                                       const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*(this->db));
		bool result = this->unmarkReferencedCore(name);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->unmarkReferencedCore(name);
	}
}
bool SQLiteDBManager::unmarkReferencedCore(const std::string& name) {

	bool result = this->isReferencedCore(name);

	for(auto &it : this->listTablesCore()) {
			if(it == name) {
				result = result && true;
			}
		}

	if(result) {
		// (1) Save table fields
		SQLTable table = this->getTableFromDatabaseCore(name);
		// (2) Save table content
		vector<map<string, string>> records = this->getCore(name);
		// (3) Drop the current table
		result = result && this->deleteTableCore(name);
		// (4) Unmark the table referenced
		table.unmarkReferenced();
		// (5) We remove the primary key field values
		for(auto &it : records) {
			it.erase(PK_FIELD_NAME);
		}
		// (6) We recreate the table
		result = result && this->createTableCore(table);
		// (7) We populate the table
		result = result && this->insertCore(name, records);
	}

	return result;
}

void SQLiteDBManager::setDatabaseConfigurationFile(const std::string& databaseConfigurationFile) {
	this->configurationDescriptionFile = databaseConfigurationFile;
}
