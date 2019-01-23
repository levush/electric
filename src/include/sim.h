/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sim.h
 * Simulation tool: header file
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

/********************* for all simulators *********************/

#define ESIM       0					/* ESIM from MIT (SIM format) */
#define RSIM       1					/* RSIM from MIT (SIM format) */
#define RNL        2					/* RNL from MIT (SIM format) */
#define COSMOS     3					/* COSMOS from CMU (SIM format) */
#define SPICE      4					/* SPICE from Berkeley (SPICE format) */
#define MOSSIM     5					/* MOSSIM from CalTech (NTK format) */
#define TEXSIM     6					/* TEGAS from Calma (TDL format) */
#define ABEL       7					/* ABEL from Data I/O (PAL programmer) */
#define VERILOG    8					/* VERILOG from Gateway (VER format) */
#define SILOS      9					/* SILOS from Simucad */
#define ALS       10					/* ALS (NET format) */
#define FASTHENRY 11					/* FastHenry (INP format) */
#define IRSIM     12					/* IRSIM from Stanford (SIM format) */
#define CDL       13					/* CDL (SPICE with no headers or parameters) */
#define MAXWELL   14					/* MAXWELL (electromagnetic simulation) */

/* the meaning of "SIM_dontrun" */
#define SIMRUNYES       -1				/* run simulation, show output */
#define SIMRUNYESPARSE   0				/* run simulation, show output, parse output */
#define SIMRUNNO         1				/* do not runs simulation */
#define SIMRUNYESQPARSE  2				/* run simulation, parse output */
#define SIMRUNYESQ       3				/* run simulation */

extern TOOL      *sim_tool;				/* the Simulator tool object */
extern INTBIG     sim_formatkey;		/* key for "SIM_format" (ESIM, etc) */
extern INTBIG     sim_netfilekey;		/* key for "SIM_netfile" */
extern INTBIG     sim_dontrunkey;		/* key for "SIM_dontrun" */
extern INTBIG     sim_weaknodekey;		/* key for "SIM_weak_node" */
extern INTBIG     sim_spice_partskey;	/* key for "SIM_spice_parts" */
extern CHAR      *sim_spice_parts;		/* cached value for "SIM_spice_parts" */
extern NODEPROTO *sim_simnt;			/* cell being simulated */
#if defined(__cplusplus)
extern EProcess  *sim_process;          /* process of simulator */
#endif
extern INTBIG     sim_filetypeesim;		/* ESIM netlist disk file descriptor */
extern INTBIG     sim_filetypefasthenry;/* FastHenry netlist disk file descriptor */
extern INTBIG     sim_filetypemossim;	/* MOSSIM netlist disk file descriptor */
extern INTBIG     sim_filetypepal;		/* PAL netlist disk file descriptor */
extern INTBIG     sim_filetypeals;		/* ALS netlist disk file descriptor */
extern INTBIG     sim_filetypealsvec;	/* ALS vectors disk file descriptor */
extern INTBIG     sim_filetypeirsimcmd;	/* IRSIM command (vectors) file descriptor */
extern INTBIG     sim_filetypenetlisp;	/* Netlisp netlist disk file descriptor */
extern INTBIG     sim_filetypequisc;	/* QUISC netlist disk file descriptor */
extern INTBIG     sim_filetypersim;		/* RSIM netlist disk file descriptor */
extern INTBIG     sim_filetypeirsim;	/* IRSIM netlist disk file descriptor */
extern INTBIG     sim_filetypesilos;	/* Silos netlist disk file descriptor */
extern INTBIG     sim_filetypespice;	/* SPICE input disk file descriptor */
extern INTBIG     sim_filetypespicecmd;	/* SPICE command disk file descriptor */
extern INTBIG     sim_filetypespiceout;	/* SPICE output disk file descriptor */
extern INTBIG     sim_filetypehspiceout;/* HSPICE output file descriptor */
extern INTBIG     sim_filetyperawspiceout; /* SPICE raw output disk file descriptor */
extern INTBIG     sim_filetypesrawspiceout; /* SmartSPICE raw output disk file descriptor */
extern INTBIG     sim_filetypecdl;		/* CDL output file descriptor */
extern INTBIG     sim_filetypectemp;	/* CDL template disk file descriptor */
extern INTBIG     sim_filetypemaxwell;	/* MAXWELL output file descriptor */
extern INTBIG     sim_filetypetegas;	/* Tegas netlist disk file descriptor */
extern INTBIG     sim_filetypetegastab;	/* Tegas table disk file descriptor */
extern INTBIG     sim_filetypeverilog;	/* Verilog disk file descriptor */
extern INTBIG     sim_filetypeverilogvcd;	/* Verilog VCD dump file descriptor */

