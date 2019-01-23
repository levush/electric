/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simwindow.c
 * Simulation tool: signal window control
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

#include "config.h"
#if SIMTOOL

#include "global.h"
#include "egraphics.h"
#include "edialogs.h"
#include "efunction.h"
#include "network.h"
#include "sim.h"
#include "simals.h"
#include "tecgen.h"
#include "tecschem.h"
#include "tecart.h"
#include "usr.h"
#include "usrtrack.h"
#include <math.h>
#include <float.h>

/*
 *     WINDOW CONTROL:
 * sim_window_init()                            Initialize simulation window system (do only once)
 * BOOLEAN sim_window_create(f, np, chw, chs, fmt) Make simulation window with "f" frames, format "fmt"
 *                                              in cell "np" charhandlers "chw/s" (l=0 to restore)
 * sim_window_titleinfo(info)                   adds "info" to title of simulation window
 * void sim_window_stopsimulation()             Stops simulation
 * INTBIG sim_window_isactive(np)               Returns nonzero if simulating, returns cell if so
 * sim_window_redraw()                          Redraws simulation window
 * INTBIG sim_window_getnumframes()             Returns number of frames in sim window
 * sim_window_setnumframes(count)               Sets number of frames in simulation window
 * INTBIG sim_window_getnumvisframes()          Returns number of visible frames in sim window
 * INTBIG sim_window_gettopvisframe()           Returns topmost visible frame in simulation window
 * sim_window_settopvisframe(count)             Sets topmost visible frame in simulation window
 * sim_window_savegraph()                       Preserves plot in database
 * sim_window_writespicecmd()                   Writes vectors as SPICE commands
 * c = sim_window_getdisplaycolor(s)            Get color of strength/state "s"
 * sim_window_setdisplaycolor(s, c)             Set color of strength/state "s" to "c"
 *
 *     FRAME CONTROL
 * sim_window_auto_anarange(void)               Zooms all frames to fit their traces (TODO: all->frameno)
 * sim_window_zoom_frame(INTBIG frameno)        Zooms In frame "frameno" by scale 2
 * sim_window_zoomout_frame(INTBIG frameno)     Zooms Out frame "frameno" by scale 2
 * sim_window_shiftup_frame(INTBIG frameno)     Shifts Up frame "frameno" by 1/4 of height
 * sim_window_shiftdown_frame(INTBIG frameno)   Shifts Dowm frame "frameno" by 1/4 of height
 * sim_window_renumberlines()                   Renumbers frames so that they have no gaps
 *
 *     CONTROL OF TRACES:
 * INTBIG sim_window_newtrace(frame, name, data) Creates trace in "frame" called "name" with user "data"
 * BOOLEAN sim_window_buscommand()              Creates a bus from currently selected signals
 * sim_window_loaddigtrace(tr, num, time, sta)  Loads trace "tr" with "num" digital signals "time/sta"
 * sim_window_loadanatrace(tr, num, time, val)  Loads trace "tr" with "num" analog signals "time/val"
 * sim_window_settraceframe(tr, frame)          Sets frame number for trace "tr"
 * sim_window_killtrace(tr)                     Deletes trace "tr"
 * sim_window_killalltraces(save)               Deletes all traces
 * sim_window_supresstraceprefix(prefix)		Ignore this prefix when displaying trace names
 *
 *     TRACE INFORMATION:
 * INTBIG *sim_window_getbustraces(tr)          Returns 0-terminated list of traces in bus "tr"
 * INTBIG sim_window_gettraceframe(tr)          Returns frame number for trace "tr"
 * INTBIG sim_window_getcurframe()              Returns current frame number
 * char *sim_window_gettracename(tr)            Returns name for trace "tr"
 * INTBIG sim_window_gettracedata(tr)           Returns user data for trace "tr"
 * float sim_window_getanatracevalue(tr, time)  Returns analog value for trace "tr" at time "time"
 * sim_window_getdigtracevalue(tr, time, &lev, &str) Returns digital value for trace "tr" at time "time"
 * CHAR *sim_window_getbustracevalue(tr, time)	Returns bus value for trace "tr" at time "time"
 * INTBIG *sim_window_findtrace(name, &count)   Locates the traces with this name
 * sim_window_inittraceloop()                   Begin search of all traces
 * sim_window_inittraceloop2()                  Begin search of all traces
 * INTBIG sim_window_nexttraceloop()            Returns next trace (0 when done)
 * INTBIG sim_window_nexttraceloop2()           Returns next trace (0 when done)
 *
 *     TRACE HIGHLIGHTING:
 * sim_window_cleartracehighlight()             Unhighlights all traces
 * sim_window_addhighlighttrace(tr)             Adds trace "tr" to highlighting
 * sim_window_deletehighlighttrace(tr)          Removes trace "tr" from highlighting
 * INTBIG sim_window_gethighlighttrace()        Returns highlighted trace (0 if none)
 * INTBIG *sim_window_gethighlighttraces()      Returns 0-terminated list of highlighted traces
 * sim_window_showhighlightedtraces()           Makes sure highlighted traces are seen
 *
 *     TIME AND CURSOR CONTROL:
 * sim_window_setmaincursor(time)               Sets position of main cursor
 * double sim_window_getmaincursor()            Returns position of main cursor
 * sim_window_setextensioncursor(time)          Sets position of extension cursor
 * double sim_window_getextensioncursor()       Returns position of extension cursor
 * sim_window_settimerange(tr, min, max)        Sets trace time range from "min" to "max"
 * sim_window_gettimerange(tr, min, max)        Returns trace time range
 * sim_window_gettimeextents(min, max)          Returns data time range
 */

#define BUSANGLE          4		/* defines look of busses */

#define NOTRACE ((TRACE *)-1)

/* the meaning of TRACE->flags */
#define TRACESELECTED     01		/* set if this trace is selected */
#define TRACEMARK         02		/* for temporarily marking a trace */
#define TRACETYPE        014		/* the type of this trace */
#define TRACEISDIGITAL     0		/*     this trace is a digital signal */
#define TRACEISANALOG     04		/*     this trace is an analog signal */
#define TRACEISBUS       010		/*     this trace is a bus */
#define TRACEISOPENBUS   014		/*     this trace is an opened bus */
#define TRACENOTDRAWN    020		/* set if trace is not to be drawn */
#define TRACEHIGHCHANGED 040		/* set if trace highlighting changed */

typedef struct Itrace
{
	CHAR     *name;				/* name of this trace */
	CHAR     *origname;			/* original name of this trace */
	INTBIG    flags;			/* state bits for this trace (see above) */
	INTBIG    nodeptr;			/* "user" data for this trace */
	INTBIG    busindex;			/* index of this trace in the bus (when part of a bus) */
	INTBIG    color;			/* color of this trace (analog only) */
	INTBIG    nameoff;			/* offset of trace name (when many per frame) */
	INTBIG    frameno;			/* frame number of this trace in window */
	INTBIG    numsteps;			/* number of steps along this trace */
	INTBIG    timelimit;		/* maximum times allocated for this trace */
	INTBIG    statelimit;		/* maximum states allocated for this trace */
	INTBIG    valuelimit;		/* maximum values allocated for this trace */
	double   *timearray;		/* time steps along this trace */
	INTBIG   *statearray;		/* states along this trace (digital) */
	float    *valuearray;		/* values along this trace (analog) */
	double    mintime, maxtime;	/* displayed range along horozintal axis */
	float     anadislow;		/* low vertical axis for this trace (analog) */
	float     anadishigh;		/* high vertical axis for this trace (analog) */
	float     anadisrange;		/* vertical range for this trace (analog) */
	struct Itrace *buschannel;
	struct Itrace *nexttrace;
} TRACE;

static GRAPHICS sim_window_desc = {LAYERA, ALLOFF, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};
static GRAPHICS sim_window_udesc = {LAYERA, ALLOFF, PATTERNED, PATTERNED,
	{0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,
	0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555}, NOVARIABLE, 0};
static INTBIG sim_window_anacolor[] = {RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW, ORANGE,
									   PURPLE, LGRAY, LRED, LGREEN, LBLUE, 0};

#define DEFWAVEXNAMEEND     112		/* default x position where signal names end */
#define DEFWAVEXNAMEENDANA  100		/* default X position where signal names end (if analog) */
#define DEFWAVEXBAR         113		/* default X position of the bar between names and waveforms */
#define DEFWAVEXSIGSTART    114		/* default X position where waveforms start */
#define WAVEXSIGEND         767		/* the X position where waveforms end */

#define WAVEYTIMETOP      30
#define WAVEYSIGBOT       31
#define WAVEYSIGTOP      560
#define WAVEYSIGRANGE    (WAVEYSIGTOP-WAVEYSIGBOT)		/* was 540 */
#define WAVEYSIGGAP       10
#define WAVEYCURSTOP     571
#define WAVEYTEXTBOT     561
#define WAVEYTEXTTOP     600

#define TOPBARSIZE        20		/* size of title bar in waveform window */

#define MINTHUMB          25		/* minimum horizontal thumb size */

/* still need to convert "sim_window_drawcursors()" */

/*
 * In X:
 *    0 to 112 is signal name
 *      113    is bar
 *   114-767 is signal waveform
 * In Y:
 *    0 to  30 is axis label
 *   31 to 560 is signal waveforms
 *   31 to 571 is the cursors
 *  561 to 600 is text values (main and extension coordinates)
 */
static INTBIG        sim_window_curx, sim_window_cury;
static INTBIG        sim_window_textsize;
static INTBIG        sim_window_lines = 0;
static INTBIG        sim_window_lines_limit = 0;
static INTBIG        sim_window_visframes;		/* frames (or lines) visible in window */
static INTBIG        sim_window_topframes;		/* upper visible frame in window */
static INTBIG        sim_window_highframe;		/* highlighted frame in window */
static INTBIG        sim_window_txthei;
static INTBIG        sim_window_curanacolor;
static INTBIG        sim_window_offx, sim_window_offy;
static INTBIG        sim_window_basex, sim_window_basey;
       INTBIG        sim_window_statekey;		/* key for "SIM_window_state" */
       INTBIG        sim_window_state;			/* cached value of "SIM_window_state" */
       INTBIG        sim_window_format;			/* type of simulation in window */
       INTBIG        sim_window_signalorder_key;/* key for "SIM_window_signalorder" */
       INTBIG        sim_window_hierpos_key;	/* key for "SIM_window_hierarchy_pos" */
static double        sim_window_maincursor, sim_window_extensioncursor;
static float         sim_window_analow, sim_window_anahigh, sim_window_anarange;
static float        *sim_window_frame_analow=NULL, *sim_window_frame_anahigh=NULL;
static float        *sim_window_frame_topval=NULL, *sim_window_frame_botval=NULL;
static INTBIG        sim_window_highlightedtracecount;
static INTBIG        sim_window_highlightedtracetotal = 0;
static TRACE       **sim_window_highlightedtraces;
static INTBIG        sim_window_bustracetotal = 0;
static TRACE       **sim_window_bustraces;
static TRACE        *sim_window_firstrace;
static TRACE        *sim_window_tracefree;
static TRACE        *sim_window_loop;
static TRACE        *sim_window_loop2;
       INTBIG        sim_colorstrengthoff;		/* color of off-strength */
       INTBIG        sim_colorstrengthnode;		/* color of node-strength */
       INTBIG        sim_colorstrengthgate;		/* color of gate-strength */
       INTBIG        sim_colorstrengthpower;	/* color of power-strength */
       INTBIG        sim_colorlevellow;			/* color of low-levels */
       INTBIG        sim_colorlevelhigh;		/* color of high-levels */
       INTBIG        sim_colorlevelundef;		/* color of undefined levels */
       INTBIG        sim_colorlevelzdef;		/* color of Z levels */
       INTBIG        sim_colorstrengthoff_key;	/* key for "SIM_window_color_str0" */
       INTBIG        sim_colorstrengthnode_key;	/* key for "SIM_window_color_str1" */
       INTBIG        sim_colorstrengthgate_key;	/* key for "SIM_window_color_str2" */
       INTBIG        sim_colorstrengthpower_key;/* key for "SIM_window_color_str3" */
       INTBIG        sim_colorlevellow_key;		/* key for "SIM_window_color_low" */
       INTBIG        sim_colorlevelhigh_key;	/* key for "SIM_window_color_high" */
       INTBIG        sim_colorlevelundef_key;	/* key for "SIM_window_color_X" */
       INTBIG        sim_colorlevelzdef_key;	/* key for "SIM_window_color_Z" */
static WINDOWPART   *sim_waveformwindow;		/* current window when arrows are clicked */
static INTBIG        sim_waveformsliderpart;	/* 0:down arrow 1:below thumb 2:thumb 3:above thumb 4:up arrow */
static INTBIG        sim_window_tracedragyoff;	/* Y offset when dragging */
static INTBIG        sim_window_initialx;		/* initial X coordinate when dragging */
static INTBIG        sim_window_initialy;		/* initial Y coordinate when dragging */
static INTBIG        sim_window_currentdragy;	/* current Y coordinate when dragging */
static INTBIG        sim_window_deltasofar;		/* for dragging thumb button on side */
static INTBIG        sim_window_initialthumb;	/* for dragging thumb button on side */
static TRACE        *sim_window_tracedragtr;	/* signal being dragged */
static INTBIG        sim_window_sigtotal = 0;
static INTBIG       *sim_window_sigvalues;
static INTBIG        sim_window_zoominix, sim_window_zoominiy;	/* initial zoom coordinate */
static INTBIG        sim_window_zoomothx, sim_window_zoomothy;	/* final zoom coordinate */
static INTBIG        sim_waveforminiththumbpos;
static double        sim_waveformhrange;
static float         sim_window_maxtime, sim_window_mintime;
static float	     sim_window_max_timetick, sim_window_min_timetick;
static float	     sim_window_time_separation;
static float         sim_window_percent[] = { 0.1f, 0.5f, 0.9f };
#define PERCENTSIZE ((INTBIG)(sizeof(sim_window_percent)/sizeof(sim_window_percent[0])))
static float         sim_window_vdd;
static CHAR         *sim_window_title = 0;
static CHAR         *sim_window_traceprefix = 0;
static void         *sim_veroldstringarray = 0;	/* string array that holds a cell's former signal order */
static INTBIG        sim_window_wavexnameend;	/* the X position where signal names end */
static INTBIG        sim_window_wavexnameendana;/* the X position where signal names end (if analog) */
       INTBIG        sim_window_wavexbar;		/* the X position of the bar between names and waveforms */
static INTBIG        sim_window_wavexsigstart;	/* the X position where waveforms start */
static BOOLEAN       sim_window_dragshown;		/* TRUE if dragging divider between names and waveforms */
static BOOLEAN       sim_window_draghighoff;	/* TRUE if highlights are off during divider dragging */
static INTBIG        sim_window_draglastx;		/* last position of divider between names and waveforms */
static INTBIG        sim_window_playing;		/* state of VCR controls */
static UINTBIG       sim_window_lastadvance;	/* time the VCR last advanced */
static UINTBIG       sim_window_advancespeed;	/* speed of the VCR (in screen pixels) */
static INTBIG        sim_window_measureframe;	/* frame in which measurement is being done */
static INTBIG        sim_window_firstmeasurex;	/* starting screen X coordinate of measurement */
static INTBIG        sim_window_firstmeasurey;	/* starting screen Y coordinate of measurement */
static INTBIG        sim_window_lastmeasurex;	/* current screen X coordinate of measurement */
static INTBIG        sim_window_lastmeasurey;	/* current screen Y coordinate of measurement */
static double        sim_window_firstmeasuretime;	/* current time coordinate of measurement */
static double        sim_window_firstmeasureyval;	/* current value coordinate of measurement */
static POLYGON      *sim_window_dragpoly = NOPOLYGON;	/* polygon for displaying measurement */

/* the meaning of "sim_window_playing" */
#define VCRSTOP         0						/* VCR is stopped */
#define VCRPLAYFOREWARD 1						/* VCR is playing forward */
#define VCRPLAYBACKWARD 2						/* VCR is playing backward */

