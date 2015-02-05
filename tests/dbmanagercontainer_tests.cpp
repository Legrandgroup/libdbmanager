#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "dbmanagercontainer.hpp"
#include "dbfactorytestproxy.hpp"	/* Use the test proxy to access internals of DBManagerFactory */

#include <iostream>
#include <fstream>
#include <stdio.h>	/* For remove() */

#include "common/tools.hpp"

using namespace std;

const char* progname;	/* The name under which we were called */
DBManagerFactoryTestProxy factoryProxy;
string database_url;
string database_structure = "<?xml version=\"1.0\" encoding=\"utf-8\"?><database><table name=\"" TEST_TABLE_NAME "\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked1\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked2\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><relationship kind=\"m:n\" policy=\"link-all\" first-table=\"linked1\" second-table=\"linked2\" /></database>";


TEST_GROUP(DBManagerContainerTests) {
};

TEST(DBManagerContainerTests, simpleAllocationFreeCheck1) {
	
	unsigned int countmanager = factoryProxy.getRefCount(database_url);
	std::cerr << "Starting " + string(__func__) + "(). Currently " + to_string(countmanager) << " managers allocated\n";
	{
		DBManagerContainer dbmc(database_url);	// Create a database without migration
		if (factoryProxy.getRefCount(database_url) != countmanager+1) {
			FAIL("DBManager ref count not incremented after container instanciation.");
		}
	}
	if (factoryProxy.getRefCount(database_url) != countmanager) {
		FAIL("DBManager ref count not restored after container destruction.");
	}
	
	std::cerr << "Leaving " + string(__func__) + "(). Currently " + to_string(factoryProxy.getRefCount(database_url)) << " managers allocated\n";
};

TEST(DBManagerContainerTests, simpleAllocationFreeCheck2) {
	
	unsigned int countmanager = factoryProxy.getRefCount(database_url);
	std::cerr << "Starting " + string(__func__) + "(). Currently " + to_string(countmanager) << " managers allocated\n";
	{
		DBManagerContainer dbmc(database_url, database_structure);	// Create a database with migration
		if (factoryProxy.getRefCount(database_url) != countmanager+1) {
			FAIL("DBManager ref count not incremented after container instanciation.");
		}
	}
	if (factoryProxy.getRefCount(database_url) != countmanager) {
		FAIL("DBManager ref count not restored after container destruction.");
	}
	std::cerr << "Leaving " + string(__func__) + "(). Currently " + to_string(factoryProxy.getRefCount(database_url)) << " managers allocated\n";
};

TEST(DBManagerContainerTests, doubleAllocationCheck) {
	
	unsigned int countmanager = factoryProxy.getRefCount(database_url);
	std::cerr << "Starting " + string(__func__) + "(). Currently " + to_string(countmanager) << " managers allocated\n";
	{
		DBManagerContainer dbmc(database_url, database_structure);	// Create a database with migration
		if (factoryProxy.getRefCount(database_url) != countmanager+1) {
			FAIL("DBManager ref count not incremented after container instanciation.");
		}
		DBManagerContainer dbmc2(database_url, database_structure);
		if (factoryProxy.getRefCount(database_url) != countmanager+2) {
			FAIL("DBManager ref count not incremented twice after 2 containers instanciation.");
		}
	}
	if (factoryProxy.getRefCount(database_url) != countmanager) {
		FAIL("DBManager ref count not restored after container destruction.");
	}
	std::cerr << "Leaving " + string(__func__) + "(). Currently " + to_string(factoryProxy.getRefCount(database_url)) << " managers allocated\n";
};

TEST(DBManagerContainerTests, checkAllocationNoLeakWhenException) {
	
	unsigned int countmanager = factoryProxy.getRefCount(database_url);
	try {
		DBManagerContainer dbmc(database_url, database_structure);
		if (factoryProxy.getRefCount(database_url) != countmanager+1) {
			FAIL("DBManager ref count not incremented after container instanciation.");
		}
		throw std::exception();
	}
	catch(const std::exception &e) {
	}
	if (factoryProxy.getRefCount(database_url) != countmanager) {
		FAIL("DBManager ref count not restored after container destruction.");
	}
}