/********************* for Simulation Window *********************/

/* the bits returned by "sim_window_isactive()" */
#define SIMWINDOWWAVEFORM       1		/* set if waveform window is active */
#define SIMWINDOWSCHEMATIC      2		/* set if schematic window is active */

#define MAXSIMWINDOWBUSWIDTH  300		/* maximum width of a bus in thw waveform window */

/* logic levels and signal strengths in the window */
#define LOGIC_LOW              -1
#define LOGIC_X                -2
#define LOGIC_HIGH             -3
#define LOGIC_Z                -4
#define OFF_STRENGTH            0
#define NODE_STRENGTH           2
#define GATE_STRENGTH           4
#define VDD_STRENGTH            6

/* the meaning of "SIM_window_state" */
#define FULLSTATE              01		/* set for full 12-state simulation */
#define SHOWWAVEFORM           02		/* set to show waveform window */
#define ADVANCETIME            04		/* set to advance time to end of simulation */
#define BUSBASEBITS           070		/* base to use for bus display */
#define BUSBASE10               0		/*   use base 10 for bus display */
#define BUSBASE2              010		/*   use base 2 for bus display */
#define BUSBASE8              020		/*   use base 8 for bus display */
#define BUSBASE16             040		/*   use base 16 for bus display */
#define WAVEPLACE            0300		/* location of waveform windows */
#define WAVEPLACECAS            0		/*   cascade waveform windows */
#define WAVEPLACETHOR        0100		/*   cascade waveform windows */
#define WAVEPLACETVER        0200		/*   cascade waveform windows */
#define SIMENGINE           01400		/* simulation engine to use for "simulate" command */
#define SIMENGINEALS            0		/*   use ALS simulation engine */
#define SIMENGINEIRSIM       0400		/*   use IRSIM simulation engine */
#define SIMENGINECUR       016000		/* simulation in this window */
#define SIMENGINECURALS         0		/*   current window has ALS simulation */
#define SIMENGINECURIRSIM   02000		/*   current window has IRSIM simulation */
#define SIMENGINECURVERILOG 04000		/*   current window has Verilog simulation */
#define SIMENGINECURSPICE  010000		/*   current window has SPICE simulation */

extern INTBIG  sim_window_statekey;			/* key for "SIM_window_state" */
extern INTBIG  sim_window_state;			/* cached value of "SIM_window_state" */
extern INTBIG  sim_window_signalorder_key;	/* key for "SIM_window_signalorder" */
extern INTBIG  sim_window_hierpos_key;		/* key for "SIM_window_hierarchy_pos" */
extern INTBIG  sim_window_format;			/* type of simulation in window */
extern INTBIG  sim_colorstrengthoff;		/* color of off-strength */
extern INTBIG  sim_colorstrengthnode;		/* color of node-strength */
extern INTBIG  sim_colorstrengthgate;		/* color of gate-strength */
extern INTBIG  sim_colorstrengthpower;		/* color of power-strength */
extern INTBIG  sim_colorlevellow;			/* color of low-levels */
extern INTBIG  sim_colorlevelhigh;			/* color of high-levels */
extern INTBIG  sim_colorlevelundef;			/* color of undefined levels */
extern INTBIG  sim_colorlevelzdef;			/* color of Z levels */
extern INTBIG  sim_colorstrengthoff_key;	/* key for "SIM_window_color_str0" */
extern INTBIG  sim_colorstrengthnode_key;	/* key for "SIM_window_color_str1" */
extern INTBIG  sim_colorstrengthgate_key;	/* key for "SIM_window_color_str2" */
extern INTBIG  sim_colorstrengthpower_key;	/* key for "SIM_window_color_str3" */
extern INTBIG  sim_colorlevellow_key;		/* key for "SIM_window_color_low" */
extern INTBIG  sim_colorlevelhigh_key;		/* key for "SIM_window_color_high" */
extern INTBIG  sim_colorlevelundef_key;		/* key for "SIM_window_color_X" */
extern INTBIG  sim_colorlevelzdef_key;		/* key for "SIM_window_color_Z" */

