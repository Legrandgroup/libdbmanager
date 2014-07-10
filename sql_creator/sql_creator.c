/**
 * \file sql_creator.c
 * \brief Check the database for coherency and create all missing fields/tables
 *
 * If a table doesn't exist, it will create it with default values.
 * If a field in a table doesn't exist, it create with its default value.
 * This code allows to generate a default configuration "on the fly", for
 * example after a full reset. It also allows to fix databases after an upgrade
 * from a previous firmware version (if new fields or tables have been added
 * in the meantime)
 */
#include <stdio.h>
#include <stdlib.h>

#include "sql_table.h"
#include "sql_column.h"

#include "sql_default_tables/sql_table_global.h"
#include "sql_default_tables/sql_table_management_interface.h"
#include "sql_default_tables/sql_table_vlan.h"
#include "sql_default_tables/sql_table_port.h"

/**
 * \fn int sql_creator (void)
 * \brief Check database. If a table doesn't exist, it create it with default values. If a field in a table doesn't exist, it will create it with a default value
 * \return 1 on success, 0 on failure
 */
int sql_creator (void) {
	
	int i;

	sql_table_t table_list[] = {
		global,
		management_interface,
		vlan,
		port,
	};

	for ( i = 0 ; i< sizeof(table_list)/sizeof(sql_table_t) ; i++) {

		if (! sql_table_exists(&table_list[i])) {
			return 0;
		}

		if (! sql_column_exist(&table_list[i])) {
			return 0;
		}
	}

	return 1;
}
