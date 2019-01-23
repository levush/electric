/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: global.h
 * Definitions of interest to all modules
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

#ifndef GLOBAL_H
#define GLOBAL_H

/* #define DEBUGMEMORY 1 */					/* uncomment to debug memory */
/* #define DEBUGPARALLELISM 1 */			/* uncomment to multithreaded code */

#include "config.h"

/* system includes */
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#ifdef TM_IN_SYS_TIME
#  include <sys/time.h>
#endif
#if STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#  include <stdarg.h>
#  define var_start(x, y) va_start(x, y)
#else
#  ifndef HAVE_STRCHR
#    define estrchr index
#  endif
#  include <varargs.h>
#  define var_start(x, y) va_start(x)
#endif
#ifdef HAVE_STRINGS_H
#  include <strings.h>
#endif
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#else
#  ifdef HAVE_INTTYPES_H
#    include <inttypes.h>
#  endif
#  ifdef HAVE_DB_H
#    include <db.h>
#  endif
#  if defined(__alpha) && defined(__DECC)
#    define   int64_t          long		/* tru64 needs "long" instead of "int" */
#    define u_int64_t unsigned long		/* tru64 needs "long" instead of "int" */
#  endif
#endif

#ifdef USEQT
#  include <qglobal.h>
#endif

#ifdef INTERNATIONAL
#  ifdef ONUNIX
#    include <libintl.h>
#    ifdef HAVE_LOCALE_H
#      include <locale.h>
#    endif
#  endif
#  ifdef WIN32
#    include "../gettext/libgnuintl.h"
#  endif
#  ifdef MACOS
#    include "libgnuintl.h"
#  endif
#  define  _(string)        gettext(string)
#  define TRANSLATE(string) gettext(string)
#else
#  ifdef _UNICODE
#    define  _(string)      (L ## string)	/* translate now */
#  else
#    define  _(string)      string			/* translate now */
#  endif
#  define TRANSLATE(string) (string)
#endif
#ifdef _UNICODE
#  define N_(string) (L ## string)			/* will be translated later */
#  define M_(string) (L ## string)			/* not used enough to be translated */
#  define x_(string) (L ## string)			/* never translate */
#  define string1byte(string) estring1byte(string)
#  define string2byte(string) estring2byte(string)
#else
#  define N_(string)  string				/* will be translated later */
#  define M_(string)  string				/* not used enough to be translated */
#  define x_(string)  string				/* never translate */
#  define string1byte(string) string
#  define string2byte(string) string
#endif
#define   b_(string)  string				/* never translate, never make unicode */
#define WIDENSTRINGDEFINE(x) x_(x)

/* integer sizes */
#ifdef WIN32
#  if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
#    define BOOLEAN         bool
#  else
#    define BOOLEAN         boolean
#  endif
#else
#  ifdef USEQT
#    define BOOLEAN         bool
#  else
#    define BOOLEAN         Boolean
#  endif
#endif
#ifndef FALSE
#  define FALSE             0
#endif
#ifndef TRUE
#  define TRUE              1
#endif

#ifdef _UNICODE
#  include <wchar.h>
#  define CHAR            wchar_t			/* 16 bits */
#  define UCHAR           wchar_t			/* 16 bits */
#  define estrlen         wcslen
#  define estrcpy         wcscpy
#  define estrncpy        wcsncpy
#  define estrcat         wcscat
#  define estrncat        wcsncat
#  define estrcmp         wcscmp
#  define estrncmp        wcsncmp
#  define estrchr         wcschr
#  define estrrchr        wcsrchr
#  define estrstr         wcsstr
#  define eatoi           _wtoi
#  define eatol           _wtol
#  define eatof           _wtof
#  ifdef WIN32
#    define esnprintf       _snwprintf
#  else
#    define esnprintf       swprintf
#  endif
#  define efprintf        fwprintf
#  define evsprintf       vswprintf
#  define evfprintf       vfwprintf 
#  define esscanf         swscanf
#  define efscanf         fwscanf
#  define estrcspn        wcscspn
#  define efputs          fputws
#else
#  define CHAR            char				/* 8 bits */
#  define UCHAR           unsigned char		/* 8 bits */
#  define estrlen         strlen
#  define estrcpy         strcpy
#  define estrncpy        strncpy
#  define estrcat         strcat
#  define estrncat        strncat
#  define estrcmp         strcmp
#  define estrncmp        strncmp
#  define estrchr         strchr
#  define estrrchr        strrchr
#  define estrstr         strstr
#  define eatoi           atoi
#  define eatol           atol
#  define eatof           atof
#  ifdef WIN32
#    define esnprintf       _snprintf
#  else
#    define esnprintf       snprintf
#  endif
#  define efprintf        fprintf
#  define evsprintf       vsprintf
#  define evfprintf       vfprintf
#  define esscanf         sscanf
#  define efscanf         fscanf
#  define estrcspn        strcspn
#  define efputs          fputs
#endif

#define CHAR1             char				/* always 8 bits */
#define UCHAR1   unsigned char				/* always 8 bits */
#define INTBIG            long				/* at least 32 bits, can hold address */
#define UINTBIG  unsigned long
#define INTSML            short				/* at least 16 bits */
#define UINTSML  unsigned short
#define INTHUGE    int64_t					/* at least 64 bits */
#define UINTHUGE u_int64_t
#define SIZEOFCHAR    (sizeof (CHAR))		/* bytes per character */
#define SIZEOFINTSML  (sizeof (INTSML))		/* bytes per short integer */
#define SIZEOFINTBIG  (sizeof (INTBIG))		/* bytes per long integer */
#define SIZEOFINTHUGE (sizeof (INTHUGE))	/* bytes per huge integer */
#define MAXINTBIG     0x7FFFFFFF			/* largest possible integer */
#ifdef WIN32
#  define INTHUGECONST(a) (a ## i64)
#else
#  define INTHUGECONST(a) (a ## LL)
#endif

/* Use to avoid "unused parameter" warnings */
#ifndef Q_UNUSED
#  define Q_UNUSED(x) (void)x;
#endif

/* basic structures */
typedef INTBIG        XARRAY[3][3];			/* 3x3 transformation matrix */
/** \ingroup Dialogs */
/** Definition of rectangle */
typedef struct
{
	INTSML top;          /**< upper y-coordinate of area */
	INTSML left;         /**< left x-coordinate of area */
	INTSML bottom;       /**< lower y-coordinate of area */
	INTSML right;        /**< right x-coordinate of area */
} RECTAREA;

/* forward declarations for structures */
struct Icomcomp;
struct Inodeinst;
struct Iportarcinst;
struct Iportexpinst;
struct Inodeproto;
struct Iportproto;
struct Iarcinst;
struct Iarcproto;
struct Inetwork;
struct Igeom;
struct Irtnode;
struct Ilibrary;
struct Itechnology;

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/*************************** VARIABLES ****************************/

#define NOVARIABLE  ((VARIABLE *)-1)

/*
 * when adding objects to this list, also update:
 *   "dbtext.c:describeobject()"
 *   "dblang*"
 *   "dbvars.c"
 *   "dbtext.c:db_makestringvar()"
 *   "usrcom.c:us_nextvars()"
 *   "usrstatus.c:us_vartypename[]"
 */
/* the meaning of VARIABLE->type */
#define VUNKNOWN                   0		/* undefined variable */
#define VINTEGER                  01		/* 32-bit integer variable */
#define VADDRESS                  02		/* unsigned address */
#define VCHAR                     03		/* character variable */
#define VSTRING                   04		/* string variable */
#define VFLOAT                    05		/* floating point variable */
#define VDOUBLE                   06		/* double-precision floating point */
#define VNODEINST                 07		/* nodeinst pointer */
#define VNODEPROTO               010		/* nodeproto pointer */
#define VPORTARCINST             011		/* portarcinst pointer */
#define VPORTEXPINST             012		/* portexpinst pointer */
#define VPORTPROTO               013		/* portproto pointer */
#define VARCINST                 014		/* arcinst pointer */
#define VARCPROTO                015		/* arcproto pointer */
#define VGEOM                    016		/* geometry pointer */
#define VLIBRARY                 017		/* library pointer */
#define VTECHNOLOGY              020		/* technology pointer */
#define VTOOL                    021		/* tool pointer */
#define	VRTNODE					 022		/* R-tree pointer */
#define VFRACT                   023		/* fractional integer (scaled by WHOLE) */
#define VNETWORK                 024		/* network pointer */

#define VVIEW                    026		/* view pointer */
#define VWINDOWPART              027		/* window partition pointer */
#define VGRAPHICS                030		/* graphics object pointer */
#define VSHORT                   031		/* 16-bit integer */
#define VCONSTRAINT              032		/* constraint solver */
#define VGENERAL                 033		/* general address/type pairs (used only in fixed-length arrays) */
#define VWINDOWFRAME             034		/* window frame pointer */
#define VPOLYGON                 035		/* polygon pointer */
#define VBOOLEAN                 036		/* boolean variable */
#define VTYPE                    037		/* all above type fields */
#define VCODE1                   040		/* variable is interpreted code (with VCODE2) */
#define VDISPLAY                0100		/* display variable (uses textdescript field) */
#define VISARRAY                0200		/* set if variable is array of above objects */
#define VCREF                   0400		/* variable points into C structure */
#define VLENGTH          03777777000		/* array length (0: array is -1 terminated) */
#define VLENGTHSH                  9		/* right shift for VLENGTH */
#define VCODE2           04000000000		/* variable is interpreted code (with VCODE1) */
#define VLISP                 VCODE1		/* variable is LISP */
#define VTCL                  VCODE2		/* variable is TCL */
#define VJAVA         (VCODE1|VCODE2)		/* variable is Java */
#define VDONTSAVE       010000000000		/* set to prevent saving on disk */
#define VCANTSET        020000000000		/* set to prevent changing value */

/* the meaning of VARIABLE->textdescript */
#define VTPOSITION               017		/* 0: position of text relative to point */
#define VTPOSCENT                  0		/* 0:   text centered about point */
#define VTPOSUP                    1		/* 0:   text centered above point */
#define VTPOSDOWN                  2		/* 0:   text centered below point */
#define VTPOSLEFT                  3		/* 0:   text centered to left of point */
#define VTPOSRIGHT                 4		/* 0:   text centered to right of point */
#define VTPOSUPLEFT                5		/* 0:   text centered to upper-left of point */
#define VTPOSUPRIGHT               6		/* 0:   text centered to upper-right of point */
#define VTPOSDOWNLEFT              7		/* 0:   text centered to lower-left of point */
#define VTPOSDOWNRIGHT             8		/* 0:   text centered to lower-right of point */
#define VTPOSBOXED                 9		/* 0:   text centered and limited to object size */
#define VTDISPLAYPART            060		/* 0: bits telling what to display */
#define VTDISPLAYVALUE             0		/* 0:   display value */
#define VTDISPLAYNAMEVALUE       040		/* 0:   display name and value */
#define VTDISPLAYNAMEVALINH      020		/* 0:   display name, value, 1-level inherit */
#define VTDISPLAYNAMEVALINHALL   060		/* 0:   display name, value, any inherit */
#define VTITALIC                0100		/* 0: set for italic text */
#define VTBOLD                  0200		/* 0: set for bold text */
#define VTUNDERLINE             0400		/* 0: set for underline text */
#define VTISPARAMETER          01000		/* 0: attribute is parameter (nodeinst only) */
#define VTINTERIOR             02000		/* 0: text only appears inside cell */
#define VTINHERIT              04000		/* 0: set to inherit value from proto to inst */
#define VTXOFF              07770000		/* 0: X offset of text */
#define VTXOFFSH                  12		/* 0: right shift of VTXOFF */
#define VTXOFFNEG          010000000		/* 0: set if X offset is negative */
#define VTYOFF          017760000000		/* 0: Y offset of text */
#define VTYOFFSH                  22		/* 0: right shift of VTYOFF */
#define VTYOFFNEG       020000000000		/* 0: set if Y offset is negative */
#define VTOFFMASKWID               9		/* 0: Width of VTXOFF and VTYOFF */
#define VTSIZE                077777		/* 1: size of text */
#define VTSIZESH                   0		/* 1: right shift of VTSIZE */
#define VTFACE             017700000		/* 1: face of text */
#define VTFACESH                  15		/* 1: right shift of VTFACE */
#define VTROTATION         060000000		/* 1: rotation of text */
#define VTROTATIONSH              22		/* 1: right shift of VTROTATION */
#define VTMAXFACE                128		/* 1: maximum value of VTFACE field */
#define VTOFFSCALE       03700000000		/* 1: scale of text offset */
#define VTOFFSCALESH              24		/* 1: right shift of VTOFFSCALE */
#define VTUNITS         034000000000		/* 1: units of text */
#define VTUNITSSH                 29		/* 1: right shift of VTUNITS */
#define VTUNITSNONE                0		/* 1:   units: none */
#define VTUNITSRES                 1		/* 1:   units: resistance */
#define VTUNITSCAP                 2		/* 1:   units: capacitance */
#define VTUNITSIND                 3		/* 1:   units: inductance */
#define VTUNITSCUR                 4		/* 1:   units: current */
#define VTUNITSVOLT                5		/* 1:   units: voltage */
#define VTUNITSDIST                6		/* 1:   units: distance */
#define VTUNITSTIME                7		/* 1:   units: time */

#define TEXTDESCRIPTSIZE 2		/* number of words in a text descriptor */

#define TDCLEAR(t)     tdclear(t)
#define TDCOPY(d, s)   tdcopy(d, s)
#define TDDIFF(t1, t2) tddiff(t1, t2)

/* macros to get the various fields from a text description */
#define TDGETPOS(t)       ((t)[0]&VTPOSITION)
#define TDGETSIZE(t)     (((t)[1]&VTSIZE)>>VTSIZESH)
#define TDGETFACE(t)     (((t)[1]&VTFACE)>>VTFACESH)
#define TDGETROTATION(t) (((t)[1]&VTROTATION)>>VTROTATIONSH)
#define TDGETDISPPART(t)  ((t)[0]&VTDISPLAYPART)
#define TDGETITALIC(t)    ((t)[0]&VTITALIC)
#define TDGETBOLD(t)      ((t)[0]&VTBOLD)
#define TDGETUNDERLINE(t) ((t)[0]&VTUNDERLINE)
#define TDGETINTERIOR(t)  ((t)[0]&VTINTERIOR)
#define TDGETINHERIT(t)   ((t)[0]&VTINHERIT)
#define TDGETISPARAM(t)   ((t)[0]&VTISPARAMETER)
#define TDGETXOFF(t)     gettdxoffset(t)
#define TDGETYOFF(t)     gettdyoffset(t)
#define TDGETOFFSCALE(t) (((t)[1]&VTOFFSCALE)>>VTOFFSCALESH)
#define TDGETUNITS(t)    (((t)[1]&VTUNITS)>>VTUNITSSH)

/* macros to set the various fields from a text description */
#define TDSETPOS(t,v)       (t)[0] = ((t)[0] & ~VTPOSITION) | (v)
#define TDSETSIZE(t,v)      (t)[1] = ((t)[1] & ~VTSIZE) | ((v) << VTSIZESH)
#define TDSETFACE(t,v)      (t)[1] = ((t)[1] & ~VTFACE) | ((v) << VTFACESH)
#define TDSETROTATION(t,v)  (t)[1] = ((t)[1] & ~VTROTATION) | ((v) << VTROTATIONSH)
#define TDSETDISPPART(t,v)  (t)[0] = ((t)[0] & ~VTDISPLAYPART) | (v)
#define TDSETITALIC(t,v)    (t)[0] = ((t)[0] & ~VTITALIC) | (v)
#define TDSETBOLD(t,v)      (t)[0] = ((t)[0] & ~VTBOLD) | (v)
#define TDSETUNDERLINE(t,v) (t)[0] = ((t)[0] & ~VTUNDERLINE) | (v)
#define TDSETINTERIOR(t,v)  (t)[0] = ((t)[0] & ~VTINTERIOR) | (v)
#define TDSETINHERIT(t,v)   (t)[0] = ((t)[0] & ~VTINHERIT) | (v)
#define TDSETISPARAM(t,v)   (t)[0] = ((t)[0] & ~VTISPARAMETER) | (v)
#define TDSETOFF(t,x, y)    settdoffset(t, x, y)
#define TDSETOFFSCALE(t,v)  (t)[1] = ((t)[1] & ~VTOFFSCALE) | ((v) << VTOFFSCALESH)
#define TDSETUNITS(t,v)     (t)[1] = ((t)[1] & ~VTUNITS) | ((v) << VTUNITSSH)

typedef struct
{
	INTBIG    key;							/* library-specific key to this name */
	UINTBIG   type;							/* the type of variables (see above) */
	UINTBIG   textdescript[TEXTDESCRIPTSIZE];	/* nature of text that displays variable */
	INTBIG    addr;							/* contents of variable */
} VARIABLE;