/* prototypes for local routines */
static void        sim_window_addsegment(WINDOWPART*, INTBIG*, INTBIG*, INTBIG, INTBIG, INTBIG, INTBIG);
static void        sim_window_addtrace(TRACE *tr, TRACE *aftertr);
static TRACE      *sim_window_alloctrace(void);
static void        sim_window_buttonhandler(WINDOWPART*, INTBIG, INTBIG, INTBIG);
static CHAR       *sim_windowconvertengineeringnotation2(double time, INTBIG precpower);
static BOOLEAN     sim_window_distancedown(INTBIG x, INTBIG y);
static void        sim_window_distanceinit(WINDOWPART *wavewin);
static void        sim_window_distanceup(void);
static void        sim_window_dividerdone(void);
static BOOLEAN     sim_window_dividerdown(INTBIG x, INTBIG y);
static void        sim_window_dividerinit(WINDOWPART *w);
static void        sim_window_donehighlightchange(WINDOWPART *wavewin);
static void        sim_window_draw_timeticks(WINDOWPART *wavewin, INTBIG y1, INTBIG y2, BOOLEAN grid);
static void        sim_window_drawdistance(void);
static void        sim_window_drawareazoom(WINDOWPART *w, BOOLEAN on, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty);
static void        sim_window_drawbox(WINDOWPART*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void        sim_window_drawcstring(WINDOWPART*, CHAR*);
static void        sim_window_drawcursors(WINDOWPART*);
static void        sim_window_drawframe(INTBIG frame, WINDOWPART *wavewin);
static void        sim_window_drawgraph(WINDOWPART*);
static void        sim_window_drawhcstring(WINDOWPART *w, CHAR *s);
static void        sim_window_drawtimescale(WINDOWPART*);
static void        sim_window_drawto(WINDOWPART*, INTBIG, INTBIG, INTBIG);
static void        sim_window_drawtracedrag(WINDOWPART*, BOOLEAN on);
static void        sim_window_drawtraceselect(WINDOWPART *wavewin, INTBIG y1, INTBIG y2, BOOLEAN on);
static void        sim_window_drawulstring(WINDOWPART *w, CHAR *s);
static BOOLEAN     sim_window_eachbdown(INTBIG, INTBIG);
static void        sim_window_eachcursordown(INTBIG x, INTBIG y,double *cursor);
static BOOLEAN     sim_window_eachdown(INTBIG x, INTBIG y);
static BOOLEAN     sim_window_eachwdown(INTBIG, INTBIG);
static INTBIG      sim_window_ensurespace(TRACE *tr);
static BOOLEAN     sim_window_findcross(TRACE *tr, double val, double *time);
static WINDOWPART *sim_window_findschematics(void);
static void        sim_window_findthistrace(CHAR *name);
static WINDOWPART *sim_window_findwaveform(void);
static float       sim_window_get_analow(int traceno);
static float       sim_window_get_anahigh(int traceno);
static float       sim_window_get_anarange(int traceno);
static float       sim_window_get_botval(int frameno);
static float       sim_window_get_extanarange(int frameno);
static float       sim_window_get_topval(int frameno);
static void        sim_window_gethighlightcolor(INTBIG state, INTBIG *texture, INTBIG *color);
static void        sim_window_highlightname(WINDOWPART *wavewin, TRACE *tr, BOOLEAN on);
static BOOLEAN     sim_window_horizontalslider(INTBIG x, INTBIG y);
static void        sim_window_hthumbtrackingcallback(INTBIG delta);
static void        sim_window_inithighlightchange(void);
static BOOLEAN     sim_window_lineseg(WINDOWPART *wavewin, INTBIG xf, INTBIG yf, INTBIG xt,
					INTBIG yt, INTBIG style, INTBIG x, INTBIG y, BOOLEAN thick);
static CHAR       *sim_window_makewidevalue(INTBIG count, INTBIG *value);
static void        sim_window_mapcoord(WINDOWPART*, INTBIG*, INTBIG*);
static void        sim_window_moveto(INTBIG, INTBIG);
static CHAR       *sim_window_namesignals(INTBIG count, TRACE **list);
static BOOLEAN     sim_window_plottrace(TRACE *tr, WINDOWPART *wavewin, INTBIG style, INTBIG x, INTBIG y);
static CHAR       *sim_window_prettyprint(float v, int i1, int i2);
static void        sim_window_putvalueontrace(TRACE *tr, NODEPROTO *simnt);
static void        sim_window_redisphandler(WINDOWPART*);
static void        sim_window_removetrace(TRACE *tr);
static void        sim_window_savesignalorder(void);
static void        sim_window_scaletowindow(WINDOWPART*, INTBIG*, INTBIG*);
static BOOLEAN     sim_window_seleachdown(INTBIG x, INTBIG y);
static float       sim_window_sensible_value(float *h, float *l, INTBIG *i1, INTBIG *i2, INTBIG n);
static double      sim_window_sensibletimevalue(double time);
static void        sim_window_set_anahigh(int frameno, float value);
static void        sim_window_set_analow(int frameno, float value);
static void        sim_window_set_botval(int frameno, float value);
static int         sim_window_set_frames(int n);
static void        sim_window_set_topval(int frameno, float value);
static void        sim_window_setcolor(INTBIG);
static void        sim_window_setmask(INTBIG);
static void        sim_window_setviewport(WINDOWPART*);
static void        sim_window_showtracebar(WINDOWPART *wavewin, INTBIG frame, INTBIG style);
static void        sim_window_termhandler(WINDOWPART *w);
static INTBIG      sim_window_timetoxpos(double);
static void        sim_window_trace_range(INTBIG tri) ;
static INTBIG      sim_window_valtoypos(INTBIG frameno, double val);
static BOOLEAN     sim_window_verticalslider(INTBIG x, INTBIG y);
static void        sim_window_vthumbtrackingcallback(INTBIG delta);
static void        sim_window_writetracename(WINDOWPART*, TRACE*);
static double      sim_window_xpostotime(INTBIG);
static INTBIG      sim_window_ypostoval(INTBIG ypos, double *val);
static BOOLEAN     sim_window_zoomdown(INTBIG, INTBIG);

/********************************* WINDOW CONTROL *********************************/

/*
 * routine to initialize the simulation window
 */
void sim_window_init(void)
{
	sim_window_signalorder_key = makekey(x_("SIM_window_signalorder"));
	sim_window_hierpos_key = makekey(x_("SIM_window_hierarchy_pos"));

	sim_window_statekey = makekey(x_("SIM_window_state"));
#if SIMTOOLIRSIM == 0
	sim_window_state = FULLSTATE | SHOWWAVEFORM;
#else
	sim_window_state = FULLSTATE | SHOWWAVEFORM | SIMENGINEIRSIM;
#endif
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_window_statekey,
		sim_window_state, VINTEGER|VDONTSAVE);

	sim_window_maincursor = 0.0000002f;
	sim_window_extensioncursor = 0.0000003f;
	(void)sim_window_set_frames(0) ;
	sim_window_visframes = 0;
	sim_window_topframes = 0;
	sim_window_highframe = -1;
	sim_window_firstrace = NOTRACE;
	sim_window_tracefree = NOTRACE;
	sim_window_highlightedtracecount = 0;
	sim_window_wavexnameend = DEFWAVEXNAMEEND;
	sim_window_wavexnameendana = DEFWAVEXNAMEENDANA;
	sim_window_wavexbar = DEFWAVEXBAR;
	sim_window_wavexsigstart = DEFWAVEXSIGSTART;
	sim_window_playing = VCRSTOP;
	sim_window_advancespeed = 2;

	/* setup simulation window colors */
	sim_colorstrengthoff = BLUE;
	sim_colorstrengthnode = GREEN;
	sim_colorstrengthgate = MAGENTA;
	sim_colorstrengthpower = BLACK;
	sim_colorlevellow = BLUE;
	sim_colorlevelhigh = MAGENTA;
	sim_colorlevelundef = BLACK;
	sim_colorlevelzdef = LGRAY;
	sim_colorstrengthoff_key = makekey(x_("SIM_window_color_str0"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthoff_key, sim_colorstrengthoff, VINTEGER|VDONTSAVE);
	sim_colorstrengthnode_key = makekey(x_("SIM_window_color_str1"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthnode_key, sim_colorstrengthnode, VINTEGER|VDONTSAVE);
	sim_colorstrengthgate_key = makekey(x_("SIM_window_color_str2"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthgate_key, sim_colorstrengthgate, VINTEGER|VDONTSAVE);
	sim_colorstrengthpower_key = makekey(x_("SIM_window_color_str3"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthpower_key, sim_colorstrengthpower, VINTEGER|VDONTSAVE);
	sim_colorlevellow_key = makekey(x_("SIM_window_color_low"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevellow_key, sim_colorlevellow, VINTEGER|VDONTSAVE);
	sim_colorlevelhigh_key = makekey(x_("SIM_window_color_high"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevelhigh_key, sim_colorlevelhigh, VINTEGER|VDONTSAVE);
	sim_colorlevelundef_key = makekey(x_("SIM_window_color_X"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevelundef_key, sim_colorlevelundef, VINTEGER|VDONTSAVE);
	sim_colorlevelzdef_key = makekey(x_("SIM_window_color_Z"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevelzdef_key, sim_colorlevelzdef, VINTEGER|VDONTSAVE);
}

/*
 * routine to terminate the simulation window module memory
 */
void sim_freewindowmemory(void)
{
	REGISTER TRACE *tr;

	sim_window_killalltraces(FALSE);

	while (sim_window_tracefree != NOTRACE)
	{
		tr = sim_window_tracefree;
		sim_window_tracefree = sim_window_tracefree->nexttrace;
		if (tr->timelimit > 0) efree((CHAR *)tr->timearray);
		if (tr->statelimit > 0) efree((CHAR *)tr->statearray);
		if (tr->valuelimit > 0) efree((CHAR *)tr->valuearray);
		efree((CHAR *)tr);
	}
	if (sim_window_highlightedtracetotal > 0) efree((CHAR *)sim_window_highlightedtraces);
	if (sim_window_bustracetotal > 0) efree((CHAR *)sim_window_bustraces);
	if (sim_window_sigtotal > 0)
	{
		efree((CHAR *)sim_window_sigvalues);
		sim_window_sigtotal = 0;
	}
	if (sim_window_lines_limit > 0)
	{
		efree((CHAR *)sim_window_frame_analow);
		efree((CHAR *)sim_window_frame_anahigh);
		efree((CHAR *)sim_window_frame_topval);
		efree((CHAR *)sim_window_frame_botval);
	}
	if (sim_window_title != 0) efree((CHAR *)sim_window_title);
	if (sim_window_traceprefix != 0) efree((CHAR *)sim_window_traceprefix);
	if (sim_veroldstringarray == 0)
		killstringarray(sim_veroldstringarray);
}

/*
 * routine to start a simulation window with room for "framecount" traces and associated cell "np".
 * The character handler to use in the waveform window is in "charhandlerwave", the character
 * handler to use in the schematic/layout window is in "charhandlerschem".  Returns true on error.
 */
BOOLEAN sim_window_create(INTBIG framecount, NODEPROTO *np, BOOLEAN (*charhandlerwave)(WINDOWPART*, INTSML, INTBIG),
	BOOLEAN (*charhandlerschem)(WINDOWPART*, INTSML, INTBIG), INTBIG format)
{
	REGISTER WINDOWPART *wavewin, *schemwin;
	REGISTER NODEPROTO *simnp;

	/* sorry, can't have more than 1 simulation window at a time */
	wavewin = sim_window_findwaveform();
	schemwin = sim_window_findschematics();
	simnp = NONODEPROTO;
	if (wavewin != NOWINDOWPART && (wavewin->state&WINDOWSIMMODE) != 0)
		simnp = wavewin->curnodeproto;
	if (schemwin != NOWINDOWPART) simnp = schemwin->curnodeproto;
	if (simnp != NONODEPROTO && simnp != np)
	{
		ttyputerr(x_("Sorry, already simulating cell %s"), describenodeproto(simnp));
		return(TRUE);
	}

	/* reinitialize waveform display if number of lines is specified */
	if (framecount > 0)
	{
		sim_window_killalltraces(TRUE);
		if (sim_window_set_frames(framecount) == 0) return TRUE;
		sim_window_topframes = 0;
	}

	/* save simulation format */
	sim_window_format = format;

	/* determine the number of visible signals */
	sim_window_visframes = sim_window_lines;
	sim_window_curanacolor = 0;
	sim_window_vdd = 0.0;

	/* remove title information */
	if (sim_window_title != 0) efree((CHAR *)sim_window_title);
	sim_window_title = 0;

	/* remove trace prefix information */
	if (sim_window_traceprefix != 0) efree((CHAR *)sim_window_traceprefix);
	sim_window_traceprefix = 0;

	/* create waveform window if requested */
	if (charhandlerwave != 0)
	{
		if (wavewin == NOWINDOWPART)
		{
			/* get window state information */
			switch (sim_window_state&WAVEPLACE)
			{
				case WAVEPLACECAS:
					wavewin = (WINDOWPART *)asktool(us_tool, x_("window-new"));
					break;
				case WAVEPLACETHOR:
					wavewin = (WINDOWPART *)asktool(us_tool, x_("window-horiz-new"));
					break;
				case WAVEPLACETVER:
					wavewin = (WINDOWPART *)asktool(us_tool, x_("window-vert-new"));
					break;
			}
			if (wavewin == NOWINDOWPART) return(TRUE);
		}
		sim_window_setviewport(wavewin);

		startobjectchange((INTBIG)wavewin, VWINDOWPART);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("state"), (wavewin->state & ~WINDOWTYPE) | WAVEFORMWINDOW |
			WINDOWSIMMODE, VINTEGER);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("curnodeproto"), (INTBIG)np, VNODEPROTO);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("buttonhandler"), (INTBIG)sim_window_buttonhandler, VADDRESS);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("charhandler"), (INTBIG)charhandlerwave, VADDRESS);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("redisphandler"), (INTBIG)sim_window_redisphandler, VADDRESS);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("termhandler"), (INTBIG)sim_window_termhandler, VADDRESS);
		endobjectchange((INTBIG)wavewin, VWINDOWPART);
		us_setcellname(wavewin);
	}

	/* find and setup associated schematics/layout window if requested */
	if (charhandlerschem != 0)
	{
		for(schemwin = el_topwindowpart; schemwin != NOWINDOWPART;
			schemwin = schemwin->nextwindowpart)
		{
			if ((schemwin->state&WINDOWTYPE) != DISPWINDOW &&
				(schemwin->state&WINDOWTYPE) != DISP3DWINDOW) continue;
			if (schemwin->curnodeproto == np) break;
		}
		if (schemwin != NOWINDOWPART)
		{
			startobjectchange((INTBIG)schemwin, VWINDOWPART);
			(void)setval((INTBIG)schemwin, VWINDOWPART, x_("state"), schemwin->state |
				WINDOWSIMMODE, VINTEGER);
			(void)setval((INTBIG)schemwin, VWINDOWPART, x_("charhandler"),
				(INTBIG)charhandlerschem, VADDRESS);
			endobjectchange((INTBIG)schemwin, VWINDOWPART);
		}
	}
	return(FALSE);
}

void sim_window_titleinfo(CHAR *title)
{
	if (sim_window_title != 0) efree((CHAR *)sim_window_title);
	allocstring(&sim_window_title, title, sim_tool->cluster);
}

/*
 * Routine to stop simulation.
 */
void sim_window_stopsimulation(void)
{
	REGISTER WINDOWPART *schemwin, *wavewin;

	/* disable simulation control in schematics/layout window */
	schemwin = sim_window_findschematics();
	if (schemwin != NOWINDOWPART)
	{
		startobjectchange((INTBIG)schemwin, VWINDOWPART);
		(void)setval((INTBIG)schemwin, VWINDOWPART, x_("state"), schemwin->state &
			~WINDOWSIMMODE, VINTEGER);
		(void)setval((INTBIG)schemwin, VWINDOWPART, x_("charhandler"),
			(INTBIG)DEFAULTCHARHANDLER, VADDRESS);
		endobjectchange((INTBIG)schemwin, VWINDOWPART);
	}

	/* disable waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin != NOWINDOWPART)
	{
		startobjectchange((INTBIG)wavewin, VWINDOWPART);
		(void)setval((INTBIG)wavewin, VWINDOWPART, x_("state"), wavewin->state &
			~WINDOWSIMMODE, VINTEGER);
		endobjectchange((INTBIG)wavewin, VWINDOWPART);
	}
}

/*
 * Routine to set vdd value used for grid 
 */
void sim_window_setvdd(float vdd)
{
	sim_window_vdd = vdd;
}

/*
 * routine that returns:
 *  0                                    if there is no active simulation
 *  SIMWINDOWWAVEFORM                    if simulating and have a waveform window
 *  SIMWINDOWSCHEMATIC                   if simulating and have a schematics window
 *  SIMWINDOWWAVEFORM|SIMWINDOWSCHEMATIC if simulating and have both windows
 * If there is active simulation, the cell being simulated is returned in "np"
 */
INTBIG sim_window_isactive(NODEPROTO **np)
{
	REGISTER INTBIG activity;
	REGISTER WINDOWPART *wavewin, *schemwin;

	*np = NONODEPROTO;
	activity = 0;
	schemwin = sim_window_findschematics();
	if (schemwin != NOWINDOWPART)
	{
		activity |= SIMWINDOWSCHEMATIC;
		*np = schemwin->curnodeproto;
	}
	wavewin = sim_window_findwaveform();
	if (wavewin != NOWINDOWPART && (wavewin->state&WINDOWSIMMODE) != 0)
	{
		activity |= SIMWINDOWWAVEFORM;
		*np = wavewin->curnodeproto;
	}
	return(activity);
}

/*
 * this procedure redraws everything, including the waveforms.
 */
void sim_window_redraw(void)
{
	REGISTER WINDOWPART *wavewin;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;
	sim_window_redisphandler(wavewin);
}

/*
 * routine that changes the number of trace frames in the simulation window to "count"
 */
void sim_window_setnumframes(INTBIG count)
{
	(void)sim_window_set_frames(count);
	if (sim_window_lines < sim_window_visframes)
		sim_window_visframes = sim_window_lines;
}

/*
 * routine that returns the number of signals in the simulation window
 */
INTBIG sim_window_getnumframes(void)
{
	return(sim_window_lines);
}

/*
 * routine that returns the number of visible signals in the simulation window
 */
INTBIG sim_window_getnumvisframes(void)
{
	return(sim_window_visframes);
}

/*
 * routine that changes the top visible trace frame in the simulation window to "count"
 */
void sim_window_settopvisframe(INTBIG top)
{
	if (top < 0) top = 0;
	sim_window_topframes = top;
}

/*
 * routine that returns the top visible signal in the simulation window.
 */
INTBIG sim_window_gettopvisframe(void)
{
	return(sim_window_topframes);
}

/********************************* CONTROL OF TRACES *********************************/

/*
 * routine to create a new trace to be displayed in window frame
 * "frameno".  Its name is set to "name", and "userdata" is
 * associated with it.  A pointer to the trace is returned (zero on error).
 */
INTBIG sim_window_newtrace(INTBIG frameno, CHAR *name, INTBIG userdata)
{
	REGISTER TRACE *tr, *otr, *lasttr;

	/* create a new trace */
	tr = sim_window_alloctrace();
	if (tr == NOTRACE) return(0);
	tr->nodeptr = userdata;
	tr->mintime = sim_window_maincursor;
	tr->maxtime = sim_window_extensioncursor;

	/* set name and convert if it has array index form */
	(void)allocstring(&tr->name, name, sim_tool->cluster);
	(void)allocstring(&tr->origname, name, sim_tool->cluster);

	/* put in active list */
	lasttr = NOTRACE;
	for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
		lasttr = otr;
	sim_window_addtrace(tr, lasttr);

	if (frameno >= 0) tr->frameno = frameno; else
	{
		tr->frameno = -1;
		sim_window_renumberlines();

		sim_window_setnumframes(sim_window_lines);
		sim_window_redraw();
	}

	/* save the new configuration */
	sim_window_savesignalorder();

	return((INTBIG)tr);
}

/*
 * Routine to open/close a bus.  Returns true on error.
 */
BOOLEAN sim_window_buscommand(void)
{
	REGISTER TRACE *tr;
	REGISTER INTBIG i, count, newtr;
	INTBIG *tracelist;

	if (sim_window_highlightedtracecount == 0) return(TRUE);
	if (sim_window_highlightedtracecount == 1)
	{
		tr = sim_window_highlightedtraces[0];
		switch (tr->flags&TRACETYPE)
		{
			case TRACEISDIGITAL:
			case TRACEISANALOG:
				ttyputerr(_("Must select multiple signals to pack into a bus"));
				break;
			case TRACEISBUS:
				ttyputmsg(_("Double-click the bus name to open it"));
				break;
			case TRACEISOPENBUS:
				ttyputmsg(_("Double-click the bus name to close it"));
				break;
		}
		return(TRUE);
	}

	/* copy highlighted traces to an array and remove highlighting */
	count = sim_window_highlightedtracecount;
	tracelist = (INTBIG *)emalloc(count * SIZEOFINTBIG, sim_tool->cluster);
	if (tracelist == 0) return(TRUE);
	for(i=0; i<count; i++)
		tracelist[i] = (INTBIG)sim_window_highlightedtraces[i];
	sim_window_cleartracehighlight();

	/* make the bus */
	newtr = sim_window_makebus(count, tracelist, 0);
	efree((CHAR *)tracelist);
	if (newtr == 0) return(TRUE);
	sim_window_addhighlighttrace(newtr);

	/* redisplay */
	sim_window_redraw();

	/* save the new configuration */
	sim_window_savesignalorder();
	return(FALSE);
}

INTBIG sim_window_makebus(INTBIG count, INTBIG *traces, CHAR *busname)
{
	REGISTER TRACE *tr, *lasttr, *newtr, **tracelist;
	REGISTER INTBIG i, flags;

	/* check for errors */
	tracelist = (TRACE **)traces;
	for(i=0; i<count; i++)
	{
		flags = tracelist[i]->flags & TRACETYPE;
		if (flags == TRACEISBUS || flags == TRACEISOPENBUS)
		{
			ttyputerr(_("Cannot place a bus into another bus"));
			return(0);
		}
		if (tracelist[i]->buschannel != NOTRACE)
		{
			ttyputerr(_("Signal '%s' is already on bus '%s'"), tracelist[i]->name,
				tracelist[i]->buschannel->name);
			return(0);
		}
	}
	if (count > MAXSIMWINDOWBUSWIDTH)
	{
		ttyputerr(_("Can only handle busses that are %d bits wide"), MAXSIMWINDOWBUSWIDTH);
		return(0);
	}

	/* find place to insert bus */
	lasttr = NOTRACE;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		for(i=0; i<count; i++)
			if (tracelist[i] == tr) break;
		if (i < count) break;
		lasttr = tr;
	}

	/* create a new trace that combines all of these */
	newtr = sim_window_alloctrace();
	if (newtr == NOTRACE) return(0);
	if (busname != 0) (void)allocstring(&newtr->name, busname, sim_tool->cluster); else
		(void)allocstring(&newtr->name, sim_window_namesignals(count, tracelist),
			sim_tool->cluster);
	(void)allocstring(&newtr->origname, newtr->name, sim_tool->cluster);
	newtr->nodeptr = 0;
	newtr->flags = TRACEISBUS;

	/* insert it after "lasttr" */
	sim_window_addtrace(newtr, lasttr);

	/* flag the signals as part of this bus */
	for(i=0; i<count; i++)
	{
		tr = tracelist[i];
		tr->buschannel = newtr;
		tr->flags |= TRACENOTDRAWN;
		if (i == 0)
		{
			newtr->mintime = tr->mintime;
			newtr->maxtime = tr->maxtime;
		} else
		{
			if (tr->mintime < newtr->mintime) newtr->mintime = tr->mintime;
			if (tr->maxtime > newtr->maxtime) newtr->maxtime = tr->maxtime;
		}
	}

	/* renumber the lines in the display */
	sim_window_renumberlines();
	return((INTBIG)newtr);
}

/*
 * Routine to generate a bus name for the "count" signals in "list".
 */
CHAR *sim_window_namesignals(INTBIG count, TRACE **list)
{
	REGISTER INTBIG i, j, rootlen, lastvalue, thisvalue, direction;
	REGISTER TRACE *tr, *ptr;
	REGISTER void *infstr;

	/* look for common root */
	rootlen = 0;
	for(i=1; i<count; i++)
	{
		ptr = list[i-1];
		tr = list[i];
		for(j=0; ; j++)
		{
			if (ptr->name[j] == 0 || tr->name[0] == 0) break;
			if (ptr->name[j] != tr->name[j]) break;
		}
		if (i == 1 || j < rootlen) rootlen = j;
	}

	/* if there is a common root, make sure it is proper */
	if (rootlen > 0)
	{
		tr = list[0];
		if (tr->name[rootlen-1] != '[')
		{
			for(i=0; i<count; i++)
			{
				tr = list[i];
				if (isdigit(tr->name[rootlen]) == 0) break;
			}
			if (i < count) rootlen = 0;
		}
	}

	/* if there is a common root, use it */
	if (rootlen > 0)
	{
		infstr = initinfstr();
		tr = list[0];
		for(j=0; j<rootlen; j++) (void)addtoinfstr(infstr, tr->name[j]);

		for(i=0; i<count; i++)
		{
			tr = list[i];
			if (i != 0) addtoinfstr(infstr, ',');
			for(j=rootlen; tr->name[j] != ']' && tr->name[j] != 0; j++)
				addtoinfstr(infstr, tr->name[j]);

			/* see if there is a sequence */
			thisvalue = 0;
			lastvalue = eatoi(&tr->name[rootlen]);
			direction = 0;
			for(j=i+1; j<count; j++)
			{
				thisvalue = eatoi(&list[j]->name[rootlen]);
				if (direction == 0)
				{
					if (lastvalue+1 == thisvalue) direction = 1; else
						if (lastvalue-1 == thisvalue) direction = -1; else
							break;
				}
				if (lastvalue + direction != thisvalue) break;
				lastvalue = thisvalue;
			}
			if (j > i+1)
			{
				formatinfstr(infstr, x_(":%ld"), thisvalue);
				i = j;
			}
		}
		if (tr->name[rootlen-1] == '[') addtoinfstr(infstr, ']');
		return(returninfstr(infstr));
	}

	/* no common root: concatenate all signal names */
	infstr = initinfstr();
	for(i=0; i<count; i++)
	{
		if (i != 0) addtoinfstr(infstr, ',');
		tr = list[i];
		addstringtoinfstr(infstr, tr->name);
	}
	return(returninfstr(infstr));
}

/*
 * Routine called when simulating, and the current cell changes to "cell".
 * caches the signal order for that cell, and updates the waveform window to
 * be associated with that cell.
 *
 * The list of signals can be found by calling "sim_window_getcachedsignals()"
 */
void sim_window_grabcachedsignalsoncell(NODEPROTO *cell)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, count;
	REGISTER WINDOWPART *w;

	if (sim_veroldstringarray == 0)
		sim_veroldstringarray = newstringarray(sim_tool->cluster);
	clearstrings(sim_veroldstringarray);
	var = getvalkey((INTBIG)cell, VNODEPROTO, VSTRING|VISARRAY,
		sim_window_signalorder_key);
	if (var != NOVARIABLE)
	{
		count = getlength(var);
		for(i=0; i<count; i++)
			addtostringarray(sim_veroldstringarray, ((CHAR **)var->addr)[i]);
	}

	/* find the waveform window and make sure it has the right cell */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state & WINDOWTYPE) != WAVEFORMWINDOW) continue;
		w->curnodeproto = cell;
		break;
	}
}

/*
 * Routine to return the number of strings that were found on the cell that
 * was passed to "sim_window_grabcachedsignalsoncell()".  The strings are returned
 * in "strings".
 */
INTBIG sim_window_getcachedsignals(CHAR ***strings)
{
	INTBIG count;

	*strings = getstringarray(sim_veroldstringarray, &count);
	return(count);
}

void sim_window_savesignalorder(void)
{
	REGISTER TRACE *tr, *otr;
	NODEPROTO *np;
	CHAR **strings;
	INTBIG count, i, j;
	REGISTER void *infstr;

	if (sim_window_isactive(&np) == 0) return;
	for(count = 0, tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		if ((tr->flags&TRACENOTDRAWN) == 0 && tr->buschannel == NOTRACE) count++;
	if (count == 0)
	{
		if (getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, sim_window_signalorder_key) != NOVARIABLE)
			(void)delvalkey((INTBIG)np, VNODEPROTO, sim_window_signalorder_key);
	} else
	{
		strings = (CHAR **)emalloc(count * (sizeof (CHAR *)), sim_tool->cluster);
		for(i=0, j=0, tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace, i++)
		{
			if ((tr->flags&TRACENOTDRAWN) != 0 || tr->buschannel != NOTRACE) continue;
			if ((tr->flags&TRACETYPE) == TRACEISBUS || (tr->flags&TRACETYPE) == TRACEISOPENBUS)
			{
				/* construct name for this bus */
				infstr = initinfstr();
				addstringtoinfstr(infstr, tr->origname);
				for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
				{
					if (otr->buschannel != tr) continue;
					formatinfstr(infstr, x_("\t%ld:%s"), otr->frameno, otr->origname);
				}
				(void)allocstring(&strings[j++], returninfstr(infstr), sim_tool->cluster);
			} else
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%ld:%s"), tr->frameno, tr->origname);
				(void)allocstring(&strings[j++], returninfstr(infstr), sim_tool->cluster);
			}
		}
		(void)setvalkey((INTBIG)np, VNODEPROTO, sim_window_signalorder_key, (INTBIG)strings,
			VSTRING|VISARRAY|(count<<VLENGTHSH));
		for(i=0; i<count; i++) efree((CHAR *)strings[i]);
		efree((CHAR *)strings);
	}
}

/*
 * routine to load trace "tri" with "count" digital events.  The events occur
 * at time "time" and have state "state".  "State" is encoded with a
 * level in the upper 8 bits (LOGIC_LOW, LOGIC_HIGH, LOGIC_X, or LOGIC_Z) and a
 * strength in the lower 8 bits (OFF_STRENGTH, NODE_STRENGTH, GATE_STRENGTH,
 * or VDD_STRENGTH).
 */
void sim_window_loaddigtrace(INTBIG tri, INTBIG count, double *time, INTSML *state)
{
	REGISTER INTBIG wanted, i;
	REGISTER TRACE *tr;

	/* ensure space in the arrays */
	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return;
	if (count > tr->timelimit)
	{
		if (tr->timelimit > 0)
		{
			efree((CHAR *)tr->timearray);
			tr->timelimit = 0;
		}
		wanted = count + 10;
		tr->timearray = (double *)emalloc(wanted * (sizeof (double)), sim_tool->cluster);
		if (tr->timearray == 0) return;
		tr->timelimit = wanted;
	}
	if (count > tr->statelimit)
	{
		if (tr->statelimit > 0)
		{
			efree((CHAR *)tr->statearray);
			tr->statelimit = 0;
		}
		wanted = count + 10;
		tr->statearray = (INTBIG *)emalloc(wanted * SIZEOFINTBIG, sim_tool->cluster);
		tr->statelimit = wanted;
	}

	/* load the data */
	for(i=0; i<count; i++)
	{
		tr->timearray[i] = time[i];
		tr->statearray[i] = state[i];
	}
	tr->flags = (tr->flags & ~TRACETYPE) | TRACEISDIGITAL;
	tr->numsteps = count;
}

/*
 * routine to load trace "tri" with "count" analog events.  The events occur
 * at time "time" and have value "value".
 */
void sim_window_loadanatrace(INTBIG tri, INTBIG count, double *time, float *value)
{
	REGISTER INTBIG wanted, i;
	REGISTER TRACE *tr;

	/* ensure space in the arrays */
	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return;
	if (count > tr->timelimit)
	{
		if (tr->timelimit > 0)
		{
			efree((CHAR *)tr->timearray);
			tr->timelimit = 0;
		}
		wanted = count + 10;
		tr->timearray = (double *)emalloc(wanted * (sizeof (double)), sim_tool->cluster);
		if (tr->timearray == 0) return;
		tr->timelimit = wanted;
	}
	if (count > tr->valuelimit)
	{
		if (tr->valuelimit > 0)
		{
			efree((CHAR *)tr->valuearray);
			tr->valuelimit = 0;
		}
		wanted = count + 10;
		tr->valuearray = (float *)emalloc(wanted * (sizeof (float)), sim_tool->cluster);
		if (tr->valuearray == 0) return;
		tr->valuelimit = wanted;
	}

	/* load the data */
	for(i=0; i<count; i++)
	{
		tr->timearray[i] = time[i];
		tr->valuearray[i] = value[i];
	}
	tr->flags = (tr->flags & ~TRACETYPE) | TRACEISANALOG;
	tr->color = sim_window_anacolor[sim_window_curanacolor];
	sim_window_curanacolor++;
	if (sim_window_anacolor[sim_window_curanacolor] == 0) sim_window_curanacolor = 0;
	tr->numsteps = count;
	sim_window_trace_range(tri);
#if 1
	sim_window_auto_anarange();
#endif
}

void sim_window_settraceframe(INTBIG tri, INTBIG frameno)
{
	REGISTER TRACE *tr;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return;
	tr->frameno = frameno;
}

/*
 * routine to delete the trace "tri"
 */
void sim_window_killtrace(INTBIG tri)
{
	REGISTER TRACE *tr, *str, *ltr, *otr;
	REGISTER INTBIG frameno;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return;
	frameno = tr->frameno;
	ltr = NOTRACE;
	for(str = sim_window_firstrace; str != NOTRACE; str = str->nexttrace)
	{
		if (str == tr) break;
		ltr = str;
	}
	if (ltr == NOTRACE) sim_window_firstrace = tr->nexttrace; else
		ltr->nexttrace = tr->nexttrace;
	tr->nexttrace = sim_window_tracefree;
	sim_window_tracefree = tr;
	efree(tr->name);
	efree(tr->origname);

	/* if a bus was deleted, make it's individual signals be visible and independent */
	if ((tr->flags&TRACETYPE) == TRACEISBUS || (tr->flags&TRACETYPE) == TRACEISOPENBUS)
	{
		for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
		{
			if (otr->buschannel != tr) continue;
			otr->buschannel = NOTRACE;
			otr->flags &= ~TRACENOTDRAWN;
		}
	}

	/* renumber the lines in the display */
	sim_window_renumberlines();

	/* save the new configuration */
	sim_window_savesignalorder();

	/* if the number of lines is less than the number of visible lines, redisplay */
	if (sim_window_lines < sim_window_visframes)
	{
		sim_window_setnumframes(sim_window_lines);
		sim_window_redraw();
	}
}

/*
 * routine to delete all traces
 */
void sim_window_killalltraces(BOOLEAN save)
{
	REGISTER TRACE *tr, *nexttr;

	/* return all plot channels to the free list */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = nexttr)
	{
		nexttr = tr->nexttrace;
		tr->nexttrace = sim_window_tracefree;
		sim_window_tracefree = tr;
		efree(tr->name);
		efree(tr->origname);
	}
	sim_window_firstrace = NOTRACE;
	sim_window_highlightedtracecount = 0;

	/* save the new configuration */
	if (save) sim_window_savesignalorder();
	sim_window_set_frames(1);
	sim_window_visframes = 1;
	sim_window_topframes = 0;
	sim_window_highframe = -1;
}

/*
 * Ignore "prefix" when it appears at the start of a trace name.
 */
void sim_window_supresstraceprefix(CHAR *prefix)
{
	if (sim_window_traceprefix != 0) efree((CHAR *)sim_window_traceprefix);
	allocstring(&sim_window_traceprefix, prefix, sim_tool->cluster);
}

/********************************* TRACE INFORMATION *********************************/

INTBIG *sim_window_getbustraces(INTBIG tri)
{
	REGISTER TRACE *tr, *otr;
	REGISTER INTBIG count, newtotal, i;
	REGISTER TRACE **newbustraces;
	static INTBIG nullbustraces[1];

	tr = (TRACE *)tri;
	count = 0;
	for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
	{
		if (otr->buschannel != tr) continue;
		if (count+1 >= sim_window_bustracetotal)
		{
			newtotal = sim_window_bustracetotal*2;
			if (newtotal == 0) newtotal = 5;
			if (count+1 > newtotal) newtotal = count+1;
			newbustraces = (TRACE **)emalloc(newtotal * (sizeof (TRACE *)),
				sim_tool->cluster);
			if (newbustraces == 0) return(0);
			for(i=0; i<count; i++)
				newbustraces[i] = sim_window_bustraces[i];
			if (sim_window_bustracetotal > 0) efree((CHAR *)sim_window_bustraces);
			sim_window_bustraces = newbustraces;
			sim_window_bustracetotal = newtotal;
		}
		sim_window_bustraces[count++] = otr;
		sim_window_bustraces[count] = 0;
	}
	if (count == 0)
	{
		nullbustraces[0] = 0;
		return(nullbustraces);
	}
	return((INTBIG *)sim_window_bustraces);
}

/*
 * routine to return the frame number associated with trace "tri"
 */
INTBIG sim_window_gettraceframe(INTBIG tri)
{
	REGISTER TRACE *tr;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return(0);
	return(tr->frameno);
}

/*
 * routine to return the current frame number
 */
INTBIG sim_window_getcurframe(void)
{
	return(sim_window_highframe);
}

/*
 * routine to return the name of trace "tri".
 */
CHAR *sim_window_gettracename(INTBIG tri)
{
	REGISTER TRACE *tr;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return(x_(""));
	return(tr->origname);
}

/*
 * routine to return the "user" data associated with trace "tri"
 */
INTBIG sim_window_gettracedata(INTBIG tri)
{
	REGISTER TRACE *tr;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return(0);
	return(tr->nodeptr);
}

/*
 * Routine to return the value of analog trace "tri" at time "time".
 * Interpolates the data to get the answer.
 */
float sim_window_getanatracevalue(INTBIG tri, double time)
{
	REGISTER TRACE *tr;
	REGISTER INTBIG j;
	REGISTER float lastvalue, thisvalue, v;
	REGISTER double lasttime, thistime;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return(0.0);

	/* must be an analog trace */
	if ((tr->flags&TRACETYPE) != TRACEISANALOG) return(0.0);

	lastvalue = 0.0;
	lasttime = 0.0;
	for(j=0; j<tr->numsteps; j++)
	{
		thistime = tr->timearray[j];
		thisvalue = tr->valuearray[j];
		if (thistime == time) return(thisvalue);
		if (time > thistime)
		{
			lasttime = thistime;
			lastvalue = thisvalue;
			continue;
		}
		if (j == 0) return(thisvalue);
		v = (float)(lastvalue + (time-lasttime) * (thisvalue-lastvalue) / (thistime-lasttime));
		return(v);
	}

	return(lastvalue);
}

/*
 * Routine to return the value of digital trace "tri" at time "time".
 * Sets the level (high/low/etc) in "level" and the strength in "strength".
 */
void sim_window_getdigtracevalue(INTBIG tri, double time, INTBIG *level, INTBIG *strength)
{
	REGISTER TRACE *tr;
	REGISTER INTBIG j;
	REGISTER double thistime;

	*level = *strength = 0;
	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return;

	/* must be an analog trace */
	if ((tr->flags&TRACETYPE) != TRACEISDIGITAL) return;

	for(j=0; j<tr->numsteps; j++)
	{
		thistime = tr->timearray[j];
		if (thistime > time) break;
		*level = tr->statearray[j] >> 8;
		*strength = tr->statearray[j] & 0377;
	}
}

/*
 * Routine to return the value of bus trace "tri" at time "time".
 * Returns the value as a string.
 */
CHAR *sim_window_getbustracevalue(INTBIG tri, double time)
{
	REGISTER TRACE *tr, *bustr;
	REGISTER INTBIG j, sigcount;

	tr = (TRACE *)tri;
	if (tr == NOTRACE || tr == 0) return(x_(""));

	/* must be an analog trace */
	if ((tr->flags&TRACETYPE) != TRACEISBUS &&
		(tr->flags&TRACETYPE) != TRACEISOPENBUS) return(x_(""));

	sigcount = sim_window_ensurespace(tr);
	for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
	{
		if (bustr->buschannel != tr) continue;
		for(j=0; j<bustr->numsteps; j++)
			if (time < bustr->timearray[j]) break;
		if (j > 0) j--;
		sim_window_sigvalues[bustr->busindex] = bustr->statearray[j] >> 8;
	}
	return(sim_window_makewidevalue(sigcount, sim_window_sigvalues));
}

LISTINTBIG *sim_window_foundtraces = 0;

/*
 * Routine to return the traces associated with name "name" (if it is a bus, there may be
 * multiple traces).  The number of traces is returned in "count" and the trace list is
 * returned by the routine.
 */
INTBIG *sim_window_findtrace(CHAR *name, INTBIG *count)
{
	REGISTER TRACE *tr;
	REGISTER CHAR *pt;
	CHAR **strings;
	REGISTER INTBIG len, i;

	if (sim_window_foundtraces == 0)
		sim_window_foundtraces = newintlistobj(sim_tool->cluster);
	clearintlistobj(sim_window_foundtraces);

	/* look for this name without modifications */
	sim_window_findthistrace(name);

	/* look for bus names in the requested string */
	for(pt = name; *pt != 0; pt++)
		if (*pt == '[') break;
	if (*pt == '[')
	{
		/* parse into individual signal names */
		len = net_evalbusname(APBUS, name, &strings, NOARCINST, NONODEPROTO, 0);
		for(i=0; i<len; i++)
			sim_window_findthistrace(strings[i]);
	} else
	{
		/* look for bus names that have no indexing */
		len = estrlen(name);
		for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		{
			if ((tr->flags&TRACETYPE) != TRACEISBUS &&
				(tr->flags&TRACETYPE) != TRACEISOPENBUS) continue;
			if (namesamen(tr->name, name, len) == 0 && tr->name[len] == '[')
				addtointlistobj(sim_window_foundtraces, (INTBIG)tr, FALSE);
		}
	}
	return(getintlistobj(sim_window_foundtraces, count));
}

