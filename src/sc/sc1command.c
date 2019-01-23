/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1command.c
 * Command table for the QUISC Silicon Compiler to interface it with Electric
 * Written by: Andrew R. Kostiuk, Queen's University
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
#if SCTOOL

#include "global.h"


/***********************************************************************
	QUISC SILICON COMPILER Command Completion Tables
-----------------------------------------------------------------------
*/

/*>>>>>>>>>>>>>>>>>>>> CONNECT command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_connap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of node a"), 0};

static COMCOMP sc_conpap = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("port on node a"), 0};

static KEYWORD sc_conb[] = {
	{x_("power"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ground"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_connbp = {sc_conb,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of node b"), 0};

static COMCOMP sc_conpbp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("port on node b"), 0};


/*>>>>>>>>>>>>>>>>>>>> CREATE command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_cellp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of cell"), 0};

static COMCOMP sc_crinstnp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("instance name"), 0};

static COMCOMP sc_crinsttp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("instance type"), 0};

static KEYWORD sc_createopt[] =
{
	{x_("cell"),		1,{&sc_cellp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("instance"),	2,{&sc_crinstnp,&sc_crinsttp,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_createp = {sc_createopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("create option"), 0};


/*>>>>>>>>>>>>>>>>>>>> EXPORT command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_exportnp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of instance"), 0};

static COMCOMP sc_exportpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of port on instance"), 0};

static COMCOMP sc_exportpnp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of port"), 0};

static KEYWORD sc_exportopt[] =
{
	{x_("bidirectional"),	0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("input"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("output"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_exporttp = {sc_exportopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("type of port"), 0};


/*>>>>>>>>>>>>>>>>>>>> EXTRACT command <<<<<<<<<<<<<<<<<<<<*/

