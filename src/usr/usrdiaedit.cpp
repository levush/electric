/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrdiaedit.cpp
 * User interface tool: dialog editor
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2001 Static Free Software.
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
 * support@staticfreesoft.com
 */

#include "global.h"
#include "egraphics.h"
#include "edialogs.h"
#include "usr.h"
#include "usrtrack.h"
#ifdef USEQT
#  include <qstrlist.h>
#endif

#define TYPEUNKNOWN      0
#define TYPEBUTTON       1
#define TYPEDEFBUTTON    2
#define TYPECHECK        3
#define TYPEAUTOCHECK    4
#define TYPESCROLL       5
#define TYPESCROLLMULTI  6
#define TYPEMESSAGE      7
#define TYPEEDITTEXT     8
#define TYPEOPAQUEEDIT   9
#define TYPEICON        10
#define TYPEUSER        11
#define TYPEPOPUP       12
#define TYPEPROGRESS    13
#define TYPEDIVIDER     14
#define TYPERADIO       15
#define TYPERADIOA      16
#define TYPERADIOB      17
#define TYPERADIOC      18
#define TYPERADIOD      19
#define TYPERADIOE      20
#define TYPERADIOF      21
#define TYPERADIOG      22

#define DIALOGFILE     x_("AllDialogs.c")
#define BORDER           2
#define MAXLINE        300
#define SINGLETOP       25
#define SINGLELEFT      61
#define GROWTHSLOP     100
#define SEPARATIONX     10
#define SEPARATIONY      9
#define NUMTYPES        23
#define GROWBOXSIZE      6

/* scale factors on qt */
#define QTNUM            16
#define QTDEN            12
#define QTSCALE(x)       ((int(x) * QTNUM + QTDEN/2) / QTDEN)

static CHAR *us_diaedittypes[] =
{
	x_("Unknown"),
	x_("Button"),
	x_("Default Button"),
	x_("Check Box"),
    x_("Check Box (automatic)"),
	x_("Scroll Area"),
	x_("Scroll Area (multi-select)"),
	x_("Message"),
	x_("Edit Text"),
	x_("Edit Text (opaque)"),
	x_("Icon"),
	x_("User Drawn"),
	x_("Popup"),
	x_("Progress"),
	x_("Divider"),
	x_("Radio Box"),
	x_("Radio Box (group A)"),
	x_("Radio Box (group B)"),
	x_("Radio Box (group C)"),
	x_("Radio Box (group D)"),
	x_("Radio Box (group E)"),
	x_("Radio Box (group F)"),
	x_("Radio Box (group G)")
};

static CHAR *us_diaeditshorttypes[] =
{
	x_("unknown"),
	x_("button"),
	x_("defbutton"),
	x_("check"),
    x_("autocheck"),
	x_("scroll"),
	x_("multiscroll"),
	x_("message"),
	x_("edittext"),
	x_("opaque"),
	x_("icon"),
	x_("useritem"),
	x_("popup"),
	x_("progress"),
	x_("divider"),
	x_("radio"),
	x_("radioa"),
	x_("radiob"),
	x_("radioc"),
	x_("radiod"),
	x_("radioe"),
	x_("radiof"),
	x_("radiog")
};

static CHAR *us_diaeditqttypes[] =
{
	x_("QWidget"),
	x_("QPushButton"),
	x_("QPushButton"),
	x_("ECheckField"),
    x_("QCheckBox"),
	x_("EScrollField"),
	x_("EScrollField"),
	x_("QLabel"),
	x_("QLineEdit"),
	x_("QLineEdit"),
	x_("EIconField"),
	x_("EUserDrawnField"),
	x_("QComboBox"),
	x_("QProgressBar"),
	x_("QWidget"),
	x_("QRadioButton"),
	x_("QRadioButton"),
	x_("QRadioButton"),
	x_("QRadioButton"),
	x_("QRadioButton"),
	x_("QRadioButton"),
	x_("QRadioButton"),
	x_("QRadioButton")
};

typedef struct Idiaitem
{
	INTBIG           top, left, bottom, right;
	INTBIG           inactive;
	INTBIG           selected;
	INTBIG           type;
	CHAR            *msg;
    CHAR            *name;
    CHAR            *comment;
	struct Idiaitem *next;
} DIAITEM;

typedef struct Idia
{
	CHAR        *comment;
    CHAR        *prefix;
    CHAR        *abbrev;
	CHAR        *title;
    CHAR        *name;
	INTBIG       top, left, bottom, right;
    INTBIG       briefHeight;
    BOOLEAN      hasUi;
	INTBIG       numitems;
	DIAITEM     *firstitem;
	struct Idia *next;
} DIA;

static void    us_diaeditaligntogrid(void *dia);
static int     us_diaeditcommentascending(const void *e1, const void *e2);
static void    us_diaeditdeletedialog(void *dia);
static CHAR   *us_diaedittitlenextinlist(void);
static BOOLEAN us_diaedittitletoplist(CHAR **a);
static void    us_diaeditduplicateitem(void *dia);
static void    us_diaedititemhit(void *dia);
static void    us_diaeditinvertitem(DIAITEM *item, void *dia);
static void    us_diaeditmakenewdialog(void *dia);
static void    us_diaeditnewitem(void *dia);
static void    us_diaeditreaddialogs(void);
static void    us_diaeditredrawsingledia(void *dia);
static void    us_diaeditsavedialogs(void);
static void    us_diaeditsaveui(CHAR *dianame);
static void    us_diaeditloadui(CHAR *dianame);
static void    us_diaeditloaduiall();
static void    us_diaeditsetgridamount(void);
static void    us_diaeditsettitle(void *dia);
static void    us_diaeditshowdetails(DIAITEM *item, INTBIG whichitem);
static void    us_diaeditwritequotedstring(CHAR *string, FILE *out);
static void    us_diadrawframe(INTBIG left, INTBIG top, INTBIG right, INTBIG bottom, void *dia);
static void    us_diaeditdragitem(INTBIG x, INTBIG y);
static void    us_diaeditgrowitem(INTBIG x, INTBIG y);
static void    us_diaeditareaselect(INTBIG x, INTBIG y);
static void    us_diaeditinvertoutline(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, void *dia);
static void    us_diaeditgrowdialog(INTBIG x, INTBIG y);
static void    us_diaeditgrowbrief(INTBIG x, INTBIG y);
static void    us_diaeditredisproutine(RECTAREA *r, void *dia);
static INTBIG  us_diaeditensuressaved(void);
static void    us_diaeditdosingle(CHAR *dianame);

static DIA      *us_diaeditfirstdia;				/* head of list of all dialogs being edited */
static DIA      *us_diaeditcurdia;					/* current dialog being edited */
static DIA      *us_diaeditloopdia;					/* for looping through dialog list */
static INTBIG    us_diaeditshownumbers;				/* nonzero to show item numbers */
static INTBIG    us_diaeditshowoutline;				/* nonzero to show item outlines */
static INTBIG    us_diaeditshowwidhei;				/* nonzero to use width/height instead of right/bottom */
static INTBIG    us_diaeditdirty;					/* nonzero if dialogs have changed */
static INTBIG    us_diaeditgridamt;					/* grid amount for aligning items */
static INTBIG    us_diaeditlastx, us_diaeditlasty;	/* last coordinate during tracking */
static INTBIG    us_diaeditinix, us_diaeditiniy;	/* initial coordinate during tracking */
static DIAITEM  *us_diaeditclickeditem;				/* current item during tracking */
static void     *us_diaeditcurdialog;				/* dialog being tracked */

/********************************** CONTROL **********************************/

/* Dialog Edit: All dialogs */
static DIALOGITEM us_diaeditalldialogitems[] =
{
 /*  1 */ {0, {116,228,140,284}, DEFBUTTON, N_("Edit")},
 /*  2 */ {0, {8,16,336,228}, SCROLL, x_("")},
 /*  3 */ {0, {52,228,76,284}, BUTTON, N_("Save")},
 /*  4 */ {0, {16,228,40,284}, BUTTON, N_("Done")},
 /*  5 */ {0, {148,228,172,284}, BUTTON, N_("New")},
 /*  6 */ {0, {180,228,204,284}, BUTTON, N_("Delete")},
 /*  7 */ {0, {244,228,268,284}, BUTTON, N_("Title")},
 /*  8 */ {0, {276,228,300,284}, BUTTON, N_("Grid")},
 /*  9 */ {0, {84,228,108,284}, BUTTON, N_("Save .ui")},
 /* 10 */ {0, {212,228,236,284}, BUTTON, N_("Load .ui")}
};
static DIALOG us_diaeditalldialog = {{49,56,394,350}, N_("Dialog Editor"), 0, 10, us_diaeditalldialogitems, 0, 0};

/* special items for the "diaeditall" dialog: */
#define DDIE_EDITDIA   1		/* edit dialog (defbutton) */
#define DDIE_DIALIST   2		/* dialog list (scroll) */
#define DDIE_SAVE      3		/* save dialogs (button) */
#define DDIE_DONE      4		/* exit editor (button) */
#define DDIE_NEWDIA    5		/* new dialog (button) */
#define DDIE_DELDIA    6		/* delete dialog (button) */
#define DDIE_DIATITLE  7		/* set dialog title (button) */
#define DDIE_SETGRID   8		/* set item grid (button) */
#define DDIE_SAVEUI    9		/* save dialog.ui (button) */
#define DDIE_LOADUI   10		/* load dialog.ui (button) */

/*
 * Routine called to edit all dialogs.
 */
void us_dialogeditor(void)
{
	INTBIG itemno, which;
	CHAR *pt;
	REGISTER void *dia;

	us_diaeditreaddialogs();	
	
	dia = DiaInitDialog(&us_diaeditalldialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DDIE_DIALIST, us_diaedittitletoplist, us_diaedittitlenextinlist,
		DiaNullDlogDone, 0, SCSELMOUSE|SCDOUBLEQUIT);
	DiaSelectLine(dia, DDIE_DIALIST, -1);

	us_diaeditshownumbers = 0;
	us_diaeditshowoutline = 1;
	us_diaeditshowwidhei = 0;
	us_diaeditgridamt = 4;
	us_diaeditdirty = 0;
	for(;;)
	{
		itemno = DiaNextHit(dia);
		if (itemno == DDIE_DONE)
		{
			if (us_diaeditensuressaved() == 0) continue;
			break;
		}
		if (itemno == DDIE_DELDIA)
		{
			us_diaeditdeletedialog(dia);
			continue;
		}
		if (itemno == DDIE_SAVE)
		{
			us_diaeditsavedialogs();
			continue;
		}
		if (itemno == DDIE_SAVEUI)
		{
			which = DiaGetCurLine(dia, DDIE_DIALIST);
			pt = DiaGetScrollLine(dia, DDIE_DIALIST, which);
			us_diaeditsaveui(pt);
			continue;
		}
		if (itemno == DDIE_LOADUI)
		{
			us_diaeditloaduiall();
			continue;
		}
		if (itemno == DDIE_NEWDIA)
		{
			us_diaeditmakenewdialog(dia);
			continue;
		}
		if (itemno == DDIE_DIATITLE)
		{
			us_diaeditsettitle(dia);
			continue;
		}
		if (itemno == DDIE_SETGRID)
		{
			us_diaeditsetgridamount();
			continue;
		}

		if (itemno == DDIE_EDITDIA)
		{
			which = DiaGetCurLine(dia, DDIE_DIALIST);
			pt = DiaGetScrollLine(dia, DDIE_DIALIST, which);
			us_diaeditdosingle(pt);
			continue;
		}
	}
	DiaDoneDialog(dia);
}

