/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: config.h
 * Site-dependent definitions
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2000 Static Free Software.
 *
 * Electric(tm) is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Electric(tm) is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Electric(tm); see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, Mass 02111-1307, USA.
 *
 * Static Free Software
 * 4119 Alpine Road
 * Portola Valley, California 94028
 * info@staticfreesoft.com
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_ 1

/********************* MACHINE DETERMINATION *********************/

/* determine if it is Macintosh OS */
#if defined(__APPLE__)
#  define MACOSX 1
#  if __APPLE_CC__ <= 934
#    define MACOSX_101 1
#  endif
#endif
#if defined(THINK_C) || defined(THINK_CPLUS) || defined(__MWERKS__) || defined(MACOSX)
#  define MACOS 1
#endif

/* determine if it is Windows 9x/NT */
#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#  define WIN32 1
#endif

/* determine if it is UNIX */
#if !defined(MACOS) && !defined(WIN32)
#  define ONUNIX 1
#endif

/********************* MACHINE-SPECIFIC DEFINES *********************/

#ifdef	MACOS						/***** Macintosh *****/
#  ifdef MACOSX
#    ifdef PROJECTBUILDER
#      include <QDOffscreen.h>
#      define STDC_HEADERS     1
#      define HAVE_STRCHR      1
#      define HAVE_QSORT       1
#      define HAVE_STRING_H    1
#      define HAVE_STDLIB_H    1
#      define HAVE_INTTYPES_H  1
#    else
#      include "defines.h"
#    endif
#    define DIRSEP         '/'								/* directory separator */
#    define DIRSEPSTR      x_("/")							/* directory separator */
#    define LIBDIR         x_("lib/")						/* location of library files */
#  else
#    include <QDOffscreen.h>
#    define u_int64_t      uint64_t
#    define STDC_HEADERS     1
#    define HAVE_STRCHR      1
#    define HAVE_QSORT       1
#    define HAVE_STRING_H    1
#    define HAVE_STDLIB_H    1
#    define HAVE_INTTYPES_H  1
#    define DIRSEP         ':'								/* directory separator */
#    define DIRSEPSTR      x_(":")							/* directory separator */
#    define LIBDIR         x_(":lib:")						/* location of library files */
#  endif
#  define MACFSTAG(x)      x
#  define NONFILECH        '|'								/* character that cannot be in file name */
#  define CADRCFILENAME    x_("cadrc")						/* CAD startup file */
#  define ESIMLOC          x_("")
#  define RSIMLOC          x_("")
#  define PRESIMLOC        x_("")
#  define RNLLOC           x_("")
#  define SPICELOC         x_("")
#  define FASTHENRYLOC     x_("")
#  define SORTLOC          x_("")
#  define FLATDRCLOC       x_("")
#  define SFLATDRCLOC      x_("")
#  define BYTES_SWAPPED    1								/* bytes are MSB first */
#endif

#ifdef WIN32						/***** Windows *****/
#  include "windows.h"
#  define STDC_HEADERS     1
#  define HAVE_GETCWD      1
#  define HAVE_QSORT       1
#  define HAVE_STRCHR      1
#  define HAVE_STRING_H    1
#  define HAVE_MKTEMP      1
#  define DIRSEP           '\\'								/* directory separator */
#  define DIRSEPSTR        x_("\\")							/* directory separator */
#  define MACFSTAG(x)      0
#  define NONFILECH        '|'								/* character that cannot be in file name */
#  define CADRCFILENAME    x_("cadrc")						/* CAD startup file */
#  define LIBDIR           x_("lib\\")						/* location of library files */
#  define TCLLIBDIR        x_("C:\\Program Files\\Tcl\\lib\\tcl8.3")/* installed TCL libraries */
#  define ESIMLOC          x_("")
#  define RSIMLOC          x_("")
#  define PRESIMLOC        x_("")
#  define RNLLOC           x_("")
#  define SPICELOC         x_("")
#  define FASTHENRYLOC     x_("")
#  define SORTLOC          x_("")
#  define FLATDRCLOC       x_("")
#  define SFLATDRCLOC      x_("")
#  undef TECHNOLOGY
#  define int64_t          __int64
#  define u_int64_t        unsigned __int64
#endif

#ifdef ONUNIX						/***** UNIX *****/
#  include "defines.h"
#  ifndef USEQT
#    include <X11/Intrinsic.h>
#  endif
#  ifdef sparc
#    define BYTES_SWAPPED  1								/* bytes are MSB first */
#  endif
#  define DIRSEP           '/'								/* directory separator */
#  define DIRSEPSTR        x_("/")							/* directory separator */
#  define MACFSTAG(x)      0
#  define NONFILECH        '?'								/* character that cannot be in file name */
#  define CADRCFILENAME    x_(".cadrc")						/* CAD startup file */
#  ifdef sun
#    define LIBDIR           x_("/usr/local/share/electric/lib/")	/* location of library files */
#    define DOCDIR           x_("/usr/local/share/electric/doc/html/")	/* location of HTML files */
#    define ESIMLOC          x_("/usr/local/bin/esim")
#    define RSIMLOC          x_("/usr/local/bin/rsim")
#    define PRESIMLOC        x_("/usr/local/bin/presim")
#    define RNLLOC           x_("/usr/local/bin/rnl")
#    define SPICELOC         x_("/usr/local/bin/spice")
#    define FASTHENRYLOC     x_("/usr/local/bin/fasthenry")
#    define SORTLOC          x_("/usr/local/bin/sort")
#    define FLATDRCLOC       x_("/usr/local/bin/ffindshort")
#    define SFLATDRCLOC      x_("/usr/local/bin/findshort")
#    define u_int64_t uint64_t
#  else
#    define LIBDIR           x_("/usr/share/electric/lib/")	/* location of library files */
#    define DOCDIR           x_("/usr/share/electric/doc/html/")	/* location of HTML files */
#    define ESIMLOC          x_("/usr/bin/esim")
#    define RSIMLOC          x_("/usr/bin/rsim")
#    define PRESIMLOC        x_("/usr/bin/presim")
#    define RNLLOC           x_("/usr/bin/rnl")
#    define SPICELOC         x_("/usr/bin/spice")
#    define FASTHENRYLOC     x_("/usr/bin/fasthenry")
#    define SORTLOC          x_("/usr/bin/sort")
#    define FLATDRCLOC       x_("/usr/bin/ffindshort")
#    define SFLATDRCLOC      x_("/usr/bin/findshort")
#  endif
#endif

/********************* SOURCE MODULE CONFIGURATION *********************/

/* The tools (User and I/O are always enabled) */
#define COMTOOL            1
#define COMPENTOOL         1
#define DBMIRRORTOOL       FORCESUNTOOLS & FORCEJAVA
#define DRCTOOL            1
#define ERCTOOL            1
#define LOGEFFTOOL         1
#define MAPPERTOOL         0
#define PLATOOL            1
#define PROJECTTOOL        1
#define ROUTTOOL           1
#define SCTOOL             1
#define SIMTOOL            1
#define VHDLTOOL           1

/* special features of tools */
#define SIMTOOLIRSIM        FORCEIRSIMTOOL
#define SIMFSDBWRITE        (FORCEIRSIMTOOL && defined(FORCEFSDB))
#define LOGEFFTOOLSUN       FORCESUNTOOLS

/* The language interfaces */
#define LANGJAVA           FORCEJAVA
#define LANGLISP           FORCELISP
#define LANGTCL            FORCETCL

#endif  /* _CONFIG_H_ - at top */
