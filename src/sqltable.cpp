/*
This file is part of libdbmanager
(see the file COPYING in the root of the sources for a link to the
homepage of libdbmanager)

libdbmanager is a C++ library providing methods for reading/modifying a
database using only C++ methods & objects and no SQL
Copyright (C) 2016 Legrand SA

libdbmanager is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License version 3
(dated 29 June 2007) as published by the Free Software Foundation.

libdbmanager is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libdbmanager (in the source code, it is enclosed in
the file named "lgpl-3.0.txt" in the root of the sources).
If not, see <http://www.gnu.org/licenses/>.
*/
#include "sqltable.hpp"

using namespace std;

SQLTable::SQLTable(const string& name) : name(name), fields(), referenced(false), foreignKeys() {
}

SQLTable::SQLTable(const SQLTable& orig) : name(orig.getName()), fields(orig.fields), referenced(orig.referenced), foreignKeys(orig.foreignKeys) {
}
	
void SQLTable::setName(const string& name) {
	this->name = name;
}

void SQLTable::addField(const tuple<string, string, bool, bool>& field) {
	this->fields.push_back(field);
}

void SQLTable::removeField(const string& name) {
	vector<tuple<string, string, bool, bool> >::iterator toDelete;
	for(vector<tuple<string, string, bool, bool> >::iterator it = this->fields.begin(); it != this->fields.end(); ++it) {
		if(get<0>(*it) == name) {
			toDelete = it;
		}
	}
	if(toDelete != this->fields.end())
		this->fields.erase(toDelete);
}

string SQLTable::getName() const {
	return this->name;
}

vector<tuple<string, string, bool, bool> > SQLTable::getFields() const {
	return this->fields;
}

bool SQLTable::hasColumn(const string& name) const {
	bool result = false;

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end(); ++it) {
		if(get<0>(*it) == name)
			result = true;
	}

	return result;
}

bool SQLTable::operator==(const SQLTable& other) const {
	bool result = true;
	
	result = result && (this->referenced == other.referenced);
	//cout << "Ref: " << result << endl;
	result = (result && (this->name == other.name));
	//cout << "Name: " << result << endl;

	if(!result)
		return result;

	result = (result && (this->fields.size() == other.fields.size()));
	//cout << "Size: " << result << endl;

	if(!result)
		return result;

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end() && result; ++it) {
		if(get<0>(*it) != PK_FIELD_NAME) {
			result = (result && other.hasColumn(get<0>(*it)));
			//cout << "Names: " << result << endl;
		}
	}

	return result;
}

bool SQLTable::operator!=(const SQLTable& other) const {
	return !(this->operator==(other));
}

vector<tuple<string, string, bool, bool> > SQLTable::diff(const SQLTable& table) const {
	vector<tuple<string, string, bool, bool> > differentFields;

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end(); ++it) {
		if(this->referenced) {
			if(get<0>(*it) != PK_FIELD_NAME) {
				if(!table.hasColumn(get<0>(*it))) {
					differentFields.push_back(*it);
				}
			}
		}
		else {
			if(!table.hasColumn(get<0>(*it))) {
				differentFields.push_back(*it);
			}
		}
	}

	return differentFields;
}

bool SQLTable::isReferenced() const {
	return this->referenced;
}

void SQLTable::markReferenced() {
	this->referenced = true;
}

void SQLTable::unmarkReferenced() {
	this->referenced = false;
}

map<string, pair<string , string>> SQLTable::getForeignKeys()  const {
	return this->foreignKeys;
}

bool SQLTable::markAsForeignKey(string fieldName, string referencedTableName, string referencedFieldName) {
	bool result = false;
	map<string, pair<string,string>>::iterator foreignKeysIt = this->foreignKeys.find(fieldName);
	if(foreignKeysIt == this->foreignKeys.end()) {	/* This field is not in the foreignKeys map yet */
		for(vector<tuple<string, string, bool, bool> >::iterator it = this->fields.begin(); it != this->fields.end(); ++it) {
			if(get<0>(*it) == fieldName) {
				this->foreignKeys.emplace(fieldName, pair<string,string>(referencedTableName, referencedFieldName));
				result = true;
			}
		}
	}

	return result;
}

bool SQLTable::unmarkAsForeignKey(string fieldName) {
	bool result = false;

	map<string, pair<string,string>>::iterator foreignKeysIt = this->foreignKeys.find(fieldName);
	if(foreignKeysIt != this->foreignKeys.end()) {
		this->foreignKeys.erase(foreignKeysIt);
		result = true;
	}

	return result;
}