/********************************** CONTROL OF DIALOGS **********************************/

/* Dialog Edit: Single dialog */
static DIALOGITEM us_diaeditsingledialogitems[] =
{
 /*  1 */ {0, {32,4,48,51}, BUTTON, N_("Edit")},
 /*  2 */ {0, {4,4,20,52}, BUTTON, N_("Done")},
 /*  3 */ {0, {60,4,76,52}, BUTTON, N_("New")},
 /*  4 */ {0, {80,4,96,52}, BUTTON, N_("Dup")},
 /*  5 */ {0, {128,4,144,52}, BUTTON, N_("Align")},
 /*  6 */ {0, {4,60,20,156}, CHECK, N_("Numbers")},
 /*  7 */ {0, {4,160,20,256}, CHECK, N_("Outlines")},
 /*  8 */ {0, {25,61,200,300}, USERDRAWN, x_("")},
 /*  9 */ {0, {100,4,116,52}, BUTTON, N_("Del")}
};
static DIALOG us_diaeditsingledialog = {{100,360,309,669}, N_("Dialog"), 0, 9, us_diaeditsingledialogitems, 0, 0};

#define DONE_EDITITEM     1		/* edit item (button) */
#define DONE_DONEEDIT     2		/* done editing dialog (button) */
#define DONE_NEWITEM      3		/* new item (button) */
#define DONE_DUPITEM      4		/* duplicate item (button) */
#define DONE_ALIGNITEMS   5		/* align all items (button) */
#define DONE_SHOWNUMBERS  6		/* show item numbers (check) */
#define DONE_SHOWOUTLINE  7		/* show item outlines (check) */
#define DONE_ITEMAREA     8		/* item area (user) */
#define DONE_DELITEM      9		/* delete item (button) */

/*
 * Routine to edit a single dialog ("dianame") from the list.
 */
void us_diaeditdosingle(CHAR *dianame)
{
	INTBIG itemno, origright, origbottom, numselected, i, whichitem;
	DIAITEM *item, *clickeditem, *lastitem, *nextitem;
	REGISTER void *dia;

	/* find the dialog */
	for(us_diaeditcurdia = us_diaeditfirstdia; us_diaeditcurdia != 0; us_diaeditcurdia = us_diaeditcurdia->next)
		if (estrcmp(us_diaeditcurdia->comment, dianame) == 0) break;
	if (us_diaeditcurdia == 0) return;

	origright = us_diaeditcurdia->right;
	origbottom = us_diaeditcurdia->bottom;
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		item->selected = 0;
		if (us_diaeditcurdia->right < item->right + us_diaeditcurdia->left + SEPARATIONX)
			us_diaeditcurdia->right = item->right + us_diaeditcurdia->left + SEPARATIONX;
		if (us_diaeditcurdia->bottom < item->bottom + us_diaeditcurdia->top + SEPARATIONY)
			us_diaeditcurdia->bottom = item->bottom + us_diaeditcurdia->top + SEPARATIONY;
	}
	if (origright != us_diaeditcurdia->right || origbottom != us_diaeditcurdia->bottom)
	{
		us_diaeditdirty = 1;
		DiaMessageInDialog(x_("Warning: dialog right adjusted by %ld, bottom by %ld"),
			us_diaeditcurdia->right - origright, us_diaeditcurdia->bottom - origbottom);
	}
	us_diaeditsingledialog.movable = us_diaeditcurdia->title;
	if (us_diaeditsingledialog.movable == 0)
		us_diaeditsingledialog.movable = x_("*** DIALOG HAS NO TITLE BAR ***");

	/* set the dialog to the right size */
	us_diaeditsingledialog.windowRect.right = (INTSML)(us_diaeditsingledialog.windowRect.left +
		us_diaeditcurdia->right - us_diaeditcurdia->left + SINGLELEFT + GROWTHSLOP);
	us_diaeditsingledialog.windowRect.bottom = (INTSML)(us_diaeditsingledialog.windowRect.top +
		us_diaeditcurdia->bottom - us_diaeditcurdia->top + SINGLETOP + GROWTHSLOP);

	/* set the userdraw area to the right size */
	us_diaeditsingledialogitems[DONE_ITEMAREA-1].r.right = (INTSML)(us_diaeditsingledialogitems[DONE_ITEMAREA-1].r.left +
		us_diaeditcurdia->right - us_diaeditcurdia->left + GROWTHSLOP);
	us_diaeditsingledialogitems[DONE_ITEMAREA-1].r.bottom = (INTSML)(us_diaeditsingledialogitems[DONE_ITEMAREA-1].r.top +
		us_diaeditcurdia->bottom - us_diaeditcurdia->top + GROWTHSLOP);

	dia = DiaInitDialog(&us_diaeditsingledialog);
	if (dia == 0) return;
	if (us_diaeditshownumbers != 0) DiaSetControl(dia, DONE_SHOWNUMBERS, 1);
	if (us_diaeditshowoutline != 0) DiaSetControl(dia, DONE_SHOWOUTLINE, 1);
	us_diaeditredrawsingledia(dia);
	DiaRedispRoutine(dia, DONE_ITEMAREA, us_diaeditredisproutine);
	DiaAllowUserDoubleClick(dia);
	for(;;)
	{
		itemno = DiaNextHit(dia);
		if (itemno == DONE_ITEMAREA)
		{
			us_diaedititemhit(dia);
			continue;
		}
		if (itemno == DONE_EDITITEM)
		{
			numselected = 0;
			for(item = us_diaeditcurdia->firstitem, i = 1; item != 0; item = item->next, i++)
			{
				if (item->selected == 0) continue;
				numselected++;
				clickeditem = item;
				whichitem = i;
			}
			if (numselected != 1) continue;
			us_diaeditshowdetails(clickeditem, whichitem);
			us_diaeditredrawsingledia(dia);
			continue;
		}
		if (itemno == DONE_DONEEDIT) break;
		if (itemno == DONE_NEWITEM)
		{
			us_diaeditnewitem(dia);
			continue;
		}
		if (itemno == DONE_DUPITEM)
		{
			us_diaeditduplicateitem(dia);
			continue;
		}
		if (itemno == DONE_ALIGNITEMS)
		{
			us_diaeditaligntogrid(dia);
			continue;
		}
		if (itemno == DONE_SHOWNUMBERS)
		{
			us_diaeditshownumbers = 1 - us_diaeditshownumbers;
			DiaSetControl(dia, itemno, us_diaeditshownumbers);
			us_diaeditredrawsingledia(dia);
			continue;
		}
		if (itemno == DONE_SHOWOUTLINE)
		{
			us_diaeditshowoutline = 1 - us_diaeditshowoutline;
			DiaSetControl(dia, itemno, us_diaeditshowoutline);
			us_diaeditredrawsingledia(dia);
			continue;
		}
		if (itemno == DONE_DELITEM)
		{
			lastitem = 0;
			for(item = us_diaeditcurdia->firstitem; item != 0; item = nextitem)
			{
				nextitem = item->next;
				if (item->selected != 0)
				{
					if (lastitem == 0) us_diaeditcurdia->firstitem = nextitem; else
						lastitem->next = nextitem;
					efree((CHAR *)item->msg);
					efree((CHAR *)item);
					us_diaeditcurdia->numitems--;
					us_diaeditdirty++;
					continue;
				}
				lastitem = item;
			}
			us_diaeditredrawsingledia(dia);
		}
	}
	DiaDoneDialog(dia);
}

/* Dialog Edit: Dialog name */
static DIALOGITEM us_diaeditnewdialogitems[] =
{
 /*  1 */ {0, {56,8,76,68}, BUTTON, N_("OK")},
 /*  2 */ {0, {56,168,76,228}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,78}, MESSAGE, N_("Name:")},
 /*  4 */ {0, {8,80,24,228}, EDITTEXT, x_("")},
 /*  5 */ {0, {32,8,48,98}, MESSAGE, N_("Short name:")},
 /*  6 */ {0, {32,100,48,228}, EDITTEXT, x_("")}
};
static DIALOG us_diaeditnewdialog = {{450,56,535,293}, N_("New Dialog"), 0, 6, us_diaeditnewdialogitems, 0, 0};

#define DNEW_NAME   4		/* dialog name (edit text) */
#define DNEW_SNAME  6		/* short dialog name (edit text) */

/*
 * Routine to create a new dialog.
 */
void us_diaeditmakenewdialog(void *mainDia)
{
	DIA *newDia, *lastdia;
	INTBIG itemno, i, listlen;
	CHAR name[200], abbrev[200], *pt;
	REGISTER void *dia;

	/* prompt for dialog name and abbreviation */
	dia = DiaInitDialog(&us_diaeditnewdialog);
	if (dia == 0) return;
	for(;;)
	{
		itemno = DiaNextHit(dia);
		if (itemno == OK || itemno == CANCEL) break;
	}
	if (itemno == OK)
	{
		estrcpy(name, DiaGetText(dia, DNEW_NAME));
		estrcpy(abbrev, DiaGetText(dia, DNEW_SNAME));
	}
	DiaDoneDialog(dia);
	if (itemno == CANCEL) return;

	/* create the dialog */
	for(newDia = us_diaeditfirstdia; newDia != 0; newDia = newDia->next)
		lastdia = newDia;
	newDia = (DIA *)emalloc(sizeof (DIA), us_tool->cluster);
	if (us_diaeditfirstdia == 0)
	{
		newDia->next = us_diaeditfirstdia;
		us_diaeditfirstdia = newDia;
	} else
	{
		newDia->next = lastdia->next;
		lastdia->next = newDia;
	}

	allocstring(&newDia->comment, name, us_tool->cluster);
	allocstring(&newDia->prefix, x_("us"), us_tool->cluster);
	allocstring(&newDia->abbrev, abbrev, us_tool->cluster);

	newDia->title = 0;
    newDia->name = 0;
	newDia->top = 75;     newDia->left = 75;
	newDia->bottom = 300; newDia->right = 300;
    newDia->briefHeight = 0;
    newDia->hasUi = FALSE;
	newDia->numitems = 0;
	newDia->firstitem = 0;

	/* redisplay the list dialog with this one included */
	DiaLoadTextDialog(mainDia, DDIE_DIALIST, us_diaedittitletoplist, us_diaedittitlenextinlist, DiaNullDlogDone, 0);
	listlen = DiaGetNumScrollLines(mainDia, DDIE_DIALIST);
	for(i=0; i<listlen; i++)
	{
		pt = DiaGetScrollLine(mainDia, DDIE_DIALIST, i);
		if (estrcmp(pt, newDia->comment) == 0) break;
	}
	if (*pt != 0) DiaSelectLine(mainDia, DDIE_DIALIST, i);
	us_diaeditdirty = 1;
}

