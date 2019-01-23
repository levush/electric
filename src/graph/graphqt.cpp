/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design Systems
 *
 * File: graphqt.cpp
 * Qt Window System interface
 * Written by: Dmitry Nadezhin, Instutute for Design Problems in Microelectronics, Russian Academy of Sciences
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
 * info@staticfreesoft.com
 */

#include "global.h"
#include "database.h"
#include "egraphics.h"
#include "eio.h"
#include "usr.h"
#include "usrtrack.h"
#include "edialogs.h"

#ifdef MACOSX
#  include <Carbon/Carbon.h>
#endif

#include "graphqt.h"

#include <qclipboard.h>
#include <qdir.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qfontdatabase.h>
#include <qfontdialog.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmenubar.h>
#include <qtextedit.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qnamespace.h>
#include <qprinter.h>
#include <qsettings.h>
#include <qsound.h>
#include <qstatusbar.h>
#include <qtimer.h>
#include <qvbox.h>
#ifdef USEMDI
#  include <qdockwindow.h>
#  include <qworkspace.h>
#endif

#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#if defined(ONUNIX) || defined(MACOSX)
#  include <pwd.h>
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

#ifdef HAVE_PTHREAD
#  include <pthread.h>
#else
#  include <thread.h>
#  include <synch.h>
#endif

#ifdef WIN32
#  include <io.h>
#endif

#if LANGTCL
#  include "dblang.h"
  Tcl_Interp *gra_tclinterp;
  INTBIG gra_initializetcl(void);
#endif

/****** windows ******/

#define WMLEFTBORDER          8					/* size of left border for windows */
#define WMTOPBORDER          30					/* size of top border for windows */
#define WMTOPBORDERMAC       44					/* size of menu border of Macintosh */
#define FLUSHTICKS          120					/* ticks between display flushes */
#define EPROCESSWAIT        100                 /* maximal time (in ms) to wait in EProcess.wait() */
#define SBARWIDTH            15					/* size of text-edit scroll bars */
#define PALETTEWIDTH        110
#define MAXLOCALSTRING      256					/* size of "gra_localstring" */

#ifdef __cplusplus
#  define VisualClass(v) v->c_class
#else
#  define VisualClass(v) v->class
#endif

EApplication        *gra = 0;
static INTBIG        gra_editposx;
static INTBIG        gra_editposy;
static INTBIG        gra_editsizex;
static INTBIG        gra_editsizey;
static UINTBIG       gra_internalchange;
static INTSML        gra_windowframeindex = 0;
static INTSML        gra_windownumber;			/* number of the window that was created */
static WINDOWFRAME  *gra_deletethisframe = 0;	/* window that is to be deleted */
static char          gra_localstring[MAXLOCALSTRING];
extern GRAPHICS      us_box, us_ebox, us_menutext;
static BOOLEAN       gra_noflush = FALSE;		/* TRUE to supress display flushing */

static QRgb gra_colortable[256];
static INTBIG        gra_windowbeingdeleted = 0;

static void       gra_adjustwindowframeon(INTSML how);
static BOOLEAN    gra_onthiswindow(QWidget *wnd, RECTAREA *r);
static INTSML     gra_buildwindow(QWidget *desktop, WINDOWFRAME *wf, BOOLEAN floating);
static void       gra_getsensiblescreensize(INTBIG *screensizex, INTBIG *screensizey);
static void       gra_reloadmap(void);
static void       gra_removewindowextent(RECTAREA *r, QWidget *wnd);
static void       gra_handlequeuedframedeletion(void);
static INTSML     gra_graphicshas(INTSML want);
static char     **gra_eprinterlist(void);

/****** the messages window ******/
static INTSML        gra_messages_typingin;		/* nonzero if typing into messages window */

/****** the status bar ******/
#define MAXSTATUSLINES  1						/* number of status lines */

static STATUSFIELD *gra_statusfields[100];
static char *gra_statusfieldtext[100];
static INTSML gra_indicatorcount = 0;

/****** the menus ******/
static INTSML      gra_pulldownmenucount_ = 0;	/* number of top-level pulldown menus */
static POPUPMENU  *gra_pulldowns_[50];			/* the current top-level pulldown menus */

/****** events ******/
typedef RETSIGTYPE (*SIGNALCAST)(int);

static INTSML        gra_doublethresh;			/* threshold of double-click */
static INTBIG        gra_cursorx, gra_cursory;	/* current position of mouse */
static QSound       *gra_clicksound;
static Qt::ButtonState   gra_lastbuttonstate = Qt::NoButton; /* last button state (for getbuckybits) */

#define TIME_SLICE_DELAY        50              /* time ( in ms ) to postpone timeslice */

static INTSML gra_stop_dowork = 0;
RETSIGTYPE gra_sigill_trap(void);
RETSIGTYPE gra_sigfpe_trap(void);
RETSIGTYPE gra_sigbus_trap(void);
RETSIGTYPE gra_sigsegv_trap(void);
static BOOLEAN gra_setcurrentwindowframe(WINDOWFRAME *wf);

static INTSML gra_nxtchar;
static INTBIG gra_nxtcharspecial;
static BOOLEAN gra_nxtcharhandler(INTSML chr, INTBIG special);
static BOOLEAN gra_nullbuttonhandler(INTBIG x, INTBIG y, INTBIG but);

typedef enum
{
	S_STARTUP, S_USER, S_TOOL, S_TRACK, S_MODAL, S_CHECKINT
} GRAPHSTATE;

#ifdef ETRACE
static char *stateNames[] =
{
	"STARTUP", "USER", "TOOL", "TRACK", "MODAL", "CHECKINT"
};
#endif

GRAPHSTATE gra_state;

/****** mouse buttons ******/

typedef struct
{
	char  *name;				/* button name */
	INTSML unique;				/* number of letters that make it unique */
} MOUSEBUTTONS;

#ifdef MACOS
#  define BUTTONS     17		/* cannot exceed NUMBUTS in "usr.h" */
#  define REALBUTS    5			/* actual number of buttons */
MOUSEBUTTONS gra_buttonname[BUTTONS] =
{						/* Shift Command Option Control */
	"Button",      1,   /*                              */
	"SButton",     2,	/* Shift                        */
	"MButton",     2,	/*       Command                */
	"SMButton",    3,	/* Shift Command                */
	"OButton",     2,	/*               Option         */
	"SOButton",    3,	/* Shift         Option         */
	"MOButton",    3,	/*       Command Option         */
	"SMOButton",   4,	/* Shift Command Option         */
	"CButton",     2,	/*                      Control */
	"SCButton",    3,	/* Shift                Control */
	"CMButton",    3,	/*       Command        Control */
	"SCMButton",   4,	/* Shift Command        Control */
	"COButton",    3,	/*               Option Control */
	"SCOButton",   4,	/* Shift         Option Control */
	"CMOButton",   4,	/*       Command Option Control */
	"SCMOButton",  5,	/* Shift Command Option Control */
	"DButton",     2
};
#else
#  define BUTTONS     45		/* cannot exceed NUMBUTS in "usr.h" */
#  define REALBUTS    5			/* actual number of buttons */
MOUSEBUTTONS gra_buttonname[BUTTONS] =
{
	{"LEFT",1},   {"MIDDLE",2},   {"RIGHT",1},   {"FORWARD",1},   {"BACKWARD",1},	/* unshifted */
	{"SLEFT",2},  {"SMIDDLE",3},  {"SRIGHT",2},  {"SFORWARD",2},  {"SBACKWARD",2},	/* shift held down */
	{"CLEFT",2},  {"CMIDDLE",3},  {"CRIGHT",2},  {"CFORWARD",2},  {"CBACKWARD",2},	/* control held down */
	{"MLEFT",2},  {"MMIDDLE",3},  {"MRIGHT",2},  {"MFORWARD",2},  {"MBACKWARD",2},	/* meta held down */
	{"SCLEFT",3}, {"SCMIDDLE",4}, {"SCRIGHT",3}, {"SCFORWARD",3}, {"SCBACKWARD",3},	/* shift and control held down*/
	{"SMLEFT",3}, {"SMMIDDLE",4}, {"SMRIGHT",3}, {"SMFORWARD",3}, {"SMBACKWARD",3},	/* shift and meta held down */
	{"CMLEFT",3}, {"CMMIDDLE",4}, {"CMRIGHT",3}, {"CMFORWARD",3}, {"CMBACKWARD",3},	/* control and meta held down */
	{"SCMLEFT",4},{"SCMMIDDLE",4},{"SCMRIGHT",4},{"SCMFORWARD",4},{"SCMBACKWARD",4},/* shift, control, and meta */
	{"DLEFT",2},  {"DMIDDLE",2},  {"DRIGHT",2},  {"DFORWARD",2},  {"DBACKWARD",2}	/* double-click */
};
#endif

/* SWAPPED means that the bytes are MSB first */
#if defined(sparc) || defined(__APPLE__)
#  define SWAPPED 1
#endif

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

#if 0
/* the normal cursor mask (NORMALCURSOR, an "X") */
static UINTSML gra_realcursormask[16] = {
	0xC003, /* XX            XX */
	0xE007, /* XXX          XXX */
	0x700E, /*  XXX        XXX  */
	0x381C, /*   XXX      XXX   */
	0x1C38, /*    XXX    XXX    */
	0x0E70, /*     XXX  XXX     */
	0x07E0, /*      XXXXXX      */
	0x03C0, /*       XXXX       */
	0x03C0, /*       XXXX       */
	0x07E0, /*      XXXXXX      */
	0x0E70, /*     XXX  XXX     */
	0x1C38, /*    XXX    XXX    */
	0x381C, /*   XXX      XXX   */
	0x700E, /*  XXX        XXX  */
	0xE007, /* XXX          XXX */
	0xC003  /* XX            XX */
};

/* the normal cursor data (NORMALCURSOR, an "X") */
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
#else
/* the normal cursor mask (NORMALCURSOR, an "X") */
static UINTSML gra_realcursormask[16] = {
	0x6006, /*  XX          XX  */
	0xF00F, /* XXXX        XXXX */
	0xF81F, /* XXXXX      XXXXX */
	0x7C3E, /*  XXXXX    XXXXX  */
	0x3E7C, /*   XXXXX  XXXXX   */
	0x1FF8, /*    XXXXXXXXXX    */
	0x0FF0, /*     XXXXXXXX     */
	0x0660, /*      XX  XX      */
	0x0660, /*      XX  XX      */
	0x0FF0, /*     XXXXXXXX     */
	0x1FF8, /*    XXXXXXXXXX    */
	0x3E7C, /*   XXXXX  XXXXX   */
	0x7C3E, /*  XXXXX    XXXXX  */
	0xF81F, /* XXXXX      XXXXX */
	0xF00F, /* XXXX        XXXX */
	0x6006  /*  XX          XX  */
};

/* the normal cursor data (NORMALCURSOR, an "X") */
static UINTSML gra_realcursordata[16] = {
	0x0000, /*                  */
	0x6006, /*  XX          XX  */
	0x700E, /*  XXX        XXX  */
	0x381C, /*   XXX      XXX   */
	0x1C38, /*    XXX    XXX    */
	0x0E70, /*     XXX  XXX     */
	0x07E0, /*      XXXXXX      */
	0x0240, /*       X  X       */
	0x0240, /*       X  X       */
	0x07E0, /*      XXXXXX      */
	0x0E70, /*     XXX  XXX     */
	0x1C38, /*    XXX    XXX    */
	0x381C, /*   XXX      XXX   */
	0x700E, /*  XXX        XXX  */
	0x6006, /*  XX          XX  */
	0x0000  /*                  */
};
#endif

/* the "draw with pen" cursor mask (PENCURSOR, a pen) */
static UINTSML gra_drawcursormask[16] = {
	0x3000, /*             XX   */
	0x7800, /*            XXXX  */
	0xFC00, /*           XXXXXX */
	0xFE00, /*          XXXXXXX */
	0x7F00, /*         XXXXXXX  */
	0x3F80, /*        XXXXXXX   */
	0x1FC0, /*       XXXXXXX    */
	0x0FE0, /*      XXXXXXX     */
	0x07F0, /*     XXXXXXX      */
	0x03F8, /*    XXXXXXX       */
	0x01FC, /*   XXXXXXX        */
	0x00FC, /*   XXXXXX         */
	0x007E, /*  XXXXXX          */
	0x003E, /*  XXXXX           */
	0x000F, /* XXXX             */
	0x0003  /* XX               */
};

/* the "draw with pen" cursor data (PENCURSOR, a pen) */
static UINTSML gra_drawcursordata[16] = {
	0x0000, /*                  */
	0x0000, /*                  */
	0x0800, /*            X     */
	0x1C00, /*           XXX    */
	0x3E00, /*          XXXXX   */
	0x1F00, /*         XXXXX    */
	0x0F80, /*        XXXXX     */
	0x07C0, /*       XXXXX      */
	0x03E0, /*      XXXXX       */
	0x01F0, /*     XXXXX        */
	0x00F0, /*     XXXX         */
	0x0070, /*     XXX          */
	0x0000, /*                  */
	0x0000, /*                  */
	0x0004, /*  X               */
	0x0000  /*                  */
};

/* the "technology" command dragging cursor mask (TECHCURSOR, a "T") */
static UINTSML gra_techcursormask[16] = {
	0x0380, /*        XXX       A */
	0x07C0, /*       XXXXX      B */
	0x0FE0, /*      XXXXXXX     C */
	0x0380, /*        XXX       D */
	0x7FFC, /*   XXXXXXXXXXXXX  E */
	0x7FFC, /*   XXXXXXXXXXXXX  F */
	0x7FFC, /*   XXXXXXXXXXXXX  G */
	0x7FFC, /*   XXXXXXXXXXXXX  H */
	0x77DC, /*   XXX XXXXX XXX  I */
	0x07C0, /*       XXXXX      J */
	0x07C0, /*       XXXXX      K */
	0x07C0, /*       XXXXX      L */
	0x07C0, /*       XXXXX      M */
	0x0FE0, /*      XXXXXXX     N */
	0x0FE0, /*      XXXXXXX     O */
	0x0FE0  /*      XXXXXXX     P */
};

/* the "technology" command dragging cursor data (TECHCURSOR, a "T") */
static UINTSML gra_techcursordata[16] = {
	0x0100, /*         X        A */
	0x0380, /*        XXX       B */
	0x0540, /*       X X X      C */
	0x0100, /*         X        D */
	0x0100, /*         X        E */
	0x3FF8, /*    XXXXXXXXXXX   F */
	0x3FF8, /*    XXXXXXXXXXX   G */
	0x2388, /*    X   XXX   X   H */
	0x0380, /*        XXX       I */
	0x0380, /*        XXX       J */
	0x0380, /*        XXX       K */
	0x0380, /*        XXX       L */
	0x0380, /*        XXX       M */
	0x0380, /*        XXX       N */
	0x07C0, /*       XXXXX      O */
	0x0000  /*                  P */
};

/* the "ibeam" text selection cursor mask (IBEAMCURSOR, an "I") */
static UINTSML gra_ibeamcursormask[16] = {
	0x07F0, /*     XXXXXXX      */
	0x07F0, /*     XXXXXXX      */
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
	0x07F0, /*     XXXXXXX      */
	0x07F0, /*     XXXXXXX      */
	0x07F0  /*     XXXXXXX      */
};

