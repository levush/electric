/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usr.c
 * User interface tool: main module
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
#include "edialogs.h"
#include "tech.h"
#include "usr.h"
#include "usrtrack.h"
#include "usrdiacom.h"
#include "sim.h"
#include "tecart.h"
#include "tecgen.h"
#include "tecschem.h"
#include <setjmp.h>		/* for nonlocal goto's */

TOOL        *us_tool;					/* the USER tool object */
INTBIG       us_longcount;				/* number of long commands */
void       (*us_displayroutine)(POLYGON*, WINDOWPART*); /* routine for graphics display */
INTBIG       us_currenteditor;			/* the current editor (index to "us_editortable") */
INTBIG       us_cursorstate;			/* current cursor being displayed */
INTBIG       us_normalcursor;			/* default cursor to use */
BOOLEAN      us_optionschanged;			/* true if options changed */
INTBIG       us_layer_letters_key;		/* variable key for "USER_layer_letters" */
INTBIG       us_highlightedkey;			/* variable key for "USER_highlighted" */
INTBIG       us_highlightstackkey;		/* variable key for "USER_highlightstack" */
INTBIG       us_binding_keys_key;		/* variable key for "USER_binding_keys" */
INTBIG       us_binding_buttons_key;	/* variable key for "USER_binding_buttons" */
INTBIG       us_binding_menu_key;		/* variable key for "USER_binding_menu" */
INTBIG       us_current_node_key;		/* variable key for "USER_current_node" */
INTBIG       us_current_arc_key;		/* variable key for "USER_current_arc" */
INTBIG       us_placement_angle_key;	/* variable key for "USER_placement_angle" */
INTBIG       us_alignment_ratio_key;	/* variable key for "USER_alignment_ratio" */
INTBIG       us_alignment_edge_ratio_key;/* variable key for "USER_alignment_edge_ratio" */
INTBIG       us_arcstylekey;			/* variable key for "USER_arc_style" */
INTBIG       us_current_technology_key;	/* variable key for "USER_current_technology" */
INTBIG       us_current_window_key;		/* variable key for "USER_current_window" */
INTBIG       us_current_constraint_key;	/* variable key for "USER_current_constraint" */
INTBIG       us_colormap_red_key;		/* variable key for "USER_colormap_red" */
INTBIG       us_colormap_green_key;		/* variable key for "USER_colormap_green" */
INTBIG       us_colormap_blue_key;		/* variable key for "USER_colormap_blue" */
INTBIG       us_copyright_file_key;		/* variable key for "USER_copyright_file" */
INTBIG       us_menu_position_key;		/* variable key for "USER_menu_position" */
INTBIG       us_menu_x_key;				/* variable key for "USER_menu_x" */
INTBIG       us_menu_y_key;				/* variable key for "USER_menu_y" */
INTBIG       us_macrorunningkey;		/* variable key for "USER_macrorunning" */
INTBIG       us_macrobuildingkey;		/* variable key for "USER_macrobuilding" */
INTBIG       us_text_editorkey;			/* variable key for "USER_text_editor" */
INTBIG       us_optionflagskey;			/* variable key for "USER_optionflags" */
INTBIG       us_gridfloatskey;			/* variable key for "USER_grid_floats" */
INTBIG       us_gridboldspacingkey;		/* variable key for "USER_grid_bold_spacing" */
INTBIG       us_quickkeyskey;			/* variable key for "USER_quick_keys" */
INTBIG       us_interactiveanglekey;	/* variable key for "USER_interactive_angle" */
INTBIG       us_tinylambdaperpixelkey;	/* variable key for "USER_tiny_lambda_per_pixel" */
INTBIG       us_ignoreoptionchangeskey;	/* variable key for "USER_ignore_option_changes" */
INTBIG       us_displayunitskey;		/* variable key for "USER_display_units" */
INTBIG       us_electricalunitskey;		/* variable key for "USER_electrical_units" */
INTBIG       us_motiondelaykey;			/* variable key for "USER_motion_delay" */
INTBIG       us_java_flags_key;			/* variable key for "JAVA_flags" */
INTBIG       us_edtec_option_key;		/* variable key for "EDTEC_option" */
INTBIG       us_electricalunits;		/* mirror for "USER_electrical_units" */
INTBIG       us_javaflags;				/* mirror for "JAVA_flags" */
INTBIG       us_useroptions;			/* mirror for "USER_optionflags" */
INTBIG       us_tinyratio;				/* mirror for "USER_tiny_lambda_per_pixel" */
INTBIG       us_separatechar;			/* separating character in port names */
INTBIG       us_filetypecolormap;		/* Color map file descriptor */
INTBIG       us_filetypehelp;			/* Help file descriptor */
INTBIG       us_filetypelog;			/* Log file descriptor */
INTBIG       us_filetypemacro;			/* Macro file descriptor */
INTBIG       us_filetypenews;			/* News file descriptor */

/* local state */
static BOOLEAN    us_ignore_cadrc;		/* set when user wants to ignore the cadrc file */
static CHAR      *us_firstlibrary;		/* library to read upon startup */
static CHAR      *us_firstmacrofile;	/* macro file to read upon startup */
static INTBIG     us_techlaststatekey;	/* variable key for "TECH_last_state" */
static INTBIG     us_filetypecadrc;		/* Startup file descriptor*/
static CHAR       us_desiredlibrary[300];	/* library to be read at startup */
static BOOLEAN    us_logging = TRUE;	/* nonzero to start session logging */

/* for tracking broadcast changes */
static TOOL      *us_batchsource;
static WINDOWPART us_oldwindow, *us_firstnewwindow, *us_secondnewwindow, *us_killedwindow;
static INTBIG     us_maplow, us_maphigh, us_newwindowcount;
static BOOLEAN    us_menuchanged, us_gridfactorschanged, us_cellstructurechanged;
static INTBIG     us_oldstate, us_oldoptions;
static NODEPROTO *us_firstchangedcell, *us_secondchangedcell;
static NODEPROTO *us_cellwithkilledname;				/* cell whose name was just killed */
static CHAR      *us_killednameoncell;					/* name that was removed from cell */

static VARMIRROR us_variablemirror[] =
{
	{&us_optionflagskey,        &us_useroptions},
	{&us_tinylambdaperpixelkey, &us_tinyratio},
	{&us_menu_x_key,            &us_menux},
	{&us_menu_y_key,            &us_menuy},
	{&us_menu_position_key,     &us_menupos},
	{&us_java_flags_key,        &us_javaflags},
	{&us_electricalunitskey,    &us_electricalunits},
	{0, 0}
};

/* internal state */
INTBIG       us_state;					/* miscellaneous state of user interface */

/* when modifying this list of status fields, update "usrcomwz.c:us_statusfields[]" */
STATUSFIELD *us_statusalign = 0;		/* current node alignment */
STATUSFIELD *us_statusangle = 0;		/* current placement angle */
STATUSFIELD *us_statusarc = 0;			/* current arc prototype */
STATUSFIELD *us_statuscell = 0;			/* current cell */
STATUSFIELD *us_statuscellsize = 0;		/* current cell size */
STATUSFIELD *us_statusselectcount = 0;	/* current selection count */
STATUSFIELD *us_statusgridsize = 0;		/* current grid size */
STATUSFIELD *us_statuslambda = 0;		/* current lambda */
STATUSFIELD *us_statusnode = 0;			/* current node prototype */
STATUSFIELD *us_statustechnology = 0;	/* current technology */
STATUSFIELD *us_statusxpos = 0;			/* current x position */
STATUSFIELD *us_statusypos = 0;			/* current y position */
STATUSFIELD *us_statusproject = 0;		/* current project */
STATUSFIELD *us_statusroot = 0;			/* current root */
STATUSFIELD *us_statuspart = 0;			/* current part */
STATUSFIELD *us_statuspackage = 0;		/* current package */
STATUSFIELD *us_statusselection = 0;	/* current selection */

/* user interface state */
NODEPROTO   *us_curnodeproto;			/* current nodeproto */
ARCPROTO    *us_curarcproto;			/* current arcproto */
INTBIG       us_alignment_ratio;		/* current alignment in fractional units */
INTBIG       us_edgealignment_ratio;	/* current edge alignment in fractional units */
BOOLEAN      us_dupdistset;				/* true if duplication distance is valid */
NODEINST    *us_dupnode;				/* a node that was duplicated */
LIBRARY     *us_clipboardlib;			/* library with cut/copy/paste cell */
NODEPROTO   *us_clipboardcell;			/* cell with cut/copy/paste objects */
INTBIG       us_dupx, us_dupy;			/* amount to move duplicated objects */
WINDOWFRAME *us_menuframe;				/* window frame on which menu resides */
#ifndef USEQT
INTSML       us_erasech, us_killch;		/* the erase and kill characters */
#endif
INTBIG       us_menux, us_menuy;		/* number of menu elements on the screen */
INTBIG       us_menupos;				/* position: 0:top 1:bottom 2:left 3:right */
INTBIG       us_menuxsz,us_menuysz;		/* size of menu elements */
INTBIG       us_menuhnx,us_menuhny;		/* highlighted nodeinst menu entry */
INTBIG       us_menuhax,us_menuhay;		/* highlighted arcinst menu entry */
INTBIG       us_menulx,us_menuhx;		/* X: low and high of menu area */
INTBIG       us_menuly,us_menuhy;		/* Y: low and high of menu area */
INTBIG       us_lastmeasurex, us_lastmeasurey;	/* last measured distance */
BOOLEAN      us_validmesaure;			/* true if a measure was done */
INTBIG       us_explorerratio = 25;		/* percentage of window used by cell explorer */
FILE        *us_logrecord;				/* logging record file */
FILE        *us_logplay;				/* logging playback file */
FILE        *us_termaudit;				/* messages window auditing file */
FILE        *us_tracefile = NULL;
INTBIG       us_logflushfreq;			/* session logging flush frequency */
INTBIG       us_quickkeyfactcount = 0;	/* number of quick keys in "factory" setup */
CHAR       **us_quickkeyfactlist;		/* quick keys in "factory" setup */

INTBIG       us_lastcommandcount;		/* the keyword count for the last command set by the "remember" command */
INTBIG       us_lastcommandtotal = 0;	/* size of keyword array for the last command set by the "remember" command */
CHAR       **us_lastcommandpar;			/* the keywords for the last command set by the "remember" command */

MACROPACK   *us_macropacktop;			/* top of list of defined macro packages */
MACROPACK   *us_curmacropack;			/* current macro package */

USERCOM     *us_usercomfree;			/* list of free user commands */
USERCOM     *us_lastcom;				/* last command and arguments */

POPUPMENU   *us_firstpopupmenu;			/* list of existing pop-up menus */
INTBIG       us_pulldownmenucount;		/* number of pulldown menus */
INTBIG      *us_pulldownmenupos;		/* position of pulldown menus */
POPUPMENU   **us_pulldowns;				/* the current pulldown menus */

/* prototypes for local routines */
static BOOLEAN us_do2init(INTBIG*, CHAR1*[]);
static BOOLEAN us_do3init(void);
static void us_causeofslice(USERCOM*);
static void us_setcommand(CHAR*, USERCOM*, INTBIG, NODEPROTO*, ARCPROTO*, CHAR*, POPUPMENU*, BOOLEAN);
static BOOLEAN us_options(INTBIG*, CHAR1*[]);
static BOOLEAN us_docadrc(CHAR*);
static void us_findcadrc(void);
static void us_checkfontassociations(LIBRARY *lib);
static void us_setfontassociationdescript(UINTBIG *descript, INTBIG *fontmatch);
static void us_setfontassociationvar(INTSML numvar, VARIABLE *firstvar, INTBIG *fontmatch);
static void us_checkinplaceedits(NODEPROTO *cell);

/******************** CONTROL ********************/

/* initialize the user interface tool */
void us_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	if (thistool == 0)
	{
		/* pass 3 */
		if (us_do3init())
			error(_("No memory to run the user interface"));
	} else if (thistool == NOTOOL)
	{
		/* pass 2 */
		if (us_do2init(argc, argv))
			error(_("No memory to run the user interface"));
	} else
	{
		/* pass 1 */
		us_tool = thistool;
		el_topwindowpart = NOWINDOWPART;	/* no windows defined */
	}
}

