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

#define SQLITE_URL_PROTO "sqlite"

/* Note: a copy operator is acceptable for this class, because even if we copy a pointer, we don't allocate it in this class, nor do we free it, we only store it */
/* Allocation/deallocation is done outside by the code that uses us to store the result */
/* This copy constructor will allow us to automatically get an assignment operator using to copy and swap paradigm (see below) */
DBManagerAllocationSlot::DBManagerAllocationSlot(DBManager *managerPtr) :
		managerPtr(managerPtr), servedReferences(0)
#ifdef __unix__
		, lockFilename(""), lockFd(NULL)
#endif
		{
}

DBManagerAllocationSlot::DBManagerAllocationSlot(const DBManagerAllocationSlot& other) :
		managerPtr(other.managerPtr),
		servedReferences(other.servedReferences)
#ifdef __unix__
		,lockFilename(other.lockFilename),
		lockFd(other.lockFd)
#endif
		{
}

/**
 * This method is a friend of DBManagerAllocationSlot class
 * swap() is needed within operator=() to implement to copy and swap paradigm
**/
void swap(DBManagerAllocationSlot& first, DBManagerAllocationSlot& second) noexcept {
	using std::swap;	// Enable ADL

	swap(first.managerPtr, second.managerPtr);
	swap(first.servedReferences, second.servedReferences);
#ifdef __unix__
	swap(first.lockFilename, second.lockFilename);
	swap(first.lockFd, second.lockFd);
#endif
	/* Once we have swapped the members of the two instances... the two instances have actually been swapped */
}

DBManagerAllocationSlot& DBManagerAllocationSlot::operator=(DBManagerAllocationSlot other) {
	swap(*this, other);	/* Become other, which is the copy of the right side operand */
	return *this;	/* After return, our previous instance (transferred to other) will be destructed. We have transferred other's personality to us */
}

DBManagerFactory& DBManagerFactory::getInstance() {
	static DBManagerFactory instance;
	return instance;
}

DBManagerFactory::DBManagerFactory() : managersStore() {
}

DBManagerFactory::~DBManagerFactory() {
	for(auto &it : this->managersStore) {
		if(this->locationUrlToProto(it.first) == SQLITE_URL_PROTO) {
			DBManagerAllocationSlot& slot = it.second;	/* Get the allocation slot for this manager URL */
			// Lionel: FIXME: Casting here could be avoided, if we used a virtual destructor in base class DBManager and all its derived classes
			SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(slot.managerPtr);
			if(db != NULL) {
				delete db;
				db = NULL;
			}
		}
	}
	this->managersStore.clear();
}

DBManager& DBManagerFactory::getDBManager(string location, string configurationDescriptionFile) {
	DBManager *manager = NULL;

	try {
		DBManagerAllocationSlot &slot= this->managersStore.at(location);        /* Try to find a reference to the a slot */
		/* If now exception is raised, it means that there already a manager with this location in the store */
#ifdef DEBUG
		cout << string(__func__) + "() called for an existing location \"" + location + "\"\n";
#endif
		manager = slot.managerPtr;
	}
	catch (const std::out_of_range& ex) {
		manager = NULL; /* If getting out of range, it means this location does not exist in the store */
	}

	if (manager == NULL) {	/* No DBManager exists yet for this location... create one */
		string databaseType = this->locationUrlToProto(location);
		if(databaseType == SQLITE_URL_PROTO) {	/* Handle sqlite:// URLs */
			string databasePath = this->locationUrlToPath(location);
			manager = new SQLiteDBManager(databasePath, configurationDescriptionFile);	/* Allocate a new manager */
			DBManagerAllocationSlot newSlot(manager);	/* Store the pointer to this new manager in a new slot */
#ifdef DEBUG
		cout << string(__func__) + "() just created a new instance for a new location \"" + location + "\"\n";
#endif
#ifdef __unix__
			/* Create a lock file for this location */
			string prefix = LOCK_FILE_PREFIX;
			string lockBasename = databasePath;
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
			throw invalid_argument("Unrecognized database type: \"" + databaseType + "\". Supported type: sqlite");
		}
	}
	this->incRefCount(location); /* If we reach there, either the manager pointer already existed or we have just successfully allocated it. In all cases, increment the reference count */
#ifdef DEBUG
	cout << string(__func__) + "(): reference count for location \"" + location + "\" is now " + to_string(this->getRefCount(location)) + "\n";
#endif
	return *manager;
}

