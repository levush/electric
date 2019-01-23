/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: io.c
 * Input/output tool: controller module
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

#include "global.h"
#include "database.h"
#include "conlay.h"
#include "egraphics.h"
#include "efunction.h"
#include "dbcontour.h"
#include "eio.h"
#include "usr.h"
#include "drc.h"
#include "network.h"
#include "usredtec.h"
#include "edialogs.h"
#include "tecart.h"
#include "tecschem.h"
#include "tecgen.h"
#include "tecmocmos.h"
#include <math.h>

/* the command parsing table */
static KEYWORD iocifoopt[] =
{
	{x_("fully-instantiated"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("exactly-as-displayed"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("individual-boxes"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("merge-boxes"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("include-cloak-layer"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ignore-cloak-layer"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("normalized"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not-normalized"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("exactly-as-displayed"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("instantiate-top"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dont-instantiate-top"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlight-resolution"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hide-resolution"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP io_cifop = {iocifoopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("style of CIF output"), 0};
static KEYWORD iocifiopt[] =
{
	{x_("rounded-wires"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("squared-wires"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iocifip = {iocifiopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("style of CIF input"), 0};
static KEYWORD iocifopt[] =
{
	{x_("input"),     1,{&iocifip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output"),    1,{&io_cifop,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP io_cifp = {iocifopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("control of CIF"), 0};

static KEYWORD iodxfaopt[] =
{
	{x_("all"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("restrict"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iodxfap = {iodxfaopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("DXF layer restriction options"), 0};
static KEYWORD iodxfopt[] =
{
	{x_("acceptable-layers"),   1,{&iodxfap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("flatten-input"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not-flatten-input"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP io_dxfp = {iodxfopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("DXF options"), 0};

static COMCOMP iogdsoarp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("maximum number of degrees per arc segment"), 0};
static COMCOMP iogdsoasp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("maximum sag distance for an arc segment"), 0};
static KEYWORD iogdsoopt[] =
{
	{x_("individual-boxes"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("merge-boxes"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("include-cloak-layer"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ignore-cloak-layer"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arc-resolution"),           1,{&iogdsoarp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arc-sag"),                  1,{&iogdsoasp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iogdsop = {iogdsoopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("style of GDS output"), 0};
static KEYWORD iogdsiopt[] =
{
	{x_("text"),                     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("expand"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arrays"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("unknown-layers"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-text"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-expand"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-arrays"),                0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-unknown-layers"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iogdsiop = {iogdsiopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("style of GDS input"), 0};
static KEYWORD iogdsopt[] =
{
	{x_("output"),   1,{&iogdsop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("input"),    1,{&iogdsiop,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP io_gdsp = {iogdsopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("control of GDS"), 0};

static KEYWORD ioedifopt[] =
{
	{x_("schematic"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("netlist"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP io_edifp = {ioedifopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("EDIF output option"), 0};

static KEYWORD ioplotnopt[] =
{
	{x_("focus"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("include-date"),0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP ioplotnp = {ioplotnopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("negating control of plot output"), 0};
static KEYWORD ioplotopt[] =
{
	{x_("not"),               1,{&ioplotnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("focus-highlighted"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("focus-window"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("include-date"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP ioplotp = {ioplotopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("control of plot output"), 0};

static KEYWORD ioveropt[] =
{
	{x_("off"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("on"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("graphical"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP ioverp = {ioveropt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("verbose option"), 0};

static COMCOMP iopostsynch = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	NOFILL|INPUTOPT, x_(" \t"), M_("PostScript synchronization file name"), 0};
static COMCOMP iopostplotwid = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("PostScript plotter width"), 0};
static COMCOMP iopostprintwid = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("PostScript printer width"), 0};
static COMCOMP iopostprinthei = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("PostScript printer height"), 0};
static COMCOMP iopostmargin = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("PostScript margin"), 0};
static KEYWORD iopostopt[] =
{
	{x_("plain"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("encapsulated"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rotate"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-rotate"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("auto-rotate"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("color"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("stippled-color"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("merged-color"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gray-scale"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("printer"),          2,{&iopostprintwid,&iopostprinthei,NOKEY,NOKEY,NOKEY}},
	{x_("plotter"),          1,{&iopostplotwid,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("margin"),           1,{&iopostmargin,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("unsynchronize"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("synchronize"),      1,{&iopostsynch,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iopostp = {iopostopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("control of PostScript format"), 0};

static COMCOMP iohpgl2sp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("internal units per pixel"), 0};
static KEYWORD iohpgl2opt[] =
{
	{x_("scale"),           1,{&iohpgl2sp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iohpgl2p = {iohpgl2opt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("HPGL/2 scaling option"), 0};
static KEYWORD iohpglopt[] =
{
	{x_("1"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("2"),               1,{&iohpgl2p,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iohpglp = {iohpglopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("HPGL version"), 0};

static COMCOMP iobloatlp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Layer name to bloat"), 0};
static COMCOMP iobloatap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("amount to bloat layer (in internal units)"), 0};

static KEYWORD iobinopt[] =
{
	{x_("no-backup"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("one-level-backup"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("many-level-backup"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-check"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP iobinp = {iobinopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Binary options"), 0};

static KEYWORD ioopt[] =
{
	{x_("cif"),            1,{&io_cifp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dxf"),            1,{&io_dxfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gds"),            1,{&io_gdsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("edif"),           1,{&io_edifp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("binary"),         1,{&iobinp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("plot"),           1,{&ioplotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("verbose"),        1,{&ioverp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("postscript"),     1,{&iopostp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hpgl"),           1,{&iohpglp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bloat-output"),   2,{&iobloatlp,&iobloatap,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP io_iop = {ioopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
		0, x_(" \t"), M_("Input/Output action"), 0};

static struct
{
	CHAR *name;
	INTBIG required, bits;
} io_formatlist[] =
{
	{x_("binary"),             1, FBINARY},
	{x_("nobackupbinary"),     1, FBINARYNOBACKUP},
	{x_("cif"),                1, FCIF},
	{x_("def"),                2, FDEF},
	{x_("dxf"),                2, FDXF},
	{x_("eagle"),              2, FEAGLE},
	{x_("ecad"),               2, FECAD},
	{x_("edif"),               2, FEDIF},
	{x_("gds"),                1, FGDS},
	{x_("hpgl"),               1, FHPGL},
	{x_("l"),                  1, FL},
	{x_("lef"),                2, FLEF},
	{x_("pads"),               2, FPADS},
	{x_("postscript"),         2, FPOSTSCRIPT},
	{x_("printed-postscript"), 2, FPRINTEDPOSTSCRIPT},
#if defined(MACOS) && !defined(MACOSX)
	{x_("quickdraw"),          1, FQUICKDRAW},
#endif
	{x_("sdf"),                2, FSDF},
#ifdef FORCECADENCE
	{x_("skill"),              2, FSKILL},
#endif
	{x_("sue"),                2, FSUE},
	{x_("text"),               1, FTEXT},
#if VHDLTOOL
	{x_("vhdl"),               1, FVHDL},
#endif
	{NULL, 0, 0}
};

#define NOBLOAT ((BLOAT *)-1)

typedef struct Ibloat
{
	CHAR  *layer;
	INTBIG  amount;
	struct Ibloat *nextbloat;
} BLOAT;

static BLOAT *io_curbloat = NOBLOAT;

/* working memory for "io_setuptechorder()" */
static INTBIG      io_maxlayers;
static INTBIG      io_mostlayers = 0;
static INTBIG     *io_overlaporder;

/* miscellaneous */
       FILE       *io_fileout;					/* channel for output */
       jmp_buf     io_filerror;					/* nonlocal jump when I/O fails */
       INTBIG      io_cifbase;					/* index used when writing CIF */
       INTBIG      io_postscriptfilenamekey;	/* key for "IO_postscript_filename" */
       INTBIG      io_postscriptfiledatekey;	/* key for "IO_postscript_filedate" */
       INTBIG      io_postscriptepsscalekey;	/* key for "IO_postscript_EPS_scale" */
       INTBIG      io_verbose;					/* 0: silent  1:chattier  -1:graphical */
       TOOL       *io_tool;						/* the I/O tool object */
       INTBIG      io_filetypeblib;				/* Binary library disk file descriptor */
       INTBIG      io_filetypecif;				/* CIF disk file descriptor */
       INTBIG      io_filetypedef;				/* DEF disk file descriptor */
       INTBIG      io_filetypedxf;				/* DXF disk file descriptor */
       INTBIG      io_filetypeeagle;			/* EAGLE disk netlist file descriptor */
       INTBIG      io_filetypeecad;				/* ECAD disk netlist file descriptor */
       INTBIG      io_filetypeedif;				/* EDIF disk file descriptor */
       INTBIG      io_filetypegds;				/* GDS disk file descriptor */
       INTBIG      io_filetypehpgl;				/* HPGL disk file descriptor */
       INTBIG      io_filetypehpgl2;			/* HPGL2 disk file descriptor */
       INTBIG      io_filetypel;				/* L disk file descriptor */
       INTBIG      io_filetypelef;				/* LEF disk file descriptor */
       INTBIG      io_filetypepads;				/* PADS netlist disk file descriptor */
       INTBIG      io_filetypeps;				/* PostScript disk file descriptor */
       INTBIG      io_filetypesdf;				/* SDF disk file descriptor */
       INTBIG      io_filetypeskill;			/* SKILL commands disk file descriptor */
       INTBIG      io_filetypesue;				/* SUE disk file descriptor */
       INTBIG      io_filetypetlib;				/* Text library disk file descriptor */
       INTBIG      io_filetypevhdl;				/* VHDL disk file descriptor */
       INTBIG      io_libinputrecursivedepth;	/* for recursing when reading dependent libraries */
       INTBIG      io_libinputreadmany;			/* nonzero if reading dependent libraries */
static INTBIG      io_state_key;				/* key for "IO_state" */
static INTBIG      io_libraryannouncetotal = 0;	/* size of library announcement queue */
static INTBIG      io_libraryannouncecount;		/* number of libraries queued for announcement */
static LIBRARY   **io_libraryannounce;			/* list of libraries queued for announcement */

/* shared prototypes */
void io_compute_center(INTBIG xc, INTBIG yc, INTBIG x1, INTBIG y1,
		INTBIG x2, INTBIG y2, INTBIG *cx, INTBIG *cy);

/* prototypes for local routines */
static double io_calc_angle(double r, double dx, double dy);
static void   io_fixrtree(RTNODE*);
static void   io_fixtechlayers(LIBRARY *lib);
static void   io_unifylambdavalues(LIBRARY *lib);
static void   io_libraryoptiondlog(void);
static void   io_cdloptionsdialog(void);
static void   io_sueoptionsdialog(void);
static void   io_getversion(LIBRARY *lib, INTBIG *major, INTBIG *minor, INTBIG *detail);
static void   io_convertallrelativetext(INTSML numvar, VARIABLE *firstvar);
static void   io_convertrelativetext(UINTBIG *descript);
static void   io_fixupnodeinst(NODEINST *ni, INTBIG dlx, INTBIG dly, INTBIG dhx, INTBIG dhy);
static void   io_convertalltextdescriptors(INTSML numvar, VARIABLE *firstvar, INTBIG how);
static void   io_converteverytextdescriptor(LIBRARY *lib, INTBIG how);
static void   io_converttextdescriptor(UINTBIG *descript, INTBIG how);
static void   io_setoutputbloat(CHAR *layer, INTBIG amount);

INTBIG *io_getstatebits(void)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, len;
	static INTBIG mybits[NUMIOSTATEBITWORDS];

	var = getvalkey((INTBIG)io_tool, VTOOL, -1, io_state_key);
	if (var == NOVARIABLE)
	{
		for(i=1; i<NUMIOSTATEBITWORDS; i++) mybits[i] = 0;
		mybits[0] = DXFFLATTENINPUT|GDSINARRAYS;
		return(mybits);
	}
	if ((var->type&VISARRAY) != 0)
	{
		for(i=0; i<NUMIOSTATEBITWORDS; i++) mybits[i] = 0;
		len = getlength(var);
		for(i=0; i<len; i++) mybits[i] = ((INTBIG *)var->addr)[i];
		return(mybits);
	}
	for(i=1; i<NUMIOSTATEBITWORDS; i++) mybits[i] = 0;
	mybits[0] = var->addr;
	return(mybits);
}

void io_setstatebits(INTBIG *bits)
{
	(void)setvalkey((INTBIG)io_tool, VTOOL, io_state_key, (INTBIG)bits,
		VINTEGER|VISARRAY|(NUMIOSTATEBITWORDS<<VLENGTHSH));
}

void io_init(INTBIG  *argc, CHAR1 *argv[], TOOL *thistool)
{
	INTBIG statebits[NUMIOSTATEBITWORDS], i;

	/* nothing for pass 2 or 3 initialization */
	if (thistool == NOTOOL || thistool == 0) return;

	/* pass 1 initialization */
	io_tool = thistool;
	io_state_key = makekey(x_("IO_state"));
	io_postscriptfilenamekey = makekey(x_("IO_postscript_filename"));
	io_postscriptfiledatekey = makekey(x_("IO_postscript_filedate"));
	io_postscriptepsscalekey = makekey(x_("IO_postscript_EPS_scale"));
	nextchangequiet();
	for(i=1; i<NUMIOSTATEBITWORDS; i++) statebits[i] = 0;
	statebits[0] = DXFFLATTENINPUT|GDSINARRAYS|HPGL2;
	(void)setvalkey((INTBIG)io_tool, VTOOL, io_state_key, (INTBIG)statebits,
		VINTEGER|VISARRAY|(NUMIOSTATEBITWORDS<<VLENGTHSH)|VDONTSAVE);
	io_verbose = 0;

	DiaDeclareHook(x_("libopt"), &ioverp, io_libraryoptiondlog);
	DiaDeclareHook(x_("cdlopt"), &iobloatlp, io_cdloptionsdialog);
	DiaDeclareHook(x_("sueopt"), &iobloatap, io_sueoptionsdialog);

	/* create disk file descriptors */
	io_filetypeblib  = setupfiletype(x_("elib"), x_("*.elib"),       MACFSTAG('Elec'), TRUE,  x_("blib"),  _("Binary library"));
	io_filetypecif   = setupfiletype(x_("cif"),  x_("*.cif"),        MACFSTAG('TEXT'), FALSE, x_("cif"),   _("CIF"));
	io_filetypedef   = setupfiletype(x_("def"),  x_("*.def"),        MACFSTAG('TEXT'), FALSE, x_("def"),   _("DEF"));
	io_filetypedxf   = setupfiletype(x_("dxf"),  x_("*.dxf"),        MACFSTAG('TEXT'), FALSE, x_("dxf"),   _("AutoCAD DXF"));
	io_filetypeeagle = setupfiletype(x_("txt"),  x_("*.txt"),        MACFSTAG('TEXT'), FALSE, x_("eagle"), _("Eagle netlist"));
	io_filetypeecad  = setupfiletype(x_("enl"),  x_("*.enl"),        MACFSTAG('TEXT'), FALSE, x_("ecad"),  _("ECAD netlist"));
	io_filetypeedif  = setupfiletype(x_("edif"), x_("*.edif;*.ed?"), MACFSTAG('TEXT'), FALSE, x_("edif"),  _("EDIF"));
	io_filetypegds   = setupfiletype(x_("gds"),  x_("*.gds"),        MACFSTAG('GDS '), TRUE,  x_("gds"),   _("GDS II"));
	io_filetypehpgl  = setupfiletype(x_("hpgl"), x_("*.hpgl"),       MACFSTAG('TEXT'), FALSE, x_("hpgl"),  _("HPGL"));
	io_filetypehpgl2 = setupfiletype(x_("hpgl2"),x_("*.hpgl2"),      MACFSTAG('TEXT'), FALSE, x_("hpgl2"), _("HPGL/2"));
	io_filetypel     = setupfiletype(x_("l"),    x_("*.l"),          MACFSTAG('TEXT'), FALSE, x_("l"),     _("L"));
	io_filetypelef   = setupfiletype(x_("lef"),  x_("*.lef"),        MACFSTAG('TEXT'), FALSE, x_("lef"),   _("LEF"));
	io_filetypepads  = setupfiletype(x_("asc"),  x_("*.asc"),        MACFSTAG('TEXT'), FALSE, x_("pad"),   _("PADS netlist"));
	io_filetypeps    = setupfiletype(x_("ps"),   x_("*.ps;*.eps"),   MACFSTAG('TEXT'), FALSE, x_("ps"),    _("PostScript"));
	io_filetypesdf   = setupfiletype(x_("sdf"),  x_("*.sdf"),        MACFSTAG('TEXT'), FALSE, x_("sdf"),   _("SDF"));
	io_filetypeskill = setupfiletype(x_("il"),   x_("*.il"),         MACFSTAG('TEXT'), FALSE, x_("skill"), _("SKILL commands"));
	io_filetypesue   = setupfiletype(x_("sue"),  x_("*.sue"),        MACFSTAG('TEXT'), FALSE, x_("sue"),   _("SUE"));
	io_filetypetlib  = setupfiletype(x_("txt"),  x_("*.txt"),        MACFSTAG('TEXT'), FALSE, x_("tlib"),  _("Text library"));
	io_filetypevhdl  = setupfiletype(x_("vhdl"), x_("*.vhdl;*.vhd"), MACFSTAG('TEXT'), FALSE, x_("vhdl"),  _("VHDL"));

	io_initcif();
	io_initdef();
	io_initdxf();
	io_initedif();
	io_initgds();
#ifdef FORCECADENCE
	io_initskill();
#endif
}

void io_done(void)
{
#ifdef DEBUGMEMORY
	if (io_mostlayers > 0) efree((CHAR *)io_overlaporder);
	io_freebininmemory();
	io_freebinoutmemory();
	io_freepostscriptmemory();
	io_freetextinmemory();
	io_freecifinmemory();
	io_freecifparsmemory();
	io_freecifoutmemory();
	io_freedefimemory();
	io_freedxfmemory();
	io_freeedifinmemory();
	io_freegdsinmemory();
	io_freegdsoutmemory();
	io_freelefimemory();
	io_freesdfimemory();
	io_freesuememory();
	if (io_libraryannouncetotal > 0)
		efree((CHAR *)io_libraryannounce);
#endif
}

void io_slice(void)
{
	ttyputmsg(M_("Input and Output are performed with the 'library' command"));
	ttyputmsg(M_("...I/O tool turned off"));
	toolturnoff(io_tool, FALSE);
}

void io_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER INTBIG *curstate, scale, wid, hei;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pp;
	REGISTER VARIABLE *var;
	INTBIG arcres, arcsag;

	if (count == 0)
	{
		count = ttygetparam(M_("IO option:"), &io_iop, MAXPARS, par);
		if (count == 0)
		{
			ttyputerr(M_("Aborted"));
			return;
		}
	}
	l = estrlen(pp = par[0]);

	/* get current state of I/O tool */
	curstate = io_getstatebits();

	/* check for bloating specifications */
	if (namesamen(pp, x_("bloat-output"), l) == 0)
	{
		if (count <= 1)
		{
			io_setoutputbloat(x_(""), 0);
			return;
		}
		if (count < 3)
		{
			ttyputusage(x_("telltool io bloat-output LAYER AMOUNT"));
			return;
		}
		io_setoutputbloat(par[1], myatoi(par[2]));
		return;
	}

	/* check for format specifications */
	if (namesamen(pp, x_("plot"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io plot OPTIONS"));
			return;
		}

		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("not"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool io plot not OPTION"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("focus"), l) == 0)
			{
				curstate[0] &= ~(PLOTFOCUS|PLOTFOCUSDPY);
				io_setstatebits(curstate);
				ttyputverbose(M_("Plot output will display entire cell"));
				return;
			}
			if (namesamen(pp, x_("include-date"), l) == 0)
			{
				curstate[0] &= ~PLOTDATES;
				io_setstatebits(curstate);
				ttyputverbose(M_("Plot output will not include dates"));
				return;
			}
			ttyputbadusage(x_("telltool io plot not"));
			return;
		}
		if (namesamen(pp, x_("focus-highlighted"), l) == 0 && l >= 7)
		{
			curstate[0] = (curstate[0] & ~PLOTFOCUSDPY) | PLOTFOCUS;
			io_setstatebits(curstate);
			ttyputverbose(M_("Plot output will focus on highlighted area"));
			return;
		}
		if (namesamen(pp, x_("focus-window"), l) == 0 && l >= 7)
		{
			curstate[0] |= (PLOTFOCUSDPY|PLOTFOCUS);
			io_setstatebits(curstate);
			ttyputverbose(M_("Plot output will focus on highlighted area"));
			return;
		}
		if (namesamen(pp, x_("include-date"), l) == 0)
		{
			curstate[0] |= PLOTDATES;
			io_setstatebits(curstate);
			ttyputverbose(M_("Plot output will include dates"));
			return;
		}
		ttyputbadusage(x_("telltool io plot"));
		return;
	}

	/* check for binary specifications */
	if (namesamen(pp, x_("binary"), l) == 0)
	{
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("no-backup"), l) == 0 && l >= 4)
		{
			curstate[0] = (curstate[0] & ~BINOUTBACKUP) | BINOUTNOBACK;
			io_setstatebits(curstate);
			ttyputverbose(M_("Binary output will not keep backups"));
			return;
		}
		if (namesamen(pp, x_("one-level-backup"), l) == 0)
		{
			curstate[0] = (curstate[0] & ~BINOUTBACKUP) | BINOUTONEBACK;
			io_setstatebits(curstate);
			ttyputverbose(M_("Binary output will keep one backup"));
			return;
		}
		if (namesamen(pp, x_("many-level-backup"), l) == 0)
		{
			curstate[0] = (curstate[0] & ~BINOUTBACKUP) | BINOUTFULLBACK;
			io_setstatebits(curstate);
			ttyputverbose(M_("Binary output will keep full backups"));
			return;
		}

		if (namesamen(pp, x_("no-check"), l) == 0 && l >= 4)
		{
			curstate[0] &= ~CHECKATWRITE;
			io_setstatebits(curstate);
			ttyputverbose(M_("Binary output will not check the database"));
			return;
		}
		if (namesamen(pp, x_("check"), l) == 0)
		{
			curstate[0] |= CHECKATWRITE;
			io_setstatebits(curstate);
			ttyputverbose(M_("Binary output will check the database"));
			return;
		}
		ttyputbadusage(x_("telltool io binary"));
		return;
	}

	if (namesamen(pp, x_("cif"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io cif (input|output)"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("input"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool io cif input OPTIONS"));
				return;
			}
			l = estrlen(pp = par[2]);
			switch (*pp)
			{
				case 'r':
					curstate[0] &= ~CIFINSQUARE;
					io_setstatebits(curstate);
					ttyputverbose(M_("CIF wires will have rounded ends"));
					break;
				case 's':
					curstate[0] |= CIFINSQUARE;
					io_setstatebits(curstate);
					ttyputverbose(M_("CIF wires will have squared ends"));
					break;
				default:
					ttyputbadusage(x_("telltool io cif input"));
			}
			return;
		}
		if (namesamen(pp, x_("output"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool io cif output OPTIONS"));
				return;
			}

			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("fully-instantiated"), l) == 0)
			{
				curstate[0] &= ~CIFOUTEXACT;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will include all cells"));
				return;
			}
			if (namesamen(pp, x_("exactly-as-displayed"), l) == 0)
			{
				curstate[0] |= CIFOUTEXACT;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will duplicate screen"));
				return;
			}
			if (namesamen(pp, x_("merge-boxes"), l) == 0)
			{
				curstate[0] |= CIFOUTMERGE;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will merge boxes"));
				return;
			}
			if (namesamen(pp, x_("individual-boxes"), l) == 0 && l >= 3)
			{
				curstate[0] &= ~CIFOUTMERGE;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will list individual boxes"));
				return;
			}
			if (namesamen(pp, x_("include-cloak-layer"), l) == 0 && l >= 3)
			{
				curstate[0] |= CIFOUTADDDRC;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will include DRC cloak layer"));
				return;
			}
			if (namesamen(pp, x_("ignore-cloak-layer"), l) == 0 && l >= 2)
			{
				curstate[0] &= ~CIFOUTADDDRC;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will ignore DRC cloak layer"));
				return;
			}
			if (namesamen(pp, x_("normalized"), l) == 0 && l >= 3)
			{
				curstate[0] |= CIFOUTNORMALIZE;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will normalize coordinates"));
				return;
			}
			if (namesamen(pp, x_("not-normalized"), l) == 0 && l >= 3)
			{
				curstate[0] &= ~CIFOUTNORMALIZE;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will use cell coordinates"));
				return;
			}
			if (namesamen(pp, x_("instantiate-top"), l) == 0 && l >= 3)
			{
				curstate[0] &= ~CIFOUTNOTOPCALL;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will instantiate top-level cell"));
				return;
			}
			if (namesamen(pp, x_("dont-instantiate-top"), l) == 0 && l >= 3)
			{
				curstate[0] |= CIFOUTNOTOPCALL;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will not instantiate top-level cell"));
				return;
			}
			if (namesamen(pp, x_("highlight-resolution"), l) == 0 && l >= 3)
			{
				curstate[1] |= CIFRESHIGH;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will highlight resolution errors"));
				return;
			}
			if (namesamen(pp, x_("hide-resolution"), l) == 0 && l >= 3)
			{
				curstate[1] &= ~CIFRESHIGH;
				io_setstatebits(curstate);
				ttyputverbose(M_("CIF generation will not highlight resolution errors"));
				return;
			}
			ttyputbadusage(x_("telltool io cif output"));
		}
		ttyputbadusage(x_("telltool io cif"));
		return;
	}

	if (namesamen(pp, x_("gds"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io gds OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("input"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool io gds input OPTION"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("text"), l) == 0)
			{
				curstate[0] |= GDSINTEXT;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input includes text"));
				return;
			}
			if (namesamen(pp, x_("no-text"), l) == 0 && l >= 4)
			{
				curstate[0] &= ~GDSINTEXT;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input ignores text"));
				return;
			}
			if (namesamen(pp, x_("expand"), l) == 0)
			{
				curstate[0] |= GDSINEXPAND;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input expands instances"));
				return;
			}
			if (namesamen(pp, x_("no-expand"), l) == 0 && l >= 4)
			{
				curstate[0] &= ~GDSINEXPAND;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input does not expand instances"));
				return;
			}
			if (namesamen(pp, x_("arrays"), l) == 0)
			{
				curstate[0] |= GDSINARRAYS;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input instantiates arrays"));
				return;
			}
			if (namesamen(pp, x_("no-arrays"), l) == 0 && l >= 4)
			{
				curstate[0] &= ~GDSINARRAYS;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input does not instantiate arrays"));
				return;
			}
			if (namesamen(pp, x_("unknown-layers"), l) == 0)
			{
				curstate[0] |= GDSINIGNOREUKN;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input includes unknown layers"));
				return;
			}
			if (namesamen(pp, x_("no-unknown-layers"), l) == 0 && l >= 4)
			{
				curstate[0] &= ~GDSINIGNOREUKN;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS input ignores unknown layers"));
				return;
			}
			ttyputbadusage(x_("telltool io gds input"));
			return;
		}
		if (namesamen(pp, x_("output"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool io gds output OPTION"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("merge-boxes"), l) == 0)
			{
				curstate[0] |= GDSOUTMERGE;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS generation will merge boxes"));
				return;
			}
			if (namesamen(pp, x_("individual-boxes"), l) == 0 && l >= 3)
			{
				curstate[0] &= ~GDSOUTMERGE;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS generation will list individual boxes"));
				return;
			}
			if (namesamen(pp, x_("include-cloak-layer"), l) == 0 && l >= 3)
			{
				curstate[0] |= GDSOUTADDDRC;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS generation will include DRC cloak layer"));
				return;
			}
			if (namesamen(pp, x_("ignore-cloak-layer"), l) == 0 && l >= 2)
			{
				curstate[0] &= ~GDSOUTADDDRC;
				io_setstatebits(curstate);
				ttyputverbose(M_("GDS generation will ignore DRC cloak layer"));
				return;
			}
			if (namesamen(pp, x_("arc-resolution"), l) == 0 && l >= 5)
			{
				getcontoursegmentparameters(&arcres, &arcsag);
				if (count > 3) arcres = atofr(par[3]) * 10 / WHOLE;
				ttyputverbose(M_("GDS arc generation will create a line every %s degrees"),
					frtoa(arcres*WHOLE/10));
				setcontoursegmentparameters(arcres, arcsag);
				return;
			}
			if (namesamen(pp, x_("arc-sag"), l) == 0 && l >= 5)
			{
				getcontoursegmentparameters(&arcres, &arcsag);
				if (count > 3) arcsag = atola(par[3], 0);
				ttyputverbose(M_("GDS arc generation will sag no more than %s"), latoa(arcsag, 0));
				setcontoursegmentparameters(arcres, arcsag);
				return;
			}
			ttyputbadusage(x_("telltool io gds output"));
			return;
		}
		ttyputbadusage(x_("telltool io gds"));
		return;
	}

	if (namesamen(pp, x_("dxf"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io dxf OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("acceptable-layers"), l) == 0)
		{
			if (count <= 2)
			{
				if ((curstate[0]&DXFALLLAYERS) == 0)
					ttyputmsg(M_("DXF input will accept only layers listed by technology")); else
						ttyputmsg(M_("DXF input will accept all layers"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("all"), l) == 0)
			{
				curstate[0] |= DXFALLLAYERS;
				io_setstatebits(curstate);
				ttyputverbose(M_("DXF input will accept all layers"));
				return;
			}
			if (namesamen(pp, x_("restrict"), l) == 0)
			{
				curstate[0] &= ~DXFALLLAYERS;
				io_setstatebits(curstate);
				ttyputverbose(M_("DXF input will accept only layers listed by technology"));
				return;
			}
		}
		if (namesamen(pp, x_("flatten-input"), l) == 0)
		{
			curstate[0] |= DXFFLATTENINPUT;
			io_setstatebits(curstate);
			ttyputverbose(M_("DXF input will flatten input blocks"));
			return;
		}
		if (namesamen(pp, x_("not-flatten-input"), l) == 0)
		{
			curstate[0] &= ~DXFFLATTENINPUT;
			io_setstatebits(curstate);
			ttyputverbose(M_("DXF input will retain input hierarchy"));
			return;
		}
		ttyputbadusage(x_("telltool io dxf"));
		return;
	}

	if (namesamen(pp, x_("postscript"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io postscript OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("encapsulated"), l) == 0)
		{
			curstate[0] |= EPSPSCRIPT;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will be encapsulated format"));
			return;
		}
		if (namesamen(pp, x_("plain"), l) == 0 && l >= 2)
		{
			curstate[0] &= ~EPSPSCRIPT;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will be plain format"));
			return;
		}
		if (namesamen(pp, x_("color"), l) == 0)
		{
			curstate[0] = (curstate[0] & ~PSCOLOR2) | PSCOLOR1;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will use color"));
			return;
		}
		if (namesamen(pp, x_("merged-color"), l) == 0 && l >= 2)
		{
			curstate[0] = (curstate[0] & ~PSCOLOR1) | PSCOLOR2;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will use merged color"));
			return;
		}
		if (namesamen(pp, x_("stippled-color"), l) == 0 && l >= 2)
		{
			curstate[0] |= PSCOLOR1 | PSCOLOR2;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will use color"));
			return;
		}
		if (namesamen(pp, x_("gray-scale"), l) == 0)
		{
			curstate[0] &= ~(PSCOLOR1|PSCOLOR2);
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will be gray-scale"));
			return;
		}
		if (namesamen(pp, x_("rotate"), l) == 0)
		{
			curstate[0] |= PSROTATE;
			curstate[1] &= ~PSAUTOROTATE;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will be rotated 90 degrees"));
			return;
		}
		if (namesamen(pp, x_("no-rotate"), l) == 0)
		{
			curstate[0] &= ~PSROTATE;
			curstate[1] &= ~PSAUTOROTATE;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will appear unrotated"));
			return;
		}
		if (namesamen(pp, x_("auto-rotate"), l) == 0)
		{
			curstate[0] &= ~PSROTATE;
			curstate[1] |= PSAUTOROTATE;
			io_setstatebits(curstate);
			ttyputverbose(M_("PostScript output will automatically rotate to fit best"));
			return;
		}
		if (namesamen(pp, x_("margin"), l) == 0 && l >= 2)
		{
			if (count == 3)
			{
				wid = atofr(par[2]);
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_margin"), wid, VFRACT);
			} else
			{
				var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
				if (var == NOVARIABLE) wid = muldiv(DEFAULTPSMARGIN, WHOLE, 75); else
					wid = var->addr;
			}
			ttyputverbose(M_("PostScript output will use a %s-wide margin"), frtoa(wid));
			return;
		}
		if (namesamen(pp, x_("plotter"), l) == 0 && l >= 2)
		{
			curstate[0] |= PSPLOTTER;
			io_setstatebits(curstate);
			if (count == 3)
			{
				wid = atofr(par[2]);
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_width"), wid, VFRACT);
				ttyputverbose(M_("PostScript output will assume a %s-wide plotter"), frtoa(wid));
			} else
				ttyputverbose(M_("PostScript output will assume a continuous-roll plotter"));
			return;
		}
		if (namesamen(pp, x_("printer"), l) == 0 && l >= 2)
		{
			curstate[0] &= ~PSPLOTTER;
			io_setstatebits(curstate);
			if (count == 4)
			{
				wid = atofr(par[2]);
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_width"), wid, VFRACT);
				hei = atofr(par[3]);
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_height"), hei, VFRACT);
				ttyputverbose(M_("PostScript output will assume a %s x %s page"),
					frtoa(wid), frtoa(hei));
			} else
				ttyputverbose(M_("PostScript output will be assume a fixed-size sheet"));
			return;
		}
		if (namesamen(pp, x_("unsynchronize"), l) == 0)
		{
			np = el_curlib->curnodeproto;
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("Edit a cell before removing synchronization"));
				return;
			}
			if (getvalkey((INTBIG)np, VNODEPROTO, VSTRING, io_postscriptfilenamekey) != NOVARIABLE)
			{
				(void)delvalkey((INTBIG)np, VNODEPROTO, io_postscriptfilenamekey);
				ttyputverbose(M_("Cell %s no longer synchronized to PostScript disk file"),
					describenodeproto(np));
			}
			return;
		}
		if (namesamen(pp, x_("synchronize"), l) == 0 && l >= 2)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool io postscript synchronize FILENAME"));
				return;
			}
			np = el_curlib->curnodeproto;
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("Edit a cell to synchronize it to a PostScript file"));
				return;
			}
			(void)setvalkey((INTBIG)np, VNODEPROTO, io_postscriptfilenamekey,
				(INTBIG)par[1], VSTRING);
			ttyputverbose(M_("Cell %s synchronized to PostScript file %s"),
				describenodeproto(np), par[1]);
			return;
		}
		ttyputbadusage(x_("telltool io postscript"));
		return;
	}

	if (namesamen(pp, x_("hpgl"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io hpgl (1|2)"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("1"), l) == 0)
		{
			curstate[0] &= ~HPGL2;
			io_setstatebits(curstate);
			ttyputmsg(M_("HPGL output will use older (version 1) format"));
			return;
		}
		if (namesamen(pp, x_("2"), l) == 0)
		{
			curstate[0] |= HPGL2;
			io_setstatebits(curstate);
			ttyputverbose(M_("HPGL output will use newer (HPGL/2) format"));
			if (count >= 4 && namesamen(par[2], x_("scale"), estrlen(par[2])) == 0)
			{
				scale = myatoi(par[3]);
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_hpgl2_scale"),
					scale, VINTEGER);
				ttyputverbose(M_("HPGL/2 plots will scale at %ld %ss per pixel"),
					scale, unitsname(el_units));
			}
			return;
		}
		ttyputbadusage(x_("telltool io hpgl"));
		return;
	}

	if (namesamen(pp, x_("edif"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io edif (schematic|netlist)"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("schematic"), l) == 0)
		{
			curstate[0] |= EDIFSCHEMATIC;
			io_setstatebits(curstate);
			ttyputverbose(M_("EDIF output will write schematics"));
			return;
		}
		if (namesamen(pp, x_("netlist"), l) == 0)
		{
			curstate[0] &= ~EDIFSCHEMATIC;
			io_setstatebits(curstate);
			ttyputverbose(M_("EDIF output will write netlists"));
			return;
		}
		ttyputbadusage(x_("telltool io edif"));
		return;
	}

	if (namesamen(pp, x_("verbose"), l) == 0)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool io verbose OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
		{
			io_verbose = 0;
			ttyputverbose(M_("I/O done silently"));
			return;
		}
		if (namesamen(pp, x_("on"), l) == 0 && l >= 2)
		{
			io_verbose = 1;
			ttyputverbose(M_("I/O prints status"));
			return;
		}
		if (namesamen(pp, x_("graphical"), l) == 0 && l >= 1)
		{
			io_verbose = -1;
			ttyputverbose(M_("I/O shows graphical progress"));
			return;
		}
		ttyputbadusage(x_("telltool io verbose"));
		return;
	}

	ttyputbadusage(x_("telltool io"));
}

/*
 * make I/O requests of this tool:
 *
 * "read" TAKES: LIBRARY to read and string style
 * "write" TAKES: LIBRARY to write and string style
 * "verbose" TAKES: new verbose factor, RETURNS: old verbose factor
 */
INTBIG io_request(CHAR *command, va_list ap)
{
	REGISTER LIBRARY *lib;
	REGISTER INTBIG i, j, len, format;
	REGISTER BOOLEAN err;
	REGISTER INTBIG arg1, arg3;
	CHAR *arg2;

	if (namesame(command, x_("verbose")) == 0)
	{
		i = io_verbose;
		io_verbose = va_arg(ap, INTBIG);
		return(i);
	}

	/* get the arguments */
	arg1 = va_arg(ap, INTBIG);
	arg2 = va_arg(ap, CHAR*);

	/* find desired library and format */
	lib = (LIBRARY *)arg1;
	format = -1;
	len = estrlen(arg2);
	for(i=0; io_formatlist[i].name != 0; i++)
		if (namesamen(arg2, io_formatlist[i].name, len) == 0 && len >= io_formatlist[i].required)
	{
		format = io_formatlist[i].bits;
		break;
	}
	if (format < 0)
	{
		ttyputerr(M_("Unknown I/O format: %s"), arg2);
		return(1);
	}

	if (namesame(command, x_("write")) == 0)
	{
		/* announce the beginning of library output */
		for(i=0; i<el_maxtools; i++)
			if (el_tools[i].writelibrary != 0)
				(*el_tools[i].writelibrary)(lib, FALSE);

		switch (format)
		{
			case FBINARY:
				err = io_writebinlibrary(lib, FALSE);
				break;

			case FBINARYNOBACKUP:
				err = io_writebinlibrary(lib, TRUE);
				break;

			case FCIF:
				err = io_writeciflibrary(lib);
				break;

			case FDXF:
				err = io_writedxflibrary(lib);
				break;

			case FEAGLE:
				err = io_writeeaglelibrary(lib);
				break;

			case FECAD:
				err = io_writeecadlibrary(lib);
				break;

			case FEDIF:
				err = io_writeediflibrary(lib);
				break;

			case FGDS:
				err = io_writegdslibrary(lib);
				break;

			case FHPGL:
				err = io_writehpgllibrary(lib);
				break;

			case FL:
				err = io_writellibrary(lib);
				break;

			case FLEF:
				err = io_writeleflibrary(lib);
				break;

			case FPADS:
				err = io_writepadslibrary(lib);
				break;

			case FPOSTSCRIPT:
			case FPRINTEDPOSTSCRIPT:
				err = io_writepostscriptlibrary(lib,
					(BOOLEAN)(format == FPRINTEDPOSTSCRIPT));
				break;

#if defined(MACOS) && !defined(MACOSX)
			case FQUICKDRAW:
				err = io_writequickdrawlibrary(lib);
				break;
#endif

#ifdef FORCECADENCE
			case FSKILL:
				err = io_writeskilllibrary(lib);
				break;
#endif

			case FTEXT:
				err = io_writetextlibrary(lib);
				break;

			default:
				for(i=0; io_formatlist[i].name != 0; i++)
					if (io_formatlist[i].bits == format)
				{
					ttyputerr(M_("Cannot write %s files"), io_formatlist[i].name);
					return(1);
				}
		}

		/* announce the ending of library output */
		if (err) return(1);
		for(i=0; i<el_maxtools; i++)
			if (el_tools[i].writelibrary != 0)
				(*el_tools[i].writelibrary)(lib, TRUE);
		return(0);
	}

	if (namesame(command, x_("read")) == 0)
	{
		arg3 = va_arg(ap, INTBIG);
		io_libinputrecursivedepth = io_libinputreadmany = 0;
		io_libraryannouncecount = 0;
		switch (format)
		{
			case FBINARY:
				err = io_readbinlibrary(lib);
				/* make sure that the lambda values are consistent */
				if (!err) io_unifylambdavalues(lib);
				break;

			case FCIF:
				err = io_readciflibrary(lib);
				break;

			case FDEF:
				err = io_readdeflibrary(lib);
				break;

			case FDXF:
				err = io_readdxflibrary(lib);
				break;

			case FEDIF:
				err = io_readediflibrary(lib);
				break;

			case FGDS:
				err = io_readgdslibrary(lib, arg3);
				break;

			case FLEF:
				err = io_readleflibrary(lib);
				break;

			case FSDF:
				err = io_readsdflibrary(lib);
				break;

			case FSUE:
				err = io_readsuelibrary(lib);
				break;

			case FTEXT:
				err = io_readtextlibrary(lib);
				/* make sure that the lambda values are consistent */
				if (!err) io_unifylambdavalues(lib);
				break;

#if VHDLTOOL
			case FVHDL:
				err = io_readvhdllibrary(lib);
				break;
#endif

			default:
				for(i=0; io_formatlist[i].name != 0; i++)
					if (io_formatlist[i].bits == format)
				{
					ttyputerr(M_("Cannot read %s files"), io_formatlist[i].name);
					return(1);
				}
		}

		/* announce the completion of library input */
		if (err) return(1);
		io_queuereadlibraryannouncement(lib);
		for(i=0; i<el_maxtools; i++)
		{
			if (el_tools[i].readlibrary != 0)
			{
				for(j=0; j<io_libraryannouncecount; j++)
					(*el_tools[i].readlibrary)(io_libraryannounce[j]);
			}
		}
		return(0);
	}
	return(-1);
}

void io_queuereadlibraryannouncement(LIBRARY *lib)
{
	REGISTER INTBIG i, newtotal;
	REGISTER LIBRARY **newlist;

	if (io_libraryannouncecount >= io_libraryannouncetotal)
	{
		newtotal = io_libraryannouncetotal * 2;
		if (io_libraryannouncecount >= newtotal)
			newtotal = io_libraryannouncecount + 5;
		newlist = (LIBRARY **)emalloc(newtotal * (sizeof (LIBRARY *)), io_tool->cluster);
		if (newlist == 0) return;
		for(i=0; i<io_libraryannouncecount; i++)
			newlist[i] = io_libraryannounce[i];
		if (io_libraryannouncetotal > 0)
			efree((CHAR *)io_libraryannounce);
		io_libraryannounce = newlist;
		io_libraryannouncetotal = newtotal;
	}
	io_libraryannounce[io_libraryannouncecount++] = lib;
}

/******************************* BLOATING *******************************/

/*
 * routine to indicate that CIF layer "layer" is to be bloated by "amount"
 */
void io_setoutputbloat(CHAR *layer, INTBIG amount)
{
	REGISTER BLOAT *bl;

	/* special case when bloating the null layer */
	if (*layer == 0)
	{
		for(bl = io_curbloat; bl != NOBLOAT; bl = bl->nextbloat)
			ttyputmsg(M_("Bloating layer %s by %ld %ss"), bl->layer, bl->amount, unitsname(el_units));
		return;
	}

	/* first see if this layer is already being bloated */
	for(bl = io_curbloat; bl != NOBLOAT; bl = bl->nextbloat)
		if (namesame(bl->layer, layer) == 0)
	{
		if (bl->amount == amount)
		{
			ttyputmsg(M_("Layer %s is already being bloated by %ld %ss"),
				bl->layer, amount, unitsname(el_units));
			return;
		}
		ttyputmsg(M_("Layer %s was being bloated by %ld %ss, now by %ld"),
			bl->layer, bl->amount, unitsname(el_units), amount);
		bl->amount = amount;
		return;
	}

	bl = (BLOAT *)emalloc(sizeof (BLOAT), io_tool->cluster);
	if (bl == 0)
	{
		ttyputnomemory();
		return;
	}
	(void)allocstring(&bl->layer, layer, io_tool->cluster);
	bl->amount = amount;
	bl->nextbloat = io_curbloat;
	io_curbloat = bl;
	ttyputmsg(M_("Layer %s will be bloated by %ld %ss"), layer, amount, unitsname(el_units));
}

/*
 * routine to tell the amount to bloat layer "lay"
 */
INTBIG io_getoutputbloat(CHAR *layer)
{
	REGISTER BLOAT *bl;

	for(bl = io_curbloat; bl != NOBLOAT; bl = bl->nextbloat)
		if (namesame(bl->layer, layer) == 0) return(bl->amount);
	return(0);
}

/******************************* LIBRARY REPAIR *******************************/

#define OLDSOURCEDCV               0		/* Source is DC Voltage */
#define OLDSOURCEAC          0400000		/* Source is AC analysis */
#define OLDSOURCEBULK       01000000		/* Source is Bulk */
#define OLDSOURCEDCC        01400000		/* Source is DC Current */
#define OLDSOURCECURMTR     02000000		/* Source is Current meter */
#define OLDSOURCENODE       02400000		/* Source is Nodeset */
#define OLDSOURCESPEC       03000000		/* Source is Special */
#define OLDSOURCETRAN       03400000		/* Source is Transient Analysis */
#define OLDSOURCEDCAN       04000000		/* Source is DC Analysis */
#define OLDSOURCEEXT        04400000		/* Source is Extension */

/*
 * routine to complete formation of library "lib" that has just been read from
 * disk.  Some data is not read, but computed from other data.
 */
void io_fixnewlib(LIBRARY *lib, void *dia)
{
	REGISTER NODEPROTO *np, *pnt, *onp;
	REGISTER NODEINST *ni, *newni;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *olib;
	REGISTER BOOLEAN first, changed;
	REGISTER INTBIG libunits, i, j, lam, oldtype, size, len, lambda,
		celllambda, totlx, tothx, totly, tothy, numchanged, *intkey;
	INTBIG lx, ly, hx, hy, major, minor, detail, type, addr, xoff, yoff, growx, growy,
		shiftx, shifty;
	UINTBIG olddescript[TEXTDESCRIPTSIZE];
	REGISTER CHAR *str, *pt, **newlist;
	float value;
	XARRAY trans;
	static POLYGON *poly = NOPOLYGON;
	static INTBIG sch_flipfloptypekey = 0;
	static INTBIG sch_transistortypekey = 0;
	static INTBIG sch_sourcekey = 0;
	static INTBIG sch_twoportkey = 0;
	CHAR line[200], *par[4], *newname;
	REGISTER VARIABLE *var, *newvar;
	REGISTER TECHNOLOGY *tech;
	REGISTER PORTPROTO *pp, *inpp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	extern ARCPROTO **gen_upconn;
	REGISTER void *infstr;

	/* make sure units are correct */
	libunits = ((lib->userbits & LIBUNITS) >> LIBUNITSSH) << INTERNALUNITSSH;
	if ((libunits & INTERNALUNITS) != (el_units & INTERNALUNITS))
	{
		ttyputmsg(_("Converting library from %s units to %s units"), unitsname(libunits),
			unitsname(el_units));
		changeinternalunits(lib, libunits, el_units);
	}

	/* check validity of instances */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			if (olib == np->lib) break;
		if (olib == NOLIBRARY)
		{
			ttyputerr(_("ERROR: Cell %s: has invalid cell structure"), describenodeproto(np));
			continue;
		}
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex != 0) continue;
			olib = ni->proto->lib;
			if (olib == lib) continue;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				if (olib == ni->proto->lib) break;
			if (olib == NOLIBRARY)
			{
				ttyputerr(_("ERROR: Cell %s: has instance from invalid library"), describenodeproto(np));
				ni->proto = gen_univpinprim;
				continue;
			}
			for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				if (onp == ni->proto) break;
			if (onp == NONODEPROTO)
			{
				ttyputerr(_("ERROR: Cell %s: has instance with invalid prototype"), describenodeproto(np));
				ni->proto = gen_univpinprim;
				continue;
			}
		}
	}

	/* create name hash table */
	db_buildnodeprotohashtable(lib);

	/* create port name hash tables */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		db_buildportprotohashtable(np);

	/* adjust every cell in the library */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			for(pe = pp->subnodeinst->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				if (pe->proto == pp->subportproto)
		{
			pp->subportexpinst = pe;  break;
		}
#ifdef MACOS
		/*
		 * The Macintosh uses a different date epoch than other machines, and this
		 * is handled in "dbtext.c".  However, the adjustment code is recent,
		 * and some old Macintosh libraries may have incorrect date information.
		 *
		 * The Macintosh epoch is 1904, but every other machine uses 1970.
		 * Therefore, the Macintosh numbers are 66 years larger.
		 *
		 * To check for bogus date information, see if the date is larger
		 * than the constant 0x9FE3EB1F.  This constant is 1989 in Mac time
		 * but is 2055 elsewhere (1989 is the earliest possible incarnation
		 * of Electric on the Mac).  If the date is that large, then either it
		 * is 2055 and Macs still exist (unlikely) or the date is bad and needs
		 * to have 66 years (0x7C254E10) subtracted from it.
		 */
		if (np->creationdate > 0x9FE3EB1F) np->creationdate -= 0x7C254E10;
		if (np->revisiondate > 0x9FE3EB1F) np->revisiondate -= 0x7C254E10;
#endif
	}

	/* look for arrayed node specifications */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (!isschematicview(np)) continue;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			net_setnodewidth(ni);
		}
	}

	/*
	 * see if this is version 4 or earlier...
	 * In version 4 and earlier, the basic unit was the centimicron and rotation
	 * was specified in degrees.  From version 5 and up, the basic unit is scaled
	 * by 20 to the half-nanometer and the rotation fields are scaled by 10 to
	 * tenth-degrees.  Also, must convert some variable names.
	 */
	io_getversion(lib, &major, &minor, &detail);

	/* this is only done after all libraries are read */
	if (io_libinputrecursivedepth == 0)
	{
		/* now fill in the connection lists on the ports */
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			inpp = pp;
			for(;;)
			{
				if (inpp == NOPORTPROTO) break;
				if (inpp->parent->primindex != 0) break;
				inpp = inpp->subportproto;
			}
			if (inpp != NOPORTPROTO)
				pp->connects = inpp->connects;
			if (pp->connects == 0)
			{
				if (gen_upconn[0] != 0) pp->connects = gen_upconn; else
					pp->connects = &gen_upconn[1];
				pp->subportproto = pp->subnodeinst->proto->firstportproto;
			}
		}

		/* rebuild list of node instances */
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->firstinst = NONODEINST;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->firstinst = NONODEINST;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			/* examine every nodeinst in the cell */
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				/* compute linked list of node instances */
				pnt = ni->proto;
				if (pnt == NONODEPROTO)
				{
					ttyputerr(_("Cell %s: has a node without a prototype"), describenodeproto(np));
					ni->proto = gen_univpinprim;
				}
				if (pnt->firstinst != NONODEINST) pnt->firstinst->previnst = ni;
				ni->nextinst = pnt->firstinst;
				ni->previnst = NONODEINST;
				pnt->firstinst = ni;
			}
		}

		/* for versions 8.00 or later, look for arcs with negation in other "head" bit */
		if (major >= 8)
		{
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					if ((ai->userbits&ISHEADNEGATED) != 0)
					{
						if ((ai->userbits&ISNEGATED) != 0)
						{
							ttyputmsg("Cell %s, arc %s is negated at both ends which is unsupported in this version of Electric.  Ignoring head negation.",
								describenodeproto(np), describearcinst(ai));
						} else
						{
							/* only the head is negated: reverse the arc */
							if ((ai->userbits&REVERSEEND) != 0)
								ai->userbits &= ~REVERSEEND; else
									ai->userbits |= REVERSEEND;
							ai->userbits |= ISNEGATED;
							ai->userbits &= ~ISHEADNEGATED;
						}
					}
				}
			}
		}

		/* for versions before 6.05bc, look for icon cells that are wrong */
		if (major < 6 ||
			(major == 6 && minor < 5) ||
			(major == 6 && minor == 5 && detail < 55))
		{
			numchanged = 0;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				first = TRUE;
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					boundobj(ni->geom, &lx, &hx, &ly, &hy);
					if (first)
					{
						first = FALSE;
						totlx = lx;   tothx = hx;
						totly = ly;   tothy = hy;
					} else
					{
						if (lx < totlx) totlx = lx;
						if (hx > tothx) tothx = hx;
						if (ly < totly) totly = ly;
						if (hy > tothy) tothy = hy;
					}
				}
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					boundobj(ai->geom, &lx, &hx, &ly, &hy);
					if (lx < totlx) totlx = lx;
					if (hx > tothx) tothx = hx;
					if (ly < totly) totly = ly;
					if (hy > tothy) tothy = hy;
				}
				if (first) continue;
				if (np->lowx == totlx && np->highx == tothx &&
					np->lowy == totly && np->highy == tothy) continue;

				for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
				{
					growx = ((tothx - np->highx) + (totlx - np->lowx)) / 2;
					growy = ((tothy - np->highy) + (totly - np->lowy)) / 2;
					if (ni->transpose != 0)
					{
						makeangle(ni->rotation, ni->transpose, trans);
					} else
					{
						makeangle((3600 - ni->rotation)%3600, 0, trans);
					}
					xform(growx, growy, &shiftx, &shifty, trans);
					ni->lowx += totlx - np->lowx - growx + shiftx;
					ni->highx += tothx - np->highx - growx + shiftx;
					ni->lowy += totly - np->lowy - growy + shifty;
					ni->highy += tothy - np->highy - growy + shifty;
					updategeom(ni->geom, ni->parent);
				}
				np->lowx = totlx;   np->highx = tothx;
				np->lowy = totly;   np->highy = tothy;
				numchanged++;
			}
			if (numchanged != 0)
			{
				ttyputmsg(_("WARNING: due to changes in bounds calculation, %ld cell sizes were adjusted"),
					numchanged);
				ttyputmsg(_("...should do Check&Repair library"));
			}
		}

		/* create network information */
		if (dia != 0) DiaSetTextProgress(dia, _("Building network data..."));
		if (io_libinputreadmany > 0)
		{
			/* many libraries read: renumber everything */
			(void)asktool(net_tool, x_("total-re-number"));
		} else
		{
			/* just 1 library read: renumber it */
			(void)asktool(net_tool, x_("library-re-number"), (INTBIG)lib);
		}
	}

	/* set the cell's technology */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->tech = whattech(np);

	/* check for incorrect layer counts on stored variables */
	io_fixtechlayers(lib);

	/* for versions 7.01 or later, convert floating-point outline information */
	if (major >= 7 || (major == 7 && minor >= 1))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VFLOAT|VISARRAY, el_trace_key);
				if (var == NOVARIABLE) continue;
				len = getlength(var);
				intkey = (INTBIG *)emalloc(len*SIZEOFINTBIG, el_tempcluster);
				lambda = lambdaofnode(ni);
				for(i=0; i<len; i++)
				{
					intkey[i] = (int)(((float *)var->addr)[i] * lambda);
				}
				setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)intkey,
					VINTEGER|VISARRAY|(len<<VLENGTHSH));
			}
		}
	}

	/* for versions 6.02 or earlier, convert text sizes */
	if (major < 6 || (major == 6 && minor <= 2))
	{
#ifdef REPORTCONVERSION
		ttyputmsg(x_("   Converting text sizes"));
#endif
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				io_convertallrelativetext(ni->numvar, ni->firstvar);
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					io_convertallrelativetext(pi->numvar, pi->firstvar);
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					io_convertallrelativetext(pe->numvar, pe->firstvar);
				if (ni->proto->primindex == 0)
					io_convertrelativetext(ni->textdescript);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				io_convertallrelativetext(ai->numvar, ai->firstvar);
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				io_convertallrelativetext(pp->numvar, pp->firstvar);
				io_convertrelativetext(pp->textdescript);
			}
			io_convertallrelativetext(np->numvar, np->firstvar);
		}
		io_convertallrelativetext(lib->numvar, lib->firstvar);
	}

	/* for versions before 6.04c, convert text descriptor values */
	if (major < 6 ||
		(major == 6 && minor < 4) ||
		(major == 6 && minor == 4 && detail < 3))
	{
#ifdef REPORTCONVERSION
		ttyputmsg(x_("   Converting text descriptors"));
#endif
		io_converteverytextdescriptor(lib, 0);
	}

	/* for versions before 6.03r, convert variables on schematic primitives */
	if (major < 6 ||
		(major == 6 && minor < 3) ||
		(major == 6 && minor == 3 && detail < 18))
	{
		if (sch_transistortypekey == 0)
			sch_transistortypekey = makekey(x_("SCHEM_transistor_type"));
		if (sch_sourcekey == 0)
			sch_sourcekey = makekey(x_("SCHEM_source"));
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech == sch_tech)
				{
					if (ni->proto == sch_transistorprim)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_transistortypekey);
						if (var != NOVARIABLE)
						{
							str = (CHAR *)var->addr;
							if (namesamen(str, x_("nmos"), 4) == 0)
							{
								ni->userbits |= TRANNMOS;    str += 4;
							} else if (namesamen(str, x_("dmos"), 4) == 0)
							{
								ni->userbits |= TRANDMOS;    str += 4;
							} else if (namesamen(str, x_("pmos"), 4) == 0)
							{
								ni->userbits |= TRANPMOS;    str += 4;
							} else if (namesamen(str, x_("npn"), 3) == 0)
							{
								ni->userbits |= TRANNPN;     str += 3;
							} else if (namesamen(str, x_("pnp"), 3) == 0)
							{
								ni->userbits |= TRANPNP;     str += 3;
							} else if (namesamen(str, x_("njfet"), 5) == 0)
							{
								ni->userbits |= TRANNJFET;   str += 5;
							} else if (namesamen(str, x_("pjfet"), 5) == 0)
							{
								ni->userbits |= TRANPJFET;   str += 5;
							} else if (namesamen(str, x_("dmes"), 4) == 0)
							{
								ni->userbits |= TRANDMES;    str += 4;
							} else if (namesamen(str, x_("emes"), 4) == 0)
							{
								ni->userbits |= TRANEMES;    str += 4;
							}
							if (*str == 0) (void)delvalkey((INTBIG)ni, VNODEINST, sch_transistortypekey); else
							{
								TDCOPY(olddescript, var->textdescript);
								infstr = initinfstr();
								addstringtoinfstr(infstr, str);
								allocstring(&newname, returninfstr(infstr), el_tempcluster);
								var = setvalkey((INTBIG)ni, VNODEINST, sch_transistortypekey,
									(INTBIG)newname, var->type);
								efree(newname);
								if (var != NOVARIABLE)
									TDCOPY(var->textdescript, olddescript);
							}
						}
					} else if (ni->proto == sch_sourceprim)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_sourcekey);
						if (var != NOVARIABLE)
						{
							str = (CHAR *)var->addr;
							if (namesamen(str, x_("vd"), 2) == 0)
							{
								ni->userbits |= OLDSOURCEDCAN;   str += 2;
							} else if (namesamen(str, x_("v"), 1) == 0)
							{
								ni->userbits |= OLDSOURCEDCV;   str++;
							} else if (namesamen(str, x_("cm"), 2) == 0)
							{
								ni->userbits |= OLDSOURCECURMTR;   str += 2;
							} else if (namesamen(str, x_("c"), 1) == 0)
							{
								ni->userbits |= OLDSOURCEDCC;   str++;
							} else if (namesamen(str, x_("t"), 1) == 0)
							{
								ni->userbits |= OLDSOURCETRAN;   str++;
							} else if (namesamen(str, x_("a"), 1) == 0)
							{
								ni->userbits |= OLDSOURCEAC;   str++;
							} else if (namesamen(str, x_("n"), 1) == 0)
							{
								ni->userbits |= OLDSOURCENODE;   str++;
							} else if (namesamen(str, x_("x"), 1) == 0)
							{
								ni->userbits |= OLDSOURCEEXT;   str++;
							} else if (namesamen(str, x_("b"), 1) == 0)
							{
								ni->userbits |= OLDSOURCEBULK;   str++;
							} else if (namesamen(str, x_("s"), 1) == 0)
							{
								ni->userbits |= OLDSOURCESPEC;   str++;
							}
							if (*str == '/') str++;
							if (*str == 0) (void)delvalkey((INTBIG)ni, VNODEINST, sch_sourcekey); else
							{
								TDCOPY(olddescript, var->textdescript);
								infstr = initinfstr();
								addstringtoinfstr(infstr, str);
								allocstring(&newname, returninfstr(infstr), el_tempcluster);
								var = setvalkey((INTBIG)ni, VNODEINST, sch_sourcekey,
									(INTBIG)newname, var->type|VDISPLAY);
								efree(newname);
								if (var != NOVARIABLE)
									TDCOPY(var->textdescript, olddescript);
							}
						}
					} else if (ni->proto == sch_diodeprim)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_diodekey);
						if (var != NOVARIABLE)
						{
							str = (CHAR *)var->addr;
							while (*str == ' ' || *str == '\t') str++;
							if (tolower(*str) == 'z')
							{
								str++;
								ni->userbits |= DIODEZENER;
							}
							if (*str == 0) (void)delvalkey((INTBIG)ni, VNODEINST, sch_diodekey); else
							{
								TDCOPY(olddescript, var->textdescript);
								infstr = initinfstr();
								addstringtoinfstr(infstr, str);
								allocstring(&newname, returninfstr(infstr), el_tempcluster);
								var = setvalkey((INTBIG)ni, VNODEINST, sch_diodekey,
									(INTBIG)newname, var->type|VDISPLAY);
								efree(newname);
								if (var != NOVARIABLE)
									TDCOPY(var->textdescript, olddescript);
							}
						}
					} else if (ni->proto == sch_capacitorprim)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_capacitancekey);
						if (var != NOVARIABLE)
						{
							str = (CHAR *)var->addr;
							while (*str == ' ' || *str == '\t') str++;
							if (tolower(*str) == 'e')
							{
								str++;
								ni->userbits |= CAPACELEC;
							}
							if (*str == 0) (void)delvalkey((INTBIG)ni, VNODEINST, sch_capacitancekey); else
							{
								TDCOPY(olddescript, var->textdescript);
								infstr = initinfstr();
								addstringtoinfstr(infstr, str);
								allocstring(&newname, returninfstr(infstr), el_tempcluster);
								var = setvalkey((INTBIG)ni, VNODEINST, sch_capacitancekey,
									(INTBIG)newname, var->type|VDISPLAY);
								efree(newname);
								if (var != NOVARIABLE)
									TDCOPY(var->textdescript, olddescript);
							}
						}
					} else if (ni->proto == sch_twoportprim)
					{
						if (sch_twoportkey == 0)
							sch_twoportkey = makekey(x_("SCHEM_twoport_type"));
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_twoportkey);
						if (var != NOVARIABLE)
						{
							str = (CHAR *)var->addr;
							while (*str == ' ') str++;
							switch (tolower(*str))
							{
								case 'u': ni->userbits |= TWOPVCVS;    str++;   break;
								case 'g': ni->userbits |= TWOPVCCS;    str++;   break;
								case 'h': ni->userbits |= TWOPCCVS;    str++;   break;
								case 'f': ni->userbits |= TWOPCCCS;    str++;   break;
								case 'l': ni->userbits |= TWOPTLINE;   str++;   break;
							}
							nextchangequiet();
							if (*str == 0) (void)delvalkey((INTBIG)ni, VNODEINST, sch_twoportkey); else
							{
								TDCOPY(olddescript, var->textdescript);
								infstr = initinfstr();
								addstringtoinfstr(infstr, str);
								allocstring(&newname, returninfstr(infstr), el_tempcluster);
								var = setvalkey((INTBIG)ni, VNODEINST, sch_spicemodelkey,
									(INTBIG)newname, var->type|VDISPLAY);
								efree(newname);
								if (var != NOVARIABLE)
									TDCOPY(var->textdescript, olddescript);
							}
						}
					} else if (ni->proto == sch_ffprim)
					{
						if (sch_flipfloptypekey == 0)
							sch_flipfloptypekey = makekey(x_("SCHEM_flipflop_type"));
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_flipfloptypekey);
						if (var != NOVARIABLE)
						{
							for (str = (CHAR *)var->addr; *str != 0; str++)
							{
								if (namesamen(str, x_("rs"), 2) == 0)
								{
									ni->userbits |= FFTYPERS;
									str++;
									continue;
								}
								if (namesamen(str, x_("jk"), 2) == 0)
								{
									ni->userbits |= FFTYPEJK;
									str++;
									continue;
								}
								if (namesamen(str, x_("t"), 1) == 0)
								{
									ni->userbits |= FFTYPET;
									continue;
								}
								if (namesamen(str, x_("d"), 1) == 0)
								{
									ni->userbits |= FFTYPED;
									continue;
								}
								if (namesamen(str, x_("ms"), 2) == 0)
								{
									ni->userbits |= FFCLOCKMS;
									str++;
									continue;
								}
								if (namesamen(str, x_("p"), 1) == 0)
								{
									ni->userbits |= FFCLOCKP;
									continue;
								}
								if (namesamen(str, x_("n"), 1) == 0)
								{
									ni->userbits |= FFCLOCKN;
									continue;
								}
							}
							nextchangequiet();
							(void)delvalkey((INTBIG)ni, VNODEINST, sch_flipfloptypekey);
						}
					}
				}
			}
		}
	}

	/* for versions before 6.05bh, make sure that "far text" bit is set right */
	if (io_libinputrecursivedepth == 0)
	{
		if (major < 6 ||
			(major == 6 && minor < 5) ||
			(major == 6 && minor == 5 && detail < 60))
		{
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					us_computenodefartextbit(ni);
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					us_computearcfartextbit(ai);
			}
		}
	}

	/* for versions before 6.05bi, make all cell centers be "visible only inside cell" */
	if (major < 6 ||
		(major == 6 && minor < 5) ||
		(major == 6 && minor == 5 && detail < 61))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto != gen_cellcenterprim) continue;
				ni->userbits |= NVISIBLEINSIDE;
			}
		}
	}

	/* for versions before 6.05ba, convert source nodes */
	if (major < 6 ||
		(major == 6 && minor < 5) ||
		(major == 6 && minor == 5 && detail < 53))
	{
		/* change all Wire_Pins that are 1x1 to be 0.5x0.5 */
		numchanged = 0;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech != sch_tech) continue;
				if (ni->proto != sch_wirepinprim) continue;
				lambda = lambdaofnode(ni);
				if (ni->highx - ni->lowx == lambda &&
					ni->highy - ni->lowy == lambda)
				{
					modifynodeinst(ni, lambda/4, lambda/4, -lambda/4, -lambda/4, 0, 0);
					numchanged++;
				}
			}
		}
		if (numchanged != 0)
			ttyputmsg(_("Note: reduced the size of %ld schematic wire pins from 1x1 to 0.5x0.5"),
				numchanged);
	}

	/* for versions before 6.05al, convert source nodes */
	if (major < 6 ||
		(major == 6 && minor < 5) ||
		(major == 6 && minor == 5 && detail < 38))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			changed = FALSE;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech != sch_tech) continue;
				if (ni->proto != sch_sourceprim) continue;
				str = 0;
				switch (ni->userbits&NTECHBITS)
				{
					case OLDSOURCEDCAN:   str = x_("AnalysisDC");         break;
					case OLDSOURCEDCV:    str = x_("DCVoltage");          break;
					case OLDSOURCEDCC:    str = x_("DCCurrent");          break;
					case OLDSOURCETRAN:   str = x_("AnalysisTransient");  break;
					case OLDSOURCEAC:     str = x_("AnalysisAC");         break;
					case OLDSOURCENODE:   str = x_("NodeSet");            break;
					case OLDSOURCEEXT:    str = x_("Extension");          break;
				}
				if (str == 0) continue;

				/* make sure the "spiceparts" library is read */
				olib = getlibrary(x_("spiceparts"));
				if (olib == NOLIBRARY)
				{
					par[0] = x_("library");
					par[1] = x_("read");
					par[2] = x_("spiceparts.txt");
					par[3] = x_("text");
					telltool(us_tool, 4, par);
					olib = getlibrary(x_("spiceparts"));
					if (olib == NOLIBRARY)
					{
						ttyputerr(_("Cannot find spice parts library for source conversion"));
						break;
					}
				}
				esnprintf(line, 200, x_("spiceparts:%s"), str);
				onp = getnodeproto(line);
				if (onp == NONODEPROTO)
				{
					ttyputerr(_("Cannot find '%s' for source conversion"), line);
					break;
				}
				newni = replacenodeinst(ni, onp, TRUE, TRUE);
				if (newni != NONODEINST)
					newni->userbits |= NEXPAND;
				changed = TRUE;
			}
			if (ni != NONODEINST) break;
			if (changed)
				ttyputmsg(_("Warning: SPICE components converted in cell %s.  Additional editing may be required."),
					describenodeproto(np));
		}
	}

	/* for versions before 6.07ap, make parameters visible by default  */
	if (major < 6 ||
		(major == 6 && minor < 7) ||
		(major == 6 && minor == 7 && detail < 26+16))
	{
		j = 0;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(i=0; i<np->numvar; i++)
			{
				var = &np->firstvar[i];
				if (TDGETISPARAM(var->textdescript) == 0) continue;
				if (TDGETINTERIOR(var->textdescript) == 0) continue;
				TDSETINTERIOR(var->textdescript, 0);
				j++;
			}
		}
		if (j > 0)
		{
			ttyputmsg(x_("Library %s: cleaned up %ld old parameter visibility settings"),
				lib->libname, j);
			lib->userbits |= LIBCHANGEDMINOR;
		}
	}

	/* for versions before 6.07b, adjust text  */
	if (major < 6 ||
		(major == 6 && minor < 7) ||
		(major == 6 && minor == 7 && detail < 2))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			tech = np->tech;
			if (tech == NOTECHNOLOGY) tech = whattech(np);
			celllambda = lib->lambda[tech->techindex];
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				onp = ni->proto;
				tech = onp->tech;
				if (tech == NOTECHNOLOGY) tech = whattech(onp);
				lambda = lib->lambda[tech->techindex];
				if (lambda == celllambda) continue;
				for(i=0; i<ni->numvar; i++)
				{
					var = &ni->firstvar[i];
					if ((var->type & VDISPLAY) == 0) continue;
					xoff = muldiv(TDGETXOFF(var->textdescript), lambda, celllambda);
					yoff = muldiv(TDGETYOFF(var->textdescript), lambda, celllambda);
					TDSETOFF(var->textdescript, xoff, yoff);
				}
				us_computenodefartextbit(ni);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				tech = ai->proto->tech;
				lambda = lib->lambda[tech->techindex];
				if (lambda == celllambda) continue;
				for(i=0; i<ai->numvar; i++)
				{
					var = &ai->firstvar[i];
					if ((var->type & VDISPLAY) == 0) continue;
					xoff = muldiv(TDGETXOFF(var->textdescript), lambda, celllambda);
					yoff = muldiv(TDGETYOFF(var->textdescript), lambda, celllambda);
					TDSETOFF(var->textdescript, xoff, yoff);
				}
				us_computearcfartextbit(ai);
			}
		}
	}

	/* for versions before 6.04o, separate transistor size and SPICE info */
	if (major < 6 ||
		(major == 6 && minor < 4) ||
		(major == 6 && minor == 4 && detail < 15))
	{
		if (sch_transistortypekey == 0)
			sch_transistortypekey = makekey(x_("SCHEM_transistor_type"));
		if (sch_sourcekey == 0)
			sch_sourcekey = makekey(x_("SCHEM_source"));
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech != sch_tech) continue;

				/* convert "SCHEM_source" to "SIM_spice_model" */
				var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_sourcekey);
				if (var != NOVARIABLE)
				{
					TDCOPY(olddescript, var->textdescript);
					newvar = setvalkey((INTBIG)ni, VNODEINST, sch_spicemodelkey, var->addr,
						var->type);
					if (newvar != 0)
						TDCOPY(newvar->textdescript, olddescript);
					(void)delvalkey((INTBIG)ni, VNODEINST, sch_sourcekey);
				}

				if (ni->proto != sch_transistorprim &&
					ni->proto != sch_transistor4prim) continue;
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_transistortypekey);
				if (var == NOVARIABLE) continue;

				/* look for "/" and split into Width and Length */
				str = (CHAR *)var->addr;
				oldtype = var->type;
				TDCOPY(olddescript, var->textdescript);
				for(pt = str; *pt != 0; pt++) if (*pt == '/') break;
				if (*pt == '/')
				{
					*pt++ = 0;

					/* determine separation of descriptors */
					us_getlenwidoffset(ni, olddescript, &xoff, &yoff);
					getsimpletype(str, &type, &addr, 0);
					type |= oldtype & (VCODE1 | VCODE2 | VDISPLAY | VDONTSAVE);
					newvar = setvalkey((INTBIG)ni, VNODEINST, el_attrkey_width, addr, type);
					if (newvar != NOVARIABLE)
					{
						TDCOPY(newvar->textdescript, olddescript);
						TDSETOFF(newvar->textdescript, TDGETXOFF(newvar->textdescript)+xoff,
							TDGETYOFF(newvar->textdescript)+yoff);
					}

					str = pt;
					for(pt = str; *pt != 0; pt++) if (*pt == '/') break;
					if (*pt == '/') *pt++ = 0;
					getsimpletype(str, &type, &addr, 0);
					type |= oldtype & (VCODE1 | VCODE2 | VDISPLAY | VDONTSAVE);
					newvar = setvalkey((INTBIG)ni, VNODEINST, el_attrkey_length, addr, type);
					if (newvar != NOVARIABLE)
					{
						TDCOPY(newvar->textdescript, olddescript);
						size = TDGETSIZE(newvar->textdescript);
						i = TXTGETPOINTS(size);
						if (i > 3) size = TXTSETPOINTS(i-2); else
						{
							i = TXTGETQLAMBDA(size);
							if (i > 3) size = TXTSETQLAMBDA(i-2);
						}
						TDSETSIZE(newvar->textdescript, size);
						TDSETOFF(newvar->textdescript, TDGETXOFF(newvar->textdescript)-xoff,
							TDGETYOFF(newvar->textdescript)-yoff);
					}

					/* see if a SPICE model is at the end */
					if (*pt != 0)
					{
						type = VSTRING | (oldtype & (VCODE1 | VCODE2 | VDISPLAY | VDONTSAVE));
						newvar = setvalkey((INTBIG)ni, VNODEINST, sch_spicemodelkey,
							(INTBIG)pt, type);
						if (newvar != NOVARIABLE)
							TDCOPY(newvar->textdescript, olddescript);
					}
				} else
				{
					/* single number: just save Area */
					getsimpletype(str, &type, &addr, 0);
					type |= oldtype & (VCODE1 | VCODE2 | VDISPLAY | VDONTSAVE);
					newvar = setvalkey((INTBIG)ni, VNODEINST, el_attrkey_area, addr, type);
					if (newvar != NOVARIABLE)
						TDCOPY(newvar->textdescript, olddescript);
				}
				(void)delvalkey((INTBIG)ni, VNODEINST, sch_transistortypekey);
			}
		}
	}

	/* for versions before 6.03g, convert schematic and mocmossub primitives */
	if (major < 6 ||
		(major == 6 && minor < 3) ||
		(major == 6 && minor == 3 && detail < 7))
	{
#ifdef REPORTCONVERSION
		ttyputmsg(x_("   Converting schematic and MOSIS CMOS primitives"));
#endif
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech == sch_tech)
				{
					if (ni->proto == sch_pwrprim)
					{
						lam = lib->lambda[sch_tech->techindex];
						io_fixupnodeinst(ni, lam/2, lam/2, -lam/2, -lam/2);
					} else if (ni->proto == sch_gndprim)
					{
						lam = lib->lambda[sch_tech->techindex];
						io_fixupnodeinst(ni, lam/2, 0, -lam/2, 0);
					} else if (ni->proto == sch_capacitorprim)
					{
						lam = lib->lambda[sch_tech->techindex];
						io_fixupnodeinst(ni, lam/2, 0, -lam/2, 0);
					} else if (ni->proto == sch_resistorprim)
					{
						lam = lib->lambda[sch_tech->techindex];
						io_fixupnodeinst(ni, 0, lam/2, 0, -lam/2);
					}
				} else if (ni->proto->tech == mocmos_tech)
				{
					if (ni->proto == mocmos_metal1metal2prim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, -lam/2, -lam/2, lam/2, lam/2);
					} else if (ni->proto == mocmos_metal4metal5prim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, -lam/2, -lam/2, lam/2, lam/2);
					} else if (ni->proto == mocmos_metal5metal6prim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, -lam, -lam, lam, lam);
					} else if (ni->proto == mocmos_ptransistorprim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, 0, -lam, 0, lam);
					} else if (ni->proto == mocmos_ntransistorprim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, 0, -lam, 0, lam);
					} else if (ni->proto == mocmos_metal1poly2prim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, -lam*3, -lam*3, lam*3, lam*3);
					} else if (ni->proto == mocmos_metal1poly12prim)
					{
						lam = lib->lambda[mocmos_tech->techindex];
						io_fixupnodeinst(ni, -lam*4, -lam*4, lam*4, lam*4);
					}
				}
			}
		}
	}

	/* for versions before 6.03l, convert mocmossub via4-via6 */
	if (major < 6 ||
		(major == 6 && minor < 3) ||
		(major == 6 && minor == 3 && detail < 12))
	{
#ifdef REPORTCONVERSION
		ttyputmsg(x_("   Converting MOSIS CMOS vias"));
#endif
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech != mocmos_tech) continue;
				if (namesame(ni->proto->protoname, x_("Metal-3-Metal-4-Con")) == 0 ||
					namesame(ni->proto->protoname, x_("Metal-4-Metal-5-Con")) == 0 ||
					namesame(ni->proto->protoname, x_("Metal-5-Metal-6-Con")) == 0)
				{
					lam = lib->lambda[mocmos_tech->techindex];
					io_fixupnodeinst(ni, -lam, -lam, lam, lam);
				}
			}
		}
	}

	/* for versions before 6.03aa, convert mocmossub well contacts */
	if (major < 6 ||
		(major == 6 && minor < 3) ||
		(major == 6 && minor == 3 && detail < 27))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				if (ni->proto->tech != mocmos_tech) continue;
				if (namesame(ni->proto->protoname, x_("Metal-1-N-Well-Con")) == 0 ||
					namesame(ni->proto->protoname, x_("Metal-1-P-Well-Con")) == 0)
				{
					lam = lib->lambda[mocmos_tech->techindex];
					io_fixupnodeinst(ni, -lam-lam/2, -lam-lam/2, lam+lam/2, lam+lam/2);
				}
			}
		}
	}

	/* for versions before 6.05ab, derive "far text" bits */
	if (major < 6 ||
		(major == 6 && minor < 5) ||
		(major == 6 && minor == 5 && detail < 28))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				us_computenodefartextbit(ni);
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				us_computearcfartextbit(ai);
		}
	}

	/* for versions before 6.05ak, convert quick-key bindings */
	if (major < 6 ||
		(major == 6 && minor < 5) ||
		(major == 6 && minor == 5 && detail < 37))
	{
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_quickkeyskey);
		if (var != NOVARIABLE)
		{
			len = getlength(var);
			newlist = (CHAR **)emalloc(len * (sizeof (CHAR *)), el_tempcluster);
			changed = FALSE;
			for(i=0; i<len; i++)
			{
				pt = ((CHAR **)var->addr)[i];
				(void)allocstring(&newlist[i], pt, el_tempcluster);
				if (pt[1] != '/') continue;
				j = pt[0] & 0xFF;
				if (j >= 0216 && j <= 0231)
				{
					/* convert former "function" key */
					esnprintf(line, 200, x_("F%ld%s"), j - 0215, &pt[1]);
					(void)reallocstring(&newlist[i], line, el_tempcluster);
					changed = TRUE;
				}
			}
			if (changed)
			{
				nextchangequiet();
				setvalkey((INTBIG)us_tool, VTOOL, us_quickkeyskey, (INTBIG)newlist,
					VSTRING|VISARRAY|(len<<VLENGTHSH));
			}
			for(i=0; i<len; i++) efree((CHAR *)newlist[i]);
			efree((CHAR *)newlist);
		}
	}

	/* for versions before 6.06i, convert electrical units */
	if (major < 6 ||
		(major == 6 && minor < 6) ||
		(major == 6 && minor == 6 && detail < 9))
	{
		var = getvalkey((INTBIG)lib, VLIBRARY, VINTEGER, us_electricalunitskey);
		if (var != NOVARIABLE)
		{
			j = var->addr;
			i = (j&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH;
			if (i >= INTCAPUNITNFARAD)
			{
				j = (j & ~INTERNALCAPUNITS) | ((i+1) << INTERNALCAPUNITSSH);
				nextchangequiet();
				setvalkey((INTBIG)lib, VLIBRARY, us_electricalunitskey, j, VINTEGER);
				us_electricalunits = j;
			}
		}
		if (j != us_electricalunits)
		io_converteverytextdescriptor(lib, 1);
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto == sch_resistorprim)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_resistancekey);
					if (var != NOVARIABLE)
					{
						pt = describesimplevariable(var);
						value = (float)eatof(pt);
						i = estrlen(pt);
						if (i > 0)
						{
							if (tolower(pt[i-1]) == 'g')
							{
								value = value * 1000000000.0f;
							} else if (i > 2 && namesame(&pt[i-3], x_("meg")) == 0)
							{
								value = value * 1000000.0f;
							} else if (tolower(pt[i-1]) == 'k')
							{
								value = value * 1000.0f;
							}
						}
						var = setvalkey((INTBIG)ni, VNODEINST, sch_resistancekey,
							castint(value), VFLOAT | (var->type & ~VTYPE));
						if (var != NOVARIABLE)
							TDSETUNITS(var->textdescript, VTUNITSRES);
					}
					continue;
				}
				if (ni->proto == sch_capacitorprim)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_capacitancekey);
					if (var != NOVARIABLE)
					{
						pt = describesimplevariable(var);
						value = (float)eatof(pt);
						i = estrlen(pt);
						if (i > 0)
						{
							if (tolower(pt[i-1]) == 'f')
							{
								value = value / 1000000000000000.0f;
							} else if (tolower(pt[i-1]) == 'p')
							{
								value = value / 1000000000000.0f;
							} else if (tolower(pt[i-1]) == 'u')
							{
								value = value / 1000000.0f;
							} else if (tolower(pt[i-1]) == 'm')
							{
								value = value / 1000.0f;
							}
						}
						var = setvalkey((INTBIG)ni, VNODEINST, sch_capacitancekey,
							castint(value), VFLOAT | (var->type & ~VTYPE));
						if (var != NOVARIABLE)
							TDSETUNITS(var->textdescript, VTUNITSCAP);
					}
					continue;
				}
				if (ni->proto == sch_inductorprim)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_inductancekey);
					if (var != NOVARIABLE)
					{
						pt = describesimplevariable(var);
						value = (float)eatof(pt);
						i = estrlen(pt);
						if (i > 0)
						{
							if (tolower(pt[i-1]) == 'u')
							{
								value = value / 1000000.0f;
							} else if (tolower(pt[i-1]) == 'm')
							{
								value = value / 1000.0f;
							}
						}
						var = setvalkey((INTBIG)ni, VNODEINST, sch_inductancekey,
							castint(value), VFLOAT | (var->type & ~VTYPE));
						if (var != NOVARIABLE)
							TDSETUNITS(var->textdescript, VTUNITSIND);
					}
					continue;
				}
			}
		}
	}

	/* for versions before 6.08, adjust ports in technology-edit libraries */
	if (major < 6 ||
		(major == 6 && minor < 8))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto == gen_portprim)
					modifynodeinst(ni, -4000, -4000, 4000, 4000, 0, 0);
			}
		}
	}

	/* for versions before 6.08v, set "technology edit" bit for appropriate cells */
	if (major < 6 ||
		(major == 6 && minor < 8) ||
		(major == 6 && minor == 8 && detail < 22))
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
				if (var != NOVARIABLE) break;
			}
			if (ni != NONODEINST)
				np->userbits |= TECEDITCELL;
		}
	}

	/* for versions before 7.00l, convert quick key bindings that refer to "facets" */
	if (major < 7 ||
		(major == 7 && minor < 0) ||
		(major == 7 && minor == 0 && detail < 12))
	{
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_quickkeyskey);
		if (var != NOVARIABLE)
		{
			len = getlength(var);
			for(i=0; i<len; i++)
			{
				for(pt = ((CHAR **)var->addr)[i]; *pt != 0; pt++)
				{
					if (namesamen(pt, "facet", 5) != 0) continue;
					strcpy(pt, "Cell");
					strcpy(&pt[4], &pt[5]);
				}
			}
		}
	}

	/* the rest of the changes are just for version 4 or earlier */
	if (major >= 5) return;

	/* setup for units conversion */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* must scale, first make sure the technologies agree with this library */
	if (lib != el_curlib)
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			changetechnologylambda(tech, lib->lambda[tech->techindex]);

	/* now scale */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		if (var != NOVARIABLE)
		{
			((INTBIG *)var->addr)[0] *= 20;
			((INTBIG *)var->addr)[1] *= 20;
		}
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			ni->lowx *= 20;   ni->highx *= 20;
			ni->lowy *= 20;   ni->highy *= 20;
			ni->rotation *= 10;
			var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
			if (var != NOVARIABLE)
			{
				if (var->addr == TECHLAMBDA)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
					if (var != NOVARIABLE)
					{
						lam = myatoi(&((CHAR *)var->addr)[8]);
						(void)esnprintf(line, 200, x_("Lambda: %ld"), lam*20);
						nextchangequiet();
						(void)setvalkey((INTBIG)ni, VNODEINST, art_messagekey,
							(INTBIG)line, VSTRING|VDISPLAY);
					}
				}
			}
			var = gettrace(ni);
			if (var != NOVARIABLE)
			{
				i = getlength(var);
				for(j=0; j<i; j++) ((INTBIG *)var->addr)[j] *= 20;
			}
			var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_degreeskey);
			if (var != NOVARIABLE) var->addr *= 10;
			boundobj(ni->geom, &ni->geom->lowx, &ni->geom->highx, &ni->geom->lowy, &ni->geom->highy);
		}
	}

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			ai->width *= 20;
			ai->end[0].xpos *= 20;   ai->end[0].ypos *= 20;
			ai->end[1].xpos *= 20;   ai->end[1].ypos *= 20;
			(void)setshrinkvalue(ai, FALSE);
			var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, el_arc_radius_key);
			if (var != NOVARIABLE) var->addr *= 20;
			for(i=0; i<2; i++)
			{
				/* make sure arcinst connects in portinst area */
				shapeportpoly(ai->end[i].nodeinst, ai->end[i].portarcinst->proto, poly, FALSE);
				if (isinside(ai->end[i].xpos, ai->end[i].ypos, poly)) continue;
				portposition(ai->end[i].nodeinst, ai->end[i].portarcinst->proto, &lx, &ly);
				if (lx == ai->end[i].xpos && ly == ai->end[i].ypos) continue;

				/* try to make manhattan fix */
				if ((ai->end[0].xpos == ai->end[1].xpos) && isinside(ai->end[i].xpos, ly, poly))
					ai->end[i].ypos = ly; else
						if ((ai->end[0].ypos == ai->end[1].ypos) &&
							isinside(lx, ai->end[i].ypos, poly))
								ai->end[i].xpos = lx; else
				{
					ai->end[i].xpos = lx;   ai->end[i].ypos = ly;
				}
			}
			ai->length = computedistance(ai->end[0].xpos,ai->end[0].ypos,
				ai->end[1].xpos,ai->end[1].ypos);
			boundobj(ai->geom, &ai->geom->lowx, &ai->geom->highx, &ai->geom->lowy, &ai->geom->highy);
		}
		db_boundcell(np, &np->lowx, &np->highx, &np->lowy, &np->highy);
		io_fixrtree(np->rtree);
	}

	/* now restore the technology lambda values */
	if (lib != el_curlib)
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			changetechnologylambda(tech, el_curlib->lambda[tech->techindex]);
}

