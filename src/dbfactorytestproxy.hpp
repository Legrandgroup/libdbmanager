/*
This file is part of libdbmanager
(see the file COPYING in the root of the sources for a link to the
homepage of libdbmanager)

libdbmanager is a C++ library providing methods for reading/modifying a
database using only C++ methods & objects and no SQL
Copyright (C) 2016 Legrand SA

libdbmanager is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License version 3
(dated 29 June 2007) as published by the Free Software Foundation.

libdbmanager is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libdbmanager (in the source code, it is enclosed in
the file named "lgpl-3.0.txt" in the root of the sources).
If not, see <http://www.gnu.org/licenses/>.
*/
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
	unsigned int getRefCount(const std::string& location) const {
		try {
			return this->factory.getRefCount(location);
		}
		catch (const std::out_of_range& ex) {	/* If this location is unknown, libdbmanager raises an exception, we return 0 instead */
			return 0;
		}
	}

	/**
	 * \brief Free all DB managers that have been allocated by this factory
	 */
	void freeAllDBManagers() {
		this->factory.freeAllDBManagers(false);
	}
};

#endif	// _DBMANAGERFACTORYPROXY_HPP_
