#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "dbfactory.hpp"
#include "dbmanagercontainer.hpp"

#include "common/tools.hpp"

using namespace std;

const char* progname;	/* The name under which we were called */

string database_url;


TEST_GROUP(SQLiteDBManagerToStringTests) {
};

TEST(SQLiteDBManagerToStringTests, toStringEmptyTable) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";

	DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
"<database><table name=\"" TEST_TABLE_NAME "\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table>\n</database>");
	
	string db_dump(dbmc.getDBManager().to_string());
	cout << db_dump;
	if (db_dump.find(TEST_TABLE_NAME " is empty") == string::npos) {
		FAIL("Could not find 'is empty' text.\n");
	}
	if (db_dump.find("Columns are: field1, field2") == string::npos) {
		FAIL("Could not find column list line.\n");
	}
};

TEST(SQLiteDBManagerToStringTests, toStringDefaultTable) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";

	DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
"<database><table name=\"" TEST_TABLE_NAME "\">\n" \
"<field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />\n" \
"<field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />\n" \
"<default-records>\n" \
"<record><field name=\"field1\" value=\"val1-R1\" /><field name=\"field2\" value=\"val2-R1\" /></record>\n" \
"<record><field name=\"field1\" value=\"val1-R2\" /><field name=\"field2\" value=\"val2-R2\" /></record>\n" \
"</default-records>\n" \
"</table>\n</database>");
	
	string db_dump(dbmc.getDBManager().to_string());
	cout << db_dump;
	if (db_dump.find("Table: " TEST_TABLE_NAME) == string::npos) {
		FAIL("Could not find table name.\n");
	}
	if (db_dump.find("field1") == string::npos) {
		FAIL("Could not find field1 header entry.\n");
	}
	if (db_dump.find("field2") == string::npos) {
		FAIL("Could not find field2 header entry.\n");
	}
	if (db_dump.find("val1-R1") == string::npos) {
		FAIL("Could not find val1-R1 entry.\n");
	}
	if (db_dump.find("val2-R1") == string::npos) {
		FAIL("Could not find val2-R1 entry.\n");
	}
	if (db_dump.find("val1-R2") == string::npos) {
		FAIL("Could not find val1-R2 entry.\n");
	}
	if (db_dump.find("val2-R2") == string::npos) {
		FAIL("Could not find val2-R2 entry.\n");
	}
};

TEST(SQLiteDBManagerToStringTests, toHtmlDefaultTable) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";

	DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
"<database><table name=\"" TEST_TABLE_NAME "\">\n" \
"<field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />\n" \
"<field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />\n" \
"<default-records>\n" \
"<record><field name=\"field1\" value=\"val1-R1\" /><field name=\"field2\" value=\"val2-R1\" /></record>\n" \
"<record><field name=\"field1\" value=\"val1-R2\" /><field name=\"field2\" value=\"val2-R2\" /></record>\n" \
"</default-records>\n" \
"</table>\n</database>");
	
	string db_dump(dbmc.getDBManager().dumpTablesAsHtml());
	cout << db_dump;
	string expected = "<h3> Table : " TEST_TABLE_NAME "</h3><table class=\"table table-striped table-bordered table-hover\"><thead><tr><th>field1</th><th>field2</th></tr></thead><tbody><tr><td>val1-R1</td><td>val2-R1</td></tr><tr><td>val1-R2</td><td>val2-R2</td></tr></tbody></table>";
	if (db_dump.find(expected) == string::npos) {
		FAIL("Did not get a exact match on table dump string. Please check the differences.");
	}
}

TEST(SQLiteDBManagerToStringTests, toStringManuallyFilledTableOneRecord) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";

	DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