#define OLDSIZE      01700
#define OLDSIZESH        6
#define OLDTXT4P         0			/* 4 point text     */
#define OLDTXT6P         1			/* 6 point text     */
#define OLDTXT8P         2			/* 8 point text     */
#define OLDTXT10P        3			/* 10 point text    */
#define OLDTXT12P        4			/* 12 point text    */
#define OLDTXT14P        5			/* 14 point text    */
#define OLDTXT16P        6			/* 16 point text    */
#define OLDTXT18P        7			/* 18 point text    */
#define OLDTXT20P        8			/* 20 point text    */
#define OLDTXTHL         9			/* half-lambda text */
#define OLDTXT1L        10			/* 1-lambda text    */
#define OLDTXT2L        11			/* 2-lambda text    */
#define OLDTXT3L        12			/* 3-lambda text    */
#define OLDTXT4L        13			/* 4-lambda text    */
#define OLDTXT5L        14			/* 5-lambda text    */
#define OLDTXT6L        15			/* 6-lambda text    */

#define OLDOFFSCALE  07700000000	/* old scale of text offset */

/*
 * Routine to do conversion "how" on every text descriptor in library "lib".
 * If "how" is 0, convert old-style text sizes to new style
 * If "how" is 1, shorten the text offset scale field by 1 bit
 */
