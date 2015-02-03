/**
 *
 * \file dbfactorytestproxy.hpp
 *
 * \brief Container to manipulate instances of DBManagerFactory for unit tests
 */

#ifndef _DBFACTORYTESTPROXY_HPP_
#define _DBFACTORYTESTPROXY_HPP_

//STL includes
#include <string>

//Library includes
#include "dbfactory.hpp"

/**
 * \class DBManagerFactoryTestProxy
 *
 * \brief Class to allow deep access to a DBManagerFactory for test purposes only
 * This class allows us to access internal values of DBManagerFactory during tests.
 * It is declared as a friend of DBManagerFactory for this purpose
 */
 
class DBManagerFactoryTestProxy {
public:
	DBManagerFactory& factory;	/*!< A reference to the factory (singleton) */
	
public:
	/**
	 * \brief Class constructor
	 *
	 * Arguments are the same as DBManagerFactory::getDBManager()
	 *
	 * \param dbLocation The location, in a URL address, of the database to manage.
	 * \param configurationDescriptionFile The path to the configuration file to use for this database, or the configuration content directly provided as a std::string (no carriage return allowed in this case)
	 */
	DBManagerFactoryTestProxy() : factory(DBManagerFactory::getInstance()) { }

	/**
	 * \brief Class destructor
	 *
	 * Will ensure the underlying DBManager object reference count is decremented (and destructed by factory if reference count reaches 0)
	 */
	~DBManagerFactoryTestProxy() { }
	
	/**
	 * \brief Wrapper around DBManagerFactory::getRefCount()
	 *
	 * \param location The URL for which we want the reference count
	 * \return The reference count
	 */
	unsigned int getRefCount(std::string location) const {
		return this->factory.getRefCount(location);
	}
};

#endif	// _DBMANAGERFACTORYPROXY_HPP_