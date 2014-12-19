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
	this->managersStore.clear();
}

DBFactory::~DBFactory() {
	for(auto &it : this->managersStore) {
		if(this->getDatabaseKind(it.first) == SQLITE_URL_PREFIX) {
			DBManagerAllocationSlot &slot = it.second;	/* Get the allocation slot for this manager URI */
			SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(slot.managerPtr);
			if(db != NULL) {
				delete db;
				db = NULL;
			}
		}
	}
	this->managersStore.clear();
}

DBManager& DBFactory::getDBManager(string location, string configurationDescriptionFile) {
	DBManager *manager = NULL;

	try {
		DBManagerAllocationSlot &slot= this->managersStore.at(location);        /* Try to find a reference to the a slot */
		/* If now exception is raised, it means that there already a manager with this location in the store */
#ifdef DEBUG
		cout << "DBManager for location \"" + location + "\" already exists\n";
#endif
		manager = slot.managerPtr;
	}
	catch (const std::out_of_range& ex) {
		manager = NULL; /* If getting out of range, it means this location does not exist in the store */
	}

	if (manager == NULL) {	/* No DBManager exists yet for this location... create one */
		string databaseKind = this->getDatabaseKind(location);
		if(databaseKind == SQLITE_URL_PREFIX) {	/* Handle sqlite:// URIs */
			string databaseLocation = this->getUrl(location);
			manager = new SQLiteDBManager(databaseLocation, configurationDescriptionFile);	/* Allocate a new manager */
			DBManagerAllocationSlot newSlot(manager);	/* Store the pointer to this new manager in a new slot */
#ifdef __unix__
			/* Create a lock file for this location */
			string prefix = LOCK_FILE_PREFIX;
			string lockBasename = databaseLocation;
			while(lockBasename.find("/") != string::npos) {	/* Replace / by _ in database location filename */
				lockBasename.replace(lockBasename.find_first_of("/"), 1, "_");
			}
			newSlot.lockFilename = prefix + lockBasename + ".lock";

			FILE *fd = fopen(newSlot.lockFilename.c_str(), "w");
			if (fd == NULL) {
				throw runtime_error("Could not create lock file \"" + newSlot.lockFilename + "\"");
			}
			if ( flock(fileno(fd), LOCK_EX | LOCK_NB) == -1 ) {	/* Try to lock using flock */
				fclose(fd);
				throw runtime_error("Could not flock() on \"" + newSlot.lockFilename + "\"");
			}
			else {
				newSlot.lockFd = fd;	/* Store the handle for this lock */
			}
			//~ cout << "Grabbed lock on file " + lockFilename + " for location " + location << endl;
#endif

			this->managersStore.emplace(location, newSlot);	/* Add this new slot to the store */
		}
		else {
			throw invalid_argument("Unrecognized database kind. Currently supported databases : sqlite");
		}
	}
	this->markRequest(location); /* If we reach there, either the manager pointer already existed or we have just successfully allocated it. In all cases, increment the reference count */
	return *manager;
}

void DBFactory::freeDBManager(string location) {
	this->unmarkRequest(location); /* If no manager is known for this location, this call will do nothing */
	try {
		DBManagerAllocationSlot &slot = this->managersStore.at(location);	/* Get a reference to the slot corresponding to this manager URI */
#ifdef __unix__
		/* Remove the lock file handle associated with this DBManager from the allocatedDbLockFd map */
#ifdef DEBUG
		cout << "Releasing lockfile \"" + slot.lockFilename + "\" for location \"" + location + "\"" << endl;
#endif
		FILE *fd = slot.lockFd;	/* Get the file descriptor for this lock */

		if (fd != NULL) {
			flock(fileno(fd), LOCK_UN);
			fclose(fd);
			slot.lockFd = NULL;
		}
		if (slot.lockFilename != "") {
			remove(slot.lockFilename.c_str());	/* Delete the file in the fs */
			slot.lockFilename = "";
		}
#endif
		/* Now remove the DBManager pointer from the slot */
		if(this->getDatabaseKind(location) == SQLITE_URL_PREFIX) {	/* Handle sqlite:// URIs */
			SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(slot.managerPtr);
			if(db != NULL) {
				delete db;
				slot.managerPtr = NULL;
			}
		}
		else {
			cerr << string(__func__) + "(): Error: unknown URI type " + this->getDatabaseKind(location) + ". Pointed DBManager object will not be deallocated properly, a memory leak will occur" << endl;
		}
		this->managersStore.erase(location);
	}
	catch (const std::out_of_range& ex) {
		return; /* If getting out of range, it means this location does not exist in the store... do nothing */
	}
}

void DBFactory::markRequest(string location) {
	try {
		DBManagerAllocationSlot &slot= this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		slot.servedReferences++;	/* And increase the reference count */
	}
	catch (const std::out_of_range& ex) {
		return;	/* If getting out of range, it means this location does not exist in the store */
	}
}

void DBFactory::unmarkRequest(string location) {
	try {
		DBManagerAllocationSlot &slot = this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		if(slot.servedReferences > 0) {
			slot.servedReferences--;	/* Decrease the reference count if positive */
		}
	}
	catch (const std::out_of_range& ex) {
		return;	/* If getting out of range, it means this location does not exist in the store */
	}
}

bool DBFactory::isUsed(string location) {
	try {
		DBManagerAllocationSlot &slot = this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		return (slot.servedReferences > 0);	/* ...if this slot is referenced at least once => return true */
	}
	catch (const std::out_of_range& ex) {
		return false;	/* If getting out of range, it means this location does not exist in the store */
	}
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