/* the "ibeam" text selection cursor data (IBEAMCURSOR, an "I") */
static UINTSML gra_ibeamcursordata[16] = {
	0x0000, /*                  */
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
	0x0360, /*      XX XX       */
	0x0000  /*                  */
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

/****** time ******/
static QTime gra_timebase;

/****** files ******/
static void         *gra_fileliststringarray = 0;
static char          gra_curpath[255] = {0};
static char        **gra_printerlist;
static INTBIG        gra_printerlistcount = 0;
static INTBIG        gra_printerlisttotal = 0;

INTSML     gra_addprinter(char *buf);
#ifdef MACOS
  void     mac_settypecreator(char *name, INTBIG type, INTBIG creator);
#endif

/******************** INITIALIZATION ********************/

int main(int argc, char *argv[])
{
	/* catch signals if Electric trys to bomb out */
	(void)signal(SIGILL, (SIGNALCAST)gra_sigill_trap);
	(void)signal(SIGFPE, (SIGNALCAST)gra_sigfpe_trap);
	(void)signal(SIGBUS, (SIGNALCAST)gra_sigbus_trap);
	(void)signal(SIGSEGV, (SIGNALCAST)gra_sigsegv_trap);

	/* primary initialization of Electric */
	gra_state = S_STARTUP;
	osprimaryosinit();
#ifdef ETRACE
	etrace(GTRACE, "  after osprimaryosinit...\n");
#endif

#if LANGTCL
	/* initialize TCL here */
	if (gra_initializetcl() == TCL_ERROR)
		error(_("Failed to initialize TCL: %s\n"), gra_tclinterp->result);
#endif

	/* secondary initialization of Electric */
#ifdef ETRACE
	etrace(GTRACE, "  calling ossecondaryinit...\n");
#endif
	ossecondaryinit(argc, argv);

    /* initialize the "click" sound */
	if (QSound::available())
	{
		void *infstr = initinfstr();
		formatinfstr(infstr, "%sclick.wav", el_libdir);
		gra_clicksound = new QSound(returninfstr(infstr));
	}

	gra_state = S_USER;
#ifdef ETRACE
	etrace(GTRACE, "  gra_state=USER\n");
#endif
	db_setcurrenttool(us_tool);
	/* first see if there is any pending input */
	el_pleasestop = 0;

#ifdef ETRACE
	etrace(GTRACE, "  main loop...\n");
#endif
	gra->exec();
	return(0);
}

/*
 * Routine to establish the default display define tty codes.
 */
void graphicsoptions(char *name, INTBIG *argc, char **argv)
{
	Q_UNUSED( name );

#ifdef ETRACE
	INTBIG i;

	etrace(GTRACE, "{ graphicsoptions: args=");
	for (i = 0; i < *argc; i++) etrace(GTRACE_CONT, " <%s>", argv[i]);
	etrace(GTRACE_CONT, "\n");
#endif

	int int_argc = *argc;
	gra = new EApplication(int_argc, argv);
	*argc = int_argc;

	/* make the messages window */
#ifdef USEMDI
    QDockWindow *messMainWin = new QDockWindow( QDockWindow::InDock, gra->mw );
	//QDockWindow *messMainWin = new QDockWindow( gra->mw, "MessagesMainWindow", 0 );
	messMainWin->setResizeEnabled( TRUE );
    messMainWin->setCloseMode( QDockWindow::Never );
	gra->mw->moveDockWindow( messMainWin, Qt::DockBottom );
	gra->messages = new EMessages( messMainWin );
    messMainWin->setWidget( gra->messages );
    messMainWin->setFixedExtentHeight( 150 );
	gra->messages->connect(messMainWin, SIGNAL(placeChanged(QDockWindow::Place)), SLOT(setIcons()));
	gra->messages->show();
#else
#  ifdef MESSMENU
	EMainWindow *messMainWin = new EMainWindow( 0, "MessagesMainWindow", Qt::WType_TopLevel, FALSE );
	messMainWin->pulldownmenuload();
#  else
	QMainWindow *messMainWin = new QMainWindow( 0, "MessagesMainWindow", Qt::WType_TopLevel );
#  endif
	gra->messages = new EMessages( messMainWin );
	messMainWin->setCentralWidget( gra->messages );
    messMainWin->installEventFilter( gra->messages );
	gra->setMainWidget( messMainWin );
#endif
    
    /* Determine initial position and size of messages window */
	INTBIG screensizex, screensizey;
	gra_getsensiblescreensize(&screensizex, &screensizey);
#ifdef ETRACE
	etrace(GTRACE, "  graphicsoptions: screensizex=%d screensizey=%d\n", screensizex, screensizey);
#endif
    int width = screensizex / 3 * 2;
    int height = screensizey / 4 - 80;
    int x = (screensizex - width) / 2;
    int y = screensizey - height - 52;
#ifndef USEMDI
    messMainWin->setGeometry(x, y, width, height);
#endif

	gra->messages->setIcons();
	messMainWin->show();

	/* Determine initial position and size of edit window */
	gra_editposx = PALETTEWIDTH;
#ifdef MACOSX
	gra_editposy = 45;
#else
	gra_editposy = 0;
#endif

	/* get screen size (but just 1 screen, if there are two) */
	gra_getsensiblescreensize(&screensizex, &screensizey);
#ifdef ETRACE
	etrace(GTRACE, "  graphicsoptions: screensizex=%d screensizey=%d\n", screensizex, screensizey);
#endif
	gra_editsizex = (screensizex - PALETTEWIDTH - 8) / 5 * 4;
	gra_editsizey = (screensizey - 2) * 3 / 5;

	QSettings settings;
	settings.insertSearchPath( QSettings::Windows, "/StaticFreeSoft" ); /* MS Windows-only */
	if ( settings.subkeyList( "/Electric" ).contains( "geometry" ) > 0 )
	{
		/* Get position and size from settings */
		gra_editposx = settings.readNumEntry( "/Electric/geometry/x", gra_editposx );
		gra_editposy = settings.readNumEntry( "/Electric/geometry/y", gra_editposy );
		gra_editsizex = settings.readNumEntry( "/Electric/geometry/width", gra_editsizex );
		gra_editsizey = settings.readNumEntry( "/Electric/geometry/height", gra_editsizey );
	} else
	{
		/* setting not found. Write them */
		settings.writeEntry( "/Electric/geometry/x", int(gra_editposx) );
		settings.writeEntry( "/Electric/geometry/y", int(gra_editposy) );
		settings.writeEntry( "/Electric/geometry/width", int(gra_editsizex) );
		settings.writeEntry( "/Electric/geometry/height", int(gra_editsizey) );
	}

#ifdef ETRACE
	etrace(GTRACE, "} graphicsoptions: el_colcursor=%#o el_maplength=%d\n", el_colcursor, el_maplength);
#endif
}

/*
 * Routine to initialize the display device.
 */
BOOLEAN initgraphics(BOOLEAN messages)
{
#ifdef ETRACE
	etrace(GTRACE, "{} initgraphics: messages=%d\n", messages);
#else
	Q_UNUSED( messages );
#endif 
	return(FALSE);
}

EApplicationWindow::EApplicationWindow()
	: EMainWindow( 0, "EApplicationWindow", WType_TopLevel, TRUE )
{
#ifdef USEMDI
    QVBox* vb = new QVBox( this );
    vb->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
    ws = new QWorkspace( vb );
    //ws->setScrollBarsEnabled( TRUE );
    setCentralWidget( vb );
#endif
}

EApplication::EApplication( int &argc, char **argv)
  : QApplication (argc, argv), mw( 0 ), textbits( 0, 0 ), charbits( 0, 0 )
{
    REGISTER UINTBIG i, j;
    char *ptr;
    char *geomSpec;

    /* initialize X and the toolkit */

	/* get switch settings */
	geomSpec = NULL;
	for(int k=1; k < argc; k++)
	{
		if (argv[k][0] == '-')
		{
			switch (argv[k][1])
			{
				case 'g':       /* Geometry */
					if (++k >= argc) continue;
					geomSpec = argv[k];
					continue;
			}
		}
	}

	/* double-click threshold (in milliseconds) */
#ifdef MACOS
	gra_doublethresh = GetDblTime()*1000/60;
#else
	gra_doublethresh = doubleClickInterval();
#endif

	/* get the information about the edit display */
	if (QPixmap::defaultDepth() == 1)
		fprintf(stderr, _("Cannot run on 1-bit deep displays\n"));
	el_colcursor = CURSOR;	/* also done in usr.c later */
	el_maplength = 256;

	gra_windownumber = 0;

	/* get fonts */
#ifdef MACOS
	fixedfont.setFamily("Monaco");
	fixedfont.setPointSize(10);
#else
	fixedfont.setFamily( QString::null );
	fixedfont.setStyleHint( QFont::TypeWriter );
	fixedfont.setStyleStrategy( QFont::NoAntialias );
	fixedfont.setFixedPitch( TRUE );
#endif

	/* initialize font cache */
	QFontDatabase fdb;
	facelist = fdb.families();
	gra_initfaces( facelist );
	char **localfacelist;
	screengetfacelist(&localfacelist, TRUE);
	ptr = getenv("ELECTRIC_TRUETYPE_FONT");
	if (ptr != NULL)
	{
		for(i=0; i<facelist.count(); i++)
		{
			if (namesame(ptr, localfacelist[i]) == 0)
			{
				defface = localfacelist[i];
				break;
			}
		}
		if (i >= facelist.count())
		{
			printf(_("Warning: environment variable 'ELECTRIC_TRUETYPE_FONT' is '%s' which is not a known font\n"),
				ptr);
			printf(_("Choices are:\n"));
			for(j=0; j<facelist.count(); j++)
			{
				printf("  %s\n", localfacelist[j]);
			}
		}
	}
	if (defface.isEmpty())
	{
#ifdef MACOSX
		for(i=0; i<facelist.count(); i++)
		{
			if (namesame("Charcoal", localfacelist[i]) == 0)
			{
				defface = localfacelist[i];
				break;
			}
		}
		if (defface.isEmpty())
#endif
		defface = font().family();
	}


	/* on machines with reverse byte ordering, fix the icon and the cursors */
#ifdef SWAPPED
	j = PROGICONSIZE * PROGICONSIZE / 16;
	for(i=0; i<j; i++)
		gra_icon[i] = ((gra_icon[i] >> 8) & 0xFF) | ((gra_icon[i] << 8) & 0xFF00);
	for(i=0; i<16; i++)
	{
		gra_realcursordata[i] = ((gra_realcursordata[i] >> 8) & 0xFF) |
			((gra_realcursordata[i] << 8) & 0xFF00);
		gra_realcursormask[i] = ((gra_realcursormask[i] >> 8) & 0xFF) |
			((gra_realcursormask[i] << 8) & 0xFF00);

		gra_drawcursordata[i] = ((gra_drawcursordata[i] >> 8) & 0xFF) |
			((gra_drawcursordata[i] << 8) & 0xFF00);
		gra_drawcursormask[i] = ((gra_drawcursormask[i] >> 8) & 0xFF) |
			((gra_drawcursormask[i] << 8) & 0xFF00);

		gra_techcursordata[i] = ((gra_techcursordata[i] >> 8) & 0xFF) |
			((gra_techcursordata[i] << 8) & 0xFF00);
		gra_techcursormask[i] = ((gra_techcursormask[i] >> 8) & 0xFF) |
			((gra_techcursormask[i] << 8) & 0xFF00);

		gra_ibeamcursordata[i] = ((gra_ibeamcursordata[i] >> 8) & 0xFF) |
			((gra_ibeamcursordata[i] << 8) & 0xFF00);
		gra_ibeamcursormask[i] = ((gra_ibeamcursormask[i] >> 8) & 0xFF) |
			((gra_ibeamcursormask[i] << 8) & 0xFF00);

		gra_nomousecursordata[i] = ((gra_nomousecursordata[i] >> 8) & 0xFF) |
			((gra_nomousecursordata[i] << 8) & 0xFF00);
		gra_menucursordata[i] = ((gra_menucursordata[i] >> 8) & 0xFF) |
			((gra_menucursordata[i] << 8) & 0xFF00);
	}
#endif
#ifndef MACOS
	programicon = QBitmap (PROGICONSIZE, PROGICONSIZE, (uchar*)gra_icon, TRUE);
#endif

	/* create the cursors */
	QBitmap bitm, mbitm;

	/* these cursors have shadows around them and show up right on any color scheme */
	nullcursor = QCursor(WaitCursor);
	handcursor = QCursor(PointingHandCursor);
	lrcursor = QCursor(SizeHorCursor);
	udcursor = QCursor(SizeVerCursor);

	bitm = QBitmap( 16, 16, (uchar*)gra_realcursordata, TRUE );
	mbitm = QBitmap( 16, 16, (uchar*)gra_realcursormask, TRUE );
	realcursor = QCursor( bitm, mbitm, 8, 8 );

	bitm = QBitmap( 16, 16, (uchar*)gra_drawcursordata, TRUE );
	mbitm = QBitmap( 16, 16, (uchar*)gra_drawcursormask, TRUE );
	drawcursor = QCursor( bitm, mbitm, 0, 16 );

	bitm = QBitmap( 16, 16, (uchar*)gra_ibeamcursordata, TRUE );
	mbitm = QBitmap( 16, 16, (uchar*)gra_ibeamcursormask, TRUE );
	ibeamcursor = QCursor( bitm, mbitm, 8, 8 );

	bitm = QBitmap( 16, 16, (uchar*)gra_techcursordata, TRUE );
	mbitm = QBitmap( 16, 16, (uchar*)gra_techcursormask, TRUE );
	techcursor = QCursor( bitm, mbitm, 8, 0 );

	/* these cursors are not really used, so the masking doesn't really matter */
	bitm = QBitmap( 16, 16, (uchar*)gra_nomousecursordata, TRUE );
	nomousecursor = QCursor( bitm, bitm, 8, 8 );

	bitm = QBitmap( 16, 16, (uchar*)gra_menucursordata, TRUE );
	menucursor = QCursor( bitm, bitm, 16, 8 );

	/* initialize the window frames */
	el_firstwindowframe = el_curwindowframe = NOWINDOWFRAME;

	/* initialize the mouse */
	us_cursorstate = -1;
	us_normalcursor = NORMALCURSOR;

#ifdef USEMDI
	/* initialize application window with workspace */
	mw = new EApplicationWindow();
#ifdef EPROGRAMNAME
    (void)sprintf(gra_localstring, EPROGRAMNAME);
#else
    (void)sprintf(gra_localstring, _("Electric"));
#endif
#ifndef MACOS
    mw->setIcon( programicon );
#endif
    mw->setCaption( gra_localstring );
    mw->setIconText( gra_localstring );

	setMainWidget( mw );
	mw->show();
#endif

#ifdef ETRACE
	etrace(GTRACE, "  initgraphics: us_cursorstate=%d us_normalcursor=%d\n",
	       us_cursorstate, us_normalcursor);
#endif 
}

bool EApplication::notify( QObject *receiver, QEvent *e )
{
#ifdef MACOSX
	if (e->spontaneous())
	{
		switch (e->type())
		{
			case QEvent::KeyPress:
				{
					QKeyEvent *ke = (QKeyEvent*)e;
					/* ttyputmsg(_("Key %x pressed"), ke->key()); */
				}
				break;
			case QEvent::KeyRelease:
				{
					QKeyEvent *ke = (QKeyEvent*)e;
					/* ttyputmsg(_("Key %x released"), ke->key()); */
				}
				break;

			case QEvent::MouseButtonPress:
				{
					QMouseEvent *me = (QMouseEvent*)e;
					/* ttyputmsg(_("notify MouseButtonPress %o"), me->button() ); */
				}
				break;
		}
	}
#endif
	return QApplication::notify( receiver, e );
}

/*
 * Routine to return the size of the screen (but only one of them if a multiscreen world)
 */
void gra_getsensiblescreensize(INTBIG *screensizex, INTBIG *screensizey)
{
#ifdef USEMDI
	if (gra->mw && gra->mw->ws)
	{
		*screensizex = gra->mw->ws->width();
		*screensizey = gra->mw->ws->height();
	} else
#endif
	{
		*screensizex = gra->desktop()->width();
		*screensizey = gra->desktop()->height();
	}
	if (*screensizex > *screensizey * 2) *screensizex /= 2; else
		if (*screensizey > *screensizex * 2) *screensizey /= 2;
}

EMessages::EMessages( QWidget *parent )
  : QVBox( parent ), stopped(FALSE)
{
    scroll = new QTextEdit( this );
	scroll->setTextFormat(Qt::PlainText);
    scroll->setReadOnly( TRUE );
    scroll->setWordWrap( QTextEdit::NoWrap );
    scroll->viewport()->setCursor( IbeamCursor );
    scroll->installEventFilter( this );

    input = new QHBox( this );
    prompt = new QLabel( input );
    line = new QLineEdit( input );
    input->hide();

    connect(line, SIGNAL(returnPressed()), SLOT(lineEntered()));
    line->installEventFilter( this );
    
    gra_messages_typingin = 0;
}

void EMessages::clear()
{
	scroll->clear();
}

void EMessages::putString(char *s, BOOLEAN important)
{
    QString qstr = QString::fromLocal8Bit( s );
	scroll->insertParagraph( qstr, -1 );
    scroll->scrollToBottom();
	qApp->flush();

	/* make sure the window isn't iconified or obscured */
	if (important)
		parentWidget()->raise();
}

char *EMessages::getString(char *promptStr)
{
	/* change state */
	GRAPHSTATE savedstate = gra_state;
	gra_state = S_MODAL;
#ifdef ETRACE
	etrace(GTRACE, "  gra_state=MODAL\n");
#endif
	gra->charhandler = 0;
	gra->buttonhandler = 0;

	/* set cursor */
	INTBIG oldnormalcursor = us_normalcursor;
	setnormalcursor(WANTTTYCURSOR);

    QString qstr = QString::fromLocal8Bit( promptStr );
	prompt->setText( qstr );
	line->clear();
	stopped = FALSE;
	input->show();
	parentWidget()->raise();
	line->grabKeyboard();
	gra->enter_loop();
	input->hide();

	/* restore cursor */
	setnormalcursor(oldnormalcursor);
	gra_state = savedstate;

	if (stopped) return(0);
	scroll->insertParagraph( prompt->text() + line->text(), -1 );
	scroll->scrollToBottom();
	char *str = EApplication::localize( line->text() );
	return(str);
}

void EMessages::setFont()
{
	bool ok;
	QFont font = scroll->font();
	font = QFontDialog::getFont( &ok, font, parentWidget() );
	if (ok)
		scroll->setFont( font );
}

void EMessages::setColor( QColor &bg, QColor &fg )
{
	QPalette palette = scroll->palette();
	palette.setColor( QColorGroup::Base, bg );
	palette.setColor( QColorGroup::Text, fg );
	scroll->setPalette( palette );
}

void EMessages::keyPressEvent( QKeyEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ EMessages::keyPressEvent %s accepted=%d\n", stateNames[gra_state], e->isAccepted());
#endif
    INTBIG special;
    int state = gra->translatekey(e, &special);

    if (state != 0 || special != 0)
    {
        us_state |= DIDINPUT;
		INTSML cmd = state;
#ifdef ETRACE
		etrace(GTRACE, "  EMessages::keyPressEvent: %s state=%#o cmd=%#o special=%#o DIDINPUT%d,%d\n",
			stateNames[gra_state], state, cmd, special, gra_cursorx, gra_cursory );
#endif

		switch (gra_state)
		{
			case S_USER:
#ifdef ETRACE
				etrace(GTRACE, "{ us_oncommand: cmd=%#o special=%#o\n", cmd, special);
#endif
				us_oncommand( cmd, special );
				gra->toolTimeSlice();
#ifdef ETRACE
				etrace(GTRACE, "} us_oncommand\n");
#endif
				break;
            default:
                break;
		}
		e->accept();
    }
    else QVBox::keyPressEvent( e );
#ifdef ETRACE
    etrace(GTRACE, "} EMessages::keyPressEvent accepted=%d\n", e->isAccepted());
#endif
}

void EMessages::lineEntered()
{
    line->releaseKeyboard();
    gra->exit_loop();
}

void EMessages::setIcons()
{
#ifdef EPROGRAMNAME
    char *title = EPROGRAMNAME;
#else
    char *title = "Electric";
#endif
#ifndef MACOS
    parentWidget()->setIcon( gra->programicon );
#endif
    (void)sprintf(gra_localstring, _("%s Messages"), title);
    parentWidget()->setCaption( gra_localstring );
    (void)strcpy(gra_localstring, _("Messages"));
    parentWidget()->setIconText( gra_localstring );
}

bool EMessages::eventFilter( QObject *watched, QEvent *e )
{
    if (e->type () == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *) e;
		if (watched == line && (ke->state() & ControlButton) && ke->key() == Key_D &&
			line->text().isEmpty()) {
			stopped = TRUE;
			lineEntered();
			return TRUE;
		}
		if (watched == scroll && ke->key() == Key_Space) ke->ignore ();
    } else if (e->type () == QEvent::Close) {
		if (watched == parentWidget())
		{
			ttyputerr(_("Cannot delete the messages window"));
			return TRUE;
		}
	}
    return QVBox::eventFilter( watched, e );
}