void io_converteverytextdescriptor(LIBRARY *lib, INTBIG how)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			io_convertalltextdescriptors(ni->numvar, ni->firstvar, how);
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				io_convertalltextdescriptors(pi->numvar, pi->firstvar, how);
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				io_convertalltextdescriptors(pe->numvar, pe->firstvar, how);
			if (ni->proto->primindex == 0)
				io_converttextdescriptor(ni->textdescript, how);
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			io_convertalltextdescriptors(ai->numvar, ai->firstvar, how);
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			io_convertalltextdescriptors(pp->numvar, pp->firstvar, how);
			io_converttextdescriptor(pp->textdescript, how);
		}
		io_convertalltextdescriptors(np->numvar, np->firstvar, how);
	}
	io_convertalltextdescriptors(lib->numvar, lib->firstvar, how);
}

/*
 * Routine to do conversion "how" on all text descriptors on all "numvar" variables
 * in "firstvar".
 */
void io_convertalltextdescriptors(INTSML numvar, VARIABLE *firstvar, INTBIG how)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		io_converttextdescriptor(var->textdescript, how);
	}
}

/*
 * Routine to do conversion "how" on text descriptor "descript".
 */
void io_converttextdescriptor(UINTBIG *descript, INTBIG how)
{
	REGISTER INTBIG size, oldscale;

	switch (how)
	{
		case 0:		/* convert size information */
			size = (descript[0]&OLDSIZE) >> OLDSIZESH;
			switch (size)
			{
				case OLDTXT4P:  size = TXTSETPOINTS(4);     break;
				case OLDTXT6P:  size = TXTSETPOINTS(6);     break;
				case OLDTXT8P:  size = TXTSETPOINTS(8);     break;
				case OLDTXT10P: size = TXTSETPOINTS(10);    break;
				case OLDTXT12P: size = TXTSETPOINTS(12);    break;
				case OLDTXT14P: size = TXTSETPOINTS(14);    break;
				case OLDTXT16P: size = TXTSETPOINTS(16);    break;
				case OLDTXT18P: size = TXTSETPOINTS(18);    break;
				case OLDTXT20P: size = TXTSETPOINTS(20);    break;
				case OLDTXTHL:  size = TXTSETQLAMBDA(4/2);  break;
				case OLDTXT1L:  size = TXTSETQLAMBDA(1*4);  break;
				case OLDTXT2L:  size = TXTSETQLAMBDA(2*4);  break;
				case OLDTXT3L:  size = TXTSETQLAMBDA(3*4);  break;
				case OLDTXT4L:  size = TXTSETQLAMBDA(4*4);  break;
				case OLDTXT5L:  size = TXTSETQLAMBDA(5*4);  break;
				case OLDTXT6L:  size = TXTSETQLAMBDA(6*4);  break;
			}
			descript[0] &= ~OLDSIZE;
			descript[1] = size << VTSIZESH;
			break;

		case 1:		/* shorten offset scale field */
			oldscale = descript[1] & OLDOFFSCALE;
			descript[1] = (descript[1] & ~VTOFFSCALE) | (oldscale & VTOFFSCALE);
			break;
	}
}