/* prototypes */
void    sim_window_init(void);
BOOLEAN sim_window_create(INTBIG, NODEPROTO*, BOOLEAN(*)(WINDOWPART*, INTSML, INTBIG),
						  BOOLEAN(*)(WINDOWPART*, INTSML, INTBIG), INTBIG);
void    sim_window_titleinfo(CHAR *title);
void    sim_window_stopsimulation(void);
void    sim_window_setvdd(float vdd);
INTBIG  sim_window_isactive(NODEPROTO **np);
void    sim_window_redraw(void);
void    sim_window_setnumframes(INTBIG);
INTBIG  sim_window_getnumframes(void);
INTBIG  sim_window_getnumvisframes(void);
INTBIG  sim_window_getcurframe(void);
void    sim_window_settopvisframe(INTBIG count);
INTBIG  sim_window_gettopvisframe(void);
void    sim_window_savegraph(void);
void    sim_window_writespicecmd(void);
void    sim_window_setdisplaycolor(INTBIG strength, INTBIG color);
INTBIG  sim_window_getdisplaycolor(INTBIG strength);
INTBIG  sim_window_newtrace(INTBIG, CHAR*, INTBIG);
BOOLEAN sim_window_buscommand(void);
INTBIG  sim_window_makebus(INTBIG count, INTBIG *traces, CHAR *busname);
void    sim_window_loaddigtrace(INTBIG, INTBIG, double*, INTSML*);
void    sim_window_loadanatrace(INTBIG, INTBIG, double*, float*);
void    sim_window_settraceframe(INTBIG, INTBIG);
void    sim_window_killtrace(INTBIG);
void    sim_window_killalltraces(BOOLEAN);
INTBIG *sim_window_getbustraces(INTBIG);
INTBIG  sim_window_gettraceframe(INTBIG);
CHAR   *sim_window_gettracename(INTBIG);
INTBIG  sim_window_gettracedata(INTBIG);
float   sim_window_getanatracevalue(INTBIG, double);
CHAR   *sim_window_getbustracevalue(INTBIG tri, double time);
void    sim_window_getdigtracevalue(INTBIG tri, double time, INTBIG *level, INTBIG *strength);
INTBIG *sim_window_findtrace(CHAR*, INTBIG*);
void    sim_window_inittraceloop(void);
void    sim_window_inittraceloop2(void);
INTBIG  sim_window_nexttraceloop(void);
INTBIG  sim_window_nexttraceloop2(void);
void    sim_window_cleartracehighlight(void);
void    sim_window_addhighlighttrace(INTBIG);
void    sim_window_deletehighlighttrace(INTBIG);
INTBIG  sim_window_gethighlighttrace(void);
INTBIG *sim_window_gethighlighttraces(void);
void    sim_window_showhighlightedtraces(void);
void    sim_window_setmaincursor(double);
double  sim_window_getmaincursor(void);
void    sim_window_setextensioncursor(double);
double  sim_window_getextensioncursor(void);
void    sim_window_settimerange(INTBIG, double, double);
void    sim_window_gettimerange(INTBIG, double*, double*);
void    sim_window_getaveragetimerange(double *avgmintime, double *avgmaxtime);
void    sim_window_gettimeextents(double*, double*);
void    sim_window_updatelayoutwindow(void);
CHAR   *sim_windowconvertengineeringnotation(double);
void    sim_window_setstate(INTBIG);
INTBIG  sim_window_getwidevalue(INTBIG **bits);
void    sim_window_auto_anarange(void);
void    sim_window_zoom_frame(INTBIG frameno);
void    sim_window_zoomout_frame(INTBIG frameno);
void    sim_window_shiftup_frame(INTBIG frameno);
void    sim_window_shiftdown_frame(INTBIG frameno);
void    sim_window_grabcachedsignalsoncell(NODEPROTO *cell);
INTBIG  sim_window_getcachedsignals(CHAR ***strings);
void    sim_window_supresstraceprefix(CHAR *prefix);
void    sim_window_renumberlines(void);
void    sim_window_advancetime(void);

