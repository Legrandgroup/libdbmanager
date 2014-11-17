#ifndef _DBFACTORY_HPP_
#define _DBFACTORY_HPP_

//STL includes
#include <string>
#include <map>
#include <exception>

//Library includes
#include "dbmanager.hpp"

/**
 * \interface DBFactory
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
	 * \param location The location, in a URI address, of the database to manage.
	 * \param configurationDescriptionFile The path to the configuration file to use for this database.
	 * \return DBManager& The reference to an instance of the DBManager class.
	 */
	DBManager& getDBManager(std::string location, std::string configurationDescriptionFile="");
	/**
	 * \brief DBManager releaser
	 *
	 * This method allows to notify the factory that the served instance of a DBManager is not used anymore.
	 *
	 * \param location The location, in a URI address, of the database concerned by the notification.
	 */
	void freeDBManager(std::string location);

private:
	DBFactory();
	~DBFactory();

	void markRequest(std::string location);
	void unmarkRequest(std::string location);
	bool isUsed(std::string location);

	std::string getDatabaseKind(std::string location);
	std::string getUrl(std::string location);

	std::map<std::string, DBManager*> allocatedManagers;
	std::map<std::string, unsigned int> requests;
};

#endif //_DBFACTORY_HPP_