extern CHAR  **el_namespace;				/* names in name space */
extern INTBIG  el_numnames;					/* number of names in name space */

/* some keys to commonly used variable names */
extern INTBIG  el_node_name_key;			/* key for "NODE_name" */
extern INTBIG  el_arc_name_key;				/* key for "ARC_name" */
extern INTBIG  el_arc_radius_key;			/* key for "ARC_radius" */
extern INTBIG  el_trace_key;				/* key for "trace" */
extern INTBIG  el_cell_message_key;			/* key for "FACET_message" */
extern INTBIG  el_schematic_page_size_key;	/* key for "FACET_schematic_page_size" */
extern INTBIG  el_transistor_width_key;		/* key for "transistor_width" */
extern INTBIG  el_prototype_center_key;		/* key for "prototype_center" */
extern INTBIG  el_essential_bounds_key;		/* key for "FACET_essentialbounds" */
extern INTBIG  el_node_size_default_key;	/* key for "NODE_size_default" */
extern INTBIG  el_arc_width_default_key;	/* key for "ARC_width_default" */
extern INTBIG  el_attrkey_area;				/* key for "ATTR_area" */
extern INTBIG  el_attrkey_length;			/* key for "ATTR_length" */
extern INTBIG  el_attrkey_width;			/* key for "ATTR_width" */
extern INTBIG  el_attrkey_M;			    /* key for "ATTR_M" */
extern INTBIG  el_techstate_key;			/* key for "TECH_state" */

/*************************** MEMORY ALLOCATION ****************************/

#define NOCLUSTER	((CLUSTER *)-1)

#define CLUSTERFILLING   1					/* set if no need to search entire cluster */

typedef struct Icluster
{
	INTBIG           address;				/* base address of this cluster */
	INTBIG           flags;					/* information bits about this cluster */
	CHAR             clustername[30];		/* for debugging only */
	INTBIG           clustersize;			/* number of pages to allocate at a time */
	struct Icluster *nextcluster;			/* next in linked list */
} CLUSTER;

extern CLUSTER *el_tempcluster;				/* cluster for temporary allocation */
extern CLUSTER *db_cluster;					/* database general allocation */

/************************** COMMAND COMPLETION ***************************/

#define TEMPLATEPARS               5		/* maximum parameters in a template */
#define MAXPARS                   20		/* maximum parameters in a command */
#define NOCOMCOMP    ((COMCOMP *)-1)
#define NOKEYWORD    ((KEYWORD *)-1)
#define NOKEY              NOCOMCOMP		/* short form for null keyword */
#define TERMKEY      {NULL, 0, {NULL, NULL, NULL, NULL, NULL}}
#define NOTOPLIST     (us_patoplist)		/* no routine for the keyword table */
#define NONEXTLIST (us_panextinlist)		/* no routine for table slot */
#define NOPARAMS       (us_paparams)		/* no routine for table slot */

/* bits in COMCOMP->interpret */
#define NOFILL                    01		/* if set, don't fill out keyword */
#define NOSHOALL                  02		/* if set, suppress options list when null */
#define INPUTOPT                  04		/* if set, this can be input option on popup */
#define INCLUDENOISE             010		/* if set, include "noise" option in list */
#define MULTIOPT                 020		/* if set, allow multiple menu picks */

/*
 * tables of command options use this structure
 */
typedef struct Ikeyword
{
	CHAR            *name;					/* name of this command */
	INTBIG           params;				/* number of parameters to command */
	struct Icomcomp *par[TEMPLATEPARS];		/* parameter types */
} KEYWORD;

/*
 * this structure defines the basic command parameter
 */
typedef struct Icomcomp
{
	KEYWORD *ifmatch;						/* list of keywords to search if it matches */
	BOOLEAN (*toplist)(CHAR**);				/* reset to top of list of keywords */
	CHAR  *(*nextcomcomp)(void);			/* give next keyword in list */
	INTBIG (*params)(CHAR*, struct Icomcomp*[], CHAR);	/* set parameters to keyword */
	INTBIG   interpret;						/* bits for interpretation */
	CHAR    *breakchrs;						/* keyword separation characters */
	CHAR    *noise;							/* describe list */
	CHAR    *def;							/* default value */
} COMCOMP;

/*************************** TOOLS ****************************/

#define NOTOOL	((TOOL *)-1)

/* tool descriptors */
typedef struct Itool
{
	CHAR    *toolname;						/* name of tool */
	INTBIG   toolstate;						/* state of tool */
	INTBIG   toolindex;						/* tool index */
	COMCOMP *parse;							/* parsing structure for tool direction */
	CLUSTER *cluster;						/* virtual memory cluster for this tool */

	void   (*init)(INTBIG*, CHAR1*[], struct Itool*);		/* initialization */
	void   (*done)(void);									/* completion */
	void   (*setmode)(INTBIG, CHAR*[]);						/* user-command options */
	INTBIG (*request)(CHAR*, va_list);						/* direct call options */
	void   (*examinenodeproto)(struct Inodeproto*);			/* to examine an entire cell at once */
	void   (*slice)(void);									/* time slice */

	void   (*startbatch)(struct Itool*, BOOLEAN);			/* start change broadcast */
	void   (*endbatch)(void);								/* end change broadcast */

	void   (*startobjectchange)(INTBIG, INTBIG);			/* broadcast that object about to be changed */
	void   (*endobjectchange)(INTBIG, INTBIG);				/* broadcast that object done being changed */

	void   (*modifynodeinst)(struct Inodeinst*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);	/* broadcast modified nodeinst */
	void   (*modifynodeinsts)(INTBIG,struct Inodeinst**,INTBIG*,INTBIG*,INTBIG*,INTBIG*,INTBIG*,INTBIG*);	/* broadcast modified nodeinsts */
	void   (*modifyarcinst)(struct Iarcinst*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);	/* broadcast modified arcinst */
	void   (*modifyportproto)(struct Iportproto*, struct Inodeinst*, struct Iportproto*);	/* broadcast modified portproto */
	void   (*modifynodeproto)(struct Inodeproto*);			/* broadcast modified nodeproto */
	void   (*modifydescript)(INTBIG, INTBIG, INTBIG, UINTBIG*);	/* broadcast modified descriptor */

	void   (*newobject)(INTBIG, INTBIG);					/* broadcast new object */
	void   (*killobject)(INTBIG, INTBIG);					/* broadcast deleted object */
	void   (*newvariable)(INTBIG, INTBIG, INTBIG, INTBIG);	/* broadcast new variable */
	void   (*killvariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);	/* broadcast deleted variable */
	void   (*modifyvariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);	/* broadcast modified array variable entry */
	void   (*insertvariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);			/* broadcast inserted array variable entry */
	void   (*deletevariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);	/* broadcast deleted array variable entry */

	void   (*readlibrary)(struct Ilibrary*);				/* broadcast newly read library */
	void   (*eraselibrary)(struct Ilibrary*);				/* broadcast library to be erased */
	void   (*writelibrary)(struct Ilibrary*, BOOLEAN);		/* broadcast library writing */

	/* variables */
	VARIABLE *firstvar;						/* first variable in list */
	INTSML    numvar;						/* number of variables in list */
} TOOL;

extern TOOL      el_tools[];				/* defined in "aidtable.c" */
extern INTBIG    el_maxtools;				/* current number of tools */
extern TOOL     *us_tool;					/* the User tool object */
extern TOOL     *io_tool;					/* the I/O tool object */
extern TOOL     *net_tool;					/* the Network tool object */

/* the meaning of TOOL->toolstate */
#define TOOLON                    01		/* set if tool is on */
#define TOOLBG                    02		/* set if tool is running in background */
#define TOOLFIX                   04		/* set if tool will fix errors */
#define TOOLLANG                 010		/* set if tool is coded in interpretive language */
#define TOOLINCREMENTAL          020		/* set if tool functions incrementally */
#define TOOLANALYSIS             040		/* set if tool does analysis */
#define TOOLSYNTHESIS           0100		/* set if tool does synthesis */

/*************************** VIEWS ****************************/

#define NOVIEW          ((VIEW *)-1)

/* the meaning of VIEW->viewstate */
#define TEXTVIEW                  01		/* view contains only text */
#define MULTIPAGEVIEW             02		/* view is one of multiple pages */
#define PERMANENTVIEW             04		/* view is statically defined and cannot be deleted */

typedef struct Iview
{
	CHAR         *viewname;					/* name of this view */
	CHAR         *sviewname;				/* short name of this view */
	struct Iview *nextview;					/* next in linked list */
	INTBIG        temp1, temp2;				/* working variables */
	INTBIG        viewstate;				/* information about the view */

	/* variables */
	VARIABLE     *firstvar;					/* first variable in list */
	INTSML        numvar;					/* number of variables in list */
} VIEW;

extern VIEW *el_views;						/* list of existing view */
extern VIEW *el_unknownview;				/* the unknown view */
extern VIEW *el_layoutview;					/* the layout view */
extern VIEW *el_schematicview;				/* the schematic view */
extern VIEW *el_iconview;					/* the icon view */
extern VIEW *el_simsnapview;				/* the simulation-snapshot view */
extern VIEW *el_skeletonview;				/* the skeleton view */
extern VIEW *el_compview;					/* the compensated view */
extern VIEW *el_vhdlview;					/* the VHDL view (text) */
extern VIEW *el_verilogview;				/* the Verilog view (text) */
extern VIEW *el_netlistview;				/* the netlist view, generic (text) */
extern VIEW *el_netlistnetlispview;			/* the netlist view, netlisp (text) */
extern VIEW *el_netlistalsview;				/* the netlist view, als (text) */
extern VIEW *el_netlistquiscview;			/* the netlist view, quisc (text) */
extern VIEW *el_netlistrsimview;			/* the netlist view, rsim (text) */
extern VIEW *el_netlistsilosview;			/* the netlist view, silos (text) */
extern VIEW *el_docview;					/* the documentation view (text) */

/*************************** NODE INSTANCES ****************************/

#define NONODEINST  ((NODEINST *)-1)

/* the meaning of NODEINST->userbits */
#define DEADN                     01		/* node is not in use */
#define NHASFARTEXT               02		/* node has text that is far away */
#define NEXPAND                   04		/* if on, draw node expanded */
#define WIPED                    010		/* set if node not drawn due to wiping arcs */
#define NSHORT                   020		/* set if node is to be drawn shortened */
/*  used by database:           0140 */
/*  used by user:          040177600 */
#define NTECHBITS          037400000		/* technology-specific bits for primitives */
#define NTECHBITSSH               17		/* right-shift of NTECHBITS */
#define NILOCKED          0100000000		/* set if node is locked (can't be changed) */

/* node instances */
typedef struct Inodeinst
{
	INTBIG               lowx, highx;		/* bounding X box of nodeinst */
	INTBIG               lowy, highy;		/* bounding Y box of nodeinst */
	INTSML               transpose;			/* nonzero to transpose before rotation */
	INTSML               rotation;			/* angle from normal (0 to 359) */
	struct Inodeproto   *proto;				/* current nodeproto */
	struct Inodeproto   *parent;			/* cell that contains this nodeinst */
	struct Inodeinst    *prevnodeinst;		/* list of instances in parent cell */
	struct Inodeinst    *nextnodeinst;
	struct Igeom        *geom;				/* relative geometry list pointer */
	struct Inodeinst    *previnst;			/* list of others of this type */
	struct Inodeinst    *nextinst;
	struct Iportarcinst *firstportarcinst;	/* first portarcinst on node */
	struct Iportexpinst *firstportexpinst;	/* first portexpinst on node */
	UINTBIG              textdescript[TEXTDESCRIPTSIZE];	/* nature of text that displays cell name */
	INTBIG               arraysize;			/* extent of array of this node (0 if not) */

	/* change information */
	CHAR                *changeaddr;		/* change associated with this nodeinst */
	INTBIG               changed;			/* clock entry for changes to this nodeinst */

	/* tool information */
	UINTBIG              userbits;			/* state flags */
	INTBIG               temp1, temp2;		/* temporaries */

	/* variables */
	VARIABLE            *firstvar;			/* first variable in list */
	INTSML               numvar;			/* number of variables in list */
} NODEINST;

/*************************** PORT ARC INSTANCES ****************************/

#define NOPORTARCINST ((PORTARCINST *)-1)

typedef struct Iportarcinst
{
	struct Iportproto   *proto;				/* portproto of this portarcinst */
	struct Iarcinst     *conarcinst;		/* arcinst connecting to this portarcinst */
	struct Iportarcinst *nextportarcinst;	/* next portarcinst in list */

	/* variables */
	VARIABLE            *firstvar;			/* first variable in list */
	INTSML               numvar;			/* number of variables in list */
} PORTARCINST;

/************************** PORT EXPORT INSTANCES ***************************/

#define NOPORTEXPINST ((PORTEXPINST *)-1)

typedef struct Iportexpinst
{
	struct Iportproto   *proto;				/* portproto of this portexpinst */
	struct Iportexpinst *nextportexpinst;	/* next portexpinst in list */
	struct Iportproto   *exportproto;		/* portproto on parent cell */

	/* variables */
	VARIABLE            *firstvar;			/* first variable in list */
	INTSML               numvar;			/* number of variables in list */
} PORTEXPINST;

/*************************** NODE PROTOTYPES ****************************/

#define NONODEPROTO ((NODEPROTO *)-1)

/* the meaning of NODEPROTO->userbits */
#define NODESHRINK                01		/* set if nonmanhattan instances shrink */
#define WANTNEXPAND               02		/* set if instances should be expanded */
#define NFUNCTION               0774		/* node function (from efunction.h) */
#define NFUNCTIONSH                2		/* right shift for NFUNCTION */
#define ARCSWIPE               01000		/* set if instances can be wiped */
#define NSQUARE                02000		/* set if node is to be kept square in size */
#define HOLDSTRACE             04000		/* primitive can hold trace information */
#define REDOCELLNET           010000		/* set to reevaluate this cell's network */
#define WIPEON1OR2            020000		/* set to erase if connected to 1 or 2 arcs */
#define LOCKEDPRIM            040000		/* set if primitive is lockable (cannot move) */
#define NEDGESELECT          0100000		/* set if primitive is selectable by edge, not area */
#define ARCSHRINK            0200000		/* set if nonmanhattan arcs on this shrink */
/*  used by database:       01400000 */
#define NNOTUSED            02000000		/* set if not used (don't put in menu) */
#define NPLOCKED            04000000		/* set if everything in cell is locked */
#define NPILOCKED          010000000		/* set if instances in cell are locked */
#define INCELLLIBRARY      020000000		/* set if cell is part of a "cell library" */
#define TECEDITCELL        040000000		/* set if cell is from a technology-library */
/*  used by user:       037700000000 */

#define GLOBALNETGROUND            0		/* index into "globalnetworks" for ground net */
#define GLOBALNETPOWER             1		/* index into "globalnetworks" for power net */

/* macro for looping through all cells in a group */
#define FOR_CELLGROUP(np, firstnp) \
	for(np = firstnp; np != NONODEPROTO; np = (np->nextcellgrp == firstnp ? NONODEPROTO : np->nextcellgrp))

#ifdef __cplusplus
class NetCellPrivate;                      /* private network data */
#else
typedef void NetCellPrivate;
#endif

typedef struct Inodeproto
{
	/* used by all tools */
	CHAR               *protoname;			/* nodeproto name !!!NEW!!! */
	INTBIG              primindex;			/* nonzero if primitive */
	INTBIG              lowx, highx;		/* bounds in X */
	INTBIG              lowy, highy;		/* bounds in Y */
	NODEINST           *firstinst;			/* first in list of instances of this kind */
	NODEINST           *firstnodeinst;		/* head of list of nodeinsts in this cell */
	struct Iarcinst    *firstarcinst;		/* head of list of arcinsts in this cell */
	struct Itechnology *tech;				/* technology with this nodeproto (prim only) */
	struct Ilibrary    *lib;				/* library with this nodeproto (cells only) !!!NEW!!! */
	struct Inodeproto  *prevnodeproto;		/* previous in library/technology list */
	struct Inodeproto  *nextnodeproto;		/* next in library/technology list */
	struct Iportproto  *firstportproto;		/* list of ports */
	struct Inodeproto  *cachedequivcell;	/* cache of equivalent cell for quick port matching */
	struct Inodeproto  *nextcellgrp;		/* next in circular list of cells in group !!!NEW!!! */
	struct Inodeproto  *nextcont;			/* next in circular list of cells in continuation !!!NEW!!! */

	INTBIG              numportprotos;		/* number of portprotos in this nodeproto */
	struct Iportproto **portprotohashtable;	/* hash table of portprotos in this nodeproto */
	INTBIG              portprotohashtablesize;	/* size of portproto hash table */

	VIEW               *cellview;			/* view of this cell */
	INTBIG              version;			/* version number of this cell */
	struct Inodeproto  *prevversion;		/* earlier version of this cell */
	struct Inodeproto  *newestversion;		/* most recent version of this cell */
	UINTBIG             creationdate;		/* date cell was created */
	UINTBIG             revisiondate;		/* date cell was last changed */

	struct Inetwork    *firstnetwork;		/* nets in this cell */
    NetCellPrivate     *netd;               /* private network data */
	INTBIG              globalnetcount;		/* number of global nets in this cell */
	INTBIG             *globalnetchar;		/* global network characteristics in this cell */
	struct Inetwork   **globalnetworks;		/* global nets in this cell */
	CHAR              **globalnetnames;		/* global net names in this cell */
	struct Irtnode     *rtree;				/* top of geometric tree of objects in cell */

	/* change information */
	CHAR               *changeaddr;			/* change associated with this cell */

	/* tool specific */
	unsigned            adirty;				/* "dirty" bit for each tool */
	UINTBIG             userbits;			/* state flags */
	INTBIG              temp1, temp2;		/* temporaries */

	/* variables */
	VARIABLE           *firstvar;			/* first variable in list */
	INTSML              numvar;				/* number of variables in list */
} NODEPROTO;

