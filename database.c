/**
 * \file database.c
 * \brief Functions to manage an sqlite3 database
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "database.h"
#include "../config.h"
#include "../utils.h"

/**
 * \var static sqlite3 *_db
 * \brief Pointer to the database object
 *
 * This variable is internal to this library... use get_db_handle() instead of referencing it directly
 */
static sqlite3 *_db = NULL;


/**
 * \brief Open the database
 * \return 1 on success, 0 on failure
 * 
 * This function open the database. The database is created if it does not exist
 */
int open_config(void) {

	int rc;
	close_config();
	rc = sqlite3_open(PATH_DB, &_db);
	if (rc != SQLITE_OK) {
		plog(LOG_ERR, "sqlite3_open failed. rc: %d, comments: %s\n", rc, sqlite3_errmsg(_db));
		sqlite3_close(_db);
		return 0;
	}
	return 1;
}


/**
 * \brief Close the database
 * 
 * This function close the databse by passing the pointer ti the database object to NULL if it exists
 */
void close_config(void) {

	if (_db) {
		sqlite3_close(_db);
		_db = NULL;
	}
}


/**
 * \brief Get database handle
 * \return A pointer to the database object
 * 
 * This function returns a pointer to the database object
 */
sqlite3 *get_db_handle(void) {
	return _db;
}


/**
 * \brief Get the content of the field of the first record of a table in the database
 * \param[in] table Table on which to search
 * \param[in] field Field to extract
 * \param[out] result A reference to a pointer to a text buffer where we will store the value (note: allocation for these strings is done within this function, it is up to the called to perform a free())
 * \param[in] max_result_sz The maximum size that we are allowed to allocate in the buffer pointed by \p result (or 0 for no limit)
 * \return 1 on success, 0 on failure (\p *result is then a NULL pointer)
 * 
 * Get value of a specified field in a specified table
 */
int get_first_param(const char *table, const char *field, char **result, size_t max_result_sz) {

	char *cmd_sql;
	const char *tail;
	const unsigned char *row_text;
	sqlite3_stmt *stmt;
	int rc;
	size_t result_sz;

	assert(table);
	assert(field);
	assert(result);
	
	*result = NULL;	/* No result yet */
	
	if (*table == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty table string is not allowed\n", __func__);
		return 0;
	}

	if (*field == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty field is not allowed\n", __func__);
		return 0;
	}

	/* Prepare sql command: SELECT field FROM table; */
	cmd_sql = g_strconcat("SELECT \"", field, "\" FROM \"", table, "\";", NULL);

	/* Prepare the command */
	rc = sqlite3_prepare(_db, cmd_sql, strlen(cmd_sql), &stmt, &tail); 
	g_free(cmd_sql);
	if (rc != SQLITE_OK) {
		plog(LOG_ERR, "sqlite3_prepare failed. cmd: '%s', rc: %d, comments: %s\n", cmd_sql, rc, sqlite3_errmsg(_db));
		sqlite3_finalize(stmt);
		return 0;
	}

	/* Check if a record was found */
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) {
		plog(LOG_WARNING, "sqlite3_step failed. rc: %d, comments: %s\n", rc, sqlite3_errmsg(_db));
		sqlite3_finalize(stmt);
		return 0;
	}

	/* Extract the result */
	row_text = sqlite3_column_text(stmt, 0);
	if (!row_text) {
		plog(LOG_WARNING, "%s: call to sqlite3_column_text() failed\n", __func__);
		sqlite3_finalize(stmt);
		return 0;
	}
	
	result_sz = strlen(row_text);	/* Get the size of the string to store */
	if (max_result_sz && result_sz>max_result_sz) {  /* Truncate the result if needed */
		plog(LOG_WARNING, "%s: call to sqlite3_column_text() returned a too long string... will be truncated\n", __func__);
		*result = (char *)malloc((1+max_result_sz)*sizeof(char));	/* +1 to account for the terminating '\0' */
		assert(*result);
		strncpy(*result, (char *)row_text, max_result_sz); /* Copy the field (column) value */
		(*result)[max_result_sz] = '\0';
	}
	else {	/* No need to truncate... copy as is */
		*result = (char *)malloc((1+strlen(row_text))*sizeof(char));	/* +1 to account for the terminating '\0' */
		assert(*result);
		strcpy(*result, (char *)row_text); /* Copy the field (column) value */
	}

	sqlite3_finalize(stmt);

	return 1;
}


