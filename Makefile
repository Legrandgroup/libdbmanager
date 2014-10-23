BUILDDIR = build
SRCDIR = src
INCLUDEDESTDIR = include
LIBDESTDIR = lib
OBJECTFILES = dbmanager.o 
CLEANFILES = $(OBJECTFILES) $(INCLUDEDESTDIR) $(LIBDESTDIR)
LIBNAME = libdbmanager.so.1.0

CXX = g++
CXXFLAGS = -fPIC -std=c++11 -O2 -I./inc
LDFLAGS = -shared -Wl,-soname,$(LIBNAME) -L./li -lsqlite-cpp -ldbus-c++-1 -ltinyxml

all : $(LIBNAME)

$(LIBNAME) : $(OBJECTFILES)
	mkdir $(INCLUDEDESTDIR)
	mkdir $(LIBDESTDIR)
	$(CXX) $(CXXFLAGS) $(OBJECTFILES) $(LDFALGS) -o $(LIBDESTDIR)/$(LIBNAME)
	cp -r $(SRCDIR)/*.hpp $(INCLUDEDESTDIR)

%.o : $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $< 

clean :
	rm -rf $(CLEANFILES)
