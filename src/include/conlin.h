/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: conlin.h
 * Linear inequality constraint system header file
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

extern CONSTRAINT *cli_constraint;		/* the constraint object for this solver */
extern BOOLEAN     cli_manhattan;		/* nonzero to assume Manhattan constraints */
extern INTBIG      cli_properties_key;	/* variable key for "CONLIN_properties" */

/***************** CONSTRAINT DATA STORED ON ARCS *************/

#define NOLINCON ((LINCON *)-1)

#define CLGEQUALS    0				/* greater than or equal */
#define CLEQUALS     1				/* equals */
#define CLLEQUALS    2				/* less than or equal */

#define CLLEFT       0				/* constraint left */
#define CLRIGHT      1				/* constraint right */
#define CLDOWN       2				/* constraint down */
#define CLUP         3				/* constraint up */

typedef struct
{
	INTBIG variable;				/* 0=left  1=right  2=up  3=down */
	INTBIG oper;					/* 0 >=    1 ==     2 <=  */
	INTBIG value;					/* the constraining distance (in WHOLE units) */
} LINCON;

#define LINCONSIZE ((sizeof (LINCON)) / SIZEOFINTBIG)

/***************** FOR THE TEXT-TO-GRAPHICS SYSTEM *************/

extern NODEPROTO *cli_curcell;		/* the current cell being equated to text */
extern BOOLEAN    cli_ownchanges;	/* true if changes are internally generated */
extern INTBIG     cli_textlines;	/* number of declaration/connection lines */
extern BOOLEAN    cli_texton;		/* true if text/graphics system is on */

/* the different types of lines of text (returned by "cli_lintype") */
#define LINEUNKN   0				/* line is unknown statement (an error) */
#define LINECOMM   1				/* line is a comment or empty */
#define LINEDECL   2				/* line is a "declare" statement */
#define LINECONN   3				/* line is a "connect" statement */
#define LINEEXPORT 4				/* line is an "export" statement */
#define LINEBEGIN  5				/* line is a "begincell" statement */
#define LINEEND    6				/* line is a "endcell" statement */

/***************** ADDITIONAL ATTRIBUTES *************/

#define NOATTR ((ATTR *)-1)

typedef struct Iattr
{
	CHAR         *name;				/* name of attribute */
	INTBIG        type;				/* type of attribute */
	struct Iattr *equiv;			/* for associating lists */
	INTBIG        value;			/* value of attribute */
	struct Iattr *nextattr;			/* next in linked list */
} ATTR;

/***************** PARSING OF "DECLARE" STATEMENTS *************/

#define NOCOMPONENT ((COMPONENT *)-1)

/* the meaning of COMPONENT->flag */
#define COMPSIZE    1				/* bit set if size was given */
#define COMPLOC     2				/* bit set if location was given */
#define COMPROT     4				/* bit set if rotation/transpose was given */
#define COMPATTR    8				/* bit set if attributes given */

typedef struct Icomponent
{
	INTBIG sizex, sizey;			/* object size (in database units) */
	INTBIG locx, locy;				/* object location (in database units) */
	INTBIG rot, trans;				/* rotation and transpose */
	INTBIG flag;					/* valid information (see above) */
	CHAR  *name;
	ATTR  *firstattr;				/* list of attributes */
	struct Icomponent *assoc;		/* for comparing component lists */
	struct Icomponent *nextcomponent;
} COMPONENT;

#define NOCOMPONENTDEC ((COMPONENTDEC *)-1)

typedef struct Icomponentdec
{
	INTBIG count;
	CHAR *protoname;
	COMPONENT *firstcomponent;
} COMPONENTDEC;

/***************** PARSING OF "CONNECT" STATEMENTS *************/

#define NOCONS ((CONS *)-1)

/* these match the "cli_linconops" in "conlin.c" */
#define GEQ     0					/* constraint demands greater than or equality */
#define EQUAL   1					/* constraint demands equality */
#define LEQ     2					/* constraint demands less than or equality */

typedef struct Icons
{
	CHAR  *direction;				/* the constraint direction */
	INTBIG amount;					/* the constraint amount (in WHOLE units) */
	INTBIG flag;					/* 0: equal   1: geq   -1: leq */
	struct Icons *assoc;			/* associated constraint */
	struct Icons *nextcons;			/* next in list */
} CONS;

#define NOCONNECTION ((CONNECTION *)-1)

#define LAYERVALID    1				/* set if connecting arc type specified */
#define END1VALID     2				/* set if end 1 specified */
#define PORT1VALID    4				/* set if port 1 specified */
#define END2VALID     8				/* set if end 2 specified */
#define PORT2VALID   16				/* set if port 2 specified */
#define OFFSETVALID  32				/* set if arc offset specified */
#define WIDTHVALID   64				/* set if arc width specified */
#define ATTRVALID   128				/* set if attributes specified */