/* pass 2 of initialization */
BOOLEAN us_do2init(INTBIG *argc, CHAR1 *argv[])
{
	CHAR *newpar[3], msg[150];
	REGISTER INTBIG i;
	INTBIG swid, shei, twid, thei;
	INTBIG keys[2];
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER TECHNOLOGY *tech;
	extern GRAPHICS us_gbox, us_box;
	REGISTER WINDOWFRAME *wf;
	REGISTER VARIABLE *varred, *vargreen, *varblue;

	/* initialize lists */
	us_usercomfree = NOUSERCOM;		/* no free command modules */

	/* define the default macro called "macro" */
	newpar[0] = newpar[1] = newpar[2] = x_("");
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_macro_macro")),
		(INTBIG)newpar, VSTRING|VISARRAY|(3<<VLENGTHSH)|VDONTSAVE);

	/* basic initialization */
	us_logplay = NULL;					/* no session playback */
	us_logrecord = NULL;				/* no session recording */
	us_termaudit = NULL;				/* no terminal auditing */
	us_logflushfreq = 10;				/* flush session recording every 10 */
	us_lastcom = NOUSERCOM;				/* no last command issued */
	us_macropacktop = NOMACROPACK;		/* no defined macro packages */
	us_curmacropack = NOMACROPACK;		/* no current macro package */
	us_menuhnx = us_menuhax = -1;		/* no menu highlighting */
	us_menupos = 0;						/* menus on the top */
	us_firstpopupmenu = NOPOPUPMENU;	/* no popup menus yet */
	us_pulldownmenucount = 0;			/* no pulldown menus in menubar */
	us_state = 0;						/* all is well */
	us_dupdistset = FALSE;				/* duplication distance not set */
	us_dupnode = NONODEINST;			/* duplication node not set */
	us_currenteditor = 0;				/* use the first editor */
	us_tool->toolstate |= INTERACTIVE;	/* use interactive cursor commands */
	us_validmesaure = FALSE;			/* nothing measured */
	us_tinyratio = K4;					/* default ratio for hashing tiny cells */
	us_javaflags = 0;					/* default Java flags */
	us_separatechar = '_';				/* default separating character */
	us_curarcproto = NOARCPROTO;
	us_curnodeproto = NONODEPROTO;
	us_layer_letters_key = makekey(x_("USER_layer_letters"));
	us_highlightedkey = makekey(x_("USER_highlighted"));
	us_highlightstackkey = makekey(x_("USER_highlightstack"));
	us_binding_keys_key = makekey(x_("USER_binding_keys"));
	us_binding_buttons_key = makekey(x_("USER_binding_buttons"));
	us_binding_menu_key = makekey(x_("USER_binding_menu"));
	us_current_node_key = makekey(x_("USER_current_node"));
	us_current_arc_key = makekey(x_("USER_current_arc"));
	us_placement_angle_key = makekey(x_("USER_placement_angle"));
	us_alignment_ratio_key = makekey(x_("USER_alignment_ratio"));
	us_alignment_edge_ratio_key = makekey(x_("USER_alignment_edge_ratio"));
	us_arcstylekey = makekey(x_("USER_arc_style"));
	us_current_technology_key = makekey(x_("USER_current_technology"));
	us_current_window_key = makekey(x_("USER_current_window"));
	us_current_constraint_key = makekey(x_("USER_current_constraint"));
	us_colormap_red_key = makekey(x_("USER_colormap_red"));
	us_colormap_green_key = makekey(x_("USER_colormap_green"));
	us_colormap_blue_key = makekey(x_("USER_colormap_blue"));
	us_copyright_file_key = makekey(x_("USER_copyright_file"));
	us_menu_position_key = makekey(x_("USER_menu_position"));
	us_menu_x_key = makekey(x_("USER_menu_x"));
	us_menu_y_key = makekey(x_("USER_menu_y"));
	us_macrorunningkey = makekey(x_("USER_macrorunning"));
	us_macrobuildingkey = makekey(x_("USER_macrobuilding"));
	us_text_editorkey = makekey(x_("USER_text_editor"));
	us_gridfloatskey = makekey(x_("USER_grid_floats"));
	us_gridboldspacingkey = makekey(x_("USER_grid_bold_spacing"));
	us_quickkeyskey = makekey(x_("USER_quick_keys"));
	us_interactiveanglekey = makekey(x_("USER_interactive_angle"));
	us_tinylambdaperpixelkey = makekey(x_("USER_tiny_lambda_per_pixel"));
	us_techlaststatekey = makekey(x_("TECH_last_state"));
	us_optionflagskey = makekey(x_("USER_optionflags"));
	us_ignoreoptionchangeskey = makekey(x_("USER_ignore_option_changes"));
	us_displayunitskey = makekey(x_("USER_display_units"));
	us_electricalunitskey = makekey(x_("USER_electrical_units"));
	us_motiondelaykey = makekey(x_("USER_motion_delay"));
	us_java_flags_key = makekey(x_("JAVA_flags"));
	us_edtec_option_key = makekey(x_("EDTEC_option"));

	/* setup disk file descriptors */
	us_filetypecadrc    = setupfiletype(x_(""),     x_("*.*"),    MACFSTAG('TEXT'), FALSE, x_("cadrc"), _("Startup"));
	us_filetypecolormap = setupfiletype(x_("map"),  x_("*.map"),  MACFSTAG('TEXT'), FALSE, x_("color"), _("Color map"));
	us_filetypehelp     = setupfiletype(x_("help"), x_("*.help"), MACFSTAG('TEXT'), FALSE, x_("help"), _("Help"));
	us_filetypelog      = setupfiletype(x_("log"),  x_("*.log"),  MACFSTAG('LOG '), FALSE,  x_("log"), _("Log"));
	us_filetypemacro    = setupfiletype(x_("mac"),  x_("*.mac"),  MACFSTAG('TEXT'), FALSE, x_("macro"), _("Macro package"));
	us_filetypenews     = setupfiletype(x_("news"), x_("*.news"), MACFSTAG('TEXT'), FALSE, x_("news"), _("News"));

	/* initially ignore all graphics */
	us_displayroutine = us_nulldisplayroutine;

	/* set opaque graphics data */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		us_figuretechopaque(tech);

	/* set selectability/invisibility on primitives */
	us_figuretechselectability();

	/* register technology-caching routines */
	keys[0] = us_layer_letters_key;
	registertechnologycache(us_initlayerletters, 1, keys);

	/* count the number of commands */
	us_longcount = 0;
	for(i=0; us_lcommand[i].name != 0; i++) us_longcount++;

	/* get switches to the program */
	us_firstlibrary = 0;
	us_desiredlibrary[0] = 0;
	us_firstmacrofile = 0;
	graphicsoptions(x_("display"), argc, argv);
	if (us_options(argc, argv)) return(TRUE);

	/* establish the library location */
	setupenvironment();

	/* initialize status output and graphics */
	if (initgraphics(TRUE)) exit(1);
	us_getcolormap(el_curtech, COLORSDEFAULT, FALSE);
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	colormapload((INTBIG *)varred->addr, (INTBIG *)vargreen->addr,
		(INTBIG *)varblue->addr, 0, 255);

	/* establish the colors to use for peripheral graphics */
	if (el_maplength == 2)
	{
		el_colcelltxt = 0;
		el_colcell = 0;
		el_colwinbor = 0;
		el_colhwinbor = 1;
		el_colmenbor = 0;
		el_colhmenbor = 1;
		el_colmentxt = 0;
		el_colmengly = 0;
		el_colcursor = 0;
	} else
	{
		if (el_maplength < 256)
		{
			us_gbox.bits = LAYERA;
			us_gbox.col = BLACK;
		} else
		{
			us_gbox.bits = LAYERG;
			us_gbox.col = GRID;
		}
		el_colcelltxt = CELLTXT;
		el_colcell = CELLOUT;
		el_colwinbor = WINBOR;
		el_colhwinbor = HWINBOR;
		el_colmenbor = MENBOR;
		el_colhmenbor = HMENBOR;
		el_colmentxt = MENTXT;
		el_colmengly = MENGLY;
	}

	us_initstatus();

	/* initialize state */
	nextchangequiet();
	(void)setval((INTBIG)us_tool, VTOOL, x_("USER_icon_style"),
		ICONSTYLEDEFAULT, VINTEGER|VDONTSAVE);

	/* create the window frame that has the palette */
	us_menuframe = newwindowframe(TRUE, 0);

	/* create the initial window structure */
	wf = newwindowframe(FALSE, 0);
	if (wf == NOWINDOWFRAME) wf = getwindowframe(FALSE);
	el_curwindowpart = newwindowpart(x_("entire"), NOWINDOWPART);
	if (el_curwindowpart == NOWINDOWPART) return(TRUE);
	el_curwindowpart->frame = wf;
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key,
		(INTBIG)el_curwindowpart, VWINDOWPART|VDONTSAVE);
	el_curwindowpart->buttonhandler = DEFAULTBUTTONHANDLER;
	el_curwindowpart->charhandler = DEFAULTCHARHANDLER;
	el_curwindowpart->changehandler = DEFAULTCHANGEHANDLER;
	el_curwindowpart->termhandler = DEFAULTTERMHANDLER;
	el_curwindowpart->redisphandler = DEFAULTREDISPHANDLER;

	/* set shadows */
	changesquiet(TRUE);
	us_useroptions = PORTSFULL|CENTEREDPRIMITIVES|AUTOSWITCHTECHNOLOGY|CELLCENTERALWAYS;
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_optionflagskey,
		(INTBIG)us_useroptions, VINTEGER|VDONTSAVE);
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_constraint_key,
		(INTBIG)el_curconstraint, VCONSTRAINT|VDONTSAVE);
	us_alignment_ratio = WHOLE;
	us_edgealignment_ratio = 0;
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_displayunitskey,
		el_units&DISPLAYUNITS, VINTEGER|VDONTSAVE);
	us_electricalunits = ELEUNITDEFAULT;
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_electricalunitskey,
		us_electricalunits, VINTEGER|VDONTSAVE);
	changesquiet(FALSE);

	/* if the screen is seriously wide, put menus on the left */
	getwindowframesize(el_curwindowpart->frame, &swid, &shei);
	if (swid*2 > shei*3) us_menupos = 2;

	/* display initial message */
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(20));
	screensettextinfo(el_curwindowpart, NOTECHNOLOGY, descript);
	us_box.col = BLACK;
	esnprintf(msg, 150, _("Electric Version %s"), el_version);
	screengettextsize(el_curwindowpart, msg, &twid, &thei);
	screendrawtext(el_curwindowpart, (swid-twid)/2, shei/2+thei, msg, &us_box);
	estrcpy(msg, _("Loading..."));
	screengettextsize(el_curwindowpart, msg, &twid, &thei);
	screendrawtext(el_curwindowpart, (swid-twid)/2, shei/2-thei, msg, &us_box);
	flushscreen();
	return(FALSE);
}

/* pass 3 of initialization */
BOOLEAN us_do3init(void)
{
	REGISTER CHAR *libname, *libfile;
	REGISTER INTBIG oldverbose, len, lambda, haveoptionsfile, *msgloc, filestatus;
	REGISTER VARIABLE *var, *varred, *vargreen, *varblue;
	CHAR *newpar[3], clipboardname[20];
	REGISTER LIBRARY *lib;

	/* default command binding */
	if (us_initialbinding()) return(TRUE);

	/* initial display in status area */
	us_redostatus(NOWINDOWFRAME);

	/* set application environment */
#if LANGLISP
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_have_lisp")), 1,
		VINTEGER|VDONTSAVE);
#endif
#if LANGTCL
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_have_tcl")), 1,
		VINTEGER|VDONTSAVE);
#endif
#if LANGJAVA
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_have_java")), 1,
		VINTEGER|VDONTSAVE);
#endif

#ifdef FORCECADENCE
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_have_cadence")), 1,
		VINTEGER|VDONTSAVE);
#endif

#ifdef FORCESUNTOOLS
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_have_suntools")), 1,
		VINTEGER|VDONTSAVE);
#endif

	/* set variable to tell whether there needs to be a quit command */
	if (!graphicshas(CANHAVEQUITCOMMAND))
	{
		nextchangequiet();
		(void)setvalkey((INTBIG)us_tool, VTOOL, makekey(x_("USER_no_quit_command")), 1,
			VINTEGER|VDONTSAVE);
	}

	/* read "cadrc" files */
	if (!us_ignore_cadrc) us_findcadrc();
	if (us_firstmacrofile != 0)
	{
		if (us_docadrc(us_firstmacrofile))
			ttyputerr(_("Cannot find startup file '%s'"), us_firstmacrofile);
		efree((CHAR *)us_firstmacrofile);
		us_firstmacrofile = 0;
	}

	/* gather initial quick keys and remember them as "factory settings" */
	us_buildquickkeylist();
	us_getquickkeylist(&us_quickkeyfactcount, &us_quickkeyfactlist);
	if (us_quickkeyfactcount > 0)
	{
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_quickkeyskey, (INTBIG)us_quickkeyfactlist,
			VSTRING|VISARRAY|(us_quickkeyfactcount<<VLENGTHSH)|VDONTSAVE);
	}

	/* initial setup of the color map */
	us_getcolormap(el_curtech, COLORSDEFAULT, FALSE);

	/* mark the current state of the color map as "unchanged" */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred != NOVARIABLE) varred->type |= VDONTSAVE;
	if (vargreen != NOVARIABLE) vargreen->type |= VDONTSAVE;
	if (varblue != NOVARIABLE) varblue->type |= VDONTSAVE;

	/* read the options library */
	libname = us_tempoptionslibraryname();
	libfile = optionsfilepath();
	haveoptionsfile = 0;
	filestatus = fileexistence(truepath(libfile));
	if (filestatus == 1 || filestatus == 3)
	{
		haveoptionsfile = 1;
		lib = newlibrary(libname, libfile);
		if (lib == NOLIBRARY) ttyputerr(_("Cannot create options library %s"), libfile); else
		{
			lib->userbits |= HIDDENLIBRARY;
			oldverbose = asktool(io_tool, x_("verbose"), 0);
			(void)asktool(io_tool, x_("read"), (INTBIG)lib, (INTBIG)x_("binary"));
			(void)asktool(io_tool, x_("verbose"), oldverbose);
			killlibrary(lib);

			/* adjust messages window position if information is there */
			var = getval((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, x_("USER_messages_position"));
			if (var != NOVARIABLE)
			{
				msgloc = (INTBIG *)var->addr;
				setmessagesframeinfo(msgloc[0], msgloc[1], msgloc[2], msgloc[3]);
			}
		}
	}
	cacheoptionbitvalues();

	/* read an initial library if it was specified in program parameters */
	if (us_firstlibrary != 0)
	{
		if (reallocstring(&el_curlib->libfile, us_firstlibrary, el_curlib->cluster) != 0)
			return(TRUE);
		libname = skippath(us_firstlibrary);
		if (reallocstring(&el_curlib->libname, libname, el_curlib->cluster) != 0)
			return(TRUE);
		len = estrlen(el_curlib->libname);
		if (namesame(&el_curlib->libname[len-5], x_(".elib")) == 0)
			el_curlib->libname[len-5] = 0;
		if (asktool(io_tool, x_("read"), (INTBIG)el_curlib, (INTBIG)x_("binary")) != 0)
		{
			ttyputerr(_("Could not read %s"), el_curlib->libfile);
			el_curlib->curnodeproto = NONODEPROTO;
		} else
		{
			if (el_curlib->curnodeproto != NONODEPROTO)
			{
				if (el_curlib->curnodeproto->tech != gen_tech)
					el_curtech = el_curlib->curnodeproto->tech;
				if (el_curtech != sch_tech && el_curtech != art_tech && el_curtech != gen_tech)
					el_curlayouttech = el_curtech;
			}
		}
		efree((CHAR *)us_firstlibrary);
		us_firstlibrary = 0;
	} else el_curlib->curnodeproto = NONODEPROTO;

	/* set the user's shadow of the current technology */
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_technology_key,
		(INTBIG)el_curtech, VTECHNOLOGY|VDONTSAVE);

	/* setup menu properly */
	if (el_curtech != sch_tech)
	{
		newpar[0] = x_("size");
		newpar[1] = x_("auto");
		us_menu(2, newpar);
		us_setmenunodearcs();
	}

	/* load the color map onto the display */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	colormapload((INTBIG *)varred->addr, (INTBIG *)vargreen->addr,
		(INTBIG *)varblue->addr, 0, 255);

	/* setup current window */
	lambda = el_curlib->lambda[el_curtech->techindex];
	if (el_curwindowpart != NOWINDOWPART)
	{
		var = getval((INTBIG)us_tool, VTOOL, VFRACT|VISARRAY, x_("USER_default_grid"));
		if (var == NOVARIABLE)
		{
			el_curwindowpart->gridx = el_curwindowpart->gridy = WHOLE;
		} else
		{
			el_curwindowpart->gridx = ((INTBIG *)var->addr)[0];
			el_curwindowpart->gridy = ((INTBIG *)var->addr)[1];
		}
	}
	us_curarcproto = el_curtech->firstarcproto;
	us_curnodeproto = NONODEPROTO;

	/* create the clipboard cell */
	estrcpy(clipboardname, x_("Clipboard!!"));
	us_clipboardlib = newlibrary(clipboardname, clipboardname);
	us_clipboardlib->userbits |= HIDDENLIBRARY;
	us_clipboardcell = newnodeproto(clipboardname, us_clipboardlib);

	/* now allow graphics to reach the screen */
	us_displayroutine = us_showpoly;

	/* now draw everything */
	us_drawmenu(1, NOWINDOWFRAME);	/* draw basic screen layout */

#ifndef MACOS
	if (haveoptionsfile == 0)
	{
		if ((us_tool->toolstate&USEDIALOGS) != 0)
		{
			(void)us_aboutdlog();
		} else
			ttyputmsg(_("Electric, from Static Free Software"));
	}
#endif

	/* if there is a current cell in the library, edit it */
	if (el_curlib->curnodeproto != NONODEPROTO)
	{
		newpar[0] = x_("editcell");
		newpar[1] = describenodeproto(el_curlib->curnodeproto);
		telltool(us_tool, 2, newpar);

		/* must reset the library-changed bit now */
		el_curlib->curnodeproto->lib->userbits &= ~(LIBCHANGEDMAJOR | LIBCHANGEDMINOR);
	}

	/* now end the batch to force these changes to be processed properly */
	us_endbatch();
	us_optionschanged = FALSE;				/* options are unchanged */
	return(FALSE);
}

/* terminate the user interface tool */
void us_done(void)
{
	REGISTER WINDOWPART *w;
#ifdef DEBUGMEMORY
	REGISTER INTBIG i;
#endif

	/* close messages audit window */
	if (us_termaudit != 0) xclose(us_termaudit);

	/* delete all windows */
	if (us_menuframe != NOWINDOWFRAME)
		killwindowframe(us_menuframe);
	while (el_topwindowpart != NOWINDOWPART)
	{
		w = el_topwindowpart;
		el_topwindowpart = el_topwindowpart->nextwindowpart;
		killwindowpart(w);
	}

	logfinishrecord();
	termgraphics();

	/* close trace */
	if (us_tracefile != NULL)
	{
		xclose(us_tracefile);
		us_tracefile = NULL;
	}

#ifdef DEBUGMEMORY
	/* free popups */
	while (us_firstpopupmenu != NOPOPUPMENU)
	{
		REGISTER POPUPMENU *pm;
		REGISTER POPUPMENUITEM *mi;

		pm = us_firstpopupmenu;
		us_firstpopupmenu = us_firstpopupmenu->nextpopupmenu;
		efree((CHAR *)pm->name);
		efree((CHAR *)pm->header);
		efree((CHAR *)pm->list);
		for(i=0; i<pm->total; i++)
		{
			mi = &pm->list[i];
			efree((CHAR *)mi->attribute);
			if (mi->value != 0) efree((CHAR *)mi->value);
			us_freeusercom(mi->response);
		}
		efree((CHAR *)pm);
	}
	noundoallowed();
	us_freeparsememory();
	us_freehighmemory();
	us_freemiscmemory();
	us_freecomekmemory();
	us_freecomtvmemory();
	us_freediacommemory();
	us_freenetmemory();
	us_freeedtecpmemory();
	us_freeedpacmemory();
	us_freeedemacsmemory();
	us_freetranslatememory();

	/* free user commands */
	while (us_usercomfree != NOUSERCOM)
	{
		REGISTER USERCOM *uc;

		uc = us_usercomfree;
		us_usercomfree = us_usercomfree->nextcom;
		efree((CHAR *)uc);
	}

	/* free remembered command */
	for(i=0; i<us_lastcommandtotal; i++)
	{
		if (us_lastcommandpar[i] != 0)
			efree((CHAR *)us_lastcommandpar[i]);
	}
	efree((CHAR *)us_lastcommandpar);

	/* free miscellaneous memory */
	us_freestatusmemory();
	us_freewindowmemory();

	/* free quick-key settings */
	{
		REGISTER INTBIG i;

		for(i=0; i<us_quickkeyfactcount; i++)
			efree((CHAR *)us_quickkeyfactlist[i]);
	}
	if (us_quickkeyfactcount > 0) efree((CHAR *)us_quickkeyfactlist);

	/* free pulldown bar */
	if (us_pulldownmenucount != 0)
	{
		efree((CHAR *)us_pulldowns);
		efree((CHAR *)us_pulldownmenupos);
	}
	us_pulldownmenucount = 0;
#endif
}

/* set the nature of the user interface tool */
void us_set(INTBIG count, CHAR *par[])
{
	REGISTER USERCOM *com;
	REGISTER INTBIG i;
	REGISTER void *infstr;

	if (count == 0) return;

	infstr = initinfstr();
	for(i=0; i<count; i++)
	{
		addstringtoinfstr(infstr, par[i]);
		addtoinfstr(infstr, ' ');
	}
	com = us_makecommand(returninfstr(infstr));
	us_execute(com, FALSE, TRUE, FALSE);
	us_freeusercom(com);
}

