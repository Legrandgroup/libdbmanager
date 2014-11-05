#include "dbmanager.hpp"

//libxml++ includes
//#include <libxml++/libxml++.h>
#include <tinyxml.h>

//SQLiteCpp includes
#include <sqlitecpp/SQLiteC++.h>

using namespace SQLite;
using namespace std;

DBManager* DBManager::instance = NULL;

/* ### Useful note ###
 *
 * Methods that affect the database are built this way :
 * Step  1: Cast the db attribute (void *) to its real target (SQLite::Database*) and assign it to a local variable db that will mask the object attribute in all subsequent calls
 * Step  2: Create a mutex on the database accesses
 * Step  3: Create a lock_guard instance that will manage the mutex created at step 2. Will lock the mutex on construction and unlock it on destruction
 * Step  4: Build the SQL Query thanks to stringstream (it is a STL stream that allows to easily put variables values in a string)
 * Step  5: Execute the built Query on the database
 * Step 6a: In case of success, return true for modifying operations or return values requested for reading operations
 * Step 6b: In case of failure, catch any exception, return false for modifying operations or return empty values for reading operations.
 */

DBManager::DBManager(string filename) : filename(filename), mut(), db(new Database(this->filename, SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE)) {
	
	cout << this->filename << endl;	//FIXME: for debug
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	db->exec("PRAGMA foreign_keys = ON");
}

DBManager::~DBManager() noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	if (db != NULL) {
		delete db;
		db = NULL;
		this->db = NULL;
	}
}


DBManager* DBManager::GetInstance() noexcept {
	//Singleton design pattern
	if(instance == NULL) {
		instance = new DBManager();
		instance->checkDefaultTables();
	}

	return instance;
}

void DBManager::FreeInstance() noexcept {
	if(instance != NULL) {
		delete instance;
		instance = NULL;
	}
}

//Get a table records, with possibility to specify some field value (name - value expected)
vector< map<string, string> > DBManager::get(const string& table, const vector<string >& columns, const bool& distinct) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
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
		cerr << e.what() << endl;
		return vector< map<string, string> >();
	}
}

//Insert a new record in the specified table
bool DBManager::insertRecord(const string& table, const map<string,string >& values) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "INSERT INTO \"" << table << "\" ";

		if(!values.empty()) {
			stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
			stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
			columnsName << "(";
			columnsValue << "(";
			for(const auto &it : values) {
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

		bool result= db->exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr  << e.what() << endl;
		return false;
	}
}

//Update a record in the specified table
bool DBManager::modifyRecord(const string& table, const map<string, string>& refFields, const map<string, string >& values, const bool& checkExistence) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	if(values.empty()) return false;
	if(checkExistence && this->get(table).empty()) { 	//It's okay to call get there, mutex isn't locked yet.
		return this->insertRecord(table, values);
	}
	//In case of check existence and empty table, won't reach there.
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);
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

		bool result= db->exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}

//Delete a record from the specified table
bool DBManager::deleteRecord(const string& table, const map<string, string>& refFields) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DELETE FROM \"" << table << "\"";
			if(!refFields.empty()) {
			ss << " WHERE ";

			for(const auto &it : refFields) {
				ss << "\"" << it.first << "\" = \"" << it.second << "\" AND ";
			}
			ss.str(ss.str().substr(0, ss.str().size()-5));
		}
	
		bool result= db->exec(ss.str()) > 0;
		if(result)
			transaction.commit();
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}

