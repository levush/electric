/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sim.cpp
 * Simulation tool: main module
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
#include "sim.h"
#include "simals.h"
#include "simirsim.h"
#include "tecgen.h"
#include "tecschem.h"
#include "usr.h"

/* prototypes for local routines */
static void    sim_checktostopsimulation(NODEPROTO *np);
static void    sim_writephases(float[], INTBIG[], INTBIG, void*);
static void    sim_optionsdlog(void);
static void    sim_spicedlog(void);
static void    sim_verilogdlog(void);
static void    sim_verdiasetcellinfo(LIBRARY *curlib, void *dia);
static BOOLEAN sim_topofcells(CHAR **c);
static CHAR   *sim_nextcells(void);
static void    sim_optionssetcolorindia(void *dia, INTBIG entry, INTBIG signal, char **newlang);
static void    sim_optionsgetcolorfromdia(void *dia, INTBIG entry, INTBIG signal);

#ifndef ALLCPLUSPLUS
extern "C"
{
#endif

/************************** ALS COMMANDS **************************/

/* ALS's build-actel-models command */
static COMCOMP qbuildp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	0, x_(" \t"), M_("Actel table file to convert to library"), 0};

/* ALS's clock command */
static INTBIG sim_nextcustomclock(CHAR*, COMCOMP*[], CHAR);
static COMCOMP clockcdurp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, sim_nextcustomclock,
	0, x_(" \t"), M_("clock: duration of this phase"), 0};
static COMCOMP clockclevp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("clock: level for this phase"), 0};
static INTBIG sim_nextcustomclock(CHAR *i, COMCOMP *j[], CHAR c)
{ Q_UNUSED( i ); Q_UNUSED( c); j[0] = &clockclevp; j[1] = &clockcdurp; return(2); }
static COMCOMP clockccyp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, sim_nextcustomclock,
	0, x_(" \t"), M_("clock: number of cycles (0 for infinite)"), 0};
