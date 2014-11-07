#include <fstream>

#include "dbfactory.hpp"
#include "sqlitedbmanager.hpp"

using namespace std;

DBFactory& DBFactory::getInstance() {
	static DBFactory instance;
	return instance;
}

DBFactory::DBFactory() {
	this->allocatedManagers.clear();
}

DBFactory::~DBFactory() {
	for(auto &it : this->allocatedManagers) {
		delete it.second;
		it.second = NULL;
	}
	this->allocatedManagers.clear();
}

DBManager& DBFactory::getDBManager(string location, string configurationDescriptionFile) {
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
				/*if(!ifstream(databaseLocation, ios::in)) { //Try to open the file to see if it exists.
					throw invalid_argument("Database file not found.");
				}//*/
				cout << "FACTORY: Generated manager with parameters (" << databaseLocation <<", " << configurationDescriptionFile << ")" << endl;
				manager = new SQLiteDBManager(databaseLocation, configurationDescriptionFile);
				this->allocatedManagers.emplace(location, manager);
			}
			else {
				throw invalid_argument("Unrecognized database kind. Currently supported databases : sqlite");
			}
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
			delete it->second;
			it->second = NULL;
			this->allocatedManagers.erase(it);
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
	cout << "MARK: Current value for " << location << ": " << this->requests[location] << "(" << this->requests.size() << ")" << endl;
}

void DBFactory::unmarkRequest(string location) {
	if(this->requests.find(location) != this->requests.end()) {
		if(this->requests[location] > 0) {
			this->requests[location] = this->requests[location]-1;
		}
	}
	cout << "UNMARK: Current value for " << location << ": " << this->requests[location] << endl;
}

bool DBFactory::isUsed(string location) {
	return (this->requests.find(location) != this->requests.end() && (this->requests[location] > 0));
}