/*
 * Routine to delete the selected dialog.
 */
void us_diaeditdeletedialog(void *dia)
{
	INTBIG which;
	DIA *lastdia;
	DIAITEM *item, *nextitem;
	CHAR *pt;

	which = DiaGetCurLine(dia, DDIE_DIALIST);
	pt = DiaGetScrollLine(dia, DDIE_DIALIST, which);
	lastdia = 0;
	for(us_diaeditcurdia = us_diaeditfirstdia; us_diaeditcurdia != 0; us_diaeditcurdia = us_diaeditcurdia->next)
	{
		if (estrcmp(us_diaeditcurdia->comment, pt) == 0) break;
		lastdia = us_diaeditcurdia;
	}
	if (us_diaeditcurdia == 0) return;
	if (lastdia == 0) us_diaeditfirstdia = us_diaeditcurdia->next; else
		lastdia->next = us_diaeditcurdia->next;

	/* delete the dialog data */
	for(item = us_diaeditcurdia->firstitem; item != 0; item = nextitem)
	{
		nextitem = item->next;
		efree(item->msg);
        if (item->name != 0) efree(item->name);
        if (item->comment != 0) efree(item->comment);
		efree((CHAR *)item);
	}
	efree(us_diaeditcurdia->comment);
	efree(us_diaeditcurdia->prefix);
	efree(us_diaeditcurdia->abbrev);
	if (us_diaeditcurdia->title != 0) efree(us_diaeditcurdia->title);
	if (us_diaeditcurdia->name != 0) efree(us_diaeditcurdia->name);
	efree((CHAR *)us_diaeditcurdia);

	/* redisplay list of dialogs */
	DiaLoadTextDialog(dia, DDIE_DIALIST, us_diaedittitletoplist, us_diaedittitlenextinlist, DiaNullDlogDone, 0);
	us_diaeditdirty = 1;
}

/* Dialog Edit: Set title */
static DIALOGITEM us_diaedittitledialogitems[] =
{
 /*  1 */ {0, {32,8,52,68}, BUTTON, N_("OK")},
 /*  2 */ {0, {32,168,52,228}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,228}, EDITTEXT, x_("")}
};
static DIALOG us_diaedittitledialog = {{450,56,511,293}, N_("Dialog Title"), 0, 3, us_diaedittitledialogitems, 0, 0};

#define DTIT_DIANAME   3		/* dialog name (edit text) */

/*
 * Routine to change the title of the selected dialog.
 */
void us_diaeditsettitle(void *dia)
{
	CHAR *pt;
	INTBIG itemno, which;
	DIA *thedia;
	REGISTER void *subdia;

	/* get the current dialog */
	which = DiaGetCurLine(dia, DDIE_DIALIST);
	pt = DiaGetScrollLine(dia, DDIE_DIALIST, which);
	for(thedia = us_diaeditfirstdia; thedia != 0; thedia = thedia->next)
		if (estrcmp(thedia->comment, pt) == 0) break;
	if (thedia == 0) return;

	/* prompt for the title */
	subdia = DiaInitDialog(&us_diaedittitledialog);
	if (subdia == 0) return;
	if (thedia->title != 0)
		DiaSetText(subdia, DTIT_DIANAME, thedia->title);
	for(;;)
	{
		itemno = DiaNextHit(subdia);
		if (itemno == OK || itemno == CANCEL) break;
	}

	/* change the title if requested */
	if (itemno == OK)
	{
		if (thedia->title != 0) efree((CHAR *)thedia->title);
		thedia->title = 0;
		pt = DiaGetText(subdia, DTIT_DIANAME);
		if (*pt != 0)
			allocstring(&thedia->title, pt, us_tool->cluster);
		us_diaeditdirty = 1;
	}
	DiaDoneDialog(subdia);
}

/*
 * Routine initialize a list of dialogs.
 */
BOOLEAN us_diaedittitletoplist(CHAR **a)
{
	Q_UNUSED( a );
	us_diaeditloopdia = us_diaeditfirstdia;
	return(TRUE);
}

/*
 * Routine to return the next in the list of dialogs.
 */
CHAR *us_diaedittitlenextinlist(void)
{
	CHAR *retval;

	if (us_diaeditloopdia == 0) retval = 0; else
	{
		retval = us_diaeditloopdia->comment;
		us_diaeditloopdia = us_diaeditloopdia->next;
	}
	return(retval);
}

/********************************** CONTROL OF ITEMS **********************************/

/*
 * Routine to handle the "single" dialog (the display of a dialog and its items).
 */
void us_diaedititemhit(void *dia)
{
	DIAITEM *clickeditem, *item;
	INTBIG top, left, bottom, right, numselected, i, shiftclick;
	INTBIG x, y;

	shiftclick = 0;
	DiaGetMouse(dia, &x, &y);

	/* see if entire dialog was grown */
	if (x > SINGLELEFT + us_diaeditcurdia->right - us_diaeditcurdia->left &&
		y > SINGLETOP + us_diaeditcurdia->bottom - us_diaeditcurdia->top)
	{
		us_diaeditlastx = x;   us_diaeditlasty = y;
		us_diaeditcurdialog = dia;
		DiaTrackCursor(dia, us_diaeditgrowdialog);
		return;
	}

	/* see if brief part of extensible dialog was grown */
	if (us_diaeditcurdia->briefHeight != 0 &&
		x > SINGLELEFT + us_diaeditcurdia->right - us_diaeditcurdia->left &&
        y >= SINGLETOP + us_diaeditcurdia->briefHeight &&
		y <= SINGLETOP + us_diaeditcurdia->briefHeight + GROWBOXSIZE)
	{
		us_diaeditlastx = x;   us_diaeditlasty = y;
		us_diaeditcurdialog = dia;
		DiaTrackCursor(dia, us_diaeditgrowbrief);
		return;
	}

	/* see what was clicked on */
	clickeditem = 0;
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		top = item->top+SINGLETOP;   bottom = item->bottom+SINGLETOP;
		left = item->left+SINGLELEFT;   right = item->right+SINGLELEFT;
		if (x >= left && x <= right && y >= top && y <= bottom)
		{
			if (clickeditem == 0 || item->selected != 0)
				clickeditem = item;
		}
	}

	/* if shift key not held down, deselect everything */
	if (shiftclick == 0 && (clickeditem == 0 || clickeditem->selected == 0))
	{
		for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
		{
			if (item->selected != 0) us_diaeditinvertitem(item, dia);
			item->selected = 0;
		}
	}

	if (clickeditem == 0)
	{
		/* area select */
		us_diaeditlastx = us_diaeditinix = x;   us_diaeditlasty = us_diaeditiniy = y;
		us_diaeditinvertoutline(x, y, x, y, dia);
		us_diaeditcurdialog = dia;
		DiaTrackCursor(dia, us_diaeditareaselect);

		us_diaeditinvertoutline(us_diaeditinix, us_diaeditiniy, us_diaeditlastx, us_diaeditlasty, dia);
		if (us_diaeditinix > us_diaeditlastx)
		{
			i = us_diaeditinix;   us_diaeditinix = us_diaeditlastx;   us_diaeditlastx = i;
		}
		if (us_diaeditiniy > us_diaeditlasty)
		{
			i = us_diaeditiniy;   us_diaeditiniy = us_diaeditlasty;   us_diaeditlasty = i;
		}
		for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
		{
			top = item->top+SINGLETOP;   bottom = item->bottom+SINGLETOP;
			left = item->left+SINGLELEFT;   right = item->right+SINGLELEFT;
			if (left < us_diaeditlastx && right > us_diaeditinix &&
				top < us_diaeditlasty && bottom > us_diaeditiniy)
			{
				if (shiftclick == 0)
				{
					item->selected = 1;
				} else
				{
					item->selected = 1 - item->selected;
				}
			}
		}
		us_diaeditredrawsingledia(dia);
		return;
	}

	/* something selected: either add or subtract selection */
	if (shiftclick == 0)
	{
		clickeditem->selected = 1;
	} else
	{
		clickeditem->selected = 1 - clickeditem->selected;
	}
	us_diaeditinvertitem(clickeditem, dia);
	if (clickeditem->selected == 0) return;

	/* if clicked on one item in corner, stretch it */
	numselected = 0;
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		if (item->selected == 0) continue;
		numselected++;
		clickeditem = item;
	}
	if (numselected == 1 && x >= clickeditem->right+SINGLELEFT-GROWBOXSIZE &&
		y >= clickeditem->bottom+SINGLETOP-GROWBOXSIZE)
	{
		/* stretch the item */
		us_diaeditlastx = us_diaeditinix = x;   us_diaeditlasty = us_diaeditiniy = y;
		us_diaeditclickeditem = clickeditem;
		us_diaeditcurdialog = dia;
		DiaTrackCursor(dia, us_diaeditgrowitem);

		if (clickeditem->right < clickeditem->left)
		{
			i = clickeditem->right;
			clickeditem->right = clickeditem->left;
			clickeditem->left = i;
			us_diaeditredrawsingledia(dia);
		}
		if (clickeditem->bottom < clickeditem->top)
		{
			i = clickeditem->bottom;
			clickeditem->bottom = clickeditem->top;
			clickeditem->top = i;
			us_diaeditredrawsingledia(dia);
		}
		return;
	}

	/* allow dragging of selected items */
	us_diaeditlastx = us_diaeditinix = x;   us_diaeditlasty = us_diaeditiniy = y;
	us_diaeditcurdialog = dia;
	DiaTrackCursor(dia, us_diaeditdragitem);
}

/*
 * Tracking routine when selecting an area of items.
 */
