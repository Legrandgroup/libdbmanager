if UNITTESTS
check_PROGRAMS = dbmanagerfactory_utests \
	dbmanager_utests

#	dbmanagercontainer_utests
	
dbmanagerfactory_utests_SOURCES= \
	dbmanagerfactory_tests.cpp
	
dbmanager_utests_SOURCES= \
	dbmanager_tests.cpp

dbmanagercontainer_utests_SOURCES= \
	dbmanagercontainer_tests.cpp

dbmanagerfactory_utests_CPPFLAGS= @CXX11FLAGS@ @CPPUTEST_CFLAGS@ -I../src/
dbmanagerfactory_utests_LDFLAGS= @CPPUTEST_LIBS@
dbmanagerfactory_utests_LDADD = ../src/libdbmanager.la

dbmanager_utests_CPPFLAGS= @CXX11FLAGS@ @CPPUTEST_CFLAGS@ -I../src/
dbmanager_utests_LDFLAGS= @CPPUTEST_LIBS@
dbmanager_utests_LDADD = ../src/libdbmanager.la

dbmanagercontainer_utests_CPPFLAGS= @CXX11FLAGS@ @CPPUTEST_CFLAGS@ -I../src/
dbmanagercontainer_utests_LDFLAGS= @CPPUTEST_LIBS@
dbmanagercontainer_utests_LDADD = ../src/libdbmanager.la

TESTS = dbmanagerfactory_utests \
	dbmanager_utests
	
#	dbmanagercontainer_utests

endif UNITTESTS
