/**
 *
 * \file dbfactory.hpp
 *
 * \brief Factory to generate, retrieve and free instances of DBManager
 */

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
	unsigned int  servedReferences;	/*!< The number of references that have been given to this allocated slot */
#ifdef __unix__
	std::string   lockFilename;	/*!< A filename used as lock for this slot */
	FILE*         lockFd;	/*!< A file descriptor on which flock() has been called on the file lockFilename */
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

	/**
	 * \brief Increment reference count for a specific location
	 *
	 * This method will increment the reference count for a given location, meaning we are publishing (once more) a reference on a DBManager object
	 *
	 * \param location The URL for which we decrement the reference count
	 */
	void incRefCount(std::string location);

	/**
	 * \brief Decrement reference count for a specific location
	 *
	 * This method will decrement the reference count for a given location, meaning we assume a reference on a DBManager object given by getDBManager() will not be used anymore
	 *
	 * \param location The URL for which we decrement the reference count
	 */
	void decRefCount(std::string location);

	/**
	 * \brief Get the reference count for a specific location
	 *
	 * This method will return the reference count for a given location
	 * Warning, it may raise an exception if the location provided is unknown
	 *
	 * \param location The URL for which we want the reference count
	 * \return The reference count
	 */
	unsigned int getRefCount(std::string location) const;

	/**
	 * \brief Check if a specific location is still used (from its reference count)
	 *
	 * This method will check if the reference count for a given location is at least 1
	 *
	 * \param location The URL for which we decrement the reference count
	 * \return true if the reference count is 1 or more
	 */
	bool isUsed(std::string location);

	/**
	 * \brief Extract the protocol part of a location URL
	 *
	 * locationUrlToPath("sqlite:///tmp") => "sqlite"
	 *
	 * \param location The URL string
	 * \return The protocol part as a string
	 */
	std::string locationUrlToProto(std::string location);
	/**
	 * \brief Extract the path part of a location URL
	 *
	 * locationUrlToPath("sqlite:///tmp") => "/tmp"
	 *
	 * \param location The URL string
	 * \return The path part as a string
	 */
	std::string locationUrlToPath(std::string location);

	std::map<std::string, DBManagerAllocationSlot> managersStore;	/*!< A map (containing elements called "slots" in this code) storing all allocated instances of DBManager objects */
	/* Note: when accessing an element of this map, use the std::map::at() method, because DBManagerAllocation's constructor requires one argument and std::map::operator[] needs to be able to insert an element using a constructor without argument */

};

/**
 * \class DBFactoryContainer
 *
 * \brief Class to encapsulate a DBManager object (generated by class DBFactory)
 * It will ensure the object is created (if not already existing), and the reference count is kepts up to date during the life cycle of DBFactoryContainer
 * (DBFactory::getInstance().freeDBManager() will be called when this DBFactoryContainer is destructed)
 */
class DBFactoryContainer
{
public:
	/**
	 * \brief Class constructor
	 *
	 * Arguments are the same as DBFactory::getDBManager()
	 *
	 * \param dbLocation The location, in a URL address, of the database to manage.
	 * \param configurationDescriptionFile The path to the configuration file to use for this database, or the configuration content directly provided as a std::string (no carriage return allowed in this case)
	 */
	DBFactoryContainer(std::string dbLocation, std::string configurationDescriptionFile="");

	/**
	 * \brief Copy constructor
	 *
	 * \param other The object to copy from
	 */
	DBFactoryContainer(const DBFactoryContainer& other);

	/**
	 * \brief Class destructor
	 *
	 * Will ensure the underlying DBManager object reference count is decremented (and destructed by factory if reference count reaches 0)
	 */
	~DBFactoryContainer();

	/**
	 * \brief swap function to allow implementing of copy-and-swap idom on members of type DBFactoryContainer
	 *
	 * This function will swap all attributes of \p first and \p second
	 * See http://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	 *
	 * \param first The first object
	 * \param second The second object
	 */
	friend void swap(DBFactoryContainer& first, DBFactoryContainer& second);

	DBManager& dbm;	/*!< The DBManager object encapsulated in this container */

private:
	std::string dbLocation;	/*!< The location URL of the database handled by the DBManager object encapsulated in this container */
	std::string configurationDescriptionFile;	/*!< The path to the configuration file to use for the encapsulated DBManager object in this container, or the configuration content directly provided as a std::string (no carriage return allowed in this case) */
};

#endif //_DBFACTORY_HPP_
