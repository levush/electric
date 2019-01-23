/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphunixx11.c
 * X Window System interface with Motif/Lesstif widgets
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
#include "egraphics.h"
#include "usr.h"
#include "eio.h"
#include "usrtrack.h"
#include "edialogs.h"
#include "config.h"
#include <signal.h>
#include <errno.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/CascadeB.h>
#include <Xm/CutPaste.h>
#include <Xm/Display.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MainW.h>
#include <Xm/MwmUtil.h>
#include <Xm/Protocols.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/Separator.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>


#ifdef HAVE_PTHREAD
#  include <pthread.h>
#else
#  include <thread.h>
#  include <synch.h>
#endif

#ifdef TRUETYPE
#  include "t1lib.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#ifdef HAVE_TERMIOS_H
#  include <termios.h>
#else
#  ifdef HAVE_TERMIO_H
#    include <termio.h>
#  else
#    include <sgtty.h>
#  endif
#endif

#ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else
#    include <time.h>
#  endif
#endif

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif

#ifdef HAVE_VFORK_H
#  include <vfork.h>
#endif

#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#  define NAMELEN(dirent) estrlen((dirent)->d_name)
#else
#  define dirent direct
#  define NAMELEN(dirent) (dirent)->d_namelen
#  ifdef HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif
#  ifdef HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif
#  ifdef HAVE_NDIR_H
#    include <ndir.h>
#  endif
#endif

#if LANGTCL
#  include "dblang.h"
  Tcl_Interp *gra_tclinterp;
  INTBIG gra_initializetcl(void);
#endif

#ifdef _UNICODE
#  define estat     _wstat
#  define eaccess   _waccess
#  define egetcwd   _wgetcwd
#  define ecreat    _wcreat
#else
#  define estat     stat
#  define eaccess   access
#  define egetcwd   getcwd
#  define ecreat    creat
#endif

#define TRUESTRCPY strcpy
#define TRUESTRLEN strlen


/****** windows ******/

#define WMLEFTBORDER         8					/* size of left border for windows */
#define WMTOPBORDER         30					/* size of top border for windows */
#define MAXSTATUSLINES       1					/* number of status lines */
#define FLUSHTICKS         120					/* ticks between display flushes */
#define SBARWIDTH           15					/* size of text-edit scroll bars */
#define PALETTEWIDTH       110
#define MAXLOCALSTRING     256					/* size of "gra_localstring" */

#ifdef __cplusplus
#  define VisualClass(v) v->c_class
#else
#  define VisualClass(v) v->class
#endif

static XtAppContext  gra_xtapp;					/* the main XT application */
static Display      *gra_maindpy;				/* the main X display (edit) */
static Display      *gra_altdpy;				/* the alternate X display (messages/other edit) */
static INTBIG        gra_mainscreen;
static INTBIG        gra_altscreen;
static WINDOWDISPLAY gra_mainwd;
static WINDOWDISPLAY gra_altwd;
static Colormap      gra_maincolmap;			/* main display colormap */
static Colormap      gra_altcolmap;				/* alternate display colormap */
static INTBIG        gra_mainredshift, gra_maingreenshift, gra_mainblueshift;
static INTBIG        gra_altredshift, gra_altgreenshift, gra_altblueshift;
static XSizeHints   *gra_editxsh;
static INTBIG        gra_editposx;
static INTBIG        gra_editposy;
static INTBIG        gra_editsizex;
static INTBIG        gra_editsizey;
static INTBIG        gra_palettewidth;
static INTBIG        gra_paletteheight;
static INTBIG        gra_palettetop;
static INTBIG        gra_shortestscreen;		/* height of shortest screen */
static UINTBIG       gra_internalchange;
static Pixmap        gra_programicon;
static INTBIG        gra_windownumberindex = 0;
static INTBIG        gra_windownumber;			/* number of the window that was created */
static BOOLEAN       gra_checkinginterrupt;		/* TRUE if checking for interrupt key */
static BOOLEAN       gra_tracking;				/* TRUE if tracking the cursor */
static INTBIG        gra_status_height;			/* space for lines at bottom of screen */
static WINDOWFRAME  *gra_deletethisframe = 0;	/* window that is to be deleted */
static Atom          gra_wm_delete_window;		/* to trap window kills */
static int           gra_argc;					/* "argc" for display */
static CHAR1       **gra_argv;					/* "argv" for display */
static CHAR          gra_localstring[MAXLOCALSTRING];
XtSignalId           gra_intsignalid;
extern GRAPHICS      us_box, us_ebox, us_menutext;

#ifndef ANYDEPTH
  static INTBIG      gra_maphigh;				/* highest entry to use in color map */
  static BOOLEAN     gra_use_backing;			/* flag if backing store used */
#endif
static Atom        gra_movedisplayprotocol;
static Atom        gra_movedisplaymsg;
static INTBIG      gra_windowbeingdeleted = 0;

int          gra_xerrors(Display *dpy, XErrorEvent *err);
void         gra_xterrors(CHAR1 *msg);
void         gra_adjustwindowframeon(Display *dpy, INTBIG screen, INTBIG how);
void         gra_finddisplays(BOOLEAN wantmultiscreen, BOOLEAN multiscreendebug);
BOOLEAN      gra_buildwindow(Display *dpy, WINDOWDISPLAY *wd, Colormap colmap, WINDOWFRAME *wf,
				BOOLEAN floating, BOOLEAN samesize, RECTAREA *r);
void         gra_reloadmap(void);
void         gra_getwindowattributes(WINDOWFRAME *wf, XWindowAttributes *xwa);
void         gra_getwindowinteriorsize(WINDOWFRAME *wf, INTBIG *wid, INTBIG *hei);
void         gra_intsignalfunc(void);
BOOLEAN      gra_loggetnextaction(CHAR *extramessage); 
void         gra_logwriteaction(INTBIG inputstate, INTBIG special, INTBIG cursorx,
				INTBIG cursory, void *extradata);
void         gra_logwritecomment(CHAR *comment);
BOOLEAN      gra_logreadline(CHAR *string, INTBIG limit);
void         gra_removewindowextent(RECTAREA *r, Widget wnd);
BOOLEAN      gra_makewindowdisplaybuffer(WINDOWDISPLAY *wd, Display *dpy);
void         gra_getdisplaydepth(Display *dpy, INTBIG *depth, INTBIG *truedepth);
void         gra_handlequeuedframedeletion(void);
WINDOWFRAME *gra_getframefromindex(INTBIG index);
void         gra_movedisplay(Widget w, XtPointer client_data, XtPointer *call_data);
INTBIG       gra_nativepopuptif(POPUPMENU **menuptr, BOOLEAN header, INTBIG left, INTBIG top);
#ifndef ANYDEPTH
  void       gra_loadstipple(WINDOWFRAME *wf, UINTSML raster[]);
  void       gra_setxstate(WINDOWFRAME *wf, GC gc, GRAPHICS *desc);
#endif

/****** the messages window ******/
#define MESLEADING              1				/* extra character spacing */

static Display      *gra_topmsgdpy;				/* the X display for the messages widget */
static Window        gra_topmsgwin;				/* the X window for the messages widget */
static Widget        gra_msgtoplevelwidget;		/* the outer messages widget */
static Widget        gra_messageswidget = 0;	/* the inner messages widget */
static BOOLEAN       gra_messages_obscured;		/* true if messages window covered */
static BOOLEAN       gra_messages_typingin;		/* true if typing into messages window */
static BOOLEAN       gra_msgontop;				/* true if messages window has focus */
static Colormap      gra_messagescolormap;

void gra_makemessageswindow(void);

/****** the menus ******/
static INTBIG      gra_popupmenuresult;
static INTBIG      gra_pulldownmenucount = 0;	/* number of top-level pulldown menus */
static POPUPMENU  *gra_pulldowns[50];			/* the current top-level pulldown menus */

Widget   gra_makepdmenu(WINDOWFRAME *wf, POPUPMENU *pm, INTBIG value, Widget parent,
			INTBIG pindex);
void     gra_pulldownindex(WINDOWFRAME *wf, POPUPMENU *pm, INTBIG order, Widget parent);
void     gra_menucb(Widget W, int item_number, XtPointer call_data);
void     gra_menupcb(Widget w, int item_number, XtPointer call_data);
void     gra_nativemenudoone(WINDOWFRAME *wf, INTBIG low, INTBIG high);
void     gra_pulldownmenuload(WINDOWFRAME *wf);

/****** the dialogs ******/
#define MAXSCROLLMULTISELECT 1000
#define MAXLOCKS              3   /* maximum locked pairs of scroll lists */
#define NOTDIALOG ((TDIALOG *)-1)

#define DIALOGNUM            16
#define DIALOGDEN            12
#define MAXICONS             20

typedef struct Itdialog
{
	Widget      window;
	Widget      windowframe;
	DIALOG     *itemdesc;
	INTBIG      windindex;
	INTBIG      defaultbutton;
	Widget      items[100];
	INTBIG      numlocks, lock1[MAXLOCKS], lock2[MAXLOCKS], lock3[MAXLOCKS];
	INTBIG      redrawitem;
	void      (*redrawroutine)(RECTAREA*, void*);
	INTBIG      opaqueitem;
	INTBIG      inix, iniy;
	INTBIG      usertextsize;
	INTBIG      dialoghit;
	CHAR        opaquefield[300];
	void      (*diaeachdown)(INTBIG x, INTBIG y);
	void      (*modelessitemhit)(void *dia, INTBIG item);
	struct Itdialog *nexttdialog;
} TDIALOG;

INTBIG          gra_dialoggraycol, gra_dialogbackgroundcol;
GC              gra_gcdia;
UCHAR1         *gra_icontruedata = 0, *gra_iconrowdata;   /* working memory for "gra_makeicon()" */
static CHAR    *gra_brokenbuf; /* working memory for DisSetText */
static INTBIG   gra_brokenbufsize = 0;

/* dialog support routines */
void     gra_flushdialog(TDIALOG *dia);
void     gra_getdialogcoordinates(RECTAREA *rect, INTBIG *x, INTBIG *y,
			INTBIG *wid, INTBIG *hei);
INTBIG   gra_scaledialogcoordinate(INTBIG x);
INTBIG   gra_makedpycolor(Display *dpy, INTBIG r, INTBIG g, INTBIG b);
void     gra_dialogaction(Widget w, XtPointer client_data,
			XmSelectionBoxCallbackStruct *call_data);
void     gra_vertslider(Widget w, XtPointer client_data,
			XmScrollBarCallbackStruct *call_data);
void     gra_setscroll(TDIALOG *dia, int item, int value);
void     gra_dialogredraw(Widget w, XtPointer client_data,
			XmSelectionBoxCallbackStruct *call_data);
void     gra_dialogredrawsep(Widget w, XtPointer client_data,
			XmSelectionBoxCallbackStruct *call_data);
void     gra_dialogopaqueaction(Widget w, XtPointer client_data,
			XmTextVerifyCallbackStruct *call_data);
void     gra_dialogdraw(Widget widget, XEvent *event, String *args, int *num_args);
void     gra_dialogcopywholelist(Widget widget, XEvent *event, String *args, int *num_args);
Pixmap   gra_makeicon(UCHAR1 *data, Widget widget);
void     gra_dialog_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont);
TDIALOG *gra_whichdialog(Widget w);
TDIALOG *gra_getdialogfromindex(INTBIG index);

TDIALOG        *gra_firstactivedialog = NOTDIALOG;
TDIALOG        *gra_trackingdialog = 0;

static BOOLEAN gra_initdialog(DIALOG *dialog, TDIALOG *dia, BOOLEAN modeless);
static INTBIG  gra_dialogstringpos;
static BOOLEAN gra_diaeachdownhandler(INTBIG x, INTBIG y);
static int     gra_stringposascending(const void *e1, const void *e2);

/****** events ******/
/* the meaning of "gra_inputstate" */
#define CHARREAD             0377			/* character that was read */
#define ISKEYSTROKE          0400			/* set if key typed */
#define ISBUTTON            07000			/* set if button pushed (or released) */
#define ISLEFT              01000			/*    left button */
#define ISMIDDLE            02000			/*    middle button */
#define ISRIGHT             03000			/*    right button */
#define ISWHLFWD            04000			/*    forward on the mouse wheel */
#define ISWHLBKW            05000			/*    backward on the mouse wheel */
#define BUTTONUP           010000			/* set if button was released */
#define SHIFTISDOWN        020000			/* set if shift key was held down */
#define CONTROLISDOWN      040000			/* set if control key was held down */
#define METAISDOWN        0100000			/* set if meta key was held down */
#define DOUBLECL          0200000			/* set if double-click */
#define MOTION            0400000			/* set if mouse motion detected */
#define WINDOWSIZE       01000000			/* set if window grown */
#define WINDOWMOVE       02000000			/* set if window moved */
#define MENUEVENT        04000000			/* set if menu entry selected (values in cursor) */
#define FILEREPLY       010000000			/* set if file selected in standard file dialog */
#define DIAITEMCLICK    020000000			/* set if item clicked in dialog */
#define DIASCROLLSEL    040000000			/* set if scroll item selected in dialog */
#define DIAEDITTEXT    0100000000			/* set if edit text changed in dialog */
#define DIAPOPUPSEL    0200000000			/* set if popup item selected in dialog */
#define DIASETCONTROL  0400000000			/* set if control changed in dialog */
#define DIAUSERMOUSE  01000000000			/* set if mouse moved in user area of dialog */
#define DIAENDDIALOG  02000000000			/* set when dialog terminates */
#define POPUPSELECT   04000000000			/* popup selection made */
#define NOEVENT                -1			/* set if nothing happened */

#define INITIALTIMERDELAY      9			/* ratio of initial repeat to others */
#define TICKSPERSECOND        10			/* frequency of timer for repeats */

typedef RETSIGTYPE (*SIGNALCAST)(int);

struct
{
	INTBIG x;
	INTBIG y;
} gra_action;

typedef struct
{
	INTBIG                cursorx, cursory;		/* current position of mouse */
	INTBIG                inputstate;			/* current state of device input */
	INTBIG                special;				/* special command to do */
	UINTBIG               eventtime;			/* time of this event */
} MYEVENTQUEUE;

#define EVENTQUEUESIZE	100

static MYEVENTQUEUE  gra_eventqueue[EVENTQUEUESIZE];
static MYEVENTQUEUE *gra_eventqueuehead;		/* points to next event in queue */
static MYEVENTQUEUE *gra_eventqueuetail;		/* points to first free event in queue */

static INTBIG        gra_inputstate;			/* current state of device input */
static INTBIG        gra_inputspecial;			/* current "special" keyboard value */
static INTBIG        gra_doublethresh;			/* threshold of double-click */
static INTBIG        gra_cursorx, gra_cursory;	/* current position of mouse */
static INTBIG        gra_lstcurx, gra_lstcury;	/* former position of mouse */
static INTBIG        gra_logrecordcount = 0;	/* count for session flushing */
static INTBIG        gra_lastloggedaction = NOEVENT;
static INTBIG        gra_lastloggedx, gra_lastloggedy;
static INTBIG        gra_lastloggedindex;
static UINTBIG       gra_lastplaybacktime = 0;	/* offset to system time */
static UINTBIG       gra_logbasetime;
static UINTBIG       gra_eventtime;				/* time that the current event was queued */
static INTBIG        gra_firstbuttonwait = 0;
static INTBIG        gra_lasteventstate = 0;	/* last event state (for repeating) */
static INTBIG        gra_lasteventx;			/* X of last event (for repeating) */
static INTBIG        gra_lasteventy;			/* Y of last event (for repeating) */
static INTBIG        gra_lasteventspecial;		/* special factor of last event (for repeating) */
static INTBIG        gra_lasteventtime;			/* time of last event (for repeating) */
static UINTBIG       gra_timeoffset = 0;		/* offset to system time */
static INTBIG        gra_cureventtime = 0;		/* current time counter (for repeating) */
static XEvent        gra_samplemotionevent;		/* fake event for repeating */
static XEvent        gra_lastbuttonpressevent;	/* last button press event for popup menus */
static Widget        gra_lastwidgetfocused = 0;	/* last widget that got focus (to switch when exposed) */

#if 0		/* for debugging X events */
static CHAR *eventNames[] =
{
	x_(""),
	x_(""),
	x_("KeyPress"),
	x_("KeyRelease"),
	x_("ButtonPress"),
	x_("ButtonRelease"),
	x_("MotionNotify"),
	x_("EnterNotify"),
	x_("LeaveNotify"),
	x_("FocusIn"),
	x_("FocusOut"),
	x_("KeymapNotify"),
	x_("Expose"),
	x_("GraphicsExpose"),
	x_("NoExpose"),
	x_("VisibilityNotify"),
	x_("CreateNotify"),
	x_("DestroyNotify"),
	x_("UnmapNotify"),
	x_("MapNotify"),
	x_("MapRequest"),
	x_("ReparentNotify"),
	x_("ConfigureNotify"),
	x_("ConfigureRequest"),
	x_("GravityNotify"),
	x_("ResizeRequest"),
	x_("CirculateNotify"),
	x_("CirculateRequest"),
	x_("PropertyNotify"),
	x_("SelectionClear"),
	x_("SelectionRequest"),
	x_("SelectionNotify"),
	x_("ColormapNotify"),
	x_("ClientMessage"),
	x_("MappingNotify")
};
#endif

void         gra_nextevent(void);
void         gra_repaint(WINDOWFRAME *wf, BOOLEAN redo);
void         gra_recalcsize(WINDOWFRAME *wf, INTBIG wid, INTBIG hei);
void         gra_addeventtoqueue(INTBIG state, INTBIG special, INTBIG x, INTBIG y);
void         gra_pickupnextevent(void);
void         gra_graphics_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont);
void         gra_messages_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont);
Boolean      gra_dowork(XtPointer client_data);
void         gra_timerticked(XtPointer closure, XtIntervalId *id);
INTBIG       gra_translatekey(XEvent *event, INTBIG *special);
RETSIGTYPE   gra_sigill_trap(void);
RETSIGTYPE   gra_sigfpe_trap(void);
RETSIGTYPE   gra_sigbus_trap(void);
RETSIGTYPE   gra_sigsegv_trap(void);
WINDOWFRAME *gra_getcurrentwindowframe(Widget widget, BOOLEAN set);
void         gra_windowdelete(Widget w, XtPointer client_data, XtPointer *call_data);

/****** mouse buttons ******/
#define BUTTONS     45		/* cannot exceed NUMBUTS in "usr.h" */
#define REALBUTS    5		/* actual number of buttons */

struct {
	CHAR  *name;				/* button name */
	INTBIG unique;				/* number of letters that make it unique */
} gra_buttonname[BUTTONS] =
{
	{x_("LEFT"),1},   {x_("MIDDLE"),2},   {x_("RIGHT"),1},   {x_("FORWARD"),1},   {x_("BACKWARD"),1},	/* unshifted */
	{x_("SLEFT"),2},  {x_("SMIDDLE"),3},  {x_("SRIGHT"),2},  {x_("SFORWARD"),2},  {x_("SBACKWARD"),2},	/* shift held down */
	{x_("CLEFT"),2},  {x_("CMIDDLE"),3},  {x_("CRIGHT"),2},  {x_("CFORWARD"),2},  {x_("CBACKWARD"),2},	/* control held down */
	{x_("MLEFT"),2},  {x_("MMIDDLE"),3},  {x_("MRIGHT"),2},  {x_("MFORWARD"),2},  {x_("MBACKWARD"),2},	/* meta held down */
	{x_("SCLEFT"),3}, {x_("SCMIDDLE"),4}, {x_("SCRIGHT"),3}, {x_("SCFORWARD"),3}, {x_("SCBACKWARD"),3},	/* shift and control held down*/
	{x_("SMLEFT"),3}, {x_("SMMIDDLE"),4}, {x_("SMRIGHT"),3}, {x_("SMFORWARD"),3}, {x_("SMBACKWARD"),3},	/* shift and meta held down */
	{x_("CMLEFT"),3}, {x_("CMMIDDLE"),4}, {x_("CMRIGHT"),3}, {x_("CMFORWARD"),3}, {x_("CMBACKWARD"),3},	/* control and meta held down */
	{x_("SCMLEFT"),4},{x_("SCMMIDDLE"),4},{x_("SCMRIGHT"),4},{x_("SCMFORWARD"),4},{x_("SCMBACKWARD"),4},/* shift, control, and meta */
	{x_("DLEFT"),2},  {x_("DMIDDLE"),2},  {x_("DRIGHT"),2},  {x_("DFORWARD"),2},  {x_("DBACKWARD"),2}	/* double-click */
};

INTBIG     gra_makebutton(INTBIG);

/****** cursors and icons ******/
static Cursor        gra_realcursor;
static Cursor        gra_nomousecursor;
static Cursor        gra_drawcursor;
static Cursor        gra_nullcursor;
static Cursor        gra_menucursor;
static Cursor        gra_handcursor;
static Cursor        gra_techcursor;
static Cursor        gra_ibeamcursor;
static Cursor        gra_lrcursor;
static Cursor        gra_udcursor;
static XColor        gra_xfc, gra_xbc;			/* cursor color structure  */

/* the icon for Electric  */
#ifdef sun
#  define PROGICONSIZE 32
  static UINTSML gra_icon[64] =
  {
	0x0000, 0x0000, /*                                  */
	0x0000, 0x0000, /*                                  */
	0xFF80, 0x01FF, /*        XXXXXXXXXXXXXXXXXX        */
	0x0040, 0x0200, /*       X                  X       */
	0x0020, 0x0400, /*      X                    X      */
	0x0010, 0x0800, /*     X                      X     */
	0x0008, 0x1000, /*    X                        X    */
	0x0008, 0x1000, /*    X                        X    */
	0x0004, 0x2000, /*   X                          X   */
	0x0784, 0x21E0, /*   X    XXXX          XXXX    X   */
	0x0782, 0x41E0, /*  X     XXXX          XXXX     X  */
	0x0782, 0x41E0, /*  X     XXXX          XXXX     X  */
	0x0781, 0x81E0, /* X      XXXX          XXXX      X */
	0x0781, 0x81E0, /* X      XXXX          XXXX      X */
	0x0781, 0x81E0, /* X      XXXX          XXXX      X */
	0x0781, 0x81E0, /* X      XXXX          XXXX      X */
	0x0781, 0x81E0, /* X      XXXX          XXXX      X */
	0x0781, 0x81E0, /* X      XXXX          XXXX      X */
	0x0781, 0x8000, /* X      XXXX                    X */
	0x0001, 0x8000, /* X                              X */
	0x8002, 0x4001, /*  X             XX             X  */
	0xC002, 0x4003, /*  X            XXXX            X  */
	0xE004, 0x2007, /*   X          XXXXXX          X   */
	0xE004, 0x2007, /*   X          XXXXXX          X   */
	0xE008, 0x1007, /*    X         XXXXXX         X    */
	0xE008, 0x1007, /*    X         XXXXXX         X    */
	0x0010, 0x0800, /*     X                      X     */
	0x0020, 0x0400, /*      X                    X      */
	0x0040, 0x0200, /*       X                  X       */
	0xFF80, 0x01FF, /*        XXXXXXXXXXXXXXXXXX        */
	0x0000, 0x0000, /*                                  */
	0x0000, 0x0000  /*                                  */
  };
#else
#  define PROGICONSIZE 64
  static UINTSML gra_icon[256] =
  {
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0xF800, 0xFFFF, 0xFFFF, 0x001F, /*            XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX            */
	0x0400, 0x0000, 0x0000, 0x0020, /*           X                                          X           */
	0x0200, 0x0000, 0x0000, 0x0040, /*          X                                            X          */
	0x0100, 0x0000, 0x0000, 0x0080, /*         X                                              X         */
	0x0080, 0x0000, 0x0000, 0x0100, /*        X                                                X        */
	0x0080, 0x0000, 0x0000, 0x0100, /*        X                                                X        */
	0x0040, 0x0000, 0x0000, 0x0200, /*       X                                                  X       */
	0x0040, 0x0000, 0x0000, 0x0200, /*       X                                                  X       */
	0x0020, 0x0000, 0x0000, 0x0400, /*      X                                                    X      */
	0x0020, 0x0000, 0x0000, 0x0400, /*      X                                                    X      */
	0x8010, 0x001F, 0xFC00, 0x0800, /*     X          XXXXXX                     XXXXXX           X     */
	0x8010, 0x001F, 0xFC00, 0x0800, /*     X          XXXXXX                     XXXXXX           X     */
	0x8008, 0x001F, 0xFC00, 0x1000, /*    X           XXXXXX                     XXXXXX            X    */
	0x8008, 0x001F, 0xFC00, 0x1000, /*    X           XXXXXX                     XXXXXX            X    */
	0x8004, 0x001F, 0xFC00, 0x2000, /*   X            XXXXXX                     XXXXXX             X   */
	0x8004, 0x001F, 0xFC00, 0x2000, /*   X            XXXXXX                     XXXXXX             X   */
	0x8004, 0x001F, 0xFC00, 0x2000, /*   X            XXXXXX                     XXXXXX             X   */
	0x8002, 0x001F, 0xFC00, 0x4000, /*  X             XXXXXX                     XXXXXX              X  */
	0x8002, 0x001F, 0xFC00, 0x4000, /*  X             XXXXXX                     XXXXXX              X  */
	0x8002, 0x001F, 0xFC00, 0x4000, /*  X             XXXXXX                     XXXXXX              X  */
	0x8002, 0x001F, 0xFC00, 0x4000, /*  X             XXXXXX                     XXXXXX              X  */
	0x8002, 0x001F, 0xFC00, 0x4000, /*  X             XXXXXX                     XXXXXX              X  */
	0x8001, 0x001F, 0xFC00, 0x8000, /* X              XXXXXX                     XXXXXX               X */
	0x8001, 0x001F, 0xFC00, 0x8000, /* X              XXXXXX                     XXXXXX               X */
	0x8001, 0x001F, 0xFC00, 0x8000, /* X              XXXXXX                     XXXXXX               X */
	0x8001, 0x001F, 0xFC00, 0x8000, /* X              XXXXXX                     XXXXXX               X */
	0x8001, 0x001F, 0xFC00, 0x8000, /* X              XXXXXX                     XXXXXX               X */
	0x8001, 0x001F, 0xFC00, 0x8000, /* X              XXXXXX                     XXXXXX               X */
	0x0001, 0x0000, 0x0000, 0x8000, /* X                                                              X */
	0x0001, 0x0000, 0x0000, 0x8000, /* X                                                              X */
	0x0001, 0x0000, 0x0000, 0x8000, /* X                                                              X */
	0x0001, 0x0000, 0x0000, 0x8000, /* X                                                              X */
	0x0001, 0x0000, 0x0000, 0x8000, /* X                                                              X */
	0x0001, 0xF000, 0x0007, 0x8000, /* X                           XXXXXXX                            X */
	0x0002, 0xF800, 0x000F, 0x4000, /*  X                         XXXXXXXXX                          X  */
	0x0002, 0xFC00, 0x001F, 0x4000, /*  X                        XXXXXXXXXXX                         X  */
	0x0002, 0xFE00, 0x003F, 0x4000, /*  X                       XXXXXXXXXXXXX                        X  */
	0x0002, 0xFE00, 0x003F, 0x4000, /*  X                       XXXXXXXXXXXXX                        X  */
	0x0002, 0xFE00, 0x003F, 0x4000, /*  X                       XXXXXXXXXXXXX                        X  */
	0x0004, 0xFE00, 0x003F, 0x2000, /*   X                      XXXXXXXXXXXXX                       X   */
	0x0004, 0xFE00, 0x003F, 0x2000, /*   X                      XXXXXXXXXXXXX                       X   */
	0x0004, 0xFE00, 0x003F, 0x2000, /*   X                      XXXXXXXXXXXXX                       X   */
	0x0008, 0xFE00, 0x003F, 0x1000, /*    X                     XXXXXXXXXXXXX                      X    */
	0x0008, 0xFE00, 0x003F, 0x1000, /*    X                     XXXXXXXXXXXXX                      X    */
	0x0010, 0xFE00, 0x003F, 0x0800, /*     X                    XXXXXXXXXXXXX                     X     */
	0x0010, 0xFE00, 0x003F, 0x0800, /*     X                    XXXXXXXXXXXXX                     X     */
	0x0020, 0xFE00, 0x003F, 0x0400, /*      X                   XXXXXXXXXXXXX                    X      */
	0x0020, 0x0000, 0x0000, 0x0400, /*      X                                                    X      */
	0x0040, 0x0000, 0x0000, 0x0200, /*       X                                                  X       */
	0x0040, 0x0000, 0x0000, 0x0200, /*       X                                                  X       */
	0x0080, 0x0000, 0x0000, 0x0100, /*        X                                                X        */
	0x0080, 0x0000, 0x0000, 0x0100, /*        X                                                X        */
	0x0100, 0x0000, 0x0000, 0x0080, /*         X                                              X         */
	0x0200, 0x0000, 0x0000, 0x0040, /*          X                                            X          */
	0x0400, 0x0000, 0x0000, 0x0020, /*           X                                          X           */
	0xF800, 0xFFFF, 0xFFFF, 0x001F, /*            XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX            */
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0x0000, 0x0000, 0x0000, 0x0000, /*                                                                  */
	0x0000, 0x0000, 0x0000, 0x0000  /*                                                                  */
  };
#endif

/* the normal cursor (NORMALCURSOR, an "X") */
static UINTSML gra_realcursordata[16] = {
	0x0000, /*                  */
	0x4002, /*  X            X  */
	0x2004, /*   X          X   */
	0x1008, /*    X        X    */
	0x0810, /*     X      X     */
	0x0420, /*      X    X      */
	0x0240, /*       X  X       */
	0x0180, /*        XX        */	
	0x0180, /*        XX        */	
	0x0240, /*       X  X       */
	0x0420, /*      X    X      */
	0x0810, /*     X      X     */
	0x1008, /*    X        X    */
	0x2004, /*   X          X   */
	0x4002, /*  X            X  */
	0x0000  /*                  */
};
static UINTSML gra_realcursormask[16] = {
	0xC003, /* XX            XX */
	0xE007, /* XXX          XXX */
	0x700E, /*  XXX        XXX  */
	0x381C, /*   XXX      XXX   */
	0x1C38, /*    XXX    XXX    */
	0x0E70, /*     XXX  XXX     */
	0x07E0, /*      XXXXXX      */
	0x03E0, /*       XXXX       */
	0x03E0, /*       XXXX       */
	0x07E0, /*      XXXXXX      */
	0x0E70, /*     XXX  XXX     */
	0x1C38, /*    XXX    XXX    */
	0x381C, /*   XXX      XXX   */
	0x700E, /*  XXX        XXX  */
	0xE007, /* XXX          XXX */
	0xC003  /* XX            XX */
};

/* the "use the TTY" cursor (WANTTTYCURSOR, the word "tty" above keyboard) */
static UINTSML gra_nomousecursordata[16] = {
	0x7757, /* XXX X X XXX XXX  */
	0x1552, /*  X  X X X X X    */
	0x3772, /*  X  XXX XXX XX   */
	0x1122, /*  X   X  X   X    */
	0x7122, /*  X   X  X   XXX  */
	0x0000, /*                  */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0x8001, /* X              X */
	0x8AA9, /* X  X X X X X   X */
	0x9551, /* X   X X X X X  X */
	0x8AA9, /* X  X X X X X   X */
	0x9551, /* X   X X X X X  X */
	0x8001, /* X              X */
	0x9FF9, /* X  XXXXXXXXXX  X */
	0x8001, /* X              X */
	0xFFFF  /* XXXXXXXXXXXXXXXX */
};
static UINTSML gra_nomousecursormask[16] = {
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0x0000, /*                  */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF  /* XXXXXXXXXXXXXXXX */
};

/* the "draw with pen" cursor (PENCURSOR, a pen) */
static UINTSML gra_drawcursordata[16] = {
	0x3000, /*             XX   */
	0x7800, /*            XXXX  */
	0xF400, /*           X XXXX */
	0xE200, /*          X   XXX */
	0x4100, /*         X     X  */
	0x2080, /*        X     X   */
	0x1040, /*       X     X    */
	0x0820, /*      X     X     */
	0x0410, /*     X     X      */
	0x0208, /*    X     X       */
	0x010C, /*   XX    X        */
	0x008C, /*   XX   X         */
	0x007E, /*  XXXXXX          */
	0x003E, /*  XXXXX           */
	0x000F, /* XXXX             */
	0x0003  /* XX               */
};
static UINTSML gra_drawcursormask[16] = {
	0x7800, /*            XXXX  */
	0xFC00, /*           XXXXXX */
	0xFE00, /*          XXXXXXX */
	0xFF00, /*         XXXXXXXX */
	0xFF80, /*        XXXXXXXXX */
	0x7FC0, /*       XXXXXXXXX  */
	0x3FE0, /*      XXXXXXXXX   */
	0x1FF0, /*     XXXXXXXXX    */
	0x0FF8, /*    XXXXXXXXX     */
	0x07FC, /*   XXXXXXXXX      */
	0x03FE, /*  XXXXXXXXX       */
	0x01FE, /*  XXXXXXXX        */
	0x00FF, /* XXXXXXXX         */
	0x007F, /* XXXXXXX          */
	0x001F, /* XXXXX            */
	0x0007  /* XXX              */
};

/* the null cursor (NULLCURSOR, an egg timer) */
static UINTSML gra_nullcursordata[16] = {
	0x1FF8, /*    XXXXXXXXXX    */
	0x2004, /*   X          X   */
	0x2004, /*   X          X   */
	0x2004, /*   X          X   */
	0x1AA8, /*    X X X X XX    */
	0x0FF0, /*     XXXXXXXX     */
	0x07E0, /*      XXXXXX      */
	0x03C0, /*       XXXX       */
	0x0240, /*       X  X       */
	0x0420, /*      X    X      */
	0x0810, /*     X      X     */
	0x1008, /*    X        X    */
	0x2AA4, /*   X  X X X X X   */
	0x3FFC, /*   XXXXXXXXXXXX   */
	0x3FFC, /*   XXXXXXXXXXXX   */
	0x1FF8  /*    XXXXXXXXXX    */
};
static UINTSML gra_nullcursormask[16] = {
	0x3FFC, /*   XXXXXXXXXXXX   */
	0x7FFE, /*  XXXXXXXXXXXXXX  */
	0x7FFE, /*  XXXXXXXXXXXXXX  */
	0x7FFE, /*  XXXXXXXXXXXXXX  */
	0x3FFC, /*   XXXXXXXXXXXX   */
	0x1FF8, /*    XXXXXXXXXX    */
	0x0FF0, /*     XXXXXXXX     */
	0x07E0, /*      XXXXXX      */
	0x07E0, /*      XXXXXX      */
	0x0FF0, /*     XXXXXXXX     */
	0x1FF8, /*    XXXXXXXXXX    */
	0x3FFC, /*   XXXXXXXXXXXX   */
	0x7FFE, /*  XXXXXXXXXXXXXX  */
	0x7FFE, /*  XXXXXXXXXXXXXX  */
	0x7FFE, /*  XXXXXXXXXXXXXX  */
	0x3FFC  /*   XXXXXXXXXXXX   */
};

/* the menu selection cursor (MENUCURSOR, a sideways arrow) */
static UINTSML gra_menucursordata[16] = {
	0x0000, /*                  */
	0x0000, /*                  */
	0x0400, /*           X      */
	0x0C00, /*           XX     */
	0x1C00, /*           XXX    */
	0x3C00, /*           XXXX   */
	0x7FFF, /* XXXXXXXXXXXXXXX  */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0x7FFF, /* XXXXXXXXXXXXXXX  */
	0x3C00, /*           XXXX   */
	0x1C00, /*           XXX    */
	0x0C00, /*           XX     */
	0x0400, /*           X      */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000  /*                  */
};
static UINTSML gra_menucursormask[16] = {
	0x0000, /*                  */
	0x0400, /*           X      */
	0x0E00, /*          XXX     */
	0x1E00, /*          XXXX    */
	0x3E00, /*          XXXXX   */
	0x7EFF, /* XXXXXXXXXXXXXXX  */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0x7EFF, /* XXXXXXXXXXXXXXX  */
	0x3E00, /*          XXXXX   */
	0x1E00, /*          XXXX    */
	0x0E00, /*          XXX     */
	0x0400, /*           X      */
	0x0000, /*                  */
	0x0000  /*                  */
};

/* the "move" command dragging cursor (HANDCURSOR, a dragging hand) */
static UINTSML gra_handcursordata[16] = {
	0x0000, /*                  */
	0x3F80, /*        XXXXXXX   */
	0xC040, /*       X       XX */
	0x0380, /*        XXX       */
	0x0040, /*       X          */
	0x0020, /*      X           */
	0x01C0, /*       XXX        */
	0x0020, /*      X           */
	0x0010, /*     X            */
	0x03FE, /*  XXXXXXXXX       */
	0x0001, /* X                */
	0x0001, /* X                */
	0x1FFE, /*  XXXXXXXXXXXX    */
	0x8080, /*        X       X */
	0x4100, /*         X     X  */
	0x3E00  /*          XXXXX   */
};
static UINTSML gra_handcursormask[16] = {
	0x3F80, /*        XXXXXXX   */
	0xFFC0, /*       XWWWWWWWXX */
	0xFFE0, /*      XWXXXXXXXWW */
	0xFFC0, /*       XWWWXXXXXX */
	0xFFE0, /*      XWXXXXXXXXX */
	0xFFF0, /*     XWXXXXXXXXXX */
	0xFFE0, /*      XWWWXXXXXXX */
	0xFFF0, /*     XWXXXXXXXXXX */
	0xFFFF, /* XXXXWXXXXXXXXXXX */
	0xFFFF, /* XWWWWWWWWWXXXXXX */
	0xFFFF, /* WXXXXXXXXXXXXXXX */
	0xFFFF, /* WXXXXXXXXXXXXXXX */
	0xFFFF, /* XWWWWWWWWWWWWXXX */
	0xFFFE, /*  XXXXXXWXXXXXXXW */
	0xFF80, /*        XWXXXXXWX */
	0x7F00  /*         XWWWWWX  */
};

/* the "technology" command dragging cursor (TECHCURSOR, a "T") */
static UINTSML gra_techcursordata[16] = {
	0x0100, /*         X        */
	0x0380, /*        XXX       */
	0x0540, /*       X X X      */
	0x0100, /*         X        */
	0x0100, /*         X        */
	0x3FF8, /*    XXXXXXXXXXX   */
	0x3FF8, /*    XXXXXXXXXXX   */
	0x2388, /*    X   XXX   X   */
	0x0380, /*        XXX       */
	0x0380, /*        XXX       */
	0x0380, /*        XXX       */
	0x0380, /*        XXX       */
	0x0380, /*        XXX       */
	0x0380, /*        XXX       */
	0x07C0, /*       XXXXX      */
	0x0000  /*                  */
};
static UINTSML gra_techcursormask[16] = {
	0x0380, /*        XXX       */
	0x07C0, /*       XXXXX      */
	0x0FE0, /*      XXXXXXX     */
	0x0380, /*        XXX       */
	0x3FF8, /*    XXXXXXXXXXX   */
	0x7FFC, /*   XXXXXXXXXXXXX  */
	0x7FFC, /*   XXXXXXXXXXXXX  */
	0x7FFC, /*   XXXXXXXXXXXXX  */
	0x07C0, /*       XXXXX      */
	0x07C0, /*       XXXXX      */
	0x07C0, /*       XXXXX      */
	0x07C0, /*       XXXXX      */
	0x07C0, /*       XXXXX      */
	0x07C0, /*       XXXXX      */
	0x0EE0, /*      XXXXXXX     */
	0x07C0  /*       XXXXX      */
};

/* the "ibeam" text selection cursor (IBEAMCURSOR, an "I") */
static UINTSML gra_ibeamcursordata[16] = {
	0x0360, /*      XX XX       */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0080, /*        X         */
	0x0360  /*      XX XX       */
};
static UINTSML gra_ibeamcursormask[16] = {
	0x07F0, /*     XXXXXXX      */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x01C0, /*       XXX        */
	0x07F0  /*     XXXXXXX      */
};

/* the "left/right arrow" cursor (LRCURSOR) */
static UINTSML gra_lrcursordata[16] = {
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000, /*                  */
	0x1008, /*    X        X    */
	0x300C, /*   XX        XX   */
	0x700E, /*  XXX        XXX  */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0x700E, /*  XXX        XXX  */
	0x300C, /*   XX        XX   */
	0x1008, /*    X        X    */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000  /*                  */
};
static UINTSML gra_lrcursormask[16] = {
	0x0000, /*                  */
	0x0000, /*                  */
	0x1008, /*    X        X    */
	0x381C, /*   XXX      XXX   */
	0x781E, /*  XXXX      XXXX  */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0xFFFF, /* XXXXXXXXXXXXXXXX */
	0x781E, /*  XXXX      XXXX  */
	0x381C, /*   XXX      XXX   */
	0x1008, /*    X        X    */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0000  /*                  */
};

/* the "up/down arrow" cursor (UDCURSOR) */
static UINTSML gra_udcursordata[16] = {
	0x0180, /*        XX        */
	0x03C0, /*       XXXX       */
	0x07E0, /*      XXXXXX      */
	0x0FF0, /*     XXXXXXXX     */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0180, /*        XX        */
	0x0FF0, /*     XXXXXXXX     */
	0x07E0, /*      XXXXXX      */
	0x03C0, /*       XXXX       */
	0x0180  /*        XX        */
};
static UINTSML gra_udcursormask[16] = {
	0x03C0, /*       XXXX       */
	0x07E0, /*      XXXXXX      */
	0x0FF0, /*     XXXXXXXX     */
	0x1FF8, /*    XXXXXXXXXX    */
	0x0FF0, /*     XXXXXXXX     */
	0x03C0, /*       XXXX       */
	0x03C0, /*       XXXX       */
	0x03C0, /*       XXXX       */
	0x03C0, /*       XXXX       */
	0x03C0, /*       XXXX       */
	0x03C0, /*       XXXX       */
	0x0FF0, /*     XXXXXXXX     */
	0x1FF8, /*    XXXXXXXXXX    */
	0x0FF0, /*     XXXXXXXX     */
	0x07E0, /*      XXXXXX      */
	0x03C0  /*       XXXX       */
};

void       gra_recolorcursor(Display *dpy, XColor *fg, XColor *bg);

/****** fonts ******/

/*
 * Default font - this font MUST exist, or Electric will die.  Best to choose a
 * font that is commonly used, so the X server is likely to have it loaded.
 */
#define DEFAULTFONTNAME      x_("fixed")
#define FONTLISTSIZE           10
#define FONTHASHSIZE          211		/* must be prime */
#define TRUETYPEITALICSLANT   0.3		/* slant to make text italic */
#define TRUETYPEBOLDEXTEND    1.5		/* extend factor to make text bold */

typedef struct
{
	XFontStruct *font;
	CHAR        *fontname;
	CHAR        *altfontname;
#ifdef ANYDEPTH
	INTSML       width[128];
	INTSML       height[128];
	UCHAR1      *data[128];
#endif
} FontRec;

typedef struct
{
	INTBIG   ascent, descent;
	INTBIG   face;
	INTBIG   italic;
	INTBIG   bold;
	INTBIG   size;
	INTBIG   spacewidth;
} FONTHASH;

       FONTHASH      gra_fonthash[FONTHASHSIZE];
static INTBIG        gra_textrotation = 0;

#ifdef ANYDEPTH
#  define FONTINIT , {0}, {0}, {0}
  static UCHAR1     *gra_textbitsdata;				/* buffer for converting text to bits */
  static INTBIG      gra_textbitsdatasize = 0;		/* size of "gra_textbitsdata" */
  static UCHAR1    **gra_textbitsrowstart;			/* pointers to "gra_textbitsdata" */
  static INTBIG      gra_textbitsrowstartsize = 0;	/* size of "gra_textbitsrowstart" */
#else
#  define FONTINIT
#endif

static FontRec       gra_messages_font = {(XFontStruct *)0, x_("fixed"), x_("fixed") FONTINIT};
static FontRec       gra_defaultFont;
static XFontStruct  *gra_curfont;					/* current writing font */
static INTBIG        gra_curfontnumber;
static INTBIG        gra_truetypesize;				/* true size of current font */
static BOOLEAN       gra_texttoosmall = FALSE;		/* TRUE if the text is too small to draw */
#ifdef TRUETYPE
  static INTBIG      gra_truetypeon;				/* nonzero if TrueType initialized */
  static INTBIG      gra_truetypeitalic;			/* true italic factor of current font */
  static INTBIG      gra_truetypebold;				/* true bold factor of current font */
  static INTBIG      gra_truetypeunderline;			/* true underline factor of current font */
  static INTBIG      gra_truetypedescent;			/* true descent value of current font */
  static INTBIG      gra_truetypeascent;			/* true ascent value of current font */
  static INTBIG      gra_spacewidth;				/* true width of space for current font */
  static INTBIG      gra_truetypedeffont;			/* default TrueType font */
  static INTBIG      gra_truetypefont;				/* current TrueType font */
  static INTBIG     *gra_descentcache;				/* cache of maximum descent for font size */
  static INTBIG     *gra_ascentcache;				/* cache of maximum ascent for font size */
  static INTBIG     *gra_swidthcache;				/* cache of space width for font size */
  static INTBIG      gra_descentcachesize = 0;		/* size of ascent/descent cache */
  static INTBIG      gra_numfaces = 0;
  static CHAR      **gra_facelist;
#endif

static CHAR *gra_resource_fontname[] =
{
	x_("font0"), x_("font1"), x_("font2"), x_("font3"), x_("font4"), x_("font5"),
	x_("font6"), x_("font7"), x_("font8"), x_("fontmenu"), x_("fontedit"), x_("fontstatus")
};

static FontRec gra_font[] =
{
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-8-*-*-*-*"),				/* 4 points */
	 x_("-*-helvetica-medium-r-normal-*-8-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-10-*-*-*-*"),				/* 6 points */
	 x_("-*-helvetica-medium-r-normal-*-10-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-12-*-*-*-*"),				/* 8 points */
	 x_("-*-helvetica-medium-r-normal-*-12-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-14-*-*-*-*"),				/* 10 points */
	 x_("-*-helvetica-medium-r-normal-*-14-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-17-*-*-*-*"),				/* 12 points */
	 x_("-*-fixed-bold-r-normal-*-16-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-18-*-*-*-*"),				/* 14 points */
	 x_("-*-helvetica-medium-r-normal-*-18-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-bold-r-normal-*-20-*-*-*-*"),					/* 16 points */
	 x_("-*-fixed-medium-r-normal-*-20-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-medium-r-normal-*-24-*-*-*-*"),				/* 18 points */
	 x_("-*-helvetica-medium-r-normal-*-24-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-bold-r-normal-*-25-*-*-*-*"),					/* 20 points */
	 x_("-*-*-medium-r-normal-*-25-*-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-helvetica-bold-r-normal-*-*-120-*-*-*"),				/* MENU */
	 x_("-*-helvetica-bold-r-normal-*-*-120-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("-*-fixed-*-*-normal-*-*-120-*-*-*"),						/* EDIT */
	 x_("-*-fixed-*-*-normal-*-*-120-*-*-*") FONTINIT},
	{(XFontStruct *)0,
	 x_("fixed"),													/* STATUS */
	 x_("fixed") FONTINIT},
	{(XFontStruct *)0, NULL, NULL FONTINIT}  					/* terminator */
};

void     gra_makefontchoices(CHAR1 **fontlist, INTBIG namecount, CHAR *required[14],
			CHAR ****fchoices, INTBIG **fnumchoices, void *dia);
int      gra_sortnumbereascending(const void *e1, const void *e2);
static void gra_settextsize(WINDOWPART *win, INTBIG fnt, INTBIG face, INTBIG italic,
			INTBIG bold, INTBIG underline, INTBIG rotation);
#ifdef TRUETYPE
  INTBIG gra_getT1stringsize(CHAR *str, INTBIG *wid, INTBIG *hei);
#endif

#ifndef ANYDEPTH
/****** rectangle saving ******/
#define NOSAVEDBOX ((SAVEDBOX *)-1)
typedef struct Isavedbox
{
	Pixmap      pix;
	WINDOWPART *win;
	INTBIG      code;
	INTBIG      lx, hx, ly, hy;
	struct Isavedbox *nextsavedbox;
} SAVEDBOX;

static SAVEDBOX     *gra_firstsavedbox = NOSAVEDBOX;
static INTBIG        gra_savedboxcodes = 0;
#endif

/****** time ******/
static Time          gra_lasttime = 0;			/* time of last click */
static time_t        gra_timebasesec;
#ifdef HAVE_FTIME
  static INTBIG      gra_timebasems;
#endif

/****** files ******/
static void         *gra_fileliststringarray = 0;
static CHAR          gra_curpath[255] = {0};
static CHAR          gra_requiredextension[20];
static CHAR         *gra_initialdirectory;
static CHAR         *gra_logfile, *gra_logfilesave;
static CHAR        **gra_printerlist;
static INTBIG        gra_printerlistcount = 0;
static INTBIG        gra_printerlisttotal = 0;
static BOOLEAN       gra_fileselectdone;

int        gra_fileselectall(const struct dirent *a);
BOOLEAN    gra_addfiletolist(CHAR *file);
void       gra_initfilelist(void);
void       gra_setcurrentdirectory(CHAR *path);
BOOLEAN    gra_addprinter(CHAR *buf);
void       gra_fileselectcancel(Widget w, XtPointer client_data,
			XmSelectionBoxCallbackStruct *call_data);
void       gra_fileselectok(Widget w, XtPointer client_data,
			XmSelectionBoxCallbackStruct *call_data);

/******************** INITIALIZATION ********************/

int main(int argc, CHAR1 *argv[])
{
	/* catch signals if Electric trys to bomb out */
	(void)signal(SIGILL, (SIGNALCAST)gra_sigill_trap);
	(void)signal(SIGFPE, (SIGNALCAST)gra_sigfpe_trap);
	(void)signal(SIGBUS, (SIGNALCAST)gra_sigbus_trap);
	(void)signal(SIGSEGV, (SIGNALCAST)gra_sigsegv_trap);

	/* primary initialization of Electric */
	osprimaryosinit();

#if LANGTCL
	/* initialize TCL here */
	if (gra_initializetcl() == TCL_ERROR)
		error(_("Failed to initialize TCL: %s\n"), gra_tclinterp->result);
#endif

	/* secondary initialization of Electric */
	ossecondaryinit(argc, argv);

	/* add a timer that ticks TICKSPERSECOND times per second */
	XtAppAddTimeOut(gra_xtapp, 1000/TICKSPERSECOND, gra_timerticked, 0);

	/* now run Electric */
	(void)XtAppAddWorkProc(gra_xtapp, (XtWorkProc)gra_dowork, 0);
	XtAppMainLoop(gra_xtapp);
	return(0);
}

/*
 * Routine to establish the default display define tty codes.
 */
void graphicsoptions(CHAR *name, INTBIG *argc, CHAR1 **argv)
{
	INTBIG ttydevice;

	/* save command arguments */
	gra_argc = *argc;
	gra_argv = argv;

	/* initialize keyboard input */
	ttydevice = fileno(stdin);
#ifdef HAVE_TERMIOS_H
	{
		struct termios sttybuf;
		tcgetattr(ttydevice, &sttybuf);
		us_erasech = sttybuf.c_cc[VERASE];
		us_killch = sttybuf.c_cc[VKILL];
	}
#else
#  ifdef HAVE_TERMIO_H
	{
		struct termio sttybuf;
		(void)ioctl(ttydevice, TCGETA, &sttybuf);
		us_erasech = sttybuf.c_cc[VERASE];
		us_killch = sttybuf.c_cc[VKILL];
	}
#  else
	{
		struct sgttyb sttybuf;
		(void)gtty(ttydevice, &sttybuf);
		us_erasech = sttybuf.sg_erase;
		us_killch = sttybuf.sg_kill;
	}
#  endif
#endif
}

/*
 * Routine to initialize the display device.
 */
BOOLEAN initgraphics(BOOLEAN messages)
{
	REGISTER INTBIG i, j, ac;
	INTBIG bitmask, depth;
	BOOLEAN wantmultiscreen, multiscreendebug;
	int namecount;
	CHAR *ptr;
	CHAR1 **fontlist;
	CHAR1 *geomSpec;
	Pixmap pixm, pixmm;
	Visual *visual;
	Arg arg[10];
	Widget dpywidget;
#ifdef ANYDEPTH
	GC gc;
	XGCValues gcv;
	XImage *image;
	XFontStruct *thefont;
	XCharStruct xcs;
	int direction, asc, des;
	UCHAR1 *dptr;
	REGISTER INTBIG amt, k;
	INTBIG width, height, x, y;
	Pixmap textmap;
#else
	XVisualInfo vinfo;
#endif
	REGISTER void *infstr;

	/* remember the initial directory and the log file locations */
	(void)allocstring(&gra_initialdirectory, currentdirectory(), db_cluster);
	infstr = initinfstr();
	addstringtoinfstr(infstr, gra_initialdirectory);
	addstringtoinfstr(infstr, x_(".electric.log"));
	(void)allocstring(&gra_logfile, returninfstr(infstr), db_cluster);
	infstr = initinfstr();
	addstringtoinfstr(infstr, gra_initialdirectory);
	addstringtoinfstr(infstr, x_(".electriclast.log"));
	(void)allocstring(&gra_logfilesave, returninfstr(infstr), db_cluster);

	/* initialize X and the toolkit */
	(void)XSetErrorHandler(gra_xerrors);
	XtToolkitInitialize();
	gra_xtapp = XtCreateApplicationContext();
	gra_intsignalid = XtAppAddSignal(gra_xtapp, (XtSignalCallbackProc)gra_intsignalfunc, 0);
	gra_checkinginterrupt = FALSE;
	gra_tracking = FALSE;

	/* on machines with reverse byte ordering, fix the icon and the cursors */
#ifdef BYTES_SWAPPED
	j = PROGICONSIZE * PROGICONSIZE / 16;
	for(i=0; i<j; i++)
		gra_icon[i] = ((gra_icon[i] >> 8) & 0xFF) | ((gra_icon[i] << 8) & 0xFF00);
	for(i=0; i<16; i++)
	{
		gra_realcursordata[i] = ((gra_realcursordata[i] >> 8) & 0xFF) |
			((gra_realcursordata[i] << 8) & 0xFF00);
		gra_realcursormask[i] = ((gra_realcursormask[i] >> 8) & 0xFF) |
			((gra_realcursormask[i] << 8) & 0xFF00);
		gra_nomousecursordata[i] = ((gra_nomousecursordata[i] >> 8) & 0xFF) |
			((gra_nomousecursordata[i] << 8) & 0xFF00);
		gra_nomousecursormask[i] = ((gra_nomousecursormask[i] >> 8) & 0xFF) |
			((gra_nomousecursormask[i] << 8) & 0xFF00);
		gra_drawcursordata[i] = ((gra_drawcursordata[i] >> 8) & 0xFF) |
			((gra_drawcursordata[i] << 8) & 0xFF00);
		gra_drawcursormask[i] = ((gra_drawcursormask[i] >> 8) & 0xFF) |
			((gra_drawcursormask[i] << 8) & 0xFF00);
		gra_nullcursordata[i] = ((gra_nullcursordata[i] >> 8) & 0xFF) |
			((gra_nullcursordata[i] << 8) & 0xFF00);
		gra_nullcursormask[i] = ((gra_nullcursormask[i] >> 8) & 0xFF) |
			((gra_nullcursormask[i] << 8) & 0xFF00);
		gra_menucursordata[i] = ((gra_menucursordata[i] >> 8) & 0xFF) |
			((gra_menucursordata[i] << 8) & 0xFF00);
		gra_menucursormask[i] = ((gra_menucursormask[i] >> 8) & 0xFF) |
			((gra_menucursormask[i] << 8) & 0xFF00);
		gra_handcursordata[i] = ((gra_handcursordata[i] >> 8) & 0xFF) |
			((gra_handcursordata[i] << 8) & 0xFF00);
		gra_handcursormask[i] = ((gra_handcursormask[i] >> 8) & 0xFF) |
			((gra_handcursormask[i] << 8) & 0xFF00);
		gra_techcursordata[i] = ((gra_techcursordata[i] >> 8) & 0xFF) |
			((gra_techcursordata[i] << 8) & 0xFF00);
		gra_techcursormask[i] = ((gra_techcursormask[i] >> 8) & 0xFF) |
			((gra_techcursormask[i] << 8) & 0xFF00);
		gra_ibeamcursordata[i] = ((gra_ibeamcursordata[i] >> 8) & 0xFF) |
			((gra_ibeamcursordata[i] << 8) & 0xFF00);
		gra_ibeamcursormask[i] = ((gra_ibeamcursormask[i] >> 8) & 0xFF) |
			((gra_ibeamcursormask[i] << 8) & 0xFF00);
		gra_lrcursordata[i] = ((gra_lrcursordata[i] >> 8) & 0xFF) |
			((gra_lrcursordata[i] << 8) & 0xFF00);
		gra_lrcursormask[i] = ((gra_lrcursormask[i] >> 8) & 0xFF) |
			((gra_lrcursormask[i] << 8) & 0xFF00);
		gra_udcursordata[i] = ((gra_udcursordata[i] >> 8) & 0xFF) |
			((gra_udcursordata[i] << 8) & 0xFF00);
		gra_udcursormask[i] = ((gra_udcursormask[i] >> 8) & 0xFF) |
			((gra_udcursormask[i] << 8) & 0xFF00);
	}
#endif

	/* get switch settings */
	wantmultiscreen = multiscreendebug = FALSE;
	geomSpec = b_("");
	for(i=1; i < gra_argc; i++)
	{
		if (gra_argv[i][0] == '-')
		{
			switch (gra_argv[i][1])
			{
				case 'M':
					multiscreendebug = TRUE;
				case 'm':
					wantmultiscreen = TRUE;
					continue;

				case 'g':       /* Geometry */
					if (++i >= gra_argc) continue;
					geomSpec = gra_argv[i];
					continue;
			}
		}
	}

	/* find the displays for the edit and messages windows */
	gra_finddisplays(wantmultiscreen, multiscreendebug);

	/* double-click threshold (in milliseconds) */
	gra_doublethresh = XtGetMultiClickTime(gra_maindpy);

	/* get the information about the edit display */
	depth = DefaultDepth(gra_maindpy, gra_mainscreen);
	if (depth == 1)
		efprintf(stderr, _("Cannot run on 1-bit deep displays\n"));
	el_colcursor = CURSOR;	/* also done in usr.c later */
#ifdef ANYDEPTH
	el_maplength = 256;
#else
	el_maplength = DisplayCells(gra_maindpy, gra_mainscreen);
	gra_maphigh = el_maplength - 1;
#endif
	resetpaletteparameters();

	/* Determine initial position and size of edit window */
	gra_editxsh = XAllocSizeHints();
	if (geomSpec == NULL) geomSpec = XGetDefault(gra_maindpy, b_("Electric"), b_("geometry"));
	if (geomSpec[0] != 0)
	{
		/* use defaults from database */
		bitmask = XWMGeometry(gra_maindpy, gra_mainscreen, geomSpec, NULL, 1, gra_editxsh,
			&gra_editxsh->x, &gra_editxsh->y, &gra_editxsh->width, &gra_editxsh->height,
			&gra_editxsh->win_gravity);
		if (bitmask & (XValue | YValue)) gra_editxsh->flags |= USPosition;
		if (bitmask & (WidthValue | HeightValue)) gra_editxsh->flags |= USSize;
	} else
	{
		/* nothing in defaults database */
		gra_editxsh->flags = PPosition | PSize;
		gra_editxsh->height = DisplayHeight(gra_maindpy, gra_mainscreen) - 2;
		gra_editxsh->width = DisplayWidth(gra_maindpy, gra_mainscreen) - PALETTEWIDTH - 8;
		if (gra_maindpy == gra_altdpy)
		{
			/* all on one screen */
			gra_editxsh->height = gra_editxsh->height / 4 * 3;
		} else
		{
			/* two screens */
			gra_editxsh->height = DisplayHeight(gra_maindpy, gra_mainscreen) - 24;
		}
		gra_editxsh->width = gra_editxsh->width / 5 * 4;
		gra_editxsh->height = gra_editxsh->height / 5 * 4;
		gra_editxsh->x = PALETTEWIDTH;
		gra_editxsh->y = 0;
	}
	gra_editposx = gra_editxsh->x;
	gra_editposy = gra_editxsh->y;
	gra_editsizex = gra_editxsh->width;
	gra_editsizey = gra_editxsh->height;
	if (gra_editsizex > gra_editsizey*2)
	{
		/* probably two windows next to each other */
		gra_editsizex /= 2;
	} else if (gra_editsizey > gra_editsizex*2)
	{
		/* probably two windows above each other */
		gra_editsizey /= 2;
	}
	gra_windownumber = 0;

	/* disable drag-and-drop (because it crashes for some unknown reason) */
	dpywidget = XmGetXmDisplay(gra_maindpy);
	ac = 0;
	XtSetArg(arg[ac], XmNdragInitiatorProtocolStyle, XmDRAG_NONE);   ac++;
	XtSetArg(arg[ac], XmNdragReceiverProtocolStyle, XmDRAG_NONE);   ac++;
	XtSetValues(dpywidget, arg, ac);
	if (gra_altdpy != gra_maindpy)
	{
		dpywidget = XmGetXmDisplay(gra_altdpy);
		ac = 0;
		XtSetArg(arg[ac], XmNdragInitiatorProtocolStyle, XmDRAG_NONE);   ac++;
		XtSetArg(arg[ac], XmNdragReceiverProtocolStyle, XmDRAG_NONE);   ac++;
		XtSetValues(dpywidget, arg, ac);
	}

	/* get atoms for disabling window kills, switching displays */
	gra_wm_delete_window = XmInternAtom(gra_maindpy, b_("WM_DELETE_WINDOW"), False);
	gra_movedisplayprotocol = XmInternAtom(gra_maindpy, b_("_MOVE_DISPLAYS"), False);
	gra_movedisplaymsg = XmInternAtom(gra_maindpy, b_("_MOTIF_WM_MESSAGES"), False);

#ifdef ANYDEPTH
	visual = DefaultVisual(gra_maindpy, gra_mainscreen);
	if (VisualClass(visual) == PseudoColor || VisualClass(visual) == StaticColor)
		gra_maincolmap = DefaultColormap(gra_maindpy, gra_mainscreen);
	if (visual->red_mask == 0xFF) gra_mainredshift = 0; else
		if (visual->red_mask == 0xFF00) gra_mainredshift = 8; else
			if (visual->red_mask == 0xFF0000) gra_mainredshift = 16;
	if (visual->green_mask == 0xFF) gra_maingreenshift = 0; else
		if (visual->green_mask == 0xFF00) gra_maingreenshift = 8; else
			if (visual->green_mask == 0xFF0000) gra_maingreenshift = 16;
	if (visual->blue_mask == 0xFF) gra_mainblueshift = 0; else
		if (visual->blue_mask == 0xFF00) gra_mainblueshift = 8; else
			if (visual->blue_mask == 0xFF0000) gra_mainblueshift = 16;
	if (gra_maindpy != gra_altdpy)
	{
		visual = DefaultVisual(gra_altdpy, gra_altscreen);
		if (VisualClass(visual) == PseudoColor || VisualClass(visual) == StaticColor)
			gra_altcolmap = DefaultColormap(gra_altdpy, gra_altscreen);
		if (visual->red_mask == 0xFF) gra_altredshift = 0; else
			if (visual->red_mask == 0xFF00) gra_altredshift = 8; else
				if (visual->red_mask == 0xFF0000) gra_altredshift = 16;
		if (visual->green_mask == 0xFF) gra_altgreenshift = 0; else
			if (visual->green_mask == 0xFF00) gra_altgreenshift = 8; else
				if (visual->green_mask == 0xFF0000) gra_altgreenshift = 16;
		if (visual->blue_mask == 0xFF) gra_altblueshift = 0; else
			if (visual->blue_mask == 0xFF00) gra_altblueshift = 8; else
				if (visual->blue_mask == 0xFF0000) gra_altblueshift = 16;
	}
#else
	/* Some servers change the default screen to StaticColor class, so that
	 * Electric cannot change the color map.  Try to find one that can be used.
	 */
	if (depth > 1)
	{
		visual = DefaultVisual(gra_maindpy, gra_mainscreen);
		if (VisualClass(visual) != PseudoColor)
		{
			/* look for pseudo color only for now */
			if (XMatchVisualInfo(gra_maindpy, gra_mainscreen, depth, PseudoColor, &vinfo))
			{
				visual = vinfo.visual;
			} else
			{
				efprintf(stderr, _("Cannot find 8-bit PseudoColor visual, try building with 'ANYDEPTH'\n"));
			}
		}
	}
#endif

	/* get fonts */
#ifdef TRUETYPE
	for(i=0; i<FONTHASHSIZE; i++) gra_fonthash[i].descent = 1;
	if (T1_SetBitmapPad(16) == 0 && T1_InitLib(NO_LOGFILE) != 0)
	{
		gra_truetypeon = 1;

		/* Preload T1 fonts here, because T1_LoadFont spoils memory */
		for(j=0; j<T1_Get_no_fonts(); j++) T1_LoadFont(j);

		gra_truetypedeffont = 0;		/* presume that the first font is good to use */
		ptr = egetenv(x_("ELECTRIC_TRUETYPE_FONT"));
		if (ptr != NULL)
		{
			j = T1_Get_no_fonts();
			for(gra_truetypedeffont=0; gra_truetypedeffont<j; gra_truetypedeffont++)
			{
				T1_LoadFont(gra_truetypedeffont);
				if (namesame(ptr, string2byte(T1_GetFontName(gra_truetypedeffont))) == 0) break;
			}
			if (gra_truetypedeffont >= j)
			{
				infstr = initinfstr();
				formatinfstr(infstr, 
					_("Warning: environment variable 'ELECTRIC_TRUETYPE_FONT' is '%s' which is not a truetype font.  Choices are:"),
						ptr);
				printf(b_("%s\n"), string1byte(returninfstr(infstr)));

				for(gra_truetypedeffont=0; gra_truetypedeffont<j; gra_truetypedeffont++)
				{
					T1_LoadFont(gra_truetypedeffont);
					printf(b_("  %s\n"), T1_GetFontName(gra_truetypedeffont));
				}
				gra_truetypedeffont = 0;
			}
		}
	} else
	{
		gra_truetypeon = 0;
		efprintf(stderr, _("Cannot load TrueType, is T1LIB_CONFIG set?\n"));
	}
#endif
	gra_defaultFont.fontname = string2byte(XGetDefault(gra_maindpy, b_("Electric"), b_("font")));
	if (gra_defaultFont.fontname == NULL) gra_defaultFont.fontname = DEFAULTFONTNAME;
	gra_defaultFont.font = XLoadQueryFont(gra_maindpy, string1byte(gra_defaultFont.fontname));
	if (gra_defaultFont.font == NULL)
		efprintf(stderr, _("Cannot open default font \"%s\"\n"), gra_defaultFont.fontname);
	for(i=0; gra_font[i].fontname != 0; i++)
	{
		/*
		 * Check the .Xdefaults or .Xresources file for a value for each of the
		 * requested fonts; they will have resource names .Font4, .Font6,...
		 * .FontStatus. Look for a matching font, if found, use it; otherwise look
		 * for the font that is hard-coded.
		 */
		ptr = string2byte(XGetDefault(gra_maindpy, b_("Electric"), string1byte(gra_resource_fontname[i])));
		if (ptr != NULL)
		{
			fontlist = XListFonts(gra_maindpy, string1byte(ptr), 10, &namecount);
			if (namecount == 0)
			{
				/* server could not find a match */
				efprintf(stderr, _("Cannot find font '%s', using '%s'\n"),
					ptr, gra_font[i].fontname);
			} else
			{
				/* replace the default (this is for debugging only) */
				(void)allocstring(&gra_font[i].fontname, string2byte(fontlist[0]), db_cluster);
			}
		} else
		{
			fontlist = XListFonts(gra_maindpy, string1byte(gra_font[i].fontname), 1, &namecount);
			if (namecount == 0)
			{
				fontlist = XListFonts(gra_maindpy, string1byte(gra_font[i].altfontname), 1, &namecount);
			}
		}

		if (namecount != 0)
		{
			/* at least one version should have worked */
			gra_font[i].font = XLoadQueryFont(gra_maindpy, fontlist[0]);
			XFreeFontNames(fontlist);
		}

		if (gra_font[i].font == NULL)
		{
			efprintf(stderr, _("Cannot find font '%s', using default\n"),
				gra_font[i].fontname);
			efprintf(stderr, _("  To avoid this message, add the following line to your .Xdefaults file:\n"));
			efprintf(stderr, M_("    Electric.%s: CORRECT-FONT-NAME\n"), gra_resource_fontname[i]);
			gra_font[i].font = gra_defaultFont.font;
		}
	}
	if (gra_font[11].font != NULL)
	{
		gra_messages_font.font = gra_font[11].font;
		gra_messages_font.fontname = gra_font[11].fontname;
	}

	/* make the messages window */
	gra_makemessageswindow();

	/* create the cursors */
	gra_xfc.flags = gra_xbc.flags = DoRed | DoGreen | DoBlue;
	XQueryColor(gra_topmsgdpy, DefaultColormap(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy)),
		&gra_xfc);
	XQueryColor(gra_topmsgdpy, DefaultColormap(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy)),
		&gra_xbc);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_realcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_realcursormask, 16, 16);
	gra_realcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 8);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_nomousecursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_nomousecursormask, 16, 16);
	gra_nomousecursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 8);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_drawcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_drawcursormask, 16, 16);
	gra_drawcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc,  &gra_xbc, 0, 16);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_nullcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_nullcursormask, 16, 16);
	gra_nullcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 8);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_menucursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_menucursormask, 16, 16);
	gra_menucursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 16, 8);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_handcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_handcursormask, 16, 16);
	gra_handcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 0, 10);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_techcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_techcursormask, 16, 16);
	gra_techcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 0);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_ibeamcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_ibeamcursormask, 16, 16);
	gra_ibeamcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 8);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_lrcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_lrcursormask, 16, 16);
	gra_lrcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 8);
	pixm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_udcursordata, 16, 16);
	pixmm = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin, (const CHAR1 *)gra_udcursormask, 16, 16);
	gra_udcursor = XCreatePixmapCursor(gra_topmsgdpy, pixm, pixmm, &gra_xfc, &gra_xbc, 8, 8);

#ifdef ANYDEPTH
	/* precache font bits */
	depth = DefaultDepth(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy));
	textmap = XCreatePixmap(gra_topmsgdpy, gra_topmsgwin, 100, 100, depth);
	if (textmap == 0)
	{
		efprintf(stderr, _("Error allocating initial text pixmap\n"));
		exitprogram();
	}
	gcv.foreground = BlackPixel(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy));
	gc = XtGetGC(gra_messageswidget, GCForeground, &gcv);
	for(i=0; gra_font[i].fontname != 0; i++)
	{
		thefont = gra_font[i].font;
		XSetFont(gra_topmsgdpy, gc, thefont->fid);
		for(j=0; j<128; j++)
		{
			gra_localstring[0] = (j < ' ' ? ' ' : (CHAR)j);
			gra_localstring[1] = 0;
			XTextExtents(thefont, string1byte(gra_localstring), 1, &direction, &asc, &des, &xcs);
			width = xcs.width;
			height = thefont->ascent + thefont->descent;
			gra_font[i].width[j] = width;
			gra_font[i].height[j] = height;
			amt = width * height;
			if (amt == 0) { gra_font[i].data[j] = 0;   continue; }
			gra_font[i].data[j] = (UCHAR1 *)emalloc(amt, us_tool->cluster);
			if (gra_font[i].data[j] == 0)
			{
				efprintf(stderr, _("Error allocating %ld bytes for font data\n"), amt);
				exitprogram();
			}
			for(k=0; k<amt; k++) gra_font[i].data[j][k] = 0;
			XSetForeground(gra_topmsgdpy, gc, 0);
			XFillRectangle(gra_topmsgdpy, textmap, gc, 0, 0, 100, 100);
			XSetForeground(gra_topmsgdpy, gc, 0xFFFFFF);
			XDrawString(gra_topmsgdpy, textmap, gc, 0,
				height-thefont->descent, string1byte(gra_localstring), 1);
			image = XGetImage(gra_topmsgdpy, textmap, 0, 0, width, height,
				AllPlanes, ZPixmap);
			if (image == 0)
			{
				efprintf(stderr, _("Error allocating %ldx%ld image for fonts\n"),
					width, height);
				exitprogram();
			}
			dptr = gra_font[i].data[j];
			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
					*dptr++ = (XGetPixel(image, x, y) == 0) ? 0 : 1;
			}
			XDestroyImage(image);
		}
	}
	XtReleaseGC(gra_messageswidget, gc);
#endif

	/* allocate space for the full-depth buffer on the main display */
	if (gra_makewindowdisplaybuffer(&gra_mainwd, gra_maindpy)) exitprogram();

	/* allocate space for the full-depth buffer on the alternate display */
	if (gra_maindpy != gra_altdpy)
	{
		if (gra_makewindowdisplaybuffer(&gra_altwd, gra_altdpy)) exitprogram();
	}

#ifndef ANYDEPTH
	/*
	 * See if there are enought X-Server resources for backing store.
	 * To request this, declare the X resource "Electric.retained:True"
	 * in the .Xdefaults or equivalent file before starting.
	 */
	gra_use_backing = FALSE;
	ptr = XGetDefault(gra_maindpy, x_("Electric"), x_("retained"));
	if (ptr != NULL)
	{
		if (namesamen(ptr, x_("True"), 4) == 0)
		{
			if (DoesBackingStore(ScreenOfDisplay(gra_maindpy, gra_mainscreen)))
				gra_use_backing = TRUE; else
			{
				efprintf(stderr, _("Backing Store is not available.\n"));
				efprintf(stderr, _("  To avoid this message, remove this line from your .Xdefaults file:\n"));
				efprintf(stderr, M_("    Electric.retained:True\n"));
			}
		}
	}
#endif

	/* initialize the window frames */
	el_firstwindowframe = el_curwindowframe = NOWINDOWFRAME;

	/* initialize the mouse */
	us_cursorstate = -1;
	us_normalcursor = NORMALCURSOR;
	setdefaultcursortype(us_normalcursor);
	gra_inputstate = NOEVENT;
	gra_eventqueuehead = gra_eventqueuetail = gra_eventqueue;

	return(FALSE);
}

/*
 * Routine to establish the displays for edit and messages and fill the globals:
 *   gra_maindpy
 *   gra_altdpy
 */
void gra_finddisplays(BOOLEAN wantmultiscreen, BOOLEAN multiscreendebug)
{
	CHAR displayname[200], maindisplayname[200], altdisplayname[200];
	INTBIG numfiles, maxdispnum, i, j;
	REGISTER INTBIG pixels, bestpixels, secondbestpixels, scr, graphicsheight, graphicswidth, dep;
	CHAR **filelist, *eprogramname;
	int argc;
	Display *dpy;
	CHAR *scrnumpos;

	/* set default displays for edit and messages windows */
	estrcpy(displayname, string2byte(XDisplayName(NULL)));
	scrnumpos = displayname + estrlen(displayname) - 1;
	estrcpy(maindisplayname, displayname);
	estrcpy(altdisplayname, displayname);
	if (multiscreendebug) printf(b_("Main display is %s\n"), displayname);

	/* determine the height of the main display */
	dpy = XOpenDisplay(string1byte(displayname));
	if (dpy == NULL) gra_shortestscreen = 600; else
	{
		scr = DefaultScreen(dpy);
		gra_shortestscreen = DisplayHeight(dpy, scr);
	}

	/* if multiple screens requested, search for them */
	maxdispnum = 0;
	if (wantmultiscreen)
	{
		/* count the number of displays (look for files named "/dev/fb*") */
		numfiles = filesindirectory(x_("/dev"), &filelist);
		for(i=0; i<numfiles; i++)
		{
			if (filelist[i][0] == 'f' && filelist[i][1] == 'b')
			{
				j = eatoi(&filelist[i][2]);
				if (j > maxdispnum) maxdispnum = j;
			}
		}

		/* if there are multiple displays, select the best for edit and messages */
		if (maxdispnum > 0)
		{
			bestpixels = 0;
			secondbestpixels = 0;
			for(i=0; i<=maxdispnum; i++)
			{
				esnprintf(scrnumpos, 100, x_("%ld"), i);
				dpy = XOpenDisplay(string1byte(displayname));
				if (dpy == NULL) continue;
				scr = DefaultScreen(dpy);
				dep = DefaultDepth(dpy, scr);
				graphicsheight = DisplayHeight(dpy, scr);
				graphicswidth = DisplayWidth(dpy, scr);
				if (graphicsheight < gra_shortestscreen) gra_shortestscreen = graphicsheight;
				if (multiscreendebug)
				{
					Visual *visual;
					CHAR *vistype;

					visual = DefaultVisual(dpy, scr);
					if (VisualClass(visual) == PseudoColor) vistype = x_("pseudo"); else
						if (VisualClass(visual) == StaticColor) vistype = x_("static"); else
							vistype = x_("direct");
					printf(b_("Display on screen %s(%ld) is %ldx%ld, %ld-bit %s color\n"),
						displayname, scr, graphicswidth, graphicsheight, dep, vistype);
				}
				pixels = graphicsheight * graphicswidth * dep;
				if (pixels > bestpixels)
				{
					if (bestpixels > secondbestpixels)
					{
						secondbestpixels = bestpixels;
						estrcpy(altdisplayname, maindisplayname);
					}
					bestpixels = pixels;
					estrcpy(maindisplayname, displayname);
				} else if (pixels > secondbestpixels)
				{
					secondbestpixels = pixels;
					estrcpy(altdisplayname, displayname);
				}
				XCloseDisplay(dpy);
			}
		}
	}

	/* open the display for the edit window */
	argc = 0;
#ifdef EPROGRAMNAME
	eprogramname = string2byte(EPROGRAMNAME);
#else
	eprogramname = x_("Electric");
#endif
	gra_maindpy = XtOpenDisplay(gra_xtapp, string1byte(maindisplayname), NULL,
		string1byte(eprogramname), NULL, 0, &argc, NULL);
	if (gra_maindpy == NULL)
	{
		efprintf(stderr, _("Cannot open main display %s\n"), maindisplayname);
		exitprogram();
	}

	/* open the display for the messages window */
	if (estrcmp(maindisplayname, altdisplayname) == 0) gra_altdpy = gra_maindpy; else
	{
		gra_altdpy = XtOpenDisplay(gra_xtapp, string1byte(altdisplayname), NULL,
			string1byte(eprogramname), NULL, 0, &argc, NULL);
		if (gra_altdpy == NULL)
		{
			efprintf(stderr, _("Cannot open alternate display %s\n"), altdisplayname);
			exitprogram();
		}
	}
	gra_mainscreen = DefaultScreen(gra_maindpy);
	gra_altscreen = DefaultScreen(gra_altdpy);
	if (multiscreendebug)
		printf(b_("So main screen is %s, alternate is %s\n"), maindisplayname, altdisplayname);
}

/*
 * Routine to create the messages window.
 */
void gra_makemessageswindow(void)
{
	XTextProperty swintitle, sicontitle;
	XWMHints *xwmh;
	XSizeHints *xsh;
	Arg arg[10];
	REGISTER INTBIG ac, screenmw, screenmh, j, msgwidth, msgheight;
	CHAR *title;
	CHAR1 *title1, title1byte[200];
	Cursor ibeam;

	/* Determine initial position and size of messages window */
	screenmw = DisplayWidth(gra_altdpy, gra_altscreen);
	screenmh = DisplayHeight(gra_altdpy, gra_altscreen);
	xsh = XAllocSizeHints();
	j = gra_messages_font.font->descent + gra_messages_font.font->ascent + MESLEADING;
	gra_status_height = MAXSTATUSLINES * j + 4;
	xsh->width = screenmh / 3 * 2;
	xsh->height = screenmh / 4 - 80;
	xsh->x = (screenmw - xsh->width) / 2;
	xsh->y = screenmh - xsh->height - 52;
	xsh->min_width = 0;
	xsh->min_height = 0;
	xsh->flags = (PPosition | PSize | PMinSize);
	msgwidth = xsh->width;
	msgheight = xsh->height;

	/* make second top-level shell for messages */
	gra_msgtoplevelwidget = XtAppCreateShell(b_("Electric"), b_("electric"),
		applicationShellWidgetClass, gra_altdpy, NULL, 0);
	if (gra_msgtoplevelwidget == 0)
	{
		efprintf(stderr, _("Cannot make messages window\n"));
		exitprogram();
	}
	ac = 0;
	XtSetArg(arg[ac], XtNx, xsh->x);   ac++;
	XtSetArg(arg[ac], XtNy, xsh->y);   ac++;
	XtSetArg(arg[ac], XtNwidth, msgwidth);   ac++;
	XtSetArg(arg[ac], XtNheight, msgheight);   ac++;
	XtSetArg(arg[ac], XmNdeleteResponse,  XmDO_NOTHING);   ac++;
	XtSetArg(arg[ac], XmNmwmFunctions,    MWM_FUNC_RESIZE|MWM_FUNC_MOVE|MWM_FUNC_MINIMIZE|MWM_FUNC_MAXIMIZE);   ac++;
#ifdef ANYDEPTH
	XtSetArg(arg[ac], XtNbackground, WHITE);   ac++;
	XtSetArg(arg[ac], XtNforeground, BLACK);   ac++;
#endif
	XtSetValues(gra_msgtoplevelwidget, arg, ac);

	/* make text widget for messages */
	ac = 0;
	XtSetArg(arg[ac], XmNeditable,   True);   ac++;
	XtSetArg(arg[ac], XmNeditMode,   XmMULTI_LINE_EDIT);   ac++;
	XtSetArg(arg[ac], XmNcolumns,    80);   ac++;
	XtSetArg(arg[ac], XmNrows,       10);   ac++;
	gra_messageswidget = (Widget)XmCreateScrolledText(gra_msgtoplevelwidget,
		b_("messages"), arg, ac);
	if (gra_messageswidget == 0)
	{
		efprintf(stderr, _("Cannot make messages widget\n"));
		exitprogram();
	}
	XtManageChild(gra_messageswidget);
	XtAddEventHandler(gra_messageswidget, KeyPressMask | VisibilityChangeMask |
		ExposureMask | FocusChangeMask, FALSE, gra_messages_event_handler, NULL);
	XtRealizeWidget(gra_msgtoplevelwidget);

	gra_topmsgwin = XtWindow(gra_msgtoplevelwidget);
	gra_topmsgdpy = XtDisplay(gra_msgtoplevelwidget);
	gra_messages_obscured = FALSE;
	gra_messages_typingin = FALSE;
#if 0
	{
	XSetWindowAttributes xswa;
	Pixel fg, bg;
	XColor xc;

	gra_messagescolormap = XCreateColormap(gra_topmsgdpy, gra_topmsgwin,
		DefaultVisual(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy)), AllocNone);
	xswa.colormap = gra_messagescolormap;
	XChangeWindowAttributes(gra_topmsgdpy, gra_topmsgwin, CWColormap, &xswa);
	XtVaGetValues(gra_messageswidget, XtNbackground, &bg, XtNforeground, &fg, NULL);
	printf(b_("initial fg=%d bg=%d\n"), fg, bg);
	xc.flags = DoRed | DoGreen | DoBlue;
	xc.pixel = bg;
	XAllocColor(gra_topmsgdpy, gra_messagescolormap, &xc);
	xc.flags = DoRed | DoGreen | DoBlue;
	xc.pixel = fg;
	XAllocColor(gra_topmsgdpy, gra_messagescolormap, &xc);
	}
#endif
	/* set I-beam cursor */
	ibeam = XCreateFontCursor(gra_topmsgdpy, XC_xterm);
	XDefineCursor(gra_topmsgdpy, gra_topmsgwin, ibeam);

	/* make the program icon */
	gra_programicon = XCreateBitmapFromData(gra_topmsgdpy, gra_topmsgwin,
		(const CHAR1 *)gra_icon, PROGICONSIZE, PROGICONSIZE);
	if (gra_programicon == 0)
	{
		efprintf(stderr, _("Cannot make program icon\n"));
		exitprogram();
	}

	/* load window manager information */
#ifdef EPROGRAMNAME
	title = string2byte(EPROGRAMNAME);
#else
	title = x_("Electric");
#endif
	(void)esnprintf(gra_localstring, MAXLOCALSTRING, _("%s Messages"), title);
	strcpy(title1byte, string1byte(gra_localstring));
	title1 = title1byte;
	XStringListToTextProperty(&title1, 1, &swintitle);

	(void)estrcpy(gra_localstring, _("Messages"));
	strcpy(title1byte, string1byte(gra_localstring));
	title1 = title1byte;
	XStringListToTextProperty(&title1, 1, &sicontitle);

	xwmh = XAllocWMHints();
	xwmh->input = True;
	xwmh->initial_state = NormalState;
	xwmh->icon_pixmap = xwmh->icon_mask = gra_programicon;
	xwmh->flags = InputHint | StateHint | IconPixmapHint;
	XSetWMProperties(gra_topmsgdpy, gra_topmsgwin, &swintitle, &sicontitle,
		gra_argv, gra_argc, xsh, xwmh, NULL);
	XFree(xsh);
	XFree(xwmh);

	/* set up to trap window kills */
	XmAddWMProtocolCallback(gra_msgtoplevelwidget, gra_wm_delete_window,
		(XtCallbackProc)gra_windowdelete, (XtPointer)gra_messageswidget);

#if 0	/* This code used to be part of the !(ANYDEPTH) case (Dmitry Nadezchin) */
	XSetWindowAttributes xswa;

	/* setup properties of the messages window */
	xswa.background_pixel = WhitePixel(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy));
	xswa.border_pixel = BlackPixel(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy));
	xswa.bit_gravity = NorthWestGravity;
	if (gra_use_backing)
	{
		xswa.backing_store = Always;
		xswa.backing_pixel = xswa.background_pixel;
		xswa.backing_planes = AllPlanes;
	}
	if (DoesSaveUnders(ScreenOfDisplay(gra_altdpy, gra_altscreen)))
		xswa.save_under = True;
	XChangeWindowAttributes(gra_topmsgdpy, gra_topmsgwin, CWColormap, &xswa);
#endif
}

#if LANGTCL
INTBIG gra_initializetcl(void)
{
	INTBIG err;
	CHAR *newArgv[2];

	/* set the program name/path */
	newArgv[0] = x_("Electric");
	newArgv[1] = NULL;
	(void)Tcl_FindExecutable(newArgv[0]);

	gra_tclinterp = Tcl_CreateInterp();
	if (gra_tclinterp == 0) error(_("from Tcl_CreateInterp"));

	/* tell Electric the TCL interpreter handle */
	el_tclinterpreter(gra_tclinterp);

	/* Make command-line arguments available in the Tcl variables "argc" and "argv" */
	Tcl_SetVar(gra_tclinterp, x_("argv"), x_(""), TCL_GLOBAL_ONLY);
	Tcl_SetVar(gra_tclinterp, x_("argc"), x_("0"), TCL_GLOBAL_ONLY);
	Tcl_SetVar(gra_tclinterp, x_("argv0"), x_("electric"), TCL_GLOBAL_ONLY);

	/* Set the "tcl_interactive" variable */
	Tcl_SetVar(gra_tclinterp, x_("tcl_interactive"), x_("1"), TCL_GLOBAL_ONLY);

	/* initialize the interpreter */
	err = Tcl_Init(gra_tclinterp);
	if (err != TCL_OK) error(_("(from Tcl_Init) %s"), gra_tclinterp->result);
	return(err);
}
#endif

/*
 * Routine to establish the library directories from the environment.
 */
void setupenvironment(void)
{
	REGISTER CHAR *pt, *buf;
	REGISTER INTBIG len, filestatus;
	REGISTER void *infstr;

	/* set machine name */
	nextchangequiet();
	(void)setval((INTBIG)us_tool, VTOOL, x_("USER_machine"),
		(INTBIG)x_("UNIX"), VSTRING|VDONTSAVE);

	pt = egetenv(x_("ELECTRIC_LIBDIR"));
	if (pt != NULL)
	{
		/* environment variable ELECTRIC_LIBDIR is set: use it */
		buf = (CHAR *)emalloc((estrlen(pt)+5) * SIZEOFCHAR, el_tempcluster);
		if (buf == 0)
		{
			efprintf(stderr, _("Cannot make environment buffer\n"));
			exitprogram();
		}
		estrcpy(buf, pt);
		if (buf[estrlen(buf)-1] != '/') estrcat(buf, x_("/"));
		(void)allocstring(&el_libdir, buf, db_cluster);
		efree(buf);
		return;
	}

	/* try the #defined library directory */
	infstr = initinfstr();
	if (LIBDIR[0] != '/') addstringtoinfstr(infstr, currentdirectory());
	addstringtoinfstr(infstr, LIBDIR);
	addstringtoinfstr(infstr, x_(".cadrc"));
	pt = returninfstr(infstr);
	filestatus = fileexistence(pt);
	if (filestatus == 1 || filestatus == 3)
	{
		len = estrlen(pt);
		pt[len-6] = 0;
		(void)allocstring(&el_libdir, pt, db_cluster);
		return;
	}

	/* try the current location (look for "lib") */
	infstr = initinfstr();
	addstringtoinfstr(infstr, currentdirectory());
	addstringtoinfstr(infstr, x_("lib/.cadrc"));
	pt = returninfstr(infstr);
	filestatus = fileexistence(pt);
	if (filestatus == 1 || filestatus == 3)
	{
		len = estrlen(pt);
		pt[len-6] = 0;
		(void)allocstring(&el_libdir, pt, db_cluster);
		return;
	}

	/* no environment, #define, or current directory: look for ".cadrc" */
	filestatus = fileexistence(x_(".cadrc"));
	if (filestatus == 1 || filestatus == 3)
	{
		/* found ".cadrc" in current directory: use that */
		(void)allocstring(&el_libdir, currentdirectory(), db_cluster);
		return;
	}

	/* look for "~/.cadrc" */
	infstr = initinfstr();
	addstringtoinfstr(infstr, truepath(x_("~/.cadrc")));
	pt = returninfstr(infstr);
	filestatus = fileexistence(pt);
	if (filestatus == 1 || filestatus == 3)
	{
		(void)allocstring(&el_libdir, pt, db_cluster);
		return;
	}

	ttyputerr(_("Warning: cannot find '.cadrc' startup file"));
	(void)allocstring(&el_libdir, x_("."), db_cluster);
}

void setlibdir(CHAR *libdir)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	if (libdir[0] != '/') addstringtoinfstr(infstr, currentdirectory());
	addstringtoinfstr(infstr, libdir);
	if (libdir[estrlen(libdir)-1] != DIRSEP) addtoinfstr(infstr, DIRSEP);
	(void)reallocstring(&el_libdir, returninfstr(infstr), db_cluster);
}

/******************** TERMINATION ********************/

void termgraphics(void)
{
	REGISTER INTBIG i;
#ifdef ANYDEPTH
	REGISTER INTBIG j;
#endif

	gra_termgraph();

	/* deallocate offscreen buffer for displays */
	efree((CHAR *)gra_mainwd.addr);
	efree((CHAR *)gra_mainwd.rowstart);
	if (gra_maindpy != gra_altdpy)
	{
		efree((CHAR *)gra_altwd.addr);
		efree((CHAR *)gra_altwd.rowstart);
	}

# ifdef TRUETYPE
	if (gra_descentcachesize > 0)
	{
		efree((CHAR *)gra_descentcache);
		efree((CHAR *)gra_ascentcache);
		efree((CHAR *)gra_swidthcache);
	}
	for(i=0; i<gra_numfaces; i++) efree((CHAR *)gra_facelist[i]);
	if (gra_numfaces > 0) efree((CHAR *)gra_facelist);
#endif

#ifdef ANYDEPTH
	for(i=0; gra_font[i].fontname != 0; i++)
	{
		for(j=0; j<128; j++)
		{
			if (gra_font[i].data[j] != 0)
				efree((CHAR *)gra_font[i].data[j]); 
		}
	}

	if (gra_textbitsdatasize > 0) efree((CHAR *)gra_textbitsdata);
	if (gra_textbitsrowstartsize > 0) efree((CHAR *)gra_textbitsrowstart);
#endif

	/* free printer list */
	for(i=0; i<gra_printerlistcount; i++)
		efree((CHAR *)gra_printerlist[i]);
	if (gra_printerlisttotal > 0) efree((CHAR *)gra_printerlist);

	XCloseDisplay(gra_maindpy);
	gra_messageswidget = 0; /* to print messages to stderr */
	if (gra_icontruedata != 0)
	{
		efree((CHAR *)gra_icontruedata);
		efree((CHAR *)gra_iconrowdata);
	}
/*	XtDestroyApplicationContext(gra_xtapp); ...causes a segmentation fault for some reason */
	efree((CHAR *)gra_initialdirectory);
	efree((CHAR *)gra_logfile);
	efree((CHAR *)gra_logfilesave);

	/* free filelist, brokenbuf */
	if (gra_fileliststringarray != 0) killstringarray(gra_fileliststringarray);
	if (gra_brokenbufsize > 0) efree(gra_brokenbuf);
}

void exitprogram(void)
{
	exit(0);
}

/******************** WINDOW CONTROL ********************/

/*
 * These routines implement multiple windows on the display (each display window
 * is called a "windowframe".
 */

WINDOWFRAME *newwindowframe(BOOLEAN floating, RECTAREA *r)
{
	WINDOWFRAME *wf, *oldlisthead;

	/* allocate one */
	wf = (WINDOWFRAME *)emalloc((sizeof (WINDOWFRAME)), us_tool->cluster);
	if (wf == 0) return(NOWINDOWFRAME);
	wf->numvar = 0;
	wf->firstmenu = 0;
	wf->windindex = gra_windownumberindex++;

	/* insert window-frame in linked list */
	oldlisthead = el_firstwindowframe;
	wf->nextwindowframe = el_firstwindowframe;
	el_firstwindowframe = wf;

	/* load an editor window into this frame */
	if (gra_buildwindow(gra_maindpy, &gra_mainwd, gra_maincolmap, wf, floating, FALSE, r))
	{
		efree((CHAR *)wf);
		el_firstwindowframe = oldlisthead;
		return(NOWINDOWFRAME);
	}

	/* remember that this is the current window frame */
	if (!floating) el_curwindowframe = wf;

	return(wf);
}

void killwindowframe(WINDOWFRAME *owf)
{
	WINDOWFRAME *wf, *lastwf;
	REGISTER INTBIG i;

	/* don't do this if window is being deleted from another place */
	if (gra_windowbeingdeleted != 0) return;

	/* kill the actual window, that will remove the frame, too */
	XtDestroyWidget(owf->toplevelwidget);

	/* ind this frame in the list */
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

#ifdef ANYDEPTH
	efree((CHAR *)wf->dataaddr8);
	efree((CHAR *)wf->rowstart);
#endif
	for(i=0; i<wf->pulldownmenucount; i++) efree((CHAR *)wf->pulldownmenulist[i]);
	if (wf->pulldownmenucount != 0)
	{
		for(i=0; i<wf->pulldownmenucount; i++) efree(wf->pulldowns[i]);
		efree((CHAR *)wf->pulldownmenus);
		efree((CHAR *)wf->pulldowns);
		efree((CHAR *)wf->pulldownmenusize);
		efree((CHAR *)wf->pulldownmenulist);
	}
	efree((CHAR *)owf);
}

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
	Arg arg[4];
	short x, y, wid, hei;

	XtSetArg(arg[0], XtNx, &x);
	XtSetArg(arg[1], XtNy, &y);
	XtSetArg(arg[2], XtNwidth, &wid);
	XtSetArg(arg[3], XtNheight, &hei);
	XtGetValues(gra_msgtoplevelwidget, arg, 4);
	*top = y;
	*left = x;
	*bottom = y+hei;
	*right = x+wid;
}

/*
 * Routine to set the size and position of the messages window.
 */
void setmessagesframeinfo(INTBIG top, INTBIG left, INTBIG bottom, INTBIG right)
{
	XtResizeWidget(gra_msgtoplevelwidget, right-left, bottom-top, 0);
	XtMoveWidget(gra_msgtoplevelwidget, left-WMLEFTBORDER, top-WMTOPBORDER);
}

void sizewindowframe(WINDOWFRAME *wf, INTBIG wid, INTBIG hei)
{
	XWindowAttributes xwa;
	INTBIG intwid, inthei;
	static WINDOWFRAME *lastwf = 0;
	static INTBIG lastwid, lasthei, repeat;

	if (wid < 0 || hei < 0) return;
	gra_getwindowattributes(wf, &xwa);
	if (wid == xwa.width && hei == xwa.height) return;
	if (wf == lastwf && wid == lastwid && hei == lasthei)
	{
		repeat++;
		if (repeat > 2) return;
	} else
	{
		lastwf = wf;   lastwid = wid;   lasthei = hei;   repeat = 1;
	}
	gra_internalchange = ticktime();
	XtResizeWidget(wf->toplevelwidget, wid, hei, 0);
	if (wf->floating)
	{
		gra_getwindowinteriorsize(wf, &intwid, &inthei);
		gra_recalcsize(wf, intwid, inthei);
	}
}

void movewindowframe(WINDOWFRAME *wf, INTBIG left, INTBIG top)
{
	XWindowAttributes xwa;

	gra_getwindowattributes(wf, &xwa);
	if (wf->floating) top += gra_palettetop;
	if (left == xwa.x && top == xwa.y) return;
	gra_internalchange = ticktime();
	XtMoveWidget(wf->toplevelwidget, left, top);
}

void gra_getwindowattributes(WINDOWFRAME *wf, XWindowAttributes *xwa)
{
	Arg arg[4];
	short x, y, wid, hei;

	XtSetArg(arg[0], XtNx, &x);
	XtSetArg(arg[1], XtNy, &y);
	XtSetArg(arg[2], XtNwidth, &wid);
	XtSetArg(arg[3], XtNheight, &hei);
	XtGetValues(wf->toplevelwidget, arg, 4);
	xwa->x = x;         xwa->y = y;
	xwa->width = wid;   xwa->height = hei;
}

void gra_getwindowinteriorsize(WINDOWFRAME *wf, INTBIG *wid, INTBIG *hei)
{
	Arg arg[2];
	short width, height;

	XtSetArg(arg[0], XtNwidth, &width);
	XtSetArg(arg[1], XtNheight, &height);
	XtGetValues(wf->graphicswidget, arg, 2);
	*wid = width;   *hei = height;
}

/*
 * Routine to close the messages window if it is in front.  Returns true if the
 * window was closed.
 */
BOOLEAN closefrontmostmessages(void)
{
	return(FALSE);
}

/*
 * Routine to bring window "wf" to the front.
 */
void bringwindowtofront(WINDOWFRAME *wf)
{
	XRaiseWindow(wf->topdpy, wf->topwin);
	XSetInputFocus(wf->topdpy, wf->topwin, RevertToNone, CurrentTime);
}

/*
 * Routine to organize the windows according to "how" (0: tile horizontally,
 * 1: tile vertically, 2: cascade).
 */
void adjustwindowframe(INTBIG how)
{
	gra_adjustwindowframeon(gra_maindpy, gra_mainscreen, how);
	if (gra_maindpy != gra_altdpy)
		gra_adjustwindowframeon(gra_altdpy, gra_altscreen, how);
}

void gra_adjustwindowframeon(Display *dpy, INTBIG screen, INTBIG how)
{
	RECTAREA r, wr;
	REGISTER INTBIG children, child, sizex, sizey;
	REGISTER WINDOWFRAME *wf, *compmenu;

	/* figure out how many windows need to be rearranged */
	children = 0;
	compmenu = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->topdpy != dpy) continue;
		if (!wf->floating) children++; else
			compmenu = wf;
	}
	if (children <= 0) return;

	/* determine area for windows */
	r.left = 0;   r.right = DisplayWidth(dpy, screen) - WMLEFTBORDER*2;
	r.top = 0;   r.bottom = DisplayHeight(dpy, screen) - WMLEFTBORDER*2;

	if (compmenu != NULL)
	{
		/* remove component menu from area of tiling */
		gra_removewindowextent(&r, compmenu->toplevelwidget);
	}
	if (dpy == gra_topmsgdpy && gra_msgtoplevelwidget != 0)
	{
		/* remove messages menu from area of tiling */
		gra_removewindowextent(&r, gra_msgtoplevelwidget);
	}

	/* rearrange the windows */
	child = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->topdpy != dpy) continue;
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
		wr.right -= WMLEFTBORDER + WMLEFTBORDER;
		wr.bottom -= WMTOPBORDER + WMLEFTBORDER;
		sizewindowframe(wf, wr.right-wr.left, wr.bottom-wr.top);
		gra_repaint(wf, FALSE);
		movewindowframe(wf, wr.left, wr.top);
		child++;
	}
}

/*
 * Helper routine to remove the location of window "wnd" from the rectangle "r".
 */
void gra_removewindowextent(RECTAREA *r, Widget wnd)
{
	RECTAREA er;
	Arg arg[4];
	short x, y, wid, hei;

	XtSetArg(arg[0], XtNx, &x);
	XtSetArg(arg[1], XtNy, &y);
	XtSetArg(arg[2], XtNwidth, &wid);
	XtSetArg(arg[3], XtNheight, &hei);
	XtGetValues(wnd, arg, 4);
	er.left = x;   er.right = er.left + wid;
	er.top = y;    er.bottom = er.top + hei;

	if (er.right-er.left > er.bottom-er.top)
	{
		/* horizontal occluding window */
		if (er.bottom - r->top < r->bottom - er.top)
		{
			/* occluding window on top */
			r->top = er.bottom + WMLEFTBORDER;
		} else
		{
			/* occluding window on bottom */
			r->bottom = er.top - WMTOPBORDER;
		}
	} else
	{
		/* vertical occluding window */
		if (er.right - r->left < r->right - er.left)
		{
			/* occluding window on left */
			r->left = er.right + WMLEFTBORDER;
		} else
		{
			/* occluding window on right */
			r->right = er.left - WMLEFTBORDER;
		}
	}
}

void getpaletteparameters(INTBIG *wid, INTBIG *hei, INTBIG *palettewidth)
{
	*wid = gra_palettewidth;
	*hei = gra_paletteheight;
	*palettewidth = PALETTEWIDTH;
}

/*
 * Routine called when the component menu has moved to a different location
 * on the screen (left/right/top/bottom).  Resets local state of the position.
 */
void resetpaletteparameters(void)
{
	gra_palettewidth = DisplayWidth(gra_maindpy, gra_mainscreen) - WMLEFTBORDER;
	gra_paletteheight = DisplayHeight(gra_maindpy, gra_mainscreen) - WMTOPBORDER;
	gra_palettetop = 0;
}

BOOLEAN gra_buildwindow(Display *dpy, WINDOWDISPLAY *wd, Colormap colmap, WINDOWFRAME *wf,
	BOOLEAN floating, BOOLEAN samesize, RECTAREA *r)
{
	XTextProperty icontitle, wintitle;
	XWindowAttributes xwa;
	XWMHints *xwmh;
	Arg arg[10];
	int ac;
	CHAR *ptr;
	CHAR1 *title1, title1byte[200];
#ifdef ANYDEPTH
	REGISTER INTBIG amt, i;
#else
	XGCValues gcv;
	Visual *visual;
	XSetWindowAttributes xswa;
	INTBIG planes[8], pixelarray[256];
#endif

	wf->floating = floating;
	wf->wd = wd;
	wf->colormap = colmap;

	/* determine window position */
	if (r != 0)
	{
		gra_editxsh->x = r->left;
		gra_editxsh->y = r->top;
		gra_editxsh->width = r->right - r->left;
		gra_editxsh->height = r->bottom - r->top;
	} else
	{
		if (samesize)
		{
			gra_getwindowattributes(wf, &xwa);
			gra_editxsh->x = xwa.x;
			gra_editxsh->y = xwa.y;
			gra_editxsh->width = xwa.width;
			gra_editxsh->height = xwa.height;
		} else
		{
			if (!floating)
			{
				gra_editxsh->x = gra_editposx;
				gra_editxsh->y = gra_editposy;
				gra_editxsh->width = gra_editsizex;
				gra_editxsh->height = gra_editsizey;
				gra_editxsh->x += (gra_windownumber%5) * 40;
				gra_editxsh->y += (gra_windownumber%5) * 40;
				if (gra_editxsh->x + gra_editxsh->width > wd->wid)
					gra_editxsh->width = wd->wid - gra_editxsh->x;
				if (gra_editxsh->y + gra_editxsh->height > wd->hei)
					gra_editxsh->height = wd->hei - gra_editxsh->y;
				gra_windownumber++;
			} else
			{
				gra_editxsh->x = 0;
				gra_editxsh->y = 1 + gra_palettetop;
				gra_editxsh->width = PALETTEWIDTH;
				gra_editxsh->height = wd->hei - 50;
				gra_internalchange = ticktime();
			}
		}
	}

	/*
	 * make top-level graphics widget
	 * this call may generate warnings like this:
	 *   Warning: Cannot convert string "<Key>Escape,_Key_Cancel" to type VirtualBinding
	 */
	wf->toplevelwidget = XtAppCreateShell(b_("electric"), b_("electric"), applicationShellWidgetClass,
		dpy, NULL, 0);
	if (wf->toplevelwidget == 0)
	{
		efprintf(stderr, _("Could not create top-level window\n"));
		return(TRUE);
	}
	(void)XtAppSetErrorHandler(gra_xtapp, gra_xterrors);
	ac = 0;
	XtSetArg(arg[ac], XtNx, gra_editxsh->x);   ac++;
	XtSetArg(arg[ac], XtNy, gra_editxsh->y);   ac++;
	XtSetArg(arg[ac], XtNwidth, gra_editxsh->width);   ac++;
	XtSetArg(arg[ac], XtNheight, gra_editxsh->height);   ac++;
	XtSetArg(arg[ac], XmNdeleteResponse,  XmDO_NOTHING);   ac++;
	if (floating)
	{
/*		XtSetArg(arg[ac], XmNoverrideRedirect,  True);   ac++;  ...wanted to make component menu float */
		XtSetArg(arg[ac], XmNmwmFunctions, MWM_FUNC_RESIZE|MWM_FUNC_MOVE|
			MWM_FUNC_MINIMIZE|MWM_FUNC_CLOSE);   ac++;
	}
	XtSetValues(wf->toplevelwidget, arg, ac);

	/* make graphics widget frame */
	ac = 0;
	XtSetArg(arg[ac], XmNwidth, gra_editxsh->width);   ac++;
	XtSetArg(arg[ac], XmNheight, gra_editxsh->height);   ac++;
	wf->intermediategraphics = (Widget)XmCreateMainWindow(wf->toplevelwidget,
		x_("graphics"), arg, ac);
	if (wf->intermediategraphics == 0)
	{
		efprintf(stderr, _("Could not create main window\n"));
		return(TRUE);
	}
	XtManageChild(wf->intermediategraphics);

	/* make graphics widget drawing area */
	ac = 0;
	XtSetArg(arg[ac], XmNresizePolicy, XmRESIZE_ANY);   ac++;
	wf->graphicswidget = (Widget)XmCreateDrawingArea(wf->intermediategraphics,
		x_("drawing_area"), arg, ac);
	if (wf->graphicswidget == 0)
	{
		efprintf(stderr, _("Could not create main widget\n"));
		return(TRUE);
	}
	XtManageChild(wf->graphicswidget);

	/* make menus */
	wf->pulldownmenucount = 0;
	if (floating) wf->menubar = 0; else
	{
		wf->menubar = (Widget)XmCreateMenuBar(wf->intermediategraphics, b_("MainWin"), NULL, 0);
		XtManageChild(wf->menubar);
		gra_pulldownmenuload(wf);
	}
	XmMainWindowSetAreas(wf->intermediategraphics, wf->menubar, NULL,
		NULL, NULL, wf->graphicswidget);

	/* add a window-manager protocol to move displays */
	if (gra_maindpy != gra_altdpy)
	{
		XmAddProtocols(wf->toplevelwidget, gra_movedisplaymsg, &gra_movedisplayprotocol, 1);
		XmAddProtocolCallback(wf->toplevelwidget, gra_movedisplaymsg, gra_movedisplayprotocol,
			(XtCallbackProc)gra_movedisplay, NULL);
		esnprintf(gra_localstring, MAXLOCALSTRING, x_("Move\\ to\\ Other\\ Display _D f.send_msg %d"),
			gra_movedisplayprotocol);
		XtVaSetValues(wf->toplevelwidget, XmNmwmMenu, gra_localstring, NULL);
	}
	XtAddEventHandler(wf->graphicswidget, ButtonPressMask | ButtonReleaseMask |
		KeyPressMask | PointerMotionMask | ExposureMask | StructureNotifyMask |
		FocusChangeMask, FALSE, gra_graphics_event_handler, NULL);
	XtAddEventHandler(wf->toplevelwidget, StructureNotifyMask, FALSE,
		gra_graphics_event_handler, NULL);
	XtRealizeWidget(wf->toplevelwidget);

	/* get info about this widget */
	wf->topdpy = XtDisplay(wf->toplevelwidget);
	wf->topwin = XtWindow(wf->toplevelwidget);
	wf->win = XtWindow(wf->graphicswidget);

	/* set up to trap window kills */
	XmAddWMProtocolCallback(wf->toplevelwidget, gra_wm_delete_window,
		(XtCallbackProc)gra_windowdelete, (XtPointer)wf->intermediategraphics);

	/* build graphics contexts */
#ifndef ANYDEPTH
	gcv.foreground = BlackPixel(wf->topdpy, DefaultScreen(wf->topdpy));
	gcv.graphics_exposures = False;     /* no event after XCopyArea */
	wf->gc = XtGetGC(wf->graphicswidget, GCForeground | GCGraphicsExposures, &gcv);
	gcv.function = GXinvert;
	wf->gcinv = XtGetGC(wf->graphicswidget, GCFunction | GCGraphicsExposures, &gcv);
	gcv.fill_style = FillStippled;
	wf->gcstip = XtGetGC(wf->graphicswidget, GCFillStyle | GCGraphicsExposures, &gcv);
#endif
	/* get window size and depth after creation */
	XGetWindowAttributes(wf->topdpy, wf->win, &xwa);
	wf->swid = xwa.width;
	wf->trueheight = xwa.height;  /* we keep the true height locally */
	wf->shei = wf->trueheight;
	if (!floating) wf->shei -= gra_status_height;
	wf->revy = wf->shei - 1;

	/* load window manager information */
	if (!floating) (void)estrcpy(gra_localstring, _("Electric")); else
		(void)estrcpy(gra_localstring, _("Components"));
	strcpy(title1byte, string1byte(gra_localstring));
	title1 = title1byte;
	XStringListToTextProperty(&title1, 1, &wintitle);
	XStringListToTextProperty(&title1, 1, &icontitle);
	xwmh = XAllocWMHints();
	xwmh->input = True;
	xwmh->initial_state = NormalState;
	xwmh->icon_pixmap = xwmh->icon_mask = gra_programicon;
	xwmh->flags = InputHint | StateHint | IconPixmapHint;
	XSetWMProperties(wf->topdpy, wf->topwin, &wintitle, &icontitle,
		gra_argv, gra_argc, gra_editxsh, xwmh, NULL);
	XFree(xwmh);
#ifdef ANYDEPTH
	/* allocate space for the 8-bit deep buffer */
	amt = wf->swid * wf->trueheight;
	wf->dataaddr8 = (UCHAR1 *)emalloc(amt, us_tool->cluster);
	if (wf->dataaddr8 == 0)
	{
		efprintf(stderr, _("Could not allocate space for offscreen buffer\n"));
		return(TRUE);
	}
	for(i=0; i<amt; i++) wf->dataaddr8[i] = 0;
	wf->rowstart = (UCHAR1 **)emalloc(wf->trueheight * SIZEOFINTBIG, us_tool->cluster);
	if (wf->rowstart == 0)
	{
		efprintf(stderr, _("Could not allocate space for row pointers\n"));
		return(TRUE);
	}
	for(i=0; i<wf->trueheight; i++)
		wf->rowstart[i] = wf->dataaddr8 + i * wf->swid;

	/* fill the color map  */
	if (DefaultDepth(wf->topdpy, DefaultScreen(wf->topdpy)) != 1)
	{
		us_getcolormap(el_curtech, COLORSDEFAULT, FALSE);
		gra_reloadmap();
	}
#else
	/* setup properties of the edit window */
	xswa.background_pixel = WhitePixel(wf->topdpy, DefaultScreen(wf->topdpy));
	xswa.border_pixel = BlackPixel(wf->topdpy, DefaultScreen(wf->topdpy));
	if (gra_use_backing)
	{
		xswa.backing_store = WhenMapped;

		/* load backing store property (Dmitry Nadezchin) */
		XChangeWindowAttributes(wf->topdpy, wf->win, CWBackingStore, &xswa);

		/* default backing_pixel and backing_planes are not set */
		xswa.backing_pixel = xswa.background_pixel;
		xswa.backing_planes = AllPlanes;
	}
	visual = DefaultVisual(wf->topdpy, DefaultScreen(wf->topdpy));
	wf->colormap = XCreateColormap(wf->topdpy, RootWindow(wf->topdpy, DefaultScreen(wf->topdpy)),
		visual, AllocNone);
	xswa.colormap = wf->colormap;
	XChangeWindowAttributes(wf->topdpy, wf->topwin, CWColormap, &xswa);

	/* fill the color map  */
	if (DefaultDepth(wf->topdpy, DefaultScreen(wf->topdpy)) != 1)
	{
		if (XAllocColorCells(wf->topdpy, wf->colormap, 1, (unsigned long *)planes, 0,
			(unsigned long *)pixelarray, el_maplength) == 0)
				efprintf(stderr, x_("XAllocColorCells failed\n"));

		/* load any map the first time to establish the map segment length */
		us_getcolormap(el_curtech, COLORSDEFAULT, FALSE);
		gra_reloadmap();
		gra_xfc.pixel = el_colcursor;
		XQueryColor(wf->topdpy, wf->colormap, &gra_xfc);
		gra_xbc.pixel = 0;
		XQueryColor(wf->topdpy, wf->colormap, &gra_xbc);
		gra_recolorcursor(wf->topdpy, &gra_xfc, &gra_xbc);
	}
#endif
	return(FALSE);
}

void gra_movedisplay(Widget w, XtPointer client_data, XtPointer *call_data)
{
	Display *dpy;
	REGISTER WINDOWFRAME *wf;
	REGISTER WINDOWDISPLAY *wd;
	Colormap colmap;

	wf = gra_getcurrentwindowframe(w, FALSE);
	if (wf == NOWINDOWFRAME) return;
	XtDestroyWidget(wf->toplevelwidget);
#ifdef ANYDEPTH
	efree((CHAR *)wf->dataaddr8);
	efree((CHAR *)wf->rowstart);
#endif
	if (wf->topdpy == gra_maindpy)
	{
		dpy = gra_altdpy;
		wd = &gra_altwd;
		colmap = gra_altcolmap;
	} else
	{
		dpy = gra_maindpy;
		wd = &gra_mainwd;
		colmap = gra_maincolmap;
	}
	if (gra_buildwindow(dpy, wd, colmap, wf, wf->floating, TRUE, 0))
		ttyputerr(_("Problem moving the display"));

	/* redisplay */
	gra_reloadmap();
	us_drawmenu(1, wf);
	us_redostatus(wf);
	us_endbatch();
}

/******************** MISCELLANEOUS EXTERNAL ROUTINES ********************/

/*
 * return nonzero if the capabilities in "want" are present
 */
BOOLEAN graphicshas(INTBIG want)
{
	if ((want & CANHAVENOWINDOWS) != 0) return(FALSE);

	/* cannot modify typefaces */
	if ((want&(CANCHOOSEFACES|CANMODIFYFONTS|CANSCALEFONTS)) != 0)
	{
#ifdef TRUETYPE
		if (gra_truetypeon != 0) return(TRUE);
#endif
		return(FALSE);
	}

	return(TRUE);
}

/*
 * Routine to make sound "sound".  If sounds are turned off, no
 * sound is made (unless "force" is TRUE)
 */
void ttybeep(INTBIG sound, BOOLEAN force)
{
	if ((us_tool->toolstate & TERMBEEP) != 0 || force)
	{
		switch (sound)
		{
			case SOUNDBEEP:
				XBell(gra_maindpy, 100);
				break;
			case SOUNDCLICK:
				if ((us_useroptions&NOEXTRASOUND) != 0) break;
				/* really want to play "click.wav" in the library area */
				break;
		}
	}
}

DIALOGITEM db_severeerrordialogitems[] =
{
 /*  1 */ {0, {80,8,104,72}, BUTTON, N_("Exit")},
 /*  2 */ {0, {80,96,104,160}, BUTTON, N_("Save")},
 /*  3 */ {0, {80,184,104,256}, BUTTON, N_("Continue")},
 /*  4 */ {0, {8,8,72,256}, MESSAGE, x_("")}
};
DIALOG db_severeerrordialog = {{50,75,163,341}, N_("Fatal Error"), 0, 4, db_severeerrordialogitems, 0, 0};

/* special items for the severe error dialog: */
#define DSVE_EXIT      1		/* Exit (button) */
#define DSVE_SAVE      2		/* Save (button) */
#define DSVE_CONTINUE  3		/* Continue (button) */
#define DSVE_MESSAGE   4		/* Error Message (stat text) */

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
	if (dia == 0) return;

	/* load the message */
	DiaSetText(dia, DSVE_MESSAGE, line);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == DSVE_EXIT) exitprogram();
		if (itemHit == DSVE_CONTINUE) break;
		if (itemHit == DSVE_SAVE)
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
	}
	DiaDoneDialog(dia);
}

#define TRUEGETENV getenv

/*
 * Routine to get the environment variable "name" and return its value.
 */
CHAR *egetenv(CHAR *name)
{
#ifdef _UNICODE
	return(string2byte(TRUEGETENV(string1byte(name))));
#else
	return(TRUEGETENV(name));
#endif
}

/*
 * Routine to get the current language that Electric speaks.
 */
CHAR *elanguage(void)
{
	CHAR *lang;

	lang = egetenv(x_("LANGUAGE"));
	if (lang == 0) lang = x_("en");
	return(lang);
}

/*
 * Routine to fork a new process.  Returns the child process number if this is the
 * parent thread.  Returns 0 if this is the child thread.
 * Returns 1 if forking is not possible (process 1 is INIT on UNIX and can't possibly
 * be assigned to a normal process).
 */
INTBIG efork(void)
{
	return(fork());
}

/*
 * Routine to run the string "command" in a shell.
 * Returns nonzero if the command cannot be run.
 */
INTBIG esystem(CHAR *command)
{
	system(string1byte(command));
	return(0);
}

/*
 * Routine to execute the program "program" with the arguments "args"
 */
void eexec(CHAR *program, CHAR *args[])
{
	CHAR1 **myargs;
	INTBIG total, i;

	/* copy to a local array */
	for(total=0; args[total] != 0; total++) ;
	myargs = (CHAR1 **)emalloc((total+1) * (sizeof (CHAR1 *)), us_tool->cluster);
	if (myargs == 0) return;
	for(i=0; i<total; i++)
	{
		myargs[i] = (CHAR1 *)emalloc(estrlen(args[i])+1, us_tool->cluster);
		TRUESTRCPY(myargs[i], string1byte(args[i]));
	}
	myargs[total] = 0;
	
	/* run it */
	execvp(string1byte(program), myargs);

	/* free the memory */
	for(i=0; i<total; i++) efree((CHAR *)myargs[i]);
	efree((CHAR *)myargs);
}

/*
 * routine to kill process "process".
 */
INTBIG ekill(INTBIG process)
{
	return(kill(process, SIGKILL));
}

/*
 * routine to wait for the completion of child process "process"
 */
void ewait(INTBIG process)
{
	REGISTER INTBIG pid;

	for(;;)
	{
		pid = wait((int *)0);
		if (pid == process) return;
		if (pid == -1)
		{
			perror(b_("Waiting"));
			return;
		}
	}
}

/*
 * Routine to return the number of processors on this machine.
 */
INTBIG enumprocessors(void)
{
	INTBIG numproc;

	numproc = sysconf(_SC_NPROCESSORS_ONLN);
	return(numproc);
}

/*
 * Routine to create a new thread that calls "function" with "argument".
 */
void enewthread(void* (*function)(void*), void *argument)
{
#ifdef HAVE_PTHREAD
	pthread_t threadptr;

	pthread_create(&threadptr, NULL, function, argument);
#else
	thread_t threadid;

	if (thr_create(NULL, 0, function, argument, THR_BOUND, &threadid) != 0)
		printf(b_("Thread creation failed\n"));
#endif
}

/*
 * Routine that creates a mutual-exclusion object and returns it.
 */
void *emakemutex(void)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_t *mutex;

	mutex = (pthread_mutex_t *)calloc(1, sizeof (pthread_mutex_t));
	pthread_mutex_init(mutex, NULL);
	return((void *)mutex);
#else
	mutex_t *mutexid;

	mutexid = (mutex_t *)calloc(1, sizeof (mutex_t));
	mutex_init(mutexid, USYNC_THREAD, 0);
	return((void *)mutexid);
#endif
}

/*
 * Routine that locks mutual-exclusion object "vmutex".  If the object is already
 * locked, the routine blocks until it is unlocked.
 */
void emutexlock(void *vmutex)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_t *mutex;

	mutex = (pthread_mutex_t *)vmutex;
	pthread_mutex_lock(mutex);
#else
	mutex_t *mutexid;

	mutexid = (mutex_t *)vmutex;
	mutex_lock(mutexid);
#endif
}

/*
 * Routine that unlocks mutual-exclusion object "vmutex".
 */
void emutexunlock(void *vmutex)
{
#ifdef HAVE_PTHREAD
	pthread_mutex_t *mutex;

	mutex = (pthread_mutex_t *)vmutex;
	pthread_mutex_unlock(mutex);
#else
	mutex_t *mutexid;

	mutexid = (mutex_t *)vmutex;
	mutex_unlock(mutexid);
#endif
}

/*
 * Routine to determine the list of printers and return it.
 * The list terminates with a zero.
 */
CHAR **eprinterlist(void)
{
	static CHAR *nulllplist[1];
	int pipechans[2];
	CHAR *execargs[5];
	INTBIG process, i, save, bptr;
	CHAR val, buf[200], *pt;
	FILE *io;

	/* remove former list */
	for(i=0; i<gra_printerlistcount; i++)
		efree((CHAR *)gra_printerlist[i]);
	gra_printerlistcount = 0;
	nulllplist[0] = 0;

	/* look for "/etc/printcap" */
	io = efopen(x_("/etc/printcap"), x_("r"));
	if (io != 0)
	{
		for(;;)
		{
			if (xfgets(buf, 200, io)) break;
			if (buf[0] == '#' || buf[0] == ' ' || buf[0] == '\t') continue;
			for(pt = buf; *pt != 0; pt++)
				if (*pt == ':') break;
			if (*pt == 0) continue;
			*pt = 0;
			if (gra_addprinter(buf))
				return(nulllplist);
		}
		fclose(io);
		if (gra_printerlistcount <= 0) return(nulllplist);
		return(gra_printerlist);
	}

	/* no file "/etc/printcap": try to run "lpget" */
	(void)epipe(pipechans);
	process = efork();
	if (process == 1)
		return(nulllplist);

	/* fork the process */
	if (process == 0)
	{
		save = channelreplacewithchannel(1, pipechans[1]);
		execargs[0] = x_("lpget");
		execargs[1] = x_("list");
		execargs[2] = 0;
		eexec(x_("lpget"), execargs);
		channelrestore(1, save);
		ttyputerr(_("Cannot find 'lpget'"));
		exit(1);
	}
	eclose(pipechans[1]);

	/* read the list from the program */
	bptr = 0;
	for(;;)
	{
		i = eread(pipechans[0], (UCHAR1 *)&val, SIZEOFCHAR);
		if (i != 1) break;
		if (val == ':')
		{
			if (gra_addprinter(buf))
				return(nulllplist);
		}
		buf[bptr++] = val;   buf[bptr] = 0;
		if (val == '\n' || val == '\r') bptr = 0;
	}
	ewait(process);
	if (gra_printerlistcount <= 0) return(nulllplist);
	return(gra_printerlist);
}

BOOLEAN gra_addprinter(CHAR *buf)
{
	CHAR **newlist;
	INTBIG i, j, newtotal;

	if (estrcmp(buf, x_("_default")) == 0) return(FALSE);
	if (buf[0] == ' ' || buf[0] == '\t') return(FALSE);

	/* stop if it is already in the list */
	for(i=0; i<gra_printerlistcount; i++)
		if (estrcmp(buf, gra_printerlist[i]) == 0) return(FALSE);

	/* make room in the list */
	if (gra_printerlistcount >= gra_printerlisttotal-1)
	{
		newtotal = gra_printerlistcount + 10;
		newlist = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), us_tool->cluster);
		if (newlist == 0) return(TRUE);
		for(i=0; i<gra_printerlistcount; i++)
			newlist[i] = gra_printerlist[i];
		if (gra_printerlisttotal > 0) efree((CHAR *)gra_printerlist);
		gra_printerlist = newlist;
		gra_printerlisttotal = newtotal;
	}

	/* insert into the list */
	for(i=0; i<gra_printerlistcount; i++)
		if (estrcmp(buf, gra_printerlist[i]) < 0) break;
	for(j=gra_printerlistcount; j>i; j--) gra_printerlist[j] = gra_printerlist[j-1];
	gra_printerlist[i] = (CHAR *)emalloc((estrlen(buf)+1) * SIZEOFCHAR, us_tool->cluster);
	estrcpy(gra_printerlist[i], buf);
	gra_printerlistcount++;
	gra_printerlist[gra_printerlistcount] = 0;
	return(FALSE);
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
	efree(sf->label);
	efree((CHAR *)sf);
}

/*
 * Routine to display "message" in the status "field" of window "frame" (uses all windows
 * if "frame" is zero).  If "cancrop" is false, field cannot be cropped and should be
 * replaced with "*" if there isn't room.
 */
void ttysetstatusfield(WINDOWFRAME *wwf, STATUSFIELD *field, CHAR *message, BOOLEAN cancrop)
{
	INTBIG len, lx, ly, hx, hy, wid, hei, sizey, i;
	INTBIG oldfont;
	CHAR *ptr;
	CHAR1 *title1, title1byte[200];
	XTextProperty wintitle;
	REGISTER WINDOWFRAME *wf;
	WINDOWPART win;

	if (field == 0) return;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;
		if (wwf != NOWINDOWFRAME && wwf != wf) continue;

		/* make a window that corresponds to this frame */
		win.frame = wf;
		win.screenlx = win.uselx = 0;
		win.screenhx = win.usehx = wf->swid;
		win.screenly = win.usely = -gra_status_height;
		win.screenhy = win.usehy = wf->shei;
		win.state = DISPWINDOW;
		computewindowscale(&win);

		if (field->line == 0)
		{
			/* should set title bar here */
#ifdef EPROGRAMNAME
			(void)estrcpy(gra_localstring, string2byte(EPROGRAMNAME));
#else
			(void)estrcpy(gra_localstring, _("Electric"));
#endif
			if (estrlen(message) > 0)
			{
				(void)estrcat(gra_localstring, x_(" ("));
				(void)estrcat(gra_localstring, field->label);
				(void)estrcat(gra_localstring, message);
				(void)estrcat(gra_localstring, x_(")"));
			}
			len = estrlen(gra_localstring);
			while (len > 0 && gra_localstring[len-1] == ' ') gra_localstring[--len] = 0;
			strcpy(title1byte, string1byte(gra_localstring));
			title1 = title1byte;
			XStringListToTextProperty(&title1, 1, &wintitle);
			XSetWMName(wf->topdpy, wf->topwin, &wintitle);
			continue;
		}

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
			esnprintf(gra_localstring, MAXLOCALSTRING, x_("%s%s"), field->label, message);
		}
		len = estrlen(gra_localstring);
		while (len > 0 && gra_localstring[len-1] == ' ') gra_localstring[--len] = 0;

		oldfont = gra_truetypesize;
# ifdef TRUETYPE
		if (gra_truetypeon != 0)
		{
			gra_settextsize(&win, 12, 0, 0, 0, 0, 0);
		} else
#endif
		gra_settextsize(&win, 6, 0, 0, 0, 0, 0);
		sizey = gra_curfont->ascent + gra_curfont->descent;

		lx = wf->swid * field->startper / 100;
		hx = wf->swid * field->endper / 100;
		ly = - (gra_status_height / field->line);
		hy = ly + sizey;
		while (gra_localstring[0] != 0)
		{
			screengettextsize(&win, gra_localstring, &wid, &hei);
			if (wid <= hx-lx) break;
			gra_localstring[estrlen(gra_localstring)-1] = 0;
		}
		screendrawbox(&win, lx, hx - 1, ly, hy, &us_ebox);
		screendrawtext(&win, lx, ly+1, gra_localstring, &us_menutext);  /* added "+1" */
		gra_settextsize(&win, oldfont, 0, 0, 0, 0, 0);

		/* draw the separator line between drawing and status areas */
		us_box.col = BLACK;
		screendrawline(&win, 1, -2, wf->swid - 1, -2, &us_box, 0);
	}
}

/*
 * Routine to force the module to return with some event so that the user interface will
 * relinquish control and let a slice happen.
 */
void forceslice(void)
{
    XSendEvent(gra_samplemotionevent.xmotion.display,
	       gra_samplemotionevent.xmotion.window,
	       True, PointerMotionMask, &gra_samplemotionevent);
}

/******************** MESSAGES WINDOW ROUTINES ********************/

/*
 * Routine to put the string "s" into the messages window.
 * Pops up the messages window if "important" is nonzero.
 */
void putmessagesstring(CHAR *s, BOOLEAN important)
{
	XmTextPosition tp;
	XWindowAttributes xwa;

	/* print to stderr if messages are not initiaized */
	if (gra_messageswidget == 0) {
		efprintf(stderr, x_("%s\n"), s);
		return;
	}

	/* make sure the window isn't iconified or obscured */
	if (important)
	{
		if (XGetWindowAttributes(gra_topmsgdpy, gra_topmsgwin, &xwa) == 0) return;
		if (xwa.map_state != IsViewable || gra_messages_obscured)
			XMapRaised(gra_topmsgdpy, gra_topmsgwin);
	}

	tp = XmTextGetLastPosition(gra_messageswidget);
	XmTextInsert(gra_messageswidget, tp, string1byte(s));
	tp = XmTextGetLastPosition(gra_messageswidget);
	XmTextInsert(gra_messageswidget, tp, b_("\n"));
	tp = XmTextGetLastPosition(gra_messageswidget);
	XmTextSetSelection(gra_messageswidget, tp, tp, 0);
	XmTextSetInsertionPosition(gra_messageswidget, tp);
	XmTextShowPosition(gra_messageswidget, tp);
}

/*
 * Routine to return the name of the key that ends a session from the messages window.
 */
CHAR *getmessageseofkey(void)
{
	return(x_("^D"));
}

CHAR *getmessagesstring(CHAR *prompt)
{
	XmTextPosition tp, starttp;
	CHAR ch[2], *block;
	CHAR1 *block1;
	INTBIG c, total, special;
	REGISTER void *infstr;

	/* show the prompt */
	tp = XmTextGetLastPosition(gra_messageswidget);
	XmTextInsert(gra_messageswidget, tp, string1byte(prompt));

	/* get initial text position */
	starttp = XmTextGetLastPosition(gra_messageswidget);
	XmTextSetSelection(gra_messageswidget, starttp, starttp, 0);
	XmTextSetInsertionPosition(gra_messageswidget, starttp);

	/* loop while typing is done */
	total = 0;
	gra_messages_typingin = TRUE;
	for(;;)
	{
		c = getnxtchar(&special);
		if (c == '\n' || c == '\r' || c == CTRLDKEY) break;
		if (c == DELETEKEY || c == BACKSPACEKEY)
		{
			tp = XmTextGetLastPosition(gra_messageswidget);
			XmTextSetSelection(gra_messageswidget, tp-1, tp, 0);
			XmTextRemove(gra_messageswidget);
			total--;
			continue;
		}
		if (c == us_killch)
		{
			tp = XmTextGetLastPosition(gra_messageswidget);
			XmTextSetSelection(gra_messageswidget, starttp, tp, 0);
			XmTextRemove(gra_messageswidget);
			total = 0;
			continue;
		}
		tp = XmTextGetLastPosition(gra_messageswidget);
		ch[0] = c;
		ch[1] = 0;
		XmTextInsert(gra_messageswidget, tp, string1byte(ch));
		tp = XmTextGetLastPosition(gra_messageswidget);
		XmTextSetSelection(gra_messageswidget, tp, tp, 0);
		total++;
	}
	gra_messages_typingin = FALSE;

	block1 = (CHAR1 *)emalloc((total+2) * (sizeof (CHAR1)), el_tempcluster);
	if (block1 == 0)
	{
		efprintf(stderr, _("Cannot make type-in buffer\n"));
		return(0);
	}
	XmTextGetSubstring(gra_messageswidget, starttp, total+1, total+2, block1);
	block1[total] = 0;
	infstr = initinfstr();
	addstringtoinfstr(infstr, string2byte(block1));
	efree((CHAR *)block1);
	block = returninfstr(infstr);

	/* add in final carriage return */
	tp = XmTextGetLastPosition(gra_messageswidget);
	XmTextInsert(gra_messageswidget, tp, b_("\n"));

	/* return the string (0 on EOF or null) */
	if (*block == 0 && c == 4) return(0);
	return(block);
}

/* X Font Selection */
DIALOGITEM gra_xfontdialogitems[] =
{
 /*  1 */ {0, {312,592,336,652}, BUTTON, N_("OK")},
 /*  2 */ {0, {312,312,336,372}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,88}, MESSAGE, N_("Foundry")},
 /*  4 */ {0, {8,96,24,296}, POPUP, x_("")},
 /*  5 */ {0, {32,8,48,88}, MESSAGE, N_("Family")},
 /*  6 */ {0, {32,96,48,296}, POPUP, x_("")},
 /*  7 */ {0, {56,8,72,88}, MESSAGE, N_("Weight")},
 /*  8 */ {0, {56,96,72,296}, POPUP, x_("")},
 /*  9 */ {0, {80,8,96,88}, MESSAGE, N_("Slant")},
 /* 10 */ {0, {80,96,96,296}, POPUP, x_("")},
 /* 11 */ {0, {104,8,120,88}, MESSAGE, N_("S Width")},
 /* 12 */ {0, {104,96,120,296}, POPUP, x_("")},
 /* 13 */ {0, {128,8,144,88}, MESSAGE, N_("Ad Style")},
 /* 14 */ {0, {128,96,144,296}, POPUP, x_("")},
 /* 15 */ {0, {152,8,168,88}, MESSAGE, N_("Pixel Size")},
 /* 16 */ {0, {152,96,168,296}, POPUP, x_("")},
 /* 17 */ {0, {176,8,192,88}, MESSAGE, N_("Point Size")},
 /* 18 */ {0, {176,96,192,296}, POPUP, x_("")},
 /* 19 */ {0, {200,8,216,88}, MESSAGE, N_("X Resolution")},
 /* 20 */ {0, {200,96,216,296}, POPUP, x_("")},
 /* 21 */ {0, {224,8,240,88}, MESSAGE, N_("Y Resolution")},
 /* 22 */ {0, {224,96,240,296}, POPUP, x_("")},
 /* 23 */ {0, {248,8,264,88}, MESSAGE, N_("Spacing")},
 /* 24 */ {0, {248,96,264,296}, POPUP, x_("")},
 /* 25 */ {0, {272,8,288,88}, MESSAGE, N_("Avg Width")},
 /* 26 */ {0, {272,96,288,296}, POPUP, x_("")},
 /* 27 */ {0, {296,8,312,88}, MESSAGE, N_("Registry")},
 /* 28 */ {0, {296,96,312,296}, POPUP, x_("")},
 /* 29 */ {0, {320,8,336,88}, MESSAGE, N_("Encoding")},
 /* 30 */ {0, {320,96,336,296}, POPUP, x_("")},
 /* 31 */ {0, {8,304,300,656}, SCROLL, x_("")}
};
DIALOG gra_xfontdialog = {{75,75,421,740}, N_("Font Selection"), 0, 31, gra_xfontdialogitems, 0, 0};

/* special items for the messages window font dialog: */
#define DMSF_FIRSTPOPUP    4		/* first popup (popup) */
#define DMSF_LASTPOPUP    30		/* last popup (popup) */
#define DMSF_FONTLIST     31		/* list of fonts (scroll) */

/*
 * routine to select fonts in the messages window
 */
void setmessagesfont(void)
{
	CHAR1 **fontlist;
	CHAR *required[14], ***choices;
	int namecount;
	INTBIG i, j, k, *numchoices;
	INTBIG itemHit;
	REGISTER void *dia;
	Arg arg[1];
	XmFontListEntry entry;
	XmFontList xfontlist;

	/* get the list of available fonts */
	fontlist = XListFonts(gra_altdpy, b_("-*-*-*-*-*-*-*-*-*-*-*-*-*-*"),
		2000, &namecount);

	/* show the dialog */
	dia = DiaInitDialog(&gra_xfontdialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DMSF_FONTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone,
		-1, SCDOUBLEQUIT|SCSELMOUSE|SCSMALLFONT|SCFIXEDWIDTH|SCHORIZBAR);

	/* setup so that all fields are possible */
	for(j=0; j<14; j++)
	{
		required[j] = (CHAR *)emalloc(200*SIZEOFCHAR, el_tempcluster);
		if (required[j] == 0)
		{
			efprintf(stderr, _("Cannot make field for dialog\n"));
			return;
		}
		estrcpy(required[j], x_("*"));
	}

	/* examine all of the fonts and make a lists of choices for each position */
	gra_makefontchoices(fontlist, namecount, required, &choices, &numchoices, dia);
	for(j=0; j<14; j++)
	{
		DiaSetPopup(dia, j*2+DMSF_FIRSTPOPUP, numchoices[j], choices[j]);
		DiaSetPopupEntry(dia, j*2+DMSF_FIRSTPOPUP, 0);
	}
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit >= DMSF_FIRSTPOPUP && itemHit <= DMSF_LASTPOPUP && (itemHit / 2) * 2 == itemHit)
		{
			/* popup changed */
			for(j=0; j<14; j++)
			{
				i = DiaGetPopupEntry(dia, j*2+DMSF_FIRSTPOPUP);
				estrcpy(required[j], choices[j][i]);
			}
			gra_makefontchoices(fontlist, namecount, required, &choices, &numchoices, dia);
			for(j=0; j<14; j++)
			{
				DiaSetPopup(dia, j*2+DMSF_FIRSTPOPUP, numchoices[j], choices[j]);
				for(i=0; i<numchoices[j]; i++)
					if (namesame(choices[j][i], required[j]) == 0) break;
				DiaSetPopupEntry(dia, j*2+DMSF_FIRSTPOPUP, i);
			}
			continue;
		}
	}
	if (itemHit == OK)
	{
		i = DiaGetCurLine(dia, DMSF_FONTLIST);
		estrcpy(required[0], DiaGetScrollLine(dia, DMSF_FONTLIST, i));
		entry = XmFontListEntryLoad(gra_altdpy, string1byte(required[0]),
			XmFONT_IS_FONT, b_("TAG"));
		xfontlist = XmFontListAppendEntry(NULL, entry);
		XmFontListEntryFree(&entry);
		XtSetArg(arg[0], XmNfontList, xfontlist);
		XtSetValues(gra_messageswidget, arg, 1);
		XmFontListFree(xfontlist);
	}
	DiaDoneDialog(dia);
	XFreeFontPath(fontlist);

	/* free the memory */
	for(j=0; j<14; j++)
	{
		for(k=0; k<numchoices[j]; k++)
			efree((CHAR *)choices[j][k]);
		efree((CHAR *)required[j]);
	}
}

void gra_makefontchoices(CHAR1 **fontlist, INTBIG namecount, CHAR *required[14],
	CHAR ****fchoices, INTBIG **fnumchoices, void *dia)
{
	CHAR *pt, *start, save, **newchoices;
	INTBIG i, j, k, newsize;
	static INTBIG totalchoices[14], numchoices[14];
	static CHAR **choices[14], *current[14];
	static BOOLEAN inited = FALSE;

	/* load the return parameters */
	*fchoices = (CHAR ***)choices;
	*fnumchoices = (INTBIG *)numchoices;

	/* one-time memory allocation and initialization */
	if (!inited)
	{
		inited = TRUE;
		for(j=0; j<14; j++)
		{
			totalchoices[j] = 5;
			choices[j] = (CHAR **)emalloc(5 * (sizeof (CHAR *)),
				el_tempcluster);
			if (choices[j] == 0)
			{
				efprintf(stderr, _("Cannot make choice field for dialog\n"));
				return;
			}
			numchoices[j] = 0;
			current[j] = (CHAR *)emalloc(200*SIZEOFCHAR, el_tempcluster);
			if (current[j] == 0)
			{
				efprintf(stderr, _("Cannot make answer field for dialog\n"));
				return;
			}
		}
	}

	/* initialize the popup choices */
	for(j=0; j<14; j++)
	{
		/* free former memory */
		for(k=0; k<numchoices[j]; k++)
			efree((CHAR *)choices[j][k]);
		numchoices[j] = 1;
		choices[j][0] = (CHAR *)emalloc(2*SIZEOFCHAR, el_tempcluster);
		if (choices[j][0] == 0)
		{
			efprintf(stderr, _("Cannot make popup for dialog\n"));
			return;
		}
		estrcpy(choices[j][0], x_("*"));
	}

	/* initialize the scroll area */
	DiaLoadTextDialog(dia, DMSF_FONTLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);

	/* examine each font and build the choice list */
	for(i=0; i<namecount; i++)
	{
		/* break the font name into fields, make sure it matches filter */
		pt = string2byte(fontlist[i]);
		for(j=0; j<14; j++)
		{
			if (*pt == '-') pt++;
			start = pt;
			while (*pt != 0 && *pt != '-') pt++;
			save = *pt;
			*pt = 0;
			if (*start == 0) estrcpy(current[j], x_("(nil)")); else
				estrcpy(current[j], start);
			*pt = save;
			if (required[j][0] == '*') continue;
			if (namesame(required[j], current[j]) != 0) break;
		}
		if (j < 14) continue;
		DiaStuffLine(dia, DMSF_FONTLIST, string2byte(fontlist[i]));

		/* add this to the list of choices */
		for(j=0; j<14; j++)
		{
			for(k=0; k<numchoices[j]; k++)
				if (namesame(current[j], choices[j][k]) == 0) break;
			if (k < numchoices[j]) continue;

			/* add this choice */
			if (numchoices[j] >= totalchoices[j])
			{
				newsize = totalchoices[j] * 2;
				newchoices = (CHAR **)emalloc(newsize * (sizeof (CHAR *)),
					el_tempcluster);
				if (newchoices == 0)
				{
					efprintf(stderr, _("Cannot make choice list for dialog\n"));
					return;
				}
				for(k=0; k<numchoices[j]; k++)
					newchoices[k] = choices[j][k];
				efree((CHAR *)choices[j]);
				choices[j] = newchoices;
				totalchoices[j] = newsize;
			}
			choices[j][numchoices[j]] = (CHAR *)emalloc((estrlen(current[j])+1) * SIZEOFCHAR,
				el_tempcluster);
			if (choices[j][numchoices[j]] == 0)
			{
				efprintf(stderr, _("Cannot make choice entry for dialog\n"));
				return;
			}
			estrcpy(choices[j][numchoices[j]], current[j]);
			numchoices[j]++;
		}
	}

	/* sort the choice lists */
	for(j=0; j<14; j++)
	{
		switch (j)
		{
			case 6:
			case 7:
			case 8:
			case 9:
			case 11:
				esort(&choices[j][1], numchoices[j]-1, sizeof (CHAR *), gra_sortnumbereascending);
				break;
			default:
				esort(&choices[j][1], numchoices[j]-1, sizeof (CHAR *), sort_stringascending);
				break;
		}
	}
	DiaSelectLine(dia, DMSF_FONTLIST, -1);
}

int gra_sortnumbereascending(const void *e1, const void *e2)
{
	REGISTER CHAR *c1, *c2;

	c1 = *((CHAR **)e1);
	c2 = *((CHAR **)e2);
	return(eatoi(c1) - eatoi(c2));
}

/*
 * routine to cut text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN cutfrommessages(void)
{
	/* stop now if messages window is not on top */
	if (!gra_msgontop) return(FALSE);

	XmTextCut(gra_messageswidget, CurrentTime);
	return(TRUE);
}

/*
 * routine to copy text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN copyfrommessages(void)
{
	/* stop now if messages window is not on top */
	if (!gra_msgontop) return(FALSE);

	XmTextCopy(gra_messageswidget, CurrentTime);
	return(TRUE);
}

/*
 * routine to paste text to the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN pastetomessages(void)
{
	/* stop now if messages window is not on top */
	if (!gra_msgontop) return(FALSE);

	XmTextPaste(gra_messageswidget);
	return(TRUE);
}

/*
 * routine to get the contents of the system cut buffer
 */
CHAR *getcutbuffer(void)
{
	unsigned long length, got;
	int status;
	CHAR *tmpbuf;
	REGISTER void *infstr;

	/* determine the amount of data in the clipboard */
	for(;;)
	{
		status = XmClipboardInquireLength(gra_topmsgdpy, gra_topmsgwin, b_("STRING"), &length);
		if (status != ClipboardLocked) break;
	}
	if (length <= 0) return(x_(""));

	/* get the data */
	tmpbuf = (CHAR *)emalloc((length+1) * SIZEOFCHAR, el_tempcluster);
	if (tmpbuf == 0) return(x_(""));
	for(;;)
	{
		status = XmClipboardRetrieve(gra_topmsgdpy, gra_topmsgwin, b_("STRING"),
			tmpbuf, length+1, &got, NULL);
		if (status != ClipboardLocked) break;
	}
	tmpbuf[got] = 0;
	infstr = initinfstr();
	addstringtoinfstr(infstr, tmpbuf);
	efree((CHAR *)tmpbuf);
	return(returninfstr(infstr));
}
        
/*
 * routine to set the contents of the system cut buffer to "msg"
 */
void setcutbuffer(CHAR *msg)
{
	XmString label;
	int status;
	static int copycount = 1;
	long itemid = 0;

	/* open the clipboard */
	label = XmStringCreateLocalized(b_("electric"));
	for(;;)
	{
		status = XmClipboardStartCopy(gra_topmsgdpy, gra_topmsgwin, label,
			CurrentTime, NULL, NULL, &itemid);
		if (status != ClipboardLocked) break;
	}
	XmStringFree(label);

	/* copy the data */
	copycount++;
	for(;;)
	{
		status = XmClipboardCopy(gra_topmsgdpy, gra_topmsgwin, itemid, b_("STRING"),
			msg, estrlen(msg)+1, copycount, NULL);
		if (status != ClipboardLocked) break;
	}

	/* close the clipboard */
	for(;;)
	{
		status = XmClipboardEndCopy(gra_topmsgdpy, gra_topmsgwin, itemid);
		if (status != ClipboardLocked) break;
	}
}

/*
 * Routine to clear all text from the messages window.
 */
void clearmessageswindow(void)
{
	XmTextPosition tp;

	tp = XmTextGetLastPosition(gra_messageswidget);
	XmTextSetSelection(gra_messageswidget, 0, tp, 0);
	XmTextRemove(gra_messageswidget);
}

/******************** GRAPHICS CONTROL ROUTINES ********************/

void flushscreen(void)
{
#ifdef ANYDEPTH
	REGISTER INTBIG x, y, lx, hx, ly, hy;
	REGISTER UCHAR1 *srow, *drow, *sval;
	REGISTER UCHAR1 **srowstart, **drowstart;
	REGISTER INTBIG *colorvalue;
	REGISTER WINDOWFRAME *wf;
	REGISTER WINDOWDISPLAY *wd;
	UCHAR1 col[4];
	GC gc;
	XGCValues gcv;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		/* nothing to do if no changes have been made to offscreen buffer */
		if (!wf->offscreendirty) continue;
		wf->offscreendirty = FALSE;

		/* get bounds of display update */
		lx = wf->copyleft;   hx = wf->copyright;
		ly = wf->copytop;    hy = wf->copybottom;
		if (lx < 0) lx = 0;
		if (hx > wf->swid) hx = wf->swid;
		if (ly < 0) ly = 0;
		if (hy > wf->trueheight) hy = wf->trueheight;

		/* copy from offscreen buffer to Ximage while doing color mapping */
		wd = wf->wd;
		srowstart = wf->rowstart;
		colorvalue = wd->colorvalue;
		drowstart = wd->rowstart;
		switch (wd->depth)
		{
			case 8:
				for(y=ly; y<hy; y++)
				{
					srow = (UCHAR1 *)(srowstart[y] + lx);
					drow = (UCHAR1 *)(drowstart[y] + lx);
					for(x=lx; x<hx; x++)
						*drow++ = colorvalue[*srow++];
				}
				break;

			case 16:
				for(y=ly; y<hy; y++)
				{
					srow = (UCHAR1 *)(srowstart[y] + lx);
					drow = (UCHAR1 *)(drowstart[y] + (lx << 1));
					for(x=lx; x<hx; x++)
					{
						sval = (UCHAR1 *)&colorvalue[*srow++];
						*drow++ = sval[0];
						*drow++ = sval[1];
					}
				}
				break;

			case 24:
			case 32:
				for(y=ly; y<hy; y++)
				{
					srow = (UCHAR1 *)(srowstart[y] + lx);
					drow = (UCHAR1 *)(drowstart[y] + (lx << 2));
					for(x=lx; x<hx; x++)
					{
						sval = (UCHAR1 *)&colorvalue[*srow++];
#if 1
#  ifdef BYTES_SWAPPED
						col[gra_mainredshift>>3] = sval[3];
						col[gra_maingreenshift>>3] = sval[2];
						col[gra_mainblueshift>>3] = sval[1];
						col[3] = sval[0];
#  else
						col[gra_mainredshift>>3] = sval[0];
						col[gra_maingreenshift>>3] = sval[1];
						col[gra_mainblueshift>>3] = sval[2];
						col[3] = sval[3];
#  endif
						*drow++ = col[3];
						*drow++ = col[2];
						*drow++ = col[1];
						*drow++ = col[0];
#else
#  ifdef BYTES_SWAPPED
						*drow++ = sval[0];
						*drow++ = sval[1];
						*drow++ = sval[2];
						*drow++ = sval[3];
#  else
						*drow++ = sval[2];
						*drow++ = sval[1];
						*drow++ = sval[0];
						*drow++ = sval[3];
#  endif
#endif
					}
				}
				break;
		}

		/* copy XImage to the screen */
		gcv.foreground = BlackPixel(wf->topdpy, DefaultScreen(wf->topdpy));
		gc = XtGetGC(wf->graphicswidget, GCForeground, &gcv);
		XPutImage(wf->topdpy, wf->win, gc, wd->image, lx, ly, lx, ly, hx-lx, hy-ly);
		XtReleaseGC(wf->graphicswidget, gc);
		XFlush(wf->topdpy);
	}
#endif
}

/*
 * Routine to mark an area of the offscreen buffer as "changed".
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

void gra_getdisplaydepth(Display *dpy, INTBIG *depth, INTBIG *truedepth)
{
	*depth = DefaultDepth(dpy, DefaultScreen(dpy));
	*truedepth = *depth;
	if (*truedepth == 24) *truedepth = 32;
}

BOOLEAN gra_makewindowdisplaybuffer(WINDOWDISPLAY *wd, Display *dpy)
{
	INTBIG amt, i, scr;
	Visual *visual;

	/* get information about this display */
	gra_getdisplaydepth(dpy, &wd->depth, &wd->truedepth);
	scr = DefaultScreen(dpy);
	wd->wid = DisplayWidth(dpy, scr);
	wd->hei = DisplayHeight(dpy, scr);
	visual = DefaultVisual(dpy, scr);

	/* allocate data for ximage */
	amt = wd->wid * wd->hei * wd->truedepth / 8;
	wd->addr = (UCHAR1 *)emalloc(amt, us_tool->cluster);
	if (wd->addr == 0)
	{
		efprintf(stderr, _("Error allocating %ld image array\n"), amt);
		return(TRUE);
	}

	/* make the ximage */
	wd->image = XCreateImage(gra_maindpy, visual, wd->depth, ZPixmap,
		0, (CHAR1 *)wd->addr, wd->wid, wd->hei, wd->truedepth, 0);
	if (wd->image == 0)
	{
		efprintf(stderr, _("Error allocating %ldx%ld image array\n"), wd->wid, wd->hei);
		return(TRUE);
	}

	/* make the row-start pointers for this image */
	wd->rowstart = (UCHAR1 **)emalloc(wd->hei * SIZEOFINTBIG, us_tool->cluster);
	if (wd->rowstart == 0)
	{
		efprintf(stderr, _("Error allocating %ld-long row pointers\n"), wd->hei);
		return(TRUE);
	}
	for(i=0; i<wd->hei; i++)
		wd->rowstart[i] = wd->addr + i * wd->image->bytes_per_line;
	return(FALSE);
}

/*
 * Routine to change the default cursor (to indicate modes).
 */
void setnormalcursor(INTBIG curs)
{
	us_normalcursor = curs;
	setdefaultcursortype(us_normalcursor);
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
#ifdef ANYDEPTH
	REGISTER INTBIG i;
	REGISTER INTBIG r, g, b, depth;
	XColor xc;
	REGISTER WINDOWFRAME *wf;
	Visual *visual;

	/* clip to the number of map entries requested */
	if (high >= el_maplength) high = el_maplength - 1;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		visual = DefaultVisual(wf->topdpy, DefaultScreen(wf->topdpy));
		for(i=low; i<=high; i++)
		{
			xc.flags = DoRed | DoGreen | DoBlue;
			r = red[i-low];
			g = green[i-low];
			b = blue[i-low];

			/* always complement the highest color */
			if (low < 255 && i == 255)
			{
				r = 255 - red[i-low];
				g = 255 - green[i-low];
				b = 255 - blue[i-low];
			}

			/* for 16bpp, crop values to 5 bits */
			xc.red   = r << 8;
			xc.green = g << 8;
			xc.blue  = b << 8;

			/* if this is color-mapped, get closest entry in that map */
			if (VisualClass(visual) == PseudoColor ||
				VisualClass(visual) == StaticColor)
			{
				XAllocColor(wf->topdpy, wf->colormap, &xc);
				wf->wd->colorvalue[i] = xc.pixel;
			} else
			{
				depth = DefaultDepth(wf->topdpy, DefaultScreen(wf->topdpy));
				if (depth == 16)
				{
					wf->wd->colorvalue[i] = ((r & 0xF8) << 8) |
						((g & 0xF8) << 3) | ((b & 0xF8) >> 3);
				} else
				{
					wf->wd->colorvalue[i] = (b << 16) | (g << 8) | r;
				}
			}
		}

		/* mark the entire screen for redrawing */
		if (low == 0 && high == 255)
		{
			gra_setrect(wf, 0, wf->swid, 0, wf->trueheight);
			flushscreen();
		}

		/* set the cursor color */
		if (low == 0 || (low <= el_colcursor && high >= el_colcursor))
		{
			gra_xfc.flags = gra_xbc.flags = DoRed | DoGreen | DoBlue;
			gra_xbc.red = red[0] << 8;
			gra_xbc.green = green[0] << 8;
			gra_xbc.blue = blue[0] << 8;
			gra_xfc.red = red[el_colcursor] << 8;
			gra_xfc.green = green[el_colcursor] << 8;
			gra_xfc.blue = blue[el_colcursor] << 8;
			gra_recolorcursor(wf->topdpy, &gra_xfc, &gra_xbc);
		}
	}
#else
	REGISTER INTBIG i;
	XColor xc[256];
	REGISTER WINDOWFRAME *wf;

	/* clip to the number of map entries requested */
	if (high > gra_maphigh) high = gra_maphigh;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		for(i=low; i<=high; i++)
		{
			xc[i-low].flags = DoRed | DoGreen | DoBlue;
			xc[i-low].pixel = i;
			xc[i-low].red   =   red[i-low] << 8;
			xc[i-low].green = green[i-low] << 8;
			xc[i-low].blue  =  blue[i-low] << 8;

			/* always complement the highest color */
			if (low < 255 && i == 255)
			{
				xc[i-low].red   = (255 -   red[i-low]) << 8;
				xc[i-low].green = (255 - green[i-low]) << 8;
				xc[i-low].blue  = (255 -  blue[i-low]) << 8;
			}
		}

		XStoreColors(wf->topdpy, wf->colormap, xc, high-low + 1);

		if (low == 0 || (low <= el_colcursor && high >= el_colcursor))
		{
			gra_xfc.pixel = el_colcursor;
			XQueryColor(wf->topdpy, wf->colormap, &gra_xfc);
			gra_xbc.pixel = 0;
			XQueryColor(wf->topdpy, wf->colormap, &gra_xbc);
			gra_recolorcursor(wf->topdpy, &gra_xfc, &gra_xbc);
		}
	}
#endif

	/* change messages window colors */
	if (low == 0 && high == 255 && gra_messageswidget != 0)
	{
		Pixel fg, bg;
#if 0
		XColor xc;

		XtVaGetValues(gra_messageswidget, XtNbackground, &bg, XtNforeground, &fg, NULL);
		xc.flags = DoRed | DoGreen | DoBlue;
		xc.pixel = bg;
		xc.red = red[0]<<8;   xc.green = green[0]<<8;   xc.blue = blue[0]<<8;
		XStoreColor(gra_topmsgdpy, gra_messagescolormap, &xc);

		xc.flags = DoRed | DoGreen | DoBlue;
		xc.pixel = bg;
		xc.red = red[CELLTXT]<<8;   xc.green = green[CELLTXT]<<8;   xc.blue = blue[CELLTXT]<<8;
		XStoreColor(gra_topmsgdpy, gra_messagescolormap, &xc);
#else
		Visual *visual;
		visual = DefaultVisual(gra_topmsgdpy, DefaultScreen(gra_topmsgdpy));
		if (VisualClass(visual) != PseudoColor && VisualClass(visual) != StaticColor)
		{
			bg = (blue[0] << 16) | (green[0] << 8) | red[0];
			fg = (blue[CELLTXT] << 16) | (green[CELLTXT] << 8) | red[CELLTXT];
			XtVaSetValues(gra_messageswidget, XmNbackground, bg, XmNforeground, fg, NULL);
			XtVaSetValues(gra_messageswidget, XtNbackground, bg, XtNforeground, fg, NULL);
		}
#endif
	}
}

/*
 * Helper routine to change the cursor color.
 * fg and bg are the XColor structures to be used. We will look up the current
 * values for the cursor and the background from the installed color map, and
 * copy them into bg and fg.
 */
void gra_recolorcursor(Display *dpy, XColor *fg, XColor *bg)
{
	XRecolorCursor(dpy, gra_realcursor, fg, bg);
	XRecolorCursor(dpy, gra_nomousecursor, fg, bg);
	XRecolorCursor(dpy, gra_drawcursor, fg, bg);
	XRecolorCursor(dpy, gra_nullcursor, fg, bg);
	XRecolorCursor(dpy, gra_menucursor, fg, bg);
	XRecolorCursor(dpy, gra_handcursor, fg, bg);
	XRecolorCursor(dpy, gra_techcursor, fg, bg);
	XRecolorCursor(dpy, gra_ibeamcursor, fg, bg);
	XRecolorCursor(dpy, gra_lrcursor, fg, bg);
	XRecolorCursor(dpy, gra_udcursor, fg, bg);
}

#ifndef ANYDEPTH
/*
 * routine to load the global Graphic Context "wf->gcstip" with the
 * stipple pattern in "raster"
 */
void gra_loadstipple(WINDOWFRAME *wf, UINTSML raster[])
{
	static Pixmap buildup = (Pixmap)NULL;
	static XImage ximage;
	static INTBIG data[8];
	static GC pgc;
	XGCValues gcv;
	REGISTER INTBIG y, pattern, depth, scr;
	static BOOLEAN reverse = FALSE;   /* for debugging and testing */

	/* initialize raster structure for patterned write */
	if (buildup == (Pixmap)NULL)
	{
		buildup = XCreatePixmap(wf->topdpy, wf->win, 32, 8, 1);
		scr = DefaultScreen(wf->topdpy);
		depth = DefaultDepth(wf->topdpy, scr);
		if (depth != 1)
		{
			/* choose polarity of stipples */
#ifdef i386
			gcv.background = BlackPixel(wf->topdpy, scr);
			gcv.foreground = WhitePixel(wf->topdpy, scr);
#else
			gcv.foreground = BlackPixel(wf->topdpy, scr);
			gcv.background = WhitePixel(wf->topdpy, scr);
#endif
		} else
		{
			/* monochrome */
			gcv.foreground = BlackPixel(wf->topdpy, scr);
			gcv.background = WhitePixel(wf->topdpy, scr);
		}
		if (reverse)
		{
			y = gcv.foreground;
			gcv.foreground = gcv.background;
			gcv.background = y;
		}
		gcv.fill_style = FillStippled;
		pgc = XCreateGC(wf->topdpy, buildup, GCForeground | GCBackground | GCFillStyle, &gcv);
		ximage.width = 32;
		ximage.height = 8;
		ximage.xoffset = 0;
		ximage.format = XYBitmap;
		ximage.data = (CHAR *)data;
		ximage.byte_order = XImageByteOrder(wf->topdpy);
		ximage.bitmap_unit = XBitmapUnit(wf->topdpy);
		ximage.bitmap_bit_order = XBitmapBitOrder(wf->topdpy);
		ximage.bitmap_pad = XBitmapPad(wf->topdpy);
		ximage.depth = 1;
		ximage.bytes_per_line = 4;
	}

	for(y=0; y<8; y++)
	{
		pattern = raster[y] & 0xFFFF;
		data[y] = pattern | (pattern << 16);
	}

	XPutImage(wf->topdpy, buildup, pgc, &ximage, 0, 0, 0, 0, 32, 8);
	XSetStipple(wf->topdpy, wf->gcstip, buildup);
}

void gra_setxstate(WINDOWFRAME *wf, GC gc, GRAPHICS *desc)
{
	XSetPlaneMask(wf->topdpy, gc, desc->bits & gra_maphigh);
	XSetForeground(wf->topdpy, gc, desc->col);
}
#endif

/*
 * Helper routine to set the cursor shape to "state".
 */
void setdefaultcursortype(INTBIG state)
{
	WINDOWFRAME *wf;

	if (us_cursorstate == state) return;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		switch (state)
		{
			case NORMALCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_realcursor);
				break;
			case WANTTTYCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_nomousecursor);
				break;
			case PENCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_drawcursor);
				break;
			case NULLCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_nullcursor);
				break;
			case MENUCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_menucursor);
				break;
			case HANDCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_handcursor);
				break;
			case TECHCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_techcursor);
				break;
			case IBEAMCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_ibeamcursor);
				break;
			case LRCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_lrcursor);
				break;
			case UDCURSOR:
				XDefineCursor(wf->topdpy, wf->topwin, gra_udcursor);
				break;
		}
	}

	us_cursorstate = state;
}

/******************** GRAPHICS LINES ********************/

/*
 * Routine to draw a line in the graphics window.
 */
void screendrawline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, GRAPHICS *desc,
	INTBIG texture)
{
#ifdef ANYDEPTH
	gra_drawline(win, x1, y1, x2, y2, desc, texture);
#else
	static INTBIG on[] =  {0, 1, 6, 1};
	static INTBIG off[] = {0, 3, 2, 7};
	static INTBIG linestyle[] = {LineSolid, LineOnOffDash, LineOnOffDash, LineOnOffDash};
	CHAR dashes[2];
	REGISTER WINDOWFRAME *wf;

	/* adjust the co-ordinates for display mapping */
	wf = win->frame;
	y1 = wf->revy - y1;
	y2 = wf->revy - y2;

	gra_setxstate(wf, wf->gc, desc);
#  ifdef sun
	/*
	 * strange OpenWindows bug requires that the width change in order to
	 * also change the line style
	 */
	XSetLineAttributes(wf->topdpy, wf->gc, 1, linestyle[texture], CapButt, JoinMiter);
#  endif
	XSetLineAttributes(wf->topdpy, wf->gc, 0, linestyle[texture], CapButt, JoinMiter);

	if (texture != 0)
	{
		dashes[0] = on[texture];
		dashes[1] = off[texture];
		XSetDashes(wf->topdpy, wf->gc, 0, dashes, 2);
	}

	XDrawLine(wf->topdpy, wf->win, wf->gc, x1, y1, x2, y2);
#endif
}

/*
 * Routine to invert bits of the line in the graphics window
 */
void screeninvertline(WINDOWPART *win, INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
#ifdef ANYDEPTH
	gra_invertline(win, x1, y1, x2, y2);
#else
	REGISTER WINDOWFRAME *wf;

	/* adjust the co-ordinates for display mapping */
	wf = win->frame;
	y1 = wf->revy - y1;
	y2 = wf->revy - y2;

	XSetPlaneMask(wf->topdpy, wf->gcinv, LAYERH & gra_maphigh);
	XSetLineAttributes(wf->topdpy, wf->gcinv, 0, LineSolid, CapButt, JoinMiter);
	XSetForeground(wf->topdpy, wf->gcinv, HIGHLIT);
	XDrawLine(wf->topdpy, wf->win, wf->gcinv, x1, y1, x2, y2);
#endif
}

/******************** GRAPHICS POLYGONS ********************/

/*
 * Routine to draw a polygon in the graphics window.
 */
void screendrawpolygon(WINDOWPART *win, INTBIG *x, INTBIG *y, INTBIG count, GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawpolygon(win, x, y, count, desc);
#else
	static XPoint *pointlist;
	static INTBIG pointcount = 0;
	REGISTER INTBIG i;
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;
	if (count > pointcount)
	{
		if (pointcount != 0) efree((CHAR *)pointlist);
		pointcount = 0;
		pointlist = (XPoint *)emalloc(count * (sizeof (XPoint)), us_tool->cluster);
		if (pointlist == 0) return;
		pointcount = count;
	}

	for(i=0; i<count; i++)
	{
		pointlist[i].x = x[i];
		pointlist[i].y = wf->revy - y[i];
	}

	if ((desc->colstyle & NATURE) == PATTERNED && desc->col != 0)
	{
		gra_loadstipple(wf, desc->raster);
		gra_setxstate(wf, wf->gcstip, desc);
		XFillPolygon(wf->topdpy, wf->win, wf->gcstip, pointlist, count, Complex,
			CoordModeOrigin);
	} else
	{
		gra_setxstate(wf, wf->gc, desc);
		XFillPolygon(wf->topdpy, wf->win, wf->gc, pointlist, count, Complex, CoordModeOrigin);
	}
#endif
}

/******************** GRAPHICS BOXES ********************/

/*
 * Routine to draw a box in the graphics window.
 */
void screendrawbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy,
	GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawbox(win, lowx, highx, lowy, highy, desc);
#else
	REGISTER WINDOWFRAME *wf;

	/* special code for patterned writes */
	wf = win->frame;
	if ((desc->colstyle & NATURE) == PATTERNED && desc->col != 0)
	{
		gra_loadstipple(wf, desc->raster);
		gra_setxstate(wf, wf->gcstip, desc);
		XFillRectangle(wf->topdpy, wf->win, wf->gcstip, lowx, wf->revy - highy,
			highx - lowx + 1, highy-lowy + 1);
#ifdef sun
		/*
		 * there seems to be a bug in OpenWindows: the right edge of stipple
		 * patterns goes haywire if the pattern is more than 32 pixels wide
		 */
		if (highx - lowx + 1 > 32)
		{
			XFillRectangle(wf->topdpy, wf->win, wf->gcstip, highx - 31,
				wf->revy - highy, 32, highy - lowy + 1);
		}
#endif
	} else
	{
		gra_setxstate(wf, wf->gc, desc);
		XFillRectangle(wf->topdpy, wf->win, wf->gc, lowx, wf->revy - highy,
			highx - lowx + 1, highy - lowy + 1);
	}
#endif
}

/*
 * routine to invert the bits in the box from (lowx, lowy) to (highx, highy)
 */
void screeninvertbox(WINDOWPART *win, INTBIG lowx, INTBIG highx, INTBIG lowy, INTBIG highy)
{
#ifdef ANYDEPTH
	gra_invertbox(win, lowx, highx, lowy, highy);
#else
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;
	XSetPlaneMask(wf->topdpy, wf->gcinv, AllPlanes);
	XFillRectangle(wf->topdpy, wf->win, wf->gcinv, lowx, wf->revy - highy,
		highx - lowx + 1, highy - lowy + 1);
#endif
}

/*
 * routine to move bits on the display starting with the area at
 * (sx,sy) and ending at (dx,dy).  The size of the area to be
 * moved is "wid" by "hei".
 */
void screenmovebox(WINDOWPART *win, INTBIG sx, INTBIG sy, INTBIG wid, INTBIG hei,
	INTBIG dx, INTBIG dy)
{
#ifdef ANYDEPTH
	gra_movebox(win, sx, sy, wid, hei, dx, dy);
#else
	REGISTER WINDOWFRAME *wf;

	/* make sure all bit planes are copied */
	wf = win->frame;
	XSetPlaneMask(wf->topdpy, wf->gc, AllPlanes);

	XCopyArea(wf->topdpy, wf->win, wf->win, wf->gc, sx, wf->revy + 1 - sy - hei,
			  wid, hei, dx, wf->revy + 1 - dy - hei);
#endif
}

/*
 * routine to save the contents of the box from "lx" to "hx" in X and from
 * "ly" to "hy" in Y.  A code is returned that identifies this box for
 * overwriting and restoring.  The routine returns negative if there is a error.
 */
INTBIG screensavebox(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
#ifdef ANYDEPTH
	return(gra_savebox(win, lx, hx, ly, hy));
#else
	REGISTER WINDOWFRAME *wf;
	SAVEDBOX *box;
	REGISTER INTBIG i;
	REGISTER INTBIG depth;

	wf = win->frame;
	i = ly;
	ly = wf->revy - hy;
	hy = wf->revy - i;

	box = (SAVEDBOX *)emalloc((sizeof (SAVEDBOX)), us_tool->cluster);
	if (box == 0) return(-1);
	box->win = win;

	depth = DefaultDepth(wf->topdpy, DefaultScreen(wf->topdpy));
	box->pix = XCreatePixmap(wf->topdpy, wf->win, hx - lx + 1, hy - ly + 1, depth);
	if (box->pix == 0) return(-1);

	box->nextsavedbox = gra_firstsavedbox;
	gra_firstsavedbox = box;
	gra_savedboxcodes++;
	box->code = gra_savedboxcodes;
	box->lx = lx;
	box->hx = hx;
	box->ly = ly;
	box->hy = hy;

	/* make sure the whole buffer is accessable */
	XSetPlaneMask(wf->topdpy, wf->gc, AllPlanes);
	XCopyArea(wf->topdpy, wf->win, box->pix, wf->gc, lx, ly, hx - lx + 1, hy - ly + 1, 0, 0);

	return(box->code);
#endif
}

/*
 * routine to shift the saved box "code" so that it is restored in a different
 * lcoation, offset by (dx,dy)
 */
void screenmovesavedbox(INTBIG code, INTBIG dx, INTBIG dy)
{
#ifdef ANYDEPTH
	gra_movesavedbox(code, dx, dy);
#else
	REGISTER SAVEDBOX *box;

	for(box = gra_firstsavedbox; box != NOSAVEDBOX; box = box->nextsavedbox)
		if (box->code == code) break;
	if (box == NOSAVEDBOX) return;
	box->lx += dx;       box->hx += dx;
	box->ly -= dy;       box->hy -= dy;
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
#ifdef ANYDEPTH
	(void)gra_restorebox(code, destroy);
#else
	REGISTER WINDOWFRAME *wf;
	REGISTER SAVEDBOX *box, *lbox;

	/* find the box corresponding to the "code" */
	lbox = NOSAVEDBOX;
	for(box = gra_firstsavedbox; box != NOSAVEDBOX; box = box->nextsavedbox)
	{
		if (box->code == code) break;
		lbox = box;
	}
	if (box == NOSAVEDBOX) return;
	wf = box->win->frame;

	if (destroy >= 0)
	{
		XSetPlaneMask(wf->topdpy, wf->gc, AllPlanes);

		/* restore the bits in the box */
		XCopyArea(wf->topdpy, box->pix, wf->win, wf->gc, 0, 0, box->hx - box->lx + 1,
			box->hy - box->ly + 1, box->lx, box->ly);
	}

	/* destroy this box's memory if requested */
	if (destroy != 0)
	{
		if (lbox == NOSAVEDBOX)
		{
			gra_firstsavedbox = box->nextsavedbox;
		} else
		{
			lbox->nextsavedbox = box->nextsavedbox;
		}
		XFreePixmap(wf->topdpy, box->pix);
		efree((CHAR *)box);
	}
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
#ifdef TRUETYPE
	INTBIG i;

	for (i = 0; i < gra_numfaces; i++)
		if (namesame(facename, gra_facelist[i]) == 0) return(i);
#endif
	return(-1);
}

/*
 * Routine to return the number of typefaces used (when "all" is FALSE)
 * or available on the system (when "all" is TRUE)
 * and to return their names in the array "list".
 */
INTBIG screengetfacelist(CHAR ***list, BOOLEAN all)
{
#ifdef TRUETYPE
	REGISTER INTBIG i, total, res;

	if (gra_numfaces == 0)
	{
		total = T1_Get_no_fonts();
		gra_numfaces = total + 1;
		gra_facelist = (CHAR **)emalloc(gra_numfaces * (sizeof (CHAR *)), us_tool->cluster);
		if (gra_facelist == 0) return(0);
		(void)allocstring(&gra_facelist[0], _("DEFAULT FACE"), us_tool->cluster);
		for(i=0; i<total; i++)
		{
			res = T1_LoadFont(i);
			(void)allocstring(&gra_facelist[i+1], (CHAR *)(res == 0 ?
				string2byte(T1_GetFontName(i)) : x_("BROKEN")), us_tool->cluster);
		}
	}
	if (gra_numfaces > VTMAXFACE)
	{
		ttyputerr(_("Warning: found %ld fonts, but can only keep track of %d"), gra_numfaces,
			VTMAXFACE);
		gra_numfaces = VTMAXFACE;
	}
	*list = gra_facelist;
	return(gra_numfaces);
#else
	return(0);
#endif
}
        
/*
 * Routine to return the default typeface used on the screen.
 */
CHAR *screengetdefaultfacename(void)
{
#ifdef TRUETYPE
	if (gra_truetypeon != 0)
	{
		T1_LoadFont(gra_truetypedeffont);
		return(string2byte(T1_GetFontName(gra_truetypedeffont)));
	}
#endif
	/* this isn't right!!! */
	return(x_("Helvetica"));
}

void screensettextinfo(WINDOWPART *win, TECHNOLOGY *tech, UINTBIG *descript)
{
	REGISTER INTBIG face, size;

	size = TDGETSIZE(descript);
	size = truefontsize(size, win, tech);
	face = TDGETFACE(descript);
	gra_settextsize(win, size, face, TDGETITALIC(descript), TDGETBOLD(descript),
		TDGETUNDERLINE(descript), TDGETROTATION(descript));
}

void gra_settextsize(WINDOWPART *win, INTBIG size, INTBIG face, INTBIG italic, INTBIG bold, INTBIG underline, INTBIG rotation)
{
#ifdef TRUETYPE
	GLYPH *glyph;
	CHAR **facelist;
	INTBIG i, newsize, *newdcache, *newacache, *newswidth, hash;
#endif

	gra_texttoosmall = FALSE;
	if (size == TXTMENU)
	{
		gra_curfontnumber = 9;
		size = 10;
	} else if (size == TXTEDITOR)
	{
		gra_curfontnumber = 10;
		size = 12;
	} else
	{
		if (size <= 4) gra_curfontnumber = 0; else
			if (size <= 6) gra_curfontnumber = 1; else
				if (size <= 8) gra_curfontnumber = 2; else
					if (size <= 10) gra_curfontnumber = 3; else
						if (size <= 12) gra_curfontnumber = 4; else
							if (size <= 14) gra_curfontnumber = 5; else
								if (size <= 16) gra_curfontnumber = 6; else
									if (size <= 18) gra_curfontnumber = 7; else
										gra_curfontnumber = 8;
	}
	gra_curfont = gra_font[gra_curfontnumber].font;
	gra_textrotation = rotation;

#ifdef TRUETYPE
	if (gra_truetypeon == 0) return;

	/* remember TrueType settings */
	if (size < 1)
	{
		gra_texttoosmall = TRUE;
		return;
	}
	if (size < MINIMUMTEXTSIZE) size = MINIMUMTEXTSIZE;
	gra_truetypesize = size;
	if (gra_truetypesize > 500) gra_truetypesize = 500;
	gra_truetypefont = gra_truetypedeffont;
	if (face > 0)
	{
		if (gra_numfaces == 0)
			screengetfacelist(&facelist, FALSE);
		if (face < gra_numfaces)
			gra_truetypefont = face-1;
	}
	gra_truetypeitalic = italic;
	gra_truetypebold = bold;
	gra_truetypeunderline = underline;

	/* for unusual fonts, remember ascent/descent in hash table */
	if (italic != 0 || bold != 0 || gra_truetypefont != gra_truetypedeffont)
	{
		if (italic != 0) italic = 1;
		if (bold != 0) bold = 1;
		hash = (((size * 2) + italic) * 5 + bold) * 7 + face;
		hash %= FONTHASHSIZE;
		for(i=0; i<FONTHASHSIZE; i++)
		{
			if (gra_fonthash[hash].descent > 0) break;
			if (gra_fonthash[hash].face == face && gra_fonthash[hash].size == size &&
				gra_fonthash[hash].italic == italic && gra_fonthash[hash].bold == bold)
			{
				gra_truetypeascent = gra_fonthash[hash].ascent;
				gra_truetypedescent = gra_fonthash[hash].descent;
				gra_spacewidth = gra_fonthash[hash].spacewidth;
				return;
			}
			hash++;
			if (hash >= FONTHASHSIZE) hash = 0;
		}
		gra_fonthash[hash].face = face;
		gra_fonthash[hash].size = size;
		gra_fonthash[hash].italic = italic;
		gra_fonthash[hash].bold = bold;
		glyph = T1_SetString(gra_truetypefont,
			b_("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
			0, 0, T1_KERNING, gra_truetypesize, 0);
		if (glyph != 0)
		{
			gra_fonthash[hash].ascent = glyph->metrics.ascent;
			gra_fonthash[hash].descent = glyph->metrics.descent;
			gra_fonthash[hash].spacewidth = (glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing) / 62;
		}
		glyph = T1_SetString(gra_truetypefont, b_("!@#$%^&?~_+-*/=(){}[]<>|\\`'\",.;:"),
			0, 0, T1_KERNING, gra_truetypesize, 0);
		if (glyph != 0)
		{
			if (glyph->metrics.descent < gra_fonthash[hash].descent)
				gra_fonthash[hash].descent = glyph->metrics.descent;
			if (glyph->metrics.ascent > gra_fonthash[hash].ascent)
				gra_fonthash[hash].ascent = glyph->metrics.ascent;
		}
		glyph = T1_SetString(gra_truetypefont, b_("A"), 0, 0, T1_KERNING, gra_truetypesize, 0);
		if (glyph != 0)
		{
			i = glyph->metrics.leftSideBearing;
			glyph = T1_SetString(gra_truetypefont, b_(" A"), 0, 0, T1_KERNING, gra_truetypesize, 0);
			if (glyph != 0)
				gra_fonthash[hash].spacewidth = glyph->metrics.leftSideBearing - i;
		}
		gra_truetypeascent = gra_fonthash[hash].ascent;
		gra_truetypedescent = gra_fonthash[hash].descent;
		gra_spacewidth = gra_fonthash[hash].spacewidth;
	} else
	{
		/* standard font: cache the maximum ascent / descent value for each font size */
		if (gra_truetypesize >= gra_descentcachesize)
		{
			newsize = gra_truetypesize + 1;
			newdcache = (INTBIG *)emalloc(newsize * SIZEOFINTBIG, us_tool->cluster);
			newacache = (INTBIG *)emalloc(newsize * SIZEOFINTBIG, us_tool->cluster);
			newswidth = (INTBIG *)emalloc(newsize * SIZEOFINTBIG, us_tool->cluster);
			for(i=0; i<gra_descentcachesize; i++)
			{
				newdcache[i] = gra_descentcache[i];
				newacache[i] = gra_ascentcache[i];
				newswidth[i] = gra_swidthcache[i];
			}
			for(i=gra_descentcachesize; i<newsize; i++)
			{
				newdcache[i] = 1;
				newacache[i] = -1;
				newswidth[i] = 0;
			}
			if (gra_descentcachesize > 0)
			{
				efree((CHAR *)gra_descentcache);
				efree((CHAR *)gra_ascentcache);
				efree((CHAR *)gra_swidthcache);
			}
			gra_descentcache = newdcache;
			gra_ascentcache = newacache;
			gra_swidthcache = newswidth;
			gra_descentcachesize = newsize;
		}
		if (gra_descentcache[gra_truetypesize] > 0)
		{
			glyph = T1_SetString(gra_truetypefont,
				b_("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
				0, 0, T1_KERNING, gra_truetypesize, 0);
			if (glyph != 0)
			{
				gra_descentcache[gra_truetypesize] = glyph->metrics.descent;
				gra_ascentcache[gra_truetypesize] = glyph->metrics.ascent;
				gra_swidthcache[gra_truetypesize] = (glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing) / 62;
			}
			glyph = T1_SetString(gra_truetypefont, b_("!@#$%^&?~_+-*/=(){}[]<>|\\`'\",.;:"),
				0, 0, T1_KERNING, gra_truetypesize, 0);
			if (glyph != 0)
			{
				if (glyph->metrics.descent < gra_descentcache[gra_truetypesize])
					gra_descentcache[gra_truetypesize] = glyph->metrics.descent;
				if (glyph->metrics.ascent > gra_ascentcache[gra_truetypesize])
					gra_ascentcache[gra_truetypesize] = glyph->metrics.ascent;
			}
			glyph = T1_SetString(gra_truetypefont, b_("A"), 0, 0, T1_KERNING, gra_truetypesize, 0);
			if (glyph != 0)
			{
				i = glyph->metrics.leftSideBearing;
				glyph = T1_SetString(gra_truetypefont, b_(" A"), 0, 0, T1_KERNING, gra_truetypesize, 0);
				if (glyph != 0)
					gra_swidthcache[gra_truetypesize] = glyph->metrics.leftSideBearing - i;
			}
		}
		gra_truetypeascent = gra_ascentcache[gra_truetypesize];
		gra_truetypedescent = gra_descentcache[gra_truetypesize];
		gra_spacewidth = gra_swidthcache[gra_truetypesize];
	}
#endif
}

void screengettextsize(WINDOWPART *win, CHAR *str, INTBIG *x, INTBIG *y)
{
#ifdef ANYDEPTH
	INTBIG thechar, i, width, len;
	INTBIG wid, hei;

	len = estrlen(str);
	if (len == 0 || gra_texttoosmall)
	{
		*x = *y = 0;
		return;
	}

# ifdef TRUETYPE
	if (gra_truetypeon != 0 &&gra_truetypesize > 0 && gra_curfontnumber <= 8)
	{
		if (gra_getT1stringsize(str, &wid, &hei) != 0)
		{
			static INTBIG errorcount = 0;

			efprintf(stderr, x_("Warning: cannot calculate screen size of string '%s' at scale %ld\n"),
				str, gra_truetypesize);
			if (errorcount++ > 10)
			{
				efprintf(stderr, x_("Too many errors, turning off TrueType\n"));
				gra_truetypeon = 0;
			}
		} else
		{
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
			return;
		}
	}
# endif

	width = 0;
	for(i=0; i<len; i++)
	{
		thechar = str[i] & 0377;
		width += gra_font[gra_curfontnumber].width[thechar];
	}
	wid = width;
	hei = gra_font[gra_curfontnumber].height[thechar];
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
#else
	int direction, asc, desc, len;   /* must be "int", and not "INTBIG" */
	XCharStruct xcs;

	len = estrlen(str);
	XTextExtents(gra_curfont, string1byte(str), len, &direction, &asc, &desc, &xcs);
	*x = xcs.width;
	*y = gra_curfont->ascent + gra_curfont->descent;
#endif
}

#ifdef TRUETYPE
/*
 * Routine to calculate the TrueType size of string "str" and return it in "wid" and "hei".
 * Returns nonzero on error.
 */
INTBIG gra_getT1stringsize(CHAR *str, INTBIG *wid, INTBIG *hei)
{
#if 0	/* cannot use "T1_GetStringWidth" because it doesn't handle space width properly */
	*wid = T1_GetStringWidth(gra_truetypefont, str, estrlen(str), 0, 0) * gra_truetypesize / 1000;
#else
	GLYPH *glyph;
	CHAR *startpos, *endpos;
	float slant, extend;
	int modflag, len;
	T1_TMATRIX matrix;

	/* use: gra_truetypeitalic, gra_truetypebold, gra_truetypeunderline */
	modflag = T1_KERNING;
	if (gra_truetypeunderline != 0) modflag |= T1_UNDERLINE;
	*wid = 0;
	for(startpos = str; *startpos != 0; startpos++)
	{
		if (*startpos == ' ')
		{
			*wid += gra_spacewidth;
			continue;
		}
		for(endpos = startpos+1; *endpos != 0; endpos++)
			if (*endpos == ' ') break;
		len = endpos - startpos;
		if (gra_truetypeitalic == 0 && gra_truetypebold == 0)
		{
			glyph = T1_SetString(gra_truetypefont, string1byte(startpos), len, 0,
				modflag, gra_truetypesize, 0);
		} else
		{
			matrix.cxx = matrix.cyy = 1.0;
			matrix.cyx = matrix.cxy = 0.0;
			if (gra_truetypeitalic == 0) slant = 0.0; else slant = TRUETYPEITALICSLANT;
			(void)T1_ShearHMatrix(&matrix, slant);
			if (gra_truetypebold == 0) extend = 1.0; else extend = TRUETYPEBOLDEXTEND;
			(void)T1_ExtendHMatrix(&matrix, extend);
			glyph = T1_SetString(gra_truetypefont, string1byte(startpos), len, 0,
				modflag, gra_truetypesize, &matrix);
		}
		startpos = endpos-1;
		if (glyph == 0) return(1);
		*wid += glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
	}
#endif
	*hei = gra_truetypeascent - gra_truetypedescent;
	if (*hei < gra_truetypesize) *hei = gra_truetypesize;
	return(0);
}
#endif

/*
 * Routine to draw a text on the graphics window
 */
void screendrawtext(WINDOWPART *win, INTBIG atx, INTBIG aty, CHAR *s, GRAPHICS *desc)
{
#ifdef ANYDEPTH
	if (gra_texttoosmall) return;
	gra_drawtext(win, atx, aty, gra_textrotation, s, desc);
#else
	INTBIG l;
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;
	l = estrlen(s);

	/* we have to wait for the window to be visible */

	/* special case for highlight layer: always complement the data */
	if (desc->bits == LAYERH)
	{
		XSetPlaneMask(wf->topdpy, wf->gc, AllPlanes);
		XSetFont(wf->topdpy, wf->gcinv, gra_curfont->fid);
		XDrawString(wf->topdpy, wf->win, wf->gcinv, atx,
			wf->revy - aty - gra_curfont->descent, s, l);
		return;
	}

	gra_setxstate(wf, wf->gc, desc);
	XSetFont(wf->topdpy, wf->gc, gra_curfont->fid);
	XDrawString(wf->topdpy, wf->win, wf->gc, atx,
		wf->revy - aty - gra_curfont->descent, s, l);
#endif
}

/*
 * Routine to convert the string "msg" (to be drawn into window "win") into an
 * array of pixels.  The size of the array is returned in "wid" and "hei", and
 * the pixels are returned in an array of character vectors "rowstart".
 * The routine returns true if the operation cannot be done.
 */
BOOLEAN gettextbits(WINDOWPART *win, CHAR *msg, INTBIG *wid, INTBIG *hei, UCHAR1 ***rowstart)
{
#ifdef ANYDEPTH
	REGISTER INTBIG i, len, thechar, x, y, atx, height, width, datasize;
	REGISTER UCHAR1 *ptr;
	REGISTER UCHAR1 *dest;
# ifdef TRUETYPE
	INTBIG bytesperline, topoffset, trueheight, xpos, xwid;
	REGISTER CHAR *startpos, *endpos;
	GLYPH *glyph;
	float slant, extend;
	int modflag;
	T1_TMATRIX matrix;
# endif

	/* quit if string is null */
	*wid = *hei = 0;
	len = estrlen(msg);
	if (len == 0 || gra_texttoosmall) return(FALSE);

	/* determine the size of the text */
# ifdef TRUETYPE
	if (gra_truetypeon != 0 && gra_truetypesize > 0 && gra_curfontnumber <= 8)
	{
		if (gra_getT1stringsize(msg, wid, hei) != 0) return(TRUE);
	} else
# endif
	{
		for(i=0; i<len; i++)
		{
			thechar = msg[i] & 0377;
			*wid += gra_font[gra_curfontnumber].width[thechar];
			if (gra_font[gra_curfontnumber].height[thechar] > *hei)
				*hei = gra_font[gra_curfontnumber].height[thechar];
		}
	}

	/* allocate space for this */
	datasize = *wid * *hei;
	if (datasize > gra_textbitsdatasize)
	{
		if (gra_textbitsdatasize > 0) efree((CHAR *)gra_textbitsdata);
		gra_textbitsdatasize = 0;

		gra_textbitsdata = (UCHAR1 *)emalloc(datasize, us_tool->cluster);
		if (gra_textbitsdata == 0) return(TRUE);
		gra_textbitsdatasize = datasize;
	}
	if (*hei > gra_textbitsrowstartsize)
	{
		if (gra_textbitsrowstartsize > 0) efree((CHAR *)gra_textbitsrowstart);
		gra_textbitsrowstartsize = 0;
		gra_textbitsrowstart = (UCHAR1 **)emalloc(*hei * (sizeof (UCHAR1 *)), us_tool->cluster);
		if (gra_textbitsrowstart == 0) return(TRUE);
		gra_textbitsrowstartsize = *hei;
	}

	/* load the row start array, clear the data */
	for(y=0; y < *hei; y++)
	{
		gra_textbitsrowstart[y] = &gra_textbitsdata[*wid * y];
		for(x=0; x < *wid; x++)
			gra_textbitsrowstart[y][x] = 0;
	}
	*rowstart = gra_textbitsrowstart;

# ifdef TRUETYPE
	if (gra_truetypeon != 0 && gra_truetypesize > 0 && gra_curfontnumber <= 8)
	{
		/* load the image array */
		modflag = T1_KERNING;
		if (gra_truetypeunderline != 0) modflag |= T1_UNDERLINE;
		xpos = 0;
		for(startpos = msg; *startpos != 0; startpos++)
		{
			if (*startpos == ' ')
			{
				xpos += gra_spacewidth;
				continue;
			}
			for(endpos = startpos+1; *endpos != 0; endpos++)
				if (*endpos == ' ') break;
			len = endpos - startpos;
			if (gra_truetypeitalic == 0 && gra_truetypebold == 0)
			{
				glyph = T1_SetString(gra_truetypefont, string1byte(startpos), len, 0,
					modflag, gra_truetypesize, 0);
			} else
			{
				matrix.cxx = matrix.cyy = 1.0;
				matrix.cyx = matrix.cxy = 0.0;
				if (gra_truetypeitalic == 0) slant = 0.0; else slant = TRUETYPEITALICSLANT;
				(void)T1_ShearHMatrix(&matrix, slant);
				if (gra_truetypebold == 0) extend = 1.0; else extend = TRUETYPEBOLDEXTEND;
				(void)T1_ExtendHMatrix(&matrix, extend);
				glyph = T1_SetString(gra_truetypefont, string1byte(startpos), len, 0,
					modflag, gra_truetypesize, &matrix);
			}
			if (glyph == 0) return(TRUE);
			startpos = endpos-1;

			xwid = glyph->metrics.rightSideBearing - glyph->metrics.leftSideBearing;
			*hei = gra_truetypeascent - gra_truetypedescent;
			topoffset = gra_truetypeascent - glyph->metrics.ascent;
			trueheight = glyph->metrics.ascent - glyph->metrics.descent;

			bytesperline = (xwid + 15) / 16 * 2;
			ptr = (UCHAR1 *)glyph->bits;
			if (ptr != 0)
			{
				for(y = 0; y < trueheight; y++)
				{
					dest = gra_textbitsrowstart[y+topoffset];
					for(x = 0; x < xwid; x++)
					{
						i = ptr[x>>3] & (1 << (x&7));
						if (i != 0) dest[x+xpos] = 1;
					}
					ptr += bytesperline;
				}
			}
			xpos += xwid;
		}
	} else
# endif
	{
		/* load the image array */
		atx = 0;
		for(i=0; i<len; i++)
		{
			thechar = msg[i] & 0377;
			width = gra_font[gra_curfontnumber].width[thechar];
			height = gra_font[gra_curfontnumber].height[thechar];

			ptr = gra_font[gra_curfontnumber].data[thechar];
			for(y=0; y<height; y++)
			{
				dest = gra_textbitsrowstart[y];
				for(x=0; x<width; x++)
				{
					if (*ptr++ != 0) dest[atx + x] = 1;
				}
			}
			atx += width;
		}
	}
	return(FALSE);
#else
	return(TRUE);
#endif
}

/******************** CIRCLE DRAWING ********************/

/*
 * Routine to draw a circle on the graphics window
 */
void screendrawcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawcircle(win, atx, aty, radius, desc);
#else
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;
	gra_setxstate(wf, wf->gc, desc);
	XDrawArc(wf->topdpy, wf->win, wf->gc, atx - radius, wf->revy - aty - radius,
		radius*2, radius*2, 0, 360*64);
#endif
}

/*
 * Routine to draw a thick circle on the graphics window
 */
void screendrawthickcircle(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius,
	GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawthickcircle(win, atx, aty, radius, desc);
#else
	screendrawcircle(win, atx, aty, radius, desc);
#endif
}

/******************** DISC DRAWING ********************/

/*
 * routine to draw a filled-in circle of radius "radius"
 */
void screendrawdisc(WINDOWPART *win, INTBIG atx, INTBIG aty, INTBIG radius, GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawdisc(win, atx, aty, radius, desc);
#else
	REGISTER WINDOWFRAME *wf;

	wf = win->frame;
	if ((desc->colstyle & NATURE) == PATTERNED && desc->col != 0)
	{
		gra_loadstipple(wf, desc->raster);
		gra_setxstate(wf, wf->gcstip, desc);
		XFillArc(wf->topdpy, wf->win, wf->gcstip, atx - radius,
			wf->revy - aty - radius, radius*2, radius*2, 0, 360*64);
	} else
	{
		gra_setxstate(wf, wf->gc, desc);
		XFillArc(wf->topdpy, wf->win, wf->gc, atx - radius, wf->revy - aty - radius,
			radius*2, radius*2, 0, 360*64);
	}
#endif
}

/******************** ARC DRAWING ********************/

/*
 * draws a thin arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG x1, INTBIG y1,
	INTBIG x2, INTBIG y2, GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawcirclearc(win, centerx, centery, x1, y1, x2, y2, FALSE, desc);
#else
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG radius, startangle, endangle;

	wf = win->frame;
	gra_setxstate(wf, wf->gc, desc);
	radius = computedistance(centerx, wf->revy - centery, x1,
		wf->revy - y1);
	startangle = figureangle(centerx, centery, x1, y1);
	endangle = figureangle(centerx, centery, x2, y2);
	if (startangle < endangle) startangle += 3600;
	XDrawArc(wf->topdpy, wf->win, wf->gc, centerx - radius,
		wf->revy - centery - radius, radius*2, radius*2, (endangle*64 + 5) / 10,
		((startangle - endangle)*64 + 5) / 10);
#endif
}

/*
 * draws a thick arc centered at (centerx, centery), clockwise,
 * passing by (x1,y1) and (x2,y2)
 */
void screendrawthickcirclearc(WINDOWPART *win, INTBIG centerx, INTBIG centery, INTBIG p1_x, INTBIG p1_y,
	INTBIG p2_x, INTBIG p2_y, GRAPHICS *desc)
{
#ifdef ANYDEPTH
	gra_drawcirclearc(win, centerx, centery, p1_x, p1_y, p2_x, p2_y, TRUE, desc);
#else
	screendrawcirclearc(win, centerx, centery, p1_x, p1_y, p2_x, p2_y, desc);
#endif
}

/******************** GRID CONTROL ********************/

/*
 * fast grid drawing routine
 */
void screendrawgrid(WINDOWPART *win, POLYGON *obj)
{
#ifdef ANYDEPTH
	gra_drawgrid(win, obj);
#else
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG i, gridsize;
	REGISTER INTBIG x, y;
	static Pixmap grid_pix;
	static GC grid_gc;
	XGCValues gcv;
	static INTBIG bestsize = 0;

	/* grid pixrect */
	wf = win->frame;
	gridsize = obj->xv[3] - obj->xv[2];

	/* for some reason, must always reallocate SEEMS OK NOW: SRP */
	if (wf->swid > bestsize)
	{
		if (bestsize != 0)
		{
			XFreePixmap(wf->topdpy, grid_pix);
			XFreeGC(wf->topdpy, grid_gc);
		}
		grid_pix = XCreatePixmap(wf->topdpy, wf->win, bestsize = wf->swid, 1, 1);
		gcv.background = 0;
		gcv.foreground = 0;
		grid_gc = XCreateGC(wf->topdpy, (Drawable)grid_pix, GCBackground | GCForeground, &gcv);
	}

	XSetForeground(wf->topdpy, grid_gc, 0);
	XFillRectangle(wf->topdpy, (Drawable)grid_pix, grid_gc, 0, 0, bestsize, 1);

	/* calculate the horizontal dots position */
	XSetForeground(wf->topdpy, grid_gc, 1);
	for(i = obj->xv[1]; i < obj->xv[5]; i += obj->xv[0])
	{
		x = muldiv(i - obj->xv[4], gridsize, obj->xv[5] - obj->xv[4]) + obj->xv[2];
		XDrawPoint(wf->topdpy, (Drawable)grid_pix, grid_gc, x, 0);
	}

	/* draw the vertical columns */
	XSetStipple(wf->topdpy, wf->gcstip, grid_pix);
	gra_setxstate(wf, wf->gcstip, obj->desc);
	for(i = obj->yv[1]; i < obj->yv[5]; i += obj->yv[0])
	{
		y = muldiv(i - obj->yv[4], obj->yv[3] - obj->yv[2], obj->yv[5] - obj->yv[4]) +
			obj->yv[2];
		XFillRectangle(wf->topdpy, wf->win, wf->gcstip, obj->xv[2],
			wf->revy - y, gridsize, 1);
	}
#endif
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
 * routine to convert from "gra_inputstate" (the typical input parameter)
 * to button numbers (the table "gra_buttonname")
 */
INTBIG gra_makebutton(INTBIG state)
{
	REGISTER INTBIG base;

	switch (state&ISBUTTON)
	{
		case ISLEFT:    base = 0;   break;
		case ISMIDDLE:  base = 1;   break;
		case ISRIGHT:   base = 2;   break;
		case ISWHLFWD:  base = 3;   break;
		case ISWHLBKW:  base = 4;   break;
	}

	if ((state&DOUBLECL) != 0) return(base+REALBUTS*8);
	switch (state & (SHIFTISDOWN|CONTROLISDOWN|METAISDOWN))
	{
		case SHIFTISDOWN                         : return(base+REALBUTS*1);
		case             CONTROLISDOWN           : return(base+REALBUTS*2);
		case                           METAISDOWN: return(base+REALBUTS*3);
		case SHIFTISDOWN|CONTROLISDOWN           : return(base+REALBUTS*4);
		case SHIFTISDOWN|              METAISDOWN: return(base+REALBUTS*5);
		case             CONTROLISDOWN|METAISDOWN: return(base+REALBUTS*6);
		case SHIFTISDOWN|CONTROLISDOWN|METAISDOWN: return(base+REALBUTS*7);
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
	/* now the first button wait has happened: graphics is inited */
	gra_firstbuttonwait = 1;

	/* if there is a button push pending, return it */
	if ((gra_inputstate != NOEVENT) && ((gra_inputstate & ISBUTTON) != 0) &&
		((gra_inputstate & BUTTONUP) == 0))
	{
		*but = gra_makebutton(gra_inputstate);
		*x = gra_cursorx;
		*y = gra_cursory;
		gra_inputstate = NOEVENT;
		if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
		return;
	}

	/* get some input (keyboard or mouse) */
	gra_nextevent();

	/* if a button push was read, return it */
	if ((gra_inputstate != NOEVENT) && ((gra_inputstate & ISBUTTON) != 0) &&
		((gra_inputstate & BUTTONUP) == 0))
	{
		*but = gra_makebutton(gra_inputstate);
		*x = gra_cursorx;
		*y = gra_cursory;
		gra_inputstate = NOEVENT;
		if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
		return;
	}

	/* no button input yet */
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
 * like during dragging: 0 for normal (the X cursor), 1 for drawing (a pen),
 * 2 for dragging (a hand), 3 for popup menu selection (a horizontal arrow), 4 for
 * hierarchical popup menu selection (arrow, stays at end).
 */
void trackcursor(BOOLEAN waitforpush, BOOLEAN (*whileup)(INTBIG, INTBIG),
	void (*whendown)(void), BOOLEAN (*eachdown)(INTBIG, INTBIG),
	BOOLEAN (*eachchar)(INTBIG, INTBIG, INTSML), void (*done)(void), INTBIG purpose)
{
	REGISTER BOOLEAN keepon;
	REGISTER INTBIG oldcursor, action;

	/* change the cursor to an appropriate icon */
	oldcursor = us_normalcursor;
	switch (purpose)
	{
		case TRACKDRAWING:    us_normalcursor = PENCURSOR;    break;
		case TRACKDRAGGING:   us_normalcursor = HANDCURSOR;   break;
		case TRACKSELECTING:
		case TRACKHSELECTING: us_normalcursor = MENUCURSOR;   break;
	}
	setdefaultcursortype(us_normalcursor);

	/* now wait for a button to go down, if requested */
	keepon = FALSE;
	gra_tracking = TRUE;
	if (waitforpush)
	{
		while (!keepon)
		{
			gra_nextevent();
			if (gra_inputstate == NOEVENT) continue;
			action = gra_inputstate;
			gra_inputstate = NOEVENT;

			/* if button just went down, stop this loop */
			if ((action&ISBUTTON) != 0 && (action & BUTTONUP) == 0) break;
			if ((action&MOTION) != 0)
			{
				keepon = (*whileup)(gra_cursorx, gra_cursory);
			} else if ((action&ISKEYSTROKE) != 0 && gra_inputspecial == 0)
			{
				keepon = (*eachchar)(gra_cursorx, gra_cursory, (INTSML)(action&CHARREAD));
			}
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
		us_endchanges(NOWINDOWPART);
		gra_nextevent();

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
	gra_tracking = FALSE;
	us_normalcursor = oldcursor;
	setdefaultcursortype(us_normalcursor);
}

/*
 * routine to read the current co-ordinates of the tablet and return them in "*x" and "*y"
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
	if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
}

/******************** KEYBOARD CONTROL ********************/

/*
 * routine to get the next character from the keyboard
 */
INTSML getnxtchar(INTBIG *special)
{
	REGISTER INTSML i;

	if ((gra_inputstate != NOEVENT) && ((gra_inputstate & ISKEYSTROKE) != 0))
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
		gra_nextevent();
		if ((gra_inputstate != NOEVENT) && (gra_inputstate & ISKEYSTROKE) != 0)
			break;
	}
	i = gra_inputstate & CHARREAD;
	*special = gra_inputspecial;
	gra_inputstate = NOEVENT;
	if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
	return(i);
}

/* not required with X11 */
void checkforinterrupt(void)
{
	XtInputMask mask;
	XEvent event;

	gra_checkinginterrupt = TRUE;
	for(;;)
	{
		mask = XtAppPending(gra_xtapp);
		if ((mask&XtIMXEvent) != 0)
		{
			XtAppNextEvent(gra_xtapp, &event);
			XtDispatchEvent(&event);
			continue;
		}
		break;
	}
	gra_checkinginterrupt = FALSE;
	setdefaultcursortype(NULLCURSOR);
}

#define BIT(c, x)   (c[x>>3] & (1<<(x&7)))

/*
 * Routine to return which "bucky bits" are held down (shift, control, etc.)
 */
INTBIG getbuckybits(void)
{
	REGISTER INTBIG bits, width, i;
	CHAR1 keys[32];
	static XModifierKeymap *mmap;
	static INTBIG first = 1;
	KeyCode code;

	if (first != 0)
	{
		first = 0;
		mmap = XGetModifierMapping(gra_maindpy);
	}

	bits = 0;

	XQueryKeymap(gra_maindpy, keys);
	width = mmap->max_keypermod;
	for (i=0; i<width; i++)
	{
		code = mmap->modifiermap[ControlMapIndex*width+i];
		if (code != 0 && BIT(keys, code) != 0) bits |= CONTROLDOWN | ACCELERATORDOWN;

		code = mmap->modifiermap[ShiftMapIndex*width+i];
		if (code != 0 && BIT(keys, code) != 0) bits |= SHIFTDOWN;

		code = mmap->modifiermap[Mod1MapIndex*width+i];
		if (code != 0 && BIT(keys, code) != 0) bits |= OPTALTMETDOWN;
	}

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
		if ((gra_inputstate & ISKEYSTROKE) != 0) return(TRUE);
		return(FALSE);
	}

	/* wait for something and analyze it */
	if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
	gra_nextevent();
	if ((gra_inputstate != NOEVENT) && (gra_inputstate & ISKEYSTROKE) != 0) return(TRUE);
	return(FALSE);
}

/****************************** FILES ******************************/

BOOLEAN gra_topoffile(CHAR **a)
{
	CHAR *b, **c;
	BOOLEAN retval;

	b = gra_curpath;
	c = &b;
	retval = topoffile(c);
	requiredextension(gra_requiredextension);
	return(retval);
}

void gra_fileselectcancel(Widget w, XtPointer client_data,
	XmSelectionBoxCallbackStruct *call_data)
{
	gra_curpath[0] = 0;
	gra_fileselectdone = TRUE;
}

void gra_fileselectok(Widget w, XtPointer client_data,
	XmSelectionBoxCallbackStruct *call_data)
{
	CHAR1 *filename;

	gra_curpath[0] = 0;
	if (XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET,
		&filename))
	{
		estrcpy(gra_curpath, string2byte(filename));
		XtFree(filename);
	}
	gra_fileselectdone = TRUE;
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

DIALOGITEM gra_fileindialogitems[] =   /* File Input */
{
 /*  1 */ {0, {128,256,152,336}, BUTTON, N_("Open")},
 /*  2 */ {0, {176,256,200,336}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {40,8,216,240}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,40,336}, MESSAGE, x_("")},
 /*  5 */ {0, {80,256,104,336}, BUTTON, N_("Up")}
};
DIALOG gra_fileindialog = {{50,75,278,421}, 0, 0, 5, gra_fileindialogitems, 0, 0};

/* special items for the file input dialog */
#define DFIN_FILELIST    3		/* File list (scroll) */
#define DFIN_FILENAME    4		/* File name (stat text) */
#define DFIN_UPBUTTON    5		/* Up (button) */

DIALOGITEM gra_fileoutdialogitems[] =   /* File Output */
{
 /*  1 */ {0, {146,256,170,336}, BUTTON, N_("OK")},
 /*  2 */ {0, {194,256,218,336}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {50,8,218,240}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,40,336}, MESSAGE, x_("")},
 /*  5 */ {0, {50,256,74,336}, BUTTON, N_("Up")},
 /*  6 */ {0, {98,256,122,336}, BUTTON, N_("Down")},
 /*  7 */ {0, {226,8,242,336}, EDITTEXT, x_("")}
};
DIALOG gra_fileoutdialog = {{50,75,301,421}, 0, 0, 7, gra_fileoutdialogitems, 0, 0};

/* special items for the file output dialog */
#define DFOU_FILELIST    3		/* File list (scroll) */
#define DFOU_DIRNAME     4		/* Directory name (stat text) */
#define DFOU_UPBUTTON    5		/* Up (button) */
#define DFOU_DOWNBUTTON  6		/* Down (button) */
#define DFOU_FILENAME    7		/* File name (edit text) */

/*
 * Routine to display a standard file prompt dialog and return the selected file.
 * The prompt message is in "msg" and the kind of file is in "filetype".  The default
 * output file name is in "defofile" (only used if "filetype" is for output files).
 */
CHAR *fileselect(CHAR *msg, INTBIG filetype, CHAR *defofile)
{
	INTBIG i, l, count, mactype, filestatus;
	BOOLEAN binary;
	CHAR *extension, *winfilter, *shortname, *longname, directory[256], fullmsg[300], *pars[5]; 
	extern COMCOMP us_yesnop;
	XmString mprompt, mfilter, mtitle, mpattern, mdirectory;
	Widget w, child;
	INTBIG wid, hei, ac, screenw, screenh;
	Arg arg[10];
	XEvent event;
	CHAR filename[256], temp[256], *childmsg;
	REGISTER WINDOWFRAME *wf;

	if (us_logplay != NULL)
	{
		if (gra_loggetnextaction(gra_curpath)) return(0);
	} else
	{
		if ((filetype&FILETYPEWRITE) == 0)
		{
			/* input file selection: display the file input dialog box */
			describefiletype(filetype, &extension, &winfilter, &mactype, &binary, &shortname, &longname);
			esnprintf(temp, 256, _("%s Selection"), msg);
			mtitle = XmStringCreateLocalized(string1byte(temp));
			mprompt = XmStringCreateLocalized(string1byte(msg));
			estrcpy(temp, winfilter);
			for(i=0; temp[i] != 0; i++) if (temp[i] == ';') break;
			temp[i] = 0;
			mfilter = XmStringCreateLocalized(string1byte(temp));
			wf = el_curwindowframe;
			if (wf == NOWINDOWFRAME) wf = el_firstwindowframe;
			screenw = wf->wd->wid;
			screenh = wf->wd->hei;
			wid = 400;   if (wid > screenw) wid = screenw;
			hei = 480;   if (hei > screenh) hei = screenh;
			estrcpy(directory, currentdirectory());
			mdirectory = XmStringCreateLocalized(string1byte(directory));
			ac = 0;
			XtSetArg(arg[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);   ac++;
			XtSetArg(arg[ac], XmNdirMask, mfilter);   ac++;
			XtSetArg(arg[ac], XmNselectionLabelString, mprompt);   ac++;
			XtSetArg(arg[ac], XmNwidth, 400);   ac++;
			XtSetArg(arg[ac], XmNheight, 480);   ac++;
			XtSetArg(arg[ac], XmNdialogTitle, mtitle);   ac++;
			XtSetArg(arg[ac], XmNdirectory, mdirectory);   ac++;
			estrcpy(gra_localstring, _("File Selection"));
			w = XmCreateFileSelectionDialog(wf->graphicswidget, string1byte(gra_localstring), arg, ac);
			XtAddCallback(w, XmNcancelCallback, (XtCallbackProc)gra_fileselectcancel, NULL);
			XtAddCallback(w, XmNokCallback, (XtCallbackProc)gra_fileselectok, NULL);
			XtManageChild(w);
			XmStringFree(mprompt);
			XmStringFree(mfilter);
			XmStringFree(mtitle);

			gra_fileselectdone = FALSE;
			while (!gra_fileselectdone)
			{
				XmUpdateDisplay(wf->toplevelwidget);
				XtAppNextEvent(gra_xtapp, &event);
				XtDispatchEvent(&event);
			}
			XtUnmanageChild(w);
			XtDestroyWidget(w);

			/* set current directory from file path */
			l = estrlen(gra_curpath);
			for(i=l-1; i>0; i--) if (gra_curpath[i] == DIRSEP) break;
			if (i > 0)
			{
				i++;
				(void)estrcpy(directory, gra_curpath);
				directory[i] = 0;
				gra_setcurrentdirectory(directory);
			}
		} else
		{
			/* determine the default directory and file name */
			estrcpy(directory, currentdirectory());
			filename[0] = 0;
			if (defofile[0] != 0)
			{
				l = estrlen(defofile);
				for(i=l-1; i>0; i--) if (defofile[i] == DIRSEP) break;
				if (i > 0)
				{
					i++;
					(void)estrcpy(directory, defofile);
					directory[i] = 0;
				}
				estrcpy(filename, &defofile[i]);
			}

			describefiletype(filetype, &extension, &winfilter, &mactype, &binary, &shortname, &longname);
			wf = el_curwindowframe;
			if (wf == NOWINDOWFRAME) wf = el_firstwindowframe;
			screenw = wf->wd->wid;
			screenh = wf->wd->hei;
			wid = 400;   if (wid > screenw) wid = screenw;
			hei = 480;   if (hei > screenh) hei = screenh;
			mprompt = XmStringCreateLocalized(string1byte(msg));
			mdirectory = XmStringCreateLocalized(string1byte(directory));
			estrcpy(temp, winfilter);
			for(i=0; temp[i] != 0; i++) if (temp[i] == ';') break;
			temp[i] = 0;
			mpattern = XmStringCreateLocalized(string1byte(temp));
			esnprintf(temp, 256, _("%s Creation"), msg);
			mtitle = XmStringCreateLocalized(string1byte(temp));
			ac = 0;
			XtSetArg(arg[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);   ac++;
			XtSetArg(arg[ac], XmNwidth, 400);   ac++;
			XtSetArg(arg[ac], XmNheight, 480);   ac++;
			XtSetArg(arg[ac], XmNselectionLabelString, mprompt);   ac++;
			XtSetArg(arg[ac], XmNdirectory, mdirectory);   ac++;
			XtSetArg(arg[ac], XmNpattern, mpattern);   ac++;
			XtSetArg(arg[ac], XmNdialogTitle, mtitle);   ac++;
			w = XmCreateFileSelectionDialog(wf->graphicswidget, b_(""), arg, ac);
			XtAddCallback(w, XmNcancelCallback, (XtCallbackProc)gra_fileselectcancel, NULL);
			XtAddCallback(w, XmNokCallback, (XtCallbackProc)gra_fileselectok, NULL);
			XtManageChild(w);
			XmStringFree(mprompt);
			XmStringFree(mdirectory);
			XmStringFree(mpattern);
			XmStringFree(mtitle);

			/* append the default output file name to the selection path */
			child = XmFileSelectionBoxGetChild(w, XmDIALOG_TEXT);
			childmsg = string2byte(XmTextGetString(child));
			esnprintf(temp, 256, x_("%s%s"), childmsg, filename);
			XmTextSetString(child, string1byte(temp));

			gra_fileselectdone = FALSE;
			while (!gra_fileselectdone)
			{
				XmUpdateDisplay(wf->toplevelwidget);
				XtAppNextEvent(gra_xtapp, &event);
				XtDispatchEvent(&event);
			}
			XtUnmanageChild(w);
			XtDestroyWidget(w);

			/* set current directory from file path */
			l = estrlen(gra_curpath);
			for(i=l-1; i>0; i--) if (gra_curpath[i] == DIRSEP) break;
			if (i > 0)
			{
				i++;
				(void)estrcpy(directory, gra_curpath);
				directory[i] = 0;
				gra_setcurrentdirectory(directory);
			}
		}
	}

	/* log this result */
	gra_logwriteaction(FILEREPLY, 0, 0, 0, gra_curpath);

	/* if writing a file, check for overwrite */
	if ((filetype&FILETYPEWRITE) != 0)
	{
		/* give warning if creating file that already exists */
		if (gra_curpath[0] != 0)
		{
			filestatus = fileexistence(gra_curpath);
			switch (filestatus)
			{
				case 1:		/* an existing file */
					esnprintf(fullmsg, 300, _("File '%s' exists.  Overwrite? "), gra_curpath);
					count = ttygetparam(fullmsg, &us_yesnop, 2, pars);
					if (count > 0 && namesamen(pars[0], x_("no"), estrlen(pars[0])) == 0)
						gra_curpath[0] = 0;
					break;
				case 2:		/* a directory */
					DiaMessageInDialog(_("'%s' is a directory: cannot write it"), gra_curpath);
					gra_curpath[0] = 0;
					break;
				case 3:		/* a read-only file */
					DiaMessageInDialog(_("'%s' is read-only: cannot overwrite it"), gra_curpath);
					gra_curpath[0] = 0;
					break;
			}
		}
	} else
	{
		/* if a file was selected, make sure it isn't a directory */
		if (gra_curpath[0] != 0)
		{
			if (fileexistence(gra_curpath) == 2) gra_curpath[0] = 0;
		}
	}

	if (gra_curpath[0] == 0) return((CHAR *)NULL); else
	{
		return(gra_curpath);
	}
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

int gra_fileselectall(const struct dirent *a)
{
	REGISTER CHAR *pt;

	pt = (CHAR *)a->d_name;
	if (estrcmp(pt, x_("..")) == 0) return(0);
	if (estrcmp(pt, x_(".")) == 0) return(0);
	return(1);
}

/*
 * Routine to search for all of the files/directories in directory "directory" and
 * return them in the array of strings "filelist".  Returns the number of files found.
 */
INTBIG filesindirectory(CHAR *directory, CHAR ***filelist)
{
	INTBIG len;
	CHAR localname[256];
	struct dirent **filenamelist;
	struct dirent *dir, *result;
	INTBIG total, i;
#ifndef HAVE_SCANDIR
#  define DIRFILECOUNT 2048  /* max number of entries in a directory */
#  ifndef _POSIX_PATH_MAX
#    define _POSIX_PATH_MAX 255
#  endif
	DIR *dirp;
	REGISTER INTBIG tdirsize;
#endif

	/* make sure there is no ending "/" */
	estrcpy(localname, directory);
	i = estrlen(localname) - 1;
	if (i > 0 && localname[i] == '/') localname[i] = 0;
	if (i == -1) esnprintf(localname, 256, x_("."));

	/* scan the directory */
#ifdef HAVE_SCANDIR
	/*
	 * On BSD, last arg is: "(int (*)(const void *, const void *))0"
	 * On non-BSD, last arg is: "(int (*)(const __ptr_t, const __ptr_t))0"
	 */
	total = scandir(string1byte(localname), &filenamelist, gra_fileselectall, NULL);
#else
	/* get space for directory */
	filenamelist = (struct dirent **)calloc(DIRFILECOUNT,
		sizeof(struct dirent *));
	if (filenamelist == 0) return(0);
	tdirsize = sizeof (struct dirent);
	dir = (struct dirent *)emalloc(tdirsize + _POSIX_PATH_MAX, us_tool->cluster);
	if (dir == 0) return(0);

	/* get the directory */
	dirp = opendir(string1byte(localname));
	if (dirp == NULL) return(-1);

	/* construct the list */
	total = 0;
	for(;;)
	{
#ifdef HAVE_POSIX_READDIR_R
		if (readdir_r(dirp, dir, &result) != 0) break;
#else
		result = (struct dirent *)readdir_r(dirp, dir);
#endif
		if (result == NULL) break;
		if (gra_fileselectall(dir))
		{
			filenamelist[total] = (struct dirent *)emalloc(tdirsize + _POSIX_PATH_MAX,
				us_tool->cluster);
			if (filenamelist[total] == 0) { closedir(dirp);   return(0); }
			memcpy(filenamelist[total], dir, sizeof(dir)+_POSIX_PATH_MAX);
			total++;
		}
	}
	closedir(dirp);
#endif

	gra_initfilelist();
	for(i=0; i<total; i++)
	{
		dir = filenamelist[i];
		if (gra_addfiletolist(string2byte(dir->d_name))) return(0);
		free((CHAR *)dir);
	}
	free((CHAR *)filenamelist);
	*filelist = getstringarray(gra_fileliststringarray, &len);
	return(len);
}

/* routine to convert a path name with "~" to a real path */
CHAR *truepath(CHAR *line)
{
	static CHAR outline[100], home[50];
	REGISTER CHAR save, *ch;
	static BOOLEAN gothome = FALSE;
	struct passwd *tmp;

	if (line[0] != '~') return(line);

	/* if it is the form "~username", figure it out */
	if (line[1] != '/')
	{
		for(ch = &line[1]; *ch != 0 && *ch != '/'; ch++);
		save = *ch;   *ch = 0;
		tmp = getpwnam(string1byte(&line[1]));
		if (tmp == 0) return(line);
		*ch = save;
		(void)estrcpy(outline, string2byte(tmp->pw_dir));
		(void)estrcat(outline, ch);
		return(outline);
	}

	/* get the user's home directory once only */
	if (!gothome)
	{
		/* get name of user */
		tmp = getpwuid(getuid());
		if (tmp == 0) return(line);
		(void)estrcpy(home, string2byte(tmp->pw_dir));
		gothome = TRUE;
	}

	/* substitute home directory for the "~" */
	(void)estrcpy(outline, home);
	(void)estrcat(outline, &line[1]);
	return(outline);
}

/*
 * Routine to return the full path to file "file".
 */
CHAR *fullfilename(CHAR *file)
{
	static CHAR fullfile[MAXPATHLEN];

	if (file[0] == '/') return(file);
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
	CHAR1 file1[300], newfile1[300];

	TRUESTRCPY(file1, string1byte(file));
	TRUESTRCPY(newfile1, string1byte(newfile));
	if (link(file1, newfile1) < 0) return(1);
	if (unlink(file1) < 0) return(1);
	return(0);
}

/*
 * routine to delete file "file"
 */
INTBIG eunlink(CHAR *file)
{
	return(unlink(string1byte(file)));
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
	CHAR cmd[256];

	esnprintf(cmd, 256, x_("mkdir %s"), dirname);
	if (system(string1byte(cmd)) == 0) return(FALSE);
	return(TRUE);
}

/*
 * Routine to return the current directory name
 */
CHAR *currentdirectory(void)
{
	static CHAR line[MAXPATHLEN];
	REGISTER INTBIG len;

#ifdef HAVE_GETCWD
	(void)egetcwd(line, MAXPATHLEN);
#else
#  ifdef HAVE_GETWD
	(void)getwd(line);
#  else
	line[0] = 0;
#  endif
#endif
	len = estrlen(line);
	if (len > 0 && line[len-1] != DIRSEP)
		estrcat(line, DIRSEPSTR);
	return(line);
}

/*
 * Routine to set the current directory name
 */
void gra_setcurrentdirectory(CHAR *path)
{
	chdir(string1byte(path));
}

/*
 * Routine to return the home directory (returns 0 if it doesn't exist)
 */
CHAR *hashomedir(void)
{
	return(x_("~/"));
}

/*
 * Routine to return the path to the "options" library.
 */
CHAR *optionsfilepath(void)
{
	return(x_("~/.electricoptions.elib"));
}

/*
 * Routine to obtain the modification date on file "filename".
 */
time_t filedate(CHAR *filename)
{
	struct stat buf;
	time_t thetime;

	estat(filename, &buf);
	thetime = buf.st_mtime;
	thetime -= machinetimeoffset();
	return(thetime);
}

/*
 * Routine to set the date on file "filename" to "date".
 * If "moddate" is TRUE, set the modification date.  Otherwise, the creation date.
 */
void setfiledate(CHAR *filename, time_t date, BOOLEAN moddate)
{
}

/*
 * Routine to lock a resource called "lockfilename" by creating such a file
 * if it doesn't exist.  Returns true if successful, false if unable to
 * lock the file.
 */
BOOLEAN lockfile(CHAR *lockfilename)
{
	INTBIG fd;

	fd = ecreat(lockfilename, 0);
	if (fd == -1 && errno == EACCES) return(FALSE);
	if (fd == -1 || close(fd) == -1) return(FALSE);
	return(TRUE);
}

/*
 * Routine to unlock a resource called "lockfilename" by deleting such a file.
 */
void unlockfile(CHAR *lockfilename)
{
	if (unlink(string1byte(lockfilename)) == -1)
		ttyputerr(_("Error unlocking %s"), lockfilename);
}

/*
 * Routine to show file "filename" in a browser window.
 * Returns true if the operation cannot be done.
 */
BOOLEAN browsefile(CHAR *filename)
{
	CHAR line[300];

	esnprintf(line, 300, x_("netscape %s&"), filename);
	if (system(string1byte(line)) == 0) return(FALSE);
	return(TRUE);
}

/****************************** CHANNELS ******************************/

/*
 * routine to create a pipe connection between the channels in "channels"
 */
INTBIG epipe(int channels[2])
{
	return(pipe(channels));
}

/*
 * Routine to set channel "channel" into an appropriate mode for single-character
 * interaction (i.e. break mode).
 */
void setinteractivemode(int channel)
{
#ifdef HAVE_TERMIOS_H
	struct termios sbuf;

	tcgetattr(channel, &sbuf);
	sbuf.c_iflag |= (BRKINT|IGNPAR|ISTRIP|IXON);
	sbuf.c_oflag |= OPOST;
	sbuf.c_lflag |= (ICANON|ISIG|ECHO);
	sbuf.c_cc[VMIN] = 1;	/* Input should wait for at least one CHAR */
	sbuf.c_cc[VTIME] = 0;	/* no matter how long that takes. */
	tcsetattr(channel, TCSANOW, &sbuf);
#else
#  ifdef HAVE_TERMIO_H
	struct termio sbuf;

	(void)ioctl(channel, TCGETA, &sbuf);
	sbuf.c_iflag |= (BRKINT|IGNPAR|ISTRIP|IXON);
	sbuf.c_oflag |= OPOST;

	/* bogus hp baud rate sanity */
	sbuf.c_cflag = (sbuf.c_cflag & ~CBAUD) | B9600;
	sbuf.c_lflag |= (ICANON|ISIG|ECHO);
	sbuf.c_cc[VMIN] = 1;	/* Input should wait for at least one CHAR */
	sbuf.c_cc[VTIME] = 0;	/* no matter how long that takes. */
	(void)ioctl(channel, TCSETA, &sbuf);
#  else
	struct sgttyb sbuf;

	(void)gtty(channel, &sbuf);
	sbuf.sg_flags |= CBREAK;
	(void)stty(channel, &sbuf);
#  endif
#endif
}

/*
 * Routine to replace channel "channel" with a pointer to file "file".
 * Returns a pointer to the original channel.
 */
INTBIG channelreplacewithfile(int channel, CHAR *file)
{
	INTBIG save;

	save = dup(channel);
	(void)close(channel);
	(void)open(string1byte(file), 1);
	return(save);
}

/*
 * Routine to replace channel "channel" with new channel "newchannel".
 * Returns a pointer to the original channel.
 */
INTBIG channelreplacewithchannel(int channel, int newchannel)
{
	INTBIG save;

	save = dup(channel);
	(void)close(channel);
	(void)dup(newchannel);
	return(save);
}

/*
 * Routine to restore channel "channel" to the pointer that was returned
 * by "channelreplacewithfile" or "channelreplacewithchannel" (and is in "saved").
 */
void channelrestore(int channel, INTBIG saved)
{
	(void)close(channel);
	(void)dup(saved);
}

/*
 * Routine to read "count" bytes from channel "channel" into "addr".
 * Returns the number of bytes read.
 */
INTBIG eread(int channel, UCHAR1 *addr, INTBIG count)
{
	return(read(channel, addr, count));
}

/*
 * Routine to write "count" bytes to channel "channel" from "addr".
 * Returns the number of bytes written.
 */
INTBIG ewrite(int channel, UCHAR1 *addr, INTBIG count)
{
	return(write(channel, addr, count));
}

/*
 * routine to close a channel in "channel"
 */
INTBIG eclose(int channel)
{
	return(close(channel));
}

/******************** TIME ROUTINES ********************/

/*
 * This routine returns the amount to add to the operating-system time
 * to adjust for a common time format.
 */
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
#ifdef HAVE_FTIME
	struct timeb tp;
	static INTBIG basesecs = 0;
	INTBIG ticks;

	ftime(&tp);
	if (basesecs == 0) basesecs = tp.time;
	ticks = (tp.time-basesecs) * 60 + (tp.millitm * 6 / 100);
	return(ticks + gra_timeoffset);
#else
#  ifdef HAVE_GETTIMEOFDAY
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);
	return(tp.tv_sec * 60 + tp.tv_usec * 6 / 100000 + gra_timeoffset);
#  else
	static UINTBIG theTime = 0;

	return(theTime++ + gra_timeoffset);
#  endif
#endif
}

/* returns the double-click interval in 60ths of a second */
INTBIG doubleclicktime(void)
{
	INTBIG dct;

	dct = gra_doublethresh;
	dct = dct * 60 / 1000;
	return(dct);
}

/*
 * Routine to wait "ticks" sixtieths of a second and then return.
 */
void gotosleep(INTBIG ticks)
{
	sleep(ticks);
}

/*
 * Routine to start counting time.
 */
void starttimer(void)
{
#ifdef HAVE_FTIME
	struct timeb tp;

	ftime(&tp);
	gra_timebasesec = tp.time;
	gra_timebasems = tp.millitm;
#else
	gra_timebasesec = time(0);
#endif
}

/*
 * Routine to stop counting time and return the number of elapsed seconds
 * since the last call to "starttimer()".
 */
float endtimer(void)
{
	float seconds;
#ifdef HAVE_FTIME
	INTBIG milliseconds;
	struct timeb timeptr;

	ftime(&timeptr);
	milliseconds = (timeptr.time - gra_timebasesec) * 1000 +
		(timeptr.millitm-gra_timebasems);
	seconds = ((float)milliseconds) / 1000.0;
#else
	seconds = time(0) - gra_timebasesec;
#endif
	return(seconds);
}

/*************************** EVENT ROUTINES ***************************/

/*
 * Work procedure
 */
Boolean gra_dowork(XtPointer client_data)
{
	tooltimeslice();
	return(False);
}

/*
 * Routine to handle window deletion
 */
void gra_windowdelete(Widget w, XtPointer client_data, XtPointer *call_data)
{
	/* simply disable deletion of the messages window */
	if (w == gra_msgtoplevelwidget)
	{
		ttyputerr(_("Cannot delete the messages window"));
		return;
	}

	/* queue this frame for deletion */
	gra_deletethisframe = gra_getcurrentwindowframe(w, FALSE);
}

void gra_handlequeuedframedeletion(void)
{
	CHAR *par[MAXPARS];
	REGISTER INTBIG windows;
	REGISTER WINDOWFRAME *wf, *delwf;
	REGISTER WINDOWPART *win, *nextw, *neww;
	REGISTER NODEPROTO *np;

	if (gra_deletethisframe == 0) return;
	delwf = gra_deletethisframe;
	gra_deletethisframe = 0;

	/* turn off component menu if it was deleted */
	if (delwf->floating)
	{
		par[0] = x_("off");
		us_menu(1, par);
		return;
	}

	/* make sure there is another window */
	windows = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (!wf->floating) windows++;
	if (windows < 2)
	{
		/* no other windows: kill the program */
		if (us_preventloss(NOLIBRARY, 0, TRUE)) return;
		bringdown();
	} else
	{
		/* there are other windows: allow this one to be killed (code copied from us_window("delete")) */

		/* remember that a window is being deleted */
		gra_windowbeingdeleted = 1;

		/* save highlighting and turn it off */
		us_pushhighlight();
		us_clearhighlightcount();

		startobjectchange((INTBIG)us_tool, VTOOL);

		/* kill all editor windows on this frame */
		neww = NOWINDOWPART;
		for(win = el_topwindowpart; win != NOWINDOWPART; win = nextw)
		{
			nextw = win->nextwindowpart;
			if (win->frame != delwf)
			{
				neww = win;
				continue;
			}

			/* kill this window */
			killwindowpart(win);
		}
		endobjectchange((INTBIG)us_tool, VTOOL);

		if (neww == NOWINDOWPART) el_curwindowpart = NOWINDOWPART;
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)neww, VWINDOWPART|VDONTSAVE);
		if (neww != NOWINDOWPART) np = neww->curnodeproto; else np = NONODEPROTO;
		(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"), (INTBIG)np, VNODEPROTO);

		/* restore highlighting */
		us_pophighlight(FALSE);

		/* now no widget being deleted, and kill the window properly */
		gra_windowbeingdeleted = 0;
		killwindowframe(delwf);
	}
}

/*
 * Routine called TICKSPERSECOND times each second.  Sees if nothing has happened since
 * the last button-down action, and repeats it if appropriate.
 */
void gra_timerticked(XtPointer closure, XtIntervalId *id)
{
	/* requeue the timer event */
	XtAppAddTimeOut(gra_xtapp, 1000/TICKSPERSECOND, gra_timerticked, 0);

	/* see if it is time to fake another motion event */
	gra_cureventtime++;
	if (gra_cureventtime > gra_lasteventtime+INITIALTIMERDELAY)
	{
		/* see if the last event was a mouse button going down */
		if ((gra_lasteventstate&ISBUTTON) != 0 && (gra_lasteventstate&BUTTONUP) == 0)
		{
			/* turn it into a null motion */
			gra_addeventtoqueue((gra_lasteventstate & ~ISBUTTON) | MOTION,
				gra_lasteventspecial, gra_lasteventx, gra_lasteventy);
			gra_lasteventtime -= INITIALTIMERDELAY;
			if (el_curwindowframe != NOWINDOWFRAME)
				XSendEvent(gra_samplemotionevent.xmotion.display, gra_samplemotionevent.xmotion.window,
					True, PointerMotionMask, &gra_samplemotionevent);
			return;
		}

		/* see if the last event was a motion with button down */
		if ((gra_lasteventstate&(MOTION|BUTTONUP)) == MOTION)
		{
			/* repeat the last event */
			gra_addeventtoqueue(gra_lasteventstate, gra_lasteventspecial, gra_lasteventx, gra_lasteventy);
			gra_lasteventtime -= INITIALTIMERDELAY;
			if (el_curwindowframe != NOWINDOWFRAME)
				XSendEvent(gra_samplemotionevent.xmotion.display, gra_samplemotionevent.xmotion.window,
					True, PointerMotionMask, &gra_samplemotionevent);
			return;
		}
	}
}

/*
 * Routine to queue an event of type "state" (with special code "special")
 * at (x,y).
 */
void gra_addeventtoqueue(INTBIG state, INTBIG special, INTBIG x, INTBIG y)
{
	MYEVENTQUEUE *next;

	next = gra_eventqueuetail + 1;
	if (next >= &gra_eventqueue[EVENTQUEUESIZE])
		next = gra_eventqueue;
	if (next == gra_eventqueuehead)
	{
		/* queue is full */
		if (!gra_checkinginterrupt) XBell(gra_maindpy, 100);
		return;
	}

	gra_lasteventstate = gra_eventqueuetail->inputstate = state;
	gra_eventqueuetail->special = special;
	gra_lasteventspecial = gra_eventqueuetail->special = special;
	gra_lasteventx = gra_eventqueuetail->cursorx = x;
	gra_lasteventy = gra_eventqueuetail->cursory = y;
	gra_eventqueuetail->eventtime = ticktime();
	gra_lasteventtime = gra_cureventtime;
	gra_eventqueuetail = next;
}

/*
 * Routine to dequeue the next event and place it in the globals
 * "gra_inputstate", "gra_cursorx", "gra_cusory".  If the event is
 * special, handle it.
 */
void gra_pickupnextevent(void)
{
	CHAR *par[2];
	INTBIG special, state;
	INTBIG x, y;
	XmTextPosition tp;

	/* get the next event from the queue */
	gra_inputstate = NOEVENT;
	while (gra_eventqueuehead != gra_eventqueuetail)
	{
		special = gra_eventqueuehead->special;
		state = gra_eventqueuehead->inputstate;
		x = gra_eventqueuehead->cursorx;
		y = gra_eventqueuehead->cursory;
		gra_eventtime = gra_eventqueuehead->eventtime;
		gra_eventqueuehead++;
		if (gra_eventqueuehead >= &gra_eventqueue[EVENTQUEUESIZE])
			gra_eventqueuehead = gra_eventqueue;

		/* stop if this is the last event in the queue */
		if (gra_eventqueuehead == gra_eventqueuetail) break;

		/* stop if this and the next event are not motion */
		if ((state&MOTION) == 0) break;
		if ((gra_eventqueuehead->inputstate&MOTION) == 0) break;
	}

	/* handle special actions */
	if (special != 0 && (special&SPECIALKEYDOWN) != 0 && !gra_tracking)
	{
		switch ((special&SPECIALKEY)>>SPECIALKEYSH)
		{
			case SPECIALCUT:
				par[0] = x_("cut");
				us_text(1, par);
				return;
			case SPECIALCOPY:
				par[0] = x_("copy");
				us_text(1, par);
				return;
			case SPECIALPASTE:
				par[0] = x_("paste");
				us_text(1, par);
				return;
			case SPECIALUNDO:
				us_undo(0, par);
				return;
			case SPECIALEND:
				/* shift to end of messages window */
				tp = XmTextGetLastPosition(gra_messageswidget);
				XmTextSetSelection(gra_messageswidget, tp, tp, 0);
				XmTextSetInsertionPosition(gra_messageswidget, tp);
				XmTextShowPosition(gra_messageswidget, tp);
				return;
		}
	}

	gra_inputstate = state;
	gra_inputspecial = special;
	gra_cursorx = x;
	gra_cursory = y;
	us_state &= ~GOTXY;
}

void gra_nextevent(void)
{
	XEvent event;
	REGISTER INTBIG windowindex;
	REGISTER INTBIG x, y;
	BOOLEAN verbose;
	POPUPMENU *pm;
	REGISTER WINDOWFRAME *wf;
	REGISTER INTBIG i, j;

	/* handle any queued window deletions */
	gra_handlequeuedframedeletion();

	/* flush the screen */
	flushscreen();

	/* get any event on the queue */
	if (gra_eventqueuehead != gra_eventqueuetail)
	{
		gra_pickupnextevent();
	} else
	{
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
			XmUpdateDisplay(wf->toplevelwidget);

		/* handle any pending event */
		XtAppNextEvent(gra_xtapp, &event);
		XtDispatchEvent(&event);

		/* get any event on the queue */
		if (gra_eventqueuehead != gra_eventqueuetail)
			gra_pickupnextevent();
	}

	/* override the event if playing back */
	if (us_logplay != NULL)
	{
		/* playing back log file: get event from disk */
		if (gra_inputstate != NOEVENT || gra_firstactivedialog != NOTDIALOG)
			(void)gra_loggetnextaction(0);
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
			gra_logwriteaction(gra_inputstate, gra_inputspecial, gra_cursorx, gra_cursory, (void *)windowindex);
		}

		/* handle menu events */
		if (gra_inputstate == MENUEVENT)
		{
			gra_inputstate = NOEVENT;
			i = gra_cursorx;
			j = gra_cursory;
			for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
				if (!wf->floating) break;
			if (wf != NOWINDOWFRAME && i >= 0 && i < wf->pulldownmenucount)
			{
				pm = us_getpopupmenu(wf->pulldowns[i]);
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
		}
	}
}

void gra_messages_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont)
{
	CHAR1 buffer[2];
	REGISTER INTBIG state;
	INTBIG special;
	KeySym key;

	switch (event->type)
	{
		case VisibilityNotify:
			if (event->xvisibility.state == VisibilityUnobscured)
				gra_messages_obscured = FALSE; else
					gra_messages_obscured = TRUE;
			break;

		case FocusIn:
			gra_msgontop = TRUE;
			break;

		case FocusOut:
			gra_msgontop = FALSE;
			break;

		case KeyPress:
			(void)XLookupString((XKeyEvent *)event, buffer, 2, &key, NULL);
			if ((key == 'c' || key == 'C') && (event->xbutton.state & ControlMask) != 0)
			{
				/* we have to do interrupts here */
				XtNoticeSignal(gra_intsignalid);
				el_pleasestop = 1;
				state = 0;
				special = 0;
			} else state = gra_translatekey(event, &special);
			if (state != 0)
			{
				us_state |= DIDINPUT;
				gra_addeventtoqueue(state, special, 0, 0);
			}
			event->type = 0;     /* disable this event so key doesn't go into window */
			break;

#if 0		/* for debugging X events */
		default:
			efprintf(stderr, _("Unhandled event %s in messages window\n"),
				eventNames[event->type]);
			break;
#endif
	}
}

void gra_graphics_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont)
{
	BOOLEAN setcursor, inmenu;
	INTBIG wid, hei, special;
	CHAR *str;
	REGISTER INTBIG x, y, state, xfx, xfy;
	REGISTER UINTBIG thetime;
	INTBIG lx, hx, ly, hy;
	REGISTER WINDOWPART *win;
	REGISTER EDITOR *e;
	REGISTER VARIABLE *var;
	REGISTER TDIALOG *dia;
	XWindowAttributes xwa;
	COMMANDBINDING commandbinding;
	REGISTER WINDOWFRAME *wf;
	static BOOLEAN overrodestatus = FALSE;
	extern INTBIG sim_window_wavexbar;
	Window thewin;
	int retval;

	switch (event->type)
	{
		case ConfigureNotify:
			wf = gra_getcurrentwindowframe(w, FALSE);
			if (wf == NOWINDOWFRAME) return;
			if (w != wf->toplevelwidget) break;

			gra_getwindowinteriorsize(wf, &wid, &hei);
			gra_getwindowattributes(wf, &xwa);

			/* update user's positioning of component menu */
			thetime = ticktime();
			if (thetime - 120 > gra_internalchange && wf->floating)
			{
				gra_palettetop = xwa.y;
				if (wid > hei) gra_palettewidth = wid; else
					gra_paletteheight = hei;
			}
			gra_internalchange = thetime;

			if (wid != wf->swid || hei != wf->trueheight)
			{
				/* window changed size */
				gra_addeventtoqueue(WINDOWSIZE, 0, wf->windindex,
					((xwa.width & 0xFFFF) << 16) | (xwa.height & 0xFFFF));
				gra_repaint(wf, TRUE);
			}

			x = xwa.x;   y = xwa.y;
			gra_addeventtoqueue(WINDOWMOVE, 0, wf->windindex,
				((x & 0xFFFF) << 16) | (y & 0xFFFF));
			break;

		case Expose:
			/* ignore partial events */
			if (event->xexpose.count != 0) break;
			if (gra_lastwidgetfocused == w)
				wf = gra_getcurrentwindowframe(w, TRUE); else
					wf = gra_getcurrentwindowframe(w, FALSE);
#ifdef ANYDEPTH
			if (wf != NOWINDOWFRAME)
			{
				wf->copyleft = 0;   wf->copyright = wf->swid;
				wf->copytop = 0;    wf->copybottom = wf->trueheight;
				wf->offscreendirty = TRUE;
			}
			flushscreen();
#else
			if (wf == NOWINDOWFRAME) return;
			gra_repaint(wf, FALSE);
#endif
			break;

		case FocusIn:
			gra_lastwidgetfocused = w;
			break;

		case ClientMessage:
			break;

		case ReparentNotify:
			break;

		case KeyPress:
			/* ignore this event if there is a modal dialog present */
			for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
				if (dia->modelessitemhit == 0) break;
			if (dia != NOTDIALOG) break;

			wf = gra_getcurrentwindowframe(w, TRUE);
			if (wf == NOWINDOWFRAME) return;
			state = gra_translatekey(event, &special);
			if (state == 0) break;
			us_state |= DIDINPUT;

			/* don't track in the status window */
			gra_cursorx = event->xkey.x;
			gra_cursory = maxi(wf->revy - event->xkey.y, 0);
			gra_addeventtoqueue(state, special, gra_cursorx, gra_cursory);
			break;

		case ButtonPress:  /* these are ignored in the messages window */
			if (us_logplay != NULL)
			{
				ttyputerr(_("Session playback aborted by mouse click"));
				xclose(us_logplay);
				us_logplay = NULL;
				break;
			}
			gra_lastbuttonpressevent = *event;

			/* ignore this event if there is a modal dialog present */
			for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
				if (dia->modelessitemhit == 0) break;
			if (dia != NOTDIALOG) break;

			wf = gra_getcurrentwindowframe(w, TRUE);
			if (wf == NOWINDOWFRAME) return;
			switch (event->xbutton.button)
			{
				case 1: state = ISLEFT;    break;
				case 2: state = ISMIDDLE;  break;
				case 3: state = ISRIGHT;   break;
				case 4: state = ISWHLFWD;  break;
				case 5: state = ISWHLBKW;  break;
			}
			if ((event->xbutton.state & ShiftMask) != 0) state |= SHIFTISDOWN;
			if ((event->xbutton.state & LockMask) != 0) state |= SHIFTISDOWN;
			if ((event->xbutton.state & ControlMask) != 0) state |= CONTROLISDOWN;
			if ((event->xbutton.state & (Mod1Mask|Mod4Mask)) != 0) state |= METAISDOWN;
			gra_cursorx = event->xbutton.x;
			gra_cursory = maxi(wf->revy - event->xbutton.y, 0);
			if ((event->xbutton.time - gra_lasttime) <= gra_doublethresh &&
				(state & (SHIFTISDOWN|CONTROLISDOWN|METAISDOWN)) == 0 &&
				gra_cursorx == gra_lstcurx && gra_cursory == gra_lstcury)
			{
				state |= DOUBLECL;
				gra_lasttime = event->xbutton.time - gra_doublethresh - 1;
			} else
			{
				gra_lasttime = event->xbutton.time;
			}
			gra_addeventtoqueue(state, 0, gra_cursorx, gra_cursory);
			gra_lstcurx = gra_cursorx;
			gra_lstcury = gra_cursory;
			us_state |= DIDINPUT;
			break;

		case ButtonRelease:
			/* ignore this event if there is a modal dialog present */
			for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
				if (dia->modelessitemhit == 0) break;
			if (dia != NOTDIALOG) break;

			wf = gra_getcurrentwindowframe(w, TRUE);
			if (wf == NOWINDOWFRAME) return;
			switch (event->xbutton.button)
			{
				case 1: state = ISLEFT | BUTTONUP;   break;
				case 2: state = ISMIDDLE | BUTTONUP; break;
				case 3: state = ISRIGHT | BUTTONUP;  break;
			}
			if ((event->xbutton.state & ShiftMask) != 0) state |= SHIFTISDOWN;
			if ((event->xbutton.state & LockMask) != 0) state |= SHIFTISDOWN;
			if ((event->xbutton.state & ControlMask) != 0) state |= CONTROLISDOWN;
			if ((event->xbutton.state & (Mod1Mask|Mod4Mask)) != 0) state |= METAISDOWN;
			gra_cursorx = event->xbutton.x;
			gra_cursory = maxi(wf->revy - event->xbutton.y, 0);
			gra_addeventtoqueue(state, 0, gra_cursorx, gra_cursory);
			us_state |= DIDINPUT;
			break;

		case MotionNotify:
			/* save this event for faking others */
			gra_samplemotionevent = *event;

			/* ignore faked events (done to wake up event loop) */
			if (event->xmotion.send_event) break;

			if (gra_firstbuttonwait == 0) break;
			if ((event->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) == 0)
			{
				/* motion while up considers any window */
				wf = gra_getcurrentwindowframe(w, FALSE);
				if (wf == NOWINDOWFRAME) return;
				gra_cursorx = event->xmotion.x;
				gra_cursory = wf->revy - event->xmotion.y;
				if (gra_cursory < 0) gra_cursory = 0;

				/* report the menu if over one */
				inmenu = FALSE;
				if (wf->floating)
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
									ttysetstatusfield(NOWINDOWFRAME, us_statusarc,
										describearcproto(us_curarcproto), TRUE);
									ttysetstatusfield(NOWINDOWFRAME, us_statusnode,
										us_describemenunode(&commandbinding), TRUE);
									inmenu = TRUE;
									overrodestatus = TRUE;
								}
								if (commandbinding.arcglyph != NOARCPROTO)
								{
									ttysetstatusfield(NOWINDOWFRAME, us_statusarc,
										describearcproto(commandbinding.arcglyph), TRUE);
									if (us_curnodeproto == NONODEPROTO) str = x_(""); else
										str = describenodeproto(us_curnodeproto);
									ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
									inmenu = TRUE;
									overrodestatus = TRUE;
								}
							}
							us_freebindingparse(&commandbinding);
						}
					}
				}
				if (!inmenu && overrodestatus)
				{
					ttysetstatusfield(NOWINDOWFRAME, us_statusarc,
						describearcproto(us_curarcproto), TRUE);
					if (us_curnodeproto == NONODEPROTO) str = x_(""); else
						str = describenodeproto(us_curnodeproto);
					ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
					overrodestatus = FALSE;
				}

				setcursor = FALSE;
				for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
				{
					/* see if the cursor is over a window partition separator */
					if (win->frame != wf) continue;
					us_gettruewindowbounds(win, &lx, &hx, &ly, &hy);
					if (gra_cursorx >= lx-1 && gra_cursorx <= lx+1 && gra_cursory > ly+1 &&
						gra_cursory < hy-1 && us_hasotherwindowpart(lx-10, gra_cursory, win))
					{
						setdefaultcursortype(LRCURSOR);
						setcursor = TRUE;
						break;
					} else if (gra_cursorx >= hx-1 && gra_cursorx <= hx+1 && gra_cursory > ly+1 &&
						gra_cursory < hy-1 && us_hasotherwindowpart(hx+10, gra_cursory, win))
					{
						setdefaultcursortype(LRCURSOR);
						setcursor = TRUE;
						break;
					} else if (gra_cursory >= ly-1 && gra_cursory <= ly+1 && gra_cursorx > lx+1 &&
						gra_cursorx < hx-1 && us_hasotherwindowpart(gra_cursorx, ly-10, win))
					{
						setdefaultcursortype(UDCURSOR);
						setcursor = TRUE;
						break;
					} else if (gra_cursory >= hy-1 && gra_cursory <= hy+1 && gra_cursorx > lx+1 &&
						gra_cursorx < hx-1 && us_hasotherwindowpart(gra_cursorx, hy+10, win))
					{
						setdefaultcursortype(UDCURSOR);
						setcursor = TRUE;
						break;
					}

					if (gra_cursorx < win->uselx || gra_cursorx > win->usehx ||
						gra_cursory < win->usely || gra_cursory > win->usehy) continue;
					if ((win->state&WINDOWTYPE) == WAVEFORMWINDOW)
					{
						xfx = muldiv(gra_cursorx - win->uselx, win->screenhx - win->screenlx,
							win->usehx - win->uselx) + win->screenlx;
						xfy = muldiv(y - win->usely, win->screenhy - win->screenly,
							win->usehy - win->usely) + win->screenly;
						if (abs(xfx - sim_window_wavexbar) < 2 && xfy >= 560)
						{
							setdefaultcursortype(LRCURSOR);
							setcursor = TRUE;
							break;
						}
					}
					if ((win->state&WINDOWTYPE) == POPTEXTWINDOW ||
						(win->state&WINDOWTYPE) == TEXTWINDOW)
					{
						e = win->editor;
						if ((e->state&EDITORTYPE) == PACEDITOR)
						{
							if (gra_cursorx <= win->usehx - SBARWIDTH &&
								gra_cursory >= win->usely + SBARWIDTH &&
								gra_cursory < e->revy)
							{
								setdefaultcursortype(IBEAMCURSOR);
								setcursor = TRUE;
								break;
							}
						}
					}
				}
				if (!setcursor) setdefaultcursortype(us_normalcursor);

				/* record the action */
				state = MOTION | BUTTONUP;
				gra_addeventtoqueue(state, 0, gra_cursorx, gra_cursory);
			} else
			{
				/* motion while button down: use last window */
				wf = el_curwindowframe;
				if (wf == NOWINDOWFRAME) return;
				gra_cursorx = event->xmotion.x;
				gra_cursory = wf->revy - event->xmotion.y;
				if (gra_cursory < 0) gra_cursory = 0;

				/* record the action */
				state = MOTION;
				gra_addeventtoqueue(state, 0, gra_cursorx, gra_cursory);
			}
			break;

#if 0		/* for debugging X events */
		default:
			efprintf(stderr, _("Unhandled event %s in graphics window.\n"),
				eventNames[event->type]);
			break;
#endif
	}
}

/*
 * routine to determine the current window frame and load the global "el_curwindowframe".
 */
WINDOWFRAME *gra_getcurrentwindowframe(Widget widget, BOOLEAN set)
{
	WINDOWFRAME *wf;
	WINDOWPART *w;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->toplevelwidget == widget) break;
		if (wf->intermediategraphics == widget) break;
		if (wf->graphicswidget == widget) break;
	}
	if (!set) return(wf);

	el_curwindowframe = wf;
	if (wf == NOWINDOWFRAME) return(NOWINDOWFRAME);

	/* ignore floating windows */
	if (wf->floating) return(wf);

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
#if 0
					return(NOWINDOWFRAME);
#else
					break;
#endif
				}
			}
		}
	}
	return(el_curwindowframe);
}

/*
 * Routine to translate key symbols to electric state.
 * Sets "special" to a nonzero code if a special function key was hit
 * Returns 0 if the key is unknown.
 */
INTBIG gra_translatekey(XEvent *event, INTBIG *special)
{
	CHAR1 buffer[2];
	KeySym key;
	INTBIG state;

	*special = 0;
	(void)XLookupString((XKeyEvent *)event, buffer, 2, &key, NULL);
	if (key > 0 && key < 0200) state = key; else
	{
		switch (key)
		{
			case XK_KP_0:      state = '0';            break;
			case XK_KP_1:      state = '1';            break;
			case XK_KP_2:      state = '2';            break;
			case XK_KP_3:      state = '3';            break;
			case XK_KP_4:      state = '4';            break;
			case XK_KP_5:      state = '5';            break;
			case XK_KP_6:      state = '6';            break;
			case XK_KP_7:      state = '7';            break;
			case XK_KP_8:      state = '8';            break;
			case XK_KP_9:      state = '9';            break;
			case XK_BackSpace: state = BACKSPACEKEY;   break;
			case XK_Tab:       state = 011;            break;
			case XK_Linefeed:  state = 012;            break;
			case XK_Return:    state = 015;            break;
			case XK_Escape:    state = ESCKEY;         break;
			case XK_Delete:    state = 0177;           break;
			case XK_KP_Enter:  state = 015;            break;

			case XK_End:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALEND<<SPECIALKEYSH);
				break;
			case XK_L4:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALUNDO<<SPECIALKEYSH);
				break;
			case XK_L6:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALCOPY<<SPECIALKEYSH);
				break;
			case XK_L8:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALPASTE<<SPECIALKEYSH);
				break;
			case XK_L10:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALCUT<<SPECIALKEYSH);
				break;

			case XK_Left:
			case XK_KP_Left:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWL<<SPECIALKEYSH);
				if ((event->xbutton.state & ShiftMask) != 0) *special |= SHIFTDOWN;
				break;
			case XK_Right:
			case XK_KP_Right:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWR<<SPECIALKEYSH);
				if ((event->xbutton.state & ShiftMask) != 0) *special |= SHIFTDOWN;
				break;
			case XK_Up:
			case XK_KP_Up:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWU<<SPECIALKEYSH);
				if ((event->xbutton.state & ShiftMask) != 0) *special |= SHIFTDOWN;
				break;
			case XK_Down:
			case XK_KP_Down:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWD<<SPECIALKEYSH);
				if ((event->xbutton.state & ShiftMask) != 0) *special |= SHIFTDOWN;
				break;

			case XK_F1:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF1<<SPECIALKEYSH);
				break;
			case XK_F2:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF2<<SPECIALKEYSH);
				break;
			case XK_F3:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF3<<SPECIALKEYSH);
				break;
			case XK_F4:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF4<<SPECIALKEYSH);
				break;
			case XK_F5:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF5<<SPECIALKEYSH);
				break;
			case XK_F6:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF6<<SPECIALKEYSH);
				break;
			case XK_F7:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF7<<SPECIALKEYSH);
				break;
			case XK_F8:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF8<<SPECIALKEYSH);
				break;
			case XK_F9:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF9<<SPECIALKEYSH);
				break;
			case XK_F10:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF10<<SPECIALKEYSH);
				break;
			case XK_F11:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF11<<SPECIALKEYSH);
				break;
			case XK_F12:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF12<<SPECIALKEYSH);
				break;

			default:
				state = 0;
				break;
		}
	}
	if (state == 0 && *special == 0) return(0);

	if ((event->xbutton.state & ControlMask) != 0)
	{
		if (!gra_messages_typingin) *special |= ACCELERATORDOWN; else
		{
			if (state >= 'a' && state <= 'z') state -= 'a' - 1; else
				if (state >= 'A' && state <= 'Z') state -= 'A' - 1;
		}
	}
	state |= ISKEYSTROKE;
	return(state);
}

void gra_repaint(WINDOWFRAME *wf, BOOLEAN redo)
{
	INTBIG wid, hei;
	static BOOLEAN inrepaint = FALSE;

	if (inrepaint) return;
	inrepaint = TRUE;

	gra_getwindowinteriorsize(wf, &wid, &hei);
	gra_recalcsize(wf, wid, hei);

	if (wf->floating)
	{
		us_startbatch(NOTOOL, FALSE);
		if (redo) us_drawmenu(1, wf); else
			us_drawmenu(-1, wf);
		us_endbatch();
		inrepaint = FALSE;
		return;
	}

	if (el_topwindowpart != NOWINDOWPART)
	{
		/* mark start of redisplay */
		us_startbatch(NOTOOL, FALSE);

		/* save and clear highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* rewrite status area */
		if (redo) us_drawmenu(1, wf); else
			us_drawmenu(-1, wf);

		/* restore highlighting */
		us_pophighlight(FALSE);

		/* finish drawing */
		us_endbatch();

		us_redostatus(wf);

		/* describe this change */
		setactivity(_("Window Resize"));
	}
	inrepaint = FALSE;
}

void gra_recalcsize(WINDOWFRAME *wf, INTBIG newwid, INTBIG newhei)
{
#ifdef ANYDEPTH
	INTBIG i, j;
	XImage *newimage;
	UCHAR1 *newdataaddr8, **newrowstart, *newdataaddr, **newoffrowstart;
	INTBIG depth, truedepth;
	Visual *visual;

	/* make sure the height isn't too small for the status bar */
	if (wf->floating && newhei < gra_status_height + 2)
	{
		newhei = gra_status_height + 2;
		ttyputmsg(_("Window too short: made minimum height"));
	}

	if (newwid != wf->swid || newhei != wf->trueheight)
	{
		/* allocate space for the 8-bit deep buffer */
		newdataaddr8 = (UCHAR1 *)emalloc(newwid * newhei, us_tool->cluster);
		if (newdataaddr8 == 0) return;
		newrowstart = (UCHAR1 **)emalloc(newhei * SIZEOFINTBIG, us_tool->cluster);
		if (newrowstart == 0) return;
		for(i=0; i<newhei; i++)
		{
			newrowstart[i] = newdataaddr8 + i * newwid;
			for(j=0; j<newwid; j++)
				newrowstart[i][j] = 0;
		}

		/* deallocate old buffers and fill in new */
		efree((CHAR *)wf->dataaddr8);
		efree((CHAR *)wf->rowstart);
		wf->dataaddr8 = newdataaddr8;
		wf->rowstart = newrowstart;

		/* allocate space for the full-depth buffer */
		if (newwid > wf->wd->wid || newhei > wf->wd->hei)
		{
			gra_getdisplaydepth(wf->topdpy, &depth, &truedepth);
			newdataaddr = (UCHAR1 *)emalloc(newwid * newhei *
				truedepth / 8, us_tool->cluster);
			if (newdataaddr == 0) return;
			newoffrowstart = (UCHAR1 **)emalloc(newhei * SIZEOFINTBIG, us_tool->cluster);
			if (newoffrowstart == 0) return;
			visual = DefaultVisual(wf->topdpy, DefaultScreen(wf->topdpy));
			newimage = XCreateImage(wf->topdpy, visual, truedepth,
				ZPixmap, 0, (CHAR1 *)newdataaddr, newwid, newhei, depth, 0);
			if (newimage == 0)
			{
				efprintf(stderr, _("Error allocating new image array that is %ldx%ld\n"),
					newwid, newhei);
				return;
			}
			for(i=0; i<newhei; i++)
				newoffrowstart[i] = newdataaddr + i * newimage->bytes_per_line;

			/* deallocate old data and set new */
			efree((CHAR *)wf->wd->addr);
			efree((CHAR *)wf->wd->rowstart);
			XFree(wf->wd->image);
			wf->wd->addr = newdataaddr;
			wf->wd->rowstart = newoffrowstart;
			wf->wd->image = newimage;
			wf->wd->wid = newwid;   wf->wd->hei = newhei;
		}
	}
#else
	XClearWindow(wf->topdpy, wf->win);
#endif
	wf->swid = newwid;
	wf->trueheight = newhei;
	wf->shei = wf->trueheight;
	if (!wf->floating) wf->shei -= gra_status_height;
	wf->revy = wf->shei - 1;
}

/*
 * support routine to print any internal errors
 */
int gra_xerrors(Display *dpy, XErrorEvent *err)
{
	CHAR1 buffer[100];

	XGetErrorText(dpy, err->error_code, buffer, 100);
	/* buffer is now in the "encoding of the current locale" */
	ttyputerr(_("ERROR: X Window System routine %d has %s"), err->request_code,
		string2byte(buffer));
	return(0);
}

void gra_xterrors(CHAR1 *msg)
{
	ttyputerr(_("ERROR: X Toolkit: %s"), string2byte(msg));
}

/* error events */
RETSIGTYPE gra_sigill_trap(void)
{
	(void)signal(SIGILL, (SIGNALCAST)gra_sigill_trap);
	error(_("FATAL ERROR: An illegal instruction has been trapped"));
}

RETSIGTYPE gra_sigfpe_trap(void)
{
	(void)signal(SIGFPE, (SIGNALCAST)gra_sigfpe_trap);
	error(_("FATAL ERROR: A numerical error has occurred"));
}

RETSIGTYPE gra_sigbus_trap(void)
{
	(void)signal(SIGBUS, (SIGNALCAST)gra_sigbus_trap);
	error(_("FATAL ERROR: A bus error has occurred"));
}

RETSIGTYPE gra_sigsegv_trap(void)
{
	(void)signal(SIGSEGV, (SIGNALCAST)gra_sigsegv_trap);
	error(_("FATAL ERROR: A segmentation violation has occurred"));
}

void gra_intsignalfunc(void)
{
	ttyputerr(_("Interrupted..."));
}

/*************************** SESSION LOGGING ROUTINES ***************************/

/* Session Playback */
DIALOGITEM gra_sesplaydialogitems[] =
{
 /*  1 */ {0, {100,132,124,212}, BUTTON, N_("Yes")},
 /*  2 */ {0, {100,8,124,88}, BUTTON, N_("No")},
 /*  3 */ {0, {4,8,20,232}, MESSAGE, N_("Electric has found a session log file")},
 /*  4 */ {0, {24,8,40,232}, MESSAGE, N_("which may be from a recent crash.")},
 /*  5 */ {0, {52,8,68,232}, MESSAGE, N_("Do you wish to replay this session")},
 /*  6 */ {0, {72,8,88,232}, MESSAGE, N_("and reconstruct the lost work?")}
};
DIALOG gra_sesplaydialog = {{75,75,208,316}, N_("Replay Log?"), 0, 6, gra_sesplaydialogitems, 0, 0};

/* special items for the session playback dialog: */
#define DSPL_YES    1		/* Yes (button) */
#define DSPL_NO     2		/* No (button) */

/*
 * routine to create a session logging file
 */
void logstartrecord(void)
{
	REGISTER INTBIG itemhit, count, filestatus;
	REGISTER LIBRARY *lib;
	REGISTER WINDOWFRAME *wf;
	REGISTER WINDOWPART *w;
	REGISTER VARIABLE *var;
	XWindowAttributes xwa;
	REGISTER void *dia;
	CHAR1 srcfile1[300], srcfile2[300];

	/* if there is already a log file, it may be from a previous crash */
	filestatus = fileexistence(gra_logfile);
	if (filestatus == 1 || filestatus == 3)
	{
		dia = DiaInitDialog(&gra_sesplaydialog);
		if (dia == 0) return;
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
			TRUESTRCPY(srcfile1, string1byte(gra_logfile));
			TRUESTRCPY(srcfile2, string1byte(gra_logfilesave));
			rename(srcfile1, srcfile2);
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
	xprintf(us_logrecord, x_("LC %ld\n"), count);
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
	xprintf(us_logrecord, x_("WO %ld\n"), gra_windownumber);
	count = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe) count++;
	xprintf(us_logrecord, x_("WT %ld\n"), count);
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		gra_getwindowattributes(wf, &xwa);
		xwa.x -= WMLEFTBORDER;
		xwa.y -= WMTOPBORDER;
		xwa.width += 2;
		if (wf->floating != 0)
		{
			xprintf(us_logrecord, x_("WC %ld %ld %ld %ld %ld\n"), wf->windindex, xwa.x, xwa.y,
				xwa.width, xwa.height);
		} else
		{
			xprintf(us_logrecord, x_("WE %ld %ld %ld %ld %ld\n"), wf->windindex, xwa.x, xwa.y,
				xwa.width, xwa.height);
		}

		count = 0;
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			if (w->frame == wf) count++;
		xprintf(us_logrecord, x_("WP %ld\n"), count);
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
	xprintf(us_logrecord, x_("; WS n w h t     Window 'n' grows to (wXh), time 't'\n"));
	xprintf(us_logrecord, x_("; WM n x y t     Window 'n' moves to (x,y), time 't'\n"));
	xprintf(us_logrecord, x_("; FS path        File selected is 'path'\n"));
	xprintf(us_logrecord, x_("; PM v t         Popup menu selected 'v', time 't'\n"));
	xprintf(us_logrecord, x_("; DI w i         Item 'i' of dialog selected\n"));
	xprintf(us_logrecord, x_("; DS w i c vals  Dialog 'w' scroll item 'i' selects 'c' lines in 'values'\n"));
	xprintf(us_logrecord, x_("; DE w i hc s    Dialog 'w' edit item 'i' hit character 'hc', text now 's'\n"));
	xprintf(us_logrecord, x_("; DP w i e       Dialog 'w' popup item 'i' set to entry 'e'\n"));
	xprintf(us_logrecord, x_("; DC w i v       Dialog 'w' item 'i' set to value 'v'\n"));
	xprintf(us_logrecord, x_("; DM w x y       Dialog 'w' coordinates at (x,y)\n"));
	xprintf(us_logrecord, x_("; DD w           Dialog 'w' done\n"));
}

/*
 * routine to begin playback of session logging file "file".  The routine
 * returns true if there is an error.  If "all" is nonzero, playback
 * the entire file with no prompt.
 */
BOOLEAN logplayback(CHAR *file)
{
	CHAR *filename, tempstring[300], *pt, *start;
	BOOLEAN cur, fromdisk;
	BOOLEAN floating;
	REGISTER WINDOWFRAME *wf;
	RECTAREA r;
	REGISTER FILE *saveio;
	REGISTER WINDOWPART *w, *nextw;
	REGISTER INTBIG i, j, wcount, count, wid, hei, uselx, usehx, usely, usehy, sindex;
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
			fromdisk = TRUE;
		} else if (estrncmp(tempstring, x_("LM"), 2) == 0)
		{
			fromdisk = FALSE;
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
			/* read library file */
			lib = newlibrary(start, pt);
			if (lib == NOLIBRARY) continue;
			if (fromdisk)
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
	gra_windownumber = eatoi(&tempstring[3]);

	/* get current windows */
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
				cur = FALSE;
			} else if (estrncmp(tempstring, x_("WF"), 2) == 0)
			{
				cur = TRUE;
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
			if (cur) el_curwindowpart = w;
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
	CHAR1 srcfile1[300], srcfile2[300];

	if (us_logrecord != NULL)
	{
		xclose(us_logrecord);
		if (fileexistence(gra_logfilesave) == 1)
			eunlink(gra_logfilesave);
		TRUESTRCPY(srcfile1, string1byte(gra_logfile));
		TRUESTRCPY(srcfile2, string1byte(gra_logfilesave));
		rename(srcfile1, srcfile2);
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
 *   DIAEDITTEXT:   dialog "special" edit item "cursorx" and changed to "extradata"
 *   DIAPOPUPSEL:   dialog "special" popup item "cursorx" set to entry "cursory"
 *   DIASETCONTROL: dialog "special" control item "cursorx" changed to "cursory"
 *   DIAENDDIALOG:  dialog "special" terminated
 *   all others:    cursor in (cursorx,cursory) and window index in "extradata"
 */
void gra_logwriteaction(INTBIG inputstate, INTBIG special, INTBIG cursorx, INTBIG cursory,
	void *extradata)
{
	REGISTER CHAR *filename;
	REGISTER INTBIG i, trueItem;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER WINDOWFRAME *wf;

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
			us_describeboundkey(inputstate&CHARREAD, special, 1),
				cursorx, cursory, extradata, ticktime()-gra_logbasetime);
	} else if ((inputstate&ISBUTTON) != 0)
	{
		if ((inputstate&BUTTONUP) != 0)
		{
			xprintf(us_logrecord, x_("BR %ld %ld %ld %ld %ld %ld\n"),
				inputstate&(ISBUTTON|SHIFTISDOWN|METAISDOWN|CONTROLISDOWN),
					cursorx, cursory, special, extradata, ticktime()-gra_logbasetime);
		} else
		{
			if ((inputstate&DOUBLECL) != 0)
			{
				xprintf(us_logrecord, x_("BD %ld %ld %ld %ld %ld %ld\n"),
					inputstate&(ISBUTTON|SHIFTISDOWN|METAISDOWN|CONTROLISDOWN),
						cursorx, cursory, special, extradata, ticktime()-gra_logbasetime);
			} else
			{
				xprintf(us_logrecord, x_("BP %ld %ld %ld %ld %ld %ld\n"),
					inputstate&(ISBUTTON|SHIFTISDOWN|METAISDOWN|CONTROLISDOWN),
						cursorx, cursory, special, extradata, ticktime()-gra_logbasetime);
			}
		}
	} else if (inputstate == MENUEVENT)
	{
		xprintf(us_logrecord, x_("ME %ld %ld %ld"), cursorx, cursory,
			ticktime()-gra_logbasetime);
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
			if (!wf->floating) break;
		if (wf != NOWINDOWFRAME && cursorx >= 0 && cursorx < wf->pulldownmenucount)
		{
			pm = us_getpopupmenu(wf->pulldowns[cursorx]);
			if (cursory >= 0 && cursory < pm->total)
			{
				mi = &pm->list[cursory];
				xprintf(us_logrecord, x_("   ; command=%s"),
					us_stripampersand(mi->attribute));
			}
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
	REGISTER WINDOWFRAME *wf;
	REGISTER TDIALOG *dia;
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
		item = eatoi(getkeyword(&pt, x_(" ")));
		if (dia == NOTDIALOG) return(FALSE);
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
		gra_inputstate = DIAENDDIALOG;
		pt = &tempstring[3];
		dia = gra_getdialogfromindex(eatoi(getkeyword(&pt, x_(" "))));
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
		gra_inputstate = eatoi(getkeyword(&pt, x_(" ")));
		gra_cursorx = eatoi(getkeyword(&pt, x_(" ")));
		gra_cursory = eatoi(getkeyword(&pt, x_(" ")));
		gra_inputspecial = eatoi(getkeyword(&pt, x_(" ")));
		i = eatoi(getkeyword(&pt, x_(" ")));
		if (tempstring[1] == 'D') gra_inputstate |= DOUBLECL;
		if (tempstring[1] == 'R') gra_inputstate |= BUTTONUP;
		nowtime = eatoi(getkeyword(&pt, x_(" ")));
		gra_timeoffset += nowtime - gra_lastplaybacktime;
		gra_lastplaybacktime = nowtime;
	}
	wf = gra_getframefromindex(i);
	if (wf != NOWINDOWFRAME && wf != el_curwindowframe)
	{
		XMapRaised(wf->topdpy, wf->topwin);
		(void)gra_getcurrentwindowframe(wf->toplevelwidget, TRUE);
	}
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
	*acceleratorstring = x_("Ctrl");
	*acceleratorprefix = x_("Ctrl-");
}

CHAR *getinterruptkey(void)
{
	return(_("Control-C in Messages Window"));
}

INTBIG nativepopupmenu(POPUPMENU **menuptr, BOOLEAN header, INTBIG left, INTBIG top)
{
	INTBIG retval;

	retval = gra_nativepopuptif(menuptr, header, left, top);
	gra_logwriteaction(POPUPSELECT, 0, retval, 0, 0);
	return(retval);
}

INTBIG gra_nativepopuptif(POPUPMENU **menuptr, BOOLEAN header, INTBIG left, INTBIG top)
{
	Widget popmenu, child, submenu, subchild, *submenulist;
	Arg arg[10];
	INTBIG ac, i, j, submenuindex, submenus, columns;
	short hei;
	CHAR msg[200], *pt;
	POPUPMENUITEM *mi, *submi;
	POPUPMENU *menu, **subpmlist;
	REGISTER WINDOWFRAME *wf;
	REGISTER USERCOM *uc;

	while (us_logplay != NULL)
	{
		if (gra_loggetnextaction(0)) break;
		j = gra_inputstate;
		gra_inputstate = NOEVENT;
		if (j == POPUPSELECT) return(gra_cursorx);
	}

	menu = *menuptr;
	wf = el_curwindowframe;
	ac = 0;
	XtSetArg(arg[ac], XmNpacking, XmPACK_COLUMN);   ac++;
	popmenu = (Widget)XmCreatePopupMenu(wf->graphicswidget, b_("menu"), arg, ac);
	if (header)
	{
		header = 2;
		child = (Widget)XmCreatePushButton(popmenu, string1byte(menu->header), NULL, 0);
		XtManageChild(child);
		XtAddCallback(child, XmNactivateCallback,
			(XtCallbackProc)gra_menupcb, (XtPointer)-2);
		XtVaCreateManagedWidget(b_("sep"), xmSeparatorWidgetClass, popmenu, NULL);
	}

	/* count the number of submenus */
	submenus = 0;
	for(j=0; j<menu->total; j++)
	{
		mi = &menu->list[j];
		if (*mi->attribute != 0 && mi->response != NOUSERCOM &&
			mi->response->menu != NOPOPUPMENU) submenus++;
	}
	if (submenus > 0)
	{
		subpmlist = (POPUPMENU **)emalloc(submenus * (sizeof (POPUPMENU *)), us_tool->cluster);
		if (subpmlist == 0) return(-1);
		submenulist = (Widget *)emalloc(submenus * (sizeof (Widget)), us_tool->cluster);
		if (submenulist == 0) return(-1);
	}

	/* load the menus */
	columns = 1;
	submenus = 0;
	for(j=0; j<menu->total; j++)
	{
		mi = &menu->list[j];
		mi->changed = FALSE;
		if (*mi->attribute == 0)
		{
			XtVaCreateManagedWidget(b_("sep"), xmSeparatorWidgetClass, popmenu, NULL);
		} else
		{
			i = 0;
			for(pt = mi->attribute; *pt != 0; pt++)
				if (*pt != '&') msg[i++] = *pt;
			msg[i] = 0;

			uc = mi->response;
			if (uc != NOUSERCOM && uc->menu != NOPOPUPMENU)
			{
				submenu = (Widget)XmVaCreateSimplePulldownMenu(popmenu, b_("menu"), j+header,
					(XtCallbackProc)gra_menupcb, NULL);
				if (submenu == 0) return(-1);
				submenulist[submenus] = submenu;
				subpmlist[submenus] = uc->menu;
				submenus++;
				XtSetArg(arg[0], XmNsubMenuId, submenu);
				child = XmCreateCascadeButton(popmenu, string1byte(msg), arg, 1);
				XtManageChild(child);
				for(i=0; i<uc->menu->total; i++)
				{
					submi = &uc->menu->list[i];
					subchild = (Widget)XmCreatePushButton(submenu,
						string1byte(submi->attribute), NULL, 0);
					XtManageChild(subchild);
					XtAddCallback(subchild, XmNactivateCallback,
						(XtCallbackProc)gra_menupcb, (XtPointer)((submenus<<16) | i));
				}
			} else
			{
				child = (Widget)XmCreatePushButton(popmenu, string1byte(msg), NULL, 0);
				XtManageChild(child);
				XtAddCallback(child, XmNactivateCallback,
					(XtCallbackProc)gra_menupcb, (XtPointer)j);
			}
		}
		XtSetArg(arg[0], XtNheight, &hei);
		XtGetValues(popmenu, arg, 1);
		if (hei >= gra_shortestscreen - 50)
		{
			columns++;
			XtSetArg(arg[0], XmNnumColumns, columns);
			XtSetValues(popmenu, arg, 1);
		}
	}
	XtAddCallback(popmenu, XmNunmapCallback,
		(XtCallbackProc)gra_menupcb, (XtPointer)-2);
	XmMenuPosition(popmenu, (XButtonPressedEvent *)&gra_lastbuttonpressevent);
	XtManageChild(popmenu);

	gra_popupmenuresult = -1;
	gra_inputstate = NOEVENT;
	gra_tracking = TRUE;
	while (gra_popupmenuresult == -1)
	{
		gra_nextevent();
		if (gra_inputstate == NOEVENT) continue;
		if ((gra_inputstate & ISBUTTON) != 0 && (gra_inputstate & BUTTONUP) != 0)
		{
			gra_inputstate = NOEVENT;
			break;
		}
	}
	gra_tracking = FALSE;
	XtDestroyWidget(popmenu);
#if 0
	for(j=0; j<submenus; j++)
		XtDestroyWidget(submenulist[j]);
#endif
	if (gra_popupmenuresult >= 0)
	{
		submenuindex = gra_popupmenuresult >> 16;
		if (submenuindex != 0) *menuptr = subpmlist[submenuindex-1];
	}
	if (submenus > 0)
	{
		efree((CHAR *)submenulist);
		efree((CHAR *)subpmlist);
	}
	if (gra_popupmenuresult < 0) return(-1);
	return(gra_popupmenuresult & 0xFFFF);
}

void gra_nativemenudoone(WINDOWFRAME *wf, INTBIG low, INTBIG high)
{
	gra_addeventtoqueue(MENUEVENT, 0, low, high);
}

void nativemenuload(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i;
	REGISTER WINDOWFRAME *wf;

	/* remember the top-level pulldown menus */
	for(i=0; i<count; i++)
		gra_pulldowns[i] = us_getpopupmenu(par[i]);
	gra_pulldownmenucount = count;

	/* build the pulldown menu bar */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;

		/* delete any dummy menu created during initialization */
		if (wf->firstmenu != 0)
		{
			XtUnmanageChild(wf->firstmenu);
			XtDestroyWidget(wf->firstmenu);
			wf->firstmenu = 0;
		}

		/* load menus into this window */
		gra_pulldownmenuload(wf);
	}
}

void gra_pulldownmenuload(WINDOWFRAME *wf)
{
	REGISTER INTBIG i, j;
	REGISTER INTBIG keysym;
	CHAR myline[256], *pt;
	CHAR mstring[2];
	REGISTER POPUPMENU *pm;
	Widget thismenu, child;

	if (gra_pulldownmenucount == 0)
	{
		/* no menus defined yet: make a dummy one */
		wf->firstmenu = XtVaCreateManagedWidget(b_("File"), xmCascadeButtonWidgetClass, wf->menubar, NULL);
		return;
	}

	/* load full menus */
	mstring[1] = 0;
	for(i=0; i<gra_pulldownmenucount; i++)
	{
		pm = gra_pulldowns[i];
		if (pm == NOPOPUPMENU) continue;

		/* see if there is a mnemonic */
		mstring[0] = 0;
		pt = myline;
		for(j=0; pm->header[j] != 0; j++)
		{
			if (pm->header[j] == '&') mstring[0] = pm->header[j+1]; else
				*pt++ = pm->header[j];
		}
		*pt = 0;

		child = XtVaCreateManagedWidget(string1byte(myline),
			xmCascadeButtonWidgetClass, wf->menubar, NULL);
		if (mstring[0] != 0)
		{
			keysym = XStringToKeysym(string1byte(mstring));
			XtVaSetValues(child, XmNmnemonic, keysym, NULL);
		}
		thismenu = XmVaCreateSimplePulldownMenu(wf->menubar, b_("menu"), i,
			(XtCallbackProc)gra_menucb, NULL);
		if (thismenu == 0) return;
		gra_pulldownindex(wf, pm, i, thismenu);
	}
}

void nativemenurename(POPUPMENU *pm, INTBIG pindex)
{
	INTBIG i, j, len, checked, special;
	INTSML key;
	CHAR line[100], *pt, metaline[50];
	Widget w, *newpulllist;
	WidgetClass wc;
	Arg arg[5];
	REGISTER POPUPMENUITEM *mi;
	REGISTER USERCOM *uc;
	XmString label, acctext;
	REGISTER WINDOWFRAME *wf;

	/* see if there is a mnemonic */
	pt = line;
	for(j=0; pm->list[pindex].attribute[j] != 0; j++)
	{
		if (pm->list[pindex].attribute[j] != '&')
			*pt++ = pm->list[pindex].attribute[j];
	}
	*pt = 0;

	/* see if there is a check */
	checked = -1;
	len = estrlen(line) - 1;
	if (len > 0 && line[len] == '<')
	{
		line[len] = 0;
		if (line[0] == '>')
		{
			checked = 1;
			estrcpy(line, &line[1]);
		} else
		{
			checked = 0;
		}
	}

	/* look for accelerators */
	len = estrlen(line);
	metaline[0] = 0;
	for(pt = line; *pt != 0; pt++) if (*pt == '/' || *pt == '\\') break;
	if (*pt != 0)
	{
		if (pt[1] != 0)
		{
			(void)us_getboundkey(pt, &key, &special);
			estrcpy(metaline, us_describeboundkey(key, special, 1));
		}
		*pt = 0;
	}

	/* set each window's menu */
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;
		for(i=0; i<wf->pulldownmenucount; i++)
			if (namesame(wf->pulldowns[i], pm->name) == 0) break;
		if (i >= wf->pulldownmenucount) continue;

		/* resize the menu if appropriate */
		if (pm->total != wf->pulldownmenusize[i])
		{
			/* unmanage all objects in the menu */
			for(j=0; j<wf->pulldownmenusize[i]; j++)
			{
				w = wf->pulldownmenulist[i][j];
				if (w != 0) XtDestroyWidget(w);
			}
			newpulllist = (Widget *)emalloc(pm->total * (sizeof (Widget)),
				us_tool->cluster);
			for(j=0; j<pm->total; j++) newpulllist[j] = 0;
			efree((CHAR *)wf->pulldownmenulist[i]);
			wf->pulldownmenulist[i] = newpulllist;
			wf->pulldownmenusize[i] = pm->total;
		}

		if (pindex < 0 || pindex >= wf->pulldownmenusize[i]) continue;
		w = wf->pulldownmenulist[i][pindex];
		if (w != 0)
		{
			label = XmStringCreateLocalized(string1byte(line));
			XtSetArg(arg[0], XmNlabelString, label);
			XtSetValues(w, arg, 1);
			XmStringFree(label);
			acctext = XmStringCreateLocalized(string1byte(metaline));
			XtSetArg(arg[0], XmNacceleratorText, acctext);
			XtSetValues(w, arg, 1);
			XmStringFree(acctext);
			if (checked == 0)
			{
				XmToggleButtonSetState(w, False, 0);
			} else if (checked == 1)
			{
				XmToggleButtonSetState(w, True, 0);
			}
			continue;
		}

		/* must create the entry */
		mi = &pm->list[pindex];
		uc = mi->response;
		if (uc->active < 0)
		{
			if (*mi->attribute == 0)
			{
				wf->pulldownmenulist[i][pindex] = XtVaCreateManagedWidget(b_("sep"), xmSeparatorWidgetClass,
					wf->pulldownmenus[i], NULL);
				continue;
			}
		}
		wf->pulldownmenulist[i][pindex] = (Widget)XmCreatePushButton(wf->pulldownmenus[i],
			string1byte(line), NULL, 0);
		if (*metaline != 0)
		{
			acctext = XmStringCreateLocalized(string1byte(metaline));
			XtVaSetValues(wf->pulldownmenulist[i][pindex], XmNacceleratorText, acctext, NULL);
			XmStringFree(acctext);
		}
		XtManageChild(wf->pulldownmenulist[i][pindex]);
		XtAddCallback(wf->pulldownmenulist[i][pindex], XmNactivateCallback,
			(XtCallbackProc)gra_menucb, (XtPointer)pindex);
	}
}

/*
 * Routine to create a pulldown menu from popup menu "pm".
 * Returns an index to the table of pulldown menus (-1 on error).
 */
void gra_pulldownindex(WINDOWFRAME *wf, POPUPMENU *pm, INTBIG order, Widget parent)
{
	REGISTER INTBIG i, pindex, *newpulldownmenucount;
	Widget *newpulldownmenus, **newpulldownmenulist;
	CHAR **newpulldowns;

	/* see if it is in the list already */
	for(i=0; i<wf->pulldownmenucount; i++)
		if (namesame(wf->pulldowns[i], pm->name) == 0) return;

	/* allocate new space with one more */
	newpulldownmenus = (Widget *)emalloc((wf->pulldownmenucount+1) *
		(sizeof (Widget)), us_tool->cluster);
	if (newpulldownmenus == 0) return;
	newpulldowns = (CHAR **)emalloc((wf->pulldownmenucount+1) *
		(sizeof (CHAR *)), us_tool->cluster);
	if (newpulldowns == 0) return;
	newpulldownmenucount = (INTBIG *)emalloc((wf->pulldownmenucount+1) *
		SIZEOFINTBIG, us_tool->cluster);
	if (newpulldownmenucount == 0) return;
	newpulldownmenulist = (Widget **)emalloc((wf->pulldownmenucount+1) *
		(sizeof (Widget *)), us_tool->cluster);
	if (newpulldownmenulist == 0) return;

	/* copy former arrays then delete them */
	for(i=0; i<wf->pulldownmenucount; i++)
	{
		newpulldownmenus[i] = wf->pulldownmenus[i];
		newpulldowns[i] = wf->pulldowns[i];
		newpulldownmenucount[i] = wf->pulldownmenusize[i];
		newpulldownmenulist[i] = wf->pulldownmenulist[i];
	}
	if (wf->pulldownmenucount != 0)
	{
		efree((CHAR *)wf->pulldownmenus);
		efree((CHAR *)wf->pulldowns);
		efree((CHAR *)wf->pulldownmenusize);
		efree((CHAR *)wf->pulldownmenulist);
	}
	wf->pulldownmenus = newpulldownmenus;
	wf->pulldowns = newpulldowns;
	wf->pulldownmenusize = newpulldownmenucount;
	wf->pulldownmenulist = newpulldownmenulist;

	pindex = wf->pulldownmenucount++;
	wf->pulldownmenus[pindex] = parent;
	(void)allocstring(&wf->pulldowns[pindex], pm->name, us_tool->cluster);
	wf->pulldownmenusize[pindex] = pm->total;
	wf->pulldownmenulist[pindex] = (Widget *)emalloc(pm->total * (sizeof (Widget)),
		us_tool->cluster);
	if (wf->pulldownmenulist[pindex] == 0) return;

	gra_makepdmenu(wf, pm, order, parent, pindex);
}

/*
 * Routine to create pulldown menu number "value" from the popup menu in "pm" and return
 * the menu handle.
 */
Widget gra_makepdmenu(WINDOWFRAME *wf, POPUPMENU *pm, INTBIG value, Widget thismenu, INTBIG pindex)
{
	REGISTER INTBIG i, j, keysym, checked, haveacc, len;
	CHAR myline[256], metaline[50], *pt;
	CHAR mstring[2];
	REGISTER USERCOM *uc;
	REGISTER POPUPMENUITEM *mi;
	Widget submenu, *childlist;
	INTBIG special;
	INTSML key;
	Arg arg[5];
	XmString acctext;

	/* build the actual menu */
	childlist = wf->pulldownmenulist[pindex];
	mstring[1] = 0;
	for(i=0; i<pm->total; i++)
	{
		childlist[i] = 0;
		mi = &pm->list[i];

		/* see if there is a mnemonic */
		mstring[0] = 0;
		pt = myline;
		for(j=0; mi->attribute[j] != 0; j++)
		{
			if (mi->attribute[j] == '&') mstring[0] = mi->attribute[j+1]; else
				*pt++ = mi->attribute[j];
		}
		*pt = 0;

		uc = mi->response;
		if (uc->active < 0)
		{
			if (*mi->attribute == 0)
			{
				/* separator */
				childlist[i] = XtVaCreateManagedWidget(b_("sep"), xmSeparatorWidgetClass, thismenu, NULL);
			} else
			{
				/* dimmed item (DIM IT!!!) */
				childlist[i] = (Widget)XmCreatePushButton(thismenu, string1byte(myline), NULL, 0);
			}
			continue;
		}

		/* see if this command is another menu */
		if (uc->menu != NOPOPUPMENU)
		{
			submenu = (Widget)XmVaCreateSimplePulldownMenu(thismenu, b_("menu"), i,
				(XtCallbackProc)gra_menucb, NULL);
			if (submenu == 0) return(0);
			XtVaSetValues(submenu, XmNtearOffModel, XmTEAR_OFF_ENABLED, NULL);
			XtSetArg(arg[0], XmNsubMenuId, submenu);
			childlist[i] = XmCreateCascadeButton(thismenu, string1byte(myline), arg, 1);
			if (mstring[0] != 0)
			{
				keysym = XStringToKeysym(string1byte(mstring));
				XtVaSetValues(childlist[i], XmNmnemonic, keysym, NULL);
			}
			XtManageChild(childlist[i]);
			gra_pulldownindex(wf, uc->menu, i, submenu);
			continue;
		}

		/* see if there is a check */
		checked = -1;
		len = estrlen(myline) - 1;
		if (myline[len] == '<')
		{
			myline[len] = 0;
			if (myline[0] == '>')
			{
				checked = 1;
				estrcpy(myline, &myline[1]);
			} else
			{
				checked = 0;
			}
		}

		/* get command title and accelerator */
		len = estrlen(myline);
		haveacc = 0;
		for(pt = myline; *pt != 0; pt++) if (*pt == '/' || *pt == '\\') break;
		if (*pt != 0)
		{
			haveacc = 1;
			(void)us_getboundkey(pt, &key, &special);
			estrcpy(metaline, us_describeboundkey(key, special, 1));
			acctext = XmStringCreateLocalized(string1byte(metaline));
			*pt = 0;
		}
		if (checked == -1)
		{
			childlist[i] = (Widget)XmCreatePushButton(thismenu, string1byte(myline), NULL, 0);
		} else
		{
			childlist[i] = (Widget)XmCreateToggleButton(thismenu, string1byte(myline), NULL, 0);
		}
		if (mstring[0] != 0)
			XtVaSetValues(childlist[i], XmNmnemonic, XStringToKeysym(string1byte(mstring)), NULL);
		XtManageChild(childlist[i]);
		if (checked == 0) XmToggleButtonSetState(childlist[i], False, 0); else
			if (checked == 1) XmToggleButtonSetState(childlist[i], True, 0);
		if (haveacc != 0)
		{
			XtVaSetValues(childlist[i], XmNacceleratorText, acctext, NULL);
			XmStringFree(acctext);
		}
		if (checked == -1)
		{
			XtAddCallback(childlist[i], XmNactivateCallback,
				(XtCallbackProc)gra_menucb, (XtPointer)i);
		} else
		{
			XtAddCallback(childlist[i], XmNvalueChangedCallback,
				(XtCallbackProc)gra_menucb, (XtPointer)i);
		}
	}
	return(thismenu);
}

void gra_menucb(Widget w, int item_number, XtPointer call_data)
{
	Widget parent;
	INTBIG i;
	REGISTER WINDOWFRAME *wf;

	parent = XtParent(w);
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;

		for(i=0; i<wf->pulldownmenucount; i++)
		{
			if (wf->pulldownmenus[i] != parent) continue;
			gra_nativemenudoone(wf, i, item_number);
			return;
		}
	}
}

void gra_menupcb(Widget w, int item_number, XtPointer call_data)
{
	gra_popupmenuresult = item_number;
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

/* Internal routine to initialize dialog structure "dialog" into "dia" */
BOOLEAN gra_initdialog(DIALOG *dialog, TDIALOG *dia, BOOLEAN modeless)
{
	Widget temp, focus, base, vertscroll;
	Arg arg[20];
	CHAR *title, *line;
	CHAR1 **fontnames;
	INTBIG i, j, amt, itemtype, x, y, wid, hei;
	int ac, direction, asc, desc;
	XmString str;
	Display *dpy;
	static XFontStruct *font;
	static INTBIG founddialogfont = 0;
	XFontStruct **fontstructs;
	XCharStruct xcs;
	Window win;
	Pixmap map;
	String translation;
	XtActionsRec actions;
	XmFontList xfontlist;
	XmFontContext fontContext;
	XmFontListEntry fontListEntry;
	XmFontType fonttype;
	GC gc;
	XGCValues gcv;
	static INTBIG userdrawactionset = 0, listcopyactionset = 0;
	REGISTER void *infstr;
	XColor col;

	/* add this to the list of active dialogs */
	dia->nexttdialog = gra_firstactivedialog;
	gra_firstactivedialog = dia;

	/* be sure the dialog is translated */
	DiaTranslate(dialog);

	/* get the current dialog structure */
	if (el_curwindowpart == NOWINDOWPART) base = gra_msgtoplevelwidget; else
		base = el_curwindowpart->frame->toplevelwidget;

	dpy = XtDisplay(base);
	gra_dialoggraycol = gra_makedpycolor(dpy, 128, 128, 128);

	dia->windindex = gra_windownumberindex++;
	dia->numlocks = 0;
	dia->modelessitemhit = 0;

	/* compute size of some items to automatically scale them */
	if (founddialogfont == 0)
	{
		founddialogfont = 1;
		XtSetArg(arg[0], XmNbuttonFontList, &xfontlist);
		XtGetValues(base, arg, 1);
		XmFontListInitFontContext(&fontContext, xfontlist);
		font = 0;
		for(;;)
		{
			fontListEntry = XmFontListNextEntry(fontContext);
			if (fontListEntry == 0) break;
			font = (XFontStruct *)XmFontListEntryGetFont(fontListEntry, &fonttype);
			if (fonttype == XmFONT_IS_FONT) break;
			i = XFontsOfFontSet((XFontSet)font, &fontstructs, &fontnames);
			if (i > 0)
			{
				font = fontstructs[0];
				break;
			}
			font = 0;
		}
		XmFontListFreeFontContext(fontContext);
	}
	if (font != 0)
	{
#ifdef INTERNATIONAL
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
					line = dialog->list[i].msg;
					XTextExtents(font, string1byte(line), estrlen(line), &direction, &asc, &desc, &xcs);
					j = xcs.width;
					amt = j - (dialog->list[i].r.right - dialog->list[i].r.left);
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
#endif
		for(i=0; i<dialog->items; i++)
		{
			j = dialog->list[i].r.right + 9;
			if (j < dialog->windowRect.right - dialog->windowRect.left) continue;
			dialog->windowRect.right = dialog->windowRect.left + j;
		}
	}

	/* create the dialog */
	title = dialog->movable;
	if (title == 0 || *title == 0) title = x_(" ");
	gra_getdialogcoordinates(&dialog->windowRect, &x, &y, &wid, &hei);
	ac = 0;
	if (modeless)
	{
		XtSetArg(arg[ac], XmNdialogStyle, XmDIALOG_MODELESS);   ac++;
	} else
	{
		XtSetArg(arg[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);   ac++;
	}
	XtSetArg(arg[ac], XmNx,               x);   ac++;
	XtSetArg(arg[ac], XmNy,               y);   ac++;
	XtSetArg(arg[ac], XmNwidth,           wid);   ac++;
	XtSetArg(arg[ac], XmNheight,          hei);   ac++;
	str = XmStringCreateLocalized(string1byte(title));
	XtSetArg(arg[ac], XmNdialogTitle,     str);   ac++;
	XtSetArg(arg[ac], XmNautoUnmanage,    False);   ac++;
	XtSetArg(arg[ac], XmNmarginWidth,     0);   ac++;
	XtSetArg(arg[ac], XmNmarginHeight,    0);   ac++;
	XtSetArg(arg[ac], XmNallowOverlap,    True);   ac++;
	XtSetArg(arg[ac], XmNresizePolicy,    XmRESIZE_NONE);   ac++;
	XtSetArg(arg[ac], XmNdeleteResponse,  XmDO_NOTHING);   ac++;
	XtSetArg(arg[ac], XmNmwmFunctions,    MWM_FUNC_MOVE);   ac++;
	XtSetArg(arg[ac], XmNdefaultPosition, False);   ac++;
	dia->window = (Widget)XmCreateBulletinBoardDialog(base, b_("dialog"), arg, ac);
	dia->redrawroutine = 0;
	dia->itemdesc = dialog;
	dia->opaqueitem = -1;
	dia->usertextsize = 8;
	dia->inix = dia->iniy = -1;
	XtManageChild(dia->window);
	XtAddEventHandler(dia->window, FocusChangeMask,
		FALSE, gra_dialog_event_handler, NULL);

	gcv.foreground = 0;
	gra_gcdia = XtGetGC(dia->window, GCForeground, &gcv);	

	/* determine the default button */
	dia->defaultbutton = 1;
	for(i=0; i<dialog->items; i++)
	{
		itemtype = dialog->list[i].type;
		if ((itemtype&ITEMTYPE) == DEFBUTTON)
			dia->defaultbutton = i+1;
	}

	/* load the items */
	focus = 0;
	for(i=0; i<dialog->items; i++)
	{
		gra_getdialogcoordinates(&dialog->list[i].r, &x, &y, &wid, &hei);
		itemtype = dialog->list[i].type;
		switch (itemtype&ITEMTYPE)
		{
			case RADIO:				/* radio buttons seem too low, so raise them */
				y -= 5;
				break;
			case EDITTEXT:			/* raise text and force it to minimum height */
				y -= 5;
				if (hei < 32) hei = 32;
				break;
			case SCROLL:			/* shrink to make room for vertical scrollbar */
			case SCROLLMULTI:
				wid -= 20;
				break;
			case POPUP:				/* popups seem too low, so raise them */
				y -= 7;
				hei += 7;
				break;
		}
		ac = 0;
		XtSetArg(arg[ac], XmNx, x);   ac++;
		XtSetArg(arg[ac], XmNy, y);   ac++;
		XtSetArg(arg[ac], XmNwidth, wid);   ac++;
		XtSetArg(arg[ac], XmNheight, hei);   ac++;
		switch (itemtype&ITEMTYPE)
		{
			case BUTTON:
			case DEFBUTTON:
				dia->items[i] = (Widget)XmCreatePushButton(dia->window,
					string1byte(dialog->list[i].msg), arg, ac);
				XtManageChild(dia->items[i]);
				XtAddCallback(dia->items[i], XmNactivateCallback,
					(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				break;
			case CHECK:
				XtSetArg(arg[ac], XmNalignment, XmALIGNMENT_BEGINNING);   ac++;
				dia->items[i] = (Widget)XmCreateToggleButton(dia->window,
					dialog->list[i].msg, arg, ac);
				XtManageChild(dia->items[i]);
				XtAddCallback(dia->items[i], XmNvalueChangedCallback,
					(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				break;
			case RADIO:
				temp = (Widget)XmCreateRadioBox(dia->window, b_("radio"), arg, ac);
				XtManageChild(temp);
				dia->items[i] = XtVaCreateManagedWidget(string1byte(dialog->list[i].msg),
					xmToggleButtonGadgetClass, temp, XmNindicatorSize, hei-8, NULL);
				XtAddCallback(dia->items[i], XmNvalueChangedCallback,
					(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				break;
			case EDITTEXT:
				dia->items[i] = (Widget)XmCreateText(dia->window, b_("text"),
					arg, ac);
				XtManageChild(dia->items[i]);
				XtAddCallback(dia->items[i], XmNvalueChangedCallback,
					(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				XtVaGetValues(dia->items[i], XmNbackground, &col, NULL);
				gra_dialogbackgroundcol = col.pixel;
				if (focus == 0) focus = dia->items[i];
				break;
			case MESSAGE:
				XtSetArg(arg[ac], XmNalignment, XmALIGNMENT_BEGINNING);   ac++;
				dia->items[i] = (Widget)XmCreateLabel(dia->window,
					dialog->list[i].msg, arg, ac);
				XtManageChild(dia->items[i]);
				break;
			case PROGRESS:
				dia->items[i] = (Widget)XmCreateDrawingArea(dia->window,
					b_("user"), arg, ac);
				XtManageChild(dia->items[i]);
				break;
			case DIVIDELINE:
				dia->items[i] = (Widget)XmCreateDrawingArea(dia->window,
					b_("user"), arg, ac);
				XtManageChild(dia->items[i]);
				XtAddCallback(dia->items[i], XmNexposeCallback,
					(XtCallbackProc)gra_dialogredrawsep, (XtPointer)i);
				dpy = XtDisplay(dia->items[i]);
				win = XtWindow(dia->items[i]);
				gcv.foreground = BlackPixelOfScreen(XtScreen(dia->items[i]));
				gc = XtGetGC(dia->items[i], GCForeground, &gcv);
				XSetForeground(dpy, gc, BlackPixelOfScreen(XtScreen(dia->items[i])));
				XFillRectangle(dpy, win, gc, 0, 0, wid, hei);
				XtReleaseGC(dia->items[i], gc);
				break;
			case USERDRAWN:
				if (userdrawactionset == 0)
				{
					actions.string = b_("draw");
					actions.proc = (XtActionProc)gra_dialogdraw;
					XtAppAddActions(gra_xtapp, &actions, 1);
					userdrawactionset = 1;
				}
				translation = b_("<BtnDown>: draw(down) ManagerGadgetArm()\n<BtnUp>: draw(up) ManagerGadgetActivate()\n<BtnMotion>: draw(motion) ManagerGadgetButtonMotion()");
				XtSetArg(arg[ac], XmNtranslations, XtParseTranslationTable(translation));   ac++;
				dia->items[i] = (Widget)XmCreateDrawingArea(dia->window,
					b_("user"), arg, ac);
				XtManageChild(dia->items[i]);
				XtAddCallback(dia->items[i], XmNexposeCallback,
					(XtCallbackProc)gra_dialogredraw, (XtPointer)i);
				break;
			case POPUP:
				XtSetArg(arg[ac], XmNresizeWidth, False);   ac++;
				XtSetArg(arg[ac], XmNresizeHeight, False);   ac++;
				dia->items[i] = (Widget)XmCreateOptionMenu(dia->window,
					b_("menu"), arg, ac);
				dialog->list[i].data = 0;
				XtAddCallback(dia->items[i], XmNentryCallback,
					(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				break;
			case ICON:
				map = gra_makeicon((UCHAR1 *)dialog->list[i].msg, dia->window);
				if (map == 0) break;
				if ((itemtype&INACTIVE) != 0)
				{
					/* inactive: make it a label */
					XtSetArg(arg[ac], XmNlabelType, XmPIXMAP);   ac++;
					XtSetArg(arg[ac], XmNlabelPixmap, map);   ac++;
					dia->items[i] = (Widget)XmCreateLabel(dia->window,
						dialog->list[i].msg, arg, ac);
				} else
				{
					/* active: make it a pushbutton */
					XtSetArg(arg[ac], XmNlabelType, XmPIXMAP);   ac++;
					XtSetArg(arg[ac], XmNlabelPixmap, map);   ac++;
					XtSetArg(arg[ac], XmNhighlightThickness, 0);   ac++;
					XtSetArg(arg[ac], XmNshadowThickness, 0);   ac++;
					XtSetArg(arg[ac], XmNborderWidth, 0);   ac++;
					dia->items[i] = (Widget)XmCreatePushButton(dia->window,
						string1byte(dialog->list[i].msg), arg, ac);
					XtManageChild(dia->items[i]);
					XtAddCallback(dia->items[i], XmNactivateCallback,
						(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				}
				XtManageChild(dia->items[i]);
				break;
			case SCROLL:
			case SCROLLMULTI:
				if (listcopyactionset == 0)
				{
					actions.string = b_("copywholelist");
					actions.proc = (XtActionProc)gra_dialogcopywholelist;
					XtAppAddActions(gra_xtapp, &actions, 1);
					listcopyactionset = 1;
				}
				translation = b_("#override <Key>osfCopy: copywholelist()");
				XtSetArg(arg[ac], XmNtranslations, XtParseTranslationTable(translation));   ac++;
				if ((itemtype&ITEMTYPE) == SCROLL)
				{
					XtSetArg(arg[ac], XmNselectionPolicy, XmBROWSE_SELECT);
				} else
				{
					XtSetArg(arg[ac], XmNselectionPolicy, XmEXTENDED_SELECT);
				}
				ac++;
				XtSetArg(arg[ac], XmNresizePolicy, XmRESIZE_NONE);   ac++;
				XtSetArg(arg[ac], XmNlistSizePolicy, XmCONSTANT);   ac++;
				dia->items[i] = (Widget)XmCreateScrolledList(dia->window,
					b_("scroll"), arg, ac);
				XtManageChild(dia->items[i]);

				/* get the vertical scroll bar out of the widget */
				XtSetArg(arg[0], XmNverticalScrollBar, &vertscroll);
				XtGetValues(dia->items[i], arg, 1);
				XtAddCallback(vertscroll, XmNvalueChangedCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);
				XtAddCallback(vertscroll, XmNdecrementCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);
				XtAddCallback(vertscroll, XmNincrementCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);
				XtAddCallback(vertscroll, XmNpageIncrementCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);
				XtAddCallback(vertscroll, XmNpageDecrementCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);
				XtAddCallback(vertscroll, XmNtoTopCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);
				XtAddCallback(vertscroll, XmNtoBottomCallback,
					(XtCallbackProc)gra_vertslider, (XtPointer)i);

				if ((itemtype&ITEMTYPE) == SCROLL)
				{
#if 0
					XtAddCallback(dia->items[i], XmNbrowseSelectionCallback,
						(XtCallbackProc)gra_dialogaction, (XtPointer)i);
#endif
				} else
				{
					XtAddCallback(dia->items[i], XmNextendedSelectionCallback,
						(XtCallbackProc)gra_dialogaction, (XtPointer)i);
				}
				break;
			default:
				break;
		}
	}

	/* set focus to first edit text field */
	if (focus != 0)
	{
		XtSetArg(arg[0], XmNinitialFocus, focus);
		XtSetValues(dia->window, arg, 1);
	}

	/* set default button if appropriate */
	itemtype = dialog->list[dia->defaultbutton-1].type;
	if ((itemtype&ITEMTYPE) == BUTTON || (itemtype&ITEMTYPE) == DEFBUTTON)
	{
		temp = dia->items[dia->defaultbutton-1];
		if (focus == 0)
		{
			XtSetArg(arg[0], XmNinitialFocus, temp);
			XtSetValues(dia->window, arg, 1);
		}
		XtSetArg(arg[0], XmNdefaultButton, temp);
		XtSetValues(dia->window, arg, 1);
		XtSetArg(arg[0], XmNshowAsDefault, 1);
		XtSetValues(temp, arg, 1);
	}

	if (*title == 0) title = _("UNTITLED");
	infstr = initinfstr();
	formatinfstr(infstr, _("Start dialog %s"), title);
	gra_logwritecomment(returninfstr(infstr));
	dia->dialoghit = -1;
	gra_flushdialog(dia);
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
	Arg arg[3];
	INTBIG dx, dy;
	short newx, newy;
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

	/* adjust the dialog position if it was moved */
	if (dia->inix != -1 && dia->iniy != -1)
	{
		XtSetArg(arg[0], XtNx, &newx);
		XtSetArg(arg[1], XtNy, &newy);
		XtGetValues(dia->window, arg, 2);
		dx = (newx - dia->inix) * DIALOGDEN / DIALOGNUM;
		dy = (newy - dia->iniy) * DIALOGDEN / DIALOGNUM;
		dia->itemdesc->windowRect.left += dx;
		dia->itemdesc->windowRect.right += dx;
		dia->itemdesc->windowRect.top += dy;
		dia->itemdesc->windowRect.bottom += dy;
	}

	XtUnmanageChild(dia->window);
	XtDestroyWidget(dia->window);

	/* free the "dia" structure */
	efree((CHAR *)dia);
}

/*
 * Routine to change the size of the dialog
 */
void DiaResizeDialog(void *vdia, INTBIG wid, INTBIG hei)
{
	TDIALOG *dia;
	Widget topwindow;

	dia = (TDIALOG *)vdia;
	wid = gra_scaledialogcoordinate(wid);
	hei = gra_scaledialogcoordinate(hei);
	topwindow = XtParent(dia->window);

	XtResizeWidget(topwindow, wid, hei, 0);
	gra_flushdialog(dia);
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
	Arg arg[5];
	INTBIG type, highlight;
	XmString str, s1, s2;
	static XmString separator = 0;
	Widget w;
	INTBIG bigitem, count, i, textwid, x, y, itemwid, itemhei, len;
	int direction, asc, desc;
	XmFontList fontlist;
	XmFontListEntry fontentry;
	XmFontContext fontlistcontext;
	XmFontType entrytype;
	XFontSet fontset;
	XtPointer retval;
	CHAR1 **fontnamelist;
	CHAR *pt;
	XFontStruct **fontstructlist;
	XCharStruct xcs;
	static XFontStruct *theFont = 0;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	highlight = 0;
	if (item < 0)
	{
		item = -item;
		highlight = 1;
	}
	item--;
	w = dia->items[item];
	type = dia->itemdesc->list[item].type;
	if ((type&ITEMTYPE) == EDITTEXT)
	{
		bigitem = (INTBIG)item;
		XtRemoveCallback(w, XmNvalueChangedCallback,
			(XtCallbackProc)gra_dialogaction, (XtPointer)bigitem);
		XmTextSetString(w, string1byte(msg));
		count = XmTextGetLastPosition(w);
		if (highlight != 0)
		{
			XmTextSetSelection(w, 0, count, 0);
			XtSetArg(arg[0], XmNinitialFocus, w);
			XtSetValues(dia->window, arg, 1);
		} else
		{
			XmTextSetSelection(w, count, count, 0);
			XmTextSetInsertionPosition(w, count); 
		}
		XtAddCallback(w, XmNvalueChangedCallback,
			(XtCallbackProc)gra_dialogaction, (XtPointer)bigitem);
	} else
	{
		/* cache the font used to draw messages (once only) */
		if (theFont == 0)
		{
			XtSetArg(arg[0], XmNfontList, &fontlist);
			XtGetValues(w, arg, 1);
			if (XmFontListInitFontContext(&fontlistcontext, fontlist))
			{
				fontentry = XmFontListNextEntry(fontlistcontext);
				if (fontentry != 0)
				{
					retval = XmFontListEntryGetFont(fontentry, &entrytype);
					if (entrytype == XmFONT_IS_FONT)
					{
						theFont = (XFontStruct *)retval;
					} else if (entrytype == XmFONT_IS_FONTSET)
					{
						fontset = (XFontSet)retval;
						count = XFontsOfFontSet(fontset, &fontstructlist, &fontnamelist);
						if (count > 0)
							theFont = fontstructlist[0];
					}
				}
				XmFontListFreeFontContext(fontlistcontext);
			}
		}

		/* see if the text fits */
		if (theFont == 0) textwid = itemwid = 10; else
		{
			XTextExtents(theFont, string1byte(msg), estrlen(msg), &direction, &asc, &desc, &xcs);
			textwid = xcs.width;
			gra_getdialogcoordinates(&dia->itemdesc->list[item].r, &x, &y, &itemwid, &itemhei);
		}

		if (textwid <= itemwid) str = XmStringCreateLocalized(string1byte(msg)); else
		{
			/* break up the text */
			len = estrlen(msg)+1;
			if (len > gra_brokenbufsize)
			{
				if (gra_brokenbufsize > 0) efree(gra_brokenbuf);
				gra_brokenbufsize = 0;
				gra_brokenbuf = (CHAR *)emalloc(len*SIZEOFCHAR, us_tool->cluster);
				if (gra_brokenbuf == 0) return;
				gra_brokenbufsize = len;
			}
			if (separator == 0) separator = XmStringSeparatorCreate();
			str = XmStringCreateLocalized(b_(""));
			pt = msg;
			for(;;)
			{
				/* look backwards for blank space that fits */
				for(len = estrlen(pt); len > 0; len--)
				{
					if (pt[len] != ' ' && pt[len] != 0) continue;
					XTextExtents(theFont, string1byte(pt), len, &direction, &asc, &desc, &xcs);
					if (xcs.width <= itemwid) break;
				}
				if (len == 0)
				{
					/* no place to break in blank space: break where possible */
					for(len = estrlen(pt)-1; len > 0; len--)
					{
						XTextExtents(theFont, string1byte(pt), len, &direction, &asc, &desc, &xcs);
						if (xcs.width <= itemwid) break;
					}
				}

				for(i=0; i<len; i++) gra_brokenbuf[i] = pt[i];
				gra_brokenbuf[len] = 0;
				s1 = XmStringCreateLocalized(string1byte(gra_brokenbuf));
				s2 = XmStringConcat(str, s1);
				XmStringFree(s1);
				XmStringFree(str);
				str = s2;
				if (len == estrlen(pt)) break;
				s2 = XmStringConcat(str, separator);
				XmStringFree(str);
				str = s2;
				pt = &pt[len];
				if (*pt == ' ') pt++;
			}
		}
		XtSetArg(arg[0], XmNlabelString, str);
		XtSetValues(w, arg, 1);
		XmStringFree(str);
	}
	gra_flushdialog(dia);
}

/*
 * Routine to return the text in item "item"
 */
CHAR *DiaGetText(void *vdia, INTBIG item)
{
	Arg arg;
	INTBIG type;
	XmString str;
	CHAR1 *text;
	CHAR *retstring;
	REGISTER void *infstr;
	XmStringContext context;
	XmStringCharSet tag;
	XmStringDirection direction;
	Boolean separator;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (dia->opaqueitem == item)
		return(dia->opaquefield);

	type = dia->itemdesc->list[item].type;
	if ((type&ITEMTYPE) == EDITTEXT)
	{
#ifdef _UNICODE
		static CHAR retstring[300];

		estrcpy(retstring, string2byte(XmTextGetString(dia->items[item])));
		return(retstring);
#else
		return(XmTextGetString(dia->items[item]));
#endif
	}

	if ((type&ITEMTYPE) == MESSAGE)
	{
		XtSetArg(arg, XmNlabelString, &str);
		XtGetValues(dia->items[item], &arg, 1);
		infstr = initinfstr();
		XmStringInitContext(&context, str);
		for(;;)
		{
			if (!XmStringGetNextSegment(context, &text, &tag, &direction, &separator)) break;
			addstringtoinfstr(infstr, string2byte(text));
			XtFree(text);
		}
		XmStringFreeContext(context);
		retstring = returninfstr(infstr);
		return(retstring);
	}
	return(x_(""));
}

/*
 * Routine to set the value in item "item" to "value"
 */
void DiaSetControl(void *vdia, INTBIG item, INTBIG value)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	XmToggleButtonSetState(dia->items[item], value, False);
	gra_flushdialog(dia);
	gra_logwriteaction(DIASETCONTROL, dia->windindex, item + 1, value, 0);
}

/*
 * Routine to return the value in item "item"
 */
INTBIG DiaGetControl(void *vdia, INTBIG item)
{
	INTBIG value;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	value = XmToggleButtonGetState(dia->items[item]);
	return(value);
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
	REGISTER INTBIG type;
	Arg arg[2];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	type = dia->itemdesc->list[item].type;
	if ((type&ITEMTYPE) == EDITTEXT)
	{
		DiaNoEditControl(dia, item+1);
		XtSetArg(arg[0], XtNbackground, gra_dialoggraycol);
		XtSetValues(dia->items[item], arg, 1);
	} else
		XtSetSensitive(dia->items[item], False);
	gra_flushdialog(dia);
}

/*
 * Routine to un-dim item "item"
 */
void DiaUnDimItem(void *vdia, INTBIG item)
{
	REGISTER INTBIG type;
	Arg arg[2];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	type = dia->itemdesc->list[item].type;
	if ((type&ITEMTYPE) == EDITTEXT)
	{
		DiaEditControl(dia, item+1);
		XtSetArg(arg[0], XtNbackground, gra_dialogbackgroundcol);
		XtSetValues(dia->items[item], arg, 1);
	} else
		XtSetSensitive(dia->items[item], True);
	gra_flushdialog(dia);
}

/*
 * Routine to change item "item" to be a message rather
 * than editable text
 */
void DiaNoEditControl(void *vdia, INTBIG item)
{
	Widget w;
	Arg arg;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	XtSetArg(arg, XmNeditable, False);
	XtSetValues(w, &arg, 1);
}

/*
 * Routine to change item "item" to be editable text rather
 * than a message
 */
void DiaEditControl(void *vdia, INTBIG item)
{
	Widget w;
	Arg arg;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	XtSetArg(arg, XmNeditable, True);
	XtSetValues(w, &arg, 1);
}

void DiaOpaqueEdit(void *vdia, INTBIG item)
{
	Widget w;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	dia->opaqueitem = item;

	w = dia->items[item];
	dia->opaquefield[0] = 0;
	XtAddCallback(w, XmNmodifyVerifyCallback, (XtCallbackProc)gra_dialogopaqueaction, 0);
}

/*
 * Routine to cause item "item" to be the default button
 */
void DiaDefaultButton(void *vdia, INTBIG item)
{
	Widget newdef, olddef;
	Arg arg[2];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	if (item == dia->defaultbutton) return;
	olddef = dia->items[dia->defaultbutton-1];
	newdef = dia->items[item-1];
	dia->defaultbutton = item;

	XtSetArg(arg[0], XmNdefaultButton, newdef);
	XtSetValues(dia->window, arg, 1);
	XtSetArg(arg[0], XmNshowAsDefault, 0);
	XtSetValues(olddef, arg, 1);
	XtSetArg(arg[0], XmNshowAsDefault, 1);
	XtSetValues(newdef, arg, 1);
}

/*
 * Routine to change the icon in item "item" to be the 32x32 bitmap (128 bytes) at "addr".
 */
void DiaChangeIcon(void *vdia, INTBIG item, UCHAR1 *addr)
{
	Widget w;
	Pixmap map;
	Arg arg;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	map = gra_makeicon(addr, dia->window);
	if (map == 0) return;
	XtSetArg(arg, XmNlabelPixmap, map);
	XtSetValues(w, &arg, 1);
}

/*
 * Routine to change item "item" into a popup with "count" entries
 * in "names".
 */
void DiaSetPopup(void *vdia, INTBIG item, INTBIG count, CHAR **names)
{
	Widget child, menu;
	Arg arg[3];
	INTBIG i, x, y, wid, hei, columns, entriespercolumn;
	short entryheight;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	gra_getdialogcoordinates(&dia->itemdesc->list[item].r, &x, &y, &wid, &hei);
	XtSetArg(arg[0], XmNpacking, XmPACK_COLUMN);
	menu = (Widget)XmCreatePulldownMenu(dia->window, b_("menu"), arg, 1);
	if (count == 0)
	{
		child = (Widget)XmCreatePushButton(menu, b_(""), NULL, 0);
		XtManageChild(child);
	}
	for(i=0; i<count; i++)
	{
		XtSetArg(arg[0], XmNwidth, wid);
		child = (Widget)XmCreatePushButton(menu, string1byte(names[i]), arg, 1);
		XtManageChild(child);
		XtAddCallback(child, XmNactivateCallback,
			(XtCallbackProc)gra_dialogaction, (XtPointer)item);

		/* adjust the number of columns if necessary */
		if (i == 0)
		{
			XtSetArg(arg[0], XtNheight, &entryheight);
			XtGetValues(menu, arg, 1);
			entriespercolumn = gra_shortestscreen / entryheight;
			columns = (count + entriespercolumn-1) / entriespercolumn;
			if (columns > 1)
			{
				XtSetArg(arg[0], XmNnumColumns, columns);
				XtSetValues(menu, arg, 1);
			}
		}
	}
	XtSetArg(arg[0], XmNsubMenuId, menu);
	XtSetValues(dia->items[item], arg, 1);

	if (dia->itemdesc->list[item].data == 0)
	{
		dia->itemdesc->list[item].data = 1;
		XtManageChild(dia->items[item]);
	}

	/* force the width of the popup */
	XtSetArg(arg[0], XmNwidth, wid);
	XtSetValues(dia->items[item], arg, 1);
	gra_flushdialog(dia);
}

/*
 * Routine to change popup item "item" so that the current entry is "entry".
 */
void DiaSetPopupEntry(void *vdia, INTBIG item, INTBIG entry)
{
	Widget w, child, *kids;
	Arg arg[1];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	XtVaGetValues(w, XmNsubMenuId, &child, NULL);
	XtVaGetValues(child, XmNchildren, &kids, NULL);
	XtSetArg(arg[0], XmNmenuHistory, kids[entry]);
	XtSetValues(w, arg, 1);
	gra_flushdialog(dia);
}

/*
 * Routine to return the current item in popup menu item "item".
 */
INTBIG DiaGetPopupEntry(void *vdia, INTBIG item)
{
	Widget w, child, *kids, kid;
	INTBIG numkids, i;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	XtVaGetValues(w, XmNsubMenuId, &child, NULL);
	XtVaGetValues(child, XmNchildren, &kids, NULL);
	XtVaGetValues(child, XmNnumChildren, &numkids, NULL);
	XtVaGetValues(w, XmNmenuHistory, &kid, NULL);
	for(i=0; i<numkids; i++)
		if (kids[i] == kid) break;
	return(i);
}

void DiaInitTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos, INTBIG flags)
{
	Widget w;
	INTBIG bigitem, lines, fontheight;
	Arg arg[2];
	Display *dpy;
	XmFontListEntry entry;
	static XmFontList fontlist;
	static INTBIG havefontlist = 0;
	RECTAREA r;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	if ((flags&SCDOUBLEQUIT) != 0)
	{
		XtAddCallback(w, XmNdefaultActionCallback,
			(XtCallbackProc)gra_dialogaction, (XtPointer)0);
	}
	if ((flags&SCREPORT) != 0)
	{
		bigitem = (INTBIG)item;
		XtAddCallback(w, XmNbrowseSelectionCallback,
			(XtCallbackProc)gra_dialogaction, (XtPointer)bigitem);
	}
	if ((flags&SCFIXEDWIDTH) != 0)
	{
		if (havefontlist == 0)
		{
			dpy = XtDisplay(w);
			entry = XmFontListEntryLoad(dpy, string1byte(gra_font[11].fontname),
				XmFONT_IS_FONT, b_("TAG"));
			fontlist = XmFontListAppendEntry(NULL, entry);
			XmFontListEntryFree(&entry);
			havefontlist = 1;
		}
		XtSetArg(arg[0], XmNfontList, fontlist);
		fontheight = gra_font[11].font->ascent + gra_font[11].font->descent;
		r = dia->itemdesc->list[item].r;
		lines = (r.bottom - r.top) / fontheight;
		XtSetArg(arg[1], XmNvisibleItemCount, lines);
		XtSetValues(w, arg, 2);
	}
	gra_flushdialog(dia);
	DiaLoadTextDialog(dia, item+1, toplist, nextinlist, donelist, sortpos);
}

void DiaLoadTextDialog(void *vdia, INTBIG item, BOOLEAN (*toplist)(CHAR **), CHAR *(*nextinlist)(void),
	void (*donelist)(void), INTBIG sortpos)
{
	Widget w;
	INTBIG i, items;
	CHAR *next, **list, line[256];
	XmString str;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];

	/* clear the list */
	XmListDeleteAllItems(w);

	if (sortpos < 0)
	{
		/* unsorted: load the list directly */
		line[0] = 0;
		next = line;
		(void)(*toplist)(&next);
		for(items=0; ; items++)
		{
			next = (*nextinlist)();
			if (next == 0) break;
			str = XmStringCreateLocalized(string1byte(next));
			XmListAddItemUnselected(w, str, 0);
			XmStringFree(str);
		}
		(*donelist)();
	} else
	{
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
		gra_dialogstringpos = sortpos;
		esort(list, items, sizeof (CHAR *), gra_stringposascending);

		/* stuff the list into the text editor */
		for(i=0; i<items; i++)
		{
			str = XmStringCreateLocalized(string1byte(list[i]));
			XmListAddItemUnselected(w, str, 0);
			XmStringFree(str);
		}

		/* deallocate the list */
		if (items > 0)
		{
			for(i=0; i<items; i++) efree((CHAR *)list[i]);
			efree((CHAR *)(CHAR *)list);
		}
	}
	if (items > 0) XmListSelectPos(w, 1, False);
	gra_flushdialog(dia);
}

/*
 * Routine to stuff line "line" at the end of the edit buffer.
 */
void DiaStuffLine(void *vdia, INTBIG item, CHAR *line)
{
	Widget w;
	XmString str;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	str = XmStringCreateLocalized(string1byte(line));
	XmListAddItemUnselected(w, str, 0);
	XmStringFree(str);
	gra_flushdialog(dia);
}

/*
 * Routine to select line "line" of scroll item "item".
 */
void DiaSelectLine(void *vdia, INTBIG item, INTBIG line)
{
	Widget w;
	INTBIG first, visible;
	Arg arg[2];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	if (line < 0) XmListDeselectAllItems(w); else
	{
		line++;
		XmListSelectPos(w, line, False);
		XtSetArg(arg[0], XmNtopItemPosition, &first);
		XtSetArg(arg[1], XmNvisibleItemCount, &visible);
		XtGetValues(w, arg, 2);
		if (line < first || line >= first+visible)
		{
			first = line - visible/2;
			if (first < 1) first = 1;
			XmListSetPos(w, first);
		}
	}
	gra_flushdialog(dia);
}

/*
 * Routine to select "count" lines in "lines" of scroll item "item".
 */
void DiaSelectLines(void *vdia, INTBIG item, INTBIG count, INTBIG *lines)
{
	Widget w;
	INTBIG first, visible, i, low, high, line;
	Arg arg[2];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	XmListDeselectAllItems(w);

	/* temporarily set the selection policy to "multiple" to enable multiple select */
	XtSetArg(arg[0], XmNselectionPolicy, XmMULTIPLE_SELECT);
	XtSetValues(w, arg, 1);

	/* make the selections */
	for(i=0; i<count; i++)
	{
		line = lines[i] + 1;
		XmListSelectPos(w, line, False);
		if (i == 0) low = high = line; else
		{
			if (line < low) low = line;
			if (line > high) high = line;
		}
	}

	/* make sure lines are visible */
	XtSetArg(arg[0], XmNtopItemPosition, &first);
	XtSetArg(arg[1], XmNvisibleItemCount, &visible);
	XtGetValues(w, arg, 2);
	if (high < first || low >= first+visible)
	{
		first = low;
		XmListSetPos(w, first);
	}

	/* restore the selection policy to "extended" */
	XtSetArg(arg[0], XmNselectionPolicy, XmEXTENDED_SELECT);
	XtSetValues(w, arg, 1);
	gra_flushdialog(dia);
}

/*
 * Returns the currently selected line in the scroll list "item".
 */
INTBIG DiaGetCurLine(void *vdia, INTBIG item)
{
	Widget w;
	INTBIG selected;
	int count, *pos;   /* must be "int", and not "INTBIG" to work on Alpha */
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	selected = -1;
	if (XmListGetSelectedPos(w, &pos, &count))
	{
		if (count >= 1) selected = pos[0]-1;
		XtFree((char*)pos);
	}
	return(selected);
}

/*
 * Returns the currently selected lines in the scroll list "item".  The returned
 * array is terminated with -1.
 */
INTBIG *DiaGetCurLines(void *vdia, INTBIG item)
{
	Widget w;
	static INTBIG selected[MAXSCROLLMULTISELECT];
	REGISTER INTBIG i;
	int count, *pos;   /* must be "int", and not "INTBIG" to work on Alpha */
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	selected[0] = -1;
	if (XmListGetSelectedPos(w, &pos, &count))
	{
		for(i=0; i<count; i++)
			selected[i] = pos[i] - 1;
		selected[count] = -1;
		XtFree((char *)pos);
	}
	return(selected);
}

INTBIG DiaGetNumScrollLines(void *vdia, INTBIG item)
{
	Widget w;
	INTBIG count;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	XtVaGetValues(w, XmNitemCount, &count, NULL);
	return(count);
}

CHAR *DiaGetScrollLine(void *vdia, INTBIG item, INTBIG line)
{
	Widget w;
	INTBIG count;
	XmString *strlist;
	CHAR1 *text;
	CHAR *retstring;
	REGISTER void *infstr;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	if (line < 0) return(x_(""));
	w = dia->items[item];
	XtVaGetValues(w, XmNitemCount, &count, XmNitems, &strlist, NULL);
	if (line >= count) retstring = x_(""); else
	{
		XmStringGetLtoR(strlist[line], XmFONTLIST_DEFAULT_TAG, &text);
		infstr = initinfstr();
		addstringtoinfstr(infstr, string2byte(text));
		retstring = returninfstr(infstr);
		XtFree(text);
	}
	return(retstring);
}

void DiaSetScrollLine(void *vdia, INTBIG item, INTBIG line, CHAR *msg)
{
	Widget w;
	int count, *pos;   /* must be "int", and not "INTBIG" to work on Alpha */
	INTBIG selected;
	XmString strlist[1];
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];

	/* remember which line was selected */
	selected = 0;
	if (XmListGetSelectedPos(w, &pos, &count))
	{
		if (count == 1) selected = pos[0];
		XtFree((char *)pos);
	}

	/* change the desired line */
	strlist[0] = XmStringCreateLocalized(string1byte(msg));
	XmListReplaceItemsPosUnselected(w, strlist, 1, line+1);
	XmStringFree(strlist[0]);

	/* restore the selected line */
	if (selected == 0) XmListDeselectAllItems(w); else
		XmListSelectPos(w, selected, False);
	gra_flushdialog(dia);
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
	rect->left = dia->itemdesc->list[item].r.left+1;
	rect->right = dia->itemdesc->list[item].r.right-1;
	rect->top = dia->itemdesc->list[item].r.top+1;
	rect->bottom = dia->itemdesc->list[item].r.bottom-1;
}

void DiaPercent(void *vdia, INTBIG item, INTBIG percent)
{
	Widget w;
	Display *dpy;
	Window win;
	INTBIG x, y, wid, hei, rwid;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	dpy = XtDisplay(w);
	win = XtWindow(w);

	/* determine size and draw interior bar */
	gra_getdialogcoordinates(&dia->itemdesc->list[item].r, &x, &y, &wid, &hei);
	rwid = wid * percent / 100;
	gcv.foreground = BlackPixelOfScreen(XtScreen(w));
	gc = XtGetGC(w, GCForeground, &gcv);
	XSetForeground(dpy, gc, BlackPixelOfScreen(XtScreen(w)));
	XFillRectangle(dpy, win, gc, 0, 0, rwid, hei);

	/* draw outline */
	XSetForeground(dpy, gc, BlackPixelOfScreen(XtScreen(w)));
	XDrawLine(dpy, win, gc, 0, 0, wid, 0);
	XDrawLine(dpy, win, gc, wid, 0, wid, hei);
	XDrawLine(dpy, win, gc, wid, hei, 0, hei);
	XDrawLine(dpy, win, gc, 0, hei, 0, 0);
	XtReleaseGC(w, gc);
}

void DiaRedispRoutine(void *vdia, INTBIG item, void (*routine)(RECTAREA*, void*))
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	dia->redrawroutine = routine;
	dia->redrawitem = item;
}

void DiaAllowUserDoubleClick(void *vdia)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
}

void DiaDrawRect(void *vdia, INTBIG item, RECTAREA *rect, INTBIG r, INTBIG g, INTBIG b)
{
	Widget w;
	Display *dpy;
	Window win;
	INTBIG xoff, yoff, lx, hx, ly, hy;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	xoff = dia->itemdesc->list[item].r.left;
	yoff = dia->itemdesc->list[item].r.top;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	gcv.foreground = gra_makedpycolor(dpy, r, g, b);
	gc = XtGetGC(w, GCForeground, &gcv);

	lx = gra_scaledialogcoordinate(rect->left-xoff);
	hx = gra_scaledialogcoordinate(rect->right-xoff);
	ly = gra_scaledialogcoordinate(rect->top-yoff);
	hy = gra_scaledialogcoordinate(rect->bottom-yoff);
	XFillRectangle(dpy, win, gc, lx, ly, hx-lx, hy-ly);
	XtReleaseGC(w, gc);
}

void DiaInvertRect(void *vdia, INTBIG item, RECTAREA *r)
{
	Widget w;
	Display *dpy;
	Window win;
	INTBIG xoff, yoff, lx, hx, ly, hy;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	xoff = dia->itemdesc->list[item].r.left;
	yoff = dia->itemdesc->list[item].r.top;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	/* get rectangle bounds */
	lx = gra_scaledialogcoordinate(r->left-xoff);
	hx = gra_scaledialogcoordinate(r->right-xoff);
	ly = gra_scaledialogcoordinate(r->top-yoff);
	hy = gra_scaledialogcoordinate(r->bottom-yoff);

	/* invert the rectangle */
	gcv.function = GXinvert;
	gc = XtGetGC(w, GCFunction, &gcv);
	XFillRectangle(dpy, win, gc, lx, ly, hx-lx, hy-ly);
	XtReleaseGC(w, gc);
}

void DiaFrameRect(void *vdia, INTBIG item, RECTAREA *r)
{
	Widget w;
	Display *dpy;
	Window win;
	INTBIG xoff, yoff, lx, hx, ly, hy;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	xoff = dia->itemdesc->list[item].r.left;
	yoff = dia->itemdesc->list[item].r.top;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	/* get rectangle bounds */
	lx = gra_scaledialogcoordinate(r->left-xoff);
	hx = gra_scaledialogcoordinate(r->right-xoff);
	ly = gra_scaledialogcoordinate(r->top-yoff);
	hy = gra_scaledialogcoordinate(r->bottom-yoff);

	/* erase the rectangle */
	gcv.foreground = WhitePixelOfScreen(XtScreen(w));
	gc = XtGetGC(w, GCForeground, &gcv);
	XFillRectangle(dpy, win, gc, lx, ly, hx-lx, hy-ly);
	XtReleaseGC(w, gc);

	/* draw outline */
	gcv.foreground = BlackPixelOfScreen(XtScreen(w));
	gc = XtGetGC(w, GCForeground, &gcv);
	XDrawLine(dpy, win, gc, lx, ly, hx-1, ly);
	XDrawLine(dpy, win, gc, hx-1, ly, hx-1, hy-1);
	XDrawLine(dpy, win, gc, hx-1, hy-1, lx, hy-1);
	XDrawLine(dpy, win, gc, lx, hy-1, lx, ly);
	XtReleaseGC(w, gc);
}

void DiaDrawLine(void *vdia, INTBIG item, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG mode)
{
	Widget w;
	Display *dpy;
	Window win;
	INTBIG xoff, yoff, x1, y1, x2, y2;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	xoff = dia->itemdesc->list[item].r.left;
	yoff = dia->itemdesc->list[item].r.top;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	x1 = gra_scaledialogcoordinate(fx-xoff);
	y1 = gra_scaledialogcoordinate(fy-yoff);
	x2 = gra_scaledialogcoordinate(tx-xoff);
	y2 = gra_scaledialogcoordinate(ty-yoff);
	switch (mode)
	{
		case DLMODEON:
			gcv.foreground = BlackPixelOfScreen(XtScreen(w));
			gc = XtGetGC(w, GCForeground, &gcv);
			break;
		case DLMODEOFF:
			gcv.foreground = WhitePixelOfScreen(XtScreen(w));
			gc = XtGetGC(w, GCForeground, &gcv);
			break;
		case DLMODEINVERT:
			gcv.function = GXinvert;
			gc = XtGetGC(w, GCFunction, &gcv);
			break;
	}
	XDrawLine(dpy, win, gc, x1, y1, x2, y2);
	XtReleaseGC(w, gc);
}

void DiaFillPoly(void *vdia, INTBIG item, INTBIG *xv, INTBIG *yv, INTBIG count, INTBIG r, INTBIG g, INTBIG b)
{
	Widget w;
	Display *dpy;
	Window win;
	INTBIG xoff, yoff, i;
	XPoint pointlist[20];
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	xoff = dia->itemdesc->list[item].r.left;
	yoff = dia->itemdesc->list[item].r.top;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	for(i=0; i<count; i++)
	{
		pointlist[i].x = gra_scaledialogcoordinate(xv[i]-xoff);
		pointlist[i].y = gra_scaledialogcoordinate(yv[i]-yoff);
	}

	/* determine color to use */
	gcv.foreground = gra_makedpycolor(dpy, r, g, b);
	gc = XtGetGC(w, GCForeground, &gcv);

	XFillPolygon(dpy, win, gc, pointlist, count, Complex, CoordModeOrigin);
	XtReleaseGC(w, gc);
	gra_flushdialog(dia);
}

void DiaPutText(void *vdia, INTBIG item, CHAR *msg, INTBIG x, INTBIG y)
{
	INTBIG fontnumber, xoff, yoff, xp, yp;
	XFontStruct *font;
	Widget w;
	Display *dpy;
	Window win;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	item--;
	w = dia->items[item];
	xoff = dia->itemdesc->list[item].r.left;
	yoff = dia->itemdesc->list[item].r.top;
	dpy = XtDisplay(w);
	win = XtWindow(w);

	fontnumber = (dia->usertextsize - 4) / 2;
	if (fontnumber < 0) fontnumber = 0;
	if (fontnumber > 8) fontnumber = 8;
	font = gra_font[fontnumber].font;
	gcv.foreground = BlackPixelOfScreen(XtScreen(w));
	gc = XtGetGC(w, GCForeground, &gcv);
	XSetFont(dpy, gc, font->fid);
	xp = gra_scaledialogcoordinate(x-xoff);
	yp = gra_scaledialogcoordinate(y-yoff);
	XDrawString(dpy, win, gc, xp, yp+font->ascent+font->descent, string1byte(msg),
		estrlen(msg));
	XtReleaseGC(w, gc);
}

void DiaSetTextSize(void *vdia, INTBIG size)
{
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	dia->usertextsize = size;
}

void DiaGetTextInfo(void *vdia, CHAR *msg, INTBIG *wid, INTBIG *hei)
{
	int direction, asc, desc;
	INTBIG len, fontnumber;
	XCharStruct xcs;
	XFontStruct *font;
	TDIALOG *dia;

	dia = (TDIALOG *)vdia;
	fontnumber = (dia->usertextsize - 4) / 2;
	if (fontnumber < 0) fontnumber = 0;
	if (fontnumber > 8) fontnumber = 8;
	font = gra_font[fontnumber].font;
	len = estrlen(msg);
	XTextExtents(font, string1byte(msg), len, &direction, &asc, &desc, &xcs);
	*wid = xcs.width;
	*hei = font->ascent + font->descent;
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
		*x = gra_cursorx;
		*y = gra_cursory;
	}
	gra_logwriteaction(DIAUSERMOUSE, dia->windindex, *x, *y, 0);
}

/****************************** DIALOG SUPPORT ******************************/

/*
 * Routine to make sure that changes to the dialog are on the screen
 */
void gra_flushdialog(TDIALOG *dia)
{
	Display *dpy;

	dpy = XtDisplay(dia->window);
	XFlush(dpy);
	XmUpdateDisplay(dia->window);
}

/*
 * Routine to convert dialog coordinates
 */
void gra_getdialogcoordinates(RECTAREA *rect, INTBIG *x, INTBIG *y, INTBIG *wid, INTBIG *hei)
{
	*x = gra_scaledialogcoordinate(rect->left);
	*y = gra_scaledialogcoordinate(rect->top);
	*wid = gra_scaledialogcoordinate(rect->right - rect->left);
	*hei = gra_scaledialogcoordinate(rect->bottom - rect->top);
}

INTBIG gra_scaledialogcoordinate(INTBIG x)
{
	return((x * DIALOGNUM + DIALOGDEN/2) / DIALOGDEN);
}

INTBIG gra_makedpycolor(Display *dpy, INTBIG r, INTBIG g, INTBIG b)
{
	XColor xc;
	INTBIG colorvalue, depth, scr, redShift, greenShift, blueShift;
	Visual *visual;

	/* determine color to use */
	scr = DefaultScreen(dpy);
	visual = DefaultVisual(dpy, scr);
	if (VisualClass(visual) == PseudoColor ||
		VisualClass(visual) == StaticColor)
	{
		xc.red   = r << 8;
		xc.green = g << 8;
		xc.blue  = b << 8;
		XAllocColor(dpy, gra_maincolmap, &xc);
		colorvalue = xc.pixel;
	} else
	{
		depth = DefaultDepth(dpy, scr);
		if (depth == 16)
		{
			colorvalue = ((r & 0xF8) << 8) |
				((g & 0xF8) << 3) | ((b & 0xF8) >> 3);
		} else
		{
			if (visual->red_mask == 0xFF) redShift = 0; else
				if (visual->red_mask == 0xFF00) redShift = 8; else
					if (visual->red_mask == 0xFF0000) redShift = 16;
			if (visual->green_mask == 0xFF) greenShift = 0; else
				if (visual->green_mask == 0xFF00) greenShift = 8; else
					if (visual->green_mask == 0xFF0000) greenShift = 16;
			if (visual->blue_mask == 0xFF) blueShift = 0; else
				if (visual->blue_mask == 0xFF00) blueShift = 8; else
					if (visual->blue_mask == 0xFF0000) blueShift = 16;
			colorvalue = (b << blueShift) | (g << greenShift) | (r << redShift);
		}
	}
	return(colorvalue);
}

/*
 * Routine to make a Pixmap from image data
 */
Pixmap gra_makeicon(UCHAR1 *data, Widget widget)
{
	static Pixmap buildupmain[MAXICONS], buildupalt[MAXICONS];
	REGISTER Pixmap *buildup;
	static INTBIG which = 0;
	static INTBIG foreground, background, screen, bgr, bgg, bgb;
	UCHAR1 *tdptr, *setptr;
	Window win;
	Display *dpy;
	Visual *visual;
	INTBIG i, j, wid, hei, bytesperrow, datasize, x, y, byte, bit,
		outx, lastoutx, outy, lastouty, sdep, pad, allocsdep;
	static GC pgc;
	static XImage *ximage;
	Arg arg[2];
	XGCValues gcv;

	/* determine sizes */
	dpy = XtDisplay(widget);
	if (dpy == gra_maindpy) buildup = buildupmain; else
		if (dpy == gra_altdpy) buildup = buildupalt; else
	{
		ttyputerr(_("Cannot find display for icon"));
		return(0);
	}
	screen = DefaultScreen(dpy);
	wid = gra_scaledialogcoordinate(32);
	hei = gra_scaledialogcoordinate(32);
	sdep = DefaultDepth(dpy, screen);
	allocsdep = sdep;
	if (allocsdep == 24) allocsdep = 32;
	pad = XBitmapPad(dpy);
	bytesperrow = ((wid*allocsdep+pad-1) / pad) * pad / 8;
	datasize = bytesperrow * hei;

	/* initialize icon storage */
	if (gra_icontruedata == 0)
	{
		for(i=0; i<MAXICONS; i++) buildupmain[i] = buildupalt[i] = 0;
		gra_icontruedata = (UCHAR1 *)emalloc(datasize, us_tool->cluster);
		if (gra_icontruedata == 0) return(0);
		gra_iconrowdata = (UCHAR1 *)emalloc(bytesperrow, us_tool->cluster);
		if (gra_iconrowdata == 0) return(0);
	}

	/* choose an icon to build */
	which++;
	if (which >= MAXICONS) which = 0;

	if (buildup[which] == 0)
	{
		win = XtWindow(widget);
		visual = DefaultVisual(dpy, screen);
		buildup[which] = XCreatePixmap(dpy, win, wid, hei, sdep);
		XtSetArg(arg[0], XtNbackground, &gcv.background);
		XtSetArg(arg[1], XtNforeground, &gcv.foreground);
		XtGetValues(widget, arg, 2);
		foreground = gcv.foreground;
		background = gcv.background;
		if (sdep > 16)
		{
			bgr = background & 0xFF;
			bgg = (background >> 8) & 0xFF;
			bgb = (background >> 16) & 0xFF;
		}
		gcv.fill_style = FillStippled;
		pgc = XtGetGC(widget, GCForeground | GCBackground, &gcv);
		ximage = XCreateImage(dpy, visual, sdep, ZPixmap, 0,
			(CHAR1 *)gra_icontruedata, wid, hei, pad, 0);
	}
	lastouty = 0;
	tdptr = gra_icontruedata;
	for(y=0; y<32; y++)
	{
		lastoutx = 0;
		for(i=0; i<bytesperrow; i++) gra_iconrowdata[i] = foreground;
		for(x=0; x<32; x++)
		{
			byte = data[y*4+(x>>3)];
			bit = byte & (0200 >> (x&7));
			if (x == 31) outx = wid-1; else
				outx = gra_scaledialogcoordinate(x);

			if (bit == 0)
			{
				for(i=lastoutx; i<=outx; i++)
				{
					switch (sdep)
					{
						case 1:
							gra_iconrowdata[i>>3] |= (0200 >> (i&7));
							break;
						case 8:
							gra_iconrowdata[i] = background;
							break;
						case 16:
							((short *)gra_iconrowdata)[i] = background;
							break;
						case 24:
						case 32:
							setptr = &((UCHAR1 *)gra_iconrowdata)[i*4];
#ifdef BYTES_SWAPPED
							*setptr++ = 0;
							*setptr++ = bgb;
							*setptr++ = bgg;
							*setptr++ = bgr;
#else
							*setptr++ = bgb;
							*setptr++ = bgg;
							*setptr++ = bgr;
							*setptr++ = 0;
#endif
							break;
					}
				}
			}
			lastoutx = outx+1;
		}
		if (y == 31) outy = hei-1; else
			outy = gra_scaledialogcoordinate(y);
		for(i=lastouty; i<=outy; i++)
		{
			for(j=0; j<bytesperrow; j++)
				tdptr[j] = gra_iconrowdata[j];
			tdptr += bytesperrow;
		}
		lastouty = outy+1;
	}
	XPutImage(dpy, buildup[which], pgc, ximage, 0, 0, 0, 0, wid, hei);
	return(buildup[which]);
}

/*
 * Routine to handle the focus-change events on dialogs.
 */
void gra_dialog_event_handler(Widget w, XtPointer data, XEvent *event, Boolean *cont)
{
	Arg arg[2];
	short inix, iniy;
	TDIALOG *dia;

	dia = gra_whichdialog(w);
	if (dia == NOTDIALOG) return;

	/* on the first focus-in, pickup the actual location of the dialog */
	if (event->type != FocusIn) return;
	if (dia->inix != -1 || dia->iniy != -1) return;
	XtSetArg(arg[0], XtNx, &inix);
	XtSetArg(arg[1], XtNy, &iniy);
	XtGetValues(dia->window, arg, 2);
	dia->inix = inix;   dia->iniy = iniy;
}

/*
 * Routine to handle vertical-slider events in a dialog's scroll list.
 */
void gra_vertslider(Widget w, XtPointer client_data, XmScrollBarCallbackStruct *call_data)
{
	int value, item, i;
	TDIALOG *dia;

	dia = gra_whichdialog(w);
	if (dia == NOTDIALOG) return;

	value = call_data->value;
	item = (int)client_data;
	for(i=0; i<dia->numlocks; i++)
	{
		if (dia->lock1[i] == item)
		{
			gra_setscroll(dia, dia->lock2[i], value);
			if (dia->lock3[i] >= 0)
				gra_setscroll(dia, dia->lock3[i], value);
			break;
		}
		if (dia->lock2[i] == item)
		{
			gra_setscroll(dia, dia->lock1[i], value);
			if (dia->lock3[i] >= 0)
				gra_setscroll(dia, dia->lock3[i], value);
			break;
		}
		if (dia->lock3[i] == item)
		{
			gra_setscroll(dia, dia->lock1[i], value);
			gra_setscroll(dia, dia->lock2[i], value);
			break;
		}
	}
}

void gra_setscroll(TDIALOG *dia, int item, int value)
{
	Widget w, vertscroll;
	int oldvalue, slidersize, increment, pageinc;

	w = dia->items[item];
	XtVaGetValues(w, XmNverticalScrollBar, &vertscroll, NULL);
	XmScrollBarGetValues(vertscroll, &oldvalue, &slidersize, &increment, &pageinc);
	XmScrollBarSetValues(vertscroll, value, slidersize, increment, pageinc, TRUE);
}

/*
 * Routine to return the dialog object associated with widget "w".
 */
TDIALOG *gra_whichdialog(Widget w)
{
	TDIALOG *dia;

	for(;;)
	{
		for(dia = gra_firstactivedialog; dia != NOTDIALOG; dia = dia->nexttdialog)
			if (dia->window == w) return(dia);
		w = XtParent(w);
		if (w == 0) break;
	}
	return(NOTDIALOG);
}

/*
 * Routine to handle events specific to dialog items
 */
void gra_dialogaction(Widget w, XtPointer client_data, XmSelectionBoxCallbackStruct *call_data)
{
	INTBIG item, itemtype, value, count, selected, myselected[1], *selectedlist, i;
	CHAR *line;
	XmPushButtonCallbackStruct *cbspb;
	XEvent *event;
	TDIALOG *dia;

	dia = gra_whichdialog(w);
	if (dia == NOTDIALOG) return;

	item = ((int)client_data) & 0xFFFF;
	itemtype = dia->itemdesc->list[item].type;
	item++;

	if ((itemtype&ITEMTYPE) == BUTTON || (itemtype&ITEMTYPE) == DEFBUTTON)
	{
		XmProcessTraversal(dia->window, XmTRAVERSE_HOME);
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	}
	if ((itemtype&ITEMTYPE) == RADIO)
	{
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	}
	if ((itemtype&ITEMTYPE) == CHECK)
	{
		value = XmToggleButtonGetState(w);
		XmToggleButtonSetState(w, value == 0 ? True : False, 0);
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	}
	if ((itemtype&ITEMTYPE) == ICON)
	{
		cbspb = (XmPushButtonCallbackStruct *)call_data;
		event = cbspb->event;
		gra_cursorx = event->xbutton.x * DIALOGDEN / DIALOGNUM +
			dia->itemdesc->list[item].r.left;
		gra_cursory = event->xbutton.y * DIALOGDEN / DIALOGNUM +
			dia->itemdesc->list[item].r.top;
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	}
	if ((itemtype&ITEMTYPE) == POPUP)
	{
		selected = DiaGetPopupEntry(dia, item);
		gra_logwriteaction(DIAPOPUPSEL, dia->windindex, item, selected, 0);
	}
	if ((itemtype&ITEMTYPE) == SCROLL || (itemtype&ITEMTYPE) == SCROLLMULTI)
	{
		if ((itemtype&ITEMTYPE) == SCROLLMULTI)
		{
			selectedlist = DiaGetCurLines(dia, item);
			for(i=0; selectedlist[i] >= 0; i++) ;
			gra_logwriteaction(DIASCROLLSEL, dia->windindex, item, i, selectedlist);
		} else
		{
			myselected[0] = DiaGetCurLine(dia, item);
			gra_logwriteaction(DIASCROLLSEL, dia->windindex, item, 1, myselected);
		}
	}
	if ((itemtype&ITEMTYPE) == EDITTEXT)
	{
		line = string2byte(XmTextGetString(w));
		gra_logwriteaction(DIAEDITTEXT, dia->windindex, item, 0, line);
	}
	if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
		dia->dialoghit = item;
}

/*
 * Routine to handle clicks in user-drawn items
 */
void gra_dialogdraw(Widget widget, XEvent *event, String *args, int *num_args)
{
	INTBIG i, item;
	TDIALOG *dia;

	dia = gra_whichdialog(widget);
	if (dia == NOTDIALOG) return;

	/* see which dialog item this applies to */
	for(i=0; i<dia->itemdesc->items; i++)
		if (widget == dia->items[i]) break;
	if (i >= dia->itemdesc->items) return;

	/* get cursor coordinates */
	gra_cursorx = event->xbutton.x * DIALOGDEN / DIALOGNUM +
		dia->itemdesc->list[i].r.left;
	gra_cursory = event->xbutton.y * DIALOGDEN / DIALOGNUM +
		dia->itemdesc->list[i].r.top;

	/* determine type of action */
	if (*num_args != 1) return;
	if (estrcmp(string2byte(args[0]), x_("motion")) == 0)
	{
		gra_inputstate = MOTION;
		if ((event->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) == 0)
			gra_inputstate |= BUTTONUP;
	} else
	{
		switch (event->xbutton.button)
		{
			case 1: gra_inputstate = ISLEFT;    break;
			case 2: gra_inputstate = ISMIDDLE;  break;
			case 3: gra_inputstate = ISRIGHT;   break;
			case 4: gra_inputstate = ISWHLFWD;  break;
			case 5: gra_inputstate = ISWHLBKW;  break;
		}
		if ((event->xbutton.state & ShiftMask) != 0)
			gra_inputstate |= SHIFTISDOWN;
		if ((event->xbutton.state & LockMask) != 0)
			gra_inputstate |= SHIFTISDOWN;
		if ((event->xbutton.state & ControlMask) != 0)
			gra_inputstate |= CONTROLISDOWN;
		if ((event->xbutton.state & (Mod1Mask|Mod4Mask)) != 0)
			gra_inputstate |= METAISDOWN;
		if (estrcmp(string2byte(args[0]), x_("up")) == 0)
			gra_inputstate |= BUTTONUP;
	}
	if (estrcmp(string2byte(args[0]), x_("down")) == 0)
	{
		item = i+1;
		if (dia->modelessitemhit != 0) (*dia->modelessitemhit)(dia, item); else
			dia->dialoghit = item;
		gra_logwriteaction(DIAITEMCLICK, dia->windindex, item, 0, 0);
	}
}

/*
 * Routine to handle requests to copy a scroll list
 */
void gra_dialogcopywholelist(Widget widget, XEvent *event, String *args, int *num_args)
{
	INTBIG count, i, status, len;
	long itemid;
	XmString *strlist, copylabel;
	CHAR1 *text;
	Display *dpy;
	Window win;

	dpy = XtDisplay(widget);
	win = XtWindow(widget);
	copylabel = XmStringCreateLocalized(b_("Electric dialog scroll list"));
	for(;;)
	{
		status = XmClipboardStartCopy(dpy, win, copylabel, CurrentTime, widget, NULL, &itemid);
		if (status != ClipboardLocked) break;
	}
	XmStringFree(copylabel);
	XtVaGetValues(widget, XmNitemCount, &count, XmNitems, &strlist, NULL);
	for(i=0; i<count; i++)
	{
		XmStringGetLtoR(strlist[i], XmFONTLIST_DEFAULT_TAG, &text);
		len = TRUESTRLEN(text);
		text[len] = '\n';
		for(;;)
		{
			status = XmClipboardCopy(dpy, win, itemid, b_("STRING"), text, len+1, i, NULL);
			if (status != ClipboardLocked) break;
		}
		text[len] = 0;
		XtFree(text);
	}
	for(;;)
	{
		status = XmClipboardEndCopy(dpy, win, itemid);
		if (status != ClipboardLocked) break;
	}
}

/*
 * Routine to handle keystrokes in an "opaque" item
 * (replaces the text with "*")
 */
void gra_dialogopaqueaction(Widget w, XtPointer client_data,
	XmTextVerifyCallbackStruct *call_data)
{
	INTBIG i;
	TDIALOG *dia;

	dia = gra_whichdialog(w);
	if (dia == NOTDIALOG) return;

	if (call_data->startPos < call_data->currInsert)
	{
		call_data->endPos = estrlen(dia->opaquefield);
		dia->opaquefield[call_data->startPos] = 0;
		return;
	}
	if (call_data->text->length > 1)
	{
		call_data->doit = False;
		return;
	}
	estrncat(dia->opaquefield, string2byte(call_data->text->ptr), call_data->text->length);
	dia->opaquefield[call_data->endPos + call_data->text->length] = 0;

	for(i=0; i<call_data->text->length; i++)
		call_data->text->ptr[i] = '*';
}

/*
 * Routine to redraw user-drawn items (calls the user's callback)
 */
void gra_dialogredraw(Widget w, XtPointer client_data, XmSelectionBoxCallbackStruct *call_data)
{
	RECTAREA ra;
	TDIALOG *dia;

	dia = gra_whichdialog(w);
	if (dia == NOTDIALOG) return;

	if (dia->redrawroutine != 0)
	{
		DiaItemRect(dia, dia->redrawitem + 1, &ra);
		(*dia->redrawroutine)(&ra, dia);
	}
}

/*
 * Routine to redraw separator lines
 */
void gra_dialogredrawsep(Widget w, XtPointer client_data, XmSelectionBoxCallbackStruct *call_data)
{
	Display *dpy;
	Window win;
	INTBIG x, y, wid, hei;
	XGCValues gcv;
	GC gc;
	TDIALOG *dia;

	dia = gra_whichdialog(w);
	if (dia == NOTDIALOG) return;

	gra_getdialogcoordinates(&dia->itemdesc->list[(INTBIG)client_data].r,
		&x, &y, &wid, &hei);
	dpy = XtDisplay(w);
	win = XtWindow(w);
	gcv.foreground = BlackPixelOfScreen(XtScreen(w));
	gc = XtGetGC(w, GCForeground, &gcv);
	XSetForeground(dpy, gc, BlackPixelOfScreen(XtScreen(w)));
	XFillRectangle(dpy, win, gc, 0, 0, wid, hei);
	XtReleaseGC(w, gc);
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
