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

#include "dbmanagerapi.hpp"	// For LIBDBMANAGER_API

//Library includes
#include "dbmanager.hpp"

/**
 * \class DBManagerAllocationSlot
 *
 * \brief Structure holding the details of the allocation slot matching with one URL
 */
class DBManagerAllocationSlot {
	
	/**
	 * \brief swap function to allow implementing of copy-and-swap idom on members of type SmartTool::Data
	 *
	 * This function will swap all attributes of \p first and \p second
	 * See http://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
	 *
	 * \param first The first object
	 * \param second The second object
	 */
	friend void (::swap)(DBManagerAllocationSlot& first, DBManagerAllocationSlot& second) noexcept;

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
	DBManagerAllocationSlot(DBManager *managerPtr);

	/**
	 * \brief Copy constructor.
	 *
	 * \param other The object to construct from
	 */
	DBManagerAllocationSlot(const DBManagerAllocationSlot& other);
	
	/**
	 * \brief Assignment operator.
	 *
	 * \param other The object assigned to us
	 * \return Ourselves, with our new identity
	 */
	DBManagerAllocationSlot& operator=(DBManagerAllocationSlot other);
	
};

/**
 * \class DBManagerFactory
 *
 * \brief Class to obtain database managers.
 *
 * This class has methods to obtains database manager instances based on specific URL.
 * For example the URL "sqlite://[sqlite database file full path]" will give you a database manager that handle SQLite databases.
 * This class implements the singleton design pattern (in the lazy way).
 *
 */
class LIBDBMANAGER_API DBManagerFactory {
public:
	friend class DBManagerContainer;
	friend class DBManagerFactoryTestProxy;	/* Friend class for unit test purposes */

	/**
	 * \brief Instance getter
	 *
	 * This method allows to obtain the unique instance of the DBManagerFactory class.
	 *
	 * \return DBManagerFactory& The reference to the unique instance of the DBManagerFactory class.
	 */
	static DBManagerFactory& getInstance();

	/**
	 * \brief DBManager getter
	 *
	 * This method allows to obtain an instance of the DBManager class that is configured to manage the database located at the location parameter.
	 *
	 * \param location The location, in a URL address, of the database to manage.
	 * \param configurationDescriptionFile The path to the configuration file to use for this database, or the configuration content directly provided as a std::string (no carriage return allowed in this case)
	 * \param exclusive When true, ensures that only one reference can exist at a time for this location. If a second call is performed on getDBManager, an exception will be raised
	 * \return DBManager& The reference to an instance of the DBManager class.
	 */
	DBManager& getDBManager(std::string location, std::string configurationDescriptionFile="", bool exclusive = false);
	
	/**
	 * \brief DBManager releaser
	 *
	 * This method allows to notify the factory that the served instance of a DBManager is not used anymore.
	 *
	 * \param location The location, in a URL address, of the database concerned by the notification.
	 */
	void freeDBManager(std::string location);

private:
	DBManagerFactory();
	~DBManagerFactory();

	/**
	 * \brief Increment reference count for a specific location
	 *
	 * This method will increment the reference count for a given location, meaning we are publishing (once more) a reference on a DBManager object
	 *
	 * \param location The URL for which we decrement the reference count
	 */
	void incRefCount(const std::string& location);

	/**
	 * \brief Decrement reference count for a specific location
	 *
	 * This method will decrement the reference count for a given location, meaning we assume a reference on a DBManager object given by getDBManager() will not be used anymore
	 *
	 * \param location The URL for which we decrement the reference count
	 */
	void decRefCount(const std::string& location);

	/**
	 * \brief Get the reference count for a specific location
	 *
	 * This method will return the reference count for a given location
	 * Warning, it may raise an exception if the location provided is unknown
	 *
	 * \param location The URL for which we want the reference count
	 * \return The reference count
	 */
	unsigned int getRefCount(const std::string& location) const;

	/**
	 * \brief Check if a specific location is still used (from its reference count)
	 *
	 * This method will check if the reference count for a given location is at least 1
	 *
	 * \param location The URL for which we decrement the reference count
	 * \return true if the reference count is 1 or more
	 */
	bool isUsed(const std::string& location) const;

	/**
	 * \brief Extract the protocol part of a location URL
	 *
	 * locationUrlToPath("sqlite:///tmp") => "sqlite"
	 *
	 * \param location The URL string
	 * \return The protocol part as a string
	 */
	std::string locationUrlToProto(const std::string& location) const;
	/**
	 * \brief Extract the path part of a location URL
	 *
	 * locationUrlToPath("sqlite:///tmp") => "/tmp"
	 *
	 * \param location The URL string
	 * \return The path part as a string
	 */
	std::string locationUrlToPath(const std::string& location) const;

	/**
	 * \brief Free all DB managers that have been allocated by this factory
	 *
	 * \param ignoreRefCount If true, free DBManagers even if they refcount is not 0, if false, raise an exception if at least one refcount is not 0
	 *
	 * Warning: this method only exists for unit test purposes. Do not use it for any other purposes.
	 * Indeed, it could deallocate DBManager for which pointers still exist (they may have been given to the outside), or contained in a DBManagerContainer that still exists...
	 * To protect from such cases, if \p ignoreRefCount is false, we will raise an exception whenever the refCount is not 0 for any DBManager in the store
	 */
	void freeAllDBManagers(bool ignoreRefCount = false);

	std::map<std::string, DBManagerAllocationSlot> managersStore;	/*!< A map (containing elements called "slots" in this code) storing all allocated instances of DBManager objects. The key is a location string, the payload is an DBManagerAllocationSlot object */
	/* Note: when accessing an element of this map, use the std::map::at() method, because DBManagerAllocationSlot's constructor requires one argument and std::map::operator[] needs to be able to insert an element using a constructor without argument */

};

#endif //_DBFACTORY_HPP_
