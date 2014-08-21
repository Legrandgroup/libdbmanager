/**
 * \file dbconfig.hpp
 * \brief Defines of database parameters in conductor
 */

#ifndef _DBCONFIG_HPP_
#define _DBCONFIG_HPP_

/**
 * Filename holding the database
 */
#define PATH_DB "/var/local/config.sqlite"

/**
 * Maximum number of fields (columns) in a SQL table stored in our database
 */
#define DB_TABLE_MAX_FIELD_NO 50

/**
 * Maximum number of records (lines) in a SQL table stored in our database
 */
#define DB_TABLE_MAX_RECORD_NO 64

/**
 * Maximum size in bytes of a field value stored in our database
 */
#define DB_PARAM_MAX_SZ 1024

#endif //_DBCONFIG_HPP_