void sim_window_findthistrace(CHAR *name)
{
	REGISTER TRACE *tr;
	REGISTER INTBIG i, len;
	REGISTER CHAR *tempname;
	REGISTER void *infstr;

	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if (namesame(tr->name, name) == 0)
		{
			addtointlistobj(sim_window_foundtraces, (INTBIG)tr, FALSE);
			if (tr->buschannel != NOTRACE)
				addtointlistobj(sim_window_foundtraces, (INTBIG)tr->buschannel, TRUE);
			return;
		}
	}

	/* see if the name is found when considering the prefix */
	if (sim_window_traceprefix != 0)
	{
		len = estrlen(sim_window_traceprefix);
		for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		{
			if (namesamen(tr->name, sim_window_traceprefix, len) == 0 &&
				namesame(&tr->name[len], name) == 0)
			{
				addtointlistobj(sim_window_foundtraces, (INTBIG)tr, FALSE);
				if (tr->buschannel != NOTRACE)
					addtointlistobj(sim_window_foundtraces, (INTBIG)tr->buschannel, TRUE);
				return;
			}
		}
	}

	/* keywords get the letters "NV" added to the end */
	infstr = initinfstr();
	addstringtoinfstr(infstr, name);
	addstringtoinfstr(infstr, x_("NV"));
	tempname = returninfstr(infstr);
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if (namesame(tr->name, tempname) == 0)
		{
			addtointlistobj(sim_window_foundtraces, (INTBIG)tr, FALSE);
			return;
		}
	}

	/* handle "v(" and "l(" prefixes of HSPICE (see also "dbtext.c:getcomplexnetworks()") */
	len = estrlen(name);
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->name[0] == 'l' || tr->name[0] == 'v') && tr->name[1] == '(')
		{
			if (namesamen(&tr->name[2], name, len) == 0)
			{
				if (tr->name[len+2] == 0 || tr->name[len+2] == ')')
				{
					addtointlistobj(sim_window_foundtraces, (INTBIG)tr, FALSE);
					return;
				}
			}
		}
	}

	/* allow "_" in simulator text to match any punctuation in desired name */
	len = estrlen(name);
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		for(i=0; ; i++)
		{
			if (tr->name[i] == 0 || tolower(name[i]) == 0) break;
			if (tolower(tr->name[i]) == tolower(name[i])) continue;
			if (tr->name[i] == '_' && ispunct(name[i])) continue;
			break;
		}
		if (tr->name[i] == 0 && tolower(name[i]) == 0)
		{
			addtointlistobj(sim_window_foundtraces, (INTBIG)tr, FALSE);
			return;
		}
	}
}

void sim_window_inittraceloop(void)
{
	sim_window_loop = sim_window_firstrace;
}

void sim_window_inittraceloop2(void)
{
	sim_window_loop2 = sim_window_firstrace;
}

INTBIG sim_window_nexttraceloop(void)
{
	REGISTER TRACE *tr;

	tr = sim_window_loop;
	if (tr == NOTRACE) return(0);
	sim_window_loop = tr->nexttrace;
	return((INTBIG)tr);
}

INTBIG sim_window_nexttraceloop2(void)
{
	REGISTER TRACE *tr;

	tr = sim_window_loop2;
	if (tr == NOTRACE) return(0);
	sim_window_loop2 = tr->nexttrace;
	return((INTBIG)tr);
}

/********************************* TRACE/FRAME HIGHLIGHTING *********************************/

/*
 * Routine to initialize changes to the trace highlighting.  After calling this, any
 * trace whose highlighting changes should have the "TRACEHIGHCHANGED" bit set in its flags.
 */
void sim_window_inithighlightchange(void)
{
	REGISTER TRACE *tr;

	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		tr->flags &= ~TRACEHIGHCHANGED;
}

/*
 * Routine to terminate changes to the trace highlighting.  Redisplays any frames
 * where the trace highlighting changed.
 */
void sim_window_donehighlightchange(WINDOWPART *wavewin)
{
	REGISTER TRACE *tr, *otr;
	REGISTER INTBIG highframe;

	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACEHIGHCHANGED) == 0) continue;
		sim_window_drawframe(tr->frameno, wavewin);
		for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
			if (otr->frameno == tr->frameno)
				otr->flags &= ~TRACEHIGHCHANGED;
	}

	/* see if a single frame is highlighted */
	highframe = -1;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if ((tr->flags&TRACESELECTED) == 0) continue;
//		if ((tr->flags&TRACETYPE) != TRACEISANALOG) continue;
		if (highframe == -1) highframe = tr->frameno;
		if (highframe != tr->frameno) highframe = -2;
	}

	if (highframe == -2) highframe = -1;
	if (highframe != sim_window_highframe)
	{
		sim_window_setviewport(wavewin);

		if (sim_window_highframe >= 0)
			sim_window_showtracebar(wavewin, sim_window_highframe, 2);
		sim_window_highframe = highframe;
		if (sim_window_highframe >= 0)
			sim_window_showtracebar(wavewin, sim_window_highframe, 1);
	}
}

/*
 * Routine to highlight trace "tr" in window "wavewin".  If "on" is TRUE, highlight it,
 * otherwise remove highlighting.
 */
void sim_window_highlightname(WINDOWPART *wavewin, TRACE *tr, BOOLEAN on)
{
	REGISTER INTBIG lx, hx, ly, hy, color, temp_trace_spacing, slot, y_min;

	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	slot = tr->frameno - sim_window_topframes;
	if (slot < 0 || slot >= sim_window_visframes) return;
	y_min = (sim_window_visframes - 1 - slot) * temp_trace_spacing + WAVEYSIGBOT;

	/* highlight/unhighlight trace name */
	ly = y_min + tr->nameoff+1;   hy = ly + sim_window_txthei;
	lx = 0;                       hx = sim_window_wavexnameend;
	sim_window_setmask(LAYERH);
	if (on) color = HIGHLIT; else color = 0;
	sim_window_setcolor(color);
	sim_window_moveto(lx, ly);
	sim_window_drawto(wavewin, hx, ly, 0);
	sim_window_drawto(wavewin, hx, hy, 0);
	sim_window_drawto(wavewin, lx, hy, 0);
	sim_window_drawto(wavewin, lx, ly, 0);
	sim_window_setmask(LAYERA);
}

/*
 * Routine to make sure the highlighted traces are visible (not scrolled away)
 */
void sim_window_showhighlightedtraces(void)
{
	REGISTER INTBIG slot;
	REGISTER TRACE *tr;

	if (sim_window_visframes == 0) return;
	if (sim_window_highlightedtracecount == 0) return;
	tr = sim_window_highlightedtraces[0];
	slot = tr->frameno - sim_window_topframes;
	if (slot >= 0 && slot < sim_window_visframes) return;

	/* shift the scroll */
	sim_window_topframes = tr->frameno - sim_window_visframes/2;
	if (sim_window_topframes < 0) sim_window_topframes = 0;
	sim_window_redraw();
}

/*
 * Routine to clear all highlighting in the waveform window.
 */
void sim_window_cleartracehighlight(void)
{
	REGISTER WINDOWPART *wavewin;
	REGISTER INTBIG i;
	REGISTER TRACE *tr;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;
	sim_window_setviewport(wavewin);

	/* initialize changes to highlighted traces */
	sim_window_inithighlightchange();

	/* clear all highlighting */
	for(i=0; i<sim_window_highlightedtracecount; i++)
	{
		tr = sim_window_highlightedtraces[i];
		tr->flags &= ~TRACESELECTED;
		tr->flags |= TRACEHIGHCHANGED;
		sim_window_highlightname(wavewin, tr, FALSE);
	}
	sim_window_highlightedtracecount = 0;

	/* show changes to highlighted traces */
	sim_window_donehighlightchange(wavewin);

	sim_window_drawcursors(wavewin);
}

/*
 * routine that adds trace "tri" to the highlighted traces in the waveform window
 */
void sim_window_addhighlighttrace(INTBIG tri)
{
	REGISTER TRACE *tr, **newtraces;
	REGISTER WINDOWPART *wavewin;
	REGISTER INTBIG newtotal, i;

	tr = (TRACE *)tri;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;
	sim_window_setviewport(wavewin);

	/* initialize changes to trace highlighting */
	sim_window_inithighlightchange();

	/* add this signal to highlighting */
	if (sim_window_highlightedtracecount+1 >= sim_window_highlightedtracetotal)
	{
		newtotal = sim_window_highlightedtracetotal * 2;
		if (newtotal == 0) newtotal = 2;
		if (sim_window_highlightedtracecount+1 > newtotal)
			newtotal = sim_window_highlightedtracecount+1;
		newtraces = (TRACE **)emalloc(newtotal * (sizeof (TRACE *)), sim_tool->cluster);
		if (newtraces == 0) return;
		for(i=0; i<sim_window_highlightedtracecount; i++)
			newtraces[i] = sim_window_highlightedtraces[i];
		if (sim_window_highlightedtracetotal > 0) efree((CHAR *)sim_window_highlightedtraces);
		sim_window_highlightedtraces = newtraces;
		sim_window_highlightedtracetotal = newtotal;
	}
	sim_window_highlightedtraces[sim_window_highlightedtracecount++] = tr;
	tr->flags |= (TRACESELECTED | TRACEHIGHCHANGED);
	sim_window_highlightedtraces[sim_window_highlightedtracecount] = 0;
	sim_window_highlightname(wavewin, tr, TRUE);

	/* show highlighting */
	sim_window_donehighlightchange(wavewin);
	sim_window_drawcursors(wavewin);
}

/*
 * routine that removes trace "tri" from the highlighted traces in the waveform window
 */
void sim_window_deletehighlighttrace(INTBIG tri)
{
	REGISTER TRACE *tr;
	REGISTER WINDOWPART *wavewin;
	REGISTER INTBIG k, j, i;

	tr = (TRACE *)tri;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;
	sim_window_setviewport(wavewin);

	/* find this signal in the highlighting */
	for(j=0; j<sim_window_highlightedtracecount; j++)
		if (tr == sim_window_highlightedtraces[j]) break;
	if (j < sim_window_highlightedtracecount)
	{
		/* erase highlighting */
		sim_window_inithighlightchange();

		/* remove the signal from the list */
		k = 0;
		for(i=0; i<sim_window_highlightedtracecount; i++)
		{
			if (i == j) continue;
			sim_window_highlightedtraces[k++] = sim_window_highlightedtraces[i];
		}
		sim_window_highlightedtracecount = k;
		tr->flags &= ~TRACESELECTED;
		tr->flags |= TRACEHIGHCHANGED;
		sim_window_highlightname(wavewin, tr, FALSE);

		/* show highlighting */
		sim_window_donehighlightchange(wavewin);
	}
	sim_window_drawcursors(wavewin);
}

/*
 * routine that returns the highlighted trace in the waveform window
 */
INTBIG sim_window_gethighlighttrace(void)
{
	if (sim_window_highlightedtracecount == 0) return(0);
	return((INTBIG)sim_window_highlightedtraces[0]);
}

/*
 * routine that returns a 0-terminated list of highlighted traces in the waveform window
 */
INTBIG *sim_window_gethighlighttraces(void)
{
	static INTBIG nothing[1];

	nothing[0] = 0;
	if (sim_window_highlightedtracecount == 0) return(nothing);
	return((INTBIG *)sim_window_highlightedtraces);
}

/*
 * Routine to update associated layout windows when the main cursor changes
 */
void sim_window_updatelayoutwindow(void)
{
	REGISTER TRACE *tr, *otr;
	REGISTER NETWORK *net;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG fx, fy, tx, ty, i, x, y, count;
	REGISTER NODEPROTO *simnt;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *schemwin;
	INTBIG texture, color, lx, hx, ly, hy;
	static POLYGON *poly = NOPOLYGON;
	XARRAY trans;

	/* make sure there is a layout/schematic window being simulated */
	schemwin = sim_window_findschematics();
	if (schemwin == NOWINDOWPART) return;
	simnt = schemwin->curnodeproto;

	/* reset all values on networks */
	for(net = simnt->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = net->temp2 = -1;

	/* assign values from simulation window traces to networks */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;

		/* can only handle digital waveforms right now */
		if ((tr->flags&TRACETYPE) == TRACEISDIGITAL)
		{
			sim_window_putvalueontrace(tr, simnt);
		} else if ((tr->flags&TRACETYPE) == TRACEISBUS || (tr->flags&TRACETYPE) == TRACEISOPENBUS)
		{
			for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
			{
				if (otr->buschannel != tr) continue;
				sim_window_putvalueontrace(otr, simnt);
			}
		}
	}

	/* light up any simulation-probe objects */
	for(ni = simnt->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != gen_simprobeprim) continue;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->network == NONETWORK) continue;
			if (ai->network->temp1 != -1) break;
		}
		if (pi == NOPORTARCINST) continue;

		sim_window_gethighlightcolor(ai->network->temp1, &texture, &color);
		sim_window_desc.col = color;
		var = gettrace(ni);
		if (var != NOVARIABLE)
		{
			count = getlength(var) / 2;
			(void)needstaticpolygon(&poly, count, sim_tool->cluster);
			x = (ni->highx + ni->lowx) / 2;   y = (ni->highy + ni->lowy) / 2;
			for(i=0; i<count; i++)
			{
				poly->xv[i] = applyxscale(schemwin, ((INTBIG *)var->addr)[i*2] + x - schemwin->screenlx) + schemwin->uselx;
				poly->yv[i] = applyyscale(schemwin, ((INTBIG *)var->addr)[i*2+1] + y - schemwin->screenly) + schemwin->usely;
			}
			poly->count = count;
			poly->style = FILLED;
			makerot(ni, trans);
			xformpoly(poly, trans);
			clippoly(poly, schemwin->uselx, schemwin->usehx, schemwin->usely, schemwin->usehy);
			if (poly->count > 1)
			{
				if (poly->count > 2)
				{
					/* always clockwise */
					if (areapoly(poly) < 0.0) reversepoly(poly);
					screendrawpolygon(schemwin, poly->xv, poly->yv, poly->count, &sim_window_desc);
				} else screendrawline(schemwin, poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1],
					&sim_window_desc, 0);
			}
			ai->network->temp2 = 0;
		} else
		{
			lx = ni->geom->lowx;   hx = ni->geom->highx;
			ly = ni->geom->lowy;   hy = ni->geom->highy;
			if (!us_makescreen(&lx, &ly, &hx, &hy, schemwin))
			{
				screendrawbox(schemwin, lx, hx, ly, hy, &sim_window_desc);
				ai->network->temp2 = 0;
			}
		}
	}

	/* redraw all arcs in the layout/schematic window */
	sim_window_desc.bits = LAYERA;
	for(ai = simnt->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		net = ai->network;
		if (net == NONETWORK) continue;
		if (net->temp1 == -1 || net->temp2 != -1) continue;

		/* get extent of arc */
		fx = ai->end[0].xpos;   fy = ai->end[0].ypos;
		tx = ai->end[1].xpos;   ty = ai->end[1].ypos;
		if (ai->proto == sch_wirearc)
		{
			/* for schematic wires, ask technology about drawing so negating bubbles work */
			(void)needstaticpolygon(&poly, 4, sim_tool->cluster);
			(void)arcpolys(ai, NOWINDOWPART);
			shapearcpoly(ai, 0, poly);
			fx = poly->xv[0];   fy = poly->yv[0];
			tx = poly->xv[1];   ty = poly->yv[1];
		}

		sim_window_gethighlightcolor(net->temp1, &texture, &color);

		/* reset background arc if trace is textured */
		if (texture != 0)
		{
			sim_window_desc.col = ALLOFF;
			us_wanttodraw(fx, fy, tx, ty, schemwin, &sim_window_desc, 0);
			if (texture < 0) texture = 0;
		}
		sim_window_desc.col = color;

		/* draw the trace on the arc */
		us_wanttodraw(fx, fy, tx, ty, schemwin, &sim_window_desc, texture);
	}
}

void sim_window_gethighlightcolor(INTBIG state, INTBIG *texture, INTBIG *color)
{
	REGISTER INTBIG strength;

	if ((sim_window_state&FULLSTATE) != 0)
	{
		/* 12-state display: determine trace texture */
		strength = state & 0377;
		if (strength == 0) *texture = -1; else
			if (strength <= NODE_STRENGTH) *texture = 1; else
				if (strength <= GATE_STRENGTH) *texture = 0; else
					*texture = 2;

		/* determine trace color */
		switch (state >> 8)
		{
			case LOGIC_LOW:  *color = sim_colorlevellow;     break;
			case LOGIC_X:    *color = sim_colorlevelundef;   break;
			case LOGIC_HIGH: *color = sim_colorlevelhigh;    break;
			case LOGIC_Z:    *color = sim_colorlevelzdef;    break;
		}
	} else
	{
		/* 2-state display */
		*texture = 0;
		if ((state >> 8) == LOGIC_HIGH) *color = sim_colorstrengthgate; else
			*color = sim_colorstrengthoff;
	}
}

void sim_window_putvalueontrace(TRACE *tr, NODEPROTO *simnt)
{
	REGISTER NETWORK *net, **netlist;
	REGISTER INTBIG j;

	if (tr->statearray == 0) return;

	/* set simulation value on the network in the associated layout/schematic window */
	netlist = getcomplexnetworks(tr->name, simnt);
	net = netlist[0];
	if (net != NONETWORK && netlist[1] == NONETWORK)
	{
		/* find the proper data for the time of the main cursor */
		for(j=1; j<tr->numsteps; j++)
			if (sim_window_maincursor < tr->timearray[j]) break;
		j--;
		net->temp1 = tr->statearray[j];
	}
}

/********************************* TIME AND CURSOR CONTROL *********************************/

/*
 * routine to set the time for the "main" cursor in the simulation window
 */
void sim_window_setmaincursor(double time)
{
	REGISTER WINDOWPART *wavewin;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;

	sim_window_maincursor = time;
	sim_window_updatelayoutwindow();
	sim_window_drawcursors(wavewin);
}

/*
 * routine to return the time of the "main" cursor in the simulation window
 */
double sim_window_getmaincursor(void)
{
	return(sim_window_maincursor);
}

/*
 * routine to set the time for the "extension" cursor in the simulation window
 */
void sim_window_setextensioncursor(double time)
{
	REGISTER WINDOWPART *wavewin;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;

	sim_window_extensioncursor = time;
	sim_window_drawcursors(wavewin);
}

/*
 * routine to return the time of the "extension" cursor in the simulation window
 */
double sim_window_getextensioncursor(void)
{
	return(sim_window_extensioncursor);
}

/*
 * routine to set the simulation trace "tri" to run from time "min" to time "max"
 */
void sim_window_settimerange(INTBIG tri, double min, double max)
{
	REGISTER TRACE *tr, *otr;

	tr = (TRACE *)tri;
	for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
	{
		if (tr != 0 && otr->frameno != tr->frameno) continue;
		otr->mintime = min;
		otr->maxtime = max;
	}
}

/*
 * routine to return the range of time displayed in the simulation window
 */
void sim_window_gettimerange(INTBIG tri, double *min, double *max)
{
	REGISTER TRACE *tr;

	tr = (TRACE *)tri;
	*min = tr->mintime;
	*max = tr->maxtime;
}

/*
 * routine to return the average range of time displayed in the simulation window
 */
void sim_window_getaveragetimerange(double *totmintime, double *totmaxtime)
{
	double mintime, maxtime;
	REGISTER INTBIG tottraces, tr, j;

	sim_window_inittraceloop();
	*totmintime = *totmaxtime = 0.0;
	tottraces = 0;
	for(j=0; ; j++)
	{
		tr = sim_window_nexttraceloop();
		if (tr == 0) break;
		sim_window_gettimerange(tr, &mintime, &maxtime);
		*totmintime += mintime;
		*totmaxtime += maxtime;
		tottraces++;
	}
	if (tottraces > 0)
	{
		*totmintime /= tottraces;
		*totmaxtime /= tottraces;
	}
}

/*
 * routine to return the range of time that is in the data
 */
void sim_window_gettimeextents(double *min, double *max)
{
	REGISTER TRACE *tr;
	double time;
	REGISTER INTBIG j;

	/* initially enclose the cursors */
	*min = sim_window_maincursor;
	*max = sim_window_extensioncursor;
	if (*max < *min)
	{
		*min = sim_window_extensioncursor;
		*max = sim_window_maincursor;
	}

	/* also enclose all of the data */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		for(j=0; j<tr->numsteps; j++)
		{
			time = tr->timearray[j];
			if (time < *min) *min = time;
			if (time > *max) *max = time;
		}
	}
	if (*min > 0.0) *min = 0.0;
}

/********************************* WINDOW METHODS *********************************/

BOOLEAN sim_window_zoomdown(INTBIG x, INTBIG y)
{
	REGISTER WINDOWPART *wavewin;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return(FALSE);

	sim_window_drawareazoom(wavewin, FALSE, sim_window_zoominix, sim_window_zoominiy,
		sim_window_zoomothx, sim_window_zoomothy);
	sim_window_scaletowindow(wavewin, &x, &y);
	sim_window_zoomothx = x;
	sim_window_zoomothy = y;
	sim_window_drawareazoom(wavewin, TRUE, sim_window_zoominix, sim_window_zoominiy,
		sim_window_zoomothx, sim_window_zoomothy);
	return(FALSE);
}

void sim_window_drawareazoom(WINDOWPART *w, BOOLEAN on, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty)
{
	sim_window_setmask(LAYERH);
	if (on) sim_window_setcolor(HIGHLIT); else
		sim_window_setcolor(ALLOFF);
	sim_window_moveto(fx, fy);
	sim_window_drawto(w, fx, ty, 0);
	sim_window_drawto(w, tx, ty, 0);
	sim_window_drawto(w, tx, fy, 0);
	sim_window_drawto(w, fx, fy, 0);
}

BOOLEAN sim_window_eachwdown(INTBIG x, INTBIG y)
{
	sim_window_eachcursordown(x, y, &sim_window_maincursor);
	sim_window_updatelayoutwindow();
	return(FALSE);
}

BOOLEAN sim_window_eachbdown(INTBIG x, INTBIG y)
{
	sim_window_eachcursordown(x, y, &sim_window_extensioncursor);
	return(FALSE);
}

void sim_window_eachcursordown(INTBIG x, INTBIG y, double *cursor)
{
	REGISTER WINDOWPART *wavewin;
	REGISTER TRACE *tr, *trh;
	double val, valh;
	INTBIG frameno, yh;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;

	tr = (TRACE *)sim_window_firstrace;
	if (tr == NOTRACE) return;
	sim_window_scaletowindow(wavewin, &x, &y);
	*cursor = sim_window_xpostotime(x);
	if (*cursor < tr->mintime) *cursor = tr->mintime;
	if (*cursor > tr->maxtime) *cursor = tr->maxtime;

	/* check if mouse is near voltage threshold horizontal line */
	yh = -1;
	valh = 0.0;
	frameno = sim_window_ypostoval(y, &val);
	if (frameno >= 0 && sim_window_vdd != 0)
	{
		INTBIG ymax = 10;
		int i;
		for (i = 0; i < PERCENTSIZE; i++)
		{
			double v = sim_window_vdd * sim_window_percent[i];
			INTBIG yp = sim_window_valtoypos(frameno, v);
			if (yp >= 0 && labs(yp - y) < ymax)
			{
				ymax = labs(yp - y);
				yh = yp;
				valh = v;
			}
		}
	}

	/* check if mouse is near crosspoint of some trace */
	trh = NOTRACE;
	if (yh >= 0)
	{
		double th = 0;
		for (tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		{
			double t = *cursor;

			if (tr->frameno != frameno) continue;
			if (sim_window_findcross(tr, valh, &t) && (trh == NOTRACE || fabs(t - *cursor) < fabs(th - *cursor)))
			{
				th = t;
				trh = tr;
			}
		}

		/* align cursor time to crosspoint */
		if (trh != NOTRACE)
		{
			if (labs(sim_window_timetoxpos(*cursor) - sim_window_timetoxpos(th)) <= 10)
				*cursor = th;
			else
				trh = NOTRACE;
		}
	}
	sim_window_drawcursors(wavewin);
	if (trh != NOTRACE)
	{
		/* highlight crossing trace and threshold line */
		sim_window_setmask(LAYERH);
		sim_window_setcolor(HIGHLIT);
		sim_window_moveto(sim_window_wavexsigstart, yh);
		sim_window_drawto(wavewin, WAVEXSIGEND, yh, 1);
		sim_window_plottrace(trh, wavewin, 2, 0, 0);
	}
}

void sim_window_hthumbtrackingcallback(INTBIG delta)
{
	REGISTER INTBIG thumbarea, j, tr;
	double mintime, maxtime, range, truerange;

	if (delta == 0) return;
	sim_waveforminiththumbpos += delta;
	thumbarea = sim_waveformwindow->usehx - sim_waveformwindow->uselx - DISPLAYSLIDERSIZE*2;
	sim_window_gettimeextents(&mintime, &maxtime);
	range = maxtime * 2.0;
	range = sim_waveformhrange;
	sim_window_inittraceloop();
	for(j=0; ; j++)
	{
		tr = sim_window_nexttraceloop();
		if (tr == 0) break;
		sim_window_gettimerange(tr, &mintime, &maxtime);
		truerange = maxtime - mintime;

		mintime = (sim_waveforminiththumbpos -
			(sim_waveformwindow->uselx+DISPLAYSLIDERSIZE+2)) * range / thumbarea;
		if (mintime < 0.0) mintime = 0.0;
		maxtime = mintime + truerange;

		sim_window_settimerange(tr, mintime, maxtime);
	}
	if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
		sim_window_redraw();
}

void sim_window_vthumbtrackingcallback(INTBIG delta)
{
	REGISTER INTBIG thumbarea, point;

	sim_window_deltasofar += delta;
	thumbarea = sim_waveformwindow->usehy - sim_waveformwindow->usely - DISPLAYSLIDERSIZE*2;
	point = (sim_waveformwindow->usehy - DISPLAYSLIDERSIZE -
		(sim_window_initialthumb + sim_window_deltasofar)) *
			sim_window_lines / thumbarea;
	if (point < 0) point = 0;
	if (point > sim_window_lines-sim_window_visframes)
		point = sim_window_lines-sim_window_visframes;
	if (point == sim_window_topframes) return;
	sim_window_topframes = point;
	sim_window_redisphandler(sim_waveformwindow);
}

/*
 * Routine for drawing highlight around dragged trace name.
 */
void sim_window_drawtracedrag(WINDOWPART *wavewin, BOOLEAN on)
{
	INTBIG y_pos, temp_trace_spacing, frameno, lx, hx, ly, hy;

	/* compute positions */
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	frameno = sim_window_tracedragtr->frameno - sim_window_topframes;
	if (frameno < 0 || frameno >= sim_window_visframes) return;
	y_pos = (sim_window_visframes - 1 - frameno) * temp_trace_spacing +
		WAVEYSIGBOT + sim_window_tracedragyoff;
	ly = y_pos + sim_window_tracedragtr->nameoff + 1;
	hy = ly + sim_window_txthei;
	lx = 0;
	hx = sim_window_wavexnameend;

	sim_window_setmask(LAYERH);
	if (on) sim_window_setcolor(HIGHLIT); else
		sim_window_setcolor(ALLOFF);
	sim_window_moveto(lx, ly);
	sim_window_drawto(wavewin, hx, ly, 0);
	sim_window_drawto(wavewin, hx, hy, 0);
	sim_window_drawto(wavewin, lx, hy, 0);
	sim_window_drawto(wavewin, lx, ly, 0);
}

/*
 * Routine for tracking the drag of signal names in the waveform window.
 */
BOOLEAN sim_window_eachdown(INTBIG x, INTBIG y)
{
	REGISTER WINDOWPART *wavewin;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return(FALSE);

	sim_window_scaletowindow(wavewin, &x, &y);
	sim_window_drawtracedrag(wavewin, FALSE);
	sim_window_tracedragyoff = y - sim_window_initialy;
	sim_window_drawtracedrag(wavewin, TRUE);
	return(FALSE);
}

/*
 * Routine for drawing highlight around selected trace names.
 */
void sim_window_drawtraceselect(WINDOWPART *wavewin, INTBIG y1, INTBIG y2, BOOLEAN on)
{
	INTBIG x, ly, hy;

	/* compute positions */
	if (y1 == y2) return;
	ly = mini(y1, y2);
	hy = maxi(y1, y2);
	x = sim_window_initialx;

	sim_window_setmask(LAYERH);
	if (!on) sim_window_setcolor(ALLOFF); else
		sim_window_setcolor(HIGHLIT);
	sim_window_moveto(x, ly);
	sim_window_drawto(wavewin, x, hy, 0);
}

/*
 * Routine for tracking the selection of signal names in the waveform window.
 */
BOOLEAN sim_window_seleachdown(INTBIG x, INTBIG y)
{
	REGISTER WINDOWPART *wavewin;
	INTBIG lowy, highy, ind, i, redo, ypos, temp_trace_spacing;
	REGISTER TRACE *tr;

	/* get the waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return(FALSE);

	sim_window_scaletowindow(wavewin, &x, &y);

	/* undraw former selection line */
	sim_window_drawtraceselect(wavewin, sim_window_currentdragy, sim_window_initialy, FALSE);
	if (y < sim_window_initialy)
	{
		lowy = y;    highy = sim_window_initialy;
	} else
	{
		lowy = sim_window_initialy;    highy = y;
	}

	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		tr->flags &= ~TRACEMARK;
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		ind = tr->frameno - sim_window_topframes;
		ypos = (sim_window_visframes - 1 - ind) * temp_trace_spacing + WAVEYSIGBOT + tr->nameoff;
		if (ypos > highy || ypos + sim_window_txthei < lowy) continue;
		tr->flags |= TRACEMARK;
	}

	/* see if what is highlighted has changed */
	redo = 0;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		for(i=0; i<sim_window_highlightedtracecount; i++)
			if (sim_window_highlightedtraces[i] == tr) break;
		if (i < sim_window_highlightedtracecount)
		{
			/* this signal was highlighted */
			if ((tr->flags&TRACEMARK) == 0) redo++;
		} else
		{
			/* this signal was not highlighted */
			if ((tr->flags&TRACEMARK) != 0) redo++;
		}
	}

	/* redo highlighting if it changed */
	if (redo != 0)
	{
		sim_window_cleartracehighlight();
		for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
			if ((tr->flags&TRACEMARK) != 0) sim_window_addhighlighttrace((INTBIG)tr);
	}

	/* show what is dragged out */
	sim_window_currentdragy = y;
	sim_window_drawtraceselect(wavewin, sim_window_currentdragy, sim_window_initialy, TRUE);
	return(FALSE);
}