/*************************** PORT PROTOTYPES ****************************/

#define NOPORTPROTO ((PORTPROTO *)-1)

/* the meaning of PORTPROTO->userbits */
#define PORTANGLE               0777		/* angle of this port from node center */
#define PORTANGLESH                0		/* right shift of PORTANGLE field */
#define PORTARANGE           0377000		/* range of valid angles about port angle */
#define PORTARANGESH               9		/* right shift of PORTARANGE field */
#define PORTNET           0177400000		/* electrical net of primitive port (0-30) */
                                            /* 31 stands for one-port net */
#define PORTNETSH                 17		/* right shift of PORTNET field */
#define PORTISOLATED      0200000000		/* set if arcs to this port do not connect */
#define PORTDRAWN         0400000000		/* set if this port should always be drawn */
#define BODYONLY         01000000000		/* set to exclude this port from the icon */
#define STATEBITS       036000000000		/* input/output/power/ground/clock state: */
#define STATEBITSSH               27		/* right shift of STATEBITS */
#define CLKPORT          02000000000		/*   un-phased clock port */
#define C1PORT           04000000000		/*   clock phase 1 */
#define C2PORT           06000000000		/*   clock phase 2 */
#define C3PORT          010000000000		/*   clock phase 3 */
#define C4PORT          012000000000		/*   clock phase 4 */
#define C5PORT          014000000000		/*   clock phase 5 */
#define C6PORT          016000000000		/*   clock phase 6 */
#define INPORT          020000000000		/*   input port */
#define OUTPORT         022000000000		/*   output port */
#define BIDIRPORT       024000000000		/*   bidirectional port */
#define PWRPORT         026000000000		/*   power port */
#define GNDPORT         030000000000		/*   ground port */
#define REFOUTPORT      032000000000		/*   bias-level reference output port */
#define REFINPORT       034000000000		/*   bias-level reference input port */
#define REFBASEPORT     036000000000		/*   bias-level reference base port */

typedef struct Iportproto
{
	struct Iarcproto **connects;			/* arc prototypes that can touch this port */
	NODEPROTO         *parent;				/* nodeproto that this portproto resides in */
	NODEINST          *subnodeinst;			/* subnodeinst that portproto comes from */
	PORTEXPINST       *subportexpinst;		/* portexpinst in subnodeinst */
	struct Iportproto *subportproto;		/* portproto in above subnodeinst */
	struct Iportproto *nextportproto;		/* next in list of port prototypes */
	CHAR              *protoname;			/* name of this port prototype */
	UINTBIG            textdescript[TEXTDESCRIPTSIZE];		/* nature of text that displays proto name */
	struct Iportproto *cachedequivport;		/* cache of equivalent port in other cell */

	/* change information */
	CHAR              *changeaddr;			/* change associated with this port */

	/* for the tools */
	UINTBIG            userbits;			/* state flags */
	struct Inetwork   *network;				/* network object within cell */
	INTBIG             temp1, temp2;		/* temporaries */

	/* variables */
	VARIABLE          *firstvar;			/* first variable in list */
	INTSML             numvar;				/* number of variables in list */
} PORTPROTO;

/*************************** ARC INSTANCES ****************************/

#define NOARCINST    ((ARCINST *)-1)

/* the meaning of ARCINST->userbits */
#define FIXED                     01		/* fixed-length arc */
#define FIXANG                    02		/* fixed-angle arc */
#define AHASFARTEXT               04		/* arc has text that is far away */
#define DEADA                    020		/* arc is not in use */
#define AANGLE                037740		/* angle of arc from end 0 to end 1 */
#define AANGLESH                   5		/* bits of right shift for AANGLE field */
#define ASHORT                040000		/* set if arc is to be drawn shortened */
#define ISHEADNEGATED        0200000		/* set if head end is negated (from Java Electric) */
#define NOEXTEND             0400000		/* set if ends do not extend by half width */
#define ISNEGATED           01000000		/* set if tail end is negated */
#define ISDIRECTIONAL       02000000		/* set if arc aims from end 0 to end 1 */
#define NOTEND0             04000000		/* no extension/negation/arrows on end 0 */
#define NOTEND1            010000000		/* no extension/negation/arrows on end 1 */
#define REVERSEEND         020000000		/* reverse extension/negation/arrow ends */
#define CANTSLIDE          040000000		/* set if arc can't slide around in ports */
/*  used by database:     0100000000 */
/*  used by user:       037600000000 */

typedef struct Iarcinst
{
	/* physical description of arcinst */
	struct Iarcproto *proto;				/* arc prototype of this arcinst */
	INTBIG            length;				/* length of arcinst */
	INTBIG            width;				/* width of arcinst */
	INTBIG            endshrink;			/* shrinkage factor on ends */
	struct
	{
		INTBIG        xpos, ypos;			/* position of arcinst end */
		NODEINST     *nodeinst;				/* connecting nodeinst */
		PORTARCINST  *portarcinst;			/* portarcinst in the connecting nodeinst */
	} end[2];								/* for each end of the arcinst */
	struct Iarcinst  *prevarcinst;			/* list of arcs in the parent cell */
	struct Iarcinst  *nextarcinst;
	struct Igeom     *geom;					/* geometry entry */
	NODEPROTO        *parent;				/* parent cell */

	/* change information */
	CHAR             *changeaddr;			/* change associated with this nodeinst */
	INTBIG            changed;				/* clock entry for changes to this nodeinst */

	/* tool specific */
	UINTBIG           userbits;				/* state flags */
	struct Inetwork  *network;				/* network object within cell */
	INTBIG            temp1, temp2;			/* temporaries */

	/* variables */
	VARIABLE         *firstvar;				/* first variable in list */
	INTSML            numvar;				/* number of variables in list */
} ARCINST;

/*************************** ARC PROTOTYPES ****************************/

#define NOARCPROTO  ((ARCPROTO *)-1)

/* the meaning of ARCPROTO->userbits */
#define WANTFIX                   01		/* these arcs are fixed-length */
#define WANTFIXANG                02		/* these arcs are fixed-angle */
#define WANTCANTSLIDE             04		/* set if arcs should not slide in ports */
#define WANTNOEXTEND             010		/* set if ends do not extend by half width */
#define WANTNEGATED              020		/* set if arcs should be negated */
#define WANTDIRECTIONAL          040		/* set if arcs should be directional */
#define CANWIPE                 0100		/* set if arcs can wipe wipable nodes */
#define CANCURVE                0200		/* set if arcs can curve */
#define AFUNCTION             017400		/* arc function (from efunction.h) */
#define AFUNCTIONSH                8		/* right shift for AFUNCTION */
#define AANGLEINC          017760000		/* angle increment for this type of arc */
#define AANGLEINCSH               13		/* right shift for AANGLEINC */
#define AEDGESELECT        020000000		/* set if arc is selectable by edge, not area */
/*  used by user:       017740000000 */
#define ANOTUSED        020000000000		/* set if not used (don't put in menu) */

typedef struct Iarcproto
{
	/* information for all tools */
	CHAR               *protoname;			/* full arcproto name */
	INTBIG              nominalwidth;		/* default width of arcs */
	INTBIG              arcindex;			/* index number of arcproto in technology */
	struct Itechnology *tech;				/* technology this arcproto is in */
	struct Iarcproto   *nextarcproto;		/* next in list for this technology */

	/* information for specific tool */
	UINTBIG             userbits;			/* state flags */
	INTBIG              temp1, temp2;		/* temporaries */

	/* variables */
	VARIABLE           *firstvar;			/* first variable in list */
	INTSML              numvar;				/* number of variables in list */
} ARCPROTO;

/**************************** NETWORKS ****************************/

#define NONETWORK    ((NETWORK *)-1)

typedef struct Inetwork
{
	INTBIG            netnameaddr;          /* address of name(s) of this network */
	INTSML            namecount;			/* number of names */
	INTSML            tempname;				/* nonzero if name is temporary (for back annotation) */
	INTSML            arccount;				/* number of arcs on this network */
	INTSML            arctotal;				/* total size of "arcaddr" array */
	INTBIG            arcaddr;				/* address of arc(s) on this network */
	INTSML            refcount;				/* number of arcs on network */
	INTSML            portcount;			/* number of ports on this network */
	INTSML            buslinkcount;			/* number of busses referencing this network */
	NODEPROTO        *parent;				/* cell that has this network */
	INTSML            globalnet;			/* index to list of global nets (-1 if not global) */
	INTSML            buswidth;				/* width of bus */
	struct Inetwork **networklist;			/* list of single-wire networks on bus */
	struct Inetwork  *nextnetwork;			/* next in linked list */
	struct Inetwork  *prevnetwork;			/* previous in linked list */
	INTBIG            temp1, temp2;			/* temporaries */

	/* variables */
	VARIABLE         *firstvar;				/* first variable in list */
	INTSML            numvar;				/* number of variables in list */
} NETWORK;

/**************************** GEOMETRY POINTERS ****************************/

#define NOGEOM          ((GEOM *)-1)

/*
 * Each nodeinst and arcinst points to a geometry module.  The module points
 * back to the nodeinst or arcinst and describes the position relative to
 * other nodes and arcs.  Geometry modules are also at the leaf positions
 * in R-trees, which organize them spatially.
 */
typedef struct Igeom
{
	BOOLEAN        entryisnode;				/* true if node, false if arc */
	union          u_entry
	{
		NODEINST  *ni;
		ARCINST   *ai;
		void      *blind;
	} entryaddr;							/* pointer to the element */
	INTBIG         lowx, highx;				/* horizontal bounds */
	INTBIG         lowy, highy;				/* vertical bounds */

	/* variables */
	VARIABLE      *firstvar;				/* first variable in list */
	INTSML         numvar;					/* number of variables in list */
} GEOM;

#define MINRTNODESIZE              4		/* lower bound on R-tree node size */
#define MAXRTNODESIZE (MINRTNODESIZE*2)

#define NORTNODE      ((RTNODE *)-1)

typedef struct Irtnode
{
	INTBIG          lowx, highx;			/* X bounds of this node */
	INTBIG          lowy, highy;			/* Y bounds of this node */
	INTSML          total;					/* number of pointers */
	INTSML          flag;					/* nonzero if children are terminal */
	UINTBIG         pointers[MAXRTNODESIZE];/* pointers */
	struct Irtnode *parent;					/* parent node */

	/* variables */
	VARIABLE       *firstvar;				/* first variable in list */
	INTSML          numvar;					/* number of variables in list */
} RTNODE;

/*************************** LIBRARIES ****************************/

#define NOLIBRARY    ((LIBRARY *)-1)

/* the meaning of LIBRARY->userbits */
#define LIBCHANGEDMAJOR           01		/* library has changed significantly */
#define REDOCELLLIB               02		/* recheck networks in library */
#define READFROMDISK              04		/* set if library came from disk */
#define LIBUNITS                 070		/* internal units in library (see INTERNALUNITS) */
#define LIBUNITSSH                 3		/* right shift for LIBUNITS */
#define LIBCHANGEDMINOR         0100		/* library has changed insignificantly */
#define HIDDENLIBRARY           0200		/* library is "hidden" (clipboard library) */
#define UNWANTEDLIB             0400		/* library is unwanted (used during input) */

typedef struct Ilibrary
{
	NODEPROTO       *firstnodeproto;		/* list of nodeprotos in this library */
	NODEPROTO       *tailnodeproto;			/* end of list of nodeprotos in this library */
	INTBIG           numnodeprotos;			/* number of nodeprotos in this library !!!NEW!!! */
	NODEPROTO      **nodeprotohashtable;	/* hash table of nodeprotos in this library !!!NEW!!! */
	VIEW           **nodeprotoviewhashtable;/* hash table of nodeproto views in this library !!!NEW!!! */
	INTBIG           nodeprotohashtablesize;/* size of nodeproto hash table !!!NEW!!! */
	NETWORK         *freenetwork;			/* free nets in this library !!!NEW!!! */
	CHAR            *libname;				/* name of this library */
	CHAR            *libfile;				/* file this library comes from */
	INTBIG          *lambda;				/* half-nanometers per unit in this library */
	NODEPROTO       *curnodeproto;			/* top cell of this library (if any) */
	struct Ilibrary *nextlibrary;			/* next library in list */
	UINTBIG          userbits;				/* state flags */
	INTBIG           temp1, temp2;			/* temporaries */
	CLUSTER         *cluster;				/* memory cluster for whole library */

	/* variables */
	VARIABLE        *firstvar;				/* first variable in list */
	INTSML           numvar;				/* number of variables in list */
} LIBRARY;

extern LIBRARY  *el_curlib;					/* pointer to current library (list head) */

/*************************** GRAPHICS and POLYGONS ****************************/

#define NOGRAPHICS  ((GRAPHICS *)-1)

typedef struct
{
	INTBIG    bits;							/* bit planes to use (color displays) */
	INTBIG    col;							/* color to draw */
	INTSML    colstyle;						/* drawing style for color displays */
	INTSML    bwstyle;						/* drawing style for B&W displays */
	UINTSML   raster[16];					/* 16x16 raster pattern (for stippling) */

	/* variables */
	VARIABLE *firstvar;						/* first variable in list */
	INTSML    numvar;						/* number of variables in list */
} GRAPHICS;


#define NOPOLYGON    ((POLYGON *)-1)

typedef struct Ipolygon
{
	INTBIG             *xv, *yv;			/* the polygon coordinates */
	INTBIG              limit;				/* maximum number of points in polygon */
	INTBIG              count;				/* current number of points in polygon */
	CLUSTER            *cluster;			/* virtual memory cluster with this polygon */
	INTBIG              style;				/* the polygon style */
	CHAR               *string;				/* string (if text graphics) */
	UINTBIG             textdescript[TEXTDESCRIPTSIZE];	/* text information (if text) */
	struct Itechnology *tech;				/* technology (if text graphics) */
	GRAPHICS           *desc;				/* graphical description of polygon */
	INTBIG              layer;				/* layer number of this polygon */
	PORTPROTO          *portproto;			/* port prototype associated with this polygon */
	struct Ipolygon    *nextpolygon;		/* for linking them into a list */

	/* variables */
	VARIABLE *firstvar;						/* first variable in list */
	INTSML    numvar;						/* number of variables in list */
} POLYGON;

typedef struct
{
	POLYGON **polygons;			/* polygons */
	INTBIG   *lx, *hx;			/* X bound of each polygon */
	INTBIG   *ly, *hy;			/* Y bound of each polygon */
	INTBIG    polylisttotal;	/* allocated size of list */
	INTBIG    polylistcount;	/* number of polygons in the list */
} POLYLIST;

/******************** EDITOR MODULES ********************/

#define NOEDITOR      ((EDITOR *)-1)

/* the meaning of EDITOR->state */
#define EDITORTYPE                07		/* type of editor */
#define PACEDITOR                 01		/* editor is point-and-click */
#define EMACSEDITOR               02		/* editor is EMACS-like */
#define EDITORINITED             010		/* set if editor has been initialized */
#define EGRAPHICSOFF             020		/* set if editor graphics is disabled */
#define LINESFIXED               040		/* set if editor disallows line insertion/deletion */
#define TEXTTYPING              0100		/* set if typing into the editor (text undo) */
#define TEXTTYPED               0200		/* set if typed into the editor (text undo) */

/* an overloaded entry */
#define charposition highlightedline

