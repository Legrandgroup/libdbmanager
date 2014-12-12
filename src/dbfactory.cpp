#include <fstream>

#ifdef __unix__
extern "C" {
	#include <sys/file.h>
}
#endif

#include "dbfactory.hpp"
#include "sqlitedbmanager.hpp"

#ifdef __unix__
#define LOCK_FILE_PREFIX "/tmp/dbmanager"
#endif

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
		if(databaseKind == SQLITE_URL_PREFIX) {	/* Handle sqlite:// URIs */
			string databaseLocation = getUrl(location);
			//Instanciation of the manager
			manager = new SQLiteDBManager(databaseLocation, configurationDescriptionFile);
			this->allocatedManagers.emplace(location, manager);
			//We create the lock file for this manager
#ifdef __unix__
			string prefix = LOCK_FILE_PREFIX;
			string lockFilename = databaseLocation;
			while(lockFilename.find("/") != string::npos) {	/* Replace / by _ in database location filename */
				lockFilename.replace(lockFilename.find_first_of("/"), 1, "_");
			}
			lockFilename = prefix + lockFilename + ".lock";
			FILE *fd = fopen(lockFilename.c_str(), "w");
			if (fd == NULL) {
				throw runtime_error("Could not create lock file \"" + lockFilename + "\"");
			}
			if ( flock(fileno(fd), LOCK_EX | LOCK_NB) == -1 ) {	/* Try to lock using flock */
				fclose(fd);
				throw runtime_error("Could not flock() on \"" + lockFilename + "\"");
			}
			else {
				this->allocatedDbLockFd.emplace(location, fd);	/* Store the handle for this lock */
			}
			//~ cout << "Grabbed lock on file " + lockFilename + " for location " + location << endl;
#endif
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
		/* Remove the DBManager* from the allocatedManagers map */
		map<string, DBManager*>::iterator it1 = this->allocatedManagers.find(location);
		if(it1 != this->allocatedManagers.end()) {
			if(getDatabaseKind(location) == SQLITE_URL_PREFIX) {	/* Handle sqlite:// URIs */
				SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(it1->second);
				if(db != NULL) {
					delete db;
					db = NULL;
					it1->second = NULL;
					this->allocatedManagers.erase(it1);
				}
			}
		}
#ifdef __unix__
		/* Remove the lock file handle associated with this DBManager from the allocatedDbLockFd map */
		map<string, FILE*>::iterator it2 = this->allocatedDbLockFd.find(location);
		if(it2 != this->allocatedDbLockFd.end()) {
			FILE *fd = it2->second;	/* Get the file descriptor for this lock */
			
			if (fd != NULL) {
				flock(fileno(fd), LOCK_UN);
				fclose(fd);
				fd = NULL;
				it2->second = NULL;
				this->allocatedDbLockFd.erase(it2);
				string prefix = LOCK_FILE_PREFIX;	/* FIXME: Lionel: duplicated code with DBFactory::getDBManager, store this into a state object */
				string lockFilename = getUrl(location);;
				while(lockFilename.find("/") != string::npos) {	/* Replace / by _ in database location filename */
					lockFilename.replace(lockFilename.find_first_of("/"), 1, "_");
				}
				lockFilename = prefix + lockFilename + ".lock";
				remove(lockFilename.c_str());
				cout << "Freed lock for " + location + " on file " + lockFilename << endl;
			}
		}
#endif
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