/********************* for ESIM *********************/

#define ESIMNAME    x_("esim")

/********************* for RSIM *********************/

#define RSIMPRENAME x_("presim")
#define RSIMNAME    x_("rsim")
#define RSIMIN      x_("rsim.in")			/* name of file with binary network */

/********************* for ALS *********************/

#define NET_EXT		x_(".net")
#define PLOT_EXT	x_(".hpgl")

/********************* for RNL *********************/

#define RNLPRENAME  x_("presim")
#define RNLNAME     x_("rnl")
#define RNLIN       x_("rnl.in")			/* name of file with binary network */
#define RNLCOMM     x_("nl.l")				/* file with simulator initialization */

/********************* for VERILOG *********************/

/* Meaning of bits in "SIM_verilog_state" */
#define VERILOGUSEASSIGN  01			/* set to use "assign" construct */
#define VERILOGUSETRIREG  02			/* set to use "trireg" wire by default */

extern INTBIG sim_verilog_statekey;		/* key for "SIM_verilog_state" */

void        sim_verlevel_up(NODEPROTO *cell);
void        sim_verlevel_set(CHAR *instname, NODEPROTO *cell);
void        sim_verparsefile(CHAR*, NODEPROTO *cell);
BOOLEAN     sim_vertopofinstances(CHAR **c);
CHAR       *sim_vernextinstance(void);
CHAR       *sim_verlevel_cur(void);
void        sim_verreportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
				void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*));
void        sim_veraddhighlightednet(CHAR *signame);

/********************* for SPICE *********************/

#define MAXSPICELEVEL     3				/* levels 1, 2, and 3 */

/* Meaning of bits in sim_spice_state */
#define SPICERESISTANCE         01		/* set for resistances */
#define SPICEPLOT               02		/* set for plots (vs prints) */
#define SPICENODENAMES         010		/* set for node names (vs numbers) */
#define SPICETYPE             0160		/* mask for spice type */
#define SPICE2                 000		/*   spice 2 */
#define SPICE3                 020		/*   spice 3 */
#define SPICEHSPICE            040		/*   hspice */
#define SPICEPSPICE            060		/*   pspice */
#define SPICEGNUCAP           0100      /*   gnucap */
#define SPICESMARTSPICE       0120      /*   smartspice */
#define SPICEGLOBALPG         0200		/* set to use global power and ground */
#define SPICECELLPARAM       01000		/* set to use cell parameters */
#define SPICEUSELAMBDAS      02000		/* set to use lambdas instead of microns for transistor sizes */
#define SPICEOUTPUT         034000		/* mask for output format */
#define SPICEOUTPUTNORM          0		/*   normal */
#define SPICEOUTPUTRAW       04000		/*   raw */
#define SPICEOUTPUTRAWSMART 010000		/*   raw for smartspice */