typedef struct Ieditor
{
	INTBIG    state;						/* status bits (see above) */
	INTBIG    swid, shei;					/* screen width and height */
	INTBIG    revy;							/* for reversing Y coordinates */
	INTBIG    offx;							/* for adjusting X coordinates */
	INTBIG    maxlines;						/* maximum lines in buffer */
	INTBIG    linecount;					/* actual number of lines in buffer */
	INTBIG    screenlines;					/* number of lines on screen */
	INTBIG    working;						/* line number being changed */
	INTBIG    firstline;					/* top line on screen */
	INTBIG    screenchars;					/* maximum characters across screen */
	INTBIG    mostchars;					/* most characters on any line */
	INTBIG    curline, curchar;				/* current start position */
	INTBIG    endline, endchar;				/* current end position */
	INTBIG    highlightedline;				/* line being highlighted (EMACS only) */
	INTBIG    horizfactor;					/* position of 1st character on left side */
	INTBIG    vthumbpos;					/* Y position of vertical thumb slider */
	INTBIG    hthumbpos;					/* X position of horizontal thumb slider */
	BOOLEAN   dirty;						/* nonzero if buffer has changed */
	INTBIG   *maxchars;						/* number of characters in each line */
	CHAR     *formerline;					/* line being changed: former state */
	CHAR    **textarray;					/* image of screen contents */
	INTBIG    savedbox;						/* where the bits are kept for popups */
	CHAR     *header;						/* header text */
	CHAR     *editobjaddr;					/* variable editing: parent object address */
	INTBIG    editobjtype;					/* variable editing: parent object type */
	CHAR     *editobjqual;					/* variable editing: attribute name on parent */
	VARIABLE *editobjvar;					/* variable editing: actual variable */
	struct Ieditor *nexteditor;				/* next in linked list */
} EDITOR;

/******************** WINDOW FRAME MODULES ********************/

#define NOWINDOWFRAME ((WINDOWFRAME *)-1)

#if !defined(USEQT) && defined(ONUNIX)
typedef struct
{
	XImage              *image;				/* the offscreen buffer that matches the visual */
	int                  wid, hei;			/* size of the offscreen buffer */
	INTBIG               depth;				/* the depth of the screen */
	INTBIG               truedepth;			/* the depth of the buffer */
	INTBIG               colorvalue[256];	/* the color map for 8-bit depth */
	UCHAR1              *addr;				/* the data in "world" */
	UCHAR1             **rowstart;			/* pointers to "addr" */
} WINDOWDISPLAY;
#endif

#ifdef USEQT
class GraphicsDraw;
#endif

typedef struct IWindowFrame
{
#ifdef USEQT
	GraphicsDraw        *draw;              /* Qt widget with interior */
#else
#  ifdef MACOS
	GWorldPtr            window;			/* the offscreen buffer for the editing window */
	CGrafPtr             realwindow;		/* the real window */
	void /*Tk_Window*/  *tkwin;				/* the TK window placeholder */
#  endif
#  ifdef WIN32
	void /*CChildFrame*/*wndframe;			/* address of Windows subframe */
	HWND                 realwindow;		/* the real window */
	UCHAR1              *data;				/* raster data for window */
	void /*CDC*/        *hDC;				/* device context for real window */
	HBITMAP              hBitmap;			/* 8-bit offscreen bitmap */
	BITMAPINFO          *bminfo;			/* bit map info structure */
	void /*CFont*/      *hFont;				/* current font in bitmap */
	void /*CDC*/        *hDCOff;			/* device context for the offscreeen bitmap */
	void /*CPalette*/   *hPalette;			/* color information */
	LOGPALETTE          *pColorPalette;		/* color information */
#  endif
#  ifdef ONUNIX
 	Widget               toplevelwidget;	/* widget for top-level frame */
	Widget               intermediategraphics;	/* intermediate graphics widget */
	Widget               graphicswidget;	/* drawing widget */
	Widget               menubar;			/* menu in frame */
	Widget               firstmenu;			/* first menu (dummy placeholder) */
	Display             *topdpy;			/* display of frame */
	WINDOWDISPLAY       *wd;				/* structure with offscreen data */
	Window               topwin;			/* window of top-level frame */
	Window               win;				/* window of graphics widget */
	INTBIG               pulldownmenucount;	/* number of pulldown menus */
	Widget              *pulldownmenus;		/* the current pulldown menu handles */
	INTBIG              *pulldownmenusize;	/* the number of entries in a menu */
	Widget             **pulldownmenulist;	/* the entries in a menu */
	CHAR               **pulldowns;			/* the current pulldown menu names */
	GC                   gc;				/* graphics context for drawing */
	GC                   gcinv;				/* graphics context for inverting */
	GC                   gcstip;			/* graphics context for stippling */
	Colormap             colormap;			/* color map */
	UCHAR1              *dataaddr8;			/* offscreen 8-bit data */
	INTBIG               trueheight;		/* full screen dimensions including status */
#  endif
#endif
	UINTBIG              starttime;			/* time when this window was refreshed from offscreen */
	BOOLEAN              offscreendirty;	/* true if offscreen area is "dirty" */
	INTBIG               copyleft, copyright, copytop, copybottom;/* rectangle to copy to screen */
	UCHAR1             **rowstart;			/* start of each row */
	INTBIG               windindex;			/* identifying index of this frame */
	BOOLEAN              floating;			/* true for floating palette window */
	INTBIG               swid, shei;		/* screen dimensions for drawing */
	INTBIG               revy;				/* for reversing Y coordinates */
	struct IWindowFrame *nextwindowframe;	/* next in list */

	/* variables */
	VARIABLE       *firstvar;				/* first variable in list */
	INTSML          numvar;					/* number of variables in list */
} WINDOWFRAME;

extern WINDOWFRAME  *el_firstwindowframe;
extern WINDOWFRAME  *el_curwindowframe;		/* current window frame */

/******************** WINDOW PARTITION MODULES ********************/

#define NOWINDOWPART      ((WINDOWPART *)-1)

/* the meaning of WINDOWPART->state */
#define GRIDON                     1		/* grid is to be displayed */
#define GRIDTOOSMALL               2		/* grid is too small to display */
#define WINDOWTYPE               070		/* type of this window */
#define DISPWINDOW               010		/* this is a normal display window */
#define TEXTWINDOW               020		/* this is a text editor window */
#define POPTEXTWINDOW            030		/* this is a popup text editor window */
#define WAVEFORMWINDOW           040		/* this is a signal waveform window */
#define DISP3DWINDOW             050		/* this is a 3-D display window */
#define OVERVIEWWINDOW           060		/* this is an overview window */
#define EXPLORERWINDOW           070		/* this is an "explorer" window */
#define HASOWNFRAME             0100		/* set if window was in its own frame */
#define INPLACEQUEUEREDRAW      0200		/* set if in-place window should be redrawn */
#define INPLACEEDIT             0400		/* set if window displays an "edit in place" */
#define SURROUNDINPLACEEDIT    01000		/* set if window displaying surround of an "edit in place" */
#define WINDOWMODE            016000		/* set if window is in a mode */
#define WINDOWSIMMODE          02000		/*    simulation mode */
#define WINDOWTECEDMODE        04000		/*    technology-edit mode */
#define WINDOWOUTLINEEDMODE   010000		/*    outline-edit mode */

#define WINDOWMODEBORDERSIZE       3		/* size of border when in a mode */
#define DISPLAYSLIDERSIZE         12		/* size of sliders on the right and bottom */
#define MAXINPLACEDEPTH           50		/* maximum edit-in-place hierarchical depth */

/* the meaning of the second argument to "WINDOWPART->changehandler" */
#define REPLACETEXTLINE            1
#define DELETETEXTLINE             2
#define INSERTTEXTLINE             3
#define REPLACEALLTEXT             4

typedef struct
{
	float     eye[3];				/* location of the viewer */
	float     view[3];				/* what the viewer is looking at */
	float     up[3];				/* which way is up */
	float     fieldofview;			/* field of view (in degrees) */
	float     nearplane, farplane;	/* near and far clipping planes */
	float     screenx, screeny;		/* screen width and height */
	float     aspect;				/* aspect ratio */
	float     xform[4][4];			/* overall transformation matrix */
} XFORM3D;

typedef struct Iwindowpart
{
	INTBIG              uselx, usehx;		/* X: bounds of window drawing area */
	INTBIG              usely, usehy;		/* Y: bounds of window drawing area */
	INTBIG              screenlx,screenhx;	/* X: low and high of window */
	INTBIG              screenly,screenhy;	/* Y: low and high of window */
	INTBIG              framelx, framehx;	/* X: bounds of window frame (global coords) */
	INTBIG              framely, framehy;	/* Y: bounds of window frame (global coords) */
	INTBIG              thumblx, thumbhx;	/* X: bounds of slider thumb (DISPWINDOW) */
	INTBIG              thumbly, thumbhy;	/* Y: bounds of slider thumb (DISPWINDOW) */
	INTBIG              hratio, vratio;		/* percentage of overall when split */
	float               scalex, scaley;		/* X and Y scale from window to drawing area */
	XFORM3D             xf3;				/* 3D information (if DISP3DWINDOW) */
	NODEPROTO          *curnodeproto;		/* cell in window */
	NODEPROTO          *topnodeproto;		/* in-place edit: top cell in window */
	INTBIG              inplacedepth;		/* in-place edit: depth of stack */
	NODEINST           *inplacestack[MAXINPLACEDEPTH];	/* in-place edit: stack */
	XARRAY              intocell;			/* in-place edit: transformation from screen to cell */
	XARRAY              outofcell;			/* in-place edit: transformation from cell to screen */
	INTBIG              gridx,gridy;		/* size of grid in window */
	INTBIG              state;				/* miscellaneous state bits (see above) */
	CHAR               *location;			/* string describing location */
	WINDOWFRAME        *frame;				/* window frame that contains this */
	EDITOR             *editor;				/* structures for editor in this window */
	void               *expwindow;			/* structures for cell explorer in this window */
	struct Iwindowpart *linkedwindowpart;	/* window associated with this */
	BOOLEAN           (*charhandler)(struct Iwindowpart*, INTSML, INTBIG);					/* routine for characters in window */
	void              (*buttonhandler)(struct Iwindowpart*, INTBIG, INTBIG, INTBIG);		/* routine for buttons in window */
	void              (*changehandler)(struct Iwindowpart*, INTBIG, CHAR*, CHAR*, INTBIG);	/* routine for changes to window */
	void              (*termhandler)(struct Iwindowpart*);									/* routine for termination of window */
	void              (*redisphandler)(struct Iwindowpart*);								/* routine for redisplay of window */
	struct Iwindowpart *nextwindowpart;		/* next window in list */
	struct Iwindowpart *prevwindowpart;		/* previous window in list */

	/* variables */
	VARIABLE           *firstvar;			/* first variable in list */
	INTSML              numvar;				/* number of variables in list */
} WINDOWPART;

extern WINDOWPART *el_topwindowpart;		/* top window in list */
extern WINDOWPART *el_curwindowpart;		/* current window */

/*************************** TECHNOLOGIES ****************************/

/* ===== LAYER DESCRIPTIONS ===== */

/* definition of a color value */
typedef struct Itech_colormap
{
	INTSML       red, green, blue;			/* color values from 0 to 255 */
} TECH_COLORMAP;

/* the meaning of TECH_POLYGON->representation */
#define POINTS                     0		/* list of scalable points */
#define BOX                        1		/* a rectangle */
#define ABSPOINTS                  2		/* list of absolute points */
#define MINBOX                     3		/* minimum sized rectangle */

/* the structure for layers of a node prototype */
typedef struct Itech_polygon
{
	INTSML       layernum;					/* drawing layer in technology */
	INTSML       portnum;					/* the port number in this technology */
	INTSML       count;						/* number of points in polygon */
	INTSML       style;						/* polygon style */
	INTSML       representation;			/* see above list */
	INTBIG      *points;					/* data list */
} TECH_POLYGON;

/* ===== ARC DESCRIPTIONS ===== */

/* the structure for layers of an arc prototype */
typedef struct Itech_arclay
{
	INTBIG       lay;						/* index of this layer */
	INTBIG       off;						/* width offset for this layer */
	INTBIG       style;						/* polygon style */
} TECH_ARCLAY;

/* the structure for an arc prototype */
typedef struct Itech_arcs
{
	CHAR        *arcname;					/* layer name */
	INTBIG       arcwidth;					/* default layer width */
	INTBIG       arcindex;					/* index of this arcinst */
	ARCPROTO    *creation;					/* actual arc prototype created for this */
	INTBIG       laycount;					/* number of layers */
	TECH_ARCLAY *list;						/* list of layers that compose arc */
	UINTBIG      initialbits;				/* initial userbits for this arcproto */
} TECH_ARCS;

/* ===== PORT DESCRIPTIONS ===== */

/* the structure for ports of a node prototype */
typedef struct Itech_ports
{
	INTBIG      *portarcs;					/* allowable arcs (list ends with -1) */
	CHAR        *protoname;					/* name of this port */
	PORTPROTO   *addr;						/* address used by later routines */
	UINTBIG      initialbits;				/* initial userbits for this port */
	INTSML       lowxmul,  lowxsum;			/* defines low X of portinst area */
	INTSML       lowymul,  lowysum;			/* defines low Y of portinst area */
	INTSML       highxmul, highxsum;		/* defines high X of portinst area */
	INTSML       highymul, highysum;		/* defines high Y of portinst area */
} TECH_PORTS;

/* ===== NODE DESCRIPTIONS ===== */

/* the structure for serpentine MOS transistor description */
typedef struct Itech_serpent
{
	TECH_POLYGON basics;					/* the basic information */
	INTSML       lwidth;					/* the extension of width on the left */
	INTSML       rwidth;					/* the extension of width on the right */
	INTSML       extendt;					/* the extension of length on the top end */
	INTSML       extendb;					/* the extension of length on the bottom end */
} TECH_SERPENT;

/* the meaning of TECH_NODES->special */
#define SERPTRANS                  1		/* serpentine transistor */
#define POLYGONAL                  2		/* polygonally defined transistor */
#define MULTICUT                   3		/* multi-cut contact */
#define MOSTRANS                   4		/* MOS transistor (nonserpentine) */

/* the structure for a node prototype */
typedef struct Itech_nodes
{
	CHAR         *nodename;
	INTSML        nodeindex;
	NODEPROTO    *creation;
	INTBIG        xsize, ysize;
	INTSML        portcount;
	TECH_PORTS   *portlist;
	INTSML        layercount;
	TECH_POLYGON *layerlist;
	INTBIG        initialbits;
	INTSML        special;
	INTSML        f1, f2, f3, f4, f5, f6;
	TECH_SERPENT *gra, *ele;
} TECH_NODES;

/* ===== VARIABLE DESCRIPTIONS ===== */

/* the structure for a variable */
typedef struct Itech_variables
{
	CHAR *name;								/* attribute name to set */
	CHAR  *value;							/* initial (address/integer) value for attribute */
	float  fvalue;							/* initial (floating) value for attribute */
	INTBIG type;							/* initial type of attribute */
} TECH_VARIABLES;

/* ===== MAIN STRUCTURE ===== */

#define NOTECHNOLOGY ((TECHNOLOGY *)-1)

/* the meaning of TECHNOLOGY->userbits */
#define NONELECTRICAL             01		/* technology is not electrical  */
#define NODIRECTIONALARCS         02		/* has no directional arcs */
#define NONEGATEDARCS             04		/* has no negated arcs */
#define NONSTANDARD              010		/* nonstandard technology (cannot be edited) */
#define STATICTECHNOLOGY         020		/* statically allocated (don't deallocate memory) */
#define NOPRIMTECHNOLOGY         040		/* no primitives in this technology (don't auto-switch to it) */

/* scaling factor for fixed-point numbers (used in technologies) */
#define WHOLE                    120