static COMCOMP clockcstrp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("clock: strength"), 0};
static COMCOMP clockclp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("clock: random distribution"), 0};
static COMCOMP clockfp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("50/50 duty cycle frequency"), 0};
static COMCOMP clockpp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("50/50 duty cycle period"), 0};
static KEYWORD clockopt[] =
{
	{x_("frequency"),     1,{&clockfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("period"),        1,{&clockpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("custom"),        3,{&clockclp,&clockcstrp,&clockccyp,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP clockop = {clockopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("clock options"), 0};
static COMCOMP clocknp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("node name"), 0};

/* ALS's go command */
static COMCOMP gop = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("maximum simulation time (in seconds)"), 0};

/* ALS's print command */
static COMCOMP printsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("node to print"), 0};
static KEYWORD printopt[] =
{
	{x_("display"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("instances"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("netlist"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("size"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("state"),          1,{&printsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vector"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("xref"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP printp = {printopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("printing options"), 0};

/* ALS's seed command */
static KEYWORD seedopt[] =
{
	{x_("reset"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-reset"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP seedp = {seedopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("random number seed setting"), 0};

/* ALS's set command */
static COMCOMP settp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("time of node set (in seconds)"), 0};
static COMCOMP setsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("strength of node set"), 0};
static KEYWORD setlopt[] =
{
	{x_("H"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("L"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("X"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP setlp = {setlopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("state level"), 0};
static COMCOMP setnp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("node name to set"), 0};

/* ALS's trace command */
static KEYWORD traceopt[] =
{
	{x_("on"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP tracep = {traceopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("trace state"), 0};

/* ALS's vector command */
static COMCOMP vectordtp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("time at which to delete"), 0};
static KEYWORD vectordopt[] =
{
	{x_("time"),       1,{&vectordtp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("all"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP vectordop = {vectordopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("vector deletion option"), 0};
static COMCOMP vectordp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("node name to delete"), 0};
static COMCOMP vectorlp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	0, x_(" \t"), M_("file name with vectors"), 0};
static COMCOMP vectorsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("file name to save vectors"), 0};
static KEYWORD vectoropt[] =
{
	{x_("delete"),      2,{&vectordp,&vectordop,NOKEY,NOKEY,NOKEY}},
	{x_("load"),        1,{&vectorlp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("save"),        1,{&vectorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("new"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP vectorp = {vectoropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("vector options"), 0};
static KEYWORD annotateopt[] =
{
	{x_("minimum"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("typical"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("maximum"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP annotatep = {annotateopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("annotate options"), 0};
static COMCOMP orderrp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("ordered list of trace names"), 0};
static KEYWORD orderopt[] =
{
	{x_("save"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("restore"),    1,{&orderrp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP orderp = {orderopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("order options"), 0};

/* ALS's command table */
static KEYWORD sim_alsopt[] =
{
	{x_("annotate"),           1,{&annotatep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("build-actel-models"), 1,{&qbuildp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clock"),              2,{&clocknp,&clockop,NOKEY,NOKEY,NOKEY}},
	{x_("erase"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("go"),                 1,{&gop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("help"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("order"),              1,{&orderp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("print"),              1,{&printp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("seed"),               1,{&seedp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set"),                4,{&setnp,&setlp,&setsp,&settp,NOKEY}},
	{x_("trace"),              1,{&tracep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vector"),             1,{&vectorp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_alsp = {sim_alsopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Simulation command"), 0};

/************************** SIMULATION WINDOW COMMANDS **************************/

static COMCOMP cursorgp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("time at the extension cursor"), 0};
static COMCOMP cursorwp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("time at the main cursor"), 0};
static KEYWORD cursoropt[] =
{
	{x_("extension"),    1,{&cursorgp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("main"),         1,{&cursorwp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("center"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP cursorp = {cursoropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("cursor color"), 0};

static COMCOMP moveap = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("distance to move (in seconds)"), 0};
static COMCOMP movesp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("distance to move (in signals)"), 0};
static KEYWORD moveopt[] =
{
	{x_("left"),         1,{&moveap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right"),        1,{&moveap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("up"),           1,{&movesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("down"),         1,{&movesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP movep = {moveopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("window move option"), 0};

static COMCOMP zoomup = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("upper time of zoom"), 0};
static COMCOMP zoomlp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("lower time of zoom"), 0};
static COMCOMP zoomap = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("amount to zoom"), 0};
static KEYWORD zoomopt[] =
{
	{x_("window"),        2,{&zoomlp,&zoomup,NOKEY,NOKEY,NOKEY}},
	{x_("in"),            1,{&zoomap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("out"),           1,{&zoomap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cursor"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("all-displayed"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP zoomp = {zoomopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("window zoom options"), 0};
static COMCOMP tracessp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("number of traces to show on the display"), 0};
static KEYWORD tracesopt[] =
{
	{x_("more"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("less"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set"),       1,{&tracessp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP tracesp = {tracesopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("window traces options"), 0};
static KEYWORD dispcolorsopt[] =
{
	{x_("white"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("black"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("red"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("blue"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("green"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cyan"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("magenta"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("yellow"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gray"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("orange"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("purple"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("brown"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-gray"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-gray"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-red"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-red"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-green"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-green"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("light-blue"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("dark-blue"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP dispcolorsp = {dispcolorsopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("color for this strength/level"), 0};
static KEYWORD dispcoloropt[] =
{
	{x_("off"),       1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("node"),      1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gate"),      1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power"),     1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("low"),       1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("high"),      1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("undefined"), 1,{&dispcolorsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP dispcolorp = {dispcoloropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("window strength color options"), 0};
static COMCOMP vectorfp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	0, x_(" \t"), M_("Vector file"), 0};
static KEYWORD sim_windowopt[] =
{
	{x_("cursor"),              1,{&cursorp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("move"),                1,{&movep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("traces"),              1,{&tracesp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("zoom"),                1,{&zoomp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("color"),               1,{&dispcolorp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("2-state-display"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("12-state-display"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("advance-time"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("freeze-time"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("display-waveform"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ignore-waveform"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("clear-saved-signals"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vector-clear"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vector-load"),         1,{&vectorfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vector-save"),         1,{&vectorfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vector-writespice"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_windowp = {sim_windowopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Simulation window command"), 0};

/************************** SPICE COMMANDS **************************/

static KEYWORD sim_spicelevelopt[] =
{
	{x_("1"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("2"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("3"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_spicelevelp = {sim_spicelevelopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Spice simulation level"), M_("show current")};
static KEYWORD sim_spiceformopt[] =
{
	{x_("2"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("3"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("hspice"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pspice"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("gnucap"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("smartspice"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP spiceformp = {sim_spiceformopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Spice format"), 0};
static KEYWORD sim_spicenotopt[] =
{
	{x_("resistance"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("capacitances"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("plot"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("global-pwr-gnd"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-nodenames"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_spicenotp = {sim_spicenotopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Spice simulation NOT option"), 0};
static COMCOMP sim_spicereadp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("File containing spice output"), 0};
static COMCOMP sim_spicesavep = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("File in which to save spice output"), 0};
static KEYWORD sim_spiceopt[] =
{
	{x_("level"),                1,{&sim_spicelevelp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("save-output"),          1,{&sim_spicesavep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("format"),               1,{&spiceformp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("parse-output"),         1,{&sim_spicereadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-spice-this-cell"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("not"),                  1,{&sim_spicenotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("resistance"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("capacitances"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("plot"),                 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output-normal"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output-raw"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output-smartraw"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("global-pwr-gnd"),       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-nodenames"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_spicep = {sim_spiceopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Spice simulation option"), 0};

/************************** VERILOG COMMANDS **************************/

static KEYWORD sim_vernotopt[] =
{
	{x_("use-assign"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-trireg"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_vernotp = {sim_vernotopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Verilog simulation NOT option"), 0};
static COMCOMP sim_verreadp = {NOKEYWORD, topoffile, nextfile, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("File containing Verilog VCD dump output"), 0};
static KEYWORD sim_veropt[] =
{
	{x_("not"),                     1,{&sim_vernotp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("use-assign"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-trireg"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("parse-output"),            1,{&sim_verreadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-verilog-this-cell"),  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sim_verp = {sim_veropt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Verilog simulation option"), 0};

/************************** FASTHENRY COMMANDS **************************/

static COMCOMP simfhaaddp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc group name"), 0};
static COMCOMP simfhathickp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc thickness"), 0};
static COMCOMP simfhawidsubp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc width subdivisions"), 0};
static COMCOMP simfhaheisubp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc height subdivisions"), 0};
static COMCOMP simfhazheadp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc head Z"), 0};
static COMCOMP simfhaztailp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc tail Z"), 0};
static KEYWORD sim_fhaopt[] =
{
	{x_("add"),                 1,{&simfhaaddp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("remove"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("thickness"),           1,{&simfhathickp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("width-subdivisions"),  1,{&simfhawidsubp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("height-subdivisions"), 1,{&simfhaheisubp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("z-head"),              1,{&simfhazheadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("z-tail"),              1,{&simfhaztailp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP sim_fhap = {sim_fhaopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry arc option"), 0};
static COMCOMP simfhpolemp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Number of poles"), 0};
static KEYWORD simfhpoleopt[] =
{
	{x_("single"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("multiple"),         1,{&simfhpolemp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP simfhpolep = {simfhpoleopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry multipole option"), 0};
static COMCOMP simfhfreqsp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Starting frequency"), 0};
static COMCOMP simfhfreqep = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Ending frequency"), 0};
static COMCOMP simfhfreqrp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Runs per decade"), 0};
static KEYWORD simfhfreqopt[] =
{
	{x_("single"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("multiple"),         3,{&simfhfreqsp,&simfhfreqep,&simfhfreqrp,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP simfhfreqp = {simfhfreqopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry frequency option"), 0};
static COMCOMP simfhthickp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry default thickness"), 0};
static COMCOMP simfhwidthp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry default width subdivisions"), 0};
static COMCOMP simfhheightp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry default height subdivisions"), 0};
static COMCOMP simfhseglenp = {NOKEYWORD, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry maximum segment length"), 0};
static KEYWORD simfhexeopt[] =
{
	{x_("none"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("run"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("multiple-run"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP simfhexep = {simfhexeopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry execution option"), 0};
static KEYWORD simfhpsopt[] =
{
	{x_("on"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP simfhpsp = {simfhpsopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry PostScript option"), 0};
static KEYWORD simfhspiceopt[] =
{
	{x_("on"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP simfhspicep = {simfhspiceopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry Spice option"), 0};
static KEYWORD sim_fhopt[] =
{
	{x_("arc"),                 1,{&sim_fhap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pole"),                1,{&simfhpolep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("frequency"),           1,{&simfhfreqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("thickness"),           1,{&simfhthickp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("width-subdivisions"),  1,{&simfhwidthp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("height-subdivisions"), 1,{&simfhheightp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("max-segment-length"),  1,{&simfhseglenp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("execute"),             1,{&simfhexep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("postscript"),          1,{&simfhpsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("spice"),               1,{&simfhspicep,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP sim_fhp = {sim_fhopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("FastHenry simulation option"), 0};

/************************** SIMULATOR COMMANDS **************************/

static KEYWORD simulatorsimulateopt[] =
{
	{x_("esim"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rsim"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rnl"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cosmos"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fasthenry"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("spice"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cspice"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("mossim"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("texsim"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("als"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("verilog"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("abel-pal"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("silos"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("internal"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP simsimulatep = {simulatorsimulateopt, NOTOPLIST, NONEXTLIST,
	NOPARAMS, INPUTOPT, x_(" \t"), M_("Netlist format to generate"), 0};
static COMCOMP simulatornetp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("net or component to point out"), 0};
static KEYWORD simulatoropt[] =
{
	{x_("spice"),                    1,{&sim_spicep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pointout"),                 1,{&simulatornetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("als"),                      1,{&sim_alsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("verilog"),                  1,{&sim_verp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fasthenry"),                1,{&sim_fhp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-execute"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("execute-only"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("execute-quietly"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("execute-and-parse"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("execute-quietly-and-parse"),0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("resume"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("simulate"),                 1,{&simsimulatep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("window"),                   1,{&sim_windowp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP sim_simulatorp = {simulatoropt,NOTOPLIST,NONEXTLIST,NOPARAMS,
		0, x_(" \t"), M_("Simulator action"), M_("show current simulator")};

extern COMCOMP us_colorreadp;

#ifndef ALLCPLUSPLUS
}
#endif

static struct
{
	CHAR *simname;
	INTBIG min;
	INTBIG format;
} sim_simtable[] =
{
	{x_("abel-pal"),  1, ABEL},
	{x_("cosmos"),	  1, COSMOS},
	{x_("esim"),      1, ESIM},
	{x_("fasthenry"), 1, FASTHENRY},
	{x_("rnl"),       2, RNL},
	{x_("rsim"),      2, RSIM},
	{x_("irsim"),     1, IRSIM},
	{x_("maxwell"),   2, MAXWELL},
	{x_("mossim"),    2, MOSSIM},
	{x_("als"),       1, ALS},
	{x_("texsim"),    1, TEXSIM},
	{x_("silos"),     2, SILOS},
	{x_("spice"),     2, SPICE},
	{x_("cdl"),       2, CDL},
	{x_("verilog"),   2, VERILOG},
	{NULL, 0, 0}  /* 0 */
};

       TOOL      *sim_tool;					/* the Simulator tool object */
       INTBIG     sim_formatkey;			/* key for "SIM_format" (ESIM, etc) */
       INTBIG     sim_netfilekey;			/* key for "SIM_netfile" */
       INTBIG     sim_dontrunkey;			/* key for "SIM_dontrun" */
       INTBIG     sim_weaknodekey;			/* key for "SIM_weak_node" */
       INTBIG     sim_spice_partskey;		/* key for "SIM_spice_parts" */
       CHAR      *sim_spice_parts = 0;		/* cached value for "SIM_spice_parts" */
       NODEPROTO *sim_simnt;				/* cell being simulated */
       EProcess  *sim_process = 0;          /* process of simulator */
static BOOLEAN    sim_circuitchanged;		/* true if circuit being simulated was changed */
static BOOLEAN    sim_undoredochange;		/* true if change comes from undo/redo */
static ARCINST   *sim_modifyarc;			/* arc whose userbits are being changed */
static INTBIG     sim_modifyarcbits;		/* former value of userbits on changed arc */
static LIBRARY   *sim_curlib;				/* for listing cells alphabetically in dialogs */
static NODEPROTO *sim_oldcellprotos;		/* for listing cells alphabetically in dialogs */
static INTBIG     sim_bussignalcount;		/* count of signals in a bus */
static INTBIG     sim_bussignaltotal = 0;	/* total size of bus signal array */
static INTBIG    *sim_bussignals;			/* bus signal array */
       INTBIG     sim_filetypeesim;			/* ESIM netlist file descriptor */
       INTBIG     sim_filetypefasthenry;	/* FastHenry netlist file descriptor */
       INTBIG     sim_filetypemossim;		/* MOSSIM netlist file descriptor */
       INTBIG     sim_filetypepal;			/* PAL netlist file descriptor */
       INTBIG     sim_filetypeals;			/* ALS netlist file descriptor */
       INTBIG     sim_filetypealsvec;		/* ALS vectors file descriptor */
       INTBIG     sim_filetypeirsimcmd;		/* IRSIM command (vectors) file descriptor */
       INTBIG     sim_filetypenetlisp;		/* Netlisp netlist file descriptor */
       INTBIG     sim_filetypequisc;		/* QUISC netlist file descriptor */
       INTBIG     sim_filetypersim;			/* RSIM netlist file descriptor */
       INTBIG     sim_filetypeirsim;		/* IRSIM netlist file descriptor */
       INTBIG     sim_filetypeirsimparam;	/* IRSIM parameter file descriptor */
       INTBIG     sim_filetypesilos;		/* Silos netlist file descriptor */
       INTBIG     sim_filetypespice;		/* SPICE input file descriptor */
       INTBIG     sim_filetypespicecmd;		/* SPICE command file descriptor */
       INTBIG     sim_filetypespiceout;		/* SPICE output file descriptor */
       INTBIG     sim_filetypehspiceout;	/* HSPICE output file descriptor */
       INTBIG     sim_filetyperawspiceout;	/* SPICE raw output disk file descriptor */
       INTBIG     sim_filetypesrawspiceout;	/* SmartSPICE raw output disk file descriptor */
       INTBIG     sim_filetypecdl;			/* CDL output file descriptor */
       INTBIG     sim_filetypectemp;		/* CDL template disk file descriptor */
       INTBIG     sim_filetypemaxwell;		/* MAXWELL output file descriptor */
       INTBIG     sim_filetypetegas;		/* Tegas netlist file descriptor */
       INTBIG     sim_filetypetegastab;		/* Tegas table file descriptor */
       INTBIG     sim_filetypeverilog;		/* Verilog file descriptor */
       INTBIG     sim_filetypeverilogvcd;	/* Verilog VCD dump file descriptor */
static INTBIG     sim_window_curactive;		/* state of simulation during broadcast */

static VARMIRROR sim_variablemirror[] =
{
	{&sim_window_statekey,        &sim_window_state},
	{&sim_colorstrengthoff_key,   &sim_colorstrengthoff},
	{&sim_colorstrengthnode_key,  &sim_colorstrengthnode},
	{&sim_colorstrengthgate_key,  &sim_colorstrengthgate},
	{&sim_colorstrengthpower_key, &sim_colorstrengthpower},
	{&sim_colorlevellow_key,      &sim_colorlevellow},
	{&sim_colorlevelhigh_key,     &sim_colorlevelhigh},
	{&sim_colorlevelundef_key,    &sim_colorlevelundef},
	{&sim_colorlevelzdef_key,     &sim_colorlevelzdef},
	{0, 0}
};

void sim_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	Q_UNUSED( argc );
	Q_UNUSED( argv );

	/* nothing on pass 2 or 3 of initialization */
	if (thistool == NOTOOL || thistool == 0) return;

	/* pass 1 initialization */
	sim_tool = thistool;
	sim_dontrunkey = makekey(x_("SIM_dontrun"));
	sim_weaknodekey = makekey(x_("SIM_weak_node"));
	sim_netfilekey = makekey(x_("SIM_netfile"));

	/* create disk file descriptors */
	sim_filetypeesim         = setupfiletype(x_("sim"), x_("*.sim"),       MACFSTAG('TEXT'), FALSE, x_("esim"), _("ESIM netlist"));
	sim_filetypefasthenry    = setupfiletype(x_("inp"), x_("*.inp"),       MACFSTAG('TEXT'), FALSE, x_("fasthenry"), _("FastHenry netlist"));
	sim_filetypehspiceout    = setupfiletype(x_("tr0"), x_("*.tr0"),       MACFSTAG('TEXT'), TRUE,  x_("hspiceout"), _("HSPICE output"));
	sim_filetypemossim       = setupfiletype(x_("ntk"), x_("*.ntk"),       MACFSTAG('TEXT'), FALSE, x_("mossim"), _("MOSSIM netlist"));
	sim_filetypepal          = setupfiletype(x_("pal"), x_("*.pal"),       MACFSTAG('TEXT'), FALSE, x_("pal"), _("Abel PAL"));
	sim_filetypeals          = setupfiletype(x_("net"), x_("*.net"),       MACFSTAG('TEXT'), FALSE, x_("als"), _("ALS netlist"));
	sim_filetypealsvec       = setupfiletype(x_("vec"), x_("*.vec"),       MACFSTAG('TEXT'), FALSE, x_("alsvec"), _("ALS vector"));
	sim_filetypeirsimcmd     = setupfiletype(x_("cmd"), x_("*.cmd"),       MACFSTAG('TEXT'), FALSE, x_("irsimcmd"), _("IRSIM vector"));
	sim_filetypenetlisp      = setupfiletype(x_("net"), x_("*.net"),       MACFSTAG('TEXT'), FALSE, x_("netlisp"), _("NetLisp netlist"));
	sim_filetypequisc        = setupfiletype(x_("sci"), x_("*.sci"),       MACFSTAG('TEXT'), FALSE, x_("quisc"), _("QUISC netlist"));
	sim_filetypersim         = setupfiletype(x_("sim"), x_("*.sim"),       MACFSTAG('TEXT'), FALSE, x_("rsim"), _("RSIM netlist"));
	sim_filetypeirsim        = setupfiletype(x_("sim"), x_("*.sim"),       MACFSTAG('TEXT'), FALSE, x_("irsim"), _("IRSIM netlist"));
	sim_filetypeirsimparam   = setupfiletype(x_("prm"), x_("*.prm"),       MACFSTAG('TEXT'), FALSE, x_("irsimparam"), _("IRSIM parameters"));
	sim_filetypesilos        = setupfiletype(x_("sil"), x_("*.sil"),       MACFSTAG('TEXT'), FALSE, x_("silos"), _("SILOS netlist"));
	sim_filetypespice        = setupfiletype(x_("spi"), x_("*.spi;*.spo"), MACFSTAG('TEXT'), FALSE, x_("spice"), _("SPICE netlist"));
	sim_filetypespicecmd     = setupfiletype(x_("cmd.sp"),x_("*.cmd.sp"),  MACFSTAG('TEXT'), FALSE, x_("spicecmd"), _("SPICE command"));
	sim_filetypespiceout     = setupfiletype(x_("spo"), x_("*.spo"),       MACFSTAG('TEXT'), FALSE, x_("spiceout"), _("SPICE output"));
	sim_filetyperawspiceout  = setupfiletype(x_("raw"), x_("*.raw"),       MACFSTAG('TEXT'), FALSE, x_("spicerawout"), _("SPICE raw output"));
	sim_filetypesrawspiceout = setupfiletype(x_("raw"), x_("*.raw"),       MACFSTAG('TEXT'), TRUE,  x_("smartspicerawout"), _("SmartSPICE raw output"));
	sim_filetypecdl          = setupfiletype(x_("cdl"), x_("*.cdl"),       MACFSTAG('TEXT'), FALSE, x_("cspice"), _("CDL netlist"));
	sim_filetypectemp        = setupfiletype(x_("cdltemplate"), x_("*.cdltemplate"), MACFSTAG('TEXT'), FALSE, x_("cdltemplate"), _("CDL template"));
	sim_filetypemaxwell      = setupfiletype(x_("mac"), x_("*.mac"),       MACFSTAG('TEXT'), FALSE, x_("maxwell"), _("Maxwell netlist"));
	sim_filetypetegas        = setupfiletype(x_("tdl"), x_("*.tdl"),       MACFSTAG('TEXT'), FALSE, x_("tegas"), _("TEGAS netlist"));
	sim_filetypetegastab     = setupfiletype(x_(""),    x_("*.*"),         MACFSTAG('TEXT'), FALSE, x_("tegastab"), _("TEGAS table"));
	sim_filetypeverilog      = setupfiletype(x_("v"),   x_("*.v;*.ver"),   MACFSTAG('TEXT'), FALSE, x_("verilog"), _("Verilog"));
	sim_filetypeverilogvcd   = setupfiletype(x_("vcd"), x_("*.dump"),      MACFSTAG('TEXT'), FALSE, x_("verilogout"), _("Verilog VCD dump"));

	/* initialize default simulator format */
	changesquiet(TRUE);
	sim_formatkey = makekey(x_("SIM_format"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_formatkey, ALS, VINTEGER|VDONTSAVE);

	/* do not have the simulator automatically run */
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, SIMRUNNO, VINTEGER|VDONTSAVE);

	/* ESIM/RSIM/RNL initialization */
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_netfilekey, (INTBIG)x_(""), VSTRING|VDONTSAVE);

	/* IRSIM initializatio */
	sim_irsim_statekey = makekey(x_("SIM_irsim_state"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_irsim_statekey, DEFIRSIMSTATE,
		VINTEGER|VDONTSAVE);

	/* SPICE initialization */
	sim_spice_partskey = makekey(x_("SIM_spice_parts"));
	sim_spice_listingfilekey = makekey(x_("SIM_listingfile"));
	sim_spice_runargskey = makekey(x_("SIM_spice_runarguments"));
	sim_spice_levelkey = makekey(x_("SIM_spice_level"));
	sim_spice_statekey = makekey(x_("SIM_spice_state"));
	sim_spice_nameuniqueid = makekey(x_("SIM_spice_nameuniqueid"));
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_listingfilekey, (INTBIG)x_(""), VSTRING|VDONTSAVE);
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey, SPICERESISTANCE|SPICENODENAMES|SPICE3,
		VINTEGER|VDONTSAVE);
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_levelkey, 1, VINTEGER|VDONTSAVE);
	(void)allocstring(&sim_spice_parts, x_("spiceparts"), sim_tool->cluster);
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_partskey, (INTBIG)sim_spice_parts,
		VSTRING|VDONTSAVE);
	DiaDeclareHook(x_("spice"), &sim_spicep, sim_spicedlog);
	changesquiet(FALSE);

	/* VERILOG initialization */
	sim_verilog_statekey = makekey(x_("SIM_verilog_state"));
	DiaDeclareHook(x_("verilog"), &sim_verp, sim_verilogdlog);

	/* FastHenry initialization */
	sim_fasthenrystatekey = makekey(x_("SIM_fasthenry_state"));
	sim_fasthenryfreqstartkey = makekey(x_("SIM_fasthenry_freqstart"));
	sim_fasthenryfreqendkey = makekey(x_("SIM_fasthenry_freqend"));
	sim_fasthenryrunsperdecadekey = makekey(x_("SIM_fasthenry_runsperdecade"));
	sim_fasthenrynumpoleskey = makekey(x_("SIM_fasthenry_numpoles"));
	sim_fasthenryseglimitkey = makekey(x_("SIM_fasthenry_seglimit"));
	sim_fasthenrythicknesskey = makekey(x_("SIM_fasthenry_thickness"));
	sim_fasthenrywidthsubdivkey = makekey(x_("SIM_fasthenry_width_subdivs"));
	sim_fasthenryheightsubdivkey = makekey(x_("SIM_fasthenry_height_subdivs"));
	sim_fasthenryzheadkey = makekey(x_("SIM_fasthenry_z_head"));
	sim_fasthenryztailkey = makekey(x_("SIM_fasthenry_z_tail"));
	sim_fasthenrygroupnamekey = makekey(x_("SIM_fasthenry_group_name"));
	sim_fasthenryinit();

	/* initialize the simulation window system */
	sim_window_init();

	/* miscellaneous initialization */
	sim_process = 0;
	sim_simnt = NONODEPROTO;
	sim_circuitchanged = FALSE;
	sim_undoredochange = FALSE;
	DiaDeclareHook(x_("simopt"), &sim_simulatorp, sim_optionsdlog);
}

void sim_done(void)
{
	REGISTER VARIABLE *var;

	if (sim_process != 0)
	{
		sim_process->kill();
		delete sim_process;
		sim_process = 0;

		var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_netfilekey);
		if (var != NOVARIABLE)
			ttyputmsg(_("Simulation net list saved in '%s'"), (CHAR *)var->addr);
	}

#ifdef DEBUGMEMORY
	/* free all memory */
	if (sim_spice_parts != 0) efree((CHAR *)sim_spice_parts);
	if (sim_bussignaltotal > 0) efree((CHAR *)sim_bussignals);
	simals_term();
	sim_freewindowmemory();
	sim_freespicememory();
	sim_freespicerun_memory();
	sim_freeirsimmemory();
	sim_freemaxwellmemory();
	sim_freeverilogmemory();
#  if SIMTOOLIRSIM != 0
	irsim_freememory();
#  endif
#endif
}

void sim_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG i, j, l, newformat, amount, curtop, internal, tr, type;
	REGISTER BOOLEAN (*charhandlerschem)(WINDOWPART*, INTSML, INTBIG) = 0,
		(*charhandlerwave)(WINDOWPART*, INTSML, INTBIG) = 0;
	REGISTER BOOLEAN (*startsimulation)(NODEPROTO*) = 0;
	REGISTER INTBIG spicestate, verilogstate, signals, options, value,
		active, vis;
	float fvalue;
	REGISTER void *infstr;
	REGISTER CHAR *pp, *deflib;
	CHAR *truename, *spiceoutfile;
	FILE *io;
	REGISTER VARIABLE *var;
	REGISTER LIBRARY *lib;
	REGISTER ARCINST *ai;
	NODEPROTO *np, *simnp;
	float factor;
	double time, size, maintime, extensiontime, maxtime, mintime;

	if (count == 0)
	{
		ttyputusage(x_("telltool simulation OPTIONS"));
		return;
	}
	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("simulate"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			ttyputusage(x_("telltool simulation simulate FORMAT"));
			return;
		}

		/* get the simulator name */
		internal = 0;
		l = estrlen(pp = par[1]);
		if (namesame(pp, x_("internal")) == 0)
		{
			internal = 1;
			switch (sim_window_state&SIMENGINE)
			{
				case SIMENGINEALS:
					l = estrlen(pp = x_("als"));
					break;
				case SIMENGINEIRSIM:
					l = estrlen(pp = x_("irsim"));
#if SIMTOOLIRSIM == 0
					ttyputmsg(_("IRSIM is not built-in: just writing the deck"));
					internal = 0;
#endif
					break;
			}
		}
		for(i=0; sim_simtable[i].simname != 0; i++)
			if (namesamen(pp, sim_simtable[i].simname, l) == 0 && l >= sim_simtable[i].min) break;
		if (sim_simtable[i].simname == 0)
		{
			ttyputbadusage(x_("telltool simulation simulate"));
			return;
		}
		newformat = sim_simtable[i].format;

		/* see if there is already a simulation running */
		if (sim_process != 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_formatkey);
			if (var != NOVARIABLE && newformat == var->addr)
			{
				ttyputmsg(_("This simulator is already running"));
				return;
			}
			ttyputerr(_("Cannot switch simulators while one is running"));
			return;
		}

		/* make sure there is a cell to simulate */
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell to simulate"));
			return;
		}

		/* remember the currently running format */
		(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_formatkey, newformat, VINTEGER|VDONTSAVE);

		/* write the appropriate simulation file */
		switch (newformat)
		{
			case ABEL:
				sim_writepalnetlist(np);
				break;
			case FASTHENRY:
				sim_writefasthenrynetlist(np);
				break;
			case COSMOS:
			case ESIM:
			case RNL:
			case RSIM:
				sim_writesim(np, newformat);
				break;
			case MOSSIM:
				sim_writemossim(np);
				break;
			case MAXWELL:
				sim_writemaxwell(np);
				break;
			case SILOS:
				sim_writesilnetlist(np);
				break;
			case SPICE:
				sim_writespice(np, FALSE);
				break;
			case CDL:
				sim_writespice(np, TRUE);
				break;
			case TEXSIM:
				sim_writetexnetlist(np);
				break;
			case VERILOG:
				sim_writevernetlist(np);
				break;
			case IRSIM:
#if SIMTOOLIRSIM != 0
				if (internal != 0)
				{
					charhandlerschem = irsim_charhandlerschem;
					charhandlerwave = irsim_charhandlerwave;
					startsimulation = irsim_startsimulation;
				} else
#endif
				{
					sim_writeirsim(np);
				}
				break;
			case ALS:
				charhandlerschem = simals_charhandlerschem;
				charhandlerwave = simals_charhandlerwave;
				startsimulation = simals_startsimulation;
				internal = 1;
				break;
		}

		if (internal != 0)
		{
			active = sim_window_isactive(&simnp);
			if (active != 0 && simnp != np)
			{
				if ((active&SIMWINDOWWAVEFORM) != 0)
				{
					ttyputerr(_("Close all simulation windows before simulating another cell"));
					return;
				}
				sim_window_stopsimulation();
				active = 0;
			}

			if (active != 0)
			{
				/* display schematic window if it is not up */
				if ((active&SIMWINDOWSCHEMATIC) == 0)
				{
					(void)sim_window_create(0, simnp, 0, charhandlerschem, newformat);
					return;
				}

				/* display waveform window if it is not up */
				if ((active&SIMWINDOWWAVEFORM) == 0 && (sim_window_state&SHOWWAVEFORM) != 0)
				{
					(void)sim_window_create(0, simnp, charhandlerwave, 0, newformat);
					return;
				}
				ttyputmsg(_("Simulation is already running"));
				return;
			}

			/* do not simulate icons */
			if (np->cellview == el_iconview)
			{
				ttyputerr(_("Cannot simulate an icon"));
				return;
			}

			/* start simulation of the cell */
			if ((*startsimulation)(np)) return;
		}
		return;
	}

	if (namesamen(pp, x_("resume"), l) == 0 && l >= 1)
	{
		var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_formatkey);
		if (var == NOVARIABLE)
		{
			ttyputerr(_("No simulation is running"));
			return;
		}
		switch (var->addr)
		{
			case RNL:
			case RSIM:
			case ESIM:
				sim_resumesim(FALSE);
				break;
			default:
				ttyputerr(_("Cannot resume this simulator"));
				break;
		}
		return;
	}

	if (namesamen(pp, x_("no-execute"), l) == 0 && l >= 1)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, SIMRUNNO, VINTEGER);
		ttyputmsg(M_("Simulator will not be invoked (deck generation only)"));
		return;
	}
	if (namesamen(pp, x_("execute-only"), l) == 0 && l >= 9)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, SIMRUNYES, VINTEGER);
		ttyputmsg(M_("Simulator will be invoked"));
		return;
	}
	if (namesamen(pp, x_("execute-and-parse"), l) == 0 && l >= 9)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, SIMRUNYESPARSE, VINTEGER);
		ttyputmsg(M_("Simulator will be invoked, output parsed"));
		return;
	}
	if (namesamen(pp, x_("execute-quietly-and-parse"), l) == 0 && l >= 16)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, SIMRUNYESQPARSE, VINTEGER);
		ttyputmsg(M_("Simulator will be invoked quietly, output parsed"));
		return;
	}
	if (namesamen(pp, x_("execute-quietly"), l) == 0 && l >= 9)
	{
		(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, SIMRUNYESQ, VINTEGER);
		ttyputmsg(M_("Simulator will be invoked quietly, output parsed"));
		return;
	}

	if (namesamen(pp, x_("pointout"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool simulation pointout ELEMENT"));
			return;
		}
		var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_formatkey);
		if (var != NOVARIABLE) switch (var->addr)
		{
			case RNL:
			case RSIM:
			case ESIM:
			case COSMOS: 
				sim_simpointout(par[1], var->addr);
				return;
		}
		ttyputerr(M_("Current simulator cannot pointout netlist objects"));
		return;
	}

	if (namesamen(pp, x_("window"), l) == 0 && l >= 1)
	{
		if (count <= 1)
		{
			ttyputusage(x_("telltool simulation window COMMAND"));
			return;
		}
		l = estrlen(pp = par[1]);

		if (namesamen(pp, x_("vector-clear"), l) == 0)
		{
			switch (sim_window_state&SIMENGINECUR)
			{
				case SIMENGINECURALS:
					par[0] = x_("vector");
					par[1] = x_("new");
					simals_com_comp(2, par);
					break;
#if SIMTOOLIRSIM != 0
				case SIMENGINECURIRSIM:
					irsim_clearallvectors();
					break;
#endif
			}
			return;
		}
		if (namesamen(pp, x_("vector-load"), l) == 0)
		{
			switch (sim_window_state&SIMENGINECUR)
			{
				case SIMENGINECURALS:
					par[0] = x_("vector");
					par[1] = x_("load");
					simals_com_comp(count, par);
					break;
#if SIMTOOLIRSIM != 0
				case SIMENGINECURIRSIM:
					irsim_loadvectorfile((count>2) ? par[2] : 0);
					break;
#endif
			}
			return;
		}
		if (namesamen(pp, x_("vector-save"), l) == 0)
		{
			switch (sim_window_state&SIMENGINECUR)
			{
				case SIMENGINECURALS:
					par[0] = x_("vector");
					par[1] = x_("save");
					simals_com_comp(count, par);
					break;
#if SIMTOOLIRSIM != 0
				case SIMENGINECURIRSIM:
					irsim_savevectorfile((count>2) ? par[2] : 0);
					break;
#endif
			}
			return;
		}
		if (namesamen(pp, x_("vector-writespice"), l) == 0)
		{
			sim_window_writespicecmd();
			return;
		}
		if (namesamen(pp, x_("clear-saved-signals"), l) == 0 && l >= 2)
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, sim_window_signalorder_key) != NOVARIABLE)
						(void)delvalkey((INTBIG)np, VNODEPROTO, sim_window_signalorder_key);
				}
			}
			if (sim_window_isactive(&np) != 0)
			{
				switch (sim_window_state&SIMENGINECUR)
				{
					case SIMENGINECURALS:
						simals_level_set_command(0);
						break;
					case SIMENGINECURVERILOG:
						sim_verlevel_set(0, np);
						break;
#if SIMTOOLIRSIM != 0
					case SIMENGINECURIRSIM:
						irsim_level_set(0, np);
						break;
#endif
				}
			}
			ttyputmsg(_("Removed default waveform signals from all cells"));
			return;
		}
		if (namesamen(pp, x_("2-state-display"), l) == 0 && l >= 1)
		{
			sim_window_setstate(sim_window_state & ~FULLSTATE);
			ttyputverbose(M_("Simulation will show only 2 states in schematics window"));
			return;
		}
		if (namesamen(pp, x_("12-state-display"), l) == 0 && l >= 1)
		{
			sim_window_setstate(sim_window_state | FULLSTATE);
			ttyputverbose(M_("Simulation will show all 12 states in schematics window"));
			return;
		}
		if (namesamen(pp, x_("advance-time"), l) == 0 && l >= 1)
		{
			sim_window_setstate(sim_window_state | ADVANCETIME);
			ttyputverbose(M_("State changes will advance time to end of activity"));
			return;
		}
		if (namesamen(pp, x_("freeze-time"), l) == 0 && l >= 1)
		{
			sim_window_setstate(sim_window_state & ~ADVANCETIME);
			ttyputverbose(M_("State changes will not advance time"));
			return;
		}
		if (namesamen(pp, x_("display-waveform"), l) == 0 && l >= 1)
		{
			sim_window_setstate(sim_window_state | SHOWWAVEFORM);
			ttyputverbose(M_("Simulation will show waveform window"));
			return;
		}
		if (namesamen(pp, x_("ignore-waveform"), l) == 0 && l >= 1)
		{
			sim_window_setstate(sim_window_state & ~SHOWWAVEFORM);
			ttyputverbose(M_("Simulation will not show waveform window"));
			return;
		}
		if (namesamen(pp, x_("color"), l) == 0 && l >= 2)
		{
			if (count <= 2)
			{
				sim_window_setdisplaycolor(99, 0);
				return;
			}
			l = estrlen(pp = par[2]);
			if (count < 4)
			{
				ttyputusage(x_("telltool simulation window color SIGNAL COLORNAME"));
				return;
			}
			i = getecolor(par[3]);
			if (i < 0)
			{
				ttyputbadusage(x_("telltool simulation window color"));
				return;
			}
			if (namesamen(pp, x_("off"), l) == 0)
			{
				sim_window_setdisplaycolor(OFF_STRENGTH, i);
				return;
			}
			if (namesamen(pp, x_("node"), l) == 0)
			{
				sim_window_setdisplaycolor(NODE_STRENGTH, i);
				return;
			}
			if (namesamen(pp, x_("gate"), l) == 0)
			{
				sim_window_setdisplaycolor(GATE_STRENGTH, i);
				return;
			}
			if (namesamen(pp, x_("power"), l) == 0)
			{
				sim_window_setdisplaycolor(VDD_STRENGTH, i);
				return;
			}
			if (namesamen(pp, x_("low"), l) == 0)
			{
				sim_window_setdisplaycolor(LOGIC_LOW, i);
				return;
			}
			if (namesamen(pp, x_("high"), l) == 0)
			{
				sim_window_setdisplaycolor(LOGIC_HIGH, i);
				return;
			}
			if (namesamen(pp, x_("undefined"), l) == 0)
			{
				sim_window_setdisplaycolor(LOGIC_X, i);
				return;
			}
			ttyputbadusage(x_("telltool simulation window color"));
			return;
		}
		if (namesamen(pp, x_("cursor"), l) == 0 && l >= 2)
		{
			if (sim_window_isactive(&np) == 0)
			{
				ttyputerr(M_("No simulator active"));
				return;
			}
			if (count < 3)
			{
				ttyputusage(x_("telltool simulation window cursor COMMAND"));
				return;
			}
			if (namesamen(par[2], x_("center"), estrlen(par[2])) == 0)
			{
				/* center the cursors visibly */
				sim_window_getaveragetimerange(&mintime, &maxtime);
				size = maxtime - mintime;
				time = (mintime + maxtime) / 2.0;
				sim_window_setmaincursor(time - size/4.0);
				sim_window_setextensioncursor(time + size/4.0);
				return;
			}
			if (count < 4)
			{
				ttyputusage(x_("telltool simulation window cursor WHICH TIME"));
				return;
			}
			time = eatof(par[3]);
			if (time < 0.0)
			{
				ttyputerr(M_("Warning: time cannot be negative, set to 0 sec."));
				time = 0.0;
			}
			if (namesamen(par[2], x_("main"), estrlen(par[2])) == 0)
			{
				sim_window_setmaincursor(time);
				return;
			}
			if (namesamen(par[2], x_("extension"), estrlen(par[2])) == 0)
			{
				sim_window_setextensioncursor(time);
				return;
			}
			ttyputbadusage(x_("telltool simulation window cursor"));
			return;
		}
		if (namesamen(pp, x_("move"), l) == 0 && l >= 1)
		{
			if ((sim_window_isactive(&np)&SIMWINDOWWAVEFORM) == 0)
			{
				ttyputerr(M_("No simulator waveform window is active"));
				return;
			}
			if (count < 3)
			{
				ttyputusage(x_("telltool simulation window move (left | right | up | down) [AMOUNT]"));
				return;
			}

			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("left"), l) == 0 || namesamen(pp, x_("right"), l) == 0)
			{
				/* determine amount to move time */
				sim_window_getaveragetimerange(&mintime, &maxtime);
				size = maxtime - mintime;
				if (count < 4) time = size / 2.0f; else
				{
					time = eatof(par[3]);
				}
				if (time <= 0.0)
				{
					ttyputerr(M_("Window movement time must be greater than 0 seconds"));
					return;
				}

				sim_window_inittraceloop();
				for(j=0; ; j++)
				{
					tr = sim_window_nexttraceloop();
					if (tr == 0) break;
					sim_window_gettimerange(tr, &mintime, &maxtime);
					if (namesamen(pp, x_("left"), l) == 0)
					{
						mintime += time;
					} else
					{
						mintime -= time;
						if (mintime < 0.0) mintime = 0.0;
					}
					maxtime = mintime + size;
					sim_window_settimerange(tr, mintime, maxtime);
				}
				if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
					sim_window_redraw();
				return;
			}
			if (namesamen(pp, x_("up"), l) == 0 || namesamen(pp, x_("down"), l) == 0)
			{
				curtop = sim_window_gettopvisframe();
				vis = sim_window_getnumvisframes();
				signals = sim_window_getnumframes();
				if (count < 4) amount = vis-1; else
				{
					amount = myatoi(par[3]);
					if (amount <= 0)
					{
						ttyputerr(M_("Window movement time must be greater than 0 signals"));
						return;
					}
				}
				if (namesamen(pp, x_("up"), l) == 0)
				{
					curtop += amount;
					if (curtop + vis > signals) curtop = signals - vis;
				} else
				{
					curtop -= amount;
					if (curtop < 0) curtop = 0;
				}
				sim_window_settopvisframe(curtop);
				sim_window_redraw();
				return;
			}

			ttyputbadusage(x_("telltool simulation window move"));
			return;
		}
		if (namesamen(pp, x_("traces"), l) == 0 && l >= 1)
		{
			if ((sim_window_isactive(&np)&SIMWINDOWWAVEFORM) == 0)
			{
				ttyputerr(M_("No simulator waveform window is active"));
				return;
			}
			if (count < 3)
			{
				ttyputusage(x_("telltool simulation window traces (more | less | set A)"));
				return;
			}
			l = estrlen(pp = par[2]);

			if (namesamen(pp, x_("more"), l) == 0)
			{
				sim_window_setnumframes(sim_window_getnumframes()+1);
				sim_window_redraw();
				return;
			}
			if (namesamen(pp, x_("less"), l) == 0)
			{
				i = sim_window_getnumframes();
				if (i <= 1)
				{
					ttyputerr(M_("Must be at least 1 signal on the display"));
					return;
				}
				sim_window_setnumframes(i+1);
				sim_window_redraw();
				return;
			}
			if (namesamen(pp, x_("set"), l) == 0)
			{
				if (count < 4)
				{
					ttyputusage(x_("telltool simulation window traces set AMOUNT"));
					return;
				}
				i = myatoi(par[3]);
				if (i < 1)
				{
					ttyputerr(M_("Must be at least 1 signal on the display"));
					return;
				}
				sim_window_setnumframes(i);
				sim_window_redraw();
				return;
			}
			ttyputbadusage(x_("telltool simulation window traces"));
			return;
		}
		if (namesamen(pp, x_("zoom"), l) == 0 && l >= 1)
		{
			if ((sim_window_isactive(&np)&SIMWINDOWWAVEFORM) == 0)
			{
				ttyputerr(M_("No simulator waveform window is active"));
				return;
			}
			if (count < 3)
			{
				ttyputusage(x_("telltool simulation window zoom (in | out | window | cursor)"));
				return;
			}
			l = estrlen(pp = par[2]);

			if (namesamen(pp, x_("cursor"), l) == 0)
			{
				maintime = sim_window_getmaincursor();
				extensiontime = sim_window_getextensioncursor();
				if (maintime == extensiontime) return;
				if (maintime > extensiontime)
				{
					size = (maintime-extensiontime) / 20.0;
					maxtime = maintime + size;
					mintime = extensiontime - size;
				} else
				{
					size = (extensiontime-maintime) / 20.0;
					maxtime = extensiontime + size;
					mintime = maintime - size;
				}
				sim_window_settimerange(0, mintime, maxtime);
				if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
					sim_window_redraw();
				return;
			}

			if (namesamen(pp, x_("in"), l) == 0)
			{
				if (count < 4) factor = 2.0; else
				{
					factor = (float)myatoi(par[3]);
					if (factor <= 1)
					{
						ttyputerr(M_("Zoom factor must be integer greater than 1"));
						return;
					}
				}

				sim_window_getaveragetimerange(&mintime, &maxtime);
				size = (maxtime - mintime) / factor;
				sim_window_inittraceloop();
				for(j=0; ; j++)
				{
					tr = sim_window_nexttraceloop();
					if (tr == 0) break;
					sim_window_gettimerange(tr, &mintime, &maxtime);
					mintime = sim_window_getmaincursor() - (size / 2.0);
					if (mintime < 0.0) mintime = 0.0;
					maxtime = mintime + size;
					sim_window_settimerange(tr, mintime, maxtime);
				}
				sim_window_redraw();
				return;
			}

			if (namesamen(pp, x_("out"), l) == 0)
			{
				if (count < 4) factor = 2.0; else
				{
					factor = (float)myatoi(par[3]);
					if (factor <= 1)
					{
						ttyputerr(M_("Zoom factor must be integer greater than 1"));
						return;
					}
				}

				sim_window_getaveragetimerange(&mintime, &maxtime);
				size = (maxtime - mintime) * factor;
				sim_window_inittraceloop();
				for(j=0; ; j++)
				{
					tr = sim_window_nexttraceloop();
					if (tr == 0) break;
					sim_window_gettimerange(tr, &mintime, &maxtime);
					mintime = sim_window_getmaincursor() - (size / 2.0);
					if (mintime < 0.0) mintime = 0.0;
					maxtime = mintime + size;
					sim_window_settimerange(tr, mintime, maxtime);
				}
				if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
					sim_window_redraw();
				return;
			}
			if (namesamen(pp, x_("all-displayed"), l) == 0)
			{
				sim_window_gettimeextents(&mintime, &maxtime);
				sim_window_settimerange(0, mintime, maxtime);
				if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
					sim_window_redraw();
				return;
			}
			if (namesamen(pp, x_("window"), l) == 0)
			{
				if (count < 4)
				{
					ttyputusage(x_("telltool simulation window zoom window MIN [MAX]"));
					return;
				}

				mintime = eatof(par[3]);
				maxtime = 0.0;
				if (count > 4) maxtime = eatof(par[4]);
				if (mintime == maxtime)
				{
					ttyputerr(M_("ERROR: Window size must be greater than 0"));
					return;
				}
				if (mintime > maxtime)
				{
					time = maxtime;
					maxtime = mintime;
					mintime = time;
				}
				sim_window_settimerange(0, mintime, maxtime);
				if (sim_window_format == ALS) (void)simals_initialize_simulator(TRUE); else
					sim_window_redraw();
				return;
			}

			ttyputbadusage(x_("telltool simulation window zoom"));
			return;
		}
		ttyputbadusage(x_("telltool simulation window"));
		return;
	}

	if (namesamen(pp, x_("als"), l) == 0 && l >= 1)
	{
		simals_com_comp(count-1, &par[1]);
		return;
	}

	if (namesamen(pp, x_("spice"), l) == 0 && l >= 2)
	{
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("level"), l) == 0)
		{
			if (count >= 3)
			{
				i = eatoi(par[2]);
				if (i <= 0 || i > MAXSPICELEVEL) i = 1;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_levelkey, i, VINTEGER);
			}
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_levelkey);
			if (var == NOVARIABLE) i = 1; else i = var->addr;
			ttyputverbose(M_("Simulation level set to %ld"), i);
			return;
		}
		if (namesamen(pp, x_("save-output"), l) == 0 && l >= 2)
		{
			if (count < 3)
			{
				ttyputerr(M_("Should supply a file name to write"));
				par[2] = x_("electric_def.spo");
			}
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_listingfilekey, (INTBIG)par[2], VSTRING|VDONTSAVE);
			ttyputverbose(M_("Simulation output will go to file %s"), par[2]);
			return;
		}
		if (namesamen(pp, x_("format"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE) spicestate = var->addr; else spicestate = 0;
			if (count <= 2)
			{
				switch (spicestate&SPICETYPE)
				{
					case SPICE2:          ttyputmsg(M_("SPICE decks for SPICE 2"));    break;
					case SPICE3:          ttyputmsg(M_("SPICE decks for SPICE 3"));    break;
					case SPICEHSPICE:     ttyputmsg(M_("SPICE decks for HSPICE"));     break;
					case SPICEPSPICE:     ttyputmsg(M_("SPICE decks for PSPICE"));     break;
					case SPICEGNUCAP:     ttyputmsg(M_("SPICE decks for Gnucap"));     break;
					case SPICESMARTSPICE: ttyputmsg(M_("SPICE decks for SmartSPICE")); break;
						
				}
				return;
			}

			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("2"), l) == 0)
			{
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(spicestate & ~SPICETYPE) | SPICE2, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("3"), l) == 0)
			{
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(spicestate & ~SPICETYPE) | SPICE3, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("hspice"), l) == 0)
			{
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(spicestate & ~SPICETYPE) | SPICEHSPICE, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("pspice"), l) == 0)
			{
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(spicestate & ~SPICETYPE) | SPICEPSPICE, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("gnucap"), l) == 0)
			{
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(spicestate & ~SPICETYPE) | SPICEGNUCAP, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("smartspice"), l) == 0)
			{
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(spicestate & ~SPICETYPE) | SPICESMARTSPICE, VINTEGER);
				return;
			}

		}
		if (namesamen(pp, x_("parse-output"), l) == 0)
		{
			if (count < 3)
			{
				ttyputerr(M_("Must supply a SPICE output file to read"));
				return;
			}
			sim_spice_execute(x_(""), par[2], NONODEPROTO);
			return;
		}
		if (namesamen(pp, x_("show-spice-this-cell"), l) == 0 && l >= 2)
		{
			/* plot the spice deck for this cell */
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to simulate"));
				return;
			}
			infstr = initinfstr();
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE) spicestate = var->addr; else spicestate = 0;
			switch (spicestate&SPICETYPE)
			{
				case SPICEGNUCAP:		/* handle ".raw" */
					formatinfstr(infstr, x_("%s.raw"), np->protoname);
					type = sim_filetypesrawspiceout;
					break;
				case SPICEHSPICE:		/* handle ".tr0" */
					formatinfstr(infstr, x_("%s.tr0"), np->protoname);
					type = sim_filetypehspiceout;
					break;
				default:				/* handle ".spo" */
					formatinfstr(infstr, x_("%s.spo"), np->protoname);
					type = sim_filetypespiceout;
					break;
			}
			allocstring(&spiceoutfile, returninfstr(infstr), el_tempcluster);

			/* get the default path (where the current library resides) */
			infstr = initinfstr();
			addstringtoinfstr(infstr, np->lib->libfile);
			deflib = returninfstr(infstr);
			l = strlen(deflib);
			for(i=l-1; i>0; i--) if (deflib[i] == DIRSEP) break;
			deflib[i] = 0;

			io = xopen(spiceoutfile, type, deflib, &truename);
			if (io == 0)
			{
				ttyputerr(_("Could not find simulation output file: %s"), spiceoutfile);
			} else
			{
				xclose(io);
				reallocstring(&spiceoutfile, truename, el_tempcluster);
				sim_spice_execute(x_(""), spiceoutfile, np);
			}
			efree(spiceoutfile);
			return;
		}
		if (namesamen(pp, x_("not"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation spice not OPTION"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("resistance"), l) == 0 || namesamen(pp, x_("capacitances"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
				if (var != NOVARIABLE)
					(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
						var->addr & ~SPICERESISTANCE, VINTEGER);
				ttyputverbose(M_("SPICE decks will not print parasitics"));
				return;
			}
			if (namesamen(pp, x_("plot"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
				if (var != NOVARIABLE)
					(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
						var->addr & ~SPICEPLOT, VINTEGER);
				ttyputverbose(M_("SPICE decks will not do plots"));
				return;
			}
			if (namesamen(pp, x_("global-pwr-gnd"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
				if (var != NOVARIABLE)
					(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
						var->addr & ~SPICEGLOBALPG, VINTEGER);
				ttyputverbose(M_("Does not write global Power and Ground"));
				return;
			}
			if (namesamen(pp, x_("use-nodenames"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
				if (var != NOVARIABLE)
					(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
						var->addr & ~SPICENODENAMES, VINTEGER);
				ttyputverbose(M_("SPICE decks will contain node numbers"));
				return;
			}
			ttyputbadusage(x_("telltool simulation spice not"));
			return;
		}
		if (namesamen(pp, x_("resistance"), l) == 0 || namesamen(pp, x_("capacitances"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					var->addr | SPICERESISTANCE, VINTEGER);
			ttyputverbose(M_("SPICE decks will print parasitics"));
			return;
		}
		if (namesamen(pp, x_("plot"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					var->addr | SPICEPLOT, VINTEGER);
			ttyputverbose(M_("SPICE decks will do plots"));
			return;
		}
		if (namesamen(pp, x_("output-normal"), l) == 0 && l >= 8)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(var->addr & ~SPICEOUTPUT) | SPICEOUTPUTNORM, VINTEGER);
			ttyputverbose(M_("Reads normal SPICE output"));
			return;
		}
		if (namesamen(pp, x_("output-raw"), l) == 0 && l >= 8)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(var->addr & ~SPICEOUTPUT) | SPICEOUTPUTRAW, VINTEGER);
			ttyputverbose(M_("Reads SPICE Rawfile output"));
			return;
		}
		if (namesamen(pp, x_("output-smartraw"), l) == 0 && l >= 8)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					(var->addr & ~SPICEOUTPUT) | SPICEOUTPUTRAWSMART, VINTEGER);
			ttyputverbose(M_("Reads SmartSPICE Rawfile output"));
			return;
		}
		if (namesamen(pp, x_("global-pwr-gnd"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					var->addr | SPICEGLOBALPG, VINTEGER);
			ttyputverbose(M_("Writes global Power and Ground"));
			return;
		}
		if (namesamen(pp, x_("use-nodenames"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
			if (var != NOVARIABLE)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey,
					var->addr | SPICENODENAMES, VINTEGER);
			ttyputverbose(M_("SPICE decks will contain node names"));
			return;
		}
		ttyputbadusage(x_("telltool simulation spice"));
		return;
	}

	if (namesamen(pp, x_("verilog"), l) == 0 && l >= 1)
	{
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("use-assign"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_verilog_statekey);
			if (var != NOVARIABLE) verilogstate = var->addr; else
				verilogstate = 0;
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_verilog_statekey,
				verilogstate | VERILOGUSEASSIGN, VINTEGER);
			ttyputverbose(M_("Verilog decks will use the 'assign' construct"));
			return;
		}
		if (namesamen(pp, x_("default-trireg"), l) == 0)
		{
			var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_verilog_statekey);
			if (var != NOVARIABLE) verilogstate = var->addr; else
				verilogstate = 0;
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_verilog_statekey,
				verilogstate | VERILOGUSETRIREG, VINTEGER);
			ttyputverbose(M_("Verilog wires will default to 'trireg'"));
			return;
		}
		if (namesamen(pp, x_("parse-output"), l) == 0)
		{
			if (count < 3)
			{
				ttyputerr(M_("Must supply a Verilog output file to read"));
				return;
			}
			sim_verparsefile(par[2], NONODEPROTO);
			return;
		}
		if (namesamen(pp, x_("show-verilog-this-cell"), l) == 0)
		{
			/* plot the spice deck for this cell */
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to simulate"));
				return;
			}
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s.dump"), np->protoname);
			allocstring(&spiceoutfile, returninfstr(infstr), el_tempcluster);

			/* get the default path (where the current library resides) */
			infstr = initinfstr();
			addstringtoinfstr(infstr, np->lib->libfile);
			deflib = returninfstr(infstr);
			l = strlen(deflib);
			for(i=l-1; i>0; i--) if (deflib[i] == DIRSEP) break;
			deflib[i] = 0;

			io = xopen(spiceoutfile, sim_filetypeverilogvcd, deflib, &truename);
			if (io == 0)
			{
				ttyputerr(_("Could not find simulation output file: %s"), spiceoutfile);
			} else
			{
				xclose(io);
				reallocstring(&spiceoutfile, truename, el_tempcluster);
				sim_verparsefile(spiceoutfile, np);
			}
			efree(spiceoutfile);
			return;
		}

		if (namesamen(pp, x_("not"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation verilog not OPTION"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("use-assign"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_verilog_statekey);
				if (var != NOVARIABLE) verilogstate = var->addr; else
					verilogstate = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_verilog_statekey,
					verilogstate & ~VERILOGUSEASSIGN, VINTEGER);
				ttyputverbose(M_("Verilog decks will not use the 'assign' construct"));
				return;
			}
			if (namesamen(pp, x_("default-trireg"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_verilog_statekey);
				if (var != NOVARIABLE) verilogstate = var->addr; else
					verilogstate = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_verilog_statekey,
					verilogstate & ~VERILOGUSETRIREG, VINTEGER);
				ttyputverbose(M_("Verilog wires will not default to 'trireg'"));
				return;
			}
			ttyputbadusage(x_("telltool simulation verilog not"));
			return;
		}
		ttyputbadusage(x_("telltool simulation verilog"));
		return;
	}

	if (namesamen(pp, x_("fasthenry"), l) == 0 && l >= 1)
	{
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("pole"), l) == 0 && l >= 3)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry pole (single | multiple NUMPOLES)"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("single"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey, options & ~FHMAKEMULTIPOLECKT, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("multiple"), l) == 0)
			{
				if (count <= 3)
				{
					ttyputusage(x_("telltool simulation fasthenry pole multiple NUMPOLES"));
					return;
				}
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey, options | FHMAKEMULTIPOLECKT, VINTEGER);
				value = eatoi(par[3]);
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrynumpoleskey, value, VINTEGER);
				return;
			}
			ttyputbadusage(x_("telltool simulation fasthenry pole"));
			return;
		}
		if (namesamen(pp, x_("frequency"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry frequency (single | multiple START END RUNSPERDECADE)"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("single"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				options |= FHUSESINGLEFREQ;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey, options, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("multiple"), l) == 0)
			{
				if (count <= 5)
				{
					ttyputusage(x_("telltool simulation fasthenry frequency multiple START END RUNSPERDECADE"));
					return;
				}
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				options &= ~FHUSESINGLEFREQ;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey, options, VINTEGER);
				fvalue = (float)eatof(par[3]);
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryfreqstartkey, castint(fvalue), VFLOAT);
				fvalue = (float)eatof(par[4]);
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryfreqendkey, castint(fvalue), VFLOAT);
				value = eatoi(par[5]);
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryrunsperdecadekey, value, VINTEGER);
				return;
			}
			ttyputbadusage(x_("telltool simulation fasthenry frequency"));
			return;
		}
		if (namesamen(pp, x_("thickness"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry thickness DEFTHICKNESS"));
				return;
			}
			value = atola(par[2], 0);
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrythicknesskey, value, VINTEGER);
			return;
		}
		if (namesamen(pp, x_("width-subdivisions"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry width-subdivisions DEFWIDTHSUBS"));
				return;
			}
			value = eatoi(par[2]);
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrywidthsubdivkey, value, VINTEGER);
			return;
		}
		if (namesamen(pp, x_("height-subdivisions"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry height-subdivisions DEFHEIGHTSUBS"));
				return;
			}
			value = eatoi(par[2]);
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryheightsubdivkey, value, VINTEGER);
			return;
		}
		if (namesamen(pp, x_("max-segment-length"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry max-segment-length MAXSEGLENGTH"));
				return;
			}
			value = atola(par[2], 0);
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenryseglimitkey, value, VINTEGER);
			return;
		}
		if (namesamen(pp, x_("execute"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry execute (none | run | multiple-run)"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("none"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					(options & ~FHEXECUTETYPE) | FHEXECUTENONE, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("run"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					(options & ~FHEXECUTETYPE) | FHEXECUTERUNFH, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("multiple-run"), l) == 0)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					(options & ~FHEXECUTETYPE) | FHEXECUTERUNFHMUL, VINTEGER);
				return;
			}
			ttyputbadusage(x_("telltool simulation fasthenry execute"));
			return;
		}
		if (namesamen(pp, x_("postscript"), l) == 0 && l >= 3)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry postscript (on | off)"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("on"), l) == 0 && l >= 2)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					options | FHMAKEPOSTSCRIPTVIEW, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					options & ~FHMAKEPOSTSCRIPTVIEW, VINTEGER);
				return;
			}
			ttyputbadusage(x_("telltool simulation fasthenry postscript"));
			return;
		}
		if (namesamen(pp, x_("spice"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry spice (on | off)"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("on"), l) == 0 && l >= 2)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					options | FHMAKESPICESUBCKT, VINTEGER);
				return;
			}
			if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
			{
				var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_fasthenrystatekey);
				if (var != NOVARIABLE) options = var->addr; else options = 0;
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_fasthenrystatekey,
					options & ~FHMAKESPICESUBCKT, VINTEGER);
				return;
			}
			ttyputbadusage(x_("telltool simulation fasthenry spice"));
			return;
		}
		if (namesamen(pp, x_("arc"), l) == 0)
		{
			ai = (ARCINST *)asktool(us_tool, x_("get-arc"));
			if (ai == NOARCINST)
			{
				ttyputerr(M_("Select an arc first"));
				return;
			}
			if (count <= 2)
			{
				ttyputusage(x_("telltool simulation fasthenry arc OPTIONS"));
				return;
			}
			l = estrlen(pp = par[2]);
			if (namesamen(pp, x_("add"), l) == 0)
			{
				if (count <= 3)
				{
					ttyputusage(x_("telltool simulation fasthenry arc add GROUPNAME"));
					return;
				}
				startobjectchange((INTBIG)ai, VARCINST);
				setvalkey((INTBIG)ai, VARCINST, sim_fasthenrygroupnamekey, (INTBIG)par[3], VSTRING|VDISPLAY);
				endobjectchange((INTBIG)ai, VARCINST);
				return;
			}
			if (namesamen(pp, x_("remove"), l) == 0)
			{
				if (getvalkey((INTBIG)ai, VARCINST, VSTRING, sim_fasthenrygroupnamekey) != NOVARIABLE)
				{
					startobjectchange((INTBIG)ai, VARCINST);
					(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenrygroupnamekey);
					endobjectchange((INTBIG)ai, VARCINST);
				}
				return;
			}
			if (namesamen(pp, x_("thickness"), l) == 0)
			{
				if (count <= 3)
				{
					if (getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenrythicknesskey) != NOVARIABLE)
					{
						startobjectchange((INTBIG)ai, VARCINST);
						(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenrythicknesskey);
						endobjectchange((INTBIG)ai, VARCINST);
					}
				} else
				{
					startobjectchange((INTBIG)ai, VARCINST);
					setvalkey((INTBIG)ai, VARCINST, sim_fasthenrythicknesskey, atola(par[3], 0), VINTEGER);
					endobjectchange((INTBIG)ai, VARCINST);
				}
				return;
			}
			if (namesamen(pp, x_("width-subdivisions"), l) == 0)
			{
				if (count <= 3)
				{
					if (getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenrywidthsubdivkey) != NOVARIABLE)
					{
						startobjectchange((INTBIG)ai, VARCINST);
						(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenrywidthsubdivkey);
						endobjectchange((INTBIG)ai, VARCINST);
					}
				} else
				{
					startobjectchange((INTBIG)ai, VARCINST);
					setvalkey((INTBIG)ai, VARCINST, sim_fasthenrywidthsubdivkey, eatoi(par[3]), VINTEGER);
					endobjectchange((INTBIG)ai, VARCINST);
				}
				return;
			}
			if (namesamen(pp, x_("height-subdivisions"), l) == 0)
			{
				if (count <= 3)
				{
					if (getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryheightsubdivkey) != NOVARIABLE)
					{
						startobjectchange((INTBIG)ai, VARCINST);
						(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenryheightsubdivkey);
						endobjectchange((INTBIG)ai, VARCINST);
					}
				} else
				{
					startobjectchange((INTBIG)ai, VARCINST);
					setvalkey((INTBIG)ai, VARCINST, sim_fasthenryheightsubdivkey, eatoi(par[3]), VINTEGER);
					endobjectchange((INTBIG)ai, VARCINST);
				}
				return;
			}
			if (namesamen(pp, x_("z-head"), l) == 0 && l >= 3)
			{
				if (count <= 3)
				{
					if (getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryzheadkey) != NOVARIABLE)
					{
						startobjectchange((INTBIG)ai, VARCINST);
						(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenryzheadkey);
						endobjectchange((INTBIG)ai, VARCINST);
					}
				} else
				{
					startobjectchange((INTBIG)ai, VARCINST);
					setvalkey((INTBIG)ai, VARCINST, sim_fasthenryzheadkey, atola(par[3], 0), VINTEGER);
					endobjectchange((INTBIG)ai, VARCINST);
				}
				return;
			}
			if (namesamen(pp, x_("z-tail"), l) == 0 && l >= 3)
			{
				if (count <= 3)
				{
					if (getvalkey((INTBIG)ai, VARCINST, VINTEGER, sim_fasthenryztailkey) != NOVARIABLE)
					{
						startobjectchange((INTBIG)ai, VARCINST);
						(void)delvalkey((INTBIG)ai, VARCINST, sim_fasthenryztailkey);
						endobjectchange((INTBIG)ai, VARCINST);
					}
				} else
				{
					startobjectchange((INTBIG)ai, VARCINST);
					setvalkey((INTBIG)ai, VARCINST, sim_fasthenryztailkey, atola(par[3], 0), VINTEGER);
					endobjectchange((INTBIG)ai, VARCINST);
				}
				return;
			}
			ttyputbadusage(x_("telltool simulation fasthenry arc"));
			return;
		}
		ttyputbadusage(x_("telltool simulation fasthenry"));
		return;
	}

	/* unknown command */
	ttyputbadusage(x_("telltool simulation"));
}

INTBIG sim_request(CHAR *command, va_list ap)
{
	REGISTER INTBIG arg1, arg2;
	CHAR *lowername;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;

	if (namesame(command, x_("traverse-up")) == 0)
	{
		arg1 = va_arg(ap, INTBIG);
		np = (NODEPROTO *)arg1;
		switch (sim_window_state&SIMENGINECUR)
		{
			case SIMENGINECURALS:
				simals_level_up_command();
				break;
			case SIMENGINECURVERILOG:
				sim_verlevel_up(np);
				break;
#if SIMTOOLIRSIM != 0
			case SIMENGINECURIRSIM:
				irsim_level_up(np);
				break;
#endif
		}
		return(0);
	}
	if (namesame(command, x_("traverse-down")) == 0)
	{
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);
		ni = (NODEINST *)arg1;
		np = (NODEPROTO *)arg2;
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var == NOVARIABLE) return(0);
		lowername = (CHAR *)var->addr;
		switch (sim_window_state&SIMENGINECUR)
		{
			case SIMENGINECURALS:
				simals_level_set_command(lowername);
				break;
			case SIMENGINECURVERILOG:
				sim_verlevel_set(lowername, np);
				break;
			case SIMENGINECURSPICE:
				break;
#if SIMTOOLIRSIM != 0
			case SIMENGINECURIRSIM:
				irsim_level_set(lowername, np);
				break;
#endif
		}
		return(0);
	}
	return(0);
}

void sim_slice(void)
{
	NODEPROTO *np;

	if (sim_circuitchanged && !sim_undoredochange)
	{
		if (sim_window_isactive(&np) != 0)
		{
			sim_window_stopsimulation();
			ttyputmsg(_("Circuit changed: simulation stopped"));
			setactivity(_("Simulation Halted"));
		}
	}
	sim_circuitchanged = FALSE;
	sim_undoredochange = FALSE;

	/* handle VCR controls in the simulation window */
	sim_window_advancetime();
}

void sim_startbatch(TOOL *source, BOOLEAN undoredo)
{
	Q_UNUSED( source );
	NODEPROTO *np;

	sim_undoredochange = undoredo;
	sim_window_curactive = sim_window_isactive(&np);
}

void sim_modifynodeinst(NODEINST *ni, INTBIG olx, INTBIG oly, INTBIG ohx, INTBIG ohy,
	INTBIG orot, INTBIG otran)
{
	Q_UNUSED( olx );
	Q_UNUSED( oly );
	Q_UNUSED( ohx );
	Q_UNUSED( ohy );
	Q_UNUSED( orot );
	Q_UNUSED( otran );

	if (sim_window_curactive == 0) return;
	sim_checktostopsimulation(ni->parent);
}

void sim_modifyarcinst(ARCINST *ai, INTBIG oxA, INTBIG oyA, INTBIG oxB, INTBIG oyB,
	INTBIG owid, INTBIG olen)
{
	Q_UNUSED( oxA );
	Q_UNUSED( oyA );
	Q_UNUSED( oxB );
	Q_UNUSED( oyB );
	Q_UNUSED( owid );
	Q_UNUSED( olen );

	if (sim_window_curactive == 0) return;
	sim_checktostopsimulation(ai->parent);
}

void sim_modifyportproto(PORTPROTO *pp, NODEINST *oni, PORTPROTO *opp)
{
	Q_UNUSED( oni );
	Q_UNUSED( opp );

	if (sim_window_curactive == 0) return;
	sim_checktostopsimulation(pp->parent);
}

void sim_newobject(INTBIG addr, INTBIG type)
{
	if (sim_window_curactive == 0) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  sim_checktostopsimulation(((NODEINST *)addr)->parent);   break;
		case VARCINST:   sim_checktostopsimulation(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: sim_checktostopsimulation(((PORTPROTO *)addr)->parent);  break;
	}
}

void sim_killobject(INTBIG addr, INTBIG type)
{
	if (sim_window_curactive == 0) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  sim_checktostopsimulation(((NODEINST *)addr)->parent);   break;
		case VARCINST:   sim_checktostopsimulation(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: sim_checktostopsimulation(((PORTPROTO *)addr)->parent);  break;
		case VNODEPROTO: sim_checktostopsimulation((NODEPROTO *)addr);            break;
	}
}

void sim_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
	REGISTER CHAR *name;
	REGISTER INTBIG i;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;

	if ((newtype&VCREF) != 0)
	{
		/* detect change to "ai->userbits" to see if arc negation changed */
		if (sim_window_curactive == 0) return;
		if ((type&VTYPE) != VARCINST) return;
		name = changedvariablename(type, key, newtype);
		if (namesame(name, x_("userbits")) != 0) return;

		ai = (ARCINST *)addr;
		if (ai != sim_modifyarc) return;
		if (((INTBIG)(ai->userbits&ISNEGATED)) != (sim_modifyarcbits&ISNEGATED))
			sim_checktostopsimulation(ai->parent);
	} else
	{
		/* detect change to cached window state */
		if ((type&VTYPE) == VTOOL && (TOOL *)addr == sim_tool)
		{
			for(i=0; sim_variablemirror[i].key != 0; i++)
			{
				if (key == *sim_variablemirror[i].key)
				{
					var = getvalkey(addr, type, -1, key);
					if (var != NOVARIABLE) *sim_variablemirror[i].value = var->addr;
					return;
				}
			}
		}

		/* other changes only valid if simulation is running */
		if (sim_window_curactive == 0) return;

		/* detect change to arc's name */
		if ((type&VTYPE) == VARCINST && key == el_arc_name_key)
		{
			ai = (ARCINST *)addr;
			sim_checktostopsimulation(ai->parent);
		}

		/* detect change to node's name */
		if ((type&VTYPE) == VNODEINST && key == el_node_name_key)
		{
			ni = (NODEINST *)addr;
			sim_checktostopsimulation(ni->parent);
		}
	}
}

void sim_killvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG oldaddr, INTBIG oldtype,
	UINTBIG *olddescript)
{
	REGISTER CHAR *name;
	Q_UNUSED( oldaddr );
	Q_UNUSED( olddescript );

	if ((oldtype&VCREF) != 0)
	{
		/* detect change to "ai->userbits" and save former value */
		if ((type&VTYPE) != VARCINST) return;
		name = changedvariablename(type, key, oldtype);
		if (namesame(name, x_("userbits")) != 0) return;

		sim_modifyarc = (ARCINST *)addr;
		sim_modifyarcbits = sim_modifyarc->userbits;
	}
}

void sim_modifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype,
	INTBIG aindex, INTBIG oldvalue)
{
	Q_UNUSED( vartype );
	Q_UNUSED( aindex );
	Q_UNUSED( oldvalue );

	if (sim_window_curactive == 0) return;
	if (type != VNODEPROTO) return;
	if (key != el_cell_message_key) return;
	sim_checktostopsimulation((NODEPROTO *)addr);
}

void sim_insertvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype, INTBIG index)
{
	Q_UNUSED( vartype );
	Q_UNUSED( index );

	if (sim_window_curactive == 0) return;
	if (type != VNODEPROTO) return;
	if (key != el_cell_message_key) return;
	sim_checktostopsimulation((NODEPROTO *)addr);
}

void sim_deletevariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype, INTBIG index,
	INTBIG oldvalue)
{
	Q_UNUSED( vartype );
	Q_UNUSED( index );
	Q_UNUSED( oldvalue );

	if (sim_window_curactive == 0) return;
	if (type != VNODEPROTO) return;
	if (key != el_cell_message_key) return;
	sim_checktostopsimulation((NODEPROTO *)addr);
}

void sim_readlibrary(LIBRARY *lib)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i;
	Q_UNUSED( lib );

	/* recache mirrored variables now that the library may have overridden them */
	for(i=0; sim_variablemirror[i].key != 0; i++)
	{
		var = getvalkey((INTBIG)sim_tool, VTOOL, -1, *sim_variablemirror[i].key);
		if (var != NOVARIABLE)
		{
			*sim_variablemirror[i].value = var->addr;
		}
	}
}

/****************************** SUPPORT ******************************/

void sim_window_setstate(INTBIG newstate)
{
	(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_window_statekey, newstate, VINTEGER);
	sim_window_state = newstate;
}

void sim_checktostopsimulation(NODEPROTO *np)
{
	NODEPROTO *onp;

	if (np->primindex != 0) return;
	(void)sim_window_isactive(&onp);
	if (onp != np)
	{
		/* cells not the same, but see if the modified one is VHDL or netlist */
		if (!insamecellgrp(onp, np)) return;
		if (np->cellview != el_vhdlview && np->cellview != el_netlistalsview)
			return;
	}
	sim_circuitchanged = TRUE;
}

/*
 * Routine to initialize the accumulation of bus signals.  Subsequent routines
 * call "sim_addbussignal()" to add a signal to the bus.  When done, call
 * "sim_getbussignals()" to get the total and the array.
 */
void sim_initbussignals(void)
{
	sim_bussignalcount = 0;
}

/*
 * Routine to add signal "signal" to the list of bus signals.
 */
void sim_addbussignal(INTBIG signal)
{
	REGISTER INTBIG i, newtotal, *newsignals;

	if (sim_bussignalcount >= sim_bussignaltotal)
	{
		newtotal = sim_bussignaltotal * 2;
		if (sim_bussignalcount >= newtotal) newtotal = sim_bussignalcount+20;
		newsignals = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, sim_tool->cluster);
		if (newsignals == 0) return;
		for(i=0; i<sim_bussignalcount; i++)
			newsignals[i] = sim_bussignals[i];
		if (sim_bussignaltotal > 0) efree((CHAR *)sim_bussignals);
		sim_bussignals = newsignals;
		sim_bussignaltotal = newtotal;
	}
	sim_bussignals[sim_bussignalcount++] = signal;
}

/*
 * Routine to return the bus signal array in "signallist" and return its size.
 */
INTBIG sim_getbussignals(INTBIG **signallist)
{
	*signallist = sim_bussignals;
	return(sim_bussignalcount);
}

/*
 * Routine to read-out a list of signal names in a waveform window "simwin".
 * Calls "addbranch()" to create a branch of the tree, "addleaf()" to add a leaf.
 * Both routines take the name and the parent node (0 if at the top)
 * and both routines return their node object.
 * The name of a node object can be obtained with "nodename()".
 */
void sim_reportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
	void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*),CHAR *(*nodename)(void*))
{
	switch (sim_window_state&SIMENGINECUR)
	{
		case SIMENGINECURALS:
			simals_reportsignals(simwin, addbranch, findbranch, addleaf, nodename);
			break;
		case SIMENGINECURVERILOG:
			sim_verreportsignals(simwin, addbranch, findbranch, addleaf, nodename);
			break;
		case SIMENGINECURSPICE:
			sim_spicereportsignals(simwin, addbranch, findbranch, addleaf, nodename);
			break;
#if SIMTOOLIRSIM != 0
		case SIMENGINECURIRSIM:
			irsim_reportsignals(simwin, addbranch, findbranch, addleaf, nodename);
			break;
#endif
	}
}

/*
 * Routine to add signal "sig" to the waveform window, overlaid if "overlay" is TRUE.
 */
void sim_addsignal(WINDOWPART *simwin, CHAR *sig, BOOLEAN overlay)
{
	switch (sim_window_state&SIMENGINECUR)
	{
		case SIMENGINECURALS:
			break;
		case SIMENGINECURVERILOG:
			sim_veraddhighlightednet(sig);
			break;
		case SIMENGINECURSPICE:
			sim_spice_addhighlightednet(sig, overlay);
			break;
#if SIMTOOLIRSIM != 0
		case SIMENGINECURIRSIM:
			irsim_adddisplayedsignal(sig);
			break;
#endif
	}
}

CHAR *sim_signalseparator(void)
{
	REGISTER CHAR *sepstr;

	switch (sim_window_state&SIMENGINECUR)
	{
		case SIMENGINECURALS:     sepstr = x_(".");   break;
		case SIMENGINECURVERILOG: sepstr = x_(".");   break;
		case SIMENGINECURSPICE:   sepstr = x_(".");   break;
#if SIMTOOLIRSIM != 0
		case SIMENGINECURIRSIM:   sepstr = x_("/");   break;
#endif
	}
	return(sepstr);
}

/****************************** DIALOG ******************************/

/* Simulation Options */
static DIALOGITEM sim_optionsdialogitems[] =
{
 /*  1 */ {0, {312,484,336,548}, BUTTON, N_("OK")},
 /*  2 */ {0, {312,384,336,448}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {124,4,140,201}, CHECK, N_("Resimulate each change")},
 /*  4 */ {0, {148,4,164,201}, CHECK, N_("Auto advance time")},
 /*  5 */ {0, {172,4,188,201}, CHECK, N_("Multistate display")},
 /*  6 */ {0, {52,4,68,201}, CHECK, N_("Show waveform window")},
 /*  7 */ {0, {228,292,244,417}, MESSAGE, N_("Maximum events:")},
 /*  8 */ {0, {228,420,244,501}, EDITTEXT, x_("")},
 /*  9 */ {0, {28,4,44,161}, MESSAGE, N_("Base for bus values:")},
 /* 10 */ {0, {28,164,44,217}, POPUP, x_("")},
 /* 11 */ {0, {76,16,92,201}, MESSAGE, N_("Place waveform window")},
 /* 12 */ {0, {96,60,112,200}, POPUP, x_("")},
 /* 13 */ {0, {4,4,20,129}, MESSAGE, N_("Simulation engine:")},
 /* 14 */ {0, {4,132,20,276}, POPUP, x_("")},
 /* 15 */ {0, {204,4,220,89}, MESSAGE, N_("IRSIM:")},
 /* 16 */ {0, {204,284,220,368}, MESSAGE, N_("ALS:")},
 /* 17 */ {0, {244,28,260,101}, RADIO, N_("Quick")},
 /* 18 */ {0, {244,104,260,177}, RADIO, N_("Local")},
 /* 19 */ {0, {244,180,260,253}, RADIO, N_("Full")},
 /* 20 */ {0, {196,4,197,565}, DIVIDELINE, x_("")},
 /* 21 */ {0, {268,16,284,129}, MESSAGE, N_("Parameter file:")},
 /* 22 */ {0, {288,28,320,273}, EDITTEXT, x_("")},
 /* 23 */ {0, {224,16,240,153}, MESSAGE, N_("Parasitics:")},
 /* 24 */ {0, {268,172,284,229}, BUTTON, N_("Set")},
 /* 25 */ {0, {4,280,345,281}, DIVIDELINE, x_("")},
 /* 26 */ {0, {329,16,345,213}, CHECK, N_("Show commands")},
 /* 27 */ {0, {32,288,48,441}, MESSAGE, N_("Low:")},
 /* 28 */ {0, {32,444,48,565}, POPUP, x_("")},
 /* 29 */ {0, {52,288,68,441}, MESSAGE, N_("High:")},
 /* 30 */ {0, {52,444,68,565}, POPUP, x_("")},
 /* 31 */ {0, {72,288,88,441}, MESSAGE, N_("Undefined (X):")},
 /* 32 */ {0, {72,444,88,565}, POPUP, x_("")},
 /* 33 */ {0, {92,288,108,441}, MESSAGE, N_("Floating (Z):")},
 /* 34 */ {0, {92,444,108,565}, POPUP, x_("")},
 /* 35 */ {0, {112,288,128,441}, MESSAGE, N_("Strength 0 (off):")},
 /* 36 */ {0, {112,444,128,565}, POPUP, x_("")},
 /* 37 */ {0, {132,288,148,441}, MESSAGE, N_("Strength 1 (node):")},
 /* 38 */ {0, {132,444,148,565}, POPUP, x_("")},
 /* 39 */ {0, {152,288,168,441}, MESSAGE, N_("Strength 2 (gate):")},
 /* 40 */ {0, {152,444,168,565}, POPUP, x_("")},
 /* 41 */ {0, {172,288,188,441}, MESSAGE, N_("Strength 3 (power):")},
 /* 42 */ {0, {172,444,188,565}, POPUP, x_("")},
 /* 43 */ {0, {8,288,24,565}, MESSAGE, N_("Waveform window colors:")}
};
static DIALOG sim_optionsdialog = {{50,75,404,650}, N_("Simulation Options"), 0, 43, sim_optionsdialogitems, 0, 0};

/* special items for the "Simulation Options" dialog: */
#define DSMO_RESIMULATE     3		/* Resimulate each change (check) */
#define DSMO_AUTOADVANCE    4		/* Auto advance time (check) */
#define DSMO_MULTISTATE     5		/* Multistate display (check) */
#define DSMO_WAVEFORMWIND   6		/* Show waveform window (check) */
#define DSMO_MAXEVENTS      8		/* Maximum events (edit text) */
#define DSMO_BUSBASE       10		/* Base for busses (popup) */
#define DSMO_WAVEFORMLOC   12		/* Place waveform window (popup) */
#define DSMO_SIMENGINE     14		/* Simulation engine to use (popup) */
#define DSMO_IRSIMPARAQUI  17		/* IRSIM: Quick parasitics (radio) */
#define DSMO_IRSIMPARALOC  18		/* IRSIM: Local parasitics (radio) */
#define DSMO_IRSIMPARAFULL 19		/* IRSIM: Full parasitics (radio) */
#define DSMO_IRSIMPARMFILE 22		/* IRSIM: Parameter file (edit text) */
#define DSMO_IRSIMSETPFILE 24		/* IRSIM: Set Parameter file (button) */
#define DSMO_IRSIMSHOWCMDS 26		/* IRSIM: Show command (check) */
#define DSMO_COLORLOW      28		/* Color for low (popup) */
#define DSMO_COLORHIGH     30		/* Color for high (popup) */
#define DSMO_COLORX        32		/* Color for X (popup) */
#define DSMO_COLORZ        34		/* Color for Z (popup) */
#define DSMO_COLORSTR0     36		/* Color for strength 0 (off) (popup) */
#define DSMO_COLORSTR1     38		/* Color for strength 1 (node) (popup) */
#define DSMO_COLORSTR2     40		/* Color for strength 2 (gate) (popup) */
#define DSMO_COLORSTR3     42		/* Color for strength 3 (power) (popup) */

static INTBIG sim_window_colorvaluelist[20] =
{
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

void sim_optionsdlog(void)
{
	INTBIG itemHit, newtracesize, i;
	REGISTER VARIABLE *var;
	CHAR line[20], *newlang[20], *iniparamfile, *pt, *colorname, *colorsymbol;
	REGISTER INTBIG noupdate, newnoupdate, newwindowstate, irsimstate,
		newirsimstate, oldplease;
	REGISTER TRAKPTR newtrakroot;
	static CHAR *busbase[4] = {x_("2"), x_("8"), x_("10"), x_("16")};
	static CHAR *placement[3] = {N_("Cascade"), N_("Tile Horizontally"), N_("Tile Vertically")};
	static CHAR *engine[2] = {N_("ALS"), N_("IRSIM")};
	REGISTER void *infstr, *dia;

	dia = DiaInitDialog(&sim_optionsdialog);
	if (dia == 0) return;
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(placement[i]);
	DiaSetPopup(dia, DSMO_WAVEFORMLOC, 3, newlang);
#if SIMTOOLIRSIM == 0
	engine[1] = N_("IRSIM (not installed)");
#endif
	for(i=0; i<2; i++) newlang[i] = TRANSLATE(engine[i]);
	DiaSetPopup(dia, DSMO_SIMENGINE, 2, newlang);
	var = getval((INTBIG)sim_tool, VTOOL, VINTEGER, x_("SIM_als_no_update"));
	if (var == NOVARIABLE) noupdate = 0; else noupdate = var->addr;
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_irsim_statekey);
	if (var == NOVARIABLE) irsimstate = DEFIRSIMSTATE; else irsimstate = var->addr;
	if (noupdate == 0) DiaSetControl(dia, DSMO_RESIMULATE, 1);
	if ((sim_window_state&ADVANCETIME) != 0) DiaSetControl(dia, DSMO_AUTOADVANCE, 1);
	if ((sim_window_state&FULLSTATE) != 0) DiaSetControl(dia, DSMO_MULTISTATE, 1);
	if ((sim_window_state&SHOWWAVEFORM) != 0) DiaSetControl(dia, DSMO_WAVEFORMWIND, 1);
	if ((irsimstate&IRSIMSHOWCOMMANDS) != 0) DiaSetControl(dia, DSMO_IRSIMSHOWCMDS, 1);
	DiaSetPopup(dia, DSMO_BUSBASE, 4, busbase);
	switch (sim_window_state&SIMENGINE)
	{
		case SIMENGINEALS:   DiaSetPopupEntry(dia, DSMO_SIMENGINE, 0);   break;
		case SIMENGINEIRSIM: DiaSetPopupEntry(dia, DSMO_SIMENGINE, 1);   break;
	}
	switch (irsimstate&IRSIMPARASITICS)
	{
		case IRSIMPARAQUICK: DiaSetControl(dia, DSMO_IRSIMPARAQUI, 1);   break;
		case IRSIMPARALOCAL: DiaSetControl(dia, DSMO_IRSIMPARALOC, 1);   break;
		case IRSIMPARAFULL:  DiaSetControl(dia, DSMO_IRSIMPARAFULL, 1);  break;
	}
	switch (sim_window_state&BUSBASEBITS)
	{
		case BUSBASE2:  DiaSetPopupEntry(dia, DSMO_BUSBASE, 0);   break;
		case BUSBASE8:  DiaSetPopupEntry(dia, DSMO_BUSBASE, 1);   break;
		case BUSBASE10: DiaSetPopupEntry(dia, DSMO_BUSBASE, 2);   break;
		case BUSBASE16: DiaSetPopupEntry(dia, DSMO_BUSBASE, 3);   break;
	}
	switch (sim_window_state&WAVEPLACE)
	{
		case WAVEPLACECAS:   DiaSetPopupEntry(dia, DSMO_WAVEFORMLOC, 0);   break;
		case WAVEPLACETHOR:  DiaSetPopupEntry(dia, DSMO_WAVEFORMLOC, 1);   break;
		case WAVEPLACETVER:  DiaSetPopupEntry(dia, DSMO_WAVEFORMLOC, 2);   break;
	}
	if (simals_trace_size == 0)
	{
		var = getval((INTBIG)sim_tool, VTOOL, VINTEGER, x_("SIM_als_num_events"));
		if (var == NOVARIABLE) simals_trace_size = DEFAULT_TRACE_SIZE; else
			simals_trace_size = var->addr;
	}
	esnprintf(line, 20, x_("%ld"), simals_trace_size);
	DiaSetText(dia, DSMO_MAXEVENTS, line);
	infstr = initinfstr();
	var = getval((INTBIG)sim_tool, VTOOL, VSTRING, x_("SIM_irsim_parameter_file"));
	if (var == NOVARIABLE)
		formatinfstr(infstr, x_("%s%s"), el_libdir, DEFIRSIMPARAMFILE); else
			addstringtoinfstr(infstr, (CHAR *)var->addr);
	(void)allocstring(&iniparamfile, returninfstr(infstr), el_tempcluster);
	DiaSetText(dia, DSMO_IRSIMPARMFILE, iniparamfile);
	for(i=0; i<20; i++)
	{
		(void)ecolorname(sim_window_colorvaluelist[i], &colorname, &colorsymbol);
		newlang[i] = TRANSLATE(colorname);
	}
	sim_optionssetcolorindia(dia, DSMO_COLORLOW, LOGIC_LOW, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORHIGH, LOGIC_HIGH, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORX, LOGIC_X, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORZ, LOGIC_Z, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORSTR0, OFF_STRENGTH, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORSTR1, NODE_STRENGTH, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORSTR2, GATE_STRENGTH, newlang);
	sim_optionssetcolorindia(dia, DSMO_COLORSTR3, VDD_STRENGTH, newlang);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DSMO_RESIMULATE || itemHit == DSMO_AUTOADVANCE ||
			itemHit == DSMO_MULTISTATE || itemHit == DSMO_WAVEFORMWIND ||
			itemHit == DSMO_IRSIMSHOWCMDS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DSMO_IRSIMPARAQUI || itemHit == DSMO_IRSIMPARALOC ||
			itemHit == DSMO_IRSIMPARAFULL)
		{
			DiaSetControl(dia, DSMO_IRSIMPARAQUI, 0);
			DiaSetControl(dia, DSMO_IRSIMPARALOC, 0);
			DiaSetControl(dia, DSMO_IRSIMPARAFULL, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DSMO_IRSIMSETPFILE)
		{
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("irsimparam/"));
			addstringtoinfstr(infstr, _("IRSIM Parameter File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, newlang);
			el_pleasestop = oldplease;
			if (i != 0 && newlang[0][0] != 0) DiaSetText(dia, DSMO_IRSIMPARMFILE, newlang[0]);
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		if (DiaGetControl(dia, DSMO_RESIMULATE) != 0) newnoupdate = 0; else
			newnoupdate = 1;
		newwindowstate = 0;
		if (DiaGetControl(dia, DSMO_AUTOADVANCE) != 0) newwindowstate |= ADVANCETIME;
		if (DiaGetControl(dia, DSMO_MULTISTATE) != 0) newwindowstate |= FULLSTATE;
		if (DiaGetControl(dia, DSMO_WAVEFORMWIND) != 0) newwindowstate |= SHOWWAVEFORM;
		switch (DiaGetPopupEntry(dia, DSMO_BUSBASE))
		{
			case 0: newwindowstate |= BUSBASE2;    break;
			case 1: newwindowstate |= BUSBASE8;    break;
			case 2: newwindowstate |= BUSBASE10;   break;
			case 3: newwindowstate |= BUSBASE16;   break;
		}
		switch (DiaGetPopupEntry(dia, DSMO_WAVEFORMLOC))
		{
			case 0: newwindowstate |= WAVEPLACECAS;    break;
			case 1: newwindowstate |= WAVEPLACETHOR;   break;
			case 2: newwindowstate |= WAVEPLACETVER;   break;
		}
		switch (DiaGetPopupEntry(dia, DSMO_SIMENGINE))
		{
			case 0: newwindowstate |= SIMENGINEALS;    break;
			case 1: newwindowstate |= SIMENGINEIRSIM;  break;
		}
		if (newnoupdate != noupdate)
			(void)setval((INTBIG)sim_tool, VTOOL, x_("SIM_als_no_update"),
				newnoupdate, VINTEGER);
		if (newwindowstate != sim_window_state)
			sim_window_setstate(newwindowstate);

		newirsimstate = 0;
		if (DiaGetControl(dia, DSMO_IRSIMPARALOC) != 0) newirsimstate |= IRSIMPARALOCAL; else
		if (DiaGetControl(dia, DSMO_IRSIMPARAFULL) != 0) newirsimstate |= IRSIMPARAFULL;
		if (DiaGetControl(dia, DSMO_IRSIMSHOWCMDS) != 0) newirsimstate |= IRSIMSHOWCOMMANDS;
		if (newirsimstate != irsimstate)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_irsim_statekey,
				newirsimstate, VINTEGER);
		pt = DiaGetText(dia, DSMO_IRSIMPARMFILE);
		if (estrcmp(pt, iniparamfile) != 0)
			(void)setval((INTBIG)sim_tool, VTOOL, x_("SIM_irsim_parameter_file"),
				(INTBIG)pt, VSTRING);
		newtracesize = eatoi(DiaGetText(dia, DSMO_MAXEVENTS));
		if (newtracesize != simals_trace_size)
		{
			newtrakroot = (TRAKPTR)simals_alloc_mem((INTBIG)(newtracesize * sizeof(TRAK)));
			if (newtrakroot != 0)
			{
				if (simals_trakroot != 0) efree((CHAR *)simals_trakroot);
				simals_trakroot = newtrakroot;
				simals_trace_size = newtracesize;
				(void)setval((INTBIG)sim_tool, VTOOL, x_("SIM_als_num_events"),
					newtracesize, VINTEGER);
			}
		}
		sim_optionsgetcolorfromdia(dia, DSMO_COLORLOW, LOGIC_LOW);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORHIGH, LOGIC_HIGH);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORX, LOGIC_X);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORZ, LOGIC_Z);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORSTR0, OFF_STRENGTH);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORSTR1, NODE_STRENGTH);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORSTR2, GATE_STRENGTH);
		sim_optionsgetcolorfromdia(dia, DSMO_COLORSTR3, VDD_STRENGTH);
	}
	efree((CHAR *)iniparamfile);
	DiaDoneDialog(dia);
}

void sim_optionssetcolorindia(void *dia, INTBIG entry, INTBIG signal, char **newlang)
{
	REGISTER INTBIG i, index;

	DiaSetPopup(dia, entry, 20, newlang);
	index = sim_window_getdisplaycolor(signal);
	for(i=0; i<20; i++) if (index == sim_window_colorvaluelist[i]) break;
	if (i < 20) DiaSetPopupEntry(dia, entry, i);
}

void sim_optionsgetcolorfromdia(void *dia, INTBIG entry, INTBIG signal)
{
	REGISTER INTBIG i;

	i = DiaGetPopupEntry(dia, entry);
	sim_window_setdisplaycolor(signal, sim_window_colorvaluelist[i]);
}

/* ALS Clock */
static DIALOGITEM sim_alsclockdialogitems[] =
{
 /*  1 */ {0, {8,320,32,384}, BUTTON, N_("OK")},
 /*  2 */ {0, {40,320,64,384}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {16,8,32,101}, RADIO, N_("Frequency:")},
 /*  4 */ {0, {40,8,56,101}, RADIO, N_("Period:")},
 /*  5 */ {0, {80,8,96,101}, RADIO, N_("Custom:")},
 /*  6 */ {0, {28,208,44,283}, EDITTEXT, x_("")},
 /*  7 */ {0, {28,112,44,199}, MESSAGE, N_("Freq/Period:")},
 /*  8 */ {0, {112,236,128,384}, RADIO, N_("Normal (gate) strength")},
 /*  9 */ {0, {236,232,252,369}, RADIO, N_("Undefined Phase")},
 /* 10 */ {0, {112,8,128,151}, MESSAGE, N_("Random Distribution:")},
 /* 11 */ {0, {112,160,128,223}, EDITTEXT, x_("")},
 /* 12 */ {0, {136,236,152,384}, RADIO, N_("Strong (VDD) strength")},
 /* 13 */ {0, {88,236,104,384}, RADIO, N_("Weak (node) strength")},
 /* 14 */ {0, {212,232,228,328}, RADIO, N_("High Phase")},
 /* 15 */ {0, {292,272,308,363}, BUTTON, N_("Delete Phase")},
 /* 16 */ {0, {164,8,279,221}, SCROLL|INACTIVE, x_("")},
 /* 17 */ {0, {164,224,180,288}, MESSAGE, N_("Duration:")},
 /* 18 */ {0, {264,272,280,363}, BUTTON, N_("Add Phase")},
 /* 19 */ {0, {292,8,308,197}, MESSAGE, N_("Phase Cycles (0 for infinite):")},
 /* 20 */ {0, {292,200,308,247}, EDITTEXT, x_("")},
 /* 21 */ {0, {164,296,180,369}, EDITTEXT, x_("")},
 /* 22 */ {0, {148,8,164,99}, MESSAGE, N_("Phase List:")},
 /* 23 */ {0, {188,232,204,327}, RADIO, N_("Low Phase")},
 /* 24 */ {0, {72,8,73,384}, DIVIDELINE, x_("")}
};
static DIALOG sim_alsclockdialog = {{50,75,367,468}, N_("Clock Specification"), 0, 24, sim_alsclockdialogitems, 0, 0};

/* special items for the "als clock" dialog: */
#define DALC_FREQUENCY       3		/* Frequency (radio) */
#define DALC_PERIOD          4		/* Period (radio) */
#define DALC_CUSTOM          5		/* Custom (radio) */
#define DALC_FREQPERIOD      6		/* Frequency/Period (edit text) */
#define DALC_FREQPERIOD_L    7		/* Frequency/Period label (stat text) */
#define DALC_STRENGTHNORM    8		/* Normal strength (radio) */
#define DALC_PHASEUNDEF      9		/* Undefined phase (radio) */
#define DALC_DISTRIBUTION_L 10		/* Random distribution label (stat text) */
#define DALC_DISTRIBUTION   11		/* Random distribution (edit text) */
#define DALC_STRENGTHSTRONG 12		/* Strong strength (radio) */
#define DALC_STRENGTHWEAK   13		/* Weak strength (radio) */
#define DALC_PHASEHIGH      14		/* High phase (radio) */
#define DALC_DELPHASE       15		/* Delete a phase (button) */
#define DALC_PHASELIST      16		/* List of phases (scroll) */
#define DALC_DURATION_L     17		/* Duration label (stat text) */
#define DALC_ADDPHASE       18		/* Add a phase (button) */
#define DALC_CYCLES_L       19		/* Number of cycles label (stat text) */
#define DALC_CYCLES         20		/* Number of cycles (edit text) */
#define DALC_DURATION       21		/* Duration (edit text) */
#define DALC_PHASELIST_L    22		/* Phases list label (stat text) */
#define DALC_PHASELOW       23		/* Low phase (radio) */

#define MAXPHASES 20
INTBIG sim_alsclockdlog(CHAR *paramstart[])
{
	INTBIG itemHit, i, j;
	INTBIG retval;
	CHAR *line;
	float durval[MAXPHASES];
	INTBIG levval[MAXPHASES], phases;
	static CHAR *retdur[MAXPHASES];
	static INTBIG first = 0;
	REGISTER void *dia;

	/* display the ALS clock dialog box */
	dia = DiaInitDialog(&sim_alsclockdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DALC_PHASELIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1, 0);

	/* setup complex clock specification */
	DiaSetText(dia, DALC_DISTRIBUTION, x_("0.0"));	/* random */
	DiaSetControl(dia, DALC_STRENGTHNORM, 1);	/* gate strength */
	DiaSetControl(dia, DALC_PHASEHIGH, 1);		/* level */
	DiaSetText(dia, DALC_CYCLES, x_("0"));			/* infinite cycles */

	/* now disable complex clock specification */
	DiaSetControl(dia, DALC_FREQUENCY, 1);		/* frequency */
	DiaDimItem(dia, DALC_DISTRIBUTION_L);		/* random */
	DiaDimItem(dia, DALC_DISTRIBUTION);
	DiaNoEditControl(dia, DALC_DISTRIBUTION);
	DiaDimItem(dia, DALC_STRENGTHWEAK);			/* strength */
	DiaDimItem(dia, DALC_STRENGTHNORM);
	DiaDimItem(dia, DALC_STRENGTHSTRONG);
	DiaDimItem(dia, DALC_PHASELOW);				/* level */
	DiaDimItem(dia, DALC_PHASEHIGH);
	DiaDimItem(dia, DALC_PHASEUNDEF);
	DiaDimItem(dia, DALC_DURATION_L);			/* duration */
	DiaDimItem(dia, DALC_DURATION);
	DiaNoEditControl(dia, DALC_DURATION);
	DiaDimItem(dia, DALC_ADDPHASE);				/* add/delete */
	DiaDimItem(dia, DALC_DELPHASE);
	DiaDimItem(dia, DALC_PHASELIST_L);			/* list header */
	DiaDimItem(dia, DALC_CYCLES_L);				/* cycles */
	DiaDimItem(dia, DALC_CYCLES);
	DiaNoEditControl(dia, DALC_CYCLES);
	DiaUnDimItem(dia, DALC_FREQPERIOD_L);		/* freq/per */
	DiaUnDimItem(dia, DALC_FREQPERIOD);
	DiaEditControl(dia, DALC_FREQPERIOD);

	/* loop until done */
	phases = 0;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DALC_FREQUENCY || itemHit == DALC_PERIOD || itemHit == DALC_CUSTOM)
		{
			DiaSetControl(dia, DALC_FREQUENCY, 0);
			DiaSetControl(dia, DALC_PERIOD, 0);
			DiaSetControl(dia, DALC_CUSTOM, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DALC_CUSTOM)
			{
				DiaUnDimItem(dia, DALC_DISTRIBUTION_L);		/* random */
				DiaUnDimItem(dia, DALC_DISTRIBUTION);
				DiaEditControl(dia, DALC_DISTRIBUTION);
				DiaUnDimItem(dia, DALC_STRENGTHWEAK);		/* strength */
				DiaUnDimItem(dia, DALC_STRENGTHNORM);
				DiaUnDimItem(dia, DALC_STRENGTHSTRONG);
				DiaUnDimItem(dia, DALC_PHASELOW);			/* level */
				DiaUnDimItem(dia, DALC_PHASEHIGH);
				DiaUnDimItem(dia, DALC_PHASEUNDEF);
				DiaUnDimItem(dia, DALC_DURATION_L);			/* duration */
				DiaUnDimItem(dia, DALC_DURATION);
				DiaEditControl(dia, DALC_DURATION);
				DiaUnDimItem(dia, DALC_ADDPHASE);			/* add/delete */
				DiaUnDimItem(dia, DALC_DELPHASE);
				DiaUnDimItem(dia, DALC_PHASELIST_L);			/* list header */
				DiaUnDimItem(dia, DALC_CYCLES_L);			/* cycles */
				DiaUnDimItem(dia, DALC_CYCLES);
				DiaEditControl(dia, DALC_CYCLES);
				DiaDimItem(dia, DALC_FREQPERIOD_L);			/* freq/per */
				DiaDimItem(dia, DALC_FREQPERIOD);
				DiaNoEditControl(dia, DALC_FREQPERIOD);
			} else
			{
				DiaDimItem(dia, DALC_DISTRIBUTION_L);		/* random */
				DiaDimItem(dia, DALC_DISTRIBUTION);
				DiaNoEditControl(dia, DALC_DISTRIBUTION);
				DiaDimItem(dia, DALC_STRENGTHWEAK);			/* strength */
				DiaDimItem(dia, DALC_STRENGTHNORM);
				DiaDimItem(dia, DALC_STRENGTHSTRONG);
				DiaDimItem(dia, DALC_PHASELOW);				/* level */
				DiaDimItem(dia, DALC_PHASEHIGH);
				DiaDimItem(dia, DALC_PHASEUNDEF);
				DiaDimItem(dia, DALC_DURATION_L);			/* duration */
				DiaDimItem(dia, DALC_DURATION);
				DiaNoEditControl(dia, DALC_DURATION);
				DiaDimItem(dia, DALC_ADDPHASE);				/* add/delete */
				DiaDimItem(dia, DALC_DELPHASE);
				DiaDimItem(dia, DALC_PHASELIST_L);			/* list header */
				DiaDimItem(dia, DALC_CYCLES_L);				/* cycles */
				DiaDimItem(dia, DALC_CYCLES);
				DiaNoEditControl(dia, DALC_CYCLES);
				DiaUnDimItem(dia, DALC_FREQPERIOD_L);		/* freq/per */
				DiaUnDimItem(dia, DALC_FREQPERIOD);
				DiaEditControl(dia, DALC_FREQPERIOD);
			}
			continue;
		}
		if (itemHit == DALC_STRENGTHWEAK || itemHit == DALC_STRENGTHNORM ||
			itemHit == DALC_STRENGTHSTRONG)
		{
			DiaSetControl(dia, DALC_STRENGTHWEAK, 0);
			DiaSetControl(dia, DALC_STRENGTHNORM, 0);
			DiaSetControl(dia, DALC_STRENGTHSTRONG, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DALC_PHASELOW || itemHit == DALC_PHASEHIGH ||
			itemHit == DALC_PHASEUNDEF)
		{
			DiaSetControl(dia, DALC_PHASELOW, 0);
			DiaSetControl(dia, DALC_PHASEHIGH, 0);
			DiaSetControl(dia, DALC_PHASEUNDEF, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DALC_ADDPHASE)
		{
			/* add phase */
			if (phases >= MAXPHASES) continue;
			line = DiaGetText(dia, DALC_DURATION);
			durval[phases] = (float)eatof(line);
			if (DiaGetControl(dia, DALC_PHASELOW) != 0) levval[phases] = 0; else
				if (DiaGetControl(dia, DALC_PHASEHIGH) != 0) levval[phases] = 1; else
					levval[phases] = 2;
			phases++;
			sim_writephases(durval, levval, phases, dia);
			continue;
		}
		if (itemHit == DALC_DELPHASE)
		{
			/* delete phase */
			if (phases <= 0) continue;
			j = DiaGetCurLine(dia, DALC_PHASELIST);
			for(i=j; i<phases; i++)
			{
				durval[i] = durval[i+1];
				levval[i] = levval[i+1];
			}
			phases--;
			sim_writephases(durval, levval, phases, dia);
			continue;
		}
	}
	retval = 0;
	if (itemHit == OK && DiaGetControl(dia, DALC_FREQUENCY) != 0)
	{
		paramstart[0] = x_("frequency");
		paramstart[1] = us_putintoinfstr(DiaGetText(dia, DALC_FREQPERIOD));
		retval = 2;
	} else if (itemHit == OK && DiaGetControl(dia, DALC_PERIOD) != 0)
	{
		paramstart[0] = x_("period");
		paramstart[1] = us_putintoinfstr(DiaGetText(dia, DALC_FREQPERIOD));
		retval = 2;
	} else if (itemHit == OK && DiaGetControl(dia, DALC_CUSTOM) != 0)
	{
		paramstart[0] = x_("custom");
		paramstart[1] = us_putintoinfstr(DiaGetText(dia, DALC_DISTRIBUTION));
		if (DiaGetControl(dia, DALC_STRENGTHWEAK) != 0) paramstart[2] = x_("1"); else
			if (DiaGetControl(dia, DALC_STRENGTHNORM) != 0) paramstart[2] = x_("2"); else
				paramstart[2] = x_("3");
		paramstart[3] = us_putintoinfstr(DiaGetText(dia, DALC_CYCLES));
		retval = 4;
		if (first == 0) for(i=0; i<MAXPHASES; i++) retdur[i] = 0;
		first++;
		for(i=0; i<phases; i++)
		{
			if (retdur[i] != 0) efree(retdur[i]);
			retdur[i] = (CHAR *)emalloc(50 * SIZEOFCHAR, sim_tool->cluster);
			if (retdur[i] == 0) break;
			(void)esnprintf(retdur[i], 50, x_("%e"), durval[i]);
			paramstart[retval++] = (CHAR *)(levval[i]==0 ? x_("low") : (levval[i]==1 ? x_("high") : x_("undefined")));
			paramstart[retval++] = retdur[i];
		}
	}
	DiaDoneDialog(dia);
	return(retval);
}

void sim_writephases(float durval[], INTBIG levval[], INTBIG phases, void *dia)
{
	INTBIG i;
	CHAR lne[50];

	DiaLoadTextDialog(dia, DALC_PHASELIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	for(i=0; i<phases; i++)
	{
		(void)esnprintf(lne, 50, _("%s for %e"),
			(levval[i]==0 ? _("low") : (levval[i]==1 ? _("high") : _("undefined"))), durval[i]);
		DiaStuffLine(dia, DALC_PHASELIST, lne);
	}
	if (phases <= 0) DiaSelectLine(dia, DALC_PHASELIST, -1); else DiaSelectLine(dia, DALC_PHASELIST, 0);
}

/* SPICE Options */
static DIALOGITEM sim_spiceoptdialogitems[] =
{
 /*  1 */ {0, {48,420,72,480}, BUTTON, N_("OK")},
 /*  2 */ {0, {8,420,32,480}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {104,8,120,224}, POPUP, x_("")},
 /*  4 */ {0, {8,120,24,223}, POPUP, x_("")},
 /*  5 */ {0, {588,216,604,375}, RADIO, N_("Use Model from File:")},
 /*  6 */ {0, {564,216,580,426}, RADIO, N_("Derive Model from Circuitry")},
 /*  7 */ {0, {32,120,48,164}, POPUP, x_("1")},
 /*  8 */ {0, {588,384,604,451}, BUTTON, N_("Browse")},
 /*  9 */ {0, {612,224,644,480}, EDITTEXT, x_("")},
 /* 10 */ {0, {32,240,48,372}, CHECK, N_("Use Node Names")},
 /* 11 */ {0, {8,240,24,359}, CHECK, N_("Use Parasitics")},
 /* 12 */ {0, {8,8,24,117}, MESSAGE, N_("SPICE Engine:")},
 /* 13 */ {0, {32,8,48,117}, MESSAGE, N_("SPICE Level:")},
 /* 14 */ {0, {352,8,368,192}, RADIO, N_("Use Built-in Header Cards")},
 /* 15 */ {0, {400,8,416,204}, RADIO, N_("Use Header Cards from File:")},
 /* 16 */ {0, {400,208,432,472}, EDITTEXT, x_("")},
 /* 17 */ {0, {448,8,464,132}, RADIO, N_("No Trailer Cards")},
 /* 18 */ {0, {496,8,512,204}, RADIO, N_("Include Trailer from File:")},
 /* 19 */ {0, {496,208,528,472}, EDITTEXT, x_("")},
 /* 20 */ {0, {208,8,224,281}, MESSAGE, x_("")},
 /* 21 */ {0, {416,36,432,204}, BUTTON, N_("Browse Header Card File")},
 /* 22 */ {0, {512,36,528,204}, BUTTON, N_("Browse Trailer Card File")},
 /* 23 */ {0, {564,8,644,206}, SCROLL, x_("")},
 /* 24 */ {0, {544,8,560,80}, MESSAGE, N_("For Cell:")},
 /* 25 */ {0, {352,196,368,480}, BUTTON, N_("Edit Built-in Headers for Technology/Level")},
 /* 26 */ {0, {200,8,201,480}, DIVIDELINE, x_("")},
 /* 27 */ {0, {536,8,537,480}, DIVIDELINE, x_("")},
 /* 28 */ {0, {232,72,312,266}, SCROLL, x_("")},
 /* 29 */ {0, {232,8,248,68}, MESSAGE, N_("Layer:")},
 /* 30 */ {0, {232,272,248,364}, MESSAGE, N_("Resistance:")},
 /* 31 */ {0, {256,272,272,364}, MESSAGE, N_("Capacitance:")},
 /* 32 */ {0, {280,272,296,364}, MESSAGE, N_("Edge Cap:")},
 /* 33 */ {0, {232,372,248,472}, EDITTEXT, x_("")},
 /* 34 */ {0, {256,372,272,472}, EDITTEXT, x_("")},
 /* 35 */ {0, {280,372,296,472}, EDITTEXT, x_("")},
 /* 36 */ {0, {320,8,336,124}, MESSAGE, N_("Min. Resistance:")},
 /* 37 */ {0, {320,128,336,216}, EDITTEXT, x_("")},
 /* 38 */ {0, {320,240,336,380}, MESSAGE, N_("Min. Capacitance:")},
 /* 39 */ {0, {320,384,336,472}, EDITTEXT, x_("")},
 /* 40 */ {0, {56,120,72,224}, POPUP, x_("")},
 /* 41 */ {0, {56,240,72,408}, CHECK, N_("Force Global VDD/GND")},
 /* 42 */ {0, {176,8,192,176}, MESSAGE, N_("SPICE primitive set:")},
 /* 43 */ {0, {176,180,192,408}, POPUP, x_("")},
 /* 44 */ {0, {128,60,144,480}, MESSAGE, x_("")},
 /* 45 */ {0, {128,8,144,56}, MESSAGE, N_("Run:")},
 /* 46 */ {0, {152,60,168,480}, EDITTEXT, x_("")},
 /* 47 */ {0, {152,8,168,56}, MESSAGE, N_("With:")},
 /* 48 */ {0, {80,240,96,408}, CHECK, N_("Use Cell Parameters")},
 /* 49 */ {0, {376,8,392,288}, RADIO, N_("Use Header Cards with extension:")},
 /* 50 */ {0, {376,296,392,392}, EDITTEXT, x_("")},
 /* 51 */ {0, {472,8,488,288}, RADIO, N_("Use Trailer Cards with extension:")},
 /* 52 */ {0, {472,296,488,392}, EDITTEXT, x_("")},
 /* 53 */ {0, {344,8,345,480}, DIVIDELINE, x_("")},
 /* 54 */ {0, {104,240,120,456}, CHECK, N_("Write Trans Sizes in Lambda")},
 /* 55 */ {0, {56,8,72,117}, MESSAGE, N_("Output format:")}
};
static DIALOG sim_spiceoptdialog = {{50,75,703,565}, N_("SPICE Options"), 0, 55, sim_spiceoptdialogitems, 0, 0};

/* special items for the "SPICE options" dialog: */
#define DSPO_AFTERWRITING        3		/* Action after writing deck (popup) */
#define DSPO_SPICETYPE           4		/* Spice type (2, 3, H, P) (popup) */
#define DSPO_MODELSFROMFILE      5		/* Use models from file (radio) */
#define DSPO_MODELSFROMCIRC      6		/* Derive models from circuitry (radio) */
#define DSPO_SPICELEVEL          7		/* Spice level (popup) */
#define DSPO_SETMODELFILE        8		/* Set model file (button) */
#define DSPO_MODELFILE           9		/* Model file (edit text) */
#define DSPO_NODENAMES          10		/* Use node names (check) */
#define DSPO_PARASITICS         11		/* Use parasitics (check) */
#define DSPO_USEOWNHEADERS      14		/* Use built-in header cards (radio) */
#define DSPO_USEFILEHEADERS     15		/* Use header cards from file (radio) */
#define DSPO_HEADERFILE         16		/* Header card file (edit text) */
#define DSPO_NOTRAILERS         17		/* No trailer cards (radio) */
#define DSPO_FILETRAILERS       18		/* Trailer cards from file (radio) */
#define DSPO_TRAILERFILE        19		/* Trailer card file (edit text) */
#define DSPO_TECHNAME           20		/* Technology name (stat text) */
#define DSPO_SETHEADERFILE      21		/* Set header card file (button) */
#define DSPO_SETTRAILERFILE     22		/* Set trailer card file (button) */
#define DSPO_CELLLIST           23		/* Cell list (scroll) */
#define DSPO_EDITHEADERS        25		/* Edit built-in header cards (button) */
#define DSPO_LAYERLIST          28		/* Layer list (scroll) */
#define DSPO_LAYERRESISTANCE    33		/* Layer resistance (edit text) */
#define DSPO_LAYERCAPACITANCE   34		/* Layer capacitance (edit text) */
#define DSPO_LAYERECAPACITANCE  35		/* Layer edge capacitance (edit text) */
#define DSPO_MINRESISTANCE      37		/* Minimum resistance (edit text) */
#define DSPO_MINCAPACITANCE     39		/* Minimum capacitance (edit text) */
#define DSPO_OUTPUTFORMAT       40		/* Form of output (popup) */
#define DSPO_GLOBALPG           41		/* Use global Power and Ground (check) */
#define DSPO_PRIMS              43		/* Name of SPICE primitive set (popup) */
#define DSPO_RUNCOMMAND         44		/* SPICE command to execute (stat text) */
#define DSPO_RUNCOMMAND_L       45		/* SPICE command to execute label (stat text) */
#define DSPO_RUNARGS            46		/* SPICE command arguments (edit text) */
#define DSPO_RUNARGS_L          47		/* SPICE command arguments label (stat text) */
#define DSPO_USECELLPARAMS      48		/* Use Cell Parameters (check) */
#define DSPO_USEEXTHEADERS      49		/* Use header cards with extension (button) */
#define DSPO_EXTHEADEREXT       50		/* Extension to use for header cards (edit text) */
#define DSPO_EXTTRAILERFILE     51		/* Use trailer cards with extension (button) */
#define DSPO_EXTTRAILEREXT      52		/* Extension to use for trailer cards (edit text) */
#define DSPO_WRITELAMBDA		54		/* Write transistors sizes in lambdas */

void sim_spicedlog(void)
{
	REGISTER INTBIG itemHit, i, j, k, l, spice_state, newspice_state, oldplease,
		level, execute, spicepartcount;
	INTBIG dummy;
	BOOLEAN reschanged, capchanged, ecapchanged, minreschanged, mincapchanged, canexecute;
	CHAR *subparams[3], *pt, header[200], *dummyfile[1], floatconv[30], *newlang[6],
		**filelist, **spicepartlist, *runargs;
	static CHAR *formatnames[6] = {N_("Spice 2"), N_("Spice 3"), N_("HSpice"), N_("PSpice"),
		N_("Gnucap"), N_("SmartSpice")};
	static CHAR *outputformats[3] = {N_("Standard"), N_("Raw"), N_("Raw/Smart")};
	static CHAR *levelnames[3] = {x_("1"), x_("2"), x_("3")};
	static CHAR *executenames[5] = {N_("Don't Run SPICE"), N_("Run SPICE"), N_("Run SPICE Quietly"),
		N_("Run SPICE, Read Output"), N_("Run SPICE Quietly, Read Output")};
	static CHAR qual[50];
	float *res, *cap, *ecap, minres, mincap, v;
	REGISTER VARIABLE *var, *varres, *varcap, *varecap;
	REGISTER NODEPROTO *np;
	REGISTER WINDOWPART *w;
	REGISTER TECHNOLOGY *curtech;
	REGISTER EDITOR *ed;
	REGISTER void *infstr, *dia;

	/* Display the SPICE options dialog box */
	dia = DiaInitDialog(&sim_spiceoptdialog);
	if (dia == 0) return;
	for(i=0; i<5; i++) newlang[i] = TRANSLATE(executenames[i]);
	DiaSetPopup(dia, DSPO_AFTERWRITING, 5, newlang);
	for(i=0; i<6; i++) newlang[i] = TRANSLATE(formatnames[i]);
	DiaSetPopup(dia, DSPO_SPICETYPE, 6, newlang);
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(levelnames[i]);
	DiaSetPopup(dia, DSPO_SPICELEVEL, 3, newlang);
	for(i=0; i<3; i++) newlang[i] = TRANSLATE(outputformats[i]);
	DiaSetPopup(dia, DSPO_OUTPUTFORMAT, 3, newlang);

	/* see which technology to describe */
	curtech = defschematictechnology(el_curtech);

	/* state of the execute flag */
	canexecute = graphicshas(CANRUNPROCESS);
	execute = 3;
	if (canexecute)
	{
		DiaUnDimItem(dia, DSPO_AFTERWRITING);
		DiaUnDimItem(dia, DSPO_RUNCOMMAND);
		DiaUnDimItem(dia, DSPO_RUNCOMMAND_L);
		DiaUnDimItem(dia, DSPO_RUNARGS);
		DiaUnDimItem(dia, DSPO_RUNARGS_L);
		var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_dontrunkey);
		if (var != NOVARIABLE)
		{
			switch (var->addr)
			{
				case SIMRUNNO:        execute = 0;   break;
				case SIMRUNYES:       execute = 1;   break;
				case SIMRUNYESQ:      execute = 2;   break;
				case SIMRUNYESPARSE:  execute = 3;   break;
				case SIMRUNYESQPARSE: execute = 4;   break;
			}
		}
		DiaSetPopupEntry(dia, DSPO_AFTERWRITING, execute);
		pt = egetenv(x_("ELECTRIC_SPICELOC"));
		if (pt == NULL) pt = SPICELOC;
		DiaSetText(dia, DSPO_RUNCOMMAND, pt);
		var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_spice_runargskey);
		if (var == NOVARIABLE) runargs = x_("%.spi"); else runargs = (CHAR *)var->addr;
		DiaSetText(dia, DSPO_RUNARGS, runargs);
	} else
	{
		DiaDimItem(dia, DSPO_AFTERWRITING);
		DiaDimItem(dia, DSPO_RUNCOMMAND);
		DiaDimItem(dia, DSPO_RUNCOMMAND_L);
		DiaDimItem(dia, DSPO_RUNARGS);
		DiaDimItem(dia, DSPO_RUNARGS_L);
	}

	/* get the spice level */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_levelkey);
	if (var == NOVARIABLE) level = 1; else level = var->addr;
	DiaSetPopupEntry(dia, DSPO_SPICELEVEL, level-1);

	/* get miscellaneous state */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
	if (var != NOVARIABLE) spice_state = var->addr; else spice_state = 0;
	if ((spice_state&SPICENODENAMES) != 0) DiaSetControl(dia, DSPO_NODENAMES, 1);
	if ((spice_state&SPICERESISTANCE) != 0) DiaSetControl(dia, DSPO_PARASITICS, 1);
	if ((spice_state&SPICEGLOBALPG) != 0) DiaSetControl(dia, DSPO_GLOBALPG, 1);
	if ((spice_state&SPICECELLPARAM) != 0) DiaSetControl(dia, DSPO_USECELLPARAMS, 1);
	if ((spice_state&SPICEUSELAMBDAS) != 0) DiaSetControl(dia, DSPO_WRITELAMBDA, 1);
	switch (spice_state & SPICETYPE)
	{
		case SPICE2:
			DiaSetPopupEntry(dia, DSPO_SPICETYPE, 0);
			DiaSetControl(dia, DSPO_NODENAMES, 0);
			DiaDimItem(dia, DSPO_NODENAMES);
			DiaSetControl(dia, DSPO_GLOBALPG, 0);
			DiaDimItem(dia, DSPO_GLOBALPG);
			break;
		case SPICE3:
			DiaSetPopupEntry(dia, DSPO_SPICETYPE, 1);
			DiaUnDimItem(dia, DSPO_NODENAMES);
			DiaSetControl(dia, DSPO_GLOBALPG, 0);
			DiaDimItem(dia, DSPO_GLOBALPG);
			break;
		case SPICEHSPICE:
			DiaSetPopupEntry(dia, DSPO_SPICETYPE, 2);
			DiaUnDimItem(dia, DSPO_NODENAMES);
			DiaUnDimItem(dia, DSPO_GLOBALPG);
			break;
		case SPICEPSPICE:
			DiaSetPopupEntry(dia, DSPO_SPICETYPE, 3);
			DiaUnDimItem(dia, DSPO_NODENAMES);
			DiaUnDimItem(dia, DSPO_GLOBALPG);
			break;
		case SPICEGNUCAP:
			DiaSetPopupEntry(dia, DSPO_SPICETYPE, 4);
			DiaSetControl(dia, DSPO_NODENAMES, 0);
			DiaDimItem(dia, DSPO_NODENAMES);
			DiaSetControl(dia, DSPO_GLOBALPG, 0);
			DiaDimItem(dia, DSPO_GLOBALPG);
			break;
		case SPICESMARTSPICE:
			DiaSetPopupEntry(dia, DSPO_SPICETYPE, 5);
			DiaUnDimItem(dia, DSPO_NODENAMES);
			DiaUnDimItem(dia, DSPO_GLOBALPG);
			break;
	}

	/* get the output format */
	switch (spice_state&SPICEOUTPUT)
	{
		case SPICEOUTPUTNORM:     DiaSetPopupEntry(dia, DSPO_OUTPUTFORMAT, 0);  break;
		case SPICEOUTPUTRAW:      DiaSetPopupEntry(dia, DSPO_OUTPUTFORMAT, 1);  break;
		case SPICEOUTPUTRAWSMART: DiaSetPopupEntry(dia, DSPO_OUTPUTFORMAT, 2);  break;
	}

	/* get model header cards */
	DiaDimItem(dia, DSPO_HEADERFILE);
	DiaDimItem(dia, DSPO_EXTHEADEREXT);
	infstr = initinfstr();
	formatinfstr(infstr, _("For technology: %s"), curtech->techname);
	DiaSetText(dia, DSPO_TECHNAME, returninfstr(infstr));
	var = getval((INTBIG)curtech, VTECHNOLOGY, VSTRING, x_("SIM_spice_model_file"));
	if (var == NOVARIABLE) DiaSetControl(dia, DSPO_USEOWNHEADERS, 1); else
	{
		pt = (CHAR *)var->addr;
		if (estrncmp(pt, x_(":::::"), 5) == 0)
		{
			DiaUnDimItem(dia, DSPO_EXTHEADEREXT);
			DiaSetText(dia, DSPO_EXTHEADEREXT, &pt[5]);
			DiaSetControl(dia, DSPO_USEEXTHEADERS, 1);
		} else
		{
			DiaUnDimItem(dia, DSPO_HEADERFILE);
			DiaSetText(dia, DSPO_HEADERFILE, pt);
			DiaSetControl(dia, DSPO_USEFILEHEADERS, 1);
		}
	}

	/* get trailer cards */
	DiaDimItem(dia, DSPO_TRAILERFILE);
	DiaDimItem(dia, DSPO_EXTTRAILEREXT);
	var = getval((INTBIG)curtech, VTECHNOLOGY, VSTRING, x_("SIM_spice_trailer_file"));
	if (var == NOVARIABLE) DiaSetControl(dia, DSPO_NOTRAILERS, 1); else
	{
		pt = (CHAR *)var->addr;
		if (estrncmp(pt, x_(":::::"), 5) == 0)
		{
			DiaUnDimItem(dia, DSPO_EXTTRAILEREXT);
			DiaSetText(dia, DSPO_EXTTRAILEREXT, &pt[5]);
			DiaSetControl(dia, DSPO_EXTTRAILERFILE, 1);
		} else
		{
			DiaUnDimItem(dia, DSPO_TRAILERFILE);
			DiaSetText(dia, DSPO_TRAILERFILE, pt);
			DiaSetControl(dia, DSPO_FILETRAILERS, 1);
		}
	}

	/* list of cells for model description files */
	DiaInitTextDialog(dia, DSPO_CELLLIST, topofcells, nextcells, DiaNullDlogDone,
		0, SCSELMOUSE|SCSELKEY|SCREPORT);
	for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		np->temp1 = 0;
		var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_spice_behave_file"));
		if (var != NOVARIABLE)
			(void)allocstring((CHAR **)&np->temp1, (CHAR *)var->addr, el_tempcluster);
	}
	pt = DiaGetScrollLine(dia, DSPO_CELLLIST, DiaGetCurLine(dia, DSPO_CELLLIST));
	DiaSetControl(dia, DSPO_MODELSFROMCIRC, 1);
	DiaSetControl(dia, DSPO_MODELSFROMFILE, 0);
	DiaDimItem(dia, DSPO_MODELFILE);
	np = getnodeproto(pt);
	if (np != NONODEPROTO)
	{
		var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_spice_behave_file"));
		if (var != NOVARIABLE)
		{
			DiaSetControl(dia, DSPO_MODELSFROMCIRC, 0);
			DiaSetControl(dia, DSPO_MODELSFROMFILE, 1);
			DiaUnDimItem(dia, DSPO_MODELFILE);
			DiaSetText(dia, DSPO_MODELFILE, (CHAR *)var->addr);
		}
	}

	/* setup the SPICE primitives */
	j = filesindirectory(el_libdir, &filelist);
	spicepartcount = 0;
	for(i=0; i<j; i++)
	{
		pt = filelist[i];
		l = estrlen(pt);
		if (namesamen(pt, x_("spiceparts"), 10) != 0) continue;
		if (namesame(&pt[l-4], x_(".mac")) != 0) continue;
		spicepartcount++;
	}
	if (spicepartcount > 0)
	{
		k = 0;
		spicepartlist = (CHAR **)emalloc(spicepartcount * (sizeof (CHAR *)), el_tempcluster);
		if (spicepartlist == 0) return;
		spicepartcount = 0;
		for(i=0; i<j; i++)
		{
			pt = filelist[i];
			l = estrlen(pt);
			if (namesamen(pt, x_("spiceparts"), 10) != 0) continue;
			if (namesame(&pt[l-4], x_(".mac")) != 0) continue;
			(void)allocstring(&spicepartlist[spicepartcount], pt, el_tempcluster);
			spicepartlist[spicepartcount][l-4] = 0;
			if (namesame(spicepartlist[spicepartcount], sim_spice_parts) == 0)
				k = spicepartcount;
			spicepartcount++;
		}
		DiaSetPopup(dia, DSPO_PRIMS, spicepartcount, spicepartlist);
		DiaSetPopupEntry(dia, DSPO_PRIMS, k);
	}

	/* list of layers in this technology, with resistance, capacitance, etc. */
	DiaInitTextDialog(dia, DSPO_LAYERLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone,
		-1, SCSELMOUSE|SCREPORT);
	varres = getval((INTBIG)curtech, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_resistance"));
	varcap = getval((INTBIG)curtech, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_capacitance"));
	varecap = getval((INTBIG)curtech, VTECHNOLOGY, VFLOAT|VISARRAY, x_("SIM_spice_edge_capacitance"));
	res = (float *)emalloc(curtech->layercount * (sizeof (float)), el_tempcluster);
	cap = (float *)emalloc(curtech->layercount * (sizeof (float)), el_tempcluster);
	ecap = (float *)emalloc(curtech->layercount * (sizeof (float)), el_tempcluster);
	for(i=0; i<curtech->layercount; i++)
	{
		DiaStuffLine(dia, DSPO_LAYERLIST, layername(curtech, i));
		res[i] = cap[i] = ecap[i] = 0.0;
		if (varres != NOVARIABLE && i < getlength(varres))
			res[i] = ((float *)varres->addr)[i];
		if (varcap != NOVARIABLE && i < getlength(varcap))
			cap[i] = ((float *)varcap->addr)[i];
		if (varecap != NOVARIABLE && i < getlength(varecap))
			ecap[i] = ((float *)varecap->addr)[i];
	}
	DiaSelectLine(dia, DSPO_LAYERLIST, 0);
	esnprintf(floatconv, 30, x_("%g"), res[0]);    DiaSetText(dia, DSPO_LAYERRESISTANCE, floatconv);
	esnprintf(floatconv, 30, x_("%g"), cap[0]);    DiaSetText(dia, DSPO_LAYERCAPACITANCE, floatconv);
	esnprintf(floatconv, 30, x_("%g"), ecap[0]);   DiaSetText(dia, DSPO_LAYERECAPACITANCE, floatconv);
	reschanged = capchanged = ecapchanged = minreschanged = mincapchanged = FALSE;

	/* get minimum resistance and capacitances */
	var = getval((INTBIG)curtech, VTECHNOLOGY, VFLOAT, x_("SIM_spice_min_resistance"));
	if (var == NOVARIABLE) minres = 0.0; else
		minres = castfloat(var->addr);
	esnprintf(floatconv, 30, x_("%g"), minres);    DiaSetText(dia, DSPO_MINRESISTANCE, floatconv);
	var = getval((INTBIG)curtech, VTECHNOLOGY, VFLOAT, x_("SIM_spice_min_capacitance"));
	if (var == NOVARIABLE) mincap = 0.0; else
		mincap = castfloat(var->addr);
	esnprintf(floatconv, 30, x_("%g"), mincap);    DiaSetText(dia, DSPO_MINCAPACITANCE, floatconv);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL || itemHit == DSPO_EDITHEADERS) break;

		if (itemHit == DSPO_SPICETYPE)
		{
			/* file format button */
			i = DiaGetPopupEntry(dia, DSPO_SPICETYPE);
			if (i != 0/*Spice2*/ && i != 4/*Gnucap*/) DiaUnDimItem(dia, DSPO_NODENAMES); else
			{
				DiaSetControl(dia, DSPO_NODENAMES, 0);
				DiaDimItem(dia, DSPO_NODENAMES);
			}
			if (i != 0/*Spice2*/ && i != 1/*Spice3*/ && i != 4/*Gnucap*/) DiaUnDimItem(dia, DSPO_GLOBALPG); else 
			{
				DiaSetControl(dia, DSPO_GLOBALPG, 0);
				DiaDimItem(dia, DSPO_GLOBALPG);
			}
			continue;
		}

		if (itemHit == DSPO_NODENAMES || itemHit == DSPO_PARASITICS ||
			itemHit == DSPO_GLOBALPG ||
			itemHit == DSPO_USECELLPARAMS || itemHit == DSPO_WRITELAMBDA)
		{
			/* check boxes */
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}

		if (itemHit == DSPO_USEOWNHEADERS || itemHit == DSPO_USEFILEHEADERS ||
			itemHit == DSPO_USEEXTHEADERS)
		{
			/* header model cards button */
			DiaSetControl(dia, DSPO_USEOWNHEADERS, 0);
			DiaSetControl(dia, DSPO_USEFILEHEADERS, 0);
			DiaSetControl(dia, DSPO_USEEXTHEADERS, 0);
			DiaSetControl(dia, itemHit, 1);
			DiaDimItem(dia, DSPO_HEADERFILE);
			DiaDimItem(dia, DSPO_EXTHEADEREXT);
			if (itemHit == DSPO_USEFILEHEADERS)
			{
				DiaUnDimItem(dia, DSPO_HEADERFILE);
			} else if (itemHit == DSPO_USEEXTHEADERS)
			{
				DiaUnDimItem(dia, DSPO_EXTHEADEREXT);
			}
			continue;
		}

		if (itemHit == DSPO_SETHEADERFILE)
		{
			/* select model cards */
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("spice/"));
			addstringtoinfstr(infstr, _("SPICE Model File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, subparams);
			el_pleasestop = oldplease;
			if (i != 0 && subparams[0][0] != 0)
			{
				DiaDimItem(dia, DSPO_EXTHEADEREXT);
				DiaUnDimItem(dia, DSPO_HEADERFILE);
				DiaSetText(dia, DSPO_HEADERFILE, subparams[0]);
				DiaSetControl(dia, DSPO_USEOWNHEADERS, 0);
				DiaSetControl(dia, DSPO_USEFILEHEADERS, 1);
				DiaSetControl(dia, DSPO_USEEXTHEADERS, 0);
			}
			continue;
		}

		if (itemHit == DSPO_NOTRAILERS || itemHit == DSPO_FILETRAILERS ||
			itemHit == DSPO_EXTTRAILERFILE)
		{
			/* trailer cards button */
			DiaSetControl(dia, DSPO_NOTRAILERS, 0);
			DiaSetControl(dia, DSPO_FILETRAILERS, 0);
			DiaSetControl(dia, DSPO_EXTTRAILERFILE, 0);
			DiaSetControl(dia, itemHit, 1);
			DiaDimItem(dia, DSPO_TRAILERFILE);
			DiaDimItem(dia, DSPO_EXTTRAILEREXT);
			if (itemHit == DSPO_FILETRAILERS)
			{
				DiaUnDimItem(dia, DSPO_TRAILERFILE);
			} else if (itemHit == DSPO_EXTTRAILERFILE)
			{
				DiaUnDimItem(dia, DSPO_EXTTRAILEREXT);
			}
			continue;
		}

		if (itemHit == DSPO_SETTRAILERFILE)
		{
			/* select trailer cards */
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("spice/"));
			addstringtoinfstr(infstr, _("SPICE Trailer File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, subparams);
			el_pleasestop = oldplease;
			if (i != 0 && subparams[0][0] != 0)
			{
				DiaUnDimItem(dia, DSPO_TRAILERFILE);
				DiaSetText(dia, DSPO_TRAILERFILE, subparams[0]);
				DiaDimItem(dia, DSPO_EXTTRAILEREXT);
				DiaSetControl(dia, DSPO_NOTRAILERS, 0);
				DiaSetControl(dia, DSPO_FILETRAILERS, 1);
				DiaSetControl(dia, DSPO_EXTTRAILERFILE, 0);
			}
			continue;
		}

		if (itemHit == DSPO_LAYERLIST)
		{
			/* selection of layers */
			i = DiaGetCurLine(dia, DSPO_LAYERLIST);
			if (i < 0 || i >= curtech->layercount) continue;
			esnprintf(floatconv, 30, x_("%g"), res[i]);   DiaSetText(dia, DSPO_LAYERRESISTANCE, floatconv);
			esnprintf(floatconv, 30, x_("%g"), cap[i]);   DiaSetText(dia, DSPO_LAYERCAPACITANCE, floatconv);
			esnprintf(floatconv, 30, x_("%g"), ecap[i]);   DiaSetText(dia, DSPO_LAYERECAPACITANCE, floatconv);
			continue;
		}

		if (itemHit == DSPO_LAYERRESISTANCE)
		{
			i = DiaGetCurLine(dia, DSPO_LAYERLIST);
			if (i < 0 || i >= curtech->layercount) continue;
			v = (float)eatof(DiaGetText(dia, DSPO_LAYERRESISTANCE));
			if (!floatsequal(res[i], v)) reschanged = TRUE;
			res[i] = v;
			continue;
		}
		if (itemHit == DSPO_LAYERCAPACITANCE)
		{
			i = DiaGetCurLine(dia, DSPO_LAYERLIST);
			if (i < 0 || i >= curtech->layercount) continue;
			v = (float)eatof(DiaGetText(dia, DSPO_LAYERCAPACITANCE));
			if (!floatsequal(cap[i], v)) capchanged = TRUE;
			cap[i] = v;
			continue;
		}
		if (itemHit == DSPO_LAYERECAPACITANCE)
		{
			i = DiaGetCurLine(dia, DSPO_LAYERLIST);
			if (i < 0 || i >= curtech->layercount) continue;
			v = (float)eatof(DiaGetText(dia, DSPO_LAYERECAPACITANCE));
			if (!floatsequal(ecap[i], v)) ecapchanged = TRUE;
			ecap[i] = v;
			continue;
		}
		if (itemHit == DSPO_MINRESISTANCE)
		{
			v = (float)eatof(DiaGetText(dia, DSPO_MINRESISTANCE));
			if (!floatsequal(v, minres)) minreschanged = TRUE;
			continue;
		}
		if (itemHit == DSPO_MINCAPACITANCE)
		{
			v = (float)eatof(DiaGetText(dia, DSPO_MINCAPACITANCE));
			if (!floatsequal(v, mincap)) mincapchanged = TRUE;
			continue;
		}

		if (itemHit == DSPO_CELLLIST)
		{
			/* selection of cells */
			DiaSetControl(dia, DSPO_MODELSFROMCIRC, 1);
			DiaSetControl(dia, DSPO_MODELSFROMFILE, 0);
			DiaSetText(dia, DSPO_MODELFILE, x_(""));
			pt = DiaGetScrollLine(dia, DSPO_CELLLIST, DiaGetCurLine(dia, DSPO_CELLLIST));
			np = getnodeproto(pt);
			if (np != NONODEPROTO && np->temp1 != 0)
			{
				DiaSetControl(dia, DSPO_MODELSFROMCIRC, 0);
				DiaSetControl(dia, DSPO_MODELSFROMFILE, 1);
				DiaSetText(dia, DSPO_MODELFILE, (CHAR *)np->temp1);
			}
			continue;
		}

		if (itemHit == DSPO_MODELFILE)
		{
			/* changing a cell's model file */
			pt = DiaGetScrollLine(dia, DSPO_CELLLIST, DiaGetCurLine(dia, DSPO_CELLLIST));
			np = getnodeproto(pt);
			if (np != NONODEPROTO)
			{
				if (np->temp1 != 0) efree((CHAR *)np->temp1);
				(void)allocstring((CHAR **)&np->temp1, DiaGetText(dia, DSPO_MODELFILE), el_tempcluster);
			}
			continue;
		}

		if (itemHit == DSPO_MODELSFROMCIRC || itemHit == DSPO_MODELSFROMFILE)
		{
			/* model for cell */
			pt = DiaGetScrollLine(dia, DSPO_CELLLIST, DiaGetCurLine(dia, DSPO_CELLLIST));
			np = getnodeproto(pt);
			DiaSetControl(dia, DSPO_MODELSFROMCIRC, 0);
			DiaSetControl(dia, DSPO_MODELSFROMFILE, 0);
			DiaSetControl(dia, itemHit, 1);
			if (itemHit == DSPO_MODELSFROMFILE)
			{
				DiaUnDimItem(dia, DSPO_MODELFILE);
				if (np != NONODEPROTO)
				{
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s.spi"), np->protoname);
					pt = returninfstr(infstr);
					if (np->temp1 != 0) efree((CHAR *)np->temp1);
					(void)allocstring((CHAR **)&np->temp1, pt, el_tempcluster);
					DiaSetText(dia, DSPO_MODELFILE, pt);
				}
			} else
			{
				DiaSetText(dia, DSPO_MODELFILE, x_(""));
				DiaDimItem(dia, DSPO_MODELFILE);
				if (np != NONODEPROTO)
				{
					if (np->temp1 != 0) efree((CHAR *)np->temp1);
					np->temp1 = 0;
				}
			}
			continue;
		}

		if (itemHit == DSPO_SETMODELFILE)
		{
			/* set model for cell */
			pt = DiaGetScrollLine(dia, DSPO_CELLLIST, DiaGetCurLine(dia, DSPO_CELLLIST));
			np = getnodeproto(pt);
			if (np == NONODEPROTO) continue;
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("spice/"));
			addstringtoinfstr(infstr, _("Cell's Model File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, subparams);
			el_pleasestop = oldplease;
			if (i != 0 && subparams[0][0] != 0)
			{
				DiaUnDimItem(dia, DSPO_MODELFILE);
				DiaSetText(dia, DSPO_MODELFILE, subparams[0]);
				if (np->temp1 != 0) efree((CHAR *)np->temp1);
				(void)allocstring((CHAR **)&np->temp1, subparams[0], el_tempcluster);
				DiaSetControl(dia, DSPO_MODELSFROMCIRC, 0);
				DiaSetControl(dia, DSPO_MODELSFROMFILE, 1);
			}
			continue;
		}
	}

	if (itemHit != CANCEL)  /* now update the values */
	{
		if (canexecute)
		{
			i = DiaGetPopupEntry(dia, DSPO_AFTERWRITING);
			if (i != execute)
			{
				switch (i)
				{
					case 0: execute = SIMRUNNO;          break;
					case 1: execute = SIMRUNYES;         break;
					case 2: execute = SIMRUNYESQ;        break;
					case 3: execute = SIMRUNYESPARSE;    break;
					case 4: execute = SIMRUNYESQPARSE;   break;
				}
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_dontrunkey, execute, VINTEGER);
			}
			pt = DiaGetText(dia, DSPO_RUNARGS);
			if (estrcmp(runargs, pt) != 0)
				(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_runargskey,
					(INTBIG)pt, VSTRING);
		}

		i = DiaGetPopupEntry(dia, DSPO_SPICELEVEL) + 1;
		if (i != level)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_levelkey, i, VINTEGER);

		newspice_state = 0;
		if (DiaGetControl(dia, DSPO_NODENAMES) != 0) newspice_state |= SPICENODENAMES;
		if (DiaGetControl(dia, DSPO_PARASITICS) != 0) newspice_state |= SPICERESISTANCE;
		if (DiaGetControl(dia, DSPO_GLOBALPG) != 0) newspice_state |= SPICEGLOBALPG;
		if (DiaGetControl(dia, DSPO_USECELLPARAMS) != 0) newspice_state |= SPICECELLPARAM;
		if (DiaGetControl(dia, DSPO_WRITELAMBDA) != 0) newspice_state |= SPICEUSELAMBDAS;
		switch (DiaGetPopupEntry(dia, DSPO_SPICETYPE))
		{
			case 0: newspice_state |= SPICE2;          break;
			case 1: newspice_state |= SPICE3;          break;
			case 2: newspice_state |= SPICEHSPICE;     break;
			case 3: newspice_state |= SPICEPSPICE;     break;
			case 4: newspice_state |= SPICEGNUCAP;     break;
			case 5: newspice_state |= SPICESMARTSPICE; break;
		}
		switch (DiaGetPopupEntry(dia, DSPO_OUTPUTFORMAT))
		{
			case 0: newspice_state |= SPICEOUTPUTNORM;       break;
			case 1: newspice_state |= SPICEOUTPUTRAW;        break;
			case 2: newspice_state |= SPICEOUTPUTRAWSMART;   break;
		}
		if (newspice_state != spice_state)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_statekey, newspice_state, VINTEGER);

		i = DiaGetPopupEntry(dia, DSPO_PRIMS);
		if (namesame(sim_spice_parts, spicepartlist[i]) != 0)
		{
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_spice_partskey,
				(INTBIG)spicepartlist[i], VSTRING);
		}

		var = getval((INTBIG)curtech, VTECHNOLOGY, VSTRING, x_("SIM_spice_model_file"));
		if (DiaGetControl(dia, DSPO_USEFILEHEADERS) != 0)
		{
			/* set model cards */
			pt = DiaGetText(dia, DSPO_HEADERFILE);
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
			{
				(void)setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_model_file"),
					(INTBIG)pt, VSTRING);
			}
		} else if (DiaGetControl(dia, DSPO_USEEXTHEADERS) != 0)
		{
			/* set model extension */
			infstr = initinfstr();
			formatinfstr(infstr, x_(":::::%s"), DiaGetText(dia, DSPO_EXTHEADEREXT));
			pt = returninfstr(infstr);
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
			{
				(void)setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_model_file"),
					(INTBIG)pt, VSTRING);
			}
		} else
		{
			/* remove model cards */
			if (var != NOVARIABLE)
				(void)delval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_model_file"));
		}

		var = getval((INTBIG)curtech, VTECHNOLOGY, VSTRING, x_("SIM_spice_trailer_file"));
		if (DiaGetControl(dia, DSPO_FILETRAILERS) != 0)
		{
			/* set trailer cards */
			pt = DiaGetText(dia, DSPO_TRAILERFILE);
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
			{
				(void)setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_trailer_file"),
					(INTBIG)pt, VSTRING);
			}
		} else if (DiaGetControl(dia, DSPO_EXTTRAILERFILE) != 0)
		{
			/* set trailer extension */
			infstr = initinfstr();
			formatinfstr(infstr, x_(":::::%s"), DiaGetText(dia, DSPO_EXTTRAILEREXT));
			pt = returninfstr(infstr);
			if (var == NOVARIABLE || estrcmp(pt, (CHAR *)var->addr) != 0)
			{
				(void)setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_trailer_file"),
					(INTBIG)pt, VSTRING);
			}
		} else
		{
			/* remove trailer cards */
			if (var != NOVARIABLE)
				(void)delval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_trailer_file"));
		}
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_spice_behave_file"));
			if (var != NOVARIABLE)
			{
				if (np->temp1 == 0)
					(void)delval((INTBIG)np, VNODEPROTO, x_("SIM_spice_behave_file")); else
				{
					if (estrcmp((CHAR *)np->temp1, (CHAR *)var->addr) != 0)
						(void)setval((INTBIG)np, VNODEPROTO, x_("SIM_spice_behave_file"), (INTBIG)np->temp1, VSTRING);
				}
			} else if (np->temp1 != 0)
				(void)setval((INTBIG)np, VNODEPROTO, x_("SIM_spice_behave_file"), (INTBIG)np->temp1, VSTRING);
			if (np->temp1 != 0) efree((CHAR *)np->temp1);
		}

		/* update layer resistance, capacitance, etc. */
		if (reschanged)
			setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_resistance"),
				(INTBIG)res, VFLOAT|VISARRAY|(curtech->layercount << VLENGTHSH));
		if (capchanged)
			setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_capacitance"),
				(INTBIG)cap, VFLOAT|VISARRAY|(curtech->layercount << VLENGTHSH));
		if (ecapchanged)
			setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_edge_capacitance"),
				(INTBIG)ecap, VFLOAT|VISARRAY|(curtech->layercount << VLENGTHSH));

		/* update minimum resistance and capacitance */
		if (minreschanged)
		{
			v = (float)eatof(DiaGetText(dia, DSPO_MINRESISTANCE));
			setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_min_resistance"),
				castint(v), VFLOAT);
		}
		if (mincapchanged)
		{
			v = (float)eatof(DiaGetText(dia, DSPO_MINCAPACITANCE));
			setval((INTBIG)curtech, VTECHNOLOGY, x_("SIM_spice_min_capacitance"),
				castint(v), VFLOAT);
		}
	}
	DiaDoneDialog(dia);
	efree((CHAR *)res);
	efree((CHAR *)cap);
	efree((CHAR *)ecap);
	if (spicepartcount > 0)
	{
		for(i=0; i<spicepartcount; i++) efree((CHAR *)spicepartlist[i]);
		efree((CHAR *)spicepartlist);
	}
	if (itemHit == DSPO_EDITHEADERS)
	{
		/* now edit the model cards */
		esnprintf(qual, 50, x_("SIM_spice_header_level%ld"), level);
		esnprintf(header, 200, _("Level %ld SPICE Model cards for %s"), level, curtech->techname);

		var = getval((INTBIG)curtech, VTECHNOLOGY, -1, qual);
		if (var == NOVARIABLE)
		{
			dummyfile[0] = x_("");
			var = setval((INTBIG)curtech, VTECHNOLOGY, qual, (INTBIG)dummyfile,
				VSTRING|VISARRAY|(1<<VLENGTHSH));
			if (var == NOVARIABLE)
			{
				ttyputerr(_("Cannot create %s on the technology"), qual);
				return;
			}
		} else
			var->type &= ~VDONTSAVE;

		/* get a new window, put an editor in it */
		w = us_wantnewwindow(0);
		if (w == NOWINDOWPART) return;
		if (us_makeeditor(w, header, &dummy, &dummy) == NOWINDOWPART) return;
		ed = w->editor;
		ed->editobjqual = qual;
		ed->editobjaddr = (CHAR *)curtech;
		ed->editobjtype = VTECHNOLOGY;
		ed->editobjvar = var;
		us_suspendgraphics(w);

		l = getlength(var);
		for(j=0; j<l; j++)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, describevariable(var, j, -1));
			us_addline(w, j, returninfstr(infstr));
		}
		us_resumegraphics(w);
		w->changehandler = us_varchanges;
	}
}

/* Verilog Options */
static DIALOGITEM sim_verilogoptdialogitems[] =
{
 /*  1 */ {0, {144,396,168,454}, BUTTON, N_("OK")},
 /*  2 */ {0, {144,268,168,326}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {80,216,96,375}, RADIO, N_("Use Model from File:")},
 /*  4 */ {0, {56,216,72,426}, RADIO, N_("Derive Model from Circuitry")},
 /*  5 */ {0, {80,392,96,463}, BUTTON, N_("Browse")},
 /*  6 */ {0, {104,224,136,480}, EDITTEXT, x_("")},
 /*  7 */ {0, {8,256,24,428}, CHECK, N_("Use ASSIGN Construct")},
 /*  8 */ {0, {28,8,168,206}, SCROLL, x_("")},
 /*  9 */ {0, {8,8,24,72}, MESSAGE, N_("Library:")},
 /* 10 */ {0, {8,76,24,206}, POPUP, x_("")},
 /* 11 */ {0, {32,256,48,428}, CHECK, N_("Default wire is Trireg")}
};
static DIALOG sim_verilogoptdialog = {{50,75,227,564}, N_("Verilog Options"), 0, 11, sim_verilogoptdialogitems, 0, 0};

/* special items for the "Verilog options" dialog: */
#define DVEO_USEMODELFILE   3		/* Use model from file (radio) */
#define DVEO_USEMODELCIRC   4		/* Use model from circuitry (radio) */
#define DVEO_SETMODELFILE   5		/* Set model file (button) */
#define DVEO_MODELFILE      6		/* Model file (edit text) */
#define DVEO_USEASSIGN      7		/* Use 'assign' construct (check) */
#define DVEO_CELLLIST       8		/* Cell list (scroll) */
#define DVEO_LIBRARYPOPUP  10		/* Library list (popup) */
#define DVEO_USETRIREG     11		/* Default wire is Trireg (check) */

/*
 * special case for the "Verilog options" dialog
 */
void sim_verilogdlog(void)
{
	INTBIG itemHit, i, verilog_state, libcount, oldplease;
	CHAR *subparams[3], *pt, **liblist;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER void *infstr, *dia;

	/* Display the Verilog options dialog box */
	dia = DiaInitDialog(&sim_verilogoptdialog);
	if (dia == 0) return;

	/* load up list of libraries */
	for(lib = el_curlib, libcount = 0; lib != NOLIBRARY; lib = lib->nextlibrary)
		libcount++;
	liblist = (CHAR **)emalloc(libcount * (sizeof (CHAR *)), el_tempcluster);
	if (liblist == 0) return;
	for(lib = el_curlib, libcount = 0; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		liblist[libcount] = lib->libname;
		libcount++;
	}
	esort(liblist, libcount, sizeof (CHAR *), sort_stringascending);
	DiaSetPopup(dia, DVEO_LIBRARYPOPUP, libcount, liblist);
	for(i=0; i<libcount; i++)
		if (estrcmp(el_curlib->libname, liblist[i]) == 0) break;
	if (i < libcount) DiaSetPopupEntry(dia, DVEO_LIBRARYPOPUP, i);

	/* get miscellaneous state */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_verilog_statekey);
	if (var != NOVARIABLE) verilog_state = var->addr; else verilog_state = 0;
	if ((verilog_state&VERILOGUSEASSIGN) != 0) DiaSetControl(dia, DVEO_USEASSIGN, 1);
	if ((verilog_state&VERILOGUSETRIREG) != 0) DiaSetControl(dia, DVEO_USETRIREG, 1);

	/* cache behavior file links */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		np->temp1 = 0;
		var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_verilog_behave_file"));
		if (var != NOVARIABLE)
			(void)allocstring((CHAR **)&np->temp1, (CHAR *)var->addr, el_tempcluster);
	}

	/* make list of cells in this library */
	sim_curlib = el_curlib;
	DiaInitTextDialog(dia, DVEO_CELLLIST, sim_topofcells, sim_nextcells, DiaNullDlogDone,
		0, SCSELMOUSE|SCSELKEY|SCREPORT);
	sim_verdiasetcellinfo(sim_curlib, dia);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;

		if (itemHit == DVEO_USEASSIGN || itemHit == DVEO_USETRIREG)
		{
			DiaSetControl(dia, itemHit, 1-DiaGetControl(dia, itemHit));
			continue;
		}

		if (itemHit == DVEO_LIBRARYPOPUP)
		{
			i = DiaGetPopupEntry(dia, DVEO_LIBRARYPOPUP);
			if (i < 0 || i >= libcount) continue;
			sim_curlib = getlibrary(liblist[i]);
			if (sim_curlib == NOLIBRARY) continue;
			DiaLoadTextDialog(dia, DVEO_CELLLIST, sim_topofcells, sim_nextcells, DiaNullDlogDone, 0);
			sim_verdiasetcellinfo(sim_curlib, dia);
			continue;
		}

		if (itemHit == DVEO_CELLLIST)
		{
			/* selection of cells */
			sim_verdiasetcellinfo(sim_curlib, dia);
			continue;
		}

		if (itemHit == DVEO_USEMODELFILE || itemHit == DVEO_USEMODELCIRC)
		{
			/* model for cell */
			DiaSetControl(dia, DVEO_USEMODELFILE, 0);
			DiaSetControl(dia, DVEO_USEMODELCIRC, 0);
			DiaSetControl(dia, itemHit, 1);
			pt = DiaGetScrollLine(dia, DVEO_CELLLIST, DiaGetCurLine(dia, DVEO_CELLLIST));
			lib = el_curlib;   el_curlib = sim_curlib;
			np = getnodeproto(pt);
			el_curlib = lib;
			if (itemHit == DVEO_USEMODELFILE)
			{
				DiaUnDimItem(dia, DVEO_MODELFILE);
				if (np != NONODEPROTO)
				{
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s.v"), np->protoname);
					pt = returninfstr(infstr);
					DiaSetText(dia, DVEO_MODELFILE, pt);
					if (np->temp1 != 0) efree((CHAR *)np->temp1);
					(void)allocstring((CHAR **)&np->temp1, pt, el_tempcluster);
				}
			} else
			{
				DiaSetText(dia, DVEO_MODELFILE, x_(""));
				DiaDimItem(dia, DVEO_MODELFILE);
				if (np != NONODEPROTO)
				{
					if (np->temp1 != 0) efree((CHAR *)np->temp1);
					np->temp1 = 0;
				}
			}
			continue;
		}

		if (itemHit == DVEO_SETMODELFILE)
		{
			/* set model for cell */
			pt = DiaGetScrollLine(dia, DVEO_CELLLIST, DiaGetCurLine(dia, DVEO_CELLLIST));
			lib = el_curlib;   el_curlib = sim_curlib;
			np = getnodeproto(pt);
			el_curlib = lib;
			if (np == NONODEPROTO) continue;
			oldplease = el_pleasestop;
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("verilog/"));
			addstringtoinfstr(infstr, _("Cell's Model File"));
			i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, subparams);
			el_pleasestop = oldplease;
			if (i != 0 && subparams[0][0] != 0)
			{
				DiaUnDimItem(dia, DVEO_MODELFILE);
				DiaSetText(dia, DVEO_MODELFILE, subparams[0]);
				if (np->temp1 != 0) efree((CHAR *)np->temp1);
				(void)allocstring((CHAR **)&np->temp1, subparams[0], el_tempcluster);
				DiaSetControl(dia, DVEO_USEMODELCIRC, 0);
				DiaSetControl(dia, DVEO_USEMODELFILE, 1);
			}
			continue;
		}
		if (itemHit == DVEO_MODELFILE)
		{
			pt = DiaGetScrollLine(dia, DVEO_CELLLIST, DiaGetCurLine(dia, DVEO_CELLLIST));
			lib = el_curlib;   el_curlib = sim_curlib;
			np = getnodeproto(pt);
			el_curlib = lib;
			if (np == NONODEPROTO) continue;
			if (np->temp1 != 0) efree((CHAR *)np->temp1);
			(void)allocstring((CHAR **)&np->temp1, DiaGetText(dia, DVEO_MODELFILE), el_tempcluster);
			continue;
		}
	}

	if (itemHit != CANCEL)  /* now update the values */
	{
		i = 0;
		if (DiaGetControl(dia, DVEO_USEASSIGN) != 0) i |= VERILOGUSEASSIGN;
		if (DiaGetControl(dia, DVEO_USETRIREG) != 0) i |= VERILOGUSETRIREG;
		if (i != verilog_state)
			(void)setvalkey((INTBIG)sim_tool, VTOOL, sim_verilog_statekey, i, VINTEGER);

		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			var = getval((INTBIG)np, VNODEPROTO, VSTRING, x_("SIM_verilog_behave_file"));
			if (var != NOVARIABLE)
			{
				if (np->temp1 == 0)
					(void)delval((INTBIG)np, VNODEPROTO, x_("SIM_verilog_behave_file")); else
				{
					if (estrcmp((CHAR *)np->temp1, (CHAR *)var->addr) != 0)
						(void)setval((INTBIG)np, VNODEPROTO, x_("SIM_verilog_behave_file"),
							(INTBIG)np->temp1, VSTRING);
				}
			} else if (np->temp1 != 0)
				(void)setval((INTBIG)np, VNODEPROTO, x_("SIM_verilog_behave_file"),
					(INTBIG)np->temp1, VSTRING);
		}
	}
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (np->temp1 != 0) efree((CHAR *)np->temp1);
	efree((CHAR *)liblist);
	DiaDoneDialog(dia);
}

/*
 * Helper routine to set information about the currently selected cell in the
 * "Verilog Options" dialog.
 */
void sim_verdiasetcellinfo(LIBRARY *curlib, void *dia)
{
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *savelib;
	REGISTER CHAR *pt;

	DiaSetControl(dia, DVEO_USEMODELCIRC, 1);
	DiaSetControl(dia, DVEO_USEMODELFILE, 0);
	DiaSetText(dia, DVEO_MODELFILE, x_(""));
	DiaDimItem(dia, DVEO_MODELFILE);
	pt = DiaGetScrollLine(dia, DVEO_CELLLIST, DiaGetCurLine(dia, DVEO_CELLLIST));
	savelib = el_curlib;   el_curlib = curlib;
	np = getnodeproto(pt);
	el_curlib = savelib;
	if (np != NONODEPROTO && np->temp1 != 0)
	{
		DiaSetControl(dia, DVEO_USEMODELCIRC, 0);
		DiaSetControl(dia, DVEO_USEMODELFILE, 1);
		DiaUnDimItem(dia, DVEO_MODELFILE);
		DiaSetText(dia, DVEO_MODELFILE, (CHAR *)np->temp1);
	}
}

/*
 * Helper routines for listing all cells in library "sim_curlib".
 */
BOOLEAN sim_topofcells(CHAR **c)
{
	Q_UNUSED( c );
	sim_oldcellprotos = sim_curlib->firstnodeproto;
	return(TRUE);
}
CHAR *sim_nextcells(void)
{
	REGISTER NODEPROTO *thisnp;
	REGISTER CHAR *ret;

	ret = 0;
	if (sim_oldcellprotos != NONODEPROTO)
	{
		thisnp = sim_oldcellprotos;
		sim_oldcellprotos = sim_oldcellprotos->nextnodeproto;
		ret = nldescribenodeproto(thisnp);
	}
	return(ret);
}

#endif  /* SIMTOOL */
