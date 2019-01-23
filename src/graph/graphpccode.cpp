/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: graphpccode.cpp
 * Interface for Win32 (Win9x/NT/2K/XP) computers
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

#include "graphpcstdafx.h"
#include "graphpc.h"
#include "graphpcdoc.h"
#include "graphpcview.h"
#include "graphpcmainframe.h"
#include "graphpcchildframe.h"
#include "graphpcmsgview.h"
#include "graphpcdialog.h"
#include "graphpcdialoglistbox.h"

#include "global.h"
#include "database.h"
#include "egraphics.h"
#include "usr.h"
#include "usrtrack.h"
#include "eio.h"
#include "edialogs.h"
#if LANGTCL
#  include "dblang.h"
#endif

/* #define USEDIRECTSOUND 1 */		/* uncomment to use direct-sound for playing sounds */

#include <io.h>
#include <signal.h>
#include <sys/stat.h>
#include <direct.h>
#include <sys/timeb.h>
#include <sys/utime.h>
#include <afxmt.h>
#include <MMSystem.h>	/* need link with "winmm.lib" */

#ifdef _UNICODE
#  define estat     _wstat
#  define estatstr  _stat
#  define eaccess   _waccess
#  define emkdir    _wmkdir
#  define egetcwd   _wgetcwd
#  define ecreat    _wcreat
#  define efullpath _wfullpath
#else
#  define estat     stat
#  define estatstr  stat
#  define eaccess   access
#  define emkdir    mkdir
#  define egetcwd   getcwd
#  define ecreat    creat
#  define efullpath _fullpath
#endif

/****** windows and control ******/
extern CElectricApp  gra_app;					/* this application */
static WINDOWFRAME  *gra_cureditwindowframe;	/* the current window frame */
static WINDOWFRAME  *gra_palettewindowframe = NOWINDOWFRAME;	/* window frame with palette */
static INTBIG        gra_windownumberindex = 0;	/* counter for window and dialog indices */
static INTBIG        gra_newwindowoffset = 0;	/* offset of the next window (stagger them) */
static CHAR         *gra_initialdirectory;		/* location where Electric started */
static BOOLEAN       gra_creatingwindow = FALSE;/* TRUE if creating a window */
static BOOLEAN       gra_noflush = FALSE;		/* TRUE to supress display flushing */

/****** the messages window ******/
#define MAXTYPEDLINE        256					/* max chars on input line */

	   CChildFrame  *gra_messageswindow;		/* the messages window */
static CRichEditCtrl *gra_editCtrl;				/* the rich-edit control for messages text */
static BOOLEAN       gra_messagescurrent;		/* true if messages window is current */
static INTBIG        gra_messagesleft;			/* left bound of messages window screen */
static INTBIG        gra_messagesright;			/* right bound of messages window screen */
static INTBIG        gra_messagestop;			/* top bound of messages window screen */
static INTBIG        gra_messagesbottom;		/* bottom bound of messages window screen */

/****** for text drawing ******/
static CDC          *gra_texthdc;				/* device context for text buffer */
static HBITMAP       gra_textbitmap;			/* bitmap for text buffer */
static BITMAPINFO   *gra_textbitmapinfo;		/* bitmap information structure */
static UCHAR1       *gra_textdatabuffer;		/* data in text buffer */
static UCHAR1      **gra_textrowstart;			/* row starts for text buffer */
static INTBIG        gra_textbufwid;			/* width of text buffer */
static INTBIG        gra_textbufhei;			/* height of text buffer */
static BOOLEAN       gra_textbufinited = FALSE;	/* true if text buffer is initialized */

/****** for offscreen copying at a larger scale ******/
static BOOLEAN       gra_biggeroffbufinited = FALSE;	/* true if bigger offscreen buffer is initialized */
static INTBIG        gra_biggeroffbufwid;		/* width of bigger offscreen buffer */
static INTBIG        gra_biggeroffbufhei;		/* height of bigger offscreen buffer */
static UCHAR1      **gra_biggeroffrowstart;		/* row starts for bigger offscreen buffer */
static BITMAPINFO   *gra_biggeroffbitmapinfo;	/* bitmap information structure */
static HBITMAP       gra_biggeroffbitmap;		/* bitmap for bigger offscreen buffer */
static UCHAR1       *gra_biggeroffdatabuffer;	/* data in bigger offscreen buffer */
static CDC          *gra_biggeroffhdc;			/* device context for bigger offscreen buffer */

/****** the status bar ******/
#define MAXSTATUSLINES        1

static STATUSFIELD  *gra_statusfields[100];
static CHAR         *gra_statusfieldtext[100];
static INTBIG        gra_indicatorcount = 0;
static UINT gra_indicators[] = {ID_INDICATOR_ELECTRIC01, ID_INDICATOR_ELECTRIC02,
	ID_INDICATOR_ELECTRIC03, ID_INDICATOR_ELECTRIC04, ID_INDICATOR_ELECTRIC05,
	ID_INDICATOR_ELECTRIC06, ID_INDICATOR_ELECTRIC07, ID_INDICATOR_ELECTRIC08,
	ID_INDICATOR_ELECTRIC09, ID_INDICATOR_ELECTRIC10, ID_INDICATOR_ELECTRIC11,
	ID_INDICATOR_ELECTRIC12, ID_INDICATOR_ELECTRIC13, ID_INDICATOR_ELECTRIC14,
	ID_INDICATOR_ELECTRIC15, ID_INDICATOR_ELECTRIC16, ID_INDICATOR_ELECTRIC17};

/****** the dialogs ******/
#define PROGRESSOFFSET           100
#define MAXSCROLLMULTISELECT    1000
#define MAXLOCKS                   3			/* maximum locked pairs of scroll lists */
#ifndef SPI_GETSNAPTODEFBUTTON
#  define SPI_GETSNAPTODEFBUTTON  95			/* why isn't this defined in system headers? */
#endif

#define NOTDIALOG ((TDIALOG *)-1)

typedef struct Itdialog
{
	CElectricDialog *window;
	DIALOG          *itemdesc;
	INTBIG           windindex;
	INTBIG           defaultbutton;
	POINT            firstpoint;
	INTBIG           numlocks, lock1[MAXLOCKS], lock2[MAXLOCKS], lock3[MAXLOCKS];
	INTBIG           redrawitem;
	INTBIG           useritemdoubleclick;
	INTBIG           usertextsize;
	void           (*redrawroutine)(RECTAREA*, void*);
	INTBIG           dialoghit;
	struct Itdialog *nexttdialog;
	void           (*diaeachdown)(INTBIG x, INTBIG y);
	void           (*modelessitemhit)(void *dia, INTBIG item);
} TDIALOG;

static TDIALOG      *gra_firstactivedialog = NOTDIALOG;
static CBrush       *gra_dialogoffbrush = 0;
static CBrush       *gra_dialogbkgrbrush = 0;
static TDIALOG      *gra_trackingdialog = 0;

/****** events ******/
#define CHARREAD             0377				/* character that was read */
#define ISKEYSTROKE          0400				/* set if key typed */
#define ISBUTTON            01000				/* set if button pushed (or released) */
#define BUTTONUP            02000				/* set if button was released */
#define SHIFTISDOWN         04000				/* set if shift key was held down */
#define ALTISDOWN          010000				/* set if alt key was held down */
#define CONTROLISDOWN      020000				/* set if control key was held down */
#define DOUBLECLICK        040000				/* set if this is second click */
#define MOTION            0100000				/* set if mouse motion detected */
#define WINDOWSIZE        0200000				/* set if window grown */
#define WINDOWMOVE        0400000				/* set if window moved */
#define MENUEVENT        01000000				/* set if menu entry selected (values in cursor) */
#define FILEREPLY        02000000				/* set if file selected in standard file dialog */
#define DIAITEMCLICK     04000000				/* set if item clicked in dialog */
#define DIASCROLLSEL    010000000				/* set if scroll item selected in dialog */
#define DIAEDITTEXT     020000000				/* set if edit text changed in dialog */
#define DIAPOPUPSEL     040000000				/* set if popup item selected in dialog */
#define DIASETCONTROL  0100000000				/* set if control changed in dialog */
#define DIAUSERMOUSE   0200000000				/* set if mouse moved in user area of dialog */
#define DIAENDDIALOG   0400000000				/* set when dialog terminates */
#define WHICHBUTTON   07000000000				/* which button was pushed */
#define ISLEFT                  0				/*   the left button */
#define ISMIDDLE      01000000000				/*   the middle button */
#define ISRIGHT       02000000000				/*   the right button */
#define ISWHLFWD      03000000000				/*   forward on the mouse wheel */
#define ISWHLBKW      04000000000				/*   backward on the mouse wheel */
#define POPUPSELECT  010000000000				/* popup selection made */
#define NOEVENT                -1				/* set if nothing happened */

#define FLUSHTICKS    120

struct		/* only used to communicate mouse coordinates for dialog logging */
{
	INTBIG x;
	INTBIG y;
} gra_action;

#define EVENTQUEUESIZE	100

typedef struct
{
	INTBIG           cursorx, cursory;			/* current position of mouse */
	INTBIG           inputstate;				/* current state of device input */
	INTBIG           special;					/* current "special" code for keyboard */
	UINTBIG          eventtime;					/* time this event happened */
} MYEVENTQUEUE;

static MYEVENTQUEUE  gra_eventqueue[EVENTQUEUESIZE];
static MYEVENTQUEUE *gra_eventqueuehead;		/* points to next event in queue */
static MYEVENTQUEUE *gra_eventqueuetail;		/* points to first free event in queue */
static UINTBIG       gra_eventtime;				/* time last event happened */
static void         *gra_eventqueuemutex = 0;	/* mutex for event queue */
static INTBIG        gra_inputstate;			/* current state of device input */
static INTBIG        gra_inputspecial;			/* current "special" keyboard value */
static INTBIG        gra_cursorx, gra_cursory;	/* current position of mouse */
static INTBIG        gra_logrecordcount = 0;	/* count for session flushing */
static INTBIG        gra_lastloggedaction = NOEVENT;
static INTBIG        gra_lastloggedx, gra_lastloggedy;
static INTBIG        gra_lastloggedindex;
static UINTBIG       gra_logbasetime;			/* base of time for log output */
static UINTBIG       gra_lastplaybacktime = 0;
static CHAR         *gra_logfile, *gra_logfilesave;


/****** pulldown menus ******/
#define IDR_MENU           3000					/* base ID for menus */

static CMenu        *gra_hMenu;					/* System Menu */
	   CMenu       **gra_pulldownmenus;			/* list of Windows pulldown menus */
static CHAR        **gra_pulldowns;				/* list of Electric pulldown menu names */
	   INTBIG        gra_pulldownmenucount;		/* number of pulldown menus */
static int           gra_menures[] = {ID_ELECTRIC_MENU01, ID_ELECTRIC_MENU02,
	ID_ELECTRIC_MENU03, ID_ELECTRIC_MENU04, ID_ELECTRIC_MENU05, ID_ELECTRIC_MENU06,
	ID_ELECTRIC_MENU07, ID_ELECTRIC_MENU08, ID_ELECTRIC_MENU09, ID_ELECTRIC_MENU10,
	ID_ELECTRIC_MENU11, ID_ELECTRIC_MENU12, ID_ELECTRIC_MENU13, ID_ELECTRIC_MENU14,
	ID_ELECTRIC_MENU15, ID_ELECTRIC_MENU16, ID_ELECTRIC_MENU17, ID_ELECTRIC_MENU18,
	ID_ELECTRIC_MENU19, ID_ELECTRIC_MENU20, ID_ELECTRIC_MENU21, ID_ELECTRIC_MENU22,
	ID_ELECTRIC_MENU23, ID_ELECTRIC_MENU24, ID_ELECTRIC_MENU25, ID_ELECTRIC_MENU26,
	ID_ELECTRIC_MENU27, ID_ELECTRIC_MENU28, ID_ELECTRIC_MENU29};

/****** mouse buttons ******/
#define BUTTONS              45					/* cannot exceed NUMBUTS in "usr.h" */
#define REALBUTS              5					/* actual number of buttons */

struct
{
	CHAR  *name;			/* button name */
	INTBIG unique;			/* number of letters that make it unique */
} gra_buttonname[BUTTONS] =
{
	{x_("LEFT"), 1},  {x_("MIDDLE"),  1}, {x_("RIGHT"),  1}, {x_("FORWARD"),   1},{x_("BACKWARD"),   1},	/* 0: unshifted */
	{x_("SLEFT"),2},  {x_("SMIDDLE"), 2}, {x_("SRIGHT"), 2}, {x_("SFORWARD"),  2},{x_("SBACKWARD"),  2},	/* 5: shift held down */
	{x_("CLEFT"),2},  {x_("CMIDDLE"), 2}, {x_("CRIGHT"), 2}, {x_("CFORWARD"),  2},{x_("CBACKWARD"),  2},	/* 10: control held down */
	{x_("ALEFT"),2},  {x_("AMIDDLE"), 2}, {x_("ARIGHT"), 2}, {x_("AFORWARD"),  2},{x_("ABACKWARD"),  2},	/* 15: alt held down */
	{x_("SCLEFT"),3}, {x_("SCMIDDLE"),3}, {x_("SCRIGHT"),3}, {x_("SCFORWARD"), 3},{x_("SCBACKWARD"), 3},	/* 20: control/shift held down */
	{x_("SALEFT"),3}, {x_("SAMIDDLE"),3}, {x_("SARIGHT"),3}, {x_("SAFORWARD"), 3},{x_("SABACKWARD"), 3},	/* 25: shift/alt held down */
	{x_("CALEFT"),3}, {x_("CAMIDDLE"),3}, {x_("CARIGHT"),3}, {x_("CAFORWARD"), 3},{x_("CABACKWARD"), 3},	/* 30: control/alt held down */
	{x_("SCALEFT"),4},{x_("SCAMIDDLE"),4},{x_("SCARIGHT"),4},{x_("SCAFORWARD"),4},{x_("SCABACKWARD"),4},	/* 35: shift/control/alt held down */
	{x_("DLEFT"),2},  {x_("DMIDDLE"), 2}, {x_("DRIGHT"), 2}, {x_("DFORWARD"),  2},{x_("DBACKWARD"),  2}		/* 40: double-click */
};

/****** cursors ******/
static HCURSOR       gra_normalCurs;			/* the default cursor */
static HCURSOR       gra_wantttyCurs;			/* a "use the TTY" cursor */
static HCURSOR       gra_penCurs;				/* a "draw with pen" cursor */
static HCURSOR       gra_menuCurs;				/* a menu selection cursor */
static HCURSOR       gra_handCurs;				/* a hand dragging cursor */
static HCURSOR       gra_techCurs;				/* a technology edit cursor */
static HCURSOR       gra_ibeamCurs;				/* a text edit cursor */
static HCURSOR       gra_waitCurs;				/* an hourglass cursor */
static HCURSOR       gra_lrCurs;				/* a left/right pointing cursor */
static HCURSOR       gra_udCurs;				/* an up/down pointing cursor */

#ifdef WINSAVEDBOX

/****** rectangle saving ******/
#define NOSAVEDBOX ((SAVEDBOX *)-1)

typedef struct Isavedbox
{
	HBITMAP           hBox;
	BITMAPINFO       *bminfo;
	CDC              *hMemDC;
	UCHAR1           *data;
	UCHAR1          **rowstart;
	WINDOWPART       *win;
	INTBIG            lx, hx, ly, hy;
	struct Isavedbox *nextsavedbox;
} SAVEDBOX;

static SAVEDBOX     *gra_firstsavedbox = NOSAVEDBOX;
#endif

/****** fonts ******/
#define MAXCACHEDFONTS 200
#define FONTHASHSIZE   211						/* must be prime */

typedef struct
{
	CFont   *font;
	INTBIG   face;
	INTBIG   italic;
	INTBIG   bold;
	INTBIG   underline;
	INTBIG   size;
} FONTHASH;

static FONTHASH      gra_fonthash[FONTHASHSIZE];
static CFont        *gra_fontcache[MAXCACHEDFONTS];
static INTBIG        gra_numfaces = 0;
static CHAR        **gra_facelist;
static INTBIG        gra_textrotation = 0;
static BOOLEAN       gra_texttoosmall = FALSE;

/****** miscellaneous ******/
#define PALETTEWIDTH        136					/* width of palette */
#define MAXPATHLEN          256					/* max chars in file path */
#define SBARWIDTH            15					/* width of scroll bars */
#define MAXLOCALSTRING      256					/* size of "gra_localstring" */

static INTBIG        gra_screenleft,			/* left bound of any editor window screen */
                     gra_screenright,			/* right bound of any editor window screen */
                     gra_screentop,				/* top bound of any editor window screen */
                     gra_screenbottom;			/* bottom bound of any editor window screen */
static CHAR          gra_localstring[MAXLOCALSTRING];	/* local string */
static void         *gra_fileliststringarray = 0;
static time_t        gra_timebasesec;
static INTBIG        gra_timebasems;
static UINTBIG       gra_timeoffset = 0;		/* offset to system time */

/****** prototypes for internal routines ******/
static void         gra_addeventtoqueue(INTBIG state, INTBIG special, INTBIG x, INTBIG y);
static BOOLEAN      gra_addfiletolist(CHAR *file);
static BOOLEAN      gra_buildoffscreenbuffer(WINDOWFRAME *wf, INTBIG wid, INTBIG hei, CDC **hdc,
						HBITMAP *bitmap, BITMAPINFO **bminfo, UCHAR1 **databuffer, UCHAR1 ***rowstart);
static BOOLEAN      gra_buildwindow(WINDOWFRAME*, BOOLEAN, RECTAREA*);
static void         gra_copyhighlightedtoclipboard(void);
static CFont       *gra_createtextfont(INTBIG, CHAR*, INTBIG, INTBIG, INTBIG);
static BOOLEAN      gra_diaeachdownhandler(INTBIG x, INTBIG y);
static INTBIG       gra_dodialogisinsideuserdrawn(TDIALOG *dia, int x, int y);
static int APIENTRY gra_enumfaces(LPLOGFONT lpLogFont, LPTEXTMETRIC lpTEXTMETRICs, DWORD fFontType, LPVOID lpData);
static void         gra_floatpalette(void);
static void         gra_freewindowframe(WINDOWFRAME *wf);
static BOOLEAN      gra_getbiggeroffscreenbuffer(WINDOWFRAME *wf, INTBIG wid, INTBIG hei);
static void         gra_getdevices(void);
static INTBIG       gra_getdialogitem(TDIALOG *dia, int x, int y);
static RECT        *gra_geteditorwindowlocation(void);
static TDIALOG     *gra_getdialogfromindex(INTBIG index);
static WINDOWFRAME *gra_getframefromindex(INTBIG index);
static CFont       *gra_gettextfont(WINDOWPART*, TECHNOLOGY*, UINTBIG*);
static HWND         gra_getwindow(WINDOWPART *win);
static BOOLEAN      gra_initdialog(DIALOG *dialog, TDIALOG *dia, BOOLEAN modeless);
static void         gra_initfilelist(void);
static BOOLEAN      gra_loggetnextaction(CHAR *message);
static BOOLEAN      gra_logreadline(CHAR *string, INTBIG limit);
static void         gra_logwriteaction(INTBIG inputstate, INTBIG special, INTBIG cursorx, INTBIG cursory, void *extradata);
static void         gra_logwritecomment(CHAR *comment);
static INTBIG       gra_makebutton(INTBIG);
static BOOLEAN      gra_makeeditwindow(WINDOWFRAME*);
static HICON        gra_makeicon(INTBIG data);
static BOOLEAN      gra_makemessageswindow(void);
static CMenu       *gra_makepdmenu(POPUPMENU *);
static BOOLEAN      gra_messagesnotvisible(void);
static INTBIG       gra_nativepopuptif(POPUPMENU **menu, BOOLEAN header, INTBIG left, INTBIG top);
static void         gra_nextevent(void);
static INTBIG       gra_pulldownindex(POPUPMENU *);
static void         gra_redrawdisplay(WINDOWFRAME*);
static void         gra_redrawstatusindicators(void);
static void         gra_reloadmap(void);
static BOOLEAN      gra_remakeeditwindow(WINDOWFRAME*);
static void         gra_removewindowextent(RECT *r, CWnd *wnd);
static void         gra_setdefaultcursor(void);
static INTBIG       gra_setregistry(HKEY key, CHAR *subkey, CHAR *value, CHAR *newstring);
static void         gra_tomessagesbottom(void);
#if LANGTCL
       INTBIG       gra_initializetcl(void);
#endif

#ifdef USEDIRECTSOUND
#  include <dsound.h>	/* need link with "dsound.lib" */
  typedef struct
  {
	IDirectSound       *gpds;
	IDirectSoundBuffer *secondarybuffer;
  } SOUND;
  static void         gra_playsound(SOUND *sound);
  static SOUND       *gra_initsound(CHAR *filename);
  extern "C" { extern HINSTANCE g_hDSoundLib; }
  typedef HRESULT (WINAPI *PFN_DSCREATE)(LPGUID lpguid, LPDIRECTSOUND *ppDS, IUnknown FAR *pUnkOuter);
  static PFN_DSCREATE gra_DSCreate;
#endif

/****** prototypes for externally called routines ******/
       void         gra_activateframe(CChildFrame *frame, BOOL bActivate);
       void         gra_buttonaction(int state, UINT nFlags, CPoint point, CWnd *frm);
       int          gra_closeframe(CChildFrame *frame);
       int          gra_closeworld(void);
       void         gra_diaredrawitem(CElectricDialog*);
       int          gra_dodialoglistkey(CElectricDialog *diawin, UINT nKey, CListBox* pListBox, UINT nIndex);
       void         gra_dodialogtextchange(CElectricDialog *diawin, int nID);
       INTBIG       gra_getregistry(HKEY key, CHAR *subkey, CHAR *value, CHAR *result);
       void         gra_itemclicked(CElectricDialog *diawin, int nID);
       void         gra_itemdoubleclicked(CElectricDialog *diawin, int nID);
       void         gra_itemvscrolled(void *vdia, int nID);
       void         gra_keyaction(UINT nChar, INTBIG special, UINT nRepCnt);
       void         gra_mouseaction(UINT nFlags, CPoint point, CWnd *frm);
       void         gra_mousewheelaction(UINT nFlags, short zDelta, CPoint point, CWnd *frm);
       void         gra_movedwindow(CChildFrame *frame, int x, int y);
       void         gra_nativemenudoone(INTBIG low, INTBIG high);
       void         gra_onint(void);
       void         gra_repaint(CChildFrame *frame, CPaintDC *dc);
       void         gra_resize(CChildFrame *frame, int cx, int cy);
       void         gra_resizemain(int cx, int cy);
       int          gra_setpropercursor(CWnd *wnd, int x, int y);
       void         gra_setrect(WINDOWFRAME*, INTBIG, INTBIG, INTBIG, INTBIG);
	   void         gra_timerticked(void);

/******************** INITIALIZATION ********************/

/*
 * routines to establish the default display
 */
void graphicsoptions(CHAR *name, INTBIG *argc, CHAR1 **argv)
{
	us_erasech = BACKSPACEKEY;
	us_killch = 025;
}

/*
 * routine to initialize the display device
 */
BOOLEAN initgraphics(BOOLEAN messages)
{
	CHAR username[256];
	UINTBIG size;
	INTBIG i;
	REGISTER void *infstr;

	gra_inputstate = NOEVENT;
	gra_pulldownmenucount = 0;	/* number of pulldown menus */
	gra_hMenu = 0;

	/* setup globals that describe location of windows */
	gra_getdevices();

	/* remember the initial directory and the log file locations */
	(void)allocstring(&gra_initialdirectory, currentdirectory(), db_cluster);
	size = 256;
	GetUserName(username, &size);
	infstr = initinfstr();
	addstringtoinfstr(infstr, gra_initialdirectory);
	addstringtoinfstr(infstr, x_("electric_"));
	addstringtoinfstr(infstr, username);
	addstringtoinfstr(infstr, x_(".log"));
	(void)allocstring(&gra_logfile, returninfstr(infstr), db_cluster);
	infstr = initinfstr();
	addstringtoinfstr(infstr, gra_initialdirectory);
	addstringtoinfstr(infstr, x_("electriclast_"));
	addstringtoinfstr(infstr, username);
	addstringtoinfstr(infstr, x_(".log"));
	(void)allocstring(&gra_logfilesave, returninfstr(infstr), db_cluster);

	/* create the scrolling messages window */
	if (!messages) gra_messageswindow = 0; else
	{
	   if (gra_makemessageswindow()) error(_("Cannot create messages window"));
	}

	/* get cursors */
	gra_normalCurs = gra_app.LoadStandardCursor(IDC_ARROW);
	gra_waitCurs = gra_app.LoadStandardCursor(IDC_WAIT);
	gra_ibeamCurs = gra_app.LoadStandardCursor(IDC_IBEAM);
	gra_wantttyCurs = gra_app.LoadCursor(IDC_CURSORTTY);
	if (gra_wantttyCurs == 0) gra_wantttyCurs = gra_normalCurs;
	gra_penCurs = gra_app.LoadCursor(IDC_CURSORPEN);
	if (gra_penCurs == 0) gra_penCurs = gra_normalCurs;
	gra_handCurs = gra_app.LoadCursor(IDC_CURSORHAND);
	if (gra_handCurs == 0) gra_handCurs = gra_normalCurs;
	gra_menuCurs = gra_app.LoadCursor(IDC_CURSORMENU);
	if (gra_menuCurs == 0) gra_menuCurs = gra_normalCurs;
	gra_techCurs = gra_app.LoadCursor(IDC_CURSORTECH);
	if (gra_techCurs == 0) gra_techCurs = gra_normalCurs;
	gra_lrCurs = gra_app.LoadStandardCursor(IDC_SIZEWE);
	if (gra_lrCurs == 0) gra_lrCurs = gra_normalCurs;
	gra_udCurs = gra_app.LoadStandardCursor(IDC_SIZENS);
	if (gra_udCurs == 0) gra_udCurs = gra_normalCurs;

	/* initialize the cursor */
	SetCursor(0);
	us_normalcursor = NORMALCURSOR;
	us_cursorstate = us_normalcursor;

	/* initialize font cache */
	for(i=0; i<MAXCACHEDFONTS; i++) gra_fontcache[i] = 0;
	for(i=0; i<FONTHASHSIZE; i++) gra_fonthash[i].font = 0;

	gra_eventqueuehead = gra_eventqueuetail = gra_eventqueue;
	el_firstwindowframe = el_curwindowframe = NOWINDOWFRAME;
	gra_cureditwindowframe = NOWINDOWFRAME;

	/* get mutex */
	(void)ensurevalidmutex(&gra_eventqueuemutex, TRUE);

	return(FALSE);
}

BOOLEAN gra_makemessageswindow(void)
{
	RECT rmsg;
	CCreateContext context;

	/* create a context to tell this window to be an EditView */
	context.m_pNewViewClass = RUNTIME_CLASS(CElectricMsgView);
	context.m_pCurrentDoc = NULL;
	context.m_pNewDocTemplate = NULL;
	context.m_pLastView = NULL;
	context.m_pCurrentFrame = NULL;

	/* set messages window location */
	rmsg.left = gra_messagesleft;
	rmsg.right = gra_messagesright;
	rmsg.top = gra_messagestop;
	rmsg.bottom = gra_messagesbottom;

	/* create the window */
	gra_messageswindow = new CChildFrame();
	gra_messageswindow->Create(NULL, _("Electric Messages"),
		WS_CHILD|WS_OVERLAPPEDWINDOW|WS_VISIBLE, rmsg, NULL, &context);
	gra_messageswindow->InitialUpdateFrame(NULL, TRUE);
	CRichEditView *msgView = (CRichEditView *)gra_messageswindow->GetActiveView();
	gra_editCtrl = &msgView->GetRichEditCtrl();

	gra_messagescurrent = FALSE;
	return(FALSE);
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
	RECT rDesktop;

	/* obtain the current device */
	gra_app.m_pMainWnd->GetClientRect(&rDesktop);
	rDesktop.bottom -= 22;		/* status bar height? */

	gra_screenleft   = 0;
	gra_screenright  = rDesktop.right-rDesktop.left;
	gra_screentop    = 0;
	gra_screenbottom = rDesktop.bottom-rDesktop.top;

	gra_messagesleft   = gra_screenleft + PALETTEWIDTH;
	gra_messagesright  = gra_screenright - PALETTEWIDTH;
	gra_messagestop    = gra_screentop - 2 + (gra_screenbottom-gra_screentop) * 4 / 5;
	gra_messagesbottom = gra_screenbottom;
}

/******************** TERMINATION ********************/

void termgraphics(void)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG i;

	gra_termgraph();

	while (el_firstwindowframe != NOWINDOWFRAME)
	{
		wf = el_firstwindowframe;
		el_firstwindowframe = el_firstwindowframe->nextwindowframe;
		delete (CChildFrame *)wf->wndframe;
		gra_freewindowframe(wf);
	}
	if (gra_messageswindow != 0)
		delete gra_messageswindow;

	if (gra_textbufinited)
	{
		efree((CHAR *)gra_textrowstart);
		efree((CHAR *)gra_textbitmapinfo);
		DeleteObject(gra_textbitmap);
		delete gra_texthdc;
	}

	if (gra_biggeroffbufinited)
	{
		efree((CHAR *)gra_biggeroffrowstart);
		efree((CHAR *)gra_biggeroffbitmapinfo);
		DeleteObject(gra_biggeroffbitmap);
		delete gra_biggeroffhdc;
	}

	if (gra_pulldownmenucount != 0)
	{
		efree((CHAR *)gra_pulldownmenus);
		for(i=0; i<gra_pulldownmenucount; i++) efree(gra_pulldowns[i]);
		efree((CHAR *)gra_pulldowns);
	}

	if (gra_fileliststringarray != 0)
		killstringarray(gra_fileliststringarray);
	efree((CHAR *)gra_initialdirectory);
	efree((CHAR *)gra_logfile);
	efree((CHAR *)gra_logfilesave);

	/* free cached fonts */
	for(i=0; i<MAXCACHEDFONTS; i++)
	{
		if (gra_fontcache[i] != 0) delete gra_fontcache[i];
		gra_fontcache[i] = 0;
	}
	for(i=0; i<FONTHASHSIZE; i++)
	{
		if (gra_fonthash[i].font != 0) delete gra_fonthash[i].font;
		gra_fonthash[i].font = 0;
	}
	for(i=0; i<gra_numfaces; i++) efree((CHAR *)gra_facelist[i]);
	if (gra_numfaces > 0) efree((CHAR *)gra_facelist);
}

void exitprogram(void)
{
	exit(0);
}

/******************** WINDOW CONTROL ********************/

WINDOWFRAME *newwindowframe(BOOLEAN floating, RECTAREA *r)
{
	WINDOWFRAME *wf, *oldlisthead;

	/* allocate one */
	wf = (WINDOWFRAME *)emalloc((sizeof (WINDOWFRAME)), us_tool->cluster);
	if (wf == 0) return(NOWINDOWFRAME);
	wf->numvar = 0;
	wf->rowstart = 0;
	wf->windindex = gra_windownumberindex++;

	/* insert window-frame in linked list */
	oldlisthead = el_firstwindowframe;
	wf->nextwindowframe = el_firstwindowframe;
	el_firstwindowframe = wf;
	el_curwindowframe = wf;
	if (floating) gra_palettewindowframe = wf; else
		gra_cureditwindowframe = wf;

	/* load an editor window into this frame */
	wf->pColorPalette = 0;
	if (gra_buildwindow(wf, floating, r))
	{
		efree((CHAR *)wf);
		el_firstwindowframe = oldlisthead;
		return(NOWINDOWFRAME);
	}
	return(wf);
}

void killwindowframe(WINDOWFRAME *owf)
{
	WINDOWFRAME *wf, *lastwf;

	/* kill the actual window */
	((CChildFrame *)owf->wndframe)->DestroyWindow();

	/* find this frame in the list */
	lastwf = NOWINDOWFRAME;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf == owf) break;
		lastwf = wf;
	}
	if (wf == NOWINDOWFRAME) return;
	if (lastwf == NOWINDOWFRAME) el_firstwindowframe = owf->nextwindowframe; else
		lastwf->nextwindowframe = owf->nextwindowframe;
	if (el_curwindowframe == owf) el_curwindowframe = NOWINDOWFRAME;
	if (gra_cureditwindowframe == owf) gra_cureditwindowframe = NOWINDOWFRAME;

	gra_freewindowframe(owf);
}