/*
 * Routine called when vertical slider parts are clicked.
 */
BOOLEAN sim_window_verticalslider(INTBIG x, INTBIG y)
{
	INTBIG newtopframe;

	if (x >= sim_waveformwindow->uselx) return(FALSE);
	switch (sim_waveformsliderpart)
	{
		case 0: /* down arrow clicked */
			if (y > sim_waveformwindow->usely + DISPLAYSLIDERSIZE) return(FALSE);
			if (y < sim_waveformwindow->usely) return(FALSE);
			if (sim_window_topframes + sim_window_visframes < sim_window_lines)
			{
				sim_window_topframes++;
				sim_window_redisphandler(sim_waveformwindow);
			}
			break;
		case 1: /* below thumb */
			if (y > sim_waveformwindow->thumbly) return(FALSE);
			newtopframe = sim_window_topframes + sim_window_visframes - 1;
			if (newtopframe + sim_window_visframes > sim_window_lines)
				newtopframe = sim_window_lines - sim_window_visframes;
			if (sim_window_topframes == newtopframe) break;
			sim_window_topframes = newtopframe;
			sim_window_redisphandler(sim_waveformwindow);
			break;
		case 2: /* on thumb (not handled here) */
			break;
		case 3: /* above thumb */
			if (y < sim_waveformwindow->thumbhy) return(FALSE);
			if (sim_window_topframes == 0) return(FALSE);
			sim_window_topframes -= sim_window_visframes - 1;
			if (sim_window_topframes < 0) sim_window_topframes = 0;
			sim_window_redisphandler(sim_waveformwindow);
			break;
		case 4: /* up arrow clicked */
			if (y < sim_waveformwindow->usehy - DISPLAYSLIDERSIZE) return(FALSE);
			if (sim_window_topframes > 0)
			{
				sim_window_topframes--;
				sim_window_redisphandler(sim_waveformwindow);
			}
			break;
	}
	return(FALSE);
}

/*
 * Routine called when horizontal slider parts are clicked.
 */
BOOLEAN sim_window_horizontalslider(INTBIG x, INTBIG y)
{
	double range;
	REGISTER TRACE *tr;

	tr = (TRACE *)sim_window_firstrace;
	if (tr == NOTRACE) return(FALSE);
	if (y >= sim_waveformwindow->usely) return(FALSE);
	switch (sim_waveformsliderpart)
	{
		case 0: /* left arrow clicked */
			if (x > sim_waveformwindow->uselx + DISPLAYSLIDERSIZE*11) return(FALSE);
			if (tr->mintime <= 0.0) break;
			range = tr->maxtime - tr->mintime;
			tr->mintime -= range/DISPLAYSMALLSHIFT;
			tr->maxtime -= range/DISPLAYSMALLSHIFT;
			if (tr->mintime < 0.0)
			{
				tr->maxtime -= tr->mintime;
				tr->mintime = 0.0;
			}
			if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
				sim_window_redraw();
			break;
		case 1: /* left of thumb */
			if (x > sim_waveformwindow->thumblx) return(FALSE);
			if (tr->mintime <= 0.0) break;
			range = tr->maxtime - tr->mintime;
			tr->mintime -= range/DISPLAYLARGESHIFT;
			tr->maxtime -= range/DISPLAYLARGESHIFT;
			if (tr->mintime < 0.0)
			{
				tr->maxtime -= tr->mintime;
				tr->mintime = 0.0;
			}
			if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
				sim_window_redraw();
			break;
		case 2: /* on thumb (not handled here) */
			break;
		case 3: /* right of thumb */
			if (x < sim_waveformwindow->thumbhx) return(FALSE);
			range = tr->maxtime - tr->mintime;
			tr->mintime += range/DISPLAYLARGESHIFT;
			tr->maxtime += range/DISPLAYLARGESHIFT;
			if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
				sim_window_redraw();
			break;
		case 4: /* right arrow clicked */
			if (x < sim_waveformwindow->usehx - DISPLAYSLIDERSIZE) return(FALSE);
			range = tr->maxtime - tr->mintime;
			tr->mintime += range/DISPLAYSMALLSHIFT;
			tr->maxtime += range/DISPLAYSMALLSHIFT;
			if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
				sim_window_redraw();
			break;
	}
	return(FALSE);
}

void sim_window_buttonhandler(WINDOWPART *w, INTBIG button, INTBIG inx, INTBIG iny)
{
	INTBIG x, y, xpos, ypos, j, ind, temp_trace_spacing, traceframesmoved, important;
	REGISTER INTBIG lx, hx, ly, hy, ind1, ind2, y_min, y_max, frame;
	CHAR *par[2], *butname;
	double range, highdisp, lowdisp, mintime, maxtime, endtime;
	float flow, fhigh;
	BOOLEAN areaselect;
	REGISTER TRACE *tr, *otr, *otrprev, *nexttr, *addloc,
		*buschain, *endbuschain;

	/* changes to the mouse-wheel are handled by the user interface */
	if (wheelbutton(button))
	{
		us_buttonhandler(w, button, inx, iny);
		return;
	}
	ttynewcommand();

	/* if in distance-measurement mode, track measurement */
	if ((us_state&MEASURINGDISTANCE) != 0)
	{
		if ((us_state&MEASURINGDISTANCEINI) != 0)
		{
			us_state &= ~MEASURINGDISTANCEINI;
			sim_window_distanceinit(w);
		}
		trackcursor(FALSE, us_ignoreup, us_nullvoid, sim_window_distancedown,
			us_stoponchar, sim_window_distanceup, TRACKDRAGGING);
		return;
	}

	/* hit in slider on left */
	if (inx < w->uselx)
	{
		if (iny < w->usely)
		{
			/* toggle the cell explorer */
			par[0] = "explore";
			us_window(1, par);
			return;
		}
		if (iny <= w->usely + DISPLAYSLIDERSIZE)
		{
			/* hit in bottom arrow: go down 1 (may repeat) */
			sim_waveformwindow = w;
			sim_waveformsliderpart = 0;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_verticalslider, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (iny < w->thumbly)
		{
			/* hit below thumb: go down lots (may repeat) */
			sim_waveformwindow = w;
			sim_waveformsliderpart = 1;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_verticalslider, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (iny <= w->thumbhy)
		{
			/* hit on thumb: track thumb motion */
			sim_waveformwindow = w;
			sim_window_deltasofar = 0;
			sim_window_initialthumb = w->thumbhy;
			us_vthumbbegin(iny, w, w->uselx-DISPLAYSLIDERSIZE, w->usely, w->usehy,
				TRUE, sim_window_vthumbtrackingcallback);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_vthumbdown, us_nullchar,
				us_vthumbdone, TRACKNORMAL); 
			return;
		}
		if (iny < w->usehy - DISPLAYSLIDERSIZE)
		{
			/* hit above thumb: go up lots (may repeat) */
			sim_waveformwindow = w;
			sim_waveformsliderpart = 3;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_verticalslider, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}

		/* hit in up arrow: go up 1 (may repeat) */
		sim_waveformwindow = w;
		sim_waveformsliderpart = 4;
		trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_verticalslider, us_nullchar,
			us_nullvoid, TRACKNORMAL);
		return;
	}

	/* hit in slider on bottom */
	if (iny < w->usely)
	{
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*1)
		{
			/* hit in blank button */
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*2)
		{
			/* hit in rewind button */
			sim_window_playing = VCRSTOP;
			sim_window_maincursor = 0.0;
			sim_window_drawcursors(w);
			sim_window_updatelayoutwindow();
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*3)
		{
			/* hit in play backwards button */
			sim_window_playing = VCRPLAYBACKWARD;
			sim_window_lastadvance = ticktime();
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*4)
		{
			/* hit in stop button */
			sim_window_playing = VCRSTOP;
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*5)
		{
			/* hit in play button */
			sim_window_playing = VCRPLAYFOREWARD;
			sim_window_lastadvance = ticktime();
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*6)
		{
			/* hit in advance to end button */
			endtime = 0.0;
			for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
				if (tr->maxtime > endtime) endtime = tr->maxtime;
			sim_window_playing = VCRSTOP;
			sim_window_maincursor = endtime;
			sim_window_drawcursors(w);
			sim_window_updatelayoutwindow();
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*7)
		{
			/* hit in blank button */
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*8)
		{
			/* hit in faster button */
			j = sim_window_advancespeed / 4;
			if (j <= 0) j = 1;
			sim_window_advancespeed += j;
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*9)
		{
			/* hit in slower button */
			j = sim_window_advancespeed / 4;
			if (j <= 0) j = 1;
			sim_window_advancespeed -= j;
			if (sim_window_advancespeed <= 0) sim_window_advancespeed = 1;
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*10)
		{
			/* hit in blank button */
			return;
		}
		if (inx <= w->uselx + DISPLAYSLIDERSIZE*11)
		{
			/* hit in left arrow: small shift down in time (may repeat) */
			sim_waveformwindow = w;
			sim_waveformsliderpart = 0;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_horizontalslider, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (inx < w->thumblx)
		{
			/* hit to left of thumb: large shift down in time (may repeat) */
			sim_waveformwindow = w;
			sim_waveformsliderpart = 1;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_horizontalslider, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}
		if (inx <= w->thumbhx)
		{
			/* hit on thumb: track thumb motion */
			sim_waveformwindow = w;
			sim_window_gettimeextents(&mintime, &maxtime);
			sim_waveformhrange = maxtime * 2.0;
			sim_window_getaveragetimerange(&mintime, &maxtime);
			if (maxtime*2.0 > sim_waveformhrange) sim_waveformhrange = maxtime*2.0;

			sim_waveforminiththumbpos = w->thumblx;
			us_hthumbbegin(inx, w, w->usely, w->uselx, w->usehx,
				sim_window_hthumbtrackingcallback);
			trackcursor(FALSE, us_nullup, us_nullvoid, us_hthumbdown, us_nullchar,
				us_hthumbdone, TRACKNORMAL); 
			return;
		}
		if (inx < w->usehx - DISPLAYSLIDERSIZE)
		{
			/* hit to right of thumb: large shift up in time (may repeat) */
			sim_waveformwindow = w;
			sim_waveformsliderpart = 3;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_horizontalslider, us_nullchar,
				us_nullvoid, TRACKNORMAL);
			return;
		}

		/* hit in right arrow: small shift up in time (may repeat) */
		sim_waveformwindow = w;
		sim_waveformsliderpart = 4;
		trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_horizontalslider, us_nullchar,
			us_nullvoid, TRACKNORMAL);
		return;
	}

	sim_window_setviewport(w);
	x = inx;   y = iny;
	sim_window_scaletowindow(w, &x, &y);

	/* see if the divider between names and signals was hit */
	if (abs(x - sim_window_wavexbar) <= 3 && y >= 560)
	{
		sim_window_dividerinit(w);
		sim_window_draghighoff = TRUE;
		for(j=0; j<sim_window_highlightedtracecount; j++)
			sim_window_highlightname(w, sim_window_highlightedtraces[j], FALSE);
		trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_dividerdown, us_nullchar,
			sim_window_dividerdone, TRACKNORMAL);
		if (sim_window_draghighoff)
		{
			for(j=0; j<sim_window_highlightedtracecount; j++)
				sim_window_highlightname(w, sim_window_highlightedtraces[j], TRUE);
		}
		return;
	}

	/* see if a signal name was hit */
	if (x <= sim_window_wavexnameend)
	{
		temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
		ind = sim_window_visframes - 1 - (y - WAVEYSIGBOT) / temp_trace_spacing;
		for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		{
			if ((tr->flags&TRACENOTDRAWN) == 0)
			{
				if (tr->frameno == ind + sim_window_topframes)
				{
					ypos = (sim_window_visframes - 1 - ind) * temp_trace_spacing + WAVEYSIGBOT + tr->nameoff;
					if (y >= ypos && y <= ypos + sim_window_txthei) break;
				}
			}
		}
		if (tr == NOTRACE)
		{
			/* not over any signal: drag out selection of signal names */
			sim_window_initialx = x;
			sim_window_initialy = y;
			sim_window_currentdragy = y;
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_seleachdown,
				us_nullchar, us_nullvoid, TRACKDRAGGING);
			sim_window_drawtraceselect(w, sim_window_currentdragy, sim_window_initialy, FALSE);
			return;
		}

		/* see if it is a double-click on a bus name */
		if (doublebutton(button))
		{
			if ((tr->flags&TRACETYPE) == TRACEISBUS)
			{
				/* closed bus: open it up */
				j = 0;
				for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
				{
					if (otr->buschannel != tr) continue;
					otr->flags &= ~TRACENOTDRAWN;
					otr->frameno = --j;
				}
				tr->flags = (tr->flags & ~TRACETYPE) | TRACEISOPENBUS;

				/* renumber lines in the display */
				sim_window_renumberlines();

				/* adjust the number of visible lines */
				sim_window_setviewport(w);

				sim_window_redraw();
				return;
			}
			if ((tr->flags&TRACETYPE) == TRACEISOPENBUS)
			{
				/* opened bus: close it up */
				for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
					if (otr->buschannel == tr) otr->flags |= TRACENOTDRAWN;
				tr->flags = (tr->flags & ~TRACETYPE) | TRACEISBUS;

				/* renumber lines in the display */
				sim_window_renumberlines();

				sim_window_redraw();
				return;
			}
			return;
		}

		/* see if the signal shares a line with any others */
		for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
		{
			if ((otr->flags&TRACENOTDRAWN) != 0) continue;
			if (otr != tr && otr->frameno == tr->frameno) break;
		}
		if (otr == NOTRACE)
		{
			/* not a shared signal: handle dragging of the name */
			sim_window_tracedragyoff = 0;
			sim_window_initialy = y;
			sim_window_tracedragtr = tr;
			sim_window_drawtracedrag(w, TRUE);
			trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_eachdown,
				us_nullchar, us_nullvoid, TRACKDRAGGING);
			sim_window_drawtracedrag(w, FALSE);

			/* see if the signal name was dragged to a new location */
			traceframesmoved = sim_window_tracedragyoff / temp_trace_spacing;
			if (traceframesmoved != 0)
			{
				/* erase highlighting */
				sim_window_cleartracehighlight();

				/* find which signal name it was dragged to */
				otrprev = NOTRACE;
				for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
				{
					if ((otr->flags&TRACENOTDRAWN) == 0)
					{
						if (otr->frameno == tr->frameno - traceframesmoved) break;
					}
					if (otr != tr) otrprev = otr;
				}
				if (otr != NOTRACE)
				{
					/* pull out the selected signal */
					sim_window_removetrace(tr);

					/* insert signal in new location */
					if (tr->frameno < otr->frameno)
					{
						sim_window_addtrace(tr, otr);
					} else
					{
						sim_window_addtrace(tr, otrprev);
					}

					/* move any bus signals inside of this */
					buschain = endbuschain = NOTRACE;
					for(otr = sim_window_firstrace; otr != NOTRACE; otr = nexttr)
					{
						nexttr = otr->nexttrace;
						if (otr->buschannel == tr)
						{
							sim_window_removetrace(otr);
							if (endbuschain == NOTRACE) endbuschain = buschain = otr; else
							{
								endbuschain->nexttrace = otr;
								endbuschain = otr;
							}
							otr->nexttrace = NOTRACE;
						}
					}
					addloc = tr;
					for(otr = buschain; otr != NOTRACE; otr = nexttr)
					{
						nexttr = otr->nexttrace;
						sim_window_addtrace(otr, addloc);
						addloc = otr;
					}

					/* renumber lines in the display */
					sim_window_renumberlines();

					/* redisplay window */
					sim_window_redraw();

					/* save the new configuration */
					sim_window_savesignalorder();
				}
				sim_window_addhighlighttrace((INTBIG)tr);
				par[0] = x_("highlight");
				telltool(net_tool, 1, par);
				return;
			}
		}

		/* highlight the selected signal */
		if (!shiftbutton(button))
		{
			/* shift not held: simply highlight this signal */
			sim_window_cleartracehighlight();
			sim_window_addhighlighttrace((INTBIG)tr);
		} else
		{
			/* complement state of this signal */
			for(j=0; j<sim_window_highlightedtracecount; j++)
				if (tr == sim_window_highlightedtraces[j]) break;
			if (j < sim_window_highlightedtracecount)
			{
				/* signal already highlighted: unhighlight it */
				sim_window_deletehighlighttrace((INTBIG)tr);
			} else
			{
				/* signal new: add to highlight */
				sim_window_addhighlighttrace((INTBIG)tr);
			}

		}
		par[0] = x_("highlight");
		telltool(net_tool, 1, par);
		return;
	}

	/* see if an area-select was requested */
	butname = buttonname(button, &important);
	areaselect = FALSE;
#ifdef WIN32
	if (namesame(butname, x_("SARIGHT")) == 0) areaselect = TRUE;
#endif
#ifdef MACOS
	if (namesame(butname, x_("SMOButton")) == 0) areaselect = TRUE;
#endif
#ifdef ONUNIX
	if (namesame(butname, x_("SMRIGHT")) == 0) areaselect = TRUE;
#endif
	if (areaselect)
	{
		/* select an area of the waveform window */
		sim_window_zoominix = sim_window_zoomothx = x;
		sim_window_zoominiy = sim_window_zoomothy = y;
		sim_window_drawareazoom(w, TRUE, sim_window_zoominix, sim_window_zoominiy,
			sim_window_zoomothx, sim_window_zoomothy);
		trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_zoomdown,
			us_nullchar, us_nullvoid, TRACKDRAGGING);
		sim_window_drawareazoom(w, FALSE, sim_window_zoominix, sim_window_zoominiy,
			sim_window_zoomothx, sim_window_zoomothy);

		/* find the area */
		lx = mini(sim_window_zoominix, sim_window_zoomothx);
		hx = maxi(sim_window_zoominix, sim_window_zoomothx);
		ly = mini(sim_window_zoominiy, sim_window_zoomothy);
		hy = maxi(sim_window_zoominiy, sim_window_zoomothy);
		lowdisp = sim_window_xpostotime(lx);
		highdisp = sim_window_xpostotime(hx);
		range = highdisp - lowdisp;

		/* zoom into it horizontally */
		if (sim_window_maincursor < lowdisp || sim_window_maincursor > highdisp)
			sim_window_setmaincursor(lowdisp);
		if (sim_window_extensioncursor < lowdisp || sim_window_extensioncursor > highdisp)
			sim_window_setextensioncursor(highdisp);
		sim_window_settimerange(0, lowdisp-range/15.0f, highdisp+range/15.0f);

		/* zoom vertically as well */
		temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
		ind1 = sim_window_visframes - 1 - (ly - WAVEYSIGBOT) / temp_trace_spacing;
		ind2 = sim_window_visframes - 1 - (hy - WAVEYSIGBOT) / temp_trace_spacing;
		y_min = (sim_window_visframes - 1 - ind1) * temp_trace_spacing + WAVEYSIGBOT;
		y_max = y_min + temp_trace_spacing - WAVEYSIGGAP;
		if (ind1 == ind2)
		{
			/* dragging happend all in one frame, see if it is analog */
			for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
			{
				if ((tr->flags&TRACENOTDRAWN) != 0) continue;
				if (tr->frameno != ind1 + sim_window_topframes) continue;
				if ((tr->flags & TRACETYPE) == TRACEISANALOG) break;
			}
			if (tr != NOTRACE)
			{
				/* invert the display computation to get real units from screen coordinates */
				frame = ind1 + sim_window_topframes;
				flow = ((ly-y_min) / (float)(y_max - y_min)) * sim_window_get_extanarange(frame) + sim_window_get_botval(frame);
				fhigh = ((hy-y_min) / (float)(y_max - y_min)) * sim_window_get_extanarange(frame) + sim_window_get_botval(frame);
				range = fhigh - flow;
				sim_window_set_analow(frame, flow-(float)range/15.0f);
				sim_window_set_anahigh(frame, fhigh+(float)range/15.0f);
			}
		}
		sim_window_redisphandler(w);
		return;
	}	

	/* see if a cursor was hit */
	xpos = sim_window_timetoxpos(sim_window_maincursor);
	if (abs(x-xpos) < 10 && y < 560)
	{
		/* main cursor hit */
		trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_eachwdown,
			us_nullchar, us_nullvoid, TRACKDRAGGING);
		return;
	}
	xpos = sim_window_timetoxpos(sim_window_extensioncursor);
	if (abs(x-xpos) < 10 && y < 560)
	{
		/* extension cursor hit */
		trackcursor(FALSE, us_nullup, us_nullvoid, sim_window_eachbdown,
			us_nullchar, us_nullvoid, TRACKDRAGGING);
		return;
	}

	/* see if the line of a signal was hit */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if (sim_window_plottrace(tr, w, 1, x, y)) break;
	}
	if (tr != NOTRACE)
	{
		/* highlight the selected signal */
		if (!shiftbutton(button))
		{
			/* shift not held: simply highlight this signal */
			sim_window_cleartracehighlight();
			sim_window_addhighlighttrace((INTBIG)tr);
		} else
		{
			/* complement state of this signal */
			for(j=0; j<sim_window_highlightedtracecount; j++)
				if (tr == sim_window_highlightedtraces[j]) break;
			if (j < sim_window_highlightedtracecount)
			{
				/* signal already highlighted: unhighlight it */
				sim_window_deletehighlighttrace((INTBIG)tr);
			} else
			{
				/* signal new: add to highlight */
				sim_window_addhighlighttrace((INTBIG)tr);
			}

		}
		par[0] = x_("highlight");
		telltool(net_tool, 1, par);
	} else
	{
		sim_window_cleartracehighlight();
	}
}

/*
 * Routine called when VCR is playing.  Advances time if appropriate.
 */
void sim_window_advancetime(void)
{
	REGISTER UINTBIG curtime;
	double dtime;
	REGISTER TRACE *tr;
	REGISTER WINDOWPART *wavewin;

	if (sim_window_playing == VCRSTOP) return;

	/* see if there is a waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;

	/* see if it is time to advance the VCR */
	curtime = ticktime();
	if (curtime - sim_window_lastadvance < 5) return;
	sim_window_lastadvance = curtime;

	/* determine how much the time changes */
	tr = sim_window_firstrace;
	if (tr == NOTRACE) return;
	dtime = sim_window_advancespeed * (tr->maxtime-tr->mintime) / (WAVEXSIGEND-sim_window_wavexsigstart);

	if (sim_window_playing == VCRPLAYFOREWARD)
	{
		/* advance time forward */
		sim_window_maincursor += dtime;
	} else
	{
		/* advance time backward */
		sim_window_maincursor -= dtime;
		if (sim_window_maincursor < 0.0) sim_window_maincursor = 0.0;
	}
	sim_window_drawcursors(wavewin);
	sim_window_updatelayoutwindow();
}

/*
 * Routine to initialize the display of measurements.
 */
void sim_window_distanceinit(WINDOWPART *w)
{
	sim_window_measureframe = -1;
	sim_waveformwindow = w;
}

/*
 * Routine called each time the cursor advances during measurement.
 */
BOOLEAN sim_window_distancedown(INTBIG x, INTBIG y)
{
	REGISTER INTBIG frame, y_min, y_max, slot, dragx, dragy, temp_trace_spacing;
	INTBIG i1, i2;
	double xval, yval, position;
	float mintime, maxtime;
	static CHAR distancemessage[200];

	/* initialize polygon */
	(void)needstaticpolygon(&sim_window_dragpoly, 4, sim_tool->cluster);
	sim_window_dragpoly->desc = &sim_window_desc;
	sim_window_desc.bits = LAYERH;

	/* determine coordinates of this measurement */
	sim_window_scaletowindow(sim_waveformwindow, &x, &y);
	frame = sim_window_ypostoval(y, &yval);
	xval = sim_window_xpostotime(x);

	/* convert coordinates back to screen */
	dragx = sim_window_timetoxpos(xval);
	slot = frame - sim_window_topframes;
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	y_min = (sim_window_visframes - 1 - slot) * temp_trace_spacing + WAVEYSIGBOT;
	y_max = y_min + temp_trace_spacing - WAVEYSIGGAP;
	position = (double)(y_max - y_min);
	position *= (yval - sim_window_get_botval(frame)) / sim_window_get_extanarange(frame);
	dragy = (INTBIG)position + y_min;

	if (sim_window_measureframe < 0)
	{
		sim_window_firstmeasurex = dragx;   sim_window_firstmeasurey = dragy;
		sim_window_firstmeasuretime = xval;
		sim_window_firstmeasureyval = yval;
		sim_window_measureframe = frame;
	} else
	{
		/* if it didn't actually move, go no further */
		if (dragx == sim_window_lastmeasurex && dragy == sim_window_lastmeasurey) return(FALSE);

		/* if it changed frames, go no further */
		if (sim_window_measureframe != frame) return(FALSE);

		/* undraw the distance */
		sim_window_desc.col = 0;
		sim_window_drawdistance();
	}
	sim_window_lastmeasurex = dragx;   sim_window_lastmeasurey = dragy;

	/* draw the new measurement */
	sim_window_desc.col = HIGHLIT;
	TDCLEAR(sim_window_dragpoly->textdescript);
	TDSETSIZE(sim_window_dragpoly->textdescript, TXTSETPOINTS(12));
	sim_window_dragpoly->tech = el_curtech;

	mintime = sim_window_mintime;
	maxtime = sim_window_maxtime;
	(void)sim_window_sensible_value(&maxtime, &mintime, &i1, &i2, 5);
	(void)esnprintf(distancemessage, 200, x_("dX=%s dY=%g"),
		sim_windowconvertengineeringnotation2(xval - sim_window_firstmeasuretime, i2),
			yval - sim_window_firstmeasureyval);
	sim_window_dragpoly->string = distancemessage;
	sim_window_drawdistance();
	sim_window_measureframe = 1;
	return(FALSE);
}

/*
 * Routine to display the current measurement information.
 */