/*
 * make requests of the user interface tool:
 *
 ***************** CONTROL OF HIGHLIGHTING ****************
 *  "show-object" TAKES: the GEOM module to highlight
 *  "show-port" TAKES: the GEOM module and PORTPROTO to highlight
 *  "show-area" TAKES: the lowx, highx, lowy, highy, cell of an area
 *  "show-line" TAKES: the x1, y1, x2, y2, cell of a line
 *  "show-multiple" TAKES: string of GEOM addresses separated by tabs
 *  "clear" clears all highlighting
 *  "down-stack" saves the currently highlighted objects on a stack
 *  "up-stack" restores the stacked highlighted objects
 *
 ***************** INFORMATION ABOUT HIGHLIGHTED OBJECTS ****************
 *  "get-node" RETURNS: the currently highlighted NODEINST
 *  "get-port" RETURNS: the currently highlighted PORTPROTO
 *  "get-arc" RETURNS: the currently highlighted ARCINST
 *  "get-object" RETURNS: the currently highlighted GEOM
 *  "get-all-nodes" RETURNS: a list of GEOMs
 *  "get-all-arcs" RETURNS: a list of GEOMs
 *  "get-all-objects" RETURNS: a list of GEOMs
 *  "get-highlighted-area" TAKES: reference lx, hx, ly, hy bounds RETGURNS: cell
 *
 ***************** WINDOW CONTROL ****************
 *  "window-new" RETURNS: newly created window
 *  "window-horiz-new" RETURNS: newly created window (prefers horizontal split)
 *  "window-vert-new" RETURNS: newly created window (prefers vertical split)
 *  "display-to-routine" TAKES: routine to call with polygons
 *  "display-highlighted-to-routine" TAKES: routine to call with polygons
 *  "flush-changes" forces display changes to be visible
 *
 ***************** EDITOR CONTROL ****************
 *  "edit-starteditor" TAKES: window, header, &chars, &lines RETURNS: zero: ok
 *  "edit-totallines" TAKES: window RETURNS: number of lines of text
 *  "edit-getline" TAKES: window and line index RETURNS: string
 *  "edit-addline" TAKES: window and line index and string to insert
 *  "edit-replaceline" TAKES: window and line index and new string
 *  "edit-deleteline" TAKES: window and line index to delete
 *  "edit-highlightline" TAKES: window and line index to highlight
 *  "edit-suspendgraphics" TAKES: window in which to suspends display changes
 *  "edit-resumegraphics" TAKES: window in which to resumes display of changes
 *  "edit-describe" RETURNS: name of editor
 *  "edit-readfile" TAKES: window and file name
 *  "edit-writefile" TAKES: window and file name
 *
 ***************** MISCELLANEOUS ****************
 *  "make-icon" TAKES: portlist, icon name, cell name, library RETURNS: cell
 */
