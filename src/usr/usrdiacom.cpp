/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrdiacom.cpp
 * Special command dialogs
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
#include "egraphics.h"
#include "usr.h"
#include "drc.h"
#include "network.h"
#include "usrdiacom.h"
#include "usrtrack.h"
#include "efunction.h"
#include "tecart.h"
#include "tecschem.h"
#include "tecgen.h"
#include "tecmocmos.h"
#include "tecmocmossub.h"
#include "usredtec.h"
#include "eio.h"
#include "sim.h"
#include "edialogs.h"
#include "conlay.h"
#include <math.h>

#ifdef USEQT
#  include <qapplication.h>
#  include <qprogressdialog.h>
#endif
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C" {
#endif
	extern COMCOMP us_colorwritep, us_colorreadp;
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

struct butlist
{
	UINTBIG value;
	INTBIG  button;
};

/* icons for text and port dialogs */
static UCHAR1 us_icon200[] =
{
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 040, 0, 0200, 0200, 060, 0, 0200, 0203, 0370, 0, 0200,
	0200, 060, 0, 0200, 0200, 040, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0,
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 010, 0, 0200, 0200, 010, 0, 0200,
	0200, 010, 0, 0200, 0200, 010, 0, 0200, 0200, 076, 0, 0200, 0200, 034, 0, 0200,
	0200, 010, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0
};
static UCHAR1 us_icon201[] =
{
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 010, 0, 0200, 0200, 034, 0, 0200,
	0200, 076, 0, 0200, 0200, 010, 0, 0200, 0200, 010, 0, 0200, 0200, 010, 0, 0200,
	0200, 010, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0,
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 010, 0200, 0200, 0, 014, 0200, 0200, 0, 0376, 0200,
	0200, 0, 014, 0200, 0200, 0, 010, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0
};
static UCHAR1 us_icon202[] =
{
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0210, 0, 0, 0200, 0230, 0, 0, 0200, 0277, 0200, 0, 0200,
	0230, 0, 0, 0200, 0210, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0,
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0100, 0200,
	0200, 0, 040, 0200, 0200, 0, 022, 0200, 0200, 0, 016, 0200, 0200, 0, 016, 0200,
	0200, 0, 036, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0
};
static UCHAR1 us_icon203[] =
{
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0201, 0, 0, 0200,
	0202, 0, 0, 0200, 0244, 0, 0, 0200, 0270, 0, 0, 0200, 0270, 0, 0, 0200,
	0274, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0,
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 0, 036, 0200, 0200, 0, 016, 0200,
	0200, 0, 016, 0200, 0200, 0, 022, 0200, 0200, 0, 040, 0200, 0200, 0, 0100, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0
};
static UCHAR1 us_icon204[] =
{
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0274, 0, 0, 0200, 0270, 0, 0, 0200,
	0270, 0, 0, 0200, 0244, 0, 0, 0200, 0202, 0, 0, 0200, 0201, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0,
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0200, 010, 0, 0200, 0200, 034, 0, 0200,
	0200, 076, 0, 0200, 0210, 010, 010, 0200, 0230, 010, 014, 0200, 0277, 0200, 0376, 0200,
	0230, 010, 014, 0200, 0210, 010, 010, 0200, 0200, 076, 0, 0200, 0200, 034, 0, 0200,
	0200, 010, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0
};
static UCHAR1 us_icon205[] =
{
	0377, 0377, 0377, 0200, 0200, 0, 0, 0200, 0274, 0, 0, 0200, 0270, 0, 0, 0200,
	0270, 0, 0, 0200, 0244, 0, 0, 0200, 0202, 0, 0, 0200, 0201, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200, 0200, 0, 0, 0200,
	0200, 0, 0, 0200, 0200, 0, 0, 0200, 0377, 0377, 0377, 0200, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#define NUMPORTCHARS 16
static CHAR *us_exportcharnames[NUMPORTCHARS] = {N_("Unknown"), N_("Input"), N_("Output"), N_("Bidirectional"),
	N_("Power"), N_("Ground"), N_("Reference Output"), N_("Reference Input"), N_("Reference Base"),
	N_("Clock"), N_("Clock phase 1"), N_("Clock phase 2"), N_("Clock phase 3"), N_("Clock phase 4"),
	N_("Clock phase 5"), N_("Clock phase 6")};
static UINTBIG us_exportcharlist[NUMPORTCHARS] = {0, INPORT, OUTPORT, BIDIRPORT, PWRPORT, GNDPORT,
	REFOUTPORT, REFINPORT, REFBASEPORT, CLKPORT, C1PORT, C2PORT, C3PORT, C4PORT, C5PORT, C6PORT};
static CHAR *us_exportintnames[NUMPORTCHARS] = {x_(""), x_("input"), x_("output"), x_("bidirectional"),
	x_("power"), x_("ground"), x_("refout"), x_("refin"), x_("refbase"),
	x_("clock"), x_("clock1"), x_("clock2"), x_("clock3"), x_("clock4"), x_("clock5"),
	x_("clock6")};
static CHAR *us_rotationtypes[4] = {N_("None"), N_("90 degrees counterclockwise"),
	N_("180 degrees"), N_("90 degrees clockwise")};
static CHAR *us_unitnames[] = {N_("None"), N_("Resistance"), N_("Capacitance"),
	N_("Inductance"), N_("Current"), N_("Voltage"), N_("Distance"), N_("Time")};

/* the entries in this array must align with the constants in the "INTERNALRESUNITS" field */
CHAR *us_resistancenames[4] = {N_("Ohms"), N_("Kilo-ohms"), N_("Mega-ohms"),
	N_("Giga-Ohms")};

/* the entries in this array must align with the constants in the "INTERNALCAPUNITS" field */
CHAR *us_capacitancenames[6] = {N_("Farads"), N_("Milli-farads"), N_("Micro-farads"),
	N_("Nano-farads"), N_("Pico-farads"), N_("Femto-farads")};

/* the entries in this array must align with the constants in the "INTERNALINDUNITS" field */
CHAR *us_inductancenames[4] = {N_("Henrys"), N_("Milli-henrys"), N_("Micro-henrys"),
	N_("Nano-henrys")};

/* the entries in this array must align with the constants in the "INTERNALCURUNITS" field */
CHAR *us_currentnames[3] = {N_("Amps"), N_("Milli-amps"), N_("Micro-amps")};

/* the entries in this array must align with the constants in the "INTERNALVOLTUNITS" field */
CHAR *us_voltagenames[4] = {N_("Kilo-vols"), N_("Volts"), N_("Milli-volts"),
	N_("Micro-volts")};

/* the entries in this array must align with the constants in the "INTERNALTIMEUNITS" field */
CHAR *us_timenames[6] = {N_("Seconds"), N_("Milli-seconds"), N_("Micro-seconds"),
	N_("Nano-seconds"), N_("Pico-seconds"), N_("Femto-seconds")};

static INTBIG us_colorvaluelist[25] =
{
	COLORT1,		/* overlappable 1 */
	COLORT2,		/* overlappable 2 */
	COLORT3,		/* overlappable 3 */
	COLORT4,		/* overlappable 4 */
	COLORT5,		/* overlappable 5 */
	WHITE,			/* white */
	BLACK,			/* black */
	RED,			/* red */
	BLUE,			/* blue */
	GREEN,			/* green */
	CYAN,			/* cyan */
	MAGENTA,		/* magenta */
	YELLOW,			/* yellow */
	GRAY,			/* gray */
	ORANGE,			/* orange */
	PURPLE,			/* purple */
	BROWN,			/* brown */
	LGRAY,			/* light gray */
	DGRAY,			/* dark gray */
	LRED,			/* light red */
	DRED,			/* dark red */
	LGREEN,			/* light green */
	DGREEN,			/* dark green */
	LBLUE,			/* light blue */
	DBLUE			/* dark blue */
};

static CHAR      *us_lastplacetextmessage = 0;
static CHAR      *us_lastfindtextmessage = 0;
static CHAR      *us_lastreplacetextmessage = 0;
static CHAR      *us_returneddialogstring = 0;
static NODEPROTO *us_oldcellprotos;
static INTBIG     us_showoldversions;				/* nonzero if cell lists include old versions */
static INTBIG     us_showcellibrarycells;			/* nonzero if cell lists include cell-library cells */
static INTBIG     us_showonlyrelevantcells;			/* nonzero if cell lists exclude cells from other views */
static INTBIG     us_defshowoldversions = 1;		/* default state of "us_showoldversions" */
static INTBIG     us_defshowcellibrarycells = 1;	/* default state of "us_showcellibrarycells" */
static INTBIG     us_defshowonlyrelevantcells = 0;	/* default state of "us_showonlyrelevantcells" */
static LIBRARY   *us_curlib;
static void      *us_trackingdialog;				/* dialog when tracking cursor */

/* prototypes for local routines */
static BOOLEAN    us_oldcelltopofcells(CHAR**);
static CHAR      *us_oldcellnextcells(void);
static CHAR     **us_languagechoices(void);
static void       us_widlendlog(NODEINST *ni);
static void       us_resistancedlog(GEOM*, VARIABLE*);
static void       us_capacitancedlog(GEOM*, VARIABLE*);
static void       us_inductancedlog(GEOM*, VARIABLE*);
static void       us_areadlog(NODEINST*);
static BOOLEAN    us_showforeignlicense(FILE *io, INTBIG section, INTBIG item, void *dia);
static INTBIG     us_makeviewlist(CHAR ***viewlist);
static void       us_setpopupface(INTBIG popupitem, INTBIG face, BOOLEAN init, void *dia);
static INTBIG     us_getpopupface(INTBIG popupitem, void *dia);
static INTBIG     us_scalabletransdlog(void);
static CHAR     **us_makelibrarylist(INTBIG *total, LIBRARY *clib, INTBIG *current);
static NODEPROTO *us_getselectedcell(void *dia);

/*
 * Routine to free all memory associated with this module.
 */
void us_freediacommemory(void)
{
	if (us_lastplacetextmessage != 0) efree((CHAR *)us_lastplacetextmessage);
	if (us_lastfindtextmessage != 0) efree((CHAR *)us_lastfindtextmessage);
	if (us_lastreplacetextmessage != 0) efree((CHAR *)us_lastreplacetextmessage);
	if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
}

/*
 * Routines for listing all cells in library "us_curlib".
 * If "us_showoldversions" is nonzero, show old versions.
 * If "us_showcellibrarycells" is nonzero, show cells that are part of cell libraries.
 * If "us_showonlyrelevantcells" is nonzero, show only cells of the same view as the current.
 */
BOOLEAN us_oldcelltopofcells(CHAR **c)
{
	Q_UNUSED( c );
	us_oldcellprotos = us_curlib->firstnodeproto;
	return(TRUE);
}

CHAR *us_oldcellnextcells(void)
{
	REGISTER NODEPROTO *thisnp;
	REGISTER LIBRARY *savelibrary;
	REGISTER VIEW *view;
	REGISTER CHAR *ret;

	while (us_oldcellprotos != NONODEPROTO)
	{
		thisnp = us_oldcellprotos;
		us_oldcellprotos = us_oldcellprotos->nextnodeproto;
		if (us_showoldversions == 0 && thisnp->newestversion != thisnp) continue;
		if (us_showcellibrarycells == 0 && (thisnp->userbits&INCELLLIBRARY) != 0)
			continue;
		if (us_showonlyrelevantcells != 0 && us_curlib->curnodeproto != NONODEPROTO)
		{
			if (el_curlib->curnodeproto == NONODEPROTO) view = el_unknownview; else
				view = el_curlib->curnodeproto->cellview;
			if (view != el_unknownview)
			{
				if (view == el_schematicview)
				{
					/* schematics: allow schematics or icons */
					if (thisnp->cellview != el_schematicview &&
						thisnp->cellview != el_iconview) continue;
				} else
				{
					if (thisnp->cellview != view) continue;
				}
			}
		}
		savelibrary = el_curlib;
		el_curlib = us_curlib;
		ret = describenodeproto(thisnp);
		el_curlib = savelibrary;
		return(ret);
	}
	return(0);
}

/*
 * Helper routine to return a translated array of strings describing the 4 language
 * choices (none, TCL, LISP, Java).
 */
CHAR **us_languagechoices(void)
{
	static CHAR *languages[] = {N_("Not Code"), N_("TCL"), N_("LISP"), N_("Java")};
	static CHAR *newlang[4];
	REGISTER INTBIG i;

#if LANGTCL == 0
	languages[1] = N_("TCL (not available)");
#endif
#if LANGLISP == 0
	languages[2] = N_("LISP (not available)");
#endif
#if LANGJAVA == 0
	languages[3] = N_("Java (not available)");
#endif
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(languages[i]);
	return(newlang);
}

/*
 * Routine to make a sorted list of libraries and return their names.
 * Returns the number of libraries in "total".  The index of library "clib"
 * is returned in "current".
 */
CHAR **us_makelibrarylist(INTBIG *total, LIBRARY *clib, INTBIG *current)
{
	REGISTER LIBRARY *lib;
	REGISTER CHAR **liblist=0;
	REGISTER INTBIG i, count;

	i = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if ((lib->userbits&HIDDENLIBRARY) == 0) i++;
	*total = i;
	*current = -1;
	if (i > 0)
	{
		liblist = (CHAR **)emalloc(i * (sizeof (CHAR *)), el_tempcluster);
		if (liblist == 0)
		{
			*total = 0;
			return(0);
		}
		count = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			if ((lib->userbits&HIDDENLIBRARY) == 0) liblist[count++] = lib->libname;
		esort(liblist, count, sizeof (CHAR *), sort_stringascending);
		for(i=0; i<count; i++)
			if (namesame(liblist[i], clib->libname) == 0) *current = i;
	}
	return(liblist);
}

/*
 * Routine to build a list of views in "viewlist" and return its length (0 on error).
 * The list must be deallocated when done (but not the individual elements).
 */
INTBIG us_makeviewlist(CHAR ***viewlist)
{
	REGISTER INTBIG viewcount, i, firstsview;
	REGISTER VIEW *v;

	for(viewcount = 0, v = el_views; v != NOVIEW; v = v->nextview) viewcount++;
	*viewlist = (CHAR **)emalloc(viewcount * (sizeof (CHAR *)), el_tempcluster);
	if (*viewlist == 0) return(0);
	for(v = el_views; v != NOVIEW; v = v->nextview) v->temp1 = 0;
	i = 0;
	(*viewlist)[i++] = el_layoutview->viewname;      el_layoutview->temp1 = 1;
	(*viewlist)[i++] = el_schematicview->viewname;   el_schematicview->temp1 = 1;
	firstsview = i;
	for(v = el_views; v != NOVIEW; v = v->nextview)
		if (v->temp1 == 0)
			(*viewlist)[i++] = v->viewname;
	esort(&(*viewlist)[firstsview], viewcount-firstsview, sizeof (CHAR *), sort_stringascending);
	return(viewcount);
}

/****************************** PROGRESS DIALOGs ******************************/

/* Progress (extended) */
static DIALOGITEM us_eprogressdialogitems[] =
{
 /*  1 */ {0, {56,8,73,230}, PROGRESS, x_("")},
 /*  2 */ {0, {32,8,48,230}, MESSAGE, N_("Reading file...")},
 /*  3 */ {0, {8,8,24,230}, MESSAGE, x_("")}
};
DIALOG us_eprogressdialog = {{50,75,135,314}, 0, 0, 3, us_eprogressdialogitems, x_("eprogress"), 0};

/* Progress (simple) */
static DIALOGITEM us_progressdialogitems[] =
{
 /*  1 */ {0, {32,8,49,230}, PROGRESS, x_("")},
 /*  2 */ {0, {8,8,24,230}, MESSAGE, N_("Reading file...")}
};
DIALOG us_progressdialog = {{50,75,112,314}, 0, 0, 2, us_progressdialogitems, x_("progress"), 0};

/* special items for the "Progress" dialogs: */
#define PROG_PROGRESS    1      /* indicator (progress) */
#define PROG_LABEL       2      /* label (message) */
#define PROG_CAPTION     3      /* capture (message) */

void *DiaInitProgress(CHAR *label, CHAR *caption)
{
#ifdef USEQT
	QProgressDialog *d = new QProgressDialog( qApp->mainWidget(), 0, FALSE );
	if (caption)
	{
		QString qcap = QString::fromLocal8Bit( caption );
		d->setCaption( qcap );
	}
	if (!label) label = _("Reading file...");
	QString qlab = QString::fromLocal8Bit( label );
	d->setLabelText( qlab );
	qApp->processEvents();
	return d;
#else
	void *dia;

	if (caption != 0)
	{
		dia = DiaInitDialog(&us_eprogressdialog);
		DiaSetText( dia, PROG_CAPTION, caption );
	} else
	{
		dia = DiaInitDialog(&us_progressdialog);
	}
	if (!label) label = _("Reading file...");
	DiaSetText( dia, PROG_LABEL, label );
	return(dia);
#endif
}

void DiaSetProgress(void *dia, INTBIG progress, INTBIG totalSteps)
{
#ifdef USEQT
	QProgressDialog *d = (QProgressDialog*)dia;
	if (totalSteps != d->totalSteps() || progress == 0)
	{
		d->setTotalSteps( totalSteps );
	}
# ifdef MACOSX
	else if (progress > 0 && progress < totalSteps &&
		progress >= d->progress() && progress < d->progress() + totalSteps/50)
	{
		return;
	}
# endif
	d->setProgress( progress );
	qApp->processEvents();
	if (d->wasCancelled()) el_pleasestop = 1;
#else
	progress = (totalSteps > 0 ? progress*100L/totalSteps : 100);
	DiaPercent(dia, 1, progress);
#endif
}

void DiaSetTextProgress(void *dia, CHAR *label)
{
#ifdef USEQT
	QProgressDialog *d = (QProgressDialog*)dia;
	QString qlab = QString::fromLocal8Bit( label );
	d->setLabelText( qlab );
	if (d->wasCancelled()) el_pleasestop = 1;
#else
	DiaSetText(dia, PROG_LABEL, label);
#endif
}

CHAR *DiaGetTextProgress(void *dia)
{
#ifdef USEQT
	static CHAR buf[300];
	QProgressDialog *d = (QProgressDialog*)dia;
	QCString str = d->labelText().ascii(); /* Need to localize !! */
	estrcpy(buf, str);
	return buf;
#else
	return DiaGetText(dia, PROG_LABEL);
#endif
}

void  DiaSetCaptionProgress(void *dia, CHAR *caption)
{
#ifdef USEQT
	QProgressDialog *d = (QProgressDialog*)dia;
	QString qcap = QString::fromLocal8Bit( caption );
	d->setCaption( qcap );
	if (d->wasCancelled()) el_pleasestop = 1;
#else
	DiaSetText(dia, PROG_CAPTION, caption);
#endif
}

void DiaDoneProgress(void *dia)
{
#ifdef USEQT
	QProgressDialog *d = (QProgressDialog*)dia;
	delete d;
#else
	DiaDoneDialog(dia);
#endif
}

/****************************** 3D DEPTH DIALOG ******************************/

/* 3D Depth */
static DIALOGITEM us_3ddepthdialogitems[] =
{
 /*  1 */ {0, {552,300,576,380}, BUTTON, N_("OK")},
 /*  2 */ {0, {552,188,576,268}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,404,168}, SCROLL, x_("")},
 /*  4 */ {0, {32,184,544,388}, USERDRAWN, x_("")},
 /*  5 */ {0, {412,8,428,88}, MESSAGE, N_("Thickness:")},
 /*  6 */ {0, {412,96,428,168}, EDITTEXT, x_("0")},
 /*  7 */ {0, {8,8,24,324}, MESSAGE, x_("")},
 /*  8 */ {0, {556,9,572,161}, CHECK, N_("Use Perspective")},
 /*  9 */ {0, {436,8,452,88}, MESSAGE, N_("Height:")},
 /* 10 */ {0, {436,96,452,168}, EDITTEXT, x_("0")},
 /* 11 */ {0, {460,8,476,180}, MESSAGE, N_("Separate flat layers by:")},
 /* 12 */ {0, {480,96,496,168}, EDITTEXT, x_("0")},
 /* 13 */ {0, {508,28,532,148}, BUTTON, N_("Clean Up")}
};
static DIALOG us_3ddepthdialog = {{75,75,660,473}, N_("3D Options"), 0, 13, us_3ddepthdialogitems, 0, 0};

/* special items for the "3D depth" dialog: */
#define D3DD_LAYERLIST   3		/* list of layers (scroll) */
#define D3DD_LAYERVIEW   4		/* layer side-view (user item) */
#define D3DD_THICKNESS   6		/* layer thickness (edit text) */
#define D3DD_TECHNAME    7		/* technology name (message) */
#define D3DD_PERSPECTIVE 8		/* user perspective (check) */
#define D3DD_HEIGHT      10		/* layer height (edit text) */
#define D3DD_FLATSEP     12		/* flat-layer separation (edit text) */
#define D3DD_CLEANUP     13		/* clean up (button) */

RECTAREA  us_3dheightrect;
INTBIG    us_3dcurlayer;
float    *us_3dheight;
float    *us_3dthickness;
INTBIG   *us_3dlayerindex;
INTBIG    us_3dlayercount;
float     us_3dlowheight, us_3dhighheight;
INTBIG    us_3dchanged;

static void us_redraw3ddepth(RECTAREA *bigr, void *dia);
static void us_3ddepthstroke(INTBIG x, INTBIG y);

INTBIG us_3ddepthdlog(void)
{
	INTBIG itemHit;
	INTBIG x, y;
	CHAR line[20];
	REGISTER WINDOWPART *w;
	REGISTER INTBIG i, j, funct, functp, ypos;
	float thickness, height, newthick, newheight;
	REGISTER void *infstr, *dia;

	/* cache the heights and thicknesses */
	us_3dheight = (float *)emalloc(el_curtech->layercount * (sizeof (float)), el_tempcluster);
	if (us_3dheight == 0) return(0);
	us_3dthickness = (float *)emalloc(el_curtech->layercount * (sizeof (float)), el_tempcluster);
	if (us_3dthickness == 0) return(0);
	us_3dlayerindex = (INTBIG *)emalloc(el_curtech->layercount * SIZEOFINTBIG, el_tempcluster);
	if (us_3dlayerindex == 0) return(0);
	for(i=0; i<el_curtech->layercount; i++)
	{
		if (get3dfactors(el_curtech, i, &us_3dheight[i], &us_3dthickness[i]))
		{
			us_3dheight[i] = 0.0;
			us_3dthickness[i] = 0.0;
		}
	}

	/* determine which layers are useful */
	us_3dlayercount = 0;
	for(i=0; i<el_curtech->layercount; i++)
	{
		funct = layerfunction(el_curtech, i);
		if ((funct&LFPSEUDO) != 0) continue;
		us_3dlayerindex[us_3dlayercount++] = i;
	}

	/* display the 3D options dialog box */
	dia = DiaInitDialog(&us_3ddepthdialog);
	if (dia == 0) return(0);
	infstr = initinfstr();
	addstringtoinfstr(infstr, _("Layer heights for technology "));
	addstringtoinfstr(infstr, el_curtech->techname);
	DiaSetText(dia, D3DD_TECHNAME, returninfstr(infstr));
	if ((us_useroptions&NO3DPERSPECTIVE) == 0)
		DiaSetControl(dia, D3DD_PERSPECTIVE, 1);
	DiaSetText(dia, D3DD_FLATSEP, x_("1.0"));
	DiaDimItem(dia, D3DD_FLATSEP);
	DiaDimItem(dia, D3DD_CLEANUP);

	/* setup list of layer names */
	DiaInitTextDialog(dia, D3DD_LAYERLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	for(i=0; i<us_3dlayercount; i++)
		DiaStuffLine(dia, D3DD_LAYERLIST, layername(el_curtech, us_3dlayerindex[i]));
	DiaSelectLine(dia, D3DD_LAYERLIST, 0);
	us_3dcurlayer = us_3dlayerindex[0];
	esnprintf(line, 20, x_("%g"), us_3dthickness[us_3dcurlayer]);
	DiaSetText(dia, D3DD_THICKNESS, line);
	esnprintf(line, 20, x_("%g"), us_3dheight[us_3dcurlayer]);
	DiaSetText(dia, D3DD_HEIGHT, line);

	/* setup layer height area */
	DiaItemRect(dia, D3DD_LAYERVIEW, &us_3dheightrect);
	DiaRedispRoutine(dia, D3DD_LAYERVIEW, us_redraw3ddepth);
	for(i=0; i<el_curtech->layercount; i++)
	{
		height = us_3dheight[i] * 2.0f;
		thickness = us_3dthickness[i] * 2.0f;
		if (i == 0)
		{
			us_3dlowheight = height - thickness/2.0f;
			us_3dhighheight = height + thickness/2.0f;
		} else
		{
			if (height - thickness/2.0f < us_3dlowheight)
				us_3dlowheight = height - thickness/2.0f;
			if (height + thickness/2.0f > us_3dhighheight)
				us_3dhighheight = height + thickness/2.0f;
		}
	}
	us_3dlowheight -= 4.0f;
	us_3dhighheight += 4.0f;
	us_redraw3ddepth(&us_3dheightrect, dia);

	/* loop until done */
	us_3dchanged = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK) break;
		if (itemHit == D3DD_LAYERLIST)
		{
			j = DiaGetCurLine(dia, D3DD_LAYERLIST);
			us_3dcurlayer = us_3dlayerindex[j];
			esnprintf(line, 20, x_("%g"), us_3dthickness[us_3dcurlayer]);
			DiaSetText(dia, D3DD_THICKNESS, line);
			esnprintf(line, 20, x_("%g"), us_3dheight[us_3dcurlayer]);
			DiaSetText(dia, D3DD_HEIGHT, line);
			us_redraw3ddepth(&us_3dheightrect, dia);
			continue;
		}
		if (itemHit == D3DD_THICKNESS)
		{
			j = DiaGetCurLine(dia, D3DD_LAYERLIST);
			us_3dcurlayer = us_3dlayerindex[j];
			newthick = (float)eatof(DiaGetText(dia, D3DD_THICKNESS));
			if (newthick == us_3dthickness[us_3dcurlayer]) continue;
			us_3dchanged++;
			us_3dthickness[us_3dcurlayer] = newthick;
			us_redraw3ddepth(&us_3dheightrect, dia);
			continue;
		}
		if (itemHit == D3DD_HEIGHT)
		{
			j = DiaGetCurLine(dia, D3DD_LAYERLIST);
			us_3dcurlayer = us_3dlayerindex[j];
			newheight = (float)eatof(DiaGetText(dia, D3DD_HEIGHT));
			if (newheight == us_3dheight[us_3dcurlayer]) continue;
			us_3dchanged++;
			us_3dheight[us_3dcurlayer] = newheight;
			us_redraw3ddepth(&us_3dheightrect, dia);
			continue;
		}
		if (itemHit == D3DD_LAYERVIEW)
		{
			DiaGetMouse(dia, &x, &y);
			ypos = (INTBIG)(us_3dheightrect.bottom - (us_3dheight[us_3dcurlayer]*2.0 - us_3dlowheight) *
				(us_3dheightrect.bottom - us_3dheightrect.top) / (us_3dhighheight - us_3dlowheight));
			if (abs(ypos-y) > 5)
			{
				/* selecting a new layer */
				for(i=0; i<us_3dlayercount; i++)
				{
					j = us_3dlayerindex[i];
					ypos = (INTBIG)(us_3dheightrect.bottom - (us_3dheight[j]*2.0 - us_3dlowheight) *
						(us_3dheightrect.bottom - us_3dheightrect.top) / (us_3dhighheight - us_3dlowheight));
					if (abs(ypos-y) <= 5) break;
				}
				if (i >= us_3dlayercount) continue;
				us_3dcurlayer = us_3dlayerindex[i];
				DiaSelectLine(dia, D3DD_LAYERLIST, i);
				esnprintf(line, 20, x_("%g"), us_3dthickness[us_3dcurlayer]);
				DiaSetText(dia, D3DD_THICKNESS, line);
				esnprintf(line, 20, x_("%g"), us_3dheight[us_3dcurlayer]);
				DiaSetText(dia, D3DD_HEIGHT, line);
				us_redraw3ddepth(&us_3dheightrect, dia);
			} else
			{
				/* clicking on already-selected layer: drag it */
				us_trackingdialog = dia;
				DiaTrackCursor(dia, us_3ddepthstroke);
			}
			continue;
		}
		if (itemHit == D3DD_PERSPECTIVE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		/* copy regular layers to pseudo-layers */
		for(i=0; i<el_curtech->layercount; i++)
		{
			functp = layerfunction(el_curtech, i);
			if ((functp&LFPSEUDO) == 0) continue;

			/* pseudo layer found: look for real one */
			for(j=0; j<el_curtech->layercount; j++)
			{
				funct = layerfunction(el_curtech, j);
				if ((funct&LFPSEUDO) != 0) continue;
				if ((functp & ~LFPSEUDO) == funct)
				{
					us_3dheight[i] = us_3dheight[j];
					us_3dthickness[i] = us_3dthickness[j];
					break;
				}
			}
		}
		set3dheight(el_curtech, us_3dheight);
		set3dthickness(el_curtech, us_3dthickness);
		j = us_useroptions;
		if (DiaGetControl(dia, D3DD_PERSPECTIVE) == 0) j |= NO3DPERSPECTIVE; else j &= ~NO3DPERSPECTIVE;
		if (j != us_useroptions)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, j, VINTEGER);
	}
	DiaDoneDialog(dia);
	efree((CHAR *)us_3dheight);
	efree((CHAR *)us_3dthickness);
	if (itemHit != CANCEL)
	{
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (w->redisphandler != 0) (*w->redisphandler)(w);
		}
	}
	return(0);
}

void us_redraw3ddepth(RECTAREA *bigr, void *dia)
{
	REGISTER INTBIG i, ypos1, ypos2, ypos, layer;
	float height, thickness;
	INTBIG wid, hei;
	CHAR *pt;

	DiaFrameRect(dia, D3DD_LAYERVIEW, bigr);
	for(i=0; i<us_3dlayercount; i++)
	{
		layer = us_3dlayerindex[i];
		height = us_3dheight[layer] * 2.0f;
		thickness = us_3dthickness[layer] * 2.0f;
		ypos = (INTBIG)(bigr->bottom - (height - us_3dlowheight) *
			(bigr->bottom - bigr->top) / (us_3dhighheight - us_3dlowheight));
		if (layer == us_3dcurlayer)
		{
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left, ypos, bigr->left+8, ypos, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+4, ypos-4, bigr->left+4, ypos+4, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left, ypos-4, bigr->left+8, ypos+4, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+8, ypos-4, bigr->left, ypos+4, DLMODEON);
		}
		if (thickness == 0)
		{
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+10, ypos, bigr->left+70, ypos, DLMODEON);
		} else
		{
			ypos1 = (INTBIG)(bigr->bottom - (height - thickness/2 - us_3dlowheight) *
				(bigr->bottom - bigr->top) / (us_3dhighheight - us_3dlowheight) - 2.0f);
			ypos2 = (INTBIG)(bigr->bottom - (height + thickness/2 - us_3dlowheight) *
				(bigr->bottom - bigr->top) / (us_3dhighheight - us_3dlowheight) + 2.0f);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+10, ypos1, bigr->left+40, ypos1, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+40, ypos1, bigr->left+50, ypos, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+40, ypos2, bigr->left+50, ypos, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+10, ypos2, bigr->left+40, ypos2, DLMODEON);
			DiaDrawLine(dia, D3DD_LAYERVIEW, bigr->left+50, ypos, bigr->left+70, ypos, DLMODEON);
		}
		pt = layername(el_curtech, layer);
		DiaGetTextInfo(dia, pt, &wid, &hei);
		DiaPutText(dia, D3DD_LAYERVIEW, pt, bigr->left+70, ypos - hei/2);
	}
}

void us_3ddepthstroke(INTBIG x, INTBIG y)
{
	Q_UNUSED( x );
	REGISTER float height;
	CHAR line[20];

	height = ((us_3dheightrect.bottom - y) * (us_3dhighheight - us_3dlowheight) +
		(us_3dheightrect.bottom - us_3dheightrect.top)/2.0f) /
		(us_3dheightrect.bottom - us_3dheightrect.top) + us_3dlowheight;
	if (us_3dheight[us_3dcurlayer] == height / 2.0f) return;
	us_3dchanged++;
	us_3dheight[us_3dcurlayer] = height / 2.0f;
	esnprintf(line, 20, x_("%g"), us_3dheight[us_3dcurlayer]);   DiaSetText(us_trackingdialog, D3DD_HEIGHT, line);
	us_redraw3ddepth(&us_3dheightrect, us_trackingdialog);
}

/****************************** ABOUT ELECTRIC DIALOG ******************************/

/*
 * the list of contributors to Electric (not including Steven M. Rubin)
 */
HELPERS us_castofthousands[] =
{
	{x_("Philip Attfield"),			N_("Box merging")},
	{x_("Brett Bissinger"),		    N_("Node extraction")},
	{x_("Ron Bolton"),				N_("Mathematical help")},
	{x_("Robert Bosnyak"),			N_("Pads library")},
	{x_("Mark Brinsmead"),			N_("Mathematical help")},
	{x_("Stefano Concina"),			N_("Polygon clipping")},
	{x_("Jonathan Gainsley"),		N_("Testing and design")},
	{x_("Peter Gallant"),			N_("ALS simulator")},
	{x_("R. Brian Gardiner"),		N_("Electric lifeline")},
	{x_("T. J. Goodman"),			N_("Texsim output")},
	{x_("Gerrit Groenewold"),		N_("SPICE parts")},
	{x_("David Groulx"),		    N_("Node extraction")},
	{x_("D. Guptill"),			    N_("X-window help")},
	{x_("David Harris"),			N_("Color PostScript output")},
	{x_("Robert Hon"),				N_("CIF input parser")},
	{x_("Jason Imada"),				N_("ROM generator")},
	{x_("Sundaravarathan Iyengar"),	N_("nMOS PLA generator")},
	{x_("Allan Jost"),				N_("VHDL compiler help, X-window help")},
	{x_("Wallace Kroeker"),			N_("Digital filter technology, CMOS PLA generator")},
	{x_("Andrew Kostiuk"),			N_("VHDL compiler, Silicon Compiler")},
	{x_("Oliver Laumann"),			N_("ELK Lisp")},
	{x_("Glen Lawson"),				N_("Maze routing, GDS input, EDIF I/O")},
	{x_("Frank Lee"),				N_("ROM generator")},
	{x_("Neil Levine"),				N_("PADS output")},
	{x_("David Lewis"),				N_("Flat DRC checking")},
	{x_("Erwin Liu"),				N_("Schematic and Round CMOS technology help")},
	{x_("Dick Lyon"),				N_("MOSIS and Round CMOS technology help")},
	{x_("John Mohammed"),			N_("Mathematical help")},
	{x_("Mark Moraes"),				N_("Hierarchical DRC, X-window help")},
	{x_("Dmitry Nadezhin"),			N_("Qt port, simulation, networks, optimizations, development")},
	{x_("Sid Penstone"),			N_("SPICE, SILOS, GDS, Box merging, technologies")},
	{x_("J. P. Polonovski"),		N_("Memory allocation help")},
	{x_("Kevin Ryan"),			    N_("X-window help")},
	{x_("Nora Ryan"),				N_("Compaction, technology conversion")},
	{x_("Miguel Saro"),				N_("French translation")},
	{x_("Brent Serbin"),			N_("ALS simulator")},
	{x_("Lyndon Swab"),				N_("HPGL output, SPICE output help, technologies")},
	{x_("Brian W. Thomson"),		N_("Mimic stitcher, RSIM interface")},
	{x_("Burnie West"),				N_("Bipolar technology, EDIF output help")},
	{x_("Telle Whitney"),			N_("River router")},
	{x_("Rob Winstanley"),			N_("CIF input, RNL output")},
	{x_("Russell Wright"),			N_("SDF input, miscellaneous help")},
	{x_("David J. Yurach"),			N_("VHDL help")},
	{0, 0}
};

CHAR *us_gnucopying[] =
{
	x_("TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION"),
	x_(""),
	x_("0. This License applies to any program or other work which contains a notice placed by"),
	x_("the copyright holder saying it may be distributed under the terms of this General"),
	x_("Public License. The 'Program', below, refers to any such program or work, and a"),
	x_("'work based on the Program' means either the Program or any derivative work under"),
	x_("copyright law: that is to say, a work containing the Program or a portion of it,"),
	x_("either verbatim or with modifications and/or translated into another language."),
	x_("(Hereinafter, translation is included without limitation in the term 'modification'.)"),
	x_("Each licensee is addressed as 'you'."),
	x_(""),
	x_("Activities other than copying, distribution and modification are not covered by this"),
	x_("License; they are outside its scope. The act of running the Program is not restricted,"),
	x_("and the output from the Program is covered only if its contents constitute a work based"),
	x_("on the Program (independent of having been made by running the Program). Whether that"),
	x_("is true depends on what the Program does."),
	x_(""),
	x_("1. You may copy and distribute verbatim copies of the Program's source code as you"),
	x_("receive it, in any medium, provided that you conspicuously and appropriately publish"),
	x_("on each copy an appropriate copyright notice and disclaimer of warranty; keep intact"),
	x_("all the notices that refer to this License and to the absence of any warranty; and"),
	x_("give any other recipients of the Program a copy of this License along with the Program."),
	x_(""),
	x_("You may charge a fee for the physical act of transferring a copy, and you may at your"),
	x_("option offer warranty protection in exchange for a fee."),
	x_(""),
	x_("2. You may modify your copy or copies of the Program or any portion of it, thus forming"),
	x_("a work based on the Program, and copy and distribute such modifications or work under"),
	x_("the terms of Section 1 above, provided that you also meet all of these conditions:"),
	x_(""),
	x_("*	a) You must cause the modified files to carry prominent notices stating that you"),
	x_("	changed the files and the date of any change."),
	x_(""),
	x_("*	b) You must cause any work that you distribute or publish, that in whole or"),
	x_("	in part contains or is derived from the Program or any part thereof, to be licensed"),
	x_("	as a whole at no charge to all third parties under the terms of this License."),
	x_(""),
	x_("*	c) If the modified program normally reads commands interactively when run, you"),
	x_("	must cause it, when started running for such interactive use in the most ordinary"),
	x_("	way, to print or display an announcement including an appropriate copyright notice"),
	x_("	and a notice that there is no warranty (or else, saying that you provide a warranty)"),
	x_("	and that users may redistribute the program under these conditions, and telling the"),
	x_("	user how to view a copy of this License. (Exception: if the Program itself is"),
	x_("	interactive but does not normally print such an announcement, your work based on the"),
	x_("	Program is not required to print an announcement.)"),
	x_(""),
	x_("These requirements apply to the modified work as a whole. If identifiable sections"),
	x_("of that work are not derived from the Program, and can be reasonably considered independent"),
	x_("and separate works in themselves, then this License, and its terms, do not apply to those"),
	x_("sections when you distribute them as separate works. But when you distribute the same"),
	x_("sections as part of a whole which is a work based on the Program, the distribution of"),
	x_("the whole must be on the terms of this License, whose permissions for other licensees"),
	x_("extend to the entire whole, and thus to each and every part regardless of who wrote it."),
	x_(""),
	x_("Thus, it is not the intent of this section to claim rights or contest your rights to"),
	x_("work written entirely by you; rather, the intent is to exercise the right to control"),
	x_("the distribution of derivative or collective works based on the Program."),
	x_(""),
	x_("In addition, mere aggregation of another work not based on the Program with the Program"),
	x_("(or with a work based on the Program) on a volume of a storage or distribution medium"),
	x_("does not bring the other work under the scope of this License."),
	x_(""),
	x_("3. You may copy and distribute the Program (or a work based on it, under Section 2)"),
	x_("in object code or executable form under the terms of Sections 1 and 2 above provided"),
	x_("that you also do one of the following:"),
	x_(""),
	x_("*	a) Accompany it with the complete corresponding machine-readable source code,"),
	x_("which must be distributed under the terms of Sections 1 and 2 above on a medium"),
	x_("customarily used for software interchange; or,"),
	x_(""),
	x_("*	b) Accompany it with a written offer, valid for at least three years, to give"),
	x_("any third party, for a charge no more than your cost of physically performing source"),
	x_("distribution, a complete machine-readable copy of the corresponding source code,"),
	x_("to be distributed under the terms of Sections 1 and 2 above on a medium customarily"),
	x_("used for software interchange; or,"),
	x_(""),
	x_("*	c) Accompany it with the information you received as to the offer to distribute"),
	x_("corresponding source code. (This alternative is allowed only for noncommercial"),
	x_("distribution and only if you received the program in object code or executable"),
	x_("form with such an offer, in accord with Subsection b above.)"),
	x_(""),
	x_("The source code for a work means the preferred form of the work for making"),
	x_("modifications to it. For an executable work, complete source code means all"),
	x_("the source code for all modules it contains, plus any associated interface"),
	x_("definition files, plus the scripts used to control compilation and installation"),
	x_("of the executable. However, as a special exception, the source code distributed"),
	x_("need not include anything that is normally distributed (in either source or binary"),
	x_("form) with the major components (compiler, kernel, and so on) of the operating"),
	x_("system on which the executable runs, unless that component itself accompanies the executable."),
	x_(""),
	x_("If distribution of executable or object code is made by offering access to copy"),
	x_("from a designated place, then offering equivalent access to copy the source code"),
	x_("from the same place counts as distribution of the source code, even though third"),
	x_("parties are not compelled to copy the source along with the object code."),
	x_(""),
	x_("4. You may not copy, modify, sublicense, or distribute the Program except as"),
	x_("expressly provided under this License. Any attempt otherwise to copy, modify,"),
	x_("sublicense or distribute the Program is void, and will automatically terminate your"),
	x_("rights under this License. However, parties who have received copies, or rights,"),
	x_("from you under this License will not have their licenses terminated so long as"),
	x_("such parties remain in full compliance."),
	x_(""),
	x_("5. You are not required to accept this License, since you have not signed it."),
	x_("However, nothing else grants you permission to modify or distribute the Program or"),
	x_("its derivative works. These actions are prohibited by law if you do not accept this"),
	x_("License. Therefore, by modifying or distributing the Program (or any work based on"),
	x_("the Program), you indicate your acceptance of this License to do so, and all its"),
	x_("terms and conditions for copying, distributing or modifying the Program or works based on it."),
	x_(""),
	x_("6. Each time you redistribute the Program (or any work based on the Program),"),
	x_("the recipient automatically receives a license from the original licensor to copy,"),
	x_("distribute or modify the Program subject to these terms and conditions. You may not"),
	x_("impose any further restrictions on the recipients' exercise of the rights granted"),
	x_("herein. You are not responsible for enforcing compliance by third parties to this License."),
	x_(""),
	x_("7. If, as a consequence of a court judgment or allegation of patent infringement"),
	x_("or for any other reason (not limited to patent issues), conditions are imposed"),
	x_("on you (whether by court order, agreement or otherwise) that contradict the conditions"),
	x_("of this License, they do not excuse you from the conditions of this License. If you"),
	x_("cannot distribute so as to satisfy simultaneously your obligations under this"),
	x_("License and any other pertinent obligations, then as a consequence you may not"),
	x_("distribute the Program at all. For example, if a patent license would not permit"),
	x_("royalty-free redistribution of the Program by all those who receive copies directly"),
	x_("or indirectly through you, then the only way you could satisfy both it and this"),
	x_("License would be to refrain entirely from distribution of the Program."),
	x_(""),
	x_("If any portion of this section is held invalid or unenforceable under any"),
	x_("particular circumstance, the balance of the section is intended to apply and"),
	x_("the section as a whole is intended to apply in other circumstances."),
	x_(""),
	x_("It is not the purpose of this section to induce you to infringe any patents"),
	x_("or other property right claims or to contest validity of any such claims; this"),
	x_("section has the sole purpose of protecting the integrity of the free software"),
	x_("distribution system, which is implemented by public license practices. Many"),
	x_("people have made generous contributions to the wide range of software distributed"),
	x_("through that system in reliance on consistent application of that system; it is"),
	x_("up to the author/donor to decide if he or she is willing to distribute software"),
	x_("through any other system and a licensee cannot impose that choice."),
	x_(""),
	x_("This section is intended to make thoroughly clear what is believed to be a"),
	x_("consequence of the rest of this License."),
	x_(""),
	x_("8. If the distribution and/or use of the Program is restricted in certain"),
	x_("countries either by patents or by copyrighted interfaces, the original copyright"),
	x_("holder who places the Program under this License may add an explicit geographical"),
	x_("distribution limitation excluding those countries, so that distribution is permitted"),
	x_("only in or among countries not thus excluded. In such case, this License incorporates"),
	x_("the limitation as if written in the body of this License."),
	x_(""),
	x_("9. The Free Software Foundation may publish revised and/or new versions of the"),
	x_("General Public License from time to time. Such new versions will be similar in"),
	x_("spirit to the present version, but may differ in detail to address new problems"),
	x_("or concerns."),
	x_(""),
	x_("Each version is given a distinguishing version number. If the Program specifies"),
	x_("a version number of this License which applies to it and 'any later version',"),
	x_("you have the option of following the terms and conditions either of that version"),
	x_("or of any later version published by the Free Software Foundation. If the Program"),
	x_("does not specify a version number of this License, you may choose any version ever"),
	x_("published by the Free Software Foundation."),
	x_(""),
	x_("10. If you wish to incorporate parts of the Program into other free programs"),
	x_("whose distribution conditions are different, write to the author to ask for"),
	x_("permission. For software which is copyrighted by the Free Software Foundation,"),
	x_("write to the Free Software Foundation; we sometimes make exceptions for this."),
	x_("Our decision will be guided by the two goals of preserving the free status of"),
	x_("all derivatives of our free software and of promoting the sharing and reuse of"),
	x_("software generally."),
	0
};
CHAR *us_gnuwarranty[] =
{
	x_("NO WARRANTY"),
	x_(""),
	x_("11. Because the program is licensed free of charge, there is no warranty for the"),
	x_("program, to the extent permitted by applicable law. Except when otherwise stated"),
	x_("in writing the copyright holders and/or other parties provide the program 'as is'"),
	x_("without warranty of any kind, either expressed or implied, including, but not"),
	x_("limited to, the implied warranties of merchantability and fitness for a particular"),
	x_("purpose. The entire risk as to the quality and performance of the program is with you."),
	x_("Should the program prove defective, you assume the cost of all necessary servicing,"),
	x_("repair or correction."),
	x_(""),
	x_("12. In no event unless required by applicable law or agreed to in writing will any"),
	x_("copyright holder, or any other party who may modify and/or redistribute the program"),
	x_("as permitted above, be liable to you for damages, including any general, special,"),
	x_("incidental or consequential damages arising out of the use or inability to use the"),
	x_("program (including but not limited to loss of data or data being rendered inaccurate"),
	x_("or losses sustained by you or third parties or a failure of the program to operate"),
	x_("with any other programs), even if such holder or other party has been advised of"),
	x_("the possibility of such damages."),
	0
};

/* icons for the "About Electric" dialog */

/* North America version */
static UCHAR1 us_icon130namerica[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,03,0377,0377, 0,07,0377,0377, 0,017,0377,0377, 0,034,0,0, 
	0,070,0,0, 0,0160,0,0, 0,0340,0,0, 01,0300,0,0, 
	03,0200,0,0, 03,0,0,0, 07,0,0,0, 06,0,0,0, 
	016,03,0374,0, 014,03,0374,0, 034,03,0374,0, 030,03,0374,0, 
	070,03,0374,0, 060,03,0374,0, 0160,03,0374,0, 0140,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
};
static UCHAR1 us_icon131namerica[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0377,0377,0300,0, 0377,0377,0340,0, 0377,0377,0360,0, 0,0,070,0, 
	0,0,034,0, 0,0,016,0, 0,0,07,0, 0,0,03,0200, 
	0,0,01,0300, 0,0,0,0300, 0,0,0,0340, 0,0,0,0140, 
	0,077,0300,0160, 0,077,0300,060, 0,077,0300,070, 0,077,0300,030, 
	0,077,0300,034, 0,077,0300,014, 0,077,0300,016, 0,077,0300,06, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
};
static UCHAR1 us_icon129namerica[] =
{
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,0,0,0, 0340,0,0,0, 
	0140,0,0,03, 0160,0,0,07, 060,0,0,017, 070,0,0,037, 
	030,0,0,077, 034,0,0,077, 014,0,0,077, 016,0,0,077, 
	06,0,0,077, 07,0,0,077, 03,0,0,077, 03,0200,0,077, 
	01,0300,0,0, 0,0340,0,0, 0,0160,0,0, 0,070,0,0, 
	0,034,0,0, 0,017,0377,0377, 0,07,0377,0377, 0,03,0377,0377, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};
static UCHAR1 us_icon132namerica[] =
{
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0300,0,0,06, 0340,0,0,016, 0360,0,0,014, 0370,0,0,034, 
	0374,0,0,030, 0374,0,0,070, 0374,0,0,060, 0374,0,0,0160, 
	0374,0,0,0140, 0374,0,0,0340, 0374,0,0,0300, 0374,0,01,0300, 
	0,0,03,0200, 0,0,07,0, 0,0,016,0, 0,0,034,0, 
	0,0,070,0, 0377,0377,0360,0, 0377,0377,0340,0, 0377,0377,0300,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};

/* Australian/New Zealand version */
static UCHAR1 us_icon130ausnz[] =
{
	0,0377,0377,0377, 07,0377,0377,0377, 017,0377,0377,0377, 037,0300,0,0, 
	076,0,0,0, 0174,0,0,0, 0170,0,0,0, 0160,0,0,0, 
	0360,0,0,0, 0360,0,070,0, 0340,0,0176,0, 0340,0,0377,0, 
	0340,0,0377,0, 0340,01,0376,0, 0340,03,0376,0, 0340,03,0374,0, 
	0340,07,0370,0, 0340,07,0370,0, 0340,017,0360,0, 0340,037,0360,0, 
	0340,037,0340,0, 0340,077,0340,0, 0340,077,0300,0, 0340,077,0200,0, 
	0340,077,0200,0, 0340,017,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
};
static UCHAR1 us_icon131ausnz[] =
{
	0377,0377,0377,0, 0377,0377,0377,0340, 0377,0377,0377,0360, 0,0,03,0370, 
	0,0,0,0174, 0,0,0,076, 0,0,0,036, 0,0,0,016, 
	0,0,0,017, 0,017,0,017, 0,037,0200,07, 0,0177,0200,07, 
	0,0177,0300,07, 0,0177,0300,07, 0,077,0340,07, 0,037,0340,07, 
	0,037,0360,07, 0,017,0360,07, 0,017,0370,07, 0,07,0370,07, 
	0,03,0374,07, 0,03,0376,07, 0,01,0377,07, 0,01,0376,07, 
	0,0,0376,07, 0,0,0374,07, 0,0,0160,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
};
static UCHAR1 us_icon129ausnz[] =
{
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,03, 0340,0,0,07, 0340,0,0,07, 
	0340,0,0,07, 0340,0,0,07, 0340,0,0,07, 0340,0,0,07, 
	0340,0,0,07, 0340,0,0,07, 0340,0,0,07, 0340,0,0,07, 
	0340,0,0,07, 0340,0,0,07, 0360,0,0,07, 0360,0,0,07, 
	0160,0,0,07, 0170,0,0,07, 0174,0,0,03, 076,0,0,0, 
	037,0300,0,0, 017,0377,0377,0377, 07,0377,0377,0377, 0,0377,0377,0377, 
};
static UCHAR1 us_icon132ausnz[] =
{
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0340,0,0,07, 0360,0,0,07, 0360,0,0,07, 
	0360,0,0,07, 0360,0,0,07, 0360,0,0,07, 0360,0,0,07, 
	0360,0,0,07, 0360,0,0,07, 0360,0,0,07, 0360,0,0,07, 
	0360,0,0,07, 0360,0,0,07, 0360,0,0,017, 0360,0,0,017, 
	0360,0,0,016, 0360,0,0,036, 0340,0,0,076, 0,0,0,0174, 
	0,0,03,0370, 0377,0377,0377,0360, 0377,0377,0377,0340, 0377,0377,0377,0, 
};

/* European version */
static UCHAR1 us_icon130europe[] =
{
	0,0,0,037, 0,0,0,037, 0,0,01,0377, 0,0,017,0377, 
	0,0,0177,0377, 0,01,0377,07, 0,07,0370,07, 0,017,0300,07, 
	0,037,0,07, 0,074,0,07, 0,0170,0,07, 0,0360,0,03, 
	01,0340,0,03, 03,0300,0,0, 03,0200,0,0, 07,0200,0,0, 
	07,0,0,0, 017,0,0,0, 017,0,0,0, 017,0,0,0, 
	037,0,0,0, 037,0,0,0, 037,0,0,0, 077,0,0,0, 
	077,0,0,0, 077,0,0,0, 077,0,0,0, 0377,01,0340,03, 
	0377,07,0370,016, 0340,07,0370,014, 0340,017,0374,032, 0340,017,0374,021, 
};
static UCHAR1 us_icon131europe[] =
{
	0370,0,0,0, 0370,0,0,0, 0377,0200,0,0, 0377,0360,0,0, 
	0377,0376,0,0, 0340,0377,0200,0, 0340,037,0340,0, 0340,03,0360,0, 
	0340,0,0370,0, 0340,0,074,0, 0340,0,036,0, 0300,0,017,0, 
	0300,0,07,0200, 0,0,03,0300, 0,0,01,0300, 0,0,01,0340, 
	0,0,0,0340, 0,0,0,0360, 0,0,0,0360, 0,0,0,0360, 
	0,0,0,0370, 0,0,0,0370, 0,0,0,0370, 0,0,0,0374, 
	0,0,0,0374, 0,0,0,0374, 0,0,0,0374, 0300,07,0200,0377, 
	0160,037,0340,0377, 020,037,0340,07, 030,077,0360,07, 010,077,0360,07, 
};
static UCHAR1 us_icon129europe[] =
{
	0340,017,0374,020, 0340,017,0374,030, 0340,07,0370,010, 0377,07,0370,016, 
	0377,01,0340,03, 077,0,0,0, 077,0,0,0, 077,0,0,0, 
	077,0,0,0, 037,0,0,0, 037,0,0,0, 037,0,0,0, 
	017,0,0,0, 017,0,0,0, 017,0,0,0, 07,0,0,0, 
	07,0200,0,0, 03,0200,0,0, 03,0300,0,0, 01,0340,0,03, 
	0,0360,0,03, 0,0170,0,07, 0,074,0,07, 0,037,0,07, 
	0,017,0300,07, 0,07,0370,07, 0,01,0377,07, 0,0,0177,0377, 
	0,0,017,0377, 0,0,01,0377, 0,0,0,037, 0,0,0,037, 
};
static UCHAR1 us_icon132europe[] =
{
	0210,077,0360,07, 0130,077,0360,07, 060,037,0340,07, 0160,037,0340,0377, 
	0300,07,0200,0377, 0,0,0,0374, 0,0,0,0374, 0,0,0,0374, 
	0,0,0,0374, 0,0,0,0370, 0,0,0,0370, 0,0,0,0370, 
	0,0,0,0360, 0,0,0,0360, 0,0,0,0360, 0,0,0,0340, 
	0,0,01,0340, 0,0,01,0300, 0,0,03,0300, 0300,0,07,0200, 
	0300,0,017,0, 0340,0,036,0, 0340,0,074,0, 0340,0,0370,0, 
	0340,03,0360,0, 0340,037,0340,0, 0340,0377,0200,0, 0377,0376,0,0, 
	0377,0360,0,0, 0377,0200,0,0, 0370,0,0,0, 0370,0,0,0, 
};

/* Italian version */
static UCHAR1 us_icon130italy[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 037,0377,0377,0377, 077,0377,0377,0377, 
	0177,0377,0377,0377, 0360,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,017,0,03, 
	0340,037,0200,07, 0340,077,0300,017, 0340,0177,0340,037, 0340,0177,0340,037, 
};
static UCHAR1 us_icon131italy[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0377,0377,0377,0370, 0377,0377,0377,0374, 
	0377,0377,0377,0376, 0,0,0,017, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0300,0,0360,07, 
	0340,01,0370,07, 0360,03,0374,07, 0370,07,0376,07, 0370,07,0376,07, 
};
static UCHAR1 us_icon129italy[] =
{
	0340,0177,0340,037, 0340,0177,0340,037, 0340,077,0300,017, 0340,037,0200,07, 
	0340,017,0,03, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0360,0,0,0, 0177,0377,0377,0377, 
	077,0377,0377,0377, 037,0377,0377,0377, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};
static UCHAR1 us_icon132italy[] =
{
	0370,07,0376,07, 0370,07,0376,07, 0360,03,0374,07, 0340,01,0370,07, 
	0300,0,0360,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,017, 0377,0377,0377,0376, 
	0377,0377,0377,0374, 0377,0377,0377,0370, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};

/* Indian version */
static UCHAR1 us_icon130india[] =
{
	0,0,0,077, 0,0,03,0377, 0,0,037,0377, 0,0,0377,0340, 
	0,03,0376,0, 0,017,0360,0, 0,037,0200,0, 0,076,0,03, 
	0,0174,0,017, 0,0370,0,037, 01,0360,0,037, 03,0340,0,077, 
	07,0300,0,077, 07,0200,0,077, 017,0,0,077, 016,0,0,037, 
	036,0,0,037, 034,0,0,017, 034,0,0,03, 074,0,0,0, 
	070,0,0,0, 070,0,0,0, 0170,0,0,0, 0160,0,0,0, 
	0160,0,0,0, 0160,0,0,0, 0360,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
};
static UCHAR1 us_icon131india[] =
{
	0374,0,0,0, 0377,0300,0,0, 0377,0370,0,0, 07,0377,0,0, 
	0,0177,0300,0, 0,017,0360,0, 0,01,0370,0, 0300,0,0174,0, 
	0360,0,076,0, 0370,0,037,0, 0370,0,017,0200, 0374,0,07,0300, 
	0374,0,03,0340, 0374,0,01,0340, 0374,0,0,0360, 0370,0,0,0160, 
	0370,0,0,0170, 0360,0,0,070, 0300,0,0,070, 0,0,0,074, 
	0,0,0,034, 0,0,0,034, 0,0,0,036, 0,0,0,016, 
	0,0,0,016, 0,0,0,016, 0,0,0,017, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
};
static UCHAR1 us_icon129india[] =
{
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0360,0,0,0, 0160,0,0,0, 0160,0,0170,0, 
	0160,0,0374,0, 0170,01,0376,0, 070,03,0377,0, 070,03,0377,0, 
	074,03,0377,0, 034,03,0377,0, 034,01,0376,0, 036,0,0374,0, 
	016,0,0170,0, 017,0,0,0, 07,0200,0,0, 07,0300,0,0, 
	03,0340,0,0, 01,0360,0,0, 0,0370,0,0, 0,0174,0,0, 
	0,076,0,0, 0,037,0200,0, 0,017,0360,0, 0,03,0376,0, 
	0,0,0377,0340, 0,0,037,0377, 0,0,03,0377, 0,0,0,077, 
};
static UCHAR1 us_icon132india[] =
{
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,017, 0,017,0,016, 0,037,0200,016, 
	0,077,0300,016, 0,0177,0340,036, 0,0177,0340,034, 0,0177,0340,034, 
	0,0177,0340,074, 0,077,0300,070, 0,037,0200,070, 0,017,0,0170, 
	0,0,0,0160, 0,0,0,0360, 0,0,01,0340, 0,0,03,0340, 
	0,0,07,0300, 0,0,017,0200, 0,0,037,0, 0,0,076,0, 
	0,0,0174,0, 0,01,0370,0, 0,017,0360,0, 0,0177,0300,0, 
	07,0377,0,0, 0377,0370,0,0, 0377,0300,0,0, 0374,0,0,0, 
};

/* Israel version */
static UCHAR1 us_icon130israel[] =
{
	0,0,0,077, 0,0,03,0377, 0,0,037,0377, 0,0,0377,0340, 
	0,03,0376,0, 0,017,0360,0, 0,037,0200,0, 0,076,0,0, 
	0,0174,0,0, 0,0370,0,0, 01,0360,0,0, 03,0340,0,0, 
	07,0300,0,0, 07,0200,0,0, 017,0,0,0, 016,0,0,0, 
	036,0,0,0, 034,0,036,0, 034,0,077,0, 074,0,077,0200, 
	070,0,077,0300, 070,0,077,0340, 0170,0,037,0340, 0160,0,017,0340, 
	0160,0,07,0340, 0160,0,03,0300, 0360,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
};
static UCHAR1 us_icon131israel[] =
{
	0374,0,0,0, 0377,0300,0,0, 0377,0370,0,0, 07,0377,0,0, 
	0,0177,0300,0, 0,017,0360,0, 0,01,0370,0, 0,0,0174,0, 
	0,0,076,0, 0,0,037,0, 0,0,017,0200, 0,0,07,0300, 
	0,0,03,0340, 0,0,01,0340, 0,0,0,0360, 0,0,0,0160, 
	0,0,0,0170, 0,0,0,070, 0,0,0,070, 0,0374,0,074, 
	01,0376,0,034, 03,0377,0,034, 03,0377,0,036, 03,0377,0,016, 
	01,0376,0,016, 0,0374,0,016, 0,0,0,017, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
};
static UCHAR1 us_icon129israel[] =
{
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0360,0,03,0200, 0160,0,07,0300, 0160,0,017,0340, 
	0160,0,017,0340, 0170,0,017,0340, 070,0,017,0340, 070,0,017,0340, 
	074,0,017,0340, 034,0,07,0300, 034,0,03,0200, 036,0,0,0, 
	016,0,0,0, 017,0,0,0, 07,0200,0,0, 07,0300,0,0, 
	03,0340,0,0, 01,0360,0,0, 0,0370,0,0, 0,0174,0,0, 
	0,076,0,0, 0,037,0200,0, 0,017,0360,0, 0,03,0376,0, 
	0,0,0377,0340, 0,0,037,0377, 0,0,03,0377, 0,0,0,077, 
};
static UCHAR1 us_icon132israel[] =
{
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,017, 0,0,0,016, 0,0,0,016, 
	0,0,0,016, 0,0,0,036, 0,0,0,034, 0,0,0,034, 
	0,0,0,074, 0,0,0,070, 0,0,0,070, 0,0,0,0170, 
	0,0,0,0160, 0,0,0,0360, 0,0,01,0340, 0,0,03,0340, 
	0,0,07,0300, 0,0,017,0200, 0,0,037,0, 0,0,076,0, 
	0,0,0174,0, 0,01,0370,0, 0,017,0360,0, 0,0177,0300,0, 
	07,0377,0,0, 0377,0370,0,0, 0377,0300,0,0, 0374,0,0,0, 
};

/* Danish version */
static UCHAR1 us_icon130denmark[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0177,0377,0377,0377, 
	0377,0377,0377,0377, 0377,0377,0377,0377, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,017,0200,0, 
	0340,037,0300,0, 0340,077,0340,0, 0340,0177,0360,0, 0340,0177,0360,0, 
	0340,0177,0360,0, 0340,0177,0360,0, 0340,0177,0360,0, 0340,077,0340,0, 
	0340,037,0300,0, 0340,017,0200,0, 0340,0,0,0, 0340,0,0,0, 
};
static UCHAR1 us_icon131denmark[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0377,0377,0377,0376, 
	0377,0377,0377,0377, 0377,0377,0377,0377, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,01,0360,07, 
	0,03,0370,07, 0,07,0374,07, 0,017,0376,07, 0,017,0376,07, 
	0,017,0376,07, 0,017,0376,07, 0,017,0376,07, 0,07,0374,07, 
	0,03,0370,07, 0,01,0360,07, 0,0,0,07, 0,0,0,07, 
};
static UCHAR1 us_icon129denmark[] =
{
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,037, 0340,0,0,077, 0340,0,0,077, 
	0340,0,0,077, 0340,0,0,077, 0340,0,0,077, 0340,0,0,077, 
	0340,0,0,037, 0340,0,0,037, 0340,0,0,017, 0340,0,0,07, 
	0340,0,0,0, 0340,0,0,0, 0377,0377,0377,0377, 0377,0377,0377,0377, 
	0177,0377,0377,0377, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};
static UCHAR1 us_icon132denmark[] =
{
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0370,0,0,07, 0374,0,0,07, 0374,0,0,07, 
	0374,0,0,07, 0374,0,0,07, 0374,0,0,07, 0374,0,0,07, 
	0370,0,0,07, 0370,0,0,07, 0360,0,0,07, 0340,0,0,07, 
	0,0,0,07, 0,0,0,07, 0377,0377,0377,0377, 0377,0377,0377,0377, 
	0377,0377,0377,0376, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};

/* Japanese version */
static UCHAR1 us_icon130japan[] =
{
	0377,0377,0377,0377, 0377,0377,0377,0377, 0377,0377,0377,0377, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
	0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 0340,03,0374,0, 
};
static UCHAR1 us_icon131japan[] =
{
	0377,0377,0377,0377, 0377,0377,0377,0377, 0377,0377,0377,0377, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
	0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 0,077,0300,07, 
};
static UCHAR1 us_icon129japan[] =
{
	0340,03,0374,0, 0340,03,0374,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,03, 0340,0,0,07, 0340,0,0,017, 0340,0,0,037, 
	0340,0,0,077, 0340,0,0,077, 0340,0,0,077, 0340,0,0,077, 
	0340,0,0,077, 0340,0,0,077, 0340,0,0,077, 0340,0,0,077, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0377,0377,0377,0377, 0377,0377,0377,0377, 0377,0377,0377,0377, 
};
static UCHAR1 us_icon132japan[] =
{
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0300,0,0,07, 0340,0,0,07, 0360,0,0,07, 0370,0,0,07, 
	0374,0,0,07, 0374,0,0,07, 0374,0,0,07, 0374,0,0,07, 
	0374,0,0,07, 0374,0,0,07, 0374,0,0,07, 0374,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0377,0377,0377,0377, 0377,0377,0377,0377, 0377,0377,0377,0377, 
};

/* Swiss version */
static UCHAR1 us_icon130swiss[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0177,0377, 0,0,0377,0377, 
	0,01,0377,0377, 0,03,0300,0, 0,07,0200,0, 0,017,0,0, 
	0,036,0,0, 0,074,0,0, 0,0170,0,0, 0,0360,0,0, 
	01,0340,0,0, 03,0300,0,0, 07,0200,0,0, 017,0,0,0, 
	036,01,0340,0, 074,03,0360,0, 0170,07,0370,0, 0370,07,0370,0, 
};
static UCHAR1 us_icon131swiss[] =
{
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0377,0376,0,0, 0377,0377,0,0, 
	0377,0377,0200,0, 0,03,0300,0, 0,01,0340,0, 0,0,0360,0, 
	0,0,0170,0, 0,0,074,0, 0,0,036,0, 0,0,017,0, 
	0,0,07,0200, 0,0,03,0300, 0,0,01,0340, 0,0,0,0360, 
	0,07,0200,0170, 0,017,0300,074, 0,037,0340,036, 0,037,0340,037, 
};
static UCHAR1 us_icon129swiss[] =
{
	0370,07,0370,0, 0170,07,0370,0, 074,03,0360,0, 036,01,0340,0, 
	017,0,0,03, 07,0200,0,07, 03,0300,0,017, 01,0340,0,017, 
	0,0360,0,017, 0,0170,0,017, 0,074,0,07, 0,036,0,03, 
	0,017,0,0, 0,07,0200,0, 0,03,0300,0, 0,01,0377,0377, 
	0,0,0377,0377, 0,0,0177,0377, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};
static UCHAR1 us_icon132swiss[] =
{
	0,037,0340,037, 0,037,0340,036, 0,017,0300,074, 0,07,0200,0170, 
	0300,0,0,0360, 0340,0,01,0340, 0360,0,03,0300, 0360,0,07,0200, 
	0360,0,017,0, 0360,0,036,0, 0340,0,074,0, 0300,0,0170,0, 
	0,0,0360,0, 0,01,0340,0, 0,03,0300,0, 0377,0377,0200,0, 
	0377,0377,0,0, 0377,0376,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 
};

/* UK/Ireland version */
static UCHAR1 us_icon130uk[] =
{
	0,0,0,077, 0,0,03,0377, 0,0,037,0377, 0,0,0377,0340, 
	0,03,0376,0, 0,017,0360,0, 0,037,0200,0, 0,076,0,0, 
	0,0174,0,0, 0,0370,0,0, 01,0360,0,0, 03,0340,0,0, 
	07,0300,0,0, 07,0200,0,0, 017,0,0,0, 016,0,0,0, 
	036,0,0,0, 034,0,0,0, 034,0,0,0, 074,03,0377,0, 
	070,03,0377,0, 070,03,0377,0, 0170,03,0377,0, 0160,03,0377,0, 
	0160,03,0377,0, 0160,0,0,0, 0360,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
};
static UCHAR1 us_icon131uk[] =
{
	0374,0,0,0, 0377,0300,0,0, 0377,0370,0,0, 07,0377,0,0, 
	0,0177,0300,0, 0,017,0360,0, 0,01,0370,0, 0,0,0174,0, 
	0,0,076,0, 0,0,037,0, 0,0,017,0200, 0,0,07,0300, 
	0,0,03,0340, 0,0,01,0340, 0,0,0,0360, 0,0,0,0160, 
	0,0,0,0170, 0,0,0,070, 0,0,0,070, 0,0377,0300,074, 
	0,0377,0300,034, 0,0377,0300,034, 0,0377,0300,036, 0,0377,0300,016, 
	0,0377,0300,016, 0,0,0,016, 0,0,0,017, 0,0,0,07, 
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
};
static UCHAR1 us_icon129uk[] =
{
	0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 0340,0,0,0, 
	0340,0,0,0, 0360,0,0,0, 0160,0,0,0, 0160,0,0,0, 
	0160,0,0,0, 0170,0,0,017, 070,0,0,017, 070,0,0,017, 
	074,0,0,017, 034,0,0,017, 034,0,0,017, 036,0,0,017, 
	016,0,0,017, 017,0,0,017, 07,0200,0,017, 07,0300,0,017, 
	03,0340,0,0, 01,0360,0,0, 0,0370,0,0, 0,0174,0,0, 
	0,076,0,0, 0,037,0200,0, 0,017,0360,0, 0,03,0376,0, 
	0,0,0377,0340, 0,0,037,0377, 0,0,03,0377, 0,0,0,077, 
};
static UCHAR1 us_icon132uk[] =
{
	0,0,0,07, 0,0,0,07, 0,0,0,07, 0,0,0,07, 
	0,0,0,07, 0,0,0,017, 0,0,0,016, 0,0,0,016, 
	0,0,0,016, 0360,0,0,036, 0360,0,0,034, 0360,0,0,034, 
	0360,0,0,074, 0360,0,0,070, 0360,0,0,070, 0360,0,0,0170, 
	0360,0,0,0160, 0360,0,0,0360, 0360,0,01,0340, 0360,0,03,0340, 
	0,0,07,0300, 0,0,017,0200, 0,0,037,0, 0,0,076,0, 
	0,0,0174,0, 0,01,0370,0, 0,017,0360,0, 0,0177,0300,0, 
	07,0377,0,0, 0377,0370,0,0, 0377,0300,0,0, 0374,0,0,0, 
};

/* Russian version */
static UCHAR1 us_icon130russia[] =
{
	0,0,0,077, 0,0,03,0377, 0,0,037,0377, 0,0,0377,0340, 
	0,03,0376,0, 0,017,0360,0, 0,037,0200,0, 0,076,0,0, 
	0,0174,0,0, 0,0370,0,0, 01,0360,0,0, 03,0340,0,0, 
	07,0300,0,0, 07,0200,0,0, 017,0,0,0, 016,0,0,0, 
	036,0,0,0, 034,0,0,0, 034,0,0,0, 074,0,0,0, 
	070,0,0,0, 070,0,0,0, 0170,0,0,0, 0160,0,0,0, 
	0160,0,0,0, 0160,0,0,0, 0360,0,0,0, 0340,0,0,0, 
	0340,0,0170,0, 0340,0,0374,0, 0340,01,0376,0, 0340,01,0376,0, 
};
static UCHAR1 us_icon131russia[] =
{
	0374,0,0,0, 0377,0300,0,0, 0377,0370,0,0, 07,0377,0,0, 
	0,0177,0300,0, 0,017,0360,0, 0,01,0370,0, 0,0,0174,0, 
	0,0,076,0, 0,0,037,0, 0,0,017,0200, 0,0,07,0300, 
	0,0,03,0340, 0,0,01,0340, 0,0,0,0360, 0,0,0,0160, 
	0,0,0,0170, 0,0,0,070, 0,0,0,070, 0,0,0,074, 
	0,0,0,034, 0,0,0,034, 0,0,0,036, 0,0,0,016, 
	0,0,0,016, 0,0,0,016, 0,0,0,017, 0,0,0,07, 
	0,036,0,07, 0,077,0,07, 0,0177,0200,07, 0,0177,0200,07, 
};
static UCHAR1 us_icon129russia[] =
{
	0340,01,0376,0, 0340,01,0376,0, 0340,0,0374,0, 0340,0,0170,0, 
	0340,0,0,0, 0360,0,0,0, 0160,0,0,0, 0160,0,0,0, 
	0160,0,0,0, 0170,0,0,0, 070,0,0,0, 070,0,0,0, 
	074,0,0,0, 034,0,0,0, 034,0,0,0, 036,0,0,0, 
	016,0,0,0, 017,0,0,0, 07,0200,0,0, 07,0300,0,0, 
	03,0340,0,0, 01,0360,0,0, 0,0370,0,0, 0,0174,0,0, 
	0,076,0,0, 0,037,0200,0, 0,017,0360,0, 0,03,0376,0, 
	0,0,0377,0340, 0,0,037,0377, 0,0,03,0377, 0,0,0,077, 
};
static UCHAR1 us_icon132russia[] =
{
	0,0177,0200,07, 0,0177,0200,07, 0,077,0,07, 0,036,0,07, 
	0,0,0,07, 0,0,0,017, 0,0,0,016, 0,0,0,016, 
	0,0,0,016, 0,0,0,036, 0,0,0,034, 0,0,0,034, 
	0,0,0,074, 0,0,0,070, 0,0,0,070, 0,0,0,0170, 
	0,0,0,0160, 0,0,0,0360, 0,0,01,0340, 0,0,03,0340, 
	0,0,07,0300, 0,0,017,0200, 0,0,037,0, 0,0,076,0, 
	0,0,0174,0, 0,01,0370,0, 0,017,0360,0, 0,0177,0300,0, 
	07,0377,0,0, 0377,0370,0,0, 0377,0300,0,0, 0374,0,0,0, 
};

struct
{
	CHAR *country;
	UCHAR1 *iconul, *iconur, *iconll, *iconlr;
} us_iconlists[] =
{
	{N_("N.America"),   us_icon130namerica,  us_icon129namerica,  us_icon131namerica,  us_icon132namerica},
	{N_("Australia,NZ"),us_icon130ausnz,     us_icon129ausnz,     us_icon131ausnz,     us_icon132ausnz},
	{N_("Denmark"),     us_icon130denmark,   us_icon129denmark,   us_icon131denmark,   us_icon132denmark},
	{N_("Europe"),      us_icon130europe,    us_icon129europe,    us_icon131europe,    us_icon132europe},
	{N_("India"),       us_icon130india,     us_icon129india,     us_icon131india,     us_icon132india},
	{N_("Italy"),       us_icon130italy,     us_icon129italy,     us_icon131italy,     us_icon132italy},
	{N_("Israel"),      us_icon130israel,    us_icon129israel,    us_icon131israel,    us_icon132israel},
	{N_("Japan"),       us_icon130japan,     us_icon129japan,     us_icon131japan,     us_icon132japan},
	{N_("Russia"),      us_icon130russia,    us_icon129russia,    us_icon131russia,    us_icon132russia},
	{N_("Switzerland"), us_icon130swiss,     us_icon129swiss,     us_icon131swiss,     us_icon132swiss},
	{N_("UK,Ireland"),  us_icon130uk,        us_icon129uk,        us_icon131uk,        us_icon132uk},
	{0,0,0,0,0}
};

/* About Electric */
static DIALOGITEM us_aboutgnudialogitems[] =
{
 /*  1 */ {0, {24,320,48,400}, BUTTON, N_("OK")},
 /*  2 */ {0, {308,12,324,356}, MESSAGE, N_("Electric comes with ABSOLUTELY NO WARRANTY")},
 /*  3 */ {0, {284,12,300,489}, MESSAGE, N_("Copyright (c) 2002 Static Free Software (www.staticfreesoft.com)")},
 /*  4 */ {0, {56,8,72,221}, MESSAGE, N_("Written by Steven M. Rubin")},
 /*  5 */ {0, {8,8,24,295}, MESSAGE, N_("The Electric(tm) Design System")},
 /*  6 */ {0, {32,8,48,246}, MESSAGE, N_("Version XXXX")},
 /*  7 */ {0, {4,420,36,452}, ICON|INACTIVE, (CHAR *)us_icon130namerica},
 /*  8 */ {0, {36,420,68,452}, ICON|INACTIVE, (CHAR *)us_icon129namerica},
 /*  9 */ {0, {4,452,36,484}, ICON|INACTIVE, (CHAR *)us_icon131namerica},
 /* 10 */ {0, {36,452,68,484}, ICON|INACTIVE, (CHAR *)us_icon132namerica},
 /* 11 */ {0, {100,8,273,487}, SCROLL, x_("")},
 /* 12 */ {0, {76,160,94,348}, BUTTON, N_("And a Cast of Thousands")},
 /* 13 */ {0, {332,12,348,330}, MESSAGE, N_("This is free software, and you are welcome to")},
 /* 14 */ {0, {308,358,326,487}, BUTTON, N_("Warranty details")},
 /* 15 */ {0, {344,358,362,487}, BUTTON, N_("Copying details")},
 /* 16 */ {0, {352,12,368,309}, MESSAGE, N_("redistribute it under certain conditions")},
 /* 17 */ {0, {76,388,92,484}, POPUP, x_("")}
};
static DIALOG us_aboutgnudialog = {{50,75,427,573}, 0, 0, 17, us_aboutgnudialogitems, 0, 0};

/* special items for the "About Electric" dialog: */
#define DABO_COPYRIGHT    3			/* copyright information (message) */
#define DABO_VERSION      6			/* version number (message) */
#define DABO_ICONUL       7			/* upper-left part of icon (icon) */
#define DABO_ICONUR       8			/* upper-right part of icon (icon) */
#define DABO_ICONLL       9			/* lower-left part of icon (icon) */
#define DABO_ICONLR       10		/* lower-right part of icon (icon) */
#define DABO_INFORMATION  11		/* information area (scroll) */
#define DABO_AUTHORS      12		/* authors (button) */
#define DABO_WARRANTY     14		/* warranty (button) */
#define DABO_COPYING      15		/* copying (button) */
#define DABO_PLUGTYPE     17		/* type of plug (popup) */

INTBIG us_aboutdlog(void)
{
	CHAR line[256], date[30], *language, *truename, *newlang[50];
	INTBIG itemHit, i, pluglocale;
	BOOLEAN castlisted;
	FILE *io;
	REGISTER void *infstr, *dia;

	/* show the "about" dialog */
#ifdef EPROGRAMNAME
	esnprintf(line, 256, _("About %s"), EPROGRAMNAME);
	us_aboutgnudialog.movable = line;
#else
	us_aboutgnudialog.movable = _("About Electric");
#endif
	dia = DiaInitDialog(&us_aboutgnudialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DABO_INFORMATION, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT|SCSMALLFONT|SCHORIZBAR);
	for(i=0; us_iconlists[i].country != 0; i++) newlang[i] = TRANSLATE(us_iconlists[i].country);
	DiaSetPopup(dia, DABO_PLUGTYPE, i, newlang);
	pluglocale = 0;

	/* show the version and copyright information */
	(void)estrcpy(line, _("Version "));
	(void)estrcat(line, el_version);
	(void)estrcat(line, x_(", "));
	(void)estrcat(line, WIDENSTRINGDEFINE(__DATE__));
	DiaSetText(dia, DABO_VERSION, line);
	estrcpy(date, timetostring(getcurrenttime()));
	date[24] = 0;
	(void)esnprintf(line, 256,
		_("Copyright (c) %s Static Free Software (www.staticfreesoft.com)"), &date[20]);
	DiaSetText(dia, DABO_COPYRIGHT, line);

	castlisted = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DABO_INFORMATION)
		{
			if (!castlisted) continue;
			i = DiaGetCurLine(dia, DABO_INFORMATION);
			if (i < 0) continue;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s: %s"), us_castofthousands[i].name,
				TRANSLATE(us_castofthousands[i].help));
			DiaSetScrollLine(dia, DABO_INFORMATION, i, returninfstr(infstr));
			continue;
		}
		if (itemHit == DABO_AUTHORS)
		{
			DiaLoadTextDialog(dia, DABO_INFORMATION, DiaNullDlogList,
				DiaNullDlogItem, DiaNullDlogDone, -1);
			for(i=0; us_castofthousands[i].name != 0; i++)
				DiaStuffLine(dia, DABO_INFORMATION, us_castofthousands[i].name);
			DiaSelectLine(dia, DABO_INFORMATION, -1);
			castlisted = TRUE;
			continue;
		}
		if (itemHit == DABO_WARRANTY)
		{
			DiaLoadTextDialog(dia, DABO_INFORMATION, DiaNullDlogList,
				DiaNullDlogItem, DiaNullDlogDone, -1);
			language = elanguage();
			if (namesame(language, x_("en")) != 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%sinternational%s%s%sLC_MESSAGES%slicense.txt"),
					el_libdir, DIRSEPSTR, language, DIRSEPSTR, DIRSEPSTR);
				io = xopen(returninfstr(infstr), el_filetypetext, 0, &truename);
				if (io == 0) language = x_("en"); else
				{
					if (us_showforeignlicense(io, 4, DABO_INFORMATION, dia))
						language = x_("en");
					xclose(io);
				}
			}
			if (namesame(language, x_("en")) == 0)
			{
				for(i=0; us_gnuwarranty[i] != 0; i++)
					DiaStuffLine(dia, DABO_INFORMATION, us_gnuwarranty[i]);
			}
			DiaSelectLine(dia, DABO_INFORMATION, -1);
			castlisted = FALSE;
			continue;
		}
		if (itemHit == DABO_COPYING)
		{
			DiaLoadTextDialog(dia, DABO_INFORMATION, DiaNullDlogList,
				DiaNullDlogItem, DiaNullDlogDone, -1);
			language = elanguage();
			if (namesame(language, x_("en")) != 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%sinternational%s%s%sLC_MESSAGES%slicense.txt"),
					el_libdir, DIRSEPSTR, language, DIRSEPSTR, DIRSEPSTR);
				io = xopen(returninfstr(infstr), el_filetypetext, 0, &truename);
				if (io == 0) language = x_("en"); else
				{
					if (us_showforeignlicense(io, 3, DABO_INFORMATION, dia))
						language = x_("en");
					xclose(io);
				}
			}
			if (namesame(language, x_("en")) == 0)
			{
				for(i=0; us_gnucopying[i] != 0; i++)
					DiaStuffLine(dia, DABO_INFORMATION, us_gnucopying[i]);
			}
			DiaSelectLine(dia, DABO_INFORMATION, -1);
			castlisted = FALSE;
			continue;
		}
		if (itemHit == DABO_PLUGTYPE)
		{
			i = DiaGetPopupEntry(dia, DABO_PLUGTYPE);
			if (i == pluglocale) continue;
			pluglocale =  i;
			DiaChangeIcon(dia, DABO_ICONUL, us_iconlists[pluglocale].iconul);
			DiaChangeIcon(dia, DABO_ICONUR, us_iconlists[pluglocale].iconur);
			DiaChangeIcon(dia, DABO_ICONLL, us_iconlists[pluglocale].iconll);
			DiaChangeIcon(dia, DABO_ICONLR, us_iconlists[pluglocale].iconlr);
			continue;
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

BOOLEAN us_showforeignlicense(FILE *io, INTBIG section, INTBIG item, void *dia)
{
	REGISTER INTBIG foundsection;
	REGISTER BOOLEAN foundtext;
	CHAR line[200];

	foundsection = 0;
	foundtext = FALSE;
	for(;;)
	{
		if (xfgets(line, 200, io)) break;
		if (line[0] == '*' && line[1] == '*')
		{
			foundsection++;
			if (foundsection > section) break;
			continue;
		}
		if (foundsection == section)
		{
			DiaStuffLine(dia, item, line);
			foundtext = TRUE;
		}
	}
	if (foundtext) return(FALSE);
	return(TRUE);
}

/****************************** ALIGNMENT OPTIONS DIALOG ******************************/

/* Alignment Options */
static DIALOGITEM us_alignmentdialogitems[] =
{
/*  1 */ {0, {68,340,92,404}, BUTTON, N_("OK")},
/*  2 */ {0, {68,32,92,96}, BUTTON, N_("Cancel")},
/*  3 */ {0, {8,8,24,205}, MESSAGE, N_("Alignment of cursor to grid:")},
/*  4 */ {0, {40,8,56,205}, MESSAGE, N_("Alignment of edges to grid:")},
/*  5 */ {0, {8,208,24,280}, EDITTEXT, x_("")},
/*  6 */ {0, {40,208,56,280}, EDITTEXT, x_("")},
/*  7 */ {0, {16,284,32,426}, MESSAGE, N_("Values of zero will")},
/*  8 */ {0, {32,284,48,428}, MESSAGE, N_("cause no alignment.")}
};
static DIALOG us_alignmentdialog = {{50,75,154,512}, N_("Alignment Options"), 0, 8, us_alignmentdialogitems, 0, 0};

/* special items for the "alignment options" dialog: */
#define DALI_GRID   5		/* grid alignment (edit text) */
#define DALI_EDGE   6		/* edge alignment (edit text) */

INTBIG us_alignmentdlog(void)
{
	INTBIG itemHit, retval;
	REGISTER void *dia;

	if (us_needwindow()) return(0);

	/* display the alignment settings dialog box */
	dia = DiaInitDialog(&us_alignmentdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DALI_GRID, frtoa(us_alignment_ratio));
	DiaSetText(dia, DALI_EDGE, frtoa(us_edgealignment_ratio));

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	if (itemHit != CANCEL)
	{
		/* see if alignment changed */
		retval = atofr(DiaGetText(dia, DALI_GRID));
		if (retval != us_alignment_ratio)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_alignment_ratio_key, retval, VINTEGER);
		retval = atofr(DiaGetText(dia, DALI_EDGE));
		if (retval != us_edgealignment_ratio)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_alignment_edge_ratio_key, retval, VINTEGER);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** ARC CREATION OPTIONS DIALOG ******************************/

/* New arc options */
static DIALOGITEM us_defarcdialogitems[] =
{
 /*  1 */ {0, {184,304,208,376}, BUTTON, N_("OK")},
 /*  2 */ {0, {184,64,208,136}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,140,20,420}, POPUP, x_("")},
 /*  4 */ {0, {4,4,20,136}, RADIO, N_("Defaults for arc:")},
 /*  5 */ {0, {32,4,48,164}, RADIO, N_("Defaults for all arcs")},
 /*  6 */ {0, {152,360,168,412}, BUTTON, N_("Set pin")},
 /*  7 */ {0, {72,8,88,64}, CHECK, N_("Rigid")},
 /*  8 */ {0, {72,104,88,204}, CHECK, N_("Fixed-angle")},
 /*  9 */ {0, {72,216,88,288}, CHECK, N_("Slidable")},
 /* 10 */ {0, {96,8,112,84}, CHECK, N_("Negated")},
 /* 11 */ {0, {96,104,112,196}, CHECK, N_("Directional")},
 /* 12 */ {0, {96,216,112,336}, CHECK, N_("Ends extended")},
 /* 13 */ {0, {32,228,52,420}, BUTTON, N_("Reset to initial defaults")},
 /* 14 */ {0, {128,64,144,152}, EDITTEXT, x_("")},
 /* 15 */ {0, {128,8,144,56}, MESSAGE, N_("Width:")},
 /* 16 */ {0, {128,176,144,224}, MESSAGE, N_("Angle:")},
 /* 17 */ {0, {128,232,144,296}, EDITTEXT, x_("")},
 /* 18 */ {0, {152,8,168,39}, MESSAGE, N_("Pin:")},
 /* 19 */ {0, {152,40,168,348}, MESSAGE, x_("")}
};
static DIALOG us_defarcdialog = {{50,75,267,505}, N_("New Arc Options"), 0, 19, us_defarcdialogitems, 0, 0};

static ARCPROTO *us_thisap;
static NODEPROTO *us_posprims;

static void us_defarcload(ARCPROTO*, NODEPROTO**, INTBIG, void*);
static BOOLEAN us_topofpins(CHAR**);
static CHAR *us_nextpins(void);

/* special items for the "defarc" dialog: */
#define DNAO_SPECARC     3		/* specific arc (popup) */
#define DNAO_SPECDEF     4		/* defaults for specific type (radio) */
#define DNAO_ALLDEF      5		/* defaults for all types (radio) */
#define DNAO_SETPIN      6		/* set pin (button) */
#define DNAO_RIGID       7		/* rigid (check) */
#define DNAO_FIXANGLE    8		/* fixed-angle (check) */
#define DNAO_SLIDABLE    9		/* slidable (check) */
#define DNAO_NEGATED     10		/* negated (check) */
#define DNAO_DIRECTIONAL 11		/* directional (check) */
#define DNAO_ENDSEXTEND  12		/* ends extended (check) */
#define DNAO_RESETSTATE  13		/* reset to initial state (button) */
#define DNAO_WIDTH       14		/* width (edit text) */
#define DNAO_WIDTH_T     15		/* width title (message) */
#define DNAO_ANGLE_T     16		/* angle title (message) */
#define DNAO_ANGLE       17		/* angle (edit text) */
#define DNAO_DEFPIN_T    18		/* default pin title (message) */
#define DNAO_DEFPIN      19		/* default pin (message) */

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C" {
#endif
	extern DIALOG us_listdialog;
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

INTBIG us_defarcdlog(void)
{
	INTBIG itemHit;
	REGISTER INTBIG allstyle, i, j, origallstyle, bits, wid;
	REGISTER VARIABLE *var;
	CHAR **arcnames;
	REGISTER ARCPROTO *ap, **arcs;
	REGISTER NODEPROTO **pins, *np;
	REGISTER void *dia, *subdia;

	/* display the default arc dialog box */
	dia = DiaInitDialog(&us_defarcdialog);
	if (dia == 0) return(0);

	/* remember all state */
	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_arcstylekey);
	if (var != NOVARIABLE) allstyle = var->addr; else
		allstyle = WANTFIXANG;
	origallstyle = allstyle;
	for(i=0, ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto) i++;
	pins = (NODEPROTO **)emalloc(i * (sizeof (NODEPROTO *)), el_tempcluster);
	arcs = (ARCPROTO **)emalloc(i * (sizeof (ARCPROTO *)), el_tempcluster);
	arcnames = (CHAR **)emalloc(i * (sizeof (CHAR *)), el_tempcluster);
	if (pins == 0 || arcs == 0 || arcnames == 0) return(0);
	j = 0;
	for(i=0, ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto, i++)
	{
		var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
		if (var != NOVARIABLE) ap->temp1 = var->addr; else
			ap->temp1 = ap->userbits;
		ap->temp2 = defaultarcwidth(ap);
		pins[i] = getpinproto(ap);
		arcs[i] = ap;
		arcnames[i] = ap->protoname;
		if (ap == us_curarcproto) j = i;
	}

	/* initially load for first arc in technology */
	us_thisap = us_curarcproto;
	us_defarcload(us_thisap, pins, allstyle, dia);
	DiaSetControl(dia, DNAO_SPECDEF, 1);
	DiaSetPopup(dia, DNAO_SPECARC, i, arcnames);
	DiaSetPopupEntry(dia, DNAO_SPECARC, j);
	efree((CHAR *)arcnames);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DNAO_RIGID || itemHit == DNAO_FIXANGLE ||
			itemHit == DNAO_SLIDABLE || itemHit == DNAO_NEGATED ||
			itemHit == DNAO_DIRECTIONAL || itemHit == DNAO_ENDSEXTEND)
		{
			j = DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, 1-j);
			if (DiaGetControl(dia, DNAO_SPECDEF) == 0) i = allstyle; else
				i = us_thisap->temp1;
			switch (itemHit)
			{
				case DNAO_RIGID:
					if (j == 0) i |= WANTFIX; else
						i &= ~WANTFIX;
					break;
				case DNAO_FIXANGLE:
					if (j == 0) i |= WANTFIXANG; else
						i &= ~WANTFIXANG;
					break;
				case DNAO_SLIDABLE:
					if (j != 0) i |= WANTCANTSLIDE; else
						i &= ~WANTCANTSLIDE;
					break;
				case DNAO_NEGATED:
					if (j == 0) i |= WANTNEGATED; else
						i &= ~WANTNEGATED;
					break;
				case DNAO_DIRECTIONAL:
					if (j == 0) i |= WANTDIRECTIONAL; else
						i &= ~WANTDIRECTIONAL;
					break;
				case DNAO_ENDSEXTEND:
					if (j != 0) i |= WANTNOEXTEND; else
						i &= ~WANTNOEXTEND;
					break;
			}
			if (DiaGetControl(dia, DNAO_SPECDEF) == 0) allstyle = i; else
				us_thisap->temp1 = i;
			continue;
		}
		if (itemHit == DNAO_SPECDEF)
		{
			us_defarcload(us_thisap, pins, allstyle, dia);
			DiaSetControl(dia, DNAO_SPECDEF, 1);
			DiaSetControl(dia, DNAO_ALLDEF, 0);
			continue;
		}
		if (itemHit == DNAO_ALLDEF)
		{
			us_defarcload(NOARCPROTO, pins, allstyle, dia);
			DiaSetControl(dia, DNAO_SPECDEF, 0);
			DiaSetControl(dia, DNAO_ALLDEF, 1);
			continue;
		}
		if (itemHit == DNAO_RESETSTATE)
		{
			allstyle = WANTFIXANG;
			DiaSetControl(dia, DNAO_RIGID, 0);
			DiaSetControl(dia, DNAO_FIXANGLE, 1);
			DiaSetControl(dia, DNAO_SLIDABLE, 1);
			DiaSetControl(dia, DNAO_NEGATED, 0);
			DiaSetControl(dia, DNAO_DIRECTIONAL, 0);
			DiaSetControl(dia, DNAO_ENDSEXTEND, 1);
			continue;
		}
		if (itemHit == DNAO_SPECARC)
		{
			i = DiaGetPopupEntry(dia, DNAO_SPECARC);
			us_thisap = arcs[i];
			us_defarcload(us_thisap, pins, allstyle, dia);
			continue;
		}
		if (itemHit == DNAO_WIDTH)
		{
			if (DiaGetControl(dia, DNAO_SPECDEF) == 0) continue;
			us_thisap->temp2 = atola(DiaGetText(dia, DNAO_WIDTH), 0);
			continue;
		}
		if (itemHit == DNAO_ANGLE)
		{
			if (DiaGetControl(dia, DNAO_SPECDEF) == 0) continue;
			us_thisap->temp1 = (us_thisap->temp1 & ~AANGLEINC) |
				((eatoi(DiaGetText(dia, DNAO_ANGLE))%360) << AANGLEINCSH);
			continue;
		}
		if (itemHit == DNAO_SETPIN)
		{
			if (DiaGetControl(dia, DNAO_SPECDEF) == 0) continue;
			subdia = DiaInitDialog(&us_listdialog);
			if (subdia == 0) return(0);
			DiaInitTextDialog(subdia, 3, us_topofpins, us_nextpins, DiaNullDlogDone,
				0, SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT);
			DiaSetText(subdia, 4, _("Select a node to use as a pin"));

			for(;;)
			{
				itemHit = DiaNextHit(subdia);
				if (itemHit == OK || itemHit == CANCEL) break;
			}
			np = getnodeproto(DiaGetScrollLine(subdia, 3, DiaGetCurLine(subdia, DNAO_SPECARC)));
			DiaDoneDialog(subdia);
			if (itemHit == CANCEL) continue;

			for(i=0, ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto, i++)
				if (us_thisap == ap) break;
			if (ap != NOARCPROTO) pins[i] = np;
			DiaSetText(dia, DNAO_DEFPIN, describenodeproto(np));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		for(i=0, ap = el_curtech->firstarcproto; ap != NOARCPROTO;
			ap = ap->nextarcproto, i++)
		{
			var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
			if (var != NOVARIABLE) bits = var->addr; else
				bits = ap->userbits;
			if (ap->temp1 != bits)
			{
				if ((ap->temp1 & AANGLEINC) != (bits & AANGLEINC))
					(void)setval((INTBIG)ap, VARCPROTO, x_("userbits"), ap->temp1, VINTEGER);
				if ((ap->temp1 & ~AANGLEINC) != (bits & ~AANGLEINC))
					(void)setvalkey((INTBIG)ap, VARCPROTO, us_arcstylekey, ap->temp1, VINTEGER);
			}
			if (ap->temp2 != defaultarcwidth(ap))
			{
				wid = (arcprotowidthoffset(ap) + ap->temp2) * WHOLE /
					el_curlib->lambda[ap->tech->techindex];
				(void)setvalkey((INTBIG)ap, VARCPROTO, el_arc_width_default_key,
					wid, VINTEGER);
			}
			np = getpinproto(ap);
			if (np != pins[i])
				(void)setval((INTBIG)ap, VARCPROTO, x_("ARC_Default_Pin"), (INTBIG)pins[i], VNODEPROTO);
		}
		if (allstyle != origallstyle)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_arcstylekey, allstyle, VINTEGER);
	}
	DiaDoneDialog(dia);
	efree((CHAR *)pins);
	efree((CHAR *)arcs);
	return(0);
}

BOOLEAN us_topofpins(CHAR **c)
{
	Q_UNUSED( c );
	us_posprims = el_curtech->firstnodeproto;
	return(TRUE);
}

CHAR *us_nextpins(void)
{
	REGISTER CHAR *nextname;
	REGISTER INTBIG i;
	REGISTER PORTPROTO *pp;

	for( ; us_posprims != NONODEPROTO; us_posprims = us_posprims->nextnodeproto)
	{
		/* test this pin for validity */
		pp = us_posprims->firstportproto;
		for(i=0; pp->connects[i] != NOARCPROTO; i++)
			if (pp->connects[i] == us_thisap) break;
		if (pp->connects[i] == NOARCPROTO) continue;
		nextname = us_posprims->protoname;
		us_posprims = us_posprims->nextnodeproto;
		return(nextname);
	}
	return(0);
}

/*
 * Helper routine for arc options
 */
void us_defarcload(ARCPROTO *ap, NODEPROTO **pins, INTBIG allstyle, void *dia)
{
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *oap;
	REGISTER INTBIG i;
	REGISTER INTBIG style;
	CHAR line[20];

	if (ap == NOARCPROTO)
	{
		style = allstyle;
		DiaDimItem(dia, DNAO_SPECARC);
		DiaUnDimItem(dia, DNAO_RESETSTATE);
		DiaDimItem(dia, DNAO_WIDTH_T);
		DiaSetText(dia, DNAO_WIDTH, x_(""));
		DiaNoEditControl(dia, DNAO_WIDTH);
		DiaDimItem(dia, DNAO_ANGLE_T);
		DiaSetText(dia, DNAO_ANGLE, x_(""));
		DiaNoEditControl(dia, DNAO_ANGLE);
		DiaDimItem(dia, DNAO_DEFPIN_T);
		DiaSetText(dia, DNAO_DEFPIN, x_(""));
		DiaDimItem(dia, DNAO_SETPIN);
	} else
	{
		style = ap->temp1;
		DiaUnDimItem(dia, DNAO_SPECARC);
		DiaDimItem(dia, DNAO_RESETSTATE);
		DiaUnDimItem(dia, DNAO_WIDTH_T);
		DiaSetText(dia, -DNAO_WIDTH, latoa(ap->temp2 - arcprotowidthoffset(ap), 0));
		DiaEditControl(dia, DNAO_WIDTH);
		DiaUnDimItem(dia, DNAO_ANGLE_T);
		(void)esnprintf(line, 20, x_("%ld"), (ap->userbits&AANGLEINC) >> AANGLEINCSH);
		DiaSetText(dia, DNAO_ANGLE, line);
		DiaEditControl(dia, DNAO_ANGLE);
		DiaUnDimItem(dia, DNAO_DEFPIN_T);
		np = NONODEPROTO;
		for(i=0, oap = el_curtech->firstarcproto; oap != NOARCPROTO; oap = oap->nextarcproto, i++)
			if (ap == oap) np = pins[i];
		DiaSetText(dia, DNAO_DEFPIN, describenodeproto(np));
		DiaUnDimItem(dia, DNAO_SETPIN);
	}
	DiaSetControl(dia, DNAO_RIGID, (style&WANTFIX) != 0 ? 1 : 0);
	DiaSetControl(dia, DNAO_FIXANGLE, (style&WANTFIXANG) != 0 ? 1 : 0);
	DiaSetControl(dia, DNAO_SLIDABLE, (style&WANTCANTSLIDE) == 0 ? 1 : 0);
	DiaSetControl(dia, DNAO_NEGATED, (style&WANTNEGATED) != 0 ? 1 : 0);
	DiaSetControl(dia, DNAO_DIRECTIONAL, (style&WANTDIRECTIONAL) != 0 ? 1 : 0);
	DiaSetControl(dia, DNAO_ENDSEXTEND, (style&WANTNOEXTEND) == 0 ? 1 : 0);
}

/****************************** ARC SIZE DIALOG ******************************/

/* Arc Size */
static DIALOGITEM us_arcsizedialogitems[] =
{
/*  1 */ {0, {36,96,60,176}, BUTTON, N_("OK")},
/*  2 */ {0, {36,4,60,84}, BUTTON, N_("Cancel")},
/*  3 */ {0, {8,4,24,84}, MESSAGE|INACTIVE, N_("Width")},
/*  4 */ {0, {8,92,24,172}, EDITTEXT, x_("")}
};
static DIALOG us_arcsizedialog = {{75,75,144,260}, N_("Set Arc Size"), 0, 4, us_arcsizedialogitems, 0, 0};

/* special items for the "arc size" dialog: */
#define DARS_WIDTH   4		/* arc width (edit text) */

INTBIG us_arcsizedlog(CHAR *paramstart[])
{
	INTBIG itemHit;
	INTBIG ret;
	static CHAR w[20];
	REGISTER void *dia;

	/* display the arc size dialog box */
	dia = DiaInitDialog(&us_arcsizedialog);
	if (dia == 0) return(0);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	ret = 0;
	if (itemHit != CANCEL)
	{
		estrcpy(w, DiaGetText(dia, DARS_WIDTH));
		paramstart[0] = w;
		ret = 1;
	}
	DiaDoneDialog(dia);
	return(ret);
}

/****************************** ARRAY DIALOG ******************************/

/* Array */
static DIALOGITEM us_arraydialogitems[] =
{
 /*  1 */ {0, {264,412,288,476}, BUTTON, N_("OK")},
 /*  2 */ {0, {264,316,288,380}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,160,44,205}, EDITTEXT, x_("1")},
 /*  4 */ {0, {108,160,124,205}, EDITTEXT, x_("1")},
 /*  5 */ {0, {180,160,196,235}, EDITTEXT, x_("0")},
 /*  6 */ {0, {208,160,224,235}, EDITTEXT, x_("0")},
 /*  7 */ {0, {268,4,284,182}, CHECK, N_("Generate array indices")},
 /*  8 */ {0, {4,216,20,393}, CHECK, N_("Flip alternate columns")},
 /*  9 */ {0, {84,216,100,376}, CHECK, N_("Flip alternate rows")},
 /* 10 */ {0, {28,36,44,151}, MESSAGE, N_("X repeat factor:")},
 /* 11 */ {0, {108,36,124,151}, MESSAGE, N_("Y repeat factor:")},
 /* 12 */ {0, {180,4,196,154}, MESSAGE, N_("X edge overlap:")},
 /* 13 */ {0, {208,4,224,154}, MESSAGE, N_("Y centerline distance:")},
 /* 14 */ {0, {160,244,176,480}, RADIO, N_("Space by edge overlap")},
 /* 15 */ {0, {184,244,200,480}, RADIO, N_("Space by centerline distance")},
 /* 16 */ {0, {28,216,44,425}, CHECK, N_("Stagger alternate columns")},
 /* 17 */ {0, {108,216,124,400}, CHECK, N_("Stagger alternate rows")},
 /* 18 */ {0, {208,244,224,480}, RADIO, N_("Space by characteristic spacing")},
 /* 19 */ {0, {232,244,248,480}, RADIO, N_("Space by last measured distance")},
 /* 20 */ {0, {244,4,260,182}, CHECK, N_("Linear diagonal array")},
 /* 21 */ {0, {52,216,68,425}, CHECK, N_("Center about original")},
 /* 22 */ {0, {132,216,148,425}, CHECK, N_("Center about original")},
 /* 23 */ {0, {154,4,155,480}, DIVIDELINE, x_("")},
 /* 24 */ {0, {292,4,308,302}, CHECK, N_("Only place entries that are DRC correct")}
};
static DIALOG us_arraydialog = {{50,75,367,565}, N_("Array Current Objects"), 0, 24, us_arraydialogitems, 0, 0};

/* special items for the "array" dialog: */
#define DARR_XREPEAT       3		/* X repeat factor (edit text) */
#define DARR_YREPEAT       4		/* Y repeat factor (edit text) */
#define DARR_XSPACING      5		/* X spacing (edit text) */
#define DARR_YSPACING      6		/* Y spacing (edit text) */
#define DARR_ADDNAMES      7		/* Add names (check) */
#define DARR_FLIPX         8		/* Flip in X (check) */
#define DARR_FLIPY         9		/* Flip in Y (check) */
#define DARR_XSPACING_L   12		/* X spacing label (message) */
#define DARR_YSPACING_L   13		/* Y spacing label (message) */
#define DARR_SPACEEDGE    14		/* Space by edge (radio) */
#define DARR_SPACECENTER  15		/* Space by center (radio) */
#define DARR_STAGGERX     16		/* Stagger in X (check) */
#define DARR_STAGGERY     17		/* Stagger in Y (check) */
#define DARR_SPACECHARSP  18		/* Space by char. spacing (radio) */
#define DARR_SPACEMEASURE 19		/* Space by measured dist (radio) */
#define DARR_LINEARDIAG   20		/* Linear diagonal array (check) */
#define DARR_CENTERX      21		/* Center in X (check) */
#define DARR_CENTERY      22		/* Center in Y (check) */
#define DARR_ONLYDRCGOOD  24		/* Only place DRC-clean entries (check) */

INTBIG us_arraydlog(CHAR *paramstart[])
{
	REGISTER INTBIG itemHit, xcentdist, ycentdist, xsize, ysize, lx=0, hx=0, ly=0, hy=0, i,
		chardistx=0, chardisty=0, curspacing, measdistx=0, measdisty=0, x, y,
		thischarx, thischary, swap;
	BOOLEAN xyrev, first, havechar;
	INTBIG xoverlap, yoverlap;
	static INTBIG lastXrepeat = 1, lastYrepeat = 1;
	static INTBIG lastXdist = 0, lastYdist = 0;
	static INTBIG lastspacing = DARR_SPACEEDGE;
	static INTBIG lastXflip = 0, lastYflip = 0;
	static INTBIG lastXcenter = 0, lastYcenter = 0;
	static INTBIG lastXstagger = 0, lastYstagger = 0;
	static INTBIG lastlineardiagonal = 0, lastaddnames = 0, lastdrcgood = 0;
	REGISTER VARIABLE *var;
	CHAR line[40];
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER GEOM **list, *geom;
	REGISTER NODEPROTO *np;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* get the objects to be arrayed */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		us_abortcommand(_("Must select circuitry before arraying it"));
		return(0);
	}
	np = geomparent(list[0]);

	/* display the array dialog box */
	dia = DiaInitDialog(&us_arraydialog);
	if (dia == 0) return(0);
	esnprintf(line, 40, x_("%ld"), lastXrepeat);   DiaSetText(dia, DARR_XREPEAT, line);
	esnprintf(line, 40, x_("%ld"), lastYrepeat);   DiaSetText(dia, DARR_YREPEAT, line);
	DiaSetControl(dia, DARR_FLIPX, lastXflip);
	DiaSetControl(dia, DARR_FLIPY, lastYflip);
	DiaSetControl(dia, DARR_CENTERX, lastXcenter);
	DiaSetControl(dia, DARR_CENTERY, lastYcenter);
	DiaSetControl(dia, DARR_STAGGERX, lastXstagger);
	DiaSetControl(dia, DARR_STAGGERY, lastYstagger);
	DiaSetControl(dia, DARR_ADDNAMES, lastaddnames);
	DiaSetControl(dia, DARR_ONLYDRCGOOD, lastdrcgood);
	DiaSetControl(dia, DARR_LINEARDIAG, lastlineardiagonal);

	/* see if a single cell instance is selected (in which case DRC validity can be done) */
	if (list[0] != NOGEOM && list[1] == NOGEOM && list[0]->entryisnode &&
		list[0]->entryaddr.ni->proto->primindex == 0)
			DiaUnDimItem(dia, DARR_ONLYDRCGOOD); else
				DiaDimItem(dia, DARR_ONLYDRCGOOD);

	/* see if a cell was selected which has a characteristic distance */
	havechar = FALSE;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (!list[i]->entryisnode) continue;
		ni = list[i]->entryaddr.ni;
		if (ni->proto->primindex != 0) continue;
		var = getval((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY,
			x_("FACET_characteristic_spacing"));
		if (var == NOVARIABLE) continue;
		thischarx = ((INTBIG *)var->addr)[0];
		thischary = ((INTBIG *)var->addr)[1];
		xyrev = FALSE;
		if (ni->transpose == 0)
		{
			if (ni->rotation == 900 || ni->rotation == 2700) xyrev = TRUE;
		} else
		{
			if (ni->rotation == 0 || ni->rotation == 1800) xyrev = TRUE;
		}
		if (xyrev)
		{
			swap = thischarx;   thischarx = thischary;   thischary = swap;
		}

		if (havechar)
		{
			/* LINTED "chardistx" and "chardisty" used in proper order */
			if (chardistx != thischarx || chardisty != thischary)
			{
				havechar = FALSE;
				break;
			}
		}
		chardistx = thischarx;
		chardisty = thischary;
		havechar = TRUE;
	}
	if (havechar)
	{
		DiaUnDimItem(dia, DARR_SPACECHARSP);
		if (lastspacing == DARR_SPACECHARSP)
		{
			lastXdist = chardistx;
			lastYdist = chardisty;
		}
	} else
	{
		DiaDimItem(dia, DARR_SPACECHARSP);
		if (lastspacing == DARR_SPACECHARSP)
		{
			lastspacing = DARR_SPACEEDGE;
			lastXdist = lastYdist = 0;
		}
	}

	/* see if there was a measured distance */
	if (us_validmesaure)
	{
		DiaUnDimItem(dia, DARR_SPACEMEASURE);
		measdistx = abs(us_lastmeasurex);
		measdisty = abs(us_lastmeasurey);
		if (lastspacing == DARR_SPACEMEASURE)
		{
			lastXdist = measdistx;
			lastYdist = measdisty;
		}
	} else
	{
		DiaDimItem(dia, DARR_SPACEMEASURE);
		if (lastspacing == DARR_SPACEMEASURE)
		{
			lastspacing = DARR_SPACEEDGE;
			lastXdist = lastYdist = 0;
		}
	}

	DiaSetText(dia, DARR_XSPACING, latoa(lastXdist, 0));
	DiaSetText(dia, DARR_YSPACING, latoa(lastYdist, 0));
	curspacing = lastspacing;
	DiaSetControl(dia, curspacing, 1);
	if (curspacing == DARR_SPACEEDGE)
	{
		DiaSetText(dia, DARR_XSPACING_L, _("X edge overlap:"));
		DiaSetText(dia, DARR_YSPACING_L, _("Y edge overlap:"));
	} else
	{
		DiaSetText(dia, DARR_XSPACING_L, _("X centerline distance:"));
		DiaSetText(dia, DARR_YSPACING_L, _("Y centerline distance:"));
	}

	/* mark the list of nodes and arcs in the cell that will be arrayed */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;
	for(i=0; list[i] != NOGEOM; i++)
	{
		geom = list[i];
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;
			ni->temp1 = 1;
		} else
		{
			ai = geom->entryaddr.ai;
			ai->temp1 = 1;
			ai->end[0].nodeinst->temp1 = ai->end[1].nodeinst->temp1 = 1;
		}
	}

	/* determine spacing between arrayed objects */
	first = TRUE;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->temp1 == 0) continue;
		if (first)
		{
			lx = ni->geom->lowx;   hx = ni->geom->highx;
			ly = ni->geom->lowy;   hy = ni->geom->highy;
			first = FALSE;
		} else
		{
			if (ni->geom->lowx < lx) lx = ni->geom->lowx;
			if (ni->geom->highx > hx) hx = ni->geom->highx;
			if (ni->geom->lowy < ly) ly = ni->geom->lowy;
			if (ni->geom->highy > hy) hy = ni->geom->highy;
		}
	}
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 == 0) continue;
		if (first)
		{
			lx = ai->geom->lowx;   hx = ai->geom->highx;
			ly = ai->geom->lowy;   hy = ai->geom->highy;
			first = FALSE;
		} else
		{
			if (ai->geom->lowx < lx) lx = ai->geom->lowx;
			if (ai->geom->highx > hx) hx = ai->geom->highx;
			if (ai->geom->lowy < ly) ly = ai->geom->lowy;
			if (ai->geom->highy > hy) hy = ai->geom->highy;
		}
	}
	xsize = xcentdist = hx - lx;
	ysize = ycentdist = hy - ly;
	xoverlap = yoverlap = 0;

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK &&
			DiaValidEntry(dia, DARR_XREPEAT) && DiaValidEntry(dia, DARR_YREPEAT) &&
			DiaValidEntry(dia, DARR_XSPACING) && DiaValidEntry(dia, DARR_YSPACING)) break;
		if (itemHit == DARR_ADDNAMES || itemHit == DARR_FLIPX ||
			itemHit == DARR_FLIPY || itemHit == DARR_STAGGERX ||
			itemHit == DARR_STAGGERY || itemHit == DARR_LINEARDIAG ||
			itemHit == DARR_CENTERX || itemHit == DARR_CENTERY ||
			itemHit == DARR_ONLYDRCGOOD)
				DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
		if (itemHit == DARR_SPACEEDGE || itemHit == DARR_SPACECENTER ||
			itemHit == DARR_SPACECHARSP || itemHit == DARR_SPACEMEASURE)
		{
			DiaSetControl(dia, DARR_SPACEEDGE, 0);
			DiaSetControl(dia, DARR_SPACECENTER, 0);
			DiaSetControl(dia, DARR_SPACECHARSP, 0);
			DiaSetControl(dia, DARR_SPACEMEASURE, 0);
			DiaSetControl(dia, itemHit, 1);
			x = atola(DiaGetText(dia, DARR_XSPACING), 0);   y = atola(DiaGetText(dia, DARR_YSPACING), 0);
			switch (curspacing)
			{
				case DARR_SPACEEDGE:   xoverlap = x;    yoverlap = y;    break;
				case DARR_SPACECENTER: xcentdist = x;   ycentdist = y;   break;
			}
			curspacing = itemHit;
			if (curspacing == DARR_SPACEEDGE)
			{
				DiaSetText(dia, DARR_XSPACING_L, _("X edge overlap:"));
				DiaSetText(dia, DARR_YSPACING_L, _("Y edge overlap:"));
			} else
			{
				DiaSetText(dia, DARR_XSPACING_L, _("X centerline distance:"));
				DiaSetText(dia, DARR_YSPACING_L, _("Y centerline distance:"));
			}
			switch (curspacing)
			{
				case DARR_SPACEEDGE:    x = xoverlap;    y = yoverlap;    break;
				case DARR_SPACECENTER:  x = xcentdist;   y = ycentdist;   break;
				case DARR_SPACECHARSP:  x = chardistx;   y = chardisty;   break;
				case DARR_SPACEMEASURE: x = measdistx;   y = measdisty;   break;
			}
			DiaSetText(dia, DARR_XSPACING, latoa(x, 0));
			DiaSetText(dia, DARR_YSPACING, latoa(y, 0));
			continue;
		}
	}
	lastXrepeat = eatoi(DiaGetText(dia, DARR_XREPEAT));
	lastYrepeat = eatoi(DiaGetText(dia, DARR_YREPEAT));
	lastXdist = atola(DiaGetText(dia, DARR_XSPACING), 0);
	lastYdist = atola(DiaGetText(dia, DARR_YSPACING), 0);
	lastXflip = DiaGetControl(dia, DARR_FLIPX);
	lastYflip = DiaGetControl(dia, DARR_FLIPY);
	lastXcenter = DiaGetControl(dia, DARR_CENTERX);
	lastYcenter = DiaGetControl(dia, DARR_CENTERY);
	lastXstagger = DiaGetControl(dia, DARR_STAGGERX);
	lastYstagger = DiaGetControl(dia, DARR_STAGGERY);
	lastaddnames = DiaGetControl(dia, DARR_ADDNAMES);
	lastdrcgood = DiaGetControl(dia, DARR_ONLYDRCGOOD);
	lastlineardiagonal = DiaGetControl(dia, DARR_LINEARDIAG);
	lastspacing = curspacing;

	paramstart[0] = x_("");
	if (itemHit != CANCEL)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, DiaGetText(dia, DARR_XREPEAT));
		if (lastXflip != 0) addtoinfstr(infstr, 'f');
		if (lastXstagger != 0) addtoinfstr(infstr, 's');
		if (lastXcenter != 0) addtoinfstr(infstr, 'c');
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, DiaGetText(dia, DARR_YREPEAT));
		if (lastYflip != 0) addtoinfstr(infstr, 'f');
		if (lastYstagger != 0) addtoinfstr(infstr, 's');
		if (lastYcenter != 0) addtoinfstr(infstr, 'c');
		addtoinfstr(infstr, ' ');
		xoverlap = lastXdist;
		yoverlap = lastYdist;
		if (DiaGetControl(dia, DARR_SPACEEDGE) == 0)
		{
			xoverlap = xsize - xoverlap;
			yoverlap = ysize - yoverlap;
		}
		addstringtoinfstr(infstr, latoa(xoverlap, 0));
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, latoa(yoverlap, 0));
		if (lastaddnames == 0) addstringtoinfstr(infstr, x_(" no-names"));
		if (lastlineardiagonal != 0) addstringtoinfstr(infstr, x_(" diagonal"));
		if (lastdrcgood != 0) addstringtoinfstr(infstr, x_(" only-drc-valid"));
		if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
		allocstring(&us_returneddialogstring, returninfstr(infstr), us_tool->cluster);
		paramstart[0] = us_returneddialogstring;
	}
	DiaDoneDialog(dia);
	return(1);
}

/*************************** "ARTWORK COLOR" AND "LAYER DISPLAY" DIALOGS ***************************/

/* Layer Patterns */
static UCHAR1 us_icon300[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0210, 0210, 0210, 0210, 0104, 0104, 021, 021, 042, 042, 042, 042, 021, 021, 0104, 0104,
	0210, 0210, 0210, 0210, 0104, 0104, 021, 021, 042, 042, 042, 042, 021, 021, 0104, 0104,
	0210, 0210, 0210, 0210, 0104, 0104, 021, 021, 042, 042, 042, 042, 021, 021, 0104, 0104,
	0210, 0210, 0210, 0210, 0104, 0104, 021, 021, 042, 042, 042, 042, 021, 021, 0104, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon301[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0314, 0314, 0377, 0377, 0314, 0314, 0, 0, 063, 063, 0377, 0377, 063, 063, 0, 0,
	0314, 0314, 0377, 0377, 0314, 0314, 0, 0, 063, 063, 0377, 0377, 063, 063, 0, 0,
	0314, 0314, 0377, 0377, 0314, 0314, 0, 0, 063, 063, 0377, 0377, 063, 063, 0, 0,
	0314, 0314, 0377, 0377, 0314, 0314, 0, 0, 063, 063, 0377, 0377, 063, 063, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon302[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0125, 0125, 0140, 0140, 0125, 0125, 0220, 0220, 0125, 0125, 0220, 0220, 0125, 0125, 0140, 0140,
	0125, 0125, 06, 06, 0125, 0125, 011, 011, 0125, 0125, 011, 011, 0125, 0125, 06, 06,
	0125, 0125, 0140, 0140, 0125, 0125, 0220, 0220, 0125, 0125, 0220, 0220, 0125, 0125, 0140, 0140,
	0125, 0125, 06, 06, 0125, 0125, 011, 011, 0125, 0125, 011, 011, 0, 0, 06, 06,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon303[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	042, 042, 0104, 0104, 0, 0, 021, 021, 0210, 0210, 0104, 0104, 0, 0, 021, 021,
	042, 042, 0104, 0104, 0, 0, 021, 021, 0210, 0210, 0104, 0104, 0, 0, 021, 021,
	042, 042, 0104, 0104, 0, 0, 021, 021, 0210, 0210, 0104, 0104, 0, 0, 021, 021,
	042, 042, 0104, 0104, 0, 0, 021, 021, 0210, 0210, 0104, 0104, 0, 0, 021, 021,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon304[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	020, 020, 04, 04, 040, 040, 02, 02, 0100, 0100, 01, 01, 0200, 0200, 0200, 0200,
	01, 01, 0100, 0100, 02, 02, 040, 040, 04, 04, 020, 020, 010, 010, 010, 010,
	020, 020, 04, 04, 040, 040, 02, 02, 0100, 0100, 01, 01, 0200, 0200, 0200, 0200,
	01, 01, 0100, 0100, 02, 02, 040, 040, 04, 04, 020, 020, 010, 010, 010, 010,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon305[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0100, 0100, 020, 0, 0200, 0200, 0, 040, 01, 01, 0100, 0, 02, 02, 0, 0200,
	01, 01, 0, 01, 0200, 0200, 02, 0, 0100, 0100, 0, 04, 040, 040, 010, 0,
	0100, 0100, 0, 020, 0200, 0200, 040, 0, 01, 01, 0, 0100, 02, 02, 0200, 0,
	01, 01, 01, 0, 0200, 0200, 0, 02, 0100, 0100, 04, 0, 040, 040, 0, 010,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon306[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	010, 0, 0, 0, 0, 04, 03, 03, 02, 0, 0204, 0204, 0, 01, 03, 03,
	0, 0200, 0, 0, 0100, 0, 060, 060, 0, 040, 0110, 0110, 020, 0, 060, 060,
	0, 010, 0, 0, 04, 0, 03, 03, 0, 02, 0204, 0204, 01, 0, 03, 03,
	0200, 0, 0, 0, 0, 0100, 060, 060, 040, 0, 0110, 0110, 0, 020, 060, 060,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon307[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	034, 034, 0, 0, 076, 076, 0314, 0314, 066, 066, 0, 0, 076, 076, 0314, 0314,
	034, 034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	034, 034, 0, 0, 076, 076, 0314, 0314, 066, 066, 0, 0, 076, 076, 0314, 0314,
	034, 034, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon308[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 042, 042, 021, 021, 0210, 0210, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 042, 042, 021, 021, 0210, 0210, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 042, 042, 021, 021, 0210, 0210, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 042, 042, 021, 021, 0210, 0210, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon309[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 042, 042, 042, 042, 0104, 0104, 0125, 0125, 0210, 0210, 042, 042,
	0, 0, 0, 0, 042, 042, 042, 042, 0104, 0104, 0125, 0125, 0210, 0210, 042, 042,
	0, 0, 0, 0, 042, 042, 042, 042, 0104, 0104, 0125, 0125, 0210, 0210, 042, 042,
	0, 0, 0, 0, 042, 042, 042, 042, 0104, 0104, 0125, 0125, 0210, 0210, 042, 042,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static UCHAR1 us_icon310[] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377,
	0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377,
	0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377,
	0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377, 0, 0, 0377, 0377,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
UINTSML us_predefpats[] =
{
	0x8888,  /* X   X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x8888,  /* X   X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x8888,  /* X   X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */
	0x8888,  /* X   X   X   X    */
	0x4444,  /*  X   X   X   X   */
	0x2222,  /*   X   X   X   X  */
	0x1111,  /*    X   X   X   X */

	0x8888,  /* X   X   X   X    */
	0x1111,  /*    X   X   X   X */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */
	0x1111,  /*    X   X   X   X */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */
	0x1111,  /*    X   X   X   X */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */
	0x1111,  /*    X   X   X   X */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */

	0xCCCC,  /* XX  XX  XX  XX   */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x3333,  /*   XX  XX  XX  XX */
	0x3333,  /*   XX  XX  XX  XX */
	0xCCCC,  /* XX  XX  XX  XX   */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x3333,  /*   XX  XX  XX  XX */
	0x3333,  /*   XX  XX  XX  XX */
	0xCCCC,  /* XX  XX  XX  XX   */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x3333,  /*   XX  XX  XX  XX */
	0x3333,  /*   XX  XX  XX  XX */
	0xCCCC,  /* XX  XX  XX  XX   */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x3333,  /*   XX  XX  XX  XX */
	0x3333,  /*   XX  XX  XX  XX */

	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0x0000,  /*                  */

	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */
	0xAAAA,  /* X X X X X X X X  */

	0x6060,  /*  XX      XX      */
	0x9090,  /* X  X    X  X     */
	0x9090,  /* X  X    X  X     */
	0x6060,  /*  XX      XX      */
	0x0606,  /*      XX      XX  */
	0x0909,  /*     X  X    X  X */
	0x0909,  /*     X  X    X  X */
	0x0606,  /*      XX      XX  */
	0x6060,  /*  XX      XX      */
	0x9090,  /* X  X    X  X     */
	0x9090,  /* X  X    X  X     */
	0x6060,  /*  XX      XX      */
	0x0606,  /*      XX      XX  */
	0x0909,  /*     X  X    X  X */
	0x0909,  /*     X  X    X  X */
	0x0606,  /*      XX      XX  */

	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */

	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */
	0x4444,  /*  X   X   X   X   */
	0x1111,  /*    X   X   X   X */

	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */
	0x1010,  /*    X       X     */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0404,  /*      X       X   */
	0x0808,  /*     X       X    */

	0x0808,  /*     X       X    */
	0x0404,  /*      X       X   */
	0x0202,  /*       X       X  */
	0x0101,  /*        X       X */
	0x8080,  /* X       X        */
	0x4040,  /*  X       X       */
	0x2020,  /*   X       X      */
	0x1010,  /*    X       X     */
	0x0808,  /*     X       X    */
	0x0404,  /*      X       X   */
	0x0202,  /*       X       X  */
	0x0101,  /*        X       X */
	0x8080,  /* X       X        */
	0x4040,  /*  X       X       */
	0x2020,  /*   X       X      */
	0x1010,  /*    X       X     */

	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0101,  /*        X       X */
	0x8080,  /* X       X        */
	0x4040,  /*  X       X       */
	0x2020,  /*   X       X      */
	0x4040,  /*  X       X       */
	0x8080,  /* X       X        */
	0x0101,  /*        X       X */
	0x0202,  /*       X       X  */
	0x0101,  /*        X       X */
	0x8080,  /* X       X        */
	0x4040,  /*  X       X       */
	0x2020,  /*   X       X      */

	0x2020,  /*   X       X      */
	0x0000,  /*                  */
	0x8080,  /* X       X        */
	0x0000,  /*                  */
	0x0202,  /*       X       X  */
	0x0000,  /*                  */
	0x0808,  /*     X       X    */
	0x0000,  /*                  */
	0x2020,  /*   X       X      */
	0x0000,  /*                  */
	0x8080,  /* X       X        */
	0x0000,  /*                  */
	0x0202,  /*       X       X  */
	0x0000,  /*                  */
	0x0808,  /*     X       X    */
	0x0000,  /*                  */

	0x0808,  /*     X       X    */
	0x0000,  /*                  */
	0x0202,  /*       X       X  */
	0x0000,  /*                  */
	0x8080,  /* X       X        */
	0x0000,  /*                  */
	0x2020,  /*   X       X      */
	0x0000,  /*                  */
	0x0808,  /*     X       X    */
	0x0000,  /*                  */
	0x0202,  /*       X       X  */
	0x0000,  /*                  */
	0x8080,  /* X       X        */
	0x0000,  /*                  */
	0x2020,  /*   X       X      */
	0x0000,  /*                  */

	0x0000,  /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030,  /*   XX      XX     */
	0x0000,  /*                  */
	0x0303,  /*       XX      XX */
	0x4848,  /*  X  X    X  X    */
	0x0303,  /*       XX      XX */
	0x0000,  /*                  */
	0x3030,  /*   XX      XX     */
	0x8484,  /* X    X  X    X   */
	0x3030,  /*   XX      XX     */

	0x1C1C,  /*    XXX     XXX   */
	0x3E3E,  /*   XXXXX   XXXXX  */
	0x3636,  /*   XX XX   XX XX  */
	0x3E3E,  /*   XXXXX   XXXXX  */
	0x1C1C,  /*    XXX     XXX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x1C1C,  /*    XXX     XXX   */
	0x3E3E,  /*   XXXXX   XXXXX  */
	0x3636,  /*   XX XX   XX XX  */
	0x3E3E,  /*   XXXXX   XXXXX  */
	0x1C1C,  /*    XXX     XXX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */

	0x0000,  /*                  */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0xCCCC,  /* XX  XX  XX  XX   */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */

	0x0000,  /*                  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x8888,  /* X   X   X   X    */

	0x0000,  /*                  */
	0x0000,  /*                  */
	0x1111,  /*    X   X   X   X */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x1111,  /*    X   X   X   X */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x1111,  /*    X   X   X   X */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x1111,  /*    X   X   X   X */
	0x0000,  /*                  */

	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x4444,  /*  X   X   X   X   */
	0x8888,  /* X   X   X   X    */

	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x5555,  /*  X X X X X X X X */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x5555,  /*  X X X X X X X X */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x5555,  /*  X X X X X X X X */
	0x2222,  /*   X   X   X   X  */
	0x0000,  /*                  */
	0x2222,  /*   X   X   X   X  */
	0x5555,  /*  X X X X X X X X */
	0x2222,  /*   X   X   X   X  */

	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */
	0x0000,  /*                  */

	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF,  /* XXXXXXXXXXXXXXXX */
	0xFFFF   /* XXXXXXXXXXXXXXXX */
};

#define DPATART_PATTERNAREA  4		/* Pattern squares for both dialogs (user item) */

static RECTAREA us_strokerect;
static INTBIG   us_origpattern[16];
static BOOLEAN  us_strokeup;
static BOOLEAN  us_patternchanged;

static void us_redrawpattern(RECTAREA*, void *dia);
static void us_patternstroke(INTBIG, INTBIG);

void us_redrawpattern(RECTAREA *bigr, void *dia)
{
	Q_UNUSED( bigr );
	INTBIG j, k, bits;
	RECTAREA r;

	for(j=0; j<16; j++)
	{
		bits = us_origpattern[j] & 0xFFFF;
		for(k=0; k<16; k++)
		{
			r.left = (INTSML)(us_strokerect.left + k*16);   r.right = r.left + 17;
			r.top = (INTSML)(us_strokerect.top + j*16);     r.bottom = r.top + 17;
			if ((bits & (1<<(15-k))) != 0) DiaDrawRect(dia, DPATART_PATTERNAREA, &r, 0, 0, 0); else
				DiaFrameRect(dia, DPATART_PATTERNAREA, &r);
		}
	}
}

void us_patternstroke(INTBIG x, INTBIG y)
{
	REGISTER INTBIG j, k, bits;
	RECTAREA r;

	j = (y - us_strokerect.top) / 16;  if (j < 0) j = 0;  if (j > 15) j = 15;
	k = (x - us_strokerect.left) / 16;  if (k < 0) k = 0;  if (k > 15) k = 15;
	r.left = (INTSML)(us_strokerect.left + k*16);   r.right = r.left + 17;
	r.top = (INTSML)(us_strokerect.top + j*16);     r.bottom = r.top + 17;
	bits = us_origpattern[j] & 0xFFFF;
	if (!us_strokeup)
	{
		DiaFrameRect(us_trackingdialog, DPATART_PATTERNAREA, &r);
		bits &= ~(1<<(15-k));
	} else
	{
		DiaDrawRect(us_trackingdialog, DPATART_PATTERNAREA, &r, 0, 0, 0);
		bits |= 1<<(15-k);
	}
	us_origpattern[j] = bits;
	us_patternchanged = TRUE;
}

/* Artwork Color */
static DIALOGITEM us_artworkdialogitems[] =
{
 /*  1 */ {0, {260,312,284,376}, BUTTON, N_("OK")},
 /*  2 */ {0, {216,312,240,376}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {108,280,124,420}, RADIO, N_("Outlined Pattern")},
 /*  4 */ {0, {34,4,290,260}, USERDRAWN, x_("")},
 /*  5 */ {0, {52,280,68,376}, RADIO, N_("Solid color")},
 /*  6 */ {0, {148,280,164,332}, MESSAGE, N_("Color:")},
 /*  7 */ {0, {80,280,96,376}, RADIO, N_("Use Pattern")},
 /*  8 */ {0, {0,8,32,40}, ICON, (CHAR *)us_icon300},
 /*  9 */ {0, {0,40,32,72}, ICON, (CHAR *)us_icon301},
 /* 10 */ {0, {0,72,32,104}, ICON, (CHAR *)us_icon302},
 /* 11 */ {0, {0,104,32,136}, ICON, (CHAR *)us_icon303},
 /* 12 */ {0, {0,136,32,168}, ICON, (CHAR *)us_icon304},
 /* 13 */ {0, {0,168,32,200}, ICON, (CHAR *)us_icon305},
 /* 14 */ {0, {0,200,32,232}, ICON, (CHAR *)us_icon306},
 /* 15 */ {0, {0,232,32,264}, ICON, (CHAR *)us_icon307},
 /* 16 */ {0, {0,264,32,296}, ICON, (CHAR *)us_icon308},
 /* 17 */ {0, {0,296,32,328}, ICON, (CHAR *)us_icon309},
 /* 18 */ {0, {0,328,32,360}, ICON, (CHAR *)us_icon310},
 /* 19 */ {0, {168,280,184,420}, POPUP, x_("")}
};
static DIALOG us_artworkdialog = {{50,75,349,505}, N_("Set Look of Highlighted"), 0, 19, us_artworkdialogitems, 0, 0};

/* special items for the "artwork color" dialog: */
#define DART_OUTLINE      3		/* Outline pattern (radio) */
#define DART_SOLID        5		/* Solid (radio) */
#define DART_PATTERN      7		/* Use Pattern (radio) */
#define DART_PREPATFIRST  8		/* First predefined pattern (user item) */
#define DART_PREPATLAST  18		/* Last predefined pattern (user item) */
#define DART_COLOR       19		/* Color (popup) */

INTBIG us_artlookdlog(void)
{
	RECTAREA r;
	INTBIG addr, type, x, y;
	REGISTER INTBIG i, j, k, bits, itemHit, len;
	UINTSML spattern[16];
	REGISTER BOOLEAN foundart;
	REGISTER GEOM **list;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	CHAR *newlang[25], *colorname, *colorsymbol;
	REGISTER void *dia;

	/* make sure there is an artwork primitive highlighted */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	if (list[0] == NOGEOM)
	{
		ttyputerr(M_("Nothing is selected"));
		return(0);
	}
	foundart = FALSE;
	for(i=0; list[i] != NOGEOM; i++)
	{
		if (list[i]->entryisnode)
		{
			ni = list[i]->entryaddr.ni;
			if (ni->proto->primindex != 0 && ni->proto->tech == art_tech) foundart = TRUE;
			addr = (INTBIG)ni;
			type = VNODEINST;
		} else
		{
			ai = list[i]->entryaddr.ai;
			if (ai->proto->tech == art_tech) foundart = TRUE;
			addr = (INTBIG)ai;
			type = VARCINST;
		}
	}
	if (!foundart)
	{
		us_abortcommand(_("First select nodes or arcs in the Artwork technology"));
		return(0);
	}

	/* display the artwork dialog box */
	dia = DiaInitDialog(&us_artworkdialog);
	if (dia == 0) return(0);
	for(i=0; i<25; i++)
	{
		(void)ecolorname(us_colorvaluelist[i], &colorname, &colorsymbol);
		newlang[i] = TRANSLATE(colorname);
	}
	DiaSetPopup(dia, DART_COLOR, 25, newlang);
	DiaItemRect(dia, DPATART_PATTERNAREA, &us_strokerect);
	DiaRedispRoutine(dia, DPATART_PATTERNAREA, us_redrawpattern);
	DiaFrameRect(dia, DPATART_PATTERNAREA, &us_strokerect);

	/* set existing conditions */
	var = getvalkey(addr, type, VINTEGER, art_colorkey);
	if (var != NOVARIABLE)
	{
		for(i=0; i<25; i++) if (us_colorvaluelist[i] == var->addr)
		{
			DiaSetPopupEntry(dia, DART_COLOR, i);
			break;
		}
	}
	DiaSetControl(dia, DART_SOLID, 1);
	for(i=0; i<16; i++) us_origpattern[i] = 0;
	var = getvalkey(addr, type, -1, art_patternkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		if (len == 16 || len == 8)
		{
			DiaSetControl(dia, DART_SOLID, 0);
			if ((var->type&VTYPE) == VINTEGER)
			{
				for(i=0; i<len; i++) us_origpattern[i] = ((UINTBIG *)var->addr)[i];
				DiaSetControl(dia, DART_PATTERN, 1);
			} else if ((var->type&VTYPE) == VSHORT)
			{
				for(i=0; i<len; i++) us_origpattern[i] = ((UINTSML *)var->addr)[i];
				DiaSetControl(dia, DART_OUTLINE, 1);
			}
			if (len == 8)
			{
				for(i=0; i<8; i++) us_origpattern[i+8] = us_origpattern[i];
			}
		}
	}
	us_redrawpattern(&us_strokerect, dia);

	/* loop until done */
	us_patternchanged = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DPATART_PATTERNAREA)
		{
			DiaGetMouse(dia, &x, &y);
			j = (y - us_strokerect.top) / 16;  if (j < 0) j = 0;  if (j > 15) j = 15;
			k = (x - us_strokerect.left) / 16;  if (k < 0) k = 0;  if (k > 15) k = 15;
			bits = us_origpattern[j] & 0xFFFF;
			if ((bits & (1<<(15-k))) != 0) us_strokeup = FALSE; else
				us_strokeup = TRUE;
			us_trackingdialog = dia;
			DiaTrackCursor(dia, us_patternstroke);
			continue;
		}
		if (itemHit == DART_SOLID || itemHit == DART_PATTERN || itemHit == DART_OUTLINE)
		{
			DiaSetControl(dia, DART_SOLID, 0);
			DiaSetControl(dia, DART_PATTERN, 0);
			DiaSetControl(dia, DART_OUTLINE, 0);
			DiaSetControl(dia, itemHit, 1);
			us_patternchanged = TRUE;
			continue;
		}
		if (itemHit >= DART_PREPATFIRST && itemHit <= DART_PREPATLAST)
		{
			DiaGetMouse(dia, &x, &y);
			DiaItemRect(dia, itemHit, &r);
			j = (r.top+r.bottom)/2;
			if (y < j-8 || y > j+8) continue;
			i = (itemHit-DART_PREPATFIRST) * 2;
			if (x > (r.left+r.right)/2) i++;
			i *= 16;
			for(j=0; j<16; j++) us_origpattern[j] = us_predefpats[i++] & 0xFFFF;
			us_redrawpattern(&us_strokerect, dia);
			us_patternchanged = TRUE;
			continue;
		}
		if (itemHit == DART_COLOR)
		{
			us_patternchanged = TRUE;
			continue;
		}
	}

	if (itemHit != CANCEL && us_patternchanged)
	{
		/* prepare for change */
		us_pushhighlight();
		us_clearhighlightcount();
		for(i=0; list[i] != NOGEOM; i++)
		{
			if (list[i]->entryisnode)
			{
				ni = list[i]->entryaddr.ni;
				if (ni->proto->primindex == 0 || ni->proto->tech != art_tech) continue;
				addr = (INTBIG)ni;
				type = VNODEINST;
			} else
			{
				ai = list[i]->entryaddr.ai;
				if (ai->proto->tech != art_tech) continue;
				addr = (INTBIG)ai;
				type = VARCINST;
			}
			startobjectchange(addr, type);

			/* make the change */
			if (DiaGetControl(dia, DART_SOLID) != 0)
			{
				var = getvalkey(addr, type, -1, art_patternkey);
				if (var != NOVARIABLE) (void)delvalkey(addr, type, art_patternkey);
			} else if (DiaGetControl(dia, DART_PATTERN) != 0)
			{
				(void)setvalkey(addr, type, art_patternkey, (INTBIG)us_origpattern,
					VINTEGER|VISARRAY|(16<<VLENGTHSH));
			} else
			{
				for(j=0; j<16; j++) spattern[j] = (UINTSML)us_origpattern[j];
				(void)setvalkey(addr, type, art_patternkey, (INTBIG)spattern,
					VSHORT|VISARRAY|(16<<VLENGTHSH));
			}
			j = DiaGetPopupEntry(dia, DART_COLOR);
			if (us_colorvaluelist[j] == BLACK)
			{
				/* black is the default */
				var = getvalkey(addr, type, VINTEGER, art_colorkey);
				if (var != NOVARIABLE) (void)delvalkey(addr, type, art_colorkey);
			} else (void)setvalkey(addr, type, art_colorkey, us_colorvaluelist[j], VINTEGER);

			/* complete change */
			endobjectchange(addr, type);
		}
		us_pophighlight(FALSE);
	}
	DiaDoneDialog(dia);
	return(0);
}

/* Layer Display Options */
static DIALOGITEM us_patterndialogitems[] =
{
 /*  1 */ {0, {204,296,228,360}, BUTTON, N_("OK")},
 /*  2 */ {0, {164,296,188,360}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,8,20,184}, MESSAGE, N_("Appearance of layer:")},
 /*  4 */ {0, {112,16,368,272}, USERDRAWN, x_("")},
 /*  5 */ {0, {4,184,20,368}, POPUP, x_("")},
 /*  6 */ {0, {64,184,80,336}, CHECK, N_("Outline Pattern")},
 /*  7 */ {0, {64,8,80,160}, CHECK, N_("Use Stipple Pattern")},
 /*  8 */ {0, {28,8,60,40}, ICON, (CHAR *)us_icon300},
 /*  9 */ {0, {28,40,60,72}, ICON, (CHAR *)us_icon301},
 /* 10 */ {0, {28,72,60,104}, ICON, (CHAR *)us_icon302},
 /* 11 */ {0, {28,104,60,136}, ICON, (CHAR *)us_icon303},
 /* 12 */ {0, {28,136,60,168}, ICON, (CHAR *)us_icon304},
 /* 13 */ {0, {28,168,60,200}, ICON, (CHAR *)us_icon305},
 /* 14 */ {0, {28,200,60,232}, ICON, (CHAR *)us_icon306},
 /* 15 */ {0, {28,232,60,264}, ICON, (CHAR *)us_icon307},
 /* 16 */ {0, {28,264,60,296}, ICON, (CHAR *)us_icon308},
 /* 17 */ {0, {28,296,60,328}, ICON, (CHAR *)us_icon309},
 /* 18 */ {0, {28,328,60,360}, ICON, (CHAR *)us_icon310},
 /* 19 */ {0, {88,184,104,368}, POPUP, x_("")},
 /* 20 */ {0, {88,8,104,184}, MESSAGE, N_("Color:")}
};
static DIALOG us_patterndialog = {{50,75,427,453}, N_("Layer Display Options"), 0, 20, us_patterndialogitems, 0, 0};

/* special items for the "Layer Display Options" dialog: */
#define DPAT_LAYER        5		/* Layer (popup) */
#define DPAT_OUTLINE      6		/* Outline this layer (check) */
#define DPAT_STIPPLE      7		/* Stipple this layer (check) */
#define DPAT_PREPATFIRST  8		/* First predefined pattern (user item) */
#define DPAT_PREPATLAST  18		/* Last predefined pattern (user item) */
#define DPAT_COLOR       19		/* Color (popup) */

INTBIG us_patterndlog(void)
{
	INTBIG itemHit, curlayer, i, j, k, newpat[18], x, y, bits, newcolor;
	INTBIG *origbits, *orignature, *origcolor;
	CHAR *colorname, *colorsymbol, *newlang[25];
	BOOLEAN *origchanged, warnedofsolidopaque;
	RECTAREA r;
	REGISTER VARIABLE *var;
	REGISTER CHAR **layernames;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the layer display options dialog box */
	dia = DiaInitDialog(&us_patterndialog);
	if (dia == 0) return(0);

	/* get memory to save state */
	origbits = (INTBIG *)emalloc(el_curtech->layercount * 16 * SIZEOFINTBIG, us_tool->cluster);
	if (origbits == 0) return(0);
	orignature = (INTBIG *)emalloc(el_curtech->layercount * SIZEOFINTBIG, us_tool->cluster);
	if (orignature == 0) return(0);
	origcolor = (INTBIG *)emalloc(el_curtech->layercount * SIZEOFINTBIG, us_tool->cluster);
	if (origcolor == 0) return(0);
	origchanged = (BOOLEAN *)emalloc(el_curtech->layercount * (sizeof (BOOLEAN)), us_tool->cluster);
	if (origchanged == 0) return(0);
	for(i=0; i<el_curtech->layercount; i++)
	{
		orignature[i] = (el_curtech->layers[i])->colstyle;
		origcolor[i] = (el_curtech->layers[i])->col;
		origchanged[i] = FALSE;
		for(j=0; j<16; j++) origbits[i*16+j] = (el_curtech->layers[i])->raster[j];

		/* see if there is an override */
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("TECH_layer_pattern_"));
		addstringtoinfstr(infstr, layername(el_curtech, i));
		var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER|VISARRAY, returninfstr(infstr));
		if (var != NOVARIABLE)
		{
			orignature[i] = ((INTBIG *)var->addr)[16];
			for(j=0; j<16; j++) origbits[i*16+j] = ((INTBIG *)var->addr)[j];
			if (getlength(var) > 17) origcolor[i] = ((INTBIG *)var->addr)[17];
		}
	}
	curlayer = 0;
	DiaItemRect(dia, DPATART_PATTERNAREA, &us_strokerect);
	DiaRedispRoutine(dia, DPATART_PATTERNAREA, us_redrawpattern);
	DiaFrameRect(dia, DPATART_PATTERNAREA, &us_strokerect);

	/* setup list of layer names */
	layernames = (CHAR **)emalloc(el_curtech->layercount * (sizeof (CHAR *)), el_tempcluster);
	for(i=0; i<el_curtech->layercount; i++)
		(void)allocstring(&layernames[i], layername(el_curtech, i), el_tempcluster);
	DiaSetPopup(dia, DPAT_LAYER, el_curtech->layercount, layernames);
	for(i=0; i<el_curtech->layercount; i++)
		efree(layernames[i]);
	efree((CHAR *)layernames);

	/* setup list of color names */
	for(i=0; i<25; i++)
	{
		(void)ecolorname(us_colorvaluelist[i], &colorname, &colorsymbol);
		newlang[i] = TRANSLATE(colorname);
	}
	DiaSetPopup(dia, DPAT_COLOR, 25, newlang);

	/* setup initial layer: number 0 */
	curlayer = 0;
	DiaSetPopupEntry(dia, DPAT_LAYER, curlayer);
	if ((orignature[curlayer]&NATURE) == PATTERNED)
	{
		DiaUnDimItem(dia, DPAT_OUTLINE);
		DiaSetControl(dia, DPAT_STIPPLE, 1);
		if ((orignature[curlayer]&OUTLINEPAT) != 0) DiaSetControl(dia, DPAT_OUTLINE, 1);
	} else DiaDimItem(dia, DPAT_OUTLINE);
	for(j=0; j<16; j++) us_origpattern[j] = origbits[curlayer*16+j] & 0xFFFF;
	for(j=0; j<25; j++) if (origcolor[curlayer] == us_colorvaluelist[j]) break;
	if (j < 25) DiaSetPopupEntry(dia, DPAT_COLOR, j);
	us_redrawpattern(&us_strokerect, dia);

	/* loop until done */
	warnedofsolidopaque = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK) break;
		if (itemHit == DPAT_COLOR)
		{
			newcolor = us_colorvaluelist[DiaGetPopupEntry(dia, DPAT_COLOR)];
			if ((newcolor&LAYERO) != 0 && (orignature[curlayer]&NATURE) == SOLIDC)
			{
				if (!warnedofsolidopaque)
					DiaMessageInDialog(_("Warning: opaque layers should be stippled (make this layer be stippled or restore its transparent color)"));
				warnedofsolidopaque = TRUE;
			}
			origcolor[curlayer] = newcolor;
			origchanged[curlayer] = TRUE;
			continue;
		}
		if (itemHit == DPAT_STIPPLE)
		{
			/* the "Use Stipple" box */
			if (DiaGetControl(dia, DPAT_STIPPLE) == 0)
			{
				DiaSetControl(dia, DPAT_STIPPLE, 1);
				DiaUnDimItem(dia, DPAT_OUTLINE);
				orignature[curlayer] = (orignature[curlayer] & ~NATURE) | PATTERNED;
			} else
			{
				/* layer becoming solid (not stippled) */
				if ((origcolor[curlayer]&LAYERO) != 0)
				{
					if (!warnedofsolidopaque)
						DiaMessageInDialog(_("Warning: opaque layers should be stippled (give this layer a transparent color, or restore stippling)"));
					warnedofsolidopaque = TRUE;
				}
				DiaSetControl(dia, DPAT_STIPPLE, 0);
				DiaSetControl(dia, DPAT_OUTLINE, 0);
				DiaDimItem(dia, DPAT_OUTLINE);
				orignature[curlayer] = (orignature[curlayer] & ~NATURE) | SOLIDC;
			}
			origchanged[curlayer] = TRUE;
			continue;
		}
		if (itemHit == DPAT_OUTLINE)
		{
			/* the "Outline Pattern" box */
			if (DiaGetControl(dia, DPAT_OUTLINE) == 0)
			{
				DiaSetControl(dia, DPAT_OUTLINE, 1);
				orignature[curlayer] |= OUTLINEPAT;
			} else
			{
				DiaSetControl(dia, DPAT_OUTLINE, 0);
				orignature[curlayer] &= ~OUTLINEPAT;
			}
			origchanged[curlayer] = TRUE;
			continue;
		}
		if (itemHit == DPAT_LAYER)
		{
			/* the layer popup */
			curlayer = DiaGetPopupEntry(dia, DPAT_LAYER);
			DiaSetControl(dia, DPAT_STIPPLE, 0);
			DiaUnDimItem(dia, DPAT_OUTLINE);
			DiaSetControl(dia, DPAT_OUTLINE, 0);
			if ((orignature[curlayer]&NATURE) == PATTERNED)
			{
				DiaSetControl(dia, DPAT_STIPPLE, 1);
				if ((orignature[curlayer]&OUTLINEPAT) != 0) DiaSetControl(dia, DPAT_OUTLINE, 1);
			} else DiaDimItem(dia, DPAT_OUTLINE);
			for(j=0; j<16; j++) us_origpattern[j] = origbits[curlayer*16+j] & 0xFFFF;
			for(j=0; j<25; j++) if (origcolor[curlayer] == us_colorvaluelist[j]) break;
			if (j < 25) DiaSetPopupEntry(dia, DPAT_COLOR, j);
			us_redrawpattern(&us_strokerect, dia);
			continue;
		}
		if (itemHit == DPATART_PATTERNAREA)
		{
			DiaGetMouse(dia, &x, &y);
			j = (y - us_strokerect.top) / 16;  if (j < 0) j = 0;  if (j > 15) j = 15;
			k = (x - us_strokerect.left) / 16;  if (k < 0) k = 0;  if (k > 15) k = 15;
			bits = origbits[curlayer*16+j] & 0xFFFF;
			if ((bits & (1<<(15-k))) != 0) us_strokeup = FALSE; else
				us_strokeup = TRUE;
			us_trackingdialog = dia;
			DiaTrackCursor(dia, us_patternstroke);
			for(j=0; j<16; j++) origbits[curlayer*16+j] = us_origpattern[j];
			origchanged[curlayer] = TRUE;
			continue;
		}
		if (itemHit >= DPAT_PREPATFIRST && itemHit <= DPAT_PREPATLAST)
		{
			DiaGetMouse(dia, &x, &y);
			DiaItemRect(dia, itemHit, &r);
			j = (r.top+r.bottom)/2;
			if (y < j-8 || y > j+8) continue;
			i = (itemHit-DPAT_PREPATFIRST) * 2;
			if (x > (r.left+r.right)/2) i++;
			i *= 16;
			for(j=0; j<16; j++)
			{
				bits = us_predefpats[i++] & 0xFFFF;
				origbits[curlayer*16+j] = bits;
				us_origpattern[j] = bits;
			}
			us_redrawpattern(&us_strokerect, dia);
			origchanged[curlayer] = TRUE;
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		for(i=0; i<el_curtech->layercount; i++)
		{
			if (!origchanged[i]) continue;
			(el_curtech->layers[i])->colstyle = (INTSML)orignature[i];
			(el_curtech->layers[i])->col = origcolor[i];
			if (origcolor[i] == COLORT1 || origcolor[i] == COLORT2 || origcolor[i] == COLORT3 ||
				origcolor[i] == COLORT4 || origcolor[i] == COLORT5)
					(el_curtech->layers[i])->bits = origcolor[i]; else
						(el_curtech->layers[i])->bits = LAYERO;
			for(j=0; j<16; j++) (el_curtech->layers[i])->raster[j] = (INTSML)origbits[i*16+j];

			/* save a shadow variable for this layer pattern */
			for(j=0; j<16; j++) newpat[j] = origbits[i*16+j];
			newpat[16] = orignature[i];
			newpat[17] = origcolor[i];
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("TECH_layer_pattern_"));
			addstringtoinfstr(infstr, layername(el_curtech, i));
			(void)setval((INTBIG)el_curtech, VTECHNOLOGY, returninfstr(infstr),
				(INTBIG)newpat, VINTEGER|VISARRAY|(18<<VLENGTHSH));
		}
	}
	efree((CHAR *)origbits);
	efree((CHAR *)orignature);
	efree((CHAR *)origchanged);
	DiaDoneDialog(dia);
	return(0);
}

/****************************** ATTRIBUTE DIALOG ******************************/

/* Attributes */
static DIALOGITEM us_attrdialogitems[] =
{
 /*  1 */ {0, {432,28,456,108}, BUTTON, N_("Done")},
 /*  2 */ {0, {148,216,164,332}, MESSAGE, N_("Attribute name:")},
 /*  3 */ {0, {8,8,24,300}, RADIO, N_("Cell")},
 /*  4 */ {0, {32,8,48,300}, RADIO, N_("Node")},
 /*  5 */ {0, {56,8,72,300}, RADIO, N_("Cell port")},
 /*  6 */ {0, {80,8,96,300}, RADIO, N_("Node port")},
 /*  7 */ {0, {104,8,120,300}, RADIO, N_("Arc")},
 /*  8 */ {0, {148,8,300,208}, SCROLL, x_("")},
 /*  9 */ {0, {148,336,164,504}, EDITTEXT, x_("")},
 /* 10 */ {0, {172,276,220,576}, EDITTEXT, x_("")},
 /* 11 */ {0, {320,216,336,364}, CHECK, N_("Instances inherit")},
 /* 12 */ {0, {172,216,188,272}, MESSAGE, N_("Value")},
 /* 13 */ {0, {112,492,128,576}, BUTTON, N_("Make Array")},
 /* 14 */ {0, {380,8,396,132}, BUTTON, N_("Delete Attribute")},
 /* 15 */ {0, {308,8,324,132}, BUTTON, N_("Create Attribute")},
 /* 16 */ {0, {356,8,372,132}, BUTTON, N_("Rename Attribute")},
 /* 17 */ {0, {332,8,348,132}, BUTTON, N_("Change Value")},
 /* 18 */ {0, {272,216,288,296}, MESSAGE, N_("X offset:")},
 /* 19 */ {0, {296,216,312,296}, MESSAGE, N_("Y offset:")},
 /* 20 */ {0, {272,420,304,452}, ICON, (CHAR *)us_icon200},
 /* 21 */ {0, {304,420,336,452}, ICON, (CHAR *)us_icon201},
 /* 22 */ {0, {336,420,368,452}, ICON, (CHAR *)us_icon202},
 /* 23 */ {0, {368,420,400,452}, ICON, (CHAR *)us_icon203},
 /* 24 */ {0, {400,420,432,452}, ICON, (CHAR *)us_icon205},
 /* 25 */ {0, {272,456,288,568}, RADIO, N_("Center")},
 /* 26 */ {0, {288,456,304,568}, RADIO, N_("Bottom")},
 /* 27 */ {0, {304,456,320,568}, RADIO, N_("Top")},
 /* 28 */ {0, {320,456,336,568}, RADIO, N_("Right")},
 /* 29 */ {0, {336,456,352,568}, RADIO, N_("Left")},
 /* 30 */ {0, {352,456,368,568}, RADIO, N_("Lower right")},
 /* 31 */ {0, {368,456,384,568}, RADIO, N_("Lower left")},
 /* 32 */ {0, {384,456,400,568}, RADIO, N_("Upper right")},
 /* 33 */ {0, {400,456,416,568}, RADIO, N_("Upper left")},
 /* 34 */ {0, {8,304,128,484}, SCROLL, x_("")},
 /* 35 */ {0, {440,236,456,404}, POPUP, x_("")},
 /* 36 */ {0, {12,492,28,564}, MESSAGE, N_("Array:")},
 /* 37 */ {0, {32,492,48,576}, BUTTON, N_("Add")},
 /* 38 */ {0, {52,492,68,576}, BUTTON, N_("Remove")},
 /* 39 */ {0, {72,492,88,576}, BUTTON, N_("Add All")},
 /* 40 */ {0, {92,492,108,576}, BUTTON, N_("Remove All")},
 /* 41 */ {0, {248,216,264,272}, MESSAGE, N_("Show:")},
 /* 42 */ {0, {248,276,264,532}, POPUP, x_("")},
 /* 43 */ {0, {224,324,240,576}, MESSAGE, x_("")},
 /* 44 */ {0, {224,216,240,324}, POPUP, x_("")},
 /* 45 */ {0, {136,8,137,576}, DIVIDELINE, x_("")},
 /* 46 */ {0, {272,300,288,380}, EDITTEXT, x_("")},
 /* 47 */ {0, {296,300,312,380}, EDITTEXT, x_("")},
 /* 48 */ {0, {368,248,384,408}, RADIO, N_("Points (max 63)")},
 /* 49 */ {0, {368,196,384,240}, EDITTEXT, x_("")},
 /* 50 */ {0, {392,248,408,408}, RADIO, N_("Lambda (max 127.75)")},
 /* 51 */ {0, {392,196,408,240}, EDITTEXT, x_("")},
 /* 52 */ {0, {416,144,432,228}, MESSAGE, N_("Text font:")},
 /* 53 */ {0, {416,236,432,404}, POPUP, x_("")},
 /* 54 */ {0, {424,456,440,528}, CHECK, N_("Italic")},
 /* 55 */ {0, {444,456,460,520}, CHECK, N_("Bold")},
 /* 56 */ {0, {464,456,480,532}, CHECK, N_("Underline")},
 /* 57 */ {0, {380,140,396,188}, MESSAGE, N_("Size:")},
 /* 58 */ {0, {440,144,456,228}, MESSAGE, N_("Rotation:")},
 /* 59 */ {0, {344,216,360,364}, CHECK, N_("Is Parameter")},
 /* 60 */ {0, {464,236,480,404}, POPUP, x_("")},
 /* 61 */ {0, {464,144,480,228}, MESSAGE, N_("Units:")}
};
static DIALOG us_attrdialog = {{75,75,564,660}, N_("Attributes"), 0, 61, us_attrdialogitems, 0, 0};

static VARIABLE *us_attrfindvarname(INTBIG curcontrol, INTBIG numvar, VARIABLE *firstvar, void *dia);
static void      us_attrgetfactors(INTBIG curcontrol, INTBIG addr, INTBIG *numvar, VARIABLE **firstvar);
static void      us_attrloadchoices(INTBIG numvar, VARIABLE *firstvar, INTBIG curcontrol, void *dia);
static CHAR     *us_attrmakevarname(INTBIG curcontrol, PORTPROTO *pp, CHAR *name);
static CHAR     *us_attrgenportvalue(INTBIG curcontrol, INTBIG addr, INTBIG type, CHAR *attrname, NODEPROTO *np, CHAR *oldvalue);
static void      us_attrsetportposition(VARIABLE *var, PORTPROTO *pp);
static void      us_attrloadportlist(INTBIG curcontrol, void *dia);
static void      us_attrselectattributeline(INTBIG curcontrol, INTBIG which, INTBIG numvar, VARIABLE *firstvar, void *dia);
static void      us_attrtextposition(INTBIG curcontrol, UINTBIG *olddescript, UINTBIG *newdescript, void *dia);

/* special items for the "attributes" dialog: */
#define DATR_CELLATTRS       3		/* Current cell (radio) */
#define DATR_NODEATTRS       4		/* Current node inst (radio) */
#define DATR_EXPORTATTRS     5		/* Current portproto (radio) */
#define DATR_PORTATTRS       6		/* Current portinst (radio) */
#define DATR_ARCATTRS        7		/* Current network (radio) */
#define DATR_ATTRLIST        8		/* Attribute list (scroll) */
#define DATR_ATTRNAME        9		/* Attribute name (edit text) */
#define DATR_ATTRVALUE      10		/* Attribute value (edit text) */
#define DATR_INHERIT        11		/* Instances inherit (check) */
#define DATR_ARRAYPORT      13		/* Array Port attribute (button) */
#define DATR_DELETEATTR     14		/* Delete attribute (button) */
#define DATR_CREATEATTR     15		/* Create attribute (button) */
#define DATR_RENAMEATTR     16		/* Rename attribute (button) */
#define DATR_CHANGEATTR     17		/* Change attribute value (button) */
#define DATR_FIRSTICON      20		/* Icon: cent/bot (icon) */
#define DATR_LASTICON       24		/* Icon: upper-left (icon) */
#define DATR_FIRSTNAMEBUT   25		/* First attribute name button (radio) */
#define DATR_NAMECENT       25		/* Attribute: centered (radio) */
#define DATR_NAMEBOT        26		/* Attribute: bottom (radio) */
#define DATR_NAMETOP        27		/* Attribute: top (radio) */
#define DATR_NAMERIGHT      28		/* Attribute: right (radio) */
#define DATR_NAMELEFT       29		/* Attribute: left (radio) */
#define DATR_NAMELOWRIGHT   30		/* Attribute: lower-right (radio) */
#define DATR_NAMELOWLEFT    31		/* Attribute: lower-left (radio) */
#define DATR_NAMEUPRIGHT    32		/* Attribute: upper-right (radio) */
#define DATR_NAMEUPLEFT     33		/* Attribute: upper-left (radio) */
#define DATR_LASTNAMEBUT    33		/* Last attribute name button (radio) */
#define DATR_PORTLIST       34		/* Port list (scroll) */
#define DATR_ROTATION       35		/* Text rotation (popup) */
#define DATR_ARRAY_L        36		/* Array label (stat text) */
#define DATR_ADDPORT        37		/* Add port to array (button) */
#define DATR_REMOVEPORT     38		/* Remove port from array (button) */
#define DATR_ADDALLPORTS    39		/* Add all ports to array (button) */
#define DATR_REMOVEALLPORTS 40		/* Remove all ports from array (button) */
#define DATR_WHATTOSHOW     42		/* What to show (popup) */
#define DATR_EVALUATION     43		/* Evaluation (stat text) */
#define DATR_LANGUAGE       44		/* Language (popup) */
#define DATR_XOFFSET        46		/* Attribute X offset (edit text) */
#define DATR_YOFFSET        47		/* Attribute Y offset (edit text) */
#define DATR_ABSTEXTSIZE_L  48		/* Absolute text size label (radio) */
#define DATR_ABSTEXTSIZE    49		/* Absolute text size (edit text) */
#define DATR_RELTEXTSIZE_L  50		/* Relative text size label (radio) */
#define DATR_RELTEXTSIZE    51		/* Relative text size (edit text) */
#define DATR_TEXTFACE_L     52		/* Text font label (stat text) */
#define DATR_TEXTFACE       53		/* Text font (popup) */
#define DATR_TEXTITALIC     54		/* Text italic (check) */
#define DATR_TEXTBOLD       55		/* Text bold (check) */
#define DATR_TEXTUNDERLINE  56		/* Text underline (check) */
#define DATR_ISPARAMETER    59		/* Is parameter (check) */
#define DATR_UNITS          60		/* Text units (popup) */
static NODEPROTO *us_attrnodeprotoaddr;
static NODEINST  *us_attrnodeinstaddr;
static ARCINST   *us_attrarcinstaddr;
static PORTPROTO *us_attrportprotoaddr, *us_attrexportprotoaddr;

INTBIG us_attributesdlog(void)
{
	REGISTER INTBIG itemHit, i, which, value, curcontrol, addr, type,
		tempaddr, temptype, listlen;
	INTBIG numvar, newval, newtype, newaddr;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	INTBIG x, y;
	RECTAREA r;
	REGISTER NODEPROTO *proto;
	REGISTER PORTPROTO *pp;
	REGISTER PORTEXPINST *pe;
	REGISTER VARIABLE *var, *newvar;
	VARIABLE *firstvar;
	CHAR *oldvalue, *newvarname, *attrname, *newlang[16], line[50];
	REGISTER CHAR *pt, *newvalue, **languages;
	HIGHLIGHT high;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the attributes dialog box */
	us_attrnodeprotoaddr = us_needcell();
	if (us_attrnodeprotoaddr == NONODEPROTO) return(0);
	dia = DiaInitDialog(&us_attrdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DATR_ATTRLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	DiaInitTextDialog(dia, DATR_PORTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	languages = us_languagechoices();
	DiaSetPopup(dia, DATR_LANGUAGE, 4, languages);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_rotationtypes[i]);
	DiaSetPopup(dia, DATR_ROTATION, 4, newlang);
	for(i=0; i<8; i++) newlang[i] = TRANSLATE(us_unitnames[i]);
	DiaSetPopup(dia, DATR_UNITS, 8, newlang);
	if (graphicshas(CANCHOOSEFACES))
	{
		DiaUnDimItem(dia, DATR_TEXTFACE_L);
		us_setpopupface(DATR_TEXTFACE, 0, TRUE, dia);
	} else
	{
		DiaDimItem(dia, DATR_TEXTFACE_L);
	}
	if (graphicshas(CANMODIFYFONTS))
	{
		DiaUnDimItem(dia, DATR_TEXTITALIC);
		DiaUnDimItem(dia, DATR_TEXTBOLD);
		DiaUnDimItem(dia, DATR_TEXTUNDERLINE);
	} else
	{
		DiaDimItem(dia, DATR_TEXTITALIC);
		DiaDimItem(dia, DATR_TEXTBOLD);
		DiaDimItem(dia, DATR_TEXTUNDERLINE);
	}
	DiaSetControl(dia, DATR_NAMECENT, 1);
	DiaSetControl(dia, DATR_RELTEXTSIZE_L, 1);
	DiaSetText(dia, DATR_RELTEXTSIZE, x_("1"));

	/* determine what attributes can be set */
	us_attrnodeinstaddr = NONODEINST;
	us_attrexportprotoaddr = NOPORTPROTO;
	us_attrportprotoaddr = NOPORTPROTO;
	us_attrarcinstaddr = NOARCINST;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		if (getlength(var) == 1)
		{
			if (!us_makehighlight(((CHAR **)var->addr)[0], &high))
			{
				if ((high.status&HIGHTYPE) == HIGHFROM)
				{
					if (!high.fromgeom->entryisnode)
					{
						us_attrarcinstaddr = high.fromgeom->entryaddr.ai;
					} else
					{
						us_attrnodeinstaddr = high.fromgeom->entryaddr.ni;
						if (high.fromport != NOPORTPROTO)
						{
							for(pe = us_attrnodeinstaddr->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
								if (pe->proto == high.fromport) break;
							if (pe != NOPORTEXPINST) us_attrexportprotoaddr = high.fromport;
							if (us_attrnodeinstaddr->proto->primindex == 0) us_attrportprotoaddr = high.fromport;
						}
					}
				} else if ((high.status&HIGHTYPE) == HIGHTEXT)
				{
					if (high.fromvar != NOVARIABLE)
					{
						if (high.fromgeom != NOGEOM)
						{
							if (high.fromgeom->entryisnode)
							{
								/* node variable */
								us_attrnodeinstaddr = high.fromgeom->entryaddr.ni;
								if (high.fromport != NOPORTPROTO)
									us_attrexportprotoaddr = high.fromport;
							} else
							{
								/* arc variable */
								us_attrarcinstaddr = high.fromgeom->entryaddr.ai;
							}
						}
					} else if (high.fromport != NOPORTPROTO)
					{
						us_attrexportprotoaddr = high.fromport;
					}
				}
			}
		}
	}

	/* load the button names and determine initial control */
	infstr = initinfstr();
	formatinfstr(infstr, _("Cell (%s)"), describenodeproto(us_attrnodeprotoaddr));
	DiaSetText(dia, DATR_CELLATTRS, returninfstr(infstr));
	curcontrol = DATR_CELLATTRS;
	if (us_attrexportprotoaddr == NOPORTPROTO)
		us_attrexportprotoaddr = us_attrnodeprotoaddr->firstportproto; else
			curcontrol = DATR_EXPORTATTRS;
	if (us_attrexportprotoaddr != NOPORTPROTO)
	{
		DiaUnDimItem(dia, DATR_EXPORTATTRS);
		infstr = initinfstr();
		formatinfstr(infstr, _("Cell Export (%s)"), us_attrexportprotoaddr->protoname);
		DiaSetText(dia, DATR_EXPORTATTRS, returninfstr(infstr));
	} else DiaDimItem(dia, DATR_EXPORTATTRS);
	if (us_attrnodeinstaddr != NONODEINST)
	{
		DiaUnDimItem(dia, DATR_NODEATTRS);
		infstr = initinfstr();
		formatinfstr(infstr, _("Node (%s)"), describenodeinst(us_attrnodeinstaddr));
		DiaSetText(dia, DATR_NODEATTRS, returninfstr(infstr));
		curcontrol = DATR_NODEATTRS;
	} else DiaDimItem(dia, DATR_NODEATTRS);
	if (us_attrportprotoaddr != NOPORTPROTO)
	{
		DiaUnDimItem(dia, DATR_PORTATTRS);
		infstr = initinfstr();
		formatinfstr(infstr, _("Node Port (%s)"), us_attrportprotoaddr->protoname);
		DiaSetText(dia, DATR_PORTATTRS, returninfstr(infstr));
		curcontrol = DATR_PORTATTRS;
	} else DiaDimItem(dia, DATR_PORTATTRS);
	if (us_attrarcinstaddr != NOARCINST)
	{
		DiaUnDimItem(dia, DATR_ARCATTRS);
		infstr = initinfstr();
		formatinfstr(infstr, _("Network (%s)"), describearcinst(us_attrarcinstaddr));
		DiaSetText(dia, DATR_ARCATTRS, returninfstr(infstr));
		curcontrol = DATR_ARCATTRS;
	} else DiaDimItem(dia, DATR_ARCATTRS);

	DiaSetControl(dia, curcontrol, 1);
	switch (curcontrol)
	{
		case DATR_CELLATTRS:   addr = (INTBIG)us_attrnodeprotoaddr;    type = VNODEPROTO;   break;
		case DATR_NODEATTRS:   addr = (INTBIG)us_attrnodeinstaddr;     type = VNODEINST;    break;
		case DATR_EXPORTATTRS: addr = (INTBIG)us_attrexportprotoaddr;  type = VPORTPROTO;   break;
		case DATR_PORTATTRS:   addr = (INTBIG)us_attrnodeinstaddr;     type = VNODEINST;    break;
		case DATR_ARCATTRS:    addr = (INTBIG)us_attrarcinstaddr;      type = VARCINST;     break;
		default:               addr = 0;                               type = VUNKNOWN;     break;
	}
	if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_EXPORTATTRS)
		DiaUnDimItem(dia, DATR_INHERIT); else
	{
		DiaSetControl(dia, DATR_INHERIT, 0);
		DiaDimItem(dia, DATR_INHERIT);
	}
	if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_NODEATTRS)
		DiaUnDimItem(dia, DATR_ISPARAMETER); else
	{
		DiaSetControl(dia, DATR_ISPARAMETER, 0);
		DiaDimItem(dia, DATR_ISPARAMETER);
	}
	us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
	us_attrloadchoices(numvar, firstvar, curcontrol, dia);
	us_attrloadportlist(curcontrol, dia);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit >= DATR_CELLATTRS && itemHit <= DATR_ARCATTRS)
		{
			/* selected different object */
			for(i=DATR_CELLATTRS; i<=DATR_ARCATTRS; i++) DiaSetControl(dia, i, 0);
			curcontrol = itemHit;
			DiaSetControl(dia, curcontrol, 1);
			switch (curcontrol)
			{
				case DATR_CELLATTRS:   addr = (INTBIG)us_attrnodeprotoaddr;    type = VNODEPROTO;   break;
				case DATR_NODEATTRS:   addr = (INTBIG)us_attrnodeinstaddr;     type = VNODEINST;    break;
				case DATR_EXPORTATTRS: addr = (INTBIG)us_attrexportprotoaddr;  type = VPORTPROTO;   break;
				case DATR_PORTATTRS:   addr = (INTBIG)us_attrnodeinstaddr;     type = VNODEINST;    break;
				case DATR_ARCATTRS:    addr = (INTBIG)us_attrarcinstaddr;      type = VARCINST;     break;
				default:               addr = 0;                               type = VUNKNOWN;     break;
			}
			DiaSetText(dia, DATR_ATTRNAME, x_(""));
			if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_EXPORTATTRS)
				DiaUnDimItem(dia, DATR_INHERIT); else
			{
				DiaSetControl(dia, DATR_INHERIT, 0);
				DiaDimItem(dia, DATR_INHERIT);
			}
			if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_NODEATTRS)
				DiaUnDimItem(dia, DATR_ISPARAMETER); else
			{
				DiaSetControl(dia, DATR_ISPARAMETER, 0);
				DiaDimItem(dia, DATR_ISPARAMETER);
			}
			us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
			us_attrloadchoices(numvar, firstvar, curcontrol, dia);
			us_attrloadportlist(curcontrol, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_ATTRLIST)
		{
			/* selected attribute name in scroll area */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			us_attrselectattributeline(curcontrol, i, numvar, firstvar, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_PORTLIST)
		{
			/* selected port name */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) attrname = 0; else
				(void)allocstring(&attrname, DiaGetScrollLine(dia, DATR_ATTRLIST, i), el_tempcluster);
			which = DiaGetCurLine(dia, DATR_PORTLIST);
			pt = DiaGetScrollLine(dia, DATR_PORTLIST, which);
			if (*pt == '>') DiaDefaultButton(dia, DATR_REMOVEPORT); else
				DiaDefaultButton(dia, DATR_ADDPORT);
			if (curcontrol == DATR_EXPORTATTRS)
			{
				for(i=0, pp = us_attrnodeprotoaddr->firstportproto; pp != NOPORTPROTO;
					pp = pp->nextportproto, i++)
						if (which == i) break;
				if (pp == NOPORTPROTO) continue;
				us_attrexportprotoaddr = pp;
				infstr = initinfstr();
				formatinfstr(infstr, _("Cell Export (%s)"), us_attrexportprotoaddr->protoname);
				DiaSetText(dia, DATR_EXPORTATTRS, returninfstr(infstr));
				addr = (INTBIG)us_attrexportprotoaddr;
			} else
			{
				for(i=0, pp = us_attrnodeinstaddr->proto->firstportproto; pp != NOPORTPROTO;
					pp = pp->nextportproto, i++)
						if (which == i) break;
				if (pp == NOPORTPROTO) continue;
				us_attrportprotoaddr = pp;
				infstr = initinfstr();
				formatinfstr(infstr, _("Node Port (%s)"), us_attrportprotoaddr->protoname);
				DiaSetText(dia, DATR_PORTATTRS, returninfstr(infstr));
			}
			us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
			us_attrloadchoices(numvar, firstvar, curcontrol, dia);

			/* see if former attribute name is in the new list */
			if (attrname != 0)
			{
				listlen = DiaGetNumScrollLines(dia, DATR_ATTRLIST);
				for(i=0; i<listlen; i++)
				{
					pt = DiaGetScrollLine(dia, DATR_ATTRLIST, i);
					if (namesame(pt, attrname) == 0)
					{
						DiaSelectLine(dia, DATR_ATTRLIST, i);
						us_attrselectattributeline(curcontrol, i, numvar, firstvar, dia);
						break;
					}
				}
				efree(attrname);
			}
			continue;
		}
		if (itemHit >= DATR_FIRSTICON && itemHit <= DATR_LASTICON)
		{
			/* icon clicks: convert to buttons */
			DiaGetMouse(dia, &x, &y);
			DiaItemRect(dia, itemHit, &r);
			i = (itemHit-DATR_FIRSTICON) * 2;
			if (y > (r.top+r.bottom)/2) i++;
			itemHit = i + DATR_FIRSTNAMEBUT;
		}
		if (itemHit >= DATR_FIRSTNAMEBUT && itemHit <= DATR_LASTNAMEBUT)
		{
			/* change text position */
			for(i=DATR_FIRSTNAMEBUT; i<=DATR_LASTNAMEBUT; i++) DiaSetControl(dia, i, 0);
			DiaSetControl(dia, itemHit, 1);
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_XOFFSET || itemHit == DATR_YOFFSET)
		{
			/* change text offset */
			if (curcontrol == DATR_PORTATTRS) continue;
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
		}
		if (itemHit == DATR_RELTEXTSIZE_L || itemHit == DATR_ABSTEXTSIZE_L)
		{
			/* change text size */
			DiaSetControl(dia, DATR_RELTEXTSIZE_L, 0);
			DiaSetControl(dia, DATR_ABSTEXTSIZE_L, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DATR_RELTEXTSIZE_L)
			{
				DiaUnDimItem(dia, DATR_RELTEXTSIZE);
				DiaDimItem(dia, DATR_ABSTEXTSIZE);
				itemHit = DATR_RELTEXTSIZE;
			} else
			{
				DiaUnDimItem(dia, DATR_ABSTEXTSIZE);
				DiaDimItem(dia, DATR_RELTEXTSIZE);
				itemHit = DATR_ABSTEXTSIZE;
			}
		}
		if (itemHit == DATR_RELTEXTSIZE || itemHit == DATR_ABSTEXTSIZE)
		{
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_TEXTITALIC || itemHit == DATR_TEXTBOLD ||
			itemHit == DATR_TEXTUNDERLINE)
		{
			/* change text style */
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_ROTATION)
		{
			/* change text rotation */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_UNITS)
		{
			/* change units */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_TEXTFACE)
		{
			/* change text face */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			us_attrtextposition(curcontrol, var->textdescript, descript, dia);
			modifydescript(addr, type, var, descript);
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_WHATTOSHOW)
		{
			/* selected "what to show" */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			us_pushhighlight();
			us_clearhighlightcount();
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			if (DiaGetPopupEntry(dia, DATR_WHATTOSHOW) == 0) var->type &= ~VDISPLAY; else
			{
				var->type |= VDISPLAY;
				us_attrtextposition(curcontrol, var->textdescript, descript, dia);
				modifydescript(addr, type, var, descript);
			}
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_pophighlight(TRUE);
			DiaDefaultButton(dia, OK);
			us_endbatch();
			continue;
		}
		if (itemHit == DATR_LANGUAGE)
		{
			/* selected evaluation option */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			switch (DiaGetPopupEntry(dia, DATR_LANGUAGE))
			{
				case 0: var->type = (var->type & ~(VCODE1|VCODE2)) | 0;       break;
				case 1: var->type = (var->type & ~(VCODE1|VCODE2)) | VTCL;    break;
				case 2: var->type = (var->type & ~(VCODE1|VCODE2)) | VLISP;   break;
				case 3: var->type = (var->type & ~(VCODE1|VCODE2)) | VJAVA;   break;
			}
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			us_attrselectattributeline(curcontrol, i, numvar, firstvar, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_INHERIT)
		{
			/* checked "instances inherit" */
			value = 1 - DiaGetControl(dia, DATR_INHERIT);
			DiaSetControl(dia, DATR_INHERIT, value);
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			TDCOPY(descript, var->textdescript);
			if (value == 0) TDSETINHERIT(descript, 0); else
				TDSETINHERIT(descript, VTINHERIT);
			startobjectchange(addr, type);
			modifydescript(addr, type, var, descript);
			endobjectchange(addr, type);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_ISPARAMETER)
		{
			/* checked "is parameter" */
			value = 1 - DiaGetControl(dia, DATR_ISPARAMETER);
			DiaSetControl(dia, DATR_ISPARAMETER, value);
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0) continue;
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			TDCOPY(descript, var->textdescript);
			if (value == 0) TDSETISPARAM(descript, 0); else
				TDSETISPARAM(descript, VTISPARAMETER);
			startobjectchange(addr, type);
			modifydescript(addr, type, var, descript);
			endobjectchange(addr, type);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_CHANGEATTR)
		{
			/* change attribute value */
			i = DiaGetCurLine(dia, DATR_ATTRLIST);
			if (i < 0) continue;
			if (namesame(DiaGetScrollLine(dia, DATR_ATTRLIST, i), DiaGetText(dia, DATR_ATTRNAME)) != 0)
			{
				ttybeep(SOUNDBEEP, TRUE);
				continue;
			}
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			pt = DiaGetText(dia, DATR_ATTRVALUE);
			TDCOPY(descript, var->textdescript);
			if ((var->type&(VCODE1|VCODE2)) != 0)
			{
				newtype = var->type;
				newaddr = (INTBIG)pt;
			} else
			{
				getsimpletype(pt, &newtype, &newaddr, 0);
				newtype |= var->type&VDISPLAY;
			}
			newvar = setvalkey(addr, type, var->key, newaddr, newtype);
			if (newvar != NOVARIABLE)
			{
				TDCOPY(newvar->textdescript, descript);
				if (curcontrol == DATR_CELLATTRS)
					us_drawcellvariable(newvar, us_attrnodeprotoaddr);
			}
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			us_attrselectattributeline(curcontrol, i, numvar, firstvar, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_RENAMEATTR)
		{
			/* rename attribute */
			pt = DiaGetText(dia, DATR_ATTRNAME);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0)
			{
				DiaMessageInDialog(_("No attribute name given"));
				continue;
			}
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			(void)allocstring(&newvarname, us_attrmakevarname(curcontrol, us_attrportprotoaddr, pt), el_tempcluster);
			pt = DiaGetText(dia, DATR_ATTRVALUE);
			newtype = var->type;
			TDCOPY(descript, var->textdescript);
			startobjectchange(addr, type);
			if (curcontrol == DATR_CELLATTRS) us_undrawcellvariable(var, us_attrnodeprotoaddr);
			(void)delvalkey(addr, type, var->key);
			if ((newtype&VTYPE) == VSTRING || (newtype&(VCODE1|VCODE2)) != 0)
			{
				newvar = setval(addr, type, newvarname, (INTBIG)pt, newtype);
			} else switch (newtype&VTYPE)
			{
				case VINTEGER:
					newvar = setval(addr, type, newvarname, eatoi(pt), newtype);
					break;
				case VFLOAT:
					newvar = setval(addr, type, newvarname, castint((float)eatof(pt)),
						newtype);
					break;
				default:
					newvar = NOVARIABLE;
					break;
			}
			efree(newvarname);
			if (newvar != NOVARIABLE)
			{
				TDCOPY(newvar->textdescript, descript);
				if (curcontrol == DATR_CELLATTRS)
					us_drawcellvariable(newvar, us_attrnodeprotoaddr);
			}
			endobjectchange(addr, type);
			us_endbatch();
			us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
			us_attrloadchoices(numvar, firstvar, curcontrol, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_DELETEATTR)
		{
			/* delete attribute */
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			if (curcontrol != DATR_EXPORTATTRS) startobjectchange(addr, type); else
				startobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			if (curcontrol == DATR_CELLATTRS) us_drawcellvariable(var, us_attrnodeprotoaddr);
			(void)delvalkey(addr, type, var->key);
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
			us_attrloadchoices(numvar, firstvar, curcontrol, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_CREATEATTR)
		{
			/* new attribute */
			pt = DiaGetText(dia, DATR_ATTRNAME);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0)
			{
				DiaMessageInDialog(_("No attribute name given"));
				continue;
			}
			(void)allocstring(&newvarname, us_attrmakevarname(curcontrol, us_attrportprotoaddr, pt), el_tempcluster);
			pt = DiaGetText(dia, DATR_ATTRVALUE);
			getsimpletype(pt, &newtype, &newval, 0);
			if (DiaGetPopupEntry(dia, DATR_WHATTOSHOW) != 0) newtype |= VDISPLAY;
			switch (DiaGetPopupEntry(dia, DATR_LANGUAGE))
			{
				case 1: newtype |= VTCL;    break;
				case 2: newtype |= VLISP;   break;
				case 3: newtype |= VJAVA;   break;
			}
			startobjectchange(addr, type);
			newvar = setval(addr, type, newvarname, newval, newtype);
			efree(newvarname);
			if (newvar != NOVARIABLE)
			{
				if (curcontrol == DATR_PORTATTRS)
				{
					/* add in offset to the port */
					us_attrsetportposition(newvar, us_attrportprotoaddr);
				}
				us_attrtextposition(curcontrol, newvar->textdescript, newvar->textdescript, dia);
				if (curcontrol == DATR_CELLATTRS)
					us_drawcellvariable(newvar, us_attrnodeprotoaddr);
			}
			if (curcontrol != DATR_EXPORTATTRS) endobjectchange(addr, type); else
				endobjectchange((INTBIG)us_attrexportprotoaddr->subnodeinst, VNODEINST);				
			us_endbatch();
			us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
			us_attrloadchoices(numvar, firstvar, curcontrol, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_ADDPORT)
		{
			/* Add port to array */
			i = DiaGetCurLine(dia, DATR_PORTLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DATR_PORTLIST, i);
			if (pt[0] == '>' && pt[1] == ' ') continue;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("> "));
			addstringtoinfstr(infstr, pt);
			DiaSetScrollLine(dia, DATR_PORTLIST, i, returninfstr(infstr));
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_REMOVEPORT)
		{
			/* Remove port from array */
			i = DiaGetCurLine(dia, DATR_PORTLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DATR_PORTLIST, i);
			if (pt[0] != '>' || pt[1] != ' ') continue;
			DiaSetScrollLine(dia, DATR_PORTLIST, i, &pt[2]);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_ADDALLPORTS)
		{
			/* Add all ports to array */
			which = DiaGetCurLine(dia, DATR_PORTLIST);
			listlen = DiaGetNumScrollLines(dia, DATR_PORTLIST);
			for(i=0; i<listlen; i++)
			{
				pt = DiaGetScrollLine(dia, DATR_PORTLIST, i);
				if (pt[0] == '>' && pt[1] == ' ') continue;
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("> "));
				addstringtoinfstr(infstr, pt);
				DiaSetScrollLine(dia, DATR_PORTLIST, i, returninfstr(infstr));
			}
			DiaSelectLine(dia, DATR_PORTLIST, which);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_REMOVEALLPORTS)
		{
			/* Remove all ports from array */
			which = DiaGetCurLine(dia, DATR_PORTLIST);
			listlen = DiaGetNumScrollLines(dia, DATR_PORTLIST);
			for(i=0; i<listlen; i++)
			{
				pt = DiaGetScrollLine(dia, DATR_PORTLIST, i);
				if (pt[0] != '>' || pt[1] != ' ') continue;
				DiaSetScrollLine(dia, DATR_PORTLIST, i, &pt[2]);
			}
			DiaSelectLine(dia, DATR_PORTLIST, which);
			DiaDefaultButton(dia, OK);
			continue;
		}
		if (itemHit == DATR_ARRAYPORT)
		{
			/* "Make Array" button: array port or export attribute */
			var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
			if (var == NOVARIABLE) continue;
			(void)allocstring(&attrname, DiaGetScrollLine(dia, DATR_ATTRLIST, DiaGetCurLine(dia, DATR_ATTRLIST)),
				el_tempcluster);
			switch (var->type&VTYPE)
			{
				case VINTEGER:
					esnprintf(line, 50, x_("%ld"), var->addr);
					(void)allocstring(&oldvalue, line, el_tempcluster);
					break;
				case VFLOAT:
					esnprintf(line, 50, x_("%g"), castfloat(var->addr));
					(void)allocstring(&oldvalue, line, el_tempcluster);
					break;
				case VSTRING:
					(void)allocstring(&oldvalue, (CHAR *)var->addr, el_tempcluster);
					break;
			}
			newtype = var->type;
			if (curcontrol == DATR_EXPORTATTRS) proto = us_attrnodeprotoaddr; else
			{
				proto = us_attrnodeinstaddr->proto;
				startobjectchange(addr, type);
			}
			for(i=0, pp = proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, i++)
			{
				pt = DiaGetScrollLine(dia, DATR_PORTLIST, i);
				if (pt[0] != '>' || pt[1] != ' ') continue;
				(void)allocstring(&newvarname, us_attrmakevarname(curcontrol, pp, attrname),
					el_tempcluster);
				if (curcontrol == DATR_EXPORTATTRS)
				{
					tempaddr = (INTBIG)pp;    temptype = VPORTPROTO;
				} else
				{
					tempaddr = addr;   temptype = type;
				}
				newvar = getval(tempaddr, temptype, -1, newvarname);
				if (newvar == NOVARIABLE)
				{
					if (curcontrol == DATR_EXPORTATTRS)
						startobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
					newvalue = us_attrgenportvalue(curcontrol, addr, type, attrname,
						proto, oldvalue);
					switch (newtype&VTYPE)
					{
						case VINTEGER:
							newvar = setval(tempaddr, temptype, newvarname,
								eatoi(newvalue), newtype);
							break;
						case VFLOAT:
							newvar = setval(tempaddr, temptype, newvarname,
								castint((float)eatof(newvalue)), newtype);
							break;
						case VSTRING:
							newvar = setval(tempaddr, temptype, newvarname,
								(INTBIG)newvalue, newtype);
							break;
						default:
							newvar = NOVARIABLE;
							break;
					}
					if (newvar != NOVARIABLE)
					{
						if (curcontrol == DATR_PORTATTRS)
							us_attrsetportposition(newvar, pp);
						us_attrtextposition(curcontrol, newvar->textdescript, newvar->textdescript, dia);
					}
					if (curcontrol == DATR_EXPORTATTRS)
						endobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
				}
				efree(newvarname);
			}
			if (curcontrol == DATR_PORTATTRS) endobjectchange(addr, type);
			us_endbatch();
			efree(oldvalue);
			efree(attrname);
			us_attrgetfactors(curcontrol, addr, &numvar, &firstvar);
			us_attrloadchoices(numvar, firstvar, curcontrol, dia);
			DiaDefaultButton(dia, OK);
			continue;
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

/* helper routine to modify the text descriptor from the dialog controls */
void us_attrtextposition(INTBIG curcontrol, UINTBIG *olddescript, UINTBIG *newdescript, void *dia)
{
	INTBIG value, x, y;

	TDCOPY(newdescript, olddescript);
	if (DiaGetControl(dia, DATR_NAMECENT) != 0) TDSETPOS(newdescript, VTPOSCENT);
	if (DiaGetControl(dia, DATR_NAMEBOT) != 0) TDSETPOS(newdescript, VTPOSUP);
	if (DiaGetControl(dia, DATR_NAMETOP) != 0) TDSETPOS(newdescript, VTPOSDOWN);
	if (DiaGetControl(dia, DATR_NAMERIGHT) != 0) TDSETPOS(newdescript, VTPOSLEFT);
	if (DiaGetControl(dia, DATR_NAMELEFT) != 0) TDSETPOS(newdescript, VTPOSRIGHT);
	if (DiaGetControl(dia, DATR_NAMELOWRIGHT) != 0) TDSETPOS(newdescript, VTPOSUPLEFT);
	if (DiaGetControl(dia, DATR_NAMELOWLEFT) != 0) TDSETPOS(newdescript, VTPOSUPRIGHT);
	if (DiaGetControl(dia, DATR_NAMEUPRIGHT) != 0) TDSETPOS(newdescript, VTPOSDOWNLEFT);
	if (DiaGetControl(dia, DATR_NAMEUPLEFT) != 0) TDSETPOS(newdescript, VTPOSDOWNRIGHT);

	if (DiaGetControl(dia, DATR_ABSTEXTSIZE_L) != 0)
	{
		value = eatoi(DiaGetText(dia, DATR_ABSTEXTSIZE));
		if (value <= 0) value = 4;
		if (value >= TXTMAXPOINTS) value = TXTMAXPOINTS;
		TDSETSIZE(newdescript, TXTSETPOINTS(value));
	} else
	{
		value = atofr(DiaGetText(dia, DATR_RELTEXTSIZE)) * 4 / WHOLE;
		if (value <= 0) value = 4;
		if (value >= TXTMAXQLAMBDA) value = TXTMAXQLAMBDA;
		TDSETSIZE(newdescript, TXTSETQLAMBDA(value));
	}
	if (DiaGetControl(dia, DATR_TEXTITALIC) != 0)
		TDSETITALIC(newdescript, VTITALIC); else
			TDSETITALIC(newdescript, 0);
	if (DiaGetControl(dia, DATR_TEXTBOLD) != 0)
		TDSETBOLD(newdescript, VTBOLD); else
			TDSETBOLD(newdescript, 0);
	if (DiaGetControl(dia, DATR_TEXTUNDERLINE) != 0)
		TDSETUNDERLINE(newdescript, VTUNDERLINE); else
			TDSETUNDERLINE(newdescript, 0);
	if (graphicshas(CANCHOOSEFACES))
	{
		value = us_getpopupface(DATR_TEXTFACE, dia);
		TDSETFACE(newdescript, value);
	}
	value = DiaGetPopupEntry(dia, DATR_UNITS);
	TDSETUNITS(newdescript, value);
	value = DiaGetPopupEntry(dia, DATR_ROTATION);
	TDSETROTATION(newdescript, value);
	value = DiaGetPopupEntry(dia, DATR_WHATTOSHOW);
	switch (value)
	{
		case 1: TDSETDISPPART(newdescript, VTDISPLAYVALUE);          break;
		case 2: TDSETDISPPART(newdescript, VTDISPLAYNAMEVALUE);      break;
		case 3: TDSETDISPPART(newdescript, VTDISPLAYNAMEVALINH);     break;
		case 4: TDSETDISPPART(newdescript, VTDISPLAYNAMEVALINHALL);  break;
	}
	if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_EXPORTATTRS)
	{
		TDSETINHERIT(newdescript, 0);
		if (DiaGetControl(dia, DATR_INHERIT) != 0) TDSETINHERIT(newdescript, VTINHERIT);
	}
	if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_NODEATTRS)
	{
		TDSETISPARAM(newdescript, 0);
		if (DiaGetControl(dia, DATR_ISPARAMETER) != 0) TDSETISPARAM(newdescript, VTISPARAMETER);
	}
	if (curcontrol != DATR_PORTATTRS)
	{
		x = atofr(DiaGetText(dia, DATR_XOFFSET)) * 4 / WHOLE;
		y = atofr(DiaGetText(dia, DATR_YOFFSET)) * 4 / WHOLE;
		TDSETOFF(newdescript, x, y);
	}
}

void us_attrselectattributeline(INTBIG curcontrol, INTBIG which, INTBIG numvar, VARIABLE *firstvar, void *dia)
{
	REGISTER VARIABLE *var, *evar;
	REGISTER INTBIG x, y, i, lambda, height;
	UINTBIG savedescript[TEXTDESCRIPTSIZE];
	CHAR buf[30];
	REGISTER void *infstr;

	if (which < 0) return;
	var = us_attrfindvarname(curcontrol, numvar, firstvar, dia);
	if (var == NOVARIABLE) return;
	DiaSetText(dia, DATR_ATTRNAME, DiaGetScrollLine(dia, DATR_ATTRLIST, which));

	/* make sure that only the value is described */
	TDCOPY(savedescript, var->textdescript);
	TDSETDISPPART(var->textdescript, 0);

	/* find lambda */
	switch (curcontrol)
	{
		case DATR_CELLATTRS:
			lambda = el_curlib->lambda[us_attrnodeprotoaddr->tech->techindex];
			break;
		case DATR_NODEATTRS:
			lambda = figurelambda(us_attrnodeinstaddr->geom);
			break;
		case DATR_EXPORTATTRS:
			lambda = el_curlib->lambda[us_attrexportprotoaddr->parent->tech->techindex];
			break;
		case DATR_ARCATTRS:
			lambda = figurelambda(us_attrarcinstaddr->geom);
			break;
		default:
			lambda = 0;
			break;
	}

	/* get the value */
	if ((var->type&VISARRAY) != 0)
	{
		DiaSetText(dia, DATR_ATTRVALUE, describevariable(var, -1, 0));
		DiaDimItem(dia, DATR_ATTRVALUE);
	} else
	{
		DiaUnDimItem(dia, DATR_ATTRVALUE);
		if ((var->type&VTYPE) == VSTRING) DiaSetText(dia, DATR_ATTRVALUE, (CHAR *)var->addr); else
			DiaSetText(dia, DATR_ATTRVALUE, describevariable(var, -1, 0));
	}
	if ((var->type&(VCODE1|VCODE2)) == 0) DiaSetText(dia, DATR_EVALUATION, x_("")); else
	{
		switch (curcontrol)
		{
			case DATR_CELLATTRS:   evar = evalvar(var, (INTBIG)us_attrnodeprotoaddr, VNODEPROTO);   break;
			case DATR_NODEATTRS:   evar = evalvar(var, (INTBIG)us_attrnodeinstaddr, VNODEINST);     break;
			case DATR_EXPORTATTRS: evar = evalvar(var, (INTBIG)us_attrexportprotoaddr, VPORTPROTO); break;
			case DATR_PORTATTRS:   evar = evalvar(var, (INTBIG)us_attrnodeinstaddr, VNODEINST);     break;
			case DATR_ARCATTRS:    evar = evalvar(var, (INTBIG)us_attrarcinstaddr, VARCINST);       break;
			default: evar = NOVARIABLE;
		}
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Evaluation: "));
		if ((evar->type&VTYPE) == VSTRING) addstringtoinfstr(infstr, (CHAR *)evar->addr); else
			addstringtoinfstr(infstr, describevariable(evar, -1, 0));
		DiaSetText(dia, DATR_EVALUATION, returninfstr(infstr));
	}

	/* restore the description information */
	TDCOPY(var->textdescript, savedescript);

	if ((var->type&VDISPLAY) == 0) DiaSetPopupEntry(dia, DATR_WHATTOSHOW, 0); else
	{
		switch (TDGETDISPPART(var->textdescript))
		{
			case VTDISPLAYVALUE:         DiaSetPopupEntry(dia, DATR_WHATTOSHOW, 1);   break;
			case VTDISPLAYNAMEVALUE:     DiaSetPopupEntry(dia, DATR_WHATTOSHOW, 2);   break;
			case VTDISPLAYNAMEVALINH:    DiaSetPopupEntry(dia, DATR_WHATTOSHOW, 3);   break;
			case VTDISPLAYNAMEVALINHALL: DiaSetPopupEntry(dia, DATR_WHATTOSHOW, 4);   break;
		}
	}
	switch (var->type&(VCODE1|VCODE2))
	{
		case 0:     DiaSetPopupEntry(dia, DATR_LANGUAGE, 0);   break;
		case VTCL:  DiaSetPopupEntry(dia, DATR_LANGUAGE, 1);   break;
		case VLISP: DiaSetPopupEntry(dia, DATR_LANGUAGE, 2);   break;
		case VJAVA: DiaSetPopupEntry(dia, DATR_LANGUAGE, 3);   break;
	}
	if (TDGETINHERIT(var->textdescript) != 0) DiaSetControl(dia, DATR_INHERIT, 1); else
		DiaSetControl(dia, DATR_INHERIT, 0);
	if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_NODEATTRS)
	{
		if (TDGETISPARAM(var->textdescript) != 0) DiaSetControl(dia, DATR_ISPARAMETER, 1); else
			DiaSetControl(dia, DATR_ISPARAMETER, 0);
	}
	if (curcontrol == DATR_PORTATTRS)
	{
		DiaSetText(dia, DATR_XOFFSET, x_("0"));
		DiaSetText(dia, DATR_YOFFSET, x_("0"));
		DiaDimItem(dia, DATR_XOFFSET);
		DiaDimItem(dia, DATR_YOFFSET);
	} else
	{
		DiaUnDimItem(dia, DATR_XOFFSET);
		DiaUnDimItem(dia, DATR_YOFFSET);
		x = TDGETXOFF(var->textdescript);
		x = x * WHOLE / 4;
		y = TDGETYOFF(var->textdescript);
		y = y * WHOLE / 4;
		DiaSetText(dia, DATR_XOFFSET, frtoa(x));
		DiaSetText(dia, DATR_YOFFSET, frtoa(y));
	}
	i = TDGETSIZE(var->textdescript);
	if (TXTGETPOINTS(i) != 0)
	{
		/* show point size */
		height = TXTGETPOINTS(i);
		DiaUnDimItem(dia, DATR_ABSTEXTSIZE);
		esnprintf(buf, 30, x_("%ld"), height);
		DiaSetText(dia, DATR_ABSTEXTSIZE, buf);
		DiaSetControl(dia, DATR_ABSTEXTSIZE_L, 1);

		/* figure out how many lambda the point value is */
		if (el_curwindowpart != NOWINDOWPART)
			height = roundfloat((float)height / el_curwindowpart->scaley);
		height = height * 4 / lambda;
		DiaSetText(dia, DATR_RELTEXTSIZE, frtoa(height * WHOLE / 4));
		DiaDimItem(dia, DATR_RELTEXTSIZE);
		DiaSetControl(dia, DATR_RELTEXTSIZE_L, 0);
	} else if (TXTGETQLAMBDA(i) != 0)
	{
		/* show lambda value */
		height = TXTGETQLAMBDA(i);
		DiaUnDimItem(dia, DATR_RELTEXTSIZE);
		DiaSetText(dia, DATR_RELTEXTSIZE, frtoa(height * WHOLE / 4));
		DiaSetControl(dia, DATR_RELTEXTSIZE_L, 1);
		/* figure out how many points the lambda value is */
		height = height * lambda / 4;
		if (el_curwindowpart != NOWINDOWPART)
			height = applyyscale(el_curwindowpart, height);
		esnprintf(buf, 30, x_("%ld"), height);
		DiaSetText(dia, DATR_ABSTEXTSIZE, buf);
		DiaDimItem(dia, DATR_ABSTEXTSIZE);
		DiaSetControl(dia, DATR_ABSTEXTSIZE_L, 0);
	}
	us_setpopupface(DATR_TEXTFACE, TDGETFACE(var->textdescript), FALSE, dia);
	i = TDGETROTATION(var->textdescript);
	DiaSetPopupEntry(dia, DATR_ROTATION, i);
	i = TDGETUNITS(var->textdescript);
	DiaSetPopupEntry(dia, DATR_UNITS, i);
	if (graphicshas(CANMODIFYFONTS))
	{
		if (TDGETITALIC(var->textdescript) != 0) DiaSetControl(dia, DATR_TEXTITALIC, 1);
		if (TDGETBOLD(var->textdescript) != 0) DiaSetControl(dia, DATR_TEXTBOLD, 1);
		if (TDGETUNDERLINE(var->textdescript) != 0) DiaSetControl(dia, DATR_TEXTUNDERLINE, 1);
	}
	for(i=DATR_FIRSTNAMEBUT; i<=DATR_LASTNAMEBUT; i++) DiaSetControl(dia, i, 0);
	switch (TDGETPOS(var->textdescript))
	{
		case VTPOSCENT:      DiaSetControl(dia, DATR_NAMECENT, 1);      break;
		case VTPOSUP:        DiaSetControl(dia, DATR_NAMEBOT, 1);       break;
		case VTPOSDOWN:      DiaSetControl(dia, DATR_NAMETOP, 1);       break;
		case VTPOSLEFT:      DiaSetControl(dia, DATR_NAMERIGHT, 1);     break;
		case VTPOSRIGHT:     DiaSetControl(dia, DATR_NAMELEFT, 1);      break;
		case VTPOSUPLEFT:    DiaSetControl(dia, DATR_NAMELOWRIGHT, 1);  break;
		case VTPOSUPRIGHT:   DiaSetControl(dia, DATR_NAMELOWLEFT, 1);   break;
		case VTPOSDOWNLEFT:  DiaSetControl(dia, DATR_NAMEUPRIGHT, 1);   break;
		case VTPOSDOWNRIGHT: DiaSetControl(dia, DATR_NAMEUPLEFT, 1);    break;
	}
}

void us_attrsetportposition(VARIABLE *var, PORTPROTO *pp)
{
	INTBIG x, y;
	REGISTER INTBIG lambda;

	portposition(us_attrnodeinstaddr, pp, &x, &y);
	x -= (us_attrnodeinstaddr->lowx + us_attrnodeinstaddr->highx) / 2;
	y -= (us_attrnodeinstaddr->lowy + us_attrnodeinstaddr->highy) / 2;
	lambda = figurelambda(us_attrnodeinstaddr->geom);
	x = x * 4 / lambda;
	y = y * 4 / lambda;
	TDSETOFF(var->textdescript, x, y);
}

CHAR *us_attrgenportvalue(INTBIG curcontrol, INTBIG addr, INTBIG type, CHAR *attrname, NODEPROTO *np,
	CHAR *oldvalue)
{
	REGISTER PORTPROTO *pp;
	REGISTER CHAR *pt, *start, save, *varname;
	REGISTER VARIABLE *var;
	CHAR line[50], *testvalue, *thisvalue;
	REGISTER INTBIG value, i;
	REGISTER void *infstr;

	/* see if there are numbers here */
	for(pt = oldvalue; *pt != 0; pt++)
		if (isdigit(*pt) != 0) break;
	if (*pt == 0) return(oldvalue);

	/* get the number and the text around it */
	start = pt;
	while (isdigit(*pt) != 0) pt++;
	value = eatoi(start);

	/* loop through higher numbers looking for unused value */
	for(i = value+1; i<value+5000; i++)
	{
		infstr = initinfstr();
		save = *start;   *start = 0;
		addstringtoinfstr(infstr, oldvalue);
		*start = save;
		esnprintf(line, 50, x_("%ld"), i);
		addstringtoinfstr(infstr, line);
		addstringtoinfstr(infstr, pt);
		(void)allocstring(&testvalue, returninfstr(infstr), el_tempcluster);

		/* see if the value is used */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			varname = us_attrmakevarname(curcontrol, pp, attrname);
			if (curcontrol == DATR_PORTATTRS)
			{
				var = getval(addr, type, -1, varname);
			} else
			{
				var = getval((INTBIG)pp, VPORTPROTO, -1, varname);
			}
			if (var == NOVARIABLE) continue;
			switch (var->type&VTYPE)
			{
				case VINTEGER:
					esnprintf(line, 50, x_("%ld"), var->addr);
					thisvalue = line;
					break;
				case VFLOAT:
					esnprintf(line, 50, x_("%g"), castfloat(var->addr));
					thisvalue = line;
					break;
				case VSTRING:
					thisvalue = (CHAR *)var->addr;
					break;
			}
			if (namesame(thisvalue, testvalue) == 0) break;
		}
		if (pp == NOPORTPROTO)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, testvalue);
			efree(testvalue);
			return(returninfstr(infstr));
		} else
		{
			efree(testvalue);
		}
	}
	return(oldvalue);
}

CHAR *us_attrmakevarname(INTBIG curcontrol, PORTPROTO *pp, CHAR *name)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	if (curcontrol == DATR_PORTATTRS)
	{
		addstringtoinfstr(infstr, x_("ATTRP_"));
		addstringtoinfstr(infstr, pp->protoname);
		addstringtoinfstr(infstr, x_("_"));
	} else
	{
		addstringtoinfstr(infstr, x_("ATTR_"));
	}
	addstringtoinfstr(infstr, name);
	return(returninfstr(infstr));
}

void us_attrloadchoices(INTBIG numvar, VARIABLE *firstvar, INTBIG curcontrol, void *dia)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, protolen;
	BOOLEAN found;
	REGISTER CHAR *varname;
	static CHAR *whattodisplay[] = {N_("Nothing"), N_("Value"), N_("Name&Value"),
		N_("Name,Inherit,Value"), N_("Name,Inherit-All,Value")};
	CHAR *newlang[5];
	INTBIG maxshowoptions;

	if (curcontrol == DATR_PORTATTRS)
	{
		protolen = estrlen(us_attrportprotoaddr->protoname);
		maxshowoptions = 5;
	} else
	{
		protolen = 0;
		maxshowoptions = 3;
	}
	for(i=0; i<maxshowoptions; i++) newlang[i] = TRANSLATE(whattodisplay[i]);
	DiaSetPopup(dia, DATR_WHATTOSHOW, maxshowoptions, newlang);
	DiaLoadTextDialog(dia, DATR_ATTRLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	found = FALSE;
	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		varname = makename(var->key);
		if (curcontrol == DATR_PORTATTRS)
		{
			if (namesamen(varname, x_("ATTRP_"), 6) == 0)
			{
				if (namesamen(&varname[6], us_attrportprotoaddr->protoname, protolen) == 0)
				{
					if (varname[6+protolen] == '_')
					{
						DiaStuffLine(dia, DATR_ATTRLIST, &varname[7+protolen]);
						found = TRUE;
					}
				}
			}
		} else
		{
			if (namesamen(varname, x_("ATTR_"), 5) == 0)
			{
				DiaStuffLine(dia, DATR_ATTRLIST, &varname[5]);
				found = TRUE;
			}
		}
		if (curcontrol == DATR_CELLATTRS || curcontrol == DATR_NODEATTRS || curcontrol == DATR_ARCATTRS)
		{
			/* see if any cell, node, or arc variables are available to the user */
			if (us_bettervariablename(varname) != 0)
			{
				DiaStuffLine(dia, DATR_ATTRLIST, varname);
				found = TRUE;
				continue;
			}
		}
	}
	if (!found)
	{
		DiaSelectLine(dia, DATR_ATTRLIST, -1);
		DiaSetText(dia, DATR_XOFFSET, x_("0"));
		DiaSetText(dia, DATR_YOFFSET, x_("0"));
		DiaSetText(dia, DATR_ATTRVALUE, x_(""));
		DiaSetPopupEntry(dia, DATR_WHATTOSHOW, 2);
		DiaSetPopupEntry(dia, DATR_LANGUAGE, 0);
		DiaSetText(dia, DATR_EVALUATION, x_(""));
		DiaSetText(dia, DATR_ATTRNAME, x_(""));
	} else
	{
		DiaSelectLine(dia, DATR_ATTRLIST, 0);
		us_attrselectattributeline(curcontrol, 0, numvar, firstvar, dia);
	}
}

void us_attrloadportlist(INTBIG curcontrol, void *dia)
{
	REGISTER INTBIG i, which;
	REGISTER PORTPROTO *pp;
	REGISTER void *infstr;

	DiaLoadTextDialog(dia, DATR_PORTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	if (curcontrol == DATR_EXPORTATTRS || curcontrol == DATR_PORTATTRS)
	{
		/* port selected: show all ports */
		if (curcontrol == DATR_EXPORTATTRS)
		{
			which = -1;
			for(i=0, pp = us_attrnodeprotoaddr->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, i++)
			{
				if (pp == us_attrexportprotoaddr) which = i;
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("> "));
				addstringtoinfstr(infstr, pp->protoname);
				DiaStuffLine(dia, DATR_PORTLIST, returninfstr(infstr));
			}
			if (which >= 0) DiaSelectLine(dia, DATR_PORTLIST, which);
		} else
		{
			which = -1;
			for(i=0, pp = us_attrnodeinstaddr->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, i++)
			{
				if (pp == us_attrportprotoaddr) which = i;
				infstr = initinfstr();
				addstringtoinfstr(infstr, x_("> "));
				addstringtoinfstr(infstr, pp->protoname);
				DiaStuffLine(dia, DATR_PORTLIST, returninfstr(infstr));
			}
			if (which >= 0) DiaSelectLine(dia, DATR_PORTLIST, which);
		}
		DiaUnDimItem(dia, DATR_ARRAYPORT);
		DiaUnDimItem(dia, DATR_ARRAY_L);
		DiaUnDimItem(dia, DATR_ADDPORT);
		DiaUnDimItem(dia, DATR_REMOVEPORT);
		DiaUnDimItem(dia, DATR_ADDALLPORTS);
		DiaUnDimItem(dia, DATR_REMOVEALLPORTS);
	} else
	{
		DiaDimItem(dia, DATR_ARRAYPORT);
		DiaDimItem(dia, DATR_ARRAY_L);
		DiaDimItem(dia, DATR_ADDPORT);
		DiaDimItem(dia, DATR_REMOVEPORT);
		DiaDimItem(dia, DATR_ADDALLPORTS);
		DiaDimItem(dia, DATR_REMOVEALLPORTS);
	}
}

void us_attrgetfactors(INTBIG curcontrol, INTBIG addr, INTBIG *numvar, VARIABLE **firstvar)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;

	switch (curcontrol)
	{
		case DATR_CELLATTRS:
			np = (NODEPROTO *)addr;
			*numvar = np->numvar;
			*firstvar = np->firstvar;
			break;
		case DATR_NODEATTRS:
		case DATR_PORTATTRS:
			ni = (NODEINST *)addr;
			*numvar = ni->numvar;
			*firstvar = ni->firstvar;
			break;
		case DATR_EXPORTATTRS:
			pp = (PORTPROTO *)addr;
			*numvar = pp->numvar;
			*firstvar = pp->firstvar;
			break;
		case DATR_ARCATTRS:
			ai = (ARCINST *)addr;
			*numvar = ai->numvar;
			*firstvar = ai->firstvar;
			break;
	}
}

VARIABLE *us_attrfindvarname(INTBIG curcontrol, INTBIG numvar, VARIABLE *firstvar, void *dia)
{
	REGISTER INTBIG i, len;
	REGISTER CHAR *pt, *varname;
	REGISTER VARIABLE *var;

	i = DiaGetCurLine(dia, DATR_ATTRLIST);
	if (i < 0)
	{
		DiaMessageInDialog(_("Select an attribute name first"));
		return(NOVARIABLE);
	}
	pt = DiaGetScrollLine(dia, DATR_ATTRLIST, i);
	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		varname = makename(var->key);
		if (curcontrol == DATR_PORTATTRS)
		{
			if (namesamen(varname, x_("ATTRP_"), 6) != 0) continue;
			len = estrlen(us_attrportprotoaddr->protoname);
			if (namesamen(&varname[6], us_attrportprotoaddr->protoname, len) != 0) continue;
			if (varname[6+len] != '_') continue;
			if (namesame(&varname[7+len], pt) == 0) return(var);
		} else
		{
			if (namesamen(varname, x_("ATTR_"), 5) != 0) continue;
			if (namesame(&varname[5], pt) == 0) return(var);
		}
	}

	/* didn't find name with "ATTR" prefix, look for name directly */
	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		varname = makename(var->key);
		if (namesame(varname, pt) == 0) return(var);
	}
	return(NOVARIABLE);
}

/****************************** ATTRIBUTE ENUMERATION ******************************/

/* Attributes enumeration */
static DIALOGITEM us_manattrdialogitems[] =
{
 /*  1 */ {0, {72,224,96,304}, BUTTON, N_("OK")},
 /*  2 */ {0, {72,12,96,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {12,4,28,192}, MESSAGE, N_("Attributes name to enumerate:")},
 /*  4 */ {0, {12,196,28,316}, EDITTEXT, x_("")},
 /*  5 */ {0, {40,60,56,248}, BUTTON, N_("Check for this attribute")}
};
static DIALOG us_manattrdialog = {{75,75,180,400}, N_("Enumerate Attributes"), 0, 5, us_manattrdialogitems, 0, 0};

/* special items for the "attribute enumerate" dialog: */
#define DATE_ATTRNAME       4		/* Attribute name (edit text) */
#define DATE_CHECKATTR      5		/* Check attribute existence (button) */

static void us_findattr(CHAR *attrname, INTBIG fake);
static void us_scanattrs(NODEPROTO *np, CHAR *attrname, INTBIG collect);

#define NOENUMATTR ((ENUMATTR *)-1)

typedef struct Ienumattr
{
	CHAR             *prefix;
	INTBIG            foundtotal;
	INTBIG            foundcount;
	INTBIG           *found;
	INTBIG            foundlow;
	INTBIG            foundhigh;
	INTBIG            added;
	struct Ienumattr *nextenumattr;
} ENUMATTR;
ENUMATTR *us_firstenumattr;

INTBIG us_attrenumdlog(void)
{
	INTBIG itemHit;
	REGISTER void *dia;

	dia = DiaInitDialog(&us_manattrdialog);
	if (dia == 0) return(0);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DATE_CHECKATTR)
		{
			us_firstenumattr = NOENUMATTR;
			us_findattr(DiaGetText(dia, DATE_ATTRNAME), 1);
		}
	}
	if (itemHit == OK)
	{
		us_firstenumattr = NOENUMATTR;
		us_findattr(DiaGetText(dia, DATE_ATTRNAME), 0);
	}
	DiaDoneDialog(dia);
	return(0);
}

void us_findattr(CHAR *attrname, INTBIG fake)
{
	REGISTER NODEPROTO *np, *onp;
	CHAR *fullattrname;
	REGISTER INTBIG cellcount, i;
	REGISTER ENUMATTR *ea, *nextea;

	/* find the current cell */
	np = getcurcell();
	if (np == NONODEPROTO) return;

	fullattrname = (CHAR *)emalloc((estrlen(attrname)+6) * SIZEOFCHAR, us_tool->cluster);
	if (fullattrname == 0) return;
	estrcpy(fullattrname, x_("ATTR_"));
	estrcat(fullattrname, attrname);

	if ((np->cellview->viewstate&MULTIPAGEVIEW) != 0)
	{
		/* examine this cell and all others in the multipage view */
		cellcount = 0;
		FOR_CELLGROUP(onp, np)
		{
			us_scanattrs(onp, fullattrname, 1);
			cellcount++;
		}
		ttyputmsg(_("Examining attribute '%s' in %ld schematic pages of cell %s"),
			attrname, cellcount, np->protoname);
	} else
	{
		/* just examine this cell */
		us_scanattrs(np, fullattrname, 1);
		ttyputmsg(_("Examining attribute '%s' in cell %s"),
			attrname, describenodeproto(np));
	}

	/* analyze the results */
	for(ea = us_firstenumattr; ea != NOENUMATTR; ea = ea->nextenumattr)
	{
		ea->foundlow = ea->foundhigh = 0;
		if (ea->foundcount <= 0) continue;
		esort(ea->found, ea->foundcount, SIZEOFINTBIG, sort_intbigascending);
		ea->foundlow = ea->found[0];
		ea->foundhigh = ea->found[ea->foundcount-1];
		for(i=1; i<ea->foundcount; i++)
			if (ea->found[i-1] == ea->found[i]) break;
		if (i < ea->foundcount)
			ttyputerr(_("ERROR: attribute has the value '%s%ld' more than once"),
				ea->prefix, ea->found[i]);
	}

	if (fake != 0)
	{
		/* report what would be added */
		for(ea = us_firstenumattr; ea != NOENUMATTR; ea = ea->nextenumattr)
		{
			if (ea->added == 0) continue;
			ttyputmsg(_("Will add %ld values starting at '%s%ld'"), ea->added,
				ea->prefix, ea->foundhigh+1);
		}
	} else
	{
		for(ea = us_firstenumattr; ea != NOENUMATTR; ea = ea->nextenumattr)
			ea->added = 0;
		if ((np->cellview->viewstate&MULTIPAGEVIEW) != 0)
		{
			/* examine this cell and all others in the multipage view */
			FOR_CELLGROUP(onp, np)
				us_scanattrs(onp, fullattrname, 0);
		} else
		{
			/* just examine this cell */
			us_scanattrs(np, fullattrname, 0);
		}

		/* report what was added */
		for(ea = us_firstenumattr; ea != NOENUMATTR; ea = ea->nextenumattr)
		{
			if (ea->added == 0) continue;
			if (ea->added == 1)
			{
				ttyputmsg(_("Added value '%s%ld'"), ea->prefix, ea->foundhigh);
			} else
			{
				ttyputmsg(_("Added values '%s%ld' to '%s%ld'"), ea->prefix,
					ea->foundhigh-ea->added+1, ea->prefix, ea->foundhigh);
			}
		}
	}

	/* free memory */
	for(ea = us_firstenumattr; ea != NOENUMATTR; ea = nextea)
	{
		nextea = ea->nextenumattr;
		efree((CHAR *)ea->prefix);
		efree((CHAR *)ea);
	}

	efree(fullattrname);
}

/*
 * Routine to examine cell "np" for attributes named "attrname".
 * If "collect" is positive, collect the names in the global list.
 * If "collect" is zero, assign new values to attributes ending with "?".
 */
void us_scanattrs(NODEPROTO *np, CHAR *attrname, INTBIG collect)
{
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt, *numericpart, save, *start;
	REGISTER INTBIG *newfound, newtotal, i, index;
	REGISTER ENUMATTR *ea;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER void *infstr;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getval((INTBIG)ni, VNODEINST, VSTRING, attrname);
		if (var == NOVARIABLE) continue;
		/* code may be string but my evaluate to non-string */
		if ( (var->type & VTYPE) != VSTRING) continue;

		/* break string into text part and numeric or "?" part */
		numericpart = 0;
		start = (CHAR *)var->addr;
		for(pt = start; *pt != 0; pt++)
		{
			if (estrcmp(pt, x_("?")) == 0)
			{
				numericpart = pt;
				break;
			}
			if (isdigit(*pt) != 0)
			{
				if (numericpart == 0) numericpart = pt;
			} else numericpart = 0;
		}
		if (numericpart == 0) continue;

		/* include this one */
		save = *numericpart;
		*numericpart = 0;
		for(ea = us_firstenumattr; ea != NOENUMATTR; ea = ea->nextenumattr)
			if (namesame(ea->prefix, start) == 0) break;
		if (ea == NOENUMATTR)
		{
			if (collect == 0) continue;

			/* this prefix has not been seen yet: add it */
			ea = (ENUMATTR *)emalloc(sizeof (ENUMATTR), us_tool->cluster);
			if (ea == 0) return;
			(void)allocstring(&ea->prefix, start, us_tool->cluster);
			ea->foundtotal = ea->foundcount = ea->added = 0;
			ea->nextenumattr = us_firstenumattr;
			us_firstenumattr = ea;
		}
		*numericpart = save;
		if (*numericpart == '?')
		{
			ea->added++;
			if (collect != 0) continue;
			infstr = initinfstr();
			ea->foundhigh++;
			formatinfstr(infstr, x_("%s%ld"), ea->prefix, ea->foundhigh);
			TDCOPY(descript, var->textdescript);
			startobjectchange((INTBIG)ni, VNODEINST);
			var = setval((INTBIG)ni, VNODEINST, attrname, (INTBIG)returninfstr(infstr),
				var->type);
			if (var != NOVARIABLE)
			{
				TDCOPY(var->textdescript, descript);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
		} else
		{
			if (collect == 0) continue;
			index = myatoi(numericpart);
			if (ea->foundcount >= ea->foundtotal)
			{
				newtotal = ea->foundtotal * 2;
				if (newtotal <= ea->foundcount) newtotal = ea->foundcount + 5;
				newfound = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, us_tool->cluster);
				if (newfound == 0) return;
				for(i=0; i<ea->foundcount; i++)
					newfound[i] = ea->found[i];
				if (ea->foundtotal > 0) efree((CHAR *)ea->found);
				ea->found = newfound;
				ea->foundtotal = newtotal;
			}
			ea->found[ea->foundcount++] = index;
		}
	}
}

/****************************** ATTRIBUTE REPORT ******************************/

/* Report for Attribute */
static DIALOGITEM us_report_attrdialogitems[] =
{
 /*  1 */ {0, {12,8,28,112}, MESSAGE, N_("Attribute name:")},
 /*  2 */ {0, {12,116,28,224}, EDITTEXT, x_("")},
 /*  3 */ {0, {60,92,84,224}, DEFBUTTON, N_("Generate Report")},
 /*  4 */ {0, {36,32,52,112}, CHECK, N_("To file:")},
 /*  5 */ {0, {36,116,52,224}, EDITTEXT, x_("")},
 /*  6 */ {0, {60,8,84,76}, BUTTON, N_("Cancel")}
};
static DIALOG us_report_attrdialog = {{75,75,168,309}, N_("Attribute Report"), 0, 6, us_report_attrdialogitems, 0, 0};

/* special items for the "report for attribute" dialog: */
#define DATRR_STATE_TOFILE		1
#define DATRR_ATTRNAME			2
#define DATRR_GENERATE			3
#define DATRR_TOFILE			4
#define DATRR_FILENAME			5
#define DATRR_CANCEL			6

static void us_reportattr(NODEPROTO *np, CHAR *attr, FILE *fd);

INTBIG us_attrreportdlog(void)
{
	INTBIG itemHit;
	CHAR *rename, *truename;
	REGISTER FILE *fd;
	REGISTER void *infstr;
	REGISTER void *dia;
	static INTBIG laststate = 0;
	static CHAR lastattr[300] = {x_("")};
	static CHAR lastfile[300] = {x_("")};

	dia = DiaInitDialog(&us_report_attrdialog);
	if (dia == 0) return(0);
	if ((laststate&DATRR_STATE_TOFILE) != 0) DiaSetControl(dia, DATRR_TOFILE, 1);
	DiaSetText(dia, -DATRR_ATTRNAME, lastattr);
	DiaSetText(dia, -DATRR_FILENAME, lastfile);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DATRR_CANCEL) break;
		if (itemHit == DATRR_TOFILE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DATRR_GENERATE)
		{
			if (DiaValidEntry(dia, DATRR_ATTRNAME)==0)
			{
				ttyputmsg(_("Invalid attribute name"));
				continue;
			}
			estrcpy(lastattr, DiaGetText(dia, DATRR_ATTRNAME));
			if (DiaGetControl(dia, DATRR_TOFILE))
			{
				if (DiaValidEntry(dia, DATRR_FILENAME)==0)
				{
					ttyputmsg(_("Invalid file name"));
					continue;
				}
				estrcpy(lastfile, DiaGetText(dia, DATRR_FILENAME));
			}
				
			infstr = initinfstr();
			formatinfstr(infstr, x_("%-12s\t%-12s\t%s\t%s\n"), x_("node"), x_("name"), lastattr, x_("hierarchical path"));

			if (DiaGetControl(dia, DATRR_TOFILE))
			{
				/* if file already exists, rename it first */
				if( fileexistence(lastfile) == 1)
				{
					rename = (CHAR *)emalloc((estrlen(lastfile)+15) * SIZEOFCHAR, el_tempcluster);
					(void)estrcpy(rename, lastfile);
					(void)estrcat(rename, x_("~"));
					if (fileexistence(rename) == 1) (void)eunlink(rename);
					if (erename(lastfile, rename) != 0)
						ttyputerr(_("Could not rename file '%s' to '%s'"), lastfile, rename);
					efree(rename);
				}
				fd = xcreate(lastfile, io_filetypetlib, 0, &truename);
				xputs(returninfstr(infstr), fd);
			} else
			{
				ttyputmsg(_("%s"), returninfstr(infstr));
				fd = NULL;
			}
			begintraversehierarchy();
			us_reportattr(NONODEPROTO, lastattr, fd);
			endtraversehierarchy();
			if (fd != NULL) xclose(fd);
			break;
		}
	}
	if (itemHit == DATRR_GENERATE)
	{
		if (DiaGetControl(dia, DATRR_TOFILE) != 0) laststate |= DATRR_STATE_TOFILE; else
			laststate &= ~DATRR_STATE_TOFILE;
	}
	DiaDoneDialog(dia);
	return(0);
}

/*
 * Routine to generate attribute report 
 */
void us_reportattr(NODEPROTO *np, CHAR *attr, FILE *fd)
{
	REGISTER NODEINST *ni;
	         NODEINST **hier;
	REGISTER INTBIG attrkey, i, arraysize;
			 INTBIG *indexlist, depth;
	REGISTER NODEPROTO *cnp;			 
	REGISTER VARIABLE *var, *varn, *varnn;
	void *infstr;
	CHAR temp[50];

	/* if np == NONODEPROTO, this is top level call */
	if (np == NONODEPROTO) np = getcurcell();
	if (np == NONODEPROTO)
	{
		ttyputerr(_("No cell in current window"));
		return;
	}

	/* make key */
	infstr = initinfstr();
	formatinfstr(infstr, x_("ATTR_%s"), attr);
	attrkey = makekey(returninfstr(infstr));

	/* search instances for attr */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* recurse */
		cnp = contentsview(ni->proto);
		if (cnp == NONODEPROTO) cnp = ni->proto;
		if (cnp != np) /* ignore icon view */
		{
			arraysize = ni->arraysize;
			if (arraysize == 0) arraysize = 1;
			for(i=0; i<arraysize; i++)
			{
				downhierarchy(ni, cnp, i);
				us_reportattr(cnp, attr, fd);
				uphierarchy();
			}
		}

		/* report var, unless it is on the icon view */
		var = getvalkey((INTBIG)ni, VNODEINST, -1, attrkey);
		if (var != NOVARIABLE && cnp != np)
		{
			varn = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			infstr = initinfstr();
			CHAR *protoname = ni->proto->primindex == 0 ? ni->proto->protoname : ni->proto->protoname;
			if (varn == NOVARIABLE)
			{
				esnprintf(temp, 50, x_("noname"));
				formatinfstr(infstr, x_("%-12s\t%-12s\t"), protoname, temp);
			}
			else
				formatinfstr(infstr, x_("%-12s\t%-12s\t"), protoname, varn->addr);
			if ((var->type & VTYPE) == VFLOAT || (var->type & VTYPE) == VDOUBLE)
				formatinfstr(infstr, x_("%4.2f\t"), castfloat(var->addr));
			else if ((var->type & VTYPE) == VINTEGER)
				formatinfstr(infstr, x_("%ld\t"), var->addr);
			else if ((var->type & VTYPE) == VCHAR)
				formatinfstr(infstr, x_("%c\t"), var->addr);
			else if ((var->type & VTYPE) == VSTRING)
				formatinfstr(infstr, x_("%s\t"), var->addr);
			else
				formatinfstr(infstr, x_("INVALID_TYPE\t"));
			gettraversalpath(np, el_curwindowpart, &hier, &indexlist, &depth, 0);
			for(i=0; i<depth; i++)
			{
				varnn = getvalkey((INTBIG)(hier[i]), VNODEINST, VSTRING, el_node_name_key);
				if (varnn != NOVARIABLE)
				{
					if (indexlist[i] == 0)
						formatinfstr(infstr, x_("%s[%s]."), hier[i]->proto->protoname, varnn->addr);
					else
						formatinfstr(infstr, x_("%s[%s[%ld]]."), hier[i]->proto->protoname, varnn->addr, indexlist[i]);
				} else
				{
					if (indexlist[i] == 0)
						formatinfstr(infstr, x_("%s[]."), hier[i]->proto->protoname);
					else
						formatinfstr(infstr, x_("%s[%ld]."), hier[i]->proto->protoname, indexlist[i]);
				}
			}
			if (varn != NOVARIABLE)
				formatinfstr(infstr, x_("%s\n"), varn->addr);
			else
				formatinfstr(infstr, x_("noname\n"));
			if (fd == NULL)
				ttyputmsg(_("%s"), returninfstr(infstr));
			else
				xputs(returninfstr(infstr), fd);
		}
	}
}

/****************************** COPYRIGHT OPTIONS ******************************/

/* Copyright Options */
static DIALOGITEM us_crodialogitems[] =
{
 /*  1 */ {0, {168,272,192,352}, BUTTON, N_("OK")},
 /*  2 */ {0, {165,76,189,156}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,432}, MESSAGE, N_("Copyright information can be added to every generated deck.")},
 /*  4 */ {0, {72,4,88,268}, RADIO, N_("Use copyright message from file:")},
 /*  5 */ {0, {96,28,128,292}, EDITTEXT, x_("")},
 /*  6 */ {0, {100,300,124,372}, BUTTON, N_("Browse")},
 /*  7 */ {0, {136,28,152,432}, MESSAGE, N_("Do not put comment characters in this file.")},
 /*  8 */ {0, {48,4,64,268}, RADIO, N_("No copyright message")}
};
static DIALOG us_crodialog = {{75,75,276,517}, N_("Copyright Options"), 0, 8, us_crodialogitems, 0, 0};

/* special items for the "copyright options" dialog: */
#define DCRO_USECOPYRIGHT 4		/* use copyright notice (radio) */
#define DCRO_FILENAME     5		/* copyright file name (edit text) */
#define DCRO_BROWSE       6		/* browse for copyright file (button) */
#define DCRO_NOCOPYRIGHT  8		/* no copyright notice (radio) */

INTBIG us_copyrightdlog(void)
{
	REGISTER void *dia, *infstr;
	REGISTER CHAR *initialfile, *pt;
	CHAR *subparams[3];
	REGISTER INTBIG itemHit, i, oldplease;
	REGISTER VARIABLE *var;

	dia = DiaInitDialog(&us_crodialog);
	if (dia == 0) return(0);
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_copyright_file_key);
	if (var == NOVARIABLE)
	{
		initialfile = x_("");
		DiaSetControl(dia, DCRO_NOCOPYRIGHT, 1);
		DiaDimItem(dia, DCRO_FILENAME);
	} else
	{
		initialfile = (CHAR *)var->addr;
		DiaSetControl(dia, DCRO_USECOPYRIGHT, 1);
		DiaUnDimItem(dia, DCRO_FILENAME);
		DiaSetText(dia, DCRO_FILENAME, initialfile);
	}

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DCRO_NOCOPYRIGHT || itemHit == DCRO_USECOPYRIGHT)
		{
			DiaSetControl(dia, DCRO_NOCOPYRIGHT, 0);
			DiaSetControl(dia, DCRO_USECOPYRIGHT, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DCRO_NOCOPYRIGHT)
			{
				DiaDimItem(dia, DCRO_FILENAME);
				continue;
			}
			if (*DiaGetText(dia, DCRO_FILENAME) != 0)
			{
				DiaUnDimItem(dia, DCRO_FILENAME);
				continue;
			}
			itemHit = DCRO_BROWSE;
			/* fall into next case */
		}
		if (itemHit == DCRO_BROWSE)
		{
			/* set copyright file */
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("text/"));
			addstringtoinfstr(infstr, _("Copyright File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, subparams);
			el_pleasestop = oldplease;
			if (i != 0 && subparams[0][0] != 0)
			{
				DiaUnDimItem(dia, DCRO_FILENAME);
				DiaSetText(dia, DCRO_FILENAME, subparams[0]);
				DiaSetControl(dia, DCRO_NOCOPYRIGHT, 0);
				DiaSetControl(dia, DCRO_USECOPYRIGHT, 1);
			}
			continue;
		}
	}
	if (itemHit == OK)
	{
		if (DiaGetControl(dia, DCRO_NOCOPYRIGHT) != 0)
		{
			if (var != NOVARIABLE)
				delvalkey((INTBIG)us_tool, VTOOL, us_copyright_file_key);
		} else
		{
			pt = DiaGetText(dia, DCRO_FILENAME);
			if (namesame(pt, initialfile) != 0)
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_copyright_file_key, (INTBIG)pt, VSTRING);
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** CELL PARAMETERS ******************************/
		
/* Cell Parameters */
static DIALOGITEM us_paramdialogitems[] =
{
 /*  1 */ {0, {192,424,216,504}, BUTTON, N_("Done")},
 /*  2 */ {0, {36,216,52,332}, MESSAGE, N_("New Parameter:")},
 /*  3 */ {0, {28,8,216,208}, SCROLL, x_("")},
 /*  4 */ {0, {36,336,52,504}, EDITTEXT, x_("")},
 /*  5 */ {0, {60,336,92,504}, EDITTEXT, x_("")},
 /*  6 */ {0, {60,216,76,332}, MESSAGE, N_("Default Value:")},
 /*  7 */ {0, {152,216,176,352}, BUTTON, N_("Create Parameter")},
 /*  8 */ {0, {8,8,24,504}, MESSAGE, N_("Parameters on cell %s:")},
 /*  9 */ {0, {152,368,176,504}, BUTTON, N_("Delete Parameter")},
 /* 10 */ {0, {124,216,140,332}, MESSAGE, N_("Units:")},
 /* 11 */ {0, {124,336,140,504}, POPUP, x_("")},
 /* 12 */ {0, {100,216,116,332}, MESSAGE, N_("Language:")},
 /* 13 */ {0, {100,336,116,504}, POPUP, x_("")}
};
static DIALOG us_paramdialog = {{75,75,300,589}, N_("Cell Parameters"), 0, 13, us_paramdialogitems, 0, 0};

/* special items for the "cell parameter" dialog: */
#define DATP_PARLIST     3		/* list of parameters (scroll) */
#define DATP_PARNAME     4		/* new parameter name (edit text) */
#define DATP_PARVALUE    5		/* new parameter value (edit text) */
#define DATP_MAKEPAR     7		/* create parameter (button) */
#define DATP_TITLE       8		/* title of list (message) */
#define DATP_DELPAR      9		/* delete parameter (button) */
#define DATP_UNITS      11		/* parameter units (popup) */
#define DATP_LANGUAGE   13		/* parameter language (popup) */

INTBIG us_attrparamdlog(void)
{
	REGISTER INTBIG itemHit, i, j, found, len, units, listlen, lang;
	BOOLEAN showparams;
	INTBIG newtype, newval, xoff, yoff;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER CHAR *pt, *varname, **languages;
	CHAR *newlang[8];
	REGISTER NODEINST *ni;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np, *onp;
	REGISTER VARIABLE *var, *fvar, *curvar, *wantvar;
	REGISTER void *infstr, *dia;

	np = us_needcell();
	if (np == NONODEPROTO) return(0);
	dia = DiaInitDialog(&us_paramdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DATP_PARLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	for(i=0; i<8; i++) newlang[i] = TRANSLATE(us_unitnames[i]);
	DiaSetPopup(dia, DATP_UNITS, 8, newlang);
	languages = us_languagechoices();
	DiaSetPopup(dia, DATP_LANGUAGE, 4, languages);
	infstr = initinfstr();
	formatinfstr(infstr, _("Parameters on cell %s:"), describenodeproto(np));
	DiaSetText(dia, DATP_TITLE, returninfstr(infstr));
	showparams = TRUE;
	wantvar = NOVARIABLE;
	curvar = NOVARIABLE;
	for(;;)
	{
		if (showparams)
		{
			DiaLoadTextDialog(dia, DATP_PARLIST, DiaNullDlogList, DiaNullDlogItem,
				DiaNullDlogDone, 0);
			j = -1;
			found = 0;
			for(i=0; i<np->numvar; i++)
			{
				var = &np->firstvar[i];
				if (TDGETISPARAM(var->textdescript) == 0) continue;
				infstr = initinfstr();
				formatinfstr(infstr, x_("%s (default: %s)"), truevariablename(var),
					describevariable(var, -1, -1));
				DiaStuffLine(dia, DATP_PARLIST, returninfstr(infstr));
				if (found == 0) j = found;
				if (var == wantvar) j = found;
				found++;
			}
			DiaSelectLine(dia, DATP_PARLIST, j);
			showparams = FALSE;
			itemHit = DATP_PARLIST;
		} else
		{
			/* didn't just update list: get an event */
			itemHit = DiaNextHit(dia);
		}
		if (itemHit == OK) break;
		if (itemHit == DATP_PARLIST)
		{
			/* clicked in parameter list: */
			DiaDimItem(dia, DATP_DELPAR);
			DiaSetPopupEntry(dia, DATP_UNITS, 0);
			DiaSetPopupEntry(dia, DATP_LANGUAGE, 0);
			i = DiaGetCurLine(dia, DATP_PARLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DATP_PARLIST, i);
			for(len=0; pt[len] != 0; len++)
				if (estrncmp(&pt[len], x_(" ("), 2) == 0) break;
			for(i=0; i<np->numvar; i++)
			{
				curvar = &np->firstvar[i];
				if (TDGETISPARAM(curvar->textdescript) != 0)
				{
					if (namesamen(pt, truevariablename(curvar), len) == 0) break;
				}
				curvar = NOVARIABLE;
			}
			if (curvar == NOVARIABLE) continue;
			DiaUnDimItem(dia, DATP_DELPAR);
			DiaSetPopupEntry(dia, DATP_UNITS, TDGETUNITS(curvar->textdescript));
			switch (curvar->type&(VCODE1|VCODE2))
			{
				case 0:     DiaSetPopupEntry(dia, DATP_LANGUAGE, 0);   break;
				case VTCL:  DiaSetPopupEntry(dia, DATP_LANGUAGE, 1);   break;
				case VLISP: DiaSetPopupEntry(dia, DATP_LANGUAGE, 2);   break;
				case VJAVA: DiaSetPopupEntry(dia, DATP_LANGUAGE, 3);   break;
			}
			DiaSetText(dia, DATP_PARNAME, truevariablename(curvar));
			DiaSetText(dia, DATP_PARVALUE, describevariable(curvar, -1, -1));
			continue;
		}
		if (itemHit == DATP_PARVALUE || itemHit == DATP_UNITS ||
			itemHit == DATP_LANGUAGE)
		{
			/* name must match current variable */
			if (curvar == NOVARIABLE) continue;
			varname = DiaGetText(dia, DATP_PARNAME);
			if (namesame(varname, truevariablename(curvar)) != 0) continue;

			/* change the scroll line */
			listlen = DiaGetNumScrollLines(dia, DATP_PARLIST);
			for(i=0; i<listlen; i++)
			{
				pt = DiaGetScrollLine(dia, DATP_PARLIST, i);
				if (namesamen(pt, varname, estrlen(varname)) == 0)
				{
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s (default: %s)"), varname,
						DiaGetText(dia, DATP_PARVALUE));
					DiaSetScrollLine(dia, DATP_PARLIST, i, returninfstr(infstr));
					break;
				}
			}

			/* change the variable on the cell */
			us_undrawcellvariable(curvar, np);
			units = DiaGetPopupEntry(dia, DATP_UNITS);
			getsimpletype(DiaGetText(dia, DATP_PARVALUE), &newtype, &newval, units);
			newtype |= VDISPLAY;
			lang = DiaGetPopupEntry(dia, DATP_LANGUAGE);
			if (lang != 0)
			{
				switch (lang)
				{
					case 1: newtype |= VTCL;    break;
					case 2: newtype |= VLISP;   break;
					case 3: newtype |= VJAVA;   break;
				}
				newval = (INTBIG)DiaGetText(dia, DATP_PARVALUE);
			}
			curvar = setvalkey((INTBIG)np, VNODEPROTO, curvar->key, newval, newtype);
			if (curvar != NOVARIABLE)
			{
				TDSETISPARAM(curvar->textdescript, VTISPARAMETER);
				TDSETINHERIT(curvar->textdescript, VTINHERIT);
				TDSETDISPPART(curvar->textdescript, VTDISPLAYNAMEVALINH);
				TDSETUNITS(curvar->textdescript, units);
			}
			us_drawcellvariable(curvar, np);
			continue;
		}
		if (itemHit == DATP_MAKEPAR)
		{
			if (!DiaValidEntry(dia, DATP_PARNAME)) continue;
			units = DiaGetPopupEntry(dia, DATP_UNITS);
			getsimpletype(DiaGetText(dia, DATP_PARVALUE), &newtype, &newval, units);
			infstr = initinfstr();
			varname = DiaGetText(dia, DATP_PARNAME);
			formatinfstr(infstr, x_("ATTR_%s"), varname);
			us_getnewparameterpos(np, &xoff, &yoff);
			newtype |= VDISPLAY;
			lang = DiaGetPopupEntry(dia, DATP_LANGUAGE);
			if (lang != 0)
			{
				switch (lang)
				{
					case 1: newtype |= VTCL;    break;
					case 2: newtype |= VLISP;   break;
					case 3: newtype |= VJAVA;   break;
				}
				newval = (INTBIG)DiaGetText(dia, DATP_PARVALUE);
			}
			wantvar = setval((INTBIG)np, VNODEPROTO, returninfstr(infstr), newval, newtype);
			if (wantvar != NOVARIABLE)
			{
				defaulttextsize(6, wantvar->textdescript);
				TDSETISPARAM(wantvar->textdescript, VTISPARAMETER);
				TDSETINHERIT(wantvar->textdescript, VTINHERIT);
				TDSETDISPPART(wantvar->textdescript, VTDISPLAYNAMEVALINH);
				TDSETUNITS(wantvar->textdescript, units);
				TDSETOFF(wantvar->textdescript, xoff, yoff);
				us_drawcellvariable(wantvar, np);
			}

			showparams = TRUE;
			continue;
		}
		if (itemHit == DATP_DELPAR)
		{
			if (curvar == NOVARIABLE) continue;
			us_undrawcellvariable(curvar, np);
			(void)delvalkey((INTBIG)np, VNODEPROTO, curvar->key);
			wantvar = NOVARIABLE;
			showparams = TRUE;
			continue;
		}
	}
	DiaDoneDialog(dia);

	/* update all instances */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex != 0) continue;
				if (!insamecellgrp(ni->proto, np)) continue;
				if (ni->proto != np)
				{
					if (ni->proto->cellview != el_iconview) continue;
					if (iconview(np) != ni->proto) continue;
				}

				/* ensure that this node matches the updated parameter list */
				for(i=0; i<ni->numvar; i++)
				{
					var = &ni->firstvar[i];
					if (TDGETISPARAM(var->textdescript) == 0) continue;
					fvar = NOVARIABLE;
					for(j=0; j<np->numvar; j++)
					{
						fvar = &np->firstvar[j];
						if (TDGETISPARAM(fvar->textdescript) == 0) continue;
						if (namesame(makename(var->key), makename(fvar->key)) == 0) break;
					}
					if (j >= np->numvar)
					{
						/* this node's parameter is no longer on the cell: delete from instance */
						startobjectchange((INTBIG)ni, VNODEINST);
						(void)delvalkey((INTBIG)ni, VNODEINST, var->key);
						endobjectchange((INTBIG)ni, VNODEINST);
						i--;
					} else
					{
						/* this node's parameter is still on the cell: make sure units are OK */
						if (TDGETUNITS(var->textdescript) != TDGETUNITS(fvar->textdescript))
						{
							startobjectchange((INTBIG)ni, VNODEINST);
							TDCOPY(descript, var->textdescript);
							TDSETUNITS(descript, TDGETUNITS(fvar->textdescript));
							modifydescript((INTBIG)ni, VNODEINST, var, descript);
							endobjectchange((INTBIG)ni, VNODEINST);
						}

						/* make sure visibility is OK */
						if (TDGETINTERIOR(fvar->textdescript) != 0) var->type &= ~VDISPLAY; else
							var->type |= VDISPLAY;
					}
				}
				for(j=0; j<np->numvar; j++)
				{
					fvar = &np->firstvar[j];
					if (TDGETISPARAM(fvar->textdescript) == 0) continue;
					for(i=0; i<ni->numvar; i++)
					{
						var = &ni->firstvar[i];
						if (TDGETISPARAM(var->textdescript) == 0) continue;
						if (namesame(makename(var->key), makename(fvar->key)) == 0) break;
					}
					if (i >= ni->numvar)
					{
						/* this cell parameter is not on the node: add to instance */
						us_addparameter(ni, fvar->key, fvar->addr, fvar->type, fvar->textdescript);
					}
				}
			}
		}
	}

	/* make sure all of this cell's parameters are valid */
	for(j=0; j<np->numvar; j++)
	{
		fvar = &np->firstvar[j];
		if (TDGETISPARAM(fvar->textdescript) == 0) continue;
		if ((fvar->type&VDISPLAY) == 0)
		{
			fvar->type |= VDISPLAY;
			us_drawcellvariable(fvar, np);
		}
	}
	return(0);
}

/****************************** CHANGE DIALOG ******************************/

/* Change */
static DIALOGITEM us_changedialogitems[] =
{
 /*  1 */ {0, {264,344,288,416}, BUTTON, N_("OK")},
 /*  2 */ {0, {264,248,288,320}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,232,24,438}, RADIO, N_("Change selected ones only")},
 /*  4 */ {0, {56,232,72,438}, RADIO, N_("Change all in this cell")},
 /*  5 */ {0, {104,232,120,438}, RADIO, N_("Change all in all libraries")},
 /*  6 */ {0, {32,232,48,438}, RADIO, N_("Change all connected to this")},
 /*  7 */ {0, {8,8,264,223}, SCROLL, x_("")},
 /*  8 */ {0, {272,8,288,78}, MESSAGE, N_("Library:")},
 /*  9 */ {0, {272,80,288,218}, POPUP, x_("")},
 /* 10 */ {0, {212,232,228,438}, CHECK, N_("Ignore port names")},
 /* 11 */ {0, {236,232,252,438}, CHECK, N_("Allow missing ports")},
 /* 12 */ {0, {140,232,156,438}, CHECK, N_("Change nodes with arcs")},
 /* 13 */ {0, {164,232,180,438}, CHECK, N_("Show primitives")},
 /* 14 */ {0, {188,232,204,438}, CHECK, N_("Show cells")},
 /* 15 */ {0, {80,232,96,438}, RADIO, N_("Change all in this library")}
};
static DIALOG us_changedialog = {{50,75,347,523}, N_("Change Nodes and Arcs"), 0, 15, us_changedialogitems, 0, 0};

static BOOLEAN us_onproto(PORTPROTO*, ARCPROTO*);

/* special items for the "change" dialog: */
#define DCHG_THISONLY       3		/* This only (radio) */
#define DCHG_INCELL         4		/* In cell (radio) */
#define DCHG_UNIVERSALLY    5		/* Universally (radio) */
#define DCHG_CONNECTED      6		/* Connected (radio) */
#define DCHG_ALTLIST        7		/* Alternate list (scroll) */
#define DCHG_LIBRARY_L      8		/* Library label (stat text) */
#define DCHG_LIBRARIES      9		/* Libraries (popup) */
#define DCHG_IGNOREPORT    10		/* Ignore port names (check) */
#define DCHG_ALLOWDELETION 11		/* Allow missing port (check) */
#define DCHG_NODESANDARCS  12		/* Change nodes with arcs (check) */
#define DCHG_SHOWPRIMS     13		/* Show primitive nodes (check) */
#define DCHG_SHOWCELLS     14		/* Show cell nodes (check) */
#define DCHG_INLIBRARY     15		/* In this library (radio) */

INTBIG us_replacedlog(void)
{
	INTBIG i, itemHit, total, ac;
	BOOLEAN loadarclist, loadnodelist;
	static INTBIG changeextent = DCHG_THISONLY;
	static INTBIG nodesandarcs = 0;
	REGISTER ARCPROTO *ap;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp1, *pp2;
	REGISTER LIBRARY *lib;
	REGISTER GEOM **list, *firstgeom;
	REGISTER CHAR **liblist;
	CHAR *par[7], *newname;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* see what is highlighted */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0);
	firstgeom = list[0];
	if (firstgeom == NOGEOM) return(0);

	/* display the change dialog box */
	dia = DiaInitDialog(&us_changedialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DCHG_ALTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT);
	liblist = 0;
	us_curlib = el_curlib;
	if (firstgeom->entryisnode)
	{
		loadnodelist = TRUE;
		loadarclist = FALSE;
		DiaUnDimItem(dia, DCHG_LIBRARY_L);
		DiaUnDimItem(dia, DCHG_LIBRARIES);
		DiaUnDimItem(dia, DCHG_IGNOREPORT);
		DiaUnDimItem(dia, DCHG_ALLOWDELETION);
		DiaUnDimItem(dia, DCHG_SHOWPRIMS);
		DiaUnDimItem(dia, DCHG_SHOWCELLS);
		ni = firstgeom->entryaddr.ni;
		if (ni->proto->primindex == 0)
		{
			DiaSetControl(dia, DCHG_SHOWCELLS, 1);
			us_curlib = ni->proto->lib;
		} else
		{
			DiaSetControl(dia, DCHG_SHOWPRIMS, 1);
		}
		DiaSetControl(dia, DCHG_NODESANDARCS, 0);
		DiaDimItem(dia, DCHG_NODESANDARCS);
	} else
	{
		loadarclist = TRUE;
		loadnodelist = FALSE;
		DiaDimItem(dia, DCHG_LIBRARY_L);
		DiaDimItem(dia, DCHG_LIBRARIES);
		DiaDimItem(dia, DCHG_IGNOREPORT);
		DiaDimItem(dia, DCHG_ALLOWDELETION);
		DiaDimItem(dia, DCHG_SHOWPRIMS);
		DiaDimItem(dia, DCHG_SHOWCELLS);
		DiaUnDimItem(dia, DCHG_NODESANDARCS);
		DiaSetControl(dia, DCHG_NODESANDARCS, nodesandarcs);
	}
	DiaSetControl(dia, changeextent, 1);

	/* loop until done */
	for(;;)
	{
		if (loadnodelist)
		{
			DiaDimItem(dia, DCHG_LIBRARY_L);
			DiaDimItem(dia, DCHG_LIBRARIES);
			DiaLoadTextDialog(dia, DCHG_ALTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
			if (DiaGetControl(dia, DCHG_SHOWCELLS) != 0)
			{
				/* cell: only list other cells as replacements */
				us_showoldversions = 1;
				us_showcellibrarycells = 1;
				us_showonlyrelevantcells = 0;
				DiaLoadTextDialog(dia, DCHG_ALTLIST, us_oldcelltopofcells, us_oldcellnextcells,
					DiaNullDlogDone, 0);
				(void)us_setscrolltocurrentcell(DCHG_ALTLIST, TRUE, FALSE, FALSE, FALSE, dia);
				DiaUnDimItem(dia, DCHG_LIBRARY_L);
				DiaUnDimItem(dia, DCHG_LIBRARIES);
				if (liblist != 0) efree((CHAR *)liblist);
				liblist = us_makelibrarylist(&total, us_curlib, &i);
				DiaSetPopup(dia, DCHG_LIBRARIES, total, liblist);
				if (i >= 0) DiaSetPopupEntry(dia, DCHG_LIBRARIES, i);
			}
			if (DiaGetControl(dia, DCHG_SHOWPRIMS) != 0)
			{
				/* primitive: list primitives in this and the generic technology */
				for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					DiaStuffLine(dia, DCHG_ALTLIST, np->protoname);
				if (el_curtech != gen_tech)
				{
					DiaStuffLine(dia, DCHG_ALTLIST, x_("Generic:Universal-Pin"));
					DiaStuffLine(dia, DCHG_ALTLIST, x_("Generic:Invisible-Pin"));
					DiaStuffLine(dia, DCHG_ALTLIST, x_("Generic:Unrouted-Pin"));
				}
				DiaSelectLine(dia, DCHG_ALTLIST, 0);
			}
			loadnodelist = FALSE;
		}
		if (loadarclist)
		{
			/* load arcs in current technology, arc's technology, and generic technology */
			DiaLoadTextDialog(dia, DCHG_ALTLIST, DiaNullDlogList, DiaNullDlogItem,
				DiaNullDlogDone, -1);
			ai = firstgeom->entryaddr.ai;
			pp1 = ai->end[0].portarcinst->proto;
			pp2 = ai->end[1].portarcinst->proto;
			for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				if (DiaGetControl(dia, DCHG_NODESANDARCS) == 0)
				{
					if (!us_onproto(pp1, ap)) continue;
					if (!us_onproto(pp2, ap)) continue;
				}
				DiaStuffLine(dia, DCHG_ALTLIST, ap->protoname);
			}
			if (el_curtech != gen_tech)
				for(ap = gen_tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				if (DiaGetControl(dia, DCHG_NODESANDARCS) == 0)
				{
					if (!us_onproto(pp1, ap)) continue;
					if (!us_onproto(pp2, ap)) continue;
				}
				DiaStuffLine(dia, DCHG_ALTLIST, describearcproto(ap));
			}
			if (ai->proto->tech != el_curtech && ai->proto->tech != gen_tech)
				for(ap = ai->proto->tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			{
				if (DiaGetControl(dia, DCHG_NODESANDARCS) == 0)
				{
					if (!us_onproto(pp1, ap)) continue;
					if (!us_onproto(pp2, ap)) continue;
				}
				DiaStuffLine(dia, DCHG_ALTLIST, describearcproto(ap));
			}
			DiaSelectLine(dia, DCHG_ALTLIST, 0);
			loadarclist = FALSE;
		}
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DCHG_THISONLY || itemHit == DCHG_INCELL ||
			itemHit == DCHG_UNIVERSALLY || itemHit == DCHG_CONNECTED ||
			itemHit == DCHG_INLIBRARY)
		{
			changeextent = itemHit;
			DiaSetControl(dia, DCHG_THISONLY, 0);
			DiaSetControl(dia, DCHG_INCELL, 0);
			DiaSetControl(dia, DCHG_INLIBRARY, 0);
			DiaSetControl(dia, DCHG_UNIVERSALLY, 0);
			DiaSetControl(dia, DCHG_CONNECTED, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DCHG_UNIVERSALLY)
			{
				if (!firstgeom->entryisnode)
				{
					if (DiaGetControl(dia, DCHG_NODESANDARCS) != 0)
					{
						DiaSetControl(dia, DCHG_NODESANDARCS, 0);
						loadarclist = TRUE;
					}
					DiaDimItem(dia, DCHG_NODESANDARCS);
				}
			} else
			{
				if (!firstgeom->entryisnode)
					DiaUnDimItem(dia, DCHG_NODESANDARCS);
			}
		}
		if (itemHit == DCHG_IGNOREPORT || itemHit == DCHG_ALLOWDELETION ||
			itemHit == DCHG_NODESANDARCS || itemHit == DCHG_SHOWCELLS ||
			itemHit == DCHG_SHOWPRIMS)
		{
			i = 1-DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			if (itemHit == DCHG_NODESANDARCS &&
				!firstgeom->entryisnode)
			{
				loadarclist = TRUE;
				nodesandarcs = i;
			}
			if (itemHit == DCHG_SHOWCELLS || itemHit == DCHG_SHOWPRIMS)
				loadnodelist = TRUE;
			continue;
		}
		if (itemHit == DCHG_LIBRARIES)
		{
			i = DiaGetPopupEntry(dia, DCHG_LIBRARIES);
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				if (namesame(lib->libname, liblist[i]) == 0) break;
			if (lib == NOLIBRARY) continue;
			us_curlib = lib;
			DiaLoadTextDialog(dia, DCHG_ALTLIST, us_oldcelltopofcells, us_oldcellnextcells,
				DiaNullDlogDone, 0);
			(void)us_setscrolltocurrentcell(DCHG_ALTLIST, TRUE, FALSE, FALSE, FALSE, dia);
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		ac = 0;
		infstr = initinfstr();
		if (us_curlib != el_curlib)
		{
			addstringtoinfstr(infstr, us_curlib->libname);
			addtoinfstr(infstr, ':');
		}
		addstringtoinfstr(infstr, DiaGetScrollLine(dia, DCHG_ALTLIST, DiaGetCurLine(dia, DCHG_ALTLIST)));
		(void)allocstring(&newname, returninfstr(infstr), el_tempcluster);
		par[ac++] = newname;
		if (DiaGetControl(dia, DCHG_INCELL) != 0) par[ac++] = x_("this-cell");
		if (DiaGetControl(dia, DCHG_INLIBRARY) != 0) par[ac++] = x_("this-library");
		if (DiaGetControl(dia, DCHG_UNIVERSALLY) != 0) par[ac++] = x_("universally");
		if (DiaGetControl(dia, DCHG_CONNECTED) != 0) par[ac++] = x_("connected");
		if (DiaGetControl(dia, DCHG_IGNOREPORT) != 0) par[ac++] = x_("ignore-port-names");
		if (DiaGetControl(dia, DCHG_ALLOWDELETION) != 0) par[ac++] = x_("allow-missing-ports");
		if (DiaGetControl(dia, DCHG_NODESANDARCS) != 0) par[ac++] = x_("nodes-too");
		us_replace(ac, par);
		efree((CHAR *)newname);
	}
	if (liblist != 0) efree((CHAR *)liblist);
	DiaDoneDialog(dia);
	return(0);
}

/*
 * helper routine to determine whether arcproto "ap" can connect to portproto "pp".
 * returns nonzero if so
 */
BOOLEAN us_onproto(PORTPROTO *pp, ARCPROTO *ap)
{
	REGISTER INTBIG i;

	for(i=0; pp->connects[i] != NOARCPROTO; i++)
		if (pp->connects[i] == ap) return(TRUE);
	return(FALSE);
}

/****************************** CHOICE DIALOGS ******************************/

/* No/Yes */
static DIALOGITEM us_noyesdialogitems[] =
{
/*  1 */ {0, {80,132,104,204}, BUTTON, N_("No")},
/*  2 */ {0, {80,60,104,124}, BUTTON, N_("Yes")},
/*  3 */ {0, {8,8,72,256}, MESSAGE, x_("")}
};
static DIALOG us_noyesdialog = {{50,75,163,341}, N_("Warning"), 0, 3, us_noyesdialogitems, 0, 0};

/* special items for "no/yes" dialog ("no" is default): */
#define DNOY_NO      1		/* No (button) */
#define DNOY_YES     2		/* Yes (button) */
#define DNOY_MESSAGE 3		/* Message (stat text) */

INTBIG us_noyesdlog(CHAR *prompt, CHAR *paramstart[])
{
	INTBIG itemHit, oldplease;
	REGISTER void *dia;

	/* display the no/yes dialog box */
	dia = DiaInitDialog(&us_noyesdialog);
	if (dia == 0) return(0);

	/* load the message */
	DiaSetText(dia, DNOY_MESSAGE, prompt);

	/* loop until done */
	oldplease = el_pleasestop;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DNOY_NO || itemHit == DNOY_YES) break;
	}
	el_pleasestop = oldplease;

	if (itemHit == DNOY_NO) paramstart[0] = x_("no"); else
		paramstart[0] = x_("yes");
	DiaDoneDialog(dia);
	return(1);
}

/* Yes/No */
static DIALOGITEM us_yesnodialogitems[] =
{
/*  1 */ {0, {64,156,88,228}, BUTTON, N_("Yes")},
/*  2 */ {0, {64,68,88,140}, BUTTON, N_("No")},
/*  3 */ {0, {6,15,54,279}, MESSAGE, x_("")}
};
static DIALOG us_yesnodialog = {{50,75,150,369}, N_("Warning"), 0, 3, us_yesnodialogitems, 0, 0};

/* special items for "yes/no" dialog ("yes" is default): */
#define DYNO_YES     1		/* Yes (button) */
#define DYNO_NO      2		/* No (button) */
#define DYNO_MESSAGE 3		/* Message (stat text) */

INTBIG us_yesnodlog(CHAR *prompt, CHAR *paramstart[])
{
	INTBIG itemHit, oldplease;
	REGISTER void *dia;

	/* display the yes/no dialog box */
	dia = DiaInitDialog(&us_yesnodialog);
	if (dia == 0) return(0);

	/* load the message */
	DiaSetText(dia, DYNO_MESSAGE, prompt);

	/* loop until done */
	oldplease = el_pleasestop;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DYNO_YES || itemHit == DYNO_NO) break;
	}
	el_pleasestop = oldplease;

	if (itemHit == DYNO_YES) paramstart[0] = x_("yes"); else paramstart[0] = x_("no");
	DiaDoneDialog(dia);
	return(1);
}

/* Prompt: No/Yes/Always */
static DIALOGITEM us_noyesalwaysdialogitems[] =
{
/*  1 */ {0, {64,168,88,248}, BUTTON, N_("No")},
/*  2 */ {0, {64,8,88,88}, BUTTON, N_("Yes")},
/*  3 */ {0, {124,88,148,168}, BUTTON, N_("Always")},
/*  4 */ {0, {8,8,56,248}, MESSAGE, N_("Allow this?")},
/*  5 */ {0, {100,8,116,248}, MESSAGE, N_("Click \"Always\" to disable the lock")}
};
static DIALOG us_noyesalwaysdialog = {{75,75,232,332}, N_("Allow this change?"), 0, 5, us_noyesalwaysdialogitems, 0, 0};

/* special items for "no/yes/always" dialog ("no" is default): */
#define DYNA_NO      1		/* No (button) */
#define DYNA_YES     2		/* Yes (button) */
#define DYNA_ALWAYS  3		/* Always (button) */
#define DYNA_MESSAGE 4		/* Message (stat text) */

INTBIG us_noyesalwaysdlog(CHAR *prompt, CHAR *paramstart[])
{
	REGISTER INTBIG itemHit;
	REGISTER void *dia;

	dia = DiaInitDialog(&us_noyesalwaysdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DYNA_MESSAGE, prompt);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DYNA_NO || itemHit == DYNA_YES || itemHit == DYNA_ALWAYS) break;
	}
	DiaDoneDialog(dia);
	if (itemHit == DYNA_NO) paramstart[0] = x_("no"); else
		if (itemHit == DYNA_YES) paramstart[0] = x_("yes"); else
			paramstart[0] = x_("always");
	return(1);
}

/* Prompt: No/Yes/Cancel */
static DIALOGITEM us_noyescanceldialogitems[] =
{
 /*  1 */ {0, {80,108,104,188}, BUTTON, N_("No")},
 /*  2 */ {0, {80,12,104,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {80,204,104,284}, BUTTON, N_("Yes")},
 /*  4 */ {0, {8,8,72,284}, MESSAGE, x_("")}
};
static DIALOG us_noyescanceldialog = {{75,75,188,368}, N_("Allow this change?"), 0, 4, us_noyescanceldialogitems, 0, 0};

/* special items for "no/yes/cancel" dialog ("no" is default): */
#define DYNC_NO      1		/* No (button) */
#define DYNC_CANCEL  2		/* Cancel (button) */
#define DYNC_YES     3		/* Yes (button) */
#define DYNC_MESSAGE 4		/* Message (stat text) */

INTBIG us_noyescanceldlog(CHAR *prompt, CHAR *paramstart[])
{
	REGISTER INTBIG itemHit;
	REGISTER void *dia;

	dia = DiaInitDialog(&us_noyescanceldialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DYNC_MESSAGE, prompt);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DYNC_NO || itemHit == DYNC_YES || itemHit == DYNC_CANCEL) break;
	}
	DiaDoneDialog(dia);
	if (itemHit == DYNC_NO) paramstart[0] = x_("no"); else
		if (itemHit == DYNC_YES) paramstart[0] = x_("yes"); else
			paramstart[0] = x_("cancel");
	return(1);
}

/* Prompt: OK */
static DIALOGITEM us_okmessagedialogitems[] =
{
/*  1 */ {0, {72,64,96,144}, BUTTON, N_("OK")},
/*  2 */ {0, {8,8,56,212}, MESSAGE, x_("")}
};
static DIALOG us_okmessagedialog = {{75,75,180,296}, 0, 0, 2, us_okmessagedialogitems, 0, 0};

/* special items for "OK prompt" dialog: */
#define DMSG_MESSAGE 2		/* Message (stat text) */

void DiaMessageInDialog(CHAR *message, ...)
{
	va_list ap;
	CHAR line[1000];
	INTBIG itemHit;
	REGISTER void *dia;

	/* display the message dialog box */
	dia = DiaInitDialog(&us_okmessagedialog);
	if (dia == 0) return;

	/* put the message in it */
	var_start(ap, message);
	evsnprintf(line, 1000, message, ap);
	va_end(ap);
	DiaSetText(dia, DMSG_MESSAGE, line);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
	}
	DiaDoneDialog(dia);
}

/****************************** COLOR OPTIONS DIALOG ******************************/

#define DEGTORAD    (EPI/180.0)
#define INTENSITYDIVISIONS 20
#define WHEELRADIUS        96
#define WHEELEDGE          (WHEELRADIUS + WHEELRADIUS/16)

/* Color Mixing */
static DIALOGITEM us_colormixdialogitems[] =
{
 /*  1 */ {0, {212,400,236,468}, BUTTON, N_("OK")},
 /*  2 */ {0, {212,320,236,388}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {276,8,292,24}, USERDRAWN, x_("")},
 /*  4 */ {0, {276,36,292,472}, RADIO, N_("Entry")},
 /*  5 */ {0, {296,8,312,24}, USERDRAWN, x_("")},
 /*  6 */ {0, {296,36,312,472}, RADIO, N_("Entry")},
 /*  7 */ {0, {316,8,332,24}, USERDRAWN, x_("")},
 /*  8 */ {0, {316,36,332,472}, RADIO, N_("Entry")},
 /*  9 */ {0, {336,8,352,24}, USERDRAWN, x_("")},
 /* 10 */ {0, {336,36,352,472}, RADIO, N_("Entry")},
 /* 11 */ {0, {356,8,372,24}, USERDRAWN, x_("")},
 /* 12 */ {0, {356,36,372,472}, RADIO, N_("Entry")},
 /* 13 */ {0, {376,8,392,24}, USERDRAWN, x_("")},
 /* 14 */ {0, {376,36,392,472}, RADIO, N_("Entry")},
 /* 15 */ {0, {396,8,412,24}, USERDRAWN, x_("")},
 /* 16 */ {0, {396,36,412,472}, RADIO, N_("Entry")},
 /* 17 */ {0, {416,8,432,24}, USERDRAWN, x_("")},
 /* 18 */ {0, {416,36,432,472}, RADIO, N_("Entry")},
 /* 19 */ {0, {436,8,452,24}, USERDRAWN, x_("")},
 /* 20 */ {0, {436,36,452,472}, RADIO, N_("Entry")},
 /* 21 */ {0, {456,8,472,24}, USERDRAWN, x_("")},
 /* 22 */ {0, {456,36,472,472}, RADIO, N_("Entry")},
 /* 23 */ {0, {476,8,492,24}, USERDRAWN, x_("")},
 /* 24 */ {0, {476,36,492,472}, RADIO, N_("Entry")},
 /* 25 */ {0, {496,8,512,24}, USERDRAWN, x_("")},
 /* 26 */ {0, {496,36,512,472}, RADIO, N_("Entry")},
 /* 27 */ {0, {516,8,532,24}, USERDRAWN, x_("")},
 /* 28 */ {0, {516,36,532,472}, RADIO, N_("Entry")},
 /* 29 */ {0, {536,8,552,24}, USERDRAWN, x_("")},
 /* 30 */ {0, {536,36,552,472}, RADIO, N_("Entry")},
 /* 31 */ {0, {556,8,572,24}, USERDRAWN, x_("")},
 /* 32 */ {0, {556,36,572,472}, RADIO, N_("Entry")},
 /* 33 */ {0, {576,8,592,24}, USERDRAWN, x_("")},
 /* 34 */ {0, {576,36,592,472}, RADIO, N_("Entry")},
 /* 35 */ {0, {32,8,244,220}, USERDRAWN, x_("")},
 /* 36 */ {0, {32,228,244,298}, USERDRAWN, x_("")},
 /* 37 */ {0, {8,16,24,144}, MESSAGE, N_("Hue/Saturation:")},
 /* 38 */ {0, {8,228,24,308}, MESSAGE, N_("Intensity:")},
 /* 39 */ {0, {252,160,268,396}, POPUP, x_("")},
 /* 40 */ {0, {36,304,52,388}, MESSAGE, N_("Red:")},
 /* 41 */ {0, {60,304,76,388}, MESSAGE, N_("Green:")},
 /* 42 */ {0, {84,304,100,388}, MESSAGE, N_("Blue:")},
 /* 43 */ {0, {36,393,52,457}, EDITTEXT, x_("")},
 /* 44 */ {0, {60,393,76,457}, EDITTEXT, x_("")},
 /* 45 */ {0, {84,393,100,457}, EDITTEXT, x_("")},
 /* 46 */ {0, {252,8,268,148}, MESSAGE, N_("Colors being edited:")},
 /* 47 */ {0, {176,304,200,472}, BUTTON, N_("Set Transparent Layers")},
 /* 48 */ {0, {116,304,132,388}, MESSAGE, N_("Opacity:")},
 /* 49 */ {0, {140,304,156,388}, MESSAGE, N_("Foreground:")},
 /* 50 */ {0, {116,393,132,457}, EDITTEXT, x_("")},
 /* 51 */ {0, {140,393,156,457}, POPUP, x_("")}
};
static DIALOG us_colormixdialog = {{75,75,676,557}, N_("Color Mixing"), 0, 51, us_colormixdialogitems, 0, 0};

/* special items for the "Color Mixing" dialog: */
#define DCLR_FIRSTPATCH     3	/* first color patch for palette (user) */
#define DCLR_FIRSTBUTTON    4	/* first button for palette (user) */
#define DCLR_LASTBUTTON    34	/* last button for palette (user) */
#define DCLR_HUESAT        35	/* Hue/Saturation wheel (user) */
#define DCLR_INTENSITY     36	/* Intensity slider (user) */
#define DCLR_PALETTES      39	/* palettes (popup) */
#define DCLR_REDVAL        43	/* red (edit text) */
#define DCLR_GREENVAL      44	/* green (edit text) */
#define DCLR_BLUEVAL       45	/* blue (edit text) */
#define DCLR_COMPPRIMARY   47	/* Compute from primaries (button) */
#define DCLR_OPACITYVAL_L  48	/* opacity label (stat text) */
#define DCLR_FOREGROUND_L  49	/* foreground label (stat text) */
#define DCLR_OPACITYVAL    50	/* opacity (edit text) */
#define DCLR_FOREGROUND    51	/* foreground (popup) */

static INTBIG  us_colormixredmap[256], us_colormixgreenmap[256], us_colormixbluemap[256];
static INTBIG  us_colormixindex[16];
static INTBIG  us_colormixcurindex;
static INTBIG  us_colormixlayercount;
static INTBIG  us_colormixcurbank;
static CHAR  **us_colormixlayernames;
static BOOLEAN us_colormixingprint;
static INTBIG *us_colormixlayerprintcolordata;
static float   us_colormixtheta, us_colormixr, us_colormixinten;
static CHAR   *us_colormixnames[5];

static void    us_colormixreload(void *dia);
static CHAR   *us_colormixentryname(CHAR *overlayernames[], INTBIG ind);
static void    us_colormixbuildindex(INTBIG layer1, INTBIG layer2, INTBIG layer3, INTBIG layer4, INTBIG layer5);
static void    us_colormixdrawpalette(RECTAREA *ra, void *dia);
static void    us_colormixtogglemarker(void *dia);
static void    us_colormixdrawsquare(INTBIG sindex, void *dia);
static void    us_colormixmergecolor(INTBIG r1, INTBIG g1, INTBIG b1, INTBIG r2, INTBIG g2, INTBIG b2,
				INTBIG *ro, INTBIG *go, INTBIG *bo);
static void    us_colormixsmoothcolors(void);
static BOOLEAN us_colormixsetcolor(void *dia, INTBIG r, INTBIG g, INTBIG b);

/*
 * Routine to do color mixing.  The current color map is mixed, and the names of the transparent
 * layers are in "overlayernames".  In addition, print colors are mixed for the "layercount" layers
 * whose names are in "layernames" and whose data is in "printcolors".  The array "printcolors" has
 * 5 entries per color: the first 3 are the R/G/B; number 4 is the opacity (a fractional value from 0
 * to 1, which means from 0 to WHOLE); number 5 is the foreground factor (0 for off, 1 for on).
 * Returns TRUE if something changed; FALSE if nothing changed or it was cancelled.
 */
BOOLEAN us_colormixdlog(CHAR *overlayernames[], INTBIG layercount, CHAR **layernames, INTBIG *printcolors)
{
	REGISTER INTBIG itemHit, i, cx, cy, numpalettes, opa, fore;
	INTBIG x, y, r, g, b;
	BOOLEAN colorchanged;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	RECTAREA ra;
	REGISTER void *dia, *infstr;
	CHAR **palettelist, *transstr[2];

	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE) return(FALSE);
	for(i=0; i<256; i++)
	{
		us_colormixredmap[i] = ((INTBIG *)varred->addr)[i];
		us_colormixgreenmap[i] = ((INTBIG *)vargreen->addr)[i];
		us_colormixbluemap[i] = ((INTBIG *)varblue->addr)[i];
	}
	us_colormixlayercount = layercount;
	us_colormixlayernames = layernames;
	us_colormixlayerprintcolordata = printcolors;

	/* display the color mixing dialog box */
	dia = DiaInitDialog(&us_colormixdialog);
	if (dia == 0) return(FALSE);
	numpalettes = 7 + (layercount+15)/16;
	palettelist = (CHAR **)emalloc(numpalettes * (sizeof (CHAR *)), el_tempcluster);
	if (palettelist == 0) return(FALSE);
	palettelist[0] = _("All transparent layers");
	palettelist[1] = _("Special colors");
	for(i=0; i<5; i++)
	{
		us_colormixnames[i] = overlayernames[i];
		infstr = initinfstr();
		formatinfstr(infstr, _("Transparent layer %ld: %s"), i+1, overlayernames[i]);
		(void)allocstring(&palettelist[i+2], returninfstr(infstr), el_tempcluster);
	}
	for(i=7; i<numpalettes; i++)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Print layers, bank %ld"), i-6);
		(void)allocstring(&palettelist[i], returninfstr(infstr), el_tempcluster);
	}
	DiaSetPopup(dia, DCLR_PALETTES, numpalettes, palettelist);
	us_colormixcurbank = 0;
	DiaSetPopupEntry(dia, DCLR_PALETTES, us_colormixcurbank);
	transstr[0] = _("Off");
	transstr[1] = _("On");
	DiaSetPopup(dia, DCLR_FOREGROUND, 2, transstr);
	us_colormixcurindex = 0;
	DiaSetControl(dia, DCLR_FIRSTBUTTON, 1);
	us_colormixdrawpalette(&ra, dia);
	DiaRedispRoutine(dia, DCLR_HUESAT, us_colormixdrawpalette);
	us_colormixingprint = FALSE;
	DiaDimItem(dia, DCLR_OPACITYVAL);
	DiaDimItem(dia, DCLR_OPACITYVAL_L);
	DiaDimItem(dia, DCLR_FOREGROUND);
	DiaDimItem(dia, DCLR_FOREGROUND_L);

	/* loop until done */
	colorchanged = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit >= DCLR_FIRSTPATCH && itemHit <= DCLR_LASTBUTTON)
		{
			/* hit on one of the 16 colors */
			us_colormixtogglemarker(dia);
			for(i=DCLR_FIRSTBUTTON; i<=DCLR_LASTBUTTON; i += 2) DiaSetControl(dia, i, 0);
			i = (itemHit + 1) / 2 * 2;
			DiaSetControl(dia, i, 1);
			us_colormixcurindex = (itemHit-DCLR_FIRSTPATCH)/2;
			us_colormixdrawsquare(us_colormixcurindex, dia);
			us_colormixtogglemarker(dia);
			continue;
		}
		if (itemHit == DCLR_PALETTES)
		{
			/* changed the set of colors */
			us_colormixtogglemarker(dia);
			us_colormixcurbank = DiaGetPopupEntry(dia, DCLR_PALETTES);
			us_colormixcurindex = 0;
			for(i=DCLR_FIRSTBUTTON; i<=DCLR_LASTBUTTON; i += 2) DiaSetControl(dia, i, 0);
			DiaSetControl(dia, DCLR_FIRSTBUTTON, 1);
			us_colormixreload(dia);
			us_colormixtogglemarker(dia);
			continue;
		}
		if (itemHit == DCLR_HUESAT)
		{
			/* hit in the hue/saturation wheel */
			DiaGetMouse(dia, &x, &y);
			DiaItemRect(dia, DCLR_HUESAT, &ra);
			cx = (ra.left + ra.right) / 2;
			cy = (ra.top + ra.bottom) / 2;
			us_colormixtogglemarker(dia);
			us_colormixtheta = (float)((figureangle(cx, cy, x, y) + 5) / 10);
			if (us_colormixtheta < 0.0) us_colormixtheta += 360.0;
			us_colormixr = (float)sqrt((float)((y-cy)*(y-cy) + (x-cx)*(x-cx)));
			if (us_colormixr > WHEELEDGE) us_colormixr = WHEELEDGE;
			us_hsvtorgb(us_colormixtheta / 360.0f, us_colormixr / WHEELEDGE, us_colormixinten, &r,
				&g, &b);
			if (us_colormixsetcolor(dia, r, g, b))
			{
				colorchanged = TRUE;
				us_colormixdrawsquare(us_colormixcurindex, dia);
			}
			us_colormixtogglemarker(dia);
			continue;
		}
		if (itemHit == DCLR_INTENSITY)
		{
			/* hit in the intensity slider */
			DiaGetMouse(dia, &x, &y);
			DiaItemRect(dia, DCLR_INTENSITY, &ra);
			cx = (ra.left + ra.right) / 2;
			cy = (ra.top + ra.bottom) / 2;
			us_colormixtogglemarker(dia);
			us_colormixinten = (float)(ra.bottom - y) / (float)(ra.bottom - ra.top);
			us_hsvtorgb(us_colormixtheta / 360.0f, us_colormixr / WHEELEDGE, us_colormixinten, &r,
				&g, &b);
			if (us_colormixsetcolor(dia, r, g, b))
			{
				colorchanged = TRUE;
				us_colormixdrawsquare(us_colormixcurindex, dia);
			}
			us_colormixtogglemarker(dia);
			continue;
		}
		if (itemHit == DCLR_REDVAL || itemHit == DCLR_GREENVAL ||
			itemHit == DCLR_BLUEVAL)
		{
			r = myatoi(DiaGetText(dia, DCLR_REDVAL));
			g = myatoi(DiaGetText(dia, DCLR_GREENVAL));
			b = myatoi(DiaGetText(dia, DCLR_BLUEVAL));
			us_colormixtogglemarker(dia);
			if (us_colormixsetcolor(dia, r, g, b))
			{
				colorchanged = TRUE;
				us_colormixdrawsquare(us_colormixcurindex, dia);
			}
			us_colormixtogglemarker(dia);
			continue;
		}
		if (itemHit == DCLR_OPACITYVAL || itemHit == DCLR_FOREGROUND)
		{
			if (us_colormixcurbank < 7) continue;
			opa = atofr(DiaGetText(dia, DCLR_OPACITYVAL));
			fore = DiaGetPopupEntry(dia, DCLR_FOREGROUND);
			i = (us_colormixcurbank-7)*16+us_colormixcurindex;
			if (opa == us_colormixlayerprintcolordata[i*5+3] &&
				fore == us_colormixlayerprintcolordata[i*5+4]) continue;
			us_colormixlayerprintcolordata[i*5+3] = opa;
			us_colormixlayerprintcolordata[i*5+4] = fore;
			colorchanged = TRUE;
			continue;
		}

		if (itemHit == DCLR_COMPPRIMARY)
		{
			/* recompute colors based on layer depth of primaries */
			us_colormixtogglemarker(dia);
			us_colormixsmoothcolors();
			us_colormixreload(dia);
			us_colormixtogglemarker(dia);
			colorchanged = TRUE;
			continue;
		}
	}

	if (itemHit == CANCEL) colorchanged = FALSE;
	if (colorchanged)
	{
		/* propagate highlight and grid colors to every appropriate entry */
		for(i=0; i<256; i++)
		{
			if ((i&LAYERG) != 0)
			{
				us_colormixredmap[i] = us_colormixredmap[LAYERG];
				us_colormixgreenmap[i] = us_colormixgreenmap[LAYERG];
				us_colormixbluemap[i] = us_colormixbluemap[LAYERG];
			}
			if ((i&LAYERH) != 0)
			{
				us_colormixredmap[i] = us_colormixredmap[LAYERH];
				us_colormixgreenmap[i] = us_colormixgreenmap[LAYERH];
				us_colormixbluemap[i] = us_colormixbluemap[LAYERH];
			}
		}
		startobjectchange((INTBIG)us_tool, VTOOL);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, (INTBIG)us_colormixredmap,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, (INTBIG)us_colormixgreenmap,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, (INTBIG)us_colormixbluemap,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		endobjectchange((INTBIG)us_tool, VTOOL);
	}
	DiaDoneDialog(dia);
	for(i=2; i<numpalettes; i++)
		efree((CHAR *)palettelist[i]);
	efree((CHAR *)palettelist);
	return(colorchanged);
}

BOOLEAN us_colormixsetcolor(void *dia, INTBIG r, INTBIG g, INTBIG b)
{
	REGISTER INTBIG i;
	Q_UNUSED( dia );

	if (us_colormixcurbank >= 7)
	{
		i = (us_colormixcurbank-7)*16+us_colormixcurindex;
		if (r == us_colormixlayerprintcolordata[i*5] &&
			g == us_colormixlayerprintcolordata[i*5+1] &&
			b == us_colormixlayerprintcolordata[i*5+2]) return(FALSE);
		us_colormixlayerprintcolordata[i*5] = r;
		us_colormixlayerprintcolordata[i*5+1] = g;
		us_colormixlayerprintcolordata[i*5+2] = b;
	} else
	{
		if (r == us_colormixredmap[us_colormixindex[us_colormixcurindex]] &&
			g == us_colormixgreenmap[us_colormixindex[us_colormixcurindex]] &&
			b == us_colormixbluemap[us_colormixindex[us_colormixcurindex]]) return(FALSE);
		us_colormixredmap[us_colormixindex[us_colormixcurindex]] = r;
		us_colormixgreenmap[us_colormixindex[us_colormixcurindex]] = g;
		us_colormixbluemap[us_colormixindex[us_colormixcurindex]] = b;
	}
	return(TRUE);
}

/*
 * Routine to rebuild the transparent layer interactions based on the single-transparent layer
 * colors.  All combinations of more than 1 transparent layer are derived automatically.
 */
void us_colormixsmoothcolors(void)
{
	INTBIG funct[5], height[5], bit[5], red[5], green[5], blue[5], fun,
		r, g, b, i, j;
	BOOLEAN gotcol;
	CHAR *lname[5], *name;

	/* get information about the current transparent layers */
	for(i=0; i<5; i++) funct[i] = -1;
	for(i=0; i<el_curtech->layercount; i++)
	{
		fun = layerfunction(el_curtech, i);
		name = layername(el_curtech, i);
		if ((fun&LFTRANS1) != 0 && funct[0] < 0) { funct[0] = fun;  lname[0] = name; }
		if ((fun&LFTRANS2) != 0 && funct[1] < 0) { funct[1] = fun;  lname[1] = name; }
		if ((fun&LFTRANS3) != 0 && funct[2] < 0) { funct[2] = fun;  lname[2] = name; }
		if ((fun&LFTRANS4) != 0 && funct[3] < 0) { funct[3] = fun;  lname[3] = name; }
		if ((fun&LFTRANS5) != 0 && funct[4] < 0) { funct[4] = fun;  lname[4] = name; }
	}
	bit[0] = LAYERT1;
	bit[1] = LAYERT2;
	bit[2] = LAYERT3;
	bit[3] = LAYERT4;
	bit[4] = LAYERT5;
	for(i=0; i<5; i++)
		height[i] = layerfunctionheight(funct[i]);

	/* sort the layers by height */
	j = 0;
	while (j == 0)
	{
		j = 1;
		for(i=1; i<5; i++)
		{
			if (height[i] <= height[i-1]) continue;
			fun = height[i];  height[i] = height[i-1];  height[i-1] = fun;
			fun = funct[i];   funct[i] = funct[i-1];    funct[i-1] = fun;
			fun = bit[i];     bit[i] = bit[i-1];        bit[i-1] = fun;
			j = 0;
		}
	}
	for(i=0; i<5; i++)
	{
		red[i] = us_colormixredmap[bit[i]];
		green[i] = us_colormixgreenmap[bit[i]];
		blue[i] = us_colormixbluemap[bit[i]];
	}

	/* now reconstruct the colors */
	for(i=0; i<256; i++)
	{
		if ((i&(LAYERH|LAYEROE|LAYERG)) != 0) continue;
		gotcol = FALSE;
		for(j=0; j<5; j++)
		{
			if ((i&bit[j]) == 0) continue;
			if (!gotcol)
			{
				r = red[j];   g = green[j];   b = blue[j];
				gotcol = TRUE;
			} else
			{
				us_colormixmergecolor(r, g, b, red[j], green[j], blue[j], &r, &g, &b);
			}
		}
		if (gotcol)
		{
			us_colormixredmap[i] = r;
			us_colormixgreenmap[i] = g;
			us_colormixbluemap[i] = b;
		}
	}
}

/*
 * Helper routine for "us_colormixsmoothcolors()" to do a vector-add of colors (r1,g1,b1)
 * and (r2,g2,b2) to produce the new color (ro,go,bo).
 */
void us_colormixmergecolor(INTBIG r1, INTBIG g1, INTBIG b1, INTBIG r2, INTBIG g2, INTBIG b2,
	INTBIG *ro, INTBIG *go, INTBIG *bo)
{
	float f1[3], f2[3], f3[3];

	f1[0] = r1 / 255.0f;
	f1[1] = g1 / 255.0f;
	f1[2] = b1 / 255.0f;
	vectornormalize3d(f1);
	vectormultiply3d(f1, 1.0f, f1);

	f2[0] = r2 / 255.0f;
	f2[1] = g2 / 255.0f;
	f2[2] = b2 / 255.0f;
	vectornormalize3d(f2);
	vectormultiply3d(f2, 0.5f, f2);

	vectoradd3d(f1, f2, f3);
	vectornormalize3d(f3);

	*ro = (INTBIG)(f3[0] * 255.0);
	*go = (INTBIG)(f3[1] * 255.0);
	*bo = (INTBIG)(f3[2] * 255.0);
}

/*
 * Routine to draw the hue/saturation wheel and the intensity slider
 */
void us_colormixdrawpalette(RECTAREA *ra, void *dia)
{
	Q_UNUSED( ra );
	REGISTER INTBIG theta, inc, rr, cx, cy, i, col, spacing;
	INTBIG r, g, b, x[4], y[4];
	RECTAREA rect, stripe;
	float intermed, intertwo;

	/* draw hue/saturation wheel */
	DiaItemRect(dia, DCLR_HUESAT, &rect);
	cx = (rect.left + rect.right) / 2;
	cy = (rect.top + rect.bottom) / 2;
	for(theta=0; theta<360; theta+=60)
	{
		for(inc=0; inc<60; inc+=20)
			for(rr = 0; rr < WHEELRADIUS/2; rr += WHEELRADIUS/8)
		{
			intermed = (float) theta + inc;
			intertwo = (float) rr + WHEELRADIUS/8;
			x[0] = (INTBIG)(((float)rr) * cos(intermed*DEGTORAD)) + cx;
			y[0] = (INTBIG)(((float)rr) * sin(intermed*DEGTORAD)) + cy;
			x[1] = (INTBIG)(intertwo * cos(intermed*DEGTORAD)) + cx;
			y[1] = (INTBIG)(intertwo * sin(intermed*DEGTORAD)) + cy;
			intermed += 21.0;
			x[2] = (INTBIG)(intertwo * cos(intermed*DEGTORAD)) + cx;
			y[2] = (INTBIG)(intertwo * sin(intermed*DEGTORAD)) + cy;
			x[3] = (INTBIG)(((float)rr) * cos(intermed*DEGTORAD)) + cx;
			y[3] = (INTBIG)(((float)rr) * sin(intermed*DEGTORAD)) + cy;
			us_hsvtorgb(((float)(theta+inc+10)) / 360.0f,
				((float)rr+WHEELRADIUS/16) / WHEELRADIUS, 1.0f, &r, &g, &b);
			DiaFillPoly(dia, DCLR_HUESAT, x, y, 4, r, g, b);
		}
		for(inc=0; inc<60; inc+=10)
			for(rr = WHEELRADIUS/8; rr < WHEELRADIUS; rr += WHEELRADIUS/8)
		{
			intermed = (float) theta + inc;
			intertwo = (float) rr + WHEELRADIUS/8;
			x[0] = (INTBIG)(((float)rr)* cos(intermed*DEGTORAD)) + cx;
			y[0] = (INTBIG)(((float)rr)* sin(intermed*DEGTORAD)) + cy;
			x[1] = (INTBIG)(intertwo * cos(intermed*DEGTORAD)) + cx;
			y[1] = (INTBIG)(intertwo * sin(intermed*DEGTORAD)) + cy;
			intermed += 12.0;
			x[2] = (INTBIG)(intertwo * cos(intermed*DEGTORAD)) + cx;
			y[2] = (INTBIG)(intertwo * sin(intermed*DEGTORAD)) + cy;
			x[3] = (INTBIG)(((float)rr) * cos(intermed*DEGTORAD)) + cx;
			y[3] = (INTBIG)(((float)rr) * sin(intermed*DEGTORAD)) + cy;
			us_hsvtorgb(((float)(theta+inc+5)) / 360.0f,
				((float)rr+WHEELRADIUS/16) / WHEELRADIUS, 1.0f, &r, &g, &b);
			DiaFillPoly(dia, DCLR_HUESAT, x, y, 4, r, g, b);
		}
	}

	/* draw the intensity slider */
	DiaItemRect(dia, DCLR_INTENSITY, &rect);
	rect.left--;   rect.right++;
	rect.top--;    rect.bottom++;
	DiaFrameRect(dia, DCLR_INTENSITY, &rect);
	rect.left++;   rect.right--;
	rect.top++;    rect.bottom--;
	spacing = (rect.bottom - rect.top) / INTENSITYDIVISIONS;
	for(i=0; i<INTENSITYDIVISIONS; i++)
	{
		stripe.left = rect.left;
		stripe.right = rect.right;
		stripe.top = (INTSML)(rect.bottom - (i+1)*spacing);
		stripe.bottom = (INTSML)(rect.bottom - i*spacing);
		col = i * 255 / INTENSITYDIVISIONS;
		DiaDrawRect(dia, DCLR_INTENSITY, &stripe, col, col, col);
	}

	/* draw the current color marker */
	us_colormixreload(dia);
	us_colormixtogglemarker(dia);
}

/*
 * Routine to draw an "x" on the hue/saturation wheel and the intensity slider
 * to indicate the current color.
 */
void us_colormixtogglemarker(void *dia)
{
	REGISTER INTBIG r, g, b, cx, cy, x, y, i, index;
	RECTAREA rect;

	/* get the current color */
	if (us_colormixcurbank >= 7)
	{
		i = (us_colormixcurbank-7)*16+us_colormixcurindex;
		r = us_colormixlayerprintcolordata[i*5];
		g = us_colormixlayerprintcolordata[i*5+1];
		b = us_colormixlayerprintcolordata[i*5+2];
	} else
	{
		index = us_colormixindex[us_colormixcurindex];
		r = us_colormixredmap[index];
		g = us_colormixgreenmap[index];
		b = us_colormixbluemap[index];
	}
	us_rgbtohsv(r, g, b, &us_colormixtheta, &us_colormixr, &us_colormixinten);
	us_colormixtheta *= 360.0;
	us_colormixr *= WHEELEDGE;

	/* show the position in the hue/saturation area */
	DiaItemRect(dia, DCLR_HUESAT, &rect);
	cx = (rect.left + rect.right) / 2;
	cy = (rect.top + rect.bottom) / 2;
	x = (INTBIG)(us_colormixr * cos(us_colormixtheta * DEGTORAD)) + cx;
	y = (INTBIG)(us_colormixr * sin(us_colormixtheta * DEGTORAD)) + cy;
	DiaDrawLine(dia, DCLR_HUESAT, x-5, y-5, x+5, y+5, DLMODEINVERT);
	DiaDrawLine(dia, DCLR_HUESAT, x-5, y+5, x+5, y-5, DLMODEINVERT);

	/* show the position in the intensity area */
	DiaItemRect(dia, DCLR_INTENSITY, &rect);
	cx = (rect.left + rect.right) / 2;
	y = rect.bottom - (INTBIG)(us_colormixinten * (float)(rect.bottom - rect.top));
	DiaDrawLine(dia, DCLR_INTENSITY, cx-5, y-5, cx+5, y+5, DLMODEINVERT);
	DiaDrawLine(dia, DCLR_INTENSITY, cx-5, y+5, cx+5, y-5, DLMODEINVERT);
}

/*
 * Routine to reload the 16 color entries according to the selected palette set.
 */
void us_colormixreload(void *dia)
{
	INTBIG i;

	if (us_colormixcurbank >= 7)
	{
		DiaUnDimItem(dia, DCLR_OPACITYVAL);
		DiaUnDimItem(dia, DCLR_OPACITYVAL_L);
		DiaUnDimItem(dia, DCLR_FOREGROUND);
		DiaUnDimItem(dia, DCLR_FOREGROUND_L);
		us_colormixingprint = TRUE;
		for(i=0; i<16; i++)
		{
			us_colormixindex[i] = (us_colormixcurbank-7)*16+i;
			if (us_colormixindex[i] >= us_colormixlayercount)
			{
				DiaSetText(dia, i*2+DCLR_FIRSTBUTTON, "");
				us_colormixindex[i] = -1;
			} else
			{
				DiaSetText(dia, i*2+DCLR_FIRSTBUTTON,
					us_colormixlayernames[us_colormixindex[i]]);
			}
			if (i != us_colormixcurindex)
				us_colormixdrawsquare(i, dia);
		}
		us_colormixdrawsquare(us_colormixcurindex, dia);
		return;
	}
	us_colormixingprint = FALSE;
	DiaDimItem(dia, DCLR_OPACITYVAL);
	DiaDimItem(dia, DCLR_OPACITYVAL_L);
	DiaDimItem(dia, DCLR_FOREGROUND);
	DiaDimItem(dia, DCLR_FOREGROUND_L);
	switch (us_colormixcurbank)
	{
		case 0:		/* all transparent layers */
			us_colormixindex[0]  = ALLOFF;
			us_colormixindex[1]  = LAYERT1;
			us_colormixindex[2]  = LAYERT2;
			us_colormixindex[3]  = LAYERT3;
			us_colormixindex[4]  = LAYERT4;
			us_colormixindex[5]  = LAYERT5;
			us_colormixindex[6]  = LAYERT1 | LAYERT2;
			us_colormixindex[7]  = LAYERT1 | LAYERT3;
			us_colormixindex[8]  = LAYERT1 | LAYERT4;
			us_colormixindex[9]  = LAYERT1 | LAYERT5;
			us_colormixindex[10] = LAYERT2 | LAYERT3;
			us_colormixindex[11] = LAYERT2 | LAYERT4;
			us_colormixindex[12] = LAYERT2 | LAYERT5;
			us_colormixindex[13] = LAYERT3 | LAYERT4;
			us_colormixindex[14] = LAYERT3 | LAYERT5;
			us_colormixindex[15] = LAYERT4 | LAYERT5;
			break;
		case 1:		/* special colors */
			us_colormixindex[0]  = ALLOFF;
			us_colormixindex[1]  = GRID;
			us_colormixindex[2]  = HIGHLIT;
			us_colormixindex[3]  = el_colcelltxt;
			us_colormixindex[4]  = el_colcell;
			us_colormixindex[5]  = el_colwinbor;
			us_colormixindex[6]  = el_colhwinbor;
			us_colormixindex[7]  = el_colmenbor;
			us_colormixindex[8]  = el_colhmenbor;
			us_colormixindex[9]  = el_colmentxt;
			us_colormixindex[10] = el_colmengly;
			us_colormixindex[11] = el_colcursor;
			us_colormixindex[12] = 0354;
			us_colormixindex[13] = 0364;
			us_colormixindex[14] = 0374;
			us_colormixindex[15] = ALLOFF;
			break;
		case 2:		/* the first transparent layer */
			us_colormixbuildindex(LAYERT1, LAYERT2, LAYERT3, LAYERT4, LAYERT5);
			break;
		case 3:		/* the second transparent layer */
			us_colormixbuildindex(LAYERT2, LAYERT1, LAYERT3, LAYERT4, LAYERT5);
			break;
		case 4:		/* the third transparent layer */
			us_colormixbuildindex(LAYERT3, LAYERT1, LAYERT2, LAYERT4, LAYERT5);
			break;
		case 5:		/* the fourth transparent layer */
			us_colormixbuildindex(LAYERT4, LAYERT1, LAYERT2, LAYERT3, LAYERT5);
			break;
		case 6:		/* the fifth transparent layer */
			us_colormixbuildindex(LAYERT5, LAYERT1, LAYERT2, LAYERT3, LAYERT4);
			break;
	}
	for(i=0; i<16; i++)
	{
		DiaSetText(dia, i*2+DCLR_FIRSTBUTTON,
			us_colormixentryname(us_colormixnames, us_colormixindex[i]));
		if (i != us_colormixcurindex)
			us_colormixdrawsquare(i, dia);
	}
	us_colormixdrawsquare(us_colormixcurindex, dia);
}

/*
 * Routine to build the entries desired given the five layers "layer1-5".
 */
void us_colormixbuildindex(INTBIG layer1, INTBIG layer2, INTBIG layer3, INTBIG layer4, INTBIG layer5)
{
	us_colormixindex[0]  = layer1;
	us_colormixindex[1]  = layer1 | layer2;
	us_colormixindex[2]  = layer1 |          layer3;
	us_colormixindex[3]  = layer1 | layer2 | layer3;
	us_colormixindex[4]  = layer1 |                   layer4;
	us_colormixindex[5]  = layer1 | layer2 |          layer4;
	us_colormixindex[6]  = layer1 |          layer3 | layer4;
	us_colormixindex[7]  = layer1 | layer2 | layer3 | layer4;
	us_colormixindex[8]  = layer1 |                            layer5;
	us_colormixindex[9]  = layer1 | layer2 |                   layer5;
	us_colormixindex[10] = layer1 |          layer3 |          layer5;
	us_colormixindex[11] = layer1 | layer2 | layer3 |          layer5;
	us_colormixindex[12] = layer1 |                   layer4 | layer5;
	us_colormixindex[13] = layer1 | layer2 |          layer4 | layer5;
	us_colormixindex[14] = layer1 |          layer3 | layer4 | layer5;
	us_colormixindex[15] = layer1 | layer2 | layer3 | layer4 | layer5;
}

/*
 * Routine to redraw color square "sindex" from the current settings.
 */
void us_colormixdrawsquare(INTBIG sindex, void *dia)
{
	RECTAREA rect;
	REGISTER INTBIG r, g, b, i, index;
	CHAR line[50];

	DiaItemRect(dia, sindex*2+DCLR_FIRSTPATCH, &rect);
	rect.left--;   rect.right++;
	rect.top--;    rect.bottom++;
	DiaFrameRect(dia, sindex*2+DCLR_FIRSTPATCH, &rect);
	rect.left++;   rect.right--;
	rect.top++;    rect.bottom--;

	if (us_colormixcurbank >= 7)
	{
		i = (us_colormixcurbank-7)*16+sindex;
		if (i >= us_colormixlayercount)
		{
			r = g = b = 255;
		} else
		{
			r = us_colormixlayerprintcolordata[i*5];
			g = us_colormixlayerprintcolordata[i*5+1];
			b = us_colormixlayerprintcolordata[i*5+2];
			estrcpy(line, frtoa(us_colormixlayerprintcolordata[i*5+3]));
			DiaSetText(dia, DCLR_OPACITYVAL, line);
			if (us_colormixlayerprintcolordata[i*5+4] != 0) DiaSetPopupEntry(dia, DCLR_FOREGROUND, 1); else
				DiaSetPopupEntry(dia, DCLR_FOREGROUND, 0);
		}
	} else
	{
		index = us_colormixindex[sindex];
		r = us_colormixredmap[index];
		g = us_colormixgreenmap[index];
		b = us_colormixbluemap[index];
	}
	DiaDrawRect(dia, sindex*2+DCLR_FIRSTPATCH, &rect, r, g, b);
	esnprintf(line, 50, x_("%ld"), r);
	DiaSetText(dia, DCLR_REDVAL, line);
	esnprintf(line, 50, x_("%ld"), g);
	DiaSetText(dia, DCLR_GREENVAL, line);
	esnprintf(line, 50, x_("%ld"), b);
	DiaSetText(dia, DCLR_BLUEVAL, line);
}

/*
 * Routine to construct the name of color map entry "ind", given the transparent layer names in "overlayernames".
 */
CHAR *us_colormixentryname(CHAR *overlayernames[], INTBIG ind)
{
	BOOLEAN gotname;
	REGISTER void *infstr;

	if (ind == ALLOFF) return(_("Background"));
	if (ind == GRID) return(_("Grid"));
	if (ind == HIGHLIT) return(_("Highlight"));
	if (ind == 0354) return(_("Extra 1"));
	if (ind == 0364) return(_("Extra 2"));
	if (ind == 0374) return(_("Extra 3"));
	if (ind == el_colcelltxt) return(_("Cell Name"));
	if (ind == el_colcell) return(_("Cell Outline"));
	if (ind == el_colwinbor) return(_("Window Border"));
	if (ind == el_colhwinbor) return(_("Current Window Border"));
	if (ind == el_colmenbor) return(_("Component Menu Border"));
	if (ind == el_colhmenbor) return(_("Highlighted Component Menu Border"));
	if (ind == el_colmentxt) return(_("Text in Component Menu"));
	if (ind == el_colmengly) return(_("Glyphs in Component Menu"));
	if (ind == el_colcursor) return(_("Cursor"));
	infstr = initinfstr();
	gotname = FALSE;
	if ((ind&LAYERT1) != 0)
	{
		if (gotname) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, overlayernames[0]);
		gotname = TRUE;
	}
	if ((ind&LAYERT2) != 0)
	{
		if (gotname) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, overlayernames[1]);
		gotname = TRUE;
	}
	if ((ind&LAYERT3) != 0)
	{
		if (gotname) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, overlayernames[2]);
		gotname = TRUE;
	}
	if ((ind&LAYERT4) != 0)
	{
		if (gotname) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, overlayernames[3]);
		gotname = TRUE;
	}
	if ((ind&LAYERT5) != 0)
	{
		if (gotname) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, overlayernames[4]);
	}
	return(returninfstr(infstr));
}

/****************************** COMPONENT MENU DIALOG ******************************/

/* Component Menu */
static DIALOGITEM us_menuposdialogitems[] =
{
 /*  1 */ {0, {128,168,152,232}, BUTTON, N_("OK")},
 /*  2 */ {0, {88,168,112,232}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,200,48,248}, EDITTEXT, x_("")},
 /*  4 */ {0, {64,8,80,136}, RADIO, N_("Menu at Top")},
 /*  5 */ {0, {88,8,104,136}, RADIO, N_("Menu at Bottom")},
 /*  6 */ {0, {112,8,128,136}, RADIO, N_("Menu on Left")},
 /*  7 */ {0, {136,8,152,136}, RADIO, N_("Menu on Right")},
 /*  8 */ {0, {8,8,24,197}, MESSAGE, N_("Number of Entries Across:")},
 /*  9 */ {0, {32,8,48,197}, MESSAGE, N_("Number of Entries Down:")},
 /* 10 */ {0, {8,200,24,248}, EDITTEXT, x_("")},
 /* 11 */ {0, {160,8,176,100}, RADIO, N_("No Menu")}
};
static DIALOG us_menuposdialog = {{50,75,235,334}, N_("Component Menu Configuration"), 0, 11, us_menuposdialogitems, 0, 0};

/* special items for the "menu" dialog: */
#define DCMP_DOWN      3		/* Down (edit text) */
#define DCMP_ATTOP     4		/* Menus at top (radio) */
#define DCMP_ATBOT     5		/* Menus at bottom (radio) */
#define DCMP_ATLEFT    6		/* Menus at left (radio) */
#define DCMP_ATRIGHT   7		/* Menus at right (radio) */
#define DCMP_ACROSS_L  8		/* Across label (message) */
#define DCMP_DOWN_L    9		/* Down label (message) */
#define DCMP_ACROSS   10		/* Across (edit text) */
#define DCMP_NOMENU   11		/* No menu (radio) */

INTBIG us_menudlog(CHAR *paramstart[])
{
	INTBIG itemHit, large, smallf;
	CHAR amt[10];
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the menu position dialog box */
	dia = DiaInitDialog(&us_menuposdialog);
	if (dia == 0) return(0);
	if ((us_tool->toolstate&MENUON) == 0)
	{
		DiaSetControl(dia, DCMP_NOMENU, 1);
		DiaNoEditControl(dia, DCMP_DOWN);
		DiaNoEditControl(dia, DCMP_ACROSS);
		DiaDimItem(dia, DCMP_ACROSS_L);
		DiaDimItem(dia, DCMP_DOWN_L);
	} else
	{
		switch (us_menupos)
		{
			case 0: DiaSetControl(dia, DCMP_ATTOP, 1);    break;
			case 1: DiaSetControl(dia, DCMP_ATBOT, 1);    break;
			case 2: DiaSetControl(dia, DCMP_ATLEFT, 1);   break;
			case 3: DiaSetControl(dia, DCMP_ATRIGHT, 1);  break;
		}
		DiaEditControl(dia, DCMP_DOWN);
		DiaEditControl(dia, DCMP_ACROSS);
		DiaUnDimItem(dia, DCMP_ACROSS_L);
		DiaUnDimItem(dia, DCMP_DOWN_L);
	}
	if (us_menux < us_menuy) { large = us_menuy;   smallf = us_menux; } else
		{ large = us_menux;   smallf = us_menuy; }
	if (us_menupos <= 1)
	{
		(void)esnprintf(amt, 10, x_("%ld"), large);
		DiaSetText(dia, DCMP_ACROSS, amt);
		(void)esnprintf(amt, 10, x_("%ld"), smallf);
		DiaSetText(dia, DCMP_DOWN, amt);
	} else
	{
		(void)esnprintf(amt, 10, x_("%ld"), smallf);
		DiaSetText(dia, DCMP_ACROSS, amt);
		(void)esnprintf(amt, 10, x_("%ld"), large);
		DiaSetText(dia, DCMP_DOWN, amt);
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK && DiaValidEntry(dia, DCMP_ACROSS) && DiaValidEntry(dia, DCMP_DOWN)) break;
		if (itemHit == DCMP_ATTOP || itemHit == DCMP_ATBOT ||
			itemHit == DCMP_ATLEFT || itemHit == DCMP_ATRIGHT ||
			itemHit == DCMP_NOMENU)
		{
			DiaSetControl(dia, DCMP_ATTOP, 0);
			DiaSetControl(dia, DCMP_ATBOT, 0);
			DiaSetControl(dia, DCMP_ATLEFT, 0);
			DiaSetControl(dia, DCMP_ATRIGHT, 0);
			DiaSetControl(dia, DCMP_NOMENU, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DCMP_NOMENU)
			{
				DiaNoEditControl(dia, DCMP_DOWN);
				DiaNoEditControl(dia, DCMP_ACROSS);
				DiaDimItem(dia, DCMP_ACROSS_L);
				DiaDimItem(dia, DCMP_DOWN_L);
			} else
			{
				DiaEditControl(dia, DCMP_DOWN);
				DiaEditControl(dia, DCMP_ACROSS);
				DiaUnDimItem(dia, DCMP_ACROSS_L);
				DiaUnDimItem(dia, DCMP_DOWN_L);
			}
		}
	}

	paramstart[0] = x_("");
	if (itemHit != CANCEL)
	{
		infstr = initinfstr();
		if (DiaGetControl(dia, DCMP_NOMENU) != 0) addstringtoinfstr(infstr, x_("off")); else
		{
			if (DiaGetControl(dia, DCMP_ATTOP) != 0) addstringtoinfstr(infstr, x_("top"));
			if (DiaGetControl(dia, DCMP_ATBOT) != 0) addstringtoinfstr(infstr, x_("bottom"));
			if (DiaGetControl(dia, DCMP_ATLEFT) != 0) addstringtoinfstr(infstr, x_("left"));
			if (DiaGetControl(dia, DCMP_ATRIGHT) != 0) addstringtoinfstr(infstr, x_("right"));
			addstringtoinfstr(infstr, x_(" size "));
			addstringtoinfstr(infstr, DiaGetText(dia, DCMP_ACROSS));
			addtoinfstr(infstr, ' ');
			addstringtoinfstr(infstr, DiaGetText(dia, DCMP_DOWN));
		}
		if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
		allocstring(&us_returneddialogstring, returninfstr(infstr), us_tool->cluster);
		paramstart[0] = us_returneddialogstring;
	}
	DiaDoneDialog(dia);
	return(1);
}

/****************************** CROSS-LIBRARY COPY DIALOG ******************************/

/* Copy between libraries */
static DIALOGITEM us_copycelldialogitems[] =
{
 /*  1 */ {0, {276,172,300,244}, BUTTON, N_("Done")},
 /*  2 */ {0, {8,8,24,168}, POPUP, x_("")},
 /*  3 */ {0, {276,40,300,112}, BUTTON, N_("<< Copy")},
 /*  4 */ {0, {276,296,300,368}, BUTTON, N_("Copy >>")},
 /*  5 */ {0, {32,8,264,172}, SCROLL, x_("")},
 /*  6 */ {0, {8,244,24,408}, POPUP, x_("")},
 /*  7 */ {0, {324,12,340,192}, BUTTON, N_("Examine contents")},
 /*  8 */ {0, {408,8,424,408}, MESSAGE, x_("")},
 /*  9 */ {0, {348,12,364,192}, BUTTON, N_("Examine contents quietly")},
 /* 10 */ {0, {312,248,328,408}, CHECK, N_("Delete after copy")},
 /* 11 */ {0, {384,248,400,408}, CHECK, N_("Copy related views")},
 /* 12 */ {0, {336,248,352,408}, CHECK, N_("Copy subcells")},
 /* 13 */ {0, {32,244,264,408}, SCROLL, x_("")},
 /* 14 */ {0, {32,172,264,244}, SCROLL, x_("")},
 /* 15 */ {0, {372,12,388,192}, BUTTON, N_("List differences")},
 /* 16 */ {0, {360,248,376,408}, CHECK, N_("Use existing subcells")}
};
static DIALOG us_copycelldialog = {{50,75,483,493}, N_("Cross-Library Copy"), 0, 16, us_copycelldialogitems, 0, 0};

static void us_loadlibrarycells(LIBRARY *lib, LIBRARY *otherlib, INTBIG examinecontents, BOOLEAN report, void *dia);
static BOOLEAN us_cellexists(NODEPROTO *cell);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif
	static int us_sortxlibentryascending(const void *e1, const void *e2);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

/* special items for the "copycell" dialog: */
#define DCPF_FIRSTLIB       2	/* First library (popup) */
#define DCPF_COPYLEFT       3	/* << Copy (button) */
#define DCPF_COPYRIGHT      4	/* Copy >> (button) */
#define DCPF_CELL1LIST      5	/* Cell 1 list (scroll list) */
#define DCPF_OTHERLIB       6	/* Other library (popup) */
#define DCPF_EXCONTENTS     7	/* Examine contents (button) */
#define DCPF_CONTINFO       8	/* contents footnote (stat text) */
#define DCPF_EXCONTENTSQ    9	/* Examine contents quietly (button) */
#define DCPF_DELETEAFTER   10	/* Delete after copy (check) */
#define DCPF_COPYRELATED   11	/* Copy related views (check) */
#define DCPF_COPYSUBCELL   12	/* Copy subcells (check) */
#define DCPF_CELL2LIST     13	/* Cell 2 list (scroll list) */
#define DCPF_CELLDIFFLIST  14	/* Cell difference list (scroll list) */
#define DCPF_LISTDIFFS     15	/* List differences (button) */
#define DCPF_USEEXISTING   16	/* Use existing subcells (check) */

INTBIG us_copycelldlog(CHAR *prompt)
{
	INTBIG itemHit, libcount, i, len, libindex, otherlibindex, listlen, examinecontents;
	CHAR *lastcellname, *param0, *param1, *newparam[10];
	REGISTER INTBIG ac;
	REGISTER LIBRARY *lib, *otherlib, *olib;
	REGISTER CHAR **liblist, *pt;
	REGISTER WINDOWPART *w;
	static LIBRARY *lastlib = NOLIBRARY, *lastotherlib = NOLIBRARY;
	static INTBIG lastdeleteaftercopy = 0;
	static INTBIG lastcopyrelated = 1;
	static INTBIG lastcopysubcells = 1;
	static INTBIG lastuseexisting = 0;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* see how many libraries should be in the popups */
	libcount = 0;
	lib = el_curlib;   otherlib = NOLIBRARY;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		if ((olib->userbits&HIDDENLIBRARY) != 0) continue;
		libcount++;
		if (olib != lib && otherlib == NOLIBRARY) otherlib = olib;
	}
	if (libcount < 2)
	{
		ttyputerr(_("There must be two libraries read in before copying between them"));
		return(0);
	}

	/* use libraries from previous invocation of this dialog */
	if (lastlib != NOLIBRARY)
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			if (olib == lastlib) lib = lastlib;
	}
	if (lastotherlib != NOLIBRARY)
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			if (olib == lastotherlib) otherlib = lastotherlib;
	}

	/* see if the initial library is specified */
	if (*prompt == '!')
	{
		olib = getlibrary(&prompt[1]);
		if (olib != NOLIBRARY && olib != lib)
			otherlib = olib;
	}

	/* load up list of libraries */
	liblist = us_makelibrarylist(&libcount, el_curlib, &i);

	/* find requested libraries */
	for(i=0; i<libcount; i++)
	{
		if (namesame(liblist[i], lib->libname) == 0) libindex = i;
		if (namesame(liblist[i], otherlib->libname) == 0) otherlibindex = i;
	}

	/* display the copycell dialog box */
	dia = DiaInitDialog(&us_copycelldialog);
	if (dia == 0)
	{
		efree((CHAR *)liblist);
		return(0);
	}
	DiaSetPopup(dia, DCPF_FIRSTLIB, libcount, liblist);
	DiaSetPopupEntry(dia, DCPF_FIRSTLIB, libindex);
	DiaSetPopup(dia, DCPF_OTHERLIB, libcount, liblist);
	DiaSetPopupEntry(dia, DCPF_OTHERLIB, otherlibindex);
	DiaUnDimItem(dia, DCPF_EXCONTENTS);
	DiaUnDimItem(dia, DCPF_EXCONTENTSQ);
	DiaSetControl(dia, DCPF_COPYRELATED, lastcopyrelated);
	DiaSetControl(dia, DCPF_COPYSUBCELL, lastcopysubcells);
	DiaSetControl(dia, DCPF_DELETEAFTER, lastdeleteaftercopy);
	DiaSetControl(dia, DCPF_USEEXISTING, lastuseexisting);
	if (lastdeleteaftercopy == 0)
	{
		DiaSetText(dia, DCPF_COPYLEFT, _("<< Copy"));
		DiaSetText(dia, DCPF_COPYRIGHT, _("Copy >>"));
	} else
	{
		DiaSetText(dia, DCPF_COPYLEFT, _("<< Move"));
		DiaSetText(dia, DCPF_COPYRIGHT, _("Move >>"));
	}
	DiaInitTextDialog(dia, DCPF_CELL1LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCREPORT|SCSELMOUSE|SCDOUBLEQUIT|SCSMALLFONT|SCFIXEDWIDTH|SCHORIZBAR);
	DiaInitTextDialog(dia, DCPF_CELL2LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCREPORT|SCSELMOUSE|SCDOUBLEQUIT|SCSMALLFONT|SCFIXEDWIDTH|SCHORIZBAR);
	DiaInitTextDialog(dia, DCPF_CELLDIFFLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCREPORT|SCSELMOUSE|SCSMALLFONT|SCFIXEDWIDTH|SCHORIZBAR);
	DiaSynchVScrolls(dia, DCPF_CELL1LIST, DCPF_CELL2LIST, DCPF_CELLDIFFLIST);
	examinecontents = 0;
	us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DCPF_LISTDIFFS)
		{
			us_loadlibrarycells(lib, otherlib, examinecontents, TRUE, dia);
			continue;
		}
		if (itemHit == DCPF_CELL1LIST)
		{
			i = DiaGetCurLine(dia, DCPF_CELL1LIST);
			DiaSelectLine(dia, DCPF_CELL2LIST, i);
			DiaSelectLine(dia, DCPF_CELLDIFFLIST, i);
			continue;
		}
		if (itemHit == DCPF_CELL2LIST)
		{
			i = DiaGetCurLine(dia, DCPF_CELL2LIST);
			DiaSelectLine(dia, DCPF_CELL1LIST, i);
			DiaSelectLine(dia, DCPF_CELLDIFFLIST, i);
			continue;
		}
		if (itemHit == DCPF_CELLDIFFLIST)
		{
			i = DiaGetCurLine(dia, DCPF_CELLDIFFLIST);
			DiaSelectLine(dia, DCPF_CELL1LIST, i);
			DiaSelectLine(dia, DCPF_CELL2LIST, i);
			continue;
		}
		if (itemHit == DCPF_DELETEAFTER)
		{
			lastdeleteaftercopy = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, lastdeleteaftercopy);
			if (lastdeleteaftercopy == 0)
			{
				DiaSetText(dia, DCPF_COPYLEFT, _("<< Copy"));
				DiaSetText(dia, DCPF_COPYRIGHT, _("Copy >>"));
			} else
			{
				DiaSetText(dia, DCPF_COPYLEFT, _("<< Move"));
				DiaSetText(dia, DCPF_COPYRIGHT, _("Move >>"));
			}
			continue;
		}
		if (itemHit == DCPF_USEEXISTING)
		{
			lastuseexisting = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, lastuseexisting);
			continue;
		}
		if (itemHit == DCPF_COPYRELATED)
		{
			lastcopyrelated = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, lastcopyrelated);
			continue;
		}
		if (itemHit == DCPF_COPYSUBCELL)
		{
			lastcopysubcells = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, lastcopysubcells);
			continue;
		}
		if (itemHit == DCPF_COPYLEFT)
		{
			/* copy into the current library ("<< Copy/Move") */
			i = DiaGetCurLine(dia, DCPF_CELL2LIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DCPF_CELL2LIST, i);
			if (*pt == 0) continue;
			(void)allocstring(&lastcellname, pt, el_tempcluster);

			infstr = initinfstr();
			addstringtoinfstr(infstr, otherlib->libname);
			addtoinfstr(infstr, ':');
			addstringtoinfstr(infstr, lastcellname);
			(void)allocstring(&param0, returninfstr(infstr), el_tempcluster);
			ac = 0;
			newparam[ac++] = param0;

			infstr = initinfstr();
			addstringtoinfstr(infstr, lib->libname);
			addtoinfstr(infstr, ':');
			for(i=0; lastcellname[i] != 0; i++)
			{
				if (lastcellname[i] == '{' || lastcellname[i] == ';') break;
				addtoinfstr(infstr, lastcellname[i]);
			}
			(void)allocstring(&param1, returninfstr(infstr), el_tempcluster);
			newparam[ac++] = param1;

			/* do the copy */
			if (DiaGetControl(dia, DCPF_DELETEAFTER) != 0) newparam[ac++] = x_("move");
			if (DiaGetControl(dia, DCPF_COPYRELATED) == 0) newparam[ac++] = x_("no-related-views");
			if (DiaGetControl(dia, DCPF_COPYSUBCELL) == 0) newparam[ac++] = x_("no-subcells");
			if (DiaGetControl(dia, DCPF_USEEXISTING) != 0) newparam[ac++] = x_("use-existing-subcells");
			us_copycell(ac, newparam);
			efree(param0);
			efree(param1);

			/* reload the dialog */
			us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);

			/* reselect the last selected line */
			len = estrlen(lastcellname);
			listlen = DiaGetNumScrollLines(dia, DCPF_CELL1LIST);
			for(i=0; i<listlen; i++)
			{
				pt = DiaGetScrollLine(dia, DCPF_CELL1LIST, i);
				if (estrncmp(lastcellname, pt, len) != 0) continue;
				DiaSelectLine(dia, DCPF_CELL1LIST, i);
				DiaSelectLine(dia, DCPF_CELL2LIST, i);
				DiaSelectLine(dia, DCPF_CELLDIFFLIST, i);
				break;
			}
			efree(lastcellname);
			continue;
		}
		if (itemHit == DCPF_COPYRIGHT)
		{
			/* copy out of the current library ("Copy/Move >>") */
			i = DiaGetCurLine(dia, DCPF_CELL1LIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DCPF_CELL1LIST, i);
			if (*pt == 0) continue;
			(void)allocstring(&lastcellname, pt, el_tempcluster);

			infstr = initinfstr();
			addstringtoinfstr(infstr, lib->libname);
			addtoinfstr(infstr, ':');
			addstringtoinfstr(infstr, lastcellname);
			(void)allocstring(&param0, returninfstr(infstr), el_tempcluster);
			ac = 0;
			newparam[ac++] = param0;

			infstr = initinfstr();
			addstringtoinfstr(infstr, otherlib->libname);
			addtoinfstr(infstr, ':');
			for(i=0; lastcellname[i] != 0; i++)
			{
				if (lastcellname[i] == '{' || lastcellname[i] == ';') break;
				addtoinfstr(infstr, lastcellname[i]);
			}
			(void)allocstring(&param1, returninfstr(infstr), el_tempcluster);
			newparam[ac++] = param1;

			/* do the copy/move */
			if (DiaGetControl(dia, DCPF_DELETEAFTER) != 0) newparam[ac++] = x_("move");
			if (DiaGetControl(dia, DCPF_COPYRELATED) == 0) newparam[ac++] = x_("no-related-views");
			if (DiaGetControl(dia, DCPF_COPYSUBCELL) == 0) newparam[ac++] = x_("no-subcells");
			if (DiaGetControl(dia, DCPF_USEEXISTING) != 0) newparam[ac++] = x_("use-existing-subcells");
			us_copycell(ac, newparam);
			efree(param0);
			efree(param1);

			/* reload the dialog */
			us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);

			/* reselect the last selected line */
			len = estrlen(lastcellname);
			listlen = DiaGetNumScrollLines(dia, DCPF_CELL2LIST);
			for(i=0; i<listlen; i++)
			{
				pt = DiaGetScrollLine(dia, DCPF_CELL2LIST, i);
				if (estrncmp(lastcellname, pt, len) != 0) continue;
				DiaSelectLine(dia, DCPF_CELL1LIST, i);
				DiaSelectLine(dia, DCPF_CELL2LIST, i);
				DiaSelectLine(dia, DCPF_CELLDIFFLIST, i);
				break;
			}
			efree(lastcellname);
			continue;
		}
		if (itemHit == DCPF_FIRSTLIB)
		{
			/* selected different left-hand library */
			i = DiaGetPopupEntry(dia, DCPF_FIRSTLIB);
			olib = getlibrary(liblist[i]);
			if (olib == NOLIBRARY) continue;
			lastlib = lib = olib;
			us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);
			continue;
		}
		if (itemHit == DCPF_OTHERLIB)
		{
			/* selected different right-hand library */
			i = DiaGetPopupEntry(dia, DCPF_OTHERLIB);
			olib = getlibrary(liblist[i]);
			if (olib == NOLIBRARY) continue;
			lastotherlib = otherlib = olib;
			us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);
			continue;
		}
		if (itemHit == DCPF_EXCONTENTS)
		{
			/* examine contents */
			examinecontents = 1;
			DiaDimItem(dia, DCPF_EXCONTENTS);
			DiaDimItem(dia, DCPF_EXCONTENTSQ);

			/* reload the dialog */
			i = DiaGetCurLine(dia, DCPF_CELL1LIST);
			us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);
			DiaSelectLine(dia, DCPF_CELL1LIST, i);
			DiaSelectLine(dia, DCPF_CELL2LIST, i);
			DiaSelectLine(dia, DCPF_CELLDIFFLIST, i);
			continue;
		}
		if (itemHit == DCPF_EXCONTENTSQ)
		{
			/* examine contents quietly */
			examinecontents = -1;
			DiaDimItem(dia, DCPF_EXCONTENTS);
			DiaDimItem(dia, DCPF_EXCONTENTSQ);

			/* reload the dialog */
			i = DiaGetCurLine(dia, DCPF_CELL1LIST);
			us_loadlibrarycells(lib, otherlib, examinecontents, FALSE, dia);
			DiaSelectLine(dia, DCPF_CELL1LIST, i);
			DiaSelectLine(dia, DCPF_CELL2LIST, i);
			DiaSelectLine(dia, DCPF_CELLDIFFLIST, i);
			continue;
		}
	}

	DiaDoneDialog(dia);
	efree((CHAR *)liblist);

	/* validate all windows */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (w->curnodeproto == NONODEPROTO) continue;
		if (us_cellexists(w->curnodeproto)) continue;

		/* window no longer valid */
		us_clearwindow(w);
	}
	if (el_curlib->curnodeproto != NONODEPROTO)
	{
		if (!us_cellexists(el_curlib->curnodeproto))
			(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"),
				(INTBIG)NONODEPROTO, VNODEPROTO);
	}

	/* update status display if necessary */
	if (us_curnodeproto != NONODEPROTO && us_curnodeproto->primindex == 0 &&
		!us_cellexists(us_curnodeproto))
	{
		if ((us_state&NONPERSISTENTCURNODE) != 0) us_setnodeproto(NONODEPROTO); else
			us_setnodeproto(el_curtech->firstnodeproto);
	}
	return(0);
}

/*
 * Routine to return true if cell "cell" exists in a library somewhere.
 */
BOOLEAN us_cellexists(NODEPROTO *cell)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np == cell) return(TRUE);
	}
	return(FALSE);
}

typedef struct
{
	NODEPROTO *np;
	CHAR      *cellname;
} XLIBENTRY;

/*
 * Helper routine for "esort" that makes cross-lib entries go in ascending order.
 */
int us_sortxlibentryascending(const void *e1, const void *e2)
{
	REGISTER XLIBENTRY *xl1, *xl2;
	REGISTER CHAR *c1, *c2;

	xl1 = (XLIBENTRY *)e1;
	xl2 = (XLIBENTRY *)e2;
	c1 = xl1->cellname;
	c2 = xl2->cellname;
	return(namesame(c1, c2));
}

/*
 * Routine to compare the two libraries "lib" and "otherlib" and list their cells in
 * the cross-library copy dialog.  If "examinecontents" is nonzero, compare cell
 * contents when their dates don't match (if negative, do so quietly).  If "report" is
 * true, just list the differences in the messages window.
 */
void us_loadlibrarycells(LIBRARY *lib, LIBRARY *otherlib, INTBIG examinecontents,
	BOOLEAN report, void *dia)
{
	REGISTER NODEPROTO *np, *curf = NONODEPROTO, *otherf = NONODEPROTO;
	REGISTER INTBIG i, j, curcount, othercount, curpos, otherpos, op;
	REGISTER CHAR *fname, *ofname, *pt;
	REGISTER LIBRARY *oldlib;
	REGISTER XLIBENTRY *curxl, *otherxl;

	if (!report)
	{
		if (examinecontents != 0) DiaSetText(dia, DCPF_CONTINFO, _("Examining contents..."));
		DiaLoadTextDialog(dia, DCPF_CELL1LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
		DiaLoadTextDialog(dia, DCPF_CELL2LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
		DiaLoadTextDialog(dia, DCPF_CELLDIFFLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
	}

	/* gather a list of cell names in "lib" */
	curcount = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		curcount++;
	if (curcount > 0)
	{
		curxl = (XLIBENTRY *)emalloc(curcount * (sizeof (XLIBENTRY)), el_tempcluster);
		if (curxl == 0) return;
		oldlib = el_curlib;   el_curlib = lib;
		for(i=0, np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto, i++)
		{
			curxl[i].np = np;
			(void)allocstring(&curxl[i].cellname, describenodeproto(np), el_tempcluster);
		}
		el_curlib = oldlib;
	} else curxl = 0;

	/* gather a list of cell names in "otherlib" */
	othercount = 0;
	for(np = otherlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		othercount++;
	if (othercount > 0)
	{
		otherxl = (XLIBENTRY *)emalloc(othercount * (sizeof (XLIBENTRY)), el_tempcluster);
		if (otherxl == 0) return;
		oldlib = el_curlib;   el_curlib = otherlib;
		for(i=0, np = otherlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto, i++)
		{
			otherxl[i].np = np;
			(void)allocstring(&otherxl[i].cellname, describenodeproto(np), el_tempcluster);
		}
		el_curlib = oldlib;
	} else otherxl = 0;

	/* sort the list of names in the current library */
	esort(curxl, curcount, sizeof (XLIBENTRY), us_sortxlibentryascending);
	esort(otherxl, othercount, sizeof (XLIBENTRY), us_sortxlibentryascending);

	/* put out the parallel list of cells in the two libraries */
	curpos = otherpos = 0;
	for(;;)
	{
		if (curpos >= curcount && otherpos >= othercount) break;
		if (curpos >= curcount) op = 2; else
			if (otherpos >= othercount) op = 1; else
		{
			curf = curxl[curpos].np;
			otherf = otherxl[otherpos].np;
			j = namesame(curxl[curpos].cellname, otherxl[otherpos].cellname);
			if (j < 0) op = 1; else
				if (j > 0) op = 2; else
					op = 3;
		}

		if (op != 1 && op != 3) fname = x_(""); else
			fname = curxl[curpos++].cellname;
		if (!report) DiaStuffLine(dia, DCPF_CELL1LIST, fname);

		if (op != 2 && op != 3) ofname = x_(""); else
			ofname = otherxl[otherpos++].cellname;
		if (!report) DiaStuffLine(dia, DCPF_CELL2LIST, ofname);

		if (op == 3)
		{
			if (curf->revisiondate < otherf->revisiondate)
			{
				if (examinecontents != 0)
				{
					if (us_samecontents(curf, otherf, examinecontents))
					{
						pt = _("<-OLD");
						if (report) ttyputmsg(x_("%s:%s OLDER THAN %s:%s (but contents are the same)"),
							lib->libname, fname, otherlib->libname, ofname);
					} else
					{
						pt = _("<-OLD*");
						if (report) ttyputmsg(x_("%s:%s OLDER THAN %s:%s (and contents are different)"),
							lib->libname, fname, otherlib->libname, ofname);
					}
				} else
				{
					pt = _("<-OLD");
					if (report) ttyputmsg(x_("%s:%s OLDER THAN %s:%s"),
						lib->libname, fname, otherlib->libname, ofname);
				}
			} else if (curf->revisiondate > otherf->revisiondate)
			{
				if (examinecontents != 0)
				{
					if (us_samecontents(curf, otherf, examinecontents))
					{
						pt = _("  OLD->");
						if (report) ttyputmsg(x_("%s:%s OLDER THAN %s:%s (but contents are the same)"),
							otherlib->libname, ofname, lib->libname, fname);
					} else
					{
						pt = _(" *OLD->");
						if (report) ttyputmsg(x_("%s:%s OLDER THAN %s:%s (and contents are different)"),
							otherlib->libname, ofname, lib->libname, fname);
					}
				} else
				{
					pt = _("  OLD->");
					if (report) ttyputmsg(x_("%s:%s OLDER THAN %s:%s"),
						otherlib->libname, ofname, lib->libname, fname);
				}
			} else
			{
				pt = _("-EQUAL-");
			}
		} else
		{
			pt = x_("");
		}
		if (!report) DiaStuffLine(dia, DCPF_CELLDIFFLIST, pt);
	}
	if (!report)
	{
		DiaSelectLine(dia, DCPF_CELL1LIST, 0);
		DiaSelectLine(dia, DCPF_CELL2LIST, 0);
		DiaSelectLine(dia, DCPF_CELLDIFFLIST, 0);
	}

	/* clean up */
	for(i=0; i<curcount; i++) efree(curxl[i].cellname);
	if (curcount > 0) 	efree((CHAR *)curxl);
	for(i=0; i<othercount; i++) efree(otherxl[i].cellname);
	if (othercount > 0) efree((CHAR *)otherxl);
	if (!report)
	{
		if (examinecontents != 0) DiaSetText(dia, DCPF_CONTINFO, _("* contents differ"));
	}
}

/****************************** CELL EDIT/CREATE DIALOGS ******************************/

/* Edit cell */
static DIALOGITEM us_editcelldialogitems[] =
{
 /*  1 */ {0, {284,208,308,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {284,16,308,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,208,280}, SCROLL, x_("")},
 /*  4 */ {0, {212,8,228,153}, CHECK, N_("Show old versions")},
 /*  5 */ {0, {260,8,276,231}, CHECK, N_("Make new window for cell")},
 /*  6 */ {0, {284,104,308,187}, BUTTON, N_("New Cell")},
 /*  7 */ {0, {8,8,24,67}, MESSAGE, N_("Library:")},
 /*  8 */ {0, {8,72,24,280}, POPUP, x_("")},
 /*  9 */ {0, {236,8,252,231}, CHECK, N_("Show cells from Cell-Library")},
 /* 10 */ {0, {331,8,347,213}, EDITTEXT, x_("")},
 /* 11 */ {0, {328,216,352,280}, BUTTON, N_("Rename")},
 /* 12 */ {0, {356,144,380,208}, BUTTON, N_("Delete")},
 /* 13 */ {0, {316,8,317,280}, DIVIDELINE, N_("item")},
 /* 14 */ {0, {360,8,376,137}, CHECK, N_("Confirm deletion")}
};
static DIALOG us_editcelldialog = {{50,75,439,365}, N_("Edit Cell"), 0, 14, us_editcelldialogitems, 0, 0};

/* special items for the "edit cell" command: */
#define DEDF_CELLLIST       3		/* Cell list (stat text) */
#define DEDF_SHOWOLDVERS    4		/* Show old versions (check) */
#define DEDF_NEWWINDOW      5		/* Make new window for cell (check) */
#define DEDF_NEWCELL        6		/* New cell (button) */
#define DEDF_LIBRARYNAME    8		/* Library name (popup) */
#define DEDF_INCLCELLLIB    9		/* Show from cell-library (check) */
#define DEDF_NEWNAME       10		/* New cell name (edit text) */
#define DEDF_RENAME        11		/* Rename cell (button) */
#define DEDF_DELETE        12		/* Delete cell (button) */
#define DEDF_CONFIRMDELETE 14		/* Confirm cell deletion (check) */

/* New cell */
static DIALOGITEM us_newcelldialogitems[] =
{
 /*  1 */ {0, {56,304,80,368}, BUTTON, N_("OK")},
 /*  2 */ {0, {56,12,80,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,160,24,367}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,157}, MESSAGE, N_("Name of new cell:")},
 /*  5 */ {0, {32,160,48,367}, POPUP, x_("")},
 /*  6 */ {0, {32,56,48,149}, MESSAGE, N_("Cell view:")},
 /*  7 */ {0, {60,84,78,297}, CHECK, N_("Make new window for cell")}
};
static DIALOG us_newcelldialog = {{350,75,445,455}, N_("New Cell Creation"), 0, 7, us_newcelldialogitems, 0, 0};

/* special items for the "new cell" command: */
#define DNWF_CELLNAME      3		/* Cell name (edit text) */
#define DNWF_VIEWLIST      5		/* View list (popup) */
#define DNWF_NEWWINDOW     7		/* Make new window for cell (check) */

INTBIG us_editcelldlog(CHAR *prompt)
{
	INTBIG itemHit, libcount, i;
	REGISTER INTBIG viewcount, libindex, defaultindex, listlen;
	REGISTER NODEPROTO *np, *curcell;
	static BOOLEAN confirmdelete = TRUE;
	CHAR **viewlist, **librarylist, *pt, *ptr, *newpar[5];
	REGISTER VIEW *v;
	static INTBIG makenewwindow = -1;
	static VIEW *lastview = NOVIEW;
	VIEW *thisview;
	REGISTER void *infstr, *dia, *subdia;

	/* make a list of view names */
	viewcount = us_makeviewlist(&viewlist);
	if (viewcount == 0) return(0);

	/* see what the current view is (if any) */
	thisview = lastview;
	if (el_curwindowpart != NOWINDOWPART)
	{
		if (el_curwindowpart->curnodeproto != NONODEPROTO)
			thisview = el_curwindowpart->curnodeproto->cellview;
	}
	defaultindex = 0;
	if (thisview != NOVIEW)
	{
		for(i=0; i<viewcount; i++)
			if (estrcmp(viewlist[i], thisview->viewname) == 0) defaultindex = i;
	}

	/* the general case: display the dialog box */
	us_curlib = el_curlib;
	for(pt = prompt; *pt != 0; pt++) if (*pt == '/') break;
	if (*pt == '/')
	{
		*pt++ = 0;
		us_curlib = getlibrary(prompt);
		if (us_curlib == NOLIBRARY) us_curlib = el_curlib;
		prompt = pt;
	}

	us_editcelldialog.movable = prompt;
	if (us_curlib->firstnodeproto == NONODEPROTO)
	{
		us_editcelldialogitems[OK-1].type = BUTTON;
		us_editcelldialogitems[DEDF_NEWCELL-1].type = DEFBUTTON;
	} else
	{
		us_editcelldialogitems[OK-1].type = DEFBUTTON;
		us_editcelldialogitems[DEDF_NEWCELL-1].type = BUTTON;
	}
	dia = DiaInitDialog(&us_editcelldialog);
	if (dia == 0) return(0);

	if (confirmdelete) DiaSetControl(dia, DEDF_CONFIRMDELETE, 1);

	/* make a list of library names */
	librarylist = us_makelibrarylist(&libcount, us_curlib, &i);
	libindex = 0;
	for(i=0; i<libcount; i++)
		if (namesame(librarylist[i], us_curlib->libname) == 0) libindex = i;
	DiaSetPopup(dia, DEDF_LIBRARYNAME, libcount, librarylist);
	DiaSetPopupEntry(dia, DEDF_LIBRARYNAME, libindex);

	/* show the cells */
	us_showoldversions = us_defshowoldversions;
	us_showcellibrarycells = us_defshowcellibrarycells;
	us_showonlyrelevantcells = 0;
	DiaInitTextDialog(dia, DEDF_CELLLIST, us_oldcelltopofcells, us_oldcellnextcells,
		DiaNullDlogDone, 0, SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT|SCREPORT);

	/* find the current node and make it the default */
	(void)us_setscrolltocurrentcell(DEDF_CELLLIST, FALSE, TRUE, TRUE, FALSE, dia);
	np = us_getselectedcell(dia);
	if (np != NONODEPROTO)
		DiaSetText(dia, DEDF_NEWNAME, np->protoname);

	/* see if there are any old versions */
	for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->newestversion != np) break;
	if (np == NONODEPROTO) DiaDimItem(dia, DEDF_SHOWOLDVERS); else
	{
		DiaUnDimItem(dia, DEDF_SHOWOLDVERS);
		DiaSetControl(dia, DEDF_SHOWOLDVERS, us_showoldversions);
	}

	/* see if there are any cell-library cells */
	for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if ((np->userbits&INCELLLIBRARY) != 0) break;
	if (np == NONODEPROTO) DiaDimItem(dia, DEDF_INCLCELLLIB); else
	{
		DiaUnDimItem(dia, DEDF_INCLCELLLIB);
		DiaSetControl(dia, DEDF_INCLCELLLIB, us_showcellibrarycells);
	}

	/* if the current window has a cell in it and multiple windows are supported, offer new one */
	curcell = getcurcell();
	if (makenewwindow < 0)
	{
		makenewwindow = 0;
		if (curcell != NONODEPROTO && graphicshas(CANUSEFRAMES)) makenewwindow = 1;
	}
	DiaSetControl(dia, DEDF_NEWWINDOW, makenewwindow);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK)
		{
			np = us_getselectedcell(dia);
			if (np == NONODEPROTO) continue;
			allocstring(&us_returneddialogstring, describenodeproto(np), us_tool->cluster);
			newpar[0] = us_returneddialogstring;
			break;
		}
		if (itemHit == DEDF_CONFIRMDELETE)
		{
			confirmdelete = !confirmdelete;
			if (confirmdelete) DiaSetControl(dia, DEDF_CONFIRMDELETE, 1); else
				DiaSetControl(dia, DEDF_CONFIRMDELETE, 0);
			continue;
		}
		if (itemHit == DEDF_NEWWINDOW)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DEDF_RENAME)
		{
			np = us_getselectedcell(dia);
			if (np == NONODEPROTO) continue;

			pt = DiaGetText(dia, DEDF_NEWNAME);
			if (estrcmp(np->protoname, pt) == 0) continue;

			i = DiaGetCurLine(dia, DEDF_CELLLIST);
			allocstring(&newpar[0], describenodeproto(np), el_tempcluster);
			allocstring(&newpar[1], pt, el_tempcluster);
			newpar[2] = x_("p");
			us_rename(3, newpar);
			efree((CHAR *)newpar[0]);
			efree((CHAR *)newpar[1]);
			DiaSetScrollLine(dia, DEDF_CELLLIST, i, nldescribenodeproto(np));
			continue;
		}
		if (itemHit == DEDF_DELETE)
		{
			np = us_getselectedcell(dia);
			if (np == NONODEPROTO) continue;
			if (confirmdelete)
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("Are you sure you want to delete cell %s?"),
					describenodeproto(np));
				if (us_yesnodlog(returninfstr(infstr), newpar) == 0) continue;
				if (namesame(newpar[0], x_("yes")) != 0) continue;
			}
			allocstring(&us_returneddialogstring, describenodeproto(np), us_tool->cluster);
			newpar[0] = us_returneddialogstring;
			us_killcell(1, newpar);
			efree((CHAR *)us_returneddialogstring);
			i = DiaGetCurLine(dia, DEDF_CELLLIST);
			DiaLoadTextDialog(dia, DEDF_CELLLIST, us_oldcelltopofcells, us_oldcellnextcells,
				DiaNullDlogDone, 0);
			if (*DiaGetScrollLine(dia, DEDF_CELLLIST, i) == 0) i--;
			DiaSelectLine(dia, DEDF_CELLLIST, i);
			continue;
		}
		if (itemHit == DEDF_CELLLIST)
		{
			np = us_getselectedcell(dia);
			if (np != NONODEPROTO)
				DiaSetText(dia, DEDF_NEWNAME, np->protoname);
			continue;
		}
		if (itemHit == DEDF_LIBRARYNAME)
		{
			i = DiaGetPopupEntry(dia, DEDF_LIBRARYNAME);
			us_curlib = getlibrary(librarylist[i]);
			DiaLoadTextDialog(dia, DEDF_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);
			np = us_getselectedcell(dia);
			if (np == NONODEPROTO) DiaSetText(dia, DEDF_NEWNAME, x_("")); else
				DiaSetText(dia, DEDF_NEWNAME, np->protoname);

			/* see if there are any old versions */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (np->newestversion != np) break;
			if (np == NONODEPROTO) DiaDimItem(dia, DEDF_SHOWOLDVERS); else
			{
				DiaUnDimItem(dia, DEDF_SHOWOLDVERS);
				DiaSetControl(dia, DEDF_SHOWOLDVERS, us_showoldversions);
			}

			/* see if there are any cell-library cells */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if ((np->userbits&INCELLLIBRARY) != 0) break;
			if (np == NONODEPROTO) DiaDimItem(dia, DEDF_INCLCELLLIB); else
			{
				DiaUnDimItem(dia, DEDF_INCLCELLLIB);
				DiaSetControl(dia, DEDF_INCLCELLLIB, us_showcellibrarycells);
			}
			continue;
		}
		if (itemHit == DEDF_SHOWOLDVERS)
		{
			us_defshowoldversions = us_showoldversions = 1 - DiaGetControl(dia, DEDF_SHOWOLDVERS);
			DiaSetControl(dia, DEDF_SHOWOLDVERS, us_showoldversions);
			i = DiaGetCurLine(dia, DEDF_CELLLIST);
			if (i >= 0) pt = DiaGetScrollLine(dia, DEDF_CELLLIST, i); else pt = x_("");
			DiaLoadTextDialog(dia, DEDF_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);
			if (*pt != 0)
			{
				listlen = DiaGetNumScrollLines(dia, DEDF_CELLLIST);
				for(i=0; i<listlen; i++)
				{
					ptr = DiaGetScrollLine(dia, DEDF_CELLLIST, i);
					if (estrcmp(ptr, pt) != 0) continue;
					DiaSelectLine(dia, DEDF_CELLLIST, i);
					break;
				}
			}
			continue;
		}
		if (itemHit == DEDF_INCLCELLLIB)
		{
			us_defshowcellibrarycells = us_showcellibrarycells = 1 - DiaGetControl(dia, DEDF_INCLCELLLIB);
			DiaSetControl(dia, DEDF_INCLCELLLIB, us_showcellibrarycells);
			i = DiaGetCurLine(dia, DEDF_CELLLIST);
			if (i >= 0) pt = DiaGetScrollLine(dia, DEDF_CELLLIST, i); else pt = x_("");
			DiaLoadTextDialog(dia, DEDF_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);
			if (*pt != 0)
			{
				listlen = DiaGetNumScrollLines(dia, DEDF_CELLLIST);
				for(i=0; i<listlen; i++)
				{
					ptr = DiaGetScrollLine(dia, DEDF_CELLLIST, i);
					if (estrcmp(ptr, pt) != 0) continue;
					DiaSelectLine(dia, DEDF_CELLLIST, i);
					break;
				}
			}
			continue;
		}
		if (itemHit == DEDF_NEWCELL)
		{
			/* display the new cell dialog box */
			makenewwindow = DiaGetControl(dia, DEDF_NEWWINDOW);
			subdia = DiaInitDialog(&us_newcelldialog);
			if (subdia == 0) continue;
			DiaSetPopup(subdia, DNWF_VIEWLIST, viewcount, viewlist);
			DiaSetControl(subdia, DNWF_NEWWINDOW, makenewwindow);
			DiaSetPopupEntry(subdia, DNWF_VIEWLIST, defaultindex);

			/* loop until done */
			for(;;)
			{
				itemHit = DiaNextHit(subdia);
				if (itemHit == CANCEL) break;
				if (itemHit == DNWF_NEWWINDOW)
				{
					DiaSetControl(subdia, itemHit, 1 - DiaGetControl(subdia, itemHit));
					continue;
				}
				if (itemHit == OK && DiaValidEntry(subdia, DNWF_CELLNAME)) break;
			}

			newpar[0] = x_("");
			if (itemHit != CANCEL)
			{
				i = DiaGetPopupEntry(subdia, DNWF_VIEWLIST);
				infstr = initinfstr();
				addstringtoinfstr(infstr, us_curlib->libname);
				addtoinfstr(infstr, ':');
				addstringtoinfstr(infstr, DiaGetText(subdia, DNWF_CELLNAME));
				for(v = el_views; v != NOVIEW; v = v->nextview)
					if (estrcmp(viewlist[i], v->viewname) == 0)
				{
					if (*v->sviewname == 0) break;
					addtoinfstr(infstr, '{');
					addstringtoinfstr(infstr, v->sviewname);
					addtoinfstr(infstr, '}');
					lastview = v;
					break;
				}
				if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
				allocstring(&us_returneddialogstring, returninfstr(infstr), us_tool->cluster);
				newpar[0] = us_returneddialogstring;
			}
			makenewwindow = DiaGetControl(subdia, DNWF_NEWWINDOW);
			DiaDoneDialog(subdia);
			if (itemHit == CANCEL) continue;
			DiaSetControl(dia, DEDF_NEWWINDOW, makenewwindow);
			break;
		}
	}
	makenewwindow = DiaGetControl(dia, DEDF_NEWWINDOW);
	DiaDoneDialog(dia);
	efree((CHAR *)librarylist);
	efree((CHAR *)viewlist);
	if (itemHit != CANCEL)
	{
		if (makenewwindow != 0 && curcell != NONODEPROTO)
		{
			newpar[1] = x_("new-window");
			us_editcell(2, newpar);
		} else
		{
			us_editcell(1, newpar);
		}
	}
	return(0);
}

/*
 * Routine to get the cell currently selected in the "edit cells" dialog.
 */
NODEPROTO *us_getselectedcell(void *dia)
{
	REGISTER INTBIG i;
	REGISTER void *infstr;
	REGISTER CHAR *pt;
	REGISTER NODEPROTO *np;

	i = DiaGetCurLine(dia, DEDF_CELLLIST);
	if (i < 0) return(NONODEPROTO);
	pt = DiaGetScrollLine(dia, DEDF_CELLLIST, i);
	if (*pt == 0) return(NONODEPROTO);
	infstr = initinfstr();
	addstringtoinfstr(infstr, us_curlib->libname);
	addtoinfstr(infstr, ':');
	addstringtoinfstr(infstr, pt);
	np = getnodeproto(returninfstr(infstr));
	return(np);
}

/****************************** CELL LISTS ******************************/

/* Cell Lists */
static DIALOGITEM us_faclisdialogitems[] =
{
 /*  1 */ {0, {464,152,488,232}, BUTTON, N_("OK")},
 /*  2 */ {0, {464,32,488,112}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {160,22,176,198}, CHECK, N_("Show only this view:")},
 /*  4 */ {0, {180,38,196,214}, POPUP, x_("")},
 /*  5 */ {0, {200,22,216,198}, CHECK, N_("Also include icon views")},
 /*  6 */ {0, {252,22,268,198}, CHECK, N_("Exclude older versions")},
 /*  7 */ {0, {140,10,156,186}, MESSAGE, N_("View filter:")},
 /*  8 */ {0, {232,10,248,186}, MESSAGE, N_("Version filter:")},
 /*  9 */ {0, {272,22,288,198}, CHECK, N_("Exclude newest versions")},
 /* 10 */ {0, {8,8,24,184}, MESSAGE, N_("Which cells:")},
 /* 11 */ {0, {88,20,104,248}, RADIO, N_("Only those under current cell")},
 /* 12 */ {0, {304,8,320,184}, MESSAGE, N_("Display ordering:")},
 /* 13 */ {0, {324,20,340,248}, RADIO, N_("Order by name")},
 /* 14 */ {0, {344,20,360,248}, RADIO, N_("Order by modification date")},
 /* 15 */ {0, {364,20,380,248}, RADIO, N_("Order by skeletal structure")},
 /* 16 */ {0, {396,8,412,184}, MESSAGE, N_("Destination:")},
 /* 17 */ {0, {416,16,432,244}, RADIO, N_("Display in messages window")},
 /* 18 */ {0, {436,16,452,244}, RADIO, N_("Save to disk")},
 /* 19 */ {0, {48,20,64,248}, RADIO, N_("Only those used elsewhere")},
 /* 20 */ {0, {108,20,124,248}, RADIO, N_("Only placeholder cells")},
 /* 21 */ {0, {28,20,44,248}, RADIO, N_("All cells")},
 /* 22 */ {0, {132,8,133,248}, DIVIDELINE, x_("")},
 /* 23 */ {0, {224,8,225,248}, DIVIDELINE, x_("")},
 /* 24 */ {0, {296,8,297,248}, DIVIDELINE, x_("")},
 /* 25 */ {0, {388,8,389,248}, DIVIDELINE, x_("")},
 /* 26 */ {0, {68,20,84,248}, RADIO, N_("Only those not used elsewhere")}
};
static DIALOG us_faclisdialog = {{75,75,572,333}, N_("Cell Lists"), 0, 26, us_faclisdialogitems, 0, 0};

/* special items for the "Cell lists" dialog: */
#define DFCL_ONEVIEW        3		/* Show just one view (check) */
#define DFCL_THEVIEW        4		/* The view to show (popup) */
#define DFCL_ICONSVIEWS     5		/* Show icon views too (check) */
#define DFCL_NOOLDVERS      6		/* Exclude older versions (check) */
#define DFCL_NONEWVERS      9		/* Exclude newest versions (check) */
#define DFCL_UNDERTHIS     11		/* Only those under current cell (radio) */
#define DFCL_BYNAME        13		/* Order by name (radio) */
#define DFCL_BYDATE        14		/* Order by modification date (radio) */
#define DFCL_BYSKELETON    15		/* Order by skeletal structure (radio) */
#define DFCL_TOMESSAGES    17		/* Display in messages window (radio) */
#define DFCL_TODISK        18		/* Display in disk file (radio) */
#define DFCL_INUSE         19		/* Only those used elsewhere (radio) */
#define DFCL_PLACEHOLDERS  20		/* Only placeholder cells (radio) */
#define DFCL_ALLCELLS      21		/* All cells (radio) */
#define DFCL_NOTINUSE      26		/* Only those not used elsewhere (radio) */

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif
	static int us_sortbycelldate(const void *e1, const void *e2);
	static int us_sortbycellname(const void *e1, const void *e2);
	static int us_sortbyskeletonstructure(const void *e1, const void *e2);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

INTBIG us_celllist(void)
{
	REGISTER INTBIG itemHit, viewcount, i, total, maxlen;
	CHAR **viewlist, *truename;
	REGISTER LIBRARY *lib, *savelib;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER VIEW *v;
	REGISTER void *infstr;
	FILE *dumpfile;
	REGISTER NODEPROTO *np, *inp, **nplist, *curf;
	static INTBIG sortorder = DFCL_BYNAME;
	static INTBIG where = DFCL_TOMESSAGES;
	static INTBIG which = DFCL_ALLCELLS;
	static INTBIG oldvers = 0, newvers = 0, alsoiconview = 0, oneview = 0;
	static VIEW *lastview = NOVIEW;
	REGISTER void *dia;

	viewcount = us_makeviewlist(&viewlist);
	if (viewcount == 0) return(0);

	/* display the dialog box */
	dia = DiaInitDialog(&us_faclisdialog);
	if (dia == 0) return(0);
	DiaSetPopup(dia, DFCL_THEVIEW, viewcount, viewlist);

	/* restore defaults */
	if (lastview != NOVIEW)
	{
		for(i=0; i<viewcount; i++)
			if (getview(viewlist[i]) == lastview) break;
		if (i < viewcount) DiaSetPopupEntry(dia, DFCL_THEVIEW, i);
	}
	curf = getcurcell();
	if (curf != NONODEPROTO) DiaUnDimItem(dia, DFCL_UNDERTHIS); else
	{
		if (which == DFCL_UNDERTHIS) which = DFCL_ALLCELLS;
		DiaDimItem(dia, DFCL_UNDERTHIS);
	}
	DiaSetControl(dia, sortorder, 1);
	DiaSetControl(dia, where, 1);
	DiaSetControl(dia, which, 1);
	DiaSetControl(dia, DFCL_NOOLDVERS, oldvers);
	DiaSetControl(dia, DFCL_NONEWVERS, newvers);
	DiaSetControl(dia, DFCL_ICONSVIEWS, alsoiconview);
	DiaSetControl(dia, DFCL_ONEVIEW, oneview);
	if (DiaGetControl(dia, DFCL_ONEVIEW) == 0)
	{
		DiaDimItem(dia, DFCL_ICONSVIEWS);
		DiaDimItem(dia, DFCL_THEVIEW);
	} else
	{
		DiaUnDimItem(dia, DFCL_ICONSVIEWS);
		DiaUnDimItem(dia, DFCL_THEVIEW);
	}

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DFCL_THEVIEW)
		{
			i = DiaGetPopupEntry(dia, DFCL_THEVIEW);
			lastview = getview(viewlist[i]);
			continue;
		}
		if (itemHit == DFCL_ONEVIEW || itemHit == DFCL_ICONSVIEWS ||
			itemHit == DFCL_NOOLDVERS || itemHit == DFCL_NONEWVERS)
		{
			i = 1-DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			switch (itemHit)
			{
				case DFCL_NOOLDVERS:  oldvers = i;      break;
				case DFCL_NONEWVERS:  newvers = i;      break;
				case DFCL_ICONSVIEWS: alsoiconview = i; break;
				case DFCL_ONEVIEW:
					oneview = i;
					if (i == 0)
					{
						DiaDimItem(dia, DFCL_ICONSVIEWS);
						DiaDimItem(dia, DFCL_THEVIEW);
					} else
					{
						DiaUnDimItem(dia, DFCL_ICONSVIEWS);
						DiaUnDimItem(dia, DFCL_THEVIEW);
					}
					break;
			}
			continue;
		}
		if (itemHit == DFCL_ALLCELLS || itemHit == DFCL_UNDERTHIS ||
			itemHit == DFCL_INUSE || itemHit == DFCL_PLACEHOLDERS ||
			itemHit == DFCL_NOTINUSE)
		{
			DiaSetControl(dia, DFCL_ALLCELLS, 0);
			DiaSetControl(dia, DFCL_UNDERTHIS, 0);
			DiaSetControl(dia, DFCL_INUSE, 0);
			DiaSetControl(dia, DFCL_PLACEHOLDERS, 0);
			DiaSetControl(dia, DFCL_NOTINUSE, 0);
			DiaSetControl(dia, itemHit, 1);
			which = itemHit;
			continue;
		}
		if (itemHit == DFCL_BYNAME || itemHit == DFCL_BYDATE ||
			itemHit == DFCL_BYSKELETON)
		{
			DiaSetControl(dia, DFCL_BYNAME, 0);
			DiaSetControl(dia, DFCL_BYDATE, 0);
			DiaSetControl(dia, DFCL_BYSKELETON, 0);
			DiaSetControl(dia, itemHit, 1);
			sortorder = itemHit;
			continue;
		}
		if (itemHit == DFCL_TOMESSAGES || itemHit == DFCL_TODISK)
		{
			DiaSetControl(dia, DFCL_TOMESSAGES, 0);
			DiaSetControl(dia, DFCL_TODISK, 0);
			DiaSetControl(dia, itemHit, 1);
			where = itemHit;
			continue;
		}
	}
	if (itemHit == OK)
	{
		/* mark cells to be shown */
		if (DiaGetControl(dia, DFCL_ALLCELLS) != 0)
		{
			/* mark all cells for display */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					np->temp1 = 1;
		} else
		{
			/* mark no cells for display, filter according to request */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					np->temp1 = 0;
			if (DiaGetControl(dia, DFCL_UNDERTHIS) != 0)
			{
				/* mark those that are under this */
				if (curf != NONODEPROTO) us_recursivemark(curf);
			} else if (DiaGetControl(dia, DFCL_INUSE) != 0)
			{
				/* mark those that are in use */
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				{
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					{
						inp = iconview(np);
						if (inp == NONODEPROTO) inp = np;
						if (inp->firstinst != NONODEINST || np->firstinst != NONODEINST) np->temp1 = 1;
					}
				}
			} else if (DiaGetControl(dia, DFCL_NOTINUSE) != 0)
			{
				/* mark those that are not in use */
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				{
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					{
						inp = iconview(np);
						if (inp != NONODEPROTO)
						{
							/* has icon: acceptable if the only instances are examples */
							if (np->firstinst != NONODEINST) continue;
							for(ni = inp->firstinst; ni != NONODEINST; ni = ni->nextinst)
								if (!isiconof(inp, ni->parent)) break;
							if (ni != NONODEINST) continue;
						} else
						{
							/* no icon: reject if this has instances */
							if (np->cellview == el_iconview)
							{
								/* this is an icon: reject if instances are not examples */
								for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
									if (!isiconof(np, ni->parent)) break;
								if (ni != NONODEINST) continue;
							} else
							{
								if (np->firstinst != NONODEINST) continue;
							}
						}
						np->temp1 = 1;
					}
				}
			} else
			{
				/* mark placeholder cells */
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				{
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					{
						var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("IO_true_library"));
						if (var != NOVARIABLE) np->temp1 = 1;
					}
				}
			}
		}

		/* filter views */
		if (DiaGetControl(dia, DFCL_ONEVIEW) != 0)
		{
			i = DiaGetPopupEntry(dia, DFCL_THEVIEW);
			v = getview(viewlist[i]);
			if (v != NOVIEW)
			{
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				{
					for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					{
						if (np->cellview != v)
						{
							if (np->cellview == el_iconview)
							{
								if (DiaGetControl(dia, DFCL_ICONSVIEWS) != 0) continue;
							}
							np->temp1 = 0;
						}
					}
				}
			}
		}

		/* filter versions */
		if (DiaGetControl(dia, DFCL_NOOLDVERS) != 0)
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (np->newestversion != np) np->temp1 = 0;
				}
			}
		}
		if (DiaGetControl(dia, DFCL_NONEWVERS) != 0)
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (np->newestversion == np) np->temp1 = 0;
				}
			}
		}

		/* now make a list and sort it */
		total = 0;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 != 0) total++;
			}
		}
		if (total == 0) ttyputmsg(_("No cells match this request")); else
		{
			nplist = (NODEPROTO **)emalloc(total * (sizeof (NODEPROTO *)), el_tempcluster);
			if (nplist == 0) return(0);
			total = 0;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (np->temp1 != 0) nplist[total++] = np;
				}
			}
			if (DiaGetControl(dia, DFCL_BYNAME) != 0)
			{
				esort(nplist, total, sizeof (NODEPROTO *), us_sortbycellname);
			} else if (DiaGetControl(dia, DFCL_BYDATE) != 0)
			{
				esort(nplist, total, sizeof (NODEPROTO *), us_sortbycelldate);
			} else if (DiaGetControl(dia, DFCL_BYSKELETON) != 0)
			{
				esort(nplist, total, sizeof (NODEPROTO *), us_sortbyskeletonstructure);
			}

			/* finally show the results */
			if (DiaGetControl(dia, DFCL_TODISK) != 0)
			{
				dumpfile = xcreate(x_("celllist.txt"), el_filetypetext, _("Cell Listing File:"), &truename);
				if (dumpfile == 0) ttyputerr(_("Cannot write cell listing")); else
				{
					efprintf(dumpfile, _("List of cells created on %s\n"), timetostring(getcurrenttime()));
					efprintf(dumpfile, _("Cell\tVersion\tCreation date\tRevision Date\tSize\tUsage\tLock\tInst-lock\tCell-lib\tDRC\tNCC\n"));
					for(i=0; i<total; i++)
						efprintf(dumpfile, x_("%s\n"), us_makecellline(nplist[i], -1));
					xclose(dumpfile);
					ttyputmsg(_("Wrote %s"), truename);
				}
			} else
			{
				maxlen = 0;
				for(i=0; i<total; i++)
					maxlen = maxi(maxlen, estrlen(nldescribenodeproto(nplist[i])));
				maxlen = maxi(maxlen+2, 7);
				infstr = initinfstr();
				addstringtoinfstr(infstr, _("Cell"));
				for(i=4; i<maxlen; i++) addtoinfstr(infstr, '-');
				addstringtoinfstr(infstr, _("Version-----Creation date"));
				addstringtoinfstr(infstr, _("---------Revision Date------------Size-------Usage-L-I-C-D-N"));
				ttyputmsg(x_("%s"), returninfstr(infstr));
				lib = NOLIBRARY;
				for(i=0; i<total; i++)
				{
					if (nplist[i]->lib != lib)
					{
						lib = nplist[i]->lib;
						ttyputmsg(_("======== LIBRARY %s: ========"), lib->libname);
					}
					savelib = el_curlib;
					el_curlib = lib;
					ttyputmsg(x_("%s"), us_makecellline(nplist[i], maxlen));
					el_curlib = lib;
				}
			}
		}
	}
	DiaDoneDialog(dia);
	efree((CHAR *)viewlist);
	return(1);
}

/*
 * Helper routine for "esort" that makes cells go by skeleton structure
 */
int us_sortbyskeletonstructure(const void *e1, const void *e2)
{
	REGISTER NODEPROTO *f1, *f2;
	REGISTER INTBIG xs1, xs2, ys1, ys2, pc1, pc2;
	INTBIG x1, y1, x2, y2;
	REGISTER PORTPROTO *pp1, *pp2;
	NODEINST dummyni;
	REGISTER NODEINST *ni;

	f1 = *((NODEPROTO **)e1);
	f2 = *((NODEPROTO **)e2);

	/* first sort by cell size */
	xs1 = f1->highx - f1->lowx;   xs2 = f2->highx - f2->lowx;
	if (xs1 != xs2) return(xs1-xs2);
	ys1 = f1->highy - f1->lowy;   ys2 = f2->highy - f2->lowy;
	if (ys1 != ys2) return(ys1-ys2);

	/* now sort by number of exports */
	pc1 = 0;
	for(pp1 = f1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto) pc1++;
	pc2 = 0;
	for(pp2 = f2->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto) pc2++;
	if (pc1 != pc2) return(pc1-pc2);

	/* now match the exports */
	for(pp1 = f1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto) pp1->temp1 = 0;
	for(pp2 = f2->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
	{
		/* locate center of this export */
		ni = &dummyni;
		initdummynode(ni);
		ni->proto = f2;
		ni->lowx = -xs1/2;   ni->highx = ni->lowx + xs1;
		ni->lowy = -ys1/2;   ni->highy = ni->lowy + ys1;
		portposition(ni, pp2, &x2, &y2);

		ni->proto = f1;
		for(pp1 = f1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
		{
			portposition(ni, pp1, &x1, &y1);
			if (x1 == x2 && y1 == y2) break;
		}
		if (pp1 == NOPORTPROTO) return(f1-f2);
		pp1->temp1 = 1;
	}
	for(pp1 = f1->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
		if (pp1->temp1 == 0) return(f1-f2);
	return(0);
}

/*
 * Helper routine for "esort" that makes cells go by date
 */
int us_sortbycelldate(const void *e1, const void *e2)
{
	REGISTER NODEPROTO *f1, *f2;
	REGISTER UINTBIG r1, r2;

	f1 = *((NODEPROTO **)e1);
	f2 = *((NODEPROTO **)e2);
	r1 = f1->revisiondate;
	r2 = f2->revisiondate;
	if (r1 == r2) return(0);
	if (r1 < r2) return(-(int)(r2-r1));
	return(r1-r2);
}

/*
 * Helper routine for "esort" that makes cell names be ascending
 */
int us_sortbycellname(const void *e1, const void *e2)
{
	REGISTER NODEPROTO *f1, *f2;
	REGISTER void *infstr;
	REGISTER CHAR *s1, *s2;

	f1 = *((NODEPROTO **)e1);
	f2 = *((NODEPROTO **)e2);
	infstr = initinfstr();
	formatinfstr(infstr, x_("%s:%s"), f1->lib->libname, nldescribenodeproto(f1));
	s1 = returninfstr(infstr);
	infstr = initinfstr();
	formatinfstr(infstr, x_("%s:%s"), f2->lib->libname, nldescribenodeproto(f2));
	s2 = returninfstr(infstr);
	return(namesame(s1, s2));
}

/****************************** CELL SELECTION DIALOGS ******************************/

/* Cell Selection */
static DIALOGITEM us_cellselectdialogitems[] =
{
 /*  1 */ {0, {336,208,360,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {336,16,360,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {36,8,251,280}, SCROLL, x_("")},
 /*  4 */ {0, {284,8,302,153}, CHECK, N_("Show old versions")},
 /*  5 */ {0, {308,8,326,253}, CHECK, N_("Show cells from Cell-Library")},
 /*  6 */ {0, {8,8,26,67}, MESSAGE, N_("Library:")},
 /*  7 */ {0, {8,72,26,280}, POPUP, x_("")},
 /*  8 */ {0, {260,8,278,205}, CHECK, N_("Show relevant cells only")}
};
static DIALOG us_cellselectdialog = {{50,75,419,365}, N_("Cell List"), 0, 8, us_cellselectdialogitems, 0, 0};

/*
 * special items for the "Delete Cell", "New Cell Instance", "List Cell Usage", and
 * "plot simulation in cell window" dialogs:
 */
#define DFCS_CELLLIST        3		/* Cell list (message) */
#define DFCS_OLDVERSIONS     4		/* Show old versions (check) */
#define DFCS_CELLLIBRARIES   5		/* Show cell-library cells (check) */
#define DFCS_LIBRARYLIST     7		/* List of libraries (popup) */
#define DFCS_RELEVANTCELLS   8		/* Show relevant cells (check) */

/*
 * Routine to present a cell list with the title "prompt".
 * If "selcell" is 0, choose a cell (including the current cell).
 * If "selcell" is 1, choose a cell for instance placement (doesn't include current cell).
 * If "selcell" is 2, choose a cell for deletion (actually, do deletion dialog).
 */
INTBIG us_cellselect(CHAR *prompt, CHAR *paramstart[], INTBIG selcell)
{
	REGISTER INTBIG itemHit, libindex;
	INTBIG i, librarycount;
	REGISTER CHAR **librarylist;
	CHAR *arglist[1], *pt;
	REGISTER NODEPROTO *np;
	REGISTER BOOLEAN curinstance, curcell;
	REGISTER void *infstr, *dia;

	/* display the dialog box */
	us_cellselectdialog.movable = prompt;
	if (selcell != 2)
	{
		/* cell selection */
		us_cellselectdialogitems[0].msg = _("OK");
		us_cellselectdialogitems[1].msg = _("Cancel");
		curinstance = TRUE;
		if (selcell == 0) curcell = TRUE; else
			curcell = FALSE;
	} else
	{
		/* cell deletion */
		us_cellselectdialogitems[0].msg = _("Delete");
		us_cellselectdialogitems[1].msg = _("Done");
		curinstance = FALSE;
		curcell = TRUE;
	}
	dia = DiaInitDialog(&us_cellselectdialog);
	if (dia == 0) return(0);
	us_showoldversions = us_defshowoldversions;
	us_showcellibrarycells = us_defshowcellibrarycells;
	us_showonlyrelevantcells = us_defshowonlyrelevantcells;
	us_curlib = el_curlib;
	DiaInitTextDialog(dia, DFCS_CELLLIST, us_oldcelltopofcells, us_oldcellnextcells,
		DiaNullDlogDone, 0, SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT);
	(void)us_setscrolltocurrentcell(DFCS_CELLLIST, curinstance, curcell, FALSE, FALSE, dia);
	if (us_showonlyrelevantcells != 0) DiaSetControl(dia, DFCS_RELEVANTCELLS, 1);

	/* make a list of library names */
	librarylist = us_makelibrarylist(&librarycount, us_curlib, &i);
	libindex = 0;
	for(i=0; i<librarycount; i++)
		if (namesame(librarylist[i], us_curlib->libname) == 0) libindex = i;
	DiaSetPopup(dia, DFCS_LIBRARYLIST, librarycount, librarylist);
	DiaSetPopupEntry(dia, DFCS_LIBRARYLIST, libindex);

	/* see if there are any old versions */
	for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->newestversion != np) break;
	if (np == NONODEPROTO) DiaDimItem(dia, DFCS_OLDVERSIONS); else
	{
		DiaUnDimItem(dia, DFCS_OLDVERSIONS);
		DiaSetControl(dia, DFCS_OLDVERSIONS, us_showoldversions);
	}

	/* see if there are any cell-library cells */
	for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if ((np->userbits&INCELLLIBRARY) != 0) break;
	if (np == NONODEPROTO) DiaDimItem(dia, DFCS_CELLLIBRARIES); else
	{
		DiaUnDimItem(dia, DFCS_CELLLIBRARIES);
		DiaSetControl(dia, DFCS_CELLLIBRARIES, us_showcellibrarycells);
	}

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK)
		{
			if (selcell != 2) break;

			/* delete the cell */
			infstr = initinfstr();
			if (us_curlib != el_curlib)
			{
				addstringtoinfstr(infstr, us_curlib->libname);
				addtoinfstr(infstr, ':');
			}
			i = DiaGetCurLine(dia, DFCS_CELLLIST);
			addstringtoinfstr(infstr, DiaGetScrollLine(dia, DFCS_CELLLIST, i));
			(void)allocstring(&pt, returninfstr(infstr), el_tempcluster);
			arglist[0] = pt;
			us_killcell(1, arglist);
			efree((CHAR *)pt);
			DiaLoadTextDialog(dia, DFCS_CELLLIST, us_oldcelltopofcells, us_oldcellnextcells,
				DiaNullDlogDone, 0);
			if (*DiaGetScrollLine(dia, DFCS_CELLLIST, i) == 0) i--;
			DiaSelectLine(dia, DFCS_CELLLIST, i);
			continue;
		}
		if (itemHit == DFCS_OLDVERSIONS)
		{
			us_defshowoldversions = us_showoldversions = 1 - DiaGetControl(dia, DFCS_OLDVERSIONS);
			DiaSetControl(dia, DFCS_OLDVERSIONS, us_showoldversions);
			DiaLoadTextDialog(dia, DFCS_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);
		}
		if (itemHit == DFCS_CELLLIBRARIES)
		{
			us_defshowcellibrarycells = us_showcellibrarycells = 1 - DiaGetControl(dia, DFCS_CELLLIBRARIES);
			DiaSetControl(dia, DFCS_CELLLIBRARIES, us_showcellibrarycells);
			DiaLoadTextDialog(dia, DFCS_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);
		}
		if (itemHit == DFCS_RELEVANTCELLS)
		{
			us_defshowonlyrelevantcells = us_showonlyrelevantcells = 1 - DiaGetControl(dia, DFCS_RELEVANTCELLS);
			DiaSetControl(dia, DFCS_RELEVANTCELLS, us_showonlyrelevantcells);
			DiaLoadTextDialog(dia, DFCS_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);
		}
		if (itemHit == DFCS_LIBRARYLIST)
		{
			i = DiaGetPopupEntry(dia, DFCS_LIBRARYLIST);
			us_curlib = getlibrary(librarylist[i]);
			DiaInitTextDialog(dia, DFCS_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0, SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT);
			(void)us_setscrolltocurrentcell(DFCS_CELLLIST, curinstance, curcell, FALSE, FALSE, dia);

			/* see if there are any old versions */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (np->newestversion != np) break;
			if (np == NONODEPROTO) DiaDimItem(dia, DFCS_OLDVERSIONS); else
			{
				DiaUnDimItem(dia, DFCS_OLDVERSIONS);
				DiaSetControl(dia, DFCS_OLDVERSIONS, us_showoldversions);
			}

			/* see if there are any cell-library cells */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if ((np->userbits&INCELLLIBRARY) != 0) break;
			if (np == NONODEPROTO) DiaDimItem(dia, DFCS_CELLLIBRARIES); else
			{
				DiaUnDimItem(dia, DFCS_CELLLIBRARIES);
				DiaSetControl(dia, DFCS_CELLLIBRARIES, us_showcellibrarycells);
			}
			continue;
		}
	}
	if (selcell != 2)
	{
		infstr = initinfstr();
		if (us_curlib != el_curlib)
		{
			addstringtoinfstr(infstr, us_curlib->libname);
			addtoinfstr(infstr, ':');
		}
		addstringtoinfstr(infstr, DiaGetScrollLine(dia, DFCS_CELLLIST, DiaGetCurLine(dia, DFCS_CELLLIST)));
		if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
		allocstring(&us_returneddialogstring, returninfstr(infstr), us_tool->cluster);
		paramstart[0] = us_returneddialogstring;
	}
	DiaDoneDialog(dia);
	if (itemHit == CANCEL) return(0);
	return(1);
}

/****************************** CELL OPTIONS DIALOG ******************************/

/* Cell options */
static DIALOGITEM us_celldialogitems[] =
{
 /*  1 */ {0, {288,512,312,576}, BUTTON, N_("OK")},
 /*  2 */ {0, {232,512,256,576}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {304,24,320,324}, CHECK, N_("Tiny cell instances hashed out")},
 /*  4 */ {0, {8,8,24,90}, MESSAGE, N_("Library:")},
 /*  5 */ {0, {28,4,188,150}, SCROLL, x_("")},
 /*  6 */ {0, {8,92,24,256}, POPUP, x_("")},
 /*  7 */ {0, {196,4,197,584}, DIVIDELINE, x_("")},
 /*  8 */ {0, {208,4,224,116}, MESSAGE, N_("For all cells:")},
 /*  9 */ {0, {28,156,44,496}, CHECK, N_("Disallow modification of anything in this cell")},
 /* 10 */ {0, {4,500,20,584}, MESSAGE, N_("Every cell:")},
 /* 11 */ {0, {52,156,68,496}, CHECK, N_("Disallow modification of instances in this cell")},
 /* 12 */ {0, {148,156,164,344}, MESSAGE|INACTIVE, N_("Characteristic X Spacing:")},
 /* 13 */ {0, {172,156,188,344}, MESSAGE|INACTIVE, N_("Characteristic Y Spacing:")},
 /* 14 */ {0, {148,348,164,424}, EDITTEXT, x_("")},
 /* 15 */ {0, {172,348,188,424}, EDITTEXT, x_("")},
 /* 16 */ {0, {76,156,92,496}, CHECK, N_("Part of a cell-library")},
 /* 17 */ {0, {76,500,92,536}, BUTTON, N_("Set")},
 /* 18 */ {0, {76,544,92,584}, BUTTON, N_("Clear")},
 /* 19 */ {0, {328,24,344,280}, MESSAGE|INACTIVE, N_("Hash cells when scale is more than:")},
 /* 20 */ {0, {328,284,344,344}, EDITTEXT, x_("")},
 /* 21 */ {0, {328,348,344,472}, MESSAGE|INACTIVE, N_("lambda per pixel")},
 /* 22 */ {0, {124,156,140,344}, RADIO, N_("Expand new instances")},
 /* 23 */ {0, {124,360,140,548}, RADIO, N_("Unexpand new instances")},
 /* 24 */ {0, {232,24,248,324}, CHECK, N_("Check cell dates during creation")},
 /* 25 */ {0, {28,544,44,584}, BUTTON, N_("Clear")},
 /* 26 */ {0, {28,500,44,536}, BUTTON, N_("Set")},
 /* 27 */ {0, {52,544,68,584}, BUTTON, N_("Clear")},
 /* 28 */ {0, {52,500,68,536}, BUTTON, N_("Set")},
 /* 29 */ {0, {256,24,272,324}, CHECK, N_("Switch technology to match current cell")},
 /* 30 */ {0, {352,24,368,280}, MESSAGE|INACTIVE, N_("Cell explorer text size:")},
 /* 31 */ {0, {352,284,368,344}, EDITTEXT, x_("")},
 /* 32 */ {0, {280,24,296,324}, CHECK, N_("Place Cell-Center in new cells")},
 /* 33 */ {0, {100,156,116,496}, CHECK, N_("Use technology editor on this cell")},
 /* 34 */ {0, {100,500,116,536}, BUTTON, N_("Set")},
 /* 35 */ {0, {100,544,116,584}, BUTTON, N_("Clear")}
};
static DIALOG us_celldialog = {{50,75,427,669}, N_("Cell Options"), 0, 35, us_celldialogitems, 0, 0};

/* special items for the "cell info" dialog: */
#define DFCO_TINYCELLGREY           3		/* Tiny cells (check) */
#define DFCO_CELLLIST               5		/* Cell names (scroll) */
#define DFCO_LIBRARYLIST            6		/* Library to use (popup) */
#define DFCO_ALLOWALLMODCELL        9		/* Allow all mod in cell (check) */
#define DFCO_ALLOWINSTMODCELL      11		/* Allow inst mod in cell (check) */
#define DFCO_CHARSPACX             14		/* X Characteristic spacing (edit text) */
#define DFCO_CHARSPACY             15		/* Y Characteristic spacing (edit text) */
#define DFCO_CELLLIBRARY           16		/* Part of cell library (check) */
#define DFCO_CELLLIBRARYSET        17		/* "Set all" for cell library (button) */
#define DFCO_CELLLIBRARYCLR        18		/* "Clear all" for cell library (button) */
#define DFCO_TINYCELLTHRESH        20		/* Tiny lambda per pixel (edit text) */
#define DFCO_EXPANDINST            22		/* Expand new instances (radio) */
#define DFCO_UNEXPANDINST          23		/* Unexpand new instances (radio) */
#define DFCO_CHECKDATES            24		/* Check dates (check) */
#define DFCO_ALLOWALLMODCELLCLR    25		/* "Clear all" for cell modification (button) */
#define DFCO_ALLOWALLMODCELLSET    26		/* "Set all" for cell modification (button) */
#define DFCO_ALLOWINSTMODCELLCLR   27		/* "Clear all" for instance modification (button) */
#define DFCO_ALLOWINSTMODCELLSET   28		/* "Set all" for instance modification (button) */
#define DFCO_AUTOTECHSWITCH        29		/* Switch technology when cell changes (check) */
#define DFCO_EXPLORERTEXTSIZE      31		/* Cell explorer text size (edit text) */
#define DFCO_PLACECELLCENTER       32		/* Place cell-center in new cells (check) */
#define DFCO_USETECHEDIT           33		/* Use technology editor on this cell (check) */
#define DFCO_USETECHEDITSET        34		/* "Set all" for technology editor use (button) */
#define DFCO_USETECHEDITCLR        35		/* "Clear all" for technology editor use (button) */

INTBIG us_celldlog(void)
{
	INTBIG itemHit, i, lx, total, value, exploretextsize;
	CHAR buf[20];
	REGISTER NODEPROTO *thiscell, *np;
	REGISTER VARIABLE *var;
	REGISTER LIBRARY *lib, *savelib;
	REGISTER CHAR *pt, **liblist;
	typedef struct
	{
		INTBIG newbits;
		INTBIG validcharacteristicspacing;
		INTBIG characteristicspacing[2];
	} CELLINFO;
	CELLINFO *fi;
	REGISTER void *dia;

	/* display the cell dialog box */
	dia = DiaInitDialog(&us_celldialog);
	if (dia == 0) return(0);

	/* show the cells in this library */
	us_showoldversions = 1;
	us_showcellibrarycells = 1;
	us_showonlyrelevantcells = 0;
	us_curlib = el_curlib;
	DiaInitTextDialog(dia, DFCO_CELLLIST, us_oldcelltopofcells, us_oldcellnextcells,
		DiaNullDlogDone, 0, SCSELMOUSE|SCSELKEY|SCREPORT);

	/* get the current cell, and set the scroll item to it */
	thiscell = us_setscrolltocurrentcell(DFCO_CELLLIST, FALSE, TRUE, TRUE, FALSE, dia);
	if (thiscell != NONODEPROTO && thiscell->lib != us_curlib)
		thiscell = NONODEPROTO;
	if (thiscell == NONODEPROTO) thiscell = us_curlib->firstnodeproto;

	/* load popup of libraries */
	liblist = us_makelibrarylist(&total, us_curlib, &i);
	DiaSetPopup(dia, DFCO_LIBRARYLIST, total, liblist);
	if (i >= 0) DiaSetPopupEntry(dia, DFCO_LIBRARYLIST, i);

	/* save cell information */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		fi = (CELLINFO *)emalloc(sizeof (CELLINFO), el_tempcluster);
		if (fi == 0) return(0);
		fi->newbits = np->userbits;
		var = getval((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, x_("FACET_characteristic_spacing"));
		if (var == NOVARIABLE) fi->validcharacteristicspacing = 0; else
		{
			fi->characteristicspacing[0] = ((INTBIG *)var->addr)[0];
			fi->characteristicspacing[1] = ((INTBIG *)var->addr)[1];
			fi->validcharacteristicspacing = 1;
		}
		np->temp1 = (INTBIG)fi;
	}

	/* load defaults for primitives and cells */
	if (thiscell != NONODEPROTO)
	{
		DiaUnDimItem(dia, DFCO_CELLLIST);
		DiaUnDimItem(dia, DFCO_ALLOWALLMODCELL);
		DiaUnDimItem(dia, DFCO_ALLOWINSTMODCELL);
		DiaUnDimItem(dia, DFCO_CHARSPACX);
		DiaUnDimItem(dia, DFCO_CHARSPACY);
		DiaUnDimItem(dia, DFCO_CELLLIBRARY);
		DiaUnDimItem(dia, DFCO_CELLLIBRARYSET);
		DiaUnDimItem(dia, DFCO_CELLLIBRARYCLR);
		DiaUnDimItem(dia, DFCO_USETECHEDIT);
		DiaUnDimItem(dia, DFCO_USETECHEDITSET);
		DiaUnDimItem(dia, DFCO_USETECHEDITCLR);
		DiaUnDimItem(dia, DFCO_EXPANDINST);
		DiaUnDimItem(dia, DFCO_UNEXPANDINST);
		DiaUnDimItem(dia, DFCO_ALLOWALLMODCELLCLR);
		DiaUnDimItem(dia, DFCO_ALLOWALLMODCELLSET);
		DiaUnDimItem(dia, DFCO_ALLOWINSTMODCELLCLR);
		DiaUnDimItem(dia, DFCO_ALLOWINSTMODCELLSET);

		fi = (CELLINFO *)thiscell->temp1;
		if ((fi->newbits&NPLOCKED) != 0) DiaSetControl(dia, DFCO_ALLOWALLMODCELL, 1); else
			DiaSetControl(dia, DFCO_ALLOWALLMODCELL, 0);
		if ((fi->newbits&NPILOCKED) != 0) DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, 1); else
			DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, 0);
		if ((fi->newbits&INCELLLIBRARY) != 0) DiaSetControl(dia, DFCO_CELLLIBRARY, 1); else
			DiaSetControl(dia, DFCO_CELLLIBRARY, 0);
		if ((fi->newbits&TECEDITCELL) != 0) DiaSetControl(dia, DFCO_USETECHEDIT, 1); else
			DiaSetControl(dia, DFCO_USETECHEDIT, 0);
		if ((fi->newbits&WANTNEXPAND) != 0) DiaSetControl(dia, DFCO_EXPANDINST, 1); else
			DiaSetControl(dia, DFCO_UNEXPANDINST, 1);
		if (fi->validcharacteristicspacing != 0)
		{
			DiaSetText(dia, DFCO_CHARSPACX, latoa(fi->characteristicspacing[0], 0));
			DiaSetText(dia, DFCO_CHARSPACY, latoa(fi->characteristicspacing[1], 0));
		}
	} else
	{
		DiaDimItem(dia, DFCO_CELLLIST);
		DiaDimItem(dia, DFCO_ALLOWALLMODCELL);
		DiaDimItem(dia, DFCO_ALLOWINSTMODCELL);
		DiaDimItem(dia, DFCO_CHARSPACX);
		DiaDimItem(dia, DFCO_CHARSPACY);
		DiaDimItem(dia, DFCO_CELLLIBRARY);
		DiaDimItem(dia, DFCO_CELLLIBRARYSET);
		DiaDimItem(dia, DFCO_CELLLIBRARYCLR);
		DiaDimItem(dia, DFCO_USETECHEDIT);
		DiaDimItem(dia, DFCO_USETECHEDITSET);
		DiaDimItem(dia, DFCO_USETECHEDITCLR);
		DiaDimItem(dia, DFCO_EXPANDINST);
		DiaDimItem(dia, DFCO_UNEXPANDINST);
		DiaDimItem(dia, DFCO_ALLOWALLMODCELLCLR);
		DiaDimItem(dia, DFCO_ALLOWALLMODCELLSET);
		DiaDimItem(dia, DFCO_ALLOWINSTMODCELLCLR);
		DiaDimItem(dia, DFCO_ALLOWINSTMODCELLSET);
	}

	/* load defaults for all nodes */
	DiaSetControl(dia, DFCO_TINYCELLGREY, (us_useroptions&DRAWTINYCELLS) == 0 ? 1 : 0);
	DiaSetControl(dia, DFCO_CHECKDATES, (us_useroptions&CHECKDATE) != 0 ? 1 : 0);
	DiaSetControl(dia, DFCO_AUTOTECHSWITCH, (us_useroptions&AUTOSWITCHTECHNOLOGY) != 0 ? 1 : 0);
	DiaSetControl(dia, DFCO_PLACECELLCENTER, (us_useroptions&CELLCENTERALWAYS) != 0 ? 1 : 0);

	DiaSetText(dia, DFCO_TINYCELLTHRESH, frtoa(us_tinyratio));
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_facet_explorer_textsize"));
	if (var == NOVARIABLE) exploretextsize = DEFAULTEXPLORERTEXTSIZE; else
		exploretextsize = var->addr;
	esnprintf(buf, 20, x_("%ld"), exploretextsize);
	DiaSetText(dia, DFCO_EXPLORERTEXTSIZE, buf);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DFCO_LIBRARYLIST)
		{
			/* clicked on the library selection popup */
			i = DiaGetPopupEntry(dia, DFCO_LIBRARYLIST);
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				if (namesame(lib->libname, liblist[i]) == 0) break;
			if (lib == NOLIBRARY) continue;
			us_curlib = lib;
			DiaLoadTextDialog(dia, DFCO_CELLLIST, us_oldcelltopofcells,
				us_oldcellnextcells, DiaNullDlogDone, 0);

			thiscell = us_setscrolltocurrentcell(DFCO_CELLLIST, FALSE, TRUE, TRUE, FALSE, dia);
			if (thiscell != NONODEPROTO && thiscell->lib != us_curlib)
				thiscell = NONODEPROTO;
			if (thiscell == NONODEPROTO) thiscell = us_curlib->firstnodeproto;
			itemHit = DFCO_CELLLIST;
		}
		if (itemHit == DFCO_CELLLIST)
		{
			/* clicked on cell name in scroll list */
			i = DiaGetCurLine(dia, DFCO_CELLLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DFCO_CELLLIST, i);
			savelib = el_curlib;
			el_curlib = us_curlib;
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (namesame(pt, describenodeproto(np)) == 0) break;
			el_curlib = savelib;
			thiscell = np;
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			if ((fi->newbits&NPLOCKED) != 0) DiaSetControl(dia, DFCO_ALLOWALLMODCELL, 1); else
				DiaSetControl(dia, DFCO_ALLOWALLMODCELL, 0);
			if ((fi->newbits&NPILOCKED) != 0) DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, 1); else
				DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, 0);
			if ((fi->newbits&INCELLLIBRARY) != 0) DiaSetControl(dia, DFCO_CELLLIBRARY, 1); else
				DiaSetControl(dia, DFCO_CELLLIBRARY, 0);
			if ((fi->newbits&TECEDITCELL) != 0) DiaSetControl(dia, DFCO_USETECHEDIT, 1); else
				DiaSetControl(dia, DFCO_USETECHEDIT, 0);
			if ((fi->newbits&WANTNEXPAND) != 0)
			{
				DiaSetControl(dia, DFCO_EXPANDINST, 1);
				DiaSetControl(dia, DFCO_UNEXPANDINST, 0);
			} else
			{
				DiaSetControl(dia, DFCO_EXPANDINST, 0);
				DiaSetControl(dia, DFCO_UNEXPANDINST, 1);
			}
			if (fi->validcharacteristicspacing != 0)
			{
				DiaSetText(dia, DFCO_CHARSPACX, latoa(fi->characteristicspacing[0], 0));
				DiaSetText(dia, DFCO_CHARSPACY, latoa(fi->characteristicspacing[1], 0));
			} else
			{
				DiaSetText(dia, DFCO_CHARSPACX, x_(""));
				DiaSetText(dia, DFCO_CHARSPACY, x_(""));
			}
			continue;
		}
		if (itemHit == DFCO_ALLOWALLMODCELL)
		{
			/* clicked on check: "Disallow modification of anything in this cell" */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			value = 1 - DiaGetControl(dia, DFCO_ALLOWALLMODCELL);
			DiaSetControl(dia, DFCO_ALLOWALLMODCELL, value);
			if (value == 0) fi->newbits &= ~NPLOCKED; else
				fi->newbits |= NPLOCKED;
			continue;
		}
		if (itemHit == DFCO_ALLOWINSTMODCELL)
		{
			/* clicked on check: "Disallow modification of instances in this cell" */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			value = 1 - DiaGetControl(dia, DFCO_ALLOWINSTMODCELL);
			DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, value);
			if (value == 0) fi->newbits &= ~NPILOCKED; else
				fi->newbits |= NPILOCKED;
			continue;
		}
		if (itemHit == DFCO_CELLLIBRARY)
		{
			/* clicked on check: "Part of a cell-library" */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			value = 1 - DiaGetControl(dia, DFCO_CELLLIBRARY);
			DiaSetControl(dia, DFCO_CELLLIBRARY, value);
			if (value == 0) fi->newbits &= ~INCELLLIBRARY; else
				fi->newbits |= INCELLLIBRARY;
			continue;
		}
		if (itemHit == DFCO_USETECHEDIT)
		{
			/* clicked on check: "Use technology editor on this cell" */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			value = 1 - DiaGetControl(dia, DFCO_USETECHEDIT);
			DiaSetControl(dia, DFCO_USETECHEDIT, value);
			if (value == 0) fi->newbits &= ~TECEDITCELL; else
				fi->newbits |= TECEDITCELL;
			continue;
		}
		if (itemHit == DFCO_EXPANDINST || itemHit == DFCO_UNEXPANDINST)
		{
			/* clicked on radio buttons: "Expand/Unexpand new instances" */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			DiaSetControl(dia, DFCO_EXPANDINST, 0);
			DiaSetControl(dia, DFCO_UNEXPANDINST, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DFCO_EXPANDINST) fi->newbits |= WANTNEXPAND; else
				fi->newbits &= ~WANTNEXPAND;
			continue;
		}
		if (itemHit == DFCO_CELLLIBRARYSET)
		{
			/* clicked on button "Set" for all cells to be "Part of a cell-library" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits |= INCELLLIBRARY;
			}
			DiaSetControl(dia, DFCO_CELLLIBRARY, 1);
			continue;
		}
		if (itemHit == DFCO_CELLLIBRARYCLR)
		{
			/* clicked on button "Clear" for all cells to be "Part of a cell-library" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits &= ~INCELLLIBRARY;
			}
			DiaSetControl(dia, DFCO_CELLLIBRARY, 0);
			continue;
		}
		if (itemHit == DFCO_USETECHEDITSET)
		{
			/* clicked on button "Set" for all cells to be "Use technology editor on this cell" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits |= TECEDITCELL;
			}
			DiaSetControl(dia, DFCO_USETECHEDIT, 1);
			continue;
		}
		if (itemHit == DFCO_USETECHEDITCLR)
		{
			/* clicked on button "Clear" for all cells to be "Use technology editor on this cell" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits &= ~TECEDITCELL;
			}
			DiaSetControl(dia, DFCO_USETECHEDIT, 0);
			continue;
		}
		if (itemHit == DFCO_ALLOWALLMODCELLCLR)
		{
			/* clicked on button "Clear" for all cells to be "Disallow modification of anything in this cell" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits &= ~NPLOCKED;
			}
			DiaSetControl(dia, DFCO_ALLOWALLMODCELL, 0);
			continue;
		}
		if (itemHit == DFCO_ALLOWALLMODCELLSET)
		{
			/* clicked on button "Set" for all cells to be "Disallow modification of anything in this cell" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits |= NPLOCKED;
			}
			DiaSetControl(dia, DFCO_ALLOWALLMODCELL, 1);
			continue;
		}
		if (itemHit == DFCO_ALLOWINSTMODCELLCLR)
		{
			/* clicked on button "Clear" for all cells to be "Disallow modification of instances in this cell" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits &= ~NPILOCKED;
			}
			DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, 0);
			continue;
		}
		if (itemHit == DFCO_ALLOWINSTMODCELLSET)
		{
			/* clicked on button "Set" for all cells to be "Disallow modification of instances in this cell" */
			for(np = us_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				fi = (CELLINFO *)np->temp1;
				fi->newbits |= NPILOCKED;
			}
			DiaSetControl(dia, DFCO_ALLOWINSTMODCELL, 1);
			continue;
		}
		if (itemHit == DFCO_CHARSPACX)
		{
			/* changed X coordinate of characteristic spacing */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			pt = DiaGetText(dia, DFCO_CHARSPACX);
			if (fi->validcharacteristicspacing == 0 && *pt == 0) continue;
			if (fi->validcharacteristicspacing == 0)
				fi->characteristicspacing[1] = 0;
			fi->validcharacteristicspacing = 1;
			fi->characteristicspacing[0] = atola(pt, 0);
			continue;
		}
		if (itemHit == DFCO_CHARSPACY)
		{
			/* changed Y coordinate of characteristic spacing */
			if (thiscell == NONODEPROTO) continue;
			fi = (CELLINFO *)thiscell->temp1;
			pt = DiaGetText(dia, DFCO_CHARSPACY);
			if (fi->validcharacteristicspacing == 0 && *pt == 0) continue;
			if (fi->validcharacteristicspacing == 0)
				fi->characteristicspacing[0] = 0;
			fi->validcharacteristicspacing = 1;
			fi->characteristicspacing[1] = atola(pt, 0);
			continue;
		}
		if (itemHit == DFCO_TINYCELLGREY || itemHit == DFCO_CHECKDATES ||
			itemHit == DFCO_AUTOTECHSWITCH || itemHit == DFCO_PLACECELLCENTER)
		{
			/* clicked on a check box */
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		/* handle cell changes */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			fi = (CELLINFO *)np->temp1;
			if ((fi->newbits&(NPLOCKED|NPILOCKED|INCELLLIBRARY|WANTNEXPAND|TECEDITCELL)) !=
				(INTBIG)(np->userbits&(NPLOCKED|NPILOCKED|INCELLLIBRARY|WANTNEXPAND|TECEDITCELL)))
					(void)setval((INTBIG)np, VNODEPROTO, x_("userbits"), fi->newbits, VINTEGER);
			if (fi->validcharacteristicspacing != 0)
				setval((INTBIG)np, VNODEPROTO, x_("FACET_characteristic_spacing"),
					(INTBIG)fi->characteristicspacing, VINTEGER|VISARRAY|(2<<VLENGTHSH));
		}

		/* handle changes to all nodes */
		startobjectchange((INTBIG)us_tool, VTOOL);
		lx = us_useroptions;
		i = DiaGetControl(dia, DFCO_TINYCELLGREY);
		if (i == 0) lx |= DRAWTINYCELLS; else lx &= ~DRAWTINYCELLS;
		i = DiaGetControl(dia, DFCO_CHECKDATES);
		if (i != 0) lx |= CHECKDATE; else lx &= ~CHECKDATE;
		i = DiaGetControl(dia, DFCO_AUTOTECHSWITCH);
		if (i != 0) lx |= AUTOSWITCHTECHNOLOGY; else lx &= ~AUTOSWITCHTECHNOLOGY;
		i = DiaGetControl(dia, DFCO_PLACECELLCENTER);
		if (i != 0) lx |= CELLCENTERALWAYS; else lx &= ~CELLCENTERALWAYS;
		if (lx != us_useroptions)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, lx, VINTEGER);
		i = atofr(DiaGetText(dia, DFCO_TINYCELLTHRESH));
		if (i != us_tinyratio)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_tinylambdaperpixelkey, i, VFRACT);
		i = eatoi(DiaGetText(dia, DFCO_EXPLORERTEXTSIZE));
		if (i != exploretextsize)
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_facet_explorer_textsize"), i, VINTEGER);
			us_redoexplorerwindow();
		}
		endobjectchange((INTBIG)us_tool, VTOOL);
	}
	DiaDoneDialog(dia);
	efree((CHAR *)liblist);
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			efree((CHAR *)np->temp1);
	return(0);
}

/****************************** FIND TEXT DIALOG ******************************/

/* Text: Search and Replace */
static DIALOGITEM us_txtsardialogitems[] =
{
 /*  1 */ {0, {92,4,116,72}, BUTTON, N_("Find")},
 /*  2 */ {0, {124,328,148,396}, BUTTON, N_("Done")},
 /*  3 */ {0, {92,84,116,152}, BUTTON, N_("Replace")},
 /*  4 */ {0, {8,76,24,396}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,8,24,72}, MESSAGE, N_("Find:")},
 /*  6 */ {0, {36,76,52,396}, EDITTEXT, x_("")},
 /*  7 */ {0, {36,8,52,72}, MESSAGE, N_("Replace:")},
 /*  8 */ {0, {92,296,116,396}, BUTTON, N_("Replace All")},
 /*  9 */ {0, {64,216,80,344}, CHECK, N_("Find Reverse")},
 /* 10 */ {0, {64,80,80,208}, CHECK, N_("Case Sensitive")},
 /* 11 */ {0, {92,164,116,284}, BUTTON, N_("Replace and Find")},
 /* 12 */ {0, {128,4,144,96}, MESSAGE, N_("Line number:")},
 /* 13 */ {0, {128,100,144,180}, EDITTEXT, x_("")},
 /* 14 */ {0, {128,188,144,268}, BUTTON, N_("Go To Line")}
};
static DIALOG us_txtsardialog = {{75,75,232,481}, N_("Search and Replace"), 0, 14, us_txtsardialogitems, 0, 0};

/* special items for the "Text Search and Replace" dialog: */
#define DFDT_FIND             1		/* Find (button) */
#define DFDT_DONE             2		/* Done (button) */
#define DFDT_REPLACE          3		/* Replace (button) */
#define DFDT_WHATTOFIND       4		/* Find text (edit text) */
#define DFDT_WHATTOREPLACE    6		/* Replace text (edit text) */
#define DFDT_REPLACEALL       8		/* Replace all (button) */
#define DFDT_FINDREVERSE      9		/* Find reverse (check) */
#define DFDT_CASESENSITIVE   10		/* Case sensitive (check) */
#define DFDT_REPLACEANDFIND  11		/* Replace and Find (button) */
#define DFDT_LINENUMBER_L    12		/* Line number label (message) */
#define DFDT_LINENUMBER      13		/* Line number (edit text) */
#define DFDT_GOTOLINE        14		/* Go To line (button) */

INTBIG us_findtextdlog(void)
{
	REGISTER INTBIG itemHit, bits, i, tot;
	REGISTER BOOLEAN textsearch;
	static INTBIG reversedir = 0;
	static INTBIG casesensitive = 0;
	REGISTER CHAR *sea, *rep, *pt;
	CHAR line[100];
	REGISTER void *dia;

	if (us_needwindow()) return(0);
	textsearch = TRUE;
	if ((el_curwindowpart->state&WINDOWTYPE) != POPTEXTWINDOW &&
		(el_curwindowpart->state&WINDOWTYPE) != TEXTWINDOW)
			textsearch = FALSE;
	dia = DiaInitDialog(&us_txtsardialog);
	if (dia == 0) return(0);
	if (us_lastfindtextmessage != 0)
		DiaSetText(dia, DFDT_WHATTOFIND, us_lastfindtextmessage);
	if (us_lastreplacetextmessage != 0)
		DiaSetText(dia, DFDT_WHATTOREPLACE, us_lastreplacetextmessage);
	if (casesensitive != 0) DiaSetControl(dia, DFDT_CASESENSITIVE, 1);
	if (textsearch)
	{
		DiaUnDimItem(dia, DFDT_FINDREVERSE);
		DiaUnDimItem(dia, DFDT_GOTOLINE);
		DiaUnDimItem(dia, DFDT_LINENUMBER_L);
		DiaUnDimItem(dia, DFDT_LINENUMBER);
		if (reversedir != 0) DiaSetControl(dia, DFDT_FINDREVERSE, 1);
	} else
	{
		DiaDimItem(dia, DFDT_FINDREVERSE);
		DiaDimItem(dia, DFDT_GOTOLINE);
		DiaDimItem(dia, DFDT_LINENUMBER_L);
		DiaDimItem(dia, DFDT_LINENUMBER);
		us_initsearchcircuittext(el_curwindowpart);
	}
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DFDT_DONE) break;
		if (itemHit == DFDT_FIND)
		{
			/* find text */
			sea = DiaGetText(dia, DFDT_WHATTOFIND);
			bits = 0;
			if (DiaGetControl(dia, DFDT_FINDREVERSE) != 0) bits |= 8;
			if (DiaGetControl(dia, DFDT_CASESENSITIVE) != 0) bits |= 4;
			if (textsearch) us_searchtext(el_curwindowpart, sea, 0, bits); else
				us_searchcircuittext(el_curwindowpart, sea, 0, bits);
			continue;
		}
		if (itemHit == DFDT_REPLACE)
		{
			/* replace text */
			rep = DiaGetText(dia, DFDT_WHATTOREPLACE);
			if (textsearch)
			{
				for(pt = rep; *pt != 0; pt++)
					(void)us_gotchar(el_curwindowpart, *pt, 0);
			} else
			{
				us_replacecircuittext(el_curwindowpart, rep);
			}
			continue;
		}
		if (itemHit == DFDT_REPLACEANDFIND)
		{
			/* replace and find text */
			sea = DiaGetText(dia, DFDT_WHATTOFIND);
			rep = DiaGetText(dia, DFDT_WHATTOREPLACE);
			bits = 0;
			if (DiaGetControl(dia, DFDT_FINDREVERSE) != 0) bits |= 8;
			if (DiaGetControl(dia, DFDT_CASESENSITIVE) != 0) bits |= 4;
			if (textsearch)
			{
				for(pt = rep; *pt != 0; pt++)
					(void)us_gotchar(el_curwindowpart, *pt, 0);
				us_searchtext(el_curwindowpart, sea, 0, bits);
			} else
			{
				us_replacecircuittext(el_curwindowpart, rep);
				us_searchcircuittext(el_curwindowpart, sea, 0, bits);
			}
			continue;
		}
		if (itemHit == DFDT_REPLACEALL)
		{
			/* replace all */
			sea = DiaGetText(dia, DFDT_WHATTOFIND);
			rep = DiaGetText(dia, DFDT_WHATTOREPLACE);
			bits = 2;
			if (DiaGetControl(dia, DFDT_FINDREVERSE) != 0) bits |= 8;
			if (DiaGetControl(dia, DFDT_CASESENSITIVE) != 0) bits |= 4;
			if (textsearch)
				us_searchtext(el_curwindowpart, sea, rep, bits); else
			{
				us_initsearchcircuittext(el_curwindowpart);
				us_searchcircuittext(el_curwindowpart, sea, rep, bits);
			}
			continue;
		}
		if (itemHit == DFDT_GOTOLINE)
		{
			/* go to line */
			i = eatoi(DiaGetText(dia, DFDT_LINENUMBER)) - 1;
			if (i < 0) continue;
			tot = us_totallines(el_curwindowpart);
			if (i > tot)
			{
				esnprintf(line, 100, x_("%ld"), tot);
				DiaSetText(dia, DFDT_LINENUMBER, line);
				continue;
			}
			us_highlightline(el_curwindowpart, i, i);
			continue;
		}
		if (itemHit == DFDT_FINDREVERSE || itemHit == DFDT_CASESENSITIVE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			if (!textsearch) us_initsearchcircuittext(el_curwindowpart);
			continue;
		}
		if (itemHit == DFDT_WHATTOFIND)
		{
			if (!textsearch) us_initsearchcircuittext(el_curwindowpart);
			continue;
		}
	}

	/* save state for next use of this dialog */
	if (us_lastfindtextmessage != 0) efree((CHAR *)us_lastfindtextmessage);
	(void)allocstring(&us_lastfindtextmessage, DiaGetText(dia, DFDT_WHATTOFIND), us_tool->cluster);
	if (us_lastreplacetextmessage != 0) efree((CHAR *)us_lastreplacetextmessage);
	(void)allocstring(&us_lastreplacetextmessage, DiaGetText(dia, DFDT_WHATTOREPLACE), us_tool->cluster);
	reversedir = DiaGetControl(dia, DFDT_FINDREVERSE);
	casesensitive = DiaGetControl(dia, DFDT_CASESENSITIVE);

	DiaDoneDialog(dia);
	return(0);
}

/****************************** FRAME OPTIONS DIALOG ******************************/

/* Drawing Options */
static DIALOGITEM us_drawingoptdialogitems[] =
{
 /*  1 */ {0, {48,456,72,520}, BUTTON, N_("OK")},
 /*  2 */ {0, {12,456,36,520}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {116,4,132,112}, MESSAGE, N_("Company Name:")},
 /*  4 */ {0, {92,116,108,232}, MESSAGE, N_("Default:")},
 /*  5 */ {0, {140,4,156,112}, MESSAGE, N_("Designer Name:")},
 /*  6 */ {0, {164,4,180,112}, MESSAGE, N_("Project Name:")},
 /*  7 */ {0, {44,4,60,100}, MESSAGE, N_("Frame size:")},
 /*  8 */ {0, {44,104,60,220}, POPUP, x_("")},
 /*  9 */ {0, {32,228,48,324}, RADIO, N_("Landscape")},
 /* 10 */ {0, {56,228,72,324}, RADIO, N_("Portrait")},
 /* 11 */ {0, {8,4,24,404}, MESSAGE, N_("For cell:")},
 /* 12 */ {0, {116,116,132,316}, EDITTEXT, x_("")},
 /* 13 */ {0, {80,4,80,524}, DIVIDELINE, x_("")},
 /* 14 */ {0, {140,116,156,316}, EDITTEXT, x_("")},
 /* 15 */ {0, {164,116,180,316}, EDITTEXT, x_("")},
 /* 16 */ {0, {92,324,108,384}, MESSAGE, N_("Library:")},
 /* 17 */ {0, {92,384,108,524}, POPUP, x_("")},
 /* 18 */ {0, {116,324,132,524}, EDITTEXT, x_("")},
 /* 19 */ {0, {140,324,156,524}, EDITTEXT, x_("")},
 /* 20 */ {0, {164,324,180,524}, EDITTEXT, x_("")},
 /* 21 */ {0, {44,332,60,444}, CHECK, N_("Title Box")}
};
static DIALOG us_drawingoptdialog = {{50,75,239,609}, N_("Frame Options"), 0, 21, us_drawingoptdialogitems, 0, 0};

/* special items for the "Drawing Options" dialog: */
#define DFRO_FRAMESIZE      8		/* Frame size (popup) */
#define DFRO_LANDSCAPE      9		/* Landscape (radio) */
#define DFRO_PORTRAIT      10		/* Portrait (radio) */
#define DFRO_CELLNAME      11		/* Cell name (stat text) */
#define DFRO_COMPANYNAME   12		/* Company Name (edit text) */
#define DFRO_DESIGNERNAME  14		/* Designer Name (edit text) */
#define DFRO_PROJECTAME    15		/* Project name (edit text) */
#define DFRO_LIBLIST       17		/* List of libraries (popup) */
#define DFRO_LCOMPANYNAME  18		/* Company Name for library (edit text) */
#define DFRO_LDESIGNERNAME 19		/* Designer Name for library (edit text) */
#define DFRO_LPROJECTAME   20		/* Project name for library (edit text) */
#define DFRO_TITLEBOX      21		/* Draw title box (check) */

INTBIG us_frameoptionsdlog(void)
{
	INTBIG itemHit, i, newframesize, vertical, newvertical, total, title, newtitle,
		curframesize, origframesize;
	BOOLEAN redraw;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib, *curlib;
	REGISTER WINDOWPART *w;
	REGISTER CHAR *pt;
	CHAR line[10], *newlang[7], **liblist, **threestrings;
	static CHAR *framenames[] = {N_("None"), N_("Half-A-Size"), N_("A-Size"),
		N_("B-Size"), N_("C-Size"), N_("D-Size"), N_("E-Size")};
	REGISTER void *infstr, *dia;

	/* show the frame options dialog */
	dia = DiaInitDialog(&us_drawingoptdialog);
	if (dia == 0) return(0);
	liblist = us_makelibrarylist(&total, el_curlib, &i);
	DiaSetPopup(dia, DFRO_LIBLIST, total, liblist);
	if (i >= 0) DiaSetPopupEntry(dia, DFRO_LIBLIST, i);

	/* defaults for all libraries */
	var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_company_name"));
	if (var != NOVARIABLE) DiaSetText(dia, DFRO_COMPANYNAME, (CHAR *)var->addr);
	var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_designer_name"));
	if (var != NOVARIABLE) DiaSetText(dia, DFRO_DESIGNERNAME, (CHAR *)var->addr);
	var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_project_name"));
	if (var != NOVARIABLE) DiaSetText(dia, DFRO_PROJECTAME, (CHAR *)var->addr);

	/* for the current library */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		threestrings = (CHAR **)emalloc(3 * (sizeof (CHAR *)), us_tool->cluster);
		if (threestrings == 0) return(0);
		var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("USER_drawing_company_name"));
		if (var != NOVARIABLE) pt = (CHAR *)var->addr; else pt = x_("");
		allocstring(&threestrings[0], pt, us_tool->cluster);
		var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("USER_drawing_designer_name"));
		if (var != NOVARIABLE) pt = (CHAR *)var->addr; else pt = x_("");
		allocstring(&threestrings[1], pt, us_tool->cluster);
		var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("USER_drawing_project_name"));
		if (var != NOVARIABLE) pt = (CHAR *)var->addr; else pt = x_("");
		allocstring(&threestrings[2], pt, us_tool->cluster);
		lib->temp1 = (INTBIG)threestrings;
	}
	threestrings = (CHAR **)el_curlib->temp1;
	DiaSetText(dia, DFRO_LCOMPANYNAME, threestrings[0]);
	DiaSetText(dia, DFRO_LDESIGNERNAME, threestrings[1]);
	DiaSetText(dia, DFRO_LPROJECTAME, threestrings[2]);
	curlib = el_curlib;

	np = getcurcell();
	if (np != NONODEPROTO)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("For cell: "));
		addstringtoinfstr(infstr, describenodeproto(np));
		DiaSetText(dia, DFRO_CELLNAME, returninfstr(infstr));
		for(i=0; i<7; i++) newlang[i] = TRANSLATE(framenames[i]);
		DiaSetPopup(dia, DFRO_FRAMESIZE, 7, newlang);
		DiaUnDimItem(dia, DFRO_LANDSCAPE);
		DiaUnDimItem(dia, DFRO_PORTRAIT);
		DiaUnDimItem(dia, DFRO_TITLEBOX);
		curframesize = 0;
		title = 0;
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, el_schematic_page_size_key);
		if (var != NOVARIABLE)
		{
			pt = (CHAR *)var->addr;
			if (*pt == 'x') curframesize = 0; else
			if (*pt == 'h') curframesize = 1; else
			if (*pt == 'a') curframesize = 2; else
			if (*pt == 'b') curframesize = 3; else
			if (*pt == 'c') curframesize = 4; else
			if (*pt == 'd') curframesize = 5; else
			if (*pt == 'e') curframesize = 6; else curframesize = 5;
			pt++;
			DiaSetPopupEntry(dia, DFRO_FRAMESIZE, curframesize);
			vertical = 0;
			title = 1;
			while (*pt != 0)
			{
				if (*pt == 'v') vertical = 1;
				if (*pt == 'n') title = 0;
				pt++;
			}
			if (curframesize > 0)
			{
				if (vertical != 0) DiaSetControl(dia, DFRO_PORTRAIT, 1); else
					DiaSetControl(dia, DFRO_LANDSCAPE, 1);
			}
		}
		if (title != 0) DiaSetControl(dia, DFRO_TITLEBOX, 1);
		origframesize = curframesize;
	} else
	{
		DiaSetText(dia, DFRO_CELLNAME, _("No cell in window"));
		DiaDimItem(dia, DFRO_LANDSCAPE);
		DiaDimItem(dia, DFRO_PORTRAIT);
		DiaDimItem(dia, DFRO_TITLEBOX);
	}
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DFRO_FRAMESIZE)
		{
			i = DiaGetPopupEntry(dia, DFRO_FRAMESIZE);
			if (i == 0)
			{
				DiaSetControl(dia, DFRO_LANDSCAPE, 0);
				DiaSetControl(dia, DFRO_PORTRAIT, 0);
			} else
			{
				if (DiaGetControl(dia, DFRO_LANDSCAPE) == 0 && DiaGetControl(dia, DFRO_PORTRAIT) == 0)
					DiaSetControl(dia, DFRO_LANDSCAPE, 1);
				if (curframesize == 0) DiaSetControl(dia, DFRO_TITLEBOX, 1);
			}
			curframesize = i;
		}
		if (itemHit == DFRO_TITLEBOX)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DFRO_LANDSCAPE || itemHit == DFRO_PORTRAIT)
		{
			DiaSetControl(dia, DFRO_LANDSCAPE, 0);
			DiaSetControl(dia, DFRO_PORTRAIT, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DFRO_LCOMPANYNAME)
		{
			threestrings = (CHAR **)curlib->temp1;
			pt = DiaGetText(dia, DFRO_LCOMPANYNAME);
			reallocstring(&threestrings[0], pt, us_tool->cluster);
			continue;
		}
		if (itemHit == DFRO_LDESIGNERNAME)
		{
			threestrings = (CHAR **)curlib->temp1;
			pt = DiaGetText(dia, DFRO_LDESIGNERNAME);
			reallocstring(&threestrings[1], pt, us_tool->cluster);
			continue;
		}
		if (itemHit == DFRO_LPROJECTAME)
		{
			threestrings = (CHAR **)curlib->temp1;
			pt = DiaGetText(dia, DFRO_LPROJECTAME);
			reallocstring(&threestrings[2], pt, us_tool->cluster);
			continue;
		}
		if (itemHit == DFRO_LIBLIST)
		{
			i = DiaGetPopupEntry(dia, DFRO_LIBLIST);
			lib = getlibrary(liblist[i]);
			if (lib == NOLIBRARY) continue;
			curlib = lib;
			threestrings = (CHAR **)curlib->temp1;
			DiaSetText(dia, DFRO_LCOMPANYNAME, threestrings[0]);
			DiaSetText(dia, DFRO_LDESIGNERNAME, threestrings[1]);
			DiaSetText(dia, DFRO_LPROJECTAME, threestrings[2]);
			continue;
		}
	}
	if (itemHit == OK)
	{
		redraw = FALSE;
		if (np != NONODEPROTO)
		{
			newframesize = DiaGetPopupEntry(dia, DFRO_FRAMESIZE);
			newvertical = DiaGetControl(dia, DFRO_PORTRAIT);
			newtitle = DiaGetControl(dia, DFRO_TITLEBOX);
			if (newframesize != origframesize || newvertical != vertical || newtitle != title)
			{
				redraw = TRUE;
				var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, el_schematic_page_size_key);
				if (newframesize == 0 && newtitle == 0)
				{
					if (var != NOVARIABLE)
						(void)delvalkey((INTBIG)np, VNODEPROTO, el_schematic_page_size_key);
				} else
				{
					pt = line;
					switch (newframesize)
					{
						case 0: *pt++ = 'x';   break;
						case 1: *pt++ = 'h';   break;
						case 2: *pt++ = 'a';   break;
						case 3: *pt++ = 'b';   break;
						case 4: *pt++ = 'c';   break;
						case 5: *pt++ = 'd';   break;
						case 6: *pt++ = 'e';   break;
					}
					if (newvertical != 0) *pt++ = 'v';
					if (newtitle == 0) *pt++ = 'n';
					*pt = 0;
					if (var == NOVARIABLE || estrcmp(line, (CHAR *)var->addr) != 0)
						(void)setvalkey((INTBIG)np, VNODEPROTO, el_schematic_page_size_key,
							(INTBIG)line, VSTRING);
				}
			}
		}

		pt = DiaGetText(dia, DFRO_COMPANYNAME);
		while (*pt == ' ' || *pt == '\t') pt++;
		var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_company_name"));
		if (*pt == 0)
		{
			/* remove company information */
			if (var != NOVARIABLE)
				(void)delval((INTBIG)us_tool, VTOOL, x_("USER_drawing_company_name"));
		} else
		{
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
				(void)setval((INTBIG)us_tool, VTOOL, x_("USER_drawing_company_name"), (INTBIG)pt, VSTRING);
		}

		pt = DiaGetText(dia, DFRO_DESIGNERNAME);
		while (*pt == ' ' || *pt == '\t') pt++;
		var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_designer_name"));
		if (*pt == 0)
		{
			/* remove designer information */
			if (var != NOVARIABLE)
				(void)delval((INTBIG)us_tool, VTOOL, x_("USER_drawing_designer_name"));
		} else
		{
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
				(void)setval((INTBIG)us_tool, VTOOL, x_("USER_drawing_designer_name"), (INTBIG)pt, VSTRING);
		}

		pt = DiaGetText(dia, DFRO_PROJECTAME);
		while (*pt == ' ' || *pt == '\t') pt++;
		var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_drawing_project_name"));
		if (*pt == 0)
		{
			/* remove designer information */
			if (var != NOVARIABLE)
				(void)delval((INTBIG)us_tool, VTOOL, x_("USER_drawing_project_name"));
		} else
		{
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
				(void)setval((INTBIG)us_tool, VTOOL, x_("USER_drawing_project_name"), (INTBIG)pt, VSTRING);
		}

		/* save library-specific information */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			threestrings = (CHAR **)lib->temp1;
			var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("USER_drawing_company_name"));
			if (*threestrings[0] == 0)
			{
				/* remove company information */
				if (var != NOVARIABLE)
					(void)delval((INTBIG)lib, VLIBRARY, x_("USER_drawing_company_name"));
			} else
			{
				if (var == NOVARIABLE || estrcmp(threestrings[0], (CHAR *)var->addr) != 0)
					(void)setval((INTBIG)lib, VLIBRARY, x_("USER_drawing_company_name"), (INTBIG)threestrings[0], VSTRING);
			}

			var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("USER_drawing_designer_name"));
			if (*threestrings[1] == 0)
			{
				/* remove company information */
				if (var != NOVARIABLE)
					(void)delval((INTBIG)lib, VLIBRARY, x_("USER_drawing_designer_name"));
			} else
			{
				if (var == NOVARIABLE || estrcmp(threestrings[1], (CHAR *)var->addr) != 0)
					(void)setval((INTBIG)lib, VLIBRARY, x_("USER_drawing_designer_name"), (INTBIG)threestrings[1], VSTRING);
			}

			var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("USER_drawing_project_name"));
			if (*threestrings[2] == 0)
			{
				/* remove company information */
				if (var != NOVARIABLE)
					(void)delval((INTBIG)lib, VLIBRARY, x_("USER_drawing_project_name"));
			} else
			{
				if (var == NOVARIABLE || estrcmp(threestrings[2], (CHAR *)var->addr) != 0)
					(void)setval((INTBIG)lib, VLIBRARY, x_("USER_drawing_project_name"), (INTBIG)threestrings[2], VSTRING);
			}
		}

		/* redraw if needed */
		if (redraw && np != NONODEPROTO)
		{
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if ((w->state&WINDOWTYPE) != DISPWINDOW) continue;
				if (w->curnodeproto != np) continue;
				if (w->redisphandler != 0) (*w->redisphandler)(w);
			}
		}
	}
	DiaDoneDialog(dia);
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		threestrings = (CHAR **)lib->temp1;
		efree((CHAR *)threestrings[0]);
		efree((CHAR *)threestrings[1]);
		efree((CHAR *)threestrings[2]);
		efree((CHAR *)threestrings);
	}
	efree((CHAR *)liblist);
	return(0);
}

/****************************** GENERAL OPTIONS DIALOG ******************************/

/* User Interface: General Options */
static DIALOGITEM us_genoptdialogitems[] =
{
 /*  1 */ {0, {180,268,204,348}, BUTTON, N_("OK")},
 /*  2 */ {0, {180,152,204,232}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,216}, CHECK, N_("Beep after long jobs")},
 /*  4 */ {0, {80,8,96,308}, CHECK, N_("Include date and version in output files")},
 /*  5 */ {0, {180,40,204,120}, BUTTON, N_("Advanced")},
 /*  6 */ {0, {128,8,144,308}, MESSAGE, N_("Maximum errors to report (0 for infinite):")},
 /*  7 */ {0, {128,312,144,372}, EDITTEXT, x_("")},
 /*  8 */ {0, {56,8,72,308}, CHECK, N_("Expandable dialogs default to fullsize")},
 /*  9 */ {0, {32,8,48,308}, CHECK, N_("Click sounds when arcs are created")},
 /* 10 */ {0, {152,8,168,252}, MESSAGE, N_("Prevent motion after selection for:")},
 /* 11 */ {0, {152,256,168,304}, EDITTEXT, x_("")},
 /* 12 */ {0, {152,308,168,384}, MESSAGE, N_("seconds")},
 /* 13 */ {0, {104,8,120,384}, CHECK, N_("Show file-selection dialog before writing netlists")}
};
static DIALOG us_genoptdialog = {{75,75,288,469}, N_("General Options"), 0, 13, us_genoptdialogitems, 0, 0};

/* special items for the "General Options" dialog: */
#define DGNO_BEEPAFTERLONG    3		/* Beep after long jobs (check) */
#define DGNO_PUTDATEINOUTPUT  4		/* Put date in output files (check) */
#define DGNO_ADVANCEDSTUFF    5		/* Handle advanced stuff (button) */
#define DGNO_MAXERRORS        7		/* Maximum errors to report (edit text) */
#define DGNO_FULLSIZEDIALOGS  8		/* Start expandable dialogs fullsize (check) */
#define DGNO_OTHERSOUNDS      9		/* Play other sounds (check) */
#define DGNO_MOTIONDELAY     11		/* Motion delay after selection (edit text) */
#define DGNO_FILESELECTDIA   13		/* Show file-selection dialog when writing netlists (check) */

/* User Interface: Advanced */
static DIALOGITEM us_advdialogitems[] =
{
 /*  1 */ {0, {8,8,24,268}, MESSAGE, N_("These are advanced commands")},
 /*  2 */ {0, {14,276,38,352}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {152,188,176,348}, BUTTON, N_("Edit Dialogs")},
 /*  4 */ {0, {88,16,112,176}, BUTTON, N_("List Changed Options")},
 /*  5 */ {0, {184,188,208,348}, BUTTON, N_("Examine Memory Arena")},
 /*  6 */ {0, {184,16,208,176}, BUTTON, N_("Dump Name Space")},
 /*  7 */ {0, {152,16,176,176}, BUTTON, N_("Show R-Tree for Cell")},
 /*  8 */ {0, {88,188,112,348}, BUTTON, N_("Save Options as Text")},
 /*  9 */ {0, {120,16,144,176}, BUTTON, N_("Interactive Undo")},
 /* 10 */ {0, {120,188,144,348}, BUTTON, N_("Toggle Internal Errors")},
 /* 11 */ {0, {54,188,78,348}, BUTTON, N_("Edit Variables")},
 /* 12 */ {0, {28,8,44,268}, MESSAGE, N_("And should not normally be used")},
 /* 13 */ {0, {54,16,78,176}, BUTTON, N_("Language Translation")}
};
static DIALOG us_advdialog = {{75,75,292,432}, N_("Advanced Users Only"), 0, 13, us_advdialogitems, 0, 0};

/* special items for the "Advanced" dialog: */
#define DADV_DIAEDIT      3		/* Edit dialogs (button) */
#define DADV_CHANGEDOPT   4		/* List changed options (button) */
#define DADV_MEMARENA     5		/* Dump memory arena (button) */
#define DADV_NAMESPACE    6		/* Dump name space (button) */
#define DADV_RTREES       7		/* Dump R-tree for current cell (button) */
#define DADV_SAVEOPT      8		/* Save options as readable dump (button) */
#define DADV_UNDO         9		/* Interactive Undo (button) */
#define DADV_INTERNAL    10		/* Toggle internal errors (button) */
#define DADV_VARIABLES   11		/* Edit variables (button) */
#define DADV_TRANSLATE   13		/* Translate (button) */

INTBIG us_generaloptionsdlog(void)
{
	INTBIG itemHit, newoptions, maxerrors, i, motiondelay;
	REGISTER VARIABLE *var;
	CHAR *par[2], line[50];
	REGISTER void *dia;

	/* show the frame options dialog */
	dia = DiaInitDialog(&us_genoptdialog);
	if (dia == 0) return(0);
	if ((us_useroptions&NODATEORVERSION) == 0) DiaSetControl(dia, DGNO_PUTDATEINOUTPUT, 1);
	if ((us_useroptions&NOPROMPTBEFOREWRITE) == 0) DiaSetControl(dia, DGNO_FILESELECTDIA, 1);
	if ((us_useroptions&BEEPAFTERLONGJOB) != 0) DiaSetControl(dia, DGNO_BEEPAFTERLONG, 1);
	if ((us_useroptions&EXPANDEDDIALOGSDEF) != 0) DiaSetControl(dia, DGNO_FULLSIZEDIALOGS, 1);
	if ((us_useroptions&NOEXTRASOUND) == 0) DiaSetControl(dia, DGNO_OTHERSOUNDS, 1);

	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_maximum_errors"));
	if (var == NOVARIABLE) maxerrors = 0; else
		maxerrors = var->addr;
	esnprintf(line, 50, x_("%ld"), maxerrors);
	DiaSetText(dia, DGNO_MAXERRORS, line);

	var = getvalkey((INTBIG)us_tool, VTOOL, VFRACT, us_motiondelaykey);
	if (var == NOVARIABLE) motiondelay = WHOLE/2; else
		motiondelay = var->addr;
	DiaSetText(dia, DGNO_MOTIONDELAY, frtoa(motiondelay));

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK || itemHit == DGNO_ADVANCEDSTUFF) break;
		if (itemHit == DGNO_PUTDATEINOUTPUT || itemHit == DGNO_BEEPAFTERLONG ||
			itemHit == DGNO_FULLSIZEDIALOGS || itemHit == DGNO_OTHERSOUNDS ||
			itemHit == DGNO_FILESELECTDIA)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		newoptions = us_useroptions & ~(NODATEORVERSION | BEEPAFTERLONGJOB |
			EXPANDEDDIALOGSDEF | NOEXTRASOUND | NOPROMPTBEFOREWRITE);
		if (DiaGetControl(dia, DGNO_PUTDATEINOUTPUT) == 0) newoptions |= NODATEORVERSION;
		if (DiaGetControl(dia, DGNO_FILESELECTDIA) == 0) newoptions |= NOPROMPTBEFOREWRITE;
		if (DiaGetControl(dia, DGNO_BEEPAFTERLONG) != 0) newoptions |= BEEPAFTERLONGJOB;
		if (DiaGetControl(dia, DGNO_FULLSIZEDIALOGS) != 0) newoptions |= EXPANDEDDIALOGSDEF;
		if (DiaGetControl(dia, DGNO_OTHERSOUNDS) == 0) newoptions |= NOEXTRASOUND;
		if (newoptions != us_useroptions)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, newoptions, VINTEGER);
		i = eatoi(DiaGetText(dia, DGNO_MAXERRORS));
		if (i != maxerrors)
		{
			if (i == 0)
			{
				if (getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_maximum_errors")) != NOVARIABLE)
					delval((INTBIG)us_tool, VTOOL, x_("USER_maximum_errors"));
			} else
			{
				(void)setval((INTBIG)us_tool, VTOOL, x_("USER_maximum_errors"), i, VINTEGER);
			}
		}
		i = atofr(DiaGetText(dia, DGNO_MOTIONDELAY));
		if (i != motiondelay)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_motiondelaykey, i, VFRACT);
	}
	DiaDoneDialog(dia);

	/* handle advanced dialog if requested */
	if (itemHit == DGNO_ADVANCEDSTUFF)
	{
		dia = DiaInitDialog(&us_advdialog);
		if (dia == 0) return(0);
		for(;;)
		{
			itemHit = DiaNextHit(dia);
			if (itemHit == CANCEL) break;
			if (itemHit == DADV_DIAEDIT)
			{
				DiaDoneDialog(dia);
				par[0] = x_("dialog-edit");
				us_debug(1, par);
				return(0);
			}
			if (itemHit == DADV_CHANGEDOPT)
			{
				par[0] = x_("options-changed");
				us_debug(1, par);
				break;
			}
			if (itemHit == DADV_MEMARENA)
			{
				par[0] = x_("arena");
				us_debug(1, par);
				break;
			}
			if (itemHit == DADV_NAMESPACE)
			{
				par[0] = x_("namespace");
				us_debug(1, par);
				break;
			}
			if (itemHit == DADV_RTREES)
			{
				par[0] = x_("rtree");
				us_debug(1, par);
				break;
			}
			if (itemHit == DADV_SAVEOPT)
			{
				DiaDoneDialog(dia);
				par[0] = x_("examine-options");
				us_debug(1, par);
				return(0);
			}
			if (itemHit == DADV_UNDO)
			{
				DiaDoneDialog(dia);
				par[0] = x_("undo");
				us_debug(1, par);
				return(0);
			}
			if (itemHit == DADV_INTERNAL)
			{
				par[0] = x_("internal-errors");
				us_debug(1, par);
				break;
			}
			if (itemHit == DADV_VARIABLES)
			{
				DiaDoneDialog(dia);
				(void)us_variablesdlog();
				return(0);
			}
			if (itemHit == DADV_TRANSLATE)
			{
				DiaDoneDialog(dia);
				us_translationdlog();
				return(0);
			}
		}
		DiaDoneDialog(dia);
	}
	return(0);
}

/****************************** GET INFO DIALOGS ******************************/

static void   us_getinfonode(NODEINST *ni, BOOLEAN canspecialize);
static void   us_getinfoarc(ARCINST *ai);
static void   us_getinfotext(HIGHLIGHT *high, BOOLEAN canspecialize);
static void   us_getinfoexport(HIGHLIGHT *high);
static void   us_getnodedisplaysize(NODEINST *ni, INTBIG *xsize, INTBIG *ysize);
static void   us_getnodemodfromdisplayinfo(NODEINST *ni, INTBIG xs, INTBIG ys, INTBIG xc, INTBIG yc,
				INTBIG r, INTBIG t, BOOLEAN positionchanged, INTBIG *dlx, INTBIG *dly, INTBIG *dhx,
				INTBIG *dhy, INTBIG *drot, INTBIG *dtran, BOOLEAN xyrev);
static void   us_listmanyhighlights(INTBIG len, HIGHLIGHT *manyhigh, INTBIG *whichone, INTBIG *oftotal, void *dia);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif
	static int    us_highlightsascending(const void *e1, const void *e2);
#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

/* Multiple Object Info */
static DIALOGITEM us_multigetinfodialogitems[] =
{
 /*  1 */ {0, {368,124,392,204}, BUTTON, N_("OK")},
 /*  2 */ {0, {368,16,392,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {20,4,360,304}, SCROLLMULTI, x_("")},
 /*  4 */ {0, {0,4,16,304}, MESSAGE, N_("0 Objects:")},
 /*  5 */ {0, {28,308,44,388}, MESSAGE, N_("X Position:")},
 /*  6 */ {0, {28,392,44,472}, EDITTEXT, x_("")},
 /*  7 */ {0, {68,308,84,388}, MESSAGE, N_("Y Position:")},
 /*  8 */ {0, {68,392,84,472}, EDITTEXT, x_("")},
 /*  9 */ {0, {108,308,124,388}, MESSAGE, N_("X Size:")},
 /* 10 */ {0, {108,392,124,472}, EDITTEXT, x_("")},
 /* 11 */ {0, {148,308,164,388}, MESSAGE, N_("Y Size:")},
 /* 12 */ {0, {148,392,164,472}, EDITTEXT, x_("")},
 /* 13 */ {0, {220,308,236,388}, MESSAGE, N_("Width:")},
 /* 14 */ {0, {220,392,236,472}, EDITTEXT, x_("")},
 /* 15 */ {0, {8,308,24,472}, MESSAGE, N_("For all selected nodes:")},
 /* 16 */ {0, {200,308,216,472}, MESSAGE, N_("For all selected arcs:")},
 /* 17 */ {0, {368,232,392,312}, BUTTON, N_("Info")},
 /* 18 */ {0, {368,340,392,420}, BUTTON, N_("Remove")},
 /* 19 */ {0, {344,308,360,472}, POPUP, x_("")},
 /* 20 */ {0, {48,336,64,472}, MESSAGE, x_("")},
 /* 21 */ {0, {88,336,104,472}, MESSAGE, x_("")},
 /* 22 */ {0, {128,336,144,472}, MESSAGE, x_("")},
 /* 23 */ {0, {168,336,184,472}, MESSAGE, x_("")},
 /* 24 */ {0, {240,336,256,472}, MESSAGE, x_("")},
 /* 25 */ {0, {192,308,193,472}, DIVIDELINE, x_("")},
 /* 26 */ {0, {272,308,288,472}, MESSAGE, N_("For all selected exports:")},
 /* 27 */ {0, {264,308,265,472}, DIVIDELINE, x_("")},
 /* 28 */ {0, {292,308,308,472}, POPUP, x_("")},
 /* 29 */ {0, {316,308,317,472}, DIVIDELINE, x_("")},
 /* 30 */ {0, {324,308,340,472}, MESSAGE, N_("For everything:")}
};
static DIALOG us_multigetinfodialog = {{75,75,476,556}, N_("Multiple Object Information"), 0, 30, us_multigetinfodialogitems, 0, 0};

/* special items for the "Multiple-object Get Info" dialog: */
#define DGIM_OBJECTLIST   3		/* list of objects (scroll) */
#define DGIM_OBJECTCOUNT  4		/* object count (stat text) */
#define DGIM_NODEXPOS     6		/* node X position (edit text) */
#define DGIM_NODEYPOS     8		/* node Y position (edit text) */
#define DGIM_NODEXSIZE   10		/* node X size (edit text) */
#define DGIM_NODEYSIZE   12		/* node Y size (edit text) */
#define DGIM_ARCWIDTH    14		/* arc width (edit text) */
#define DGIM_INFO        17		/* info (button) */
#define DGIM_REMOVE      18		/* remove (button) */
#define DGIM_SELECTION   19		/* selection (popup) */
#define DGIM_NODEXPRANGE 20		/* range of node X position values (stat text) */
#define DGIM_NODEYPRANGE 21		/* range of node Y position values (stat text) */
#define DGIM_NODEXSRANGE 22		/* range of node X size values (stat text) */
#define DGIM_NODEYSRANGE 23		/* range of node Y size values (stat text) */
#define DGIM_ARCWIDRANGE 24		/* range of arc width values (stat text) */
#define DGIM_EXPORTCHARS 28		/* characteristics of all exports (popup) */

INTBIG us_showdlog(BOOLEAN canspecialize)
{
	HIGHLIGHT high, *manyhigh;
	CHAR *pt, *newlang[NUMPORTCHARS+1];
	REGISTER NODEINST *ni, *ni1, *ni2, **nis, *onenode;
	REGISTER ARCINST *ai, *onearc;
	REGISTER GEOM *geom, *got1, *got2;
	REGISTER BOOLEAN nodeinfochanged, arcinfochanged, positionchanged, xyrev, highlightchanged;
	REGISTER INTBIG len, i, j, k, swap, distx, disty, cx, cy, gotjustone, *linelist,
		nodistx, nodisty, value, *whichone, *oftotal, *dlxs, *dlys, *dhxs, *dhys,
		*drots, *dtrans, newbit, lambda;
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2,
		xpos, ypos, xsize, ysize, cx1, cy1, cx2, cy2;
	XARRAY trans;
	REGISTER INTBIG itemHit, which;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* if in outline-edit mode, show the points */
	if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
	{
		us_tracedlog();
		return(0);
	}

	/* see if anything is highlighted */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE)
	{
		us_abortcommand(_("Nothing is highlighted"));
		return(0);
	}

	/* special dialog when only 1 highlighted object */
	gotjustone = -1;
	onenode = NONODEINST;
	onearc = NOARCINST;
	len = getlength(var);
	for(i=0; i<len; i++)
	{
		if (us_makehighlight(((CHAR **)var->addr)[i], &high)) continue;
		if ((high.status&HIGHTYPE) != HIGHFROM) continue;
		if (high.fromgeom == NOGEOM)
		{
			gotjustone = -1;
			break;
		}
		gotjustone = i;
		if (!high.fromgeom->entryisnode)
		{
			ai = high.fromgeom->entryaddr.ai;
			if (onearc == NOARCINST) onearc = ai;
			if (onearc == ai) continue;
			gotjustone = -1;
			break;
		} else
		{
			ni = high.fromgeom->entryaddr.ni;
			if (onenode == NONODEINST) onenode = ni;
			if (onenode == ni) continue;
			gotjustone = -1;
			break;
		}
	}
	if (onenode != NONODEINST && onearc != NOARCINST) gotjustone = -1;
	if (len == 1) gotjustone = 0;
	if (gotjustone >= 0)
	{
		/* get the one highlighted object */
		if (us_makehighlight(((CHAR **)var->addr)[gotjustone], &high))
		{
			us_abortcommand(_("Highlight unintelligible"));
			return(0);
		}

		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (high.fromvar == NOVARIABLE && high.fromport != NOPORTPROTO)
				us_getinfoexport(&high); else
					us_getinfotext(&high, canspecialize);
			return(0);
		}
		if ((high.status&HIGHTYPE) == HIGHFROM)
		{
			if (!high.fromgeom->entryisnode)
			{
				/* arc getinfo */
				ai = (ARCINST *)high.fromgeom->entryaddr.ai;
				us_getinfoarc(ai);
				return(0);
			}
			ni = (NODEINST *)high.fromgeom->entryaddr.ni;
			us_getinfonode(ni, canspecialize);
			return(0);
		}
	}

	/* multiple objects highlighted: remember them all */
	manyhigh = (HIGHLIGHT *)emalloc(len * (sizeof (HIGHLIGHT)), el_tempcluster);
	if (manyhigh == 0) return(0);
	whichone = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
	if (whichone == 0) return(0);
	oftotal = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
	if (oftotal == 0) return(0);
	for(i=0; i<len; i++)
	{
		if (us_makehighlight(((CHAR **)var->addr)[i], &manyhigh[i]))
		{
			us_abortcommand(_("Highlight unintelligible"));
			break;
		}
	}

	/* sort the objects */
	esort(manyhigh, len, sizeof (HIGHLIGHT), us_highlightsascending);

	/* determine the enumeration of repeats */
	for(i=0; i<len; i++)
	{
		whichone[i] = oftotal[i] = 0;
		for(j=i+1; j<len; j++)
		{
			if ((manyhigh[i].status&HIGHTYPE) != (manyhigh[j].status&HIGHTYPE))
				break;
			if ((manyhigh[i].status&HIGHTYPE) == HIGHFROM)
			{
				got1 = manyhigh[i].fromgeom;
				got2 = manyhigh[j].fromgeom;
				if (got1->entryisnode != got2->entryisnode) break;
				if (got1->entryisnode)
				{
					if (got1->entryaddr.ni->proto != got2->entryaddr.ni->proto) break;
				} else
				{
					if (got1->entryaddr.ai->proto != got2->entryaddr.ai->proto) break;
				}
			}
		}
		if (j == i+1) continue;
		for(k=i; k<j; k++)
		{
			whichone[k] = k-i+1;
			oftotal[k] = j-i;
		}
		i = j-1;
	}

	dia = DiaInitDialog(&us_multigetinfodialog);
	if (dia == 0)
	{
		efree((CHAR *)manyhigh);
		efree((CHAR *)whichone);
		efree((CHAR *)oftotal);
		return(0);
	}
	DiaInitTextDialog(dia, DGIM_OBJECTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCDOUBLEQUIT|SCREPORT);
	newlang[0] = _("Leave selection alone");
	newlang[1] = _("Make all Hard-to-select");
	newlang[2] = _("Make all Easy-to-select");
	DiaSetPopup(dia, DGIM_SELECTION, 3, newlang);

	newlang[0] = _("Leave selection alone");
	for(i=0; i<NUMPORTCHARS; i++) newlang[i+1] = TRANSLATE(us_exportcharnames[i]);
	DiaSetPopup(dia, DGIM_EXPORTCHARS, NUMPORTCHARS, newlang);

	/* multiple objects selected: list them */
	us_listmanyhighlights(len, manyhigh, whichone, oftotal, dia);

	/* if there are exactly two node/arc objects, show the distance between them */
	got1 = got2 = NOGEOM;
	for(i=0; i<len; i++)
	{
		if ((manyhigh[i].status&HIGHTYPE) == HIGHFROM)
		{
			geom = manyhigh[i].fromgeom;
			cx = (geom->lowx + geom->highx) / 2;
			cy = (geom->lowy + geom->highy) / 2;
			if (i == 0)
			{
				got1 = geom;
				cx1 = cx;
				cy1 = cy;
			} else if (i == 1)
			{
				got2 = geom;
				cx2 = cx;
				cy2 = cy;
			}
		}
	}
	if (len == 2 && got1 != NOGEOM && got2 != NOGEOM)
	{
		DiaStuffLine(dia, DGIM_OBJECTLIST, x_("----------------------------"));
		infstr = initinfstr();
		if (got1->entryisnode)
			us_getnodedisplayposition(got1->entryaddr.ni, &cx1, &cy1);
		if (got2->entryisnode)
			us_getnodedisplayposition(got2->entryaddr.ni, &cx2, &cy2);
		formatinfstr(infstr, _("Distance between objects: X=%s Y=%s"),
			latoa(abs(cx1-cx2), 0), latoa(abs(cy1-cy2), 0));
		DiaStuffLine(dia, DGIM_OBJECTLIST, returninfstr(infstr));
		if (got1->entryisnode && got1->entryaddr.ni->proto->primindex == 0 &&
			got2->entryisnode && got2->entryaddr.ni->proto->primindex == 0)
		{
			/* report edge distance between two cell instances */
			ni1 = got1->entryaddr.ni;
			lx1 = ni1->lowx;   hx1 = ni1->highx;
			ly1 = ni1->lowy;   hy1 = ni1->highy;
			makerot(ni1, trans);
			xform(lx1, ly1, &lx1, &ly1, trans);
			xform(hx1, hy1, &hx1, &hy1, trans);
			if (hx1 < lx1) { swap = lx1;   lx1 = hx1;   hx1 = swap; }
			if (hy1 < ly1) { swap = ly1;   ly1 = hy1;   hy1 = swap; }

			ni2 = got2->entryaddr.ni;
			lx2 = ni2->lowx;   hx2 = ni2->highx;
			ly2 = ni2->lowy;   hy2 = ni2->highy;
			makerot(ni2, trans);
			xform(lx2, ly2, &lx2, &ly2, trans);
			xform(hx2, hy2, &hx2, &hy2, trans);
			if (hx2 < lx2) { swap = lx2;   lx2 = hx2;   hx2 = swap; }
			if (hy2 < ly2) { swap = ly2;   ly2 = hy2;   hy2 = swap; }

			nodistx = nodisty = 0;
			if (lx1 > hx2) distx = lx1 - hx2; else
			{
				if (lx2 > hx1) distx = lx2 - hx1; else
				{
					nodistx = 1;
					distx = 0;
				}
			}
			if (ly1 > hy2) disty = ly1 - hy2; else
			{
				if (ly2 > hy1) disty = ly2 - hy1; else
				{
					nodisty = 1;
					disty = 0;
				}
			}
			if (nodistx != 0 && nodisty != 0)
				DiaStuffLine(dia, DGIM_OBJECTLIST, _("Cells overlap")); else
			{
				infstr = initinfstr();
				addstringtoinfstr(infstr, _("Distance between edges:"));
				if (nodistx == 0)
				{
					formatinfstr(infstr, _(" X=%s"), latoa(distx, 0));
				}
				if (nodisty == 0)
				{
					formatinfstr(infstr, _(" Y=%s"), latoa(disty, 0));
				}
				DiaStuffLine(dia, DGIM_OBJECTLIST, returninfstr(infstr));
			}
		}
	}

	/* loop dialog */
	highlightchanged = nodeinfochanged = arcinfochanged = positionchanged = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL || itemHit == DGIM_INFO) break;
		if (itemHit == DGIM_OBJECTLIST)
		{
			linelist = DiaGetCurLines(dia, DGIM_OBJECTLIST);
			us_clearhighlightcount();
			highlightchanged = TRUE;
			j = 0;
			for(i=0; linelist[i] >= 0; i++)
			{
				which = linelist[i];
				if (which >= len) continue;
				high = manyhigh[which];
				us_addhighlight(&high);
				j++;
			}
			if (j == 0)
			{
				DiaDimItem(dia, DGIM_INFO);
				DiaDimItem(dia, DGIM_REMOVE);
			} else
			{
				DiaUnDimItem(dia, DGIM_INFO);
				DiaUnDimItem(dia, DGIM_REMOVE);
			}
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			continue;
		}
		if (itemHit == DGIM_NODEXPOS || itemHit == DGIM_NODEYPOS ||
			itemHit == DGIM_NODEXSIZE || itemHit == DGIM_NODEYSIZE)
		{
			nodeinfochanged = TRUE;
			if (itemHit == DGIM_NODEXPOS || itemHit == DGIM_NODEYPOS) positionchanged = TRUE;
			continue;
		}
		if (itemHit == DGIM_ARCWIDTH)
		{
			arcinfochanged = TRUE;
			continue;
		}
		if (itemHit == DGIM_REMOVE)
		{
			linelist = DiaGetCurLines(dia, DGIM_OBJECTLIST);
			j = 0;
			for(i=0; linelist[i] >= 0; i++) j++;
			esort(linelist, j, SIZEOFINTBIG, sort_intbigdescending);
			which = len;
			for(i=0; i<j; i++)
			{
				which = linelist[i];
				if (which >= len) continue;
				for(k=which; k<len-1; k++)
				{
					manyhigh[k] = manyhigh[k+1];
					whichone[k] = whichone[k+1];
					oftotal[k] = oftotal[k+1];
				}
				len--;
			}
			while (which >= len && which > 0) which--;
			if (which < len)
			{
				high = manyhigh[which];
				us_clearhighlightcount();
				us_addhighlight(&high);
				us_showallhighlight();
				us_endchanges(NOWINDOWPART);
			}
			highlightchanged = TRUE;
			us_listmanyhighlights(len, manyhigh, whichone, oftotal, dia);
			continue;
		}
	}

	/* remember the selected item */
	which = DiaGetCurLine(dia, DGIM_OBJECTLIST);

	/* if cancelling, restore highlighting */
	if (itemHit == CANCEL && highlightchanged)
	{
		us_clearhighlightcount();
		for(i=0; i<len; i++)
			us_addhighlight(&manyhigh[i]);
	}

	/* accept multiple changes */
	if (itemHit != CANCEL && (arcinfochanged || nodeinfochanged ||
		DiaGetPopupEntry(dia, DGIM_SELECTION) != 0 || DiaGetPopupEntry(dia, DGIM_EXPORTCHARS) != 0))
	{
		us_pushhighlight();
		us_clearhighlightcount();
		if (nodeinfochanged)
		{
			/* handle multiple changes at once */
			nis = (NODEINST **)emalloc(len * (sizeof (NODEINST *)), us_tool->cluster);
			dlxs = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
			dlys = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
			dhxs = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
			dhys = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
			drots = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
			dtrans = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
			k = 0;
			for(i=0; i<len; i++)
			{
				high = manyhigh[i];
				if ((high.status&HIGHTYPE) != HIGHFROM) continue;
				if (!high.fromgeom->entryisnode) continue;
				ni = (NODEINST *)high.fromgeom->entryaddr.ni;
				us_getnodedisplaysize(ni, &xsize, &ysize);
				xyrev = FALSE;
				if (ni->transpose == 0)
				{
					if (ni->rotation == 900 || ni->rotation == 2700) xyrev = TRUE;
				} else
				{
					if (ni->rotation == 0 || ni->rotation == 1800) xyrev = TRUE;
				}
				us_getnodedisplayposition(ni, &xpos, &ypos);
				pt = DiaGetText(dia, DGIM_NODEXPOS);
				if (*pt != 0) xpos = atola(pt, 0);
				pt = DiaGetText(dia, DGIM_NODEYPOS);
				if (*pt != 0) ypos = atola(pt, 0);
				if (ni->proto->primindex != 0)
				{
					pt = DiaGetText(dia, DGIM_NODEXSIZE);
					if (*pt != 0) xsize = atola(pt, 0);
					pt = DiaGetText(dia, DGIM_NODEYSIZE);
					if (*pt != 0) ysize = atola(pt, 0); 
				}
				us_getnodemodfromdisplayinfo(ni, xsize, ysize, xpos, ypos,
					ni->rotation, ni->transpose, positionchanged, &dlxs[k], &dlys[k], &dhxs[k], &dhys[k],
						&drots[k], &dtrans[k], xyrev);
				nis[k] = ni;
				k++;
			}
			for(i=0; i<k; i++)
				startobjectchange((INTBIG)nis[i], VNODEINST);
			modifynodeinsts(k, nis, dlxs, dlys, dhxs, dhys, drots, dtrans);
			for(i=0; i<k; i++)
				endobjectchange((INTBIG)nis[i], VNODEINST);
			efree((CHAR *)nis);
			efree((CHAR *)dlxs);
			efree((CHAR *)dlys);
			efree((CHAR *)dhxs);
			efree((CHAR *)dhys);
			efree((CHAR *)drots);
			efree((CHAR *)dtrans);
		}
		if (arcinfochanged)
		{
			for(i=0; i<len; i++)
			{
				high = manyhigh[i];
				if ((high.status&HIGHTYPE) != HIGHFROM) continue;
				if (high.fromgeom->entryisnode) continue;
				ai = (ARCINST *)high.fromgeom->entryaddr.ai;
				lambda = lambdaofarc(ai);
				value = atola(DiaGetText(dia, DGIM_ARCWIDTH), lambda);
				if (value >= 0)
				{
					value = arcwidthoffset(ai) + value;
					if (value != ai->width)
					{
						startobjectchange((INTBIG)ai, VARCINST);
						(void)modifyarcinst(ai, value - ai->width, 0, 0, 0, 0);
						endobjectchange((INTBIG)ai, VARCINST);
					}
				}
			}
		}

		/* if export characteristics changed */
		j = DiaGetPopupEntry(dia, DGIM_EXPORTCHARS);
		if (j != 0)
		{
			for(i=0; i<len; i++)
			{
				high = manyhigh[i];
				if ((high.status&HIGHTYPE) != HIGHTEXT) continue;
				if (high.fromgeom == NOGEOM) continue;
				if (high.fromport == NOPORTPROTO) continue;
				newbit = high.fromport->userbits;
				newbit = (newbit & ~STATEBITS) | us_exportcharlist[j-1];
				if (newbit != (INTBIG)high.fromport->userbits)
				{
					setval((INTBIG)high.fromport, VPORTPROTO, x_("userbits"), newbit,
						VINTEGER);
					changeallports(high.fromport);
				}
			}
		}

		/* if selection changed, fix it */
		j = DiaGetPopupEntry(dia, DGIM_SELECTION);
		if (j != 0)
		{
			for(i=0; i<len; i++)
			{
				high = manyhigh[i];
				if ((high.status&HIGHTYPE) != HIGHFROM) continue;
				if (high.fromgeom->entryisnode)
				{
					ni = (NODEINST *)high.fromgeom->entryaddr.ni;
					if (j == 1) ni->userbits |= HARDSELECTN; else
						ni->userbits &= ~HARDSELECTN;
				} else
				{
					ai = (ARCINST *)high.fromgeom->entryaddr.ai;
					if (j == 1) ai->userbits |= HARDSELECTA; else
						ai->userbits &= ~HARDSELECTA;
				}
			}
		}
		us_pophighlight(TRUE);
	}
	if (itemHit == DGIM_INFO) high = manyhigh[which];

	/* cleanup */
	DiaDoneDialog(dia);
	efree((CHAR *)manyhigh);
	efree((CHAR *)whichone);
	efree((CHAR *)oftotal);

	/* if info requested for one of the objects, give it now */
	if (itemHit == DGIM_INFO)
	{
		if ((high.status&HIGHTYPE) == HIGHTEXT)
		{
			if (high.fromvar == NOVARIABLE && high.fromport != NOPORTPROTO)
				us_getinfoexport(&high); else
					us_getinfotext(&high, canspecialize);
			return(0);
		}
		if ((high.status&HIGHTYPE) == HIGHFROM)
		{
			if (!high.fromgeom->entryisnode)
			{
				/* arc getinfo */
				ai = (ARCINST *)high.fromgeom->entryaddr.ai;
				us_getinfoarc(ai);
				return(0);
			}
			ni = (NODEINST *)high.fromgeom->entryaddr.ni;
			us_getinfonode(ni, canspecialize);
			return(0);
		}
	}
	return(0);
}
	
/*
 * Helper routine for "esort" that makes highlights go in ascending order
 * (used by "us_showdlog()").
 */
int us_highlightsascending(const void *e1, const void *e2)
{
	REGISTER HIGHLIGHT *c1, *c2;
	REGISTER GEOM *got1, *got2;
	REGISTER INTBIG s1, s2;
	REGISTER INTBIG in1, in2;

	c1 = (HIGHLIGHT *)e1;
	c2 = (HIGHLIGHT *)e2;
	s1 = c1->status & HIGHTYPE;
	s2 = c2->status & HIGHTYPE;
	if (s1 != s2) return(s1-s2);
	if (s2 != HIGHFROM) return(0);
	got1 = c1->fromgeom;
	got2 = c2->fromgeom;
	in1 = got1->entryisnode;
	in2 = got2->entryisnode;
	if (in1 != in2) return(in1-in2);
	if (got1->entryisnode)
	{
		return(namesame(describenodeproto(got1->entryaddr.ni->proto),
			describenodeproto(got2->entryaddr.ni->proto)));
	}
	return(namesame(got1->entryaddr.ai->proto->protoname,
		got2->entryaddr.ai->proto->protoname));
}

void us_listmanyhighlights(INTBIG len, HIGHLIGHT *manyhigh, INTBIG *whichone, INTBIG *oftotal, void *dia)
{
	REGISTER INTBIG i, dx, dy, xsize, ysize, xpos, ypos, width, value, minxsize, maxxsize,
		minysize, maxysize, minxpos, maxxpos, minypos, maxypos, minwidth, maxwidth;
	INTBIG cx, cy, xvalue, yvalue;
	XARRAY trans;
	CHAR buf[100];
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER GEOM *geom;
	REGISTER VARIABLE *var;
	REGISTER INTBIG gotxpos, gotypos, gotxsize, gotysize, gotwidth;
	REGISTER void *infstr;

	esnprintf(buf, 100, x_("%ld %s:"), len, makeplural(_("selection"), len));
	DiaSetText(dia, DGIM_OBJECTCOUNT, buf);
	gotxpos = gotypos = gotxsize = gotysize = gotwidth = xsize = ysize = xpos = ypos = width = 0;
	DiaLoadTextDialog(dia, DGIM_OBJECTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	minxsize = minysize = minxpos = minypos = minwidth = 0;
	maxxsize = maxysize = maxxpos = maxypos = maxwidth = -1;
	for(i=0; i<len; i++)
	{
		infstr = initinfstr();
		switch (manyhigh[i].status&HIGHTYPE)
		{
			case HIGHFROM:
				geom = manyhigh[i].fromgeom;
				cx = (geom->lowx + geom->highx) / 2;
				cy = (geom->lowy + geom->highy) / 2;
				if (geom->entryisnode)
				{
					ni = (NODEINST *)geom->entryaddr.ni;
					addstringtoinfstr(infstr, _("Node "));
					addstringtoinfstr(infstr, describenodeinst(ni));
					var = getvalkey((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY,
						el_prototype_center_key);
					if (var != NOVARIABLE)
					{
						dx = ((INTBIG *)var->addr)[0] + (ni->lowx+ni->highx)/2 -
							(ni->proto->lowx+ni->proto->highx)/2;
						dy = ((INTBIG *)var->addr)[1] + (ni->lowy+ni->highy)/2 -
							(ni->proto->lowy+ni->proto->highy)/2;
						makerot(ni, trans);
						xform(dx, dy, &cx, &cy, trans);
					}

					/* accumulate X and Y size */
					if (ni->proto->primindex != 0)
					{
						us_getnodedisplaysize(ni, &xvalue, &yvalue);
						if (minxsize > maxxsize) minxsize = maxxsize = xvalue; else
						{
							if (xvalue < minxsize) minxsize = xvalue;
							if (xvalue > maxxsize) maxxsize = xvalue;
						}
						if (minysize > maxysize) minysize = maxysize = yvalue; else
						{
							if (yvalue < minysize) minysize = yvalue;
							if (yvalue > maxysize) maxysize = yvalue;
						}
						if (gotxsize == 0)
						{
							xsize = xvalue;
							gotxsize = 1;
						} else if (gotxsize == 1)
						{
							if (xsize != xvalue) gotxsize = -1;
						}
						if (gotysize == 0)
						{
							ysize = yvalue;
							gotysize = 1;
						} else if (gotysize == 1)
						{
							if (ysize != yvalue) gotysize = -1;
						}
					}

					/* accumulate X and Y position */
					us_getnodedisplayposition(ni, &xvalue, &yvalue);
					if (minxpos > maxxpos) minxpos = maxxpos = xvalue; else
					{
						if (xvalue < minxpos) minxpos = xvalue;
						if (xvalue > maxxpos) maxxpos = xvalue;
					}
					if (minypos > maxypos) minypos = maxypos = yvalue; else
					{
						if (yvalue < minypos) minypos = yvalue;
						if (yvalue > maxypos) maxypos = yvalue;
					}
					if (gotxpos == 0)
					{
						xpos = xvalue;
						gotxpos = 1;
					} else if (gotxpos == 1)
					{
						if (xpos != xvalue) gotxpos = -1;
					}
					if (gotypos == 0)
					{
						ypos = yvalue;
						gotypos = 1;
					} else if (gotypos == 1)
					{
						if (ypos != yvalue) gotypos = -1;
					}
				} else
				{
					ai = (ARCINST *)geom->entryaddr.ai;
					addstringtoinfstr(infstr, _("Arc "));
					addstringtoinfstr(infstr, describearcinst(ai));
					value = ai->width - arcwidthoffset(ai);
					if (minwidth > maxwidth) minwidth = maxwidth = value; else
					{
						if (value < minwidth) minwidth = value;
						if (value > maxwidth) maxwidth = value;
					}
					if (gotwidth == 0)
					{
						width = value;
						gotwidth = 1;
					} else if (gotwidth == 1)
					{
						if (width != value) gotwidth = -1;
					}
				}
				break;
			case HIGHBBOX:
				formatinfstr(infstr, _("Bounds: X from %s to %s, Y from %s to %s"),
					latoa(manyhigh[i].stalx, 0), latoa(manyhigh[i].stahx, 0),
					latoa(manyhigh[i].staly, 0), latoa(manyhigh[i].stahy, 0));
				break;
			case HIGHLINE:
				formatinfstr(infstr, _("Line (%s, %s) to (%s, %s)"),
					latoa(manyhigh[i].stalx, 0), latoa(manyhigh[i].staly, 0),
					latoa(manyhigh[i].stahx, 0), latoa(manyhigh[i].stahy, 0));
				break;
			case HIGHTEXT:
				if (manyhigh[i].fromport != NOPORTPROTO)
				{
					formatinfstr(infstr, _("Export "));
					addstringtoinfstr(infstr, manyhigh[i].fromport->protoname);
					break;
				}
				if (manyhigh[i].fromvar != NOVARIABLE)
				{
					if (manyhigh[i].fromport != NOPORTPROTO)
					{
						formatinfstr(infstr, _("Text on export %s"),
							manyhigh[i].fromport->protoname);
						break;
					}
					if (manyhigh[i].fromgeom == NOGEOM)
					{
						formatinfstr(infstr, _("Text on cell %s"),
							describenodeproto(manyhigh[i].cell));
						break;
					}
					if (!manyhigh[i].fromgeom->entryisnode)
					{
						ai = (ARCINST *)manyhigh[i].fromgeom->entryaddr.ai;
						formatinfstr(infstr, _("Text on arc %s"), describearcinst(ai));
						break;
					}
					ni = (NODEINST *)manyhigh[i].fromgeom->entryaddr.ni;
					formatinfstr(infstr, _("Text on node %s"), describenodeinst(ni));
					break;
				}
				addstringtoinfstr(infstr, _("Text object"));
				break;
		}
		if (whichone[i] != 0)
			formatinfstr(infstr, x_(" (%ld of %ld)"), whichone[i], oftotal[i]);
		DiaStuffLine(dia, DGIM_OBJECTLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DGIM_OBJECTLIST, 0);

	if (gotxpos == 1)
	{
		DiaSetText(dia, DGIM_NODEXPOS, latoa(xpos, 0));
		DiaSetText(dia, DGIM_NODEXPRANGE, _("All the same"));
	} else if (minxpos <= maxxpos)
	{
		esnprintf(buf, 100, x_("%s to %s"), latoa(minxpos, 0), latoa(maxxpos, 0));
		DiaSetText(dia, DGIM_NODEXPRANGE, buf);
	}
	if (gotypos == 1)
	{
		DiaSetText(dia, DGIM_NODEYPOS, latoa(ypos, 0));
		DiaSetText(dia, DGIM_NODEYPRANGE, _("All the same"));
	} else if (minypos <= maxypos)
	{
		esnprintf(buf, 100, x_("%s to %s"), latoa(minypos, 0), latoa(maxypos, 0));
		DiaSetText(dia, DGIM_NODEYPRANGE, buf);
	}
	if (gotxsize == 1)
	{
		DiaSetText(dia, DGIM_NODEXSIZE, latoa(xsize, 0));
		DiaSetText(dia, DGIM_NODEXSRANGE, _("All the same"));
	} else if (minxsize <= maxxsize)
	{
		esnprintf(buf, 100, x_("%s to %s"), latoa(minxsize, 0), latoa(maxxsize, 0));
		DiaSetText(dia, DGIM_NODEXSRANGE, buf);
	}
	if (gotysize == 1)
	{
		DiaSetText(dia, DGIM_NODEYSIZE, latoa(ysize, 0));
		DiaSetText(dia, DGIM_NODEYSRANGE, _("All the same"));
	} else if (minysize <= maxysize)
	{
		esnprintf(buf, 100, x_("%s to %s"), latoa(minysize, 0), latoa(maxysize, 0));
		DiaSetText(dia, DGIM_NODEYSRANGE, buf);
	}
	if (gotwidth == 1)
	{
		DiaSetText(dia, DGIM_ARCWIDTH, latoa(width, 0));
		DiaSetText(dia, DGIM_ARCWIDRANGE, _("All the same"));
	} else if (minwidth <= maxwidth)
	{
		esnprintf(buf, 100, x_("%s to %s"), latoa(minwidth, 0), latoa(maxwidth, 0));
		DiaSetText(dia, DGIM_ARCWIDRANGE, buf);
	}
}
/* Text info */
static DIALOGITEM us_showtextdialogitems[] =
{
 /*  1 */ {0, {480,236,504,308}, BUTTON, N_("OK")},
 /*  2 */ {0, {480,20,504,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {200,8,216,64}, RADIO, N_("Left")},
 /*  4 */ {0, {40,8,88,340}, EDITTEXT, x_("")},
 /*  5 */ {0, {332,64,348,112}, EDITTEXT, x_("")},
 /*  6 */ {0, {136,8,152,80}, RADIO, N_("Center")},
 /*  7 */ {0, {152,8,168,80}, RADIO, N_("Bottom")},
 /*  8 */ {0, {168,8,184,64}, RADIO, N_("Top")},
 /*  9 */ {0, {184,8,200,72}, RADIO, N_("Right")},
 /* 10 */ {0, {216,8,232,104}, RADIO, N_("Lower right")},
 /* 11 */ {0, {232,8,248,104}, RADIO, N_("Lower left")},
 /* 12 */ {0, {248,8,264,104}, RADIO, N_("Upper right")},
 /* 13 */ {0, {264,8,280,96}, RADIO, N_("Upper left")},
 /* 14 */ {0, {280,8,296,80}, RADIO, N_("Boxed")},
 /* 15 */ {0, {332,120,348,280}, RADIO, N_("Points (max 63)")},
 /* 16 */ {0, {120,16,136,110}, MESSAGE, N_("Text corner:")},
 /* 17 */ {0, {164,244,180,324}, EDITTEXT, x_("")},
 /* 18 */ {0, {188,244,204,324}, EDITTEXT, x_("")},
 /* 19 */ {0, {164,152,180,241}, MESSAGE, N_("X offset:")},
 /* 20 */ {0, {188,152,204,241}, MESSAGE, N_("Y offset:")},
 /* 21 */ {0, {136,112,168,144}, ICON, (CHAR *)us_icon200},
 /* 22 */ {0, {168,112,200,144}, ICON, (CHAR *)us_icon201},
 /* 23 */ {0, {200,112,232,144}, ICON, (CHAR *)us_icon202},
 /* 24 */ {0, {232,112,264,144}, ICON, (CHAR *)us_icon203},
 /* 25 */ {0, {264,112,296,144}, ICON, (CHAR *)us_icon204},
 /* 26 */ {0, {4,8,36,340}, MESSAGE, x_("")},
 /* 27 */ {0, {128,194,148,294}, BUTTON, N_("Edit Text")},
 /* 28 */ {0, {236,152,252,340}, CHECK, N_("Visible only inside cell")},
 /* 29 */ {0, {96,8,112,340}, MESSAGE, x_("")},
 /* 30 */ {0, {212,232,228,340}, POPUP, x_("")},
 /* 31 */ {0, {212,152,228,231}, MESSAGE, N_("Language:")},
 /* 32 */ {0, {308,8,324,88}, MESSAGE, N_("Show:")},
 /* 33 */ {0, {308,92,324,280}, POPUP, x_("")},
 /* 34 */ {0, {356,64,372,112}, EDITTEXT, x_("")},
 /* 35 */ {0, {356,120,372,280}, RADIO, N_("Lambda (max 127.75)")},
 /* 36 */ {0, {380,92,396,280}, POPUP, x_("")},
 /* 37 */ {0, {380,8,396,87}, MESSAGE, N_("Type face:")},
 /* 38 */ {0, {404,8,420,91}, CHECK, N_("Italic")},
 /* 39 */ {0, {404,104,420,187}, CHECK, N_("Bold")},
 /* 40 */ {0, {404,196,420,279}, CHECK, N_("Underline")},
 /* 41 */ {0, {344,8,360,56}, MESSAGE, N_("Size")},
 /* 42 */ {0, {268,260,292,332}, BUTTON, N_("Node Info")},
 /* 43 */ {0, {268,160,292,232}, BUTTON, N_("See Node")},
 /* 44 */ {0, {428,92,444,280}, POPUP, x_("")},
 /* 45 */ {0, {428,8,444,87}, MESSAGE, N_("Rotation:")},
 /* 46 */ {0, {452,92,468,280}, POPUP, x_("")},
 /* 47 */ {0, {452,8,468,87}, MESSAGE, N_("Units:")}
};
static DIALOG us_showtextdialog = {{50,75,563,424}, N_("Information on Highlighted Text"), 0, 47, us_showtextdialogitems, 0, 0};

/* special items for the "Text Get Info" dialog: */
#define DGIT_CORNERLEFT       3		/* left (radio) */
#define DGIT_TEXTVALUE        4		/* text name (edit text) */
#define DGIT_TEXTSIZEABS      5		/* Absolute text size (edit text) */
#define DGIT_CORNERCENTER     6		/* center (radio) */
#define DGIT_CORNERBOT        7		/* bottom (radio) */
#define DGIT_CORNERTOP        8		/* top (radio) */
#define DGIT_CORNERRIGHT      9		/* right (radio) */
#define DGIT_CORNERLOWRIGHT  10		/* lower right (radio) */
#define DGIT_CORNERLOWLEFT   11		/* lower left (radio) */
#define DGIT_CORNERUPRIGHT   12		/* upper right (radio) */
#define DGIT_CORNERUPLEFT    13		/* upper left (radio) */
#define DGIT_CORNERBOXED     14		/* boxed (radio) */
#define DGIT_TEXTSIZEABS_L   15		/* Absolute text size (radio) */
#define DGIT_XOFFSET         17		/* X offset (edit text) */
#define DGIT_YOFFSET         18		/* Y offset (edit text) */
#define DGIT_XOFFSET_L       19		/* X offset label (stat text) */
#define DGIT_YOFFSET_L       20		/* Y offset label (stat text) */
#define DGIT_LOWICON         21		/* first icon (icon) */
#define DGIT_HIGHICON        25		/* last icon (icon) */
#define DGIT_TEXTTYPE        26		/* title (stat text) */
#define DGIT_EDITTEXT        27		/* Edit text (button) */
#define DGIT_INSIDECELL      28		/* Only inside cell (check) */
#define DGIT_EVALUATION      29		/* Evaluated value (stat text) */
#define DGIT_LANGUAGE        30		/* Language (popup) */
#define DGIT_LANGUAGE_L      31		/* Language label (stat text) */
#define DGIT_WHATTOSHOW_L    32		/* What to show label (stat text) */
#define DGIT_WHATTOSHOW      33		/* What to show (popup) */
#define DGIT_TEXTSIZEREL     34		/* Relative text size (edit text) */
#define DGIT_TEXTSIZEREL_L   35		/* Relative text size label (radio) */
#define DGIT_TEXTFACE        36		/* Typeface (popup) */
#define DGIT_TEXTFACE_L      37		/* Typeface label (stat text) */
#define DGIT_TEXTITALIC      38		/* Italic type (check) */
#define DGIT_TEXTBOLD        39		/* Bold type (check) */
#define DGIT_TEXTUNDERLINE   40		/* Underline type (check) */
#define DGIT_OBJECTINFO      42		/* Get Info on object (button) */
#define DGIT_OBJECTHIGH      43		/* Highlight object (button) */
#define DGIT_ROTATION        44		/* Text rotation (popup) */
#define DGIT_UNITS           46		/* Text units (popup) */
#define DGIT_UNITS_L         47		/* Text units label (stat text) */

void us_getinfotext(HIGHLIGHT *high, BOOLEAN canspecialize)
{
	REGISTER INTBIG itemHit, j, i, height, objtype, lambda, savetype,
		xcur, ycur, lindex, xc, yc, maxshowoptions;
	INTBIG x, y, nlx, nhx, nly, nhy, oldtype, newval;
	REGISTER BOOLEAN changed, posnotoffset, cantbox;
	UINTBIG descript[TEXTDESCRIPTSIZE], newdescript[TEXTDESCRIPTSIZE], newtype;
	CHAR *str, *newstr, *formerstr, *newlang[15], **languages, buf[30];
	RECTAREA itemRect;
	HIGHLIGHT newhigh;
	REGISTER NODEINST *ni;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *cell;
	REGISTER VARIABLE *var, *noevar;
	REGISTER TECHNOLOGY *tech;
	static CHAR *whattodisplay[] = {N_("Value"), N_("Name&Value"),
		N_("Name,Inherit,Value"), N_("Name,Inherit-All,Value")};
	static struct butlist poslist[10] =
	{
		{VTPOSCENT,      DGIT_CORNERCENTER},
		{VTPOSUP,        DGIT_CORNERBOT},
		{VTPOSDOWN,      DGIT_CORNERTOP},
		{VTPOSLEFT,      DGIT_CORNERRIGHT},
		{VTPOSRIGHT,     DGIT_CORNERLEFT},
		{VTPOSUPLEFT,    DGIT_CORNERLOWRIGHT},
		{VTPOSUPRIGHT,   DGIT_CORNERLOWLEFT},
		{VTPOSDOWNLEFT,  DGIT_CORNERUPRIGHT},
		{VTPOSDOWNRIGHT, DGIT_CORNERUPLEFT},
		{VTPOSBOXED,     DGIT_CORNERBOXED}
	};
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display text */
	var = high->fromvar;
	tech = us_hightech(high);
	cell = us_highcell(high);
	if (cell == NONODEPROTO) lib = el_curlib; else
		lib = cell->lib;
	lambda = lib->lambda[tech->techindex];

	objtype = VUNKNOWN;
	if (var == NOVARIABLE)
	{
		/* this must be a cell name */
		if (high->fromgeom == NOGEOM || !high->fromgeom->entryisnode)
			return;

		/* get information about the cell name */
		ni = high->fromgeom->entryaddr.ni;
		str = describenodeproto(ni->proto);
		TDCOPY(descript, ni->textdescript);
		if (ni->rotation != 0 || ni->transpose != 0)
			us_rotatedescript(high->fromgeom, descript);
		objtype = VNODEINST;

		/* display the text dialog box */
		us_showtextdialog.list[DGIT_OBJECTINFO-1].msg = _("Node Info");
		us_showtextdialog.list[DGIT_OBJECTHIGH-1].msg = _("See Node");
		dia = DiaInitDialog(&us_showtextdialog);
		if (dia == 0) return;
		DiaSetText(dia, DGIT_TEXTVALUE, str);
		DiaNoEditControl(dia, DGIT_TEXTVALUE);
		DiaSetText(dia, DGIT_TEXTTYPE, _("Text information for cell name:"));
		DiaDimItem(dia, DGIT_EDITTEXT);
		DiaDimItem(dia, DGIT_INSIDECELL);
		languages = us_languagechoices();
		DiaSetPopup(dia, DGIT_LANGUAGE, 4, languages);
		DiaDimItem(dia, DGIT_LANGUAGE);
		DiaDimItem(dia, DGIT_LANGUAGE_L);
		DiaDimItem(dia, DGIT_WHATTOSHOW_L);
		DiaDimItem(dia, DGIT_WHATTOSHOW);
		DiaDimItem(dia, DGIT_UNITS);
		DiaDimItem(dia, DGIT_UNITS_L);
		for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_rotationtypes[i]);
		DiaSetPopup(dia, DGIT_ROTATION, 4, newlang);

		/* set the orientation button */
		for(i=0; i<10; i++)
			if (poslist[i].value == TDGETPOS(descript))
		{
			DiaSetControl(dia, poslist[i].button, 1);
			break;
		}

		/* set the size fields */
		i = TDGETSIZE(descript);
		if (TXTGETPOINTS(i) != 0)
		{
			/* show point size */
			esnprintf(buf, 30, x_("%ld"), TXTGETPOINTS(i));
			DiaSetText(dia, DGIT_TEXTSIZEABS, buf);
			DiaSetControl(dia, DGIT_TEXTSIZEABS_L, 1);

			/* figure out how many lambda the point value is */
			height = TXTGETPOINTS(i);
			if (el_curwindowpart != NOWINDOWPART)
				height = roundfloat((float)height / el_curwindowpart->scaley);
			height = height * 4 / lambda;
			DiaSetText(dia, DGIT_TEXTSIZEREL, frtoa(height * WHOLE / 4));
		} else if (TXTGETQLAMBDA(i) != 0)
		{
			/* show lambda value */
			height = TXTGETQLAMBDA(i);
			DiaSetText(dia, DGIT_TEXTSIZEREL, frtoa(height * WHOLE / 4));
			DiaSetControl(dia, DGIT_TEXTSIZEREL_L, 1);

			/* figure out how many points the lambda value is */
			height = height * lambda / 4;
			if (el_curwindowpart != NOWINDOWPART)
				height = applyyscale(el_curwindowpart, height);
			esnprintf(buf, 30, x_("%ld"), height);
			DiaSetText(dia, DGIT_TEXTSIZEABS, buf);
		}
		if (graphicshas(CANCHOOSEFACES))
		{
			DiaUnDimItem(dia, DGIT_TEXTFACE_L);
			us_setpopupface(DGIT_TEXTFACE, TDGETFACE(descript), TRUE, dia);
		} else
		{
			DiaDimItem(dia, DGIT_TEXTFACE_L);
		}
		i = TDGETROTATION(descript);
		DiaSetPopupEntry(dia, DGIT_ROTATION, i);
		if (graphicshas(CANMODIFYFONTS))
		{
			DiaUnDimItem(dia, DGIT_TEXTITALIC);
			DiaUnDimItem(dia, DGIT_TEXTBOLD);
			DiaUnDimItem(dia, DGIT_TEXTUNDERLINE);
			if (TDGETITALIC(descript) != 0) DiaSetControl(dia, DGIT_TEXTITALIC, 1);
			if (TDGETBOLD(descript) != 0) DiaSetControl(dia, DGIT_TEXTBOLD, 1);
			if (TDGETUNDERLINE(descript) != 0) DiaSetControl(dia, DGIT_TEXTUNDERLINE, 1);
		} else
		{
			DiaDimItem(dia, DGIT_TEXTITALIC);
			DiaDimItem(dia, DGIT_TEXTBOLD);
			DiaDimItem(dia, DGIT_TEXTUNDERLINE);
		}

		/* set offsets */
		i = TDGETXOFF(descript);
		DiaSetText(dia, DGIT_XOFFSET, latoa(i * lambda / 4, lambda));
		j = TDGETYOFF(descript);
		DiaSetText(dia, DGIT_YOFFSET, latoa(j * lambda / 4, lambda));

		/* loop until done */
		changed = FALSE;
		for(;;)
		{
			itemHit = DiaNextHit(dia);
			if (itemHit == OK || itemHit == CANCEL || itemHit == DGIT_OBJECTINFO) break;
			if (itemHit == DGIT_OBJECTHIGH)
			{
				if (high->fromgeom != NOGEOM)
				{
					us_clearhighlightcount();
					newhigh.status = HIGHFROM;
					newhigh.cell = ni->parent;
					newhigh.fromgeom = ni->geom;
					newhigh.fromport = NOPORTPROTO;
					newhigh.frompoint = 0;
					newhigh.fromvar = NOVARIABLE;
					newhigh.fromvarnoeval = NOVARIABLE;
					us_addhighlight(&newhigh);
					us_showallhighlight();
					us_endchanges(NOWINDOWPART);
				}
				continue;
			}
			if (itemHit >= DGIT_LOWICON && itemHit <= DGIT_HIGHICON)
			{
				DiaItemRect(dia, itemHit, &itemRect);
				DiaGetMouse(dia, &x, &y);
				itemHit = (itemHit-DGIT_LOWICON) * 2;
				if (y > (itemRect.top + itemRect.bottom) / 2) itemHit++;
				itemHit = poslist[itemHit].button;
			}
			for(i=0; i<10; i++) if (itemHit == poslist[i].button)
			{
				for(j=0; j<10; j++) DiaSetControl(dia, poslist[j].button, 0);
				DiaSetControl(dia, itemHit, 1);
				changed = TRUE;
				continue;
			}
			if (itemHit == DGIT_XOFFSET || itemHit == DGIT_YOFFSET ||
				itemHit == DGIT_TEXTFACE || itemHit == DGIT_ROTATION) changed = TRUE;
			if (itemHit == DGIT_TEXTSIZEREL) itemHit = DGIT_TEXTSIZEREL_L;
			if (itemHit == DGIT_TEXTSIZEABS) itemHit = DGIT_TEXTSIZEABS_L;
			if (itemHit == DGIT_TEXTSIZEREL_L || itemHit == DGIT_TEXTSIZEABS_L)
			{
				DiaSetControl(dia, DGIT_TEXTSIZEREL_L, 0);
				DiaSetControl(dia, DGIT_TEXTSIZEABS_L, 0);
				DiaSetControl(dia, itemHit, 1);
				changed = TRUE;
				continue;
			}
			if (itemHit == DGIT_TEXTITALIC || itemHit == DGIT_TEXTBOLD ||
				itemHit == DGIT_TEXTUNDERLINE)
			{
				DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
				changed = TRUE;
				continue;
			}
		}
		if (itemHit != CANCEL)
		{
			/* get the new descriptor */
			TDCOPY(newdescript, descript);
			if (changed)
			{
				xcur = atola(DiaGetText(dia, DGIT_XOFFSET), 0);
				ycur = atola(DiaGetText(dia, DGIT_YOFFSET), 0);
				us_setdescriptoffset(newdescript, xcur*4/lambda, ycur*4/lambda);
				if (DiaGetControl(dia, DGIT_TEXTSIZEABS_L) != 0)
				{
					j = eatoi(DiaGetText(dia, DGIT_TEXTSIZEABS));
					if (j <= 0) j = 4;
					if (j >= TXTMAXPOINTS) j = TXTMAXPOINTS;
					TDSETSIZE(newdescript, TXTSETPOINTS(j));
				} else
				{
					j = atofr(DiaGetText(dia, DGIT_TEXTSIZEREL)) * 4 / WHOLE;
					if (j <= 0) j = 4;
					if (j >= TXTMAXQLAMBDA) j = TXTMAXQLAMBDA;
					TDSETSIZE(newdescript, TXTSETQLAMBDA(j));
				}
				if (DiaGetControl(dia, DGIT_TEXTITALIC) != 0)
					TDSETITALIC(newdescript, VTITALIC); else
						TDSETITALIC(newdescript, 0);
				if (DiaGetControl(dia, DGIT_TEXTBOLD) != 0)
					TDSETBOLD(newdescript, VTBOLD); else
						TDSETBOLD(newdescript, 0);
				if (DiaGetControl(dia, DGIT_TEXTUNDERLINE) != 0)
					TDSETUNDERLINE(newdescript, VTUNDERLINE); else
						TDSETUNDERLINE(newdescript, 0);
				if (graphicshas(CANCHOOSEFACES))
				{
					i = us_getpopupface(DGIT_TEXTFACE, dia);
					TDSETFACE(newdescript, i);
				}
				i = DiaGetPopupEntry(dia, DGIT_ROTATION);
				TDSETROTATION(newdescript, i);
				for(j=0; j<10; j++)
					if (DiaGetControl(dia, poslist[j].button) != 0) break;
				TDSETPOS(newdescript, poslist[j].value);
				if (DiaGetControl(dia, DGIT_INSIDECELL) != 0)
					TDSETINTERIOR(newdescript, VTINTERIOR); else
						TDSETINTERIOR(newdescript, 0);

				/* if the node is rotated, modify grab-point */
				if (ni->rotation != 0 || ni->transpose != 0)
					us_rotatedescriptI(high->fromgeom, newdescript);
			}

			/* see if changes were made */
			if (newdescript != descript)
			{
				/* save highlighting */
				us_pushhighlight();
				us_clearhighlightcount();

				/* set the new descriptor */
				startobjectchange((INTBIG)high->fromgeom->entryaddr.blind, objtype);
				us_modifytextdescript(high, newdescript);
				endobjectchange((INTBIG)high->fromgeom->entryaddr.blind, objtype);

				/* restore highlighting */
				us_pophighlight(TRUE);
			}
		}
		DiaDoneDialog(dia);
		if (itemHit == DGIT_OBJECTINFO)
		{
			us_clearhighlightcount();
			newhigh.status = HIGHFROM;
			newhigh.cell = ni->parent;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			newhigh.fromvar = NOVARIABLE;
			newhigh.fromvarnoeval = NOVARIABLE;
			us_addhighlight(&newhigh);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			us_getinfonode(ni, FALSE);
		}
		return;
	}

	/* handle standard variables */
	noevar = var;
	if (high->fromvarnoeval != NOVARIABLE) noevar = high->fromvarnoeval;

	/* special case if known variables are selected */
	if (canspecialize)
	{
		if (high->fromgeom != NOGEOM)
		{
			if (TDGETUNITS(var->textdescript) == VTUNITSRES)
			{
				us_resistancedlog(high->fromgeom, var);
				return;
			}
			if (TDGETUNITS(var->textdescript) == VTUNITSCAP)
			{
				us_capacitancedlog(high->fromgeom, var);
				return;
			}
			if (TDGETUNITS(var->textdescript) == VTUNITSIND)
			{
				us_inductancedlog(high->fromgeom, var);
				return;
			}
			if (high->fromgeom->entryisnode)
			{
				ni = high->fromgeom->entryaddr.ni;
				if (var->key == sch_globalnamekey)
				{
					(void)us_globalsignaldlog();
					return;
				}
				if (var->key == sch_diodekey)
				{
					us_areadlog(ni);
					return;
				}
				if (var->key == el_attrkey_length || var->key == el_attrkey_width)
				{
					/* must be a transistor */
					if (isfet(high->fromgeom))
					{
						us_widlendlog(ni);
						return;
					}
				}
				if (var->key == el_attrkey_area)
				{
					us_areadlog(ni);
					return;
				}
			}
		}

		/*
		 * if this is a displayable text variable, edit it in place.
		 * Can only in-place edit displayable text; with value shown; non-code; and arrays can
		 * only be edited in-place if they are string arrays
		 */
		if ((var->type&VDISPLAY) != 0 && (noevar->type&(VCODE1|VCODE2)) == 0 &&
			TDGETROTATION(var->textdescript) == 0 &&
			((var->type&VTYPE) == VSTRING || (var->type&VISARRAY) == 0))
		{
			/* save and clear highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* edit the variable */
			if (high->fromport != NOPORTPROTO)
			{
				us_editvariabletext(var, VPORTPROTO, (INTBIG)high->fromport,
					makename(var->key));
			} else if (high->fromgeom == NOGEOM)
			{
				us_editvariabletext(var, VNODEPROTO, (INTBIG)high->cell,
					makename(var->key));
			} else
			{
				if (high->fromgeom->entryisnode) objtype = VNODEINST; else
					objtype = VARCINST;
				us_editvariabletext(var, objtype, (INTBIG)high->fromgeom->entryaddr.blind,
					makename(var->key));
			}

			/* restore highlighting */
			us_pophighlight(FALSE);
			return;
		}
	}

	/* prepare dialog to show appropriate buttons */
	us_showtextdialog.list[DGIT_OBJECTINFO-1].msg = _("Info");
	us_showtextdialog.list[DGIT_OBJECTHIGH-1].msg = _("See");
	if (high->fromgeom != NOGEOM)
	{
		if (high->fromgeom->entryisnode)
		{
			objtype = VNODEINST;
			if (high->fromgeom->entryaddr.ni->proto != gen_invispinprim)
			{
				us_showtextdialog.list[DGIT_OBJECTINFO-1].msg = _("Node Info");
				us_showtextdialog.list[DGIT_OBJECTHIGH-1].msg = _("See Node");
			}
		} else
		{
			objtype = VARCINST;
			us_showtextdialog.list[DGIT_OBJECTINFO-1].msg = _("Arc Info");
			us_showtextdialog.list[DGIT_OBJECTHIGH-1].msg = _("See Arc");
		}
	}

	/* display the text dialog box */
	dia = DiaInitDialog(&us_showtextdialog);
	if (dia == 0) return;
	if (high->fromgeom == NOGEOM && TDGETISPARAM(var->textdescript) != 0)
		maxshowoptions = 4; else
			maxshowoptions = 2;
	for(i=0; i<maxshowoptions; i++) newlang[i] = TRANSLATE(whattodisplay[i]);
	DiaSetPopup(dia, DGIT_WHATTOSHOW, maxshowoptions, newlang);
	languages = us_languagechoices();
	DiaSetPopup(dia, DGIT_LANGUAGE, 4, languages);
	DiaDimItem(dia, DGIT_EDITTEXT);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_rotationtypes[i]);
	DiaSetPopup(dia, DGIT_ROTATION, 4, newlang);

	/* enable items that may get dimmed */
	DiaUnDimItem(dia, DGIT_INSIDECELL);
	DiaUnDimItem(dia, DGIT_LANGUAGE);
	DiaUnDimItem(dia, DGIT_LANGUAGE_L);
	DiaUnDimItem(dia, DGIT_WHATTOSHOW_L);
	DiaUnDimItem(dia, DGIT_WHATTOSHOW);
	DiaUnDimItem(dia, DGIT_UNITS);
	DiaUnDimItem(dia, DGIT_UNITS_L);
	DiaUnDimItem(dia, DGIT_OBJECTINFO);
	DiaUnDimItem(dia, DGIT_OBJECTHIGH);
	DiaUnDimItem(dia, DGIT_CORNERBOXED);

	/* describe the text */
	posnotoffset = FALSE;
	cantbox = FALSE;
	infstr = initinfstr();
	if (high->fromgeom == NOGEOM)
	{
		DiaDimItem(dia, DGIT_OBJECTINFO);
		DiaDimItem(dia, DGIT_OBJECTHIGH);
		DiaSetText(dia, DGIT_XOFFSET_L, _("X position:"));
		DiaSetText(dia, DGIT_YOFFSET_L, _("Y position:"));
		formatinfstr(infstr, _("%s on cell '%s'"), us_trueattributename(var),
			describenodeproto(high->cell));
	} else
	{
		lambda = figurelambda(high->fromgeom);
		if (high->fromport != NOPORTPROTO)
		{
			DiaDimItem(dia, DGIT_OBJECTINFO);
			DiaDimItem(dia, DGIT_OBJECTHIGH);
			formatinfstr(infstr, _("%s on port '%s'"), us_trueattributename(var),
				high->fromport->protoname);
		} else
		{
			if (high->fromgeom->entryisnode)
			{
				ni = high->fromgeom->entryaddr.ni;
				if (ni->proto == gen_invispinprim)
				{
					addstringtoinfstr(infstr, us_invisiblepintextname(var));
					DiaSetText(dia, DGIT_XOFFSET_L, _("X position:"));
					DiaSetText(dia, DGIT_YOFFSET_L, _("Y position:"));
					posnotoffset = TRUE;
					DiaDimItem(dia, DGIT_OBJECTINFO);
					DiaDimItem(dia, DGIT_OBJECTHIGH);
				} else
				{
					formatinfstr(infstr, _("%s on node '%s'"), us_trueattributename(var),
						us_describenodeinsttype(ni->proto, ni, ni->userbits&NTECHBITS));
				}
				boundobj(high->fromgeom, &nlx, &nhx, &nly, &nhy);
				if (nhx == nlx || nhy == nly)
				{
					cantbox = TRUE;
					DiaDimItem(dia, DGIT_CORNERBOXED);
				}
			} else
			{
				formatinfstr(infstr, _("%s on arc '%s'"), us_trueattributename(var),
					describearcinst(high->fromgeom->entryaddr.ai));
			}
		}
	}
	DiaSetText(dia, DGIT_TEXTTYPE, returninfstr(infstr));

	/* describe how the text is shown */
	TDCOPY(descript, noevar->textdescript);
	switch (TDGETDISPPART(descript))
	{
		case VTDISPLAYVALUE:         DiaSetPopupEntry(dia, DGIT_WHATTOSHOW, 0);   break;
		case VTDISPLAYNAMEVALUE:     DiaSetPopupEntry(dia, DGIT_WHATTOSHOW, 1);   break;
		case VTDISPLAYNAMEVALINH:
			if (maxshowoptions == 4)
				DiaSetPopupEntry(dia, DGIT_WHATTOSHOW, 2);
			break;
		case VTDISPLAYNAMEVALINHALL:
			if (maxshowoptions == 4)
				DiaSetPopupEntry(dia, DGIT_WHATTOSHOW, 3);
			break;
	}

	/* describe the text units */
	for(i=0; i<8; i++) newlang[i] = TRANSLATE(us_unitnames[i]);
	DiaSetPopup(dia, DGIT_UNITS, 8, newlang);
	DiaSetPopupEntry(dia, DGIT_UNITS, TDGETUNITS(descript));

	/* handle arrays of text */
	lindex = -1;
	DiaEditControl(dia, DGIT_TEXTVALUE);
	if ((var->type&VISARRAY) != 0)
	{
		if (getlength(var) == 1) lindex = 0; else
		{
			DiaNoEditControl(dia, DGIT_TEXTVALUE);
			DiaUnDimItem(dia, DGIT_EDITTEXT);
		}
	}

	/* show the value */
	TDSETDISPPART(noevar->textdescript, 0);
	savetype = noevar->type;
	if ((noevar->type&(VCODE1|VCODE2)) != 0)
	{
		/* code: show it unevaluated */
		noevar->type = (noevar->type & ~(VCODE1|VCODE2|VTYPE)) | VSTRING;
		(void)allocstring(&formerstr, describevariable(noevar, lindex, -1),
			el_tempcluster);
	} else
	{
		/* not code: show values */
		str = describevariable(noevar, lindex, -1);
		(void)allocstring(&formerstr, str, el_tempcluster);
	}
	str = formerstr;
	noevar->type = savetype;
	TDCOPY(noevar->textdescript, descript);
	DiaSetText(dia, -DGIT_TEXTVALUE, str);

	/* set the "language" popup */
	i = noevar->type & (VCODE1|VCODE2);
	switch (i)
	{
		case 0:     DiaSetPopupEntry(dia, DGIT_LANGUAGE, 0);   break;
		case VTCL:  DiaSetPopupEntry(dia, DGIT_LANGUAGE, 1);   break;
		case VLISP: DiaSetPopupEntry(dia, DGIT_LANGUAGE, 2);   break;
		case VJAVA: DiaSetPopupEntry(dia, DGIT_LANGUAGE, 3);   break;
	}
	if (i != 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Evaluation: "));
		addstringtoinfstr(infstr, describevariable(var, -1, 0));
		DiaSetText(dia, DGIT_EVALUATION, returninfstr(infstr));
	}
	if ((var->type&VISARRAY) != 0 && lindex < 0)
	{
		DiaDimItem(dia, DGIT_LANGUAGE);
		DiaDimItem(dia, DGIT_LANGUAGE_L);
	}

	/* if this is on a node and the node is rotated, modify grab-point */
	if (high->fromgeom != NOGEOM && high->fromgeom->entryisnode)
	{
		ni = high->fromgeom->entryaddr.ni;
		if (ni->rotation != 0 || ni->transpose != 0)
			us_rotatedescript(high->fromgeom, descript);
	}

	/* set the orientation button */
	for(i=0; i<10; i++)
		if (poslist[i].value == TDGETPOS(descript))
	{
		DiaSetControl(dia, poslist[i].button, 1);
		break;
	}

	/* set the size fields */
	i = TDGETSIZE(descript);
	if (TXTGETPOINTS(i) != 0)
	{
		/* show point size */
		esnprintf(buf, 30, x_("%ld"), TXTGETPOINTS(i));
		DiaSetText(dia, DGIT_TEXTSIZEABS, buf);
		DiaSetControl(dia, DGIT_TEXTSIZEABS_L, 1);

		/* figure out how many lambda the point value is */
		height = TXTGETPOINTS(i);
		if (el_curwindowpart != NOWINDOWPART)
			height = roundfloat((float)height / el_curwindowpart->scaley);
		height = height * 4 / lambda;
		DiaSetText(dia, DGIT_TEXTSIZEREL, frtoa(height * WHOLE / 4));
	} else if (TXTGETQLAMBDA(i) != 0)
	{
		/* show lambda value */
		height = TXTGETQLAMBDA(i);
		DiaSetText(dia, DGIT_TEXTSIZEREL, frtoa(height * WHOLE / 4));
		DiaSetControl(dia, DGIT_TEXTSIZEREL_L, 1);

		/* figure out how many points the lambda value is */
		height = height * lambda / 4;
		if (el_curwindowpart != NOWINDOWPART)
			height = applyyscale(el_curwindowpart, height);
		esnprintf(buf, 30, x_("%ld"), height);
		DiaSetText(dia, DGIT_TEXTSIZEABS, buf);
	}
	if (graphicshas(CANCHOOSEFACES))
	{
		DiaUnDimItem(dia, DGIT_TEXTFACE_L);
		us_setpopupface(DGIT_TEXTFACE, TDGETFACE(descript), TRUE, dia);
	} else
	{
		DiaDimItem(dia, DGIT_TEXTFACE_L);
	}
	i = TDGETROTATION(descript);
	DiaSetPopupEntry(dia, DGIT_ROTATION, i);
	if (graphicshas(CANMODIFYFONTS))
	{
		DiaUnDimItem(dia, DGIT_TEXTITALIC);
		DiaUnDimItem(dia, DGIT_TEXTBOLD);
		DiaUnDimItem(dia, DGIT_TEXTUNDERLINE);
		if (TDGETITALIC(descript) != 0) DiaSetControl(dia, DGIT_TEXTITALIC, 1);
		if (TDGETBOLD(descript) != 0) DiaSetControl(dia, DGIT_TEXTBOLD, 1);
		if (TDGETUNDERLINE(descript) != 0) DiaSetControl(dia, DGIT_TEXTUNDERLINE, 1);
	} else
	{
		DiaDimItem(dia, DGIT_TEXTITALIC);
		DiaDimItem(dia, DGIT_TEXTBOLD);
		DiaDimItem(dia, DGIT_TEXTUNDERLINE);
	}

	/* set the interior checkbox */
	if (TDGETINTERIOR(descript) != 0) DiaSetControl(dia, DGIT_INSIDECELL, 1);

	/* set offsets */
	if (posnotoffset)
	{
		ni = high->fromgeom->entryaddr.ni;
		DiaSetText(dia, DGIT_XOFFSET, latoa((ni->lowx + ni->highx) / 2, lambda));
		DiaSetText(dia, DGIT_YOFFSET, latoa((ni->lowy + ni->highy) / 2, lambda));
	} else
	{
		i = TDGETXOFF(descript);
		DiaSetText(dia, DGIT_XOFFSET, latoa(i * lambda / 4, lambda));
		j = TDGETYOFF(descript);
		DiaSetText(dia, DGIT_YOFFSET, latoa(j * lambda / 4, lambda));
	}

	/* loop until done */
	changed = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL ||
			itemHit == DGIT_EDITTEXT || itemHit == DGIT_OBJECTINFO) break;
		if (itemHit == DGIT_OBJECTHIGH)
		{
			if (high->fromgeom != NOGEOM)
			{
				us_clearhighlightcount();
				newhigh.status = HIGHFROM;
				newhigh.cell = geomparent(high->fromgeom);
				newhigh.fromgeom = high->fromgeom;
				newhigh.fromport = NOPORTPROTO;
				newhigh.frompoint = 0;
				newhigh.fromvar = NOVARIABLE;
				newhigh.fromvarnoeval = NOVARIABLE;
				us_addhighlight(&newhigh);
				us_showallhighlight();
				us_endchanges(NOWINDOWPART);
			}
			continue;
		}
		if (itemHit >= DGIT_LOWICON && itemHit <= DGIT_HIGHICON)
		{
			DiaItemRect(dia, itemHit, &itemRect);
			DiaGetMouse(dia, &x, &y);
			itemHit = (itemHit-DGIT_LOWICON) * 2;
			if (y > (itemRect.top + itemRect.bottom) / 2) itemHit++;
			itemHit = poslist[itemHit].button;
			if (itemHit == DGIT_CORNERBOXED && cantbox) continue;
		}
		for(i=0; i<10; i++) if (itemHit == poslist[i].button)
		{
			for(j=0; j<10; j++) DiaSetControl(dia, poslist[j].button, 0);
			DiaSetControl(dia, itemHit, 1);
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIT_INSIDECELL)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			changed = TRUE;
		}
		if (itemHit == DGIT_XOFFSET || itemHit == DGIT_YOFFSET ||
			itemHit == DGIT_LANGUAGE || itemHit == DGIT_WHATTOSHOW ||
			itemHit == DGIT_TEXTFACE || itemHit == DGIT_ROTATION) changed = TRUE;
		if (itemHit == DGIT_TEXTSIZEREL) itemHit = DGIT_TEXTSIZEREL_L;
		if (itemHit == DGIT_TEXTSIZEABS) itemHit = DGIT_TEXTSIZEABS_L;
		if (itemHit == DGIT_TEXTSIZEREL_L || itemHit == DGIT_TEXTSIZEABS_L)
		{
			DiaSetControl(dia, DGIT_TEXTSIZEREL_L, 0);
			DiaSetControl(dia, DGIT_TEXTSIZEABS_L, 0);
			DiaSetControl(dia, itemHit, 1);
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIT_TEXTITALIC || itemHit == DGIT_TEXTBOLD ||
			itemHit == DGIT_TEXTUNDERLINE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			changed = TRUE;
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		/* get the new descriptor */
		TDCOPY(newdescript, descript);
		newtype = noevar->type;
		if (changed)
		{
			xcur = atola(DiaGetText(dia, DGIT_XOFFSET), lambda);
			ycur = atola(DiaGetText(dia, DGIT_YOFFSET), lambda);
			if (posnotoffset)
			{
				ni = high->fromgeom->entryaddr.ni;
				us_pushhighlight();
				us_clearhighlightcount();
				startobjectchange((INTBIG)ni, VNODEINST);
				xc = (ni->lowx + ni->highx) / 2;
				yc = (ni->lowy + ni->highy) / 2;
				modifynodeinst(ni, xcur-xc, ycur-yc, xcur-xc, ycur-yc, 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
				us_pophighlight(TRUE);
			} else
			{
				us_setdescriptoffset(newdescript, xcur*4/lambda, ycur*4/lambda);
			}
			if (DiaGetControl(dia, DGIT_TEXTSIZEABS_L) != 0)
			{
				j = eatoi(DiaGetText(dia, DGIT_TEXTSIZEABS));
				if (j <= 0) j = 4;
				if (j >= TXTMAXPOINTS) j = TXTMAXPOINTS;
				TDSETSIZE(newdescript, TXTSETPOINTS(j));
			} else
			{
				j = atofr(DiaGetText(dia, DGIT_TEXTSIZEREL)) * 4 / WHOLE;
				if (j <= 0) j = 4;
				if (j >= TXTMAXQLAMBDA) j = TXTMAXQLAMBDA;
				TDSETSIZE(newdescript, TXTSETQLAMBDA(j));
			}
			if (DiaGetControl(dia, DGIT_TEXTITALIC) != 0)
				TDSETITALIC(newdescript, VTITALIC); else
					TDSETITALIC(newdescript, 0);
			if (DiaGetControl(dia, DGIT_TEXTBOLD) != 0)
				TDSETBOLD(newdescript, VTBOLD); else
					TDSETBOLD(newdescript, 0);
			if (DiaGetControl(dia, DGIT_TEXTUNDERLINE) != 0)
				TDSETUNDERLINE(newdescript, VTUNDERLINE); else
					TDSETUNDERLINE(newdescript, 0);
			if (graphicshas(CANCHOOSEFACES))
			{
				i = us_getpopupface(DGIT_TEXTFACE, dia);
				TDSETFACE(newdescript, i);
			}
			i = DiaGetPopupEntry(dia, DGIT_ROTATION);
			TDSETROTATION(newdescript, i);
			for(j=0; j<10; j++)
				if (DiaGetControl(dia, poslist[j].button) != 0) break;
			TDSETPOS(newdescript, poslist[j].value);
			if (DiaGetControl(dia, DGIT_INSIDECELL) != 0)
				TDSETINTERIOR(newdescript, VTINTERIOR); else
					TDSETINTERIOR(newdescript, 0);
			switch (DiaGetPopupEntry(dia, DGIT_WHATTOSHOW))
			{
				case 0: TDSETDISPPART(newdescript, VTDISPLAYVALUE);          break;
				case 1: TDSETDISPPART(newdescript, VTDISPLAYNAMEVALUE);      break;
				case 2: TDSETDISPPART(newdescript, VTDISPLAYNAMEVALINH);     break;
				case 3: TDSETDISPPART(newdescript, VTDISPLAYNAMEVALINHALL);  break;
			}
			newtype &= ~(VCODE1 | VCODE2);
			switch (DiaGetPopupEntry(dia, DGIT_LANGUAGE))
			{
				case 1: newtype |= VTCL;    break;
				case 2: newtype |= VLISP;   break;
				case 3: newtype |= VJAVA;   break;
			}
			i = DiaGetPopupEntry(dia, DGIT_UNITS);
			TDSETUNITS(newdescript, i);

			/* if this is on a node and the node is rotated, modify grab-point */
			if (high->fromgeom != NOGEOM && high->fromgeom->entryisnode)
			{
				ni = high->fromgeom->entryaddr.ni;
				if (ni->rotation != 0 || ni->transpose != 0)
					us_rotatedescriptI(high->fromgeom, newdescript);
			}
		}

		/* get the new name */
		newstr = DiaGetText(dia, DGIT_TEXTVALUE);

		/* see if changes were made */
		if (TDDIFF(newdescript, descript) ||
			 newtype != noevar->type || estrcmp(str, newstr) != 0)
		{
			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* set the new descriptor */
			if (high->fromgeom == NOGEOM)
			{
				us_undrawcellvariable(noevar, high->cell);
			} else
			{
				startobjectchange((INTBIG)high->fromgeom->entryaddr.blind, objtype);
			}

			/* handle changes */
			us_modifytextdescript(high, newdescript);
			if ((newtype&(VISARRAY|VCODE1|VCODE2)) == 0)
			{
				getsimpletype(newstr, &oldtype, &newval, TDGETUNITS(newdescript));

				/* make sure that node and arc names are strings */
				if (noevar->key == el_node_name_key || noevar->key == el_arc_name_key)
				{
					oldtype = VSTRING;
					newval = (INTBIG)newstr;
				}
				newtype = (newtype & ~VTYPE) | (oldtype & VTYPE);
			} else
			{
				if ((newtype&(VCODE1|VCODE2)) != 0)
				{
					oldtype = VSTRING;
					newtype &= ~VISARRAY;
				} else
				{
					oldtype = newtype & VTYPE;
				}
				switch (oldtype)
				{
					case VINTEGER:
					case VSHORT:
					case VBOOLEAN:
						newval = myatoi(newstr);
						break;
					case VFRACT:
						newval = atofr(newstr);
						break;
					case VSTRING:
						newval = (INTBIG)newstr;
						break;
				}
			}
			if ((newtype&VISARRAY) == 0)
			{
				if (high->fromport != NOPORTPROTO)
				{
					(void)setval((INTBIG)high->fromport, VPORTPROTO,
						makename(noevar->key), newval, newtype);
				} else if (high->fromgeom == NOGEOM)
				{
					(void)setval((INTBIG)high->cell, VNODEPROTO,
						makename(noevar->key), newval, newtype);
				} else
				{
					(void)setval((INTBIG)high->fromgeom->entryaddr.blind, objtype,
						makename(noevar->key), newval, newtype);
				}
			} else
			{
				if (lindex == 0)
				{
					(void)setind((INTBIG)high->fromgeom->entryaddr.blind, objtype,
						makename(noevar->key), 0, newval);
				}
			}

			/* redisplay the text */
			if (high->fromgeom == NOGEOM)
			{
				us_drawcellvariable(noevar, high->cell);
			} else
			{
				endobjectchange((INTBIG)high->fromgeom->entryaddr.blind, objtype);
			}
			/* restore highlighting */
			us_pophighlight(TRUE);
		}
	}
	DiaDoneDialog(dia);
	if (itemHit == DGIT_EDITTEXT)
	{
		/* save and clear highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* edit the variable */
		us_editvariabletext(var, objtype,
			(INTBIG)high->fromgeom->entryaddr.blind, makename(var->key));

		/* restore highlighting */
		us_pophighlight(FALSE);
	}
	if (itemHit == DGIT_OBJECTINFO)
	{
		us_clearhighlightcount();
		newhigh.status = HIGHFROM;
		newhigh.cell = geomparent(high->fromgeom);
		newhigh.fromgeom = high->fromgeom;
		newhigh.fromport = NOPORTPROTO;
		newhigh.frompoint = 0;
		newhigh.fromvar = NOVARIABLE;
		newhigh.fromvarnoeval = NOVARIABLE;
		us_addhighlight(&newhigh);
		us_showallhighlight();
		us_endchanges(NOWINDOWPART);
		if (!high->fromgeom->entryisnode)
		{
			us_getinfoarc(high->fromgeom->entryaddr.ai);
		} else
		{
			us_getinfonode(high->fromgeom->entryaddr.ni, FALSE);
		}
	}
	if (formerstr != 0) efree(formerstr);
}

/* Port info */
static DIALOGITEM us_portinfodialogitems[] =
{
 /*  1 */ {0, {268,376,292,448}, BUTTON, N_("OK")},
 /*  2 */ {0, {268,284,292,356}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {120,8,136,56}, RADIO, N_("Left")},
 /*  4 */ {0, {8,112,24,448}, EDITTEXT, x_("")},
 /*  5 */ {0, {88,216,104,264}, EDITTEXT, x_("")},
 /*  6 */ {0, {56,8,72,88}, RADIO, N_("Center")},
 /*  7 */ {0, {72,8,88,80}, RADIO, N_("Bottom")},
 /*  8 */ {0, {88,8,104,56}, RADIO, N_("Top")},
 /*  9 */ {0, {104,8,120,64}, RADIO, N_("Right")},
 /* 10 */ {0, {136,8,152,104}, RADIO, N_("Lower right")},
 /* 11 */ {0, {152,8,168,96}, RADIO, N_("Lower left")},
 /* 12 */ {0, {168,8,184,104}, RADIO, N_("Upper right")},
 /* 13 */ {0, {184,8,200,96}, RADIO, N_("Upper left")},
 /* 14 */ {0, {88,272,104,432}, RADIO, N_("Points (max 63)")},
 /* 15 */ {0, {268,192,292,264}, BUTTON, N_("Attributes")},
 /* 16 */ {0, {40,16,56,107}, MESSAGE, N_("Text corner:")},
 /* 17 */ {0, {208,232,224,296}, EDITTEXT, x_("")},
 /* 18 */ {0, {208,384,224,448}, EDITTEXT, x_("")},
 /* 19 */ {0, {208,160,224,225}, MESSAGE, N_("X offset:")},
 /* 20 */ {0, {208,312,224,377}, MESSAGE, N_("Y offset:")},
 /* 21 */ {0, {56,112,88,144}, ICON, (CHAR *)us_icon200},
 /* 22 */ {0, {88,112,120,144}, ICON, (CHAR *)us_icon201},
 /* 23 */ {0, {120,112,152,144}, ICON, (CHAR *)us_icon202},
 /* 24 */ {0, {152,112,184,144}, ICON, (CHAR *)us_icon203},
 /* 25 */ {0, {184,112,216,144}, ICON, (CHAR *)us_icon204},
 /* 26 */ {0, {8,8,24,104}, MESSAGE, N_("Export name:")},
 /* 27 */ {0, {40,280,56,424}, POPUP, x_("")},
 /* 28 */ {0, {40,160,56,276}, MESSAGE, N_("Characteristics:")},
 /* 29 */ {0, {240,8,256,120}, CHECK, N_("Always drawn")},
 /* 30 */ {0, {216,8,232,100}, CHECK, N_("Body only")},
 /* 31 */ {0, {112,216,128,264}, EDITTEXT, x_("")},
 /* 32 */ {0, {112,272,128,432}, RADIO, N_("Lambda (max 127.75)")},
 /* 33 */ {0, {136,248,152,448}, POPUP, x_("")},
 /* 34 */ {0, {136,160,152,243}, MESSAGE, N_("Text font:")},
 /* 35 */ {0, {184,160,200,231}, CHECK, N_("Italic")},
 /* 36 */ {0, {184,256,200,327}, CHECK, N_("Bold")},
 /* 37 */ {0, {184,352,200,440}, CHECK, N_("Underline")},
 /* 38 */ {0, {100,160,116,208}, MESSAGE, N_("Size")},
 /* 39 */ {0, {268,8,292,80}, BUTTON, N_("See Node")},
 /* 40 */ {0, {268,100,292,172}, BUTTON, N_("Node Info")},
 /* 41 */ {0, {160,248,176,416}, POPUP, x_("")},
 /* 42 */ {0, {160,160,176,237}, MESSAGE, N_("Rotation:")},
 /* 43 */ {0, {232,160,248,449}, MESSAGE, x_("")},
 /* 44 */ {0, {64,160,80,276}, MESSAGE, N_("Reference name:")},
 /* 45 */ {0, {64,280,80,448}, EDITTEXT, x_("")}
};
static DIALOG us_portinfodialog = {{50,75,351,534}, N_("Export Information"), 0, 45, us_portinfodialogitems, 0, 0};

/* special items for the "Export Get Info" dialog: */
#define DGIE_CORNERLEFT        3		/* left (radio) */
#define DGIE_TEXTVALUE         4		/* text name (edit text) */
#define DGIE_ABSTEXTSIZE       5		/* absolute text size (edit text) */
#define DGIE_CORNERCENTER      6		/* center (radio) */
#define DGIE_CORNERBOT         7		/* bottom (radio) */
#define DGIE_CORNERTOP         8		/* top (radio) */
#define DGIE_CORNERRIGHT       9		/* right (radio) */
#define DGIE_CORNERLOWRIGHT   10		/* lower right (radio) */
#define DGIE_CORNERLOWLEFT    11		/* lower left (radio) */
#define DGIE_CORNERUPRIGHT    12		/* upper right (radio) */
#define DGIE_CORNERUPLEFT     13		/* upper left (radio) */
#define DGIE_ABSTEXTSIZE_L    14		/* absolute text size label (radio) */
#define DGIE_ATTRIBUTES       15		/* attributes (button) */
#define DGIE_XOFFSET          17		/* X offset (edit text) */
#define DGIE_YOFFSET          18		/* Y offset (edit text) */
#define DGIE_LOWICON          21		/* first icon (icon) */
#define DGIE_HIGHICON         25		/* last icon (icon) */
#define DGIE_CHARACTERISTICS  27		/* characteristics (popup) */
#define DGIE_ALWAYSDRAWN      29		/* always drawn (check) */
#define DGIE_BODYONLY         30		/* body only (check) */
#define DGIE_RELTEXTSIZE      31		/* relative text size (edit text) */
#define DGIE_RELTEXTSIZE_L    32		/* relative text size label (radio) */
#define DGIE_TEXTFACE         33		/* text face (popup) */
#define DGIE_TEXTFACE_L       34		/* text size label (stat text) */
#define DGIE_TEXTITALIC       35		/* text italic (check) */
#define DGIE_TEXTBOLD         36		/* text italic (check) */
#define DGIE_TEXTUNDERLINE    37		/* text italic (check) */
#define DGIE_SEETHENODE       39		/* see node (button) */
#define DGIE_INFOONNODE       40		/* get info on node (button) */
#define DGIE_ROTATION         41		/* rotation on text (popup) */
#define DGIE_OFFSETREASON     43		/* explanation of offset (stat text) */
#define DGIE_REFNAME_L        44		/* reference name label (stat text) */
#define DGIE_REFNAME          45		/* reference name (edit text) */

void us_getinfoexport(HIGHLIGHT *high)
{
	INTBIG itemHit, j, i, lambda, xcur, ycur, height;
	UINTBIG newbit;
	BOOLEAN changed;
	UINTBIG descript[TEXTDESCRIPTSIZE], newdescript[TEXTDESCRIPTSIZE];
	INTBIG x, y;
	CHAR *str, *pt, *newstr, *refname, *newlang[NUMPORTCHARS], buf[80];
	RECTAREA itemRect;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cell;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var;
	HIGHLIGHT newhigh;
	REGISTER void *dia;
	static struct butlist poslist[9] =
	{
		{VTPOSCENT,      DGIE_CORNERCENTER},
		{VTPOSUP,        DGIE_CORNERBOT},
		{VTPOSDOWN,      DGIE_CORNERTOP},
		{VTPOSLEFT,      DGIE_CORNERRIGHT},
		{VTPOSRIGHT,     DGIE_CORNERLEFT},
		{VTPOSUPLEFT,    DGIE_CORNERLOWRIGHT},
		{VTPOSUPRIGHT,   DGIE_CORNERLOWLEFT},
		{VTPOSDOWNLEFT,  DGIE_CORNERUPRIGHT},
		{VTPOSDOWNRIGHT, DGIE_CORNERUPLEFT}
	};

	/* display the port dialog box */
	dia = DiaInitDialog(&us_portinfodialog);
	if (dia == 0) return;
	for(i=0; i<NUMPORTCHARS; i++) newlang[i] = TRANSLATE(us_exportcharnames[i]);
	DiaSetPopup(dia, DGIE_CHARACTERISTICS, NUMPORTCHARS, newlang);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_rotationtypes[i]);
	DiaSetPopup(dia, DGIE_ROTATION, 4, newlang);

	/* get information about the export */
	tech = us_hightech(high);
	cell = us_highcell(high);
	if (cell == NONODEPROTO) lib = el_curlib; else
		lib = cell->lib;
	lambda = lib->lambda[tech->techindex];
	pp = high->fromport;
	ni = high->fromgeom->entryaddr.ni;
	str = pp->protoname;
	TDCOPY(descript, pp->textdescript);
	if (ni->rotation != 0 || ni->transpose != 0)
		us_rotatedescript(ni->geom, descript);

	/* set port information */
	DiaSetText(dia, -DGIE_TEXTVALUE, str);
	if ((pp->userbits&PORTDRAWN) != 0) DiaSetControl(dia, DGIE_ALWAYSDRAWN, 1);
	if ((pp->userbits&BODYONLY) != 0) DiaSetControl(dia, DGIE_BODYONLY, 1);

	/* set the export characteristics button */
	for(i=0; i<NUMPORTCHARS; i++)
		if (us_exportcharlist[i] == (pp->userbits&STATEBITS))
	{
		DiaSetPopupEntry(dia, DGIE_CHARACTERISTICS, i);
		break;
	}

	/* set reference name */
	var = getval((INTBIG)pp, VPORTPROTO, VSTRING, x_("EXPORT_reference_name"));
	if (var == NOVARIABLE) refname = x_(""); else
	{
		refname = (CHAR *)var->addr;
		DiaSetText(dia, DGIE_REFNAME, refname);
	}
	if ((pp->userbits&STATEBITS) == REFOUTPORT || (pp->userbits&STATEBITS) == REFINPORT ||
		(pp->userbits&STATEBITS) == REFBASEPORT)
	{
		DiaUnDimItem(dia, DGIE_REFNAME_L);
		DiaUnDimItem(dia, DGIE_REFNAME);
	} else
	{
		DiaDimItem(dia, DGIE_REFNAME_L);
		DiaDimItem(dia, DGIE_REFNAME);
	}

	/* set the orientation button */
	for(i=0; i<9; i++)
		if (poslist[i].value == TDGETPOS(descript))
	{
		DiaSetControl(dia, poslist[i].button, 1);
		break;
	}

	/* set the size information */
	i = TDGETSIZE(descript);
	if (TXTGETPOINTS(i) != 0)
	{
		/* show point size */
		esnprintf(buf, 30, x_("%ld"), TXTGETPOINTS(i));
		DiaSetText(dia, DGIE_ABSTEXTSIZE, buf);
		DiaSetControl(dia, DGIE_ABSTEXTSIZE_L, 1);

		/* figure out how many lambda the point value is */
		height = TXTGETPOINTS(i);
		if (el_curwindowpart != NOWINDOWPART)
			height = roundfloat((float)height / el_curwindowpart->scaley);
		height = height * 4 / lambda;
		DiaSetText(dia, DGIE_RELTEXTSIZE, frtoa(height * WHOLE / 4));
	} else if (TXTGETQLAMBDA(i) != 0)
	{
		/* show lambda value */
		height = TXTGETQLAMBDA(i);
		DiaSetText(dia, DGIE_RELTEXTSIZE, frtoa(height * WHOLE / 4));
		DiaSetControl(dia, DGIE_RELTEXTSIZE_L, 1);

		/* figure out how many points the lambda value is */
		height = height * lambda / 4;
		if (el_curwindowpart != NOWINDOWPART)
			height = applyyscale(el_curwindowpart, height);
		esnprintf(buf, 30, x_("%ld"), height);
		DiaSetText(dia, DGIE_ABSTEXTSIZE, buf);
	}
	if (graphicshas(CANCHOOSEFACES))
	{
		DiaUnDimItem(dia, DGIE_TEXTFACE_L);
		us_setpopupface(DGIE_TEXTFACE, TDGETFACE(descript), TRUE, dia);
	} else
	{
		DiaDimItem(dia, DGIE_TEXTFACE_L);
	}
	j = TDGETROTATION(descript);
	DiaSetPopupEntry(dia, DGIE_ROTATION, j);
	if (graphicshas(CANMODIFYFONTS))
	{
		DiaUnDimItem(dia, DGIE_TEXTITALIC);
		DiaUnDimItem(dia, DGIE_TEXTBOLD);
		DiaUnDimItem(dia, DGIE_TEXTUNDERLINE);
		if (TDGETITALIC(descript) != 0) DiaSetControl(dia, DGIE_TEXTITALIC, 1);
		if (TDGETBOLD(descript) != 0) DiaSetControl(dia, DGIE_TEXTBOLD, 1);
		if (TDGETUNDERLINE(descript) != 0) DiaSetControl(dia, DGIE_TEXTUNDERLINE, 1);
	} else
	{
		DiaDimItem(dia, DGIE_TEXTITALIC);
		DiaDimItem(dia, DGIE_TEXTBOLD);
		DiaDimItem(dia, DGIE_TEXTUNDERLINE);
	}

	/* set offsets */
	i = TDGETXOFF(descript);
	DiaSetText(dia, DGIE_XOFFSET, latoa(i * lambda / 4, lambda));
	j = TDGETYOFF(descript);
	DiaSetText(dia, DGIE_YOFFSET, latoa(j * lambda / 4, lambda));
	esnprintf(buf, 80, "Offset in increments of %s",
		latoa((TDGETOFFSCALE(descript)+1) * lambda / 4, lambda));
	DiaSetText(dia, DGIE_OFFSETREASON, buf);
	if (i == 0 && j == 0 && ni->proto == gen_invispinprim)
	{
		DiaDimItem(dia, DGIE_XOFFSET);
		DiaDimItem(dia, DGIE_YOFFSET);
	} else
	{
		DiaUnDimItem(dia, DGIE_XOFFSET);
		DiaUnDimItem(dia, DGIE_YOFFSET);
	}

	/* loop until done */
	changed = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK || itemHit == DGIE_ATTRIBUTES || itemHit == DGIE_INFOONNODE)
		{
			/* do not allow empty export names */
			pt = DiaGetText(dia, DGIE_TEXTVALUE);
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0)
			{
				DiaMessageInDialog(_("Must have a non-empty export name"));
				continue;
			}
			break;
		}
		if (itemHit == DGIE_SEETHENODE)
		{
			us_clearhighlightcount();
			newhigh.status = HIGHFROM;
			newhigh.cell = ni->parent;
			newhigh.fromgeom = ni->geom;
			newhigh.fromport = pp->subportproto;
			newhigh.frompoint = 0;
			newhigh.fromvar = NOVARIABLE;
			newhigh.fromvarnoeval = NOVARIABLE;
			us_addhighlight(&newhigh);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			continue;
		}
		if (itemHit >= DGIE_LOWICON && itemHit <= DGIE_HIGHICON)
		{
			DiaItemRect(dia, itemHit, &itemRect);
			DiaGetMouse(dia, &x, &y);
			itemHit = (itemHit-DGIE_LOWICON) * 2;
			if (y > (itemRect.top + itemRect.bottom) / 2) itemHit++;
			if (itemHit == 9) continue;
			itemHit = poslist[itemHit].button;
		}
		for(i=0; i<9; i++) if (itemHit == poslist[i].button)
		{
			for(j=0; j<9; j++) DiaSetControl(dia, poslist[j].button, 0);
			DiaSetControl(dia, itemHit, 1);
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIE_ALWAYSDRAWN || itemHit == DGIE_BODYONLY)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			changed = TRUE;
		}
		if (itemHit == DGIE_TEXTFACE || itemHit == DGIE_XOFFSET ||
			itemHit == DGIE_YOFFSET || itemHit == DGIE_CHARACTERISTICS ||
			itemHit == DGIE_ROTATION) changed = TRUE;
		if (itemHit == DGIE_RELTEXTSIZE) itemHit = DGIE_RELTEXTSIZE_L;
		if (itemHit == DGIE_ABSTEXTSIZE) itemHit = DGIE_ABSTEXTSIZE_L;
		if (itemHit == DGIE_RELTEXTSIZE_L || itemHit == DGIE_ABSTEXTSIZE_L)
		{
			DiaSetControl(dia, DGIE_RELTEXTSIZE_L, 0);
			DiaSetControl(dia, DGIE_ABSTEXTSIZE_L, 0);
			DiaSetControl(dia, itemHit, 1);
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIE_TEXTITALIC || itemHit == DGIE_TEXTBOLD ||
			itemHit == DGIE_TEXTUNDERLINE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIE_CHARACTERISTICS)
		{
			i = DiaGetPopupEntry(dia, DGIE_CHARACTERISTICS);
			newbit = us_exportcharlist[i];
			if (newbit == REFOUTPORT || newbit == REFINPORT || newbit == REFBASEPORT)
			{
				DiaUnDimItem(dia, DGIE_REFNAME_L);
				DiaUnDimItem(dia, DGIE_REFNAME);
			} else
			{
				DiaDimItem(dia, DGIE_REFNAME_L);
				DiaDimItem(dia, DGIE_REFNAME);
			}
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		/* get new port characteristics if applicable */
		if (changed)
		{
			newbit = pp->userbits;
			i = DiaGetPopupEntry(dia, DGIE_CHARACTERISTICS);
			newbit = (newbit & ~STATEBITS) | us_exportcharlist[i];
			if (DiaGetControl(dia, DGIE_ALWAYSDRAWN) == 0) newbit &= ~PORTDRAWN; else
				newbit |= PORTDRAWN;
			if (DiaGetControl(dia, DGIE_BODYONLY) == 0) newbit &= ~BODYONLY; else
				newbit |= BODYONLY;
			if (newbit != pp->userbits)
			{
				setval((INTBIG)pp, VPORTPROTO, x_("userbits"), newbit, VINTEGER);
				changeallports(pp);
			}
		}
		newstr = DiaGetText(dia, DGIE_REFNAME);
		while (*newstr == ' ') newstr++;
		if (estrcmp(refname, newstr) != 0)
		{
			if (*newstr == 0)
			{
				if (getval((INTBIG)pp, VPORTPROTO, VSTRING, x_("EXPORT_reference_name")) != NOVARIABLE)
					delval((INTBIG)pp, VPORTPROTO, x_("EXPORT_reference_name"));
			} else
			{
				setval((INTBIG)pp, VPORTPROTO, x_("EXPORT_reference_name"),
					(INTBIG)newstr, VSTRING);
			}
		}

		/* get the new descriptor */
		TDCOPY(newdescript, descript);
		if (changed)
		{
			xcur = atola(DiaGetText(dia, DGIE_XOFFSET), 0);
			ycur = atola(DiaGetText(dia, DGIE_YOFFSET), 0);
			us_setdescriptoffset(newdescript, xcur*4/lambda, ycur*4/lambda);
			if (DiaGetControl(dia, DGIE_ABSTEXTSIZE_L) != 0)
			{
				j = eatoi(DiaGetText(dia, DGIE_ABSTEXTSIZE));
				if (j <= 0) j = 4;
				if (j >= TXTMAXPOINTS) j = TXTMAXPOINTS;
				TDSETSIZE(newdescript, TXTSETPOINTS(j));
			} else
			{
				j = atofr(DiaGetText(dia, DGIE_RELTEXTSIZE)) * 4 / WHOLE;
				if (j <= 0) j = 4;
				if (j >= TXTMAXQLAMBDA) j = TXTMAXQLAMBDA;
				TDSETSIZE(newdescript, TXTSETQLAMBDA(j));
			}
			if (DiaGetControl(dia, DGIE_TEXTITALIC) != 0)
				TDSETITALIC(newdescript, VTITALIC); else
					TDSETITALIC(newdescript, 0);
			if (DiaGetControl(dia, DGIE_TEXTBOLD) != 0)
				TDSETBOLD(newdescript, VTBOLD); else
					TDSETBOLD(newdescript, 0);
			if (DiaGetControl(dia, DGIE_TEXTUNDERLINE) != 0)
				TDSETUNDERLINE(newdescript, VTUNDERLINE); else
					TDSETUNDERLINE(newdescript, 0);
			if (graphicshas(CANCHOOSEFACES))
			{
				i = us_getpopupface(DGIE_TEXTFACE, dia);
				TDSETFACE(newdescript, i);
			}
			j = DiaGetPopupEntry(dia, DGIE_ROTATION);
			TDSETROTATION(newdescript, j);
			for(j=0; j<9; j++)
				if (DiaGetControl(dia, poslist[j].button) != 0) break;
			TDSETPOS(newdescript, poslist[j].value);

			/* if this node is rotated, modify grab-point */
			if (ni->rotation != 0 || ni->transpose != 0)
				us_rotatedescriptI(high->fromgeom, newdescript);
		}

		/* get the new name */
		newstr = DiaGetText(dia, DGIE_TEXTVALUE);

		/* see if changes were made */
		if (TDDIFF(newdescript, descript) || estrcmp(str, newstr) != 0)
		{
			/* save highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* handle changes */
			startobjectchange((INTBIG)high->fromgeom->entryaddr.blind, VNODEINST);
			if (TDDIFF(newdescript, descript))
				us_modifytextdescript(high, newdescript);
			if (estrcmp(str, newstr) != 0) us_renameport(pp, newstr);
			endobjectchange((INTBIG)high->fromgeom->entryaddr.blind, VNODEINST);

			/* restore highlighting */
			us_pophighlight(TRUE);
		}
	}
	DiaDoneDialog(dia);
	if (itemHit == DGIE_INFOONNODE)
	{
		us_clearhighlightcount();
		newhigh.status = HIGHFROM;
		newhigh.cell = ni->parent;
		newhigh.fromgeom = ni->geom;
		newhigh.fromport = pp->subportproto;
		newhigh.frompoint = 0;
		newhigh.fromvar = NOVARIABLE;
		newhigh.fromvarnoeval = NOVARIABLE;
		us_addhighlight(&newhigh);
		us_showallhighlight();
		us_endchanges(NOWINDOWPART);
		us_getinfonode(ni, FALSE);
		return;
	}
	if (itemHit == DGIE_ATTRIBUTES)
		(void)us_attributesdlog();
}

/* Arc info */
static DIALOGITEM us_showarcdialogitems[] =
{
 /*  1 */ {0, {148,336,172,408}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,336,132,408}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {188,336,212,408}, BUTTON, N_("Attributes")},
 /*  4 */ {0, {104,88,120,176}, MESSAGE, x_("")},
 /*  5 */ {0, {8,88,24,392}, MESSAGE, x_("")},
 /*  6 */ {0, {32,88,48,392}, MESSAGE, x_("")},
 /*  7 */ {0, {56,88,72,392}, EDITTEXT, x_("")},
 /*  8 */ {0, {80,88,96,172}, EDITTEXT, x_("")},
 /*  9 */ {0, {128,88,144,320}, MESSAGE, x_("")},
 /* 10 */ {0, {80,280,96,364}, MESSAGE, x_("")},
 /* 11 */ {0, {176,88,192,320}, MESSAGE, x_("")},
 /* 12 */ {0, {8,16,24,80}, MESSAGE, N_("Type:")},
 /* 13 */ {0, {32,16,48,80}, MESSAGE, N_("Network:")},
 /* 14 */ {0, {80,16,96,80}, MESSAGE, N_("Width:")},
 /* 15 */ {0, {104,16,120,80}, MESSAGE, N_("Angle:")},
 /* 16 */ {0, {128,16,144,80}, MESSAGE, N_("Head:")},
 /* 17 */ {0, {80,216,96,280}, MESSAGE, N_("Bus size:")},
 /* 18 */ {0, {176,16,192,80}, MESSAGE, N_("Tail:")},
 /* 19 */ {0, {104,196,120,324}, CHECK, N_("Easy to Select")},
 /* 20 */ {0, {252,16,268,112}, CHECK, N_("Negated")},
 /* 21 */ {0, {276,16,292,112}, CHECK, N_("Directional")},
 /* 22 */ {0, {300,16,316,120}, CHECK, N_("Ends extend")},
 /* 23 */ {0, {252,136,268,240}, CHECK, N_("Ignore head")},
 /* 24 */ {0, {276,136,292,232}, CHECK, N_("Ignore tail")},
 /* 25 */ {0, {300,136,316,304}, CHECK, N_("Reverse head and tail")},
 /* 26 */ {0, {252,328,268,424}, CHECK, N_("Temporary")},
 /* 27 */ {0, {276,312,292,416}, CHECK, N_("Fixed-angle")},
 /* 28 */ {0, {56,16,72,80}, MESSAGE, N_("Name:")},
 /* 29 */ {0, {228,136,244,300}, MESSAGE, x_("")},
 /* 30 */ {0, {152,40,168,80}, MESSAGE, N_("At:")},
 /* 31 */ {0, {152,88,168,220}, MESSAGE, x_("")},
 /* 32 */ {0, {152,232,168,272}, BUTTON, N_("See")},
 /* 33 */ {0, {200,40,216,80}, MESSAGE, N_("At:")},
 /* 34 */ {0, {200,88,216,220}, MESSAGE, x_("")},
 /* 35 */ {0, {200,232,216,272}, BUTTON, N_("See")},
 /* 36 */ {0, {152,280,168,320}, BUTTON, N_("Info")},
 /* 37 */ {0, {200,280,216,320}, BUTTON, N_("Info")},
 /* 38 */ {0, {300,312,316,416}, CHECK, N_("Slidable")},
 /* 39 */ {0, {228,312,244,404}, CHECK, N_("Rigid")},
 /* 40 */ {0, {228,16,244,132}, MESSAGE, x_("")}
};
static DIALOG us_showarcdialog = {{50,75,375,509}, N_("Arc Information"), 0, 40, us_showarcdialogitems, 0, 0};

/* special items for the "Arc Get Info" dialog: */
#define DGIA_ATTRIBUTES    3		/* attributes (button) */
#define DGIA_ANGLE         4		/* angle (stat text) */
#define DGIA_TYPE          5		/* type (stat text) */
#define DGIA_NETWORK       6		/* network (stat text) */
#define DGIA_NAME          7		/* name (edit text) */
#define DGIA_WIDTH         8		/* width (edit text) */
#define DGIA_NODEHEAD      9		/* head node (stat text) */
#define DGIA_BUSSIZE      10		/* bus size (stat text) */
#define DGIA_NODETAIL     11		/* tail node (stat text) */
#define DGIA_EASYSELECT   19		/* easy to select (check) */
#define DGIA_NEGATED      20		/* negated (check) */
#define DGIA_DIRECTIONAL  21		/* directional (check) */
#define DGIA_ENDEXTEND    22		/* ends extend (check) */
#define DGIA_IGNOREHEAD   23		/* ignore head (check) */
#define DGIA_IGNORETAIL   24		/* ignore tail (check) */
#define DGIA_REVERSE      25		/* reverse (check) */
#define DGIA_TEMPORARY    26		/* temporarily (check) */
#define DGIA_FIXANGLE     27		/* fixed-angle (check) */
#define DGIA_EXTRAINFO    29		/* extra information (stat text) */
#define DGIA_HEADPOS      31		/* head coordinate (stat text) */
#define DGIA_SEEHEAD      32		/* see head (button) */
#define DGIA_TAILPOS      34		/* tail coordinate (stat text) */
#define DGIA_SEETAIL      35		/* see tail (button) */
#define DGIA_HEADINFO     36		/* head info (button) */
#define DGIA_TAILINFO     37		/* tail info (button) */
#define DGIA_SLIDABLE     38		/* slidable (check) */
#define DGIA_RIGID        39		/* rigid (check) */
#define DGIA_EXTRAINFO_L  40		/* extra information label (stat text) */

void us_getinfoarc(ARCINST *ai)
{
	INTBIG itemHit, rigstate, oldrigstate, lambda;
	BOOLEAN changed;
	CHAR ent[50], *str, *newstr, *newlang[30], *colorname, *colorsymbol;
	INTBIG i, wid, oldwid, end;
	REGISTER VARIABLE *var;
	HIGHLIGHT high;
	REGISTER void *dia;

	/* artwork arcs have a color popup */
	if (ai->proto->tech == art_tech) us_showarcdialogitems[DGIA_EXTRAINFO-1].type = POPUP; else
		us_showarcdialogitems[DGIA_EXTRAINFO-1].type = MESSAGE;

	/* show the dialog */
	dia = DiaInitDialog(&us_showarcdialog);
	if (dia == 0) return;

	/* enable everything */
	DiaUnDimItem(dia, DGIA_RIGID);
	DiaUnDimItem(dia, DGIA_FIXANGLE);
	DiaUnDimItem(dia, DGIA_NAME);
	DiaUnDimItem(dia, DGIA_ANGLE);
	DiaUnDimItem(dia, DGIA_SLIDABLE);
	DiaUnDimItem(dia, DGIA_NEGATED);
	DiaUnDimItem(dia, DGIA_DIRECTIONAL);
	DiaUnDimItem(dia, DGIA_ENDEXTEND);
	DiaUnDimItem(dia, DGIA_IGNOREHEAD);
	DiaUnDimItem(dia, DGIA_IGNORETAIL);
	DiaUnDimItem(dia, DGIA_REVERSE);
	DiaUnDimItem(dia, DGIA_TEMPORARY);
	DiaUnDimItem(dia, DGIA_WIDTH);
	DiaUnDimItem(dia, DGIA_EASYSELECT);
	DiaUnDimItem(dia, DGIA_EXTRAINFO);

	/* set prototype name */
	DiaSetText(dia, DGIA_TYPE, ai->proto->protoname);

	/* load the network and arc names if any */
	var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
	if (var != NOVARIABLE)
	{
		str = (CHAR *)var->addr;
		DiaSetText(dia, -DGIA_NAME, str);
	} else str = x_("");
	if (ai->network != NONETWORK)
		DiaSetText(dia, DGIA_NETWORK, describenetwork(ai->network)); else
			DiaSetText(dia, DGIA_NETWORK, _("*** NONE ***"));

	/* load the width */
	lambda = lambdaofarc(ai);
	oldwid = ai->width - arcwidthoffset(ai);
	DiaSetText(dia, DGIA_WIDTH, latoa(oldwid, lambda));

	/* load the position */
	(void)esnprintf(ent, 50, x_("(%s,%s)"), latoa(ai->end[1].xpos, lambda), latoa(ai->end[1].ypos, lambda));
	DiaSetText(dia, DGIA_HEADPOS, ent);
	DiaSetText(dia, DGIA_NODEHEAD, describenodeinst(ai->end[1].nodeinst));
	(void)esnprintf(ent, 50, x_("(%s,%s)"), latoa(ai->end[0].xpos, lambda), latoa(ai->end[0].ypos, lambda));
	DiaSetText(dia, DGIA_TAILPOS, ent);
	DiaSetText(dia, DGIA_NODETAIL, describenodeinst(ai->end[0].nodeinst));

	/* load angle */
	(void)esnprintf(ent, 50, x_(" %ld"), (ai->userbits&AANGLE) >> AANGLESH);
	DiaSetText(dia, DGIA_ANGLE, ent);

	/* load the selectability factor */
	if ((ai->userbits&HARDSELECTA) == 0) DiaSetControl(dia, DGIA_EASYSELECT, 1);

	/* load bus width if any */
	if (ai->network != NONETWORK && ai->network->buswidth > 1)
	{
		(void)esnprintf(ent, 50, x_(" %d"), ai->network->buswidth);
		DiaSetText(dia, DGIA_BUSSIZE, ent);
	} else DiaSetText(dia, DGIA_BUSSIZE, _("N/A"));

	/* set the constraint buttons */
	if (el_curconstraint == cla_constraint)
	{
		oldrigstate = 1;
		if (((ai->userbits&FIXED) == 0 || ai->changed == cla_changeclock+3) &&
			ai->changed != cla_changeclock+2)
		{
			if (ai->changed == cla_changeclock+3) oldrigstate = 3;
		} else
		{
			if (ai->changed == cla_changeclock+2) oldrigstate = 2; else
				oldrigstate = 0;
		}
		switch (oldrigstate)
		{
			case 0:
				DiaSetControl(dia, DGIA_RIGID, 1);
				break;
			case 1:
				break;
			case 2:
				DiaSetControl(dia, DGIA_RIGID, 1);
				/* FALLTHROUGH */
			case 3:
				DiaSetControl(dia, DGIA_TEMPORARY, 1);
				break;
		}
		if ((ai->userbits&FIXANG) != 0) DiaSetControl(dia, DGIA_FIXANGLE, 1);
		if ((ai->userbits&CANTSLIDE) == 0) DiaSetControl(dia, DGIA_SLIDABLE, 1);
	} else
	{
		DiaDimItem(dia, DGIA_RIGID);
		DiaDimItem(dia, DGIA_FIXANGLE);
		DiaDimItem(dia, DGIA_SLIDABLE);
		DiaDimItem(dia, DGIA_TEMPORARY);
		newstr = (CHAR *)(*(el_curconstraint->request))(x_("describearc"), (INTBIG)ai);
		if (*newstr != 0)
		{
			DiaSetText(dia, DGIA_EXTRAINFO, newstr);
			DiaSetText(dia, DGIA_EXTRAINFO_L, _("Constraint:"));
		}
	}
	if ((ai->userbits&ISNEGATED) != 0) DiaSetControl(dia, DGIA_NEGATED, 1);
	if ((ai->userbits&ISDIRECTIONAL) != 0) DiaSetControl(dia, DGIA_DIRECTIONAL, 1);
	if ((ai->userbits&NOEXTEND) == 0) DiaSetControl(dia, DGIA_ENDEXTEND, 1);
	if ((ai->userbits&NOTEND1) != 0) DiaSetControl(dia, DGIA_IGNOREHEAD, 1);
	if ((ai->userbits&NOTEND0) != 0) DiaSetControl(dia, DGIA_IGNORETAIL, 1);
	if ((ai->userbits&REVERSEEND) != 0) DiaSetControl(dia, DGIA_REVERSE, 1);

	/* show color choices if artwork */
	if (ai->proto->tech == art_tech)
	{
		DiaSetText(dia, DGIA_EXTRAINFO_L, _("Color:"));
		for(i=0; i<25; i++)
		{
			(void)ecolorname(us_colorvaluelist[i], &colorname, &colorsymbol);
			newlang[i] = TRANSLATE(colorname);
		}
		DiaSetPopup(dia, DGIA_EXTRAINFO, 25, newlang);
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, art_colorkey);
		if (var == NOVARIABLE) i = 6; else
		{
			for(i=0; i<25; i++) if (us_colorvaluelist[i] == var->addr) break;
		}
		DiaSetPopupEntry(dia, DGIA_EXTRAINFO, i);
	}

	/* if the arc can't be edited, disable all changes */
	if (us_cantedit(ai->parent, NONODEINST, FALSE))
	{
		DiaDimItem(dia, DGIA_RIGID);
		DiaDimItem(dia, DGIA_FIXANGLE);
		DiaDimItem(dia, DGIA_NAME);
		DiaDimItem(dia, DGIA_ANGLE);
		DiaDimItem(dia, DGIA_SLIDABLE);
		DiaDimItem(dia, DGIA_NEGATED);
		DiaDimItem(dia, DGIA_DIRECTIONAL);
		DiaDimItem(dia, DGIA_ENDEXTEND);
		DiaDimItem(dia, DGIA_IGNOREHEAD);
		DiaDimItem(dia, DGIA_IGNORETAIL);
		DiaDimItem(dia, DGIA_REVERSE);
		DiaDimItem(dia, DGIA_TEMPORARY);
		DiaDimItem(dia, DGIA_WIDTH);
		DiaDimItem(dia, DGIA_EASYSELECT);
		DiaDimItem(dia, DGIA_EXTRAINFO);
	}

	/* loop until done */
	changed = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL ||
			itemHit == DGIA_HEADINFO || itemHit == DGIA_TAILINFO ||
			itemHit == DGIA_ATTRIBUTES) break;
		if (itemHit == DGIA_RIGID || itemHit == DGIA_FIXANGLE ||
			itemHit == DGIA_SLIDABLE || itemHit == DGIA_NEGATED ||
			itemHit == DGIA_DIRECTIONAL || itemHit == DGIA_ENDEXTEND ||
			itemHit == DGIA_IGNOREHEAD || itemHit == DGIA_IGNORETAIL ||
			itemHit == DGIA_REVERSE || itemHit == DGIA_TEMPORARY ||
			itemHit == DGIA_EASYSELECT)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIA_EXTRAINFO)
		{
			if (ai->proto->tech == art_tech) changed = TRUE;
			continue;
		}
		if (itemHit == DGIA_NAME || itemHit == DGIA_WIDTH)
		{
			changed = TRUE;
			continue;
		}
		if (itemHit == DGIA_SEEHEAD || itemHit == DGIA_SEETAIL)
		{
			/* highlight an end */
			if (itemHit == DGIA_SEEHEAD) end = 1; else end = 0;
			us_clearhighlightcount();
			high.status = HIGHFROM;
			high.cell = ai->parent;
			high.fromgeom = ai->end[end].nodeinst->geom;
			high.fromport = ai->end[end].portarcinst->proto;
			high.frompoint = 0;
			high.fromvar = NOVARIABLE;
			high.fromvarnoeval = NOVARIABLE;
			us_addhighlight(&high);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			continue;
		}
	}

	if (itemHit != CANCEL && changed)
	{
		us_pushhighlight();
		us_clearhighlightcount();
		startobjectchange((INTBIG)ai, VARCINST);
		lambda = lambdaofarc(ai);
		wid = atola(DiaGetText(dia, DGIA_WIDTH), lambda);
		if (wid < 0) wid = oldwid;
		wid = arcwidthoffset(ai) + wid;
		startobjectchange((INTBIG)ai, VARCINST);
		if (DiaGetControl(dia, DGIA_RIGID) == 0)
		{
			if (DiaGetControl(dia, DGIA_TEMPORARY) == 0) rigstate = 1; else
				rigstate = 3;
		} else
		{
			if (DiaGetControl(dia, DGIA_TEMPORARY) == 0) rigstate = 0; else
				rigstate = 2;
		}
		if (rigstate != oldrigstate) switch (rigstate)
		{
			case 0:
				(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEREMOVETEMP, 0);
				(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPERIGID, 0);
				break;
			case 1:
				(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEREMOVETEMP, 0);
				(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPEUNRIGID, 0);
				break;
			case 2:
				(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPETEMPRIGID, 0);
				break;
			case 3:
				(void)(*el_curconstraint->setobject)((INTBIG)ai, VARCINST, CHANGETYPETEMPUNRIGID, 0);
				break;
		}
		i = DiaGetControl(dia, DGIA_FIXANGLE);
		if (i != 0 && (ai->userbits&FIXANG) == 0)
			(void)(*el_curconstraint->setobject)((INTBIG)ai,VARCINST,CHANGETYPEFIXEDANGLE,0);
		if (i == 0 && (ai->userbits&FIXANG) != 0)
			(void)(*el_curconstraint->setobject)((INTBIG)ai,VARCINST,CHANGETYPENOTFIXEDANGLE,0);
		i = DiaGetControl(dia, DGIA_SLIDABLE);
		if (i != 0 && (ai->userbits&CANTSLIDE) != 0)
			(void)(*el_curconstraint->setobject)((INTBIG)ai,VARCINST,CHANGETYPESLIDABLE,0);
		if (i == 0 && (ai->userbits&CANTSLIDE) == 0)
			(void)(*el_curconstraint->setobject)((INTBIG)ai,VARCINST,CHANGETYPENOTSLIDABLE,0);
		i = DiaGetControl(dia, DGIA_NEGATED);
		if (i != 0 && (ai->userbits&ISNEGATED) == 0)
			(void)setval((INTBIG)ai, VARCINST,x_("userbits"), ai->userbits | ISNEGATED, VINTEGER);
		if (i == 0 && (ai->userbits&ISNEGATED) != 0)
			(void)setval((INTBIG)ai, VARCINST,x_("userbits"), ai->userbits & ~ISNEGATED, VINTEGER);
		i = DiaGetControl(dia, DGIA_DIRECTIONAL);
		if (i != 0 && (ai->userbits&ISDIRECTIONAL) == 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits | ISDIRECTIONAL, VINTEGER);
		if (i == 0 && (ai->userbits&ISDIRECTIONAL) != 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~ISDIRECTIONAL, VINTEGER);
		i = DiaGetControl(dia, DGIA_ENDEXTEND);
		if (i != 0 && (ai->userbits&NOEXTEND) != 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~NOEXTEND, VINTEGER);
		if (i == 0 && (ai->userbits&NOEXTEND) == 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits | NOEXTEND, VINTEGER);
		i = DiaGetControl(dia, DGIA_IGNOREHEAD);
		if (i != 0 && (ai->userbits&NOTEND1) == 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits | NOTEND1, VINTEGER);
		if (i == 0 && (ai->userbits&NOTEND1) != 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~NOTEND1, VINTEGER);
		i = DiaGetControl(dia, DGIA_IGNORETAIL);
		if (i != 0 && (ai->userbits&NOTEND0) == 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits | NOTEND0, VINTEGER);
		if (i == 0 && (ai->userbits&NOTEND0) != 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~NOTEND0, VINTEGER);
		i = DiaGetControl(dia, DGIA_REVERSE);
		if (i != 0 && (ai->userbits&REVERSEEND) == 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits | REVERSEEND, VINTEGER);
		if (i == 0 && (ai->userbits&REVERSEEND) != 0)
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~REVERSEEND, VINTEGER);
		if (DiaGetControl(dia, DGIA_EASYSELECT) != 0) ai->userbits &= ~HARDSELECTA; else
			ai->userbits |= HARDSELECTA;
		newstr = DiaGetText(dia, DGIA_NAME);
		while (*newstr == ' ') newstr++;
		if (estrcmp(str, newstr) != 0)
		{
			if (*newstr == 0) us_setarcname(ai, (CHAR *)0); else
				us_setarcname(ai, newstr);
		}
		(void)modifyarcinst(ai, wid - ai->width, 0, 0, 0, 0);

		/* update color if it changed */
		if (ai->proto->tech == art_tech)
		{
			i = DiaGetPopupEntry(dia, DGIA_EXTRAINFO);
			if (us_colorvaluelist[i] == BLACK)
			{
				var = getvalkey((INTBIG)ai, VARCINST, VINTEGER, art_colorkey);
				if (var != NOVARIABLE)
					(void)delvalkey((INTBIG)ai, VARCINST, art_colorkey);
			} else
			{
				setvalkey((INTBIG)ai, VARCINST, art_colorkey, us_colorvaluelist[i], VINTEGER);
			}
		}
		endobjectchange((INTBIG)ai, VARCINST);
		us_pophighlight(FALSE);
	}
	DiaDoneDialog(dia);
	if (itemHit == DGIA_HEADINFO || itemHit == DGIA_TAILINFO)
	{
		if (itemHit == DGIA_HEADINFO) end = 1; else end = 0;
		us_clearhighlightcount();
		high.status = HIGHFROM;
		high.cell = ai->parent;
		high.fromgeom = ai->end[end].nodeinst->geom;
		high.fromport = ai->end[end].portarcinst->proto;
		high.frompoint = 0;
		high.fromvar = NOVARIABLE;
		high.fromvarnoeval = NOVARIABLE;
		us_addhighlight(&high);
		us_showallhighlight();
		us_endchanges(NOWINDOWPART);
		us_getinfonode(ai->end[end].nodeinst, FALSE);
	}
	if (itemHit == DGIA_ATTRIBUTES)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_attributesdlog();
	}
}

void us_getnodedisplaysize(NODEINST *ni, INTBIG *xsize, INTBIG *ysize)
{
	REGISTER BOOLEAN xyrev, serptrans;
	REGISTER INTBIG fun;
	INTBIG plx, ply, phx, phy;
	REGISTER VARIABLE *var;

	serptrans = FALSE;
	if (ni->proto->primindex != 0)
	{
		fun = (ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun == NPTRANMOS || fun == NPTRADMOS || fun == NPTRAPMOS)
		{
			var = gettrace(ni);
			if (var != NOVARIABLE) serptrans = TRUE;
		}
	}

	xyrev = FALSE;
	if (!serptrans)
	{
		if (ni->transpose == 0)
		{
			if (ni->rotation == 900 || ni->rotation == 2700) xyrev = TRUE;
		} else
		{
			if (ni->rotation == 0 || ni->rotation == 1800) xyrev = TRUE;
		}
	}
	nodesizeoffset(ni, &plx, &ply, &phx, &phy);
	if (!xyrev)
	{
		*xsize = ni->highx-ni->lowx-plx-phx;
		*ysize = ni->highy-ni->lowy-ply-phy;
	} else
	{
		*xsize = ni->highy-ni->lowy-ply-phy;
		*ysize = ni->highx-ni->lowx-plx-phx;
	}
}

void us_getnodemodfromdisplayinfo(NODEINST *ni, INTBIG xs, INTBIG ys, INTBIG xc, INTBIG yc,
	INTBIG r, INTBIG t, BOOLEAN positionchanged, INTBIG *dlx, INTBIG *dly, INTBIG *dhx,
	INTBIG *dhy, INTBIG *drot, INTBIG *dtran, BOOLEAN xyrev)
{
	INTBIG plx, ply, phx, phy, cox, coy;
	REGISTER INTBIG dx, dy, nlx, nhx, nly, nhy, swap;
	REGISTER VARIABLE *var;
	XARRAY trans;

	nodesizeoffset(ni, &plx, &ply, &phx, &phy);
	if (xyrev)
	{
		swap = xs;   xs = ys;   ys = swap;
		xc -= (phy-ply)/2;
		yc -= (phx-plx)/2;
	} else
	{
		xc -= (phx-plx)/2;
		yc -= (phy-ply)/2;
	}
	*dlx = *dhx = *dly = *dhy = 0;
	*drot = *dtran = 0;
	corneroffset(ni, ni->proto, ni->rotation, ni->transpose, &cox, &coy, FALSE);
	if (positionchanged || r != ni->rotation || t != ni->transpose ||
		xs != ni->highx-ni->lowx-plx-phx || ys != ni->highy-ni->lowy-ply-phy)
	{
		if (!positionchanged)
		{
			/* only size changed: adjust position appropriately */
			dx = (xs + plx + phx) - (ni->highx - ni->lowx);
			dy = (ys + ply + phy) - (ni->highy - ni->lowy);
			nlx = ni->lowx - dx/2;
			nhx = nlx + xs + plx + phx;
			nly = ni->lowy - dy/2;
			nhy = nly + ys + ply + phy;
			*dlx = nlx-ni->lowx;
			*dly = nly-ni->lowy;
			*dhx = nhx-ni->highx;
			*dhy = nhy-ni->highy;
			*drot = r - ni->rotation;
			*dtran = t - ni->transpose;
		} else
		{

			/* position (and possibly size) changed: update from dialog */
			if ((us_useroptions&CENTEREDPRIMITIVES) == 0)
			{
				dx = xc - ni->lowx - cox;   dy = yc - ni->lowy - coy;
				nlx = ni->lowx + dx;
				nhx = nlx + xs + plx + phx;
				nly = ni->lowy + dy;
				nhy = nly + ys + ply + phy;
			} else
			{
				var = getvalkey((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
				if (var != NOVARIABLE)
				{
					dx = ((INTBIG *)var->addr)[0] + (ni->lowx+ni->highx)/2 -
						(ni->proto->lowx+ni->proto->highx)/2;
					dy = ((INTBIG *)var->addr)[1] + (ni->lowy+ni->highy)/2 -
						(ni->proto->lowy+ni->proto->highy)/2;
					makerot(ni, trans);
					xform(dx, dy, &cox, &coy, trans);
					xc -= cox - (ni->lowx+ni->highx)/2;
					yc -= coy - (ni->lowy+ni->highy)/2;
				}
				nlx = xc - xs/2 - (plx+phx)/2;
				nhx = nlx + xs + plx + phx;
				nly = yc - ys/2 - (ply+phy)/2;
				nhy = nly + ys + ply + phy;
			}
			*dlx = nlx-ni->lowx;
			*dly = nly-ni->lowy;
			*dhx = nhx-ni->highx;
			*dhy = nhy-ni->highy;
			*drot = r - ni->rotation;
			*dtran = t - ni->transpose;
		}
	}
}

/* Get Info: Node */
static DIALOGITEM us_shownodedialogitems[] =
{
 /*  1 */ {0, {32,8,48,56}, MESSAGE, N_("Name:")},
 /*  2 */ {0, {8,8,24,56}, MESSAGE, N_("Type:")},
 /*  3 */ {0, {32,64,48,392}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,64,24,392}, MESSAGE, x_("")},
 /*  5 */ {0, {76,80,92,176}, EDITTEXT, x_("")},
 /*  6 */ {0, {100,80,116,174}, EDITTEXT, x_("")},
 /*  7 */ {0, {56,76,72,193}, MESSAGE, x_("")},
 /*  8 */ {0, {100,8,116,72}, MESSAGE, N_("Y size:")},
 /*  9 */ {0, {76,8,92,72}, MESSAGE, N_("X size:")},
 /* 10 */ {0, {76,208,92,288}, MESSAGE, N_("X position:")},
 /* 11 */ {0, {76,296,92,392}, EDITTEXT, x_("")},
 /* 12 */ {0, {100,296,116,390}, EDITTEXT, x_("")},
 /* 13 */ {0, {100,208,116,288}, MESSAGE, N_("Y position:")},
 /* 14 */ {0, {56,292,72,392}, MESSAGE, N_("Lower-left:")},
 /* 15 */ {0, {124,8,140,72}, MESSAGE, N_("Rotation:")},
 /* 16 */ {0, {124,80,140,128}, EDITTEXT, x_("")},
 /* 17 */ {0, {124,136,140,246}, AUTOCHECK, N_("Transposed")},
 /* 18 */ {0, {124,264,140,392}, AUTOCHECK, N_("Easy to Select")},
 /* 19 */ {0, {148,32,172,104}, BUTTON, N_("More")},
 /* 20 */ {0, {148,120,172,192}, BUTTON, N_("Close")},
 /* 21 */ {0, {148,208,172,280}, BUTTON, N_("Apply")},
 /* 22 */ {0, {148,300,172,372}, DEFBUTTON, N_("OK")},
 /* 23 */ {0, {180,8,196,104}, RADIOA, N_("Expanded")},
 /* 24 */ {0, {180,112,196,216}, RADIOA, N_("Unexpanded")},
 /* 25 */ {0, {180,220,196,392}, AUTOCHECK, N_("Only Visible Inside Cell")},
 /* 26 */ {0, {229,8,245,156}, MESSAGE, x_("")},
 /* 27 */ {0, {205,8,221,156}, MESSAGE, x_("")},
 /* 28 */ {0, {205,160,221,392}, EDITTEXT, x_("")},
 /* 29 */ {0, {229,160,245,392}, POPUP, x_("")},
 /* 30 */ {0, {256,20,272,108}, RADIOB, N_("Ports:")},
 /* 31 */ {0, {256,124,272,244}, RADIOB, N_("Parameters:")},
 /* 32 */ {0, {256,260,272,380}, RADIOB, N_("Attributes")},
 /* 33 */ {0, {276,8,404,392}, SCROLL, x_("")},
 /* 34 */ {0, {416,12,432,104}, AUTOCHECK, N_("Locked")},
 /* 35 */ {0, {412,212,436,284}, BUTTON, N_("Get Info")},
 /* 36 */ {0, {412,316,436,388}, BUTTON, N_("Attributes")},
 /* 37 */ {0, {445,161,461,393}, EDITTEXT, x_("")},
 /* 38 */ {0, {469,161,485,393}, POPUP, x_("")},
 /* 39 */ {0, {493,161,509,393}, MESSAGE, x_("")},
 /* 40 */ {0, {493,9,509,161}, MESSAGE, x_("")},
 /* 41 */ {0, {469,9,485,161}, MESSAGE, x_("")},
 /* 42 */ {0, {445,9,461,157}, MESSAGE, x_("")}
};
static DIALOG us_shownodedialog = {{50,75,568,478}, N_("Node Information"), 0, 42, us_shownodedialogitems, x_("shownode"), 176};

/* special items for the "shownode" dialog: */
#define DGIN_NAME_L         1		/* Name label (message) */
#define DGIN_TYPE_L         2		/* Type label (message) */
#define DGIN_NAME           3		/* Name (edittext) */
#define DGIN_TYPE           4		/* Type (message) */
#define DGIN_XSIZE          5		/* X size (edittext) */
#define DGIN_YSIZE          6		/* Y size (edittext) */
#define DGIN_SIZE           7		/* Size information (message) */
#define DGIN_YSIZE_L        8		/* Y size label (message) */
#define DGIN_XSIZE_L        9		/* X size label (message) */
#define DGIN_XPOSITION_L   10		/* X position label (message) */
#define DGIN_XPOSITION     11		/* X position (edittext) */
#define DGIN_YPOSITION     12		/* Y position (edittext) */
#define DGIN_YPOSITION_L   13		/* Y position label (message) */
#define DGIN_POSITION      14		/* Position information (message) */
#define DGIN_ROTATTION_L   15		/* Rotation label (message) */
#define DGIN_ROTATION      16		/* Rotation (edittext) */
#define DGIN_TRANSPOSE     17		/* Transposed (autocheck) */
#define DGIN_EASYSELECT    18		/* Easy to Select (autocheck) */
#define DGIN_MOREORLESS    19		/* More/Less (button) */
#define DGIN_CLOSE         20		/* Close (button) */
#define DGIN_APPLY         21		/* Apply changes (button) */
#define DGIN_OK            22		/* OK (defbutton) */
#define DGIN_EXPANDED      23		/* expanded (radioa) */
#define DGIN_UNEXPANDED    24		/* unexpanded (radioa) */
#define DGIN_VISINSIDE     25		/* Only visible inside cell (autocheck) */
#define DGIN_SPECIAL2_L    26		/* Special line 2 title (message) */
#define DGIN_SPECIAL1_L    27		/* Special line 1 title (message) */
#define DGIN_SPECIAL1      28		/* Special line 1 value (edittext) */
#define DGIN_SPECIAL2      29		/* Special line 2 value (popup) */
#define DGIN_SEEPORTS      30		/* Show ports in list (radiob) */
#define DGIN_SEEPARAMETERS 31		/* Show parameters in list (radiob) */
#define DGIN_SEEATTRIBUTES 32		/* Show parameters in list (radiob) */
#define DGIN_CONLIST       33		/* connection list (scroll) */
#define DGIN_NODELOCKED    34		/* Node is locked (autocheck) */
#define DGIN_GETINFO       35		/* Get Info on arc/export (button) */
#define DGIN_ATTRIBUTES    36		/* Attributes (button) */
#define DGIN_PARATTR       37		/* Parameter/attribute value (edittext) */
#define DGIN_PARATTRLAN    38		/* Parameter/attribute language (popup) */
#define DGIN_PARATTREV     39		/* Parameter/attribute evaluation (message) */
#define DGIN_PARATTREV_L   40		/* Parameter/attribute evaluation title (message) */
#define DGIN_PARATTRLAN_L  41		/* Parameter/attribute language title (message) */
#define DGIN_PARATTR_L     42		/* Parameter/attribute name (message) */

#define PARAMDEFAULTSTRING _(" (default)")

class EDiaShowNode : public EDialogModeless
{
public:
	EDiaShowNode();
	~EDiaShowNode();
	void reset();
	void geomhaschanged(GEOM *geom);
	void highlighthaschanged(void);
	void refill();

	static EDiaShowNode *dialog;        /* dialog instance */
	static BOOLEAN showmoreinited;		/* TRUE if "showmore" is initialized */
	static BOOLEAN showmore;			/* TRUE if in fullsize mode */
private:
	void itemHitAction(INTBIG itemHit);
	void setState(BOOLEAN valid);
	void setEditable(BOOLEAN ediatble);
	INTBIG chatportproto(PORTPROTO *pp, PORTPROTO *selected, INTBIG lineno);

	NODEINST  *ni;						/* the node being displayed in the dialog */
	BOOLEAN    makingchanges;			/* TRUE if updating the current node (ignore broadcasts) */
	BOOLEAN    xyrev;					/* TRUE if X and Y are reversed */
	INTBIG     paramcount;				/* number of parameters */
	INTBIG     paramtotal;				/* allocated space for parameters */
	INTBIG    *paramlanguage;			/* language information for each parameter */
	CHAR     **paramname;				/* name for each parameter */
	CHAR     **paramvalue;				/* value for each parameter */
	CHAR     **parameval;				/* evaluated value for each parameter */

	INTBIG     attrcount;				/* number of attributes */
	INTBIG     attrtotal;				/* allocated space for attributes */
	INTBIG    *attrlanguage;			/* language information for each attribute */
	CHAR     **attrname;				/* name for each attribute */
	CHAR     **attrvalue;				/* value for each attribute */
	CHAR     **attreval;				/* evaluated value for each attribute */

	INTBIG     serpwidth;				/* serpentine transistor width (negative if not relevant) */
	BOOLEAN    techedrel;				/* TRUE if from technology editor */
	CHAR      *initialeditfield;		/* initial value for special nodes */
	CHAR      *namestr;					/* initial node name */
	INTBIG     listcontents;			/* item number of what is in "more" list (ports, attributes, parameters) */
	BOOLEAN    changed;					/* TRUE if something changed in the dialog */
	BOOLEAN    positionchanged;			/* TRUE if position changed in the dialog */
	INTBIG     highlineno;				/* line highlighted in list */
	ARCINST   *selectedai;				/* the ARCINST that is selected in the port list */
	PORTPROTO *selectedpp;				/* the export that is selected in the port list */
	VARIABLE  *unitvar;					/* variable that holds miscellaneous units */
	double     startoffset, endangle;	/* for describing circles */
};

EDiaShowNode *EDiaShowNode::dialog = 0;
BOOLEAN       EDiaShowNode::showmore;
BOOLEAN       EDiaShowNode::showmoreinited = FALSE;

/*
 * Routine called when this object is modified
 */
void us_geomhaschanged(GEOM *geom)
{
	if (EDiaShowNode::dialog == 0) return;
	EDiaShowNode::dialog->geomhaschanged(geom);
}

/*
 * Routine called when highlight is modified
 */
void us_highlighthaschanged(void)
{
	if (EDiaShowNode::dialog == 0) return;
	EDiaShowNode::dialog->highlighthaschanged();
}

void us_getinfonode(NODEINST *ni, BOOLEAN canspecialize)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG fun;

	/* see if specialized dialogs should be shown */
	if (canspecialize)
	{
		np = ni->proto;
		if (np == mocmos_scalablentransprim || np == mocmos_scalableptransprim)
		{
			(void)us_scalabletransdlog();
			return;
		}
		if (np == sch_globalprim)
		{
			(void)us_globalsignaldlog();
			return;
		}
		fun = nodefunction(ni);
		if (fun == NPRESIST)
		{
			us_resistancedlog(ni->geom, NOVARIABLE);
			return;
		}
		if (fun == NPCAPAC || fun == NPECAPAC)
		{
			us_capacitancedlog(ni->geom, NOVARIABLE);
			return;
		}
		if (fun == NPINDUCT)
		{
			us_inductancedlog(ni->geom, NOVARIABLE);
			return;
		}
		if (fun == NPDIODE || fun == NPDIODEZ)
		{
			us_areadlog(ni);
			return;
		}
		if (np == sch_transistorprim || np == sch_transistor4prim)
		{
			switch (fun)
			{
				case NPTRANMOS:  case NPTRA4NMOS:  us_widlendlog(ni);  return;
				case NPTRADMOS:  case NPTRA4DMOS:  us_widlendlog(ni);  return;
				case NPTRAPMOS:  case NPTRA4PMOS:  us_widlendlog(ni);  return;
				case NPTRANPN:   case NPTRA4NPN:   us_areadlog(ni);    return;
				case NPTRAPNP:   case NPTRA4PNP:   us_areadlog(ni);    return;
				case NPTRANJFET: case NPTRA4NJFET: us_widlendlog(ni);  return;
				case NPTRAPJFET: case NPTRA4PJFET: us_widlendlog(ni);  return;
				case NPTRADMES:  case NPTRA4DMES:  us_widlendlog(ni);  return;
				case NPTRAEMES:  case NPTRA4EMES:  us_widlendlog(ni);  return;
			}
		}
	}

	/* if the dialog is already up, just show it */
	if (EDiaShowNode::dialog == 0)
	{
		if (!EDiaShowNode::showmoreinited)
		{
			EDiaShowNode::showmoreinited = TRUE;
			if ((us_useroptions&EXPANDEDDIALOGSDEF) != 0) EDiaShowNode::showmore = TRUE; else
				EDiaShowNode::showmore = FALSE;
		}
		if (EDiaShowNode::showmore)
		{
			us_shownodedialogitems[DGIN_MOREORLESS-1].msg = _("Less");
		} else
		{
			us_shownodedialogitems[DGIN_MOREORLESS-1].msg = _("More");
		}
		EDiaShowNode::dialog = new EDiaShowNode();
	}
	EDiaShowNode::dialog->refill();
}

EDiaShowNode::EDiaShowNode() : EDialogModeless(&us_shownodedialog)
{
	paramtotal = 0;
	attrtotal = 0;
	reset();
};

EDiaShowNode::~EDiaShowNode()
{
	if (paramtotal > 0)
	{
		for(int i=0; i<paramtotal; i++)
		{
			if (paramname[i] != 0) efree((CHAR *)paramname[i]);
			if (paramvalue[i] != 0) efree((CHAR *)paramvalue[i]);
			if (parameval[i] != 0) efree((CHAR *)parameval[i]);
		}
		efree((CHAR*)paramlanguage);
		efree((CHAR*)paramname);
		efree((CHAR*)paramvalue);
		efree((CHAR*)parameval);
	}
	if (attrtotal > 0)
	{
		for(int i=0; i<attrtotal; i++)
		{
			if (attrname[i] != 0) efree((CHAR *)attrname[i]);
			if (attrvalue[i] != 0) efree((CHAR *)attrvalue[i]);
			if (attreval[i] != 0) efree((CHAR *)attreval[i]);
		}
		efree((CHAR*)attrlanguage);
		efree((CHAR*)attrname);
		efree((CHAR*)attrvalue);
		efree((CHAR*)attreval);
	}
}

void EDiaShowNode::reset()
{
	if ((us_useroptions&EXPANDEDDIALOGSDEF) != 0) showmore = TRUE; else
		showmore = FALSE;
	showExtension( showmore );
	if (showmore)
	{
		us_shownodedialogitems[DGIN_MOREORLESS-1].msg = _("Less");
	} else
	{
		us_shownodedialogitems[DGIN_MOREORLESS-1].msg = _("More");
	}
	setText(DGIN_MOREORLESS, us_shownodedialogitems[DGIN_MOREORLESS-1].msg);

	initialeditfield = 0;
	listcontents = DGIN_SEEPORTS;
	makingchanges = FALSE;
}

void EDiaShowNode::geomhaschanged(GEOM *geom)
{
	REGISTER NODEINST *newNi;

	if (isHidden() || makingchanges) return;
	if (geom->entryisnode)
	{
		newNi = geom->entryaddr.ni;
		if (newNi != ni) return;
		refill();
	}
}

void EDiaShowNode::highlighthaschanged(void)
{
	if (isHidden() || makingchanges) return;
	refill();
}

/*
 * Routine to setup the node "get info" dialog so that all fields are prepared.
 * If "valid" is TRUE, there is a valid node in the dialog (otherwise, clear everything).
 */
void EDiaShowNode::setState(BOOLEAN valid)
{
	/* reset everything, that can be changed */
	paramcount = 0;
	attrcount = 0;
	loadTextDialog(DGIN_CONLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
	setText(DGIN_TYPE, x_(""));
	setText(DGIN_NAME, x_(""));
	setText(DGIN_XSIZE, x_(""));
	setText(DGIN_YSIZE, x_(""));
	setText(DGIN_XSIZE_L, _("X size:"));
	setText(DGIN_YSIZE_L, _("Y size:"));
	setText(DGIN_ROTATION, x_(""));
	setControl(DGIN_TRANSPOSE, 0);
	setText(DGIN_SIZE, x_(""));
	setText(DGIN_XPOSITION, x_(""));
	setText(DGIN_YPOSITION, x_(""));
	setText(DGIN_POSITION, _("Lower-left:"));
	setControl(DGIN_EASYSELECT, 0);
	setControl(DGIN_VISINSIDE, 0);
	dimItem(DGIN_SEEPARAMETERS);
	dimItem(DGIN_SEEATTRIBUTES);
	setText(DGIN_SPECIAL1, x_(""));
	setText(DGIN_SPECIAL1_L, x_(""));
	setPopup(DGIN_SPECIAL2, 0, NULL);
	setText(DGIN_SPECIAL2_L, x_(""));
	dimItem(DGIN_APPLY);
	setControl(DGIN_NODELOCKED, 0);
	setText(DGIN_PARATTR, x_(""));
	setText(DGIN_PARATTR_L, x_(""));
	setText(DGIN_PARATTREV, x_(""));
	setText(DGIN_PARATTREV_L, x_(""));
	setText(DGIN_PARATTRLAN_L, x_(""));

	if (valid)
	{
		/* have a node: enable everything */
		unDimItem(DGIN_SEEPORTS);
		unDimItem(DGIN_GETINFO);
		unDimItem(DGIN_ATTRIBUTES);
		unDimItem(DGIN_APPLY);
	} else
	{
		/* no node: disable everything */
		dimItem(DGIN_SEEPORTS);
		dimItem(DGIN_GETINFO);
		dimItem(DGIN_ATTRIBUTES);
		dimItem(DGIN_APPLY);
	}
	setEditable( valid );
}

/*
 * Routine to change state of editable fields.
 * If "editable" is TRUE, let the fields be changed (otherwise, dim the value fields).
 */
void EDiaShowNode::setEditable(BOOLEAN editable)
{
	if (editable)
	{
		unDimItem(DGIN_EXPANDED);
		unDimItem(DGIN_UNEXPANDED);
		unDimItem(DGIN_NAME);
		unDimItem(DGIN_XSIZE);
		unDimItem(DGIN_YSIZE);
		unDimItem(DGIN_ROTATION);
		unDimItem(DGIN_TRANSPOSE);
		unDimItem(DGIN_XPOSITION);
		unDimItem(DGIN_YPOSITION);
		unDimItem(DGIN_EASYSELECT);
		unDimItem(DGIN_VISINSIDE);
		unDimItem(DGIN_PARATTR);
		unDimItem(DGIN_PARATTRLAN);
		unDimItem(DGIN_SPECIAL1);
		unDimItem(DGIN_SPECIAL2);
	} else
	{
		dimItem(DGIN_EXPANDED);
		dimItem(DGIN_UNEXPANDED);
		dimItem(DGIN_NAME);
		dimItem(DGIN_XSIZE);
		dimItem(DGIN_YSIZE);
		dimItem(DGIN_ROTATION);
		dimItem(DGIN_TRANSPOSE);
		dimItem(DGIN_XPOSITION);
		dimItem(DGIN_YPOSITION);
		dimItem(DGIN_EASYSELECT);
		dimItem(DGIN_VISINSIDE);
		dimItem(DGIN_PARATTR);
		dimItem(DGIN_PARATTRLAN);
		dimItem(DGIN_SPECIAL1);
		dimItem(DGIN_SPECIAL2);
	}
}

/*
 * Routine to fill-in the node "get info" dialog with the currently selected node.
 *
 * There are two lines in the full dialog that have special meaning for special nodes:
 *    Node Type					Line 1 (edit)	Line 2 (popup)
 *    -------------------------	-------------	--------------
 *    MOS Serpentine Transistor	Width/Length
 *    Scalable MOS Transistor	Width
 *    Schematic Transistor		Type
 *    Diode						Size
 *    Resistor					Ohms
 *    Capacitor					Farads
 *    Inductor					Henrys
 *    Black-box					Function
 *    Flip-flop									Type
 *    Global					Name			Characteristics
 *    Technology-edit object	Type
 *    Artwork primitive							Color
 *    Artwork circle			Degrees
 */
void EDiaShowNode::refill(void)
{
	REGISTER NODEPROTO *np, *cnp;
	REGISTER PORTPROTO *pp, *fromport;
	REGISTER INTBIG lambda, i, j, fun, lineno, xsize, ysize;
	REGISTER BOOLEAN holdsoutline, line1used, line2used;
	INTBIG plx, phx, ply, phy, xpos, ypos, len, wid;
	REGISTER VARIABLE *var, *ovar;
	REGISTER CHAR *pt;
	CHAR **languages, ent[100], *newlang[25], *colorname, *colorsymbol;
	REGISTER UINTBIG characteristics;
	REGISTER void *infstr;
	HIGHLIGHT high;
	static CHAR *transistortype[9] = {N_("N-type MOS"), N_("Depletion MOS"), N_("P-type MOS"),
		N_("NPN Bipolar"), N_("PNP Bipolar"), N_("N-type JFET"), N_("P-type JFET"), N_("Depletion MESFET"),
		N_("Enhancement MESFET")};
	static CHAR *flipfloptype[12] = {N_("RS Master/slave"), N_("JK Master/slave"), N_("D Master/slave"),
		N_("T Master/slave"), N_("RS Positive"), N_("JK Positive"), N_("D Positive"),
		N_("T Positive"), N_("RS Negative"), N_("JK Negative"), N_("D Negative"),
		N_("T Negative")};

	/* ensure dialog visible */
	show();

	/* see what is selected */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE)
	{
		/* clear the dialog */
		setState(FALSE);
		ni = NONODEINST;
		return;
	}
	if (getlength(var) != 1)
	{
		/* too much highlighted: clear the dialog */
		setState(FALSE);
		ni = NONODEINST;
		return;
	}
	if (us_makehighlight(((CHAR **)var->addr)[0], &high)) return;
	if ((high.status&HIGHTYPE) != HIGHFROM)
	{
		/* no object selected: clear the dialog */
		setState(FALSE);
		ni = NONODEINST;
		return;
	}
	if (!high.fromgeom->entryisnode)
	{
		/* no node selected: clear the dialog */
		setState(FALSE);
		ni = NONODEINST;
		return;
	}
	ni = high.fromgeom->entryaddr.ni;
	fromport = high.fromport;
	np = ni->proto;
	lambda = lambdaofnode(ni);
	if (initialeditfield != 0)
	{
		efree((CHAR *)initialeditfield);
		initialeditfield = 0;
	}
	line1used = line2used = FALSE;

	/* things that may become dimmed: start them undimmed */
	setState(TRUE);

	/* count the number of attributes on the node */
	attrcount = 0;
	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if (TDGETISPARAM(var->textdescript) != 0) continue;
		pt = makename(var->key);
		if (namesamen(pt, x_("ATTR_"), 5) == 0) attrcount++;
	}

	/* make sure there is space */
	if (attrcount > attrtotal)
	{
		if (attrtotal > 0)
		{
			for(i=0; i<attrtotal; i++)
			{
				if (attrname[i] != 0) efree((CHAR *)attrname[i]);
				if (attrvalue[i] != 0) efree((CHAR *)attrvalue[i]);
				if (attreval[i] != 0) efree((CHAR *)attreval[i]);
			}
			efree((CHAR *)attrlanguage);
			efree((CHAR *)attrname);
			efree((CHAR *)attrvalue);
			efree((CHAR *)attreval);
		}
		attrtotal = 0;
		attrname = (CHAR **)emalloc(attrcount * (sizeof (CHAR *)), us_tool->cluster);
		attrvalue = (CHAR **)emalloc(attrcount * (sizeof (CHAR *)), us_tool->cluster);
		attreval = (CHAR **)emalloc(attrcount * (sizeof (CHAR *)), us_tool->cluster);
		attrlanguage = (INTBIG *)emalloc(attrcount * SIZEOFINTBIG, us_tool->cluster);
		if (attrname == 0 || attrvalue == 0 ||
			attreval == 0 || attrlanguage == 0) return;
		attrtotal = attrcount;
		for(i=0; i<attrtotal; i++)
		{
			attrname[i] = 0;
			attrvalue[i] = 0;
			attreval[i] = 0;
		}
	}

	/* gather the attributes */
	if (attrcount > 0)
	{
		attrcount = 0;
		for(i=0; i<ni->numvar; i++)
		{
			var = &ni->firstvar[i];
			if (TDGETISPARAM(var->textdescript) != 0) continue;
			pt = makename(var->key);
			if (namesamen(pt, x_("ATTR_"), 5) != 0) continue;
			if (attrname[attrcount] != 0)
				efree((CHAR *)attrname[attrcount]);
			if (attrvalue[attrcount] != 0)
				efree((CHAR *)attrvalue[attrcount]);
			if (attreval[attrcount] != 0)
				efree((CHAR *)attreval[attrcount]);

			(void)allocstring(&attrname[attrcount],
				truevariablename(var), us_tool->cluster);
			(void)allocstring(&attrvalue[attrcount],
				describevariable(var, -1, -1), us_tool->cluster);
			(void)allocstring(&attreval[attrcount],
				describevariable(evalvar(var, (INTBIG)ni, VNODEINST), -1, -1),
					us_tool->cluster);
			attrlanguage[attrcount] = var->type & (VCODE1|VCODE2);
			attrcount++;
		}
	}

	/* see if this node has parameters */
	paramcount = 0;
	cnp = NONODEPROTO;
	if (np->primindex == 0)
	{
		/* see if there are parameters on the instance */
		cnp = contentsview(np);
		if (cnp == NONODEPROTO) cnp = np;
		for(i=0; i<cnp->numvar; i++)
		{
			var = &cnp->firstvar[i];
			if (TDGETISPARAM(var->textdescript) != 0) paramcount++;
		}
	}

	/* make sure there is space */
	if (paramcount > paramtotal)
	{
		if (paramtotal > 0)
		{
			for(i=0; i<paramtotal; i++)
			{
				if (paramname[i] != 0) efree((CHAR *)paramname[i]);
				if (paramvalue[i] != 0) efree((CHAR *)paramvalue[i]);
				if (parameval[i] != 0) efree((CHAR *)parameval[i]);
			}
			efree((CHAR *)paramlanguage);
			efree((CHAR *)paramname);
			efree((CHAR *)paramvalue);
			efree((CHAR *)parameval);
		}
		paramtotal = 0;
		paramname = (CHAR **)emalloc(paramcount * (sizeof (CHAR *)), us_tool->cluster);
		paramvalue = (CHAR **)emalloc(paramcount * (sizeof (CHAR *)), us_tool->cluster);
		parameval = (CHAR **)emalloc(paramcount * (sizeof (CHAR *)), us_tool->cluster);
		paramlanguage = (INTBIG *)emalloc(paramcount * SIZEOFINTBIG, us_tool->cluster);
		if (paramname == 0 || paramvalue == 0 ||
			parameval == 0 || paramlanguage == 0) return;
		paramtotal = paramcount;
		for(i=0; i<paramtotal; i++)
		{
			paramname[i] = 0;
			paramvalue[i] = 0;
			parameval[i] = 0;
		}
	}

	/* gather the parameters */
	if (paramcount > 0)
	{
		paramcount = 0;
		for(i=0; i<cnp->numvar; i++)
		{
			var = &cnp->firstvar[i];
			if (TDGETISPARAM(var->textdescript) == 0) continue;
			if (paramname[paramcount] != 0)
				efree((CHAR *)paramname[paramcount]);
			if (paramvalue[paramcount] != 0)
				efree((CHAR *)paramvalue[paramcount]);
			if (parameval[paramcount] != 0)
				efree((CHAR *)parameval[paramcount]);

			(void)allocstring(&paramname[paramcount],
				truevariablename(var), us_tool->cluster);
			for(j=0; j<ni->numvar; j++)
			{
				ovar = &ni->firstvar[j];
				if (TDGETISPARAM(ovar->textdescript) == 0) continue;
				if (namesame(truevariablename(var), truevariablename(ovar)) != 0) continue;
				(void)allocstring(&paramvalue[paramcount],
					describevariable(ovar, -1, -1), us_tool->cluster);
				(void)allocstring(&parameval[paramcount],
					describevariable(evalvar(ovar, (INTBIG)ni, VNODEINST), -1, -1), us_tool->cluster);
				paramlanguage[paramcount] = ovar->type & (VCODE1|VCODE2);
				break;
			}
			if (j >= ni->numvar)
			{
				infstr = initinfstr();
				addstringtoinfstr(infstr, describevariable(var, -1, -1));
				addstringtoinfstr(infstr, PARAMDEFAULTSTRING);
				(void)allocstring(&paramvalue[paramcount],
					returninfstr(infstr), us_tool->cluster);
				(void)allocstring(&parameval[paramcount],
					describevariable(evalvar(var, (INTBIG)cnp, VNODEPROTO), -1, -1), us_tool->cluster);
				paramlanguage[paramcount] = 0;
			}
			paramcount++;
		}
	}

	/* show the appropriate list */
	if (listcontents == DGIN_SEEPARAMETERS && paramcount == 0)
	{
		if (attrcount > 0) listcontents = DGIN_SEEATTRIBUTES; else
			listcontents = DGIN_SEEPORTS;
	}
	if (listcontents == DGIN_SEEATTRIBUTES && attrcount == 0)
	{
		if (paramcount > 0) listcontents = DGIN_SEEPARAMETERS; else
			listcontents = DGIN_SEEPORTS;
	}
	setControl(DGIN_SEEPARAMETERS, 0);
	setControl(DGIN_SEEATTRIBUTES, 0);
	setControl(DGIN_SEEPORTS, 0);
	setControl(listcontents, 1);
	if (paramcount == 0) dimItem(DGIN_SEEPARAMETERS); else
		unDimItem(DGIN_SEEPARAMETERS);
	if (attrcount == 0) dimItem(DGIN_SEEATTRIBUTES); else
		unDimItem(DGIN_SEEATTRIBUTES);
	dimItem(DGIN_PARATTRLAN_L);
	dimItem(DGIN_PARATTRLAN);
	if (paramcount != 0 || attrcount != 0)
	{
		languages = us_languagechoices();
		if (paramcount != 0 && attrcount != 0)
		{
			setText(DGIN_PARATTRLAN_L, _("Parameter/Attribute type:"));
		} else if (paramcount != 0)
		{
			setText(DGIN_PARATTRLAN_L, _("Parameter type:"));
		} else if (attrcount != 0)
		{
			setText(DGIN_PARATTRLAN_L, _("Attribute type:"));
		}
		setPopup(DGIN_PARATTRLAN, 4, languages);
		unDimItem(DGIN_PARATTR);
		unDimItem(DGIN_PARATTRLAN_L);
		unDimItem(DGIN_PARATTRLAN);
	}
	initTextDialog(DGIN_CONLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCREPORT|SCHORIZBAR);

	if (listcontents == DGIN_SEEPARAMETERS)
	{
		/* show all parameters */
		highlineno = -1;
		if (paramcount == 0)
		{
			setText(DGIN_PARATTR, x_(""));
			dimItem(DGIN_PARATTR);
		}
		for(i=0; i<paramcount; i++)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s = %s"), paramname[i], paramvalue[i]);
			stuffLine(DGIN_CONLIST, returninfstr(infstr));
			if (highlineno < 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("Parameter '%s'"), paramname[i]);
				setText(DGIN_PARATTR_L, returninfstr(infstr));
				unDimItem(DGIN_PARATTR);
				setText(DGIN_PARATTR, paramvalue[i]);
				switch (paramlanguage[i])
				{
					case 0:     setPopupEntry(DGIN_PARATTRLAN, 0);   break;
					case VTCL:  setPopupEntry(DGIN_PARATTRLAN, 1);   break;
					case VLISP: setPopupEntry(DGIN_PARATTRLAN, 2);   break;
					case VJAVA: setPopupEntry(DGIN_PARATTRLAN, 3);   break;
				}
				if (paramlanguage[i] != 0)
				{
					setText(DGIN_PARATTREV_L, _("Evaluation:"));
					setText(DGIN_PARATTREV, parameval[i]);
				}
				highlineno = i;
			}
		}
		selectLine(DGIN_CONLIST, highlineno);
	} else if (listcontents == DGIN_SEEATTRIBUTES)
	{
		/* show all attributes */
		highlineno = -1;
		if (attrcount == 0)
		{
			setText(DGIN_PARATTR, x_(""));
			dimItem(DGIN_PARATTR);
		}
		for(i=0; i<attrcount; i++)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s = %s"), attrname[i], attrvalue[i]);
			stuffLine(DGIN_CONLIST, returninfstr(infstr));
			if (highlineno < 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("Attribute '%s'"), attrname[i]);
				setText(DGIN_PARATTR_L, returninfstr(infstr));
				unDimItem(DGIN_PARATTR);
				setText(DGIN_PARATTR, attrvalue[i]);
				switch (attrlanguage[i])
				{
					case 0:     setPopupEntry(DGIN_PARATTRLAN, 0);   break;
					case VTCL:  setPopupEntry(DGIN_PARATTRLAN, 1);   break;
					case VLISP: setPopupEntry(DGIN_PARATTRLAN, 2);   break;
					case VJAVA: setPopupEntry(DGIN_PARATTRLAN, 3);   break;
				}
				if (attrlanguage[i] != 0)
				{
					setText(DGIN_PARATTREV_L, _("Evaluation:"));
					setText(DGIN_PARATTREV, attreval[i]);
				}
				highlineno = i;
			}
		}
		selectLine(DGIN_CONLIST, highlineno);
	} else
	{
		/* describe all ports */
		setText(DGIN_PARATTR, x_(""));
		dimItem(DGIN_PARATTR);
		lineno = 0;
		highlineno = -1;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp == fromport) highlineno = lineno;
			lineno = chatportproto(pp, fromport, lineno);
		}
		selectLine(DGIN_CONLIST, highlineno);
	}
	dimItem(DGIN_GETINFO);

	/* see if this node has outline information */
	holdsoutline = FALSE;
	var = gettrace(ni);
	if (var != NOVARIABLE)
	{
		holdsoutline = TRUE;
		dimItem(DGIN_XSIZE);
		dimItem(DGIN_YSIZE);
	}

	/* if there is outline information on a transistor, remember that */
	serpwidth = -1;
	fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
	if (fun == NPTRANMOS || fun == NPTRADMOS || fun == NPTRAPMOS)
	{
		if (np->primindex != 0 && (np->userbits&HOLDSTRACE) != 0 && holdsoutline)
		{
			/* serpentine transistor: show width, edit length */
			serpwidth = 0;
			transistorsize(ni, &len, &wid);
			if (len != -1 && wid != -1)
			{
				esnprintf(ent, 100, _("Width=%s; Length:"), latoa(wid, lambda));
				setText(DGIN_SPECIAL1_L, ent);
				(void)allocstring(&initialeditfield, latoa(len, lambda), us_tool->cluster);
				setText(DGIN_SPECIAL1, initialeditfield);
				serpwidth = len;
				line1used = TRUE;
			}
		}
	}

	/* load the prototype name */
	setText(DGIN_TYPE, describenodeproto(np));

	/* load the node name if any */
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
	if (var != NOVARIABLE)
	{
		namestr = (CHAR *)var->addr;
		setText(-DGIN_NAME, namestr);
	} else namestr = x_("");

	/* load the size */
	xyrev = FALSE;
	if ((fun == NPTRANMOS || fun == NPTRADMOS || fun == NPTRAPMOS) && serpwidth < 0)
	{
		if (np == mocmos_scalablentransprim || np == mocmos_scalableptransprim)
			setText(DGIN_XSIZE_L, _("Max Width:")); else
				setText(DGIN_XSIZE_L, _("Width:"));
		setText(DGIN_YSIZE_L, _("Length:"));
	} else
	{
		if (ni->transpose == 0)
		{
			if (ni->rotation == 900 || ni->rotation == 2700) xyrev = TRUE;
		} else
		{
			if (ni->rotation == 0 || ni->rotation == 1800) xyrev = TRUE;
		}
		if (xyrev) setText(DGIN_SIZE, _("Transformed:")); else
		{
			if ((ni->rotation%900) != 0)
				setText(DGIN_SIZE, _("Untransformed:"));
		}
	}
	nodesizeoffset(ni, &plx, &ply, &phx, &phy);
	if (xyrev)
	{
		xsize = ni->highy-ni->lowy-ply-phy;
		ysize = ni->highx-ni->lowx-plx-phx;
	} else
	{
		xsize = ni->highx-ni->lowx-plx-phx;
		ysize = ni->highy-ni->lowy-ply-phy;
	}
	setText(DGIN_XSIZE, latoa(xsize, lambda));
	setText(DGIN_YSIZE, latoa(ysize, lambda));

	/* load the position */
	if ((us_useroptions&CENTEREDPRIMITIVES) != 0)
	{
		var = getvalkey((INTBIG)ni->proto, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		if (var != NOVARIABLE)
		{
			setText(DGIN_POSITION, _("Cell center:"));
		} else
		{
			setText(DGIN_POSITION, _("Center:"));
		}
	}
	us_getnodedisplayposition(ni, &xpos, &ypos);
	setText(DGIN_XPOSITION, latoa(xpos, lambda));
	setText(DGIN_YPOSITION, latoa(ypos, lambda));

	/* load rotation */
	setText(DGIN_ROTATION, frtoa(ni->rotation*WHOLE/10));
	if (ni->transpose != 0) setControl(DGIN_TRANSPOSE, 1);

	/* set the expansion button */
	if (np->primindex == 0)
	{
		if ((ni->userbits&NEXPAND) != 0) i = DGIN_EXPANDED; else i = DGIN_UNEXPANDED;
		setControl(i, 1);
		noEditControl(DGIN_XSIZE);
		noEditControl(DGIN_YSIZE);
	} else
	{
		dimItem(DGIN_EXPANDED);
		dimItem(DGIN_UNEXPANDED);
		editControl(DGIN_XSIZE);
		editControl(DGIN_YSIZE);
	}
	if ((ni->userbits&NVISIBLEINSIDE) != 0) setControl(DGIN_VISINSIDE, 1);

	/* load easy of selection */
	if ((ni->userbits&HARDSELECTN) == 0) setControl(DGIN_EASYSELECT, 1);
	if (np->primindex == 0 && (us_useroptions&NOINSTANCESELECT) != 0)
		dimItem(DGIN_EASYSELECT);

	/* load locked state */
	if ((ni->userbits&NILOCKED) != 0) setControl(DGIN_NODELOCKED, 1);

	/* load special node information */
	if (np == sch_diodeprim)
	{
		if ((ni->userbits&NTECHBITS) == DIODEZENER)
			setText(DGIN_SPECIAL1_L, _("Zener diode size:")); else
				setText(DGIN_SPECIAL1_L, _("Diode size:"));
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_diodekey);
		if (var == NOVARIABLE) pt = x_("0"); else
			pt = describesimplevariable(var);
		(void)allocstring(&initialeditfield, pt, us_tool->cluster);
		setText(DGIN_SPECIAL1, initialeditfield);
		line1used = TRUE;
	}
	if (np == sch_resistorprim)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Resistance"));
		formatinfstr(infstr, x_(" (%s):"),
			TRANSLATE(us_resistancenames[(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH]));
		setText(DGIN_SPECIAL1_L, returninfstr(infstr));
		unitvar = getvalkey((INTBIG)ni, VNODEINST, -1, sch_resistancekey);
		if (unitvar == NOVARIABLE) pt = x_("0"); else
			pt = describevariable(unitvar, -1, -1);
		(void)allocstring(&initialeditfield, pt, us_tool->cluster);
		setText(DGIN_SPECIAL1, initialeditfield);
		line1used = TRUE;
	}
	if (np == sch_capacitorprim)
	{
		infstr = initinfstr();
		if ((ni->userbits&NTECHBITS) == CAPACELEC)
			addstringtoinfstr(infstr, _("Electrolytic cap:")); else
				addstringtoinfstr(infstr, _("Capacitance"));
		formatinfstr(infstr, x_(" (%s):"),
			TRANSLATE(us_capacitancenames[(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH]));
		setText(DGIN_SPECIAL1_L, returninfstr(infstr));
		unitvar = getvalkey((INTBIG)ni, VNODEINST, -1, sch_capacitancekey);
		if (unitvar == NOVARIABLE) pt = x_("0"); else
			pt = describevariable(unitvar, -1, -1);
		(void)allocstring(&initialeditfield, pt, us_tool->cluster);
		setText(DGIN_SPECIAL1, initialeditfield);
		line1used = TRUE;
	}
	if (np == sch_inductorprim)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Inductance"));
		formatinfstr(infstr, x_(" (%s):"),
			TRANSLATE(us_inductancenames[(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH]));
		setText(DGIN_SPECIAL1_L, returninfstr(infstr));
		unitvar = getvalkey((INTBIG)ni, VNODEINST, -1, sch_inductancekey);
		if (unitvar == NOVARIABLE) pt = x_("0"); else
			pt = describevariable(unitvar, -1, -1);
		(void)allocstring(&initialeditfield, pt, us_tool->cluster);
		setText(DGIN_SPECIAL1, initialeditfield);
		line1used = TRUE;
	}
	if (np == sch_bboxprim)
	{
		setText(DGIN_SPECIAL1_L, _("Function:"));
		var = getvalkey((INTBIG)ni, VNODEINST, -1, sch_functionkey);
		if (var == NOVARIABLE) pt = x_(""); else
			pt = describesimplevariable(var);
		(void)allocstring(&initialeditfield, pt, us_tool->cluster);
		setText(DGIN_SPECIAL1, initialeditfield);
		line1used = TRUE;
	}
	if (np == sch_transistorprim || np == sch_transistor4prim)
	{
		switch (nodefunction(ni))
		{
			case NPTRANMOS:  case NPTRA4NMOS:  i = 0;  break;
			case NPTRADMOS:  case NPTRA4DMOS:  i = 1;  break;
			case NPTRAPMOS:  case NPTRA4PMOS:  i = 2;  break;
			case NPTRANPN:   case NPTRA4NPN:   i = 3;  break;
			case NPTRAPNP:   case NPTRA4PNP:   i = 4;  break;
			case NPTRANJFET: case NPTRA4NJFET: i = 5;  break;
			case NPTRAPJFET: case NPTRA4PJFET: i = 6;  break;
			case NPTRADMES:  case NPTRA4DMES:  i = 7;  break;
			case NPTRAEMES:  case NPTRA4EMES:  i = 8;  break;
		}
		setText(DGIN_SPECIAL1_L, _("Transistor type:"));
		setText(DGIN_SPECIAL1, TRANSLATE(transistortype[i]));
		line1used = TRUE;
	}
	if (np == sch_ffprim)
	{
		setText(DGIN_SPECIAL2_L, _("Flip-flop type:"));
		for(i=0; i<12; i++) newlang[i] = TRANSLATE(flipfloptype[i]);
		setPopup(DGIN_SPECIAL2, 12, newlang);
		switch (ni->userbits&FFTYPE)
		{
			case FFTYPERS: i = 0;   break;
			case FFTYPEJK: i = 1;   break;
			case FFTYPED:  i = 2;   break;
			case FFTYPET:  i = 3;   break;
		}
		switch (ni->userbits&FFCLOCK)
		{
			case FFCLOCKMS: i += 0;   break;
			case FFCLOCKP:  i += 4;   break;
			case FFCLOCKN:  i += 8;   break;
		}
		setPopupEntry(DGIN_SPECIAL2, i);
		line2used = TRUE;
	}
	if (np == sch_globalprim)
	{
		setText(DGIN_SPECIAL1_L, _("Global name:"));
		setText(DGIN_SPECIAL2_L, _("Characteristics:"));
		for(i=0; i<NUMPORTCHARS; i++) newlang[i] = TRANSLATE(us_exportcharnames[i]);
		setPopup(DGIN_SPECIAL2, NUMPORTCHARS, newlang);
		characteristics = ((ni->userbits&NTECHBITS) >> NTECHBITSSH) << STATEBITSSH;
		for(i=0; i<NUMPORTCHARS; i++)
			if (us_exportcharlist[i] == characteristics)
		{
			setPopupEntry(DGIN_SPECIAL2, i);
			break;
		}
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_globalnamekey);
		if (var != NOVARIABLE)
		{
			(void)allocstring(&initialeditfield, (CHAR *)var->addr, us_tool->cluster);
			setText(DGIN_SPECIAL1, initialeditfield);
		}
		line1used = line2used = TRUE;
	}
	if (np == mocmos_scalablentransprim || np == mocmos_scalableptransprim)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
		if (var == NOVARIABLE) pt = x_(""); else
			pt = describesimplevariable(var);
		(void)allocstring(&initialeditfield, pt, us_tool->cluster);
		if (var == NOVARIABLE)
		{
			(void)allocstring(&initialeditfield, latoa(xsize, lambda), us_tool->cluster);
			setText(DGIN_SPECIAL1_L, _("Actual width:"));
			setText(DGIN_SPECIAL1, initialeditfield);
		}
		line1used = TRUE;
	}

	/* get technology-edit relevance */
	techedrel = FALSE;
	pt = us_teceddescribenode(ni);
	if (pt != 0)
	{
		setText(DGIN_SPECIAL2_L, _("Tech. edit relevance:"));
		newlang[0] = pt;
		setPopup(DGIN_SPECIAL2, 1, newlang);
		dimItem(DGIN_SPECIAL2);
		line2used = TRUE;
		techedrel = TRUE;
	} else if (np->primindex != 0 && np->tech == art_tech)
	{
		setText(DGIN_SPECIAL2_L, _("Color:"));
		for(i=0; i<25; i++)
		{
			(void)ecolorname(us_colorvaluelist[i], &colorname, &colorsymbol);
			newlang[i] = TRANSLATE(colorname);
		}
		setPopup(DGIN_SPECIAL2, 25, newlang);
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
		if (var == NOVARIABLE) i = 6; else
		{
			for(i=0; i<25; i++) if (us_colorvaluelist[i] == var->addr) break;
		}
		setPopupEntry(DGIN_SPECIAL2, i);
		line2used = TRUE;
	}

	/* load the degrees of a circle if appropriate */
	if (np == art_circleprim || np == art_thickcircleprim)
	{
		getarcdegrees(ni, &startoffset, &endangle);
		if (startoffset != 0.0)
		{
			setText(DGIN_SPECIAL1_L, _("Offset angle / Degrees of circle:"));
			esnprintf(ent, 100, x_("%g / %g"), startoffset*180.0/EPI, endangle*180.0/EPI);
			(void)allocstring(&initialeditfield, ent, us_tool->cluster);
			setText(DGIN_SPECIAL1, initialeditfield);
		} else
		{
			setText(DGIN_SPECIAL1_L, _("Degrees of circle:"));
			if (endangle == 0.0) setText(DGIN_SPECIAL1, x_("360")); else
			{
				esnprintf(ent, 100, x_("%g"), endangle*180.0/EPI);
				(void)allocstring(&initialeditfield, ent, us_tool->cluster);
				setText(DGIN_SPECIAL1, initialeditfield);
			}
		}
		line1used = TRUE;
	}

	/* if the special lines weren't used, clear them */
	if (!line1used)
	{
		setText(DGIN_SPECIAL1_L, x_(""));
		setText(DGIN_SPECIAL1, x_(""));
		dimItem(DGIN_SPECIAL1);
	}
	if (!line2used)
	{
		setText(DGIN_SPECIAL2_L, x_(""));
		setPopup(DGIN_SPECIAL2, 0, 0);
		dimItem(DGIN_SPECIAL2);
	}

	/* if the node can't be edited, disable all changes */
	if (us_cantedit(ni->parent, ni, FALSE))
		setEditable(FALSE);

	changed = FALSE;
	positionchanged = FALSE;
	selectedai = NOARCINST;
	selectedpp = NOPORTPROTO;
}

/*
 * Coroutine to handle hits in the modeless "layer visibility" dialog.
 */
void EDiaShowNode::itemHitAction(INTBIG itemHit)
{
	REGISTER INTBIG i, lambda, xs, ys, xc, yc, r, t,
		j, len, dlen;
	UINTBIG newbits;
	INTBIG dlx, dhx, dly, dhy, drot, dtran, newtype, newaddr, type, addr;
	float value;
	double newstart, newend;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER void *infstr;
	CHAR *newstr, *str;
	HIGHLIGHT high;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/* the "Info" button shows a dialog of info about the selected arc or export */
	if (itemHit == DGIN_GETINFO)
	{
		if (selectedai != NOARCINST)
		{
			us_clearhighlightcount();
			high.status = HIGHFROM;
			high.cell = selectedai->parent;
			high.fromgeom = selectedai->geom;
			high.fromport = NOPORTPROTO;
			high.frompoint = 0;
			high.fromvar = NOVARIABLE;
			high.fromvarnoeval = NOVARIABLE;
			us_addhighlight(&high);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			us_getinfoarc(selectedai);
			return;
		}
		if (selectedpp != NOPORTPROTO)
		{
			us_clearhighlightcount();
			high.status = HIGHTEXT;
			high.cell = selectedpp->parent;
			high.fromgeom = selectedpp->subnodeinst->geom;
			high.fromport = selectedpp;
			high.frompoint = 0;
			high.fromvar = NOVARIABLE;
			high.fromvarnoeval = NOVARIABLE;
			us_addhighlight(&high);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			us_getinfoexport(&high);
			return;
		}
		return;
	}

	/* the "attributes" button shows parameter/attribute information */
	if (itemHit == DGIN_ATTRIBUTES)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_attributesdlog();
		return;
	}

	if (itemHit == DGIN_MOREORLESS)
	{
		showmore = !showmore;
		if (showmore)
			us_shownodedialogitems[DGIN_MOREORLESS-1].msg = _("Less"); else
	            us_shownodedialogitems[DGIN_MOREORLESS-1].msg = _("More");
		setText(DGIN_MOREORLESS, us_shownodedialogitems[DGIN_MOREORLESS-1].msg);
		showExtension( showmore );
		return;
	}

	if (itemHit == DGIN_SEEPORTS || itemHit == DGIN_SEEPARAMETERS ||
		itemHit == DGIN_SEEATTRIBUTES)
	{
		listcontents = itemHit;
		refill();
		return;
	}

	if (itemHit == DGIN_TRANSPOSE || itemHit == DGIN_EASYSELECT ||
		itemHit == DGIN_VISINSIDE || itemHit == DGIN_NODELOCKED ||
		itemHit == DGIN_EXPANDED || itemHit == DGIN_UNEXPANDED)
	{
		changed = TRUE;
		return;
	}

	if (itemHit == DGIN_NAME || itemHit == DGIN_XSIZE ||
		itemHit == DGIN_YSIZE || itemHit == DGIN_ROTATION ||
		itemHit == DGIN_PARATTR || itemHit == DGIN_PARATTRLAN ||
		itemHit == DGIN_SPECIAL1 || itemHit == DGIN_SPECIAL2)
			changed = TRUE;
	if (itemHit == DGIN_XPOSITION || itemHit == DGIN_YPOSITION)
	{
		changed = TRUE;
		positionchanged = TRUE;
	}
	if (itemHit == DGIN_PARATTRLAN)
	{
		if (listcontents == DGIN_SEEPORTS || highlineno < 0) return;
		if (listcontents == DGIN_SEEPARAMETERS)
		{
			paramlanguage[highlineno] = 0;
			switch (getPopupEntry(DGIN_PARATTRLAN))
			{
				case 1: paramlanguage[highlineno] |= VTCL;    break;
				case 2: paramlanguage[highlineno] |= VLISP;   break;
				case 3: paramlanguage[highlineno] |= VJAVA;   break;
			}
		} else
		{
			attrlanguage[highlineno] = 0;
			switch (getPopupEntry(DGIN_PARATTRLAN))
			{
				case 1: attrlanguage[highlineno] |= VTCL;    break;
				case 2: attrlanguage[highlineno] |= VLISP;   break;
				case 3: attrlanguage[highlineno] |= VJAVA;   break;
			}
		}
		return;
	}

	if (itemHit == DGIN_PARATTR)
	{
		if (listcontents == DGIN_SEEPORTS || highlineno < 0) return;
		if (listcontents == DGIN_SEEPARAMETERS && highlineno < paramcount)
		{
			(void)reallocstring(&paramvalue[highlineno],
				getText(DGIN_PARATTR), us_tool->cluster);
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s = %s"), paramname[highlineno],
				paramvalue[highlineno]);
			setScrollLine(DGIN_CONLIST, highlineno, returninfstr(infstr));
		}
		if (listcontents == DGIN_SEEATTRIBUTES && highlineno < attrcount)
		{
			(void)reallocstring(&attrvalue[highlineno],
				getText(DGIN_PARATTR), us_tool->cluster);
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s = %s"), attrname[highlineno],
				attrvalue[highlineno]);
			setScrollLine(DGIN_CONLIST, highlineno, returninfstr(infstr));
		}
		return;
	}

	if (itemHit == DGIN_CONLIST)
	{
		if (ni == NONODEINST) return;
		highlineno = getCurLine(DGIN_CONLIST);
		if (listcontents == DGIN_SEEPARAMETERS)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("Parameter '%s'"), paramname[highlineno]);
			setText(DGIN_PARATTR_L, returninfstr(infstr));
			setText(DGIN_PARATTR, paramvalue[highlineno]);
			switch (paramlanguage[highlineno])
			{
				case 0:     setPopupEntry(DGIN_PARATTRLAN, 0);   break;
				case VTCL:  setPopupEntry(DGIN_PARATTRLAN, 1);   break;
				case VLISP: setPopupEntry(DGIN_PARATTRLAN, 2);   break;
				case VJAVA: setPopupEntry(DGIN_PARATTRLAN, 3);   break;
			}
			setText(DGIN_PARATTREV_L, x_(""));
			setText(DGIN_PARATTREV, x_(""));
			if (paramlanguage[highlineno] != 0)
			{
				setText(DGIN_PARATTREV_L, _("Evaluation:"));
				setText(DGIN_PARATTREV, parameval[highlineno]);
			}
		} else if (listcontents == DGIN_SEEATTRIBUTES)
		{
			infstr = initinfstr();
			formatinfstr(infstr, x_("Attribute '%s'"), attrname[highlineno]);
			setText(DGIN_PARATTR_L, returninfstr(infstr));
			setText(DGIN_PARATTR, attrvalue[highlineno]);
			switch (attrlanguage[highlineno])
			{
				case 0:     setPopupEntry(DGIN_PARATTRLAN, 0);   break;
				case VTCL:  setPopupEntry(DGIN_PARATTRLAN, 1);   break;
				case VLISP: setPopupEntry(DGIN_PARATTRLAN, 2);   break;
				case VJAVA: setPopupEntry(DGIN_PARATTRLAN, 3);   break;
			}
			setText(DGIN_PARATTREV_L, x_(""));
			setText(DGIN_PARATTREV, x_(""));
			if (attrlanguage[highlineno] != 0)
			{
				setText(DGIN_PARATTREV_L, _("Evaluation:"));
				setText(DGIN_PARATTREV, attreval[highlineno]);
			}
		} else
		{
			selectedai = NOARCINST;
			selectedpp = NOPORTPROTO;
			ai = NOARCINST;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;
				if (ai->temp2 == highlineno) break;
			}
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				pp = pe->exportproto;
				if (pp->temp2 == highlineno) break;
			}
			if (pi == NOPORTARCINST && pe == NOPORTEXPINST)
			{
				dimItem(DGIN_GETINFO);
				return;
			}
			unDimItem(DGIN_GETINFO);
			if (pi != NOPORTARCINST)
			{
				selectedai = ai;
			} else
			{
				selectedpp = pe->exportproto;
			}
		}
		return;
	}

	if (itemHit == DGIN_OK || itemHit == DGIN_CLOSE || itemHit == DGIN_APPLY)
	{
		if (itemHit != DGIN_CLOSE && ni != NONODEINST && changed)
		{
			/* see if size, rotation, or position changed */
			np = ni->proto;
			makingchanges = TRUE;
			lambda = lambdaofnode(ni);
			xs = atola(getText(DGIN_XSIZE), lambda);
			ys = atola(getText(DGIN_YSIZE), lambda);
			xc = atola(getText(DGIN_XPOSITION), lambda);
			yc = atola(getText(DGIN_YPOSITION), lambda);
			r = atofr(getText(DGIN_ROTATION))*10/WHOLE;
			t = (getControl(DGIN_TRANSPOSE) != 0 ? 1 : 0);
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange((INTBIG)ni, VNODEINST);
			us_getnodemodfromdisplayinfo(ni, xs, ys, xc, yc, r, t, positionchanged,
				&dlx, &dly, &dhx, &dhy, &drot, &dtran, xyrev);
			modifynodeinst(ni, dlx, dly, dhx, dhy, drot, dtran);

			newbits = ni->userbits;
			if (getControl(DGIN_VISINSIDE) != 0) newbits |= NVISIBLEINSIDE; else
				newbits &= ~NVISIBLEINSIDE;
			if (getControl(DGIN_NODELOCKED) != 0) newbits |= NILOCKED; else
				newbits &= ~NILOCKED;
			if (newbits != ni->userbits)
				(void)setval((INTBIG)ni, VNODEINST, x_("userbits"), newbits, VINTEGER);

			/* update transistor width if it changed */
			if (serpwidth >= 0)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
				{
					while (*newstr == ' ') newstr++;
					lambda = lambdaofnode(ni);
					if (*newstr != 0 && atola(newstr, lambda) != serpwidth)
					{
						(void)setvalkey((INTBIG)ni, VNODEINST, el_transistor_width_key,
							atofr(newstr), VFRACT);
					}
				}
			}

			/* update diode area if it changed */
			if (np == sch_diodeprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
					us_setvariablevalue(ni->geom, sch_diodekey, newstr, VSTRING|VDISPLAY, 0);
			}

			/* update capacitance if it changed */
			if (np == sch_capacitorprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
				{
					if (isanumber(newstr))
					{
						value = figureunits(newstr, VTUNITSCAP,
							(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH);
						us_setfloatvariablevalue(ni->geom, sch_capacitancekey, unitvar, value);
					} else
					{
						TDCLEAR(descript);   defaulttextdescript(descript, ni->geom);  TDSETOFF(descript, 0, 0);
						us_setvariablevalue(ni->geom, sch_capacitancekey, newstr, VDISPLAY, descript);
					}
				}
			}

			/* update resistance if it changed */
			if (np == sch_resistorprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
				{
					if (isanumber(newstr))
					{
						value = figureunits(newstr, VTUNITSRES,
							(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH);
						us_setfloatvariablevalue(ni->geom, sch_resistancekey, unitvar, value);
					} else
					{
						TDCLEAR(descript);   defaulttextdescript(descript, ni->geom);  TDSETOFF(descript, 0, 0);
						us_setvariablevalue(ni->geom, sch_resistancekey, newstr, VDISPLAY, descript);
					}
				}
			}

			/* update inductance if it changed */
			if (np == sch_inductorprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
				{
					if (isanumber(newstr))
					{
						value = figureunits(newstr, VTUNITSIND,
							(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH);
						us_setfloatvariablevalue(ni->geom, sch_inductancekey, unitvar, value);
					} else
					{
						TDCLEAR(descript);   defaulttextdescript(descript, ni->geom);  TDSETOFF(descript, 0, 0);
						us_setvariablevalue(ni->geom, sch_inductancekey, newstr, VDISPLAY, descript);
					}
				}
			}

			/* update black-box function if it changed */
			if (np == sch_bboxprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
					us_setvariablevalue(ni->geom, sch_functionkey, newstr, VSTRING|VDISPLAY, 0);
			}

			/* update flip-flop type if it changed */
			if (np == sch_ffprim)
			{
				newbits = ni->userbits & ~NTECHBITS;
				switch (getPopupEntry(DGIN_SPECIAL2))
				{
					case 0:  newbits |= FFTYPERS | FFCLOCKMS;   break;
					case 1:  newbits |= FFTYPEJK | FFCLOCKMS;   break;
					case 2:  newbits |= FFTYPED  | FFCLOCKMS;   break;
					case 3:  newbits |= FFTYPET  | FFCLOCKMS;   break;
					case 4:  newbits |= FFTYPERS | FFCLOCKP;    break;
					case 5:  newbits |= FFTYPEJK | FFCLOCKP;    break;
					case 6:  newbits |= FFTYPED  | FFCLOCKP;    break;
					case 7:  newbits |= FFTYPET  | FFCLOCKP;    break;
					case 8:  newbits |= FFTYPERS | FFCLOCKN;    break;
					case 9:  newbits |= FFTYPEJK | FFCLOCKN;    break;
					case 10: newbits |= FFTYPED  | FFCLOCKN;    break;
					case 11: newbits |= FFTYPET  | FFCLOCKN;    break;
				}
				(void)setval((INTBIG)ni, VNODEINST, x_("userbits"), newbits, VINTEGER);
			}

			/* update global name/type if it changed */
			if (np == sch_globalprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				i = getPopupEntry(DGIN_SPECIAL2);
				newbits = (ni->userbits & ~NTECHBITS) |
					(((us_exportcharlist[i] >> STATEBITSSH) << NTECHBITSSH) & NTECHBITS);
				(void)setval((INTBIG)ni, VNODEINST, x_("userbits"), newbits, VINTEGER);
				if (namesame(newstr, initialeditfield) != 0)
					(void)setvalkey((INTBIG)ni, VNODEINST, sch_globalnamekey, (INTBIG)newstr,
						VSTRING|VDISPLAY);
			}

			/* update width of scalable layout transistors if it changed */
			if (np == mocmos_scalablentransprim || np == mocmos_scalableptransprim)
			{
				newstr = getText(DGIN_SPECIAL1);
				if (namesame(newstr, initialeditfield) != 0)
				{
					/* set width on transistor */
					getsimpletype(newstr, &newtype, &newaddr, 0);
					newtype |= VDISPLAY;
					var = setvalkey((INTBIG)ni, VNODEINST, el_attrkey_width, newaddr, newtype);
					if (var != NOVARIABLE)
						TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);
				}
			}

			/* update color if it changed */
			if (np->primindex != 0 && np->tech == art_tech && !techedrel)
			{
				i = getPopupEntry(DGIN_SPECIAL2);
				if (us_colorvaluelist[i] == BLACK)
				{
					var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
					if (var != NOVARIABLE)
						(void)delvalkey((INTBIG)ni, VNODEINST, art_colorkey);
				} else
				{
					setvalkey((INTBIG)ni, VNODEINST, art_colorkey, us_colorvaluelist[i], VINTEGER);
				}
			}

			/* update ease of selection if changed */
			if (getControl(DGIN_EASYSELECT) != 0) ni->userbits &= ~HARDSELECTN; else
				ni->userbits |= HARDSELECTN;

			/* update expansion bit if changed */
			if (np->primindex == 0)
			{
				i = ni->userbits;
				if (getControl(DGIN_EXPANDED) != 0) i |= NEXPAND; else
					i &= ~NEXPAND;
				if (i != (INTBIG)ni->userbits)
					ni->userbits = i;
			}

			/* update name if it changed */
			newstr = getText(DGIN_NAME);
			while (*newstr == ' ') newstr++;
			if (estrcmp(newstr, namestr) != 0)
			{
				/* change the name of the nodeinst */
				if (*newstr == 0)
					(void)delvalkey((INTBIG)ni, VNODEINST, el_node_name_key); else
				{
					var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
					if (var != NOVARIABLE)
					{
						TDCOPY(descript, var->textdescript);
					} else
					{
						TDCLEAR(descript);
						defaulttextdescript(descript, ni->geom);
					}
					var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)newstr, VSTRING|VDISPLAY);
					if (var != NOVARIABLE)
					{
						TDCOPY(var->textdescript, descript);
						defaulttextsize(3, var->textdescript);
					}

					/* shift text down if on a cell instance */
					if (var != NOVARIABLE && np->primindex == 0)
					{
						us_setdescriptoffset(var->textdescript,
							0, (ni->highy-ni->lowy) / el_curlib->lambda[el_curtech->techindex]);
					}
				}
			}

			/* update the circle degrees if it changed */
			if (np == art_circleprim || np == art_thickcircleprim)
			{
				str = getText(DGIN_SPECIAL1_L);
				if (estrcmp(str, _("Degrees of circle:")) == 0)
				{
					newend = eatof(getText(DGIN_SPECIAL1));
					if (newend == 360.0) newend = 0.0;
					newend = newend * EPI / 180.0;
					if (newend != endangle)
						setarcdegrees(ni, startoffset, newend);
				} else
				{
					str = getText(DGIN_SPECIAL1);
					while (*str == ' ' || *str == '\t') str++;
					newstart = eatof(str);
					while (*str != 0 && *str != '/') str++;
					if (*str == 0)
					{
						newend = newstart;
						newstart = 0.0;
					} else
					{
						str++;
						while (*str == ' ' || *str == '\t') str++;
						newend = eatof(str);
					}
					
					if (newend == 360.0) newend = newstart = 0.0;
					newend = newend * EPI / 180.0;
					newstart = newstart * EPI / 180.0;
					if (newend != endangle || newstart != startoffset)
						setarcdegrees(ni, newstart, newend);
				}
			}
			endobjectchange((INTBIG)ni, VNODEINST);

			/* update parameters if they changed */
			for(i=0; i<paramcount; i++)
			{
				var = NOVARIABLE;
				for(j=0; j<ni->numvar; j++)
				{
					var = &ni->firstvar[j];
					if (namesame(paramname[i], truevariablename(var)) == 0) break;
				}
				if (j < ni->numvar)
				{
					/* see if the value changed */
					type = var->type;
					if (estrcmp(paramvalue[i], describevariable(var, -1, -1)) == 0 &&
						(type&(VCODE1|VCODE2)) == paramlanguage[i]) continue;
					type = (type & ~(VCODE1|VCODE2)) | paramlanguage[i];
					if ((type&VDISPLAY) == 0)
					{
						type |= VDISPLAY;
						TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);
					}
					us_setvariablevalue(ni->geom, var->key, paramvalue[i], type, var->textdescript);
				} else
				{
					/* parameter not there: see if default value was changed */
					if (np->primindex == 0)
					{
						len = estrlen(paramvalue[i]);
						dlen = estrlen(PARAMDEFAULTSTRING);
						if (len < dlen || estrcmp(&paramvalue[i][len-dlen], PARAMDEFAULTSTRING) != 0)
						{
							getsimpletype(paramvalue[i], &type, &addr, 0);
							if ((paramlanguage[i]&(VCODE1|VCODE2)) != 0)
							{
								type = (type & ~(VCODE1|VCODE2)) | paramlanguage[i];
								addr = (INTBIG)paramvalue[i];
							}
							infstr = initinfstr();
							formatinfstr(infstr, x_("ATTR_%s"), paramname[i]);
							us_addparameter(ni, makekey(returninfstr(infstr)), addr, type, 0);
						}
					}
				}
			}

			/* update attributes if they changed */
			for(i=0; i<attrcount; i++)
			{
				var = NOVARIABLE;
				for(j=0; j<ni->numvar; j++)
				{
					var = &ni->firstvar[j];
					if (namesame(attrname[i], truevariablename(var)) == 0) break;
				}
				if (j < ni->numvar)
				{
					/* see if the value changed */
					type = var->type;
					if (estrcmp(attrvalue[i], describevariable(var, -1, -1)) == 0 &&
						(type&(VCODE1|VCODE2)) == attrlanguage[i]) continue;
					type = (type & ~(VCODE1|VCODE2)) | attrlanguage[i];
					if ((type&VDISPLAY) == 0)
					{
						type |= VDISPLAY;
						TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);
					}
					us_setvariablevalue(ni->geom, var->key, attrvalue[i], type, var->textdescript);
				}
			}
			us_pophighlight(TRUE);
			us_endchanges(NOWINDOWPART);
			us_state |= HIGHLIGHTSET;
			us_showallhighlight();
			flushscreen();
			makingchanges = FALSE;
		}

		if (itemHit == DGIN_OK || itemHit == DGIN_CLOSE)
		{
			hide();
		}
		return;
	}
}

/*
 * routine to display information about port prototype "pp" on nodeinst "ni".
 * If the port prototype's "temp1" is nonzero, this port has already been
 * mentioned and should not be done again.
 */
INTBIG EDiaShowNode::chatportproto(PORTPROTO *pp, PORTPROTO *selected, INTBIG lineno)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER INTBIG i, count, moreinfo, lambda;
	REGISTER CHAR *activity;
	CHAR line[256];
	REGISTER void *infstr;

	/* talk about the port prototype */
	lambda = lambdaofnode(ni);
	infstr = initinfstr();
	activity = describeportbits(pp->userbits);
	if (*activity != 0)
	{
		formatinfstr(infstr, _("%s port %s connects to"), activity, pp->protoname);
	} else
	{
		formatinfstr(infstr, _("Port %s connects to"), pp->protoname);
	}
	count = 0;
	for(i=0; pp->connects[i] != NOARCPROTO; i++)
	{
		ap = pp->connects[i];
		if ((ni->proto->primindex == 0 || ni->proto->tech != gen_tech) &&
			ap->tech == gen_tech) continue;
		if (count > 0) addstringtoinfstr(infstr, x_(","));
		addstringtoinfstr(infstr, x_(" "));
		addstringtoinfstr(infstr, ap->protoname);
		count++;
	}
	moreinfo = 0;
	if (pp == selected) moreinfo = 1;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto == pp) moreinfo = 1;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		if (pe->proto == pp) moreinfo = 1;
	if (moreinfo != 0) addstringtoinfstr(infstr, x_(":"));
	stuffLine(DGIN_CONLIST, returninfstr(infstr));
	lineno++;

	/* mention if it is highlighted */
	if (pp == selected)
	{
		stuffLine(DGIN_CONLIST, _("  Highlighted port"));
		lineno++;
	}

	/* talk about any arcs on this prototype */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (pi->proto != pp) continue;
		ai = pi->conarcinst;
		if (ai->end[0].portarcinst == pi) i = 0; else i = 1;
		(void)esnprintf(line, 256, _("  Connected at (%s,%s) to %s arc"), latoa(ai->end[i].xpos, lambda),
			latoa(ai->end[i].ypos, lambda), describearcinst(ai));
		ai->temp2 = lineno;
		stuffLine(DGIN_CONLIST, line);
		lineno++;
	}

	/* talk about any exports of this prototype */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (pe->proto != pp) continue;
		infstr = initinfstr();
		formatinfstr(infstr, _("  Available as %s export '%s'"), describeportbits(pe->exportproto->userbits),
			pe->exportproto->protoname);
		stuffLine(DGIN_CONLIST, returninfstr(infstr));
		pe->exportproto->temp2 = lineno;
		lineno++;
	}
	return(lineno);
}

/****************************** SCALABLE TRANSISTOR DIALOG ******************************/

/* Scalable Transistors */
static DIALOGITEM us_scatrndialogitems[] =
{
 /*  1 */ {0, {128,212,152,292}, BUTTON, N_("OK")},
 /*  2 */ {0, {128,12,152,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {128,112,152,192}, BUTTON, N_("More")},
 /*  4 */ {0, {8,8,24,152}, MESSAGE, N_("Number of contacts:")},
 /*  5 */ {0, {8,157,24,209}, POPUP, x_("")},
 /*  6 */ {0, {32,8,48,292}, CHECK, N_("Move contacts half-lambda closer in")},
 /*  7 */ {0, {80,8,96,152}, MESSAGE, N_("Actual width:")},
 /*  8 */ {0, {80,156,96,292}, EDITTEXT, x_("")},
 /*  9 */ {0, {56,8,72,152}, MESSAGE, N_("Maximum width:")},
 /* 10 */ {0, {56,156,72,292}, MESSAGE, x_("")},
 /* 11 */ {0, {104,156,120,292}, MESSAGE, x_("")},
 /* 12 */ {0, {104,8,120,152}, MESSAGE, x_("")}
};
static DIALOG us_scatrndialog = {{75,75,236,376}, N_("Scalable Transistor Information"), 0, 12, us_scatrndialogitems, 0, 0};

/* special items for the "global signal" dialog: */
#define DSCT_MORE            3		/* More button (button) */
#define DSCT_NUMCONTACTS     5		/* Number of contacts (popup) */
#define DSCT_INSETCONTACTS   6		/* Inset contacts (check) */
#define DSCT_ACTUALWID       8		/* Actual width of transistor (edit text) */
#define DSCT_MAXIMUMWID     10		/* Maximum width of transistor (stat text) */
#define DSCT_ACTUALEVAL     11		/* Evaluation of actual width (stat text) */
#define DSCT_ACTUALEVAL_L   12		/* Label for evaluation value (stat text) */

INTBIG us_scalabletransdlog(void)
{
	CHAR *pt, *origactualwidth, *origvarstring, newvarstring[20];
	REGISTER INTBIG itemHit, lambda, numcontacts, xsize, widlang;
	INTBIG plx, ply, phx, phy;
	REGISTER BOOLEAN insetcontacts, changed;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER void *dia;
	static CHAR *contcount[3] = {x_("0"), x_("1"), x_("2")};

	/* see what is highlighted */
	ni = (NODEINST *)us_getobject(VNODEINST, TRUE);
	if (ni == NONODEINST) return(0);

	/* see if there is a global signal name already on it */
	dia = DiaInitDialog(&us_scatrndialog);
	if (dia == 0) return(0);
	DiaSetPopup(dia, DSCT_NUMCONTACTS, 3, contcount);
	numcontacts = 2;
	insetcontacts = FALSE;
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, mocmos_transcontactkey);
	if (var == NOVARIABLE) origvarstring = x_("2"); else
		origvarstring = (CHAR *)var->addr;
	if (*origvarstring == '0' || *origvarstring == '1' || *origvarstring == '2')
	{
		numcontacts = *origvarstring - '0';
		origvarstring++;
	}
	if (*origvarstring == 'i' || *origvarstring == 'I') insetcontacts = TRUE;
	DiaSetPopupEntry(dia, DSCT_NUMCONTACTS, numcontacts);
	if (insetcontacts) DiaSetControl(dia, DSCT_INSETCONTACTS, 1);

	/* load width information */
	nodesizeoffset(ni, &plx, &ply, &phx, &phy);
	lambda = lambdaofnode(ni);
	xsize = ni->highx - ni->lowx - plx - phx;
	origactualwidth = latoa(xsize, lambda);
	DiaSetText(dia, DSCT_MAXIMUMWID, origactualwidth);
	widlang = 0;
	var = getvalkeynoeval((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
	if (var != NOVARIABLE)
	{
		widlang = var->type & (VCODE1|VCODE2);
		origactualwidth = describevariable(var, -1, -1);
		if (widlang != 0)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
			DiaSetText(dia, DSCT_ACTUALEVAL_L, _("Evaluation:"));
			DiaSetText(dia, DSCT_ACTUALEVAL, describevariable(var, -1, -1));
		}
	}
	DiaSetText(dia, DSCT_ACTUALWID, origactualwidth);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL || itemHit == DSCT_MORE) break;
		if (itemHit == DSCT_INSETCONTACTS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		numcontacts = DiaGetPopupEntry(dia, DSCT_NUMCONTACTS);
		if (DiaGetControl(dia, DSCT_INSETCONTACTS) != 0) insetcontacts = TRUE; else
			insetcontacts = FALSE;
		esnprintf(newvarstring, 20, x_("%ld"), numcontacts);
		if (insetcontacts) estrcat(newvarstring, x_("I"));
		pt = DiaGetText(dia, DSCT_ACTUALWID);
		if (namesame(pt, origactualwidth) != 0 || namesame(newvarstring, origvarstring) != 0)
			changed = TRUE; else changed = FALSE;
		if (changed)
		{
			us_pushhighlight();
			us_clearhighlightcount();
			startobjectchange((INTBIG)ni, VNODEINST);
			if (namesame(newvarstring, origvarstring) != 0)
				setvalkey((INTBIG)ni, VNODEINST, mocmos_transcontactkey, (INTBIG)newvarstring, VSTRING);
			if (namesame(pt, origactualwidth) != 0)
			{
				var = setvalkey((INTBIG)ni, VNODEINST, el_attrkey_width, (INTBIG)pt, VSTRING|VDISPLAY|widlang);
				if (var != NOVARIABLE)
					TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			us_pophighlight(TRUE);
		}
	}
	DiaDoneDialog(dia);

	if (itemHit == DSCT_MORE)
	{
		/* get info on node */
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
	return(0);
}

/****************************** GLOBAL SIGNAL DIALOG ******************************/

/* Global Signal */
static DIALOGITEM us_globdialogitems[] =
{
 /*  1 */ {0, {100,196,124,276}, BUTTON, N_("OK")},
 /*  2 */ {0, {100,12,124,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,172}, MESSAGE, N_("Global signal name:")},
 /*  4 */ {0, {32,8,48,280}, EDITTEXT, x_("")},
 /*  5 */ {0, {64,8,80,140}, MESSAGE, N_("Characteristics:")},
 /*  6 */ {0, {64,144,80,284}, POPUP, x_("")},
 /*  7 */ {0, {100,104,124,184}, BUTTON, N_("More")}
};
static DIALOG us_globdialog = {{75,75,208,368}, N_("Global Signal"), 0, 7, us_globdialogitems, 0, 0};

/* special items for the "global signal" dialog: */
#define DGLO_SIGNAME         4		/* Signal name (edit text) */
#define DGLO_CHARACTERISTICS 6		/* Characteristics (popup) */
#define DGLO_MORE            7		/* More information (button) */

INTBIG us_globalsignaldlog(void)
{
	CHAR *newlang[NUMPORTCHARS], *newsigname;
	REGISTER INTBIG i, itemHit;
	REGISTER UINTBIG characteristic;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER void *dia;

	/* see what is highlighted */
	ni = (NODEINST *)us_getobject(VNODEINST, TRUE);
	if (ni == NONODEINST) return(0);
	if (ni->proto != sch_globalprim) return(0);

	/* see if there is a global signal name already on it */
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_globalnamekey);
	if (var == NOVARIABLE) us_globdialog.items = 6; else
		us_globdialog.items = 7;
	dia = DiaInitDialog(&us_globdialog);
	if (dia == 0) return(0);

	for(i=0; i<NUMPORTCHARS; i++) newlang[i] = TRANSLATE(us_exportcharnames[i]);
	DiaSetPopup(dia, DGLO_CHARACTERISTICS, NUMPORTCHARS, newlang);
	characteristic = ((ni->userbits&NTECHBITS) >> NTECHBITSSH) << STATEBITSSH;
	for(i=0; i<NUMPORTCHARS; i++)
		if (us_exportcharlist[i] == characteristic)
	{
		DiaSetPopupEntry(dia, DGLO_CHARACTERISTICS, i);
		break;
	}
	if (var != NOVARIABLE)
		DiaSetText(dia, DGLO_SIGNAME, (CHAR *)var->addr);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL || itemHit == DGLO_MORE) break;
	}

	if (itemHit == CANCEL) newsigname = 0; else
	{
		newsigname = DiaGetText(dia, DGLO_SIGNAME);
		(void)setvalkey((INTBIG)ni, VNODEINST, sch_globalnamekey, (INTBIG)newsigname, VSTRING|VDISPLAY);
		i = DiaGetPopupEntry(dia, DGLO_CHARACTERISTICS);
		ni->userbits = (ni->userbits & ~NTECHBITS) |
			(((us_exportcharlist[i] >> STATEBITSSH) << NTECHBITSSH) & NTECHBITS);
	}
	DiaDoneDialog(dia);

	if (itemHit == DGLO_MORE)
	{
		/* get info on node */
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
	return(0);
}

/****************************** GRID OPTIONS DIALOG ******************************/

/* Grid options */
static DIALOGITEM us_griddialogitems[] =
{
 /*  1 */ {0, {116,384,140,448}, BUTTON, N_("OK")},
 /*  2 */ {0, {116,12,140,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,269}, MESSAGE, N_("Grid dot spacing (current window):")},
 /*  4 */ {0, {32,272,48,352}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,272,24,366}, MESSAGE, N_("Horizontal:")},
 /*  6 */ {0, {32,372,48,452}, EDITTEXT, x_("")},
 /*  7 */ {0, {60,8,76,269}, MESSAGE, N_("Default grid spacing (new windows):")},
 /*  8 */ {0, {60,272,76,352}, EDITTEXT, x_("")},
 /*  9 */ {0, {8,372,24,466}, MESSAGE, N_("Vertical:")},
 /* 10 */ {0, {60,372,76,452}, EDITTEXT, x_("")},
 /* 11 */ {0, {88,8,104,269}, MESSAGE, N_("Distance between bold dots:")},
 /* 12 */ {0, {88,272,104,352}, EDITTEXT, x_("")},
 /* 13 */ {0, {120,132,136,329}, CHECK, N_("Align grid with circuitry")},
 /* 14 */ {0, {88,372,104,452}, EDITTEXT, x_("")}
};
static DIALOG us_griddialog = {{50,75,199,550}, N_("Grid Options"), 0, 14, us_griddialogitems, 0, 0};

/* special items for the "grid settings" dialog: */
#define DGRD_HORIZSPAC       4		/* Horizontal spacing (edit text) */
#define DGRD_VERTSPAC        6		/* Vertical spacing (edit text) */
#define DGRD_DEFHORIZSPAC    8		/* Default horizontal spacing (edit text) */
#define DGRD_DEFVERTSPAC    10		/* Default vertical spacing (edit text) */
#define DGRD_BOLDHORIZSPAC  12		/* Bold dot horizontal spacing (edit text) */
#define DGRD_ALIGNDOTS      13		/* Align with circuitry (check) */
#define DGRD_BOLDVERTSPAC   14		/* Bold dot vertical spacing (edit text) */

INTBIG us_griddlog(void)
{
	REGISTER INTBIG itemHit, xspacing, yspacing, xboldspacing, yboldspacing,
		newgridx, newgridy, oldgridfloats, gridfloats;
	INTBIG defgrid[2], bolddots[2];
	REGISTER VARIABLE *var;
	CHAR buf[20];
	REGISTER void *dia;

	/* display the grid settings dialog box */
	dia = DiaInitDialog(&us_griddialog);
	if (dia == 0) return(0);
	if (el_curwindowpart == NOWINDOWPART || el_curwindowpart->curnodeproto == NONODEPROTO)
	{
		DiaDimItem(dia, DGRD_HORIZSPAC);
		DiaDimItem(dia, DGRD_VERTSPAC);
	} else
	{
		DiaUnDimItem(dia, DGRD_HORIZSPAC);
		DiaUnDimItem(dia, DGRD_VERTSPAC);
		DiaSetText(dia, -DGRD_HORIZSPAC, frtoa(el_curwindowpart->gridx));
		DiaSetText(dia, DGRD_VERTSPAC, frtoa(el_curwindowpart->gridy));
	}
	var = getval((INTBIG)us_tool, VTOOL, VFRACT|VISARRAY, x_("USER_default_grid"));
	if (var == NOVARIABLE) xspacing = yspacing = WHOLE; else
	{
		xspacing = ((INTBIG *)var->addr)[0];
		yspacing = ((INTBIG *)var->addr)[1];
	}
	DiaSetText(dia, DGRD_DEFHORIZSPAC, frtoa(xspacing));
	DiaSetText(dia, DGRD_DEFVERTSPAC, frtoa(yspacing));

	var = getvalkey((INTBIG)us_tool, VTOOL, -1, us_gridboldspacingkey);
	if (var == NOVARIABLE) xboldspacing = yboldspacing = 10; else
	{
		if ((var->type&VISARRAY) == 0)
			xboldspacing = yboldspacing = var->addr; else
		{
			xboldspacing = ((INTBIG *)var->addr)[0];
			yboldspacing = ((INTBIG *)var->addr)[1];
		}
	}
	esnprintf(buf, 20, x_("%ld"), xboldspacing);   DiaSetText(dia, DGRD_BOLDHORIZSPAC, buf);
	esnprintf(buf, 20, x_("%ld"), yboldspacing);   DiaSetText(dia, DGRD_BOLDVERTSPAC, buf);

	oldgridfloats = 0;
	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_gridfloatskey);
	if (var != NOVARIABLE && var->addr != 0) oldgridfloats = 1;
	DiaSetControl(dia, DGRD_ALIGNDOTS, oldgridfloats);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DGRD_ALIGNDOTS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		/* change window-independent grid options */
		defgrid[0] = atofr(DiaGetText(dia, DGRD_DEFHORIZSPAC));
		defgrid[1] = atofr(DiaGetText(dia, DGRD_DEFVERTSPAC));
		if (defgrid[0] != xspacing || defgrid[1] != yspacing)
			(void)setval((INTBIG)us_tool, VTOOL, x_("USER_default_grid"),
				(INTBIG)defgrid, VFRACT|VISARRAY|(2<<VLENGTHSH));
		gridfloats = DiaGetControl(dia, DGRD_ALIGNDOTS);
		bolddots[0] = eatoi(DiaGetText(dia, DGRD_BOLDHORIZSPAC));
		bolddots[1] = eatoi(DiaGetText(dia, DGRD_BOLDVERTSPAC));

		/* see if grid changed in this cell */
		if (el_curwindowpart != NOWINDOWPART && el_curwindowpart->curnodeproto != NONODEPROTO)
		{
			newgridx = atofr(DiaGetText(dia, DGRD_HORIZSPAC));
			newgridy = atofr(DiaGetText(dia, DGRD_VERTSPAC));
			if (newgridx != el_curwindowpart->gridx || newgridy != el_curwindowpart->gridy ||
				bolddots[0] != xboldspacing || bolddots[1] != yboldspacing ||
				gridfloats != oldgridfloats)
			{
				/* adjust grid in current window */
				us_pushhighlight();
				us_clearhighlightcount();
				startobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);

				/* turn grid off if on */
				if ((el_curwindowpart->state&GRIDON) != 0)
					(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("state"),
						el_curwindowpart->state & ~GRIDON, VINTEGER);

				if (newgridx != el_curwindowpart->gridx || newgridy != el_curwindowpart->gridy)
				{
					(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("gridx"), newgridx, VINTEGER);
					(void)setval((INTBIG)el_curwindowpart, VWINDOWPART, x_("gridy"), newgridy, VINTEGER);
				}
				if (bolddots[0] != xboldspacing || bolddots[1] != yboldspacing)
					(void)setvalkey((INTBIG)us_tool, VTOOL, us_gridboldspacingkey,
						(INTBIG)bolddots, VINTEGER|VISARRAY|(2<<VLENGTHSH));
				if (gridfloats != oldgridfloats)
					(void)setvalkey((INTBIG)us_tool, VTOOL, us_gridfloatskey,
						(INTBIG)gridfloats, VINTEGER);

				/* show new grid */
				us_gridset(el_curwindowpart, GRIDON);

				/* restore highlighting */
				endobjectchange((INTBIG)el_curwindowpart, VWINDOWPART);
				us_pophighlight(FALSE);
			}
		} else
		{
			/* no current window, but save grid information if it changed */
			if (bolddots[0] != xboldspacing || bolddots[1] != yboldspacing)
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_gridboldspacingkey,
					(INTBIG)bolddots, VINTEGER|VISARRAY|(2<<VLENGTHSH));
			if (gridfloats != oldgridfloats)
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_gridfloatskey,
					(INTBIG)gridfloats, VINTEGER);
		}

	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** HELP DIALOG ******************************/

/* Help */
static DIALOGITEM us_helpdialogitems[] =
{
 /*  1 */ {0, {288,376,312,440}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,32,24,91}, MESSAGE, N_("Topics:")},
 /*  3 */ {0, {8,192,280,636}, SCROLL, x_("")},
 /*  4 */ {0, {24,8,309,177}, SCROLL, x_("")}
};
static DIALOG us_helpdialog = {{50,75,378,720}, N_("Help"), 0, 4, us_helpdialogitems, 0, 0};

/* special items for the "help" dialog: */
#define DHLP_HELP     3		/* help (scroll) */
#define DHLP_TOPICS   4		/* topics (scroll) */

INTBIG us_helpdlog(CHAR *prompt)
{
	CHAR *filename, *line, buf[256], *platform, *pt;
	INTBIG itemHit, i;
	FILE *in;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* get the help file */
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, prompt);
	line = returninfstr(infstr);
	in = xopen(line, us_filetypehelp, x_(""), &filename);
	if (in == NULL)
	{
		us_abortcommand(_("Cannot find help file: %s"), line);
		return(0);
	}

	/* show the "help" dialog */
	dia = DiaInitDialog(&us_helpdialog);
	if (dia == 0)
	{
		xclose(in);
		return(0);
	}
	DiaInitTextDialog(dia, DHLP_HELP, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCHORIZBAR|SCSMALLFONT|SCFIXEDWIDTH);
	DiaInitTextDialog(dia, DHLP_TOPICS, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCSELKEY|SCREPORT|SCSMALLFONT);

	/* determine the platform that is running */
	var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_machine"));
	if (var == NOVARIABLE) platform = x_(""); else
		platform = (CHAR *)var->addr;

	/* load the topics list */
	for(;;)
	{
		if (xfgets(buf, 256, in)) break;
		if (buf[0] != '!') continue;
		for(pt = buf; *pt != 0; pt++)
			if (*pt == '[') break;
		if (*pt == '[')
		{
			*pt++ = 0;
			if (namesamen(pt, platform, estrlen(platform)) != 0) continue;
		}
		DiaStuffLine(dia, DHLP_TOPICS, &buf[1]);
	}
	DiaSelectLine(dia, DHLP_TOPICS, -1);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DHLP_TOPICS)
		{
			line = DiaGetScrollLine(dia, DHLP_TOPICS, DiaGetCurLine(dia, DHLP_TOPICS));
			xseek(in, 0, 0);
			DiaLoadTextDialog(dia, DHLP_HELP, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
			for(;;)
			{
				if (xfgets(buf, 256, in)) break;
				if (buf[0] != '!') continue;
				for(pt = buf; *pt != 0; pt++)
					if (*pt == '[') break;
				if (*pt == '[')
				{
					*pt++ = 0;
					if (namesamen(pt, platform, estrlen(platform)) != 0) continue;
				}
				if (estrcmp(&buf[1], line) == 0) break;
			}
			for(i=0; ; i++)
			{
				if (xfgets(buf, 256, in)) break;
				if (buf[0] == '!') break;
				if (buf[0] == 0 && i == 0) continue;
				DiaStuffLine(dia, DHLP_HELP, buf);
			}
			DiaSelectLine(dia, DHLP_HELP, -1);
			continue;
		}
	}
	DiaDoneDialog(dia);
	xclose(in);
	return(0);
}

/****************************** ICON STYLE DIALOG ******************************/

/* Icon Options */
static DIALOGITEM us_iconoptdialogitems[] =
{
 /*  1 */ {0, {216,309,240,389}, BUTTON, N_("OK")},
 /*  2 */ {0, {180,308,204,388}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,88}, MESSAGE, N_("Inputs on:")},
 /*  4 */ {0, {8,92,24,192}, POPUP, x_("")},
 /*  5 */ {0, {32,8,48,88}, MESSAGE, N_("Outputs on:")},
 /*  6 */ {0, {32,92,48,192}, POPUP, x_("")},
 /*  7 */ {0, {56,8,72,88}, MESSAGE, N_("Bidir. on:")},
 /*  8 */ {0, {56,92,72,192}, POPUP, x_("")},
 /*  9 */ {0, {8,208,24,288}, MESSAGE, N_("Power on:")},
 /* 10 */ {0, {8,292,24,392}, POPUP, x_("")},
 /* 11 */ {0, {32,208,48,288}, MESSAGE, N_("Ground on:")},
 /* 12 */ {0, {32,292,48,392}, POPUP, x_("")},
 /* 13 */ {0, {56,208,72,288}, MESSAGE, N_("Clock on:")},
 /* 14 */ {0, {56,292,72,392}, POPUP, x_("")},
 /* 15 */ {0, {88,8,104,116}, CHECK, N_("Draw leads")},
 /* 16 */ {0, {144,8,160,160}, MESSAGE, N_("Export location:")},
 /* 17 */ {0, {144,164,160,264}, POPUP, x_("")},
 /* 18 */ {0, {168,8,184,160}, MESSAGE, N_("Export style:")},
 /* 19 */ {0, {168,164,184,264}, POPUP, x_("")},
 /* 20 */ {0, {112,8,128,116}, CHECK, N_("Draw body")},
 /* 21 */ {0, {113,264,129,340}, EDITTEXT, x_("")},
 /* 22 */ {0, {192,8,208,160}, MESSAGE, N_("Export technology:")},
 /* 23 */ {0, {192,164,208,264}, POPUP, x_("")},
 /* 24 */ {0, {252,8,268,184}, MESSAGE, N_("Instance location:")},
 /* 25 */ {0, {252,188,268,288}, POPUP, x_("")},
 /* 26 */ {0, {224,8,240,184}, CHECK, N_("Reverse export order")},
 /* 27 */ {0, {88,148,104,248}, MESSAGE, N_("Lead length:")},
 /* 28 */ {0, {88,264,104,340}, EDITTEXT, x_("")},
 /* 29 */ {0, {112,148,128,248}, MESSAGE, N_("Lead spacing:")}
};
static DIALOG us_iconoptdialog = {{75,75,352,476}, N_("Icon Options"), 0, 29, us_iconoptdialogitems, 0, 0};
/* special items for the "Icon Style" dialog: */
#define DICO_INPUTLOC        4		/* Input location (popup) */
#define DICO_OUTPUTLOC       6		/* Output location (popup) */
#define DICO_BIDIRLOC        8		/* Bidir location (popup) */
#define DICO_POWERLOC       10		/* Power location (popup) */
#define DICO_GROUNDLOC      12		/* Ground location (popup) */
#define DICO_CLOCKLOC       14		/* Clock location (popup) */
#define DICO_DRAWLEADS      15		/* Draw leads (check) */
#define DICO_PORTLOC        17		/* Port Location (popup) */
#define DICO_PORTSTYLE      19		/* Port Style (popup) */
#define DICO_DRAWBODY       20		/* Draw body (check) */
#define DICO_LEADSPACING    21		/* Lead spacing (edit text) */
#define DICO_EXPORTTECH     23		/* Export technology (popup) */
#define DICO_INSTANCELOC    25		/* Instance location (popup) */
#define DICO_REVEXPORTORD   26		/* Reverse order of exports (check) */
#define DICO_LEADLENGTH     28		/* Lead length (edit text) */

INTBIG us_iconstyledlog(void)
{
	INTBIG itemHit, style, origstyle, index, i, leadspacing, leadlength;
	REGISTER VARIABLE *var;
	CHAR *newlang[4], line[50];
	REGISTER void *dia;
	static CHAR *location[] = {N_("Left side"), N_("Right side"), N_("Top side"),
		N_("Bottom side")};
	static CHAR *portlocation[] = {N_("Body"), N_("Lead end"), N_("Lead middle")};
	static CHAR *portstyle[] = {N_("Centered"), N_("Inward"), N_("Outward")};
	static CHAR *porttech[] = {N_("Universal"), N_("Schematic")};
	static CHAR *instlocation[] = {N_("Upper-right"), N_("Upper-left"),
		N_("Lower-right"), N_("Lower-left")};

	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_lead_length"));
	if (var != NOVARIABLE) leadlength = var->addr; else leadlength = ICONLEADDEFLEN;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_lead_spacing"));
	if (var != NOVARIABLE) leadspacing = var->addr; else leadspacing = ICONLEADDEFSEP;
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_icon_style"));
	if (var != NOVARIABLE) style = var->addr; else style = ICONSTYLEDEFAULT;
	origstyle = style;
	dia = DiaInitDialog(&us_iconoptdialog);
	if (dia == 0) return(0);
	esnprintf(line, 50, x_("%ld"), leadlength);
	DiaSetText(dia, DICO_LEADLENGTH, line);
	esnprintf(line, 50, x_("%ld"), leadspacing);
	DiaSetText(dia, DICO_LEADSPACING, line);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(location[i]);
	DiaSetPopup(dia, DICO_INPUTLOC, 4, newlang);
	index = (style & ICONSTYLESIDEIN) >> ICONSTYLESIDEINSH;
	DiaSetPopupEntry(dia, DICO_INPUTLOC, index);
	DiaSetPopup(dia, DICO_OUTPUTLOC, 4, newlang);
	index = (style & ICONSTYLESIDEOUT) >> ICONSTYLESIDEOUTSH;
	DiaSetPopupEntry(dia, DICO_OUTPUTLOC, index);
	DiaSetPopup(dia, DICO_BIDIRLOC, 4, newlang);
	index = (style & ICONSTYLESIDEBIDIR) >> ICONSTYLESIDEBIDIRSH;
	DiaSetPopupEntry(dia, DICO_BIDIRLOC, index);
	DiaSetPopup(dia, DICO_POWERLOC, 4, newlang);
	index = (style & ICONSTYLESIDEPOWER) >> ICONSTYLESIDEPOWERSH;
	DiaSetPopupEntry(dia, DICO_POWERLOC, index);
	DiaSetPopup(dia, DICO_GROUNDLOC, 4, newlang);
	index = (style & ICONSTYLESIDEGROUND) >> ICONSTYLESIDEGROUNDSH;
	DiaSetPopupEntry(dia, DICO_GROUNDLOC, index);
	DiaSetPopup(dia, DICO_CLOCKLOC, 4, newlang);
	index = (style & ICONSTYLESIDECLOCK) >> ICONSTYLESIDECLOCKSH;
	DiaSetPopupEntry(dia, DICO_CLOCKLOC, index);

	for(i=0; i<3; i++) newlang[i] = TRANSLATE(portlocation[i]);
	DiaSetPopup(dia, DICO_PORTLOC, 3, newlang);
	index = (style & ICONSTYLEPORTLOC) >> ICONSTYLEPORTLOCSH;
	DiaSetPopupEntry(dia, DICO_PORTLOC, index);
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(portstyle[i]);
	DiaSetPopup(dia, DICO_PORTSTYLE, 3, newlang);
	index = (style & ICONSTYLEPORTSTYLE) >> ICONSTYLEPORTSTYLESH;
	DiaSetPopupEntry(dia, DICO_PORTSTYLE, index);

	for(i=0; i<2; i++) newlang[i] = TRANSLATE(porttech[i]);
	DiaSetPopup(dia, DICO_EXPORTTECH, 2, newlang);
	index = (style & ICONSTYLETECH) >> ICONSTYLETECHSH;
	DiaSetPopupEntry(dia, DICO_EXPORTTECH, index);

	for(i=0; i<4; i++) newlang[i] = TRANSLATE(instlocation[i]);
	DiaSetPopup(dia, DICO_INSTANCELOC, 4, newlang);
	index = (style & ICONINSTLOC) >> ICONINSTLOCSH;
	DiaSetPopupEntry(dia, DICO_INSTANCELOC, index);

	if ((style&ICONSTYLEDRAWNOLEADS) == 0) DiaSetControl(dia, DICO_DRAWLEADS, 1);
	if ((style&ICONSTYLEDRAWNOBODY) == 0) DiaSetControl(dia, DICO_DRAWBODY, 1);
	if ((style&ICONSTYLEREVEXPORT) != 0) DiaSetControl(dia, DICO_REVEXPORTORD, 1);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DICO_DRAWLEADS || itemHit == DICO_DRAWBODY ||
			itemHit == DICO_REVEXPORTORD)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}
	if (itemHit == OK)
	{
		style = 0;
		index = DiaGetPopupEntry(dia, DICO_INPUTLOC);
		style |= index << ICONSTYLESIDEINSH;
		index = DiaGetPopupEntry(dia, DICO_OUTPUTLOC);
		style |= index << ICONSTYLESIDEOUTSH;
		index = DiaGetPopupEntry(dia, DICO_BIDIRLOC);
		style |= index << ICONSTYLESIDEBIDIRSH;
		index = DiaGetPopupEntry(dia, DICO_POWERLOC);
		style |= index << ICONSTYLESIDEPOWERSH;
		index = DiaGetPopupEntry(dia, DICO_GROUNDLOC);
		style |= index << ICONSTYLESIDEGROUNDSH;
		index = DiaGetPopupEntry(dia, DICO_CLOCKLOC);
		style |= index << ICONSTYLESIDECLOCKSH;
		index = DiaGetPopupEntry(dia, DICO_EXPORTTECH);
		style |= index << ICONSTYLETECHSH;

		index = DiaGetPopupEntry(dia, DICO_PORTLOC);
		style |= index << ICONSTYLEPORTLOCSH;
		index = DiaGetPopupEntry(dia, DICO_PORTSTYLE);
		style |= index << ICONSTYLEPORTSTYLESH;
		index = DiaGetPopupEntry(dia, DICO_INSTANCELOC);
		style |= index << ICONINSTLOCSH;

		if (DiaGetControl(dia, DICO_DRAWLEADS) == 0) style |= ICONSTYLEDRAWNOLEADS;
		if (DiaGetControl(dia, DICO_DRAWBODY) == 0) style |= ICONSTYLEDRAWNOBODY;
		if (DiaGetControl(dia, DICO_REVEXPORTORD) != 0) style |= ICONSTYLEREVEXPORT;
		if (style != origstyle)
			setval((INTBIG)us_tool, VTOOL, x_("USER_icon_style"), style, VINTEGER);

		i = myatoi(DiaGetText(dia, DICO_LEADLENGTH));
		if (i != leadlength)
			setval((INTBIG)us_tool, VTOOL, x_("USER_icon_lead_length"), i, VINTEGER);
		i = myatoi(DiaGetText(dia, DICO_LEADSPACING));
		if (i != leadspacing)
			setval((INTBIG)us_tool, VTOOL, x_("USER_icon_lead_spacing"), i, VINTEGER);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** LANGUAGE INTERPRETER DIALOG ******************************/

/* Language: Java options */
static DIALOGITEM us_javaoptdialogitems[] =
{
 /*  1 */ {0, {88,112,112,192}, BUTTON, N_("OK")},
 /*  2 */ {0, {88,16,112,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,172}, CHECK, N_("Disable compiler")},
 /*  4 */ {0, {32,8,48,172}, CHECK, N_("Disable evaluation")},
 /*  5 */ {0, {56,8,72,172}, CHECK, N_("Enable Jose")}
};
static DIALOG us_javaoptdialog = {{75,75,196,277}, N_("Java Options"), 0, 5, us_javaoptdialogitems, 0, 0};

/* special items for the "Java options" command: */
#define DLJO_DISCOMPILER    3		/* Disable compiler (check) */
#define DLJO_DISEVALUATE    4		/* Disable evaluation (check) */
#define DLJO_USEJOSE        5		/* Use Jose (check) */

INTBIG us_javaoptionsdlog(void)
{
	REGISTER INTBIG itemHit, newflags;
	REGISTER void *dia;

	/* display the java options dialog box */
	dia = DiaInitDialog(&us_javaoptdialog);
	if (dia == 0) return(0);
	if ((us_javaflags&JAVANOCOMPILER) != 0) DiaSetControl(dia, DLJO_DISCOMPILER, 1);
	if ((us_javaflags&JAVANOEVALUATE) != 0) DiaSetControl(dia, DLJO_DISEVALUATE, 1);
	if ((us_javaflags&JAVAUSEJOSE) != 0) DiaSetControl(dia, DLJO_USEJOSE, 1);
#ifndef DBMIRRORTOOL
	DiaDimItem(DLJO_USEJOSE);
#endif

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DLJO_DISCOMPILER || itemHit == DLJO_DISEVALUATE || itemHit == DLJO_USEJOSE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		newflags = 0;
		if (DiaGetControl(dia, DLJO_DISCOMPILER) != 0) newflags |= JAVANOCOMPILER;
		if (DiaGetControl(dia, DLJO_DISEVALUATE) != 0) newflags |= JAVANOEVALUATE;
		if (DiaGetControl(dia, DLJO_USEJOSE) != 0) newflags |= JAVAUSEJOSE;
		if (newflags != us_javaflags)
			setvalkey((INTBIG)us_tool, VTOOL, us_java_flags_key, newflags, VINTEGER);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** LAYER HIGHLIGHT DIALOG ******************************/

/* Layer Highlighting */
static DIALOGITEM us_highlightlayerdialogitems[] =
{
 /*  1 */ {0, {108,184,132,248}, BUTTON, N_("Done")},
 /*  2 */ {0, {12,184,36,248}, BUTTON, N_("None")},
 /*  3 */ {0, {8,8,136,170}, SCROLL, x_("")}
};
static DIALOG us_highlightlayerdialog = {{50,75,195,334}, N_("Layer to Highlight"), 0, 3, us_highlightlayerdialogitems, 0, 0};

/* special items for the "highlight layer" command: */
#define DHGL_NOLAYER    2		/* No layer (button) */
#define DHGL_LAYERLIST  3		/* Layer list (scroll) */

INTBIG us_highlayerlog(void)
{
	REGISTER INTBIG itemHit, i;
	REGISTER INTBIG funct;
	REGISTER CHAR *la;
	CHAR buf[2], *newpar[3];
	REGISTER void *dia;

	if (us_needwindow()) return(0);

	/* display the grid settings dialog box */
	dia = DiaInitDialog(&us_highlightlayerdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DHGL_LAYERLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCREPORT|SCSELMOUSE);
	for(i=0; i<el_curtech->layercount; i++)
	{
		if (el_curtech->layers[i]->bits == LAYERO ||
			el_curtech->layers[i]->bits == LAYERN) continue;
		funct = layerfunction(el_curtech, i);
		if ((funct&LFPSEUDO) != 0) continue;
		DiaStuffLine(dia, DHGL_LAYERLIST, layername(el_curtech, i));
	}
	DiaSelectLine(dia, DHGL_LAYERLIST, -1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DHGL_NOLAYER)
		{
			DiaSelectLine(dia, DHGL_LAYERLIST, -1);
			newpar[0] = x_("default");
			us_color(1, newpar);
			continue;
		}
		if (itemHit == DHGL_LAYERLIST)
		{
			i = DiaGetCurLine(dia, DHGL_LAYERLIST);
			if (i < 0) continue;
			la = DiaGetScrollLine(dia, DHGL_LAYERLIST, i);
			for(i=0; i<el_curtech->layercount; i++)
				if (estrcmp(la, layername(el_curtech, i)) == 0) break;
			if (i >= el_curtech->layercount) continue;
			newpar[0] = x_("default");
			us_color(1, newpar);
			la = us_layerletters(el_curtech, i);
			buf[0] = *la;
			buf[1] = 0;
			newpar[0] = x_("highlight");
			newpar[1] = buf;
			us_color(2, newpar);
			continue;
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** LIBRARY PATH DIALOG ******************************/

/* Library paths */
static DIALOGITEM us_librarypathdialogitems[] =
{
 /*  1 */ {0, {76,312,100,376}, BUTTON, N_("OK")},
 /*  2 */ {0, {76,32,100,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,24,220}, MESSAGE, N_("Location of library files:")},
 /*  4 */ {0, {32,8,64,400}, EDITTEXT, x_("")}
};
static DIALOG us_librarypathdialog = {{50,75,159,485}, N_("Current Library Path"), 0, 4, us_librarypathdialogitems, 0, 0};

/* special items for the "library paths" dialog: */
#define DLBP_LIBLOC     4		/* library file location (stat text) */

INTBIG us_librarypathdlog(void)
{
	INTBIG itemHit;
	CHAR *pt;
	REGISTER void *dia;

	/* display the library paths dialog box */
	dia = DiaInitDialog(&us_librarypathdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DLBP_LIBLOC, el_libdir);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK && DiaValidEntry(dia, DLBP_LIBLOC)) break;
	}

	if (itemHit != CANCEL)
	{
		pt = DiaGetText(dia, DLBP_LIBLOC);
		if (estrcmp(pt, el_libdir) != 0) setlibdir(pt);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** LIBRARY SELECTION DIALOG ******************************/

/* Change Library */
static DIALOGITEM us_chglibrarydialogitems[] =
{
 /*  1 */ {0, {164,220,188,300}, BUTTON, N_("OK")},
 /*  2 */ {0, {116,220,140,300}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,128}, MESSAGE, N_("Current Library:")},
 /*  4 */ {0, {4,132,20,316}, MESSAGE, x_("")},
 /*  5 */ {0, {52,4,196,196}, SCROLL, x_("")},
 /*  6 */ {0, {32,16,48,168}, MESSAGE|INACTIVE, N_("Switch to Library:")}
};
static DIALOG us_chglibrarydialog = {{75,75,280,401}, N_("Set Current Library"), 0, 6, us_chglibrarydialogitems, 0, 0};

/* special items for the "change current library" dialog: */
#define DCHL_CURLIB    4		/* current library (stat text) */
#define DCHL_LIBLIST   5		/* library list (scroll) */

INTBIG us_oldlibrarydlog(void)
{
	REGISTER INTBIG itemHit, i, listlen;
	REGISTER LIBRARY *lib;
	REGISTER CHAR *pt;
	REGISTER void *dia;

	/* display the library selection dialog box */
	dia = DiaInitDialog(&us_chglibrarydialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DCHL_CURLIB, el_curlib->libname);
	DiaInitTextDialog(dia, DCHL_LIBLIST, topoflibs, nextlibs, DiaNullDlogDone, 0,
		SCSELMOUSE|SCDOUBLEQUIT);

	/* select the current library */
	listlen = DiaGetNumScrollLines(dia, DCHL_LIBLIST);
	for(i=0; i<listlen; i++)
	{
		pt = DiaGetScrollLine(dia, DCHL_LIBLIST, i);
		if (namesame(pt, el_curlib->libname) == 0)
		{
			DiaSelectLine(dia, DCHL_LIBLIST, i);
			break;
		}
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	lib = el_curlib;
	if (itemHit != CANCEL)
	{
		pt = DiaGetScrollLine(dia, DCHL_LIBLIST, DiaGetCurLine(dia, DCHL_LIBLIST));
		lib = getlibrary(pt);
	}
	DiaDoneDialog(dia);
	if (lib != NOLIBRARY && lib != el_curlib)
		us_switchtolibrary(lib);
	return(0);
}

/****************************** NODE: CREATE ANNULAR RING DIALOG ******************************/

/* Annular Ring */
static DIALOGITEM us_annringdialogitems[] =
{
 /*  1 */ {0, {268,176,292,240}, BUTTON, N_("OK")},
 /*  2 */ {0, {268,20,292,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {164,12,180,160}, MESSAGE, N_("Inner Radius:")},
 /*  4 */ {0, {164,164,180,244}, EDITTEXT, x_("")},
 /*  5 */ {0, {188,12,204,160}, MESSAGE, N_("Outer Radius:")},
 /*  6 */ {0, {188,164,204,244}, EDITTEXT, x_("")},
 /*  7 */ {0, {212,12,228,160}, MESSAGE, N_("Number of segments:")},
 /*  8 */ {0, {212,164,228,244}, EDITTEXT, x_("32")},
 /*  9 */ {0, {236,12,252,160}, MESSAGE, N_("Number of degrees:")},
 /* 10 */ {0, {236,164,252,244}, EDITTEXT, x_("360")},
 /* 11 */ {0, {8,8,24,172}, MESSAGE, N_("Layer to use for ring:")},
 /* 12 */ {0, {28,8,156,244}, SCROLL, x_("")}
};
static DIALOG us_annringdialog = {{75,75,376,330}, N_("Annulus Construction"), 0, 12, us_annringdialogitems, 0, 0};

/* special items for the "Annular ring" dialog: */
#define DANR_INNERRADIUS  4		/* inner radius (edit text) */
#define DANR_OUTERRADIUS  6		/* outer radius (edit text) */
#define DANR_NUMSEGS      8		/* number of segments (edit text) */
#define DANR_NUMDEGREES  10		/* number of degrees (edit text) */
#define DANR_LAYER       12		/* layer to use (scroll) */

INTBIG us_annularringdlog(void)
{
	INTBIG itemHit, i;
	REGISTER INTBIG lx, hx, ly, hy, cx, cy, layers, fun;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *layer, *parent, *prim;
	CHAR inner[20], outer[20], segs[20], degrees[20], *pars[6];
	static POLYGON *poly = NOPOLYGON;
	HIGHLIGHT newhigh;
	REGISTER void *dia;

	parent = us_needcell();
	if (parent == NONODEPROTO) return(0);

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* count all pure-layer nodes in the current technology */
	layers = 0;
	for(prim = el_curtech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		fun = (prim->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun == NPNODE) layers++;
	}
	if (layers <= 0)
	{
		us_abortcommand(_("This technology has no pure-layer nodes"));
		return(0);
	}

	/* display the window view dialog box */
	dia = DiaInitDialog(&us_annringdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DANR_LAYER, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE);
	for(prim = el_curtech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		fun = (prim->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPNODE) continue;
		DiaStuffLine(dia, DANR_LAYER, prim->protoname);
	}
	DiaSelectLine(dia, DANR_LAYER, 0);
	DiaSetText(dia, DANR_NUMSEGS, x_("32"));
	DiaSetText(dia, DANR_NUMDEGREES, x_("360"));

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	if (itemHit == OK)
	{
		/* figure out which pure-layer node to create */
		i = DiaGetCurLine(dia, DANR_LAYER);
		layer = getnodeproto(DiaGetScrollLine(dia, DANR_LAYER, i));
		if (layer == NONODEPROTO) { DiaDoneDialog(dia);   return(0); }

		/* create the pure-layer node */
		cx = (el_curwindowpart->screenlx + el_curwindowpart->screenhx) / 2;
		cy = (el_curwindowpart->screenly + el_curwindowpart->screenhy) / 2;
		lx = cx - (layer->highx - layer->lowx) / 2;
		hx = lx + layer->highx - layer->lowx;
		ly = cy - (layer->highy - layer->lowy) / 2;
		hy = ly + layer->highy - layer->lowy;
		ni = newnodeinst(layer, lx, hx, ly, hy, 0, 0, parent);
		if (ni == NONODEINST) { DiaDoneDialog(dia);   return(0); }
		endobjectchange((INTBIG)ni, VNODEINST);

		/* highlight the pure-layer node */
		us_clearhighlightcount();
		newhigh.status = HIGHFROM;
		newhigh.cell = parent;
		newhigh.fromgeom = ni->geom;
		newhigh.fromport = NOPORTPROTO;
		newhigh.frompoint = 0;
		us_addhighlight(&newhigh);

		/* turn it into an annular ring */
		estrcpy(inner, DiaGetText(dia, DANR_INNERRADIUS));
		estrcpy(outer, DiaGetText(dia, DANR_OUTERRADIUS));
		estrcpy(segs, DiaGetText(dia, DANR_NUMSEGS));
		estrcpy(degrees, DiaGetText(dia, DANR_NUMDEGREES));
		pars[0] = x_("trace");
		pars[1] = x_("construct-annulus");
		pars[2] = inner;
		pars[3] = outer;
		pars[4] = segs;
		pars[5] = degrees;
		us_node(6, pars);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** NODE: CREATE LAYOUT TEXT DIALOG ******************************/

/* Node: Make Text Layout */
static DIALOGITEM us_spelldialogitems[] =
{
 /*  1 */ {0, {196,192,220,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {196,12,220,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,200,24,248}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,196}, MESSAGE, N_("Size (max 63):")},
 /*  5 */ {0, {164,8,180,72}, MESSAGE, N_("Message:")},
 /*  6 */ {0, {164,76,180,272}, EDITTEXT, x_("")},
 /*  7 */ {0, {136,76,152,272}, POPUP, x_("")},
 /*  8 */ {0, {136,8,152,72}, MESSAGE, N_("Layer:")},
 /*  9 */ {0, {88,76,104,268}, POPUP, x_("")},
 /* 10 */ {0, {88,8,104,72}, MESSAGE, N_("Font:")},
 /* 11 */ {0, {112,8,128,92}, CHECK, N_("Italic")},
 /* 12 */ {0, {112,100,128,168}, CHECK, N_("Bold")},
 /* 13 */ {0, {112,176,128,272}, CHECK, N_("Underline")},
 /* 14 */ {0, {36,200,52,248}, EDITTEXT, x_("")},
 /* 15 */ {0, {36,8,52,196}, MESSAGE, N_("Scale factor:")},
 /* 16 */ {0, {64,200,80,248}, EDITTEXT, x_("")},
 /* 17 */ {0, {64,8,80,196}, MESSAGE, N_("Dot separation (lambda):")}
};
static DIALOG us_spelldialog = {{75,75,304,356}, N_("Create Text Layout"), 0, 17, us_spelldialogitems, 0, 0};

/* special items for the "Layout Text" dialog: */
#define DPLT_TEXTSIZE     3		/* Text size (edit text) */
#define DPLT_TEXT         6		/* Text to place (edit text) */
#define DPLT_LAYER        7		/* Layer (popup) */
#define DPLT_FONT         9		/* Font (popup) */
#define DPLT_FONT_L      10		/* Font label (stat text) */
#define DPLT_ITALIC      11		/* Italic (check) */
#define DPLT_BOLD        12		/* Bold (check) */
#define DPLT_UNDERLINE   13		/* Underline (check) */
#define DPLT_SCALE       14		/* Scale (edit text) */
#define DPLT_SEPARATION  16		/* Dot separation (edit text) */

INTBIG us_placetextdlog(void)
{
	INTBIG itemHit, layers, fun;
	CHAR **layernames, line[20];
	static INTBIG lastprim = 0, lastsize = 12, lastfont = 0, lastscale = 1,
		lastitalic = 0, lastbold = 0, lastseparation = 0, lastunderline = 0;
	REGISTER NODEPROTO *prim;
	REGISTER void *dia;

	/* count all pure-layer nodes in the current technology */
	layers = 0;
	for(prim = el_curtech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		fun = (prim->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun == NPNODE) layers++;
	}
	if (layers <= 0)
	{
		us_abortcommand(_("This technology has no pure-layer nodes"));
		return(0);
	}

	/* display the text-layout dialog box */
	dia = DiaInitDialog(&us_spelldialog);
	if (dia == 0) return(0);
	esnprintf(line, 20, x_("%ld"), lastsize);
	DiaSetText(dia, DPLT_TEXTSIZE, line);
	esnprintf(line, 20, x_("%ld"), lastscale);
	DiaSetText(dia, DPLT_SCALE, line);
	esnprintf(line, 20, x_("%ld"), lastseparation);
	DiaSetText(dia, DPLT_SEPARATION, line);
	layernames = (CHAR **)emalloc(layers * (sizeof (CHAR *)), el_tempcluster);
	if (layernames == 0) return(0);
	layers = 0;
	for(prim = el_curtech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		fun = (prim->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPNODE) continue;
		layernames[layers++] = prim->protoname;
	}
	DiaSetPopup(dia, DPLT_LAYER, layers, layernames);
	if (lastprim < layers) DiaSetPopupEntry(dia, DPLT_LAYER, lastprim);
	if (us_lastplacetextmessage != 0)
		DiaSetText(dia, -DPLT_TEXT, us_lastplacetextmessage);
	if (graphicshas(CANCHOOSEFACES))
	{
		DiaUnDimItem(dia, DPLT_FONT_L);
		us_setpopupface(DPLT_FONT, lastfont, TRUE, dia);
	} else
	{
		DiaDimItem(dia, DPLT_FONT_L);
	}
	if (graphicshas(CANMODIFYFONTS))
	{
		DiaUnDimItem(dia, DPLT_ITALIC);
		DiaUnDimItem(dia, DPLT_BOLD);
		DiaUnDimItem(dia, DPLT_UNDERLINE);
		DiaSetControl(dia, DPLT_ITALIC, lastitalic);
		DiaSetControl(dia, DPLT_BOLD, lastbold);
		DiaSetControl(dia, DPLT_UNDERLINE, lastunderline);
	} else
	{
		DiaDimItem(dia, DPLT_ITALIC);
		DiaDimItem(dia, DPLT_BOLD);
		DiaDimItem(dia, DPLT_UNDERLINE);
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DPLT_ITALIC || itemHit == DPLT_BOLD || itemHit == DPLT_UNDERLINE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}
	lastsize = eatoi(DiaGetText(dia, DPLT_TEXTSIZE));
	lastscale = eatoi(DiaGetText(dia, DPLT_SCALE));
	lastseparation = eatoi(DiaGetText(dia, DPLT_SEPARATION));

	if (graphicshas(CANCHOOSEFACES))
	{
		lastfont = us_getpopupface(DPLT_FONT, dia);
	}
	if (graphicshas(CANMODIFYFONTS))
	{
		lastitalic = DiaGetControl(dia, DPLT_ITALIC);
		lastbold = DiaGetControl(dia, DPLT_BOLD);
		lastunderline = DiaGetControl(dia, DPLT_UNDERLINE);
	}
	lastprim = DiaGetPopupEntry(dia, DPLT_LAYER);
	if (us_lastplacetextmessage != 0) efree((CHAR *)us_lastplacetextmessage);
	(void)allocstring(&us_lastplacetextmessage, DiaGetText(dia, DPLT_TEXT), us_tool->cluster);

	if (itemHit == OK)
	{
		us_layouttext(layernames[lastprim], lastsize, lastscale, lastfont, lastitalic,
			lastbold, lastunderline, lastseparation, us_lastplacetextmessage);
	}
	DiaDoneDialog(dia);
	efree((CHAR *)layernames);
	return(0);
}

/****************************** NODE CREATION OPTIONS DIALOG ******************************/

/* New Node Options */
static DIALOGITEM us_defnodedialogitems[] =
{
 /*  1 */ {0, {384,352,408,416}, BUTTON, N_("OK")},
 /*  2 */ {0, {344,352,368,416}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,208,48,318}, EDITTEXT, x_("")},
 /*  4 */ {0, {32,24,48,205}, MESSAGE, N_("X size of new primitives:")},
 /*  5 */ {0, {56,24,72,205}, MESSAGE, N_("Y size of new primitives:")},
 /*  6 */ {0, {56,208,72,318}, EDITTEXT, x_("")},
 /*  7 */ {0, {8,4,24,142}, MESSAGE, N_("For primitive:")},
 /*  8 */ {0, {8,144,24,354}, POPUP, x_("")},
 /*  9 */ {0, {164,24,180,230}, MESSAGE, N_("Rotation of new nodes:")},
 /* 10 */ {0, {164,240,180,293}, EDITTEXT, x_("")},
 /* 11 */ {0, {164,300,180,405}, CHECK, N_("Transposed")},
 /* 12 */ {0, {132,4,133,422}, DIVIDELINE, x_("")},
 /* 13 */ {0, {140,4,156,152}, MESSAGE, N_("For all nodes:")},
 /* 14 */ {0, {188,24,204,338}, CHECK, N_("Disallow modification of locked primitives")},
 /* 15 */ {0, {212,24,228,338}, CHECK, N_("Move after Duplicate")},
 /* 16 */ {0, {236,24,252,338}, CHECK, N_("Duplicate/Array/Extract copies exports")},
 /* 17 */ {0, {108,40,124,246}, MESSAGE, N_("Rotation of new nodes:")},
 /* 18 */ {0, {108,256,124,309}, EDITTEXT, x_("")},
 /* 19 */ {0, {108,317,124,422}, CHECK, N_("Transposed")},
 /* 20 */ {0, {84,24,100,249}, CHECK, N_("Override default orientation")},
 /* 21 */ {0, {292,268,308,309}, EDITTEXT, x_("")},
 /* 22 */ {0, {337,20,453,309}, SCROLL, x_("")},
 /* 23 */ {0, {316,20,332,309}, MESSAGE, N_("Primitive function abbreviations:")},
 /* 24 */ {0, {460,20,476,165}, MESSAGE, N_("Function:")},
 /* 25 */ {0, {460,168,476,309}, EDITTEXT, x_("")},
 /* 26 */ {0, {260,4,261,422}, DIVIDELINE, x_("")},
 /* 27 */ {0, {268,4,284,185}, MESSAGE, N_("Node naming:")},
 /* 28 */ {0, {292,20,308,265}, MESSAGE, N_("Length of cell abbreviations:")},
 /* 29 */ {0, {261,332,476,333}, DIVIDELINE, x_("")}
};
static DIALOG us_defnodedialog = {{50,75,535,507}, N_("New Node Options"), 0, 29, us_defnodedialogitems, 0, 0};

/* special items for the "new node defaults" dialog: */
#define DDFN_XSIZE            3		/* X size (edit text) */
#define DDFN_YSIZE            6		/* Y size (edit text) */
#define DDFN_PRIMNAME         8		/* Primitive name (popup) */
#define DDFN_ALLROTATION     10		/* all Rotation (edit text) */
#define DDFN_ALLTRANSPOSE    11		/* all Transposed (check) */
#define DDFN_ALLOWPRIMMOD    14		/* Allow prim mod (check) */
#define DDFN_MOVEAFTERDUP    15		/* Move after dup (check) */
#define DDFN_COPYPORTS       16		/* Dup copies ports (check) */
#define DDFN_NODEROTATION_L  17		/* node rotation (stat text) */
#define DDFN_NODEROTATION    18		/* node rotation (edit text) */
#define DDFN_NODETRANSPOSE   19		/* node Transposed (check) */
#define DDFN_OVERRIDEORIENT  20		/* override orientation (check) */
#define DDFN_ABBREVLEN       21		/* Length of abbreviations (edit text) */
#define DDFN_FUNCTLIST       22		/* List of functions (scroll) */
#define DDFN_FUNNAME         24		/* Function name (message) */
#define DDFN_ABBREV          25		/* New abbreviation (edit text) */

typedef struct
{
	INTBIG xsize, ysize;
	INTBIG pangle;
} DEFPRIMINFO;

INTBIG us_defnodedlog(void)
{
	REGISTER INTBIG itemHit, i, j, pangle, thispangle, numprims, reloadprim, value, len;
	INTBIG plx, ply, phx, phy, lx, pxs, pys, nodesize[2];
	REGISTER NODEPROTO *thisprim, *np;
	REGISTER BOOLEAN abbrevchanged;
	REGISTER VARIABLE *var;
	REGISTER CHAR **primnames;
	REGISTER DEFPRIMINFO *dpi;
	REGISTER INTBIG abbrevlen;
	CHAR **shortnames, *name, *pt, num[20];
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the defnode dialog box */
	dia = DiaInitDialog(&us_defnodedialog);
	if (dia == 0) return(0);

	/* construct lists of primitives */
	numprims = 0;
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		numprims++;
	primnames = (CHAR **)emalloc(numprims * (sizeof (CHAR *)), el_tempcluster);
	numprims = 0;
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		primnames[numprims++] = np->protoname;
	DiaSetPopup(dia, DDFN_PRIMNAME, numprims, primnames);
	efree((CHAR *)primnames);

	/* save existing state */
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		dpi = (DEFPRIMINFO *)emalloc(sizeof (DEFPRIMINFO), el_tempcluster);
		if (dpi == 0) return(0);
		np->temp1 = (INTBIG)dpi;
		defaultnodesize(np, &pxs, &pys);
		nodeprotosizeoffset(np, &plx, &ply, &phx, &phy, NONODEPROTO);
		dpi->xsize = pxs - plx - phx;
		dpi->ysize = pys - ply - phy;
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, us_placement_angle_key);
		if (var == NOVARIABLE) dpi->pangle = -1; else
			dpi->pangle = var->addr;
	}

	/* load defaults for primitives */
	thisprim = el_curtech->firstnodeproto;

	/* load defaults for all nodes */
	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_placement_angle_key);
	if (var == NOVARIABLE) pangle = 0; else pangle = var->addr;
	DiaSetText(dia, -DDFN_ALLROTATION, frtoa(pangle%3600*WHOLE/10));
	DiaSetControl(dia, DDFN_ALLTRANSPOSE, (pangle >= 3600 ? 1 : 0));
	DiaSetControl(dia, DDFN_ALLOWPRIMMOD, (us_useroptions&NOPRIMCHANGES) != 0 ? 1 : 0);
	DiaSetControl(dia, DDFN_MOVEAFTERDUP, (us_useroptions&NOMOVEAFTERDUP) == 0 ? 1 : 0);
	DiaSetControl(dia, DDFN_COPYPORTS, (us_useroptions&DUPCOPIESPORTS) != 0 ? 1 : 0);

	/* load node abbreviation information */
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_node_abbrevlen_key);
	if (var == NOVARIABLE) abbrevlen = NETDEFAULTABBREVLEN; else
		abbrevlen = var->addr;
	esnprintf(num, 20, x_("%ld"), abbrevlen);
	DiaSetText(dia, DDFN_ABBREVLEN, num);
	var = getvalkey((INTBIG)net_tool, VTOOL, VSTRING|VISARRAY, net_node_abbrev_key);
	if (var == NOVARIABLE) len = 0; else
		len = getlength(var);
	shortnames = (CHAR **)emalloc(MAXNODEFUNCTION * (sizeof (CHAR *)), net_tool->cluster);
	if (shortnames == 0) return(0);
	for(i=0; i<MAXNODEFUNCTION; i++)
	{
		name = nodefunctionshortname(i);
		if (i < len)
		{
			pt = ((CHAR **)var->addr)[i];
			if (*pt != 0) name = pt;
		}
		(void)allocstring(&shortnames[i], name, net_tool->cluster);
	}
	DiaInitTextDialog(dia, DDFN_FUNCTLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	for(i=0; i<MAXNODEFUNCTION; i++)
	{
		infstr = initinfstr();
		formatinfstr(infstr, x_("%s (%s)"), nodefunctionname(i, NONODEINST), shortnames[i]);
		DiaStuffLine(dia, DDFN_FUNCTLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DDFN_FUNCTLIST, 0);
	DiaSetText(dia, DDFN_FUNNAME, nodefunctionname(0, NONODEINST));
	DiaSetText(dia, DDFN_ABBREV, shortnames[0]);

	/* loop until done */
	reloadprim = 1;
	abbrevchanged = FALSE;
	for(;;)
	{
		if (reloadprim != 0)
		{
			reloadprim = 0;
			dpi = (DEFPRIMINFO *)thisprim->temp1;
			DiaSetPopupEntry(dia, DDFN_PRIMNAME, thisprim->primindex-1);
			DiaSetText(dia, DDFN_XSIZE, latoa(dpi->xsize, 0));
			DiaSetText(dia, DDFN_YSIZE, latoa(dpi->ysize, 0));
			if (dpi->pangle < 0)
			{
				DiaSetControl(dia, DDFN_OVERRIDEORIENT, 0);
				DiaSetText(dia, DDFN_NODEROTATION, x_(""));
				DiaSetControl(dia, DDFN_NODETRANSPOSE, 0);
				DiaDimItem(dia, DDFN_NODEROTATION_L);
				DiaDimItem(dia, DDFN_NODEROTATION);
				DiaDimItem(dia, DDFN_NODETRANSPOSE);
			} else
			{
				DiaSetControl(dia, DDFN_OVERRIDEORIENT, 1);
				DiaUnDimItem(dia, DDFN_NODEROTATION_L);
				DiaUnDimItem(dia, DDFN_NODEROTATION);
				DiaUnDimItem(dia, DDFN_NODETRANSPOSE);
				DiaSetText(dia, DDFN_NODEROTATION, frtoa(dpi->pangle%3600*WHOLE/10));
				DiaSetControl(dia, DDFN_NODETRANSPOSE, (dpi->pangle >= 3600 ? 1 : 0));
			}
		}

		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DDFN_PRIMNAME)
		{
			i = DiaGetPopupEntry(dia, DDFN_PRIMNAME);
			for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (np->primindex == i+1) break;
			thisprim = np;
			reloadprim = 1;
			continue;
		}
		if (itemHit == DDFN_XSIZE || itemHit == DDFN_YSIZE)
		{
			dpi = (DEFPRIMINFO *)thisprim->temp1;
			dpi->xsize = atola(DiaGetText(dia, DDFN_XSIZE), 0);
			dpi->ysize = atola(DiaGetText(dia, DDFN_YSIZE), 0);
			continue;
		}
		if (itemHit == DDFN_NODEROTATION)
		{
			if (DiaGetControl(dia, DDFN_OVERRIDEORIENT) == 0) continue;
			dpi = (DEFPRIMINFO *)thisprim->temp1;
			dpi->pangle = atofr(DiaGetText(dia, DDFN_NODEROTATION)) * 10 / WHOLE;
			if (DiaGetControl(dia, DDFN_NODETRANSPOSE) != 0) dpi->pangle += 3600;
			continue;
		}
		if (itemHit == DDFN_ALLTRANSPOSE || itemHit == DDFN_ALLOWPRIMMOD ||
			itemHit == DDFN_MOVEAFTERDUP || itemHit == DDFN_COPYPORTS ||
			itemHit == DDFN_NODETRANSPOSE || itemHit == DDFN_OVERRIDEORIENT)
		{
			value = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, value);
			if (itemHit == DDFN_OVERRIDEORIENT)
			{
				dpi = (DEFPRIMINFO *)thisprim->temp1;
				if (value != 0) dpi->pangle = 0; else
					dpi->pangle = -1;
				reloadprim = 1;
			}
			if (itemHit == DDFN_NODETRANSPOSE)
			{
				if (DiaGetControl(dia, DDFN_OVERRIDEORIENT) == 0) continue;
				dpi = (DEFPRIMINFO *)thisprim->temp1;
				dpi->pangle = atofr(DiaGetText(dia, DDFN_NODEROTATION)) * 10 / WHOLE;
				if (DiaGetControl(dia, DDFN_NODETRANSPOSE) != 0) dpi->pangle += 3600;
				reloadprim = 1;
			}
			continue;
		}
		if (itemHit == DDFN_FUNCTLIST)
		{
			i = DiaGetCurLine(dia, DDFN_FUNCTLIST);
			DiaSetText(dia, DDFN_FUNNAME, nodefunctionname(i, NONODEINST));
			DiaSetText(dia, DDFN_ABBREV, shortnames[i]);
			continue;
		}
		if (itemHit == DDFN_ABBREV)
		{
			i = DiaGetCurLine(dia, DDFN_FUNCTLIST);
			pt = DiaGetText(dia, DDFN_ABBREV);
			if (namesame(shortnames[i], pt) == 0) continue;
			abbrevchanged = TRUE;
			(void)reallocstring(&shortnames[i], pt, net_tool->cluster);
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s (%s)"), nodefunctionname(i, NONODEINST), shortnames[i]);
			DiaSetScrollLine(dia, DDFN_FUNCTLIST, i, returninfstr(infstr));
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		/* handle primitive size changes */
		for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			dpi = (DEFPRIMINFO *)np->temp1;
			defaultnodesize(np, &pxs, &pys);
			nodeprotosizeoffset(np, &plx, &ply, &phx, &phy, NONODEPROTO);
			if (dpi->xsize != pxs - plx - phx || dpi->ysize != pys - ply - phy)
			{
				nodesize[0] = dpi->xsize+plx+phx;
				nodesize[1] = dpi->ysize+ply+phy;
				nodesize[0] = nodesize[0] * WHOLE / el_curlib->lambda[np->tech->techindex];
				nodesize[1] = nodesize[1] * WHOLE / el_curlib->lambda[np->tech->techindex];
				(void)setvalkey((INTBIG)np, VNODEPROTO, el_node_size_default_key,
					(INTBIG)nodesize, VINTEGER|VISARRAY|(2<<VLENGTHSH));
			}
			var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, us_placement_angle_key);
			if (var == NOVARIABLE) thispangle = -1; else
				thispangle = var->addr;
			if (thispangle != dpi->pangle)
			{
				if (dpi->pangle < 0)
				{
					(void)delvalkey((INTBIG)np, VNODEPROTO, us_placement_angle_key);
				} else
				{
					setvalkey((INTBIG)np, VNODEPROTO, us_placement_angle_key,
						dpi->pangle, VINTEGER);
				}
			}
		}

		/* handle changes to all nodes */
		lx = us_useroptions;
		i = DiaGetControl(dia, DDFN_ALLOWPRIMMOD);
		if (i != 0) lx |= NOPRIMCHANGES; else lx &= ~NOPRIMCHANGES;
		i = DiaGetControl(dia, DDFN_MOVEAFTERDUP);
		if (i == 0) lx |= NOMOVEAFTERDUP; else lx &= ~NOMOVEAFTERDUP;
		i = DiaGetControl(dia, DDFN_COPYPORTS);
		if (i != 0) lx |= DUPCOPIESPORTS; else lx &= ~DUPCOPIESPORTS;
		if (lx != us_useroptions)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, lx, VINTEGER);
		j = (atofr(DiaGetText(dia, DDFN_ALLROTATION))*10/WHOLE) % 3600;
		if (DiaGetControl(dia, DDFN_ALLTRANSPOSE) != 0) j += 3600;
		if (j != pangle)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_placement_angle_key, j, VINTEGER);

		/* set abbreviation info */
		if (abbrevchanged)
		{
			for(i=0; i<MAXNODEFUNCTION; i++)
			{
				name = nodefunctionshortname(i);
				if (estrcmp(name, shortnames[i]) == 0) shortnames[i][0] = 0;
			}
			setvalkey((INTBIG)net_tool, VTOOL, net_node_abbrev_key, (INTBIG)shortnames,
				VSTRING|VISARRAY|(MAXNODEFUNCTION<<VLENGTHSH));
		}
		i = eatoi(DiaGetText(dia, DDFN_ABBREVLEN));
		if (i != abbrevlen)
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_node_abbrevlen_key, i, VINTEGER);
	}
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		efree((CHAR *)np->temp1);
	for(i=0; i<MAXNODEFUNCTION; i++)
		efree((CHAR *)shortnames[i]);
	efree((CHAR *)shortnames);
	DiaDoneDialog(dia);
	return(1);
}

/****************************** NODE INFORMATION DIALOGS ******************************/

/* Resistance */
static DIALOGITEM us_resistancedialogitems[] =
{
 /*  1 */ {0, {40,192,64,256}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,16,64,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,24,24,118}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,128,24,239}, POPUP, x_("")},
 /*  5 */ {0, {40,104,64,168}, BUTTON, N_("More...")}
};
static DIALOG us_resistancedialog = {{50,75,124,345}, N_("Resistance"), 0, 5, us_resistancedialogitems, 0, 0};

/* special items for the "resistance" dialog: */
#define DRES_RESISTANCE   3		/* resistance value (edit text) */
#define DRES_UNITS        4		/* units (popup) */
#define DRES_MORE         5		/* More... (button) */

void us_resistancedlog(GEOM *geom, VARIABLE *var)
{
	INTBIG itemHit, i, dispunits;
	float f;
	CHAR *newlang[4], *initstr, *termstr, *pt;
	UINTBIG des[TEXTDESCRIPTSIZE];
	REGISTER void *dia;

	/* display the resistance dialog box */
	dia = DiaInitDialog(&us_resistancedialog);
	if (dia == 0) return;

	/* set the units popup */
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_resistancenames[i]);
	DiaSetPopup(dia, DRES_UNITS, 4, newlang);
	dispunits = (us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH;
	DiaSetPopupEntry(dia, DRES_UNITS, dispunits);

	/* show the resistance value */
	if (var != NOVARIABLE) TDCOPY(des, var->textdescript); else
	{
		var = getvalkey((INTBIG)geom->entryaddr.ni, VNODEINST, -1, sch_resistancekey);
		TDCLEAR(des);   defaulttextdescript(des, geom);  TDSETOFF(des, 0, 0);
	}
	if (var != NOVARIABLE)
	{
		pt = describesimplevariable(var);
		if (isanumber(pt))
		{
			f = (float)eatof(describesimplevariable(var));
			(void)allocstring(&initstr, displayedunits(f, VTUNITSRES, dispunits), us_tool->cluster);
		} else
		{
			(void)allocstring(&initstr, pt, us_tool->cluster);
		}
	} else
	{
		f = figureunits(x_("100"), VTUNITSRES, dispunits);
		(void)allocstring(&initstr, displayedunits(f, VTUNITSRES, dispunits), us_tool->cluster);
	}
	DiaSetText(dia, DRES_RESISTANCE, initstr);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK || itemHit == DRES_MORE) break;
		if (itemHit == DRES_UNITS)
		{
			f = figureunits(DiaGetText(dia, DRES_RESISTANCE), VTUNITSRES, dispunits);
			dispunits = DiaGetPopupEntry(dia, DRES_UNITS);
			DiaSetText(dia, DRES_RESISTANCE, displayedunits(f, VTUNITSRES, dispunits));
			continue;
		}
	}

	termstr = DiaGetText(dia, DRES_RESISTANCE);
	DiaDoneDialog(dia);
	if (itemHit != CANCEL && namesame(initstr, termstr) != 0)
	{
		if (isanumber(termstr))
		{
			f = figureunits(termstr, VTUNITSRES, dispunits);
			us_setfloatvariablevalue(geom, var->key /*sch_resistancekey*/, var, f);
		} else
		{
			us_setvariablevalue(geom, var->key /*sch_resistancekey*/, termstr, VDISPLAY, des);
		}
	}
	if (itemHit == DRES_MORE)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
}

/* Capacitance */
static DIALOGITEM us_capacitancedialogitems[] =
{
 /*  1 */ {0, {40,176,64,240}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,16,64,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,24,110}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,120,24,246}, POPUP, x_("")},
 /*  5 */ {0, {40,96,64,160}, BUTTON, N_("More...")}
};
static DIALOG us_capacitancedialog = {{50,75,123,330}, N_("Capacitance"), 0, 5, us_capacitancedialogitems, 0, 0};

/* special items for the "capacitance" dialog: */
#define DCAP_CAPACITANCE  3		/* capacitance value (edit text) */
#define DCAP_UNITS        4		/* units (popup) */
#define DCAP_MORE         5		/* More... (button) */

void us_capacitancedlog(GEOM *geom, VARIABLE *var)
{
	INTBIG itemHit, i, dispunits;
	CHAR *newlang[6], *initstr, *termstr, *pt;
	float f;
	UINTBIG des[TEXTDESCRIPTSIZE];
	REGISTER void *dia;

	/* display the capacitance dialog box */
	dia = DiaInitDialog(&us_capacitancedialog);
	if (dia == 0) return;

	/* set the units popup */
	for(i=0; i<6; i++) newlang[i] = TRANSLATE(us_capacitancenames[i]);
	DiaSetPopup(dia, DCAP_UNITS, 6, newlang);
	dispunits = (us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH;
	DiaSetPopupEntry(dia, DCAP_UNITS, dispunits);

	/* show the capacitance value */
	if (var != NOVARIABLE) TDCOPY(des, var->textdescript); else
	{
		var = getvalkey((INTBIG)geom->entryaddr.ni, VNODEINST, -1, sch_capacitancekey);
		TDCLEAR(des);   defaulttextdescript(des, geom);  TDSETOFF(des, 0, 0);
	}
	if (var != NOVARIABLE)
	{
		pt = describesimplevariable(var);
		if (isanumber(pt))
		{
			f = (float)eatof(describesimplevariable(var));
			(void)allocstring(&initstr, displayedunits(f, VTUNITSCAP, dispunits), us_tool->cluster);
		} else
		{
			(void)allocstring(&initstr, pt, us_tool->cluster);
		}
	} else
	{
		f = figureunits(x_("100"), VTUNITSCAP, dispunits);
		(void)allocstring(&initstr, displayedunits(f, VTUNITSCAP, dispunits), us_tool->cluster);
	}
	DiaSetText(dia, DCAP_CAPACITANCE, initstr);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK || itemHit == DCAP_MORE) break;
		if (itemHit == DCAP_UNITS)
		{
			f = figureunits(DiaGetText(dia, DCAP_CAPACITANCE), VTUNITSCAP, dispunits);
			dispunits = DiaGetPopupEntry(dia, DCAP_UNITS);
			DiaSetText(dia, DCAP_CAPACITANCE, displayedunits(f, VTUNITSCAP, dispunits));
			continue;
		}
	}

	termstr = DiaGetText(dia, DCAP_CAPACITANCE);
	DiaDoneDialog(dia);
	if (itemHit != CANCEL && namesame(initstr, termstr) != 0)
	{
		if (isanumber(termstr))
		{
			f = figureunits(termstr, VTUNITSCAP, dispunits);
			us_setfloatvariablevalue(geom, var->key /*sch_capacitancekey*/, var, f);
		} else
		{
			us_setvariablevalue(geom, var->key /*sch_capacitancekey*/, termstr, VDISPLAY, des);
		}
	}
	if (itemHit == DCAP_MORE)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
}

/* Inductance */
static DIALOGITEM us_inductancedialogitems[] =
{
 /*  1 */ {0, {40,168,64,232}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,8,64,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,16,24,110}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,120,24,237}, POPUP, x_("")},
 /*  5 */ {0, {40,88,64,152}, BUTTON, N_("More...")}
};
static DIALOG us_inductancedialog = {{50,75,126,321}, N_("Inductance"), 0, 5, us_inductancedialogitems, 0, 0};

/* special items for the "inductance" dialog: */
#define DIND_INDUCTANCE   3		/* inductance value (edit text) */
#define DIND_UNITS        4		/* units (popup) */
#define DIND_MORE         5		/* More... (button) */

void us_inductancedlog(GEOM *geom, VARIABLE *var)
{
	INTBIG itemHit, i, dispunits;
	CHAR *newlang[3], *initstr, *termstr, *pt;
	float f;
	UINTBIG des[TEXTDESCRIPTSIZE];
	REGISTER void *dia;

	/* display the inductance dialog box */
	dia = DiaInitDialog(&us_inductancedialog);
	if (dia == 0) return;

	/* set the units popup */
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_inductancenames[i]);
	DiaSetPopup(dia, DIND_UNITS, 4, newlang);
	dispunits = (us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH;
	DiaSetPopupEntry(dia, DIND_UNITS, dispunits);

	/* show the inductance value */
	if (var != NOVARIABLE) TDCOPY(des, var->textdescript); else
	{
		var = getvalkey((INTBIG)geom->entryaddr.ni, VNODEINST, -1, sch_inductancekey);
		TDCLEAR(des);   defaulttextdescript(des, geom);  TDSETOFF(des, 0, 0);
	}
	if (var != NOVARIABLE)
	{
		pt = describesimplevariable(var);
		if (isanumber(pt))
		{
			f = (float)eatof(describesimplevariable(var));
			(void)allocstring(&initstr, displayedunits(f, VTUNITSIND, dispunits), us_tool->cluster);
		} else
		{
			(void)allocstring(&initstr, pt, us_tool->cluster);
		}
	} else
	{
		f = figureunits(x_("100"), VTUNITSIND, dispunits);
		(void)allocstring(&initstr, displayedunits(f, VTUNITSIND, dispunits), us_tool->cluster);
	}
	DiaSetText(dia, DIND_INDUCTANCE, initstr);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK || itemHit == DIND_MORE) break;
		if (itemHit == DIND_UNITS)
		{
			f = figureunits(DiaGetText(dia, DIND_INDUCTANCE), VTUNITSIND, dispunits);
			dispunits = DiaGetPopupEntry(dia, DIND_UNITS);
			DiaSetText(dia, DIND_INDUCTANCE, displayedunits(f, VTUNITSIND, dispunits));
			continue;
		}
	}

	termstr = DiaGetText(dia, DIND_INDUCTANCE);
	DiaDoneDialog(dia);
	if (itemHit != CANCEL && namesame(initstr, termstr) != 0)
	{
		if (isanumber(termstr))
		{
			f = figureunits(termstr, VTUNITSIND, dispunits);
			us_setfloatvariablevalue(geom, var->key /*sch_inductancekey*/, var, f);
		} else
		{
			us_setvariablevalue(geom, var->key /*sch_inductancekey*/, termstr, VDISPLAY, des);
		}
	}
	if (itemHit == DIND_MORE)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
}

/* Area */
static DIALOGITEM us_areadialogitems[] =
{
 /*  1 */ {0, {40,184,64,248}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,8,64,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,80,24,174}, EDITTEXT, x_("")},
 /*  4 */ {0, {40,96,64,160}, BUTTON, N_("More...")}
};
static DIALOG us_areadialog = {{50,75,124,333}, N_("Area"), 0, 4, us_areadialogitems, 0, 0};

/* special items for the "area" dialog: */
#define DARE_AREA       3		/* area value (edit text) */
#define DARE_MORE       4		/* More... (button) */

void us_areadlog(NODEINST *ni)
{
	INTBIG itemHit, key, type;
	static CHAR line[80];
	REGISTER CHAR *pt;
	REGISTER VARIABLE *var;
	UINTBIG des[TEXTDESCRIPTSIZE];
	REGISTER void *dia;

	/* display the area dialog box */
	if (ni->proto == sch_diodeprim)
	{
		(void)estrcpy(line, _("Diode area"));
		key = sch_diodekey;
	} else
	{
		(void)estrcpy(line, _("Transistor area"));
		key = el_attrkey_area;
	}
	us_areadialog.movable = line;
	dia = DiaInitDialog(&us_areadialog);
	if (dia == 0) return;

	type = VDISPLAY;
	TDCLEAR(des);   defaulttextdescript(des, ni->geom);  TDSETOFF(des, 0, 0);
	var = getvalkey((INTBIG)ni, VNODEINST, -1, key);
	if (var == NOVARIABLE) pt = x_("10"); else
	{
		type = var->type;
		TDCOPY(des, var->textdescript);
		pt = describesimplevariable(var);
	}
	DiaSetText(dia, DARE_AREA, pt);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK || itemHit == DARE_MORE) break;
	}

	if (itemHit != CANCEL)
	{
		us_setvariablevalue(ni->geom, key, DiaGetText(dia, DARE_AREA), type, des);
	}
	DiaDoneDialog(dia);
	if (itemHit == DARE_MORE)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
}

/* Width/Length */
static DIALOGITEM us_widlendialogitems[] =
{
 /*  1 */ {0, {112,184,136,248}, BUTTON, N_("OK")},
 /*  2 */ {0, {112,8,136,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,92,24,248}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,83}, MESSAGE, N_("Width:")},
 /*  5 */ {0, {60,92,76,248}, EDITTEXT, x_("")},
 /*  6 */ {0, {60,8,76,84}, MESSAGE, N_("Length:")},
 /*  7 */ {0, {112,96,136,160}, BUTTON, N_("More")},
 /*  8 */ {0, {32,92,48,247}, POPUP, x_("")},
 /*  9 */ {0, {84,92,100,247}, POPUP, x_("")}
};
static DIALOG us_widlendialog = {{50,75,195,332}, N_("Transistor Information"), 0, 9, us_widlendialogitems, 0, 0};

/* special items for the "width/length" dialog: */
#define DWAL_WIDTH      3		/* Width value (edit text) */
#define DWAL_LENGTH     5		/* Length value (edit text) */
#define DWAL_LENGTH_L   6		/* Length label (stat text) */
#define DWAL_MORE       7		/* More... (button) */
#define DWAL_WIDLANG    8		/* Width language (popup) */
#define DWAL_LENLANG    9		/* Length language (popup) */

void us_widlendlog(NODEINST *ni)
{
	INTBIG itemHit, widtype, lentype, widlang, lenlang;
	CHAR *widstr, *lenstr, *pt, **languages;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER void *dia;

	/* display the width/length dialog box */
	dia = DiaInitDialog(&us_widlendialog);
	if (dia == 0) return;
	languages = us_languagechoices();
	DiaSetPopup(dia, DWAL_WIDLANG, 4, languages);

	np = ni->proto;
	if (np == mocmos_scalablentransprim || np == mocmos_scalableptransprim)
	{
		DiaDimItem(dia, DWAL_LENGTH);
		DiaDimItem(dia, DWAL_LENLANG);
		DiaDimItem(dia, DWAL_LENGTH_L);
	} else
	{
		DiaUnDimItem(dia, DWAL_LENGTH);
		DiaUnDimItem(dia, DWAL_LENLANG);
		DiaUnDimItem(dia, DWAL_LENGTH_L);
		DiaSetPopup(dia, DWAL_LENLANG, 4, languages);
		lentype = VINTEGER|VDISPLAY;
		var = getvalkeynoeval((INTBIG)ni, VNODEINST, -1, el_attrkey_length);
		if (var == NOVARIABLE) (void)allocstring(&lenstr, x_("2"), us_tool->cluster); else
		{
			lentype = var->type;
			lenlang = lentype & (VCODE1|VCODE2);
			switch (lenlang)
			{
				case 0:     DiaSetPopupEntry(dia, DWAL_LENLANG, 0);   break;
				case VTCL:  DiaSetPopupEntry(dia, DWAL_LENLANG, 1);   break;
				case VLISP: DiaSetPopupEntry(dia, DWAL_LENLANG, 2);   break;
				case VJAVA: DiaSetPopupEntry(dia, DWAL_LENLANG, 3);   break;
			}
			(void)allocstring(&lenstr, describevariable(var, -1, -1), us_tool->cluster);
		}
		DiaSetText(dia, DWAL_LENGTH, lenstr);
	}

	widtype = VINTEGER|VDISPLAY;
	var = getvalkeynoeval((INTBIG)ni, VNODEINST, -1, el_attrkey_width);
	if (var == NOVARIABLE) (void)allocstring(&widstr, x_("2"), us_tool->cluster); else
	{
		widtype = var->type;
		widlang = widtype & (VCODE1|VCODE2);
		switch (widlang)
		{
			case 0:     DiaSetPopupEntry(dia, DWAL_WIDLANG, 0);   break;
			case VTCL:  DiaSetPopupEntry(dia, DWAL_WIDLANG, 1);   break;
			case VLISP: DiaSetPopupEntry(dia, DWAL_WIDLANG, 2);   break;
			case VJAVA: DiaSetPopupEntry(dia, DWAL_WIDLANG, 3);   break;
		}
		(void)allocstring(&widstr, describevariable(var, -1, -1), us_tool->cluster);
	}
	DiaSetText(dia, DWAL_WIDTH, widstr);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK || itemHit == DWAL_MORE) break;
	}

	if (itemHit != CANCEL)
	{
		if (np != mocmos_scalablentransprim && np != mocmos_scalableptransprim)
		{
			pt = DiaGetText(dia, DWAL_LENGTH);
			lentype &= ~(VCODE1|VCODE2);
			switch (DiaGetPopupEntry(dia, DWAL_LENLANG))
			{
				case 1: lentype |= VTCL;    break;
				case 2: lentype |= VLISP;   break;
				case 3: lentype |= VJAVA;   break;
			}
			if (estrcmp(pt, lenstr) != 0 || lenlang != (lentype & (VCODE1|VCODE2)))
			{
				us_setvariablevalue(ni->geom, el_attrkey_length, pt, lentype, 0);
			}
		}

		pt = DiaGetText(dia, DWAL_WIDTH);
		widtype &= ~(VCODE1|VCODE2);
		switch (DiaGetPopupEntry(dia, DWAL_WIDLANG))
		{
			case 1: widtype |= VTCL;    break;
			case 2: widtype |= VLISP;   break;
			case 3: widtype |= VJAVA;   break;
		}
		if (estrcmp(pt, widstr) != 0 || widlang != (widtype & (VCODE1|VCODE2)))
		{
			us_setvariablevalue(ni->geom, el_attrkey_width, pt, widtype, 0);
		}
	}
	DiaDoneDialog(dia);
	if (itemHit == DWAL_MORE)
	{
		us_endchanges(NOWINDOWPART);
		(void)us_showdlog(FALSE);
	}
}

/****************************** NODE SIZE DIALOG ******************************/

/* Node Size */
static DIALOGITEM us_nodesizedialogitems[] =
{
 /*  1 */ {0, {104,132,128,212}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,4,128,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,12,24,92}, MESSAGE|INACTIVE, N_("X Size:")},
 /*  4 */ {0, {36,12,52,92}, MESSAGE|INACTIVE, N_("Y Size:")},
 /*  5 */ {0, {8,100,24,200}, EDITTEXT, x_("")},
 /*  6 */ {0, {36,100,52,200}, EDITTEXT, x_("")},
 /*  7 */ {0, {64,4,96,212}, MESSAGE|INACTIVE, x_("")}
};
static DIALOG us_nodesizedialog = {{75,75,212,297}, N_("Set Node Size"), 0, 7, us_nodesizedialogitems, 0, 0};

/* special items for the "node size" dialog: */
#define DNOS_XSIZE       5		/* X size (edit text) */
#define DNOS_YSIZE       6		/* Y size (edit text) */
#define DNOS_EXTRAINFO   7		/* Extra message (stat text) */

INTBIG us_nodesizedlog(CHAR *paramstart[])
{
	INTBIG itemHit, allmanhattan;
	INTBIG ret;
	REGISTER INTBIG i;
	static CHAR x[20], y[20];
	REGISTER GEOM **list;
	REGISTER NODEINST *ni;
	REGISTER void *dia;

	/* display the node size dialog box */
	dia = DiaInitDialog(&us_nodesizedialog);
	if (dia == 0) return(0);

	/* see if there are nonmanhattan nodes selected */
	allmanhattan = 1;
	list = us_gethighlighted(WANTNODEINST, 0, 0);
	for(i=0; list[i] != NOGEOM; i++)
	{
		ni = list[i]->entryaddr.ni;
		if ((ni->rotation % 900) != 0) allmanhattan = 0;
	}
	if (allmanhattan == 0)
	{
		DiaSetText(dia, DNOS_EXTRAINFO,
			_("Nonmanhattan nodes selected: sizing in unrotated directions"));
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	ret = 0;
	if (itemHit != CANCEL)
	{
		estrcpy(x, DiaGetText(dia, DNOS_XSIZE));
		estrcpy(y, DiaGetText(dia, DNOS_YSIZE));
		paramstart[ret++] = x;
		paramstart[ret++] = y;
		if (allmanhattan != 0) paramstart[ret++] = x_("use-transformation");
	}
	DiaDoneDialog(dia);
	return(ret);
}

/****************************** SAVING OPTIONS WITH LIBRARIES DIALOG ******************************/

/* Saving Options with Libraries */
static DIALOGITEM us_optionsavingdialogitems[] =
{
 /*  1 */ {0, {280,216,304,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {280,12,304,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,4,44,294}, MESSAGE, N_("Marked options are saved with the library,")},
 /*  4 */ {0, {92,4,268,294}, SCROLL, x_("")},
 /*  5 */ {0, {68,4,84,294}, MESSAGE, N_("Click an option to change its mark.")},
 /*  6 */ {0, {44,4,60,294}, MESSAGE, N_("and are restored when the library is read.")},
 /*  7 */ {0, {4,4,20,90}, MESSAGE, N_("For library:")},
 /*  8 */ {0, {4,92,20,294}, MESSAGE, x_("")}
};
static DIALOG us_optionsavingdialog = {{50,75,363,378}, N_("Saving Options with Libraries"), 0, 8, us_optionsavingdialogitems, 0, 0};

/* special items for the "Option Saving" dialog: */
#define DOPS_OPTIONLIST    4		/* List of options (scroll) */
#define DOPS_LIBNAME       8		/* Library name (stat text) */

/* this define should match the one in "dbvars.c" */
#define SAVEDBITWORDS 2

INTBIG us_optionsavingdlog(void)
{
	INTBIG itemHit, i, len, opt, listlen;
	INTBIG bits[SAVEDBITWORDS], savebits[SAVEDBITWORDS], origbits[SAVEDBITWORDS];
	REGISTER VARIABLE *var;
	CHAR *name, *msg;
	REGISTER void *infstr;
	REGISTER void *dia;

	for(i=0; i<SAVEDBITWORDS; i++) savebits[i] = 0;
	var = getval((INTBIG)el_curlib, VLIBRARY, -1, x_("LIB_save_options"));
	if (var != NOVARIABLE)
	{
		if ((var->type&VISARRAY) == 0)
		{
			savebits[0] = var->addr;
		} else
		{
			len = getlength(var);
			for(i=0; i<len; i++) savebits[i] = ((INTBIG *)var->addr)[i];
		}
	}
	for(i=0; i<SAVEDBITWORDS; i++) origbits[i] = savebits[i];

	/* display the visibility dialog box */
	dia = DiaInitDialog(&us_optionsavingdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DOPS_LIBNAME, el_curlib->libname);
	DiaInitTextDialog(dia, DOPS_OPTIONLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCREPORT);
	for(opt=0; ; opt++)
	{
		if (describeoptions(opt, &name, bits)) break;
		infstr = initinfstr();
		for(i=0; i<SAVEDBITWORDS; i++) if ((savebits[i]&bits[i]) != 0) break;
		if (i < SAVEDBITWORDS) addtoinfstr(infstr, '>'); else
			addtoinfstr(infstr, ' ');
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, name);
		DiaStuffLine(dia, DOPS_OPTIONLIST, returninfstr(infstr));
	}
	DiaSelectLine(dia, DOPS_OPTIONLIST, -1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DOPS_OPTIONLIST)
		{
			i = DiaGetCurLine(dia, DOPS_OPTIONLIST);
			msg = DiaGetScrollLine(dia, DOPS_OPTIONLIST, i);
			if (*msg == ' ') *msg = '>'; else *msg = ' ';
			DiaSetScrollLine(dia, DOPS_OPTIONLIST, i, msg);
			DiaSelectLine(dia, DOPS_OPTIONLIST, -1);
			continue;
		}
	}

	if (itemHit == OK)
	{
		for(i=0; i<SAVEDBITWORDS; i++) savebits[i] = 0;
		listlen = DiaGetNumScrollLines(dia, DOPS_OPTIONLIST);
		for(opt=0; opt<listlen; opt++)
		{
			if (describeoptions(opt, &name, bits)) break;
			msg = DiaGetScrollLine(dia, DOPS_OPTIONLIST, opt);
			if (*msg == '>')
			{
				for(i=0; i<SAVEDBITWORDS; i++)
					savebits[i] |= bits[i];
			}
		}
		for(i=0; i<SAVEDBITWORDS; i++) if (origbits[i] != savebits[i]) break;
		if (i < SAVEDBITWORDS)
			(void)setval((INTBIG)el_curlib, VLIBRARY, x_("LIB_save_options"),
				(INTBIG)savebits, VINTEGER|VISARRAY|(SAVEDBITWORDS<<VLENGTHSH));
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** SAVING OPTIONS DIALOGS ******************************/

/* User Interface: Save Options */
static DIALOGITEM us_saveoptsdialogitems[] =
{
 /*  1 */ {0, {172,196,196,276}, BUTTON, N_("Yes")},
 /*  2 */ {0, {172,104,196,184}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {172,12,196,92}, BUTTON, N_("No")},
 /*  4 */ {0, {8,8,24,276}, MESSAGE, N_("These Options have changed.  Save?")},
 /*  5 */ {0, {32,12,164,276}, SCROLL, x_("")}
};
static DIALOG us_saveoptsdialog = {{75,75,280,360}, N_("Save Options?"), 0, 5, us_saveoptsdialogitems, 0, 0};

/* special items for the "save options" command: */
#define DSVO_YES      1		/* Yes (button) */
#define DSVO_CANCEL   2		/* Cancel (button) */
#define DSVO_NO       3		/* No (button) */
#define DSVO_CHANGED  5		/* List of changed options (scroll) */

/*
 * Routine to prompt the user to save options.  Returns true if the program should not
 * exit (because "cancel" was hit).  If "atquit" is TRUE, this is a prompt at exit time.
 */
BOOLEAN us_saveoptdlog(BOOLEAN atquit)
{
	INTBIG itemHit;
	REGISTER void *dia;

	/* display the quit dialog box */
	if (atquit) us_saveoptsdialog.movable = _("Save Options?"); else
		us_saveoptsdialog.movable = _("Also Save Options?");
	dia = DiaInitDialog(&us_saveoptsdialog);
	if (dia == 0) return(FALSE);
	DiaInitTextDialog(dia, DSVO_CHANGED, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCHORIZBAR);

	/* load the changed options */
	explainoptionchanges(DSVO_CHANGED, dia);
	DiaSelectLine(dia, DSVO_CHANGED, -1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DSVO_YES || itemHit == DSVO_CANCEL || itemHit == DSVO_NO) break;
	}

	if (itemHit == DSVO_YES)
		us_saveoptions();
	DiaDoneDialog(dia);
	if (itemHit == DSVO_CANCEL) return(TRUE);
	return(FALSE);
}


/* Options Being Saved */
static DIALOGITEM us_optsavedialogitems[] =
{
 /*  1 */ {0, {240,205,264,285}, BUTTON, N_("OK")},
 /*  2 */ {0, {240,16,264,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,4,208,300}, SCROLL, x_("")},
 /*  4 */ {0, {4,4,20,276}, MESSAGE, N_("These options are being saved:")},
 /*  5 */ {0, {212,4,228,300}, CHECK, N_("Only show options changed this session")}
};
static DIALOG us_optsavedialog = {{75,75,348,385}, N_("Options Being Saved"), 0, 5, us_optsavedialogitems, 0, 0};

/* special items for the "options being saved" command: */
#define DOBS_LIST     3		/* List of options (scroll) */
#define DOBS_CHANGED  5		/* Only show changed options (check) */

INTBIG us_examineoptionsdlog(void)
{
	INTBIG itemHit;
	BOOLEAN onlychanged;
	REGISTER void *dia;

	/* display the examine options dialog box */
	dia = DiaInitDialog(&us_optsavedialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DOBS_LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCHORIZBAR);

	/* load the changed options */
	onlychanged = FALSE;
	explainsavedoptions(DOBS_LIST, onlychanged, dia);
	DiaSelectLine(dia, DOBS_LIST, -1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DOBS_CHANGED)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			onlychanged = !onlychanged;
			DiaLoadTextDialog(dia, DOBS_LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
			explainsavedoptions(DOBS_LIST, onlychanged, dia);
			DiaSelectLine(dia, DOBS_LIST, -1);
			continue;
		}
	}

	DiaDoneDialog(dia);
	return(0);
}

/* Find Options */
static DIALOGITEM us_findoptdialogitems[] =
{
 /*  1 */ {0, {292,8,316,88}, BUTTON, N_("Find")},
 /*  2 */ {0, {24,4,288,372}, SCROLL, x_("")},
 /*  3 */ {0, {4,4,20,84}, MESSAGE, N_("Options:")},
 /*  4 */ {0, {292,292,316,372}, BUTTON, N_("Done")},
 /*  5 */ {0, {296,92,312,288}, EDITTEXT, x_("")}
};
static DIALOG us_findoptdialog = {{75,75,400,457}, N_("Finding Options"), 0, 5, us_findoptdialogitems, 0, 0};

/* special items for the "find options" command: */
#define DFIO_FIND     1		/* Find this option (button) */
#define DFIO_LIST     2		/* List of options (scroll) */
#define DFIO_DONE     4		/* Done (button) */
#define DFIO_SEARCH   5		/* string to search for (edit text) */

INTBIG us_findoptionsdlog(void)
{
	INTBIG itemHit, curline, startline, searchstringlen, i, len;
	REGISTER void *dia;
	REGISTER CHAR *searchstring, *pt;

	/* display the find options dialog box */
	dia = DiaInitDialog(&us_findoptdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DFIO_LIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCHORIZBAR);
	listalloptions(DFIO_LIST, dia);
	curline = 0;
	DiaSelectLine(dia, DFIO_LIST, curline);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DFIO_DONE) break;
		if (itemHit == DFIO_FIND)
		{
			searchstring = DiaGetText(dia, DFIO_SEARCH);
			searchstringlen = estrlen(searchstring);
			startline = curline;
			for(;;)
			{
				curline++;
				if (curline == startline)
				{
					ttybeep(SOUNDBEEP, FALSE);
					break;
				}
				pt = DiaGetScrollLine(dia, DFIO_LIST, curline);
				if (*pt == 0)
				{
					curline = -1;
					continue;
				}

				len = estrlen(pt);
				for(i=0; i<=len-searchstringlen; i++)
					if (namesamen(searchstring, &pt[i], searchstringlen) == 0) break;
				if (i <= len-searchstringlen)
				{
					DiaSelectLine(dia, DFIO_LIST, curline);
					break;
				}
			}
			continue;
		}
	}

	DiaDoneDialog(dia);
	return(0);
}

/****************************** OUTLINE INFO DIALOG ******************************/

/* Outline Info */
static DIALOGITEM us_outlinedialogitems[] =
{
 /*  1 */ {0, {76,208,100,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {20,208,44,272}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,168,192}, SCROLL, x_("")},
 /*  4 */ {0, {184,8,200,28}, MESSAGE, N_("X:")},
 /*  5 */ {0, {184,32,200,104}, EDITTEXT, x_("")},
 /*  6 */ {0, {216,8,232,28}, MESSAGE, N_("Y:")},
 /*  7 */ {0, {216,32,232,104}, EDITTEXT, x_("")},
 /*  8 */ {0, {208,160,232,272}, BUTTON, N_("Duplicate Point")},
 /*  9 */ {0, {176,160,200,272}, BUTTON, N_("Delete Point")},
 /* 10 */ {0, {132,208,156,272}, BUTTON, N_("Apply")}
};
static DIALOG us_outlinedialog = {{50,75,291,356}, N_("Outline Information"), 0, 10, us_outlinedialogitems, 0, 0};

/* special items for the "outline" dialog: */
#define DOLI_POINTS     3		/* Points (scroll) */
#define DOLI_XVALUE     5		/* X (edit text) */
#define DOLI_YVALUE     7		/* Y (edit text) */
#define DOLI_DUPLICATE  8		/* duplicate (button) */
#define DOLI_DELETE     9		/* delete (button) */
#define DOLI_APPLY     10		/* apply (button) */

INTBIG us_tracedlog(void)
{
	INTBIG itemHit, len, i, j, space, lambda;
	BOOLEAN changed;
	INTBIG *pts, *newpts, x, y;
	CHAR lne[256];
	HIGHLIGHT *high;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER void *dia;

	/* make sure there is a highlighted node with outline information */
	high = us_getonehighlight();
	if (high == NOHIGHLIGHT) return(0);
	if ((high->status&HIGHTYPE) != HIGHFROM) return(0);
	if (!high->fromgeom->entryisnode) return(0);
	ni = (NODEINST *)high->fromgeom->entryaddr.ni;
	if ((ni->proto->userbits&HOLDSTRACE) == 0) return(0);
	var = gettrace(ni);
	lambda = lambdaofnode(ni);

	/* display the outline dialog box */
	dia = DiaInitDialog(&us_outlinedialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DOLI_POINTS, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCREPORT);

	/* copy outline data and display it */
	if (var == NOVARIABLE) len = 0; else len = getlength(var) / 2;
	x = (ni->highx + ni->lowx) / 2;
	y = (ni->highy + ni->lowy) / 2;
	space = len+1;
	pts = (INTBIG *)emalloc(space * 2 * SIZEOFINTBIG, el_tempcluster);
	for(i=0; i<len; i++)
	{
		pts[i*2] = ((INTBIG *)var->addr)[i*2] + x;
		pts[i*2+1] = ((INTBIG *)var->addr)[i*2+1] + y;
		(void)esnprintf(lne, 256, x_("%ld: (%s, %s)"), i, latoa(pts[i*2], lambda),
			latoa(pts[i*2+1], lambda));
		DiaStuffLine(dia, DOLI_POINTS, lne);
	}
	if (len == 0) DiaSelectLine(dia, DOLI_POINTS, -1); else
	{
		DiaSelectLine(dia, DOLI_POINTS, 0);
		DiaSetText(dia, -DOLI_XVALUE, latoa(pts[0], lambda));
		DiaSetText(dia, DOLI_YVALUE, latoa(pts[1], lambda));
	}

	/* loop until done */
	changed = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DOLI_POINTS)
		{
			/* load this point */
			i = DiaGetCurLine(dia, DOLI_POINTS);
			if (i < 0 || i >= len) continue;
			DiaSetText(dia, DOLI_XVALUE, latoa(pts[i*2], lambda));
			DiaSetText(dia, DOLI_YVALUE, latoa(pts[i*2+1], lambda));
			continue;
		}
		if (itemHit == DOLI_DUPLICATE)
		{
			/* duplicate point */
			changed = TRUE;
			if (len == 0)
			{
				pts[0] = pts[1] = 0;
				(void)esnprintf(lne, 256, x_("%d: (%s, %s)"), 0, latoa(pts[0], lambda),
					latoa(pts[1], lambda));
				DiaSetScrollLine(dia, DOLI_POINTS, 0, lne);
				len++;
				continue;
			}

			if (len >= space)
			{
				newpts = (INTBIG *)emalloc((len+1) * 2 * SIZEOFINTBIG, el_tempcluster);
				if (newpts == 0) continue;
				for(i=0; i<len; i++)
				{
					newpts[i*2] = pts[i*2];
					newpts[i*2+1] = pts[i*2+1];
				}
				efree((CHAR *)pts);
				pts = newpts;
				space = len + 1;
			}
			i = DiaGetCurLine(dia, DOLI_POINTS);
			for(j=len; j>i; j--)
			{
				pts[j*2] = pts[(j-1)*2];
				pts[j*2+1] = pts[(j-1)*2+1];
				(void)esnprintf(lne, 256, x_("%ld: (%s, %s)"), j, latoa(pts[j*2], lambda),
					latoa(pts[j*2+1], lambda));
				DiaSetScrollLine(dia, DOLI_POINTS, j, lne);
			}
			len++;
			continue;
		}
		if (itemHit == DOLI_DELETE)
		{
			/* delete point */
			changed = TRUE;
			if (len <= 1) continue;
			i = DiaGetCurLine(dia, DOLI_POINTS);
			for(j=i; j<len-1; j++)
			{
				pts[j*2] = pts[(j+1)*2];
				pts[j*2+1] = pts[(j+1)*2+1];
				(void)esnprintf(lne, 256, x_("%ld: (%s, %s)"), j, latoa(pts[j*2], lambda),
					latoa(pts[j*2+1], lambda));
				DiaSetScrollLine(dia, DOLI_POINTS, j, lne);
			}
			DiaSetScrollLine(dia, DOLI_POINTS, len-1, x_(""));
			len--;
			if (i == len) DiaSelectLine(dia, DOLI_POINTS, i-1);
			continue;
		}
		if (itemHit == DOLI_XVALUE || itemHit == DOLI_YVALUE)
		{
			/* change to X or Y coordinates */
			i = DiaGetCurLine(dia, DOLI_POINTS);
			if (i < 0) continue;
			x = atola(DiaGetText(dia, DOLI_XVALUE), lambda);
			y = atola(DiaGetText(dia, DOLI_YVALUE), lambda);
			if (pts[i*2] == x && pts[i*2+1] == y) continue;
			changed = TRUE;
			pts[i*2] = x;   pts[i*2+1] = y;
			(void)esnprintf(lne, 256, x_("%ld: (%s, %s)"), i, latoa(pts[i*2], lambda),
				latoa(pts[i*2+1], lambda));
			DiaSetScrollLine(dia, DOLI_POINTS, i, lne);
			continue;
		}
		if (itemHit == DOLI_APPLY)
		{
			/* apply */
			us_pushhighlight();
			us_clearhighlightcount();
			us_settrace(ni, pts, len);
			us_pophighlight(TRUE);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
			changed = FALSE;
			continue;
		}
	}

	if (itemHit != CANCEL && changed)
	{
		us_pushhighlight();
		us_clearhighlightcount();
		us_settrace(ni, pts, len);
		us_pophighlight(TRUE);
	}
	DiaDoneDialog(dia);
	efree((CHAR *)pts);
	return(0);
}

/****************************** PORT CREATION DIALOG ******************************/

/* New export */
static DIALOGITEM us_portdialogitems[] =
{
 /*  1 */ {0, {108,256,132,328}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,32,132,104}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,116,24,340}, EDITTEXT, x_("")},
 /*  4 */ {0, {32,176,48,340}, POPUP, x_("")},
 /*  5 */ {0, {8,8,24,112}, MESSAGE, N_("Export name:")},
 /*  6 */ {0, {56,8,72,128}, CHECK, N_("Always drawn")},
 /*  7 */ {0, {32,8,48,175}, MESSAGE, N_("Export characteristics:")},
 /*  8 */ {0, {80,8,96,128}, CHECK, N_("Body only")},
 /*  9 */ {0, {56,176,72,340}, MESSAGE, N_("Reference export:")},
 /* 10 */ {0, {80,176,96,340}, EDITTEXT, x_("")}
};
static DIALOG us_portdialog = {{50,75,191,425}, N_("Create Export on Highlighted Node"), 0, 10, us_portdialogitems, 0, 0};

/* special items for the "Create Export" dialog: */
#define DCEX_PORTNAME    3		/* Export name (edit text) */
#define DCEX_PORTTYPE    4		/* Export characteristics (popup) */
#define DCEX_ALWAYSDRAWN 6		/* Always drawn (check) */
#define DCEX_BODYONLY    8		/* Body only (check) */
#define DCEX_REFPORT_L   9		/* Reference export label (stat text) */
#define DCEX_REFPORT    10		/* Reference export (edit text) */

INTBIG us_portdlog(void)
{
	INTBIG itemHit, i, count, bits, mask;
	UINTBIG exportchar;
	REGISTER PORTPROTO *pp;
	static INTBIG lastchar = 0;
	REGISTER NODEINST *ni;
	CHAR *newlang[NUMPORTCHARS], *params[20], *refname, *pt, *portname;
	REGISTER void *dia;

	/* get the selected node */
	ni = (NODEINST *)us_getobject(VNODEINST, FALSE);
	if (ni == NONODEINST) return(0);

	/* disallow port action if lock is on */
	if (us_cantedit(ni->parent, NONODEINST, TRUE)) return(0);

	/* display the port dialog box */
	dia = DiaInitDialog(&us_portdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, -DCEX_PORTNAME, x_(""));
	for(i=0; i<NUMPORTCHARS; i++) newlang[i] = TRANSLATE(us_exportcharnames[i]);
	DiaSetPopup(dia, DCEX_PORTTYPE, NUMPORTCHARS, newlang);
	DiaSetPopupEntry(dia, DCEX_PORTTYPE, lastchar);
	exportchar = us_exportcharlist[lastchar];
	if (exportchar == REFOUTPORT || exportchar == REFINPORT ||
		exportchar == REFBASEPORT)
	{
		DiaUnDimItem(dia, DCEX_REFPORT_L);
		DiaUnDimItem(dia, DCEX_REFPORT);
	} else
	{
		DiaDimItem(dia, DCEX_REFPORT_L);
		DiaDimItem(dia, DCEX_REFPORT);
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK && DiaValidEntry(dia, DCEX_PORTNAME)) break;
		if (itemHit == DCEX_PORTTYPE)
		{
			lastchar = DiaGetPopupEntry(dia, DCEX_PORTTYPE);
			exportchar = us_exportcharlist[lastchar];
			if (exportchar == REFOUTPORT || exportchar == REFINPORT ||
				exportchar == REFBASEPORT)
			{
				DiaUnDimItem(dia, DCEX_REFPORT_L);
				DiaUnDimItem(dia, DCEX_REFPORT);
			} else
			{
				DiaDimItem(dia, DCEX_REFPORT_L);
				DiaDimItem(dia, DCEX_REFPORT);
			}
		}
		if (itemHit == DCEX_ALWAYSDRAWN || itemHit == DCEX_BODYONLY)
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
	}

	if (itemHit != CANCEL)
	{
		count = 0;
		if (lastchar > 0) params[count++] = us_exportintnames[lastchar];
		if (DiaGetControl(dia, DCEX_ALWAYSDRAWN) != 0) params[count++] = x_("always-drawn");
		if (DiaGetControl(dia, DCEX_BODYONLY) != 0) params[count++] = x_("body-only");
		exportchar = us_exportcharlist[lastchar];
		if (exportchar == REFOUTPORT || exportchar == REFINPORT ||
			exportchar == REFBASEPORT)
		{
			refname = DiaGetText(dia, DCEX_REFPORT);
			while (*refname == ' ') refname++;
			if (*refname != 0)
			{
				params[count++] = x_("refname");
				params[count++] = refname;
			}
		} else refname = 0;

		pt = DiaGetText(dia, DCEX_PORTNAME);
		while (*pt == ' ') pt++;
		if (*pt == 0) us_abortcommand(_("Null name")); else
		{
			/* find a unique port name */
			portname = us_uniqueportname(pt, ni->parent);
			if (namesame(portname, pt) != 0)
				ttyputmsg(_("Already a port called %s, calling this %s"), pt, portname);

			/* get the specification details */
			pp = us_portdetails(NOPORTPROTO, count, params, ni, &bits, &mask, FALSE, pt, &refname);
			if (pp != NOPORTPROTO)
			{
				/* save highlighting */
				us_pushhighlight();
				us_clearhighlightcount();

				/* make the port */
				pp = us_makenewportproto(ni->parent, ni, pp, portname, mask, bits, pp->textdescript);
				if (*refname != 0)
					setval((INTBIG)pp, VPORTPROTO, x_("EXPORT_reference_name"), (INTBIG)refname, VSTRING);

				/* restore highlighting */
				us_pophighlight(FALSE);
			}
		}
	}
	DiaDoneDialog(dia);
	return(0);
}
/****************************** PORT DISPLAY DIALOG ******************************/

/* Port Display Options */
static DIALOGITEM us_portdisplaydialogitems[] =
{
 /*  1 */ {0, {148,208,172,272}, BUTTON, N_("OK")},
 /*  2 */ {0, {148,20,172,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {36,8,52,123}, RADIO, N_("Full Names")},
 /*  4 */ {0, {60,8,76,123}, RADIO, N_("Short Names")},
 /*  5 */ {0, {84,8,100,123}, RADIO, N_("Crosses")},
 /*  6 */ {0, {8,8,24,151}, MESSAGE, N_("Ports (on instances):")},
 /*  7 */ {0, {36,156,52,271}, RADIO, N_("Full Names")},
 /*  8 */ {0, {60,156,76,271}, RADIO, N_("Short Names")},
 /*  9 */ {0, {84,156,100,271}, RADIO, N_("Crosses")},
 /* 10 */ {0, {8,156,24,295}, MESSAGE, N_("Exports (in cells):")},
 /* 11 */ {0, {116,8,132,280}, CHECK, N_("Move node with export name")},
 /* 12 */ {0, {108,8,109,292}, DIVIDELINE, x_("")}
};
static DIALOG us_portdisplaydialog = {{133,131,314,435}, N_("Port and Export Options"), 0, 12, us_portdisplaydialogitems, 0, 0};

/* special items for the "Port Display" dialog: */
#define DPDP_PFULLNAMES    3		/* Ports: Full Names (radio) */
#define DPDP_PSHORTNAMES   4		/* Ports: Short Names (radio) */
#define DPDP_PCROSSNAMES   5		/* Ports: Crosses (radio) */
#define DPDP_EFULLNAMES    7		/* Export: Full Names (radio) */
#define DPDP_ESHORTNAMES   8		/* Export: Short Names (radio) */
#define DPDP_ECROSSNAMES   9		/* Export: Crosses (radio) */
#define DPDP_MOVENODE     11		/* Move node with port (check) */

INTBIG us_portdisplaydlog(void)
{
	REGISTER INTBIG itemHit, labels, newoptions;
	REGISTER void *dia;

	/* display the port labels dialog */
	if (us_needwindow()) return(0);
	dia = DiaInitDialog(&us_portdisplaydialog);
	if (dia == 0) return(0);
	labels = us_useroptions & (PORTLABELS|EXPORTLABELS);
	switch (labels&PORTLABELS)
	{
		case PORTSFULL:  DiaSetControl(dia, DPDP_PFULLNAMES, 1);   break;
		case PORTSSHORT: DiaSetControl(dia, DPDP_PSHORTNAMES, 1);  break;
		case PORTSCROSS: DiaSetControl(dia, DPDP_PCROSSNAMES, 1);  break;
	}
	switch (labels&EXPORTLABELS)
	{
		case EXPORTSFULL:  DiaSetControl(dia, DPDP_EFULLNAMES, 1);   break;
		case EXPORTSSHORT: DiaSetControl(dia, DPDP_ESHORTNAMES, 1);  break;
		case EXPORTSCROSS: DiaSetControl(dia, DPDP_ECROSSNAMES, 1);  break;
	}
	if ((us_useroptions&MOVENODEWITHEXPORT) != 0)
		DiaSetControl(dia, DPDP_MOVENODE, 1);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DPDP_MOVENODE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DPDP_PFULLNAMES || itemHit == DPDP_PSHORTNAMES ||
			itemHit == DPDP_PCROSSNAMES)
		{
			DiaSetControl(dia, DPDP_PFULLNAMES, 0);
			DiaSetControl(dia, DPDP_PSHORTNAMES, 0);
			DiaSetControl(dia, DPDP_PCROSSNAMES, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DPDP_EFULLNAMES || itemHit == DPDP_ESHORTNAMES ||
			itemHit == DPDP_ECROSSNAMES)
		{
			DiaSetControl(dia, DPDP_EFULLNAMES, 0);
			DiaSetControl(dia, DPDP_ESHORTNAMES, 0);
			DiaSetControl(dia, DPDP_ECROSSNAMES, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
	}
	newoptions = us_useroptions & ~(PORTLABELS | EXPORTLABELS | MOVENODEWITHEXPORT);
	if (DiaGetControl(dia, DPDP_PFULLNAMES) != 0) newoptions |= PORTSFULL; else
		if (DiaGetControl(dia, DPDP_PSHORTNAMES) != 0) newoptions |= PORTSSHORT; else
			newoptions |= PORTSCROSS;
	if (DiaGetControl(dia, DPDP_EFULLNAMES) != 0) newoptions |= EXPORTSFULL; else
		if (DiaGetControl(dia, DPDP_ESHORTNAMES) != 0) newoptions |= EXPORTSSHORT; else
			newoptions |= EXPORTSCROSS;
	if (DiaGetControl(dia, DPDP_MOVENODE) != 0) newoptions |= MOVENODEWITHEXPORT;
	DiaDoneDialog(dia);
	if (itemHit == OK)
	{
		if (us_useroptions != newoptions)
		{
			startobjectchange((INTBIG)us_tool, VTOOL);
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, newoptions, VINTEGER);
			endobjectchange((INTBIG)us_tool, VTOOL);
		}
	}
	return(0);
}

/****************************** PRINT OPTIONS DIALOG ******************************/

/* Printing Options */
static DIALOGITEM us_printingoptdialogitems[] =
{
 /*  1 */ {0, {44,400,68,458}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,400,32,458}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,204}, RADIO, N_("Plot only Highlighted Area")},
 /*  4 */ {0, {8,8,24,142}, RADIO, N_("Plot Entire Cell")},
 /*  5 */ {0, {108,104,124,220}, CHECK, N_("Encapsulated")},
 /*  6 */ {0, {8,228,24,383}, CHECK, N_("Plot Date In Corner")},
 /*  7 */ {0, {308,108,324,166}, RADIO, N_("HPGL")},
 /*  8 */ {0, {308,180,324,254}, RADIO, N_("HPGL/2")},
 /*  9 */ {0, {332,20,348,187}, RADIO, N_("HPGL/2 plot fills page")},
 /* 10 */ {0, {356,20,372,177}, RADIO, N_("HPGL/2 plot fixed at:")},
 /* 11 */ {0, {356,180,372,240}, EDITTEXT, x_("")},
 /* 12 */ {0, {356,244,372,406}, MESSAGE, N_("internal units per pixel")},
 /* 13 */ {0, {260,20,276,172}, CHECK, N_("Synchronize to file:")},
 /* 14 */ {0, {260,172,292,464}, MESSAGE, x_("")},
 /* 15 */ {0, {236,20,252,108}, MESSAGE, N_("EPS Scale:")},
 /* 16 */ {0, {236,124,252,164}, EDITTEXT, x_("2")},
 /* 17 */ {0, {212,8,228,80}, MESSAGE, N_("For cell:")},
 /* 18 */ {0, {212,80,228,464}, MESSAGE, x_("")},
 /* 19 */ {0, {300,8,301,464}, DIVIDELINE, x_("")},
 /* 20 */ {0, {108,8,124,96}, MESSAGE, N_("PostScript:")},
 /* 21 */ {0, {108,312,124,464}, POPUP, x_("")},
 /* 22 */ {0, {100,8,101,464}, DIVIDELINE, x_("")},
 /* 23 */ {0, {308,8,324,98}, MESSAGE, N_("HPGL Level:")},
 /* 24 */ {0, {132,20,148,90}, RADIO, N_("Printer")},
 /* 25 */ {0, {132,100,148,170}, RADIO, N_("Plotter")},
 /* 26 */ {0, {156,20,172,100}, MESSAGE, N_("Width (in):")},
 /* 27 */ {0, {156,104,172,144}, EDITTEXT, x_("2")},
 /* 28 */ {0, {56,228,72,384}, POPUP, x_("")},
 /* 29 */ {0, {32,228,48,384}, MESSAGE, N_("Default printer:")},
 /* 30 */ {0, {156,172,172,260}, MESSAGE, N_("Height (in):")},
 /* 31 */ {0, {156,264,172,304}, EDITTEXT, x_("2")},
 /* 32 */ {0, {180,20,196,256}, POPUP, x_("")},
 /* 33 */ {0, {156,332,172,420}, MESSAGE, N_("Margin (in):")},
 /* 34 */ {0, {156,424,172,464}, EDITTEXT, x_("2")},
 /* 35 */ {0, {80,8,96,280}, MESSAGE, N_("Print and Copy resolution factor:")},
 /* 36 */ {0, {56,8,72,204}, RADIO, N_("Plot only Displayed Window")},
 /* 37 */ {0, {80,284,96,338}, EDITTEXT, x_("")}
};
static DIALOG us_printingoptdialog = {{50,75,431,549}, N_("Printing Options"), 0, 37, us_printingoptdialogitems, 0, 0};

/* special items for the "print options" dialog: */
#define DPRO_SHOWHIGHLIGHT    3		/* focus on highlighted (radio) */
#define DPRO_SHOWALL          4		/* plot entire cell (radio) */
#define DPRO_EPS              5		/* encapsulated postscript (check) */
#define DPRO_PRINTDATE        6		/* date in corner (check) */
#define DPRO_HPGL             7		/* HPGL (radio) */
#define DPRO_HPGL2            8		/* HPGL/2 (radio) */
#define DPRO_HPGL2FILLPAGE    9		/* HPGL/2 plot fills page (radio) */
#define DPRO_HPGL2FIXSCALE   10		/* HPGL/2 plot fixed at (radio) */
#define DPRO_HPGL2SCALE      11		/* HPGL/2 plot scale (edit text) */
#define DPRO_HPGL2FIXSCALE_L 12		/* HPGL/2 plot fixed at (label) (stat text) */
#define DPRO_SYNCHRONIZE     13		/* synchronize to file (check) */
#define DPRO_SYNCHFILE       14		/* file to synchronize (stat text) */
#define DPRO_EPSSCALE_L      15		/* EPS scale label (stat text) */
#define DPRO_EPSSCALE        16		/* EPS scale (edit text) */
#define DPRO_CELL_L          17		/* Cell label (stat text) */
#define DPRO_CELL            18		/* Cell (stat text) */
#define DPRO_PSSTYLE         21		/* PostScript style (popup) */
#define DPRO_PRINTER         24		/* Printer (radio) */
#define DPRO_PLOTTER         25		/* Plotter (radio) */
#define DPRO_PRINTWID_L      26		/* Printer width label (stat text) */
#define DPRO_PRINTWID        27		/* Printer width (edit text) */
#define DPRO_PRINTERS        28		/* printer list (popup) */
#define DPRO_PRINTHEI_L      30		/* Printer height label (stat text) */
#define DPRO_PRINTHEI        31		/* Printer height (edit text) */
#define DPRO_ROTATE          32		/* Rotate output (popup) */
#define DPRO_PRINTMARGIN     34		/* Print margin (edit text) */
#define DPRO_EXTRARES_L      35		/* label for extra resolution (stat text) */
#define DPRO_SHOWWINDOW      36		/* plot displayed window (radio) */
#define DPRO_EXTRARES        37		/* extra resolution when printing/copying (edit text) */

INTBIG us_plotoptionsdlog(void)
{
	REGISTER INTBIG itemHit, scale, i, *curstate, wid, hei, margin, printercount,
		oldplease, epsscale, resfactor;
	INTBIG oldstate[NUMIOSTATEBITWORDS];
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pt;
	CHAR buf[50], *subparams[3], *newlang[3], **printerlist, *defprinter;
	static CHAR *postscripttype[4] = {N_("Black&White"), N_("Color"), N_("Color Stippled"), N_("Color Merged")};
	static CHAR *rotatetype[3] = {N_("No Rotation"), N_("Rotate plot 90 degrees"),
		N_("Auto-rotate plot to fit")};
	REGISTER void *infstr;
	REGISTER void *dia;

	curstate = io_getstatebits();
	for(i=0; i<NUMIOSTATEBITWORDS; i++) oldstate[i] = curstate[i];

	/* display the print options dialog box */
	dia = DiaInitDialog(&us_printingoptdialog);
	if (dia == 0) return(0);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(postscripttype[i]);
	DiaSetPopup(dia, DPRO_PSSTYLE, 4, newlang);
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(rotatetype[i]);
	DiaSetPopup(dia, DPRO_ROTATE, 3, newlang);

	/* show printers and default */
	printerlist = eprinterlist();
	for(printercount=0; printerlist[printercount] != 0; printercount++) ;
	if (printercount == 0)
	{
		newlang[0] = _("<<Cannot set>>");
		DiaSetPopup(dia, DPRO_PRINTERS, 1, newlang);
	} else
	{
		DiaSetPopup(dia, DPRO_PRINTERS, printercount, printerlist);
		defprinter = 0;
		var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_default_printer"));
		if (var != NOVARIABLE) defprinter = (CHAR *)var->addr; else
		{
			CHAR *printer = egetenv(x_("PRINTER"));
			if (printer != 0 && *printer != 0) defprinter = printer;
		}
		if (defprinter != 0)
		{
			for(i=0; i<printercount; i++)
				if (namesame(defprinter, printerlist[i]) == 0) break;
			if (i < printercount) DiaSetPopupEntry(dia, DPRO_PRINTERS, i);
		}
	}

	/* show state */
	switch (curstate[0]&(PSCOLOR1|PSCOLOR2))
	{
		case 0:                 DiaSetPopupEntry(dia, DPRO_PSSTYLE, 0);   break;
		case PSCOLOR1:          DiaSetPopupEntry(dia, DPRO_PSSTYLE, 1);   break;
		case PSCOLOR1|PSCOLOR2: DiaSetPopupEntry(dia, DPRO_PSSTYLE, 2);   break;
		case PSCOLOR2:          DiaSetPopupEntry(dia, DPRO_PSSTYLE, 3);   break;
	}
	if ((curstate[0]&PLOTDATES) != 0) DiaSetControl(dia, DPRO_PRINTDATE, 1);
	if ((curstate[0]&PLOTFOCUS) == 0) DiaSetControl(dia, DPRO_SHOWALL, 1); else
	{
		if ((curstate[0]&PLOTFOCUSDPY) != 0) DiaSetControl(dia, DPRO_SHOWWINDOW, 1); else
			DiaSetControl(dia, DPRO_SHOWHIGHLIGHT, 1);
	}		
	if ((curstate[0]&EPSPSCRIPT) != 0) DiaSetControl(dia, DPRO_EPS, 1);
	if ((curstate[1]&PSAUTOROTATE) != 0) DiaSetPopupEntry(dia, DPRO_ROTATE, 2); else
		if ((curstate[0]&PSROTATE) != 0) DiaSetPopupEntry(dia, DPRO_ROTATE, 1);
	if ((curstate[0]&PSPLOTTER) != 0)
	{
		DiaSetControl(dia, DPRO_PLOTTER, 1);
		DiaDimItem(dia, DPRO_PRINTHEI_L);
		DiaDimItem(dia, DPRO_PRINTHEI);
	} else
	{
		DiaSetControl(dia, DPRO_PRINTER, 1);
		DiaUnDimItem(dia, DPRO_PRINTHEI_L);
		DiaUnDimItem(dia, DPRO_PRINTHEI);
	}

	/* show PostScript page sizes */
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_width"));
	if (var == NOVARIABLE) wid = muldiv(DEFAULTPSWIDTH, WHOLE, 75); else
		wid = var->addr;
	DiaSetText(dia, DPRO_PRINTWID, frtoa(wid));
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_height"));
	if (var == NOVARIABLE) hei = muldiv(DEFAULTPSHEIGHT, WHOLE, 75); else
		hei = var->addr;
	DiaSetText(dia, DPRO_PRINTHEI, frtoa(hei));
	var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
	if (var == NOVARIABLE) margin = muldiv(DEFAULTPSMARGIN, WHOLE, 75); else
		margin = var->addr;
	DiaSetText(dia, DPRO_PRINTMARGIN, frtoa(margin));

	/* show printer resolution scale factor */
	var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_print_resolution_scale"));
	if (var == NOVARIABLE) resfactor = 1; else
		resfactor = var->addr;
	esnprintf(buf, 50, x_("%ld"), resfactor);
	DiaSetText(dia, DPRO_EXTRARES, buf);
#ifndef WIN32
	DiaDimItem(dia, DPRO_EXTRARES_L);
	DiaDimItem(dia, DPRO_EXTRARES);
#endif

	np = el_curlib->curnodeproto;
	DiaDimItem(dia, DPRO_EPSSCALE_L);
	DiaDimItem(dia, DPRO_EPSSCALE);
	epsscale = 1;
	if (np == NONODEPROTO)
	{
		DiaDimItem(dia, DPRO_SYNCHRONIZE);
		DiaDimItem(dia, DPRO_CELL_L);
	} else
	{
		DiaSetText(dia, DPRO_CELL, describenodeproto(np));
		DiaUnDimItem(dia, DPRO_CELL_L);
		if ((curstate[0]&EPSPSCRIPT) != 0)
		{
			DiaUnDimItem(dia, DPRO_EPSSCALE_L);
			DiaUnDimItem(dia, DPRO_EPSSCALE);
			var = getvalkey((INTBIG)np, VNODEPROTO, VFRACT, io_postscriptepsscalekey);
			if (var != NOVARIABLE && var->addr != 0)
			{
				epsscale = var->addr;
			}
			DiaSetText(dia, DPRO_EPSSCALE, frtoa(epsscale));
		}
		DiaUnDimItem(dia, DPRO_SYNCHRONIZE);
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, io_postscriptfilenamekey);
		if (var != NOVARIABLE)
		{
			DiaSetControl(dia, DPRO_SYNCHRONIZE, 1);
			DiaSetText(dia, DPRO_SYNCHFILE, (CHAR *)var->addr);
		}
	}
	if ((curstate[0]&HPGL2) != 0)
	{
		DiaSetControl(dia, DPRO_HPGL2, 1);
		DiaUnDimItem(dia, DPRO_HPGL2FILLPAGE);
		DiaUnDimItem(dia, DPRO_HPGL2FIXSCALE);
		DiaUnDimItem(dia, DPRO_HPGL2FIXSCALE_L);
		DiaEditControl(dia, DPRO_HPGL2SCALE);
		var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_hpgl2_scale"));
		if (var == NOVARIABLE)
		{
			DiaSetControl(dia, DPRO_HPGL2FILLPAGE, 1);
			DiaSetText(dia, DPRO_HPGL2SCALE, x_("20"));
		} else
		{
			DiaSetControl(dia, DPRO_HPGL2FIXSCALE, 1);
			(void)esnprintf(buf, 50, x_("%ld"), var->addr);
			DiaSetText(dia, DPRO_HPGL2SCALE, buf);
		}
	} else
	{
		DiaSetControl(dia, DPRO_HPGL, 1);
		DiaSetControl(dia, DPRO_HPGL2FILLPAGE, 1);
		DiaSetText(dia, DPRO_HPGL2SCALE, x_("20"));
		DiaDimItem(dia, DPRO_HPGL2FILLPAGE);
		DiaDimItem(dia, DPRO_HPGL2FIXSCALE);
		DiaDimItem(dia, DPRO_HPGL2FIXSCALE_L);
		DiaNoEditControl(dia, DPRO_HPGL2SCALE);
	}

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DPRO_EPS || itemHit == DPRO_PRINTDATE ||
			itemHit == DPRO_SYNCHRONIZE)
		{
			i = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			if (itemHit == DPRO_EPS)
			{
				if (i == 0)
				{
					DiaDimItem(dia, DPRO_EPSSCALE_L);
					DiaDimItem(dia, DPRO_EPSSCALE);
				} else
				{
					DiaUnDimItem(dia, DPRO_EPSSCALE_L);
					DiaUnDimItem(dia, DPRO_EPSSCALE);
				}
			}
			if (itemHit == DPRO_SYNCHRONIZE)
			{
				if (i != 0)
				{
					/* select a file name to synchronize with this cell */
					oldplease = el_pleasestop;
					infstr = initinfstr();
					addstringtoinfstr(infstr, x_("ps/"));
					addstringtoinfstr(infstr, _("PostScript File: "));
					i = ttygetparam(returninfstr(infstr), &us_colorwritep, 1, subparams);
					el_pleasestop = oldplease;
					if (i != 0 && subparams[0][0] != 0)
					{
						DiaUnDimItem(dia, DPRO_SYNCHFILE);
						DiaSetText(dia, DPRO_SYNCHFILE, subparams[0]);
					} else
					{
						DiaSetControl(dia, DPRO_SYNCHRONIZE, 0);
					}
				} else
				{
					DiaDimItem(dia, DPRO_SYNCHFILE);
				}
			}
			continue;
		}
		if (itemHit == DPRO_SHOWHIGHLIGHT || itemHit == DPRO_SHOWALL ||
			itemHit == DPRO_SHOWWINDOW)
		{
			DiaSetControl(dia, DPRO_SHOWHIGHLIGHT, 0);
			DiaSetControl(dia, DPRO_SHOWALL, 0);
			DiaSetControl(dia, DPRO_SHOWWINDOW, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DPRO_HPGL)
		{
			DiaSetControl(dia, DPRO_HPGL, 1);
			DiaSetControl(dia, DPRO_HPGL2, 0);
			DiaDimItem(dia, DPRO_HPGL2FILLPAGE);
			DiaDimItem(dia, DPRO_HPGL2FIXSCALE);
			DiaDimItem(dia, DPRO_HPGL2FIXSCALE_L);
			DiaNoEditControl(dia, DPRO_HPGL2SCALE);
			continue;
		}
		if (itemHit == DPRO_HPGL2)
		{
			DiaSetControl(dia, DPRO_HPGL, 0);
			DiaSetControl(dia, DPRO_HPGL2, 1);
			DiaUnDimItem(dia, DPRO_HPGL2FILLPAGE);
			DiaUnDimItem(dia, DPRO_HPGL2FIXSCALE);
			DiaUnDimItem(dia, DPRO_HPGL2FIXSCALE_L);
			DiaEditControl(dia, DPRO_HPGL2SCALE);
			continue;
		}
		if (itemHit == DPRO_HPGL2FILLPAGE || itemHit == DPRO_HPGL2FIXSCALE)
		{
			DiaSetControl(dia, DPRO_HPGL2FILLPAGE, 0);
			DiaSetControl(dia, DPRO_HPGL2FIXSCALE, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DPRO_PRINTER || itemHit == DPRO_PLOTTER)
		{
			DiaSetControl(dia, DPRO_PRINTER, 0);
			DiaSetControl(dia, DPRO_PLOTTER, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DPRO_PLOTTER)
			{
				DiaDimItem(dia, DPRO_PRINTHEI_L);
				DiaDimItem(dia, DPRO_PRINTHEI);
			} else
			{
				DiaUnDimItem(dia, DPRO_PRINTHEI_L);
				DiaUnDimItem(dia, DPRO_PRINTHEI);
			}
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		i = DiaGetPopupEntry(dia, DPRO_PRINTERS);
		var = getval((INTBIG)io_tool, VTOOL, VSTRING, x_("IO_default_printer"));
		if (i < printercount)
		{
			if (var == NOVARIABLE || estrcmp(printerlist[i], (CHAR *)var->addr) != 0)
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_default_printer"),
					(INTBIG)printerlist[i], VSTRING);
		}

		if (DiaGetControl(dia, DPRO_SHOWHIGHLIGHT) != 0)
			curstate[0] = (curstate[0] & ~PLOTFOCUSDPY) | PLOTFOCUS; else
		{
			if (DiaGetControl(dia, DPRO_SHOWALL) != 0) curstate[0] &= ~(PLOTFOCUS|PLOTFOCUSDPY); else
				curstate[0] |= PLOTFOCUSDPY | PLOTFOCUS;
		}
		if (DiaGetControl(dia, DPRO_EPS) != 0) curstate[0] |= EPSPSCRIPT; else
			curstate[0] &= ~EPSPSCRIPT;
		switch (DiaGetPopupEntry(dia, DPRO_ROTATE))
		{
			case 0: curstate[0] &= ~PSROTATE;   curstate[1] &= ~PSAUTOROTATE;   break;
			case 1: curstate[0] |= PSROTATE;    curstate[1] &= ~PSAUTOROTATE;   break;
			case 2: curstate[0] &= ~PSROTATE;   curstate[1] |= PSAUTOROTATE;    break;
		}
		if (DiaGetControl(dia, DPRO_PRINTDATE) != 0) curstate[0] |= PLOTDATES; else
			curstate[0] &= ~PLOTDATES;
		if (DiaGetControl(dia, DPRO_HPGL2) != 0) curstate[0] |= HPGL2; else
			curstate[0] &= ~HPGL2;
		switch (DiaGetPopupEntry(dia, DPRO_PSSTYLE))
		{
			case 0: curstate[0] &= ~(PSCOLOR1|PSCOLOR2);                  break;
			case 1: curstate[0] = (curstate[0] & ~PSCOLOR2) | PSCOLOR1;   break;
			case 2: curstate[0] |= PSCOLOR1 | PSCOLOR2;                   break;
			case 3: curstate[0] = (curstate[0] & ~PSCOLOR1) | PSCOLOR2;   break;
		}

		/* set PostScript page sizes */
		if (DiaGetControl(dia, DPRO_PRINTER) == 0) curstate[0] |= PSPLOTTER; else
		{
			/* printed */
			curstate[0] &= ~PSPLOTTER;
			i = atofr(DiaGetText(dia, DPRO_PRINTHEI));
			if (i != hei)
				(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_height"), i, VFRACT);
		}
		i = atofr(DiaGetText(dia, DPRO_PRINTWID));
		if (i != wid)
			(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_width"), i, VFRACT);
		i = atofr(DiaGetText(dia, DPRO_PRINTMARGIN));
		if (i != margin)
			(void)setval((INTBIG)io_tool, VTOOL, x_("IO_postscript_margin"), i, VFRACT);

		/* set printer resolution scale factor */
		i = eatoi(DiaGetText(dia, DPRO_EXTRARES));
		if (i != resfactor)
			(void)setval((INTBIG)io_tool, VTOOL, x_("IO_print_resolution_scale"), i, VINTEGER);

		if (np != NONODEPROTO)
		{
			var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, io_postscriptfilenamekey);
			if (DiaGetControl(dia, DPRO_SYNCHRONIZE) != 0)
			{
				/* add a synchronization file */
				pt = DiaGetText(dia, DPRO_SYNCHFILE);
				if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
					(void)setvalkey((INTBIG)np, VNODEPROTO, io_postscriptfilenamekey,
						(INTBIG)pt, VSTRING);
			} else
			{
				/* remove a synchronization file */
				if (var != NOVARIABLE)
					(void)delvalkey((INTBIG)np, VNODEPROTO, io_postscriptfilenamekey);
			}
			if (DiaGetControl(dia, DPRO_EPS) != 0)
			{
				i = atofr(DiaGetText(dia, DPRO_EPSSCALE));
				if (i != epsscale)
					(void)setvalkey((INTBIG)np, VNODEPROTO, io_postscriptepsscalekey,
						i, VFRACT);
			}
		}
		if ((curstate[0]&HPGL2) != 0)
		{
			var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_hpgl2_scale"));
			if (DiaGetControl(dia, DPRO_HPGL2FILLPAGE) != 0)
			{
				if (var != NOVARIABLE)
					(void)delval((INTBIG)io_tool, VTOOL, x_("IO_hpgl2_scale"));
			} else
			{
				scale = myatoi(DiaGetText(dia, DPRO_HPGL2SCALE));
				if (scale <= 0) scale = 1;
				if (var == NOVARIABLE || scale != var->addr)
					(void)setval((INTBIG)io_tool, VTOOL, x_("IO_hpgl2_scale"), scale, VINTEGER);
			}
		}

		for(i=0; i<NUMIOSTATEBITWORDS; i++) if (curstate[i] != oldstate[i]) break;
		if (i < NUMIOSTATEBITWORDS) io_setstatebits(curstate);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** PURE LAYER NODE DIALOG ******************************/

/* Edit: Pure Layer Node */
static DIALOGITEM us_purelayerdialogitems[] =
{
 /*  1 */ {0, {204,116,228,196}, BUTTON, N_("OK")},
 /*  2 */ {0, {204,12,228,92}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,8,196,200}, SCROLL, x_("")},
 /*  4 */ {0, {4,12,20,100}, MESSAGE, N_("Technology:")},
 /*  5 */ {0, {4,100,20,200}, MESSAGE, x_("")}
};
static DIALOG us_purelayerdialog = {{75,75,312,284}, N_("Make Pure Layer Node"), 0, 5, us_purelayerdialogitems, 0, 0};

/* special items for the "pure layer node" dialog: */
#define DPLN_NODELIST     3		/* list of nodes (scroll) */
#define DPLN_TECHNOLOGY   5		/* technology (stat text) */

INTBIG us_purelayernodedlog(CHAR *paramstart[])
{
	REGISTER INTBIG itemHit, fun;
	REGISTER INTBIG ret;
	REGISTER NODEPROTO *np;
	REGISTER void *dia;

	if (us_needcell() == NONODEPROTO) return(0);

	/* see if there are any pure layer nodes */
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun == NPNODE) break;
	}
	if (np == NONODEPROTO)
	{
		ttyputerr(_("This technology has no pure-layer nodes"));
		return(0);
	}

	/* display the dialog box */
	dia = DiaInitDialog(&us_purelayerdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DPLN_NODELIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCDOUBLEQUIT);
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		fun = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPNODE) continue;
		DiaStuffLine(dia, DPLN_NODELIST, np->protoname);
	}
	DiaSelectLine(dia, DPLN_NODELIST, 0);
	DiaSetText(dia, DPLN_TECHNOLOGY, el_curtech->techname);
	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}
	ret = 0;
	if (itemHit == OK)
	{
		ret = 1;
		if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
		allocstring(&us_returneddialogstring,
			DiaGetScrollLine(dia, DPLN_NODELIST, DiaGetCurLine(dia, DPLN_NODELIST)), us_tool->cluster);
		paramstart[0] = us_returneddialogstring;
	}
	DiaDoneDialog(dia);
	return(ret);
}

/****************************** QUICK KEY OPTIONS DIALOG ******************************/

/* Quick Keys */
static DIALOGITEM us_quickkeydialogitems[] =
{
 /*  1 */ {0, {520,320,544,384}, BUTTON, N_("OK")},
 /*  2 */ {0, {520,12,544,76}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,4,168,168}, SCROLL, x_("")},
 /*  4 */ {0, {192,20,336,184}, SCROLL, x_("")},
 /*  5 */ {0, {4,4,20,68}, MESSAGE, N_("Menu:")},
 /*  6 */ {0, {172,20,188,136}, MESSAGE, N_("SubMenu/Item:")},
 /*  7 */ {0, {360,36,504,200}, SCROLL, x_("")},
 /*  8 */ {0, {340,36,356,140}, MESSAGE, N_("SubItem:")},
 /*  9 */ {0, {24,228,504,392}, SCROLL, x_("")},
 /* 10 */ {0, {4,228,20,328}, MESSAGE, N_("Quick Key:")},
 /* 11 */ {0, {256,192,280,220}, BUTTON, x_(">>")},
 /* 12 */ {0, {520,236,544,296}, BUTTON, N_("Remove")},
 /* 13 */ {0, {520,96,544,216}, BUTTON, N_("Factory Settings")}
};
static DIALOG us_quickkeydialog = {{75,75,634,478}, N_("Quick Key Options"), 0, 13, us_quickkeydialogitems, 0, 0};

static void us_setlastquickkeys(void *dia);
static void us_setmiddlequickkeys(void *dia);
static void us_loadquickkeys(POPUPMENU *pm);
static CHAR *us_makequickkey(INTBIG i);

/* special items for the "Quick Keys" dialog: */
#define DQKO_MENULIST       3		/* menu list (scroll) */
#define DQKO_SUBLIST        4		/* submenu/item list (scroll) */
#define DQKO_SUBSUBLIST     7		/* subitem list (scroll) */
#define DQKO_KEYLIST        9		/* quick key list (scroll) */
#define DQKO_ADDKEY        11		/* ">>" (button) */
#define DQKO_REMOVEKEY     12		/* "<<" (button) */
#define DQKO_FACTORYRESET  13		/* Factory settings (button) */

#define MAXQUICKKEYS 300
static INTSML         us_quickkeyskeys[MAXQUICKKEYS];
static INTBIG         us_quickkeysspecial[MAXQUICKKEYS];
static POPUPMENU     *us_quickkeysmenu[MAXQUICKKEYS];
static INTBIG         us_quickkeysindex[MAXQUICKKEYS];
static INTBIG         us_quickkeyscount;

INTBIG us_quickkeydlog(void)
{
	INTBIG itemHit, i, j, k, which, whichmiddle, whichbottom, keychanged, special;
	CHAR **quickkeylist;
	INTSML key;
	INTBIG quickkeycount;
	REGISTER CHAR *menuname, *menucommand, *pt;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	REGISTER void *dia;

	/* display the window view dialog box */
	dia = DiaInitDialog(&us_quickkeydialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DQKO_MENULIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	DiaInitTextDialog(dia, DQKO_SUBLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	DiaInitTextDialog(dia, DQKO_SUBSUBLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	DiaInitTextDialog(dia, DQKO_KEYLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);

	/* make a list of quick keys */
	us_buildquickkeylist();
	for(i=0; i<us_quickkeyscount; i++)
		DiaStuffLine(dia, DQKO_KEYLIST, us_makequickkey(i));
	DiaSelectLine(dia, DQKO_KEYLIST, 0);

	/* load the list of menus */
	for(i=0; i<us_pulldownmenucount; i++)
		DiaStuffLine(dia, DQKO_MENULIST, us_removeampersand(us_pulldowns[i]->header));
	DiaSelectLine(dia, DQKO_MENULIST, 0);
	us_setmiddlequickkeys(dia);

	/* loop until done */
	keychanged = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DQKO_MENULIST)
		{
			/* click in the top list (the pulldown menus) */
			us_setmiddlequickkeys(dia);
			continue;
		}
		if (itemHit == DQKO_SUBLIST)
		{
			/* click in the middle list (the submenus/items) */
			us_setlastquickkeys(dia);
			continue;
		}
		if (itemHit == DQKO_SUBSUBLIST)
		{
			/* click in the lower list (the subitems) */
			which = DiaGetCurLine(dia, DQKO_MENULIST);
			whichmiddle = DiaGetCurLine(dia, DQKO_SUBLIST);
			whichbottom = DiaGetCurLine(dia, DQKO_SUBSUBLIST);
			pm = us_pulldowns[which];
			mi = &pm->list[whichmiddle];
			uc = mi->response;
			pm = uc->menu;
			if (pm == NOPOPUPMENU) continue;
			for(j=0; j<us_quickkeyscount; j++)
				if (pm == us_quickkeysmenu[j] && whichbottom == us_quickkeysindex[j]) break;
			if (j < us_quickkeyscount) DiaSelectLine(dia, DQKO_KEYLIST, j);
			continue;
		}
		if (itemHit == DQKO_ADDKEY)
		{
			/* click in the ">>" button (add command to quick keys) */
			which = DiaGetCurLine(dia, DQKO_MENULIST);
			whichmiddle = DiaGetCurLine(dia, DQKO_SUBLIST);
			whichbottom = DiaGetCurLine(dia, DQKO_SUBSUBLIST);
			pm = us_pulldowns[which];
			mi = &pm->list[whichmiddle];
			uc = mi->response;
			if (uc->menu != NOPOPUPMENU)
			{
				pm = uc->menu;
				whichmiddle = whichbottom;
			}
			which = DiaGetCurLine(dia, DQKO_KEYLIST);
			us_quickkeysmenu[which] = pm;
			us_quickkeysindex[which] = whichmiddle;
			DiaSetScrollLine(dia, DQKO_KEYLIST, which, us_makequickkey(which));
			keychanged++;
			continue;
		}
		if (itemHit == DQKO_REMOVEKEY)
		{
			/* click in the "<<" button (remove command from quick key) */
			which = DiaGetCurLine(dia, DQKO_KEYLIST);
			us_quickkeysmenu[which] = NOPOPUPMENU;
			DiaSetScrollLine(dia, DQKO_KEYLIST, which, us_makequickkey(which));
			keychanged++;
			continue;
		}
		if (itemHit == DQKO_FACTORYRESET)
		{
			/* click in the "Factory Settings" button */
			for(i=0; i<us_quickkeyscount; i++)
				us_quickkeysmenu[i] = NOPOPUPMENU;
			for(i=0; i<us_quickkeyfactcount; i++)
			{
				pt = us_quickkeyfactlist[i];
				menuname = us_getboundkey(pt, &key, &special);
				for(j=0; j<us_quickkeyscount; j++)
				{
					if (us_samekey(key, special, us_quickkeyskeys[j], us_quickkeysspecial[j]))
						break;
				}
				if (j >= us_quickkeyscount) continue;
				for(pt = menuname; *pt != 0 && *pt != '/'; pt++) ;
				if (*pt == 0) continue;
				*pt = 0;
				pm = us_getpopupmenu(menuname);
				*pt = '/';
				menucommand = pt + 1;
				for(k=0; k<pm->total; k++)
				{
					mi = &pm->list[k];
					if (namesame(us_removeampersand(mi->attribute), menucommand) == 0) break;
				}
				if (k >= pm->total) continue;
				us_quickkeysmenu[j] = pm;
				us_quickkeysindex[j] = k;
			}
			DiaLoadTextDialog(dia, DQKO_KEYLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
			for(i=0; i<us_quickkeyscount; i++)
				DiaStuffLine(dia, DQKO_KEYLIST, us_makequickkey(i));
			DiaSelectLine(dia, DQKO_KEYLIST, 0);
			keychanged++;
			continue;
		}
	}

	DiaDoneDialog(dia);
	if (itemHit == OK && keychanged != 0)
	{
		us_getquickkeylist(&quickkeycount, &quickkeylist);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_quickkeyskey, (INTBIG)quickkeylist,
			VSTRING|VISARRAY|(quickkeycount<<VLENGTHSH));
		for(i=0; i<quickkeycount; i++)
			efree((CHAR *)quickkeylist[i]);
		efree((CHAR *)quickkeylist);
	}
	return(0);
}

/*
 * Helper routine for "us_quickkeydlog" to construct the string describing key "i".
 */
CHAR *us_makequickkey(INTBIG i)
{
	REGISTER POPUPMENUITEM *mi;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, us_describeboundkey(us_quickkeyskeys[i], us_quickkeysspecial[i], 1));
	if (us_quickkeysmenu[i] != NOPOPUPMENU)
	{
		mi = &us_quickkeysmenu[i]->list[us_quickkeysindex[i]];
		addstringtoinfstr(infstr, x_("   "));
		addstringtoinfstr(infstr, us_removeampersand(mi->attribute));
	}
	return(returninfstr(infstr));
}

/*
 * Routine to scan all pulldown menus and build an internal list of quick keys.
 */
void us_buildquickkeylist(void)
{
	REGISTER INTBIG i, morebits;

	us_quickkeyscount = 0;
	for(i=0; i<26; i++)
	{
		us_quickkeysspecial[us_quickkeyscount] = ACCELERATORDOWN;
		us_quickkeyskeys[us_quickkeyscount] = (CHAR)('A' + i);
		us_quickkeyscount++;
	}
	for(i=0; i<10; i++)
	{
		us_quickkeysspecial[us_quickkeyscount] = ACCELERATORDOWN;
		us_quickkeyskeys[us_quickkeyscount] = (CHAR)('0' + i);
		us_quickkeyscount++;
	}
	for(i=0; i<26; i++)
	{
		us_quickkeysspecial[us_quickkeyscount] = 0;
		us_quickkeyskeys[us_quickkeyscount] = (CHAR)('a' + i);
		us_quickkeyscount++;
	}
	for(i=0; i<26; i++)
	{
		us_quickkeysspecial[us_quickkeyscount] = 0;
		us_quickkeyskeys[us_quickkeyscount] = (CHAR)('A' + i);
		us_quickkeyscount++;
	}
	for(i=0; i<10; i++)
	{
		us_quickkeysspecial[us_quickkeyscount] = 0;
		us_quickkeyskeys[us_quickkeyscount] = (CHAR)('0' + i);
		us_quickkeyscount++;
	}
	us_quickkeysspecial[us_quickkeyscount] = ACCELERATORDOWN;
	us_quickkeyskeys[us_quickkeyscount] = '.';
	us_quickkeyscount++;
	us_quickkeysspecial[us_quickkeyscount] = ACCELERATORDOWN;
	us_quickkeyskeys[us_quickkeyscount] = ',';
	us_quickkeyscount++;
	us_quickkeysspecial[us_quickkeyscount] = ACCELERATORDOWN;
	us_quickkeyskeys[us_quickkeyscount] = '+';
	us_quickkeyscount++;
	us_quickkeysspecial[us_quickkeyscount] = ACCELERATORDOWN;
	us_quickkeyskeys[us_quickkeyscount] = '-';
	us_quickkeyscount++;
	for(i=0; i<12; i++)
	{
		us_quickkeysspecial[us_quickkeyscount] = 0;
		us_quickkeysspecial[us_quickkeyscount] =
			SPECIALKEYDOWN|((SPECIALKEYF1+i)<<SPECIALKEYSH);
		us_quickkeyscount++;
	}
	for(i=0; i<4; i++)
	{
		morebits = 0;
		if ((i&1) != 0) morebits |= SHIFTDOWN;
		if ((i&2) != 0) morebits |= ACCELERATORDOWN;
		us_quickkeyskeys[us_quickkeyscount] = 0;
		us_quickkeysspecial[us_quickkeyscount] =
			SPECIALKEYDOWN|((SPECIALKEYARROWL)<<SPECIALKEYSH)|morebits;
		us_quickkeyscount++;
		us_quickkeyskeys[us_quickkeyscount] = 0;
		us_quickkeysspecial[us_quickkeyscount] =
			SPECIALKEYDOWN|((SPECIALKEYARROWR)<<SPECIALKEYSH)|morebits;
		us_quickkeyscount++;
		us_quickkeyskeys[us_quickkeyscount] = 0;
		us_quickkeysspecial[us_quickkeyscount] =
			SPECIALKEYDOWN|((SPECIALKEYARROWU)<<SPECIALKEYSH)|morebits;
		us_quickkeyscount++;
		us_quickkeyskeys[us_quickkeyscount] = 0;
		us_quickkeysspecial[us_quickkeyscount] =
			SPECIALKEYDOWN|((SPECIALKEYARROWD)<<SPECIALKEYSH)|morebits;
		us_quickkeyscount++;
	}

	for(i=0; i<us_quickkeyscount; i++) us_quickkeysmenu[i] = NOPOPUPMENU;
	for(i=0; i<us_pulldownmenucount; i++)
		us_loadquickkeys(us_pulldowns[i]);
}

/*
 * Routine to convert the internal list of quick keys to an array of bindings
 * in "quickkeylist" (that is "quickkeycount" long).
 */
void us_getquickkeylist(INTBIG *quickkeycount, CHAR ***quickkeylist)
{
	REGISTER INTBIG count, i;
	REGISTER CHAR **keylist;
	REGISTER POPUPMENUITEM *mi;
	REGISTER void *infstr;

	count = 0;
	for(i=0; i<us_quickkeyscount; i++)
		if (us_quickkeysmenu[i] != NOPOPUPMENU) count++;
	keylist = (CHAR **)emalloc(count * (sizeof (CHAR *)), us_tool->cluster);
	if (keylist == 0)
	{
		*quickkeycount = 0;
		return;
	}
	count = 0;
	for(i=0; i<us_quickkeyscount; i++)
	{
		if (us_quickkeysmenu[i] == NOPOPUPMENU) continue;
		infstr = initinfstr();
		addstringtoinfstr(infstr, us_describeboundkey(us_quickkeyskeys[i], us_quickkeysspecial[i], 0));
		addstringtoinfstr(infstr, us_quickkeysmenu[i]->name);
		addtoinfstr(infstr, '/');
		mi = &us_quickkeysmenu[i]->list[us_quickkeysindex[i]];
		addstringtoinfstr(infstr, us_removeampersand(mi->attribute));
		(void)allocstring(&keylist[count], returninfstr(infstr), us_tool->cluster);
		count++;
	}
	*quickkeycount = count;
	*quickkeylist = keylist;
}

/*
 * Helper routine for "us_quickkeydlog" to recursively examine menu "pm" and
 * load the quick keys tables.
 */
void us_loadquickkeys(POPUPMENU *pm)
{
	REGISTER INTBIG j, i;
	INTSML key;
	INTBIG special;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	REGISTER CHAR *pt;
	CHAR menuline[200];

	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		uc = mi->response;
		if (uc->menu != NOPOPUPMENU)
		{
			us_loadquickkeys(uc->menu);
			continue;
		}
		if (uc->active < 0 && *mi->attribute == 0) continue;

		estrcpy(menuline, mi->attribute);
		j = estrlen(menuline) - 1;
		if (menuline[j] == '<') menuline[j] = 0;
		for(pt = menuline; *pt != 0; pt++)
			if (*pt == '/' || *pt == '\\') break;
		if (*pt == 0) continue;
		(void)us_getboundkey(pt, &key, &special);
		for(j=0; j<us_quickkeyscount; j++)
		{
			if (!us_samekey(key, special, us_quickkeyskeys[j], us_quickkeysspecial[j]))
				continue;
			us_quickkeysmenu[j] = pm;
			us_quickkeysindex[j] = i;
		}
	}
}

/*
 * Helper routine for "us_quickkeydlog" to load the middle table (the
 * submenu/items) when the selected top table has changed.
 */
void us_setmiddlequickkeys(void *dia)
{
	REGISTER INTBIG which, i;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;

	DiaLoadTextDialog(dia, DQKO_SUBLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	which = DiaGetCurLine(dia, DQKO_MENULIST);
	pm = us_pulldowns[which];
	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		uc = mi->response;
		if (uc->active < 0 && *mi->attribute == 0)
		{
			DiaStuffLine(dia, DQKO_SUBLIST, x_("---"));
			continue;
		}
		DiaStuffLine(dia, DQKO_SUBLIST, us_removeampersand(mi->attribute));
	}
	DiaSelectLine(dia, DQKO_SUBLIST, 0);
	us_setlastquickkeys(dia);
}

/*
 * Helper routine for "us_quickkeydlog" to load the bottom table (the
 * subitems) when the selected middle table has changed.
 */
void us_setlastquickkeys(void *dia)
{
	REGISTER INTBIG which, whichmiddle, i, j;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;

	DiaLoadTextDialog(dia, DQKO_SUBSUBLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	which = DiaGetCurLine(dia, DQKO_MENULIST);
	whichmiddle = DiaGetCurLine(dia, DQKO_SUBLIST);
	pm = us_pulldowns[which];
	mi = &pm->list[whichmiddle];
	uc = mi->response;
	if (uc->menu != NOPOPUPMENU)
	{
		pm = uc->menu;
		for(i=0; i<pm->total; i++)
		{
			mi = &pm->list[i];
			uc = mi->response;
			if (uc->active < 0 && *mi->attribute == 0)
			{
				DiaStuffLine(dia, DQKO_SUBSUBLIST, x_("---"));
				continue;
			}
			DiaStuffLine(dia, DQKO_SUBSUBLIST, us_removeampersand(mi->attribute));
		}
		DiaSelectLine(dia, DQKO_SUBSUBLIST, 0);
		mi = &pm->list[0];
		whichmiddle = 0;
	}

	for(j=0; j<us_quickkeyscount; j++)
		if (pm == us_quickkeysmenu[j] && whichmiddle == us_quickkeysindex[j]) break;
	if (j < us_quickkeyscount) DiaSelectLine(dia, DQKO_KEYLIST, j);
}

/****************************** QUIT DIALOG ******************************/

/* Quit */
static DIALOGITEM us_quitdialogitems[] =
{
 /*  1 */ {0, {100,16,124,80}, BUTTON, N_("Yes")},
 /*  2 */ {0, {100,128,124,208}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {136,16,160,80}, BUTTON, N_("No")},
 /*  4 */ {0, {8,16,92,208}, MESSAGE, x_("")},
 /*  5 */ {0, {136,128,160,208}, BUTTON, N_("No to All")}
};
static DIALOG us_quitdialog = {{50,75,219,293}, 0, 0, 5, us_quitdialogitems, 0, 0};

/* special items for the "quit" command: */
#define DQUT_YES      1		/* Yes (button) */
#define DQUT_CANCEL   2		/* Cancel (button) */
#define DQUT_NO       3		/* No (button) */
#define DQUT_MESSAGE  4		/* Message (stat text) */
#define DQUT_NOTOALL  5		/* No to All (button) */

/*
 * Returns:
 *  0  cancel ("Cancel")
 *  1  do not save ("No")
 *  2  do not save any libraries ("No to all")
 *  3  save ("Yes")
 */
INTBIG us_quitdlog(CHAR *prompt, INTBIG notoall)
{
	INTBIG itemHit, i, len, oldplease;
	INTBIG retval;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the quit dialog box */
	if (notoall == 0) us_quitdialog.items = 4; else
		us_quitdialog.items = 5;
	dia = DiaInitDialog(&us_quitdialog);
	if (dia == 0) return(0);

	/* load the message */
	len = estrlen(prompt);
	for(i=0; i<len; i++) if (estrncmp(&prompt[i], _(" has changed.  "), 15) == 0) break;
	if (i >= len) DiaSetText(dia, DQUT_MESSAGE, prompt); else
	{
		infstr = initinfstr();
		prompt[i+15] = 0;
		addstringtoinfstr(infstr, prompt);
		addstringtoinfstr(infstr, _("Save?"));
		DiaSetText(dia, DQUT_MESSAGE, returninfstr(infstr));
	}

	/* loop until done */
	oldplease = el_pleasestop;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL ||
			itemHit == DQUT_NO || itemHit == DQUT_NOTOALL) break;
	}
	el_pleasestop = oldplease;

	DiaDoneDialog(dia);
	switch (itemHit)
	{
		case CANCEL:       retval = 0;   break;
		case DQUT_NO:      retval = 1;   break;
		case DQUT_NOTOALL: retval = 2;   break;
		case OK:           retval = 3;   break;
	}
	return(retval);
}

/****************************** RENAME DIALOG ******************************/

/* Rename */
static DIALOGITEM us_rendialogitems[] =
{
 /*  1 */ {0, {196,236,220,316}, BUTTON, N_("OK")},
 /*  2 */ {0, {116,236,140,316}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {56,8,288,220}, SCROLL, x_("")},
 /*  4 */ {0, {292,8,308,88}, MESSAGE, N_("New name:")},
 /*  5 */ {0, {292,84,308,316}, EDITTEXT, x_("")},
 /*  6 */ {0, {8,8,24,316}, POPUP, x_("")},
 /*  7 */ {0, {32,84,48,316}, POPUP, x_("")},
 /*  8 */ {0, {32,8,48,84}, MESSAGE, N_("Library:")}
};
static DIALOG us_rendialog = {{75,75,392,401}, N_("Rename Object"), 0, 8, us_rendialogitems, 0, 0};

/* special items for the "rename" dialog: */
#define DREN_OLDNAMES    3		/* list of old names (scroll) */
#define DREN_NEWNAME     5		/* new name (edit text) */
#define DREN_OBJTYPE     6		/* object type (popup) */
#define DREN_LIBRARY     7		/* library selection (popup) */
#define DREN_LIBRARY_L   8		/* library label (stat text) */

#define RENAMECATEGORIES  5

static INTBIG      us_renametype;
static LIBRARY    *us_renamelib;
static LIBRARY    *us_renamecurlib;
static TECHNOLOGY *us_renametech;
static PORTPROTO  *us_renamediaport;
static NODEPROTO  *us_renamenodeproto;
static INTBIG      us_renamenetindex;
static void       *us_renamenetsa;

static BOOLEAN us_initrenamelist(CHAR **c);
static CHAR   *us_renamelistitem(void);

INTBIG us_renamedlog(INTBIG type)
{
	REGISTER INTBIG itemHit, typeindex;
	INTBIG count, i;
	REGISTER BOOLEAN initcatagory;
	CHAR *par[3], *newlang[7], **liblist;
	REGISTER void *dia, *infstr;
	static CHAR *renamename[RENAMECATEGORIES] = {N_("Libraries:"), N_("Technologies:"),
		N_("Exports:"), N_("Cells:"), N_("Networks:")};
	static INTBIG renametype[RENAMECATEGORIES] = {VLIBRARY, VTECHNOLOGY, VPORTPROTO, VNODEPROTO, VNETWORK};
	static CHAR *renameletter[RENAMECATEGORIES] = {x_("l"), x_("t"), x_("r"), x_("p"), x_("n")};

	/* display the rename dialog box */
	dia = DiaInitDialog(&us_rendialog);
	if (dia == 0) return(0);
	us_renamenetsa = 0;
	for(i=0; i<RENAMECATEGORIES; i++) newlang[i] = TRANSLATE(renamename[i]);
	DiaSetPopup(dia, DREN_OBJTYPE, RENAMECATEGORIES, newlang);
	for(typeindex=0; typeindex<RENAMECATEGORIES; typeindex++)
	{
		if (type == renametype[typeindex])
		{
			DiaSetPopupEntry(dia, DREN_OBJTYPE, typeindex);
			break;
		}
	}
	liblist = us_makelibrarylist(&count, el_curlib, &i);
	DiaSetPopup(dia, DREN_LIBRARY, count, liblist);
	if (i >= 0) DiaSetPopupEntry(dia, DREN_LIBRARY, i);
	us_renamecurlib = el_curlib;

	/* loop until done */
	initcatagory = TRUE;
	for(;;)
	{
		if (initcatagory)
		{
			initcatagory = FALSE;
			typeindex = DiaGetPopupEntry(dia, DREN_OBJTYPE);
			us_renametype = renametype[typeindex];
			DiaInitTextDialog(dia, DREN_OLDNAMES, us_initrenamelist, us_renamelistitem,
				DiaNullDlogDone, 0, SCSELMOUSE|SCREPORT);
			if (us_renametype == VNODEPROTO)
			{
				DiaUnDimItem(dia, DREN_LIBRARY);
				DiaUnDimItem(dia, DREN_LIBRARY_L);
				(void)us_setscrolltocurrentcell(DREN_OLDNAMES, TRUE, TRUE, FALSE, TRUE, dia);
			} else
			{
				DiaDimItem(dia, DREN_LIBRARY);
				DiaDimItem(dia, DREN_LIBRARY_L);
			}
			i = DiaGetCurLine(dia, DREN_OLDNAMES);
			if (i >= 0)
				DiaSetText(dia, -DREN_NEWNAME, DiaGetScrollLine(dia, DREN_OLDNAMES, i));
		}
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK)
		{
			i = DiaGetCurLine(dia, DREN_OLDNAMES);
			if (i < 0) continue;
			if (!DiaValidEntry(dia, DREN_NEWNAME)) continue;

			/* do the rename */
			par[2] = renameletter[typeindex];
			i = DiaGetCurLine(dia, DREN_OLDNAMES);
			infstr = initinfstr();
			if (us_renametype == VNODEPROTO)
				formatinfstr(infstr, x_("%s:"), us_renamecurlib->libname);
			addstringtoinfstr(infstr, DiaGetScrollLine(dia, DREN_OLDNAMES, i));
			allocstring(&par[0], returninfstr(infstr), el_tempcluster);
			allocstring(&par[1], DiaGetText(dia, DREN_NEWNAME), el_tempcluster);
			us_rename(3, par);
			efree((CHAR *)par[0]);
			efree((CHAR *)par[1]);
			break;
		}
		if (itemHit == DREN_OLDNAMES)
		{
			i = DiaGetCurLine(dia, DREN_OLDNAMES);
			if (i >= 0)
				DiaSetText(dia, -DREN_NEWNAME, DiaGetScrollLine(dia, DREN_OLDNAMES, i));
			continue;
		}
		if (itemHit == DREN_OBJTYPE)
		{
			initcatagory = TRUE;
			continue;
		}
		if (itemHit == DREN_LIBRARY)
		{
			typeindex = DiaGetPopupEntry(dia, DREN_LIBRARY);
			us_renamecurlib = getlibrary(liblist[typeindex]);
			initcatagory = TRUE;
			continue;
		}
	}
	if (us_renamenetsa != 0) killstringarray(us_renamenetsa);
	if (liblist != 0) efree((CHAR *)liblist);
	DiaDoneDialog(dia);
	return(0);
}

BOOLEAN us_initrenamelist(CHAR **c)
{
	Q_UNUSED( c );
	REGISTER NODEPROTO *np;
	CHAR **strings;
	REGISTER INTBIG i;
	INTBIG count;
	REGISTER CHAR *pt, save, *ptr;
	REGISTER NETWORK *net;

	us_renamelib = el_curlib;
	us_renametech = el_technologies;
	us_renamenodeproto = us_renamecurlib->firstnodeproto;
	np = getcurcell();
	if (np == NONODEPROTO)
	{
		us_renamediaport = NOPORTPROTO;
		us_renamenetindex = 0;
	} else
	{
		us_renamediaport = np->firstportproto;
		us_renamenetindex = 0;
		if (us_renametype == VNETWORK)
		{
			if (us_renamenetsa == 0)
				us_renamenetsa = newstringarray(us_tool->cluster);
			clearstrings(us_renamenetsa);
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			{
				for(i=0; i<net->namecount; i++)
				{
					pt = networkname(net, i);
					for(ptr = pt; *ptr != 0; ptr++)
						if (*ptr == '[') break;
					save = *ptr;
					*ptr = 0;
					addtostringarray(us_renamenetsa, pt);
					*ptr = save;
				}
			}
			strings = getstringarray(us_renamenetsa, &count);
			esort(strings, count, sizeof (CHAR *), sort_stringascending);
		}
	}
	return(TRUE);
}

CHAR *us_renamelistitem(void)
{
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER CHAR *pt;
	CHAR **strings;
	INTBIG count;

	switch (us_renametype)
	{
		case VLIBRARY:
			for(;;)
			{
				if (us_renamelib == NOLIBRARY) break;
				lib = us_renamelib;
				us_renamelib = us_renamelib->nextlibrary;
				if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
				return(lib->libname);
			}
			break;
		case VTECHNOLOGY:
			if (us_renametech == NOTECHNOLOGY) break;
			tech = us_renametech;
			us_renametech = us_renametech->nexttechnology;
			return(tech->techname);
		case VPORTPROTO:
			if (us_renamediaport == NOPORTPROTO) break;
			pp = us_renamediaport;
			us_renamediaport = us_renamediaport->nextportproto;
			return(pp->protoname);
		case VNODEPROTO:
			if (us_renamenodeproto == NONODEPROTO) break;
			np = us_renamenodeproto;
			us_renamenodeproto = us_renamenodeproto->nextnodeproto;
			return(np->protoname);
		case VNETWORK:
			for(;;)
			{
				strings = getstringarray(us_renamenetsa, &count);
				if (us_renamenetindex >= count) break;
				pt = strings[us_renamenetindex];
				us_renamenetindex++;
				if (us_renamenetindex > 1 &&
					namesame(strings[us_renamenetindex-1], strings[us_renamenetindex-2]) == 0)
						continue;
				return(pt);
			}
	}
	return(0);
}

/****************************** ROM GENERATION DIALOG ******************************/

/* ROM Generator */
static DIALOGITEM us_romgdialogitems[] =
{
 /*  1 */ {0, {104,164,128,244}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,48,128,128}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,200}, MESSAGE, N_("ROM personality file:")},
 /*  4 */ {0, {24,4,56,296}, EDITTEXT, x_("")},
 /*  5 */ {0, {4,224,20,272}, BUTTON, N_("Set")},
 /*  6 */ {0, {72,4,88,176}, MESSAGE, N_("Gate size (nanometers):")},
 /*  7 */ {0, {72,180,88,272}, EDITTEXT, x_("")}
};
static DIALOG us_romgdialog = {{75,75,212,381}, N_("ROM Generation"), 0, 7, us_romgdialogitems, 0, 0};

/* special items for the "ROM generation" dialog: */
#define DROG_FILENAME        4		/* ROM personality file (edit text) */
#define DSLO_SETFILE         5		/* set personality file (button) */
#define DSLO_GATESIZE        7		/* gate size (edit text) */

INTBIG us_romgendlog(void)
{
	REGISTER INTBIG itemHit, i, l, oldplease;
	REGISTER void *dia, *infstr;
	REGISTER CHAR *pt, *filename, *gatesize, *cellname;
	CHAR *subparams[5], buf[50];

	/* display the dialog box */
	dia = DiaInitDialog(&us_romgdialog);
	if (dia == 0) return(0);
	esnprintf(buf, 50, x_("%ld"), el_curlib->lambda[el_curtech->techindex]);
	DiaSetText(dia, DSLO_GATESIZE, buf);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DSLO_SETFILE)
		{
			/* set personality file */
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("text/"));
			addstringtoinfstr(infstr, _("Personality File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, subparams);
			el_pleasestop = oldplease;
			if (i != 0 && subparams[0][0] != 0)
				DiaSetText(dia, DROG_FILENAME, subparams[0]);
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		filename = DiaGetText(dia, DROG_FILENAME);
		gatesize = DiaGetText(dia, DSLO_GATESIZE);
		l = strlen(filename);
		for(i=l-1; i>0; i--)
			if (filename[i] == DIRSEP) break;
		if (i > 0) i++;
		infstr = initinfstr();
		for(pt = &filename[i]; *pt != 0; pt++)
		{
			if (*pt == '.') break;
			addtoinfstr(infstr, *pt);
		}
		cellname = returninfstr(infstr);
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("romgen.main(\""));
		for(pt = filename; *pt != 0; pt++)
		{
			addtoinfstr(infstr, *pt);
			if (*pt == '\\') addtoinfstr(infstr, *pt);
		}
		formatinfstr(infstr, x_("\", \"%s\", %s)"), cellname, gatesize);
		pt = returninfstr(infstr);
		ttyputmsg("Executing %s", pt);
		(void)doquerry(pt, VJAVA, VINTEGER);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** SELECTION: OPTIONS DIALOG ******************************/

/* Selection Options */
static DIALOGITEM us_seloptdialogitems[] =
{
 /*  1 */ {0, {108,196,132,276}, BUTTON, N_("OK")},
 /*  2 */ {0, {108,4,132,84}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,4,24,280}, CHECK, N_("Easy selection of cell instances")},
 /*  4 */ {0, {32,4,48,280}, CHECK, N_("Easy selection of annotation text")},
 /*  5 */ {0, {56,4,72,280}, CHECK, N_("Center-based primitives")},
 /*  6 */ {0, {80,4,96,280}, CHECK, N_("Dragging must enclose entire object")}
};
static DIALOG us_seloptdialog = {{75,75,216,365}, N_("Selection Options"), 0, 6, us_seloptdialogitems, 0, 0};

/* special items for the "selection options" dialog: */
#define DSLO_EASYINSTANCES   3		/* easy selection of instances (check) */
#define DSLO_EASYANNOTATION  4		/* easy selection of annotation (check) */
#define DSLO_CENTERPRIMS     5		/* Center-based primitives (check) */
#define DSLO_DRAGMUSTENCLOSE 6		/* Dragging must enclose entire object (check) */

INTBIG us_selectoptdlog(void)
{
	REGISTER INTBIG itemHit;
	REGISTER INTBIG options;
	REGISTER void *dia;

	/* display the dialog box */
	dia = DiaInitDialog(&us_seloptdialog);
	if (dia == 0) return(0);
	if ((us_useroptions&NOINSTANCESELECT) == 0) DiaSetControl(dia, DSLO_EASYINSTANCES, 1);
	if ((us_useroptions&NOTEXTSELECT) == 0) DiaSetControl(dia, DSLO_EASYANNOTATION, 1);
	if ((us_useroptions&CENTEREDPRIMITIVES) != 0) DiaSetControl(dia, DSLO_CENTERPRIMS, 1);
	if ((us_useroptions&MUSTENCLOSEALL) != 0) DiaSetControl(dia, DSLO_DRAGMUSTENCLOSE, 1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DSLO_EASYINSTANCES || itemHit == DSLO_EASYANNOTATION ||
			itemHit == DSLO_CENTERPRIMS || itemHit == DSLO_DRAGMUSTENCLOSE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		options = us_useroptions;
		if (DiaGetControl(dia, DSLO_EASYINSTANCES) != 0) options &= ~NOINSTANCESELECT; else
			options |= NOINSTANCESELECT;
		if (DiaGetControl(dia, DSLO_EASYANNOTATION) != 0) options &= ~NOTEXTSELECT; else
			options |= NOTEXTSELECT;
		if (DiaGetControl(dia, DSLO_CENTERPRIMS) == 0) options &= ~CENTEREDPRIMITIVES; else
			options |= CENTEREDPRIMITIVES;
		if (DiaGetControl(dia, DSLO_DRAGMUSTENCLOSE) == 0) options &= ~MUSTENCLOSEALL; else
			options |= MUSTENCLOSEALL;
		if (options != us_useroptions)
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, options, VINTEGER);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** SELECTION: PORT/NODE/NET DIALOG ******************************/

/* Selection: Export/Net/Node */
static DIALOGITEM us_selnamedialogitems[] =
{
 /*  1 */ {0, {192,60,216,140}, BUTTON, N_("Done")},
 /*  2 */ {0, {8,8,180,192}, SCROLLMULTI, x_("")}
};
static DIALOG us_selnamedialog = {{75,75,300,276}, N_("Select Port"), 0, 2, us_selnamedialogitems, 0, 0};

/* special items for the "select export/net" dialog: */
#define DSPN_LIST   2		/* export/net/node list (scroll) */

static INTBIG     us_selportnodenet;
static NETWORK   *us_selnet;
static PORTPROTO *us_selport;
static NODEINST  *us_selnode;
static ARCINST   *us_selarc;

static BOOLEAN us_topofobject(CHAR **c);
static CHAR   *us_nextobject(void);

BOOLEAN us_topofobject(CHAR **c)
{
	Q_UNUSED( c );
	REGISTER NODEPROTO *np;

	np = us_needcell();
	switch (us_selportnodenet)
	{
		case 0: us_selnet = np->firstnetwork;      break;
		case 1: us_selport = np->firstportproto;   break;
		case 2: us_selnode = np->firstnodeinst;    break;
		case 3: us_selarc = np->firstarcinst;      break;
	}
	return(TRUE);
}

CHAR *us_nextobject(void)
{
	REGISTER CHAR *retname;
	static CHAR genname[100];
	REGISTER NETWORK *retnet;
	REGISTER NODEINST *retnode;
	REGISTER ARCINST *retarc;
	REGISTER VARIABLE *var;

	switch (us_selportnodenet)
	{
		case 0:		/* network */
			if (us_selnet == NONETWORK) break;
			retnet = us_selnet;
			us_selnet = us_selnet->nextnetwork;
			if (retnet->globalnet >= 0) return(describenetwork(retnet));
			if (retnet->namecount != 0) return(networkname(retnet, 0));
			esnprintf(genname, 100, x_("NET%ld"), (INTBIG)retnet);
			return(genname);
		case 1:		/* port */
			if (us_selport == NOPORTPROTO) break;
			retname = us_selport->protoname;
			us_selport = us_selport->nextportproto;
			return(retname);
		case 2:		/* node */
			for(;;)
			{
				if (us_selnode == NONODEINST) return(0);
				retnode = us_selnode;
				us_selnode = us_selnode->nextnodeinst;
				var = getvalkey((INTBIG)retnode, VNODEINST, VSTRING, el_node_name_key);
				if (var != NOVARIABLE) break;
			}
			return((CHAR *)var->addr);
		case 3:		/* arc */
			for(;;)
			{
				if (us_selarc == NOARCINST) return(0);
				retarc = us_selarc;
				us_selarc = us_selarc->nextarcinst;
				var = getvalkey((INTBIG)retarc, VARCINST, VSTRING, el_arc_name_key);
				if (var != NOVARIABLE) break;
			}
			return((CHAR *)var->addr);
	}
	return(0);
}

/*
 * special code for the "selection object" dialog
 * "selport" is: 0=network, 1=port, 2=node, 3=arc
 */
INTBIG us_selectobjectdlog(INTBIG selportnodenet)
{
	REGISTER INTBIG itemHit, *whichlist, which, i, j;
	REGISTER BOOLEAN first;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net, **netlist;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt;
	REGISTER void *infstr;
	REGISTER void *dia;

	np = us_needcell();
	if (np == NONODEPROTO) return(0);

	/* display the dialog box */
	us_selportnodenet = selportnodenet;
	switch (selportnodenet)
	{
		case 0: us_selnamedialog.movable = _("Select Network");   break;
		case 1: us_selnamedialog.movable = _("Select Export");    break;
		case 2: us_selnamedialog.movable = _("Select Node");      break;
		case 3: us_selnamedialog.movable = _("Select Arc");       break;
	}
	dia = DiaInitDialog(&us_selnamedialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DSPN_LIST, us_topofobject, us_nextobject, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DSPN_LIST)
		{
			whichlist = DiaGetCurLines(dia, DSPN_LIST);
			if (whichlist[0] < 0) continue;
			infstr = initinfstr();
			first = FALSE;
			for(i=0; whichlist[i] >= 0; i++)
			{
				which = whichlist[i];
				pt = DiaGetScrollLine(dia, DSPN_LIST, which);
				switch (selportnodenet)
				{
					case 0:		/* find network */
						netlist = getcomplexnetworks(pt, np);
						for(j=0; netlist[j] != NONETWORK; j++)
						{
							net = netlist[j];
							for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
							{
								if (ai->network != net) continue;
								if (first) addtoinfstr(infstr, '\n');
								first = TRUE;
								formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
									describenodeproto(np), (INTBIG)ai->geom);
							}
						}
						break;
					case 1:		/* find port */
						pp = getportproto(np, pt);
						if (pp != NOPORTPROTO)
						{
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;0"),
								describenodeproto(np), (INTBIG)pp->subnodeinst->geom,
									(INTBIG)pp);
						}
						break;
					case 2:		/* find node */
						for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
						{
							var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
							if (var == NOVARIABLE) continue;
							if (namesame(pt, (CHAR *)var->addr) != 0) continue;
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
								describenodeproto(np), (INTBIG)ni->geom);
							break;
						}
						break;
					case 3:		/* find arc */
						for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
						{
							var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
							if (var == NOVARIABLE) continue;
							if (namesame(pt, (CHAR *)var->addr) != 0) continue;
							if (first) addtoinfstr(infstr, '\n');
							first = TRUE;
							formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0"),
								describenodeproto(np), (INTBIG)ai->geom);
							break;
						}
						break;
				}
			}
			us_setmultiplehighlight(returninfstr(infstr), FALSE);
			us_showallhighlight();
			us_endchanges(NOWINDOWPART);
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** SPREAD DIALOG ******************************/

/* Spread */
static DIALOGITEM us_spreaddialogitems[] =
{
 /*  1 */ {0, {96,128,120,200}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,16,120,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {55,15,71,205}, EDITTEXT, x_("")},
 /*  4 */ {0, {20,230,36,380}, RADIO, N_("Spread up")},
 /*  5 */ {0, {45,230,61,380}, RADIO, N_("Spread down")},
 /*  6 */ {0, {70,230,86,380}, RADIO, N_("Spread left")},
 /*  7 */ {0, {95,230,111,380}, RADIO, N_("Spread right")},
 /*  8 */ {0, {25,15,41,180}, MESSAGE, N_("Distance to spread")}
};
static DIALOG us_spreaddialog = {{50,75,188,464}, N_("Spread About Highlighted"), 0, 8, us_spreaddialogitems, 0, 0};

/* special items for the "spread" dialog: */
#define DSPR_DISTANCE  3		/* Distance (edit text) */
#define DSPR_UP        4		/* Up (radio) */
#define DSPR_DOWN      5		/* Down (radio) */
#define DSPR_LEFT      6		/* Left (radio) */
#define DSPR_RIGHT     7		/* Right (radio) */

INTBIG us_spreaddlog(void)
{
	CHAR *param[2];
	INTBIG itemHit;
	static INTBIG lastamount = WHOLE;
	static INTBIG defdir  = DSPR_UP;
	REGISTER void *dia;

	/* display the spread dialog box */
	dia = DiaInitDialog(&us_spreaddialog);
	if (dia == 0) return(0);

	/* "up" is the default direction, distance is 1 */
	DiaSetText(dia, -DSPR_DISTANCE, frtoa(lastamount));
	DiaSetControl(dia, defdir, 1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK && DiaValidEntry(dia, DSPR_DISTANCE)) break;
		if (itemHit == DSPR_UP || itemHit == DSPR_DOWN ||
			itemHit == DSPR_LEFT || itemHit == DSPR_RIGHT)
		{
			DiaSetControl(dia, DSPR_UP, 0);
			DiaSetControl(dia, DSPR_DOWN, 0);
			DiaSetControl(dia, DSPR_LEFT, 0);
			DiaSetControl(dia, DSPR_RIGHT, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DSPR_UP) != 0)
		{
			defdir = DSPR_UP;   param[0] = x_("up");
		} else if (DiaGetControl(dia, DSPR_DOWN) != 0)
		{
			defdir = DSPR_DOWN;   param[0] = x_("down");
		} else if (DiaGetControl(dia, DSPR_LEFT) != 0)
		{
			defdir = DSPR_LEFT;   param[0] = x_("left");
		} else if (DiaGetControl(dia, DSPR_RIGHT) != 0)
		{
			defdir = DSPR_RIGHT;   param[0] = x_("right");
		}
		param[1] = DiaGetText(dia, DSPR_DISTANCE);
		lastamount = atofr(param[1]);
		us_spread(2, param);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** TECHNOLOGY EDIT: CONVERT LIBRARY TO TECHNOLOGY ******************************/

/* Technology Edit: Convert Library */
static DIALOGITEM us_tecedlibtotechdialogitems[] =
{
 /*  1 */ {0, {96,284,120,364}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,16,120,96}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {88,120,104,252}, CHECK, N_("Also write C code")},
 /*  4 */ {0, {8,8,24,224}, MESSAGE, N_("Creating new technology:")},
 /*  5 */ {0, {8,228,24,372}, EDITTEXT, x_("")},
 /*  6 */ {0, {40,8,56,372}, MESSAGE, N_("Already a technology with this name")},
 /*  7 */ {0, {64,8,80,224}, MESSAGE, N_("Rename existing technology to:")},
 /*  8 */ {0, {64,228,80,372}, EDITTEXT, x_("")},
 /*  9 */ {0, {112,120,128,272}, CHECK, N_("Also write Java code")}
};
static DIALOG us_tecedlibtotechdialog = {{75,75,212,457}, N_("Convert Library to Technology"), 0, 9, us_tecedlibtotechdialogitems, 0, 0};

/* special items for the "convert library to technology" dialog: */
#define DLTT_WRITECCODE     3		/* Write C code (check) */
#define DLTT_TECHNAME       5		/* New tech name (edit text) */
#define DLTT_EXISTWARN_L1   6		/* Warning that tech exists (stat text) */
#define DLTT_EXISTWARN_L2   7		/* Warning that tech exists (stat text) */
#define DLTT_RENAME         8		/* New name of existing technology (edit text) */
#define DLTT_WRITEJAVACCODE 9		/* Write Java code (check) */

INTBIG us_libtotechnologydlog(void)
{
	INTBIG itemHit;
	BOOLEAN checkforconflict, conflicts;
	CHAR *par[5], *pt;
	REGISTER TECHNOLOGY *tech;
	REGISTER void *dia;

	/* display the dependent library dialog box */
	dia = DiaInitDialog(&us_tecedlibtotechdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DLTT_TECHNAME, el_curlib->libname);

	/* loop until done */
	checkforconflict = TRUE;
	for(;;)
	{
		if (checkforconflict)
		{
			checkforconflict = FALSE;
			pt = DiaGetText(dia, DLTT_TECHNAME);
			for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
				if (namesame(tech->techname, pt) == 0) break;
			if (tech == NOTECHNOLOGY)
			{
				DiaDimItem(dia, DLTT_EXISTWARN_L1);
				DiaDimItem(dia, DLTT_EXISTWARN_L2);
				DiaSetText(dia, DLTT_RENAME, x_(""));
				DiaDimItem(dia, DLTT_RENAME);
				conflicts = FALSE;
			} else
			{
				DiaUnDimItem(dia, DLTT_EXISTWARN_L1);
				DiaUnDimItem(dia, DLTT_EXISTWARN_L2);
				DiaUnDimItem(dia, DLTT_RENAME);
				conflicts = TRUE;
			}
		}
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK)
		{
			if (conflicts)
			{
				if (!DiaValidEntry(dia, DLTT_RENAME)) continue;
			}
			break;
		}
		if (itemHit == DLTT_TECHNAME)
		{
			checkforconflict = TRUE;
			continue;
		}
		if (itemHit == DLTT_WRITECCODE)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			DiaSetControl(dia, DLTT_WRITEJAVACCODE, 0);
			continue;
		}
		if (itemHit == DLTT_WRITEJAVACCODE)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (conflicts)
		{
			par[0] = DiaGetText(dia, DLTT_TECHNAME);
			par[1] = DiaGetText(dia, DLTT_RENAME);
			par[2] = x_("t");
			us_rename(3, par);
		}
		par[0] = x_("edit");
		if (DiaGetControl(dia, DLTT_WRITECCODE) != 0) par[1] = x_("library-to-tech-and-C"); else
		{
			if (DiaGetControl(dia, DLTT_WRITEJAVACCODE) != 0) par[1] = x_("library-to-tech-and-Java"); else
				par[1] = x_("library-to-tech");
		}
		par[2] = DiaGetText(dia, DLTT_TECHNAME);
		us_technology(3, par);
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** TECHNOLOGY EDIT: DEPENDENT LIBRARIES DIALOG ******************************/

/* Dependent Libraries */
static DIALOGITEM us_dependentlibdialogitems[] =
{
 /*  1 */ {0, {208,368,232,432}, BUTTON, N_("OK")},
 /*  2 */ {0, {208,256,232,320}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,177,174}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,24,153}, MESSAGE, N_("Dependent Libraries:")},
 /*  5 */ {0, {208,8,224,165}, MESSAGE, N_("Libraries are examined")},
 /*  6 */ {0, {40,192,64,256}, BUTTON, N_("Remove")},
 /*  7 */ {0, {88,192,112,256}, BUTTON, N_("<< Add")},
 /*  8 */ {0, {128,280,144,427}, MESSAGE, N_("Library (if not in list):")},
 /*  9 */ {0, {152,280,168,432}, EDITTEXT, x_("")},
 /* 10 */ {0, {8,272,24,361}, MESSAGE, N_("All Libraries:")},
 /* 11 */ {0, {224,8,240,123}, MESSAGE, N_("from bottom up")},
 /* 12 */ {0, {32,272,118,438}, SCROLL, x_("")},
 /* 13 */ {0, {184,8,200,67}, MESSAGE, N_("Current:")},
 /* 14 */ {0, {184,72,200,254}, MESSAGE, x_("")}
};
static DIALOG us_dependentlibdialog = {{50,75,299,524}, N_("Dependent Library Selection"), 0, 14, us_dependentlibdialogitems, 0, 0};

static void us_showliblist(CHAR**, INTBIG, void*);

/* special items for the "dependent libraries" dialog: */
#define DTED_DEPENDENTLIST  3		/* Dependent list (scroll) */
#define DTED_REMOVELIB      6		/* Remove lib (button) */
#define DTED_ADDLIB         7		/* Add lib (button) */
#define DTED_NEWNAME        9		/* New name (edit text) */
#define DTED_LIBLIST       12		/* Library list (scroll) */
#define DTED_CURRENTLIB    14		/* Current lib (stat text) */

INTBIG us_dependentlibdlog(void)
{
	INTBIG itemHit, i, j, liblistlen;
	REGISTER VARIABLE *var;
	CHAR **liblist, **newliblist, *pt;
	REGISTER void *dia;

	/* display the dependent library dialog box */
	dia = DiaInitDialog(&us_dependentlibdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DTED_CURRENTLIB, el_curlib->libname);
	DiaInitTextDialog(dia, DTED_LIBLIST, topoflibs, nextlibs, DiaNullDlogDone, 0, SCSELMOUSE|SCSELKEY);
	DiaInitTextDialog(dia, DTED_DEPENDENTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE|SCSELKEY);
	var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_dependent_libraries"));
	if (var == NOVARIABLE) liblistlen = 0; else
	{
		liblistlen = getlength(var);
		liblist = (CHAR **)emalloc(liblistlen * (sizeof (CHAR *)), el_tempcluster);
		if (liblist == 0) return(0);
		for(i=0; i<liblistlen; i++)
			(void)allocstring(&liblist[i], ((CHAR **)var->addr)[i], el_tempcluster);
	}
	us_showliblist(liblist, liblistlen, dia);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DTED_REMOVELIB)
		{
			/* remove */
			i = DiaGetCurLine(dia, DTED_DEPENDENTLIST);
			if (i < 0 || i >= liblistlen) continue;
			efree(liblist[i]);
			for(j=i; j<liblistlen-1; j++) liblist[j] = liblist[j+1];
			liblistlen--;
			if (liblistlen == 0) efree((CHAR *)liblist);
			us_showliblist(liblist, liblistlen, dia);
			continue;
		}
		if (itemHit == DTED_ADDLIB)
		{
			/* add */
			pt = DiaGetText(dia, DTED_NEWNAME);
			while (*pt == ' ') pt++;
			if (*pt == 0) pt = DiaGetScrollLine(dia, DTED_LIBLIST, DiaGetCurLine(dia, DTED_LIBLIST));
			i = DiaGetCurLine(dia, DTED_DEPENDENTLIST);
			if (i < 0) i = 0;

			/* create a new list */
			newliblist = (CHAR **)emalloc((liblistlen+1) * (sizeof (CHAR *)), el_tempcluster);
			if (newliblist == 0) return(0);
			for(j=0; j<liblistlen; j++) newliblist[j] = liblist[j];
			if (liblistlen != 0) efree((CHAR *)liblist);
			liblist = newliblist;

			for(j=liblistlen; j>i; j--) liblist[j] = liblist[j-1];
			liblistlen++;
			(void)allocstring(&liblist[i], pt, el_tempcluster);
			us_showliblist(liblist, liblistlen, dia);
			DiaSetText(dia, DTED_NEWNAME, x_(""));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (liblistlen == 0)
		{
			if (var != NOVARIABLE)
				(void)delval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_dependent_libraries"));
		} else
		{
			(void)setval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_dependent_libraries"),
				(INTBIG)liblist, VSTRING|VISARRAY|(liblistlen<<VLENGTHSH));
		}
	}
	for(i=0; i<liblistlen; i++) efree(liblist[i]);
	if (liblistlen != 0) efree((CHAR *)liblist);
	DiaDoneDialog(dia);
	return(0);
}

void us_showliblist(CHAR **liblist, INTBIG liblistlen, void *dia)
{
	REGISTER INTBIG i;

	DiaLoadTextDialog(dia, DTED_DEPENDENTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	if (liblistlen == 0)
	{
		DiaSelectLine(dia, DTED_DEPENDENTLIST, -1);
		return;
	}
	for(i=0; i<liblistlen; i++) DiaStuffLine(dia, DTED_DEPENDENTLIST, liblist[i]);
	DiaSelectLine(dia, DTED_DEPENDENTLIST, 0);
}

/****************************** TECHNOLOGY EDIT: VARIABLES DIALOG ******************************/

/* Technology Variables */
static DIALOGITEM us_techvarsdialogitems[] =
{
 /*  1 */ {0, {208,472,232,536}, BUTTON, N_("OK")},
 /*  2 */ {0, {208,376,232,440}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,8,143,264}, SCROLL, x_("")},
 /*  4 */ {0, {176,16,192,55}, MESSAGE, N_("Type:")},
 /*  5 */ {0, {176,56,192,142}, MESSAGE, x_("")},
 /*  6 */ {0, {152,104,168,536}, MESSAGE, x_("")},
 /*  7 */ {0, {24,280,143,536}, SCROLL, x_("")},
 /*  8 */ {0, {8,16,24,240}, MESSAGE, N_("Current Variables on Technology:")},
 /*  9 */ {0, {8,288,24,419}, MESSAGE, N_("Possible Variables:")},
 /* 10 */ {0, {208,280,232,344}, BUTTON, N_("<< Copy")},
 /* 11 */ {0, {208,24,232,88}, BUTTON, N_("Remove")},
 /* 12 */ {0, {176,216,192,533}, EDITTEXT, x_("")},
 /* 13 */ {0, {208,136,232,237}, BUTTON, N_("Edit Strings")},
 /* 14 */ {0, {176,168,192,212}, MESSAGE, N_("Value:")},
 /* 15 */ {0, {152,16,168,98}, MESSAGE, N_("Description:")}
};
static DIALOG us_techvarsdialog = {{50,75,293,622}, N_("Technology Variables"), 0, 15, us_techvarsdialogitems, 0, 0};

static void us_setcurrenttechvar(TECHVAR*, void*);

/* special items for the "technology variables" dialog: */
#define DTEV_CURVARS      3		/* Current vars (scroll) */
#define DTEV_TYPE         5		/* Type (stat text) */
#define DTEV_DESCRIPTION  6		/* Description (stat text) */
#define DTEV_ALLVARS      7		/* Known vars (scroll) */
#define DTEV_COPY        10		/* Copy (button) */
#define DTEV_REMOVE      11		/* Remove (button) */
#define DTEV_VALUE       12		/* the Value (edit text) */
#define DTEV_EDITSTRINGS 13		/* Edit Strings (button) */

INTBIG us_techvarsdlog(void)
{
	CHAR **varnames, *name, *cmd[5];
	INTBIG itemHit, i, j;
	REGISTER VARIABLE *var, *ovar;
	TECHVAR *newvars, *tvar, *ltvar, *t;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* show the "technology variables" dialog */
	dia = DiaInitDialog(&us_techvarsdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DTEV_CURVARS, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	DiaInitTextDialog(dia, DTEV_ALLVARS, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);

	/* load the known variables list */
	for(i=0; us_knownvars[i].varname != 0; i++)
		DiaStuffLine(dia, DTEV_ALLVARS, us_knownvars[i].varname);
	DiaSelectLine(dia, DTEV_ALLVARS, -1);

	/* see what variables are already in the list */
	var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_variable_list"));
	newvars = NOTECHVAR;
	if (var != NOVARIABLE)
	{
		j = getlength(var);
		varnames = (CHAR **)var->addr;
		for(i=0; i<j; i++)
		{
			ovar = getval((INTBIG)el_curlib, VLIBRARY, -1, varnames[i]);
			if (ovar == NOVARIABLE) continue;
			DiaStuffLine(dia, DTEV_CURVARS, varnames[i]);
			tvar = (TECHVAR *)emalloc(sizeof (TECHVAR), el_tempcluster);
			if (tvar == 0) break;
			(void)allocstring(&tvar->varname, varnames[i], el_tempcluster);
			tvar->nexttechvar = newvars;
			tvar->changed = FALSE;
			switch (ovar->type&(VTYPE|VISARRAY))
			{
				case VFLOAT:   tvar->fval = castfloat(ovar->addr);   break;
				case VINTEGER: tvar->ival = ovar->addr;              break;
				case VSTRING:
					(void)allocstring(&tvar->sval, (CHAR *)ovar->addr, el_tempcluster);
					break;
			}
			tvar->vartype = ovar->type;
			newvars = tvar;
		}
	}
	DiaSelectLine(dia, DTEV_CURVARS, -1);

	/* set dialog allowances state */
	us_setcurrenttechvar(newvars, dia);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL ||
			itemHit == DTEV_EDITSTRINGS) break;

		/* hit in one scroll area turns off highlight in the other */
		if (itemHit == DTEV_CURVARS)
		{
			DiaSelectLine(dia, DTEV_ALLVARS, -1);
			us_setcurrenttechvar(newvars, dia);
		}
		if (itemHit == DTEV_ALLVARS)
		{
			DiaSelectLine(dia, DTEV_CURVARS, -1);
			us_setcurrenttechvar(newvars, dia);
		}
		/* change to the value */
		if (itemHit == DTEV_VALUE)
		{
			i = DiaGetCurLine(dia, DTEV_CURVARS);
			if (i < 0) continue;
			name = DiaGetScrollLine(dia, DTEV_CURVARS, i);
			for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
				if (namesame(t->varname, name) == 0) break;
			if (t == NOTECHVAR) continue;
			switch (t->vartype&(VTYPE|VISARRAY))
			{
				case VINTEGER:
					t->ival = myatoi(DiaGetText(dia, DTEV_VALUE));
					t->changed = TRUE;
					break;
				case VFLOAT:
					t->fval = (float)eatof(DiaGetText(dia, DTEV_VALUE));
					t->changed = TRUE;
					break;
				case VSTRING:
					(void)reallocstring(&t->sval, DiaGetText(dia, DTEV_VALUE), el_tempcluster);
					t->changed = TRUE;
					break;
			}
			continue;
		}

		/* the "<< Copy" button */
		if (itemHit == DTEV_COPY)
		{
			i = DiaGetCurLine(dia, DTEV_ALLVARS);
			if (i < 0) continue;
			name = DiaGetScrollLine(dia, DTEV_ALLVARS, i);
			for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
				if (namesame(t->varname, name) == 0) break;
			if (t != NOTECHVAR) continue;

			tvar = (TECHVAR *)emalloc(sizeof (TECHVAR), el_tempcluster);
			if (tvar == 0) break;
			(void)allocstring(&tvar->varname, name, el_tempcluster);
			tvar->vartype = us_knownvars[i].vartype;
			tvar->ival = 0;
			tvar->fval = 0.0;
			if ((tvar->vartype&(VTYPE|VISARRAY)) == VSTRING)
				(void)allocstring(&tvar->sval, x_(""), el_tempcluster);
			tvar->changed = TRUE;
			tvar->nexttechvar = newvars;
			newvars = tvar;
			DiaLoadTextDialog(dia, DTEV_CURVARS, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
			for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
				DiaStuffLine(dia, DTEV_CURVARS, t->varname);
			DiaSelectLine(dia, DTEV_CURVARS, -1);
			us_setcurrenttechvar(newvars, dia);
			continue;
		}

		/* the "Remove" button */
		if (itemHit == DTEV_REMOVE)
		{
			i = DiaGetCurLine(dia, DTEV_CURVARS);
			if (i < 0) continue;
			name = DiaGetScrollLine(dia, DTEV_CURVARS, i);

			ltvar = NOTECHVAR;
			for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
			{
				if (namesame(t->varname, name) == 0) break;
				ltvar = t;
			}
			if (t == NOTECHVAR) continue;
			if (ltvar == NOTECHVAR) newvars = t->nexttechvar; else
				ltvar->nexttechvar = t->nexttechvar;
			if ((t->vartype&(VTYPE|VISARRAY)) == VSTRING) efree(t->sval);
			efree(t->varname);
			efree((CHAR *)t);
			DiaLoadTextDialog(dia, DTEV_CURVARS, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
			for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
				DiaStuffLine(dia, DTEV_CURVARS, t->varname);
			DiaSelectLine(dia, DTEV_CURVARS, -1);
			us_setcurrenttechvar(newvars, dia);
			continue;
		}
	}
	if (itemHit == DTEV_EDITSTRINGS)
	{
		i = DiaGetCurLine(dia, DTEV_CURVARS);
		if (i < 0) itemHit = OK; else
			name = DiaGetScrollLine(dia, DTEV_CURVARS, i);
	}
	DiaDoneDialog(dia);
	if (itemHit == OK || itemHit == DTEV_EDITSTRINGS)
	{
		j = 0;
		for(t = newvars; t != NOTECHVAR; t = t->nexttechvar) j++;
		if (j > 0)
		{
			varnames = (CHAR **)emalloc(j * (sizeof (CHAR *)), el_tempcluster);
			if (varnames == 0) return(0);
			j = 0;
			for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
			{
				(void)allocstring(&varnames[j], t->varname, el_tempcluster);
				j++;
				if (!t->changed) continue;
				switch (t->vartype&(VTYPE|VISARRAY))
				{
					case VINTEGER:
						(void)setval((INTBIG)el_curlib, VLIBRARY, t->varname,
							t->ival, VINTEGER);
						break;
					case VFLOAT:
						(void)setval((INTBIG)el_curlib, VLIBRARY, t->varname,
							castint(t->fval), VFLOAT);
						break;
					case VSTRING:
						(void)setval((INTBIG)el_curlib, VLIBRARY, t->varname,
							(INTBIG)t->sval, VSTRING);
						break;
					case VSTRING|VISARRAY:
						cmd[0] = x_("EMPTY");
						(void)setval((INTBIG)el_curlib, VLIBRARY, t->varname,
							(INTBIG)cmd, VSTRING|VISARRAY|(1<<VLENGTHSH));
						break;
				}
			}
			(void)setval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_variable_list"),
				(INTBIG)varnames, VSTRING|VISARRAY|(j<<VLENGTHSH));
			for(i=0; i<j; i++) efree(varnames[i]);
			efree((CHAR *)varnames);
		} else
		{
			if (getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_variable_list")) != NOVARIABLE)
				(void)delval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_variable_list"));
		}
	}
	if (itemHit == DTEV_EDITSTRINGS)
	{
		cmd[0] = x_("textedit");
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("lib:~."));
		addstringtoinfstr(infstr, name);
		(void)allocstring(&cmd[1], returninfstr(infstr), el_tempcluster);
		cmd[2] = x_("header");
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Editing technology variable: "));
		addstringtoinfstr(infstr, name);
		(void)allocstring(&cmd[3], returninfstr(infstr), el_tempcluster);
		us_var(4, cmd);
		efree(cmd[1]);
		efree(cmd[3]);
	}
	return(0);
}

void us_setcurrenttechvar(TECHVAR *newvars, void *dia)
{
	TECHVAR *t;
	INTBIG i;
	CHAR line[20], *name;

	DiaDimItem(dia, DTEV_EDITSTRINGS);
	DiaSetText(dia, DTEV_VALUE, x_(""));
	DiaNoEditControl(dia, DTEV_VALUE);
	DiaDimItem(dia, DTEV_REMOVE);
	DiaDimItem(dia, DTEV_COPY);
	DiaSetText(dia, DTEV_TYPE, x_(""));
	DiaSetText(dia, DTEV_DESCRIPTION, x_(""));
	i = DiaGetCurLine(dia, DTEV_CURVARS);
	if (i >= 0)
	{
		DiaUnDimItem(dia, DTEV_REMOVE);
		name = DiaGetScrollLine(dia, DTEV_CURVARS, i);
		for(i=0; us_knownvars[i].varname != 0; i++)
			if (namesame(us_knownvars[i].varname, name) == 0) break;
		if (us_knownvars[i].varname != 0)
			DiaSetText(dia, DTEV_DESCRIPTION, us_knownvars[i].description);
		for(t = newvars; t != NOTECHVAR; t = t->nexttechvar)
			if (namesame(t->varname, name) == 0) break;
		if (t != NOTECHVAR) switch (t->vartype&(VTYPE|VISARRAY))
		{
			case VINTEGER:
				DiaSetText(dia, DTEV_TYPE, _("Integer"));
				DiaEditControl(dia, DTEV_VALUE);
				(void)esnprintf(line, 20, x_("%ld"), t->ival);
				DiaSetText(dia, -DTEV_VALUE, line);
				break;
			case VFLOAT:
				DiaSetText(dia, DTEV_TYPE, _("Real"));
				DiaEditControl(dia, DTEV_VALUE);
				(void)esnprintf(line, 20, x_("%g"), t->fval);
				DiaSetText(dia, -DTEV_VALUE, line);
				break;
			case VSTRING:
				DiaSetText(dia, DTEV_TYPE, _("String"));
				DiaEditControl(dia, DTEV_VALUE);
				DiaSetText(dia, -DTEV_VALUE, t->sval);
				break;
			case VSTRING|VISARRAY:
				DiaSetText(dia, DTEV_TYPE, _("Strings"));
				DiaUnDimItem(dia, DTEV_EDITSTRINGS);
				break;
		}
	}

	i = DiaGetCurLine(dia, DTEV_ALLVARS);
	if (i >= 0)
	{
		name = DiaGetScrollLine(dia, DTEV_ALLVARS, i);
		for(i=0; us_knownvars[i].varname != 0; i++)
			if (namesame(us_knownvars[i].varname, name) == 0) break;
		if (us_knownvars[i].varname != 0)
		{
			DiaSetText(dia, DTEV_DESCRIPTION, us_knownvars[i].description);
			switch (us_knownvars[i].vartype&(VTYPE|VISARRAY))
			{
				case VINTEGER:         DiaSetText(dia, DTEV_TYPE, _("Integer"));   break;
				case VFLOAT:           DiaSetText(dia, DTEV_TYPE, _("Real"));      break;
				case VSTRING:          DiaSetText(dia, DTEV_TYPE, _("String"));    break;
				case VSTRING|VISARRAY: DiaSetText(dia, DTEV_TYPE, _("Strings"));   break;
			}
		}
		DiaUnDimItem(dia, DTEV_COPY);
	}
}

/****************************** TECHNOLOGY SELECTION DIALOG ******************************/

/* Technologies */
static DIALOGITEM us_techselectdialogitems[] =
{
 /*  1 */ {0, {96,216,120,280}, BUTTON, N_("OK")},
 /*  2 */ {0, {24,216,48,280}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,153,193}, SCROLL, x_("")},
 /*  4 */ {0, {160,8,208,292}, MESSAGE, x_("")}
};
static DIALOG us_techselectdialog = {{50,75,267,376}, N_("Change Current Technology"), 0, 4, us_techselectdialogitems, 0, 0};

/* special items for the "select technology" dialog: */
#define DSLT_TECHLIST     3		/* Technology list (scroll) */
#define DSLT_DESCRIPTION  4		/* Technology description (stat text) */

/*
 * the meaning of "us_techlist":
 *  0  list all technologies without modification
 *  1  list all technologies, splitting "schematics" into "digital" and "analog"
 *  2  list all technologies that can be edited
 *  3  list all technologies that can be deleted
 */
static INTBIG      us_techlist;
static TECHNOLOGY *us_postechcomcomp;

static void    us_stufftechdescript(void *dia);
static BOOLEAN us_topoftechs(CHAR**);
static CHAR   *us_nexttechs(void);

BOOLEAN us_topoftechs(CHAR **c) { Q_UNUSED( c ); us_postechcomcomp = el_technologies; return(TRUE); }

CHAR *us_nexttechs(void)
{
	REGISTER CHAR *retname;
	REGISTER TECHNOLOGY *tech;

	for(;;)
	{
		if (us_postechcomcomp == NOTECHNOLOGY)
		{
			us_postechcomcomp = 0;
			if (us_techlist == 1) return(x_("schematic, analog"));
		}
		if (us_postechcomcomp == 0) return(0);

		/* get the next technology in the list */
		tech = us_postechcomcomp;
		us_postechcomcomp = us_postechcomcomp->nexttechnology;

		/* adjust the name if requested */
		retname = tech->techname;
		if (tech == sch_tech && us_techlist == 1)
			retname = x_("schematic, digital");

		/* ignore if requested */
		if (us_techlist == 2 && (tech->userbits&NONSTANDARD) != 0) continue;
		if (us_techlist == 3 && tech == gen_tech) continue;

		/* accept */
		break;
	}
	return(retname);
}

void us_stufftechdescript(void *dia)
{
	REGISTER TECHNOLOGY *t;
	REGISTER CHAR *tech;

	tech = DiaGetScrollLine(dia, DSLT_TECHLIST, DiaGetCurLine(dia, DSLT_TECHLIST));
	if (namesamen(tech, x_("schematic"), 9) == 0)
	{
		t = sch_tech;
		DiaSetText(dia, DSLT_DESCRIPTION, t->techdescript);
		return;
	}

	for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
		if (estrcmp(t->techname, tech) == 0) break;
	if (t == NOTECHNOLOGY) return;
	DiaSetText(dia, DSLT_DESCRIPTION, t->techdescript);
}

INTBIG us_technologydlog(CHAR *prompt, CHAR *paramstart[])
{
	REGISTER INTBIG itemHit, i, listlen;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *defaulttech, *pt;
	REGISTER void *dia;

	/* display the new cell dialog box */
	us_techselectdialog.movable = prompt;

	/* the list of technologies depends on the nature of the operation */
	us_techlist = 0;
	defaulttech = x_("");
	if (namesame(prompt, _("Change current technology")) == 0)
	{
		us_techlist = 1;
		np = getcurcell();
		if (np != NONODEPROTO)
			defaulttech = us_techname(np);
	} else if (namesame(prompt, _("Edit technology")) == 0) 
	{
		us_techlist = 2;
		defaulttech = el_curtech->techname;
	} else if (namesame(prompt, _("Document technology")) == 0)
	{
		us_techlist = 0;
		defaulttech = el_curtech->techname;
	} else if (namesame(prompt, _("Convert to new technology")) == 0) 
	{
		us_techlist = 0;
	} else if (namesame(prompt, _("Delete technology")) == 0)
	{
		us_techlist = 3;
	}

	dia = DiaInitDialog(&us_techselectdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DSLT_TECHLIST, us_topoftechs, us_nexttechs, DiaNullDlogDone, 0,
		SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT|SCREPORT);
	listlen = DiaGetNumScrollLines(dia, DSLT_TECHLIST);
	for(i=0; i<listlen; i++)
	{
		pt = DiaGetScrollLine(dia, DSLT_TECHLIST, i);
		if (estrcmp(pt, defaulttech) == 0)
		{
			DiaSelectLine(dia, DSLT_TECHLIST, i);
			break;
		}
	}
	us_stufftechdescript(dia);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
		if (itemHit == DSLT_TECHLIST)
		{
			us_stufftechdescript(dia);
			continue;
		}
	}

	paramstart[0] = x_("");
	if (itemHit != CANCEL)
	{
		if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
		allocstring(&us_returneddialogstring, DiaGetScrollLine(dia, DSLT_TECHLIST,
			DiaGetCurLine(dia, DSLT_TECHLIST)), us_tool->cluster);
		paramstart[0] = us_returneddialogstring;
	}
	DiaDoneDialog(dia);
	return(1);
}

/****************************** TECHNOLOGY OPTIONS DIALOG ******************************/

/* Technology Options */
static DIALOGITEM us_techsetdialogitems[] =
{
 /*  1 */ {0, {240,328,264,392}, BUTTON, N_("OK")},
 /*  2 */ {0, {240,244,264,308}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,20,48,156}, MESSAGE, N_("Metal layers:")},
 /*  4 */ {0, {32,164,48,312}, POPUP, x_("")},
 /*  5 */ {0, {216,20,232,156}, RADIO, N_("Full Geometry")},
 /*  6 */ {0, {216,164,232,300}, RADIO, N_("Stick Figures")},
 /*  7 */ {0, {88,320,104,424}, MESSAGE, N_("Artwork:")},
 /*  8 */ {0, {112,332,128,468}, CHECK, N_("Arrows filled")},
 /*  9 */ {0, {144,320,160,424}, MESSAGE, N_("Schematics:")},
 /* 10 */ {0, {168,332,184,492}, MESSAGE, N_("Negating Bubble Size")},
 /* 11 */ {0, {168,496,184,556}, EDITTEXT, x_("")},
 /* 12 */ {0, {8,8,24,192}, MESSAGE, N_("MOSIS CMOS:")},
 /* 13 */ {0, {136,320,137,624}, DIVIDELINE, x_("")},
 /* 14 */ {0, {4,316,228,317}, DIVIDELINE, x_("")},
 /* 15 */ {0, {144,20,160,260}, CHECK, N_("Disallow stacked vias")},
 /* 16 */ {0, {56,32,72,300}, RADIO, N_("SCMOS rules (4 metal or less)")},
 /* 17 */ {0, {76,32,92,300}, RADIO, N_("Submicron rules")},
 /* 18 */ {0, {96,32,112,300}, RADIO, N_("Deep rules (5 metal or more)")},
 /* 19 */ {0, {168,20,184,300}, CHECK, N_("Alternate Active and Poly contact rules")},
 /* 20 */ {0, {32,332,48,468}, MESSAGE, N_("Metal layers:")},
 /* 21 */ {0, {32,476,48,612}, POPUP, x_("")},
 /* 22 */ {0, {8,320,24,584}, MESSAGE, N_("MOSIS CMOS Submicron (old):")},
 /* 23 */ {0, {80,320,81,624}, DIVIDELINE, x_("")},
 /* 24 */ {0, {56,332,72,624}, CHECK, N_("Automatically convert to new MOSIS CMOS")},
 /* 25 */ {0, {120,20,136,300}, CHECK, N_("Second Polysilicon layer")},
 /* 26 */ {0, {192,332,208,624}, MESSAGE, N_("Use Lambda values from this Technology:")},
 /* 27 */ {0, {212,388,228,552}, POPUP, x_("")},
 /* 28 */ {0, {192,20,208,300}, CHECK, N_("Show Special transistors")}
};
static DIALOG us_techsetdialog = {{75,75,348,708}, N_("Technology Options"), 0, 28, us_techsetdialogitems, 0, 0};

/* special items for the "Technology Options" dialog: */
#define DTHO_MOCMOS_METALS       4		/* MOCMOS metal layers (popup) */
#define DTHO_MOCMOS_FULLGEOM     5		/* MOCMOS full-geometry (radio) */
#define DTHO_MOCMOS_STICKS       6		/* MOCMOS stick-figure (radio) */
#define DTHO_ARTWORK_ARROWS      8		/* ARTWORK filled arrow (check) */
#define DTHO_SCHEM_BUBBLESIZE   11		/* SCHEMATICS invert bubble (edit text) */
#define DTHO_MOCMOS_STACKVIA    15		/* MOCMOS stacked vias (check) */
#define DTHO_MOCMOS_SCMOS       16		/* MOCMOS SCMOS rules (radio) */
#define DTHO_MOCMOS_SUBM        17		/* MOCMOS submicron rules (radio) */
#define DTHO_MOCMOS_DEEP        18		/* MOCMOS deep rules (radio) */
#define DTHO_MOCMOS_ALTCONT     19		/* MOCMOS alternate act/poly (check) */
#define DTHO_MOCMOSSUB_METALS   21		/* MOCMOSSUB metal layers (popup) */
#define DTHO_MOCMOSSUB_CONVERT  24		/* MOCMOSSUB convert technology (check) */
#define DTHO_MOCMOS_TWOPOLY     25		/* MOCMOS second polysilicon layer (check) */
#define DTHO_SCHEM_LAMBDATECH   27		/* SCHEMATICS tech to use for lambda (popup) */
#define DTHO_MOCMOS_SPECTRAN    28		/* MOCMOS use special transistors (check) */

INTBIG us_techoptdlog(void)
{
	REGISTER INTBIG itemHit, which, i, mocmosbits, mocmossubbits, artbits,
		origmocmosbits, origmocmossubbits, origartbits, schbubblesize, origschbubblesize,
		*oldnodewidthoffset, len;
	REGISTER INTBIG techcount;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *inischemtech, *tech;
	WINDOWPART *w;
	CHAR *newlang[5], **techlist;
	static CHAR *metalcount[] = {N_("2 Layers"), N_("3 Layers"), N_("4 Layers"),
		N_("5 Layers"), N_("6 Layers")};
	CHAR *par[2];
	REGISTER void *dia;

	/* display the options dialog box */
	dia = DiaInitDialog(&us_techsetdialog);
	if (dia == 0) return(0);
	for(i=0; i<5; i++) newlang[i] = TRANSLATE(metalcount[i]);
	DiaSetPopup(dia, DTHO_MOCMOS_METALS, 5, newlang);
	DiaSetPopup(dia, DTHO_MOCMOSSUB_METALS, 5, newlang);

	/* get state of "mocmossub" technology */
	origmocmossubbits = mocmossubbits = asktech(mocmossub_tech, x_("get-state"));
	switch (mocmossubbits&MOCMOSSUBMETALS)
	{
		case MOCMOSSUB2METAL: DiaSetPopupEntry(dia, DTHO_MOCMOSSUB_METALS, 0);   break;
		case MOCMOSSUB3METAL: DiaSetPopupEntry(dia, DTHO_MOCMOSSUB_METALS, 1);   break;
		case MOCMOSSUB4METAL: DiaSetPopupEntry(dia, DTHO_MOCMOSSUB_METALS, 2);   break;
		case MOCMOSSUB5METAL: DiaSetPopupEntry(dia, DTHO_MOCMOSSUB_METALS, 3);   break;
		case MOCMOSSUB6METAL: DiaSetPopupEntry(dia, DTHO_MOCMOSSUB_METALS, 4);   break;
	}
	if ((mocmossubbits&MOCMOSSUBNOCONV) == 0) DiaSetControl(dia, DTHO_MOCMOSSUB_CONVERT, 1);

	/* get state of "mocmos" technology */
	origmocmosbits = mocmosbits = asktech(mocmos_tech, x_("get-state"));
	DiaUnDimItem(dia, DTHO_MOCMOS_DEEP);
	DiaUnDimItem(dia, DTHO_MOCMOS_SCMOS);
	DiaUnDimItem(dia, DTHO_MOCMOS_TWOPOLY);
	switch (mocmosbits&MOCMOSMETALS)
	{
		case MOCMOS2METAL:
			DiaSetPopupEntry(dia, DTHO_MOCMOS_METALS, 0);
			DiaDimItem(dia, DTHO_MOCMOS_DEEP);
			break;
		case MOCMOS3METAL:
			DiaSetPopupEntry(dia, DTHO_MOCMOS_METALS, 1);
			DiaDimItem(dia, DTHO_MOCMOS_DEEP);
			break;
		case MOCMOS4METAL:
			DiaSetPopupEntry(dia, DTHO_MOCMOS_METALS, 2);
			DiaDimItem(dia, DTHO_MOCMOS_DEEP);
			break;
		case MOCMOS5METAL:
			DiaSetPopupEntry(dia, DTHO_MOCMOS_METALS, 3);
			DiaDimItem(dia, DTHO_MOCMOS_SCMOS);
			break;
		case MOCMOS6METAL:
			DiaSetPopupEntry(dia, DTHO_MOCMOS_METALS, 4);
			DiaDimItem(dia, DTHO_MOCMOS_SCMOS);
			break;
	}
	if ((mocmosbits&MOCMOSNOSTACKEDVIAS) != 0) DiaSetControl(dia, DTHO_MOCMOS_STACKVIA, 1);
	if ((mocmosbits&MOCMOSALTAPRULES) != 0) DiaSetControl(dia, DTHO_MOCMOS_ALTCONT, 1);
	if ((mocmosbits&MOCMOSTWOPOLY) != 0) DiaSetControl(dia, DTHO_MOCMOS_TWOPOLY, 1);
	if ((mocmosbits&MOCMOSSPECIALTRAN) != 0) DiaSetControl(dia, DTHO_MOCMOS_SPECTRAN, 1);
	switch (mocmosbits&MOCMOSRULESET)
	{
		case MOCMOSSCMOSRULES:
			DiaSetControl(dia, DTHO_MOCMOS_SCMOS, 1);
			break;
		case MOCMOSSUBMRULES:
			DiaSetControl(dia, DTHO_MOCMOS_SUBM, 1);
			break;
		case MOCMOSDEEPRULES:
			DiaSetControl(dia, DTHO_MOCMOS_DEEP, 1);
			DiaDimItem(dia, DTHO_MOCMOS_TWOPOLY);
			break;
	}
	if ((mocmosbits&MOCMOSSTICKFIGURE) != 0) DiaSetControl(dia, DTHO_MOCMOS_STICKS, 1); else
		DiaSetControl(dia, DTHO_MOCMOS_FULLGEOM, 1);

	/* cache "mocmos" node sizes in case they change and so must layout */
	oldnodewidthoffset = 0;
	var = getval((INTBIG)mocmos_tech, VTECHNOLOGY, VFRACT|VISARRAY, x_("TECH_node_width_offset"));
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		oldnodewidthoffset = (INTBIG *)emalloc(len * SIZEOFINTBIG, el_tempcluster);
		if (oldnodewidthoffset == 0) return(1);
		for(i=0; i<len; i++)
			oldnodewidthoffset[i] = ((INTBIG *)var->addr)[i];
	}

	/* get state of "artwork" technology */
	origartbits = artbits = asktech(art_tech, x_("get-state"));
	if ((artbits&ARTWORKFILLARROWHEADS) != 0) DiaSetControl(dia, DTHO_ARTWORK_ARROWS, 1);

	/* get state of "schematic" technology */
	origschbubblesize = schbubblesize = asktech(sch_tech, x_("get-bubble-size"));
	DiaSetText(dia, DTHO_SCHEM_BUBBLESIZE, frtoa(schbubblesize));
	techcount = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if ((tech->userbits&(NONELECTRICAL|NOPRIMTECHNOLOGY)) != 0) continue;
		if (tech == sch_tech || tech == gen_tech) continue;
		techcount++;
	}
	techlist = (CHAR **)emalloc(techcount * (sizeof (CHAR *)), el_tempcluster);
	if (techlist == 0) return(1);
	techcount = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if ((tech->userbits&(NONELECTRICAL|NOPRIMTECHNOLOGY)) != 0) continue;
		if (tech == sch_tech || tech == gen_tech) continue;
		techlist[techcount] = tech->techname;
		tech->temp1 = techcount;
		techcount++;
	}
	DiaSetPopup(dia, DTHO_SCHEM_LAMBDATECH, techcount, techlist);
	inischemtech = defschematictechnology(el_curtech);
	if (inischemtech != NOTECHNOLOGY)
		DiaSetPopupEntry(dia, DTHO_SCHEM_LAMBDATECH, inischemtech->temp1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DTHO_MOCMOS_METALS)
		{
			i = DiaGetPopupEntry(dia, DTHO_MOCMOS_METALS);
			if (i <= 2)
			{
				/* 4 metal layers or less */
				if (DiaGetControl(dia, DTHO_MOCMOS_DEEP) != 0)
				{
					DiaSetControl(dia, DTHO_MOCMOS_DEEP, 0);
					DiaSetControl(dia, DTHO_MOCMOS_SUBM, 1);
				}
				DiaUnDimItem(dia, DTHO_MOCMOS_SCMOS);
				DiaDimItem(dia, DTHO_MOCMOS_DEEP);
			} else
			{
				/* 5 metal layers or more */
				if (DiaGetControl(dia, DTHO_MOCMOS_SCMOS) != 0)
				{
					DiaSetControl(dia, DTHO_MOCMOS_SCMOS, 0);
					DiaSetControl(dia, DTHO_MOCMOS_SUBM, 1);
				}
				DiaDimItem(dia, DTHO_MOCMOS_SCMOS);
				DiaUnDimItem(dia, DTHO_MOCMOS_DEEP);
			}
			continue;
		}
		if (itemHit == DTHO_MOCMOS_SCMOS || itemHit == DTHO_MOCMOS_SUBM ||
			itemHit == DTHO_MOCMOS_DEEP)
		{
			DiaSetControl(dia, DTHO_MOCMOS_SCMOS, 0);
			DiaSetControl(dia, DTHO_MOCMOS_SUBM, 0);
			DiaSetControl(dia, DTHO_MOCMOS_DEEP, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DTHO_MOCMOS_DEEP)
			{
				DiaSetControl(dia, DTHO_MOCMOS_TWOPOLY, 0);
				DiaDimItem(dia, DTHO_MOCMOS_TWOPOLY);
			} else
			{
				DiaUnDimItem(dia, DTHO_MOCMOS_TWOPOLY);
			}
			continue;
		}
		if (itemHit == DTHO_MOCMOS_FULLGEOM || itemHit == DTHO_MOCMOS_STICKS)
		{
			DiaSetControl(dia, DTHO_MOCMOS_FULLGEOM, 0);
			DiaSetControl(dia, DTHO_MOCMOS_STICKS, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DTHO_ARTWORK_ARROWS || itemHit == DTHO_MOCMOS_STACKVIA ||
			itemHit == DTHO_MOCMOS_SCMOS || itemHit == DTHO_MOCMOS_ALTCONT || 
			itemHit == DTHO_MOCMOSSUB_CONVERT || itemHit == DTHO_MOCMOS_TWOPOLY ||
			itemHit == DTHO_MOCMOS_SPECTRAN)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit == OK)
	{
		/* update "mocmossub" technology */
		which = DiaGetPopupEntry(dia, DTHO_MOCMOSSUB_METALS);
		mocmossubbits &= ~MOCMOSSUBMETALS;
		switch (which)
		{
			case 0: mocmossubbits |= MOCMOSSUB2METAL;   break;
			case 1: mocmossubbits |= MOCMOSSUB3METAL;   break;
			case 2: mocmossubbits |= MOCMOSSUB4METAL;   break;
			case 3: mocmossubbits |= MOCMOSSUB5METAL;   break;
			case 4: mocmossubbits |= MOCMOSSUB6METAL;   break;
		}
		if (DiaGetControl(dia, DTHO_MOCMOSSUB_CONVERT) == 0) mocmossubbits |= MOCMOSSUBNOCONV; else
			mocmossubbits &= ~MOCMOSSUBNOCONV;
		if (origmocmossubbits != mocmossubbits)
		{
			setvalkey((INTBIG)mocmossub_tech, VTECHNOLOGY, el_techstate_key, mocmossubbits, VINTEGER);
			if (el_curtech == mocmossub_tech)
			{
				par[0] = x_("size");
				par[1] = x_("auto");
				us_menu(2, par);
				us_setmenunodearcs();
			}
		}

		/* update "mocmos" technology */
		which = DiaGetPopupEntry(dia, DTHO_MOCMOS_METALS);
		mocmosbits &= ~MOCMOSMETALS;
		switch (which)
		{
			case 0: mocmosbits |= MOCMOS2METAL;   break;
			case 1: mocmosbits |= MOCMOS3METAL;   break;
			case 2: mocmosbits |= MOCMOS4METAL;   break;
			case 3: mocmosbits |= MOCMOS5METAL;   break;
			case 4: mocmosbits |= MOCMOS6METAL;   break;
		}
		if (DiaGetControl(dia, DTHO_MOCMOS_STICKS) != 0) mocmosbits |= MOCMOSSTICKFIGURE; else
			mocmosbits &= ~MOCMOSSTICKFIGURE;
		if (DiaGetControl(dia, DTHO_MOCMOS_STACKVIA) != 0) mocmosbits |= MOCMOSNOSTACKEDVIAS; else
			mocmosbits &= ~MOCMOSNOSTACKEDVIAS;
		if (DiaGetControl(dia, DTHO_MOCMOS_ALTCONT) != 0) mocmosbits |= MOCMOSALTAPRULES; else
			mocmosbits &= ~MOCMOSALTAPRULES;
		if (DiaGetControl(dia, DTHO_MOCMOS_TWOPOLY) != 0) mocmosbits |= MOCMOSTWOPOLY; else
			mocmosbits &= ~MOCMOSTWOPOLY;
		if (DiaGetControl(dia, DTHO_MOCMOS_SPECTRAN) != 0) mocmosbits |= MOCMOSSPECIALTRAN; else
			mocmosbits &= ~MOCMOSSPECIALTRAN;
		mocmosbits &= ~MOCMOSRULESET;
		if (DiaGetControl(dia, DTHO_MOCMOS_SCMOS) != 0) mocmosbits |= MOCMOSSCMOSRULES; else
			if (DiaGetControl(dia, DTHO_MOCMOS_SUBM) != 0) mocmosbits |= MOCMOSSUBMRULES; else
				if (DiaGetControl(dia, DTHO_MOCMOS_DEEP) != 0) mocmosbits |= MOCMOSDEEPRULES;
		if (origmocmosbits != mocmosbits)
		{
			setvalkey((INTBIG)mocmos_tech, VTECHNOLOGY, el_techstate_key, mocmosbits, VINTEGER);
			if ((origmocmosbits&MOCMOSSTICKFIGURE) != (mocmosbits&MOCMOSSTICKFIGURE))
				us_figuretechopaque(mocmos_tech);
			if (el_curtech == mocmos_tech)
			{
				par[0] = x_("size");
				par[1] = x_("auto");
				us_menu(2, par);
			}
		}

		/* see if change to "mocmos" options causes node sizes to change */
		if (oldnodewidthoffset != 0)
		{
			REGISTER INTBIG lambda, *newnodewidthoffset, index, dlx, dhx, dly, dhy;
			REGISTER NODEINST *ni;
			INTBIG sx, sy;
			REGISTER NODEPROTO *np;
			REGISTER LIBRARY *lib;

			var = getval((INTBIG)mocmos_tech, VTECHNOLOGY, VFRACT|VISARRAY, x_("TECH_node_width_offset"));
			if (var != NOVARIABLE)
			{
				len = getlength(var);
				newnodewidthoffset = (INTBIG *)var->addr;
				for(i=0; i<len; i++)
					if (oldnodewidthoffset[i] != newnodewidthoffset[i]) break;
				if (i < len)
				{
					/* node sizes changed */
					for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
					{
						for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
						{
							for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
							{
								if (ni->proto->primindex == 0) continue;
								if (ni->proto->tech != mocmos_tech) continue;
								/* ignore if default size */
								defaultnodesize(ni->proto, &sx, &sy);
								if (sx == ni->highx - ni->lowx &&
									sy == ni->highy - ni->lowy) continue;
								index = (ni->proto->primindex - 1) * 4;
								if (oldnodewidthoffset[index] == newnodewidthoffset[index] &&
									oldnodewidthoffset[index+1] == newnodewidthoffset[index+1] &&
									oldnodewidthoffset[index+2] == newnodewidthoffset[index+2] &&
									oldnodewidthoffset[index+3] == newnodewidthoffset[index+3])
										continue;
								lambda = lib->lambda[mocmos_tech->techindex];
								dlx = (newnodewidthoffset[index] - oldnodewidthoffset[index]) * lambda / WHOLE;
								dhx = (newnodewidthoffset[index+1] - oldnodewidthoffset[index+1]) * lambda / WHOLE;
								dly = (newnodewidthoffset[index+2] - oldnodewidthoffset[index+2]) * lambda / WHOLE;
								dhy = (newnodewidthoffset[index+3] - oldnodewidthoffset[index+3]) * lambda / WHOLE;
								drcminnodesize(ni->proto, lib, &sx, &sy, 0);

								/* only scale an axis if it is larger than default */
								if (ni->highx - ni->lowx <= sx) dlx = dhx = 0;
								if (ni->highy - ni->lowy <= sy) dly = dhy = 0;

								startobjectchange((INTBIG)ni, VNODEINST);
								modifynodeinst(ni, -dlx, -dly, dhx, dhy, 0, 0);
								endobjectchange((INTBIG)ni, VNODEINST);
							}
						}
					}
				}
			}
			efree((CHAR *)oldnodewidthoffset);
		}

		/* update "artwork" technology */
		if (DiaGetControl(dia, DTHO_ARTWORK_ARROWS) != 0) artbits |= ARTWORKFILLARROWHEADS; else
			artbits &= ~ARTWORKFILLARROWHEADS;
		if (artbits != origartbits)
		{
			setvalkey((INTBIG)art_tech, VTECHNOLOGY, el_techstate_key, artbits, VINTEGER);
			if (el_curtech == art_tech)
				us_drawmenu(0, NOWINDOWFRAME);
		}

		/* update "schematic" technology */
		schbubblesize = atofr(DiaGetText(dia, DTHO_SCHEM_BUBBLESIZE));
		if (schbubblesize != origschbubblesize)
			(void)asktech(sch_tech, x_("set-bubble-size"), schbubblesize);
		i = DiaGetPopupEntry(dia, DTHO_SCHEM_LAMBDATECH);
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			if (namesame(tech->techname, techlist[i]) == 0) break;
		if (tech != NOTECHNOLOGY && tech == inischemtech) tech = NOTECHNOLOGY;
		if (tech != NOTECHNOLOGY)
			setval((INTBIG)sch_tech, VTECHNOLOGY, x_("TECH_layout_technology"),
				(INTBIG)tech->techname, VSTRING);
		efree((CHAR *)techlist);

		/* redisplay all windows if anything changed */
		if (origmocmosbits != mocmosbits || origmocmossubbits != mocmossubbits ||
			origartbits != artbits || origschbubblesize != schbubblesize)
		{
			us_pushhighlight();
			us_clearhighlightcount();
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if (w->redisphandler != 0) (*w->redisphandler)(w);
			us_pophighlight(FALSE);
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

/****************************** TEXT MODIFICATION DIALOG ******************************/

/* Text: Modify */
static DIALOGITEM us_txtmodsizedialogitems[] =
{
 /*  1 */ {0, {200,376,224,456}, BUTTON, N_("OK")},
 /*  2 */ {0, {160,376,184,456}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {156,180,172,340}, RADIO, N_("Points (max 63)")},
 /*  4 */ {0, {156,124,172,172}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,8,24,256}, CHECK, N_("Change size of node text")},
 /*  6 */ {0, {28,8,44,260}, CHECK, N_("Change size of arc text")},
 /*  7 */ {0, {48,8,64,260}, CHECK, N_("Change size of export text")},
 /*  8 */ {0, {88,8,104,260}, CHECK, N_("Change size of instance name text")},
 /*  9 */ {0, {68,8,84,260}, CHECK, N_("Change size of nonlayout text")},
 /* 10 */ {0, {16,264,32,476}, RADIO, N_("Change only selected objects")},
 /* 11 */ {0, {36,264,52,476}, RADIO, N_("Change all in this cell")},
 /* 12 */ {0, {96,264,112,476}, RADIO, N_("Change all in this library")},
 /* 13 */ {0, {180,180,196,340}, RADIO, N_("Lambda (max 127.75)")},
 /* 14 */ {0, {180,124,196,172}, EDITTEXT, x_("")},
 /* 15 */ {0, {204,68,220,152}, MESSAGE, N_("Text font:")},
 /* 16 */ {0, {204,156,220,336}, POPUP, x_("")},
 /* 17 */ {0, {228,68,244,140}, CHECK, N_("Italic")},
 /* 18 */ {0, {228,168,244,232}, CHECK, N_("Bold")},
 /* 19 */ {0, {228,256,244,336}, CHECK, N_("Underline")},
 /* 20 */ {0, {168,68,184,116}, MESSAGE, N_("Size")},
 /* 21 */ {0, {108,8,124,260}, CHECK, N_("Change size of cell text")},
 /* 22 */ {0, {56,264,72,476}, RADIO, N_("Change all cells with view:")},
 /* 23 */ {0, {76,304,92,476}, POPUP, x_("")},
 /* 24 */ {0, {132,8,148,476}, MESSAGE, x_("")}
};
static DIALOG us_txtmodsizedialog = {{75,75,328,561}, N_("Change Text Size"), 0, 24, us_txtmodsizedialogitems, 0, 0};

static void us_settextsize(HIGHLIGHT *high, UINTBIG *descript, INTBIG changenode, INTBIG changearc,
	INTBIG changeexport, INTBIG changenonlayout, INTBIG changeinstance, INTBIG changecell);
static void us_includetextsize(INTBIG size, INTBIG *lowabs, INTBIG *highabs, INTBIG *lowrel, INTBIG *highrel);
static void us_gathertextsize(HIGHLIGHT *high, INTBIG changenode, INTBIG changearc, INTBIG changeexport,
	INTBIG changenonlayout, INTBIG changeinstance, INTBIG changecell,
	INTBIG *lowabs, INTBIG *highabs, INTBIG *lowrel, INTBIG *highrel);
static void us_setcurtextsizes(void *dia, INTBIG changenode, INTBIG changearc, INTBIG changeexport,
	INTBIG changenonlayout, INTBIG changeinstance, INTBIG changecell, INTBIG *lowabs,
	INTBIG *highabs, INTBIG *lowrel, INTBIG *highrel, CHAR **viewlist);

/* special items for "modify text size" dialog: */
#define DMTX_ABSTEXTSIZE_L   3		/* Absolute text size label (radio) */
#define DMTX_ABSTEXTSIZE     4		/* Absolute text size (edit text) */
#define DMTX_NODES           5		/* change node text (check) */
#define DMTX_ARCS            6		/* change arc text (check) */
#define DMTX_EXPORTS         7		/* change export text (check) */
#define DMTX_INSTANCES       8		/* change instance name text (check) */
#define DMTX_NONLAYOUTS      9		/* change nonlayout text (check) */
#define DMTX_SELECTED       10		/* change selected (radio) */
#define DMTX_ALLINCELL      11		/* change all in cell (radio) */
#define DMTX_ALLINLIB       12		/* change all in library (radio) */
#define DMTX_RELTEXTSIZE_L  13		/* Relative text size label (radio) */
#define DMTX_RELTEXTSIZE    14		/* Relative text size (edit text) */
#define DMTX_TEXTFACE       16		/* Text face (popup) */
#define DMTX_TEXTFACE_L     15		/* Text face label (stat text) */
#define DMTX_TEXTITALIC     17		/* Text italic (check) */
#define DMTX_TEXTBOLD       18		/* Text bold (check) */
#define DMTX_TEXTUNDERLINE  19		/* Text underline (check) */
#define DMTX_CELL           21		/* change cell text (check) */
#define DMTX_ALLWITHVIEW    22		/* change all cells with view (radio) */
#define DMTX_WHICHVIEW      23		/* view to select (popup) */
#define DMTX_CURSIZE        24		/* current text sizes (stat text) */

INTBIG us_modtextsizedlog(void)
{
	INTBIG itemHit, i, len, changenode, changearc, changeexport, changenonlayout,
		changeinstance, changecell, viewcount, lowabs, highabs, lowrel, highrel;
	UINTBIG newdescript[TEXTDESCRIPTSIZE];
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np, *cell;
	REGISTER BOOLEAN updaterange;
	REGISTER WINDOWPART *win;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VIEW *v;
	HIGHLIGHT high;
	CHAR **viewlist;
	REGISTER void *dia, *infstr;

	/* display the modify text size dialog box */
	dia = DiaInitDialog(&us_txtmodsizedialog);
	if (dia == 0) return(0);
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	cell = getcurcell();

	/* load the list of views */
	viewcount = us_makeviewlist(&viewlist);
	DiaSetPopup(dia, DMTX_WHICHVIEW, viewcount, viewlist);

	/* set the text size popups */
	if (var == NOVARIABLE) DiaDimItem(dia, DMTX_SELECTED); else
		DiaUnDimItem(dia, DMTX_SELECTED);
	if (cell == NONODEPROTO) DiaDimItem(dia, DMTX_ALLINCELL); else
		DiaUnDimItem(dia, DMTX_ALLINCELL);
	if (var != NOVARIABLE)
	{
		DiaSetControl(dia, DMTX_SELECTED, 1);
	} else if (cell != NONODEPROTO)
	{
		DiaSetControl(dia, DMTX_ALLINCELL, 1);
	} else
	{
		DiaSetControl(dia, DMTX_ALLINLIB, 1);
	}
	if (graphicshas(CANCHOOSEFACES))
	{
		DiaUnDimItem(dia, DMTX_TEXTFACE_L);
		us_setpopupface(DMTX_TEXTFACE, 0, TRUE, dia);
	} else
	{
		DiaDimItem(dia, DMTX_TEXTFACE_L);
	}
	if (graphicshas(CANMODIFYFONTS))
	{
		DiaUnDimItem(dia, DMTX_TEXTITALIC);
		DiaUnDimItem(dia, DMTX_TEXTBOLD);
		DiaUnDimItem(dia, DMTX_TEXTUNDERLINE);
	} else
	{
		DiaDimItem(dia, DMTX_TEXTITALIC);
		DiaDimItem(dia, DMTX_TEXTBOLD);
		DiaDimItem(dia, DMTX_TEXTUNDERLINE);
	}
	DiaSetControl(dia, DMTX_RELTEXTSIZE_L, 1);
	DiaSetText(dia, DMTX_RELTEXTSIZE, x_("1"));
	updaterange = TRUE;
	for(;;)
	{
		if (updaterange)
		{
			changenode = DiaGetControl(dia, DMTX_NODES);
			changearc = DiaGetControl(dia, DMTX_ARCS);
			changeexport = DiaGetControl(dia, DMTX_EXPORTS);
			changenonlayout = DiaGetControl(dia, DMTX_NONLAYOUTS);
			changeinstance = DiaGetControl(dia, DMTX_INSTANCES);
			changecell = DiaGetControl(dia, DMTX_CELL);
			us_setcurtextsizes(dia, changenode, changearc, changeexport, changenonlayout,
				changeinstance, changecell, &lowabs, &highabs, &lowrel, &highrel, viewlist);
			infstr = initinfstr();
			if (lowabs > highabs && lowrel > highrel)
			{
				addstringtoinfstr(infstr, _("No text to change"));
			} else
			{
				addstringtoinfstr(infstr, _("Text runs from"));
				if (lowabs <= highabs)
				{
					formatinfstr(infstr, x_(" %ld to %ld points"), lowabs, highabs);
					if (lowrel <= highrel) addtoinfstr(infstr, ';');
				}
				if (lowrel <= highrel)
					formatinfstr(infstr, x_(" %s to %s lambda"),
						frtoa(lowrel * WHOLE / 4), frtoa(highrel * WHOLE / 4));
			}
			DiaSetText(dia, DMTX_CURSIZE, returninfstr(infstr));
			updaterange = FALSE;
		}
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DMTX_SELECTED || itemHit == DMTX_ALLINCELL ||
			itemHit == DMTX_ALLINLIB || itemHit == DMTX_ALLWITHVIEW)
		{
			DiaSetControl(dia, DMTX_SELECTED, 0);
			DiaSetControl(dia, DMTX_ALLINCELL, 0);
			DiaSetControl(dia, DMTX_ALLINLIB, 0);
			DiaSetControl(dia, DMTX_ALLWITHVIEW, 0);
			DiaSetControl(dia, itemHit, 1);
			updaterange = TRUE;
			continue;
		}
		if (itemHit == DMTX_NODES || itemHit == DMTX_ARCS ||
			itemHit == DMTX_EXPORTS || itemHit == DMTX_NONLAYOUTS ||
			itemHit == DMTX_INSTANCES || itemHit == DMTX_CELL)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			updaterange = TRUE;
			continue;
		}
		if (itemHit == DMTX_RELTEXTSIZE) itemHit = DMTX_RELTEXTSIZE_L;
		if (itemHit == DMTX_ABSTEXTSIZE) itemHit = DMTX_ABSTEXTSIZE_L;
		if (itemHit == DMTX_RELTEXTSIZE_L || itemHit == DMTX_ABSTEXTSIZE_L)
		{
			DiaSetControl(dia, DMTX_RELTEXTSIZE_L, 0);
			DiaSetControl(dia, DMTX_ABSTEXTSIZE_L, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DMTX_TEXTITALIC || itemHit == DMTX_TEXTBOLD ||
			itemHit == DMTX_TEXTUNDERLINE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}
	if (itemHit == OK)
	{
		changenode = DiaGetControl(dia, DMTX_NODES);
		changearc = DiaGetControl(dia, DMTX_ARCS);
		changeexport = DiaGetControl(dia, DMTX_EXPORTS);
		changenonlayout = DiaGetControl(dia, DMTX_NONLAYOUTS);
		changeinstance = DiaGetControl(dia, DMTX_INSTANCES);
		changecell = DiaGetControl(dia, DMTX_CELL);

		/* determine text descriptor */
		TDCLEAR(newdescript);
		if (DiaGetControl(dia, DMTX_ABSTEXTSIZE_L) != 0)
		{
			i = eatoi(DiaGetText(dia, DMTX_ABSTEXTSIZE));
			if (i <= 0) i = 4;
			if (i >= TXTMAXPOINTS) i = TXTMAXPOINTS;
			TDSETSIZE(newdescript, TXTSETPOINTS(i));
		} else
		{
			i = atofr(DiaGetText(dia, DMTX_RELTEXTSIZE)) * 4 / WHOLE;
			if (i <= 0) i = 4;
			if (i >= TXTMAXQLAMBDA) i = TXTMAXQLAMBDA;
			TDSETSIZE(newdescript, TXTSETQLAMBDA(i));
		}
		if (DiaGetControl(dia, DMTX_TEXTITALIC) != 0)
			TDSETITALIC(newdescript, VTITALIC); else
				TDSETITALIC(newdescript, 0);
		if (DiaGetControl(dia, DMTX_TEXTBOLD) != 0)
			TDSETBOLD(newdescript, VTBOLD); else
				TDSETBOLD(newdescript, 0);
		if (DiaGetControl(dia, DMTX_TEXTUNDERLINE) != 0)
			TDSETUNDERLINE(newdescript, VTUNDERLINE); else
				TDSETUNDERLINE(newdescript, 0);
		if (graphicshas(CANCHOOSEFACES))
		{
			i = us_getpopupface(DMTX_TEXTFACE, dia);
			TDSETFACE(newdescript, i);
		}

		/* make the change */
		for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
			(void)startobjectchange((INTBIG)win, VWINDOWPART);

		if (DiaGetControl(dia, DMTX_SELECTED) != 0)
		{
			/* change highlighted */
			len = getlength(var);
			if (len > 0)
			{
				for(i=0; i<len; i++)
				{
					(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
			}
		} else if (DiaGetControl(dia, DMTX_ALLINCELL) != 0)
		{
			high.status = HIGHFROM;
			high.cell = cell;
			high.fromport = NOPORTPROTO;
			high.fromvar = NOVARIABLE;
			high.fromvarnoeval = NOVARIABLE;
			for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				high.fromgeom = ni->geom;
				us_settextsize(&high, newdescript, changenode, changearc,
					changeexport, changenonlayout, changeinstance, changecell);
			}
			for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				high.fromgeom = ai->geom;
				us_settextsize(&high, newdescript, changenode, changearc,
					changeexport, changenonlayout, changeinstance, changecell);
			}
			high.status = HIGHTEXT;
			high.fromgeom = NOGEOM;
			for(i=0; i<cell->numvar; i++)
			{
				high.fromvar = &cell->firstvar[i];
				us_settextsize(&high, newdescript, changenode, changearc,
					changeexport, changenonlayout, changeinstance, changecell);
			}
		} else if (DiaGetControl(dia, DMTX_ALLWITHVIEW) != 0)
		{
			/* change all with a particular view */
			i = DiaGetPopupEntry(dia, DMTX_WHICHVIEW);
			if (i < 0) v = NOVIEW; else
				v = getview(viewlist[i]);
			for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->cellview != v) continue;
				high.status = HIGHFROM;
				high.cell = np;
				high.fromport = NOPORTPROTO;
				high.fromvar = NOVARIABLE;
				high.fromvarnoeval = NOVARIABLE;
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					high.fromgeom = ni->geom;
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					high.fromgeom = ai->geom;
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
				high.status = HIGHTEXT;
				high.fromgeom = NOGEOM;
				for(i=0; i<np->numvar; i++)
				{
					high.fromvar = &np->firstvar[i];
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
			}
		} else
		{
			/* change all in library */
			for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				high.status = HIGHFROM;
				high.cell = np;
				high.fromport = NOPORTPROTO;
				high.fromvar = NOVARIABLE;
				high.fromvarnoeval = NOVARIABLE;
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					high.fromgeom = ni->geom;
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					high.fromgeom = ai->geom;
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
				high.status = HIGHTEXT;
				high.fromgeom = NOGEOM;
				for(i=0; i<np->numvar; i++)
				{
					high.fromvar = &np->firstvar[i];
					us_settextsize(&high, newdescript, changenode, changearc,
						changeexport, changenonlayout, changeinstance, changecell);
				}
			}
		}

		/* force redisplay */
		us_state |= HIGHLIGHTSET;
		for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
			(void)endobjectchange((INTBIG)win, VWINDOWPART);
	}
	DiaDoneDialog(dia);
	efree((CHAR *)viewlist);
	return(0);
}

/*
 * Routine to determine the current range of text sizes for the dialog.
 */
void us_setcurtextsizes(void *dia, INTBIG changenode, INTBIG changearc, INTBIG changeexport,
	INTBIG changenonlayout, INTBIG changeinstance, INTBIG changecell, INTBIG *lowabs,
	INTBIG *highabs, INTBIG *lowrel, INTBIG *highrel, CHAR **viewlist)
{
	REGISTER INTBIG i, len;
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np, *cell;
	REGISTER VIEW *v;
	HIGHLIGHT high;

	*lowabs = 0;   *highabs = -1;
	*lowrel = 0;   *highrel = -1;

	if (DiaGetControl(dia, DMTX_SELECTED) != 0)
	{
		/* examine highlighted */
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
		len = getlength(var);
		if (len > 0)
		{
			for(i=0; i<len; i++)
			{
				(void)us_makehighlight(((CHAR **)var->addr)[i], &high);
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
		}
	} else if (DiaGetControl(dia, DMTX_ALLINCELL) != 0)
	{
		cell = getcurcell();
		high.status = HIGHFROM;
		high.cell = cell;
		high.fromport = NOPORTPROTO;
		high.fromvar = NOVARIABLE;
		high.fromvarnoeval = NOVARIABLE;
		for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			high.fromgeom = ni->geom;
			us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
				changeinstance, changecell, lowabs, highabs, lowrel, highrel);
		}
		for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			high.fromgeom = ai->geom;
			us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
				changeinstance, changecell, lowabs, highabs, lowrel, highrel);
		}
		high.status = HIGHTEXT;
		high.fromgeom = NOGEOM;
		for(i=0; i<cell->numvar; i++)
		{
			high.fromvar = &cell->firstvar[i];
			us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
				changeinstance, changecell, lowabs, highabs, lowrel, highrel);
		}
	} else if (DiaGetControl(dia, DMTX_ALLWITHVIEW) != 0)
	{
		/* change all with a particular view */
		i = DiaGetPopupEntry(dia, DMTX_WHICHVIEW);
		if (i < 0) v = NOVIEW; else
			v = getview(viewlist[i]);
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->cellview != v) continue;
			high.status = HIGHFROM;
			high.cell = np;
			high.fromport = NOPORTPROTO;
			high.fromvar = NOVARIABLE;
			high.fromvarnoeval = NOVARIABLE;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				high.fromgeom = ni->geom;
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				high.fromgeom = ai->geom;
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
			high.status = HIGHTEXT;
			high.fromgeom = NOGEOM;
			for(i=0; i<np->numvar; i++)
			{
				high.fromvar = &np->firstvar[i];
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
		}
	} else
	{
		/* change all in library */
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			high.status = HIGHFROM;
			high.cell = np;
			high.fromport = NOPORTPROTO;
			high.fromvar = NOVARIABLE;
			high.fromvarnoeval = NOVARIABLE;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				high.fromgeom = ni->geom;
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				high.fromgeom = ai->geom;
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
			high.status = HIGHTEXT;
			high.fromgeom = NOGEOM;
			for(i=0; i<np->numvar; i++)
			{
				high.fromvar = &np->firstvar[i];
				us_gathertextsize(&high, changenode, changearc, changeexport, changenonlayout,
					changeinstance, changecell, lowabs, highabs, lowrel, highrel);
			}
		}
	}
}

void us_gathertextsize(HIGHLIGHT *high, INTBIG changenode, INTBIG changearc, INTBIG changeexport,
	INTBIG changenonlayout, INTBIG changeinstance, INTBIG changecell,
	INTBIG *lowabs, INTBIG *highabs, INTBIG *lowrel, INTBIG *highrel)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	if ((high->status&HIGHTYPE) == HIGHFROM)
	{
		if (high->fromgeom->entryisnode)
		{
			ni = high->fromgeom->entryaddr.ni;
			if (ni->proto == gen_invispinprim)
			{
				if (changenonlayout != 0)
				{
					for(i=0; i<ni->numvar; i++)
					{
						var = &ni->firstvar[i];
						us_includetextsize(TDGETSIZE(var->textdescript), lowabs, highabs, lowrel, highrel);
					}
				}
			} else
			{
				if (ni->proto->primindex == 0 && changeinstance != 0)
				{
					us_includetextsize(TDGETSIZE(ni->textdescript), lowabs, highabs, lowrel, highrel);
				}
				if (changenode != 0)
				{
					for(i=0; i<ni->numvar; i++)
					{
						var = &ni->firstvar[i];
						us_includetextsize(TDGETSIZE(var->textdescript), lowabs, highabs, lowrel, highrel);
					}
				}
			}
			if (changeexport != 0)
			{
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				{
					pp = pe->exportproto;
					us_includetextsize(TDGETSIZE(pp->textdescript), lowabs, highabs, lowrel, highrel);
				}
			}
		} else
		{
			ai = high->fromgeom->entryaddr.ai;
			if (changearc != 0)
			{
				for(i=0; i<ai->numvar; i++)
				{
					var = &ai->firstvar[i];
					us_includetextsize(TDGETSIZE(var->textdescript), lowabs, highabs, lowrel, highrel);
				}
			}
		}
	} else if ((high->status&HIGHTYPE) == HIGHTEXT)
	{
		if (high->fromvar != NOVARIABLE)
		{
			if (high->fromgeom == NOGEOM)
			{
				if (changecell != 0)
					us_includetextsize(TDGETSIZE(high->fromvar->textdescript), lowabs, highabs, lowrel, highrel);
			} else if (high->fromgeom->entryisnode)
			{
				if (high->fromport != NOPORTPROTO)
				{
					if (changeexport != 0)
						us_includetextsize(TDGETSIZE(high->fromvar->textdescript), lowabs, highabs, lowrel, highrel);
				} else
				{
					ni = high->fromgeom->entryaddr.ni;
					if (ni->proto == gen_invispinprim)
					{
						if (changenonlayout != 0)
							us_includetextsize(TDGETSIZE(high->fromvar->textdescript), lowabs, highabs, lowrel, highrel);
					} else if (changenode != 0)
						us_includetextsize(TDGETSIZE(high->fromvar->textdescript), lowabs, highabs, lowrel, highrel);
				}
			} else
			{
				if (changearc != 0)
					us_includetextsize(TDGETSIZE(high->fromvar->textdescript), lowabs, highabs, lowrel, highrel);
			}
		} else if (high->fromport != NOPORTPROTO)
		{
			if (changeexport != 0)
				us_includetextsize(TDGETSIZE(high->fromport->textdescript), lowabs, highabs, lowrel, highrel);
		}
	}
}

/*
 * Routine to add size "size" to the list of relative sizes in "lowrel" to "highrel" or absolute
 * sizes in "lowabs" to "highabs".
 */
void us_includetextsize(INTBIG size, INTBIG *lowabs, INTBIG *highabs, INTBIG *lowrel, INTBIG *highrel)
{
	REGISTER INTBIG thesize;

	thesize = TXTGETQLAMBDA(size);
	if (thesize == 0)
	{
		/* absolute size */
		thesize = TXTGETPOINTS(size);
		if (*highabs < *lowabs)
		{
			*highabs = *lowabs = thesize;
		} else
		{
			if (thesize < *lowabs) *lowabs = thesize;
			if (thesize > *highabs) *highabs = thesize;
		}
	} else
	{
		if (*highrel < *lowrel)
		{
			*highrel = *lowrel = thesize;
		} else
		{
			if (thesize < *lowrel) *lowrel = thesize;
			if (thesize > *highrel) *highrel = thesize;
		}
	}
}

void us_settextsize(HIGHLIGHT *high, UINTBIG *descript, INTBIG changenode, INTBIG changearc,
	INTBIG changeexport, INTBIG changenonlayout, INTBIG changeinstance, INTBIG changecell)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i;
	UINTBIG newdescript[TEXTDESCRIPTSIZE];
	REGISTER VARIABLE *var, *fromvar;

	/* make sure the right variable is being altered */
	fromvar = high->fromvar;
	if (high->fromvarnoeval != NOVARIABLE) fromvar = high->fromvarnoeval;

	if ((high->status&HIGHTYPE) == HIGHFROM)
	{
		if (high->fromgeom->entryisnode)
		{
			ni = high->fromgeom->entryaddr.ni;
			if (ni->proto == gen_invispinprim)
			{
				if (changenonlayout != 0)
				{
					for(i=0; i<ni->numvar; i++)
					{
						var = &ni->firstvar[i];
						TDCOPY(newdescript, var->textdescript);
						TDSETSIZE(newdescript, TDGETSIZE(descript));
						TDSETFACE(newdescript, TDGETFACE(descript));
						TDSETITALIC(newdescript, TDGETITALIC(descript));
						TDSETBOLD(newdescript, TDGETBOLD(descript));
						TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
						modifydescript((INTBIG)ni, VNODEINST, var, newdescript);
					}
				}
			} else
			{
				if (ni->proto->primindex == 0 && changeinstance != 0)
				{
					TDCOPY(newdescript, ni->textdescript);
					TDSETSIZE(newdescript, TDGETSIZE(descript));
					TDSETFACE(newdescript, TDGETFACE(descript));
					TDSETITALIC(newdescript, TDGETITALIC(descript));
					TDSETBOLD(newdescript, TDGETBOLD(descript));
					TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
					(void)setind((INTBIG)ni, VNODEINST, x_("textdescript"), 0, newdescript[0]);
					(void)setind((INTBIG)ni, VNODEINST, x_("textdescript"), 1, newdescript[1]);
				}
				if (changenode != 0)
				{
					for(i=0; i<ni->numvar; i++)
					{
						var = &ni->firstvar[i];
						TDCOPY(newdescript, var->textdescript);
						TDSETSIZE(newdescript, TDGETSIZE(descript));
						TDSETFACE(newdescript, TDGETFACE(descript));
						TDSETITALIC(newdescript, TDGETITALIC(descript));
						TDSETBOLD(newdescript, TDGETBOLD(descript));
						TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
						modifydescript((INTBIG)ni, VNODEINST, var, newdescript);
					}
				}
			}
			if (changeexport != 0)
			{
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				{
					pp = pe->exportproto;
					TDCOPY(newdescript, pp->textdescript);
					TDSETSIZE(newdescript, TDGETSIZE(descript));
					TDSETFACE(newdescript, TDGETFACE(descript));
					TDSETITALIC(newdescript, TDGETITALIC(descript));
					TDSETBOLD(newdescript, TDGETBOLD(descript));
					TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
					(void)setind((INTBIG)pp, VPORTPROTO, x_("textdescript"), 0, newdescript[0]);
					(void)setind((INTBIG)pp, VPORTPROTO, x_("textdescript"), 1, newdescript[1]);
				}
			}
		} else
		{
			ai = high->fromgeom->entryaddr.ai;
			if (changearc != 0)
			{
				for(i=0; i<ai->numvar; i++)
				{
					var = &ai->firstvar[i];
					TDCOPY(newdescript, var->textdescript);
					TDSETSIZE(newdescript, TDGETSIZE(descript));
					TDSETFACE(newdescript, TDGETFACE(descript));
					TDSETITALIC(newdescript, TDGETITALIC(descript));
					TDSETBOLD(newdescript, TDGETBOLD(descript));
					TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
					modifydescript((INTBIG)ai, VARCINST, var, newdescript);
				}
			}
		}
	} else if ((high->status&HIGHTYPE) == HIGHTEXT)
	{
		if (fromvar != NOVARIABLE)
		{
			TDCOPY(newdescript, fromvar->textdescript);
			TDSETSIZE(newdescript, TDGETSIZE(descript));
			TDSETFACE(newdescript, TDGETFACE(descript));
			TDSETITALIC(newdescript, TDGETITALIC(descript));
			TDSETBOLD(newdescript, TDGETBOLD(descript));
			TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
			if (high->fromgeom == NOGEOM)
			{
				if (changecell != 0)
				{
					/* changing variable on cell */
					modifydescript((INTBIG)high->cell, VNODEPROTO, fromvar, newdescript);
				}
			} else if (high->fromgeom->entryisnode)
			{
				if (high->fromport != NOPORTPROTO)
				{
					if (changeexport != 0)
					{
						/* changing export variable */
						pp = high->fromport;
						modifydescript((INTBIG)pp, VPORTPROTO, fromvar, newdescript);
					}
				} else
				{
					ni = high->fromgeom->entryaddr.ni;
					if (ni->proto == gen_invispinprim)
					{
						if (changenonlayout != 0)
						{
							/* changing nonlayout text variable */
							modifydescript((INTBIG)ni, VNODEINST, fromvar, newdescript);
						}
					} else if (changenode != 0)
					{
						/* changing node variable */
						modifydescript((INTBIG)ni, VNODEINST, fromvar, newdescript);
					}
				}
			} else
			{
				if (changearc != 0)
				{
					/* changing arc variable */
					ai = high->fromgeom->entryaddr.ai;
					modifydescript((INTBIG)ai, VARCINST, fromvar, newdescript);
				}
			}
		} else if (high->fromport != NOPORTPROTO)
		{
			if (changeexport != 0)
			{
				pp = high->fromport;
				ni = pp->subnodeinst;
				TDCOPY(newdescript, pp->textdescript);
				TDSETSIZE(newdescript, TDGETSIZE(descript));
				TDSETFACE(newdescript, TDGETFACE(descript));
				TDSETITALIC(newdescript, TDGETITALIC(descript));
				TDSETBOLD(newdescript, TDGETBOLD(descript));
				TDSETUNDERLINE(newdescript, TDGETUNDERLINE(descript));
				(void)setind((INTBIG)pp, VPORTPROTO, x_("textdescript"), 0, newdescript[0]);
				(void)setind((INTBIG)pp, VPORTPROTO, x_("textdescript"), 1, newdescript[1]);
			}
		}
	}
}

/****************************** TEXT OPTIONS DIALOG ******************************/

/* Text options */
static DIALOGITEM us_deftextdialogitems[] =
{
 /*  1 */ {0, {368,328,392,400}, BUTTON, N_("OK")},
 /*  2 */ {0, {368,212,392,284}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {296,32,312,88}, RADIO, N_("Left")},
 /*  4 */ {0, {28,196,44,244}, EDITTEXT, x_("")},
 /*  5 */ {0, {232,32,248,104}, RADIO, N_("Center")},
 /*  6 */ {0, {248,32,264,104}, RADIO, N_("Bottom")},
 /*  7 */ {0, {264,32,280,88}, RADIO, N_("Top")},
 /*  8 */ {0, {280,32,296,96}, RADIO, N_("Right")},
 /*  9 */ {0, {312,32,328,128}, RADIO, N_("Lower right")},
 /* 10 */ {0, {328,32,344,128}, RADIO, N_("Lower left")},
 /* 11 */ {0, {344,32,360,128}, RADIO, N_("Upper right")},
 /* 12 */ {0, {360,32,376,120}, RADIO, N_("Upper left")},
 /* 13 */ {0, {376,32,392,104}, RADIO, N_("Boxed")},
 /* 14 */ {0, {68,8,84,135}, RADIO, N_("Exports & Ports")},
 /* 15 */ {0, {216,8,232,103}, MESSAGE, N_("Text corner:")},
 /* 16 */ {0, {232,136,264,168}, ICON, (CHAR *)us_icon200},
 /* 17 */ {0, {264,136,296,168}, ICON, (CHAR *)us_icon201},
 /* 18 */ {0, {296,136,328,168}, ICON, (CHAR *)us_icon202},
 /* 19 */ {0, {328,136,360,168}, ICON, (CHAR *)us_icon203},
 /* 20 */ {0, {360,136,392,168}, ICON, (CHAR *)us_icon204},
 /* 21 */ {0, {336,308,352,380}, RADIO, N_("Outside")},
 /* 22 */ {0, {248,224,264,296}, RADIO, N_("Off")},
 /* 23 */ {0, {216,204,232,400}, MESSAGE, N_("Smart Vertical Placement:")},
 /* 24 */ {0, {236,308,252,380}, RADIO, N_("Inside")},
 /* 25 */ {0, {260,308,276,380}, RADIO, N_("Outside")},
 /* 26 */ {0, {324,224,340,296}, RADIO, N_("Off")},
 /* 27 */ {0, {292,204,308,400}, MESSAGE, N_("Smart Horizontal Placement:")},
 /* 28 */ {0, {312,308,328,380}, RADIO, N_("Inside")},
 /* 29 */ {0, {184,8,200,280}, CHECK, N_("New text visible only inside cell")},
 /* 30 */ {0, {160,100,176,224}, POPUP, x_("")},
 /* 31 */ {0, {160,8,176,99}, MESSAGE, N_("Text editor:")},
 /* 32 */ {0, {4,12,20,411}, MESSAGE, N_("Default text information for different types of text:")},
 /* 33 */ {0, {52,196,68,244}, EDITTEXT, x_("")},
 /* 34 */ {0, {28,8,44,135}, RADIO, N_("Nodes")},
 /* 35 */ {0, {80,140,96,228}, MESSAGE, N_("Type face:")},
 /* 36 */ {0, {48,8,64,135}, RADIO, N_("Arcs")},
 /* 37 */ {0, {28,252,44,412}, RADIO, N_("Points (max 63)")},
 /* 38 */ {0, {108,8,124,135}, RADIO, N_("Instance names")},
 /* 39 */ {0, {52,252,68,412}, RADIO, N_("Lambda (max 127.75)")},
 /* 40 */ {0, {88,8,104,135}, RADIO, N_("Nonlayout text")},
 /* 41 */ {0, {208,8,209,412}, DIVIDELINE, x_("")},
 /* 42 */ {0, {152,8,153,412}, DIVIDELINE, x_("")},
 /* 43 */ {0, {209,188,392,189}, DIVIDELINE, x_("")},
 /* 44 */ {0, {100,140,116,376}, POPUP, x_("")},
 /* 45 */ {0, {128,140,144,212}, CHECK, N_("Italic")},
 /* 46 */ {0, {128,236,144,296}, CHECK, N_("Bold")},
 /* 47 */ {0, {128,324,144,412}, CHECK, N_("Underline")},
 /* 48 */ {0, {40,140,56,188}, MESSAGE, N_("Size")},
 /* 49 */ {0, {128,8,144,135}, RADIO, N_("Cell text")}
};
static DIALOG us_deftextdialog = {{50,75,451,497}, N_("Text Options"), 0, 49, us_deftextdialogitems, 0, 0};

/* special items for "text options" dialog: */
#define DTXO_GRABLEFT        3		/* left (radio) */
#define DTXO_ABSTEXTSIZE     4		/* absolute text size (edit text) */
#define DTXO_GRABCENTER      5		/* center (radio) */
#define DTXO_GRABBOT         6		/* bottom (radio) */
#define DTXO_GRABTOP         7		/* top (radio) */
#define DTXO_GRABRIGHT       8		/* right (radio) */
#define DTXO_GRABLOWRIGHT    9		/* lower right (radio) */
#define DTXO_GRABLOWLEFT    10		/* lower left (radio) */
#define DTXO_GRABUPRIGHT    11		/* upper right (radio) */
#define DTXO_GRABUPLEFT     12		/* upper left (radio) */
#define DTXO_GRABBOXED      13		/* boxed (radio) */
#define DTXO_EXPORTSIZE     14		/* set default export text size (radio) */
#define DTXO_LOWICON        16		/* low icon (icon) */
#define DTXO_HIGHICON       20		/* high icon (icon) */
#define DTXO_SMARTHOROUT    21		/* smart horizontal outside (radio) */
#define DTXO_SMARTVERTOFF   22		/* smart vertical off (radio) */
#define DTXO_SMARTVERTIN    24		/* smart vertical inside (radio) */
#define DTXO_SMARTVERTOUT   25		/* smart vertical outside (radio) */
#define DTXO_SMARTHOROFF    26		/* smart horizontal off (radio) */
#define DTXO_SMARTHORIN     28		/* smart horizontal inside (radio) */
#define DTXO_ONLYINSIDE     29		/* only inside cell (check) */
#define DTXO_TEXTEDITOR     30		/* text editor (popup) */
#define DTXO_RELTEXTSIZE    33		/* relative text size (edit text) */
#define DTXO_NODESIZE       34		/* set default node text size (radio) */
#define DTXO_TEXTFACE_L     35		/* text face label (stat text) */
#define DTXO_ARCSIZE        36		/* set default arc text size (radio) */
#define DTXO_ABSTEXTSIZE_L  37		/* absolute text size label (radio) */
#define DTXO_INSTSIZE       38		/* set default cell instance text size (radio) */
#define DTXO_RELTEXTSIZE_L  39		/* relative text size label (radio) */
#define DTXO_NONLAYOUTSIZE  40		/* set default invis-pin text size (radio) */
#define DTXO_TEXTFACE       44		/* text face (popup) */
#define DTXO_TEXTITALIC     45		/* text italic (check) */
#define DTXO_TEXTBOLD       46		/* text bold (check) */
#define DTXO_TEXTUNDERLINE  47		/* text underline (check) */
#define DTXO_CELLSIZE       49		/* set default cell text size (radio) */

void us_setdeftextinfo(UINTBIG *descript, void *dia);

INTBIG us_deftextdlog(CHAR *prompt)
{
	Q_UNUSED( prompt );
	INTBIG itemHit, i, j, ecount, cureditor;
	INTBIG x, y;
	CHAR *newlang[16];
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *w;
	REGISTER INTBIG grabpoint, textstyle, smarthstyle, smartvstyle;
	REGISTER void *dia;
	UINTBIG fontnode[TEXTDESCRIPTSIZE], fontarc[TEXTDESCRIPTSIZE],
		fontexport[TEXTDESCRIPTSIZE], fontnonlayout[TEXTDESCRIPTSIZE],
		fontinst[TEXTDESCRIPTSIZE], fontcell[TEXTDESCRIPTSIZE],
		ofontnode[TEXTDESCRIPTSIZE], ofontarc[TEXTDESCRIPTSIZE],
		ofontexport[TEXTDESCRIPTSIZE], ofontnonlayout[TEXTDESCRIPTSIZE],
		ofontinst[TEXTDESCRIPTSIZE], ofontcell[TEXTDESCRIPTSIZE], *curdescript;
	RECTAREA itemRect;
	static struct butlist poslist[10] =
	{
		{VTPOSCENT,      DTXO_GRABCENTER},
		{VTPOSUP,        DTXO_GRABBOT},
		{VTPOSDOWN,      DTXO_GRABTOP},
		{VTPOSLEFT,      DTXO_GRABRIGHT},
		{VTPOSRIGHT,     DTXO_GRABLEFT},
		{VTPOSUPLEFT,    DTXO_GRABLOWRIGHT},
		{VTPOSUPRIGHT,   DTXO_GRABLOWLEFT},
		{VTPOSDOWNLEFT,  DTXO_GRABUPRIGHT},
		{VTPOSDOWNRIGHT, DTXO_GRABUPLEFT},
		{VTPOSBOXED,     DTXO_GRABBOXED}
	};

	/* display the default text dialog box */
	dia = DiaInitDialog(&us_deftextdialog);
	if (dia == 0) return(0);

	/* load the editor choice popup */
	for(ecount=0; us_editortable[ecount].editorname != 0; ecount++)
		newlang[ecount] = us_editortable[ecount].editorname;
	DiaSetPopup(dia, DTXO_TEXTEDITOR, ecount, newlang);
	cureditor = 0;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_text_editorkey);
	if (var != NOVARIABLE)
	{
		for(i=0; i<ecount; i++)
			if (namesame(newlang[i], (CHAR *)var->addr) == 0) break;
		if (i < ecount) cureditor = i;
	}
	DiaSetPopupEntry(dia, DTXO_TEXTEDITOR, cureditor);

	/* make sure there are no text editors running */
	DiaUnDimItem(dia, DTXO_TEXTEDITOR);
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if ((w->state&WINDOWTYPE) == TEXTWINDOW || (w->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		DiaDimItem(dia, DTXO_TEXTEDITOR);
		break;
	}

	/* set current defaults */
	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_style"));
	if (var == NOVARIABLE) textstyle = VTPOSCENT; else textstyle = var->addr;
	grabpoint = textstyle & VTPOSITION;
	for(i=0; i<10; i++) if (grabpoint == (INTBIG)poslist[i].value)
	{
		DiaSetControl(dia, poslist[i].button, 1);
		break;
	}
	if ((textstyle&VTINTERIOR) != 0) DiaSetControl(dia, DTXO_ONLYINSIDE, 1);

	var = getval((INTBIG)us_tool, VTOOL, VINTEGER, x_("USER_default_text_smart_style"));
	if (var == NOVARIABLE)
	{
		smarthstyle = smartvstyle = 0;
	} else
	{
		smarthstyle = var->addr & 03;
		smartvstyle = (var->addr >> 2) & 03;
	}
	switch (smarthstyle)
	{
		case 1:  DiaSetControl(dia, DTXO_SMARTHORIN, 1);  break;
		case 2:  DiaSetControl(dia, DTXO_SMARTHOROUT, 1);  break;
		default: DiaSetControl(dia, DTXO_SMARTHOROFF, 1);  break;
	}
	switch (smartvstyle)
	{
		case 1:  DiaSetControl(dia, DTXO_SMARTVERTIN, 1);  break;
		case 2:  DiaSetControl(dia, DTXO_SMARTVERTOUT, 1);  break;
		default: DiaSetControl(dia, DTXO_SMARTVERTOFF, 1);  break;
	}

	/* get default text information */
	if (graphicshas(CANCHOOSEFACES))
	{
		DiaUnDimItem(dia, DTXO_TEXTFACE_L);
		us_setpopupface(DTXO_TEXTFACE, 0, TRUE, dia);
	} else
	{
		DiaDimItem(dia, DTXO_TEXTFACE_L);
	}
	if (graphicshas(CANMODIFYFONTS))
	{
		DiaUnDimItem(dia, DTXO_TEXTITALIC);
		DiaUnDimItem(dia, DTXO_TEXTBOLD);
		DiaUnDimItem(dia, DTXO_TEXTUNDERLINE);
	} else
	{
		DiaDimItem(dia, DTXO_TEXTITALIC);
		DiaDimItem(dia, DTXO_TEXTBOLD);
		DiaDimItem(dia, DTXO_TEXTUNDERLINE);
	}
	TDCLEAR(fontnode);      defaulttextsize(3, fontnode);      TDCOPY(ofontnode, fontnode);
	TDCLEAR(fontarc);       defaulttextsize(4, fontarc);       TDCOPY(ofontarc, fontarc);
	TDCLEAR(fontexport);    defaulttextsize(1, fontexport);    TDCOPY(ofontexport, fontexport);
	TDCLEAR(fontnonlayout); defaulttextsize(2, fontnonlayout); TDCOPY(ofontnonlayout, fontnonlayout);
	TDCLEAR(fontinst);      defaulttextsize(5, fontinst);      TDCOPY(ofontinst, fontinst);
	TDCLEAR(fontcell);      defaulttextsize(6, fontcell);      TDCOPY(ofontcell, fontcell);
	DiaSetControl(dia, DTXO_NODESIZE, 1);
	curdescript = fontnode;
	us_setdeftextinfo(curdescript, dia);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;

		if (itemHit >= DTXO_LOWICON && itemHit <= DTXO_HIGHICON)
		{
			DiaItemRect(dia, itemHit, &itemRect);
			DiaGetMouse(dia, &x, &y);
			itemHit = (itemHit-DTXO_LOWICON) * 2;
			if (y > (itemRect.top + itemRect.bottom) / 2) itemHit++;
			itemHit = poslist[itemHit].button;
		}

		/* hits on the orientation buttons */
		for(i=0; i<10; i++) if (itemHit == poslist[i].button)
		{
			DiaSetControl(dia, poslist[grabpoint].button, 0);
			grabpoint = i;
			DiaSetControl(dia, poslist[i].button, 1);
			break;
		}

		if (itemHit == DTXO_SMARTVERTOFF || itemHit == DTXO_SMARTVERTIN ||
			itemHit == DTXO_SMARTVERTOUT)
		{
			DiaSetControl(dia, DTXO_SMARTVERTOFF, 0);
			DiaSetControl(dia, DTXO_SMARTVERTIN, 0);
			DiaSetControl(dia, DTXO_SMARTVERTOUT, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DTXO_SMARTHOROFF || itemHit == DTXO_SMARTHORIN || itemHit == DTXO_SMARTHOROUT)
		{
			DiaSetControl(dia, DTXO_SMARTHOROFF, 0);
			DiaSetControl(dia, DTXO_SMARTHORIN, 0);
			DiaSetControl(dia, DTXO_SMARTHOROUT, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DTXO_ONLYINSIDE)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DTXO_NODESIZE || itemHit == DTXO_ARCSIZE ||
			itemHit == DTXO_EXPORTSIZE || itemHit == DTXO_NONLAYOUTSIZE ||
			itemHit == DTXO_INSTSIZE || itemHit == DTXO_CELLSIZE)
		{
			DiaSetControl(dia, DTXO_NODESIZE, 0);
			DiaSetControl(dia, DTXO_ARCSIZE, 0);
			DiaSetControl(dia, DTXO_EXPORTSIZE, 0);
			DiaSetControl(dia, DTXO_NONLAYOUTSIZE, 0);
			DiaSetControl(dia, DTXO_INSTSIZE, 0);
			DiaSetControl(dia, DTXO_CELLSIZE, 0);
			DiaSetControl(dia, itemHit, 1);
			switch (itemHit)
			{
				case DTXO_NODESIZE:      curdescript = fontnode;          break;
				case DTXO_ARCSIZE:       curdescript = fontarc;           break;
				case DTXO_EXPORTSIZE:    curdescript = fontexport;        break;
				case DTXO_NONLAYOUTSIZE: curdescript = fontnonlayout;     break;
				case DTXO_INSTSIZE:      curdescript = fontinst;          break;
				case DTXO_CELLSIZE:      curdescript = fontcell;          break;
			}
			us_setdeftextinfo(curdescript, dia);
			continue;
		}
		if (itemHit == DTXO_ABSTEXTSIZE_L || itemHit == DTXO_RELTEXTSIZE_L)
		{
			DiaSetControl(dia, DTXO_ABSTEXTSIZE_L, 0);
			DiaSetControl(dia, DTXO_RELTEXTSIZE_L, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DTXO_ABSTEXTSIZE_L)
			{
				DiaUnDimItem(dia, DTXO_ABSTEXTSIZE);
				DiaDimItem(dia, DTXO_RELTEXTSIZE);
				itemHit = DTXO_ABSTEXTSIZE;
			} else
			{
				DiaUnDimItem(dia, DTXO_RELTEXTSIZE);
				DiaDimItem(dia, DTXO_ABSTEXTSIZE);
				itemHit = DTXO_RELTEXTSIZE;
			}
		}
		if (itemHit == DTXO_ABSTEXTSIZE || itemHit == DTXO_RELTEXTSIZE)
		{
			if (DiaGetControl(dia, DTXO_ABSTEXTSIZE_L) != 0)
			{
				i = eatoi(DiaGetText(dia, DTXO_ABSTEXTSIZE));
				if (i <= 0) i = 4;
				if (i >= TXTMAXPOINTS) i = TXTMAXPOINTS;
				TDSETSIZE(curdescript, TXTSETPOINTS(i));
			} else
			{
				i = atofr(DiaGetText(dia, DTXO_RELTEXTSIZE)) * 4 / WHOLE;
				if (i <= 0) i = 4;
				if (i >= TXTMAXQLAMBDA) i = TXTMAXQLAMBDA;
				TDSETSIZE(curdescript, TXTSETQLAMBDA(i));
			}
			continue;
		}
		if (itemHit == DTXO_TEXTFACE)
		{
			TDSETFACE(curdescript, us_getpopupface(DTXO_TEXTFACE, dia));
			continue;
		}
		if (itemHit == DTXO_TEXTITALIC)
		{
			i = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			if (i == 0) TDSETITALIC(curdescript, 0); else
				TDSETITALIC(curdescript, VTITALIC);
			continue;
		}
		if (itemHit == DTXO_TEXTBOLD)
		{
			i = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			if (i == 0) TDSETBOLD(curdescript, 0); else
				TDSETBOLD(curdescript, VTBOLD);
			continue;
		}
		if (itemHit == DTXO_TEXTUNDERLINE)
		{
			i = 1 - DiaGetControl(dia, itemHit);
			DiaSetControl(dia, itemHit, i);
			if (i == 0) TDSETUNDERLINE(curdescript, 0); else
				TDSETUNDERLINE(curdescript, VTUNDERLINE);
			continue;
		}
	}

	if (itemHit == OK)
	{
		if (TDDIFF(ofontnode, fontnode))
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_node_text_size"),
				(INTBIG)fontnode, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH));
		}

		if (TDDIFF(ofontarc, fontarc))
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_arc_text_size"),
				(INTBIG)fontarc, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH));
		}

		if (TDDIFF(ofontexport, fontexport))
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_export_text_size"),
				(INTBIG)fontexport, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH));
		}

		if (TDDIFF(ofontnonlayout, fontnonlayout))
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_nonlayout_text_size"),
				(INTBIG)fontnonlayout, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH));
		}

		if (TDDIFF(ofontinst, fontinst))
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_instance_text_size"),
				(INTBIG)fontinst, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH));
		}

		if (TDDIFF(ofontcell, fontcell))
		{
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_facet_text_size"),
				(INTBIG)fontcell, VINTEGER|VISARRAY|(TEXTDESCRIPTSIZE<<VLENGTHSH));
		}

		if (DiaGetControl(dia, DTXO_ONLYINSIDE) != 0) grabpoint |= VTINTERIOR;
		if (grabpoint != textstyle)
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_style"), grabpoint, VINTEGER);
		if (DiaGetControl(dia, DTXO_SMARTHOROFF) != 0) i = 0; else
			if (DiaGetControl(dia, DTXO_SMARTHORIN) != 0) i = 1; else
				if (DiaGetControl(dia, DTXO_SMARTHOROUT) != 0) i = 2;
		if (DiaGetControl(dia, DTXO_SMARTVERTOFF) != 0) j = 0; else
			if (DiaGetControl(dia, DTXO_SMARTVERTIN) != 0) j = 1; else
				if (DiaGetControl(dia, DTXO_SMARTVERTOUT) != 0) j = 2;
		if (smarthstyle != i || smartvstyle != j)
			setval((INTBIG)us_tool, VTOOL, x_("USER_default_text_smart_style"),
				i | (j << 2), VINTEGER);
		i = DiaGetPopupEntry(dia, DTXO_TEXTEDITOR);
		if (i != cureditor)
		{
			setvalkey((INTBIG)us_tool, VTOOL, us_text_editorkey, (INTBIG)us_editortable[i].editorname,
				VSTRING);
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

void us_setdeftextinfo(UINTBIG *descript, void *dia)
{
	INTBIG i;
	CHAR buf[30];

	i = TDGETSIZE(descript);
	if (TXTGETPOINTS(i) != 0)
	{
		/* show point size */
		esnprintf(buf, 30, x_("%ld"), TXTGETPOINTS(i));
		DiaUnDimItem(dia, DTXO_ABSTEXTSIZE);
		DiaSetText(dia, DTXO_ABSTEXTSIZE, buf);
		DiaSetControl(dia, DTXO_ABSTEXTSIZE_L, 1);

		/* clear lambda amount */
		DiaSetText(dia, DTXO_RELTEXTSIZE, x_(""));
		DiaDimItem(dia, DTXO_RELTEXTSIZE);
		DiaSetControl(dia, DTXO_RELTEXTSIZE_L, 0);
	} else if (TXTGETQLAMBDA(i) != 0)
	{
		/* show lambda amount */
		DiaUnDimItem(dia, DTXO_RELTEXTSIZE);
		DiaSetText(dia, DTXO_RELTEXTSIZE, frtoa(TXTGETQLAMBDA(i) * WHOLE / 4));
		DiaSetControl(dia, DTXO_RELTEXTSIZE_L, 1);

		/* clear point size */
		DiaSetText(dia, DTXO_ABSTEXTSIZE, x_(""));
		DiaDimItem(dia, DTXO_ABSTEXTSIZE);
		DiaSetControl(dia, DTXO_ABSTEXTSIZE_L, 0);
	}
	us_setpopupface(DTXO_TEXTFACE, TDGETFACE(descript), FALSE, dia);
	if (graphicshas(CANMODIFYFONTS))
	{
		if (TDGETITALIC(descript) != 0) DiaSetControl(dia, DTXO_TEXTITALIC, 1); else
			DiaSetControl(dia, DTXO_TEXTITALIC, 0);
		if (TDGETBOLD(descript) != 0) DiaSetControl(dia, DTXO_TEXTBOLD, 1); else
			DiaSetControl(dia, DTXO_TEXTBOLD, 0);
		if (TDGETUNDERLINE(descript) != 0) DiaSetControl(dia, DTXO_TEXTUNDERLINE, 1); else
			DiaSetControl(dia, DTXO_TEXTUNDERLINE, 0);
	}
}

/****************************** UNITS DIALOG ******************************/

/* Units */
static DIALOGITEM us_unitsdialogitems[] =
{
 /*  1 */ {0, {292,509,316,581}, BUTTON, N_("OK")},
 /*  2 */ {0, {256,509,280,581}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {196,512,212,588}, EDITTEXT, x_("")},
 /*  4 */ {0, {196,308,212,508}, MESSAGE, N_("Lambda size (internal units):")},
 /*  5 */ {0, {32,132,48,292}, POPUP, x_("")},
 /*  6 */ {0, {8,8,24,124}, MESSAGE, N_("Display Units:")},
 /*  7 */ {0, {238,132,254,292}, POPUP, x_("")},
 /*  8 */ {0, {214,8,230,126}, MESSAGE, N_("Internal Units:")},
 /*  9 */ {0, {52,308,188,588}, SCROLL, x_("")},
 /* 10 */ {0, {32,380,48,500}, MESSAGE, N_("Technologies:")},
 /* 11 */ {0, {8,452,24,588}, MESSAGE, x_("")},
 /* 12 */ {0, {8,308,24,452}, MESSAGE, N_("Current library:")},
 /* 13 */ {0, {308,324,324,476}, RADIO, N_("Change all libraries")},
 /* 14 */ {0, {284,324,300,476}, RADIO, N_("Change current library")},
 /* 15 */ {0, {8,300,324,301}, DIVIDELINE, x_("")},
 /* 16 */ {0, {220,512,236,588}, MESSAGE, x_("")},
 /* 17 */ {0, {260,324,276,476}, RADIO, N_("Change no libraries")},
 /* 18 */ {0, {236,308,252,476}, MESSAGE, N_("When changing lambda:")},
 /* 19 */ {0, {32,16,48,132}, MESSAGE, N_("Distance:")},
 /* 20 */ {0, {56,132,72,292}, POPUP, x_("")},
 /* 21 */ {0, {238,16,254,132}, MESSAGE, N_("Distance:")},
 /* 22 */ {0, {56,16,72,132}, MESSAGE, N_("Resistance:")},
 /* 23 */ {0, {80,132,96,292}, POPUP, x_("")},
 /* 24 */ {0, {152,16,168,132}, MESSAGE, N_("Voltage:")},
 /* 25 */ {0, {80,16,96,132}, MESSAGE, N_("Capacitance:")},
 /* 26 */ {0, {104,132,120,292}, POPUP, x_("")},
 /* 27 */ {0, {176,16,192,132}, MESSAGE, N_("Time:")},
 /* 28 */ {0, {104,16,120,132}, MESSAGE, N_("Inductance:")},
 /* 29 */ {0, {128,132,144,292}, POPUP, x_("")},
 /* 30 */ {0, {176,132,192,292}, POPUP, x_("")},
 /* 31 */ {0, {128,16,144,132}, MESSAGE, N_("Current:")},
 /* 32 */ {0, {152,132,168,292}, POPUP, x_("")},
 /* 33 */ {0, {258,16,274,294}, MESSAGE, N_("(read manual before changing this)")}
};
static DIALOG us_unitsdialog = {{50,75,383,672}, N_("Change Units"), 0, 33, us_unitsdialogitems, 0, 0};
static BOOLEAN us_topoftechlambda(CHAR **c);
static CHAR *us_nexttechlambda(void);

/* special items for the "units" dialog: */
#define DUNO_LAMBDA       3		/* Lambda (edit text) */
#define DUNO_DISTDUNIT    5		/* Display distance units (popup) */
#define DUNO_DISTIUNIT    7		/* Internal distance units (popup) */
#define DUNO_TECHLIST     9		/* Technologies (scroll) */
#define DUNO_CURLIB      11		/* Current lib (stat text) */
#define DUNO_CHGALLLIBS  13		/* Change all libs (radio) */
#define DUNO_CHGTHISLIB  14		/* Change this lib (radio) */
#define DUNO_MICRONS     16		/* Micron value (stat text) */
#define DUNO_CHGNOLIBS   17		/* Change no libs (radio) */
#define DUNO_RESDUNIT    20		/* Display resistance units (popup) */
#define DUNO_CAPDUNIT    23		/* Display capacitance units (popup) */
#define DUNO_INDDUNIT    26		/* Display inductance units (popup) */
#define DUNO_CURDUNIT    29		/* Display current units (popup) */
#define DUNO_TIMEDUNIT   30		/* Display time units (popup) */
#define DUNO_VOLTDUNIT   32		/* Display voltage units (popup) */

INTBIG us_lambdadlog(void)
{
	REGISTER INTBIG itemHit, i, count, newiunit, newdunit, oldiunit, olddunit,
		which, newlam, oldlam, *newlamarray, listlen;
	BOOLEAN toldposlambda, dochange;
	float lambdainmicrons;
	REGISTER WINDOWPART *w;
	CHAR ent[30], *newlang[8], *pt, *line;
	REGISTER TECHNOLOGY **techarray, *tech, *curtech;
	static CHAR *dispunitnames[8] = {N_("Lambda units"), N_("Inches"), N_("Centimeters"),
		N_("Millimeters"), N_("Mils"), N_("Microns"), N_("Centimicrons"), N_("Nanometers")};
	static CHAR *intunitnames[2] = {N_("Half-Nanometers"), N_("Half-Decimicrons")};
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the units dialog box */
	dia = DiaInitDialog(&us_unitsdialog);
	if (dia == 0) return(0);
	DiaSetText(dia, DUNO_CURLIB, el_curlib->libname);
	DiaSetControl(dia, DUNO_CHGALLLIBS, 1);

	/* set the internal units */
	for(i=0; i<2; i++) newlang[i] = TRANSLATE(intunitnames[i]);
	DiaSetPopup(dia, DUNO_DISTIUNIT, 2, newlang);
	oldiunit = el_units&INTERNALUNITS;
	switch (oldiunit)
	{
		case INTUNITHNM:   DiaSetPopupEntry(dia, DUNO_DISTIUNIT, 0);   break;
		case INTUNITHDMIC: DiaSetPopupEntry(dia, DUNO_DISTIUNIT, 1);   break;
	}

	/* load the display units */
	for(i=0; i<8; i++) newlang[i] = TRANSLATE(dispunitnames[i]);
	DiaSetPopup(dia, DUNO_DISTDUNIT, 8, newlang);
	olddunit = el_units&DISPLAYUNITS;
	switch (olddunit)
	{
		case DISPUNITLAMBDA: DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 0);   break;
		case DISPUNITINCH:   DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 1);   break;
		case DISPUNITCM:     DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 2);   break;
		case DISPUNITMM:     DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 3);   break;
		case DISPUNITMIL:    DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 4);   break;
		case DISPUNITMIC:    DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 5);   break;
		case DISPUNITCMIC:   DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 6);   break;
		case DISPUNITNM:     DiaSetPopupEntry(dia, DUNO_DISTDUNIT, 7);   break;
	}
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_resistancenames[i]);
	DiaSetPopup(dia, DUNO_RESDUNIT, 4, newlang);
	DiaSetPopupEntry(dia, DUNO_RESDUNIT,
		(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH);
	for(i=0; i<6; i++) newlang[i] = TRANSLATE(us_capacitancenames[i]);
	DiaSetPopup(dia, DUNO_CAPDUNIT, 6, newlang);
	DiaSetPopupEntry(dia, DUNO_CAPDUNIT,
		(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_inductancenames[i]);
	DiaSetPopup(dia, DUNO_INDDUNIT, 4, newlang);
	DiaSetPopupEntry(dia, DUNO_INDDUNIT,
		(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH);
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(us_currentnames[i]);
	DiaSetPopup(dia, DUNO_CURDUNIT, 3, newlang);
	DiaSetPopupEntry(dia, DUNO_CURDUNIT,
		(us_electricalunits&INTERNALCURUNITS) >> INTERNALCURUNITSSH);
	for(i=0; i<4; i++) newlang[i] = TRANSLATE(us_voltagenames[i]);
	DiaSetPopup(dia, DUNO_VOLTDUNIT, 4, newlang);
	DiaSetPopupEntry(dia, DUNO_VOLTDUNIT,
		(us_electricalunits&INTERNALVOLTUNITS) >> INTERNALVOLTUNITSSH);
	for(i=0; i<6; i++) newlang[i] = TRANSLATE(us_timenames[i]);
	DiaSetPopup(dia, DUNO_TIMEDUNIT, 6, newlang);
	DiaSetPopupEntry(dia, DUNO_TIMEDUNIT,
		(us_electricalunits&INTERNALTIMEUNITS) >> INTERNALTIMEUNITSSH);

	/* load the list of technologies */
	curtech = el_curtech;
	if (curtech == sch_tech || curtech == gen_tech || curtech == art_tech)
		curtech = defschematictechnology(el_curtech);
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		tech->temp1 = el_curlib->lambda[tech->techindex];
	DiaInitTextDialog(dia, DUNO_TECHLIST, us_topoftechlambda, us_nexttechlambda, DiaNullDlogDone, 0,
		SCSELMOUSE|SCREPORT);
	listlen = DiaGetNumScrollLines(dia, DUNO_TECHLIST);
	for(i=0; i<listlen; i++)
	{
		line = DiaGetScrollLine(dia, DUNO_TECHLIST, i);
		for(pt = line; *pt != 0; pt++) if (*pt == ' ') break;
		*pt = 0;
		if (namesame(line, curtech->techname) == 0) break;
	}
	DiaSelectLine(dia, DUNO_TECHLIST, i);

	/* set the current value of lambda */
	(void)esnprintf(ent, 30, x_("%ld"), curtech->temp1);
	DiaSetText(dia, -DUNO_LAMBDA, ent);
	lambdainmicrons = scaletodispunit(curtech->temp1, DISPUNITMIC);
	(void)esnprintf(ent, 30, x_("(%gu)"), lambdainmicrons);
	DiaSetText(dia, DUNO_MICRONS, ent);

	/* loop until done */
	toldposlambda = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK && DiaValidEntry(dia, DUNO_LAMBDA)) break;
		if (itemHit == DUNO_CHGALLLIBS || itemHit == DUNO_CHGTHISLIB || itemHit == DUNO_CHGNOLIBS)
		{
			DiaSetControl(dia, DUNO_CHGALLLIBS, 0);
			DiaSetControl(dia, DUNO_CHGTHISLIB, 0);
			DiaSetControl(dia, DUNO_CHGNOLIBS, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DUNO_LAMBDA)
		{
			i = eatoi(DiaGetText(dia, DUNO_LAMBDA));
			if (i <= 0 && !toldposlambda)
			{
				toldposlambda = TRUE;
				DiaMessageInDialog(_("Lambda value must be positive"));
			}
			curtech->temp1 = i;
			lambdainmicrons = scaletodispunit(curtech->temp1, DISPUNITMIC);
			(void)esnprintf(ent, 30, x_("(%gu)"), lambdainmicrons);
			DiaSetText(dia, DUNO_MICRONS, ent);
			which = DiaGetCurLine(dia, DUNO_TECHLIST);
			infstr = initinfstr();
			lambdainmicrons = scaletodispunit(curtech->temp1, DISPUNITMIC);
			formatinfstr(infstr, x_("%s (lambda=%ld, %gu)"), curtech->techname, curtech->temp1,
				lambdainmicrons);
			DiaSetScrollLine(dia, DUNO_TECHLIST, which, returninfstr(infstr));
			continue;
		}
		if (itemHit == DUNO_TECHLIST)
		{
			which = DiaGetCurLine(dia, DUNO_TECHLIST);
			line = DiaGetScrollLine(dia, DUNO_TECHLIST, which);
			for(pt = line; *pt != 0; pt++) if (*pt == ' ') break;
			*pt = 0;
			for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
				if (namesame(line, tech->techname) == 0) break;
			*pt = ' ';
			if (tech == NOTECHNOLOGY) continue;
			curtech = tech;
			(void)esnprintf(ent, 30, x_("%ld"), curtech->temp1);
			DiaSetText(dia, -DUNO_LAMBDA, ent);
			lambdainmicrons = scaletodispunit(curtech->temp1, DISPUNITMIC);
			(void)esnprintf(ent, 30, x_("(%gu)"), lambdainmicrons);
			DiaSetText(dia, DUNO_MICRONS, ent);
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		/* set display units */
		i = DiaGetPopupEntry(dia, DUNO_DISTDUNIT);
		switch (i)
		{
			case 0:  newdunit = DISPUNITLAMBDA;  break;
			case 1:  newdunit = DISPUNITINCH;    break;
			case 2:  newdunit = DISPUNITCM;      break;
			case 3:  newdunit = DISPUNITMM;      break;
			case 4:  newdunit = DISPUNITMIL;     break;
			case 5:  newdunit = DISPUNITMIC;     break;
			case 6:  newdunit = DISPUNITCMIC;    break;
			case 7:  newdunit = DISPUNITNM;      break;
			default: newdunit = DISPUNITLAMBDA;  break;
		}
		if (newdunit != olddunit)
			setvalkey((INTBIG)us_tool, VTOOL, us_displayunitskey, newdunit, VINTEGER);

		/* set other display units if they changed */
		newiunit = 0;
		i = DiaGetPopupEntry(dia, DUNO_RESDUNIT);
		newiunit |= i << INTERNALRESUNITSSH;
		i = DiaGetPopupEntry(dia, DUNO_CAPDUNIT);
		newiunit |= (i << INTERNALCAPUNITSSH);
		i = DiaGetPopupEntry(dia, DUNO_INDDUNIT);
		newiunit |= i << INTERNALINDUNITSSH;
		i = DiaGetPopupEntry(dia, DUNO_CURDUNIT);
		newiunit |= i << INTERNALCURUNITSSH;
		i = DiaGetPopupEntry(dia, DUNO_VOLTDUNIT);
		newiunit |= i << INTERNALVOLTUNITSSH;
		i = DiaGetPopupEntry(dia, DUNO_TIMEDUNIT);
		newiunit |= i << INTERNALTIMEUNITSSH;
		if (newiunit != us_electricalunits)
		{
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_electricalunitskey,
				newiunit, VINTEGER);

			/* redisplay windows to ensure units look right */
			us_pushhighlight();
			us_clearhighlightcount();
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if (w->redisphandler != 0) (*w->redisphandler)(w);
			us_pophighlight(FALSE);
		}
		/* see if internal unit was changed */
		i = DiaGetPopupEntry(dia, DUNO_DISTIUNIT);
		switch (i)
		{
			case 0: newiunit = INTUNITHNM;     break;
			case 1: newiunit = INTUNITHDMIC;   break;
		}
		if (newiunit != oldiunit)
			changeinternalunits(NOLIBRARY, el_units, newiunit);

		/* see if lambda values changed */
		count = 0;
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		{
			if (tech->temp1 <= 0)
				tech->temp1 = el_curlib->lambda[tech->techindex];
			if (tech->temp1 != el_curlib->lambda[tech->techindex]) count++;
		}
		if (count != 0)
		{
			dochange = TRUE;
			if (DiaGetControl(dia, DUNO_CHGALLLIBS) != 0)
			{
				/* changing all libraries */
				i = us_noyesdlog(_("This will scale all of your circuitry, making it potentially incompatible with other libraries.  Continue?"),
					newlang);
				if (i == 0 || namesame(newlang[0], x_("no")) == 0) dochange = FALSE;
				i = 2;
			} else if (DiaGetControl(dia, DUNO_CHGTHISLIB) != 0)
			{
				/* changing only the current library */
				if (el_curlib->nextlibrary == NOLIBRARY)
				{
					i = us_noyesdlog(_("This will scale your circuitry, making it potentially incompatible with other libraries.  Continue?"),
						newlang);
				} else
				{
					i = us_noyesdlog(_("This will scale the circuitry in the current library, making it potentially incompatible with other libraries.  Continue?"),
						newlang);
				}
				if (i == 0 || namesame(newlang[0], x_("no")) == 0) dochange = FALSE;
				i = 1;
			} else
			{
				/* changing no libraries */
				i = us_noyesdlog(_("This will scale the technology primitives, making them potentially incompatible with existing circuitry.  Continue?"),
					newlang);
				if (i == 0 || namesame(newlang[0], x_("no")) == 0) dochange = FALSE;
				i = 0;
			}

			if (dochange)
			{
				techarray = (TECHNOLOGY **)emalloc(count * (sizeof (TECHNOLOGY *)), el_tempcluster);
				if (techarray == 0) return(0);
				newlamarray = (INTBIG *)emalloc(count * SIZEOFINTBIG, el_tempcluster);
				if (newlamarray == 0) return(0);
				count = 0;
				for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
				{
					if (tech->temp1 == el_curlib->lambda[tech->techindex]) continue;
					techarray[count] = tech;
					newlamarray[count] = tech->temp1;
					count++;
				}
				oldlam = el_curlib->lambda[el_curtech->techindex];
				newlam = el_curtech->temp1;

				/* save highlighting */
				us_pushhighlight();
				us_clearhighlightcount();

				changelambda(count, techarray, newlamarray, el_curlib, i);
				us_setlambda(NOWINDOWFRAME);
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				{
					if (w->curnodeproto == NONODEPROTO) continue;
					if (w->curnodeproto->tech != el_curtech) continue;
					if (i == 2 || (i == 1 && w->curnodeproto->lib == el_curlib))
					{
						w->screenlx = muldiv(w->screenlx, newlam, oldlam);
						w->screenhx = muldiv(w->screenhx, newlam, oldlam);
						w->screenly = muldiv(w->screenly, newlam, oldlam);
						w->screenhy = muldiv(w->screenhy, newlam, oldlam);
						computewindowscale(w);
					} else us_redisplay(w);
				}

				/* restore highlighting */
				us_pophighlight(FALSE);
				efree((CHAR *)techarray);
				efree((CHAR *)newlamarray);
			}
		}
	}
	DiaDoneDialog(dia);
	return(0);
}

static TECHNOLOGY *us_curtechlambda;

BOOLEAN us_topoftechlambda(CHAR **c)
{
	Q_UNUSED( c );
	us_curtechlambda = el_technologies;
	return(TRUE);
}

CHAR *us_nexttechlambda(void)
{
	float lambdainmicrons;
	REGISTER TECHNOLOGY *tech;
	REGISTER void *infstr;

	for(;;)
	{
		if (us_curtechlambda == NOTECHNOLOGY) return(0);
		tech = us_curtechlambda;
		us_curtechlambda = us_curtechlambda->nexttechnology;
		if (tech != sch_tech && tech != art_tech && tech != gen_tech) break;
	}
	infstr = initinfstr();
	lambdainmicrons = scaletodispunit(tech->temp1, DISPUNITMIC);
	formatinfstr(infstr, x_("%s (lambda=%ld, %gu)"), tech->techname,
		tech->temp1, lambdainmicrons);
	return(returninfstr(infstr));
}

/* Prompt: different electrical unit */
static DIALOGITEM us_elecunitdialogitems[] =
{
 /*  1 */ {0, {208,212,232,352}, BUTTON, N_("Use new units")},
 /*  2 */ {0, {208,20,232,160}, BUTTON, N_("Use former units")},
 /*  3 */ {0, {8,8,24,276}, MESSAGE, N_("Warning: displayed units have changed")},
 /*  4 */ {0, {36,8,52,108}, MESSAGE, N_("Formerly:")},
 /*  5 */ {0, {36,200,52,358}, MESSAGE, N_("New library:")},
 /*  6 */ {0, {60,16,76,166}, MESSAGE, x_("")},
 /*  7 */ {0, {60,208,76,358}, MESSAGE, x_("")},
 /*  8 */ {0, {84,16,100,166}, MESSAGE, x_("")},
 /*  9 */ {0, {84,208,100,358}, MESSAGE, x_("")},
 /* 10 */ {0, {108,16,124,166}, MESSAGE, x_("")},
 /* 11 */ {0, {108,208,124,358}, MESSAGE, x_("")},
 /* 12 */ {0, {132,16,148,166}, MESSAGE, x_("")},
 /* 13 */ {0, {132,208,148,358}, MESSAGE, x_("")},
 /* 14 */ {0, {156,16,172,166}, MESSAGE, x_("")},
 /* 15 */ {0, {156,208,172,358}, MESSAGE, x_("")},
 /* 16 */ {0, {180,16,196,166}, MESSAGE, x_("")},
 /* 17 */ {0, {180,208,196,358}, MESSAGE, x_("")},
 /* 18 */ {0, {60,168,76,204}, MESSAGE, x_("")},
 /* 19 */ {0, {84,168,100,204}, MESSAGE, x_("")},
 /* 20 */ {0, {108,168,124,204}, MESSAGE, x_("")},
 /* 21 */ {0, {132,168,148,204}, MESSAGE, x_("")},
 /* 22 */ {0, {156,168,172,204}, MESSAGE, x_("")},
 /* 23 */ {0, {180,168,196,204}, MESSAGE, x_("")}
};
static DIALOG us_elecunitdialog = {{75,75,316,442}, 0, 0, 23, us_elecunitdialogitems, 0, 0};

#define DAEU_USENEW       1		/* use new units (button) */
#define DAEU_OLDNEW       2		/* use old units (button) */
#define DAEU_NEWLIBNAME   5		/* new library name (stat text) */
#define DAEU_OLDRES       6		/* old resistance unit (stat text) */
#define DAEU_NEWRES       7		/* new resistance unit (stat text) */
#define DAEU_OLDCAP       8		/* old capacitance unit (stat text) */
#define DAEU_NEWCAP       9		/* new capacitance unit (stat text) */
#define DAEU_OLDIND      10		/* old inductance unit (stat text) */
#define DAEU_NEWIND      11		/* new inductance unit (stat text) */
#define DAEU_OLDCUR      12		/* old current unit (stat text) */
#define DAEU_NEWCUR      13		/* new current unit (stat text) */
#define DAEU_OLDVOLT     14		/* old voltage unit (stat text) */
#define DAEU_NEWVOLT     15		/* new voltage unit (stat text) */
#define DAEU_OLDTIME     16		/* old time unit (stat text) */
#define DAEU_NEWTIME     17		/* new time unit (stat text) */
#define DAEU_DIFFRES     18		/* if resistance units differ (stat text) */
#define DAEU_DIFFCAP     19		/* if capacitance units differ (stat text) */
#define DAEU_DIFFIND     20		/* if inductance units differ (stat text) */
#define DAEU_DIFFCUR     21		/* if current units differ (stat text) */
#define DAEU_DIFFVOLT    22		/* if voltage units differ (stat text) */
#define DAEU_DIFFTIME    23		/* if time units differ (stat text) */

/*
 * Routine to warn that the electrical units have changed, and one set should be chosen.
 */
void us_adjustelectricalunits(LIBRARY *lib, INTBIG newunits)
{
	REGISTER INTBIG itemHit;
	void *infstr;
	REGISTER void *dia;

	dia = DiaInitDialog(&us_elecunitdialog);
	if (dia == 0) return;
	infstr = initinfstr();
	formatinfstr(infstr, x_("Library %s:"), lib->libname);
	DiaSetText(dia, DAEU_NEWLIBNAME, returninfstr(infstr));
	DiaSetText(dia, DAEU_OLDRES,
		TRANSLATE(us_resistancenames[(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH]));
	DiaSetText(dia, DAEU_NEWRES,
		TRANSLATE(us_resistancenames[(newunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH]));
	if ((us_electricalunits&INTERNALRESUNITS) != (newunits&INTERNALRESUNITS))
		DiaSetText(dia, DAEU_DIFFRES, x_("<-->"));

	DiaSetText(dia, DAEU_OLDCAP,
		TRANSLATE(us_capacitancenames[(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH]));
	DiaSetText(dia, DAEU_NEWCAP,
		TRANSLATE(us_capacitancenames[(newunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH]));
	if ((us_electricalunits&INTERNALCAPUNITS) != (newunits&INTERNALCAPUNITS))
		DiaSetText(dia, DAEU_DIFFCAP, x_("<-->"));

	DiaSetText(dia, DAEU_OLDIND,
		TRANSLATE(us_inductancenames[(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH]));
	DiaSetText(dia, DAEU_NEWIND,
		TRANSLATE(us_inductancenames[(newunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH]));
	if ((us_electricalunits&INTERNALINDUNITS) != (newunits&INTERNALINDUNITS))
		DiaSetText(dia, DAEU_DIFFIND, x_("<-->"));

	DiaSetText(dia, DAEU_OLDCUR,
		TRANSLATE(us_currentnames[(us_electricalunits&INTERNALCURUNITS) >> INTERNALCURUNITSSH]));
	DiaSetText(dia, DAEU_NEWCUR,
		TRANSLATE(us_currentnames[(newunits&INTERNALCURUNITS) >> INTERNALCURUNITSSH]));
	if ((us_electricalunits&INTERNALCURUNITS) != (newunits&INTERNALCURUNITS))
		DiaSetText(dia, DAEU_DIFFCUR, x_("<-->"));

	DiaSetText(dia, DAEU_OLDVOLT,
		TRANSLATE(us_voltagenames[(us_electricalunits&INTERNALVOLTUNITS) >> INTERNALVOLTUNITSSH]));
	DiaSetText(dia, DAEU_NEWVOLT,
		TRANSLATE(us_voltagenames[(newunits&INTERNALVOLTUNITS) >> INTERNALVOLTUNITSSH]));
	if ((us_electricalunits&INTERNALVOLTUNITS) != (newunits&INTERNALVOLTUNITS))
		DiaSetText(dia, DAEU_DIFFVOLT, x_("<-->"));

	DiaSetText(dia, DAEU_OLDTIME,
		TRANSLATE(us_timenames[(us_electricalunits&INTERNALTIMEUNITS) >> INTERNALTIMEUNITSSH]));
	DiaSetText(dia, DAEU_NEWTIME,
		TRANSLATE(us_timenames[(newunits&INTERNALTIMEUNITS) >> INTERNALTIMEUNITSSH]));
	if ((us_electricalunits&INTERNALTIMEUNITS) != (newunits&INTERNALTIMEUNITS))
		DiaSetText(dia, DAEU_DIFFTIME, x_("<-->"));
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DAEU_USENEW || itemHit == DAEU_OLDNEW) break;
	}
	if (itemHit == DAEU_USENEW)
	{
		us_electricalunits = newunits;
		nextchangequiet();
		setvalkey((INTBIG)us_tool, VTOOL, us_electricalunitskey, us_electricalunits, VINTEGER);
	}
	DiaDoneDialog(dia);
}

/****************************** VARIABLES DIALOG ******************************/

/* Variables */
static DIALOGITEM us_variabledialogitems[] =
{
 /*  1 */ {0, {408,344,432,400}, BUTTON, N_("OK")},
 /*  2 */ {0, {352,8,368,56}, MESSAGE, N_("Value:")},
 /*  3 */ {0, {336,8,337,408}, DIVIDELINE, x_("")},
 /*  4 */ {0, {24,8,40,64}, MESSAGE, N_("Object:")},
 /*  5 */ {0, {8,80,24,240}, RADIO, N_("Currently Highlighted")},
 /*  6 */ {0, {56,256,72,408}, RADIO, N_("Current Constraint")},
 /*  7 */ {0, {24,80,40,240}, RADIO, N_("Current Cell")},
 /*  8 */ {0, {40,80,56,240}, RADIO, N_("Current Library")},
 /*  9 */ {0, {8,256,24,408}, RADIO, N_("Current Technology")},
 /* 10 */ {0, {24,256,40,408}, RADIO, N_("Current Tool")},
 /* 11 */ {0, {144,24,160,96}, MESSAGE, N_("Attribute:")},
 /* 12 */ {0, {160,8,304,184}, SCROLL, x_("")},
 /* 13 */ {0, {312,32,328,152}, CHECK, N_("New Attribute:")},
 /* 14 */ {0, {312,160,328,400}, EDITTEXT, x_("")},
 /* 15 */ {0, {216,192,232,251}, CHECK, N_("Array")},
 /* 16 */ {0, {240,200,256,248}, MESSAGE, N_("Index:")},
 /* 17 */ {0, {240,250,256,312}, EDITTEXT, x_("")},
 /* 18 */ {0, {408,192,432,296}, BUTTON, N_("Set Attribute")},
 /* 19 */ {0, {344,80,376,400}, EDITTEXT, x_("")},
 /* 20 */ {0, {408,24,432,144}, BUTTON, N_("Delete Attribute")},
 /* 21 */ {0, {168,192,184,288}, CHECK, N_("Displayable")},
 /* 22 */ {0, {192,192,208,288}, CHECK, N_("Temporary")},
 /* 23 */ {0, {276,224,300,361}, BUTTON, N_("Examine Attribute")},
 /* 24 */ {0, {112,40,128,80}, MESSAGE, N_("Type:")},
 /* 25 */ {0, {112,80,128,216}, MESSAGE, x_("")},
 /* 26 */ {0, {144,184,160,224}, MESSAGE, N_("Type:")},
 /* 27 */ {0, {144,224,160,383}, MESSAGE, x_("")},
 /* 28 */ {0, {136,8,137,408}, DIVIDELINE, x_("")},
 /* 29 */ {0, {80,80,112,408}, MESSAGE, x_("")},
 /* 30 */ {0, {80,32,96,80}, MESSAGE, N_("Name:")},
 /* 31 */ {0, {168,304,184,410}, POPUP, x_("")},
 /* 32 */ {0, {384,80,400,160}, MESSAGE, N_("Evaluation:")},
 /* 33 */ {0, {384,160,400,400}, MESSAGE, x_("")},
 /* 34 */ {0, {232,320,248,366}, BUTTON, N_("Next")},
 /* 35 */ {0, {248,320,264,366}, BUTTON, N_("Prev")},
 /* 36 */ {0, {40,256,56,408}, RADIO, N_("Current Window")}
};
static DIALOG us_variabledialog = {{50,75,492,495}, N_("Variable Control"), 0, 36, us_variabledialogitems, 0, 0};

/* special items for the "variables" dialog: */
#define DVAR_CURHIGHOBJECT  5		/* Currently highlighted object (radio) */
#define DVAR_CURCONSTRAINT  6		/* Current constraint (radio) */
#define DVAR_CURCELL        7		/* Current cell (radio) */
#define DVAR_CURLIB         8		/* Current library (radio) */
#define DVAR_CURTECH        9		/* Current technology (radio) */
#define DVAR_CURTOOL       10		/* Current tool (radio) */
#define DVAR_ATTRLIST      12		/* Attribute name list (scroll) */
#define DVAR_NEWATTR       13		/* New Attribute (check) */
#define DVAR_NEWATTRNAME   14		/* New Attribute name (edit text) */
#define DVAR_ATTRARRAY     15		/* Array attribute (check) */
#define DVAR_ARRAYINDEX_L  16		/* Array index label (stat text) */
#define DVAR_ARRAYINDEX    17		/* Array index (edit text) */
#define DVAR_SETATTR       18		/* Set Attribute (button) */
#define DVAR_ATTRVALUE     19		/* Attribute value (edit text) */
#define DVAR_DELATTR       20		/* Delete Attribute (button) */
#define DVAR_DISPLAYABLE   21		/* Displayable attribute (check) */
#define DVAR_TEMPORARY     22		/* Temporary attribute (check) */
#define DVAR_EXAMINE       23		/* Examine (button) */
#define DVAR_OBJTYPE       25		/* Object type (stat text) */
#define DVAR_ATTRTYPE      27		/* Attribute type (stat text) */
#define DVAR_OBJNAME       29		/* Object name (stat text) */
#define DVAR_LANGUAGE      31		/* Attribute language (popup) */
#define DVAR_EVALUATION_L  32		/* Evaluation label (stat text) */
#define DVAR_EVALUATION    33		/* Evaluation (stat text) */
#define DVAR_NEXTINDEX     34		/* Current constraint (button) */
#define DVAR_PREVINDEX     35		/* Current constraint (button) */
#define DVAR_CURWINDOW     36		/* Current window (radio) */

static INTBIG    us_possearch, us_varaddr, us_curvaraddr;
static INTBIG    us_vartype, us_curvartype, us_varlength;
static VARIABLE *us_variablesvar;

static BOOLEAN us_topofdlgvars(CHAR**);
static CHAR   *us_nextdlgvars(void);
static void   us_varestablish(INTBIG, INTBIG, CHAR*, void*);
static void   us_varidentify(void *dia);

BOOLEAN us_topofdlgvars(CHAR **c)
{
	Q_UNUSED( c );
	us_possearch = initobjlist(us_varaddr, us_vartype, FALSE);
	return(TRUE);
}
CHAR *us_nextdlgvars(void)
{
	VARIABLE *var;

	return(nextobjectlist(&var, us_possearch));
}

INTBIG us_variablesdlog(void)
{
	INTBIG itemHit, i, j, which, newtype, newaddr;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER INTBIG search, newval, *newarray, oldlen;
	CHAR *varname, *name, *newstr, line[100], **languages;
	HIGHLIGHT high;
	REGISTER GEOM *geom;
	VARIABLE *var;
	NODEPROTO *curcell;
	REGISTER void *dia;
	static CHAR nullstr[] = {x_("")};

	/* display the variables dialog box */
	dia = DiaInitDialog(&us_variabledialog);
	if (dia == 0) return(0);
	languages = us_languagechoices();
	DiaSetPopup(dia, DVAR_LANGUAGE, 4, languages);
	DiaNoEditControl(dia, DVAR_NEWATTRNAME);
	DiaNoEditControl(dia, DVAR_ARRAYINDEX);

	/* presume the current cell or library */
	curcell = getcurcell();
	us_varaddr = (INTBIG)curcell;
	us_vartype = VNODEPROTO;
	if (curcell == NONODEPROTO)
	{
		/* no current cell: cannot select cell */
		DiaDimItem(dia, DVAR_CURCELL);
		us_varaddr = (INTBIG)el_curlib;
		us_vartype = VLIBRARY;
	} else
	{
		DiaUnDimItem(dia, DVAR_CURCELL);
	}

	/* see if a single object is highlighted */
	us_curvaraddr = 0;
	us_curvartype = VUNKNOWN;
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		if (getlength(var) == 1)
		{
			if (!us_makehighlight(((CHAR **)var->addr)[0], &high))
			{
				if ((high.status&HIGHTYPE) == HIGHFROM)
				{
					if (!high.fromgeom->entryisnode) us_vartype = VARCINST; else
						us_vartype = VNODEINST;
					us_curvaraddr = us_varaddr = (INTBIG)high.fromgeom->entryaddr.blind;
					us_curvartype = us_vartype;
				}
			}
		}
	}
	if (us_vartype == VNODEPROTO || us_vartype == VLIBRARY)
		DiaDimItem(dia, DVAR_CURHIGHOBJECT); else
			DiaUnDimItem(dia, DVAR_CURHIGHOBJECT);

	/* initialize the attribute list */
	DiaInitTextDialog(dia, DVAR_ATTRLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCREPORT);
	us_varestablish(us_varaddr, us_vartype, x_(""), dia);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == DVAR_SETATTR ||
			itemHit == DVAR_DELATTR || itemHit == CANCEL) break;
		if (itemHit == DVAR_ATTRLIST)
		{
			/* select a new attribute name */
			us_varidentify(dia);
			continue;
		}
		if (itemHit == DVAR_NEWATTR)
		{
			/* want a new variable */
			DiaSetText(dia, DVAR_NEWATTRNAME, x_(""));
			i = DiaGetControl(dia, DVAR_NEWATTR);
			DiaSetControl(dia, DVAR_NEWATTR, 1-i);
			if (i != 0) DiaNoEditControl(dia, DVAR_NEWATTRNAME); else
				DiaEditControl(dia, DVAR_NEWATTRNAME);
			DiaSetText(dia, DVAR_ATTRVALUE, x_(""));
			DiaSetText(dia, -DVAR_NEWATTRNAME, x_(""));
			DiaUnDimItem(dia, DVAR_ATTRARRAY);
			DiaUnDimItem(dia, DVAR_DISPLAYABLE);
			DiaUnDimItem(dia, DVAR_TEMPORARY);
			DiaUnDimItem(dia, DVAR_LANGUAGE);
			DiaDimItem(dia, DVAR_EXAMINE);
			DiaDimItem(dia, DVAR_DELATTR);
			DiaUnDimItem(dia, DVAR_SETATTR);
			continue;
		}
		if (itemHit == DVAR_EXAMINE)
		{
			/* examine the variable */
			which = DiaGetCurLine(dia, DVAR_ATTRLIST);
			if (which < 0) continue;
			varname = DiaGetScrollLine(dia, DVAR_ATTRLIST, which);
			search = initobjlist(us_varaddr, us_vartype, FALSE);
			for(;;)
			{
				name = nextobjectlist(&var, search);
				if (name == 0) break;
				if (estrcmp(name, varname) == 0)
				{
					if ((var->type&VISARRAY) != 0)
					{
						i = myatoi(DiaGetText(dia, DVAR_ARRAYINDEX));
						if (i < 0) i = 0;
						if (i > getlength(var)) i = getlength(var)-1;
						(void)esnprintf(line, 100, x_("%s[%ld]"), varname, i);
						us_varestablish(((INTBIG *)var->addr)[i], var->type&VTYPE, line, dia);
					} else us_varestablish(var->addr, var->type&VTYPE, varname, dia);
					break;
				}
			}
			continue;
		}
		if (itemHit == DVAR_ARRAYINDEX)
		{
			/* changing index */
			if (DiaGetControl(dia, DVAR_NEWATTR) != 0) continue;
			i = myatoi(DiaGetText(dia, DVAR_ARRAYINDEX));
			if (i < 0) i = 0;
			if (i > 0 && i >= us_varlength)
			{
				i = us_varlength-1;
				(void)esnprintf(line, 100, x_("%ld"), i);
				DiaSetText(dia, DVAR_ARRAYINDEX, line);
			}
			DiaSetText(dia, DVAR_ATTRVALUE, describevariable(us_variablesvar, i, -1));
			continue;
		}
		if (itemHit == DVAR_NEXTINDEX)
		{
			/* increment index */
			i = myatoi(DiaGetText(dia, DVAR_ARRAYINDEX)) + 1;
			if (DiaGetControl(dia, DVAR_NEWATTR) == 0)
			{
				if (i >= us_varlength) i = us_varlength-1;
			}
			(void)esnprintf(line, 100, _("%ld"), i);
			DiaSetText(dia, DVAR_ARRAYINDEX, line);
			if (DiaGetControl(dia, DVAR_NEWATTR) == 0)
				DiaSetText(dia, DVAR_ATTRVALUE, describevariable(us_variablesvar, i, -1));
			continue;
		}
		if (itemHit == DVAR_PREVINDEX)
		{
			/* decrement index */
			i = myatoi(DiaGetText(dia, DVAR_ARRAYINDEX)) - 1;
			if (i < 0) continue;
			(void)esnprintf(line, 100, x_("%ld"), i);
			DiaSetText(dia, DVAR_ARRAYINDEX, line);
			if (DiaGetControl(dia, DVAR_NEWATTR) == 0)
				DiaSetText(dia, DVAR_ATTRVALUE, describevariable(us_variablesvar, i, -1));
			continue;
		}
		if (itemHit == DVAR_CURHIGHOBJECT)
		{
			/* want current object */
			us_varestablish(us_curvaraddr, us_curvartype, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_CURCELL)
		{
			/* want current cell */
			us_varestablish((INTBIG)curcell, VNODEPROTO, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_CURLIB)
		{
			/* want current library */
			us_varestablish((INTBIG)el_curlib, VLIBRARY, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_CURTECH)
		{
			/* want current technology */
			us_varestablish((INTBIG)el_curtech, VTECHNOLOGY, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_CURTOOL)
		{
			/* want current tool */
			us_varestablish((INTBIG)us_tool, VTOOL, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_CURWINDOW)
		{
			/* want current window */
			us_varestablish((INTBIG)el_curwindowpart, VWINDOWPART, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_CURCONSTRAINT)
		{
			/* want current constraint */
			us_varestablish((INTBIG)el_curconstraint, VCONSTRAINT, x_(""), dia);
			continue;
		}
		if (itemHit == DVAR_DISPLAYABLE)
		{
			/* displayable attribute */
			DiaSetControl(dia, DVAR_DISPLAYABLE, 1-DiaGetControl(dia, DVAR_DISPLAYABLE));
			continue;
		}
		if (itemHit == DVAR_TEMPORARY)
		{
			/* temporary attribute */
			DiaSetControl(dia, DVAR_TEMPORARY, 1-DiaGetControl(dia, DVAR_TEMPORARY));
			continue;
		}
		if (itemHit == DVAR_ATTRARRAY)
		{
			/* array attribute */
			i = DiaGetControl(dia, DVAR_ATTRARRAY);
			DiaSetControl(dia, DVAR_ATTRARRAY, 1-i);
			if (i == 0)
			{
				DiaUnDimItem(dia, DVAR_ARRAYINDEX_L);
				DiaUnDimItem(dia, DVAR_ARRAYINDEX);
				DiaUnDimItem(dia, DVAR_NEXTINDEX);
				DiaUnDimItem(dia, DVAR_PREVINDEX);
				DiaEditControl(dia, DVAR_ARRAYINDEX);
				DiaSetText(dia, DVAR_ARRAYINDEX, x_("0"));
			} else
			{
				DiaDimItem(dia, DVAR_ARRAYINDEX_L);
				DiaDimItem(dia, DVAR_ARRAYINDEX);
				DiaDimItem(dia, DVAR_NEXTINDEX);
				DiaDimItem(dia, DVAR_PREVINDEX);
				DiaNoEditControl(dia, DVAR_ARRAYINDEX);
				DiaSetText(dia, DVAR_ARRAYINDEX, x_(""));
			}
			continue;
		}
	}

	/* handle attribute deletion */
	if (itemHit == DVAR_DELATTR)
	{
		startobjectchange(us_varaddr, us_vartype);
		(void)delvalkey(us_varaddr, us_vartype, us_variablesvar->key);
		endobjectchange(us_varaddr, us_vartype);
	}

	/* handle attribute setting */
	if (itemHit == DVAR_SETATTR)
	{
		/* initialize object to be changed */
		startobjectchange(us_varaddr, us_vartype);
		if (DiaGetControl(dia, DVAR_CURHIGHOBJECT) != 0)
		{
			us_pushhighlight();
			us_clearhighlightcount();
		}

		/* get new attribute string */
		newstr = DiaGetText(dia, DVAR_ATTRVALUE);

		/* determine type of attribute */
		if (DiaGetControl(dia, DVAR_NEWATTR) != 0)
		{
			/* setting new attribute */
			varname = DiaGetText(dia, DVAR_NEWATTRNAME);
			getsimpletype(newstr, &newtype, &newaddr, 0);
			if (DiaGetControl(dia, DVAR_ATTRARRAY) != 0) newtype |= VISARRAY;
		} else
		{
			/* modifying existing attribute */
			which = DiaGetCurLine(dia, DVAR_ATTRLIST);
			if (which >= 0) varname = DiaGetScrollLine(dia, DVAR_ATTRLIST, which); else
				varname = x_("");
			newtype = us_variablesvar->type;
		}
		if (DiaGetControl(dia, DVAR_DISPLAYABLE) != 0) newtype |= VDISPLAY; else
			newtype &= ~VDISPLAY;
		if (DiaGetControl(dia, DVAR_TEMPORARY) != 0) newtype |= VCANTSET; else
			newtype &= ~VCANTSET;
		newtype &= ~(VCODE1 | VCODE2);
		switch (DiaGetPopupEntry(dia, DVAR_LANGUAGE))
		{
			case 1: newtype |= VTCL;    break;
			case 2: newtype |= VLISP;   break;
			case 3: newtype |= VJAVA;   break;
		}

		/* get proper attribute value */
		newval = (INTBIG)newstr;
		if ((newtype&(VCODE1|VCODE2)) == 0)
		{
			switch (newtype&VTYPE)
			{
				case VINTEGER:
				case VADDRESS:
				case VSHORT:
				case VBOOLEAN:
					newval = myatoi(newstr);
					break;
				case VFRACT:
					newval = atofr(newstr);
					break;
				case VFLOAT:
				case VDOUBLE:
					newval = castint((float)eatof(newstr));
					break;
			}
		}

		/* set the attribute if valid */
		if (*varname != 0)
		{
			/* see if it is an array attribute */
			if (DiaGetControl(dia, DVAR_ATTRARRAY) != 0)
			{
				/* get the array index, examine former attribute */
				which = myatoi(DiaGetText(dia, DVAR_ARRAYINDEX));
				if (which < 0) which = 0;
				var = getval(us_varaddr, us_vartype, -1, varname);
				if (var == NOVARIABLE)
				{
					/* attribute did not exist: create the array */
					newarray = emalloc(((which+1) * SIZEOFINTBIG), el_tempcluster);
					if (newarray == 0) return(0);
					for(j=0; j<=which; j++) newarray[j] = newval;
					newtype |= VISARRAY | ((which+1)<<VLENGTHSH);
					var = setval(us_varaddr, us_vartype, varname, (INTBIG)newarray, newtype);
					if (var != NOVARIABLE)
					{
						if (us_vartype == VARCINST) geom = ((ARCINST *)us_varaddr)->geom; else
							if (us_vartype == VNODEINST) geom = ((NODEINST *)us_varaddr)->geom; else
								geom = NOGEOM;
						defaulttextdescript(var->textdescript, geom);
					}
					efree((CHAR *)newarray);
				} else if (getlength(var) <= which)
				{
					/* extend existing array attribute */
					oldlen = getlength(var);
					newarray = (INTBIG *)emalloc(((which+1) * SIZEOFINTBIG), el_tempcluster);
					if (newarray == 0) return(0);
					if ((newtype&VTYPE) == VSTRING)
					{
						for(j=0; j<oldlen; j++)
							(void)allocstring((CHAR **)&newarray[j],
								(CHAR *)((INTBIG *)var->addr)[j], el_tempcluster);
						for(j=oldlen; j<which; j++) newarray[j] = (INTBIG)nullstr;
					} else
					{
						for(j=0; j<oldlen; j++) newarray[j] = ((INTBIG *)var->addr)[j];
						for(j=oldlen; j<which; j++) newarray[j] = 0;
					}
					newarray[which] = newval;
					newtype = (newtype & ~VLENGTH) | ((which+1)<<VLENGTHSH);
					(void)setval(us_varaddr, us_vartype, varname, (INTBIG)newarray, newtype);
					if ((newtype&VTYPE) == VSTRING)
						for(j=0; j<oldlen; j++) efree((CHAR *)newarray[j]);
					efree((CHAR *)newarray);
				} else
				{
					/* set a single attribute entry */
					(void)setind(us_varaddr, us_vartype, varname, which, newval);
				}
			} else
			{
				/* setting non-array or code attribute */
				var = getval(us_varaddr, us_vartype, newtype, varname);
				if (var != NOVARIABLE) TDCOPY(descript, var->textdescript); else
				{
					if (us_vartype == VARCINST) geom = ((ARCINST *)us_varaddr)->geom; else
						if (us_vartype == VNODEINST) geom = ((NODEINST *)us_varaddr)->geom; else
							geom = NOGEOM;
					TDCLEAR(descript);
					defaulttextdescript(descript, geom);
				}
				var = setval(us_varaddr, us_vartype, varname, newval, newtype);
				if (var != NOVARIABLE) TDCOPY(var->textdescript, descript);
			}
		}

		/* finish the change */
		if (DiaGetControl(dia, DVAR_CURHIGHOBJECT) != 0) us_pophighlight(TRUE);
		endobjectchange(us_varaddr, us_vartype);
	}
	DiaDoneDialog(dia);
	return(0);
}

void us_varestablish(INTBIG addr, INTBIG type, CHAR *thisname, void *dia)
{
	REGISTER void *infstr;

	/* determine which radio button to set */
	DiaSetControl(dia, DVAR_CURHIGHOBJECT, 0);
	DiaSetControl(dia, DVAR_CURCELL, 0);
	DiaSetControl(dia, DVAR_CURLIB, 0);
	DiaSetControl(dia, DVAR_CURTECH, 0);
	DiaSetControl(dia, DVAR_CURTOOL, 0);
	DiaSetControl(dia, DVAR_CURWINDOW, 0);
	DiaSetControl(dia, DVAR_CURCONSTRAINT, 0);
	if (us_curvaraddr != 0 && addr == us_curvaraddr && type == us_curvartype)
	{
		DiaSetControl(dia, DVAR_CURHIGHOBJECT, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("~"));
		thisname = x_("");
	} else if (getcurcell() != NONODEPROTO && addr == (INTBIG)getcurcell() && type == VNODEPROTO)
	{
		DiaSetControl(dia, DVAR_CURCELL, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("cell:~"));
		thisname = x_("");
	} else if (addr == (INTBIG)el_curlib && type == VLIBRARY)
	{
		DiaSetControl(dia, DVAR_CURLIB, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("lib:~"));
		thisname = x_("");
	} else if (addr == (INTBIG)el_curtech && type == VTECHNOLOGY)
	{
		DiaSetControl(dia, DVAR_CURTECH, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("tech:~"));
		thisname = x_("");
	} else if (addr == (INTBIG)us_tool && type == VTOOL)
	{
		DiaSetControl(dia, DVAR_CURTOOL, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("tool:~"));
		thisname = x_("");
	} else if (addr == (INTBIG)el_curwindowpart && type == VWINDOWPART)
	{
		DiaSetControl(dia, DVAR_CURWINDOW, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("window:~"));
		thisname = x_("");
	} else if (addr == (INTBIG)el_curconstraint && type == VCONSTRAINT)
	{
		DiaSetControl(dia, DVAR_CURCONSTRAINT, 1);
		DiaSetText(dia, DVAR_OBJNAME, x_("constraint:~"));
		thisname = x_("");
	}

	if (*thisname != 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, DiaGetText(dia, DVAR_OBJNAME));
		addtoinfstr(infstr, '.');
		addstringtoinfstr(infstr, thisname);
		DiaSetText(dia, DVAR_OBJNAME, returninfstr(infstr));
	}
	us_varaddr = addr;
	us_vartype = type;
	DiaSetText(dia, DVAR_OBJTYPE, us_variabletypename(type));
	DiaLoadTextDialog(dia, DVAR_ATTRLIST, us_topofdlgvars, us_nextdlgvars, DiaNullDlogDone, 0);
	us_varidentify(dia);
}

void us_varidentify(void *dia)
{
	INTBIG which, aindex, type;
	REGISTER INTBIG search, language;
	CHAR *varname, *name;
	VARIABLE *var;

	DiaSetControl(dia, DVAR_NEWATTR, 0);
	DiaSetText(dia, DVAR_NEWATTRNAME, x_(""));
	DiaNoEditControl(dia, DVAR_NEWATTRNAME);
	DiaDimItem(dia, DVAR_ATTRARRAY);
	DiaDimItem(dia, DVAR_LANGUAGE);
	DiaDimItem(dia, DVAR_SETATTR);
	DiaDimItem(dia, DVAR_DELATTR);
	DiaDimItem(dia, DVAR_DISPLAYABLE);
	DiaDimItem(dia, DVAR_TEMPORARY);
	DiaDimItem(dia, DVAR_EXAMINE);
	DiaDimItem(dia, DVAR_ARRAYINDEX_L);
	DiaDimItem(dia, DVAR_NEXTINDEX);
	DiaDimItem(dia, DVAR_PREVINDEX);
	which = DiaGetCurLine(dia, DVAR_ATTRLIST);
	if (which < 0) return;
	varname = DiaGetScrollLine(dia, DVAR_ATTRLIST, which);
	search = initobjlist(us_varaddr, us_vartype, FALSE);
	for(;;)
	{
		name = nextobjectlist(&us_variablesvar, search);
		if (name == 0) break;
		if (estrcmp(name, varname) == 0) break;
	}
	if (name == 0) return;

	us_varlength = getlength(us_variablesvar);
	type = us_variablesvar->type&VTYPE;
	if ((us_variablesvar->type&VCREF) == 0)
	{
		DiaUnDimItem(dia, DVAR_DELATTR);
		DiaUnDimItem(dia, DVAR_DISPLAYABLE);
		DiaUnDimItem(dia, DVAR_TEMPORARY);
		DiaUnDimItem(dia, DVAR_LANGUAGE);
	}
	if ((us_variablesvar->type&VCANTSET) == 0) DiaUnDimItem(dia, DVAR_SETATTR);
	DiaSetText(dia, DVAR_ATTRTYPE, us_variabletypename(type));
	if ((us_variablesvar->type&VDISPLAY) != 0) DiaSetControl(dia, DVAR_DISPLAYABLE, 1); else
		DiaSetControl(dia, DVAR_DISPLAYABLE, 0);
	if ((us_variablesvar->type&VDONTSAVE) != 0) DiaSetControl(dia, DVAR_TEMPORARY, 1); else
		DiaSetControl(dia, DVAR_TEMPORARY, 0);
	language = us_variablesvar->type & (VCODE1|VCODE2);
	switch (language)
	{
		case 0:     DiaSetPopupEntry(dia, DVAR_LANGUAGE, 0);   break;
		case VTCL:  DiaSetPopupEntry(dia, DVAR_LANGUAGE, 1);   break;
		case VLISP: DiaSetPopupEntry(dia, DVAR_LANGUAGE, 2);   break;
		case VJAVA: DiaSetPopupEntry(dia, DVAR_LANGUAGE, 3);   break;
	}
	if ((us_variablesvar->type&VISARRAY) != 0)
	{
		DiaSetControl(dia, DVAR_ATTRARRAY, 1);
		DiaUnDimItem(dia, DVAR_ARRAYINDEX_L);
		DiaUnDimItem(dia, DVAR_NEXTINDEX);
		DiaUnDimItem(dia, DVAR_PREVINDEX);
		DiaSetText(dia, DVAR_ARRAYINDEX, x_("0"));
		DiaEditControl(dia, DVAR_ARRAYINDEX);
		aindex = 0;
	} else
	{
		DiaSetControl(dia, DVAR_ATTRARRAY, 0);
		DiaSetText(dia, DVAR_ARRAYINDEX, x_(""));
		DiaNoEditControl(dia, DVAR_ARRAYINDEX);
		aindex = -1;
	}
	DiaSetText(dia, DVAR_ATTRVALUE, describevariable(us_variablesvar, aindex, -1));
	if (language != 0)
	{
		var = doquerry((CHAR *)us_variablesvar->addr, language, us_variablesvar->type & ~(VCODE1|VCODE2));
		if (var != NOVARIABLE)
			TDCOPY(var->textdescript, us_variablesvar->textdescript);
		DiaSetText(dia, DVAR_EVALUATION, describevariable(var, aindex, -1));
		DiaSetText(dia, DVAR_EVALUATION_L, _("Evaluation:"));
	} else
	{
		DiaSetText(dia, DVAR_EVALUATION, x_(""));
		DiaSetText(dia, DVAR_EVALUATION_L, x_(""));
	}
	if (type != VUNKNOWN && type != VINTEGER && type != VADDRESS && type != VCHAR &&
		type != VSTRING && type != VFLOAT && type != VDOUBLE && type != VFRACT && type != VSHORT && type != VBOOLEAN)
			DiaUnDimItem(dia, DVAR_EXAMINE);
}

/****************************** VIEW CREATION DIALOG ******************************/

/* New view */
static DIALOGITEM us_newviewdialogitems[] =
{
 /*  1 */ {0, {64,232,88,304}, BUTTON, N_("OK")},
 /*  2 */ {0, {64,16,88,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,145}, MESSAGE, N_("New view name:")},
 /*  4 */ {0, {32,8,48,145}, MESSAGE, N_("View abbreviation:")},
 /*  5 */ {0, {8,148,24,304}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,148,48,304}, EDITTEXT, x_("")},
 /*  7 */ {0, {68,104,84,213}, CHECK, N_("Textual View")}
};
static DIALOG us_newviewdialog = {{50,75,154,391}, N_("New View"), 0, 7, us_newviewdialogitems, 0, 0};

/* special items for the "new view" dialog: */
#define DNVW_VIEWNAME    5		/* New view name (edit text) */
#define DNVW_VIEWABBR    6		/* View abbreviation (edit text) */
#define DNVW_TEXTVIEW    7		/* Textual view (check) */

INTBIG us_newviewdlog(CHAR *paramstart[])
{
	INTBIG itemHit;
	REGISTER void *infstr;
	REGISTER void *dia;

	/* display the port dialog box */
	dia = DiaInitDialog(&us_newviewdialog);
	if (dia == 0) return(0);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK && DiaValidEntry(dia, DNVW_VIEWNAME) && DiaValidEntry(dia, DNVW_VIEWABBR)) break;
		if (itemHit == DNVW_TEXTVIEW)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}
	}

	paramstart[0] = x_("");
	if (itemHit != CANCEL)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, DiaGetText(dia, DNVW_VIEWNAME));
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, DiaGetText(dia, DNVW_VIEWABBR));
		if (DiaGetControl(dia, DNVW_TEXTVIEW) != 0) addstringtoinfstr(infstr, _(" text"));
		if (us_returneddialogstring != 0) efree((CHAR *)us_returneddialogstring);
		allocstring(&us_returneddialogstring, returninfstr(infstr), us_tool->cluster);
		paramstart[0] = us_returneddialogstring;
	}
	DiaDoneDialog(dia);
	return(1);
}

/****************************** VIEW SELECTION DIALOG ******************************/

/* View: Select */
static DIALOGITEM us_viewseldialogitems[] =
{
 /*  1 */ {0, {176,108,200,188}, BUTTON, N_("OK")},
 /*  2 */ {0, {176,8,200,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,168,188}, SCROLL, x_("")}
};
static DIALOG us_viewseldialog = {{75,75,284,272}, N_("Select View"), 0, 3, us_viewseldialogitems, 0, 0};

/* special items for the "view selection" dialog: */
#define DVSL_VIEWLIST     3		/* View list (scroll) */

static INTBIG us_viewlistdeleteonly;
static VIEW  *us_posviewcomcomp;

static BOOLEAN us_diatopofviews(CHAR **c);
static CHAR   *us_dianextviews(void);

BOOLEAN us_diatopofviews(CHAR **c)
{
	Q_UNUSED( c );
	us_posviewcomcomp = el_views;
	return(TRUE);
}

CHAR *us_dianextviews(void)
{
	REGISTER VIEW *v;

	for(;;)
	{
		v = us_posviewcomcomp;
		if (v == NOVIEW) break;
		us_posviewcomcomp = v->nextview;
		if (us_viewlistdeleteonly != 0 &&
			(v->viewstate&PERMANENTVIEW) != 0) continue;
		return(v->viewname);
	}
	return(0);
}

INTBIG us_viewdlog(INTBIG deleteview)
{
	INTBIG itemHit, i;
	CHAR *arg[3];
	REGISTER void *dia;

	/* display the port dialog box */
	us_viewlistdeleteonly = deleteview;
	if (deleteview != 0) us_viewseldialog.movable = _("View to Delete"); else
		us_viewseldialog.movable = _("Select View"); 
	dia = DiaInitDialog(&us_viewseldialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DVSL_VIEWLIST, us_diatopofviews, us_dianextviews, DiaNullDlogDone, 0,
		SCSELMOUSE);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
	}

	if (itemHit != CANCEL)
	{
		i = DiaGetCurLine(dia, DVSL_VIEWLIST);
		if (i >= 0)
		{
			arg[0] = x_("delete");
			arg[1] = DiaGetScrollLine(dia, DVSL_VIEWLIST, i);
			us_view(2, arg);
		}
	}
	DiaDoneDialog(dia);
	return(1);
}

/****************************** VISIBLE LAYERS DIALOG ******************************/

/* Layer Visibility */
static DIALOGITEM us_visiblelaydialogitems[] =
{
 /*  1 */ {0, {32,8,208,222}, SCROLL, x_("")},
 /*  2 */ {0, {8,8,24,82}, MESSAGE, N_("Layer set:")},
 /*  3 */ {0, {8,88,24,294}, POPUP, x_("")},
 /*  4 */ {0, {40,230,56,382}, MESSAGE, N_("Text visibility options:")},
 /*  5 */ {0, {60,242,76,394}, AUTOCHECK, N_("Node text")},
 /*  6 */ {0, {80,242,96,394}, AUTOCHECK, N_("Arc text")},
 /*  7 */ {0, {100,242,116,394}, AUTOCHECK, N_("Port text")},
 /*  8 */ {0, {120,242,136,394}, AUTOCHECK, N_("Export text")},
 /*  9 */ {0, {140,242,156,394}, AUTOCHECK, N_("Nonlayout text")},
 /* 10 */ {0, {160,242,176,394}, AUTOCHECK, N_("Instance names")},
 /* 11 */ {0, {180,242,196,394}, AUTOCHECK, N_("Cell text")},
 /* 12 */ {0, {212,16,228,222}, MESSAGE, N_("Click to change visibility.")},
 /* 13 */ {0, {228,16,244,222}, MESSAGE, N_("Marked layers are visible.")},
 /* 14 */ {0, {252,8,276,108}, BUTTON, N_("All Visible")},
 /* 15 */ {0, {252,122,276,222}, BUTTON, N_("All Invisible")},
 /* 16 */ {0, {214,316,238,388}, BUTTON, N_("Done")},
 /* 17 */ {0, {250,316,274,388}, DEFBUTTON, N_("Apply")}
};
static DIALOG us_visiblelaydialog = {{50,75,335,479}, N_("Layer Visibility"), 0, 17, us_visiblelaydialogitems, x_("visiblelay"), 0};

/* special items for the "visiblelay" dialog: */
#define DVSL_LAYERLIST     1		/* Layers (scroll) */
#define DVSL_LAYERSET_L    2		/* Layer set (message) */
#define DVSL_LAYERSET      3		/* Set of layers (popup) */
#define DVSL_OPTIONS_L     4		/* Text visibility options (message) */
#define DVSL_NODETEXT      5		/* Show node text (autocheck) */
#define DVSL_ARCTEXT       6		/* Show arc text (autocheck) */
#define DVSL_PORTTEXT      7		/* Show port text (autocheck) */
#define DVSL_EXPORTTEXT    8		/* Show export text (autocheck) */
#define DVSL_NONLAYTEXT    9		/* Show nonlayout text (autocheck) */
#define DVSL_INSTTEXT     10		/* Show instance text (autocheck) */
#define DVSL_CELLTEXT     11		/* Show cell text (autocheck) */
#define DVSL_CLICK_L      12		/* Click to change (message) */
#define DVSL_MARKED_L     13		/* Marked layers (message) */
#define DVSL_ALLVISIBLE   14		/* Make all visible (button) */
#define DVSL_ALLINVISIBLE 15		/* Make all invisible (button) */
#define DVSL_DONE         16		/* Done (button) */
#define DVSL_APPLY        17		/* Apply (defbutton) */

#define LAYERSELEC 0		/* Electric layers */
#define LAYERSDXF  1		/* DXF layers */
#define LAYERSGDS  2		/* GDS layers */

class EDiaVisibleLayer : public EDialogModeless
{
public:
	EDiaVisibleLayer();
	void showLayerSet(CHAR *prompt);
	static EDiaVisibleLayer *dialog;
private:
	BOOLEAN changes;

	void itemHitAction(INTBIG itemHit);
	void loadSet();
};

EDiaVisibleLayer *EDiaVisibleLayer::dialog = 0;

INTBIG us_visiblelayersdlog(CHAR *prompt)
{

	if (us_needwindow()) return(0);

	/* display the visibility dialog box */
	if (EDiaVisibleLayer::dialog == 0) EDiaVisibleLayer::dialog = new EDiaVisibleLayer();
	EDiaVisibleLayer::dialog->showLayerSet( prompt );
	return(0);
}

EDiaVisibleLayer::EDiaVisibleLayer()
	: EDialogModeless(&us_visiblelaydialog)
{
}

void EDiaVisibleLayer::showLayerSet( CHAR * prompt )
{
	CHAR *layersets[3];

	show();
	layersets[LAYERSELEC] = _("Electric layers");
	layersets[LAYERSDXF] = _("DXF layers");
	layersets[LAYERSGDS] = _("GDS layers");
	setPopup(DVSL_LAYERSET, 3, layersets);
	if (namesame(prompt, x_("DXF")) == 0) setPopupEntry(DVSL_LAYERSET, LAYERSDXF); else
		if (namesame(prompt, x_("GDS")) == 0) setPopupEntry(DVSL_LAYERSET, LAYERSGDS);
	initTextDialog(DVSL_LAYERLIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE|SCREPORT);
	loadSet();
	if ((us_useroptions&HIDETXTNODE)     == 0) setControl(DVSL_NODETEXT, 1);
	if ((us_useroptions&HIDETXTARC)      == 0) setControl(DVSL_ARCTEXT, 1);
	if ((us_useroptions&HIDETXTPORT)     == 0) setControl(DVSL_PORTTEXT, 1);
	if ((us_useroptions&HIDETXTEXPORT)   == 0) setControl(DVSL_EXPORTTEXT, 1);
	if ((us_useroptions&HIDETXTNONLAY)   == 0) setControl(DVSL_NONLAYTEXT, 1);
	if ((us_useroptions&HIDETXTINSTNAME) == 0) setControl(DVSL_INSTTEXT, 1);
	if ((us_useroptions&HIDETXTCELL)    == 0) setControl(DVSL_CELLTEXT, 1);
	changes = FALSE;
}

/*
 * Coroutine to handle hits in the modeless "layer visibility" dialog.
 */
void EDiaVisibleLayer::itemHitAction(INTBIG itemHit)
{
	INTBIG i, val, layerset, listlen;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	WINDOWPART *w;
	CHAR *msg, *entry, lnum[20], *lname;

	if (itemHit == DVSL_DONE)
	{
		hide();
		return;
	}
	if (itemHit == DVSL_LAYERLIST)
	{
		/* toggle layer visibility for this layer */
		i = getCurLine(DVSL_LAYERLIST);
		if (i < 0) return;
		msg = getScrollLine(DVSL_LAYERLIST, i);
		if (*msg == ' ') *msg = '>'; else *msg = ' ';
		setScrollLine(DVSL_LAYERLIST, i, msg);
		changes = TRUE;
		return;
	}
	if (itemHit == DVSL_ALLVISIBLE || itemHit == DVSL_ALLINVISIBLE)
	{
		/* set visibility for all layers */
		listlen = getNumScrollLines(DVSL_LAYERLIST);
		for(i=0; i<listlen; i++)
		{
			msg = getScrollLine(DVSL_LAYERLIST, i);
			if (itemHit == DVSL_ALLVISIBLE) *msg = '>'; else *msg = ' ';
			setScrollLine(DVSL_LAYERLIST, i, msg);
		}
		changes = TRUE;
		return;
	}
	if (itemHit == DVSL_LAYERSET)
	{
		/* construct new layer set */
		loadSet();
		changes = FALSE;
		return;
	}
	if (itemHit == DVSL_APPLY)
	{
		i = us_useroptions;
		if (getControl(DVSL_NODETEXT) != 0) i &= ~HIDETXTNODE; else
			i |= HIDETXTNODE;
		if (getControl(DVSL_ARCTEXT) != 0) i &= ~HIDETXTARC; else
			i |= HIDETXTARC;
		if (getControl(DVSL_PORTTEXT) != 0) i &= ~HIDETXTPORT; else
			i |= HIDETXTPORT;
		if (getControl(DVSL_EXPORTTEXT) != 0) i &= ~HIDETXTEXPORT; else
			i |= HIDETXTEXPORT;
		if (getControl(DVSL_NONLAYTEXT) != 0) i &= ~HIDETXTNONLAY; else
			i |= HIDETXTNONLAY;
		if (getControl(DVSL_INSTTEXT) != 0) i &= ~HIDETXTINSTNAME; else
			i |= HIDETXTINSTNAME;
		if (getControl(DVSL_CELLTEXT) != 0) i &= ~HIDETXTCELL; else
			i |= HIDETXTCELL;
		if (i != us_useroptions)
		{
			startobjectchange((INTBIG)us_tool, VTOOL);
			(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey, i, VINTEGER);
			endobjectchange((INTBIG)us_tool, VTOOL);
		}

		if (changes)
		{
			/* save highlighting */
			us_pushhighlight();

			/* start of change to all display windows */
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if ((w->state&WINDOWTYPE) == DISPWINDOW ||
					(w->state&WINDOWTYPE) == DISP3DWINDOW)
						startobjectchange((INTBIG)w, VWINDOWPART);

			layerset = getPopupEntry(DVSL_LAYERSET);
			switch (layerset)
			{
				case LAYERSELEC:
					/* change the layer visibility */
					for(i=0; i<el_curtech->layercount; i++)
					{
						msg = getScrollLine(DVSL_LAYERLIST, i);
						val = el_curtech->layers[i]->colstyle;
						if ((val&INVISIBLE) == 0 && *msg == ' ')
						{
							(void)setval((INTBIG)el_curtech->layers[i], VGRAPHICS,
								x_("colstyle"), val | INVISIBLE, VSHORT);
						} else if ((val&INVISIBLE) != 0 && *msg != ' ')
						{
							(void)setval((INTBIG)el_curtech->layers[i], VGRAPHICS,
								x_("colstyle"), val & ~INVISIBLE, VSHORT);
						}
					}
					break;
				case LAYERSDXF:
				case LAYERSGDS:
					np = getcurcell();
					if (np == NONODEPROTO) break;
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						if (layerset == LAYERSDXF)
						{
							var = getval((INTBIG)ni, VNODEINST, VSTRING, x_("IO_dxf_layer"));
							if (var == NOVARIABLE) continue;
							lname = (CHAR *)var->addr;
						} else
						{
							var = getval((INTBIG)ni, VNODEINST, VINTEGER, x_("IO_gds_layer"));
							if (var == NOVARIABLE) continue;
							esnprintf(lnum, 20, x_("%ld"), var->addr);
							lname = lnum;
						}
						listlen = getNumScrollLines(DVSL_LAYERLIST);
						for(i=0; i<listlen; i++)
						{
							entry = getScrollLine(DVSL_LAYERLIST, i);
							if (namesame(&entry[2], lname) == 0) break;
						}
						if (*entry != 0)
						{
							startobjectchange((INTBIG)ni, VNODEINST);
							if (entry[0] == '>')
							{
								/* make node visible */
								var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
								if (var != NOVARIABLE && var->addr == 0)
									(void)delvalkey((INTBIG)ni, VNODEINST, art_colorkey);
							} else
							{
								/* make node invisible */
								setvalkey((INTBIG)ni, VNODEINST, art_colorkey, 0, VINTEGER);
							}
							endobjectchange((INTBIG)ni, VNODEINST);
						}
					}
					break;
			}

			/* end of change to all display windows (redraws) */
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if ((w->state&WINDOWTYPE) == DISPWINDOW ||
					(w->state&WINDOWTYPE) == DISP3DWINDOW)
						endobjectchange((INTBIG)w, VWINDOWPART);

			/* restore highlighting */
			us_pophighlight(FALSE);
			us_endchanges(NOWINDOWPART);
			flushscreen();
		}
		changes = FALSE;
		return;
	}
}

/*
 * Helper routine for "us_visiblelayersdlog()" to load the layers into the list.
 */
void EDiaVisibleLayer::loadSet()
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var, *cvar;
	REGISTER INTBIG i, layerset, fun, listlen;
	REGISTER CHAR *lname, *entry;
	CHAR lbuff[100], lnum[20];
	REGISTER void *infstr;

	loadTextDialog(DVSL_LAYERLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0);
	layerset = getPopupEntry(DVSL_LAYERSET);
	switch (layerset)
	{
		case LAYERSELEC:
			for(i=0; i<el_curtech->layercount; i++)
			{
				infstr = initinfstr();
				if ((el_curtech->layers[i]->colstyle&INVISIBLE) != 0) addtoinfstr(infstr, ' '); else
					addtoinfstr(infstr, '>');
				addtoinfstr(infstr, ' ');
				addstringtoinfstr(infstr, layername(el_curtech, i));
				fun = layerfunction(el_curtech, i);
				if ((fun&LFPSEUDO) != 0) addstringtoinfstr(infstr, _(" (for pins)"));
				stuffLine(DVSL_LAYERLIST, returninfstr(infstr));
			}
			break;
		case LAYERSDXF:
		case LAYERSGDS:
			np = getcurcell();
			if (np == NONODEPROTO) break;
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (layerset == LAYERSDXF)
				{
					var = getval((INTBIG)ni, VNODEINST, VSTRING, x_("IO_dxf_layer"));
					if (var == NOVARIABLE) continue;
					lname = (CHAR *)var->addr;
				} else
				{
					var = getval((INTBIG)ni, VNODEINST, VINTEGER, x_("IO_gds_layer"));
					if (var == NOVARIABLE) continue;
					esnprintf(lnum, 20, x_("%ld"), var->addr);
					lname = lnum;
				}
				listlen = getNumScrollLines(DVSL_LAYERLIST);
				for(i=0; i<listlen; i++)
				{
					entry = getScrollLine(DVSL_LAYERLIST, i);
					if (namesame(&entry[2], lname) == 0) break;
				}
				if (i >= listlen)
				{
					cvar = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, art_colorkey);
					if (cvar != NOVARIABLE && cvar->addr == 0)
						esnprintf(lbuff, 100, x_("  %s"), lname); else
							esnprintf(lbuff, 100, x_("> %s"), lname);
					stuffLine(DVSL_LAYERLIST, lbuff);
				}
			}
			break;
	}
	selectLine(DVSL_LAYERLIST, -1);
}

/****************************** WINDOW VIEW DIALOG ******************************/

/* Window Views */
static DIALOGITEM us_windowviewdialogitems[] =
{
 /*  1 */ {0, {256,56,280,166}, BUTTON, N_("Restore View")},
 /*  2 */ {0, {216,8,240,72}, BUTTON, N_("Done")},
 /*  3 */ {0, {32,8,208,234}, SCROLL, x_("")},
 /*  4 */ {0, {8,96,24,229}, EDITTEXT, x_("")},
 /*  5 */ {0, {216,120,240,230}, BUTTON, N_("Save This View")},
 /*  6 */ {0, {8,8,24,90}, MESSAGE, N_("View name:")}
};
static DIALOG us_windowviewdialog = {{50,75,342,318}, N_("Window Views"), 0, 6, us_windowviewdialogitems, 0, 0};

/* special items for the "Window View" dialog: */
#define DWNV_RESTOREVIEW  1		/* restore view (button) */
#define DWNV_VIEWLIST     3		/* view list (scroll) */
#define DWNV_VIEWNAME     4		/* new view name (edit text) */
#define DWNV_SAVEVIEW     5		/* save view (button) */

INTBIG us_windowviewdlog(void)
{
	INTBIG itemHit, i;
	REGISTER VARIABLE *var;
	CHAR *par[3], *pt;
	REGISTER void *dia;

	/* display the window view dialog box */
	dia = DiaInitDialog(&us_windowviewdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DWNV_VIEWLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, 0,
		SCSELMOUSE | SCSELKEY | SCDOUBLEQUIT);
	for(i=0; i<us_tool->numvar; i++)
	{
		var = &us_tool->firstvar[i];
		pt = makename(var->key);
		if (namesamen(pt, x_("USER_windowview_"), 16) == 0) DiaStuffLine(dia, DWNV_VIEWLIST, &pt[16]);
	}
	DiaSelectLine(dia, DWNV_VIEWLIST, -1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == DWNV_RESTOREVIEW)
		{
			/* restore selected view */
			par[0] = x_("name");
			i = DiaGetCurLine(dia, DWNV_VIEWLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DWNV_VIEWLIST, i);
			(void)allocstring(&par[1], pt, el_tempcluster);
			break;
		}

		if (itemHit == DWNV_SAVEVIEW)
		{
			/* save selected view */
			if (!DiaValidEntry(dia, DWNV_VIEWNAME)) continue;
			par[0] = x_("save");
			par[1] = DiaGetText(dia, DWNV_VIEWNAME);
			us_window(2, par);
			DiaLoadTextDialog(dia, DWNV_VIEWLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
			for(i=0; i<us_tool->numvar; i++)
			{
				var = &us_tool->firstvar[i];
				pt = makename(var->key);
				if (namesamen(pt, x_("USER_windowview_"), 16) == 0) DiaStuffLine(dia, DWNV_VIEWLIST, &pt[16]);
			}
			DiaSelectLine(dia, DWNV_VIEWLIST, -1);
			continue;
		}
	}

	DiaDoneDialog(dia);
	if (itemHit == DWNV_RESTOREVIEW)
	{
		us_window(2, par);
		efree(par[1]);
	}
	return(0);
}

/****************************** SUPPORT ******************************/

/*
 * Routine to set the correct line selection in scroll item "scrollitem".  This item
 * has a list of cell names, and the current one should be selected.  If "curinstance"
 * is true, consider the currently selected cell instance (if any).  If "forcecurlib"
 * is true, strip off library names when matching.  If "justcells" is true, only
 * consider cell names, not cell names.
 */
NODEPROTO *us_setscrolltocurrentcell(INTBIG scrollitem, BOOLEAN curinstance, BOOLEAN curcell,
	BOOLEAN forcecurlib, BOOLEAN justcells, void *dia)
{
	Q_UNUSED( forcecurlib );
	REGISTER NODEPROTO *np, *enp, *shownp;
	REGISTER GEOM **list;
	REGISTER CHAR *line, *curnode, *pt, *thisnode;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, listlen;
	REGISTER void *infstr;

	/* find the current node */
	np = getcurcell();
	shownp = NONODEPROTO;

	/* if we can use the current cell and it is valid, take it */
	if (curcell && np != NONODEPROTO) shownp = np;

	/* if there is an explorer window open, use it's selection */
	enp = us_currentexplorernode();
	if (enp != NONODEPROTO) shownp = enp;

	curnode = 0;
	if (shownp != NONODEPROTO)
	{
		if (justcells) curnode = shownp->protoname; else
		{
			curnode = nldescribenodeproto(shownp);
		}
	}

	/* if there is a current cell, look at contents */
	if (np != NONODEPROTO)
	{
		if (curinstance)
		{
			list = us_gethighlighted(WANTNODEINST, 0, 0);
			if (list[0] != NOGEOM)
			{
				curnode = 0;
				for(i=0; list[i] != NOGEOM; i++)
				{
					if (!list[i]->entryisnode)
					{
						curnode = 0;
						break;
					}
					ni = list[i]->entryaddr.ni;
					if (ni->proto->primindex != 0) continue;
					if (justcells) thisnode = ni->proto->protoname; else
						thisnode = nldescribenodeproto(ni->proto);
					if (curnode == 0) curnode = thisnode; else
					{
						if (namesame(curnode, thisnode) != 0)
						{
							curnode = 0;
							break;
						}
					}
				}
			}
		}
		if (namesame(np->protoname, x_("cellstructure")) == 0)
		{
			list = us_gethighlighted(WANTNODEINST, 0, 0);
			if (list[0] != NOGEOM && list[1] == NOGEOM)
			{
				if (list[0]->entryisnode)
				{
					ni = list[0]->entryaddr.ni;
					var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
					if (var != NOVARIABLE)
					{
						curnode = (CHAR *)var->addr;
						if (justcells)
						{
							infstr = initinfstr();
							for(pt = curnode; *pt != 0; pt++)
							{
								if (*pt == ';' || *pt == '{') break;
								addtoinfstr(infstr, *pt);
							}
							curnode = returninfstr(infstr);
						}
					}
				}
			}
		}
	}

	/* now find it in the list */
	if (curnode != 0)
	{
		listlen = DiaGetNumScrollLines(dia, scrollitem);
		for(i=0; i<listlen; i++)
		{
			line = DiaGetScrollLine(dia, scrollitem, i);
			if (estrcmp(line, curnode) == 0)
			{
				DiaSelectLine(dia, scrollitem, i);
				break;
			}
		}
	}
	return(np);
}

/*
 * Routine to get face field of "textdescript" and set popup entry of "popupitem".
 * If "init" is TRUE "popupitem" is initialized by all system fonts.
 */
void us_setpopupface(INTBIG popupitem, INTBIG face, BOOLEAN init, void *dia)
{
	INTBIG i, facecount;
	CHAR *facename, **facelist;

	if (graphicshas(CANCHOOSEFACES))
	{
		if (init)
		{
			facecount = screengetfacelist(&facelist, TRUE);
			DiaSetPopup(dia, popupitem, facecount, facelist);
		}
		facecount = screengetfacelist(&facelist, FALSE);
		if (face >= facecount) face = 0;
		i = 0;
		if (face > 0)
		{
			facename = facelist[face];
			facecount = screengetfacelist(&facelist, TRUE);
			for (i = facecount - 1; i > 0 && namesame(facename, facelist[i]) != 0; i--);
		}
		DiaSetPopupEntry(dia, popupitem, i);
	}
}

/*
 * Routine to set face field of "textdescript" to face selected by "popupitem".
 */
INTBIG us_getpopupface(INTBIG popupitem, void *dia)
{
	INTBIG value, facecount;
	CHAR **facelist;

	value = 0;
	if (graphicshas(CANCHOOSEFACES))
	{
		value = DiaGetPopupEntry(dia, popupitem);
		if (value > 0)
		{
			facecount = screengetfacelist(&facelist, TRUE);
			value = screenfindface(facelist[value]);
			if (value < 0)
			{
				ttyputerr(x_("Too many used fonts, using default"));
				value = 0;
				DiaSetPopupEntry(dia, popupitem, 0);
			}
		}
	}
	return(value);
}