void gra_freewindowframe(WINDOWFRAME *wf)
{
	if (wf->rowstart != 0)
	{
		efree((CHAR *)wf->rowstart);
		efree((CHAR *)wf->bminfo);
		DeleteObject(wf->hBitmap);
		delete ((CDC *)wf->hDCOff);
		efree((CHAR *)wf->pColorPalette);
	}
	efree((CHAR *)wf);
}

WINDOWFRAME *getwindowframe(BOOLEAN canfloat)
{
	if (el_curwindowframe != NOWINDOWFRAME)
	{
		if (canfloat || !el_curwindowframe->floating)
			return(el_curwindowframe);
	}
	if (gra_cureditwindowframe != NOWINDOWFRAME)
		return(gra_cureditwindowframe);
	return(NOWINDOWFRAME);
}

/*
 * routine to return size of window frame "wf" in "wid" and "hei"
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
	RECT fr;
	CWnd *parent;

	parent = gra_messageswindow->GetParent();
	gra_messageswindow->GetWindowRect(&fr);
	parent->ScreenToClient(&fr);
	*top = fr.top;
	*left = fr.left;
	*bottom = fr.bottom;
	*right = fr.right;
}

/*
 * Routine to set the size and position of the messages window.
 */
void setmessagesframeinfo(INTBIG top, INTBIG left, INTBIG bottom, INTBIG right)
{
	(void)gra_messageswindow->SetWindowPos(0, left, top, right-left, bottom-top,
		SWP_NOZORDER);
}

void sizewindowframe(WINDOWFRAME *wf, INTBIG wid, INTBIG hei)
{
	RECT fr, cr;
	int framewid, framehei;
	WINDOWPLACEMENT wp;

	/* if window frame already this size, stop now */
	if (wid == wf->swid && hei == wf->shei) return;

	/* resize window if needed */
	((CChildFrame *)wf->wndframe)->GetClientRect(&cr);
	if (wid != cr.right-cr.left || hei != cr.bottom-cr.top)
	{
		((CChildFrame *)wf->wndframe)->GetWindowPlacement(&wp);
		fr = wp.rcNormalPosition;

		framewid = (fr.right - fr.left) - (cr.right - cr.left);
		framehei = (fr.bottom - fr.top) - (cr.bottom - cr.top);

		/* determine new window size */
		wid += framewid;
		hei += framehei;

		/* resize the window */
		if (((CChildFrame *)wf->wndframe)->SetWindowPos(0, 0, 0, wid, hei, SWP_NOMOVE|SWP_NOZORDER) == 0)
		{
			/* failed to resize window */
			return;
		}
	}

	/* rebuild the offscreen windows */
	if (gra_remakeeditwindow(wf))
	{
//		fr.left += 1;   fr.right -= 2;   fr.bottom -= 2;
//		SetWindowPos(wf->realwindow, NULL, 0, 0, fr.right-fr.left, fr.bottom-fr.top, SWP_NOZORDER | SWP_NOMOVE);
		return;
	}
	gra_reloadmap();
}

void movewindowframe(WINDOWFRAME *wf, INTBIG left, INTBIG top)
{
	RECT fr;
	WINDOWPLACEMENT wp;

	((CChildFrame *)wf->wndframe)->GetWindowPlacement(&wp);
	fr = wp.rcNormalPosition;

	/* determine new window location */
	if (left == fr.left && top == fr.top) return;
	((CChildFrame *)wf->wndframe)->SetWindowPos(0, left, top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
}

/*
 * Routine to close the messages window if it is in front.  Returns true if the
 * window was closed.
 */
BOOLEAN closefrontmostmessages(void)
{
	if (gra_messagescurrent && gra_messageswindow != 0)
	{
		gra_messageswindow->DestroyWindow();
		gra_messagescurrent = FALSE;
		gra_messageswindow = 0;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to bring window "win" to the front.
 */
void bringwindowtofront(WINDOWFRAME *fwf)
{
	REGISTER WINDOWFRAME *wf;

	/* validate the window frame */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (wf == fwf) break;
	if (wf == NOWINDOWFRAME)
	{
		ttyputmsg(x_("Warning: attempt to manipulate deleted window"));
		return;
	}

	/* bring it to the top */
	((CChildFrame *)fwf->wndframe)->BringWindowToTop();
}

/*
 * Routine to organize the windows according to "how" (0: tile horizontally,
 * 1: tile vertically, 2: cascade).
 */
void adjustwindowframe(INTBIG how)
{
	RECT r;
	REGISTER INTBIG children, ret;
	REGISTER WINDOWFRAME *wf;
	HWND *kids;
	CWnd *parent, *compmenu;

	/* figure out how many windows need to be rearranged */
	children = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (!wf->floating) children++;
	if (children <= 0) return;

	/* make a list of windows to rearrange */
	kids = (HWND *)emalloc(children * (sizeof (HWND)), el_tempcluster);
	if (kids == 0) return;
	children = 0;
	compmenu = NULL;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (!wf->floating)
		{
			kids[children++] = wf->realwindow;
			parent = ((CChildFrame *)wf->wndframe)->GetParent();
		} else
		{
			compmenu = (CChildFrame *)wf->wndframe;
		}
	}

	/* determine area for windows */
	parent->GetClientRect(&r);
	if (compmenu != NULL)
	{
		/* remove component menu from area of tiling */
		gra_removewindowextent(&r, compmenu);
	}
	if (gra_messageswindow != 0)
	{
		/* remove messages menu from area of tiling */
		gra_removewindowextent(&r, gra_messageswindow);
	}

	/* rearrange the windows */
	switch (how)
	{
		case 0:		/* tile horizontally */
			ret = TileWindows(parent->m_hWnd, MDITILE_HORIZONTAL, &r, children, kids);
			break;
		case 1:		/* tile vertically */
			ret = TileWindows(parent->m_hWnd, MDITILE_VERTICAL, &r, children, kids);
			break;
		case 2:		/* cascade */
			ret = CascadeWindows(parent->m_hWnd, 0, &r, children, kids);
			break;
	}
	if (ret == 0)
	{
		ret = GetLastError();
		ttyputerr(_("Window rearrangement failed (error %ld)"), ret);
	}
	efree((CHAR *)kids);
}

/*
 * Helper routine to remove the location of window "wnd" from the rectangle "r".
 */
void gra_removewindowextent(RECT *r, CWnd *wnd)
{
	RECT er;
	CWnd *parent;

	parent = wnd->GetParent();
	wnd->GetWindowRect(&er);
	parent->ScreenToClient(&er);
	if (er.right-er.left > er.bottom-er.top)
	{
		/* horizontal occluding window */
		if (er.bottom - r->top < r->bottom - er.top)
		{
			/* occluding window on top */
			r->top = er.bottom;
		} else
		{
			/* occluding window on bottom */
			r->bottom = er.top;
		}
	} else
	{
		/* vertical occluding window */
		if (er.right - r->left < r->right - er.left)
		{
			/* occluding window on left */
			r->left = er.right;
		} else
		{
			/* occluding window on right */
			r->right = er.left;
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
	RECT r;
	CMainFrame *wnd;

	wnd = (CMainFrame *)AfxGetMainWnd();
	wnd->GetClientRect(&r);
	*hei = r.bottom - r.top - 50;
	*wid = r.right - r.left;
	*palettewidth = PALETTEWIDTH;
}

/*
 * Routine called when the component menu has moved to a different location
 * on the screen (left/right/top/bottom).  Resets local state of the position.
 */
void resetpaletteparameters(void)
{
}

RECT *gra_geteditorwindowlocation(void)
{
	static RECT redit;

	redit.top = gra_newwindowoffset + gra_screentop;
	redit.bottom = redit.top + (gra_screenbottom-gra_screentop) * 4 / 5 - 1;
	redit.left = gra_newwindowoffset + gra_screenleft + PALETTEWIDTH;
	redit.right = redit.left + (gra_screenright-gra_screenleft-PALETTEWIDTH) * 4 / 5;

	/* is the editing window visible on this display? */
	if ((redit.bottom > gra_screenbottom) || (redit.right > gra_screenright))
	{
		gra_newwindowoffset = 0;
		redit.top = gra_screentop;
		redit.bottom = redit.top + (gra_screenbottom-gra_screentop) * 3 / 5;
		redit.left = gra_screenleft + PALETTEWIDTH;
		redit.right = redit.left + (gra_screenright-gra_screenleft-PALETTEWIDTH) * 4 / 5;
	}
	gra_newwindowoffset += 30;
	return(&redit);
}

/*
 * routine to build a new window frame
 */
BOOLEAN gra_buildwindow(WINDOWFRAME *wf, BOOLEAN floating, RECTAREA *r)
{
	RECT redit;
	REGISTER INTBIG i;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	CChildFrame *tempFrame;

	wf->floating = floating;
	wf->wndframe = 0;
	if (r != 0)
	{
		redit.left = r->left;   redit.right = r->right;
		redit.top = r->top;     redit.bottom = r->bottom;
	}
	if (!floating)
	{
		/* get editor window location */
		if (r == 0) redit = *gra_geteditorwindowlocation();

		/* create the editing window */
		tempFrame = new CChildFrame();
		tempFrame->Create(NULL, x_("Electric"),
			WS_CHILD|WS_OVERLAPPEDWINDOW|WS_VISIBLE, redit, NULL, NULL);
		wf->wndframe = tempFrame;
		wf->realwindow = tempFrame->m_hWnd;
	} else
	{
		/* default palette window location */
		if (r == 0)
		{
			redit.top = gra_screentop + 2;
			redit.bottom = (gra_screentop+gra_screenbottom)/2;
			redit.left = gra_screenleft + 1;
			redit.right = redit.left + PALETTEWIDTH/2;
		}

		/* create the floating window */
		gra_creatingwindow = TRUE;
		tempFrame = new CChildFrame();
		tempFrame->m_wndFloating = 1;
		tempFrame->Create(NULL, x_("Components"),
			WS_CHILD|WS_VISIBLE|WS_CAPTION, redit, NULL, NULL);
		gra_creatingwindow = FALSE;
		wf->wndframe = tempFrame;
		wf->realwindow = tempFrame->m_hWnd;
	}
	if (wf->realwindow == 0) return(TRUE);

	wf->hDC = (CDC *)((CChildFrame *)wf->wndframe)->GetDC();
	((CChildFrame *)wf->wndframe)->ReleaseDC((CDC *)wf->hDC);
	((CChildFrame *)wf->wndframe)->ShowWindow(SW_SHOW);
	wf->hPalette = 0;

	/* load any map the first time to establish the map segment length */
	el_maplength = 256;

	/* create a color palette for the buffer */
	varred   = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue  = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred != NOVARIABLE && vargreen != NOVARIABLE && varblue != NOVARIABLE)
	{
		wf->pColorPalette = (LOGPALETTE *)emalloc(256 * (sizeof (PALETTEENTRY)) + 2 * (sizeof (WORD)),
			db_cluster);
		if (wf->pColorPalette == 0) return(TRUE);
		wf->pColorPalette->palVersion = 0x300;
		wf->pColorPalette->palNumEntries = 256;
		for(i=0; i<256; i++)
		{
			wf->pColorPalette->palPalEntry[i].peRed   = (UCHAR1)((INTBIG *)varred->addr)[i];
			wf->pColorPalette->palPalEntry[i].peGreen = (UCHAR1)((INTBIG *)vargreen->addr)[i];
			wf->pColorPalette->palPalEntry[i].peBlue  = (UCHAR1)((INTBIG *)varblue->addr)[i];
			wf->pColorPalette->palPalEntry[i].peFlags = 0;
		}
		wf->pColorPalette->palPalEntry[255].peRed   = (UCHAR1)(255-((INTBIG *)varred->addr)[255]);
		wf->pColorPalette->palPalEntry[255].peGreen = (UCHAR1)(255-((INTBIG *)vargreen->addr)[255]);
		wf->pColorPalette->palPalEntry[255].peBlue  = (UCHAR1)(255-((INTBIG *)varblue->addr)[255]);
		wf->hPalette = (CPalette *)new CPalette();
		((CPalette *)wf->hPalette)->CreatePalette(wf->pColorPalette);
	}

	/* build the offscreen buffer for the window */
	if (gra_makeeditwindow(wf))
	{
/*		DestroyWindow(wf->realwindow); */
		return(TRUE);
	}

	return(FALSE);
}

/*
 * Routine to redraw editing window "wf" due to a drag or grow
 */
void gra_redrawdisplay(WINDOWFRAME *wf)
{
	us_beginchanges();
	us_drawmenu(-1, wf);
	us_endchanges(NOWINDOWPART);
	us_state |= HIGHLIGHTSET;
	us_showallhighlight();
}

/*
 *  Routine to allocate the offscreen buffer that corresponds with the window
 *  "wf->realwindow".  The buffer is 8-bits deep and is stored in "wf->window".
 *  Returns true if memory cannot be allocated.
 */
BOOLEAN gra_makeeditwindow(WINDOWFRAME *wf)
{
	RECT r;

	/* get information about the window that was just created */
	((CChildFrame *)wf->wndframe)->GetClientRect(&r);

	/* set frame characteristics */
	wf->swid = r.right - r.left;
	wf->shei = r.bottom - r.top;
	wf->revy = wf->shei - 1;
	wf->offscreendirty = FALSE;

	/* allocate the main offscreen buffer */
	if (gra_buildoffscreenbuffer(wf, wf->swid, wf->shei, (CDC **)&wf->hDCOff, &wf->hBitmap,
		&wf->bminfo, &wf->data, &wf->rowstart)) return(TRUE);

	/* attach the color palette to drawing window section */
	if (wf->hPalette != 0)
	{
		(void)((CDC *)wf->hDCOff)->SelectPalette((CPalette *)wf->hPalette, TRUE);
		(void)((CDC *)wf->hDCOff)->RealizePalette();
	}

	return(FALSE);
}

/*
 * Routine to build an offscreen buffer that is "wid" by "hei" and based on window frame "wf".
 * The device context is placed in "hdc", the bitmap in "bitmap", the actual offscreen memory in
 * "databuffer" and the row start array in "rowstart".  Returns true on error.
 */
BOOLEAN gra_buildoffscreenbuffer(WINDOWFRAME *wf, INTBIG wid, INTBIG hei, CDC **hdc,
	HBITMAP *bitmap, BITMAPINFO **bmiInfo, UCHAR1 **databuffer, UCHAR1 ***rowstart)
{
	BITMAPINFOHEADER bmiHeader;
	REGISTER VARIABLE *varred, *vargreen, *varblue;
	RGBQUAD bmiColors[256];
	REGISTER INTBIG i, j;
	REGISTER BOOLEAN retval;
	REGISTER INTBIG size;
	REGISTER UCHAR1 *ptr;

	/* make the info structure */
	*bmiInfo = (BITMAPINFO *)emalloc(sizeof(bmiHeader) + sizeof(bmiColors), db_cluster);
	if (*bmiInfo == 0) return(TRUE);

	/* grab the device context */
	wf->hDC = (CDC *)((CChildFrame *)wf->wndframe)->GetDC();

	/* setup the header block */
	bmiHeader.biSize = (DWORD)sizeof(bmiHeader);
	bmiHeader.biWidth = (LONG)wid;
	bmiHeader.biHeight = (LONG)-hei;
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 8;
	bmiHeader.biCompression = (DWORD)BI_RGB;
	bmiHeader.biSizeImage = 0;
	bmiHeader.biXPelsPerMeter = 0;
	bmiHeader.biYPelsPerMeter = 0;
	bmiHeader.biClrUsed = 256;
	bmiHeader.biClrImportant = 0;

	/* get color map information */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred != NOVARIABLE && vargreen != NOVARIABLE && varblue != NOVARIABLE)
	{
		for(i=0; i<255; i++)
		{
			bmiColors[i].rgbRed = (UCHAR1)((INTBIG *)varred->addr)[i];
			bmiColors[i].rgbGreen = (UCHAR1)((INTBIG *)vargreen->addr)[i];
			bmiColors[i].rgbBlue = (UCHAR1)((INTBIG *)varblue->addr)[i];
			bmiColors[i].rgbReserved = 0;
		}
		bmiColors[255].rgbRed = (UCHAR1)(255-((INTBIG *)varred->addr)[255]);
		bmiColors[255].rgbGreen = (UCHAR1)(255-((INTBIG *)vargreen->addr)[255]);
		bmiColors[255].rgbBlue = (UCHAR1)(255-((INTBIG *)varblue->addr)[255]);
	}

	/* create a device context for this offscreen buffer */
	retval = TRUE;
	*hdc = new CDC();
	BOOL result = (*hdc)->CreateCompatibleDC((CDC *)wf->hDC);
	if (result)
	{
		/* prepare structure describing offscreen buffer */
		ptr = (UCHAR1 *)*bmiInfo;
		memcpy(ptr, &bmiHeader, sizeof(bmiHeader));
		ptr += sizeof(bmiHeader);
		memcpy(ptr, &bmiColors, sizeof(bmiColors));

		/* allocate offscreen buffer */
		*bitmap = CreateDIBSection((*hdc)->m_hDC, *bmiInfo, DIB_RGB_COLORS, (void **)databuffer, NULL, 0);
		if (*bitmap != 0)
		{
			(*hdc)->SelectObject(CBitmap::FromHandle(*bitmap));

			/* setup row pointers to offscreen buffer */
			*rowstart = (UCHAR1 **)emalloc(hei * (sizeof (UCHAR1 *)), db_cluster);
			if (*rowstart == 0) return(TRUE);
			size = (wid+3)/4*4;
			for(j=0; j<hei; j++) (*rowstart)[j] = (*databuffer) + (size * j);
			retval = FALSE;
		}
	}

	/* release the device context */
	((CChildFrame *)wf->wndframe)->ReleaseDC((CDC *)wf->hDC);
	return(retval);
}

/*
 * Routine to rebuild the offscreen buffer when its size or depth has changed.
 * Returns true if the window cannot be rebuilt.
 */
BOOLEAN gra_remakeeditwindow(WINDOWFRAME *wf)
{
	/* free memory associated with the frame */
	if (wf->rowstart != 0)
	{
		efree((CHAR *)wf->rowstart);
		efree((CHAR *)wf->bminfo);
		DeleteObject(wf->hBitmap);
		delete (CDC *)wf->hDCOff;
	}
	wf->rowstart = 0;

	return(gra_makeeditwindow(wf));
}

/******************** MISCELLANEOUS EXTERNAL ROUTINES ********************/

/*
 * return true if the capabilities in "want" are present
 */
BOOLEAN graphicshas(INTBIG want)
{
	/* only 1 status area, shared by each frame */
	if ((want&CANSTATUSPERFRAME) != 0) return(FALSE);

	/* cannot run subprocesses */
	if ((want&CANRUNPROCESS) != 0) return(FALSE);

	return(TRUE);
}

/*
 * Routine to make sound "sound".  If sounds are turned off, no
 * sound is made (unless "force" is TRUE)
 */
void ttybeep(INTBIG sound, BOOLEAN force)
{
	void *infstr;
#ifdef USEDIRECTSOUND
	static BOOLEAN clicksoundinited = FALSE;
	static SOUND *clicksound = 0;
#endif

	if ((us_tool->toolstate & TERMBEEP) != 0 || force)
	{
		switch (sound)
		{
			case SOUNDBEEP:
				MessageBeep(MB_ICONASTERISK);
				break;
			case SOUNDCLICK:
				if ((us_useroptions&NOEXTRASOUND) != 0) break;
#ifdef USEDIRECTSOUND
				/* this crashed on some Windows 2000 systems (a laptop) */
				/* try "waveOutOpen()" and others... */
				if (!clicksoundinited)
				{
					infstr = initinfstr();
					formatinfstr(infstr, x_("%sclick.wav"), el_libdir);
					clicksound = gra_initsound(returninfstr(infstr));
					clicksoundinited = TRUE;
				}
				if (clicksound != 0)
					gra_playsound(clicksound);
#else
				infstr = initinfstr();
				formatinfstr(infstr, x_("%sclick.wav"), el_libdir);
				PlaySound(returninfstr(infstr), 0, SND_FILENAME);
#endif
				break;
		}
	}
}

#ifdef USEDIRECTSOUND

/*
 * Routine to create a SOUND object for the WAVE file "filename".  Returns zero on error.
 */
SOUND *gra_initsound(CHAR *filename)
{
	UINTBIG i, buffersize, len1, len2;
	UCHAR1 *data, *ptr1 = NULL, *ptr2 = NULL;
	SOUND *sound;
	CMainFrame *frame;
	WORD extrabytes;
	DSBUFFERDESC sbuf;
	IDirectSoundBuffer *primarybuffer;
	WAVEFORMATEX *waveinfo;
	HMMIO mmio;
	MMIOINFO mmioinfo;
	MMCKINFO mmiobufheader, mmiobuf;
	PCMWAVEFORMAT formatchunk;

	/* make the object */
	sound = (SOUND *)emalloc(sizeof (SOUND), us_tool->cluster);
	if (sound == 0) return(0);
	sound->gpds = 0;
	sound->secondarybuffer = 0;

	gra_DSCreate = (PFN_DSCREATE)GetProcAddress(g_hDSoundLib, x_("DirectSoundCreate"));
	if (gra_DSCreate == NULL)
	{
		return(0);
	}

	/* open the direct sound system */
	if (DirectSoundCreate(NULL, &sound->gpds, NULL) != DS_OK) return(0);
	frame = (CMainFrame *)AfxGetMainWnd();
	if (sound->gpds->SetCooperativeLevel(frame->m_hWnd, DSSCL_PRIORITY) != DS_OK) return(0);

	/* make a primary sound buffer */
	ZeroMemory(&sbuf, sizeof(DSBUFFERDESC));
	sbuf.dwSize = sizeof (DSBUFFERDESC);
	sbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (sound->gpds->CreateSoundBuffer(&sbuf, &primarybuffer, NULL) != DS_OK) return(0);

	/* get the wave file */
	mmio = mmioOpen(filename, NULL, MMIO_ALLOCBUF|MMIO_READ);
	if (mmio == NULL) return(0);
	if (mmioDescend(mmio, &mmiobufheader, NULL, 0) != 0) return(0);
	if (mmiobufheader.ckid != FOURCC_RIFF ||
		mmiobufheader.fccType != mmioFOURCC('W', 'A', 'V', 'E')) return(0);

	/* find the format chunk of the wave file */
	mmiobuf.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if (mmioDescend(mmio, &mmiobuf, &mmiobufheader, MMIO_FINDCHUNK) != 0) return(0);
	if (mmiobuf.cksize < sizeof(PCMWAVEFORMAT)) return(0);
	if (mmioRead(mmio, (UCHAR1 *)&formatchunk, sizeof(formatchunk)) != sizeof(formatchunk))
		return(0);

	/* allocate the waveformat structure */
	extrabytes = 0;
	if (formatchunk.wf.wFormatTag != WAVE_FORMAT_PCM)
	{
		if (mmioRead(mmio, (UCHAR1 *)&extrabytes, sizeof(extrabytes)) != sizeof(extrabytes))
			return(0);
	}
	waveinfo = (WAVEFORMATEX *)emalloc(sizeof(WAVEFORMATEX)+extrabytes, db_cluster);
	if (waveinfo == 0) return(0);

	/* load the waveformat structure */
	memcpy(waveinfo, &formatchunk, sizeof(formatchunk));
	waveinfo->cbSize = extrabytes;
	if (extrabytes != 0)
	{
		if (mmioRead(mmio, (UCHAR1 *)(((UCHAR1 *)&waveinfo->cbSize)+sizeof(extrabytes)),
			extrabytes) != extrabytes) return(0);
	}

	/* exit the format chunk of the wave file */
	if (mmioAscend(mmio, &mmiobuf, 0) != 0) return(0);

	/* find the data chunk of the wave file */
	(void)mmioSeek(mmio, mmiobufheader.dwDataOffset + sizeof(FOURCC), SEEK_SET);
	mmiobuf.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if (mmioDescend(mmio, &mmiobuf, &mmiobufheader, MMIO_FINDCHUNK) != 0) return(0);

	/* allocate space for the data */
	data = (UCHAR1 *)emalloc(mmiobuf.cksize, db_cluster);
	if (data == 0) return(0);

	/* get the data from the wave file */
	if (mmioGetInfo(mmio, &mmioinfo, 0) != 0) return(0);
	buffersize = mmiobuf.cksize;
	if (buffersize > mmiobuf.cksize) buffersize = mmiobuf.cksize;
	mmiobuf.cksize -= buffersize;

	/* copy the data */
	for (i = 0; i < buffersize; i++)
	{
		if (mmioinfo.pchNext == mmioinfo.pchEndRead)
		{
			if (mmioAdvance(mmio, &mmioinfo, MMIO_READ) != 0) return(0);
			if (mmioinfo.pchNext == mmioinfo.pchEndRead) return(0);
		}
		data[i] = *mmioinfo.pchNext++;
	}

	/* clean up the wave file */
	if (mmioSetInfo(mmio, &mmioinfo, 0) != 0) return(0);
	mmioClose(mmio, 0);

	/* create a sound buffer */
	ZeroMemory(&sbuf, sizeof(DSBUFFERDESC));
	sbuf.dwSize = sizeof (DSBUFFERDESC);
	sbuf.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_STATIC;
	sbuf.dwBufferBytes = buffersize;
	sbuf.lpwfxFormat = waveinfo;
	if (sound->gpds->CreateSoundBuffer(&sbuf, &sound->secondarybuffer, NULL) != 0) return(0);

	/* load the data into the buffer */
	if (sound->secondarybuffer->Lock(0, buffersize, (void **)&ptr1, &len1,
		(void **)&ptr2, &len2, 0L) != 0) return(0);
	CopyMemory(ptr1, data, buffersize);
	if (sound->secondarybuffer->Unlock(ptr1, buffersize, ptr2, 0) != 0) return(0);
	return(sound);
}

/*
 * Routine to play sound "sound" and only return when it is finished playing.
 */
void gra_playsound(SOUND *sound)
{
	UINTBIG status;

	if (sound->secondarybuffer == 0) return;

	/* play the sound */
	sound->secondarybuffer->SetCurrentPosition(0);
	if (sound->secondarybuffer->Play(0, 0, 0) != DS_OK) return;

	/* query the sound */
	for(;;)
	{
		if (sound->secondarybuffer->GetStatus(&status) != DS_OK) return;
		if ((status&DSBSTATUS_PLAYING) == 0) break;
	}
}
#endif

extern "C" {
DIALOGITEM db_severeerrordialogitems[] =
{
 /*  1 */ {0, {80,8,104,72}, BUTTON, N_("Exit")},
 /*  2 */ {0, {80,96,104,160}, BUTTON, N_("Save")},
 /*  3 */ {0, {80,184,104,256}, BUTTON, N_("Continue")},
 /*  4 */ {0, {8,8,72,256}, MESSAGE, x_("")}
};
DIALOG db_severeerrordialog = {{50,75,163,341}, N_("Fatal Error"), 0, 4, db_severeerrordialogitems, 0, 0};
};

#define DSER_EXIT      1		/* Exit (button) */
#define DSER_SAVE      2		/* Save (button) */
#define DSER_CONTINUE  3		/* Continue (button) */
#define DSER_ERRMSG    4		/* Error message (stat text) */

void error(CHAR *s, ...)
{
	va_list ap;
	CHAR line[500];
	REGISTER INTBIG itemHit, *curstate;
	REGISTER INTBIG retval;
	REGISTER LIBRARY *lib;
	REGISTER void *dia;

	/* disable any session logging */
	if (us_logplay != NULL)
	{
		xclose(us_logplay);
		us_logplay = NULL;
	}

	/* build the error message */
	var_start(ap, s);
	evsnprintf(line, 500, s, ap);
	va_end(ap);

	/* display the severe error dialog box */
	dia = DiaInitDialog(&db_severeerrordialog);
	if (dia) return;

	/* load the message */
	DiaSetText(dia, DSER_ERRMSG, line);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DSER_EXIT) exitprogram();
		if (itemHit == DSER_SAVE)
		{
			/* save libraries: make sure that backups are kept */
			curstate = io_getstatebits();
			if ((curstate[0]&BINOUTBACKUP) == BINOUTNOBACK)
			{
				curstate[0] |= BINOUTONEBACK;
				io_setstatebits(curstate);
			}

			/* save each modified library */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
				if ((lib->userbits&(LIBCHANGEDMAJOR|LIBCHANGEDMINOR)) == 0) continue;

				/* save the library in binary format */
				makeoptionstemporary(lib);
				retval = asktool(io_tool, x_("write"), (INTBIG)lib, (INTBIG)x_("binary"));
				restoreoptionstate(lib);
				if (retval != 0) return;
			}
		}
		if (itemHit == DSER_CONTINUE) break;
	}
	DiaDoneDialog(dia);
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
	SYSTEM_INFO si;

	GetSystemInfo(&si);
	return(si.dwNumberOfProcessors);
}

#define MFCTHREADS 1			/* comment out to use WIN32 versions */

#define MUTEXCLASS CSemaphore
//#define MUTEXCLASS CCriticalSection
//#define MUTEXCLASS CSingleLock

/*
 * Routine to create a new thread that calls "function" with "argument".
 */
void enewthread(void* (*function)(void*), void *argument)
{
#ifdef MFCTHREADS
	CWinThread *thread = AfxBeginThread((AFX_THREADPROC)function, argument);
#else
	UINTBIG threadid;

	if (CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)function, argument, 0, &threadid) == 0)
	{
		/* function failed */
		ttyputmsg(x_("Failed to create a thread"));
	}
#endif
}

/*
 * Routine that creates a mutual-exclusion object and returns it.
 */
void *emakemutex(void)
{
#ifdef MFCTHREADS
	MUTEXCLASS *mutex;

	mutex = new MUTEXCLASS();
	mutex->Unlock();
	return((void *)mutex);
#else
	HANDLE mutex;
	static INTBIG mutexindex = 1;
	CHAR mutexname[50];

	esnprintf(mutexname, 50, x_("mutex%ld"), mutexindex++);
	mutex = ::CreateMutex(NULL, FALSE, mutexname);
	return((void *)mutex);
#endif
}

/*
 * Routine that locks mutual-exclusion object "vmutex".  If the object is already
 * locked, the routine blocks until it is unlocked.
 */
void emutexlock(void *vmutex)
{
#ifdef MFCTHREADS
	MUTEXCLASS *mutex;

	mutex = (MUTEXCLASS *)vmutex;
	mutex->Lock();
#else
	HANDLE mutex;
	int result;

	mutex = (HANDLE)vmutex;
	result = WaitForSingleObject(mutex, INFINITE);
	if (result != WAIT_OBJECT_0)
		ttyputerr(x_("Wait on mutex %ld returned %ld"), mutex, result);
#endif
}

/*
 * Routine that unlocks mutual-exclusion object "vmutex".
 */