INTBIG us_request(CHAR *command, va_list ap)
{
	HIGHLIGHT newhigh;
	REGISTER GEOM *from;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER void (*curdisplay)(POLYGON*, WINDOWPART*);
	REGISTER INTBIG len, i, arg1, arg2, arg3, arg4, arg5;
	REGISTER GEOM **list;

	if (namesamen(command, x_("get-"), 4) == 0)
	{
		if (namesame(&command[4], x_("node")) == 0)
			return((INTBIG)us_getobject(VNODEINST, FALSE));
		if (namesame(&command[4], x_("arc")) == 0)
			return((INTBIG)us_getobject(VARCINST, FALSE));
		if (namesame(&command[4], x_("object")) == 0)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (var == NOVARIABLE) return(-1);
			len = getlength(var);
			if (len > 1) return(-1);
			(void)us_makehighlight(((CHAR **)var->addr)[0], &newhigh);
			if ((newhigh.status&HIGHTYPE) != HIGHFROM) return(-1);
			return((INTBIG)newhigh.fromgeom);
		}
		if (namesame(&command[4], x_("port")) == 0)
		{
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
			if (var == NOVARIABLE) return(-1);
			len = getlength(var);
			if (len > 1) return(-1);
			(void)us_makehighlight(((CHAR **)var->addr)[0], &newhigh);
			if ((newhigh.status&HIGHTYPE) != HIGHFROM || !newhigh.fromgeom->entryisnode)
				return(-1);
			return((INTBIG)newhigh.fromport);
		}
		if (namesame(&command[4], x_("all-nodes")) == 0)
			return((INTBIG)us_gethighlighted(WANTNODEINST, 0, 0));
		if (namesame(&command[4], x_("all-arcs")) == 0)
			return((INTBIG)us_gethighlighted(WANTARCINST, 0, 0));
		if (namesame(&command[4], x_("all-objects")) == 0)
			return((INTBIG)us_gethighlighted(WANTNODEINST|WANTARCINST, 0, 0));
		if (namesame(&command[4], x_("highlighted-area")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			arg3 = va_arg(ap, INTBIG);
			arg4 = va_arg(ap, INTBIG);
			return((INTBIG)us_getareabounds((INTBIG *)arg1, (INTBIG *)arg2, (INTBIG *)arg3, (INTBIG *)arg4));
		}
	}

	if (namesamen(command, x_("edit-"), 5) == 0)
	{
		if (namesame(&command[5], x_("getline")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			return((INTBIG)us_getline((WINDOWPART *)arg1, arg2));
		}
		if (namesame(&command[5], x_("totallines")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			return(us_totallines((WINDOWPART *)arg1));
		}
		if (namesame(&command[5], x_("addline")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			arg3 = va_arg(ap, INTBIG);
			us_addline((WINDOWPART *)arg1, arg2, (CHAR *)arg3);
			return(0);
		}
		if (namesame(&command[5], x_("replaceline")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			arg3 = va_arg(ap, INTBIG);
			us_replaceline((WINDOWPART *)arg1, arg2, (CHAR *)arg3);
			return(0);
		}
		if (namesame(&command[5], x_("deleteline")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			us_deleteline((WINDOWPART *)arg1, arg2);
			return(0);
		}
		if (namesame(&command[5], x_("highlightline")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			us_highlightline((WINDOWPART *)arg1, arg2, arg2);
			return(0);
		}
		if (namesame(&command[5], x_("starteditor")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			arg3 = va_arg(ap, INTBIG);
			arg4 = va_arg(ap, INTBIG);
			if (us_makeeditor((WINDOWPART *)arg1, (CHAR *)arg2, (INTBIG *)arg3,
				(INTBIG *)arg4) == NOWINDOWPART) return(-1);
			return(0);
		}
		if (namesame(&command[5], x_("suspendgraphics")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			us_suspendgraphics((WINDOWPART *)arg1);
			return(0);
		}
		if (namesame(&command[5], x_("resumegraphics")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			us_resumegraphics((WINDOWPART *)arg1);
			return(0);
		}
		if (namesame(&command[5], x_("describe")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			us_describeeditor((CHAR **)arg1);
			return(0);
		}
		if (namesame(&command[5], x_("readfile")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			us_readtextfile((WINDOWPART *)arg1, (CHAR *)arg2);
			return(0);
		}
		if (namesame(&command[5], x_("writefile")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			us_writetextfile((WINDOWPART *)arg1, (CHAR *)arg2);
			return(0);
		}
	}

	if (namesamen(command, x_("show-"), 5) == 0)
	{
		if (namesame(&command[5], x_("object")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);

			/* fill in the highlight information */
			from = (GEOM *)arg1;
			us_ensurewindow(geomparent(from));
			newhigh.status = HIGHFROM;
			newhigh.cell = geomparent(from);
			newhigh.fromgeom = from;
			newhigh.fromport = NOPORTPROTO;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
			us_showallhighlight();
			return(0);
		}
		if (namesame(&command[5], x_("port")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);

			/* fill in the highlight information */
			from = (GEOM *)arg1;
			pp = (PORTPROTO *)arg2;
			us_ensurewindow(geomparent(from));
			newhigh.status = HIGHFROM;
			newhigh.cell = geomparent(from);
			newhigh.fromgeom = from;
			newhigh.fromport = pp;
			newhigh.frompoint = 0;
			us_addhighlight(&newhigh);
			us_showallhighlight();
			return(0);
		}
		if (namesame(&command[5], x_("line")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			arg3 = va_arg(ap, INTBIG);
			arg4 = va_arg(ap, INTBIG);
			arg5 = va_arg(ap, INTBIG);

			np = (NODEPROTO *)arg5;
			if (np == NONODEPROTO) return(0);

			/* fill in the highlight information */
			us_ensurewindow(np);
			newhigh.status = HIGHLINE;
			newhigh.cell = np;
			newhigh.stalx = arg1;
			newhigh.staly = arg2;
			newhigh.stahx = arg3;
			newhigh.stahy = arg4;
			us_addhighlight(&newhigh);
			us_showallhighlight();
			return(0);
		}
		if (namesame(&command[5], x_("area")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);
			arg2 = va_arg(ap, INTBIG);
			arg3 = va_arg(ap, INTBIG);
			arg4 = va_arg(ap, INTBIG);
			arg5 = va_arg(ap, INTBIG);

			np = (NODEPROTO *)arg5;
			if (np == NONODEPROTO) return(0);

			/* fill in the highlight information */
			us_ensurewindow(np);
			newhigh.status = HIGHBBOX;
			newhigh.cell = np;
			newhigh.stalx = arg1;
			newhigh.stahx = arg2;
			newhigh.staly = arg3;
			newhigh.stahy = arg4;
			us_addhighlight(&newhigh);
			us_showallhighlight();
			return(0);
		}
		if (namesame(&command[5], x_("multiple")) == 0)
		{
			/* get the arguments */
			arg1 = va_arg(ap, INTBIG);

			/* fill in the highlight information */
			us_setmultiplehighlight((CHAR *)arg1, FALSE);
			us_showallhighlight();
			return(0);
		}
	}

	if (namesame(command, x_("flush-changes")) == 0)
	{
		us_endchanges(NOWINDOWPART);
		us_showallhighlight();
		us_beginchanges();
		return(0);
	}

	if (namesame(command, x_("display-to-routine")) == 0)
	{
		if (el_curwindowpart == NOWINDOWPART) return(0);

		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		/* code cannot be called by multiple procesors: uses globals */
		NOT_REENTRANT;

		curdisplay = us_displayroutine;
		us_displayroutine = (void (*)(POLYGON*, WINDOWPART*))arg1;
		us_redisplaynow(el_curwindowpart, FALSE);
		us_endchanges(el_curwindowpart);
		us_displayroutine = curdisplay;
		return(0);
	}

	if (namesame(command, x_("display-highlighted-to-routine")) == 0)
	{
		if (el_curwindowpart == NOWINDOWPART) return(0);

		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		/* code cannot be called by multiple procesors: uses globals */
		NOT_REENTRANT;

		curdisplay = us_displayroutine;
		us_displayroutine = (void (*)(POLYGON*, WINDOWPART*))arg1;

		/* get the objects to be displayed */
		list = us_gethighlighted(WANTNODEINST | WANTARCINST, 0, 0);
		begintraversehierarchy();
		for(i=0; list[i] != NOGEOM; i++)
		{
			if (list[i]->entryisnode)
			{
				if (us_drawcell(list[i]->entryaddr.ni, LAYERA,
					el_matid, 3, el_curwindowpart) < 0) break;
			} else
			{
				if (us_drawarcinst(list[i]->entryaddr.ai, LAYERA,
					el_matid, 3, el_curwindowpart) < 0) break;
			}
		}
		us_displayroutine = curdisplay;
		endtraversehierarchy();
		return(0);
	}

	if (namesame(command, x_("make-icon")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);
		arg3 = va_arg(ap, INTBIG);
		arg4 = va_arg(ap, INTBIG);

		return((INTBIG)us_makeiconcell((PORTPROTO *)arg1, (CHAR *)arg2,
			(CHAR *)arg3, (LIBRARY *)arg4));
	}

	if (namesamen(command, x_("window-"), 7) == 0)
	{
		/* get a new window */
		if (namesame(&command[7], x_("new")) == 0)
		{
			return((INTBIG)us_wantnewwindow(0));
		}
		if (namesame(&command[7], x_("horiz-new")) == 0)
		{
			if (us_needwindow()) return(0);
			return((INTBIG)us_wantnewwindow(1));
		}
		if (namesame(&command[7], x_("vert-new")) == 0)
		{
			if (us_needwindow()) return(0);
			return((INTBIG)us_wantnewwindow(2));
		}
		return(0);
	}

	if (namesame(command, x_("down-stack")) == 0)
	{
		us_pushhighlight();
		return(0);
	}

	if (namesame(command, x_("up-stack")) == 0)
	{
		us_pophighlight(FALSE);
		return(0);
	}

	if (namesame(command, x_("clear")) == 0)
	{
		us_clearhighlightcount();
		return(0);
	}
	return(-1);
}

/* examine an entire cell */
void us_examinenodeproto(NODEPROTO *np) { Q_UNUSED( np ); }

/* one time slice for the user tool: process a command */
void us_slice(void)
{
#ifndef USEQT
	INTBIG special, but, x, y;
	INTSML cmd;
#endif
	CHAR *args[4];

	if (us_desiredlibrary[0] != 0)
	{
		args[0] = x_("read");
		args[1] = us_desiredlibrary;
		args[2] = x_("make-current");
		us_library(3, args);
		us_desiredlibrary[0] = 0;
	}

	/* reset the cursor if it was waiting */
	if (us_cursorstate == WAITCURSOR)
		setdefaultcursortype(us_normalcursor);

	/* switch to the right technology */
	if ((us_state&CURCELLCHANGED) != 0)
	{
		us_state &= ~CURCELLCHANGED;
		if (el_curwindowpart != NOWINDOWPART)
		{
			switch (el_curwindowpart->state&WINDOWTYPE)
			{
				case DISPWINDOW:
				case DISP3DWINDOW:
					us_ensurepropertechnology(el_curwindowpart->curnodeproto, 0, FALSE);
					break;
			}
			us_setcellsize(el_curwindowpart);
			return;
		}
	}

	/* set all cell message changes detected during the last broadcast */
	us_doubchanges();

	/* handle language loops */
	if ((us_state&LANGLOOP) != 0)
	{
		if (languageconverse(0))
		{
			us_state &= ~LANGLOOP;
			ttyputmsg(_("Back to Electric"));
		}

		setactivity(_("Language Interpreter"));

		/* set all cell message changes detected during the last broadcast */
		us_doubchanges();
		return;
	}

	/* start logging now if requested */
	if (us_logging)
	{
		logstartrecord();
		us_logging = FALSE;
	}

#ifndef USEQT
	/* first see if there is any pending input */
	el_pleasestop = 0;
	if (ttydataready())
	{
		el_pleasestop = 0;
		stoptablet();
		cmd = ttygetchar(&special);
		us_oncommand(cmd, special);
		return;
	}
	if (el_pleasestop != 0) return;

	/* get a button */
	waitforbutton(&x, &y, &but);
	if (el_pleasestop != 0) return;
	if (but < 0) return;

	/* execute the button */
	us_ontablet(x, y, but);
#endif
}

/*
 * handle button pushes
 */
void us_ontablet(INTBIG x, INTBIG y, INTBIG but)
{
	REGISTER USERCOM *item;
	REGISTER CHAR *str;
	CHAR *par[2];
	INTBIG lx, hx, ly, hy;
	BOOLEAN verbose, drawexplorericon;
	REGISTER VARIABLE *var;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER WINDOWPART *w;
	REGISTER WINDOWFRAME *curframe, *wf;
	COMMANDBINDING commandbinding;

	/* reset single-key suspension flag */
	if ((us_state&SKIPKEYS) != 0)
	{
		ttyputmsg(_("Single-key command suspension now lifted"));
		us_state &= ~SKIPKEYS;
	}

	/* wheel rolls get handled immediately */
	if (wheelbutton(but))
	{
		if (el_curwindowpart->buttonhandler != 0)
		{
			(*el_curwindowpart->buttonhandler)(el_curwindowpart, but, x, y);
		}
		return;
	}

	/* first see if this is a slide to a window partition divider */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		/* see if the cursor is over a window partition separator */
		us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
		if (x >= lx-1 && x <= lx+1 && y > ly+1 && y < hy-1 &&
			us_hasotherwindowpart(lx-10, y, w))
		{
			us_vpartdividerbegin(x, ly, hy, w->frame);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_vpartdividerdown, us_nullchar,
				us_vpartdividerdone, TRACKNORMAL);
			return;
		} else if (x >= hx-1 && x <= hx+1 && y > ly+1 && y < hy-1 &&
			us_hasotherwindowpart(hx+10, y, w))
		{
			us_vpartdividerbegin(x, ly, hy, w->frame);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_vpartdividerdown, us_nullchar,
				us_vpartdividerdone, TRACKNORMAL);
			return;
		} else if (y >= ly-1 && y <= ly+1 && x > lx+1 && x < hx-1 &&
			us_hasotherwindowpart(x, ly-10, w))
		{
			us_hpartdividerbegin(y, lx, hx, w->frame);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_hpartdividerdown, us_nullchar,
				us_hpartdividerdone, TRACKNORMAL);
			return;
		} else if (y >= hy-1 && y <= hy+1 && x > lx+1 && x < hx-1 &&
			us_hasotherwindowpart(x, hy+10, w))
		{
			us_hpartdividerbegin(y, lx, hx, w->frame);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_hpartdividerdown, us_nullchar,
				us_hpartdividerdone, TRACKNORMAL);
			return;
		}
	}

	/* switch windows to the current frame */
	curframe = getwindowframe(TRUE);
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (w->frame != curframe) continue;
		if (x >= w->uselx && x <= w->usehx && y >= w->usely && y <= w->usehy)
		{
			/* make this window the current one */
			if (w != el_curwindowpart)
			{
				(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
					VWINDOWPART|VDONTSAVE);
				if (w->curnodeproto == NONODEPROTO) lib = el_curlib; else
					lib = w->curnodeproto->lib;
				(void)setval((INTBIG)lib, VLIBRARY, x_("curnodeproto"),
					(INTBIG)w->curnodeproto, VNODEPROTO);
			}
			break;
		}
	}

	/* see if it is a fixed menu hit */
	if ((us_tool->toolstate&MENUON) != 0 &&
		(us_menuframe == NOWINDOWFRAME || curframe == us_menuframe) &&
		y >= us_menuly && y <= us_menuhy && x >= us_menulx && x <= us_menuhx)
	{
		x = (x-us_menulx) / us_menuxsz;
		y = (y-us_menuly) / us_menuysz;
		if (x < 0 || y < 0 || x >= us_menux || y >= us_menuy) return;
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
		if (var == NOVARIABLE) return;
		if (us_menupos <= 1) str = ((CHAR **)var->addr)[y * us_menux + x]; else
			str = ((CHAR **)var->addr)[x * us_menuy + y];
		us_parsebinding(str, &commandbinding);
		item = us_makecommand(commandbinding.command);
		us_freebindingparse(&commandbinding);
		if (item == NOUSERCOM || item->active < 0)
		{
			ttyputerr(_("No command is attached to this menu entry"));
			if (item != NOUSERCOM) us_freeusercom(item);
			return;
		}
		if ((us_tool->toolstate&ONESHOTMENU) != 0)
		{
			us_menuhnx = x;   us_menuhny = y;
			us_highlightmenu(x, y, el_colhmenbor);
		}
		us_state &= ~GOTXY;
		us_forceeditchanges();
		if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
			verbose = FALSE;
		us_execute(item, verbose, TRUE, TRUE);
		us_causeofslice(item);
		if ((us_tool->toolstate&ONESHOTMENU) != 0)
		{
			us_highlightmenu(us_menuhnx, us_menuhny, el_colmenbor);
			us_menuhnx = -1;
		}
		us_freeusercom(item);
		flushscreen();
	} else
	{
		/* get cursor position and check windows */
		if (us_setxy(x, y))
		{
			/* see if it is a slider hit in a display window */
			wf = getwindowframe(TRUE);
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if (w->frame != wf) continue;
				if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
				{
					if (x >= w->uselx-DISPLAYSLIDERSIZE && x < w->usehx &&
						y >= w->usely-DISPLAYSLIDERSIZE && y < w->usehy)
					{
						if (w->buttonhandler != 0)
							(*w->buttonhandler)(w, but, x, y);
						return;
					}
				}
				if ((w->state&WINDOWTYPE) == DISPWINDOW)
				{
					if (x > w->usehx && x <= w->usehx+DISPLAYSLIDERSIZE &&
						y > w->usely && y < w->usehy)
					{
						/* check for vertical slider hit */
						np = w->curnodeproto;
						if (w != el_curwindowpart)
						{
							(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
								VWINDOWPART|VDONTSAVE);
							(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"),
								(INTBIG)np, VNODEPROTO);
						}
						if (np == NONODEPROTO) return;
						if (np->highy == np->lowy) return;
						if (y <= w->usely+DISPLAYSLIDERSIZE)
						{
							/* small shift up (may repeat) */
							us_arrowclickbegin(w, w->usehx, w->usehx+DISPLAYSLIDERSIZE,
								w->usely, w->usely+DISPLAYSLIDERSIZE,
								(w->screenhy - w->screenly) / DISPLAYSMALLSHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_varrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
						if (y <= w->thumbly)
						{
							/* large shift up (may repeat) */
							us_arrowclickbegin(w, w->usehx, w->usehx+DISPLAYSLIDERSIZE,
								w->usely+DISPLAYSLIDERSIZE, w->thumbhy,
								(w->screenhy - w->screenly) / DISPLAYLARGESHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_varrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
						if (y <= w->thumbhy)
						{
							/* on thumb (track thumb motion) */
							us_vthumbbegin(y, w, w->usehx, w->usely, w->usehy, FALSE,
								us_vthumbtrackingcallback);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_vthumbdown, us_nullchar,
								us_vthumbdone, TRACKNORMAL);
							return;
						}
						if (y <= w->usehy-DISPLAYSLIDERSIZE)
						{
							/* large shift down (may repeat) */
							us_arrowclickbegin(w, w->usehx, w->usehx+DISPLAYSLIDERSIZE,
								w->thumbhy, w->usehy-DISPLAYSLIDERSIZE,
								-(w->screenhy - w->screenly) / DISPLAYLARGESHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_varrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
						if (y <= w->usehy)
						{
							/* small shift down (may repeat) */
							us_arrowclickbegin(w, w->usehx, w->usehx+DISPLAYSLIDERSIZE,
								w->usehy-DISPLAYSLIDERSIZE, w->usehy,
								-(w->screenhy - w->screenly) / DISPLAYSMALLSHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_varrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
					}
					if (y < w->usely && y >= w->usely-DISPLAYSLIDERSIZE &&
						x > w->uselx && x < w->usehx)
					{
						/* see if the explorer button was hit */
						drawexplorericon = us_windowgetsexploericon(w);
						lx = w->uselx;
						if (drawexplorericon)
						{
							lx += DISPLAYSLIDERSIZE;
							if (x <= lx)
							{
								par[0] = "explore";
								us_window(1, par);
								return;
							}
						}

						/* check for horizontal slider hit */
						np = w->curnodeproto;
						if (w != el_curwindowpart)
						{
							(void)setvalkey((INTBIG)us_tool, VTOOL, us_current_window_key, (INTBIG)w,
								VWINDOWPART|VDONTSAVE);
							(void)setval((INTBIG)el_curlib, VLIBRARY, x_("curnodeproto"),
								(INTBIG)np, VNODEPROTO);
						}
						if (np == NONODEPROTO) return;
						if (np->highx == np->lowx) return;
						if (x <= lx+DISPLAYSLIDERSIZE)
						{
							/* small shift left (may repeat) */
							us_arrowclickbegin(w, lx, lx+DISPLAYSLIDERSIZE,
								w->usely-DISPLAYSLIDERSIZE, w->usely,
								-(w->screenhx - w->screenlx) / DISPLAYSMALLSHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_harrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
						if (x <= w->thumblx)
						{
							/* large shift left (may repeat) */
							us_arrowclickbegin(w, lx+DISPLAYSLIDERSIZE, w->thumblx,
								w->usely-DISPLAYSLIDERSIZE, w->usely,
								-(w->screenhx - w->screenlx) / DISPLAYLARGESHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_harrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
						if (x <= w->thumbhx)
						{
							/* on thumb (track thumb motion) */
							us_hthumbbegin(x, w, w->usely, lx, w->usehx, us_hthumbtrackingcallback);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_hthumbdown, us_nullchar,
								us_hthumbdone, TRACKNORMAL);
							return;
						}
						if (x <= w->usehx-DISPLAYSLIDERSIZE)
						{
							/* large shift right (may repeat) */
							us_arrowclickbegin(w, w->thumbhx, w->usehx-DISPLAYSLIDERSIZE,
								w->usely-DISPLAYSLIDERSIZE, w->usely,
								(w->screenhx - w->screenlx) / DISPLAYLARGESHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_harrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
						if (x <= w->usehx)
						{
							/* small shift right (may repeat) */
							us_arrowclickbegin(w, w->usehx-DISPLAYSLIDERSIZE, w->usehx,
								w->usely-DISPLAYSLIDERSIZE, w->usely,
								(w->screenhx - w->screenlx) / DISPLAYSMALLSHIFT);
							trackcursor(FALSE, us_nullup, us_nullvoid, us_harrowdown, us_nullchar,
								us_nullvoid, TRACKNORMAL);
							return;
						}
					}
				}
			}
			ttyputerr(_("Cursor off the screen"));
			return;
		}
		if (el_curwindowpart != NOWINDOWPART)
		{
			/* if in distance-measurement mode, track measurement */
			if ((us_state&MEASURINGDISTANCE) != 0)
			{
				if ((el_curwindowpart->state&WINDOWTYPE) == DISPWINDOW &&
					el_curwindowpart->curnodeproto != NONODEPROTO)
				{
					if ((us_state&MEASURINGDISTANCEINI) != 0)
					{
						us_state &= ~MEASURINGDISTANCEINI;
						us_distanceinit();
					}
					trackcursor(FALSE, us_ignoreup, us_nullvoid, us_distancedown,
						us_stoponchar, us_distanceup, TRACKDRAGGING);
					return;
				}
			}

			/* handle normal button click in this window */
			if (el_curwindowpart->buttonhandler != 0)
			{
				(*el_curwindowpart->buttonhandler)(el_curwindowpart, but, x, y);
			}
		}
	}
}

/*
 * Routine to return true if there is another window partion on the same frame as
 * "ow" that covers the coordinates (x,y).
 */
BOOLEAN us_hasotherwindowpart(INTBIG x, INTBIG y, WINDOWPART *ow)
{
	REGISTER WINDOWPART *w;
	INTBIG lx, hx, ly, hy;

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (w == ow) continue;
		if (w->frame != ow->frame) continue;
		us_gettruewindowbounds(w, &lx, &hx, &ly, &hy);
		if (x > lx && x < hx && y > ly && y < hy) return(TRUE);
	}
	return(FALSE);
}

/*
 * default button handler for buttons pushed in normal windows
 */
void us_buttonhandler(WINDOWPART *w, INTBIG but, INTBIG x, INTBIG y)
{
	REGISTER USERCOM *item;
	REGISTER VARIABLE *var;
	BOOLEAN verbose;
	COMMANDBINDING commandbinding;
	INTBIG unimportant;
	Q_UNUSED( w );
	Q_UNUSED( x );
	Q_UNUSED( y );

	/* get the command attached to the button */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_buttons_key);
	if (var == NOVARIABLE) return;
	us_parsebinding(((CHAR **)var->addr)[but], &commandbinding);
	item = us_makecommand(commandbinding.command);
	us_freebindingparse(&commandbinding);
	if (item == NOUSERCOM || item->active < 0)
	{
		ttyputerr(_("No command is attached to the %s button"),
			buttonname(but, &unimportant));
		if (item != NOUSERCOM) us_freeusercom(item);
		return;
	}
	us_forceeditchanges();
	if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
		verbose = FALSE;
	us_execute(item, verbose, TRUE, TRUE);
	us_causeofslice(item);
	us_freeusercom(item);
	flushscreen();
}

/* routine that is called when keyboard input is ready */
void us_oncommand(INTSML cmd, INTBIG special)
{
	INTBIG x, y;

	/* Read and execute the command */
	us_state &= ~GOTXY;
	(void)getxy(&x, &y);

	if (el_curwindowpart != NOWINDOWPART && el_curwindowpart->charhandler != 0)
	{
		if ((*el_curwindowpart->charhandler)(el_curwindowpart, cmd, special))
			us_killcurrentwindow(TRUE);
	} else
	{
		(void)(*DEFAULTCHARHANDLER)(el_curwindowpart, cmd, special);
	}
}

/* EMACS warning */
static DIALOGITEM us_emawarndialogitems[] =
{
 /*  1 */ {0, {160,116,184,196}, BUTTON, N_("OK")},
 /*  2 */ {0, {4,4,20,340}, MESSAGE, N_("You have just typed Control-X")},
 /*  3 */ {0, {24,4,40,340}, MESSAGE, N_("Followed quickly by another control character.")},
 /*  4 */ {0, {52,4,68,340}, MESSAGE, N_("Could it be that you think this is EMACS?")},
 /*  5 */ {0, {84,4,100,340}, MESSAGE, N_("The second control character has been ignored.")},
 /*  6 */ {0, {104,4,120,340}, MESSAGE, N_("If you wish to disable this check, click below.")},
 /*  7 */ {0, {128,32,144,284}, BUTTON, N_("Disable EMACS character check")}
};
static DIALOG us_emawarndialog = {{75,75,268,425}, N_("This is Not EMACS"), 0, 7, us_emawarndialogitems, 0, 0};

/* special items for the "EMACS warning" dialog: */
#define DEMW_DISABLECHECK     7		/* Disable EMACS check (button) */

/*
 * default character handler for keys typed in normal windows
 */
BOOLEAN us_charhandler(WINDOWPART *w, INTSML cmd, INTBIG special)
{
	REGISTER INTBIG i, longintro, bits, itemHit;
	INTBIG count;
	BOOLEAN verbose;
	REGISTER void *dia;
	static CHAR prompt[] = {x_("-")};
	REGISTER USERCOM *uc;
	CHAR *paramstart[MAXPARS], *pt;
	static UINTBIG timeoflastctlx = 0;
	static BOOLEAN doemacscheck = TRUE;
	REGISTER UINTBIG thiseventtime;
	extern COMCOMP us_userp;
	COMMANDBINDING commandbinding;
	Q_UNUSED( w );

	/* special case: warn about EMACS-like commands */
	thiseventtime = eventtime();
	bits = getbuckybits();
	if (thiseventtime - timeoflastctlx < 60)
	{
		/* another character typed less than a second after a ^X */
		if ((bits&CONTROLDOWN) != 0)
		{
			/* a control character!  warn that this is not EMACS */
			if (doemacscheck)
			{
				dia = DiaInitDialog(&us_emawarndialog);
				if (dia == 0) return(FALSE);
				for(;;)
				{
					itemHit = DiaNextHit(dia);
					if (itemHit == OK) break;
					if (itemHit == DEMW_DISABLECHECK) doemacscheck = FALSE;
				}
				DiaDoneDialog(dia);
				return(FALSE);
			}
		}
	}
	if (cmd == 'x' && (bits&CONTROLDOWN) != 0)
	{
		timeoflastctlx = thiseventtime;
	}

	/* get the command attached to the key */
	i = us_findboundkey(cmd, special, &pt);
	if (i < 0) pt = x_("");
	us_parsebinding(pt, &commandbinding);

	/* see if the key introduces a long command */
	longintro = 0;
	if (namesame(commandbinding.command, x_("telltool user")) == 0) longintro = 1;

	/* if single-key commands were suspended, wait for carriage-return */
	if ((us_state&SKIPKEYS) != 0)
	{
		if ((cmd&127) == '\r' || (cmd&127) == '\n' || longintro != 0)
		{
			ttyputmsg(_("Single-key command suspension now lifted"));
			us_state &= ~SKIPKEYS;
		}
		if (longintro == 0)
		{
			us_freebindingparse(&commandbinding);
			return(FALSE);
		}
	}

	/* if this is a "telltool user" character, fill out the command */
	if (longintro != 0)
	{
		us_freebindingparse(&commandbinding);

		/* get the full command */
		prompt[0] = (CHAR)cmd;
		count = ttygetfullparam(prompt, &us_userp, MAXPARS, paramstart);
		if (count <= 0) return(FALSE);

		/* parse into fields and execute */
		uc = us_buildcommand(count, paramstart);
		if (uc == NOUSERCOM)
		{
			/* this is very bad!  code should deal with it better!!! */
			ttyputnomemory();
			return(FALSE);
		}
		us_forceeditchanges();
		us_execute(uc, FALSE, TRUE, TRUE);
		us_causeofslice(uc);
		us_freeusercom(uc);
	} else
	{
		/* check for new windows and get set tablet position for command */
		if (*commandbinding.command == 0)
		{
			us_abortcommand(_("The '%s' key has no meaning in this window"),
				us_describeboundkey(cmd, special, 1));
			us_freebindingparse(&commandbinding);
			return(FALSE);
		}
		uc = us_makecommand(commandbinding.command);
		us_freebindingparse(&commandbinding);
		if (uc == NOUSERCOM) return(FALSE);
		if (uc->active >= 0)
		{
			us_forceeditchanges();
			if ((us_tool->toolstate&ECHOBIND) != 0) verbose = TRUE; else
				verbose = FALSE;
			us_execute(uc, verbose, TRUE, TRUE);
			us_causeofslice(uc);
		}
		us_freeusercom(uc);
	}
	flushscreen();
	return(FALSE);
}

/*
 * routine to save the command in "uc" as the one that produced this slice
 */
void us_causeofslice(USERCOM *uc)
{
	REGISTER void *infstr;

	if (uc->active < 0) return;
	infstr = initinfstr();
	addstringtoinfstr(infstr, uc->comname);
	us_appendargs(infstr, uc);
	setactivity(returninfstr(infstr));
}

void us_forceeditchanges(void)
{
	REGISTER WINDOWPART *w;

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if ((w->state&WINDOWTYPE) == TEXTWINDOW) us_shipchanges(w);
}

/******************** CHANGES TO THE DISPLAY ********************/

void us_startbatch(TOOL *source, BOOLEAN undoredo)
{
	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;
	Q_UNUSED( undoredo );

	us_batchsource = source;
	us_firstchangedcell = us_secondchangedcell = NONODEPROTO;
	us_state &= ~HIGHLIGHTSET;
	us_beginchanges();
	us_menuchanged = FALSE;
	us_cellstructurechanged = FALSE;
	us_cellwithkilledname = NONODEPROTO;
}

void us_endbatch(void)
{
	REGISTER WINDOWPART *w;

	/* redraw windows whose in-place context has changed */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&INPLACEQUEUEREDRAW) == 0) continue;
		w->state &= ~INPLACEQUEUEREDRAW;
		if (w->redisphandler != 0) (*w->redisphandler)(w);
	}
	us_endchanges(NOWINDOWPART);
	us_showallhighlight();
	if (us_cellstructurechanged)
		us_redoexplorerwindow();
	if (us_batchsource == us_tool && us_firstchangedcell != NONODEPROTO &&
		us_secondchangedcell != NONODEPROTO)
	{
		ttyputmsg(_("This change affected more than one cell"));
	}
}

/*
 * routine to display any delayed highlighting
 */
void us_showallhighlight(void)
{
	REGISTER INTBIG i, len;
	REGISTER VARIABLE *var;
	HIGHLIGHT high;

	/* stop now if highlighting has not been delayed */
	if ((us_state&HIGHLIGHTSET) == 0) return;

	/* show all highlighting */
	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(i=0; i<len; i++)
		{
			if (us_makehighlight(((CHAR **)var->addr)[i], &high)) continue;
			us_sethighlight(&high, HIGHLIT);
		}
	}

	/* mark that highlighting is not delayed */
	us_state &= ~HIGHLIGHTSET;
}

void us_startobjectchange(INTBIG addr, INTBIG type)
{
	switch (type&VTYPE)
	{
		case VNODEINST:
			us_undisplayobject(((NODEINST *)addr)->geom);   break;
		case VARCINST:
			us_undisplayobject(((ARCINST *)addr)->geom);    break;
		case VWINDOWPART:
			copywindowpart(&us_oldwindow, (WINDOWPART *)addr);  break;
		case VTOOL:
			if ((TOOL *)addr == us_tool) us_maplow = -1;
			us_oldstate = us_tool->toolstate;
			us_oldoptions = us_useroptions;
			us_menuchanged = FALSE;
			us_newwindowcount = 0;
			us_firstnewwindow = us_secondnewwindow = NOWINDOWPART;
			us_killedwindow = NOWINDOWPART;
			us_gridfactorschanged = FALSE;
			break;
	}
}

void us_endobjectchange(INTBIG addr, INTBIG type)
{
	REGISTER WINDOWPART *w, *ow, *w1, *w2, *w3;
	REGISTER INTBIG ulx, uhx, uly, uhy, bits, lambda;
	static POLYGON *poly = NOPOLYGON;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER TECHNOLOGY *tech;
	INTBIG dummy, cenx, ceny;
	REGISTER INTBIG alignment;
	extern GRAPHICS us_egbox, us_gbox;
	REGISTER VARIABLE *varred, *vargreen, *varblue, *var;

	switch (type&VTYPE)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			us_queueredraw(ni->geom, FALSE);
			us_geomhaschanged(ni->geom);
			break;
		case VARCINST:
			ai = (ARCINST *)addr;
			us_queueredraw(ai->geom, FALSE);
			us_geomhaschanged(ai->geom);
			break;
		case VTOOL:
			if ((TOOL *)addr != us_tool) break;

			/* handle changes to the menu */
			if ((us_oldstate&MENUON) != (us_tool->toolstate&MENUON) || us_menuchanged)
			{
				/* destroy or create the menu if requested */
				us_menuchanged = FALSE;
				if (us_menuframe != NOWINDOWFRAME && (us_tool->toolstate&MENUON) == 0)
				{
					killwindowframe(us_menuframe);
					us_menuframe = NOWINDOWFRAME;
					return;
				}
				if (us_menuframe == NOWINDOWFRAME && (us_tool->toolstate&MENUON) != 0)
					us_menuframe = newwindowframe(TRUE, 0);
				us_drawmenu(1, us_menuframe);
				return;
			}

			/* handle changes to the color map */
			if (us_maplow != -1)
			{
				varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
				vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
				varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
				if (varred != NOVARIABLE && vargreen != NOVARIABLE && varblue != NOVARIABLE)
					colormapload(&((INTBIG *)varred->addr)[us_maplow],
						&((INTBIG *)vargreen->addr)[us_maplow], &((INTBIG *)varblue->addr)[us_maplow],
							us_maplow, us_maphigh);
			}

			/* rewrite status information in header if configuration changed */
			if (us_newwindowcount != 0 || us_killedwindow != NOWINDOWPART)
			{
				us_redostatus(NOWINDOWFRAME);
			}

			/* see if windows were created */
			if (us_newwindowcount != 0)
			{
				/* special case for split edit windows */
				if (us_killedwindow != NOWINDOWPART && us_newwindowcount == 2 &&
					(us_firstnewwindow->state&WINDOWTYPE) == DISPWINDOW &&
						(us_secondnewwindow->state&WINDOWTYPE) == DISPWINDOW &&
							us_firstnewwindow->curnodeproto == us_secondnewwindow->curnodeproto &&
								us_firstnewwindow->curnodeproto == us_killedwindow->curnodeproto)
				{
					/* use block transfer */
					w1 = &us_oldwindow;
					w2 = us_firstnewwindow;
					w3 = us_secondnewwindow;

					/* make sure the windows have the same scale */
					if (us_windowcansplit(w1, w2, w3))
					{
						/* make sure that "w3" is the larger window */
						if (w2->usehx - w2->uselx > w3->usehx - w3->uselx ||
							w2->usehy - w2->usely > w3->usehy - w3->usely)
						{
							w = w2;   w2 = w3;   w3 = w;
						}
						screenmovebox(w2, (w1->usehx+w1->uselx - w3->usehx+w3->uselx) / 2,
							(w1->usehy+w1->usely - w3->usehy+w3->usely) / 2,
								w3->usehx-w3->uselx+1, w3->usehy-w3->usely+1,
									w3->uselx, w3->usely);
						if (w2->curnodeproto != NONODEPROTO && w2->curnodeproto == w3->curnodeproto)
							screenmovebox(w2, w3->uselx, w3->usely, w2->usehx-w2->uselx+1,
								w2->usehy-w2->usely+1, w2->uselx, w2->usely);
						us_drawwindow(w2, el_colwinbor);
						us_drawwindow(w3, el_colwinbor);
						break;
					}
				}

				/* simple window creation: draw and outline */
				if (us_newwindowcount > 2)
				{
					/* many windows created: redraw all of them */
					for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					{
						us_drawwindow(w, el_colwinbor);
						if (w->redisphandler != 0) (*w->redisphandler)(w);
					}
				} else
				{
					/* one or two windows created: draw them */
					us_drawwindow(us_firstnewwindow, el_colwinbor);
					if (us_firstnewwindow->redisphandler != 0)
						(*us_firstnewwindow->redisphandler)(us_firstnewwindow);
					if (us_secondnewwindow != NOWINDOWPART)
					{
						us_drawwindow(us_secondnewwindow, el_colwinbor);
						if (us_secondnewwindow->redisphandler != 0)
							(*us_secondnewwindow->redisphandler)(us_secondnewwindow);
					}
				}
				break;
			}

			/* redraw all windows if appropriate options changed */
			bits = DRAWTINYCELLS|PORTLABELS|EXPORTLABELS|HIDETXTNODE|HIDETXTARC|
				HIDETXTPORT|HIDETXTEXPORT|HIDETXTNONLAY|HIDETXTINSTNAME|HIDETXTCELL;
			if ((us_oldoptions&bits) != (us_useroptions&bits))
			{
				for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
					if (w->redisphandler != 0) (*w->redisphandler)(w);
			}
			break;

		case VWINDOWPART:
			w = (WINDOWPART *)addr;
			ow = &us_oldwindow;

			/* adjust window if changing in or out of being a display window */
			if ((w->state&WINDOWTYPE) == DISPWINDOW && (ow->state&WINDOWTYPE) != DISPWINDOW)
			{
				/* became a display window: shrink the bottom and right edge */
				w->usehx -= DISPLAYSLIDERSIZE;
				w->usely += DISPLAYSLIDERSIZE;
				computewindowscale(w);
			}
			if ((w->state&WINDOWTYPE) != DISPWINDOW && (ow->state&WINDOWTYPE) == DISPWINDOW)
			{
				/* no longer a display window: shrink the bottom and right edge */
				w->usehx += DISPLAYSLIDERSIZE;
				w->usely -= DISPLAYSLIDERSIZE;
				computewindowscale(w);
			}

			/* adjust window if changing in or out of being a waveform/tec edit/outline edit window */
			if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW && (ow->state&WINDOWTYPE) != WAVEFORMWINDOW)
			{
				/* became a waveform window: shrink the left edge */
				w->uselx += DISPLAYSLIDERSIZE;
				w->usely += DISPLAYSLIDERSIZE;
				computewindowscale(w);
			}
			if ((w->state&WINDOWTYPE) != WAVEFORMWINDOW && (ow->state&WINDOWTYPE) == WAVEFORMWINDOW)
			{
				/* no longer a waveform window: shrink the bottom and right edge */
				w->uselx -= DISPLAYSLIDERSIZE;
				w->usely -= DISPLAYSLIDERSIZE;
				computewindowscale(w);
			}

			/* set the cursor according to the current window */
			if (el_curwindowpart == NOWINDOWPART) setnormalcursor(NORMALCURSOR); else
			{
				if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
				{
					setnormalcursor(PENCURSOR);
				} else
				{
					if ((el_curwindowpart->state&WINDOWTECEDMODE) != 0)
					{
						setnormalcursor(TECHCURSOR);
					} else
					{
						setnormalcursor(NORMALCURSOR);
					}
				}
			}

			/* switch between 2-D and 3-D drawing */
			if ((ow->state&WINDOWTYPE) == DISPWINDOW && (w->state&WINDOWTYPE) == DISP3DWINDOW)
			{
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}
			if ((ow->state&WINDOWTYPE) == DISP3DWINDOW && (w->state&WINDOWTYPE) == DISPWINDOW)
			{
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}

			/* adjust window if mode changed */
			if ((w->state&WINDOWMODE) != (ow->state&WINDOWMODE))
			{
				us_setwindowmode(w, ow->state, w->state);
				computewindowscale(w);
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}

			/* see if the cell changed */
			if (w->curnodeproto != ow->curnodeproto || w->buttonhandler != ow->buttonhandler ||
				w->charhandler != ow->charhandler || w->termhandler != ow->termhandler ||
					w->redisphandler != ow->redisphandler || w->changehandler != ow->changehandler)
			{
				us_setcellname(w);
				us_setcellsize(w);
				us_setgridsize(w);
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}

			/* see if the window slid vertically */
			if (w->screenlx == ow->screenlx && w->screenhx == ow->screenhx &&
				w->screenly - ow->screenly == w->screenhy - ow->screenhy &&
					w->screenly != ow->screenly)
			{
				/* window slid vertically */
				ulx = w->uselx;       uhx = w->usehx;
				uly = w->usely;       uhy = w->usehy;
				ow->uselx = ulx;      ow->usehx = uhx;
				ow->usely = uly;      ow->usehy = uhy;
				computewindowscale(ow);

				/* bit sliding didn't work, redisplay entire window */
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}

			/* see if the window slid horizontally */
			if (w->screenly == ow->screenly && w->screenhy == ow->screenhy &&
				w->screenlx - ow->screenlx == w->screenhx - ow->screenhx &&
					w->screenlx != ow->screenlx)
			{
				/* window slid horizontally */
				ulx = w->uselx;       uhx = w->usehx;
				uly = w->usely;       uhy = w->usehy;
				ow->uselx = ulx;      ow->usehx = uhx;
				ow->usely = uly;      ow->usehy = uhy;
				computewindowscale(ow);

				/* bit sliding didn't work, redisplay entire window */
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}

			/* redraw waveform window */
			if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW)
			{
				if (w->redisphandler != 0) (*w->redisphandler)(w);
				break;
			}

			/* handle grid going off or changing size while on */
			if (((ow->state&GRIDON) != 0 && (w->state&GRIDON) == 0) ||
				((w->state&GRIDON) != 0 &&
					(ow->gridx != w->gridx || ow->gridy != w->gridy || us_gridfactorschanged)))
			{
				/* must redraw everything on B&W displays or those with less than 8-bits */
				us_setgridsize(w);
				if (el_maplength < 256)
				{
					if (w->redisphandler != 0) (*w->redisphandler)(w);
					break;
				}

				/* erase the grid layer */
				screendrawbox(w, w->uselx, w->usehx, w->usely, w->usehy, &us_egbox);

				/* if grid went off and nothing else changed, all is done */
				if ((w->state&GRIDON) == 0 && w->screenlx == ow->screenlx &&
					w->screenhx == ow->screenhx && w->screenly == ow->screenly &&
						w->screenhy == ow->screenhy) break;
			}

			/* handle grid going on or staying on and changing size */
			if (w->screenlx == ow->screenlx && w->screenhx == ow->screenhx &&
				w->screenly == ow->screenly && w->screenhy == ow->screenhy)
					if (((ow->state & (GRIDTOOSMALL|GRIDON)) != GRIDON &&
						(w->state & (GRIDTOOSMALL|GRIDON)) == GRIDON) ||
							((ow->state&GRIDON) != 0 && (w->state&GRIDON) != 0 &&
								(ow->gridx != w->gridx || ow->gridy != w->gridy || us_gridfactorschanged)))
			{
				np = w->curnodeproto;
				if (np == NONODEPROTO) break;

				/* get polygon */
				(void)needstaticpolygon(&poly, 6, us_tool->cluster);

				/* grid spacing */
				lambda = np->lib->lambda[np->tech->techindex];
				poly->xv[0] = muldiv(w->gridx, lambda, WHOLE);
				poly->yv[0] = muldiv(w->gridy, lambda, WHOLE);

				/* initial grid location */
				var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_gridfloatskey);
				if (var == NOVARIABLE || var->addr == 0)
				{
					poly->xv[1] = w->screenlx / poly->xv[0] * poly->xv[0];
					poly->yv[1] = w->screenly / poly->yv[0] * poly->yv[0];
				} else
				{
					grabpoint(np, &cenx, &ceny);
					tech = np->tech;
					if (tech == NOTECHNOLOGY) tech = el_curtech;
					alignment = muldiv(us_alignment_ratio, el_curlib->lambda[tech->techindex], WHOLE);
					poly->xv[1] = us_alignvalue(cenx, alignment, &dummy);
					poly->yv[1] = us_alignvalue(ceny, alignment, &dummy);
					poly->xv[1] += (w->screenlx-poly->xv[1]) / poly->xv[0] * poly->xv[0];
					poly->yv[1] += (w->screenly-poly->yv[1]) / poly->yv[0] * poly->yv[0];
				}

				/* display screen extent */
				poly->xv[2] = w->uselx;     poly->yv[2] = w->usely;
				poly->xv[3] = w->usehx;     poly->yv[3] = w->usehy;

				/* object space extent */
				poly->xv[4] = w->screenlx;  poly->yv[4] = w->screenly;
				poly->xv[5] = w->screenhx;  poly->yv[5] = w->screenhy;

				poly->count = 6;
				poly->style = GRIDDOTS;
				poly->desc = &us_gbox;
				(*us_displayroutine)(poly, w);
				flushscreen();
				break;
			}

			/* no sliding or grid change: simply redisplay the window */
			if (w->redisphandler != 0) (*w->redisphandler)(w);
			break;
	}
}

void us_modifynodeinst(NODEINST *ni, INTBIG oldlx, INTBIG oldly, INTBIG oldhx, INTBIG oldhy,
	INTBIG oldrot, INTBIG oldtran)
{
	REGISTER INTBIG dlx, dhx, dly, dhy;
	Q_UNUSED( oldrot );
	Q_UNUSED( oldtran );

	/* look for the special "cell-center" and "essential-bounds" nodes in the "generic" technology */
	if (ni->proto == gen_cellcenterprim)
		us_setnodeprotocenter(ni->lowx, ni->lowy, ni->parent); else
			if (ni->proto == gen_essentialprim)
				us_setessentialbounds(ni->parent);

	/* track changes to node that was duplicated */
	if (ni == us_dupnode)
	{
		us_dupx += (ni->lowx + ni->highx) / 2 - (oldlx + oldhx) / 2;
		us_dupy += (ni->lowy + ni->highy) / 2 - (oldly + oldhy) / 2;
	}

	/* if node moved, remember the cell */
	dlx = ni->lowx - oldlx;   dhx = ni->highx - oldhx;
	dly = ni->lowy - oldly;   dhy = ni->highy - oldhy;
	if ((dlx == dhx && dlx != 0) || (dly == dhy && dly != 0))
	{
		if (us_firstchangedcell == NONODEPROTO || us_firstchangedcell == ni->parent)
		{
			us_firstchangedcell = ni->parent;
		} else if (us_secondchangedcell == NONODEPROTO || us_secondchangedcell != ni->parent)
		{
			us_secondchangedcell = ni->parent;
		}
	}

	/* see if this affects in-place editing */
	us_checkinplaceedits(ni->parent);
}

void us_modifynodeproto(NODEPROTO *np)
{
	REGISTER WINDOWPART *w;

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (np != w->curnodeproto) continue;
		us_setcellsize(w);
		us_drawdispwindowsliders(w);
	}

	/* see if this affects in-place editing */
	us_checkinplaceedits(np);
}

void us_modifydescript(INTBIG addr, INTBIG type, INTBIG key, UINTBIG *old)
{
	Q_UNUSED( key );
	Q_UNUSED( old );

	switch (type&VTYPE)
	{
		case VNODEINST:
			us_computenodefartextbit((NODEINST *)addr);
			break;
		case VARCINST:
			us_computearcfartextbit((ARCINST *)addr);
			break;
	}
}

void us_newobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER UINTBIG wiped;
	REGISTER INTBIG i;
	REGISTER WINDOWPART *w;
	REGISTER NODEPROTO *onp;
	REGISTER EDITOR *ed;

	switch (type&VTYPE)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;

			/* look for the "cell-center" or "essential-bounds" nodes in the "generic" technology */
			if (ni->proto == gen_cellcenterprim) us_addcellcenter(ni); else
				if (ni->proto == gen_essentialprim)
					us_setessentialbounds(ni->parent);

			/* if a new instance was created, then the cell structure has changed */
			if (ni->proto->primindex == 0) us_cellstructurechanged = TRUE;

			/* recompute the parent's technology */
			ni->parent->tech = whattech(ni->parent);

			/* see if this affects in-place editing */
			us_checkinplaceedits(ni->parent);
			break;

		case VARCINST:
			ai = (ARCINST *)addr;

			/* see if this arcinst wipes out the visibility of some pins */
			if ((ai->proto->userbits&CANWIPE) != 0) for(i=0; i<2; i++)
			{
				ni = ai->end[i].nodeinst;

				wiped = us_computewipestate(ni);
				if (wiped != (ni->userbits&WIPED))
				{
					/* if node was visible, erase it before setting WIPED bit */
					if ((ni->userbits&WIPED) == 0) us_undisplayobject(ni->geom);
					ni->userbits = (ni->userbits & ~WIPED) | wiped;
				}
			}

			/* recompute the parent's technology */
			ai->parent->tech = whattech(ai->parent);

			/* see if this affects in-place editing */
			us_checkinplaceedits(ai->parent);
			break;

		case VNODEPROTO:
			np = (NODEPROTO *)addr;

			/* update status display if this is new version of what's displayed */
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if (w->curnodeproto == NONODEPROTO) continue;
				if (w->curnodeproto != np) continue;
				if ((w->state&WINDOWTYPE) == TEXTWINDOW)
				{
					ed = w->editor;
					if (ed != NOEDITOR) (void)reallocstring(&ed->header,
						describenodeproto(w->curnodeproto), us_tool->cluster);
					w->redisphandler(w);
				}
				us_setcellname(w);
			}

			/* the cell structure has changed */
			us_cellstructurechanged = TRUE;

			onp = us_curnodeproto;
			if (onp == NONODEPROTO) return;
			if (onp->primindex != 0) return;
			if (onp != np) return;
			us_curnodeproto = NONODEPROTO;
			us_shadownodeproto(NOWINDOWFRAME, onp);

			/* recompute the technology */
			np->tech = whattech(np);
			break;

		case VPORTPROTO:
			pp = (PORTPROTO *)addr;

			/* look at all instances of this nodeproto for use on display */
			for(ni = pp->parent->firstinst; ni != NONODEINST; ni = ni->nextinst)
				if ((ni->userbits & NEXPAND) == 0 || (pp->userbits&PORTDRAWN) != 0)
					us_queueredraw(ni->geom, FALSE);

			/* see if this affects in-place editing */
			us_checkinplaceedits(pp->parent);
			break;

		case VLIBRARY:
			/* the cell structure has changed */
			us_cellstructurechanged = TRUE;
			break;

		case VWINDOWPART:
			w = (WINDOWPART *)addr;
			us_newwindowcount++;
			if (us_firstnewwindow == NOWINDOWPART) us_firstnewwindow = w; else
				us_secondnewwindow = w;
			break;
	}
}

void us_killobject(INTBIG addr, INTBIG type)
{
	REGISTER INTBIG i;
	REGISTER UINTBIG wiped;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER WINDOWPART *w;

	switch (type&VTYPE)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			ni->userbits |= (RETDONN|REODONN);

			/* stop tracking duplicated node if it is deleted */
			if (ni == us_dupnode) us_dupnode = NONODEINST;

			/* if an instance was deleted, then the cell structure has changed */
			if (ni->proto->primindex == 0) us_cellstructurechanged = TRUE;

			/* look for the "cell-center" node in the "generic" technology */
			if (ni->proto == gen_cellcenterprim)
				us_delnodeprotocenter(ni->parent); else
					if (ni->proto == gen_essentialprim)
						us_setessentialbounds(ni->parent);

			/* recompute the parent's technology */
			ni->parent->tech = whattech(ni->parent);

			/* see if this affects in-place editing */
			us_checkinplaceedits(ni->parent);
			break;

		case VARCINST:
			ai = (ARCINST *)addr;

			/* see if this arcinst wiped out the visibility of some pins */
			if ((ai->proto->userbits&CANWIPE) != 0) for(i=0; i<2; i++)
			{
				ni = ai->end[i].nodeinst;

				/* if nodeinst still live and has other arcs, leave it */
				if ((ni->userbits&DEADN) != 0) continue;

				wiped = us_computewipestate(ni);
				if (wiped != (ni->userbits&WIPED))
				{
					/* if node was invisible, display before resetting WIPED bit */
					if ((ni->userbits&WIPED) != 0) us_queueredraw(ni->geom, FALSE);
					ni->userbits = (ni->userbits & ~WIPED) | wiped;
				}
			}

			/* recompute the parent's technology */
			ai->parent->tech = whattech(ai->parent);

			/* see if this affects in-place editing */
			us_checkinplaceedits(ai->parent);
			break;

		case VNODEPROTO:
			np = (NODEPROTO *)addr;

			/* if this may be a technology-edit cell, let it know */
			if (namesamen(np->protoname, x_("layer-"), 6) == 0)
				us_deltecedlayercell(np);
			if (namesamen(np->protoname, x_("node-"), 5) == 0)
				us_deltecednodecell(np);

			us_removeubchange(np);

			/* the cell structure has changed */
			us_cellstructurechanged = TRUE;
			break;

		case VPORTPROTO:
			pp = (PORTPROTO *)addr;

			/* look at all instances of this nodeproto for use on display */
			for(ni = pp->parent->firstinst; ni != NONODEINST; ni = ni->nextinst)
				if ((ni->userbits & NEXPAND) == 0 || (pp->userbits&PORTDRAWN) != 0)
			{
				us_undisplayobject(ni->geom);
				us_queueredraw(ni->geom, FALSE);
			}

			/* see if this affects in-place editing */
			us_checkinplaceedits(pp->parent);
			break;

		case VLIBRARY:
			/* the cell structure has changed */
			us_cellstructurechanged = TRUE;
			break;

		case VWINDOWPART:
			w = (WINDOWPART *)addr;
			copywindowpart(&us_oldwindow, w);
			us_killedwindow = w;
			break;
	}
}

void us_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
	REGISTER INTBIG len, i, x, y;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *w;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER LIBRARY *lib;
	REGISTER TOOL *tool;
	REGISTER TECHNOLOGY *tech;
	REGISTER CHAR *name, *pt;
	REGISTER USERCOM *rb;
	COMMANDBINDING commandbinding;

	if ((newtype&VCREF) != 0)
	{
		name = changedvariablename(type, key, newtype);

		/* see if cell name changed */
		if ((type&VTYPE) == VNODEPROTO && namesame(name, x_("protoname")) == 0)
		{
			np = (NODEPROTO *)addr;
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
				if (w->curnodeproto != NONODEPROTO && w->curnodeproto == np)
					us_setcellname(w);
			us_cellstructurechanged = TRUE;
			if (np == us_cellwithkilledname)
			{
				/* tell the technology editor that a cell name changed */
				us_renametecedcell(us_cellwithkilledname, us_killednameoncell);
			}
			return;
		}

		/* see if screen extent changed */
		if ((type&VTYPE) == VWINDOWPART && namesamen(name, x_("screen"), 6) == 0)
		{
			w = (WINDOWPART *)addr;
			computewindowscale(w);
			return;
		}

		/* see if technology name changed */
		if ((type&VTYPE) == VTECHNOLOGY && namesame(name, x_("techname")) == 0)
		{
			tech = (TECHNOLOGY *)addr;
			if (tech == el_curtech) us_settechname(NOWINDOWFRAME);
			return;
		}

		/* see if layer visibility changed */
		if ((type&VTYPE) == VGRAPHICS && namesame(name, x_("colstyle")) == 0)
		{
			us_figuretechselectability();
			return;
		}

		/* see if current cell or library name changed */
		if ((type&VTYPE) == VLIBRARY &&
			(namesame(name, x_("curnodeproto")) == 0 || namesame(name, x_("libname")) == 0))
		{
			lib = (LIBRARY *)addr;
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if (w->curnodeproto == NONODEPROTO) continue;
				if (w->curnodeproto->lib == lib) us_setcellname(w);
			}
			if (namesame(name, x_("libname")) == 0) us_cellstructurechanged = TRUE;
			if (namesame(name, x_("curnodeproto")) == 0) us_state |= CURCELLCHANGED;
			return;
		}

		/* see if a tool was turned on */
		if ((type&VTYPE) == VTOOL && namesame(name, x_("toolstate")) == 0)
		{
			tool = (TOOL *)addr;
			if ((tool->toolstate & TOOLON) != 0 &&
				(tool->toolstate & TOOLINCREMENTAL) != 0)
					us_toolturnedon(tool);
		}
		return;
	}

	/* see if an option variable changed */
	if ((newtype&VDONTSAVE) == 0)
	{
		if (isoptionvariable(addr, type, makename(key)))
			us_optionschanged = TRUE;
	}

	/* handle changes to objects on the user interface */
	if (addr == (INTBIG)us_tool)
	{
		/* pick up mirrors */
		for(i=0; us_variablemirror[i].key != 0; i++)
		{
			if (key == *us_variablemirror[i].key)
			{
				var = getvalkey(addr, type, -1, key);
				if (var != NOVARIABLE) *us_variablemirror[i].value = var->addr;
				if (key == us_menu_x_key || key == us_menu_y_key || key == us_menu_position_key)
					us_menuchanged = TRUE;
				return;
			}
		}

		if (key == us_alignment_ratio_key)
		{
			var = getvalkey(addr, type, VINTEGER, key);
			if (var != NOVARIABLE) us_alignment_ratio = var->addr;
			us_setalignment(NOWINDOWFRAME);
			return;
		}

		if (key == us_alignment_edge_ratio_key)
		{
			var = getvalkey(addr, type, VINTEGER, key);
			if (var != NOVARIABLE) us_edgealignment_ratio = var->addr;
			us_setalignment(NOWINDOWFRAME);
			return;
		}

		if (key == us_displayunitskey)
		{
			var = getvalkey(addr, type, VINTEGER, key);
			if (var != NOVARIABLE)
			{
				/* code cannot be called by multiple procesors: uses globals */
				NOT_REENTRANT;

				el_units = (el_units & ~DISPLAYUNITS) | (var->addr & DISPLAYUNITS);
			}
			return;
		}

		/* see if default text editor changed */
		if (key == us_text_editorkey)
		{
			var = getvalkey(addr, type, VSTRING, key);
			if (var == NOVARIABLE) return;
			for(i=0; us_editortable[i].editorname != 0; i++)
				if (namesame((CHAR *)var->addr, us_editortable[i].editorname) == 0) break;
			if (us_editortable[i].editorname == 0) return;

			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			us_currenteditor = i;
			return;
		}

		/* set current window if it changed */
		if (key == us_current_window_key)
		{
			var = getvalkey(addr, type, VWINDOWPART, key);
			if (var == NOVARIABLE) return;
			w = (WINDOWPART *)var->addr;
			us_highlightwindow(w, FALSE);
			if (w != NOWINDOWPART) us_setcellsize(w);
			return;
		}

		/* set colormap updating if it changed */
		if (key == us_colormap_red_key || key == us_colormap_green_key ||
			key == us_colormap_blue_key)
		{
			us_maplow = 0;
			var = getvalkey(addr, type, VINTEGER|VISARRAY, key);
			if (var == NOVARIABLE) return;
			len = getlength(var);
			if (us_maplow == -1) us_maphigh = len-1; else
				if (len-1 > us_maphigh) us_maphigh = len-1;
			return;
		}

		/* show highlight if it changed */
		if (key == us_highlightedkey)
		{
			us_state |= HIGHLIGHTSET;
			us_setselectioncount();
			us_highlighthaschanged();
			return;
		}

		/* shadow current nodeproto or arcproto if it changed */
		if (key == us_current_node_key)
		{
			var = getvalkey(addr, type, VNODEPROTO, key);
			if (var != NOVARIABLE) us_shadownodeproto(NOWINDOWFRAME, (NODEPROTO *)var->addr);
			return;
		}
		if (key == us_current_arc_key)
		{
			var = getvalkey(addr, type, VARCPROTO, key);
			if (var != NOVARIABLE) us_shadowarcproto(NOWINDOWFRAME, (ARCPROTO *)var->addr);
			return;
		}

		/* shadow placement angle if it changed */
		if (key == us_placement_angle_key)
		{
			us_setnodeangle(NOWINDOWFRAME);
			return;
		}

		/* switch technology if it changed */
		if (key == us_current_technology_key)
		{
			var = getvalkey(addr, type, VTECHNOLOGY, key);
			if (var == NOVARIABLE) return;
			el_curtech = (TECHNOLOGY *)var->addr;
			if (el_curtech != sch_tech && el_curtech != art_tech && el_curtech != gen_tech)
				el_curlayouttech = el_curtech;
			us_settechname(NOWINDOWFRAME);
			us_setlambda(NOWINDOWFRAME);
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart) us_setcellsize(w);
			return;
		}

		/* switch constraint solver if it changed */
		if (key == us_current_constraint_key)
		{
			var = getvalkey(addr, type, VCONSTRAINT, key);
			if (var == NOVARIABLE) return;
			el_curconstraint = (CONSTRAINT *)var->addr;
			return;
		}

		if (key == us_gridfloatskey || key == us_gridboldspacingkey)
		{
			us_gridfactorschanged = TRUE;
			return;
		}

		if (key == us_quickkeyskey)
		{
			var = getvalkey(addr, type, VSTRING|VISARRAY, key);
			if (var != NOVARIABLE) us_adjustquickkeys(var, FALSE);
			return;
		}

		/* see if popup menu was created */
		if (namesamen(makename(key), x_("USER_binding_popup_"), 19) == 0)
		{
			var = getvalkey(addr, type, VSTRING|VISARRAY, key);
			if (var == NOVARIABLE) return;
			len = getlength(var);

			/* create the popup menu */
			pm = (POPUPMENU *)emalloc(sizeof(POPUPMENU), us_tool->cluster);
			if (pm == 0)
			{
				ttyputnomemory();
				return;
			}
			mi = (POPUPMENUITEM *)emalloc((len-1) * sizeof(POPUPMENUITEM), us_tool->cluster);
			if (mi == 0)
			{
				ttyputnomemory();
				return;
			}

			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			pm->nextpopupmenu = us_firstpopupmenu;
			us_firstpopupmenu = pm;
			(void)allocstring(&pm->name, &makename(key)[19], us_tool->cluster);
			(void)allocstring(&pm->header, ((CHAR **)var->addr)[0], us_tool->cluster);
			pm->list = mi;
			pm->total = len-1;

			/* fill the menu */
			for(i=1; i<len; i++)
			{
				mi[i-1].response = rb = us_allocusercom();
				rb->active = -1;
				(void)allocstring(&mi[i-1].attribute, x_(""), us_tool->cluster);
				mi[i-1].value = 0;
				us_parsebinding(((CHAR **)var->addr)[i], &commandbinding);
				us_setcommand(commandbinding.command, rb, i-1, commandbinding.nodeglyph,
					commandbinding.arcglyph, commandbinding.menumessage,
						commandbinding.popup, commandbinding.inputpopup);
				us_freebindingparse(&commandbinding);
			}
			return;
		}
		return;
	}

	/* detect change to SPICE primitive set */
	if (type == VTOOL && key == sim_spice_partskey)
	{
		us_checkspiceparts();
		return;
	}

	/* handle changes to objects on technologies */
	if (type == VTECHNOLOGY)
	{
		/* changes to technology variables may invalidate caches */
		changedtechnologyvariable(key);

		/* if layer letters changed, recache the data */
		if (key == us_layer_letters_key)
		{
			us_initlayerletters();
			return;
		}

		if (key == el_techstate_key)
		{
			var = getvalkey(addr, VTECHNOLOGY, VINTEGER, el_techstate_key);
			if (var != NOVARIABLE)
				(void)asktech((TECHNOLOGY *)addr, x_("set-state"), var->addr);
		}
		return;
	}

	/* handle changes to objects on NODEPROTOs */
	if (type == VNODEPROTO)
	{
		if (key == el_node_size_default_key)
		{
			/* remember the size of the wire-pin in Schematics */
			np = (NODEPROTO *)addr;
			if (np == sch_wirepinprim)
			{
				defaultnodesize(sch_wirepinprim, &sch_wirepinsizex, &sch_wirepinsizey);
			}

			/* redraw the node in the component menu */
			if ((us_tool->toolstate&MENUON) == 0) return;
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
			if (var == NOVARIABLE) return;
			len = getlength(var);
			for(i=0; i<len; i++)
			{
				pt = ((CHAR **)var->addr)[i];
				us_parsebinding(pt, &commandbinding);
				if (commandbinding.nodeglyph != NONODEPROTO)
				{
					if (us_menupos <= 1)
					{
						y = i / us_menux;
						x = i % us_menux;
					} else
					{
						x = i / us_menuy;
						y = i % us_menuy;
					}
					us_drawmenuentry(x, y, pt);
				}
				us_freebindingparse(&commandbinding);
			}
			return;
		}
		return;
	}

	/* update far text if displayable variable is added to NODEINST or ARCINST */
	if ((newtype&VDISPLAY) != 0)
	{
		if (type == VNODEINST) us_computenodefartextbit((NODEINST *)addr); else
			if (type == VARCINST) us_computearcfartextbit((ARCINST *)addr);
	}

	/* handle changes to objects on ARCPROTOs */
	if (type == VARCPROTO)
	{
		if (key == el_arc_width_default_key)
		{
			/* redraw the arc in the component menu */
			if ((us_tool->toolstate&MENUON) == 0) return;
			var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_binding_menu_key);
			if (var == NOVARIABLE) return;
			ap = (ARCPROTO *)addr;
			len = getlength(var);
			for(i=0; i<len; i++)
			{
				pt = ((CHAR **)var->addr)[i];
				us_parsebinding(pt, &commandbinding);
				if (commandbinding.arcglyph != NOARCPROTO)
				{
					if (us_menupos <= 1)
					{
						y = i / us_menux;
						x = i % us_menux;
					} else
					{
						x = i / us_menuy;
						y = i % us_menuy;
					}
					us_drawmenuentry(x, y, pt);
				}
				us_freebindingparse(&commandbinding);
			}
			return;
		}
		return;
	}
}