void io_fixupnodeinst(NODEINST *ni, INTBIG dlx, INTBIG dly, INTBIG dhx, INTBIG dhy)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;

	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPETEMPUNRIGID, 0);
	}
	modifynodeinst(ni, dlx, dly, dhx, dhy, 0, 0);
}

void io_convertallrelativetext(INTSML numvar, VARIABLE *firstvar)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		io_convertrelativetext(var->textdescript);
	}
}

void io_convertrelativetext(UINTBIG *descript)
{
	REGISTER INTBIG size;

	size = (descript[0]&OLDSIZE) >> OLDSIZESH;
	if (size >= OLDTXT4P && size <= OLDTXT20P) return;

	if (size <= 13)
	{
		/* TXTSMALL */
		descript[0] = (descript[0] & ~OLDSIZE) | (OLDTXT1L << OLDSIZESH);
		return;
	}

	if (size == 14)
	{
		/* TXTMEDIUM */
		descript[0] = (descript[0] & ~OLDSIZE) | (OLDTXT2L << OLDSIZESH);
		return;
	}

	/* TXTLARGE */
	descript[0] = (descript[0] & ~OLDSIZE) | (OLDTXT3L << OLDSIZESH);
}

/*
 * Routine to convert old primitive names to their proper nodeprotos.
 */
