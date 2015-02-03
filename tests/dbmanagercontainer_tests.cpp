#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "dbmanagercontainer.hpp"
#include "dbfactorytestproxy.hpp"	/* Use the test proxy to access internals of DBManagerFactory */

#include <iostream>
#include <string.h>
#include <stdio.h>	/* For remove() */

using namespace std;

#define TEST_TABLE_NAME "unittests"

char *progname;	/* The name under which we were called */
DBManagerFactoryTestProxy factoryProxy;
string database_url_prefix = "sqlite://";
string database_url;
string database_structure = "<?xml version=\"1.0\" encoding=\"utf-8\"?><database><table name=\"unittests\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked1\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked2\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><relationship kind=\"m:n\" policy=\"link-all\" first-table=\"linked1\" second-table=\"linked2\" /></database>";

string mktemp_filename(string filename) {
	
	string tmpdir;
#ifdef __unix__
	tmpdir = "/tmp/";
#elif _WIN32
	tmpdir = string(getenv("TEMP")) + '/';
#endif
	
	filename = tmpdir + filename + "-XXXXXX";	/* Add the expected mktemp template */
	
	size_t filenameSz = filename.size();
	
	char tmpfn_template_cstr[filenameSz+1];	/* +1 to hold terminating \0 */
	strncpy(tmpfn_template_cstr, filename.c_str(), filenameSz+1);
	tmpfn_template_cstr[filenameSz] = '\0';	/* Make sure we terminate the C-string */
	return string(mktemp(tmpfn_template_cstr));
}

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
	string database_url1 = database_url_prefix + tmp_fn1;
	cerr << "Will use temporary file \"" + tmp_fn1 + "\" for database 1\n";
	
	string tmp_fn2 = mktemp_filename(progname);
	string database_url2 = database_url_prefix + tmp_fn2;
	cerr << "Will use temporary file \"" + tmp_fn2 + "\" for database 2\n";
	
	unsigned int countmanager1 = 0;	/* No reference served for database 1 */
	unsigned int countmanager2 = 0;	/* No reference served for database 2 */
	{
		DBManagerContainer dbmc1(database_url1, database_structure);
		if (factoryProxy.getRefCount(database_url1) != 1) {
			FAIL("DBManager 1 ref count not incremented after container instanciation.");
		}
		DBManagerContainer dbmc2(database_url2, database_structure);
		if (factoryProxy.getRefCount(database_url2) != 1) {
			FAIL("DBManager 2 ref count not incremented after container instanciation.");
		}
	}
	if (factoryProxy.getRefCount(database_url1) != 0) {
		FAIL("DBManager 1 ref count not restored after container destruction.");
	}
	if (factoryProxy.getRefCount(database_url2) != 0) {
		FAIL("DBManager 2 ref count not restored after container destruction.");
	}
}

TEST(DBManagerContainerTests, checkStructureFrombufferOrFile) {
	// FIXME: Lionel, create two instances, one from XML buffer, the other one from the same buffer dumped to a file
	// Compare both structures
}

TEST(DBManagerContainerTests, checkGetDBManager) {
	DBManagerContainer dbmc(database_url, database_structure);
	
	DBManager& dbm1 = DBManagerFactory::getInstance().getDBManager(database_url, database_structure);
	DBManager& dbm2 = dbmc.getDBManager();
	POINTERS_EQUAL(&dbm1, &dbm2);	/* Make sure container returns the same as the factory */
}


int main(int argc, char** argv) {
	
	int rc;
	
	progname = strrchr(argv[0], '/');	/* Find the last / in progname */
	if (progname)	/* We found the last occurence of / in argv[0], just move forward to point to the exec name */
		progname++;
	else	/* We did not find any '/', use argv[0] as is */
		progname = argv[0];
	
	string tmp_fn = mktemp_filename(progname);
	database_url = database_url_prefix + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\"\n";
	
	{
		/* Note: we need to create a first instance of the DBManager outside of the cpputest macros, otherwise a memory leak is detected */
		DBManager &manager = DBManagerFactory::getInstance().getDBManager(database_url, database_structure);
		
		rc = CommandLineTestRunner::RunAllTests(argc, argv);
	}
	remove(tmp_fn.c_str());	/* Remove the temporary database file */
	return rc;
}