/**
 * \brief Get all records from a table that match \p key_name = \p key_value
 * \param[in] table The name of the sqlite table in which we will search
 * \param[in] no_fields_in_table Number of fields in the table (must be set to 0 if \p field_name is specified)
 * \param[in] key_name The field name on which we will search
 * \param[in] key_value The value to match
 * \param[in] field_name String containing the name of the field to return (or NULL to return all the fields of the record)
 * \param[out] results_list A pointer to a table of text buffers where we will store the records (note: allocation for these strings is done within this function, it is up to the called to perform a free())
 * \param[in] max_result_sz The size allocated for text in each entry in buffer \p results_list (or 0 for no limit)
 * \param[in] max_list_entries The maximum number of entries (records) that we are allowed to store into the buffer \p results_list
 * \return 1 on success, 0 on failure
 * 
 * Within the \p table, dump all records for the entry that match \p key_value in the record \p key_name<br>
 * The output (stored in \p results_list) is a table of text buffer which each contain the fields of one record separated by "\n".<br>
 * e.g: results_list[record 1] = "field1\nfield2\n...fieldn\n"<br>
 *	results_list[record 2] = "field1\nfield2\n...fieldn\n"<br>
 *	...<br>
 *	results_list[record n] = "field1\nfield2\n...fieldn\n"<br>
 * Before calling this funtion, \p value must not contain only NULL elements in the first \p max_list_entries entries.
 */