void us_killvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG oldaddr,
	INTBIG oldtype, UINTBIG *olddescript)
{
	REGISTER INTBIG len, i;
	REGISTER CHAR *name;
	REGISTER POPUPMENU *pm, *lastpm;
	HIGHLIGHT high;
	REGISTER WINDOWPART *w;
	REGISTER EDITOR *e;
	Q_UNUSED( olddescript );

	if ((oldtype&VCREF) != 0)
	{
		name = changedvariablename(type, key, oldtype);

		/* see if cell name changed */
		if ((type&VTYPE) == VNODEPROTO && namesame(name, x_("protoname")) == 0)
		{
			us_cellwithkilledname = (NODEPROTO *)addr;
			us_killednameoncell = (CHAR *)oldaddr;
			return;
		}
		return;
	}

	/* see if an option variable changed */
	if (isoptionvariable(addr, type, makename(key)))
		us_optionschanged = TRUE;

	/* close any text editor windows that are examining the variable */
	if ((oldtype&VTYPE) == VSTRING && (oldtype&VISARRAY) != 0)
	{
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			if ((w->state&WINDOWTYPE) == TEXTWINDOW)
		{
			e = w->editor;
			if (e == NOEDITOR) continue;
			if (e->editobjvar == NOVARIABLE) continue;
			if (e->editobjvar->addr != oldaddr || e->editobjvar->type != (UINTBIG)oldtype) continue;
			if ((INTBIG)e->editobjaddr != addr || e->editobjtype != type) continue;
			us_delcellmessage(w->curnodeproto);
		}
	}

	/* handle changes to objects on the user interface */
	if (addr == (INTBIG)us_tool)
	{
		/* show highlight if it changed */
		if (key == us_highlightedkey && (us_state&HIGHLIGHTSET) == 0)
		{
			len = (oldtype&VLENGTH) >> VLENGTHSH;
			for(i=0; i<len; i++)
			{
				if (us_makehighlight(((CHAR **)oldaddr)[i], &high)) continue;
				us_sethighlight(&high, ALLOFF);
			}
			us_setselectioncount();
			us_highlighthaschanged();
			return;
		}

		/* shadow placement angle if it changed */
		if (key == us_placement_angle_key)
		{
			us_setnodeangle(NOWINDOWFRAME);
			return;
		}

		/* see if popup menu was deleted */
		if (namesamen(makename(key), x_("USER_binding_popup_"), 19) == 0)
		{
			/* code cannot be called by multiple procesors: uses globals */
			NOT_REENTRANT;

			name = &makename(key)[19];
			lastpm = NOPOPUPMENU;
			for(pm = us_firstpopupmenu; pm != NOPOPUPMENU; pm = pm->nextpopupmenu)
			{
				if (namesame(name, pm->name) == 0) break;
				lastpm = pm;
			}
			if (pm == NOPOPUPMENU) return;
			if (lastpm == NOPOPUPMENU) us_firstpopupmenu = pm->nextpopupmenu; else
				lastpm->nextpopupmenu = pm->nextpopupmenu;
			for(i=0; i<pm->total; i++)
			{
				efree(pm->list[i].attribute);
				us_freeusercom(pm->list[i].response);
			}
			efree(pm->name);
			efree(pm->header);
			efree((CHAR *)pm->list);
			efree((CHAR *)pm);
			return;
		}
	}
}