extern INTBIG sim_spice_levelkey;		/* key for "SIM_spice_level" */
extern INTBIG sim_spice_statekey;		/* key for "SIM_spice_state" */
extern INTBIG sim_spice_state;			/* value of "SIM_spice_state" */
extern INTBIG sim_spice_nameuniqueid;	/* key for "SIM_spice_nameuniqueid" */
extern INTBIG sim_spice_listingfilekey;	/* key for "SIM_listingfile" */
extern INTBIG sim_spice_runargskey;		/* key for "SIM_spice_runarguments" */

void sim_spicereportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
	void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*));
void     sim_spice_addhighlightednet(CHAR *name, BOOLEAN);

/********************* for FASTHENRY *********************/

/* the meaning of "tool:sim.SIM_fasthenry_state" */
#define FHUSESINGLEFREQ       01			/* use single frequency */
#define FHMAKEMULTIPOLECKT    02			/* make multipole circuit */
#define FHMAKEPOSTSCRIPTVIEW  04			/* make PostScript view */
#define FHMAKESPICESUBCKT    010			/* make SPICE subcircuit */
#define FHEXECUTETYPE       0160			/* what to do after writing the deck */
#define FHEXECUTENONE          0			/*    no simulator run */
#define FHEXECUTERUNFH       020			/*    run fasthenry */
#define FHEXECUTERUNFHMUL    040			/*    run fasthenry for every group */

extern INTBIG sim_fasthenrystatekey;		/* variable key for "SIM_fasthenry_state" */
extern INTBIG sim_fasthenryfreqstartkey;	/* variable key for "SIM_fasthenry_freqstart" */
extern INTBIG sim_fasthenryfreqendkey;		/* variable key for "SIM_fasthenry_freqend" */
extern INTBIG sim_fasthenryrunsperdecadekey;/* variable key for "SIM_fasthenry_runsperdecade" */
extern INTBIG sim_fasthenrynumpoleskey;		/* variable key for "SIM_fasthenry_numpoles" */
extern INTBIG sim_fasthenryseglimitkey;		/* variable key for "SIM_fasthenry_seglimit" */
extern INTBIG sim_fasthenrythicknesskey;	/* variable key for "SIM_fasthenry_thickness" */
extern INTBIG sim_fasthenrywidthsubdivkey;	/* variable key for "SIM_fasthenry_width_subdivs" */
extern INTBIG sim_fasthenryheightsubdivkey;	/* variable key for "SIM_fasthenry_height_subdivs" */
extern INTBIG sim_fasthenryzheadkey;		/* variable key for "SIM_fasthenry_z_head" */
extern INTBIG sim_fasthenryztailkey;		/* variable key for "SIM_fasthenry_z_tail" */
extern INTBIG sim_fasthenrygroupnamekey;	/* variable key for "SIM_fasthenry_group_name" */

/* prototypes for tool interface */
void        sim_init(INTBIG*, CHAR1*[], TOOL*);
void        sim_done(void);
void        sim_set(INTBIG, CHAR*[]);
INTBIG      sim_request(CHAR *command, va_list ap);
void        sim_slice(void);
void        sim_startbatch(TOOL*, BOOLEAN);
void        sim_modifynodeinst(NODEINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void        sim_modifyarcinst(ARCINST*,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG,INTBIG);
void        sim_modifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
void        sim_newobject(INTBIG, INTBIG);
void        sim_killobject(INTBIG, INTBIG);
void        sim_newvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void        sim_killvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
void        sim_modifyvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void        sim_insertvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void        sim_deletevariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
void        sim_readlibrary(LIBRARY*);

/* prototypes for intratool interface */
void        sim_freewindowmemory(void);
void        sim_freespicememory(void);
void        sim_freespicerun_memory(void);
void        sim_freeirsimmemory(void);
void        sim_freemaxwellmemory(void);
void        sim_freeverilogmemory(void);
void        sim_spice_xprintf(FILE*, BOOLEAN, CHAR*, ...);
NETWORK    *sim_spice_networkfromname(CHAR *name);
CHAR       *sim_spice_signalname(NETWORK *net);
void        sim_simpointout(CHAR*, INTBIG);
void        sim_spice_execute(CHAR*, CHAR*, NODEPROTO*);
void        sim_writefasthenrynetlist(NODEPROTO*);
void        sim_fasthenryinit(void);
void        sim_writesim(NODEPROTO*, INTBIG);
void        sim_writeirsim(NODEPROTO*);
void        sim_writepalnetlist(NODEPROTO*);
void        sim_writemaxwell(NODEPROTO*);
void        sim_writemossim(NODEPROTO*);
void        sim_writetexnetlist(NODEPROTO*);
void        sim_writespice(NODEPROTO*, BOOLEAN);
void        sim_writevernetlist(NODEPROTO*);
void        sim_writesilnetlist(NODEPROTO*);
void        sim_resumesim(BOOLEAN);
INTBIG      sim_alsclockdlog(CHAR *paramstart[]);
void        sim_initbussignals(void);
void        sim_addbussignal(INTBIG signal);
INTBIG      sim_getbussignals(INTBIG **signallist);
void        sim_reportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
				void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*));
