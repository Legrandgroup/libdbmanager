if UNITTESTS
check_PROGRAMS = dbmanagerutests

dbmanagerutests_SOURCES= \
	unittests.cpp
	
dbmanagerutests_CPPFLAGS= @CXX11FLAGS@ @CPPUTEST_CFLAGS@ -I../src/
dbmanagerutests_LDFLAGS= @CPPUTEST_LIBS@
dbmanagerutests_LDADD = ../src/libdbmanager.la

TESTS = dbmanagerutests
endif UNITTESTS
