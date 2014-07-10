/**
 * \file sql_table_global.h
 */
#ifndef _SQL_TABLE_GLOBAL_H_
#define _SQL_TABLE_GLOBAL_H_

#include "../sql_creator.h"

/**
 * Declaration of the fields in table management-interface
 */
sql_table_t global = {
	.name = "global",
	.fields = {
			BUILD_DB_FIELD_T("admin-password", "", TEXT, 1, 0),
			BUILD_DB_FIELD_T("switch-name", "SwitchFTTO", TEXT, 1, 0),
	},
	.nbfields = 2,
	.pre_filled_records = NULL
};

#endif
