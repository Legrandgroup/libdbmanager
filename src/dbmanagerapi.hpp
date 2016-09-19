/*
This file is part of libdbmanager
(see the file COPYING in the root of the sources for a link to the
homepage of libdbmanager)

libdbmanager is a C++ library providing methods for reading/modifying a
database using only C++ methods & objects and no SQL
Copyright (C) 2016 Legrand SA

libdbmanager is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License version 3
(dated 29 June 2007) as published by the Free Software Foundation.

libdbmanager is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with libdbmanager (in the source code, it is enclosed in
the file named "lgpl-3.0.txt" in the root of the sources).
If not, see <http://www.gnu.org/licenses/>.
*/
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