NODEPROTO *io_convertoldprimitives(TECHNOLOGY *tech, CHAR *name)
{
	if (namesame(name, x_("Cell-Center")) == 0) return(gen_cellcenterprim);
	if (tech == art_tech)
	{
		if (namesame(name, x_("Message")) == 0 ||
			namesame(name, x_("Centered-Message")) == 0 ||
			namesame(name, x_("Left-Message")) == 0 ||
			namesame(name, x_("Right-Message")) == 0) return(gen_invispinprim);
		if (namesame(name, x_("Opened-FarDotted-Polygon")) == 0)
			return(art_openedthickerpolygonprim);
	}
	if (tech == mocmos_tech)
	{
		if (namesame(name, x_("Metal-1-Substrate-Con")) == 0)
			return(mocmos_metal1nwellprim);
		if (namesame(name, x_("Metal-1-Well-Con")) == 0)
			return(mocmos_metal1pwellprim);
	}
	return(NONODEPROTO);
}

/*
 * Routine to convert old libraries that have no cellgroup pointers.  Finds
 * matching names and presumes that they are in the same cellgroup.
 */
void io_buildcellgrouppointersfromnames(LIBRARY *lib)
{
	REGISTER NODEPROTO *np, *onp, *prevmatch;

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->nextcellgrp = np;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		/* ignore old versions */
		if (np->newestversion != np) continue;

		/* skip if already in a cellgroup */
		if (np->nextcellgrp != np) continue;

		/* find others in this cell group */
		prevmatch = np;
		for(onp = np->nextnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			/* ignore old versions */
			if (onp->newestversion != onp) continue;

			/* ignore if the name doesn't match */
			if (namesame(np->protoname, onp->protoname) != 0) continue;

			/* add this to the cellgroup */
			prevmatch->nextcellgrp = onp;
			prevmatch = onp;
		}

		/* make this cellgroup's list circular */
		if (prevmatch != np)
			prevmatch->nextcellgrp = np;
	}
}

