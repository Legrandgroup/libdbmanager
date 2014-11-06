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

	DBManager* getDBManager(std::string location, std::string port = "");
	void freeDBManager(std::string location, std::string port = "");
private:
	std::map<std::string, DBManager*> allocatedManagers;
	std::map<std::string, std::string> databasePorts;
};

#endif //_DBFACTORY_HPP_