void DBManagerFactory::freeDBManager(string location) {
	this->decRefCount(location); /* If no manager is known for this location, this call will do nothing */
#ifdef DEBUG
	cout << string(__func__) + "(): reference count for location \"" + location + "\" has been decremented to " + to_string(this->getRefCount(location)) + "\n";
#endif
	if (!this->isUsed(location)) {	/* Instance in this slot is not referenced anymore, destroy the slot */
		try {
			DBManagerAllocationSlot &slot = this->managersStore.at(location);	/* Get a reference to the slot corresponding to this manager URL */
#ifdef __unix__
			/* Remove the lock file handle associated with this DBManager from the allocatedDbLockFd map */
#ifdef DEBUG
			cout << "Releasing lockfile \"" + slot.lockFilename + "\" for location \"" + location + "\"\n";
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
			if(this->locationUrlToProto(location) == SQLITE_URL_PROTO) {	/* Handle sqlite:// URLs */
				// Lionel: FIXME: Casting here could be avoided, if we used a virtual destructor in base class DBManager and all its derived classes
				SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(slot.managerPtr);
				if(db != NULL) {
					delete db;
					slot.managerPtr = NULL;
				}
			}
			else {
				cerr << string(__func__) + "(): Error: unknown database type \"" + this->locationUrlToProto(location) + "\". Pointed DBManager object will not be deallocated properly, a memory leak will occur\n";
			}
			this->managersStore.erase(location);
		}
		catch (const std::out_of_range& ex) {
			return; /* If getting out of range, it means this location does not exist in the store... do nothing */
		}
	}
}

void DBManagerFactory::incRefCount(string location) {
	try {
		DBManagerAllocationSlot &slot= this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		slot.servedReferences++;	/* And increase the reference count */
	}
	catch (const std::out_of_range& ex) {
		return;	/* If getting out of range, it means this location does not exist in the store */
	}
}

void DBManagerFactory::decRefCount(string location) {
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

unsigned int DBManagerFactory::getRefCount(string location) const {
	try {
		const DBManagerAllocationSlot &slot = this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		return slot.servedReferences;	/* ... and return the reference count */
	}
	catch (const std::out_of_range& ex) {
		cerr << string(__func__) + "(): Error: location \"" + location + "\" does not exist\n";
		throw out_of_range("Unknown location url");
	}
}

bool DBManagerFactory::isUsed(string location) {
	try {
		return (this->getRefCount(location) > 0);	/* If the reference count for this location is at least 1 => return true */
	}
	catch (const std::out_of_range& ex) {
		return false;	/* If getting out of range, it means this location does not exist in the store */
	}
}

string DBManagerFactory::locationUrlToProto(string location) {
	string databaseProto;
	static const string separator = "://";
	if(location.find(separator) != string::npos) {
		databaseProto = location.substr(0, location.find(separator));
	}
	return databaseProto;
}

string DBManagerFactory::locationUrlToPath(string location) {
	string databasePath;
	static const string separator = "://";
	if(location.find(separator) != string::npos) {
		unsigned int startPos = location.find(separator)+separator.length();
		databasePath = location.substr(startPos, location.length()-startPos);
		if ((databasePath.length() >= 1) &&
			(databasePath[0] != '/')) {	/* If path is not absolute... */
			databasePath.insert(0, 1, '/');	/* Make it absolute relative to the root by prepending a '/' */
		}
	}
	return databasePath;
}
