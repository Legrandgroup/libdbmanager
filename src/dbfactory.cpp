#include <fstream>

#include "dbfactory.hpp"
#include "sqlitedbmanager.hpp"

using namespace std;

DBFactory::DBFactory() {
	this->allocatedManagers.clear();
	this->databasePorts.clear();
}

DBFactory::~DBFactory() {
	for(auto &it : this->allocatedManagers) {
		delete it.second;
		it.second = NULL;
	}
	this->allocatedManagers.clear();
	this->databasePorts.clear();
}

DBManager* DBFactory::getDBManager(std::string location, std::string port) {
	DBManager *manager = NULL;
	if(this->allocatedManagers.find(location) != this->allocatedManagers.end()) {
		manager = this->allocatedManagers[location];
	}
	if(manager == NULL) {
		string separator = "://";
		if(location.find(separator) != string::npos) {
			string databaseKind = location.substr(0, location.find(separator));
			if(databaseKind == "sqlite") {
				unsigned int startPos = location.find(separator)+separator.length();
				string databaseLocation = "/" + location.substr(startPos, location.length()-startPos);
				if(!ifstream(databaseLocation, ios::in)) { //Try to open the file to see if it exists.
					throw invalid_argument("Database file not found.");
				}
				manager = new SQLiteDBManager(databaseLocation);
				this->allocatedManagers.emplace(location, manager);
			}
			else {
				throw invalid_argument("Unrecognized database kind. Currently supported databases : sqlite");
			}
		}
	}

	return manager;
}

void DBFactory::freeDBManager(std::string location, std::string port) {
	map<string, DBManager*>::iterator it = this->allocatedManagers.find(location);
	if(it != this->allocatedManagers.end()) {
		delete it->second;
		it->second = NULL;
		this->allocatedManagers.erase(it);
	}
}