int table_search_records(const char *table, int no_fields_in_table, const char *key_name, const char *key_value, const char *field_name, char *results_list[], size_t max_result_sz, int max_list_entries) {

	char *cmd_sql;
	const char *tail;
	const unsigned char *row_text[no_fields_in_table + 1];	/* row_text has to be NULL terminated in order to be used by glib */
	char *record;
	sqlite3_stmt *stmt;
	int rc, i, record_number;
	size_t result_sz;
	int dump_all_records_mode = 0;	/* Do we dump only the matching record(s) (0) or all table (1)? */
	int dump_all_fields_mode = 0;	/* Do we dump only a specific field (0) or all fields (1)? */
	
	if (!key_name || !key_value) {	/* No key, so dump all */
		dump_all_records_mode = 1;
	}
	else if (*key_name == '\0' || *key_value == '\0') {
		dump_all_records_mode = 1;
	}
	
	if (!field_name) {	/* No field name, so dump all fields */
		dump_all_fields_mode = 1;
	}
	else if (*field_name == '\0') {
		dump_all_fields_mode = 1;
	}
	
	assert(table);
	if (*table == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty table string is not allowed\n", __func__);
		return 0;
	}
	
	if (dump_all_fields_mode)
		assert(no_fields_in_table);	/* The number of fields of the table is mandatory when returning all fields */
	else
		assert(no_fields_in_table == 0);	/* There should be 0 in this argument if only one field is returned (this will give us an array of one string in row_text */
	
	assert(results_list);
	assert(max_list_entries);
	
	for (i = 0 ; i < max_list_entries ; i++) {
		assert(results_list[i] == NULL);
	}
	
	if (dump_all_records_mode) {
		if (dump_all_fields_mode)
			cmd_sql = g_strconcat("SELECT * FROM \"", table, "\";", NULL);	/* Prepare sql command: SELECT * FROM table; */
		else
			cmd_sql = g_strconcat("SELECT DISTINCT  \"", field_name, "\" FROM \"", table, "\";", NULL);	/* Prepare sql command: SELECT DISTINCT column FROM table; */
	}
	else {
		if (dump_all_fields_mode)
			cmd_sql = g_strconcat("SELECT * FROM \"", table, "\" WHERE \"", key_name, "\" = \"", key_value,  "\";", NULL);	/* Prepare sql command: SELECT * FROM table WHERE "key" = "value"; */
		else
			cmd_sql = g_strconcat("SELECT DISTINCT  \"", field_name, "\" FROM \"", table, "\" WHERE \"", key_name, "\" = \"", key_value,  "\";", NULL);	/* Prepare sql command: SELECT DISTINCT column FROM table WHERE "key" = "value"; */
	}
	
	/* Prepare the command */
	rc = sqlite3_prepare(_db, cmd_sql, strlen(cmd_sql), &stmt, &tail); 
	g_free(cmd_sql);
	if (rc != SQLITE_OK) {
		plog(LOG_ERR, "sqlite3_prepare failed. cmd: '%s', rc: %d, comments: %s\n", cmd_sql, rc, sqlite3_errmsg(_db));
		sqlite3_finalize(stmt);
		return 0;
	}
	
	/* Parse the results */
	record_number = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {	/* Process each record */
		/* If we have reached the max number of entries in results_list exit the loop and log a warning */
		if (record_number >= max_list_entries) {
			plog(LOG_WARNING, "%s: too many records in the table\n", __func__);
			break;
		}
		
		if (dump_all_fields_mode) {
			/* Contatenate all fields from a record, separating them with '\n' */
			for (i = 0 ; i < no_fields_in_table ; i++) {
				row_text[i] = sqlite3_column_text(stmt, i);
				if (!row_text[i]) {
					plog(LOG_WARNING, "%s: call to sqlite3_column_text() failed\n", __func__);
					row_text[i]="";
				}
			}
			row_text[no_fields_in_table] = NULL;	/* Terminate the list with NULL for g_strjoinv() below */
			record = g_strjoinv("\n", (gchar **)row_text);
			if (!record) {
				plog(LOG_WARNING, "%s: call to g_strjoinv() failed\n", __func__);
			}
		}
		else {	/* Get only a specific field */
			row_text[0] = sqlite3_column_text(stmt, 0);	/* Use row_text[0] to store the pointer */
			if (!row_text[0]) {
				plog(LOG_WARNING, "%s: call to sqlite3_column_text() failed\n", __func__);
				row_text[0]="";
			}
			record = g_strdup(row_text[0]);	/* Duplicate row_text to record (in our stack) */
			if (!record) {
				plog(LOG_WARNING, "%s: call to g_strdup() failed\n", __func__);
			}
		}
		if (!record) record = g_strdup("");	/* Replace NULL record with the copue of an empty string  */
		
		result_sz = strlen(record);     /* Get the size of the string to store */
		if (max_result_sz && result_sz>max_result_sz) {  /* Truncate the result if needed */
			plog(LOG_WARNING, "%s: concatenated fields form a too long string... will be truncated\n", __func__);
			record[max_result_sz] = '\0';
		}
		
		/* Store the result in results_list[] */
		results_list[record_number] = (char *)malloc((1+strlen(record))*sizeof(char));  /* +1 to account for the terminating '\0' */
		assert(results_list[record_number]);
		strcpy(results_list[record_number], record); /* Copy the result record in one entry of results_list[] */
		
		g_free(record);
		record_number++;
	}

	sqlite3_finalize(stmt);

	return 1;
}


/**
 * \brief Dump a table of the database
 * \param[in] table The name of the sqlite table to dump
 * \param[in] no_fields_in_table Number of fields in the table
 * \param[out] results_list A pointer to a table of text buffers where we will store the records (note: allocation for these strings is done within this function, it is up to the called to perform a free())
 * \param[in] max_result_sz The size allocated for text in each entry in buffer \p results_list (or 0 for no limit)
 * \param[in] max_list_entries The maximum number of entries (records) that we are allowed to store into the buffer \p results_list (if results_list is an array of n items, then \p max_list_entries should be set to n (or less))
 * \return 1 on success, 0 on failure
 * 
 * Dump all records from a given table. We will internally call table_search_records() without any key to return all records, and without any specific field to return all fields of each records
 * Before calling this funtion, \p value must not contain only NULL elements in the first \p max_list_entries entries.
 */
int table_get_all_records(const char *table, int no_fields_in_table, char *results_list[], size_t max_result_sz, int max_list_entries) {

	return table_search_records(table, no_fields_in_table, NULL, NULL, NULL, results_list, max_result_sz, max_list_entries);
}