void DBManager::checkDefaultTables() {
	try {
		cout << "Launched check of XML atabase configuration file." << endl;
		//Loading of default table model thanks to XML definition file.
		TiXmlDocument doc("/tmp/conductor_db.conf");
		if(doc.LoadFile()||doc.LoadFile(/etc/conductor_db.conf)){// || doc.LoadFile("/etc/conductor_db.conf")) { // Will try to load file in tmp first then the one in etc
			cout << "XML OK LOADED" << endl;
			vector<SQLTable> tables;
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
							fieldElem = fieldElem->NextSiblingElement();
						}
						tables.push_back(table);
					}
					tableElem = tableElem->NextSiblingElement();
				}
			}

			cout << "XML OK FIRST PARSE" << endl;
			//Then we check relation in order to add foreign keys and create tables for m:n relationships.
			//*
			dbElem = doc.FirstChildElement();
			set<string> relationShipTables;	//Tables creation for relationship purpose.
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
									relationShipTables.emplace(this->createRelation(relationElem->Attribute("kind"), relationElem->Attribute("policy"), linkedtables));
								}
					}
					relationElem = relationElem->NextSiblingElement();
				}
			}//*/
			cout << "XML OK SECOND PARSE" << endl;

			//cout << endl << "Starting the checking." << endl;
			//Check if tables in db match models
			for(auto &table : tables) {
				this->checkTableInDatabaseMatchesModel(table);
			}

			cout << "XML OK CHECKED TABLES" << endl;
			//Remove tables that are present in db but not in model
			set<string> sqliteSpecificTables;	//Tables not to delete if they exist for internal sqlite behavior.
			sqliteSpecificTables.emplace("sqlite_sequence");

			vector<string> tablesInDbTmp = listTables();
			set<string> tablesInDb;
			for(auto &it :tablesInDbTmp) {
				tablesInDb.emplace(it);
			}

			for(auto &table : tables) {
				// << "Checking model table " << table.getName() << endl;
				if(tablesInDb.find(table.getName()) != tablesInDb.end()) {
					//cout << "Shouldn't be there" << endl;
					tablesInDb.erase(tablesInDb.find(table.getName()));
				}
			}
			for(auto &it : sqliteSpecificTables) {
				//cout << "Checking sqlite table " << it << endl;
				if(tablesInDb.find(it) != tablesInDb.end()) {
					//cout << "Shouldn't be there" << endl;
					tablesInDb.erase(tablesInDb.find(it));
				}
			}
			for(auto &it : relationShipTables) {
				//cout << "Checking relationship table " << it << endl;
				if(tablesInDb.find(it) != tablesInDb.end()) {
					//cout << "Shouldn't be there" << endl;
					tablesInDb.erase(tablesInDb.find(it));
				}
			}
			for(auto &it : tablesInDb) {
				//cout << "Deleting " << it << endl;
				this->deleteTable(it);
			}

			/*
			for(auto &it : listTables()) {
				bool deleteIt = true;
				for(auto &table : tables) {
					bool isntInModel = (it != table.getName());
					bool isntASQLiteInternTable = (sqliteSpecificTables.find(it) != sqliteSpecificTables.end());
					bool isntARelationShipTable = (relationShipTables.find(it) != relationShipTables.end());
					cout << table.getName() << ": " << endl;
					cout << "MOdel: " << isntInModel << endl;
					cout << "SQLIte Intern: " << isntASQLiteInternTable << endl;
					cout << "Rel Tab: " <<  isntARelationShipTable << endl;
					deleteIt = deleteIt && isntInModel && isntASQLiteInternTable;
				}
				if(deleteIt)
					this->deleteTable(it);
			}//*/
		}
		else {
			cout << "Throwing exception..." << endl;
			throw string("Unable to load any configuration file.");
		}
	}
	catch(const string &e) {
		cerr << "Exception caught while reading database description file :" << endl << e << endl << "Will terminate..." << endl;
		terminate();
	}
	catch(const exception &e) {
		cerr << "Exception caught while reading database description file :" << endl << e.what() << endl << "Will terminate..." << endl;
		terminate();
	}
}