typedef struct Itechnology
{
	CHAR               *techname;			/* name of this technology */
	INTBIG              techindex;			/* 0-based index of this technology */
	INTBIG              deflambda;			/* the default size of a unit */
	NODEPROTO          *firstnodeproto;		/* list of primitive nodeprotos */
	ARCPROTO           *firstarcproto;		/* pointer to type description */
	VARIABLE           *firstvar;			/* first variable in list */
	INTSML              numvar;				/* number of variables in list */
	COMCOMP            *parse;				/* parsing structure for tech direction */
	CLUSTER            *cluster;			/* virtual memory cluster for technology */
	CHAR               *techdescript;		/* description of this technology */

	INTBIG              layercount;			/* number of layers */
	GRAPHICS          **layers;				/* layer descriptions */
	INTBIG              arcprotocount;		/* number of arcs */
	TECH_ARCS         **arcprotos;			/* raw data for arcs */
	INTBIG              nodeprotocount;		/* number of nodes */
	TECH_NODES        **nodeprotos;			/* raw data for nodes */
	TECH_VARIABLES     *variables;			/* variable descriptions */

	BOOLEAN           (*init)(struct Itechnology*, INTBIG);			/* process initialization */
	void              (*term)(void);								/* completion */
	void              (*setmode)(INTBIG, CHAR *[]);					/* set operation mode */
	INTBIG            (*request)(CHAR*, va_list);					/* direct call options */

	/* node description */
	INTBIG            (*nodepolys)(NODEINST*, INTBIG*, WINDOWPART*);	/* returns total real polygons in node */
	INTBIG            (*nodeEpolys)(NODEINST*, INTBIG*, WINDOWPART*);	/* returns total electrical polys in node */
	void              (*shapenodepoly)(NODEINST*, INTBIG, POLYGON*);	/* returns real polygon shape in node */
	void              (*shapeEnodepoly)(NODEINST*, INTBIG, POLYGON*);	/* returns electrical polygon shape in node */
	INTBIG            (*allnodepolys)(NODEINST *ni, POLYLIST *plist, WINDOWPART*, BOOLEAN);	/* returns all real polygons in node */
	INTBIG            (*allnodeEpolys)(NODEINST *ni, POLYLIST *plist, WINDOWPART*, BOOLEAN);	/* returns all electrical polygons in node */
	void              (*nodesizeoffset)(NODEINST*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);	/* returns offset to true size */

	/* node port description */
	void              (*shapeportpoly)(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, BOOLEAN);/* returns port shape on a node */

	/* arc description */
	INTBIG            (*arcpolys)(ARCINST*, WINDOWPART*);			/* returns total polygons in arc */
	void              (*shapearcpoly)(ARCINST*, INTBIG, POLYGON*);	/* returns polygon shape in arc */
	INTBIG            (*allarcpolys)(ARCINST *ni, POLYLIST *plist, WINDOWPART*);	/* returns all polygons in arc */
	INTBIG            (*arcwidthoffset)(ARCINST*);					/* returns offset to true width */

	struct Itechnology *nexttechnology;		/* next in linked list */
	UINTBIG             userbits;			/* state flags */
	INTBIG              temp1, temp2;		/* temporaries */
} TECHNOLOGY;

extern TECHNOLOGY *el_technologies;			/* defined in "tectable.c" */
extern TECHNOLOGY *el_curtech;				/* pointer to current technology */
extern TECHNOLOGY *el_curlayouttech;		/* last layout technology referenced */
extern INTBIG      el_maxtech;				/* current number of technologies */

/*************************** CONSTRAINT SYSTEMS ****************************/

#define NOCONSTRAINT ((CONSTRAINT *)-1)

typedef struct Iconstraint
{
	CHAR     *conname;						/* constraint system name */
	CHAR     *condesc;						/* constraint system description */
	COMCOMP  *parse;						/* parsing structure for constraint direction */
	CLUSTER  *cluster;						/* virtual memory cluster for constraint */
	VARIABLE *firstvar;						/* first variable in list */
	INTSML    numvar;						/* number of variables in list */
	void    (*init)(struct Iconstraint*);	/* initialize a constraint system */
	void    (*term)(void);					/* terminate a constraint system */
	void    (*setmode)(INTBIG, CHAR*[]);	/* user-command options */
	INTBIG  (*request)(CHAR*, INTBIG);		/* direct call options */
	void    (*solve)(NODEPROTO*);			/* solve a batch of changes */

	void    (*newobject)(INTBIG,INTBIG);				/* constraint newly created object */
	void    (*killobject)(INTBIG,INTBIG);				/* constraint deleted object */
	BOOLEAN (*setobject)(INTBIG,INTBIG,INTBIG,INTBIG);	/* set constraint properties on object */

	void    (*modifynodeinst)(NODEINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);		/* constrain a modified node instance */
	void    (*modifynodeinsts)(INTBIG,NODEINST**,INTBIG*,INTBIG*,INTBIG*,INTBIG*,INTBIG*,INTBIG*);	/* constrain a modified node instances */
	void    (*modifyarcinst)(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);	/* constrain a modified arc instance */
	void    (*modifyportproto)(PORTPROTO*, NODEINST*, PORTPROTO*);				/* constrain a modified port prototype */
	void    (*modifynodeproto)(NODEPROTO*);										/* constrain a modified node prototype */
	void    (*modifydescript)(INTBIG, INTBIG, INTBIG, UINTBIG*);				/* constrain a modified descriptor */

	void    (*newlib)(LIBRARY*);			/* constrain a newly created library */
	void    (*killlib)(LIBRARY*);			/* constrain a deleted library */

	void    (*newvariable)(INTBIG, INTBIG, INTBIG, INTBIG);						/* constrain a newly created variable */
	void    (*killvariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);	/* constrain a deleted variable */
	void    (*modifyvariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);	/* constrain a modified array variable entry */
	void    (*insertvariable)(INTBIG, INTBIG, INTBIG, INTBIG);					/* constrain an inserted array variable entry */
	void    (*deletevariable)(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);			/* constrain a deleted array variable entry */
	void    (*setvariable)(void);												/* set constraint properties on variable */
} CONSTRAINT;

extern CONSTRAINT *el_curconstraint;		/* current constraint solver */
extern CONSTRAINT  el_constraints[];		/* list of constraint solvers */

/*************************** LISTS ****************************/

/* extendable lists of INTBIG */
typedef struct
{
	INTBIG  *list;			/* the list of INTBIG */
	INTBIG   count;			/* number of entries in the list */
	INTBIG   total;			/* number of entries allocated for the list */
	CLUSTER *cluster;		/* memory cluster for this list */
} LISTINTBIG;

LISTINTBIG *newintlistobj(CLUSTER *clus);
void        killintlistobj(LISTINTBIG *li);
void        clearintlistobj(LISTINTBIG *li);
INTBIG     *getintlistobj(LISTINTBIG *li, INTBIG *count);
BOOLEAN     ensureintlistobj(LISTINTBIG *li, INTBIG needed);
BOOLEAN     addtointlistobj(LISTINTBIG *li, INTBIG value, BOOLEAN unique);


/* extendable strings */
typedef struct
{
	CHAR    *string;		/* the string */
	INTBIG   count;			/* number of characters in the string */
	INTBIG   total;			/* number of characters allocated for the string */
	CLUSTER *cluster;		/* memory cluster for this string */
} STRINGOBJ;

STRINGOBJ *newstringobj(CLUSTER *clus);
void       killstringobj(STRINGOBJ *so);
void       clearstringobj(STRINGOBJ *so);
CHAR      *getstringobj(STRINGOBJ *so);
void       addstringtostringobj(CHAR *string, STRINGOBJ *so);

/*************************** MISCELLANEOUS ****************************/

#define REGISTER            register
#define EPI               3.14159265		/* Pi */
#define NOSTRING        ((CHAR *)-1)
#define FARTEXTLIMIT              20		/* distance (in lambda) of "far text" */
#define DEFTECH             x_("mocmos")		/* default technology */
#define DEFCONSTR           x_("layout")		/* default constraint system */
#define MINIMUMTEXTSIZE            6		/* minimum text size (smaller text not drawn) */

/* the sounds available to "ttybeep()" */
#define SOUNDBEEP                  0		/* standard "beep" sound */
#define SOUNDCLICK                 1		/* a subtle "click" sound for wiring */

/* the meaning of "el_units" */
#define DISPLAYUNITS             017		/* the display units */
#define DISPUNITLAMBDA             0		/*   lambda display (12.3) */
#define DISPUNITINCH              01		/*   inch display (12.3") */
#define DISPUNITCM                02		/*   centimeter display (12.3cm) */
#define DISPUNITMM                04		/*   millimeter display (12.3mm) */
#define DISPUNITMIL              010		/*   mil display (12.3mil) */
#define DISPUNITMIC              011		/*   micron display (12.3u) */
#define DISPUNITCMIC             012		/*   centimicron display (12.3cu) */
#define DISPUNITNM               013		/*   nanometer (millimicron) display (12.3nm) */
#define INTERNALUNITS           0160		/* the internal units */
#define INTERNALUNITSSH            4		/* right shift for INTERNALUNITS */
#define INTUNITHNM                 0		/*   half-nanometer (half millimicron) units */
#define INTUNITHDMIC             020		/*   half-decimicron units */

/* the meaning of "USER_electrical_units" on USER tool and LIBRARY */
#define INTERNALRESUNITS          017		/* the internal resistance units */
#define INTERNALRESUNITSSH          0		/* right shift for INTERNALRESUNITS */
#define INTRESUNITOHM               0		/*   ohm */
#define INTRESUNITKOHM              1		/*   kilo-ohm */
#define INTRESUNITMOHM              2		/*   mega-ohm */
#define INTRESUNITGOHM              3		/*   giga-ohm */
#define INTERNALCAPUNITS         0360		/* the internal capacitance units */
#define INTERNALCAPUNITSSH          4		/* right shift for INTERNALCAPUNITS */
#define INTCAPUNITFARAD             0		/*   farad */
#define INTCAPUNITMFARAD            1		/*   milli-farad */
#define INTCAPUNITUFARAD            2		/*   micro-farad */
#define INTCAPUNITNFARAD            3		/*   nano-farad */
#define INTCAPUNITPFARAD            4		/*   pico-farad */
#define INTCAPUNITFFARAD            5		/*   femto-farad */
#define INTERNALINDUNITS        07400		/* the internal inductance units */
#define INTERNALINDUNITSSH          8		/* right shift for INTERNALINDUNITS */
#define INTINDUNITHENRY             0		/*   henry */
#define INTINDUNITMHENRY            1		/*   milli-henry */
#define INTINDUNITUHENRY            2		/*   micro-henry */
#define INTINDUNITNHENRY            3		/*   nano-henry */
#define INTERNALCURUNITS      0170000		/* the internal current units */
#define INTERNALCURUNITSSH         12		/* right shift for INTERNALCURUNITS */
#define INTCURUNITAMP               0		/*   amp */
#define INTCURUNITMAMP              1		/*   milli-amp */
#define INTCURUNITUAMP              2		/*   micro-amp */
#define INTERNALVOLTUNITS    03600000		/* the internal voltage units */
#define INTERNALVOLTUNITSSH        16		/* right shift for INTERNALVOLTUNITS */
#define INTVOLTUNITKVOLT            0		/*   kilo-volt */
#define INTVOLTUNITVOLT             1		/*   volt */
#define INTVOLTUNITMVOLT            2		/*   milli-volt */
#define INTVOLTUNITUVOLT            3		/*   micro-volt */
#define INTERNALTIMEUNITS   074000000		/* the internal time units */
#define INTERNALTIMEUNITSSH        20		/* right shift for INTERNALTIMEUNITS */
#define INTTIMEUNITSEC              0		/*   second */
#define INTTIMEUNITMSEC             1		/*   milli-second */
#define INTTIMEUNITUSEC             2		/*   micro-second */
#define INTTIMEUNITNSEC             3		/*   nano-second */
#define INTTIMEUNITPSEC             4		/*   pico-second */
#define INTTIMEUNITFSEC             5		/*   femto-second */
#define ELEUNITDEFAULT          (INTRESUNITOHM<<INTERNALRESUNITSSH)|\
								(INTCAPUNITPFARAD<<INTERNALCAPUNITSSH)|\
								(INTINDUNITNHENRY<<INTERNALINDUNITSSH)|\
								(INTCURUNITMAMP<<INTERNALCURUNITSSH)|\
								(INTVOLTUNITVOLT<<INTERNALVOLTUNITSSH)|\
								(INTTIMEUNITSEC<<INTERNALTIMEUNITSSH)

/*
 * reasons for aborting (the argument to "stopping()")
 * (update table "dbtext.c:db_stoppingreason[]" when changing this)
 */
#define STOPREASONCONTOUR          0		/* "Contour gather" */
#define STOPREASONDRC              1		/* "DRC" */
#define STOPREASONPLAYBACK         2		/* "Playback" */
#define STOPREASONBINARY           3		/* "Binary" */
#define STOPREASONCIF              4		/* "CIF" */
#define STOPREASONDXF              5		/* "DXF" */
#define STOPREASONEDIF             6		/* "EDIF" */
#define STOPREASONVHDL             7		/* "VHDL" */
#define STOPREASONCOMPACT          8		/* "Compaction" */
#define STOPREASONERC              9		/* "ERC" */
#define STOPREASONCHECKIN         10		/* "Check-in" */
#define STOPREASONLOCK            11		/* "Lock wait" */
#define STOPREASONNCC             12		/* "Network comparison" */
#define STOPREASONPORT            13		/* "Port exploration" */
#define STOPREASONROUTING         14		/* "Routing" */
#define STOPREASONSILCOMP         15		/* "Silicon compiler" */
#define STOPREASONDISPLAY         16		/* "Display" */
#define STOPREASONSIMULATE        17		/* "Simulation" */
#define STOPREASONDECK            18		/* "Deck generation" */
#define STOPREASONSPICE           19		/* "SPICE" */
#define STOPREASONCHECK           20		/* "Check" */
#define STOPREASONARRAY           21		/* "Array" */
#define STOPREASONITERATE         22		/* "Iteration" */
#define STOPREASONREPLACE         23		/* "Replace" */
#define STOPREASONSPREAD          24		/* "Spread" */
#define STOPREASONEXECUTE         25		/* "Execution" */
#define STOPREASONCOMFILE         26		/* "Command file" */
#define STOPREASONSELECT          27		/* "Selection" */
#define STOPREASONTRACK           28		/* "Tracking" */
#define STOPREASONNETWORK         29		/* "Network evaluation" */

/* special key meanings, found in "ttygetchar()", "getnxtchar()", "getbuckybits()" */
#define SHIFTDOWN                 01		/* set if Shift held down */
#define CONTROLDOWN               02		/* set if Control held down */
#define OPTALTMETDOWN             04		/* set if Option/Alt/Meta held down */
#define COMMANDDOWN              010		/* set if Command held down (Mac only) */
#define ACCELERATORDOWN          020		/* set if accelerator key (Command or Control) held down */
#define SPECIALKEYDOWN           040		/* set if this is a special key */
#define SPECIALKEY             07700		/* the special key typed */
#define SPECIALKEYSH               6		/* right-shift of SPECIALKEY */
#define SPECIALKEYF1               1		/*    Function key F1 */
#define SPECIALKEYF2               2		/*    Function key F2 */
#define SPECIALKEYF3               3		/*    Function key F3 */
#define SPECIALKEYF4               4		/*    Function key F4 */
#define SPECIALKEYF5               5		/*    Function key F5 */
#define SPECIALKEYF6               6		/*    Function key F6 */
#define SPECIALKEYF7               7		/*    Function key F7 */
#define SPECIALKEYF8               8		/*    Function key F8 */
#define SPECIALKEYF9               9		/*    Function key F9 */
#define SPECIALKEYF10             10		/*    Function key F10 */
#define SPECIALKEYF11             11		/*    Function key F11 */
#define SPECIALKEYF12             12		/*    Function key F12 */
#define SPECIALKEYARROWL          13		/*    Left arrow key */
#define SPECIALKEYARROWR          14		/*    Right arrow key */
#define SPECIALKEYARROWU          15		/*    Up arrow key */
#define SPECIALKEYARROWD          16		/*    Down arrow key */
#define SPECIALEND                17		/*    END key */
#define SPECIALCUT                18		/*    Cut key */
#define SPECIALCOPY               19		/*    Copy key */
#define SPECIALPASTE              20		/*    Paste key */
#define SPECIALUNDO               21		/*    Undo key */

/*
 * the "filetype" parameter in "multifileselectin", "describefiletype",
 * "setupfiletype", "getfiletype", "xopen", "xcreate"
 */
#define FILETYPE               01777		/* type of file */
#define FILETYPEWRITE          02000		/* bit set to write file (instead of read) */
#define FILETYPEAPPEND         04000		/* bit set to append to file (instead of read) */
#define FILETYPETEMPFILE      010000		/* bit set to generate temp filename (write only) */

/* the "purpose" parameter (cursor shape) for "trackcursor()" */
#define TRACKNORMAL                0		/* standard cursor */
#define TRACKDRAWING               1		/* pen cursor */
#define TRACKDRAGGING              2		/* hand cursor */
#define TRACKSELECTING             3		/* horizontal arrow cursor */
#define TRACKHSELECTING            4		/* arrow cursor */

extern XARRAY      el_matid;				/* identity matrix */
extern INTBIG      el_pleasestop;			/* nonzero if abort is requested */
extern INTBIG      el_units;				/* display and internal units */
extern CHAR       *el_libdir;				/* pointer to library directory */
extern CHAR       *el_version;				/* current version string */
extern INTBIG      el_filetypetext;			/* plain text disk file descriptor */

/*************************** DECLARATIONS ****************************/

/* node routines */
NODEINST    *newnodeinst(NODEPROTO *proto, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy,
				INTBIG transposition, INTBIG rotation, NODEPROTO *parent);
BOOLEAN      killnodeinst(NODEINST *ni);
void         defaultnodesize(NODEPROTO *np, INTBIG *xs, INTBIG *ys);
void         modifynodeinst(NODEINST *node, INTBIG Dlx, INTBIG Dly, INTBIG Dhx,
				INTBIG Dhy, INTBIG Drotation, INTBIG Dtransposition);
