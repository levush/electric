/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1electric.c
 * Modules for the QUISC Silicon Compiler to interface it with Electric
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
#include "efunction.h"
#include "edialogs.h"
#include "sc1.h"
#include "usr.h"

/***********************************************************************
	Global Variables
------------------------------------------------------------------------
*/

TOOL             *sc_tool;			/* the Silicon Compiler tool object */
INTBIG            sc_filetypescsim;	/* Silicon compiler simulation file descriptor */
INTBIG            sc_filetypesctab;	/* Silicon compiler table file descriptor */
static INTBIG     sc_simkey;		/* key for the ALS info */
static INTBIG     sc_silkey;		/* key for the silos information */
static INTBIG     sc_bitskey;
static INTBIG     sc_numskey;
static NODEPROTO *sc_layer1proto = NONODEPROTO;
static NODEPROTO *sc_layer2proto = NONODEPROTO;
static NODEPROTO *sc_viaproto = NONODEPROTO;
static NODEPROTO *sc_pwellproto = NONODEPROTO;
static NODEPROTO *sc_nwellproto = NONODEPROTO;
static ARCPROTO	 *sc_layer1arc = NOARCPROTO;
static ARCPROTO	 *sc_layer2arc = NOARCPROTO;
static LIBRARY   *sc_celllibrary = NOLIBRARY;

extern CHAR      *sc_errmsg;
extern int        sc_sim_format;
extern COMCOMP    sc_makep;

/* prototypes for local routines */
static void Sc_set_leaf_port_type(PORTPROTO*, int);
static void Sc_create_connection(NODEINST **ni, PORTPROTO **pp, INTBIG x, INTBIG y,
			ARCPROTO *arc);
static void sc_optionsdlog(void);

/***********************************************************************
Module:  sc_init
------------------------------------------------------------------------
Description:
	Initialize routine for the Silicon Compiler tool.
------------------------------------------------------------------------
*/

void sc_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	Q_UNUSED( argc );
	Q_UNUSED( argv );
	/* only initialize during pass 1 */
	if (thistool == NOTOOL || thistool == 0) return;
	sc_tool = thistool;
	Sc_initialize();
	if ((sc_simkey = makekey(x_("SC_sim"))) == -1)
		ttyputmsg(x_("ERROR making SIM key in SC Tool"));
	if ((sc_bitskey = makekey(x_("SC_bits"))) == -1)
		ttyputmsg(x_("ERROR making BITS key in SC Tool"));
	if ((sc_numskey = makekey(x_("SC_nums"))) == -1)
		ttyputmsg(x_("ERROR making NUMS key in SC Tool"));
	if ((sc_silkey = makekey(x_("SC_silos"))) == -1)
		ttyputmsg(x_("ERROR making SILOS key in SC Tool."));
	DiaDeclareHook(x_("silcomp"), &sc_makep, sc_optionsdlog);
	sc_filetypescsim = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("scsim"), _("QUISC simulation"));
	sc_filetypesctab = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("sctab"), _("QUISC table"));
}

/***********************************************************************
Module:  sc_slice
------------------------------------------------------------------------
Description:
	Main module of the Silicon Compiler tool.  Invoked by the ontool
	command.
------------------------------------------------------------------------
*/

void sc_slice(void)
{
	el_pleasestop = 0;
	Sc_main();
	toolturnoff(sc_tool, FALSE);
}

/***********************************************************************
Module:  sc_set
------------------------------------------------------------------------
Description:
	Module invoked by the telltool command.
------------------------------------------------------------------------
*/

void sc_set(INTBIG count, CHAR *pars[])
{
	Sc_one_command(count, pars);
}

/***********************************************************************
Module:  sc_request
------------------------------------------------------------------------
Description:
	Module invoked by the asktool command.
------------------------------------------------------------------------
*/

INTBIG sc_request(CHAR *command, va_list ap)
{
	Q_UNUSED( ap );

	if (namesame(command, x_("cell-lib")) == 0)
		return((INTBIG)sc_celllibrary);
	return(0);
}

/***********************************************************************
Default Silicon Compiler tool routines.
------------------------------------------------------------------------
*/

void sc_done(void) {}

/***********************************************************************
Module:  Sc_stop and Sc_clear_stop
------------------------------------------------------------------------
Description:
	Sc_stop returns TRUE if the stopping condition is TRUE.  Sc_clear_stop
	clears the stopping condition.
------------------------------------------------------------------------
*/

int Sc_stop(void)
{
	if (stopping(STOPREASONSILCOMP)) return(TRUE);
	return(FALSE);
}

void Sc_clear_stop(void)
{
	el_pleasestop = 0;
}


INTBIG ScGetParameter(INTBIG paramnum)
{
	REGISTER VARIABLE *var;

	var = NOVARIABLE;
	switch (paramnum)
	{
		case SC_PARAM_MAKE_HORIZ_ARC:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_horiz_arc"));
			if (var == NOVARIABLE) return((INTBIG)DEFAULT_ARC_HORIZONTAL);
			break;
		case SC_PARAM_MAKE_VERT_ARC:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_vert_arc"));
			if (var == NOVARIABLE) return((INTBIG)DEFAULT_ARC_VERTICAL);
			break;
		case SC_PARAM_MAKE_L1_WIDTH:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_l1_width"));
			if (var == NOVARIABLE) return(DEFAULT_L1_TRACK_WIDTH);
			break;
		case SC_PARAM_MAKE_L2_WIDTH:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_l2_width"));
			if (var == NOVARIABLE) return(DEFAULT_L2_TRACK_WIDTH);
			break;
		case SC_PARAM_MAKE_PWR_WIDTH:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_pwr_width"));
			if (var == NOVARIABLE) return(DEFAULT_POWER_TRACK_WIDTH);
			break;
		case SC_PARAM_MAKE_MAIN_PWR_WIDTH:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_main_pwr_width"));
			if (var == NOVARIABLE) return(DEFAULT_MAIN_POWER_WIDTH);
			break;
		case SC_PARAM_MAKE_MAIN_PWR_RAIL:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_main_pwr_rail"));
			if (var == NOVARIABLE) return(DEFAULT_MAIN_POWER_RAIL);
			break;
		case SC_PARAM_MAKE_PWELL_SIZE:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_pwell_size"));
			if (var == NOVARIABLE) return(DEFAULT_PWELL_SIZE);
			break;
		case SC_PARAM_MAKE_PWELL_OFFSET:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_pwell_offset"));
			if (var == NOVARIABLE) return(DEFAULT_PWELL_OFFSET);
			break;
		case SC_PARAM_MAKE_NWELL_SIZE:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_nwell_size"));
			if (var == NOVARIABLE) return(DEFAULT_NWELL_SIZE);
			break;
		case SC_PARAM_MAKE_NWELL_OFFSET:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_nwell_offset"));
			if (var == NOVARIABLE) return(DEFAULT_NWELL_OFFSET);
			break;
		case SC_PARAM_MAKE_VIA_SIZE:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_via_size"));
			if (var == NOVARIABLE) return(DEFAULT_VIA_SIZE);
			break;
		case SC_PARAM_MAKE_MIN_SPACING:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_min_spacing"));
			if (var == NOVARIABLE) return(DEFAULT_MIN_SPACING);
			break;
		case SC_PARAM_ROUTE_FEEDTHRU_SIZE:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_feedthru_size"));
			if (var == NOVARIABLE) return(DEFAULT_FEED_THROUGH_SIZE);
			break;
		case SC_PARAM_ROUTE_PORT_X_MIN_DIST:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_port_x_min_dist"));
			if (var == NOVARIABLE) return(DEFAULT_PORT_X_MIN_DISTANCE);
			break;
		case SC_PARAM_ROUTE_ACTIVE_DIST:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_active_dist"));
			if (var == NOVARIABLE) return(DEFAULT_ACTIVE_DISTANCE);
			break;
		case SC_PARAM_PLACE_NUM_ROWS:
			var = getval((INTBIG)sc_tool, VTOOL, -1, x_("SC_num_rows"));
			if (var == NOVARIABLE) return(DEFAULT_NUM_OF_ROWS);
			break;
	}
	return(var->addr);
}