"<database><table name=\"" TEST_TABLE_NAME "\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table>\n</database>");
	
	vector<map<string,string>> vals;
	map<string, string> vals1;
	vals1.emplace("field1", "val1");
	vals1.emplace("field2", "val2");
	vals.push_back(vals1);
	dbmc.getDBManager().insert(TEST_TABLE_NAME, vals);
	
	string db_dump(dbmc.getDBManager().to_string());
	cout << db_dump;
	if (db_dump.find("Table: " TEST_TABLE_NAME) == string::npos) {
		FAIL("Could not find table name.\n");
	}
	if (db_dump.find("field1") == string::npos) {
		FAIL("Could not find field1 header entry.\n");
	}
	if (db_dump.find("field2") == string::npos) {
		FAIL("Could not find field2 header entry.\n");
	}
	if (db_dump.find("val1") == string::npos) {
		FAIL("Could not find val1 entry.\n");
	}
	if (db_dump.find("val2") == string::npos) {
		FAIL("Could not find val2 entry.\n");
	}
};

TEST(SQLiteDBManagerToStringTests, toStringManuallyFilledTableTwoRecordsLoostCheck) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";

	DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
"<database><table name=\"" TEST_TABLE_NAME "\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table>\n</database>");
	
	vector<map<string,string>> vals;
	map<string, string> vals1;
	vals1.emplace("field1", "val1-R1");
	vals1.emplace("field2", "val2-R1");
	vals.push_back(vals1);
	map<string, string> vals2;
	vals2.emplace("field1", "val1-R2");
	vals2.emplace("field2", "val2-R2");
	vals.push_back(vals2);
	dbmc.getDBManager().insert(TEST_TABLE_NAME, vals);
	
	string db_dump(dbmc.getDBManager().to_string());
	cout << db_dump;
	if (db_dump.find("Table: " TEST_TABLE_NAME) == string::npos) {
		FAIL("Could not find table name.\n");
	}
	if (db_dump.find("field1") == string::npos) {
		FAIL("Could not find field1 header entry.\n");
	}
	if (db_dump.find("field2") == string::npos) {
		FAIL("Could not find field2 header entry.\n");
	}
	if (db_dump.find("val1-R1") == string::npos) {
		FAIL("Could not find val1-R1 entry.\n");
	}
	if (db_dump.find("val2-R1") == string::npos) {
		FAIL("Could not find val2-R1 entry.\n");
	}
	if (db_dump.find("val1-R2") == string::npos) {
		FAIL("Could not find val1-R2 entry.\n");
	}
	if (db_dump.find("val2-R2") == string::npos) {
		FAIL("Could not find val2-R2 entry.\n");
	}
};

TEST(SQLiteDBManagerToStringTests, toStringManuallyFilledTableTwoRecordsExactCheck) {

	string tmp_fn = mktemp_filename(progname);
	string database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\" for database\n";

	DBManagerContainer dbmc(database_url, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" \
"<database><table name=\"" TEST_TABLE_NAME "\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table>\n</database>");
	
	vector<map<string,string>> vals;
	map<string, string> vals1;
	vals1.emplace("field1", "val1-R1");
	vals1.emplace("field2", "val2-R1");
	vals.push_back(vals1);
	map<string, string> vals2;
	vals2.emplace("field1", "val1-R2");
	vals2.emplace("field2", "val2-R2");
	vals.push_back(vals2);
	dbmc.getDBManager().insert(TEST_TABLE_NAME, vals);
	
	string db_dump(dbmc.getDBManager().to_string());
	cout << db_dump;
	string expected = "+---------+---------+\n" \
"| field1  | field2  |\n" \
"+---------+---------+\n" \
"| val1-R1 | val2-R1 |\n" \
"| val1-R2 | val2-R2 |\n" \
"+---------+---------+";
	if (db_dump.find(expected) == string::npos) {
		FAIL("Did not get a exact match on table dump string. Please check the differences.");
	}
};


int main(int argc, char** argv) {
	
	int rc;
	
	progname = get_progname();
	
	string tmp_fn = mktemp_filename(progname);
	database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\"\n";
	
	{
		/* Note: we need to create a first instance of the DBManager outside of the cpputest macros, otherwise a memory leak is detected */
		DBManager &manager = DBManagerFactory::getInstance().getDBManager(database_url);
		
		rc = CommandLineTestRunner::RunAllTests(argc, argv);
	}
	remove(tmp_fn.c_str());	/* Remove the temporary database file */
	return rc;
}