#if LANGTCL
INTBIG gra_initializetcl(void)
{
	INTBIG err;
	char *newArgv[2];

	/* set the program name/path */
	newArgv[0] = "Electric";
	newArgv[1] = NULL;
	(void)Tcl_FindExecutable(newArgv[0]);

	gra_tclinterp = Tcl_CreateInterp();
	if (gra_tclinterp == 0) error(_("from Tcl_CreateInterp"));

	/* tell Electric the TCL interpreter handle */
	el_tclinterpreter(gra_tclinterp);

	/* Make command-line arguments available in the Tcl variables "argc" and "argv" */
	Tcl_SetVar(gra_tclinterp, "argv", "", TCL_GLOBAL_ONLY);
	Tcl_SetVar(gra_tclinterp, "argc", "0", TCL_GLOBAL_ONLY);
	Tcl_SetVar(gra_tclinterp, "argv0", "electric", TCL_GLOBAL_ONLY);

	/* Set the "tcl_interactive" variable */
	Tcl_SetVar(gra_tclinterp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);

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
	REGISTER char *pt, *buf;
	REGISTER INTBIG len, filestatus;
	REGISTER void *infstr;

#ifdef ETRACE
	etrace(GTRACE, "{ setupenvironment: us_tool->USER_machine\n");
#endif

	/* set machine name */
	nextchangequiet();
#ifdef MACOSX
	(void)setval((INTBIG)us_tool, VTOOL, "USER_machine",
		(INTBIG)"MacintoshX", VSTRING|VDONTSAVE);
#endif
#ifdef WIN32
	(void)setval((INTBIG)us_tool, VTOOL, "USER_machine",
		(INTBIG)"Windows", VSTRING|VDONTSAVE);
#endif
#ifdef ONUNIX
	(void)setval((INTBIG)us_tool, VTOOL, "USER_machine",
		(INTBIG)"UNIX", VSTRING|VDONTSAVE);
#endif

	pt = getenv("ELECTRIC_LIBDIR");
	if (pt != NULL)
	{
		/* environment variable ELECTRIC_LIBDIR is set: use it */
		buf = (char *)emalloc(strlen(pt)+5, el_tempcluster);
		if (buf == 0)
		{
			fprintf(stderr, _("Cannot make environment buffer\n"));
			exitprogram();
		}
		strcpy(buf, pt);
		if (buf[strlen(buf)-1] != '/') strcat(buf, "/");
		(void)allocstring(&el_libdir, buf, db_cluster);
		efree(buf);
#ifdef ETRACE
		etrace(GTRACE, "} setupenvironment: el_libdir = %s\n", el_libdir);
#endif
		return;
	}

	/* try the #defined library directory */
	infstr = initinfstr();
	if (LIBDIR[0] != '/') addstringtoinfstr(infstr, currentdirectory());
	addstringtoinfstr(infstr, LIBDIR);
	addstringtoinfstr(infstr, CADRCFILENAME);
	pt = returninfstr(infstr);
#ifdef ETRACE
	etrace(GTRACE, "   setupenvironment: trying %s\n", pt);
#endif
	filestatus = fileexistence(pt);
	if (filestatus == 1 || filestatus == 3)
	{
		len = strlen(pt);
		pt[len-strlen(CADRCFILENAME)] = 0;
		(void)allocstring(&el_libdir, pt, db_cluster);
#ifdef ETRACE
		etrace(GTRACE, "} setupenvironment: el_libdir = %s\n", el_libdir);
#endif
		return;
	}

	/* try the current location (look for "lib") */
	infstr = initinfstr();
	addstringtoinfstr(infstr, currentdirectory());
	addstringtoinfstr(infstr, "lib/");
	addstringtoinfstr(infstr, CADRCFILENAME);
	pt = returninfstr(infstr);
#ifdef ETRACE
	etrace(GTRACE, "   setupenvironment: trying %s\n", pt);
#endif
	filestatus = fileexistence(pt);
	if (filestatus == 1 || filestatus == 3)
	{
		len = strlen(pt);
		pt[len-strlen(CADRCFILENAME)] = 0;
		(void)allocstring(&el_libdir, pt, db_cluster);
#ifdef ETRACE
		etrace(GTRACE, "} setupenvironment: el_libdir = %s\n", el_libdir);
#endif
		return;
	}

	/* no environment, #define, or current directory: look for ".cadrc" */
#ifdef ETRACE
	etrace(GTRACE, "   setupenvironment: trying %s\n", CADRCFILENAME);
#endif
	filestatus = fileexistence(CADRCFILENAME);
	if (filestatus == 1 || filestatus == 3)
	{
		/* found ".cadrc" in current directory: use that */
		(void)allocstring(&el_libdir, currentdirectory(), db_cluster);
#ifdef ETRACE
		etrace(GTRACE, "} setupenvironment: el_libdir = %s\n", el_libdir);
#endif
		return;
	}

	/* look for "~/.cadrc" */
	infstr = initinfstr();
	formatinfstr(infstr, "~/%s", CADRCFILENAME);
	pt = truepath(returninfstr(infstr));
#ifdef ETRACE
	etrace(GTRACE, "   setupenvironment: trying %s\n", pt);
#endif
	filestatus = fileexistence(pt);
	if (filestatus == 1 || filestatus == 3)
	{
		(void)allocstring(&el_libdir, pt, db_cluster);
#ifdef ETRACE
		etrace(GTRACE, "} setupenvironment: el_libdir = %s\n", el_libdir);
#endif
		return;
	}

	ttyputerr(_("Warning: cannot find '%s' startup file"), CADRCFILENAME);
	(void)allocstring(&el_libdir, ".", db_cluster);
#ifdef ETRACE
	etrace(GTRACE, "} setupenvironment: el_libdir = %s\n", el_libdir);
#endif
}

void setlibdir(char *libdir)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	if (libdir[0] != '/') addstringtoinfstr(infstr, currentdirectory());
	addstringtoinfstr(infstr, libdir);
	if (libdir[strlen(libdir)-1] != DIRSEP) addtoinfstr(infstr, DIRSEP);
	(void)reallocstring(&el_libdir, returninfstr(infstr), db_cluster);
#ifdef ETRACE
	etrace(GTRACE, "{}setlibdir: el_libdir = %s\n", el_libdir);
#endif
}

/******************** TERMINATION ********************/

EApplication::~EApplication()
{
	REGISTER INTBIG i;

	/* deallocate font cache */
	gra_termfaces();

#ifdef USEMDI
	/*???*/
#else
	if (messages) delete messages->parentWidget();
#endif

	gra_termgraph();

	killstringarray(gra_fileliststringarray);

	/* free printer list */
	for(i=0; i<gra_printerlistcount; i++)
		efree((char *)gra_printerlist[i]);
	if (gra_printerlisttotal > 0) efree((char *)gra_printerlist);
}

void termgraphics(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}termgraphics\n");
#endif
    delete gra;
    gra = 0;
}

void exitprogram(void)
{
	exit(0);
}

/******************** WINDOW CONTROL ********************/

/*
 * These routines implement multiple windows on the display (each display window
 * is called a "windowframe".  Currently these routines are unimplemented.
 */

WINDOWFRAME *newwindowframe(BOOLEAN floating, RECTAREA *r)
{
	WINDOWFRAME *wf, *oldlisthead;

#ifdef ETRACE
	etrace(GTRACE, "{ newwindowframe: floating=%d", floating);
	if (r != NULL) etrace(GTRACE_CONT, " r=%d,%d %d,%d", r->top, r->left, r->bottom, r->right);
	etrace(GTRACE_CONT, "\n");
#else
	Q_UNUSED( r );
#endif

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
	if (gra_buildwindow(QApplication::desktop(), wf, floating) != 0)
	{
		efree((char *)wf);
		el_firstwindowframe = oldlisthead;
#ifdef ETRACE
		etrace(GTRACE, "} newwindowframe: failed\n");
#endif
		return(NOWINDOWFRAME);
	}

	/* remember that this is the current window frame */
	if (!floating) el_curwindowframe = wf;

#ifdef ETRACE
	etrace(GTRACE, "} newwindowframe: wf=%d swid=%d shei=%d\n",
	       wf->windindex, wf->swid, wf->shei);
#endif
	return(wf);
}

void killwindowframe(WINDOWFRAME *owf)
{
	WINDOWFRAME *wf, *lastwf;

#ifdef ETRACE
	etrace(GTRACE, "{}killwindowframe: owf=%d gra_windowbeingdeleted=%d\n",
	       owf->windindex, gra_windowbeingdeleted);
#endif

	/* don't do this if window is being deleted from another place */
	if (gra_windowbeingdeleted != 0) return;

	/* kill the actual window, that will remove the frame, too */
#ifdef USEMDI
	/*??*/
#else
	delete owf->draw->parentWidget();
#endif

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

	efree((char *)owf);
}

void movedisplay(void)
{
#if 1
	WINDOWFRAME *wf = getwindowframe(1);
	if (wf == NOWINDOWFRAME) return;
#  ifdef ETRACE
	etrace(GTRACE, "{ movedisplay: wf->windindex=%d\n",
		wf->windindex);
#  endif
	QWidget *qwin = wf->draw->parentWidget();
	QWidget *newDesktop;
	QDesktopWidget *desktop = QApplication::desktop();
	newDesktop = desktop->screen( (desktop->screenNumber(qwin) + 1) % desktop->numScreens() );
	qwin->deleteLater();
	qwin = NULL;
	if (gra_buildwindow(newDesktop, wf, wf->floating) != 0)
	  ttyputerr(_("Problem moving the display"));

	/* redisplay */
	us_drawmenu(1, wf);
	us_redostatus(wf);
	us_endbatch();
#  ifdef ETRACE
	etrace(GTRACE, "} movedisplay: wf->windindex=%d\n",
		wf->windindex);
#  endif
#else
	ttyputmsg(_("(Display move works only with Qt version 3\n"));
#endif
}

WINDOWFRAME *getwindowframe(BOOLEAN canfloat)
{
#ifdef ETRACE
	etrace(GTRACE, "{}getwindowframe: canfloat=%d el_curwindowframe=%#x windindex=%d\n",
	       canfloat, el_curwindowframe,
	       el_curwindowframe != NOWINDOWFRAME ? el_curwindowframe->windindex : -1);
#else
	Q_UNUSED( canfloat );
#endif
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
    QRect geom = gra->messages->parentWidget()->geometry();
    *top = geom.top();
    *left = geom.left();
    *bottom = geom.bottom();
    *right = geom.right();
#ifdef ETRACE
	etrace(GTRACE, "{}getmessagesframeinfo: top=%d left=%d bottom=%d right=%d\n",
	       *top, *left, *bottom, *right);
#endif
}

/*
 * Routine to set the size and position of the messages window.
 */
void setmessagesframeinfo(INTBIG top, INTBIG left, INTBIG bottom, INTBIG right)
{
#ifdef ETRACE
	etrace(GTRACE, "{}setmessagesframeinfo: top=%d left=%d bottom=%d right=%d\n",
	       top, left, bottom, right);
#endif
    QRect geom( QPoint( left, top ) , QPoint( right, bottom ) );
    gra->messages->parentWidget()->setGeometry( geom );
}

void sizewindowframe(WINDOWFRAME *wf, INTBIG wid, INTBIG hei)
{
	static WINDOWFRAME *lastwf = 0;
	static INTBIG lastwid, lasthei, repeat;

#ifdef ETRACE
	etrace(GTRACE, "{ sizewindowframe: windindex=%d wid=%d hei=%d\n",
	       wf->windindex, wid, hei);
#endif
	if (wid == wf->draw->parentWidget()->width() && hei == wf->draw->parentWidget()->height())
	{
#ifdef ETRACE
		etrace(GTRACE, "} sizewindowframe: nothing to do\n");
#endif
		return;
	}
	if (wf == lastwf && wid == lastwid && hei == lasthei)
	{
		repeat++;
		if (repeat > 2)
		{
#ifdef ETRACE
			etrace(GTRACE, "} sizewindowframe: failed repeat=%d\n", repeat);
#endif
			return;
		}
	} else
	{
		lastwf = wf;   lastwid = wid;   lasthei = hei;   repeat = 1;
	}
	gra_internalchange = ticktime();
#ifdef ETRACE
	etrace(GTRACE, "  sizewindowframe: resize to %d %d\n", wid, hei);
#endif
	wf->draw->parentWidget()->resize(wid, hei);
#ifdef ETRACE
	etrace(GTRACE, "} sizewindowframe: done\n");
#endif
}

void movewindowframe(WINDOWFRAME *wf, INTBIG left, INTBIG top)
{
#ifdef ETRACE
	etrace(GTRACE, "{}movewindowframe: windindex=%d left=%d top=%d\n",
	       wf->windindex, left, top);
#endif
#ifdef MACOS
	/* allow for menubar */
	if (wf->floating != 0 && top < WMTOPBORDERMAC) top = WMTOPBORDERMAC;
#endif
	if (left == wf->draw->parentWidget()->x() && top == wf->draw->parentWidget()->y()) return;
	gra_internalchange = ticktime();
	wf->draw->parentWidget()->move(left, top);
}

/*
 * Routine to close the messages window if it is in front.  Returns true if the
 * window was closed.
 */
BOOLEAN closefrontmostmessages(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}closefrontmostmessages:\n");
#endif

	return(FALSE);
}

/*
 * Routine to bring window "wf" to the front.
 */
void bringwindowtofront(WINDOWFRAME *wf)
{
#ifdef ETRACE
	etrace(GTRACE, "{}bringwindowtofront: windindex\n", wf->windindex);
#endif
	wf->draw->parentWidget()->raise();
	wf->draw->setFocus();
}

/*
 * Routine to organize the windows according to "how" (0: tile horizontally,
 * 1: tile vertically, 2: cascade).
 */
void adjustwindowframe(INTBIG how)
{
#ifdef ETRACE
	etrace(GTRACE, "{ adjustwindowframe: how=%d\n", how);
#endif

	gra_adjustwindowframeon(how);

#ifdef ETRACE
	etrace(GTRACE, "} adjustwindowframe:\n");
#endif
}

void gra_adjustwindowframeon(INTSML how)
{
	RECTAREA r, wr;
	REGISTER INTBIG children, child, sizex, sizey, scr;
	REGISTER WINDOWFRAME *wf, *compmenu;

	/* loop through the screens */
	for(scr=0; scr<gra->desktop()->numScreens(); scr++)
	{
		/* determine area for windows */
		QRect qr = gra->desktop()->screenGeometry(scr);
		r.left = qr.left();   r.right = qr.right();
#ifdef MACOSX
		r.top = qr.top() + WMTOPBORDERMAC;
#else
		r.top = qr.top();
#endif
		r.bottom = qr.bottom();

		/* figure out how many windows need to be rearranged */
		children = 0;
		compmenu = 0;
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (!gra_onthiswindow(wf->draw->parentWidget(), &r)) continue;
			if (wf->floating == 0) children++; else
				compmenu = wf;
		}
		if (children <= 0) return;

		if (compmenu != NULL)
		{
			/* remove component menu from area of tiling */
			if (gra_onthiswindow(compmenu->draw->parentWidget(), &r))
				gra_removewindowextent(&r, compmenu->draw->parentWidget());
		}
		if (gra->messages != 0)
		{
			/* remove messages menu from area of tiling */
			if (gra_onthiswindow(gra->messages->parentWidget(), &r))
				gra_removewindowextent(&r, gra->messages->parentWidget());
		}

		/* rearrange the windows */
		child = 0;
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (!gra_onthiswindow(wf->draw->parentWidget(), &r)) continue;
			if (wf->floating != 0) continue;
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
#ifdef MACOS
			wr.left += 1;
			wr.bottom -= 23;
#else
			wr.bottom -= WMTOPBORDER;
#endif
			sizewindowframe(wf, wr.right-wr.left, wr.bottom-wr.top);
			movewindowframe(wf, wr.left, wr.top);
			child++;
		}
	}
}

/*
 * Helper routine to determine whether window "window" is on the display bounded by "left",
 * "right", "top" and "bottom".  Returns true if so.
 */
BOOLEAN gra_onthiswindow(QWidget *wnd, RECTAREA *r)
{
	REGISTER INTBIG pixels, onpixels;

    QRect er = wnd->geometry();

	pixels = (er.right() - er.left()) * (er.bottom() - er.top());
	if (er.left() > r->right || er.right() < r->left ||
		er.top() > r->bottom || er.bottom() < r->top) return(FALSE);
	if (er.left() < r->left) er.rLeft() = r->left;
	if (er.right() > r->right) er.rRight() = r->right;
	if (er.top() < r->top) er.rTop() = r->top;
	if (er.bottom() > r->bottom) er.rBottom() = r->bottom;
	onpixels = (er.right() - er.left()) * (er.bottom() - er.top());
	if (onpixels * 2 >= pixels) return(TRUE);
	return(FALSE);
}

/*
 * Helper routine to remove the location of window "wnd" from the rectangle "r".
 */
void gra_removewindowextent(RECTAREA *r, QWidget *wnd)
{
    QRect er = wnd->frameGeometry();

    if (er.width() > er.height())
    {
	    /* horizontal occluding window */
	    if (er.bottom() - r->top < r->bottom - er.top())
	    {
		    /* occluding window on top */
		    r->top = er.bottom();
	    } else
	    {
		    /* occluding window on bottom */
		    r->bottom = er.top();
	    }
    } else
    {
	    /* vertical occluding window */
	    if (er.right() - r->left < r->right - er.left())
	    {
		    /* occluding window on left */
		    r->left = er.right();
	    } else
	    {
		    /* occluding window on right */
		    r->right = er.left();
	    }
    }
}

void getpaletteparameters(INTBIG *wid, INTBIG *hei, INTBIG *palettewidth)
{
#ifdef USEMDI
	*wid = gra->mw->ws->width();
	*hei = gra->mw->ws->height();
#else
	*wid = gra->desktop()->width() - WMLEFTBORDER;
	*hei = gra->desktop()->height() - WMTOPBORDER;
#endif
	*palettewidth = PALETTEWIDTH;
#ifdef ETRACE
	etrace(GTRACE, "{}getpaletteparameters: wid=%d hei=%d palettewidth=%d\n",
	       *wid, *hei, *palettewidth);
#endif
}

/*
 * Routine called when the component menu has moved to a different location
 * on the screen (left/right/top/bottom).  Resets local state of the position.
 */
void resetpaletteparameters(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}resetpaletteparameters:\n");
#endif
}

GraphicsDraw::GraphicsDraw( QWidget *parent )
  : QWidget( parent, "draw", WRepaintNoErase )
{
    setMouseTracking( TRUE );
    setFocusPolicy( QWidget::StrongFocus );
	metaheld = false;
}

bool GraphicsDraw::eventFilter( QObject *watched, QEvent *e )
{
    if (e->type () == QEvent::Close) {
		if (watched == parentWidget())
		{
#ifdef ETRACE
			etrace(GTRACE, "{ GraphicsDraw::eventFilter::Close %s\n", stateNames[gra_state]);
#endif
			/* queue this frame for deletion */
			gra_deletethisframe = wf;
			gra->toolTimeSlice();
#ifdef ETRACE
			etrace(GTRACE, "} GraphicsDraw::eventFilter::Close\n");
#endif
			return TRUE;
		}
	}
    return QWidget::eventFilter( watched, e );
}

#ifdef DOUBLESELECT
static BOOLEAN activationChanged = FALSE;
static UINTBIG oldActivationTick = 0;
#  define ACTIVATION_TICKS 10
#endif
void GraphicsDraw::windowActivationChange( bool oldActive )
{
#ifdef DOUBLESELECT
	//ttyputmsg(M_("windowActivationChange %s%d %d %ld"), isActiveWindow()?"+":"-", oldActive, wf->windindex, ticktime());
	if ( oldActive )
	{
		gra->activationChange();
	} else
    {
		gra_setcurrentwindowframe(wf);
		activationChanged = (ticktime() - oldActivationTick > ACTIVATION_TICKS);
/*		ttyputmsg(M_("dt=%ld"), ticktime() - oldActivationTick); */
	}
#else
	if (isActiveWindow())
		gra_setcurrentwindowframe(wf);
#endif
}

void EMessages::windowActivationChange( bool oldActive )
{
#ifdef DOUBLESELECT
	if ( oldActive )
	{
		gra->activationChange();
	}
#else
	Q_UNUSED( oldActive );
#endif
}