void         modifynodeinsts(INTBIG count, NODEINST **nis, INTBIG *deltalxs, INTBIG *deltalys,
				INTBIG *deltahxs, INTBIG *deltahys, INTBIG *deltarots, INTBIG *deltatranss);
NODEINST    *replacenodeinst(NODEINST *oldnode, NODEPROTO *newproto, BOOLEAN ignoreportnames,
				BOOLEAN allowdeletedports);
INTBIG       nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win);
void         shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly);
INTBIG       allnodepolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN reasonable);
INTBIG       nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win);
void         shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly);
INTBIG       allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN reasonable);
void         nodesizeoffset(NODEINST *ni, INTBIG *lowx, INTBIG *lowy, INTBIG *highx, INTBIG *highy);
void         nodeprotosizeoffset(NODEPROTO *np, INTBIG *lowx, INTBIG *lowy, INTBIG *highx, INTBIG *highy, NODEPROTO *parent);
INTBIG       nodefunction(NODEINST *ni);
CHAR        *nodefunctionname(INTBIG function, NODEINST *ni);
CHAR        *nodefunctionshortname(INTBIG function);
CHAR        *nodefunctionconstantname(INTBIG function);
CHAR        *describenodeinst(NODEINST *ni);
CHAR        *ntdescribenodeinst(NODEINST *ni);
void         transistorsize(NODEINST *ni, INTBIG *length, INTBIG *width);
void         getarcdegrees(NODEINST *ni, double *startoffset, double *endangle);
void         setarcdegrees(NODEINST *ni, double startoffset, double endangle);
void         getarcendpoints(NODEINST *ni, double startoffset, double endangle, INTBIG *fx, INTBIG *fy,
				INTBIG *tx, INTBIG *ty);
void         transistorports(NODEINST *ni, PORTPROTO **gateleft, PORTPROTO **gateright,
				PORTPROTO **activetop, PORTPROTO **activebottom);
INTBIG       lambdaofnode(NODEINST *ni);
VARIABLE    *gettrace(NODEINST *ni);
void         corneroffset(NODEINST *ni, NODEPROTO *proto, INTBIG rotation, INTBIG transpose,
				INTBIG *xoff, INTBIG *yoff, BOOLEAN centerbased);
void         initdummynode(NODEINST *ni);
BOOLEAN      isfet(GEOM *pos);
NODEPROTO   *getpurelayernode(TECHNOLOGY *tech, INTBIG layer, INTBIG function);
void         drcminnodesize(NODEPROTO *np, LIBRARY *lib, INTBIG *sizex, INTBIG *sizey, CHAR **rule);
BOOLEAN      invisiblepinwithoffsettext(NODEINST *ni, INTBIG *x, INTBIG *y, BOOLEAN repair);
BOOLEAN      isinlinepin(NODEINST *ni, ARCINST ***ailist);

/* arc routines */
ARCINST     *newarcinst(ARCPROTO *proto, INTBIG width, INTBIG initialuserbits, NODEINST *nodeA,
				PORTPROTO *portA, INTBIG xA, INTBIG yA, NODEINST *nodeB, PORTPROTO *portB,
				INTBIG xB, INTBIG yB, NODEPROTO *parent);
BOOLEAN      killarcinst(ARCINST *arc);
INTBIG       defaultarcwidth(ARCPROTO *ap);
BOOLEAN      modifyarcinst(ARCINST *arc, INTBIG Dwidth, INTBIG Dx1, INTBIG Dy1, INTBIG Dx2, INTBIG Dy2);
ARCINST     *aconnect(GEOM *fromgeom, PORTPROTO *fromport, GEOM *togeom, PORTPROTO *toport,
				ARCPROTO *arc, INTBIG x, INTBIG y, ARCINST **arc2, ARCINST **arc3,
				NODEINST **con1, NODEINST **con2, INTBIG ang, BOOLEAN nozigzag, BOOLEAN report);
ARCINST     *replacearcinst(ARCINST *oldarc, ARCPROTO *newproto);
INTBIG       arcpolys(ARCINST *arc, WINDOWPART *win);
void         shapearcpoly(ARCINST *arc, INTBIG box, POLYGON *poly);
INTBIG       allarcpolys(ARCINST *arc, POLYLIST *plist, WINDOWPART *win);
CHAR        *arcfunctionname(INTBIG function);
CHAR        *describearcinst(ARCINST *arc);
CHAR        *describearcproto(ARCPROTO *arcproto);
ARCPROTO    *getarcproto(CHAR *specification);
INTBIG       arcwidthoffset(ARCINST *arc);
INTBIG       arcprotowidthoffset(ARCPROTO *arc);
NODEPROTO   *getpinproto(ARCPROTO *arc);
void         makearcpoly(INTBIG length, INTBIG width, ARCINST *arc, POLYGON *poly, INTBIG style);
BOOLEAN      curvedarcoutline(ARCINST *arc, POLYGON *poly, INTBIG style, INTBIG width);
INTBIG       setshrinkvalue(ARCINST *arc, BOOLEAN extend);
void         determineangle(ARCINST *arc);
INTBIG       lambdaofarc(ARCINST *ai);
BOOLEAN      arcconnects(INTBIG ang, INTBIG lx1, INTBIG hx1, INTBIG ly1, INTBIG hy1, INTBIG lx2, INTBIG hx2,
				INTBIG ly2, INTBIG hy2, INTBIG *x1, INTBIG *y1, INTBIG *x2, INTBIG *y2);
void         initdummyarc(ARCINST *ai);
ARCPROTO    *getarconlayer(INTBIG layer, TECHNOLOGY *tech);

/* cell routines */
NODEPROTO   *newnodeproto(CHAR *name, LIBRARY *library);
BOOLEAN      killnodeproto(NODEPROTO *cell);
NODEPROTO   *copynodeproto(NODEPROTO *originalcell, LIBRARY *destlibrary, CHAR *name, BOOLEAN useexisting);
NODEPROTO   *getnodeproto(CHAR *specification);
CHAR        *describenodeproto(NODEPROTO *nodeproto);
CHAR        *nldescribenodeproto(NODEPROTO *nodeproto);
void         grabpoint(NODEPROTO *nodeproto, INTBIG *grabx, INTBIG *graby);
BOOLEAN      isachildof(NODEPROTO *parent, NODEPROTO *child);
INTBIG       framesize(INTBIG *x, INTBIG *y, NODEPROTO *np);
INTBIG       framepolys(NODEPROTO *np);
void         framepoly(INTBIG box, POLYGON *poly, NODEPROTO *np);
void         begintraversehierarchy(void);
void         endtraversehierarchy(void);
void         downhierarchy(NODEINST *ni, NODEPROTO *np, INTBIG index);
void         uphierarchy(void);
void         popouthierarchy(INTBIG climb);
INTBIG       getpopouthierarchy(void);
void         sethierarchicalparent(NODEPROTO *np, NODEINST *parentni, WINDOWPART *win, INTBIG thisindex, INTBIG *viewinfo);
void         gettraversalpath(NODEPROTO *here, WINDOWPART *win, NODEINST ***nilist, INTBIG **indexlist, INTBIG *depth, INTBIG maxdepth);
NODEPROTO   *getcurcell(void);
INTBIG       lambdaofcell(NODEPROTO *np);
BOOLEAN      hasresistors(NODEPROTO *np);
NODEINST    *descentparent(NODEPROTO *np, INTBIG *index, WINDOWPART *win, INTBIG *viewinfo);
void        *newhierarchicaltraversal(void);
void         gethierarchicaltraversal(void *snapshot);
void         sethierarchicaltraversal(void *snapshot);
void         killhierarchicaltraversal(void *snapshot);

/* cell routines */
void         initparameterizedcells(void);
CHAR        *inparameterizedcells(NODEPROTO *np, CHAR *parname);
void         addtoparameterizedcells(NODEPROTO *np, CHAR *parname, CHAR *shortname);
CHAR        *parameterizedname(NODEINST *ni, CHAR *cellname);

/* export routines */
PORTPROTO   *newportproto(NODEPROTO *cell, NODEINST *nodeincell, PORTPROTO *portinnode, CHAR *name);
BOOLEAN      killportproto(NODEPROTO *cell, PORTPROTO *port);
BOOLEAN      moveportproto(NODEPROTO *cell, PORTPROTO *port, NODEINST *newnode, PORTPROTO *newnodeport);
PORTPROTO   *getportproto(NODEPROTO *cell, CHAR *portname);
void         shapeportpoly(NODEINST *node, PORTPROTO *port, POLYGON *poly, BOOLEAN purpose);
void         shapetransportpoly(NODEINST *node, PORTPROTO *port, POLYGON *poly, XARRAY trans);
void         portposition(NODEINST *node, PORTPROTO *port, INTBIG *x, INTBIG *y);
void         reduceportpoly(POLYGON *poly, NODEINST *node, PORTPROTO *port, INTBIG width, INTBIG angle);
PORTPROTO   *equivalentport(NODEPROTO *cell, PORTPROTO *port, NODEPROTO *equivalentcell);
BOOLEAN      portispower(PORTPROTO *port);
BOOLEAN      portisground(PORTPROTO *port);
BOOLEAN      portisnamedpower(PORTPROTO *port);
BOOLEAN      portisnamedground(PORTPROTO *port);
void         changeallports(PORTPROTO *port);
CHAR        *describeportbits(INTBIG userbits);

/* view routines */
VIEW        *newview(CHAR *viewname, CHAR *abbreviation);
BOOLEAN      killview(VIEW *view);
BOOLEAN      changecellview(NODEPROTO *cell, VIEW *view);
VIEW        *getview(CHAR *specification);
NODEPROTO   *contentsview(NODEPROTO *cell);
NODEPROTO   *iconview(NODEPROTO *cell);
BOOLEAN     insamecellgrp(NODEPROTO *a, NODEPROTO *b);
BOOLEAN      isiconof(NODEPROTO *subnp, NODEPROTO *cell);
NODEPROTO   *layoutview(NODEPROTO *cell);
NODEPROTO   *anyview(NODEPROTO *cell, VIEW *v);
BOOLEAN      isschematicview(NODEPROTO *cell);
BOOLEAN      isiconview(NODEPROTO *cell);

/* window routines */
WINDOWPART  *newwindowpart(CHAR *location, WINDOWPART *protowindow);
void         killwindowpart(WINDOWPART *window);
void         copywindowpart(WINDOWPART *dwindow, WINDOWPART *swindow);
INTBIG       truefontsize(INTBIG font, WINDOWPART *window, TECHNOLOGY *tech);
void         computewindowscale(WINDOWPART *window);
INTBIG       applyxscale(WINDOWPART *window, INTBIG value);
INTBIG       applyyscale(WINDOWPART *window, INTBIG value);

/* variable routines */
VARIABLE    *getval(INTBIG addr, INTBIG  type, INTBIG want, CHAR *name);
VARIABLE    *getvalkey(INTBIG addr, INTBIG type, INTBIG want, INTBIG key);
VARIABLE    *getvalnoeval(INTBIG addr, INTBIG  type, INTBIG want, CHAR *name);
VARIABLE    *getvalkeynoeval(INTBIG addr, INTBIG type, INTBIG want, INTBIG key);
VARIABLE    *evalvar(VARIABLE *var, INTBIG addr, INTBIG type);
VARIABLE    *getparentval(CHAR *name, INTBIG height);
VARIABLE    *getparentvalkey(INTBIG key, INTBIG height);
BOOLEAN      parentvaldefaulted(void);
WINDOWPART  *setvariablewindow(WINDOWPART *win);
BOOLEAN      getcurrentvariableenvironment(INTBIG *addr, INTBIG *type, WINDOWPART **win);
BOOLEAN      getlastvariableobject(INTBIG *addr, INTBIG *type);
#ifdef DEBUGMEMORY
#  define    setval(a, t, n, na, nt) _setval((a), (t), (n), (na), (nt), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
  VARIABLE *_setval(INTBIG addr, INTBIG type, CHAR *name, INTBIG newaddr, INTBIG newtype, CHAR*, INTBIG);
#  define    setvalkey(a, t, k, na, nt) _setvalkey((a), (t), (k), (na), (nt), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
  VARIABLE *_setvalkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG newaddr, INTBIG newtype, CHAR*, INTBIG);
#  define    setind(a, t, n, i, na) _setind((a), (t), (n), (i), (na), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
  BOOLEAN   _setind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG newaddr, CHAR*, INTBIG);
#  define    setindkey(a, t, k, i, na) _setindkey((a), (t), (k), (i), (na), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
  BOOLEAN   _setindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG newaddr, CHAR*, INTBIG);
#else
  VARIABLE  *setval(INTBIG addr, INTBIG type, CHAR *name, INTBIG newaddr, INTBIG newtype);
  VARIABLE  *setvalkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG newaddr, INTBIG newtype);
  BOOLEAN    setind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG newaddr);
  BOOLEAN    setindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG newaddr);
#endif
BOOLEAN      delval(INTBIG addr, INTBIG type, CHAR *name);
BOOLEAN      delvalkey(INTBIG addr, INTBIG type, INTBIG key);
BOOLEAN      getind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG *value);
BOOLEAN      getindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG *value);
BOOLEAN      insind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex, INTBIG newaddr);
BOOLEAN      insindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex, INTBIG newaddr);
BOOLEAN      delind(INTBIG addr, INTBIG type, CHAR *name, INTBIG aindex);
BOOLEAN      delindkey(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex);
void         renameval(CHAR *oldname, CHAR *newname);
INTBIG       makekey(CHAR *name);
CHAR        *makename(INTBIG key);
INTBIG       initobjlist(INTBIG addr, INTBIG type, BOOLEAN restrictdirect);
CHAR        *nextobjectlist(VARIABLE **var, INTBIG search);
BOOLEAN      copyvars(INTBIG fromaddr, INTBIG fromtype, INTBIG toaddr, INTBIG totype, BOOLEAN uniquenames);
void         modifydescript(INTBIG addr, INTBIG type, VARIABLE *var, UINTBIG *newdescript);
INTBIG       getlength(VARIABLE *var);
CHAR        *describeobject(INTBIG addr, INTBIG type);
CHAR        *describevariable(VARIABLE *var, INTBIG aindex, INTBIG purpose);
CHAR        *describedisplayedvariable(VARIABLE *var, INTBIG aindex, INTBIG purpose);
CHAR        *describesimplevariable(VARIABLE *var);
void         getsimpletype(CHAR *pt, INTBIG *type, INTBIG *addr, INTBIG units);
void         cacheoptionbitvalues(void);
void         makeoptionstemporary(LIBRARY *lib);
void         restoreoptionstate(LIBRARY *lib);
BOOLEAN      isoptionvariable(INTBIG addr, INTBIG type, CHAR *name);
BOOLEAN      optionshavechanged(void);
BOOLEAN      describeoptions(INTBIG aindex, CHAR **name, INTBIG *bits);
void         explainoptionchanges(INTBIG item, void *dia);
void         explainsavedoptions(INTBIG item, BOOLEAN onlychanged, void *dia);
CHAR        *truevariablename(VARIABLE *var);
void         listalloptions(INTBIG item, void *dia);
BOOLEAN      isdeprecatedvariable(INTBIG addr, INTBIG type, CHAR *name);

/* change control routines */
void         startobjectchange(INTBIG address, INTBIG type);
void         endobjectchange(INTBIG address, INTBIG type);
INTBIG       undonature(BOOLEAN undo);
INTBIG       undoabatch(TOOL **tool);
INTBIG       redoabatch(TOOL **tool);
void         noundoallowed(void);
void         noredoallowed(void);
INTBIG       historylistsize(INTBIG newsize);
void         setactivity(CHAR *message);
void         showhistorylist(INTBIG batch);
CHAR        *changedvariablename(INTBIG type, INTBIG key, INTBIG subtype);
INTBIG       getcurrentbatchnumber(void);
void         nextchangequiet(void);
void         changesquiet(BOOLEAN quiet);

/* layer routines */
CHAR        *layername(TECHNOLOGY *tech, INTBIG layer);
INTBIG       layerfunction(TECHNOLOGY *tech, INTBIG layer);
BOOLEAN      layerismetal(INTBIG fun);
BOOLEAN      layerispoly(INTBIG fun);
BOOLEAN      layerisgatepoly(INTBIG fun);
BOOLEAN      layeriscontact(INTBIG fun);
INTBIG       layerfunctionheight(INTBIG layerfunct);
BOOLEAN      samelayer(TECHNOLOGY *tech, INTBIG layer1, INTBIG layer2);
INTBIG       nonpseudolayer(INTBIG layer, TECHNOLOGY *tech);
INTBIG       maxdrcsurround(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer);
INTBIG       drcmindistance(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer1, INTBIG size1,
				INTBIG layer2, INTBIG size2, BOOLEAN connected, BOOLEAN multicut, INTBIG *edge,
				CHAR **rule);