void ScSetParameter(INTBIG paramnum, INTBIG addr)
{
	switch (paramnum)
	{
		case SC_PARAM_MAKE_HORIZ_ARC:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_horiz_arc"), addr, VARCPROTO);
			break;
		case SC_PARAM_MAKE_VERT_ARC:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_vert_arc"), addr, VARCPROTO);
			break;
		case SC_PARAM_MAKE_L1_WIDTH:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_l1_width"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_L2_WIDTH:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_l2_width"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_PWR_WIDTH:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_pwr_width"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_MAIN_PWR_WIDTH:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_main_pwr_width"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_MAIN_PWR_RAIL:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_main_pwr_rail"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_PWELL_SIZE:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_pwell_size"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_PWELL_OFFSET:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_pwell_offset"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_NWELL_SIZE:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_nwell_size"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_NWELL_OFFSET:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_nwell_offset"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_VIA_SIZE:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_via_size"), addr, VINTEGER);
			break;
		case SC_PARAM_MAKE_MIN_SPACING:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_min_spacing"), addr, VINTEGER);
			break;
		case SC_PARAM_ROUTE_FEEDTHRU_SIZE:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_feedthru_size"), addr, VINTEGER);
			break;
		case SC_PARAM_ROUTE_PORT_X_MIN_DIST:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_port_x_min_dist"), addr, VINTEGER);
			break;
		case SC_PARAM_ROUTE_ACTIVE_DIST:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_active_dist"), addr, VINTEGER);
			break;
		case SC_PARAM_PLACE_NUM_ROWS:
			setval((INTBIG)sc_tool, VTOOL, x_("SC_num_rows"), addr, VINTEGER);
			break;
	}
}

/***********************************************************************

		D A T A B A S E     I N T E R F A C I N G     M O D U L E S

------------------------------------------------------------------------
*/

/***********************************************************************
Module:  Sc_find_leaf_cell
------------------------------------------------------------------------
Description:
	Return a pointer to the named leaf cell.  If cell is not found,
	return NULL.
------------------------------------------------------------------------
Calling Sequence:  leafcell = Sc_find_leaf_cell(name);

Name		Type		Description
----		----		-----------
name		*char		String name of leaf cell.
leafcell	*char		Generic pointer to found cell,
								NULL if not found.
------------------------------------------------------------------------
*/

CHAR  *Sc_find_leaf_cell(CHAR *name)
{
	NODEPROTO *np, *laynp;
	REGISTER void *infstr;

	np = getnodeproto(name);
	if (np == NONODEPROTO)
	{
		infstr = initinfstr();
		if (sc_celllibrary != NOLIBRARY)
		{
			formatinfstr(infstr, x_("%s:%s"), sc_celllibrary->libname, name);
		} else
		{
			formatinfstr(infstr, x_("%s"), name);
		}
		np = getnodeproto(returninfstr(infstr));
		if (np == NONODEPROTO) return((CHAR *)NULL);
	}
	laynp = layoutview(np);
	if (laynp != NONODEPROTO) np = laynp;
	if (np->primindex == 0) return((CHAR *)np);
	return((CHAR *)NULL);
}

/***********************************************************************
Module:  Sc_leaf_cell_name
------------------------------------------------------------------------
Description:
	Return a pointer to the name of a leaf cell.
------------------------------------------------------------------------
Calling Sequence:  leafcellname = Sc_leaf_cell_name(leafcell);

Name			Type		Description
----			----		-----------
leafcell		*char		Pointer to the leaf cell.
leafcellname	*char		String of leaf cell name.
------------------------------------------------------------------------
*/

CHAR  *Sc_leaf_cell_name(CHAR *leafcell)
{
	NODEPROTO *np;

	np = (NODEPROTO *)leafcell;
	if (np->primindex != 0) return(np->protoname);
	return(np->protoname);
}

/***********************************************************************
Module:  Sc_first_leaf_cell
------------------------------------------------------------------------
Description:
	Return a pointer to the first leaf cell in the current library.
	Return NULL if error, eg. no library, no leaf cells, etc.
------------------------------------------------------------------------
Calling Sequence:  leafcell = Sc_first_leaf_cell();

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the first leaf cell, NULL if
								no leaf cell found.
------------------------------------------------------------------------
*/

CHAR  *Sc_first_leaf_cell(void)
{
	if (el_curlib == NOLIBRARY) return((CHAR *)NULL);
	if (el_curlib->firstnodeproto == NONODEPROTO) return((CHAR *)NULL);
	return((CHAR *)el_curlib->firstnodeproto);
}

/***********************************************************************
Module:  Sc_next_leaf_cell
------------------------------------------------------------------------
Description:
	Return a pointer to the next leaf cell in the current library.
	Return NULL when end of list is reached.
------------------------------------------------------------------------
Calling Sequence:  nextleafcell = Sc_next_leaf_cell(leafcell);

Name			Type		Description
----			----		-----------
leafcell		*char		Pointer to the current leaf cell.
nextleafcell	*char		Returned pointer to next leaf cell.
------------------------------------------------------------------------
*/

CHAR  *Sc_next_leaf_cell(CHAR *leafcell)
{
	if (((NODEPROTO *)leafcell)->nextnodeproto == NONODEPROTO)
		return((CHAR *)NULL);
	return((CHAR *)(((NODEPROTO *)leafcell)->nextnodeproto));
}

/***********************************************************************
Module:  Sc_leaf_cell_bits
------------------------------------------------------------------------
Description:
	Return the value of the bits field of the leaf cell for the
	Silicon Compiler.
------------------------------------------------------------------------
Calling Sequence:  bits = Sc_leaf_cell_bits(leafcell);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
bits		int			Value of the SC bit field.
------------------------------------------------------------------------
*/

int Sc_leaf_cell_bits(CHAR *leafcell)
{
	VARIABLE	*var;

	/* check if variable exits */
	if ((var = getvalkey((INTBIG)leafcell, VNODEPROTO, VINTEGER, sc_bitskey))
		== NOVARIABLE)
	{
		if (setvalkey((INTBIG)leafcell, VNODEPROTO, sc_bitskey, 0, VINTEGER) ==
			NOVARIABLE) ttyputmsg(_("ERROR creating SCBITS variable."));
		return(0);
	}
	return(var->addr);
}

/***********************************************************************
Module:  Sc_leaf_cell_bits_address
------------------------------------------------------------------------
Description:
	Return the address of the bits field of the leaf cell for the
	Silicon Compiler.
------------------------------------------------------------------------
Calling Sequence:  bits_addr = Sc_leaf_cell_bits_address(leafcell);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
bits_addr	*int		Address of the SC bits field.
------------------------------------------------------------------------
*/

int  *Sc_leaf_cell_bits_address(CHAR *leafcell)
{
	VARIABLE	*var;
	static int retval;

	/* check if variable exits */
	if ((var = getvalkey((INTBIG)leafcell, VNODEPROTO, VINTEGER, sc_bitskey))
		== NOVARIABLE)
	{
		if (setvalkey((INTBIG)leafcell, VNODEPROTO, sc_bitskey, 0, VINTEGER) ==
			NOVARIABLE) ttyputmsg(_("ERROR creating SCBITS variable."));
		if ((var = getvalkey((INTBIG)leafcell, VNODEPROTO, VINTEGER,
			sc_bitskey)) == NOVARIABLE)
		{
			ttyputmsg(_("ERROR creating SCBITS variable."));
			return((int *)NULL);
		}
	}
	retval = var->addr;
	return(&retval);
}

/***********************************************************************
Module:  Sc_leaf_cell_xsize
------------------------------------------------------------------------
Description:
	Return the size in the x direction for the indicated leaf cell.
------------------------------------------------------------------------
Calling Sequence:  xsize = Sc_leaf_cell_xsize(leafcell);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
xsize		int			Returned size in x direction.
------------------------------------------------------------------------
*/

int Sc_leaf_cell_xsize(CHAR *leafcell)
{
	return(((NODEPROTO *)leafcell)->highx - ((NODEPROTO *)leafcell)->lowx);
}

/***********************************************************************
Module:  Sc_leaf_cell_ysize
------------------------------------------------------------------------
Description:
	Return the size in the y direction for the indicated leaf cell.
------------------------------------------------------------------------
Calling Sequence:  xsize = Sc_leaf_cell_ysize(leafcell);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
xsize		int			Returned size in y direction.
------------------------------------------------------------------------
*/

int Sc_leaf_cell_ysize(CHAR *leafcell)
{
	return(((NODEPROTO *)leafcell)->highy - ((NODEPROTO *)leafcell)->lowy);
}

/***********************************************************************
Module:  Sc_leaf_cell_sim_info
------------------------------------------------------------------------
Description:
	Return an array of pointers to strings for the leaf cell's
	simulation information.  Return NULL on error.  Note that the
	array should be terminated by a NULL.
------------------------------------------------------------------------
Calling Sequence:  simlist = Sc_leaf_cell_sim_info(leafcell);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
simlist		**char		Array of pointers to simulation info,
								NULL on error.
------------------------------------------------------------------------
*/