void emutexunlock(void *vmutex)
{
#ifdef MFCTHREADS
	MUTEXCLASS *mutex;

	mutex = (MUTEXCLASS *)vmutex;
	mutex->Unlock();
#else
	HANDLE mutex;

	mutex = (HANDLE)vmutex;
	ReleaseMutex(mutex);
#endif
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

/*
 * Routine to establish the library directories from the environment.
 * The registry entry for this is stored in:
 *   HKEY_LOCAL_MACHINE\Software\Static Free Software\Electric\ELECTRIC_LIBDIR
 * If this directory is valid (i.e. has the file "cadrc" in it) then it is
 * used.  If the directory is not valid, the registry entry is removed.
 */
void setupenvironment(void)
{
	CHAR tmp[256], subkey[256], value[256], elibname[256], testfile[256], applocation[256];
	REGISTER INTBIG doupdate;
	FILE *io;

	/* default library location */
	(void)allocstring(&el_libdir, LIBDIR, db_cluster);

	/* see if the registry has an overriding library directory path */
	estrcpy(subkey, x_("Software\\Static Free Software\\Electric"));
	estrcpy(value, x_("ELECTRIC_LIBDIR"));
	if (gra_getregistry(HKEY_LOCAL_MACHINE, subkey, value, tmp) == 0)
	{
		if (tmp[0] != 0)
		{
			estrcpy(testfile, tmp);
			estrcat(testfile, x_("cadrc"));
			io = efopen(testfile, x_("r"));
			if (io != 0)
			{
				/* valid "cadrc" file in directory, use it */
				fclose(io);
				(void)reallocstring(&el_libdir, tmp, db_cluster);
			} else
			{
				/* key points to bogus directory: delete it */
				(void)gra_setregistry(HKEY_LOCAL_MACHINE, subkey, value, x_(""));
			}
		}
	}

	/*
	 * linking an extension to an application is described in the article:
	 * "Fusing Your Applications to the System Through the Windows95 Shell"
	 */

	/* see if the ".elib" extension is defined */
	estrcpy(subkey, x_(".elib"));
	if (gra_getregistry(HKEY_CLASSES_ROOT, subkey, x_(""), elibname) == 0)
	{
		/* determine location of application */
		GetModuleFileName(0, applocation, 256);

		/* see what the registry thinks is the application location */
		doupdate = 1;
		esnprintf(subkey, 256, x_("%s\\shell\\open\\command\\"), elibname);
		if (gra_getregistry(HKEY_CLASSES_ROOT, subkey, x_(""), tmp) == 0)
		{
			if (estrcmp(applocation, tmp) == 0) doupdate = 0;
		}

		/* if the application location must be updated, do it now */
		if (doupdate != 0)
		{
			if (gra_setregistry(HKEY_CLASSES_ROOT, subkey, x_(""), applocation) != 0)
				ttyputerr(_("Error setting key"));
		}
	}

	/* set the machine name */
	nextchangequiet();
	(void)setval((INTBIG)us_tool, VTOOL, x_("USER_machine"),
		(INTBIG)x_("Windows"), VSTRING|VDONTSAVE);
}

/*
 * Routine to change the global "el_libdir" as well as the registry.
 */
void setlibdir(CHAR *libdir)
{
	CHAR *pp, tmp[256], subkey[256], value[256];
	BOOLEAN doupdate;
	REGISTER void *infstr;

	/* make sure UNIX '/' isn't in use */
	for(pp = libdir; *pp != 0; pp++)
		if (*pp == '/') *pp = DIRSEP;

	infstr = initinfstr();
	libdir = efullpath(tmp, libdir, 256);
	addstringtoinfstr(infstr, libdir);
	if (libdir[estrlen(libdir)-1] != DIRSEP) addtoinfstr(infstr, DIRSEP);
	(void)reallocstring(&el_libdir, returninfstr(infstr), db_cluster);

	/* see what the current registry key says */
	doupdate = TRUE;
	estrcpy(subkey, x_("Software\\Static Free Software\\Electric"));
	estrcpy(value, x_("ELECTRIC_LIBDIR"));
	if (gra_getregistry(HKEY_LOCAL_MACHINE, subkey, value, tmp) == 0)
	{
		/* if the current registry entry is correct, do not update */
		if (estrcmp(el_libdir, tmp) == 0) doupdate = FALSE;
	}

	/* if the key must be updated, do it now */
	if (doupdate)
	{
		if (gra_setregistry(HKEY_LOCAL_MACHINE, subkey, value, el_libdir) != 0)
			ttyputerr(_("Error setting key"));
	}
}

/*
 * Routine to querry key "key", subkey "subkey", value "value" of the registry and
 * return the value found in "result".  Returns zero if successful, nonzero if the value
 * was not found in the registry.
 */
INTBIG gra_getregistry(HKEY key, CHAR *subkey, CHAR *value, CHAR *result)
{
	HKEY hKey;
	DWORD size;
	INTBIG retval, err;

	if (RegOpenKeyEx(key, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return(1);
	size = 256;
	err = RegQueryValueEx(hKey, value, NULL, NULL, (UCHAR1 *)result, &size);
	if (err == ERROR_SUCCESS) retval = 0; else
 		retval = 1;
	RegCloseKey(hKey);
	return(retval);
}

/*
 * Routine to set key "key", subkey "subkey", value "value" of the registry to
 * the string in "newstring".  Returns zero if successful, nonzero if the registry
 * was not changed.
 */
INTBIG gra_setregistry(HKEY key, CHAR *subkey, CHAR *value, CHAR *newstring)
{
	HKEY hKey;
	INTBIG err;

	if (RegCreateKey(key, subkey, &hKey) != ERROR_SUCCESS) return(1);
	err = RegSetValueEx(hKey, value, 0, REG_SZ, (UCHAR1 *)newstring,
		estrlen(newstring)*SIZEOFCHAR);
	RegCloseKey(hKey);
	if (err != 0) return(1);
	return(0);
}

/*
 * Routine to force the module to return with some event so that the user interface will
 * relinquish control and let a slice happen.
 */
void forceslice(void)
{
}

/******************** MESSAGES WINDOW ROUTINES ********************/

/*
 * Routine to put the string "s" into the messages window.
 * Pops up the messages window if "important" is true.
 */
void putmessagesstring(CHAR *s, BOOLEAN important)
{
	REGISTER INTBIG line, pos1, pos2;

	/* ensure window exists, is noniconic, and is on top */
	if (important)
	{
		if (gra_messageswindow == 0)
		   (void)gra_makemessageswindow();
		if (gra_messageswindow->IsIconic())
			gra_messageswindow->ShowWindow(SW_RESTORE);
		if (gra_messagesnotvisible())
			gra_messageswindow->BringWindowToTop();
	}

	/* get next line and load it up */
	if (gra_messageswindow == 0) return;

	/* if messages buffer is running out of space, clear from the top */
	line = gra_editCtrl->GetLineCount();
	pos1 = gra_editCtrl->LineIndex(line-1);
	pos2 = gra_editCtrl->GetLimitText();
	if (pos2-pos1 < 2000)
	{
		gra_editCtrl->SetSel(0, 2000);
		gra_editCtrl->ReplaceSel(x_("..."));
	}

	gra_tomessagesbottom();
	gra_editCtrl->ReplaceSel(s);
	gra_editCtrl->ReplaceSel(x_("\r\n"));
}

void gra_tomessagesbottom(void)
{
	int pos;

	pos = gra_editCtrl->GetLimitText();
	gra_editCtrl->SetSel(pos, pos);
}

/*
 * Routine to return the name of the key that ends a session from the messages window.
 */
CHAR *getmessageseofkey(void)
{
	return(_("ESC"));
}

/*
 * Routine to get a string from the scrolling messages window.  Returns zero if end-of-file
 * (^D) is typed.
 */
CHAR *getmessagesstring(CHAR *prompt)
{
	long line, pos, startpos, startposonline, charcount, pos2, i;
	CHAR cTmpCh, *pt, ch[2];
	static CHAR typedline[MAXTYPEDLINE];

	/* ensure window exists, is noniconic, and is on top */
	if (gra_messageswindow == 0)
	   (void)gra_makemessageswindow();
	if (gra_messageswindow->IsIconic())
		gra_messageswindow->ShowWindow(SW_RESTORE);
	if (gra_messagesnotvisible())
		gra_messageswindow->BringWindowToTop();
	flushscreen();

	/* advance to next line and allocate maximum number of characters */
	gra_messageswindow->ActivateFrame();
	gra_tomessagesbottom();
	gra_editCtrl->ReplaceSel(prompt);
	gra_editCtrl->GetSel(startpos, pos2);
	line = gra_editCtrl->GetLineCount() - 1;
	startposonline = estrlen(prompt);
	for(;;)
	{
		/* continue to force the messages window up */
		if (gra_messageswindow == 0)
		   (void)gra_makemessageswindow();
		if (gra_messageswindow->IsIconic())
			gra_messageswindow->ShowWindow(SW_RESTORE);

		gra_nextevent();
		if (gra_inputstate == NOEVENT) continue;
		if ((gra_inputstate&ISKEYSTROKE) != 0 && gra_inputspecial == 0)
		{
			cTmpCh = (CHAR)(gra_inputstate & CHARREAD);
			if (cTmpCh == CTRLDKEY || cTmpCh == ESCKEY)
			{
				/* ESC (eof) */
				gra_editCtrl->GetSel(pos, pos2);
				if (startpos == pos)
				{
					gra_editCtrl->ReplaceSel(x_("\r\n"));
					gra_inputstate = NOEVENT;
					return(NULL);
				}
				cTmpCh = 0x0D;
			}
			if (cTmpCh == BACKSPACEKEY)
			{
				/* backspace */
				gra_editCtrl->GetSel(pos, pos2);
				if (pos <= startpos) continue;
				gra_editCtrl->SetSel(pos-1, pos);
				ch[0] = 0;
				gra_editCtrl->ReplaceSel(ch);
				continue;
			}
			if (cTmpCh == 0x0A || cTmpCh == 0x0D)
			{
				/* end of line */
				charcount = gra_editCtrl->GetLine(line, typedline, MAXTYPEDLINE);
				typedline[charcount] = 0;
				for(i=startposonline; typedline[i] != 0; i++)
					if (typedline[i] == '\r' || typedline[i] == '\n')
				{
					typedline[i] = 0;
					break;
				}
				pt = &typedline[startposonline];
				gra_inputstate = NOEVENT;
				return(pt);
			}
			gra_tomessagesbottom();
			ch[0] = cTmpCh;   ch[1] = 0;
			gra_editCtrl->ReplaceSel(ch);
		}
		gra_inputstate = NOEVENT;
	}
}

/*
 * routine to select fonts in the messages window
 */
void setmessagesfont(void)
{
	CFont *fnt;
	CFontDialog dlg;
	LOGFONT lf;

	if (dlg.DoModal() == IDOK)
	{
		/* Retrieve the dialog data */
		dlg.GetCurrentFont(&lf);
		fnt = new CFont();
		fnt->CreateFontIndirect(&lf);
		gra_editCtrl->SetFont(fnt);
	}
}

/*
 * routine to cut text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN cutfrommessages(void)
{
	if (gra_messageswindow != 0)
	{
		if (!gra_messageswindow->IsIconic())
		{
			CMDIFrameWnd *parent = gra_messageswindow->GetMDIFrame();
			CWnd *wnd = parent->MDIGetActive();
			if (gra_messageswindow == wnd)
			{
				/* do the cut */
				gra_editCtrl->Cut();
				return(TRUE);
			}
		}
	}
	if (el_curwindowpart == NOWINDOWPART) return(FALSE);
	if ((el_curwindowpart->state&WINDOWTYPE) == DISPWINDOW)
		gra_copyhighlightedtoclipboard();
	return(FALSE);
}

/*
 * routine to copy text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN copyfrommessages(void)
{
	if (gra_messageswindow != 0)
	{
		if (!gra_messageswindow->IsIconic())
		{
			CMDIFrameWnd *parent = gra_messageswindow->GetMDIFrame();
			CWnd *wnd = parent->MDIGetActive();
			if (gra_messageswindow == wnd)
			{
				/* do the copy */
				gra_editCtrl->Copy();
				return(TRUE);
			}
		}
	}
	if (el_curwindowpart == NOWINDOWPART) return(FALSE);
	if ((el_curwindowpart->state&WINDOWTYPE) == DISPWINDOW)
		gra_copyhighlightedtoclipboard();
	return(FALSE);
}

/*
 * Routine to build the bigger offscreen buffer for copying at higher resolution.
 */
BOOLEAN gra_getbiggeroffscreenbuffer(WINDOWFRAME *wf, INTBIG wid, INTBIG hei)
{
	/* see if text buffer needs to be (re)initialized */
	if (gra_biggeroffbufinited)
	{
		if (wid > gra_biggeroffbufwid || hei > gra_biggeroffbufhei)
		{
			efree((CHAR *)gra_biggeroffrowstart);
			efree((CHAR *)gra_biggeroffbitmapinfo);
			DeleteObject(gra_biggeroffbitmap);
			delete gra_biggeroffhdc;
			gra_biggeroffbufinited = FALSE;
		}
	}

	/* allocate text buffer if needed */
	if (!gra_biggeroffbufinited)
	{
		if (gra_buildoffscreenbuffer(wf, wid, hei, &gra_biggeroffhdc, &gra_biggeroffbitmap,
			&gra_biggeroffbitmapinfo, &gra_biggeroffdatabuffer, &gra_biggeroffrowstart)) return(TRUE);

		/* remember information about text buffer */
		gra_biggeroffbufwid = wid;   gra_biggeroffbufhei = hei;
		gra_biggeroffbufinited = TRUE;
	}
	return(FALSE);
}

/*
 * routine to copy the highlighted graphics to the clipboard.
 */
void gra_copyhighlightedtoclipboard(void)
{
	REGISTER INTBIG wid, hei, *dest, x, y, r, g, b, index, hsize, dsize, rowbytes,
		*redmap, *greenmap, *bluemap, savewid, savehei, bigwid, bighei, resfactor;
	INTBIG lx, hx, ly, hy;
	REGISTER UCHAR1 *source, **saverowstart, *savedata;
	REGISTER UCHAR1 *data, *store;
	REGISTER WINDOWFRAME *wf;
	REGISTER VARIABLE *varred, *vargreen, *varblue, *var;
	HGLOBAL h;
	BITMAPINFOHEADER *bmiHeader;
	BITMAPINFO *savebmiinfo;
	HBITMAP savebitmap;
	BOOLEAN savedirty;
	CDC *saveoff;

	/* determine the size of the area to copy */
	if (us_getareabounds(&lx, &hx, &ly, &hy) == NONODEPROTO) return;

	/* get the copy resolution scale factor */
	var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_print_resolution_scale"));
	if (var == NOVARIABLE) resfactor = 1; else
		resfactor = var->addr;

	/* make the window bigger */
	wf = el_curwindowpart->frame;
	bigwid = wf->swid * resfactor;
	bighei = wf->shei * resfactor;
	if (gra_getbiggeroffscreenbuffer(wf, bigwid, bighei)) return;
	saveoff = (CDC *)wf->hDCOff;     wf->hDCOff = (void *)gra_biggeroffhdc;
	savebitmap = wf->hBitmap;        wf->hBitmap = gra_biggeroffbitmap;
	savebmiinfo = wf->bminfo;        wf->bminfo = gra_biggeroffbitmapinfo;
	savedata = wf->data;             wf->data = gra_biggeroffdatabuffer;
	saverowstart = wf->rowstart;     wf->rowstart = gra_biggeroffrowstart;
	savewid = wf->swid;              wf->swid = bigwid;
	savehei = wf->shei;              wf->shei = bighei;
	savedirty = wf->offscreendirty;  wf->offscreendirty = FALSE;
	wf->revy = wf->shei - 1;
	if (wf->hPalette != 0)
	{
		(void)((CDC *)wf->hDCOff)->SelectPalette((CPalette *)wf->hPalette, TRUE);
		(void)((CDC *)wf->hDCOff)->RealizePalette();
	}
	el_curwindowpart->uselx *= resfactor;
	el_curwindowpart->usehx *= resfactor;
	el_curwindowpart->usely *= resfactor;
	el_curwindowpart->usehy *= resfactor;
	computewindowscale(el_curwindowpart);

	/* redraw the window in the larger buffer */
	gra_noflush = TRUE;
	us_redisplaynow(el_curwindowpart, TRUE);
	us_endchanges(NOWINDOWPART);
	gra_noflush = FALSE;

	/* determine the size of the part to copy */
	(void)us_makescreen(&lx, &ly, &hx, &hy, el_curwindowpart);

	/* restore the normal state of the window */
	wf->hDCOff = saveoff;
	wf->hBitmap = savebitmap;
	wf->bminfo = savebmiinfo;
	wf->data = savedata;
	wf->rowstart = saverowstart;
	wf->swid = savewid;
	wf->shei = savehei;
	wf->revy = wf->shei - 1;
	wf->offscreendirty = savedirty;
	if (wf->hPalette != 0)
	{
		(void)((CDC *)wf->hDCOff)->SelectPalette((CPalette *)wf->hPalette, TRUE);
		(void)((CDC *)wf->hDCOff)->RealizePalette();
	}
	el_curwindowpart->uselx /= resfactor;
	el_curwindowpart->usehx /= resfactor;
	el_curwindowpart->usely /= resfactor;
	el_curwindowpart->usehy /= resfactor;
	computewindowscale(el_curwindowpart);

	/* get information about the area to be copied */
	wid = hx - lx + 1;
	hei = hy - ly + 1;
	rowbytes = wid*4;
	dsize = rowbytes * hei;

	/* get colormap information */
	varred   = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue  = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred == NOVARIABLE || vargreen == NOVARIABLE || varblue == NOVARIABLE) return;
	redmap = (INTBIG *)varred->addr;
	greenmap = (INTBIG *)vargreen->addr;
	bluemap = (INTBIG *)varblue->addr;

	hsize = sizeof (BITMAPINFOHEADER);
	hsize += 12;		/* why is this needed? */
	h = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE|GMEM_ZEROINIT, hsize + dsize);
	if (h == 0) return;
	store = *((UCHAR1 **)h);
	bmiHeader = (BITMAPINFOHEADER *)store;
	bmiHeader->biSize = (DWORD)sizeof(BITMAPINFOHEADER);
	bmiHeader->biWidth = wid;
	bmiHeader->biHeight = hei;
	bmiHeader->biPlanes = 1;
	bmiHeader->biBitCount = 32;
	bmiHeader->biCompression = (DWORD)BI_RGB;
	data = &store[hsize];

	/* load the image data */
	for(y=0; y<hei; y++)
	{
		dest = (INTBIG *)(&data[y * rowbytes]);
		source = &gra_biggeroffrowstart[bighei-1-(y+ly)][lx];
		for(x=0; x<wid; x++)
		{
			index = ((*source++) & ~HIGHLIT) & 0xFF;
			r = redmap[index];
			g = greenmap[index];
			b = bluemap[index];
			*dest++ = b | (g << 8) | (r << 16);
		}
	}

	/* put the image into the clipboard */
	if (OpenClipboard(0) != 0)
	{
		EmptyClipboard();
		SetClipboardData(CF_DIB, h);
		CloseClipboard();
	}
}

/*
 * routine to paste text to the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN pastetomessages(void)
{
	if (gra_messageswindow == 0) return(FALSE);
	if (gra_messageswindow->IsIconic()) return(FALSE);
	CMDIFrameWnd *parent = gra_messageswindow->GetMDIFrame();
	CWnd *wnd = parent->MDIGetActive();
	if (gra_messageswindow != wnd) return(FALSE);

	/* do the copy */
	gra_editCtrl->Paste();
	return(TRUE);
}

/*
 * routine to get the contents of the system cut buffer
 */
CHAR *getcutbuffer(void)
{
	CHAR *ptr;
	REGISTER void *infstr;

	if (OpenClipboard(NULL) == 0) return(x_(""));
	ptr = (CHAR *)GetClipboardData(CF_TEXT);
	infstr = initinfstr();
	if (ptr != 0)
		addstringtoinfstr(infstr, ptr);
	CloseClipboard();
	return(returninfstr(infstr));
}

/*
 * routine to set the contents of the system cut buffer to "msg"
 */
void setcutbuffer(CHAR *msg)
{
	HGLOBAL hnd;

	if (OpenClipboard(NULL) == 0) return;

	/* Remove the current Clipboard contents */
	if (EmptyClipboard() == 0)
	{
		CloseClipboard();
		return;
	}

	hnd = GlobalAlloc(GHND, estrlen(msg)+SIZEOFCHAR);
	estrcpy(*((CHAR **)hnd), msg);
	(void)SetClipboardData(CF_TEXT, hnd);

	CloseClipboard();
}

/*
 * Routine to return true if the messages window is obscured by
 * an editor window.  Should ignore edit windows below this!!!
 */
BOOLEAN gra_messagesnotvisible(void)
{
	RECT mr, r;
	CWnd *wnd;
	WINDOWFRAME *wf;

	gra_messageswindow->GetWindowRect(&mr);
	mr.top += 2;   mr.bottom -= 2;
	mr.left += 2;  mr.right -= 2;

	/* look for "previous" windows that may obscure this one */
	wnd = gra_messageswindow;
	for(;;)
	{
		wnd = wnd->GetNextWindow(GW_HWNDPREV);
		if (wnd == 0) break;

		/* ignore palette */
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
			if ((CChildFrame *)wf->wndframe == wnd) break;
		if (wf != NOWINDOWFRAME && wf->floating) continue;
		wnd->GetWindowRect(&r);
		if (r.left < mr.right && r.right > mr.left &&
			r.top < mr.bottom && r.bottom > mr.top) return(TRUE);
	}
	return(FALSE);
}

/*
 * Routine to clear all text from the messages window.
 */
void clearmessageswindow(void)
{
	CHAR ch[1];
	int pos;

	if (gra_messageswindow == 0) return;
	pos = gra_editCtrl->GetLimitText();
	gra_editCtrl->SetSel(0, pos);
	ch[0] = 0;
	gra_editCtrl->ReplaceSel(ch);
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
 * Routine to free status field object "sf".
 */
void ttyfreestatusfield(STATUSFIELD *sf)
{
	INTBIG i, j;

	for(i=0; i<gra_indicatorcount; i++)
	{
		if (gra_statusfields[i] != sf) continue;

		/* remove the field */
		efree(gra_statusfieldtext[i]);
		for(j=i+1; j<gra_indicatorcount; j++)
		{
			gra_statusfields[j-1] = gra_statusfields[j];
			gra_statusfieldtext[j-1] = gra_statusfieldtext[j];
		}
		gra_indicatorcount--;
		gra_redrawstatusindicators();
		break;
	}

	efree(sf->label);
	efree((CHAR *)sf);
}

/*
 * Routine to display "message" in the status "field" of window "wf" (uses all windows
 * if "wf" is zero).  If "cancrop" is false, field cannot be cropped and should be
 * replaced with "*" if there isn't room.
 */
void ttysetstatusfield(WINDOWFRAME *wf, STATUSFIELD *field, CHAR *message, BOOLEAN cancrop)
{
	INTBIG len, i, j, pos, newpos;
	CMainFrame *wnd;

	/* figure out which indicator this is */
	if (field == 0) return;

	/* construct the status line */
	len = estrlen(field->label);
	if (len + estrlen(message) >= MAXLOCALSTRING)
	{
		estrcpy(gra_localstring, field->label);
		i = MAXLOCALSTRING - len - 1;
		estrncat(gra_localstring, message, i);
		gra_localstring[MAXLOCALSTRING-1] = 0;
	} else
	{
		esnprintf(gra_localstring, 256, x_("%s%s"), field->label, message);
	}
	len = estrlen(gra_localstring);
	while (len > 0 && gra_localstring[len-1] == ' ') gra_localstring[--len] = 0;

	/* if this is a title bar setting, do it now */
	if (field->line == 0)
	{
		if (wf == NOWINDOWFRAME) return;
		if (*gra_localstring == 0) estrcpy(gra_localstring, _("*** NO CELL ***"));
		((CChildFrame *)wf->wndframe)->SetWindowText(gra_localstring);
		return;
	}

	/* ignore null fields */
	if (*field->label == 0) return;

	/* see if this indicator is in the list */
	wnd = (CMainFrame *)AfxGetMainWnd();
	for(i=0; i<gra_indicatorcount; i++)
	{
		if (gra_statusfields[i]->line != field->line ||
			gra_statusfields[i]->startper != field->startper ||
			gra_statusfields[i]->endper != field->endper) continue;

		/* load the string */
		(void)reallocstring(&gra_statusfieldtext[i], gra_localstring, db_cluster);
		wnd->m_wndStatusBar.SetPaneText(i+1, gra_localstring);
		return;
	}

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* not found: find where this field fits in the order */
	newpos = (field->line-1) * 100 + field->startper;
	for(i=0; i<gra_indicatorcount; i++)
	{
		pos = (gra_statusfields[i]->line-1) * 100 + gra_statusfields[i]->startper;
		if (newpos < pos) break;
	}
	for(j=gra_indicatorcount; j>i; j--)
	{
		gra_statusfieldtext[j] = gra_statusfieldtext[j-1];
		gra_statusfields[j] = gra_statusfields[j-1];
	}
	gra_statusfields[i] = field;
	(void)allocstring(&gra_statusfieldtext[i], gra_localstring, db_cluster);
	gra_indicatorcount++;

	/* reload all indicators */
	gra_redrawstatusindicators();
}

void gra_redrawstatusindicators(void)
{
	INTBIG i, wid, screenwid, totalwid;
	RECT r;
	CMainFrame *wnd;

	wnd = (CMainFrame *)AfxGetMainWnd();
	wnd->GetClientRect(&r);
	screenwid = r.right - r.left - gra_indicatorcount * 5;
	wnd->m_wndStatusBar.SetIndicators(gra_indicators, gra_indicatorcount+1);
	totalwid = 0;
	for(i=0; i<gra_indicatorcount; i++)
		totalwid += gra_statusfields[i]->endper - gra_statusfields[i]->startper;
	for(i=0; i<=gra_indicatorcount; i++)
	{
		if (i == 0)
		{
			wnd->m_wndStatusBar.SetPaneInfo(i, gra_indicators[i], SBPS_NOBORDERS, 0);
			wnd->m_wndStatusBar.SetPaneText(i, x_(""));
		} else
		{
			wid = gra_statusfields[i-1]->endper - gra_statusfields[i-1]->startper;
			wid = wid * screenwid / totalwid;
			wnd->m_wndStatusBar.SetPaneInfo(i, gra_indicators[i], SBPS_NORMAL, wid);
			wnd->m_wndStatusBar.SetPaneText(i, gra_statusfieldtext[i-1]);
		}
	}
}

/******************** GRAPHICS CONTROL ROUTINES ********************/

void flushscreen(void)
{
	WINDOWFRAME *wf;

	if (gra_noflush) return;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->wndframe == 0) continue;

		/* if screen has not changed, stop now */
		if (!wf->offscreendirty) continue;
		wf->offscreendirty = FALSE;

		/* refresh screen */
		wf->hDC = (CDC *)((CChildFrame *)wf->wndframe)->GetDC();
		((CDC *)wf->hDC)->BitBlt(wf->copyleft, wf->copytop, wf->copyright-wf->copyleft,
			   wf->copybottom-wf->copytop, (CDC *)wf->hDCOff, wf->copyleft, wf->copytop, SRCCOPY);
		((CChildFrame *)wf->wndframe)->ReleaseDC((CDC *)wf->hDC);
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
		if (lx < wf->copyleft)   wf->copyleft = lx;
		if (hx > wf->copyright)  wf->copyright = hx;
		if (ly < wf->copytop)    wf->copytop = ly;
		if (hy > wf->copybottom) wf->copybottom = hy;
	} else
	{
		wf->copyleft = lx;
		wf->copyright = hx;
		wf->copytop = ly;
		wf->copybottom = hy;
		wf->offscreendirty = TRUE;
		wf->starttime = ticktime();
	}

	/* flush the screen every two seconds */
	checktimefreq++;
	if (checktimefreq > 10000)
	{
		checktimefreq = 0;
		if (ticktime() - wf->starttime > FLUSHTICKS) flushscreen();
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
	INTBIG forecolor, backcolor;
	REGISTER WINDOWFRAME *wf;
	RGBQUAD bmiColors[256];

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		/* do not touch the color map when in B&W mode */
		if (wf->pColorPalette == 0) continue;

		i = GetDIBColorTable(((CDC *)wf->hDCOff)->m_hDC, 0, 256, bmiColors);
		for(i=low; i<=high; i++)
		{
			bmiColors[i].rgbRed = (UCHAR1)red[i-low];
			bmiColors[i].rgbGreen = (UCHAR1)green[i-low];
			bmiColors[i].rgbBlue = (UCHAR1)blue[i-low];
			bmiColors[i].rgbReserved = 0;
			if (i == 255)
			{
				bmiColors[i].rgbRed = (UCHAR1)(255-red[i-low]);
				bmiColors[i].rgbGreen = (UCHAR1)(255-green[i-low]);
				bmiColors[i].rgbBlue = (UCHAR1)(255-blue[i-low]);
			}

			wf->pColorPalette->palPalEntry[i].peRed = bmiColors[i].rgbRed;
			wf->pColorPalette->palPalEntry[i].peGreen = bmiColors[i].rgbGreen;
			wf->pColorPalette->palPalEntry[i].peBlue = bmiColors[i].rgbBlue;
			wf->pColorPalette->palPalEntry[i].peFlags = 0;
		}
		i = SetDIBColorTable(((CDC *)wf->hDCOff)->m_hDC, 0, 256, bmiColors);

		if (wf->hPalette != 0)
			delete (CPalette *)wf->hPalette;

		wf->hPalette = (CPalette *)new CPalette();
		((CPalette *)wf->hPalette)->CreatePalette(wf->pColorPalette);

		/* graphics window */
		((CDC *)wf->hDCOff)->SelectPalette((CPalette *)wf->hPalette, TRUE);
		((CDC *)wf->hDCOff)->RealizePalette();
		gra_setrect(wf, 0, wf->swid-1, 0, wf->shei-1);
	}

	/* change the messages window to conform */
	if (low == 0 && high == 255 && gra_messageswindow != 0)
	{
		forecolor = (blue[CELLTXT] << 16) | (green[CELLTXT] << 8) | red[CELLTXT];
		backcolor = (blue[0] << 16) | (green[0] << 8) | red[0];
		CElectricMsgView *msgView = (CElectricMsgView *)gra_messageswindow;
		msgView->SetColors((COLORREF)forecolor, (COLORREF)backcolor);
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
		case NORMALCURSOR:  SetCursor(gra_normalCurs);   break;
		case WANTTTYCURSOR: SetCursor(gra_wantttyCurs);  break;
		case PENCURSOR:     SetCursor(gra_penCurs);      break;
		case NULLCURSOR:    SetCursor(gra_normalCurs);   break;
		case MENUCURSOR:    SetCursor(gra_menuCurs);     break;
		case HANDCURSOR:    SetCursor(gra_handCurs);     break;
		case TECHCURSOR:    SetCursor(gra_techCurs);     break;
		case IBEAMCURSOR:   SetCursor(gra_ibeamCurs);    break;
		case WAITCURSOR:    SetCursor(gra_waitCurs);     break;
		case LRCURSOR:      SetCursor(gra_lrCurs);       break;
		case UDCURSOR:      SetCursor(gra_udCurs);       break;
	}
	ShowCursor(TRUE);
	us_cursorstate = state;
}

/*
 * Routine to change the default cursor (to indicate modes).
 */
void setnormalcursor(INTBIG curs)
{
	us_normalcursor = curs;
	setdefaultcursortype(us_normalcursor);
}

/******************** GRAPHICS LINES ********************/

void screendrawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc, INTBIG texture)
{
	gra_drawline(win, x1, y1, x2, y2, desc, texture);
}

void screeninvertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	gra_invertline(win, x1, y1, x2, y2);
}

/******************** GRAPHICS POLYGONS ********************/

void screendrawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc)
{
	gra_drawpolygon(win, x, y, count, desc);
}

/******************** GRAPHICS BOXES ********************/

void screendrawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy, GRAPHICS *desc)
{
	gra_drawbox(win, lowx, highx, lowy, highy, desc);
}

/*
 * routine to invert the bits in the box from (lowx, lowy) to (highx, highy)
 */
void screeninvertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy)
{
	gra_invertbox(win, lowx, highx, lowy, highy);
}

/*
 * routine to move bits on the display starting with the area at
 * (sx,sy) and ending at (dx,dy).  The size of the area to be
 * moved is "wid" by "hei".
 */
void screenmovebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei, INTBIG dx, INTBIG dy)
{
#ifdef WINSAVEDBOX
	REGISTER INTBIG totop, tobottom, toleft, toright, fromleft, fromtop;
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;

	/* setup source rectangle */
	fromleft = sx;
	fromtop = wf->revy + 1 - sy - hei;

	/* setup destination rectangle */
	toleft = dx;
	toright = toleft + wid;
	totop = wf->revy + 1 - dy - hei;
	tobottom = totop + hei;

	((CDC *)wf->hDCOff)->BitBlt(toleft, totop, wid, hei, (CDC *)wf->hDCOff,
		fromleft, fromtop, SRCCOPY);

	gra_setrect(wf, toleft, toright, totop, tobottom);
#else
	gra_movebox(win, sx, sy, wid, hei, dx, dy);
#endif
}

/*
 * routine to save the contents of the box from "lx" to "hx" in X and from
 * "ly" to "hy" in Y.  A code is returned that identifies this box for
 * overwriting and restoring.  The routine returns -1 if there is a error.
 */