/*
 * Routine to find the version of Electric that generated library "lib" and
 * parse it into three fields: the major version number, minor version, and
 * a detail version number.
 */
void io_getversion(LIBRARY *lib, INTBIG *major, INTBIG *minor, INTBIG *detail)
{
	REGISTER VARIABLE *var;
	INTBIG emajor, eminor, edetail;
	CHAR *libversion;

	var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("LIB_former_version"));
	if (var == NOVARIABLE)
	{
		*major = *minor = *detail = 0;
		return;
	}
	libversion = (CHAR *)var->addr;
	parseelectricversion(libversion, major, minor, detail);

	/* see if this library is newer than the current version of Electric */
	parseelectricversion(el_version, &emajor, &eminor, &edetail);
	if (*major > emajor ||
		(*major == emajor && *minor > eminor) ||
		(*major == emajor && *minor == eminor && *detail > edetail))
	{
		ttyputerr(_("Warning: library %s comes from a NEWER version of Electric (%s)"),
			lib->libname, libversion);
	}
}

/* Technology: Lambda Adjustment */
static DIALOGITEM io_techadjlamdialogitems[] =
{
 /*  1 */ {0, {232,220,256,300}, BUTTON, N_("Done")},
 /*  2 */ {0, {204,264,220,512}, BUTTON, N_("Use New Size for all Technologies")},
 /*  3 */ {0, {204,8,220,256}, BUTTON, N_("Use Current Size for all Technologies")},
 /*  4 */ {0, {28,300,128,512}, SCROLL, x_("")},
 /*  5 */ {0, {148,8,164,164}, MESSAGE, N_("Current lambda size:")},
 /*  6 */ {0, {148,264,164,416}, MESSAGE, N_("New lambda size:")},
 /*  7 */ {0, {148,164,164,256}, MESSAGE, x_("")},
 /*  8 */ {0, {148,416,164,512}, MESSAGE, x_("")},
 /*  9 */ {0, {176,264,192,512}, BUTTON, N_("<< Use New Size in Current Library")},
 /* 10 */ {0, {176,8,192,256}, BUTTON, N_("Use Current Size in New Library >>")},
 /* 11 */ {0, {80,16,96,292}, MESSAGE, N_("and choose from the actions below.")},
 /* 12 */ {0, {8,8,24,508}, MESSAGE, N_("This new library uses different lambda values than existing libraries.")},
 /* 13 */ {0, {28,8,44,292}, MESSAGE, N_("You should unify the lambda values.")},
 /* 14 */ {0, {60,8,76,292}, MESSAGE, N_("Click on each technology in this list")},
 /* 15 */ {0, {136,8,137,512}, DIVIDELINE, x_("")},
 /* 16 */ {0, {112,8,128,284}, MESSAGE, N_("Use 'Check and Repair Libraries' when done.")}
};
static DIALOG io_techadjlamdialog = {{75,75,340,596}, N_("Lambda Value Adjustment"), 0, 16, io_techadjlamdialogitems, 0, 0};

/* special items for the "lambda adjustment" dialog: */
#define DTLA_USENEWALWAYS   2		/* Use new unit always (button) */
#define DTLA_USECURALWAYS   3		/* Use current unit always (button) */
#define DTLA_TECHLIST       4		/* List of technologies (scroll) */
#define DTLA_CURLAMBDA      7		/* Current lambda value (stat text) */
#define DTLA_NEWLAMBDA      8		/* New lambda value (stat text) */
#define DTLA_USENEW         9		/* Use new unit (button) */
#define DTLA_USECUR        10		/* Use current unit (button) */

/*
 * Routine to examine new library "lib" and make sure that the lambda values in it are
 * the same as existing libraries/technologies.  Automatically adjusts values if possible,
 * and prompts the user to help out if necessary.
 */
