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