void sim_window_drawdistance(void)
{
	INTBIG fx, fy, tx, ty;

	sim_window_dragpoly->count = 1;
	sim_window_dragpoly->style = TEXTBOTLEFT;
	fx = sim_window_firstmeasurex;   fy = sim_window_firstmeasurey;
	tx = sim_window_lastmeasurex;    ty = sim_window_lastmeasurey;
	if (!clipline(&fx, &fy, &tx, &ty, sim_waveformwindow->screenlx, sim_waveformwindow->screenhx,
		sim_waveformwindow->screenly, sim_waveformwindow->screenhy))
	{
		sim_window_dragpoly->xv[0] = (fx+tx) / 2;
		sim_window_dragpoly->yv[0] = (fy+ty) / 2;
		us_showpoly(sim_window_dragpoly, sim_waveformwindow);
	}

	/* draw the line */
	sim_window_dragpoly->xv[0] = sim_window_firstmeasurex;
	sim_window_dragpoly->yv[0] = sim_window_firstmeasurey;
	sim_window_dragpoly->xv[1] = sim_window_lastmeasurex;
	sim_window_dragpoly->yv[1] = sim_window_lastmeasurey;
	sim_window_dragpoly->count = 2;
	sim_window_dragpoly->style = OPENED;
	us_showpoly(sim_window_dragpoly, sim_waveformwindow);

	/* draw crosses at the end */
	sim_window_dragpoly->xv[0] = sim_window_firstmeasurex;
	sim_window_dragpoly->yv[0] = sim_window_firstmeasurey;
	sim_window_dragpoly->count = 1;
	sim_window_dragpoly->style = CROSS;
	us_showpoly(sim_window_dragpoly, sim_waveformwindow);
	sim_window_dragpoly->xv[0] = sim_window_lastmeasurex;
	sim_window_dragpoly->yv[0] = sim_window_lastmeasurey;
	us_showpoly(sim_window_dragpoly, sim_waveformwindow);
}

/*
 * Routine called when the button is released during measurement.
 */
void sim_window_distanceup(void)
{
	if (sim_window_measureframe >= 0)
	{
		/* leave the results in the messages window */
		ttyputmsg(x_("%s"), sim_window_dragpoly->string);
	}
}

/*
 * Routine to initialize the dragging of the divider between the signal names and the waveforms.
 */
void sim_window_dividerinit(WINDOWPART *w)
{
	sim_waveformwindow = w;
	sim_window_dragshown = FALSE;
}

/*
 * Routine called whenever the mouse moves during dragging of the divider between the signal
 * names and the waveforms.
 */
BOOLEAN sim_window_dividerdown(INTBIG x, INTBIG y)
{
	INTBIG dummyx, lowy, highy;
	Q_UNUSED( y );

	dummyx = 0;
	lowy = 31;
	highy = WAVEYTEXTTOP;
	sim_window_mapcoord(sim_waveformwindow, &dummyx, &lowy);
	sim_window_mapcoord(sim_waveformwindow, &dummyx, &highy);
	if (sim_window_dragshown)
	{
		/* if it didn't actually move, go no further */
		if (x == sim_window_draglastx) return(FALSE);

		/* undraw the line */
		sim_window_desc.bits = LAYERH;
		sim_window_desc.col = ALLOFF;
		screendrawline(sim_waveformwindow, sim_window_draglastx, lowy,
			sim_window_draglastx, highy, &sim_window_desc, 0);
	}

	sim_window_draglastx = x;
	if (sim_window_draglastx >= sim_waveformwindow->usehx)
		sim_window_draglastx = sim_waveformwindow->usehx-1;
	if (sim_window_draglastx < sim_waveformwindow->uselx)
		sim_window_draglastx = sim_waveformwindow->uselx;

	/* draw the outline */
	sim_window_desc.bits = LAYERH;
	sim_window_desc.col = HIGHLIT;
	screendrawline(sim_waveformwindow, sim_window_draglastx, lowy,
		sim_window_draglastx, highy, &sim_window_desc, 0);

	sim_window_dragshown = TRUE;
	return(FALSE);
}

/*
 * Routine called when the mouse is released after dragging of the divider between the signal
 * names and the waveforms.
 */
void sim_window_dividerdone(void)
{
	REGISTER INTBIG newbar;
	INTBIG dummyx, lowy, highy;

	if (sim_window_dragshown)
	{
		/* undraw the outline */
		sim_window_desc.bits = LAYERH;
		sim_window_desc.col = ALLOFF;

		dummyx = 0;
		lowy = 31;
		highy = 560;
		sim_window_mapcoord(sim_waveformwindow, &dummyx, &lowy);
		sim_window_mapcoord(sim_waveformwindow, &dummyx, &highy);
		screendrawline(sim_waveformwindow, sim_window_draglastx, lowy,
			sim_window_draglastx, highy, &sim_window_desc, 0);

		/* compute new bar position and redraw */
		newbar = muldiv(sim_window_draglastx - sim_waveformwindow->uselx,
			sim_waveformwindow->screenhx - sim_waveformwindow->screenlx,
				sim_waveformwindow->usehx - sim_waveformwindow->uselx) +
					sim_waveformwindow->screenlx;
		if (newbar != sim_window_wavexbar && newbar > 20 && newbar < WAVEXSIGEND-10)
		{
			sim_window_wavexbar = newbar;
			sim_window_wavexsigstart = newbar+1;
			sim_window_wavexnameend = newbar-1;
			sim_window_wavexnameendana = newbar-13;
			sim_window_redisphandler(sim_waveformwindow);
			sim_window_draghighoff = FALSE;
		}
	}
}

void sim_window_termhandler(WINDOWPART *w)
{
	REGISTER WINDOWPART *schemwin;
	REGISTER WINDOWPART *ww;
	Q_UNUSED( w );

	/* see if there is another simulation window (because this one split) */
	for(ww = el_topwindowpart; ww != NOWINDOWPART; ww = ww->nextwindowpart)
	{
		if (ww == w) continue;
		if ((ww->state&WINDOWTYPE) == WAVEFORMWINDOW) break;
	}
	if (ww != NOWINDOWPART) return;

	/* disable simulation control in schematics/layout window */
	schemwin = sim_window_findschematics();
	if (schemwin == NOWINDOWPART) return;

	startobjectchange((INTBIG)schemwin, VWINDOWPART);
	(void)setval((INTBIG)schemwin, VWINDOWPART, x_("state"), schemwin->state &
		~WINDOWSIMMODE, VINTEGER);
	(void)setval((INTBIG)schemwin, VWINDOWPART, x_("charhandler"),
		(INTBIG)DEFAULTCHARHANDLER, VADDRESS);
	endobjectchange((INTBIG)schemwin, VWINDOWPART);
}

static INTBIG sim_rewinddots[] = {
	1,9, 1,8, 1,7, 1,6, 1,5, 1,4, 1,3, 1,2, 1,1,
	2,5,
	3,4, 3,5, 3,6,
	4,4, 4,5, 4,6,
	5,3, 5,4, 5,5, 5,6, 5,7,
	6,3, 6,4, 6,5, 6,6, 6,7,
	7,2, 7,3, 7,4, 7,5, 7,6, 7,7, 7,8,
	8,2, 8,3, 8,4, 8,5, 8,6, 8,7, 8,8,
	9,1, 9,2, 9,3, 9,4, 9,5, 9,6, 9,7, 9,8, 9,9,
	-1,-1};
static INTBIG sim_playbackwardsdots[] = {
	1,5,
	2,5,
	3,4, 3,5, 3,6,
	4,4, 4,5, 4,6,
	5,3, 5,4, 5,5, 5,6, 5,7,
	6,3, 6,4, 6,5, 6,6, 6,7,
	7,2, 7,3, 7,4, 7,5, 7,6, 7,7, 7,8,
	8,2, 8,3, 8,4, 8,5, 8,6, 8,7, 8,8,
	9,1, 9,2, 9,3, 9,4, 9,5, 9,6, 9,7, 9,8, 9,9,
	-1,-1};
static INTBIG sim_stopdots[] = {
	2,2, 2,3, 2,4, 2,5, 2,6, 2,7, 2,8,
	3,2, 3,3, 3,4, 3,5, 3,6, 3,7, 3,8,
	4,2, 4,3, 4,4, 4,5, 4,6, 4,7, 4,8,
	5,2, 5,3, 5,4, 5,5, 5,6, 5,7, 5,8,
	6,2, 6,3, 6,4, 6,5, 6,6, 6,7, 6,8,
	7,2, 7,3, 7,4, 7,5, 7,6, 7,7, 7,8,
	8,2, 8,3, 8,4, 8,5, 8,6, 8,7, 8,8,
	-1,-1};
static INTBIG sim_playforwarddots[] = {
	1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7, 1,8, 1,9,
	2,2, 2,3, 2,4, 2,5, 2,6, 2,7, 2,8,
	3,2, 3,3, 3,4, 3,5, 3,6, 3,7, 3,8,
	4,3, 4,4, 4,5, 4,6, 4,7,
	5,3, 5,4, 5,5, 5,6, 5,7,
	6,4, 6,5, 6,6,
	7,4, 7,5, 7,6,
	8,5,
	9,5,
	-1,-1};
static INTBIG sim_fastforwarddots[] = {
	1,1, 1,2, 1,3, 1,4, 1,5, 1,6, 1,7, 1,8, 1,9,
	2,2, 2,3, 2,4, 2,5, 2,6, 2,7, 2,8,
	3,2, 3,3, 3,4, 3,5, 3,6, 3,7, 3,8,
	4,3, 4,4, 4,5, 4,6, 4,7,
	5,3, 5,4, 5,5, 5,6, 5,7,
	6,4, 6,5, 6,6,
	7,4, 7,5, 7,6,
	8,5,
	9,1, 9,2, 9,3, 9,4, 9,5, 9,6, 9,7, 9,8, 9,9,
	-1,-1};
static INTBIG sim_fasterdots[] = {
	2,7,
	3,7, 3,8,
	4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7, 4,8, 4,9,
	5,7, 5,8,
	6,7,
	7,1, 7,2, 7,3, 7,4, 7,5,
	8,3, 8,5,
	9,5,
	-1,-1};
static INTBIG sim_slowerdots[] = {
	2,3,
	3,2, 3,3,
	4,1, 4,2, 4,3, 4,4, 4,5, 4,6, 4,7, 4,8, 4,9,
	5,2, 5,3,
	6,3,
	7,5, 7,7, 7,8, 7,9,
	8,5, 8,7, 8,9,
	9,5, 9,6, 9,7, 9,9,
	-1,-1};

/*
 * This procedure redraws the display, allowing for resize.
 */
void sim_window_redisphandler(WINDOWPART *w)
{
	INTBIG lx, hx, ly, hy;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	double range;
	REGISTER INTBIG thumbtop, thumbarea, thumbsize, extraroom;
	REGISTER TRACE *tr;
	REGISTER CHAR *simname;
	REGISTER void *infstr;

	sim_window_setviewport(w);

	/* clear the window */
	sim_window_desc.bits = LAYERA;
	sim_window_desc.col = ALLOFF;
	w->uselx -= DISPLAYSLIDERSIZE;
	w->usely -= DISPLAYSLIDERSIZE;
	lx = w->uselx;   hx = w->usehx;
	ly = w->usely;   hy = w->usehy;
	screendrawbox(w, lx, hx, ly, hy, &sim_window_desc);

	/* draw vertical slider on the left */
	us_drawverticalslider(w, lx-2, ly+DISPLAYSLIDERSIZE, hy, TRUE);
	if (sim_window_lines > 0)
	{
		thumbsize = sim_window_visframes * 100 / sim_window_lines;
		if (thumbsize >= 100) thumbsize = 100;
		if (thumbsize == 100 && sim_window_topframes == 0)
		{
			w->thumbly = 0;   w->thumbhy = -1;
		} else
		{
			thumbtop = sim_window_topframes * 100 / sim_window_lines;
			thumbarea = w->usehy - w->usely - DISPLAYSLIDERSIZE*2;

			w->thumbhy = w->usehy - DISPLAYSLIDERSIZE - thumbarea * thumbtop / 100;
			w->thumbly = w->thumbhy - thumbarea * thumbsize / 100;
			if (w->thumbhy > w->usehy-DISPLAYSLIDERSIZE-2) w->thumbhy = w->usehy-DISPLAYSLIDERSIZE-2;
			if (w->thumbly < w->usely+DISPLAYSLIDERSIZE*2+2) w->thumbly = w->usely+DISPLAYSLIDERSIZE*2+2;
		}
		us_drawverticalsliderthumb(w, lx-2, ly+DISPLAYSLIDERSIZE, hy, w->thumbly, w->thumbhy);
	}

	/* draw horizontal slider on the bottom */
	tr = (TRACE *)sim_window_firstrace;
	if (tr != NOTRACE)
	{
		extraroom = DISPLAYSLIDERSIZE * 10;
		us_drawhorizontalslider(w, ly+DISPLAYSLIDERSIZE, lx+DISPLAYSLIDERSIZE+extraroom, hx, 0);
		thumbarea = w->usehx - w->uselx - DISPLAYSLIDERSIZE*2 - extraroom;
		if (tr->mintime <= 0.0)
		{
			w->thumblx = w->uselx+extraroom+DISPLAYSLIDERSIZE*2+2;
			w->thumbhx = w->thumblx + thumbarea/2;
		} else
		{
			range = tr->maxtime * 2.0;
			w->thumblx = (INTBIG)(w->uselx+extraroom+DISPLAYSLIDERSIZE*2+2 + tr->mintime/range*thumbarea);
			w->thumbhx = (INTBIG)(w->uselx+extraroom+DISPLAYSLIDERSIZE*2+2 + tr->maxtime/range*thumbarea);
			if (w->thumbhx < w->thumblx + MINTHUMB) w->thumbhx = w->thumblx + MINTHUMB;
		}
		if (w->thumbhx > w->usehx-DISPLAYSLIDERSIZE-2) w->thumbhx = w->usehx-DISPLAYSLIDERSIZE-2;
		if (w->thumblx < w->uselx+DISPLAYSLIDERSIZE*2+2) w->thumblx = w->uselx+DISPLAYSLIDERSIZE*2+2;
		us_drawhorizontalsliderthumb(w, ly+DISPLAYSLIDERSIZE, lx+DISPLAYSLIDERSIZE+extraroom, hx, w->thumblx, w->thumbhx, 0);
	}

	/* draw the VCR controls in horizontal slider */
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*2, ly, sim_rewinddots);
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*3, ly, sim_playbackwardsdots);
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*4, ly, sim_stopdots);
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*5, ly, sim_playforwarddots);
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*6, ly, sim_fastforwarddots);
	us_drawslidercorner(w, lx+DISPLAYSLIDERSIZE*7, lx+DISPLAYSLIDERSIZE*7-1, ly, ly+DISPLAYSLIDERSIZE-1, TRUE);
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*8, ly, sim_fasterdots);
	us_drawwindowicon(w, lx+DISPLAYSLIDERSIZE*9, ly, sim_slowerdots);
	us_drawslidercorner(w, lx+DISPLAYSLIDERSIZE*10, lx+DISPLAYSLIDERSIZE*10-1, ly, ly+DISPLAYSLIDERSIZE-1, TRUE);

	/* draw explorer icon in corner of sliders */
	us_drawexplorericon(w, lx, ly);

	/* reset bounds */
	w->uselx += DISPLAYSLIDERSIZE;
	w->usely += DISPLAYSLIDERSIZE;

	/* draw red outline */
	if ((w->state&WINDOWSIMMODE) != 0)
		us_showwindowmode(w);

	/* write banner at top of window */
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(12));
	screensettextinfo(w, NOTECHNOLOGY, descript);
	sim_window_desc.col = el_colmentxt;
	simname = 0;
	switch (sim_window_state & SIMENGINECUR)
	{
		case SIMENGINECURALS:     simname = "ALS";       break;
		case SIMENGINECURIRSIM:   simname = "IRSIM";     break;
		case SIMENGINECURVERILOG: simname = "Verilog";   break;
		case SIMENGINECURSPICE:   simname = "Spice";     break;
	}
	infstr = initinfstr();
	if ((w->state&WINDOWSIMMODE) == 0)
	{
		formatinfstr(infstr, _("Inactive %s Simulation Window"), simname);
	} else
	{
		formatinfstr(infstr, _("%s Simulation Window"), simname);
	}
	if (sim_window_title != 0) formatinfstr(infstr, x_(", %s"), sim_window_title);
	addstringtoinfstr(infstr, x_(", ? for help"));
	lx = w->uselx;
	screendrawtext(w, lx+5, hy-TOPBARSIZE+2, returninfstr(infstr), &sim_window_desc);
	screeninvertbox(w, lx, hx, hy-TOPBARSIZE, hy);

	sim_window_drawtimescale(w);
	sim_window_drawgraph(w);
	sim_window_drawcursors(w);
	if (sim_window_highframe >= 0)
		sim_window_showtracebar(w, sim_window_highframe, 1);
}

/****************************** SUPPORT FUNCTIONS ******************************/

/*
 * Routine to convert from simulation time to the proper X position in the waveform window.
 */
INTBIG sim_window_timetoxpos(double time)
{
	double result;
	REGISTER TRACE *tr;

	tr = sim_window_firstrace;
	if (tr == NOTRACE) return(0);
	result = (WAVEXSIGEND-sim_window_wavexsigstart) * (time - tr->mintime) /
		(tr->maxtime - tr->mintime) + sim_window_wavexsigstart;
	return(rounddouble(result));
}

/*
 * Routine to convert from the X position in the waveform window to simulation time.
 */
double sim_window_xpostotime(INTBIG xpos)
{
	double result;
	REGISTER TRACE *tr;

	tr = sim_window_firstrace;
	if (tr == NOTRACE) return(xpos);
	result = (xpos-sim_window_wavexsigstart) * (tr->maxtime-tr->mintime) /
		(WAVEXSIGEND-sim_window_wavexsigstart) + tr->mintime;
	return(result);
}

/*
 * Routine to convert from framno and simulation value to the proper Y position in the waveform window.
 */
INTBIG sim_window_valtoypos(INTBIG frameno, double val)
{
	float topval, botval;
	INTBIG y_min, y, trace_spacing;

	topval = sim_window_get_topval(frameno);
	botval = sim_window_get_botval(frameno);
	if (val < botval || val > topval || topval == botval) return(-1);
	if (sim_window_visframes <= 0) return(-1);
	trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	y_min = (sim_window_visframes - 1 - (frameno - sim_window_topframes)) *
		trace_spacing + WAVEYSIGBOT;
	y = (INTBIG) ((val - botval) / (topval - botval) * (trace_spacing - WAVEYSIGGAP));
	return y_min + y;
}

/*
 * Routine to convert from the Y position in the waveform window to simulation value and frameno.
 */
INTBIG sim_window_ypostoval(INTBIG ypos, double *val)
{
	INTBIG frameno, trace_spacing;
	float topval, botval;

	if (sim_window_visframes <= 0) return(-1);
	trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	frameno = (INTBIG)((ypos - WAVEYSIGBOT)/(double)trace_spacing);
	ypos -= WAVEYSIGBOT + frameno*trace_spacing;
	if (ypos < 0 || ypos > trace_spacing - WAVEYSIGGAP) return(-1);
	frameno = sim_window_visframes - 1 - (frameno - sim_window_topframes);
	topval = sim_window_get_topval(frameno);
	botval = sim_window_get_botval(frameno);
	if (topval == botval) return(-1);
	*val = botval + (topval - botval)*ypos/(trace_spacing - WAVEYSIGGAP);
	return(frameno);
}

/*
 * This procedure prints out the time scale on the bottom of the waveform window.
 */
void sim_window_drawtimescale(WINDOWPART *wavewin)
{
	INTBIG x_pos;
	float time;
	REGISTER TRACE *tr;
	INTBIG i1, i2;

	tr = sim_window_firstrace;
	if (tr == NOTRACE) return;

	sim_window_setmask(LAYERA);
	sim_window_setcolor(MENTXT);
	sim_window_max_timetick = sim_window_maxtime = (float)tr->maxtime;
	sim_window_min_timetick = sim_window_mintime = (float)tr->mintime;
	sim_window_time_separation = sim_window_sensible_value(&sim_window_max_timetick, &sim_window_min_timetick, &i1, &i2, 5);
	if (sim_window_time_separation == 0.0) return;

	time = sim_window_min_timetick;
	for(;;)
	{
		if (time > sim_window_maxtime) break;
		x_pos = sim_window_timetoxpos(time);
		sim_window_moveto(x_pos, 0);
		sim_window_drawcstring(wavewin, sim_windowconvertengineeringnotation2(time, i2));
		time += sim_window_time_separation;
	}
}

void sim_window_draw_timeticks(WINDOWPART *wavewin, INTBIG y1, INTBIG y2, BOOLEAN grid)
{
	INTBIG x_pos;
	float time;

	time = sim_window_max_timetick;

	for(;;)
	{
		if (time < sim_window_mintime) break;
		x_pos = sim_window_timetoxpos(time);

		if (grid == TRUE) {
			sim_window_moveto(x_pos, y1);
			sim_window_drawto(wavewin, x_pos, y2, 1);
		}

		sim_window_moveto(x_pos, y1);
		sim_window_drawto(wavewin, x_pos, y1+10, 0);
		sim_window_moveto(x_pos, y2);
		sim_window_drawto(wavewin, x_pos, y2-10, 0);
		time -= sim_window_time_separation;
	}
}

/*
 * Routine to truncate the time value "time" to the next lower value that is "integral"
 */
double sim_window_sensibletimevalue(double time)
{
	double scale;

	if (time != 0.0)
	{
		scale = 1.0;
		while (doubleslessthan(time, 1.0))
		{
			time *= 10.0;
			scale /= 10.0;
		}
		time = doublefloor(time) * scale;
	}
	return(time);
}

/*
 * Routine to show the vertical bar between signal names and waveform data.
 * If "style" is 0, draw it solid
 * If "style" is 1, highlight it
 * If "style" is 2, unhighlight it
 */
void sim_window_showtracebar(WINDOWPART *wavewin, INTBIG frame, INTBIG style)
{
	INTBIG y_pos, temp_trace_spacing, visframe, ly, hy, xpos;

	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	visframe = frame - sim_window_topframes;
	if (visframe < 0 || visframe >= sim_window_visframes) return;
	y_pos = (sim_window_visframes - 1 - visframe) * temp_trace_spacing + WAVEYSIGBOT;

	/* draw the vertical line between the label and the trace */
	switch (style)
	{
		case 0:
			sim_window_setcolor(MENGLY);
			sim_window_moveto(sim_window_wavexbar, y_pos);
			sim_window_drawto(wavewin, sim_window_wavexbar, y_pos+temp_trace_spacing-WAVEYSIGGAP, 0);
			break;
		case 1:
			sim_window_setcolor(HIGHLIT);
			sim_window_setmask(LAYERH);
			ly = y_pos;   xpos = 0;
			sim_window_mapcoord(wavewin, &xpos, &ly);
			hy = y_pos+temp_trace_spacing-WAVEYSIGGAP;
			xpos = sim_window_wavexbar;
			sim_window_mapcoord(wavewin, &xpos, &hy);
			screendrawbox(wavewin, xpos-3, xpos+1, ly, hy, &sim_window_desc);
			sim_window_setmask(LAYERA);
			break;
		case 2:
			sim_window_setcolor(ALLOFF);
			sim_window_setmask(LAYERH);
			ly = y_pos;   xpos = 0;
			sim_window_mapcoord(wavewin, &xpos, &ly);
			hy = y_pos+temp_trace_spacing-WAVEYSIGGAP;
			xpos = sim_window_wavexbar;
			sim_window_mapcoord(wavewin, &xpos, &hy);
			screendrawbox(wavewin, xpos-3, xpos+1, ly, hy, &sim_window_desc);
			sim_window_setmask(LAYERA);
			break;
	}
}

void sim_window_writetracename(WINDOWPART *wavewin, TRACE *tr)
{
	INTBIG y_pos, temp_trace_spacing, frameno, px, py, textwid,
		maxheight, maxwidth, len, save, size;
	REGISTER CHAR *pt;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	INTBIG wid, hei;

	/* compute positions */
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	frameno = tr->frameno - sim_window_topframes;
	if (frameno < 0 || frameno >= sim_window_visframes) return;
	y_pos = (sim_window_visframes - 1 - frameno) * temp_trace_spacing + WAVEYSIGBOT;

	/* draw the vertical line between the label and the trace */
	sim_window_showtracebar(wavewin, tr->frameno, 0);

	/* set the text color */
	if ((tr->flags&TRACETYPE) == TRACEISANALOG) sim_window_setcolor(tr->color); else
		sim_window_setcolor(MENTXT);

	/* write the text */
	if (tr->buschannel == NOTRACE) px = 1; else px = 10;
	if ((tr->flags&TRACETYPE) == TRACEISANALOG) textwid = sim_window_wavexnameendana; else
		textwid = sim_window_wavexnameend;
	py = y_pos + tr->nameoff;
	maxheight = applyyscale(wavewin, temp_trace_spacing);
	maxwidth = applyxscale(wavewin, textwid-px);
	sim_window_mapcoord(wavewin, &px, &py);
	pt = tr->name;
	if (sim_window_traceprefix != 0)
	{
		len = estrlen(sim_window_traceprefix);
		if (namesamen(pt, sim_window_traceprefix, len) == 0)
			pt += len;
	}
	len = estrlen(pt);
	TDCLEAR(descript);
	for(size = sim_window_textsize; size >= 4; size--)
	{
		TDSETSIZE(descript, TXTSETPOINTS(size));
		screensettextinfo(wavewin, NOTECHNOLOGY, descript);
		screengettextsize(wavewin, pt, &wid, &hei);
		if (hei > maxheight && size > 4) continue;
		sim_window_txthei = hei * (wavewin->screenhy - wavewin->screenly) /
			(wavewin->usehy - wavewin->usely - 20);
		while (len > 1 && wid > maxwidth)
		{
			len--;
			save = pt[len];   pt[len] = 0;
			screengettextsize(wavewin, pt, &wid, &hei);
			pt[len] = (CHAR)save;
		}
		save = pt[len];   pt[len] = 0;
		screendrawtext(wavewin, px, py, pt, &sim_window_desc);
		pt[len] = (CHAR)save;
		break;
	}
}

/*
 * This procedure draws the cursors on the timing diagram.  The other color
 * planes are write protected so that the cursor doesn't alter any of the
 * timing diagram information.
 */
void sim_window_drawcursors(WINDOWPART *wavewin)
{
	INTBIG x_pos;
	INTBIG lx, ly, hx, hy, swap, textwid, extx, deltax, level, strength;
	CHAR s2[60];
	double mainv, extv, deltav;
	REGISTER TRACE *tr;

	/* erase all cursors */
	sim_window_setcolor(ALLOFF);
	sim_window_setmask(LAYERH);
	sim_window_drawbox(wavewin, sim_window_wavexsigstart, 0, WAVEXSIGEND, WAVEYCURSTOP, 0);
	sim_window_setcolor(HIGHLIT);

	/* draw main cursor */
	x_pos = sim_window_timetoxpos(sim_window_maincursor);
	if (x_pos >= sim_window_wavexsigstart && x_pos <= WAVEXSIGEND)
	{
		sim_window_moveto(x_pos, WAVEYSIGBOT);
		sim_window_drawto(wavewin, x_pos, WAVEYCURSTOP, 0);
	}

	/* draw extension cursor */
	x_pos = sim_window_timetoxpos(sim_window_extensioncursor);
	if (x_pos >= sim_window_wavexsigstart && x_pos <= WAVEXSIGEND)
	{
		sim_window_moveto(x_pos, WAVEYSIGBOT);
		sim_window_drawto(wavewin, x_pos, WAVEYCURSTOP, 0);
		sim_window_moveto(x_pos-5, WAVEYCURSTOP-12);
		sim_window_drawto(wavewin, x_pos+5, WAVEYCURSTOP-2, 0);
		sim_window_moveto(x_pos+5, WAVEYCURSTOP-12);
		sim_window_drawto(wavewin, x_pos-5, WAVEYCURSTOP-2, 0);
	}

	/* draw cursor locations textually */
	sim_window_setmask(LAYERA & ~LAYERH);
	sim_window_setcolor(ALLOFF);
	lx = sim_window_wavexsigstart;   ly = WAVEYTEXTBOT;
	hx = WAVEXSIGEND;                hy = WAVEYTEXTTOP;
	sim_window_mapcoord(wavewin, &lx, &ly);
	sim_window_mapcoord(wavewin, &hx, &hy);
	if (lx > hx) { swap = lx;   lx = hx;   hx = swap; }
	if (ly > hy) { swap = ly;   ly = hy;   hy = swap; }
	screendrawbox(wavewin, lx, hx, ly+1, hy, &sim_window_desc);
	
	sim_window_setcolor(MENTXT);
	sim_window_moveto(sim_window_wavexsigstart, WAVEYTEXTTOP);
	estrcpy(s2, _("Main: "));
	estrcat(s2, sim_windowconvertengineeringnotation(sim_window_maincursor));
	sim_window_drawulstring(wavewin, s2);

	textwid = (WAVEXSIGEND - sim_window_wavexsigstart) / 3;
	extx = sim_window_wavexsigstart + textwid;
	deltax = WAVEXSIGEND - textwid;
	if (sim_window_highlightedtracecount == 1)
	{
		tr = sim_window_highlightedtraces[0];
		switch (tr->flags&TRACETYPE)
		{
			case TRACEISANALOG:
				mainv = sim_window_getanatracevalue((INTBIG)tr, sim_window_maincursor);
				sim_window_moveto(sim_window_wavexsigstart, 585);
				esnprintf(s2, 60, x_("Y=%g"), mainv);
				sim_window_drawulstring(wavewin, s2);

				extv = sim_window_getanatracevalue((INTBIG)tr, sim_window_extensioncursor);
				sim_window_moveto(extx, 585);
				esnprintf(s2, 60, x_("Y=%g"), extv);
				sim_window_drawulstring(wavewin, s2);

				deltav = fabs(mainv-extv);
				sim_window_moveto(deltax, 585);
				esnprintf(s2, 60, x_("dY=%g"), deltav);
				sim_window_drawulstring(wavewin, s2);
				break;
			case TRACEISDIGITAL:
				sim_window_getdigtracevalue((INTBIG)tr, sim_window_maincursor, &level, &strength);
				switch (level)
				{
					case LOGIC_LOW:  estrcpy(s2, x_("=LOW"));   break;
					case LOGIC_HIGH: estrcpy(s2, x_("=HIGH"));  break;
					case LOGIC_X:    estrcpy(s2, x_("=X"));     break;
					case LOGIC_Z:    estrcpy(s2, x_("=Z"));     break;
					default:         estrcpy(s2, x_("=?"));     break;
				}
				sim_window_moveto(sim_window_wavexsigstart, 585);
				sim_window_drawulstring(wavewin, s2);

				sim_window_getdigtracevalue((INTBIG)tr, sim_window_extensioncursor, &level, &strength);
				switch (level)
				{
					case LOGIC_LOW:  estrcpy(s2, x_("=LOW"));   break;
					case LOGIC_HIGH: estrcpy(s2, x_("=HIGH"));  break;
					case LOGIC_X:    estrcpy(s2, x_("=X"));     break;
					case LOGIC_Z:    estrcpy(s2, x_("=Z"));     break;
					default:         estrcpy(s2, x_("=?"));     break;
				}
				sim_window_moveto(extx, 585);
				sim_window_drawulstring(wavewin, s2);
				break;
			case TRACEISBUS:
			case TRACEISOPENBUS:
				sim_window_moveto(sim_window_wavexsigstart, 585);
				esnprintf(s2, 60, x_("=%s"),
					sim_window_getbustracevalue((INTBIG)tr, sim_window_maincursor));
				sim_window_drawulstring(wavewin, s2);

				sim_window_moveto(extx, 585);
				esnprintf(s2, 60, x_("=%s"),
					sim_window_getbustracevalue((INTBIG)tr, sim_window_extensioncursor));
				sim_window_drawulstring(wavewin, s2);
				break;
		}
	}

	sim_window_moveto(extx, WAVEYTEXTTOP);
	estrcpy(s2, _("Ext: "));
	estrcat(s2, sim_windowconvertengineeringnotation(sim_window_extensioncursor));
	sim_window_drawulstring(wavewin, s2);

	sim_window_moveto(deltax, WAVEYTEXTTOP);
	estrcpy(s2, _("Delta: "));
	deltav = sim_window_extensioncursor - sim_window_maincursor;
	if (deltav < 0.0) deltav = -deltav;
	estrcat(s2, sim_windowconvertengineeringnotation(deltav));
	sim_window_drawulstring(wavewin, s2);
}

