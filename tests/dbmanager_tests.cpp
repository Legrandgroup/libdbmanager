#include "dbfactory.hpp"

#include "common/tools.hpp"

#include <CppUTest/TestHarness.h>	// cpputest headers should come after all other headers to avoid compilation errors with gcc 6
#include <CppUTest/CommandLineTestRunner.h>

using namespace std;

DBManager* global_manager;	/* One unique DBManager accessible globally to all testcases */

//Test DBManager class
//Lionel: this is obsolete now, we test the DBManager class via the factory
/*
TEST_GROUP(DBManagerClassTests) {
};

TEST(DBManagerClassTests, RemoveTest) {
	map<string, string> values;
	values.insert(pair<string,string>("admin-password", "testpasswordModify"));
	DBManager::GetInstance()->insertRecord("global", values);
	DBManager::GetInstance()->deleteRecord("global", values);
}

TEST(DBManagerClassTests, ModifyTest) {
	map<string, string> values;
	values.insert(pair<string,string>("admin-password", "testpasswordModify"));
	DBManager::GetInstance()->insertRecord("global", values);
	map<string, string> newValues;
	newValues.insert(pair<string, string>("switch-name", "modifiedName"));
	DBManager::GetInstance()->modifyRecord("global", values, newValues);
}

TEST(DBManagerClassTests, getAndInsertTest) {
	//Check to ensure we have an empty result when table is empty.
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global").size() == 0));
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("management-interface").size() == 0));

	//We try different parameters of the methods : with column selection, with duplicate removal, both.
	vector<string> columns;
	columns.push_back("admin-password");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns).size() == 0));
	columns.clear();
	columns.push_back("switch-name");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns).size() == 0));
	columns.clear();
	columns.push_back("admin-password");
	columns.push_back("switch-name");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns).size() == 0));
	columns.clear();
	columns.push_back("admin-password");
	columns.push_back("switch-name");
	columns.push_back("inexistant_columns");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns).size() == 0));
	columns.clear();
	columns.push_back("IP-address-type");
	columns.push_back("IP-netmask");
	columns.push_back("vlan");
	columns.push_back("remote-support-server");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("management-interface", columns).size() == 0));

	columns.clear();
	columns.push_back("admin-password");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns, true).size() == 0));
	columns.clear();
	columns.push_back("switch-name");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns, true).size() == 0));
	columns.clear();
	columns.push_back("admin-password");
	columns.push_back("switch-name");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns, true).size() == 0));
	columns.clear();
	columns.push_back("admin-password");
	columns.push_back("switch-name");
	columns.push_back("inexistant_columns");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns, true).size() == 0));
	columns.clear();
	columns.push_back("IP-address-type");
	columns.push_back("IP-netmask");
	columns.push_back("vlan");
	columns.push_back("remote-support-server");
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("management-interface", columns, true).size() == 0));

	map<string, string> values;
	values.insert(pair<string,string>("admin-password", "testpassword"));
	DBManager::GetInstance()->insertRecord("global", values);
	vector<map<string, string> > result = DBManager::GetInstance()->get("global");
	CHECK(static_cast<bool>(result.size() == 1));
	CHECK(static_cast<bool>(result.at(0)["admin-password"] == "testpassword"));
	CHECK(static_cast<bool>(result.at(0)["switch-name"] == "WiFi-SOHO"));

	values.clear();
	values.insert(pair<string,string>("admin-password", "testpassword"));
	values.insert(pair<string,string>("switch-name", "testSWname"));
	DBManager::GetInstance()->insertRecord("global", values);
	result = DBManager::GetInstance()->get("global");
	CHECK(static_cast<bool>(result.size() == 2));
	for(map<string, string>::iterator it = values.begin(); it != values.end(); it++) {
		CHECK(static_cast<bool>(result.at(1)[it->first] == it->second));
	}
	columns.clear();
	columns.push_back("switch-name");
	result = DBManager::GetInstance()->get("global", columns);
	CHECK(static_cast<bool>(result.size() == 2));
	CHECK(static_cast<bool>(result.at(1)["switch-name"] == values["switch-name"]));
	CHECK_THROWS(out_of_range, result.at(1).at("admin-password"));

	columns.clear();
	values.clear();
	values.insert(pair<string,string>("admin-password", "testpassword"));
	values.insert(pair<string,string>("switch-name", "testSWname"));
	DBManager::GetInstance()->insertRecord("global", values);
	CHECK(static_cast<bool>(DBManager::GetInstance()->get("global", columns).size() == (DBManager::GetInstance()->get("global", columns, true).size()+1)));
};//*/