#ifdef DOUBLESELECT
void EApplication::activationChange()
{
	oldActivationTick = ticktime();
}
#endif

void GraphicsDraw::focusInEvent( QFocusEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::focusInEvent %s\n", stateNames[gra_state]);
#endif
    QWidget::focusInEvent( e );
#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::focusInEvent\n");
#endif
}

void GraphicsDraw::focusOutEvent( QFocusEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::focusOutEvent %s\n", stateNames[gra_state]);
#endif
    QWidget::focusOutEvent( e );
#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::focusOutEvent\n");
#endif
}

void GraphicsDraw::keyPressEvent( QKeyEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::keyPressEvent %s accepted=%d\n", stateNames[gra_state], e->isAccepted());
#endif

    INTBIG special;
	BOOLEAN keepon;

	int state = gra->translatekey(e, &special);
    if (state != 0 || special != 0)
    {
        us_state |= DIDINPUT;
		INTBIG cmd = state;
		keepCursorPos( mapFromGlobal( QCursor::pos() ) );
#ifdef ETRACE
		etrace(GTRACE, "  GraphicsDraw::keyPressEvent: %s state=%#o cmd=%#o special=%#o DIDINPUT%d,%d\n",
			stateNames[gra_state], state, cmd, special, gra_cursorx, gra_cursory );
#endif

		switch (gra_state)
		{
			case S_USER:
#ifdef ETRACE
				etrace(GTRACE, "{ us_oncommand: cmd=%#o\n", cmd);
#endif
				us_oncommand( cmd, special );
				gra->toolTimeSlice();
#ifdef ETRACE
				etrace(GTRACE, "} us_oncommand\n");
#endif
				break;
			case S_TRACK:
#ifdef ETRACE
				etrace(GTRACE, "  trackcursor { eachchar x=%d y=%d cmd=%#o\n", gra_cursorx, gra_cursory, cmd);
#endif
				keepon = (*gra->eachchar)(gra_cursorx, gra_cursory, cmd);
				flushscreen();
#ifdef ETRACE
				etrace(GTRACE, "  trackcursor } eachchar: keepon=%d el_pleasestop=%d\n", keepon, el_pleasestop);
#endif
				if (keepon || el_pleasestop) gra->exit_loop();
				break;
			case S_MODAL:
#ifdef ETRACE
				etrace(GTRACE, "  modalloop { charhandler cmd=%#o special=%#o\n", cmd, special);
#endif
				keepon = (*gra->charhandler)(cmd, special);
				flushscreen();
#ifdef ETRACE
				etrace(GTRACE, "  modalloop } charhandler: keepon=%d el_pleasestop=%d\n", keepon, el_pleasestop);
#endif
				if (keepon || el_pleasestop) gra->exit_loop();
				break;
            default:
                break;
		}
		e->accept();
	}
	/* Linux Meta key modifier does not come through in QKeyEvent->state() for some reason
	   so we fake it by setting metaheld here, and unsetting it in KeyReleaseEvent */
	else if ((e->key() == Key_Super_L) || (e->key() == Key_Super_R)) {
		metaheld = true;
#ifdef ETRACE
		etrace(GTRACE, "  metaheld set to true");
#endif
	}
    else QWidget::keyPressEvent( e );
#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::keyPressEvent accepted=%d\n", e->isAccepted());
#endif
}

void GraphicsDraw::keyReleaseEvent( QKeyEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::keyReleaseEvent %s accepted=%d\n", stateNames[gra_state], e->isAccepted());
#endif
	/* Linux Meta key modifier does not come through in QKeyEvent->state() for some reason
	   so we fake it by setting metaheld in KeyPressEvent, and unsetting it here */
	if ((e->key() == Key_Super_L) || (e->key() == Key_Super_R)) {
		metaheld = false;
#ifdef ETRACE
		etrace(GTRACE, "  metaheld set to false");
#endif
	}
	else QWidget::keyReleaseEvent( e );
#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::keyReleaseEvent accepted=%d\n", e->isAccepted());
#endif
}


INTSML gra_buildwindow(QWidget *desktop, WINDOWFRAME *wf, BOOLEAN floating)
{
	wf->floating = floating;
	int x, y, width, height;

	/* determine window position */
#ifdef USEMDI
	int screenw = gra->mw->ws->width();
	int screenh = gra->mw->ws->height();
#else
	int screenw = desktop->width();
	int screenh = desktop->height();
#endif
	if (!floating)
	{
		x = gra_editposx;
		y = gra_editposy;
		width = gra_editsizex;
		height = gra_editsizey;
		x += (gra_windownumber%5) * 40;
		y += (gra_windownumber%5) * 40;
		if (x + width > screenw)
			width = screenw - x;
		if (y + height > screenh)
			height = screenh - y;
		gra_windownumber++;
	} else
	{
		x = 0;
		y = 1;
		width = PALETTEWIDTH;
		height = screenh - 50;
		gra_internalchange = ticktime();
	}

	/*
	 * make top-level graphics widget
	 */
#ifdef USEMDI
#ifdef ETRACE
	etrace(GTRACE, "  gra_buildwindow: screenw=%d screenh=%d width=%d height=%d\n", screenw, screenh, width, height);
#endif
	wf->draw = new GraphicsDraw( gra->mw->ws );
	wf->draw->wf = wf;

	/* get window size and depth after creation */
	wf->swid = wf->shei = -1;
	wf->rowstart = 0;
	QWidget *qwin = wf->draw->parentWidget();
#ifdef ETRACE
	etrace(GTRACE, "qwin->className=%s\n", qwin->className());
#endif
	gra->sendPostedEvents( qwin, QEvent::ChildInserted );
	qwin = wf->draw->parentWidget();
#ifdef ETRACE
	etrace(GTRACE, "qwin->className=%s\n", qwin->className());
#endif

#else
	EMainWindow *qwin = new EMainWindow( desktop, "GraphicsMainWindow", Qt::WType_TopLevel, !floating );
	if (!floating) qwin->pulldownmenuload();
	wf->draw = new GraphicsDraw( qwin );
	wf->draw->wf = wf;
	qwin->setCentralWidget( wf->draw );
#endif

	/* load window manager information */
#ifndef MACOS
	qwin->setIcon( gra->programicon );
#endif
	if (!floating) (void)strcpy(gra_localstring, _("Electric")); else
		(void)strcpy(gra_localstring, _("Components"));
	qwin->setCaption( gra_localstring );
	qwin->setIconText( gra_localstring );

    qwin->installEventFilter( wf->draw );
	qwin->setFocusProxy( wf->draw );
	qwin->resize( width, height );
	qwin->move( x, y );

	/* get window size and depth after creation */
	wf->swid = wf->shei = -1;
	wf->rowstart = 0;
#ifdef USEMDI
	wf->draw->show();
#endif
	qwin->show();

	/* reload cursor */
	us_cursorstate = -1;
	setdefaultcursortype(us_normalcursor);

	/* fill the color map  */
#if 0
	us_getcolormap(el_curtech, COLORSDEFAULT, FALSE);
#endif
	gra_reloadmap();

	return(0);
}

/******************** MISCELLANEOUS EXTERNAL ROUTINES ********************/

/*
 * return nonzero if the capabilities in "want" are present
 */
BOOLEAN graphicshas(INTBIG want)
{
	BOOLEAN has = gra_graphicshas(want);
#ifdef ETRACE
	etrace(GTRACE, "{}graphicshas: want=%#o has=%d\n", want, has);
#endif
	return(has);
}

static INTSML gra_graphicshas(INTSML want)
{
	if ((want & CANHAVENOWINDOWS) != 0) return(0);
	return(1);
}


/*
 * Routine to make sound "sound".  If sounds are turned off, no
 * sound is made (unless "force" is TRUE)
 */
void ttybeep(INTBIG sound, BOOLEAN force)
{
#ifdef ETRACE
	etrace(GTRACE, "{}ttybeep force=%d\n", force);
#endif
	if ((us_tool->toolstate & TERMBEEP) != 0 || force)
	{
		switch (sound)
		{
			case SOUNDBEEP:
				QApplication::beep();
				break;
			case SOUNDCLICK:
				if ((us_useroptions&NOEXTRASOUND) != 0) break;
				if (QSound::available())
					gra_clicksound->play();
				break;
		}
	}
}

DIALOGITEM db_severeerrordialogitems[] =
{
 /*  1 */ {0, {80,8,104,72}, BUTTON, N_("Exit")},
 /*  2 */ {0, {80,96,104,160}, BUTTON, N_("Save")},
 /*  3 */ {0, {80,184,104,256}, BUTTON, N_("Continue")},
 /*  4 */ {0, {8,8,72,256}, MESSAGE, ""}
};
DIALOG db_severeerrordialog = {{50,75,163,341}, N_("Fatal Error"), 0, 4, db_severeerrordialogitems, 0, 0};

/* special items for the severe error dialog: */
#define DSVE_EXIT      1		/* Exit (button) */
#define DSVE_SAVE      2		/* Save (button) */
#define DSVE_CONTINUE  3		/* Continue (button) */
#define DSVE_MESSAGE   4		/* Error Message (stat text) */

void error(char *s, ...)
{
	va_list ap;
	char line[500];
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
#ifdef ETRACE
	etrace(GTRACE, "{ error: line=<%s> us_logplay=NULL\n", line);
#endif

	/* print to stderr if graphics not initiaized */
	if (gra == 0) {
		fprintf(stderr, "Fatal Error: %s\n", line);
#ifdef ETRACE
		etrace(GTRACE, "} error stderr\n");
#endif
		return;
	}

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
#ifdef ETRACE
			etrace(GTRACE, "  error: saving libraries\n");
#endif
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
				retval = asktool(io_tool, "write", (INTBIG)lib, (INTBIG)"binary");
				restoreoptionstate(lib);
				if (retval != 0)
				{
#ifdef ETRACE
					etrace(GTRACE, "} error: saving libraries failed\n");
#endif
					return;
				}
			}
		}
	}
	DiaDoneDialog(dia);
#ifdef ETRACE
	etrace(GTRACE, "} error\n");
#endif
}

/*
 * Routine to get the environment variable "name" and return its value.
 */
char *egetenv(char *name)
{
	char *value = getenv(name);
#ifdef ETRACE
	etrace(GTRACE, "{}getenv: name=%s value=<%s>\n", name, value != NULL ? value : "NULL");
#endif
	return(value);
}

/*
 * Routine to get the current language that Electric speaks.
 */
char *elanguage(void)
{
	char *lang;

	lang = getenv("LANGUAGE");
	if (lang == 0) lang = "en";
	return(lang);
}

/*
 * Routine to run the string "command" in a shell.
 * Returns nonzero if the command cannot be run.
 */
INTBIG esystem(char *command)
{
#ifdef ETRACE
	etrace(GTRACE, "{}esystem: command=<%s>\n", command);
#endif
	system(command);
	return(0);
}

/*
 * Routine to execute the program "program" with the arguments "args"
 */
void eexec(char *program, char *args[])
{
#ifdef ETRACE
	char **arg;

	etrace(GTRACE, "{}exec: program=<%s> args=", program);
	for (arg = args; *arg != NULL; arg++)
	{
		etrace(GTRACE_CONT, " <%s>", *arg);
	}
	etrace(GTRACE_CONT, "\n");
#endif
	execvp(program, args);
}

/*
 * Routine to return the number of processors on this machine.
 */
INTBIG enumprocessors(void)
{
#ifdef ONUNIX
	return(sysconf(_SC_NPROCESSORS_ONLN));
#endif
#ifdef MACOS
	return(MPProcessors());
#endif
	return(1);
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
		printf("Thread creation failed\n");
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

	mutexid = calloc(1, sizeof (mutex_t));
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
char **eprinterlist(void)
{
	char **list;
#ifdef ETRACE
	char **p;

	etrace(GTRACE, "{ eprinterlist:\n");
#endif
	list = gra_eprinterlist();
#ifdef ETRACE
	etrace(GTRACE, "} eprinterlist:");
	for (p = list; *p != NULL; p++) etrace(GTRACE_CONT, " <%s>", *p);
	etrace(GTRACE_CONT, "\n");
#endif
	return(list);
}

static char **gra_eprinterlist(void)
{
	static char *nulllplist[1];
	INTBIG i;
	char buf[200], *pt;
	FILE *io;

	/* remove former list */
	for(i=0; i<gra_printerlistcount; i++)
		efree((char *)gra_printerlist[i]);
	gra_printerlistcount = 0;
	nulllplist[0] = 0;

	/* look for "/etc/printcap" */
	io = fopen("/etc/printcap", "r");
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
			if (gra_addprinter(buf) != 0)
				return(nulllplist);
		}
		fclose(io);
		if (gra_printerlistcount <= 0) return(nulllplist);
		return(gra_printerlist);
	}

	/* no file "/etc/printcap": try to run "lpget" */
	QProcess lpget;
	lpget.addArgument( "lpget" );
	lpget.addArgument( "list" );
	if ( !lpget.launch( QString::null ) )
		return(nulllplist);
	do
	{
		if (lpget.canReadLineStdout())
		{
			QString qstr = lpget.readLineStdout();
			int pos = qstr.find(':');
			if (pos < 0) continue;
			qstr.remove( 0, pos + 1 );
			char *str = EApplication::localize( qstr );
			gra_addprinter( str );
			continue;
		}
	} while( lpget.isRunning() );
	if (gra_printerlistcount <= 0) return(nulllplist);
	return(gra_printerlist);
}

INTSML gra_addprinter(char *buf)
{
	char **newlist;
	INTBIG i, j, newtotal;

	if (strcmp(buf, "_default") == 0) return(0);
	if (buf[0] == ' ' || buf[0] == '\t') return(0);

	/* stop if it is already in the list */
	for(i=0; i<gra_printerlistcount; i++)
		if (strcmp(buf, gra_printerlist[i]) == 0) return(0);

	/* make room in the list */
	if (gra_printerlistcount >= gra_printerlisttotal-1)
	{
		newtotal = gra_printerlistcount + 10;
		newlist = (char **)emalloc(newtotal * (sizeof (char *)), us_tool->cluster);
		if (newlist == 0) return(1);
		for(i=0; i<gra_printerlistcount; i++)
			newlist[i] = gra_printerlist[i];
		if (gra_printerlisttotal > 0) efree((char *)gra_printerlist);
		gra_printerlist = newlist;
		gra_printerlisttotal = newtotal;
	}

	/* insert into the list */
	for(i=0; i<gra_printerlistcount; i++)
		if (strcmp(buf, gra_printerlist[i]) < 0) break;
	for(j=gra_printerlistcount; j>i; j--) gra_printerlist[j] = gra_printerlist[j-1];
	gra_printerlist[i] = (char *)emalloc(strlen(buf)+1, us_tool->cluster);
	strcpy(gra_printerlist[i], buf);
	gra_printerlistcount++;
	gra_printerlist[gra_printerlistcount] = 0;
	return(0);
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
	INTSML i, j;
	REGISTER WINDOWFRAME *wf;

#ifdef ETRACE
	etrace(GTRACE, "{}ttyfreestatusfield: %s\n", sf->label);
#endif
	for(i=0; i<gra_indicatorcount; i++)
	{
		if (gra_statusfields[i] != sf) continue;

		/* remove the field */
		efree(gra_statusfieldtext[i]);
#ifdef USEMDI
		gra->mw->removeStatusItem( i );
#else
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (wf->floating) continue;
			EMainWindow *qwin = (EMainWindow*)wf->draw->parentWidget();
			qwin->removeStatusItem( i );
		}
#endif

		for(j=i+1; j<gra_indicatorcount; j++)
		{
			gra_statusfields[j-1] = gra_statusfields[j];
			gra_statusfieldtext[j-1] = gra_statusfieldtext[j];
		}
		gra_indicatorcount--;
	}

	efree(sf->label);
	efree((char *)sf);
}

/*
 * Routine to display "message" in the status "field" of window "frame" (uses all windows
 * if "frame" is zero).  If "cancrop" is false, field cannot be cropped and should be
 * replaced with "*" if there isn't room.
 */
void ttysetstatusfield(WINDOWFRAME *wwf, STATUSFIELD *field, char *message, BOOLEAN cancrop)
{
	INTSML len, i;
	REGISTER WINDOWFRAME *wf;

#ifdef ETRACE_
	etrace(GTRACE, "{%cttysetstatusfield: wwf=%d field=%s message=<%s> cancrop=%d\n",
	       field != 0 ? ' ' : '}',
	       wwf != NOWINDOWFRAME ? wwf->windindex : -1,
	       field != 0 ? field->label : "<NULL>",
	       message != NULL ? message : "<NULL>",
	       cancrop);
#else
	Q_UNUSED( cancrop );
#endif

	/* figure out which indicator this is */
	if (field == 0) return;

	/* if this is a title bar setting, do it now */
	if (field->line == 0)
	{
		if (wwf == NOWINDOWFRAME) return;
			/* should set title bar here */
#ifdef EPROGRAMNAME
		(void)strcpy(gra_localstring, EPROGRAMNAME);
#else
		(void)strcpy(gra_localstring, _("Electric"));
#endif
		if (strlen(message) > 0)
		{
			(void)strcat(gra_localstring, " (");
			(void)strcat(gra_localstring, field->label);
			(void)strcat(gra_localstring, message);
			(void)strcat(gra_localstring, ")");
		}
		len = strlen(gra_localstring);
		while (len > 0 && gra_localstring[len-1] == ' ') gra_localstring[--len] = 0;
		wwf->draw->parentWidget()->setCaption( gra_localstring );
		return;
	}

	/* ignore null fields */
	if (*field->label == 0) return;

	/* construct the status line */
	len = strlen(field->label);
	if (len + strlen(message) >= MAXLOCALSTRING)
	{
		strcpy(gra_localstring, field->label);
		i = MAXLOCALSTRING - len - 1;
		strncat(gra_localstring, message, i);
		gra_localstring[MAXLOCALSTRING-1] = 0;
	} else
	{
		sprintf(gra_localstring, "%s%s", field->label, message);
	}
	len = strlen(gra_localstring);
	while (len > 0 && gra_localstring[len-1] == ' ') gra_localstring[--len] = 0;

	/* see if this indicator is in the list */
	for(i=0; i<gra_indicatorcount; i++)
	{
		if (gra_statusfields[i]->line != field->line ||
			gra_statusfields[i]->startper != field->startper ||
			gra_statusfields[i]->endper != field->endper) continue;

		/* load the string */
#ifdef USEMDI
		gra->mw->changeStatusItem( i, gra_localstring );
#else
		for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		{
			if (wf->floating) continue;
			if (wwf != NOWINDOWFRAME && wwf != wf) continue;
			EMainWindow *qwin = (EMainWindow*)wf->draw->parentWidget();
			qwin->changeStatusItem( i, gra_localstring );
		}	
#endif
        (void)reallocstring(&gra_statusfieldtext[i], gra_localstring, db_cluster);
        return;
	}

	/* not found: find where this field fits in the order */
	(void)allocstring(&gra_statusfieldtext[i], gra_localstring, db_cluster);
#ifdef USEMDI
	gra->mw->addStatusItem( field->endper - field->startper, gra_localstring );
#else
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;
		EMainWindow *qwin = (EMainWindow*)wf->draw->parentWidget();
		qwin->addStatusItem( field->endper - field->startper, gra_localstring );
	}