void us_modifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype,
	INTBIG aindex, INTBIG oldvalue)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG x, y;
	REGISTER CHAR *str;
	REGISTER USERCOM *rb;
	POPUPMENU *pm;
	COMMANDBINDING commandbinding;
	Q_UNUSED( oldvalue );

	if ((vartype&VCREF) != 0)
	{
		/* see if PORTPROTO's "textdescript[1]" field changed */
		if ((type&VTYPE) == VPORTPROTO && aindex == 1)
		{
			str = changedvariablename(type, key, vartype);
			if (namesame(str, x_("textdescript")) == 0)
				us_computenodefartextbit(((PORTPROTO *)addr)->subnodeinst);
		}
		return;
	}

	/* see if an option variable changed */
	if ((vartype&VDONTSAVE) == 0)
	{
		if (isoptionvariable(addr, type, makename(key)))
			us_optionschanged = TRUE;
	}

	/* handle changes to objects on the user interface */
	if (addr == (INTBIG)us_tool)
	{
		/* set colormap updating if it changed */
		if (key == us_colormap_red_key || key == us_colormap_green_key || key == us_colormap_blue_key)
		{
			if (us_maplow == -1) us_maplow = us_maphigh = aindex; else
			{
				if (aindex < us_maplow) us_maplow = aindex;
				if (aindex > us_maphigh) us_maphigh = aindex;
			}
		}

		/* set menu binding if it changed */
		if (key == us_binding_menu_key)
		{
			if (us_menuchanged) return;
			if ((us_tool->toolstate&MENUON) == 0) return;
			var = getvalkey(addr, type, VSTRING|VISARRAY, key);
			if (var == NOVARIABLE) return;
			us_parsebinding(((CHAR **)var->addr)[aindex], &commandbinding);
			if (us_menupos <= 1)
			{
				y = aindex / us_menux;
				x = aindex % us_menux;
			} else
			{
				x = aindex / us_menuy;
				y = aindex % us_menuy;
			}
			if (*commandbinding.command == 0) us_drawmenuentry(x, y, x_("")); else
				us_drawmenuentry(x, y, ((CHAR **)var->addr)[aindex]);
			us_freebindingparse(&commandbinding);
			return;
		}

		/* set popup menu binding if it changed */
		if (namesamen(makename(key), x_("USER_binding_popup_"), 19) == 0)
		{
			var = getvalkey(addr, type, VSTRING|VISARRAY, key);
			if (var == NOVARIABLE) return;
			if (aindex == 0)
			{
				/* special case: set menu header */
				str = &makename(key)[19];
				for(pm = us_firstpopupmenu; pm != NOPOPUPMENU; pm = pm->nextpopupmenu)
					if (namesame(str, pm->name) == 0) break;
				if (pm != NOPOPUPMENU)
					(void)reallocstring(&pm->header, ((CHAR **)var->addr)[0], us_tool->cluster);
				return;
			}
			us_parsebinding(((CHAR **)var->addr)[aindex], &commandbinding);
			if (commandbinding.popup != NOPOPUPMENU)
			{
				rb = commandbinding.popup->list[aindex-1].response;
				us_setcommand(commandbinding.command, rb, aindex-1, commandbinding.nodeglyph,
					commandbinding.arcglyph, commandbinding.menumessage,
						commandbinding.popup, commandbinding.inputpopup);
				nativemenurename(commandbinding.popup, aindex-1);
			}
			us_freebindingparse(&commandbinding);
			return;
		}
	}
}