void sim_window_drawgraph(WINDOWPART *wavewin)
{
	REGISTER INTBIG i, lx, ly;
	INTBIG x, y, wid, hei;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	CHAR *msg;

	/* determine height of text in the window */
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(sim_window_textsize));
	screensettextinfo(wavewin, NOTECHNOLOGY, descript);
	screengettextsize(wavewin, x_("Xy"), &x, &y);
	sim_window_txthei = y * (wavewin->screenhy - wavewin->screenly) /
		(wavewin->usehy - wavewin->usely - 20);

	/* draw vertical divider between signal names and signal waveforms */
	sim_window_setcolor(MENTXT);
	sim_window_moveto(sim_window_wavexbar, WAVEYTEXTBOT);
	sim_window_drawto(wavewin, sim_window_wavexbar, WAVEYTEXTTOP, 0);

	/* plot it all */
	if (sim_window_firstrace == NOTRACE)
	{
		sim_window_setcolor(MENTXT);
		TDCLEAR(descript);
		TDSETSIZE(descript, TXTSETPOINTS(20));
		screensettextinfo(wavewin, NOTECHNOLOGY, descript);
		msg = _("Nothing plotted: type 'a' to add a signal");
		screengettextsize(wavewin, msg, &wid, &hei);
		lx = (wavewin->usehx + wavewin->uselx) / 2 - wid / 2;
		ly = (wavewin->usehy + wavewin->usely) / 2 - hei / 2;
		screendrawtext(wavewin, lx, ly, msg, &sim_window_desc);
	} else
	{
		/* draw the frames */
		for(i=0; i<sim_window_visframes; i++)
			sim_window_drawframe(i+sim_window_topframes, wavewin);
		for(i=0; i<sim_window_highlightedtracecount; i++)
			sim_window_highlightname(wavewin, sim_window_highlightedtraces[i], TRUE);
	}
}

/*
 * Routine to draw frame "frame" in waveform window "wavewin".
 */
void sim_window_drawframe(INTBIG frame, WINDOWPART *wavewin)
{
	INTBIG y_min, y_max, y_pos, maxoff, slot, i, temp_trace_spacing, i1, i2;
	BOOLEAN hasana, last, drawtext, grid=TRUE;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	INTBIG x, y, px, py;
	float separation, topval, botval, val;
	CHAR *s2;
	REGISTER TRACE *tr, *onetr;
	int vgrid;

	/* see how many traces are in this frame, determine whether it is analog */
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	hasana = FALSE;
	maxoff = 0;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if (tr->frameno != frame) continue;
		tr->nameoff = -1;
		maxoff++;
		if ((tr->flags&TRACETYPE) == TRACEISANALOG) hasana = TRUE;
		onetr = tr;
	}
	if (maxoff == 1)
	{
		/* only one trace: center it */
		onetr->nameoff = (temp_trace_spacing - WAVEYSIGGAP - sim_window_txthei) / 2;
		if (onetr->nameoff < 0) onetr->nameoff = 0;
	} else
	{
		/* multiple traces: compute them */
		slot = 0;
		for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		{
			if (tr->frameno != frame) continue;
			tr->nameoff = muldiv(slot, temp_trace_spacing - WAVEYSIGGAP - sim_window_txthei, maxoff-1);
			slot++;
		}
	}

	if (wavewin->state & GRIDON) grid = TRUE;
		else grid = FALSE;

	/* clear the waveform area */
	y_min = (sim_window_visframes - 1 - (frame - sim_window_topframes)) *
		temp_trace_spacing + WAVEYSIGBOT;
	y_max = y_min + temp_trace_spacing - WAVEYSIGGAP;

	/* if there are analog traces on this line, show height values */
	if (hasana)
	{
		sim_window_setmask(LAYERA);
		sim_window_setcolor(MENTXT);
		sim_window_drawbox(wavewin, sim_window_wavexsigstart, y_min, WAVEXSIGEND, y_max, 0);
		topval = sim_window_get_anahigh(frame);
		botval = sim_window_get_analow(frame);
		vgrid = (y_max-y_min) / (2 * sim_window_txthei);
		if (vgrid < 1) vgrid = 1;
		if (vgrid > 4) vgrid = 4 + (vgrid-4)/3;
		separation = sim_window_sensible_value(&topval, &botval, &i1, &i2, vgrid);
		sim_window_set_topval(frame, topval);
		sim_window_set_botval(frame, botval);
		if (sim_window_vdd > 0.0)
		{
			/* draw voltage thresholds */
			sim_window_setcolor(ALLOFF);
			for(i=0; i < PERCENTSIZE; i++)
			{
				y = sim_window_valtoypos(frame, sim_window_percent[i] * sim_window_vdd);
				if (y < 0) continue;
				sim_window_moveto(sim_window_wavexsigstart, y);
				sim_window_drawto(wavewin, WAVEXSIGEND, y, 1);
			}
		}
		sim_window_setcolor(MENTXT);
		sim_window_draw_timeticks(wavewin, y_min, y_max, grid);
		val = topval;
		if (separation > 0.0)
		{
			for(i=0; ; i++)
			{
				if (val + 0.5f * separation < botval) break;

				y_pos = roundfloat((val-botval) / (topval-botval) * (float)(y_max-y_min)) + y_min;
				sim_window_moveto(sim_window_wavexnameendana, y_pos);
				sim_window_drawto(wavewin, sim_window_wavexbar, y_pos, 0);

				if (grid)
				{
					sim_window_moveto(sim_window_wavexsigstart, y_pos);
					sim_window_drawto(wavewin, WAVEXSIGEND, y_pos, 1);
				}

				if (val - 0.5f*separation < botval) last = TRUE; else
					last = FALSE;

				/* see if the text should be drawn */
				drawtext = FALSE;
				if (sim_window_txthei*2 < temp_trace_spacing)
				{
					if (sim_window_txthei*4 < temp_trace_spacing)
					{
						drawtext = TRUE;
					} else
					{
						if (i == 0 || last) drawtext = TRUE;
					}
				}
				if (drawtext)
				{
					s2 = sim_window_prettyprint(val, i1, i2);

					TDCLEAR(descript);
					TDSETSIZE(descript, TXTSETPOINTS(sim_window_textsize));
					screensettextinfo(wavewin, NOTECHNOLOGY, descript);
					screengettextsize(wavewin, s2, &x, &y);
					px = sim_window_wavexnameendana;   py = y_pos;
					sim_window_mapcoord(wavewin, &px, &py);
					if (!last)
					{
						if (i == 0) py -= y; else
							py -= y/2;
					}
					screendrawtext(wavewin, px-x, py, s2, &sim_window_desc);
				}
				val -= separation;
			}
		}
	} else
	{
		sim_window_setmask(LAYERA);
		sim_window_setcolor(0);
		sim_window_drawbox(wavewin, sim_window_wavexsigstart, y_min, WAVEXSIGEND, y_max, 2);
		sim_window_set_topval(frame, 0.0);
		sim_window_set_botval(frame, 0.0);
	}

	/* draw the highlighted traces in this frame */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if (tr->frameno != frame) continue;
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if ((tr->flags&TRACESELECTED) == 0) continue;
		(void)sim_window_plottrace(tr, wavewin, 0, 0, 0);
	}

	/* draw the unhighlighted traces in this frame */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if (tr->frameno != frame) continue;
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if ((tr->flags&TRACESELECTED) != 0) continue;
		(void)sim_window_plottrace(tr, wavewin, 0, 0, 0);
	}
}

/*
 * Routine to draw a trace "tr" in waveform window "wavewin".  "style" is:
 *  0   draw normal trace
 *  1   search for point (x,y) on trace, return TRUE if found
 *  2   draw trace in highlight
 */
BOOLEAN sim_window_plottrace(TRACE *tr, WINDOWPART *wavewin, INTBIG style, INTBIG x, INTBIG y)
{
	INTBIG x_min, x_max, y_min, y_max, y_ctr, j, state, laststate, first,
		slot, temp_trace_spacing, fx, fy, tx, ty, color, strength,
		cfx, cfy, ctx, cty, sigcount, flags;
	CHAR s2[50];
	REGISTER BOOLEAN hitpoint, firstinbus, thick;
	REGISTER TRACE *bustr;
	double time, position, nexttime;

	/* get coordinates */
	if (sim_window_visframes == 0) return(FALSE);
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	slot = tr->frameno - sim_window_topframes;
	if (slot < 0 || slot >= sim_window_visframes) return(FALSE);
	y_min = (sim_window_visframes - 1 - slot) * temp_trace_spacing + WAVEYSIGBOT;
	y_max = y_min + temp_trace_spacing - WAVEYSIGGAP;

	if ((tr->flags&TRACESELECTED) != 0) thick = TRUE; else
		thick = FALSE;

	/* handle name field on the left */
	if (style == 0)
	{
		/* draw normal trace name */
		sim_window_writetracename(wavewin, tr);
	}
	hitpoint = FALSE;

	/* handle busses */
	flags = tr->flags & TRACETYPE;
	if (flags == TRACEISBUS || flags == TRACEISOPENBUS)
	{
		/* enumerate signals on the bus and make sure the global array has room */
		sigcount = sim_window_ensurespace(tr);

		/* find starting time */
		firstinbus = TRUE;
		for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
		{
			if (bustr->buschannel != tr) continue;

			/* LINTED "time" used in proper order */
			if (firstinbus || bustr->timearray[0] < time)
				time = bustr->timearray[0];
			firstinbus = FALSE;
		}

		/* draw the waveform */
		x_min = sim_window_wavexsigstart;
		for(;;)
		{
			/* compute signal value at time "time" */
			for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
			{
				if (bustr->buschannel != tr) continue;
				for(j=0; j<bustr->numsteps; j++)
					if (time < bustr->timearray[j]) break;
				if (j > 0) j--;
				sim_window_sigvalues[bustr->busindex] = bustr->statearray[j] >> 8;
			}

			/* compute the value string */
			estrcpy(s2, sim_window_makewidevalue(sigcount, sim_window_sigvalues));

			/* draw the value */
			x_max = sim_window_timetoxpos(time);
			if (x_max < sim_window_wavexsigstart) x_max = sim_window_wavexsigstart;
			if (x_max > WAVEXSIGEND) break;
			if (x_max > x_min)
			{
				if (style == 0)
				{
					sim_window_setcolor(sim_colorstrengthpower);
					sim_window_moveto(x_max+BUSANGLE/2, (y_min+y_max)/2);
					sim_window_drawhcstring(wavewin, s2);
				}
				hitpoint |= sim_window_lineseg(wavewin, x_min, (y_min+y_max)/2, x_min+BUSANGLE, y_min,
					style, x, y, thick);
				hitpoint |= sim_window_lineseg(wavewin, x_min+BUSANGLE, y_min, x_max-BUSANGLE, y_min,
					style, x, y, thick);
				hitpoint |= sim_window_lineseg(wavewin, x_max-BUSANGLE, y_min, x_max, (y_min+y_max)/2,
					style, x, y, thick);
				hitpoint |= sim_window_lineseg(wavewin, x_max, (y_min+y_max)/2, x_max-BUSANGLE, y_max,
					style, x, y, thick);
				hitpoint |= sim_window_lineseg(wavewin, x_max-BUSANGLE, y_max, x_min+BUSANGLE, y_max,
					style, x, y, thick);
				hitpoint |= sim_window_lineseg(wavewin, x_min+BUSANGLE, y_max, x_min, (y_min+y_max)/2,
					style, x, y, thick);
				x_min = x_max;
			}

			/* advance to the next time */
			nexttime = time;
			for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
			{
				if (bustr->buschannel != tr) continue;
				for(j=0; j<bustr->numsteps; j++)
					if (bustr->timearray[j] > time) break;
				if (j < bustr->numsteps)
				{
					if (nexttime == time || bustr->timearray[j] < nexttime)
						nexttime = bustr->timearray[j];
				}
			}
			if (nexttime == time) break;
			time = nexttime;
		}
		if (x_min < WAVEXSIGEND)
		{
			x_max = WAVEXSIGEND;
			sim_window_setcolor(sim_colorstrengthpower);
			hitpoint |= sim_window_lineseg(wavewin, x_min, (y_min+y_max)/2, x_min+BUSANGLE, y_min,
				style, x, y, thick);
			hitpoint |= sim_window_lineseg(wavewin, x_min+BUSANGLE, y_min, x_max-BUSANGLE, y_min,
				style, x, y, thick);
			hitpoint |= sim_window_lineseg(wavewin, x_max-BUSANGLE, y_min, x_max, (y_min+y_max)/2,
				style, x, y, thick);
			hitpoint |= sim_window_lineseg(wavewin, x_max, (y_min+y_max)/2, x_max-BUSANGLE, y_max,
				style, x, y, thick);
			hitpoint |= sim_window_lineseg(wavewin, x_max-BUSANGLE, y_max, x_min+BUSANGLE, y_max,
				style, x, y, thick);
			hitpoint |= sim_window_lineseg(wavewin, x_min+BUSANGLE, y_max, x_min, (y_min+y_max)/2,
				style, x, y, thick);
		}
		return(hitpoint);
	}

	/* handle digital waveform */
	if (flags == TRACEISDIGITAL)
	{
		first = 1;
		for(j=0; j<tr->numsteps; j++)
		{
			time = tr->timearray[j];
			if (j == tr->numsteps-1) x_max = WAVEXSIGEND; else
			{
				x_max = sim_window_timetoxpos(tr->timearray[j+1]);
				if (x_max < sim_window_wavexsigstart) continue;
				if (x_max > WAVEXSIGEND) x_max = WAVEXSIGEND;
			}
			x_min = sim_window_timetoxpos(time);
			if (x_min < sim_window_wavexsigstart) x_min = sim_window_wavexsigstart;
			if (x_min > WAVEXSIGEND) continue;

			state = tr->statearray[j] >> 8;
			strength = tr->statearray[j] & 0377;
			if (strength == 0) color = sim_colorstrengthoff; else
				if (strength <= NODE_STRENGTH) color = sim_colorstrengthnode; else
					if (strength <= GATE_STRENGTH) color = sim_colorstrengthgate; else
						color = sim_colorstrengthpower;
			sim_window_setcolor(color);

			/* LINTED "laststate" used in proper order */
			if (first == 0 && state != laststate)
			{
				hitpoint |= sim_window_lineseg(wavewin, x_min, y_min, x_min, y_max,
					style, x, y, thick);
			}
			first = 0;
			switch (state)
			{
				case LOGIC_LOW:
					hitpoint |= sim_window_lineseg(wavewin, x_min, y_min, x_max, y_min,
						style, x, y, thick);
					break;
				case LOGIC_X:
					if (style == 1)
					{
						if (x >= x_min && x <= x_max && y >= y_min && y <= y_max)
							hitpoint = TRUE;
					} else
					{
						sim_window_drawbox(wavewin, x_min, y_min, x_max, y_max, 1);
					}
					break;
				case LOGIC_HIGH:
					hitpoint |= sim_window_lineseg(wavewin, x_min, y_max, x_max, y_max,
						style, x, y, thick);
					break;
				case LOGIC_Z:
					y_ctr = (y_min+y_max)/2;
					hitpoint |= sim_window_lineseg(wavewin, x_min, y_ctr, x_max, y_ctr,
						style, x, y, thick);
					break;
				default:
					if (style == 1)
					{
						if (x >= x_min && x <= x_max && y >= y_min && y <= y_max)
							hitpoint = TRUE;
					} else
					{
						sim_window_drawbox(wavewin, x_min, y_min, x_max, y_max, 0);
						if ((x_max - x_min) <= 1) break;
						sim_window_setcolor(ALLOFF);
						sim_window_drawbox(wavewin, x_min+1, y_min+1, x_max-1, y_max-1, 0);
						(void)esnprintf(s2, 50, x_("0x%lX"), state);
						sim_window_moveto((x_min + x_max) / 2, y_min+3);
						sim_window_setcolor(color);
						sim_window_drawcstring(wavewin, s2);
					}
					break;
			}
			laststate = state;
		}
		return(hitpoint);
	}

	/* handle analog waveform */
	first = 1;
	for(j=0; j<tr->numsteps; j++)
	{
		time = tr->timearray[j];
		tx = sim_window_timetoxpos(time);
		position = (double)(y_max - y_min);
		position *= (tr->valuearray[j] - sim_window_get_botval(tr->frameno)) / sim_window_get_extanarange(tr->frameno);
		ty = (INTBIG)position + y_min;
		if (j != 0 && tx >= sim_window_wavexsigstart)
		{
			if (tx > WAVEXSIGEND)
			{
				/* "to" data point off the screen: interpolate */
				/* LINTED "fx" and "fy" used in proper order */
				ty = (ty-fy) * (WAVEXSIGEND-fx) / (tx-fx) + fy;
				tx = WAVEXSIGEND;
			}

			if (first != 0 && fx < sim_window_wavexsigstart)
			{
				/* "from" data point off the screen: interpolate */
				fy = (ty-fy) * (sim_window_wavexsigstart-fx) / (tx-fx) + fy;
				fx = sim_window_wavexsigstart;
			}
			first = 0;
			if (style != 2) sim_window_setcolor(tr->color);
			if (fy >= y_min && fy <= y_max &&
				ty >= y_min && ty <= y_max)
			{
				hitpoint |= sim_window_lineseg(wavewin, fx, fy, tx, ty, style, x, y, thick);
			} else
			{
				cfx = fx;   cfy = fy;
				ctx = tx;   cty = ty;
				if (!clipline(&cfx, &cfy, &ctx, &cty, sim_window_wavexsigstart, WAVEXSIGEND,
					y_min, y_max))
				{
					hitpoint |= sim_window_lineseg(wavewin, cfx, cfy, ctx, cty, style, x, y, thick);
				}
			}
		}
		fx = tx;   fy = ty;
		if (fx >= WAVEXSIGEND) break;
	}
	return(hitpoint);
}

INTBIG sim_window_ensurespace(TRACE *tr)
{
	REGISTER INTBIG sigcount, newtotal;
	REGISTER TRACE *bustr;

	sigcount = 0;
	for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
	{
		if (bustr->buschannel == tr)
			bustr->busindex = sigcount++;
	}

	/* make sure there is room for all of the signal values */
	if (sigcount > sim_window_sigtotal)
	{
		newtotal = sim_window_sigtotal * 2;
		if (newtotal == 0) newtotal = 8;
		if (newtotal < sigcount) newtotal = sigcount;
		if (sim_window_sigtotal > 0) efree((CHAR *)sim_window_sigvalues);
		sim_window_sigtotal = 0;
		sim_window_sigvalues = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, sim_tool->cluster);
		if (sim_window_sigvalues == 0) return(0);
		sim_window_sigtotal = newtotal;
	}
	return(sigcount);
}

/*
 * Routine to draw from (xf,yf) to (xt,yt) in window "wavewin".  If "style" is zero, draw it.
 * If "style" is 1, do not draw, but return TRUE if (x,y) is on the line.
 */
BOOLEAN sim_window_lineseg(WINDOWPART *wavewin, INTBIG xf, INTBIG yf, INTBIG xt, INTBIG yt,
	INTBIG style, INTBIG x, INTBIG y, BOOLEAN thick)
{
	REGISTER INTBIG dist, drawstyle;

	if (style != 1)
	{
		sim_window_moveto(xf, yf);
		if (thick) drawstyle = 3; else
			drawstyle = 0;
		sim_window_drawto(wavewin, xt, yt, drawstyle);
		return(FALSE);
	}
	dist = disttoline(xf, yf, xt, yt, x, y);
	if (dist < 5) return(TRUE);
	return(FALSE);
}

/*
 * Routine to find cross of a trace "tr" with horizontal line "val" nearest to "time".
 * Return TRUE if crass was found
 */
BOOLEAN sim_window_findcross(TRACE *tr, double val, double *time)
{
	INTBIG j;
	double btime, ctime;
	BOOLEAN bfound;

	/* handle analog waveform */
	bfound = FALSE;
	btime = 0;
	for(j=1; j<tr->numsteps; j++)
	{
		if ((tr->valuearray[j-1] > val && tr->valuearray[j] > val) ||
			(tr->valuearray[j-1] < val && tr->valuearray[j] < val)) continue;
		if (tr->valuearray[j-1] == val && tr->valuearray[j] == val)
		{
			if (*time < tr->timearray[j-1]) ctime = tr->timearray[j-1];
			else if (tr->timearray[j] < *time) ctime = tr->timearray[j];
			else ctime = *time;
		} else
		{
			ctime = (val - tr->valuearray[j-1]) / (tr->valuearray[j] - tr->valuearray[j-1]) *
				(tr->timearray[j] - tr->timearray[j-1]) + tr->timearray[j-1];
		}
		if (!bfound || fabs(ctime - *time) < fabs(btime - *time))
		{
			bfound = TRUE;
			btime = ctime;
		}
	}
	if (bfound)
		*time = btime;
	return(bfound);
}

/*
 * routine to snapshot the simulation window in a cell with artwork primitives
 */
