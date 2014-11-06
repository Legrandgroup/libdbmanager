lib_LTLIBRARIES = libdbmanager.la
libdbmanager_la_CFLAGS= @CXX11FLAGS@
libdbmanager_la_LDFLAGS=-version-info 0:0:0 @SQLITECPP_LIBS@ @TINYXML_LIBS@
libdbmanager_la_SOURCES = \
	sqlitedbmanager.cpp \
	sqlitedbmanager.hpp \
	dbmanager.hpp \
	dbfactory.cpp \
	dbfactory.hpp \
	sqltable.cpp \
	sqltable.hpp 

pkgincludedir = $(includedir)/dbmanager
pkginclude_HEADERS = \
	sqlitedbmanager.hpp \
	dbmanager.hpp \
	dbfactory.hpp \
	sqltable.hpp 

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = libdbmanager.pc
