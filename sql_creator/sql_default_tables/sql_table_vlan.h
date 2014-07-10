/**
 * \file sql_table_vlan.h
 */
#ifndef _SQL_TABLE_VLAN_H_
#define _SQL_TABLE_VLAN_H_

#include "../sql_creator.h"

/**
 * Declaration of the fields in table vlan
 */
sql_table_t vlan = {
	.name = "vlan",
	.fields = {
			BUILD_DB_FIELD_T("id", "1", TEXT, 1, 0),
			BUILD_DB_FIELD_T("name", "Default", TEXT, 1, 0),
			BUILD_DB_FIELD_T("profile", "Data/Undefined", TEXT, 1, 0),
	},
	.nbfields = 3,
	.pre_filled_records = NULL
};

#endif