static KEYWORD sc_echo[] =
{
	{x_("echo"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_extractp = {sc_echo,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("optional echo flag"), 0};


/*>>>>>>>>>>>>>>>>>>>> LIBRARY command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_libusep = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of library"), 0};

static KEYWORD sc_libopt[] =
{
	{x_("read"),		1,{&sc_libusep,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_libp = {sc_libopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("library option"), 0};


/*>>>>>>>>>>>>>>>>>>>> MAKE command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_makemsp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Minimum spacing (in internal units)"), 0};

static COMCOMP sc_makevsp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Via Size (internal units)"), 0};

static COMCOMP sc_makeavp = {NOKEYWORD,topofarcs,nextarcs,NOPARAMS,
	0, x_(" \t"), M_("Arc type to use for vertical wires"), 0};

static COMCOMP sc_makeahp = {NOKEYWORD,topofarcs,nextarcs,NOPARAMS,
	0, x_(" \t"), M_("Arc type to use for horizontal wires"), 0};


static KEYWORD sc_makempropt[] =
{
	{x_("horizontal"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertical"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP sc_makemprp = {sc_makempropt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Arc to use for main power wires"), 0};

static COMCOMP sc_makevtp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Layer2 Track width (internal units)"), 0};

static COMCOMP sc_makehtp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Layer1 Track width (internal units)"), 0};

static COMCOMP sc_makeptp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Power Track width (internal units)"), 0};

static COMCOMP sc_makemtp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Main Power Bus width (internal units)"), 0};

static COMCOMP sc_makepwsp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("P-well Size (internal units)"), 0};

static COMCOMP sc_makepwop = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("P-well Offset from bottom (internal units)"), 0};

static COMCOMP sc_makenwsp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("N-well Size (internal units)"), 0};

static COMCOMP sc_makenwop = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("N-well Offset from top (internal units)"), 0};

static KEYWORD sc_makesetopt[] =
{
	{x_("verbose"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-verbose"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("min-spacing"),		1,{&sc_makemsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("via-size"),		1,{&sc_makevsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arc-horizontal"),	1,{&sc_makeahp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("arc-vertical"),	1,{&sc_makeavp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("l1-track-width"),	1,{&sc_makehtp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("l2-track-width"),	1,{&sc_makevtp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power-track-width"),1,{&sc_makeptp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("main-power-width"),1,{&sc_makemtp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("main-power-rail"), 1,{&sc_makemprp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("p-well-size"),		1,{&sc_makepwsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("p-well-offset"),	1,{&sc_makepwop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("n-well-size"),		1,{&sc_makenwsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("n-well-offset"),	1,{&sc_makenwop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_makesetp = {sc_makesetopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Make set-control option"), 0};

static KEYWORD sc_makeopt[] =
{
	{x_("information"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-control"),		5,{&sc_makesetp,&sc_makesetp,&sc_makesetp,
								  &sc_makesetp,&sc_makesetp}},
	{x_("show-control"),	0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

COMCOMP sc_makep = {sc_makeopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Make option"), 0};


/*>>>>>>>>>>>>>>>>>>>> ORDER command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_orderp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("leaf cell to order ports"), 0};

static COMCOMP sc_orderfp = {NOKEYWORD,topoffile,nextfile,NOPARAMS,
	NOFILL, x_(" \t"), M_("Optional input file"), 0};


/*>>>>>>>>>>>>>>>>>>>> PLACE command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_placerp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Number of rows"), 0};

static COMCOMP sc_placevp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Vertical cost scaling factor"), 0};

static COMCOMP sc_placenp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Movement limit for Net Balancing"), 0};

static KEYWORD sc_placesetopt[] =
{
	{x_("verbose"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-verbose"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("sort"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-sort"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rows"),			1,{&sc_placerp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("net-balance"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-net-balance"),	0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("limit-net-balance"),1,{&sc_placenp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vertical-cost"),	1,{&sc_placevp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_placesetp = {sc_placesetopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Placement set-control option"), 0};

static KEYWORD sc_placeopt[] =
{
	{x_("set-control"),		5,{&sc_placesetp,&sc_placesetp,&sc_placesetp,
						   &sc_placesetp,&sc_placesetp}},
	{x_("show-control"),	0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_placep = {sc_placeopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("placement algorithm"), 0};


/*>>>>>>>>>>>>>>>>>>>> PULL command <<<<<<<<<<<<<<<<<<<<*/

static KEYWORD sc_pullopt[] =
{
	{x_("information"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_pullp = {sc_pullopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("pull (flatten) cell"), 0};


/*>>>>>>>>>>>>>>>>>>>> ROUTE command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_routeftp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Size of Feed Throughs (internal units)"), 0};

static COMCOMP sc_routepdp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Minimum Distance between ports (internal units)"), 0};

static COMCOMP sc_routefwp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Fuzzy Window Limit (internal units)"), 0};

static COMCOMP sc_routeadp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Minimum Active distance (internal units)"), 0};

static KEYWORD sc_routesetopt[] =
{
	{x_("verbose"),				0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("no-verbose"),			0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("feed-through-size"),	1,{&sc_routeftp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("port-x-min-distance"),	1,{&sc_routepdp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fuzzy-window-limit"),	1,{&sc_routefwp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("active-distance"),		1,{&sc_routeadp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default"),				0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_routesetp = {sc_routesetopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Route set-control option"), 0};

static KEYWORD sc_routeopt[] =
{
	{x_("set-control"),		5,{&sc_routesetp,&sc_routesetp,&sc_routesetp,
						   &sc_routesetp,&sc_routesetp}},
	{x_("show-control"),	0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_routep = {sc_routeopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Routing option"), 0};


/*>>>>>>>>>>>>>>>>>>>> SELECT command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_selectp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of cell"), 0};


/*>>>>>>>>>>>>>>>>>>>> SET command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_setpdp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("direction(s) to attach to port (u d r l)"), 0};

static COMCOMP sc_setpdpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of port"), 0};

static COMCOMP sc_setpdcp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of cell"), 0};

static COMCOMP sc_setnumvp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("From edge (internal units)"), 0};

static KEYWORD sc_setnumopt[] =
{
	{x_("top-active"),		1,{&sc_setnumvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("bottom-active"),	1,{&sc_setnumvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("left-active"),		1,{&sc_setnumvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("right-active"),	1,{&sc_setnumvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_setnump = {sc_setnumopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("cell number option"), 0};

static COMCOMP sc_setnnip = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("instance name"), 0};

static COMCOMP sc_setnnpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("port name"), 0};

static COMCOMP sc_setnnnp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("node name"), 0};

static KEYWORD sc_setopt[] =
{
	{x_("leaf-cell-numbers"),5,{&sc_setpdcp,&sc_setnump,&sc_setnump,
						  &sc_setnump,&sc_setnump}},
	{x_("node-name"),		 3,{&sc_setnnip,&sc_setnnpp,&sc_setnnnp,NOKEY,NOKEY}},
	{x_("port-direction"),	 3,{&sc_setpdcp,&sc_setpdpp,&sc_setpdp,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_setp = {sc_setopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("set option"), 0};


/*>>>>>>>>>>>>>>>>>>>> SHOW command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_showpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("cell name"), 0};

static KEYWORD sc_shownopt[] =
{
	{x_("all"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_shownp = {sc_shownopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("show instances option"), 0};

static COMCOMP sc_shownump = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("leaf cell name"), 0};

static KEYWORD sc_showopt[] =
{
	{x_("cells"),				0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("instances"),			2,{&sc_showpp,&sc_shownp,NOKEY,NOKEY,NOKEY}},
	{x_("nodes"),				0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("leaf-cell-numbers"),	1,{&sc_shownump,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("placement"),			1,{&sc_showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("ports"),				1,{&sc_showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("routing"),				1,{&sc_showpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_showp = {sc_showopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("show option"), 0};


/*>>>>>>>>>>>>>>>>>>>> SIMULATION command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_simsetp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of cell"), 0};

static COMCOMP sc_simwrfilep = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("name of output file"), 0};

static COMCOMP sc_simformatp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"),  M_("Output format: als or silos"), 0};

static KEYWORD sc_simopt[] =
{
	{x_("set"),		1,{&sc_simsetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show"),	1,{&sc_simsetp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("write"),	2,{&sc_simsetp,&sc_simwrfilep,NOKEY,NOKEY,NOKEY}},
	{x_("format"),	2,{&sc_simformatp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

static COMCOMP sc_simp = {sc_simopt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("set option"), 0};


/*>>>>>>>>>>>>>>>>>>>> HELP command <<<<<<<<<<<<<<<<<<<<*/

static COMCOMP sc_helpp = {NOKEYWORD,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("help object"), 0};


/*>>>>>>>>>>>>>>>>>>>> SILICON COMPILER commands <<<<<<<<<<<<<<<<<<<<*/

static KEYWORD sc_opt[] =
{
	{x_("compile"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("connect"),		4,{&sc_connap,&sc_conpap,&sc_connbp,&sc_conpbp,NOKEY}},
	{x_("create"),		1,{&sc_createp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("delete"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("export"),		4,{&sc_exportnp,&sc_exportpp,&sc_exportpnp,&sc_exporttp,NOKEY}},
	{x_("extract"),		1,{&sc_extractp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("help"),		1,{&sc_helpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("library"),		1,{&sc_libp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("make"),		1,{&sc_makep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("order"),		2,{&sc_orderp,&sc_orderfp,NOKEY,NOKEY,NOKEY}},
	{x_("place"),		1,{&sc_placep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pull"),		1,{&sc_pullp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("route"),		1,{&sc_routep,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("schematic"),	0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("select"),		1,{&sc_selectp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set"),			1,{&sc_setp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show"),		1,{&sc_showp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("simulation"),	1,{&sc_simp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("verify"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("quit"),		0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};

COMCOMP sc_silcomp = {sc_opt,NOTOPLIST,NONEXTLIST,NOPARAMS,
	0, x_(" \t"), M_("Silicon Compiler action"), 0};

#endif  /* SCTOOL - at top */
