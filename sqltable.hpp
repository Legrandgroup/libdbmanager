/**
 *
 * * \file sqltable.hpp
 *
 * * \brief Class that modelizes a sql table. 
 *
 * */

#ifndef _SQLTABLE_HPP_
#define _SQLTABLE_HPP_

#include <tuple>
#include <vector>
#include <string>
#include <iostream>


using namespace std;

class SQLTable {
public:
	SQLTable(const string& name);
	SQLTable(const SQLTable& orig);
	
	//Setters
	void setName(const string& name);
	void addField(const tuple<string, string, bool, bool>& field);
	void removeField(const string& name);

	//Getters
	string getName() const;
	vector<tuple<string, string, bool, bool> > getFields() const;

	//Operators
	bool operator==(const SQLTable& other) const;
	vector<tuple<string, string, bool, bool> > diff(const SQLTable& table) const;

	//Utility methods
	bool hasColumn(const string& name) const;
private:
	string name;
	vector<tuple<string, string, bool, bool> > fields;
};

#endif //_SQLTABLE_HPP_
