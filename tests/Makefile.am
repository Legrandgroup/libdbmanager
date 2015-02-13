AUTOMAKE_OPTIONS = subdir-objects

if UNITTESTS
check_PROGRAMS = dbfactory_utests \
	dbmanager_utests \
	dbmanagercontainer_utests \
	sqlitedbmanager_tostring_utests
	
dbfactory_utests_SOURCES= \
	dbfactory_tests.cpp \
	common/tools.cpp
	
dbmanager_utests_SOURCES= \
	dbmanager_tests.cpp \
	common/tools.cpp

dbmanagercontainer_utests_SOURCES= \
	dbmanagercontainer_tests.cpp \
	common/tools.cpp

sqlitedbmanager_tostring_utests_SOURCES= \
	sqlitedbmanager_tostring_tests.cpp \
	common/tools.cpp

AM_CPPFLAGS= @CXX11FLAGS@ @CPPUTEST_CFLAGS@ -I../src/
AM_LDFLAGS= @CPPUTEST_LIBS@

dbfactory_utests_LDADD = ../src/libdbmanager.la

dbmanager_utests_LDADD = ../src/libdbmanager.la

dbmanagercontainer_utests_LDADD = ../src/libdbmanager.la

sqlitedbmanager_tostring_utests_LDADD = ../src/libdbmanager.la

TESTS = dbfactory_utests \
	dbmanager_utests \
	dbmanagercontainer_utests \
	sqlitedbmanager_tostring_utests

endif UNITTESTS