void sim_window_savegraph(void)
{
	REGISTER INTBIG x_min, x_max, y_min, y_max, j, maxoff, state, laststate, color, first,
		slot, temp_trace_spacing, range, lastcolor, fx, fy, tx, ty, strength,
		sigcount, cx, cy, *data, maxsteps, len;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER BOOLEAN hasana, firstinbus;
	INTBIG x, y, busdescr[12], tx1, ty1, tx2, ty2, datapos, cfx, cfy, ctx, cty;
	CHAR s2[15], *pt, *newname;
	REGISTER NODEPROTO *np, *plotnp;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER WINDOWPART *wavewin;
	REGISTER TRACE *tr, *otr, *bustr;
	double time, position, separation, nexttime;
	REGISTER void *infstr;

	/* get cell being saved */
	np = getcurcell();
	if (np == NONODEPROTO) return;

	/* get waveform window */
	wavewin = sim_window_findwaveform();
	if (wavewin == NOWINDOWPART) return;

	/* determine height of text in the window */
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(sim_window_textsize));
	screensettextinfo(wavewin, NOTECHNOLOGY, descript);
	screengettextsize(wavewin, x_("Xy"), &x, &y);
	sim_window_txthei = y * (wavewin->screenhy - wavewin->screenly) /
		(wavewin->usehy - wavewin->usely - 20);

	/* create the plot cell */
	infstr = initinfstr();
	addstringtoinfstr(infstr, np->protoname);
	addstringtoinfstr(infstr, x_("{sim}"));
	plotnp = newnodeproto(returninfstr(infstr), el_curlib);
	if (plotnp == NONODEPROTO) return;
	setactivity(x_("Make Simulation Plot"));

	/* determine maximum number of steps */
	maxsteps = 1;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if (tr->numsteps > maxsteps) maxsteps = tr->numsteps;
	}

	/* make space */
	data = (INTBIG *)emalloc(maxsteps * 4 * SIZEOFINTBIG, el_tempcluster);
	if (data == 0) return;

	/* compute transformation for points in the plot area */
	tx1 = sim_window_wavexsigstart;   ty1 = WAVEYSIGBOT;    sim_window_mapcoord(wavewin, &tx1, &ty1);
	tx2 = WAVEXSIGEND;                ty2 = WAVEYCURSTOP;   sim_window_mapcoord(wavewin, &tx2, &ty2);
	sim_window_offx = (tx2 - tx1) / 2;   sim_window_basex = tx1;
	sim_window_offy = (ty2 - ty1) / 2;   sim_window_basey = ty1;

	/* write the header */
	tx1 = 383;   ty1 = WAVEYTEXTTOP;   sim_window_mapcoord(wavewin, &tx1, &ty1);
	ni = newnodeinst(gen_invispinprim, tx1, tx1, ty1, ty1, 0, 0, plotnp);
	if (ni == NONODEINST) { efree((CHAR *)data);  return; }
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Simulation snapshot of cell "));
	addstringtoinfstr(infstr, describenodeproto(np));
	allocstring(&newname, returninfstr(infstr), el_tempcluster);
	var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)newname, VSTRING|VDISPLAY);
	efree(newname);
	if (var != NOVARIABLE)
	{
		TDSETSIZE(var->textdescript, TXTSETPOINTS(14));
		TDSETPOS(var->textdescript, VTPOSUP);
	}
	endobjectchange((INTBIG)ni, VNODEINST);

	/* set name offsets */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace) tr->nameoff = -1;
	temp_trace_spacing = WAVEYSIGRANGE / sim_window_visframes;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		slot = tr->frameno - sim_window_topframes;
		if (slot < 0 || slot >= sim_window_visframes) continue;

		/* determine position of trace label on the line */
		if (tr->nameoff < 0)
		{
			hasana = FALSE;
			maxoff = 0;
			for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
			{
				if ((otr->flags&TRACENOTDRAWN) != 0) continue;
				if (otr->frameno != tr->frameno) continue;
				maxoff++;
				if ((otr->flags&TRACETYPE) == TRACEISANALOG) hasana = TRUE;
			}
			if (maxoff == 1)
			{
				/* only one trace: center it */
				tr->nameoff = (temp_trace_spacing - WAVEYSIGGAP - sim_window_txthei) / 2;
				if (tr->nameoff < 0) tr->nameoff = 0;
			} else
			{
				/* multiple traces: compute them */
				range = (temp_trace_spacing - WAVEYSIGGAP - sim_window_txthei*3) / (maxoff-1);
				if (range <= 0) range = 1;
				slot = 0;
				for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
				{
					if ((otr->flags&TRACENOTDRAWN) != 0) continue;
					if (otr->frameno != tr->frameno) continue;
					otr->nameoff = slot * range + sim_window_txthei;
					slot++;
				}
			}

			/* if there are analog traces on this line, show height values */
			if (hasana)
			{
				y_min = (sim_window_visframes - 1 - (tr->frameno - sim_window_topframes)) *
					temp_trace_spacing + WAVEYSIGBOT;
				y_max = y_min + temp_trace_spacing - WAVEYSIGGAP;

				tx1 = sim_window_wavexnameendana;   ty1 = y_min;   sim_window_mapcoord(wavewin, &tx1, &ty1);
				tx2 = sim_window_wavexbar;          ty2 = y_min;   sim_window_mapcoord(wavewin, &tx2, &ty2);
				data[0] = -(tx2-tx1)/2;  data[1] = 0;
				data[2] = (tx2-tx1)/2;   data[3] = 0;
				ni = newnodeinst(art_openedpolygonprim, tx1,tx2, ty1,ty2, 0, 0, plotnp);
				if (ni == NONODEINST) { efree((CHAR *)data);  return; }
				(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
					VINTEGER|VISARRAY|(4<<VLENGTHSH));
				endobjectchange((INTBIG)ni, VNODEINST);

				(void)esnprintf(s2, 15, x_("%g"), sim_window_get_analow(tr->frameno));
				tx1 = sim_window_wavexnameend;   ty1 = y_min;   sim_window_mapcoord(wavewin, &tx1, &ty1);
				ni = newnodeinst(gen_invispinprim, tx1, tx1, ty1, ty1, 0, 0, plotnp);
				if (ni == NONODEINST) { efree((CHAR *)data);  return; }
				var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)s2, VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
				{
					TDSETSIZE(var->textdescript, TXTSETPOINTS(10));
					TDSETPOS(var->textdescript, VTPOSUPLEFT);
				}
				endobjectchange((INTBIG)ni, VNODEINST);

				tx1 = sim_window_wavexnameendana;   ty1 = y_max;   sim_window_mapcoord(wavewin, &tx1, &ty1);
				tx2 = sim_window_wavexbar;          ty2 = y_max;   sim_window_mapcoord(wavewin, &tx2, &ty2);
				data[0] = -(tx2-tx1)/2;  data[1] = 0;
				data[2] = (tx2-tx1)/2;   data[3] = 0;
				ni = newnodeinst(art_openedpolygonprim, tx1,tx2, ty1,ty2, 0, 0, plotnp);
				if (ni == NONODEINST) { efree((CHAR *)data);  return; }
				(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
					VINTEGER|VISARRAY|(4<<VLENGTHSH));
				endobjectchange((INTBIG)ni, VNODEINST);

				(void)esnprintf(s2, 15, x_("%g"), sim_window_get_anahigh(tr->frameno));
				tx1 = sim_window_wavexnameend;   ty1 = y_max;   sim_window_mapcoord(wavewin, &tx1, &ty1);
				ni = newnodeinst(gen_invispinprim, tx1, tx1, ty1, ty1, 0, 0, plotnp);
				if (ni == NONODEINST) { efree((CHAR *)data);  return; }
				var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)s2, VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
				{
					TDSETSIZE(var->textdescript, TXTSETPOINTS(10));
					TDSETPOS(var->textdescript, VTPOSDOWNLEFT);
				}
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
	}

	/* plot time values */
	tr = sim_window_firstrace;
	if (tr != NOTRACE)
	{
		separation = sim_window_sensibletimevalue((tr->maxtime - tr->mintime) / 5.0);
		time = sim_window_sensibletimevalue(tr->maxtime);
		while (time + separation <tr->maxtime)
			time += separation;
		for(;;)
		{
			if (time < tr->mintime) break;
			x = sim_window_timetoxpos(time);
			tx1 = x;   ty1 = 0;   sim_window_mapcoord(wavewin, &tx1, &ty1);
			ni = newnodeinst(gen_invispinprim, tx1, tx1, ty1, ty1, 0, 0, plotnp);
			if (ni == NONODEINST) { efree((CHAR *)data);  return; }
			var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey,
				(INTBIG)sim_windowconvertengineeringnotation(time), VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
			{
				TDSETSIZE(var->textdescript, TXTSETPOINTS(10));
				TDSETPOS(var->textdescript, VTPOSUP);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			time -= separation;
		}
	}

	/* plot it all */
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) continue;
		if (stopping(STOPREASONDISPLAY)) break;

		/* compute positions */
		slot = tr->frameno - sim_window_topframes;
		if (slot < 0 || slot >= sim_window_visframes) continue;
		y_min = (sim_window_visframes - 1 - slot) * temp_trace_spacing + WAVEYSIGBOT;
		y_max = y_min + temp_trace_spacing - WAVEYSIGGAP;

		/* draw the vertical line between the label and the trace */
		tx1 = sim_window_wavexbar;   ty1 = y_min;   sim_window_mapcoord(wavewin, &tx1, &ty1);
		tx2 = sim_window_wavexbar;   ty2 = y_min+temp_trace_spacing-WAVEYSIGGAP;   sim_window_mapcoord(wavewin, &tx2, &ty2);
		data[0] = 0;   data[1] = -(ty2-ty1)/2;
		data[2] = 0;   data[3] = (ty2-ty1)/2;
		ni = newnodeinst(art_openedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
		if (ni == NONODEINST) { efree((CHAR *)data);  return; }
		(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
			VINTEGER|VISARRAY|(4<<VLENGTHSH));
		endobjectchange((INTBIG)ni, VNODEINST);

		/* write the text */
		(void)esnprintf(s2, 15, x_("%g"), sim_window_get_anahigh(tr->frameno));
		tx1 = 1;   ty1 = (y_min + y_max)/2;   sim_window_mapcoord(wavewin, &tx1, &ty1);
		ni = newnodeinst(gen_invispinprim, tx1, tx1, ty1, ty1, 0, 0, plotnp);
		if (ni == NONODEINST) { efree((CHAR *)data);  return; }
		pt = tr->name;
		if (sim_window_traceprefix != 0)
		{
			len = estrlen(sim_window_traceprefix);
			if (namesamen(pt, sim_window_traceprefix, len) == 0)
				pt += len;
		}
		var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)pt, VSTRING|VDISPLAY);
		if (var != NOVARIABLE)
		{
			TDSETSIZE(var->textdescript, TXTSETPOINTS(10));
			TDSETPOS(var->textdescript, VTPOSRIGHT);
		}
		endobjectchange((INTBIG)ni, VNODEINST);
		if ((tr->flags&TRACETYPE) == TRACEISANALOG)
		{
			tx1 = 0;     ty1 = y_min + tr->nameoff;
			sim_window_mapcoord(wavewin, &tx1, &ty1);
			tx2 = sim_window_wavexnameendana;   ty2 = y_min + tr->nameoff;
			sim_window_mapcoord(wavewin, &tx2, &ty2);
			data[0] = -(tx2-tx1)/2;  data[1] = 0;
			data[2] = (tx2-tx1)/2;   data[3] = 0;
			ni = newnodeinst(art_openedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
			if (ni == NONODEINST) { efree((CHAR *)data);  return; }
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
				VINTEGER|VISARRAY|(4<<VLENGTHSH));
			(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, tr->color, VINTEGER);
			endobjectchange((INTBIG)ni, VNODEINST);
		}

		/* write busses */
		if ((tr->flags&TRACETYPE) == TRACEISBUS ||
			(tr->flags&TRACETYPE) == TRACEISOPENBUS)
		{
			/* enumerate signals on the bus and make sure the global array has room */
			sigcount = sim_window_ensurespace(tr);

			/* find starting time */
			firstinbus = TRUE;
			for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
			{
				if (bustr->buschannel != tr) continue;

				/* LINTED "time" used in proper order */
				if (firstinbus || bustr->timearray[0] < time)
					time = bustr->timearray[0];
				firstinbus = FALSE;
			}

			/* draw the waveform */
			x_min = sim_window_wavexsigstart;
			for(;;)
			{
				/* compute signal value at time "time" */
				for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
				{
					if (bustr->buschannel != tr) continue;
					for(j=0; j<bustr->numsteps; j++)
						if (time < bustr->timearray[j]) break;
					if (j > 0) j--;
					sim_window_sigvalues[bustr->busindex] = bustr->statearray[j] >> 8;
				}

				/* draw the value */
				x_max = sim_window_timetoxpos(time);
				if (x_max < sim_window_wavexsigstart) x_max = sim_window_wavexsigstart;
				if (x_max > WAVEXSIGEND) break;
				if (x_max > x_min)
				{
					tx1 = x_min;   ty1 = y_min;   sim_window_mapcoord(wavewin, &tx1, &ty1);
					tx2 = x_max;   ty2 = y_max;   sim_window_mapcoord(wavewin, &tx2, &ty2);
					cx = (tx1 + tx2) / 2;   cy = (ty1 + ty2) / 2;
					ni = newnodeinst(art_closedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
					if (ni == NONODEINST) { efree((CHAR *)data);  return; }
					busdescr[0] = x_min;           busdescr[1] = (y_min+y_max)/2;
					busdescr[2] = x_min+BUSANGLE;  busdescr[3] = y_min;
					busdescr[4] = x_max-BUSANGLE;  busdescr[5] = y_min;
					busdescr[6] = x_max;           busdescr[7] = (y_min+y_max)/2;
					busdescr[8] = x_max-BUSANGLE;  busdescr[9] = y_max;
					busdescr[10] = x_min+BUSANGLE; busdescr[11] = y_max;
					for(j=0; j<12; j += 2)
					{
						sim_window_mapcoord(wavewin, &busdescr[j], &busdescr[j+1]);
						busdescr[j] -= cx;
						busdescr[j+1] -= cy;
					}
					(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)busdescr,
						VINTEGER|VISARRAY|(12<<VLENGTHSH));
					endobjectchange((INTBIG)ni, VNODEINST);

					/* draw the value string */
					estrcpy(s2, sim_window_makewidevalue(sigcount, sim_window_sigvalues));
					tx1 = x_max+BUSANGLE/2;   ty1 = (y_min+y_max)/2;   sim_window_mapcoord(wavewin, &tx1, &ty1);
					ni = newnodeinst(gen_invispinprim, tx1, tx1, ty1, ty1, 0, 0, plotnp);
					if (ni == NONODEINST) { efree((CHAR *)data);  return; }
					var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)s2, VSTRING|VDISPLAY);
					if (var != NOVARIABLE)
					{
						TDSETSIZE(var->textdescript, TXTSETPOINTS(10));
						TDSETPOS(var->textdescript, VTPOSRIGHT);
					}
					endobjectchange((INTBIG)ni, VNODEINST);
					x_min = x_max;
				}

				/* advance to the next time */
				nexttime = time;
				for(bustr = sim_window_firstrace; bustr != NOTRACE; bustr = bustr->nexttrace)
				{
					if (bustr->buschannel != tr) continue;
					for(j=0; j<bustr->numsteps; j++)
						if (bustr->timearray[j] > time) break;
					if (j < bustr->numsteps)
					{
						if (nexttime == time || bustr->timearray[j] < nexttime)
							nexttime = bustr->timearray[j];
					}
				}
				if (nexttime == time) break;
				time = nexttime;
			}
			if (x_min < WAVEXSIGEND)
			{
				x_max = WAVEXSIGEND;
				tx1 = x_min;   ty1 = y_min;   sim_window_mapcoord(wavewin, &tx1, &ty1);
				tx2 = x_max;   ty2 = y_max;   sim_window_mapcoord(wavewin, &tx2, &ty2);
				cx = (tx1 + tx2) / 2;   cy = (ty1 + ty2) / 2;
				ni = newnodeinst(art_closedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
				if (ni == NONODEINST) { efree((CHAR *)data);  return; }
				busdescr[0] = x_min;           busdescr[1] = (y_min+y_max)/2;
				busdescr[2] = x_min+BUSANGLE;  busdescr[3] = y_min;
				busdescr[4] = x_max-BUSANGLE;  busdescr[5] = y_min;
				busdescr[6] = x_max;           busdescr[7] = (y_min+y_max)/2;
				busdescr[8] = x_max-BUSANGLE;  busdescr[9] = y_max;
				busdescr[10] = x_min+BUSANGLE; busdescr[11] = y_max;
				for(j=0; j<12; j += 2)
				{
					sim_window_mapcoord(wavewin, &busdescr[j], &busdescr[j+1]);
					busdescr[j] -= cx;
					busdescr[j+1] -= cy;
				}
				(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)busdescr,
					VINTEGER|VISARRAY|(12<<VLENGTHSH));
				endobjectchange((INTBIG)ni, VNODEINST);
			}
			continue;
		}

		first = 1;
		datapos = 0;
		lastcolor = ALLOFF;
		fx = fy = 0;
		laststate = -1;
		for(j=0; j<tr->numsteps; j++)
		{
			time = tr->timearray[j];
			if ((tr->flags&TRACETYPE) == TRACEISDIGITAL)
			{
				/* digital waveform */
				if (j == tr->numsteps-1) x_max = WAVEXSIGEND; else
				{
					x_max = sim_window_timetoxpos(tr->timearray[j+1]);
					if (x_max < sim_window_wavexsigstart) continue;
				}
				x_min = sim_window_timetoxpos(time);
				if (x_min < sim_window_wavexsigstart) x_min = sim_window_wavexsigstart;
				if (x_min > WAVEXSIGEND) continue;

				strength = tr->statearray[j] & 0377;
				if (strength == 0) color = sim_colorstrengthoff; else
					if (strength <= NODE_STRENGTH) color = sim_colorstrengthnode; else
						if (strength <= GATE_STRENGTH) color = sim_colorstrengthgate; else
							color = sim_colorstrengthpower;

				/* LINTED "lastcolor" used in proper order */
				if (datapos > 0 && color != lastcolor)
				{
					/* strength has changed, dump out data so far */
					tx1 = sim_window_wavexsigstart;   ty1 = WAVEYSIGBOT;    sim_window_mapcoord(wavewin, &tx1, &ty1);
					tx2 = WAVEXSIGEND;                ty2 = WAVEYCURSTOP;   sim_window_mapcoord(wavewin, &tx2, &ty2);
					ni = newnodeinst(art_openedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
					if (ni == NONODEINST) { efree((CHAR *)data);  return; }
					(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
						VINTEGER|VISARRAY|(datapos<<VLENGTHSH));
					(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, lastcolor, VINTEGER);
					endobjectchange((INTBIG)ni, VNODEINST);
					datapos = 0;
				}
				lastcolor = color;

				state = tr->statearray[j] >> 8;
				switch (state)
				{
					case LOGIC_LOW:
						if (laststate == LOGIC_HIGH)
							sim_window_addsegment(wavewin, data, &datapos, x_min, y_max, x_min, y_min);
						sim_window_addsegment(wavewin, data, &datapos, x_min, y_min, x_max, y_min);
						break;
					case LOGIC_HIGH:
						if (laststate == LOGIC_LOW)
							sim_window_addsegment(wavewin, data, &datapos, x_min, y_min, x_min, y_max);
						sim_window_addsegment(wavewin, data, &datapos, x_min, y_max, x_max, y_max);
						break;
					default:
						if (datapos > 0)
						{
							/* doing unknown block, dump out any line data */
							tx1 = sim_window_wavexsigstart;   ty1 = WAVEYSIGBOT;    sim_window_mapcoord(wavewin, &tx1, &ty1);
							tx2 = WAVEXSIGEND;                ty2 = WAVEYCURSTOP;   sim_window_mapcoord(wavewin, &tx2, &ty2);
							ni = newnodeinst(art_openedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
							if (ni == NONODEINST) { efree((CHAR *)data);  return; }
							(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
								VINTEGER|VISARRAY|(datapos<<VLENGTHSH));
							(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, lastcolor,
								VINTEGER);
							endobjectchange((INTBIG)ni, VNODEINST);
						}
						datapos = 0;
						tx1 = x_min;   ty1 = y_min;   sim_window_mapcoord(wavewin, &tx1, &ty1);
						tx2 = x_max;   ty2 = y_max;   sim_window_mapcoord(wavewin, &tx2, &ty2);
						ni = newnodeinst(art_filledboxprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
						if (ni == NONODEINST) { efree((CHAR *)data);  return; }
						(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, lastcolor, VINTEGER);
						endobjectchange((INTBIG)ni, VNODEINST);
						break;
				}
				laststate = state;
				first = 0;
			} else
			{
				/* analog waveform */
				tx = sim_window_timetoxpos(time);
				position = (double)(y_max - y_min);
				position *= (tr->valuearray[j] - sim_window_get_analow(tr->frameno)) / sim_window_get_anarange(tr->frameno);
				ty = (INTBIG)position + y_min;
				if (j != 0 && tx >= sim_window_wavexsigstart)
				{
					if (tx > WAVEXSIGEND)
					{
						/* "to" data point off the screen: interpolate */
						/* LINTED "fx" and "fy" used in proper order */
						ty = (ty-fy) * (WAVEXSIGEND-fx) / (tx-fx) + fy;
						tx = WAVEXSIGEND;
					}

					if (first != 0 && fx < sim_window_wavexsigstart)
					{
						/* "from" data point off the screen: interpolate */
						fy = (ty-fy) * (sim_window_wavexsigstart-fx) / (tx-fx) + fy;
						fx = sim_window_wavexsigstart;
					}
					first = 0;
					if (fy >= y_min && fy <= y_max &&
						ty >= y_min && ty <= y_max)
					{
						sim_window_addsegment(wavewin, data, &datapos, fx, fy, tx, ty);
					} else
					{
						cfx = fx;   cfy = fy;
						ctx = tx;   cty = ty;
						if (!clipline(&cfx, &cfy, &ctx, &cty, sim_window_wavexsigstart, WAVEXSIGEND, y_min, y_max))
						{
							sim_window_addsegment(wavewin, data, &datapos, cfx, cfy, ctx, cty);
						}
					}
				}
				fx = tx;   fy = ty;
				if (fx >= WAVEXSIGEND) break;
			}
		}
		if (datapos > 0)
		{
			tx1 = sim_window_wavexsigstart;   ty1 = WAVEYSIGBOT;    sim_window_mapcoord(wavewin, &tx1, &ty1);
			tx2 = WAVEXSIGEND;                ty2 = WAVEYCURSTOP;   sim_window_mapcoord(wavewin, &tx2, &ty2);
			ni = newnodeinst(art_openedpolygonprim, tx1, tx2, ty1, ty2, 0, 0, plotnp);
			if (ni == NONODEINST) { efree((CHAR *)data);  return; }
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)data,
				VINTEGER|VISARRAY|(datapos<<VLENGTHSH));
			if ((tr->flags&TRACETYPE) == TRACEISANALOG)
				(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, tr->color, VINTEGER); else
					(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, lastcolor, VINTEGER);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
	}

	/* ensure that the cell has the right size */
	(*el_curconstraint->solve)(plotnp);

	/* clean up */
	ttyputmsg(x_("Created cell %s"), describenodeproto(plotnp));
	efree((CHAR *)data);
}

/*
 * Set the display color for strength "strength" to "color".
 */
void sim_window_setdisplaycolor(INTBIG strength, INTBIG color)
{
	if (strength == LOGIC_LOW)
	{
		if (color != sim_colorlevellow)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevellow_key, color, VINTEGER);
	} else if (strength == LOGIC_HIGH)
	{
		if (color != sim_colorlevelhigh)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevelhigh_key, color, VINTEGER);
	} else if (strength == LOGIC_X)
	{
		if (color != sim_colorlevelundef)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevelundef_key, color, VINTEGER);
	} else if (strength == LOGIC_Z)
	{
		if (color != sim_colorlevelzdef)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorlevelzdef_key, color, VINTEGER);
	} else if (strength == OFF_STRENGTH)
	{
		if (color != sim_colorstrengthoff)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthoff_key, color, VINTEGER);
	} else if (strength <= NODE_STRENGTH)
	{
		if (color != sim_colorstrengthnode)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthnode_key, color, VINTEGER);
	} else if (strength <= GATE_STRENGTH)
	{
		if (color != sim_colorstrengthgate)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthgate_key, color, VINTEGER);
	} else if (strength <= VDD_STRENGTH)
	{
		if (color != sim_colorstrengthpower)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_colorstrengthpower_key, color, VINTEGER);
	}
}

/*
 * Returns the display color for strength "strength".
 */
INTBIG sim_window_getdisplaycolor(INTBIG strength)
{
	if (strength == LOGIC_LOW) return(sim_colorlevellow);
	if (strength == LOGIC_HIGH) return(sim_colorlevelhigh);
	if (strength == LOGIC_X) return(sim_colorlevelundef);
	if (strength == LOGIC_Z) return(sim_colorlevelzdef);
	if (strength == OFF_STRENGTH) return(sim_colorstrengthoff);
	if (strength <= NODE_STRENGTH) return(sim_colorstrengthnode);
	if (strength <= GATE_STRENGTH) return(sim_colorstrengthgate);
	if (strength <= VDD_STRENGTH) return(sim_colorstrengthpower);
	return(0);
}

void sim_window_addsegment(WINDOWPART *wavewin, INTBIG *data, INTBIG *datapos, INTBIG fx, INTBIG fy,
	INTBIG tx, INTBIG ty)
{
	REGISTER INTBIG truefx, truefy, pos;
	INTBIG x, y;

	x = fx;   y = fy;   sim_window_mapcoord(wavewin, &x, &y);
	truefx = (x-sim_window_basex) - sim_window_offx;
	truefy = (y-sim_window_basey) - sim_window_offy;

	pos = *datapos;
	if (pos < 2 || data[pos-2] != truefx || data[pos-1] != truefy)
	{
		data[pos++] = truefx;
		data[pos++] = truefy;
	}

	x = tx;   y = ty;   sim_window_mapcoord(wavewin, &x, &y);
	data[pos++] = (x-sim_window_basex) - sim_window_offx;
	data[pos++] = (y-sim_window_basey) - sim_window_offy;
	*datapos = pos;
}

#define SIMSTEPSIZE 1e-11f
#define EDGERATE    3e-11f
#define SUPPLY      1.8

/*
 * Routine to dump vectors in SPICE format.
 */
void sim_window_writespicecmd(void)
{
	FILE *fptr;
	int j, state, prevstate;
	double simDuration;
	CHAR hvar[80], *pt;
	REGISTER TRACE *tr;
	REGISTER NETWORK *net, **netlist;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *cell, *simnt;
	REGISTER WINDOWPART *schemwin;

	/* get cell being saved */
	cell = getcurcell();
	if (cell == NONODEPROTO) return;
	schemwin = sim_window_findschematics();
	if (schemwin == NOWINDOWPART)
	{
		ttyputerr(_("Must have original schematics or layout circuit as well as waveform"));
		return;
	}
	simnt = schemwin->curnodeproto;

	/* SPICE command file named fname.cmd.sp */
	fptr = xcreate(cell->protoname, sim_filetypespicecmd, _("SPICE Command File"), &pt);
	if (fptr == NULL) return;
	ttyputmsg(_("Writing %s"), pt);

	/* Initialize variables */
	simDuration = 0.0;

	/* Create the SPICE command file */
	efprintf(fptr, x_("* %s\n"), pt);
	efprintf(fptr, x_("* SPICE command file for cell %s from library %s\n"),
		describenodeproto(cell), cell->lib->libname);
	us_emitcopyright(fptr, x_("* "), x_(""));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		(void)esnprintf(hvar, 80, x_("%s"), timetostring(getcurrenttime()));
		efprintf(fptr, x_("* Written on %s by Electric VLSI Design System, %s\n\n"),
			hvar, el_version);
	} else
	{
		efprintf(fptr, x_("* Written by Electric VLSI Design System\n\n"));
	}

	efprintf(fptr, x_(".include %s.spi\n\n"), cell->protoname);

	efprintf(fptr, x_("* Power Supply\n"));
	efprintf(fptr, x_(".param SUPPLY=%g\n"), SUPPLY);
	efprintf(fptr, x_("VVDD VDD GND SUPPLY\n\n"));

	efprintf(fptr, x_("* Inputs\n"));
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		/* only interested in digital traces with information on them */
		if ((tr->flags&TRACETYPE) != TRACEISDIGITAL) continue;
		if (tr->numsteps <= 0) continue;

		/* find the export associated with this signal */
		netlist = getcomplexnetworks(tr->name, simnt);
		net = netlist[0];
		if (net == NONETWORK || netlist[1] != NONETWORK) continue;
		for(pp = simnt->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			if (pp->network == net) break;
		if (pp == NOPORTPROTO) continue;

		/* dump information for input and bidirectional exports */
		if ((pp->userbits&STATEBITS) == INPORT ||
			(pp->userbits&STATEBITS) == BIDIRPORT)
		{
			efprintf(fptr, x_("V%s %s GND PWL "), tr->name, tr->name);
			for(j=0; j<tr->numsteps; j++)
			{
				state = tr->statearray[j] >> 8;
				if (j == 0)
				{
					efprintf(fptr, x_("0 %s "), (state==LOGIC_HIGH) ? x_("SUPPLY") : x_("0"));
				} else
				{
					pt = displayedunits((float)tr->timearray[j], VTUNITSTIME, INTTIMEUNITNSEC);
					efprintf(fptr, x_("%s %s "), pt, (prevstate==LOGIC_HIGH) ? x_("SUPPLY") : x_("0"));
				}
				if (tr->timearray[j] > 0)
				{
					pt = displayedunits((float)tr->timearray[j]+EDGERATE, VTUNITSTIME, INTTIMEUNITNSEC);
					efprintf(fptr, x_("%s %s "), pt, (state==LOGIC_HIGH) ? x_("SUPPLY") : x_("0"));
				}
				prevstate = state;
				if (tr->timearray[j] > simDuration) simDuration = tr->timearray[j];
			}
			efprintf(fptr, x_("\n"));
		}
		efprintf(fptr, x_("R%s %s GND 1G\n"), tr->name, tr->name);
	}
	efprintf(fptr, x_("\n"));

	efprintf(fptr, x_("* Simulation Commands\n"));
	efprintf(fptr, x_(".options post\n"));
	estrcpy(hvar, displayedunits(SIMSTEPSIZE, VTUNITSTIME, INTTIMEUNITNSEC));
	pt = displayedunits((float)simDuration+30.0f*EDGERATE, VTUNITSTIME, INTTIMEUNITNSEC);
	efprintf(fptr, x_(".tran %s %s\n"), hvar, pt);
	efprintf(fptr, x_(".end\n"));
	xclose(fptr);
}

/*
 * Routine to remove trace "tr" from the waveform window's linked list.
 */
void sim_window_removetrace(TRACE *tr)
{
	TRACE *lasttr, *otr;

	lasttr = NOTRACE;
	for(otr = sim_window_firstrace; otr != NOTRACE; otr = otr->nexttrace)
	{
		if (otr == tr) break;
		lasttr = otr;
	}
	if (lasttr == NOTRACE) sim_window_firstrace = tr->nexttrace; else
		lasttr->nexttrace = tr->nexttrace;
}

/*
 * Routine to renumber the traces in the window.
 */
void sim_window_renumberlines(void)
{
	REGISTER INTBIG i, lastframeno;
	REGISTER TRACE *tr, *otr;

	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
		tr->flags &= ~TRACEMARK;
	i = 0;
	for(tr = sim_window_firstrace; tr != NOTRACE; tr = tr->nexttrace)
	{
		if ((tr->flags&TRACENOTDRAWN) != 0) {tr->frameno = -1;   continue; }
		if ((tr->flags&TRACEMARK) != 0) continue;
		lastframeno = tr->frameno;
		tr->frameno = i;
		tr->flags |= TRACEMARK;
		for(otr = tr->nexttrace; otr != NOTRACE; otr = otr->nexttrace)
		{
			if ((otr->flags&TRACENOTDRAWN) != 0) {otr->frameno = -1;   continue; }
			if (otr->frameno != lastframeno) continue;
			otr->frameno = i;
			otr->flags |= TRACEMARK;
		}
		i++;
	}

	(void)sim_window_set_frames(i);
	sim_window_auto_anarange();
}

/*
 * Routine to insert trace "tr" after "aftertr" in the waveform window's linked list.
 */
void sim_window_addtrace(TRACE *tr, TRACE *aftertr)
{
	if (aftertr == NOTRACE)
	{
		tr->nexttrace = sim_window_firstrace;
		sim_window_firstrace = tr;
	} else
	{
		tr->nexttrace = aftertr->nexttrace;
		aftertr->nexttrace = tr;
	}
}

TRACE *sim_window_alloctrace(void)
{
	REGISTER TRACE *tr;

	if (sim_window_tracefree == NOTRACE)
	{
		tr = (TRACE *)emalloc(sizeof (TRACE), sim_tool->cluster);
		if (tr == 0) return(0);
	} else
	{
		tr = sim_window_tracefree;
		sim_window_tracefree = tr->nexttrace;
	}
	tr->flags = 0;
	tr->timelimit = 0;
	tr->statelimit = 0;
	tr->valuelimit = 0;
	tr->numsteps = 0;
	tr->nameoff = 0;
	tr->statearray = 0;
	tr->buschannel = NOTRACE;
	return(tr);
}

/********************************* LOW LEVEL GRAPHICS *********************************/

