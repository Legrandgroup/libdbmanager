#include "sqltable.hpp"


SQLTable::SQLTable(const string& name) {
	this->setName(name);
}

SQLTable::SQLTable(const SQLTable &orig) {
	this->setName(orig.getName());
	vector<tuple<string, string, bool, bool> > temp = orig.getFields();
	for(vector<tuple<string, string, bool, bool> >::iterator it = temp.begin(); it != temp.end(); it++) {
		this->addField(*it);
	}
}
	
void SQLTable::setName(const string& name) {
	this->name = name;
}

void SQLTable::addField(const tuple<string, string, bool, bool>& field) {
	this->fields.push_back(field);
}

void SQLTable::removeField(const string& name) {
	for(vector<tuple<string, string, bool, bool> >::iterator it = this->fields.begin(); it != this->fields.end(); it++) {
		if(get<0>(*it) == name) 
			it = this->fields.erase(it);
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

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end(); it++) {
		if(get<0>(*it) == name)
			result = true;
	}

	return result;
}

bool SQLTable::operator==(const SQLTable& other) const {
	bool result = true;
	
	result = (result && (this->name == other.name));
	
	result = (result && (this->fields.size() == other.fields.size()));

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end() && result; it++) {
		result = (result && other.hasColumn(get<0>(*it)));
	}

	return result;
}