INTBIG screensavebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
#ifdef WINSAVEDBOX
	SAVEDBOX *box;
	REGISTER INTBIG xsize, ysize;
	REGISTER INTBIG i, x, y;
	REGISTER WINDOWFRAME *wf;
	REGISTER UCHAR1 *source, *dest;

	wf = win->frame;
	i = ly;
	ly = wf->revy-hy;
	hy = wf->revy-i;
	xsize = hx-lx+1;
	ysize = hy-ly+1;
	box = (SAVEDBOX *)emalloc((sizeof (SAVEDBOX)), us_tool->cluster);
	if (box == 0) return(-1);

	if (gra_buildoffscreenbuffer(wf, xsize, ysize, &box->hMemDC,
		&box->hBox, &box->bminfo, &box->data, &box->rowstart)) return(-1);

	box->win = win;
	box->nextsavedbox = gra_firstsavedbox;
	gra_firstsavedbox = box;
	box->lx = lx;           box->hx = hx;
	box->ly = ly;           box->hy = hy;

	/* move the bits */
	for(y=0; y<ysize; y++)
	{
		source = &wf->rowstart[ly+y][lx];
		dest = box->rowstart[y];
		for(x=0; x<xsize; x++) *dest++ = *source++;
	}

	return((INTBIG)box);
#else
	return(gra_savebox(win, lx, hx, ly, hy));
#endif
}

/*
 * routine to shift the saved box "code" so that it is restored in a different location,
 * offset by (dx,dy)
 */
void screenmovesavedbox(INTBIG code, INTBIG dx, INTBIG dy)
{
#ifdef WINSAVEDBOX
	REGISTER SAVEDBOX *box;

	if (code == -1) return;
	box = (SAVEDBOX *)code;
	box->lx += dx;       box->hx += dx;
	box->ly -= dy;       box->hy -= dy;
#else
	gra_movesavedbox(code, dx, dy);
#endif
}

/*
 * routine to restore saved box "code" to the screen.  "destroy" is:
 *  0   restore box, do not free memory
 *  1   restore box, free memory
 * -1   free memory
 */
void screenrestorebox(INTBIG code, INTBIG destroy)
{
#ifdef WINSAVEDBOX
	REGISTER SAVEDBOX *box, *lbox, *tbox;
	REGISTER INTBIG x, y, xsize, ysize;
	REGISTER UCHAR1 *source, *dest;
	REGISTER WINDOWFRAME *wf;

	/* get the box */
	if (code == -1) return;
	box = (SAVEDBOX *)code;

	/* move the bits */
	if (destroy >= 0)
	{
		wf = box->win->frame;
		xsize = box->hx - box->lx + 1;
		ysize = box->hy - box->ly + 1;
		for(y=0; y<ysize; y++)
		{
			dest = &wf->rowstart[box->ly+y][box->lx];
			source = box->rowstart[y];
			for(x=0; x<xsize; x++) *dest++ = *source++;
		}
		efree((CHAR *)box->rowstart);
		efree((CHAR *)box->bminfo);
		DeleteObject(box->hBox);
		delete box->hMemDC;
		gra_setrect(wf, box->lx, box->hx+1, box->ly, box->hy+1);
	}

	/* destroy this box's memory if requested */
	if (destroy != 0)
	{
		lbox = NOSAVEDBOX;
		for(tbox = gra_firstsavedbox; tbox != NOSAVEDBOX; tbox = tbox->nextsavedbox)
		{
			if (tbox == box)
				break;
			lbox = tbox;
		}
		if (lbox == NOSAVEDBOX)
			gra_firstsavedbox = box->nextsavedbox;
		else
			lbox->nextsavedbox = box->nextsavedbox;
		efree((CHAR *)box);
	}
#else
	(void)gra_restorebox(code, destroy);
#endif
}

/******************** GRAPHICS TEXT ********************/

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
	CDC *curdc;
	CChildFrame *child;
	REGISTER INTBIG i;

	if (gra_numfaces == 0)
	{
		if (el_curwindowframe != NOWINDOWFRAME)
			child = (CChildFrame *)el_curwindowframe->wndframe; else
				child = gra_messageswindow;
		curdc = (CDC *)child->GetDC();
		EnumFonts(curdc->m_hDC, NULL, (FONTENUMPROC)gra_enumfaces, (LPARAM)NULL);
		if (gra_numfaces > VTMAXFACE)
		{
			ttyputerr(_("Warning: found %ld fonts, but can only keep track of %d"), gra_numfaces,
				VTMAXFACE);
			for(i=VTMAXFACE; i<gra_numfaces; i++)
			{
				efree((CHAR *)gra_facelist[i]);
				gra_facelist[i] = 0;
			}
			gra_numfaces = VTMAXFACE;
		}
		esort(&gra_facelist[1], gra_numfaces-1, sizeof (CHAR *), sort_stringascending);
	}
	*list = gra_facelist;
	return(gra_numfaces);
}

/*
 * Routine to return the default typeface used on the screen.
 */
CHAR *screengetdefaultfacename(void)
{
	return(x_("Arial"));
}

/*
 * Callback routine to get the number of fonts
 */
int APIENTRY gra_enumfaces(LPLOGFONT lpLogFont, LPTEXTMETRIC lpTEXTMETRICs, DWORD fFontType, LPVOID lpData)
{
	INTBIG newcellotal, i;
	CHAR **newfacelist;

	if (gra_numfaces == 0) newcellotal = 2; else
		newcellotal = gra_numfaces + 1;
	newfacelist = (CHAR **)emalloc(newcellotal * (sizeof (CHAR *)), us_tool->cluster);
	if (newfacelist == 0) return(0);
	for(i=0; i<gra_numfaces; i++)
		newfacelist[i] = gra_facelist[i];
	for(i=gra_numfaces; i<newcellotal; i++)
		newfacelist[i] = 0;
	if (gra_numfaces > 0) efree((CHAR *)gra_facelist);
	gra_facelist = newfacelist;
	if (gra_numfaces == 0)
		(void)allocstring(&gra_facelist[gra_numfaces++], _("DEFAULT FACE"), us_tool->cluster);
	if (gra_facelist[gra_numfaces] != 0) efree((CHAR *)gra_facelist[gra_numfaces]);
	(void)allocstring(&gra_facelist[gra_numfaces++], lpLogFont->lfFaceName, us_tool->cluster);
	return(1);
}

void screensettextinfo(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript)
{
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;
	wf->hFont = 0;
	wf->hFont = gra_gettextfont(win, tech, descript);
	if (wf->hFont == 0) return;
	((CDC *)wf->hDCOff)->SelectObject((CFont *)wf->hFont);
}

CFont *gra_gettextfont(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript)
{
	static CFont *txteditor = 0, *txtmenu = 0;
	CFont *theFont;
	REGISTER INTBIG font, size, italic, bold, underline, face, i;
	REGISTER UINTBIG hash;
	REGISTER CHAR *facename, **list;

	gra_texttoosmall = FALSE;
	font = TDGETSIZE(descript);
	if (font == TXTEDITOR)
	{
		if (txteditor == 0) txteditor = gra_createtextfont(12, x_("Lucida Console"), 0, 0, 0);
		return(txteditor);
	}
	if (font == TXTMENU)
	{
		if (txtmenu == 0)   txtmenu = gra_createtextfont(13, x_("MS Sans Serif"), 0, 0, 0);
		return(txtmenu);
	}
	size = truefontsize(font, win, tech);
	if (size < 1)
	{
		gra_texttoosmall = TRUE;
		return(0);
	}
	if (size < MINIMUMTEXTSIZE) size = MINIMUMTEXTSIZE;

	face = TDGETFACE(descript);
	italic = TDGETITALIC(descript);
	bold = TDGETBOLD(descript);
	underline = TDGETUNDERLINE(descript);
	gra_textrotation = TDGETROTATION(descript);
	if (face == 0 && italic == 0 && bold == 0 && underline == 0)
	{
		if (size >= MAXCACHEDFONTS) size = MAXCACHEDFONTS-1;
		if (gra_fontcache[size] == 0)
			gra_fontcache[size] = gra_createtextfont(size, x_("Arial"), 0, 0, 0);
		return(gra_fontcache[size]);
	} else
	{
		hash = size + 3*italic + 5*bold + 7*underline + 11*face;
		hash %= FONTHASHSIZE;
		for(i=0; i<FONTHASHSIZE; i++)
		{
			if (gra_fonthash[hash].font == 0) break;
			if (gra_fonthash[hash].face == face && gra_fonthash[hash].size == size &&
				gra_fonthash[hash].italic == italic && gra_fonthash[hash].bold == bold &&
				gra_fonthash[hash].underline == underline)
					return(gra_fonthash[hash].font);
			hash++;
			if (hash >= FONTHASHSIZE) hash = 0;
		}
		facename = x_("Arial");
		if (face > 0)
		{
			if (gra_numfaces == 0) (void)screengetfacelist(&list, FALSE);
			if (face < gra_numfaces)
				facename = gra_facelist[face];
		}
		theFont = gra_createtextfont(size, facename, italic, bold, underline);
		if (gra_fonthash[hash].font == 0)
		{
			gra_fonthash[hash].font = theFont;
			gra_fonthash[hash].face = face;
			gra_fonthash[hash].size = size;
			gra_fonthash[hash].italic = italic;
			gra_fonthash[hash].bold = bold;
			gra_fonthash[hash].underline = underline;
		}
		return(theFont);
	}
}

CFont *gra_createtextfont(INTBIG fontSize, CHAR *face, INTBIG italic, INTBIG bold,
	INTBIG underline)
{
	LOGFONT lf;
	CFont *hf;

	lf.lfHeight = -fontSize;
	estrcpy(lf.lfFaceName, face);
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	if (bold != 0) lf.lfWeight = FW_BOLD; else
		lf.lfWeight = FW_NORMAL;
	if (italic != 0) lf.lfItalic = 1; else
		lf.lfItalic = 0;
	if (underline != 0) lf.lfUnderline = 1; else
		lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_STROKE_PRECIS;
	lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	hf = new CFont();
	hf->CreateFontIndirect(&lf);
	return(hf);
}

void screengettextsize(WINDOWPART *win, CHAR *str, INTBIG *x, INTBIG *y)
{
	INTBIG len;
	CSize textSize;
	REGISTER WINDOWFRAME *wf;

	if (gra_texttoosmall)
	{
		*x = *y = 0;
		return;
	}
	wf = win->frame;
	len = estrlen(str);
	textSize = ((CDC *)wf->hDCOff)->GetTextExtent(str, len);
	switch (gra_textrotation)
	{
		case 0:			/* normal */
			*x = textSize.cx;
			*y = textSize.cy+1;
			break;
		case 1:			/* 90 degrees counterclockwise */
			*x = -textSize.cy+1;
			*y = textSize.cx;
			break;
		case 2:			/* 180 degrees */
			*x = -textSize.cx;
			*y = -textSize.cy+1;
			break;
		case 3:			/* 90 degrees clockwise */
			*x = textSize.cy+1;
			*y = -textSize.cx;
			break;
	}
}

void screendrawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, CHAR *s, GRAPHICS *desc)
{
	if (gra_texttoosmall) return;
	gra_drawtext(win, atx, aty, gra_textrotation, s, desc);
}

/*
 * Routine to convert the string "msg" (to be drawn into window "win") into an
 * array of pixels.  The size of the array is returned in "wid" and "hei", and
 * the pixels are returned in an array of character vectors "rowstart".
 * The routine returns true if the operation cannot be done.
 */
BOOLEAN gettextbits(WINDOWPART *win, CHAR *msg, INTBIG *wid, INTBIG *hei, UCHAR1 ***rowstart)
{
	REGISTER INTBIG i, len, j;
	CSize textSize;
	REGISTER WINDOWFRAME *wf;
	REGISTER UCHAR1 *ptr;

	/* copy the string and correct it */
	len = estrlen(msg);
	if (len == 0 || gra_texttoosmall)
	{
		*wid = *hei = 0;
		return(FALSE);
	}
	/* determine size of string */
	wf = win->frame;
	textSize = ((CDC *)wf->hDCOff)->GetTextExtent(msg, len);
	*wid = textSize.cx;
	*hei = textSize.cy;

	/* see if text buffer needs to be (re)initialized */
	if (gra_textbufinited)
	{
		if (*wid > gra_textbufwid || *hei > gra_textbufhei)
		{
			efree((CHAR *)gra_textrowstart);
			efree((CHAR *)gra_textbitmapinfo);
			DeleteObject(gra_textbitmap);
			delete gra_texthdc;
			gra_textbufinited = FALSE;
		}
	}

	/* allocate text buffer if needed */
	if (!gra_textbufinited)
	{
		if (gra_buildoffscreenbuffer(wf, *wid, *hei, &gra_texthdc, &gra_textbitmap,
			&gra_textbitmapinfo, &gra_textdatabuffer, &gra_textrowstart)) return(TRUE);
		gra_texthdc->SetTextColor(PALETTEINDEX(1));
		gra_texthdc->SetROP2(R2_COPYPEN);
		gra_texthdc->SetBkMode(TRANSPARENT);
		gra_texthdc->SetTextAlign(TA_TOP<<8);

		/* remember information about text buffer */
		gra_textbufwid = *wid;   gra_textbufhei = *hei;
		gra_textbufinited = TRUE;
	}

	/* clear the text buffer */
	for(i=0; i < *hei; i++)
	{
		ptr = gra_textrowstart[i];
		for(j=0; j < *wid; j++) *ptr++ = 0;
	}

	/* write to the text buffer */
	gra_texthdc->SelectObject((CFont *)wf->hFont);
	gra_texthdc->TextOut(0, 0, msg, len);

	*rowstart = gra_textrowstart;
	return(FALSE);
}

/******************** CIRCLE DRAWING ********************/

void screendrawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	gra_drawcircle(win, atx, aty, radius, desc);
}

void screendrawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius,
	GRAPHICS *desc)
{
	gra_drawthickcircle(win, atx, aty, radius, desc);
}

/******************** FILLED CIRCLE DRAWING ********************/

/*
 * routine to draw a filled-in circle of radius "radius"
 */
void screendrawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
	gra_drawdisc(win, atx, aty, radius, desc);
}

/******************** ARC DRAWING ********************/

/*
 * draws a thin arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1_x, INTBIG p1_y,
	INTBIG p2_x, INTBIG p2_y, GRAPHICS *desc)
{
	gra_drawcirclearc(win, centerx, centery, p1_x, p1_y, p2_x, p2_y, FALSE, desc);
}

/*
 * draws a thick arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawthickcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1_x, INTBIG p1_y,
	INTBIG p2_x, INTBIG p2_y, GRAPHICS *desc)
{
	gra_drawcirclearc(win, centerx, centery, p1_x, p1_y, p2_x, p2_y, TRUE, desc);
}

/******************** GRID CONTROL ********************/

/*
 * grid drawing routine
 */
void screendrawgrid(WINDOWPART *win, POLYGON *obj)
{
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
	if (b >= BUTTONS - REALBUTS) return(TRUE);
	return(FALSE);
}

/*
 * routine to tell whether button "but" is a context button (right)
 */
BOOLEAN contextbutton(INTBIG b)
{
	if ((b%5) == 2) return(TRUE);
	return(FALSE);
}

/*
 * routine to tell whether button "but" has the "shift" key held
 */
BOOLEAN shiftbutton(INTBIG b)
{
	b = b / REALBUTS;

	/* this "switch" statement is taken from the array "gra_buttonname" */
	switch (b)
	{
		case 0: return(FALSE);	/* no shift keys */
		case 1: return(TRUE);	/* shift */
		case 2: return(FALSE);	/* control */
		case 3: return(FALSE);	/* alt */
		case 4: return(TRUE);	/* shift-control */
		case 5: return(TRUE);	/* shift-alt */
		case 6: return(FALSE);	/* control-alt */
		case 7: return(TRUE);	/* shift-control-alt */
		case 8: return(FALSE);	/* double-click */
	}
	return(FALSE);
}

/*
 * routine to tell whether button "but" is a "mouse wheel" button
 */
BOOLEAN wheelbutton(INTBIG b)
{
	b = b % REALBUTS;
	if (b == 3 || b == 4) return(TRUE);
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
 * routine to convert from "state" (the typical input parameter)
 * to button numbers (the table "gra_buttonname")
 */
INTBIG gra_makebutton(INTBIG state)
{
	REGISTER INTBIG base;

	switch (state&WHICHBUTTON)
	{
		case ISLEFT:    base = 0;   break;
		case ISMIDDLE:  base = 1;   break;
		case ISRIGHT:   base = 2;   break;
		case ISWHLFWD:  base = 3;   break;
		case ISWHLBKW:  base = 4;   break;
	}

	if ((state&DOUBLECLICK) != 0) return(base+REALBUTS*8);
	switch (state & (SHIFTISDOWN|CONTROLISDOWN|ALTISDOWN))
	{
		case SHIFTISDOWN                        : return(base+REALBUTS*1);
		case             CONTROLISDOWN          : return(base+REALBUTS*2);
		case                           ALTISDOWN: return(base+REALBUTS*3);
		case SHIFTISDOWN|CONTROLISDOWN          : return(base+REALBUTS*4);
		case SHIFTISDOWN|              ALTISDOWN: return(base+REALBUTS*5);
		case             CONTROLISDOWN|ALTISDOWN: return(base+REALBUTS*6);
		case SHIFTISDOWN|CONTROLISDOWN|ALTISDOWN: return(base+REALBUTS*7);
	}
	return(base);
}

/*
 * routine to wait for a button push and return its index (0 based) in "*but".
 * The coordinates of the cursor are placed in "*x" and "*y".  If there is no
 * button push, the value of "*but" is negative.
 */
void waitforbutton(INTBIG *x, INTBIG *y, INTBIG *but)
{
	if (gra_inputstate != NOEVENT && (gra_inputstate&(ISBUTTON|BUTTONUP)) == ISBUTTON)
	{
		*but = gra_makebutton(gra_inputstate);
		*x = gra_cursorx;
		*y = gra_cursory;
		gra_inputstate = NOEVENT;
		if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
		return;
	}
	gra_nextevent();
	if (gra_inputstate != NOEVENT && (gra_inputstate&(ISBUTTON|BUTTONUP)) == ISBUTTON)
	{
		*but = gra_makebutton(gra_inputstate);
		*x = gra_cursorx;
		*y = gra_cursory;
		gra_inputstate = NOEVENT;
		if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
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
	INTBIG oldnormalcursor, special, x, y, but;
	INTSML chr;

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
void trackcursor(BOOLEAN waitforpush, BOOLEAN (*whileup)(INTBIG, INTBIG),
	void (*whendown)(void), BOOLEAN (*eachdown)(INTBIG, INTBIG),
	BOOLEAN (*eachchar)(INTBIG, INTBIG, INTSML), void (*done)(void), INTBIG purpose)
{
	REGISTER BOOLEAN keepon;
	INTBIG action, oldcursor;

	/* change the cursor to an appropriate icon */
	oldcursor = us_normalcursor;
	switch (purpose)
	{
		case TRACKDRAWING:
			setnormalcursor(PENCURSOR);
			break;
		case TRACKDRAGGING:
			setnormalcursor(HANDCURSOR);
			break;
		case TRACKSELECTING:
		case TRACKHSELECTING:
			setnormalcursor(MENUCURSOR);
			break;
	}

	/* now wait for a button to go down, if requested */
	keepon = FALSE;
	if (waitforpush)
	{
		while (!keepon)
		{
			gra_nextevent();
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
		gra_nextevent();
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
	setnormalcursor(oldcursor);
}

/*
 * routine to read the current co-ordinates of the tablet and return them
 * in "*x" and "*y".
 */
void readtablet(INTBIG *x, INTBIG *y)
{
	*x = gra_cursorx;
	*y = gra_cursory;
}

/*
 * routine to turn off the cursor tracking if it is on
 */
void stoptablet(void)
{
	if (us_cursorstate != IBEAMCURSOR)
		setdefaultcursortype(us_normalcursor);
}

/******************** KEYBOARD CONTROL ********************/

/*
 * routine to get the next character from the keyboard
 */
INTSML getnxtchar(INTBIG *special)
{
	REGISTER INTSML i;

	if (gra_inputstate != NOEVENT && (gra_inputstate&ISKEYSTROKE) != 0)
	{
		i = (INTSML)(gra_inputstate & CHARREAD);
		*special = gra_inputspecial;
		gra_inputstate = NOEVENT;
		return(i);
	}
	if (us_cursorstate != IBEAMCURSOR)
		setdefaultcursortype(WANTTTYCURSOR);
	for(;;)
	{
		gra_nextevent();
		if (gra_inputstate != NOEVENT && (gra_inputstate&ISKEYSTROKE) != 0)
			break;
	}
	i = (INTSML)(gra_inputstate & CHARREAD);
	*special = gra_inputspecial;
	gra_inputstate = NOEVENT;
	if (us_cursorstate != IBEAMCURSOR)
		setdefaultcursortype(us_normalcursor);
	return(i);
}

void checkforinterrupt(void)
{
	MSG msg;

	while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!gra_app.PumpMessage())
		{
			::PostQuitMessage(0);
			break;
		}
	}
	setdefaultcursortype(WAITCURSOR);
}

/*
 * Routine to return which "bucky bits" are held down (shift, control, etc.)
 */
INTBIG getbuckybits(void)
{
	REGISTER INTBIG bits;

	bits = 0;
	if ((GetKeyState(VK_CONTROL)&0x8000) != 0) bits |= CONTROLDOWN|ACCELERATORDOWN;
	if ((GetKeyState(VK_SHIFT)&0x8000) != 0) bits |= SHIFTDOWN;
	if ((GetKeyState(VK_MENU)&0x8000) != 0) bits |= OPTALTMETDOWN;
	return(bits);
}

/*
 * routine to tell whether data is waiting at the terminal.  Returns true
 * if data is ready.
 */
BOOLEAN ttydataready(void)
{
	/* see if something is already pending */
	if (gra_inputstate != NOEVENT)
	{
		if ((gra_inputstate&ISKEYSTROKE) != 0) return(TRUE);
	}

	/* wait for something and analyze it */
	gra_nextevent();
	if (gra_inputstate != NOEVENT && (gra_inputstate&ISKEYSTROKE) != 0) return(TRUE);
	return(FALSE);
}

/****************************** FILES ******************************/

/*
 * Routine to prompt for multiple files of type "filetype", giving the
 * message "msg".  Returns a string that contains all of the file names,
 * separated by the NONFILECH (a character that cannot be in a file name).
 */
#define MAXMULTIFILECHARS 4000
CHAR *multifileselectin(CHAR *msg, INTBIG filetype)
{
	CHAR fs[MAXMULTIFILECHARS], prompt[256], *extension, *winfilter, *shortname,
		*longname, *pt;
	CFileDialog *fileDlg;
	REGISTER INTBIG which;
	REGISTER INTBIG ret;
	INTBIG mactype;
	BOOLEAN binary;
	REGISTER void *infstr;

	/* build filter string */
	describefiletype(filetype, &extension, &winfilter, &mactype, &binary, &shortname, &longname);
	estrcpy(fs, longname);
	estrcat(fs, x_(" ("));
	estrcat(fs, winfilter);
	estrcat(fs, x_(")|"));
	estrcat(fs, winfilter);
	estrcat(fs, _("|All Files (*.*)|*.*||"));

	fileDlg = new CFileDialog(TRUE, NULL, NULL, NULL, fs, NULL);
	if (fileDlg == NULL) return(0);
	fs[0] = 0;
	fileDlg->m_ofn.lpstrFile = fs;
	fileDlg->m_ofn.nMaxFile = MAXMULTIFILECHARS;
	fileDlg->m_ofn.Flags |= OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT |
		OFN_EXPLORER;
	esnprintf(prompt, 256, _("%s Selection"), msg);
	fileDlg->m_ofn.lpstrTitle = prompt;
	pt = 0;
	ret = fileDlg->DoModal();
	if (ret == IDOK)
	{
		pt = fs;
		infstr = initinfstr();
		which = 0;
		while (*pt != 0)
		{
			if (which == 0)
			{
				estrcpy(gra_localstring, pt);
			} else
			{
				if (which > 1) addtoinfstr(infstr, NONFILECH);
				addstringtoinfstr(infstr, gra_localstring);
				addtoinfstr(infstr, DIRSEP);
				addstringtoinfstr(infstr, pt);
			}
			which++;
			while (*pt != 0) pt++;
			pt++;
		}
		if (which == 1) addstringtoinfstr(infstr, gra_localstring);
		pt = returninfstr(infstr);
	}
	delete fileDlg;
	return(pt);
}

/*
 * Routine to display a standard file prompt dialog and return the selected file.
 * The prompt message is in "msg" and the kind of file is in "filetype".  The default
 * output file name is in "defofile" (only used if "filetype" is negative).
 */
CHAR *fileselect(CHAR *msg, INTBIG filetype, CHAR *defofile)
{
	REGISTER INTBIG i;
	CHAR ofile[256], fs[256], prompt[256], *extension, *winfilter, *shortname,
		*longname;
	CFileDialog *fileDlg;
	INTBIG mactype;
	BOOLEAN binary;

	/* build filter string */
	describefiletype(filetype, &extension, &winfilter, &mactype, &binary, &shortname, &longname);
	estrcpy(fs, longname);
	estrcat(fs, x_(" ("));
	estrcat(fs, winfilter);
	estrcat(fs, x_(")|"));
	estrcat(fs, winfilter);
	estrcat(fs, x_("|"));
	estrcat(fs, _("All Files"));
	estrcat(fs, x_(" (*.*)|*.*||"));

	if (us_logplay != NULL)
	{
		if (gra_loggetnextaction(gra_localstring)) return(0);
		if (gra_inputstate != FILEREPLY) gra_localstring[0] = 0;
	} else
	{
		gra_localstring[0] = 0;
		if ((filetype & FILETYPEWRITE) == 0)
		{
			/* open file dialog */
			fileDlg = new CFileDialog(TRUE, NULL, NULL, NULL, fs, NULL);
			if (fileDlg == NULL) gra_localstring[0] = 0; else
			{
				fileDlg->m_ofn.Flags |= OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
				esnprintf(prompt, 256, _("%s Selection"), msg);
				fileDlg->m_ofn.lpstrTitle = prompt;
				if (fileDlg->DoModal() == IDOK)
					estrcpy(gra_localstring, fileDlg->GetPathName());
				delete fileDlg;
			}
		} else
		{
			/* save file dialog */
			for(i = estrlen(defofile)-1; i > 0; i--)
				if (defofile[i] == ':' || defofile[i] == '/' || defofile[i] == '\\') break;
			if (i > 0) i++;
			(void)estrcpy(ofile, &defofile[i]);
			fileDlg = new CFileDialog(FALSE, extension, ofile, NULL, fs, NULL);
			if (fileDlg != NULL)
			{
				fileDlg->m_ofn.Flags |= OFN_OVERWRITEPROMPT;
				esnprintf(prompt, 256, _("%s Creation"), msg);
				fileDlg->m_ofn.lpstrTitle = prompt;
				if (fileDlg->DoModal() == IDOK)
					estrcpy(gra_localstring, fileDlg->GetPathName());
				delete fileDlg;
			}
		}
	}

	/* log this result */
	gra_logwriteaction(FILEREPLY, 0, 0, 0, gra_localstring);

	if (gra_localstring[0] == 0) return((CHAR *)NULL); else
		return(gra_localstring);
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
	HANDLE handle;
	WIN32_FIND_DATA data;
	BOOL found;
	CHAR *dir;
	INTBIG len;
	REGISTER void *infstr;

	/* search for all files in directory */
	infstr = initinfstr();
	addstringtoinfstr(infstr, directory);
	addstringtoinfstr(infstr, x_("*.*"));
	dir = returninfstr(infstr);

	handle = FindFirstFile(dir, &data);
	if (handle == INVALID_HANDLE_VALUE) return(0);

	gra_initfilelist();
	for (found = 1; found; found = FindNextFile(handle, &data))
	{
		if (gra_addfiletolist(data.cFileName)) return(0);
	}
	FindClose(handle);

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
	static CHAR fullfile[MAXPATHLEN];

	/* is this a hack?  Full path defined when it starts with "LETTER:" */
	if (isalpha(file[0]) != 0 && file[1] == ':') return(file);

	/* full path also defined by "\\" at the start */
	if (file[0] == '\\' && file[1] == '\\') return(file);

	/* build the proper path */
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
#ifdef _UNICODE
	return(_wrename(file, newfile));
#else
	return(rename(file, newfile));
#endif
}

/*
 * routine to delete file "file"
 */
INTBIG eunlink(CHAR *file)
{
#ifdef _UNICODE
	return(_wunlink(file));
#else
	return(_unlink(file));
#endif
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
	struct estatstr buf;

	if (estat(name, &buf) < 0) return(0);
	if ((buf.st_mode & S_IFMT) == S_IFDIR) return(2);

	/* a file: see if it is writable */
	if (eaccess(name, 2) == 0) return(1);
	return(3);
}

/*
 * Routine to create a directory.
 * Returns true on error.
 */
BOOLEAN createdirectory(CHAR *dirname)
{
	if (emkdir(dirname) == 0) return(FALSE);
	return(TRUE);
}

/*
 * Routine to return the current directory name
 */
CHAR *currentdirectory(void)
{
	static CHAR line[MAXPATHLEN+1];
	REGISTER INTBIG len;

	egetcwd(line, MAXPATHLEN);
	len = estrlen(line);
	if (line[len-1] != DIRSEP)
	{
		line[len++] = DIRSEP;
		line[len] = 0;
	}
	return(line);
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
	CHAR username[256];
	UINTBIG size;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, x_("electricoptions_"));
	size = 256;
	GetUserName(username, &size);
	addstringtoinfstr(infstr, username);
	addstringtoinfstr(infstr, x_(".elib"));
	return(returninfstr(infstr));
}

/*
 * Routine to obtain the modification date on file "filename".
 */
time_t filedate(CHAR *filename)
{
	struct estatstr buf;
	time_t thetime;

	estat(filename, &buf);
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
	int fd;

	fd = ecreat(lockfilename, 0);
	if (fd == -1) return(FALSE);
	if (close(fd) == -1) return(FALSE);
	return(TRUE);
}

/*
 * Routine to unlock a resource called "lockfilename" by deleting such a file.
 */
void unlockfile(CHAR *lockfilename)
{
	INTBIG attrs;
	attrs = GetFileAttributes(lockfilename);
	if ((attrs & FILE_ATTRIBUTE_READONLY) != 0)
	{
		if (SetFileAttributes(lockfilename, attrs & ~FILE_ATTRIBUTE_READONLY) == 0)
		{
			ttyputerr(_("Error removing readonly bit on %s (error %ld)"),
				lockfilename, GetLastError());
		}
	}

	if (DeleteFile(lockfilename) == 0)
		ttyputerr(_("Error unlocking %s (error %ld)"), lockfilename, GetLastError());
}

/*
 * Routine to show file "filename" in a browser window.
 * Returns true if the operation cannot be done.
 */
BOOLEAN browsefile(CHAR *filename)
{
	CMainFrame *wnd;
	INTBIG err;

	wnd = (CMainFrame *)AfxGetMainWnd();
	err = (INTBIG)ShellExecute(wnd->m_hWnd, x_("open"), filename, x_(""),
		x_("C:\\Temp\\"), SW_SHOWNORMAL);
	if (err > 32) return(FALSE);
	ttyputmsg(_("Could not browse %s (error %ld)"), filename, err);
	return(TRUE);
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

time_t machinetimeoffset(void)
{
	return(0);
}

/* returns the time at which the current event occurred */
UINTBIG eventtime(void)
{
	return(gra_eventtime);
}

/* returns the current time in 60ths of a second */
UINTBIG ticktime(void)
{
	UINTBIG msTime;

	/* convert milliseconds to ticks */
	msTime = GetTickCount();
	return(msTime*6/100 + gra_timeoffset);
}

/* returns the double-click interval in 60ths of a second */
INTBIG doubleclicktime(void)
{
	INTBIG dct;

	dct = GetDoubleClickTime();
	dct = dct * 60 / 1000;
	return(dct);
}

/*
 * Routine to wait "ticks" sixtieths of a second and then return.
 */
void gotosleep(INTBIG ticks)
{
	Sleep(ticks * 100 / 6);
}

/*
 * Routine to start counting time.
 */
void starttimer(void)
{
	struct timeb timeptr;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	ftime(&timeptr);
	gra_timebasesec = timeptr.time;
	gra_timebasems = timeptr.millitm;
}

/*
 * Routine to stop counting time and return the number of elapsed seconds
 * since the last call to "starttimer()".
 */
float endtimer(void)
{
	float seconds;
	INTBIG milliseconds;
	struct timeb timeptr;

	ftime(&timeptr);
	milliseconds = (timeptr.time - gra_timebasesec) * 1000 + (timeptr.millitm-gra_timebasems);
	seconds = ((float)milliseconds) / 1000.0f;
	return(seconds);
}

/*************************** EVENT ROUTINES ***************************/

#define INITIALTIMERDELAY 8

INTBIG gra_lasteventstate = 0;
INTBIG gra_lasteventx;
INTBIG gra_lasteventy;
INTBIG gra_lasteventtime;
INTBIG gra_cureventtime = 0;

void gra_timerticked(void)
{
	gra_cureventtime++;
	if (gra_cureventtime > gra_lasteventtime+INITIALTIMERDELAY)
	{
		/* see if the last event was a mouse button going down */
		if ((gra_lasteventstate&(ISBUTTON|BUTTONUP)) == ISBUTTON)
		{
			/* turn it into a null motion */
			gra_addeventtoqueue((gra_lasteventstate & ~ISBUTTON) | MOTION, 0,
				gra_lasteventx, gra_lasteventy);
			gra_lasteventtime -= INITIALTIMERDELAY;
			return;
		}

		/* see if the last event was a motion with button down */
		if ((gra_lasteventstate&(MOTION|BUTTONUP)) == MOTION)
		{
			/* repeat the last event */
			gra_addeventtoqueue(gra_lasteventstate, 0, gra_lasteventx, gra_lasteventy);
			gra_lasteventtime -= INITIALTIMERDELAY;
			return;
		}
	}
}

void gra_addeventtoqueue(INTBIG state, INTBIG special, INTBIG x, INTBIG y)
{
	MYEVENTQUEUE *next, *prev;

	if (db_multiprocessing) emutexlock(gra_eventqueuemutex);

	next = gra_eventqueuetail + 1;
	if (next >= &gra_eventqueue[EVENTQUEUESIZE])
		next = gra_eventqueue;
	if (next == gra_eventqueuehead)
	{
		/* queue is full: see if last event was repeated */
		if (gra_eventqueuetail == gra_eventqueue)
			prev = &gra_eventqueue[EVENTQUEUESIZE-1]; else
				prev = gra_eventqueuetail - 1;
		if (prev->inputstate == state)
		{
			prev->cursorx = x;
			prev->cursory = y;
		} else
		{
			MessageBeep(MB_ICONASTERISK);
		}
	} else
	{
		gra_lasteventstate = gra_eventqueuetail->inputstate = state;
		gra_eventqueuetail->special = special;
		gra_lasteventx = gra_eventqueuetail->cursorx = x;
		gra_lasteventy = gra_eventqueuetail->cursory = y;
		gra_eventqueuetail->eventtime = ticktime();
		gra_lasteventtime = gra_cureventtime;
		gra_eventqueuetail = next;
	}

	if (db_multiprocessing) emutexunlock(gra_eventqueuemutex);
}

/*
 * Routine to get the next Electric input action and set the global "gra_inputstate"
 * accordingly.
 */
void gra_nextevent(void)
{
	MSG msg;
	REGISTER INTBIG windowindex, i, j, trueItem;
	BOOLEAN saveslice;
	REGISTER INTBIG x, y;
	BOOLEAN verbose;
	POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	extern BOOLEAN el_inslice;

	flushscreen();

	/* process events */
	while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!gra_app.PumpMessage())
		{
			::PostQuitMessage(0);
			break;
		}
	}

	if (us_logplay != NULL)
	{
		/* playing back log file: wait for an event */
		gra_inputstate = NOEVENT;
		static INTBIG lastx = 0, lasty = 0;
		while (gra_eventqueuehead != gra_eventqueuetail)
		{
			gra_inputstate = gra_eventqueuehead->inputstate;
			gra_inputspecial = gra_eventqueuehead->special;
			gra_cursorx = gra_eventqueuehead->cursorx;
			gra_cursory = gra_eventqueuehead->cursory;
			us_state &= ~GOTXY;
			gra_eventqueuehead++;
			if (gra_eventqueuehead >= &gra_eventqueue[EVENTQUEUESIZE])
				gra_eventqueuehead = gra_eventqueue;

			/* mouse click terminates */
			if ((gra_inputstate&ISBUTTON) != 0)
			{
				ttyputerr(_("Session playback aborted by mouse click"));
				xclose(us_logplay);
				us_logplay = NULL;
				gra_inputstate = NOEVENT;
				return;
			}

			/* stop if this is the last event in the queue */
			if (gra_eventqueuehead == gra_eventqueuetail) break;

			/* stop if the mouse moved */
			if (gra_cursorx != lastx || gra_cursory != lasty) break;

			/* stop if this and the next event are not motion */
			if ((gra_inputstate&MOTION) == 0) break;
			if ((gra_eventqueuehead->inputstate&MOTION) == 0) break;
		}
		lastx = gra_cursorx;   lasty = gra_cursory;

		if (gra_inputstate != NOEVENT || gra_firstactivedialog != NOTDIALOG)
		{
			/* replace this with the logged action */
			(void)gra_loggetnextaction(0);
		}
	} else
	{
		/* normal interaction: get next event */
		gra_inputstate = NOEVENT;
		while (gra_eventqueuehead != gra_eventqueuetail)
		{
			gra_inputstate = gra_eventqueuehead->inputstate;
			gra_inputspecial = gra_eventqueuehead->special;
			gra_cursorx = gra_eventqueuehead->cursorx;
			gra_cursory = gra_eventqueuehead->cursory;
			gra_eventtime = gra_eventqueuehead->eventtime;
			us_state &= ~GOTXY;
			gra_eventqueuehead++;
			if (gra_eventqueuehead >= &gra_eventqueue[EVENTQUEUESIZE])
				gra_eventqueuehead = gra_eventqueue;

			/* stop if this is the last event in the queue */
			if (gra_eventqueuehead == gra_eventqueuetail) break;

			/* stop if this and the next event are not motion */
			if ((gra_inputstate&MOTION) == 0) break;
			if ((gra_eventqueuehead->inputstate&MOTION) == 0) break;
		}
	}

	/* record valid events */
	if (gra_inputstate != NOEVENT)
	{
		if (gra_inputstate == WINDOWSIZE || gra_inputstate == WINDOWMOVE)
		{
			windowindex = gra_cursorx;
			x = gra_cursory >> 16;
			y = gra_cursory & 0xFFFF;
			gra_logwriteaction(gra_inputstate, 0, x, y, (void *)windowindex);
			gra_inputstate = NOEVENT;
		} else
		{
			windowindex = -1;
			if (el_curwindowframe != NOWINDOWFRAME) windowindex = el_curwindowframe->windindex;
			gra_logwriteaction(gra_inputstate, gra_inputspecial, gra_cursorx,
				gra_cursory, (void *)windowindex);
		}

		/* handle menu events */
		if (gra_inputstate == MENUEVENT)
		{
			gra_inputstate = NOEVENT;
			i = gra_cursorx;
			if (i >= 0 && i < gra_pulldownmenucount)
			{
				pm = us_getpopupmenu(gra_pulldowns[i]);
				for(trueItem=j=0; j < pm->total; j++)
				{
					mi = &pm->list[j];
					if (mi->response->active < 0 && *mi->attribute == 0) continue;
					trueItem++;
					if (trueItem != gra_cursory) continue;
					us_state |= DIDINPUT;
					us_state &= ~GOTXY;
					setdefaultcursortype(us_normalcursor);

					/* special case for "system" commands": don't erase */
					us_forceeditchanges();
					saveslice = el_inslice;
					el_inslice = TRUE;
					if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
						verbose = FALSE;
					us_execute(mi->response, verbose, TRUE, TRUE);
					el_inslice = saveslice;
					db_setcurrenttool(us_tool);
					setactivity(mi->attribute);
					break;
				}
			}
		}
	}
}

void gra_buttonaction(int state, UINT nFlags, CPoint point, CWnd *frm)
{
	WINDOWFRAME *wf;
	INTBIG event, itemtype, x, y, item;
	TDIALOG *dia;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if ((CChildFrame *)wf->wndframe == frm) break;
	if (wf == NOWINDOWFRAME)
	{
		for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
			if (dia->window == frm) break;
		if (dia == NOTDIALOG) return;

		item = gra_getdialogitem(dia, point.x, point.y);
		if (item == 0) return;
		itemtype = dia->itemdesc->list[item-1].type;
		if ((itemtype&ITEMTYPE) == USERDRAWN || (itemtype&ITEMTYPE) == ICON ||
			(dia->modelessitemhit != 0 && gra_trackingdialog == 0))
		{
			if (state == 0)
			{
				gra_itemclicked((CElectricDialog *)frm, item-1+ID_DIALOGITEM_0);
				return;
			}
			if (state == 2)
			{
				gra_itemdoubleclicked((CElectricDialog *)frm, item-1);
				return;
			}
		}
	}

	/* set appropriate button */
	if ((nFlags & MK_LBUTTON) != 0) event = ISBUTTON|ISLEFT;
	if ((nFlags & MK_MBUTTON) != 0) event = ISBUTTON|ISMIDDLE;
	if ((nFlags & MK_RBUTTON) != 0) event = ISBUTTON|ISRIGHT;

	/* add in extras */
	if ((nFlags & MK_SHIFT) != 0) event |= SHIFTISDOWN;
	if ((nFlags & MK_CONTROL) != 0) event |= CONTROLISDOWN;
	if ((GetKeyState(VK_MENU)&0x8000) != 0) event |= ALTISDOWN;
	if (state == 2 && (event&(SHIFTISDOWN|CONTROLISDOWN|ALTISDOWN)) == 0)
		event |= DOUBLECLICK;
	if (state == 1)
		event |= BUTTONUP;

	x = point.x;
	if (wf == NOWINDOWFRAME) y = point.y; else
		y = wf->revy - point.y;
	us_state |= DIDINPUT;
	gra_addeventtoqueue(event, 0, x, y);
}

void gra_mouseaction(UINT nFlags, CPoint point, CWnd *frm)
{
	WINDOWFRAME *wf;
	INTBIG event, x, y, inmenu;
	REGISTER VARIABLE *var;
	CHAR *str;
	static BOOLEAN overrodestatus = FALSE;
	COMMANDBINDING commandbinding;
	TDIALOG *dia;

	/* find the appropriate window */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if ((CChildFrame *)wf->wndframe == frm) break;
	inmenu = 0;
	if (wf != NOWINDOWFRAME)
	{
		/* report the menu if over one */
		if ((nFlags & (MK_LBUTTON|MK_MBUTTON|MK_RBUTTON)) == 0 && wf->floating)
		{
			gra_cursorx = point.x;
			gra_cursory = wf->revy - point.y;
			us_state &= ~GOTXY;
			if (us_menuxsz > 0 && us_menuysz > 0)
			{
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
								overrodestatus = TRUE;
								inmenu = 1;
							}
							if (commandbinding.arcglyph != NOARCPROTO)
							{
								ttysetstatusfield(NOWINDOWFRAME, us_statusarc, describearcproto(commandbinding.arcglyph), TRUE);
								if (us_curnodeproto == NONODEPROTO) str = x_(""); else
									str = describenodeproto(us_curnodeproto);
								ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
								overrodestatus = TRUE;
								inmenu = 1;
							}
						}
						us_freebindingparse(&commandbinding);
					}
				}
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

	if (gra_inputstate != NOEVENT) return;

	if (wf == NOWINDOWFRAME)
	{
		for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
			if (dia->window == frm) break;
		if (dia == NOTDIALOG) return;
		if (gra_dodialogisinsideuserdrawn(dia, point.x, point.y) == 0) return;
	}

	x = point.x;
	if (wf == NOWINDOWFRAME) y = point.y; else
		y = wf->revy - point.y;
	event = MOTION;

	if ((nFlags & (MK_LBUTTON|MK_MBUTTON|MK_RBUTTON)) == 0)
		event |= BUTTONUP;
	gra_addeventtoqueue(event, 0, x, y);
}