typedef struct Iconnection
{
	CHAR  *layer;					/* the connecting arc type (optional) */
	INTBIG width;					/* the width of the arc (optional) */
	ATTR  *firstattr;				/* list of attributes (optional) */
	CHAR  *end1;					/* the node at end 1 */
	CHAR  *port1;					/* the port on the node at end 1 (optional) */
	CONS  *firstcons;				/* the constraints on the arc */
	INTBIG xoff, yoff;				/* the actual distance (optional) */
	CHAR  *end2;					/* the node at end 2 */
	CHAR  *port2;					/* the port on the node at end 2 (optional) */
	INTBIG flag;					/* bits saying what is valid in this list */
} CONNECTION;

/***************** PARSING OF "EXPORT" STATEMENTS *************/

#define NOEXPORT ((EXPORT *)-1)

#define EXPCHAR      1				/* set if power/ground characteristics valid */
#define EXPATTR      2				/* set if attributes valid */

typedef struct Iexport
{
	CHAR  *portname;				/* the exported port name */
	CHAR  *component;				/* the component source of the port */
	CHAR  *subport;					/* the subport on the exported component */
	INTBIG bits;					/* the characteristics/location of the port */
	ATTR  *firstattr;				/* list of attributes (optional) */
	INTBIG flag;					/* bits saying what is valid in list */
	struct Iexport *nextexport;		/* next in list */
} EXPORT;

/***************** INTERFACE PROTOTYPES *************/

void cli_linconinit(CONSTRAINT*);
void cli_linconterm(void);
void cli_linconsetmode(INTBIG, CHAR*[]);
INTBIG cli_linconrequest(CHAR*, INTBIG);
void cli_linconsolve(NODEPROTO*);
void cli_linconnewobject(INTBIG, INTBIG);
void cli_linconkillobject(INTBIG, INTBIG);
BOOLEAN cli_linconsetobject(INTBIG, INTBIG, INTBIG, INTBIG);
void cli_linconmodifynodeinst(NODEINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cli_linconmodifynodeinsts(INTBIG,NODEINST**, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
void cli_linconmodifyarcinst(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cli_linconmodifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
void cli_linconmodifynodeproto(NODEPROTO*);
void cli_linconmodifydescript(INTBIG, INTBIG, INTBIG, UINTBIG*);
void cli_linconnewlib(LIBRARY*);
void cli_linconkilllib(LIBRARY*);
void cli_linconnewvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void cli_linconkillvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
void cli_linconmodifyvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cli_linconinsertvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void cli_lincondeletevariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void cli_linconsetvariable(void);
extern COMCOMP cli_linconp;

/***************** PROTOTYPES FOR INTERMODULE ROUTINES *************/

void cli_maketextcell(NODEPROTO*);
void cli_highlightequivalent(GEOM*);
void cli_eq_solve(void);
void cli_eq_newnode(NODEINST*);
void cli_eq_newport(PORTPROTO*);
void cli_eq_newarc(ARCINST*);
void cli_eq_killarc(ARCINST*);
void cli_eq_killnode(NODEINST*);
void cli_eq_killport(PORTPROTO*);
void cli_eq_modarc(ARCINST*);
void cli_eq_modnode(NODEINST*);
void cli_eq_modport(PORTPROTO*);
void cli_eq_newvar(INTBIG, INTBIG, INTBIG);
void cli_eq_killvar(INTBIG, INTBIG, INTBIG);
INTBIG cli_linetype(CHAR*);
COMPONENTDEC *cli_parsecomp(CHAR*, BOOLEAN);
void cli_deletecomponentdec(COMPONENTDEC*);
WINDOWPART *cli_makeeditorwindow(CHAR*, INTBIG*, INTBIG*);
void cli_replaceendcell(CHAR*, WINDOWPART*);
EXPORT *cli_parseexport(CHAR*, BOOLEAN);
void cli_deleteexport(EXPORT*);
CONNECTION *cli_parseconn(CHAR*, BOOLEAN);
void cli_deleteconnection(CONNECTION*);
ARCINST *cli_findarcname(CHAR*);
NODEINST *cli_findnodename(CHAR*);
CHAR *cli_parsebegincell(CHAR*, BOOLEAN);
void cli_replacename(CHAR*, CHAR*);
BOOLEAN cli_pickwire(CONNECTION*, NODEINST**, PORTPROTO**, NODEINST**, PORTPROTO**, ARCPROTO**, INTBIG*, ARCINST*, NODEINST*, NODEINST*);
void cli_deletearcconstraint(ARCINST*, INTBIG, INTBIG, INTBIG);
BOOLEAN cli_addarcconstraint(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG);
void cli_solvecell(NODEPROTO*, BOOLEAN, BOOLEAN);
BOOLEAN cli_uniqueport(NODEINST*, PORTPROTO*);
void cli_deleteequivcon(ARCINST*);
void cli_changeequivcomp(NODEINST*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
