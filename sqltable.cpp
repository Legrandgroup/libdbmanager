#include "sqltable.hpp"


SQLTable::SQLTable(const string& name) {
	this->setName(name);
	this->unmarkReferenced();
}

SQLTable::SQLTable(const SQLTable &orig) {
	this->setName(orig.getName());
	vector<tuple<string, string, bool, bool> > temp = orig.getFields();
	for(vector<tuple<string, string, bool, bool> >::iterator it = temp.begin(); it != temp.end(); ++it) {
		this->addField(*it);
	}
	(orig.referenced)? this->markReferenced():this->unmarkReferenced();
}
	
void SQLTable::setName(const string& name) {
	this->name = name;
}

void SQLTable::addField(const tuple<string, string, bool, bool>& field) {
	this->fields.push_back(field);
	/*if(get<3>(field)) {
		//cout << "Adding primary key " << get<0>(field) << " to table " << this->name << endl;
		this->primaryKey.emplace(get<0>(field));
	}//*/
}

void SQLTable::removeField(const string& name) {
	for(vector<tuple<string, string, bool, bool> >::iterator it = this->fields.begin(); it != this->fields.end(); ++it) {
		if(get<0>(*it) == name) {
			if(this->fields.size() == 1) {
				this->fields.clear();
				it = this->fields.end();
				--it;
			}
			else
				it = this->fields.erase(it);

			/*
			set<string>::iterator setIt = this->primaryKey.find(get<0>(*it));
			if(setIt != this->primaryKey.end()) {
				this->primaryKey.erase(setIt);
			}/*/
		}
	}
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
	
	result = result && (this->referenced && other.referenced);

	result = (result && (this->name == other.name));

	if(!result)
		return result;
	if(other.referenced) {
		result = (result && (this->fields.size() == other.fields.size()-1));
	}
	else {
		result = (result && (this->fields.size() == other.fields.size()));
	}

	if(!result)
		return result;

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end() && result; ++it) {
		if(get<0>(*it) != PK_FIELD_NAME) {
			result = (result && other.hasColumn(get<0>(*it)));
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
		if(!table.hasColumn(get<0>(*it))) {
			differentFields.push_back(*it);
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
	if(foreignKeysIt == this->foreignKeys.end()) {
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