extern "C" INTBIG sim_window_wavexbar;

int gra_setpropercursor(CWnd *frm, int x, int y)
{
	WINDOWFRAME *wf;
	INTBIG lx, hx, ly, hy, xfx, xfy;
	REGISTER WINDOWPART *w;
	REGISTER EDITOR *e;

	/* find the appropriate window */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if ((CChildFrame *)wf->wndframe == frm) break;
	if (wf != NOWINDOWFRAME)
	{
		/* if the window is known, set the cursor appropriately */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->frame != wf) continue;
			y = wf->revy - y;

			/* see if the cursor is over a window partition separator */
			us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
			if (x >= lx-1 && x <= lx+1 && y > ly+1 && y < hy-1 &&
				us_hasotherwindowpart(lx-10, y, w))
			{
				setdefaultcursortype(LRCURSOR);
				return(1);
			} else if (x >= hx-1 && x <= hx+1 && y > ly+1 && y < hy-1 &&
				us_hasotherwindowpart(hx+10, y, w))
			{
				setdefaultcursortype(LRCURSOR);
				return(1);
			} else if (y >= ly-1 && y <= ly+1 && x > lx+1 && x < hx-1 &&
				us_hasotherwindowpart(x, ly-10, w))
			{
				setdefaultcursortype(UDCURSOR);
				return(1);
			} else if (y >= hy-1 && y <= hy+1 && x > lx+1 && x < hx-1 &&
				us_hasotherwindowpart(x, hy+10, w))
			{
				setdefaultcursortype(UDCURSOR);
				return(1);
			}
			if (x < w->uselx || x > w->usehx || y < w->usely || y > w->usehy) continue;
			if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
			{
				xfx = muldiv(x - w->uselx, w->screenhx - w->screenlx,
					w->usehx - w->uselx) + w->screenlx;
				xfy = muldiv(y - w->usely, w->screenhy - w->screenly,
					w->usehy - w->usely) + w->screenly;
				if (abs(xfx - sim_window_wavexbar) < 2 && xfy >= 560)
				{
					setdefaultcursortype(LRCURSOR);
					return(1);
				}
			}
			if ((w->state&WINDOWTYPE) == POPTEXTWINDOW ||
				(w->state&WINDOWTYPE) == TEXTWINDOW)
			{
				e = w->editor;
				if ((e->state&EDITORTYPE) == PACEDITOR)
				{
					if (x <= w->usehx - SBARWIDTH && y >= w->usely + SBARWIDTH && y < e->revy)
					{
						setdefaultcursortype(IBEAMCURSOR);
						return(1);
					}
				}
			}
			break;
		}
	}
	setdefaultcursortype(us_normalcursor);
	return(0);
}

void gra_mousewheelaction(UINT nFlags, short zDelta, CPoint point, CWnd *frm)
{
	WINDOWFRAME *wf;
	INTBIG event;
	INTBIG x, y;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if ((CChildFrame *)wf->wndframe == frm) break;
	if (wf == NOWINDOWFRAME) return;

	/* set appropriate "button" */
	if (zDelta > 0) event = ISBUTTON|ISWHLFWD; else
		event = ISBUTTON|ISWHLBKW;

	/* add in extras */
	if ((nFlags & MK_SHIFT) != 0) event |= SHIFTISDOWN;
	if ((nFlags & MK_CONTROL) != 0) event |= CONTROLISDOWN;
	if ((GetKeyState(VK_MENU)&0x8000) != 0) event |= ALTISDOWN;

	x = point.x;
	if (wf == NOWINDOWFRAME) y = point.y; else
		y = wf->revy - point.y;
	us_state |= DIDINPUT;
	gra_addeventtoqueue(event, 0, x, y);
}

void gra_keyaction(UINT nChar, INTBIG special, UINT nRepCnt)
{
	POINT pt, p2;
	CWnd *wnd;
	INTBIG event, x, y;

	/* determine corner of window */
	if (el_curwindowframe == NOWINDOWFRAME) wnd = AfxGetMainWnd(); else
		wnd = (CWnd *)el_curwindowframe->wndframe;
	p2.x = p2.y = 0;
	wnd->MapWindowPoints(0, &p2, 1);

	/* determine current cursor coordinates */
	GetCursorPos(&pt);
	x = pt.x - p2.x;
	y = pt.y - p2.y;
	if (el_curwindowframe != NOWINDOWFRAME)
		y = el_curwindowframe->revy - y;

	/* queue the event */
	if (nChar == 015) nChar = 012;
	event = (nChar & CHARREAD) | ISKEYSTROKE;
	us_state |= DIDINPUT;
	gra_addeventtoqueue(event, special, x, y);
}

void gra_setdefaultcursor(void)
{
	int curstate;

	curstate = us_cursorstate;
	us_cursorstate++;
	setdefaultcursortype(curstate);
}

void gra_activateframe(CChildFrame *frame, BOOL bActivate)
{
	REGISTER WINDOWFRAME *wf;
	REGISTER WINDOWPART *w;

	if (!bActivate)
	{
		if (frame == gra_messageswindow) gra_messagescurrent = FALSE; else
			el_curwindowframe = NOWINDOWFRAME;
		return;
	}

	if (frame == gra_messageswindow) gra_messagescurrent = TRUE; else
	{
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
 			if ((CChildFrame *)wf->wndframe == frame) break;
		if (wf != NOWINDOWFRAME)
		{
			el_curwindowframe = wf;
			gra_cureditwindowframe = wf;

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
							(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key,
								(INTBIG)w, VWINDOWPART|VDONTSAVE);
							(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"),
								(INTBIG)w->curnodeproto, VNODEPROTO);
							break;
						}
					}
				}
			}
		}
	}

	/* if another windows was activated, make sure the palette is on top */
	gra_floatpalette();
}

/*
 * Routine to ensure that the floating component menu is on top
 */
void gra_floatpalette(void)
{
	REGISTER WINDOWFRAME *wf;

	if (gra_creatingwindow) return;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (!wf->floating) continue;
		((CChildFrame *)wf->wndframe)->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
			SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		break;
	}
}

int gra_closeframe(CChildFrame *frame)
{
	WINDOWFRAME *wf;
	CHAR *par[2];

	if (frame == gra_messageswindow)
	{
		gra_messagescurrent = FALSE;
		gra_messageswindow = 0;
		return(1);
	}

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (frame == (CChildFrame *)wf->wndframe) break;
	}
	if (wf == NOWINDOWFRAME) return(0);

//	((CChildFrame *)wf->wndframe)->SetActiveWindow();
	par[0] = x_("delete");
	us_window(1, par);
	return(0);
}

int gra_closeworld(void)
{
	if (us_preventloss(NOLIBRARY, 0, TRUE)) return(1);  /* keep working */
	bringdown();
	return(0);
}

void gra_repaint(CChildFrame *frame, CPaintDC *dc)
{
	WINDOWFRAME *wf;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (frame == (CChildFrame *)wf->wndframe)
		{
			dc->BitBlt(0, 0, wf->swid, wf->shei, (CDC *)wf->hDCOff, 0, 0, SRCCOPY);
			break;
		}
	}
	gra_floatpalette();
}

void gra_resize(CChildFrame *frame, int cx, int cy)
{
	WINDOWFRAME *wf;
	RECT r;
	WINDOWPLACEMENT wp;

	if (frame->IsIconic()) return;

	if (frame == gra_messageswindow)
	{
		gra_messageswindow->GetWindowPlacement(&wp);
		r = wp.rcNormalPosition;
		gra_messagesleft = r.left;
		gra_messagesright = r.right;
		gra_messagestop = r.top;
		gra_messagesbottom = r.bottom;
		return;
	}

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (frame == (CChildFrame *)wf->wndframe)
		{
			sizewindowframe(wf, cx, cy);
			gra_redrawdisplay(wf);
			gra_addeventtoqueue(WINDOWSIZE, 0, wf->windindex, ((cx & 0xFFFF) << 16) | (cy & 0xFFFF));
			break;
		}
	}
	gra_floatpalette();
}

void gra_movedwindow(CChildFrame *frame, int x, int y)
{
	RECT fr, cr, r;
	int framewid, framehei;
	WINDOWPLACEMENT wp;
	WINDOWFRAME *wf;

	if (frame == gra_messageswindow)
	{
		gra_messageswindow->GetWindowPlacement(&wp);
		r = wp.rcNormalPosition;
		gra_messagesleft = r.left;
		gra_messagesright = r.right;
		gra_messagestop = r.top;
		gra_messagesbottom = r.bottom;
		return;
	}

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (frame == (CChildFrame *)wf->wndframe)
		{
			((CChildFrame *)wf->wndframe)->GetWindowPlacement(&wp);
			fr = wp.rcNormalPosition;
			((CChildFrame *)wf->wndframe)->GetClientRect(&cr);
			framewid = (fr.right - fr.left) - (cr.right - cr.left);
			framehei = (fr.bottom - fr.top) - (cr.bottom - cr.top);
			x -= framewid/2;   y -= (framehei-framewid/2);
			gra_addeventtoqueue(WINDOWMOVE, 0, wf->windindex, ((x & 0xFFFF) << 16) | (y & 0xFFFF));
			break;
		}
	}
	gra_floatpalette();
}

void gra_resizemain(int cx, int cy)
{
	WINDOWPLACEMENT wndpl;
	CHAR tmp[256], subkey[256], value[256];
	CMainFrame *wnd;

	gra_redrawstatusindicators();
	if (gra_palettewindowframe != NOWINDOWFRAME)
		us_drawmenu(-1, gra_palettewindowframe);

	/* remember this size in the registry */
	wnd = (CMainFrame *)AfxGetMainWnd();
	wnd->GetWindowPlacement(&wndpl);
	estrcpy(subkey, x_("Software\\Static Free Software\\Electric"));
	estrcpy(value, x_("Window_Location"));
	esnprintf(tmp, 256, x_("%ld,%ld,%ld,%ld,%d"), wndpl.rcNormalPosition.top, wndpl.rcNormalPosition.left,
		wndpl.rcNormalPosition.bottom, wndpl.rcNormalPosition.right, wndpl.showCmd);
	(void)gra_setregistry(HKEY_LOCAL_MACHINE, subkey, value, tmp);
}

/* handle interrupts */
void gra_onint(void)
{
	el_pleasestop = 1;
	ttyputerr(_("Interrupted..."));
}

/*************************** SESSION LOGGING ROUTINES ***************************/

extern "C" {
/* Session Playback */
DIALOGITEM gra_sesplaydialogitems[] =
{
 /*  1 */ {0, {100,132,124,212}, BUTTON, N_("Yes")},
 /*  2 */ {0, {100,8,124,88}, BUTTON, N_("No")},
 /*  3 */ {0, {8,8,24,232}, MESSAGE, N_("Electric has found a session log file")},
 /*  4 */ {0, {24,8,40,232}, MESSAGE, N_("which may be from a recent crash.")},
 /*  5 */ {0, {52,8,68,232}, MESSAGE, N_("Do you wish to replay this session")},
 /*  6 */ {0, {68,8,84,232}, MESSAGE, N_("and reconstruct the lost work?")}
};
DIALOG gra_sesplaydialog = {{75,75,208,316}, N_("Replay Log?"), 0, 6, gra_sesplaydialogitems, 0, 0};
};

/* special items for the session playback dialog: */
#define DSPL_YES    1		/* Yes (button) */
#define DSPL_NO     2		/* No (button) */

/*
 * routine to create a session logging file
 */
void logstartrecord(void)
{
	UCHAR1 count;
	REGISTER INTBIG itemhit, filestatus;
	REGISTER LIBRARY *lib;
	REGISTER WINDOWFRAME *wf;
	REGISTER WINDOWPART *w;
	RECT fr;
	WINDOWPLACEMENT wp;
	REGISTER VARIABLE *var;
	REGISTER void *dia;

	/* if there is already a log file, it may be from a previous crash */
	filestatus = fileexistence(gra_logfile);
	if (filestatus == 1 || filestatus == 3)
	{
		dia = DiaInitDialog(&gra_sesplaydialog);
		for(;;)
		{
			itemhit = DiaNextHit(dia);
			if (itemhit == DSPL_YES || itemhit == DSPL_NO) break;
		}
		DiaDoneDialog(dia);
		if (itemhit == DSPL_YES)
		{
			if (fileexistence(gra_logfilesave) == 1)
				eunlink(gra_logfilesave);
			erename(gra_logfile, gra_logfilesave);
			(void)logplayback(gra_logfilesave);
		}
	}

	us_logrecord = xcreate(gra_logfile, us_filetypelog, 0, 0);
	if (us_logrecord == 0) return;
	gra_logbasetime = ticktime();

	/* document the header */
	xprintf(us_logrecord, x_("; ============= The header:\n"));
	xprintf(us_logrecord, x_("; LC n           Library count\n"));
	xprintf(us_logrecord, x_("; LD name file   Library read from disk\n"));
	xprintf(us_logrecord, x_("; LM name file   Library not read from disk\n"));
	xprintf(us_logrecord, x_("; WO n           Window offset currently 'n'\n"));
	xprintf(us_logrecord, x_("; WT n           Window count\n"));
	xprintf(us_logrecord, x_("; WC n x y w h   Component window, index 'n' at (x,y), size (wXh)\n"));
	xprintf(us_logrecord, x_("; WE n x y w h   Edit window, index 'n' at (x,y), size (wXh)\n"));
	xprintf(us_logrecord, x_("; WP n           Window has 'n' partitions\n"));
	xprintf(us_logrecord, x_("; WB lx hx ly hy slx shx sly shy state cell loc    Window partition (background)\n"));
	xprintf(us_logrecord, x_("; WF lx hx ly hy slx shx sly shy state cell loc    Window partition (foreground)\n"));
	xprintf(us_logrecord, x_("; CI n           Current window index\n"));
	xprintf(us_logrecord, x_("; CT tech        Current technology\n"));

	/* log current libraries */
	count = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if ((lib->userbits&HIDDENLIBRARY) == 0) count++;
	xprintf(us_logrecord, x_("LC %d\n"), count);
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		if ((lib->userbits&READFROMDISK) != 0)
		{
			xprintf(us_logrecord, x_("LD %s %s\n"), lib->libname, lib->libfile);
		} else
		{
			xprintf(us_logrecord, x_("LM %s %s\n"), lib->libname, lib->libfile);
		}
	}

	/* log current windows */
	xprintf(us_logrecord, x_("WO %ld\n"), gra_newwindowoffset);
	count = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe) count++;
	xprintf(us_logrecord, x_("WT %d\n"), count);
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		((CChildFrame *)wf->wndframe)->GetWindowPlacement(&wp);
		fr = wp.rcNormalPosition;
		if (wf->floating != 0)
		{
			xprintf(us_logrecord, x_("WC %ld %ld %ld %ld %ld\n"), wf->windindex, fr.left, fr.top,
				fr.right-fr.left, fr.bottom-fr.top);
		} else
		{
			xprintf(us_logrecord, x_("WE %ld %ld %ld %ld %ld\n"), wf->windindex, fr.left, fr.top,
				fr.right-fr.left, fr.bottom-fr.top);
		}

		count = 0;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			if (w->frame == wf) count++;
		xprintf(us_logrecord, x_("WP %d\n"), count);
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if (w->frame != wf) continue;
			if (w == el_curwindowpart)
			{
				xprintf(us_logrecord, x_("WF %ld %ld %ld %ld %ld %ld %ld %ld %ld %s %s\n"),
					w->uselx, w->usehx, w->usely, w->usehy, w->screenlx, w->screenhx,
					w->screenly, w->screenhy, w->state, describenodeproto(w->curnodeproto),
					w->location);
			} else
			{
				xprintf(us_logrecord, x_("WB %ld %ld %ld %ld %ld %ld %ld %ld %ld %s %s\n"),
					w->uselx, w->usehx, w->usely, w->usehy, w->screenlx, w->screenhx,
					w->screenly, w->screenhy, w->state, describenodeproto(w->curnodeproto),
					w->location);
			}
		}
	}
	xprintf(us_logrecord, x_("CI %ld\n"), gra_windownumberindex);

	/* log current technology (macros store this in %H) */
	var = getval((INTBIG)us_tool, VTOOL, VSTRING, x_("USER_local_caph"));
	if (var != NOVARIABLE)
	{
		/* technology name found in local variable */
		xprintf(us_logrecord, x_("CT %s\n"), (CHAR *)var->addr);
	} else
	{
		/* just write the current technology name */
		xprintf(us_logrecord, x_("CT %s\n"), el_curtech->techname);
	}

	/* document the body */
	xprintf(us_logrecord, x_("; ============= The body:\n"));
	xprintf(us_logrecord, x_("; KT k x y s w t Key 'k' typed at (x,y), special 's', window 'w', time 't'\n"));
	xprintf(us_logrecord, x_("; BP n x y s w t Button 'n' pressed at (x,y), special 's', window 'w', time 't'\n"));
	xprintf(us_logrecord, x_("; BD n x y s w t Button 'n' double-clicked at (x,y), special 's', window 'w', time 't'\n"));
	xprintf(us_logrecord, x_("; BR n x y s w t Button 'n' released at (x,y), special 's', window 'w', time 't'\n"));
	xprintf(us_logrecord, x_("; MP x y s w     Motion with button pressed at (x,y), special 's', window 'w'\n"));
	xprintf(us_logrecord, x_("; MR x y s w     Motion with button released at (x,y), special 's', window 'w'\n"));
	xprintf(us_logrecord, x_("; ME m i t       Invoked menu 'm', item 'i', time 't'\n"));
	xprintf(us_logrecord, x_("; WS w x y t     Window 'w' grows to (xXy), time 't'\n"));
	xprintf(us_logrecord, x_("; WM w x y t     Window 'w' moves to (x,y), time 't'\n"));
	xprintf(us_logrecord, x_("; FS path        File selected is 'path'\n"));
	xprintf(us_logrecord, x_("; PM v t         Popup menu selected 'v', time 't'\n"));
	xprintf(us_logrecord, x_("; DI w i         Item 'i' of dialog 'w' selected\n"));
	xprintf(us_logrecord, x_("; DS w i c vals  Dialog 'w' scroll item 'i' selects 'c' lines in 'vals'\n"));
	xprintf(us_logrecord, x_("; DE w i hc s    Dialog 'w' edit item 'i' hit character 'hc', text now 's'\n"));
	xprintf(us_logrecord, x_("; DP w i e       Dialog 'w' popup item 'i' set to entry 'e'\n"));
	xprintf(us_logrecord, x_("; DC w i v       Dialog 'w' item 'i' set to value 'v'\n"));
	xprintf(us_logrecord, x_("; DM w x y       Dialog 'w' coordinates at (x,y)\n"));
	xprintf(us_logrecord, x_("; DD w           Dialog 'w' done\n"));
}

/*
 * routine to begin playback of session logging file "file".  The routine
 * returns true if there is an error.
 */
