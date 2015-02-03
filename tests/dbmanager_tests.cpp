#include <CppUTest/TestHarness.h>
#include <CppUTest/CommandLineTestRunner.h>

#include "dbfactory.hpp"

using namespace std;

#define TEST_TABLE_NAME "unittests"

//Test DBManager class
//Lionel: this is obsolete now, we test the DMManager class via the factory
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

//We will test DBManagerFactory class

// Global variable to prevent false-positive memory leak on each test that require a manager.
DBManager &manager = DBManagerFactory::getInstance().getDBManager("sqlite://tmp/utests.sqlite", "<?xml version=\"1.0\" encoding=\"utf-8\"?><database><table name=\"unittests\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked1\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><table name=\"linked2\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field2\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /><field name=\"field3\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table><relationship kind=\"m:n\" policy=\"link-all\" first-table=\"linked1\" second-table=\"linked2\" /></database>");
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
	for(auto &it : manager.get("linked1"))
		if(it["field1"] == vals1["field1"] && it["field2"] == vals1["field2"]  && it["field3"] == vals1["field3"])
			idLinked1 = it["id"];

	string idLinked2;
	for(auto &it : manager.get("linked2"))
		if(it["field1"] == vals2["field1"] && it["field2"] == vals2["field2"]  && it["field3"] == vals2["field3"])
			idLinked2 = it["id"];

	manager.unlinkRecords("linked1", vals1, "linked2", vals2);

	map<string, string> linkingRecord;
	linkingRecord.emplace("linked1#id", idLinked1);
	linkingRecord.emplace("linked2#id", idLinked2);

	bool testOk = true;
	for(auto &it : manager.get("linked1_linked2"))
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

	manager.linkRecords("linked1", vals1, "linked2", vals2);

	string idLinked1;
	for(auto &it : manager.get("linked1"))
		if(it["field1"] == vals1["field1"] && it["field2"] == vals1["field2"]  && it["field3"] == vals1["field3"])
			idLinked1 = it["id"];

	string idLinked2;
	for(auto &it : manager.get("linked2"))
		if(it["field1"] == vals2["field1"] && it["field2"] == vals2["field2"]  && it["field3"] == vals2["field3"])
			idLinked2 = it["id"];

	map<string, string> linkingRecord;
	linkingRecord.emplace("linked1#id", idLinked1);
	linkingRecord.emplace("linked2#id", idLinked2);

	bool testOk = false;
	for(auto &it : manager.get("linked1_linked2"))
		if(it["linked1#id"] == linkingRecord["linked1#id"] && it["linked2#id"] == linkingRecord["linked2#id"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in linkage of records.");
};

TEST(DBManagerMethodsTests, deleteSomeRecordsInDatabaseTest) {
	map<string, string> vals;
	vals.emplace("field1", "unikval7");
	vals.emplace("field2", "unikval8");
	vals.emplace("field3", "unikval9");

	manager.remove(TEST_TABLE_NAME, vals);

	bool testOk = true;
	for(auto &it : manager.get(TEST_TABLE_NAME))
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

	manager.modify(TEST_TABLE_NAME, vals, newVals);

	bool testOk = false;
	for(auto &it : manager.get(TEST_TABLE_NAME))
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
	manager.insert(TEST_TABLE_NAME, vals);

	map<string, string> newVals;
	newVals.emplace("field1", "unikval4");
	newVals.emplace("field2", "unikval5");
	newVals.emplace("field3", "unikval6");

	manager.modify(TEST_TABLE_NAME, vals, newVals);

	bool testOk = false;
	for(auto &it : manager.get(TEST_TABLE_NAME))
		if(it["field1"] == newVals["field1"] && it["field2"] == newVals["field2"] && it["field3"] == newVals["field3"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in one record insertion in database.");
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
	manager.insert(TEST_TABLE_NAME, vals);
	bool testVals1Ok = false;
	bool testVals2Ok = false;
	for(auto &it : manager.get(TEST_TABLE_NAME)) {
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
	manager.insert(TEST_TABLE_NAME, vals);
	bool testOk = false;
	for(auto &it : manager.get(TEST_TABLE_NAME))
		if(it["field1"] == vals["field1"])
			testOk = true;

	if(!testOk)
		FAIL("Issue in one record insertion in database.");
};


TEST(DBManagerMethodsTests, getDatabaseContentTest) {
	manager.remove(TEST_TABLE_NAME, map<string, string>());
	if(!manager.get(TEST_TABLE_NAME).empty())
		FAIL("Expected empty database.");

};


bool testStringInRecordValue(DBManager& manager, const string& value) {
	map<string, string> vals;
	vals.emplace("field1", value);
	manager.insert(TEST_TABLE_NAME, vals);
	
	bool testOk = false;
	for(auto &it : manager.get(TEST_TABLE_NAME))
		if(it["field1"] == vals["field1"])
			testOk = true;
	if(!testOk)
		FAIL("Issue in one record insertion in database.");
	
	map<string, string> modifiedVals;
	modifiedVals.emplace("field1", "zz" + value);
	manager.modify(TEST_TABLE_NAME, vals, modifiedVals);
	for(auto &it : manager.get(TEST_TABLE_NAME))
		if(it["field1"] == modifiedVals["field1"])
			testOk = true;
	if(!testOk)
		FAIL("Issue in one record modification in database.");

	manager.remove(TEST_TABLE_NAME, modifiedVals);
	if(!manager.get(TEST_TABLE_NAME).empty())
		FAIL("Expected empty database.");
	
	return true;
}

TEST(DBManagerInputRobustnessTests, doubleQuotesInSQLValues) {
	testStringInRecordValue(manager, "val\"");
};

TEST(DBManagerInputRobustnessTests, singleQuotesInSQLValues) {
	testStringInRecordValue(manager, "val'");
};

TEST(DBManagerInputRobustnessTests, trailingBackSlashInSQLValues) {
	testStringInRecordValue(manager, "val\\");
};

TEST(DBManagerInputRobustnessTests, carriageReturnInSQLValues) {
	testStringInRecordValue(manager, "val\n\n");
};

TEST(DBManagerInputRobustnessTests, dollarInSQLValues) {
	testStringInRecordValue(manager, "val$ABC");
};

TEST(DBManagerInputRobustnessTests, percentInSQLValues) {
	testStringInRecordValue(manager, "val%");
};

TEST(DBManagerInputRobustnessTests, ampersandInSQLValues) {
	testStringInRecordValue(manager, "val &");
};

TEST(DBManagerInputRobustnessTests, equalityInSQLValues) {
	testStringInRecordValue(manager, "val == ");
};


int main(int argc, char** argv) {
	return CommandLineTestRunner::RunAllTests(argc, argv);
}
