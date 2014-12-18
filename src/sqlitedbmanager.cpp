#include "sqlitedbmanager.hpp"
#include <fstream>

using namespace SQLite;
using namespace std;

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

SQLiteDBManager::SQLiteDBManager(const string& filename, const string& configurationDescriptionFile) : filename(filename), configurationDescriptionFile(configurationDescriptionFile), mut(), db(new Database(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE)) {
	db->exec("PRAGMA foreign_keys = ON"); //Activation of foreign key support in SQLite database
	this->checkDefaultTables();			  //Will proceed migration if some changes are detected between configuration file and database state.
}

SQLiteDBManager::~SQLiteDBManager() noexcept {
	if (this->db != NULL) {
		delete this->db;
		this->db = NULL;
	}
}

void SQLiteDBManager::checkDefaultTables(const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*db);
		if(this->checkDefaultTablesCore())
			transaction.commit();
	}
	else {
		this->checkDefaultTablesCore();
	}
}

bool SQLiteDBManager::checkDefaultTablesCore() {
	try {
		bool result = false;
		TiXmlDocument doc;
		/* Load the default table model base on the provided input XML definition
		 * This XML can be provided inside a file (this->configurationDescriptionFile then contains the PATH to this file)
		 * or it can be provided directly as a string buffer (this->configurationDescriptionFile then stores the actual XML content)
		 * We first check if this->configurationDescriptionFile is an existing file, then we try to parse it as XML
		 */
		//Try to load from a file. If configurationDescriptionFile attribute doesn't match a file path, it assumes it is the content of the file instead. If parsing fails, exception is thrown.
		if(((ifstream(this->configurationDescriptionFile, ios::out)) && doc.LoadFile(this->configurationDescriptionFile)) || (doc.Parse(this->configurationDescriptionFile.data()) == NULL)){
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

void SQLiteDBManager::checkTableInDatabaseMatchesModel(const SQLTable &model, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*db);
		if(this->checkTableInDatabaseMatchesModelCore(model))
			transaction.commit();
	}
	else {
		this->checkTableInDatabaseMatchesModelCore(model);
	}
}

bool SQLiteDBManager::checkTableInDatabaseMatchesModelCore(const SQLTable &model) noexcept {
	bool result = true;
	//Create table if it doesn't exist in the database.
	if(!db->tableExists(model.getName())) {
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

bool SQLiteDBManager::createTable(const SQLTable& table, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*db);

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
		ss << "CREATE TABLE \"" << table.getName() << "\" (";
		vector< tuple<string,string,bool,bool> > fields = table.getFields();

		if(table.isReferenced()) {
			ss << "\"id\" INTEGER PRIMARY KEY AUTOINCREMENT, ";
		}

		for(auto &it : fields) {
			//0 -> field name
			//1 -> field default value
			//2 -> field is not null
			//3 -> field is unique
			ss << "\"" << std::get<0>(it) << "\" TEXT ";
			if(std::get<2>(it))
				ss << "NOT NULL ";
			if(std::get<3>(it))
				ss << "UNIQUE ";
			ss << "DEFAULT \"" << std::get<1>(it) << "\", ";
		}


		ss.str(ss.str().substr(0, ss.str().size()-2));
		ss << ")";

		db->exec(ss.str());
		return true;
	}
	catch(const Exception & e) {
		cerr << "createTableCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::addFieldsToTable(const string& table, const vector<tuple<string, string, bool, bool> >& fields, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);

		bool result = this->addFieldsToTableCore(table, fields);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->addFieldsToTableCore(table, fields);
	}
}

bool SQLiteDBManager::addFieldsToTableCore(const string& table, const vector<tuple<string, string, bool, bool> >& fields) noexcept {
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

bool SQLiteDBManager::removeFieldsFromTable(const string & table, const vector<tuple<string, string, bool, bool> >& fields, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
		bool result = this->removeFieldsFromTableCore(table, fields);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->removeFieldsFromTableCore(table, fields);
	}
}

bool SQLiteDBManager::removeFieldsFromTableCore(const string & table, const vector<tuple<string, string, bool, bool> >& fields) noexcept {
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


bool SQLiteDBManager::deleteTable(const string& table, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		Transaction transaction(*db);

		bool result = this->deleteTableCore(table);
		if (result)
			transaction.commit();
		return result;
	}
	else {
		return this->deleteTableCore(table);
	}
}

bool SQLiteDBManager::deleteTableCore(const string& table) noexcept {
	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DROP TABLE \"" << table << "\"";

		db->exec(ss.str());
		return true;
	}
	catch(const Exception & e) {
		cerr << "deleteTableCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::createTable(const string& table, const map< string, string >& values, const bool& isAtomic) {
	SQLTable tab(table);
	for(const auto &it : values) {
		tab.addField(tuple<string, string, bool, bool>(it.first, it.second, true, false));
	}
	return this->createTable(tab, isAtomic);
}

vector< string > SQLiteDBManager::listTables(const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return listTablesCore();
	}
	else {
		return listTablesCore();
	}
}

