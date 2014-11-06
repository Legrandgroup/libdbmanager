#ifndef _DBFACTORY_HPP_
#define _DBFACTORY_HPP_

//STL includes
#include <string>
#include <map>
#include <exception>

//Library includes
#include "dbmanager.hpp"

class DBFactory {
public:
	DBFactory();
	~DBFactory();

	DBManager* getDBManager(std::string location, std::string configurationDescriptionFile="");
	void freeDBManager(std::string location);
private:
	std::map<std::string, DBManager*> allocatedManagers;
};

#endif //_DBFACTORY_HPP_