#endif
	gra_statusfields[gra_indicatorcount++] = field;
#ifdef ETRACE_
	etrace(GTRACE, "} ttysetstatusfield\n");
#endif
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
void putmessagesstring(char *s, BOOLEAN important)
{
#ifdef ETRACE 
	etrace(GTRACE, "{}putmessagesstring: <%s> important=%d\n", s, important);
#endif
	/* print to stderr if messages are not initiaized */
	if (gra == 0 || gra->messages == 0) {
		fprintf(stderr, "%s\n", s);
		return;
	}
	gra->messages->putString( s, important );
}

/*
 * Routine to return the name of the key that ends a session from the messages window.
 */
char *getmessageseofkey(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}getmessageseofkey: ^D\n");
#endif
	return("^D");
}

char *getmessagesstring(char *prompt)
{
#ifdef ETRACE
	etrace(GTRACE, "{ getmessagesstring: prompt=<%s>\n", prompt);
#endif
	char *str = gra->messages->getString( prompt );
#ifdef ETRACE
	etrace(GTRACE, "} getmessagesstring: <%s> gra_state=%s\n",
	       str ? str : "STOPPED", stateNames[gra_state]);
#endif
	return(str);
}

/*
 * routine to select fonts in the messages window
 */
void setmessagesfont(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{ setmessagesfont\n");
#endif
	gra->messages->setFont();
#ifdef ETRACE
	etrace(GTRACE, "} setmessagesfont\n");
#endif
}

/*
 * routine to cut text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN cutfrommessages(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}cutrommessages: result=0\n");
#endif
	return(FALSE);
}

/*
 * routine to copy text from the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN copyfrommessages(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}copyfrommessages: result=0\n");
#endif
	return(FALSE);
}

/*
 * routine to paste text to the messages window if it is current.  Returns true
 * if sucessful.
 */
BOOLEAN pastetomessages(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}pastetomessages: result=0\n");
#endif
	return(FALSE);
}

/*
 * routine to get the contents of the system cut buffer
 */
char *getcutbuffer(void)
{
	char *string;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, EApplication::localize( EApplication::clipboard()->text() ) );
	string = returninfstr(infstr);
#ifdef ETRACE
	etrace(GTRACE, "{}getcutbuffer: <%s>\n", string);
#endif
	return(string);
}
        
/*
 * routine to set the contents of the system cut buffer to "msg"
 */
void setcutbuffer(char *msg)
{
#ifdef ETRACE
	etrace(GTRACE, "{}setcutbuffer: msg=<%s>\n", msg);
#endif
	EApplication::clipboard()->setText( QString::fromLocal8Bit( msg ) );
}

/*
 * Routine to clear all text from the messages window.
 */
void clearmessageswindow(void)
{
	gra->messages->clear();
#ifdef ETRACE
	etrace(GTRACE, "{}clearmessageswindow\n");
#endif
}

/******************** GRAPHICS CONTROL ROUTINES ********************/

void flushscreen(void)
{
	REGISTER WINDOWFRAME *wf;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		/* nothing to do if no changes have been made to offscreen buffer */
		QRect r;
		if (!wf->offscreendirty) continue;
		wf->offscreendirty = FALSE;
		r = QRect( wf->copyleft, wf->copytop,
			   wf->copyright - wf->copyleft, wf->copybottom - wf->copytop );
		wf->draw->repaint( r, FALSE );
	}
	qApp->flush();
}

/*
 * Routine to mark an area of the offscreen buffer as "changed".
 */
void gra_setrect(WINDOWFRAME *wf, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	static INTBIG checktimefreq = 0;

	if (gra_noflush) return;
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

/*
 * Routine to change the default cursor (to indicate modes).
 */
void setnormalcursor(INTBIG curs)
{
#ifdef ETRACE
	etrace(GTRACE, "{ setnormalcursor: curs=%d\n", curs);
#endif
	us_normalcursor = curs;
	setdefaultcursortype(us_normalcursor);
#ifdef ETRACE
	etrace(GTRACE, "} setnormalcursor\n", curs);
#endif
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
	REGISTER WINDOWFRAME *wf;
	REGISTER INTSML i;
	REGISTER INTBIG r, g, b;

#ifdef ETRACE
	etrace(GTRACE, "{ colormapload: low=%d high=%d\n", low, high);
#endif
	/* clip to the number of map entries requested */
	if (high >= el_maplength) high = el_maplength - 1;

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		for(i=low; i<=high; i++)
		{
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

			gra_colortable[i] = qRgb( r, g, b );
			wf->draw->image.setColor( i, gra_colortable[i] );
		}
		/* mark the entire screen for redrawing */
		gra_setrect(wf, 0, wf->swid, 0, wf->shei);
	}

	flushscreen();

	/* change messages window colors */
	if (low == 0 && high == 255)
	{
		QColor bg( blue[0], green[0], red[0]);
		QColor fg( blue[CELLTXT], green[CELLTXT], red[CELLTXT]);
		gra->messages->setColor( bg, fg );
	}

#ifdef ETRACE
	etrace(GTRACE, "} colormapload\n");
#endif
}

/*
 * Helper routine to set the cursor shape to "state".
 */
void setdefaultcursortype(INTBIG state)
{
	WINDOWFRAME *wf;

	if (us_cursorstate == state) return;

    QCursor cursor = gra->realcursor;
    switch (state)
    {
        case NORMALCURSOR:
            cursor = gra->realcursor;
            break;
        case WANTTTYCURSOR:
            cursor = gra->nomousecursor;
            break;
        case PENCURSOR:
            cursor = gra->drawcursor;
            break;
        case NULLCURSOR:
            cursor = gra->nullcursor;
            break;
        case MENUCURSOR:
            cursor = gra->menucursor;
            break;
        case HANDCURSOR:
            cursor = gra->handcursor;
            break;
        case TECHCURSOR:
            cursor = gra->techcursor;
            break;
        case IBEAMCURSOR:
            cursor = gra->ibeamcursor;
            break;
        case LRCURSOR:
            cursor = gra->lrcursor;
            break;
        case UDCURSOR:
            cursor = gra->udcursor;
            break;
    }

	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
        wf->draw->setCursor( cursor );
	}

#ifdef ETRACE
	etrace(GTRACE, "  setdefaultcursortype: oldcursor=%d newcursor=%d\n",
	       us_cursorstate, state);
#endif
	us_cursorstate = state;
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
#ifdef MACOS
	if (b == 16) return(TRUE);
#else
	if (b >= BUTTONS - REALBUTS) return(TRUE);
#endif
	return(FALSE);
}

/*
 * routine to tell whether button "but" is a context button (right)
 */
BOOLEAN contextbutton(INTBIG b)
{
#ifdef MACOS
	if ((b%4) >= 2) return(TRUE);
#else
	if ((b%5) == 2) return(TRUE);
#endif
	return(FALSE);
}

/*
 * routine to tell whether button "but" has the "shift" key held
 */
BOOLEAN shiftbutton(INTBIG b)
{
#ifdef MACOS
	if ((b%2) == 1) return(TRUE);
#else
	/* this "switch" statement is taken from the array "gra_butonname" */
	b = b / REALBUTS;
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
#endif
	return(FALSE);
}

/*
 * routine to tell whether button "but" is a "mouse wheel" button
 */
BOOLEAN wheelbutton(INTBIG b)
{
#ifndef MACOS
	b = b % REALBUTS;
	if (b == 3 || b == 4) return(TRUE);
#endif
	return(FALSE);
}

/*
 * routine to return the name of button "b" (from 0 to "buttoncount()").
 * The number of letters unique to the button is placed in "important".
 */
char *buttonname(INTBIG b, INTBIG *important)
{
	*important = gra_buttonname[b].unique;
	return(gra_buttonname[b].name);
}

/*
 * routine to wait for a button push and return its index (0 based) in "*but".
 * The coordinates of the cursor are placed in "*x" and "*y".  If there is no
 * button push, the value of "*but" is negative.
 */
void waitforbutton(INTBIG *x, INTBIG *y, INTBIG *but)
{
	/* This function is not used on Qt */
	Q_UNUSED( x );
	Q_UNUSED( y );
#ifdef ETRACE
	etrace(GTRACE, "{} waitforbutton: but=-1\n");
#endif
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
#ifdef ETRACE
	etrace(GTRACE, "{ modalloop: cursor=%d %s\n",
	       cursor, stateNames[gra_state]);
#endif
	/* change state */
	GRAPHSTATE savedstate = gra_state;
	gra_state = S_MODAL;
#ifdef ETRACE
	etrace(GTRACE, "  gra_state=MODAL\n");
#endif
	gra->charhandler = charhandler;
	gra->buttonhandler = buttonhandler;

	/* set cursor */
	INTBIG oldnormalcursor = us_normalcursor;
	setnormalcursor(cursor);
#if 1
	flushscreen();
#endif

	qApp->enter_loop();

	/* restore cursor */
	setnormalcursor(oldnormalcursor);
	gra_state = savedstate;
#ifdef ETRACE
	etrace(GTRACE, "  gra_state=%s\n", stateNames[gra_state]);
#endif
	gra->charhandler = 0;
	gra->buttonhandler = 0;
#ifdef ETRACE
	etrace(GTRACE, "} modalloop\n");
#endif
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
#ifdef ETRACE
	etrace(GTRACE, "{ trackcursor: waitforpush=%d purpose=%d %s\n",
	       waitforpush, purpose, stateNames[gra_state]);
#endif
	/* change state */
	GRAPHSTATE savedstate = gra_state;
	gra_state = S_TRACK;
#ifdef ETRACE
	etrace(GTRACE, "  gra_state=TRACK\n");
#endif
	gra->waitforpush = waitforpush;
	gra->whileup = whileup;
	gra->whendown = whendown;
	gra->eachdown = eachdown;
	gra->eachchar = eachchar;

	/* change the cursor to an appropriate icon */
	INTSML oldnormalcursor = us_normalcursor;
	switch (purpose)
	{
		case TRACKDRAWING:    us_normalcursor = PENCURSOR;    break;
		case TRACKDRAGGING:   us_normalcursor = HANDCURSOR;   break;
		case TRACKSELECTING:
		case TRACKHSELECTING: us_normalcursor = MENUCURSOR;   break;
	}
	setdefaultcursortype(us_normalcursor);

	flushscreen();

	BOOLEAN keepon = FALSE;
	if (!waitforpush) {
#ifdef ETRACE
        etrace(GTRACE, "  trackcursor { whendown\n");
#endif
		(*whendown)();
#ifdef ETRACE
		etrace(GTRACE, "  trackcursor } whendown\n");
		etrace(GTRACE, "  trackcursor { eachdown x=%d y=%d\n", gra_cursorx, gra_cursory);
#endif
		keepon = (*eachdown)(gra_cursorx, gra_cursory);
#ifdef ETRACE
		etrace(GTRACE, "  trackcursor } eachdown: keepon=%d el_pleasestp=%d\n",
		       keepon, el_pleasestop);
#endif
	}

	/* enter modal loop */
	if (!keepon && !el_pleasestop) qApp->enter_loop();

	/* inform the user that all is done */
#ifdef ETRACE
	etrace(GTRACE, "  trackcursor { done\n");
#endif
	(*done)();
#ifdef ETRACE
	etrace(GTRACE, "  trackcursor } done\n");
#endif
	flushscreen();

	/* restore the state of the world */
	us_normalcursor = oldnormalcursor;
	setdefaultcursortype(us_normalcursor);
	gra_state = savedstate;
#ifdef ETRACE
	etrace(GTRACE, "  gra_state=%s\n", stateNames[gra_state]);
#endif
	gra->whileup = 0;
	gra->whendown = 0;
	gra->eachdown = 0;
	gra->eachchar = 0;
#ifdef ETRACE
	etrace(GTRACE, "} trackcursor\n");
#endif
}

/*
 * routine to read the current co-ordinates of the tablet and return them in "*x" and "*y"
 */
void readtablet(INTBIG *x, INTBIG *y)
{
	*x = gra_cursorx;
	*y = gra_cursory;
#ifdef ETRACE
	etrace(GTRACE, "{}readtablet: x=%d y=%d\n", *x, *y);
#endif
}

/*
 * routine to turn off the cursor tracking if it is on
 */
void stoptablet(void)
{
	/* This function is not used on Qt */
#ifdef ETRACE
	etrace(GTRACE, "{ stoptablet\n");
#endif
	if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
#ifdef ETRACE
	etrace(GTRACE, "} stoptablet\n");
#endif
}

/******************** KEYBOARD CONTROL ********************/

/*
 * routine to get the next character from the keyboard
 */
INTSML getnxtchar(INTBIG *special)
{
	/* This function is not used on Qt */
#ifdef ETRACE
	etrace(GTRACE, "{ getnxtchar\n");
#endif
	modalloop(gra_nxtcharhandler, gra_nullbuttonhandler, WANTTTYCURSOR);
#ifdef ETRACE
	etrace(GTRACE, "} getnxtchar: char=%#o special=%#o\n",
	       gra_nxtchar, gra_nxtcharspecial);
#endif
	*special = gra_nxtcharspecial;
	return(gra_nxtchar);
}

static BOOLEAN gra_nxtcharhandler(INTSML chr, INTBIG special)
{
	gra_nxtchar = chr;
	gra_nxtcharspecial = special;
	return(TRUE);
}

static BOOLEAN gra_nullbuttonhandler(INTBIG x, INTBIG y, INTBIG but)
{
	Q_UNUSED( x );
	Q_UNUSED( y );
	Q_UNUSED( but );
	return(FALSE);
}

/* not required with X11 */
void checkforinterrupt(void)
{
#ifdef ETRACE_
	etrace(GTRACE, "{ checkforinterrupt: %s\n", stateNames[gra_state]);
#endif
#if 0
	GRAPHSTATE savedstate = gra_state;
	gra_state = S_CHECKINT;

	if (gra->hasPendingEvents())
		gra->processEvents();
	setdefaultcursortype(NULLCURSOR);

	gra_state = savedstate;
#endif
#ifdef ETRACE_
	etrace(GTRACE, "} checkforinterrupt\n");
#endif
}

/*
 * Routine to return which "bucky bits" are held down (shift, control, etc.)
 */
INTBIG getbuckybits(void)
{
	REGISTER INTBIG bits;

	bits = 0;
	if (gra_lastbuttonstate & Qt::ControlButton) bits |= CONTROLDOWN|ACCELERATORDOWN;
	if (gra_lastbuttonstate & Qt::ShiftButton) bits |= SHIFTDOWN;
	if (gra_lastbuttonstate & Qt::AltButton) bits |= OPTALTMETDOWN;
	return(bits);
}

/*
 * routine to tell whether data is waiting at the terminal.  Returns true
 * if data is ready.
 */
BOOLEAN ttydataready(void)
{
	/* This function is not used on Qt */
#ifdef ETRACE
	etrace(GTRACE, "{}ttydataready: result=0\n");
#endif
	return(FALSE);
}

/****************************** FILES ******************************/

#ifdef MACOS
/*
 * Routine to set the type and creator of file "name" to "type" and "creator"
 */
void mac_settypecreator(char *name, INTBIG type, INTBIG creator)
{
	FSRef ref, parentRef;
	OSErr result;
	FSCatalogInfo catalogInfo;

	/* convert the full path to a FSRef */
	FSPathMakeRef((unsigned char *)name, &ref, NULL);
	
	/* get info about the FSRef */
	result = FSGetCatalogInfo(&ref, kFSCatInfoNodeFlags + kFSCatInfoFinderInfo, &catalogInfo , NULL, NULL, &parentRef);
	if (result != noErr) return;

	/* set the type and creator */
	((FileInfo *)&catalogInfo.finderInfo)->fileType = type;
	((FileInfo *)&catalogInfo.finderInfo)->fileCreator = creator;
	FSSetCatalogInfo(&ref, kFSCatInfoFinderInfo, &catalogInfo);
}
#endif

char *EApplication::localize (QString qstr)
{
    static QCString str[2];
	static int i = 0;

    str[i] = qstr.local8Bit();
    const char *s = str[i];
	i = (i + 1) % (sizeof(str)/sizeof(str[0]));
    return((char*)s);
}

char *EApplication::localizeFilePath (QString filePath, bool addSeparator )
{
    static QCString filePathCS;

    if ( addSeparator && !filePath.isEmpty() && filePath.right(1) != QString::fromLatin1("/") )
	filePath += '/';
    filePath = QDir::convertSeparators( filePath );
    filePathCS = QFile::encodeName( filePath );
    const char *s = filePathCS;
    return((char*)s);
}

/*
 * Routine to prompt for multiple files of type "filetype", giving the
 * message "msg".  Returns a string that contains all of the file names,
 * separated by the NONFILECH (a character that cannot be in a file name).
 */
char *multifileselectin(char *msg, INTBIG filetype)
{
	return(fileselect(msg, filetype, ""));
}

extern "C" COMCOMP us_yesnop;

/*
 * Routine to display a standard file prompt dialog and return the selected file.
 * The prompt message is in "msg" and the kind of file is in "filetype".  The default
 * output file name is in "defofile" (only used if "filetype" is for output files).
 */
