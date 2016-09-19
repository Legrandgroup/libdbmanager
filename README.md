libdbmanager
============

## Overview

libdbmanager is a C++ framework to access an underlying database without have to write any line of SQL syntax (which is prone to error and SQL injections).

This library is architectured to support any type of underlying database, although today, only sqlite3 is supported.

The libdbmanager library depends on the following external libraries to be executed (and compiled):

* [libsqlite3](http://www.sqlite.org/) ([Debian package(s)](https://packages.debian.org/search?keywords=libsqlite3-dev&searchon=names&exact=1&suite=all&section=all))
* [libtinyxml](http://code.google.com/p/libtinyxml/) ([Debian package(s)](https://packages.debian.org/search?keywords=libtinyxml-dev&searchon=names&exact=1&suite=all&section=all))
* [libsqlitecpp] (library that requires patching and compilation from sources)

These dependencies will be checked by the ./configure script (from the GNU autotools, see below)

## Compilation and installation

### Windows

For Windows platforms, the library can be built as a native C++ library (using MinGW for example).

Once compiled in a dll, the dependencies for libdbmanager are:
| libgcc_s_dw2-1.dll  | mingw64 gcc library
| libgcc_s_sjlj-1.dll | mingw64 threading and mutex support
| libwinpthread-1.dll | mingw64 windows threading adaptation layer
| libstdc++-6.dll     | mingw64 stdc++ library
| zlib1.dll           | zlib
| libsqlite3-0.dll    | libsqlite3 from sqlite-autoconf-3080801
| libsqlitecpp-0.dll  | libsqlitecpp 0.7.0
| libtinyxml-0.dll    | tinyxml 2.6.2

### Linux

This library uses autotools to compile and will result in a `libdbmanager.so` shared object. It is currently only tested under a Linux environment  but porting to any Posix-like machine should be easy.

This library make an extensive use of C++11 extensions, so a pure C++99 compiler will fail to compile (although this is achievable using libboost). The configure script will check for this compiler compatibility.

First, you can download the source code by using git:
```
git clone https://github.com/Legrandgroup/libdbmanager.git
```

Then, move inside the cloned repository:
```
cd libdbmanager
```

Note:
If you want to compile a tagged version of the library, you can list existing tags with
```
git tag
```

And select you can checkout a specific tag with
```
git checkout <tag_name>
```

Now you can build the project using the GNU autotools:
```
./autogen.sh
./configure
make all install
```

Note:
You can add some option to this configure script:
* `--enable-unittests`: will include the source code for building unit tests with the make check command
* `--enable-extended-documentation`: will generate an extended Doxygen documentation that helps developer to maintain the library core code.
* `--enable-debug`: will use the -DDEBUG define while compiling to generate additionnal debug output code (do not use this on production releases).
* `--with-pkgconfigdir=<path>`: will use this specific path to output the package config (.pc) file generated for libdbmanager.

If you need to generate the doxygen documentation, you just have to issue:
```
make doxygen-doc
```

To run unit tests, type:
```
make check
```

## Library usage from the user perspective

This section contains all the information you need to know in order to use the shared library in your code. If you need to modify the library internal mechanisms, you will need to [know more about the internal structure of libdbmanager by reading further](#Library internal architecture).

The library usage is based on 2 mechanisms: there is a container that allows to manipulate a database manager instance and the database manager itself.

The database itself is described by two elements:

* The type (protocol) and location (path) of the database, specified as a URL
* A description of the database structure (tables, records and fields) provided as an XML description file

A library user just needs to know:

* that the database type is specified to the factory in the protocol-part of the URL (we are just going to explain this a bit more in depth [below](#DatabaselocationURL)),
* that the database location is specified using the path-part of the URL (we are just going to explain this a bit more in depth [below](#DatabaselocationURL)),
* how to use DBManagerContainer objects and the DBManager interface (more on this later on...)

### Database location URL

If you just have 4 lines to read in this chapter:

A URL follows the template:
```
proto://path/to/db
```

For example, for a sqlite3 database located in the file /tmp/db.sqlite, we will use the following URL:
```
sqlite:///tmp/db.sqlite
```

Database location URLs are a way to describe as a string both the type (protocol) and location (path) of the database.

Only SQLite is implemented for now, but for database types that are not handled yet, the additional URLs are expected as below:
| SQLite    | sqlite://full_path_to_file |
| MySQL     | mysql://url:port           |
| Oracle    | oracle://url:port          |
| SQLServer | sqlserver://url:port       |

### Database structure XML description

The second argument to `getDBManager()` can be either:

* a PATH to an XML file containing the database architecture
* or direclty a string containing this XML code.

Whichever way the XML content is provided to libdbmanager, this XML content describes the tables the database should have, default records for tables and optional links between tables.

#### XML description format

The database configuration file content must comply with the following format:
```xml
<database>
    <table name="...">
        <field name="..." default-value="..." is-not-null="..." is-unique="..." />
        <field name="..." default-value="..." is-not-null="..." is-unique="..." />
        <default-records>
            <record>
                <field name="..." value="..." />
                <field name="..." value="..." />
            </record>
            <record>
                <field name="..." value="..." />
                <field name="..." value="..." />
            </record>
        </default-records>
    </table>
    <table name="...">
        <field name="..." default-value="..." is-not-null="..." is-unique="..." />
        <field name="..." default-value="..." is-not-null="..." is-unique="..." />
        <default-records>
            <record>
                <field name="..." value="..." />
                <field name="..." value="..." />
            </record>
            <record>
                <field name="..." value="..." />
                <field name="..." value="..." />
            </record>
        </default-records>
    </table>
    <!-- kind possible value : m:n -->
    <!-- policy possible value : none, link-all -->
    <relationship kind="..." policy="..." first-table="..." second-table="..." />
    <relationship kind="..." policy="..." first-table="..." second-table="..." />
</database>
```

This file describes how the database should be architectured.

Warning: If the second parameter is the content of the file, it should NOT contain any carriage returns (CR, `'\n'`). If carriage returns are present, the tinyxml library will fail to parse it.

Therefore, the library checks the presence of specified tables at the very first instanciation of a DBManager for this database, and add the required tables if they are missing. If after this pass, there are tables in the database that are not specified in the file, they will be dropped. If a table should have default records and is empty in the database, those default records will be inserted.

This is a very important point, libdbmanager will modify you database (in an possibly irreversible way) to match the XML architecture you provide, so you have to be very careful about this XML description.

A more advanced use of the XML architecture is to create a relationship between 2 tables.

In order to do this you need to specify a kind of relationship (1:1, 1:n, m:n) and a policy (none, link-all) plus 2 tables names. Of course thoses tables should exist in the database.

The policy is used to populate automatically the linking table (the third table that links the two tables in the relationship). For this policy, you can either:

* leave it empty (policy="none")
* or create a realtionship between each record of the 2 tables (policy="link-all")

Warning: Currently, only m:n relationships are handled by the library.

Now, let's go back to the 2 basic object types explained above:

* [The container that allows to manipulate a database manager instance](#DBManagerContainer usage) (class `DBManagerContainer`, described in [dbmanagercontainer.hpp](src/dbmanagercontainer.hpp))
* [The database manager itself](#DBManager usage) (class `DBManager`, described in [dbmanager.hpp](src/dbmanager.hpp))

When you use this library, you will just need to know:

* that the database type is specified to the factory in the protocol-part of the URL,
* that the database location is specified using the path-part of the URL,
* how to use the DBManager interface.

A URL follows the template:
```
proto://path/to/db
```

For example, for a sqlite3 database located in the file `/tmp/db.sqlite`, we will use the following URL:
```
sqlite:///tmp/db.sqlite
```

### DBManagerContainer usage

libdbmanager internally allocate only the necessary `DBManager` instances to handle a database.

In order to automatically handle the life cycle of the `DBManager` instances, theses instances are packaged into `DBManagerContainer` objects.

When a `DBManagerContainer` object is created, it:

* allocates a `DBManager` instance (if needed), based on a the database location URL and database structure XML provided in its constructor
* stores the reference to this `DBManager`

When the `DBManagerContainer` goes out of scope (goes through destruction), it will free the reference it holds (and the `DBManager` object may be deallocated if no other container uses it anymore).

The reference count of `DBManager` usage is done inside the [internal factory object](#Factory usage) but you don't need to worry about it, it will be done for you by libdbmanager.

In order to manipulate the DBManager object held inside a DBManagerContainer, just use `DBManagerContainer::getDBManager()`

In order to use libdbmanager, you will only need to be able to instanciate DBManagerContainer object. Add the following line in your source code to do so:
```c
#include <dbmanagercontainer.hpp>
```

In order to get a handle on a DBManager, just create an instance of DBManagerContainer:
```c
DBManagerContainer dbmc(database_url);
```

Where _database_url_ is a string describing the database location URL.

The second argument of DBManagerContainer's constructor is optional. It contains the database structure XML description.

This second argument can be either:

* a PATH to an XML file containing the database architecture
* or direclty a string containing this XML code.

If the second argument (database structure) is not provided, no migration from a previous (and potentially different) database structure will be performed. You will also not be sure that the database tables and fields are really what you expect (you may actually run on a database created by a previous firmware/software).

Warning: If the second parameter is the content of the file, it should NOT contain any carriage returns (CR, `'\n'`). If carriage returns are present, the tinyxml library will fail to parse it.

A typical scenario is to create the very first `DBManagerContainer` instance in your main program, and keep it in scope until the termination of your program. For this instance, you will provide both the URL and XML description (so that the migration from a potential previous database occurs immediately before any access to the database).

While this initial instance is alive, you will then create (in sub functions) more `DBManagerContainer` instances with only the database URL (always matching the one used initially) but no XML description. This will give you a new reference on the unique `DBManager` object handling your database.
```c
#include <dbmanagercontainer.hpp>
#include <iostream>
#include <std>
#include <map>
// Other includes here
 
using namespace std;
 
string database_url = "sqlite:///tmp/config.sqlite";
string database_structure = "<database><table name=\"global\"><field name=\"name\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table></database>";    // This is a sample database XML structure
 
string get_global_name() {
    vector<string> columns;
    columns.push_back("name");
    vector<map<string, string> > result = DBManagerContainer(database_url).getDBManager().get("global", columns, true);
    string name = "";
    if(!result.empty()) {
        return result.at(0)["name"];
    }
    else {
        return "";
    }
}
 
int main(int argc, char *argv[]) {
    DBManagerContainer dbmc(database_url, database_structure);  // Get a very first instance and perform the migration if needed. This instance will make sure DBManager hanlder exists until it goes out of scope (at program termination)
    cout << get_global_name() << endl;
}
```
 
###DBManager usage

libdbmanager provides a DBManager interface. This interface is used to manipulate the database (whatever undelying specific type of database (sqlite etc...) is in use).

The DBManager interface is described by the class DBManager in file [dbmanager.hpp](src/dbmanager.hpp)

For sqlite-type database, the concrete class that implements this interface is `SQLiteDBManager` in the file [sqlitedbmanager.hpp](src/sqlitedbmanager.hpp)

The DBManager interface provide C++ methods to modify the content of database tables. These methods allow to:

* get records from a table in the database,
* insert records in the database,
* modify some existing record in the database (if the record does not exist, it is inserted),
* remove some existing record in the database,
* link 2 records of 2 tables linked by a m:n relationship,
* unlink 2 records of 2 tables linked by a m:n relationship,
* get all records in alls tables linked to a record of a table in the case of m:n relationship.

The data type used by this interface are only maps, vectors and strings of the C++ STL.

Example of code where we read all records of a table in the database (this table exists in the database):
```c
#include <dbmanagercontainer.hpp>
#include <iostream>
#include <std>
#include <map>
// Other includes here
 
using namespace std;
 
string database_url = "sqlite:///tmp/config.sqlite";
string database_structure = "<database><table name=\"table_example\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table></database>";
 
int main(int argc, char *argv[]) {
    DBManagerContainer dbmc(database_url, database_structure);
    for(auto &it : dbmc.getDBManager().get("table_example")) {
        cout << "Record : " << endl; 
        for(auto &it2 : it)    
            cout << it2.first << ": " << it2.second <<  endl;;
    }
    return 0;
}
```
 
### Library internal architecture

Internally, DBManagerContainers are using a there is a factory that allows to obtain a database manager instance.
#### Factory usage

The first mechanism is the factory. It provides an instance of the DBManager interface according to the requested database type. The following table indicates which databases kinds are handled.
| SQLite    | Supported         |
| MySQL	    | Not yet supported |
| Oracle    | Not yet supported |
| SQLServer | Not yet supported |

The factory is implemented by the class DBFactory in file [dbfactory.hpp](src/dbfactory.hpp)

##### `DBFactory::getInstance()`

The DBFactory class implements the singleton design pattern, which ensure there is only one instance of a DBFactory class in the whole program.

To get this unique instance of the factory, just use the getInstance() class method of the:
```
DBFactory::getInstance()
```

This DBFactory instance will then be able to retreive for you an instance of the required DBManager implementation (SQLite, MySQL...). You will have to use the getDBManager() method.

##### `DBFactory::getDBManager()`

```
DBFactory::getInstance().getDBManager(string, string)
```

`getDBManager()` has 2 arguments: an URL to the database and a description of the database architecture (configuration).

The first argument (database location URL) specifies the database type and the location of the database (see [above](#Database location URL) for the specifications of location URL)

The second argument to getDBManager() can be either:

* a PATH to an XML file containing the database architecture
* or direclty a string containing this XML code.

This Database structure XML notation [is detailed above](#Database structure XML description).

If the content of the XML database architecture file is left empty, no check will be performed on the status of the database, no migration will occur (but the database will not be flushed either).

Warning: If the second parameter is the content of the file, it should NOT contain any carriage returns (CR, `'\n'`). If carriage returns are present, the tinyxml library will fail to parse it.

A call to `getDBManager()` will return a reference to a DBManager instance matching the type of database requested (internally this will be a instance of SQLiteDBManager for an sqlite database)

The common usage of the factory is the following:
```c
string url_to_database = "{some URL}";
string url_to_config_file_or_config_file_content("{some configuration file url or content}");
DBFactory::getInstance().getDBManager(url_to_database, url_to_config_file_or_config_file_content); //We perform the check on the database thanks to the configuration file that is not empty.
 
DBFactory::getInstance().getDBManager(url_to_database).aMethod(); //We get the same manager even if we do not specify a configuration file: it is empty so no check is performed on the database. aMethod is not an existing method of any class of the library but it has to be read as a placeholder for any method of the library.
 
//Do something on the database with the manager
 
DBFactory::getInstance().freeDBManager(url_to_database); //We notify to the factory we don't need this manager anymore. It is optional but it is always a good idea to notify the factory when we do not need a manager anymore.
Database migration procedure
```

The `DBManager::checkDefaultTables()` method parses the XML database configuration file and modifies the database accordingly.

It will therefore add and drop tables, or alter tables to add or drop columns. It also create or destroy relationships between tables according to the XML database architecture.

There are 2 ways to proceed a migration:

* you can provide the database configuration file at the very first time you are getting a DBManager instance,
* you can set the DBManager database XML configuration file and explicitely call `DBManager::checkDefaultTables()`.

The first option automatically sets the database architecture (from the XML description) of the manager and then calls `DBManager::checkDefaultTables()` but this process only takes place at the first instanciation of the database manager.

Example using the first option:
```c
string dbUrl = "sqlite://tmp/test.sqlite";
string dbConfiguration = "<database><table name=\"table_example\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table></database>";
DBFactory::getInstance().getDBManager(dbUrl , dbConfiguration); //If it is the first time we get hte manager, it is instanciated and therefore the manager proceeds the migration.
```

The second option gives you more control on the moment when the migration is performed. You can obtain a reference to the database manager without giving any database configuration file, you can perform your own migration operations (for example on database content)

Once your own pre-migration operations are performed, you can set the manager's configuration file and then call the `DBManager::checkDefaultTables()` (this will request libdbmanager to migrate the structure of the database).

Example using the second option:
```c
string dbUrl = "sqlite://tmp/test.sqlite";
string dbConfiguration = "<database><table name=\"table_example\"><field name=\"field1\" default-value=\"\" is-not-null=\"true\" is-unique=\"false\" /></table></database>";
DBManager& manager = DBFactory::getInstance().getDBManager(dbUrl); //We get the manager
 
// Proceed pre-migration operations
 
manager.setDatabaseConfigurationFile(dbConfiguration);
manager.checkDefaultTables();
```

### Allocation slots

In order to keep records of each allocated DBManager objects, the DBFactory class has an internal dictionnary (a C++ map, in its attribute named `DBFactory::managersStore`).

This map's key is a [database location URL string](#Database location URL). The value for this key is a DBManagerAllocationSlot instance.

Using this mecanism, each allocated DBManager can be easily added/removed from DBFactory's internal store (managersStore).

For each entry, we will store all DBManagerAllocationSlot attributes altogether - that is:

* A pointer to the DBManager handled by this slot (`managerPtr`)
* A count of references to this DBManager served to the outside (`servedReferences`)
* If we are on a UNIX OS, the filename used for the lock file for this database (`lockFilename`)
* If we are on a UNIX OS, the filedescriptor used for flock() (`lockFd`)