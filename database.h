/**
 * \file database.h
 * \brief Library for managing requests to a sqlite3 database
 */
#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <sqlite3.h>

int open_config(void);
void close_config(void);
sqlite3 *get_db_handle(void);

int get_first_param(const char *table, const char *field, char **result, size_t max_result_sz);
int table_search_records(const char *table, int no_fields_in_table, const char *key_name, const char *key_value, const char *field_name, char *results_list[], size_t max_result_sz, int max_list_entries);
int table_get_all_records(const char *table, int no_fields_in_table, char *results_list[], size_t max_result_sz, int max_list_entries);
int table_get_field_from_all_records(const char *table, const char *field_name, char *results_list[], size_t max_result_sz, int max_list_entries);

int set_param(const char *table, const char *field, char *value);
int insert_db_record(const char *table, int nb_fields, const char *fields[], char *values[]);
int modify_db_record(const char *table, int field_number, const char *key, const char *value, const char *fields[], const char *values[]);
int delete_db_record(const char *table, const char *key, const char *value);

#endif