char *fileselect(char *msg, INTBIG filetype, char *defofile)
{
	INTBIG mactype;
	BOOLEAN binary;
	char *extension, *winfilter, *shortname, *longname, temp[300];

#ifdef ETRACE
	etrace(GTRACE, "{ fileselect: msg=%s filetype=%d defofile\n", msg, filetype, defofile);
#endif

	QString fileName;
	QWidget *parent;

#ifdef MACOSX
	parent = 0;
#else
	parent = el_curwindowframe != NOWINDOWFRAME ?
	  el_curwindowframe->draw->parentWidget() :
	  gra->mainWidget();
#endif
	describefiletype(filetype, &extension, &winfilter, &mactype, &binary, &shortname, &longname);
	sprintf(temp, "%s (%s);;All Files (*)", msg, winfilter);
	QString filter = QString::fromLocal8Bit( temp );
	if ((filetype&FILETYPEWRITE) == 0)
	{
		/* input file selection: display the file input dialog box */
		sprintf(temp, _("%s Selection"), msg);
		QString title = QString::fromLocal8Bit( temp );

		fileName = QFileDialog::getOpenFileName(
			QString::null, filter, parent, "FileSelection", title);
	} else
	{
		/* output file selection: display the file input dialog box */
		sprintf(temp, _("%s Selection"), msg);
		QString startWith = QFile::decodeName( fullfilename(defofile) );
#ifdef MACOSX
		QFileDialog *fd = new QFileDialog(parent, temp, TRUE);
		fd->setMode(QFileDialog::AnyFile);
		fd->setFilter(filter);
		fd->setSelection(startWith);
		if (fd->exec() != QDialog::Accepted) fileName = QString::null; else
			fileName = fd->selectedFile();
		delete fd;
#else
		QString title = QString::fromLocal8Bit( temp );
		fileName = QFileDialog::getSaveFileName(
			startWith, filter, parent, "File Creation", title);
#endif

		/* give warning if creating file that already exists */
		if (!fileName.isEmpty() && QFile::exists( fileName ))
		{
			QFileInfo fileInfo( fileName );
			char fullmsg[300], *pars[5];
			const char *s = EApplication::localize( fileName );
			if (fileInfo.isDir())
			{
				DiaMessageInDialog(_("'%s' is a directory: cannot write it"), s);
				fileName = QString::null;
			} else if (!fileInfo.isWritable())
			{
				DiaMessageInDialog(_("'%s' is read-only: cannot overwrite it"), s);
				fileName = QString::null;
			} else
			{
				sprintf(fullmsg, _("File '%s' exists.  Overwrite? "), s);
				INTSML count = ttygetparam(fullmsg, &us_yesnop, 2, pars);
				if (count > 0 && namesamen(pars[0], "no", strlen(pars[0])) == 0)
					fileName = QString::null;
			}
		}
	}
	if (!fileName.isEmpty()) {
		QDir::setCurrent( QFileInfo(fileName).dirPath() );
	}
	strcpy(gra_curpath, EApplication::localizeFilePath( fileName, FALSE) );

#ifdef ETRACE
	etrace(GTRACE, "} fileselect: %s\n", gra_curpath);
#endif
	if (gra_curpath[0] == 0) return((char *)NULL); else
	{
		return(gra_curpath);
	}
}

/*
 * Routine to search for all of the files/directories in directory "directory" and
 * return them in the array of strings "filelist".  Returns the number of files found.
 */
INTBIG filesindirectory(char *directory, char ***filelist)
{
	INTBIG len;
	QDir dir( QString::fromLocal8Bit (directory) );

#ifdef ETRACE
	etrace(GTRACE, "{ fileindirectory: %s\n", directory);
#endif
	if (gra_fileliststringarray == 0)
	{
		gra_fileliststringarray = newstringarray(db_cluster);
		if (gra_fileliststringarray == 0) return 0;
	}
	clearstrings(gra_fileliststringarray);

	for(uint i=0; i < dir.count(); i++)
	{
		if (dir[i] == "." || dir[i] == "..") continue;
		addtostringarray( gra_fileliststringarray,  EApplication::localizeFilePath ( dir[i], FALSE ) );
	}
	*filelist = getstringarray(gra_fileliststringarray, &len);
#ifdef ETRACE
	for (int i = 0; i < len; i++)
	  etrace(GTRACE, "\t%s\n", (*filelist)[i]);
	etrace(GTRACE, "} filesindirectory\n");
#endif
	return(len);
}

/* routine to convert a path name with "~" to a real path */
char *truepath(char *line)
{
#if defined(ONUNIX) || defined(MACOSX)
	static char outline[100], home[50];
	REGISTER char save, *ch;
	static INTSML gothome = 0;
	struct passwd *tmp;

	if (line[0] != '~') return(line);

	/* if it is the form "~username", figure it out */
	if (line[1] != '/')
	{
		for(ch = &line[1]; *ch != 0 && *ch != '/'; ch++);
		save = *ch;   *ch = 0;
		tmp = getpwnam(&line[1]);
		if (tmp == 0) return(line);
		*ch = save;
		(void)strcpy(outline, tmp->pw_dir);
		(void)strcat(outline, ch);
		return(outline);
	}

	/* get the user's home directory once only */
	if (gothome == 0)
	{
		/* get name of user */
		tmp = getpwuid(getuid());
		if (tmp == 0) return(line);
		(void)strcpy(home, tmp->pw_dir);
		gothome++;
	}

	/* substitute home directory for the "~" */
	(void)strcpy(outline, home);
	(void)strcat(outline, &line[1]);
	return(outline);
#else
	/* only have tilde parsing on UNIX and Mac OS 10 */
	return(line);
#endif
}

/*
 * Routine to return the full path to file "file".
 */
char *fullfilename(char *file)
{
#if 1
	QFileInfo fileInfo( QString::fromLocal8Bit( file ) );
	return EApplication::localizeFilePath( fileInfo.absFilePath(), FALSE );
#else
	static char fullfile[MAXPATHLEN];

	if (file[0] == '/') return(file);
	strcpy(fullfile, currentdirectory());
	strcat(fullfile, file);
	return(fullfile);
#endif
}

/*
 * routine to rename file "file" to "newfile"
 * returns nonzero on error
 */
INTBIG erename(char *file, char *newfile)
{
#ifdef WIN32
	return(rename(file, newfile));
#else
	if (link(file, newfile) < 0) return(1);
	if (unlink(file) < 0) return(1);
	return(0);
#endif
}

/*
 * routine to delete file "file"
 */
INTBIG eunlink(char *file)
{
	return(unlink(file));
}

/*
 * Routine to return information about the file or directory "name":
 *  0: does not exist
 *  1: is a file
 *  2: is a directory
 *  3: is a locked file (read-only)
 */
INTBIG fileexistence(char *name)
{
	QFileInfo fileInfo( QString::fromLocal8Bit( name ));
	if (!fileInfo.exists()) return(0);
	if (fileInfo.isDir()) return(2);
	if (!fileInfo.isWritable()) return(3);
	return(1);
}

/*
 * Routine to create a directory.
 * Returns true on error.
 */
BOOLEAN createdirectory(char *dirname)
{
	char cmd[256];

	sprintf(cmd, "mkdir %s", dirname);
	if (system(cmd) == 0) return(FALSE);
	return(TRUE);
}

/*
 * Routine to return the current directory name
 */
char *currentdirectory(void)
{
	char *s = EApplication::localizeFilePath(QDir::currentDirPath(), TRUE);
#ifdef ETRACE
	etrace(GTRACE, "{}currentdirectory: %s\n", s);
#endif
	return(s);
}

/*
 * Routine to return the home directory (returns 0 if it doesn't exist)
 */
char *hashomedir(void)
{
	char *s = EApplication::localizeFilePath(QDir::homeDirPath(), TRUE);
	return(s);
}

/*
 * Routine to return the path to the "options" library.
 */
char *optionsfilepath(void)
{
	return("~/.electricoptions.elib");
}

/*
 * routine to return the date of the last modification to file "filename"
 */
time_t filedate(char *filename)
{
	struct stat buf;

	stat(filename, &buf);
	buf.st_mtime -= machinetimeoffset();
	return(buf.st_mtime);
}

/*
 * Routine to lock a resource called "lockfilename" by creating such a file
 * if it doesn't exist.  Returns true if successful, false if unable to
 * lock the file.
 */
BOOLEAN lockfile(char *lockfilename)
{
	INTBIG fd;

	fd = creat(lockfilename, 0);
	if (fd == -1 && errno == EACCES) return(FALSE);
	if (fd == -1 || close(fd) == -1) return(FALSE);
	return(TRUE);
}

/*
 * Routine to unlock a resource called "lockfilename" by deleting such a file.
 */
void unlockfile(char *lockfilename)
{
	if (unlink(lockfilename) == -1)
		ttyputerr(_("Error unlocking %s"), lockfilename);
}

/*
 * Routine to show file "filename" in a browser window.
 * Returns true if the operation cannot be done.
 */
BOOLEAN browsefile(char *filename)
{
	char line[300];

	sprintf(line, "netscape %s&", filename);
	if (system(line) == 0) return(FALSE);
	return(TRUE);
}

/************************ EProcess class ***************************/

EProcessPrivate::EProcessPrivate()
	: QProcess()
{
}

void EProcessPrivate::idle()
{
	QTimer timer( this );
	timer.start( EPROCESSWAIT, TRUE );
	qApp->processOneEvent();
}

EProcess::EProcess()
{
	d = new EProcessPrivate();
	setCommunication( TRUE, TRUE, TRUE );
}

EProcess::~EProcess()
{
	clearArguments();
	delete d;
}

void EProcess::clearArguments()
{
	d->clearArguments();
}

void EProcess::addArgument( CHAR *arg )
{
	d->addArgument( arg );
}

void EProcess::setCommunication( BOOLEAN cStdin, BOOLEAN cStdout, BOOLEAN cStderr )
{
	int comms = 0;
	
	if ( cStdin) comms |= QProcess::Stdin;
	if ( cStdout ) comms |= QProcess::Stdout;
	if ( cStderr ) comms |= QProcess::Stderr;
	if ( cStdout || cStderr ) comms |= QProcess::DupStderr;
	d->setCommunication( comms );
}

BOOLEAN EProcess::start( CHAR *infile )
{
	d->buf.resize( 0 );

	if (!infile) return d->start(); else
	{
		QFile file( infile );
		if ( !file.open( IO_ReadOnly ) )
		{
			ttyputmsg(_("File %s not opened. Process not launched"), infile);
			return FALSE;
		}
		QByteArray inp = file.readAll();
		file.close();
		return d->launch( inp );
	}
}

void EProcess::wait()
{
	while (d->isRunning()) d->idle();
}

void EProcess::kill()
{
#if 1
	d->kill();
	wait();
	char *arg0 = EApplication::localize( d->arguments()[0] );
	if (d->normalExit())
		ttyputmsg(_("%s execution terminated"), arg0 );
	else
		ttyputmsg(_("%s execution reports error %d"), arg0, d->exitStatus());
#else
	if ( d->_cStdout || d->_cStderr) eclose(d->opipe[0]);
	if (ekill(d->process) == 0)
	{
		ewait(d->process);
		if (errno > 0)
			ttyputmsg(_("%s execution reports error %d"), d->arguments[0], errno);
		else ttyputmsg(_("%s execution terminated"), d->arguments[0]);
	}
#endif
}

INTSML EProcess::getChar()
{
	if (d->bufp >= d->buf.size())
	{
		d->bufp = 0;
		for(;;)
		{
			d->buf = (d->communication() & QProcess::Stdout) != 0 ? d->readStdout() : d->readStderr();
			if ( d->buf.size() > 0 ) break;
			if ( !d->isRunning() ) return EOF;
			d->idle();
		}
	}
	return d->buf[d->bufp++];
}

void EProcess::putChar( UCHAR1 ch )
{
	QString qstr = QChar( ch );
	d->writeToStdin( qstr );
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
	return(ticktime());
}

/* returns the current time in 60ths of a second */
UINTBIG ticktime(void)
{
	return (-QTime::currentTime().msecsTo( QTime() ) ) * 6 / 100;
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
#ifdef WIN32
	Sleep(ticks * 100 / 6);
#else
	sleep(ticks);
#endif
}

/*
 * Routine to start counting time.
 */
void starttimer(void)
{
	gra_timebase.start();
}

/*
 * Routine to stop counting time and return the number of elapsed seconds
 * since the last call to "starttimer()".
 */
float endtimer(void)
{
	return ((float)gra_timebase.elapsed()) / 1000.0;
}

/*************************** EVENT ROUTINES ***************************/

void EApplication::toolTimeSlice()
{
#ifdef ETRACE
    etrace(GTRACE, "{ toolTimeSlice %s %d\n", stateNames[gra_state], gra_stop_dowork);
#endif
    if (gra_state == S_USER) {
        Q_ASSERT( gra_stop_dowork == 0 );
        gra_stop_dowork++;

	/* announce end of broadcast of changes */
	db_endbatch();

	gra_state = S_TOOL;
#if 1
	do {
		tooltimeslice();
	} while((us_state&LANGLOOP) != 0);
#else
	tooltimeslice();
#endif

	gra_state = S_USER;
	db_setcurrenttool(us_tool);
	flushscreen();

	/* first see if there is any pending input */
	el_pleasestop = 0;
	if (us_cursorstate != IBEAMCURSOR) setdefaultcursortype(us_normalcursor);
	gra_handlequeuedframedeletion();
        gra_stop_dowork--;
    }
#ifdef ETRACE
    etrace(GTRACE, "} toolTimeSlice\n");
#endif
}

void gra_handlequeuedframedeletion(void)
{
	char *par[MAXPARS];
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
		par[0] = "off";
		us_menu(1, par);
		return;
	}

	/* make sure there is another window */
	windows = 0;
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
		if (!wf->floating) windows++;
#ifndef MACOS
	if (windows < 2)
	{
		/* no other windows: kill the program */
		if (us_preventloss(NOLIBRARY, 0, TRUE)) return;
		bringdown();
	} else
#endif
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
		(void)setval((INTBIG)el_curlib, VLIBRARY, "curnodeproto", (INTBIG)np, VNODEPROTO);

		/* restore highlighting */
		us_pophighlight(FALSE);

		/* now no widget being deleted, and kill the window properly */
		gra_windowbeingdeleted = 0;
		killwindowframe(delwf);
	}
}

void GraphicsDraw::keepCursorPos( QPoint pos )
{
    gra_cursorx = pos.x();
    gra_cursory = wf->revy - pos.y();
    if (gra_cursory < 0) gra_cursory = 0;
	us_state &= ~GOTXY;
}

void GraphicsDraw::buttonPressed( QPoint pos, INTSML but )
{
#ifdef DOUBLESELECT
	if (activationChanged)
	{
		activationChanged = FALSE;
		return;
	}
#endif
    keepCursorPos( pos );
    us_state |= DIDINPUT;
    if (gra_state == S_USER)
	{
#ifdef ETRACE
        etrace(GTRACE, "{  on_tablet gra_cursorx=%d gra_cursory=%d but=%d\n",
			gra_cursorx, gra_cursory, but);
#endif
        us_ontablet(gra_cursorx, gra_cursory, but);
#ifdef ETRACE
        etrace(GTRACE, "}  on_tablet\n");
#endif
    } else if (gra_state == S_MODAL && gra->buttonhandler != 0)
	{
#ifdef ETRACE
        etrace(GTRACE, "  modalloop { buttonhandler: x=%d y=%d but=%d\n", gra_cursorx, gra_cursory, but);
#endif
		BOOLEAN keepon = gra->buttonhandler(gra_cursorx, gra_cursory, but);
#ifdef ETRACE
		etrace(GTRACE, "  modalloop } buttonhandler: keepon=%d el_pleasestop=%d\n", keepon, el_pleasestop);
#endif
		if (keepon || el_pleasestop) gra->exit_loop();
    }
    gra->toolTimeSlice();
}

void GraphicsDraw::mouseDoubleClickEvent( QMouseEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::mouseDoubleClickEvent %s %d %d,%d %#o\n",
	   stateNames[gra_state], wf->windindex, e->x(), e->y(), e->state());
#endif
    gra_lastbuttonstate = e->stateAfter();
    if (gra_state == S_USER || gra_state == S_MODAL)
	{
		INTSML but;
#ifdef MACOS
		but = 16;
#else
		switch (e->button())
		{
			case LeftButton: but = 0; break;
			case RightButton: but = 2; break;
			case MidButton: but = 1; break;
            default: but = 0;
		}
		but += REALBUTS*8;
#endif
		buttonPressed( e->pos(), but );
    }
    else QWidget::mouseDoubleClickEvent( e );

#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::mouseDoubleClickEvent\n");
#endif
}

extern "C" INTBIG sim_window_wavexbar;