void sim_window_setviewport(WINDOWPART *w)
{
	float scalex, scaley;
	REGISTER INTBIG maxlines;

	w->screenlx = 0;   w->screenhx = WAVEXSIGEND;
	w->screenly = 0;   w->screenhy = WAVEYTEXTTOP;
	computewindowscale(w);

	scalex = (w->usehx - w->uselx) / (float)WAVEXSIGEND;
	scaley = ((w->usehy-20) - w->usely) / (float)WAVEYTEXTTOP;
	if (scalex > 2.0 && scaley > 2.0) sim_window_textsize = 0; else
		if (scalex > 1.0 && scaley > 1.0) sim_window_textsize = 16; else
			sim_window_textsize = 12;

	maxlines = ((w->usehy - w->usely) * WAVEYSIGRANGE / WAVEYTEXTTOP) / 25;
	if (maxlines != sim_window_visframes)
	{
		if (sim_window_lines > sim_window_visframes)
			sim_window_visframes = sim_window_lines;
		if (sim_window_visframes > maxlines)
			sim_window_visframes = maxlines;
	}
}

/*
 * Routine to return the WINDOWPART that has the waveform display
 */
WINDOWPART *sim_window_findwaveform(void)
{
	WINDOWPART *w;

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if ((w->state & WINDOWTYPE) == WAVEFORMWINDOW)
			return(w);
	return(NOWINDOWPART);
}

/*
 * Routine to return the WINDOWPART that has the schematics/layout being simulated
 */
WINDOWPART *sim_window_findschematics(void)
{
	WINDOWPART *w;
	INTBIG state;

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		state = w->state & (WINDOWTYPE|WINDOWSIMMODE);
		if (state == (DISPWINDOW|WINDOWSIMMODE) ||
			state == (DISP3DWINDOW|WINDOWSIMMODE))
				return(w);
	}
	return(NOWINDOWPART);
}

void sim_window_mapcoord(WINDOWPART *w, INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG newx, newy;

	newx = w->uselx + applyxscale(w, *x - w->screenlx);
	newy = w->usely + (*y - w->screenly) *
		((w->usehy-20) - (w->usely)) / (w->screenhy - w->screenly);
	*x = newx;   *y = newy;
}

void sim_window_scaletowindow(WINDOWPART *w, INTBIG *x, INTBIG *y)
{
	*x = muldiv(*x - w->uselx, w->screenhx - w->screenlx,
		w->usehx - w->uselx) + w->screenlx;
	*y = muldiv(*y - w->usely, w->screenhy - w->screenly,
		(w->usehy-20) - w->usely) + w->screenly;
}

/* Some Control Sequences for the graphics terminal */
void sim_window_setcolor(INTBIG color)
{
	sim_window_desc.col = color;
}

void sim_window_setmask(INTBIG mask)
{
	sim_window_desc.bits = mask;
}

/* Graphics Commands */
void sim_window_moveto(INTBIG x, INTBIG y)
{
	sim_window_curx = x;
	sim_window_cury = y;
}

void sim_window_drawto(WINDOWPART *w, INTBIG x, INTBIG y, INTBIG texture)
{
	INTBIG fx, fy, tx, ty, cfx, cfy, ctx, cty;

	fx = sim_window_curx;   fy = sim_window_cury;
	tx = x;   ty = y;
	sim_window_mapcoord(w, &fx, &fy);
	sim_window_mapcoord(w, &tx, &ty);
	cfx = fx;   cfy = fy;
	ctx = tx;   cty = ty;
	if (!clipline(&cfx, &cfy, &ctx, &cty, w->uselx, w->usehx, w->usely, w->usehy))
	{
		fx = cfx;   fy = cfy;
		tx = ctx;   ty = cty;
		screendrawline(w, fx, fy, tx, ty, &sim_window_desc, texture);
	}
	sim_window_curx = x;
	sim_window_cury = y;
}

/*
 * Routine to draw from "xll" to "xur" and from "yll" to "yur" in window "w".  "style" is:
 *  0 to draw solid
 *  1 to draw texture
 *  2 to draw solid but expand the Y values by 1 pixel
 */
void sim_window_drawbox(WINDOWPART *w, INTBIG xll, INTBIG yll, INTBIG xur, INTBIG yur, INTBIG style)
{
	INTBIG lx, ly, hx, hy, swap;

	lx = xll;   ly = yll;
	hx = xur;   hy = yur;
	sim_window_mapcoord(w, &lx, &ly);
	sim_window_mapcoord(w, &hx, &hy);
	if (lx > hx) { swap = lx;   lx = hx;   hx = swap; }
	if (ly > hy) { swap = ly;   ly = hy;   hy = swap; }
	if (style == 1)
	{
		sim_window_udesc.bits = sim_window_desc.bits;
		sim_window_udesc.col = sim_window_desc.col;
		screendrawbox(w, lx, hx, ly, hy, &sim_window_udesc);
	} else
	{
		if (style == 2)
		{
			ly--;   hy++;
		}
		screendrawbox(w, lx, hx, ly, hy, &sim_window_desc);
	}
}

/*
 * Routine to draw string "s" in window "w" with highlighting and by erasing what
 * is underneath first
 */
void sim_window_drawhcstring(WINDOWPART *w, CHAR *s)
{
	INTBIG xw, yw, oldcol, oldmask;
	INTBIG px, py;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	px = sim_window_curx;   py = sim_window_cury;
	sim_window_mapcoord(w, &px, &py);

	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(sim_window_textsize));
	screensettextinfo(w, NOTECHNOLOGY, descript);
	screengettextsize(w, s, &xw, &yw);
	py -= yw/2;

	/* erase the area underneath */
	oldcol = sim_window_desc.col;
	oldmask = sim_window_desc.bits;
	sim_window_desc.col = ALLOFF;
	sim_window_desc.bits = LAYERA;
	screendrawbox(w, px, px+xw, py, py+yw, &sim_window_desc);
	sim_window_desc.col = oldcol;
	sim_window_desc.bits = oldmask;

	/* draw the text */
	screendrawtext(w, px, py, s, &sim_window_desc);
}

void sim_window_drawcstring(WINDOWPART *w, CHAR *s)
{
	INTBIG wid, hei;
	INTBIG px, py;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	px = sim_window_curx;   py = sim_window_cury;
	sim_window_mapcoord(w, &px, &py);
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(sim_window_textsize));
	screensettextinfo(w, NOTECHNOLOGY, descript);
	screengettextsize(w, s, &wid, &hei);
	screendrawtext(w, px-wid/2, py, s, &sim_window_desc);
}

void sim_window_drawulstring(WINDOWPART *w, CHAR *s)
{
	INTBIG wid, hei;
	INTBIG px, py;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	px = sim_window_curx;   py = sim_window_cury;
	sim_window_mapcoord(w, &px, &py);
	TDCLEAR(descript);
	TDSETSIZE(descript, TXTSETPOINTS(sim_window_textsize));
	screensettextinfo(w, NOTECHNOLOGY, descript);
	screengettextsize(w, s, &wid, &hei);
	screendrawtext(w, px, py-hei, s, &sim_window_desc);
}

/*
 * Name: sim_windowconvertengineeringnotation
 *
 * Description:
 *	This procedure converts a floating point number and represents time
 * in engineering units such as pico, micro, milli, etc.
 *
 * Calling Arguments:
 *	time = floating point value to be converted to Engineering notation
 *	s1   = pointer to a string that will be used to print the converted value
 */
CHAR *sim_windowconvertengineeringnotation(double time)
{
	REGISTER INTBIG timeleft, timeright;
	double scaled;
	INTBIG inttime;
	CHAR *sectype;
	REGISTER CHAR *negative;
	static CHAR s1[50];

	negative = x_("");
	if (time < 0.0)
	{
		negative = x_("-");
		time = - time;
	}
	if (doublesequal(time, 0.0))
	{
		(void)esnprintf(s1, 50, x_("0 %s"), _("s"));
		return(s1);
	}
	if (time < 1.0E-15 || time >= 1000.0)
	{
		(void)esnprintf(s1, 50, x_("%s%g%s"), negative, time, _("s"));
		return(s1);
	}

	/* get proper time unit to use */
	scaled = time * 1.0E17;   inttime = rounddouble(scaled);
	if (scaled < 200000.0 && inttime < 100000)
	{
		sectype = _("fs");
	} else
	{
		scaled = time * 1.0E14;   inttime = rounddouble(scaled);
		if (scaled < 200000.0 && inttime < 100000)
		{
			sectype = _("ps");
		} else
		{
			scaled = time * 1.0E11;   inttime = rounddouble(scaled);
			if (scaled < 200000.0 && inttime < 100000)
			{
				sectype = _("ns");
			} else
			{
				scaled = time * 1.0E8;   inttime = rounddouble(scaled);
				if (scaled < 200000.0 && inttime < 100000)
				{
					sectype = _("us");
				} else
				{
					scaled = time * 1.0E5;   inttime = rounddouble(scaled);
					if (scaled < 200000.0 && inttime < 100000)
					{
						sectype = _("ms");
					} else
					{
						inttime = rounddouble(time * 1.0E2);
						sectype = _("s");
					}
				}
			}
		}
	}
	timeleft = inttime / 100;
	timeright = inttime % 100;
	if (timeright == 0)
	{
		(void)esnprintf(s1, 50, x_("%s%ld%s"), negative, timeleft, sectype);
	} else
	{
		if ((timeright%10) == 0)
		{
			(void)esnprintf(s1, 50, x_("%s%ld.%ld%s"), negative, timeleft, timeright/10, sectype);
		} else
		{
			(void)esnprintf(s1, 50, x_("%s%ld.%02ld%s"), negative, timeleft, timeright, sectype);
		}
	}
	return(s1);
}

/*
 * Name: sim_windowconvertengineeringnotation2
 *
 * Description:
 *	This procedure converts a floating point number and represents time
 * in engineering units such as pico, micro, milli, etc.
 *
 * Calling Arguments:
 *	time = floating point value to be converted to Engineering notation
 *  precpower = decimal power of necessary time precision
 *	s1   = pointer to a string that will be used to print the converted value
 */
CHAR *sim_windowconvertengineeringnotation2(double time, INTBIG precpower)
{
	REGISTER INTBIG timeleft, timeright;
	double scaled;
	INTBIG inttime;
	CHAR *sectype;
	REGISTER CHAR *negative;
	INTBIG scalepower;
	static CHAR s1[50];

	negative = x_("");
	if (time < 0.0)
	{
		negative = x_("-");
		time = - time;
	}
	if (doublesequal(time, 0.0))
	{
		(void)esnprintf(s1, 50, x_("0 %s"), _("s"));
		return(s1);
	}
	if (time < 1.0E-15 || time >= 1000.0)
	{
		(void)esnprintf(s1, 50, x_("%s%g%s"), negative, time, _("s"));
		return(s1);
	}

	/* get proper time unit to use */
	scaled = time * 1.0E17;   inttime = rounddouble(scaled);
	if (scaled < 200000.0 && inttime < 100000)
	{
		sectype = _("fs");
		scalepower = -15;
	} else
	{
		scaled = time * 1.0E14;   inttime = rounddouble(scaled);
		if (scaled < 200000.0 && inttime < 100000)
		{
			sectype = _("ps");
			scalepower = -12;
		} else
		{
			scaled = time * 1.0E11;   inttime = rounddouble(scaled);
			if (scaled < 200000.0 && inttime < 100000)
			{
				sectype = _("ns");
				scalepower = -9;
			} else
			{
				scaled = time * 1.0E8;   inttime = rounddouble(scaled);
				if (scaled < 200000.0 && inttime < 100000)
				{
					sectype = _("us");
					scalepower = -6;
				} else
				{
					scaled = time * 1.0E5;   inttime = rounddouble(scaled);
					if (scaled < 200000.0 && inttime < 100000)
					{
						sectype = _("ms");
						scalepower = -3;
					} else
					{
						scaled = time * 1.0E2;  inttime = rounddouble(scaled);
						sectype = _("s");
						scalepower = 0;
					}
				}
			}
		}
	}
	if (precpower < scalepower)
	{
		scaled /= 1.0E2;
		esnprintf(s1, 50, x_("%s%.*f%s"), negative, (int)(scalepower - precpower), scaled, sectype);
	} else
	{
		timeleft = inttime / 100;
		timeright = inttime % 100;
		if (timeright == 0)
		{
			(void)esnprintf(s1, 50, x_("%s%ld%s"), negative, timeleft, sectype);
		} else
		{
			if ((timeright%10) == 0)
			{
				(void)esnprintf(s1, 50, x_("%s%ld.%ld%s"), negative, timeleft, timeright/10, sectype);
			} else
			{
				(void)esnprintf(s1, 50, x_("%s%ld.%02ld%s"), negative, timeleft, timeright, sectype);
			}
		}
	}
	return(s1);
}

/*
 * Routine to look at the "count" signal values in "value" and convert them to a readable
 * value in the current simulation display base.
 */
CHAR *sim_window_makewidevalue(INTBIG count, INTBIG *value)
{
	REGISTER INTBIG i, j, k, val, base, groupsize;
	INTHUGE bigval;
	static CHAR s2[MAXSIMWINDOWBUSWIDTH+3];
	static CHAR *hexstring = x_("0123456789ABCDEF");

	/* if any values are undefined, the result is "XX" */
	for(i=0; i<count; i++)
		if (value[i] != LOGIC_LOW && value[i] != LOGIC_HIGH) return(x_("XX"));

	/* bases 2, 8 and 16 are simple */
	base = sim_window_state & BUSBASEBITS;
	if (base == BUSBASE10 && count > 64) base = BUSBASE16;
	if (base == BUSBASE2 || base == BUSBASE8 || base == BUSBASE16)
	{
		switch (base)
		{
			case BUSBASE2:  groupsize = 1;   break;
			case BUSBASE8:  groupsize = 3;   break;
			case BUSBASE16: groupsize = 4;   break;
			default: return(0);
		}
		j = MAXSIMWINDOWBUSWIDTH+3;
		s2[--j] = 0;
		for(i = count-1; i >= 0; i -= groupsize)
		{
			val = 0;
			for(k=0; k<groupsize; k++)
			{
				if (i-k < 0) break;
				if (value[i-k] == LOGIC_HIGH) val |= (1<<k);
			}
			s2[--j] = hexstring[val];
		}
		switch (base)
		{
			case BUSBASE2:  s2[--j] = 'b';  s2[--j] = '0';   break;
			case BUSBASE8:  s2[--j] = '0';                   break;
			case BUSBASE16: s2[--j] = 'x';  s2[--j] = '0';   break;
		}
		return(&s2[j]);
	}

	bigval = 0;
	for(i=0; i<count; i++)
	{
		bigval <<= 1;
		if (value[i] == LOGIC_HIGH) bigval |= 1;
	}
	return(hugeinttoa(bigval));
}

/* Simulation: Wide Value */
static DIALOGITEM sim_widedialogitems[] =
{
 /*  1 */ {0, {48,188,72,268}, BUTTON, N_("OK")},
 /*  2 */ {0, {12,188,36,268}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,8,24,168}, MESSAGE, N_("Value to set on bus:")},
 /*  4 */ {0, {32,8,48,168}, EDITTEXT, x_("")},
 /*  5 */ {0, {56,8,72,72}, MESSAGE, N_("Base:")},
 /*  6 */ {0, {56,76,72,144}, POPUP, x_("")}
};
static DIALOG sim_widedialog = {{75,75,156,352}, N_("Set Wide Value"), 0, 6, sim_widedialogitems, 0, 0};

/* special items for the "wide value" dialog: */
#define DSWV_VALUE     4		/* value (edit text) */
#define DSWV_BASE      6		/* base (popup) */

/*
 * Routine to prompt for a wide value and return the number of bits.
 * The bit values are stored in "bits".
 * Returns negative on abort/error.
 */
INTBIG sim_window_getwidevalue(INTBIG **bits)
{
	REGISTER INTBIG i, j, itemHit, groupsize, testbit, val;
	REGISTER UINTHUGE bigval, bigtestbit;
	REGISTER CHAR *pt, chr;
	REGISTER void *dia;
	static INTBIG theBits[MAXSIMWINDOWBUSWIDTH];
	static INTBIG lastbase = 2;
	static CHAR *base[4] = {x_("2"), x_("8"), x_("10"), x_("16")};

	dia = DiaInitDialog(&sim_widedialog);
	if (dia == 0) return(-1);
	DiaSetPopup(dia, DSWV_BASE, 4, base);
	DiaSetPopupEntry(dia, DSWV_BASE, lastbase);
	DiaSetText(dia, -DSWV_VALUE, x_("0"));
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}
	lastbase = DiaGetPopupEntry(dia, DSWV_BASE);
	if (lastbase == 2)
	{
		/* base 10 is special */
		bigval = 0;
		for(pt = DiaGetText(dia, DSWV_VALUE); *pt != 0; pt++)
		{
			if (!isdigit(*pt)) break;
			bigval = bigval * 10 + (*pt - '0');
		}
		bigtestbit = INTHUGECONST(0x8000000000000000);
		for(i=0; i<64; i++)
		{
			if ((bigval & bigtestbit) != 0) theBits[i] = 1; else
				theBits[i] = 0;
			bigtestbit >>= 1;
		}
		DiaDoneDialog(dia);
		*bits = theBits;
		return(64);
	}

	/* bases 2, 8, and 16 are simpler */
	switch (lastbase)
	{
		case 0: groupsize = 1;   break;		/* base 2 */
		case 1: groupsize = 3;   break;		/* base 8 */
		case 3: groupsize = 4;   break;		/* base 16 */
		default: return(-1);
	}
	j = 0;
	for(pt = DiaGetText(dia, DSWV_VALUE); *pt != 0; pt++)
	{
		chr = tolower(*pt);
		if (chr >= '0' && chr <= '9') val = chr - '0'; else
			if (chr >= 'a' && chr <= 'f') val = chr - 'a' + 10; else
				val = 100;
		if (val >= (1 << groupsize))
		{
			DiaMessageInDialog(_("Invalid base %s value"), base[lastbase]);
			break;
		}
		testbit = 1 << (groupsize-1);
		for(i=0; i<groupsize; i++)
		{
			if ((val&testbit) != 0) theBits[j] = 1; else
				theBits[j] = 0;
			j++;
			testbit >>= 1;
		}
	}
	DiaDoneDialog(dia);
	*bits = theBits;
	return(j);
}

void sim_window_auto_anarange(void)
{
	TRACE *tr;
	int i, *t;
	double l, h;

	if (sim_window_lines > 0)
		t = (int *)emalloc(sim_window_lines*sizeof(int), sim_tool->cluster);
	else
		t = NULL;
	for(i=0; i < sim_window_lines; i++) t[i] = 0;

	l = 0;   h = 1;
	for(i=0, tr = sim_window_firstrace; tr != NOTRACE; ++i, tr = tr->nexttrace) {

		if (i == 0 ) {
			l  = tr->anadislow;
			h = tr->anadishigh;
		} else {
			if ( tr->anadislow  < l ) l = tr->anadislow;
			if ( tr->anadishigh > h ) h = tr->anadishigh;
		}

		if (tr->frameno < sim_window_lines && tr->frameno >=0 ) {
			if (t[tr->frameno] == 0) {
				sim_window_frame_analow[tr->frameno] = tr->anadislow;
				sim_window_frame_anahigh[tr->frameno] = tr->anadishigh;
				t[tr->frameno] = 1;
			} else {
				if (sim_window_frame_analow[tr->frameno] > tr->anadislow)
					sim_window_frame_analow[tr->frameno] = tr->anadislow;

				if (sim_window_frame_anahigh[tr->frameno] < tr->anadishigh)
					sim_window_frame_anahigh[tr->frameno] = tr->anadishigh;
			}
		}
	}

	if (t) efree((CHAR *)t);

	sim_window_analow = (float)l;
	sim_window_anahigh = (float)h;
	sim_window_anarange = (float)(h - l);

}

void sim_window_trace_range(INTBIG tri)
{
	TRACE *tr;
	int j;
	float h, l;

	tr = (TRACE *)tri;

	if (tr->numsteps >0) {
		h = tr->valuearray[0];
		l = tr->valuearray[0];
	}

	for (j=1; j<tr->numsteps; ++j) {
		if ( tr->valuearray[j] < l ) l = tr->valuearray[j];
		if ( tr->valuearray[j] > h ) h = tr->valuearray[j];
	}

/*
	d = fabs(h-l);
	if (d/(h+l)<DBL_EPSILON) d = 0.5;
	double m = 1.0;
	while ( d > 10.0 ) { d/=10.0; m*=10.0; }
	while ( d < 1.0  ) { d*=10.0; m/=10.0; }

	int li = (int)(l/m);
	int hi = (int)(h/m);
	if (li < 0) --li;
	if (hi > 0) ++hi;
	l = li * m;
	h = hi * m;
 */

#if 1
	if (l == h)
	{
		l -= 0.5;
		h += 0.5;
	}
#endif
	tr->anadislow   = l;
	tr->anadishigh  = h;
	tr->anadisrange = h-l;
}

int sim_window_set_frames(int n)
{
	int i;
	INTBIG newlimit;
	float *newanalow, *newanahigh, *newtopval, *newbotval;

	if (n > sim_window_lines_limit)
	{
		newlimit = (sim_window_lines_limit < 16 ? 16 : sim_window_lines_limit);
		while(newlimit < n) newlimit *= 2;
		newanalow = (float *)emalloc(newlimit*sizeof(float), sim_tool->cluster);
		newanahigh = (float *)emalloc(newlimit*sizeof(float), sim_tool->cluster);
		newtopval = (float *)emalloc(newlimit*sizeof(float), sim_tool->cluster);
		newbotval = (float *)emalloc(newlimit*sizeof(float), sim_tool->cluster);
		if(newanalow == NULL || newanahigh == NULL || newtopval == NULL || newbotval == NULL)
		{
			if(newanalow != NULL) efree((CHAR *)newanalow);
			if(newanahigh != NULL) efree((CHAR *)newanahigh);
			if(newtopval != NULL) efree((CHAR *)newtopval);
			if(newbotval != NULL) efree((CHAR *)newbotval);
			return 0;
		}
		for (i=0; i<sim_window_lines; ++i) {
			newanalow[i] = sim_window_frame_analow[i];
			newanahigh[i] = sim_window_frame_anahigh[i];
			newtopval[i] = sim_window_frame_topval[i];
			newbotval[i] = sim_window_frame_botval[i];
		}
		if (sim_window_lines_limit > 0)
		{
			efree((CHAR *)sim_window_frame_analow);
			efree((CHAR *)sim_window_frame_anahigh);
			efree((CHAR *)sim_window_frame_topval);
			efree((CHAR *)sim_window_frame_botval);
		}
		sim_window_frame_analow = newanalow;
		sim_window_frame_anahigh = newanahigh;
		sim_window_frame_topval = newtopval;
		sim_window_frame_botval = newbotval;
		sim_window_lines_limit = newlimit;
	}

	for (i=sim_window_lines; i<n; ++i) {
		sim_window_frame_analow[i]  = sim_window_analow;
		sim_window_frame_anahigh[i] = sim_window_anahigh;
		sim_window_frame_topval[i] = sim_window_analow;
		sim_window_frame_botval[i] = sim_window_anahigh;
	}

	sim_window_lines = n;
	return 1;
}

float sim_window_get_analow(int frameno)
{
	if (frameno >=0 && frameno < sim_window_lines)
		return sim_window_frame_analow[frameno];
	else
		return sim_window_analow;
}

float sim_window_get_anahigh(int frameno)
{
	if (frameno >=0 && frameno < sim_window_lines)
		return sim_window_frame_anahigh[frameno];
	else
		return sim_window_anahigh;
}

float sim_window_get_topval(int frameno)
{
	if (frameno >=0 && frameno < sim_window_lines)
		return sim_window_frame_topval[frameno];
	else
		return sim_window_anahigh;
}

float sim_window_get_botval(int frameno)
{
	if (frameno >=0 && frameno < sim_window_lines)
		return sim_window_frame_botval[frameno];
	else
		return sim_window_analow;
}

void sim_window_set_anahigh(int frameno, float value)
{
	if (frameno >=0 && frameno < sim_window_lines)
		sim_window_frame_anahigh[frameno] = value;
}

void sim_window_set_analow(int frameno, float value)
{
	if (frameno >=0 && frameno < sim_window_lines)
		sim_window_frame_analow[frameno] = value;
}

void sim_window_set_topval(int frameno, float value)
{
	if (frameno >=0 && frameno < sim_window_lines)
		sim_window_frame_topval[frameno] = value;
}

void sim_window_set_botval(int frameno, float value)
{
	if (frameno >=0 && frameno < sim_window_lines)
		sim_window_frame_botval[frameno] = value;
}

float sim_window_get_anarange(int frameno)
{
	if (frameno >=0 && frameno < sim_window_lines)
		return sim_window_frame_anahigh[frameno] - sim_window_frame_analow[frameno];
	else
		return sim_window_anarange;
}

float sim_window_get_extanarange(int frameno)
{
	if (frameno >=0 && frameno < sim_window_lines)
	{
		float diff;
		diff = sim_window_frame_topval[frameno] - sim_window_frame_botval[frameno];
		if (!floatsequal(diff, 0.0)) return(diff);
		return(sim_window_frame_anahigh[frameno] - sim_window_frame_analow[frameno]);
	}
	else
		return sim_window_anarange;
}

void sim_window_zoom_frame(INTBIG frameno)
{
	float v;
	if (frameno >=0 && frameno < sim_window_lines) {
		v = sim_window_get_anarange(frameno);
		v /= 4.0f;
		sim_window_frame_anahigh[frameno] -=v;
		sim_window_frame_analow[frameno] +=v;
	}
}

void sim_window_zoomout_frame(INTBIG frameno)
{
	float v;
	if (frameno >=0 && frameno < sim_window_lines) {
		v = sim_window_get_anarange(frameno);
		v /= 2.0f;
		sim_window_frame_anahigh[frameno] +=v;
		sim_window_frame_analow[frameno] -=v;
	}
}

void sim_window_shiftup_frame(INTBIG frameno)
{
	float v;
	if (frameno >=0 && frameno < sim_window_lines) {
		v = sim_window_get_anarange(frameno);
		v /= 4.0f;
		sim_window_frame_anahigh[frameno] +=v;
		sim_window_frame_analow[frameno] +=v;
	}
}

void sim_window_shiftdown_frame(INTBIG frameno)
{
	float v;
	if (frameno >=0 && frameno < sim_window_lines) {
		v = sim_window_get_anarange(frameno);
		v /= 4.0f;
		sim_window_frame_anahigh[frameno] -=v;
		sim_window_frame_analow[frameno] -=v;
	}
}

/* This function takes a high value h, a low value l, chops the interval between
h and l in n bits, and rounds the high, low and the interval to a 1,2,5
range.  The integers i1 and i2 are the powers of 10 that belong to the
largest value in the interval and the step size.  These integers can be
used for printing the scale. */

float sim_window_sensible_value(float *h, float *l, INTBIG *i1, INTBIG *i2, INTBIG n)
{
	float d, m, max;
	int di, hi, li;

	if (fabs(*h) > fabs(*l)) max = (float)fabs(*h); else max = (float)fabs(*l);
	*i1 = *i2 = 0;
	if (max == 0.0) return(0.0);

	while ( max >= 10.0 ) { max/=10.0; ++*i1; }
	while ( max <= 1.0  ) { max*=10.0; --*i1; }

	d = (float)fabs(*h - *l)/(float)n;

	if ((float)fabs(d/(*h+*l))<FLT_EPSILON) d = 0.1f;
	m=1.0;
	while ( d >= 10.0 ) { d/=10.0; m*=10.0; ++*i2; }
	while ( d <= 1.0  ) { d*=10.0; m/=10.0; --*i2; }

	di = (int)d;

	if (di > 2 && di <= 5) di=5;
	else 
		if (di > 5) di = 10;

	li = (int)(*l/m);
	hi = (int)(*h/m);
	li = (li/di)*di;
	hi = (hi/di)*di;
	if (li<0) li-=di;
	if (hi>0) hi+=di;
	*l = (float)((double)li*m);
	*h = (float)((double)hi*m);

	return (float)di*m;
}

CHAR *sim_window_prettyprint(float v, int i1, int i2)
{
	double d;
	int i, p;
	static CHAR s[20];

	d = 1.0;
	if (i2 > 0)
		for (i = 0; i < i2; ++i) d *= 10.0;
	if (i2 < 0)
		for (i = 0; i > i2; --i) d /= 10.0;

	if (fabs((double)v)*100.0 < d)
	{
		esnprintf(s, 20, x_("0"));
	} else
	{
		if (i1 <= 4 && i1 >=0 && i2 >=0)
		{
			esnprintf(s, 20, x_("%.0f"), v);
		} else
		{
			if (i1 <= 4 && i1 >= -2 && i2 < 0)
			{
				esnprintf(s, 20, x_("%.*f"), (-i2), v);
			} else
			{
				p = i1-12-1;
				if (p < 0) p=0;
				esnprintf(s, 20, x_("%.*fe%+2d"), p, (double)v/d, i2);
			}
		}
	}
	return(s);
}

#endif  /* SIMTOOL - at top */
