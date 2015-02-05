#include "tools.hpp"

#include <stdlib.h>	// For mktemp()
#include <string.h> // For strrchr()

using namespace std;

string mktemp_filename(string filename) {
	
	string tmpdir;
#if (defined(WIN32) || defined(_WIN32))
	tmpdir = string(getenv("TEMP")) + '/';
#elif (defined(__unix__))
	tmpdir = "/tmp/";
#endif
	
	filename = tmpdir + filename + "-XXXXXX";	/* Add the expected mktemp template */
	
	size_t filenameSz = filename.size();
	
	char tmpfn_template_cstr[filenameSz+1];	/* +1 to hold terminating \0 */
	strncpy(tmpfn_template_cstr, filename.c_str(), filenameSz+1);
	tmpfn_template_cstr[filenameSz] = '\0';	/* Make sure we terminate the C-string */
	return string(mktemp(tmpfn_template_cstr));
}

const char* _get_progname(const char* argv0) {

	const char* progname = argv0;
#if (defined(__unix__) || defined(__MINGW32__) || defined(__MINGW64__))
	progname = strrchr(progname, '/');	/* Find the last / in progname */
#endif
#if (defined(WIN32))
	progname = strrchr(progname, '\\');	/* Find the last \ in progname */
#endif
	if (progname)	/* We found the last occurence of / in argv0, just move forward to point to the exec name */
		progname++;
	else	/* We did not find any '/' or '\\', use argv0 as is */
		progname = argv0;
	
	return progname;
}