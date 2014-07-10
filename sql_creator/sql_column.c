/**
 * \file sql_column.c
 * \brief Create a field with default values if it does not exist in a database table
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include <glib.h>

#include "sql_column.h"
#include "../database.h"
#include "../../utils.h"

/**
 * \fn static int sql_add_column(sql_table_t *table, char *column_name)
 * \brief Create a column with default parameters
 * \param table The table where to create the column
 * \param column_name The name of the column to add
 * \return 1 on success, 0 on failure
 */
static int sql_add_column(sql_table_t *table, char *column_name) {
	
	int rc;
	char *cmd_sql;
	char *err = NULL;
	int i;
	sqlite3 *_db;
	
	assert(table);
	assert(column_name);
	_db = get_db_handle();

	cmd_sql = g_strconcat("ALTER TABLE '", table->name, "' ADD COLUMN \"", NULL);

	for ( i = 0 ; i < table->nbfields ; i++ ) {
		if (!strcmp(table->fields[i].name, column_name)) {
			cmd_sql = g_strconcat(cmd_sql, table->fields[i].name, "\" ", NULL);
			switch(table->fields[i].type) {
				case TEXT:
					cmd_sql = g_strconcat(cmd_sql, "TEXT ", NULL);
					break;
				case NUMERIC:
					cmd_sql = g_strconcat(cmd_sql, "NUMERIC ", NULL);
					break;
				case INTEGER:
					cmd_sql = g_strconcat(cmd_sql, "INTEGER ", NULL);
					break;
				case REAL:
					cmd_sql = g_strconcat(cmd_sql, "REAL ", NULL);
					break;
				case NONE:
					cmd_sql = g_strconcat(cmd_sql, "NONE ", NULL);
					break;
			}
			if (table->fields[i].is_not_null != 0) {
				cmd_sql = g_strconcat(cmd_sql, "NOT NULL", NULL);
			}
			if (table->fields[i].is_primary_key != 0) {
				cmd_sql = g_strconcat(cmd_sql, " PRIMARY KEY", NULL);
			}
			cmd_sql = g_strconcat(cmd_sql, " DEFAULT ('", table->fields[i].default_value, "');", NULL);
			break;
		}
	}

	rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
	g_free(cmd_sql);
	if (rc != SQLITE_OK ) {
		plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
		sqlite3_free(err);
		return 0;
	}
	
	return 1;
	
}

/**
 * \fn int sql_column_exist(sql_table_t *table)
 * \brief Determine if all column frome a table exist
 * \param table Table to analyze
 * \return 1 on success, 0 on failure
 *
 * Analyse a table. If a column does not exist, it create it with default parameters.
 */
int sql_column_exist(sql_table_t *table) {
	
	int rc;
	char *cmd_sql;
	sqlite3_stmt *stmt;
	const char *tail;
	int i, j;
	int is_present = 0;
	sqlite3 *_db;
	
	assert(table);
	_db = get_db_handle();
	
	cmd_sql = g_strconcat("SELECT * FROM '", table->name, "';", NULL);
	
	rc = sqlite3_prepare(_db, cmd_sql, strlen(cmd_sql), &stmt, &tail);
	g_free(cmd_sql);
	if (rc != SQLITE_OK) {
		plog(LOG_ERR, "sqlite3_prepare failed. cmd:%s rc:%d\n", cmd_sql, rc);
		sqlite3_finalize(stmt);
		return 0;
	}
	
	for ( i = 0 ; i < table->nbfields; i++) {
		for (j = 0 ; j <  sqlite3_column_count(stmt); j++) {
			if (!strcmp(sqlite3_column_name(stmt, j), table->fields[i].name)) {
				is_present++;
				continue;
			}
		}
		if (is_present == 0) {
#ifdef DEBUG
			plog(LOG_DEBUG, "%s: will add column %s in table %s\n", __func__, table->fields[i].name, table->name);
#endif
			if(! sql_add_column(table, table->fields[i].name)) {
				plog(LOG_ERR, "%s: sql_add_column failed with param %s\n", __func__, table->fields[i].name);
				return 0;
			}
			
		}
		is_present = 0;
	}

	sqlite3_finalize(stmt);
	
	return 1;
}