vector< string > SQLiteDBManager::listTablesCore() {
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

string SQLiteDBManager::dumpTables() {
	stringstream dump;
	vector< string > tables = this->listTables();

	for(vector<string>::iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		//Récupération des valeurs
		vector<map<string, string> > records = this->get(*tableName);

		if(!records.empty()) {
			//Calcul du plus long mot
			map<string, unsigned int> longests;
			for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					//Init of values
					if(longests.find(mapIt->first) == longests.end()) {
						longests.emplace(mapIt->first, 0);
					}

					//First is column name.
					if(mapIt->first.size() > longests[mapIt->first]) {
						longests[mapIt->first] = mapIt->first.size();
					}
					if(mapIt->second.size() > longests[mapIt->first]) {
						longests[mapIt->first] = mapIt->second.size();
					}
				}
			}

			//Première ligne : nom des colonnes
			stringstream Hsep;
			stringstream headers;
			Hsep << "+-";
			headers << "| ";
			for(map<string, string>::iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); ++mapIt) {
				//First is column name.
				for(unsigned int i = 0; i < longests[mapIt->first]; ++i) {
					Hsep << "-";
				}
				headers << mapIt->first;

				for(unsigned int i = 0; i < (longests[mapIt->first]-mapIt->first.size()); ++i) {
					headers << " ";
				}

				//Check if iterator is the last one with data
				if(next(mapIt) != records.at(0).end()) {
					Hsep << "-+-";
					headers << " | ";
				}
			}
			Hsep << "-+";
			headers << " |";

			stringstream values;
			for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				values << "| ";
				for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					values << mapIt->second;

					for(unsigned int i = 0; i < (longests[mapIt->first]-mapIt->second.size()); ++i) {
						values << " ";
					}

					//Check if iterator is the last one with data
					if(next(mapIt) != vectIt->end()) {
						values << " | ";
					}
				}
				values << " |";

				//Check if iterator is the last one with data
				if(next(vectIt) != records.end()) {
					values << endl;
				}
			}

			stringstream tableDump;
			tableDump << "Table: " << *tableName << endl;
			tableDump << Hsep.str() << endl;
			tableDump << headers.str() << endl;
			tableDump << Hsep.str() << endl;
			tableDump << values.str() << endl;
			tableDump << Hsep.str() << endl;

			dump << tableDump.str();
			//Check if iterator is the last one with data
			if(next(tableName) != tables.end()) {
				dump << endl;
			}
		}
		else {
			dump << "Table " << *tableName << " is empty." << endl;
		}
	}

	return dump.str();
}

