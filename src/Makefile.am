lib_LTLIBRARIES = libdbmanager.la
libdbmanager_la_CPPFLAGS=@CXX11FLAGS@ -Wall -Weffc++ @SQLITECPP_CFLAGS@
libdbmanager_la_LDFLAGS=@LT_VERSION_INFO@ @LT_NO_UNDEFINED@ @SQLITECPP_LIBS@ @TINYXML_LIBS@
libdbmanager_la_SOURCES = \
	dbmanagerapi.hpp \
	sqlitedbmanager.cpp \
	sqlitedbmanager.hpp \
	dbmanager.hpp \
	dbfactory.cpp \
	dbfactory.hpp \
	dbmanagercontainer.cpp \
	dbmanagercontainer.hpp \
	sqltable.cpp \
	sqltable.hpp 

pkgincludedir = $(includedir)/dbmanager
pkginclude_HEADERS = \
	dbmanagerapi.hpp \
	dbmanager.hpp \
	dbmanagercontainer.hpp \
	dbfactory.hpp

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = dbmanager.pc