void        sim_addsignal(WINDOWPART *simwin, CHAR *sig, BOOLEAN overlay);
CHAR       *sim_signalseparator(void);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif

#if defined(__cplusplus)

class SpiceCell;
class SpiceInst;
class SpiceBind;
class SpiceParam;
class SpiceNet;

class SpiceCell
{
	friend class SpiceInst;
	friend class SpiceNet;
public:
	SpiceCell( CHAR *name = "" );
	~SpiceCell();
	CHAR *name() { return _name; }
	static SpiceCell *findByName( CHAR *name );
	void setup();
	static void traverseAll();
	static void clearAll();
	void printSpice();
private:
	void traverse();
	CHAR *_name;
	static SpiceCell *_firstCell;
	SpiceCell *_nextCell;
	SpiceInst *_firstInst, *_lastInst;
	SpiceNet *_firstNet;
	INTBIG _pathlen;
	INTBIG _totalnets;
	static CHAR *_path;
	static CHAR *_pathend;
	static CHAR *_rpath;
	static CHAR *_rpathbeg;
};

class SpiceInst
{
	friend class SpiceCell;
	friend class SpiceBind;
public:
    SpiceInst( SpiceCell *parentCell, CHAR *name );
	~SpiceInst();
	CHAR *name() { return _name; }
	void setInstCell( SpiceCell *instCell );
	void addParam( SpiceParam *param );
	void addParamM( NODEINST *ni );
	void addError( CHAR *error );
private:
	void addBind( SpiceBind *bind );
	void printSpice();
	void traverse();
	CHAR *_name;
	SpiceCell *_parentCell;
	SpiceCell *_instCell;
	SpiceBind *_firstBind, *_lastBind;
	SpiceParam *_firstParam, *_lastParam;
	CHAR *_error;
	SpiceInst *_nextInst;
};

class SpiceBind
{
	friend class SpiceInst;
public:
	SpiceBind( SpiceInst *inst, SpiceNet *net = 0 );
	~SpiceBind();
private:
	void printSpice();
	SpiceInst *_inst;
	SpiceNet *_net;
	SpiceBind *_nextBind;
};

class SpiceParam
{
	friend class SpiceInst;
public:
	SpiceParam( CHAR *str );
	~SpiceParam();
	CHAR *str() { return _str; }
private:
	CHAR *_str;
	SpiceParam *_nextParam;
};

class SpiceNet
{
	friend class SpiceCell;
	friend class SpiceBind;
 public:
	SpiceNet( SpiceCell *parentCell, INTBIG netnumber, NETWORK *netw = 0 );
	~SpiceNet();
private:
	void printSpice();
	INTBIG _netnumber;
	NETWORK *_netw;
	SpiceCell *_parentCell;
	SpiceNet *_nextNet;
};

extern CHAR **sim_spice_printlist;
extern INTBIG sim_spice_printlistlen;

#endif
