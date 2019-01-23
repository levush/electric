/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: database.h
 * Database manager: header file
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

/*************************** FLAG BIT SETTINGS ****************************/

#define MARKN             040				/* NODEINST->userbits: nodeinst is marked */
#define TOUCHN           0100				/* NODEINST->userbits: nodeinst is touched */

#define FIXEDMOD   0100000000				/* ARCINST->userbits: fixed arc was changed */

#define CELLMOD       0400000				/* NODEPROTO->userbits: cell is changed */
#define CELLNOMOD    01000000				/* NODEPROTO->userbits: cell is not changed */

/*************************** CHANGES ****************************/

#define NOCHANGE	((CHANGE *)-1)

/* meaning of "changetype" (when changing this, also change "broadcasttype[]" in "db_change()") */
#define NODEINSTNEW      0					/* new nodeinst */
#define NODEINSTKILL     1					/* killed nodeinst */
#define NODEINSTMOD      2					/* modified nodeinst */

#define ARCINSTCHANGE    3					/* base of ARCINST changes */
#define ARCINSTNEW       3					/* new arcinst */
#define ARCINSTKILL      4					/* deleted arcinst */
#define ARCINSTMOD       5					/* modified arcinst */

#define PORTPROTOCHANGE  6					/* base of PORTPROTO changes */
#define PORTPROTONEW     6					/* new portinst on a nodeproto */
#define PORTPROTOKILL    7					/* deleted portinst on a nodeproto */
#define PORTPROTOMOD     8					/* motified portinst on a nodeproto */

#define NODEPROTOCHANGE  9					/* base of NODEPROTO changes */
#define NODEPROTONEW     9					/* new nodeproto */
#define NODEPROTOKILL   10					/* killed nodeproto */
#define NODEPROTOMOD    11					/* modified nodeproto */

#define VARIABLECHANGE  12					/* base of VARIABLE changes */
#define OBJECTSTART     12					/* start of object change */
#define OBJECTEND       13					/* end of object change */
#define OBJECTNEW       14					/* new object */
#define OBJECTKILL      15					/* deleted object */
#define VARIABLENEW     16					/* new variable */
#define VARIABLEKILL    17					/* deleted variable */
#define VARIABLEMOD     18					/* changed single entry in array variable */
#define VARIABLEINS     19					/* inserted single entry in array variable */
#define VARIABLEDEL     20					/* deleted single entry in array variable */
#define DESCRIPTMOD     21					/* changed text descriptor */

typedef struct Ichange
{
	INTBIG          changetype;				/* type of change */
	INTBIG          entryaddr;				/* address of object that was changed */
	struct Ichange *nextchange;				/* address of next change module in list */
	struct Ichange *prevchange;				/* address of previous change module */
	INTBIG          p1,p2,p3,p4,p5,p6;		/* old values before the change */
} CHANGE;

/*************************** CHANGE BATCHES ****************************/

#define NOCHANGECELL	((CHANGECELL *)-1)

typedef struct Ichangecell
{
	NODEPROTO           *changecell;		/* the cell that changed */
	BOOLEAN              forcedlook;		/* true if library must be re-examined */
	struct Ichangecell  *nextchangecell;	/* next in list */
} CHANGECELL;


#define NOCHANGEBATCH	((CHANGEBATCH *)-1)

typedef struct Ichangebatch
{
	CHANGE              *changehead;		/* head of list of things to change */
	CHANGE              *changetail;		/* tail of list of things to change */
	struct Ichangebatch *nextchangebatch;	/* next in list */
	struct Ichangebatch *lastchangebatch;	/* last in list */
	CHANGECELL          *firstchangecell;	/* the cell in which this change was made */
	TOOL                *tool;				/* tool that made this batch */
	CHAR                *activity;			/* description of activity */
	INTBIG               batchnumber;		/* identifying index of this batch */
	BOOLEAN              done;				/* true if this batch was done */
} CHANGEBATCH;

/*************************** BOX MERGING ****************************/