INTBIG       drcminwidth(TECHNOLOGY *tech, LIBRARY *lib, INTBIG layer, CHAR **rule);
INTBIG       getecolor(CHAR *colorname);
BOOLEAN      ecolorname(INTBIG color, CHAR **colorname, CHAR **colorsymbol);
BOOLEAN      get3dfactors(TECHNOLOGY *tech, INTBIG layer, float *height, float *thickness);
void         set3dheight(TECHNOLOGY *tech, float *depth);
void         set3dthickness(TECHNOLOGY *tech, float *thickness);

/* geometric routines */
INTBIG       initsearch(INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, NODEPROTO *cell);
GEOM        *nextobject(INTBIG initsearchvalue);
void         termsearch(INTBIG initsearchvalue);
void         boundobj(GEOM *object, INTBIG *lowx, INTBIG *highx, INTBIG *lowy, INTBIG *highy);
CHAR        *geomname(GEOM *geom);
NODEPROTO   *geomparent(GEOM *geom);
void         linkgeom(GEOM *geom, NODEPROTO *cell);
void         undogeom(GEOM *geom, NODEPROTO *cell);
void         updategeom(GEOM *geom, NODEPROTO *cell);
BOOLEAN      geomstructure(NODEPROTO *cell);

/* library routines */
LIBRARY     *getlibrary(CHAR *libname);
LIBRARY     *newlibrary(CHAR *name, CHAR *diskfile);
void         selectlibrary(LIBRARY *library, BOOLEAN changelambda);
void         killlibrary(LIBRARY *library);
void         eraselibrary(LIBRARY *library);
LIBRARY     *whichlibrary(INTBIG, INTBIG);

/* technology routines */
TECHNOLOGY  *gettechnology(CHAR *techname);
TECHNOLOGY  *whattech(NODEPROTO *cell);
void         addtechnology(TECHNOLOGY *technology);
BOOLEAN      killtechnology(TECHNOLOGY *technology);
void         registertechnologycache(void (*routine)(void), INTBIG count, INTBIG *variablekeys);
void         changedtechnologyvariable(INTBIG key);
void         telltech(TECHNOLOGY *technology, INTBIG count, CHAR *par[]);
INTBIG       asktech(TECHNOLOGY *technology, CHAR *command, ...);
TECHNOLOGY  *defschematictechnology(TECHNOLOGY *deftech);

/* tool routines */
void         toolturnon(TOOL *tool);
void         toolturnoff(TOOL *tool, BOOLEAN permanently);
void         telltool(TOOL *tool, INTBIG count, CHAR *args[]);
INTBIG       asktool(TOOL *tool, CHAR *command, ...);
TOOL        *gettool(CHAR *toolname);

/* transformations and mathematics */
void         makeangle(INTBIG rotation, INTBIG transposition, XARRAY matrix);
INTBIG       sine(INTBIG angle);
INTBIG       cosine(INTBIG angle);
INTBIG       pickprime(INTBIG start);
INTBIG       getprime(INTBIG n);
void         makerot(NODEINST *node, XARRAY matrix);
void         makerotI(NODEINST *node, XARRAY matrix);
void         maketrans(NODEINST *node, XARRAY matrix);
void         maketransI(NODEINST *node, XARRAY matrix);
void         transid(XARRAY matrix);
void         transcpy(XARRAY matsource, XARRAY matdest);
void         transmult(XARRAY mata, XARRAY matb, XARRAY matdest);
BOOLEAN      ismanhattan(XARRAY trans);
void         xform(INTBIG x, INTBIG y, INTBIG *newx, INTBIG *newy, XARRAY matrix);
void         xformbox(INTBIG*, INTBIG*, INTBIG*, INTBIG*, XARRAY);
INTBIG       muldiv(INTBIG a, INTBIG b, INTBIG c);
INTBIG       mult(INTBIG a, INTBIG b);
INTBIG       mini(INTBIG a, INTBIG b);
INTBIG       maxi(INTBIG a, INTBIG b);
INTBIG       roundfloat(float v);
INTBIG       rounddouble(double v);
BOOLEAN      floatsequal(float a, float b);
BOOLEAN      doublesequal(double a, double b);
BOOLEAN      floatslessthan(float a, float b);
BOOLEAN      doubleslessthan(double a, double b);
float        floatfloor(float a);
double       doublefloor(double a);
INTBIG       castint(float f);
float        castfloat(INTBIG i);
INTBIG       intsqrt(INTBIG v);
INTBIG       scalefromdispunit(float value, INTBIG dispunit);
float        scaletodispunit(INTBIG value, INTBIG dispunit);
float        scaletodispunitsq(INTBIG value, INTBIG dispunit);
INTBIG       computedistance(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
INTBIG       figureangle(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
double       ffigureangle(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
INTBIG       intersect(INTBIG x1, INTBIG y1, INTBIG angle1, INTBIG x2, INTBIG y2, INTBIG angle2,
				INTBIG *x, INTBIG *y);
INTBIG       fintersect(INTBIG x1, INTBIG y1, double fang1, INTBIG x2, INTBIG y2,
				double fang2, INTBIG *x, INTBIG *y);
BOOLEAN      segintersect(INTBIG fx1, INTBIG fy1, INTBIG tx1, INTBIG ty1,
				INTBIG fx2, INTBIG fy2, INTBIG tx2, INTBIG ty2, INTBIG *ix, INTBIG *iy);
BOOLEAN      isonline(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
INTBIG       disttoline(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
INTBIG       closestpointtosegment(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG *x, INTBIG *y);
BOOLEAN      findcenters(INTBIG r, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG d,
				INTBIG *ix1, INTBIG *iy1, INTBIG *ix2, INTBIG *iy2);
INTBIG       circlelineintersection(INTBIG icx, INTBIG icy, INTBIG isx, INTBIG isy, INTBIG lx1,
				INTBIG ly1, INTBIG lx2, INTBIG ly2, INTBIG *ix1, INTBIG *iy1, INTBIG *ix2, INTBIG *iy2,
				INTBIG tolerance);
BOOLEAN      circletangents(INTBIG x, INTBIG y, INTBIG cx, INTBIG cy, INTBIG sx, INTBIG sy,
				INTBIG *ix1, INTBIG *iy1, INTBIG *ix2, INTBIG *iy2);
void         arcbbox(INTBIG xs, INTBIG ys, INTBIG xe, INTBIG ye, INTBIG xc, INTBIG yc,
				INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
BOOLEAN      clipline(INTBIG *fx, INTBIG *fy, INTBIG *tx, INTBIG *ty, INTBIG lx, INTBIG hx,
				INTBIG ly, INTBIG hy);
void         cliparc(POLYGON *in, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
void         subrange(INTBIG low, INTBIG high, INTBIG lowmul, INTBIG lowsum, INTBIG highmul, INTBIG highsum,
				INTBIG *newlow, INTBIG *newhigh, INTBIG lambda);
BOOLEAN      polyinrect(POLYGON *poly, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
void         clippoly(POLYGON *poly, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
void         closestpoint(POLYGON *poly, INTBIG *x, INTBIG *y);
INTBIG       getrange(INTBIG low, INTBIG high, INTBIG mul,INTBIG sum, INTBIG lambda);
INTBIG       rotatelabel(INTBIG oldstyle, INTBIG rotation, XARRAY trans);
INTBIG       cropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
				INTBIG uy);
void         arctopoly(INTBIG cx, INTBIG cy, INTBIG sx, INTBIG sy, INTBIG ex, INTBIG ey, POLYGON *poly,
				INTBIG arcres, INTBIG arcsag);
void         circletopoly(INTBIG cx, INTBIG cy, INTBIG radius, POLYGON *poly, INTBIG arcres, INTBIG arcsag);
void         adjustdisoffset(INTBIG addr, INTBIG type, TECHNOLOGY *tech, POLYGON *poly, UINTBIG *textdescription);
void         vectoradd3d(float *a, float *b, float *sum);
void         vectorsubtract3d(float *a, float *b, float *diff);
void         vectormultiply3d(float *a, float b, float *res);
void         vectordivide3d(float *a, float b, float *res);
float        vectormagnitude3d(float *a);
float        vectordot3d(float *a, float *b);
void         vectornormalize3d(float *a);
void         vectorcross3d(float *a, float *b, float *res);
void         matrixid3d(float xform[4][4]);
void         matrixmult3d(float a[4][4], float b[4][4], float res[4][4]);
void         matrixinvert3d(float mats[4][4], float matd[4][4]);
void         matrixxform3d(float *vec, float mat[4][4], float *res);

/* polygon routines */
#ifdef DEBUGMEMORY
  POLYGON  *_allocpolygon(INTBIG points, CLUSTER *cluster, CHAR*, INTBIG);
  POLYLIST *_allocpolylist(CLUSTER *cluster, CHAR*, INTBIG);
  BOOLEAN   _ensurepolylist(POLYLIST *list, INTBIG tot, CLUSTER *cluster, CHAR*, INTBIG);
#  define    allocpolygon(p, c) _allocpolygon((p), (c), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
#  define    allocpolylist(c) _allocpolylist((c), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
#  define    ensurepolylist(l, t, c) _ensurepolylist((l), (t), (c), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
#else
  POLYGON   *allocpolygon(INTBIG points, CLUSTER *cluster);
  POLYLIST  *allocpolylist(CLUSTER *cluster);
  BOOLEAN    ensurepolylist(POLYLIST *list, INTBIG tot, CLUSTER *cluster);
#endif
BOOLEAN      needstaticpolygon(POLYGON **, INTBIG points, CLUSTER *cluster);
void         freepolygon(POLYGON *poly);
void         freepolylist(POLYLIST *plist);
BOOLEAN      extendpolygon(POLYGON *poly, INTBIG newcount);
BOOLEAN      polysame(POLYGON *poly1, POLYGON *poly2);
void         polycopy(POLYGON *dpoly, POLYGON *spoly);
BOOLEAN      isinside(INTBIG x, INTBIG y, POLYGON *poly);
float        areapoly(POLYGON *poly);
float        areapoints(INTBIG count, INTBIG *x, INTBIG *y);
void         reversepoly(POLYGON *poly);
void         xformpoly(POLYGON *poly, XARRAY trans);
BOOLEAN      isbox(POLYGON *poly, INTBIG *xl, INTBIG *xh, INTBIG *yl, INTBIG *yh);
void         maketruerect(POLYGON *poly);
INTBIG       polydistance(POLYGON *poly, INTBIG x, INTBIG y);
INTBIG       polyminsize(POLYGON *poly);
INTBIG       polyseparation(POLYGON*, POLYGON*);
BOOLEAN      polyintersect(POLYGON*, POLYGON*);
void         makerectpoly(INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, POLYGON *poly);
void         maketruerectpoly(INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, POLYGON *poly);
void         makedisparrayvarpoly(GEOM *geom, WINDOWPART *win, VARIABLE *var, POLYGON *poly);
void         getdisparrayvarlinepos(INTBIG addr, INTBIG type, TECHNOLOGY *tech,
				WINDOWPART *win, VARIABLE *var, INTBIG line, INTBIG *x, INTBIG *y, BOOLEAN lowleft);
void         getcenter(POLYGON *poly, INTBIG *x, INTBIG *y);
void         getbbox(POLYGON *poly, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
BOOLEAN      polysplithoriz(INTBIG yl, POLYGON *which, POLYGON **abovep, POLYGON **belowp);
BOOLEAN      polysplitvert(INTBIG xl, POLYGON *which, POLYGON **leftp, POLYGON **rightp);

/* box/polygon merging routines */
void         mrginit(void);
void         mrgterm(void);
void         mrgstorebox(INTBIG layer, TECHNOLOGY *tech, INTBIG length, INTBIG width, INTBIG xc, INTBIG yc);
void         mrgdonecell(void (*writepolygon)(INTBIG layernum, TECHNOLOGY *tech,
				INTBIG *x, INTBIG *y, INTBIG count));
void        *mergenew(CLUSTER *cluster);
void         mergeaddpolygon(void *merge, INTBIG layer, TECHNOLOGY *tech, POLYGON *poly);
void         mergesubpolygon(void *merge, INTBIG layer, TECHNOLOGY *tech, POLYGON *poly);
void         mergeaddmerge(void *merge, void *addmerge, XARRAY trans);
BOOLEAN      mergebbox(void *merge, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
void         mergeextract(void *merge, void (*writepolygon)(INTBIG layernum, TECHNOLOGY *tech,
				INTBIG *x, INTBIG *y, INTBIG count));
void         mergedelete(void *merge);
CHAR        *mergedescribe(void *merge);
void         mergeshow(void *vmerge, NODEPROTO *prim, INTBIG showXoff, INTBIG showYoff, CHAR *title);

/* string manipulation routines */
CHAR        *latoa(INTBIG value, INTBIG lambda);
INTBIG       atola(CHAR *str, INTBIG lambda);
CHAR        *frtoa(INTBIG value);
INTBIG       atofr(CHAR *str);
INTBIG       myatoi(CHAR *str);
BOOLEAN      isanumber(CHAR *str);
CHAR        *hugeinttoa(INTHUGE a);
void         evsnprintf(CHAR *line, INTBIG len, CHAR *msg, va_list ap);
CHAR        *explainduration(float duration);
CHAR        *displayedunits(float value, INTBIG unittype, INTBIG unitscale);
float        figureunits(CHAR *str, INTBIG unittype, INTBIG unitscale);
void         parseelectricversion(CHAR *version, INTBIG *major, INTBIG *minor, INTBIG *detail);
CHAR        *makeplural(CHAR *word, INTBIG amount);
CHAR        *makeabbrev(CHAR *str, BOOLEAN upper);
INTBIG       namesame(CHAR *str1, CHAR *str2);
INTBIG       namesamen(CHAR *str1, CHAR *str2, INTBIG length);
INTBIG       namesamenumeric(CHAR *name1, CHAR *name2);
INTBIG       stringmatch(CHAR *str1, CHAR *str2);
INTBIG       parse(CHAR *keyword, COMCOMP *list, BOOLEAN noise);
void        *initinfstr(void);
void         addtoinfstr(void*, CHAR c);
void         addstringtoinfstr(void*, CHAR *str);
void         formatinfstr(void*, CHAR *msg, ...);
CHAR        *returninfstr(void*);
void        *newstringarray(CLUSTER *cluster);
void         killstringarray(void *sa);
void         clearstrings(void *sa);
void         addtostringarray(void *sa, CHAR *string);
void         stringarraytotextcell(void *sa, NODEPROTO *np, BOOLEAN permanent);
CHAR       **getstringarray(void *sa, INTBIG *count);
CHAR        *getkeyword(CHAR **ptin, CHAR *brk);
CHAR         tonextchar(CHAR **ptin);
CHAR         seenextchar(CHAR **ptin);
void         defaulttextdescript(UINTBIG *textdescript, GEOM *geom);
void         defaulttextsize(INTBIG texttype, UINTBIG *textdescript);
INTBIG       gettdxoffset(UINTBIG *t);
INTBIG       gettdyoffset(UINTBIG *t);
void         settdoffset(UINTBIG *t, INTBIG x, INTBIG y);
void         tdclear(UINTBIG *t);
void         tdcopy(UINTBIG *d, UINTBIG *s);
BOOLEAN      tddiff(UINTBIG *t1, UINTBIG *t2);
void         propervaroffset(INTBIG *x, INTBIG *y);
CHAR        *unitsname(INTBIG units);
void         myencrypt(CHAR *text, CHAR *key);
CHAR        *ectime(time_t *curtime);
#ifdef _UNICODE
  CHAR1     *estring1byte(CHAR *string);
  CHAR      *estring2byte(CHAR1 *string);
#endif

/* language interface routines */
CHAR        *languagename(void);
BOOLEAN      languageconverse(INTBIG language);
VARIABLE    *doquerry(CHAR *code, INTBIG language, UINTBIG type);
BOOLEAN      loadcode(CHAR *program, INTBIG language);

/* error reporting routines */
void         initerrorlogging(CHAR *system);
void        *logerror(CHAR *message, NODEPROTO *cell, INTBIG sortkey);
void         addgeomtoerror(void *errorlist, GEOM *geom, BOOLEAN showit, INTBIG pathlen, NODEINST **path);
void         addexporttoerror(void *errorlist, PORTPROTO *pp, BOOLEAN showit);
void         addlinetoerror(void *errorlist, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
void         addpolytoerror(void *errorlist, POLYGON *poly);
void         addpointtoerror(void *errorlist, INTBIG x, INTBIG y);
void         termerrorlogging(BOOLEAN explain);
void         sorterrors(void);
INTBIG       numerrors(void);
CHAR        *reportnexterror(INTBIG showhigh, GEOM **g1, GEOM **g2);
CHAR        *reportpreverror(void);
void        *getnexterror(void *e);
CHAR        *describeerror(void *e);
void         reporterror(void *e);
INTBIG       getnumerrorgeom(void *e);
void        *geterrorgeom(void *e, INTBIG index);
CHAR        *describeerrorgeom(void *ehv);
void         showerrorgeom(void *ehv);
void         adviseofchanges(void (*routine)(void));

/* memory allocation routines */
#ifdef DEBUGMEMORY
  INTBIG   *_emalloc(INTBIG size, CLUSTER *cluster, CHAR*, INTBIG);
# define     emalloc(x, c) _emalloc((x), (c), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
  BOOLEAN   _allocstring(CHAR **addr, CHAR *str, CLUSTER *cluster, CHAR*, INTBIG);
# define     allocstring(a, s, c) _allocstring((a), (s), (c), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
  BOOLEAN   _reallocstring(CHAR **addr, CHAR *str, CLUSTER *cluster, CHAR*, INTBIG);
# define     reallocstring(a, s, c) _reallocstring((a), (s), (c), WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
#else
  INTBIG    *emalloc(INTBIG size, CLUSTER *cluster);
  BOOLEAN    allocstring(CHAR **addr, CHAR *str, CLUSTER *cluster);
  BOOLEAN    reallocstring(CHAR **addr, CHAR *str, CLUSTER *cluster);
#endif
void         efree(CHAR *addr);
CLUSTER     *alloccluster(CHAR *name);
void         freecluster(CLUSTER *cluster);
NODEINST    *allocnodeinst(CLUSTER *cluster);
void         freenodeinst(NODEINST *ni);
ARCINST     *allocarcinst(CLUSTER *cluster);
void         freearcinst(ARCINST *ai);
PORTPROTO   *allocportproto(CLUSTER *cluster);
void         freeportproto(PORTPROTO *pp);
PORTARCINST *allocportarcinst(CLUSTER *cluster);
void         freeportarcinst(PORTARCINST *pi);
PORTEXPINST *allocportexpinst(CLUSTER *cluster);
void         freeportexpinst(PORTEXPINST *pe);
NODEPROTO   *allocnodeproto(CLUSTER *cluster);
void         freenodeproto(NODEPROTO *np);
VIEW        *allocview(void);
void         freeview(VIEW *view);
GEOM        *allocgeom(CLUSTER *cluster);
void         freegeom(GEOM *geom);
RTNODE      *allocrtnode(CLUSTER *cluster);
void         freertnode(RTNODE *rtnode);
LIBRARY     *alloclibrary(void);
void         freelibrary(LIBRARY *lib);
TECHNOLOGY  *alloctechnology(CLUSTER *cluster);
void         freetechnology(TECHNOLOGY *tech);
void         freewindowpart(WINDOWPART *win);

/* terminal output routines */
void         ttynewcommand(void);
void         ttyputerr(CHAR *format, ...);
void         ttyputmsg(CHAR *format, ...);
void         ttyputverbose(CHAR *format, ...);
void         ttyputinstruction(CHAR *keystroke, INTBIG length, CHAR *meaning);
void         ttyputusage(CHAR *usage);
void         ttyputbadusage(CHAR *command);
void         ttyputnomemory(void);
void         error(CHAR *format, ...);
INTBIG       ttyquiet(INTBIG flag);
void         ttybeep(INTBIG sound, BOOLEAN force);
void         telldatabaseerror(void);
void         ttyclearmessages(void);

/* terminal input routines */
INTSML       ttygetchar(INTBIG *special);
CHAR        *ttygetline(CHAR *prompt);
CHAR        *ttygetlinemessages(CHAR *prompt);
INTBIG       ttygetparam(CHAR *prompt, COMCOMP *parameter, INTBIG keycount, CHAR *paramstart[]);
INTBIG       ttygetfullparam(CHAR *prompt, COMCOMP *parameter, INTBIG keycount, CHAR *paramstart[]);
BOOLEAN      ttydataready(void);
void         checkforinterrupt(void);
INTBIG       getbuckybits(void);

/* mouse/tablet input routines */
BOOLEAN      getxy(INTBIG *x, INTBIG *y);
void         gridalign(INTBIG *xcur, INTBIG *ycur, INTBIG alignment, NODEPROTO *cell);
INTBIG       buttoncount(void);
CHAR        *buttonname(INTBIG but, INTBIG *important);
BOOLEAN      doublebutton(INTBIG but);
BOOLEAN      shiftbutton(INTBIG but);
BOOLEAN      contextbutton(INTBIG but);
BOOLEAN      wheelbutton(INTBIG but);
void         waitforbutton(INTBIG *x, INTBIG *y, INTBIG *but);
void         modalloop(BOOLEAN (*charhandler)(INTSML chr, INTBIG special),
				BOOLEAN (*buttonhandler)(INTBIG x, INTBIG y, INTBIG but), INTBIG cursor);
void         trackcursor(BOOLEAN waitforpush, BOOLEAN (*whileup)(INTBIG x, INTBIG y), void (*whendown)(void),
				BOOLEAN (*eachdown)(INTBIG x, INTBIG y), BOOLEAN (*eachchar)(INTBIG x, INTBIG y, INTSML chr),
				void (*done)(void), INTBIG purpose);
void         readtablet(INTBIG *x, INTBIG *y);
void         stoptablet(void);

/* time routines */
UINTBIG      ticktime(void);
UINTBIG      eventtime(void);
INTBIG       doubleclicktime(void);
void         gotosleep(INTBIG ticks);
void         starttimer(void);
float        endtimer(void);
time_t       getcurrenttime(void);
CHAR        *timetostring(time_t time);
void         parsetime(time_t time, INTBIG *year, INTBIG *month, INTBIG *mday,
				INTBIG *hour, INTBIG *minute, INTBIG *second);
INTBIG       parsemonth(CHAR *monthname);

/* disk file routines */
CHAR        *truepath(CHAR *path);
CHAR        *fullfilename(CHAR *file);
CHAR        *skippath(CHAR *file);
INTBIG       erename(CHAR *file, CHAR *newfile);
INTBIG       eunlink(CHAR *file);
CHAR        *fileselect(CHAR*, INTBIG, CHAR*);
CHAR        *multifileselectin(CHAR *msg, INTBIG filetype);
INTBIG       fileexistence(CHAR *file);
CHAR        *currentdirectory(void);
CHAR        *hashomedir(void);
BOOLEAN      createdirectory(CHAR *directory);
INTBIG       filesindirectory(CHAR *directory, CHAR ***filelist);
BOOLEAN      lockfile(CHAR *file);
void         unlockfile(CHAR *file);
time_t       filedate(CHAR *file);
INTBIG       filesize(FILE *stream);
BOOLEAN      browsefile(CHAR *filename);
void         describefiletype(INTBIG filetype, CHAR **extension, CHAR **winfilter,
				INTBIG *mactype, BOOLEAN *binary, CHAR **shortname, CHAR **longname);
INTBIG       setupfiletype(CHAR *extension, CHAR *winfilter, INTBIG mactype, BOOLEAN binary,
				CHAR *shortname, CHAR *longname);
INTBIG       getfiletype(CHAR *shortname);
FILE        *efopen(CHAR *filename, CHAR *mode);
FILE        *xopen(CHAR *file, INTBIG filetype, CHAR *otherdir, CHAR **truename);
FILE        *xcreate(CHAR *name, INTBIG filetype, CHAR *prompt, CHAR **truename);
FILE        *xappend(CHAR *file);
void         xclose(FILE *stream);
void         xflushbuf(FILE *stream);
BOOLEAN      xeof(FILE *stream);
void         xseek(FILE *stream, INTBIG pos, INTBIG nature);
INTBIG       xtell(FILE *f);
void         xprintf(FILE *stream, CHAR *format, ...);
INTSML       xgetc(FILE *stream);
void         xungetc(CHAR chr, FILE *stream);
void         xputc(CHAR chr, FILE *stream);
void         xputs(CHAR *str, FILE *stream);
INTBIG       xfread(UCHAR1 *str, INTBIG size, INTBIG count, FILE *stream);
INTBIG       xfwrite(UCHAR1 *str, INTBIG size, INTBIG count, FILE *stream);
BOOLEAN      xfgets(CHAR *str, INTBIG limit, FILE *stream);

#if !defined(USEQT)
/* channel routines */
INTBIG       eread(int channel, UCHAR1 *addr, INTBIG count);
INTBIG       ewrite(int channel, UCHAR1 *addr, INTBIG count);
INTBIG       eclose(int channels);
INTBIG       epipe(int channels[2]);
INTBIG       channelreplacewithfile(int channel, CHAR *file);
INTBIG       channelreplacewithchannel(int channel, int newchannel);
void         channelrestore(int channel, INTBIG saved);
void         setinteractivemode(int channel);
#endif

/* miscellaneous routines */
void         changelambda(INTBIG count, TECHNOLOGY **techarray, INTBIG *newlam,
				LIBRARY *whichlib, INTBIG how);
void         changetechnologylambda(TECHNOLOGY *tech, INTBIG newlambda);
INTBIG       figurelambda(GEOM *geom);
void         changeinternalunits(LIBRARY *whichlib, INTBIG oldunits, INTBIG newunits);
NETWORK     *getnetwork(CHAR *name, NODEPROTO *cell);
NETWORK    **getcomplexnetworks(CHAR *name, NODEPROTO *cell);
NETWORK     *getnetonport(NODEINST *node, PORTPROTO *port);
CHAR        *describenetwork(NETWORK *net);
CHAR        *networkname(NETWORK *net, INTBIG i);
void         bringdown(void);
BOOLEAN      stopping(INTBIG reason);
CHAR        *egetenv(CHAR *name);
CHAR        *elanguage(void);
#if !defined(USEQT)
INTBIG       efork(void);
#endif
INTBIG       esystem(CHAR *command);
void         eexec(CHAR *program, CHAR *args[]);
#if !defined(USEQT)
INTBIG       ekill(INTBIG process);
void         ewait(INTBIG process);
#endif
INTBIG       enumprocessors(void);
void         enewthread(void* (*function)(void*), void *argument);
void        *emakemutex(void);
void         emutexlock(void *vmutex);
void         emutexunlock(void *vmutex);
CHAR       **eprinterlist(void);
void         flushscreen(void);
void         exitprogram(void);
void         esort(void *data, INTBIG entries, INTBIG structsize,
				   int (*ordergood)(const void *e1, const void *e2));
int          sort_intbigascending(const void *e1, const void *e2);
int          sort_intbigdescending(const void *e1, const void *e2);
int          sort_stringascending(const void *e1, const void *e2);
int          sort_cellnameascending(const void *e1, const void *e2);
int          sort_exportnameascending(const void *e1, const void *e2);
int          sort_exportnamedescending(const void *e1, const void *e2);
BOOLEAN      ensurevalidmutex(void **mutex, BOOLEAN showerror);
void         setmultiprocesslocks(BOOLEAN on);
#ifdef DEBUGPARALLELISM
#  define NOT_REENTRANT ensurenonparallel(WIDENSTRINGDEFINE(__FILE__), (INTBIG)__LINE__)
#else
#  define NOT_REENTRANT
#endif
void         ensurenonparallel(CHAR *file, INTBIG line);

/* graphics and control (not documented in Internals Manual) */
void         osprimaryosinit(void);
void         ossecondaryinit(INTBIG, CHAR1*[]);
void         tooltimeslice(void);
void         forceslice(void);
void         graphicsoptions(CHAR *name, INTBIG *argc, CHAR1 **argv);
void         setupenvironment(void);
void         setlibdir(CHAR *libdir);
BOOLEAN      initgraphics(BOOLEAN messages);
void         termgraphics(void);
WINDOWFRAME *newwindowframe(BOOLEAN floating, RECTAREA *r);
void         killwindowframe(WINDOWFRAME*);
void         getpaletteparameters(INTBIG *wid, INTBIG *hei, INTBIG *palettewidth);
void         resetpaletteparameters(void);
void         sizewindowframe(WINDOWFRAME *frame, INTBIG wid, INTBIG hei);
void         movewindowframe(WINDOWFRAME *frame, INTBIG left, INTBIG top);
#ifdef USEQT
void         movedisplay(void);
#endif
void         bringwindowtofront(WINDOWFRAME *frame);
void         adjustwindowframe(INTBIG how);
WINDOWFRAME *getwindowframe(BOOLEAN canfloat);
void         getwindowframesize(WINDOWFRAME *frame, INTBIG *wid, INTBIG *hei);
BOOLEAN      graphicshas(INTBIG want);
void         setdefaultcursortype(INTBIG state);
void         setnormalcursor(INTBIG curs);
void         colormapload(INTBIG *red, INTBIG *green, INTBIG *blue, INTBIG low, INTBIG high);
void         screendrawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc,
				INTBIG texture);
void         screeninvertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2);
void         screendrawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc);
void         screendrawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, GRAPHICS *desc);
void         screeninvertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy);
void         screenmovebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei, INTBIG dx, INTBIG dy);
INTBIG       screensavebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
void         screenmovesavedbox(INTBIG code, INTBIG dx, INTBIG dy);
void         screenrestorebox(INTBIG code, INTBIG destroy);
void         screensettextinfo(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript);
INTBIG       screenfindface(CHAR *facename);
INTBIG       screengetfacelist(CHAR ***list, BOOLEAN all);
CHAR        *screengetdefaultfacename(void);
void         screengettextsize(WINDOWPART *win, CHAR *str, INTBIG *x, INTBIG *y);
void         screendrawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, CHAR *s, GRAPHICS *desc);
BOOLEAN      gettextbits(WINDOWPART *win, CHAR *msg, INTBIG *wid, INTBIG *hei, UCHAR1 ***rowstart);
void         screendrawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc);
void         screendrawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1x, INTBIG p1y,
				INTBIG p2x, INTBIG p2y, GRAPHICS *desc);
void         screendrawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc);
void         screendrawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc);
void         screendrawthickcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1x, INTBIG p1y,
				INTBIG p2x, INTBIG p2y, GRAPHICS *desc);