string SQLiteDBManager::dumpTablesAsHtml() {
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

	bool foreignKeysEnabled = false;
	Statement query(*db, "PRAGMA foreign_keys");
	while(query.executeStep()) {
		if(string(query.getColumn(0).getText()) == "1") {
			foreignKeysEnabled = true;
		}
		else {
			//cout << "Text of pragma is " << query.getColumn(0).getText() << endl;
		}
	}
	if(foreignKeysEnabled) {
		htmlDump << "<p> Foreign Keys are enabled </p>";
	}
	else {
		htmlDump << "<p> Foreign Keys are disabled </p>";
	}

	vector< string > tables = this->listTables();

	for(vector<string>::iterator tableName = tables.begin(); tableName != tables.end(); ++tableName) {
		htmlDump << "<h3> Table : " << *tableName << "</h3>";
		//Récupération des valeurs
		vector<map<string, string> > records = this->get(*tableName);

		if(!records.empty()) {
			htmlDump << "<table class=\"table table-striped table-bordered table-hover\">";
			htmlDump << "<thead>";
			htmlDump << "<tr>";
			//Première ligne : nom des colonnes
			for(map<string, string>::iterator mapIt = records.at(0).begin(); mapIt != records.at(0).end(); ++mapIt) {
				//First is column name.
				htmlDump << "<th>" << mapIt->first;
				set<string> primarykeys = this->getPrimaryKeys(*tableName);
				if(primarykeys.find(mapIt->first) != primarykeys.end()) {
					htmlDump << " [PK] ";
				}

				map<string, bool> uniqueness = this->getUniqueness(*tableName);
				if(uniqueness[mapIt->first]) {
					htmlDump << " [U] ";
				}
				htmlDump << "</th>";
			}
			htmlDump << "</tr>";
			htmlDump << "</thead>";

			htmlDump << "<tbody>";
			for(vector<map<string, string> >::iterator vectIt = records.begin(); vectIt != records.end(); ++vectIt) {
				htmlDump << "<tr>";
				for(map<string, string>::iterator mapIt = vectIt->begin(); mapIt != vectIt->end(); ++mapIt) {
					htmlDump << "<td>" << mapIt->second << "</td>";
				}
				htmlDump << "</tr>";
			}
			htmlDump << "</tbody>";
			htmlDump << "</table>";
		}
		else {

			vector<string> fields;
			Statement query(*db, "PRAGMA table_info(\"" + *tableName + "\")");
			while(query.executeStep()) {
				fields.push_back(query.getColumn(1).getText());
			}

			htmlDump << "<table class=\"table table-striped table-bordered table-hover\">";
			htmlDump << "<thead>";
			htmlDump << "<tr>";
			//Première ligne : nom des colonnes
			for(auto &it : fields) {
				htmlDump << "<th>" << it;
				set<string> primarykeys = this->getPrimaryKeys(*tableName);
				if(primarykeys.find(it) != primarykeys.end()) {
					htmlDump << " [PK] ";
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

bool SQLiteDBManager::isReferenced(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->isReferencedCore(name);
	}
	else {
		return this->isReferencedCore(name);
	}
}

bool SQLiteDBManager::isReferencedCore(const string& name) {
	bool result = false;
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
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
		cerr << "IsReferencedCore: " << e.what() << endl;
	}
	return result;
}

set<string> SQLiteDBManager::getPrimaryKeys(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getPrimaryKeysCore(name);
	}
	else {
		return this->getPrimaryKeysCore(name);
	}
}

set<string> SQLiteDBManager::getPrimaryKeysCore(const string& name) {
	set<string> result;
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		while(query.executeStep()) {
			////cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
			//+1 Because of behavior of the pragma.
			//The pk column is equal to 0 if the field isn't part of the primary key.
			//If the field is part of the primary key, it is equal to the index of the record +1
			//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
			if(query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
				result.emplace(query.getColumn(1).getText());
			}
		}
	}
	catch(const Exception &e) {
		cerr << "getPrimaryKeysCore: " << e.what() << endl;
	}
	return result;
}

map<string, string> SQLiteDBManager::getDefaultValues(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getDefaultValuesCore(name);
	}
	else {
		return this->getDefaultValuesCore(name);
	}
}

map<string, string> SQLiteDBManager::getDefaultValuesCore(const string& name) {
	try {
		bool referenced = this->isReferencedCore(name);
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
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

map<string, bool> SQLiteDBManager::getNotNullFlags(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getNotNullFlagsCore(name);
	}
	else {
		return this->getNotNullFlagsCore(name);
	}
}

map<string, bool> SQLiteDBManager::getNotNullFlagsCore(const string& name) {
	try {
		bool referenced = this->isReferencedCore(name);
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
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

map<string, bool> SQLiteDBManager::getUniqueness(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getUniquenessCore(name);
	}
	else {
		return this->getUniquenessCore(name);
	}
}

map<string, bool> SQLiteDBManager::getUniquenessCore(const string& name) {
	try {
		bool referenced = this->isReferencedCore(name);
		//(1) We get the field list
		set<string> notUniqueFields = this->getFieldNamesCore(name);

		//(2) We obtain the unique indexes
		set<string> uniqueFields;
		Statement query2(*db, "PRAGMA index_list(\"" + name + "\")");
		while(query2.executeStep()) {
			string fieldName = query2.getColumn(1).getText();
			if(!(referenced && (fieldName == PK_FIELD_NAME))) {
				if(query2.getColumn(2).getInt() == 1) {
					Statement query3(*db, "PRAGMA index_info(\"" + fieldName + "\")");
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

string SQLiteDBManager::createRelation(const string &kind, const vector<string> &tables, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
		string result = this->createRelationCore(kind, tables);
		if(!result.empty())
			transaction.commit();
		return result;
	}
	else {
		return this->createRelationCore(kind, tables);
	}
}

string SQLiteDBManager::createRelationCore(const string &kind, const vector<string> &tables) {
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
			ss << "CREATE TABLE \"" << relationName << "\" (";

			ss << "\"" << fieldName1.str()  << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
			ss << "\"" << fieldName2.str() << "\" INTEGER REFERENCES \"" << table2 << "\"(\"" << PK_FIELD_NAME << "\"), ";
			ss << "PRIMARY KEY (\"" << table1 << "#" << PK_FIELD_NAME << "\", \"" << table2 << "#" << PK_FIELD_NAME << "\"))";

			//We use the referenced table primary keys as foreign keys (see m:n relationship theory if it bugs you).
			db->exec(ss.str());
		}

		return relationName;
	}
	else {
		return string();
	}
}

std::vector< std::map<string, string> > SQLiteDBManager::get(const std::string& table, const std::vector<std::string >& columns, const bool& distinct, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getCore(table, columns, distinct);
	}
	else {
		return this->getCore(table, columns, distinct);
	}
}

bool SQLiteDBManager::insert(const std::string& table, const std::vector<std::map<std::string , std::string>>& values, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);

		bool result = this->insertCore(table, values);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->insertCore(table, values);
	}
}

bool SQLiteDBManager::modify(const std::string& table, const std::map<std::string, std::string>& refFields, const std::map<std::string, std::string >& values, const bool& checkExistence, const bool& isAtomic) noexcept {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);

		bool result = this->modifyCore(table, refFields, values, checkExistence);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->modifyCore(table, refFields, values, checkExistence);
	}
}