/* box descriptor */
typedef struct Iboxpoly
{
	INTBIG             ll[2], lr[2], ul[2], ur[2];
	INTBIG             top, bot, left, right;
	INTBIG             ishor;				/* "ishor" unused */
	INTBIG             numsides, numint[4];
	struct Iboxpoly   *nextbox;
	struct Ipolycoord *polyint[4];
	struct Ipolycoord *auxpolyint[4];
} BOXPOLY;

/* box list head */
typedef struct Iboxlisthead
{
	INTBIG               layer;
	TECHNOLOGY          *tech;
	struct Iboxpoly     *box;
	struct Iboxlisthead *nextlayer;
} BOXLISTHEAD;

/* polygon edge descriptor */
typedef struct Ipolycoord
{
	INTBIG             indx, ed_or, ed_start[2], ed_end[2], ed_val, seen;
	struct Ipolycoord *nextpoly;
} POLYCOORD;

/* polygon edge ring head */
typedef struct Ipolylisthead
{
	struct Ipolycoord *firstpoly;
	INTBIG             numedge, modified;
} POLYLISTHEAD;

/*************************** ERROR CODES ****************************/

/* error messages */
#define DBNOERROR       0					/* no error */
#define DBNOMEM	        1					/* no memory */
#define DBBADTRANS      2					/* bad transposition */
#define DBBADROT        3					/* bad rotation */
#define DBBADPROTO      4					/* bad prototype */
#define DBBADPARENT     5					/* bad parent */
#define DBBADINST       6					/* invalid instance */
#define DBBADNAME       7					/* invalid name */
#define DBBADWIDTH      8					/* bad width */
#define DBBADENDAN      9					/* bad end A node/port */
#define DBBADENDBN     10					/* bad end B node/port */
#define DBBADENDAC     11					/* bad end A connection */
#define DBBADENDBC     12					/* bad end B connection */
#define DBBADENDAP     13					/* bad end A position */
#define DBBADENDBP     14					/* bad end B position */
#define DBBADNEWWID    15					/* bad new width */
#define DBBADCELL      16					/* bad cell */
#define DBBADLIB       17					/* bad library */
#define DBBADSIZE      18					/* bad size */
#define DBBADOBJECT    19					/* bad object type */
#define DBBADSUBPORT   20					/* bad sub port */
#define DBHASARCS      21					/* still has arcs */
#define DBHASPORTS     22					/* still has ports */
#define DBHASINSTANCES 23					/* cell has instances */
#define DBRECURSIVE    24					/* recursive call */
#define DBNOTINPORT    25					/* arc not in port */
#define DBCONFLICT     26					/* conflicts with primitive */
#define DBPORTMM       27					/* port mismatch */
#define DBDUPLICATE    28					/* duplicate name */
#define DBPRIMITIVE    29					/* primitive prototype */
#define DBBADTMAT      30					/* bad transformation matrix */
#define DBNOVAR        31					/* variable does not exist */
#define DBVARFIXED     32					/* variable cannot be set */
#define DBVARARRDIS    33					/* variable cannot be displayable array */
#define DBLASTECH      34					/* this is the last technology */
#define DBTECINUSE     35					/* technology is still in use */
#define DBNOSLIDING    36					/* sliding not allowed */