/*
TEST(DBManagerClassTests, GetInstanceTest) {
	DBManager *appel1 = DBManager::GetInstance();
	DBManager *appel2 = DBManager::GetInstance();
	POINTERS_EQUAL(appel1, appel2);
};//*/

//We will test DBManager class
const char* progname;	/* The name under which we were called */
string database_url;
string database_structure = "<?xml version=\"1.0\" encoding=\"utf-8\"?><database>" \
"<table name=\"" TEST_TABLE_NAME "\">" \
	"<field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
"</table>" \
"<table name=\"double_unique\">" \
	"<field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"true\" />" \
	"<field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"true\" />" \
"</table>" \
"<table name=\"linked1\">" \
	"<field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
"</table>" \
"<table name=\"linked2\">" \
	"<field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
	"<field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" />" \
"</table>" \
"<relationship kind=\"m:n\" policy=\"link-all\" first-table=\"linked1\" second-table=\"linked2\" />" \
"</database>";

TEST_GROUP(DBManagerMethodsTests) {
};
TEST_GROUP(DBManagerInputRobustnessTests) {
};

TEST(DBManagerMethodsTests, unlinkRecordsInDatabaseTest) {
	map<string, string> vals1;
	vals1.emplace("field1", "unikval7");
	vals1.emplace("field2", "unikval8");
	vals1.emplace("field3", "unikval9");

	map<string, string> vals2;
	vals2.emplace("field1", "unikval10");
	vals2.emplace("field2", "unikval11");
	vals2.emplace("field3", "unikval12");

	string idLinked1;
	for(auto &it : global_manager->get("linked1"))
		if(it["field1"] == vals1["field1"] && it["field2"] == vals1["field2"]  && it["field3"] == vals1["field3"])
			idLinked1 = it["id"];

	string idLinked2;
	for(auto &it : global_manager->get("linked2"))
		if(it["field1"] == vals2["field1"] && it["field2"] == vals2["field2"]  && it["field3"] == vals2["field3"])
			idLinked2 = it["id"];

	global_manager->unlinkRecords("linked1", vals1, "linked2", vals2);

	map<string, string> linkingRecord;
	linkingRecord.emplace("linked1#id", idLinked1);
	linkingRecord.emplace("linked2#id", idLinked2);

	bool testOk = true;
	for(auto &it : global_manager->get("linked1_linked2"))
		if(it["linked1#id"] == linkingRecord["linked1#id"] && it["linked2#id"] == linkingRecord["linked2#id"])
			testOk = false;

	if(!testOk)
		FAIL("Issue in unlinkage of records.");
};