TEST(DBManagerContainerTests, checkAllocationInterferencesBetweenTwoDatabases) {
	
	/* Generate two databases */
	string tmp_fn1 = mktemp_filename(progname);
	string database_url1 = DATABASE_SQLITE_TYPE + tmp_fn1;
	cerr << "Will use temporary file \"" + tmp_fn1 + "\" for database 1\n";
	
	string tmp_fn2 = mktemp_filename(progname);
	string database_url2 = DATABASE_SQLITE_TYPE + tmp_fn2;
	cerr << "Will use temporary file \"" + tmp_fn2 + "\" for database 2\n";
	
	{
		DBManagerContainer dbmc1(database_url1, database_structure);
		if (factoryProxy.getRefCount(database_url1) != 1) {
			FAIL("DBManager 1 ref count not incremented after container instanciation.");
		}
		DBManagerContainer dbmc2(database_url2, database_structure);
		if (factoryProxy.getRefCount(database_url2) != 1) {
			FAIL("DBManager 2 ref count not incremented after container instanciation.");
		}
		
		map<string, string> vals1;
		vals1.emplace("field1", "dbmc1");
		dbmc1.getDBManager().insert(TEST_TABLE_NAME, vals1);	/* Insert in database 1 */
		
		map<string, string> vals2;
		vals1.emplace("field1", "dbmc1");
		dbmc2.getDBManager().insert(TEST_TABLE_NAME, vals2);	/* Insert in database 2 */
		
		bool testOk = false;
		for(auto &it : dbmc1.getDBManager().get(TEST_TABLE_NAME))
			if(it["field1"] == vals1["field1"])
				testOk = true;
		for(auto &it : dbmc2.getDBManager().get(TEST_TABLE_NAME))
			if(it["field1"] == vals2["field1"])
				testOk = true;
		
		if(!testOk)
			FAIL("Issue in one record insertion in database.");
	}
	if (factoryProxy.getRefCount(database_url1) != 0) {
		FAIL("DBManager 1 ref count not zeroed after container destruction.");
	}
	if (factoryProxy.getRefCount(database_url2) != 0) {
		FAIL("DBManager 2 ref count not zeroed after container destruction.");
	}
	remove(tmp_fn1.c_str());	/* Remove the temporary database files */
	remove(tmp_fn2.c_str());
}

TEST(DBManagerContainerTests, checkStructureFrombufferOrFile) {
	/* Generate two databases */
	string tmp_fn1 = mktemp_filename(progname);
	string database_url1 = DATABASE_SQLITE_TYPE + tmp_fn1;
	cerr << "Will use temporary file \"" + tmp_fn1 + "\" for database 1\n";
	
	string tmp_fn2 = mktemp_filename(progname);
	string database_url2 = DATABASE_SQLITE_TYPE + tmp_fn2;
	cerr << "Will use temporary file \"" + tmp_fn2 + "\" for database 2\n";
	
	string tmp_dbstruct_fn = mktemp_filename(progname);
	ofstream dbstruct_file;
	dbstruct_file.open(tmp_dbstruct_fn);
	dbstruct_file << database_structure;
	dbstruct_file.close();
	
	{
		DBManagerContainer dbmc1(database_url1, database_structure);
		DBManagerContainer dbmc2(database_url2, tmp_dbstruct_fn);
		
		map<string, string> vals;
		vals.emplace("field1", "val1");
		dbmc1.getDBManager().insert(TEST_TABLE_NAME, vals);	/* Insert in database 1 */
		dbmc2.getDBManager().insert(TEST_TABLE_NAME, vals);	/* Insert in database 2 */
		
		if (dbmc1.getDBManager().get(TEST_TABLE_NAME) != dbmc2.getDBManager().get(TEST_TABLE_NAME))
			FAIL("Both databases do not match.");
	}
	remove(tmp_fn1.c_str());	/* Remove the temporary database files and structure description file */
	remove(tmp_fn2.c_str());
	remove(tmp_dbstruct_fn.c_str());
}

TEST(DBManagerContainerTests, checkTableNameWithDoubleQuote) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";
		{
		DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<database><table name='tablename\"test'><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table></database>");
		
		map<string, string> vals;
		vals.emplace("field1", "val1");
		dbmc.getDBManager().insert("tablename\"test", vals);	/* Insert test record in database */
		
		bool testOk = false;
		for(auto &it : dbmc.getDBManager().get("tablename\"test"))
			if(it["field1"] == vals["field1"])
				testOk = true;
	}
	remove(tmp_fn.c_str());	/* Remove the temporary database file */
}

TEST(DBManagerContainerTests, checkFieldNameWithDoubleQuote) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";
		{
		DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<database><table name=\"" TEST_TABLE_NAME "\"><field name='fieldname\"test' default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table></database>");
		
		map<string, string> vals;
		vals.emplace("fieldname\"test", "val1");
		dbmc.getDBManager().insert(TEST_TABLE_NAME, vals);	/* Insert test record in database */
		
		bool testOk = false;
		for(auto &it : dbmc.getDBManager().get(TEST_TABLE_NAME))
			if(it["fieldname\"test"] == vals["fieldname\"test"])
				testOk = true;
	}
	remove(tmp_fn.c_str());	/* Remove the temporary database file */
}

TEST(DBManagerContainerTests, checkGetDBManager) {
	DBManagerContainer dbmc(database_url, database_structure);
	
	DBManager& dbm1 = DBManagerFactory::getInstance().getDBManager(database_url, database_structure);
	DBManager& dbm2 = dbmc.getDBManager();
	POINTERS_EQUAL(&dbm1, &dbm2);	/* Make sure container returns the same as the factory */
}


int main(int argc, char** argv) {
	
	int rc;
	
	progname = get_progname();
	
	string tmp_fn = mktemp_filename(progname);
	database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\"\n";
	
	{
		/* Note: we need to create a first instance of the DBManager outside of the cpputest macros, otherwise a memory leak is detected */
		DBManager &manager = DBManagerFactory::getInstance().getDBManager(database_url, database_structure);
		
		rc = CommandLineTestRunner::RunAllTests(argc, argv);
	}
	remove(tmp_fn.c_str());	/* Remove the temporary database file */
	return rc;
}