void us_checkinplaceedits(NODEPROTO *cell)
{
	REGISTER WINDOWPART *w;
	REGISTER INTBIG i;

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&INPLACEEDIT) == 0) continue;
		for(i=0; i<w->inplacedepth; i++)
			if (cell == w->inplacestack[i]->parent) break;
		if (i < w->inplacedepth) continue;

		/* must redraw window "w" */
		w->state |= INPLACEQUEUEREDRAW;
	}
}

/*
 * routine to place the command "str" in the command object "rb".  Also sets
 * the node glyph if "nodeglyph" is not NONODEPROTO, the arc glyph if
 * "arcglyph" is not NOARCPROTO and the menu message if "menumessage" is nonzero.
 * If "popup" is not NOPOPUPMENU, this is a popup (and is input-popup if
 * "inputpopup" is true).  For popup menus, "mindex" is the menu entry.
 */
void us_setcommand(CHAR *str, USERCOM *rb, INTBIG mindex,
	NODEPROTO *nodeglyph, ARCPROTO *arcglyph, CHAR *menumessage,
	POPUPMENU *popup, BOOLEAN inputpopup)
{
	CHAR *a[MAXPARS+1];
	REGISTER INTBIG newcount, j, i;
	COMCOMP *carray[MAXPARS];
	extern COMCOMP us_userp;
	REGISTER void *infstr;

	/* handle un-binding */
	if (*str == 0)
	{
		if (popup != NOPOPUPMENU)
		{
			if (rb->message != 0) efree(rb->message);
			rb->message = 0;
			if (menumessage != 0)
			{
				(void)allocstring(&rb->message, menumessage, us_tool->cluster);
			}
			if (rb->message == 0)
				(void)reallocstring(&popup->list[mindex].attribute, x_(""), us_tool->cluster); else
					(void)reallocstring(&popup->list[mindex].attribute, rb->message, us_tool->cluster);
			popup->list[mindex].maxlen = -1;
			/* should "popup->list[mindex].value" be freed here? !!! */
		}
		if (rb->active >= 0)
		{
			for(j=0; j<rb->count; j++) efree(rb->word[j]);
			rb->count = 0;
			if (rb->message != 0) efree(rb->message);
			rb->message = 0;
		}
		rb->active = -1;
		return;
	}

	/* parse this command */
	newcount = us_parsecommand(str, a);
	if (newcount <= 0) return;
	i = parse(a[0], &us_userp, TRUE);
	if (i < 0) return;

	/* remove former command if it exists */
	if (rb->active >= 0)
	{
		for(j=0; j<rb->count; j++) efree(rb->word[j]);
		if (rb->message != 0) efree(rb->message);
		rb->message = 0;
	}

	/* set the new command */
	for(j=0; j<newcount-1; j++)
	{
		if (allocstring(&rb->word[j], a[j+1], us_tool->cluster))
		{
			ttyputnomemory();
			rb->active = -1;
			return;
		}
	}

	/* set all other information */
	if (menumessage == 0) rb->message = 0; else
	{
		if (rb->message != 0) efree(rb->message);
		(void)allocstring(&rb->message, menumessage, us_tool->cluster);
	}
	rb->count = newcount-1;
	rb->active = i;
	rb->menu = us_getpopupmenu(a[0]);
	if (rb->comname != 0) efree(rb->comname);
	(void)allocstring(&rb->comname, a[0], us_tool->cluster);
	rb->nodeglyph = nodeglyph;
	rb->arcglyph = arcglyph;
	if (popup != NOPOPUPMENU)
	{
		/* load popup menu with full command string */
		if (rb->message != 0)
			(void)reallocstring(&popup->list[mindex].attribute, rb->message, us_tool->cluster); else
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, rb->comname);
			us_appendargs(infstr, rb);
			(void)reallocstring(&popup->list[mindex].attribute, returninfstr(infstr), us_tool->cluster);
		}
		popup->list[mindex].valueparse = NOCOMCOMP;
		if (inputpopup)
		{
			/* determine command completion for input portion */
			i = us_fillcomcomp(rb, carray);
			if (i > rb->count && carray[rb->count] != NOCOMCOMP)
				popup->list[mindex].valueparse = carray[rb->count];
		}
		if (popup->list[mindex].valueparse == NOCOMCOMP)
			popup->list[mindex].maxlen = -1; else popup->list[mindex].maxlen = 20;
	}
}

/* Technology: Different Options */
static DIALOGITEM us_techdifoptdialogitems[] =
{
 /*  1 */ {0, {140,264,164,440}, BUTTON, N_("Use Requested Options")},
 /*  2 */ {0, {140,32,164,208}, BUTTON, N_("Leave Current Options")},
 /*  3 */ {0, {8,8,24,344}, MESSAGE, N_("This library requests different options for technology:")},
 /*  4 */ {0, {8,348,24,472}, MESSAGE, x_("")},
 /*  5 */ {0, {32,8,48,132}, MESSAGE, N_("Current options:")},
 /*  6 */ {0, {32,136,80,472}, MESSAGE, x_("")},
 /*  7 */ {0, {84,8,100,132}, MESSAGE, N_("Requested options:")},
 /*  8 */ {0, {84,136,132,472}, MESSAGE, x_("")}
};
static DIALOG us_techdifoptdialog = {{75,75,248,556}, N_("Technology Options Conflict"), 0, 8, us_techdifoptdialogitems, 0, 0};

/* special items for the "technology options" dialog: */
#define DDFT_USEREQUESTED  1		/* Use requested options (button) */
#define DDFT_LEAVECURRENT  2		/* Leave current options (button) */
#define DDFT_TECHNAME      4		/* Technology name (stat text) */
#define DDFT_CUROPTION     6		/* Current options (stat text) */
#define DDFT_REQOPTION     8		/* Requested options (stat text) */