BOOLEAN logplayback(CHAR *file)
{
	CHAR *filename, tempstring[300], *pt, *start;
	UCHAR1 count, wcount, i, j, cur, fromdisk;
	BOOLEAN floating;
	REGISTER WINDOWFRAME *wf;
	RECTAREA r;
	REGISTER FILE *saveio;
	REGISTER WINDOWPART *w, *nextw;
	REGISTER INTBIG wid, hei, uselx, usehx, usely, usehy, sindex;
	REGISTER INTBIG screenlx, screenhx, screenly, screenhy, state;
	REGISTER LIBRARY *lib, *firstlib;

	us_logplay = xopen(file, us_filetypelog, x_(""), &filename);
	if (us_logplay == NULL) return(TRUE);
	ttyputmsg(_("Playing log file..."));
	ttyputmsg(_("   Move mouse continuously to advance playback"));
	ttyputmsg(_("   Click mouse to abort playback"));
	gra_lastplaybacktime = 0;

	/* get current libraries */
	(void)gra_logreadline(tempstring, 300);
	if (estrncmp(tempstring, x_("LC"), 2) != 0)
	{
		ttyputerr(_("Log file is corrupt (error %d)"), 1);
		return(TRUE);
	}
	count = eatoi(&tempstring[3]);
	firstlib = NOLIBRARY;
	for(i=0; i<count; i++)
	{
		(void)gra_logreadline(tempstring, 300);
		if (estrncmp(tempstring, x_("LD"), 2) == 0)
		{
			fromdisk = 1;
		} else if (estrncmp(tempstring, x_("LM"), 2) == 0)
		{
			fromdisk = 0;
		} else
		{
			ttyputerr(_("Log file is corrupt (error %d)"), 2);
			return(TRUE);
		}
		start = &tempstring[3];
		for(pt = start; *pt != 0; pt++) if (*pt == ' ') break;
		if (*pt == 0)
		{
			ttyputerr(_("Log file is corrupt (error %d)"), 3);
			return(TRUE);
		}
		*pt++ = 0;
		lib = getlibrary(start);
		if (lib == NOLIBRARY)
		{
			/* read library file "gra_localstring" */
			lib = newlibrary(start, pt);
			if (lib == NOLIBRARY) continue;
			if (fromdisk != 0)
			{
				saveio = us_logplay;
				us_logplay = 0;
				(void)asktool(io_tool, x_("read"), (INTBIG)lib, (INTBIG)x_("binary"), 0);
				us_logplay = saveio;
			}
		}
		if (firstlib == NOLIBRARY) firstlib = lib;
	}
	selectlibrary(firstlib, TRUE);

	/* delete all existing windows */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = nextw)
	{
		nextw = w->nextwindowpart;
		db_retractwindowpart(w);
	}
	el_curwindowpart = NOWINDOWPART;

	/* get current windows */
	(void)gra_logreadline(tempstring, 300);
	if (estrncmp(tempstring, x_("WO"), 2) != 0)
	{
		ttyputerr(_("Log file is corrupt (error %d)"), 4);
		return(TRUE);
	}
	gra_newwindowoffset = eatoi(&tempstring[3]);

	(void)gra_logreadline(tempstring, 300);
	if (estrncmp(tempstring, x_("WT"), 2) != 0)
	{
		ttyputerr(_("Log file is corrupt (error %d)"), 5);
		return(TRUE);
	}
	count = eatoi(&tempstring[3]);

	for(i=0; i<count; i++)
	{
		(void)gra_logreadline(tempstring, 300);
		if (estrncmp(tempstring, x_("WC"), 2) == 0)
		{
			floating = 1;
		} else if (estrncmp(tempstring, x_("WE"), 2) == 0)
		{
			floating = 0;
		} else
		{
			ttyputerr(_("Log file is corrupt (error %d)"), 6);
			return(TRUE);
		}
		pt = &tempstring[3];
		sindex = eatoi(getkeyword(&pt, x_(" ")));
		r.left = eatoi(getkeyword(&pt, x_(" ")));
		r.top = eatoi(getkeyword(&pt, x_(" ")));
		wid = eatoi(getkeyword(&pt, x_(" ")));
		hei = eatoi(getkeyword(&pt, x_(" ")));
		r.right = r.left + (INTSML)wid;
		r.bottom = r.top + (INTSML)hei;
		if (floating)
		{
			/* get the floating window frame */
			for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
				if (wf->floating) break;
			if (wf == NOWINDOWFRAME) wf = newwindowframe(TRUE, &r);
		} else
		{
			/* create a new window frame */
			wf = newwindowframe(FALSE, &r);
		}
		wf->windindex = sindex;

		(void)gra_logreadline(tempstring, 300);
		if (estrncmp(tempstring, x_("WP"), 2) != 0)
		{
			ttyputerr(_("Log file is corrupt (error %d)"), 7);
			return(TRUE);
		}
		wcount = eatoi(&tempstring[3]);
		for(j=0; j<wcount; j++)
		{
			(void)gra_logreadline(tempstring, 300);
			if (estrncmp(tempstring, x_("WB"), 2) == 0)
			{
				cur = 0;
			} else if (estrncmp(tempstring, x_("WF"), 2) == 0)
			{
				cur = 1;
			} else
			{
				ttyputerr(_("Log file is corrupt (error %d)"), 8);
				return(TRUE);
			}
			pt = &tempstring[3];
			uselx = eatoi(getkeyword(&pt, x_(" ")));
			usehx = eatoi(getkeyword(&pt, x_(" ")));
			usely = eatoi(getkeyword(&pt, x_(" ")));
			usehy = eatoi(getkeyword(&pt, x_(" ")));
			screenlx = eatoi(getkeyword(&pt, x_(" ")));
			screenhx = eatoi(getkeyword(&pt, x_(" ")));
			screenly = eatoi(getkeyword(&pt, x_(" ")));
			screenhy = eatoi(getkeyword(&pt, x_(" ")));
			state = eatoi(getkeyword(&pt, x_(" ")));
			start = getkeyword(&pt, x_(" "));
			while (*pt != 0 && *pt != ' ') pt++;
			if (*pt == ' ') pt++;

			w = newwindowpart(pt, NOWINDOWPART);
			w->buttonhandler = DEFAULTBUTTONHANDLER;
			w->charhandler = DEFAULTCHARHANDLER;
			w->changehandler = DEFAULTCHANGEHANDLER;
			w->termhandler = DEFAULTTERMHANDLER;
			w->redisphandler = DEFAULTREDISPHANDLER;
			w->uselx = uselx;   w->usehx = usehx;
			w->usely = usely;   w->usehy = usehy;
			w->screenlx = screenlx;   w->screenhx = screenhx;
			w->screenly = screenly;   w->screenhy = screenhy;
			computewindowscale(w);
			w->state = state;
			w->curnodeproto = getnodeproto(start);
			w->frame = wf;
			if (cur != 0) el_curwindowpart = w;
			us_redisplay(w);
		}
	}
	(void)gra_logreadline(tempstring, 300);
	if (estrncmp(tempstring, x_("CI"), 2) != 0)
	{
		ttyputerr(_("Log file is corrupt (error %d)"), 9);
		return(TRUE);
	}
	gra_windownumberindex = eatoi(&tempstring[3]);

	/* switch to proper technology */
	(void)gra_logreadline(tempstring, 300);
	if (estrncmp(tempstring, x_("CT"), 2) != 0)
	{
		ttyputerr(_("Log file is corrupt (error %d)"), 10);
		return(TRUE);
	}
	us_ensurepropertechnology(NONODEPROTO, &tempstring[3], TRUE);

	return(FALSE);
}

/*
 * routine to terminate session logging
 */
void logfinishrecord(void)
{
	if (us_logrecord != NULL)
	{
		xclose(us_logrecord);
		if (fileexistence(gra_logfilesave) == 1)
			eunlink(gra_logfilesave);
		erename(gra_logfile, gra_logfilesave);
	}
	us_logrecord = NULL;
}

/*
 * Routine to log an event (if logging) of type "inputstate".
 * The event has parameters (cursorx,cursory) and "extradata", depending on "inputstate":
 *   WINDOWSIZE:    window "extradata" is now "cursorx" x "cursory"
 *   WINDOWMOVE:    window "extradata" is now at (cursorx, cursory)
 *   MENUEVENT:     selected menu "cursorx", item "cursory"
 *   FILEREPLY:     file selected by standard-file dialog is in "extradata"
 *   POPUPSELECT:   popup menu returned "cursorx"
 *   DIAITEMCLICK:  dialog "special" item "cursorx" clicked
 *   DIASCROLLSEL:  dialog "special" scroll item "cursorx" set to "cursory" entries in "extradata"
 *   DIAEDITTEXT:   dialog "special" edit item "cursorx" changed to "extradata"
 *   DIAPOPUPSEL:   dialog "special" popup item "cursorx" set to entry "cursory"
 *   DIASETCONTROL: dialog "special" control item "cursorx" changed to "cursory"
 *   DIAENDDIALOG:  dialog "special" terminated
 *   all others:    cursor in (cursorx,cursory) and window index in "extradata"
 */
void gra_logwriteaction(INTBIG inputstate, INTBIG special, INTBIG cursorx, INTBIG cursory,
	void *extradata)
{
	REGISTER CHAR *filename;
	REGISTER INTBIG i, j, trueItem;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;

	if (us_logrecord == NULL) return;

	/* ignore redundant cursor motion */
	if (inputstate == MOTION || inputstate == (MOTION|BUTTONUP))
	{
		i = (INTBIG)extradata;
		if (inputstate == gra_lastloggedaction && cursorx == gra_lastloggedx &&
			cursory == gra_lastloggedy && i == gra_lastloggedindex)
				return;
	}
	gra_lastloggedaction = inputstate;
	gra_lastloggedx = cursorx;
	gra_lastloggedy = cursory;
	gra_lastloggedindex = (INTBIG)extradata;

	if ((inputstate&MOTION) != 0)
	{
		if ((inputstate&BUTTONUP) != 0)
		{
			xprintf(us_logrecord, x_("MR %ld %ld %ld %ld\n"),
				cursorx, cursory, special, extradata);
		} else
		{
			xprintf(us_logrecord, x_("MP %ld %ld %ld %ld\n"),
				cursorx, cursory, special, extradata);
		}
	} else if ((inputstate&ISKEYSTROKE) != 0)
	{
		xprintf(us_logrecord, x_("KT '%s' %ld %ld %ld %ld\n"),
			us_describeboundkey((INTSML)(inputstate&CHARREAD), special, 1),
				cursorx, cursory, extradata, ticktime()-gra_logbasetime);
	} else if ((inputstate&ISBUTTON) != 0)
	{
		if ((inputstate&BUTTONUP) != 0)
		{
			xprintf(us_logrecord, x_("BR %ld %ld %ld %ld %ld %ld\n"),
				inputstate&(WHICHBUTTON|SHIFTISDOWN|ALTISDOWN|CONTROLISDOWN),
					cursorx, cursory, special, extradata, ticktime()-gra_logbasetime);
		} else
		{
			if ((inputstate&DOUBLECLICK) != 0)
			{
				xprintf(us_logrecord, x_("BD %ld %ld %ld %ld %ld %ld\n"),
					inputstate&(WHICHBUTTON|SHIFTISDOWN|ALTISDOWN|CONTROLISDOWN),
						cursorx, cursory, special, extradata, ticktime()-gra_logbasetime);
			} else
			{
				xprintf(us_logrecord, x_("BP %ld %ld %ld %ld %ld %ld\n"),
					inputstate&(WHICHBUTTON|SHIFTISDOWN|ALTISDOWN|CONTROLISDOWN),
						cursorx, cursory, special, extradata, ticktime()-gra_logbasetime);
			}
		}
	} else if (inputstate == MENUEVENT)
	{
		xprintf(us_logrecord, x_("ME %ld %ld %ld"), cursorx, cursory,
			ticktime()-gra_logbasetime);
		if (cursorx >= 0 && cursorx < gra_pulldownmenucount)
		{
			pm = us_getpopupmenu(gra_pulldowns[cursorx]);
			for(trueItem=j=0; j < pm->total; j++)
			{
				mi = &pm->list[j];
				if (mi->response->active < 0 && *mi->attribute == 0) continue;
				trueItem++;
				if (trueItem == cursory) break;
			}
			if (j < pm->total)
				xprintf(us_logrecord, x_("   ; command=%s"),
					us_stripampersand(mi->attribute));
		}
		xprintf(us_logrecord, x_("\n"));
	} else if (inputstate == FILEREPLY)
	{
		filename = (CHAR *)extradata;
		xprintf(us_logrecord, x_("FS %s\n"), filename);
	} else if (inputstate == POPUPSELECT)
	{
		xprintf(us_logrecord, x_("PM %ld %ld\n"), cursorx, ticktime()-gra_logbasetime);
	} else if (inputstate == DIAEDITTEXT)
	{
		filename = (CHAR *)extradata;
		xprintf(us_logrecord, x_("DE %ld %ld %s\n"), special, cursorx, filename);
	} else if (inputstate == DIASCROLLSEL)
	{
		xprintf(us_logrecord, x_("DS %ld %ld %ld"), special, cursorx, cursory);
		for(i=0; i<cursory; i++) xprintf(us_logrecord, x_(" %ld"), ((INTBIG *)extradata)[i]);
		xprintf(us_logrecord, x_("\n"));
	} else if (inputstate == DIAPOPUPSEL)
	{
		xprintf(us_logrecord, x_("DP %ld %ld %ld\n"), special, cursorx, cursory);
	} else if (inputstate == DIASETCONTROL)
	{
		xprintf(us_logrecord, x_("DC %ld %ld %ld\n"), special, cursorx, cursory);
	} else if (inputstate == DIAITEMCLICK)
	{
		xprintf(us_logrecord, x_("DI %ld %ld\n"), special, cursorx);
	} else if (inputstate == DIAUSERMOUSE)
	{
		xprintf(us_logrecord, x_("DM %ld %ld %ld\n"), special, cursorx, cursory);
	} else if (inputstate == DIAENDDIALOG)
	{
		xprintf(us_logrecord, x_("DD %ld\n"), special);
	} else if (inputstate == WINDOWMOVE)
	{
		xprintf(us_logrecord, x_("WM %ld %ld %ld %ld\n"), extradata,
			cursorx, cursory, ticktime()-gra_logbasetime);
	} else if (inputstate == WINDOWSIZE)
	{
		xprintf(us_logrecord, x_("WS %ld %ld %ld %ld\n"), extradata,
			cursorx, cursory, ticktime()-gra_logbasetime);
	} else
	{
		ttyputmsg(x_("Unknown event being logged: %ld"), inputstate);
	}

	/* flush the log file every so often */
	gra_logrecordcount++;
	if (gra_logrecordcount >= us_logflushfreq)
	{
		gra_logrecordcount = 0;
		xflushbuf(us_logrecord);
	}
}

void gra_logwritecomment(CHAR *comment)
{
	if (us_logrecord == 0) return;
	xprintf(us_logrecord, x_("; %s\n"), comment);
}

