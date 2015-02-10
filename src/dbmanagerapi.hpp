/**
 * \file dbmanagerapi.hpp
 * \brief Preprocessor defines (class prefixes) to compile DLLs under Windows
 *
 * This header must be included to properly define LIBDBMANAGER_API in the remaining API headers
 */
 
 #ifndef _DBMANAGERAPI_HPP_
 #define _DBMANAGERAPI_HPP_
 
 #if !defined(LIBDBMANAGER_STATICLIB)
        #if defined(BUILDING_LIBDBMANAGER)
                #if   (defined(__MINGW32__) || defined(__MINGW64__))
                        #define LIBDBMANAGER_API
                #elif (defined(WIN32) || defined(_WIN32))
                        #define LIBDBMANAGER_API  __declspec(dllexport)
                #else
                        #define LIBDBMANAGER_API
                #endif

                #define LIBDBMANAGER_LIB_EXPORT
                #undef LIBDBMANAGER_LIB_IMPORT
        #else
                #if   (defined(__MINGW32__) || defined(__MINGW64__))
                        #define LIBDBMANAGER_API
                #elif (defined(WIN32) || defined(_WIN32))
                        #define LIBDBMANAGER_API  __declspec(dllimport)
                #else
                        #define LIBDBMANAGER_API
                #endif

                #define LIBDBMANAGER_LIB_IMPORT
                #undef LIBDBMANAGER_LIB_EXPORT
        #endif
#endif

#endif	// _DBMANAGERAPI_HPP_