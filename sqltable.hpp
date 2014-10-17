/**
 *
 * \file sqltable.hpp
 *
 * \brief Header file that defines the class that modelizes a sql table. 
 *
 * */

#ifndef _SQLTABLE_HPP_
#define _SQLTABLE_HPP_

//STL includes
#include <tuple>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>

#define PK_FIELD_NAME "id"

using namespace std;

/**
 * \class SQLTable
 *
 * \brief Class that modelizes a SQL table.
 *
 */
class SQLTable {
public:
	/**
	 * \brief Constructor.
	 *
	 * Only constructor of the class.
	 *
	 * \param name The table name.
	 */
	SQLTable(const string& name);
	/**
	 * \brief Copy constructor.
	 *
	 *
	 * \param The object to copy.
	 */
	SQLTable(const SQLTable& orig);
	
	//Setters
	/**
	 * \brief name attribute setter
	 *
	 * \param name The new value for the name attribute.
	 */
	void setName(const string& name);
	/**
	 * \brief fields attribute setter
	 *
	 * Allows to add a field to the table.
	 * \param field A C++ STL tuple composed of 2 std::string  and 2 bool. First string is the field name, second string is the default value for the field, first bool sets the NOT NULL SQL property of the field and second bool sets the PRIMARY KEY SQL property of the field.
	 */
	void addField(const tuple<string, string, bool, bool>& field);
	/**
	 * \brief fields attribute setter
	 *
	 * Allows to remove a field from the table.
	 * \param name The name of the field to remove from the table.
	 */
	void removeField(const string& name);

	//Getters
	/**
	 * \brief name attribute getter
	 *
	 * \return string The name attribute value.
	 */
	string getName() const;
	/**
	 * \brief fields attribute getter
	 *
	 * \return vector<tuple<string, string, bool, bool> > The fields attribute value.
	 */
	vector<tuple<string, string, bool, bool> > getFields() const;

	//Operators
	/**
	 * \brief Equality testing operator.
	 *
	 * Tests if 2 SQLTable objects are equals. Currently, 2 SQLTable objects are equals if they have the same name and the same fields name (regardless of other fields property).
	 * \return bool The result of the test.
	 */
	bool operator==(const SQLTable& other) const;
	/**
	 * \brief Inequality testing operator.
	 *
	 * Tests if 2 SQLTable objects are not equals. Currently, 2 SQLTable objects are equals if they have the same name and the same fields name (regardless of other fields property).
	 * \return bool The result of the test.
	 */
	bool operator!=(const SQLTable& other) const;
	/**
	 * \brief Get differences between 2 SQLTable objects fields.
	 * 
	 * This methods allows to find the fields that are differents between 2 SQLTable objects. It only checks if the table in parameter has all fields of the SQLTable object that executes this method. 
	 *
	 * Example of result: 
	 * We have 2 objects A and B. 
	 *
	 * A has 2 fields : '1' & '2'. 
	 *
	 * B has 3 fields '1' & '2' & '3'.
	 *
	 * Calling A.diff(B) will return no field because B has all A's fields.
	 *
	 * Calling B.diff(A) will return field '3' because A doesn't have it.
	 *
	 * \param table The SQLTable in which this object fields presence will be checked.
	 * \return vector<tuple<string, string, bool, bool> > The fields of this objet that the table parameter doesn't have.
	 */
	vector<tuple<string, string, bool, bool> > diff(const SQLTable& table) const;

	//Utility methods
	/**
	 * \brief A method to know if a table has a specific field.
	 *
	 * \param name The name of field to check presence.
	 * \return bool The result of the check.
	 */
	bool hasColumn(const string& name) const;
	bool isReferenced() const;
	void markReferenced();
	void unmarkReferenced();
	map<string, pair<string , string>> getForeignKeys() const;
	bool markAsForeignKey(string fieldName, string referencedTableName, string referencedFieldName);
	bool unmarkAsForeignKey(string fieldName);
private:
	string name;										/*!< The name of the table.*/
	vector<tuple<string, string, bool, bool> > fields;	/*!< The fields of the table. A field is a C++ STL tuple composed of 2 std::string and 2 bool. First string is the field name, second string is the default value for the field, first bool sets the NOT NULL SQL property of the field and second bool sets the UNIQUE SQL property of the field. */
	bool referenced;
	map<string, pair<string , string>> foreignKeys;
};

#endif //_SQLTABLE_HPP_