void         screendrawgrid(WINDOWPART *win, POLYGON *obj);
INTSML       getnxtchar(INTBIG *special);
time_t       machinetimeoffset(void);
BOOLEAN      logplayback(CHAR *file);
void         logstartrecord(void);
void         logfinishrecord(void);
void         etrace(INTBIG mode, CHAR *s, ...);
void         getacceleratorstrings(CHAR **acceleratorstring, CHAR **acceleratorprefix);
CHAR        *getinterruptkey(void);
CHAR        *getmessagesstring(CHAR *prompt);
void         putmessagesstring(CHAR *str, BOOLEAN important);
CHAR        *getmessageseofkey(void);
void         setmessagesfont(void);
void         getmessagesframeinfo(INTBIG *top, INTBIG *left, INTBIG *bottom, INTBIG *right);
void         setmessagesframeinfo(INTBIG top, INTBIG left, INTBIG bottom, INTBIG right);
BOOLEAN      closefrontmostmessages(void);
void         clearmessageswindow(void);
BOOLEAN      cutfrommessages(void);
BOOLEAN      copyfrommessages(void);
BOOLEAN      pastetomessages(void);
CHAR        *getcutbuffer(void);
void         setcutbuffer(CHAR *msg);
CHAR        *optionsfilepath(void);
#if defined(WIN32) || defined(USEQT)
void         printewindow(void);
#endif

/* command completion coroutines (not documented in Internals Manual) */
void         requiredextension(CHAR *extension);
CHAR        *nexttools(void);
CHAR        *nextarcs(void);
CHAR        *nextcells(void);
CHAR        *nextfile(void);
CHAR        *nextlibs(void);
CHAR        *nextnets(void);
CHAR        *nexttechs(void);
CHAR        *nextviews(void);
BOOLEAN      topoftools(CHAR **chrpos);
BOOLEAN      topoffile(CHAR **chrpos);
BOOLEAN      topoflibfile(CHAR **chrpos);
BOOLEAN      topoflibs(CHAR **chrpos);
BOOLEAN      topofnets(CHAR **chrpos);
BOOLEAN      topoftechs(CHAR **chrpos);
BOOLEAN      topofviews(CHAR **chrpos);
BOOLEAN      topofcells(CHAR **chrpos);
BOOLEAN      topofarcs(CHAR **chrpos);
BOOLEAN      us_patoplist(CHAR **chrpos);
INTBIG       us_paparams(CHAR *word, COMCOMP *arr[], CHAR breakc);
CHAR        *us_panextinlist(void);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

#if defined(__cplusplus)

/* EProcess class */

class EProcessPrivate;

class EProcess
{
public:
	EProcess();
	~EProcess();
	void clearArguments();
	void addArgument( CHAR *arg );
	void setCommunication( BOOLEAN cStdin, BOOLEAN cStdout, BOOLEAN cStderr );
	BOOLEAN start( CHAR *infile = 0 );
	void wait();
	void kill();
	INTSML getChar();
	void putChar( UCHAR1 ch );
private:
	EProcessPrivate *d;
};

#endif

#endif /* GLOBAL_H */