CHAR  **Sc_leaf_cell_sim_info(CHAR *leafcell)
{
	VARIABLE	*simvar;
	CHAR	**simlist, **simlist2;
	int		i, numsim, use_key;

	if (sc_sim_format == SC_ALS_FORMAT)
		use_key = sc_simkey;
	else if (sc_sim_format == SC_SILOS_FORMAT)
		use_key = sc_silkey;
	if ((simvar = getvalkey((INTBIG)leafcell, VNODEPROTO, VSTRING | VISARRAY,
		use_key)) == NOVARIABLE) return((CHAR **)NULL);
	simlist2 = (CHAR **)simvar->addr;

	/* count the number of simulation lines */
	numsim = 0;
	for (i = 0; simlist2[i] != NOSTRING; i++) numsim++;

	/* create array of pointers */
	if ((simlist = (CHAR **)emalloc(sizeof(CHAR *)*(numsim+1),sc_tool->cluster))
		== 0) return((CHAR **)NULL);
	for (i = 0; i < numsim; i++) simlist[i] = simlist2[i];
	simlist[numsim] = NULL;
	return(simlist);
}

/***********************************************************************
Module:  Sc_leaf_cell_set_sim
------------------------------------------------------------------------
Description:
	Set the passed simulation information to the indicated leaf cell.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_leaf_cell_set_sim(simline, leafcell);

Name		Type		Description
----		----		-----------
simline		*SCSIM		Lines of simulation information.
leafcell	*char		Pointer to the leaf cell.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_leaf_cell_set_sim(SCSIM *simline, CHAR *leafcell)
{
	CHAR	**simlist;
	SCSIM	*nextsim;
	int		i, numsim, use_key;

	if (sc_sim_format == SC_ALS_FORMAT)
		use_key = sc_simkey;
	else if (sc_sim_format == SC_SILOS_FORMAT)
		use_key = sc_silkey;
	/* count the number of simulation lines */
	numsim = 0;
	for (nextsim = simline; nextsim; nextsim = nextsim->next) numsim++;

	/* create array of pointers */
	if ((simlist = (CHAR **)emalloc(sizeof(CHAR *)*(numsim+1),sc_tool->cluster)) == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	for (i = 0; i < numsim; i++)
	{
		simlist[i] = simline->model;
		simline = simline->next;
	}
	simlist[numsim] = NOSTRING;

	/* set variable */
	if (setvalkey((INTBIG)leafcell, VNODEPROTO, use_key, (INTBIG)simlist,
		VSTRING | VISARRAY) == NOVARIABLE) return(Sc_seterrmsg(SC_SIMXSETVAL));
	return(0);
}

/***********************************************************************
Module:  Sc_leaf_cell_get_nums
------------------------------------------------------------------------
Description:
	Fill in the cell_nums structure for the indicated leaf cell.
------------------------------------------------------------------------
Calling Sequence:  Sc_leaf_cell_get_nums(leafcell, nums);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
nums		*SCCELLNUMS	Address of structure to fill.
------------------------------------------------------------------------
*/

void Sc_leaf_cell_get_nums(CHAR *leafcell, SCCELLNUMS *nums)
{
	VARIABLE	*var;
	int		i, j, *iarray, *jarray;

	i = sizeof(SCCELLNUMS) / sizeof(int);

	/* check if variable exits */
	var = getvalkey((INTBIG)leafcell, VNODEPROTO, VINTEGER|VISARRAY, sc_numskey);
	if (var == NOVARIABLE)
	{
		iarray = (int *)nums;
		for (j = 0; j < i; j++) iarray[j] = 0;
		return;
	}

	iarray = (int *)nums;
	jarray = (int *)var->addr;
	for (j = 0; j < i; j++) iarray[j] = jarray[j];
}

