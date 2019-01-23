/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usr.h
 * User interface tool: header file
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/******************** CONSTANTS ********************/

/* the meaning of NODEINST->userbits: (other bit (0137400177) used globally) */
#define KILLN                   0200		/* if on, this nodeinst is marked for death */
#define REWANTN                 0400		/* nodeinst re-drawing is scheduled */
#define RELOCLN                01000		/* only local nodeinst re-drawing desired */
#define RETDONN                02000		/* transparent nodeinst re-draw is done */
#define REODONN                04000		/* opaque nodeinst re-draw is done */
#define NODEFLAGBIT           010000		/* general flag used in spreading and highlighting */
#define WANTEXP               020000		/* if on, nodeinst wants to be (un)expanded */
#define TEMPFLG               040000		/* temporary flag for nodeinst display */
#define HARDSELECTN          0100000		/* set if hard to select */
#define NVISIBLEINSIDE     040000000		/* set if node only visible inside cell */

/* the meaning of ARCINST->userbits: (bits 0-1, 4-24 (0177777763) used globally) */
#define KILLA             0200000000		/* if on, this arcinst is marked for death */
#define REWANTA           0400000000		/* arcinst re-drawing is scheduled */
#define RELOCLA          01000000000		/* only local arcinst re-drawing desired */
#define RETDONA          02000000000		/* transparent arcinst re-draw is done */
#define REODONA          04000000000		/* opaque arcinst re-draw is done */
#define ARCFLAGBIT      010000000000		/* general flag used in spreading and highlighting */
#define HARDSELECTA     020000000000		/* set if hard to select */

/* the meaning of NODEPROTO->userbits: (bits 0-23 (077777777) used globally) */
#define NHASOPA           0100000000		/* set if prototype has opaque graphics */
#define NFIRSTOPA       017600000000		/* index of first opaque layer (if any) */
#define NFIRSTOPASH               25		/* right shift of NFIRSTOPA */
#define NINVISIBLE      020000000000		/* set if primitive is invisible */

	/* the meaning of ARCPROTO->userbits: (bits 0-22,31 (020037777777) used globally) */
#define CANCONNECT         040000000		/* temporary for finding port connections */
#define AHASOPA           0100000000		/* set if prototype has opaque graphics */
#define AFIRSTOPA        07600000000		/* index of first opaque layer (if any) */
#define AINVISIBLE      010000000000		/* set if arc is invisible */
#define AFIRSTOPASH               25		/* right shift of AFIRSTOPA */

/* the meaning of PORTPROTO->userbits: (bits 0-31 used globally) */

/* the meaning of "us_tool->toolstate" (bits 0-6 (0177) used globally) */
#define NOKEYLOCK            0200000		/* set to ignore key lockout after errors */
#define JUSTTHEFACTS         0400000		/* set to print only informative messages */
#define NODETAILS           01000000		/* set to make no mention of specific commands */
#define USEDIALOGS          02000000		/* set to use dialogs where appropriate */
#define ONESHOTMENU         04000000		/* set to have fixed menus highlight in one-shot mode */
#define INTERACTIVE        010000000		/* set if cursor commands drag */
#define INTEGRAL           020000000		/* set if windows must be integral pixels */
#define ECHOBIND           040000000		/* set to echo bound commands in full */
#define MENUON            0100000000		/* set if menu is on */
#define SHOWXY            0200000000		/* set to display cursor rather than TECH/LAMBDA */
#define TTYAUDIT          0400000000		/* set to log messages window */
#define TERMBEEP         01000000000		/* set to enable terminal beep */
#define TERMNOINTERRUPT  02000000000		/* set to disable interrupts */

/* the meaning of "tool:user.USER_optionflags" */
#define NOTEXTSELECT                  01	/* set to disable selection of ALL text unless "special" option used */
#define DRAWTINYCELLS                 02	/* clear to "hash-out" tiny cells */
#define CHECKDATE                     04	/* set to check cell dates */
#define NOEXTRASOUND                 010	/* set to disable extra sounds (like arc-creation clicks) */
#define NOPRIMCHANGES                020	/* set to lock lockable primitives */
#define PORTLABELS                  0700	/* bits for port labels */
#define PORTLABELSSH                   6	/* right shift for port labels */
#define PORTSFULL                      0	/* ports as complete text */
#define PORTSCROSS                  0100	/* ports drawn as crosses */
#define PORTSSHORT                  0300	/* ports drawn as shortened text */

#define NOINSTANCESELECT           02000	/* set to disable selection of instances unless "special" option used */
#define NOMOVEAFTERDUP             04000	/* set to disable moving of objects after duplication */
#define DUPCOPIESPORTS            010000	/* set to have duplication/array/extract copy ports */
#define NO3DPERSPECTIVE           020000	/* set to turn off perspective in 3D view */
#define EXPANDEDDIALOGSDEF        040000	/* set to have expanded dialogs default to expanded */
#define CENTEREDPRIMITIVES       0100000	/* set to place primitives by centers */
#define AUTOSWITCHTECHNOLOGY     0200000	/* set to switch technology when cell changes */
#define EXPORTLABELS            01400000	/* bits for export labels */
#define EXPORTLABELSSH                17	/* right shift for export labels */
#define EXPORTSFULL                    0	/* exports drawn as complete text */
#define EXPORTSCROSS             0400000	/* exports drawn as crosses */
#define EXPORTSSHORT            01400000	/* exports drawn as shortened text */
#define MOVENODEWITHEXPORT      02000000	/* move node when export text is moved */
#define NODATEORVERSION         04000000	/* set to supress date/version in output files */
#define BEEPAFTERLONGJOB       010000000	/* set to beep after a long job */
#define CELLCENTERALWAYS       020000000	/* set to place cell-center in new cells */
#define MUSTENCLOSEALL         040000000	/* set to force area-selection to completely enclose objects */
#define HIDETXTNODE           0100000000	/* set to supress node text */
#define HIDETXTARC            0200000000	/* set to supress arc text */
#define HIDETXTPORT           0400000000	/* set to supress port text */
#define HIDETXTEXPORT        01000000000	/* set to supress export text */
#define HIDETXTNONLAY        02000000000	/* set to supress nonlayout text */
#define HIDETXTINSTNAME      04000000000	/* set to supress instance name text */
#define HIDETXTCELL         010000000000	/* set to supress cell text */
#define NOPROMPTBEFOREWRITE 020000000000	/* set to disable file name dialog before writing output files */

/* the meaning of "JAVA_flags" on the user tool object */
#define JAVANOCOMPILER             1		/* disable compiler */
#define JAVANOEVALUATE             2		/* do not evaluate code */
#define JAVAUSEJOSE                4		/* Use Jose */

/* special keys */
#define CTRLCKEY                  03		/* the Control-C key */
#define CTRLDKEY                  04		/* the Control-D key */
#define BACKSPACEKEY             010		/* the BACKSPACE key */
#define TABKEY                   011		/* the TAB key */
#define ESCKEY                   033		/* the Escape key */
#define DELETEKEY               0177		/* the DELETE key */

/* the O/S capabilities */
#define CANLOGINPUT               01		/* can log input for playback */
#define CANUSEFRAMES              02		/* can display multiple window frames */
#define CANSHOWPOPUP              04		/* can display popup menus */
#define CANRUNPROCESS            010		/* can run subprocesses (fork and exec) */
#define CANHAVENOWINDOWS         020		/* can exist with no edit windows */
#define CANSTATUSPERFRAME        040		/* can put separate status info in each frame */
#define CANCHOOSEFACES          0100		/* can choose alternate type faces */
#define CANMODIFYFONTS          0200		/* can modify fonts with italic, bold, etc. */
#define CANSCALEFONTS           0400		/* can scale fonts smoothly */
#define CANDOTHREADS           01000		/* can handle threads */
#define CANHAVEQUITCOMMAND     02000		/* can have quit command in menu */

/* the icon styles (in tool:user.USER_icon_style) */
#define ICONSTYLESIDEIN             03			/* side number for input ports     (         */
#define ICONSTYLESIDEINSH            0			/* shift for ICONSTYLESIDEIN */
#define ICONSTYLESIDEOUT           014			/* side number for input ports      0=left   */
#define ICONSTYLESIDEOUTSH           2			/* shift for ICONSTYLESIDEOUT */
#define ICONSTYLESIDEBIDIR         060			/* side number for input ports      1=right  */
#define ICONSTYLESIDEBIDIRSH         4			/* shift for ICONSTYLESIDEBIDIR */
#define ICONSTYLESIDEPOWER        0300			/* side number for input ports      2=top    */
#define ICONSTYLESIDEPOWERSH         6			/* shift for ICONSTYLESIDEPOWER */
#define ICONSTYLESIDEGROUND      01400			/* side number for input ports      3=bottom */
#define ICONSTYLESIDEGROUNDSH        8			/* shift for ICONSTYLESIDEGROUND */
#define ICONSTYLESIDECLOCK       06000			/* side number for input ports     )         */
#define ICONSTYLESIDECLOCKSH        10			/* shift for ICONSTYLESIDECLOCK */
#define ICONSTYLEPORTLOC       0140000			/* location of ports */
#define ICONSTYLEPORTLOCSH          14			/* shift for ICONSTYLEPORTLOC */
#define ICONSTYLEPORTSTYLE     0600000			/* style of port text */
#define ICONSTYLEPORTSTYLESH        16			/* shift for ICONSTYLEPORTSTYLE */
#define ICONSTYLEDRAWNOLEADS  01000000			/* nonzero to remove leads */
#define ICONSTYLEDRAWNOBODY   02000000			/* nonzero to remove body */
#define ICONSTYLETECH        014000000			/* technology of ports  0=generic, 1=schematic */
#define ICONSTYLETECHSH             20			/* shift for ICONSTYLETECH */
#define ICONINSTLOC          060000000			/* instance location */
#define ICONINSTLOCSH               22			/* shift for ICONINSTLOC */
#define ICONSTYLEREVEXPORT  0100000000			/* nonzero to reverse export order */