void us_diaeditareaselect(INTBIG x, INTBIG y)
{
	RECTAREA r;

	DiaItemRect(us_diaeditcurdialog, DONE_ITEMAREA, &r);
	if (x < r.left || x > r.right || y < r.top || y > r.bottom) return;
	us_diaeditinvertoutline(us_diaeditinix, us_diaeditiniy, us_diaeditlastx, us_diaeditlasty, us_diaeditcurdialog);
	us_diaeditlastx = x;   us_diaeditlasty = y;
	us_diaeditinvertoutline(us_diaeditinix, us_diaeditiniy, us_diaeditlastx, us_diaeditlasty, us_diaeditcurdialog);
}

/*
 * Tracking routine when changing the size of an item.
 */
void us_diaeditgrowitem(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx, dy, right, bottom;

	x = us_diaeditinix + (x - us_diaeditinix) / us_diaeditgridamt * us_diaeditgridamt;
	y = us_diaeditiniy + (y - us_diaeditiniy) / us_diaeditgridamt * us_diaeditgridamt;
	dx = x - us_diaeditlastx;   dy = y - us_diaeditlasty;
	right = us_diaeditclickeditem->right + dx;
	bottom = us_diaeditclickeditem->bottom + dy;
	if (right > us_diaeditcurdia->right - us_diaeditcurdia->left)
		right = us_diaeditcurdia->right - us_diaeditcurdia->left;
	if (bottom > us_diaeditcurdia->bottom - us_diaeditcurdia->top)
		bottom = us_diaeditcurdia->bottom - us_diaeditcurdia->top;
	if (right == us_diaeditclickeditem->right && bottom == us_diaeditclickeditem->bottom)
		return;

	us_diaeditclickeditem->right = right;
	us_diaeditclickeditem->bottom = bottom;
	us_diaeditlastx = x;   us_diaeditlasty = y;
	us_diaeditredrawsingledia(us_diaeditcurdialog);
	us_diaeditdirty = 1;
}

/*
 * Tracking routine when moving an item.
 */
void us_diaeditdragitem(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx, dy, top, left, bottom, right;
	DIAITEM *item;

	x = us_diaeditinix + (x - us_diaeditinix) / us_diaeditgridamt * us_diaeditgridamt;
	y = us_diaeditiniy + (y - us_diaeditiniy) / us_diaeditgridamt * us_diaeditgridamt;
	dx = x - us_diaeditlastx;   dy = y - us_diaeditlasty;
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		if (item->selected == 0) continue;
		left = item->left + dx;
		right = item->right + dx;
		top = item->top + dy;
		bottom = item->bottom + dy;

		if (left < 0) dx -= left;
		if (right > us_diaeditcurdia->right - us_diaeditcurdia->left)
			dx -= right - (us_diaeditcurdia->right - us_diaeditcurdia->left);
		if (top < 0) dy -= top;
		if (bottom > us_diaeditcurdia->bottom - us_diaeditcurdia->top)
			dy -= bottom - (us_diaeditcurdia->bottom - us_diaeditcurdia->top);

		item->left += dx;   item->right += dx;
		item->top += dy;   item->bottom += dy;
	}
	us_diaeditlastx = x;   us_diaeditlasty = y;
	us_diaeditredrawsingledia(us_diaeditcurdialog);
	us_diaeditdirty = 1;
}

/*
 * Tracking routine when changing the size of the dialog.
 */
void us_diaeditgrowdialog(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx, dy;
	RECTAREA r;

	DiaItemRect(us_diaeditcurdialog, DONE_ITEMAREA, &r);
	if (x < r.left || x > r.right || y < r.top || y > r.bottom) return;
	dx = x - us_diaeditlastx;   dy = y - us_diaeditlasty;
	us_diaeditcurdia->right += dx;
	us_diaeditcurdia->bottom += dy;
    if (us_diaeditcurdia->briefHeight != 0 && us_diaeditcurdia->top + us_diaeditcurdia->briefHeight >= us_diaeditcurdia->bottom - GROWBOXSIZE)
    {
        us_diaeditcurdia->briefHeight = us_diaeditcurdia->bottom - us_diaeditcurdia->top - GROWBOXSIZE;
        if (us_diaeditcurdia->briefHeight <= 0) us_diaeditcurdia->briefHeight = 1;
    }
	us_diaeditlastx = x;   us_diaeditlasty = y;
	us_diaeditredrawsingledia(us_diaeditcurdialog);
	us_diaeditdirty = 1;
}

/*
 * Tracking routine when changing the brief part of extensible dialog.
 */
void us_diaeditgrowbrief(INTBIG x, INTBIG y)
{
	REGISTER INTBIG dx, dy;
	RECTAREA r;

	DiaItemRect(us_diaeditcurdialog, DONE_ITEMAREA, &r);
	if (x < r.left || x > r.right || y < r.top || y > r.bottom) return;
	dx = x - us_diaeditlastx;   dy = y - us_diaeditlasty;
	us_diaeditcurdia->briefHeight += dy;
    if (us_diaeditcurdia->top + us_diaeditcurdia->briefHeight >= us_diaeditcurdia->bottom - GROWBOXSIZE)
        us_diaeditcurdia->briefHeight = us_diaeditcurdia->bottom - us_diaeditcurdia->top - GROWBOXSIZE;
    if (us_diaeditcurdia->briefHeight <= 0) us_diaeditcurdia->briefHeight = 1;
	us_diaeditlastx = x;   us_diaeditlasty = y;
	us_diaeditredrawsingledia(us_diaeditcurdialog);
	us_diaeditdirty = 1;
}

/* Dialog Edit: Grid alignment */
static DIALOGITEM us_diaeditgriddialogitems[] =
{
 /*  1 */ {0, {32,8,52,68}, BUTTON, N_("OK")},
 /*  2 */ {0, {32,168,52,228}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,58,24,178}, EDITTEXT, x_("")}
};
static DIALOG us_diaeditgriddialog = {{450,56,511,293}, N_("Grid Amount"), 0, 3, us_diaeditgriddialogitems, 0, 0};

#define DGRI_AMOUNT   3		/* grid amount (edit text) */

/*
 * Routine to set the grid amount.
 */
void us_diaeditsetgridamount(void)
{
	INTBIG itemno;
	CHAR line[50];
	REGISTER void *dia;

	dia = DiaInitDialog(&us_diaeditgriddialog);
	if (dia == 0) return;
	esnprintf(line, 50, x_("%ld"), us_diaeditgridamt);
	DiaSetText(dia, DGRI_AMOUNT, line);
	for(;;)
	{
		itemno = DiaNextHit(dia);
		if (itemno == OK || itemno == CANCEL) break;
	}
	if (itemno == OK)
		us_diaeditgridamt = eatoi(DiaGetText(dia, DGRI_AMOUNT));
	DiaDoneDialog(dia);
}

/*
 * Routine to create a new item.
 */
void us_diaeditnewitem(void *dia)
{
	DIAITEM *item, *lastitem;
	CHAR *pt;

	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		item->selected = 0;
		lastitem = item;
	}
	item = (DIAITEM *)emalloc(sizeof (DIAITEM), us_tool->cluster);
	if (us_diaeditcurdia->firstitem == 0) us_diaeditcurdia->firstitem = item; else
		lastitem->next = item;
	item->next = 0;

	item->top = 20;
	item->left = 20;
	if (us_diaeditcurdia->firstitem == item) item->bottom = 44; else
		item->bottom = 36;
	item->right = 100;
	item->inactive = 0;
	item->selected = 1;
	item->type = TYPEBUTTON;
	pt = x_("item");
	allocstring(&item->msg, pt, us_tool->cluster);
    item->name = item->comment = 0;
	us_diaeditcurdia->numitems++;
	us_diaeditredrawsingledia(dia);
	us_diaeditdirty = 1;
}

/*
 * Routine to duplicate the selected item(s).
 */
void us_diaeditduplicateitem(void *dia)
{
	DIAITEM *item, *newitem, *lastitem;

	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
		lastitem = item;
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		if (item->selected != 1) continue;
		newitem = (DIAITEM *)emalloc(sizeof (DIAITEM), us_tool->cluster);
		if (us_diaeditcurdia->firstitem == 0) us_diaeditcurdia->firstitem = newitem; else
			lastitem->next = newitem;
		lastitem = newitem;
		newitem->next = 0;

		newitem->top = item->top + us_diaeditgridamt;
		newitem->left = item->left + us_diaeditgridamt;
		newitem->bottom = item->bottom + us_diaeditgridamt;
		newitem->right = item->right + us_diaeditgridamt;
		newitem->inactive = item->inactive;
		item->selected = 0;
		newitem->selected = 2;
		newitem->type = item->type;
		allocstring(&newitem->msg, item->msg, us_tool->cluster);
        newitem->name = newitem->comment = 0;
		us_diaeditcurdia->numitems++;
	}
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
		if (item->selected != 0) item->selected = 1;
	us_diaeditredrawsingledia(dia);
	us_diaeditdirty = 1;
}

/*
 * Routine to align all items in this dialog to the grid.
 */
void us_diaeditaligntogrid(void *dia)
{
	DIAITEM *item;

	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		if (item->selected == 0) continue;
		item->top = item->top / us_diaeditgridamt * us_diaeditgridamt;
		item->left = item->left / us_diaeditgridamt * us_diaeditgridamt;
		item->bottom = item->bottom / us_diaeditgridamt * us_diaeditgridamt;
		item->right = item->right / us_diaeditgridamt * us_diaeditgridamt;
		us_diaeditdirty = 1;
	}
	us_diaeditredrawsingledia(dia);
}

/********************************** ITEM CONTROL **********************************/

/* Dialog Edit: Item details */
static DIALOGITEM us_diaedititemdialogitems[] =
{
 /*  1 */ {0, {104,260,124,320}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,20,124,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {56,8,72,332}, EDITTEXT, x_("")},
 /*  4 */ {0, {8,8,24,58}, MESSAGE, N_("Top:")},
 /*  5 */ {0, {8,60,24,90}, EDITTEXT, x_("")},
 /*  6 */ {0, {32,8,48,58}, MESSAGE, N_("Left:")},
 /*  7 */ {0, {32,60,48,90}, EDITTEXT, x_("")},
 /*  8 */ {0, {8,110,24,160}, MESSAGE, N_("Bottom:")},
 /*  9 */ {0, {8,162,24,192}, EDITTEXT, x_("")},
 /* 10 */ {0, {32,110,48,160}, MESSAGE, N_("Right:")},
 /* 11 */ {0, {32,162,48,192}, EDITTEXT, x_("")},
 /* 12 */ {0, {80,8,96,214}, POPUP, x_("")},
 /* 13 */ {0, {80,228,97,332}, CHECK, N_("Inactive")},
 /* 14 */ {0, {20,200,36,332}, CHECK, N_("Width and Height")}
};
static DIALOG us_diaedititemdialog = {{450,56,583,397}, N_("Item Information"), 0, 14, us_diaedititemdialogitems, 0, 0};

