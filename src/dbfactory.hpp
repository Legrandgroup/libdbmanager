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
	static DBFactory& getInstance();
	DBManager& getDBManager(std::string location, std::string configurationDescriptionFile="");
	void freeDBManager(std::string location);

private:
	DBFactory();
	~DBFactory();

	void markRequest(std::string location);
	void unmarkRequest(std::string location);
	bool isUsed(std::string location);

	string getDatabaseKind(string location);
	string getUrl(string location);

	std::map<std::string, DBManager*> allocatedManagers;
	std::map<std::string, unsigned int> requests;
};

#endif //_DBFACTORY_HPP_