void GraphicsDraw::mouseMoveEvent( QMouseEvent *e )
{
	INTSML setcursor, inmenu;
	char *str;
	REGISTER INTBIG x, y, xfx, xfy;
	REGISTER VARIABLE *var;
	INTBIG lx, hx, ly, hy;
	REGISTER WINDOWPART *win;
	COMMANDBINDING commandbinding;
	static INTSML overrodestatus = 0;

#ifdef ETRACE_
	etrace(GTRACE, "{ GraphicsDraw::mouseMoveEvent %s %d %d,%d %#o\n",
		stateNames[gra_state], wf->windindex, e->x(), e->y(), e->state());
#endif
	gra_lastbuttonstate = e->stateAfter();
	QWidget::mouseMoveEvent( e );
#if 1
	gra_cursorx = e->pos().x();
	gra_cursory = wf->revy - e->pos().y();
	if (gra_cursory < 0) gra_cursory = 0;
	us_state &= ~GOTXY;
#else
	keepCursorPos( e->pos() );
#endif
	if ((e->state() & (LeftButton | RightButton | MidButton)) == 0 || gra->waitforpush)
	{
		/* motion while up considers any window */

		/* report the menu if over one */
		inmenu = 0;
		if (wf->floating)
		{
			x = (gra_cursorx-us_menulx) / us_menuxsz;
			y = (gra_cursory-us_menuly) / us_menuysz;
			if (x >= 0 && y >= 0 && x < us_menux && y < us_menuy)
			{
				var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
				if (var != NOVARIABLE)
				{
					if (us_menupos <= 1) str = ((char **)var->addr)[y * us_menux + x]; else
						str = ((char **)var->addr)[x * us_menuy + y];
					us_parsebinding(str, &commandbinding);
					if (*commandbinding.command != 0)
					{
						if (commandbinding.nodeglyph != NONODEPROTO)
						{
							ttysetstatusfield(NOWINDOWFRAME, us_statusarc,
								describearcproto(us_curarcproto), TRUE);
							ttysetstatusfield(NOWINDOWFRAME, us_statusnode,
								us_describemenunode(&commandbinding), TRUE);
							inmenu = 1;
							overrodestatus = 1;
						}
						if (commandbinding.arcglyph != NOARCPROTO)
						{
							ttysetstatusfield(NOWINDOWFRAME, us_statusarc,
								describearcproto(commandbinding.arcglyph), TRUE);
							if (us_curnodeproto == NONODEPROTO) str = ""; else
								str = describenodeproto(us_curnodeproto);
							ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
							inmenu = 1;
							overrodestatus = 1;
						}
					}
					us_freebindingparse(&commandbinding);
				}
			}
		}
		if (inmenu == 0 && overrodestatus != 0)
		{
			ttysetstatusfield(NOWINDOWFRAME, us_statusarc,
				describearcproto(us_curarcproto), TRUE);
			if (us_curnodeproto == NONODEPROTO) str = ""; else
				str = describenodeproto(us_curnodeproto);
			ttysetstatusfield(NOWINDOWFRAME, us_statusnode, str, TRUE);
			overrodestatus = 0;
		}

		setcursor = 0;
		for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
		{
			/* see if the cursor is over a window partition separator */
			if (win->frame != wf) continue;
			us_gettruewindowbounds(win, &lx, &hx, &ly, &hy);
			if (gra_cursorx >= lx-1 && gra_cursorx <= lx+1 && gra_cursory > ly+1 &&
				gra_cursory < hy-1 && us_hasotherwindowpart(lx-10, gra_cursory, win))
			{
				setdefaultcursortype(LRCURSOR);
				setcursor = 1;
				break;
			} else if (gra_cursorx >= hx-1 && gra_cursorx <= hx+1 && gra_cursory > ly+1 &&
				gra_cursory < hy-1 && us_hasotherwindowpart(hx+10, gra_cursory, win))
			{
				setdefaultcursortype(LRCURSOR);
				setcursor = 1;
				break;
			} else if (gra_cursory >= ly-1 && gra_cursory <= ly+1 && gra_cursorx > lx+1 &&
				gra_cursorx < hx-1 && us_hasotherwindowpart(gra_cursorx, ly-10, win))
			{
				setdefaultcursortype(UDCURSOR);
				setcursor = 1;
				break;
			} else if (gra_cursory >= hy-1 && gra_cursory <= hy+1 && gra_cursorx > lx+1 &&
				gra_cursorx < hx-1 && us_hasotherwindowpart(gra_cursorx, hy+10, win))
			{
				setdefaultcursortype(UDCURSOR);
				setcursor = 1;
				break;
			}

			if (gra_cursorx < win->uselx || gra_cursorx > win->usehx ||
				gra_cursory < win->usely || gra_cursory > win->usehy) continue;
			if ((win->state&WINDOWTYPE) == WAVEFORMWINDOW)
			{
				xfx = muldiv(gra_cursorx - win->uselx, win->screenhx - win->screenlx,
					win->usehx - win->uselx) + win->screenlx;
				xfy = muldiv(gra_cursory - win->usely, win->screenhy - win->screenly,
					win->usehy - win->usely) + win->screenly;
				if (abs(xfx - sim_window_wavexbar) < 2 && xfy >= 560)
				{
					setdefaultcursortype(LRCURSOR);
					setcursor = 1;
					break;
				}
			}
			if ((win->state&WINDOWTYPE) == POPTEXTWINDOW ||
				(win->state&WINDOWTYPE) == TEXTWINDOW)
			{
				EDITOR *ed = win->editor;
				if ((ed->state&EDITORTYPE) == PACEDITOR)
				{
					if (gra_cursorx <= win->usehx - SBARWIDTH &&
						gra_cursory >= win->usely + SBARWIDTH &&
						gra_cursory < ed->revy)
					{
						setdefaultcursortype(IBEAMCURSOR);
						setcursor = 1;
						break;
					}
				}
			}
		}
		if (setcursor == 0) setdefaultcursortype(us_normalcursor);
		if (gra_state == S_TRACK) {
#ifdef ETRACE
		  etrace(GTRACE, "  trackcursor { whileup x=%d y=%d\n",
			 gra_cursorx, gra_cursory);
#endif
		  BOOLEAN keepon = (*gra->whileup)(gra_cursorx, gra_cursory);
#ifdef ETRACE
		  etrace(GTRACE, "  trackcursor } whileup: keepon=%d el_pleasestop=%d\n",
			 keepon, el_pleasestop);
#endif
		  if (keepon || el_pleasestop) gra->exit_loop();
		}
	} else
	{
		/* motion while button down: use last window */
	        if (gra_state == S_TRACK) {
#ifdef ETRACE
		  etrace(GTRACE, "  trackcursor { eachdown x=%d y=%d\n",
			 gra_cursorx, gra_cursory);
#endif
		  BOOLEAN keepon = (*gra->eachdown)(gra_cursorx, gra_cursory);
#ifdef ETRACE
		  etrace(GTRACE, "  trackcursor } eachdown: keepon=%d el_pleasestop=%d\n",
			 keepon, el_pleasestop);
#endif
		  flushscreen();
		  if (keepon || el_pleasestop) gra->exit_loop();
		}
	}

#ifdef ETRACE_
	//	etrace(GTRACE, "} GraphicsDraw::mouseMoveEvent\n");
#endif
}

void GraphicsDraw::mousePressEvent( QMouseEvent *e )
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::mousePressEvent %s %d %d,%d %#o\n",
	   stateNames[gra_state], wf->windindex, e->x(), e->y(), e->state());
    etrace(GTRACE, "{ GraphicsDraw::mousePressEvent %s %d %d,%d state: %x\n",
	   stateNames[gra_state], wf->windindex, e->x(), e->y(), e->state());
#endif
    gra_lastbuttonstate = e->stateAfter();
    if (gra_state == S_USER || gra_state == S_MODAL)
	{
        INTSML but;
		switch (e->button())
		{
			case LeftButton: but = 0; break;
			case RightButton: but = 2; break;
			case MidButton: but = 1; break;
            default: but = 0;
		}
#ifdef MACOS
		but = 0;
		if (e->state() & ShiftButton) but++;      /* Shift key */
		if (e->state() & ControlButton) but += 2; /* Command key */
		if (e->state() & AltButton) but += 4;     /* Option key */
		if (e->state() & MetaButton) but += 8;    /* Control key */
#else
		INTSML modifier = 0;
		if (e->state() & ShiftButton) modifier |= 1;
		if (e->state() & ControlButton) modifier |= 2;
		if (e->state() & AltButton) modifier |= 4;
		/* In Linux, Meta key does not come through in state(), so we
		   fake it using KeyPressEvent<->KeyReleaseEvent and setting
		   metaheld */
		/* Note that under qt, Meta and alt are synonymous */
		if (metaheld) modifier |= 4;  
		switch (modifier) 
		{
			case 1: but += REALBUTS*1; break;
			case 2: but += REALBUTS*2; break;
			case 4: but += REALBUTS*3; break;
			case 3: but += REALBUTS*4; break;
			case 5: but += REALBUTS*5; break;
			case 6: but += REALBUTS*6; break;
			case 7: but += REALBUTS*7; break;
		}
#endif
		buttonPressed( e->pos(), but );
    } else if (gra_state == S_TRACK)
	{
        if ((e->state() & (LeftButton | RightButton | MidButton)) == 0)
		{
			keepCursorPos( e->pos() );
			if (gra->waitforpush)
			{
#ifdef ETRACE
				etrace(GTRACE, "  trackcursor { whendown\n");
#endif
				(*gra->whendown)();
#ifdef ETRACE
				etrace(GTRACE, "  trackcursor } whendown\n");
#endif
				gra->waitforpush = 0;
			}
#ifdef ETRACE
			etrace(GTRACE, "  trackcursor { eachdown x=%d y=%d\n", gra_cursorx, gra_cursory);
#endif
			BOOLEAN keepon = (*gra->eachdown)(gra_cursorx, gra_cursory);
#ifdef ETRACE
			etrace(GTRACE, "  trackcursor } eachdown: keepon=%d el_pleasestop=%d\n", keepon, el_pleasestop);
#endif
			if (keepon || el_pleasestop) gra->exit_loop();
		}
		us_state |= DIDINPUT;
    }

#ifdef ETRACE
	etrace(GTRACE, "} GraphicsDraw::mousePressEvent\n");
#endif
}

void GraphicsDraw::mouseReleaseEvent( QMouseEvent *e )
{
#ifdef ETRACE
	etrace(GTRACE, "{ GraphicsDraw::mouseReleaseEvent %s %d %d,%d %#o\n",
	       stateNames[gra_state], wf->windindex, e->x(), e->y(), e->state());
#endif
	gra_lastbuttonstate = e->stateAfter();
	keepCursorPos( e->pos() );

	if (gra_state == S_TRACK && !gra->waitforpush) {
	  if ((e->stateAfter() & (LeftButton | RightButton | MidButton)) == 0)
	    gra->exit_loop();
	}
	us_state |= DIDINPUT;
#ifdef ETRACE
	etrace(GTRACE, "} GraphicsDraw::mouseReleaseEvent\n");
#endif
}

void GraphicsDraw::paintEvent( QPaintEvent * e)
{
    QRect r = e->rect();
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::paintEvent %d %s %d,%d %d,%d\n",
	   wf->windindex, stateNames[gra_state],
	   r.left(), r.top(), r.right(), r.bottom());
#endif
    /* get bounds of display update */
    int lx = r.left();
    int hx = r.right() + 1;
    int ly = r.top();
    int hy = r.bottom() + 1;
    if (lx < 0) lx = 0;
    if (hx > wf->swid) hx = wf->swid;
    if (ly < 0) ly = 0;
    if (hy > wf->shei) hy = wf->shei;

    /* copy from offscreen buffer to Ximage while doing color mapping */
    QPainter p( this );
    p.drawImage( r.topLeft(), image, r );
#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::paintEvent\n");
#endif
}


void GraphicsDraw::resizeEvent( QResizeEvent *e )
{
    static INTSML inresize = 0;

#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::resizeEvent %s OldSize=%d,%d NewSize=%d,%d\n",
	   stateNames[gra_state],
	   e->oldSize().width(),
	   e->oldSize().height(),
	   e->size().width(),
	   e->size().height());
#endif
    if (width() != wf->swid || height() != wf->shei && inresize == 0) {
		inresize = 1;

#ifdef ETRACE
		etrace(GTRACE, "{ inresize: wf=%d\n", wf->windindex);
#endif
		recalcSize();

		if (wf->floating)
		{
			us_startbatch(NOTOOL, FALSE);
			us_drawmenu(1, wf);
			us_endbatch();
		} else
		{
			/* mark start of redisplay */
			us_startbatch(NOTOOL, FALSE);

			/* save and clear highlighting */
			us_pushhighlight();
			us_clearhighlightcount();

			/* rewrite status area */
#if 1
			us_drawmenu(-1, wf);
#else
			us_drawmenu(1, wf);
#endif

			/* restore highlighting */
			us_pophighlight(FALSE);

			/* finish drawing */
			us_endbatch();

			us_redostatus(wf);

			/* describe this change */
			setactivity(_("Window Resize"));
		}
		inresize = 0;
#ifdef ETRACE
		etrace(GTRACE, "} inresize\n");
#endif
    }
    QWidget::resizeEvent( e );
#ifdef ETRACE
    etrace(GTRACE, "} GraphicsDraw::resizeEvent\n");
#endif
}

void GraphicsDraw::wheelEvent( QWheelEvent * e)
{
#ifdef ETRACE
    etrace(GTRACE, "{ GraphicsDraw::wheelEvent %s %d %d,%d %#o\n",
		stateNames[gra_state], wf->windindex, e->x(), e->y(), e->state());
#endif
    if (gra_state == S_USER || gra_state == S_MODAL)
	{
		INTSML but = e->delta() > 0 ? 3 : 4;
		INTSML modifier = 0;
		if (e->state() & ShiftButton) modifier |= 1;
		if (e->state() & ControlButton) modifier |= 2;
		if (e->state() & AltButton) modifier |= 4;
		switch (modifier)
		{
			case 1: but += REALBUTS*1; break;
			case 2: but += REALBUTS*2; break;
			case 4: but += REALBUTS*3; break;
			case 3: but += REALBUTS*4; break;
			case 5: but += REALBUTS*5; break;
			case 6: but += REALBUTS*6; break;
			case 7: but += REALBUTS*7; break;
		}
		buttonPressed( e->pos(), but );
    } else QWidget::wheelEvent( e );

#ifdef ETRACE
	etrace(GTRACE, "} GraphicsDraw::wheelEvent\n");
#endif
}

/*
 * routine to load the global "el_curwindowframe" and "el_topwindowpart".
 * Returns TRUE if the frame was just entered.
 */
static BOOLEAN gra_setcurrentwindowframe(WINDOWFRAME *wf)
{
	WINDOWPART *w;
	REGISTER BOOLEAN changed;

	changed = FALSE;
	if (wf != el_curwindowframe) 
	{
		if ((el_curwindowframe == NOWINDOWFRAME || !el_curwindowframe->floating) &&
			(wf == NOWINDOWFRAME || !wf->floating)) changed = TRUE;
	}
	el_curwindowframe = wf;
	if (wf == NOWINDOWFRAME) return(changed);

	/* ignore floating windows */
	if (wf->floating) return(changed);

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
					(void)setval((INTBIG)el_curlib, VLIBRARY, "curnodeproto",
						(INTBIG)w->curnodeproto, VNODEPROTO);
					break;
				}
			}
		}
	}
	return(changed);
}

int EApplication::translatekey(QKeyEvent *e, INTBIG *special)
{
	int state;
	char *par[1];

#ifdef ETRACE
	etrace(GTRACE, "  translatekey: ascii=%#o key=%#o state=%x accepted=%d\n",
	       e->ascii(), e->key(), e->state(), e->isAccepted());
	etrace(GTRACE, "  translatekey: ascii=%#o key=%x state=%x accepted=%d\n",
	       e->ascii(), e->key(), e->state(), e->isAccepted());
#endif
	gra_lastbuttonstate = e->stateAfter();
	*special = 0;
#if 1
	if (e->ascii() >= ' ' && e->ascii() < 0177 && (e->state() & ControlButton) == 0) return(e->ascii());
#endif
	if (e->key() > 0 && e->key() < 0200) state = e->key(); else
	{
		switch (e->key())
		{
			case Key_BackSpace:state = BACKSPACEKEY;   break;
			case Key_Tab:      state = 011;            break;
			//case XK_Linefeed:  state = 012;            break;
			case Key_Return:   state = 015;            break;
			case Key_Escape:   state = ESCKEY;         break;
			case Key_Delete:   state = 0177;           break;
			case Key_Enter:    state = 015;            break;

			case Key_End:
				/* shift to end of messages window */
#if 0
				tp = XmTextGetLastPosition(gra_messageswidget);
				XmTextSetSelection(gra_messageswidget, tp, tp, 0);
				XmTextSetInsertionPosition(gra_messageswidget, tp);
				XmTextShowPosition(gra_messageswidget, tp);
#endif
				e->accept();
#ifdef ETRACE
				etrace(GTRACE, "  EApplication::translatekey } special\n");
#endif
				return(0);
			case Key_F14:
				us_undo(0, par);
				e->accept();
#ifdef ETRACE
				etrace(GTRACE, "  EApplication::translatekey } special\n");
#endif
				return(0);
			case Key_F16:
				par[0] = "copy";
				us_text(1, par);
				e->accept();
#ifdef ETRACE
				etrace(GTRACE, "  EApplication::translatekey } special\n");
#endif
				return(0);
			case Key_F18:
				par[0] = "paste";
				us_text(1, par);
				e->accept();
#ifdef ETRACE
				etrace(GTRACE, "  EApplication::translatekey } special\n");
#endif
				return(0);
			case Key_F20:
				par[0] = "cut";
				us_text(1, par);
				e->accept();
#ifdef ETRACE
				etrace(GTRACE, "  EApplication::translatekey } special\n");
#endif
				return(0);

			case Key_Left:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWL<<SPECIALKEYSH);
				if ((e->state() & ShiftButton) != 0) *special |= SHIFTDOWN;
				break;
			case Key_Right:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWR<<SPECIALKEYSH);
				if ((e->state() & ShiftButton) != 0) *special |= SHIFTDOWN;
				break;
			case Key_Up:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWU<<SPECIALKEYSH);
				if ((e->state() & ShiftButton) != 0) *special |= SHIFTDOWN;
				break;
			case Key_Down:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYARROWD<<SPECIALKEYSH);
				if ((e->state() & ShiftButton) != 0) *special |= SHIFTDOWN;
				break;

			case Key_F1:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF1<<SPECIALKEYSH);
				break;
			case Key_F2:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF2<<SPECIALKEYSH);
				break;
			case Key_F3:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF3<<SPECIALKEYSH);
				break;
			case Key_F4:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF4<<SPECIALKEYSH);
				break;
			case Key_F5:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF5<<SPECIALKEYSH);
				break;
			case Key_F6:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF6<<SPECIALKEYSH);
				break;
			case Key_F7:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF7<<SPECIALKEYSH);
				break;
			case Key_F8:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF8<<SPECIALKEYSH);
				break;
			case Key_F9:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF9<<SPECIALKEYSH);
				break;
			case Key_F10:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF10<<SPECIALKEYSH);
				break;
			case Key_F11:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF11<<SPECIALKEYSH);
				break;
			case Key_F12:
				state = 0;
				*special = SPECIALKEYDOWN|(SPECIALKEYF12<<SPECIALKEYSH);
				break;

			default:           state = 0;     break;
		}
	}
	if (state == 0 && *special == 0) return(0);

	if ((e->state() & ControlButton) != 0)
	{
		if (gra_messages_typingin == 0)
		{
			*special |= ACCELERATORDOWN;
			state = tolower(state);
		} else
		{
			if (state >= 'a' && state <= 'z') state -= 'a' - 1; else
				if (state >= 'A' && state <= 'Z') state -= 'A' - 1;
		}
	}
	return(state);
}

void GraphicsDraw::recalcSize()
{
	int i;

	/* allocate QImage for the offscreen buffer */
	QImage newImage;
	if (!newImage.create( width(), height(), 8, 256 )) return;
	newImage.fill( 0 );
	for (i = 0; i < 256; i++) newImage.setColor( i, gra_colortable[i] );
	image = newImage;
	wf->rowstart = newImage.jumpTable();

#ifdef ETRACE
	etrace(GTRACE, "  gra_recalcsize: wf=%d swid=%d shei=%d\n",
	       wf->windindex, width(), height());
#endif
	wf->swid = width();
	wf->shei = height();
	wf->revy = wf->shei - 1;
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

/*************************** SESSION LOGGING ROUTINES ***************************/

/* Session Playback */

/*
 * routine to create a session logging file
 */
void logstartrecord(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}logstartrecord\n");
#endif
}

/*
 * routine to begin playback of session logging file "file".  The routine
 * returns true if there is an error.  If "all" is nonzero, playback
 * the entire file with no prompt.
 */
BOOLEAN logplayback(char *file)
{
#ifdef ETRACE
	etrace(GTRACE, "{}logplayback: file=%s result=1\n", file);
#else
	Q_UNUSED( file );
#endif
	return(TRUE);
}

/*
 * routine to terminate session logging
 */
void logfinishrecord(void)
{
#ifdef ETRACE
	etrace(GTRACE, "{}logfinishrecord\n");
#endif
}

/****************************** MENUS ******************************/

void getacceleratorstrings(char **acceleratorstring, char **acceleratorprefix)
{
#ifdef MACOS
	*acceleratorstring = "Cmd";
	*acceleratorprefix = "Cmd-";
#else
	*acceleratorstring = "Ctrl";
	*acceleratorprefix = "Ctrl-";
#endif
}

char *getinterruptkey(void)
{
#ifdef MACOS
	return(_("Command-."));
#else
	return(_("Control-C in Messages Window"));
#endif
}

