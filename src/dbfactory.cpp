#include <fstream>

#include "dbfactory.hpp"
#include "sqlitedbmanager.hpp"

using namespace std;

#define SQLITE_URL_PREFIX "sqlite"

DBFactory& DBFactory::getInstance() {
	static DBFactory instance;
	return instance;
}

DBFactory::DBFactory() {
	this->allocatedManagers.clear();
}

DBFactory::~DBFactory() {
	for(auto &it : this->allocatedManagers) {
		if(getDatabaseKind(it.first) == SQLITE_URL_PREFIX) {
			SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(it.second);
			if(db != NULL) {
				delete db;
				db = NULL;
			}
		}
	}
	this->allocatedManagers.clear();
}

DBManager& DBFactory::getDBManager(string location, string configurationDescriptionFile) {
	DBManager *manager = NULL;
	if(this->allocatedManagers.find(location) != this->allocatedManagers.end()) {
		manager = this->allocatedManagers[location];
	}
	if(manager == NULL) {
		string databaseKind = getDatabaseKind(location);
		if(databaseKind == SQLITE_URL_PREFIX) {
			string databaseLocation = getUrl(location);
			manager = new SQLiteDBManager(databaseLocation, configurationDescriptionFile);
			this->allocatedManagers.emplace(location, manager);
		}
		else {
			throw invalid_argument("Unrecognized database kind. Currently supported databases : sqlite");
		}
	}
	markRequest(location); //If we reach there, it is either the manager pointer is already allocated or we successfully allocated it.
	return *manager;
}

void DBFactory::freeDBManager(string location) {
	unmarkRequest(location); //If it isn't allocated, it will does nothing
	if(!isUsed(location)) {
		map<string, DBManager*>::iterator it = this->allocatedManagers.find(location);
		if(it != this->allocatedManagers.end()) {
			if(getDatabaseKind(location) == SQLITE_URL_PREFIX) {
				SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(it->second);
				if(db != NULL) {
					delete db;
					db = NULL;
					this->allocatedManagers.erase(it);
				}
			}
		}
	}
}

void DBFactory::markRequest(string location) {
	if(this->requests.find(location) != this->requests.end()) {
		this->requests[location] = this->requests[location]+1;
	}
	else {
		this->requests.emplace(location, 1);
	}
}

void DBFactory::unmarkRequest(string location) {
	if(this->requests.find(location) != this->requests.end()) {
		if(this->requests[location] > 0) {
			this->requests[location] = this->requests[location]-1;
		}
	}
}

bool DBFactory::isUsed(string location) {
	return (this->requests.find(location) != this->requests.end() && (this->requests[location] > 0));
}

string DBFactory::getDatabaseKind(string location) {
	string databaseKind;
	string separator = "://";
	if(location.find(separator) != string::npos) {
		databaseKind = location.substr(0, location.find(separator));
	}
	return databaseKind;
}

string DBFactory::getUrl(string location) {
	string databaseLocation;
	string separator = "://";
	if(location.find(separator) != string::npos) {
		unsigned int startPos = location.find(separator)+separator.length();
		databaseLocation = "/" + location.substr(startPos, location.length()-startPos);
	}
	return databaseLocation;
}