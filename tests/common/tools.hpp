#ifndef __TOOLS_HPP__
#define __TOOLS_HPP__

#define TEST_TABLE_NAME "unittests"
#define DATABASE_SQLITE_TYPE "sqlite://"

/**
 * \brief Allocate a temporary filename (in the system's temporary directory) and return its name
 *
 * Warning: this is only good for testing, because this function is prone to race-conditions.
 * In order not to have this side-effect we should use mkstemp or mkdtemp
 *
 * \param filename A basename used to build the temporary filename (use the program name for example)
 * \return The temporary filename absolute PATH as a string
**/
std::string mktemp_filename(std::string filename);

/**
 * \brief Extract the program name (basename)
 *
 * This function will use its argument (should be argv[0])] to find out under which name the current process was called
 * Use get_progname() to avoid having to specify argv[0]
 *
 * \return The program basename as a C-string
**/
const char* _get_progname(const char* argv0);

#define get_progname() _get_progname(argv[0])

#endif	// __TOOLS_HPP__