/**
 * \brief Dump the content of a specific field from all records of a table of the database
 * \param[in] table The name of the table in which we will extract the \p field_name
 * \param[in] field_name The name of the only field we will dump into \p results_list
 * \param[out] results_list A pointer to a table of text buffers where we will store the records (note: allocation for these strings is done within this function, it is up to the called to perform a free())
 * \param[in] max_result_sz The size allocated for text in each entry in buffer \p results_list (or 0 for no limit)
 * \param[in] max_list_entries The maximum number of entries (records) that we are allowed to store into the buffer \p results_list (if results_list is an array of n items, then \p max_list_entries should be set to n (or less))
 * \return 1 on success, 0 on failure
 * 
 * Dump the content of the specified \p field for all records of the \p table<br>
 * The output is an array of strings which each contain one value.<br>
 * e.g: results_list[record 1] = "field1"<br>
 *	results_list[record 2] = "field1"<br>
 *	...<br>
 *	results_list[record n] = "field1"
 */
int table_get_field_from_all_records(const char *table, const char *field_name, char *results_list[], size_t max_result_sz, int max_list_entries) {
	
	assert(field_name);
	assert(*field_name);	/* field_name must be a non empty string */
	return table_search_records(table, 0, NULL, NULL, field_name, results_list, max_result_sz, max_list_entries);
}


/**
 * \brief Set value on a field in the database
 * \param[in] table Table on which to store
 * \param[in] field Field where we will store the new value
 * \param[in] value The new value
 * \return 1 on success, 0 on failure
 * 
 * Store \p value in the specified \p field in a specific \p table
 */
int set_param(const char *table, const char *field, char *value) {

	char *cmd_sql;
	char *err = NULL;
	int rc;

	assert(table);
	assert(field);
	assert(value);
	
	if (*table == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty table string is not allowed\n", __func__);
		return 0;
	}

	if (*field== '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty field is not allowed\n", __func__);
		return 0;
	}

	if (*value == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty value is not allowed\n", __func__);
		return 0;
	}

	/* Format the sql UPDATE command: UPDATE table SET field='value' */
	cmd_sql = g_strconcat("UPDATE \"", table, "\" SET \"", field, "\"='", sqlite3_mprintf("%q", value), "';", NULL); /* sqlite3_mprintf() is used to escape " and \ characters */

	/* Execute the update command */
	rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
	if (rc != SQLITE_OK ) {
		plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
		sqlite3_free(err);
		g_free(cmd_sql);
		return 0;
	}

	g_free(cmd_sql);
	return 1;
}


/**
 * \brief Insert a record in a database table
 * \param[in] table The name of the sqlite table in which we delete a record
 * \param[in] field_number Number of fields in the table
 * \param[in] fields The fields names to add (this array must have exactly \p field_number entries)
 * \param[in] values The new values to insert (this array must have exactly \p field_number entries)
 * \return 1 on success, 0 on failure
 */
int insert_db_record(const char *table, int field_number, const char *fields[], char *values[]) {
	
	char *cmd_sql;
	char *err = NULL;
	int i;
	int rc;

	assert(table);
	assert(field_number);
	assert(fields);
	assert(values);

	if (*table == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty table string is not allowed\n", __func__);
		return 0;
	}

	/* Format the sql INSERT command: INSERT INTO table ("champ1", "champ2", ...) VALUES ("val1", "val2", ...); */
	cmd_sql = g_strconcat("INSERT INTO \"", table, "\" (", NULL);

	for (i = 0 ; i < field_number ; i++) {
		cmd_sql = g_strconcat(cmd_sql, "\"", fields[i], "\"", NULL);
		if ( i < (field_number - 1)) {
			cmd_sql = g_strconcat(cmd_sql, ", ", NULL);
		}
	}

	cmd_sql = g_strconcat(cmd_sql, ") VALUES (", NULL);

	for (i = 0 ; i < field_number ; i++) {
		cmd_sql = g_strconcat(cmd_sql, "'", sqlite3_mprintf("%q", values[i]), "'", NULL); /* sqlite3_mprintf() is used to escape " and \ characters */
		if ( i < (field_number - 1)) {
			cmd_sql = g_strconcat(cmd_sql, ", ", NULL);
		}
	}

	cmd_sql = g_strconcat(cmd_sql, ");", NULL);

	/* Execute the insert command */
	rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
	if (rc != SQLITE_OK ) {
		plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
		sqlite3_free(err);
		g_free(cmd_sql);
		return 0;
	}

	g_free(cmd_sql);
	return 1;
}


