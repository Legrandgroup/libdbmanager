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
#include <fstream>

#ifdef __unix__
extern "C" {
	#include <sys/file.h>
}
#endif

#include "dbmanagercontainer.hpp"
#include "dbfactory.hpp"

using namespace std;

DBManagerContainer::DBManagerContainer(std::string dbLocation, std::string configurationDescriptionFile, bool exclusive):
		dbLocation(dbLocation),
		configurationDescriptionFile(configurationDescriptionFile),
		exclusive(exclusive),
		dbm(DBManagerFactory::getInstance().getDBManager(dbLocation, configurationDescriptionFile, exclusive)) {
}

DBManagerContainer::DBManagerContainer(const DBManagerContainer& other):
		dbLocation(other.dbLocation),
		configurationDescriptionFile(other.configurationDescriptionFile),
		exclusive(other.exclusive),
		dbm(DBManagerFactory::getInstance().getDBManager(dbLocation, configurationDescriptionFile, exclusive)) {
}

DBManagerContainer::~DBManagerContainer() {
	DBManagerFactory::getInstance().freeDBManager(this->dbLocation);
}

void swap(DBManagerContainer& first, DBManagerContainer& second) {
	using std::swap;	// Enable ADL

	swap(first.dbLocation, second.dbLocation);
	swap(first.configurationDescriptionFile, second.configurationDescriptionFile);
	swap(first.exclusive, second.exclusive);

	/* We whould use swap() here to be exception safe but DBManager does not implement the swap() method */
	/* Because we are dealing with references, there is no risk of exception during the following 3 lines, which is what we must ensure in this swap() method */
	DBManager& temp = first.dbm;
	try {
		first.dbm = second.dbm;
		second.dbm = temp;
	}
	catch (exception& ex) {
		cerr << string(__func__) + "(): Error while swapping arguments' dbm attributes\n";
		throw ex;
	}
	/* Once we have swapped the members of the two instances... the two instances have actually been swapped */
}