INTBIG nativepopupmenu(POPUPMENU **menuptr, BOOLEAN header, INTBIG left, INTBIG top)
{
	INTBIG i, j, submenuindex, submenus;
	POPUPMENUITEM *mi, *submi;
	POPUPMENU *menu, **subpmlist;
	REGISTER WINDOWFRAME *wf;
	REGISTER USERCOM *uc;
	INTBIG menuresult;

	menu = *menuptr;
	wf = el_curwindowframe;
	QWidget *parent = wf != NOWINDOWFRAME ? wf->draw->parentWidget() : gra->mainWidget();
	EPopupMenu qmenu( parent, *menuptr );
	if (header)
	{
		qmenu.insertItem( QString::fromLocal8Bit( menu->header ), -1 );
		qmenu.insertSeparator();
	}

	/* count the number of submenus */
	submenus = 0;
	subpmlist = 0;
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
	}

	/* load the menus */
	submenus = 0;
	for(j=0; j<menu->total; j++)
	{
		mi = &menu->list[j];
		mi->changed = FALSE;
		if (*mi->attribute == 0)
		{
			qmenu.insertSeparator();
		} else
		{
			uc = mi->response;
			if (uc != NOUSERCOM && uc->menu != NOPOPUPMENU)
			{
				QPopupMenu *submenu = new QPopupMenu( &qmenu );
				if (submenu == 0) return(-1);
				qmenu.insertItem( QString::fromLocal8Bit( mi->attribute ), submenu, j );
				subpmlist[submenus] = uc->menu;
				submenus++;
				for(i=0; i<uc->menu->total; i++)
				{
					submi = &uc->menu->list[i];
					submenu->insertItem( QString::fromLocal8Bit( submi->attribute ), (submenus<<16) | i );
				}
			} else
			{
				QString qstr = QString::fromLocal8Bit( mi->attribute );
				if (mi->value)
				{
					qstr += "\t" + QString::fromLocal8Bit( mi->value );
				}
				qmenu.insertItem( qstr, j );
			}
		}
	}

	QPoint pos;
	if (left >= 0 && top >= 0 && el_curwindowframe != NOWINDOWFRAME)
	{
		pos = el_curwindowframe->draw->mapToGlobal( QPoint( left, el_curwindowframe->revy - top ) );
	}
	else pos = QCursor::pos();
	menuresult = qmenu.exec( pos );
	if (menuresult >= 0)
	{
		submenuindex = menuresult >> 16;
		if (submenuindex != 0) *menuptr = subpmlist[submenuindex-1];
	}
	if (submenus > 0)
	{
		efree((char *)subpmlist);
	}
	if (menuresult < 0) return(-1);
	return(menuresult & 0xFFFF);
}

EPopupMenu::EPopupMenu( QWidget *parent, struct Ipopupmenu *menu )
	: QPopupMenu( parent ), menu( menu )
{
}

void EPopupMenu::keyPressEvent( QKeyEvent *e )
{
	int i;
	for(i=0; i<menu->total; i++)
	{
		if (isItemActive( i )) break;
	}
	if (i < menu->total)
	{
		POPUPMENUITEM *mi = &menu->list[i];
		USERCOM *uc = mi->response;
		if (*mi->attribute != 0 && (uc == NOUSERCOM || uc->menu == NOPOPUPMENU) &&
			mi->value != 0 && mi->maxlen > 0)
		{
			int key = e->key();
			BOOLEAN changed = FALSE;
			if ((e->state() & ControlButton) && key == Key_U)
			{ /* clear */
				mi->value[0] = 0;
				changed = TRUE;
			} else if ((e->state() & ControlButton) && key == Key_H ||
				!e->state() && (key == Key_Backspace || key == Key_Delete))
			{ /* backspace */
				INTBIG len = estrlen(mi->value);
				if (len > 0) mi->value[len - 1] = 0;
				changed = TRUE;
			} else if (!e->state() && e->text().length() == 1 && e->text()[0] >= ' ')
			{ /* append */
				if (!mi->changed) mi->value[0] = 0;
				INTBIG len = estrlen(mi->value);
				if (len < mi->maxlen)
				{
					char *str = EApplication::localize( e->text() );
					mi->value[len] = *str;
					mi->value[len + 1] = 0;
				}
				changed = TRUE;
			}
			if (changed)
			{
				QString qstr = QString::fromLocal8Bit( mi->attribute );
				if (mi->value)
				{
					qstr += "\t" + QString::fromLocal8Bit( mi->value );
				}
				changeItem( qstr, i );
				mi->changed = TRUE;
				return;
			}
		}
	}
	QPopupMenu::keyPressEvent( e );
}

void nativemenuload(INTBIG count, char *par[])
{
	REGISTER INTBIG i;
	REGISTER WINDOWFRAME *wf;

	/* remember the top-level pulldown menus */
	for(i=0; i<count; i++) gra_pulldowns_[i] = us_getpopupmenu(par[i]);
	gra_pulldownmenucount_ = count;

	/* build the pulldown menu bar */
#ifdef USEMDI
	gra->mw->pulldownmenuload();
#else
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;
		/* load menus into this window */
		EMainWindow *qwin = (EMainWindow*)wf->draw->parentWidget();
		qwin->pulldownmenuload();
	}
#  ifdef MESSMENU
	EMainWindow *ewin = (EMainWindow*)gra->messages->parentWidget();
	ewin->pulldownmenuload();
#  endif
#endif
}

void nativemenurename(POPUPMENU *pm, INTBIG pindex)
{
	REGISTER WINDOWFRAME *wf;

#ifdef ETRACE_
	POPUPMENUITEM *mi = &pm->list[pindex];
	etrace(GTRACE, "{ nativemenurename %s %d '%s'\n", pm->name, pindex,
	       mi->attribute != 0 ? mi->attribute : "");
#endif
	/* build the pulldown menu bar */
#ifdef USEMDI
	gra->mw->nativemenurename( pm, pindex );
#else
	for(wf = el_firstwindowframe; wf != NOWINDOWFRAME; wf = wf->nextwindowframe)
	{
		if (wf->floating) continue;
		/* load menus into this window */
		EMainWindow *qwin = (EMainWindow*)wf->draw->parentWidget();
		qwin->nativemenurename( pm, pindex );
	}
#  ifdef MESSMENU
	EMainWindow *ewin = (EMainWindow*)gra->messages->parentWidget();
	ewin->nativemenurename( pm, pindex );
#  endif
#endif
#ifdef ETRACE_
	etrace(GTRACE, "} nativemenurename\n");
#endif
}

EMainWindow::EMainWindow( QWidget *parent, char *name, WFlags f, BOOLEAN status )
	: QMainWindow( parent, name, f )
{
    pulldownmenus = 0;
    pulldowns = 0;
	pulldownmenucount = 0;
    if ( status )
	{
        for (int i = 0; i < gra_indicatorcount; i++)
		{
            statusItems[i] = new EStatusItem( statusBar() );
			STATUSFIELD *sf = gra_statusfields[i];
			statusBar()->addWidget( statusItems[i], sf->endper - sf->startper );
			statusItems[i]->setText( gra_statusfieldtext[i] );
		}
    }
}

EMainWindow::~EMainWindow()
{
	if (pulldownmenus != 0) efree((char*)pulldownmenus);
	if (pulldowns != 0)
	{
		for (int i = 0; i < pulldownmenucount; i++)
		{
			if (pulldowns[i] != NOSTRING) efree(pulldowns[i]);
		}
		efree((char*)pulldowns);
	}
}

void EMainWindow::pulldownmenuload()
{
	menuBar()->clear();
	for(int i=0; i<gra_pulldownmenucount_; i++)
	{
		POPUPMENU *pm = gra_pulldowns_[i];
		if (pm == NOPOPUPMENU) continue;
		INTSML menuindex = pulldownindex(pm);
		if (menuindex < 0) continue;
		QPopupMenu *menu = pulldownmenus[menuindex];
		if (menu != NULL)
			menuBar()->insertItem( QString::fromLocal8Bit( pm->header ), menu );
	}
}

void EMainWindow::nativemenurename(POPUPMENU *pm, INTBIG pindex)
{
	Q_UNUSED( pindex );

	int i;

	/* find this pulldown menu */
	for(i=0; i<pulldownmenucount; i++)
		if (pulldowns[i] != NOSTRING && namesame(pulldowns[i], pm->name) == 0) break;
	if (i >= pulldownmenucount) return;

	pulldownmenus[i]->clear();
	makepdmenu( pm, i);
}

/*
* Routine to create a pulldown menu from popup menu "pm".
* Returns an index to the table of pulldown menus (-1 on error).
*/
INTSML EMainWindow::pulldownindex(POPUPMENU *pm)
{
    REGISTER INTBIG i, pindex;
    QPopupMenu **newpulldownmenus;
    char **newpulldowns;

    /* see if it is in the list already */
    for(i=0; i<pulldownmenucount; i++)
	    if (pulldowns[i] != NOSTRING && namesame(pulldowns[i], pm->name) == 0) return(i);

    /* allocate new space with one more */
    newpulldownmenus = (QPopupMenu **)emalloc((pulldownmenucount+1) *
	    (sizeof (QPopupMenu *)), us_tool->cluster);
    if (newpulldownmenus == 0) return(-1);
    newpulldowns = (char **)emalloc((pulldownmenucount+1) *
	    (sizeof (char *)), us_tool->cluster);
    if (newpulldowns == 0) return(-1);

    /* copy former arrays then delete them */
    for(i=0; i<pulldownmenucount; i++)
    {
	    newpulldownmenus[i] = pulldownmenus[i];
	    newpulldowns[i] = pulldowns[i];
    }
    if (pulldownmenucount != 0)
    {
	    efree((char *)pulldownmenus);
	    efree((char *)pulldowns);
    }

    pulldownmenus = newpulldownmenus;
    pulldowns = newpulldowns;

    pindex = pulldownmenucount++;
    pulldowns[pindex] = NOSTRING;
    pulldownmenus[pindex] = new QPopupMenu( this );
    if (pulldownmenus[pindex] == 0)
	    return(-1);
    connect( pulldownmenus[pindex], SIGNAL(activated(int)), SLOT(menuAction(int)));
    (void)allocstring(&pulldowns[pindex], pm->name, us_tool->cluster);
    makepdmenu( pm, pindex);
     return(pindex);
}

/*
 * Routine to create pulldown menu number "value" from the popup menu in "pm" and return
 * the menu handle.
 */
void EMainWindow::makepdmenu(POPUPMENU *pm, INTSML value)
{
	QPopupMenu *menu = pulldownmenus[value];
	char *pt;

	/* build the actual menu */
	for(int i=0; i<pm->total; i++)
	{
		POPUPMENUITEM *mi = &pm->list[i];
		USERCOM *uc = mi->response;
		int id = value*(1 << 16) + i;
		if (uc->active < 0)
		{
			if (*mi->attribute == 0)
			{
				/* separator */
				menu->insertSeparator();
			} else
			{
				/* dimmed item (DIM IT!!!) */
				menu->insertItem( QString::fromLocal8Bit( mi->attribute ), id );
				menu->setItemEnabled( id, FALSE );
			}
			continue;
		}

		/* see if this command is another menu */
		if (uc->menu != NOPOPUPMENU)
		{
			int submenuindex = pulldownindex(uc->menu);
			QPopupMenu *submenu = pulldownmenus[submenuindex];
			if (submenu != 0) menu->insertItem( QString::fromLocal8Bit( mi->attribute ), submenu, id );
			continue;
		}

		/* see if there is a check */
		int checked = -1;
		char myline[256];
		strcpy( myline, mi->attribute );
		int len = strlen(myline) - 1;
		if (myline[len] == '<')
		{
			myline[len] = 0;
			if (myline[0] == '>')
			{
				checked = 1;
				strcpy(myline, &myline[1]);
			} else
			{
				checked = 0;
			}
		}

		/* get command title and accelerator */
		for(pt = myline; *pt != 0; pt++) if (*pt == '/' || *pt == '\\') break;
		if (*pt != 0)
		{
			INTSML key;
			INTBIG special;
			(void)us_getboundkey(pt, &key, &special);
#ifdef MACOSX
			esnprintf(pt, 50, x_("     %s"), us_describeboundkey(key, special, 1));
#else
			esnprintf(pt, 50, x_("\t%s"), us_describeboundkey(key, special, 1));
#endif
		}
#ifdef ETRACE
		etrace(GTRACE, "  set menu entry '%s'\n", myline );
#endif
		menu->insertItem( QString::fromLocal8Bit( myline ) , id );
		if (checked != -1) menu->setItemChecked( id, checked > 0 );
	}
}

void EMainWindow::menuAction( int id )
{
	/* handle menu events */
	INTSML low = id >> 16;
	INTSML high = id & 0xFFFF;
	BOOLEAN verbose;
	static INTSML ntry = 0;
#ifdef ETRACE
	etrace(GTRACE, "{ menuAction: %s %d %d\n", stateNames[gra_state], low, high);
#endif
	if (gra_state != S_USER && ntry < 2)
	{
		ttyputmsg(_("menu disabled in input mode, state=%d, ntry=%d"), gra_state,ntry);
		ttybeep(SOUNDBEEP, TRUE);
#ifdef ETRACE
		etrace(GTRACE, "} menuAction: ignored\n");
#endif
		ntry++;
		return;	
	}
	ntry = 0;
	POPUPMENU *pm = us_getpopupmenu(pulldowns[low]);
	if (high >= 0 && high < pm->total)
	{
		us_state |= DIDINPUT;
		us_state &= ~GOTXY;
#ifdef ETRACE
		etrace(GTRACE, "  menuAction: DIDINPUT GOTXY\n");
#endif
		setdefaultcursortype(NULLCURSOR);
		us_forceeditchanges();
		if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
			verbose = FALSE;
		us_execute(pm->list[high].response, verbose, TRUE, TRUE);
		db_setcurrenttool(us_tool);
		setactivity(pm->list[high].attribute);
		gra->toolTimeSlice();
	}
#ifdef ETRACE
	etrace(GTRACE, "}  menuAction\n");
#endif
}

void EMainWindow::addStatusItem( int stretch, char *str )
{
	EStatusItem *statusItem = new EStatusItem( statusBar() );
	statusItem->setText( str );
	statusItems[gra_indicatorcount] = statusItem;
	statusItem->show();
	statusBar()->addWidget( statusItem, stretch );
}

void EMainWindow::removeStatusItem( int i )
{
	EStatusItem *statusItem = statusItems[i];
	statusBar()->removeWidget( statusItem );
	delete statusItem;
	for(int j=i+1; j<gra_indicatorcount; j++)
	{
		statusItems[j-1] = statusItems[j];
		statusItems[j] = 0;
	}
}

void EMainWindow::changeStatusItem( int i, char *str )
{
	EStatusItem *statusItem = statusItems[i];

	/* load the string */
	statusItem->setText( str );
}

EStatusItem::EStatusItem( QWidget *parent, char *name )
    : QFrame( parent, name )
{
}

void EStatusItem::setText( const QString &text )
{
    stext = text;
	update( contentsRect() );
}

void EStatusItem::drawContents( QPainter *p )
{
#ifdef MACOSX
	QFont curfont = p->font();
	int size = curfont.pointSize();
	if (size != 9)
	{
		curfont.setPointSize(9);
		p->setFont(curfont);
	}
#endif
    p->drawText( contentsRect(), AlignAuto | AlignVCenter | SingleLine, stext );
}

/****************************** PRINTING ******************************/

/* call this routine to print the current window */
void printewindow(void)
{
	REGISTER WINDOWPART *win;
	REGISTER WINDOWFRAME *oldWf, *wf;
	INTBIG lx, hx, ly, hy, i, margin, marginpixels;
	REGISTER VARIABLE *var;
	static QPrinter *printer = 0;
	QPainter p;
	QRect r;
	QRgb white, oldbackground;

	win = el_curwindowpart;
	if (win == NOWINDOWPART)
	{
		ttyputerr(_("No current window to print"));
		return;
	}
	oldWf = win->frame;

	if (printer == 0)
		printer = new QPrinter( QPrinter::PrinterResolution );
/*	printer->setPageSize(QPrinter::Custom); */
	if (printer->setup(gra->mw))
	{
		QPaintDeviceMetrics pdm( printer );
		if (!p.begin(printer)) return;

		/* determine margins */
		var = getval((INTBIG)io_tool, VTOOL, VFRACT, x_("IO_postscript_margin"));
		if (var == NOVARIABLE) margin = WHOLE/4*3; else
			margin = var->addr;
		marginpixels = muldiv(margin, pdm.logicalDpiX(), WHOLE);
		/* ttyputmsg(M_("Printer width=%d height=%d margin=%d"), pdm.width(), pdm.height(), marginpixels); */

		INTBIG saveslx = win->screenlx;
		INTBIG saveshx = win->screenhx;
		INTBIG savesly = win->screenly;
		INTBIG saveshy = win->screenhy;
		INTBIG saveulx = win->uselx;
		INTBIG saveuhx = win->usehx;
		INTBIG saveuly = win->usely;
		INTBIG saveuhy = win->usehy;

		wf = (WINDOWFRAME *)emalloc((sizeof (WINDOWFRAME)), us_tool->cluster);
		if (wf == 0) return;
		win->frame = wf;
		wf->draw = 0;
		wf->starttime = 0;
		wf->offscreendirty = FALSE;
		wf->copyleft = wf->copyright = wf->copytop = wf->copybottom = 0;
		wf->floating = FALSE;
		wf->swid = pdm.width();
		wf->shei = pdm.height();
		wf->revy = wf->shei - 1;
		wf->nextwindowframe = NOWINDOWFRAME;
		wf->firstvar = NOVARIABLE;
		wf->numvar = 0;
		QImage image;
		if (!image.create( pdm.width(), pdm.height(), 8, 256))
		{
			efree((CHAR *)wf);
			return;
		}
		image.fill( 0 );
		white = qRgb(255, 255, 255);
		image.setColor( 0, white);
		for (i = 1; i < 256; i++) image.setColor( i, gra_colortable[i] );
		wf->rowstart = image.jumpTable();
			
		win->uselx = marginpixels;
		win->usehx = wf->swid - marginpixels;
		win->usely = marginpixels;
		win->usehy = wf->shei - marginpixels;

		INTBIG wlx, whx, wly, why;
		INTBIG slx, shx, sly, shy;
		NODEPROTO *np = win->curnodeproto;
		if ((win->state&WINDOWTYPE) == WAVEFORMWINDOW || np == NONODEPROTO)
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
		win->frame = oldWf;
		win->screenlx = saveslx;
		win->screenhx = saveshx;
		win->screenly = savesly;
		win->screenhy = saveshy;
		win->uselx = saveulx;
		win->usehx = saveuhx;
		win->usely = saveuly;
		win->usehy = saveuhy;
		computewindowscale(win);

		r.rTop() = 0; r.rBottom() = wf->shei - 1;
		r.rLeft() = 0; r.rRight() = wf->swid - 1;
#if 0
		/* printer->newPage(); */
		lx = win->uselx;   hx = win->usehx;
		ly = wf->revy - win->usehy; hy = wf->revy - win->usely;

		r.rTop() = ly;   r.rBottom() = hy;
		r.rLeft() = lx;  r.rRight() = hx;

		/* make background color be white */

		oldbackground = wf->draw->image.color(0);
		wf->draw->image.setColor(0, white);
#endif

		/* draw the image */
		p.drawImage(r.topLeft(), image, r);
		p.end();

		efree((CHAR *)wf);
	}
}