/***********************************************************************
Module:  Sc_leaf_cell_set_nums
------------------------------------------------------------------------
Description:
	Set the cell_nums variable for the indicated leaf cell.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_leaf_cell_set_nums(leafcell, nums);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to the leaf cell.
nums		*SCCELLNUMS	Pointer to cell nums structure.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_leaf_cell_set_nums(CHAR *leafcell, SCCELLNUMS *nums)
{
	VARIABLE	*var;
	int		i, j, *iarray;
	INTBIG   *jarray;

	i = sizeof(SCCELLNUMS) / sizeof(int);

	/* check if variable exits */
	var = getvalkey((INTBIG)leafcell, VNODEPROTO, VINTEGER|VISARRAY, sc_numskey);
	if (var == NOVARIABLE)
	{
		if ((jarray = emalloc((i + 1) * sizeof(INTBIG), sc_tool->cluster)) == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		iarray = (int *)nums;
		for (j = 0; j < i; j++) jarray[j] = iarray[j];
		jarray[j] = -1;
		if (setvalkey((INTBIG)leafcell, VNODEPROTO, sc_numskey, (INTBIG)jarray,
			VINTEGER | VISARRAY) == NOVARIABLE)
				return(Sc_seterrmsg(SC_NOSET_CELL_NUMS));
		return(SC_NOERROR);
	}
	iarray = (int *)nums;
	jarray = (INTBIG *)var->addr;
	for (j = 0; j < i; j++) jarray[j] = iarray[j];
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_find_leaf_port
------------------------------------------------------------------------
Description:
	Return a pointer to the named port on the cell.  If no port found,
	return NULL.
------------------------------------------------------------------------
Calling Sequence:  leafport = Sc_find_leaf_port(leafcell, portname);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to leaf cell.
portname	*char		String name of port.
leafport	*char		Generic pointer to first port,
								NULL if not found.
------------------------------------------------------------------------
*/

CHAR  *Sc_find_leaf_port(CHAR *leafcell, CHAR *portname)
{
	PORTPROTO	*pp;

	for (pp = ((NODEPROTO *)leafcell)->firstportproto; pp != NOPORTPROTO;
		pp = pp->nextportproto)
	{
		if (namesame(pp->protoname, portname) == 0)
			return((CHAR *)pp);
	}
	return((CHAR *)NULL);
}

/***********************************************************************
Module:  Sc_leaf_port_name
------------------------------------------------------------------------
Description:
	Return a pointer to the name of a leaf port.
------------------------------------------------------------------------
Calling Sequence:  portname = Sc_leaf_port_name(leafport);

Name		Type		Description
----		----		-----------
leafport	*char		Pointer to port.
portname	*char		String of port name.
------------------------------------------------------------------------
*/

CHAR  *Sc_leaf_port_name(CHAR *leafport)
{
	return(((PORTPROTO *)leafport)->protoname);
}

/***********************************************************************
Module:  Sc_first_leaf_port
------------------------------------------------------------------------
Description:
	Return a pointer to the first port of the cell.  If no ports,
	return NULL.
------------------------------------------------------------------------
Calling Sequence:  leafport = Sc_first_leaf_port(leafcell);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to leaf cell.
leafport	*char		Generic pointer to first port,
								NULL if not found.
------------------------------------------------------------------------
*/

CHAR  *Sc_first_leaf_port(CHAR *leafcell)
{
	PORTPROTO	*pp;

	pp = ((NODEPROTO *)leafcell)->firstportproto;
	if (pp == NOPORTPROTO) return((CHAR *)NULL);
	return((CHAR *)pp);
}

/***********************************************************************
Module:  Sc_next_leaf_port
------------------------------------------------------------------------
Description:
	Return a pointer to the next port of the cell.  If no ports left,
	return NULL.
------------------------------------------------------------------------
Calling Sequence:  nextport = Sc_next_leaf_port(curport);

Name		Type		Description
----		----		-----------
curport		*char		Pointer to current port.
nextport	*char		Generic pointer to next port,
								NULL if not found.
------------------------------------------------------------------------
*/

CHAR  *Sc_next_leaf_port(CHAR *curport)
{
	PORTPROTO	*pp;

	pp = ((PORTPROTO *)curport)->nextportproto;
	if (pp == NOPORTPROTO) return((CHAR *)NULL);
	return((CHAR *)pp);
}

/***********************************************************************
Module:  Sc_leaf_port_bits
------------------------------------------------------------------------
Description:
	Return the bits field of a leaf port.
------------------------------------------------------------------------
Calling Sequence:  bits = Sc_leaf_port_bits(leafport);

Name		Type		Description
----		----		-----------
leafport	*char		Pointer to port.
bits		int			Value of the bits field.
------------------------------------------------------------------------
*/

int Sc_leaf_port_bits(CHAR *leafport)
{
	VARIABLE	*var;

	/* check if variable exits */
	var = getvalkey((INTBIG)leafport, VPORTPROTO, VINTEGER, sc_bitskey);
	if (var == NOVARIABLE)
	{
		if (setvalkey((INTBIG)leafport, VPORTPROTO, sc_bitskey, 0, VINTEGER) ==
			NOVARIABLE) ttyputmsg(_("ERROR creating SCBITS variable."));
		return(0);
	}
	return(var->addr);
}

/***********************************************************************
Module:  Sc_leaf_port_bits_address
------------------------------------------------------------------------
Description:
	Return the address of the bits field of a leaf port.
------------------------------------------------------------------------
Calling Sequence:  bits_a = Sc_leaf_port_bits_address(leafport);

Name		Type		Description
----		----		-----------
leafport	*char		Pointer to port.
bits_a		*int		Address of the bits field.
------------------------------------------------------------------------
*/

int  *Sc_leaf_port_bits_address(CHAR *leafport)
{
	VARIABLE	*var;
	static int retval;

	/* check if variable exits */
	var = getvalkey((INTBIG)leafport, VPORTPROTO, VINTEGER, sc_bitskey);
	if (var == NOVARIABLE)
	{
		if (setvalkey((INTBIG)leafport, VPORTPROTO, sc_bitskey, 0, VINTEGER) ==
			NOVARIABLE) ttyputmsg(_("ERROR creating SCBITS variable."));
		if ((var = getvalkey((INTBIG)leafport, VPORTPROTO, VINTEGER, sc_bitskey))
			== NOVARIABLE)
		{
			ttyputmsg(_("ERROR creating SCBITS variable."));
			return((int *)NULL);
		}
	}
	retval = var->addr;
	return(&retval);
}

/***********************************************************************
Module:  Sc_leaf_port_type
------------------------------------------------------------------------
Description:
	Return the type of the leaf port.
------------------------------------------------------------------------
Calling Sequence:  type = Sc_leaf_port_type(leafport);

Name		Type		Description
----		----		-----------
leafport	*char		Pointer to leaf port.
type		int			Returned type.
------------------------------------------------------------------------
*/

int Sc_leaf_port_type(CHAR *leafport)
{
	if (portispower((PORTPROTO *)leafport)) return(SCPWRPORT);
	if (portisground((PORTPROTO *)leafport)) return(SCGNDPORT);

	switch (((PORTPROTO *)leafport)->userbits & STATEBITS)
	{
		case BIDIRPORT: return(SCBIDIRPORT);
		case OUTPORT:   return(SCOUTPORT);
		case INPORT:    return(SCINPORT);
	}
	return(SCUNPORT);
}

/***********************************************************************
Module:  Sc_set_leaf_port_type
------------------------------------------------------------------------
Description:
	Set the type of the leaf port.
------------------------------------------------------------------------
Calling Sequence:  Sc_set_leaf_port_type(leafport, type);

Name		Type		Description
----		----		-----------
leafport	*PORTPROTO	Pointer to leaf port.
type		int			Type to set (eg. input, output, etc.).
------------------------------------------------------------------------
*/

void Sc_set_leaf_port_type(PORTPROTO *leafport, int type)
{
	leafport->userbits &= ~STATEBITS;
	switch (type)
	{
		case SCGNDPORT:
			leafport->userbits |= GNDPORT;
			break;
		case SCPWRPORT:
			leafport->userbits |= PWRPORT;
			break;
		case SCBIDIRPORT:
			leafport->userbits |= BIDIRPORT;
			break;
		case SCOUTPORT:
			leafport->userbits |= OUTPORT;
			break;
		case SCINPORT:
			leafport->userbits |= INPORT;
			break;
		default:
			leafport->userbits |= GNDPORT;
			break;
	}
	return;
}

/***********************************************************************
Module:  Sc_leaf_port_direction
------------------------------------------------------------------------
Description:
	Return the directions that a port can be attached to (i.e. up,
	down, left, right).
------------------------------------------------------------------------
Calling Sequence:  direct = Sc_leaf_port_direction(port);

Name		Type		Description
----		----		-----------
port		*char		Pointer to port.
direct		int			Returned direction for attaching.
------------------------------------------------------------------------
*/

int Sc_leaf_port_direction(CHAR *port)
{
	int		direct, bits;

	direct = 0;
	bits = Sc_leaf_port_bits(port);
	if (bits & SCPORTDIRMASK) direct = bits & SCPORTDIRMASK;
		else
	{
		/* if no direction specified, assume both up and down */
		direct |= SCPORTDIRUP;
		direct |= SCPORTDIRDOWN;
	}

	return(direct);
}

/***********************************************************************
Module:  Sc_leaf_port_xpos
------------------------------------------------------------------------
Description:
	Return the xpos of the indicated leaf port from the left side of
	it's parent leaf cell.
------------------------------------------------------------------------
Calling Sequence:  xpos = Sc_leaf_port_xpos(port);

Name		Type		Description
----		----		-----------
port		*char		Pointer to leaf port.
xpos		int			Returned position from left side of cell.
------------------------------------------------------------------------
*/

int Sc_leaf_port_xpos(CHAR *port)
{
	INTBIG		x, y;

	if (port != 0)
	{
		portposition(((PORTPROTO *)port)->subnodeinst,
			((PORTPROTO *)port)->subportproto, &x, &y);
		return(x - ((PORTPROTO *)port)->parent->lowx);
	}
	return(0);
}

/***********************************************************************
Module:  Sc_leaf_port_ypos
------------------------------------------------------------------------
Description:
	Return the ypos of the indicated leaf port from the bottom of
	it's parent leaf cell.
------------------------------------------------------------------------
Calling Sequence:  ypos = Sc_leaf_port_ypos(port);

Name		Type		Description
----		----		-----------
port		*char		Pointer to leaf port.
ypos		int			Returned position from bottom of cell.
------------------------------------------------------------------------
*/

int Sc_leaf_port_ypos(CHAR *port)
{
	INTBIG		x, y;

	if (port != 0)
	{
		portposition(((PORTPROTO *)port)->subnodeinst,
			((PORTPROTO *)port)->subportproto, &x, &y);
		return(y - ((PORTPROTO *)port)->parent->lowy);
	}
	return(0);
}

/***********************************************************************
Module:  Sc_leaf_port_set_first
------------------------------------------------------------------------
Description:
	Set the first port of the indicated leaf cell to the indicated
	leaf port.
------------------------------------------------------------------------
Calling Sequence:  Sc_leaf_port_set_first(leafcell, leafport);

Name		Type		Description
----		----		-----------
leafcell	*char		Pointer to leaf cell.
leafport	*char		Pointer to leaf port.
------------------------------------------------------------------------
*/

void Sc_leaf_port_set_first(CHAR *leafcell, CHAR *leafport)
{
	((NODEPROTO *)leafcell)->firstportproto = (PORTPROTO *)leafport;
}

/***********************************************************************
Module:  Sc_leaf_port_set_next
------------------------------------------------------------------------
Description:
	Set the next port of the indicated leaf port to the indicated
	leaf port.  If the second leaf port is NULL, set to NOPORTPROTO.
------------------------------------------------------------------------
Calling Sequence:  Sc_leaf_port_set_next(leafport1, leafport2);

Name		Type		Description
----		----		-----------
leafport1	*char		Pointer to first leaf port.
leafport2	*char		Pointer to second leaf port.
------------------------------------------------------------------------
*/

void Sc_leaf_port_set_next(CHAR *leafport1, CHAR *leafport2)
{
	if (leafport2 != 0)
	{
		((PORTPROTO *)leafport1)->nextportproto = (PORTPROTO *)leafport2;
	} else
	{
		((PORTPROTO *)leafport1)->nextportproto = NOPORTPROTO;
	}
}

/***********************************************************************
Module:  Sc_library_read
------------------------------------------------------------------------
Description:
	Read the specified library.  Set error codes.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_library_read(libname);

Name		Type		Description
----		----		-----------
libname		*char		String giving name of library.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_library_read(CHAR *libname)
{
	LIBRARY    *lib;

	/* check to see if library already exists */
	for (lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if (namesame(libname, lib->libname) == 0)
			break;
	}
	if (lib == NOLIBRARY)
	{
		if ((lib = newlibrary(libname, libname)) == NOLIBRARY)
			return(Sc_seterrmsg(SC_LIBNOMAKE, libname));
	} else
	{
		eraselibrary(lib);
		if (reallocstring(&lib->libfile, libname, lib->cluster))
			return(Sc_seterrmsg(SC_NOMEMORY));
	}
	if (asktool(io_tool, x_("read"), (INTBIG)lib, x_("binary")) != 0)
	{
		eraselibrary(lib);
		return(Sc_seterrmsg(SC_LIBNOREAD, libname));
	}
	selectlibrary(lib, TRUE);
	return(0);
}

