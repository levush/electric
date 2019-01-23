/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: ioedifi.c
 * Input/output tool: EDIF 2 0 0 netlist reader
 * Written by: Glen Lawson
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

#define IGNORESTRINGSWITHSQUAREBRACKETS 1		/* uncomment to ignore strings starting with "[" */
#define SLOPDISTANCE                    1		/* allow connections when this many lambda away (0 to force exact placement) */

/*
 * Notes (11/12/92):
 *	I have tried EDIF files from CADENCE and VALID only.
 *	Does not fully support portbundles
 *	Multiple ports of the same name are named port_x (x is 1 to n duplicate)
 *	Keywords such as ARRAY have unnamed parameters, ie (array (name..) 5 6)
 *	this is handled in the io_edprocess_integer function called by io_edget_keyword,
 *	this is a hack to fix this problem, a real table driven parser should be used.
 * Improved (3/9/92) by Steven Rubin:
 *	handle progress dialogs during input
 *	changed "malloc" and "free" to "emalloc" and "efree"
 *    memory allocation bug in "io_edpop_stack()" caused crash when current technology gas GDS layers
 * gml (1/94)
 *	Use circle arcs instead of splines.
 *	Support text justifications and text height
 * 	Better NAME/RENAME/STRINGDISPLAY/ANNOTATE text handling.
 * gml (3/94)
 *  ANSI prototypes
 * gml (9/94)
 *	Changed arcs to simple polygons plus ARC attribute
 * gml (12/94)
 *  Can read NETLIST views
 */

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "efunction.h"
#include "database.h"
#include "edialogs.h"
#include "tecgen.h"
#include "tecart.h"
#include "tecschem.h"
#include "eio.h"
#include "network.h"
#include "usr.h"
#include <math.h>

/* some length defines */
#define LINE      4096
#define WORD      4096
#define MAXLAYERS  256

#define LONGJMPEOF       1		/* error: "Unexpected end-of-file" */
#define LONGJMPNOMEM     2		/* error: "No memory" */
#define LONGJMPPTMIS     3		/* error: "Point list mismatch" */
#define LONGJMPNOLIB     4		/* error: "Could not create library" */
#define LONGJMPNOINT     5		/* error: "No integer value" */
#define LONGJMPILLNUM    6		/* error: "Illegal number value" */
#define LONGJMPNOMANT    7		/* error: "No matissa value" */
#define LONGJMPNOEXP     8		/* error: "No exponent value" */
#define LONGJMPILLNAME   9		/* error: "Illegal name" */
#define LONGJMPILLDELIM 10		/* error: "Illegal delimeter" */

typedef enum
{
	UPPERLEFT,
	UPPERCENTER,
	UPPERRIGHT,
	CENTERLEFT,
	CENTERCENTER,
	CENTERRIGHT,
	LOWERLEFT,
	LOWERCENTER,
	LOWERRIGHT
} TEXTJUST;

/* generic defines */
static CHAR *DELIMETERS = x_(" \t\r\n=.:;'()}\"");

/* name table for layers */
#define NONAMETABLE ((NAMETABLE_PTR)0)
typedef struct
{
	CHAR *original;					/* the original MASK layer */
	CHAR *replace;					/* the replacement layer */
	NODEPROTO *node;				/* the basic electric node */
	ARCPROTO *arc;					/* the basic arc type */
	int textheight;					/* default text height */
	TEXTJUST justification;			/* default justification */
	int visible; 					/* layer is visible */
} NAMETABLE, *NAMETABLE_PTR;

/* the following is a table of EDIF keywords, and functions */
typedef enum
{
	KUNKNOWN,
	KINIT,
	KANNOTATE,
	KARC,
	KARRAY,
	KAUTHOR,
	KBOOLEAN,
	KBORDERWIDTH,
	KBOUNDINGBOX,
	KCELL,
	KCELLREF,
	KCELLTYPE,
	KCIRCLE,
	KCOLOR,
	KCONTENTS,
	KCOMMENT,
	KCOMMENTGRAPHICS,
	KCONNECTLOCATION,
	KCORNERTYPE,
	KCURVE,
	KDATAORIGIN,
	KDCFANOUTLOAD,
	KDCMAXFANOUT,
	KDELTA,
	KDESIGN,
	KDESIGNATOR,
	KDIRECTION,
	KDISPLAY,
	KDOT,
	KEDIF,
	KEDIFLEVEL,
	KEDIFVERSION,
	KENDTYPE,
	KEXTERNAL,
	KFABRICATE,
	KFALSE,
	KFIGURE,
	KFIGUREGROUP,
	KFIGUREGROUPOVERRIDE,
	KFIGUREGROUPREF,
	KFILLPATTERN,
	KGRIDMAP,
	KINSTANCE,
	KINSTANCEREF,
	KINTEGER,
	KINTERFACE,
	KJOINED,
	KJUSTIFY,
	KKEYWORDDISPLAY,
	KEDIFKEYMAP,
	KEDIFKEYLEVEL,
	KLIBRARY,
	KLIBRARYREF,
	KLISTOFNETS,
	KLISTOFPORTS,
	KLOGICREF,
	KMEMBER,
	KMUSTJOIN,
	KNAME,
	KNET,
	KNETBUNDLE,
	KNETREF,
	KNUMBER,
	KNUMBERDEFINITION,
	KORIENTATION,
	KORIGIN,
	KOWNER,
	KPAGE,
	KPAGESIZE,
	KPARAMETER,
	KPATH,
	KPATHWIDTH,
	KPOINT,
	KPOINTLIST,
	KPOLYGON,
	KPORT,
	KPORTBUNDLE,
	KPORTIMPLEMENTATION,
	KPORTINSTANCE,
	KPORTLIST,
	KPORTREF,
	KPROGRAM,
	KPROPERTY,
	KPROPERTYDISPLAY,
	KPT,
	KNAMEDEF,
	KOPENSHAPE,
	KRECTANGLE,
	KRENAME,
	KSCALE,
	KSCALEDINTEGER,
	KSCALEX,
	KSCALEY,
	KSITE,
	KSHAPE,
	KSTATUS,
	KSTRING,
	KSTRINGDISPLAY,
	KSYMBOL,
	KTECHNOLOGY,
	KTEXTHEIGHT,
	KTIMESTAMP,
	KTRANSFORM,
	KTRUE,
	KUNIT,
	KUSERDATA,
	KVERSION,
	KVIEW,
	KVIEWLIST,
	KVIEWREF,
	KVIEWTYPE,
	KVISIBLE,
	KWEAKJOINED,
	KWRITTEN
} KSTATE;

#define KNULL ((KSTATE *) 0)
typedef struct
{
	CHAR *name;							/* the name of the keyword */
	INTBIG (*func)(void);				/* the function to execute */
	KSTATE next;						/* the next state, if any */
	KSTATE *state;						/* edif state */
} EDIFKEY, *EDIFKEY_PTR;
#define NOEDIFKEY ((EDIFKEY_PTR) 0)

/* Edif viewtypes ... */
typedef enum
{
	VNULL,
	VBEHAVIOR,
	VDOCUMENT,
	VGRAPHIC,
	VLOGICMODEL,
	VMASKLAYOUT,
	VNETLIST,
	VPCBLAYOUT,
	VSCHEMATIC,
	VSTRANGER,
	VSYMBOLIC,
	VSYMBOL 	/* not a real EDIF view, but electric has one */
} VTYPES;

/* Edif geometry types ... */
typedef enum
{
	GUNKNOWN,
	GRECTANGLE,
	GPOLYGON,
	GSHAPE,
	GOPENSHAPE,
	GTEXT,
	GPATH,
	GINSTANCE,
	GCIRCLE,
	GARC,
	GPIN,
	GNET,
	GBUS
} GTYPES;

/* 8 standard orientations */
typedef enum
{
	OUNKNOWN,
	OR0,
	OR90,
	OR180,
	OR270,
	OMX,
	OMY,
	OMYR90,
	OMXR90
} OTYPES;

typedef struct _ept
{
	int x, y;
	struct _ept *nextpt;
} EPT, *EPT_PTR;
#define NOEPT ((EPT_PTR)0)

typedef enum {INOUT, INPUTE, OUTPUTE} EDPORTDIR;

#define INCH (10*el_curlib->lambda[io_edifgbl.technology->techindex])

typedef struct _edport
{
	CHAR *name;
	CHAR *reference;
	EDPORTDIR direction;
	int arrayx, arrayy;
	struct _edport *next;
} EDPORT, *EDPORT_PTR;
#define NOEDPORT ((EDPORT_PTR)0)

typedef struct _ednetport
{
	NODEINST *ni;					/* unique node port is attached to */
	PORTPROTO *pp;					/* port type (no portarc yet) */
	int member;						/* member for an array */
	struct _ednetport *next;		/* next common to a net */
} EDNETPORT, *EDNETPORT_PTR;
#define NOEDNETPORT ((EDNETPORT_PTR)0)

typedef enum
{
	EVUNKNOWN,
	EVCADENCE,
	EVVALID,
	EVSYNOPSYS,
	EVMENTOR,
	EVVIEWLOGIC
} EDVENDOR;

#define MAXSHEETS 6
typedef enum
{
	SHEET_ASIZE = 0,
	SHEET_BSIZE = 1,
	SHEET_CSIZE = 2,
	SHEET_DSIZE = 3,
	SHEET_ESIZE = 4,
	SHEET_INFSIZE = 5
} SHEET_SIZE;

typedef enum
{
	PUNKNOWN,
	PSTRING,
	PINTEGER,
	PNUMBER
} PROPERTY_TYPE;

typedef struct _edproperty
{
	CHAR *name;
	union
	{
		int integer;
		float number;
		CHAR *string;
	} val;
	PROPERTY_TYPE type;
	struct _edproperty *next;
} EDPROPERTY, *EDPROPERTY_PTR;
#define NOEDPROPERTY ((EDPROPERTY_PTR)0)

/* edifgbl declaration */
typedef struct
{
	/* general information variables */
	FILE *edif_file;             	/* the input file */
	jmp_buf env;					/* longjmp return */

	/* view information ... */
	int pageno;						/* the schematic page number */
	VTYPES active_view;				/* indicates we are in a NETLIST view */
	int drawing_number;				/* the current drawing number */
	int p_ref, c_ref;				/* peripheral and core reference names */
	EDVENDOR vendor;				/* the current vendor type */

	/* parser variables ... */
	KSTATE state;			    	/* the current parser state */
	int lineno;                  	/* the current line number */
	CHAR buffer[LINE+1];         	/* the read buffer */
	CHAR *pos;                   	/* the position within the buffer */
	int ignoreblock;				/* no update flag */
	CHAR *delimeters;				/* parsing delimeters for edif */
	int errors, warnings;			/* load status */

	/* electric context data ... */
	LIBRARY *library;				/* the new library */
	TECHNOLOGY *technology;			/* the active technology */
	NODEPROTO *current_cell;		/* the current active cell */
	NODEINST *current_node;			/* the current active instance */
	ARCINST *current_arc;			/* the current active arc */
	NODEPROTO *figure_group;		/* the current figure group node */
	ARCPROTO *arc_type;				/* the current (if exists) arc type */
	NODEPROTO *proto;				/* the cellRef type */
	PORTPROTO *current_port;		/* the current port proto */

	/* general geometry information ... */
	GTYPES geometry;				/* the current geometry type */
	EPT *points;					/* the list of points */
	EPT *lastpt;					/* the end of the list */
	int pt_count;					/* the count of points */
	OTYPES orientation;				/* the orientation of the structure */
	EDPORTDIR direction;			/* port direction */

	/* geometric path constructors ... */
	int path_width;					/* the width of the path */
	int extend_corner;				/* extend path corner flag */
	int extend_end;              	/* extend path end flag */

	/* array variables ... */
	int isarray;					/* set if truely an array */
	int arrayx, arrayy;				/* the bounds of the array */
	int deltaxX, deltaxY;			/* offsets in x and y for an X increment */
	int deltayX, deltayY;			/* offsets in x and y for an Y increment */
	int deltapts;					/* which offset flag */
	int memberx, membery;			/* the element of the array */

	/* text variables ... */
	CHAR string[LINE+1];			/* Text string */
	int textheight;					/* the current default text height */
	TEXTJUST justification;			/* the current text justificaton */
	int visible;					/* is stringdisplay visible */
	struct {						/* save block for name and rename strings */
		CHAR string[LINE+1];		/* the saved string, if NULL not saved */
		EPT *points;				/* origin x and y */
		int pt_count;				/* count of points (usually 1) */
		int textheight;				/* the height of text */
		int visible;				/* visiblity of the text */
		TEXTJUST justification; 	/* justification of the text */
		OTYPES orientation;			/* orientation of the text */
	} save_text;

	/* technology data ... */
	double scale;					/* scaling value */
	double dbunit;					/* database units */
	double meters;					/* per number of meters */
	double val1, val2;				/* temporary storage */

	/* current name and rename of EDIF objects ... */
	CHAR cell_reference[WORD+1];	/* the current cell name */
	CHAR cell_name[WORD+1];			/* the current cell name (original) */
	CHAR port_reference[WORD+1];	/* the current port name */
	CHAR port_name[WORD+1];			/* the current port name (original) */
	CHAR instance_reference[WORD+1]; /* the current instance name */
	CHAR instance_name[WORD+1];		/* the current instance name (original) */
	CHAR bundle_reference[WORD+1];	/* the current bundle name */
	CHAR bundle_name[WORD+1];		/* the current bundle name (original) */
	CHAR net_reference[WORD+1];		/* the current net name */
	CHAR net_name[WORD+1];			/* the current net name (original) */
	CHAR property_reference[WORD+1]; /* the current property name */
	CHAR property_name[WORD+1];		/* the current property name (original) */

	PROPERTY_TYPE ptype;			/* the type of property */
	union {
		CHAR *string;				/* string buffer */
		int integer;				/* integer buffer */
		float number;				/* number buffer */
	} pval;

	CHAR name[WORD+1];				/* the name of the object */
	CHAR original[WORD+1];			/* the original name of the object */

	/* layer or figure information ... */
	int layer_ptr;					/* pointer to layer table entry */
	NAMETABLE_PTR nametbl[MAXLAYERS]; /* the current name mapping table */
	NAMETABLE_PTR cur_nametbl; 		/* the current name table entry */

	/* cell name lookup (from rename) */
	NAMETABLE_PTR *celltbl;			/* the cell lookup table */
	int celltbl_cnt;				/* the count of entries in the cell table */
	int celltbl_sze;				/* maximum size of the table */

	/* port data for port exporting */
	EDPORT_PTR ports;				/* active port list */
	EDPORT_PTR free_ports;			/* the list of free ports */

	/* property data for all objects */
	EDPROPERTY_PTR properties;		/* active property list */
	EDPROPERTY_PTR free_properties;	/* the list of free properties */

	/* net constructors */
	EDNETPORT_PTR firstnetport;		/* the list of ports on a net */
	EDNETPORT_PTR lastnetport; 		/* the last in the list on a net */
	EDNETPORT_PTR free_netports;	/* the list of free ports */

	/* view NETLIST layout */
	SHEET_SIZE sheet_size;			/* the size of the target sheet */
	int sh_xpos, sh_ypos;			/* current sheet position */
	int sh_offset;					/* next x offset */
	int ipos, bpos, opos;			/* current position pointers */
} EDIFGBL;

BOOLEAN io_edgblinited = FALSE;

static EDIFGBL io_edifgbl;

/* prototypes for local routines */
static INTBIG      io_edload_edif(INTBIG key_count, EDIFKEY_PTR keywords);
static INTBIG      io_edfreesavedptlist(void);
static double      io_edgetnumber(void);
static INTBIG      io_edcheck_name(void);
static INTBIG      io_edpop_stack(void);
static void        io_edprocess_integer(INTBIG value);
static INTSML      io_edgetrot(OTYPES orientation);
static INTSML      io_edgettrans(OTYPES orientation);
static int         io_edcompare_name(const void *name1, const void *name2);
static INTBIG      io_edis_integer(CHAR *buffer);
static CHAR       *io_edget_keyword(CHAR *buffer);
static CHAR       *io_edget_token(CHAR *buffer, CHAR idelim);
static CHAR       *io_edpos_token(void);
static BOOLEAN     io_edget_line(CHAR *line, INTBIG limit, FILE *file);
static void        io_edget_delim(CHAR delim);
static INTBIG      io_edallocport(void);
static INTBIG      io_edallocproperty(CHAR *name, PROPERTY_TYPE type, int integer,
						float number, CHAR *string);
static INTBIG      io_edallocnetport(void);
static void        io_edeq_of_a_line(double sx, double sy, double ex, double ey, double *A, double *B, double *C);
static INTBIG      io_eddetermine_intersection(double A[2], double B[2], double C[2], double *x, double *y);
static void        io_ededif_arc(INTBIG x[3], INTBIG y[3], INTBIG *xc, INTBIG *yc, INTBIG *r, double *so,
						double *ar, INTBIG *rot, INTBIG *trans);
static NODEPROTO  *io_edmakeiconcell(PORTPROTO *fpp, CHAR *iconname, CHAR *pt, LIBRARY *lib);
static void        io_ednamearc(ARCINST *ai);
static INTBIG      io_edfindport(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap, NODEINST *nis[],
						PORTPROTO *pps[]);
static INTBIG      io_edfindport_geom(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap,
						NODEINST *nis[], PORTPROTO *pps[], ARCINST **ai);
static void        io_edfreenetports(void);
static INTBIG      io_ediftextsize(INTBIG textheight);
static NODEINST   *io_edifiplacepin(NODEPROTO *np, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy,
						INTBIG trans, INTBIG angle, NODEPROTO *parent);

/* keyword parser entry points, only these keywords are supported by this parser,
   if you need more, add them. Each entry point can have an exit point in the
   function io_edpop_stack. This is processed when ')' is encountered in the EDIF
   file.
 */
static INTBIG io_ednoop(void);
static INTBIG io_edtechnology(void);
static INTBIG io_edfabricate(void);
static INTBIG io_edpage(void);
static INTBIG io_edfigure(void);
static INTBIG io_edfigureGroupOverride(void);
static INTBIG io_edscale(void);
static INTBIG io_edendType(void);
static INTBIG io_edcornerType(void);
static INTBIG io_edstringDisplay(void);
static INTBIG io_edtrue(void);
static INTBIG io_edfalse(void);
static INTBIG io_edgetlayer(void);
static INTBIG io_edpointList(void);
static INTBIG io_edpoint(void);
static INTBIG io_edjustify(void);
static INTBIG io_edtextHeight(void);
static INTBIG io_edpt(void);
static INTBIG io_edorientation(void);
static INTBIG io_edpathWidth(void);
static INTBIG io_edrectangle(void);
static INTBIG io_edcircle(void);
static INTBIG io_edpath(void);
static INTBIG io_edpolygon(void);
static INTBIG io_edopenShape(void);
static INTBIG io_edfreeptlist(void);
static INTBIG io_edlibrary(void);
static INTBIG io_eddesign(void);
static INTBIG io_edcell(void);
static INTBIG io_edview(void);
static INTBIG io_edviewType(void);
static INTBIG io_edcontents(void);
static INTBIG io_edport(void);
static INTBIG io_eddirection(void);
static INTBIG io_edinstance(void);
static INTBIG io_edarray(void);
static INTBIG io_eddelta(void);
static INTBIG io_edcellRef(void);
static INTBIG io_edproperty(void);
static INTBIG io_ednet(void);
static INTBIG io_ednetBundle(void);
static INTBIG io_edjoined(void);
static INTBIG io_edportRef(void);
static INTBIG io_edinstanceRef(void);
static INTBIG io_edmember(void);
static INTBIG io_edname(void);
static INTBIG io_edrename(void);
static INTBIG io_edboundingBox(void);
static INTBIG io_edsymbol(void);
static INTBIG io_edportImplementation(void);
static INTBIG io_edconnectLocation(void);
static INTBIG io_eddot(void);
static INTBIG io_edprogram(void);
static INTBIG io_edunit(void);
static INTBIG io_edinteger(void);
static INTBIG io_ednumber(void);
static INTBIG io_edstring(void);
static INTBIG io_edinterface(void);

/* Parser keyword state tables, only some are provided, others are simply incomplete.
   If you have time, please feel free to complete these keyword tables and the following
   parse array. Note that a bad state does not cause the parser to quit, the action is
   still executed.
 */
static KSTATE Sarray[] = {KINSTANCE, KPORT, KNET, KUNKNOWN};
static KSTATE Sauthor[] = {KWRITTEN, KUNKNOWN};
static KSTATE SboundingBox[] = {KSYMBOL, KCONTENTS, KUNKNOWN};
static KSTATE Scell[] = {KEXTERNAL, KLIBRARY, KUNKNOWN};
static KSTATE ScellRef[] = {KDESIGN, KVIEWREF, KINSTANCE, KUNKNOWN};
static KSTATE ScellType[] = {KCELL, KUNKNOWN};
static KSTATE Scontents[] = {KVIEW, KUNKNOWN};
static KSTATE Sdesign[] = {KEDIF, KUNKNOWN};
static KSTATE Sdirection[] = {KPORT, KUNKNOWN};
static KSTATE Sedif[] = {KINIT, KUNKNOWN};
static KSTATE SedifLevel[] = {KEDIF, KEXTERNAL, KLIBRARY, KUNKNOWN};
static KSTATE SedifVersion[] = {KEDIF, KUNKNOWN};
static KSTATE Sinstance[] = {KCONTENTS, KPAGE, KPORTIMPLEMENTATION, KCOMMENTGRAPHICS, KUNKNOWN};
static KSTATE SinstanceRef[] = {KINSTANCEREF, KPORTREF, KUNKNOWN};
static KSTATE Sinterface[] = {KVIEW, KUNKNOWN};
static KSTATE Sjoined[] = {KINTERFACE, KNET, KMUSTJOIN, KWEAKJOINED, KUNKNOWN};
static KSTATE SkeywordMap[] = {KEDIF, KUNKNOWN};
static KSTATE Slibrary[] = {KEDIF, KUNKNOWN};
static KSTATE SlibraryRef[] = {KCELLREF, KFIGUREGROUPREF, KLOGICREF, KUNKNOWN};
static KSTATE SlistOfNets[] = {KNETBUNDLE, KUNKNOWN};
static KSTATE Snet[] = {KCONTENTS, KPAGE, KLISTOFNETS, KUNKNOWN};
static KSTATE SnetBundle[] = {KCONTENTS, KPAGE, KUNKNOWN};
static KSTATE SnumberDefinition[] = {KTECHNOLOGY, KUNKNOWN};
static KSTATE Sport[] = {KINTERFACE, KLISTOFPORTS, KUNKNOWN};
static KSTATE Sscale[] = {KNUMBERDEFINITION, KUNKNOWN};
static KSTATE Sstatus[] = {KCELL, KDESIGN, KEDIF, KEXTERNAL, KLIBRARY, KVIEW, KUNKNOWN};
static KSTATE Ssymbol[] = {KINTERFACE, KUNKNOWN};
static KSTATE Stechnology[] = {KEXTERNAL, KLIBRARY, KUNKNOWN};
static KSTATE Stimestamp[] = {KWRITTEN, KUNKNOWN};
static KSTATE Sunit[] = {KPARAMETER, KPROPERTY, KSCALE, KUNKNOWN};
static KSTATE Sview[] = {KCELL, KUNKNOWN};
static KSTATE SviewRef[] = {KINSTANCE, KINSTANCEREF, KNETREF, KPORTREF, KSITE, KVIEWLIST, KUNKNOWN};
static KSTATE SviewType[] = {KVIEW, KUNKNOWN};
static KSTATE Swritten[] = {KSTATUS, KUNKNOWN};

