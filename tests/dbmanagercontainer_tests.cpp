#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "dbmanagercontainer.hpp"
#include "dbfactorytestproxy.hpp"	/* Use the test proxy to access internals of DBManagerFactory */

using namespace std;

#define TEST_TABLE_NAME "unittests"

DBManager &manager = DBManagerFactory::getInstance().getDBManager("sqlite://tmp/utests.sqlite", "<?xml version=\"1.0\" encoding=\"utf-8\"?><database><table name=\"unittests\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked1\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked2\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><relationship kind=\"m:n\" policy=\"link-all\" first-table=\"linked1\" second-table=\"linked2\" /></database>");
DBManagerFactoryTestProxy factoryProxy;
string database_url = "sqlite://tmp/utests.sqlite";
string database_structure = "<?xml version=\"1.0\" encoding=\"utf-8\"?><database><table name=\"unittests\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked1\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked2\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><relationship kind=\"m:n\" policy=\"link-all\" first-table=\"linked1\" second-table=\"linked2\" /></database>";
 
TEST_GROUP(DBManagerContainerTests) {
};

TEST(DBManagerContainerTests, simpleAllocationFreeCheck1) {
	
	unsigned int countmanager = factoryProxy.getRefCount(database_url);
	{
		DBManagerContainer dbmc(database_url);	// Create a database without migration
		if (factoryProxy.getRefCount(database_url) != countmanager+1) {
			FAIL("DBManager ref count not incremented after container instanciation.");
		}
	}
	if (factoryProxy.getRefCount(database_url) != countmanager) {
		FAIL("DBManager ref count not restored after container destruction.");
	}
};

TEST(DBManagerContainerTests, simpleAllocationFreeCheck2) {
	
	unsigned int countmanager = factoryProxy.getRefCount(database_url);
	{
		DBManagerContainer dbmc(database_url, database_structure);	// Create a database with migration
		if (factoryProxy.getRefCount(database_url) != countmanager+1) {
			FAIL("DBManager ref count not incremented after container instanciation.");
		}
	}
	if (factoryProxy.getRefCount(database_url) != countmanager) {
		FAIL("DBManager ref count not restored after container destruction.");
	}
};


int main(int argc, char** argv) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
