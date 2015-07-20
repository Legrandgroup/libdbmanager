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
DBManagerAllocationSlot::DBManagerAllocationSlot(DBManager* managerPtr, const bool& exclusive) :
		managerPtr(managerPtr), servedReferences(0), exclusive(exclusive)
#ifdef __unix__
		, lockFilename(""), lockFd(NULL)
#endif
		{
}

DBManagerAllocationSlot::DBManagerAllocationSlot(const DBManagerAllocationSlot& other) :
		managerPtr(other.managerPtr),
		servedReferences(other.servedReferences),
		exclusive(other.exclusive)
#ifdef __unix__
		,lockFilename(other.lockFilename),
		lockFd(other.lockFd)
#endif
		{
	/* Note: we do not prevent copy construction even if exclusive is set, because we want to allow copy construction or assignment
	 * exlusivity only means not serving the DBManager reference to the outside of the library, but we can have (temporarily) more than one slot with exclusivity set
	 */
}

/**
 * This method is a friend of DBManagerAllocationSlot class
 * swap() is needed within operator=() to implement to copy and swap paradigm
**/
void swap(DBManagerAllocationSlot& first, DBManagerAllocationSlot& second) noexcept {
	using std::swap;	// Enable ADL

	swap(first.managerPtr, second.managerPtr);
	swap(first.servedReferences, second.servedReferences);
	swap(first.exclusive, second.exclusive);
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
	this->freeAllDBManagers(true);
}

DBManager& DBManagerFactory::getDBManager(const string& location, const string& configurationDescriptionFile, const bool& exclusive) {
	DBManager *manager = NULL;

	try {
		DBManagerAllocationSlot& slot= this->managersStore.at(location);        /* Try to find a reference to the a slot */
		/* If now exception is raised, it means that there already a manager with this location in the store */
#ifdef DEBUG
		cout << string(__func__) + "() called for an existing location \"" + location + "\"\n";
#endif
		manager = slot.managerPtr;
		if (manager == NULL) {	/* A slot exists but is not valid... raise an exception */
			throw runtime_error("Invalid slot (DBManager==NULL) for location " + location);
		}
		/* A DBManager already exists for this location */
		if (slot.servedReferences > 0) {
				if (exclusive || slot.exclusive) {	/* If we ask exclusivity or the existing manager is set to be exclusive, and refCount is at least one, we will fail to guarantee exclusivity */
					throw runtime_error("Failed to ensure requested exclusivity for location " + location); /* Exclusivity for this DBManager cannot be met... raise an exception (that won't be catched by parent try/catch block */
				}
		}
		else if (slot.servedReferences == 0) {	/* This slot is not used anymore... we will adjust exclusivity to the new request */
			slot.exclusive = exclusive;
		}
	}
	catch (const std::out_of_range& ex) {	/* If getting out of range, it means this location does not exist in the store. Create the slot and DBManager */
		string databaseType = this->locationUrlToProto(location);
		if(databaseType == SQLITE_URL_PROTO) {	/* Handle sqlite:// URLs */
			string databasePath = this->locationUrlToPath(location);
			manager = new SQLiteDBManager(databasePath, configurationDescriptionFile);	/* Allocate a new manager */
			DBManagerAllocationSlot newSlot(manager, exclusive);	/* Store the pointer to this new manager in a new slot */
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

void DBManagerFactory::freeDBManager(const string& location) {
	this->decRefCount(location); /* If no manager is known for this location, this call will do nothing */
#ifdef DEBUG
	cout << string(__func__) + "(): reference count for location \"" + location + "\" has been decremented to " + to_string(this->getRefCount(location)) + "\n";
#endif
	if (!this->isUsed(location)) {	/* Instance in this slot is not referenced anymore, destroy the slot */
		try {
			DBManagerAllocationSlot& slot = this->managersStore.at(location);	/* Get a reference to the slot corresponding to this manager URL */
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

void DBManagerFactory::freeAllDBManagers(const bool& ignoreRefCount) {
	for (auto &it : this->managersStore) {
		if (!ignoreRefCount && this->isUsed(it.first)) {
			throw runtime_error("Refusing to free the DBManager for a slot that is still referenced");
		}
		if (this->locationUrlToProto(it.first) == SQLITE_URL_PROTO) {
			DBManagerAllocationSlot& slot = it.second;	/* Get the allocation slot for this manager URL */
			// Lionel: FIXME: Casting here could be avoided, if we used a virtual destructor in base class DBManager and all its derived classes
			SQLiteDBManager *db = dynamic_cast<SQLiteDBManager*>(slot.managerPtr);
			if (db != NULL) {
				delete db;
				db = NULL;
			}
		}
		//this->freeDBManager(it.first);
	}
	this->managersStore.clear();
}

void DBManagerFactory::incRefCount(const string& location) {
	try {
		DBManagerAllocationSlot& slot= this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		slot.servedReferences++;	/* And increase the reference count */
	}
	catch (const std::out_of_range& ex) {
		return;	/* If getting out of range, it means this location does not exist in the store */
	}
}

void DBManagerFactory::decRefCount(const string& location) {
	try {
		DBManagerAllocationSlot& slot = this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		if (slot.servedReferences > 0) {
			slot.servedReferences--;	/* Decrease the reference count if positive */
			if (slot.servedReferences == 0)
				slot.exclusive = false;	/* Reset exclusivity if no reference exists anymore on this slot */
		}
	}
	catch (const std::out_of_range& ex) {
		return;	/* If getting out of range, it means this location does not exist in the store */
	}
}

unsigned int DBManagerFactory::getRefCount(const string& location) const {
	try {
		const DBManagerAllocationSlot &slot = this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		return slot.servedReferences;	/* ... and return the reference count */
	}
	catch (const std::out_of_range& ex) {
		cerr << string(__func__) + "(): Error: location \"" + location + "\" does not exist\n";
		throw out_of_range("Unknown location url");
	}
}

bool DBManagerFactory::isUsed(const string& location) const {
	try {
		return (this->getRefCount(location) > 0);	/* If the reference count for this location is at least 1 => return true */
	}
	catch (const std::out_of_range& ex) {
		return false;	/* If getting out of range, it means this location does not exist in the store */
	}
}

bool DBManagerFactory::isExclusive(const string& location) const {
	try {
		const DBManagerAllocationSlot& slot = this->managersStore.at(location);	/* Get a reference to the corresponding slot */
		return slot.exclusive;
	}
	catch (const std::out_of_range& ex) {
		cerr << string(__func__) + "(): Error: location \"" + location + "\" does not exist\n";
		throw out_of_range("Unknown location url");
	}
}

string DBManagerFactory::locationUrlToProto(const string& location) const {
	string databaseProto;
	static const string separator = "://";
	if(location.find(separator) != string::npos) {
		databaseProto = location.substr(0, location.find(separator));
	}
	return databaseProto;
}

string DBManagerFactory::locationUrlToPath(const string& location) const {
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
