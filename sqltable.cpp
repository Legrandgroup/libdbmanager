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
		if(get<0>(*it) == name) {
			if(this->fields.size() == 1) {
				this->fields.clear();
				it = this->fields.end();
				it--;
			}
			else
				it = this->fields.erase(it);
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

	for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end(); it++) {
		//cout "testing equality of " << get<0>(*it) << " and " << name;
		if(get<0>(*it) == name)
			result = true;
		//cout result << endl;
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

vector<tuple<string, string, bool, bool> > SQLTable::diff(const SQLTable& table) const {
	vector<tuple<string, string, bool, bool> > differentFields;
//	if(this->name == table.getName()) {
		for(vector<tuple<string, string, bool, bool> >::const_iterator it = this->fields.begin(); it != this->fields.end(); it++) {
			//cout "Testing column: " << get<0>(*it) << "...\t";
			if(!table.hasColumn(get<0>(*it))) {
				//cout "Diff" << endl;
				differentFields.push_back(*it);
			}
			//else
			//cout "Same" << endl;
		}
		//cout "Vector size: " << differentFields.size() << endl;
//	}
	return differentFields;
}