/* To add a new keyword, insert into this sorted list and into the enumerated keyword
   list in ioedif.h. If no action is required, just use the io_ednoop as the action
   routine. If some action is required, add a new function. Note that all operations
   are dependent on the current state. If a keyword is correct, but the required state
   table is not, add it to the above data structure (_kstate). Or make it non-state
   dependent with a KNULL in the last field.
 */
static EDIFKEY edif_keywords[] =
{
	{x_("annotate"), io_ednoop, KANNOTATE, KNULL},
	{x_("arc"), io_ednoop, KARC, KNULL},
	{x_("array"), io_edarray, KARRAY, Sarray},
	{x_("author"), io_ednoop, KAUTHOR, Sauthor},
	{x_("boolean"), io_ednoop, KBOOLEAN, KNULL},
	{x_("borderWidth"), io_ednoop, KBORDERWIDTH, KNULL},
	{x_("boundingBox"), io_edboundingBox, KBOUNDINGBOX, SboundingBox},
	{x_("cell"), io_edcell, KCELL, Scell },
	{x_("cellRef"), io_edcellRef, KCELLREF, ScellRef},
	{x_("cellType"), io_ednoop, KCELLTYPE, ScellType},
	{x_("circle"), io_edcircle, KCIRCLE, KNULL},
	{x_("color"), io_ednoop, KCOLOR, KNULL},
	{x_("comment"), io_ednoop, KCOMMENT, KNULL},
	{x_("commentGraphics"), io_ednoop, KCOMMENTGRAPHICS, KNULL},
	{x_("connectLocation"), io_edconnectLocation, KCONNECTLOCATION, KNULL},
	{x_("contents"), io_edcontents, KCONTENTS, Scontents},
	{x_("cornerType"), io_edcornerType, KCORNERTYPE, KNULL},
	{x_("curve"), io_ednoop, KCURVE, KNULL},
	{x_("dataOrigin"), io_ednoop, KDATAORIGIN, KNULL},
	{x_("dcFanoutLoad"), io_ednoop, KDCFANOUTLOAD, KNULL},
	{x_("dcMaxFanout"), io_ednoop, KDCMAXFANOUT, KNULL},
	{x_("delta"), io_eddelta, KDELTA, KNULL},
	{x_("design"), io_eddesign, KDESIGN, Sdesign},
	{x_("designator"), io_ednoop, KDESIGNATOR, KNULL},
	{x_("direction"), io_eddirection, KDIRECTION, Sdirection},
	{x_("display"), io_edfigure, KDISPLAY, KNULL},
	{x_("dot"), io_eddot, KDOT, KNULL},
	{x_("e"), io_ednoop, KSCALEDINTEGER, KNULL},
	{x_("edif"), io_ednoop, KEDIF, Sedif},
	{x_("edifLevel"), io_ednoop, KEDIFLEVEL, SedifLevel},
	{x_("edifVersion"), io_ednoop, KEDIFVERSION, SedifVersion},
	{x_("endType"), io_edendType, KENDTYPE, KNULL},
	{x_("external"), io_edlibrary, KEXTERNAL, Slibrary},
	{x_("fabricate"), io_edfabricate, KFABRICATE, KNULL},
	{x_("false"), io_edfalse, KFALSE, KNULL},
	{x_("figure"), io_edfigure, KFIGURE, KNULL},
	{x_("figureGroup"), io_ednoop, KFIGUREGROUP, KNULL},
	{x_("figureGroupOverride"), io_edfigureGroupOverride, KFIGUREGROUPOVERRIDE, KNULL},
	{x_("fillpattern"), io_ednoop, KFILLPATTERN, KNULL},
	{x_("gridMap"), io_ednoop, KGRIDMAP, KNULL},
	{x_("instance"), io_edinstance, KINSTANCE, Sinstance},
	{x_("instanceRef"), io_edinstanceRef, KINSTANCEREF, SinstanceRef},
	{x_("integer"), io_edinteger, KINTEGER, KNULL},
	{x_("interface"), io_edinterface, KINTERFACE, Sinterface},
	{x_("joined"), io_edjoined, KJOINED, Sjoined},
	{x_("justify"), io_edjustify, KJUSTIFY, KNULL},
	{x_("keywordDisplay"), io_ednoop, KKEYWORDDISPLAY, KNULL},
	{x_("keywordLevel"), io_ednoop, KEDIFKEYLEVEL, KNULL},
	{x_("keywordMap"), io_ednoop, KEDIFKEYMAP, SkeywordMap},
	{x_("library"), io_edlibrary, KLIBRARY, Slibrary},
	{x_("libraryRef"), io_ednoop, KLIBRARYREF, SlibraryRef},
	{x_("listOfNets"), io_ednoop, KLISTOFNETS, SlistOfNets},
	{x_("listOfPorts"), io_ednoop, KLISTOFPORTS, KNULL},
	{x_("member"), io_edmember, KMEMBER, KNULL},
	{x_("name"), io_edname, KNAME, KNULL},
	{x_("net"), io_ednet, KNET, Snet},
	{x_("netBundle"), io_ednetBundle, KNETBUNDLE, SnetBundle},
	{x_("number"), io_ednumber, KNUMBER, KNULL},
	{x_("numberDefinition"), io_ednoop, KNUMBERDEFINITION, SnumberDefinition},
	{x_("openShape"), io_edopenShape, KOPENSHAPE, KNULL},
	{x_("orientation"), io_edorientation, KORIENTATION, KNULL},
	{x_("origin"), io_ednoop, KORIGIN, KNULL},
	{x_("owner"), io_ednoop, KOWNER, KNULL},
	{x_("page"), io_edpage, KPAGE, KNULL},
	{x_("pageSize"), io_ednoop, KPAGESIZE, KNULL},
	{x_("path"), io_edpath, KPATH, KNULL},
	{x_("pathWidth"), io_edpathWidth, KPATHWIDTH, KNULL},
	{x_("point"), io_edpoint, KPOINT, KNULL},
	{x_("pointList"), io_edpointList, KPOINTLIST, KNULL},
	{x_("polygon"), io_edpolygon, KPOLYGON, KNULL},
	{x_("port"), io_edport, KPORT, Sport},
	{x_("portBundle"), io_ednoop, KPORTBUNDLE, KNULL},
	{x_("portImplementation"), io_edportImplementation, KPORTIMPLEMENTATION, KNULL},
	{x_("portInstance"), io_ednoop,KPORTINSTANCE, KNULL},
	{x_("portList"), io_ednoop, KPORTLIST, KNULL},
	{x_("portRef"), io_edportRef, KPORTREF, KNULL},
	{x_("program"), io_edprogram, KPROGRAM, KNULL},
	{x_("property"), io_edproperty, KPROPERTY, KNULL},
	{x_("propertyDisplay"), io_ednoop, KPROPERTYDISPLAY, KNULL},
	{x_("pt"), io_edpt, KPT, KNULL},
	{x_("rectangle"), io_edrectangle, KRECTANGLE, KNULL},
	{x_("rename"), io_edrename, KRENAME, KNULL},
	{x_("scale"), io_edscale, KSCALE, Sscale},
	{x_("scaleX"), io_ednoop, KSCALEX, KNULL},
	{x_("scaleY"), io_ednoop, KSCALEY, KNULL},
	{x_("shape"), io_ednoop, KSHAPE, KNULL},
	{x_("status"), io_ednoop, KSTATUS, Sstatus},
	{x_("string"), io_edstring, KSTRING, KNULL},
	{x_("stringDisplay"), io_edstringDisplay, KSTRINGDISPLAY, KNULL},
	{x_("symbol"), io_edsymbol, KSYMBOL, Ssymbol},
	{x_("technology"), io_edtechnology, KTECHNOLOGY, Stechnology},
	{x_("textHeight"), io_edtextHeight, KTEXTHEIGHT, KNULL},
	{x_("timestamp"), io_ednoop, KTIMESTAMP, Stimestamp},
	{x_("transform"), io_ednoop, KTRANSFORM, KNULL},
	{x_("true"), io_edtrue, KTRUE, KNULL},
	{x_("unit"), io_edunit, KUNIT, Sunit},
	{x_("userData"), io_ednoop, KUSERDATA, KNULL},
	{x_("version"), io_ednoop, KVERSION, KNULL},
	{x_("view"), io_edview, KVIEW, Sview},
	{x_("viewRef"), io_ednoop, KVIEWREF, SviewRef},
	{x_("viewType"), io_edviewType, KVIEWTYPE, SviewType},
	{x_("visible"), io_ednoop, KVISIBLE, KNULL},
	{x_("written"), io_ednoop, KWRITTEN, Swritten},
	{0,0,KUNKNOWN,0}
};

/* edif keyword stack, this stack matches the current depth of the EDIF file,
   for the keyword instance it would be KEDIF, KLIBRARY, KCELL, KCONTENTS, KINSTANCE
 */
static KSTATE kstack[1000];
static INTBIG kstack_ptr = 0;
static INTBIG filelen, filepos;

/* some standard artwork primitivies */
static PORTPROTO *default_port = NOPORTPROTO;
static PORTPROTO *default_iconport = NOPORTPROTO;
static PORTPROTO *default_busport = NOPORTPROTO;
static PORTPROTO *default_input = NOPORTPROTO;
static PORTPROTO *default_output = NOPORTPROTO;
       INTBIG     EDIF_name_key = 0;
static INTBIG     EDIF_array_key = 0;
static INTBIG     EDIF_annotate_key = 0;
static void      *io_edifprogressdialog;
static struct
{
	int width;
	int height;
} EDIF_sheet_bounds[MAXSHEETS] =
{
	{8, 10},
	{16, 10},
	{16, 20},
	{32, 20},
	{32, 40},
	{64, 40}
};

#define RET_NOMEMORY() { ttyputnomemory(); return(1); }


/*
 * Routine to free all memory associated with this module.
 */
void io_freeedifinmemory(void)
{
	EDNETPORT_PTR nport, nnport;

	/* free the netport freelist */
	for (nport = io_edifgbl.free_netports; nport != NOEDNETPORT; nport = nnport)
	{
		nnport = nport->next;
		efree((CHAR *)nport);
	}
	if (io_edifgbl.ptype == PSTRING) efree(io_edifgbl.pval.string);
	io_edfreesavedptlist();
	io_edfreeptlist();
}

/*
 * routine to write a ".edif" file from the library "lib"
 */