/**
 * \brief Modify a record from a given table where the field \p key_name contains \p key_value
 * \param[in] table The name of the sqlite table in which we modify records
 * \param[in] field_number Number of fields in the table
 * \param[in] key_name The field name on which we will perform the search
 * \param[in] key_value The value to match
 * \param[in] fields The fields names to modify (this array must have exactly \p field_number entries)
 * \param[in] values The new values to insert (this array must have exactly \p field_number entries)
 * \return 1 on success, 0 on failure
 */
int modify_db_record(const char *table, int field_number, const char *key_name, const char *key_value, const char *fields[], const char *values[]) {
	
	char *cmd_sql;
	char *err = NULL;
	int rc;
	int i;
	
	assert(table);
	assert(field_number);
	assert(key_name);
	assert(key_value);
	assert(fields);
	assert(values);
	
	if (*table == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty table string is not allowed\n", __func__);
		return 0;
	}
	
	if (*key_name == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty key_name is not allowed\n", __func__);
		return 0;
	}
	
	if (*key_value == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty key_value is not allowed\n", __func__);
		return 0;
	}
	
	/* Format the sql UPDATE command: UPDATE table SET "field1" = "value1", "field2" = "value2" WHERE "key" = "value"; */
	cmd_sql =  g_strconcat("UPDATE \"", table, "\" SET ", NULL);
	
	/* As the key  is a member of table fields, we perform the loop on (field_number - 1) */
	for (i = 0 ; i < (field_number - 1); i++) {
		cmd_sql = g_strconcat(cmd_sql, "\"", fields[i], "\" = '", sqlite3_mprintf("%q", values[i]), "'", NULL); /* sqlite3_mprintf() is used to escape " and \ characters */
		if ( i < (field_number - 2)) {
			cmd_sql = g_strconcat(cmd_sql, ", ", NULL);
		}
	}
	
	cmd_sql =  g_strconcat(cmd_sql, " WHERE \"", key_name, "\" = '", sqlite3_mprintf("%q", key_value), "';", NULL); /* sqlite3_mprintf() is used to escape " and \ characters */

	/* Execute the update command */
	rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
	if (rc != SQLITE_OK ) {
		plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
		sqlite3_free(err);
		g_free(cmd_sql);
		return 0;
	}

	g_free(cmd_sql);
	return 1;
}


/**
 * \brief Delete a record from a given table the field \p key_name contains \p key_value
 * \param[in] table The name of the sqlite table in which we delete a record
 * \param[in] key_name The field name on which we will perform the search
 * \param[in] key_value The value to match
 * \return 1 on success, 0 on failure
 */
int delete_db_record(const char *table, const char *key_name, const char *key_value) {
	
	char *cmd_sql;
	char *err = NULL;
	int rc;
	
	assert(table);
	assert(key_name);
	assert(key_value);
	
	if (*table == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty table string is not allowed\n", __func__);
		return 0;
	}
	
	if (*key_name == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty key_name is not allowed\n", __func__);
		return 0;
	}
	
	if (*key_value == '\0') {		/* Empty string not allowed */
		plog(LOG_ERR, "%s: empty key_value is not allowed\n", __func__);
		return 0;
	}
	

	/* Format the sql DELETE command: DELETE FROM table WHERE "field" = "value"; */
	cmd_sql =  g_strconcat("DELETE FROM \"", table, "\" WHERE \"", key_name, "\" = '", sqlite3_mprintf("%q", key_value), "';", NULL); /* sqlite3_mprintf() is used to escape " and \ characters */
	
	/* Execute the delte command */
	rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
	if (rc != SQLITE_OK ) {
		plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
		sqlite3_free(err);
		g_free(cmd_sql);
		return 0;
	}

	g_free(cmd_sql);
	return 1;
}