void DBManager::checkTableInDatabaseMatchesModel(const SQLTable &model) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	cout << "Checking table " << model.getName() << endl;
	if(!db->tableExists(model.getName())) {
		this->createTable(model);
	}
	else {
		Statement query(*db, "PRAGMA table_info(\""+ model.getName()  + "\")");
		SQLTable tableInDb(model.getName());
		while(query.executeStep()) {
			string name = query.getColumn(1).getText();
			string defaultValue = query.getColumn(5).getText();
			bool isNotNull = (query.getColumn(3).getInt() == 1);
			map<string, bool> uniqueFields = this->getUniqueness(model.getName());
			bool isUnique = uniqueFields[name];
			tableInDb.addField(tuple<string,string,bool,bool>(name, defaultValue, isNotNull, isUnique));
		}

		if(this->isReferenced(model.getName())) {
			tableInDb.markReferenced();
		}
		else {
			tableInDb.unmarkReferenced();
		}

		if(model != tableInDb) {
				cout << "Model n db don't match." << endl;
				this->addFieldsToTable(tableInDb.getName(), model.diff(tableInDb));
				this->removeFieldsFromTable(tableInDb.getName(), tableInDb.diff(model));
		}
	}
}

bool DBManager::createTable(const SQLTable& table) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);
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

		//cout << ss.str() << endl;
		db->exec(ss.str());
		transaction.commit();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::addFieldsToTable(const string& table, const vector<tuple<string, string, bool, bool> >& fields) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);
		//Ate flag to move cursor at the end of the string when we do ss.str("blabla");
		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
		bool result = true;

		cout << "Having fields to add to table " << table << endl;

		if(!fields.empty()) {//*
			cout << "Fields to add not empty" << endl;
			//(1) We save field names, primary keys, default values and not null flags
			Statement query(*db, "PRAGMA table_info(\"" + table + "\")");
			vector<string> fieldNames;
			bool isReferenced = false;
			//set<string> primarykeys;
			map<string, string> defaultValues;
			map<string, bool> notNull;
			while(query.executeStep()) {
				cout << "At least tryied once" << endl;
				//+1 Because of behavior of the pragma.
				//The pk column is equal to 0 if the field isn't part of the primary key.
				//If the field is part of the primary key, it is equal to the index of the record +1
				//(+1 because for record of index 0, it would be marked as not part of the primary key without the +1).
				if(query.getColumn(5).getInt() == (query.getColumn(0).getInt()+1)) {
					isReferenced = true;
					//primarykeys.emplace(query.getColumn(1).getText());
				}
				else {
					cout << "Referenced check done" << endl;

					string dv = query.getColumn(4).getText();
					cout << "Default value Extracted from column" << endl;
					if(dv.find_first_of('"') != string::npos && dv.find_last_of('"') != string::npos) {
						cout << "Default value need to remove superfluous quotes" << endl;
						size_t start = dv.find_first_of('"')+1;
						size_t length = dv.find_last_of('"') - start;
						dv = dv.substr(start, length);
					}
					cout << "Default value will be memorized" << endl;
					defaultValues.emplace(query.getColumn(1).getText(), dv);
					cout << "Default value check done" << endl;
					notNull.emplace(query.getColumn(1).getText(), (query.getColumn(3).getInt() == 1));
					cout << "Not null check done" << endl;
					fieldNames.push_back(query.getColumn(1).getText());
					cout << "Added field " << query.getColumn(1).getText() << endl;
				}
			}

			cout << "Properties fetched" << endl;

			//(2) Then we save table's records
			vector<map<string, string>> records;
			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			ss << "SELECT ";

			vector<string > newColumns;
			ss << "*";
			//We fetch the names of table's columns in order to populate the map correctly
			//(With only * as columns name, we are notable to match field names to field values in order to build the map)
			Statement query2(*db, "PRAGMA table_info(\"" + table + "\");");
			while(query2.executeStep())
				newColumns.push_back(query2.getColumn(1).getText());

			ss << " FROM \"" << table << "\"";
			cout << ss.str() << endl;
			Statement query3(*db, ss.str());

			while(query3.executeStep()) {
				map<string, string> record;
				for(int i = 0; i < query3.getColumnCount(); ++i) {
					if(query3.getColumn(i).isNull()) {
						record.emplace(newColumns.at(i), "");
					}
					else {
						record.emplace(newColumns.at(i), query3.getColumn(i).getText());
					}
				}
				records.push_back(record);
			}

			cout << "Records fetched" << endl;

			//(4) We can add the new fields informations
			for(auto &it : fields) {
				string name = std::get<0>(it);
				string dv = std::get<1>(it);
				bool nn = std::get<2>(it);
				bool unique = std::get<3>(it);
				fieldNames.push_back(name);
				defaultValues.emplace(name, dv);
				notNull.emplace(name, nn);
				//TODO Add uniqueness constraint handling
			}

			if(!isReferenced) {
				cout << "##########" << endl << "Starting add Field to table "<< endl << "##########" << endl;


				//(3) Now that everything is saved, we can drop the "old" table
				ss.str("");
				ss << "DROP TABLE \"" << table << "\"";
				cout << ss.str() << endl;
				db->exec(ss.str());



				//(5) We can recreate the table
				ss.str("");
				ss << "CREATE TABLE \"" << table << "\" (";

				for(auto &it : fieldNames) {
					ss << "\"" << it << "\" TEXT ";
					if(notNull[it])
						ss << "NOT NULL ";
					ss << "DEFAULT \"" << defaultValues[it] << "\", ";
				}

				ss.str(ss.str().substr(0, ss.str().size()-2));
				ss << ")";

				cout << ss.str() << endl;
				db->exec(ss.str());

				//(6) The new table is created, populate with olds records (new fields will have default value)
				ss.str("");
				ss << "INSERT INTO \"" << table << "\" ";
				if(!records.empty()) {
					if(!records.at(0).empty()) {
						stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
						columnsName << "(";
						for(const auto &mapIt : records.at(0)) {
							columnsName << "\"" << mapIt.first << "\",";
						}
						columnsName.str(columnsName.str().substr(0, columnsName.str().size()-1));
						columnsName << ")";

						ss << columnsName.str() << " VALUES ";
					}
					else {
						ss << "DEFAULT VALUES";
					}
				}

				for(auto &vectIt : records) {
					if(!vectIt.empty()) {
						stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
						columnsValue << "(";
						for(const auto &mapIt : vectIt) {
							columnsValue << "\"" << mapIt.second << "\",";
						}
						columnsValue << "(";
						columnsValue.str(columnsValue.str().substr(0, columnsValue.str().size()-2));
						columnsValue << "),";

						ss <<  columnsValue.str();
					}
					else {
						ss << "DEFAULT VALUES";
					}

				}
				ss.str(ss.str().substr(0, ss.str().size()-1));

				ss << ";";
				cout << ss.str() << endl;
				result = result && db->exec(ss.str()) > 0;
	//*/
				/*
				for(const auto &it : fields) {
					ss.str("");
					ss << "ALTER TABLE \"" << table << "\" ADD \"" << std::get<0>(it) << "\" TEXT ";
					if(std::get<2>(it))
						ss << "NOT NULL ";
					ss << "DEFAULT \"" << std::get<1>(it) << "\"";
					db->exec(ss.str());
				}//*/
			}//*
			else {
				cout << "Table is referenced." << endl;
				//We'll search for the names
				Statement query(*db, "SELECT name FROM sqlite_master WHERE type='table'");
				set<string> linkingTables;
				while(query.executeStep()) {
					string name = query.getColumn(0).getText();
					if(name.find(table) != string::npos && name.find("_") != string::npos) {
						linkingTables.emplace(name);
						cout << "Emplaced " << name << endl;
					}
				}
				//No need to check field properties : linking tables fits a specific model : 2 integer column noted as primary keys and referencing the primary keys of 2 tables.

				cout << "Linking tables names found." << endl;

				//Now we have all the linker tables names, we can fetch their records.
				map<string, vector<map<string, string>>> recordsByTable;
				for(auto &it : linkingTables) {
					vector<map<string, string>> records;
					stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
					ss << "SELECT ";

					vector<string > newColumns;
					ss << "*";
					//We fetch the names of table's columns in order to populate the map correctly
					//(With only * as columns name, we are notable to match field names to field values in order to build the map)
					Statement query3(*db, "PRAGMA table_info(\"" + it + "\");");
					while(query3.executeStep())
						newColumns.push_back(query3.getColumn(1).getText());

					ss << " FROM \"" << it << "\"";
					cout << ss.str() << endl;
					Statement query4(*db, ss.str());

					while(query4.executeStep()) {
						map<string, string> record;
						for(int i = 0; i < query4.getColumnCount(); ++i) {
							if(query4.getColumn(i).isNull()) {
								record.emplace(newColumns.at(i), "");
							}
							else {
								record.emplace(newColumns.at(i), query4.getColumn(i).getText());
							}
						}
						records.push_back(record);
					}
					recordsByTable.emplace(it, records);
				}

				cout << "Linking tables records fetched." << endl;

				//Now the linking Tables are saved, we can drop them
				for(auto &it : linkingTables) {
					ss.str("");
					ss << "DROP TABLE \"" << it << "\"";
					cout << ss.str() << endl;
					db->exec(ss.str());
				}

				cout << "Linking tables dropped." << endl;

				//We can now drop our referenced table
				ss.str("");
				ss << "DROP TABLE \"" << table << "\"";
				cout << ss.str() << endl;
				db->exec(ss.str());

				cout << "Referenced table dropped." << endl;

				//We can recreate our referenced table
				ss.str("");
				ss << "CREATE TABLE \"" << table << "\" (";

				ss << "\"id\" INTEGER PRIMARY KEY AUTOINCREMENT, ";

				for(auto &it : fieldNames) {
					ss << "\"" << it << "\" TEXT ";
					if(notNull[it])
						ss << "NOT NULL ";
					ss << "DEFAULT \"" << defaultValues[it] << "\", ";
				}

				ss.str(ss.str().substr(0, ss.str().size()-2));
				ss << ")";

				cout << ss.str() << endl;
				db->exec(ss.str());

				cout << "Referenced table recreated." << endl;

				//We can populate it
				if(!records.empty()) {
					cout << "Record isn't empty." << endl;
					ss.str("");
					ss << "INSERT INTO \"" << table << "\" ";

					if(!records.at(0).empty()) {
						stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
						columnsName << "(";
						for(const auto &mapIt : records.at(0)) {
							columnsName << "\"" << mapIt.first << "\",";
						}
						columnsName.str(columnsName.str().substr(0, columnsName.str().size()-1));
						columnsName << ")";

						ss << columnsName.str() << " VALUES ";
					}
					else {
						ss << "DEFAULT VALUES";
					}


					for(auto &vectIt : records) {
						if(!vectIt.empty()) {
							stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
							columnsValue << "(";
							for(const auto &mapIt : vectIt) {
								columnsValue << "\"" << mapIt.second << "\",";
							}
							columnsValue << "(";
							columnsValue.str(columnsValue.str().substr(0, columnsValue.str().size()-2));
							columnsValue << "),";

							ss <<  columnsValue.str();
						}
						else {
							ss << "DEFAULT VALUES";
						}

					}
					ss.str(ss.str().substr(0, ss.str().size()-1));

					ss << ";";
					cout << ss.str() << endl;
					result = result && db->exec(ss.str()) > 0;
				}
				cout << "Referenced table populated." << endl;

				//Now that the table is recreated we can recreate the linking tables
				for(auto &it : linkingTables) {
					ss.str("");
					cout << "Will create linking table " << it << endl;
					ss << "CREATE TABLE \"" << it << "\" (";
					size_t pos = it.find_first_of("_");
					string table1 = it.substr(0, pos);
					string table2 = it.substr(pos+1, it.size()-(pos+1));
					ss << "\"" << table1 << "#" << PK_FIELD_NAME << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
					ss << "\"" << table2 << "#" << PK_FIELD_NAME << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
					ss << "PRIMARY KEY (\"" << table1 << "#" << PK_FIELD_NAME << "\", \"" << table2 << "#" << PK_FIELD_NAME << "\"))";

					cout << ss.str() << endl;
					db->exec(ss.str());
				}

				cout << "Linking tables recreated." << endl;

				//Now the linking tables are recreated we can populate them
				for(auto &it : linkingTables) {
					if(!recordsByTable[it].empty()) {
						ss.str("");
						ss << "INSERT INTO \"" << it << "\" ";

						if(!recordsByTable[it].at(0).empty()) {
							stringstream columnsName(ios_base::in | ios_base::out | ios_base::ate);
							columnsName << "(";
							for(const auto &mapIt : recordsByTable[it].at(0)) {
								columnsName << "\"" << mapIt.first << "\",";
							}
							columnsName.str(columnsName.str().substr(0, columnsName.str().size()-1));
							columnsName << ")";

							ss << columnsName.str() << " VALUES ";
						}
						else {
							ss << "DEFAULT VALUES";
						}

						for(auto &vectIt : recordsByTable[it]) {
							if(!vectIt.empty()) {
								stringstream columnsValue(ios_base::in | ios_base::out | ios_base::ate);
								columnsValue << "(";
								for(const auto &mapIt : vectIt) {
									columnsValue << "\"" << mapIt.second << "\",";
								}
								columnsValue << "(";
								columnsValue.str(columnsValue.str().substr(0, columnsValue.str().size()-2));
								columnsValue << "),";

								ss <<  columnsValue.str();
							}
							else {
								ss << "DEFAULT VALUES";
							}

						}
						ss.str(ss.str().substr(0, ss.str().size()-1));

						ss << ";";
						cout << ss.str() << endl;
						result = result && db->exec(ss.str()) > 0;
					}
					cout << "Linking tables populated." << endl;
				}
			}
		}

		if(result)
			transaction.commit();//*/
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::removeFieldsFromTable(const string & table, const vector<tuple<string, string, bool, bool> >& fields) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);
		bool result = true;
		/*if(!fields.empty()) {
			vector<string> remainingFields;

			Statement query(*db, "PRAGMA table_info(\""+ table  + "\")");
			SQLTable tableInDb(table);
			while(query.executeStep()) {
				string name = query.getColumn(1).getText();
				bool isRemaining = true;
				for(const auto &it : fields) {
					if(std::get<0>(it) == name)
						isRemaining = false;
				}
				if(isRemaining)
					remainingFields.push_back(name);
			}
	
			if(!remainingFields.empty()) {
				stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
				cout << "Trying to alter table " << table << endl;
				ss << "ALTER TABLE \"" << table << "\" RENAME TO \"" << table << "OLD\"";

				db->exec(ss.str());
				ss.str("");
				ss << "CREATE TABLE \"" << table << "\" AS SELECT ";
				for(auto &it : remainingFields) {
					ss << "\"" << it << "\",";
				}
				ss.str(ss.str().substr(0, ss.str().size()-1));

				ss << " FROM \"" << table << "OLD\"";

				cout << ss.str() << endl;
				db->exec(ss.str());
				ss.str("");
				ss << "DROP TABLE \"" << table << "OLD\"";
				cout << ss.str() << endl;
				db->exec(ss.str());
			}
		}//*/
	
		if(result) {
			transaction.commit();
		}
		return result;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return false;
	}
}