BOOLEAN io_readediflibrary(LIBRARY *lib)
{
	INTBIG key_count, i;
	CHAR *filename, *msg;
	NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	io_edifgbl.edif_file = xopen(lib->libfile, io_filetypeedif, x_(""), &filename);
	if (io_edifgbl.edif_file == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}
	filelen = filesize(io_edifgbl.edif_file);
	filepos = 0;

	/* one-time inits */
	if (!io_edgblinited)
	{
		io_edifgbl.firstnetport = io_edifgbl.lastnetport = io_edifgbl.free_netports = NOEDNETPORT;
		io_edifgbl.ptype = PUNKNOWN;
		io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
		io_edifgbl.pt_count = 0;
		io_edifgbl.save_text.points = NOEPT;
		io_edifgbl.save_text.pt_count = 0;
		io_edgblinited = TRUE;
	}

	/* parser inits */
	io_edifgbl.state = KINIT;
	io_edifgbl.lineno = 1;
	io_edifgbl.buffer[0] = 0;
	io_edifgbl.pos = io_edifgbl.buffer;
	io_edifgbl.delimeters = DELIMETERS;
	io_edifgbl.errors = io_edifgbl.warnings = 0;
	io_edifgbl.ignoreblock = 0;
	io_edifgbl.vendor = EVUNKNOWN;

	/* general inits */
	var = getval((INTBIG)io_tool, VTOOL, VFLOAT, x_("IO_edif_input_scale"));
	if (var == NOVARIABLE) io_edifgbl.scale = 1.0; else
		io_edifgbl.scale = castfloat(var->addr);
	io_edifgbl.dbunit = 1.0;
	/*  io_edifgbl.meters = 0.000000001; */ /* default 1 db unit per nanometer */
	io_edifgbl.meters = scaletodispunit(1, DISPUNITCM) / 100.0;
	io_edifgbl.library = lib;
	io_edifgbl.technology = sch_tech;
	io_edifgbl.celltbl_sze = io_edifgbl.celltbl_cnt = 0;
	io_edifgbl.ports = io_edifgbl.free_ports = NOEDPORT;
	io_edifgbl.properties = io_edifgbl.free_properties = NOEDPROPERTY;
	io_edfreenetports();

	/* active database inits */
	io_edifgbl.current_cell = NONODEPROTO;
	io_edifgbl.current_node = NONODEINST;
	io_edifgbl.current_arc = NOARCINST;
	io_edifgbl.current_port = NOPORTPROTO;
	io_edifgbl.figure_group = NONODEPROTO;
	io_edifgbl.arc_type = NOARCPROTO;
	io_edifgbl.proto = NONODEPROTO;

	/* name inits */
	io_edifgbl.cell_reference[0] = 0;
	io_edifgbl.port_reference[0] = 0;
	io_edifgbl.instance_reference[0] = 0;
	io_edifgbl.bundle_reference[0] = 0;
	io_edifgbl.net_reference[0] = 0;
	io_edifgbl.property_reference[0] = 0;

	/* geometry inits */
	io_edifgbl.layer_ptr = 0;
	io_edifgbl.cur_nametbl = NONAMETABLE;
	io_edfreeptlist();

	/* text inits */
	io_edifgbl.textheight = 0;
	io_edifgbl.justification = LOWERLEFT;
	io_edifgbl.visible = 1;
	io_edifgbl.save_text.string[0] = 0;
	io_edfreesavedptlist();

	io_edifgbl.sheet_size = SHEET_DSIZE;
	io_edifgbl.sh_xpos = -1;
	io_edifgbl.sh_ypos = -1;

	kstack_ptr = 0;

	if (default_port == NOPORTPROTO) default_port = getportproto(sch_wirepinprim, x_("wire"));
	default_iconport = sch_wirepinprim->firstportproto;
	if (default_busport == NOPORTPROTO) default_busport = getportproto(sch_buspinprim, x_("bus"));
	if (default_input == NOPORTPROTO) default_input = getportproto(sch_offpageprim, x_("y"));
	if (default_output == NOPORTPROTO) default_output = getportproto(sch_offpageprim, x_("a"));

	if (EDIF_name_key == 0) EDIF_name_key = makekey(x_("EDIF_name"));
	if (EDIF_array_key == 0) EDIF_array_key = makekey(x_("EDIF_array"));
	if (EDIF_annotate_key == 0) EDIF_annotate_key = makekey(x_("EDIF_annotate"));

	/* now load the edif netlist */
	infstr = initinfstr();
	formatinfstr(infstr, _("Reading %s"), filename);
	msg = returninfstr(infstr);
	if (io_verbose < 0)
	{
		io_edifprogressdialog = DiaInitProgress(msg, 0);
		if (io_edifprogressdialog == 0)
		{
			xclose(io_edifgbl.edif_file);
			return(TRUE);
		}
		DiaSetProgress(io_edifprogressdialog, 0, filelen);
	} else ttyputmsg(msg);

	/* count the number of keywords */
	key_count = 0;
	while (edif_keywords[key_count].name != 0) key_count++;
	if (io_edload_edif(key_count, edif_keywords))
	{
		ttyputerr(_("error: bad design, aborting netlist translation"));
		if (io_verbose < 0) DiaDoneProgress(io_edifprogressdialog);
		return(TRUE);
	}
	if (io_edifgbl.errors || io_edifgbl.warnings)
		ttyputmsg(_("info: a total of %d errors, and %d warnings encountered during load"),
			io_edifgbl.errors, io_edifgbl.warnings);

	xclose(io_edifgbl.edif_file);
	if (io_verbose < 0)
	{
		DiaSetProgress(io_edifprogressdialog, 999, 1000);
		DiaSetTextProgress(io_edifprogressdialog, _("Cleaning Up..."));
	}

	/* free the layer table */
	for (i = 0; i < io_edifgbl.layer_ptr; i++)
	{
		if (io_edifgbl.nametbl[i]->original != io_edifgbl.nametbl[i]->replace)
			efree(io_edifgbl.nametbl[i]->replace);
		efree(io_edifgbl.nametbl[i]->original);
		efree((CHAR *)io_edifgbl.nametbl[i]);
	}
	/* note the master list is static */

	/* now the cell name table */
	for(i = 0; i < io_edifgbl.celltbl_cnt; i++)
	{
		if (io_edifgbl.celltbl[i]->original != io_edifgbl.celltbl[i]->replace)
			efree(io_edifgbl.celltbl[i]->replace);
		efree(io_edifgbl.celltbl[i]->original);
		efree((CHAR *)io_edifgbl.celltbl[i]);
	}

	/* and the master list ... */
	if (io_edifgbl.celltbl_cnt != 0)
	{
		efree((CHAR *)io_edifgbl.celltbl);
	} else
	{
		ttyputerr(_("error: no data"));
		if (io_verbose < 0) DiaDoneProgress(io_edifprogressdialog);
		return(TRUE);
	}

	if (io_edifgbl.library->curnodeproto == NONODEPROTO)
		io_edifgbl.library->curnodeproto = io_edifgbl.library->firstnodeproto;

	for (np = io_edifgbl.library->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		net_examinenodeproto(np);

	/* recompute bounds */
	(*el_curconstraint->solve)(NONODEPROTO);

	if (io_verbose < 0) DiaDoneProgress(io_edifprogressdialog);
	return(FALSE);
}

/*
 * Module: io_edload_edif
 * Function:  Will load the edif netlist into memory
 * Method:  Does a simple keyword lookup to load the lisp structured
 *		 EDIF language into Electric's database
 */
INTBIG io_edload_edif(INTBIG key_count, EDIFKEY_PTR keywords)
{
	KSTATE *state;
	CHAR *msg;
	CHAR *token, word[WORD+1];
	INTBIG low, high, mid, change, errcode;
	INTBIG keycount;
	INTBIG savestack;

	/* setup the error return location */
	errcode = setjmp(io_edifgbl.env);
	if (errcode != 0)
	{
		switch (errcode)
		{
			case LONGJMPEOF:      msg = _("Unexpected end-of-file");   break;
			case LONGJMPNOMEM:    msg = _("No memory");                break;
			case LONGJMPPTMIS:    msg = _("Point list mismatch");      break;
			case LONGJMPNOLIB:    msg = _("Could not create library"); break;
			case LONGJMPNOINT:    msg = _("No integer value");         break;
			case LONGJMPILLNUM:   msg = _("Illegal number value");     break;
			case LONGJMPNOMANT:   msg = _("No matissa value");         break;
			case LONGJMPNOEXP:    msg = _("No exponent value");        break;
			case LONGJMPILLNAME:  msg = _("Illegal name");             break;
			case LONGJMPILLDELIM: msg = _("Illegal delimeter");        break;
			default:              msg = _("Unknown error");            break;
		}
		ttyputerr(_("line #%d: %s"), io_edifgbl.lineno, msg);
		return(1);
	}

	savestack = -1;

	/* now read and parse the edif netlist */
	keycount = 0;
	while ((token = io_edget_keyword(word)) != NULL)
	{
		keycount++;
		if ((keycount%100) == 0)
		{
			if (io_verbose < 0) DiaSetProgress(io_edifprogressdialog, filepos, filelen); else
				ttyputmsg(_("%ld keywords read (%d%% done)"), keycount, filepos*100/filelen);
		}

		/* locate the keyword, and execute the function */
		low = 0;
		high = key_count - 1;
		while (low <= high)
		{
			mid = (low + high) / 2;
			change = namesame(token, keywords[mid].name);
			if (change < 0) high = mid - 1; else
				if (change > 0) low = mid + 1; else
			{
				/* found the function, check state */
				if (keywords[mid].state != KNULL && io_edifgbl.state != KUNKNOWN)
				{
					state = keywords[mid].state;
					while (*state && io_edifgbl.state != *state) state++;
					if (io_edifgbl.state != *state)
					{
						ttyputerr(_("error, line #%d: illegal state for keyword <%s>"),
							io_edifgbl.lineno, token);
						io_edifgbl.errors++;
					}
				}

				/* call the function */
				kstack[kstack_ptr++] = io_edifgbl.state;
				io_edifgbl.state = keywords[mid].next;
				if (savestack >= kstack_ptr)
				{
					savestack = -1;
					io_edifgbl.ignoreblock = 0;
				}
				if (!io_edifgbl.ignoreblock) (keywords[mid].func)();
				if (io_edifgbl.ignoreblock)
				{
					if (savestack == -1) savestack = kstack_ptr;
				}
				break;
			}
		}
		if (low > high)
		{
			ttyputerr(_("warning, line #%d: unknown keyword <%s>"), io_edifgbl.lineno, token);
			io_edifgbl.warnings++;
			kstack[kstack_ptr++] = io_edifgbl.state;
			io_edifgbl.state = KUNKNOWN;
		}
	}
	if (io_edifgbl.state != KINIT)
	{
		ttyputerr(_("line #%d: unexpected end-of-file encountered"), io_edifgbl.lineno);
		io_edifgbl.errors++;
	}
	return(0);
}

/* parser routines */
INTBIG io_ednoop(void)
{
	return(0);
}

INTBIG io_edtechnology(void)
{
	return(0);
}

INTBIG io_edfabricate(void)
{
	INTBIG pos;
	CHAR name[WORD+1];

	pos = io_edifgbl.layer_ptr;

	io_edifgbl.nametbl[pos] = (NAMETABLE_PTR)emalloc(sizeof(NAMETABLE), io_tool->cluster);
	if (io_edifgbl.nametbl[pos] == NONAMETABLE) RET_NOMEMORY();

	/* first get the original and replacement layers */
	if ((io_edget_token(name, 0)) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	if (allocstring(&io_edifgbl.nametbl[pos]->original, name, io_tool->cluster)) RET_NOMEMORY();
	if ((io_edget_token(name, 0)) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	if (allocstring(&io_edifgbl.nametbl[pos]->replace, name, io_tool->cluster)) RET_NOMEMORY();
	io_edifgbl.nametbl[pos]->textheight = 0;
	io_edifgbl.nametbl[pos]->justification = LOWERLEFT;
	io_edifgbl.nametbl[pos]->visible = 1;

	/* now bump the position */
	io_edifgbl.layer_ptr++;
	return(0);
}

INTBIG io_edpage(void)
{
	CHAR view[WORD+1], cname[WORD+1];
	NODEPROTO *proto;

	/* check for the name */
	(void)io_edcheck_name();

	(void)esnprintf(view, WORD+1, x_("p%d"), ++io_edifgbl.pageno);

	/* locate this in the list of cells */
	for (proto = io_edifgbl.library->firstnodeproto; proto != NONODEPROTO; proto = proto->nextnodeproto)
	{
		if (!namesame(proto->protoname, io_edifgbl.cell_name) &&
			!namesame(proto->cellview->sviewname, view)) break;
	}
	if (proto == NONODEPROTO)
	{
		/* allocate the cell */
		esnprintf(cname, WORD+1, x_("%s{%s}"), io_edifgbl.cell_name, view);
		proto = us_newnodeproto(cname, io_edifgbl.library);
		if (proto == NONODEPROTO) longjmp(io_edifgbl.env, LONGJMPNOMEM);
		proto->temp1 = 0;
	}
	else if (proto->temp1) io_edifgbl.ignoreblock = 1;

	io_edifgbl.current_cell = proto;
	return(0);
}

INTBIG io_edfigure(void)
{
	CHAR layer[WORD+1];
	INTBIG low, high, change, mid;
	INTBIG pos;

	/* get the layer name */
	/* check for figuregroup override */
	if (*io_edpos_token() == '(') return(0);
	if (io_edget_token(layer, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	/* now look for this layer in the list of layers */
	low = 0;
	high = io_edifgbl.layer_ptr - 1;
	while (low <= high)
	{
		mid = (low + high) / 2;
		change = namesame(layer, io_edifgbl.nametbl[mid]->original);
		if (change < 0) high = mid - 1; else
			if (change > 0) low = mid + 1; else
		{
			/* found the layer */
			io_edifgbl.figure_group = io_edifgbl.nametbl[mid]->node;
			io_edifgbl.arc_type = io_edifgbl.nametbl[mid]->arc;
			io_edifgbl.textheight = io_edifgbl.nametbl[mid]->textheight;
			io_edifgbl.justification = io_edifgbl.nametbl[mid]->justification;
			io_edifgbl.visible = io_edifgbl.nametbl[mid]->visible;

			/* allow new definitions */
			io_edifgbl.cur_nametbl = io_edifgbl.nametbl[mid];
			break;
		}
	}
	if (low > high)
	{
		pos = io_edifgbl.layer_ptr;
		io_edifgbl.nametbl[pos] = (NAMETABLE_PTR)emalloc(sizeof(NAMETABLE), io_tool->cluster);
		if (io_edifgbl.nametbl[pos] == NONAMETABLE ||
			allocstring(&io_edifgbl.nametbl[pos]->original, layer, io_tool->cluster))
				longjmp(io_edifgbl.env, LONGJMPNOMEM);
		io_edifgbl.nametbl[pos]->replace = io_edifgbl.nametbl[pos]->original;
		io_edifgbl.figure_group = io_edifgbl.nametbl[pos]->node = art_boxprim;
		io_edifgbl.arc_type = io_edifgbl.nametbl[pos]->arc = sch_wirearc;
		io_edifgbl.textheight = io_edifgbl.nametbl[pos]->textheight = 0;
		io_edifgbl.justification = io_edifgbl.nametbl[pos]->justification = LOWERLEFT;
		io_edifgbl.visible = io_edifgbl.nametbl[pos]->visible = 1;

		/* allow new definitions */
		io_edifgbl.cur_nametbl = io_edifgbl.nametbl[pos];

		/* now sort the list */
		io_edifgbl.layer_ptr++;
		esort(io_edifgbl.nametbl, io_edifgbl.layer_ptr, sizeof(NAMETABLE_PTR),
			io_edcompare_name);
	}
	return(0);
}

INTBIG io_edfigureGroupOverride(void)
{
	return(io_edgetlayer());
}

INTBIG io_edscale(void)
{
	/* get the scale */
	io_edifgbl.val1 = io_edgetnumber();
	io_edifgbl.val2 = io_edgetnumber();
	return(0);
}

INTBIG io_edendType(void)
{
	CHAR type[WORD+1];

	/* get the endtype */
	if (io_edget_token(type, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (!namesame(type, x_("EXTEND"))) io_edifgbl.extend_end = 1;
	return(0);
}

INTBIG io_edcornerType(void)
{
	CHAR type[WORD+1];

	/* get the endtype */
	if (io_edget_token(type, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (!namesame(type, x_("EXTEND"))) io_edifgbl.extend_corner = 1;
	return(0);
}

INTBIG io_edstringDisplay(void)
{
	INTBIG kptr;

	/* init the point lists */
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	io_edifgbl.visible = 1;
	io_edifgbl.justification = LOWERLEFT;
	io_edifgbl.textheight = 0;

	/* get the string, remove the quote */
	io_edget_delim('\"');
	if (io_edget_token(io_edifgbl.string, '\"') == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	/* check for RENAME */
	if (kstack[kstack_ptr-1] != KRENAME) return(0);
	(void)estrcpy(io_edifgbl.original, io_edifgbl.string);
	if (kstack[kstack_ptr - 2] == KARRAY) kptr = kstack_ptr - 3; else
		kptr = kstack_ptr - 2;
	switch (kstack[kptr])
	{
		case KCELL:
			(void)estrcpy(io_edifgbl.cell_name, io_edifgbl.original);
			break;
		case KPORT:
			(void)estrcpy(io_edifgbl.port_name, io_edifgbl.original);
			break;
		case KINSTANCE:
			(void)estrcpy(io_edifgbl.instance_name, io_edifgbl.original);
			break;
		case KNET:
			(void)estrcpy(io_edifgbl.net_name, io_edifgbl.original);
			break;
		case KNETBUNDLE:
			(void)estrcpy(io_edifgbl.bundle_name, io_edifgbl.original);
			break;
		case KPROPERTY:
			(void)estrcpy(io_edifgbl.property_name, io_edifgbl.original);
			break;
		default:
			break;
	}
	return(0);
}

INTBIG io_edtrue(void)
{
	/* check previous keyword */
	if (kstack_ptr > 1 && kstack[kstack_ptr-1] == KVISIBLE)
	{
		io_edifgbl.visible = 1;
		if (io_edifgbl.cur_nametbl != NONAMETABLE)
			io_edifgbl.cur_nametbl->visible = 1;
	}
	return(0);
}

INTBIG io_edfalse(void)
{
	/* check previous keyword */
	if (kstack_ptr > 1 && kstack[kstack_ptr-1] == KVISIBLE)
	{
		io_edifgbl.visible = 0;
		if (io_edifgbl.cur_nametbl != NONAMETABLE)
			io_edifgbl.cur_nametbl->visible = 0;
	}
	return(0);
}

INTBIG io_edgetlayer(void)
{
	CHAR layer[WORD+1];
	INTBIG low, high, change, mid;
	INTBIG pos;

	/* get the layer name */
	if (io_edget_token(layer, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	/* now look for this layer in the list of layers */
	low = 0;
	high = io_edifgbl.layer_ptr - 1;
	while (low <= high)
	{
		mid = (low + high) / 2;
		change = namesame(layer, io_edifgbl.nametbl[mid]->original);
		if (change < 0) high = mid - 1; else
			if (change > 0) low = mid + 1; else
		{
			/* found the layer */
			io_edifgbl.figure_group = io_edifgbl.nametbl[mid]->node;
			io_edifgbl.arc_type = io_edifgbl.nametbl[mid]->arc;
			io_edifgbl.textheight = io_edifgbl.nametbl[mid]->textheight;
			io_edifgbl.justification = io_edifgbl.nametbl[mid]->justification;
			io_edifgbl.visible = io_edifgbl.nametbl[mid]->visible;
			break;
		}
	}
	if (low > high)
	{
#ifdef LAYERWARNING
		ttyputerr(_("warning, line #%d: unknown layer <%s>"), io_edifgbl.lineno, layer);
		io_edifgbl.warnings++;
#endif

		/* insert and resort the list */
		pos = io_edifgbl.layer_ptr;

		io_edifgbl.nametbl[pos] = (NAMETABLE_PTR)emalloc(sizeof(NAMETABLE), io_tool->cluster);
		if (io_edifgbl.nametbl[pos] == NONAMETABLE ||
			allocstring(&io_edifgbl.nametbl[pos]->original, layer, io_tool->cluster))
				longjmp(io_edifgbl.env, LONGJMPNOMEM);

		io_edifgbl.nametbl[pos]->replace = io_edifgbl.nametbl[pos]->original;
		io_edifgbl.figure_group = io_edifgbl.nametbl[pos]->node = art_boxprim;
		io_edifgbl.arc_type = io_edifgbl.nametbl[pos]->arc = sch_wirearc;
		io_edifgbl.textheight = io_edifgbl.nametbl[pos]->textheight = 0;
		io_edifgbl.justification = io_edifgbl.nametbl[pos]->justification = LOWERLEFT;
		io_edifgbl.visible = io_edifgbl.nametbl[pos]->visible = 1;

		/* now sort the list */
		io_edifgbl.layer_ptr++;
		esort(io_edifgbl.nametbl, io_edifgbl.layer_ptr, sizeof(NAMETABLE_PTR),
			io_edcompare_name);
	}
	return(0);
}

/* geometry routines */
INTBIG io_edpointList(void)
{
	return(0);
}

INTBIG io_edpoint(void)
{
	return(0);
}

INTBIG io_edjustify(void)
{
	CHAR val[WORD+1];

	/* get the textheight value of the point */
	if (io_edget_token(val, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	if (namesame(val, x_("UPPERLEFT")))
		io_edifgbl.justification = UPPERLEFT;
	else if (namesame(val, x_("UPPERCENTER")))
		io_edifgbl.justification = UPPERCENTER;
	else if (namesame(val, x_("UPPERRIGHT")))
		io_edifgbl.justification = UPPERRIGHT;
	else if (namesame(val, x_("CENTERLEFT")))
		io_edifgbl.justification = CENTERLEFT;
	else if (namesame(val, x_("CENTERCENTER")))
		io_edifgbl.justification = CENTERCENTER;
	else if (namesame(val, x_("CENTERRIGHT")))
		io_edifgbl.justification = CENTERRIGHT;
	else if (namesame(val, x_("LOWERLEFT")))
		io_edifgbl.justification = LOWERLEFT;
	else if (namesame(val, x_("LOWERCENTER")))
		io_edifgbl.justification = LOWERCENTER;
	else if (namesame(val, x_("LOWERRIGHT")))
		io_edifgbl.justification = LOWERRIGHT;
	else
	{
		ttyputerr(_("warning, line #%d: unknown keyword <%s>"), io_edifgbl.lineno, val);
		io_edifgbl.warnings++;
		return(0);
	}

	if (io_edifgbl.cur_nametbl != NONAMETABLE)
		io_edifgbl.cur_nametbl->justification = io_edifgbl.justification;
	return(0);
}

INTBIG io_edtextHeight(void)
{
	CHAR val[WORD+1];

	/* get the textheight value of the point */
	if (io_edget_token(val, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	io_edifgbl.textheight = (INTBIG)(eatol(val) * io_edifgbl.scale);

	if (io_edifgbl.cur_nametbl != NONAMETABLE)
		io_edifgbl.cur_nametbl->textheight = io_edifgbl.textheight;

	return(0);
}

INTBIG io_edpt(void)
{
	CHAR xstr[WORD+1], ystr[WORD+1];
	EPT_PTR point;
	INTBIG x, y, s;

	/* get the x and y values of the point */
	if (io_edget_token(xstr, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	if (io_edget_token(ystr, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (kstack_ptr > 1 && kstack[kstack_ptr-1] == KDELTA)
	{
		x = (INTBIG)(eatol(xstr) * io_edifgbl.scale);
		y = (INTBIG)(eatol(ystr) * io_edifgbl.scale);

		/* transform the points per orientation */
		switch (io_edifgbl.orientation)
		{
			case OR0:                               break;
			case OR90:   s = x;   x = -y;  y = s;   break;
			case OR180:  x = -x;  y = -y;           break;
			case OR270:  s = x;   x = y;   y = -s;  break;
			case OMY:    x = -x;                    break;
			case OMX:    y = -y;                    break;
			case OMYR90: s = y;   y = -x;  x = -s;  break;
			case OMXR90: s = x;   x = y;   y = s;   break;
			default:                                break;
		}

		/* set the array deltas */
		if (io_edifgbl.deltapts == 0)
		{
			io_edifgbl.deltaxX = x;
			io_edifgbl.deltaxY = y;
		} else
		{
			io_edifgbl.deltayX = x;
			io_edifgbl.deltayY = y;
		}
		io_edifgbl.deltapts++;
	} else
	{
		/* allocate a point to read */
		point = (EPT_PTR) emalloc(sizeof (EPT), io_tool->cluster);
		if (point == NOEPT) RET_NOMEMORY();

		/* and set the values */
		point->x = eatol(xstr);
		if (point->x > 0) point->x = (INTBIG)(point->x * io_edifgbl.scale + 0.5); else
			point->x = (INTBIG)(point->x * io_edifgbl.scale - 0.5);
		point->y = eatol(ystr);
		if (point->y > 0) point->y = (INTBIG)(point->y * io_edifgbl.scale + 0.5); else
			point->y = (INTBIG)(point->y * io_edifgbl.scale - 0.5);
		point->nextpt = NOEPT;

		/* add it to the list of points */
		if (io_edifgbl.points) io_edifgbl.lastpt->nextpt = point; else
			io_edifgbl.points = point;
		io_edifgbl.lastpt = point;
		io_edifgbl.pt_count++;
	}
	return(0);
}

INTBIG io_edorientation(void)
{
	CHAR orient[WORD+1];

	/* get the orientation keyword */
	if (io_edget_token(orient, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (!namesame(orient, x_("R0"))) io_edifgbl.orientation = OR0;
	else if (!namesame(orient, x_("R90"))) io_edifgbl.orientation = OR90;
	else if (!namesame(orient, x_("R180"))) io_edifgbl.orientation = OR180;
	else if (!namesame(orient, x_("R270"))) io_edifgbl.orientation = OR270;
	else if (!namesame(orient, x_("MY"))) io_edifgbl.orientation = OMY;
	else if (!namesame(orient, x_("MX"))) io_edifgbl.orientation = OMX;
	else if (!namesame(orient, x_("MYR90"))) io_edifgbl.orientation = OMYR90;
	else if (!namesame(orient, x_("MXR90"))) io_edifgbl.orientation = OMXR90;
	else
	{
		ttyputerr(_("warning, line #%d: unknown orientation value <%s>"), io_edifgbl.lineno, orient);
		io_edifgbl.warnings++;
	}
	return(0);
}

INTBIG io_edpathWidth(void)
{
	CHAR width[WORD+1];

	/* get the width string */
	if (io_edget_token(width, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	io_edifgbl.path_width = eatol(width);

	return(0);
}

INTBIG io_edrectangle(void)
{
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	return(0);
}

INTBIG io_edcircle(void)
{
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	return(0);
}

INTBIG io_edpath(void)
{
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	io_edifgbl.path_width = 0;
	return(0);
}

INTBIG io_edpolygon(void)
{
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	return(0);
}

INTBIG io_edopenShape(void)
{
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	return(0);
}

INTBIG io_edfreeptlist(void)
{
	while (io_edifgbl.points != NOEPT)
	{
		io_edifgbl.lastpt = io_edifgbl.points->nextpt;
		efree((CHAR *)io_edifgbl.points);
		io_edifgbl.points = io_edifgbl.lastpt;
		io_edifgbl.pt_count--;
	}
	if (io_edifgbl.pt_count != 0)
		longjmp(io_edifgbl.env, LONGJMPPTMIS);
	return(0);
}

INTBIG io_edfreesavedptlist(void)
{
	EPT_PTR lastpt;

	while (io_edifgbl.save_text.points != NOEPT)
	{
		lastpt = io_edifgbl.save_text.points->nextpt;
		efree((CHAR *)io_edifgbl.save_text.points);
		io_edifgbl.save_text.points = lastpt;
		io_edifgbl.save_text.pt_count--;
	}
	if (io_edifgbl.save_text.pt_count != 0)
		longjmp(io_edifgbl.env, LONGJMPPTMIS);
	return(0);
}

INTBIG io_edprogram(void)
{
	CHAR program[WORD+1];

	if (io_edget_token(program, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	if (!namesamen(&program[1], x_("VIEWlogic"), 9))
	{
		io_edifgbl.vendor = EVVIEWLOGIC;
	} else if (!namesamen(&program[1], x_("edifout"), 7))
	{
		io_edifgbl.vendor = EVCADENCE;
	}
	return(0);
}

INTBIG io_edunit(void)
{
	CHAR type[WORD+1];
	REGISTER VARIABLE *var;
	float edifscale;

	if (io_edget_token(type, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	if (kstack[kstack_ptr-1] == KSCALE && !namesame(type, x_("DISTANCE")))
	{
		/* get the scale */
		io_edifgbl.dbunit = io_edifgbl.val1;
		io_edifgbl.meters = io_edifgbl.val2;

		/* just make the scale be so that the specified number of database units becomes 1 lambda */
		io_edifgbl.scale = el_curlib->lambda[sch_tech->techindex];

		var = getval((INTBIG)io_tool, VTOOL, VFLOAT, x_("IO_edif_input_scale"));
		if (var == NOVARIABLE) edifscale = 1.0; else
			edifscale = castfloat(var->addr);
		io_edifgbl.scale *= edifscale;
	}
	return(0);
}

INTBIG io_edlibrary(void)
{
	CHAR *name, nbuf[WORD+1];

	/* get the name of the library */
	name = nbuf;
	if (io_edget_token(name, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
#ifdef EDIFLIBNAME
	/* retrieve the library */
	io_edifgbl.library = getlibrary(name);
	if (io_edifgbl.library == NOLIBRARY)
	{
		/* create the library */
		io_edifgbl.library = newlibrary(name, name);
		if (io_edifgbl.library == NOLIBRARY)
			longjmp(io_edifgbl.env, LONGJMPNOLIB);
		ttyputmsg(_("Reading library %s", name);
	} else
	{
		/* flag existing cells */
		for (np = io_edifgbl.library->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 1;
		ttyputmsg(_("Updating library %s", name);
	}
#endif
	return(0);
}

/* cell definition routines */
INTBIG io_eddesign(void)
{
	CHAR name[WORD+1];

	/* get the name of the cell */
	if (io_edget_token(name, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	return(0);
}

/* note: cells are considered primitive unless they contain instance
   information, i.e. (contents  (instance ... , this cell will become
   a COMPLEX cell.
 */
INTBIG io_edcell(void)
{
	io_edifgbl.active_view = VNULL;
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	io_edifgbl.pageno = 0;
	io_edifgbl.sh_xpos = io_edifgbl.sh_ypos = -1;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.cell_reference, io_edifgbl.name);
		(void)estrcpy(io_edifgbl.cell_name, io_edifgbl.name);
	}
	return(0);
}

/* view: Indicates a new view of the cell
 * 	 just clears the active view flag
 */
INTBIG io_edview(void)
{
	io_edifgbl.active_view = VNULL;
	return(0);
}

/* viewType:  Indicates the view style for this cell, ie
 *	BEHAVIOR, DOCUMENT, GRAPHIC, LOGICMODEL, MASKLAYOUT, NETLIST,
 *  	PCBLAYOUT, SCHEMATIC, STRANGER, SYMBOLIC
 * we are only concerned about the viewType NETLIST.
 */
INTBIG io_edviewType(void)
{
	CHAR name[WORD+1], cname[WORD+1], view[WORD+1];
	NODEPROTO *proto;

	/* get the viewType */
	if (io_edget_token(name, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (!namesame(name, x_("BEHAVIOR"))) io_edifgbl.active_view = VBEHAVIOR;
	else if (!namesame(name, x_("DOCUMENT"))) io_edifgbl.active_view = VDOCUMENT;
	else if (!namesame(name, x_("GRAPHIC")))
	{
		io_edifgbl.active_view = VGRAPHIC;
		estrcpy(view, x_(""));
	}
	else if (!namesame(name, x_("LOGICMODEL"))) io_edifgbl.active_view = VLOGICMODEL;
	else if (!namesame(name, x_("MASKLAYOUT")))
	{
		io_edifgbl.active_view = VMASKLAYOUT;
		estrcpy(view, x_("lay"));
	}
	else if (!namesame(name, x_("NETLIST")))
	{
		io_edifgbl.active_view = VNETLIST;
		estrcpy(view, x_("sch"));
	}
	else if (!namesame(name, x_("PCBLAYOUT"))) io_edifgbl.active_view = VPCBLAYOUT;
	else if (!namesame(name, x_("SCHEMATIC")))
	{
		io_edifgbl.active_view = VSCHEMATIC;
		estrcpy(view, x_("sch"));
	}
	else if (!namesame(name, x_("STRANGER"))) io_edifgbl.active_view = VSTRANGER;
	else if (!namesame(name, x_("SYMBOLIC"))) io_edifgbl.active_view = VSYMBOLIC;

	/* immediately allocate MASKLAYOUT and VGRAPHIC viewtypes */
	if (io_edifgbl.active_view == VMASKLAYOUT || io_edifgbl.active_view == VGRAPHIC ||
		io_edifgbl.active_view == VNETLIST || io_edifgbl.active_view == VSCHEMATIC)
	{
		/* locate this in the list of cells */
		for (proto = io_edifgbl.library->firstnodeproto; proto != NONODEPROTO;
			proto = proto->nextnodeproto)
		{
			if (!namesame(proto->protoname, io_edifgbl.cell_name) &&
				!namesame(proto->cellview->sviewname, view)) break;
		}
		if (proto == NONODEPROTO)
		{
			/* allocate the cell */
			esnprintf(cname, WORD+1, x_("%s{%s}"), io_edifgbl.cell_name, view);
			proto = us_newnodeproto(cname, io_edifgbl.library);
			if (proto == NONODEPROTO) longjmp(io_edifgbl.env, LONGJMPNOMEM);
			proto->temp1 = 0;
		}
		else if (proto->temp1) io_edifgbl.ignoreblock = 1;

		io_edifgbl.current_cell = proto;
	}
	else io_edifgbl.current_cell = NONODEPROTO;

	/* add the name to the celltbl */
	if (io_edifgbl.celltbl_cnt == io_edifgbl.celltbl_sze)
	{
		io_edifgbl.celltbl_sze += 100;
		if (io_edifgbl.celltbl_cnt == 0)
		{
			io_edifgbl.celltbl = (NAMETABLE_PTR *)emalloc(sizeof (NAMETABLE_PTR) *
				io_edifgbl.celltbl_sze, io_tool->cluster);
			if (io_edifgbl.celltbl == (NAMETABLE_PTR *)0) RET_NOMEMORY();
		} else
		{
			NAMETABLE_PTR *newtbl;
			long i;

			newtbl = (NAMETABLE_PTR *)emalloc(sizeof (NAMETABLE_PTR) * io_edifgbl.celltbl_sze,
				io_tool->cluster);
			if (newtbl == (NAMETABLE_PTR *)0) RET_NOMEMORY();
			for(i=0; i<io_edifgbl.celltbl_cnt; i++) newtbl[i] = io_edifgbl.celltbl[i];
			io_edifgbl.celltbl = newtbl;
		}
	}
	io_edifgbl.celltbl[io_edifgbl.celltbl_cnt] = (NAMETABLE_PTR)emalloc(sizeof (NAMETABLE),
		io_tool->cluster);
	if (io_edifgbl.celltbl[io_edifgbl.celltbl_cnt] == NONAMETABLE) RET_NOMEMORY();
	if (allocstring(&io_edifgbl.celltbl[io_edifgbl.celltbl_cnt]->original, io_edifgbl.cell_reference,
		io_tool->cluster)) RET_NOMEMORY();
	if (allocstring(&io_edifgbl.celltbl[io_edifgbl.celltbl_cnt]->replace, io_edifgbl.cell_name,
		io_tool->cluster)) RET_NOMEMORY();

	/* now sort by name */
	esort(io_edifgbl.celltbl, ++(io_edifgbl.celltbl_cnt), sizeof (NAMETABLE_PTR),
		io_edcompare_name);
	return(0);
}

/* contents:  Indicates additional information about higher level blocks
 * initially verifies the appropriate state, ie
 * 	(view XXX (viewType Netlist)
 *	  (contents ...
 */
INTBIG io_edcontents(void)
{
	return(0);
}

/* port: Define connection point for the cell, must be in INTERFACE */
INTBIG io_edport(void)
{
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	io_edifgbl.direction = INPUTE;
	io_edifgbl.port_reference[0] = 0;
	io_edifgbl.isarray = 0;
	io_edifgbl.arrayx = io_edifgbl.arrayy = 1;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.port_reference, io_edifgbl.name);
		(void)estrcpy(io_edifgbl.port_name, io_edifgbl.name);
	}
	return(0);
}

/* direction:  Set the direction of a port */
INTBIG io_eddirection(void)
{
	CHAR name[WORD+1];

	/* get the direction */
	if (io_edget_token(name, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (!namesame(name, x_("INOUT"))) io_edifgbl.direction = INOUT; else
		if (!namesame(name, x_("INPUT"))) io_edifgbl.direction = INPUTE; else
			if (!namesame(name, x_("OUTPUT"))) io_edifgbl.direction = OUTPUTE;
	return(0);
}

/* instance definition routines */
/* instance:  Indicates a new instance within the cell
 */
INTBIG io_edinstance(void)
{
	/* set the current geometry type */
	io_edfreeptlist();
	io_edifgbl.proto = NONODEPROTO;
	io_edifgbl.geometry = GINSTANCE;
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	io_edifgbl.isarray = 0;
	io_edifgbl.arrayx = io_edifgbl.arrayy = 1;
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	io_edifgbl.instance_reference[0] = 0;
	io_edifgbl.current_node = NONODEINST;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.instance_reference, io_edifgbl.name);
		(void)estrcpy(io_edifgbl.instance_name, io_edifgbl.name);
	}
	return(0);
}

INTBIG io_edarray(void)
{
	/* note array is a special process integer value function */
	io_edifgbl.isarray = 1;
	io_edifgbl.arrayx = io_edifgbl.arrayy = 0;
	io_edifgbl.deltaxX = io_edifgbl.deltaxY = 0;
	io_edifgbl.deltayX = io_edifgbl.deltayY = 0;
	io_edifgbl.deltapts = 0;
	if (io_edcheck_name())
	{
		switch (kstack[kstack_ptr-1])
		{
			case KCELL:
				(void)estrcpy(io_edifgbl.cell_reference, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.cell_name, io_edifgbl.name);
				break;
			case KPORT:
				(void)estrcpy(io_edifgbl.port_name, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.port_reference, io_edifgbl.name);
				break;
			case KINSTANCE:
				(void)estrcpy(io_edifgbl.instance_name, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.instance_reference, io_edifgbl.name);
				break;
			case KNET:
				(void)estrcpy(io_edifgbl.net_reference, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.net_name, io_edifgbl.name);
				break;
			case KPROPERTY:
				(void)estrcpy(io_edifgbl.property_reference, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.property_name, io_edifgbl.name);
				break;
			default:
				break;
		}
	}
	return(0);
}

INTBIG io_eddelta(void)
{
	io_edifgbl.deltaxX = io_edifgbl.deltaxY = 0;
	io_edifgbl.deltayX = io_edifgbl.deltayY = 0;
	io_edifgbl.deltapts = 0;
	return(0);
}

/* cellRef:  determines the cell type of an instance
 */
INTBIG io_edcellRef(void)
{
	CHAR name[WORD+1], view[WORD+1];
	NODEPROTO *proto;
	INTBIG low, high, change, mid;

	/* get the name of the cell */
	if (io_edget_token(name, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	switch (io_edifgbl.active_view)
	{
		case VMASKLAYOUT:
			estrcpy(view, x_("lay"));
			break;
		default:
			if (kstack[kstack_ptr - 1] == KDESIGN) estrcpy(view, x_("p1")); else
				estrcpy(view, x_("ic"));
	}
	if (io_edifgbl.vendor == EVVIEWLOGIC && !namesame(name, x_("SPLITTER")))
	{
		io_edifgbl.proto = NONODEPROTO;
		return(0);
	}

	/* look for this cell name in the cell list */
	low = 0;
	high = io_edifgbl.celltbl_cnt - 1;
	while (low <= high)
	{
		mid = (low + high) / 2;
		change = namesame(name, io_edifgbl.celltbl[mid]->original);
		if (change < 0) high = mid - 1; else
			if (change > 0) low = mid + 1; else
		{
			/* found the name */
			(void)estrcpy(name, io_edifgbl.celltbl[mid]->replace);
			break;
		}
	}
	if (low > high)
	{
		ttyputerr(_("could not find cellRef <%s>"), name);
	}

	/* now look for this cell in the list of cells, if not found create it */
	/* locate this in the list of cells */
	for (proto = io_edifgbl.library->firstnodeproto; proto != NONODEPROTO; proto = proto->nextnodeproto)
	{
		if (!namesame(proto->protoname, name) &&
			!namesame(proto->cellview->sviewname, view)) break;
	}
	if (proto == NONODEPROTO)
	{
		/* allocate the cell */
		if (view[0] != 0)
		{
			estrcat(name, x_("{"));
			estrcat(name, view);
			estrcat(name, x_("}"));
		}
		proto = us_newnodeproto(name, io_edifgbl.library);
		if (proto == NONODEPROTO) longjmp(io_edifgbl.env, LONGJMPNOMEM);
		proto->temp1 = 0;
	}

	/* set the parent */
	io_edifgbl.proto = proto;
	return(0);
}

INTBIG io_edinteger(void)
{
	CHAR value[WORD+1];

	if (io_edget_token(value, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);

	if (io_edifgbl.ptype == PSTRING) efree(io_edifgbl.pval.string);
	io_edifgbl.pval.integer = eatoi(value);
	io_edifgbl.ptype = PINTEGER;
	return(0);
}

INTBIG io_ednumber(void)
{
	if (io_edifgbl.ptype == PSTRING) efree(io_edifgbl.pval.string);
	io_edifgbl.pval.number = (float)io_edgetnumber();
	io_edifgbl.ptype = PNUMBER;
	return(0);
}

INTBIG io_edstring(void)
{
	CHAR *token, value[WORD+1];

	if (*(token = io_edpos_token()) != '(' && *token != ')')
	{
		if (io_edget_token(value, 0) == NULL)
			longjmp(io_edifgbl.env, LONGJMPEOF);
		if (io_edifgbl.ptype == PSTRING) efree(io_edifgbl.pval.string);

		value[estrlen(value) - 1] = 0;
		io_edifgbl.pval.string = (CHAR *)emalloc(estrlen(value) * SIZEOFCHAR, io_tool->cluster);
		if (io_edifgbl.pval.string == NULL)
			longjmp(io_edifgbl.env, LONGJMPNOMEM);

		(void)estrcpy(io_edifgbl.pval.string, &value[1]);
		io_edifgbl.ptype = PSTRING;
	}
	return(0);
}

INTBIG io_edproperty(void)
{
	io_edifgbl.property_reference[0] = 0;
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	if (io_edifgbl.ptype == PSTRING) efree(io_edifgbl.pval.string);
	io_edifgbl.ptype = PUNKNOWN;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.property_reference, io_edifgbl.name);
		(void)estrcpy(io_edifgbl.property_name, io_edifgbl.name);
	}
	return(0);
}

/* net:  Define a net name followed by edifgbl and local pin list
 *		(net NAME
 *			(joined
 *			(portRef NAME (instanceRef NAME))
 *			(portRef NAME)))
 */
INTBIG io_ednet(void)
{
	io_edifgbl.net_reference[0] = 0;
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	io_edifgbl.current_arc = NOARCINST;
	io_edifgbl.current_node = NONODEINST;
	io_edifgbl.current_port = NOPORTPROTO;
	io_edfreenetports();
	io_edifgbl.isarray = 0;
	io_edifgbl.arrayx = io_edifgbl.arrayy = 1;
	if (kstack[kstack_ptr-2] != KNETBUNDLE) io_edifgbl.geometry = GNET;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.net_reference, io_edifgbl.name);
		(void)estrcpy(io_edifgbl.net_name, io_edifgbl.name);
	}
	return(0);
}

INTBIG io_ednetBundle(void)
{
	io_edifgbl.geometry = GBUS;
	io_edifgbl.bundle_reference[0] = 0;
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	io_edifgbl.isarray = 0;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.bundle_reference, io_edifgbl.name);
		(void)estrcpy(io_edifgbl.bundle_name, io_edifgbl.name);
	}
	return(0);
}

INTBIG io_edjoined(void)
{
	return(0);
}

/* portRef:  specifies a pin on a net */
INTBIG io_edportRef(void)
{
	io_edifgbl.port_reference[0] = 0;
	io_edifgbl.instance_reference[0] = 0;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.port_reference, io_edifgbl.name);
	}

	/* allocate a netport */
	if (io_edallocnetport())
		longjmp(io_edifgbl.env, LONGJMPNOMEM);
	return(0);
}

/* instanceRef:  identifies the name of the instance attached to the net */
INTBIG io_edinstanceRef(void)
{
	io_edifgbl.instance_reference[0] = 0;
	if (io_edcheck_name())
	{
		(void)estrcpy(io_edifgbl.instance_reference, io_edifgbl.name);
	}
	return(0);
}

/* array member function, determines a specific element of an array */
INTBIG io_edmember(void)
{
	io_edifgbl.memberx = -1; /* no member */
	io_edifgbl.membery = -1; /* no member */
	if (io_edcheck_name())
	{
		switch (kstack[kstack_ptr-1])
		{
			case KPORTREF:
				(void)estrcpy(io_edifgbl.port_reference, io_edifgbl.name);
				break;
			case KINSTANCEREF:
				(void)estrcpy(io_edifgbl.instance_reference, io_edifgbl.name);
				break;
			default:
				break;
		}
	}
	return(0);
}

INTBIG io_edname(void)
{
	INTBIG kptr;

	if (kstack[kstack_ptr-1] == KARRAY || kstack[kstack_ptr-1] == KMEMBER) kptr = kstack_ptr-2; else
		kptr = kstack_ptr - 1;
	if (io_edcheck_name())
	{
		switch (kstack[kptr])
		{
			case KCELL:
				(void)estrcpy(io_edifgbl.cell_reference, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.cell_name, io_edifgbl.name);
				break;
			case KPORTIMPLEMENTATION:		/* added by smr */
			case KPORT:
				(void)estrcpy(io_edifgbl.port_name, io_edifgbl.name);
				/* FALLTHROUGH */ 
			case KPORTREF:
				(void)estrcpy(io_edifgbl.port_reference, io_edifgbl.name);
				break;
			case KINSTANCE:
				(void)estrcpy(io_edifgbl.instance_name, io_edifgbl.name);
				/* FALLTHROUGH */ 
			case KINSTANCEREF:
				(void)estrcpy(io_edifgbl.instance_reference, io_edifgbl.name);
				break;
			case KNET:
				(void)estrcpy(io_edifgbl.net_reference, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.net_name, io_edifgbl.name);
				break;
			case KPROPERTY:
				(void)estrcpy(io_edifgbl.property_reference, io_edifgbl.name);
				(void)estrcpy(io_edifgbl.property_name, io_edifgbl.name);
				break;
			default:
				break;
		}

		/* init the point lists */
		io_edfreeptlist();
		io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
		io_edifgbl.orientation = OR0;
		io_edifgbl.visible = 1;
		io_edifgbl.justification = LOWERLEFT;
		io_edifgbl.textheight = 0;
		(void)estrcpy(io_edifgbl.string, io_edifgbl.name);
	}
	return(0);
}

INTBIG io_edrename(void)
{
	CHAR name[WORD+1];
	INTBIG kptr;

	/* get the name of the object */
	if (io_edget_token(name, 0) == NULL)
		longjmp(io_edifgbl.env, LONGJMPEOF);
	(void)estrcpy(io_edifgbl.name, (name[0] == '&') ? &name[1] : name);

	/* and the original name */
	if (*io_edpos_token() == '(')
	{
		/* must be stringDisplay, copy name to original */
		(void)estrcpy(io_edifgbl.original, io_edifgbl.name);
	} else
	{
		if  (io_edget_token(name, 0) == NULL)
			longjmp(io_edifgbl.env, LONGJMPEOF);

		/* copy name without quotes */
		(void)estrcpy(io_edifgbl.original, &name[1]);
		io_edifgbl.original[estrlen(name)-2] = 0;
	}
	if (kstack[kstack_ptr - 1]  == KNAME) kptr = kstack_ptr - 1; else
		kptr = kstack_ptr;
	if (kstack[kptr - 1] == KARRAY) kptr = kptr - 2; else
		kptr = kptr -1;
	switch (kstack[kptr])
	{
		case KCELL:
			(void)estrcpy(io_edifgbl.cell_reference, io_edifgbl.name);
			(void)estrcpy(io_edifgbl.cell_name, io_edifgbl.original);
			break;
		case KPORT:
			(void)estrcpy(io_edifgbl.port_reference, io_edifgbl.name);
			(void)estrcpy(io_edifgbl.port_name, io_edifgbl.original);
			break;
		case KINSTANCE:
			(void)estrcpy(io_edifgbl.instance_reference, io_edifgbl.name);
			(void)estrcpy(io_edifgbl.instance_name, io_edifgbl.original);
			break;
		case KNETBUNDLE:
			(void)estrcpy(io_edifgbl.bundle_reference, io_edifgbl.name);
			(void)estrcpy(io_edifgbl.bundle_name, io_edifgbl.original);
			break;
		case KNET:
			(void)estrcpy(io_edifgbl.net_reference, io_edifgbl.name);
			(void)estrcpy(io_edifgbl.net_name, io_edifgbl.original);
			break;
		case KPROPERTY:
			(void)estrcpy(io_edifgbl.property_reference, io_edifgbl.name);
			(void)estrcpy(io_edifgbl.property_name, io_edifgbl.original);
			break;
		default:
			break;
	}
	return(0);
}

/* symbol routines and schematic routines */
/* boundingBox - describes the bounding box of a schematic symbol */
INTBIG io_edboundingBox(void)
{
	io_edifgbl.figure_group = art_openeddottedpolygonprim;
	return(0);
}

/* symbol - describes a schematic symbol */
INTBIG io_edsymbol(void)
{
	CHAR cname[WORD+1];
	NODEPROTO *proto;
	INTBIG center[2];

	io_edifgbl.active_view = VSYMBOL;

	/* locate this in the list of cells */
	for (proto = io_edifgbl.library->firstnodeproto; proto != NONODEPROTO; proto = proto->nextnodeproto)
	{
		if (!namesame(proto->protoname, io_edifgbl.cell_name) &&
			!namesame(proto->cellview->sviewname, x_("ic"))) break;
	}
	if (proto == NONODEPROTO)
	{
		/* allocate the cell */
		esnprintf(cname, WORD+1, x_("%s{ic}"), io_edifgbl.cell_name);
		proto = us_newnodeproto(cname, io_edifgbl.library);
		if (proto == NONODEPROTO) longjmp(io_edifgbl.env, LONGJMPNOMEM);
		proto->userbits |= WANTNEXPAND;
		proto->temp1 = 0;
		center[0] = center[1] = 0;
		(void)setvalkey((INTBIG)proto, VNODEPROTO, el_prototype_center_key,
			(INTBIG)center, VINTEGER|VISARRAY|(2<<VLENGTHSH));

	}
	else if (proto->temp1) io_edifgbl.ignoreblock = 1;

	io_edifgbl.current_cell = proto;
	io_edifgbl.figure_group = NONODEPROTO;
	return(0);
}

INTBIG io_edportImplementation(void)
{
	/* set the current geometry type */
	io_edfreeptlist();
	io_edifgbl.proto = sch_wirepinprim;
	io_edifgbl.geometry = GPIN;
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	io_edifgbl.isarray = 0;
	io_edifgbl.arrayx = io_edifgbl.arrayy = 1;
	io_edifgbl.name[0] = io_edifgbl.original[0] = 0;
	(void)io_edcheck_name();
	return(0);
}

INTBIG io_edconnectLocation(void)
{
	return(0);
}

INTBIG io_edinterface(void)
{
	NODEPROTO *np;
	CHAR nodename[WORD+1];

	/* create schematic page 1 to represent all I/O for this schematic */
	/* locate this in the list of cells */
	for (np = io_edifgbl.library->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (!namesame(np->protoname, io_edifgbl.cell_name) &&
			!namesame(np->cellview->sviewname, x_("sch"))) break;
	}
	if (np == NONODEPROTO)
	{
		/* allocate the cell */
		esnprintf(nodename, WORD+1, x_("%s{sch}"), io_edifgbl.cell_name);
		io_edifgbl.current_cell = us_newnodeproto(nodename, io_edifgbl.library);
		if (io_edifgbl.current_cell == NONODEPROTO)
			longjmp(io_edifgbl.env, LONGJMPNOMEM);
		io_edifgbl.current_cell->temp1 = 0;
	}
	else io_edifgbl.current_cell = np;

	/* now set the current position in the schematic page */
	io_edifgbl.ipos = io_edifgbl.bpos = io_edifgbl.opos = 0;
	return(0);
}

INTBIG io_eddot(void)
{
	io_edfreeptlist();
	io_edifgbl.points = io_edifgbl.lastpt = NOEPT;
	io_edifgbl.orientation = OR0;
	return(0);
}

double io_edgetnumber(void)
{
	CHAR value[WORD+1];
	INTBIG matissa, exponent;

	if (io_edget_token(value, 0) == NULL)
	{
		longjmp(io_edifgbl.env, LONGJMPNOINT);
	}
	if (value[0] == '(')
	{
		/* must be in e notation */
		if (io_edget_token(value, 0) == NULL ||
			namesame(value, x_("e")))
			longjmp(io_edifgbl.env, LONGJMPILLNUM);

		/* now the matissa */
		if (io_edget_token(value, 0) == NULL)
			longjmp(io_edifgbl.env, LONGJMPNOMANT);
		matissa = eatoi(value);

		/* now the exponent */
		if (io_edget_token(value, 0) == NULL)
			longjmp(io_edifgbl.env, LONGJMPNOEXP);
		exponent = eatoi(value);
		io_edget_delim(')');
		return((double)matissa * pow(10.0, (double)exponent));
	}
	return((double) eatoi(value));
}

INTBIG io_edcheck_name(void)
{
	CHAR *pp;
	CHAR name[WORD+1];

	if (*(pp = io_edpos_token()) != '(' && *pp != ')')
	{
		if (io_edget_token(name, 0) == NULL)
			longjmp(io_edifgbl.env, LONGJMPILLNAME);
		(void)estrcpy(io_edifgbl.name, (name[0] == '&') ? &name[1] : name);
		return(1);
	}
	return(0);
}

static void io_edmake_electric_name(CHAR *ref, CHAR *name, int x, int y)
{
}

/* general utilities ... */
static void io_edcheck_busnames(ARCINST *ai, CHAR *base)
{
	PORTARCINST *pai;
	NODEINST *ni;
	INTBIG i, l;
	VARIABLE *var;
	CHAR *name, newname[WORD+1];

	if (ai->proto != sch_busarc)
	{
		/* verify the name */
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE && ((name = (CHAR *)var->addr) != NULL))
		{
			l = estrlen(base);
			if (!namesamen(name, base, l) && isdigit(name[l]))
			{
				(void)esnprintf(newname, WORD+1, x_("%s[%s]"), base, &name[l]);
				var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)newname, VSTRING);
				if (var != NOVARIABLE)
					defaulttextsize(4, var->textdescript);
			}
		}
	}
	ai->temp1 = 1;
	for (i = 0; i < 2; i++)
	{
		ni = ai->end[i].nodeinst;
		if (((ni->proto->userbits & NFUNCTION) >> NFUNCTIONSH) == NPPIN)
		{
			/* scan through this nodes portarcinst's */
			for (pai = ni->firstportarcinst; pai != NOPORTARCINST; pai = pai->nextportarcinst)
			{
				if (pai->conarcinst->temp1 == 0)
					io_edcheck_busnames(pai->conarcinst, base);
			}
		}
	}
}

/* pop the keyword state stack, called when ')' is encountered. Note this function
   needs to broken into a simple function call structure. */
#define MAXBUSPINS 256
INTBIG io_edpop_stack(void)
{
	INTBIG eindex, key, lambda;
	INTBIG lx, ly, hx, hy, cx, cy, cnt, Ix, Iy, gx, gy, xoff, yoff, dist;
	INTBIG *trace, *pt, pts[26], fcnt, tcnt, psx, psy;
	INTBIG width, height, x[3], y[3], radius, trans;
	INTBIG count, user_max, i, j, dup, fbus, tbus, instcount, instptx, instpty;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	CHAR nodename[WORD+1], portname[WORD+1], basename[WORD+1], *name, orig[WORD+1];
	CHAR **layers, **layer_numbers;
	NODEPROTO *np, *nnp;
	ARCPROTO *ap, *lap;
	ARCINST *ai;
	PORTPROTO *ppt, *fpp, *lpp, *pp, *lastport;
	NODEINST *ni, *lastpin, *fni, *lni;
	EPT_PTR point;
	EDPROPERTY_PTR property, nproperty;
	double ar, so;
	VARIABLE *var;
	XARRAY rot;
	EDPORT_PTR eport, neport;
	/* bus connection variables */
	PORTPROTO *fpps[MAXBUSPINS], *tpps[MAXBUSPINS];
	NODEINST *fnis[MAXBUSPINS], *tnis[MAXBUSPINS];
	REGISTER void *infstr;

	lambda = el_curlib->lambda[io_edifgbl.technology->techindex];
	if (kstack_ptr)
	{
		if (!io_edifgbl.ignoreblock)
		{
			switch (io_edifgbl.state)
			{
			case KFIGURE:
				io_edifgbl.cur_nametbl = NONAMETABLE;
				io_edifgbl.visible = 1;
				io_edifgbl.justification = LOWERLEFT;
				io_edifgbl.textheight = 0;
				break;
			case KBOUNDINGBOX:
				io_edifgbl.figure_group = NONODEPROTO;
				break;
			case KTECHNOLOGY:
				var = getval((INTBIG)io_edifgbl.technology, VTECHNOLOGY, VSTRING|VISARRAY,
					x_("IO_gds_layer_numbers"));
				if (var != NOVARIABLE)
				{
					layer_numbers = (CHAR **)var->addr;
					count = getlength(var);
					var = getval((INTBIG)io_edifgbl.technology, VTECHNOLOGY, VSTRING|VISARRAY,
						x_("TECH_layer_names"));
					if (var != NOVARIABLE)
					{
						layers = (CHAR **) var->addr;
						user_max = io_edifgbl.layer_ptr;
						/* for all layers assign their GDS number */
						for (i=0; i<count; i++)
						{
							if (*layer_numbers[i] != 0)
							{
								/* search for this layer */
								for (j=0; j<user_max; j++)
								{
									if (!namesame(io_edifgbl.nametbl[j]->replace, layers[i])) break;
								}
								if (user_max == j)
								{
									/* add to the list */
									io_edifgbl.nametbl[io_edifgbl.layer_ptr] = (NAMETABLE_PTR)
										emalloc(sizeof(NAMETABLE), io_tool->cluster);
									if (io_edifgbl.nametbl[io_edifgbl.layer_ptr] == NONAMETABLE) RET_NOMEMORY();
									if (allocstring(&io_edifgbl.nametbl[io_edifgbl.layer_ptr]->replace, layers[i],
										io_tool->cluster)) RET_NOMEMORY();
									esnprintf(orig, WORD+1, x_("layer_%s"), layer_numbers[i]);
									if (allocstring(&io_edifgbl.nametbl[io_edifgbl.layer_ptr]->original,
										orig, io_tool->cluster)) RET_NOMEMORY();
									io_edifgbl.nametbl[io_edifgbl.layer_ptr]->textheight = 0;
									io_edifgbl.nametbl[io_edifgbl.layer_ptr]->justification = LOWERLEFT;
									io_edifgbl.layer_ptr++;
								}
							}
						}
					}
				}

				/* sort the layer list */
				esort(io_edifgbl.nametbl, io_edifgbl.layer_ptr, sizeof(NAMETABLE_PTR),
					io_edcompare_name);

				/* now look for nodes to map MASK layers to */
				for (eindex = 0; eindex < io_edifgbl.layer_ptr; eindex++)
				{
					esnprintf(nodename, WORD+1, x_("%s-node"), io_edifgbl.nametbl[eindex]->replace);
					for (np = io_edifgbl.technology->firstnodeproto; np != NONODEPROTO;
						np = np->nextnodeproto)
					{
						if (!namesame(nodename, np->protoname)) break;
					}
					if (np == NONODEPROTO)
					{
						np = art_boxprim;
					}
					for (ap = io_edifgbl.technology->firstarcproto; ap != NOARCPROTO;
						ap = ap->nextarcproto)
					{
						if (!namesame(ap->protoname, io_edifgbl.nametbl[eindex]->replace))
							break;
					}
					io_edifgbl.nametbl[eindex]->node = np;
					io_edifgbl.nametbl[eindex]->arc = ap;
				}
				break;
			case KINTERFACE:
				if (io_edifgbl.active_view == VNETLIST)
				{
					/* create a black-box symbol at the current scale */
					np = io_edifgbl.current_cell;
					(void)esnprintf(nodename, WORD+1, x_("%s{ic}"), np->protoname);
					nnp = io_edmakeiconcell(np->firstportproto, np->protoname,
						nodename, io_edifgbl.library);
					if (nnp == NONODEPROTO)
					{
						ttyputerr(_("error, line #%d: could not create icon <%s>"),
							io_edifgbl.lineno, nodename);
						io_edifgbl.errors++;
					} else
					{
						/* now compute the bounds of this cell */
						db_boundcell(nnp, &nnp->lowx, &nnp->highx, &nnp->lowy, &nnp->highy);
					}
				}
				break;
			case KVIEW:

				if (io_edifgbl.vendor == EVVIEWLOGIC && io_edifgbl.active_view != VNETLIST)
				{
					/* fixup incorrect bus nets */
					for (ai = io_edifgbl.current_cell->firstarcinst; ai != NOARCINST;
						ai = ai->nextarcinst)
					{
						ai->temp1 = ai->temp2 = 0;
					}

					/* now scan for BUS nets, and verify all wires connected to bus */
					for (ai = io_edifgbl.current_cell->firstarcinst; ai != NOARCINST;
						ai = ai->nextarcinst)
					{
						if (ai->temp1 == 0 && ai->proto == sch_busarc)
						{
							/* get name of arc */
							var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
							if (var != NOVARIABLE && ((name = (CHAR *)var->addr) != NULL))
							{
								/* create the basename */
								(void)estrcpy(basename, name);
								name = estrrchr(basename, '[');
								if (name != NULL) *name = 0;

								/* expand this arc, and locate non-bracketed names */
								io_edcheck_busnames(ai, basename);
							}
						}
					}
				}

				/* move the port list to the free list */
				for (eport = io_edifgbl.ports; eport != NOEDPORT; eport = neport)
				{
					neport = eport->next;
					eport->next = io_edifgbl.free_ports;
					io_edifgbl.free_ports = eport;
					efree(eport->name);
					efree(eport->reference);
				}
				io_edifgbl.ports = NOEDPORT;
				break;
			case KPORT:
				if (io_edallocport()) return(1);
				eport = io_edifgbl.ports;
				np = io_edifgbl.current_cell;
				switch (eport->direction)
				{
					case INPUTE:
						cx = 0;
						cy = io_edifgbl.ipos;
						io_edifgbl.ipos += INCH;
						fpp = default_input;
						break;
					case INOUT:
						cx = 3*INCH;
						cy = io_edifgbl.bpos;
						io_edifgbl.bpos += INCH;
						fpp = default_input;
						break;
					case OUTPUTE:
						cx = 6*INCH;
						cy = io_edifgbl.opos;
						io_edifgbl.opos += INCH;
						fpp = default_output;
						break;
				}

				/* now create the off-page reference */
				defaultnodesize(sch_offpageprim, &psx, &psy);
				ni = newnodeinst(sch_offpageprim,
					cx + (sch_offpageprim->lowx+sch_offpageprim->highx-psx)/2,
					cx + (sch_offpageprim->lowx+sch_offpageprim->highx+psx)/2,
					cy + (sch_offpageprim->lowy+sch_offpageprim->highy-psy)/2,
					cy + (sch_offpageprim->lowy+sch_offpageprim->highy+psy)/2, 0,0, np);
				if (ni == NONODEINST)
				{
					ttyputerr(_("error, line #%d: could not create external port"),
						io_edifgbl.lineno);
					io_edifgbl.errors++;
					break;
				}

				/* now create the port */
				ppt = newportproto(ni->parent, ni, fpp, eport->name);
				if (ppt == NOPORTPROTO)
				{
					ttyputerr(_("error, line #%d: could not create port <%s>"),
						io_edifgbl.lineno, eport->name);
					io_edifgbl.errors++;
				} else
				{
					switch (eport->direction)
					{
						case INPUTE:
							ppt->userbits = (ppt->userbits & ~STATEBITS) | INPORT;
							break;
						case OUTPUTE:
							ppt->userbits = (ppt->userbits & ~STATEBITS) | OUTPORT;
							break;
						case INOUT:
							ppt->userbits = (ppt->userbits & ~STATEBITS) | BIDIRPORT;
							break;
					}
				}
				io_edifgbl.port_reference[0] = 0;

				/* move the property list to the free list */
				for (property = io_edifgbl.properties; property != NOEDPROPERTY; property = nproperty)
				{
					key = makekey(property->name);
					switch (property->type)
					{
						case PINTEGER:
							var = setvalkey((INTBIG)ppt, VPORTPROTO, key, (INTBIG)property->val.integer,
								VINTEGER);
							break;
						case PNUMBER:
							var = setvalkey((INTBIG)ppt, VPORTPROTO, key, castint(property->val.number),
								VFLOAT);
							break;
						case PSTRING:
							var = setvalkey((INTBIG)ppt, VPORTPROTO, key, (INTBIG)property->val.string,
								VSTRING);
							break;
						default:
							break;
					}
					nproperty = property->next;
					property->next = io_edifgbl.free_properties;
					io_edifgbl.free_properties = property;
					efree(property->name);
				}
				io_edifgbl.properties = NOEDPROPERTY;
				break;
			case KINSTANCE:
				if (io_edifgbl.active_view == VNETLIST)
				{
					for (Ix = 0; Ix < io_edifgbl.arrayx; Ix++)
					{
						for (Iy = 0; Iy < io_edifgbl.arrayy; Iy++)
						{
							/* create this instance in the current sheet */
							width = io_edifgbl.proto->highx - io_edifgbl.proto->lowx;
							height = io_edifgbl.proto->highy - io_edifgbl.proto->lowy;
							width = (width + INCH - 1) / INCH;
							height = (height + INCH - 1) / INCH;

							/* verify room for the icon */
							if (io_edifgbl.sh_xpos != -1)
							{
								if ((io_edifgbl.sh_ypos + (height + 1)) >=
									EDIF_sheet_bounds[io_edifgbl.sheet_size].height)
								{
									io_edifgbl.sh_ypos = 1;
									if ((io_edifgbl.sh_xpos += io_edifgbl.sh_offset) >=
										EDIF_sheet_bounds[io_edifgbl.sheet_size].width)
											io_edifgbl.sh_xpos = io_edifgbl.sh_ypos = -1; else
												io_edifgbl.sh_offset = 2;
								}
							}
							if (io_edifgbl.sh_xpos == -1)
							{
								/* create the new page */
								(void)esnprintf(nodename, WORD+1, x_("%s{p%d}"), io_edifgbl.cell_name,
									++io_edifgbl.pageno);
								io_edifgbl.current_cell = np = us_newnodeproto(nodename, io_edifgbl.library);
								if (io_edifgbl.current_cell == NONODEPROTO)
									longjmp(io_edifgbl.env, LONGJMPNOMEM);
								np->temp1 = 0;
								io_edifgbl.sh_xpos = io_edifgbl.sh_ypos = 1;
								io_edifgbl.sh_offset = 2;
							}

							/* create this instance */
							/* find out where true center moves */
							cx = ((io_edifgbl.proto->lowx+io_edifgbl.proto->highx) >> 1) +
								((io_edifgbl.sh_xpos -
								(EDIF_sheet_bounds[io_edifgbl.sheet_size].width >> 1)) * INCH);
							cy = ((io_edifgbl.proto->lowy+io_edifgbl.proto->highy) >> 1) +
								((io_edifgbl.sh_ypos -
								(EDIF_sheet_bounds[io_edifgbl.sheet_size].height >> 1)) * INCH);
							io_edifgbl.current_node = ni = newnodeinst(io_edifgbl.proto,
								io_edifgbl.proto->lowx + cx,
								io_edifgbl.proto->highx + cx,
								io_edifgbl.proto->lowy + cy,
								io_edifgbl.proto->highy + cy,
								io_edgettrans(io_edifgbl.orientation),
								io_edgetrot(io_edifgbl.orientation),
								io_edifgbl.current_cell);
							if (ni == NONODEINST)
							{
								ttyputerr(_("error, line #%d: could not create instance"),
									io_edifgbl.lineno);
								io_edifgbl.errors++;
								break;
							} else
							{
								if (io_edifgbl.proto->userbits & WANTNEXPAND)
									ni->userbits |= NEXPAND;

								/* update the current position */
								if ((width + 2) > io_edifgbl.sh_offset)
									io_edifgbl.sh_offset = width + 2;
								if ((io_edifgbl.sh_ypos += (height + 1)) >=
									EDIF_sheet_bounds[io_edifgbl.sheet_size].height)
								{
									io_edifgbl.sh_ypos = 1;
									if ((io_edifgbl.sh_xpos += io_edifgbl.sh_offset) >=
										EDIF_sheet_bounds[io_edifgbl.sheet_size].width)
										io_edifgbl.sh_xpos = io_edifgbl.sh_ypos = -1; else
											io_edifgbl.sh_offset = 2;
								}

								/* name the instance */
								if (io_edifgbl.instance_reference[0] != 0)
								{
									/* if single element or array with no offset */
									/* construct the representative extended EDIF name (includes [...]) */
									if ((io_edifgbl.arrayx == 1 && io_edifgbl.arrayy == 1) ||
										(io_edifgbl.deltaxX == 0 && io_edifgbl.deltaxY == 0 &&
										io_edifgbl.deltayX == 0 && io_edifgbl.deltayY == 0))
											(void)estrcpy(nodename, io_edifgbl.instance_reference);
										/* if array in the x dimension */
									else if (io_edifgbl.arrayx > 1)
									{
										if (io_edifgbl.arrayy > 1)
											(void)esnprintf(nodename, WORD+1, x_("%s[%ld,%ld]"),
												io_edifgbl.instance_reference, Ix, Iy);
										else
											(void)esnprintf(nodename, WORD+1, x_("%s[%ld]"),
												io_edifgbl.instance_reference, Ix);
									}
									/* if array in the y dimension */
									else if (io_edifgbl.arrayy > 1)
										(void)esnprintf(nodename, WORD+1, x_("%s[%ld]"),
											io_edifgbl.instance_reference, Iy);

									/* check for array element descriptor */
									if (io_edifgbl.arrayx > 1 || io_edifgbl.arrayy > 1)
									{
										/* array descriptor is of the form index:index:range index:index:range */
										(void)esnprintf(basename, WORD+1, x_("%ld:%ld:%d %ld:%ld:%d"), Ix,
											(io_edifgbl.deltaxX == 0 && io_edifgbl.deltayX == 0) ?
											io_edifgbl.arrayx-1:Ix, io_edifgbl.arrayx, Iy,
											(io_edifgbl.deltaxY == 0 && io_edifgbl.deltayY == 0) ?
											io_edifgbl.arrayy-1:Iy, io_edifgbl.arrayy);
										var = setvalkey((INTBIG)ni, VNODEINST, EDIF_array_key, (INTBIG)basename,
											VSTRING);

									}

									/* now set the name of the component (note that Electric allows any string
									 * of characters as a name, this name is open to the user, for ECO and other
									 * consistancies, the EDIF_name is saved on a variable)
									 */
									if (!namesame(io_edifgbl.instance_reference, io_edifgbl.instance_name))
									{
										var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key,
											(INTBIG)nodename, VSTRING|VDISPLAY);
										if (var != NOVARIABLE)
											defaulttextsize(3, var->textdescript);
									} else
									{
										/* now add the original name as the displayed name (only to the first element) */
										if (Ix == 0 && Iy == 0)
										{
											var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key,
												(INTBIG)io_edifgbl.instance_name, VSTRING|VDISPLAY);
											if (var != NOVARIABLE)
												defaulttextsize(3, var->textdescript);
										}
										/* now save the EDIF name (not displayed) */
										var = setvalkey((INTBIG)ni, VNODEINST, EDIF_name_key,
											(INTBIG)nodename, VSTRING);
									}
								}
							}
						}
					}
				}

				/* move the property list to the free list */
				for (property = io_edifgbl.properties; property != NOEDPROPERTY; property = nproperty)
				{
					if (io_edifgbl.current_node != NONODEINST)
					{
						key = makekey(property->name);
						switch (property->type)
						{
							case PINTEGER:
								var = setvalkey((INTBIG)io_edifgbl.current_node, VNODEINST, key,
									(INTBIG)property->val.integer, VINTEGER);
								break;
							case PNUMBER:
								var = setvalkey((INTBIG)io_edifgbl.current_node, VNODEINST, key,
									castint(property->val.number), VFLOAT);
								break;
							case PSTRING:
								var = setvalkey((INTBIG)io_edifgbl.current_node, VNODEINST, key,
									(INTBIG)property->val.string, VSTRING);
								break;
							default:
								break;
						}
					}
					nproperty = property->next;
					property->next = io_edifgbl.free_properties;
					io_edifgbl.free_properties = property;
					efree(property->name);
				}
				io_edifgbl.properties = NOEDPROPERTY;
				io_edifgbl.instance_reference[0] = 0;
				io_edifgbl.current_node = NONODEINST;
				io_edfreesavedptlist();
				break;
			case KNET:
				/* move the property list to the free list */
				for (property = io_edifgbl.properties; property != NOEDPROPERTY; property = nproperty)
				{
					if (io_edifgbl.current_arc != NOARCINST)
					{
						key = makekey(property->name);
						switch (property->type)
						{
							case PINTEGER:
								var = setvalkey((INTBIG)io_edifgbl.current_arc, VARCINST, key,
									(INTBIG)property->val.integer, VINTEGER);
								break;
							case PNUMBER:
								var = setvalkey((INTBIG)io_edifgbl.current_arc, VARCINST, key,
									castint(property->val.number), VFLOAT);
								break;
							case PSTRING:
								var = setvalkey((INTBIG)io_edifgbl.current_arc, VARCINST, key,
									(INTBIG)property->val.string, VSTRING);
								break;
							default:
								break;
						}
					}
					nproperty = property->next;
					property->next = io_edifgbl.free_properties;
					io_edifgbl.free_properties = property;
					efree(property->name);
				}
				io_edifgbl.properties = NOEDPROPERTY;
				io_edifgbl.net_reference[0] = 0;
				io_edifgbl.current_arc = NOARCINST;
				if (io_edifgbl.geometry != GBUS)
					io_edifgbl.geometry = GUNKNOWN;
				io_edfreesavedptlist();
				break;
			case KNETBUNDLE:
				io_edifgbl.bundle_reference[0] = 0;
				io_edifgbl.current_arc = NOARCINST;
				io_edifgbl.geometry = GUNKNOWN;
				io_edfreesavedptlist();
				break;
			case KPROPERTY:
				if (io_edifgbl.active_view == VNETLIST || io_edifgbl.active_view == VSCHEMATIC)
				{
					/* add as a variable to the current object */
					i = 0;
					switch (kstack[kstack_ptr - 1])
					{
						case KINTERFACE:
							/* add to the {sch} view nodeproto */
							for (np = io_edifgbl.library->firstnodeproto; np != NONODEPROTO;
								np = np->nextnodeproto)
							{
								if (!namesame(np->protoname, io_edifgbl.cell_name) &&
									!namesame(np->cellview->sviewname, x_("sch"))) break;
							}
							if (np == NONODEPROTO)
							{
								/* allocate the cell */
								esnprintf(nodename, WORD+1, x_("%s{sch}"), io_edifgbl.cell_name);
								np = us_newnodeproto(nodename, io_edifgbl.library);
								if (np == NONODEPROTO) longjmp(io_edifgbl.env, LONGJMPNOMEM);
								np->temp1 = 0;
							}
							i = (INTBIG) np;
							j = VNODEPROTO;
							break;
						case KINSTANCE:
						case KNET:
						case KPORT:
							i = -1;
							break;
						default:
							i = 0;
							break;
					}
					if (i > 0)
					{
						key = makekey(io_edifgbl.property_reference);
						switch (io_edifgbl.ptype)
						{
							case PINTEGER:
								var = setvalkey(i, j, key, (INTBIG)io_edifgbl.pval.integer, VINTEGER);
								break;
							case PNUMBER:
								var = setvalkey(i, j, key, castint(io_edifgbl.pval.number), VFLOAT);
								break;
							case PSTRING:
								var = setvalkey(i, j, key, (INTBIG)io_edifgbl.pval.string, VSTRING);
								break;
							default:
								break;
						}
					} else if (i == -1)
					{
						/* add to the current property list, will be added latter */
						switch (io_edifgbl.ptype)
						{
							case PINTEGER:
								(void)io_edallocproperty(io_edifgbl.property_reference, io_edifgbl.ptype,
									(INTBIG)io_edifgbl.pval.integer, 0.0, NULL);
								break;
							case PNUMBER:
								(void)io_edallocproperty(io_edifgbl.property_reference, io_edifgbl.ptype,
									0, io_edifgbl.pval.number, NULL);
								break;
							case PSTRING:
								(void)io_edallocproperty(io_edifgbl.property_reference, io_edifgbl.ptype,
									0, 0.0, io_edifgbl.pval.string);
								break;
							default:
								break;
						}
					}
				}
				io_edifgbl.property_reference[0] = 0;
				io_edfreesavedptlist();
				break;
			case KPORTIMPLEMENTATION:
				io_edifgbl.geometry = GUNKNOWN;
				break;
			case KTRANSFORM:
				if (kstack_ptr <= 1 || kstack[kstack_ptr-1] != KINSTANCE)
				{
					io_edfreeptlist();
					break;
				}

				/* get the corner offset */
				instcount = io_edifgbl.pt_count;
				if (instcount > 0)
				{
					instptx = io_edifgbl.points->x;
					instpty = io_edifgbl.points->y;
				}

				/* if no points are specified, presume the origin */
				if (instcount == 0)
				{
					instptx = instpty = 0;
					instcount = 1;
				}

				/* create node instance rotations about the origin not center */
				makeangle(io_edgetrot(io_edifgbl.orientation), io_edgettrans(io_edifgbl.orientation), rot);

				if (instcount == 1 && io_edifgbl.proto != NONODEPROTO)
				{
					for (Ix = 0; Ix < io_edifgbl.arrayx; Ix++)
					{
						lx = instptx + Ix * io_edifgbl.deltaxX;
						ly = instpty + Ix * io_edifgbl.deltaxY;
						for (Iy = 0; Iy < io_edifgbl.arrayy; Iy++)
						{
							/* find out where true center moves */
							cx = (io_edifgbl.proto->lowx+io_edifgbl.proto->highx)/2;
							cy = (io_edifgbl.proto->lowy+io_edifgbl.proto->highy)/2;
							xform(cx, cy, &gx, &gy, rot);

							/* now calculate the delta movement of the center */
							cx = gx - cx;
							cy = gy - cy;
							io_edifgbl.current_node = ni = newnodeinst(io_edifgbl.proto,
								lx + io_edifgbl.proto->lowx + cx,
								lx + io_edifgbl.proto->highx + cx,
								ly + io_edifgbl.proto->lowy + cy,
								ly + io_edifgbl.proto->highy + cy,
								io_edgettrans(io_edifgbl.orientation),
								io_edgetrot(io_edifgbl.orientation),
								io_edifgbl.current_cell);
							if (ni == NONODEINST)
							{
								ttyputerr(_("error, line #%d: could not create instance"),
									io_edifgbl.lineno);
								io_edifgbl.errors++;

								/* and exit for loop */
								Ix = io_edifgbl.arrayx;
								Iy = io_edifgbl.arrayy;
							} else
							{
								if (io_edifgbl.proto->userbits & WANTNEXPAND)
									ni->userbits |= NEXPAND;
							}
							if (io_edifgbl.geometry == GPIN && lx == 0 && ly == 0)
							{
								/* determine an appropriate port name */
								(void)estrcpy(portname, io_edifgbl.port_name);

								/* check if port already exists (will occur for multiple portImplementation statements */
								for (dup = 0, (void)estrcpy(basename, portname);
									(ppt = getportproto(ni->parent, portname)) != NOPORTPROTO;
									dup++)
								{
									if (dup == 0) pp = ppt;
									(void)esnprintf(portname, WORD+1, x_("%s_%ld"), basename, dup+1);
								}

								/* only once */
								Ix = io_edifgbl.arrayx;
								Iy = io_edifgbl.arrayy;
								ppt = newportproto(ni->parent, ni, default_iconport, portname);
								if (ppt == NOPORTPROTO)
								{
									ttyputerr(_("error, line #%d: could not create port <%s>"),
										io_edifgbl.lineno, portname);
									io_edifgbl.errors++;
								} else
								{
									/* locate the direction of the port */
									for (eport = io_edifgbl.ports; eport != NOEDPORT; eport = eport->next)
									{
										if (!namesame(eport->reference, io_edifgbl.port_reference))
										{
											/* set the direction */
											switch (eport->direction)
											{
												case INPUTE:
													ppt->userbits = (ppt->userbits & ~STATEBITS) | INPORT;
													break;
												case OUTPUTE:
													ppt->userbits = (ppt->userbits & ~STATEBITS) | OUTPORT;
													break;
												case INOUT:
													ppt->userbits = (ppt->userbits & ~STATEBITS) | BIDIRPORT;
													break;
											}
											break;
										}
									}
								}
							} else
							{
								/* name the instance */
								if (io_edifgbl.instance_reference[0] != 0)
								{
									/* if single element or array with no offset */
									/* construct the representative extended EDIF name (includes [...]) */
									if ((io_edifgbl.arrayx == 1 && io_edifgbl.arrayy == 1) ||
										(io_edifgbl.deltaxX == 0 && io_edifgbl.deltaxY == 0 &&
										io_edifgbl.deltayX == 0 && io_edifgbl.deltayY == 0))
										(void)estrcpy(nodename, io_edifgbl.instance_reference);
										/* if array in the x dimension */
									else if (io_edifgbl.arrayx > 1)
									{
										if (io_edifgbl.arrayy > 1)
											(void)esnprintf(nodename, WORD+1, x_("%s[%ld,%ld]"),
												io_edifgbl.instance_reference, Ix, Iy);
										else
											(void)esnprintf(nodename, WORD+1, x_("%s[%ld]"),
												io_edifgbl.instance_reference, Ix);
									}
									/* if array in the y dimension */
									else if (io_edifgbl.arrayy > 1)
										(void)esnprintf(nodename, WORD+1, x_("%s[%ld]"),
											io_edifgbl.instance_reference, Iy);

									/* check for array element descriptor */
									if (io_edifgbl.arrayx > 1 || io_edifgbl.arrayy > 1)
									{
										/* array descriptor is of the form index:index:range index:index:range */
										(void)esnprintf(basename, WORD+1, x_("%ld:%ld:%d %ld:%ld:%d"),
											Ix, (io_edifgbl.deltaxX == 0 && io_edifgbl.deltayX == 0) ? io_edifgbl.arrayx-1:Ix,
											io_edifgbl.arrayx, Iy,
											(io_edifgbl.deltaxY == 0 && io_edifgbl.deltayY == 0) ? io_edifgbl.arrayy-1:Iy,
											io_edifgbl.arrayy);
										var = setvalkey((INTBIG)ni, VNODEINST, EDIF_array_key, (INTBIG)basename,
											VSTRING);
									}

									/* now set the name of the component (note that Electric allows any string
			   						 * of characters as a name, this name is open to the user, for ECO and other
									 * consistancies, the EDIF_name is saved on a variable)
									 */
									if (!namesame(io_edifgbl.instance_reference, io_edifgbl.instance_name))
									{
										var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key,
											(INTBIG)nodename, VSTRING|VDISPLAY);
										if (var != NOVARIABLE)
											defaulttextsize(3, var->textdescript);
									} else
									{
										/* now add the original name as the displayed name (only to the first element) */
										if (Ix == 0 && Iy == 0)
										{
											var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key,
												(INTBIG)io_edifgbl.instance_name, VSTRING|VDISPLAY);
											if (var != NOVARIABLE)
												defaulttextsize(3, var->textdescript);
										}

										/* now save the EDIF name (not displayed) */
										var = setvalkey((INTBIG)ni, VNODEINST, EDIF_name_key,
											(INTBIG)nodename, VSTRING);
									}

									/* now check for saved name attributes */
									if (io_edifgbl.save_text.points != NOEPT)
									{
										/* now set the position, relative to the center of the current object */
										xoff = io_edifgbl.save_text.points->x - ((ni->highx+ni->lowx)>>1);
										yoff = io_edifgbl.save_text.points->y - ((ni->highy+ni->lowy)>>1);

										/* convert to quarter lambda units */
										xoff = 4*xoff / lambda;
										yoff = 4*yoff / lambda;

										/*
										 * determine the size of text, 0.0278 in == 2 points or 36 (2xpixels) == 1 in
										 * fonts range from 4 to 20 points
										 */
										if (io_edifgbl.save_text.textheight == 0) i = TXTSETQLAMBDA(4); else
										{
											i = io_ediftextsize(io_edifgbl.save_text.textheight);
										}
										TDCOPY(descript, var->textdescript);
										TDSETOFF(descript, xoff, yoff);
										TDSETSIZE(descript, i);
										TDSETPOS(descript, VTPOSCENT);
										switch (io_edifgbl.save_text.justification)
										{
											case UPPERLEFT:
												TDSETPOS(descript, VTPOSUPRIGHT);
												break;
											case UPPERCENTER:
												TDSETPOS(descript, VTPOSUP);
												break;
											case UPPERRIGHT:
												TDSETPOS(descript, VTPOSUPLEFT);
												break;
											case CENTERLEFT:
												TDSETPOS(descript, VTPOSRIGHT);
												break;
											case CENTERCENTER:
												TDSETPOS(descript, VTPOSCENT);
												break;
											case CENTERRIGHT:
												TDSETPOS(descript, VTPOSLEFT);
												break;
											case LOWERLEFT:
												TDSETPOS(descript, VTPOSDOWNRIGHT);
												break;
											case LOWERCENTER:
												TDSETPOS(descript, VTPOSDOWN);
												break;
											case LOWERRIGHT:
												TDSETPOS(descript, VTPOSDOWNLEFT);
												break;
										}
										TDCOPY(var->textdescript, descript);
									}
								}
							}
							if (io_edifgbl.deltayX == 0 && io_edifgbl.deltayY == 0) break;

							/* bump the y delta and x deltas */
							lx += io_edifgbl.deltayX;
							ly += io_edifgbl.deltayY;
						}
						if (io_edifgbl.deltaxX == 0 && io_edifgbl.deltaxY == 0) break;
					}
				}
				io_edfreeptlist();
				break;
			case KPORTREF:
				/* check for the last pin */
				fni = io_edifgbl.current_node;
				fpp = io_edifgbl.current_port;
				if (io_edifgbl.port_reference[0] != 0)
				{
					/* For internal pins of an instance, determine the base port location and
					 * other pin assignments
					 */
					if (io_edifgbl.instance_reference[0] != 0)
					{
						(void)estrcpy(nodename, io_edifgbl.instance_reference);

						/* locate the node and and port */
						if (io_edifgbl.active_view == VNETLIST)
						{
							/* scan all pages for this nodeinst */
							ni = NONODEINST;
							for (np = io_edifgbl.library->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
							{
								if (namesame(np->protoname, io_edifgbl.cell_name) != 0) continue;
								for (ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
								{
									if ((var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, EDIF_name_key)) == NOVARIABLE &&
										(var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key)) == NOVARIABLE)
											continue;
									if (!namesame((CHAR *)var->addr, nodename)) break;
								}
								if (ni != NONODEINST) break;
							}
							if (ni == NONODEINST)
							{
								(void)ttyputmsg(_("error, line #%d: could not locate netlist node (%s)"),
									io_edifgbl.lineno, nodename);
								break;
							}
							ap = gen_unroutedarc;
						} else
						{
							/* net always references the current page */
							for (ni = io_edifgbl.current_cell->firstnodeinst; ni != NONODEINST;
								ni = ni->nextnodeinst)
							{
								if ((var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, EDIF_name_key)) == NOVARIABLE &&
									(var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key)) == NOVARIABLE)
										continue;
								if (!namesame((CHAR *)var->addr, nodename)) break;
							}
							if (ni == NONODEINST)
							{
								(void)ttyputmsg(_("error, line #%d: could not locate schematic node '%s' in cell %s"),
									io_edifgbl.lineno, nodename, describenodeproto(io_edifgbl.current_cell));
								break;
							}
							if (io_edifgbl.isarray == 0) ap = sch_wirearc; else
								ap = sch_busarc;
						}

						/* locate the port for this portref */
						pp = getportproto(ni->proto, io_edifgbl.port_reference);
						if (pp == NOPORTPROTO)
						{
							(void)ttyputmsg(_("error, line #%d: could not locate port (%s) on node (%s)"),
								io_edifgbl.lineno, io_edifgbl.port_reference, nodename);
							break;
						}

						/* we have both, set global variable */
						io_edifgbl.current_node = ni;
						io_edifgbl.current_port = pp;
						np = ni->parent;

						/* create extensions for net labels on single pin nets (externals), and
						 * placeholder for auto-routing later
						 */
						if (io_edifgbl.active_view == VNETLIST)
						{
							/* route all pins with an extension */
							if (ni != NONODEINST)
							{
								portposition(ni, pp, &hx, &hy);
								lx = hx;
								ly = hy;
								switch (pp->userbits&STATEBITS)
								{
									case INPORT:
										lx = hx - (INCH/10);
										break;
									case BIDIRPORT:
										ly = hy - (INCH/10);
										break;
									case OUTPORT:
										lx = hx + (INCH/10);
										break;
								}

								/* need to create a destination for the wire */
								if (io_edifgbl.isarray != 0)
								{
									lni = io_edifiplacepin(sch_buspinprim, lx + sch_buspinprim->lowx,
										lx + sch_buspinprim->highx, ly + sch_buspinprim->lowy,
										ly + sch_buspinprim->highy, 0, 0, np);
									if (lni == NONODEINST)
									{
										ttyputmsg(_("error, line#%d: could not create bus pin"),
											io_edifgbl.lineno);
										break;
									}
									lpp = default_busport;
									lap = sch_busarc;
								} else
								{
									lni = io_edifiplacepin(sch_wirepinprim, lx + sch_wirepinprim->lowx,
										lx + sch_wirepinprim->highx, ly + sch_wirepinprim->lowy,
										ly + sch_wirepinprim->highy, 0, 0, np);
									if (lni == NONODEINST)
									{
										ttyputmsg(_("error, line#%d: could not create wire pin"),
											io_edifgbl.lineno);
										break;
									}
									lpp = default_port;
									lap = sch_wirearc;
								}
								io_edifgbl.current_arc = newarcinst(lap, defaultarcwidth(lap), CANTSLIDE,
									lni, lpp, lx, ly, ni, pp, hx, hy, np);
								if (io_edifgbl.current_arc == NOARCINST)
									ttyputmsg(_("error, line #%d: could not create auto-path"),
										io_edifgbl.lineno);
								else
									io_ednamearc(io_edifgbl.current_arc);
							}
						}
					} else
					{
						/* external port reference, look for a off-page reference in {sch} with this
			 			  port name */
						for (np = io_edifgbl.library->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
						{
							if (namesame(np->protoname, io_edifgbl.cell_name) != 0) continue;
							if (np->cellview == el_schematicview) break;
						}
						if (np == NONODEPROTO)
						{
							ttyputmsg(_("error, line #%d: could not locate top level schematic"),
								io_edifgbl.lineno);
							break;
						}

						/* now look for an instance with the correct port name */
						pp = getportproto(np, io_edifgbl.port_reference);
						if (pp == NOPORTPROTO)
						{
							if (io_edifgbl.original != 0 && *io_edifgbl.original != 0)
								pp = getportproto(np, io_edifgbl.original);
							if (pp == NOPORTPROTO)
							{
								ttyputmsg(_("error, line #%d: could not locate port '%s'"), io_edifgbl.lineno,
									io_edifgbl.port_reference);
								break;
							}
						}
						fni = pp->subnodeinst;
						fpp = pp->subportproto;
						portposition(fni, fpp, &lx, &ly);

						/* determine x position by original placement */
						switch (pp->userbits&STATEBITS)
						{
							case INPORT:
								lx = 1*INCH;
								break;
							case BIDIRPORT:
								lx = 4*INCH;
								break;
							case OUTPORT:
								lx = 5*INCH;
								break;
						}

						/* need to create a destination for the wire */
						if (io_edifgbl.isarray != 0)
						{
							ni = io_edifiplacepin(sch_buspinprim, lx + sch_buspinprim->lowx,
								lx + sch_buspinprim->highx, ly + sch_buspinprim->lowy,
								ly + sch_buspinprim->highy, 0, 0, np);
							if (ni == NONODEINST)
							{
								ttyputmsg(_("error, line#%d: could not create bus pin"),
									io_edifgbl.lineno);
								break;
							}
							pp = default_busport;
						} else
						{
							ni = io_edifiplacepin(sch_wirepinprim, lx + sch_wirepinprim->lowx,
								lx + sch_wirepinprim->highx, ly + sch_wirepinprim->lowy,
								ly + sch_wirepinprim->highy, 0, 0, np);
							if (ni == NONODEINST)
							{
								ttyputmsg(_("error, line#%d: could not create wire pin"),
									io_edifgbl.lineno);
								break;
							}
							pp = default_port;
						}
						if (io_edifgbl.isarray == 0) ap = sch_wirearc; else
							ap = sch_busarc;
					}

					/* now connect if we have from node and port */
					if (fni != NONODEINST && fpp != NOPORTPROTO)
					{
						if (fni->parent != ni->parent)
						{
							ttyputmsg(_("error, line #%d: could not create path (arc) between cells %s and %s"),
								io_edifgbl.lineno, describenodeproto(fni->parent), describenodeproto(ni->parent));
						} else
						{
							/* locate the position of the new ports */
							portposition(fni, fpp, &lx, &ly);
							portposition(ni, pp, &hx, &hy);

							/* for nets with no physical representation ... */
							io_edifgbl.current_arc = NOARCINST;
							if (lx == hx && ly == hy) dist = 0; else
								dist = computedistance(lx, ly, hx, hy);
							if (dist <= lambda*SLOPDISTANCE)
							{
								io_edifgbl.current_arc = newarcinst(ap, defaultarcwidth(ap),
									FIXANG|CANTSLIDE, fni, fpp, lx, ly, ni, pp, hx, hy, np);
								if (io_edifgbl.current_arc == NOARCINST)
								{
									ttyputmsg(_("error, line #%d: could not create path (arc)"),
										io_edifgbl.lineno);
								}
							}
							/* use unrouted connection for NETLIST views */
							else if (io_edifgbl.active_view == VNETLIST)
							{
								io_edifgbl.current_arc = newarcinst(ap, defaultarcwidth(ap),
									CANTSLIDE, fni, fpp, lx, ly, ni, pp, hx, hy, np);
								if (io_edifgbl.current_arc == NOARCINST)
								{
									ttyputmsg(_("error, line #%d: could not create auto-path"),
										io_edifgbl.lineno);
								}
							}

							/* add the net name */
							if (io_edifgbl.current_arc != NOARCINST)
								io_ednamearc(io_edifgbl.current_arc);
						}
					}
				}
				break;
			case KINSTANCEREF:
				break;
			case KPATH:
				/* check for openShape type path */
				if (io_edifgbl.path_width == 0 &&
					io_edifgbl.geometry != GNET && io_edifgbl.geometry != GBUS) goto dopoly;
				fcnt = 0;
				if (io_edifgbl.geometry == GBUS || io_edifgbl.isarray) np = sch_buspinprim;
				else np = sch_wirepinprim;
				if (io_edifgbl.points == NOEPT) break;
				for (point = io_edifgbl.points; point->nextpt != NOEPT; point = point->nextpt)
				{
					if (io_edifgbl.geometry == GNET || io_edifgbl.geometry == GBUS)
					{
						/* create a pin to pin connection */
						if (fcnt == 0)
						{
							/* look for the first pin */
							fcnt = io_edfindport(io_edifgbl.current_cell, point->x,
								point->y, sch_wirearc, fnis, fpps);
							if (fcnt == 0)
							{
								/* create the "from" pin */
								fnis[0] = io_edifiplacepin(np, point->x + np->lowx, point->x + np->highx,
									point->y + np->lowy, point->y + np->highy,
									0, 0, io_edifgbl.current_cell);
								if (fnis[0] == NONODEINST) fcnt = 0; else
								{
									fpps[0] = (io_edifgbl.geometry == GBUS || io_edifgbl.isarray) ?
										default_busport : default_port;
									fcnt = 1;
								}
							}
						}
						/* now the second ... */
						tcnt = io_edfindport(io_edifgbl.current_cell, point->nextpt->x,
							point->nextpt->y, sch_wirearc, tnis, tpps);
						if (tcnt == 0)
						{
							/* create the "to" pin */
							tnis[0] = io_edifiplacepin(np, point->nextpt->x + np->lowx,
								point->nextpt->x + np->highx,
								point->nextpt->y + np->lowy,
								point->nextpt->y + np->highy,
								0, 0, io_edifgbl.current_cell);
							if (tnis[0] == NONODEINST) tcnt = 0; else
							{
								tpps[0] = (io_edifgbl.geometry == GBUS || io_edifgbl.isarray) ?
									default_busport : default_port;
								tcnt = 1;
							}
						}

						if (tcnt == 0 || fcnt == 0)
						{
							ttyputerr(_("error, line #%d: could not create path"), io_edifgbl.lineno);
							io_edifgbl.errors++;
						} else
						{
							/* connect it */
							for (count = 0; count < fcnt || count < tcnt; count++)
							{
								if (count < fcnt)
								{
									lastpin = fnis[count];
									lastport = fpps[count];

									/* check node for array variable */
									var = getvalkey((INTBIG)lastpin, VNODEINST, VSTRING, EDIF_array_key);
									if (var != NOVARIABLE) fbus = 1; else
										if (lastport->protoname[estrlen(lastport->protoname)-1] == ']') fbus = 1; else
											fbus = 0;
								}
								if (count < tcnt)
								{
									ni = tnis[count];
									pp = tpps[count];

									/* check node for array variable */
									var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, EDIF_array_key);
									if (var != NOVARIABLE) tbus = 1; else
										if (pp->protoname[estrlen(pp->protoname)-1] == ']') tbus = 1; else
											tbus = 0;
								}

								/* if bus to bus */
								if ((lastport == default_busport || fbus) &&
									(pp == default_busport || tbus)) ap = sch_busarc;
										/* wire to other */
								else ap = sch_wirearc;

								ai = newarcinst(ap, defaultarcwidth(ap), FIXANG|CANTSLIDE,
									lastpin, lastport, point->x, point->y,
									ni, pp, point->nextpt->x, point->nextpt->y,
									io_edifgbl.current_cell);
								if (ai == NOARCINST)
								{
									ttyputerr(_("error, line #%d: could not create path (arc)"),
										io_edifgbl.lineno);
									io_edifgbl.errors++;
								} else if (io_edifgbl.geometry == GNET && io_edifgbl.points == point)
								{
									if (io_edifgbl.net_reference)
									{
										var = setvalkey((INTBIG)ai, VARCINST, EDIF_name_key,
											(INTBIG)io_edifgbl.net_reference, VSTRING);
									}
									if (io_edifgbl.net_name)
									{
										/* set name of arc but don't display name */
										var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key,
											(INTBIG)io_edifgbl.net_name, VSTRING);
										if (var != NOVARIABLE)
											defaulttextsize(4, var->textdescript);
									}
								} else if (io_edifgbl.geometry == GBUS && io_edifgbl.points == point)
								{
									if (io_edifgbl.bundle_reference)
									{
										var = setvalkey((INTBIG)ai, VARCINST, EDIF_name_key,
											(INTBIG)io_edifgbl.bundle_reference, VSTRING);
									}
									if (io_edifgbl.bundle_name)
									{
										/* set bus' EDIF name but don't display name */
										var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key,
											(INTBIG)io_edifgbl.bundle_name, VSTRING);
										if (var != NOVARIABLE)
											defaulttextsize(4, var->textdescript);
									}
								}
							}
							if (ai != NOARCINST) io_edifgbl.current_arc = ai;
							for (count = 0; count < tcnt; count++)
							{
								fnis[count] = tnis[count];
								fpps[count] = tpps[count];
							}
							fcnt = tcnt;
						}
					} else
					{
						/* rectalinear paths with some width */
						/* create a path from here to there (orthogonal only now) */
						lx = hx = point->x;
						ly = hy = point->y;
						if (lx > point->nextpt->x) lx = point->nextpt->x;
						if (hx < point->nextpt->x) hx = point->nextpt->x;
						if (ly > point->nextpt->y) ly = point->nextpt->y;
						if (hy < point->nextpt->y) hy = point->nextpt->y;
						if (ly == hy || io_edifgbl.extend_end)
						{
							ly -= io_edifgbl.path_width/2;
							hy += io_edifgbl.path_width/2;
						}
						if (lx == hx || io_edifgbl.extend_end)
						{
							lx -= io_edifgbl.path_width/2;
							hx += io_edifgbl.path_width/2;
						}
						ni = io_edifiplacepin(io_edifgbl.figure_group, lx, hx, ly, hy,
							io_edgettrans(io_edifgbl.orientation),
							io_edgetrot(io_edifgbl.orientation),
							io_edifgbl.current_cell);
						if (ni == NONODEINST)
						{
							ttyputerr(_("error, line #%d: could not create path"),
								io_edifgbl.lineno);
							io_edifgbl.errors++;
						}
					}
				}
				io_edfreeptlist();
				break;
			case KCIRCLE:
				if (io_edifgbl.pt_count == 2)
				{
					lx = mini(io_edifgbl.points->x, io_edifgbl.lastpt->x);
					hx = maxi(io_edifgbl.points->x, io_edifgbl.lastpt->x);
					ly = mini(io_edifgbl.points->y, io_edifgbl.lastpt->y);
					hy = maxi(io_edifgbl.points->y, io_edifgbl.lastpt->y);
					if (lx == hx)
					{
						lx -= (hy - ly)>>1;
						hx += (hy - ly)>>1;
					} else
					{
						ly -= (hx - lx)>>1;
						hy += (hx - lx)>>1;
					}

					/* create the node instance */
					ni = newnodeinst(art_circleprim, lx, hx, ly, hy,
						io_edgettrans(io_edifgbl.orientation),
						io_edgetrot(io_edifgbl.orientation),
						io_edifgbl.current_cell);
					if (ni == NONODEINST)
					{
						ttyputerr(_("error, line #%d: could not create circle"),
							io_edifgbl.lineno);
						io_edifgbl.errors++;
					}
				}
				io_edfreeptlist();
				break;
			case KNAME:
				/* save the data and break */
				io_edfreesavedptlist();
				(void)estrcpy(io_edifgbl.save_text.string, io_edifgbl.string);
				io_edifgbl.string[0] = 0;
				io_edifgbl.save_text.points = io_edifgbl.points;
				io_edifgbl.points = NOEPT;
				io_edifgbl.save_text.pt_count = io_edifgbl.pt_count;
				io_edifgbl.pt_count = 0;
				io_edifgbl.save_text.textheight = io_edifgbl.textheight;
				io_edifgbl.textheight = 0;
				io_edifgbl.save_text.justification = io_edifgbl.justification;
				io_edifgbl.justification = LOWERLEFT;
				io_edifgbl.save_text.orientation = io_edifgbl.orientation;
				io_edifgbl.orientation = OR0;
				io_edifgbl.save_text.visible = io_edifgbl.visible;
				io_edifgbl.visible = 1;
				break;
			case KSTRINGDISPLAY:
				if (kstack_ptr <= 1)
				{
					ttyputerr(_("error, line #%d: bad location for \"stringDisplay\""),
						io_edifgbl.lineno);
					io_edifgbl.errors++;
				}
				else if (kstack[kstack_ptr-1] == KRENAME)
				{
					/* save the data and break */
					io_edfreesavedptlist();
					(void)estrcpy(io_edifgbl.save_text.string, io_edifgbl.string);
					io_edifgbl.string[0] = 0;
					io_edifgbl.save_text.points = io_edifgbl.points;
					io_edifgbl.points = NOEPT;
					io_edifgbl.save_text.pt_count = io_edifgbl.pt_count;
					io_edifgbl.pt_count = 0;
					io_edifgbl.save_text.textheight = io_edifgbl.textheight;
					io_edifgbl.textheight = 0;
					io_edifgbl.save_text.justification = io_edifgbl.justification;
					io_edifgbl.justification = LOWERLEFT;
					io_edifgbl.save_text.orientation = io_edifgbl.orientation;
					io_edifgbl.orientation = OR0;
					io_edifgbl.save_text.visible = io_edifgbl.visible;
					io_edifgbl.visible = 1;
				}

				/* output the data for annotate (display graphics) and string (on properties) */
				else if (kstack[kstack_ptr-1] == KANNOTATE || kstack[kstack_ptr-1] == KSTRING)
				{
#ifdef IGNORESTRINGSWITHSQUAREBRACKETS
					/* supress this if it starts with "[" */
					if (io_edifgbl.string[0] != '[' && io_edifgbl.points != 0)
#endif
					{
						/* see if a pre-existing node or arc exists to add text */
						ai = NOARCINST;
						ni = NONODEINST;
						if (io_edifgbl.property_reference[0] != 0 && io_edifgbl.current_node != NONODEINST)
						{
							ni = io_edifgbl.current_node;
							key = makekey(io_edifgbl.property_reference);
							xoff = io_edifgbl.points->x - ((ni->highx+ni->lowx)>>1);
							yoff = io_edifgbl.points->y - ((ni->highy+ni->lowy)>>1);
						}
						else if (io_edifgbl.property_reference[0] != 0 && io_edifgbl.current_arc != NOARCINST)
						{
							ai = io_edifgbl.current_arc;
							key = makekey(io_edifgbl.property_reference);
							xoff = io_edifgbl.points->x - ((ai->end[0].xpos + ai->end[1].xpos)>>1);
							yoff = io_edifgbl.points->y - ((ai->end[0].ypos + ai->end[1].ypos)>>1);
						} else
						{
							/* create the node instance */
							xoff = yoff = 0;
							ni = newnodeinst(gen_invispinprim,
								io_edifgbl.points->x, io_edifgbl.points->x,
								io_edifgbl.points->y, io_edifgbl.points->y, 0, 0,
								io_edifgbl.current_cell);
							key = EDIF_annotate_key;
						}
						if (ni != NONODEINST || ai != NOARCINST)
						{
							if (ni != NONODEINST)
							{
								var = setvalkey((INTBIG)ni, VNODEINST, key, (INTBIG)io_edifgbl.string,
									(io_edifgbl.visible ? VSTRING|VDISPLAY : VSTRING));

								/* now set the position, relative to the center of the current object */
								xoff = io_edifgbl.points->x - ((ni->highx+ni->lowx)>>1);
								yoff = io_edifgbl.points->y - ((ni->highy+ni->lowy)>>1);
							} else
							{
								var = setvalkey((INTBIG)ai, VARCINST, key, (INTBIG)io_edifgbl.string,
									(io_edifgbl.visible ? VSTRING|VDISPLAY : VSTRING));

								/* now set the position, relative to the center of the current object */
								xoff = io_edifgbl.points->x - ((ai->end[0].xpos + ai->end[1].xpos)>>1);
								yoff = io_edifgbl.points->y - ((ai->end[0].ypos + ai->end[1].ypos)>>1);
							}

							/* convert to quarter lambda units */
							xoff = 4*xoff / lambda;
							yoff = 4*yoff / lambda;

							/* determine the size of text, 0.0278 in == 2 points or 36 (2xpixels) == 1 in
		 					fonts range from 4 to 31 */
							if (io_edifgbl.textheight == 0) i = TXTSETQLAMBDA(4); else
							{
								i = io_ediftextsize(io_edifgbl.textheight);
							}
							TDCOPY(descript, var->textdescript);
							TDSETSIZE(descript, i);
							TDSETOFF(descript, xoff, yoff);
							TDSETPOS(descript, VTPOSCENT);
							switch (io_edifgbl.justification)
							{
								case UPPERLEFT:    TDSETPOS(descript, VTPOSUPRIGHT);    break;
								case UPPERCENTER:  TDSETPOS(descript, VTPOSUP);         break;
								case UPPERRIGHT:   TDSETPOS(descript, VTPOSUPLEFT);     break;
								case CENTERLEFT:   TDSETPOS(descript, VTPOSRIGHT);      break;
								case CENTERCENTER: TDSETPOS(descript, VTPOSCENT);       break;
								case CENTERRIGHT:  TDSETPOS(descript, VTPOSLEFT);       break;
								case LOWERLEFT:    TDSETPOS(descript, VTPOSDOWNRIGHT);  break;
								case LOWERCENTER:  TDSETPOS(descript, VTPOSDOWN);       break;
								case LOWERRIGHT:   TDSETPOS(descript, VTPOSDOWNLEFT);   break;
							}
							TDCOPY(var->textdescript, descript);
						} else
						{
							ttyputerr(_("error, line #%d: nothing to attach text to"), io_edifgbl.lineno);
							io_edifgbl.errors++;
						}
					}
				}

				/* clean up DISPLAY attributes */
				io_edfreeptlist();
				io_edifgbl.cur_nametbl = NONAMETABLE;
				io_edifgbl.visible = 1;
				io_edifgbl.justification = LOWERLEFT;
				io_edifgbl.textheight = 0;
				break;
			case KDOT:
				if (io_edifgbl.geometry == GPIN)
				{
					for (eport = io_edifgbl.ports; eport != NOEDPORT; eport = eport->next)
					{
						if (!namesame(eport->reference, io_edifgbl.name)) break;
					}
					if (eport != NOEDPORT)
					{
						/* create a internal wire port using the pin-proto, and create the port
	  					and export */
						ni = io_edifiplacepin(io_edifgbl.proto,
							io_edifgbl.points->x+io_edifgbl.proto->lowx,
							io_edifgbl.points->x+io_edifgbl.proto->highx,
							io_edifgbl.points->y+io_edifgbl.proto->lowy,
							io_edifgbl.points->y+io_edifgbl.proto->highy,
							io_edgettrans(io_edifgbl.orientation),
							io_edgetrot(io_edifgbl.orientation),
							io_edifgbl.current_cell);
						if (ni == NONODEINST)
						{
							ttyputerr(_("error, line #%d: could not create pin"), io_edifgbl.lineno);
							io_edifgbl.errors++;
						}
						(void)estrcpy(portname, eport->name);
						for (dup = 0, (void)estrcpy(basename, portname);
							(ppt = getportproto(ni->parent, portname)) != NOPORTPROTO; dup++)
						{
							if (dup == 0) pp = ppt;
							(void)esnprintf(portname, WORD+1, x_("%s_%ld"), basename, dup+1);
						}
						ppt = newportproto(ni->parent, ni, default_iconport, portname);
						if (ppt == NOPORTPROTO)
						{
							ttyputerr(_("error, line #%d: could not create port <%s>"),
								io_edifgbl.lineno, portname);
							io_edifgbl.errors++;
						} else
						{
							/* set the direction */
							switch (eport->direction)
							{
								case INPUTE:
									ppt->userbits = (ppt->userbits & ~STATEBITS) | INPORT;
									break;
								case OUTPUTE:
									ppt->userbits = (ppt->userbits & ~STATEBITS) | OUTPORT;
									break;
								case INOUT:
									ppt->userbits = (ppt->userbits & ~STATEBITS) | BIDIRPORT;
									break;
							}
						}
					}
				} else
				{
					/* create the node instance */
					ni = newnodeinst(io_edifgbl.figure_group != NONODEPROTO ?
						io_edifgbl.figure_group : art_boxprim,
						io_edifgbl.points->x, io_edifgbl.points->x,
						io_edifgbl.points->y, io_edifgbl.points->y,
						io_edgettrans(io_edifgbl.orientation),
						io_edgetrot(io_edifgbl.orientation),
						io_edifgbl.current_cell);
					if (ni == NONODEINST)
					{
						ttyputerr(_("error, line #%d: could not create rectangle"),
							io_edifgbl.lineno);
						io_edifgbl.errors++;
					}
				}
				io_edfreeptlist();
				break;
			case KRECTANGLE:
				if (kstack_ptr > 1 && (kstack[kstack_ptr-1] == KPAGESIZE ||
					kstack[kstack_ptr-1] == KBOUNDINGBOX)) break;
				if (io_edifgbl.pt_count == 2)
				{
					/* create the node instance */
					if (io_edifgbl.points->x > io_edifgbl.lastpt->x)
					{
						lx = io_edifgbl.lastpt->x;
						hx = io_edifgbl.points->x;
					} else
					{
						hx = io_edifgbl.lastpt->x;
						lx = io_edifgbl.points->x;
					}
					if (io_edifgbl.points->y > io_edifgbl.lastpt->y)
					{
						ly = io_edifgbl.lastpt->y;
						hy = io_edifgbl.points->y;
					} else
					{
						hy = io_edifgbl.lastpt->y;
						ly = io_edifgbl.points->y;
					}
					ni = newnodeinst(io_edifgbl.figure_group != NONODEPROTO ?
						io_edifgbl.figure_group : art_boxprim,
						lx, hx, ly, hy,
						io_edgettrans(io_edifgbl.orientation),
						io_edgetrot(io_edifgbl.orientation),
						io_edifgbl.current_cell);
					if (ni == NONODEINST)
					{
						ttyputerr(_("error, line #%d: could not create rectangle"),
							io_edifgbl.lineno);
						io_edifgbl.errors++;
					}
					else if (io_edifgbl.figure_group == art_openeddottedpolygonprim)
					{
						cnt = 5;
						cx = (io_edifgbl.points->x + io_edifgbl.lastpt->x) / 2;
						cy = (io_edifgbl.points->y + io_edifgbl.lastpt->y) / 2;
						pts[0] = io_edifgbl.points->x-cx;
						pts[1] = io_edifgbl.points->y-cy;
						pts[2] = io_edifgbl.points->x-cx;
						pts[3] = io_edifgbl.lastpt->y-cy;
						pts[4] = io_edifgbl.lastpt->x-cx;
						pts[5] = io_edifgbl.lastpt->y-cy;
						pts[6] = io_edifgbl.lastpt->x-cx;
						pts[7] = io_edifgbl.points->y-cy;
						pts[8] = io_edifgbl.points->x-cx;
						pts[9] = io_edifgbl.points->y-cy;

						/* store the trace information */
						(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)pts,
							VINTEGER|VISARRAY|((cnt*2)<<VLENGTHSH));
					}
					else if (io_edifgbl.geometry == GPIN)
					{
						/* create a rectangle using the pin-proto, and create the port
	   					 * and export
	   					 */
						/* ensure full sized port */
						lx += io_edifgbl.proto->lowx;
						hx += io_edifgbl.proto->highx;
						ly += io_edifgbl.proto->lowy;
						hy += io_edifgbl.proto->highy;

						/* create the node instance */
						ni = io_edifiplacepin(io_edifgbl.proto, lx, hx, ly, hy,
							io_edgettrans(io_edifgbl.orientation),
							io_edgetrot(io_edifgbl.orientation),
							io_edifgbl.current_cell);
						if (ni == NONODEINST)
						{
							ttyputerr(_("error, line #%d: could not create pin"),
								io_edifgbl.lineno);
							io_edifgbl.errors++;
						}
						ppt = getportproto(ni->parent, io_edifgbl.name);
						if (ppt == NOPORTPROTO)
							ppt = newportproto(ni->parent, ni, default_iconport, io_edifgbl.name);
						if (ppt == NOPORTPROTO)
						{
							ttyputerr(_("error, line #%d: could not create port <%s>"),
								io_edifgbl.lineno, io_edifgbl.name);
							io_edifgbl.errors++;
						}
					}
				}
				io_edfreeptlist();

				break;

			case KARC:
				/* set array values */
				for (point = io_edifgbl.points, i = 0; point && i < 3; point = point->nextpt,i++)
				{
					x[i] = point->x;
					y[i] = point->y;
				}
				io_ededif_arc(x, y, &cx, &cy, &radius, &so, &ar, &j, &trans);
				lx = cx - radius;   hx = cx + radius;
				ly = cy - radius;   hy = cy + radius;

				/* get the bounds of the circle */
				ni = newnodeinst(io_edifgbl.figure_group != NONODEPROTO &&
					io_edifgbl.figure_group != art_boxprim ?
					io_edifgbl.figure_group : art_circleprim, lx, hx, ly, hy,
					trans, j, io_edifgbl.current_cell);
				if (ni == NONODEINST)
				{
					ttyputerr(_("error, line #%d: could not create arc"), io_edifgbl.lineno);
					io_edifgbl.errors++;
				} else
				{
					/* store the angle of the arc */
					setarcdegrees(ni, so, ar*EPI/1800.0);
				}
				io_edfreeptlist();
				break;

			case KSYMBOL:
			case KPAGE:
				np = io_edifgbl.current_cell;
				if (np != NONODEPROTO)
				{
					/* now compute the bounds of this cell */
					db_boundcell(np, &np->lowx, &np->highx, &np->lowy, &np->highy);
				}
				io_edifgbl.active_view = VNULL;
				break;

			case KOPENSHAPE:
			case KPOLYGON:
dopoly:
				if (io_edifgbl.pt_count == 0) break;

				/* get the bounds of the poly */
				lx = hx = io_edifgbl.points->x;
				ly = hy = io_edifgbl.points->y;
				point = io_edifgbl.points->nextpt;
				while (point)
				{
					if (lx > point->x) lx = point->x;
					if (hx < point->x) hx = point->x;
					if (ly > point->y) ly = point->y;
					if (hy < point->y) hy = point->y;
					point = point->nextpt;
				}
				if (lx != hx || ly != hy)
				{
					if (io_edifgbl.figure_group != NONODEPROTO && io_edifgbl.figure_group != art_boxprim)
						np = io_edifgbl.figure_group; else
					{
						if (io_edifgbl.state == KPOLYGON) np = art_closedpolygonprim; else
							np = art_openedpolygonprim;
					}
					ni = newnodeinst(np, lx, hx, ly, hy, io_edgettrans(io_edifgbl.orientation),
						io_edgetrot(io_edifgbl.orientation), io_edifgbl.current_cell);
					if (ni == NONODEINST)
					{
						ttyputerr(_("error, line #%d: could not create polygon"), io_edifgbl.lineno);
						io_edifgbl.errors++;
					} else
					{
						pt = trace = emalloc((io_edifgbl.pt_count*2*SIZEOFINTBIG), el_tempcluster);
						if (trace == 0) RET_NOMEMORY();
						cx = (hx + lx) / 2;
						cy = (hy + ly) / 2;
						point = io_edifgbl.points;
						while (point != NOEPT)
						{
							*pt++ = point->x - cx;
							*pt++ = point->y - cy;
							point = point->nextpt;
						}

						/* store the trace information */
						(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)trace,
							VINTEGER|VISARRAY|((io_edifgbl.pt_count*2)<<VLENGTHSH));

						/* free the polygon memory */
						efree((CHAR *)trace);
					}
				}
				io_edfreeptlist();
				break;
			case KARRAY:
				switch (kstack[kstack_ptr-1])
				{
					case KPORT:
						io_edmake_electric_name(io_edifgbl.port_reference, io_edifgbl.port_name, io_edifgbl.arrayx,
							io_edifgbl.arrayy);
					default:
						break;
				}
				if (io_edifgbl.arrayx == 0) io_edifgbl.arrayx = 1;
				if (io_edifgbl.arrayy == 0) io_edifgbl.arrayy = 1;
				break;
			case KMEMBER:
				if (io_edifgbl.memberx != -1)
				{
					/* adjust the name of the current INSTANCE/NET/PORT(/VALUE) */
					if (io_edifgbl.membery != -1)
						(void)esnprintf(basename, WORD+1, x_("[%d,%d]"), io_edifgbl.memberx, io_edifgbl.membery);
					else (void)esnprintf(basename, WORD+1, x_("[%d]"), io_edifgbl.memberx);
					switch (kstack[kstack_ptr-1])
					{
						case KINSTANCEREF:
							(void)estrcat(io_edifgbl.instance_reference, basename);
							break;
						case KPORTREF:
							(void)estrcat(io_edifgbl.port_reference, basename);
							break;
						default:
							break;
					}
				}
				break;
			case KDESIGN:
				if (io_edifgbl.proto != NONODEPROTO)
				{
					if (io_verbose < 0)
					{
						infstr = initinfstr();
						formatinfstr(infstr, _("Design %s"),
							io_edifgbl.proto->protoname);
						DiaSetTextProgress(io_edifprogressdialog, returninfstr(infstr));
					}
					else ttyputmsg(_("Design %s"), io_edifgbl.proto->protoname);
					io_edifgbl.library->curnodeproto = io_edifgbl.proto;
				}
				break;

			case KEDIF:
				/* free the port freelist */
				for (eport = io_edifgbl.free_ports; eport != NOEDPORT; eport = neport)
				{
					neport = eport->next;
					efree((CHAR *)eport);
				}
				io_edifgbl.free_ports = NOEDPORT;

				/* free the netport list */
				io_edfreenetports();
				break;
			default:
				break;
			}
		}
		if (kstack_ptr) io_edifgbl.state = kstack[--kstack_ptr]; else
			io_edifgbl.state = KINIT;
		return(0);
	}
	else return(1);
}

/* module: io_edprocess_integer
   function: will do cleanup processing of an integer argument such as in (array (...) 1 2)
 */
void io_edprocess_integer(INTBIG value)
{
	if (kstack_ptr)
	{
		if (!io_edifgbl.ignoreblock)
		{
			switch (io_edifgbl.state)
			{
				case KARRAY:
					if (io_edifgbl.arrayx == 0) io_edifgbl.arrayx = value;
						else if (io_edifgbl.arrayy == 0) io_edifgbl.arrayy = value;
					break;
				case KMEMBER:
					if (io_edifgbl.memberx == -1) io_edifgbl.memberx = value;
						else if (io_edifgbl.membery == -1) io_edifgbl.membery = value;
					break;
				default:
					break;
			}
		}
	}
}

INTSML io_edgetrot(OTYPES orientation)
{
	switch (orientation)
	{
		case OR0:     return(0);
		case OR90:    return(90*10);
		case OR180:   return(180*10);
		case OR270:   return(270*10);
		case OMX:     return(270*10);
		case OMY:     return(90*10);
		case OMXR90:  return(180*10);
		case OMYR90:  return(0);
		default:      break;
	}
	return(0);
}

INTSML io_edgettrans(OTYPES orientation)
{
	switch (orientation)
	{
		case OR0:     return(0);
		case OR90:    return(0);
		case OR180:   return(0);
		case OR270:   return(0);
		case OMY:     return(1);
		case OMX:     return(1);
		case OMYR90:  return(1);
		case OMXR90:  return(1);
		default:      break;
	}
	return(0);
}

/*
 * Routine to return the text height to use when it says "textheight" units in the EDIF file.
 */
INTBIG io_ediftextsize(INTBIG textheight)
{
	REGISTER INTBIG i, lambda;

	lambda = el_curlib->lambda[io_edifgbl.technology->techindex];
	i = io_edifgbl.textheight / lambda;
	if (i < 0) i = 4;
	if (i > TXTMAXQLAMBDA) i = TXTMAXQLAMBDA;
	i = TXTSETQLAMBDA(i);
	return(i);
}

/* helper routine for "esort" */
int io_edcompare_name(const void *name1, const void *name2)
{
	REGISTER NAMETABLE_PTR *n1, *n2;

	n1 = (NAMETABLE_PTR *)name1;
	n2 = (NAMETABLE_PTR *)name2;
	return(namesame((*n1)->original, (*n2)->original));
}

/* Will check if a string is an valid integer */
INTBIG io_edis_integer(CHAR *buffer)
{
	/* remove sign */
	if (*buffer == '+' || *buffer == '-') buffer++;
	while (isdigit(*buffer))
	{
		buffer++;
		if (*buffer == 0) return(1);
	}
	return(0);
}

/* get a keyword routine */
CHAR *io_edget_keyword(CHAR *buffer)
{
	CHAR *p;

	/* look for a '(' before the edif keyword */
	while ((p = io_edget_token(buffer, 0)) && (*p != '('))
	{
		if (*p == ')') io_edpop_stack(); else
			if (io_edis_integer(buffer)) io_edprocess_integer(eatoi(buffer));
	}
	return(io_edget_token(buffer, 0));
}

/* get a token routine */
CHAR *io_edget_token(CHAR *buffer, CHAR idelim)
{
	CHAR *sptr;
	CHAR delim;

	/* locate the first non-white space character for non-delimited searches */
	delim = idelim;
	if (!delim)
	{
		if (io_edpos_token() == NULL)
			return(NULL);
	}

	/* set up the string copy */
	sptr = buffer;
	if (!delim && *io_edifgbl.pos == '"')
	{
		/* string search */
		delim = '"';
		*(sptr++) = *(io_edifgbl.pos++);
	}

	/* non locate the next white space or the delimiter */
	for(;;)
	{
		if (*io_edifgbl.pos == 0)
		{
			/* end of string, get the next line */
			if (!io_edget_line(io_edifgbl.buffer, LINE, io_edifgbl.edif_file))
			{
				/* end-of-file, if no delimiter return NULL, otherwise the string */
				if (delim) break;
				*sptr = 0;
				return(buffer);
			}
			io_edifgbl.pos = io_edifgbl.buffer;
		} else if (!delim)
		{
			switch (*io_edifgbl.pos)
			{
				/* the space characters */
				case '\n':
				case '\r':
					io_edifgbl.lineno++;
					/* FALLTHROUGH */ 
				case ' ':
				case '\t':
					*sptr = 0;
					/* skip to the next character */
					io_edifgbl.pos++;
					return(buffer);
					/* special EDIF delimiters */
				case '(':
				case ')':
					if (sptr == buffer)
						*(sptr++) = *(io_edifgbl.pos++);
					*sptr = 0;
					return(buffer);
				default:
					*(sptr++) = *(io_edifgbl.pos++);
					break;
			}
		} else
		{
			/* check for user specified delimiter */
			if (*io_edifgbl.pos == delim)
			{
				/* skip the delimiter, unless string */
				if (delim != idelim) *(sptr++) = *(io_edifgbl.pos++); else
					io_edifgbl.pos++;
				*sptr = 0;
				return(buffer);
			} else
			{
				if (*io_edifgbl.pos == '\r' || *io_edifgbl.pos == '\n')
				{
					io_edifgbl.lineno++;
				}

				*(sptr++) = *(io_edifgbl.pos++);
			}
		}
	}
	return(NULL);
}

/*
 * Module: io_edpos_token
 * Function: special routine to position to the next token
 */
CHAR *io_edpos_token(void)
{
	for(;;)
	{
		switch (*io_edifgbl.pos)
		{
			case 0:
				/* end of string, get the next line */
				if (!io_edget_line(io_edifgbl.buffer, LINE, io_edifgbl.edif_file))
				{
					/* end-of-file */
					return(NULL);
				}
				io_edifgbl.pos = io_edifgbl.buffer;
				break;

				/* the space characters */
			case '\n':
			case '\r':
				io_edifgbl.lineno++;
				/* FALLTHROUGH */ 
			case ' ':
			case '\t':
				io_edifgbl.pos++;
				break;
			default:
				return(io_edifgbl.pos);
		}
	}
}

/*
 * routine to read a line from file "file" and place it in "line" (which has
 * up to "limit" characters).  Returns FALSE on end-of-file.  Updates "filepos".
 */
BOOLEAN io_edget_line(CHAR *line, INTBIG limit, FILE *file)
{
	REGISTER CHAR *pp;
	REGISTER INTSML c;
	REGISTER INTBIG total;

	pp = line;
	total = 1;
	for(;;)
	{
		c = xgetc(file);
		if (c == EOF)
		{
			if (pp == line) return(FALSE);
			break;
		}
		filepos++;
		*pp = (CHAR)c;
		if (*pp == '\n')
		{
			pp++;
			break;
		}
		pp++;
		if ((++total) >= limit) break;
	}
	*pp = 0;
	return(TRUE);
}

/* get a delimeter routine */
void io_edget_delim(CHAR delim)
{
	CHAR *token;

	token = io_edpos_token();
	if (token == NULL) longjmp(io_edifgbl.env, LONGJMPEOF);
	if (*token != delim) longjmp(io_edifgbl.env, LONGJMPILLDELIM); else
		io_edifgbl.pos++;
}

/************ INPUT SUPPORT *********/

/* This function allocates ports */
INTBIG io_edallocport(void)
{
	EDPORT_PTR port;

	if (io_edifgbl.free_ports != NOEDPORT)
	{
		port = io_edifgbl.free_ports;
		io_edifgbl.free_ports = port->next;
	} else
	{
		port = (EDPORT_PTR)emalloc(sizeof (EDPORT), io_tool->cluster);
		if (port == NOEDPORT) RET_NOMEMORY();
	}

	port->next = io_edifgbl.ports;
	io_edifgbl.ports = port;
	if (allocstring(&port->reference, io_edifgbl.port_reference, io_tool->cluster))
		RET_NOMEMORY();
	if (allocstring(&port->name, io_edifgbl.port_name, io_tool->cluster))
		RET_NOMEMORY();

	port->direction = io_edifgbl.direction;
	port->arrayx = io_edifgbl.arrayx;
	port->arrayy = io_edifgbl.arrayy;
	return(0);
}

INTBIG io_edallocproperty(CHAR *name, PROPERTY_TYPE type, int integer,
	float number, CHAR *string)
{
	EDPROPERTY_PTR property;
	REGISTER void *infstr;

	if (io_edifgbl.free_properties != NOEDPROPERTY)
	{
		property = io_edifgbl.free_properties;
		io_edifgbl.free_properties = property->next;
	} else
	{
		property = (EDPROPERTY_PTR)emalloc(sizeof (EDPROPERTY), io_tool->cluster);
		if (property == NOEDPROPERTY)
			RET_NOMEMORY();
	}

	property->next = io_edifgbl.properties;
	io_edifgbl.properties = property;
	infstr = initinfstr();
	formatinfstr(infstr, x_("ATTR_%s"), name);
	if (allocstring(&property->name, returninfstr(infstr), io_tool->cluster))
		RET_NOMEMORY();
	property->type = type;
	switch (property->type)
	{
		case PINTEGER:
			property->val.integer = integer;
			break;
		case PNUMBER:
			property->val.number = number;
			break;
		case PSTRING:
			if (allocstring(&property->val.string, string, io_tool->cluster))
				RET_NOMEMORY();
			break;
		default:
			break;
	}
	return(0);
}

/* This function allocates net ports */
INTBIG io_edallocnetport(void)
{
	EDNETPORT_PTR netport;

	if (io_edifgbl.free_netports != NOEDNETPORT)
	{
		netport = io_edifgbl.free_netports;
		io_edifgbl.free_netports = netport->next;
	} else
	{
		netport = (EDNETPORT_PTR)emalloc(sizeof (EDNETPORT), io_tool->cluster);
		if (netport == NOEDNETPORT) RET_NOMEMORY();
	}

	if (io_edifgbl.firstnetport == NOEDNETPORT)io_edifgbl.firstnetport = netport; else
		io_edifgbl.lastnetport->next = netport;

	io_edifgbl.lastnetport = netport;
	netport->next = NOEDNETPORT;
	netport->ni = NONODEINST;
	netport->pp = NOPORTPROTO;
	netport->member = 0;
	return(0);
}

/* free the netport list */
void io_edfreenetports(void)
{
	EDNETPORT_PTR nport;

	while (io_edifgbl.firstnetport != NOEDNETPORT)
	{
		nport = io_edifgbl.firstnetport;
		io_edifgbl.firstnetport = nport->next;
		nport->next = io_edifgbl.free_netports;
		io_edifgbl.free_netports = nport;
	}
	io_edifgbl.firstnetport = io_edifgbl.lastnetport = NOEDNETPORT;
}

/*
 * Module: io_edeq_of_a_line
 * Function: Calculates the equation of a line (Ax + By + C = 0)
 * Inputs:  sx, sy  - Start point
 *          ex, ey  - End point
 *          px, py  - Point line should pass through
 * Outputs: A, B, C - constants for the line
 */
void io_edeq_of_a_line(double sx, double sy, double ex, double ey, double *A, double *B, double *C)
{
	if (sx == ex)
	{
		*A = 1.0;
		*B = 0.0;
		*C = -ex;
	} else if (sy == ey)
	{
		*A = 0.0;
		*B = 1.0;
		*C = -ey;
	} else
	{
		/* let B = 1 then */
		*B = 1.0;
		if (sx != 0.0)
		{
			/* Ax1 + y1 + C = 0    =>    A = -(C + y1) / x1 */
			/* C = -Ax2 - y2       =>    C = (Cx2 + y1x2) / x1 - y2   =>   Cx1 - Cx2 = y1x2 - y2x1  */
			/* C = (y2x1 - y1x2) / (x2 - x1) */
			*C = (ey * sx - sy * ex) / (ex - sx);
			*A = -(*C + sy) / sx;
		} else
		{
			*C = (sy * ex - ey * sx) / (sx - ex);
			*A = -(*C + ey) / ex;
		}
	}
}

/*
 * Module: io_eddetermine_intersection
 * Function: Will determine the intersection of two lines
 * Inputs: Ax + By + C = 0 form
 * Outputs: x, y - the point of intersection
 * returns 1 if found, 0 if non-intersecting
 */
INTBIG io_eddetermine_intersection(double A[2], double B[2], double C[2], double *x, double *y)
{
	double A1B2, A2B1;
	double A2C1, A1C2;
	double X, Y;

	/* check for parallel lines */
	if ((A1B2 = A[0] * B[1]) == (A2B1 = A[1] * B[0]))
	{
		/* check for coincident lines */
		if (C[0] == C[1]) return(-1);
		return(0);
	}
	A1C2 = A[0] * C[1];
	A2C1 = A[1] * C[0];

	if (A[0])
	{
		Y = (A2C1 - A1C2) / (A1B2 - A2B1);
		X = -(B[0] * Y + C[0]) / A[0];
	} else
	{
		Y = (A1C2 - A2C1) / (A2B1 - A1B2);
		X = -(B[1] * Y + C[1]) / A[1];
	}
	*y = Y;
	*x = X;
	return(1);
}

void io_ededif_arc(INTBIG ix[3], INTBIG iy[3], INTBIG *ixc, INTBIG *iyc, INTBIG *r, double *so,
	double *ar, INTBIG *rot, INTBIG *trans)
{
	double x[3], y[3], px[3], py[3], a[3];
	double A[2], B[2], C[2];
	double dx, dy, R, area, xc, yc;
	INTBIG i;

	for(i=0; i<3; i++)
	{
		x[i] = (double)ix[i];
		y[i] = (double)iy[i];
	}

	/* get line equations of perpendicular bi-sectors of p1 to p2 */
	px[1] = (x[0] + x[1]) / 2.0;
	py[1] = (y[0] + y[1]) / 2.0;

	/* now rotate end point 90 degrees */
	px[0] = px[1] - (y[0] - py[1]);
	py[0] = py[1] + (x[0] - px[1]);
	io_edeq_of_a_line(px[0], py[0], px[1], py[1], &A[0], &B[0], &C[0]);

	/* get line equations of perpendicular bi-sectors of p2 to p3 */
	px[1] = (x[2] + x[1]) / 2.0;
	py[1] = (y[2] + y[1]) / 2.0;

	/* now rotate end point 90 degrees */
	px[2] = px[1] - (y[2] - py[1]);
	py[2] = py[1] + (x[2] - px[1]);
	io_edeq_of_a_line(px[1], py[1], px[2], py[2], &A[1], &B[1], &C[1]);

	/* determine the point of intersection */
	(void)io_eddetermine_intersection(A, B, C, &xc, &yc);

	*ixc = rounddouble(xc);
	*iyc = rounddouble(yc);

	dx = ((double)*ixc) - x[0];
	dy = ((double)*iyc) - y[0];
	R = sqrt(dx * dx + dy * dy);
	*r = rounddouble(R);

	/* now calculate the angle to the start and endpoint */
	dx = x[0] - xc;  dy = y[0] - yc;
	if (dx == 0.0 && dy == 0.0)
	{
		ttyputerr(_("Domain error doing arc computation"));
		return;
	}
	a[0] = atan2(dy, dx) * 1800.0 / EPI;
	if (a[0] < 0.0) a[0] += 3600.0;

	dx = x[2] - xc;  dy = y[2] - yc;
	if (dx == 0.0 && dy == 0.0)
	{
		ttyputerr(_("Domain error doing arc computation"));
		return;
	}
	a[2] = atan2(dy, dx) * 1800.0 / EPI;
	if (a[2] < 0.0) a[2] += 3600.0;

	/* determine the angle of rotation and object orientation */
	/* determine the direction */
	/* calculate x1*y2 + x2*y3 + x3*y1 - y1*x2 - y2*x3 - y3*x1 */
	area = x[0]*y[1] + x[1]*y[2] + x[2]*y[0] - y[0]*x[1] - y[1]*x[2] - y[2]*x[0];
	if (area > 0.0)
	{
		/* counter clockwise */
		if (a[2] < a[0]) a[2] += 3600.0;
		*ar = a[2] - a[0];
		*rot = rounddouble(a[0]);
		*so = (a[0] - (double)*rot) * EPI / 1800.0;
		*trans = 0;
	} else
	{
		/* clockwise */
		if (a[0] > 2700)
		{
			*rot = 3600 - rounddouble(a[0]) + 2700;
			*so = ((3600.0 - a[0] + 2700.0) - (double)*rot) * EPI / 1800.0;
		} else
		{
			*rot = 2700 - rounddouble(a[0]);
			*so = ((2700.0 - a[0]) - (double)*rot) * EPI / 1800.0;
		}
		if (a[0] < a[2]) a[0] += 3600;
		*ar = a[0] - a[2];
		*trans = 1;
	}
}

/*
 * routine to generate an icon in library "lib" with name "iconname" from the
 * port list in "fpp".  The icon cell is called "pt".  The icon cell is
 * returned (NONODEPROTO on error). (Note: copied and adapted from us_makeiconcell)
 */
NODEPROTO *io_edmakeiconcell(PORTPROTO *fpp, CHAR *iconname, CHAR *pt, LIBRARY *lib)
{
	REGISTER NODEPROTO *np, *bbproto, *pinproto, *buspproto, *pintype;
	REGISTER NODEINST *bbni, *pinni;
	REGISTER PORTPROTO *pp, *port, *inputport, *outputport, *bidirport,
		*topport, *whichport, *bpp;
	REGISTER ARCPROTO *wireproto, *busproto, *wiretype;
	REGISTER INTBIG inputside, outputside, bidirside, topside;
	REGISTER INTBIG eindex, xsize, ysize, xpos, ypos, xbbpos, ybbpos, spacing,
		lambda;
	REGISTER UINTBIG character;
	REGISTER VARIABLE *var;

	/* get the necessary symbols */
	pinproto = sch_wirepinprim;
	buspproto = sch_buspinprim;
	bbproto = sch_bboxprim;
	wireproto = sch_wirearc;
	busproto = sch_busarc;
	outputport = bbproto->firstportproto;
	topport = outputport->nextportproto;
	inputport = topport->nextportproto;
	bidirport = inputport->nextportproto;

	/* create the new icon cell */
	lambda = lib->lambda[sch_tech->techindex];
	np = us_newnodeproto(pt, lib);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot create icon %s"), pt);
		return(NONODEPROTO);
	}
	np->userbits |= WANTNEXPAND;

	/* determine number of inputs and outputs */
	inputside = outputside = bidirside = topside = 0;
	for(pp = fpp; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if ((pp->userbits&BODYONLY) != 0) continue;
		character = pp->userbits & STATEBITS;

		/* special detection for power and ground ports */
		if (portispower(pp) || portisground(pp)) character = GNDPORT;

		/* make a count of the types of ports and save it on the ports */
		switch (character)
		{
			case OUTPORT:
				pp->temp1 = outputside++;
				break;
			case BIDIRPORT:
				pp->temp1 = bidirside++;
				break;
			case PWRPORT:
			case GNDPORT:
				pp->temp1 = topside++;
				break;
			default:		/* INPORT, unlabeled, and all CLOCK ports */
				pp->temp1 = inputside++;
				break;
		}
	}

	/* create the Black Box with the correct size */
	ysize = maxi(maxi(inputside, outputside), 5) * 2 * lambda;
	xsize = maxi(maxi(topside, bidirside), 3) * 2 * lambda;

	/* create the Black Box instance */
	bbni = newnodeinst(bbproto, 0, xsize, 0, ysize, 0, 0, np);
	if (bbni == NONODEINST) return(NONODEPROTO);

	/* put the original cell name on the Black Box */
	var = setval((INTBIG)bbni, VNODEINST, x_("SCHEM_function"), (INTBIG)iconname, VSTRING|VDISPLAY);
	if (var != NOVARIABLE) defaulttextdescript(var->textdescript, bbni->geom);

	/* place pins around the Black Box */
	for(pp = fpp; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if ((pp->userbits&BODYONLY) != 0) continue;
		character = pp->userbits & STATEBITS;

		/* special detection for power and ground ports */
		if (portispower(pp) || portisground(pp)) character = GNDPORT;

		/* make a count of the types of ports and save it on the ports */
		switch (character)
		{
			case OUTPORT:
				xpos = xsize + 2 * lambda;
				xbbpos = xsize;
				eindex = pp->temp1;
				spacing = 2 * lambda;
				if (outputside*2 < inputside) spacing = 4 * lambda;
				ybbpos = ypos = ysize - ((ysize - (outputside-1)*spacing) / 2 + eindex * spacing);
				whichport = outputport;
				break;
			case BIDIRPORT:
				eindex = pp->temp1;
				spacing = 2 * lambda;
				if (bidirside*2 < topside) spacing = 4 * lambda;
				xbbpos = xpos = xsize - ((xsize - (bidirside-1)*spacing) / 2 + eindex * spacing);
				ypos = -2 * lambda;
				ybbpos = 0;
				whichport = bidirport;
				break;
			case PWRPORT:
			case GNDPORT:
				eindex = pp->temp1;
				spacing = 2 * lambda;
				if (topside*2 < bidirside) spacing = 4 * lambda;
				xbbpos = xpos = xsize - ((xsize - (topside-1)*spacing) / 2 + eindex * spacing);
				ypos = ysize + 2 * lambda;
				ybbpos = ysize;
				whichport = topport;
				break;
			default:		/* INPORT, unlabeled, and all CLOCK ports */
				xpos = -2 * lambda;
				xbbpos = 0;
				eindex = pp->temp1;
				spacing = 2 * lambda;
				if (inputside*2 < outputside) spacing = 4 * lambda;
				ybbpos = ypos = ysize - ((ysize - (inputside-1)*spacing) / 2 + eindex * spacing);
				whichport = inputport;
				break;
		}

		/* determine type of pin */
		pintype = pinproto;
		wiretype = wireproto;
		if (pp->subnodeinst != NONODEINST)
		{
			bpp = pp;
			while (bpp->subnodeinst->proto->primindex == 0) bpp = bpp->subportproto;
			if (bpp->subnodeinst->proto == buspproto)
			{
				pintype = buspproto;
				wiretype = busproto;
			}
		}

		/* create the pin */
		pinni = io_edifiplacepin(pintype, xpos, xpos, ypos, ypos, 0, 0, np);
		if (pinni == NONODEINST) return(NONODEPROTO);

		/* export the port that should be on this pin */
		port = newportproto(np, pinni, pintype->firstportproto, pp->protoname);
		if (port != NOPORTPROTO)
			port->userbits = pp->userbits | PORTDRAWN;

		/* wire this pin to the black box */
		(void)newarcinst(wiretype, defaultarcwidth(wiretype),
			us_makearcuserbits(wiretype), pinni, pintype->firstportproto,
			xpos, ypos, bbni, whichport, xbbpos, ybbpos, np);
	}
	return(np);
}

void io_ednamearc(ARCINST *ai)
{
	int Ix, Iy;
	REGISTER VARIABLE *var;
	CHAR *name, basename[WORD+1] ;

	if (io_edifgbl.isarray)
	{
		/* name the bus */
		if (io_edifgbl.net_reference)
		{
			(void)setvalkey((INTBIG)ai, VARCINST, EDIF_name_key,
				(INTBIG)io_edifgbl.net_reference, VSTRING);
		}

		/* a typical foreign array bus will have some range value appended
		 * to the name. This will cause some odd names with duplicate values
		 */
		name = basename;
		*name = 0;
		for (Ix = 0; Ix < io_edifgbl.arrayx; Ix++)
		{
			for (Iy = 0; Iy < io_edifgbl.arrayy; Iy++)
			{
				if (name != basename) *name++ = ',';
				if (io_edifgbl.arrayx > 1)
				{
					if (io_edifgbl.arrayy > 1)
						(void)esnprintf(name, WORD, x_("%s[%d,%d]"), io_edifgbl.net_name, Ix, Iy);
					else
						(void)esnprintf(name, WORD, x_("%s[%d]"), io_edifgbl.net_name, Ix);
				}
				else (void)esnprintf(name, WORD, x_("%s[%d]"), io_edifgbl.net_name, Iy);
				name += estrlen(name);
			}
		}
		var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)basename, VSTRING);
		if (var != NOVARIABLE)
			defaulttextsize(4, var->textdescript);
	} else
	{
		if (io_edifgbl.net_reference)
		{
			(void)setvalkey((INTBIG)ai, VARCINST, EDIF_name_key,
				(INTBIG)io_edifgbl.net_reference, VSTRING);
		}
		if (io_edifgbl.net_name)
		{
			/* set name of arc but don't display name */
			var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key,
				(INTBIG)io_edifgbl.net_name, VSTRING);
			if (var != NOVARIABLE)
				defaulttextsize(4, var->textdescript);
		}
	}
}

INTBIG io_edfindport(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap, NODEINST *nis[],
	PORTPROTO *pps[])
{
	INTBIG cnt;
	ARCINST *ai = NOARCINST, *ar1,*ar2;
	ARCPROTO *nap;
	NODEINST *ni, *fno, *tno;
	PORTPROTO *fpt, *tpt, *pp;
	NODEPROTO *np, *pnt;
	INTBIG wid, bits1, bits2, lx, hx, ly, hy, j;
	INTBIG fendx, fendy, tendx, tendy;

	cnt = io_edfindport_geom(cell, x, y, ap, nis, pps, &ai);

	if (cnt == 0 && ai != NOARCINST)
	{
		/* direct hit on an arc, verify connection */
		nap = ai->proto;
		np = getpinproto(nap);
		if (np == NONODEPROTO) return(0);
		pp = np->firstportproto;
		for (j = 0; pp->connects[j] != NOARCPROTO; j++)
			if (pp->connects[j] == ap) break;
		if (pp->connects[j] == NOARCPROTO) return(0);

		/* try to split arc (from us_getnodeonarcinst)*/
		/* break is at (prefx, prefy): save information about the arcinst */
		fno = ai->end[0].nodeinst;	fpt = ai->end[0].portarcinst->proto;
		tno = ai->end[1].nodeinst;	tpt = ai->end[1].portarcinst->proto;
		fendx = ai->end[0].xpos;	  fendy = ai->end[0].ypos;
		tendx = ai->end[1].xpos;	  tendy = ai->end[1].ypos;
		wid = ai->width;  pnt = ai->parent;
		bits1 = bits2 = ai->userbits;
		if ((bits1&ISNEGATED) != 0)
		{
			if ((bits1&REVERSEEND) == 0) bits2 &= ~ISNEGATED; else
				bits1 &= ~ISNEGATED;
		}
		if (figureangle(fendx,fendy, x,y) != figureangle(x,y, tendx, tendy))
		{
			bits1 &= ~FIXANG;
			bits2 &= ~FIXANG;
		}

		/* create the splitting pin */
		lx = x - (np->highx-np->lowx)/2;   hx = lx + np->highx-np->lowx;
		ly = y - (np->highy-np->lowy)/2;   hy = ly + np->highy-np->lowy;
		ni = io_edifiplacepin(np, lx,hx, ly,hy, 0, 0, pnt);
		if (ni == NONODEINST)
		{
			ttyputerr(_("Cannot create splitting pin"));
			return(0);
		}
		endobjectchange((INTBIG)ni, VNODEINST);

		/* set the node, and port */
		nis[cnt] = ni;
		pps[cnt++] = pp;

		/* create the two new arcinsts */
		ar1 = newarcinst(nap, wid, bits1, fno, fpt, fendx, fendy, ni, pp, x, y, pnt);
		ar2 = newarcinst(nap, wid, bits2, ni, pp, x, y, tno, tpt, tendx, tendy, pnt);
		if (ar1 == NOARCINST || ar2 == NOARCINST)
		{
			ttyputerr(_("Error creating the split arc parts"));
			return(cnt);
		}
		(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)ar1, VARCINST, FALSE);
		endobjectchange((INTBIG)ar1, VARCINST);
		endobjectchange((INTBIG)ar2, VARCINST);

		/* delete the old arcinst */
		startobjectchange((INTBIG)ai, VARCINST);
		if (killarcinst(ai))
			ttyputerr(_("Error deleting original arc"));
	}

	return(cnt);
}

/* module: ro_mazefindport
   function: will locate the nodeinstance and portproto corresponding to
   to a direct intersection with the given point.
   inputs:
   cell - cell to search
   x, y  - the point to exam
   ap    - the arc used to connect port (must match pp)
   nis   - pointer to ni pointer buffer.
   pps   - pointer to portproto pointer buffer.
   outputs:
   returns cnt if found, 0 not found, -1 on error
   ni = found ni instance
   pp = found pp proto.
 */
INTBIG io_edfindport_geom(NODEPROTO *cell, INTBIG x, INTBIG y, ARCPROTO *ap,
	NODEINST *nis[], PORTPROTO *pps[], ARCINST **ai)
{
	REGISTER INTBIG j, cnt, sea, dist, bestdist, bestpx, bestpy, bits, wid,
		lx, hx, ly, hy, xs, ys;
	INTBIG px, py;
	REGISTER GEOM *geom;
	REGISTER ARCINST *newai;
	static POLYGON *poly = NOPOLYGON;
	REGISTER PORTPROTO *pp, *bestpp;
	REGISTER NODEINST *ni, *bestni;
	NODEPROTO *np;

	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	cnt = 0;
	bestni = NONODEINST;
	sea = initsearch(x, x, y, y, cell);
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;

		switch (geom->entryisnode)
		{
			case TRUE:
				/* now locate a portproto */
				ni = geom->entryaddr.ni;
				for (pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					shapeportpoly(ni, pp, poly, FALSE);
					if (isinside(x, y, poly))
					{
						/* check if port connects to arc ...*/
						for (j = 0; pp->connects[j] != NOARCPROTO; j++)
							if (pp->connects[j] == ap)
						{
							nis[cnt] = ni;
							pps[cnt] = pp;
							cnt++;
						}
					} else
					{
						portposition(ni, pp, &px, &py);
						dist = computedistance(x, y, px, py);
						if (bestni == NONODEINST || dist < bestdist)
						{
							bestni = ni;
							bestpp = pp;
							bestdist = dist;
							bestpx = px;
							bestpy = py;
						}
					}
				}
				break;

			case FALSE:
				/* only accept valid wires */
				if (geom->entryaddr.ai->proto->tech == sch_tech)
					*ai = geom->entryaddr.ai;
				break;
		}
	}

	/* special case: make it connect to anything that is near */
	if (cnt == 0 && bestni != NONODEINST)
	{
		/* create a new node in the proper position */
		np = getpinproto(ap);
		xs = np->highx - np->lowx;   lx = x - xs/2;   hx = lx + xs;
		ys = np->highy - np->lowy;   ly = y - ys/2;   hy = ly + ys;
		ni = io_edifiplacepin(np, lx, hx, ly, hy, 0, 0, cell);
		wid = ap->nominalwidth;
		bits = us_makearcuserbits(ap);
		newai = newarcinst(ap, wid, bits, ni, ni->proto->firstportproto, x, y, bestni, bestpp, bestpx, bestpy, cell);
		nis[cnt] = ni;
		pps[cnt] = ni->proto->firstportproto;
		cnt++;
	}
	return(cnt);
}


NODEINST *io_edifiplacepin(NODEPROTO *np, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy,
	INTBIG trans, INTBIG angle, NODEPROTO *parent)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG cx, cy, sx, sy, icx, icy, isx, isy;

	cx = (lx + hx) / 2;
	cy = (ly + hy) / 2;
	sx = hx - lx;
	sy = hy - ly;

	for(ni = parent->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != np) continue;
		if (ni->rotation != angle || ni->transpose != trans) continue;
		icx = (ni->lowx + ni->highx) / 2;
		icy = (ni->lowy + ni->highy) / 2;
		isx = ni->highx - ni->lowx;
		isy = ni->highy - ni->lowy;
		if (icx != cx || icy != cy) continue;
		return(ni);
	}
	ni = newnodeinst(np, lx, hx, ly, hy, trans, angle, parent);
	return(ni);
}