#define DITD_MESSAGE  3			/* item text (edit text) */
#define DITD_TOP      5			/* top coordinate (edit text) */
#define DITD_LEFT     7			/* left coordinate (edit text) */
#define DITD_BOTTOM_L 8			/* bottom coordinate label (message) */
#define DITD_BOTTOM   9			/* bottom coordinate (edit text) */
#define DITD_RIGHT_L  10		/* right coordinate label (message) */
#define DITD_RIGHT    11		/* right coordinate (edit text) */
#define DITD_ITEMTYPE 12		/* item type (popup) */
#define DITD_INACTIVE 13		/* inactive item (check) */
#define DITD_WIDHEI   14		/* show width&height (check) */

/*
 * Routine to display the details of a single item.
 * returns nonzero if something changed.
 */
void us_diaeditshowdetails(DIAITEM *item, INTBIG whichitem)
{
	CHAR line[50], *pt;
	INTBIG itemno;
	REGISTER void *dia;

	esnprintf(line, 50, x_("Item %ld Details"), whichitem);
	us_diaedititemdialog.movable = line;
	dia = DiaInitDialog(&us_diaedititemdialog);
	if (dia == 0) return;
	DiaSetPopup(dia, DITD_ITEMTYPE, NUMTYPES, us_diaedittypes);
	DiaSetPopupEntry(dia, DITD_ITEMTYPE, item->type);
	if (item->inactive != 0) DiaSetControl(dia, DITD_INACTIVE, 1);
	esnprintf(line, 50, x_("%ld"), item->top);    DiaSetText(dia, DITD_TOP, line);
	esnprintf(line, 50, x_("%ld"), item->left);   DiaSetText(dia, DITD_LEFT, line);
	if (us_diaeditshowwidhei != 0)
	{
		DiaSetControl(dia, DITD_WIDHEI, 1);
		DiaSetText(dia, DITD_BOTTOM_L, x_("Height:"));
		DiaSetText(dia, DITD_RIGHT_L, x_("Width:"));
		esnprintf(line, 50, x_("%ld"), item->bottom - item->top);   DiaSetText(dia, DITD_BOTTOM, line);
		esnprintf(line, 50, x_("%ld"), item->right - item->left);   DiaSetText(dia, DITD_RIGHT, line);
	} else
	{
		esnprintf(line, 50, x_("%ld"), item->bottom);   DiaSetText(dia, DITD_BOTTOM, line);
		esnprintf(line, 50, x_("%ld"), item->right);    DiaSetText(dia, DITD_RIGHT, line);
	}
	DiaSetText(dia, -DITD_MESSAGE, item->msg);
	for(;;)
	{
		itemno = DiaNextHit(dia);
		if (itemno == OK || itemno == CANCEL) break;
		if (itemno == DITD_INACTIVE)
		{
			DiaSetControl(dia, itemno, 1 - DiaGetControl(dia, itemno));
			continue;
		}
		if (itemno == DITD_WIDHEI)
		{
			us_diaeditshowwidhei = 1 - us_diaeditshowwidhei;
			DiaSetControl(dia, itemno, us_diaeditshowwidhei);
			if (us_diaeditshowwidhei != 0)
			{
				DiaSetText(dia, DITD_BOTTOM_L, x_("Height:"));
				DiaSetText(dia, DITD_RIGHT_L, x_("Width:"));
				esnprintf(line, 50, x_("%ld"), item->bottom - item->top);   DiaSetText(dia, DITD_BOTTOM, line);
				esnprintf(line, 50, x_("%ld"), item->right - item->left);   DiaSetText(dia, DITD_RIGHT, line);
			} else
			{
				DiaSetText(dia, DITD_BOTTOM_L, x_("Bottom:"));
				DiaSetText(dia, DITD_RIGHT_L, x_("Right:"));
				esnprintf(line, 50, x_("%ld"), item->bottom);   DiaSetText(dia, DITD_BOTTOM, line);
				esnprintf(line, 50, x_("%ld"), item->right);    DiaSetText(dia, DITD_RIGHT, line);
			}
			continue;
		}
	}
	if (itemno == OK)
	{
		item->top = eatoi(DiaGetText(dia, DITD_TOP));
		item->left = eatoi(DiaGetText(dia, DITD_LEFT));
		if (us_diaeditshowwidhei != 0)
		{
			item->bottom = item->top + eatoi(DiaGetText(dia, DITD_BOTTOM));
			item->right = item->left + eatoi(DiaGetText(dia, DITD_RIGHT));
		} else
		{
			item->bottom = eatoi(DiaGetText(dia, DITD_BOTTOM));
			item->right = eatoi(DiaGetText(dia, DITD_RIGHT));
		}
		efree((CHAR *)item->msg);
		pt = DiaGetText(dia, DITD_MESSAGE);
		allocstring(&item->msg, pt, us_tool->cluster);
		item->type = DiaGetPopupEntry(dia, DITD_ITEMTYPE);
		item->inactive = DiaGetControl(dia, DITD_INACTIVE);
		us_diaeditdirty = 1;
	}
	DiaDoneDialog(dia);
}

/********************************** DISPLAY **********************************/

/*
 * Routine to highlight item "item" by inverting a rectangle around it.
 */
void us_diaeditinvertitem(DIAITEM *item, void *dia)
{
	INTBIG top, left, bottom, right;
	RECTAREA r;

	top = item->top+SINGLETOP-1;   bottom = item->bottom+SINGLETOP;
	left = item->left+SINGLELEFT-1;   right = item->right+SINGLELEFT;
	DiaDrawLine(dia, DONE_ITEMAREA, left, top, left, bottom, DLMODEINVERT);
	DiaDrawLine(dia, DONE_ITEMAREA, left, bottom, right, bottom, DLMODEINVERT);
	DiaDrawLine(dia, DONE_ITEMAREA, right, bottom, right, top, DLMODEINVERT);
	DiaDrawLine(dia, DONE_ITEMAREA, right, top, left, top, DLMODEINVERT);
	r.left = (INTSML)(right - GROWBOXSIZE);   r.right = (INTSML)right;
	r.top = (INTSML)(bottom - GROWBOXSIZE);   r.bottom = (INTSML)bottom;
	DiaInvertRect(dia, DONE_ITEMAREA, &r);
}

/*
 * Routine called to redisplay the dialog items.
 */
void us_diaeditredisproutine(RECTAREA *r, void *dia)
{
	Q_UNUSED( r );
	us_diaeditredrawsingledia(dia);
}

/*
 * Routine to redraw all of the items in the dialog.
 */
void us_diaeditredrawsingledia(void *dia)
{
	DIAITEM *item;
	RECTAREA r;
	CHAR *msg, line[50];
	INTBIG top, left, bottom, right, x, y, height, i;

	DiaItemRect(dia, DONE_ITEMAREA, &r);
	DiaDrawRect(dia, DONE_ITEMAREA, &r, 255, 255, 255);
	r.left = SINGLELEFT;
	r.right = (INTSML)(SINGLELEFT + us_diaeditcurdia->right-us_diaeditcurdia->left + BORDER);
	r.top = SINGLETOP;
    r.bottom = (INTSML)(SINGLETOP + us_diaeditcurdia->bottom-us_diaeditcurdia->top + BORDER);
	DiaFrameRect(dia, DONE_ITEMAREA, &r);
	r.left = r.right;   r.right = (INTSML)(r.left + GROWBOXSIZE);
	r.top = r.bottom;   r.bottom = (INTSML)(r.top + GROWBOXSIZE);
	DiaDrawRect(dia, DONE_ITEMAREA, &r, 0, 0, 0);
    if (us_diaeditcurdia->briefHeight != 0)
    {
        r.top = (INTSML)(SINGLETOP + us_diaeditcurdia->briefHeight);
        r.bottom = (INTSML)(r.top + GROWBOXSIZE);
        DiaDrawRect(dia, DONE_ITEMAREA, &r, 0, 0, 0);
        DiaDrawLine(dia, DONE_ITEMAREA, SINGLELEFT, r.top, r.left, r.top, DLMODEON);
    }
	DiaSetTextSize(dia, 12);

	for(item = us_diaeditcurdia->firstitem, i = 1; item != 0; item = item->next, i++)
	{
		top = item->top+SINGLETOP;   bottom = item->bottom+SINGLETOP;
		left = item->left+SINGLELEFT;   right = item->right+SINGLELEFT;
		switch (item->type)
		{
			case TYPEBUTTON:
			case TYPEDEFBUTTON:
				us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, item->msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, item->msg, left + (right-left-x)/2, bottom - (bottom-top+y)/2);
				break;
			case TYPECHECK:
			case TYPEAUTOCHECK:
				height = bottom - top - 4;
				if (us_diaeditshowoutline != 0)
					us_diadrawframe(left, top, right, bottom, dia);
				us_diadrawframe(left, top+2, left+height, bottom-2, dia);
				DiaDrawLine(dia, DONE_ITEMAREA, left, bottom-2, left+height, top+2, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, left, top+2, left+height, bottom-2, DLMODEON);
				DiaGetTextInfo(dia, item->msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, item->msg, left+height+5, bottom - (bottom-top+y)/2);
				break;
			case TYPERADIO:
			case TYPERADIOA:
			case TYPERADIOB:
			case TYPERADIOC:
			case TYPERADIOD:
			case TYPERADIOE:
			case TYPERADIOF:
			case TYPERADIOG:
				height = bottom - top - 4;
				if (us_diaeditshowoutline != 0)
					us_diadrawframe(left, top, right, bottom, dia);
				DiaDrawLine(dia, DONE_ITEMAREA, left+height/2, top+2, left+height, (top+bottom)/2, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, left+height, (top+bottom)/2, left+height/2, bottom-2, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, left+height/2, bottom-2, left, (top+bottom)/2, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, left, (top+bottom)/2, left+height/2, top+2, DLMODEON);
				DiaGetTextInfo(dia, item->msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, item->msg, left+height+5, bottom - (bottom-top+y)/2);
				break;
			case TYPESCROLL:
				msg = x_("SCROLL AREA");
				us_diadrawframe(left+2, top+2, right-2, bottom-2, dia);
				us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, msg, left + (right-left-x)/2, bottom - (bottom-top+y)/2);
				break;
			case TYPESCROLLMULTI:
				msg = x_("SCROLL AREA (MULTI)");
				us_diadrawframe(left+2, top+2, right-2, bottom-2, dia);
				us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, msg, left + (right-left-x)/2, bottom - (bottom-top+y)/2);
				break;
			case TYPEMESSAGE:
				if (us_diaeditshowoutline != 0)
					us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, item->msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, item->msg, left+5, bottom - (bottom-top+y)/2);
				break;
			case TYPEEDITTEXT:
			case TYPEOPAQUEEDIT:
				us_diadrawframe(left-2, top-2, right+2, bottom+2, dia);
				us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, item->msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, item->msg, left+5, bottom - (bottom-top+y)/2);
				break;
			case TYPEICON:
				msg = x_("ICON");
				us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, msg, left + (right-left-x)/2, bottom - (bottom-top+y)/2);
				break;
			case TYPEUSER:
				us_diadrawframe(left, top, right, bottom, dia);
				DiaDrawLine(dia, DONE_ITEMAREA, left, top, right, bottom, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, left, bottom, right, top, DLMODEON);
				break;
			case TYPEPOPUP:
				msg = x_("POPUP");
				us_diadrawframe(left, top, right, bottom, dia);
				height = (bottom - top) / 3;
				DiaDrawLine(dia, DONE_ITEMAREA, right-height, top+height, right-height*3, top+height, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, right-height*3, top+height, right-height*2, bottom-height, DLMODEON);
				DiaDrawLine(dia, DONE_ITEMAREA, right-height*2, bottom-height, right-height, top+height, DLMODEON);
				DiaGetTextInfo(dia, msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, msg, left + (right-left-x)/2, bottom - (bottom-top+y)/2);
				break;
			case TYPEPROGRESS:
				msg = x_("PROGRESS");
				us_diadrawframe(left, top, right, bottom, dia);
				DiaGetTextInfo(dia, msg, &x, &y);
				DiaPutText(dia, DONE_ITEMAREA, msg, left + (right-left-x)/2, bottom - (bottom-top+y)/2);
				break;
			case TYPEDIVIDER:
				us_diadrawframe(left, top, right, bottom, dia);
				break;
		}
		if (us_diaeditshownumbers != 0)
		{
			DiaSetTextSize(dia, 10);
			esnprintf(line, 50, x_("%ld"), i);
			DiaGetTextInfo(dia, line, &x, &y);
			DiaPutText(dia, DONE_ITEMAREA, line, right - x, top);
			DiaSetTextSize(dia, 12);
		}
		if (item->selected != 0) us_diaeditinvertitem(item, dia);
	}
}