bool SQLiteDBManager::remove(const std::string& table, const std::map<std::string, std::string>& refFields, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);

		bool result = this->removeCore(table, refFields);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->removeCore(table, refFields);
	}
}

vector< std::map<string, string> > SQLiteDBManager::getCore(const string& table, const vector<std::string >& columns, const bool& distinct) noexcept {
	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "SELECT ";

		if(distinct)
			ss << "DISTINCT ";

		vector<string > newColumns;
		if(columns.empty()) {
			ss << "*";
			//We fetch the names of table's columns in order to populate the map correctly
			//(With only * as columns name, we are notable to match field names to field values in order to build the map)
			Statement query(*db, "PRAGMA table_info(\"" + table + "\");");
			while(query.executeStep())
				newColumns.push_back(query.getColumn(1).getText());
		}
		else if(columns.size() == 1) {
			if(columns.at(0) == "*") {
				ss << "*";
				//We fetch the names of table's columns in order to populate the map correctly
				//(With only * as columns name, we are notable to match field names to field values in order to build the map)
				Statement query(*db, "PRAGMA table_info(\"" + table + "\");");
				while(query.executeStep())
					newColumns.push_back(query.getColumn(1).getText());
			}
			else {
				ss << "\"" << columns.at(0) << "\"";
				newColumns.push_back(columns.at(0));
			}
		}
		else {
			for(const auto &it : columns) {
				ss << "\"" << it << "\"";
				ss << ", ";
				newColumns.push_back(it);
			}
			ss.str(ss.str().substr(0, ss.str().size()-2)); //Solution that remove last ", "

		}

		ss << " FROM \"" << table << "\"";

		Statement query(*db, ss.str());

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

bool SQLiteDBManager::insertCore(const string& table, const vector<map<std::string , string>>& values) {
	try {
		bool result = true;
		for(auto &itVect : values) {
			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			//Only one request is used. We could have use one request for each record, but it would have been slower.
			ss << "INSERT INTO \"" << table << "\" ";
			if(!itVect.empty()) {
				stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
				stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
				columnsName << "(";
				columnsValue << "(";
				for(const auto &it : itVect) {
					columnsName << "\"" << it.first << "\",";
					columnsValue << "\"" << it.second << "\",";
				}
				columnsName.str(columnsName.str().substr(0, columnsName.str().size()-1));
				columnsValue.str(columnsValue.str().substr(0, columnsValue.str().size()-1));
				columnsName << ")";
				columnsValue << ")";

				ss << columnsName.str() << " VALUES " << columnsValue.str() << ";";
			}
			else {
				ss << "DEFAULT VALUES";
			}

			result = result && db->exec(ss.str()) > 0;
		}

		return result;
	}
	catch(const Exception &e) {
		cerr  << "insertCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::modifyCore(const string& table, const map<string, string>& refFields, const map<string, string >& values, const bool& checkExistence) noexcept {
	if(values.empty()) return false;
	if(checkExistence && this->getCore(table).empty()) { 	//It's okay to call get there, mutex isn't locked yet.
		vector<map<string,string>> vals;
		vals.push_back(values);
		return this->insertCore(table, vals);
	}
	//In case of check existence and empty table, won't reach there.
	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		ss << "UPDATE \"" << table << "\" SET ";

		for(const auto &it : values) {
			ss << "\"" << it.first << "\" = \"" << it.second << "\", ";
		}
		ss.str(ss.str().substr(0, ss.str().size()-2));
		ss << " ";

		if(!refFields.empty()) {
			ss << "WHERE ";
			for(const auto &it : refFields) {
				ss << "\"" << it.first << "\" = \"" << it.second << "\" AND ";
			}
			ss.str(ss.str().substr(0, ss.str().size()-5));
		}

		return db->exec(ss.str()) > 0;
	}
	catch(const Exception &e) {
		cerr << "modifyCore: " << e.what() << endl;
		return false;
	}
}

bool SQLiteDBManager::removeCore(const string& table, const map<std::string, string>& refFields) {
	try {
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DELETE FROM \"" << table << "\"";
			if(!refFields.empty()) {
			ss << " WHERE ";

			for(const auto &it : refFields) {
				ss << "\"" << it.first << "\" = \"" << it.second << "\" AND ";
			}
			ss.str(ss.str().substr(0, ss.str().size()-5));
		}

		return db->exec(ss.str()) > 0;
	}
	catch(const Exception &e) {
		cerr << "deleteCore: " <<e.what() << endl;
		return false;
	}
}

set<string> SQLiteDBManager::getFieldNames(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		return this->getFieldNamesCore(name);
	}
	else {
		return this->getFieldNamesCore(name);
	}
}