void io_unifylambdavalues(LIBRARY *lib)
{
	REGISTER TECHNOLOGY *tech, *otech, **techarray;
	REGISTER LIBRARY *olib;
	REGISTER NODEPROTO *np, **libcells;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER INTBIG itemHit, i, count, recompute, newunit, curunit, *newlamarray,
		oldbits;
	REGISTER CHAR *pt;
	float lambdainmicrons;
	CHAR line[50];
	REGISTER void *dia;

	/* see if this library has incompatible lambda values */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech == art_tech) continue;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if (lib->lambda[tech->techindex] != olib->lambda[tech->techindex])
				break;
		}
		if (olib != NOLIBRARY) break;
	}
	if (tech == NOTECHNOLOGY) return;

	/* this library has different values: check usage in this and other libraries */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		olib->temp1 = (INTBIG)emalloc(el_maxtech * (sizeof (NODEPROTO *)), el_tempcluster);
		if (olib->temp1 == 0) return;
		for(i=0; i<el_maxtech; i++) ((NODEPROTO **)olib->temp1)[i] = NONODEPROTO;
	}
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		libcells = (NODEPROTO **)olib->temp1;
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				tech = ni->proto->tech;
				libcells[tech->techindex] = np;
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				tech = ai->proto->tech;
				libcells[tech->techindex] = np;
			}
		}
	}

	/* see if there are inconsistencies that cannot be automatically resolved */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech == art_tech) continue;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if (((NODEPROTO **)olib->temp1)[tech->techindex] == NONODEPROTO) continue;
			if (((NODEPROTO **)lib->temp1)[tech->techindex] == NONODEPROTO) continue;
			if (lib->lambda[tech->techindex] != olib->lambda[tech->techindex])
				break;
		}
		if (olib != NOLIBRARY) break;
	}
	if (tech != NOTECHNOLOGY)
	{
		/* get the user to resolve the inconsistencies */
		dia = DiaInitDialog(&io_techadjlamdialog);
		if (dia == 0) return;
		DiaInitTextDialog(dia, DTLA_TECHLIST, DiaNullDlogList, DiaNullDlogItem,
			DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
		DiaUnDimItem(dia, DTLA_USECURALWAYS);
		DiaUnDimItem(dia, DTLA_USENEWALWAYS);
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		{
			if (tech == art_tech) continue;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			{
				if (((NODEPROTO **)olib->temp1)[tech->techindex] == NONODEPROTO) continue;
				if (((NODEPROTO **)lib->temp1)[tech->techindex] == NONODEPROTO) continue;
				if (lib->lambda[tech->techindex] != olib->lambda[tech->techindex])
					break;
			}
			if (olib == NOLIBRARY) continue;
			DiaStuffLine(dia, DTLA_TECHLIST, tech->techname);
		}
		DiaSelectLine(dia, DTLA_TECHLIST, 0);
		recompute = 1;
		tech = NOTECHNOLOGY;
		for(;;)
		{
			if (recompute != 0)
			{
				recompute = 0;

				/* figure out which technology is selected */
				i = DiaGetCurLine(dia, DTLA_TECHLIST);
				pt = DiaGetScrollLine(dia, DTLA_TECHLIST, i);
				for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
					if (namesame(pt, tech->techname) == 0) break;
				if (tech == NOTECHNOLOGY) continue;

				/* figure out which units are in force */
				curunit = newunit = lib->lambda[tech->techindex];
				for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				{
					if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
					if (((NODEPROTO **)olib->temp1)[tech->techindex] == NONODEPROTO) continue;
					if (((NODEPROTO **)olib->temp1)[tech->techindex] == 0) continue;
					if (olib->lambda[tech->techindex] != lib->lambda[tech->techindex]) break;
				}
				if (olib != NOLIBRARY) curunit = olib->lambda[tech->techindex];

				/* see if it has already been overridden */
				if (((INTBIG *)lib->temp1)[tech->techindex] == 0) newunit = curunit;

				/* set dialog values and offer choices */
				lambdainmicrons = scaletodispunit(curunit, DISPUNITMIC);
				esnprintf(line, 50, x_("%gu"), lambdainmicrons);
				DiaSetText(dia, DTLA_CURLAMBDA, line);
				lambdainmicrons = scaletodispunit(newunit, DISPUNITMIC);
				esnprintf(line, 50, x_("%gu"), lambdainmicrons);
				DiaSetText(dia, DTLA_NEWLAMBDA, line);
				if (newunit == curunit)
				{
					DiaDimItem(dia, DTLA_USENEW);
					DiaDimItem(dia, DTLA_USECUR);
				} else
				{
					DiaUnDimItem(dia, DTLA_USENEW);
					DiaUnDimItem(dia, DTLA_USECUR);
				}
			}
			itemHit = DiaNextHit(dia);
			if (itemHit == OK) break;
			if (itemHit == DTLA_TECHLIST) recompute = 1;
			if (itemHit == DTLA_USENEW)
			{
				/* use new unit */
				if (tech != NOTECHNOLOGY)
				{
					for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
					{
						if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
						if (((NODEPROTO **)olib->temp1)[tech->techindex] == NONODEPROTO) continue;
						((NODEPROTO **)olib->temp1)[tech->techindex] = 0;
					}
				}
				recompute = 1;
				continue;
			}
			if (itemHit == DTLA_USENEWALWAYS)
			{
				/* use new unit always */
				for(otech = el_technologies; otech != NOTECHNOLOGY; otech = otech->nexttechnology)
				{
					for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
					{
						if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
						if (((NODEPROTO **)olib->temp1)[otech->techindex] == NONODEPROTO) continue;
						((NODEPROTO **)olib->temp1)[otech->techindex] = 0;
					}
				}
				recompute = 1;
				DiaDimItem(dia, DTLA_USECURALWAYS);
				DiaDimItem(dia, DTLA_USENEWALWAYS);
				continue;
			}
			if (itemHit == DTLA_USECURALWAYS)
			{
				/* use current unit always */
				for(otech = el_technologies; otech != NOTECHNOLOGY; otech = otech->nexttechnology)
					((NODEPROTO **)lib->temp1)[otech->techindex] = 0;
				recompute = 1;
				DiaDimItem(dia, DTLA_USECURALWAYS);
				DiaDimItem(dia, DTLA_USENEWALWAYS);
				continue;
			}
			if (itemHit == DTLA_USECUR)
			{
				/* use current unit */
				if (tech != NOTECHNOLOGY) ((NODEPROTO **)lib->temp1)[tech->techindex] = 0;
				recompute = 1;
				continue;
			}
		}
		DiaDoneDialog(dia);
	}

	/* do automatic conversions */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (tech == art_tech) continue;
		if (((NODEPROTO **)lib->temp1)[tech->techindex] == NONODEPROTO)
			((NODEPROTO **)lib->temp1)[tech->techindex] = 0;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if (((NODEPROTO **)olib->temp1)[tech->techindex] == NONODEPROTO)
				((NODEPROTO **)olib->temp1)[tech->techindex] = 0;
		}
	}

	/* adjust lambda values in old libraries to match this one */
	techarray = (TECHNOLOGY **)emalloc(el_maxtech * (sizeof (TECHNOLOGY *)), el_tempcluster);
	if (techarray == 0) return;
	newlamarray = (INTBIG *)emalloc(el_maxtech * SIZEOFINTBIG, el_tempcluster);
	if (newlamarray == 0) return;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		if (olib == lib) continue;

		/* see how many technologies are affected */
		count = 0;
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		{
			if (((NODEPROTO **)olib->temp1)[tech->techindex] != 0) continue;
			if (lib->lambda[tech->techindex] == olib->lambda[tech->techindex]) continue;
			techarray[count] = tech;
			newlamarray[count] = lib->lambda[tech->techindex];
			count++;
		}
		if (count == 0) continue;

		/* adjust this library */
		oldbits = olib->userbits;
		changelambda(count, techarray, newlamarray, olib, 1);
		olib->userbits = (olib->userbits & ~LIBCHANGEDMAJOR) |
			(oldbits & LIBCHANGEDMAJOR);
	}

	/* change lambda values in this library to match old ones */
	count = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (((NODEPROTO **)lib->temp1)[tech->techindex] != 0) continue;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
			if (olib->lambda[tech->techindex] == lib->lambda[tech->techindex]) continue;
			if (((NODEPROTO **)olib->temp1)[tech->techindex] != 0) break;
		}
		if (olib == NOLIBRARY) continue;
		techarray[count] = tech;
		newlamarray[count] = olib->lambda[tech->techindex];
		count++;
	}
	if (count != 0)
	{
		/* adjust new library */
		changelambda(count, techarray, newlamarray, lib, 1);
	}

	/* free memory used here */
	efree((CHAR *)techarray);
	efree((CHAR *)newlamarray);
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		efree((CHAR *)olib->temp1);
}

/****************************** LIBRARY OPTIONS DIALOG ******************************/

/* Library: Options */
static DIALOGITEM io_liboptdialogitems[] =
{
 /*  1 */ {0, {124,148,148,228}, BUTTON, N_("OK")},
 /*  2 */ {0, {124,16,148,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,236}, RADIO, N_("No backup of library files")},
 /*  4 */ {0, {32,8,48,236}, RADIO, N_("Backup of last library file")},
 /*  5 */ {0, {56,8,72,236}, RADIO, N_("Backup history of library files")},
 /*  6 */ {0, {92,8,108,236}, CHECK, N_("Check database after write")}
};
static DIALOG io_liboptdialog = {{75,75,232,321}, N_("Library Options"), 0, 6, io_liboptdialogitems, 0, 0};

/* special items for the "library options" dialog: */
#define DLBO_NOBACKUP         3		/* no backup (radio) */
#define DLBO_ONEBACKUP        4		/* backup one level (radio) */
#define DLBO_FULLBACKUP       5		/* backup history (radio) */
#define DLBO_CHECKAFTERWRITE  6		/* check after write (check) */

void io_libraryoptiondlog(void)
{
	INTBIG itemHit, i, *origstate, curstate[NUMIOSTATEBITWORDS];
	REGISTER void *dia;

	/* display the library paths dialog box */
	dia = DiaInitDialog(&io_liboptdialog);
	if (dia == 0) return;

	/* get current state of I/O tool */
	origstate = io_getstatebits();
	for(i=0; i<NUMIOSTATEBITWORDS; i++) curstate[i] = origstate[i];
	switch (curstate[0]&BINOUTBACKUP)
	{
		case BINOUTNOBACK:   DiaSetControl(dia, DLBO_NOBACKUP, 1);    break;
		case BINOUTONEBACK:  DiaSetControl(dia, DLBO_ONEBACKUP, 1);   break;
		case BINOUTFULLBACK: DiaSetControl(dia, DLBO_FULLBACKUP, 1);  break;
	}
	if ((curstate[0]&CHECKATWRITE) != 0) DiaSetControl(dia, DLBO_CHECKAFTERWRITE, 1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DLBO_NOBACKUP || itemHit == DLBO_ONEBACKUP ||
			itemHit == DLBO_FULLBACKUP)
		{
			DiaSetControl(dia, DLBO_NOBACKUP, 0);
			DiaSetControl(dia, DLBO_ONEBACKUP, 0);
			DiaSetControl(dia, DLBO_FULLBACKUP, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DLBO_CHECKAFTERWRITE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		curstate[0] &= ~(BINOUTBACKUP|CHECKATWRITE);
		if (DiaGetControl(dia, DLBO_ONEBACKUP) != 0) curstate[0] |= BINOUTONEBACK; else
			if (DiaGetControl(dia, DLBO_FULLBACKUP) != 0) curstate[0] |= BINOUTFULLBACK;
		if (DiaGetControl(dia, DLBO_CHECKAFTERWRITE) != 0) curstate[0] |= CHECKATWRITE;
		for(i=0; i<NUMIOSTATEBITWORDS; i++)
			if (curstate[i] != origstate[i]) break;
		if (i < NUMIOSTATEBITWORDS)
			io_setstatebits(curstate);
	}
	DiaDoneDialog(dia);
}

/* CDL Options */
static DIALOGITEM io_cdloptdialogitems[] =
{
 /*  1 */ {0, {100,236,124,316}, BUTTON, N_("OK")},
 /*  2 */ {0, {100,64,124,144}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,184}, MESSAGE, N_("Cadence Library Name:")},
 /*  4 */ {0, {8,187,24,363}, EDITTEXT, x_("")},
 /*  5 */ {0, {32,8,48,184}, MESSAGE, N_("Cadence Library Path:")},
 /*  6 */ {0, {32,187,64,363}, EDITTEXT, x_("")},
 /*  7 */ {0, {72,8,88,176}, CHECK, N_("Convert brackets")}
};
static DIALOG io_cdloptdialog = {{75,75,208,447}, N_("CDL Options"), 0, 7, io_cdloptdialogitems, 0, 0};

/* special items for the "CDL Options" dialog: */
#define DCDL_LIBNAME     4		/* Cadence Library Name (edit text) */
#define DCDL_LIBPATH     6		/* Cadence Library Path (edit text) */
#define DCDL_CONVBRACKET 7		/* Convert brackets (check) */

/*
 * Routine to run the CDL Options dialog.
 */
void io_cdloptionsdialog(void)
{
	INTBIG i, itemHit, *curstate, newstate[NUMIOSTATEBITWORDS];
	REGISTER VARIABLE *var;
	REGISTER CHAR *inilibname, *inilibpath;
	BOOLEAN libnamechanged, libpathchanged;
	REGISTER void *dia;

	dia = DiaInitDialog(&io_cdloptdialog);
	if (dia == 0) return;
	var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_cdl_library_name"));
	if (var == NOVARIABLE) inilibname = x_(""); else
		inilibname = (CHAR *)var->addr;
	DiaSetText(dia, DCDL_LIBNAME, inilibname);
	var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_cdl_library_path"));
	if (var == NOVARIABLE) inilibpath = x_(""); else
		inilibpath = (CHAR *)var->addr;
	DiaSetText(dia, DCDL_LIBPATH, inilibpath);
	curstate = io_getstatebits();
	if ((curstate[1]&CDLNOBRACKETS) != 0) DiaSetControl(dia, DCDL_CONVBRACKET, 1);

	libnamechanged = libpathchanged = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DCDL_CONVBRACKET)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DCDL_LIBNAME)
		{
			if (estrcmp(inilibname, DiaGetText(dia, DCDL_LIBNAME)) != 0)
				libnamechanged = TRUE;
			continue;
		}
		if (itemHit == DCDL_LIBPATH)
		{
			if (estrcmp(inilibpath, DiaGetText(dia, DCDL_LIBPATH)) != 0)
				libpathchanged = TRUE;
			continue;
		}
	}
	if (itemHit == OK)
	{
		if (libnamechanged != 0)
		{
			(void)setval((INTBIG)io_tool, VTOOL, x_("IO_cdl_library_name"),
				(INTBIG)DiaGetText(dia, DCDL_LIBNAME), VSTRING);
		}
		if (libpathchanged != 0)
		{
			(void)setval((INTBIG)io_tool, VTOOL, x_("IO_cdl_library_path"),
				(INTBIG)DiaGetText(dia, DCDL_LIBPATH), VSTRING);
		}
		for(i=0; i<NUMIOSTATEBITWORDS; i++) newstate[i] = curstate[i];
		if (DiaGetControl(dia, DCDL_CONVBRACKET) != 0) newstate[1] |= CDLNOBRACKETS; else
			newstate[1] &= ~CDLNOBRACKETS;
		for(i=0; i<NUMIOSTATEBITWORDS; i++) if (curstate[i] != newstate[i]) break;
		if (i < NUMIOSTATEBITWORDS) io_setstatebits(newstate);
	}
	DiaDoneDialog(dia);
}

/* Sue Options */
static DIALOGITEM io_sueoptdialogitems[] =
{
 /*  1 */ {0, {36,120,60,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {36,16,60,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,200}, CHECK, N_("Make 4-port transistors")}
};
static DIALOG io_sueoptdialog = {{50,75,119,285}, N_("SUE Options"), 0, 3, io_sueoptdialogitems, 0, 0};

/* special items for the "Sue Options" dialog: */
#define DSUE_USE4PORTS  3		/* Use 4-port transistors (check) */

/*
 * Routine to run the Sue Options dialog.
 */
void io_sueoptionsdialog(void)
{
	INTBIG i, itemHit, *curstate, newstate[NUMIOSTATEBITWORDS];
	REGISTER void *dia;

	dia = DiaInitDialog(&io_sueoptdialog);
	if (dia == 0) return;
	curstate = io_getstatebits();
	if ((curstate[1]&SUEUSE4PORTTRANS) != 0) DiaSetControl(dia, DSUE_USE4PORTS, 1);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DSUE_USE4PORTS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}
	if (itemHit == OK)
	{
		for(i=0; i<NUMIOSTATEBITWORDS; i++) newstate[i] = curstate[i];
		if (DiaGetControl(dia, DSUE_USE4PORTS) != 0) newstate[1] |= SUEUSE4PORTTRANS; else
			newstate[1] &= ~SUEUSE4PORTTRANS;
		for(i=0; i<NUMIOSTATEBITWORDS; i++) if (curstate[i] != newstate[i]) break;
		if (i < NUMIOSTATEBITWORDS) io_setstatebits(newstate);
	}
	DiaDoneDialog(dia);
}

#define VARTYPELAYTOLAY    0		/* this variable is a layer-to-layer list */
#define VARTYPELAYER       1		/* this variable is a layer list */
#define VARTYPELAYERNONDRC 2		/* this variable is a layer list, not related to DRC */
#define VARTYPENODE        3		/* this variable is a node list */
#define VARTYPENODE2       4		/* this variable has 2 entries per node */

static struct
{
	CHAR   *variable;
	CHAR   *meaning;
	INTBIG  variabletype;
	INTBIG  key;
	INTBIG  defaultint;
	CHAR   *defaultstring;
	float   defaultfloat;
} io_techlayervariables[] =
{
	{x_("IO_cif_layer_names"),                      N_("CIF Layer Names"),                          VARTYPELAYERNONDRC,0,  0,     x_(""), 0.0},
	{x_("IO_dxf_layer_names"),                      N_("DXF Layer Names"),                          VARTYPELAYERNONDRC,0,  0,     x_(""), 0.0},
	{x_("IO_gds_layer_numbers"),                    N_("GDS Layer Numbers"),                        VARTYPELAYERNONDRC,0, -1,     x_(""), 0.0},
	{x_("IO_skill_layer_names"),                    N_("SKILL Layer Names"),                        VARTYPELAYERNONDRC,0,  0,     x_(""), 0.0},
	{x_("SIM_spice_resistance"),                    N_("SPICE Layer Resistances"),                  VARTYPELAYERNONDRC,0,  0,     x_(""), 0.0},
	{x_("SIM_spice_capacitance"),                   N_("SPICE Layer Capacitances"),                 VARTYPELAYERNONDRC,0,  0,     x_(""), 0.0},
	{x_("DRC_min_connected_distances"),             N_("Normal Connected Design Rule spacings"),    VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_connected_distances_rule"),        N_("Normal Connected Design Rule"),             VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances"),           N_("Normal Unconnected Design Rule spacings"),  VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances_rule"),      N_("Normal Unconnected Design Rule"),           VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_connected_distances_wide"),        N_("Wide Connected Design Rule spacings"),      VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_connected_distances_wide_rule"),   N_("Wide Connected Design Rule"),               VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances_wide"),      N_("Wide Unconnected Design Rule spacings"),    VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances_wide_rule"), N_("Wide Unconnected Design Rule"),             VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_connected_distances_multi"),       N_("Multicut Connected Design Rule spacings"),  VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_connected_distances_multi_rule"),  N_("Multicut Connected Design Rule"),           VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances_wide"),      N_("Wide Unconnected Design Rule spacings"),    VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances_wide_rule"), N_("Wide Unconnected Design Rule"),             VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_unconnected_distances_multi"),     N_("Multicut Unconnected Design Rule spacings"),VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_edge_distances"),                  N_("Edge Design Rule spacings"),                VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_edge_distances_rule"),             N_("Edge Design Rules"),                        VARTYPELAYTOLAY,   0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_width"),			                N_("Minimum Layer Widths"),                     VARTYPELAYER,      0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_width_rule"),			            N_("Minimum Layer Width Rules"),                VARTYPELAYER,      0,  0,     x_(""), 0.0},
	{x_("DRC_min_node_size"),			            N_("Minimum Node Sizes"),                       VARTYPENODE2,      0, -WHOLE, x_(""), 0.0},
	{x_("DRC_min_node_size_rule"),			        N_("Minimum Node Size Rules"),                  VARTYPENODE,       0,  0,     x_(""), 0.0},
	{0, 0, FALSE, 0, 0, x_(""), 0.0}
};

