/**
 *
 * * \file dbmanager.hpp
 *
 * * \brief Class for managing requests to a sqlite3 database
 *
 * */

#ifndef _DBMANAGER_HPP_
#define _DBMANAGER_HPP_

#include <string>
#include <exception>

extern "C" {
	#include <sqlite3.h>
}

#include "dbconfig.hpp"

using namespace std;

class DBManager {
public:
	static DBManager* GetInstance();



private :
	DBManager(string filename = PATH_DB);
	~DBManager();

	void open() throw (exception);

	static DBManager* instance;
	string filename;
	sqlite3 *db;

	
}

#endif //_DBMANAGER_HPP_