set<string> SQLiteDBManager::getFieldNamesCore(const string& name) {
	try {
		bool referenced = this->isReferencedCore(name);
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		set<string> fieldNames;
		while(query.executeStep()) {
			string fieldName = query.getColumn(1).getText();
			if(!(referenced && (fieldName == PK_FIELD_NAME))) {
				fieldNames.emplace(fieldName);
			}
		}

		return fieldNames;
	}
	catch(const Exception &e) {
		cerr << "getFieldsNameCore: " << e.what() << endl;
		return set<string>();
	}
}

SQLTable SQLiteDBManager::getTableFromDatabaseCore(const string& table) {
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

bool SQLiteDBManager::linkRecords(const string& table1, const map<string, string>& record1, const string& table2, const map<string, string>& record2, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
		bool result = this->linkRecordsCore(table1, record1, table2, record2);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->linkRecordsCore(table1, record1, table2, record2);
	}
}

bool SQLiteDBManager::linkRecordsCore(const string& table1, const map<string, string>& record1, const string& table2, const map<string, string>& record2) {
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

bool SQLiteDBManager::applyPolicy(const string& relationshipName, const string& relationshipPolicy, const vector<string>& linkedTables, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
		bool result = this->applyPolicyCore(relationshipName, relationshipPolicy, linkedTables);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->applyPolicyCore(relationshipName, relationshipPolicy, linkedTables);
	}
}

bool SQLiteDBManager::applyPolicyCore(const string& relationshipName, const string& relationshipPolicy, const vector<string>& linkedTables) {
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

bool SQLiteDBManager::unlinkRecords(const string& table1, const map<string, string>& record1, const string& table2, const map<string, string>& record2, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
		bool result = this->unlinkRecordsCore(table1, record1, table2, record2);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->unlinkRecordsCore(table1, record1, table2, record2);
	}
}

bool SQLiteDBManager::unlinkRecordsCore(const string& table1, const map<string, string>& record1, const string& table2, const map<string, string>& record2) {
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

map<string, vector<map<string,string>>> SQLiteDBManager::getLinkedRecords(const string& table, const map<string, string>& record, const bool & isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */

		return this->getLinkedRecordsCore(table, record);
	}
	else {
		return this->getLinkedRecordsCore(table, record);
	}
}

map<string, vector<map<string,string>>> SQLiteDBManager::getLinkedRecordsCore(const string& table, const map<std::string, std::string>& record) {
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

bool SQLiteDBManager::markReferenced(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
		bool result = this->markReferencedCore(name);
		if(result)
			transaction.commit();
		return result;
	}
	else {
		return this->markReferencedCore(name);
	}
}

bool SQLiteDBManager::markReferencedCore(const string& name) {
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

bool SQLiteDBManager::unmarkReferenced(const string& name, const bool& isAtomic) {
	if(isAtomic) {
		std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
		Transaction transaction(*db);
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

void SQLiteDBManager::setDatabaseConfigurationFile(const string& databaseConfigurationFile) {
	this->configurationDescriptionFile = databaseConfigurationFile;
}