bool DBManager::deleteTable(const string& table) noexcept {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Transaction transaction(*db);

		stringstream ss(ios_base::in | ios_base::out | ios_base::ate);

		ss << "DROP TABLE \"" << table << "\"";

		db->exec(ss.str());
		transaction.commit();
		return true;
	}
	catch(const Exception & e) {
		cerr << e.what() << endl;
		return false;
	}
}

bool DBManager::createTable(const string& table, const map< string, string >& values) {
	SQLTable tab(table);
	for(const auto &it : values) {
		tab.addField(tuple<string, string, bool, bool>(it.first, it.second, true, false));
	}
	return this->createTable(tab);
}

vector< string > DBManager::listTables() {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Statement query(*db, "SELECT name FROM sqlite_master WHERE type='table'");
		vector<string> tablesInDb;
		while(query.executeStep()) {
			tablesInDb.push_back(query.getColumn(0).getText());
		}

		return tablesInDb;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return vector<string>();
	}
}

string DBManager::dumpTables() {
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

string DBManager::dumpTablesAsHtml() {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
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
			cout << "Text of pragma is " << query.getColumn(0).getText() << endl;
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


bool DBManager::isReferenced(string name) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	bool result = false;
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		while(query.executeStep()) {
			//cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
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
		cerr << e.what() << endl;
	}
	return result;
}

set<string> DBManager::getPrimaryKeys(string name) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	set<string> result;
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		while(query.executeStep()) {
			//cout << query.getColumn(0).getInt() << "|" << query.getColumn(1).getText() << "|" << query.getColumn(2).getText() << "|" << query.getColumn(3).getInt() << "|" << query.getColumn(4).getText() << "|" << query.getColumn(5).getInt() << endl;
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
		cerr << e.what() << endl;
	}
	return result;
}

