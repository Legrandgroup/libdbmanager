#ifndef _DBFACTORY_HPP_
#define _DBFACTORY_HPP_

//STL includes
#include <string>
#include <map>
#include <exception>

//Library includes
#include "dbmanager.hpp"

/**
 * \class DBManagerAllocationSlot
 *
 * \brief Structure holding the details of the allocation slot matching with one URL
 */
class DBManagerAllocationSlot {
public:
	DBManager*    managerPtr;	/*!< A pointer to the manager object corresponding to this allocated slot */
	unsigned int  servedReferences;	/*< The number of references that have been given to this allocated slot */
#ifdef __unix__
	std::string   lockFilename;	/*< A filename used as lock for this slot */
	FILE*         lockFd;	/*< A file descriptor on which flock() has been called on the file lockFilename */
#endif
	/**
	 * \brief Class constructor
	 *
	 * \param managerPtr A pointer to the DBManager instance to store in this slot
	 */
	DBManagerAllocationSlot(DBManager *managerPtr) :
		managerPtr(managerPtr), servedReferences(0)
#ifdef __unix__
		, lockFilename(""), lockFd(NULL)
#endif
			{ }
};

/**
 * \class DBFactory
 *
 * \brief Class to obtain database managers.
 *
 * This class has methods to obtains database manager instances based on specific URL.
 * For example the URL "sqlite://[sqlite database file full path]" will give you a database manager that handle SQLite databases.
 * This class implements the singleton design pattern (in the lazy way).
 *
 */
class DBFactory {
public:
	/**
	 * \brief Instance getter
	 *
	 * This method allows to obtain the unique instance of the DBFactory class.
	 *
	 * \return DBFactory& The reference to the unique instance of the DBFactory class.
	 */
	static DBFactory& getInstance();

	/**
	 * \brief DBManager getter
	 *
	 * This method allows to obtain an instance of the DBManager class that is configured to manage the database located at the location parameter.
	 *
	 * \param location The location, in a URL address, of the database to manage.
	 * \param configurationDescriptionFile The path to the configuration file to use for this database, or the configuration content directly provided as a std::string (no carriage return allowed in this case)
	 * \return DBManager& The reference to an instance of the DBManager class.
	 */
	DBManager& getDBManager(std::string location, std::string configurationDescriptionFile="");
	/**
	 * \brief DBManager releaser
	 *
	 * This method allows to notify the factory that the served instance of a DBManager is not used anymore.
	 *
	 * \param location The location, in a URL address, of the database concerned by the notification.
	 */
	void freeDBManager(std::string location);

private:
	DBFactory();
	~DBFactory();

	void incRefCount(std::string location);
	void decRefCount(std::string location);
	bool isUsed(std::string location);

	std::string locationUrlToProto(std::string location);
	std::string locationUrlToPath(std::string location);

	std::map<std::string, DBManagerAllocationSlot> managersStore;	/*!< A map (containing elements called "slots" in this code) storing all allocated instances of DBManager objects */
	/* Note: when accessing an element of this map, use the std::map::at() method, because DBManagerAllocation's constructor requires one argument and std::map::operator[] needs to be able to insert an element using a constructor without argument */

};

#endif //_DBFACTORY_HPP_
