/**
 * \file sql_table_port.h
 */
#ifndef _SQL_TABLE_PORT_H_
#define _SQL_TABLE_PORT_H_

#include "../sql_creator.h"

/**
  * Declaration of the records in table port
  */
char *port_table_pre_filled_records[] = {
	"1,access,1",
	"2,access,1",
	"3,access,1",
	"4,access,1",
	"5,access,1",
	"6,access,1",
	"7,access,1",
	"8,access,1",
	"9,access,1",
	"10,access,1",
	"11,access,1",
	"12,access,1",
	NULL};

/**
 * Declaration of the fields in table port
 */
sql_table_t port = {
	.name = "port",
	.fields = {
			BUILD_DB_FIELD_T("num", "1", TEXT, 1, 0),
			BUILD_DB_FIELD_T("mode", "access", TEXT, 1, 0),
			BUILD_DB_FIELD_T("vlan", "1", TEXT, 1, 0),
	},
	.nbfields = 3,
	.pre_filled_records = port_table_pre_filled_records
};

#endif