/***********************************************************************
Module:  Sc_library_use
------------------------------------------------------------------------
Description:
	Use the specified library as a cell library.  Set error codes.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_library_use(libname);

Name		Type		Description
----		----		-----------
libname		*char		String giving name of library.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_library_use(CHAR *libname)
{
	LIBRARY    *lib;

	/* check to see if library already exists */
	for (lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if (namesame(libname, lib->libname) == 0) break;
	if (lib == NOLIBRARY)
	{
		return(Sc_seterrmsg(SC_NOLIBRARY, libname));
	}
	sc_celllibrary = lib;
	return(0);
}

/***********************************************************************
Module:  Sc_setup_for_maker
------------------------------------------------------------------------
Description:
	Set up the future creation of the maker by setting the appropriate
	prototypes.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_setup_for_maker();

Name		Type		Description
----		----		-----------
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_setup_for_maker(ARCPROTO *layer1, ARCPROTO *layer2)
{
	REGISTER ARCPROTO *ap, *alternatearc, **arclist;
	REGISTER int fun, i;
	REGISTER TECHNOLOGY *tech;
	static POLYGON *poly = NOPOLYGON;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly, 4, sc_tool->cluster);

	/* find the horizontal (layer 1) and vertical (layer 2) arcs */
	sc_layer1arc = sc_layer2arc = alternatearc = NOARCPROTO;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		switch ((ap->userbits&AFUNCTION)>>AFUNCTIONSH)
		{
			case APMETAL1:
				sc_layer1arc = ap;
				break;
			case APMETAL2:  case APMETAL3:  case APMETAL4: case APMETAL5:
			case APMETAL6:  case APMETAL7:  case APMETAL8: case APMETAL9:
			case APMETAL10: case APMETAL11: case APMETAL12:
				sc_layer2arc = ap;
				break;
			case APPOLY1: case APPOLY2: case APPOLY3:
				alternatearc = ap;
				break;
		}
	}
	if (sc_layer1arc == NOARCPROTO) sc_layer1arc = alternatearc;

	/* allow overrides */
	if (layer1 != NOARCPROTO) sc_layer1arc = layer1;
	if (layer2 != NOARCPROTO) sc_layer2arc = layer2;
	if (sc_layer1arc == NOARCPROTO) return(Sc_seterrmsg(SC_NO_LAYER1_ARC));
	if (sc_layer2arc == NOARCPROTO) return(Sc_seterrmsg(SC_NO_LAYER2_ARC));

	/* use technology of the first arc */
	tech = sc_layer1arc->tech;

	/* find the contact between the two layers */
	for(sc_viaproto = tech->firstnodeproto; sc_viaproto != NONODEPROTO;
		sc_viaproto = sc_viaproto->nextnodeproto)
	{
		fun = (sc_viaproto->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPCONTACT && fun != NPCONNECT) continue;
		arclist = sc_viaproto->firstportproto->connects;
		for(i=0; arclist[i] != NOARCPROTO; i++)
			if (arclist[i] == sc_layer1arc) break;
		if (arclist[i] == NOARCPROTO) continue;
		for(i=0; arclist[i] != NOARCPROTO; i++)
			if (arclist[i] == sc_layer2arc) break;
		if (arclist[i] != NOARCPROTO) break;
	}
	if (sc_viaproto == NONODEPROTO) return(Sc_seterrmsg(SC_NO_VIA));

	/* find the pin nodes on the connecting layers */
	sc_layer1proto = getpinproto(sc_layer1arc);
	if (sc_layer1proto == NONODEPROTO) return(Sc_seterrmsg(SC_NO_LAYER1_NODE));
	sc_layer2proto = getpinproto(sc_layer2arc);
	if (sc_layer2proto == NONODEPROTO) return(Sc_seterrmsg(SC_NO_LAYER2_NODE));

	/*
	 * find the pure-layer node on the P-well layer
	 * if the p-well size is zero don't look for the node
	 * allows technologies without p-wells to be routed (i.e. GaAs)
	 */
	if (ScGetParameter(SC_PARAM_MAKE_PWELL_SIZE) == 0) sc_pwellproto = NONODEPROTO; else
	{
		sc_pwellproto = getpurelayernode(tech, -1, LFWELL|LFPTYPE);
		if (sc_pwellproto == NONODEPROTO) return(Sc_seterrmsg(SC_NO_LAYER_PWELL));
	}
	if (ScGetParameter(SC_PARAM_MAKE_NWELL_SIZE) == 0) sc_nwellproto = NONODEPROTO; else
	{
		sc_nwellproto = getpurelayernode(tech, -1, LFWELL|LFNTYPE);
		if (sc_nwellproto == NONODEPROTO) return(Sc_seterrmsg(SC_NO_LAYER_PWELL));
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_create_leaf_cell
------------------------------------------------------------------------
Description:
	Create a new leaf cell of the indicated name in the VLSI Layout Tool
	Environment.  Return NULL on error.
------------------------------------------------------------------------
Calling Sequence:  bcell = Sc_create_leaf_cell(name);

Name		Type		Description
----		----		-----------
name		*char		Name of new leaf cell.
bcell		*char		Pointer to created leaf cell.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_leaf_cell(CHAR *name)
{
	NODEPROTO	*bcell;
	CHAR	*view, layoutname[256];

	/*
	 * use layoutname because we don't know how
	 * long "name" is
	 */
	(void) estrcpy (layoutname, name);

	if ((view = estrchr (layoutname, '{')) == NULL)
		(void) estrcat (layoutname, x_("{lay}")); else
			if (estrncmp (view, x_("{lay}"), 5) != 0)
	{
		view = '\0';
		(void) estrcat (layoutname, x_("{lay}"));
	}

	bcell = us_newnodeproto(layoutname, el_curlib);
	if (bcell == NONODEPROTO) return((CHAR *)NULL);
	return((CHAR *)bcell);
}

/***********************************************************************
Module:  Sc_create_leaf_instance
------------------------------------------------------------------------
Description:
	Create a new leaf instance of the indicated type in the VLSI Layout
	Tool Environment.  Return NULL on error.
------------------------------------------------------------------------
Calling Sequence:  binst = Sc_create_leaf_instance(name, type, minx, maxx,
								miny, maxy, transpose, rotation, parent);

Name		Type		Description
----		----		-----------
name		*char		Name of new leaf instance.
type		*char		Pointer to leaf cell type.
minx, maxx	int			Bounds in X.
miny, maxy	int			Bounds in Y.
transpose	int			Transposition flag (TRUE == transpose).
rotation	int			Degrees of rotation.
parent		*char		Cell in which to create instance.
binst		*char		Pointer to created leaf instance.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_leaf_instance(CHAR *name, CHAR *type, int minx, int maxx, int miny,
	int maxy, int transpose, int rotation, CHAR *parent)
{
	NODEINST	*binst;
	REGISTER VARIABLE *var;

	binst = newnodeinst((NODEPROTO *)type, minx, maxx, miny, maxy, transpose,
		rotation, (NODEPROTO *)parent);
	if (binst == NONODEINST) return((CHAR *)NULL);

	var = setvalkey((INTBIG)binst, VNODEINST, el_node_name_key, (INTBIG)name, VSTRING|VDISPLAY);
	if (var == NOVARIABLE)
	{
		ttyputmsg(_("ERROR creating name of instance '%s'."), name);
	} else
	{
		defaulttextsize(3, var->textdescript);
	}
	return((CHAR *)binst);
}

/***********************************************************************
Module:  Sc_create_layer1_node
------------------------------------------------------------------------
Description:
	Create a node in layer1 in the indicated cell at the given position.
------------------------------------------------------------------------
Calling Sequence:  binst = Sc_create_layer1_node(xpos, ypos, xsize,
								ysize, bcell);

Name		Type		Description
----		----		-----------
xpos		int			Center X position.
ypos		int			Center Y position.
xsize		int			Size in X.
ysize		int			Size in Y.
bcell		*char		Cell in which to create feed.
binst		*char		Returned pointer to created instance.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_layer1_node(int xpos, int ypos, int xsize, int ysize, CHAR *bcell)
{
	NODEINST	*binst;
	int		lowx, highx, lowy, highy;

	lowx = xpos - (xsize >> 1);
	highx = lowx + xsize;
	lowy = ypos - (ysize >> 1);
	highy = lowy + ysize;
	binst = newnodeinst(sc_layer1proto, lowx, highx, lowy, highy, 0, 0,
		(NODEPROTO *)bcell);
	if (binst == NONODEINST) return((CHAR *)NULL);
	return((CHAR *)binst);
}

/***********************************************************************
Module:  Sc_create_layer2_node
------------------------------------------------------------------------
Description:
	Create a layer2 node in the indicated cell at the given position.
------------------------------------------------------------------------
Calling Sequence:  binst = Sc_create_layer2_node(xpos, ypos, xsize,
								ysize, bcell);

Name		Type		Description
----		----		-----------
xpos		int			Center X position.
ypos		int			Center Y position.
xsize		int			Width in X direction.
ysize		int			Size in Y direction.
bcell		*char		Cell in which to create feed.
binst		*char		Returned pointer to created instance.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_layer2_node(int xpos, int ypos, int xsize, int ysize, CHAR *bcell)
{
	NODEINST	*binst;
	int		lowx, highx, lowy, highy;

	lowx =  xpos - (xsize >> 1);
	highx = lowx + xsize;
	lowy = ypos - (ysize >> 1);
	highy = lowy + ysize;
	binst = newnodeinst(sc_layer2proto, lowx, highx, lowy, highy, 0, 0,
		(NODEPROTO *)bcell);
	if (binst == NONODEINST) return((CHAR *)NULL);
	return((CHAR *)binst);
}

/***********************************************************************
Module:  Sc_create_via
------------------------------------------------------------------------
Description:
	Create a via in the indicated cell at the given position.
------------------------------------------------------------------------
Calling Sequence:  binst = Sc_create_via(xpos, ypos, bcell);

Name		Type		Description
----		----		-----------
xpos		int			Center X position.
ypos		int			Center Y position.
bcell		*char		Cell in which to create feed.
binst		*char		Returned pointer to created instance.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_via(int xpos, int ypos, CHAR *bcell)
{
	NODEINST	*binst;
	int		lowx, highx, lowy, highy;
	INTBIG pxs, pys;

	defaultnodesize(sc_viaproto, &pxs, &pys);
	lowx = xpos - pxs / 2;
	highx = lowx + pxs;
	lowy = ypos - pys / 2;
	highy = lowy + pys;
	binst = newnodeinst(sc_viaproto, lowx, highx, lowy, highy, 0, 0,
		(NODEPROTO *)bcell);
	if (binst == NONODEINST) return((CHAR *)NULL);
	return((CHAR *)binst);
}

/***********************************************************************
Module:  Sc_create_pwell
------------------------------------------------------------------------
Description:
	Create a pwell in the indicated cell at the given position.
------------------------------------------------------------------------
Calling Sequence:  binst = Sc_create_pwell(xpos, ypos, xsize, ysize, bcell);

Name		Type		Description
----		----		-----------
xpos		int			Center X position.
ypos		int			Center Y position.
xsize		int			Size in X.
ysize		int			Size in Y.
bcell		*char		Cell in which to create feed.
binst		*char		Returned pointer to created instance.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_pwell(int xpos, int ypos, int xsize, int ysize, CHAR *bcell)
{
	NODEINST	*binst;
	int		lowx, highx, lowy, highy;

	if (sc_pwellproto == NONODEPROTO) return((CHAR *)NULL);

	lowx = xpos - (xsize >> 1);
	highx = lowx + xsize;
	lowy = ypos - (ysize >> 1);
	highy = lowy + ysize;
	binst = newnodeinst(sc_pwellproto, lowx, highx, lowy, highy, 0, 0,
		(NODEPROTO *)bcell);
	if (binst == NONODEINST) return((CHAR *)NULL);
	return((CHAR *)binst);
}

/***********************************************************************
Module:  Sc_create_nwell
------------------------------------------------------------------------
Description:
	Create a nwell in the indicated cell at the given position.
	Added 2/11/01 David_Harris@hmc.edu
------------------------------------------------------------------------
Calling Sequence:  binst = Sc_create_nwell(xpos, ypos, xsize, ysize, bcell);

Name		Type		Description
----		----		-----------
xpos		int			Center X position.
ypos		int			Center Y position.
xsize		int			Size in X.
ysize		int			Size in Y.
bcell		*char		Cell in which to create feed.
binst		*char		Returned pointer to created instance.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_nwell(int xpos, int ypos, int xsize, int ysize, CHAR *bcell)
{
	NODEINST	*binst;
	int		lowx, highx, lowy, highy;

	if (sc_nwellproto == NONODEPROTO) return((CHAR *)NULL);

	lowx = xpos - (xsize >> 1);
	highx = lowx + xsize;
	lowy = ypos - (ysize >> 1);
	highy = lowy + ysize;
	binst = newnodeinst(sc_nwellproto, lowx, highx, lowy, highy, 0, 0,
		(NODEPROTO *)bcell);
	if (binst == NONODEINST) return((CHAR *)NULL);
	return((CHAR *)binst);
}

/***********************************************************************
Module:  Sc_create_track_layer1
------------------------------------------------------------------------
Description:
	Create a track between the two given ports on the first routing
	layer.  Note that ports of primitive instances are passed as NULL
	and must be determined.
------------------------------------------------------------------------
Calling Sequence:  inst = Sc_create_track_layer1(instA, portA,
								instB, portB, width, bcell);

Name		Type		Description
----		----		-----------
instA		*char		Pointer to first instance.
portA		*char		Pointer to first port.
instB		*char		Pointer to second instance.
portB		*char		Pointer to second port.
width		int			Width of track.
bcell		*char		Pointer to cell in which to create.
inst		*char		Returned pointer to created track.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_track_layer1(CHAR *instA, CHAR *portA, CHAR *instB, CHAR *portB,
	int width, CHAR *bcell)
{
	PORTPROTO *pa, *pb;
	NODEINST *na, *nb;
	ARCINST *inst;
	INTBIG xA, yA, xB, yB, i;

	/* copy into internal structures */
	na = (NODEINST *)instA;
	nb = (NODEINST *)instB;
	if (portA != NULL) pa = (PORTPROTO *)portA; else
		pa = na->proto->firstportproto;
	if (portB != NULL) pb = (PORTPROTO *)portB; else
		pb = nb->proto->firstportproto;

	/* find center positions */
	portposition(na, pa, &xA, &yA);
	portposition(nb, pb, &xB, &yB);

	/* make sure the arc can connect */
	for(i=0; pa->connects[i] != NOARCPROTO; i++)
		if (pa->connects[i] == sc_layer1arc) break;
	if (pa->connects[i] == NOARCPROTO)
	{
		/* must place a via */
		Sc_create_connection(&na, &pa, xA, yA, sc_layer1arc);
	}
	for(i=0; pb->connects[i] != NOARCPROTO; i++)
		if (pb->connects[i] == sc_layer1arc) break;
	if (pb->connects[i] == NOARCPROTO)
	{
		/* must place a via */
		Sc_create_connection(&nb, &pb, xB, yB, sc_layer1arc);
	}

	inst = newarcinst(sc_layer1arc, width, FIXANG, na, pa, xA, yA,
		nb, pb, xB, yB, (NODEPROTO *)bcell);
	if (inst == NOARCINST) return((CHAR *)NULL);
	return((CHAR *)inst);
}