#define ICONSTYLEDEFAULT (0<<ICONSTYLESIDEINSH) | (1<<ICONSTYLESIDEOUTSH) | \
						 (2<<ICONSTYLESIDEBIDIRSH) | (3<<ICONSTYLESIDEPOWERSH) | \
						 (3<<ICONSTYLESIDEGROUNDSH) | (0<<ICONSTYLESIDECLOCKSH) | \
						 (1<<ICONSTYLEPORTLOCSH) | (0<<ICONSTYLEPORTSTYLESH) | \
						 (0<<ICONSTYLETECHSH)
#define ICONLEADDEFLEN   2
#define ICONLEADDEFSEP   2

/* the "style" parameter to "us_getcolormap()" */
#define COLORSEXISTING             0		/* reload existing colors */
#define COLORSDEFAULT              1		/* load default colors (gray background) */
#define COLORSBLACK                2		/* load black background colors */
#define COLORSWHITE                3		/* load white background colors */

/* editor defaults in the user interface */
#define DEFAULTBUTTONHANDLER     us_buttonhandler
#define DEFAULTCHARHANDLER       us_charhandler
#define DEFAULTCHANGEHANDLER       0
#define DEFAULTTERMHANDLER         0
#define DEFAULTREDISPHANDLER     us_redisplay

/* the values in call to "us_gethighlighted()" */
#define WANTNODEINST                1		/* want nodes */
#define WANTARCINST                 2		/* want arcs */

/* the meaning of "us_state" */
#define COMMANDFAILED             01		/* set if current command has error */
#define SKIPKEYS                  02		/* set to ignore further single-key commands */
#define GOTXY                     04		/* set if X/Y co-ordinates are known */
#define LANGLOOP                 010		/* set if interpreting language code */
#define HIGHLIGHTSET             020		/* set if highlighting turned on in broadcast */
#define DIDINPUT                 040		/* set if just did input but haven't executed yet */
#define MACROTERMINATED         0100		/* set if macro has terminated early */
#define SNAPMODE               03600		/* snapping mode bits */
#define SNAPMODENONE               0		/* snapping mode: none */
#define SNAPMODECENTER          0200		/* snapping mode: center (circles, arcs) */
#define SNAPMODEMIDPOINT        0400		/* snapping mode: midpoint (lines) */
#define SNAPMODEENDPOINT        0600		/* snapping mode: endpoint (lines, arcs) */
#define SNAPMODETANGENT        01000		/* snapping mode: tangent (circles, arcs) */
#define SNAPMODEPERP           01200		/* snapping mode: perpendicular (lines, arcs, circles) */
#define SNAPMODEQUAD           01400		/* snapping mode: quadrant (circles, arcs) */
#define SNAPMODEINTER          01600		/* snapping mode: any intersection */
#define NONOVERLAPPABLEDISPLAY 04000		/* set to disallow overlappable layers in display */
#define NONPERSISTENTCURNODE  010000		/* clear to have current node be persistent */
#define CURCELLCHANGED        020000		/* set if current cell changed */
#define MEASURINGDISTANCE     040000		/* set if measuring distance */
#define MEASURINGDISTANCEINI 0100000		/* set if measurement initializing */

/* cursor definitions */
#define NORMALCURSOR               0		/* a normal arrow cursor */
#define WANTTTYCURSOR              1		/* a "use the TTY" cursor */
#define PENCURSOR                  2		/* a "pen" cursor */
#define NULLCURSOR                 3		/* a null cursor */
#define MENUCURSOR                 4		/* a menu selection cursor */
#define HANDCURSOR                 5		/* a hand cursor (for dragging) */
#define TECHCURSOR                 6		/* the technology edit cursor ("T") */
#define IBEAMCURSOR                7		/* the text edit cursor ("I") */
#define WAITCURSOR                 8		/* the hourglass cursor */
#define LRCURSOR                   9		/* a left/right pointing cursor */
#define UDCURSOR                  10		/* an up/down pointing cursor */

#define MESSAGESWIDTH             80		/* width of messages window */
#define DEFAULTEXPLORERTEXTSIZE   12		/* default size of text in explorer window */

#define DISPLAYSMALLSHIFT         20		/* fraction of screen that shifts when arrows are clicked */
#define DISPLAYLARGESHIFT          2		/* fraction of screen that shifts when scrollbar is clicked */

/******************** VARIABLES ********************/

/* static display information */
extern INTBIG       us_longcount;			/* number of long commands */
extern void       (*us_displayroutine)(POLYGON*, WINDOWPART*); /* routine for graphics display */
extern INTBIG       us_cursorstate;			/* see defines above */
extern INTBIG       us_normalcursor;		/* default cursor to use */
extern BOOLEAN      us_optionschanged;		/* true if options changed */
extern INTBIG       us_layer_letters_key;	/* variable key for "USER_layer_letters" */
extern INTBIG       us_highlightedkey;		/* variable key for "USER_highlighted" */
extern INTBIG       us_highlightstackkey;	/* variable key for "USER_highlightstack" */
extern INTBIG       us_binding_keys_key;	/* variable key for "USER_binding_keys" */
extern INTBIG       us_binding_buttons_key;	/* variable key for "USER_binding_buttons" */
extern INTBIG       us_binding_menu_key;	/* variable key for "USER_binding_menu" */
extern INTBIG       us_current_node_key;	/* variable key for "USER_current_node" */
extern INTBIG       us_current_arc_key;		/* variable key for "USER_current_arc" */
extern INTBIG       us_placement_angle_key;	/* variable key for "USER_placement_angle" */
extern INTBIG       us_alignment_ratio_key;	/* variable key for "USER_alignment_ratio" */
extern INTBIG       us_alignment_edge_ratio_key;/* variable key for "USER_alignment_edge_ratio" */
extern INTBIG       us_arcstylekey;			/* variable key for "USER_arc_style" */
extern INTBIG       us_current_technology_key;	/* variable key for "USER_current_technology" */
extern INTBIG       us_current_window_key;	/* variable key for "USER_current_window" */
extern INTBIG       us_current_constraint_key;	/* variable key for "USER_current_constraint" */
extern INTBIG       us_colormap_red_key;	/* variable key for "USER_colormap_red" */
extern INTBIG       us_colormap_green_key;	/* variable key for "USER_colormap_green" */
extern INTBIG       us_colormap_blue_key;	/* variable key for "USER_colormap_blue" */
extern INTBIG       us_copyright_file_key;	/* variable key for "USER_copyright_file" */
extern INTBIG       us_menu_position_key;	/* variable key for "USER_colormap_position" */
extern INTBIG       us_menu_x_key;			/* variable key for "USER_colormap_x" */
extern INTBIG       us_menu_y_key;			/* variable key for "USER_colormap_y" */
extern INTBIG       us_macrorunningkey;		/* variable key for "USER_macrorunning" */
extern INTBIG       us_macrobuildingkey;	/* variable key for "USER_macrobuilding" */
extern INTBIG       us_text_editorkey;		/* variable key for "USER_text_editor" */
extern INTBIG       us_optionflagskey;		/* variable key for "USER_optionflags" */
extern INTBIG       us_gridfloatskey;		/* variable key for "USER_grid_floats" */
extern INTBIG       us_gridboldspacingkey;	/* variable key for "USER_grid_bold_spacing" */
extern INTBIG       us_quickkeyskey;		/* variable key for "USER_quick_keys" */
extern INTBIG       us_interactiveanglekey;	/* variable key for "USER_interactive_angle" */
extern INTBIG       us_tinylambdaperpixelkey;	/* variable key for "USER_tiny_lambda_per_pixel" */
extern INTBIG       us_ignoreoptionchangeskey;	/* variable key for "USER_ignore_option_changes" */
extern INTBIG       us_displayunitskey;		/* variable key for "USER_display_units" */
extern INTBIG       us_electricalunitskey;	/* variable key for "USER_electrical_units" */
extern INTBIG       us_motiondelaykey;		/* variable key for "USER_motion_delay" */
extern INTBIG       us_java_flags_key;		/* variable key for "JAVA_flags" */
extern INTBIG       us_edtec_option_key;	/* variable key for "EDTEC_option" */