BOOLEAN gra_loggetnextaction(CHAR *message)
{
	REGISTER BOOLEAN eof;
	CHAR tempstring[300], *pt, *start;
	REGISTER TDIALOG *dia;
	REGISTER WINDOWFRAME *wf;
	INTSML boundkey;
	INTBIG sellist[MAXSCROLLMULTISELECT], i, x, y, item, count;
	UINTBIG nowtime;

	eof = gra_logreadline(tempstring, 300);
	if (stopping(STOPREASONPLAYBACK)) eof = TRUE;
	if (eof)
	{
		/* stop playback */
		ttyputmsg(_("End of session playback file"));
		xclose(us_logplay);
		us_logplay = NULL;
		return(TRUE);
	}

	/* load the event structure */
	if (estrncmp(tempstring, x_("WS"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		x = eatoi(getkeyword(&pt, x_(" ")));
		y = eatoi(getkeyword(&pt, x_(" ")));
		wf = gra_getframefromindex(eatoi(getkeyword(&pt, x_(" "))));
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
		if (wf != NOWINDOWFRAME)
			sizewindowframe(wf, x, y);
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("WM"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		x = eatoi(getkeyword(&pt, x_(" ")));
		y = eatoi(getkeyword(&pt, x_(" ")));
		wf = gra_getframefromindex(eatoi(getkeyword(&pt, x_(" "))));
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
		if (wf != NOWINDOWFRAME)
			movewindowframe(wf, x, y);
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DI"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		if (dia == NOTDIALOG) return(FALSE);
		item = eatoi(getkeyword(&pt, x_(" ")));
		if (dia->modelessitemhit == 0)
		{
			/* current modal dialog */
			dia->dialoghit = item;
		} else
		{
			/* a modeless dialog */
			(*dia->modelessitemhit)(dia, item);
		}
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DS"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		if (dia == NOTDIALOG) return(FALSE);
		item = eatoi(getkeyword(&pt, x_(" ")));
		count = eatoi(getkeyword(&pt, x_(" ")));
		for(i=0; i<count; i++) sellist[i] = eatoi(getkeyword(&pt, x_(" ")));
		if (count == 1)
		{
			DiaSelectLine(dia, item, sellist[0]);
		} else
		{
			DiaSelectLines(dia, item, count, sellist);
		}
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DP"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		if (dia == NOTDIALOG) return(FALSE);
		item = eatoi(getkeyword(&pt, x_(" ")));
		i = eatoi(getkeyword(&pt, x_(" ")));
		DiaSetPopupEntry(dia, item, i);
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DC"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		if (dia == NOTDIALOG) return(FALSE);
		item = eatoi(getkeyword(&pt, x_(" ")));
		i = eatoi(getkeyword(&pt, x_(" ")));
		DiaSetControl(dia, item, i);
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DE"), 2) == 0)
	{
		gra_inputstate = NOEVENT;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		if (dia == NOTDIALOG) return(FALSE);
		item = eatoi(getkeyword(&pt, x_(" ")));
		while (*pt != 0 && *pt != ' ') pt++;
		if (*pt == ' ') pt++;
		DiaSetText(dia, item, pt);
		if (dia->modelessitemhit == 0)
		{
			/* current modal dialog */
			dia->dialoghit = item;
		} else
		{
			/* a modeless dialog */
			(*dia->modelessitemhit)(dia, item);
		}
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DM"), 2) == 0)
	{
		gra_inputstate = DIAUSERMOUSE;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		if (dia == NOTDIALOG) return(FALSE);
		gra_action.x = eatoi(getkeyword(&pt, x_(" ")));
		gra_action.y = eatoi(getkeyword(&pt, x_(" ")));
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("DD"), 2) == 0)
	{
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
		gra_inputstate = DIAENDDIALOG;
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("ME"), 2) == 0)
	{
		gra_inputstate = MENUEVENT;
		pt = &tempstring[3];
		gra_cursorx = eatoi(getkeyword(&pt, x_(" ")));
		gra_cursory = eatoi(getkeyword(&pt, x_(" ")));
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("FS"), 2) == 0)
	{
		gra_inputstate = FILEREPLY;
		pt = &tempstring[3];
		if (message != 0) estrcpy(message, pt);
		return(FALSE);
	}
	if (estrncmp(tempstring, x_("PM"), 2) == 0)
	{
		gra_inputstate = POPUPSELECT;
		pt = &tempstring[3];
		gra_cursorx = eatoi(getkeyword(&pt, x_(" ")));
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
		return(FALSE);
	}

	if (estrncmp(tempstring, x_("MR"), 2) == 0 || estrncmp(tempstring, x_("MP"), 2) == 0)
	{
		pt = &tempstring[3];
		gra_inputstate = MOTION;
		gra_cursorx = eatoi(getkeyword(&pt, x_(" ")));
		gra_cursory = eatoi(getkeyword(&pt, x_(" ")));
		gra_inputspecial = eatoi(getkeyword(&pt, x_(" ")));
		i = eatoi(getkeyword(&pt, x_(" ")));
		if (tempstring[1] == 'R') gra_inputstate |= BUTTONUP;
	} else if (estrncmp(tempstring, x_("KT"), 2) == 0)
	{
		start = &tempstring[4];
		for(i = estrlen(start)-1; i>0; i--)
			if (start[i] == '\'') break;
		if (i <= 0) return(FALSE);
		start[i] = 0;
		pt = &start[i+2];
		(void)us_getboundkey(start, &boundkey, &gra_inputspecial);
		gra_inputstate = ISKEYSTROKE | (boundkey & CHARREAD);
		gra_cursorx = eatoi(getkeyword(&pt, x_(" ")));
		gra_cursory = eatoi(getkeyword(&pt, x_(" ")));
		i = eatoi(getkeyword(&pt, x_(" ")));
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
	} else if (tempstring[0] == 'B')
	{
		pt = &tempstring[3];
		gra_inputstate = ISBUTTON | eatoi(getkeyword(&pt, x_(" ")));
		gra_cursorx = eatoi(getkeyword(&pt, x_(" ")));
		gra_cursory = eatoi(getkeyword(&pt, x_(" ")));
		gra_inputspecial = eatoi(getkeyword(&pt, x_(" ")));
		i = eatoi(getkeyword(&pt, x_(" ")));
		if (tempstring[1] == 'D') gra_inputstate |= DOUBLECLICK;
		if (tempstring[1] == 'R') gra_inputstate |= BUTTONUP;
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
	}
	wf = gra_getframefromindex(i);
	if (wf != NOWINDOWFRAME && wf != el_curwindowframe)
		((CChildFrame *)wf->wndframe)->ActivateFrame();
	us_state &= ~GOTXY;
	return(FALSE);
}

BOOLEAN gra_logreadline(CHAR *string, INTBIG limit)
{
	for(;;)
	{
		if (xfgets(string, limit, us_logplay) != 0) return(TRUE);
		if (string[0] != ';') break;
	}
	return(FALSE);
}

WINDOWFRAME *gra_getframefromindex(INTBIG index)
{
	REGISTER WINDOWFRAME *wf;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (wf->windindex == index) return(wf);
	return(NOWINDOWFRAME);
}

TDIALOG *gra_getdialogfromindex(INTBIG index)
{
	REGISTER TDIALOG *dia;

	for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
		if (dia->windindex == index) return(dia);
	return(NOTDIALOG);
}

/****************************** MENUS ******************************/

void getacceleratorstrings(CHAR **acceleratorstring, CHAR **acceleratorprefix)
{
	*acceleratorstring = _("Ctrl");
	*acceleratorprefix = _("Ctrl-");
}

CHAR *getinterruptkey(void)
{
	return(_("Windows-C"));
}

INTBIG nativepopupmenu(POPUPMENU **menu, BOOLEAN header, INTBIG left, INTBIG top)
{
	INTBIG retval;

	retval = gra_nativepopuptif(menu, header, left, top);
	gra_logwriteaction(POPUPSELECT, 0, retval, 0, 0);
	return(retval);
}

INTBIG gra_nativepopuptif(POPUPMENU **menu, BOOLEAN header, INTBIG left, INTBIG top)
{
	CMainFrame *wnd;
	INTBIG j, k, pindex, submenus;
	REGISTER POPUPMENUITEM *mi, *submi;
	REGISTER POPUPMENU *themenu, **subpmlist;
	REGISTER USERCOM *uc;
	HMENU popmenu, subpopmenu, *submenulist;
	POINT p, p2;
	UINT flags;

	while (us_logplay != NULL)
	{
		if (gra_loggetnextaction(0)) break;
		j = gra_inputstate;
		gra_inputstate = NOEVENT;
		if (j == POPUPSELECT) return(gra_cursorx);
	}

	themenu = *menu;
	wnd = (CMainFrame *)AfxGetMainWnd();
	flags = TPM_NONOTIFY | TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON;
	if (left < 0 || top < 0)
	{
		p2.x = p2.y = 0;
		wnd->MapWindowPoints(0, &p2, 1);
		GetCursorPos(&p);
		left = p.x - p2.x;
		top = p.y - p2.y;
		flags |= TPM_CENTERALIGN|TPM_VCENTERALIGN;
	} else
	{
		if (el_curwindowpart != NOWINDOWPART)
			top = el_curwindowpart->frame->revy - top;
		flags |= TPM_LEFTALIGN;
	}
	popmenu = CreatePopupMenu();
	if (popmenu == 0) return(-1);
	if (header)
	{
		if (AppendMenu(popmenu, MF_STRING, 0, themenu->header) == 0) return(-1);
		if (AppendMenu(popmenu, MF_SEPARATOR, 0, 0) == 0) return(-1);
	}

	/* count the number of submenus */
	submenus = 0;
	for(j=0; j < themenu->total; j++)
	{
		mi = &themenu->list[j];
		if (*mi->attribute != 0 && mi->response != NOUSERCOM &&
			mi->response->menu != NOPOPUPMENU) submenus++;
	}
	if (submenus > 0)
	{
		submenulist = (HMENU *)emalloc(submenus * (sizeof (HMENU)), us_tool->cluster);
		if (submenulist == 0) return(-1);
		subpmlist = (POPUPMENU **)emalloc(submenus * (sizeof (POPUPMENU *)), us_tool->cluster);
		if (subpmlist == 0) return(-1);
	}

	/* load the menus */
	submenus = 0;
	for(j=0; j < themenu->total; j++)
	{
		mi = &themenu->list[j];
		mi->changed = FALSE;
		if (*mi->attribute == 0)
		{
			if (AppendMenu(popmenu, MF_SEPARATOR, 0, 0) == 0) return(-1);
		} else
		{
			uc = mi->response;
			if (uc != NOUSERCOM && uc->menu != NOPOPUPMENU)
			{
				subpopmenu = CreatePopupMenu();
				if (subpopmenu == 0) return(-1);
				submenulist[submenus] = subpopmenu;
				subpmlist[submenus] = uc->menu;
				submenus++;

				for(k=0; k < uc->menu->total; k++)
				{
					submi = &uc->menu->list[k];
					if (*submi->attribute == 0)
					{
						if (AppendMenu(subpopmenu, MF_SEPARATOR, 0, 0) == 0) return(-1);
					} else
					{
						if (AppendMenu(subpopmenu, MF_STRING, (submenus << 16) | (k+1), submi->attribute) == 0) return(-1);
					}
				}
				if (InsertMenu(popmenu, j+2, MF_POPUP | MF_BYPOSITION,
					(DWORD)subpopmenu, mi->attribute) == 0)
						return(0);
			} else
			{
				if (AppendMenu(popmenu, MF_STRING, j+1, mi->attribute) == 0) return(-1);
			}
		}
	}
	pindex = TrackPopupMenu(popmenu, flags, left, top, 0, wnd->m_hWnd, 0) - 1;
	if (pindex < 0) return(-1);
	j = pindex >> 16;
	if (j != 0) *menu = subpmlist[j-1];
 	DestroyMenu(popmenu);
	for(j=0; j<submenus; j++)
		DestroyMenu(submenulist[j]);
	if (submenus > 0)
	{
		efree((CHAR *)submenulist);
		efree((CHAR *)subpmlist);
	}
	return(pindex & 0xFFFF);
}

/*
 * routine to handle the menu
 */
void gra_nativemenudoone(INTBIG item, INTBIG menuindex)
{
	gra_addeventtoqueue(MENUEVENT, 0, menuindex, item);
}

/*
 * Routine to establish the "count" pulldown menu names in "par" as the pulldown menu bar
 */
void nativemenuload(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, menuindex;
	REGISTER POPUPMENU *pm;
	POPUPMENU *pulls[25];

	if (gra_hMenu == 0)
	{
		gra_hMenu = new CMenu();
		gra_hMenu->CreateMenu();
	}

	for(i=0; i<count; i++)
	{
		pm = us_getpopupmenu(par[i]);
		if (pm == NOPOPUPMENU) continue;
		pulls[i] = pm;

		menuindex = gra_pulldownindex(pm);
		if (menuindex < 0) continue;
		if (gra_hMenu->InsertMenu(menuindex, MF_ENABLED | MF_POPUP | MF_BYPOSITION,
			(DWORD)gra_pulldownmenus[menuindex]->m_hMenu, pm->header) == 0)
		{
			ttyputerr(_("Item %s can't be inserted in menu bar"), par[i]);
			return;
		}
	}
	CWnd* parent = AfxGetMainWnd();
	CMenu* lastMenu = parent->GetMenu();
	if (parent->SetMenu(gra_hMenu) == 0)
	{
		ttyputerr(_("SetMenu failed"));
		return;
	}
	gra_hMenu->Detach();
	parent->DrawMenuBar();
}

/*
 * Routine to create a pulldown menu from popup menu "pm".  Returns an index to
 * the table of pulldown menus (-1 on error).
 */
INTBIG gra_pulldownindex(POPUPMENU *pm)
{
	REGISTER INTBIG i, pindex;
	CMenu **newpulldownmenus;
	CHAR **newpulldowns;

	for(i=0; i<gra_pulldownmenucount; i++)
		if (namesame(gra_pulldowns[i], pm->name) == 0) return(i);

	/* allocate new space with one more */
	newpulldownmenus = (CMenu **)emalloc((gra_pulldownmenucount+1) *
		(sizeof (CMenu *)), us_tool->cluster);
	if (newpulldownmenus == 0) return(-1);
	newpulldowns = (CHAR **)emalloc((gra_pulldownmenucount+1) *
		(sizeof (CHAR *)), us_tool->cluster);
	if (newpulldowns == 0) return(-1);

	/* copy former arrays then delete them */
	for(i=0; i<gra_pulldownmenucount; i++)
	{
		newpulldownmenus[i] = gra_pulldownmenus[i];
		newpulldowns[i] = gra_pulldowns[i];
	}
	if (gra_pulldownmenucount != 0)
	{
		efree((CHAR *)gra_pulldownmenus);
		efree((CHAR *)gra_pulldowns);
	}

	gra_pulldownmenus = newpulldownmenus;
	gra_pulldowns = newpulldowns;

	pindex = gra_pulldownmenucount++;
	(void)allocstring(&gra_pulldowns[pindex], pm->name, us_tool->cluster);
	gra_pulldownmenus[pindex] = gra_makepdmenu(pm);
	if (gra_pulldownmenus[pindex] == 0) return(-1);
	return(pindex);
}

CMenu *gra_makepdmenu(POPUPMENU *pm)
{
	CMenu *hPrevMenu, *hMenu;
	REGISTER USERCOM *uc;
	REGISTER POPUPMENUITEM *mi;
	REGISTER INTBIG j, submenuindex, len, idIndex;
	INTSML key;
	INTBIG special;
	REGISTER INTBIG flags;
	REGISTER CHAR *pt;
	CHAR myline[100];

	hMenu = new CMenu();
	if (hMenu->CreatePopupMenu() == 0) return(0);

	idIndex = 0;
	for(j=0; j < pm->total; j++)
	{
		mi = &pm->list[j];
		uc = mi->response;
		if (uc->active < 0)
		{
			if (*mi->attribute == 0)
			{
				if (hMenu->AppendMenu(MF_SEPARATOR) == 0) return(0);
			} else
			{
				if (hMenu->AppendMenu(MF_STRING | MF_DISABLED, gra_menures[idIndex++], mi->attribute) == 0) return(0);
			}
			continue;
		}

		if (uc->menu != NOPOPUPMENU)
		{
			hPrevMenu = hMenu;
			idIndex++;
			submenuindex = gra_pulldownindex(uc->menu);
			if (hPrevMenu->InsertMenu(-1, MF_POPUP | MF_BYPOSITION,
				(DWORD)gra_pulldownmenus[submenuindex]->m_hMenu, mi->attribute) == 0)
					return(0);
			continue;
		}
		flags = 0;
		if (mi->attribute[0] == '>')
		{
			flags = MF_CHECKED;
			estrcpy(myline, &mi->attribute[1]);
		} else
		{
			estrcpy(myline, mi->attribute);
		}
		len = estrlen(myline);
		if (myline[len-1] == '<') myline[len-1] = 0;
		for(pt = myline; *pt != 0; pt++) if (*pt == '/' || *pt == '\\') break;
		if (*pt != 0)
		{
			(void)us_getboundkey(pt, &key, &special);
			esnprintf(pt, 50, x_("\t%s"), us_describeboundkey(key, special, 1));
		}
		if (hMenu->AppendMenu(MF_STRING|flags, gra_menures[idIndex++], myline) == 0) return(0);
	}
	return(hMenu);
}

/* routine to redraw entry "pindex" of popupmenu "pm" because it changed */
void nativemenurename(POPUPMENU *pm, INTBIG pindex)
{
	INTBIG i, j, k, len, key, idIndex;
	REGISTER INTBIG flags;
	CHAR line[100], *pt, slash;
	USERCOM *uc;
	REGISTER POPUPMENUITEM *mi;

	/* find this pulldown menu */
	for(i=0; i<gra_pulldownmenucount; i++)
		if (namesame(gra_pulldowns[i], pm->name) == 0) break;
	if (i >= gra_pulldownmenucount) return;

	/* make sure the menu didn't change size */
	j = (INTBIG)gra_pulldownmenus[i]->GetMenuItemCount();
	if (pm->total != j)
	{
		if (pm->total > j)
		{
			/* must add new entries */
			for(k=j; k<pm->total; k++)
				gra_pulldownmenus[i]->AppendMenu(MF_STRING, gra_menures[k], x_("X"));
		} else
		{
			/* must delete extra entries */
			for(k=pm->total; k<j; k++)
				gra_pulldownmenus[i]->RemoveMenu(pm->total, MF_BYPOSITION);
		}
	}

	idIndex = 0;
	for(j=0; j < pm->total; j++)
	{
		mi = &pm->list[j];
		uc = mi->response;
		if (j == pindex) break;
		if (uc->active < 0 && *mi->attribute == 0) continue;
		idIndex++;
	}
	if (uc->active < 0)
	{
		if (*pm->list[pindex].attribute == 0)
		{
			gra_pulldownmenus[i]->ModifyMenu(pindex, MF_BYPOSITION|MF_SEPARATOR);
			return;
		} else
		{
			gra_pulldownmenus[i]->EnableMenuItem(pindex, MF_BYPOSITION|MF_GRAYED);
		}
	} else
	{
		gra_pulldownmenus[i]->EnableMenuItem(pindex, MF_BYPOSITION|MF_ENABLED);
	}

	/* copy and examine the menu string */
	flags = 0;
	if (pm->list[pindex].attribute[0] == '>')
	{
		flags = MF_CHECKED;
		(void)estrcpy(line, &pm->list[pindex].attribute[1]);
	} else
	{
		(void)estrcpy(line, pm->list[pindex].attribute);
	}
	len = estrlen(line);
	if (line[len-1] == '<') line[len-1] = 0;

	/* handle single-key equivalents */
	for(pt = line; *pt != 0; pt++) if (*pt == '/' || *pt == '\\') break;
	if (*pt != 0)
	{
		slash = *pt;
		*pt++ = 0;
		len = estrlen(line);
		if (*pt != 0)
		{
			if (estrlen(pt) > 1)
			{
				esnprintf(&line[len], 50, x_("\t%s"), pt);
			} else
			{
				key = *pt & 0xFF;
				if (slash == '/')
				{
					esnprintf(&line[len], 50, x_("\tCtrl-%c"), key);
				} else
				{
					esnprintf(&line[len], 50, x_("\t%c"), key);
				}
			}
		}
	}
	gra_pulldownmenus[i]->ModifyMenu(pindex, MF_BYPOSITION|MF_STRING|flags, gra_menures[idIndex], line);
}

/****************************** DIALOGS ******************************/

/*
 * Routine to initialize a dialog described by "dialog".
 * Returns the address of the dialog object (0 if dialog cannot be initialized).
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

/* Internal routine to initialize dialog structure "dialog" into "dia" */
BOOLEAN gra_initdialog(DIALOG *dialog, TDIALOG *dia, BOOLEAN modeless)
{
	DLGTEMPLATE *dtheader;
	WORD *dtmenu, *dtclass, *dttitle;
	int headerlength, menulength, classlength, titlelength, itemslength,
		*itemlengths, i, j, len, itemtype, style, itemclass, totallen,
		dialogdbnx, dialogdbny, dialogdbux, dialogdbuy;
	INTBIG dbu, dbuL, dbuH;
	DLGITEMTEMPLATE **dtitems;
	RECT r;
	UCHAR1 *descblock, *pt;
	CHAR *title, *msg;
	WORD output[100];
	wchar_t *woutput;
	CProgressCtrl *prog;
	CListBoxEx *list;
	CStatic *stat;
	CWnd *wnd;
	CSize textSize;
	REGISTER void *infstr;
	HFONT hf;

	/* add this to the list of active dialogs */
	dia->nexttdialog = gra_firstactivedialog;
	gra_firstactivedialog = dia;

	/* be sure the dialog is translated */
	DiaTranslate(dialog);

	/*
	 * for small fonts, dbuH = 16, dbuL = 8,
	 * for large fonts, dbuH = 20, dbuL = 10
	 */
	dbu = GetDialogBaseUnits();
	dbuL = dbu & 0xFFFF;
	dbuH = (dbu >> 16) & 0xFFFF;
	dialogdbnx = 2 * (46-dbuL)-1;   dialogdbux = 14 * dbuL;
	dialogdbny = 131 * 8;           dialogdbuy = 105 * dbuH;

	dia->window = new CElectricDialog();
	dia->itemdesc = dialog;
	dia->windindex = gra_windownumberindex++;
	dia->numlocks = 0;
	dia->modelessitemhit = 0;
	dia->redrawroutine = 0;

	/* compute size of some items to automatically scale them */
#ifdef INTERNATIONAL
	CDC *dc;
	CFont *cf;
	hf = (HFONT)GetStockObject(SYSTEM_FONT);
	cf = CFont::FromHandle(hf);
	dc = AfxGetApp()->m_pMainWnd->GetDC();
	dc->SelectObject(cf);
	for(i=0; i<dialog->items; i++)
	{
		itemtype = dialog->list[i].type & ITEMTYPE;
		switch (itemtype)
		{
			case DEFBUTTON:
			case BUTTON:
			case CHECK:
			case RADIO:
			case MESSAGE:
				int offset, amt;
				if (itemtype == BUTTON || itemtype == DEFBUTTON) offset = 8; else
					if (itemtype == CHECK || itemtype == RADIO) offset = 16; else
						offset = 0;
				pt = dialog->list[i].msg;
				textSize = dc->GetTextExtent(pt, estrlen(pt));
				j = textSize.cx * dialogdbnx / dialogdbux;
				amt = (j + offset) - (dialog->list[i].r.right - dialog->list[i].r.left);
				if (amt > 0)
				{
					for(j=0; j<dialog->items; j++)
					{
						if (j == i) continue;
						if (dialog->list[j].r.left >= dialog->list[i].r.right &&
							dialog->list[j].r.left < dialog->list[i].r.right+amt)
						{
							dialog->list[j].r.left += amt;
							dialog->list[j].r.right += amt;
						}
					}
					dialog->list[i].r.right += amt;
				}
				break;
		}
	}
	for(i=0; i<dialog->items; i++)
	{
		j = dialog->list[i].r.right + 5;
		if (j < dialog->windowRect.right - dialog->windowRect.left) continue;
		dialog->windowRect.right = dialog->windowRect.left + j;
	}
	AfxGetApp()->m_pMainWnd->ReleaseDC(dc);
#endif

	/* determine item lengths */
	itemlengths = (int *)emalloc(dialog->items * (sizeof (int)), us_tool->cluster);
	if (itemlengths == 0) return(TRUE);
	dtitems = (DLGITEMTEMPLATE **)emalloc(dialog->items * (sizeof (DLGITEMTEMPLATE *)), us_tool->cluster);
	if (dtitems == 0) return(TRUE);
	itemslength = 0;
	for(i=0; i<dialog->items; i++)
	{
		itemlengths[i] = (sizeof DLGITEMTEMPLATE) + 3 * (sizeof (WORD));
		itemtype = dialog->list[i].type;
		switch (itemtype&ITEMTYPE)
		{
			case BUTTON:
			case DEFBUTTON:
			case CHECK:
			case RADIO:
			case EDITTEXT:
			case MESSAGE:
				itemlengths[i] += (estrlen(dialog->list[i].msg) + 1) * (sizeof (WORD));
				break;
			default:
				itemlengths[i] += 1 * (sizeof (WORD));
				break;
		}
		itemlengths[i] = (itemlengths[i] + 3) & ~3;
		itemslength += itemlengths[i];
	}

	/* allocate space for entire dialog template */
	headerlength = sizeof (DLGTEMPLATE);
	menulength = sizeof (WORD);
	classlength = sizeof (WORD);
	if (dialog->movable != 0) title = dialog->movable; else
		title = x_("");
	titlelength = (estrlen(title) + 1) * (sizeof (WORD));
	i = headerlength + menulength + classlength + titlelength;
	totallen = (i + 3) & ~3;
	descblock = (UCHAR1 *)emalloc(totallen + itemslength, us_tool->cluster);
	if (descblock == 0) return(TRUE);
	pt = descblock;
	dtheader = (DLGTEMPLATE *)pt;  pt += headerlength;
	dtmenu = (WORD *)pt;           pt += menulength;
	dtclass = (WORD *)pt;          pt += classlength;
	dttitle = (WORD *)pt;          pt += titlelength;
	for(i=0; i<dialog->items; i++)
	{
		pt = (UCHAR1 *)(((DWORD)pt + 3) & ~3);
		dtitems[i] = (DLGITEMTEMPLATE *)pt;
		pt += itemlengths[i];
	}

	/* load dialog template header */
	dtheader->style = WS_VISIBLE | WS_DLGFRAME | WS_POPUP /* | DS_ABSALIGN */;
	if (dialog->movable != 0) dtheader->style |= WS_CAPTION | DS_MODALFRAME;
	dtheader->dwExtendedStyle = WS_EX_CONTROLPARENT;
	dtheader->cdit = (unsigned short)dialog->items;
	dtheader->x = dialog->windowRect.left;
	dtheader->y = dialog->windowRect.top;
	dtheader->cx = (dialog->windowRect.right - dialog->windowRect.left) * dialogdbnx / dialogdbux;
	dtheader->cy = (dialog->windowRect.bottom - dialog->windowRect.top) * dialogdbny / dialogdbuy;

	/* no menu or class in this dialog */
	dtmenu[0] = 0;
	dtclass[0] = 0;

	/* load the dialog title */
	woutput = (wchar_t *)output;
#ifdef _UNICODE
	len = estrlen(title);
	for(j=0; j<len; j++) *dttitle++ = title[j];
#else
	len = MultiByteToWideChar(CP_ACP, 0, title, estrlen(title), woutput, 100);
	for(j=0; j<len; j++) *dttitle++ = output[j];
#endif
	*dttitle++ = 0;

	/* find the default button */
	dia->defaultbutton = 1;
	for(i=0; i<dialog->items; i++)
	{
		itemtype = dialog->list[i].type;
		if ((itemtype&ITEMTYPE) == DEFBUTTON) dia->defaultbutton = i+1;
	}

	/* load the items */
	for(i=0; i<dialog->items; i++)
	{
		dtitems[i]->x = dialog->list[i].r.left * dialogdbnx / dialogdbux;
		dtitems[i]->y = dialog->list[i].r.top * dialogdbny / dialogdbuy;
		dtitems[i]->cx = (dialog->list[i].r.right - dialog->list[i].r.left) * dialogdbnx / dialogdbux;
		dtitems[i]->cy = (dialog->list[i].r.bottom - dialog->list[i].r.top) * dialogdbny / dialogdbuy;
		dtitems[i]->dwExtendedStyle = 0;
		dtitems[i]->id = i+ID_DIALOGITEM_0;
		itemtype = dialog->list[i].type;
		switch (itemtype&ITEMTYPE)
		{
			case BUTTON:
			case DEFBUTTON:
				itemclass = 0x80;
				style = WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP;
				if (i+1 == dia->defaultbutton) style |= BS_DEFPUSHBUTTON;
				break;
			case CHECK:
				itemclass = 0x80;
				style = WS_VISIBLE | WS_CHILD | BS_CHECKBOX | WS_TABSTOP;
				break;
			case RADIO:
				itemclass = 0x80;
				style = WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON | WS_TABSTOP;
				break;
			case EDITTEXT:
				itemclass = 0x81;
				style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | WS_TABSTOP;
				if (dtitems[i]->cy > 10) style |= ES_MULTILINE;
				dtitems[i]->y--;
				dtitems[i]->cy += 2;
				break;
			case MESSAGE:
				itemclass = 0x82;
				style = WS_CHILD | WS_VISIBLE | SS_LEFT;
				if ((itemtype&INACTIVE) == 0) style |= SS_NOTIFY;
				dtitems[i]->y--;
				dtitems[i]->cy += 2;
				break;
			case PROGRESS:
			case SCROLL:
			case SCROLLMULTI:
				/* dummy: create it for real later */
				itemclass = 0x82;
				style = WS_CHILD;
				dtitems[i]->id += PROGRESSOFFSET;
				break;
			case USERDRAWN:
				itemclass = 0x82;
				style = WS_CHILD | WS_VISIBLE;
				dtitems[i]->x += 1000;
				dtitems[i]->y += 1000;
				break;
			case POPUP:
				itemclass = 0x85;
				dtitems[i]->cy *= 8;
				dtitems[i]->y -= 2;
				style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST | WS_TABSTOP;
				break;
			case ICON:
				itemclass = 0x82;
				style = WS_CHILD | WS_VISIBLE;
				dtitems[i]->x += 1000;
				dtitems[i]->y += 1000;
				if ((itemtype&INACTIVE) == 0) style |= SS_NOTIFY;
				break;
			default:
				itemclass = 0x82;
				style = WS_CHILD | SS_LEFT;
				break;
		}
		dtitems[i]->style = style;
		pt = ((UCHAR1 *)dtitems[i]) + (sizeof (DLGITEMTEMPLATE));
		dtclass = (WORD *)pt;
		*dtclass++ = 0xFFFF;
		*dtclass++ = itemclass;
		switch (itemtype&ITEMTYPE)
		{
			case BUTTON:
			case DEFBUTTON:
			case CHECK:
			case RADIO:
			case EDITTEXT:
			case MESSAGE:
				msg = dialog->list[i].msg;
				woutput = (wchar_t *)output;
#ifdef _UNICODE
				len = estrlen(msg);
				for(j=0; j<len; j++) *dtclass++ = msg[j];
#else
				len = MultiByteToWideChar(CP_ACP, 0, msg, estrlen(msg), woutput, 100);
				for(j=0; j<len; j++) *dtclass++ = output[j];
#endif
				*dtclass++ = 0;
				break;
			default:
				*dtclass++ = 0;
				break;
		}
		*dtclass++ = 0;
	}

	/* create the dialog */
	if (dia->window->CreateIndirect(dtheader) == 0)
	{
		return(TRUE);
	}

	/* finish initialization */
	wnd = 0;
	for(i=0; i<dialog->items; i++)
	{
		itemtype = dialog->list[i].type;
		switch (itemtype&ITEMTYPE)
		{
			case EDITTEXT:
				if (wnd == 0)
					wnd = dia->window->GetDlgItem(i+ID_DIALOGITEM_0);
				(void)allocstring((CHAR **)&dialog->list[i].data, dialog->list[i].msg, el_tempcluster);
				break;
			case PROGRESS:
				stat = (CStatic *)dia->window->GetDlgItem(i+ID_DIALOGITEM_0+PROGRESSOFFSET);
				stat->GetWindowRect(&r);
				dia->window->ScreenToClient(&r);
				prog = new CProgressCtrl();
				prog->Create(WS_CHILD | WS_VISIBLE, r, dia->window, i+ID_DIALOGITEM_0);
				prog->ModifyStyleEx(0, WS_EX_CLIENTEDGE);	/* makes it look 3D */
				break;
			case SCROLL:
			case SCROLLMULTI:
				r.left = dialog->list[i].r.left;
				r.top = dialog->list[i].r.top;
				r.right = dialog->list[i].r.right;
				r.bottom = dialog->list[i].r.bottom;
				style = WS_BORDER | WS_CHILD | WS_VISIBLE | LBS_USETABSTOPS |
					WS_VSCROLL | WS_HSCROLL | LBS_WANTKEYBOARDINPUT | LBS_SORT | WS_TABSTOP;
				if ((itemtype&ITEMTYPE) == SCROLLMULTI)
					style |= LBS_EXTENDEDSEL;
				if ((itemtype&INACTIVE) == 0) style |= LBS_NOTIFY;
				list = new CListBoxEx();
				list->vdia = dia;
				list->Create(style, r, dia->window, i+ID_DIALOGITEM_0);
				list->ModifyStyleEx(0, WS_EX_CLIENTEDGE);	/* makes it look 3D */
				hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
				list->SetFont(CFont::FromHandle(hf));
				list->SetTabStops(1);
				break;
			case ICON:
				dialog->list[i].data = (INTBIG)dialog->list[i].msg;
				break;
		}
	}
	if (wnd == 0)
		wnd = dia->window->GetDlgItem(dia->defaultbutton-1+ID_DIALOGITEM_0);
	dia->window->GotoDlgCtrl(wnd);
	dia->window->SetDefID(dia->defaultbutton-1+ID_DIALOGITEM_0);

	/* determine location of dialog in screen coordinates */
	dia->window->GetWindowRect(&r);

	/*
	 * For some reason, the dialog doesn't always appear where we want it.
	 * Probably because it is forced to be placed relative to the previous dialog,
	 * rather than at an absolute location, as we want.
	 *
	 * The hack is to move the dialog after creating it.  This causes a side-effect
	 * when the mouse is set to "snap to cursor": the snapping happens too soon and
	 * the cursor is in the wrong place.  So we must move the cursor too.
	 */
	if (r.left != dtheader->x || r.top != dtheader->y)
	{
		BOOL res;
		INTBIG dx, dy;
		POINT cp;

		dia->window->SetWindowPos(0, dtheader->x, dtheader->y, 0, 0,
			SWP_NOZORDER|SWP_NOSIZE);
		dx = dtheader->x - r.left;
		dy = dtheader->y - r.top;
		dia->window->GetWindowRect(&r);

		/* see if mouse is set to "snap to cursor" */
		SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0, &res, 0);
		if (res)
		{
			/* mouse snapping on: adjust position */
			GetCursorPos(&cp);
			SetCursorPos(cp.x + dx, cp.y + dy);
		}
	}

	dia->firstpoint.x = r.left;
	dia->firstpoint.y = r.top;

	dia->useritemdoubleclick = 0;
	dia->usertextsize = 8;

	efree((CHAR *)itemlengths);
	efree((CHAR *)dtitems);
	efree((CHAR *)descblock);

	if (*title == 0) title = _("UNTITLED");
	infstr = initinfstr();
	formatinfstr(infstr, _("Start dialog %s"), title);
	gra_logwritecomment(returninfstr(infstr));
	dia->dialoghit = -1;
	return(FALSE);
}

/*
 * Routine to handle actions and return the next item hit.
 */
INTBIG DiaNextHit(void *vdia)
{
	INTBIG item;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	for(;;)
	{
		flushscreen();
		gra_nextevent();
		if (dia->dialoghit != -1)
		{
			item = dia->dialoghit;
			dia->dialoghit = -1;
			return(item);
		}
	}
}

void DiaDoneDialog(void *vdia)
{
	REGISTER INTBIG dx, dy, i, itemtype;
	RECT rect;
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

	/* if playing back, search for end-dialog marker */
	while (us_logplay != NULL)
	{
		if (gra_loggetnextaction(0)) break;
		if (gra_inputstate == DIAENDDIALOG) break;
		gra_inputstate = NOEVENT;
	}
	gra_inputstate = NOEVENT;
	gra_logwriteaction(DIAENDDIALOG, dia->windindex, 0, 0, 0);

	/* free memory used by edit text fields */
	for(i=0; i<dia->itemdesc->items; i++)
	{
		itemtype = dia->itemdesc->list[i].type;
		if ((itemtype&ITEMTYPE) == EDITTEXT)
			efree((CHAR *)dia->itemdesc->list[i].data);
	}

	/* update dialog location if it was moved */
	dia->window->GetWindowRect(&rect);
	dx = rect.left - dia->firstpoint.x;
	dy = rect.top - dia->firstpoint.y;
	dia->itemdesc->windowRect.left += (INTSML)dx;
	dia->itemdesc->windowRect.right += (INTSML)dx;
	dia->itemdesc->windowRect.top += (INTSML)dy;
	dia->itemdesc->windowRect.bottom += (INTSML)dy;

	dia->window->EndDialog(0);

	/* free the "dia" structure */
	efree((CHAR *)dia);
}

/*
 * Routine to change the size of the dialog
 */
void DiaResizeDialog(void *vdia, INTBIG wid, INTBIG hei)
{
	TDIALOG *dia;
	RECT rect;
#define WINDOWPADWIDTH        7
#define WINDOWPADHEIGHT      26

	dia = (TDIALOG *)vdia;
	dia->window->GetWindowRect(&rect);
	dia->window->SetWindowPos(NULL, rect.left, rect.top, wid+WINDOWPADWIDTH, hei+WINDOWPADHEIGHT,
		SWP_NOMOVE | SWP_NOZORDER);
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
	INTBIG type, highlight, len;
	REGISTER CHAR *pt;
	CWnd *wnd;
	CEdit *edit;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	highlight = 0;
	if (item < 0)
	{
		item = -item;
		highlight = 1;
	}
	item--;
	for(pt = msg; *pt != 0; pt++) if (estrncmp(pt, x_("(c)"), 3) == 0)
	{
		(void)estrcpy(pt, x_("©"));		/* "copyright" character */
		(void)estrcpy(&pt[1], &pt[3]);
		break;
	}
	wnd = dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (wnd == 0) return;
	wnd->SetWindowText(msg);
	type = dia->itemdesc->list[item].type;
	if ((type&ITEMTYPE) == EDITTEXT)
	{
		edit = (CEdit *)wnd;
		len = estrlen(msg);
		if (highlight == 0) edit->SetSel(len, len, FALSE); else
		{
			edit->SetSel(0, len, FALSE);
			dia->window->GotoDlgCtrl(wnd);
		}	
		(void)reallocstring((CHAR **)&dia->itemdesc->list[item].data, msg, el_tempcluster);
	}
}

/*
 * Routine to return the text in item "item"
 */
CHAR *DiaGetText(void *vdia, INTBIG item)
{
	static int bufnum = 0;
	static CHAR line[10][300];
	CWnd *wnd;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	wnd = dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (wnd == 0) return(x_(""));
	bufnum++;
	if (bufnum >= 10) bufnum = 0;
	wnd->GetWindowText(line[bufnum], 300);
	return(line[bufnum]);
}

/*
 * Routine to set the value in item "item" to "value"
 */
void DiaSetControl(void *vdia, INTBIG item, INTBIG value)
{
	CButton *but;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	but = (CButton *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (but == 0) return;
	but->SetCheck(value);
	gra_logwriteaction(DIASETCONTROL, dia->windindex, item + 1, value, 0);
}

/*
 * Routine to return the value in item "item"
 */
INTBIG DiaGetControl(void *vdia, INTBIG item)
{
	CButton *but;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	but = (CButton *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (but == 0) return(0);
	return(but->GetCheck());
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
	CWnd *wnd, *focus;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	wnd = dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	focus = CWnd::GetFocus();
	if (focus == wnd)
		dia->window->NextDlgCtrl();
	wnd->EnableWindow(0);
}

/*
 * Routine to un-dim item "item"
 */
void DiaUnDimItem(void *vdia, INTBIG item)
{
	CWnd *wnd;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	wnd = dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	wnd->EnableWindow(1);
}

/*
 * Routine to change item "item" to be a message rather
 * than editable text
 */
void DiaNoEditControl(void *vdia, INTBIG item)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	DiaDimItem(dia, item);
}

/*
 * Routine to change item "item" to be editable text rather
 * than a message
 */
void DiaEditControl(void *vdia, INTBIG item)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	DiaUnDimItem(dia, item);
}

void DiaOpaqueEdit(void *vdia, INTBIG item)
{
	CEdit *edit;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	edit = (CEdit *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	edit->ModifyStyle(0, ES_PASSWORD);
	edit->SetPasswordChar('*');
}

/*
 * Routine to cause item "item" to be the default button
 */
void DiaDefaultButton(void *vdia, INTBIG item)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->defaultbutton = item;
	dia->window->SetDefID(dia->defaultbutton-1+ID_DIALOGITEM_0);
}

/*
 * Routine to change the icon in item "item" to be the 32x32 bitmap (128 bytes) at "addr".
 */
void DiaChangeIcon(void *vdia, INTBIG item, UCHAR1 *addr)
{
	HICON map;
	INTBIG x, y;
	CDC *dc;
	RECT rr;
	TDIALOG *dia;
	COLORREF backc;

	dia = (TDIALOG *)vdia;
	item--;
	dia->itemdesc->list[item].data = (INTBIG)addr;
	map = gra_makeicon(dia->itemdesc->list[item].data);

	/* redraw */
	dc = dia->window->GetDC();

	/* erase the area */
	rr.left = dia->itemdesc->list[item].r.left;
	rr.right = dia->itemdesc->list[item].r.right;
	rr.top = dia->itemdesc->list[item].r.top;
	rr.bottom = dia->itemdesc->list[item].r.bottom;
	if (gra_dialogbkgrbrush == 0)
	{
		backc = 0;
		for(y=rr.top; y<rr.bottom; y++)
		{
			for(x=rr.left; x<rr.right; x++)
			{
				backc = dia->window->GetDC()->GetPixel(x, y);
				if (backc != 0) break;
			}
			if (backc != 0) break;
		}
		gra_dialogbkgrbrush = new CBrush(backc);
	}
	dc->FillRect(&rr, gra_dialogbkgrbrush);

	/* draw the icon */
	x = dia->itemdesc->list[item].r.left;
	y = dia->itemdesc->list[item].r.top;
	dc->DrawIcon(x, y, map);
	DestroyIcon(map);
	dia->window->ReleaseDC(dc);
}

/*
 * Routine to change item "item" into a popup with "count" entries
 * in "names".
 */
void DiaSetPopup(void *vdia, INTBIG item, INTBIG count, CHAR **names)
{
	INTBIG i;
	CComboBox *cb;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	cb = (CComboBox *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (cb == 0) return;
	cb->ResetContent();
	for(i=0; i<count; i++) cb->AddString(names[i]);
	cb->SetCurSel(0);
}

/*
 * Routine to change popup item "item" so that the current entry is "entry".
 */
void DiaSetPopupEntry(void *vdia, INTBIG item, INTBIG entry)
{
	CComboBox *cb;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	cb = (CComboBox *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (cb == 0) return;
	cb->SetCurSel(entry);
}

/*
 * Routine to return the current item in popup menu item "item".
 */
INTBIG DiaGetPopupEntry(void *vdia, INTBIG item)
{
	CComboBox *cb;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	cb = (CComboBox *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (cb == 0) return(0);
	return(cb->GetCurSel());
}

void DiaInitTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **),
	CHAR *(*nextinlist)(void), void (*donelist)(void), INTBIG sortpos, INTBIG flags)
{
	long add, remove;
	static CFont *fnt = 0;
	LOGFONT lf;
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return;

	add = remove = 0;
	if ((flags&SCHORIZBAR) == 0) remove |= WS_HSCROLL; else
	{
		add |= WS_HSCROLL;
		list->SetHorizontalExtent(1000);
	}
	list->ModifyStyle(remove, add, SWP_NOSIZE);
	dia->itemdesc->list[item].data = flags;
	if ((flags&SCFIXEDWIDTH) != 0)
	{
		if (fnt == 0)
		{
			lf.lfHeight = -10;
			estrcpy(lf.lfFaceName, x_("Lucida Console"));
			lf.lfWidth = 0;
			lf.lfEscapement = 0;
			lf.lfOrientation = 0;
			lf.lfWeight = FW_NORMAL;
			lf.lfItalic = 0;
			lf.lfUnderline = 0;
			lf.lfStrikeOut = 0;
			lf.lfCharSet = 0;
			lf.lfOutPrecision = OUT_STROKE_PRECIS;
			lf.lfClipPrecision = CLIP_STROKE_PRECIS;
			lf.lfQuality = 1;
			lf.lfPitchAndFamily = FF_DONTCARE | FF_SWISS;

			fnt = new CFont();
			fnt->CreateFontIndirect(&lf);
		}
		list->SetFont(fnt);
	}
	DiaLoadTextDialog(vdia, item+1, toplist, nextinlist, donelist, sortpos);
}

void DiaLoadTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **),
		CHAR *(*nextinlist)(void), void (*donelist)(void), INTBIG sortpos)
{
	CHAR *next, line[256];
	INTBIG i;
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return;

	/* clear the list */
	list->ResetContent();

	/* load the list */
	line[0] = 0;
	next = line;
	(void)(*toplist)(&next);
	for(i=0; ; i++)
	{
		next = (*nextinlist)();
		if (next == 0) break;
		if (sortpos < 0) list->InsertString(-1, next); else
			list->AddString(next);
	}
	(*donelist)();
	if (i > 0) list->SetCurSel(0);
}

/*
 * Routine to stuff line "line" at the end of the edit buffer.
 */
void DiaStuffLine(void *vdia, INTBIG item, CHAR *line)
{
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return;
	list->InsertString(-1, line);
}

/*
 * Routine to select line "line" of scroll item "item".
 */
void DiaSelectLine(void *vdia, INTBIG item, INTBIG line)
{
	CListBoxEx *list;
	INTBIG numitems, botitem, topitem, visibleitems, type;
	CPoint pt;
	BOOL outside;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	type = dia->itemdesc->list[item].type;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return;
	pt.x = 0;   pt.y = 0;
	topitem = list->ItemFromPoint(pt, outside); 
	pt.x = 0;   pt.y = 9999;
	botitem = list->ItemFromPoint(pt, outside);
	visibleitems = botitem - topitem;
	numitems = list->GetCount();
	if (line < topitem || line >= botitem)
	{
		topitem = line - visibleitems/2;
		list->SetTopIndex(topitem);
	}
	if ((type&ITEMTYPE) == SCROLL)
	{
		list->SetCurSel(line);
	} else
	{
		list->SetSel(-1, FALSE);
		list->SetSel(line, TRUE);
	}
}

/*
 * Routine to select "count" lines in "lines" of scroll item "item".
 */
void DiaSelectLines(void *vdia, INTBIG item, INTBIG count, INTBIG *lines)
{
	CListBoxEx *list;
	INTBIG numitems, botitem, topitem, visibleitems, i, low, high;
	CPoint pt;
	BOOL outside;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	if (count <= 0) return;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return;
	pt.x = 0;   pt.y = 0;
	topitem = list->ItemFromPoint(pt, outside); 
	pt.x = 0;   pt.y = 9999;
	botitem = list->ItemFromPoint(pt, outside);
	visibleitems = botitem - topitem;
	numitems = list->GetCount();
	low = high = lines[0];
	for(i=1; i<count; i++)
	{
		if (lines[i] < low) low = lines[i];
		if (lines[i] > high) high = lines[i];
	}
	if (high < topitem || low >= botitem)
	{
		topitem = low;
		list->SetTopIndex(topitem);
	}
	list->SetSel(-1, FALSE);
	for(i=0; i<count; i++)
		list->SetSel(lines[i], TRUE);
}

/*
 * Returns the currently selected line in the scroll list "item".
 */
INTBIG DiaGetCurLine(void *vdia, INTBIG item)
{
	INTBIG line;
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return(-1);
	line = list->GetCurSel();
	if (line == LB_ERR) return(-1);
	return(line);
}

/*
 * Returns the currently selected lines in the scroll list "item".  The returned
 * array is terminated with -1.
 */
INTBIG *DiaGetCurLines(void *vdia, INTBIG item)
{
	REGISTER INTBIG total, i;
	CListBoxEx *list;
	static INTBIG selitemlist[MAXSCROLLMULTISELECT];
	int selintlist[MAXSCROLLMULTISELECT];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) total = 0; else
	{
		total = list->GetSelItems(MAXSCROLLMULTISELECT-1, selintlist);
		if (total == LB_ERR) total = 0;
		for(i=0; i<total; i++) selitemlist[i] = selintlist[i];
	}
	selitemlist[total] = -1;
	return(selitemlist);
}

INTBIG DiaGetNumScrollLines(void *vdia, INTBIG item)
{
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return(0);
	return (list->GetCount());
}

CHAR *DiaGetScrollLine(void *vdia, INTBIG item, INTBIG line)
{
	static CHAR text[300];
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return(x_(""));
	if (list->GetText(line, text) == -1) return(x_(""));
	return(text);
}

void DiaSetScrollLine(void *vdia, INTBIG item, INTBIG line, CHAR *msg)
{
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	list = (CListBoxEx *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	if (list == 0) return;
	list->DeleteString(line);
	list->InsertString(line, msg);
	list->SetCurSel(line);
}

void DiaSynchVScrolls(void *vdia, INTBIG item1, INTBIG item2, INTBIG item3)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	if (item1 <= 0 || item1 > dia->itemdesc->items) return;
	if (item2 <= 0 || item2 > dia->itemdesc->items) return;
	if (item3 < 0 || item3 > dia->itemdesc->items) return;
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
	if (item <= 0 || item > dia->itemdesc->items) return;
	item--;
	*rect = dia->itemdesc->list[item].r;
}

void DiaPercent(void *vdia, INTBIG item, INTBIG percent)
{
	CProgressCtrl *prog;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	prog = (CProgressCtrl *)dia->window->GetDlgItem(item+ID_DIALOGITEM_0);
	prog->SetPos(percent);
}

void DiaRedispRoutine(void *vdia, INTBIG item, void (*routine)(RECTAREA*, void*))
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->redrawroutine = routine;
	dia->redrawitem = item;
}

void DiaAllowUserDoubleClick(void *vdia)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->useritemdoubleclick = 1;
}

void DiaDrawRect(void *vdia, INTBIG item, RECTAREA *ur, INTBIG r, INTBIG g, INTBIG b)
{
	RECT rr;
	CDC *dc;
	CBrush *brush;
	COLORREF color;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	rr.left = ur->left;
	rr.right = ur->right;
	rr.top = ur->top;
	rr.bottom = ur->bottom;

	color = (b << 16) | (g << 8) | r;
	brush = new CBrush(color);
	dc = dia->window->GetDC();
	dc->FillRect(&rr, brush);
	dia->window->ReleaseDC(dc);
	delete brush;
}

void DiaFrameRect(void *vdia, INTBIG item, RECTAREA *ur)
{
	RECT r;
	CDC *dc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	r.left = ur->left;
	r.right = ur->right-1;
	r.top = ur->top;
	r.bottom = ur->bottom-1;

	if (gra_dialogoffbrush == 0) gra_dialogoffbrush = new CBrush((COLORREF)0xFFFFFF);
	dc = dia->window->GetDC();
	dc->FillRect(&r, gra_dialogoffbrush);
	dc->MoveTo(r.left, r.top);
	dc->LineTo(r.right, r.top);
	dc->LineTo(r.right, r.bottom);
	dc->LineTo(r.left, r.bottom);
	dc->LineTo(r.left, r.top);
	dia->window->ReleaseDC(dc);
}

