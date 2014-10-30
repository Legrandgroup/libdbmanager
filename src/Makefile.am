lib_LTLIBRARIES = libdbmanager.la
libdbmanager_la_LDFLAGS=-version-info 0:0:0 @SQLITECPP_LIBS@ @TINYXML_LIBS@
libdbmanager_la_SOURCES = \
	dbconfig.hpp \
	dbmanager.cpp \
	dbmanager.hpp \
	sqltable.cpp \
	sqltable.hpp 

pkgincludedir = $(includedir)/dbmanager
pkginclude_HEADERS = \
	dbconfig.hpp \
	dbmanager.hpp \
	sqltable.hpp 

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdbmanager.pc