/*
 * Routine to draw a rectangular outline.
 */
void us_diadrawframe(INTBIG left, INTBIG top, INTBIG right, INTBIG bottom, void *dia)
{
	RECTAREA r;

	r.left = (INTSML)left;   r.right = (INTSML)right;
	r.top = (INTSML)top;     r.bottom = (INTSML)bottom;
	DiaFrameRect(dia, DONE_ITEMAREA, &r);
}

/*
 * Routine to invert a rectangular outline.
 */
void us_diaeditinvertoutline(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, void *dia)
{
	DiaDrawLine(dia, DONE_ITEMAREA, fx, fy, fx, ty, DLMODEINVERT);
	DiaDrawLine(dia, DONE_ITEMAREA, fx, ty, tx, ty, DLMODEINVERT);
	DiaDrawLine(dia, DONE_ITEMAREA, tx, ty, tx, fy, DLMODEINVERT);
	DiaDrawLine(dia, DONE_ITEMAREA, tx, fy, fx, fy, DLMODEINVERT);
}

/********************************** INPUT/OUTPUT **********************************/

/*
 * Routine to read the dialogs into memory.
 */
void us_diaeditreaddialogs(void)
{
	FILE *in;
	DIA *dia, *lastdia;
	DIAITEM *item, *lastitem;
	CHAR *pt, *ept, *filename, *itemname;
	INTBIG len, itemnum;
	CHAR line[MAXLINE];
	REGISTER void *infstr;

	us_diaeditfirstdia = 0;
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, DIALOGFILE);
	pt = truepath(returninfstr(infstr));
	in = xopen(pt, el_filetypetext, x_(""), &filename);
	if (in == 0) return;
	for(;;)
	{
		if (xfgets(line, MAXLINE, in)) break;
		if (line[0] == 0) continue;
		if (estrncmp(line, x_("/*"), 2) == 0 && estrncmp(line, x_("/* special"), 10) != 0)
		{
			/* comment before a dialog */
			dia = (DIA *)emalloc(sizeof (DIA), us_tool->cluster);
			len = estrlen(line);
			line[len-3] = 0;
			allocstring(&dia->comment, &line[3], us_tool->cluster);
            dia->prefix = dia->abbrev = dia->title = dia->name = 0;
            dia->hasUi = FALSE;
			dia->firstitem = 0;
			dia->numitems = 0;
			if (us_diaeditfirstdia == 0) us_diaeditfirstdia = dia; else
				lastdia->next = dia;
			lastdia = dia;
			dia->next = 0;
			continue;
		}
		if (estrncmp(line, x_("static DIALOGITEM"), 17) == 0) continue;
		if (*line == '{') continue;
		if (estrncmp(line, x_(" /* "), 4) == 0)
		{
			/* an item */
			item = (DIAITEM *)emalloc(sizeof (DIAITEM), us_tool->cluster);
			if (dia->firstitem == 0) dia->firstitem = item; else
				lastitem->next = item;
			item->next = 0;
            item->name = item->comment = 0;
			lastitem = item;
			pt = &line[15];
			item->top = eatoi(pt);
			while (*pt != ',' && *pt != 0) pt++;
			if (*pt == ',') pt++;
			item->left = eatoi(pt);
			while (*pt != ',' && *pt != 0) pt++;
			if (*pt == ',') pt++;
			item->bottom = eatoi(pt);
			while (*pt != ',' && *pt != 0) pt++;
			if (*pt == ',') pt++;
			item->right = eatoi(pt);
			while (*pt != '}' && *pt != 0) pt++;
			if (*pt == 0) continue;
			pt += 3;
			item->type = TYPEUNKNOWN;
			if (estrncmp(pt, x_("BUTTON"), 6) == 0) item->type = TYPEBUTTON; else
			if (estrncmp(pt, x_("DEFBUTTON"), 9) == 0) item->type = TYPEDEFBUTTON; else
			if (estrncmp(pt, x_("CHECK"), 5) == 0) item->type = TYPECHECK; else
			if (estrncmp(pt, x_("AUTOCHECK"), 9) == 0) item->type = TYPEAUTOCHECK; else
			if (estrncmp(pt, x_("SCROLLMULTI"), 11) == 0) item->type = TYPESCROLLMULTI; else
			if (estrncmp(pt, x_("SCROLL"), 6) == 0) item->type = TYPESCROLL; else
			if (estrncmp(pt, x_("MESSAGE"), 7) == 0) item->type = TYPEMESSAGE; else
			if (estrncmp(pt, x_("EDITTEXT"), 8) == 0) item->type = TYPEEDITTEXT; else
			if (estrncmp(pt, x_("OPAQUEEDIT"), 10) == 0) item->type = TYPEOPAQUEEDIT; else
			if (estrncmp(pt, x_("ICON"), 4) == 0) item->type = TYPEICON; else
			if (estrncmp(pt, x_("USERDRAWN"), 9) == 0) item->type = TYPEUSER; else
			if (estrncmp(pt, x_("POPUP"), 5) == 0) item->type = TYPEPOPUP; else
			if (estrncmp(pt, x_("PROGRESS"), 8) == 0) item->type = TYPEPROGRESS; else
			if (estrncmp(pt, x_("DIVIDELINE"), 10) == 0) item->type = TYPEDIVIDER; else
            if (estrncmp(pt, x_("RADIOA"), 6) == 0) item->type = TYPERADIOA; else
            if (estrncmp(pt, x_("RADIOB"), 6) == 0) item->type = TYPERADIOB; else
            if (estrncmp(pt, x_("RADIOC"), 6) == 0) item->type = TYPERADIOC; else
            if (estrncmp(pt, x_("RADIOD"), 6) == 0) item->type = TYPERADIOD; else
            if (estrncmp(pt, x_("RADIOE"), 6) == 0) item->type = TYPERADIOE; else
            if (estrncmp(pt, x_("RADIOF"), 6) == 0) item->type = TYPERADIOF; else
            if (estrncmp(pt, x_("RADIOG"), 6) == 0) item->type = TYPERADIOG; else
			if (estrncmp(pt, x_("RADIO"), 5) == 0) item->type = TYPERADIO;
			while (*pt != ',' && *pt != '|' && *pt != 0) pt++;
			item->inactive = 0;
			if (estrncmp(pt, x_("|INACTIVE"), 9) == 0)
			{
				item->inactive = 1;
				pt += 9;
			}
			if (*pt == 0) continue;
			if (item->type == TYPEICON)
			{
				pt += 2;
				ept = pt;
				while (*ept != '}' && *ept != 0) ept++;
				*ept = 0;
				allocstring(&item->msg, pt, us_tool->cluster);
			} else
			{
				pt += 3;
				if ((pt[-1] == 'N' || pt[-1] == 'x') && pt[0] == '_' && pt[1] == '(') pt += 3;
				ept = pt;
				while (*ept != '"' && *ept != 0) ept++;
				*ept = 0;
				allocstring(&item->msg, pt, us_tool->cluster);
			}
			dia->numitems++;
			continue;
		}
		if (*line == '}') continue;
		if (estrncmp(line, x_("static DIALOG "), 14) == 0)
		{
			pt = &line[14];
            ept = pt;
			while (*ept != '_' && *ept != 0) ept++;
			*ept = 0;
			allocstring(&dia->prefix, pt, us_tool->cluster);
            ept = pt = ept + 1;
			while (*ept != ' ' && *ept != 0) ept++;
            *ept = 0;
            if (estrcmp(ept - 6, x_("dialog")) == 0) ept[-6] = 0;
			allocstring(&dia->abbrev, pt, us_tool->cluster);
			ept += 5;
			dia->top = eatoi(ept);
			while (*ept != ',' && *ept != 0) ept++;
			if (*ept == ',') ept++;
			dia->left = eatoi(ept);
			while (*ept != ',' && *ept != 0) ept++;
			if (*ept == ',') ept++;
			dia->bottom = eatoi(ept);
			while (*ept != ',' && *ept != 0) ept++;
			if (*ept == ',') ept++;
			dia->right = eatoi(ept);
			while (*ept != '}' && *ept != 0) ept++;
			ept += 3;
			if (*ept == '0') dia->title = 0; else
			{
				if ((ept[0] == 'N' || ept[0] == 'x') && ept[1] == '_' && ept[2] == '(') ept += 3;
				if (*ept == '"')
				{
					ept++;
					pt = ept;
					while (*pt != '"' && *pt != 0) pt++;
					*pt = 0;
					allocstring(&dia->title, ept, us_tool->cluster);
				}
                ept = pt + 1;
			}
            dia->briefHeight = 0;
            dia->hasUi = FALSE;
            while (*ept != ',' && *ept != '}' && *ept != 0) ept++;
			if (*ept == '}') continue;
			if (*ept == ',') ept++;
			while (*ept != ',' && *ept != '}' && *ept != 0) ept++;
            if (*ept == '}') continue;
            if (*ept == ',') ept++;
			while (*ept != ',' && *ept != '}' && *ept != 0) ept++;
            if (*ept == '}') continue;
            if (*ept == ',') ept++;
			while (*ept != ',' && *ept != '}' && *ept != 0) ept++;
            if (*ept == '}') continue;
            if (*ept == ',') ept++;
            while (*ept == ' ') ept++;
            if (*ept == '"' || estrncmp(ept, "x_(\"", 4) == 0) dia->hasUi = TRUE;
			while (*ept != ',' && *ept != '}' && *ept != 0) ept++;
            if (*ept == '}') continue;
            if (*ept == ',') ept++;
            dia->briefHeight = eatoi(ept);
			continue;
		}
        if (estrncmp(line, x_("#define "), 8) == 0)
        {
            pt = &line[8];
            ept = pt;
            while (*ept != ' ' && *ept != '_' && *ept != 0) ept++;
            *ept = 0;
            if (dia->name == 0)
				allocstring(&dia->name, pt, us_tool->cluster);
            if (estrcmp(dia->name, pt) != 0)
                ttyputmsg(x_("Ignore name %s, use %s"), pt, dia->name);
            ept = pt = ept + 1;
            while (*ept != ' ' && *ept != 0) ept++;
            *ept = 0;
			allocstring(&itemname, pt, us_tool->cluster);
            ept++;
            while (*ept == ' ' || *ept == '\t') ept++;
            pt = ept;
            while (*ept != ' ' && *ept != '\t' && *ept != 0) ept++;
            *ept = 0;
            itemnum = eatol(pt);
            if (itemnum <= 0 || itemnum > dia->numitems) continue;
            for (item = dia->firstitem; itemnum > 1; itemnum--) item = item->next;
            item->name = itemname;
            ept++;
            while (*ept == ' ' || *ept == '\t') ept++;
            if (estrncmp(ept, x_("/* "), 3) != 0) continue;
            ept += 3;
            pt = ept;
            while (*ept != 0 && *ept != '*' && *ept != '(') ept++;
            while (ept > pt && ept[-1] == ' ') ept--;
            *ept = 0;
			allocstring(&item->comment, pt, us_tool->cluster);
            continue;
        }
	}
	xclose(in);
}