/***********************************************************************
Module:  Sc_create_track_layer2
------------------------------------------------------------------------
Description:
	Create a track between the two given ports on the second routing
	layer.  Note that ports of primitive instances are passed as NULL
	and must be determined.
------------------------------------------------------------------------
Calling Sequence:  inst = Sc_create_track_layer2(instA, portA,
								instB, portB, width, bcell);

Name		Type		Description
----		----		-----------
instA		*char		Pointer to first instance.
portA		*char		Pointer to first port.
instB		*char		Pointer to second instance.
portB		*char		Pointer to second port.
width		int			Width of track.
bcell		*char		Pointer to cell in which to create.
inst		*char		Returned pointer to created track.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_track_layer2(CHAR *instA, CHAR *portA, CHAR *instB, CHAR *portB,
	int width, CHAR *bcell)
{
	PORTPROTO *pa, *pb;
	NODEINST *na, *nb;
	ARCINST *inst;
	INTBIG xA, yA, xB, yB, i;

	/* copy into internal structures */
	na = (NODEINST *)instA;
	nb = (NODEINST *)instB;
	if (portA != NULL) pa = (PORTPROTO *)portA; else
		pa = na->proto->firstportproto;
	if (portB != NULL) pb = (PORTPROTO *)portB; else
		pb = nb->proto->firstportproto;

	/* find center positions */
	portposition(na, pa, &xA, &yA);
	portposition(nb, pb, &xB, &yB);

	/* make sure the arc can connect */
	for(i=0; pa->connects[i] != NOARCPROTO; i++)
		if (pa->connects[i] == sc_layer2arc) break;
	if (pa->connects[i] == NOARCPROTO)
	{
		/* must place a via */
		Sc_create_connection(&na, &pa, xA, yA, sc_layer2arc);
	}
	for(i=0; pb->connects[i] != NOARCPROTO; i++)
		if (pb->connects[i] == sc_layer2arc) break;
	if (pb->connects[i] == NOARCPROTO)
	{
		/* must place a via */
		Sc_create_connection(&nb, &pb, xB, yB, sc_layer2arc);
	}

	inst = newarcinst(sc_layer2arc, width, FIXANG, na, pa, xA, yA,
		nb, pb, xB, yB, (NODEPROTO *)bcell);
	if (inst == NOARCINST) return((CHAR *)NULL);
	return((CHAR *)inst);
}

