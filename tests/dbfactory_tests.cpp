#include "dbfactory.hpp"

#include "common/tools.hpp"

#include <CppUTest/TestHarness.h>	// cpputest headers should come after all other headers to avoid compilation errors with gcc 6
#include <CppUTest/CommandLineTestRunner.h>

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