extern INTBIG       us_electricalunits;		/* mirror for "USER_electrical_units" */
extern INTBIG       us_javaflags;			/* mirror for "JAVA_flags" */
extern INTBIG       us_useroptions;			/* mirror for "USER_optionflags" */
extern INTBIG       us_tinyratio;			/* mirror for "USER_tiny_lambda_per_pixel" */
extern INTBIG       us_separatechar;		/* separating character in port names */

/* file type descriptors */
extern INTBIG       us_filetypecolormap;	/* Color map */
extern INTBIG       us_filetypehelp;		/* Help */
extern INTBIG       us_filetypelog;			/* Log */
extern INTBIG       us_filetypemacro;		/* Macro */
extern INTBIG       us_filetypenews;		/* News */

/* internal state */
extern INTBIG       us_state;				/* miscellaneous state of user interface */

/* user interface state */
extern NODEPROTO   *us_curnodeproto;		/* current nodeproto */
extern ARCPROTO    *us_curarcproto;			/* current arcproto */
extern INTBIG       us_alignment_ratio;		/* current alignment in fractional units */
extern INTBIG       us_edgealignment_ratio;	/* current edge alignment in fractional units */
extern BOOLEAN      us_dupdistset;			/* true if duplication distance is valid */
extern NODEINST    *us_dupnode;				/* a node that was duplicated */
extern LIBRARY     *us_clipboardlib;		/* library with cut/copy/paste cell */
extern NODEPROTO   *us_clipboardcell;		/* cell with cut/copy/paste objects */
extern INTBIG       us_dupx, us_dupy;		/* amount to move duplicated objects */
extern WINDOWFRAME *us_menuframe;			/* window frame on which menu resides */
#ifndef USEQT
extern INTSML       us_erasech, us_killch;	/* the erase and kill characters */
#endif
extern INTBIG       us_menux, us_menuy;		/* number of menu elements on the screen */
extern INTBIG       us_menupos;				/* position: 0:top 1:bottom 2:left 3:right */
extern INTBIG       us_menuxsz,us_menuysz;	/* size of menu elements */
extern INTBIG       us_menuhnx,us_menuhny;	/* highlighted nodeinst menu entry */
extern INTBIG       us_menuhax,us_menuhay;	/* highlighted arcinst menu entry */
extern INTBIG       us_menulx,us_menuhx;	/* X: low and high of menu area */
extern INTBIG       us_menuly,us_menuhy;	/* Y: low and high of menu area */
extern INTBIG       us_lastmeasurex, us_lastmeasurey;	/* last measured distance */
extern BOOLEAN      us_validmesaure;		/* true if a measure was done */
extern INTBIG       us_explorerratio;		/* percentage of window used by cell explorer */
extern FILE        *us_logrecord;			/* logging record file */
extern FILE        *us_logplay;				/* logging playback file */
extern FILE        *us_termaudit;			/* messages window auditing file */
extern FILE        *us_tracefile;			/* trace file */
extern INTBIG       us_logflushfreq;		/* session logging flush frequency */
extern INTBIG       us_quickkeyfactcount;	/* number of quick keys in "factory" setup */
extern CHAR       **us_quickkeyfactlist;	/* quick keys in "factory" setup */
extern INTBIG       us_lastcommandcount;	/* the keyword count for the last command set by the "remember" command */
extern INTBIG       us_lastcommandtotal;	/* size of keyword array for the last command set by the "remember" command */
extern CHAR       **us_lastcommandpar;		/* the keywords for the last command set by the "remember" command */

/******************** HIGHLIGHT MODULES ********************/

/*
 * Highlight modules contain the following:
 *
 *  status   fromgeom  fromport   frompoint  fromvar     cell         what is selected
 * ========  ========  =========  =========  ========  =========  ============================
 * HIGHFROM  nodeinst  ---------  ---------  --------  nodeproto  node
 * HIGHFROM  arcinst   ---------  ---------  --------  nodeproto  arc
 * HIGHFROM  nodeinst  portproto  ---------  --------  nodeproto  node and port
 * HIGHFROM  nodeinst  ---------  integer    --------  nodeproto  node and outline point
 * HIGHTEXT  nodeinst  ---------  ---------  variable  nodeproto  displayable variable on node
 * HIGHTEXT  arcinst   ---------  ---------  variable  nodeproto  displayable variable on arc
 * HIGHTEXT  nodeinst  portproto  ---------  variable  nodeproto  displayable variable on port
 * HIGHTEXT  --------  ---------  ---------  variable  nodeproto  displayable variable on cell
 * HIGHTEXT  nodeinst  portproto  ---------  --------  nodeproto  port name
 * HIGHTEXT  nodeinst  ---------  ---------  --------  nodeproto  node name
 * HIGHBBOX  --------  ---------  ---------  --------  nodeproto  bounding box
 * HIGHLINE  --------  ---------  ---------  --------  nodeproto  line
 */

#define NOHIGHLIGHT ((HIGHLIGHT *)-1)
#define HIGHFROM                  01		/* set to display "from" object */
#define HIGHBBOX                  04		/* set to display bounding box */
#define HIGHLINE                 010		/* set to display a line */
#define HIGHTEXT                 020		/* set to display text */
#define HIGHTYPE                 037		/* the above highlight types */
#define HIGHEXTRA                040		/* set to display extra information */
#define HIGHSNAP                0100		/* set if snap point selected */
#define HIGHSNAPTAN             0200		/* set if tangent snap point selected */
#define HIGHSNAPPERP            0400		/* set if perpendicular snap point selected */
#define HIGHNOBOX              01000		/* set to disable basic selection box */

typedef struct Ihighlight
{
	struct Ihighlight *nexthighlight;		/* next in linked list */
	INTBIG            status;				/* nature of this highlighting (see above) */
	NODEPROTO        *cell;					/* the cell containing this highlight */
	GEOM             *fromgeom;				/* "from" object: geometry module */
	PORTPROTO        *fromport;				/* "from" object: port on node */
	INTBIG            frompoint;			/* "from" object: polygon vertex */
	VARIABLE         *fromvar;				/* "from" object: variable with text */
	VARIABLE         *fromvarnoeval;		/* "from" object: unevaluated variable with text */
	INTBIG            snapx, snapy;			/* HIGHSNAP: coordinate of snap point */
	INTBIG            stalx,staly;			/* HIGHBBOX: lower corner HIGHLINE: point 1 */
	INTBIG            stahx,stahy;			/* HIGHBBOX: upper corner HIGHLINE: point 2 */
} HIGHLIGHT;

/******************** MACRO PACKAGE MODULES ********************/

#define NOMACROPACK ((MACROPACK *)-1)

typedef struct Imacropack
{
	CHAR               *packname;			/* name of this macro package */
	INTBIG              total;				/* number of macros in this package */
	struct  Imacropack *nextmacropack;		/* next in list of macros */
} MACROPACK;

extern MACROPACK *us_macropacktop;			/* top of list of defined macro packages */
extern MACROPACK *us_curmacropack;			/* current macro package */

/******************** USER COMMAND MODULES ********************/

#define NOUSERCOM     ((USERCOM *)-1)
#define NUMKEYS                  256		/* maximum keys on keyboard */
#define NUMBUTS                   45		/* maximum buttons on tablet/mouse */

typedef struct Iusercom
{
	INTBIG             active;				/* command index (-1 if none) */
	CHAR              *comname;				/* name of this command */
	struct Ipopupmenu *menu;				/* pop-up menu to execute (if command is one) */
	CHAR              *message;				/* message to place in menu */
	NODEPROTO         *nodeglyph;			/* nodeproto glyph to display in menu */
	ARCPROTO          *arcglyph;			/* arcproto glyph to display in menu */
	INTBIG             count;				/* number of valid parameters */
	CHAR              *word[MAXPARS];		/* parameter from user */
	struct Iusercom   *nextcom;				/* next in list */
} USERCOM;

extern USERCOM *us_usercomfree;				/* list of free user commands */
extern USERCOM *us_lastcom;					/* last command and arguments */

/******************** POP-UP MENU MODULES ********************/

#define NOPOPUPMENUITEM ((POPUPMENUITEM *)-1)

typedef struct
{
	CHAR    *attribute;						/* attribute name */
	CHAR    *value;							/* value of the attribute (0 for none) */
	COMCOMP *valueparse;					/* command completion for "value" field */
	INTBIG   maxlen;						/* length of value field (-1 to preserve) */
	USERCOM *response;						/* command in this menu entry */
	BOOLEAN  changed;						/* true if value is changed */
} POPUPMENUITEM;

#define NOPOPUPMENU ((POPUPMENU *)-1)