void Sc_create_connection(NODEINST **ni, PORTPROTO **pp, INTBIG x, INTBIG y,
	ARCPROTO *arc)
{
	REGISTER NODEPROTO *via;
	REGISTER NODEINST *vianode;
	REGISTER INTBIG fun, i, j, lx, hx, ly, hy, wid;
	INTBIG sx, sy;
	REGISTER ARCPROTO **arclist, **niarclist;
	REGISTER ARCINST *zeroarc;

	niarclist = (*pp)->connects;

	/* always use the standard via (David Harris) */
	via = sc_viaproto;
	if (via == NONODEPROTO) return;
	fun = (via->userbits&NFUNCTION) >> NFUNCTIONSH;
	if (fun != NPCONTACT && fun != NPCONNECT) return;
	arclist = via->firstportproto->connects;

	/* make sure that this contact connects to the desired arc */
	for(i=0; arclist[i] != NOARCPROTO; i++)
		if (arclist[i] == arc) break;
	if (arclist[i] == NOARCPROTO) return;

	/* make sure that this contact connects to the initial node */
	for(i=0; arclist[i] != NOARCPROTO; i++)
	{
		for(j=0; niarclist[j] != NOARCPROTO; j++)
		{
			if (arclist[i] == niarclist[j]) break;
		}
		if (niarclist[j] != NOARCPROTO) break;
	}

	/* use this via to make the connection */
	defaultnodesize(via, &sx, &sy);
	lx = x - sx/2;   hx = lx + sx;
	ly = y - sy/2;   hy = ly + sy;
	vianode = newnodeinst(via, lx, hx, ly, hy, 0, 0, (*ni)->parent);
	if (vianode == NONODEINST) return;
	wid = defaultarcwidth(arclist[i]);
	zeroarc = newarcinst(arclist[i], wid, FIXED, *ni, *pp, x, y,
		vianode, via->firstportproto, x, y, (*ni)->parent);
	if (zeroarc == NOARCINST) return;
	*ni = vianode;
	*pp = via->firstportproto;
}

/***********************************************************************
Module:  Sc_create_export_port
------------------------------------------------------------------------
Description:
	Create an export at the given instance at the given port.
	Note that ports of primitive instances are passed as NULL and must
	be determined.
------------------------------------------------------------------------
Calling Sequence:  xport = Sc_create_export_port(inst, port, name,
								type, bcell);

Name		Type		Description
----		----		-----------
instA		*char		Pointer to instance.
port		*char		Pointer to port.
name		*char		Pointer to name.
type		int			Type of port (eg. input, output, etc.)
bcell		*char		Pointer to cell in which to create.
xport		*char		Returned pointer to created port.
------------------------------------------------------------------------
*/

CHAR  *Sc_create_export_port(CHAR *inst, CHAR *port, CHAR *name, int type, CHAR *bcell)
{
	PORTPROTO	*xport;

	/* check if primative */
	if ((PORTPROTO *)port == NULL)
		port = (CHAR *)(((NODEINST *)inst)->proto->firstportproto);

	xport = newportproto((NODEPROTO *)bcell, (NODEINST *)inst,
		(PORTPROTO *)port, name);
	if (xport == NOPORTPROTO) return((CHAR *)NULL);
	Sc_set_leaf_port_type(xport, type);
	return((CHAR *)xport);
}

/*
 * routine to examine a schematic from the current cell
 */
void Sc_schematic(void)
{
	ttyputmsg(_("Sorry, cannot handle schematics yet"));
}

/****************************** DIALOG ******************************/

/* Silicon Compiler Options */
static DIALOGITEM sc_optionsdialogitems[] =
{
 /*  1 */ {0, {500,268,524,326}, BUTTON, N_("OK")},
 /*  2 */ {0, {500,36,524,94}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {8,204,24,384}, POPUP, x_("")},
 /*  4 */ {0, {32,204,48,276}, EDITTEXT, x_("")},
 /*  5 */ {0, {8,8,24,180}, MESSAGE, N_("Horizontal routing arc:")},
 /*  6 */ {0, {60,8,76,180}, MESSAGE, N_("Vertical routing arc:")},
 /*  7 */ {0, {32,8,48,174}, MESSAGE, N_("Horizontal wire width:")},
 /*  8 */ {0, {60,204,76,384}, POPUP, x_("")},
 /*  9 */ {0, {84,8,100,174}, MESSAGE, N_("Vertical wire width:")},
 /* 10 */ {0, {84,204,100,276}, EDITTEXT, x_("")},
 /* 11 */ {0, {120,8,136,174}, MESSAGE, N_("Power wire width:")},
 /* 12 */ {0, {120,204,136,276}, EDITTEXT, x_("")},
 /* 13 */ {0, {144,8,160,174}, MESSAGE, N_("Main power wire width:")},
 /* 14 */ {0, {144,204,160,276}, EDITTEXT, x_("")},
 /* 15 */ {0, {204,8,220,199}, MESSAGE, N_("P-Well height (0 for none):")},
 /* 16 */ {0, {168,204,184,352}, POPUP, x_("")},
 /* 17 */ {0, {228,8,244,199}, MESSAGE, N_("P-Well offset from bottom:")},
 /* 18 */ {0, {204,204,220,276}, EDITTEXT, x_("")},
 /* 19 */ {0, {320,8,336,199}, MESSAGE, N_("Via size:")},
 /* 20 */ {0, {232,204,248,276}, EDITTEXT, x_("")},
 /* 21 */ {0, {344,8,360,199}, MESSAGE, N_("Minimum metal spacing:")},
 /* 22 */ {0, {256,204,272,276}, EDITTEXT, x_("")},
 /* 23 */ {0, {384,8,400,199}, MESSAGE, N_("Routing: feed-through size:")},
 /* 24 */ {0, {280,204,296,276}, EDITTEXT, x_("")},
 /* 25 */ {0, {408,8,424,199}, MESSAGE, N_("Routing: min. port distance:")},
 /* 26 */ {0, {320,204,336,276}, EDITTEXT, x_("")},
 /* 27 */ {0, {432,8,448,199}, MESSAGE, N_("Routing: min. active dist.:")},
 /* 28 */ {0, {344,204,360,276}, EDITTEXT, x_("")},
 /* 29 */ {0, {472,8,488,199}, MESSAGE, N_("Number of rows of cells:")},
 /* 30 */ {0, {384,204,400,276}, EDITTEXT, x_("")},
 /* 31 */ {0, {408,204,424,276}, EDITTEXT, x_("")},
 /* 32 */ {0, {168,8,184,180}, MESSAGE, N_("Main power arc:")},
 /* 33 */ {0, {256,8,272,199}, MESSAGE, N_("N-Well height (0 for none):")},
 /* 34 */ {0, {432,204,448,276}, EDITTEXT, x_("")},
 /* 35 */ {0, {280,8,296,199}, MESSAGE, N_("N-Well offset from top:")},
 /* 36 */ {0, {472,204,488,276}, EDITTEXT, x_("")}
};
static DIALOG sc_optionsdialog = {{50,75,583,473}, N_("Silicon Compiler Options"), 0, 36, sc_optionsdialogitems, 0, 0};

/* special items for the "Silicon Compiler Options" dialog: */
#define DSCO_HORIZARCS     3		/* Horizontal arc list (popup) */
#define DSCO_HORIZWIDTH    4		/* Horizontal arc width (edit text) */
#define DSCO_VERTARCS      8		/* Vertical arc list (popup) */
#define DSCO_VERTWIDTH    10		/* Vertical arc width (edit text) */
#define DSCO_POWERWIDTH   12		/* Power arc width (edit text) */
#define DSCO_MPOWERWIDTH  14		/* Main power arc width (edit text) */
#define DSCO_MPOWERARC    16		/* Main power arc (popup) */
#define DSCO_PWELLHEIGHT  18		/* P-Well height (edit text) */
#define DSCO_PWELLOFFSET  20		/* P-Well offset (edit text) */
#define DSCO_NWELLHEIGHT  22		/* N-Well height (edit text) */
#define DSCO_NWELLOFFSET  24		/* N-Well offset (edit text) */
#define DSCO_VIASIZE      26		/* Via size (edit text) */
#define DSCO_MINSPACING   28		/* Minimum spacing (edit text) */
#define DSCO_FEEDTHRU     30		/* Routing feed-through size (edit text) */
#define DSCO_PORTDIST     31		/* Routing minimum port dist (edit text) */
#define DSCO_ACTIVEDIST   34		/* Routing minimum active dist (edit text) */
#define DSCO_NUMROWS      36		/* Number of rows of cells (edit text) */

