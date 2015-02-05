#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "dbfactory.hpp"

#include "common/tools.hpp"

using namespace std;

TEST_GROUP(DBManagerFactoryTests) {
};

TEST(DBManagerFactoryTests, getInstanceTest) {
	DBManagerFactory &appel1 = DBManagerFactory::getInstance();
	DBManagerFactory &appel2 = DBManagerFactory::getInstance();
	if(&appel1 != &appel2)
		FAIL("DBManagerFactory references are not the same.");
};

int main(int argc, char** argv) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