map<string, string> DBManager::getDefaultValues(string name) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		map<string, string> defaultValues;
		while(query.executeStep()) {
			defaultValues.emplace(query.getColumn(1).getText(),query.getColumn(3).getText());
		}

		return defaultValues;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return map<string, string>();
	}
}

map<string, bool> DBManager::getNotNullFlags(string name) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		map<string, bool> notNullFlags;
		while(query.executeStep()) {
			notNullFlags.emplace(query.getColumn(1).getText(),(query.getColumn(4).getInt() == 1));
		}

		return notNullFlags;
	}
	catch(const Exception &e) {
		cerr << e.what() << endl;
		return map<string, bool>();
	}
}

map<string, bool> DBManager::getUniqueness(string name) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	std::lock_guard<std::mutex> lock(this->mut);	/* Lock the mutex (will be unlocked when object lock goes out of scope) */
	try {
		//(1) We get the field list
		Statement query(*db, "PRAGMA table_info(\"" + name + "\")");
		set<string> notUniqueFields;
		while(query.executeStep()) {
			notUniqueFields.emplace(query.getColumn(1).getText());
		}

		//(2) We obtain the unique indexes
		set<string> uniqueFields;
		Statement query2(*db, "PRAGMA index_list(\"" + name + "\")");
		while(query2.executeStep()) {
			if(query2.getColumn(2).getInt() == 1) {
				Statement query3(*db, "PRAGMA index_info(\"" + string(query2.getColumn(1).getText()) + "\")");
				while(query3.executeStep()) {
					uniqueFields.emplace(query3.getColumn(2).getText());
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
		cerr << e.what() << endl;
		return map<string, bool>();
	}
}

string DBManager::createRelation(const string &kind, const string &policy, const vector<string> &tables) {
	
	Database* db = reinterpret_cast<Database*>(this->db); /* Cast void *db to its hidden real type */
	
	if(kind == "m:n" && tables.size() == 2) {
		vector<string> tablesInDb = listTables();
		Transaction transaction(*db);
		string table1 = tables.at(0);
		string table2 = tables.at(1);
		string relationName = table1 + "_" + table2;
		bool addIt = true;
		for(auto &it : tablesInDb) {
			if(it == relationName) {
				addIt = false;
			}
		}
		if(addIt) {
			stringstream ss(ios_base::in | ios_base::out | ios_base::ate);
			ss << "CREATE TABLE \"" << relationName << "\" (";

			ss << "\"" << table1 << "#" << PK_FIELD_NAME << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
			ss << "\"" << table2 << "#" << PK_FIELD_NAME << "\" INTEGER REFERENCES \"" << table1 << "\"(\"" << PK_FIELD_NAME << "\"), ";
			ss << "PRIMARY KEY (\"" << table1 << "#" << PK_FIELD_NAME << "\", \"" << table2 << "#" << PK_FIELD_NAME << "\"))";

			cout << ss.str() << endl;
			db->exec(ss.str());

			transaction.commit();
		}
		return relationName;
	}
	else
		return string();
}