/*
 * this table rearranges the MOSIS CMOS Submicron layer tables from
 * their old 4-layer metal (34 layers) to 6-layer metal (40 layers)
 */
static INTBIG tech_mocmossubarrange[] =
{
	 0,  1,  2,  3,  6,  7,  8,  9, 10, 11,
	12, 13, 14, 15, 16, 17, 18, 21, 22, 23,
	24, 25, 26, 27, 28, 31, 32, 33, 34, 35,
	36, 37, 38, 39
};

static struct
{
	CHAR *techname;
	INTBIG oldlayercount, newlayercount;
	INTBIG *arrangement;
} io_techlayerfixes[] =
{
	{x_("mocmossub"), 34, 40, tech_mocmossubarrange},
	{x_("mocmos"),    34, 40, tech_mocmossubarrange},
	{0, 0, 0, 0}
};

void io_fixtechlayers(LIBRARY *lib)
{
	REGISTER INTBIG i, j, k, l, l1, l2, oldpos, newpos, newl1, newl2, *newints;
	REGISTER float *newfloats;
	REGISTER BOOLEAN factorywarned;
	REGISTER CHAR **newstrings;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var;

	factorywarned = FALSE;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(i=0; io_techlayervariables[i].variable != 0; i++)
		{
			if (io_techlayervariables[i].key == 0)
				io_techlayervariables[i].key = makekey(io_techlayervariables[i].variable);
			var = getvalkey((INTBIG)tech, VTECHNOLOGY, -1, io_techlayervariables[i].key);
			if (var == NOVARIABLE) continue;
			switch (io_techlayervariables[i].variabletype)
			{
				case VARTYPELAYTOLAY:
					/* this variable should have one per layer-to-layer combination in the technology */
					l = tech->layercount;
					if (getlength(var) == (l * l + l) / 2) continue;
					break;
				case VARTYPELAYER:
				case VARTYPELAYERNONDRC:
					/* this variable should have one per layer in the technology */
					if (getlength(var) == tech->layercount) continue;
					break;
				case VARTYPENODE:
					/* this variable should have one per node in the technology */
					if (getlength(var) == tech->nodeprotocount) continue;
					break;
				case VARTYPENODE2:
					/* this variable should have two per node in the technology */
					if (getlength(var) == tech->nodeprotocount*2) continue;
					break;
			}

			/* layers are inconsistent: see if there are rules to fix it */
			for(j=0; io_techlayerfixes[j].techname != 0; j++)
			{
				if (namesame(tech->techname, io_techlayerfixes[j].techname) != 0) continue;
				switch (io_techlayervariables[i].variabletype)
				{
					case VARTYPELAYTOLAY:
						l = io_techlayerfixes[j].oldlayercount;
						if ((l*l+l)/2 != getlength(var)) continue;
						break;
					case VARTYPELAYER:
					case VARTYPELAYERNONDRC:
						if (io_techlayerfixes[j].oldlayercount != getlength(var)) continue;
						break;
					default:
						continue;
				}
				if (io_techlayerfixes[j].newlayercount != tech->layercount) continue;
				break;
			}
			if (io_techlayerfixes[j].techname != 0)
			{
				if (io_techlayervariables[i].variabletype == VARTYPELAYTOLAY)
				{
					k = tech->layercount;
					l = (k * k + k)/2;
					newints = (INTBIG *)emalloc(l * SIZEOFINTBIG, tech->cluster);
					for(k=0; k<l; k++)
						newints[k] = io_techlayervariables[i].defaultint;
					for(l1=0; l1<io_techlayerfixes[j].oldlayercount; l1++)
					{
						for(l2=l1; l2<io_techlayerfixes[j].oldlayercount; l2++)
						{
							oldpos = (l1+1) * (l1/2) + (l1&1) * ((l1+1)/2);
							oldpos = l2 + io_techlayerfixes[j].oldlayercount * l1 - oldpos;
							newl1 = io_techlayerfixes[j].arrangement[l1];
							newl2 = io_techlayerfixes[j].arrangement[l2];
							newpos = (newl1+1) * (newl1/2) + (newl1&1) * ((newl1+1)/2);
							newpos = newl2 + tech->layercount * newl1 - newpos;
							newints[newpos] = ((INTBIG *)var->addr)[oldpos];
						}
					}
					(void)setvalkey((INTBIG)tech, VTECHNOLOGY, io_techlayervariables[i].key,
						(INTBIG)newints, (var->type&VTYPE)|VISARRAY|(l<<VLENGTHSH));
					efree((CHAR *)newints);
				} else
				{
					/* able to fix the ordering */
					if ((var->type&VTYPE) == VSTRING)
					{
						newstrings = (CHAR **)emalloc(tech->layercount * (sizeof (CHAR *)),
							tech->cluster);
						for(k=0; k<tech->layercount; k++)
							newstrings[k] = io_techlayervariables[i].defaultstring;
						for(k=0; k<getlength(var); k++)
							newstrings[io_techlayerfixes[j].arrangement[k]] =
								((CHAR **)var->addr)[k];
						(void)setvalkey((INTBIG)tech, VTECHNOLOGY, io_techlayervariables[i].key,
							(INTBIG)newstrings, (var->type&VTYPE)|VISARRAY|(tech->layercount<<VLENGTHSH));
						efree((CHAR *)newstrings);
					} else if ((var->type&VTYPE) == VFLOAT)
					{
						newfloats = (float *)emalloc(tech->layercount * (sizeof (float)),
							tech->cluster);
						for(k=0; k<tech->layercount; k++)
							newfloats[k] = io_techlayervariables[i].defaultfloat;
						for(k=0; k<getlength(var); k++)
							newfloats[io_techlayerfixes[j].arrangement[k]] =
								((float *)var->addr)[k];
						(void)setvalkey((INTBIG)tech, VTECHNOLOGY, io_techlayervariables[i].key,
							(INTBIG)newfloats, (var->type&VTYPE)|VISARRAY|(tech->layercount<<VLENGTHSH));
						efree((CHAR *)newfloats);
					} else
					{
						newints = (INTBIG *)emalloc(tech->layercount * SIZEOFINTBIG,
							tech->cluster);
						for(k=0; k<tech->layercount; k++)
							newints[k] = io_techlayervariables[i].defaultint;
						for(k=0; k<getlength(var); k++)
							newints[io_techlayerfixes[j].arrangement[k]] =
								((INTBIG *)var->addr)[k];
						(void)setvalkey((INTBIG)tech, VTECHNOLOGY, io_techlayervariables[i].key,
							(INTBIG)newints, (var->type&VTYPE)|VISARRAY|(tech->layercount<<VLENGTHSH));
						efree((CHAR *)newints);
					}
				}
				continue;
			}

			/* unable to fix: issue a warning */
			ttyputmsg(_("Warning: library %s has %s in technology %s which are inconsistent"),
				lib->libname, TRANSLATE(io_techlayervariables[i].meaning), tech->techname);
			if (io_techlayervariables[i].variabletype != VARTYPELAYERNONDRC)
			{
				if (!factorywarned)
				{
					factorywarned = TRUE;
					ttyputmsg(_("Should do a 'Factory Reset' of design rules in 'DRC Rules' dialog"));
				}
			}
		}
	}
}

void io_fixrtree(RTNODE *rtree)
{
	REGISTER INTBIG i;
	REGISTER GEOM *geom;
	REGISTER RTNODE *subrt;

	if (rtree->total <= 0) return;
	if (rtree->flag != 0)
	{
		geom = (GEOM *)rtree->pointers[0];
		rtree->lowx = geom->lowx;   rtree->highx = geom->highx;
		rtree->lowy = geom->lowy;   rtree->highy = geom->highy;
		for(i=1; i<rtree->total; i++)
		{
			geom = (GEOM *)rtree->pointers[i];
			if (geom->lowx < rtree->lowx) rtree->lowx = geom->lowx;
			if (geom->highx > rtree->highx) rtree->highx = geom->highx;
			if (geom->lowy < rtree->lowy) rtree->lowy = geom->lowy;
			if (geom->highy > rtree->highy) rtree->highy = geom->highy;
		}
	} else
	{
		subrt = (RTNODE *)rtree->pointers[0];
		io_fixrtree(subrt);
		rtree->lowx = subrt->lowx;   rtree->highx = subrt->highx;
		rtree->lowy = subrt->lowy;   rtree->highy = subrt->highy;
		for(i=1; i<rtree->total; i++)
		{
			subrt = (RTNODE *)rtree->pointers[i];
			io_fixrtree(subrt);
			if (subrt->lowx < rtree->lowx) rtree->lowx = subrt->lowx;
			if (subrt->highx > rtree->highx) rtree->highx = subrt->highx;
			if (subrt->lowy < rtree->lowy) rtree->lowy = subrt->lowy;
			if (subrt->highy > rtree->highy) rtree->highy = subrt->highy;
		}
	}
}

/*
 * routine to convert port names that have changed (specifically those
 * in the Schematics technology).  Given the port name on a node proto,
 * returns the correct port (or NOPORTPROTO if not known).
 */
PORTPROTO *io_convertoldportname(CHAR *portname, NODEPROTO *np)
{
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG len;
	CHAR truename[300];

	if (np->primindex == 0) return(NOPORTPROTO);
	if (np == sch_sourceprim || np == sch_meterprim)
	{
		if (namesame(portname, x_("top")) == 0)
			return(np->firstportproto);
		if (namesame(portname, x_("bottom")) == 0)
			return(np->firstportproto->nextportproto);
	}
	if (np == sch_twoportprim)
	{
		if (namesame(portname, x_("upperleft")) == 0)
			return(np->firstportproto);
		if (namesame(portname, x_("lowerleft")) == 0)
			return(np->firstportproto->nextportproto);
		if (namesame(portname, x_("upperright")) == 0)
			return(np->firstportproto->nextportproto->nextportproto);
		if (namesame(portname, x_("lowerright")) == 0)
			return(np->firstportproto->nextportproto->nextportproto->nextportproto);
	}

	/* some technologies switched from ports ending in "-bot" to the ending "-bottom" */
	len = estrlen(portname) - 4;
	if (len > 0 && namesame(&portname[len], x_("-bot")) == 0)
	{
		estrcpy(truename, portname);
		estrcat(truename, x_("tom"));
		pp = getportproto(np, truename);
		if (pp != NOPORTPROTO) return(pp);
	}
	return(NOPORTPROTO);
}

/*
 * Routine to determine the area of cell "np" that is to be printed.
 * Returns true if the area cannot be determined.
 */
BOOLEAN io_getareatoprint(NODEPROTO *np, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, BOOLEAN reduce)
{
	REGISTER INTBIG wid, hei, *curstate;
	INTBIG hlx, hhx, hly, hhy;
	REGISTER NODEPROTO *onp;

	curstate = io_getstatebits();
	us_fullview(np, lx, hx, ly, hy);

	/* extend it and make it square */
	wid = *hx - *lx;
	hei = *hy - *ly;
	if (reduce)
	{
		*lx -= wid/8;   *hx += wid/8;
		*ly -= hei/8;   *hy += hei/8;
		us_squarescreen(el_curwindowpart, NOWINDOWPART, FALSE, lx, hx, ly, hy, 0);
	}

	if ((curstate[0]&PLOTFOCUS) != 0)
	{
		if ((curstate[0]&PLOTFOCUSDPY) != 0)
		{
			*lx = el_curwindowpart->screenlx;
			*hx = el_curwindowpart->screenhx;
			*ly = el_curwindowpart->screenly;
			*hy = el_curwindowpart->screenhy;
		} else
		{
			onp = (NODEPROTO *)asktool(us_tool, x_("get-highlighted-area"),
				(INTBIG)&hlx, (INTBIG)&hhx, (INTBIG)&hly, (INTBIG)&hhy);
			if (onp == NONODEPROTO)
				ttyputerr(_("Warning: no highlighted area; printing entire cell")); else
			{
				if (hhx == hlx || hhy == hly)
				{
					ttyputerr(_("Warning: no highlighted area; highlight area and reissue command"));
					return(TRUE);
				}
				*lx = hlx;   *hx = hhx;
				*ly = hly;   *hy = hhy;
			}
		}
	}
	return(FALSE);
}

/******************** LAYER ORDERING FOR GRAPHIC COPY/PRINT ********************/

INTBIG io_setuptechorder(TECHNOLOGY *tech)
{
	REGISTER INTBIG i, j, k, l, m, *neworder, newamount;
	REGISTER INTBIG *layers;
	INTBIG order[LFNUMLAYERS];
	REGISTER VARIABLE *var;

	/* determine order of overlappable layers in current technology */
	io_maxlayers = 0;
	i = tech->layercount;
	var = getval((INTBIG)tech, VTECHNOLOGY, VINTEGER|VISARRAY,
		x_("TECH_layer_function"));
	if (var != NOVARIABLE)
	{
		layers = (INTBIG *)var->addr;
		for(j=0; j<LFNUMLAYERS; j++)
			order[j] = layerfunctionheight(j);
		for(j=0; j<LFNUMLAYERS; j++)
		{
			for(k=0; k<LFNUMLAYERS; k++)
			{
				if (order[k] != j) continue;
				for(l=0; l<i; l++)
				{
					if ((layers[l]&LFTYPE) != k) continue;
					if (io_maxlayers >= io_mostlayers)
					{
						newamount = io_mostlayers * 2;
						if (newamount <= 0) newamount = 10;
						if (newamount < io_maxlayers) newamount = io_maxlayers;
						neworder = (INTBIG *)emalloc(newamount * SIZEOFINTBIG, io_tool->cluster);
						if (neworder == 0) return(io_maxlayers);
						for(m=0; m<io_maxlayers; m++)
							neworder[m] = io_overlaporder[m];
						if (io_mostlayers > 0) efree((CHAR *)io_overlaporder);
						io_overlaporder = neworder;
						io_mostlayers = newamount;
					}
					io_overlaporder[io_maxlayers++] = l;
				}
				break;
			}
		}
	}
	return(io_maxlayers);
}

/*
 * Routine to return the layer in plotting position "i" (from 0 to the value returned
 * by "io_setuptechorder" - 1).
 */
INTBIG io_nextplotlayer(INTBIG i)
{
	return(io_overlaporder[i]);
}

/******************** MATH HELPERS ********************/

/*
 * This routine is used by "ioedifo.c" and "routmaze.c".
 */
void io_compute_center(INTBIG xc, INTBIG yc, INTBIG x1, INTBIG y1,
	INTBIG x2, INTBIG y2, INTBIG *cx, INTBIG *cy)
{
	int r, dx, dy, a1, a2, a;
	double pie, theta, radius, Dx, Dy;

	/* reconstruct angles to p1 and p2 */
	Dx = x1 - xc;
	Dy = y1 - yc;
	radius = sqrt(Dx * Dx + Dy * Dy);
	r = rounddouble(radius);
	a1 = (int)io_calc_angle(r, (double)(x1 - xc), (double)(y1 - yc));
	a2 = (int)io_calc_angle(r, (double)(x2 - xc), (double)(y2 - yc));
	if (a1 < a2) a1 += 3600;
	a = (a1 + a2) >> 1;
	pie = acos(-1.0);
	theta = (double) a *pie / 1800.0;	/* in radians */
	Dx = radius * cos(theta);
	Dy = radius * sin(theta);
	dx = rounddouble(Dx);
	dy = rounddouble(Dy);
	*cx = xc + dx;
	*cy = yc + dy;
}

double io_calc_angle(double r, double dx, double dy)
{
	double ratio, a1, a2;

	ratio = 1800.0 / EPI;
	a1 = acos(dx/r) * ratio;
	a2 = asin(dy/r) * ratio;
	if (a2 < 0.0) return(3600.0 - a1);
	return(a1);
}