typedef struct Ipopupmenu
{
	CHAR              *name;				/* pop-up menu name */
	POPUPMENUITEM     *list;				/* array of pop-up menu items */
	INTBIG             total;				/* number of items */
	CHAR              *header;				/* header information for the menu */
	struct Ipopupmenu *nextpopupmenu;		/* next in linked list */
} POPUPMENU;

extern POPUPMENU *us_firstpopupmenu;		/* list of existing pop-up menus */

/******************** PULLDOWN MENUS ********************/

#define MENUBARHEIGHT             25		/* size in pixels */

extern INTBIG      us_pulldownmenucount;	/* number of pulldown menus */
extern INTBIG     *us_pulldownmenupos;		/* position of pulldown menus */
extern POPUPMENU **us_pulldowns;			/* the current pulldown menus */

/******************** BINDINGS ********************/

typedef struct
{
	NODEPROTO *nodeglyph;					/* node to be drawn in the menu */
	ARCPROTO  *arcglyph;					/* arc to be drawn in the menu */
	INTBIG     backgroundcolor;				/* background color of the menu */
	INTBIG     menumessagesize;				/* size of text to be drawn in the menu */
	CHAR      *menumessage;					/* text to be drawn in the menu */
	POPUPMENU *popup;						/* popup menu that goes here */
	BOOLEAN    inputpopup;					/* true if the popup is input type */
	CHAR      *command;						/* command to execute */
} COMMANDBINDING;

/******************** HOOKS TO DIALOGS IN OTHER TOOLS ********************/

#define NODIALOGHOOK ((DIALOGHOOK *)-1)

typedef struct IDialogHook
{
	CHAR               *terminputkeyword;
	COMCOMP            *getlinecomp;
	void              (*routine)(void);
	struct IDialogHook *nextdialoghook;
} DIALOGHOOK;

/******************** STATUS BAR ********************/

typedef struct
{
	INTBIG line;							/* line in status area where field resides */
	INTBIG startper, endper;				/* percentage of status line used by field */
	CHAR  *label;							/* field label */
} STATUSFIELD;

extern STATUSFIELD *us_statusalign;			/* current node alignment */
extern STATUSFIELD *us_statusangle;			/* current placement angle */
extern STATUSFIELD *us_statusarc;			/* current arc prototype */
extern STATUSFIELD *us_statuscell;			/* current cell */
extern STATUSFIELD *us_statuscellsize;		/* current cell size */
extern STATUSFIELD *us_statusselectcount;	/* current selection count */
extern STATUSFIELD *us_statusgridsize;		/* current grid size */
extern STATUSFIELD *us_statuslambda;		/* current lambda */
extern STATUSFIELD *us_statusnode;			/* current node prototype */
extern STATUSFIELD *us_statustechnology;	/* current technology */
extern STATUSFIELD *us_statusxpos;			/* current x position */
extern STATUSFIELD *us_statusypos;			/* current y position */
extern STATUSFIELD *us_statusproject;		/* current project */
extern STATUSFIELD *us_statusroot;			/* current root */
extern STATUSFIELD *us_statuspart;			/* current part */
extern STATUSFIELD *us_statuspackage;		/* current package */
extern STATUSFIELD *us_statusselection;		/* current selection */

/******************** EDITOR TABLE ********************/

typedef struct
{
	CHAR    *editorname;							/* name of this editor */
	WINDOWPART *(*makeeditor)(WINDOWPART*, CHAR*, INTBIG*, INTBIG*);/* create new editor in window */
	void    (*terminate)(EDITOR*);						/* terminate editor */
	INTBIG  (*totallines)(WINDOWPART*);					/* report number of lines in editor buffer */
	CHAR   *(*getline)(WINDOWPART*, INTBIG);			/* get line from editor buffer */
	void    (*addline)(WINDOWPART*, INTBIG, CHAR*);		/* add line to editor buffer */
	void    (*replaceline)(WINDOWPART*, INTBIG, CHAR*);	/* replace line in editor */
	void    (*deleteline)(WINDOWPART*, INTBIG);			/* delete line in editor */
	void    (*highlightline)(WINDOWPART*, INTBIG, INTBIG);	/* highlight line in editor */
	void    (*suspendgraphics)(WINDOWPART*);			/* suspend display changes in editor */
	void    (*resumegraphics)(WINDOWPART*);				/* resume display changes in editor */
	void    (*writetextfile)(WINDOWPART*, CHAR*);		/* write editer buffer to file */
	void    (*readtextfile)(WINDOWPART*, CHAR*);		/* read file into editor */
	void    (*editorterm)(WINDOWPART*);					/* editor termination routine */
	void    (*shipchanges)(WINDOWPART*);				/* routine to force any changes out */
	BOOLEAN (*gotchar)(WINDOWPART*, INTSML, INTBIG);	/* character input routine */
	void    (*cut)(WINDOWPART*);						/* text cut routine */
	void    (*copy)(WINDOWPART*);						/* text copy routine */
	void    (*paste)(WINDOWPART*);						/* text paste routine */
	void    (*undo)(WINDOWPART*);						/* text undo routine */
	void    (*search)(WINDOWPART*, CHAR*, CHAR*, INTBIG);	/* text search/replace routine */
	void    (*pan)(WINDOWPART*, INTBIG, INTBIG);		/* pan window in direction */
} EDITORTABLE;

extern EDITORTABLE us_editortable[];
extern INTBIG us_currenteditor;

/******************** PARSING MODULES ********************/

/* the main command table (with additional information) */
struct commandinfo
{
	CHAR    *name;							/* name of this command */
	void   (*routine)(INTBIG, CHAR*[]);		/* routine to execute this command */
	INTBIG   params;						/* number of parameters to command */
	COMCOMP *par[TEMPLATEPARS];				/* parameter types */
};

/* defined in "usercom.c" */
extern struct commandinfo us_lcommand[];

#define MAXKEYWORD               100		/* maximum keywords on input */

extern KEYWORD *us_pathiskey;				/* the current keyword list */

/******************** MIRRORING VARIABLES ********************/

typedef struct
{
	INTBIG  *key;
	INTBIG  *value;
} VARMIRROR;

/******************** PROTOTYPES ********************/