#define DBADDSTRINGTOINFSTR		(1<<16)
#define DBADDTECHNOLOGY			(2<<16)
#define DBADDTOINFSTR			(3<<16)
#define DBALLOCARCINST			(4<<16)
#define DBALLOCGEOM				(5<<16)
#define DBALLOCLIBRARY			(6<<16)
#define DBALLOCNODEINST			(7<<16)
#define DBALLOCNODEPROTO		(8<<16)
#define DBALLOCPOLYGON			(9<<16)
#define DBALLOCPORTARCINST		(10<<16)
#define DBALLOCPORTEXPINST		(11<<16)
#define DBALLOCPORTPROTO		(12<<16)
#define DBALLOCSTRING			(13<<16)
#define DBALLOCTECHNOLOGY		(14<<16)
#define DBALLOCVIEW				(15<<16)
#define DBCOPYNODEPROTO			(16<<16)
#define DBDELIND				(17<<16)
#define DBDELINDKEY				(18<<16)
#define DBDELVALKEY				(19<<16)
#define DBDESCRIBEVARIABLE		(20<<16)
#define DBEXTENDPOLYGON			(21<<16)
#define DBINITINFSTR			(22<<16)
#define DBINITOBJLIST			(23<<16)
#define DBINSIND				(24<<16)
#define DBINSINDKEY				(25<<16)
#define DBKILLARCINST			(26<<16)
#define DBKILLLIBRARY			(27<<16)
#define DBKILLNODEINST			(28<<16)
#define DBKILLNODEPROTO			(29<<16)
#define DBKILLPORTPROTO			(30<<16)
#define DBKILLTECHNOLOGY		(31<<16)
#define DBMAKEKEY				(32<<16)
#define DBMODIFYARCINST			(33<<16)
#define DBMOVEPORTPROTO			(34<<16)
#define DBNEWARCINST			(35<<16)
#define DBNEWLIBRARY			(36<<16)
#define DBNEWNODEINST			(37<<16)
#define DBNEWNODEPROTO			(38<<16)
#define DBNEWPORTPROTO			(39<<16)
#define DBNEWVIEW				(40<<16)
#define DBREPLACEARCINST		(41<<16)
#define DBREPLACENODEINST		(42<<16)
#define DBRETURNINFSTR			(43<<16)
#define DBSETIND				(44<<16)
#define DBSETINDKEY				(45<<16)
#define DBSETVAL				(46<<16)
#define DBSETVALKEY				(47<<16)
#define DBTRANSMULT				(48<<16)
#define DBXFORM					(49<<16)

extern INTBIG       db_lasterror;			/* last error message */
extern BOOLEAN      db_printerrors;			/* flag for printing internal errors */

/*************************** VARIABLE KEYS ****************************/

extern INTBIG  db_tech_node_width_offset_key;	/* variable "TECH_node_width_offset" */
extern INTBIG  db_tech_layer_function_key;		/* variable "TECH_layer_function" */
extern INTBIG  db_tech_layer_names_key;			/* variable "TECH_layer_names" */
extern INTBIG  db_tech_arc_width_offset_key;	/* variable "TECH_arc_width_offset" */

/*************************** MULTIPROCESSOR CONTROL ****************************/

extern BOOLEAN db_multiprocessing;				/* true if multiprocessing */

/*************************** CHANGE CONTROL ****************************/

extern BOOLEAN db_donextchangequietly;			/* true to do next change quietly */
extern BOOLEAN db_dochangesquietly;				/* true to do changes quietly */
extern UINTBIG db_changetimestamp;				/* timestamp for changes to database */
extern UINTBIG db_traversaltimestamp;			/* timestamp for hierarchy traversal */
extern INTBIG  db_broadcasting;					/* nonzero if broadcasting */

/*************************** LANGUAGE CONTROL ****************************/

extern BOOLEAN db_onanobject;			/* TRUE if code evaluation is "on an object" (getval, etc.) */
extern INTBIG  db_onobjectaddr;			/* the address of the first object that we are on */
extern INTBIG  db_onobjecttype;			/* the type of the first object that we are on */
extern INTBIG  db_lastonobjectaddr;		/* the address of the last object that we were on */
extern INTBIG  db_lastonobjecttype;		/* the type of the last object that we were on */

/*************************** PROTOTYPES ****************************/