TEST(DBManagerMethodsTests, linkRecordsInDatabaseTest) {
	map<string, string> vals1;
	vals1.emplace("field1", "unikval7");
	vals1.emplace("field2", "unikval8");
	vals1.emplace("field3", "unikval9");

	map<string, string> vals2;
	vals2.emplace("field1", "unikval10");
	vals2.emplace("field2", "unikval11");
	vals2.emplace("field3", "unikval12");

	global_manager->linkRecords("linked1", vals1, "linked2", vals2);

	string idLinked1;
	for(auto &it : global_manager->get("linked1"))
		if(it["field1"] == vals1["field1"] && it["field2"] == vals1["field2"]  && it["field3"] == vals1["field3"])
			idLinked1 = it["id"];

	string idLinked2;
	for(auto &it : global_manager->get("linked2"))
		if(it["field1"] == vals2["field1"] && it["field2"] == vals2["field2"]  && it["field3"] == vals2["field3"])
			idLinked2 = it["id"];

	map<string, string> linkingRecord;
	linkingRecord.emplace("linked1#id", idLinked1);
	linkingRecord.emplace("linked2#id", idLinked2);

	bool testOk = false;
	for(auto &it : global_manager->get("linked1_linked2"))
		if(it["linked1#id"] == linkingRecord["linked1#id"] && it["linked2#id"] == linkingRecord["linked2#id"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in linkage of records.");
};

TEST(DBManagerMethodsTests, deleteAllRecordsInDatabaseTest) {
	vector<map<string,string>> vals;
	map<string, string> vals1;
	vals1.emplace("field1", "val1");
	vals1.emplace("field2", "val2");
	vals1.emplace("field3", "val3");
	vals.push_back(vals1);
	map<string, string> vals2;
	vals2.emplace("field1", "val4");
	vals2.emplace("field2", "val5");
	vals2.emplace("field3", "val6");
	vals.push_back(vals2);
	global_manager->insert(TEST_TABLE_NAME, vals);
	global_manager->remove(TEST_TABLE_NAME, map<string,string>());	// Remove all records
	bool testOk = true;
	for(auto &it : global_manager->get(TEST_TABLE_NAME))
		testOk = false;	/* If there is still a record, the deletion failed */

	if(!testOk)
		FAIL("Failed to delete all records in database.");
}

TEST(DBManagerMethodsTests, deleteSomeRecordsInDatabaseTest) {
	map<string, string> vals;
	vals.emplace("field1", "unikval7");
	vals.emplace("field2", "unikval8");
	vals.emplace("field3", "unikval9");

	global_manager->remove(TEST_TABLE_NAME, vals);

	bool testOk = true;
	for(auto &it : global_manager->get(TEST_TABLE_NAME))
		if(it["field1"] == vals["field1"] && it["field2"] == vals["field2"]  && it["field3"] == vals["field3"])
			testOk = false;

	if(!testOk)
		FAIL("Issue in one record deletion in database.");
};

TEST(DBManagerMethodsTests, modifyNonExistingRecordsInDatabaseTest) {
	map<string, string> vals;
	map<string, string> newVals;
	newVals.emplace("field1", "unikval7");
	newVals.emplace("field2", "unikval8");
	newVals.emplace("field3", "unikval9");

	global_manager->modify(TEST_TABLE_NAME, vals, newVals);

	bool testOk = false;
	for(auto &it : global_manager->get(TEST_TABLE_NAME))
		if(it["field1"] == newVals["field1"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in one record insertion in database.");
};

TEST(DBManagerMethodsTests, modifyRecordsInDatabaseTest) {
	map<string, string> vals;
	vals.emplace("field1", "unikval1");
	vals.emplace("field2", "unikval2");
	vals.emplace("field3", "unikval3");
	global_manager->insert(TEST_TABLE_NAME, vals);

	map<string, string> newVals;
	newVals.emplace("field1", "unikval4");
	newVals.emplace("field2", "unikval5");
	newVals.emplace("field3", "unikval6");

	global_manager->modify(TEST_TABLE_NAME, vals, newVals);

	bool testOk = false;
	for(auto &it : global_manager->get(TEST_TABLE_NAME))
		if(it["field1"] == newVals["field1"] && it["field2"] == newVals["field2"] && it["field3"] == newVals["field3"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in one record insertion in database.");
};

TEST(DBManagerMethodsTests, modifyOrInsertOnDuplicateUnique) {

	global_manager->remove("double_unique", map<string, string>());
	map<string, string> vals1;
	vals1.emplace("field1", "val1");
	vals1.emplace("field2", "unikval2");
	vals1.emplace("field3", "unikval3");
	global_manager->insert("double_unique", vals1);
	cout << global_manager->to_string();
	map<string, string> match;
	match.emplace("field2", "differentunikval2");
	match.emplace("field3", "unikval3");
	map<string, string> vals2;
	vals2.emplace("field1", "newval1");
	if (global_manager->modify("double_unique", match, vals2, true)) {
		cerr << "Error in modify(). Database is:\n" << global_manager->to_string() << endl;
		FAIL("Unicity could not be guaranteed. modify() should have been rejected.");
	}
	bool testOk = false;
	for(auto &it : global_manager->get("double_unique"))
		if(it["field1"] == vals1["field1"] && it["field2"] == vals1["field2"] && it["field3"] == vals1["field3"]) {
			if (testOk) {
				FAIL("Duplicate entry while unique fields.");
			}
			else {
				testOk = true;
			}
		}

	if(!testOk) {
		cerr << "Failure after modify(). Initial record was altered in database:\n" << global_manager->to_string() << endl;
		FAIL("Issue... initial record was altered in database.");
	}
};

TEST(DBManagerMethodsTests, modifyInsertIfNotExistsInEmptyDatabaseTest) {

	global_manager->remove(TEST_TABLE_NAME, map<string, string>()); /* Flush table */

	map<string, string> newVals;
	newVals.emplace("field1", "unikval4");
	newVals.emplace("field2", "unikval5");
	newVals.emplace("field3", "unikval6");

	map<string, string> nonExistingVals;
	nonExistingVals.emplace("field1", "unikval0");
	global_manager->modify(TEST_TABLE_NAME, nonExistingVals, newVals, true);

	bool testOk = false;
	for(auto &it : global_manager->get(TEST_TABLE_NAME))
		if(it["field1"] == newVals["field1"] && it["field2"] == newVals["field2"] && it["field3"] == newVals["field3"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in one record insertion in database.");
};

TEST(DBManagerMethodsTests, modifyInsertIfNotExistsInDatabaseTest) {

	global_manager->remove(TEST_TABLE_NAME, map<string, string>());	/* Flush table */

	map<string, string> vals;
	vals.emplace("field1", "unikval1");
	vals.emplace("field2", "unikval2");
	vals.emplace("field3", "unikval3");
	global_manager->insert(TEST_TABLE_NAME, vals);

	map<string, string> newVals;
	newVals.emplace("field1", "unikval4");
	newVals.emplace("field2", "unikval5");
	newVals.emplace("field3", "unikval6");
	map<string, string> nonExistingVals;
	nonExistingVals.emplace("field1", "unikval0");

	global_manager->modify(TEST_TABLE_NAME, nonExistingVals, newVals, true);

	unsigned int recordsOk = 0;


	for (auto &it : global_manager->get(TEST_TABLE_NAME)) {
		if (it["field1"] == vals["field1"] && it["field2"] == vals["field2"] && it["field3"] == vals["field3"])
			recordsOk++;
		if (it["field1"] == newVals["field1"] && it["field2"] == newVals["field2"] && it["field3"] == newVals["field3"])
			recordsOk++;
	}

	if (recordsOk != 2) { /* There should now be two records (we have modified a non existing one... so it has been added) */
		cerr << "Expected only 2 records o, database but instead got\n" << global_manager->to_string() << endl;
		FAIL("Issue in one record insertion in database.");
	}
};

TEST(DBManagerMethodsTests, modifyNoInsertIfNotExistsInDatabaseTest) {
	map<string, string> vals;
	vals.emplace("field1", "unikval1");
	vals.emplace("field2", "unikval2");
	vals.emplace("field3", "unikval3");
	global_manager->insert(TEST_TABLE_NAME, vals);

	map<string, string> newVals;
	newVals.emplace("field1", "unikval4");
	newVals.emplace("field2", "unikval5");
	newVals.emplace("field3", "unikval6");
	map<string, string> nonExistingVals;
	nonExistingVals.emplace("field1", "unikval0");

	if (global_manager->modify(TEST_TABLE_NAME, nonExistingVals, newVals, false)) {	/* We asked for no addition... so modify should fail */
		FAIL("Expected failure when modifying one non existing record in database.");
	}
};

TEST(DBManagerMethodsTests, insertSomeRecordsInDatabaseTest) {
	vector<map<string,string>> vals;
	map<string, string> vals1;
	vals1.emplace("field1", "val1");
	vals1.emplace("field2", "val2");
	vals1.emplace("field3", "val3");
	vals.push_back(vals1);
	map<string, string> vals2;
	vals2.emplace("field1", "val4");
	vals2.emplace("field2", "val5");
	vals2.emplace("field3", "val6");
	vals.push_back(vals2);
	global_manager->insert(TEST_TABLE_NAME, vals);
	bool testVals1Ok = false;
	bool testVals2Ok = false;
	for(auto &it : global_manager->get(TEST_TABLE_NAME)) {
		if(it["field1"] == vals1["field1"] && it["field2"] == vals1["field2"] && it["field3"] == vals1["field3"])
			testVals1Ok = true;
		if(it["field1"] == vals2["field1"] && it["field2"] == vals2["field2"] && it["field3"] == vals2["field3"])
			testVals2Ok = true;
	}

	if(!testVals1Ok || !testVals2Ok)
		FAIL("Issue in some records insertion in database.");
};

TEST(DBManagerMethodsTests, insertOneRecordInDatabaseTest) {
	map<string, string> vals;
	vals.emplace("field1", "val1");
	global_manager->insert(TEST_TABLE_NAME, vals);
	bool testOk = false;
	for(auto &it : global_manager->get(TEST_TABLE_NAME))
		if(it["field1"] == vals["field1"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in one record insertion in database.");
};


TEST(DBManagerMethodsTests, getDatabaseContentTest) {
	global_manager->remove(TEST_TABLE_NAME, map<string, string>());
	if(!global_manager->get(TEST_TABLE_NAME).empty())
		FAIL("Expected empty database.");

};


bool testStringInRecordValue(DBManager* manager, const string& value) {

	manager->remove(TEST_TABLE_NAME, map<string, string>());	/* Flush table */

	map<string, string> vals;
	vals.emplace("field1", value);
	manager->insert(TEST_TABLE_NAME, vals);
	
	bool testOk = false;
	for(auto &it : manager->get(TEST_TABLE_NAME))
		if(it["field1"] == vals["field1"])
			testOk = true;
	if(!testOk)
		FAIL("Issue in one record insertion in database.");
	
	map<string, string> modifiedVals;
	modifiedVals.emplace("field1", "zz" + value);
	manager->modify(TEST_TABLE_NAME, vals, modifiedVals);
	for(auto &it : manager->get(TEST_TABLE_NAME))
		if(it["field1"] == modifiedVals["field1"])
			testOk = true;
	if(!testOk)
		FAIL("Issue in one record modification in database.");

	manager->remove(TEST_TABLE_NAME, modifiedVals);
	if(!manager->get(TEST_TABLE_NAME).empty()) {
		cerr << "Expected empty database but instead got\n" << manager->to_string() << endl;
		FAIL("Expected empty database.");
	}
	
	return true;
}

TEST(DBManagerInputRobustnessTests, doubleQuotesInSQLValues) {
	testStringInRecordValue(global_manager, "val\"");
};

TEST(DBManagerInputRobustnessTests, singleQuotesInSQLValues) {
	testStringInRecordValue(global_manager, "val'");
};

TEST(DBManagerInputRobustnessTests, trailingBackSlashInSQLValues) {
	testStringInRecordValue(global_manager, "val\\");
};

TEST(DBManagerInputRobustnessTests, carriageReturnInSQLValues) {
	testStringInRecordValue(global_manager, "val\n\n");
};

TEST(DBManagerInputRobustnessTests, dollarInSQLValues) {
	testStringInRecordValue(global_manager, "val$ABC");
};

TEST(DBManagerInputRobustnessTests, percentInSQLValues) {
	testStringInRecordValue(global_manager, "val%");
};

TEST(DBManagerInputRobustnessTests, ampersandInSQLValues) {
	testStringInRecordValue(global_manager, "val &");
};

TEST(DBManagerInputRobustnessTests, equalityInSQLValues) {
	testStringInRecordValue(global_manager, "val == ");
};


int main(int argc, char** argv) {
	
	int rc;
	
	progname = get_progname();

	string tmp_fn = mktemp_filename(progname);
	database_url = DATABASE_SQLITE_TYPE + tmp_fn;
	cerr << "Will use temporary file \"" + tmp_fn + "\"\n";
	
	{
		/* Note: we need to create a first instance of the DBManager outside of the cpputest macros, otherwise a memory leak is detected */
		DBManager &manager = DBManagerFactory::getInstance().getDBManager(database_url, database_structure);
		global_manager = &manager;	/* Set the global variable so that the tests can all use it */
		
		rc = CommandLineTestRunner::RunAllTests(argc, argv);
	}
	remove(tmp_fn.c_str());	/* Remove the temporary database file */
	return rc;
}
