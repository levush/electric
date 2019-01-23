/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphmac.c
 * Interface for Apple Macintosh computers
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
 
/* Macintosh things to do:
 *		Function keys and non-command keys should have menu listing on the right
 *		Component menu can be too tall:
 *			Always placed at (0,0), even if main screen is elsewhere
 *			Should add parameters to "getpaletteparameters" for use in "usrmisc.c"
 *		Implement tear-off menus
 *		Reimplement dialogs native
 *		Session logging
 */
#  ifdef __MWERKS__
#    if __MWERKS__ >= 0x2100
#      define NEWCODEWARRIOR    1
#    endif
#  endif
#include "global.h"
#include "database.h"
#include "egraphics.h"
#include "usr.h"
#include "eio.h"
#include "efunction.h"
#include "edialogs.h"
#include "usrdiacom.h"
#include "usrtrack.h"
#include "dblang.h"

#include <Controls.h>
#include <Devices.h>
#include <Dialogs.h>
#include <Fonts.h>
#include <Gestalt.h>
#include <LowMem.h>
#include <Menus.h>
#include <Retrace.h>
#include <Scrap.h>
#include <Script.h>
#include <signal.h>
#include <Sound.h>
#include <ToolUtils.h>
#include <unix.h>

#if	LANGTCL
#  include "dblang.h"
#  include "tclMac.h"
#endif

/****** the messages window ******/
static WindowPtr        gra_messageswindow;		/* the scrolled messages window */
static BOOLEAN          gra_messagesinfront;	/* nonzero if messages is frontmost */
static INTBIG           gra_linesInFolder;		/* lines in text folder */
static INTBIG           gra_messagesfont;		/* font in messages window */
static INTBIG           gra_messagesfontsize;	/* size of font in messages window */
static INTBIG           gra_messagesleft;		/* left bound of messages window screen */
static INTBIG           gra_messagesright;		/* right bound of messages window screen */
static INTBIG           gra_messagestop;		/* top bound of messages window screen */
static INTBIG           gra_messagesbottom;		/* bottom bound of messages window screen */
static ControlHandle    gra_vScroll;			/* vertical scroll control in messages window */
static ControlHandle    gra_hScroll;			/* horizontal scroll control in messages window */
static TEHandle         gra_TEH;				/* text editing handle in messages window */
static ControlActionUPP gra_scrollvprocUPP;		/* UPP for "gra_scrollvproc" */
static ControlActionUPP gra_scrollhprocUPP;		/* UPP for "gra_scrollhproc" */
static GWorldPtr        gra_textbuf = 0;		/* temp buffer for displaying text */
static INTBIG           gra_textbufwid = 0;		/* width of text temp buffer */
static INTBIG           gra_textbufhei = 0;		/* height of text temp buffer */
static UCHAR1         **gra_textbufrowstart;	/* row start array for text temp buffer */
static BOOLEAN          gra_texttoosmall = FALSE; /* TRUE if text too small to draw */

/****** events ******/
#define CHARREAD          0377					/* character that was read */
#define ISKEYSTROKE       0400					/* set if key typed */
#define ISBUTTON         01000					/* set if button pushed (or released) */
#define BUTTONUP         02000					/* set if button was released */
#define SHIFTISDOWN      04000					/* set if shift key was held down */
#define COMMANDISDOWN   010000					/* set if command key was held down */
#define OPTIONISDOWN    020000					/* set if option key was held down */
#define CONTROLISDOWN   040000					/* set if control key was held down */
#define DOUBLECLICK    0100000					/* set if this is second click */
#define MOTION         0200000					/* set if mouse motion detected */
#define FILEREPLY      0400000					/* set if standard file reply detected */
#define WINDOWCHANGE  01000000					/* set if window moved and/or grown */
#define NOEVENT             -1					/* set if nothing happened */
#define MENUEVENT     BUTTONUP					/* set if menu entry selected (values in cursor) */

#ifdef NEWCODEWARRIOR
#  define SIGNALCAST __signal_func_ptr
#else
#  define SIGNALCAST _Sigfun*
#endif

struct
{
	INTBIG x;
	INTBIG y;
	INTBIG kind;
} gra_action;

static INTBIG        gra_inputstate;			/* current state of device input */
static INTBIG        gra_inputspecial;			/* current "special" keyboard value */
static INTBIG        gra_lastclick;				/* time of last click */
static INTBIG        gra_lstcurx, gra_lstcury;	/* current position of mouse */
static INTBIG        gra_cursorx, gra_cursory;	/* current position of mouse */
static WindowPtr     gra_lastclickedwindow = 0;		/* last window where mouse-down happened */
static WINDOWFRAME  *gra_lastclickedwindowframe = NOWINDOWFRAME;	/* last window frame where mouse-down happened */

//#define EVENTQUEUESIZE	100

//typedef struct
//{
//	INTBIG           cursorx, cursory;			/* current position of mouse */
//	INTBIG           inputstate;				/* current state of device input */
//	INTBIG           special;					/* current "special" code for keyboard */
//} MYEVENTQUEUE;

//static MYEVENTQUEUE  gra_eventqueue[EVENTQUEUESIZE];
//static MYEVENTQUEUE *gra_eventqueuehead;		/* points to next event in queue */
//static MYEVENTQUEUE *gra_eventqueuetail;		/* points to first free event in queue */

void gra_onint(void);

/****** timing ******/
#define FLUSHTICKS  120							/* ticks between display flushes */
#define MOTIONCHECK   2							/* ticks between mouse motion checks */
#define INTCHECK     30							/* MOTIONCHECKs between interrupt checks */
typedef struct VBLRect
{
	VBLTask myVBLTask;
	long    vblA5;
} VBLRec;

static VBLRec        gra_vblrec;
static INTBIG        gra_checkcountdown;		/* countdown to interrupt checks */
static BOOLEAN       gra_motioncheck;			/* true if mouse motion can be checked */
static BOOLEAN       gra_cancheck;				/* true if interrupt can be checked */
static UINTBIG       gra_timestart;

/****** stuff for TCL/TK ******/
#if	LANGTCL
  Tcl_Interp *myTCLInterp;
#endif

/****** pulldown menus ******/
#define MENUSIZE              19				/* size of menu bar */
#define appleMENU            128				/* resource ID for Apple menu */
#define USERMENUBASE         130				/* base resource ID for other menus */
#define aboutMeCommand         1				/* menu entry for "About Electric..." item */

static INTBIG        gra_lowmenu;				/* low word of selected pulldown menu */
static INTBIG        gra_highmenu;				/* high word of selected pulldown menu */
static INTBIG        gra_pulldownmenucount;		/* number of pulldown menus */
static INTBIG        gra_pulldownmenutotal = 0;	/* number of pulldown menus allocated */
static BOOLEAN       gra_tkmenusloaded = FALSE;	/* nonzero if TK menus are built */
static MenuHandle   *gra_pulldownmenus;			/* the current pulldown menu handles */
static CHAR        **gra_pulldowns;				/* the current pulldown menus */
static MenuHandle    gra_appleMenu;				/* the Apple menu */

/****** mouse buttons ******/
#define BUTTONS      17							/* cannot exceed NUMBUTS in "usr.h" */

typedef struct
{
	CHAR  *name;								/* button name */
	INTBIG unique;								/* number of letters that make it unique */
} BUTTONNAMES;

static BUTTONNAMES   gra_buttonname[BUTTONS] =
{						/* Shift Command Option Control */
	x_("Button"),      1,   /*                              */
	x_("SButton"),     2,	/* Shift                        */
	x_("MButton"),     2,	/*       Command                */
	x_("SMButton"),    3,	/* Shift Command                */
	x_("OButton"),     2,	/*               Option         */
	x_("SOButton"),    3,	/* Shift         Option         */
	x_("MOButton"),    3,	/*       Command Option         */
	x_("SMOButton"),   4,	/* Shift Command Option         */
	x_("CButton"),     2,	/*                      Control */
	x_("SCButton"),    3,	/* Shift                Control */
	x_("CMButton"),    3,	/*       Command        Control */
	x_("SCMButton"),   4,	/* Shift Command        Control */
	x_("COButton"),    3,	/*               Option Control */
	x_("SCOButton"),   4,	/* Shift         Option Control */
	x_("CMOButton"),   4,	/*       Command Option Control */
	x_("SCMOButton"),  5,	/* Shift Command Option Control */
	x_("DButton"),     2
};
static INTBIG        gra_doubleclick;					/* interval between double clicks */

/****** cursors ******/
enum {wantttyCURSOR = 129, penCURSOR, nullCURSOR, menulCURSOR, handCURSOR, techCURSOR,
	lrCURSOR, udCURSOR};

static CursHandle    gra_wantttyCurs;			/* a "use the TTY" cursor */
static CursHandle    gra_penCurs;				/* a "draw with pen" cursor */
static CursHandle    gra_nullCurs;				/* a null cursor */
static CursHandle    gra_menuCurs;				/* a menu selection cursor */
static CursHandle    gra_handCurs;				/* a hand dragging cursor */
static CursHandle    gra_techCurs;				/* a technology edit cursor */
static CursHandle    gra_ibeamCurs;				/* a text edit cursor */
static CursHandle    gra_lrCurs;				/* a left/right cursor */
static CursHandle    gra_udCurs;				/* an up/down cursor */

/****** rectangle saving ******/
#define NOSAVEDBOX ((SAVEDBOX *)-1)

typedef struct Isavedbox
{
	UCHAR1           *pix;
	WINDOWPART       *win;
	INTBIG            lx, hx, ly, hy;
	INTBIG            truelx, truehx;
	struct Isavedbox *nextsavedbox;
} SAVEDBOX;

SAVEDBOX *gra_firstsavedbox = NOSAVEDBOX;

/****** dialogs ******/
#define aboutMeDLOG           128				/* resource ID for "About Electric" alert dialog */
#define errorALERT            130				/* resource ID for "error" alert dialog */
#define NAMERSRC             2000				/* resource ID for user name */
#define COMPANYRSRC          2001				/* resource ID for company name */
#define SPECIALRSRC          2002				/* resource ID for special instructions */
#define CHECKRSRC            2003				/* resource ID for checksum */
#define MAXSCROLLMULTISELECT 1000

/* the four scroller arrows */
#define UPARROW         0
#define DOWNARROW       1
#define LEFTARROW       2
#define RIGHTARROW      3

#define THUMBSIZE      16		/* width of the thumb area in scroll slider */
#define MAXSCROLLS      4		/* maximum scroll items in a dialog */
#define MAXLOCKS        3		/* maximum locked pairs of scroll lists */
#define MAXMATCH       50
#define MINSCROLLTICKS  2		/* minimum ticks between scrollbar slider arrows */
#define MINPAGETICKS   20		/* minimum ticks between scrollbar slider page shifts */

typedef void (*USERTYPE)(RECTAREA*, void*);

typedef struct
{
	INTBIG  count;
	INTBIG  current;
	CHAR **namelist;
} POPUPDATA;

typedef struct
{
	INTBIG   scrollitem;		/* item number of SCROLL area (-1 if none) */
	RECTAREA userrect;			/* position of SCROLL area */
	INTBIG   flags;				/* state SCROLL area */
	INTBIG   vthumbpos;			/* position of vertical thumb slider  */
	INTBIG   hthumbpos;			/* position of horizontal thumb slider */
	INTBIG   horizfactor;		/* shift of horizontal text (0 to 100) */
	INTBIG   firstline;			/* line number of top line */
	INTBIG   linesinfolder;		/* number of lines displayable */
	INTBIG   whichlow;			/* first currently highlighted line */
	INTBIG   whichhigh;			/* last currently highlighted line */
	INTBIG   lineheight;		/* height of line of text */
	INTBIG   lineoffset;		/* offset to baseline for text */
	CHAR   **scrolllist;		/* list of text lines */
	INTBIG   scrolllistsize;	/* size of line list */
	INTBIG   scrolllistlen;		/* number of valid lines/list */
} DSCROLL;

#define NOTDIALOG ((TDIALOG *)-1)

typedef struct Itdialog
{
	DIALOG    *dlgresaddr;			/* address of this dialog */
	INTBIG     defaultbutton;		/* default button */
	WindowPtr  theDialog;

	/* for the scroll item */
	INTBIG     scrollcount;			/* number of scroll items */
	INTBIG     curscroll;			/* current scroll item */
	DSCROLL    scroll[MAXSCROLLS];	/* data structures for the scroll item(s) */
	INTBIG     numlocks, lock1[MAXLOCKS], lock2[MAXLOCKS], lock3[MAXLOCKS];

	/* for the current edit text item */
	INTBIG     curitem;				/* current edit item */
	INTBIG     editstart, editend;	/* start/end selected text in edit item */
	INTBIG     firstch;				/* first displayed character in edit item */
	INTBIG     opaqueitem;			/* item number of opaque edit text */
	INTBIG     usertextsize;		/* size of text in userdrawn fields */
	INTBIG     userdoubleclick;		/* nonzero if double-click in user area returns OK */

	/* information for this dialog */
	INTBIG     xoffset, yrev;
	INTBIG     revy;				/* for reversing Y coordinates */
	INTBIG     slineheight;			/* height of a line of scroll text */
	INTBIG     slineoffset;			/* scroll text: distance up to baseline */
	INTBIG     lineheight;			/* height of a line of other text */
	INTBIG     lineoffset;			/* other text: distance up to baseline */
	INTBIG     firstupdate;
	INTBIG     curlineoffset;

	/* during tracking */
	RECTAREA   rect;				/* the current rectangle being tracked */
	CHAR      *msg;
	POPUPDATA *pd;					/* the current popup menu */
	INTBIG     cnt;					/* the current entry count in the popup menu */
	INTBIG     sta;					/* the current start point in the popup menu */
	UINTBIG    lastdiatime;			/* time of last scrollbar slider button action */
	INTBIG     lastx, lasty;
	INTBIG     wasin, lbase, hbase, offset, ddata;

	void      (*diaeachdown)(INTBIG x, INTBIG y);
	void      (*modelessitemhit)(void *dia, INTBIG item);
 	struct Itdialog *nexttdialog;	/* next in list of active dialogs */
} TDIALOG;

static TDIALOG        *gra_firstactivedialog = NOTDIALOG;
static TDIALOG        *gra_trackingdialog = 0;
static TDIALOG        *gra_handlingdialog = NOTDIALOG;	/* address of dialog handling this event */
static FileFilterUPP   gra_fileFilterProcUPP;
static INTBIG          gra_dialogstringpos;

/* prototypes for local routines */
static pascal  Boolean gra_fileFilterProc(CInfoPBPtr pb, Ptr mydata);
static int     gra_stringposascending(const void *e1, const void *e2);
static BOOLEAN gra_initdialog(DIALOG *dialog, TDIALOG *dia, BOOLEAN modeless);
static BOOLEAN gra_diaeachdownhandler(INTBIG x, INTBIG y);
static INTBIG  gra_getnextcharacter(TDIALOG *dia, INTBIG oak, INTBIG x, INTBIG y, INTBIG chr,
				INTBIG special, UINTBIG time, BOOLEAN shifted);
static void    DTextCopy(TDIALOG *dia);
static void    DTextCut(TDIALOG *dia);
static CHAR    DTextPaste(TDIALOG *dia);
static BOOLEAN Dbuttondown(INTBIG x, INTBIG y);
static BOOLEAN Dcheckdown(INTBIG x, INTBIG y);
static void    Dclear(TDIALOG *dia);
static void    Dcorrectxy(TDIALOG *dia, INTBIG *x, INTBIG *y);
static void    Ddonedialogwindow(TDIALOG *dia);
static void    Ddoneedit(TDIALOG *dia);
static BOOLEAN Ddownarrow(INTBIG x, INTBIG y);
static BOOLEAN Ddownpage(INTBIG x, INTBIG y);
static BOOLEAN Ddragwin(INTBIG x, INTBIG y);
static void    Ddragwindow(TDIALOG *dia, INTBIG x, INTBIG y);
static void    Ddrawarrow(TDIALOG *dia, INTBIG sc, INTBIG which, INTBIG filled);
static void    Ddrawbox(TDIALOG *dia, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, GRAPHICS *which);
static void    Ddrawcircle(TDIALOG *dia, RECTAREA *r, INTBIG dim);
static void    Ddrawdisc(TDIALOG *dia, RECTAREA *r);
static void    Ddrawhorizslider(TDIALOG *dia, INTBIG sc);
static void    Ddrawitem(TDIALOG *dia, INTBIG type, RECTAREA *r, CHAR *msg, INTBIG dim);
static void    Ddrawline(TDIALOG *dia, INTBIG xf, INTBIG yf, INTBIG xt, INTBIG yt);
static void    Ddrawmsg(TDIALOG *dia, INTBIG sc, CHAR *msg, INTBIG which);
static void    Ddrawpolygon(TDIALOG *dia, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG filled);
static void    Ddrawpopupentries(TDIALOG *dia);
static void    Ddrawrectframe(TDIALOG *dia, RECTAREA *r, INTBIG on, INTBIG dim);
static void    Ddrawroundrectframe(TDIALOG *dia, RECTAREA *r, INTBIG arc, INTBIG dim);
static void    Ddrawtext(TDIALOG *dia, CHAR *msg, INTBIG len, INTBIG x, INTBIG y, INTBIG dim);
static void    Ddrawvertslider(TDIALOG *dia, INTBIG sc);
static void    Deditbox(TDIALOG *dia, RECTAREA *r, INTBIG draw, INTBIG dim);
static BOOLEAN Deditdown(INTBIG x, INTBIG y);
static void    Dforcedialog(TDIALOG *dia);
static INTBIG  Dgetcolorindex(INTBIG r, INTBIG g, INTBIG b);
static INTBIG  Dgeteditpos(TDIALOG *dia, RECTAREA *r, INTBIG x, INTBIG y, CHAR *msg);
static INTBIG  Dgettextsize(TDIALOG *dia, CHAR *msg, INTBIG len);
static void    Dgetwindowextent(TDIALOG *dia, INTBIG *ly, INTBIG *hy);
static void    Dgrayrect(TDIALOG *dia, RECTAREA *r);
static INTBIG  Dhandlepopup(TDIALOG *dia, RECTAREA *r, POPUPDATA *pd);
static void    Dhighlight(TDIALOG *dia, INTBIG on);
static void    Dhighlightrect(TDIALOG *dia, RECTAREA *r);
static BOOLEAN Dhscroll(INTBIG x, INTBIG y);
static void    Dinsertstr(TDIALOG *dia, CHAR *insmsg);
static void    Dinsetrect(RECTAREA *r, INTBIG amt);
static void    Dintdrawrect(TDIALOG *dia, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b);
static void    Dinvertbox(TDIALOG *dia, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy);
static void    Dinvertentry(TDIALOG *dia, INTBIG sc, INTBIG on);
static void    Dinvertrect(TDIALOG *dia, RECTAREA *r);
static void    Dinvertrectframe(TDIALOG *dia, RECTAREA *r);
static void    Dinvertroundrect(TDIALOG *dia, RECTAREA *r);
static BOOLEAN Dleftarrow(INTBIG x, INTBIG y);
static BOOLEAN Dleftpage(INTBIG x, INTBIG y);
static void    Dnewdialogwindow(TDIALOG *dia, RECTAREA *r, CHAR *movable, BOOLEAN modeless);
static INTBIG  Dneweditbase(TDIALOG *dia);
static void    Dputicon(TDIALOG *dia, INTBIG x, INTBIG y, UCHAR1 *data);
static void    Dredrawscroll(TDIALOG *dia, INTBIG sc);
static void    Drestorerect(INTBIG sr);
static BOOLEAN Drightarrow(INTBIG x, INTBIG y);
static BOOLEAN Drightpage(INTBIG x, INTBIG y);
static INTBIG  Dsaverect(TDIALOG *dia, RECTAREA *r);
static void    Dsetscroll(TDIALOG *dia, INTBIG item, INTBIG value);
static void    Dsyncvscroll(TDIALOG *dia, INTBIG item);
static void    Dsethscroll(TDIALOG *dia, INTBIG sc);
static void    Dsettextsmall(TDIALOG *dia, INTBIG sc);
static void    Dsetvscroll(TDIALOG *dia, INTBIG sc);
static void    Dshiftbits(TDIALOG *dia, RECTAREA *sr, RECTAREA *dr);
static void    Dstuffmessage(TDIALOG *dia, CHAR *msg, RECTAREA *r, INTBIG dim);
static void    Dstufftext(TDIALOG *dia, CHAR *msg, RECTAREA *r);
static void    Dtextlocation(TDIALOG *dia, CHAR *msg, INTBIG len, RECTAREA *r, INTBIG *wid, INTBIG *line);
static void    Dtrackcursor(TDIALOG *dia, INTBIG, INTBIG, BOOLEAN (*eachdown)(INTBIG, INTBIG));
static BOOLEAN Dtrackpopup(INTBIG x, INTBIG y);
static BOOLEAN Duparrow(INTBIG x, INTBIG y);
static BOOLEAN Duppage(INTBIG x, INTBIG y);
static BOOLEAN Dvscroll(INTBIG x, INTBIG y);
static INTBIG  Dwaitforaction(TDIALOG *dia, INTBIG *x, INTBIG *y, INTBIG *chr, INTBIG *special, UINTBIG *time, BOOLEAN *shifted);
static INTBIG  Dwhichitem(TDIALOG *dia, INTBIG x, INTBIG y);
static INTSML  Dgetnextcharacter(TDIALOG *dia, INTBIG *itemHit);
/****** miscellaneous ******/
#define EFONT	            kFontIDHelvetica	/* font in the editing window */
#define TFONT               kFontIDCourier		/* font in text editing */
#define SFONT	            kFontIDGeneva		/* font for status at the bottom of editing windows */
#define MFONT	            0					/* font in menus */
#define MSFONT	            kFontIDCourier		/* font in the messages windows */
#define DFONT	            0					/* font in dialogs */
#define DSFONT	            kFontIDCourier		/* small font in dialog scroll areas */
#define SBARWIDTH           15					/* width of scroll bar at right of edit window */
#define SBARHEIGHT          15					/* height of scroll bar at bottom of edit window */
#define	PALETTEWIDTH        80					/* width of palette */
#define FLOATINGHEADERSIZE  10					/* size of tool palette drag bar */
#define MAXSTATUSLINES       1					/* lines in status bar */
#define MAXLOCALSTRING     256					/* size of "gra_localstring" */

static FontInfo      gra_curfontinfo;			/* current font information */
static INTBIG        gra_winoffleft;			/* left offset of window contents to edge */
static INTBIG        gra_winoffright;			/* right offset of window contents to edge */
static INTBIG        gra_winofftop;				/* top offset of window contents to edge */
static INTBIG        gra_winoffbottom;			/* bottom offset of window contents to edge */
static INTBIG        gra_floatwinoffleft;		/* left offset for floating window contents to edge */
static INTBIG        gra_floatwinoffright;		/* right offset for floating window contents to edge */
static INTBIG        gra_floatwinofftop;		/* top offset for floating window contents to edge */
static INTBIG        gra_floatwinoffbottom;		/* bottom offset for floating window contents to edge */
static INTBIG        gra_screenleft;			/* left bound of any editor window screen */
static INTBIG        gra_screenright;			/* right bound of any editor window screen */
static INTBIG        gra_screentop;				/* top bound of any editor window screen */
static INTBIG        gra_screenbottom;			/* bottom bound of any editor window screen */
static GDHandle      gra_origgdevh;				/* the original graphics device */
static CHAR          gra_localstring[MAXLOCALSTRING];	/* local string */
static INTBIG        gra_logrecordindex = 0;	/* count for session flushing */
static INTBIG        gra_playbackmultiple = 0;	/* count for multi-key playback */
static BOOLEAN       gra_multiwindow;			/* true if using multiple windows */
static void         *gra_fileliststringarray = 0;
static INTBIG        gra_windowframeindex = 0;
static INTBIG        gra_numfaces = 0;
static CHAR        **gra_facelist;
static INTBIG       *gra_faceid;

/****** prototypes for local routines ******/
static BOOLEAN      gra_addfiletolist(CHAR *file);
static void         gra_adjusthtext(INTBIG);
static void         gra_adjustvtext(void);
static void         gra_applemenu(INTBIG);
static void         gra_backup(void);
static BOOLEAN      gra_buildwindow(WINDOWFRAME*, BOOLEAN);
static void         gra_centermessage(CHAR*, INTBIG, INTBIG, INTBIG);
static void         gra_fakemouseup(void);
static OSErr        gra_findprocess(OSType typeToFind, OSType creatorToFind, ProcessSerialNumberPtr processSN,
						ProcessInfoRecPtr infoRecToFill);
static void         gra_getdevices(void);
static Rect        *gra_geteditorwindowlocation(void);
       CGrafPtr     gra_getoffscreen(WINDOWPART *win);
static INTBIG       gra_gettraptype(INTBIG);
       CGrafPtr     gra_getwindow(WINDOWPART *win);
static pascal OSErr gra_handleodoc(AppleEvent*, AppleEvent*, long);
static pascal OSErr gra_handlequit(AppleEvent*, AppleEvent*, long);
static void         gra_hidemessageswindow(void);
static void         gra_initfilelist(void);
static void         gra_initializemenus(void);
static INTBIG       gra_initializetcl(void);
static void         gra_installvbl(void);
static CHAR        *gra_macintoshinitialization(void);
static INTBIG       gra_makebutton(INTBIG);
static BOOLEAN      gra_makeeditwindow(WINDOWFRAME*);
static CHAR        *gra_makefullname(CHAR*, INTBIG);
static CHAR        *gra_makefullnamefromFSS(FSSpec theFSS);
static MenuHandle   gra_makepdmenu(POPUPMENU*, INTBIG);
static CHAR        *gra_makepstring(CHAR *string);
static void         gra_mygrowwindow(WindowPtr, INTBIG, INTBIG, Rect*);
static void         gra_nativemenudoone(INTBIG low, INTBIG high);
static void         gra_nextevent(INTBIG, EventRecord*);
static INTBIG       gra_numtoolboxtraps(void);
static BOOLEAN      gra_onthiswindow(WindowPtr window, INTBIG left, INTBIG right, INTBIG top, INTBIG bottom);
static INTBIG       gra_pulldownindex(POPUPMENU*);
static void         gra_rebuildrowstart(WINDOWFRAME*);
static void         gra_recachepixmap(PixPatHandle);
static void         gra_redrawdisplay(WINDOWFRAME*);
static void         gra_reloadmap(void);
static INTBIG       gra_remakeeditwindow(WINDOWFRAME*);
static void         gra_removewindowextent(RECTAREA *r, RECTAREA *er);
static void         gra_savewindowsettings(void);
static pascal void  gra_scrollhproc(ControlHandle, short);
static pascal void  gra_scrollvproc(ControlHandle, short);
static void         gra_setcurrentwindowframe(void);
       void         gra_setrect(WINDOWFRAME*, INTBIG, INTBIG, INTBIG, INTBIG);
static void         gra_setuptextbuffer(INTBIG width, INTBIG height);
static void         gra_setview(WindowPtr);
static void         gra_setvscroll(void);
static BOOLEAN      gra_showmessageswindow(void);
static void         gra_showselect(void);
static BOOLEAN      gra_trapavailable(INTBIG);
static void         gra_waitforaction(INTBIG, EventRecord*);
static void         Dredrawdialogwindow(TDIALOG *dia);
static CHAR        *gra_systemfoldername(void);
       void         mac_settypecreator(CHAR *name, INTBIG type, INTBIG creator);
static void         gra_activatewindow(WindowPtr, BOOLEAN);
static void         gra_disposeoswindow(WindowPtr);
static void         gra_dragfloatingwindow(WindowPtr, Point);
static void         gra_drawosgraphics(WINDOWFRAME*);
static void         gra_drawpalettedragbar(WINDOWFRAME *wf);
static void         gra_frontnonfloatingwindow(WindowPtr *);
static WindowPtr    gra_lastfloatingwindow(void);
static void         gra_selectoswindow(WindowPtr);
static void         gra_showallfloaters(BOOLEAN);

/******************** INITIALIZATION ********************/

void main(void)
{
	short argc;
	CHAR *argv[4], *progname;

	/* initialize for the operating system */
	progname = gra_macintoshinitialization();
	argc = 1;
	argv[0] = progname;

	/* primary initialization of Electric */
	osprimaryosinit();

	/* secondary initialization of Electric */
	ossecondaryinit(argc, argv);

#if	LANGTCL
	/* only need TCL, initialize later to get proper path */
	if (gra_initializetcl() == TCL_ERROR)
		error(_("gra_initializetcl failed: %s\n"), myTCLInterp->result);
#endif

	/* now run Electric */
	for(;;)
	{
		tooltimeslice();
	}
}

/*
 * routine to establish the default display
 */
void graphicsoptions(CHAR *name, INTBIG *argc, CHAR **argv)
{
	/* set multiple window factor */
	if (*argc > 0 && argv[0][estrlen(argv[0])-1] == '1') gra_multiwindow = FALSE; else
		gra_multiwindow = TRUE;
	us_erasech = BACKSPACEKEY;
	us_killch = 025;
}
/*
 * routine to initialize the display device.  Creates status window if "messages" is true.
 */
BOOLEAN initgraphics(BOOLEAN messages)
{
	WINDOWFRAME *wf;
	CHAR fontname[100];
	Rect rstat, r;
	INTBIG err;
	short theFont;
	long response;
	Handle f;
	CTabHandle iclut;
	REGISTER INTBIG i;
	RgnHandle grayrgn;

	/* get double-click interval */
	gra_doubleclick = GetDblTime();
	gra_lastclick = 0;
	gra_lstcurx = -1;

	/* setup UPPs */
	gra_scrollvprocUPP = NewControlActionProc(gra_scrollvproc);
	gra_scrollhprocUPP = NewControlActionProc(gra_scrollhproc);
	gra_fileFilterProcUPP = NewFileFilterProc(gra_fileFilterProc);
	/* initialize menus */
	gra_initializemenus();

	/* get cursors */
	gra_wantttyCurs = GetCursor(wantttyCURSOR);
	gra_penCurs = GetCursor(penCURSOR);
	gra_nullCurs = GetCursor(nullCURSOR);
	gra_menuCurs = GetCursor(menulCURSOR);
	gra_handCurs = GetCursor(handCURSOR);
	gra_techCurs = GetCursor(techCURSOR);
	gra_ibeamCurs = GetCursor(iBeamCursor);
	gra_lrCurs = GetCursor(lrCURSOR);
	gra_udCurs = GetCursor(udCURSOR);
	us_normalcursor = NORMALCURSOR;

	/* setup globals that describe location of windows */
	gra_getdevices();

	/* determine font and size of text in messages window */
	f = GetResource('MPSR', 1004);
	if (f == 0)
	{
		/* no window resource: use defaults */
		gra_messagesfont = MSFONT;
		gra_messagesfontsize = 12;
	} else
	{
		HLock(f);

		/* get messages window extent */
		rstat.left = ((INTSML *)(*f))[0];
		rstat.right = ((INTSML *)(*f))[1];
		rstat.top = ((INTSML *)(*f))[2];
		rstat.bottom = ((INTSML *)(*f))[3];

		/* is the top of the messages window visible on this display? */
		r.left = rstat.left+gra_winoffleft;   r.right = rstat.right+gra_winoffright;
		r.top = rstat.top+gra_winofftop;     r.bottom = r.top + MENUSIZE;
		grayrgn = GetGrayRgn();
		if (RectInRgn(&r, grayrgn))
		{
			gra_messagesleft = rstat.left;   gra_messagesright = rstat.right;
			gra_messagestop = rstat.top;     gra_messagesbottom = rstat.bottom;
		}

		/* get font and size */
		gra_messagesfontsize = ((INTSML *)(*f))[8];
		(void)estrcpy(&fontname[1], &((CHAR *)(*f))[18]);
		fontname[0] = estrlen(&fontname[1]);
		GetFNum((UCHAR1 *)fontname, &theFont);
		gra_messagesfont = theFont;
		HUnlock(f);
	}

	/* create the scrolling messages window */
	gra_messageswindow = 0;
	gra_TEH = 0;
	if (messages)
	{
		if (gra_showmessageswindow()) error(_("Cannot create messages window"));
	}

	/* initialize the mouse */
	if (gra_nullCurs != 0L) SetCursor(&(**gra_nullCurs)); else
		SetCursor(&qd.arrow);
	us_cursorstate = NULLCURSOR;
	gra_inputstate = NOEVENT;
//	gra_eventqueuehead = gra_eventqueuetail = gra_eventqueue;

	/* create the CLUT for identity mapping on color displays */
	iclut = (CTabHandle)NewHandle(2056);
	if (iclut == 0) error(_("Cannot allocate identity color lookup table"));
	(*iclut)->ctSeed = GetCTSeed();
	(*iclut)->ctFlags = 0;
	(*iclut)->ctSize = 255;
	for(i=0; i<256; i++)
	{
		(*iclut)->ctTable[i].value = i;
		(*iclut)->ctTable[i].rgb.red = i<<8;
		(*iclut)->ctTable[i].rgb.green = i<<8;
		(*iclut)->ctTable[i].rgb.blue = i<<8;
	}

	/* setup to handle input files via apple events */
	err = Gestalt(gestaltAppleEventsAttr, &response);
	if (err == 0 && ((response >> gestaltAppleEventsPresent) & 1) != 0)
	{
		/* apple events method */
		err = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
			NewAEEventHandlerProc(gra_handleodoc), 0, 0);
		err = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
			NewAEEventHandlerProc(gra_handlequit), 0, 0);
	}

	/* initialize vertical retrace manager to check for interrupts */
	gra_installvbl();

	/* create the first window frame */
	if (!gra_multiwindow)
	{
		wf = (WINDOWFRAME *)emalloc((sizeof (WINDOWFRAME)), us_tool->cluster);
		if (wf == 0) error(_("Cannot create Macintosh window info structure"));
		wf->numvar = 0;
		wf->windindex = gra_windowframeindex++;
		el_firstwindowframe = el_curwindowframe = wf;
		el_firstwindowframe->nextwindowframe = NOWINDOWFRAME;

		/* load an editor window into this frame */
		if (gra_buildwindow(el_curwindowframe, FALSE))
			error(_("Cannot create Macintosh window info structure"));
	}

	/* catch interrupts */
	(void)signal(SIGINT, (SIGNALCAST)gra_onint);

	return(FALSE);
}

/*
 * Routine to establish the library directories from the environment.
 */
void setupenvironment(void)
{
	REGISTER void *infstr;

	/* initialize system directories for library files */
	infstr = initinfstr();
	addstringtoinfstr(infstr, gra_systemfoldername());
	addstringtoinfstr(infstr, LIBDIR);
	(void)allocstring(&el_libdir, returninfstr(infstr), db_cluster);

	/* set machine name */
	nextchangequiet();
	(void)setval((INTBIG)us_tool, VTOOL, x_("USER_machine"),
		(INTBIG)x_("Macintosh"), VSTRING|VDONTSAVE);
}

void setlibdir(CHAR *libdir)
{
	CHAR *pp;
	REGISTER void *infstr;

	/* make sure UNIX '/' isn't in use */
	for(pp = libdir; *pp != 0; pp++)
		if (*pp == '/') *pp = DIRSEP;

	infstr = initinfstr();
	if (libdir[0] == ':')
	{
		pp = gra_systemfoldername();
	} else
	{
		pp = currentdirectory();
	}
	addstringtoinfstr(infstr, pp);
	addstringtoinfstr(infstr, libdir);
	if (libdir[estrlen(libdir)-1] != DIRSEP) addtoinfstr(infstr, DIRSEP);
	(void)reallocstring(&el_libdir, returninfstr(infstr), db_cluster);
}

/*
 * Routine to examine the display devices available and to setup globals that describe
 * the editing windows and messages window extents.  On exit, the globals "gra_screenleft",
 * "gra_screenright", "gra_screentop", and "gra_screenbottom" will describe the area
 * for the editing windows and the variables "gra_messagesleft", "gra_messagesright",
 * "gra_messagestop", and "gra_messagesbottom" will describe the messages window.
 */
void gra_getdevices(void)
{
	GDHandle gdevh, bestcgdevh, bestbwgdevh, curgdevh;
	INTBIG totaldevices, iscolor, anycolor;
	INTBIG bestcpixels, bestbwpixels, pixels, ys;

	/* obtain the current device */
	gra_origgdevh = GetGDevice();

	/* find the devices */
	bestcpixels = bestbwpixels = 0L;
	totaldevices = anycolor = 0;
	for(gdevh = GetDeviceList(); gdevh != 0L; gdevh = GetNextDevice(gdevh))
	{
		totaldevices++;

		/* determine size of this display */
		pixels = (*gdevh)->gdRect.right - (*gdevh)->gdRect.left;
		ys = (*gdevh)->gdRect.bottom - (*gdevh)->gdRect.top;
		pixels *= ys;

		/* The low bit is set if the display is color */
		iscolor = 1;
		if (((*gdevh)->gdFlags&1) == 0) iscolor = 0;

		/* color displays must be at least 8 bits deep */
		if ((*(*gdevh)->gdPMap)->pixelSize < 8) iscolor = 0;

		/* accumulate the best of each display type */
		if (iscolor != 0)
		{
			if (pixels > bestcpixels || (pixels == bestcpixels && gdevh == gra_origgdevh))
			{
				bestcgdevh = gdevh;
				bestcpixels = pixels;
			}
		} else
		{
			if (pixels > bestbwpixels || (pixels == bestbwpixels && gdevh == gra_origgdevh))
			{
				bestbwgdevh = gdevh;
				bestbwpixels = pixels;
			}
		}
	}

	/* if there is a color device, choose it */
	if (bestcpixels != 0) curgdevh = bestcgdevh; else
		if (bestbwpixels != 0) curgdevh = bestbwgdevh; else
	{
		ParamText((UCHAR1 *)gra_makepstring(_("For some reason, Electric cannot find any displays on which to run.")), x_("\p"), x_("\p"), x_("\p"));
		StopAlert(errorALERT, 0L);
		exitprogram();
	}

	/* set the extent of the editing windows */
	gra_screenleft   = (*curgdevh)->gdRect.left;
	gra_screenright  = (*curgdevh)->gdRect.right;
	gra_screentop    = (*curgdevh)->gdRect.top;
	gra_screenbottom = (*curgdevh)->gdRect.bottom;

	/* set the extent of the messages window */
	gra_messagesleft   = gra_screenleft + PALETTEWIDTH + 1;
	gra_messagesright  = gra_screenright - PALETTEWIDTH - 1;
	gra_messagestop    = gra_screentop + MENUSIZE*3 + 2 +
		(gra_screenbottom-(gra_screentop+MENUSIZE*2)) * 3 / 5;
	gra_messagesbottom = gra_screenbottom;

	/* if multiple displays exist, choose another for the messages window */
	if (totaldevices > 1)
	{
		/* look for another screen to be used for the messages display */
		for(gdevh = GetDeviceList(); gdevh != 0L; gdevh = GetNextDevice(gdevh))
			if (gdevh != curgdevh)
		{
			INTBIG i, siz;
			gra_messagesleft = (*gdevh)->gdRect.left;
			gra_messagesright = (*gdevh)->gdRect.right;
			gra_messagestop = (*gdevh)->gdRect.top;
			gra_messagesbottom = (*gdevh)->gdRect.bottom;

			i = (gra_messagesleft + gra_messagesright) / 2;
			siz = (gra_messagesright - gra_messagesleft) * 2 / 5;
			gra_messagesleft = i - siz;     gra_messagesright = i + siz;
			i = (gra_messagestop + gra_messagesbottom) / 2;
			siz = (gra_messagesbottom - gra_messagestop) / 4;
			gra_messagestop = i - siz;      gra_messagesbottom = i + siz;
			return;
		}
	}
}

CHAR *gra_macintoshinitialization(void)
{
	long ppcresponse;
	SysEnvRec ser;
	ProcessSerialNumber psn;
	ProcessInfoRec pinfo;
	INTBIG sysversion;
	CHAR processname[50];
	static CHAR appname[50];
#if defined(__MWERKS__) && __MC68020__
	long response;
#endif

	MaxApplZone();
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();

	/* must have system 7 */
	SysEnvirons(2, &ser);
	sysversion = (ser.systemVersion >> 8) & 0xFF;
	if (sysversion < 7)
		error(_("The Macintosh must be running system 7 or later"));
	switch (sysversion)
	{
		case 7:
			gra_winoffleft = 1;   gra_winoffright = -2;
			gra_winoffbottom = -2;  gra_winofftop = MENUSIZE;
			break;
		default:		/* works for system 9 */
			gra_winoffleft = 6;   gra_winoffright = -7;
			gra_winoffbottom = -7;  gra_winofftop = MENUSIZE+3;
			break;
	}
	gra_floatwinoffleft = 1;   gra_floatwinoffright = 1;
	gra_floatwinofftop = 1;    gra_floatwinoffbottom = 1;

	/* see if this is a powerPC */
	Gestalt(gestaltPPCToolboxAttr, &ppcresponse);

	/* check for 020 if the code was compiled for it */
#if defined(__MWERKS__) && __MC68020__
	Gestalt(gestaltProcessorType, &response);
	if (ppcresponse == 0 &&
		(response == gestalt68000 || response == gestalt68010))
			error(_("The Macintosh must have a 68020 or better processor"));
#endif

	/* check for floating point unit if the code was compiled for it */
#if defined(__MWERKS__) && __MC68881__
	Gestalt(gestaltFPUType, &response);
	if (ppcresponse == 0 && response == gestaltNoFPU)
		error(_("The Macintosh must have a Floating Point Processor"));
#endif
	/* determine "arguments" to the program */
	pinfo.processInfoLength = sizeof (ProcessInfoRec);
	pinfo.processName = (UCHAR1 *)processname;
	pinfo.processAppSpec = 0;
	GetCurrentProcess(&psn);
	GetProcessInformation(&psn, &pinfo);
	estrncpy(appname, (CHAR *)&pinfo.processName[1], pinfo.processName[0]);
	appname[pinfo.processName[0]] = 0;
	return(appname);
}

/*
 * routine to determine whether a trap exists in the Mac
 */
BOOLEAN gra_trapavailable(INTBIG theTrap)
{
	INTBIG ttype;
	ttype = gra_gettraptype(theTrap);
	if (ttype == ToolTrap)
	{
		theTrap &= 0x7FF;
		if (theTrap >= gra_numtoolboxtraps()) return(FALSE);
	}
	return(NGetTrapAddress(theTrap, ttype) != NGetTrapAddress(0, ToolTrap));
}

/*
 * support routine to determine what kind of trap this is
 */
INTBIG gra_gettraptype(INTBIG theTrap)
{
	if ((theTrap&0x800) != 0) return(ToolTrap);
	return(OSTrap);
}

/*
 * support routine to determine how many Mac Toolbox traps are available
 */
INTBIG gra_numtoolboxtraps(void)
{
	/* 0xA86E is "InitGraf" */
	if (NGetTrapAddress(0xA86E, 1) == NGetTrapAddress(0xAA6E, 1)) return(0x200);
	return(0x400);
}

#if	LANGTCL

INTBIG gra_initializetcl(void)
{
	INTBIG err;
	CHAR *newArgv[2];

	/* set the program name/path */
	newArgv[0] = x_("Electric");
	newArgv[1] = NULL;
	(void)Tcl_FindExecutable(newArgv[0]);

	myTCLInterp = Tcl_CreateInterp();
	if (myTCLInterp == 0) error(_("from Tcl_CreateInterp"));

	/* tell Electric the TCL interpreter handle */
	el_tclinterpreter(myTCLInterp);

	/* Make command-line arguments available in the Tcl variables "argc" and "argv" */
	Tcl_SetVar(myTCLInterp, x_("argv"), x_(""), TCL_GLOBAL_ONLY);
	Tcl_SetVar(myTCLInterp, x_("argc"), x_("0"), TCL_GLOBAL_ONLY);
	Tcl_SetVar(myTCLInterp, x_("argv0"), x_("electric"), TCL_GLOBAL_ONLY);

	/* Set the "tcl_interactive" variable */
	Tcl_SetVar(myTCLInterp, x_("tcl_interactive"), x_("1"), TCL_GLOBAL_ONLY);

	/* initialize the interpreter */
	err = Tcl_Init(myTCLInterp);
	if (err != TCL_OK) error(_("(from Tcl_Init) %s"), myTCLInterp->result);

	return(err);
}

#endif

/******************** TERMINATION ********************/

void termgraphics(void)
{
	REGISTER INTBIG i;

	gra_termgraph();
	while (el_firstwindowframe != NOWINDOWFRAME)
		gra_disposeoswindow((WindowPtr)el_firstwindowframe->realwindow);
	if (gra_messageswindow != 0)
		gra_disposeoswindow(gra_messageswindow);
	for(i=0; i<gra_numfaces; i++) efree((CHAR *)gra_facelist[i]);
	if (gra_numfaces > 0)
	{
		efree((CHAR *)gra_facelist);
		efree((CHAR *)gra_faceid);
	}

	if (gra_pulldownmenucount != 0)
	{
		efree((CHAR *)gra_pulldownmenus);
		for(i=0; i<gra_pulldownmenucount; i++) efree(gra_pulldowns[i]);
		efree((CHAR *)gra_pulldowns);
	}
}

/*
 * routine to handle a "quit" Apple Event
 */
pascal OSErr gra_handlequit(AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefCon)
{
	if (us_preventloss(NOLIBRARY, 0, TRUE)) return(noErr);
	bringdown();
	return(noErr);
}

/*
 * routine to quit the program.  It unregisters the name first and then exits.
 */
void exitprogram(void)
{
	ExitToShell();
}

/******************** WINDOW CONTROL ********************/

/*
 * Routine to create a new window frame (floating if "floating" nonzero).
 */
WINDOWFRAME *newwindowframe(BOOLEAN floating, RECTAREA *r)
{
	WINDOWFRAME *wf, *oldlisthead;

	if (!gra_multiwindow) return(NOWINDOWFRAME);

	/* allocate one */
	wf = (WINDOWFRAME *)emalloc((sizeof (WINDOWFRAME)), us_tool->cluster);
	if (wf == 0) return(NOWINDOWFRAME);
	wf->numvar = 0;
	wf->windindex = gra_windowframeindex++;

	/* insert window-frame in linked list */
	oldlisthead = el_firstwindowframe;
	wf->nextwindowframe = el_firstwindowframe;
	el_firstwindowframe = wf;

	/* load an editor window into this frame */
	if (gra_buildwindow(wf, floating))
	{
		efree((CHAR *)wf);
		el_firstwindowframe = oldlisthead;
		return(NOWINDOWFRAME);
	}

	/* remember that this is the current window frame */
	if (!floating) el_curwindowframe = wf;

	return(wf);
}

void killwindowframe(WINDOWFRAME *wf)
{
	if (gra_multiwindow)
	{
		/* kill the actual window, that will remove the frame, too */
		gra_disposeoswindow((WindowPtr)wf->realwindow);
	}
}

/*
 * Routine to return the current window frame.
 */
WINDOWFRAME *getwindowframe(BOOLEAN canfloat)
{
	return(el_curwindowframe);
}

/*
 * routine to return size of window "win" in "wid" and "hei"
 */
void getwindowframesize(WINDOWFRAME *wf, INTBIG *wid, INTBIG *hei)
{
	*wid = wf->swid;
	*hei = wf->shei;
}

/*
 * Routine to get the extent of the messages window.
 */
void getmessagesframeinfo(INTBIG *top, INTBIG *left, INTBIG *bottom, INTBIG *right)
{
	Rect r;

	if (gra_messageswindow == 0)
	{
		*left = gra_messagesleft;  *right = gra_messagesright;
		*top = gra_messagestop;    *bottom = gra_messagesbottom;
	} else
	{
		r = (*((WindowPeek)gra_messageswindow)->strucRgn)->rgnBBox;
		*left = r.left+gra_winoffleft;  *right = r.right+gra_winoffright;
		*top = r.top+gra_winofftop;     *bottom = r.bottom+gra_winoffbottom;
	}
}

/*
 * Routine to set the size and position of the messages window.
 */
void setmessagesframeinfo(INTBIG top, INTBIG left, INTBIG bottom, INTBIG right)
{
	MoveWindow(gra_messageswindow, left, top, 0);
	SizeWindow(gra_messageswindow, right-left, bottom-top, 1);
}

/*
 * Routine to return the current Macintosh window associated with Electric window "win".
 * This is only called from "ioplotmac.c"
 */
CGrafPtr gra_getwindow(WINDOWPART *win)
{
	return(win->frame->realwindow);
}

/*
 * Routine to return the offscreen Macintosh buffer associated with Electric window "win".
 * This is only called from "ioplotmac.c"
 */
CGrafPtr gra_getoffscreen(WINDOWPART *win)
{
	return(win->frame->window);
}

void sizewindowframe(WINDOWFRAME *wf, INTBIG wid, INTBIG hei)
{
	Rect         fr;

	fr = (*((WindowPeek)wf->realwindow)->strucRgn)->rgnBBox;

	/* determine new window size */
	if (wf != NOWINDOWFRAME && wf->floating) hei += FLOATINGHEADERSIZE;
	if (wid == fr.right-fr.left && hei == fr.bottom-fr.top) return;

	/* resize the window */
	SizeWindow((WindowRef)wf->realwindow, wid, hei, 1);

	/* rebuild the offscreen windows */
	if (gra_remakeeditwindow(wf) > 0)
	{
		SizeWindow((WindowRef)wf->realwindow, fr.right-fr.left, fr.bottom-fr.top, 1);
		return;
	}
	gra_reloadmap();

	SetPort((WindowPtr)wf->realwindow);
	EraseRect(&wf->realwindow->portRect);
	gra_drawosgraphics(wf);
}

void movewindowframe(WINDOWFRAME *wf, INTBIG left, INTBIG top)
{
	INTBIG        oleft, otop;
	Rect         fr;

	fr = (*((WindowPeek)wf->realwindow)->strucRgn)->rgnBBox;

	/* determine new window location */
	top += MENUSIZE+1;
	if (left == fr.left && top == fr.top) return;
	oleft = fr.left;   otop = fr.top;
	MoveWindow((WindowRef)wf->realwindow, left, top, 0);
}

/*
 * routine to grow window "w" to the new size of "wid" by "hei".  If the grow fails,
 * reset the window to its former size in "formerrect".
 */
void gra_mygrowwindow(WindowPtr w, INTBIG wid, INTBIG hei, Rect *formerrect)
{
	REGISTER WINDOWFRAME *wf;
	Rect r;
	REGISTER INTBIG ret, top;

	if (w == gra_messageswindow && gra_messageswindow != 0)
	{
		SetPort(w);
		InvalRect(&w->portRect);

		gra_setview(w);
		HidePen();
		top = w->portRect.top;
		MoveControl(gra_vScroll, w->portRect.right-SBARWIDTH, top);
		SizeControl(gra_vScroll, SBARWIDTH+1, w->portRect.bottom-top-(SBARHEIGHT-2));
		MoveControl(gra_hScroll, w->portRect.left-1, w->portRect.bottom-SBARHEIGHT);
		SizeControl(gra_hScroll, w->portRect.right-w->portRect.left-(SBARWIDTH-2), SBARHEIGHT+1);
		ShowPen();

		gra_setvscroll();
		gra_adjustvtext();

		r = (*((WindowPeek)w)->strucRgn)->rgnBBox;
		gra_messagesleft = r.left+gra_winoffleft;  gra_messagesright = r.right+gra_winoffright;
		gra_messagestop = r.top+gra_winofftop;     gra_messagesbottom = r.bottom+gra_winoffbottom;
		/* remember window settings */
		gra_savewindowsettings();
		return;
	}

	/* find the window */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (w == (WindowPtr)wf->realwindow) break;
	if (wf == NOWINDOWFRAME) return;

	/* rebuild the window pointers */
	ret = gra_remakeeditwindow(wf);

	/* no change necessary */
	if (ret < 0) return;

	/* change failed: move it back */
	if (ret > 0)
	{
		MoveWindow((WindowPtr)wf->realwindow, formerrect->left, formerrect->top, 0);
		SizeWindow((WindowPtr)wf->realwindow, formerrect->right-formerrect->left,
			formerrect->bottom-formerrect->top, 1);
		return;
	}

	/* change succeeded, redraw the window */
	gra_reloadmap();
	SetPort((WindowPtr)wf->realwindow);
	EraseRect(&wf->realwindow->portRect);
	RectRgn(wf->realwindow->clipRgn, &wf->realwindow->portRect);
	gra_drawosgraphics(wf);
	gra_redrawdisplay(wf);
}

/*
 * Routine to close the messages window if it is in front.  Returns true if the
 * window was closed.
 */
BOOLEAN closefrontmostmessages(void)
{
	if (gra_messagesinfront)
	{
		gra_hidemessageswindow();
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to bring window "win" to the front.
 */
void bringwindowtofront(WINDOWFRAME *wf)
{
	gra_selectoswindow((WindowPtr)wf->realwindow);
}

/*
 * Routine to organize the windows according to "how" (0: tile horizontally,
 * 1: tile vertically, 2: cascade).
 */
void adjustwindowframe(INTBIG how)
{
	RECTAREA r, wr;
	Rect fr;
	REGISTER INTBIG children, child, sizex, sizey, left, right, top, bottom;
	REGISTER WINDOWFRAME *wf, *compmenu;
	GDHandle gdevh;

	/* loop through the screens */
	for(gdevh = GetDeviceList(); gdevh != 0L; gdevh = GetNextDevice(gdevh))
	{
		left = (*gdevh)->gdRect.left;
		right = (*gdevh)->gdRect.right;
		top = (*gdevh)->gdRect.top;
		bottom = (*gdevh)->gdRect.bottom;

		/* figure out how many windows need to be rearranged */
		children = 0;
		compmenu = 0;
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (!gra_onthiswindow((WindowPtr)wf->realwindow, left, right, top, bottom)) continue;
			if (!wf->floating) children++; else
				compmenu = wf;
		}
		if (children <= 0) continue;

		/* determine area for windows */
		r.left = left;          r.right = right;
		r.top = top+MENUSIZE;   r.bottom = bottom;

		if (compmenu != NULL)
		{
			/* remove component menu from area of tiling */
			fr = (*((WindowPeek)compmenu->realwindow)->strucRgn)->rgnBBox;
			wr.left = fr.left;   wr.right = fr.right;
			wr.top = fr.top;     wr.bottom = fr.bottom;
			gra_removewindowextent(&r, &wr);
		}
		if (gra_messageswindow != 0)
		{
			if (gra_onthiswindow(gra_messageswindow, left, right, top, bottom))
			{
				/* remove messages menu from area of tiling */
				fr = (*((WindowPeek)gra_messageswindow)->strucRgn)->rgnBBox;
				wr.left = fr.left;   wr.right = fr.right;
				wr.top = fr.top;     wr.bottom = fr.bottom;
				gra_removewindowextent(&r, &wr);
			}
		}
		/* rearrange the windows */
		child = 0;
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (!gra_onthiswindow((WindowPtr)wf->realwindow, left, right, top, bottom)) continue;
			if (wf->floating) continue;
			switch (how)
			{
				case 0:		/* tile horizontally */
					wr.left = r.left;
					wr.right = r.right;
					sizey = (r.bottom - r.top) / children;
					wr.top = r.top + child*sizey;
					wr.bottom = wr.top + sizey;
					break;
				case 1:		/* tile vertically */
					wr.top = r.top;
					wr.bottom = r.bottom;
					sizex = (r.right - r.left) / children;
					wr.left = r.left + child*sizex;
					wr.right = wr.left + sizex;
					break;
				case 2:		/* cascade */
					sizex = (r.right - r.left) / children / 2;
					sizey = (r.bottom - r.top) / children / 2;
					wr.left = r.left + child*sizex;
					wr.right = wr.left + (r.right-r.left)/2;
					wr.top = r.top + child*sizey;
					wr.bottom = wr.top + (r.bottom-r.top)/2;
					break;
			}
			wr.top += gra_winofftop;
			wr.left += gra_winoffleft;
			wr.right += gra_winoffright;
			wr.bottom += gra_winoffbottom;
			MoveWindow((WindowPtr)wf->realwindow, wr.left, wr.top, 0);
			SizeWindow((WindowPtr)wf->realwindow, wr.right-wr.left,
				wr.bottom-wr.top, 1);
			fr = (*((WindowPeek)wf->realwindow)->strucRgn)->rgnBBox;
			gra_mygrowwindow((WindowPtr)wf->realwindow, wr.right-wr.left,
				wr.bottom-wr.top, &fr);
			child++;
		}
	}
}

/*
 * Helper routine to determine whether window "window" is on the display bounded by "left",
 * "right", "top" and "bottom".  Returns true if so.
 */
BOOLEAN gra_onthiswindow(WindowPtr window, INTBIG left, INTBIG right, INTBIG top, INTBIG bottom)
{
	Rect fr;
	REGISTER INTBIG pixels, onpixels;

	fr = (*((WindowPeek)window)->strucRgn)->rgnBBox;
	pixels = (fr.right-fr.left) * (fr.bottom-fr.top);
	if (fr.left > right || fr.right < left ||
		fr.top > bottom || fr.bottom < top) return(FALSE);
	if (fr.left < left) fr.left = left;
	if (fr.right > right) fr.right = right;
	if (fr.top < top) fr.top = top;
	if (fr.bottom > bottom) fr.bottom = bottom;
	onpixels = (fr.right-fr.left) * (fr.bottom-fr.top);
	if (onpixels * 2 >= pixels) return(TRUE);
	return(FALSE);
}

/*
 * Helper routine to remove the location of window "wnd" from the rectangle "r".
 */
void gra_removewindowextent(RECTAREA *r, RECTAREA *er)
{
	if (er->right-er->left > er->bottom-er->top)
	{
		/* horizontal occluding window */
		if (er->left >= r->right || er->right <= r->left) return;
		if (er->bottom - r->top < r->bottom - er->top)
		{
			/* occluding window on top */
			r->top = er->bottom;
		} else
		{
			/* occluding window on bottom */
			r->bottom = er->top;
		}
	} else
	{
		/* vertical occluding window */
		if (er->top >= r->bottom || er->bottom <= r->top) return;
		if (er->right - r->left < r->right - er->left)
		{
			/* occluding window on left */
			r->left = er->right;
		} else
		{
			/* occluding window on right */
			r->right = er->left;
		}
	}
}

/*
 * routine to return the important pieces of information in placing the floating
 * palette with the fixed menu.  The maximum screen size is placed in "wid" and
 * "hei", and the amount of space that is being left for this palette on the
 * side of the screen is placed in "palettewidth".
 */
void getpaletteparameters(INTBIG *wid, INTBIG *hei, INTBIG *palettewidth)
{
	*hei = gra_screenbottom - (gra_screentop+MENUSIZE) - FLOATINGHEADERSIZE;
	*wid = gra_screenright - gra_screenleft;
	*palettewidth = PALETTEWIDTH;
}

/*
 * Routine called when the component menu has moved to a different location
 * on the screen (left/right/top/bottom).  Resets local state of the position.
 */
void resetpaletteparameters(void)
{
}

Rect *gra_geteditorwindowlocation(void)
{
	static Rect redit;
	static INTBIG offset = 0;

	if (gra_multiwindow)
	{
		redit.top = offset + gra_screentop + MENUSIZE;
		redit.bottom = redit.top + (gra_screenbottom-(gra_screentop+MENUSIZE)) * 3 / 4;
		redit.left = offset + gra_screenleft + PALETTEWIDTH + 1;
		redit.right = redit.left + (gra_screenright-gra_screenleft-PALETTEWIDTH-1) * 4 / 5;

		/* is the editing window visible on this display? */
		if (redit.bottom > gra_screenbottom || redit.right > gra_screenright)
		{
			offset = 0;
			redit.top = gra_screentop + MENUSIZE;
			redit.bottom = redit.top + (gra_screenbottom-(gra_screentop+MENUSIZE)) * 3 / 4;
			redit.left = gra_screenleft + PALETTEWIDTH + 1;
			redit.right = redit.left + (gra_screenright-gra_screenleft-PALETTEWIDTH-1) * 4 / 5;
		}
		offset += 30;
	} else
	{
		redit.top = gra_screentop + MENUSIZE;
		redit.bottom = gra_screenbottom;
		redit.left = gra_screenleft;
		redit.right = gra_screenright;
	}
	redit.top += MENUSIZE;
	return(&redit);
}

/*
 * routine to build a new window frame
 */
BOOLEAN gra_buildwindow(WINDOWFRAME *wf, BOOLEAN floating)
{
	Rect redit;
	WindowPtr frontmost, wBehind;
	static BOOLEAN first = TRUE;
	WStateData *wst;
	INTBIG cy, ty, left, right, top, bottom;
	CHAR line[200];
	Handle name, company, special;

	wf->floating = floating;
	if (!floating)
	{
		/* get editor window location */
		redit = *gra_geteditorwindowlocation();

		/* deactivate any other nonfloating window that is current */
		gra_frontnonfloatingwindow(&frontmost);
		if (frontmost != 0) gra_activatewindow(frontmost, FALSE);

		/* figure out which window to put this behind */
		wBehind = gra_lastfloatingwindow();
		if (wBehind == 0) wBehind = (WindowPtr)-1;

		/* create the editing window */
		wf->realwindow = (CGrafPtr)NewCWindow(0L, &redit, x_("\p"), 0, zoomDocProc, wBehind, 1, 0L);
	} else
	{
		/* default palette window location */
		redit.top = gra_screentop + MENUSIZE + 2;
		redit.bottom = (gra_screentop+gra_screenbottom)/2;
		redit.left = gra_screenleft + 1;
		redit.right = redit.left + PALETTEWIDTH/2;

		/* create the editing window */
		wf->realwindow = (CGrafPtr)NewCWindow(0L, &redit, x_("\p"), 0, plainDBox, (WindowRef)(-1L),
			1, 0L);
	}

	if (wf->realwindow == 0) return(TRUE);
	ShowHide((WindowRef)wf->realwindow, 1);
	gra_frontnonfloatingwindow(&frontmost);
	gra_activatewindow((WindowRef)wf->realwindow, FALSE);
	gra_activatewindow(frontmost, TRUE);
	SetPort((WindowPtr)wf->realwindow);
	EraseRect(&wf->realwindow->portRect);
	gra_drawosgraphics(wf);

	if (!floating)
	{
		if (first)
		{
			/* get user name/company/special message */
			name = GetResource('STR ', NAMERSRC);
			company = GetResource('STR ', COMPANYRSRC);
			special = GetResource('STR ', SPECIALRSRC);
			(void)estrcpy(line, _(" Version "));
			(void)estrcat(line, el_version);
			line[0] = estrlen(&line[1]);

			/* display the welcome message */
			TextFont(0);
			TextSize(12);
			left = wf->realwindow->portRect.left;
			right = wf->realwindow->portRect.right;
			top = wf->realwindow->portRect.top;
			bottom = wf->realwindow->portRect.bottom;
			cy = (top+bottom) / 2;
			ty = (bottom-top) / 5;
			gra_centermessage(gra_makepstring(_("Electric Design System")), left, right, ty);
			gra_centermessage(line, left, right, ty+15);
			gra_centermessage(gra_makepstring(_("Licensed from Static Free Software")),
				left, right, ty+40);
			if (name == 0)
				gra_centermessage(gra_makepstring(_("***** UNKNOWN USER *****")), left, right, cy); else
			{
				HLock(name);
				gra_centermessage(*name, left, right, cy);
				HUnlock(name);
			}
			if (company == 0)
				gra_centermessage(gra_makepstring(_("***** UNKNOWN LOCATION *****")), left, right, cy+20); else
			{
				HLock(company);
				gra_centermessage(*company, left, right, cy+20);
				HUnlock(company);
			}
			if (special != 0)
			{
				HLock(special);
				gra_centermessage(*special, left, right, cy+40);
				HUnlock(special);
			}

			gra_centermessage(gra_makepstring(_("Please wait for loading...")), left, right, bottom-30);
		}
		wst = (WStateData *) *(((WindowPeek)wf->realwindow)->dataHandle);
		wst->stdState.top = gra_screentop + MENUSIZE*2;
		wst->stdState.bottom = gra_screenbottom;
		wst->stdState.left = gra_screenleft + PALETTEWIDTH;
		wst->stdState.right = gra_screenright;
	}

	/* build the offscreen buffer for the window */
	if (gra_makeeditwindow(wf))
	{
		DisposeWindow((WindowPtr)wf->realwindow);
		return(TRUE);
	}

	/* load any map the first time to establish the map segment length */
	el_maplength = 256;
	SetGWorld(wf->realwindow, gra_origgdevh);
	if (!floating && first) us_getcolormap(el_curtech, COLORSDEFAULT, FALSE); else
		gra_reloadmap();
	first = FALSE;

	/* want the editing window to be the primary one */
	SetPort((WindowPtr)wf->realwindow);

	return(FALSE);
}

void gra_centermessage(CHAR *msg, INTBIG left, INTBIG right, INTBIG y)
{
	INTBIG wid;

	wid = StringWidth((UCHAR1 *)msg);
	MoveTo((left+right-wid)/2, y);
	DrawString((UCHAR1 *)msg);
}

/*
 * Routine to redraw editing window "wf" due to a drag or grow
 */
void gra_redrawdisplay(WINDOWFRAME *wf)
{
	us_beginchanges();
	if (wf == NOWINDOWFRAME || wf->floating) us_drawmenu(0, wf); else
		us_drawmenu(-1, wf);
	us_redostatus(wf);
	us_endchanges(NOWINDOWPART);
	us_state |= HIGHLIGHTSET;
	us_showallhighlight();
}

/*
 * Routine to allocate the offscreen buffer that corresponds with the window
 * "wf->realwindow".  The buffer is 8-bits deep.
 * Returns true if memory cannot be allocated.
 */
BOOLEAN gra_makeeditwindow(WINDOWFRAME *wf)
{
	INTBIG height, bytes, i;
	UCHAR1 *addr;
	Rect r;

	r = wf->realwindow->portRect;
	if (!wf->floating) r.bottom -= SBARHEIGHT; else
		r.bottom -= FLOATINGHEADERSIZE;
	if (NewGWorld(&wf->window, 8, &r, 0L, 0L, 0) != noErr)
	{
		ParamText((UCHAR1 *)gra_makepstring(_("Cannot create the offscreen window")), x_("\p"), x_("\p"), x_("\p"));
		Alert(errorALERT, 0L);
		return(TRUE);
	}

	SetGWorld(wf->window, 0L);
	(void)LockPixels(wf->window->portPixMap);
	PenMode(patCopy);
	BackColor(0);
	EraseRect(&wf->window->portRect);
	UnlockPixels(wf->window->portPixMap);
	height = r.bottom - r.top;
	wf->rowstart = (UCHAR1 **)emalloc(height * SIZEOFINTBIG, us_tool->cluster);
	if (wf->rowstart == 0)
	{
		DisposeGWorld(wf->window);
		SetGWorld(wf->realwindow, gra_origgdevh);
		ParamText((UCHAR1 *)gra_makepstring(_("Cannot create the offscreen pointers")), x_("\p"), x_("\p"), x_("\p"));
		Alert(errorALERT, 0L);
		return(TRUE);
	}

	bytes = (*wf->window->portPixMap)->rowBytes & 0x7FFF;
	addr = (UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap);
	for(i=0; i<height; i++)
	{
		wf->rowstart[i] = addr;
		addr += bytes;
	}
	SetGWorld(wf->realwindow, gra_origgdevh);

	wf->swid = wf->window->portRect.right - wf->window->portRect.left;
	wf->shei = wf->window->portRect.bottom - wf->window->portRect.top;
	wf->revy = wf->shei - 1;
	wf->offscreendirty = FALSE;
	return(FALSE);
}

/*
 * Routine to rebuild the offscreen buffer when its size or depth has changed.
 * Returns -1 if no change is necessary, 0 if the change was successful, and
 * 1 if the change failed (memory error).
 */
INTBIG gra_remakeeditwindow(WINDOWFRAME *wf)
{
	INTBIG bytes, i, height;
	short res;
	UCHAR1 *addr, **newrowstart;
	Rect r;

	/* quit now if there was no change */
	r = wf->realwindow->portRect;
	if (!wf->floating) r.bottom -= SBARHEIGHT; else
		r.bottom -= FLOATINGHEADERSIZE;
	height = r.bottom - r.top;
	if (wf->swid == r.right - r.left && wf->shei == height) return(-1);

	/* reallocate rowstart array */
	newrowstart = (UCHAR1 **)emalloc(height * SIZEOFINTBIG, us_tool->cluster);
	if (newrowstart == 0)
	{
		ParamText((UCHAR1 *)gra_makepstring(_("Not enough memory to modify the window")), x_("\p"), x_("\p"), x_("\p"));
		Alert(errorALERT, 0L);
		return(1);
	}

	res = UpdateGWorld(&wf->window, 8, &r, 0L, 0L, 0);
	if (res != noErr)
	{
		efree((CHAR *)newrowstart);
		ParamText((UCHAR1 *)gra_makepstring(_("Not enough memory to modify the window")), x_("\p"), x_("\p"), x_("\p"));
		Alert(errorALERT, 0L);
		return(1);
	}

	/* this next line should not be needed (fixed in 7.0) */
	RectRgn(wf->window->clipRgn, &wf->window->portRect);

	/* clear the new buffer */
	SetGWorld(wf->window, 0L);
	(void)LockPixels(wf->window->portPixMap);
	PenMode(patCopy);
	BackColor(0);
	EraseRect(&wf->window->portRect);
	UnlockPixels(wf->window->portPixMap);
	SetGWorld(wf->realwindow, gra_origgdevh);

	/* load new row start array */
	bytes = (*wf->window->portPixMap)->rowBytes & 0x7FFF;
	addr = (UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap);
	for(i=0; i<height; i++)
	{
		newrowstart[i] = addr;
		addr += bytes;
	}
	efree((CHAR *)wf->rowstart);
	wf->rowstart = newrowstart;

	wf->swid = wf->window->portRect.right - wf->window->portRect.left;
	wf->shei = wf->window->portRect.bottom - wf->window->portRect.top;
	wf->revy = wf->shei - 1;
	wf->offscreendirty = FALSE;
	return(0);
}

/*
 * routine to rebuild the array "wf->rowstart".  Since it points to the
 * image buffer, and since that buffer is a handle, the buffer can be moved by
 * the system for no good reason.  Before using the array, each
 * routine should see if it is valid and call this to repair the array.
 */
void gra_rebuildrowstart(WINDOWFRAME *wf)
{
	INTBIG height, bytes, i;
	UCHAR1 *addr;
	Rect r;

	/* the array has moved: recompute it */
	addr = (UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap);
	r = wf->realwindow->portRect;
	if (!wf->floating) r.bottom -= SBARHEIGHT; else
		r.bottom -= FLOATINGHEADERSIZE;
	height = r.bottom - r.top;
	bytes = (*wf->window->portPixMap)->rowBytes & 0x7FFF;
	for(i=0; i<height; i++)
	{
		wf->rowstart[i] = addr;
		addr += bytes;
	}
}

void gra_drawosgraphics(WINDOWFRAME *wf)
{
	WindowPtr win;

	/* figure out which window is being drawn */
	win = (WindowPtr)wf->realwindow;
	if (wf == NOWINDOWFRAME)
	{
		if (gra_messageswindow == 0) return;
		win = gra_messageswindow;
	}

	if (wf == NOWINDOWFRAME || !wf->floating)
	{
		/* for all but menu, draw the grow icon */
		DrawGrowIcon(win);
	}

	if (wf != NOWINDOWFRAME && wf->floating)
	{
		/* for floating windows, draw the small drag bar */
		gra_drawpalettedragbar(wf);
	}
}

void gra_drawpalettedragbar(WINDOWFRAME *wf)
{
	Rect r;
	Pattern thePat;
	WindowPtr win, frontWindow;

	win = (WindowPtr)wf->realwindow;
	if (wf == NOWINDOWFRAME)
	{
		if (gra_messageswindow == 0) return;
		win = gra_messageswindow;
	}

	SetPort(win);
	r = win->portRect;
	r.bottom = FLOATINGHEADERSIZE;
	EraseRect(&r);
	MoveTo(0, FLOATINGHEADERSIZE);
	LineTo(r.right, FLOATINGHEADERSIZE);
	gra_frontnonfloatingwindow(&frontWindow);
	if (win != gra_messageswindow || win == frontWindow)
	{
		/* draw the small drag bar */
		r = win->portRect;
		r.bottom = FLOATINGHEADERSIZE;
		GetIndPattern(&thePat, 0, 24);
		PenPat(&thePat);
		PaintRect(&r);
		PenPat(&qd.black);

		/* now draw the close box */
		r.top = 2;
		r.bottom = FLOATINGHEADERSIZE-1;
		r.left = 4;
		r.right = FLOATINGHEADERSIZE+1;
		PenMode(patBic);
		PaintRect(&r);
		PenMode(patCopy);
		FrameRect(&r);
	}
}

/******************** FLOATING WINDOW ROUTINES ********************/

/*
 * Routine to return the frontmost window that is NOT floating (zero if none).
 */
void gra_frontnonfloatingwindow(WindowPtr *result)
{
	WindowPeek   theWindow, firstWindow;
	WINDOWFRAME *wf;

	firstWindow = (WindowPeek)LMGetWindowList();
	for(theWindow = firstWindow; theWindow != 0; theWindow = theWindow->nextWindow)
	{
		/* see if this window is in the edit-window list */
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
			if (wf->realwindow == (CGrafPtr)theWindow) break;

		/* if not in the list, it must be messages or a dialog, and therefore valid */
		if (wf == NOWINDOWFRAME) break;

		/* accept editor window only if it is nonfloating */
		if (!wf->floating) break;
	}

	/* return what was found */
	*result = (WindowPtr)theWindow;
}

/*
 * Routine to make all floating windows visible/invisible.
 */
void gra_showallfloaters(BOOLEAN vis)
{
	WINDOWFRAME *wf;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (!wf->floating) continue;
		if (vis) ShowHide((WindowPtr)wf->realwindow, 1); else
			ShowHide((WindowPtr)wf->realwindow, 0);
	}
}

/*
 * Routine to return the address of the last floating window.  All normal
 * windows must go behind this.
 */
WindowPtr gra_lastfloatingwindow(void)
{
	WindowPeek   theWindow, firstWindow;
	WindowPtr    last;
	WINDOWFRAME *wf;

	last = 0;
	firstWindow = (WindowPeek)LMGetWindowList();
	for(theWindow = firstWindow; theWindow != 0; theWindow = theWindow->nextWindow)
	{
		/* see if this window is in the list */
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
			if (wf->realwindow == (CGrafPtr)theWindow) break;
		if (wf == NOWINDOWFRAME) continue;

		/* make sure it is floating */
		if (!wf->floating) continue;

		/* floating window found: save its address */
		last = (WindowPtr)wf->realwindow;
	}
	return(last);
}

void gra_dragfloatingwindow(WindowPtr windowToDrag, Point startPoint)
{
	Rect        dragRect, r;
	GrafPtr     savePort, windowManagerPort;
	RgnHandle   dragRegion;
	INTBIG       dragResult;
	INTBIG       topLimit, newHorizontalWindowPosition, newVerticalWindowPosition,
				horizontalOffset, verticalOffset;
	WINDOWFRAME *wf;

	/* see if this is an editor window */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if ((WindowPtr)wf->realwindow == windowToDrag) break;

	/* adjust the top of the dragging rectangle so that it�s below the menu bar */
	dragRect = qd.screenBits.bounds;
	topLimit = GetMBarHeight();
	if (dragRect.top < topLimit) dragRect.top = topLimit;
	InsetRect(&dragRect, 4, 4);

	/* Set up the Window Manager port */
	GetPort(&savePort);
	GetWMgrPort(&windowManagerPort);
	SetPort(windowManagerPort);
	SetClip(GetGrayRgn());

	/* Create a region to drag */
	dragRegion = NewRgn();

	r = (*((WindowPeek)windowToDrag)->strucRgn)->rgnBBox;
	RectRgn(dragRegion, &r);

	/* Drag the window around */
	dragResult = DragGrayRgn(dragRegion, startPoint, &dragRect, &dragRect, noConstraint, nil);

	/* Restore the port for coordinate conversion */
	SetPort(savePort);

	if (dragResult != 0)
	{
		horizontalOffset = dragResult & 0xFFFF;
		verticalOffset = dragResult >> 16;

		/* Only move it if it stayed inside the dragging box */
		if (verticalOffset != -32768)
		{
			r = (*((WindowPeek)windowToDrag)->strucRgn)->rgnBBox;
			if (wf != NOWINDOWFRAME && wf->floating)
			{
				newHorizontalWindowPosition = r.left + horizontalOffset + gra_floatwinoffleft;
				newVerticalWindowPosition = r.top + verticalOffset + gra_floatwinofftop;
			} else
			{
				newHorizontalWindowPosition = r.left + horizontalOffset + gra_winoffleft;
				newVerticalWindowPosition = r.top + verticalOffset + gra_winofftop;
			}
			MoveWindow(windowToDrag, newHorizontalWindowPosition, newVerticalWindowPosition, false);
		}
	}

	/* Get rid of the dragging region */
	DisposeRgn(dragRegion);

	/* if this is the messages window, adjust the bits */
	if (windowToDrag == gra_messageswindow)
	{
		r = (*((WindowPeek)windowToDrag)->strucRgn)->rgnBBox;
		gra_messagesleft = r.left+gra_winoffleft;   gra_messagesright = r.right+gra_winoffright;
		gra_messagestop = r.top+gra_winofftop;      gra_messagesbottom = r.bottom+gra_winoffbottom;

		/* save new messages window settings */
		gra_savewindowsettings();
	}
}

void gra_selectoswindow(WindowPtr windowToSelect)
{
	WindowPtr    currentFrontWindow;
	WINDOWFRAME *wf;
	WindowPtr    lastFloatingWindow;

	/* handle floating windows specially */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (wf->realwindow == (CGrafPtr)windowToSelect)
	{
		if (wf->floating)
		{
			BringToFront(windowToSelect);
			return;
		}
		break;
	}

	/* quit now if this is already selected */
	gra_frontnonfloatingwindow(&currentFrontWindow);
	if (currentFrontWindow == windowToSelect) return;

	/* deactivate current frontmost window */
	if (currentFrontWindow != 0) gra_activatewindow(currentFrontWindow, FALSE);

	/* bring this window to the front */
	lastFloatingWindow = gra_lastfloatingwindow();
	if (lastFloatingWindow == 0) BringToFront(windowToSelect); else
		SendBehind(windowToSelect, lastFloatingWindow);

	/* activate this one */
	gra_activatewindow(windowToSelect, TRUE);
}
/*
 * Routine to highlight and activate a window.
 */
void gra_activatewindow(WindowPtr win, BOOLEAN active)
{
	Rect r;
	WINDOWFRAME *wf;

	HiliteWindow(win, active?1:0);
	if (win == gra_messageswindow && gra_messageswindow != 0)
	{
		if (active)
		{
			ShowControl(gra_vScroll);
			ShowControl(gra_hScroll);
			gra_messagesinfront = TRUE;
		} else
		{
			HideControl(gra_vScroll);
			HideControl(gra_hScroll);
			gra_messagesinfront = FALSE;
		}
		DrawGrowIcon(win);
		return;
	}

	/* see if it is a standard edit window */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if ((WindowPtr)wf->realwindow == win) break;
	if (wf == NOWINDOWFRAME) return;
	if (wf->floating) return;

	/* update the grow box in the edit window */
	r.right = win->portRect.right;
	r.left = r.right - 15;
	r.bottom = win->portRect.bottom;
	r.top = r.bottom - 15;
	RectRgn(win->clipRgn, &r);
	DrawGrowIcon(win);
	RectRgn(win->clipRgn, &win->portRect);
}

/*
 * Routine to close a window.
 */
void gra_disposeoswindow(WindowPtr win)
{
	WINDOWFRAME *wf, *lastwf;
	WindowPtr front;

	/* find and remove the window frame from the list */
	lastwf = NOWINDOWFRAME;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->realwindow == (CGrafPtr)win)
		{
			if (lastwf == NOWINDOWFRAME) el_firstwindowframe = wf->nextwindowframe; else
				lastwf->nextwindowframe = wf->nextwindowframe;
			DisposeGWorld(wf->window);
			wf->realwindow = 0;
			wf->window = 0;
			efree((CHAR *)wf);
			break;
		}
		lastwf = wf;
	}

	/* deactivate, make invisible, and dispose */
	gra_activatewindow(win, FALSE);
	ShowHide(win, 0);
	DisposeWindow(win);

	/* activate the next one (if there is) */
	gra_frontnonfloatingwindow(&front);
	if (front != 0) gra_activatewindow(front, TRUE);

	/* determine current window frame */
	gra_setcurrentwindowframe();
}

/*
 * routine to determine the current window frame and load the global "el_curwindowframe".
 */
void gra_setcurrentwindowframe(void)
{
	WindowPeek theWindow, firstWindow;
	WINDOWFRAME *wf;
	WINDOWPART *w;

	el_curwindowframe = NOWINDOWFRAME;
	firstWindow = (WindowPeek)LMGetWindowList();
	for(theWindow = firstWindow; theWindow != 0; theWindow = theWindow->nextWindow)
	{
		/* see if this window is in the edit-window list */
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
			if (wf->realwindow == (CGrafPtr)theWindow) break;

		/* if not in the list, it must be a dialog */
		if (wf == NOWINDOWFRAME)
		{
			el_curwindowframe = wf;
			break;
		}

		/* ignore floating windows */
		if (wf->floating) continue;

		el_curwindowframe = wf;

		/* see if the change of window frame invalidates the current window */
		if (el_curwindowpart == NOWINDOWPART || el_curwindowpart->frame != wf)
		{
			/* must choose new window (if not broadcasting) */
			if (db_broadcasting == 0)
			{
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				{
					if (w->frame == wf)
					{
						(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
							VWINDOWPART|VDONTSAVE);
						(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"),
							(INTBIG)w->curnodeproto, VNODEPROTO);
						break;
					}
				}
			}
		}
		break;
	}
}

void gra_savewindowsettings(void)
{
	Handle f;
	INTBIG i;
	CHAR line[256];

	/* get the current font name and messages window location */
	GetFontName(gra_messagesfont, (UCHAR1 *)line);

	/* remove any previous resource */
	f = GetResource('MPSR', 1004);
	if (f != 0) RemoveResource(f);

	/* make a handle for this information */
	f = NewHandle(line[0]+19);

	/* load the window location information */
	((INTSML *)(*f))[0] = gra_messagesleft;
	((INTSML *)(*f))[1] = gra_messagesright;
	((INTSML *)(*f))[2] = gra_messagestop;
	((INTSML *)(*f))[3] = gra_messagesbottom;
	((INTSML *)(*f))[4] = 0;
	((INTSML *)(*f))[5] = 0;
	((INTSML *)(*f))[6] = 0;
	((INTSML *)(*f))[7] = 0;

	/* load the font information */
	((INTSML *)(*f))[8] = gra_messagesfontsize;
	for(i=0; i<line[0]; i++) (*f)[i+18] = line[i+1];
	(*f)[line[0]+18] = 0;

	/* write the resource */
	AddResource(f, 'MPSR', 1004, x_("\pWindow"));
	f = GetResource('MPSR', 1004);
	if (f != 0) WriteResource(f);
}

/******************** MISCELLANEOUS EXTERNAL ROUTINES ********************/

/*
 * return true if the capabilities in "want" are present
 */
BOOLEAN graphicshas(INTBIG want)
{
	/* cannot run subprocesses */
	if ((want&(CANRUNPROCESS|CANDOTHREADS)) != 0) return(FALSE);

	if (!gra_multiwindow)
	{
		if ((want&CANUSEFRAMES) != 0) return(FALSE);
	}

	/* popups can only be drawn if the mouse is down */
	if ((want&CANSHOWPOPUP) != 0)
	{
		if (!Button()) return(FALSE);
	}

	return(TRUE);
}

/*
 * Routine to make sound "sound".  If sounds are turned off, no
 * sound is made (unless "force" is TRUE)
 */
void ttybeep(INTBIG sound, BOOLEAN force)
{
	BOOLEAN soundinited = FALSE;
	Movie movie;
	short movieRefNum, resID = 0;
	OSErr err;
	CHAR filename[256];
	FSSpec fss;

	if ((us_tool->toolstate & TERMBEEP) != 0 || force)
	{
		switch (sound)
		{
			case SOUNDBEEP:
				SysBeep(20);
				break;
			case SOUNDCLICK:
				if ((us_useroptions&NOEXTRASOUND) != 0) break;
				if (!soundinited)
				{
					esnprintf((CHAR *)&filename[1], 255, x_("%sclick.wav"), el_libdir);
					filename[0] = estrlen((CHAR *)&filename[1]);
					FSMakeFSSpec(0, 0, (UCHAR1 *)filename, &fss);
					err = EnterMovies();
					if (err != 0) return;
					err = OpenMovieFile(&fss, &movieRefNum, fsRdPerm);
					if (err != 0) return;
					err = NewMovieFromFile(&movie, movieRefNum, &resID, NULL, newMovieActive, NULL);
					if (err != 0) return;
					SetMovieVolume(movie, kFullVolume);
					soundinited = TRUE;
				}
				GoToBeginningOfMovie(movie);
				StartMovie(movie);
				while (!IsMovieDone(movie))
				{
					MoviesTask(movie, 0);
					err = GetMoviesError();
				}
				break;
		}
	}
}

void error(CHAR *s, ...)
{
	va_list ap;
	CHAR line[256];

	var_start(ap, s);
	evsnprintf(&line[1], 255, s, ap);
	va_end(ap);
	line[0] = estrlen(&line[1]);
	ParamText((UCHAR1 *)gra_makepstring(_("Error: ")), (UCHAR1 *)line, x_("\p"), x_("\p"));
	StopAlert(130, 0L);
	exitprogram();
}

/*
 * Routine to get the environment variable "name" and return its value.
 */
CHAR *egetenv(CHAR *name)
{
	return(0);
}

/*
 * Routine to get the current language that Electric speaks.
 */
CHAR *elanguage(void)
{
#ifdef INTERNATIONAL
	return(x_("fr"));
#else
	return(x_("en"));
#endif
}

/*
 * Routine to fork a new process.  Returns the child process number if this is the
 * parent thread.  Returns 0 if this is the child thread.
 * Returns 1 if forking is not possible (process 1 is INIT on UNIX and can't possibly
 * be assigned to a normal process).
 */
INTBIG efork(void)
{
	return(1);
}

/*
 * Routine to run the string "command" in a shell.
 * Returns nonzero if the command cannot be run.
 */
INTBIG esystem(CHAR *command)
{
	return(1);
}

/*
 * Routine to execute the program "program" with the arguments "args"
 */
void eexec(CHAR *program, CHAR *args[])
{
}

/*
 * routine to send signal "signal" to process "process".
 */
INTBIG ekill(INTBIG process)
{
	return(1);
}

/*
 * routine to wait for the completion of child process "process"
 */
void ewait(INTBIG process)
{
}
/*
 * Routine to return the number of processors on this machine.
 */
INTBIG enumprocessors(void)
{
	return(1);
}

/*
 * Routine to create a new thread that calls "function" with "argument".
 */
void enewthread(void* (*function)(void*), void *argument)
{
}

/*
 * Routine that creates a mutual-exclusion object and returns it.
 */
void *emakemutex(void)
{
	return(0);
}

/*
 * Routine that locks mutual-exclusion object "vmutex".  If the object is already
 * locked, the routine blocks until it is unlocked.
 */
void emutexlock(void *vmutex)
{
}

/*
 * Routine that unlocks mutual-exclusion object "vmutex".
 */
void emutexunlock(void *vmutex)
{
}

/*
 * Routine to determine the list of printers and return it.
 * The list terminates with a zero.
 */
CHAR **eprinterlist(void)
{
	static CHAR *lplist[1];

	lplist[0] = 0;
	return(lplist);
}

/******************** TIMING ROUTINES ********************/

#if defined(powerc) || defined(__powerc)
static void gra_dovbl(VBLRec *recPtr)
{
	long curA5;

	curA5 = SetA5(recPtr->vblA5);

	/* do the task */
	gra_motioncheck = TRUE;
	if (--gra_checkcountdown <= 0)
	{
		gra_checkcountdown = INTCHECK;
		gra_cancheck = TRUE;
	}

	/* reset counter and global pointers */
	recPtr->myVBLTask.vblCount = MOTIONCHECK;
	curA5 = SetA5(curA5);
}
#else
static pascal long GetVBLRec(void) = 0x2E88;

static void gra_dovbl(void)
{
	long curA5;
	VBLRec *recPtr;

	recPtr = (VBLRec *)GetVBLRec();
	curA5 = SetA5(recPtr->vblA5);

	/* do the task */
	gra_motioncheck = TRUE;
	if (--gra_checkcountdown <= 0)
	{
		gra_checkcountdown = INTCHECK;
		gra_cancheck = TRUE;
	}

	/* reset counter and global pointers */
	recPtr->myVBLTask.vblCount = MOTIONCHECK;
	curA5 = SetA5(curA5);
}
#endif

void gra_installvbl(void)
{
	OSErr err;

	gra_vblrec.myVBLTask.qType = vType;
	gra_vblrec.myVBLTask.vblAddr = NewVBLProc(gra_dovbl);
	gra_vblrec.myVBLTask.vblCount = MOTIONCHECK;
	gra_vblrec.myVBLTask.vblPhase = 0;
	gra_vblrec.vblA5 = SetCurrentA5();
	err = VInstall((QElemPtr)&gra_vblrec.myVBLTask);
	if (err == noErr) gra_cancheck = TRUE;
	gra_cancheck = FALSE;
	gra_motioncheck = FALSE;
	gra_checkcountdown = INTCHECK;
}

/******************** MESSAGES WINDOW ROUTINES ********************/

/*
 * Routine to delete the messages window.
 */
void gra_hidemessageswindow(void)
{
	if (gra_messageswindow == 0) return;
	gra_disposeoswindow(gra_messageswindow);
	gra_messagesinfront = FALSE;
	gra_messageswindow = 0;
}

/*
 * Routine to create the messages window.
 */
BOOLEAN gra_showmessageswindow(void)
{
	Rect r;
	WindowPtr frontmost, wBehind;

	if (gra_messageswindow == 0)
	{
		/* deactivate any other nonfloating window that is current */
		gra_frontnonfloatingwindow(&frontmost);
		if (frontmost != 0) gra_activatewindow(frontmost, FALSE);

		/* figure out which window to put this behind */
		wBehind = gra_lastfloatingwindow();
		if (wBehind == 0) wBehind = (WindowPtr)-1;

		r.left = gra_messagesleft;   r.right = gra_messagesright;
		r.top = gra_messagestop;     r.bottom = gra_messagesbottom;
		gra_messageswindow = (WindowPtr)NewWindow(0L, &r, (UCHAR1 *)gra_makepstring(_("Electric Messages")),
			1, documentProc, wBehind, 1, 0L);
		if (gra_messageswindow == 0) return(TRUE);
		SetPort(gra_messageswindow);
		EraseRect(&gra_messageswindow->portRect);
		TextFont(gra_messagesfont);
		TextSize(gra_messagesfontsize);
		r = gra_messageswindow->portRect;
		r.left = r.right - SBARWIDTH;
		r.right++;
		r.bottom -= 14;
		r.top += 1;
		gra_vScroll = NewControl((WindowRef)gra_messageswindow, &r, x_("\p"), 1, 0, 0, 0,
			scrollBarProc, 0L);
		if (gra_vScroll == 0) return(TRUE);
		r = gra_messageswindow->portRect;
		r.top = r.bottom - SBARHEIGHT;
		r.bottom++;
		r.right -= 14;
		r.left--;
		gra_hScroll = NewControl((WindowRef)gra_messageswindow, &r, x_("\p"), 1, 0, 0, 0,
			scrollBarProc, 0L);
		if (gra_hScroll == 0) return(TRUE);
		HideControl(gra_vScroll);
		HideControl(gra_hScroll);
		DrawGrowIcon((WindowRef)gra_messageswindow);
		SetControlMinimum(gra_hScroll, 0);
		SetControlMaximum(gra_hScroll, 90);
	}
	if (gra_TEH == 0)
	{
		r = gra_messageswindow->portRect;
		gra_TEH = TENew(&r, &r);
		if (gra_TEH == 0) return(TRUE);
	}
	gra_setview(gra_messageswindow);
	TEActivate(gra_TEH);
	return(FALSE);
}

void gra_setview(WindowPtr w)
{
	INTBIG width;

	(*gra_TEH)->viewRect = w->portRect;
	(*gra_TEH)->viewRect.right -= SBARWIDTH;
	(*gra_TEH)->viewRect.bottom -= SBARHEIGHT;
	width = (*gra_TEH)->viewRect.right - (*gra_TEH)->viewRect.left;
	InsetRect(&(*gra_TEH)->viewRect, 4, 4);
	(*gra_TEH)->destRect = (*gra_TEH)->viewRect;
	gra_linesInFolder = ((*gra_TEH)->destRect.bottom - (*gra_TEH)->destRect.top) /
		(*gra_TEH)->lineHeight;
	(*gra_TEH)->destRect.bottom = (*gra_TEH)->destRect.top + (*gra_TEH)->lineHeight *
		gra_linesInFolder;
	(*gra_TEH)->destRect.left += 4;
	(*gra_TEH)->destRect.right = (*gra_TEH)->destRect.left + width*10;
	TECalText(gra_TEH);
}

pascal void gra_scrollvproc(ControlHandle theControl, short theCode)
{
	INTBIG pageSize, scrollAmt;

	if (theCode == 0) return;
	pageSize = ((*gra_TEH)->viewRect.bottom-(*gra_TEH)->viewRect.top) / (*gra_TEH)->lineHeight - 1;
	switch (theCode)
	{
		case kControlUpButtonPart:   scrollAmt = -1;          break;
		case kControlDownButtonPart: scrollAmt = 1;           break;
		case kControlPageUpPart:     scrollAmt = -pageSize;   break;
		case kControlPageDownPart:   scrollAmt = pageSize;    break;
	}
	SetControlValue(theControl, GetControlValue(theControl)+scrollAmt);
	gra_adjustvtext();
}

pascal void gra_scrollhproc(ControlHandle theControl, short theCode)
{
	INTBIG scrollAmt, pos, oldpos;

	if (theCode == 0) return;
	switch (theCode)
	{
		case kControlUpButtonPart:   scrollAmt = -1;    break;
		case kControlDownButtonPart: scrollAmt = 1;     break;
		case kControlPageUpPart:     scrollAmt = -10;   break;
		case kControlPageDownPart:   scrollAmt = 10;    break;
	}
	oldpos = GetControlValue(theControl);
	pos = oldpos + scrollAmt;
	if (pos < 0) pos = 0;
	if (pos > 90) pos = 90;
	SetControlValue(theControl, pos);
	gra_adjusthtext(oldpos);
}

void gra_adjustvtext(void)
{
	INTBIG oldScroll, newScroll, delta;

	oldScroll = (*gra_TEH)->viewRect.top - (*gra_TEH)->destRect.top;
	newScroll = GetControlValue(gra_vScroll) * (*gra_TEH)->lineHeight;
	delta = oldScroll - newScroll;
	if (delta != 0) TEScroll(0, delta, gra_TEH);
}

void gra_adjusthtext(INTBIG oldpos)
{
	INTBIG pos, wid, delta;

	pos = GetControlValue(gra_hScroll);
	wid = ((*gra_TEH)->viewRect.right - (*gra_TEH)->viewRect.left) / 10;
	delta = wid * (pos - oldpos);
	if (delta != 0) TEScroll(-delta, 0, gra_TEH);
}

void gra_setvscroll(void)
{
	INTBIG n;

	n = (*gra_TEH)->nLines - gra_linesInFolder + 1;
	if ((*gra_TEH)->teLength > 0 && (*((*gra_TEH)->hText))[(*gra_TEH)->teLength-1] != '\r') n++;
	SetControlMaximum(gra_vScroll, n > 0 ? n : 0);
}

void gra_showselect(void)
{
	INTBIG topLine, bottomLine, theLine;

	gra_setvscroll();
	gra_adjustvtext();

	topLine = GetControlValue(gra_vScroll);
	bottomLine = topLine + gra_linesInFolder;

	if ((*gra_TEH)->selStart < (*gra_TEH)->lineStarts[topLine] ||
		(*gra_TEH)->selStart >= (*gra_TEH)->lineStarts[bottomLine])
	{
		for (theLine = 0; (*gra_TEH)->selStart >= (*gra_TEH)->lineStarts[theLine]; theLine++) ;
		SetControlValue(gra_vScroll, theLine - gra_linesInFolder / 2);
		gra_adjustvtext();
	}
}

/*
 * routine to cut text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN cutfrommessages(void)
{
	void io_maccopyhighlighted(void);

	/* if a desk accessory wants the "cut" command, stop now */
	if (SystemEdit(2) != 0) return(TRUE);

	if (gra_messagesinfront)
	{
		TECut(gra_TEH);
		ZeroScrap();
		TEToScrap();
		return(TRUE);
	}
	if (el_curwindowpart == NOWINDOWPART) return(FALSE);
	if ((el_curwindowpart->state&WINDOWTYPE) == DISPWINDOW)
		io_maccopyhighlighted();
	return(FALSE);
}

/*
 * routine to copy text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN copyfrommessages(void)
{
	void io_maccopyhighlighted(void);

	/* if a desk accessory wants the "copy" command, stop now */
	if (SystemEdit(3) != 0) return(TRUE);

	if (gra_messagesinfront)
	{
		TECopy(gra_TEH);
		ZeroScrap();
		TEToScrap();
		return(TRUE);
	}
	if (el_curwindowpart == NOWINDOWPART) return(FALSE);
	if ((el_curwindowpart->state&WINDOWTYPE) == DISPWINDOW)
		io_maccopyhighlighted();
	return(FALSE);
}

/*
 * routine to paste text to the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN pastetomessages(void)
{
	/* if a desk accessory wants the "paste" command, stop now */
	if (SystemEdit(4) != 0) return(TRUE);

	if (gra_messagesinfront)
	{
		TEFromScrap();
		TEPaste(gra_TEH);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to get the contents of the system cut buffer
 */
CHAR *getcutbuffer(void)
{
	Handle sc;
	INTBIG scoffset, len, i;
	REGISTER void *infstr;

	/* get cut buffer */
	sc = NewHandle(0);
	len = GetScrap(sc, 'TEXT', (long *)&scoffset);
	if (len < 0) ttyputerr(x_("Error %ld reading scrap"), len);
	infstr = initinfstr();
	for(i=0; i<len; i++) addtoinfstr(infstr, (*sc)[i]);
	return(returninfstr(infstr));
}

/*
 * routine to set the contents of the system cut buffer to "msg"
 */
void setcutbuffer(CHAR *msg)
{
	INTBIG err;

	ZeroScrap();
	err = PutScrap(estrlen(msg), 'TEXT', msg);
	if (err != 0) ttyputerr(x_("Error %ld copying to scrap"), err);
}

static DIALOGITEM gra_fontdialogitems[] =
{
 /*  1 */ {0, {136,192,160,256}, BUTTON, N_("OK")},
 /*  2 */ {0, {88,192,112,256}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,24,24,56}, MESSAGE, N_("Font")},
 /*  4 */ {0, {24,200,40,232}, MESSAGE, N_("Size")},
 /*  5 */ {0, {32,24,160,184}, SCROLL, x_("")},
 /*  6 */ {0, {48,192,64,248}, EDITTEXT, x_("")}
};
DIALOG gra_fontdialog = {{50,75,219,340}, N_("Messages Window Font"), 0, 6, gra_fontdialogitems, 0, 0};
/* special items for the font dialog: */
#define DMSF_FONTLIST    5		/* Font list (scroll) */
#define DMSF_FONTSIZE    6		/* Size (edit text) */

void setmessagesfont(void)
{
	INTBIG itemHit, i, tot, which, fontnum;
	INTBIG typ, fonttype;
	short id;
	Handle f;
	CHAR line[256];
	FontInfo finfo;
	void *dia;

	/* display the font dialog box */
	dia = DiaInitDialog(&gra_fontdialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DMSF_FONTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE|SCDOUBLEQUIT);

	/* if there are 'FONT's, then no names will be obtained by "GetResInfo" below!!! */
	fonttype = 'FOND';
	tot = CountResources(fonttype);
	if (tot == 0)
	{
		fonttype = 'FONT';
		tot = CountResources(fonttype);
	}
	which = fontnum = 0;
	for(i=1; i<=tot; i++)
	{
		SetResLoad(0);
		f = GetIndResource(fonttype, i);
		GetResInfo(f, &id, (ResType *)&typ, (UCHAR1 *)line);
		SetResLoad(1);
		if (line[0] == 0) continue;
		GetFNum((UCHAR1 *)line, &id);
		if (id == gra_messagesfont) which = fontnum;
		line[line[0]+1] = 0;
		DiaStuffLine(dia, DMSF_FONTLIST, &line[1]);
		fontnum++;
	}
	DiaSelectLine(dia, DMSF_FONTLIST, which);
	(void)esnprintf(line, 256, x_("%ld"), gra_messagesfontsize);
	DiaSetText(dia, -DMSF_FONTSIZE, line);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	if (itemHit != CANCEL)
	{
		(void)estrcpy(&line[1], DiaGetScrollLine(dia, DMSF_FONTLIST, DiaGetCurLine(dia, DMSF_FONTLIST)));
		line[0] = estrlen(&line[1]);
		GetFNum((UCHAR1 *)line, &id);
		i = myatoi(DiaGetText(dia, DMSF_FONTSIZE));
		if (i != gra_messagesfontsize || id != gra_messagesfont)
		{
			if (gra_messageswindow == 0)
			{
				if (gra_showmessageswindow()) return;
			}

			gra_messagesfontsize = i;
			gra_messagesfont = id;
			SetPort(gra_messageswindow);
			TextSize(i);
			TextFont(id);
			GetFontInfo(&finfo);
			(*gra_TEH)->txSize = i;
			(*gra_TEH)->txFont = id;
			(*gra_TEH)->fontAscent = finfo.ascent;
			(*gra_TEH)->lineHeight = finfo.ascent + finfo.descent + finfo.leading;
			gra_setview(gra_messageswindow);
			EraseRect(&gra_messageswindow->portRect);
			DrawControls(gra_messageswindow);
			DrawGrowIcon(gra_messageswindow);
			TEUpdate(&gra_messageswindow->portRect, gra_TEH);

			/* save new messages window settings */
			gra_savewindowsettings();
		}
	}
	DiaDoneDialog(dia);
}

/*
 * Routine to force the module to return with some event so that the user interface will
 * relinquish control and let a slice happen.
 */
void forceslice(void)
{
}

/*
 * Routine to put the string "s" into the messages window.
 * Pops up the messages window if "important" is true.
 */
void putmessagesstring(CHAR *s, BOOLEAN important)
{
	INTBIG len, insert;
	Rect r;
	GrafPtr savePort;
	WINDOWFRAME *wf;

	len = estrlen(s);
	GetPort(&savePort);
	if (gra_messageswindow == 0)
	{
		if (!important) return;
		if (gra_showmessageswindow()) return;
	}

	/* see if the messages window occludes any edit window */
	if (important)
	{
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (SectRect(&(*((WindowPeek)gra_messageswindow)->strucRgn)->rgnBBox,
				&(*((WindowPeek)wf->realwindow)->strucRgn)->rgnBBox, &r) != 0)
			{
				gra_selectoswindow(gra_messageswindow);
				break;
			}
		}
	}

	if ((*gra_TEH)->teLength > 30000)
	{
		/* cut out the top half of the buffer before it overflows */
		TESetSelect(0, 15000, gra_TEH);
		TEKey(BACKSPACEKEY, gra_TEH);
		insert = 32767;
		TESetSelect(insert, insert, gra_TEH);
	}
	TEInsert(s, len, gra_TEH);
	TEKey('\r', gra_TEH);
	insert = 32767;
	TESetSelect(insert, insert, gra_TEH);
	gra_showselect();
	SetPort(savePort);
}
/*
 * Routine to return the name of the key that ends a session from the messages window.
 */
CHAR *getmessageseofkey(void)
{
	return(x_("^D"));
}

/*
 * Routine to get a string from the scrolling messages window.  Returns zero if end-of-file
 * (^D) is typed.
 */
CHAR *getmessagesstring(CHAR *prompt)
{
	EventRecord theEvent;
	INTBIG start, end, i, j, ch;
	static CHAR outline[256];

	if (gra_messageswindow == 0)
	{
		if (gra_showmessageswindow()) return(x_(""));
	}
	gra_selectoswindow(gra_messageswindow);

	/* show the prompt */
	TESetSelect(32767, 32767, gra_TEH);
	TEInsert(prompt, estrlen(prompt), gra_TEH);
	TESetSelect(32767, 32767, gra_TEH);
	start = (*gra_TEH)->selStart;
	for(;;)
	{
		/* continue to force the messages window up */
		if (gra_messageswindow == 0)
		{
			if (gra_showmessageswindow()) return(x_(""));
		}
		gra_waitforaction(0, &theEvent);
		end = (*gra_TEH)->selStart-1;
		if (end < start) continue;
		ch = (*(*gra_TEH)->hText)[end];
		if (ch == '\r' || ch == 4) break;
	}
	TEKey(BACKSPACEKEY, gra_TEH);
	TEKey('\r', gra_TEH);
	if (ch == 4) return(0);
	j = 0;
	for(i=start; i<end; i++) outline[j++] = (*(*gra_TEH)->hText)[i];
	outline[j] = 0;
	return(outline);
}

/*
 * Routine to remove the last character from the scrolling messages window.
 * Called from "usrterminal.c"
 */
void gra_backup(void)
{
	GrafPtr savePort;

	GetPort(&savePort);
	if (gra_messageswindow == 0)
	{
		if (gra_showmessageswindow()) return;
	}
	TEKey(BACKSPACEKEY, gra_TEH);
	gra_showselect();
	SetPort(savePort);
}

/*
 * Routine to clear all text from the messages window.
 */
void clearmessageswindow(void)
{
	TESetSelect(0, 32767, gra_TEH);
	TEDelete(gra_TEH);
}

/******************** STATUS BAR ROUTINES ********************/

/*
 * Routine to return the number of status lines on the display.
 */
INTBIG ttynumstatuslines(void)
{
	return(MAXSTATUSLINES);
}

/*
 * Routine to display "message" in the status "field" of window "frame" (uses all windows
 * if "frame" is zero).  If "cancrop" is false, field cannot be cropped and should be
 * replaced with "*" if there isn't room.
 */
void ttysetstatusfield(WINDOWFRAME *mwwant, STATUSFIELD *sf, CHAR *message, BOOLEAN cancrop)
{
	INTBIG len, i, width, winwid, startx, endx;
	Rect clear;
	REGISTER WINDOWFRAME *wf;

	if (sf == 0) return;
	if (sf->line != 0 && sf->label[0] == 0) return;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;
		if (mwwant != NOWINDOWFRAME && mwwant != wf) continue;

		/* construct the status line */
		len = estrlen(sf->label);
		if (len + estrlen(message) >= MAXLOCALSTRING-1)
		{
			estrcpy(&gra_localstring[1], sf->label);
			i = MAXLOCALSTRING - len - 2;
			estrncat(&gra_localstring[1], message, i);
			gra_localstring[MAXLOCALSTRING-1] = 0;
		} else
		{
			esnprintf(&gra_localstring[1], MAXLOCALSTRING-1, x_("%s%s"), sf->label, message);
		}
		len = estrlen(&gra_localstring[1]);
		while (len > 0 && gra_localstring[len] == ' ') gra_localstring[len--] = 0;

		/* special case for window title */
		if (sf->line == 0)
		{
			gra_localstring[0] = estrlen(&gra_localstring[1]);
			SetWTitle((WindowPtr)wf->realwindow, (UCHAR1 *)gra_localstring);
			return;
		}

		/* determine how much room there is for the status field */
		SetPort((WindowPtr)wf->realwindow);
		winwid = wf->realwindow->portRect.right - wf->realwindow->portRect.left - SBARWIDTH;
		startx = winwid * sf->startper / 100 + wf->realwindow->portRect.left;
		endx = winwid * sf->endper / 100 + wf->realwindow->portRect.left;

		/* make sure the message fits */
		width = TextWidth(&gra_localstring[1], 0, len);
		if (width > endx-startx && !cancrop)
		{
			for(i=estrlen(sf->label); gra_localstring[i+1] != 0; i++)
				gra_localstring[i+1] = '*';
		}
		while (len > 0 && width > endx-startx)
		{
			len--;
			width = TextWidth(&gra_localstring[1], 0, len);
		}

		/* display the field */
		TextFont(SFONT);
		TextSize(9);
		TextMode(srcOr);
		clear.left = startx;
		clear.right = endx;
		clear.bottom = wf->realwindow->portRect.bottom;
		clear.top = clear.bottom - SBARHEIGHT+1;
		EraseRect(&clear);
		if (len > 0)
		{
			MoveTo(startx, wf->realwindow->portRect.bottom-4);
			DrawText(&gra_localstring[1], 0, len);
		}
	}
}

/*
 * Routine to free status field object "sf".
 */
void ttyfreestatusfield(STATUSFIELD *sf)
{
	efree(sf->label);
	efree((CHAR *)sf);
}

/******************** GRAPHICS CONTROL ROUTINES ********************/

void flushscreen(void)
{
	WINDOWFRAME *wf;
	Rect dr, copyrect;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		/* if screen has not changed, stop now */
		if (!wf->offscreendirty) continue;
		wf->offscreendirty = FALSE;

		/* make sure region falls inside screen */
		SetPort((WindowPtr)wf->realwindow);
		copyrect.left = wf->copyleft;   copyrect.right = wf->copyright;
		copyrect.top = wf->copytop;     copyrect.bottom = wf->copybottom;
		if (copyrect.left < 0) copyrect.left = 0;
		if (copyrect.right > (*wf->window->portPixMap)->bounds.right)
			copyrect.right = (*wf->window->portPixMap)->bounds.right;
		if (copyrect.top < 0) copyrect.top = 0;
		if (copyrect.bottom > (*wf->window->portPixMap)->bounds.bottom)
			copyrect.bottom = (*wf->window->portPixMap)->bounds.bottom;

		(void)LockPixels(wf->window->portPixMap);
		if (!wf->floating)
		{
			CopyBits((BitMap *)*(wf->window->portPixMap), (BitMap *)*wf->realwindow->portPixMap,
				&copyrect, &copyrect, srcCopy, 0L);
		} else
		{
			dr = copyrect;
			dr.top += FLOATINGHEADERSIZE;
			dr.bottom += FLOATINGHEADERSIZE;
			CopyBits((BitMap *)*(wf->window->portPixMap), (BitMap *)*wf->realwindow->portPixMap,
				&copyrect, &dr, srcCopy, 0L);
		}
		UnlockPixels(wf->window->portPixMap);
	}
}

/*
 * Routine to accumulate the rectangle of change to the offscreen PixMap
 */
void gra_setrect(WINDOWFRAME *wf, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	static INTBIG checktimefreq = 0;

	if (wf->offscreendirty)
	{
		if (lx < wf->copyleft) wf->copyleft = lx;
		if (hx > wf->copyright) wf->copyright = hx;
		if (ly < wf->copytop) wf->copytop = ly;
		if (hy > wf->copybottom) wf->copybottom = hy;
	} else
	{
		wf->copyleft = lx;   wf->copyright = hx;
		wf->copytop = ly;    wf->copybottom = hy;
		wf->offscreendirty = TRUE;
		wf->starttime = TickCount();
	}

	/* flush the screen every two seconds */
	checktimefreq++;
	if (checktimefreq > 10000)
	{
		checktimefreq = 0;
		if (TickCount() - wf->starttime > FLUSHTICKS) flushscreen();
	}
}

void gra_reloadmap(void)
{
	REGISTER VARIABLE *varred, *vargreen, *varblue;

	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE) return;
	colormapload((INTBIG *)varred->addr, (INTBIG *)vargreen->addr, (INTBIG *)varblue->addr, 0, 255);
}

void colormapload(INTBIG *red, INTBIG *green, INTBIG *blue, INTBIG low, INTBIG high)
{
	INTBIG i;
	CTabHandle clut;
	REGISTER WINDOWFRAME *wf;
	RGBColor fg, bg;

	clut = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		clut = (*wf->window->portPixMap)->pmTable;
		for(i=low; i<=high; i++)
		{
			(*clut)->ctTable[i].rgb.red = red[i-low] << 8;
			(*clut)->ctTable[i].rgb.green = green[i-low] << 8;
			(*clut)->ctTable[i].rgb.blue = blue[i-low] << 8;
			if (i == 255)
			{
				(*clut)->ctTable[i].rgb.red = (255-red[i-low]) << 8;
				(*clut)->ctTable[i].rgb.green = (255-green[i-low]) << 8;
				(*clut)->ctTable[i].rgb.blue = (255-blue[i-low]) << 8;
			}
		}
		(*clut)->ctSeed++;

		/* mark the entire screen for redrawing */
		if (low == 0 && high == 255)
			gra_setrect(wf, 0, (*wf->window->portPixMap)->bounds.right, 0,
				(*wf->window->portPixMap)->bounds.bottom);
	}

	/* recache table and all PixMaps */
	if (clut != 0) CTabChanged(clut);

	/* set the messages window colors */
	if (low == 0 && high == 255 && gra_messageswindow != 0)
	{
		fg.red = red[CELLTXT] << 8;
		fg.green = green[CELLTXT] << 8;
		fg.blue = blue[CELLTXT] << 8;
		bg.red = red[0] << 8;
		bg.green = green[0] << 8;
		bg.blue = blue[0] << 8;
		SetPort(gra_messageswindow);
		RGBForeColor(&fg);
		RGBBackColor(&bg);
		InvalRect(&gra_messageswindow->portRect);
	}
}

/*
 * helper routine to set the cursor shape to "state"
 */
void setdefaultcursortype(INTBIG state)
{
	if (us_cursorstate == state) return;

	switch (state)
	{
		case NORMALCURSOR:
			SetCursor(&qd.arrow);
			break;
		case WANTTTYCURSOR:
			if (gra_wantttyCurs != 0L) SetCursor(&(**gra_wantttyCurs)); else
			SetCursor(&qd.arrow);
			break;
		case PENCURSOR:
			if (gra_penCurs != 0L) SetCursor(&(**gra_penCurs)); else
				SetCursor(&qd.arrow);
			break;
		case NULLCURSOR:
			if (gra_nullCurs != 0L) SetCursor(&(**gra_nullCurs)); else
				SetCursor(&qd.arrow);
			break;
		case MENUCURSOR:
			if (gra_menuCurs != 0L) SetCursor(&(**gra_menuCurs)); else
				SetCursor(&qd.arrow);
			break;
		case HANDCURSOR:
			if (gra_handCurs != 0L) SetCursor(&(**gra_handCurs)); else
				SetCursor(&qd.arrow);
			break;
		case TECHCURSOR:
			if (gra_techCurs != 0L) SetCursor(&(**gra_techCurs)); else
				SetCursor(&qd.arrow);
			break;
		case IBEAMCURSOR:
			if (gra_ibeamCurs != 0L) SetCursor(&(**gra_ibeamCurs)); else
				SetCursor(&qd.arrow);
			break;
		case LRCURSOR:
			if (gra_lrCurs != 0L) SetCursor(&(**gra_lrCurs)); else
				SetCursor(&qd.arrow);
			break;
		case UDCURSOR:
			if (gra_udCurs != 0L) SetCursor(&(**gra_udCurs)); else
				SetCursor(&qd.arrow);
			break;
	}
	us_cursorstate = state;
}

/*
 * Routine to change the default cursor (to indicate modes).
 */
void setnormalcursor(INTBIG curs)
{
	us_normalcursor = curs;
}

/******************** LINE DRAWING ********************/

void screeninvertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_invertline(win, x1, y1, x2, y2);
}
void screendrawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc, INTBIG texture)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawline(win, x1, y1, x2, y2, desc, texture);
}

/******************** POLYGON DRAWING ********************/

/*
 * routine to draw a polygon in the arrays (x, y) with "count" points.
 */
void screendrawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawpolygon(win, x, y, count, desc);
}

/******************** BOX DRAWING ********************/

void screendrawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawbox(win, lowx, highx, lowy, highy, desc);
}

/*
 * routine to invert the bits in the box from (lowx, lowy) to (highx, highy)
 */
void screeninvertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy)
{
	Rect r;
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;

	r.left = lowx;                  r.right = highx + 1;
	r.bottom = wf->revy-lowy + 1;   r.top = wf->revy-highy;

	/* prepare for graphics */
	SetGWorld(wf->window, 0L);
	(void)LockPixels(wf->window->portPixMap);

	PenMode(patXor);
	PenPat((ConstPatternParam)&qd.black);
	PaintRect(&r);

	UnlockPixels(wf->window->portPixMap);
	SetGWorld(wf->realwindow, gra_origgdevh);

	gra_setrect(wf, lowx, highx + 1, wf->revy-highy, wf->revy-lowy + 1);
}

/*
 * routine to move bits on the display starting with the area at
 * (sx,sy) and ending at (dx,dy).  The size of the area to be
 * moved is "wid" by "hei".
 */
void screenmovebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei, INTBIG dx, INTBIG dy)
{
	Rect from, to;
	INTBIG xsize, ysize, x, y, dir, fromstart, frominc, tostart, toinc;
	REGISTER UCHAR1 *frombase, *tobase;
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;

	/* make sure the image buffer has not moved */
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	/* setup source rectangle */
	from.left = sx;
	from.right = from.left + wid;
	from.top = wf->revy + 1 - sy - hei;
	from.bottom = from.top + hei;

	/* setup destination rectangle */
	to.left = dx;
	to.right = to.left + wid;
	to.top = wf->revy + 1 - dy - hei;
	to.bottom = to.top + hei;

	/* determine size of bits to move */
	xsize = wid;   ysize = hei;

	/* determine direction of bit copy */
	if (from.left < to.left) dir = 1; else dir = 0;
	if (from.top < to.top)
	{
		fromstart = from.bottom-1;   frominc = -1;
		tostart = to.bottom-1;       toinc = -1;
	} else
	{
		fromstart = from.top;   frominc = 1;
		tostart = to.top;       toinc = 1;
	}

	/* move the bits */
	if (dir == 0)
	{
		/* normal forward copy in X */
		for(y = 0; y < ysize; y++)
		{
			frombase = wf->rowstart[fromstart] + from.left;
			fromstart += frominc;
			tobase = wf->rowstart[tostart] + to.left;
			tostart += toinc;
			for(x = 0; x < xsize; x++) *tobase++ = *frombase++;
		}
	} else
	{
		/* reverse copy in X */
		for(y = 0; y < ysize; y++)
		{
			frombase = wf->rowstart[fromstart] + from.right;
			fromstart += frominc;
			tobase = wf->rowstart[tostart] + to.right;
			tostart += toinc;
			for(x = 0; x < xsize; x++) *tobase-- = *frombase--;
		}
	}
	gra_setrect(wf, to.left, to.right, to.top, to.bottom);
}

/*
 * routine to save the contents of the box from "lx" to "hx" in X and from
 * "ly" to "hy" in Y.  A code is returned that identifies this box for
 * overwriting and restoring.  The routine returns -1 if there is a error.
 */
INTBIG screensavebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	SAVEDBOX *box;
	REGISTER INTBIG toindex, xsize, ysize;
	REGISTER INTBIG i, x, y, truelx, truehx;
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;

	/* make sure the image buffer has not moved */
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	truelx = lx;   truehx = hx;
	i = ly;   ly = wf->revy-hy;   hy = wf->revy-i;
	xsize = hx-lx+1;
	ysize = hy-ly+1;

	box = (SAVEDBOX *)emalloc((sizeof (SAVEDBOX)), us_tool->cluster);
	if (box == 0) return(-1);
	box->pix = (UCHAR1 *)emalloc(xsize * ysize, us_tool->cluster);
	if (box->pix == 0) return(-1);
	box->win = win;
	box->nextsavedbox = gra_firstsavedbox;
	gra_firstsavedbox = box;
	box->lx = lx;           box->hx = hx;
	box->ly = ly;           box->hy = hy;
	box->truelx = truelx;   box->truehx = truehx;

	/* move the bits */
	toindex = 0;
	for(y = ly; y <= hy; y++)
	{
		for(x = lx; x <= hx; x++)
			box->pix[toindex++] = wf->rowstart[y][x];
	}

	return((INTBIG)box);
}

/*
 * routine to shift the saved box "code" so that it is restored in a different location,
 * offset by (dx,dy)
 */
void screenmovesavedbox(INTBIG code, INTBIG dx, INTBIG dy)
{
	REGISTER SAVEDBOX *box;
	REGISTER WINDOWFRAME *wf;

	if (code == -1) return;
	box = (SAVEDBOX *)code;
	wf = box->win->frame;
	box->truelx += dx;   box->truehx += dx;
	box->lx += dx;       box->hx += dx;
	box->ly -= dy;       box->hy -= dy;
}

/*
 * routine to restore saved box "code" to the screen.  "destroy" is:
 *  0   restore box, do not free memory
 *  1   restore box, free memory
 * -1   free memory
 */
void screenrestorebox(INTBIG code, INTBIG destroy)
{
	REGISTER SAVEDBOX *box, *lbox, *tbox;
	REGISTER INTBIG fromindex;
	REGISTER INTBIG x, y;
	REGISTER WINDOWFRAME *wf;

	/* get the box */
	if (code == -1) return;
	box = (SAVEDBOX *)code;
	wf = box->win->frame;

	/* move the bits */
	if (destroy >= 0)
	{
		/* make sure the image buffer has not moved */
		if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
			gra_rebuildrowstart(wf);
		fromindex = 0;
		for(y = box->ly; y <= box->hy; y++)
			for(x = box->lx; x <= box->hx; x++)
				wf->rowstart[y][x] = box->pix[fromindex++];
		gra_setrect(wf, box->truelx, box->truehx+1, box->ly, box->hy+1);
	}

	/* destroy this box's memory if requested */
	if (destroy != 0)
	{
		lbox = NOSAVEDBOX;
		for(tbox = gra_firstsavedbox; tbox != NOSAVEDBOX; tbox = tbox->nextsavedbox)
		{
			if (tbox == box) break;
			lbox = tbox;
		}
		if (lbox == NOSAVEDBOX) gra_firstsavedbox = box->nextsavedbox; else
			lbox->nextsavedbox = box->nextsavedbox;
		efree((CHAR *)box->pix);
		efree((CHAR *)box);
	}
}

/******************** TEXT DRAWING ********************/

/*
 * Routine to find face with name "facename" and to return
 * its index in list of used fonts. If font was not used before,
 * then it is appended to list of used fonts.
 * If font "facename" is not available on the system, -1 is returned. 
 */
INTBIG screenfindface(CHAR *facename)
{
	INTBIG i;

	for (i = 0; i < gra_numfaces; i++)
		if (namesame(facename, gra_facelist[i]) == 0) return(i);
	return(-1);
}

/*
 * Routine to return the number of typefaces used (when "all" is FALSE)
 * or available on the system (when "all" is TRUE)
 * and to return their names in the array "list".
 * "screenfindface
 */
INTBIG screengetfacelist(CHAR ***list, BOOLEAN all)
{
	Handle f;
	INTBIG i, tot;
	short id;
	INTBIG typ, fonttype;
	CHAR line[256];

	if (gra_numfaces == 0)
	{
		fonttype = 'FOND';
		tot = CountResources(fonttype);
		if (tot == 0)
		{
			fonttype = 'FONT';
			tot = CountResources(fonttype);
		}
		gra_facelist = (CHAR **)emalloc((tot+1) * (sizeof (CHAR *)), us_tool->cluster);
		if (gra_facelist == 0) return(0);
		gra_faceid = (INTBIG *)emalloc((tot+1) * SIZEOFINTBIG, us_tool->cluster);
		if (gra_faceid == 0) return(0);
		(void)allocstring(&gra_facelist[0], _("DEFAULT FACE"), us_tool->cluster);
		gra_numfaces = 1;
		for(i=1; i<=tot; i++)
		{
			SetResLoad(0);
			f = GetIndResource(fonttype, i);
			GetResInfo(f, &id, (ResType *)&typ, (UCHAR1 *)line);
			SetResLoad(1);
			if (line[0] == 0) continue;
			GetFNum((UCHAR1 *)line, &id);
			line[line[0]+1] = 0;
			(void)allocstring(&gra_facelist[gra_numfaces], &line[1], us_tool->cluster);
			gra_faceid[gra_numfaces] = id;
			gra_numfaces++;
		}
		if (gra_numfaces > VTMAXFACE)
		{
			ttyputerr(_("Warning: found %ld fonts, but can only keep track of %d"), gra_numfaces,
				VTMAXFACE);
			gra_numfaces = VTMAXFACE;
		}
	}
	*list = gra_facelist;
	return(gra_numfaces);
}
/*
 * Routine to return the default typeface used on the screen.
 */
CHAR *screengetdefaultfacename(void)
{
	return(x_("Helvetica"));
}

INTBIG gra_textrotation = 0;

void screensettextinfo(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript)
{
	REGISTER INTBIG size, style, face;
	REGISTER INTBIG fontid;
	REGISTER WINDOWFRAME *wf;
	CHAR **facelist;

	gra_texttoosmall = FALSE;
	size = TDGETSIZE(descript);

	/* make sure the temp buffer exists */
	if (gra_textbuf == 0) gra_setuptextbuffer(100, 25);

	/* set sizes in this buffer */
	SetGWorld(gra_textbuf, 0L);
	if (size == TXTEDITOR)
	{
		TextFont(TFONT);
		TextSize(10);
		gra_textrotation = 0;
	} else if (size == TXTMENU)
	{
		TextFont(MFONT);
		TextSize(12);
		gra_textrotation = 0;
	} else
	{
		size = truefontsize(size, win, tech);
		if (size < 1)
		{
			gra_texttoosmall = TRUE;
			wf = win->frame;
			SetGWorld(wf->realwindow, gra_origgdevh);
			return;
		}
		if (size < MINIMUMTEXTSIZE) size = MINIMUMTEXTSIZE;
		TextSize(size);

		style = 0;
		if (TDGETITALIC(descript) != 0) style |= italic;
		if (TDGETBOLD(descript) != 0) style |= bold;
		if (TDGETUNDERLINE(descript) != 0) style |= underline;
		gra_textrotation = TDGETROTATION(descript);
		TextFace(style);
		
		face = TDGETFACE(descript);
		fontid = EFONT;
		if (face > 0)
		{
			if (gra_numfaces == 0) (void)screengetfacelist(&facelist, FALSE);
			if (face < gra_numfaces)
				fontid = gra_faceid[face];
		}
		TextFont(fontid);
	}
	GetFontInfo(&gra_curfontinfo);

	/* restore the world */
	wf = win->frame;
	SetGWorld(wf->realwindow, gra_origgdevh);
}

void screengettextsize(WINDOWPART *win, CHAR *str, INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG wid, hei, len;
	REGISTER WINDOWFRAME *wf;
	if (gra_texttoosmall)
	{
		*x = *y = 0;
		return;
	}

	/* make sure the temp buffer exists */
	if (gra_textbuf == 0) gra_setuptextbuffer(100, 25);

	/* determine text size in this buffer */
	SetGWorld(gra_textbuf, 0L);
	len = estrlen(str);
	wid = TextWidth(str, 0, len);
	hei = gra_curfontinfo.ascent + gra_curfontinfo.descent;
	switch (gra_textrotation)
	{
		case 0:			/* normal */
			*x = wid;
			*y = hei;
			break;
		case 1:			/* 90 degrees counterclockwise */
			*x = -hei;
			*y = wid;
			break;
		case 2:			/* 180 degrees */
			*x = -wid;
			*y = -hei;
			break;
		case 3:			/* 90 degrees clockwise */
			*x = hei;
			*y = -wid;
			break;
	}

	/* restore the world */
	wf = win->frame;
	SetGWorld(wf->realwindow, gra_origgdevh);
}

void screendrawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, CHAR *s, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	if (gra_texttoosmall) return;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawtext(win, atx, aty, gra_textrotation, s, desc);
}

BOOLEAN gettextbits(WINDOWPART *win, CHAR *msg, INTBIG *wid, INTBIG *hei, UCHAR1 ***rowstart)
{
	REGISTER INTBIG len;
	Rect sr;
	REGISTER WINDOWFRAME *wf;

	len = estrlen(msg);
	if (len == 0 || gra_texttoosmall)
	{
		*wid = *hei = 0;
		return(FALSE);
	}

	wf = win->frame;

	/* make sure the temp buffer exists */
	if (gra_textbuf == 0) gra_setuptextbuffer(100, 25);

	/* determine string size */
	SetGWorld(gra_textbuf, 0L);
	*wid = TextWidth(msg, 0, len);
	*hei = gra_curfontinfo.ascent + gra_curfontinfo.descent;

	/* make sure the temp buffer is big enough */
	gra_setuptextbuffer(*wid, *hei);

	/* write to the temp buffer */
	sr.left = 0;   sr.right = *wid;
	sr.top = 0;    sr.bottom = *hei;
	EraseRect(&sr);
	TextMode(srcCopy);
	MoveTo(0, gra_curfontinfo.ascent);
	DrawText(msg, 0, len);
	SetGWorld(wf->realwindow, gra_origgdevh);
	*rowstart = gra_textbufrowstart;
	SetFontLock(1);
	
	/* recache pointers if main window buffer moved */
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);
	return(FALSE);
}

void gra_setuptextbuffer(INTBIG width, INTBIG height)
{
	Rect r;
	REGISTER INTBIG i, bytes;
	REGISTER UCHAR1 *addr;

	if (width <= gra_textbufwid && height <= gra_textbufhei) return;

	r.left = 0;
	r.right = width;
	r.top = 0;
	r.bottom = height;
	if (gra_textbuf == 0)
	{
		NewGWorld(&gra_textbuf, 8, &r, 0L, 0L, 0);
		(void)LockPixels(gra_textbuf->portPixMap);
	} else
	{
		UpdateGWorld(&gra_textbuf, 8, &r, 0L, 0L, 0);
		(void)LockPixels(gra_textbuf->portPixMap);
		DisposePtr((Ptr)gra_textbufrowstart);
	}
	gra_textbufrowstart = (UCHAR1 **)NewPtr(height * (sizeof (UCHAR1 *)));
	bytes = (*gra_textbuf->portPixMap)->rowBytes & 0x7FFF;
	addr = (UCHAR1 *)GetPixBaseAddr(gra_textbuf->portPixMap);
	for(i=0; i<height; i++)
	{
		gra_textbufrowstart[i] = addr;
		addr += bytes;
	}
	gra_textbufwid = width;   gra_textbufhei = height;
}

/******************** CIRCLE DRAWING ********************/

void screendrawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawcircle(win, atx, aty, radius, desc);
}

void screendrawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius,
	GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawthickcircle(win, atx, aty, radius, desc);
}

/******************** DISC DRAWING ********************/

/*
 * routine to draw a filled-in circle at (atx,aty) with radius "radius"
 */
void screendrawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawdisc(win, atx, aty, radius, desc);
}

/******************** ARC DRAWING ********************/

/*
 * draws a thin arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1,
	INTBIG x2, INTBIG y2, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawcirclearc(win, centerx, centery, x1, y1, x2, y2, FALSE, desc);
}

/*
 * draws a thick arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawthickcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1,
	INTBIG x2, INTBIG y2, GRAPHICS *desc)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawcirclearc(win, centerx, centery, x1, y1, x2, y2, TRUE, desc);
}

/******************** GRID DRAWING ********************/

/*
 * grid drawing routine
 */
void screendrawgrid(WINDOWPART *win, POLYGON *obj)
{
	REGISTER WINDOWFRAME *wf;

	/* make sure the image buffer has not moved */
	wf = win->frame;
	if ((UCHAR1 *)GetPixBaseAddr(wf->window->portPixMap) != wf->rowstart[0])
		gra_rebuildrowstart(wf);

	gra_drawgrid(win, obj);
}

/******************** MOUSE CONTROL ********************/

/*
 * routine to return the number of buttons on the mouse
 */
INTBIG buttoncount(void)
{
	return(mini(BUTTONS, NUMBUTS));
}

/*
 * routine to tell whether button "but" is a double-click
 */
BOOLEAN doublebutton(INTBIG b)
{
	if (b == 16) return(TRUE);
	return(FALSE);
}

/*
 * routine to tell whether button "but" is a context button (right)
 */
BOOLEAN contextbutton(INTBIG b)
{
	if ((b%4) >= 2) return(TRUE);
	return(FALSE);
}

/*
 * routine to tell whether button "but" has the "shift" key held
 */
BOOLEAN shiftbutton(INTBIG b)
{
	if ((b%2) == 1) return(TRUE);
	return(FALSE);
}

/*
 * routine to tell whether button "but" is a "mouse wheel" button
 */
BOOLEAN wheelbutton(INTBIG b)
{
	return(FALSE);
}

/*
 * routine to return the name of button "b" (from 0 to "buttoncount()").
 * The number of letters unique to the button is placed in "important".
 */
CHAR *buttonname(INTBIG b, INTBIG *important)
{
	*important = gra_buttonname[b].unique;
	return(gra_buttonname[b].name);
}

/*
 * routine to convert from "gra_inputstate" (the typical input parameter)
 * to button numbers (the table "gra_buttonname")
 */
INTBIG gra_makebutton(INTBIG state)
{
	INTBIG base;

	if ((state&DOUBLECLICK) != 0) return(16);
	base = 0;
	if ((state&CONTROLISDOWN) != 0) base += 8;
	if ((state&OPTIONISDOWN) != 0) base += 4;
	if ((state&COMMANDISDOWN) != 0) base += 2;
	if ((state&SHIFTISDOWN) != 0) base++;
	return(base);
}

/*
 * routine to wait for a button push and return its index (0 based) in "*but".
 * The coordinates of the cursor are placed in "*x" and "*y".  If there is no
 * button push, the value of "*but" is negative.
 */
void waitforbutton(INTBIG *x, INTBIG *y, INTBIG *but)
{
	EventRecord theEvent;

	if (gra_inputstate != NOEVENT && (gra_inputstate&(ISBUTTON|BUTTONUP)) == ISBUTTON)
	{
		*but = gra_makebutton(gra_inputstate);
		*x = gra_cursorx;
		*y = gra_cursory;
		gra_inputstate = NOEVENT;
		if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(NULLCURSOR);
		return;
	}

	gra_waitforaction(1, &theEvent);

	if (gra_inputstate != NOEVENT && (gra_inputstate&(ISBUTTON|BUTTONUP)) == ISBUTTON)
	{
		*but = gra_makebutton(gra_inputstate);
		*x = gra_cursorx;
		*y = gra_cursory;
		gra_inputstate = NOEVENT;
		if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(NULLCURSOR);
		return;
	}
	*but = -1;
}

/*
 * routine to do modal loop, calling "charhandler" for each typed key and "buttonhandler"
 * for each pushed button. The "charhandler" routine is called with the character value
 * that was typed. The "buttonhandler" routine is called with coordinates of cursor.
 * The "charhandler" and "buttonhandler" returns true to abort loop.
 * The value of "cursor" determines cursor appearance.
 */
void modalloop(BOOLEAN (*charhandler)(INTSML chr, INTBIG special),
			   BOOLEAN (*buttonhandler)(INTBIG x, INTBIG y, INTBIG but), INTBIG cursor)
{
	INTBIG oldnormalcursor, chr, x, y, but;
	INTBIG special;

	/* set cursor */
	oldnormalcursor = us_normalcursor;
	setnormalcursor(cursor);

	for (;;)
	{
		if (ttydataready())
		{
			chr = getnxtchar(&special);
			if ((*charhandler)(chr, special)) break;
		} else
		{
			waitforbutton(&x, &y, &but);
			if (but >= 0 && buttonhandler != 0)
				if ((*buttonhandler)(x, y, but)) break;
		}
	}

	/* restore cursor */
	setnormalcursor(oldnormalcursor);
}

/*
 * routine to track the cursor until a button is released, calling "whileup" for
 * each co-ordinate when the mouse moves before the first button push, calling
 * "whendown" once when the button goes down, calling "eachdown" for each
 * co-ordinate when the mouse moves after the button is pushed, calling
 * "eachchar" for each key that is typed at any time, and calling "done" once
 * when done.  The "whendown" and "done" routines are called with no parameters;
 * "whileup" and "eachdown" are called with the X and Y coordinates of the
 * cursor; and "eachchar" is called with the X, Y, and character value that was
 * typed.  The "whileup", "eachdown", and "eachchar" routines return nonzero to
 * abort tracking.
 * If "waitforpush" is true then the routine will wait for a button to
 * actually be pushed before tracking (otherwise it will begin tracking
 * immediately).  The value of "purpose" determines what the cursor will look
 * like during dragging: 0 for normal (the standard cursor), 1 for drawing (a pen),
 * 2 for dragging (a hand), 3 for popup menu selection (a horizontal arrow), 4 for
 * hierarchical popup menu selection (arrow, stays at end).
 */
void trackcursor(BOOLEAN waitforpush, BOOLEAN (*whileup)(INTBIG, INTBIG), void (*whendown)(void),
	BOOLEAN (*eachdown)(INTBIG, INTBIG), BOOLEAN (*eachchar)(INTBIG, INTBIG, INTSML), void (*done)(void),
		INTBIG purpose)
{
	REGISTER INTBIG action;
	REGISTER BOOLEAN keepon;
	EventRecord theEvent;
	/* change the cursor to an appropriate icon */
	switch (purpose)
	{
		case TRACKDRAWING:    setdefaultcursortype(PENCURSOR);    break;
		case TRACKDRAGGING:   setdefaultcursortype(HANDCURSOR);   break;
		case TRACKSELECTING:
		case TRACKHSELECTING: setdefaultcursortype(MENUCURSOR);   break;
	}

	/* now wait for a button to go down, if requested */
	keepon = FALSE;
	if (waitforpush)
	{
		while (!keepon)
		{
			gra_waitforaction(2, &theEvent);
			if (gra_inputstate == NOEVENT) continue;
			action = gra_inputstate;
			gra_inputstate = NOEVENT;

			/* if button just went down, stop this loop */
			if ((action&ISBUTTON) != 0 && (action&BUTTONUP) == 0) break;
			if ((action&MOTION) != 0)
			{
				keepon = (*whileup)(gra_cursorx, gra_cursory);
			} else if ((action&ISKEYSTROKE) != 0 && gra_inputspecial == 0)
			{
				keepon = (*eachchar)(gra_cursorx, gra_cursory, (INTSML)(action&CHARREAD));
			}
			if (el_pleasestop != 0) keepon = TRUE;
		}
	}

	/* button is now down, real tracking begins */
	if (!keepon)
	{
		(*whendown)();
		keepon = (*eachdown)(gra_cursorx, gra_cursory);
	}

	/* now track while the button is down */
	while (!keepon)
	{
		gra_waitforaction(2, &theEvent);
		us_endchanges(NOWINDOWPART);

		/* for each motion, report the coordinates */
		if (gra_inputstate == NOEVENT) continue;
		action = gra_inputstate;
		gra_inputstate = NOEVENT;
		if ((action&ISBUTTON) != 0 && (action&BUTTONUP) != 0) break;
		if ((action&MOTION) != 0)
		{
			keepon = (*eachdown)(gra_cursorx, gra_cursory);
		} else if ((action&ISKEYSTROKE) != 0 && gra_inputspecial == 0)
		{
			keepon = (*eachchar)(gra_cursorx, gra_cursory, (INTSML)(action&CHARREAD));
		}
		if (el_pleasestop != 0) keepon = TRUE;
	}

	/* inform the user that all is done */
	(*done)();

	/* restore the state of the world */
	if (purpose != TRACKHSELECTING) setdefaultcursortype(NULLCURSOR);
}

/*
 * routine to read the current co-ordinates of the tablet and return them
 * in "*x" and "*y".
 */
void readtablet(INTBIG *x, INTBIG *y)
{
	*x = gra_cursorx;   *y = gra_cursory;
}

/*
 * routine to turn off the cursor tracking if it is on
 */
void stoptablet(void)
{
	if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(NULLCURSOR);
}

/******************** KEYBOARD CONTROL ********************/

/*
 * routine to get the next character from the keyboard
 */
INTSML getnxtchar(INTBIG *special)
{
	REGISTER INTSML i;
	EventRecord theEvent;

	if (gra_inputstate != NOEVENT && (gra_inputstate&ISKEYSTROKE) != 0)
	{
		i = gra_inputstate & CHARREAD;
		*special = gra_inputspecial;
		gra_inputstate = NOEVENT;
		return(i);
	}
	if (us_cursorstate != IBEAMCURSOR)
		setdefaultcursortype(WANTTTYCURSOR);
	for(;;)
	{
		gra_waitforaction(0, &theEvent);
		if (gra_inputstate != NOEVENT && (gra_inputstate&ISKEYSTROKE) != 0)
			break;
	}
	i = gra_inputstate & CHARREAD;
	*special = gra_inputspecial;
	gra_inputstate = NOEVENT;
	if (us_cursorstate != IBEAMCURSOR)
		setdefaultcursortype(NULLCURSOR);
	return(i);
}

void checkforinterrupt(void)
{
	EventRecord theEvent;
	short oak;

	if (el_pleasestop == 0 && gra_cancheck)
	{
		gra_cancheck = FALSE;
		oak = EventAvail(keyDownMask, &theEvent);
		if (oak != 0)
		{
			(void)GetNextEvent(keyDownMask, &theEvent);
			if (theEvent.what == keyDown)
				if ((theEvent.modifiers & cmdKey) != 0)
					if ((theEvent.message & charCodeMask) == '.')
			{
				ttyputmsg(_("Interrupted..."));
				el_pleasestop = 1;
			}
		}
	}
}

/*
 * Routine to return which "bucky bits" are held down (shift, control, etc.)
 */
#define KEYPRESSED(k) ((thekeys[k>>3] >> (k&7)) & 1)
INTBIG getbuckybits(void)
{
	UCHAR1 thekeys[16];
	REGISTER INTBIG bits;
	
	GetKeys((unsigned long *)thekeys);
	bits = 0;
	if (KEYPRESSED(0x38) != 0) bits |= SHIFTDOWN;
	if (KEYPRESSED(0x3B) != 0) bits |= CONTROLDOWN;
	if (KEYPRESSED(0x3A) != 0) bits |= OPTALTMETDOWN;
	if (KEYPRESSED(0x37) != 0) bits |= ACCELERATORDOWN;
	return(bits);
}

/*
 * routine to tell whether data is waiting at the terminal.  Returns true
 * if data is ready.
 */
BOOLEAN ttydataready(void)
{
	EventRecord theEvent;

	/* see if something is already pending */
	if (gra_inputstate != NOEVENT)
	{
		if ((gra_inputstate&ISKEYSTROKE) != 0) return(TRUE);
		return(FALSE);
	}

	/* wait for something and analyze it */
	gra_waitforaction(1, &theEvent);
	if (gra_inputstate != NOEVENT && (gra_inputstate&ISKEYSTROKE) != 0) return(TRUE);
	return(FALSE);
}

/****************************** FILES ******************************/

/*
 * Routine to set the type and creator of file "name" to "type" and "creator"
 */
void mac_settypecreator(CHAR *name, INTBIG type, INTBIG creator)
{
	FInfo finfo;
	CHAR pname[256];

	(void)estrcpy(&pname[1], name);
	pname[0] = estrlen(name);
	(void)GetFInfo((UCHAR1 *)pname, 0, &finfo);
	finfo.fdType = type;
	finfo.fdCreator = creator;
	(void)SetFInfo((UCHAR1 *)pname, 0, &finfo);
}

/*
 * Macintosh routine to return the name of the system folder
 */
CHAR *gra_systemfoldername(void)
{
	static CHAR foldername[256];
	CHAR thisname[256];
	INTBIG i, j, len;
	WDPBRec wpb;
	CInfoPBRec cpb;
	SysEnvRec sysenv;

	/* get system folder reference number */
	SysEnvirons(2, &sysenv);

	/* determine directory ID of system folder */
	wpb.ioNamePtr = (UCHAR1 *)foldername;
	wpb.ioVRefNum = sysenv.sysVRefNum;
	wpb.ioWDIndex = 0;
	wpb.ioWDProcID = 0L;
	wpb.ioWDVRefNum = wpb.ioVRefNum;
	i = PBGetWDInfo(&wpb, 0);
	if (i != noErr) return(x_(""));

	/* find the name of the system folder */
	foldername[0] = 0;
	cpb.dirInfo.ioDrParID = wpb.ioWDDirID;
	for(j=0; ; j++)
	{
		cpb.dirInfo.ioNamePtr = (UCHAR1 *)thisname;
		cpb.dirInfo.ioVRefNum = wpb.ioWDVRefNum;
		cpb.dirInfo.ioFDirIndex = -1;
		cpb.dirInfo.ioDrDirID = cpb.dirInfo.ioDrParID;
		i = PBGetCatInfo(&cpb, 0);
		if (i != noErr) return(x_(""));
		len = thisname[0] + 1;
		if (j != 0) thisname[len++] = ':';
		for(i=0; i<foldername[0]; i++) thisname[len++] = foldername[i+1];
		thisname[len] = 0;
		for(i=0; i<len; i++) foldername[i+1] = thisname[i+1];
		foldername[0] = len-1;
		if (cpb.dirInfo.ioDrDirID == fsRtDirID) break;
	}

	/* see if there is a folder with the name ":Preferences:Electric Files" added on */
	for(i=0; i<foldername[0]; i++) thisname[i+1] = foldername[i+1];
	thisname[foldername[0]+1] = 0;
	(void)estrcat(&thisname[1], x_(":Preferences:Electric Files"));
	thisname[0] = estrlen(&thisname[1]);
	cpb.dirInfo.ioNamePtr = (UCHAR1 *)thisname;
	cpb.dirInfo.ioVRefNum = wpb.ioWDVRefNum;
	cpb.dirInfo.ioFDirIndex = 0;
	i = PBGetCatInfo(&cpb, 0);
	if (i != noErr) return(&foldername[1]);
	if ((cpb.dirInfo.ioFlAttrib&16) == 0) return(&foldername[1]);
	(void)estrcpy(foldername, &thisname[1]);
	return(foldername);
}

/*
 * Routine to convert a Pascal string file name in "thisname" and its volume
 * reference number in "refnum" into a full path name (and a C string).
 */
CHAR *gra_makefullname(CHAR *thisname, INTBIG refnum)
{
	INTBIG err, len, i;
	CInfoPBRec cpb;
	CHAR line[256];
	static CHAR sofar[256];

	len = thisname[0];
	for(i=0; i<len; i++) sofar[i] = thisname[i+1];
	sofar[len] = 0;
	cpb.hFileInfo.ioVRefNum = refnum;
	cpb.hFileInfo.ioDirID = 0;
	cpb.hFileInfo.ioCompletion = 0L;
	cpb.hFileInfo.ioNamePtr = (StringPtr)line;
	cpb.hFileInfo.ioFDirIndex = -1;
	for(;;)
	{
		err = PBGetCatInfo(&cpb, 0);
		if (err != noErr) break;
		line[line[0]+1] = 0;
		estrcat(line, x_(":"));
		estrcat(line, sofar);
		estrcpy(sofar, &line[1]);
		if (cpb.hFileInfo.ioFlParID == 0) break;
		cpb.hFileInfo.ioDirID = cpb.hFileInfo.ioFlParID;
	}
	return(sofar);
}

/*
 * Routine to convert a FSS specification in "theFSS" into a full path name
 * (and a C string).
 */
CHAR *gra_makefullnamefromFSS(FSSpec theFSS)
{
	DirInfo block;
	short len;
	static CHAR fileName[256];
	CHAR dirName[256];
	OSErr err;

	if (theFSS.parID != 0)
	{
		theFSS.name[theFSS.name[0]+1] = 0;
		estrcpy(fileName, (CHAR *)&theFSS.name[1]);
		block.ioDrParID = theFSS.parID;
		block.ioNamePtr = (StringPtr)dirName;
		do {
			block.ioVRefNum = theFSS.vRefNum;
			block.ioFDirIndex = -1;
			block.ioDrDirID = block.ioDrParID;
			err = PBGetCatInfo((CInfoPBPtr)&block, 0);
			dirName[dirName[0]+1] = 0;
			len = estrlen(&dirName[1]);
			BlockMove(fileName, fileName + len+1, estrlen(fileName)+1);
			estrcpy(fileName, &dirName[1]);
			fileName[len] = ':';
		} while (block.ioDrDirID != 2);
		return(fileName);
	}
	/* no directory ID specified in FSS, use old name/vrefnum method */
	return(gra_makefullname((CHAR *)theFSS.name, theFSS.vRefNum));
}

/*
 * File filter for binary library files.  Accepts "Elec" type files or those
 * that end with ".elib".
 */
pascal Boolean gra_fileFilterProc(CInfoPBPtr pb, Ptr mydata)
{
	INTBIG pos;
	UCHAR1 *str;

	if ((pb->hFileInfo.ioFlAttrib&ioDirMask) != 0) return(0);
	str = pb->hFileInfo.ioNamePtr;
	pos = str[0];
	if (str[pos-4] == '.' && str[pos-3] == 'e' && str[pos-2] == 'l' && str[pos-1] == 'i' &&
		str[pos] == 'b') return(0);
	if (pb->hFileInfo.ioFlFndrInfo.fdType == 'Elec') return(0);
	return(1);
}

/*
 * Routine to prompt for multiple files of type "filetype", giving the
 * message "msg".  Returns a string that contains all of the file names,
 * separated by the NONFILECH (a character that cannot be in a file name).
 */
CHAR *multifileselectin(CHAR *msg, INTBIG filetype)
{
	return(fileselect(msg, filetype, x_("")));
}

/*
 * Routine to display a standard file prompt dialog and return the selected file.
 * The prompt message is in "msg" and the kind of file is in "filetype".  The default
 * output file name is in "defofile" (only used if "filetype" is negative).
 */
CHAR *fileselect(CHAR *msg, INTBIG filetype, CHAR *defofile)
{
	SFTypeList myTypes;
	static Point SFwhere = {90, 82};
	INTBIG len;
	REGISTER INTBIG i;
	CHAR save;
	StandardFileReply reply;
	WindowPtr savewin;
	WDPBRec wdpb;
	CHAR leng, ofile[256], prompt[256];

	if (us_logplay != NULL)
	{
		(void)xfread((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logplay);
		if ((gra_action.kind&0xFFFF) != FILEREPLY) gra_localstring[0] = 0; else
		{
			(void)xfread((UCHAR1 *)&leng, 1, 1, us_logplay);
			len = leng;
			(void)xfread((UCHAR1 *)gra_localstring, 1, len, us_logplay);
			gra_localstring[len] = 0;
		}
	} else
	{
		estrcpy(&prompt[1], msg);
		prompt[0] = estrlen(msg);
		GetPort(&savewin);
		if ((filetype&FILETYPEWRITE) == 0)
		{
			/* input file selection */
			/* ParamText((UCHAR1 *)prompt, "\p", "\p", "\p"); */
			if ((filetype&FILETYPE) == io_filetypeblib)
			{
				/* special case for Electric binary libraries */
				/* SFGetFile(SFwhere, prompt, gra_fileFilterProcUPP, -1, myTypes,
					0, &sfreply); */
				StandardGetFile(gra_fileFilterProcUPP, -1, myTypes, &reply);
			} else
			{
				/* standard file input */
				/* SFGetFile(SFwhere, prompt, 0, -1, myTypes, 0, &sfreply); */
				StandardGetFile(0, -1, myTypes, &reply);
			}
		} else
		{
			/* output file selection */
			for(i = estrlen(defofile)-1; i > 0; i--)
				if (defofile[i] == ':') break;
			if (i > 0)
			{
				/* there is a ":" in the path, set the default directory */
				i++;
				save = defofile[i];
				defofile[i] = 0;
				(void)estrcpy(&ofile[1], defofile);
				ofile[0] = estrlen(defofile);
				wdpb.ioNamePtr = (StringPtr)ofile;
				PBHSetVol(&wdpb, 0);
				defofile[i] = save;
				defofile = &defofile[i];
			}
			(void)estrcpy(&ofile[1], defofile);
			ofile[0] = estrlen(defofile);
			StandardPutFile((UCHAR1 *)prompt, (UCHAR1 *)ofile, &reply);
		}
		SetPort(savewin);
		if (reply.sfGood == 0) gra_localstring[0] = 0; else
			(void)estrcpy(gra_localstring, gra_makefullnamefromFSS(reply.sfFile));
	}
	/* log this result if logging */
	if (us_logrecord != NULL)
	{
		gra_action.kind = FILEREPLY;
		(void)xfwrite((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logrecord);
		leng = estrlen(gra_localstring);
		(void)xfwrite((UCHAR1 *)&leng, 1, 1, us_logrecord);
		len = leng;
		(void)xfwrite((UCHAR1 *)gra_localstring, 1, len, us_logrecord);
		gra_logrecordindex++;
		if (gra_logrecordindex >= us_logflushfreq)
		{
			/* flush the session log file */
			gra_logrecordindex = 0;
			xflushbuf(us_logrecord);
		}
	}
	return(gra_localstring);
}

/*
 * routine to handle an "open document" Apple Event and read a library
 */
pascal OSErr gra_handleodoc(AppleEvent *theAppleEvent, AppleEvent *reply, long handlerRefCon)
{
	FSSpec myFSS;
	AEDescList docList;
	OSErr err;
	CHAR *argv[3];
	long sindex, itemsInList;
	Size actualSize;
	AEKeyword keywd;
	DescType returnedType;

	/* get the direct parameter, a descriptor list, and put it into a doclist */
	err = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList, &docList);
	if (err) return(err);

	/* check for missing parameters */
	err = AEGetAttributePtr(theAppleEvent, keyMissedKeywordAttr, typeWildCard,
		&returnedType, nil, 0, &actualSize);
	if (err != errAEDescNotFound)
	{
		if (!err) return(errAEEventNotHandled);
		return(err);
	}

	/* count the number of descriptor records in the list */
	err = AECountItems(&docList, &itemsInList);

	/*
	 * now get each descriptor record from the list, coerce the returned data to
	 * an FSSpec record, and open the associated file
	 */
	for(sindex = 1; sindex <= itemsInList; sindex++)
	{
		err = AEGetNthPtr(&docList, sindex, typeFSS, &keywd, &returnedType,
			(Ptr)&myFSS, sizeof(myFSS), &actualSize);
		if (err) return(err);
		us_beginchanges();
		argv[0] = x_("read");
		argv[1] = gra_makefullnamefromFSS(myFSS);
		argv[2] = x_("make-current");
		us_library(3, argv);
		if (el_curwindowpart != NOWINDOWPART)
			us_ensurepropertechnology(el_curwindowpart->curnodeproto, 0, TRUE);
		us_endchanges(NOWINDOWPART);		
	}
	err = AEDisposeDesc(&docList);

	return(noErr);
}

/*
 * Helper routine to initialize the list of files in a directory.
 */
void gra_initfilelist(void)
{
	if (gra_fileliststringarray == 0)
	{
		gra_fileliststringarray = newstringarray(db_cluster);
		if (gra_fileliststringarray == 0) return;
	}
	clearstrings(gra_fileliststringarray);
}

/*
 * Helper routine to add "file" to the list of files in a directory.
 * Returns true on error.
 */
BOOLEAN gra_addfiletolist(CHAR *file)
{
	addtostringarray(gra_fileliststringarray, file);
	return(FALSE);
}

/*
 * Routine to search for all of the files/directories in directory "directory" and
 * return them in the array of strings "filelist".  Returns the number of files found.
 */
INTBIG filesindirectory(CHAR *directory, CHAR ***filelist)
{
	INTBIG err, i;
	INTBIG dirid, len;
	CHAR file[256];
	CInfoPBRec cinfo;

	if (*directory == 0) dirid = 0; else
	{
		(void)estrcpy(&file[1], directory);
		file[0] = estrlen(directory);
		SetVol((UCHAR1 *)file, 0);
		cinfo.hFileInfo.ioCompletion = 0L;
		cinfo.hFileInfo.ioNamePtr = (StringPtr)file;
		cinfo.hFileInfo.ioVRefNum = 0;
		cinfo.hFileInfo.ioFDirIndex = 0;
		cinfo.hFileInfo.ioDirID = 0L;
		err = PBGetCatInfo(&cinfo, 0);
		if (err != noErr) return(0);
		dirid = cinfo.hFileInfo.ioDirID;
	}

	gra_initfilelist();
	for(i=1; ; i++)
	{
		cinfo.hFileInfo.ioCompletion = 0L;
		cinfo.hFileInfo.ioNamePtr = (StringPtr)file;
		cinfo.hFileInfo.ioVRefNum = 0;
		cinfo.hFileInfo.ioFDirIndex = i;
		cinfo.hFileInfo.ioDirID = dirid;
		err = PBGetCatInfo(&cinfo, 0);
		if (err != noErr) break;
		file[file[0]+1] = 0;
		if (gra_addfiletolist(&file[1])) return(0);
	}

	*filelist = getstringarray(gra_fileliststringarray, &len);
	return(len);
}

/* routine to convert a path name with "~" to a real path */
CHAR *truepath(CHAR *line)
{
	/* only have tilde parsing on UNIX */
	return(line);
}

/*
 * Routine to return the full path to file "file".
 */
CHAR *fullfilename(CHAR *file)
{
	REGISTER CHAR *pt;
	static CHAR fullfile[256];

	/* see if there is a ':' anywhere in the file name */
	for(pt = file; *pt != 0; pt++)
		if (*pt == ':') break;

	/* if it has a colon but does not begin with one, it is a full path already */
	if (*pt == ':' && *file != ':') return(file);

	/* make it a full path */
	estrcpy(fullfile, currentdirectory());
	estrcat(fullfile, file);
	return(fullfile);
}

/*
 * routine to rename file "file" to "newfile"
 * returns nonzero on error
 */
INTBIG erename(CHAR *file, CHAR *newfile)
{
	CHAR oldname[256], newname[256];
	
	estrcpy(&oldname[1], file);
	oldname[0] = estrlen(file);
	estrcpy(&newname[1], newfile);
	newname[0] = estrlen(newfile);
	if (Rename((UCHAR1 *)oldname, 0, (UCHAR1 *)newname) == 0) return(0);
	return(1);
}

/*
 * routine to delete file "file"
 */
INTBIG eunlink(CHAR *file)
{
	CHAR filename[256];
	
	estrcpy(&filename[1], file);
	filename[0] = estrlen(file);
	if (HDelete(0, 0, (UCHAR1 *)filename) == 0) return(0);
	return(-1);
}

/*
 * Routine to return information about the file or directory "name":
 *  0: does not exist
 *  1: is a file
 *  2: is a directory
 *  3: is a locked file (read-only)
 */
INTBIG fileexistence(CHAR *name)
{
	struct stat buf;
	Str255 fname;
	ParamBlockRec pb;

	if (stat(name, &buf) < 0) return(0);
	if ((buf.st_mode & S_IFMT) == S_IFDIR) return(2);

	/* a file: see if it is writable */
	estrcpy((CHAR *)&fname[1], name);
	fname[0] = estrlen(name);
	pb.fileParam.ioNamePtr = fname;	
	pb.fileParam.ioVRefNum = 0;
	pb.fileParam.ioFVersNum = 0;
	pb.fileParam.ioFDirIndex = 0;
	if (PBGetFInfo(&pb, 0) != 0) return(1);
	if ((pb.fileParam.ioFlAttrib&1) == 0) return(1);
	return(3);
}

/*
 * Routine to create a directory.
 * Returns true on error.
 */
BOOLEAN createdirectory(CHAR *dirname)
{
	FSSpec fsp;
	long dir;
	INTBIG err;
	CHAR pname[256];

	estrcpy(&pname[1], dirname);
	pname[0] = estrlen(dirname);
	err = FSMakeFSSpec(0, 0, (UCHAR1 *)pname, &fsp);
	if (err != noErr && err != fnfErr) return(TRUE);
	err = FSpDirCreate(&fsp, smSystemScript, &dir);
	if (err != noErr && err != dupFNErr) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return the current directory name
 */
CHAR *currentdirectory(void)
{
	return(gra_makefullname((CHAR *)x_("\p"), 0));
}

/*
 * Routine to return the home directory (returns 0 if it doesn't exist)
 */
CHAR *hashomedir(void)
{
	return(0);
}

/*
 * Routine to return the path to the "options" library.
 */
CHAR *optionsfilepath(void)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, x_("electricoptions.elib"));
	return(returninfstr(infstr));
}

/*
 * Routine to obtain the modification date on file "filename".
 */
time_t filedate(CHAR *filename)
{
	struct stat buf;
	time_t thetime;

	stat(filename, &buf);
	thetime = buf.st_mtime;
	thetime -= machinetimeoffset();
	return(thetime);
}

/*
 * Routine to lock a resource called "lockfilename" by creating such a file
 * if it doesn't exist.  Returns true if successful, false if unable to
 * lock the file.
 */
BOOLEAN lockfile(CHAR *lockfilename)
{
	INTBIG err;
	FSSpec fsp;
	CHAR pname[256];

	estrcpy(&pname[1], lockfilename);
	pname[0] = estrlen(lockfilename);
	err = FSMakeFSSpec(0, 0, (UCHAR1 *)pname, &fsp);
	err = FSpCreate(&fsp, 'Elec', 'Lock', smSystemScript);
	if (err == noErr) return(TRUE);
	return(FALSE);
}

/*
 * Routine to unlock a resource called "lockfilename" by deleting such a file.
 */
void unlockfile(CHAR *lockfilename)
{
	FSSpec fsp;
	CHAR pname[256];

	estrcpy(&pname[1], lockfilename);
	pname[0] = estrlen(lockfilename);
	FSMakeFSSpec(0, 0, (UCHAR1 *)pname, &fsp);
	FSpDelete(&fsp);
}

/*
 * Routine to show file "document" in a browser window.
 * Returns true if the operation cannot be done.
 *
 * This routine is taken from "FinderOpenSel" by:
 * C.K. Haun <TR>
 * Apple Developer Tech Support
 * April 1992, Cupertino, CA USA
 * Of course, Copyright 1991-1992, Apple Computer Inc.
 */
BOOLEAN browsefile(CHAR *document)
{
	BOOLEAN launch;
	AppleEvent aeEvent, aeReply;
	AEDesc aeDirDesc, listElem;
	FSSpec dirSpec, procSpec;
	AEDesc fileList;
	OSErr myErr;
	ProcessSerialNumber process;
	AliasHandle DirAlias, FileAlias;
	FSSpec theFileToOpen;
	ProcessInfoRec infoRec;
	Str31 processName;
	Str255 pname;
	static AEDesc myAddressDesc;
	static BOOLEAN gotFinderAddress = FALSE;

	/* get the FSSpec of the file */
	launch = TRUE;
	estrcpy((CHAR *)&pname[1], document);
	pname[0] = estrlen(document);
	FSMakeFSSpec(0, 0, pname, &theFileToOpen);

	/* we are working locally, find the finder on this machine */
	if (!gotFinderAddress)
	{
		infoRec.processInfoLength = sizeof(ProcessInfoRec);
		infoRec.processName = (StringPtr)&processName;
		infoRec.processAppSpec = &procSpec;
		myErr = gra_findprocess('FNDR', 'MACS', &process, &infoRec);
		if (myErr != noErr) return(TRUE);
		myErr = AECreateDesc(typeProcessSerialNumber, (Ptr)&process, sizeof(process), &myAddressDesc);
		if (myErr != noErr) return(TRUE);
		gotFinderAddress = TRUE;
	}

	/* Create the FinderEvent */
	if (launch)
	{
		myErr = AECreateAppleEvent('FNDR', 'sope', &myAddressDesc,
			kAutoGenerateReturnID, kAnyTransactionID, &aeEvent);
	} else
	{
		myErr = AECreateAppleEvent('FNDR', 'srev', &myAddressDesc,
			kAutoGenerateReturnID, kAnyTransactionID, &aeEvent);
	}
	if (myErr != noErr) return(TRUE);

	/*
	 * Now we build all the bits of an OpenSelection event.  Basically, we need to create
	 * an alias for the item to open, and an alias to the parent folder (directory) of that
	 * item.  We can also pass a list of files if we want.
	 */

	/* make a spec for the parent folder */
	FSMakeFSSpec(theFileToOpen.vRefNum, theFileToOpen.parID, nil, &dirSpec);
	NewAlias(nil, &dirSpec, &DirAlias);

	/* Create alias for file */
	NewAlias(nil, &theFileToOpen, &FileAlias);

	/* Create the file list */
	myErr = AECreateList(nil, 0, false, &fileList);

	/* create the folder descriptor */
	HLock((Handle)DirAlias);
	AECreateDesc(typeAlias, (Ptr)*DirAlias, GetHandleSize((Handle)DirAlias), &aeDirDesc);
	HUnlock((Handle)DirAlias);
	if ((myErr = AEPutParamDesc(&aeEvent, keyDirectObject, &aeDirDesc)) == noErr)
	{
		/* done with the desc, kill it */
		AEDisposeDesc(&aeDirDesc);

		/* create the file descriptor and add to aliasList */
		HLock((Handle)FileAlias);
		AECreateDesc(typeAlias, (Ptr)*FileAlias, GetHandleSize((Handle)FileAlias), &listElem);
		HLock((Handle)FileAlias);
		myErr = AEPutDesc(&fileList, 0, &listElem);
	}
	if (myErr == noErr)
	{
		AEDisposeDesc(&listElem);

		/* Add the file alias list to the event */
		myErr = AEPutParamDesc(&aeEvent, 'fsel', &fileList);
		AEDisposeDesc(&fileList);

		if (myErr == noErr)
			myErr = AESend(&aeEvent, &aeReply, kAENoReply + kAEAlwaysInteract + kAECanSwitchLayer,
				kAENormalPriority, kAEDefaultTimeout, nil, nil);
		if (!launch)
			SetFrontProcess(&process);
	}
	AEDisposeDesc(&aeEvent);

	if ((Handle)DirAlias)
		DisposeHandle((Handle)DirAlias);
	if ((Handle)FileAlias)
		DisposeHandle((Handle)FileAlias);
	return(FALSE);
}

/* This runs through the process list looking for the indicated application */
OSErr gra_findprocess(OSType typeToFind, OSType creatorToFind, ProcessSerialNumberPtr processSN,
	ProcessInfoRecPtr infoRecToFill)
{
	ProcessSerialNumber tempPSN;
	OSErr myErr = noErr;
	tempPSN.lowLongOfPSN = kNoProcess;
	processSN->lowLongOfPSN = kNoProcess;
	processSN->highLongOfPSN = kNoProcess;
	do
	{
		myErr = GetNextProcess(processSN);
		if (myErr != noErr) break;
		GetProcessInformation(processSN, infoRecToFill);
	}
	while (infoRecToFill->processSignature != creatorToFind ||
		infoRecToFill->processType != typeToFind);
	return(myErr);
}

/****************************** CHANNELS ******************************/

/*
 * routine to create a pipe connection between the channels in "channels"
 */
INTBIG epipe(int channels[2])
{
	return(0);
}

/*
 * Routine to set channel "channel" into an appropriate mode for single-character
 * interaction (i.e. break mode).
 */
void setinteractivemode(int channel)
{
}

/*
 * Routine to replace channel "channel" with a pointer to file "file".
 * Returns a pointer to the original channel.
 */
INTBIG channelreplacewithfile(int channel, CHAR *file)
{
	return(0);
}

/*
 * Routine to replace channel "channel" with new channel "newchannel".
 * Returns a pointer to the original channel.
 */
INTBIG channelreplacewithchannel(int channel, int newchannel)
{
	return(0);
}

/*
 * Routine to restore channel "channel" to the pointer that was returned
 * by "channelreplacewithfile" or "channelreplacewithchannel" (and is in "saved").
 */
void channelrestore(int channel, INTBIG saved)
{
}

/*
 * Routine to read "count" bytes from channel "channel" into "addr".
 * Returns the number of bytes read.
 */
INTBIG eread(int channel, UCHAR1 *addr, INTBIG count)
{
	return(0);
}

/*
 * Routine to write "count" bytes to channel "channel" from "addr".
 * Returns the number of bytes written.
 */
INTBIG ewrite(int channel, UCHAR1 *addr, INTBIG count)
{
	return(0);
}

/*
 * routine to close a channel in "channel"
 */
INTBIG eclose(int channel)
{
	return(0);
}

/*************************** TIME ROUTINES ***************************/

/*
 * The Macintosh uses an "epoch" of January 1, 1904.
 * All other machines use an "epoch" of January 1, 1970.
 * This means that time on the Macintosh is off by 66 years.
 * Specifically, there were 17 leap years between 1904 and 1970, so
 * there were  66*365+17 = 24107 days
 * Since each day has 24*60*60 = 86400 seconds in it, there were
 * 24107 * 86400 = 2,082,844,800 seconds difference between epochs.
 *
 * There is an extra "gotcha".  MSL (the Metrowerks Library) uses an epoch
 * of January 1, 1900!  So if this is MSL, add another 4 years to the epoch:
 * (365*4 = 1460 * 86400 = 126,144,000
 *
 * However, since Macintosh systems deal with local time and other
 * systems deal with GMT time, the Mac must also offset by the time zone.
 */
#define MACEPOCHOFFSET 2082844800
#define MSLEPOCHOFFSET  126144000

/*
 * This routine returns the amount to add to the operating-system time
 * to adjust for a common time format.
 */
time_t machinetimeoffset(void)
{
	static time_t offset;
	static BOOLEAN offsetcomputed = FALSE;
	UINTBIG timezoneoffset;
	INTBIG gmtoffset;
	MachineLocation ml;

	if (!offsetcomputed)
	{
		offsetcomputed = TRUE;
		offset = MACEPOCHOFFSET;
#ifdef __MSL__
		offset += MSLEPOCHOFFSET;
#endif
		ReadLocation(&ml);
		timezoneoffset = ml.u.gmtDelta & 0xFFFFFF;
		gmtoffset = timezoneoffset & 0x7FFFFF;
		if ((timezoneoffset & 0x800000) != 0)
			gmtoffset = (~0x7FFFFF) | gmtoffset;
		offset += gmtoffset;
	}
	return(offset);
}

/* returns the time at which the current event occurred */
UINTBIG eventtime(void)
{
	return(ticktime());
}

/* returns the current time in 60ths of a second */
UINTBIG ticktime(void)
{
	return(TickCount());
}

/* returns the double-click interval in 60ths of a second */
INTBIG doubleclicktime(void)
{
	return(gra_doubleclick);
}

/*
 * Routine to wait "ticks" sixtieths of a second and then return.
 */
void gotosleep(INTBIG ticks)
{
#ifdef NEWCODEWARRIOR
	unsigned long l;

	Delay(ticks, &l);
#else
	long l;

	Delay(ticks, &l);
#endif
}

/*
 * Routine to start counting time.
 */
void starttimer(void)
{
	gra_timestart = clock();
}

/*
 * Routine to stop counting time and return the number of elapsed seconds
 * since the last call to "starttimer()".
 */
float endtimer(void)
{
	float seconds;
	UINTBIG thistime;

	thistime = clock();
	seconds = ((float)(thistime - gra_timestart)) / 60.0;
	return(seconds);
}

/*************************** EVENT ROUTINES ***************************/

/*
 * Hack routine that writes a fake "mouse up" event to the session logging file.  This
 * is necessary because certain toolbox routines (i.e. TEClick) track the
 * mouse while it is down in order to handle selection of text.  Since this tracking
 * cannot be obtained and logged, a fake "SHIFT mouse" is logged after the routine finishes
 * so that the text selection is effected in the same way
 */
void gra_fakemouseup(void)
{
	Point p;

	if (us_logrecord == NULL) return;
	GetMouse(&p);
	gra_action.kind = ISBUTTON | SHIFTISDOWN;
	gra_action.x = p.h;
	gra_action.y = p.v;
	if (xfwrite((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logrecord) == 0)
	{
		ttyputerr(_("Error writing session log file: recording disabled"));
		logfinishrecord();
	}
	gra_logrecordindex++;
	if (gra_logrecordindex >= us_logflushfreq)
	{
		/* flush the session log file */
		gra_logrecordindex = 0;
		xflushbuf(us_logrecord);
	}
}

/*
 * helper routine to wait for some keyboard or mouse input.  The value of "nature" is:
 *   0  allow mouse, keyboard
 *   1  allow mouse, keyboard, pulldown menus
 *   2  allow mouse, keyboard, pulldown menus, motion
 */
void gra_waitforaction(INTBIG nature, EventRecord *theEvent)
{
	REGISTER INTBIG err;
	static INTBIG last_cursorx, last_cursory;
	REGISTER BOOLEAN saveevent;
	static INTBIG fakewhen = 0;
	Rect r, fr;

	if (us_logplay != NULL)
	{
		if (EventAvail(updateMask | activMask, theEvent) != 0)
		{
			gra_nextevent(4, theEvent);
			return;
		}
		flushscreen();
		err = xfread((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logplay);
		if (stopping(STOPREASONPLAYBACK)) err = 0;
		if (err != 0)
		{
			if (gra_playbackmultiple <= 0)
			{
				gra_playbackmultiple = 0;
				for(;;)
				{
					gra_inputstate = NOEVENT;
					gra_nextevent(0, theEvent);
					if (gra_inputstate == NOEVENT) continue;
					if ((gra_inputstate&ISKEYSTROKE) == 0) continue;
					if ((gra_inputstate&CHARREAD) < '0' || (gra_inputstate&CHARREAD) > '9') break;
					gra_playbackmultiple = gra_playbackmultiple * 10 + (gra_inputstate&CHARREAD) - '0';
				}
				if (gra_inputstate != NOEVENT && (gra_inputstate&CHARREAD) == 'q')
					err = 0;
			}
			gra_playbackmultiple--;

			/* allow Command-. to interrupt long playbacks */
			if ((gra_playbackmultiple%10) == 9)
			{
				if (EventAvail(keyDownMask, theEvent) != 0)
				{
					(void)WaitNextEvent(keyDownMask, theEvent, 0, 0);
					if ((theEvent->modifiers & cmdKey) != 0 &&
						(theEvent->message & charCodeMask) == '.')
							gra_playbackmultiple = 0;
				}
			}

			gra_inputstate = gra_action.kind & 0xFFFF;
			gra_cursorx = gra_action.x;
			gra_cursory = gra_action.y;
			us_state &= ~GOTXY;
			if (gra_inputstate == MENUEVENT)
			{
				(void)xfread((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logplay);
				gra_lowmenu = gra_action.x;
				gra_highmenu = gra_action.y;
			} else if (gra_inputstate == WINDOWCHANGE)
			{
				(void)xfread((UCHAR1 *)&r, 1, sizeof (r), us_logplay);
				fr = (*((WindowPeek)el_curwindowframe->realwindow)->strucRgn)->rgnBBox;
				MoveWindow((WindowPtr)el_curwindowframe->realwindow, r.left, r.top, 0);
				SizeWindow((WindowPtr)el_curwindowframe->realwindow, r.right-r.left,
					r.bottom-r.top, 1);
				gra_mygrowwindow((WindowPtr)el_curwindowframe->realwindow, r.right-r.left,
					r.bottom-r.top, &fr);
			}
		}

		/* convert to an event */
		fakewhen += gra_doubleclick + 1;
		theEvent->what = nullEvent;
		theEvent->when = fakewhen;
		theEvent->modifiers = 0;
		if ((gra_inputstate&SHIFTISDOWN) != 0) theEvent->modifiers |= shiftKey;
		if ((gra_inputstate&COMMANDISDOWN) != 0) theEvent->modifiers |= cmdKey;
		if ((gra_inputstate&OPTIONISDOWN) != 0) theEvent->modifiers |= optionKey;
		if ((gra_inputstate&CONTROLISDOWN) != 0) theEvent->modifiers |= controlKey;
		if ((gra_inputstate&BUTTONUP) != 0) theEvent->modifiers |= btnState;
		theEvent->where.h = gra_cursorx;
		theEvent->where.v = el_curwindowframe->revy - gra_cursory;
		if ((gra_inputstate&ISKEYSTROKE) != 0)
		{
			theEvent->what = keyDown;
			theEvent->message = gra_inputstate & CHARREAD;
		} else if ((gra_inputstate&ISBUTTON) != 0)
		{
			if ((gra_inputstate&BUTTONUP) == 0) theEvent->what = mouseDown; else
				theEvent->what = mouseUp;
			if ((gra_inputstate&DOUBLECLICK) != 0)
			{
				theEvent->when--;
				fakewhen--;
			}
		}
		/* stop now if end of playback file */
		if (err == 0)
		{
			ttyputmsg(_("End of session playback file"));
			xclose(us_logplay);
			us_logplay = NULL;
			return;
		}
	} else
	{
		flushscreen();
		gra_nextevent(nature, theEvent);
	}

	if (us_logrecord != NULL && gra_inputstate != NOEVENT)
	{
		saveevent = TRUE;
		if ((gra_inputstate&MOTION) != 0)
		{
			if (gra_cursorx == last_cursorx && gra_cursory == last_cursory) saveevent = FALSE;
		}
		if (saveevent)
		{
			gra_action.kind = gra_inputstate;
			gra_action.x = last_cursorx = gra_cursorx;
			gra_action.y = last_cursory = gra_cursory;
			if (xfwrite((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logrecord) == 0)
			{
				ttyputerr(_("Error writing session log file: recording disabled"));
				logfinishrecord();
			}
			if (gra_inputstate == MENUEVENT)
			{
				gra_action.x = gra_lowmenu;
				gra_action.y = gra_highmenu;
				(void)xfwrite((UCHAR1 *)&gra_action, 1, sizeof (gra_action), us_logrecord);
			} else if (gra_inputstate == WINDOWCHANGE)
			{
				if (el_curwindowframe != NOWINDOWFRAME)
				{
					r = (*((WindowPeek)el_curwindowframe->realwindow)->strucRgn)->rgnBBox;
					(void)xfwrite((UCHAR1 *)&r, 1, sizeof (r), us_logrecord);
				}
			}
			gra_logrecordindex++;
			if (gra_logrecordindex >= us_logflushfreq)
			{
				/* flush the session log file */
				gra_logrecordindex = 0;
				xflushbuf(us_logrecord);
			}
		}
	}

	/* deal with special event types */
	if (gra_inputstate == MENUEVENT)
	{
		if (gra_highmenu == appleMENU && gra_lowmenu == 1)
		{
			gra_applemenu(gra_lowmenu);
		} else gra_nativemenudoone(gra_lowmenu, gra_highmenu);
		gra_inputstate = NOEVENT;
	} else if (gra_inputstate == WINDOWCHANGE) gra_inputstate = NOEVENT;
}

/*
 * Routine to get the next Electric input action and set the global "gra_inputstate"
 * accordingly.  The value of "nature" is:
 *   0  allow mouse, keyboard (no window switching)
 *   1  allow mouse, keyboard, pulldown menus
 *   2  allow mouse, keyboard, motion
 *   4  allow update and activate events only
 */
void gra_nextevent(INTBIG nature, EventRecord *theEvent)
{
	INTBIG oak, cntlCode, x, y, stroke, oldpos, findres, inmenu, item;
	INTBIG key, theResult, xv, yv, lx, hx, ly, hy, modifiers, special;
	WStateData *wst;
	REGISTER WINDOWPART *w;
	REGISTER EDITOR *e;
	REGISTER VARIABLE *var;
	CHAR *par[1], *str, lastchar;
	static BOOLEAN firstwupdate = TRUE;
	WindowPtr theWindow, win, frontWindow;
	Rect r, fr;
	ControlHandle theControl;
	TDIALOG *dia;
	WINDOWFRAME *wf;
	static BOOLEAN overrodestatus = FALSE;
	COMMANDBINDING commandbinding;
	extern INTBIG sim_window_wavexbar;

	gra_inputstate = NOEVENT;
	HiliteMenu(0);
	if (gra_messageswindow != 0) TEIdle(gra_TEH);
	if (nature == 4) oak = WaitNextEvent(updateMask | activMask, theEvent, 0, 0); else
		oak = WaitNextEvent(everyEvent, theEvent, 0, 0);
	if (oak == 0 && nature != 4 && gra_motioncheck)
	{
		gra_motioncheck = FALSE;
		oak = theEvent->what = app3Evt;
		if (Button())
		{
			theEvent->modifiers |= btnState;
		} else
		{
			theEvent->modifiers &= ~btnState;
		}
	}
	if (oak == 0)
	{
		if (nature != 2)
		{
			if (FindWindow(theEvent->where, &theWindow) != inContent)
				setdefaultcursortype(NORMALCURSOR);
		}
		return;
	}
	if (oak != 0) switch (theEvent->what)
	{
		case mouseUp:
			if (el_curwindowframe != NOWINDOWFRAME)
			{
				SetPort((WindowPtr)el_curwindowframe->realwindow);
				GlobalToLocal(&theEvent->where);
				gra_cursorx = theEvent->where.h;
				gra_cursory = el_curwindowframe->revy - theEvent->where.v;
				us_state &= ~GOTXY;
			}
			gra_inputstate = ISBUTTON | BUTTONUP;
			us_state |= DIDINPUT;
			break;
		case mouseDown:
			switch (FindWindow(theEvent->where, &theWindow))
			{
				case inMenuBar:
					if (nature != 1) break;
					key = MenuSelect(theEvent->where);
					if (HiWord(key) == appleMENU && LoWord(key) != 1)
					{
						gra_applemenu(LoWord(key));
						return;
					}
					gra_highmenu = HiWord(key);
					gra_lowmenu = LoWord(key);
					gra_inputstate = MENUEVENT;
					return;
				case inSysWindow:
					SystemClick(theEvent, theWindow);
					break;
				case inContent:
					SetPort(theWindow);
					GlobalToLocal(&theEvent->where);

					/* determine which editor window was hit */
					for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
						if ((WindowPtr)wf->realwindow == theWindow) break;
					gra_lastclickedwindow = theWindow;
					gra_lastclickedwindowframe = wf;

					/* handle clicks in title bar of floating windows */
					if (wf != NOWINDOWFRAME && wf->floating && theEvent->where.v < FLOATINGHEADERSIZE)
					{
						if (theEvent->where.v >= 2 && theEvent->where.v <= 8 &&
							theEvent->where.h >= 4 && theEvent->where.h <= 10)
						{
							/* click in close box: turn off floating window */
							par[0] = x_("off");
							us_menu(1, par);
						} else
						{
							/* click in drag bar: move it */
							LocalToGlobal(&theEvent->where);
							gra_dragfloatingwindow(theWindow, theEvent->where);
						}
						break;
					}

					gra_frontnonfloatingwindow(&frontWindow);
					if (theWindow != frontWindow)
					{
						/* click in new window: see if it is allowed */
						if (nature == 0)
						{
							ttybeep(SOUNDBEEP, TRUE);
							break;
						}

						/* select this one as the frontmost window */
						gra_selectoswindow(theWindow);
						gra_setcurrentwindowframe();

						/* when reentering edit window, force proper cursor */
						if (wf == NOWINDOWFRAME) break;
						oak = us_cursorstate;
						us_cursorstate++;
						setdefaultcursortype(oak);
					}

					/* special case to ensure that "el_curwindowframe" gets floating window */
					if (el_curwindowframe != NOWINDOWFRAME && el_curwindowframe->floating)
						gra_setcurrentwindowframe();
					if (wf != NOWINDOWFRAME && wf->floating) el_curwindowframe = wf;

					if (theWindow == gra_messageswindow && gra_messageswindow != 0)
					{
						cntlCode = FindControl(theEvent->where, theWindow, &theControl);
						if (cntlCode == kControlIndicatorPart)
						{
							oldpos = GetControlValue(theControl);
							TrackControl(theControl, theEvent->where, 0L);
							if (theControl == gra_vScroll) gra_adjustvtext(); else
								gra_adjusthtext(oldpos);
							break;
						}
						if (cntlCode == kControlUpButtonPart || cntlCode == kControlDownButtonPart ||
							cntlCode == kControlPageUpPart || cntlCode == kControlPageDownPart)
						{
							if (theControl == gra_vScroll)
								TrackControl(theControl, theEvent->where, gra_scrollvprocUPP); else
									TrackControl(theControl, theEvent->where, gra_scrollhprocUPP);
							break;
						}
						TEClick(theEvent->where, (theEvent->modifiers&shiftKey) != 0, gra_TEH);
						gra_fakemouseup();
						break;
					}

					/* ignore clicks in the messages area at the bottom */
					if (wf != NOWINDOWFRAME && !wf->floating &&
						theEvent->where.v >= theWindow->portRect.bottom - SBARHEIGHT) break;

					/* handle clicks in modeless dialogs */
					if (wf == NOWINDOWFRAME)
					{
						for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
							if (dia->theDialog == theWindow) break;
						if (dia != NOTDIALOG && dia->modelessitemhit != 0)
						{
							gra_cursorx = theEvent->where.h;
							gra_cursory = theEvent->where.v;
							oak = 1;
							if (theEvent->when - gra_doubleclick < gra_lastclick &&
								abs(gra_cursorx-gra_lstcurx) < 5 && abs(gra_cursory-gra_lstcury) < 5)
							{
								oak = 5;
								gra_lastclick = theEvent->when - gra_doubleclick - 1;
							} else gra_lastclick = theEvent->when;
							gra_lstcurx = gra_cursorx;   gra_lstcury = gra_cursory;
							item = gra_getnextcharacter(dia, oak, gra_cursorx, gra_cursory,
								0, 0, theEvent->when, FALSE);
							if (item >= 0) (*dia->modelessitemhit)(dia, item);
							break;
						}
					}

					/* create the "click" event */
					gra_inputstate = ISBUTTON;
					if ((theEvent->modifiers&shiftKey) != 0) gra_inputstate |= SHIFTISDOWN;
					if ((theEvent->modifiers&cmdKey) != 0) gra_inputstate |= COMMANDISDOWN;
					if ((theEvent->modifiers&optionKey) != 0) gra_inputstate |= OPTIONISDOWN;
					if ((theEvent->modifiers&controlKey) != 0) gra_inputstate |= CONTROLISDOWN;
					gra_cursorx = theEvent->where.h;
					if (wf == NOWINDOWFRAME) gra_cursory = theEvent->where.v; else
					{
						gra_cursory = wf->revy - theEvent->where.v;
						if (wf->floating) gra_cursory += FLOATINGHEADERSIZE;
					}
					us_state &= ~GOTXY;
					if (theEvent->when - gra_doubleclick < gra_lastclick &&
						(gra_inputstate & (SHIFTISDOWN|COMMANDISDOWN|OPTIONISDOWN|CONTROLISDOWN)) == 0 &&
						abs(gra_cursorx-gra_lstcurx) < 5 && abs(gra_cursory-gra_lstcury) < 5)
					{
						gra_inputstate |= DOUBLECLICK;
						gra_lastclick = theEvent->when - gra_doubleclick - 1;
					} else gra_lastclick = theEvent->when;
					gra_lstcurx = gra_cursorx;   gra_lstcury = gra_cursory;
					us_state |= DIDINPUT;
					break;
				case inDrag:
					if (theWindow != FrontWindow() && nature == 0)
					{
						ttybeep(SOUNDBEEP, TRUE);
						break;
					}
					gra_selectoswindow(theWindow);
					gra_setcurrentwindowframe();
					gra_dragfloatingwindow(theWindow, theEvent->where);
					break;
				case inGrow:
					SetPort(theWindow);
					SetRect(&r, 80, 80, qd.screenBits.bounds.right+500,
						qd.screenBits.bounds.bottom+500);
					fr = (*((WindowPeek)theWindow)->strucRgn)->rgnBBox;
					theResult = GrowWindow(theWindow, theEvent->where, &r);
					if (theResult != 0)
					{
						SizeWindow(theWindow, LoWord(theResult), HiWord(theResult), 1);
						gra_mygrowwindow(theWindow, LoWord(theResult), HiWord(theResult), &fr);
					}
					gra_inputstate = WINDOWCHANGE;
					break;
				case inZoomIn:
					if (TrackBox(theWindow, theEvent->where, inZoomIn) == 0) break;
					SetPort(theWindow);
					EraseRect(&theWindow->portRect);
					wst = (WStateData *) *(((WindowPeek)theWindow)->dataHandle);
					fr = (*((WindowPeek)theWindow)->strucRgn)->rgnBBox;
					MoveWindow(theWindow, wst->userState.left, wst->userState.top, 0);
					SizeWindow(theWindow, wst->userState.right-wst->userState.left,
						wst->userState.bottom-wst->userState.top, 1);
					gra_mygrowwindow(theWindow, wst->userState.right-wst->userState.left,
						wst->userState.bottom-wst->userState.top, &fr);
					gra_inputstate = WINDOWCHANGE;
					break;
				case inZoomOut:
					if (TrackBox(theWindow, theEvent->where, inZoomOut) == 0) break;
					SetPort(theWindow);
					EraseRect(&theWindow->portRect);
					wst = (WStateData *) *(((WindowPeek)theWindow)->dataHandle);
					fr = (*((WindowPeek)theWindow)->strucRgn)->rgnBBox;
					MoveWindow(theWindow, wst->stdState.left, wst->stdState.top, 0);
					SizeWindow(theWindow, wst->stdState.right-wst->stdState.left,
						wst->stdState.bottom-wst->stdState.top, 1);
					gra_mygrowwindow(theWindow, wst->stdState.right-wst->stdState.left,
						wst->stdState.bottom-wst->stdState.top, &fr);
					gra_inputstate = WINDOWCHANGE;
					break;
				case inGoAway:
					SetPort(theWindow);
					if (TrackGoAway(theWindow, theEvent->where) == 0) break;
					if (theWindow == gra_messageswindow && gra_messageswindow != 0)
					{
						gra_hidemessageswindow();
					} else if (el_curwindowframe != NOWINDOWFRAME &&
						theWindow == (WindowPtr)el_curwindowframe->realwindow)
					{
						par[0] = x_("delete");
						us_window(1, par);
					} else
					{
						/* determine which editor window was hit */
						for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
							if ((WindowPtr)wf->realwindow == theWindow) break;
						if (wf != NOWINDOWFRAME) gra_disposeoswindow(theWindow); else
							gra_disposeoswindow(theWindow);
					}
					break;
			}
			break;
		case keyDown:
		case autoKey:
			special = 0;
			switch ((theEvent->message&keyCodeMask) >> 8)
			{
				case 122: special = SPECIALKEYDOWN|(SPECIALKEYF1<<SPECIALKEYSH);    break;
				case 120: special = SPECIALKEYDOWN|(SPECIALKEYF2<<SPECIALKEYSH);    break;
				case  99: special = SPECIALKEYDOWN|(SPECIALKEYF3<<SPECIALKEYSH);    break;
				case 118: special = SPECIALKEYDOWN|(SPECIALKEYF4<<SPECIALKEYSH);    break;
				case  96: special = SPECIALKEYDOWN|(SPECIALKEYF5<<SPECIALKEYSH);    break;
				case  97: special = SPECIALKEYDOWN|(SPECIALKEYF6<<SPECIALKEYSH);    break;
				case  98: special = SPECIALKEYDOWN|(SPECIALKEYF7<<SPECIALKEYSH);    break;
				case 100: special = SPECIALKEYDOWN|(SPECIALKEYF8<<SPECIALKEYSH);    break;
				case 101: special = SPECIALKEYDOWN|(SPECIALKEYF9<<SPECIALKEYSH);    break;
				case 109: special = SPECIALKEYDOWN|(SPECIALKEYF10<<SPECIALKEYSH);   break;
				case 103: special = SPECIALKEYDOWN|(SPECIALKEYF11<<SPECIALKEYSH);   break;
				case 111: special = SPECIALKEYDOWN|(SPECIALKEYF12<<SPECIALKEYSH);   break;
			}
			if (special != 0) stroke = 0; else
			{
				stroke = (theEvent->message & charCodeMask) & CHARREAD;
				if (stroke == 0) break;
			}
			if (el_curwindowframe != NOWINDOWFRAME)
			{
				SetPort((WindowPtr)el_curwindowframe->realwindow);
				GlobalToLocal(&theEvent->where);
				gra_cursorx = theEvent->where.h;
				gra_cursory = el_curwindowframe->revy - theEvent->where.v;
				us_state &= ~GOTXY;
			}
			modifiers = theEvent->modifiers;

			/* arrows are special */
			if (stroke >= 034 && stroke <= 037)
			{
				switch (stroke)
				{
					case 034: special = SPECIALKEYDOWN|(SPECIALKEYARROWL<<SPECIALKEYSH);    break;
					case 035: special = SPECIALKEYDOWN|(SPECIALKEYARROWR<<SPECIALKEYSH);    break;
					case 036: special = SPECIALKEYDOWN|(SPECIALKEYARROWU<<SPECIALKEYSH);    break;
					case 037: special = SPECIALKEYDOWN|(SPECIALKEYARROWD<<SPECIALKEYSH);    break;
				}
				if ((modifiers&shiftKey) != 0) special |= SHIFTDOWN;
			}
			if ((modifiers&cmdKey) != 0) special |= ACCELERATORDOWN;

			/* see if this goes to a modeless dialog */
			gra_frontnonfloatingwindow(&theWindow);
			for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
				if (dia->theDialog == theWindow) break;
			if (dia != NOTDIALOG && dia->modelessitemhit != 0)
			{
				if ((special&ACCELERATORDOWN) != 0)
				{
					if (stroke == 'c')
					{
						DTextCopy(dia);
						break;
					}
					if (stroke == 'x')
					{
						DTextCut(dia);
						stroke = BACKSPACEKEY;
						special = 0;
					} else if (stroke == 'v')
					{
						stroke = DTextPaste(dia);
						if (stroke == 0) break;
						special = 0;
					}
				}
				item = gra_getnextcharacter(dia, 0, theEvent->where.h, theEvent->where.v,
					stroke, special, theEvent->when, (special&SHIFTDOWN) != 0);
				if (item >= 0) (*dia->modelessitemhit)(dia, item);
				break;
			}

			if ((modifiers & cmdKey) != 0)
			{
				if (stroke == '.')
				{
					el_pleasestop = 2;
					return;
				}
				if (nature == 1)
				{
					/* handle cut/copy/paste specially in dialogs */
					if (gra_handlingdialog != NOTDIALOG)
					{
						if (stroke == 'c')
						{
							DTextCopy(gra_handlingdialog);
							break;
						}
						if (stroke == 'x')
						{
							DTextCut(gra_handlingdialog);
							gra_inputstate = BACKSPACEKEY & CHARREAD;
							us_state |= DIDINPUT;
							break;
						}
						if (stroke == 'v')
						{
							lastchar = DTextPaste(gra_handlingdialog);
							if (lastchar != 0)
							{
								gra_inputstate = lastchar & CHARREAD;
								us_state |= DIDINPUT;
							}
							break;
						}
						break;
					}
					gra_inputstate = stroke | ISKEYSTROKE;
					gra_inputspecial = special;
					us_state |= DIDINPUT;
					break;
				}
			}
			gra_frontnonfloatingwindow(&frontWindow);
			if (gra_messageswindow == frontWindow && gra_messageswindow != 0 && special == 0)
			{
				TESetSelect(32767, 32767, gra_TEH);
				TEKey(stroke, gra_TEH);
				gra_showselect();
				break;
			}
			gra_inputstate = stroke | ISKEYSTROKE;
			gra_inputspecial = special;
			us_state |= DIDINPUT;
			break;

		case activateEvt:
			/* only interested in activation, not deactivation */
			if ((theEvent->modifiers&activeFlag) == 0) break;

			/* ignore messages window */
			win = (WindowPtr)theEvent->message;
			if (win == gra_messageswindow && gra_messageswindow != 0) break;

			/* ignore editor windows */
			for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
				if ((WindowPtr)wf->realwindow == win) break;
			if (wf != NOWINDOWFRAME) break;

			/* probably a dialog, just process it */
			SetPort(win);
			InvalRect(&win->portRect);
			break;

		case updateEvt:
			win = (WindowPtr)theEvent->message;
			SetPort(win);
			BeginUpdate(win);
			if (win == gra_messageswindow && gra_messageswindow != 0)
			{
				EraseRect(&win->portRect);
				DrawControls(win);
				DrawGrowIcon(win);
				TEUpdate(&win->portRect, gra_TEH);
				EndUpdate(win);
				break;
			}

			for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
				if ((WindowPtr)wf->realwindow == win) break;
			if (wf != NOWINDOWFRAME)
			{
				if (!firstwupdate)
				{
					gra_drawosgraphics(wf);
					if (!wf->floating)
						gra_setrect(wf, (*wf->window->portPixMap)->bounds.right-SBARWIDTH-1,
							(*wf->window->portPixMap)->bounds.right-SBARWIDTH+1,
								0, (*wf->window->portPixMap)->bounds.bottom);
					r = (*wf->realwindow->visRgn)->rgnBBox;
					if (wf->floating)
					{
						r.top -= FLOATINGHEADERSIZE;
						r.bottom -= FLOATINGHEADERSIZE;
					}
					gra_setrect(wf, r.left, r.right, r.top, r.bottom);

					if (!wf->floating) us_redostatus(wf);
				}
				firstwupdate = FALSE;
			}

			/* probably a dialog, just process it */
			SetPort(win);
			for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
				if (dia->theDialog == win) break;
			if (dia != NOTDIALOG)
				Dredrawdialogwindow(dia);
			EndUpdate(win);
			break;

		case app3Evt:
			/* see if this happened in an editor window */
			findres = FindWindow(theEvent->where, &theWindow);
			if (theWindow == 0)
			{
				if ((theEvent->modifiers&btnState) == 0) break;
				wf = el_curwindowframe;
			} else
			{
				for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
					if ((WindowPtr)wf->realwindow == theWindow) break;
				SetPort(theWindow);
			}
			GlobalToLocal(&theEvent->where);

			/* report the menu if over one */
			inmenu = 0;
			if (wf != NOWINDOWFRAME && (theEvent->modifiers&btnState) == 0 && wf->floating)
			{
				gra_cursorx = theEvent->where.h;
				gra_cursory = wf->revy - theEvent->where.v;
				us_state &= ~GOTXY;
				x = (gra_cursorx-us_menulx) / us_menuxsz;
				y = (gra_cursory-us_menuly) / us_menuysz;
				if (x >= 0 && y >= 0 && x < us_menux && y < us_menuy)
				{
					var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
					if (var != NOVARIABLE)
					{
						if (us_menupos <= 1) str = ((CHAR **)var->addr)[y * us_menux + x]; else
							str = ((CHAR **)var->addr)[x * us_menuy + y];
						us_parsebinding(str, &commandbinding);
						if (*commandbinding.command != 0)
						{
							if (commandbinding.nodeglyph != NONODEPROTO)
							{
								ttysetstatusfield(NOWINDOWFRAME, us_statusarc, describearcproto(us_curarcproto), TRUE);
								ttysetstatusfield(NOWINDOWFRAME, us_statusnode, us_describemenunode(&commandbinding), TRUE);
								inmenu = 1;
								overrodestatus = TRUE;
							}
							if (commandbinding.arcglyph != NOARCPROTO)
							{
								ttysetstatusfield(NOWINDOWFRAME, us_statusarc, describearcproto(commandbinding.arcglyph), TRUE);
								if (us_curnodeproto == NONODEPROTO) str = x_(""); else
									str = describenodeproto(us_curnodeproto);
								ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
								inmenu = 1;
								overrodestatus = TRUE;
							}
						}
						us_freebindingparse(&commandbinding);
					}
				}
			}
			if (inmenu == 0 && overrodestatus)
			{
				ttysetstatusfield(NOWINDOWFRAME, us_statusarc, describearcproto(us_curarcproto), TRUE);
				if (us_curnodeproto == NONODEPROTO) str = x_(""); else
					str = describenodeproto(us_curnodeproto);
				ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
				overrodestatus = FALSE;
			}
			if (nature == 2)
			{
				/* handle cursor motion */
				gra_cursorx = theEvent->where.h;
				if (wf == NOWINDOWFRAME) gra_cursory = theEvent->where.v; else
					gra_cursory = wf->revy - theEvent->where.v;
				us_state &= ~GOTXY;
				gra_inputstate = MOTION;
				if ((theEvent->modifiers&btnState) != 0) gra_inputstate |= BUTTONUP;
				return;
			}
			/* checkout the cursor position */
			if (findres == inContent)
			{
				if (theWindow == gra_messageswindow && gra_messageswindow != 0)
				{
					if (theEvent->where.h > theWindow->portRect.right - SBARWIDTH ||
						theEvent->where.v > theWindow->portRect.bottom - SBARHEIGHT)
							setdefaultcursortype(NORMALCURSOR); else
								setdefaultcursortype(IBEAMCURSOR);
					return;
				}
				if (wf == NOWINDOWFRAME || theWindow != (WindowPtr)wf->realwindow)
				{
					setdefaultcursortype(NORMALCURSOR);
					return;
				}
				x = theEvent->where.h;
				if (wf == NOWINDOWFRAME) y = theEvent->where.v; else
					y = wf->revy - theEvent->where.v;
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				{
					if (w->frame != wf) continue;

					/* see if the cursor is over a window partition separator */
					us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
					if (x >= lx-1 && x <= lx+1 && y > ly+1 && y < hy-1 &&
						us_hasotherwindowpart(lx-10, y, w))
					{
						setdefaultcursortype(LRCURSOR);
						return;
					} else if (x >= hx-1 && x <= hx+1 && y > ly+1 && y < hy-1 &&
						us_hasotherwindowpart(hx+10, y, w))
					{
						setdefaultcursortype(LRCURSOR);
						return;
					} else if (y >= ly-1 && y <= ly+1 && x > lx+1 && x < hx-1 &&
						us_hasotherwindowpart(x, ly-10, w))
					{
						setdefaultcursortype(UDCURSOR);
						return;
					} else if (y >= hy-1 && y <= hy+1 && x > lx+1 && x < hx-1 &&
						us_hasotherwindowpart(x, hy+10, w))
					{
						setdefaultcursortype(UDCURSOR);
						return;
					}

					if (x < w->uselx || x > w->usehx || y < w->usely || y > w->usehy) continue;
					if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
					{
						xv = muldiv(x - w->uselx, w->screenhx - w->screenlx,
							w->usehx - w->uselx) + w->screenlx;
						yv = muldiv(y - w->usely, w->screenhy - w->screenly,
							w->usehy - w->usely) + w->screenly;
						if (abs(xv - sim_window_wavexbar) < 2 && yv >= 560)
						{
							setdefaultcursortype(LRCURSOR);
							return;
						}
					}
					if ((w->state&WINDOWTYPE) == POPTEXTWINDOW ||
						(w->state&WINDOWTYPE) == TEXTWINDOW)
					{
						e = w->editor;
						if ((e->state&EDITORTYPE) == PACEDITOR)
						{
							if (x > w->usehx - SBARWIDTH || y < w->usely + SBARHEIGHT || y >= e->revy)
								setdefaultcursortype(NORMALCURSOR); else
									setdefaultcursortype(IBEAMCURSOR);
							return;
						}
					} else if ((us_tool->toolstate&SHOWXY) != 0)
					{
						xv = x;   yv = y;
						xv = muldiv(xv - w->uselx, w->screenhx - w->screenlx,
							w->usehx - w->uselx) + w->screenlx;
						yv = muldiv(yv - w->usely, w->screenhy - w->screenly,
							w->usehy - w->usely) + w->screenly;
						gridalign(&xv, &yv, 1, w->curnodeproto);
						us_setcursorpos(wf, xv, yv);
					}
				}
				setdefaultcursortype(us_normalcursor);
				return;
			}
			setdefaultcursortype(NORMALCURSOR);
			return;

		case kHighLevelEvent:
			(void)AEProcessAppleEvent(theEvent);
			break;

		case osEvt:
			switch ((theEvent->message >> 24) & 0xFF)
			{
				case suspendResumeMessage:
					if ((theEvent->message&resumeFlag) == 0)
					{
						/* suspend the application */
						gra_frontnonfloatingwindow(&theWindow);
						if (theWindow != 0) gra_activatewindow(theWindow, FALSE);
						gra_showallfloaters(FALSE);
					} else
					{
						/* resume the application */
						gra_frontnonfloatingwindow(&theWindow);
						if (theWindow != 0) gra_activatewindow(theWindow, TRUE);
						gra_showallfloaters(TRUE);
					}
					break;
			}
	}
}

/* handle interrupts */
void gra_onint(void)
{
	(void)signal(SIGINT, (SIGNALCAST)gra_onint);
	el_pleasestop = 1;
	ttyputerr(_("Interrupted..."));
}

/*************************** SESSION LOGGING ROUTINES ***************************/

/*
 * routine to begin playback of session logging file "file".  The routine
 * returns true if there is an error.
 */
BOOLEAN logplayback(CHAR *file)
{
	REGISTER INTBIG comcount;
	CHAR *filename;

	us_logplay = xopen(file, us_filetypelog, x_(""), &filename);
	if (us_logplay == NULL) return(TRUE);
	ttyputmsg(_("Type any key to playback the next step in the log file"));
	ttyputmsg(_("Type a number followed by 'x' to playback that many steps"));
	ttyputmsg(_("Type 'q' to terminate playback"));

	comcount = filesize(us_logplay) / (sizeof (gra_action));
	ttyputmsg(_("There are no more than %ld steps to playback"), comcount);
	gra_playbackmultiple = comcount;
	return(FALSE);
}

/*
 * routine to create a session logging file
 */
void logstartrecord(void)
{
#if 0		/* no session logging on Macintosh yet */
	us_logrecord = xcreate(ELECTRICLOG, us_filetypelog, 0, 0);
#else
	us_logrecord = NULL;
#endif
}

/*
 * routine to terminate session logging
 */
void logfinishrecord(void)
{
	if (us_logrecord != NULL) xclose(us_logrecord);
	us_logrecord = NULL;
}

/****************************** MENUS ******************************/

void gra_initializemenus(void)
{
	CHAR aboutelectric[100];

	gra_pulldownmenucount = 0;
	gra_appleMenu = NewMenu(appleMENU, x_("\p\024"));
	estrcpy(&aboutelectric[1], _("About Electric"));
	estrcat(&aboutelectric[1], x_("..."));
	aboutelectric[0] = estrlen(&aboutelectric[1]);
	AppendMenu(gra_appleMenu, (UCHAR1 *)aboutelectric);
	InsertMenu(gra_appleMenu, 0);
	DrawMenuBar();
	AppendResMenu(gra_appleMenu, 'DRVR');
}

/*
 * routine to handle the Apple menu, including the "About Electric..." dialog
 */
void gra_applemenu(INTBIG sindex)
{
	Str255 name;
	GrafPtr savePort;

	GetPort(&savePort);
	if (sindex == aboutMeCommand)
	{
		(void)us_aboutdlog();
	} else
	{
		GetMenuItemText(gra_appleMenu, sindex, name);
		(void)OpenDeskAcc(name);
	}
	SetPort(savePort);
}

void getacceleratorstrings(CHAR **acceleratorstring, CHAR **acceleratorprefix)
{
	*acceleratorstring = x_("Cmd");
	*acceleratorprefix = x_("Cmd-");
}

CHAR *getinterruptkey(void)
{
	return(_("Command-."));
}

INTBIG nativepopupmenu(POPUPMENU **menu, BOOLEAN header, INTBIG left, INTBIG top)
{
	INTBIG ret, len, i, j, index, menuid, submenus, submenubaseindex, submenuindex;
	CHAR myline[256], origline[256], submenu[4], *pt;
	MenuHandle thismenu, subpopmenu;
	Point p;
	REGISTER USERCOM *uc;
	REGISTER POPUPMENUITEM *mi;
	REGISTER POPUPMENU *themenu;

	themenu = *menu;
	if (gra_lastclickedwindow != 0) SetPort(gra_lastclickedwindow); else
		SetPort((WindowPtr)el_firstwindowframe->realwindow);
	if (left < 0 && top < 0)
	{
		p.h = gra_cursorx;
		if (gra_lastclickedwindowframe == NOWINDOWFRAME) p.v = gra_cursory; else
			p.v = gra_lastclickedwindowframe->revy - gra_cursory;
	} else
	{
		p.h = left;   p.v = top;
		LocalToGlobal(&p);
	}

	estrcpy(&myline[1], us_stripampersand(themenu->header));
	myline[0] = estrlen(&myline[1]);
	thismenu = NewMenu(2048, (UCHAR1 *)myline);
	if (thismenu == 0) return(-1);
	
	/* remember the first submenu for this popup */
	submenubaseindex = gra_pulldownmenucount;

	/* load the menus */
	submenus = 0;
	for(i=0; i<themenu->total; i++)
	{
		mi = &themenu->list[i];
		mi->changed = FALSE;
		if (*mi->attribute == 0)
		{
			(void)estrcpy(myline, x_(" (-"));
			myline[0] = estrlen(&myline[1]);
			AppendMenu(thismenu, (UCHAR1 *)myline);
		} else
		{
			/* quote illegal characters */
			pt = origline;
			for(j=0; mi->attribute[j] != 0; j++)
			{
				if (mi->attribute[j] == '&') continue;
				if (mi->attribute[j] == '(') *pt++ = '{'; else
					if (mi->attribute[j] == ')') *pt++ = '}'; else
						*pt++ = mi->attribute[j];
			}
			*pt = 0;

			uc = mi->response;
			if (uc != NOUSERCOM && uc->menu != NOPOPUPMENU)
			{
				submenuindex = gra_pulldownindex(uc->menu);
				if (submenuindex < 0)  return(-1);
				subpopmenu = gra_pulldownmenus[submenuindex];
				InsertMenu(subpopmenu, -1);
				myline[1] = '!';   myline[2] = USERMENUBASE+submenuindex;
				(void)estrcpy(&myline[3], origline);
				submenu[0] = '/';   submenu[1] = 0x1B;   submenu[2] = 0;
				(void)estrcat(&myline[1], submenu);
				myline[0] = estrlen(&myline[1]);
				AppendMenu(thismenu, (UCHAR1 *)myline);
				submenus++;
			} else
			{
				/* insert command title */
				pt = origline;
				if (pt[0] == '>' && pt[1] == ' ')
				{
					myline[1] = '!';
					myline[2] = 022;
					(void)estrcpy(&myline[3], &pt[2]);
					len = estrlen(myline);
					if (myline[len-2] == ' ' && myline[len-1] == '<') myline[len-2] = 0;
				} else (void)estrcpy(&myline[1], pt);
				myline[0] = estrlen(&myline[1]);
				AppendMenu(thismenu, (UCHAR1 *)myline);
			}
		}
	}

	/* run the popup menu */
	InsertMenu(thismenu, -1);
	ret = PopUpMenuSelect(thismenu, p.v, p.h, 1);

	/* delete the memory */
	DeleteMenu(2048);
	DisposeMenu(thismenu);
	for(j=0; j<submenus; j++)
	{
		DeleteMenu(j+submenubaseindex+USERMENUBASE);
		DisposeMenu(gra_pulldownmenus[j+submenubaseindex]);
	}

	/* restore base of submenus in use */
	gra_pulldownmenucount = submenubaseindex;

	/* determine selection */
	menuid = HiWord(ret);
	if (menuid == 0) return(-1);
	if (menuid != 2048)
	{
		index = menuid-USERMENUBASE;
		j = 0;
		for(i=0; i<themenu->total; i++)
		{
			mi = &themenu->list[i];
			if (*mi->attribute == 0) continue;
			uc = mi->response;
			if (uc == NOUSERCOM || uc->menu == NOPOPUPMENU) continue;
			if (j == index-submenubaseindex) break;
			j++;
		}
		if (i >= themenu->total) return(-1);
		*menu = uc->menu;
	}
	return(LoWord(ret) - 1);
}

void gra_nativemenudoone(INTBIG low, INTBIG high)
{
	INTBIG i, j;
	POPUPMENU *pm;
	BOOLEAN verbose;

	i = high - USERMENUBASE;
	if (i >= 0 && i < gra_pulldownmenucount)
	{
		pm = us_getpopupmenu(gra_pulldowns[i]);
		j = abs(low) - 1;
		if (j >= 0 && j < pm->total)
		{
			us_state |= DIDINPUT;
			us_state &= ~GOTXY;
			setdefaultcursortype(NULLCURSOR);
			us_forceeditchanges();
			if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
				verbose = FALSE;
			us_execute(pm->list[j].response, verbose, TRUE, TRUE);
			db_setcurrenttool(us_tool);
			setactivity(pm->list[j].attribute);
		}
	}
	HiliteMenu(0);
}
/* routine to redraw entry "sindex" of popupmenu "pm" because it changed */
void nativemenurename(POPUPMENU *pm, INTBIG sindex)
{
	INTBIG i, submenuindex, cmdchar, j, k, boundspecial;
	INTSML boundkey;
	CHAR line[100], *pt;
	USERCOM *uc;

	for(i=0; i<gra_pulldownmenucount; i++)
		if (namesame(gra_pulldowns[i], pm->name) == 0) break;
	if (i >= gra_pulldownmenucount) return;

	/* make sure the menu didn't change size */
	j = CountMItems(gra_pulldownmenus[i]);
	if (pm->total != j)
	{
		if (pm->total > j)
		{
			/* must add new entries */
			for(k=j; k<pm->total; k++)
				AppendMenu(gra_pulldownmenus[i], x_("\pX"));
		} else
		{
			/* must delete extra entries */
			for(k=pm->total; k<j; k++)
				DeleteMenuItem(gra_pulldownmenus[i], pm->total);
		}
	}

	uc = pm->list[sindex].response;
	(void)estrcpy(&line[1], us_stripampersand(pm->list[sindex].attribute));
	if (uc->active < 0)
	{
		if (gra_pulldownmenus[i] != 0)
			DisableItem(gra_pulldownmenus[i], sindex+1);
		if (*pm->list[sindex].attribute == 0) (void)estrcpy(line, x_(" -"));
	} else
	{
		if (gra_pulldownmenus[i] != 0)
			EnableItem(gra_pulldownmenus[i], sindex+1);
	}

	line[0] = estrlen(&line[1]);
	pt = line;
	if (pt[pt[0]] == '<') pt[0]--;
	if (pt[1] != '>')
	{
		if (gra_pulldownmenus[i] != 0)
			CheckItem(gra_pulldownmenus[i], sindex+1, 0);
	} else
	{
		pt[1] = pt[0] - 1;
		pt++;
		if (gra_pulldownmenus[i] != 0)
			CheckItem(gra_pulldownmenus[i], sindex+1, 1);
	}
	pt[pt[0]+1] = 0;
	cmdchar = 0;
	for(j=pt[0]; j > 0; j--) if (pt[j] == '/' || pt[j] == '\\') break;
	if (pt[j] == '/' || pt[j] == '\\')
	{
		(void)us_getboundkey(&pt[j], &boundkey, &boundspecial);
		if ((boundspecial&ACCELERATORDOWN) == 0)
			esnprintf(&pt[j], 100-j, x_("\t%s"), us_describeboundkey(boundkey, boundspecial, 1));
	}
	if (gra_pulldownmenus[i] != 0)
	{
		SetMenuItemText(gra_pulldownmenus[i], sindex+1, (UCHAR1 *)pt);
		SetItemCmd(gra_pulldownmenus[i], sindex+1, cmdchar);
	}

	/* see if this command is another menu */
	if (uc->menu != NOPOPUPMENU)
	{
		for(submenuindex=0; submenuindex<gra_pulldownmenucount; submenuindex++)
			if (namesame(gra_pulldowns[submenuindex], uc->menu->name) == 0) break;
		if (submenuindex < gra_pulldownmenucount)
		{
			if (gra_pulldownmenus[i] != 0)
			{
				SetItemCmd(gra_pulldownmenus[i], sindex+1, 0x1B);
				SetItemMark(gra_pulldownmenus[i], sindex+1, USERMENUBASE+submenuindex);
			}
		}
	}
}

/*
 * Routine to establish the "count" pulldown menu names in "par" as the pulldown menu bar.
 * Returns true on error.
 */
void nativemenuload(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, menuindex;
	REGISTER POPUPMENU *pm;
	POPUPMENU *pulls[25];

	/* build the pulldown menu bar */
	for(i=0; i<count; i++)
	{
		pm = us_getpopupmenu(par[i]);
		if (pm == NOPOPUPMENU) continue;
		pulls[i] = pm;
		menuindex = gra_pulldownindex(pm);
		if (menuindex < 0) continue;
		InsertMenu(gra_pulldownmenus[menuindex], 0);
	}
	DrawMenuBar();
}

/*
 * Routine to create a pulldown menu from popup menu "pm".
 * Returns an index to the table of pulldown menus (-1 on error).
 */
INTBIG gra_pulldownindex(POPUPMENU *pm)
{
	REGISTER INTBIG i, sindex, newtotal;
	MenuHandle *newpulldownmenus;
	CHAR **newpulldowns;

	/* see if it is in the list already */
	for(i=0; i<gra_pulldownmenucount; i++)
		if (namesame(gra_pulldowns[i], pm->name) == 0) return(i);

	/* make room for the new menu */
	if (gra_pulldownmenucount >= gra_pulldownmenutotal)
	{
		newtotal = gra_pulldownmenutotal * 2;
		if (newtotal < gra_pulldownmenucount) newtotal = gra_pulldownmenucount + 5;
		
		newpulldownmenus = (MenuHandle *)emalloc(newtotal *
			(sizeof (MenuHandle)), us_tool->cluster);
		if (newpulldownmenus == 0) return(-1);
		newpulldowns = (CHAR **)emalloc(newtotal *
			(sizeof (CHAR *)), us_tool->cluster);
		if (newpulldowns == 0) return(-1);
		for(i=0; i<gra_pulldownmenucount; i++)
		{
			newpulldownmenus[i] = gra_pulldownmenus[i];
			newpulldowns[i] = gra_pulldowns[i];
		}
		if (gra_pulldownmenutotal > 0)
		{
			efree((CHAR *)gra_pulldownmenus);
			efree((CHAR *)gra_pulldowns);
		}
		gra_pulldownmenus = newpulldownmenus;
		gra_pulldowns = newpulldowns;
		gra_pulldownmenutotal = newtotal;
	}

	sindex = gra_pulldownmenucount++;
	(void)allocstring(&gra_pulldowns[sindex], pm->name, us_tool->cluster);
	gra_pulldownmenus[sindex] = gra_makepdmenu(pm, USERMENUBASE+sindex);
	if (gra_pulldownmenus[sindex] == 0) return(-1);
	return(sindex);
}

/*
 * Routine to create pulldown menu number "value" from the popup menu in "pm" and return
 * the menu handle.
 */
MenuHandle gra_makepdmenu(POPUPMENU *pm, INTBIG value)
{
	REGISTER INTBIG i, j, submenuindex, len;
	INTBIG boundspecial;
	INTSML boundkey;
	CHAR myline[256], attrib[256], *pt;
	REGISTER USERCOM *uc;
	REGISTER POPUPMENUITEM *mi;
	MenuHandle thismenu;
	CHAR submenu[4];

	estrcpy(&myline[1], us_stripampersand(pm->header));
	myline[0] = estrlen(&myline[1]);
	thismenu = NewMenu(value, (UCHAR1 *)myline);
	if (thismenu == 0) return(0);

	/* build the actual menu */
	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];

		/* quote illegal characters */
		pt = attrib;
		for(j=0; mi->attribute[j] != 0; j++)
		{
			if (mi->attribute[j] == '&') continue;
			if (mi->attribute[j] == '(') *pt++ = '{'; else
				if (mi->attribute[j] == ')') *pt++ = '}'; else
					*pt++ = mi->attribute[j];
		}
		*pt = 0;

		uc = mi->response;
		if (uc->active < 0)
		{
			(void)estrcpy(myline, x_(" ("));
			if (*attrib == 0) (void)estrcat(myline, x_("-")); else
				(void)estrcat(myline, attrib);
			myline[0] = estrlen(&myline[1]);
			AppendMenu(thismenu, (UCHAR1 *)myline);
			continue;
		}

		/* see if this command is another menu */
		if (uc->menu != NOPOPUPMENU)
		{
			submenuindex = gra_pulldownindex(uc->menu);
			if (submenuindex < 0) continue;
			InsertMenu(gra_pulldownmenus[submenuindex], -1);
			myline[1] = '!';   myline[2] = USERMENUBASE+submenuindex;
			(void)estrcpy(&myline[3], attrib);
			submenu[0] = '/';   submenu[1] = 0x1B;   submenu[2] = 0;
			(void)estrcat(&myline[1], submenu);
			myline[0] = estrlen(&myline[1]);
			AppendMenu(thismenu, (UCHAR1 *)myline);
			continue;
		}

		/* insert command title */
		pt = attrib;
		len = estrlen(pt) - 1;
		if (pt[len] == '<') pt[len] = 0;
		for(j=estrlen(pt)-1; j > 0; j--) if (pt[j] == '/' || pt[j] == '\\') break;
		if (pt[j] == '/' || pt[j] == '\\')
		{
			(void)us_getboundkey(&pt[j], &boundkey, &boundspecial);
			if ((boundspecial&ACCELERATORDOWN) == 0)
				esnprintf(&pt[j], 256-j, x_("\t%s"), us_describeboundkey(boundkey, boundspecial, 1));
		}
		if (pt[0] == '>')
		{
			myline[1] = '!';
			myline[2] = 022;
			(void)estrcpy(&myline[3], &pt[1]);
		} else (void)estrcpy(&myline[1], pt);
		myline[0] = estrlen(&myline[1]);
		AppendMenu(thismenu, (UCHAR1 *)myline);
	}
	return(thismenu);
}

/****************************** DIALOGS ******************************/

/*
 * Routine to initialize a dialog described by "dialog".
 * Returns true if dialog cannot be initialized.
 */
void *DiaInitDialog(DIALOG *dialog)
{
	TDIALOG *dia;

	dia = (TDIALOG *)emalloc(sizeof (TDIALOG), db_cluster);
	if (dia == 0) return(0);
	if (gra_initdialog(dialog, dia, FALSE)) return(0);
	return(dia);
}

/*
 * Routine to initialize dialog "dialog" in modeless style, calling
 * "itemhit" for each hit.
 */
void *DiaInitDialogModeless(DIALOG *dialog, void (*itemhit)(void *dia, INTBIG item))
{
	TDIALOG *dia;

	dia = (TDIALOG *)emalloc(sizeof (TDIALOG), db_cluster);
	if (dia == 0) return(0);
	if (gra_initdialog(dialog, dia, TRUE)) return(0);
	dia->modelessitemhit = itemhit;
	return(dia);
}

/*
 * Routine to initialize a dialog described by "dialog".
 * Returns true if dialog cannot be initialized.
 */
BOOLEAN gra_initdialog(DIALOG *dialog, TDIALOG *dia, BOOLEAN modeless)
{
	INTBIG itemtype, i, pureitemtype, amt, j;
	RECTAREA r;
	CHAR *save, *line;
	POPUPDATA *pd;

	/* add this to the list of active dialogs */
	dia->nexttdialog = gra_firstactivedialog;
	gra_firstactivedialog = dia;

	/* be sure the dialog is translated */
	DiaTranslate(dialog);

	/* initialize dialog data structures */
	dia->defaultbutton = OK;
	dia->dlgresaddr = dialog;
	dia->curitem = -1;
	dia->opaqueitem = -1;
	dia->usertextsize = 10;
	dia->userdoubleclick = 0;
	dia->lastdiatime = 0;
	dia->numlocks = 0;
	for(i=0; i<MAXSCROLLS; i++)
	{
		dia->scroll[i].scrollitem = -1;
		dia->scroll[i].horizfactor = 0;
		dia->scroll[i].scrolllistlen = 0;
		dia->scroll[i].scrolllistsize = 0;
	}
	dia->curscroll = 0;
	dia->scrollcount = 0;
	dia->firstch = 0;
	dia->modelessitemhit = 0;

#ifdef INTERNATIONAL
	/* compute size of some items to automatically scale them */
	SetPort(gra_messageswindow);
	TextFont(DFONT);
	TextSize(12);
	for(i=0; i<dialog->items; i++)
	{
		INTBIG offset;
		itemtype = dialog->list[i].type & ITEMTYPE;
		r = dia->dlgresaddr->list[i].r;
		switch (itemtype)
		{
			case DEFBUTTON:
			case BUTTON:
			case CHECK:
			case RADIO:
			case MESSAGE:
				if (itemtype == BUTTON || itemtype == DEFBUTTON) offset = 8; else
					if (itemtype == CHECK || itemtype == RADIO) offset = 16; else
						offset = 5;
				line = dialog->list[i].msg;
				j = TextWidth(line, 0, estrlen(line));
				amt = (j + offset) - (r.right - r.left);
				if (amt > 0)
				{
					INTBIG ycenter, k, shiftamt[100];
					for(j = 0; j < dialog->items; j++) shiftamt[j] = 0;
					for(j=0; j<dialog->items; j++)
					{
						if (j == i) continue;
						if (dialog->list[j].r.left >= dialog->list[i].r.right &&
							dialog->list[j].r.left < dialog->list[i].r.right+amt)
						{
							shiftamt[j] = amt;
							ycenter = (dialog->list[j].r.top + dialog->list[j].r.bottom) / 2;
							for(k=0; k<dialog->items; k++)
							{
								if (k == i || k == j) continue;
								if (dialog->list[k].r.right < dialog->list[j].r.left) continue;
								if (dialog->list[k].r.top < ycenter &&
									dialog->list[k].r.bottom > ycenter)
										shiftamt[k] = amt;
							}
						}
					}
					dia->dlgresaddr->list[i].r.right += amt;
					for(j = 0; j < dialog->items; j++)
					{
						dialog->list[j].r.left += shiftamt[j];
						dialog->list[j].r.right += shiftamt[j];
					}
				}
				break;
		}
	}
#endif
	for(i=0; i<dialog->items; i++)
	{
		j = dia->dlgresaddr->list[i].r.right + 5;
		if (j < dialog->windowRect.right - dialog->windowRect.left) continue;
		dialog->windowRect.right = dialog->windowRect.left + j;
	}
	/* make the window */
	Dnewdialogwindow(dia, &dialog->windowRect, dialog->movable, modeless);

	/* find the default button */
	for(i=0; i<dialog->items; i++)
	{
		itemtype = dia->dlgresaddr->list[i].type;
		if ((itemtype&ITEMTYPE) == DEFBUTTON)
			dia->defaultbutton = i+1;
	}

	/* loop through all of the dialog entries, drawing them */
	for(i=0; i<dialog->items; i++)
	{
		/* draw the item */
		itemtype = dia->dlgresaddr->list[i].type;
		pureitemtype = itemtype & ITEMTYPE;
		line = dia->dlgresaddr->list[i].msg;
		r = dia->dlgresaddr->list[i].r;

		if (pureitemtype == EDITTEXT)
		{
			if (dia->curitem == -1) dia->curitem = i;
		}
		if (pureitemtype == SCROLL || pureitemtype == SCROLLMULTI)
		{
			if (dia->scrollcount < MAXSCROLLS)
				dia->scroll[dia->scrollcount++].scrollitem = i;
		}
		if (pureitemtype == MESSAGE || pureitemtype == EDITTEXT ||
			pureitemtype == BUTTON || pureitemtype == DEFBUTTON ||
			pureitemtype == CHECK || pureitemtype == RADIO)
		{
			amt = estrlen(line) + 1;
			if (pureitemtype == CHECK || pureitemtype == RADIO) amt++;
			save = (CHAR *)emalloc(amt * SIZEOFCHAR, el_tempcluster);
			if (save == 0) return(TRUE);
			if (pureitemtype == CHECK || pureitemtype == RADIO)
			{
				(void)estrcpy(&save[1], line);
				save[0] = 0;
			} else (void)estrcpy(save, line);
			dia->dlgresaddr->list[i].data = (INTBIG)save;
		} else if (pureitemtype == POPUP)
		{
			pd = (POPUPDATA *)emalloc(sizeof (POPUPDATA), el_tempcluster);
			if (pd == 0) return(TRUE);
			pd->count = 0;
			dia->dlgresaddr->list[i].data = (INTBIG)pd;
			line = (CHAR *)pd;
		} else if (pureitemtype == ICON)
		{
			dia->dlgresaddr->list[i].data = (INTBIG)line;
		} else dia->dlgresaddr->list[i].data = 0;

		Ddrawitem(dia, itemtype, &r, line, 0);

		/* highlight the default button */
		if (i == dia->defaultbutton-1 &&
			(itemtype == BUTTON || itemtype == DEFBUTTON)) Dhighlightrect(dia, &r);
	}
	if (dia->curitem >= 0)
	{
		dia->editstart = 0;
		dia->editend = estrlen(dia->dlgresaddr->list[dia->curitem].msg);
		Dhighlight(dia, 1);
	}
	dia->firstupdate = 1;
	return(FALSE);
}

/*
 * Routine to handle actions and return the next item hit.
 */
INTBIG DiaNextHit(void *vdia)
{
	INTBIG chr, itemHit, oak, x, y, special;
	UINTBIG time;
	BOOLEAN shifted;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	for(;;)
	{
		/* if interrupted, stop dialog */
		if (el_pleasestop != 0) return(CANCEL);

		/* get the next event, ignore fluff */
		oak = Dwaitforaction(dia, &x, &y, &chr, &special, &time, &shifted);
		itemHit = gra_getnextcharacter(dia, oak, x, y, chr, special, time, shifted);
		if (itemHit == -1) continue;
		break;
	}
	return(itemHit);
}

void DiaDoneDialog(void *vdia)
{
	INTBIG i, j, type, puretype;
	POPUPDATA *oldpd;
	TDIALOG *dia, *ldia, *odia;

	dia = (TDIALOG *)vdia;

	/* remove this from the list of active dialogs */
	ldia = NOTDIALOG;
	for(odia = gra_firstactivedialog; odia != NOTDIALOG; odia = odia->nexttdialog)
	{
		if (odia == dia) break;
		ldia = odia;
	}
	if (odia != NOTDIALOG)
	{
		if (ldia == NOTDIALOG) gra_firstactivedialog = dia->nexttdialog; else
			ldia->nexttdialog = dia->nexttdialog;
	}

	/* free all the edit text and message buffers */
	for(i=0; i<dia->dlgresaddr->items; i++)
	{
		type = dia->dlgresaddr->list[i].type;
		puretype = type & ITEMTYPE;
		if (puretype == POPUP)
		{
			oldpd = (POPUPDATA *)dia->dlgresaddr->list[i].data;
			for(j=0; j<oldpd->count; j++) efree((CHAR *)oldpd->namelist[j]);
			if (oldpd->count > 0) efree((CHAR *)oldpd->namelist);
		}
		if (puretype == MESSAGE || puretype == EDITTEXT || puretype == POPUP ||
			puretype == BUTTON || puretype == DEFBUTTON ||
			puretype == RADIO || puretype == CHECK)
				efree((CHAR *)dia->dlgresaddr->list[i].data);
	}

	/* free all items in the scroll area */
	for(i=0; i<dia->scrollcount; i++)
	{
		for(j=0; j<dia->scroll[i].scrolllistlen; j++)
			efree((CHAR *)dia->scroll[i].scrolllist[j]);
		if (dia->scroll[i].scrolllistsize > 0) efree((CHAR *)dia->scroll[i].scrolllist);
	}
	Ddonedialogwindow(dia);

	/* redraw all other dialogs */
	for(odia = gra_firstactivedialog; odia != NOTDIALOG; odia = odia->nexttdialog)
		Dredrawdialogwindow(odia);

	/* free the "dia" structure */
	efree((CHAR *)dia);
}

/*
 * Routine to change the size of the dialog
 */
void DiaResizeDialog(void *vdia, INTBIG wid, INTBIG hei)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	SizeWindow((WindowRef)dia->theDialog, wid, hei, 1);
}

/*
 * Force the dialog to be visible.
 */
void DiaBringToTop(void *vdia)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
}

/*
 * Routine to set the text in item "item" to "msg"
 */
void DiaSetText(void *vdia, INTBIG item, CHAR *msg)
{
	INTBIG highlight, type, puretype, oldcur, dim;
	INTBIG amt;
	CHAR *save, *pt;
	RECTAREA r;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;

	/* determine whether item is highlighted */
	highlight = 0;
	if (item < 0)
	{
		item = -item;
		highlight = 1;
	}
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type;
	puretype = type & ITEMTYPE;

	/* special case when renaming buttons */
	if (puretype == BUTTON || puretype == DEFBUTTON ||
		puretype == CHECK || puretype == RADIO)
	{
		/* save the new string */
		amt = estrlen(msg)+1;
		if (puretype == CHECK || puretype == RADIO) amt++;
		save = (CHAR *)emalloc(amt * SIZEOFCHAR, el_tempcluster);
		if (save == 0) return;
		if (puretype == CHECK || puretype == RADIO)
		{
			(void)estrcpy(&save[1], msg);
			save[0] = ((CHAR *)dia->dlgresaddr->list[item].data)[0];
		} else (void)estrcpy(save, msg);
		efree((CHAR *)dia->dlgresaddr->list[item].data);
		dia->dlgresaddr->list[item].data = (INTBIG)save;

		r = dia->dlgresaddr->list[item].r;
		if ((type&INACTIVE) != 0) dim = 1; else dim = 0;
		Dintdrawrect(dia, &r, 255, 255, 255);
		Ddrawitem(dia, type, &r, msg, dim);
		if (puretype == RADIO && ((CHAR *)dia->dlgresaddr->list[item].data)[0] != 0)
		{
			/* draw the circle in a selected radio button */
			r.right = r.left + 12;
			r.top = (r.top + r.bottom) / 2 - 6;
			r.bottom = r.top + 12;
			Dinsetrect(&r, 3);
			Ddrawdisc(dia, &r);
		}
		if (puretype == CHECK && ((CHAR *)dia->dlgresaddr->list[item].data)[0] != 0)
		{
			/* draw the "X" in a selected check box */
			r.right = r.left + 12;
			r.top = (r.top + r.bottom) / 2 - 6;
			r.bottom = r.top + 12;
			Ddrawline(dia, r.left, r.top, r.right-1, r.bottom-1);
			Ddrawline(dia, r.left, r.bottom-1, r.right-1, r.top);
		}
		return;
	}

	/* convert copyright character sequence */
	for(pt = msg; *pt != 0; pt++) if (estrncmp(pt, x_("(c)"), 3) == 0)
	{
		(void)estrcpy(pt, x_("\251"));		/* "copyright" character */
		(void)estrcpy(&pt[1], &pt[3]);
		break;
	}

	/* handle messages and edit text */
	oldcur = dia->curitem;   Ddoneedit(dia);
	if (puretype == MESSAGE || puretype == EDITTEXT)
	{
		/* save the new string */
		amt = estrlen(msg)+1;
		save = (CHAR *)emalloc(amt * SIZEOFCHAR, el_tempcluster);
		if (save == 0) return;
		(void)estrcpy(save, msg);
		efree((CHAR *)dia->dlgresaddr->list[item].data);
		dia->dlgresaddr->list[item].data = (INTBIG)save;

		/* redisplay the item */
		if (puretype == MESSAGE)
			Dstuffmessage(dia, msg, &dia->dlgresaddr->list[item].r, 0); else
				Dstufftext(dia, msg, &dia->dlgresaddr->list[item].r);
		if (puretype == EDITTEXT)
		{
			if (highlight != 0)
			{
				Ddoneedit(dia);
				oldcur = item;
				dia->editstart = 0;
				dia->editend = estrlen(msg);
				dia->firstch = 0;
			} else if (oldcur == item)
			{
				dia->editstart = dia->editend = estrlen(msg);
			}
		}
	}
	dia->curitem = oldcur;
	Dhighlight(dia, 1);
}

/*
 * Routine to return the text in item "item"
 */
CHAR *DiaGetText(void *vdia, INTBIG item)
{
	INTBIG type;
	POPUPDATA *pd;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return(x_(""));
	type = dia->dlgresaddr->list[item].type & ITEMTYPE;
	if (type == POPUP)
	{
		pd = (POPUPDATA *)dia->dlgresaddr->list[item].data;
		return(pd->namelist[pd->current]);
	}
	if (type == MESSAGE || type == EDITTEXT ||
		type == BUTTON || type == DEFBUTTON)
			return((CHAR *)dia->dlgresaddr->list[item].data);
	if (type == CHECK || type == RADIO)
		return(&((CHAR *)dia->dlgresaddr->list[item].data)[1]);
	return(0);
}

/*
 * Routine to set the value in item "item" to "value"
 */
void DiaSetControl(void *vdia, INTBIG item, INTBIG value)
{
	INTBIG type;
	RECTAREA r;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type & ITEMTYPE;
	r = dia->dlgresaddr->list[item].r;
	if (type == CHECK)
	{
		/* check box */
		r.right = r.left + 12;
		r.top = (r.top + r.bottom) / 2 - 6;
		r.bottom = r.top + 12;
		Dintdrawrect(dia, &r, 255, 255, 255);
		Ddrawrectframe(dia, &r, 1, 0);
		if (value != 0)
		{
			Ddrawline(dia, r.left, r.top, r.right-1, r.bottom-1);
			Ddrawline(dia, r.left, r.bottom-1, r.right-1, r.top);
		}
		((CHAR *)dia->dlgresaddr->list[item].data)[0] = (CHAR)value;
	} else if (type == RADIO)
	{
		/* radio button */
		r.right = r.left + 12;
		r.top = (r.top + r.bottom) / 2 - 6;
		r.bottom = r.top + 12;
		Dintdrawrect(dia, &r, 255, 255, 255);
		Ddrawcircle(dia, &r, 0);
		if (value != 0)
		{
			Dinsetrect(&r, 3);
			Ddrawdisc(dia, &r);
		}
		((CHAR *)dia->dlgresaddr->list[item].data)[0] = (CHAR)value;
	}
}

/*
 * Routine to return the value in item "item"
 */
INTBIG DiaGetControl(void *vdia, INTBIG item)
{
	INTBIG type;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return(0);
	type = dia->dlgresaddr->list[item].type & ITEMTYPE;
	if (type == CHECK || type == RADIO)
		return((INTBIG)((CHAR *)dia->dlgresaddr->list[item].data)[0]);
	return(0);
}

/*
 * Routine to check item "item" to make sure that there is
 * text in it.  If so, it returns true.  Otherwise it beeps and returns false.
 */
BOOLEAN DiaValidEntry(void *vdia, INTBIG item)
{
	CHAR *msg;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	msg = DiaGetText(dia, item);
	while (*msg == ' ' || *msg == '\t') msg++;
	if (*msg != 0) return(TRUE);
	ttybeep(SOUNDBEEP, TRUE);
	return(FALSE);
}

/*
 * Routine to dim item "item"
 */
void DiaDimItem(void *vdia, INTBIG item)
{
	CHAR *msg;
	INTBIG type;
	RECTAREA r;
	TDIALOG *dia;
	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	if (item == dia->curitem) Ddoneedit(dia);
	dia->dlgresaddr->list[item].type |= INACTIVE;
	type = dia->dlgresaddr->list[item].type;
	r = dia->dlgresaddr->list[item].r;
	msg = (CHAR *)dia->dlgresaddr->list[item].data;
	if ((type&ITEMTYPE) == CHECK || (type&ITEMTYPE) == RADIO) msg++;
	Ddrawitem(dia, type, &r, msg, 1);
}

/*
 * Routine to un-dim item "item"
 */
void DiaUnDimItem(void *vdia, INTBIG item)
{
	CHAR *msg;
	INTBIG type;
	RECTAREA r;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	dia->dlgresaddr->list[item].type &= ~INACTIVE;
	type = dia->dlgresaddr->list[item].type;
	r = dia->dlgresaddr->list[item].r;
	msg = (CHAR *)dia->dlgresaddr->list[item].data;
	if ((type&ITEMTYPE) == CHECK || (type&ITEMTYPE) == RADIO) msg++;
	Ddrawitem(dia, type, &r, msg, 0);

	/* if undimming selected radio button, redraw disc */
	if ((type&ITEMTYPE) == RADIO && ((CHAR *)dia->dlgresaddr->list[item].data)[0] != 0)
	{
		r.right = r.left + 9;
		r.left += 3;
		r.top = (r.top + r.bottom) / 2 - 3;
		r.bottom = r.top + 6;
		Ddrawdisc(dia, &r);
	}
}

/*
 * Routine to change item "item" to be a message rather
 * than editable text
 */
void DiaNoEditControl(void *vdia, INTBIG item)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	if (item == dia->curitem) Ddoneedit(dia);
	dia->dlgresaddr->list[item].type = MESSAGE;
	Deditbox(dia, &dia->dlgresaddr->list[item].r, 0, 0);
}

/*
 * Routine to change item "item" to be editable text rather
 * than a message
 */
void DiaEditControl(void *vdia, INTBIG item)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	dia->dlgresaddr->list[item].type = EDITTEXT;
	Deditbox(dia, &dia->dlgresaddr->list[item].r, 1, 0);
}

void DiaOpaqueEdit(void *vdia, INTBIG item)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	if (dia->dlgresaddr->list[item].type != EDITTEXT) return;
	dia->opaqueitem = item;
}

/*
 * Routine to cause item "item" to be the default button
 */
void DiaDefaultButton(void *vdia, INTBIG item)
{
	INTBIG olddefault;
	INTBIG itemtype, dim;
	RECTAREA r;
	CHAR *line;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	if (item == dia->defaultbutton) return;
	olddefault = dia->defaultbutton - 1;
	dia->defaultbutton = item;

	/* redraw the old item without highlighting */
	itemtype = dia->dlgresaddr->list[olddefault].type;
	r = dia->dlgresaddr->list[olddefault].r;
	Dinsetrect(&r, -4);  r.right++;
	Dintdrawrect(dia, &r, 255, 255, 255);
	Dinsetrect(&r, 4);   r.right--;
	line = (CHAR *)dia->dlgresaddr->list[olddefault].data;
	if ((itemtype&INACTIVE) != 0) dim = 1; else dim = 0;
	Ddrawitem(dia, itemtype, &r, line, dim);

	/* highlight the new default button */
	r = dia->dlgresaddr->list[item-1].r;
	Dhighlightrect(dia, &r);
}

/*
 * Routine to change the icon in item "item" to be the 32x32 bitmap (128 bytes) at "addr".
 */
void DiaChangeIcon(void *vdia, INTBIG item, UCHAR1 *addr)
{
	INTBIG type;
	RECTAREA r;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type;
	if ((type&ITEMTYPE) != ICON) return;
	dia->dlgresaddr->list[item].data = (INTBIG)addr;
	r = dia->dlgresaddr->list[item].r;
	Ddrawitem(dia, type, &r, (CHAR *)addr, 0);
}

/*
 * Routine to change item "item" into a popup with "count" entries
 * in "names".
 */
void DiaSetPopup(void *vdia, INTBIG item, INTBIG count, CHAR **names)
{
	POPUPDATA *pd;
	INTBIG i, type;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type;
	if ((type&ITEMTYPE) != POPUP) return;

	/* copy into a POPUPDATA structure */
	pd = (POPUPDATA *)dia->dlgresaddr->list[item].data;
	for(i=0; i<pd->count; i++) efree((CHAR *)pd->namelist[i]);
	if (pd->count > 0) efree((CHAR *)pd->namelist);
	pd->count = count;
	pd->current = 0;
	pd->namelist = (CHAR **)emalloc(count * (sizeof (CHAR *)), el_tempcluster);
	if (pd->namelist == 0) return;
	for(i=0; i<count; i++)
	{
		pd->namelist[i] = (CHAR *)emalloc((estrlen(names[i])+1) * SIZEOFCHAR,
			el_tempcluster);
		if (pd->namelist[i] == 0) return;
		(void)estrcpy(pd->namelist[i], names[i]);
	}

	/* display the popup */
	Ddrawitem(dia, POPUP, &dia->dlgresaddr->list[item].r, (CHAR *)pd, 0);
}

/*
 * Routine to change popup item "item" so that the current entry is "entry".
 */
void DiaSetPopupEntry(void *vdia, INTBIG item, INTBIG entry)
{
	POPUPDATA *pd;
	INTBIG type;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type;
	if ((type&ITEMTYPE) != POPUP) return;
	pd = (POPUPDATA *)dia->dlgresaddr->list[item].data;
	if (entry < 0 || entry >= pd->count) return;
	pd->current = entry;

	Ddrawitem(dia, POPUP, &dia->dlgresaddr->list[item].r, (CHAR *)pd,
		type&INACTIVE);
}

/*
 * Routine to return the current item in popup menu item "item".
 */
INTBIG DiaGetPopupEntry(void *vdia, INTBIG item)
{
	POPUPDATA *pd;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return(0);
	if ((dia->dlgresaddr->list[item].type&ITEMTYPE) != POPUP) return(0);
	pd = (POPUPDATA *)dia->dlgresaddr->list[item].data;
	return(pd->current);
}

void DiaInitTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos, INTBIG flags)
{
	DSCROLL *scr;
	INTBIG sc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return;
	scr = &dia->scroll[sc];

	/* save information about this scroll area */
	scr->flags = flags;
	if ((scr->flags&SCSMALLFONT) != 0)
	{
		scr->lineheight = dia->slineheight;
		scr->lineoffset = dia->slineoffset;
	} else
	{
		scr->lineheight = dia->lineheight;
		scr->lineoffset = dia->lineoffset;
	}

	/* compute size of actual area (not including scroll bars) */
	scr->userrect = dia->dlgresaddr->list[item].r;
	scr->userrect.right -= 14;
	if ((scr->flags&SCHORIZBAR) != 0) scr->userrect.bottom -= 14;
	scr->linesinfolder = (scr->userrect.bottom - scr->userrect.top) / scr->lineheight;

	/* draw sliders */
	Ddrawvertslider(dia, sc);
	if ((scr->flags&SCHORIZBAR) != 0) Ddrawhorizslider(dia, sc);

	/* load the text */
	scr->scrolllistlen = 0;
	DiaLoadTextDialog(dia, item+1, toplist, nextinlist, donelist, sortpos);
}

void DiaLoadTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos)
{
	INTBIG i, items, sc;
	CHAR *next, **list, line[256];
	DSCROLL *scr;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return;
	scr = &dia->scroll[sc];

	/* deallocate all former items in the list */
	for(i=0; i<scr->scrolllistlen; i++) efree((CHAR *)scr->scrolllist[i]);
	scr->scrolllistlen = 0;
	scr->firstline = 0;

	/* count the number of items to be put in the text editor */
	line[0] = 0;
	next = line;
	(void)(*toplist)(&next);
	for(items=0; ; items++) if ((*nextinlist)() == 0) break;
	(*donelist)();

	/* allocate space for the strings */
	if (items > 0)
	{
		list = (CHAR **)emalloc(items * (sizeof (CHAR *)), el_tempcluster);
		if (list == 0) return;
	}

	/* get the list */
	line[0] = 0;
	next = line;
	(void)(*toplist)(&next);
	for(i=0; i<items; i++)
	{
		next = (*nextinlist)();
		if (next == 0) next = x_("???");
		list[i] = (CHAR *)emalloc((estrlen(next)+1) * SIZEOFCHAR, el_tempcluster);
		if (list[i] == 0) return;
		estrcpy(list[i], next);
	}
	(*donelist)();

	/* sort the list */
	if (sortpos >= 0)
	{
		gra_dialogstringpos = sortpos;
		esort(list, items, sizeof (CHAR *), gra_stringposascending);
	}

	/* stuff the list into the text editor */
	scr->whichlow = -1;
	Dinsetrect(&scr->userrect, 1);
	Dintdrawrect(dia, &scr->userrect, 255, 255, 255);
	Dinsetrect(&scr->userrect, -1);
	for(i=0; i<items; i++) DiaStuffLine(dia, item+1, list[i]);
	Dsetvscroll(dia, sc);
	if (scr->scrolllistlen > 0) DiaSelectLine(dia, item+1, 0);

	/* deallocate the list */
	if (items > 0)
	{
		for(i=0; i<items; i++) efree((CHAR *)list[i]);
		efree((CHAR *)list);
	}
}

/*
 * Routine to stuff line "line" at the end of the edit buffer.
 */
void DiaStuffLine(void *vdia, INTBIG item, CHAR *line)
{
	CHAR **newlist, *pt;
	INTBIG i, sc;
	DSCROLL *scr;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return;
	scr = &dia->scroll[sc];

	if (scr->scrolllistlen >= scr->scrolllistsize)
	{
		newlist = (CHAR **)emalloc((scr->scrolllistsize+10) * (sizeof (CHAR *)),
			el_tempcluster);
		if (newlist == 0) return;
		for(i=0; i<scr->scrolllistlen; i++) newlist[i] = scr->scrolllist[i];
		if (scr->scrolllistsize != 0) efree((CHAR *)scr->scrolllist);
		scr->scrolllist = newlist;
		scr->scrolllistsize += 10;
	}
	pt = (CHAR *)emalloc((estrlen(line)+1) * SIZEOFCHAR, el_tempcluster);
	if (pt == 0) return;
	(void)estrcpy(pt, line);
	if (scr->scrolllistlen < scr->firstline+scr->linesinfolder)
		Ddrawmsg(dia, sc, line, scr->scrolllistlen);
	scr->scrolllist[scr->scrolllistlen++] = pt;
}

/*
 * Routine to select line "line" of scroll item "item".
 */
void DiaSelectLine(void *vdia, INTBIG item, INTBIG l)
{
	INTBIG lines[1];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	lines[0] = l;
	DiaSelectLines(dia, item, 1, lines);
}

/*
 * Routine to select "count" lines in "lines" of scroll item "item".
 */
void DiaSelectLines(void *vdia, INTBIG item, INTBIG count, INTBIG *lines)
{
	INTBIG n, sc, whichlow, whichhigh, i;
	DSCROLL *scr;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return;
	scr = &dia->scroll[sc];
	if (count == 0)
	{
		Dinvertentry(dia, sc, 0);
		scr->whichlow = -1;
		return;
	}

	whichlow = whichhigh = lines[0];
	for(i=1; i<count; i++)
	{
		if (lines[i] < whichlow) whichlow = lines[i];
		if (lines[i] > whichhigh) whichhigh = lines[i];
	}

	/* if the new line is visible, simply shift the highlighting */
	if (whichhigh-scr->firstline >= 0 && whichlow-scr->firstline < scr->linesinfolder)
	{
		Dinvertentry(dia, sc, 0);
	} else
	{
		/* must shift the buffer */
		if (whichlow < 0) Dinvertentry(dia, sc, 0); else
		{
			n = whichlow - scr->linesinfolder/2;
			if (n > scr->scrolllistlen-scr->linesinfolder)
				n = scr->scrolllistlen-scr->linesinfolder;
			if (n < 0) n = 0;
			scr->firstline = n;
			Dredrawscroll(dia, sc);
		}
	}
	scr->whichlow = whichlow;
	scr->whichhigh = whichhigh;
	Dinvertentry(dia, sc, 1);
	Dsetvscroll(dia, sc);
}

/*
 * Returns the currently selected line in the scroll list "item".
 */
INTBIG DiaGetCurLine(void *vdia, INTBIG item)
{
	INTBIG sc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return(-1);
	for(sc=0; sc<dia->scrollcount; sc++)
		if (dia->scroll[sc].scrollitem == item) return(dia->scroll[sc].whichlow);
	return(-1);
}

/*
 * Returns the currently selected lines in the scroll list "item".  The returned
 * array is terminated with -1.
 */
INTBIG *DiaGetCurLines(void *vdia, INTBIG item)
{
	static INTBIG sellist[MAXSCROLLMULTISELECT];
	INTBIG sc, i, j;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	sellist[0] = -1;
	if (item < 0 || item >= dia->dlgresaddr->items) return(sellist);
	for(sc=0; sc<dia->scrollcount; sc++)
	{
		if (dia->scroll[sc].scrollitem == item)
		{
			j = 0;
			for(i=dia->scroll[sc].whichlow; i <= dia->scroll[sc].whichhigh; i++)
				sellist[j++] = i;
			sellist[j] = -1;
		}
	}
	return(sellist);
}

INTBIG DiaGetNumScrollLines(void *vdia, INTBIG item)
{
	static CHAR buf[500];
	DSCROLL *scr;
	INTBIG sc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return(0);
	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return(0);
	scr = &dia->scroll[sc];
	return(scr->scrolllistlen);
}

CHAR *DiaGetScrollLine(void *vdia, INTBIG item, INTBIG l)
{
	INTBIG sc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return(x_(""));
	for(sc=0; sc<dia->scrollcount; sc++)
		if (dia->scroll[sc].scrollitem == item)
	{
		if (l < 0 || l >= dia->scroll[sc].scrolllistlen) break;
		return(dia->scroll[sc].scrolllist[l]);
	}
	return(x_(""));
}

void DiaSetScrollLine(void *vdia, INTBIG item, INTBIG l, CHAR *msg)
{
	CHAR *ins;
	INTBIG sc;
	DSCROLL *scr;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return;
	scr = &dia->scroll[sc];

	if (l >= scr->scrolllistlen) DiaStuffLine(dia, item+1, msg); else
	{
		ins = (CHAR *)emalloc((estrlen(msg)+1) * SIZEOFCHAR, el_tempcluster);
		if (ins == 0) return;
		(void)estrcpy(ins, msg);
		efree((CHAR *)scr->scrolllist[l]);
		scr->scrolllist[l] = ins;
	}
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsetvscroll(dia, sc);
}

void DiaSynchVScrolls(void *vdia, INTBIG item1, INTBIG item2, INTBIG item3)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	if (item1 <= 0 || item1 > dia->dlgresaddr->items) return;
	if (item2 <= 0 || item2 > dia->dlgresaddr->items) return;
	if (item3 < 0 || item3 > dia->dlgresaddr->items) return;
	if (dia->numlocks >= MAXLOCKS) return;
	dia->lock1[dia->numlocks] = item1 - 1;
	dia->lock2[dia->numlocks] = item2 - 1;
	dia->lock3[dia->numlocks] = item3 - 1;
	dia->numlocks++;
}

void DiaUnSynchVScrolls(void *vdia)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->numlocks = 0;
}

void DiaItemRect(void *vdia, INTBIG item, RECTAREA *rect)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	*rect = dia->dlgresaddr->list[item].r;
	Dforcedialog(dia);
}

void DiaPercent(void *vdia, INTBIG item, INTBIG p)
{
	RECTAREA r;
	INTBIG type;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type;
	if ((type&ITEMTYPE) != PROGRESS) return;
	r = dia->dlgresaddr->list[item].r;
	if (p == 0)
	{
		Dintdrawrect(dia, &r, 255, 255, 255);
		Ddrawrectframe(dia, &r, 1, 0);
		return;
	}
	r.left++;
	r.right = r.left + (r.right-1-r.left)*p/100;
	r.top++;   r.bottom--;
	Dgrayrect(dia, &r);
	flushscreen();
}

void DiaRedispRoutine(void *vdia, INTBIG item, void (*routine)(RECTAREA*, void*))
{
	INTBIG type;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (item < 0 || item >= dia->dlgresaddr->items) return;
	type = dia->dlgresaddr->list[item].type;
	if ((type&ITEMTYPE) != USERDRAWN) return;
	dia->dlgresaddr->list[item].data = (INTBIG)routine;
}

void DiaAllowUserDoubleClick(void *vdia)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->userdoubleclick = 1;
}

void DiaDrawRect(void *vdia, INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	Dintdrawrect(dia, rect, r, g, b);
}

void DiaInvertRect(void *vdia, INTBIG item, RECTAREA *r)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	Dinvertrect(dia, r);
}

void DiaFrameRect(void *vdia, INTBIG item, RECTAREA *r)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	Dintdrawrect(dia, r, 255, 255, 255);
	Ddrawrectframe(dia, r, 1, 0);
}

void DiaDrawLine(void *vdia, INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	SetPort(dia->theDialog);
	switch (mode)
	{
		case DLMODEON:     PenMode(patCopy);  break;
		case DLMODEOFF:    PenMode(patBic);   break;
		case DLMODEINVERT: PenMode(patXor);   break;

	}
	MoveTo(fx, fy);
	LineTo(tx, ty);
	PenMode(patCopy);
}

void DiaFillPoly(void *vdia, INTBIG item, INTBIG *x, INTBIG *y, INTBIG count, INTBIG r, INTBIG g, INTBIG b)
{
	RGBColor color, oldcolor;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	color.red = r << 8;
	color.green = g << 8;
	color.blue = b << 8;
	SetPort(dia->theDialog);
	GetForeColor(&oldcolor);
	RGBForeColor(&color);
	Ddrawpolygon(dia, x, y, count, 1);
	RGBForeColor(&oldcolor);
}

void DiaPutText(void *vdia, INTBIG item, CHAR *msg, INTBIG x, INTBIG y)
{
	FontInfo fontinfo;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	SetPort(dia->theDialog);
	TextFont(DSFONT);
	TextSize(dia->usertextsize);
	GetFontInfo(&fontinfo);
	MoveTo(x, y + fontinfo.ascent);
	DrawText(msg, 0, estrlen(msg));
}

void DiaSetTextSize(void *vdia, INTBIG size)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->usertextsize = size;
}

void DiaGetTextInfo(void *vdia, CHAR *msg, INTBIG *wid, INTBIG *hei)
{
	FontInfo fontinfo;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	SetPort(dia->theDialog);
	TextFont(DSFONT);
	TextSize(dia->usertextsize);
	GetFontInfo(&fontinfo);
	*wid = TextWidth(msg, 0, estrlen(msg));
	*hei = fontinfo.ascent + fontinfo.descent;
}

void DiaTrackCursor(void *vdia, void (*eachdown)(INTBIG x, INTBIG y))
{
	gra_trackingdialog = (TDIALOG *)vdia;
	gra_trackingdialog->diaeachdown = eachdown;
	trackcursor(FALSE, us_nullup, us_nullvoid, gra_diaeachdownhandler,
		us_nullchar, us_nullvoid, TRACKNORMAL);
}

BOOLEAN gra_diaeachdownhandler(INTBIG ox, INTBIG oy)
{
	INTBIG x, y;

	DiaGetMouse(gra_trackingdialog, &x, &y);
	(void)((*gra_trackingdialog->diaeachdown)(x, y));
	return(FALSE);
}

void DiaGetMouse(void *vdia, INTBIG *x, INTBIG *y)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	readtablet(x, y);
}

/****************************** DIALOG SUPPORT ******************************/

/*
 * Routine to parse the next input event and return the affected item.
 * If the routine returns -1, nothing has happened.
 */
INTBIG gra_getnextcharacter(TDIALOG *dia, INTBIG oak, INTBIG x, INTBIG y, INTBIG chr,
	INTBIG special, UINTBIG time, BOOLEAN shifted)
{
	RECTAREA r;
	CHAR *msg;
	INTBIG thumbval;
	static CHAR match[MAXMATCH];
	static INTBIG matchpos = 0;
	static UINTBIG lasttime = 0;
	INTBIG i, j, type, which, v, t, n, sc, newfirst, item;
	INTBIG selitems[MAXSCROLLMULTISELECT];
	DSCROLL *scr;
	POPUPDATA *pd;
	CHAR ch[2];

	if (oak == 1 || oak == 5)
	{
		/* hit in the window: find the item */
		item = Dwhichitem(dia, x, y);
		if (item == 0) return(-1);
		type = dia->dlgresaddr->list[item-1].type;
		r = dia->dlgresaddr->list[item-1].r;

		if ((type&ITEMTYPE) == MESSAGE || (type&INACTIVE) != 0) return(-1);

		/* if the item is a popup menu, display it and track */
		if ((type&ITEMTYPE) == POPUP)
		{
			pd = (POPUPDATA *)dia->dlgresaddr->list[item-1].data;
			i = Dhandlepopup(dia, &r, pd);
			if (i == pd->current) return(-1);
			pd->current = i;
			Ddrawitem(dia, POPUP, &dia->dlgresaddr->list[item-1].r, (CHAR *)pd, 0);
			return(item);
		}

		/* items under the user's control are given to the user */
		if (type == USERDRAWN)
		{
			/* double click in user selects and returns */
			if (oak == 5 && dia->userdoubleclick != 0)
			{
				item = dia->defaultbutton;
			}
			return(item);
		}
		/* if the item is edit text, make it the current one */
		if (type == EDITTEXT)
		{
			if (dia->curitem != item - 1) Ddoneedit(dia); else Dhighlight(dia, 0);
			dia->curitem = item - 1;
			msg = (CHAR *)dia->dlgresaddr->list[item-1].data;
			i = Dgeteditpos(dia, &r, x, y, msg);
			if (oak == 5)
			{
				/* look for a full word about position "base" */
				for(dia->editstart=i-1; dia->editstart>=0; dia->editstart--)
					if (!isalnum(msg[dia->editstart])) break;
				dia->editstart++;
				for(dia->editend = dia->editstart; msg[dia->editend] != 0; dia->editend++)
					if (!isalnum(msg[dia->editend])) break;
				Dhighlight(dia, 1);
			} else
			{
				dia->editstart = dia->editend = i;
				Dhighlight(dia, 1);
			}
			dia->lbase = dia->editstart;   dia->hbase = dia->editend;
			dia->rect = r;
			dia->msg = msg;
			Dtrackcursor(dia, x, y, Deditdown);
			return(item);
		}

		/* if the item is a button, reverse it and track */
		if (type == BUTTON || type == DEFBUTTON)
		{
			r.left++;   r.right--;
			r.top++;    r.bottom--;
			Dinvertroundrect(dia, &r);
			dia->rect = r;
			dia->wasin = 1;
			Dtrackcursor(dia, x, y, Dbuttondown);
			if (dia->wasin == 0) return(-1);
			Dinvertroundrect(dia, &r);
			return(item);
		}
		/* if the item is a check, outline it and track */
		if (type == CHECK)
		{
			dia->rect = r;
			r.right = r.left + 11;   r.left++;
			r.top = (r.top + r.bottom) / 2 - 5;
			r.bottom = r.top + 10;
			Ddrawrectframe(dia, &r, 1, 0);
			dia->wasin = 1;
			dia->ddata = ((CHAR *)dia->dlgresaddr->list[item-1].data)[0];
			Dtrackcursor(dia, x, y, Dcheckdown);
			if (dia->wasin == 0) return(-1);
			Ddrawrectframe(dia, &r, 0, 0);
			if (dia->ddata != 0)
			{
				Ddrawline(dia, r.left, r.top, r.right-1, r.bottom-1);
				Ddrawline(dia, r.left, r.bottom-1, r.right-1, r.top);
			}
			return(item);
		}

		/* if the item is a scroll area, select a line */
		if (type == SCROLL || type == SCROLLMULTI)
		{
			for(sc=0; sc<dia->scrollcount; sc++)
				if (dia->scroll[sc].scrollitem == item - 1) break;
			if (sc >= dia->scrollcount) return(-1);
			scr = &dia->scroll[dia->curscroll=sc];

			if (x > scr->userrect.right)
			{
				/* cursor in vertical slider */
				if (scr->scrolllistlen <= scr->linesinfolder) return(-1);
				if (y > scr->userrect.bottom-16)
				{
					/* the down arrow */
					Ddrawarrow(dia, sc, DOWNARROW, 1);
					Dtrackcursor(dia, x, y, Ddownarrow);
					Ddrawarrow(dia, sc, DOWNARROW, 0);
					Dsyncvscroll(dia, item-1);
					return(-1);
				}
				if (y < scr->userrect.top+16)
				{
					/* the up arrow */
					Ddrawarrow(dia, sc, UPARROW, 1);
					Dtrackcursor(dia, x, y, Duparrow);
					Ddrawarrow(dia, sc, UPARROW, 0);
					Dsyncvscroll(dia, item-1);
					return(-1);
				}
				if (y > scr->vthumbpos+THUMBSIZE/2 && y <= scr->userrect.bottom-16)
				{
					/* scroll down one page */
					Dtrackcursor(dia, x, y, Ddownpage);
					Dsyncvscroll(dia, item-1);
					return(-1);
				}
				if (y < scr->vthumbpos-THUMBSIZE/2 && y >= scr->userrect.top+16)
				{
					/* scroll up one page */
					Dtrackcursor(dia, x, y, Duppage);
					Dsyncvscroll(dia, item-1);
					return(-1);
				}
				if (y >= scr->vthumbpos-THUMBSIZE/2 && y <= scr->vthumbpos+THUMBSIZE/2)
				{
					/* drag slider appropriately */
					v = y;   t = scr->vthumbpos;
					dia->rect = dia->dlgresaddr->list[item-1].r;
					dia->rect.left = dia->rect.right - 14;
					dia->rect.right--;
					dia->rect.top = scr->vthumbpos - THUMBSIZE/2;
					dia->rect.bottom = scr->vthumbpos + THUMBSIZE/2;
					Dinvertrectframe(dia, &dia->rect);
					dia->offset = t-v;
					Dtrackcursor(dia, x, y, Dvscroll);
					dia->rect.top = scr->vthumbpos - THUMBSIZE/2;
					dia->rect.bottom = scr->vthumbpos + THUMBSIZE/2;
					Dinvertrectframe(dia, &dia->rect);
					r = scr->userrect;
					r.top += 16;   r.bottom -= 16;
					thumbval = scr->vthumbpos - r.top - THUMBSIZE/2;
					thumbval *= scr->scrolllistlen - scr->linesinfolder;
					thumbval /= r.bottom - r.top - THUMBSIZE;
					i = thumbval;
					if (i < 0) i = 0;
					if (i == scr->firstline) return(-1);
					scr->firstline = i;
					Dredrawscroll(dia, sc);
					Dinvertentry(dia, sc, 1);
					Dsetvscroll(dia, sc);
					Dsyncvscroll(dia, item-1);
					return(-1);
				}
			} else if (y > scr->userrect.bottom)
			{
				/* cursor in horizontal slider */
				if (x > scr->userrect.right-16)
				{
					/* the right arrow */
					Ddrawarrow(dia, sc, RIGHTARROW, 1);
					Dtrackcursor(dia, x, y, Drightarrow);
					Ddrawarrow(dia, sc, RIGHTARROW, 0);
					return(-1);
				}
				if (x < scr->userrect.left+16)
				{
					/* the left arrow */
					Ddrawarrow(dia, sc, LEFTARROW, 1);
					Dtrackcursor(dia, x, y, Dleftarrow);
					Ddrawarrow(dia, sc, LEFTARROW, 0);
					return(-1);
				}
				if (x > scr->hthumbpos+THUMBSIZE/2 && x <= scr->userrect.right-16)
				{
					/* scroll right one page */
					Dtrackcursor(dia, x, y, Drightpage);
					return(-1);
				}
				if (x < scr->hthumbpos-THUMBSIZE/2 && x >= scr->userrect.left+16)
				{
					/* scroll left one page */
					Dtrackcursor(dia, x, y, Dleftpage);
					return(-1);
				}
				if (x >= scr->hthumbpos-THUMBSIZE/2 && x <= scr->hthumbpos+THUMBSIZE/2)
				{
					/* drag slider appropriately */
					v = x;   t = scr->hthumbpos;
					dia->rect = dia->dlgresaddr->list[item-1].r;
					dia->rect.top = dia->rect.bottom - 14;
					dia->rect.bottom--;
					dia->rect.left = scr->hthumbpos - THUMBSIZE/2;
					dia->rect.right = scr->hthumbpos + THUMBSIZE/2;
					Dinvertrectframe(dia, &dia->rect);
					dia->offset = t - v;
					Dtrackcursor(dia, x, y, Dhscroll);
					dia->rect.left = scr->hthumbpos - THUMBSIZE/2;
					dia->rect.right = scr->hthumbpos + THUMBSIZE/2;
					Dinvertrectframe(dia, &dia->rect);
					r = scr->userrect;
					r.left += 16;   r.right -= 16;
					thumbval = scr->hthumbpos - r.left - THUMBSIZE/2;
					thumbval *= 100;
					thumbval /= r.right - r.left - THUMBSIZE;
					i = thumbval;
					if (i < 0) i = 0;
					if (i == scr->horizfactor) return(-1);
					scr->horizfactor = i;
					Dredrawscroll(dia, sc);
					Dinvertentry(dia, sc, 1);
					Dsethscroll(dia, sc);
					return(-1);
				}
			} else
			{
				/* double click in scroll selects and returns */
				if (oak == 5 && scr->scrolllistlen > 0 && (scr->flags&SCDOUBLEQUIT) != 0)
					return(dia->defaultbutton);

				if ((scr->flags&SCSELMOUSE) != 0)
				{
					/* cursor in list: select an entry */
					which = y - r.top;
					which /= scr->lineheight;
					which += scr->firstline;
					if (which >= scr->scrolllistlen) which = scr->scrolllistlen - 1;
					if (type == SCROLLMULTI && shifted && scr->whichlow >= 0)
					{
						if (which < scr->whichlow)
						{
							j = 0;
							for(i=which; i<= scr->whichhigh; i++)
								selitems[j++] = i;
							DiaSelectLines(dia, item, j, selitems);
						} else if (which > scr->whichhigh)
						{
							j = 0;
							for(i=scr->whichlow; i<= which; i++)
								selitems[j++] = i;
							DiaSelectLines(dia, item, j, selitems);
						} else
						{
							if (which - scr->whichlow < scr->whichhigh - which)
							{
								j = 0;
								for(i=which; i<= scr->whichhigh; i++)
									selitems[j++] = i;
								DiaSelectLines(dia, item, j, selitems);
							} else
							{
								j = 0;
								for(i=scr->whichlow; i<= which; i++)
									selitems[j++] = i;
								DiaSelectLines(dia, item, j, selitems);
							}
						}

					} else
					{
						DiaSelectLine(dia, item, which);
					}
					if ((scr->flags&SCREPORT) == 0) return(-1);
				}
			}
		}
		return(item);
	}

	/* get the character, return immediately if special */
	if (oak != 0) return(-1);
	if (chr == ESCKEY) return(CANCEL);
	if (chr == '\n' || chr == '\r' || chr == CTRLCKEY) return(dia->defaultbutton);

	/* handle arrow positioning */
	if ((special&SPECIALKEYDOWN) != 0)
	{
		switch ((special&SPECIALKEY)>>SPECIALKEYSH)
		{
			case SPECIALKEYARROWL:
				if (dia->curitem < 0) return(-1);
				Dhighlight(dia, 0);
				dia->editstart--;
				if (dia->editstart < 0) dia->editstart = 0;
				dia->editend = dia->editstart;
				newfirst = Dneweditbase(dia);
				if (dia->firstch != newfirst)
				{
					dia->firstch = newfirst;
					msg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
					Dstufftext(dia, msg, &dia->dlgresaddr->list[dia->curitem].r);
				}
				Dhighlight(dia, 1);
				return(-1);
			case SPECIALKEYARROWR:
				if (dia->curitem < 0) return(-1);
				Dhighlight(dia, 0);
				msg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
				if (msg[dia->editend] != 0) dia->editend++;
				dia->editstart = dia->editend;
				newfirst = Dneweditbase(dia);
				if (dia->firstch != newfirst)
				{
					dia->firstch = newfirst;
					Dstufftext(dia, msg, &dia->dlgresaddr->list[dia->curitem].r);
				}
				Dhighlight(dia, 1);
				return(-1);
			case SPECIALKEYARROWD:
				if (dia->scrollcount <= 0) return(-1);
				scr = &dia->scroll[sc = dia->curscroll];
				if (scr->scrolllistlen <= 0) return(-1);
				if (scr->whichlow >= scr->scrolllistlen-1) return(-1);
				item = scr->scrollitem + 1;
				DiaSelectLine(dia, item, scr->whichlow+1);
				if ((scr->flags&SCREPORT) == 0) return(-1);
				return(item);
			case SPECIALKEYARROWU:
				if (dia->scrollcount <= 0) return(-1);
				scr = &dia->scroll[sc = dia->curscroll];
				if (scr->scrolllistlen <= 0) return(-1);
				if (scr->whichlow <= 0) return(-1);
				item = scr->scrollitem + 1;
				DiaSelectLine(dia, item, scr->whichlow-1);
				if ((scr->flags&SCREPORT) == 0) return(-1);
				return(item);
		}
	}

	/* tab to next edit text item */
	if (chr == TABKEY)
	{
		type = 0;
		for(i=dia->curitem+1; i<dia->dlgresaddr->items; i++)
		{
			type = dia->dlgresaddr->list[i].type;
			if (type == EDITTEXT) break;
		}
		if (type != EDITTEXT)
		{
			for(i=0; i<dia->curitem; i++)
			{
				type = dia->dlgresaddr->list[i].type;
				if (type == EDITTEXT) break;
			}
		}
		if (type != EDITTEXT) return(-1);
		Ddoneedit(dia);
		dia->curitem = i;
		dia->editstart = 0;
		msg = (CHAR *)dia->dlgresaddr->list[i].data;
		dia->editend = estrlen(msg);
		dia->firstch = 0;
		Dhighlight(dia, 1);
		return(-1);
	}

	if (dia->curitem < 0 && dia->scrollcount > 0 &&
		(dia->scroll[dia->curscroll].flags&SCSELKEY) != 0)
	{
		/* use key to select line in scroll area */
		scr = &dia->scroll[sc=dia->curscroll];
		item = scr->scrollitem + 1;
		if (chr >= 'A' && chr <= 'Z') chr += 040;

		/* if it has been more than a second, reset the match string */
		if (time - lasttime > 60) matchpos = 0;
		lasttime = time;

		/* add this character to the match string */
		if (matchpos < MAXMATCH)
		{
			match[matchpos] = chr;
			matchpos++;
		}

		/* find that string */
		for(which = 0; which < scr->scrolllistlen; which++)
		{
			for(i=0; i<matchpos; i++)
			{
				n = scr->scrolllist[which][i];
				if (n >= 'A' && n <= 'Z') n += 040;
				if (match[i] != n) break;
			}
			if (i >= matchpos)
			{
				DiaSelectLine(dia, item, which);
				break;
			}
		}
		if ((scr->flags&SCREPORT) == 0) return(-1);
		return(item);
	}

	/* handle delete/backspace key */
	if ((chr == BACKSPACEKEY || chr == DELETEKEY) && dia->curitem >= 0)
	{
		if (dia->editstart == dia->editend && dia->editstart > 0)
			dia->editstart--;
		chr = 0;
	}

	/* insert character into edit text */
	ch[0] = chr;   ch[1] = 0;
	Dinsertstr(dia, ch);
	return(dia->curitem + 1);
}

BOOLEAN Dbuttondown(INTBIG x, INTBIG y)
{
	INTBIG in;
	TDIALOG *dia;

	dia = gra_trackingdialog;
	if (x < dia->rect.left || x > dia->rect.right || y < dia->rect.top || y > dia->rect.bottom)
		in = 0; else
			in = 1;
	if (in != dia->wasin)
		Dinvertroundrect(dia, &dia->rect);
	dia->wasin = in;
	return(FALSE);
}

BOOLEAN Dtrackpopup(INTBIG x, INTBIG y)
{
	INTBIG which;
	RECTAREA r;
	TDIALOG *dia;

	dia = gra_trackingdialog;
	if (x < dia->rect.left || x > dia->rect.right || y < dia->rect.top || y > dia->rect.bottom)
		which = -1; else
	{
		which = (y - dia->rect.top - 1) / dia->lineheight;
		if (which >= dia->cnt-dia->sta) which = dia->cnt-dia->sta-1;

	}
	if (which != dia->wasin)
	{
		r.left = dia->rect.left + 1;
		r.right = dia->rect.right - 1;
		if (dia->wasin >= 0)
		{
			r.top = dia->rect.top + (dia->wasin * dia->lineheight) + 1;
			r.bottom = r.top + dia->lineheight;
			Dinvertrect(dia, &r);
		}

		/* special case if in the "up" arrow */
		if (which == 0 && dia->sta != 0)
		{
			dia->sta--;   dia->cnt--;
			Ddrawpopupentries(dia);
			dia->wasin = -1;
			return(FALSE);
		}

		/* special case if in the "down" arrow */
		if (which == dia->cnt-dia->sta-1 && dia->cnt != dia->pd->count)
		{
			dia->sta++;   dia->cnt++;
			Ddrawpopupentries(dia);
			dia->wasin = -1;
			return(FALSE);
		}

		if (which >= 0)
		{
			r.top = dia->rect.top + (which * dia->lineheight) + 1;
			r.bottom = r.top + dia->lineheight;
			Dinvertrect(dia, &r);
		}

		dia->wasin = which;
	}
	return(FALSE);
}

BOOLEAN Dcheckdown(INTBIG x, INTBIG y)
{
	INTBIG in;
	RECTAREA r;
	TDIALOG *dia;

	dia = gra_trackingdialog;
	if (x < dia->rect.left || x > dia->rect.right || y < dia->rect.top || y > dia->rect.bottom)
		in = 0; else
			in = 1;
	if (in != dia->wasin)
	{
		r = dia->rect;
		r.right = r.left + 11;   r.left++;
		r.top = (r.top + r.bottom) / 2 - 5;
		r.bottom = r.top + 10;
		if (in != 0) Ddrawrectframe(dia, &r, 1, 0); else
		{
			Ddrawrectframe(dia, &r, 0, 0);
			if (dia->ddata != 0)
			{
				Ddrawline(dia, r.left, r.top, r.right-1, r.bottom-1);
				Ddrawline(dia, r.left, r.bottom-1, r.right-1, r.top);
			}
		}
	}
	dia->wasin = in;
	return(FALSE);
}

BOOLEAN Deditdown(INTBIG x, INTBIG y)
{
	INTBIG l, h, pos, wid, len, prevpos, basepos;
	CHAR *msg;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* shift the text if the cursor moves outside the field */
	if (dia->rect.bottom-dia->rect.top < dia->lineheight*2)
	{
		if (y < dia->rect.top || y > dia->rect.bottom) return(FALSE);

		/* scroll horizontally if there is only 1 line in the edit field */
		if (x < dia->rect.left && dia->firstch > 0)
		{
			Dhighlight(dia, 0);
			dia->firstch--;
			Dstufftext(dia, dia->msg, &dia->rect);
			Dhighlight(dia, 1);
			gotosleep(10);
			return(FALSE);
		}
		if (x > dia->rect.right && dia->firstch < dia->editend-1)
		{
			Dhighlight(dia, 0);
			dia->firstch++;
			Dstufftext(dia, dia->msg, &dia->rect);
			Dhighlight(dia, 1);
			gotosleep(10);
			return(FALSE);
		}
	} else
	{
		if (x < dia->rect.left || x > dia->rect.right) return(FALSE);

		/* scroll vertically if there are multiple lines in the field */
		if (y < dia->rect.top && dia->firstch > 0)
		{
			msg = dia->msg;
			prevpos = 0;
			basepos = 0;
			while (*msg != 0)
			{
				for(len = estrlen(msg); len>0; len--)
				{
					wid = Dgettextsize(dia, msg, len);
					if (wid < dia->rect.right-dia->rect.left-2) break;
				}
				if (len == 0) break;
				basepos += len;
				if (basepos == dia->firstch) break;
				prevpos += len;
				msg += len;
			}
			Dhighlight(dia, 0);
			dia->firstch = prevpos;
			Dstufftext(dia, dia->msg, &dia->rect);
			Dhighlight(dia, 1);
			gotosleep(30);
			return(FALSE);
		}
		if (y > dia->rect.bottom && dia->firstch < dia->editend-1)
		{
			msg = &dia->msg[dia->firstch];
			for(len = estrlen(msg); len>0; len--)
			{
				wid = Dgettextsize(dia, msg, len);
				if (wid < dia->rect.right-dia->rect.left-2) break;
			}
			if (len == estrlen(msg)) return(FALSE);
			Dhighlight(dia, 0);
			dia->firstch += len;
			Dstufftext(dia, dia->msg, &dia->rect);
			Dhighlight(dia, 1);
			gotosleep(30);
			return(FALSE);
		}
	}

	pos = Dgeteditpos(dia, &dia->rect, x, y, dia->msg);
	l = dia->lbase;   h = dia->hbase;
	if (pos > h) h = pos;
	if (pos < l) l = pos;
	if (l != dia->editstart || h != dia->editend)
	{
		Dhighlight(dia, 0);
		dia->editstart = l;   dia->editend = h;
		Dhighlight(dia, 1);
	}
	return(FALSE);
}

BOOLEAN Ddownarrow(INTBIG x, INTBIG y)
{
	short sc;
	DSCROLL *scr;
	short i;
	RECTAREA r, fr, tr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINSCROLLTICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;

	scr = &dia->scroll[sc=dia->curscroll];
	if (scr->scrolllistlen-scr->firstline <= scr->linesinfolder) return(TRUE);

	/* remove highlighting */
	Dinvertentry(dia, sc, 1);
	
	/* shift screen pointer */
	scr->firstline++;

	/* shift the window contents up by 1 line (scr->lineheight) */
	r = scr->userrect;
	fr.left = r.left+1;                 fr.right = r.right-1;
	fr.top = r.top+1+scr->lineheight;   fr.bottom = r.bottom-1;
	tr.left = r.left+1;   tr.right = r.right-1;
	tr.top = r.top+1;     tr.bottom = r.bottom-1-scr->lineheight;
	Dshiftbits(dia, &fr, &tr);

	/* fill in a new last line */
	fr.top = r.bottom-1-scr->lineheight;   fr.bottom = r.bottom-1;
	Dintdrawrect(dia, &fr, 255, 255, 255);
	i = scr->firstline + scr->linesinfolder - 1;
	if (i < scr->scrolllistlen)
		Ddrawmsg(dia, sc, scr->scrolllist[i], i-scr->firstline);

	/* restore highlighting */
	Dinvertentry(dia, sc, 1);

	/* redo thumb position */
	Dsetvscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Duparrow(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	RECTAREA r, fr, tr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINSCROLLTICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;

	scr = &dia->scroll[sc=dia->curscroll];
	if (scr->firstline <= 0) return(TRUE);

	/* remove highlighting */
	Dinvertentry(dia, sc, 1);

	/* shift screen pointer */
	scr->firstline--;

	/* shift the window contents down by 1 line (scr->lineheight) */
	r = scr->userrect;
	fr.left = r.left+1;   fr.right = r.right-1;
	fr.top = r.top+1;     fr.bottom = r.bottom-1-scr->lineheight;

	tr.left = r.left+1;                 tr.right = r.right-1;
	tr.top = r.top+1+scr->lineheight;   tr.bottom = r.bottom-1;
	Dshiftbits(dia, &fr, &tr);

	/* fill in a new top line */
	fr.top = r.top+1;   fr.bottom = r.top+1+scr->lineheight;
	Dintdrawrect(dia, &fr, 255, 255, 255);
	Ddrawmsg(dia, sc, scr->scrolllist[scr->firstline], 0);

	/* restore highlighting */
	Dinvertentry(dia, sc, 1);

	/* redo thumb position */
	Dsetvscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Ddownpage(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINPAGETICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;

	scr = &dia->scroll[sc=dia->curscroll];
	if (y <= scr->vthumbpos+THUMBSIZE/2 || y > scr->userrect.bottom-16) return(TRUE);
	if (scr->scrolllistlen-scr->firstline <= scr->linesinfolder) return(TRUE);
	scr->firstline += scr->linesinfolder-1;
	if (scr->scrolllistlen-scr->firstline <= scr->linesinfolder)
		scr->firstline = scr->scrolllistlen-scr->linesinfolder;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsetvscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Duppage(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINPAGETICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;

	scr = &dia->scroll[sc=dia->curscroll];
	if (y >= scr->vthumbpos-THUMBSIZE/2 || y < scr->userrect.top+16) return(TRUE);
	if (scr->firstline <= 0) return(TRUE);
	scr->firstline -= scr->linesinfolder-1;
	if (scr->firstline < 0) scr->firstline = 0;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsetvscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Dvscroll(INTBIG x, INTBIG y)
{
	INTBIG l, sc;
	DSCROLL *scr;
	TDIALOG *dia;

	dia = gra_trackingdialog;
	scr = &dia->scroll[sc=dia->curscroll];
	l = scr->vthumbpos;
	scr->vthumbpos = y + dia->offset;
	if (scr->vthumbpos < scr->userrect.top+16+THUMBSIZE/2)
		scr->vthumbpos = scr->userrect.top+16+THUMBSIZE/2;
	if (scr->vthumbpos > scr->userrect.bottom-16-THUMBSIZE/2)
		scr->vthumbpos = scr->userrect.bottom-16-THUMBSIZE/2;
	if (scr->vthumbpos == l) return(FALSE);
	dia->rect.top = l - THUMBSIZE/2;
	dia->rect.bottom = l + THUMBSIZE/2;
	Dinvertrectframe(dia, &dia->rect);
	dia->rect.top = scr->vthumbpos - THUMBSIZE/2;
	dia->rect.bottom = scr->vthumbpos + THUMBSIZE/2;
	Dinvertrectframe(dia, &dia->rect);
	return(FALSE);
}

BOOLEAN Drightarrow(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINSCROLLTICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;

	scr = &dia->scroll[sc=dia->curscroll];
	if (scr->horizfactor >= 100) return(TRUE);
	scr->horizfactor++;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsethscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Dleftarrow(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINSCROLLTICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;
	scr = &dia->scroll[sc=dia->curscroll];
	if (scr->horizfactor <= 0) return(TRUE);
	scr->horizfactor--;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsethscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Drightpage(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINPAGETICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;
	scr = &dia->scroll[sc=dia->curscroll];
	if (x <= scr->hthumbpos+THUMBSIZE/2 || x > scr->userrect.right-16) return(TRUE);
	if (scr->horizfactor >= 100) return(TRUE);
	scr->horizfactor += 10;
	if (scr->horizfactor >= 100) scr->horizfactor = 100;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsethscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Dleftpage(INTBIG x, INTBIG y)
{
	INTBIG sc;
	DSCROLL *scr;
	UINTBIG thisdiatime;
	TDIALOG *dia;

	dia = gra_trackingdialog;

	/* limit the speed of the button */
	thisdiatime = ticktime();
	if (thisdiatime - dia->lastdiatime < MINPAGETICKS) return(FALSE);
	dia->lastdiatime = thisdiatime;

	scr = &dia->scroll[sc=dia->curscroll];
	if (x >= scr->hthumbpos-THUMBSIZE/2 || x < scr->userrect.left+16) return(TRUE);
	if (scr->horizfactor <= 0) return(1);
	scr->horizfactor -= 10;
	if (scr->horizfactor <= 0) scr->horizfactor = 0;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsethscroll(dia, sc);
	return(FALSE);
}

BOOLEAN Dhscroll(INTBIG x, INTBIG y)
{
	INTBIG l, sc;
	DSCROLL *scr;
	TDIALOG *dia;

	dia = gra_trackingdialog;
	scr = &dia->scroll[sc=dia->curscroll];
	l = scr->hthumbpos;
	scr->hthumbpos = x + dia->offset;
	if (scr->hthumbpos < scr->userrect.left+16+THUMBSIZE/2)
		scr->hthumbpos = scr->userrect.left+16+THUMBSIZE/2;
	if (scr->hthumbpos > scr->userrect.right-16-THUMBSIZE/2)
		scr->hthumbpos = scr->userrect.right-16-THUMBSIZE/2;
	if (scr->hthumbpos == l) return(FALSE);
	dia->rect.left = l - THUMBSIZE/2;
	dia->rect.right = l + THUMBSIZE/2;
	Dinvertrectframe(dia, &dia->rect);
	dia->rect.left = scr->hthumbpos - THUMBSIZE/2;
	dia->rect.right = scr->hthumbpos + THUMBSIZE/2;
	Dinvertrectframe(dia, &dia->rect);
	return(FALSE);
}

void Dredrawscroll(TDIALOG *dia, INTBIG sc)
{
	RECTAREA r;
	INTBIG i;
	DSCROLL *scr;

	scr = &dia->scroll[sc];
	r = scr->userrect;
	r.left++;        r.right--;
	r.top++;         r.bottom--;
	Dintdrawrect(dia, &r, 255, 255, 255);
	for(i=scr->firstline; i<scr->scrolllistlen; i++)
	{
		if (i-scr->firstline >= scr->linesinfolder) break;
		Ddrawmsg(dia, sc, scr->scrolllist[i], i-scr->firstline);
	}
}

/*
 * Routine to set the vertical scroll bar
 */
void Dsetvscroll(TDIALOG *dia, INTBIG sc)
{
	RECTAREA r;
	INTBIG f;
	DSCROLL *scr;

	/* first redraw the border */
	Ddrawvertslider(dia , sc);

	/* get the area of the slider without arrows */
	scr = &dia->scroll[sc];
	r = scr->userrect;
	r.top += 16;
	r.bottom -= 16;
	r.left = r.right;
	r.right += 13;

	/* if there is nothing to scroll, clear this area */
	if (scr->scrolllistlen <= scr->linesinfolder)
	{
		Dintdrawrect(dia, &r, 255, 255, 255);
		return;
	}

	/* gray the scroll area */
	Dgrayrect(dia, &r);

	/* compute position of vertical thumb area */
	f = scr->firstline;   f *= r.bottom - r.top-THUMBSIZE;
	f /= scr->scrolllistlen - scr->linesinfolder;
	scr->vthumbpos = r.top + THUMBSIZE/2 + f;
	if (scr->vthumbpos > r.bottom-THUMBSIZE/2) scr->vthumbpos = r.bottom-THUMBSIZE/2;

	/* draw the thumb */
	r.top = scr->vthumbpos - THUMBSIZE/2;
	r.bottom = scr->vthumbpos + THUMBSIZE/2;
	Dintdrawrect(dia, &r, 255, 255, 255);
	Ddrawrectframe(dia, &r, 1, 0);
}

/*
 * Routine to see if the vertical scroll in this area is tied to another and to change it.
 */
void Dsyncvscroll(TDIALOG *dia, INTBIG item)
{
	INTBIG value, i;
	DSCROLL *scr;

	scr = &dia->scroll[dia->curscroll];
	value = scr->firstline;
	for(i=0; i<dia->numlocks; i++)
	{
		if (dia->lock1[i] == item)
		{
			Dsetscroll(dia, dia->lock2[i], value);
			if (dia->lock3[i] >= 0)
				Dsetscroll(dia, dia->lock3[i], value);
			break;
		}
		if (dia->lock2[i] == item)
		{
			Dsetscroll(dia, dia->lock1[i], value);
			if (dia->lock3[i] >= 0)
				Dsetscroll(dia, dia->lock3[i], value);
			break;
		}
		if (dia->lock3[i] == item)
		{
			Dsetscroll(dia, dia->lock1[i], value);
			Dsetscroll(dia, dia->lock2[i], value);
			break;
		}
	}
}

/*
 * Routine to set the first line of the scroll list in item "item" to "value"
 */
void Dsetscroll(TDIALOG *dia, INTBIG item, INTBIG value)
{
	REGISTER INTBIG sc;
	DSCROLL *scr;

	for(sc=0; sc<dia->scrollcount; sc++) if (dia->scroll[sc].scrollitem == item) break;
	if (sc >= dia->scrollcount) return;
	scr = &dia->scroll[sc];
	scr->firstline = value;
	Dredrawscroll(dia, sc);
	Dinvertentry(dia, sc, 1);
	Dsetvscroll(dia, sc);
}

/*
 * Routine to set the horizontal scroll bar
 */
void Dsethscroll(TDIALOG *dia, INTBIG sc)
{
	RECTAREA r;
	INTBIG f;
	DSCROLL *scr;

	/* get the area of the slider without arrows */
	scr = &dia->scroll[sc];
	r = scr->userrect;
	r.left += 16;
	r.right -= 16;
	r.top = r.bottom;
	r.bottom += 13;

	/* gray the scroll area */
	Dgrayrect(dia, &r);

	/* compute position of vertical thumb area */
	f = scr->horizfactor;   f *= (INTBIG)(r.right-r.left-THUMBSIZE);   f /= 100;
	scr->hthumbpos = r.left + THUMBSIZE/2 + f;
	if (scr->hthumbpos > r.right-THUMBSIZE/2) scr->hthumbpos = r.right-THUMBSIZE/2;

	/* draw the thumb */
	r.left = scr->hthumbpos - THUMBSIZE/2;
	r.right = scr->hthumbpos + THUMBSIZE/2;
	Dintdrawrect(dia, &r, 255, 255, 255);
	Ddrawrectframe(dia, &r, 1, 0);
}

void Dinvertentry(TDIALOG *dia, INTBIG sc, INTBIG on)
{
	RECTAREA r;
	REGISTER INTBIG which;
	DSCROLL *scr;

	scr = &dia->scroll[sc];
	if (scr->whichlow < 0) return;
	for(which = scr->whichlow; which <= scr->whichhigh; which++)
	{
		if (which-scr->firstline >= 0 && which-scr->firstline < scr->linesinfolder)
		{
			r.left = scr->userrect.left+1;
			r.right = scr->userrect.right-1;
			r.top = scr->userrect.top + scr->lineheight*(which-scr->firstline)+1;
			r.bottom = r.top + scr->lineheight;
			if (r.bottom >= scr->userrect.bottom) r.bottom = scr->userrect.bottom-1;
			Dinvertrect(dia, &r);
		}
	}
}

/*
 * routine to determine which item falls under the coordinates (x, y)
 */
INTBIG Dwhichitem(TDIALOG *dia, INTBIG x, INTBIG y)
{
	INTBIG i;
	RECTAREA r;

	for(i=0; i<dia->dlgresaddr->items; i++)
	{
		r = dia->dlgresaddr->list[i].r;
		if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) return(i+1);
	}
	return(0);
}

void Dinsertstr(TDIALOG *dia, CHAR *insmsg)
{
	INTBIG i, j, newcurspos;
	CHAR *oldmsg, *newmsg, *pt;

	if (dia->curitem < 0) return;
	Dhighlight(dia, 0);
	oldmsg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;

	/* allocate space for the new message and fill it */
	newmsg = (CHAR *)((estrlen(oldmsg)+estrlen(insmsg)+dia->editend-dia->editstart+1) * SIZEOFCHAR,
		el_tempcluster);
	if (newmsg == 0) return;
	j = 0;
	for(i=0; i<dia->editstart; i++) newmsg[j++] = oldmsg[i];
	for(i=0; insmsg[i] != 0; i++) newmsg[j++] = insmsg[i];
	newcurspos = j;
	for(i=dia->editend; oldmsg[i] != 0; i++) newmsg[j++] = oldmsg[i];
	newmsg[j] = 0;
	dia->editstart = dia->editend = newcurspos;

	/* replace the message */
	efree((CHAR *)dia->dlgresaddr->list[dia->curitem].data);
	dia->dlgresaddr->list[dia->curitem].data = (INTBIG)newmsg;

	/* make sure the cursor is visible */
	dia->firstch = Dneweditbase(dia);
	if (dia->opaqueitem == dia->curitem)
	{
		(void)allocstring(&oldmsg, newmsg, el_tempcluster);
		for(pt = oldmsg; *pt != 0; pt++)
			*pt = '\245';		/* bullet */
		Dstufftext(dia, oldmsg, &dia->dlgresaddr->list[dia->curitem].r);
		efree(oldmsg);
	} else
	{
		Dstufftext(dia, newmsg, &dia->dlgresaddr->list[dia->curitem].r);
	}

	/* set new highlighting */
	Dhighlight(dia, 1);
}

INTBIG Dneweditbase(TDIALOG *dia)
{
	INTBIG wid, prevpos, basepos, y, firstoff, len, newfirst;
	CHAR *newmsg, *msg;
	RECTAREA r;

	newfirst = dia->firstch;
	newmsg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
	r = dia->dlgresaddr->list[dia->curitem].r;
	if (r.bottom-r.top < dia->lineheight*2)
	{
		/* if there is only 1 line in the edit field, shift horizontally */
		if (dia->editend < newfirst)
		{
			newfirst = dia->editend-1;
			if (newfirst < 0) newfirst = 0;
		}
		while (dia->editend > newfirst)
		{
			wid = Dgettextsize(dia, &newmsg[newfirst], dia->editend-newfirst);
			if (wid <= r.right-r.left-2) break;
			newfirst++;
		}
	} else
	{
		/* multiple lines in the edit field, shift vertically */
		while (dia->editend < newfirst)
		{
			msg = newmsg;
			prevpos = 0;
			basepos = 0;
			while (*msg != 0)
			{
				for(len = estrlen(msg); len>0; len--)
				{
					wid = Dgettextsize(dia, msg, len);
					if (wid < r.right-r.left-2) break;
				}
				if (len == 0) break;
				basepos += len;
				if (basepos == newfirst) break;
				prevpos += len;
				msg += len;
			}
			newfirst = prevpos;
		}
		if (dia->editend > newfirst)
		{
			y = r.top + dia->lineheight;
			msg = &newmsg[newfirst];
			firstoff = 0;
			basepos = 0;
			while (*msg != 0)
			{
				for(len = estrlen(msg); len>0; len--)
				{
					wid = Dgettextsize(dia, msg, len);
					if (wid < r.right-r.left-2) break;
				}
				if (len == 0) break;
				if (dia->editend <= newfirst + basepos + len) break;
				if (firstoff == 0) firstoff = len;
				msg += len;
				basepos += len;
				y += dia->lineheight;
				if (y > r.bottom) { newfirst += firstoff;   break; }
			}
		}
	}
	return(newfirst);
}

void Dhighlight(TDIALOG *dia, INTBIG on)
{
	INTBIG i, sx, ex, sline, eline, line, dummy, es, ee;
	CHAR *msg;
	RECTAREA r, itemrect;

	if (dia->curitem < 0) return;
	itemrect = dia->dlgresaddr->list[dia->curitem].r;
	msg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
	es = dia->editstart;   ee = dia->editend;
	if (es > ee) { i = es;   es = ee;   ee = i; }
	Dtextlocation(dia, msg, es, &itemrect, &sx, &sline);
	Dtextlocation(dia, msg, ee, &itemrect, &ex, &eline);
	for(i=sline; i<=eline; i++)
	{
		r.top = itemrect.top + i * dia->lineheight;
		if (r.top >= itemrect.bottom) break;
		r.bottom = r.top + dia->lineheight;
		if (r.bottom > itemrect.bottom) r.bottom = itemrect.bottom;

		r.left = itemrect.left;
		if (i == sline)
		{
			Dtextlocation(dia, msg, es+1, &itemrect, &dummy, &line);
			if (line != sline && sline != eline) continue;
			r.left += sx;
		}
		r.right = itemrect.right;
		if (i == eline)
		{
			if (ex == 0 && sline != eline) continue;
			r.right = itemrect.left + ex + 1;
		}

		Dinvertrect(dia, &r);
	}
}

/*
 * routine to determine where the cursor is in the edit item "r" given character
 * "len" of "msg".  The horizontal offset is placed in "wid" and the line number
 * in "line".
 */
void Dtextlocation(TDIALOG *dia, CHAR *msg, INTBIG position, RECTAREA *r, INTBIG *x, INTBIG *line)
{
	INTBIG basepos, len, wid;
	/* if beyond the end, any hit is the end */
	*line = *x = 0;
	if (dia->firstch >= estrlen(msg)) return;

	/* set initial message and offset in corner of edit box */
	msg = &msg[dia->firstch];
	basepos = dia->firstch;
	while (*msg != 0)
	{
		for(len = estrlen(msg); len>0; len--)
		{
			wid = Dgettextsize(dia, msg, len);
			if (wid < r->right-r->left-2) break;
		}
		if (len <= 0) { *x = 0;   break; }

		if (position - basepos <= len)
		{
			*x = Dgettextsize(dia, msg, position - basepos);
			break;
		}
		msg += len;
		basepos += len;
		(*line)++;
	}
}

INTBIG Dgeteditpos(TDIALOG *dia, RECTAREA *r, INTBIG x, INTBIG y, CHAR *msg)
{
	INTBIG pos, i, basepos, lastpos, len, wid;

	/* if beyond the end, any hit is the end */
	if (dia->firstch > estrlen(msg)) return(estrlen(msg));

	/* set initial message and offset in corner of edit box */
	msg = &msg[dia->firstch];
	basepos = dia->firstch;
	while (*msg != 0)
	{
		for(len = estrlen(msg); len>0; len--)
		{
			wid = Dgettextsize(dia, msg, len);
			if (wid < r->right-r->left-2) break;
		}
		if (len <= 0) break;

		if (y > r->top && y <= r->top+dia->lineheight)
		{
			lastpos = 0;
			for(i=0; i<len; i++)
			{
				pos = Dgettextsize(dia, msg, i+1);
				if (r->left+(lastpos+pos)/2 > x) break;
				lastpos = pos;
			}
			return(basepos+i);
		}

		msg += len;
		basepos += len;
		y -= dia->lineheight;
	}
	return(basepos);
}

void Ddoneedit(TDIALOG *dia)
{
	INTBIG was;

	if (dia->curitem < 0) return;
	Dhighlight(dia, 0);
	was = dia->curitem;
	dia->curitem = -1;
	if (dia->firstch == 0) return;
	dia->firstch = 0;
	Dstufftext(dia, (CHAR *)dia->dlgresaddr->list[was-1].data, &dia->dlgresaddr->list[was].r);
}

void Ddrawitem(TDIALOG *dia, INTBIG type, RECTAREA *r, CHAR *msg, INTBIG dim)
{
	INTBIG j, save, itemtype;
	POPUPDATA *pd;
	RECTAREA myrect;
	INTBIG xv[3], yv[3];

	itemtype = type & ITEMTYPE; 
	if (itemtype == POPUP)
	{
		/* popup menu item */
		pd = (POPUPDATA *)msg;
		if (pd->count <= 0) return;
		Dstuffmessage(dia, pd->namelist[pd->current], r, dim);
		Dinsetrect(r, -1);
		Ddrawrectframe(dia, r, 1, 0);
		Ddrawline(dia, r->left+3, r->bottom, r->right, r->bottom);
		Ddrawline(dia, r->right, r->top+3, r->right, r->bottom);
		Dinsetrect(r, 1);
		xv[0] = r->right - 18;   yv[0] = r->top + 4;
		xv[1] = r->right - 12;   yv[1] = r->top + 10;
		xv[2] = r->right - 6;    yv[2] = r->top + 4;
		save = r->left;   r->left = r->right - 19;
		Dintdrawrect(dia, r, 255, 255, 255);
		r->left = save;
		Ddrawpolygon(dia, xv, yv, 3, 1);
	} else if (itemtype == BUTTON || itemtype == DEFBUTTON)
	{
		/* button */
		j = Dgettextsize(dia, msg, estrlen(msg));
		Ddrawroundrectframe(dia, r, 12, dim);
		Ddrawtext(dia, msg, estrlen(msg), (r->left+r->right-j)/2,
			(r->top+r->bottom+dia->lineheight)/2, dim);
	} else if (itemtype == CHECK)
	{
		/* check box */
		myrect = *r;
		myrect.right = myrect.left + 12;
		myrect.top = (myrect.top + myrect.bottom) / 2 - 6;
		myrect.bottom = myrect.top + 12;
		Ddrawrectframe(dia, &myrect, 1, dim);
		Ddrawtext(dia, msg, estrlen(msg), myrect.right+4, r->top+dia->lineheight, dim);
	} else if (itemtype == RADIO)
	{
		/* radio button */
		myrect = *r;
		myrect.right = myrect.left + 12;
		myrect.top = (myrect.top + myrect.bottom) / 2 - 6;
		myrect.bottom = myrect.top + 12;
		Ddrawcircle(dia, &myrect, dim);
		Ddrawtext(dia, msg, estrlen(msg), myrect.right+4, myrect.top+dia->lineheight-2, dim);
	} else if (itemtype == MESSAGE)
	{
		/* message */
		Dstuffmessage(dia, msg, r, dim);
	} else if (itemtype == EDITTEXT)
	{
		/* edit text */
		if (dim == 0) Dstufftext(dia, msg, r);
		Deditbox(dia, r, 1, dim);
	} else if (itemtype == SCROLL || itemtype == SCROLLMULTI)
	{
		/* scrollable list */
		Ddrawrectframe(dia, r, 1, dim);
	} else if (itemtype == ICON)
	{
		/* 32x32 icon */
		Dputicon(dia, r->left, r->top, (UCHAR1 *)msg);
	} else if (itemtype == DIVIDELINE)
	{
		/* dividing line */
		DiaDrawRect(dia, 0, r, 0, 0, 0);
	}
}

void Ddrawpopupentries(TDIALOG *dia)
{
	INTBIG i, y;
	INTBIG xv[3], yv[3];

	Dintdrawrect(dia, &dia->rect, 255, 255, 255);
	Ddrawrectframe(dia, &dia->rect, 1, 0);
	for(i=dia->sta; i<dia->cnt; i++)
	{
		if (i == dia->sta && dia->sta != 0)
		{
			y = dia->rect.top+(i-dia->sta+1)*dia->lineheight+1;
			xv[0] = dia->rect.left + 18;   yv[0] = y - 4;
			xv[1] = dia->rect.left + 12;   yv[1] = y - 10;
			xv[2] = dia->rect.left + 6;    yv[2] = y - 4;
			Ddrawpolygon(dia, xv, yv, 3, 1);
			continue;
		}
		if (i == dia->cnt-1 && dia->cnt != dia->pd->count)
		{
			y = dia->rect.top+(i-dia->sta+1)*dia->lineheight+1;
			xv[0] = dia->rect.left + 18;   yv[0] = y - 10;
			xv[1] = dia->rect.left + 12;   yv[1] = y - 4;
			xv[2] = dia->rect.left + 6;    yv[2] = y - 10;
			Ddrawpolygon(dia, xv, yv, 3, 1);
			continue;
		}
		Ddrawtext(dia, dia->pd->namelist[i], estrlen(dia->pd->namelist[i]), dia->rect.left+1,
			dia->rect.top+(i-dia->sta+1)*dia->lineheight+1, 0);
	}
}

void Dstufftext(TDIALOG *dia, CHAR *msg, RECTAREA *r)
{
	INTBIG len, wid, vpos;

	Dintdrawrect(dia, r, 255, 255, 255);

	/* display the message in multiple lines */
	vpos = r->top+dia->lineheight;
	if (dia->firstch > estrlen(msg)) return;
	msg = &msg[dia->firstch];
	while (*msg != 0)
	{
		for(len = estrlen(msg); len>0; len--)
		{
			wid = Dgettextsize(dia, msg, len);
			if (wid < r->right-r->left-2) break;
		}
		if (len <= 0) break;
		Ddrawtext(dia, msg, len, r->left+1, vpos, 0);
		msg += len;
		vpos += dia->lineheight;
		if (vpos > r->bottom) break;
	}
}

void Dstuffmessage(TDIALOG *dia, CHAR *msg, RECTAREA *r, INTBIG dim)
{
	INTBIG len, wid, truelen, vpos, mostch, i;
	CHAR *pt;
	REGISTER void *infstr;

	for(pt = msg; *pt != 0; pt++) if (estrncmp(pt, x_("(tm)"), 4) == 0)
	{
		(void)estrcpy(pt, x_("\252"));		/* "tm" character */
		(void)estrcpy(&pt[1], &pt[4]);
		break;
	}
	Dintdrawrect(dia, r, 255, 255, 255);

	/* see how much fits */
	truelen = estrlen(msg);
	for(len = truelen; len > 0; len--)
	{
		wid = Dgettextsize(dia, msg, len);
		if (wid <= r->right-r->left-2) break;
	}
	if (len <= 0) return;

	/* if it all fits or must fit (because there is just 1 line), draw it */
	vpos = r->top+dia->lineheight;
	if (len == truelen || dia->lineheight*2 > r->bottom - r->top)
	{
		if (len != truelen)
		{
			/* single line doesn't fit: put ellipsis at end */
			infstr = initinfstr();
			for(i=0; i<len-3; i++) addtoinfstr(infstr, msg[i]);
			addstringtoinfstr(infstr, x_("..."));
			msg = returninfstr(infstr);
		}
		Ddrawtext(dia, msg, len, r->left+1, vpos, dim);
		return;
	}

	/* write the message in multiple lines */
	while (truelen > 0)
	{
		mostch = 0;
		for(len = truelen; len > 0; len--)
		{
			wid = Dgettextsize(dia, msg, len);
			if (wid > r->right-r->left-2) continue;
			if (mostch == 0) mostch = len;
			if (msg[len] == 0 || msg[len] == ' ') break;
		}
		if (len <= 0) len = mostch;
		Ddrawtext(dia, msg, len, r->left+1, vpos, dim);
		if (msg[len] == ' ') len++;
		msg += len;
		truelen -= len;
		vpos += dia->lineheight;
		if (vpos > r->bottom) break;
	}
}

void Deditbox(TDIALOG *dia, RECTAREA *r, INTBIG draw, INTBIG dim)
{
	Dinsetrect(r, -3);
	Ddrawrectframe(dia, r, draw, dim);
	Dinsetrect(r, 3);
}

/*
 * routine to draw line "msg" in position "which" of scroll area "sc"
 */
void Ddrawmsg(TDIALOG *dia, INTBIG sc, CHAR *msg, INTBIG which)
{
	INTBIG wid, len, firstch, offset;
	INTBIG skip;
	DSCROLL *scr;

	Dsettextsmall(dia, sc);
	scr = &dia->scroll[sc];
	firstch = offset = 0;
	if (scr->horizfactor != 0)
	{
		skip = scr->userrect.right - scr->userrect.left - 6;
		skip *= (INTBIG)scr->horizfactor;   skip /= 10;
		wid = Dgettextsize(dia, msg, estrlen(msg));
		if (skip >= wid) return;
		for(firstch = 0; firstch < estrlen(msg); firstch++)
		{
			wid = Dgettextsize(dia, msg, firstch);
			if (wid >= skip) break;
		}
		offset = wid - skip;
	}

	for(len = estrlen(&msg[firstch]); len>0; len--)
	{
		wid = Dgettextsize(dia, &msg[firstch], len);
		if (wid+offset < scr->userrect.right-scr->userrect.left-6) break;
	}
	if (len <= 0) return;
	Ddrawtext(dia, &msg[firstch], len, scr->userrect.left+2+offset,
		scr->userrect.top + scr->lineheight*(which+1), 0);
	Dsettextsmall(dia, -1);
}

void Dinsetrect(RECTAREA *r, INTBIG amt)
{
	r->left += amt;   r->right -= amt;
	r->top += amt;    r->bottom -= amt;
}

void Ddrawvertslider(TDIALOG *dia, INTBIG sc)
{
	RECTAREA r;
	DSCROLL *scr;

	scr = &dia->scroll[sc];
	r = scr->userrect;
	r.left = r.right-1;   r.right += 14;
	Ddrawrectframe(dia, &r, 1, 0);
	Ddrawline(dia, r.left, r.top+15, r.right-1, r.top+15);
	Ddrawarrow(dia, sc, UPARROW, 0);
	Ddrawline(dia, r.left, r.bottom-16, r.right-1, r.bottom-16);
	Ddrawarrow(dia, sc, DOWNARROW, 0);
}

void Ddrawhorizslider(TDIALOG *dia, INTBIG sc)
{
	RECTAREA r;
	DSCROLL *scr;

	scr = &dia->scroll[sc];
	r = scr->userrect;
	r.top = r.bottom-1;   r.bottom += 14;
	Ddrawrectframe(dia, &r, 1, 0);
	Ddrawline(dia, r.left+15, r.top, r.left+15, r.bottom-1);
	Ddrawarrow(dia, sc, LEFTARROW, 0);
	Ddrawline(dia, r.right-16, r.top, r.right-16, r.bottom-1);
	Ddrawarrow(dia, sc, RIGHTARROW, 0);
	Dsethscroll(dia, sc);
}

#define ARROWLEN     7
void Ddrawarrow(TDIALOG *dia, INTBIG sc, INTBIG which, INTBIG filled)
{
	INTBIG i, left, top, bottom, right;
	INTBIG xv[ARROWLEN], yv[ARROWLEN];
	DSCROLL *scr;
	static INTBIG arrowx[] = {4, 10, 10, 13, 7, 1, 4};
	static INTBIG arrowy[] = {12, 12, 8, 8, 2, 8, 8};

	scr = &dia->scroll[sc];
	top = scr->userrect.top;
	bottom = scr->userrect.bottom-1;
	right = scr->userrect.right-1;
	left = scr->userrect.left;
	for(i=0; i<ARROWLEN; i++)
	{
		switch (which)
		{
			case UPARROW:
				xv[i] = right+arrowx[i];
				yv[i] = top+arrowy[i];
				break;
			case DOWNARROW:
				xv[i] = right+arrowx[i];
				yv[i] = bottom-arrowy[i];
				break;
			case LEFTARROW:
				xv[i] = left+arrowy[i];
				yv[i] = bottom+arrowx[i];
				break;
			case RIGHTARROW:
				xv[i] = right-arrowy[i];
				yv[i] = bottom+arrowx[i];
				break;
		}
	}
	Ddrawpolygon(dia, xv, yv, ARROWLEN, filled);
}

void DTextCopy(TDIALOG *dia)
{
	CHAR *msg;
	INTBIG i;
	REGISTER void *infstr;

	if (dia->curitem < 0) return;
	
	/* copy selected text to the scrap */
	msg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
	infstr = initinfstr();
	for(i=dia->editstart; i < dia->editend; i++)
		addtoinfstr(infstr, msg[i]);
	setcutbuffer(returninfstr(infstr));
}
void DTextCut(TDIALOG *dia)
{
	CHAR *msg;
	INTBIG i, len, newcaret;
	REGISTER void *infstr;

	if (dia->curitem < 0) return;

	/* copy selected text to the scrap */
	msg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
	len = estrlen(msg);
	infstr = initinfstr();
	for(i=dia->editstart; i < dia->editend; i++)
		addtoinfstr(infstr, msg[i]);
	setcutbuffer(returninfstr(infstr));
	
	/* remove selected text */
	newcaret = dia->editstart;
	infstr = initinfstr();
	for(i=0; i<dia->editstart; i++)
		addtoinfstr(infstr, msg[i]);
	for(i=dia->editend; i<len; i++)
		addtoinfstr(infstr, msg[i]);
	DiaSetText(dia, dia->curitem+1, returninfstr(infstr));
	Dhighlight(dia, 0);
	dia->editstart = dia->editend = newcaret;
	Dhighlight(dia, 1);
}

CHAR DTextPaste(TDIALOG *dia)
{
	CHAR *msg, *oldmsg;
	INTBIG i, len, newcaret;
	REGISTER void *infstr;

	if (dia->curitem < 0) return(0);
	oldmsg = (CHAR *)dia->dlgresaddr->list[dia->curitem].data;
	len = estrlen(oldmsg);
	msg = getcutbuffer();
	newcaret = dia->editstart + estrlen(msg);
	infstr = initinfstr();
	for(i=0; i<dia->editstart; i++)
		addtoinfstr(infstr, oldmsg[i]);
	addstringtoinfstr(infstr, msg);
	for(i=dia->editend; i<len; i++)
		addtoinfstr(infstr, oldmsg[i]);
	DiaSetText(dia, dia->curitem+1, returninfstr(infstr));
	Dhighlight(dia, 0);
	dia->editstart = dia->editend = newcaret;
	Dhighlight(dia, 1);
	return(0);
}

void Dredrawdialogwindow(TDIALOG *dia)
{
	INTBIG i, itemtype, dim, pureitemtype;
	CHAR *line;
	RECTAREA r;
	USERTYPE routine;

	Dclear(dia);

	/* do not handle the first macintosh update event */
	if (dia->firstupdate != 0)
	{
		dia->firstupdate = 0;
		return;
	}

	/* redraw the window frame */
	Dsettextsmall(dia, -1);

	for(i=0; i<dia->dlgresaddr->items; i++)
	{
		/* draw the item */
		itemtype = dia->dlgresaddr->list[i].type;
		pureitemtype = itemtype & ITEMTYPE;
		r = dia->dlgresaddr->list[i].r;
		if (pureitemtype == USERDRAWN)
		{
			routine = (USERTYPE)dia->dlgresaddr->list[i].data;
			if (routine != 0) (*routine)(&r, dia);
			continue;
		}
		if (pureitemtype == MESSAGE || pureitemtype == EDITTEXT || pureitemtype == POPUP ||
			pureitemtype == BUTTON || pureitemtype == DEFBUTTON || pureitemtype == ICON)
		{
			line = (CHAR *)dia->dlgresaddr->list[i].data;
		} else if (pureitemtype == CHECK || pureitemtype == RADIO)
		{
			line = &((CHAR *)dia->dlgresaddr->list[i].data)[1];
		} else line = dia->dlgresaddr->list[i].msg;
		if ((itemtype&INACTIVE) != 0) dim = 1; else dim = 0;
		Ddrawitem(dia, itemtype, &r, line, dim);
		
		/* handle set controls */
		if (pureitemtype == CHECK && ((CHAR *)dia->dlgresaddr->list[i].data)[0] != 0)
		{
			r.right = r.left + 12;
			r.top = (r.top + r.bottom) / 2 - 6;
			r.bottom = r.top + 12;
			Ddrawline(dia, r.left, r.top, r.right-1, r.bottom-1);
			Ddrawline(dia, r.left, r.bottom-1, r.right-1, r.top);
		}
		if (pureitemtype == RADIO && ((CHAR *)dia->dlgresaddr->list[i].data)[0] != 0)
		{
			r.right = r.left + 12;
			r.top = (r.top + r.bottom) / 2 - 6;
			r.bottom = r.top + 12;
			Dinsetrect(&r, 3);
			Ddrawdisc(dia, &r);
		}

		/* highlight the default button */
		if (i == dia->defaultbutton-1 &&
			(itemtype == BUTTON || itemtype == DEFBUTTON)) Dhighlightrect(dia, &r);
	}

	/* turn on anything being edited */
	if (dia->curitem >= 0) Dhighlight(dia, 1);
	for(i=0; i<dia->scrollcount; i++)
	{
		Dredrawscroll(dia, i);
		Dinvertentry(dia, i, 1);      /* restores previous inverted entry */
		Dsetvscroll(dia, i);
		if ((dia->scroll[i].flags&SCHORIZBAR) != 0)
		{
			Ddrawhorizslider(dia, i);
			Dsethscroll(dia, i);
		}
	}
}

/****************************** DIALOG PRIMITIVE GRAPHICS ******************************/

/*
 * routine to prepare a dialog in location "r", draggable if "movable" is nonzero
 */
void Dnewdialogwindow(TDIALOG *dia, RECTAREA *r, CHAR *movable, BOOLEAN modeless)
{
	FontInfo fi;
	WindowPtr frontmost, wBehind;
	INTBIG type;
	Str255 line;

	setdefaultcursortype(NORMALCURSOR);

	if (movable != 0)
	{
		estrcpy((CHAR *)&line[1], movable);
		line[0] = estrlen((CHAR *)&line[1]);
		type = noGrowDocProc;
	} else
	{
		line[0] = 0;
		type = dBoxProc;
	}

	gra_frontnonfloatingwindow(&frontmost);
	if (frontmost != 0) gra_activatewindow(frontmost, FALSE);

	if (modeless)
	{
		wBehind = gra_lastfloatingwindow();
		if (wBehind == 0) wBehind = (WindowPtr)-1;
	} else wBehind = (WindowPtr)-1;

	dia->theDialog = (GrafPtr)NewCWindow(0L, (Rect *)r, line, 0, type, wBehind, 0, 0L);
	if (dia->theDialog == 0) return;

	ShowHide(dia->theDialog, 1);
	if (modeless)
	{
		gra_frontnonfloatingwindow(&frontmost);
		gra_activatewindow((WindowRef)dia->theDialog, FALSE);
		gra_activatewindow(frontmost, TRUE);
	} else
	{
		gra_activatewindow(dia->theDialog, TRUE);
	}

	SetPort(dia->theDialog);

	/* setup for small scroll text */
	TextFont(DSFONT);
	TextSize(10);
	GetFontInfo(&fi);
	dia->slineheight = fi.ascent + fi.descent + fi.leading;
	dia->slineoffset = fi.descent;

	/* setup for all other text */
	TextFont(DFONT);
	TextSize(12);
	GetFontInfo(&fi);
	dia->lineheight = fi.ascent + fi.descent + fi.leading;
	dia->lineoffset = fi.descent;
	dia->curlineoffset = dia->lineoffset;
}

/*
 * routine to terminate the current dialog
 */
void Ddonedialogwindow(TDIALOG *dia)
{
	Rect r;
	INTBIG wid, hei;

	if (dia->dlgresaddr->movable != 0)
	{
		r = (*((WindowPeek)dia->theDialog)->strucRgn)->rgnBBox;
#if 1
		wid = dia->dlgresaddr->windowRect.right - dia->dlgresaddr->windowRect.left;
		hei = dia->dlgresaddr->windowRect.bottom - dia->dlgresaddr->windowRect.top;
		dia->dlgresaddr->windowRect.left = r.left+gra_winoffleft;
		dia->dlgresaddr->windowRect.right = dia->dlgresaddr->windowRect.left+wid;
		dia->dlgresaddr->windowRect.top = r.top+gra_winofftop;
		dia->dlgresaddr->windowRect.bottom = dia->dlgresaddr->windowRect.top+hei;
#else
		dia->dlgresaddr->windowRect.left = r.left+gra_winoffleft;
		dia->dlgresaddr->windowRect.right = r.right+gra_winoffright;
		dia->dlgresaddr->windowRect.top = r.top+gra_winofftop;
		dia->dlgresaddr->windowRect.bottom = r.bottom+gra_winoffbottom;
#endif
	}
	gra_disposeoswindow(dia->theDialog);
}

/*
 * routine to erase the dialog window
 */
void Dclear(TDIALOG *dia)
{
	gra_selectoswindow(dia->theDialog);
	SetPort(dia->theDialog);
}

/*
 * routine to force the dialog window to be current (on window systems
 * where this is relevant)
 */
void Dforcedialog(TDIALOG *dia)
{
	gra_selectoswindow(dia->theDialog);
	SetPort(dia->theDialog);
}

/*
 * routine to wait for the next action from the dialog window.  Returns:
 *  -1  Nothing happened
 *   0  Key typed, character in "chr"/"special"
 *   1  Mouse button pushed, coordinates in (x,y), modifiers in "chr"
 *   2  Mouse button released, coordinates in (x,y)
 *   4  Mouse motion while button down, coordinates in (x,y)
 *   5  Double-click of mouse button, coordinates in (x,y), modifiers in "chr"
 */
INTBIG Dwaitforaction(TDIALOG *dia, INTBIG *x, INTBIG *y, INTBIG *chr, INTBIG *special, UINTBIG *time, BOOLEAN *shifted)
{
	INTBIG sx, sy, but;

	gra_handlingdialog = dia;
	if (ttydataready())
	{
		*chr = getnxtchar(special);
		*time = ticktime();
		*shifted = FALSE;
		gra_handlingdialog = NOTDIALOG;
		return(0);
	}

	/* get button */
	waitforbutton(&sx, &sy, &but);
	gra_handlingdialog = NOTDIALOG;
	*x = sx;   *y = sy;
	if (but < 0) return(-1);
	*time = ticktime();
	*shifted = shiftbutton(but);
	if (doublebutton(but)) return(5);
	return(1);
}

/*
 * routine to track mouse movements while the button remains down, calling
 * the routine "eachdown" for each advance.  The routine is given the
 * mouse coordinates as parameters.  The initial mouse position is (fx,fy).
 */
void Dtrackcursor(TDIALOG *dia, INTBIG fx, INTBIG fy, BOOLEAN (*eachdown)(INTBIG, INTBIG))
{
	gra_trackingdialog = dia;
	trackcursor(FALSE, us_nullup, us_nullvoid, eachdown, us_nullchar, us_nullvoid, TRACKNORMAL);
}

/*
 * routine to move the rectangle "sr" to "dr"
 */
void Dshiftbits(TDIALOG *dia, RECTAREA *sr, RECTAREA *dr)
{
	SetPort(dia->theDialog);
	CopyBits(&dia->theDialog->portBits, &dia->theDialog->portBits,
		(Rect *)sr, (Rect *)dr, srcCopy, 0);
}

/*
 * routine to handle popup menu "pd" in rectangle "r".  Returns negative if
 * the function is not possible.  Returns the entry in "pd" that is selected
 * (returns pd->current if no change is made).
 */
INTBIG Dhandlepopup(TDIALOG *dia, RECTAREA *r, POPUPDATA *pd)
{
	INTBIG ret, i, j;
	CHAR *itemname;
	Str255 name;
	MenuHandle menu;
	Point p;

	SetPort(dia->theDialog);
	p.h = r->left;   p.v = r->top;
	LocalToGlobal(&p);
	menu = NewMenu(2048, x_("\p"));
	for(i=0; i<pd->count; i++)
	{
		itemname = pd->namelist[i];
		for(j=0; itemname[j] != 0; j++)
		{
			if (itemname[j] == '(') name[j+1] = '{'; else
				if (itemname[j] == ')') name[j+1] = '}'; else
					name[j+1] = itemname[j];
		}
		name[0] = j;
		AppendMenu(menu, name);
	}
	InsertMenu(menu, -1);
	ret = PopUpMenuSelect(menu, p.v, p.h, pd->current+1);
	DeleteMenu(2048);
	DisposeMenu(menu);
	if (HiWord(ret) == 0) return(pd->current);
	return(LoWord(ret) - 1);
}

/*
 * routine to draw the 32x32 bitmap in "data" at (x,y)
 */
void Dputicon(TDIALOG *dia, INTBIG x, INTBIG y, UCHAR1 *data)
{
	BitMap b;

	b.baseAddr = (CHAR1 *)data;
	b.rowBytes = 4;
	b.bounds.left = x;   b.bounds.top = y;
	b.bounds.right = x+32;   b.bounds.bottom = y+32;
	CopyBits(&b, &dia->theDialog->portBits, &b.bounds, &b.bounds, srcCopy, 0L);
}

/*
 * routine to draw a line from (xf,yf) to (xt,yt) in the dialog
 */
void Ddrawline(TDIALOG *dia, INTBIG xf, INTBIG yf, INTBIG xt, INTBIG yt)
{
	SetPort(dia->theDialog);
	MoveTo(xf, yf);
	LineTo(xt, yt);
}

/*
 * routine to draw a filled rectangle at "rect" in the dialog with color (r,g,b).
 */
void Dintdrawrect(TDIALOG *dia, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b)
{
	RGBColor color, oldcolor;

	SetPort(dia->theDialog);
	color.red = r << 8;
	color.green = g << 8;
	color.blue = b << 8;
	GetForeColor(&oldcolor);
	RGBForeColor(&color);
	FillRect((Rect *)rect, &qd.black);
	RGBForeColor(&oldcolor);
}

/*
 * routine to draw a gray rectangle at "r" in the dialog
 */
void Dgrayrect(TDIALOG *dia, RECTAREA *r)
{
	SetPort(dia->theDialog);
	FillRect((Rect *)r, &qd.ltGray);
}

/*
 * routines to invert a rectangle at "r" in the dialog
 */
void Dinvertrect(TDIALOG *dia, RECTAREA *r)
{
	SetPort(dia->theDialog);
	InvertRect((Rect *)r);
}

/*
 * routine to invert a rounded rectangle at "r" in the dialog
 */
void Dinvertroundrect(TDIALOG *dia, RECTAREA *r)
{
	SetPort(dia->theDialog);
	InvertRoundRect((Rect *)r, 10, 10);
}

/*
 * routine to draw the outline of rectangle "r" in the dialog.  Erases if "on" is zero
 */
void Ddrawrectframe(TDIALOG *dia, RECTAREA *r, INTBIG on, INTBIG dim)
{
	SetPort(dia->theDialog);
	if (on == 0) PenMode(patBic);
	FrameRect((Rect *)r);
	PenMode(patCopy);
	if (dim != 0)
	{
		PenMode(patBic);
		PenPat(&qd.gray);
		PaintRect((Rect *)r);
		PenMode(patCopy);
		PenPat(&qd.black);
	}
}
/*
 * routine to highlight the outline of rectangle "r" in the dialog
 */
void Dhighlightrect(TDIALOG *dia, RECTAREA *r)
{
	Dinsetrect(r, -4);
	SetPort(dia->theDialog);
	PenSize(3, 3);
	FrameRoundRect((Rect *)r, 16, 16);
	PenSize(1, 1);
}

/*
 * routine to draw the outline of rounded rectangle "r" in the dialog
 */
void Ddrawroundrectframe(TDIALOG *dia, RECTAREA *r, INTBIG arc, INTBIG dim)
{
	SetPort(dia->theDialog);
	FrameRoundRect((Rect *)r, arc, arc);
	if (dim != 0)
	{
		PenMode(patBic);
		PenPat(&qd.gray);
		PaintRect((Rect *)r);
		PenMode(patCopy);
		PenPat(&qd.black);
	}
}

/*
 * routine to invert the outline of rectangle "r" in the dialog
 */
void Dinvertrectframe(TDIALOG *dia, RECTAREA *r)
{
	SetPort(dia->theDialog);
	PenMode(patXor);
	FrameRect((Rect *)r);
	PenMode(patCopy);
}

/*
 * routine to draw the polygon in "count" points (xv,yv) and fill it if "filled" is nonzero
 */
void Ddrawpolygon(TDIALOG *dia, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG filled)
{
	PolyHandle ph;
	INTBIG i, l;

	SetPort(dia->theDialog);
	ph = OpenPoly();
	for(i=0; i<count; i++)
	{
		if (i == 0) l = count-1; else l = i-1;
		MoveTo(xv[l], yv[l]);
		LineTo(xv[i], yv[i]);
	}
	ClosePoly();
	if (filled != 0) FillPoly(ph, &qd.black); else
	{
		FillPoly(ph, &qd.white);
		FramePoly(ph);
	}
	KillPoly(ph);
}

/*
 * routine to draw a circle outline in rectangle "r" in the dialog
 */
void Ddrawcircle(TDIALOG *dia, RECTAREA *r, INTBIG dim)
{
	SetPort(dia->theDialog);
	FrameOval((Rect *)r);
	if (dim != 0)
	{
		PenMode(patBic);
		PenPat(&qd.gray);
		PaintRect((Rect *)r);
		PenMode(patCopy);
		PenPat(&qd.black);
	}
}

/*
 * routine to draw a filled circle in rectangle "r" in the dialog
 */
void Ddrawdisc(TDIALOG *dia, RECTAREA *r)
{
	SetPort(dia->theDialog);
	FillOval((Rect *)r, &qd.black);
}

/*
 * routine to determine the width of the "len" characters at "msg"
 */
INTBIG Dgettextsize(TDIALOG *dia, CHAR *msg, INTBIG len)
{
	INTBIG wid;

	if (len == 0) return(0);
	SetPort(dia->theDialog);
	wid = TextWidth(msg, 0, len);
	return(wid);
}

/*
 * Routine to set the current text size to the large or small scroll text.
 */
void Dsettextsmall(TDIALOG *dia, INTBIG sc)
{
	INTBIG smallf;
	DSCROLL *scr;

	if (sc < 0) smallf = 0; else
	{
		scr = &dia->scroll[sc];
		smallf = scr->flags & SCSMALLFONT;
	}
	if (smallf != 0)
	{
		dia->curlineoffset = scr->lineoffset;
		TextFont(DSFONT);
		TextSize(10);
	} else
	{
		dia->curlineoffset = dia->lineoffset;
		TextFont(DFONT);
		TextSize(12);
	}
}

/*
 * routine to draw "len" characters of "msg" at (x,y)
 */
void Ddrawtext(TDIALOG *dia, CHAR *msg, INTBIG len, INTBIG x, INTBIG y, INTBIG dim)
{
	Rect r;

	SetPort(dia->theDialog);
	MoveTo(x, y-dia->curlineoffset-1);
	DrawText(msg, 0, len);
	if (dim != 0)
	{
		r.left = x;     r.right = x + TextWidth(msg, 0, len);
		r.bottom = y;   r.top = y - dia->lineheight;
		PenPat(&qd.gray);
		PenMode(patBic);
		PaintRect(&r);
		PenMode(patCopy);
		PenPat(&qd.black);
	}
}

CHAR *gra_makepstring(CHAR *string)
{
	static CHAR pstring[256];
	REGISTER INTBIG len;

	estrncpy(&pstring[1], string, 255);
	len = estrlen(string);
	if (len > 255) len = 255;
	pstring[0] = len;
	return(pstring);
}

/*
 * Helper routine for "DiaLoadTextDialog" that makes strings go in ascending order.
 */
int gra_stringposascending(const void *e1, const void *e2)
{
	REGISTER CHAR *c1, *c2;

	c1 = *((CHAR **)e1);
	c2 = *((CHAR **)e2);
	return(namesame(&c1[gra_dialogstringpos], &c2[gra_dialogstringpos]));
}
