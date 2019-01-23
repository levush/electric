/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iocifpars.h
 * CIF Parsing header
 * Written by: Robert W. Hon, Schlumberger Palo Alto Research
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

/******************** parser.h ********************/

/* enumerated types for cif 2.0 parser */

#define NIL           0

#define SEMANTICERROR 0
#define SYNTAXERROR   1
#define WIRECOM       2
#define BOXCOM        3
#define POLYCOM       4
#define FLASHCOM      5
#define DEFSTART      6
#define DEFEND        7
#define DELETEDEF     8
#define LAYER         9
#define CALLCOM      10
#define COMMENT      11
#define NULLCOMMAND  12
#define USERS        13
#define END          14
#define ENDFILE      15
#define SYMNAME      16
#define INSTNAME     17
#define GEONAME      18
#define LABELCOM     19

/******************** types.h ********************/

/* data types for cif 2.0 parser */

typedef struct {INTBIG x,y;} point;

/* types for tlists */
enum ttype {MIRROR, TRANSLATE, ROTATE};

typedef struct
{
	enum ttype kind;
	union
	{
		struct {CHAR xcoord;} mi;		/* whether to mirror x coord */
		struct {INTBIG xt,yt;} tr;		/* translation params */
		struct {INTBIG xrot,yrot;} ro;	/* rotation vector */
	} guts;
} tentry;

/******************** error.h ********************/

/* error codes for reporting errors */

#define FATALINTERNAL 0
#define FATALSYNTAX   1
#define FATALSEMANTIC 2
#define FATALOUTPUT   3
#define ADVISORY      4
#define OTHER         5			/* OTHER must be last */

/******************** interpreter.h ********************/

/* structures for the interpreter */

#define REL       0
#define NOREL     1
#define SAME      2
#define DONTCARE  3

#define RECTANGLE 1		/* codes for object types */
#define POLY      2
#define WIRE      3
#define FLASH     4
#define CALL      5
#define MBOX      6		/* manhattan box */
#define NAME      7		/* geometry name */
#define LABEL     8		/* label type */

/******************** trans.h ********************/

/* data types for transformation package */

typedef struct tma
{
	float a11, a12, a21, a22, a31, a32, a33;
	struct tma *prev, *next;
	INTBIG type;
	CHAR multiplied;
} tmatrix;

typedef tmatrix *transform;

/******************** shared.h ********************/

/* utility stuff for cif interpreter */


typedef CHAR *objectptr;		/* will hold a pointer to any object */

typedef struct
{
	INTBIG l,r,b,t;
} bbrecord;						/* bounding box */

typedef struct ste
{
	INTBIG st_symnumber;		/* symbol number for this entry */
	struct ste *st_overflow;	/* bucket ovflo */
	CHAR st_expanded,st_frozen,st_defined,st_dumped;
	bbrecord st_bb;				/* bb as if this symbol were called by itself */
	CHAR st_bbvalid;
	CHAR *st_name;
	INTBIG st_ncalls;			/* number of calls made by this symbol */
	objectptr st_guts;			/* pointer to linked list of objects */
} stentry;

typedef struct
{
	INTBIG sy_type;				/* type of object, must come first */
	bbrecord sy_bb;				/* bb must be next */
	objectptr sy_next;			/* for ll */
								/* layer is next, if applicable */
	INTBIG sy_symnumber;		/* rest is noncritical */
	stentry *sy_unid;
	CHAR *sy_name;
	tmatrix sy_tm;
	CHAR *sy_tlist;				/* trans list for this call */
} symcall;						/* symbol call object */

/* hack structure for referencing first fields of any object */
struct cifhack
{
	INTBIG kl_type;
	bbrecord kl_bb;
	struct cifhack *kl_next;	/* for ll */
	INTBIG kl_layer;			/* may or may not be present */
};

typedef struct
{
	INTBIG gn_type;				/* geometry name */
	bbrecord gn_bb;
	objectptr gn_next;
	INTBIG gn_layer;
	CHAR *gn_name;
	point gn_pos;
} gname;

typedef struct
{
	INTBIG la_type;
	bbrecord la_bb;
	objectptr la_next;
	CHAR *la_name;
	point la_pos;
} label;

typedef struct
{
	INTBIG bo_type;				/* type comes first */
	bbrecord bo_bb;				/* then bb */
	objectptr bo_next;			/* then link for ll */
	INTBIG bo_layer;			/* then layer */
	INTBIG bo_length,bo_width;
	point bo_center;
	INTBIG bo_xrot,bo_yrot;
} box;

typedef struct
{
	INTBIG mb_type;				/* type comes first */
	bbrecord mb_bb;				/* then bb */
	objectptr mb_next;			/* then link for ll */
	INTBIG mb_layer;			/* then layer */
} mbox;

typedef struct
{
	INTBIG fl_type;
	bbrecord fl_bb;
	objectptr fl_next;			/* for ll */
	INTBIG fl_layer;
	point fl_center;
	INTBIG fl_diameter;
} flash;

typedef struct
{
	INTBIG po_type;
	bbrecord po_bb;
	objectptr po_next;			/* for ll */
	INTBIG po_layer;
	INTBIG po_numpts;			/* length of path, points follow */
	point po_p[1];				/* array of points in path */
} polygon;

typedef struct
{
	INTBIG wi_type;
	bbrecord wi_bb;
	objectptr wi_next;			/* for ll */
	INTBIG wi_layer;
	INTBIG wi_width;
	INTBIG wi_numpts;			/* length of path, points follow */
	point wi_p[1];				/* array of points in path */
} wire;

/************ types.c **********/

typedef struct lp
{
	point pvalue;
	struct lp *pnext;
} linkedpoint;

typedef struct pathrecord
{
	linkedpoint *pfirst,*plast;
	INTBIG plength;
} *path;

/* types for tlists */
typedef struct lt
{
	tentry tvalue;
	struct lt *tnext;
} linkedtentry;

typedef struct trecord
{
	linkedtentry *tfirst,*tlast;
	INTBIG tlength;
} *tlist;

/* prototypes for intramodule interface */
INTBIG  io_doneinterpreter(void);
INTBIG  io_initparser(void);
BOOLEAN io_infromfile(FILE*);
INTBIG  io_parsefile(void);
INTBIG  io_doneparser(void);
INTBIG  io_fatalerrors(void);
void    io_iboundbox(INTBIG*, INTBIG*, INTBIG*, INTBIG*);
BOOLEAN io_createlist(void);
INTBIG  io_initinterpreter(void);
INTBIG  io_pathlength(path);
point   io_removepoint(path);
void    io_bbflash(INTBIG, INTBIG, point, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
void    io_bbbox(INTBIG, INTBIG, INTBIG, point, INTBIG, INTBIG, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
CHAR   *io_makepath(void);
void    io_freepath(path);
BOOLEAN io_appendpoint(path, point);
tentry io_removetentry(tlist);
INTBIG io_tlistlength(tlist);
INTBIG io_findlayernum(CHAR*);
void io_outputusercommand(INTBIG, CHAR*);
void io_outputds(INTBIG, CHAR*, INTBIG, INTBIG, INTBIG, INTBIG);
void io_outputpolygon(INTBIG, CHAR*);
void io_outputwire(INTBIG, INTBIG, CHAR*);
void io_outputbox(INTBIG, INTBIG, INTBIG, point, INTBIG, INTBIG);
void io_outputflash(INTBIG, INTBIG, point);
void io_outputgeoname(CHAR*, point, INTBIG);
void io_outputlabel(CHAR*, point);
void io_outputcall(INTBIG, CHAR*, CHAR*);
void io_outputdf(void);
