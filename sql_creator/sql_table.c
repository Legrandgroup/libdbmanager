/**
 * \file sql_table.c
 * \brief Create a table if it does not exist in the database
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include <glib.h>

#include "sql_table.h"
#include "../database.h"
#include "../../utils.h"

/**
 * \brief Create a table with default values in the database
 * \param table Table to create
 * \return 1 on success, 0 on failure
 */
static int sql_add_table(sql_table_t *table) {
	
	int rc;
	int i, j;
	char *cmd_sql;
	char *err = NULL;
	char *sql_formatted_records_list = NULL;
	gchar** pre_filled_records_list = NULL;
	sqlite3 *_db;
	
	assert(table);
	_db = get_db_handle();
	
	cmd_sql = g_strconcat("CREATE TABLE \"", table->name, "\" (", NULL);

	for ( i = 0 ; i < table->nbfields ; i++ ) {
		if (i != 0) {
			cmd_sql = g_strconcat(cmd_sql, ", ", NULL);
		}
		cmd_sql = g_strconcat(cmd_sql, "\"", table->fields[i].name, "\"", " " , NULL);
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
		cmd_sql = g_strconcat(cmd_sql, " DEFAULT ('", table->fields[i].default_value, "')", NULL);
	}

	cmd_sql = g_strconcat(cmd_sql, ");", NULL);
	rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
	if (rc != SQLITE_OK ) {
		plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
		sqlite3_free(err);
		g_free(cmd_sql);
		return 0;
	}
	

	/* Populate table with default values */
	if (table->pre_filled_records == NULL) {	/* No default records to add into the table */
		cmd_sql = g_strconcat("INSERT INTO \"", table->name, "\" DEFAULT VALUES;", NULL);
		rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
		if (rc != SQLITE_OK ) {
			plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
			sqlite3_free(err);
			g_free(cmd_sql);
			return 0;
		}
	}
	else { /* There are default records that we will use to fill-in the table with */
		i=0;
		while(table->pre_filled_records[i]) {
			cmd_sql = g_strconcat("INSERT INTO \"", table->name, "\" (", NULL);
			/* Get fields */
			for ( j = 0 ; j < table->nbfields ; j++ ) {
				if (j != 0) {
					cmd_sql = g_strconcat(cmd_sql, ", ", NULL);
				}
				cmd_sql = g_strconcat(cmd_sql, "\"", table->fields[j].name, "\"", NULL);
			}
			/* Get values*/
			pre_filled_records_list = g_strsplit(table->pre_filled_records[i], ",", (table->nbfields+1));	/* Pre-filled records are comma (',') separated values */
			sql_formatted_records_list = g_strjoinv("\",\"", pre_filled_records_list);	/* This string contains the records value in a SQL syntax */
			g_strfreev(pre_filled_records_list);
			cmd_sql = g_strconcat(cmd_sql, ") VALUES (\"", sql_formatted_records_list, "\");", NULL);
			g_free(sql_formatted_records_list);
			rc = sqlite3_exec(_db, cmd_sql, NULL, 0, &err);
			if (rc != SQLITE_OK ) {
				plog(LOG_ERR, "%s: sqlite3_exec failed with command %s. Error message is %s\n", __func__, cmd_sql, err);
				sqlite3_free(err);
				g_free(cmd_sql);
				return 0;
			}
			i++;
		}
	}

	g_free(cmd_sql);

	return 1;
}

/**
 * \brief Check if a specified table exists in the database
 * \param table Table to verify
 * \return 1 on success, 0 on failure
 *
 * Analyse a database. If a table does not exist, create it with default parameters.
 */
int sql_table_exists(sql_table_t *table) {
	
	int rc;
	char *cmd_sql;
	sqlite3_stmt *stmt;
	const char *tail;
	sqlite3 *_db;
	
	assert(table);
	_db = get_db_handle();
	
	cmd_sql = g_strconcat("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='", table->name, "';", NULL);
	rc = sqlite3_prepare(_db, cmd_sql, strlen(cmd_sql), &stmt, &tail);
	g_free(cmd_sql);
	if(rc != SQLITE_OK) {
		plog(LOG_ERR, "sqlite3_prepare failed. cmd:%s rc:%d\n", cmd_sql, rc);
		sqlite3_finalize(stmt);
		return 0;
	}
	
	if (sqlite3_step(stmt) == SQLITE_ROW ) {
		rc = sqlite3_column_int(stmt, 0);
	}
	
	sqlite3_finalize(stmt);
	
	if (rc == 0) {
#ifdef DEBUG
		plog(LOG_DEBUG, "%s: will add table %s\n", __func__, table->name);
#endif
		if (! sql_add_table(table)) {
			plog(LOG_ERR, "%s: create_table failed with param %s\n", __func__, table->name);
			return 0;
		}
	}

	return 1;
}