/*
 * Routine to save the dialogs to disk.
 */
void us_diaeditsavedialogs(void)
{
	FILE *out;
	DIA *dia, **diaList;
	CHAR *truename;
	DIAITEM *item;
	INTBIG i, total;
	REGISTER void *infstr;
    INTBIG maxnamelen;

	/* sort the dialogs */
	for(total = 0, dia = us_diaeditfirstdia; dia != 0; dia = dia->next) total++;
	diaList = (DIA **)emalloc(total * (sizeof (DIA *)), us_tool->cluster);
	for(i = 0, dia = us_diaeditfirstdia; dia != 0; dia = dia->next) diaList[i++] = dia;
	esort(diaList, total, sizeof (DIA *), us_diaeditcommentascending);
	for(i=0; i<total-1; i++) diaList[i]->next = diaList[i+1];
	diaList[total-1]->next = 0;
	us_diaeditfirstdia = diaList[0];
	efree((CHAR *)diaList);

	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, DIALOGFILE);
	out = xcreate(returninfstr(infstr), el_filetypetext, 0, &truename);
	if (out == 0) return;
	for(dia = us_diaeditfirstdia; dia != 0; dia = dia->next)
	{
		efprintf(out, x_("/* %s */\n"), dia->comment);
		efprintf(out, x_("static DIALOGITEM %s_%sdialogitems[] =\n"), dia->prefix, dia->abbrev);
		efprintf(out, x_("{\n"));
        maxnamelen = 0;
		for(i=1, item = dia->firstitem; item != 0; item = item->next, i++)
		{
			efprintf(out, x_(" /* %2ld */ {0, {%ld,%ld,%ld,%ld}, "), i, item->top, item->left,
				item->bottom, item->right);
			switch (item->type)
			{
				case TYPEBUTTON:      efprintf(out, x_("BUTTON"));      break;
				case TYPEDEFBUTTON:   efprintf(out, x_("DEFBUTTON"));   break;
				case TYPECHECK:       efprintf(out, x_("CHECK"));       break;
				case TYPEAUTOCHECK:   efprintf(out, x_("AUTOCHECK"));   break;
				case TYPESCROLL:      efprintf(out, x_("SCROLL"));      break;
				case TYPESCROLLMULTI: efprintf(out, x_("SCROLLMULTI")); break;
				case TYPEMESSAGE:     efprintf(out, x_("MESSAGE"));     break;
				case TYPEEDITTEXT:    efprintf(out, x_("EDITTEXT"));    break;
				case TYPEOPAQUEEDIT:  efprintf(out, x_("OPAQUEEDIT"));  break;
				case TYPEICON:        efprintf(out, x_("ICON"));        break;
				case TYPEUSER:        efprintf(out, x_("USERDRAWN"));   break;
				case TYPEPOPUP:       efprintf(out, x_("POPUP"));       break;
				case TYPEPROGRESS:    efprintf(out, x_("PROGRESS"));    break;
				case TYPEDIVIDER:     efprintf(out, x_("DIVIDELINE"));  break;
				case TYPERADIO:       efprintf(out, x_("RADIO"));       break;
				case TYPERADIOA:      efprintf(out, x_("RADIOA"));      break;
				case TYPERADIOB:      efprintf(out, x_("RADIOB"));      break;
				case TYPERADIOC:      efprintf(out, x_("RADIOC"));      break;
				case TYPERADIOD:      efprintf(out, x_("RADIOD"));      break;
				case TYPERADIOE:      efprintf(out, x_("RADIOE"));      break;
				case TYPERADIOF:      efprintf(out, x_("RADIOF"));      break;
				case TYPERADIOG:      efprintf(out, x_("RADIOG"));      break;
			}
			if (item->inactive != 0) efprintf(out, x_("|INACTIVE"));
			efprintf(out, x_(", "));
			if (item->type == TYPEICON)
			{
				efprintf(out, x_("%s"), item->msg);
			} else
			{
				us_diaeditwritequotedstring(item->msg, out);
			}
			efprintf(out, x_("}"));
			if (item->next != 0) efprintf(out, x_(","));
			efprintf(out, x_("\n"));
            if (item->name && (INTBIG)estrlen(item->name) > maxnamelen)
				maxnamelen = estrlen(item->name);
		}
		efprintf(out, x_("};\n"));
		efprintf(out, x_("static DIALOG %s_%sdialog = {{%ld,%ld,%ld,%ld}, "), dia->prefix, dia->abbrev, dia->top, dia->left,
			dia->bottom, dia->right);
		if (dia->title == 0) efprintf(out, x_("0")); else
			us_diaeditwritequotedstring(dia->title, out);
		efprintf(out, x_(", 0"));
		efprintf(out, x_(", %ld, %s_%sdialogitems"), i-1, dia->prefix, dia->abbrev);
        if (dia->hasUi) efprintf(out, x_(", x_(\"%s\")"), dia->abbrev); else
            efprintf(out, x_(", 0"));
		efprintf(out, x_(", %ld};\n"), dia->briefHeight);
		efprintf(out, x_("\n"));
        if (maxnamelen > 0)
        {
            efprintf(out, x_("/* special items for the \"%s\" dialog: */\n"),  dia->abbrev);
            for(i=1, item = dia->firstitem; item != 0; item = item->next, i++)
            {
                if (item->name == 0) continue;
                efprintf(out, x_("#define %s_%s%*ld"), dia->name, item->name, maxnamelen - estrlen(item->name) + 3, i);
                if (item->comment != 0) efprintf(out, x_("\t\t/* %s (%s) */"), item->comment, us_diaeditshorttypes[item->type]);
                efprintf(out, x_("\n"));
            }
            efprintf(out, x_("\n"));
        }
	}
	xclose(out);
	us_diaeditdirty = 0;
}

/*
 * Routine to save the dialogs to .ui file..
 */
