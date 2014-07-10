/**
 * \file sql_table_management_interface.h
 */
#ifndef _SQL_TABLE_MANAGEMENT_INTERFACE_H_
#define _SQL_TABLE_MANAGEMENT_INTERFACE_H_

#include "../sql_creator.h"

/**
 * Declaration of the fields in table management-interface
 */
sql_table_t management_interface = {
	.name = "management-interface",
	.fields = {
			BUILD_DB_FIELD_T("IP-address-type", "autoip", TEXT, 1, 0),
			BUILD_DB_FIELD_T("IP-address", "", TEXT, 1, 0),
			BUILD_DB_FIELD_T("IP-netmask", "", TEXT, 1, 0),
			BUILD_DB_FIELD_T("default-gateway", "",TEXT, 1, 0),
			BUILD_DB_FIELD_T("dns-server", "", TEXT, 1, 0),
			BUILD_DB_FIELD_T("vlan", "", TEXT, 1, 0),
			BUILD_DB_FIELD_T("remote-support-server", "", TEXT, 1, 0)
	},
	.nbfields = 7,
	.pre_filled_records = NULL
};

#endif