/* database prototypes */
void         db_addportarcinst(NODEINST*, PORTARCINST*);
void         db_addportexpinst(NODEINST*, PORTEXPINST*);
BOOLEAN      db_addtortnode(UINTBIG, RTNODE*, NODEPROTO*);
void         db_boundcell(NODEPROTO*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
void         db_buildnodeprotohashtable(LIBRARY *lib);
void         db_buildportprotohashtable(NODEPROTO*);
CHANGE      *db_change(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void         db_changeport(PORTPROTO*, NODEINST*, PORTPROTO*);
void         db_checkallmemoryfree(void);
void         db_clearportcache(NODEPROTO *cell);
void         db_correctcellgroups(NODEPROTO *cell);;
VARIABLE    *db_dummyvariable(void);
void         db_endbatch(void);
void         db_enterarcinst(ARCINST*);
void         db_enternodeinst(NODEINST*);
void         db_enterportproto(PORTPROTO*);
BOOLEAN      db_enterwindowpart(WINDOWPART *w);
INTBIG       db_error(INTBIG);
NODEPROTO   *db_findnodeprotoname(CHAR *name, VIEW *view, LIBRARY *lib);
void         db_forcehierarchicalanalysis(NODEPROTO*);
void         db_freearcproto(ARCPROTO *ap);
void         db_freechangememory(void);
void         db_freeerrormemory(void);
void         db_freegeomemory(void);
void         db_freemathmemory(void);
void         db_freemergememory(void);
void         db_freenoprotomemory(void);
void         db_freertree(RTNODE *rtn);
void         db_freetclmemory(void);
void         db_freetechnologymemory(void);
void         db_freetextmemory(void);
void         db_freevar(INTBIG, INTBIG);
void         db_freevariablememory(void);
void         db_freevars(VARIABLE**, INTSML*);
CHANGEBATCH *db_getcurrentbatch(void);
float        db_getcurrentscale(INTBIG, INTBIG, INTBIG);
void         db_getinternalunitscale(INTBIG*, INTBIG*, INTBIG, INTBIG);
void         db_gettraversalpath(NODEPROTO *here, WINDOWPART *win, NODEINST ***nilist, INTBIG *depth);
BOOLEAN      db_getvarptr(INTBIG, INTBIG, VARIABLE***, INTSML**);
void         db_initclusters(void);
void         db_initdatabase(void);
void         db_initgeometry(void);
void         db_initializechanges(void);
void         db_initlanguages(void);
void         db_inittechcache(void);
void         db_inittechnologies(void);
void         db_inittranslation(void);
void         db_insertnodeproto(NODEPROTO*);
void         db_killarcinst(ARCINST*);
void         db_killnodeinst(NODEINST*);
void         db_mrgdatainit(void);
INTBIG       db_namehash(CHAR *name);
ARCINST     *db_newarcinst(ARCPROTO*, INTBIG, INTBIG, NODEINST*, PORTPROTO*, INTBIG, INTBIG,
	NODEINST*, PORTPROTO*, INTBIG, INTBIG, NODEPROTO*);
ARCPROTO    *db_newarcproto(TECHNOLOGY*, CHAR*, INTBIG, INTBIG);
NODEINST    *db_newnodeinst(NODEPROTO*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*);
PORTPROTO   *db_newportproto(NODEPROTO*, NODEINST*, PORTPROTO*, CHAR*);
NODEPROTO   *db_newprimnodeproto(CHAR*, INTBIG, INTBIG, INTBIG, TECHNOLOGY*);
PORTPROTO   *db_newprimportproto(NODEPROTO*, ARCPROTO**, CHAR*);
void         db_printclusterarena(CHAR*);
void         db_printrtree(RTNODE*, RTNODE*, INTBIG);
void         db_removecellfromgroup(NODEPROTO*);
void         db_removechangecell(NODEPROTO*);
void         db_removeportexpinst(PORTPROTO*);
void         db_retractarcinst(ARCINST*);
void         db_retractnodeinst(NODEINST*);
void         db_retractnodeproto(NODEPROTO*);
void         db_retractportproto(PORTPROTO*);
void         db_retractwindowpart(WINDOWPART *w);
void         db_rtnbbox(RTNODE*, INTBIG, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
void         db_setchangecell(NODEPROTO*);
void         db_setcurrenttool(TOOL*);
BOOLEAN      db_stillinport(ARCINST*, INTBIG, INTBIG, INTBIG);
void         db_termlanguage(void);
void         db_undodlog(void);
NODEPROTO   *db_whichnodeproto(INTBIG, INTBIG);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