void us_diaeditsaveui(CHAR *dianame)
{
    FILE *out;
	CHAR *truename;
	REGISTER void *infstr;
    DIAITEM *item;
	INTBIG i;

	/* find the dialog */
	for(us_diaeditcurdia = us_diaeditfirstdia; us_diaeditcurdia != 0; us_diaeditcurdia = us_diaeditcurdia->next)
		if (estrcmp(us_diaeditcurdia->comment, dianame) == 0) break;
	if (us_diaeditcurdia == 0) return;
    for (item = us_diaeditcurdia->firstitem; item != 0 && item->name != 0; item = item->next);
    if (us_diaeditcurdia->name == 0 || item != 0)
    {
        ttyputmsg(x_("All dialog items should have names"));
        return;
    }

	infstr = initinfstr();
    formatinfstr(infstr, x_("%sui/%s.ui"), el_libdir, us_diaeditcurdia->abbrev);
	out = xcreate(returninfstr(infstr), el_filetypetext, 0, &truename);
	if (out == 0) return;
    efprintf(out, x_("<!DOCTYPE UI><UI version=\"3.0\" stdsetdef=\"1\">\n"));
    efprintf(out, x_("<class>%s</class>\n"), us_diaeditcurdia->abbrev);
    efprintf(out, x_("<widget class=\"EDialogPrivate\">\n"));
    efprintf(out, x_("    <property name=\"name\">\n"));
    efprintf(out, x_("        <cstring>%s</cstring>\n"), us_diaeditcurdia->name);
    efprintf(out, x_("    </property>\n"));
    efprintf(out, x_("    <property name=\"geometry\">\n"));
    efprintf(out, x_("        <rect>\n"));
    efprintf(out, x_("            <x>%d</x>\n"), QTSCALE(us_diaeditcurdia->left));
    efprintf(out, x_("            <y>%d</y>\n"), QTSCALE(us_diaeditcurdia->top));
    efprintf(out, x_("            <width>%d</width>\n"), QTSCALE(us_diaeditcurdia->right - us_diaeditcurdia->left));
	INTBIG height = us_diaeditcurdia->briefHeight != 0 ? us_diaeditcurdia->briefHeight : us_diaeditcurdia->bottom - us_diaeditcurdia->top;
    efprintf(out, x_("            <height>%d</height>\n"), QTSCALE(height));
    efprintf(out, x_("        </rect>\n"));
    efprintf(out, x_("    </property>\n"));
    if (us_diaeditcurdia->title != 0)
    {
        efprintf(out, x_("    <property name=\"caption\">\n"));
        efprintf(out, x_("        <string>%s</string>\n"), us_diaeditcurdia->title);
        efprintf(out, x_("    </property>\n"));
    }
	i = 0;
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		if (us_diaeditcurdia->briefHeight != 0 && item->bottom > us_diaeditcurdia->briefHeight) continue;
        efprintf(out, x_("    <widget class=\"%s\">\n"), us_diaeditqttypes[item->type]);
        efprintf(out, x_("        <property name=\"name\">\n"));
		efprintf(out, x_("            <cstring>%ld_%s</cstring>\n"), ++i, item->name);
        efprintf(out, x_("        </property>\n"));
        efprintf(out, x_("        <property name=\"geometry\">\n"));
        efprintf(out, x_("            <rect>\n"));
        efprintf(out, x_("                <x>%d</x>\n"), QTSCALE(item->left));
        efprintf(out, x_("                <y>%d</y>\n"), QTSCALE(item->top));
        efprintf(out, x_("                <width>%d</width>\n"), QTSCALE(item->right - item->left));
        efprintf(out, x_("                <height>%d</height>\n"), QTSCALE(item->bottom - item->top));
        efprintf(out, x_("            </rect>\n"));
        efprintf(out, x_("        </property>\n"));
        if (item->msg != 0 && item->type != TYPEICON && *item->msg != 0)
        {
            efprintf(out, x_("        <property name=\"text\">\n"));
            efprintf(out, x_("            <string>%s</string>\n"), item->msg);
            efprintf(out, x_("        </property>\n"));
        }
        efprintf(out, x_("    </widget>\n"));
    }
    efprintf(out, x_("</widget>\n"));
    efprintf(out, x_("</UI>\n"));
    xclose(out);
    ttyputmsg(x_("Form %s created"), truename);
	if (us_diaeditcurdia->briefHeight == 0) return;

	infstr = initinfstr();
    formatinfstr(infstr, x_("%sui/%s_ext.ui"), el_libdir, us_diaeditcurdia->abbrev);
	out = xcreate(returninfstr(infstr), el_filetypetext, 0, &truename);
	if (out == 0) return;
    efprintf(out, x_("<!DOCTYPE UI><UI version=\"3.0\" stdsetdef=\"1\">\n"));
    efprintf(out, x_("<class>%s_ext</class>\n"), us_diaeditcurdia->abbrev);
    efprintf(out, x_("<widget class=\"QWidget\">\n"));
    efprintf(out, x_("    <property name=\"name\">\n"));
    efprintf(out, x_("        <cstring>%s_ext</cstring>\n"), us_diaeditcurdia->name);
    efprintf(out, x_("    </property>\n"));
    efprintf(out, x_("    <property name=\"geometry\">\n"));
    efprintf(out, x_("        <rect>\n"));
    efprintf(out, x_("            <x>%d</x>\n"), 0);
    efprintf(out, x_("            <y>%d</y>\n"), 0);
    efprintf(out, x_("            <width>%d</width>\n"), QTSCALE(us_diaeditcurdia->right - us_diaeditcurdia->left));
    efprintf(out, x_("            <height>%d</height>\n"), QTSCALE(us_diaeditcurdia->bottom - us_diaeditcurdia->top - us_diaeditcurdia->briefHeight));
    efprintf(out, x_("        </rect>\n"));
    efprintf(out, x_("    </property>\n"));
	for(item = us_diaeditcurdia->firstitem; item != 0; item = item->next)
	{
		if (item->bottom <= us_diaeditcurdia->briefHeight) continue;
        efprintf(out, x_("    <widget class=\"%s\">\n"), us_diaeditqttypes[item->type]);
        efprintf(out, x_("        <property name=\"name\">\n"));
		efprintf(out, x_("            <cstring>%ld_%s</cstring>\n"), ++i, item->name);
        efprintf(out, x_("        </property>\n"));
        efprintf(out, x_("        <property name=\"geometry\">\n"));
        efprintf(out, x_("            <rect>\n"));
        efprintf(out, x_("                <x>%d</x>\n"), QTSCALE(item->left));
        efprintf(out, x_("                <y>%d</y>\n"), QTSCALE(item->top - us_diaeditcurdia->briefHeight));
        efprintf(out, x_("                <width>%d</width>\n"), QTSCALE(item->right - item->left));
        efprintf(out, x_("                <height>%d</height>\n"), QTSCALE(item->bottom - item->top));
        efprintf(out, x_("            </rect>\n"));
        efprintf(out, x_("        </property>\n"));
        if (item->msg != 0 && item->type != TYPEICON && *item->msg != 0)
        {
            efprintf(out, x_("        <property name=\"text\">\n"));
            efprintf(out, x_("            <string>%s</string>\n"), item->msg);
            efprintf(out, x_("        </property>\n"));
        }
        efprintf(out, x_("    </widget>\n"));
    }
    efprintf(out, x_("</widget>\n"));
    efprintf(out, x_("</UI>\n"));
    xclose(out);
    ttyputmsg(x_("Form %s created"), truename);
}

/*
 * Routine to load and copate all dialogs from .ui file..
 */
void    us_diaeditloaduiall()
{
	for(us_diaeditcurdia = us_diaeditfirstdia; us_diaeditcurdia != 0; us_diaeditcurdia = us_diaeditcurdia->next)
	{
		if(us_diaeditcurdia->hasUi) 
			us_diaeditloadui(us_diaeditcurdia->comment);
	}
}

/*
 * Routine to load the dialogs from .ui file..
 */
void us_diaeditloadui(CHAR *dianame)
{
#ifdef USEQUI
    DIAITEM *item;
    BOOLEAN err = FALSE;
    uint i;

	/* find the dialog */
	for(us_diaeditcurdia = us_diaeditfirstdia; us_diaeditcurdia != 0; us_diaeditcurdia = us_diaeditcurdia->next)
		if (estrcmp(us_diaeditcurdia->comment, dianame) == 0) break;
	if (us_diaeditcurdia == 0) return;

    QStrList *slist = EDialog::itemNamesFromUi( us_diaeditcurdia->abbrev );
    if (!slist)
    {
        ttyputmsg(x_("Form %s.ui not found"), us_diaeditcurdia->abbrev);
        return;
    }
	if (us_diaeditcurdia->briefHeight != 0)
	{
		QStrList *slist_ext = EDialog::itemNamesFromUi( us_diaeditcurdia->abbrev, TRUE );
		if (!slist_ext)
		{
			ttyputmsg(x_("Form %s_ext.ui not found"), us_diaeditcurdia->abbrev);
			delete slist;
			return;
		}
		for (slist_ext->first(); slist_ext->current(); slist_ext->next())
			slist->append( slist_ext->current() );
		delete slist_ext;
	}
	DIAITEM **items = (DIAITEM **)emalloc(slist->count() * (sizeof (DIAITEM *)), us_tool->cluster);
    for (i = 0; i < slist->count(); i++) items[i] = 0;
    BOOLEAN changed = FALSE;
	for(i=0, item = us_diaeditcurdia->firstitem; item != 0; item = item->next, i++)
    {
        if (item->name == 0)
        {
            ttyputmsg(x_("All dialog items should have names"));
            err = TRUE;
            continue;
        }
		char line[100];
		sprintf(line, "%d_%s", i+1, item->name);
        int k = slist->find( line );
        if (k < 0)
        {
            ttyputmsg(x_("Item %s not found in .ui"), line);
            err = TRUE;
            continue;
        }
        if (items[k] != 0)
        {
            ttyputmsg(x_("Duplicate item %s"), line);
            err = TRUE;
        }
        if (k != i) changed = TRUE;
        items[k] = item;
        
    }
    for (i = 0; i < slist->count(); i++)
    {
        if (items[i] == 0)
        {
            ttyputmsg(x_("Item %s not found in AllDialogs.c"), slist->at(i));
            err = TRUE;
        }
    }
    if (!err)
    {
		ttyputmsg(x_("Dialog \"%s\" matches %s.ui"), dianame, us_diaeditcurdia->abbrev);
    }
    delete slist;
#else
    ttyputmsg(x_("Loading .ui is disabled"));
#endif
}

/*
 * Sorting coroutine to order the dialogs by their comment field.
 */
int us_diaeditcommentascending(const void *e1, const void *e2)
{
	DIA *d1, *d2;

	d1 = *((DIA **)e1);
	d2 = *((DIA **)e2);
	return(estrcmp(d1->comment, d2->comment));
}

/*
 * Routine to write "string" to "out", quoting appropriately for language translation.
 */
void us_diaeditwritequotedstring(CHAR *string, FILE *out)
{
	CHAR *pt;

	if (*string == 0)
	{
		efprintf(out, x_("x_(\"\")"));
		return;
	}
	for(pt = string; *pt != 0; pt++)
		if (isalpha(*pt) != 0) break;
	if (*pt != 0) efprintf(out, x_("N_(\"%s\")"), string); else
		efprintf(out, x_("x_(\"%s\")"), string);
}

/* Dialog Edit: Save */
static DIALOGITEM us_diaeditsavedialogitems[] =
{
 /*  1 */ {0, {8,8,32,68}, BUTTON, N_("Save")},
 /*  2 */ {0, {8,88,32,148}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,168,32,228}, BUTTON, N_("No Save")}
};
static DIALOG us_diaeditsavedialog = {{450,56,491,293}, N_("Dialogs changed.  Save?"), 0, 3, us_diaeditsavedialogitems, 0, 0};

#define DSAV_SAVE    1		/* save dialogs before exiting (button) */
#define DSAV_CANCEL  2		/* cancel exit (button) */
#define DSAV_NOSAVE  3		/* do not save dialogs before exiting (button) */

/*
 * Routine to ensure that the dialogs are saved.  Returns nonzero if
 * they are saved, zero if the editor should not quit.
 */
INTBIG us_diaeditensuressaved(void)
{
	REGISTER INTBIG itemno;
	REGISTER void *dia;

	if (us_diaeditdirty != 0)
	{
		dia = DiaInitDialog(&us_diaeditsavedialog);
		if (dia == 0) return(1);
		for(;;)
		{
			itemno = DiaNextHit(dia);
			if (itemno == DSAV_SAVE)
			{
				us_diaeditsavedialogs();
				break;
			}
			if (itemno == DSAV_CANCEL || itemno == DSAV_NOSAVE) break;
		}
		DiaDoneDialog(dia);
		if (itemno == DSAV_CANCEL) return(0);
	}
	return(1);
}