void DiaInvertRect(void *vdia, INTBIG item, RECTAREA *ur)
{
	RECT r;
	CDC *dc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	r.left = ur->left;
	r.right = ur->right-1;
	r.top = ur->top;
	r.bottom = ur->bottom-1;

	dc = dia->window->GetDC();
	dc->InvertRect(&r);
	dia->window->ReleaseDC(dc);
}

void DiaDrawLine(void *vdia, INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode)
{
	CDC *dc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dc = dia->window->GetDC();
	switch (mode)
	{
		case DLMODEON:     dc->SetROP2(R2_BLACK);   break;
		case DLMODEOFF:    dc->SetROP2(R2_WHITE);   break;
		case DLMODEINVERT: dc->SetROP2(R2_NOT);     break;
	}
	dc->MoveTo(fx, fy);
	dc->LineTo(tx, ty);
	dc->SetROP2(R2_COPYPEN);
	dia->window->ReleaseDC(dc);
}

void DiaFillPoly(void *vdia, INTBIG item, INTBIG *x, INTBIG *y, INTBIG count, INTBIG r, INTBIG g, INTBIG b)
{
	CBrush *polybrush;
	CPen *polypen;
	COLORREF brushcolor;
	POINT points[50];
	INTBIG i;
	CDC *dc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	for(i=0; i<count; i++)
	{
		points[i].x = x[i];
		points[i].y = y[i];
	}

	brushcolor = (COLORREF)((b << 16) | (g << 8) | r);
	polybrush = new CBrush(brushcolor);
	polypen = new CPen(PS_SOLID, 0, brushcolor);
	dc = dia->window->GetDC();
	dc->SelectObject(polybrush);
	dc->SelectObject(polypen);
	dc->Polygon(points, count);
	delete polybrush;
	delete polypen;
	dia->window->ReleaseDC(dc);
}

void DiaPutText(void *vdia, INTBIG item, CHAR *msg, INTBIG x, INTBIG y)
{
	CFont *font;
	CDC *dc;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(dia->usertextsize));
	font = gra_gettextfont(NOWINDOWPART, NOTECHNOLOGY, descript);
	if (font == 0) return;
	dc = dia->window->GetDC();
	dc->SelectObject(font);
	dc->SetBkMode(TRANSPARENT);
	dc->TextOut(x, y, msg, estrlen(msg));
	dia->window->ReleaseDC(dc);
}

void DiaSetTextSize(void *vdia, INTBIG size)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->usertextsize = size;
}

void DiaGetTextInfo(void *vdia, CHAR *msg, INTBIG *wid, INTBIG *hei)
{
	CFont *font;
	CSize textSize;
	CDC *dc;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(dia->usertextsize));
	font = gra_gettextfont(NOWINDOWPART, NOTECHNOLOGY, descript);
	if (font == 0) return;
	dc = dia->window->GetDC();
	dc->SelectObject(font);
	textSize = dc->GetTextExtent(msg, estrlen(msg));
	*wid = textSize.cx;
	*hei = textSize.cy+1;
	dia->window->ReleaseDC(dc);
}

void DiaTrackCursor(void *vdia, void (*eachdown)(INTBIG x, INTBIG y))
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	gra_trackingdialog = dia;
	dia->diaeachdown = eachdown;
	trackcursor(FALSE, us_nullup, us_nullvoid, gra_diaeachdownhandler,
		us_nullchar, us_nullvoid, TRACKNORMAL);
	gra_trackingdialog = 0;
}

BOOLEAN gra_diaeachdownhandler(INTBIG ox, INTBIG oy)
{
	INTBIG x, y;

	if (gra_trackingdialog == 0) return(FALSE);
	DiaGetMouse(gra_trackingdialog, &x, &y);
	(void)((*gra_trackingdialog->diaeachdown)(x, y));
	return(FALSE);
}

void DiaGetMouse(void *vdia, INTBIG *x, INTBIG *y)
{
	POINT p, p2;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	if (us_logplay != NULL)
	{
		if (!gra_loggetnextaction(0))
		{
			*x = gra_action.x;
			*y = gra_action.y;
		}
	}
	if (us_logplay == NULL)
	{
		p2.x = p2.y = 0;
		dia->window->MapWindowPoints(0, &p2, 1);
		GetCursorPos(&p);
		*x = p.x - p2.x;   *y = p.y - p2.y;
	}
	gra_logwriteaction(DIAUSERMOUSE, dia->windindex, *x, *y, 0);
}

/************************* DIALOG SUPPORT *************************/

/*
 * Routine to redraw user items and divider lines
 */
void gra_diaredrawitem(CElectricDialog *cedia)
{
	RECTAREA ra;
	INTBIG i, itemtype, x, y;
	HICON map;
	CDC *dc;
	TDIALOG *dia;

	/* find the dialog that needs to be redrawn */
	for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
		if (dia->window == cedia) break;
	if (dia == NOTDIALOG) return;

	/* if it has a custom redraw routine, call it */
	if (dia->redrawroutine != 0)
	{
		DiaItemRect(dia, dia->redrawitem, &ra);
		(*dia->redrawroutine)(&ra, dia);
	}

	/* redraw any special items */
	for(i=0; i<dia->itemdesc->items; i++)
	{
		itemtype = dia->itemdesc->list[i].type;
		if ((itemtype&ITEMTYPE) == DIVIDELINE)
		{
			DiaDrawRect(dia, i+1, &dia->itemdesc->list[i].r, 0, 0, 0);
		}
		if ((itemtype&ITEMTYPE) == ICON)
		{
			if (dia->itemdesc->list[i].data != 0)
			{
				x = dia->itemdesc->list[i].r.left;
				y = dia->itemdesc->list[i].r.top;
				map = gra_makeicon(dia->itemdesc->list[i].data);
				dc = dia->window->GetDC();
				dc->DrawIcon(x, y, map);
				dia->window->ReleaseDC(dc);
				DestroyIcon(map);
			}
		}
	}
}

/*
 * Routine called when a scroll item is scrolled vertically
 */
void gra_itemvscrolled(void *vdia, int nID)
{
	int value, i, item, itemtype;
	CListBoxEx *list;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item = nID - ID_DIALOGITEM_0;
	itemtype = dia->itemdesc->list[item].type & ITEMTYPE;
	if (itemtype != SCROLL && itemtype != SCROLLMULTI) return;

	list = (CListBoxEx *)dia->window->GetDlgItem(nID);
	value = list->GetTopIndex();
	for(i=0; i<dia->numlocks; i++)
	{
		if (dia->lock1[i] == item)
		{
			list = (CListBoxEx *)dia->window->GetDlgItem(dia->lock2[i]+ID_DIALOGITEM_0);
			list->SetTopIndex(value);
			if (dia->lock3[i] >= 0)
			{
				list = (CListBoxEx *)dia->window->GetDlgItem(dia->lock3[i]+ID_DIALOGITEM_0);
				list->SetTopIndex(value);
			}
			return;
		}
		if (dia->lock2[i] == item)
		{
			list = (CListBoxEx *)dia->window->GetDlgItem(dia->lock1[i]+ID_DIALOGITEM_0);
			list->SetTopIndex(value);
			if (dia->lock3[i] >= 0)
			{
				list = (CListBoxEx *)dia->window->GetDlgItem(dia->lock3[i]+ID_DIALOGITEM_0);
				list->SetTopIndex(value);
			}
			return;
		}
		if (dia->lock3[i] == item)
		{
			list = (CListBoxEx *)dia->window->GetDlgItem(dia->lock1[i]+ID_DIALOGITEM_0);
			list->SetTopIndex(value);
			list = (CListBoxEx *)dia->window->GetDlgItem(dia->lock2[i]+ID_DIALOGITEM_0);
			list->SetTopIndex(value);
			return;
		}
	}
}

/*
 * Routine called when an item is clicked
 */
void gra_itemclicked(CElectricDialog *diawin, int nID)
{
	int itemtype, line, count, item;
	CListBoxEx *list;
	CComboBox *cb;
	int selintlist[MAXSCROLLMULTISELECT];
	TDIALOG *dia;

	for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
		if (dia->window == diawin) break;
	if (dia == NOTDIALOG) return;

	if (nID == 1)
	{
		item = dia->defaultbutton;
		if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
			dia->dialoghit = item;
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
		return;
	}
	if (nID == 2)
	{
		item = 2;
		if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
			dia->dialoghit = item;
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
		return;
	}

	/* handle scroll areas */
	itemtype = dia->itemdesc->list[nID-ID_DIALOGITEM_0].type;
	if ((itemtype&ITEMTYPE) == SCROLL || (itemtype&ITEMTYPE) == SCROLLMULTI)
	{
		list = (CListBoxEx *)dia->window->GetDlgItem(nID);

		/* log the selection */
		if ((itemtype&ITEMTYPE) == SCROLLMULTI)
		{
			count = list->GetSelItems(MAXSCROLLMULTISELECT-1, selintlist);
			if (count != LB_ERR)
				gra_logwriteaction(DIASCROLLSEL, dia->windindex, nID - ID_DIALOGITEM_0 + 1, count, selintlist);
		} else
		{
			line = list->GetCurSel();
			selintlist[0] = line;
			if (line != LB_ERR)
				gra_logwriteaction(DIASCROLLSEL, dia->windindex, nID - ID_DIALOGITEM_0 + 1, 1, selintlist);
		}

		/* if no mouse selection allowed, deselect the list (leaves an outline, but oh well) */
		if ((dia->itemdesc->list[nID-ID_DIALOGITEM_0].data&SCSELMOUSE) == 0)
		{
			if (list == 0) return;
			list->SetCurSel(-1);
			return;
		}

		/* ignore clicks in scroll areas that do not want hits reported */
		if ((dia->itemdesc->list[nID-ID_DIALOGITEM_0].data&SCREPORT) == 0)
			return;
	}
	if ((itemtype&ITEMTYPE) == POPUP)
	{
		cb = (CComboBox *)dia->window->GetDlgItem(nID);
		gra_logwriteaction(DIAPOPUPSEL, dia->windindex, nID - ID_DIALOGITEM_0 + 1, cb->GetCurSel(), 0);
	}

	item = nID - ID_DIALOGITEM_0 + 1;
	gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
		dia->dialoghit = item;
}

/*
 * Routine called when an item is double-clicked
 */
void gra_itemdoubleclicked(CElectricDialog *diawin, int nID)
{
	INTBIG itemtype, item;
	BOOLEAN allowdouble;
	TDIALOG *dia;

	for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
		if (dia->window == diawin) break;
	if (dia == NOTDIALOG) return;

	if (dia->window == 0) return;
	itemtype = dia->itemdesc->list[nID].type;

	allowdouble = FALSE;
	if ((itemtype&ITEMTYPE) == SCROLL || (itemtype&ITEMTYPE) == SCROLLMULTI)
	{
		if ((dia->itemdesc->list[nID].data&SCDOUBLEQUIT) != 0) allowdouble = TRUE;
	}
	if ((itemtype&ITEMTYPE) == USERDRAWN)
	{
		if (dia->useritemdoubleclick != 0) allowdouble = TRUE;
	}

	if (allowdouble)
	{
		item = dia->defaultbutton;
		if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
			dia->dialoghit = item;
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	}
}

/*
 * Called when a key is typed to a list box.  Returns nonzero to accept the
 * keystroke, zero to ignore typed keys in the list box.
 */
int gra_dodialoglistkey(CElectricDialog *diawin, UINT nKey, CListBox* pListBox, UINT nIndex)
{
	CListBoxEx *list;
	INTBIG i, itemtype, line, count;
	int selintlist[MAXSCROLLMULTISELECT];
	TDIALOG *dia;

	for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
		if (dia->window == diawin) break;
	if (dia == NOTDIALOG) return(0);

	/* always allow arrow keys */
	if (nKey == VK_DOWN || nKey == VK_UP) return(1);

	for(i=0; i<dia->itemdesc->items; i++)
	{
		itemtype = dia->itemdesc->list[i].type;
		if ((itemtype&ITEMTYPE) != SCROLL && (itemtype&ITEMTYPE) != SCROLLMULTI) continue;
		list = (CListBoxEx *)dia->window->GetDlgItem(i+ID_DIALOGITEM_0);
		if (list != pListBox) continue;
		if ((dia->itemdesc->list[i].data&SCSELKEY) == 0) return(0);
		if ((itemtype&ITEMTYPE) == SCROLLMULTI)
		{
			count = list->GetSelItems(MAXSCROLLMULTISELECT-1, selintlist);
			if (count != LB_ERR)
				gra_logwriteaction(DIASCROLLSEL, dia->windindex, i + 1, (int)count, selintlist);
		} else
		{
			line = list->GetCurSel();
			selintlist[0] = line;
			if (line != LB_ERR)
				gra_logwriteaction(DIASCROLLSEL, dia->windindex, i + 1, 1, selintlist);
		}
		break;
	}

	return(1);
}

/*
 * Routine called when a character is typed
 */
void gra_dodialogtextchange(CElectricDialog *diawin, int nID)
{
	INTBIG item;
	TDIALOG *dia;

	for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
		if (dia->window == diawin) break;
	if (dia == NOTDIALOG) return;

	/* get current contents of the edit field */
	item = nID + 1;
	if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
		dia->dialoghit = item;
	gra_logwriteaction(DIAEDITTEXT, dia->windindex, item, 0, DiaGetText(dia, item));
}

/*
 * Routine to return a nonzero item number if point (x,y) is inside a user-drawn item
 */
INTBIG gra_dodialogisinsideuserdrawn(TDIALOG *dia, int x, int y)
{
	INTBIG i, itemtype;

	if (dia->window == 0) return(0);
	i = gra_getdialogitem(dia, x, y);
	itemtype = dia->itemdesc->list[i-1].type;
	if ((itemtype&ITEMTYPE) != USERDRAWN &&
		(itemtype&ITEMTYPE) != ICON) return(0);
	return(i);
}

/*
 * Routine to return a nonzero item number if point (x,y) is inside a user-drawn item
 */
INTBIG gra_getdialogitem(TDIALOG *dia, int x, int y)
{
	INTBIG i;

	if (dia->window == 0) return(0);
	for(i=0; i<dia->itemdesc->items; i++)
	{
		if (x < dia->itemdesc->list[i].r.left) continue;
		if (x > dia->itemdesc->list[i].r.right) continue;
		if (y < dia->itemdesc->list[i].r.top) continue;
		if (y > dia->itemdesc->list[i].r.bottom) continue;
		return(i+1);
	}
	return(0);
}

/*
 * Routine to make an icon from data
 */
HICON gra_makeicon(INTBIG idata)
{
	UCHAR1 zero[128], *data;
	int i;

	data = (UCHAR1 *)idata;
	for(i=0; i<128; i++)
	{
		zero[i] = 0;
		data[i] = ~data[i];
	}
	HICON icon = CreateIcon(0, 32, 32, 1, 1, (UCHAR1 *)data, zero);
	for(i=0; i<128; i++) data[i] = ~data[i];
	return(icon);
}

/****************************** TCL SUPPORT ******************************/

#if LANGTCL
INTBIG gra_initializetcl(void)
{
	INTBIG err;
	CHAR *newArgv[2];
	void *infstr;

	/* set the program name/path */
	newArgv[0] = x_("Electric");
	newArgv[1] = NULL;
	(void)Tcl_FindExecutable(newArgv[0]);

	tcl_interp = Tcl_CreateInterp();
	if (tcl_interp == 0) error(_("from Tcl_CreateInterp"));

	/* tell Electric the TCL interpreter handle */
	el_tclinterpreter(tcl_interp);

	/* set the Tcl library directory - for internal Tcl use */
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, x_("tcl8.3"));
	if (Tcl_SetVar(tcl_interp, x_("tcl_library"), returninfstr(infstr), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG) == NULL)
	{
		ttyputerr(_("Tcl_SetVar failed: %s"), tcl_interp->result);
		return(1);
	}

	/* Make command-line arguments available in the Tcl variables "argc" and "argv" */
	Tcl_SetVar(tcl_interp, x_("argv"), x_(""), TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp, x_("argc"), x_("0"), TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp, x_("argv0"), x_("electric"), TCL_GLOBAL_ONLY);

	/* Set the "tcl_interactive" variable */
	Tcl_SetVar(tcl_interp, x_("tcl_interactive"), x_("1"), TCL_GLOBAL_ONLY);

	/* initialize the interpreter */
	err = Tcl_Init(tcl_interp);
	if (err != TCL_OK) error(_("(from Tcl_Init) %s"), tcl_interp->result);

	return(err);
}
#endif

/****************************** PRINTING ******************************/

/* call this routine to print the current window */
void printewindow(void)
{
	INTBIG pagewid, pagehei, pagehpixperinch, pagevpixperinch, ypos, i, l, style,
		marginx, marginy, slx, shx, sly, shy, *curstate, xpos, boxsize, dashindent,
		ulx, uhx, uly, uhy, width, height, centerx, centery, prod1, prod2, indent,
		fontHeight, page, topmargin, leftmargin, savewid, savehei, resfactor,
		saveslx, saveshx, savesly, saveshy, bigwid, bighei, wlx, whx, wly, why,
		lastboxxpos, yposbox, topypos;
	REGISTER CHAR *name, *header, *cptr;
	REGISTER UCHAR1 **saverowstart, *savedata, *ptr;
	WINDOWPART *win;
	NODEPROTO *np;
	WINDOWFRAME *wf;
	REGISTER VARIABLE *var;
	BOOLEAN savedirty, *done;
	HCURSOR hCursorBusy, hCursorOld;
	static DOCINFO di = {sizeof(DOCINFO), x_("Electric"), NULL};
	RGBQUAD bmiColors[256];
	BITMAPINFO *bmiInfo, *savebmiinfo;
	CDC printDC, *pDCPrint, *saveoff;
	HDC hPrnDC;
	DEVMODE *dm;
	CFont *fnt;
	HBITMAP savebitmap;
	REGISTER void *infstr;
	CPen penBlack, penGrey;

	/* get cell to plot */
	win = el_curwindowpart;
	if (win == NOWINDOWPART)
	{
		ttyputerr(_("No current window to print"));
		return;
	}
	wf = win->frame;
	np = win->curnodeproto;

	/* get printing control bits */
	curstate = io_getstatebits();

	if (np == NONODEPROTO)
	{
		/* allow explorer window, but nothing else */
		if ((win->state&WINDOWTYPE) != EXPLORERWINDOW)
		{
			ttyputerr(_("Cannot print this type of window"));
			return;
		}
	}

	/* create a print structure */
	CPrintDialog printDlg(FALSE);

	/* set requested orientation */
	if (printDlg.GetDefaults() == 0) return;
	dm = printDlg.GetDevMode();
	if ((curstate[1]&PSAUTOROTATE) != 0 && np != NONODEPROTO &&
		(np->cellview->viewstate&TEXTVIEW) == 0)
	{
		(void)io_getareatoprint(np, &wlx, &whx, &wly, &why, FALSE);
		if (whx-wlx > why-wly) curstate[0] |= PSROTATE; else
			curstate[0] &= ~PSROTATE;
	}
	if ((curstate[0]&PSROTATE) != 0) dm->dmOrientation = DMORIENT_LANDSCAPE; else
		dm->dmOrientation = DMORIENT_PORTRAIT;
	printDlg.m_pd.Flags &= ~PD_RETURNDEFAULT;

	/* show the dialog */
	if (printDlg.DoModal() != IDOK) return;

	/* set cursor to busy */
	hCursorBusy = LoadCursor(NULL, IDC_WAIT);
	hCursorOld = SetCursor(hCursorBusy);

	/* collect size information about the Printer DC */
	hPrnDC = printDlg.m_pd.hDC;   /* GetPrinterDC(); */
	if (hPrnDC == 0)
	{
		ttyputerr(_("Printing error: could not start print job"));
		return;
	}
	pDCPrint = printDC.FromHandle(hPrnDC);
	pagewid = pDCPrint->GetDeviceCaps(HORZRES);
	pagehei = pDCPrint->GetDeviceCaps(VERTRES);
	pagehpixperinch = pDCPrint->GetDeviceCaps(LOGPIXELSX);
	pagevpixperinch = pDCPrint->GetDeviceCaps(LOGPIXELSY);

	/* start printing */
	pDCPrint->StartDoc(&di);

	if (np == NONODEPROTO && (win->state&WINDOWTYPE) == EXPLORERWINDOW)
	{
		/* an explorer window: print text */
		us_explorerreadoutinit(win);
		pDCPrint->StartPage();

		/* print the window */
		fontHeight = MulDiv(10, GetDeviceCaps(hPrnDC, LOGPIXELSY), 72);
		fnt = new CFont();
 		fnt->CreateFont(-fontHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, x_("Courier"));
		pDCPrint->SelectObject(fnt);
		pDCPrint->SetTextColor(RGB(0,0,0));
		pDCPrint->SetBkColor(RGB(255,255,255));

		var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
		if (var == NOVARIABLE) i = muldiv(DEFAULTPSMARGIN, WHOLE, 75); else
			i = var->addr;
		topmargin = muldiv(i, pagevpixperinch, WHOLE);
		leftmargin = muldiv(i, pagehpixperinch, WHOLE);

		ypos = topmargin;
		page = 1;
		lastboxxpos = -1;
		boxsize = fontHeight * 2 / 3;
		dashindent = boxsize / 5;
		penBlack.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(0, 0, 0));
		penGrey.CreatePen(PS_SOLID | PS_COSMETIC, 1, RGB(128, 128, 128));

		pDCPrint->SelectObject(&penBlack);
		for(;;)
		{
			name = us_explorerreadoutnext(win, &indent, &style, &done, 0);
			if (name == 0) break;

			/* insert page breaks */
			if (ypos+fontHeight >= pagehei-topmargin)
			{
				ypos = topmargin;
				pDCPrint->EndPage();
				pDCPrint->StartPage();
			}

			/* print the header */
			if (ypos == topmargin)
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("Cell Explorer    Page %ld"), page);
				header = returninfstr(infstr);
				pDCPrint->TextOut(leftmargin, 0, header, estrlen(header));
				page++;
				if (topmargin < fontHeight*2) ypos += fontHeight*2;
			}
			xpos = leftmargin + indent * fontHeight;

			/* draw the box */
			yposbox = ypos + (fontHeight-boxsize) / 2;
			pDCPrint->MoveTo(xpos, yposbox);
			pDCPrint->LineTo(xpos+boxsize, yposbox);
			pDCPrint->LineTo(xpos+boxsize, yposbox+boxsize);
			pDCPrint->LineTo(xpos, yposbox+boxsize);
			pDCPrint->LineTo(xpos, yposbox);
			if (style >= 2)
			{
				/* draw horizontal line in box */
				pDCPrint->MoveTo(xpos+dashindent, yposbox+boxsize/2);
				pDCPrint->LineTo(xpos+boxsize-dashindent, yposbox+boxsize/2);
				if (style == 2)
				{
					/* draw vertical line in box */
					pDCPrint->MoveTo(xpos+boxsize/2, yposbox+dashindent);
					pDCPrint->LineTo(xpos+boxsize/2, yposbox+boxsize-dashindent);
				}
			}

			/* draw the connecting lines */
			if (indent > 0)
			{
				pDCPrint->SelectObject(&penGrey);
				pDCPrint->MoveTo(xpos, yposbox+boxsize/2);
				pDCPrint->LineTo(xpos-fontHeight+boxsize/2, yposbox+boxsize/2);
				for(i=0; i<indent; i++)
				{
					if (done[i]) continue;
					l = leftmargin+i*fontHeight+boxsize/2;
					if (l == lastboxxpos)
						topypos = yposbox+boxsize/2-fontHeight+boxsize/2; else
							topypos = yposbox+boxsize/2-fontHeight;
					pDCPrint->MoveTo(l, yposbox+boxsize/2);
					pDCPrint->LineTo(l, topypos);
				}
				pDCPrint->SelectObject(&penBlack);
			}
			lastboxxpos = xpos + boxsize/2;

			/* show the text */
			pDCPrint->TextOut(xpos+boxsize+boxsize/2, ypos, name, estrlen(name));
			ypos += fontHeight;
		}
		delete fnt;
		pDCPrint->EndPage();
	} else if (np != NONODEPROTO && (np->cellview->viewstate&TEXTVIEW) != 0)
	{
		/* a text window: print text */
		pDCPrint->StartPage();

		/* print the window */
		fontHeight = MulDiv(10, GetDeviceCaps(hPrnDC, LOGPIXELSY), 72);
		fnt = new CFont();
 		fnt->CreateFont(-fontHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEVICE_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, x_("Courier"));
		pDCPrint->SelectObject(fnt);
		pDCPrint->SetTextColor(RGB(0,0,0));
		pDCPrint->SetBkColor(RGB(255,255,255));

		var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
		if (var == NOVARIABLE) i = muldiv(DEFAULTPSMARGIN, WHOLE, 75); else
			i = var->addr;
		topmargin = muldiv(i, pagevpixperinch, WHOLE);
		leftmargin = muldiv(i, pagehpixperinch, WHOLE);

		ypos = topmargin;
		page = 1;
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
		if (var != NOVARIABLE)
		{
			l = getlength(var);
			for(i=0; i<l; i++)
			{
				cptr = ((CHAR **)var->addr)[i];
				if (i == l-1 && *cptr == 0) continue;
				if (ypos+fontHeight >= pagehei-topmargin)
				{
					ypos = topmargin;
					pDCPrint->EndPage();
					pDCPrint->StartPage();
				}
				if (ypos == topmargin)
				{
					/* print the header */
					infstr = initinfstr();
					formatinfstr(infstr, _("Library: %s   Cell: %s   Page %ld"), np->lib->libname,
						describenodeproto(np), page);
					header = returninfstr(infstr);
					pDCPrint->TextOut(leftmargin, 0, header, estrlen(header));

					if ((us_useroptions&NODATEORVERSION) == 0 && page == 1)
					{
						infstr = initinfstr();
						if (np->creationdate != 0)
							formatinfstr(infstr, _("Created: %s"), timetostring((time_t)np->creationdate));
						if (np->revisiondate != 0)
							formatinfstr(infstr, _("   Revised: %s"), timetostring((time_t)np->revisiondate));
						header = returninfstr(infstr);
						pDCPrint->TextOut(leftmargin, fontHeight, header, estrlen(header));
					}
					page++;
					if (topmargin < fontHeight*2) ypos += fontHeight*2;
				}
				pDCPrint->TextOut(leftmargin, ypos, cptr, estrlen(cptr));
				ypos += fontHeight;
			}
		}
		delete fnt;
 		pDCPrint->EndPage();
	} else if (np != NONODEPROTO)
	{
		/* a graphics window: get the print resolution scale factor */
		var = getval((INTBIG)io_tool, VTOOL, VINTEGER, x_("IO_print_resolution_scale"));
		if (var == NOVARIABLE) resfactor = 1; else
			resfactor = var->addr;

		/* make a bigger offscreen window */
		bigwid = wf->swid * resfactor;
		bighei = wf->shei * resfactor;
		if (gra_getbiggeroffscreenbuffer(wf, bigwid, bighei)) return;
		saveoff = (CDC *)wf->hDCOff;     wf->hDCOff = (void *)gra_biggeroffhdc;
		savebitmap = wf->hBitmap;        wf->hBitmap = gra_biggeroffbitmap;
		savebmiinfo = wf->bminfo;        wf->bminfo = gra_biggeroffbitmapinfo;
		savedata = wf->data;             wf->data = gra_biggeroffdatabuffer;
		saverowstart = wf->rowstart;     wf->rowstart = gra_biggeroffrowstart;
		savewid = wf->swid;              wf->swid = bigwid;
		savehei = wf->shei;              wf->shei = bighei;
		savedirty = wf->offscreendirty;  wf->offscreendirty = FALSE;
		wf->revy = wf->shei - 1;
		if (wf->hPalette != 0)
		{
			(void)((CDC *)wf->hDCOff)->SelectPalette((CPalette *)wf->hPalette, TRUE);
			(void)((CDC *)wf->hDCOff)->RealizePalette();
		}
		slx = win->uselx;   shx = win->usehx;
		sly = win->usely;   shy = win->usehy;
		win->uselx *= resfactor;
		win->usehx *= resfactor;
		win->usely *= resfactor;
		win->usehy *= resfactor;
		saveslx = win->screenlx;
		saveshx = win->screenhx;
		savesly = win->screenly;
		saveshy = win->screenhy;
		if ((win->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			wlx = win->screenlx;
			whx = win->screenhx;
			wly = win->screenly;
			why = win->screenhy;
		} else
		{
			(void)io_getareatoprint(np, &wlx, &whx, &wly, &why, FALSE);
			us_squarescreen(win, NOWINDOWPART, FALSE, &wlx, &whx, &wly, &why, 1);
		}
		win->screenlx = slx = wlx;
		win->screenhx = shx = whx;
		win->screenly = sly = wly;
		win->screenhy = shy = why;
		computewindowscale(win);

		/* redraw the window in the larger buffer */
		gra_noflush = TRUE;
		if ((win->state&WINDOWTYPE) == WAVEFORMWINDOW)
		{
			(*win->redisphandler)(win);
		} else
		{
			us_redisplaynow(win, TRUE);
			us_endchanges(win);
		}
		gra_noflush = FALSE;
		(void)us_makescreen(&slx, &sly, &shx, &shy, win);

		/* restore the normal state of the window */
		wf->hDCOff = saveoff;
		wf->hBitmap = savebitmap;
		wf->bminfo = savebmiinfo;
		wf->data = savedata;
		wf->rowstart = saverowstart;
		wf->swid = savewid;
		wf->shei = savehei;
		wf->revy = wf->shei - 1;
		wf->offscreendirty = savedirty;
		if (wf->hPalette != 0)
		{
			(void)((CDC *)wf->hDCOff)->SelectPalette((CPalette *)wf->hPalette, TRUE);
			(void)((CDC *)wf->hDCOff)->RealizePalette();
		}
		win->uselx /= resfactor;
		win->usehx /= resfactor;
		win->usely /= resfactor;
		win->usehy /= resfactor;
		win->screenlx = saveslx;
		win->screenhx = saveshx;
		win->screenly = savesly;
		win->screenhy = saveshy;
		computewindowscale(win);

		marginx = pagehpixperinch / 2;
		marginy = pagevpixperinch / 2;

		/* make the plot have square pixels */
		ulx = marginx;
		uly = marginy;
		uhx = pagewid-marginx;
		uhy = pagehei-marginy;
		prod1 = (shx - slx) * (uhy - uly);
		prod2 = (shy - sly) * (uhx - ulx);
		if (prod1 != prod2)
		{
			/* adjust the scale */
			if (prod1 > prod2)
			{
				/* make it fill the width of the screen */
				height = muldiv(uhx - ulx, shy - sly, shx - slx);
				centery = (uly + uhy) / 2;
				uly = centery - height/2;
				uhy = uly + height;
			} else
			{
				/* make it fill the height of the screen */
				width = muldiv(uhy - uly, shx - slx, shy - sly);
				centerx = (ulx + uhx) / 2;
				ulx = centerx - width/2;
				uhx = ulx + width;
			}
		}

		/* adjust the background color to white */
		i = GetDIBColorTable(gra_biggeroffhdc->m_hDC, 0, 256, bmiColors);
		bmiColors[0].rgbRed = 0xFF;
		bmiColors[0].rgbGreen = 0xFF;
		bmiColors[0].rgbBlue = 0xFF;
		bmiColors[0].rgbReserved = 0;

		bmiInfo = (BITMAPINFO *)emalloc(sizeof(BITMAPINFOHEADER) + sizeof(bmiColors), db_cluster);
		if (bmiInfo == 0) return;
		ptr = (UCHAR1 *)bmiInfo;
		memcpy(ptr, gra_biggeroffbitmapinfo, sizeof(BITMAPINFOHEADER));
		ptr += sizeof(BITMAPINFOHEADER);
		memcpy(ptr, &bmiColors, sizeof(bmiColors));

		pDCPrint->StartPage();

		/* print the window */
		StretchDIBits(hPrnDC, ulx, uly, uhx-ulx, uhy-uly,
			slx, sly, shx-slx+1, shy-sly+1,
			gra_biggeroffdatabuffer, bmiInfo, DIB_RGB_COLORS, SRCCOPY);
		efree((CHAR *)bmiInfo);

		pDCPrint->EndPage();
	}

	/* finish printing */
	pDCPrint->EndDoc();
	pDCPrint->Detach();
	DeleteDC(hPrnDC);

	/* restore original cursor */
	SetCursor(hCursorOld);
}
