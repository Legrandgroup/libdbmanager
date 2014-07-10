/**
 * \file sql_creator.h
 * \brief Definitions of data structures for the internal database
 */
#ifndef _SQL_CREATOR_H_
#define _SQL_CREATOR_H_

#include "../../config.h"

#ifdef __GNUC__
#if __GNUC_PREREQ(4,3)
#else
#error gcc version 4.3 or above is required to handle the __COUNTER__ macro used in sql_creator.h
#endif
#endif

/**
 * \def BUILD_DB_FIELD_T
 * \brief Builds a sql_field_t entry from its arguments and increase the counter of elements to get the total number of fields in a table
 *
 * Warning: use BUILD_DB_FIELD_T to build only one instance sqlite3 table... as it makes use of the special variable __COUNTER__ that is only reset to 0 at the start of a new C file
 * If you use BUILD_DB_FIELD_T in two table definitions in the same file, the second table will have a wrong nbfields count!
 */
 
#define BUILD_DB_FIELD_T(nm,d,t,nu,p)    { .name = (__COUNTER__!=-1)?nm:"", .default_value = d, .type = t, .is_not_null = nu, .is_primary_key = p }

/**
 * \enum sql_field_type_t
 * \brief sqlite3 variables datatypes
 */
typedef enum {
	TEXT,          /*!< Text field */
	NUMERIC,       /*!< Numeric field */
	INTEGER,       /*!< Integer field */
	REAL,          /*!< Floating point real field */
	NONE           /*!< Unknown */
} sql_field_type_t;

/**
 * \struct sql_field_t
 * \brief sqlite3 field description
 * \var sql_field_t::name
 * The name of the field
 * \var sql_field_t::default_value
 * The default value for this field if non-specified
 * \var sql_field_t::type
 * The SQL type as defined in sql_field_type_t
 * \var sql_field_t::is_not_null
 * Tells SQL if an empty entry is allowed
 * \var sql_field_t::is_primary_key
 * Tells SQL if this field is a primary key for the table
 */
typedef struct {
	char *name;
	char *default_value;
	sql_field_type_t type;
	int is_not_null;
	int is_primary_key;
} sql_field_t;

/**
 * \struct sql_table_t
 * \brief sqlite3 table description
 * \var sql_table_t::name
 * The name of the SQL table
 * \var sql_table_t::nbfields
 * The number of fields (the number of valid entries contained inside member array sql_table_t::fields
 * \var sql_table_t::fields
 * An array of sql_field_t fields containing all fields (columns) of this table
 * \var sql_table_t::pre_filled_records
 * An array of strings containing the default records for this table (one entry (string) per record, in which fields are separated by commas ',')
 */
typedef struct {
	char *name;
	int nbfields;
	sql_field_t fields[DB_TABLE_MAX_FIELD_NO];
	char **pre_filled_records;
} sql_table_t;

int sql_creator(void);

#endif