void sc_optionsdlog(void)
{
	INTBIG itemHit, numarcnames, horizindex, vertindex, i, l1width, l2width,
		pwrwidth, mainpwrwidth, pwellsize, pwelloffset, nwellsize, nwelloffset,
		viasize, minspacing, feedthrusize, portxmindist, activedist, numrows,
		mainpwrrail;
	REGISTER ARCPROTO *ap, *aphoriz, *apvert;
	CHAR **arcnames, numstring[20], *transnames[2];
	REGISTER void *dia;
	static CHAR *powerarcnames[2] = {N_("Horizontal Arc"), N_("Vertical Arc")};

	/* get parameters */
	aphoriz      = (ARCPROTO *)ScGetParameter(SC_PARAM_MAKE_HORIZ_ARC);
	apvert       = (ARCPROTO *)ScGetParameter(SC_PARAM_MAKE_VERT_ARC);
	l1width      = ScGetParameter(SC_PARAM_MAKE_L1_WIDTH);
	l2width      = ScGetParameter(SC_PARAM_MAKE_L2_WIDTH);
	pwrwidth     = ScGetParameter(SC_PARAM_MAKE_PWR_WIDTH);
	mainpwrwidth = ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH);
	mainpwrrail  = ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL);
	pwellsize    = ScGetParameter(SC_PARAM_MAKE_PWELL_SIZE);
	pwelloffset  = ScGetParameter(SC_PARAM_MAKE_PWELL_OFFSET);
	nwellsize    = ScGetParameter(SC_PARAM_MAKE_NWELL_SIZE);
	nwelloffset  = ScGetParameter(SC_PARAM_MAKE_NWELL_OFFSET);
	viasize      = ScGetParameter(SC_PARAM_MAKE_VIA_SIZE);
	minspacing   = ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);
	feedthrusize = ScGetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE);
	portxmindist = ScGetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST);
	activedist   = ScGetParameter(SC_PARAM_ROUTE_ACTIVE_DIST);
	numrows      = ScGetParameter(SC_PARAM_PLACE_NUM_ROWS);

	/* gather list of arcs */
	numarcnames = 0;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		numarcnames++;
	numarcnames++;
	arcnames = (CHAR **)emalloc(numarcnames * (sizeof (CHAR *)), el_tempcluster);
	if (arcnames == 0) return;
	numarcnames = 0;
	arcnames[numarcnames++] = _("NOT SELECTED");
	horizindex = vertindex = 0;
	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		arcnames[numarcnames] = ap->protoname;
		if (aphoriz == ap) horizindex = numarcnames;
		if (apvert == ap) vertindex = numarcnames;
		numarcnames++;
	}

	/* display the silicon compiler options dialog box */
	dia = DiaInitDialog(&sc_optionsdialog);
	if (dia == 0) return;
	for(i=0; i<2; i++) transnames[i] = TRANSLATE(powerarcnames[i]);
	DiaSetPopup(dia, DSCO_MPOWERARC, 2, transnames);
	DiaSetPopup(dia, DSCO_VERTARCS, numarcnames, arcnames);
	DiaSetPopup(dia, DSCO_HORIZARCS, numarcnames, arcnames);
	DiaSetPopupEntry(dia, DSCO_VERTARCS, vertindex);
	DiaSetPopupEntry(dia, DSCO_HORIZARCS, horizindex);
	DiaSetText(dia, DSCO_HORIZWIDTH, latoa(l1width, 0));
	DiaSetText(dia, DSCO_VERTWIDTH, latoa(l2width, 0));
	DiaSetText(dia, DSCO_POWERWIDTH, latoa(pwrwidth, 0));
	DiaSetText(dia, DSCO_MPOWERWIDTH, latoa(mainpwrwidth, 0));
	DiaSetText(dia, DSCO_PWELLHEIGHT, latoa(pwellsize, 0));
	DiaSetText(dia, DSCO_PWELLOFFSET, latoa(pwelloffset, 0));
	DiaSetText(dia, DSCO_VIASIZE, latoa(viasize, 0));
	DiaSetText(dia, DSCO_MINSPACING, latoa(minspacing, 0));
	DiaSetText(dia, DSCO_FEEDTHRU, latoa(feedthrusize, 0));
	DiaSetText(dia, DSCO_PORTDIST, latoa(portxmindist, 0));
	DiaSetText(dia, DSCO_ACTIVEDIST, latoa(activedist, 0));
	esnprintf(numstring, 20, x_("%ld"), numrows);
	DiaSetText(dia, DSCO_NUMROWS, numstring);
	DiaSetPopupEntry(dia, DSCO_MPOWERARC, mainpwrrail);
	DiaSetText(dia, DSCO_NWELLHEIGHT, latoa(nwellsize, 0));
	DiaSetText(dia, DSCO_NWELLOFFSET, latoa(nwelloffset, 0));

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}

	if (itemHit != CANCEL)
	{
		i = DiaGetPopupEntry(dia, DSCO_VERTARCS);
		if (i == 0) ap = NOARCPROTO; else
			ap = getarcproto(arcnames[i]);
		if (ap != apvert) ScSetParameter(SC_PARAM_MAKE_VERT_ARC, (INTBIG)ap);
		i = DiaGetPopupEntry(dia, DSCO_HORIZARCS);
		if (i == 0) ap = NOARCPROTO; else
			ap = getarcproto(arcnames[i]);
		if (ap != aphoriz) ScSetParameter(SC_PARAM_MAKE_HORIZ_ARC, (INTBIG)ap);

		i = atola(DiaGetText(dia, DSCO_HORIZWIDTH), 0);
		if (i != l1width) ScSetParameter(SC_PARAM_MAKE_L1_WIDTH, i);
		i = atola(DiaGetText(dia, DSCO_VERTWIDTH), 0);
		if (i != l2width) ScSetParameter(SC_PARAM_MAKE_L2_WIDTH, i);
		i = atola(DiaGetText(dia, DSCO_POWERWIDTH), 0);
		if (i != pwrwidth) ScSetParameter(SC_PARAM_MAKE_PWR_WIDTH, i);
		i = atola(DiaGetText(dia, DSCO_MPOWERWIDTH), 0);
		if (i != mainpwrwidth) ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH, i);
		i = atola(DiaGetText(dia, DSCO_PWELLHEIGHT), 0);
		if (i != pwellsize) ScSetParameter(SC_PARAM_MAKE_PWELL_SIZE, i);
		i = atola(DiaGetText(dia, DSCO_PWELLOFFSET), 0);
		if (i != pwelloffset) ScSetParameter(SC_PARAM_MAKE_PWELL_OFFSET, i);
		i = atola(DiaGetText(dia, DSCO_VIASIZE), 0);
		if (i != viasize) ScSetParameter(SC_PARAM_MAKE_VIA_SIZE, i);
		i = atola(DiaGetText(dia, DSCO_MINSPACING), 0);
		if (i != minspacing) ScSetParameter(SC_PARAM_MAKE_MIN_SPACING, i);
		i = atola(DiaGetText(dia, DSCO_FEEDTHRU), 0);
		if (i != feedthrusize) ScSetParameter(SC_PARAM_ROUTE_FEEDTHRU_SIZE, i);
		i = atola(DiaGetText(dia, DSCO_PORTDIST), 0);
		if (i != portxmindist) ScSetParameter(SC_PARAM_ROUTE_PORT_X_MIN_DIST, i);
		i = atola(DiaGetText(dia, DSCO_ACTIVEDIST), 0);
		if (i != activedist) ScSetParameter(SC_PARAM_ROUTE_ACTIVE_DIST, i);
		i = eatoi(DiaGetText(dia, DSCO_NUMROWS));
		if (i != numrows) ScSetParameter(SC_PARAM_PLACE_NUM_ROWS, i);
		i = DiaGetPopupEntry(dia, DSCO_MPOWERARC);
		if (i != mainpwrrail) ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL, i);
		i = atola(DiaGetText(dia, DSCO_NWELLHEIGHT), 0);
		if (i != nwellsize) ScSetParameter(SC_PARAM_MAKE_NWELL_SIZE, i);
		i = atola(DiaGetText(dia, DSCO_NWELLOFFSET), 0);
		if (i != nwelloffset) ScSetParameter(SC_PARAM_MAKE_NWELL_OFFSET, i);
	}
	efree((CHAR *)arcnames);
	DiaDoneDialog(dia);
}

#endif  /* SCTOOL - at top */