/* prototypes for tool interface */
void   us_init(INTBIG*, CHAR1*[], TOOL*);
void   us_done(void);
void   us_set(INTBIG, CHAR*[]);
INTBIG us_request(CHAR*, va_list);
void   us_examinenodeproto(NODEPROTO*);
void   us_slice(void);
void   us_startbatch(TOOL*, BOOLEAN);
void   us_endbatch(void);
void   us_startobjectchange(INTBIG, INTBIG);
void   us_endobjectchange(INTBIG, INTBIG);
void   us_modifynodeinst(NODEINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void   us_modifynodeproto(NODEPROTO*);
void   us_modifydescript(INTBIG, INTBIG, INTBIG, UINTBIG*);
void   us_newobject(INTBIG, INTBIG);
void   us_killobject(INTBIG, INTBIG);
void   us_newvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void   us_killvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
void   us_modifyvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void   us_readlibrary(LIBRARY*);
void   us_eraselibrary(LIBRARY*);
void   us_writelibrary(LIBRARY*, BOOLEAN);

/* prototypes for user commands */
void us_arc(INTBIG, CHAR*[]);
void us_array(INTBIG, CHAR*[]);
void us_bind(INTBIG, CHAR*[]);
void us_color(INTBIG, CHAR*[]);
void us_commandfile(INTBIG, CHAR*[]);
void us_constraint(INTBIG, CHAR*[]);
void us_copycell(INTBIG, CHAR*[]);
void us_create(INTBIG, CHAR*[]);
void us_debug(INTBIG, CHAR*[]);
void us_defarc(INTBIG, CHAR*[]);
void us_defnode(INTBIG, CHAR*[]);
void us_duplicate(INTBIG, CHAR*[]);
void us_echo(INTBIG, CHAR*[]);
void us_editcell(INTBIG, CHAR*[]);
void us_erase(INTBIG, CHAR*[]);
void us_find(INTBIG, CHAR*[]);
void us_getproto(INTBIG, CHAR*[]);
void us_grid(INTBIG, CHAR*[]);
void us_help(INTBIG, CHAR*[]);
void us_if(INTBIG, CHAR*[]);
void us_interpret(INTBIG, CHAR*[]);
void us_iterate(INTBIG, CHAR*[]);
void us_killcell(INTBIG, CHAR*[]);
void us_lambda(INTBIG, CHAR*[]);
void us_library(INTBIG, CHAR*[]);
void us_macbegin(INTBIG, CHAR*[]);
void us_macdone(INTBIG, CHAR*[]);
void us_macend(INTBIG, CHAR*[]);
void us_menu(INTBIG, CHAR*[]);
void us_mirror(INTBIG, CHAR*[]);
void us_move(INTBIG, CHAR*[]);
void us_node(INTBIG, CHAR*[]);
void us_offtool(INTBIG, CHAR*[]);
void us_ontool(INTBIG, CHAR*[]);
void us_outhier(INTBIG, CHAR*[]);
void us_package(INTBIG, CHAR*[]);
void us_port(INTBIG, CHAR*[]);
void us_quit(INTBIG, CHAR*[]);
void us_redraw(INTBIG, CHAR*[]);
void us_remember(INTBIG, CHAR*[]);
void us_rename(INTBIG, CHAR*[]);
void us_replace(INTBIG, CHAR*[]);
void us_rotate(INTBIG, CHAR*[]);
void us_show(INTBIG, CHAR*[]);
void us_size(INTBIG, CHAR*[]);
void us_spread(INTBIG, CHAR*[]);
void us_system(INTBIG, CHAR*[]);
void us_technology(INTBIG, CHAR*[]);
void us_telltool(INTBIG, CHAR*[]);
void us_terminal(INTBIG, CHAR*[]);
void us_text(INTBIG, CHAR*[]);
void us_undo(INTBIG, CHAR*[]);
void us_var(INTBIG, CHAR*[]);
void us_view(INTBIG, CHAR*[]);
void us_visiblelayers(INTBIG, CHAR*[]);
void us_window(INTBIG, CHAR*[]);
void us_yanknode(INTBIG, CHAR*[]);

/* prototypes for text editing */
void           us_describeeditor(CHAR**);
WINDOWPART    *us_makeeditor(WINDOWPART*, CHAR*, INTBIG*, INTBIG*);
INTBIG         us_totallines(WINDOWPART*);
CHAR          *us_getline(WINDOWPART*, INTBIG);
void           us_addline(WINDOWPART*, INTBIG, CHAR*);
void           us_replaceline(WINDOWPART*, INTBIG, CHAR*);
void           us_deleteline(WINDOWPART*, INTBIG);
void           us_highlightline(WINDOWPART*, INTBIG, INTBIG);
void           us_suspendgraphics(WINDOWPART*);
void           us_resumegraphics(WINDOWPART*);
void           us_writetextfile(WINDOWPART*, CHAR*);
void           us_readtextfile(WINDOWPART*, CHAR*);
void           us_editorterm(WINDOWPART*);
void           us_shipchanges(WINDOWPART*);
BOOLEAN        us_gotchar(WINDOWPART*, INTSML, INTBIG);
void           us_cuttext(WINDOWPART*);
void           us_copytext(WINDOWPART*);
void           us_pastetext(WINDOWPART*);
void           us_undotext(WINDOWPART*);
void           us_searchtext(WINDOWPART*, CHAR*, CHAR*, INTBIG);
void           us_pantext(WINDOWPART*, INTBIG, INTBIG);

/* prototypes for intratool interface */
void           us_3dbuttonhandler(WINDOWPART *w, INTBIG but, INTBIG x, INTBIG y);
void           us_3denddrawing(void);
void           us_3dfillview(WINDOWPART *win);
void           us_3dpanview(WINDOWPART *win, INTBIG x, INTBIG y);
void           us_3dsetinteraction(INTBIG interaction);
void           us_3dsetupviewing(WINDOWPART*);
void           us_3dshowpoly(POLYGON*, WINDOWPART*);
void           us_3dstartdrawing(WINDOWPART*);
void           us_3dzoomview(WINDOWPART *win, float z);
void           us_abortcommand(CHAR*, ...);
void           us_abortedmsg(void);
void           us_addcellcenter(NODEINST *ni);
void           us_addhighlight(HIGHLIGHT*);
void           us_addparameter(NODEINST *ni, INTBIG key, INTBIG addr, INTBIG type, UINTBIG *descript);
void           us_addpossiblearcconnections(void*);
void           us_adjustdisplayabletext(NODEINST*);
void           us_adjustelectricalunits(LIBRARY *lib, INTBIG newunits);
void           us_adjustfornodeincell(NODEPROTO *prim, NODEPROTO *cell, INTBIG *cx, INTBIG *cy);
BOOLEAN        us_adjustparameterlocations(NODEINST *ni);
void           us_adjustperpendicularsnappoints(HIGHLIGHT*, HIGHLIGHT*);
void           us_adjustquickkeys(VARIABLE *var, BOOLEAN warnofchanges);
void           us_adjusttangentsnappoints(HIGHLIGHT*, HIGHLIGHT*);
void           us_alignnodes(INTBIG total, NODEINST **nodelist, BOOLEAN horizontal, INTBIG direction);
INTBIG         us_alignvalue(INTBIG, INTBIG, INTBIG*);
EDITOR        *us_alloceditor(void);
USERCOM       *us_allocusercom(void);
void           us_appendargs(void*, USERCOM*);
void           us_arrayfromfile(CHAR *file);
void           us_beginchanges(void);
CHAR          *us_bettervariablename(CHAR *pt);
INTBIG         us_bottomrecurse(NODEINST*, PORTPROTO*);
USERCOM       *us_buildcommand(INTBIG, CHAR*[]);
void           us_buildquickkeylist(void);
void           us_buildtexthighpoly(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, POLYGON*);
void           us_buttonhandler(WINDOWPART*, INTBIG, INTBIG, INTBIG);
BOOLEAN        us_cantedit(NODEPROTO*, NODEINST*, BOOLEAN);
BOOLEAN        us_charhandler(WINDOWPART*, INTSML, INTBIG);
void           us_chatportproto(NODEINST*, PORTPROTO*);
void           us_check_cell_date(NODEPROTO*, UINTBIG);
void           us_checkdatabase(BOOLEAN);
void           us_checkspiceparts(void);
BOOLEAN        us_cleanupcell(NODEPROTO*, BOOLEAN);
void           us_clearclipboard(void);
void           us_clearhighlightcount(void);
void           us_clearwindow(WINDOWPART*);
CHAR          *us_commandvarname(INTSML);
void           us_computearcfartextbit(ARCINST *ai);
void           us_computenodefartextbit(NODEINST *ni);
UINTBIG        us_computewipestate(NODEINST *ni);
void           us_continuesessionlogging(void);
NODEPROTO     *us_convertcell(NODEPROTO*, TECHNOLOGY*);
CHAR          *us_convertmactoworld(CHAR *buf);
CHAR          *us_convertworldtomac(CHAR *buf);
void           us_copyexplorerwindow(void);
void           us_copylisttocell(GEOM**, NODEPROTO*, NODEPROTO*, BOOLEAN, BOOLEAN, BOOLEAN);
void           us_copyobjects(WINDOWPART*);
NODEPROTO     *us_copyrecursively(NODEPROTO*, CHAR*, LIBRARY*, VIEW*, BOOLEAN, BOOLEAN, CHAR*, BOOLEAN, BOOLEAN, BOOLEAN);
void           us_coverimplant(void);
void           us_createexplorerstruct(WINDOWPART*);
void           us_createqueuedexports(void);
NODEPROTO     *us_currentexplorernode(void);
BOOLEAN        us_cursoroverhigh(HIGHLIGHT*, INTBIG, INTBIG, WINDOWPART*);
INTBIG         us_curvearcaboutpoint(ARCINST *ai, INTBIG xcur, INTBIG ycur);
INTBIG         us_curvearcthroughpoint(ARCINST *ai, INTBIG xcur, INTBIG ycur);
void           us_cutobjects(WINDOWPART*);
void           us_deleteexplorerstruct(WINDOWPART*);
void           us_delcellmessage(NODEPROTO*);
void           us_delhighlight(HIGHLIGHT*);
void           us_delnodeprotocenter(NODEPROTO*);
void           us_deltecedlayercell(NODEPROTO *np);
void           us_deltecednodecell(NODEPROTO *np);
BOOLEAN        us_demandxy(INTBIG*, INTBIG*);
CHAR          *us_describeboundkey(INTSML key, INTBIG special, INTBIG readable);
void           us_describecontents(NODEPROTO*);
CHAR          *us_describefont(INTBIG);
CHAR          *us_describemenunode(COMMANDBINDING *commandbinding);
CHAR          *us_describenodeinsttype(NODEPROTO *np, NODEINST *ni, INTBIG bits);
CHAR          *us_describestyle(INTBIG);
CHAR          *us_describetextdescript(UINTBIG*);
void           us_dialogeditor(void);
CHAR          *us_displayedportname(PORTPROTO *pp, INTBIG dispstyle);
void           us_docommands(FILE*, BOOLEAN, CHAR*);
void           us_doexpand(NODEINST*, INTBIG, INTBIG);
void           us_dofillet(void);
void           us_dokillcell(NODEPROTO *np);
void           us_dopeek(INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*, WINDOWPART*);
void           us_doubchanges(void);
void           us_dounexpand(NODEINST*);
INTBIG         us_drawarcinst(ARCINST*, INTBIG, XARRAY, INTBIG, WINDOWPART*);
void           us_drawdispwindowsliders(WINDOWPART *w);
void           us_drawexplorericon(WINDOWPART *w, INTBIG lx, INTBIG ly);
INTBIG         us_drawcell(NODEINST*, INTBIG, XARRAY, INTBIG, WINDOWPART*);
void           us_drawcellvariable(VARIABLE *var, NODEPROTO *np);
void           us_drawhorizontalslider(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_drawhorizontalsliderthumb(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_drawmenu(INTBIG, WINDOWFRAME*);
void           us_drawmenuentry(INTBIG, INTBIG, CHAR*);
INTBIG         us_drawnodeinst(NODEINST*, INTBIG, XARRAY, INTBIG, WINDOWPART*);
void           us_drawnodeprotovariables(NODEPROTO *np, XARRAY trans, WINDOWPART *w, BOOLEAN ignoreinherit);
void           us_drawportprotovariables(PORTPROTO *pp, INTBIG on, XARRAY trans, WINDOWPART *w, BOOLEAN ignoreinherit);
void           us_drawslidercorner(WINDOWPART *win, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, BOOLEAN drawtopright);
void           us_drawverticalslider(WINDOWPART*, INTBIG, INTBIG, INTBIG, BOOLEAN);
void           us_drawverticalsliderthumb(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_drawwindow(WINDOWPART*, INTBIG);
void           us_drawwindowicon(WINDOWPART *w, INTBIG lx, INTBIG ly, INTBIG *dots);
void           us_dumppulldownmenus(void);
void           us_editvariabletext(VARIABLE*, INTBIG, INTBIG, CHAR*);
void           us_emitcopyright(FILE *f, CHAR *prefix, CHAR *postfix);
void           us_endchanges(WINDOWPART*);
void           us_ensurepropertechnology(NODEPROTO *cell, CHAR *forcedtechnology, BOOLEAN always);
void           us_ensurewindow(NODEPROTO*);
void           us_erasegeometry(NODEPROTO*);
void           us_erasenodeinst(NODEINST*);
void           us_eraseobjectsinlist(NODEPROTO *np, GEOM **list);
INTBIG         us_erasepassthru(NODEINST*, BOOLEAN, ARCINST**);
void           us_erasescreen(WINDOWFRAME*);
void           us_erasewindow(WINDOWPART*);
BOOLEAN        us_evaluatevariable(CHAR*, INTBIG*, INTBIG*, CHAR**);
void           us_execute(USERCOM*, BOOLEAN, BOOLEAN, BOOLEAN);
void           us_executesafe(USERCOM*, BOOLEAN, BOOLEAN, BOOLEAN);
BOOLEAN        us_expandaddrtypearray(INTBIG*, INTBIG**, INTBIG**, INTBIG);
void           us_explorebuttonhandler(WINDOWPART*, INTBIG, INTBIG, INTBIG);
BOOLEAN        us_explorecharhandler(WINDOWPART*, INTSML, INTBIG);
void           us_explorehpan(WINDOWPART*, INTBIG);
void           us_explorevpan(WINDOWPART*, INTBIG);
void           us_explorerreadoutinit(WINDOWPART *w);
CHAR          *us_explorerreadoutnext(WINDOWPART *w, INTBIG *indent, INTBIG *style, BOOLEAN **donelist, void **en);
void           us_exploreredisphandler(WINDOWPART*);
int            us_exportnameindexascending(const void *e1, const void *e2);
int            us_exportnametypeascending(const void *e1, const void *e2);
BOOLEAN        us_celledithandler(WINDOWPART*, INTSML, INTBIG);
BOOLEAN        us_cellfromtech(NODEPROTO *cell, TECHNOLOGY *tech);
BOOLEAN        us_figuredrawpath(GEOM *fromgeom, PORTPROTO *fromport, GEOM *togeom, PORTPROTO *toport,
				INTBIG *xcur, INTBIG *ycur);
void           us_figuregrabpoint(UINTBIG*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_figuretechopaque(TECHNOLOGY*);
void           us_figuretechselectability(void);
void           us_figurevariableplace(UINTBIG*, INTBIG, CHAR*[]);
INTBIG         us_fillcomcomp(USERCOM*, COMCOMP*[]);
INTBIG         us_findboundkey(INTSML key, INTBIG special, CHAR **binding);
void           us_findlowerport(NODEINST**, PORTPROTO**);
void           us_findnamedmenu(CHAR *name, INTBIG *key, INTBIG *item);
void           us_findobject(INTBIG, INTBIG, WINDOWPART*, HIGHLIGHT*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_findexistingparameter(NODEINST *ni, INTBIG key, UINTBIG *textdescript);
VARIABLE      *us_findparametersource(VARIABLE *var, NODEINST *ni);
void           us_forceeditchanges(void);
void           us_freebindingparse(COMMANDBINDING *commandbinding);
void           us_freecomekmemory(void);
void           us_freecomtvmemory(void);
void           us_freediacommemory(void);
void           us_freeedemacsmemory(void);
void           us_freeeditor(EDITOR*);
void           us_freeedpacmemory(void);
void           us_freeedtecpmemory(void);
void           us_freehighmemory(void);
void           us_freemiscmemory(void);
void           us_freenetmemory(void);
void           us_freeparsememory(void);
void           us_freewindowmemory(void);
void           us_freestatusmemory(void);
void           us_freetranslatememory(void);
void           us_freeusercom(USERCOM*);
void           us_fullview(NODEPROTO*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
void           us_geomhaschanged(GEOM *geom);
BOOLEAN        us_geominrect(GEOM *geom, INTBIG slx, INTBIG shx, INTBIG sly, INTBIG shy);
INTBIG        *us_getallhighlighted(void);
void           us_getarcbounds(ARCINST *ai, INTBIG *nlx, INTBIG *nhx, INTBIG *nly, INTBIG *nhy);
NODEPROTO     *us_getareabounds(INTBIG*, INTBIG*, INTBIG*, INTBIG*);
CHAR          *us_getboundkey(CHAR *binding, INTSML *boundkey, INTBIG *boundspecial);
GEOM          *us_getclosest(INTBIG, INTBIG, INTBIG, NODEPROTO*);
void           us_getcolormap(TECHNOLOGY*, INTBIG, BOOLEAN);
INTBIG         us_getcolormapentry(CHAR*, BOOLEAN);
COMCOMP       *us_getcomcompfromkeyword(CHAR *keyword);
void           us_gethighaddrtype(HIGHLIGHT *high, INTBIG *addr, INTBIG *type);
void           us_gethighdescript(HIGHLIGHT *high, UINTBIG *descript);
GEOM         **us_gethighlighted(INTBIG type, INTBIG *textcount, CHAR ***textinfo);
CHAR          *us_gethighstring(HIGHLIGHT *high);
void           us_gethightextcenter(HIGHLIGHT *high, INTBIG*, INTBIG*, INTBIG*);
void           us_gethightextsize(HIGHLIGHT *high, INTBIG *xw, INTBIG *yw, WINDOWPART *w);
void           us_getlenwidoffset(NODEINST *ni, UINTBIG *descript, INTBIG *xoff, INTBIG *yoff);
void           us_getlowleft(NODEINST*, INTBIG*, INTBIG*);
VARIABLE      *us_getmacro(CHAR*);
MACROPACK     *us_getmacropack(CHAR*);
void           us_getnewparameterpos(NODEPROTO *np, INTBIG *xoff, INTBIG *yoff);
void           us_getnodebounds(NODEINST *ni, INTBIG *nlx, INTBIG *nhx, INTBIG *nly, INTBIG *nhy);
void           us_getnodedisplayposition(NODEINST *ni, INTBIG *xpos, INTBIG *ypos);
NODEINST      *us_getnodeonarcinst(GEOM**, PORTPROTO**, GEOM*, PORTPROTO*, INTBIG, INTBIG, INTBIG);
BOOLEAN        us_getnodetext(NODEINST*, WINDOWPART*, POLYGON*, VARIABLE**, VARIABLE**, PORTPROTO**);
UINTBIG        us_getobject(INTBIG, BOOLEAN);
TECHNOLOGY    *us_getobjectinfo(INTBIG addr, INTBIG type, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
HIGHLIGHT     *us_getonehighlight(void);
BOOLEAN        us_getonesnappoint(INTBIG *x, INTBIG *y);
INTBIG         us_getplacementangle(NODEPROTO *np);
POPUPMENU     *us_getpopupmenu(CHAR*);
INTBIG        *us_getprintcolors(TECHNOLOGY *tech);
void           us_getquickkeylist(INTBIG *quickkeycount, CHAR ***quickkeylist);
void           us_getslide(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG*, INTBIG*);
void           us_getsnappoint(HIGHLIGHT*, INTBIG*, INTBIG*);
INTBIG         us_gettextposition(CHAR*);
void           us_gettextscreensize(CHAR *str, UINTBIG *descript, WINDOWPART *w,
				TECHNOLOGY *tech, GEOM *geom, INTBIG *xw, INTBIG *yw);
INTBIG         us_gettextsize(CHAR*, INTBIG);
void           us_gettruewindowbounds(WINDOWPART *w, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy);
BOOLEAN        us_gettwoobjects(GEOM**, PORTPROTO**, GEOM**, PORTPROTO**);
BOOLEAN        us_getvar(CHAR*, INTBIG*, INTBIG*, CHAR**, BOOLEAN*, INTBIG*);
void           us_graphcells(NODEPROTO *top);
void           us_gridset(WINDOWPART*, INTBIG);
BOOLEAN        us_hasotherwindowpart(INTBIG x, INTBIG y, WINDOWPART *ow);
void           us_highlighteverywhere(GEOM*, PORTPROTO*, INTBIG, BOOLEAN, INTBIG, BOOLEAN);
void           us_highlighthaschanged(void);
void           us_highlightmenu(INTBIG, INTBIG, INTBIG);
void           us_highlightwindow(WINDOWPART*, BOOLEAN);
TECHNOLOGY    *us_hightech(HIGHLIGHT *high);
NODEPROTO     *us_highcell(HIGHLIGHT *high);
void           us_hsvtorgb(float, float, float, INTBIG*, INTBIG*, INTBIG*);
void           us_identifyports(INTBIG, NODEINST**, NODEPROTO*, INTBIG, BOOLEAN);
void           us_illustratecommandset(void);
void           us_inheritattributes(NODEINST *ni);
BOOLEAN        us_initialbinding(void);
void           us_initlayerletters(void);
void           us_initnodetext(NODEINST*, INTBIG, WINDOWPART*);
void           us_initqueuedexports(void);
void           us_initsearchcircuittext(WINDOWPART *win);
void           us_initstatus(void);
CHAR          *us_invisiblepintextname(VARIABLE *var);
BOOLEAN        us_islasteval(INTSML, INTBIG);
void           us_killcurrentwindow(BOOLEAN);
void           us_killwindowpickanother(WINDOWPART*);
CHAR          *us_layerletters(TECHNOLOGY*, INTBIG);
void           us_layouttext(CHAR *layer, INTBIG tsize, INTBIG scale, INTBIG font,
					INTBIG italic, INTBIG bold, INTBIG underline, INTBIG separation, CHAR *msg);
int            us_librarytemp1ascending(const void *e1, const void *e2);
INTBIG         us_makearcuserbits(ARCPROTO*);
USERCOM       *us_makecommand(CHAR*);
ARCINST       *us_makeconnection(NODEINST*, PORTPROTO*, ARCPROTO*, NODEINST*, PORTPROTO*,
				ARCPROTO*, NODEPROTO*, INTBIG, INTBIG,
				ARCINST**, ARCINST**, NODEINST**, NODEINST**,
				INTBIG, BOOLEAN, INTBIG*, BOOLEAN);
CHAR          *us_makecellline(NODEPROTO*, INTBIG);
BOOLEAN        us_makehighlight(CHAR*, HIGHLIGHT*);
CHAR          *us_makehighlightstring(HIGHLIGHT*);
NODEPROTO     *us_makeiconcell(PORTPROTO*, CHAR*, CHAR*, LIBRARY*);
PORTPROTO     *us_makenewportproto(NODEPROTO*, NODEINST*, PORTPROTO*, CHAR*, INTBIG, INTBIG, UINTBIG*);
BOOLEAN        us_makescreen(INTBIG*, INTBIG*, INTBIG*, INTBIG*, WINDOWPART*);
void           us_maketextpoly(CHAR*, WINDOWPART*, INTBIG, INTBIG, NODEINST*, TECHNOLOGY*, UINTBIG*, POLYGON*);
void           us_manymove(GEOM**, NODEINST**, INTBIG, INTBIG, INTBIG);
BOOLEAN        us_matchpurecommand(CHAR *command, CHAR *purecommand);
INTBIG         us_modarcbits(INTBIG, BOOLEAN, CHAR*, GEOM**);
void           us_modifytextdescript(HIGHLIGHT*, UINTBIG*);
void           us_moveselectedtext(INTBIG numtexts, CHAR **textlist, GEOM**, INTBIG odx, INTBIG ody);
NODEPROTO     *us_needcell(void);
BOOLEAN        us_needwindow(void);
void           us_nettravel(NODEINST*, INTBIG);
MACROPACK     *us_newmacropack(CHAR*);
NODEPROTO     *us_newnodeproto(CHAR *name, LIBRARY *lib);
CHAR          *us_nextvars(void);
BOOLEAN        us_nodemoveswithtext(HIGHLIGHT *high);
NODEPROTO     *us_nodetocreate(BOOLEAN getcontents, NODEPROTO *cell);
void           us_nulldisplayroutine(POLYGON*, WINDOWPART*);
void           us_parsebinding(CHAR*, COMMANDBINDING*);
INTBIG         us_parsecommand(CHAR*, CHAR*[]);
void           us_pasteobjects(WINDOWPART*);
NODEINST      *us_pickhigherinstance(NODEPROTO *inp);
CHAR          *us_plugincommand(INTBIG menukey, INTBIG menuitem, CHAR *command);
void           us_pophighlight(BOOLEAN);
POPUPMENUITEM *us_popupmenu(POPUPMENU**, BOOLEAN*, BOOLEAN, INTBIG, INTBIG, INTBIG);
PORTPROTO     *us_portdetails(PORTPROTO*, INTBIG, CHAR*[], NODEINST*, INTBIG*, INTBIG*, BOOLEAN, CHAR*, CHAR**);
void           us_portposition(NODEINST *ni, PORTPROTO *pp, INTBIG *x, INTBIG *y);
void           us_portsynchronize(LIBRARY*);
BOOLEAN        us_preventloss(LIBRARY*, INTBIG, BOOLEAN);
void           us_printarctoolinfo(ARCINST*);
void           us_printcolorvalue(INTBIG);
void           us_printmacro(VARIABLE*);
void           us_printmacros(void);
void           us_printnodetoolinfo(NODEINST*);
void           us_printtechnology(TECHNOLOGY*);
void           us_pushhighlight(void);
CHAR          *us_putintoinfstr(CHAR *str);
void           us_queueexport(PORTPROTO *pp);
BOOLEAN        us_queuenewexport(NODEINST *ni, PORTPROTO *pp, PORTPROTO *origpp);
void           us_queueopaque(GEOM*, BOOLEAN);
void           us_queueredraw(GEOM*, BOOLEAN);
void           us_recursivelyreplacemenu(POPUPMENU *pm, POPUPMENU *oldpm, POPUPMENU *newpm);
void           us_recursivemark(NODEPROTO *np);
void           us_redisplay(WINDOWPART*);
void           us_redisplaynow(WINDOWPART*, BOOLEAN);
void           us_redoexplorerwindow(void);
void           us_redostatus(WINDOWFRAME*);
BOOLEAN        us_reexportport(PORTPROTO*, NODEINST*);
void           us_regridselected(void);
CHAR          *us_removeampersand(CHAR *name);
void           us_removeubchange(NODEPROTO*);
void           us_renameport(PORTPROTO*, CHAR*);
void           us_renametecedcell(NODEPROTO *np, CHAR *oldname);
void           us_replaceallarcs(NODEPROTO *cell, GEOM **list, ARCPROTO *ap, BOOLEAN connected, BOOLEAN thiscell);
void           us_replacecircuittext(WINDOWPART *win, CHAR *replace);
void           us_replacelibraryreferences(LIBRARY *oldlib, LIBRARY *newlib);
NODEINST      *us_replacenodeinst(NODEINST *oldni, NODEPROTO *newnp, BOOLEAN ignoreportnames, BOOLEAN allowdeletedports);
void           us_reportarcscreated(CHAR *who, INTBIG numarcs1, CHAR *type1, INTBIG numarcs2, CHAR *type2);
void           us_rgbtohsv(INTBIG, INTBIG, INTBIG, float*, float*, float*);
void           us_rotatedescript(GEOM*, UINTBIG*);
void           us_rotatedescriptI(GEOM*, UINTBIG*);
ARCINST       *us_runarcinst(NODEINST*, PORTPROTO*, INTBIG, INTBIG, NODEINST*, PORTPROTO*, INTBIG, INTBIG, ARCPROTO*, INTBIG);
BOOLEAN        us_samecontents(NODEPROTO *np1, NODEPROTO *np2, INTBIG explain);
BOOLEAN        us_samekey(INTSML key1, INTBIG special1, INTSML key2, INTBIG special2);
BOOLEAN        us_saveeverything(void);
BOOLEAN        us_saveoptdlog(BOOLEAN atquit);
void           us_saveoptions(void);
void           us_scaletowindow(INTBIG*, INTBIG*, WINDOWPART*);
void           us_scaletraceinfo(NODEINST*, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_scanforkeyequiv(POPUPMENU*);
void           us_searchcircuittext(WINDOWPART *win, CHAR *search, CHAR *replaceall, INTBIG bits);
INTBIG         us_selectarea(NODEPROTO *np, INTBIG slx, INTBIG shx, INTBIG sly, INTBIG shy,
				INTBIG special, INTBIG selport, INTBIG nobbox, CHAR ***highlist);
void           us_selectsnap(HIGHLIGHT*, INTBIG, INTBIG);
void           us_settool(INTBIG, CHAR*[], INTBIG);
void           us_setalignment(WINDOWFRAME*);
void           us_setarcname(ARCINST*, CHAR*);
void           us_setarcproto(ARCPROTO*, BOOLEAN);
void           us_setcolorentry(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, BOOLEAN);
void           us_setcursorpos(WINDOWFRAME*, INTBIG, INTBIG);
void           us_setdescriptoffset(UINTBIG*, INTBIG, INTBIG);
void           us_setessentialbounds(NODEPROTO *cell);
void           us_setcellname(WINDOWPART *w);
void           us_setcellsize(WINDOWPART *w);
void           us_setfind(HIGHLIGHT*, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_setfloatvariablevalue(GEOM *geom, INTBIG key, VARIABLE *oldvar, float value);
void           us_setgridsize(WINDOWPART *w);
void           us_sethighlight(HIGHLIGHT*, INTBIG);
void           us_setkeybinding(CHAR *binding, INTSML key, INTBIG special, BOOLEAN quietly);
void           us_setkeyequiv(POPUPMENU*, INTBIG);
void           us_setlambda(WINDOWFRAME*);
void           us_setmenunodearcs(void);
void           us_setmenusize(INTBIG, INTBIG, INTBIG, BOOLEAN);
void           us_setmultiplehighlight(CHAR*, BOOLEAN);
void           us_setnodeangle(WINDOWFRAME*);
void           us_setnodeproto(NODEPROTO*);
void           us_setnodeprotocenter(INTBIG, INTBIG, NODEPROTO*);
NODEPROTO     *us_setscrolltocurrentcell(INTBIG scrollitem, BOOLEAN curinstance, BOOLEAN curcell,
					BOOLEAN forcecurlib, BOOLEAN justcells, void *dia);
void           us_setselectioncount(void);
void           us_settechname(WINDOWFRAME*);
void           us_settrace(NODEINST*, INTBIG*, INTBIG);
INTBIG         us_setunexpand(NODEINST*, INTBIG);
void           us_setvariablevalue(GEOM *geom, INTBIG key, CHAR *newvalue, INTBIG oldtype, UINTBIG *descript);
void           us_setwindowmode(WINDOWPART*, INTBIG oldstate, INTBIG newstate);
void           us_setxarray(INTBIG addr, INTBIG type, CHAR *name, XARRAY array);
BOOLEAN        us_setxy(INTBIG, INTBIG);
void           us_shadowarcproto(WINDOWFRAME*, ARCPROTO*);
void           us_shadownodeproto(WINDOWFRAME*, NODEPROTO*);
void           us_showallhighlight(void);
void           us_showcurrentarcproto(void);
void           us_showcurrentnodeproto(void);
void           us_showlayercoverage(NODEPROTO *cell);
void           us_showpoly(POLYGON*, WINDOWPART*);
void           us_showwindowmode(WINDOWPART *w);
NODEPROTO     *us_skeletonize(NODEPROTO *np, CHAR *newname, LIBRARY *newlib, BOOLEAN quiet);
void           us_slideleft(INTBIG);
void           us_slideup(INTBIG);
int            us_sortarcsbyname(const void *e1, const void *e2);
NODEPROTO    **us_sortlib(LIBRARY*, CHAR*);
int            us_sortnodesbyname(const void *e1, const void *e2);
int            us_sortpopupmenuascending(const void *e1, const void *e2);
WINDOWPART    *us_splitcurrentwindow(INTBIG, BOOLEAN, WINDOWPART**, INTBIG);
INTBIG         us_spreadaround(NODEINST *ni, INTBIG amount, CHAR *direction);
void           us_spreadrotateconnection(NODEINST *theni);
void           us_squarescreen(WINDOWPART*, WINDOWPART*, BOOLEAN, INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG);
INTBIG         us_stretchtonodes(ARCPROTO *typ, INTBIG wid, NODEINST *ni1, PORTPROTO *pp1, INTBIG otherx, INTBIG othery);
CHAR          *us_stripampersand(CHAR*);
WINDOWPART    *us_subwindow(INTBIG, INTBIG, INTBIG, INTBIG, WINDOWPART*);
void           us_switchtocell(NODEPROTO*, INTBIG, INTBIG, INTBIG, INTBIG, NODEINST*, PORTPROTO*, BOOLEAN, BOOLEAN, BOOLEAN);
void           us_switchtolibrary(LIBRARY*);
void           us_tecedaddfunstring(void*, INTBIG);
void           us_teceddeletelayercell(NODEPROTO *np);
void           us_teceddeletenodecell(NODEPROTO *np);
void           us_tecedentry(INTBIG, CHAR*[]);
INTBIG         us_tecedgetoption(NODEINST*);
void           us_tecedrenamecell(CHAR *oldname, CHAR *newname);
CHAR          *us_teceddescribenode(NODEINST *ni);
CHAR          *us_techname(NODEPROTO *np);
CHAR          *us_tempoptionslibraryname(void);
void           us_textcellchanges(WINDOWPART*, INTBIG, CHAR*, CHAR*, INTBIG);
void           us_toolturnedon(TOOL *tool);
BOOLEAN        us_topofallthings(CHAR**);
BOOLEAN        us_topofarcnodes(CHAR**);
BOOLEAN        us_topofcells(CHAR**);
BOOLEAN        us_topofcommands(CHAR**);
BOOLEAN        us_topofconstraints(CHAR**);
BOOLEAN        us_topofcports(CHAR**);
BOOLEAN        us_topofedtecarc(CHAR**);
BOOLEAN        us_topofedteclay(CHAR**);
BOOLEAN        us_topofedtecnode(CHAR**);
BOOLEAN        us_topofexpports(CHAR**);
BOOLEAN        us_topofhighlight(CHAR**);
BOOLEAN        us_topoflayers(CHAR**);
BOOLEAN        us_topofmacros(CHAR**);
BOOLEAN        us_topofmbuttons(CHAR**);
BOOLEAN        us_topofnodes(CHAR**);
BOOLEAN        us_topofpopupmenu(CHAR**);
BOOLEAN        us_topofports(CHAR**);
BOOLEAN        us_topofprims(CHAR**);
BOOLEAN        us_topofvars(CHAR**);
BOOLEAN        us_topofwindows(CHAR**);
void           us_translationdlog(void);
CHAR          *us_trueattributename(VARIABLE *var);
void           us_undisplayobject(GEOM*);
void           us_undoportproto(NODEINST*, PORTPROTO*);
void           us_undrawcellvariable(VARIABLE *var, NODEPROTO *np);
CHAR          *us_uniqueobjectname(CHAR *name, NODEPROTO *cell, INTBIG type, void *exclude);
CHAR          *us_uniqueportname(CHAR*, NODEPROTO*);
void           us_unknowncommand(void);
void           us_unqueueredraw(LIBRARY *lib);
void           us_validatemenu(POPUPMENU *pm);
BOOLEAN        us_validname(CHAR *name, INTBIG type);
void           us_varchanges(WINDOWPART*, INTBIG, CHAR*, CHAR*, INTBIG);
CHAR          *us_variableattributes(VARIABLE*, INTBIG);
CHAR          *us_variabletypename(INTBIG);
INTBIG         us_variabletypevalue(CHAR*);
CHAR          *us_variableunits(VARIABLE *var);
WINDOWPART    *us_wantnewwindow(INTBIG orientation);
void           us_wanttodraw(INTBIG, INTBIG, INTBIG, INTBIG, WINDOWPART*, GRAPHICS*, INTBIG);
void           us_wanttoread(CHAR*);
INTBIG         us_widestarcinst(ARCPROTO*, NODEINST*, PORTPROTO*);
BOOLEAN        us_windowcansplit(WINDOWPART*, WINDOWPART*, WINDOWPART*);
void           us_windowfit(WINDOWFRAME*, BOOLEAN, INTBIG);
BOOLEAN        us_windowgetsexploericon(WINDOWPART *w);
void           us_writeprotoname(PORTPROTO*, INTBIG, XARRAY, INTBIG, INTBIG, WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void           us_writetext(INTBIG, INTBIG, INTBIG, INTBIG, GRAPHICS*, CHAR*, UINTBIG*, WINDOWPART*, TECHNOLOGY*);
void           us_yankonenode(NODEINST*);

/* terminal interface */
void           us_ttyprint(BOOLEAN, CHAR*, va_list);

/* the status bar */
STATUSFIELD   *ttydeclarestatusfield(INTBIG line, INTBIG startper, INTBIG endper, CHAR *label);
void           ttyfreestatusfield(STATUSFIELD *field);
void           ttysetstatusfield(WINDOWFRAME *frame, STATUSFIELD *field, CHAR *message, BOOLEAN cancrop);
INTBIG         ttynumstatuslines(void);

/* popup and pulldown menus */
void           nativemenuload(INTBIG, CHAR*[]);
void           nativemenurename(POPUPMENU*, INTBIG);
INTBIG         nativepopupmenu(POPUPMENU **menu, BOOLEAN header, INTBIG left, INTBIG top);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