void us_readlibrary(LIBRARY *lib)
{
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt, *name, *curstatename, *newstatename;
	CHAR *par[MAXPARS];
	REGISTER INTBIG majversion, minversion, found, i, j, nvers, len,
		curstate, itemHit, col;
	REGISTER BOOLEAN warnofchanges;
	REGISTER TECHNOLOGY *mocmostech, *tech;
	REGISTER POPUPMENU *pm;
	REGISTER POPUPMENUITEM *mi;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER USERCOM *rb;
	COMMANDBINDING commandbinding;
	extern COMCOMP us_yesnop;
	REGISTER void *infstr, *dia;

	/* create any popup menus found in the library */
	for(i=0; i<us_tool->numvar; i++)
	{
		/* only want permanent user-interface variables (ones just read in) */
		var = &us_tool->firstvar[i];
		if ((var->type&VDONTSAVE) != 0) continue;

		/* handle popup menus */
		if (namesamen(makename(var->key), x_("USER_binding_popup_"), 19) == 0)
		{
			name = &makename(var->key)[19];
			if (us_getpopupmenu(name) != NOPOPUPMENU) continue;

			/* create the popup menu */
			len = getlength(var);
			pm = (POPUPMENU *)emalloc(sizeof(POPUPMENU), us_tool->cluster);
			if (pm == 0)
			{
				ttyputnomemory();
				break;
			}
			mi = (POPUPMENUITEM *)emalloc((len-1) * sizeof(POPUPMENUITEM), us_tool->cluster);
			if (mi == 0)
			{
				ttyputnomemory();
				return;
			}
			pm->nextpopupmenu = us_firstpopupmenu;
			us_firstpopupmenu = pm;
			(void)allocstring(&pm->name, name, us_tool->cluster);
			(void)allocstring(&pm->header, ((CHAR **)var->addr)[0], us_tool->cluster);
			pm->list = mi;
			pm->total = len-1;

			/* initialize the menu */
			for(j=1; j<len; j++)
			{
				mi[j-1].response = rb = us_allocusercom();
				rb->active = -1;
				(void)allocstring(&mi[j-1].attribute, x_(""), us_tool->cluster);
				mi[j-1].value = 0;
			}
			us_scanforkeyequiv(pm);
		}
	}

	/* fill any popup menus found in the library */
	for(i=0; i<us_tool->numvar; i++)
	{
		/* only want permanent user-interface variables (ones just read in) */
		var = &us_tool->firstvar[i];
		if ((var->type&VDONTSAVE) != 0) continue;

		/* handle popup menus */
		if (namesamen(makename(var->key), x_("USER_binding_popup_"), 19) == 0)
		{
			var->type |= VDONTSAVE;
			name = &makename(var->key)[19];
			pm = us_getpopupmenu(name);
			mi = pm->list;
			len = getlength(var);

			/* fill the menu */
			for(j=1; j<len; j++)
			{
				rb = mi[j-1].response;
				us_parsebinding(((CHAR **)var->addr)[j], &commandbinding);
				us_setcommand(commandbinding.command, rb, j-1, commandbinding.nodeglyph,
					commandbinding.arcglyph, commandbinding.menumessage,
						pm, commandbinding.inputpopup);
				us_freebindingparse(&commandbinding);
			}
		}
	}

	/* recache mirrored variables now that the library may have overridden them */
	for(i=0; us_variablemirror[i].key != 0; i++)
	{
		var = getvalkey((INTBIG)us_tool, VTOOL, -1, *us_variablemirror[i].key);
		if (var != NOVARIABLE)
		{
			*us_variablemirror[i].value = var->addr;
		}
	}

	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_alignment_ratio_key);
	if (var != NOVARIABLE) us_alignment_ratio = var->addr;
	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_alignment_edge_ratio_key);
	if (var != NOVARIABLE) us_edgealignment_ratio = var->addr;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_quickkeyskey);
	if (var != NOVARIABLE)
	{
		if ((lib->userbits&HIDDENLIBRARY) == 0) warnofchanges = TRUE; else
			warnofchanges = FALSE;
		us_adjustquickkeys(var, warnofchanges);
	}

	var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_displayunitskey);
	if (var != NOVARIABLE)
		el_units = (el_units & ~DISPLAYUNITS) | (var->addr & DISPLAYUNITS);

	/* make sure the electrical units are the same */
	var = getvalkey((INTBIG)lib, VLIBRARY, VINTEGER, us_electricalunitskey);
	if (var != NOVARIABLE && var->addr != us_electricalunits)
	{
		us_adjustelectricalunits(lib, var->addr);
	}

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_text_editorkey);
	if (var != NOVARIABLE)
	{
		for(i=0; us_editortable[i].editorname != 0; i++)
			if (namesame((CHAR *)var->addr, us_editortable[i].editorname) == 0) break;
		if (us_editortable[i].editorname != 0)
			us_currenteditor = i;
	}
	defaultnodesize(sch_wirepinprim, &sch_wirepinsizex, &sch_wirepinsizey);

	/* grab new SPICE parts */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_spice_partskey);
	if (var != NOVARIABLE) us_checkspiceparts();

	/* recache technology variables */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		/* reset technology state ("TECH_state") */
		var = getvalkey((INTBIG)tech, VTECHNOLOGY, VINTEGER, el_techstate_key);
		if (var != NOVARIABLE)
			(void)asktech(tech, x_("set-state"), var->addr);

		/* check out desired state ("TECH_last_state") */
		var = getvalkey((INTBIG)tech, VTECHNOLOGY, VINTEGER, us_techlaststatekey);
		if (var != NOVARIABLE)
		{
			curstate = asktech(tech, x_("get-state"));
			if (curstate != var->addr)
			{
				/* see if there is anything from this technology in the library */
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						if (ni->proto->primindex == 0) continue;
						if (ni->proto->tech == tech) break;
					}
					if (ni != NONODEINST) break;
					for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
						if (ai->proto->tech == tech) break;
					if (ai != NOARCINST) break;
				}
				if (np != NONODEPROTO)
				{
					curstatename = (CHAR *)asktech(tech, x_("describe-state"), curstate);
					newstatename = (CHAR *)asktech(tech, x_("describe-state"), var->addr);
					if (curstatename != 0 && newstatename != 0)
					{
						if ((lib->userbits&HIDDENLIBRARY) != 0) itemHit = DDFT_USEREQUESTED; else
						{
							/* display the technology option conflict dialog box */
							dia = DiaInitDialog(&us_techdifoptdialog);
							if (dia == 0) return;

							/* load the message */
							DiaSetText(dia, DDFT_TECHNAME, tech->techname);
							DiaSetText(dia, DDFT_CUROPTION, curstatename);
							DiaSetText(dia, DDFT_REQOPTION, newstatename);

							/* loop until done */
							for(;;)
							{
								itemHit = DiaNextHit(dia);
								if (itemHit == DDFT_USEREQUESTED || itemHit == DDFT_LEAVECURRENT) break;
							}
							DiaDoneDialog(dia);
						}

						if (itemHit == DDFT_USEREQUESTED)
						{
							(void)asktech(tech, x_("set-state"), var->addr);
							setvalkey((INTBIG)tech, VTECHNOLOGY, el_techstate_key, var->addr, VINTEGER);
							if (el_curtech == tech)
							{
								par[0] = x_("size");
								par[1] = x_("auto");
								us_menu(2, par);
								us_setmenunodearcs();
							}
						}
					}
				}
			}
		}

		/* see if layer patterns changed */
		for(i=0; i<tech->layercount; i++)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("TECH_layer_pattern_"));
			addstringtoinfstr(infstr, layername(tech, i));
			pt = returninfstr(infstr);
			var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER|VISARRAY, pt);
			if (var == NOVARIABLE) continue;

			/* adjust for old patterns with smaller amounts of data */
			len = getlength(var);
			if (len < 16)
			{
				INTBIG newpattern[18];

				for(j=0; j<8; j++)
					newpattern[j] = newpattern[j+8] = ((INTBIG *)var->addr)[j];
				for(j=8; j<len; j++)
					newpattern[j+8] = ((INTBIG *)var->addr)[j];
				nextchangequiet();
				(void)setval((INTBIG)el_curtech, VTECHNOLOGY, pt,
					(INTBIG)newpattern, VINTEGER|VISARRAY|(18<<VLENGTHSH));
				var = getval((INTBIG)el_curtech, VTECHNOLOGY, VINTEGER|VISARRAY, pt);
			}

			(el_curtech->layers[i])->colstyle = (INTSML)(((INTBIG *)var->addr)[16]);
			for(j=0; j<16; j++)
				(el_curtech->layers[i])->raster[j] = (INTSML)(((INTBIG *)var->addr)[j]);
			if (getlength(var) > 17)
			{
				col = ((INTBIG *)var->addr)[17];
				(el_curtech->layers[i])->col = col;
				if (col == COLORT1 || col == COLORT2 || col == COLORT3 ||
					col == COLORT4 || col == COLORT5)
						(el_curtech->layers[i])->bits = col; else
							(el_curtech->layers[i])->bits = LAYERO;
			}
		}
	}

	/* redisplay the explorer window */
	us_cellstructurechanged = TRUE;

	/* set cell's technology and center attributes */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		np->tech = whattech(np);
		us_setessentialbounds(np);
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->proto == gen_cellcenterprim)
				us_setnodeprotocenter(ni->lowx, ni->lowy, np);
	}

	/* do font conversion if any is specified */
	us_checkfontassociations(lib);

	/*
	 * see if the library is old enough to require MOCMOS conversion
	 * or serpentine port conversion
	 */
	var = getval((INTBIG)lib, VLIBRARY, VSTRING, x_("LIB_former_version"));
	if (var == NOVARIABLE) return;

	/* parse the former Electric version number */
	pt = (CHAR *)var->addr;
	majversion = eatoi(pt);
	while (*pt != 0 && *pt != '.') pt++;
	if (*pt == 0) minversion = 0; else minversion = eatoi(pt+1);
	nvers = majversion*100 + minversion;
	if (nvers > 335) return;

	/* versions 3.35 or earlier may require MOCMOS conversion */
	mocmostech = gettechnology(x_("mocmos"));
	if (mocmostech == NOTECHNOLOGY) return;

	/* see if there is any MOCMOS data in the library */
	found = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			if (ni->proto->primindex != 0 && ni->proto->tech == mocmostech)
		{
			found++;
			break;
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			if (ai->proto->tech == mocmostech)
		{
			found++;
			break;
		}
		if (found != 0) break;
	}
	if (found == 0) return;

	/* there are MOCMOS elements in an old library: offer conversion */
	ttyputmsg(_("This library contains old format MOSIS CMOS (mocmos) cells"));
	i = ttygetparam(_("Would you like to convert them? [y] "), &us_yesnop, MAXPARS, par);
	if (i > 0 && namesamen(par[0], x_("no"), estrlen(par[0])) == 0)
	{
		ttyputmsg(_("No conversion done.  To do this later, use:"));
		ttyputmsg(x_("    -technology tell mocmos convert-old-format-library"));
		return;
	}

	/* do the conversion */
	tech_convertmocmoslib(lib);
}

/*
 * Routine to examine the font associations stored in the library and correct if
 * necessary.
 */
void us_checkfontassociations(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG *fontmatch, i, len, index, j, maxindex;
	CHAR *facename, *pt;
	REGISTER VARIABLE *var;

	var = getval((INTBIG)lib, VLIBRARY, VSTRING|VISARRAY, x_("LIB_font_associations"));
	if (var == NOVARIABLE) return;
	len = getlength(var);
	maxindex = 0;
	for(i=0; i<len; i++)
	{
		pt = ((CHAR **)var->addr)[i];
		index = eatoi(pt);
		if (index > maxindex) maxindex = index;
	}
	if (maxindex <= 0) return;
	maxindex++;
	fontmatch = (INTBIG *)emalloc(maxindex * SIZEOFINTBIG, us_tool->cluster);
	if (fontmatch == 0) return;
	for(i=0; i<maxindex; i++) fontmatch[i] = 0;
	for(i=0; i<len; i++)
	{
		pt = ((CHAR **)var->addr)[i];
		index = eatoi(pt);
		while (*pt != 0 && *pt != '/') pt++;
		if (*pt == 0) continue;
		facename = &pt[1];

		/* see if there is face "facename" and associate it with font "index" */
		j = screenfindface(facename);
		if (j >= 0) fontmatch[index] = j; else
		{
			ttyputerr(x_("Font %s not found, using default"), facename);
		}
	}

	/* now apply the transformation to all face indices */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			us_setfontassociationvar(ni->numvar, ni->firstvar, fontmatch);
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				us_setfontassociationvar(pi->numvar, pi->firstvar, fontmatch);
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				us_setfontassociationvar(pe->numvar, pe->firstvar, fontmatch);
			if (ni->proto->primindex == 0)
				us_setfontassociationdescript(ni->textdescript, fontmatch);
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			us_setfontassociationvar(ai->numvar, ai->firstvar, fontmatch);
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			us_setfontassociationvar(pp->numvar, pp->firstvar, fontmatch);
			us_setfontassociationdescript(pp->textdescript, fontmatch);
		}
		us_setfontassociationvar(np->numvar, np->firstvar, fontmatch);
	}
	us_setfontassociationvar(lib->numvar, lib->firstvar, fontmatch);

	(void)delval((INTBIG)lib, VLIBRARY, x_("LIB_font_associations"));
	efree((CHAR *)fontmatch);
}

/*
 * Helper routine for "us_checkfontassociations".
 */
void us_setfontassociationvar(INTSML numvar, VARIABLE *firstvar, INTBIG *fontmatch)
{
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		us_setfontassociationdescript(var->textdescript, fontmatch);
	}
}

/*
 * Helper routine for "us_checkfontassociations".
 */
void us_setfontassociationdescript(UINTBIG *descript, INTBIG *fontmatch)
{
	REGISTER INTBIG face;

	face = TDGETFACE(descript);
	if (face != 0) TDSETFACE(descript, fontmatch[face]);
}

void us_eraselibrary(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		us_removeubchange(np);
	us_unqueueredraw(lib);
}

void us_writelibrary(LIBRARY *lib, BOOLEAN pass2)
{
	REGISTER INTBIG state;
	REGISTER TECHNOLOGY *tech;
	Q_UNUSED( pass2 );

	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		if (asktech(tech, x_("has-state")) == 0) continue;
		state = asktech(tech, x_("get-state"));
		nextchangequiet();
		setvalkey((INTBIG)tech, VTECHNOLOGY, us_techlaststatekey, state, VINTEGER);
	}

	/* save electrical units on the library */
	nextchangequiet();
	setvalkey((INTBIG)lib, VLIBRARY, us_electricalunitskey, us_electricalunits, VINTEGER);
}

/*
 * handle options. Returns true upon error
 */
BOOLEAN us_options(INTBIG *argc, CHAR1 *argv[])
{
	REGISTER CHAR *retur, *thisargv;

	us_ignore_cadrc = FALSE;
	us_logging = TRUE;

	while (*argc > 1 && argv[1][0] == '-')
	{
		thisargv = string2byte(argv[1]);
		switch (thisargv[1])
		{
			case '-':			/* long options */
				if (estrcmp(&thisargv[2], x_("help")) == 0)
				{
					error(_("Usage: electric [-c] [-geom WxH+X+Y] [-i MACROFILE] [-m] [-n] [-t TECHNOLOGY] [LIBRARY]"));
				}
				if (estrcmp(&thisargv[2], x_("version")) == 0)
				{
					error(_("Electric version %s"), el_version);
				}
				break;

			case 'c':			/* ignore cadrc file */
				us_ignore_cadrc = TRUE;
				break;

			case 'g':			/* X11 Geometry spec SRP 2-19-91 */
				(*argc)--; argv++;	/* bump past the geometry string */
				break;

			case 'i':			/* initial macro file to load after cadrc */
				allocstring(&us_firstmacrofile, string2byte(argv[2]), us_tool->cluster);
				(*argc)--;   argv++;
				break;

			case 'm':			/* search for multiple monitors, used in graphXXX.c */
			case 'M':
				break;

			case 'n':			/* no session recording */
				us_logging = FALSE;
				break;

			case 'q':			/* qt drawing, used in graphqt.cpp */
				break;

			case 't':
				if (thisargv[2] != 0) retur = &thisargv[2]; else
				{
					retur = string2byte(argv[2]);
					(*argc)--;   argv++;
				}
				el_curtech = gettechnology(retur);
				if (el_curtech == NOTECHNOLOGY) error(_("Unknown technology: '%s'"), retur);
				if (el_curtech != sch_tech && el_curtech != art_tech && el_curtech != gen_tech)
					el_curlayouttech = el_curtech;
				break;

#ifdef	sun
			case 'W':			/* SUN window switches must be ignored */
				switch (thisargv[2])
				{
					case 'g':	/* default color */
					case 'H':	/* help */
					case 'i':	/* iconic */
					case 'n':	/* no label */
						break;
					case 'I':	/* icon image */
					case 'l':	/* label */
					case 'L':	/* icon label */
					case 't':	/* font */
					case 'T':	/* icon font */
					case 'w':	/* width */
						(*argc)--;   argv++;   break;
					case 'p':	/* position */
					case 'P':	/* closed position */
					case 's':	/* size */
						(*argc) -= 2;   argv += 2;   break;
					case 'b':	/* background color */
					case 'f':	/* foreground color */
						(*argc) -= 3;   argv += 3;   break;
				}
				break;
#endif

			default:
				error(_("Unrecognized switch: %s"), thisargv);
		}
		(*argc)--;
		argv++;
	}

	/* remember the initial library if specified */
	if (*argc == 2)
	{
		(*argc)--;
		allocstring(&us_firstlibrary, string2byte(argv[1]), us_tool->cluster);
	}

	/* check the arguments */
	if (*argc > 1)
		error(_("Usage: electric [-o] [-n] [-c] [-f] [-geom WxH+X+Y] [-t TECHNOLOGY] [LIBRARY]"));
	return(FALSE);
}

void us_findcadrc(void)
{
	CHAR *suf, *hd;
	REGISTER void *infstr;

	/* try "cadrc" in current directory */
	if (!us_docadrc(CADRCFILENAME)) return;

	/* if there is a "home" directory, try it there */
	hd = hashomedir();
	if (hd != 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, hd);
		addstringtoinfstr(infstr, CADRCFILENAME);
		suf = returninfstr(infstr);
		if (!us_docadrc(suf)) return;
	}

	/* not found there: try library directory */
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, CADRCFILENAME);
	suf = returninfstr(infstr);
	if (!us_docadrc(suf)) return;

	ttyputerr(_("Cannot find '%s' startup file: installation may be incorrect"),
		CADRCFILENAME);
}

/*
 * get the commands in the system's or user's "cadrc" file: "name".
 * Returns true if the file cannot be found.
 */
BOOLEAN us_docadrc(CHAR *name)
{
	FILE *in;
	CHAR *filename;

	/* look for startup file */
	in = xopen(name, us_filetypecadrc, x_(""), &filename);
	if (in == NULL) return(TRUE);

	/* create a new macro package */
	us_curmacropack = us_newmacropack(x_("CADRC"));

	/* execute commands beginning with "electric" in this file */
	us_docommands(in, FALSE, x_("electric"));
	xclose(in);

	/* now there is no macro package */
	us_curmacropack = NOMACROPACK;
	return(FALSE);
}

void us_wanttoread(CHAR *name)
{
	estrcpy(us_desiredlibrary, name);
}
