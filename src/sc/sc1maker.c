/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1maker.c
 * Modules for Making cell created by the QUISC Silicon Compiler
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

#define MAINPWRLAYER2 1

#include "global.h"
#include "sc1.h"
#include <math.h>

/******************** Maker Version Blerb ********************/

static CHAR *sc_makerblerb[] =
{
	M_("*********** QUISC MAKER MODULE - VERSION 1.03"),
	x_(" "),
	M_("    o  Determination of final position"),
	M_("    o  Include squeezing rows in vertical direction"),
	M_("    o  Squeeze tracks together if nonadjacent via"),
	M_("    o  Creating ties to power and ground"),
	M_("    o  Routing Power and Ground buses"),
	M_("    o  Creation in VLSI Layout Tool's database"),
	x_("----------------------------------------------------------"),
	0
} ;


/***********************************************************************
	File Variables
------------------------------------------------------------------------
*/

#define DEFAULT_VERBOSE				0		/* verbose default */

static int sc_makerverbose = DEFAULT_VERBOSE;

/***********************************************************************
	External Variables
------------------------------------------------------------------------
*/

extern SCCELL		*sc_curcell;

/* prototypes for local routines */
static int  Sc_maker_set_control(int, CHAR*[]);
static void Sc_maker_show_control(void);
static int  Sc_maker_create_info(SCCELL*);
static int  Sc_maker_set_up(SCCELL*, SCMAKERDATA**);
static int  Sc_maker_create_layout(SCMAKERDATA*);
static void Sc_free_maker_data(SCMAKERDATA*);

/***********************************************************************
Module:  Sc_maker
------------------------------------------------------------------------
Description:
	Main procedure of QUISC Maker.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_maker(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointer to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_maker(int count, CHAR *pars[])
{
	int i, err, l;
	SCMAKERDATA *make_data;

	/* parameter check */
	if (count)
	{
		l = estrlen(pars[0]);
		l = maxi(l, 2);
		if (namesamen(pars[0], x_("set-control"), l) == 0)
			return(Sc_maker_set_control(count - 1, &pars[1]));
		if (namesamen(pars[0], x_("show-control"), l) == 0)
		{
			Sc_maker_show_control();
			return(SC_NOERROR);
		}
		if (namesamen(pars[0], x_("information"), l) == 0)
		{
			/* EMPTY */ 
		} else
		{
			return(Sc_seterrmsg(SC_MAKE_XCMD, pars[0]));
		}
	}

	for (i = 0; sc_makerblerb[i]; i++)
		ttyputverbose(TRANSLATE(sc_makerblerb[i]));

	/* check if working in a cell */
	if (sc_curcell == NULL)
		return(Sc_seterrmsg(SC_NOCELL));

	/* check if placement structure exists */
	if (sc_curcell->placement == NULL)
		return(Sc_seterrmsg(SC_CELL_NO_PLACE, sc_curcell->name));

	/* check if route structure exists */
	if (sc_curcell->route == NULL)
		return(Sc_seterrmsg(SC_CELL_NO_ROUTE, sc_curcell->name));

	if (count)
	{
		/* information about the cell */
		return(Sc_maker_create_info(sc_curcell));
	}

	/* show controlling parameters */
	if (sc_makerverbose)
		Sc_maker_show_control();

	(void)Sc_cpu_time(TIME_RESET);
	ttyputmsg(_("Starting MAKER..."));

	/* set up make structure */
	if ((err = Sc_maker_set_up(sc_curcell, &make_data)))
		return(err);

	/* create actual layout */
	ttyputmsg(_("Creating cell %s"), make_data->cell->name);
	if ((err = Sc_maker_create_layout(make_data)))
		return(err);

	Sc_free_maker_data(make_data);

	ttyputmsg(_("Done (time = %s)"), Sc_cpu_time(TIME_ABS));

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_maker_set_control
------------------------------------------------------------------------
Description:
	Set the maker control structure values.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_maker_set_control(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointers to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_maker_set_control(int count, CHAR *pars[])
{
	int numcmd, l, n;
	REGISTER ARCPROTO *ap;

	if (count)
	{
		for (numcmd = 0; numcmd < count; numcmd++)
		{
			l = estrlen(pars[numcmd]);
			n = maxi(l, 2);
			if (namesamen(pars[numcmd], x_("verbose"), n) == 0)
			{
				sc_makerverbose = TRUE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("l2-track-width"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_L2_WIDTH, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("l1-track-width"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_L1_WIDTH, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("power-track-width"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_PWR_WIDTH, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("main-power-width"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("main-power-rail"), n) == 0)
			{
				if (++numcmd < count)
				{
					if (namesame(pars[numcmd], x_("horizontal")) == 0)
						ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL, 0);
					if (namesame(pars[numcmd], x_("vertical")) == 0)
						ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL, 1);
				}
				continue;
			}
			if (namesamen(pars[numcmd], x_("via-size"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_VIA_SIZE, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("min-spacing"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_MIN_SPACING, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("default"), n) == 0)
			{
				sc_makerverbose = DEFAULT_VERBOSE;
				ScSetParameter(SC_PARAM_MAKE_MIN_SPACING, DEFAULT_MIN_SPACING);
				ScSetParameter(SC_PARAM_MAKE_VIA_SIZE, DEFAULT_VIA_SIZE);
				ScSetParameter(SC_PARAM_MAKE_HORIZ_ARC, (INTBIG)DEFAULT_ARC_HORIZONTAL);
				ScSetParameter(SC_PARAM_MAKE_VERT_ARC, (INTBIG)DEFAULT_ARC_VERTICAL);
				ScSetParameter(SC_PARAM_MAKE_L2_WIDTH, DEFAULT_L2_TRACK_WIDTH);
				ScSetParameter(SC_PARAM_MAKE_L1_WIDTH, DEFAULT_L1_TRACK_WIDTH);
				ScSetParameter(SC_PARAM_MAKE_PWR_WIDTH, DEFAULT_POWER_TRACK_WIDTH);
				ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH, DEFAULT_MAIN_POWER_WIDTH);
				ScSetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL, DEFAULT_MAIN_POWER_RAIL);
				ScSetParameter(SC_PARAM_MAKE_PWELL_SIZE, DEFAULT_PWELL_SIZE);
				ScSetParameter(SC_PARAM_MAKE_PWELL_OFFSET, DEFAULT_PWELL_OFFSET);
				ScSetParameter(SC_PARAM_MAKE_NWELL_SIZE, DEFAULT_NWELL_SIZE);
				ScSetParameter(SC_PARAM_MAKE_NWELL_OFFSET, DEFAULT_NWELL_OFFSET);
				continue;
			}
			n = maxi(l, 5);
			if (namesamen(pars[numcmd], x_("no-verbose"), n) == 0)
			{
				sc_makerverbose = FALSE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("arc-vertical"), n) == 0)
			{
				if (++numcmd < count)
				{
					ap = getarcproto(pars[numcmd]);
					ScSetParameter(SC_PARAM_MAKE_VERT_ARC, (INTBIG)ap);
				}
				continue;
			}
			if (namesamen(pars[numcmd], x_("arc-horizontal"), n) == 0)
			{
				if (++numcmd < count)
				{
					ap = getarcproto(pars[numcmd]);
					ScSetParameter(SC_PARAM_MAKE_HORIZ_ARC, (INTBIG)ap);
				}
				continue;
			}
			n = maxi(l, 8);
			if (namesamen(pars[numcmd], x_("p-well-size"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_PWELL_SIZE, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("p-well-offset"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_PWELL_OFFSET, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("n-well-size"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_NWELL_SIZE, eatoi(pars[numcmd]));
				continue;
			}
			if (namesamen(pars[numcmd], x_("n-well-offset"), n) == 0)
			{
				if (++numcmd < count)
					ScSetParameter(SC_PARAM_MAKE_NWELL_OFFSET, eatoi(pars[numcmd]));
				continue;
			}
			return(Sc_seterrmsg(SC_MAKE_SET_XCMD, pars[numcmd]));
		}
	} else
		return(Sc_seterrmsg(SC_MAKE_SET_NOCMD));

	Sc_maker_show_control();
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_maker_show_control
------------------------------------------------------------------------
Description:
	Print the maker control structure values.
------------------------------------------------------------------------
Calling Sequence:  Sc_maker_show_control();
------------------------------------------------------------------------
*/

void Sc_maker_show_control(void)
{
	CHAR *str1;
	static CHAR *hv[3] = {N_("Horizontal"), N_("Vertical")};

	ttyputmsg(M_("************ MAKER CONTROL STRUCTURE"));
	if (sc_makerverbose) str1 = M_("TRUE"); else
		str1 = M_("FALSE");
	ttyputmsg(M_("Verbose output           =  %s"), str1);
	ttyputmsg(M_("Minimum metal spacing    =  %d"),
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING));
	ttyputmsg(M_("Via Size                 =  %d"),
		ScGetParameter(SC_PARAM_MAKE_VIA_SIZE));
	ttyputmsg(M_("Horizontal (layer 1) arc =  '%s'"),
		describearcproto((ARCPROTO *)ScGetParameter(SC_PARAM_MAKE_HORIZ_ARC)));
	ttyputmsg(M_("Vertical (layer 2) arc   =  '%s'"),
		describearcproto((ARCPROTO *)ScGetParameter(SC_PARAM_MAKE_VERT_ARC)));
	ttyputmsg(M_("Layer 1 Track width      =  %d"),
		ScGetParameter(SC_PARAM_MAKE_L1_WIDTH));
	ttyputmsg(M_("Layer 2 Track width      =  %d"),
		ScGetParameter(SC_PARAM_MAKE_L2_WIDTH));
	ttyputmsg(M_("Power Track width        =  %d"),
		ScGetParameter(SC_PARAM_MAKE_PWR_WIDTH));
	ttyputmsg(M_("Main Power Bus width     =  %d"),
		ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH));
	ttyputmsg(M_("Main Power Bus rail      =  %s"),
		hv[ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL)]);
	ttyputmsg(M_("P-well size (0 for none) =  %d"),
		ScGetParameter(SC_PARAM_MAKE_PWELL_SIZE));
	ttyputmsg(M_("P-well offset (bottom)   =  %d"),
		ScGetParameter(SC_PARAM_MAKE_PWELL_OFFSET));
	ttyputmsg(M_("N-well size (0 for none) =  %d"),
		ScGetParameter(SC_PARAM_MAKE_NWELL_SIZE));
	ttyputmsg(M_("N-well offset (bottom)   =  %d"),
		ScGetParameter(SC_PARAM_MAKE_NWELL_OFFSET));
}

/***********************************************************************
Module:  Sc_maker_create_info
------------------------------------------------------------------------
Description:
	Synthesize the total information for the indicated cell.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_maker_create_info(cell);

Name		Type		Description
----		----		-----------
cell		*SCCELL		Pointer to the cell.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_maker_create_info(SCCELL *cell)
{
	SCMAKERINFO *info;
	SCROWLIST *row;
	int max_height;
	int num_leaf_cells, num_feeds, num_rows;
	int min_xpos, max_xpos, ysize;
	int track_to_track, row_to_track;
	SCNBPLACE *place;
	SCROUTECHANNEL *chan;
	SCROUTETRACK *track;
	SCROUTETRACKMEM *mem;
	int num_channels, num_tracks, track_length;

	info = NULL;
	info = (SCMAKERINFO *)emalloc(sizeof(SCMAKERINFO), sc_tool->cluster);
	if (info == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));

	/* go through rows of placed cells and create maker information */
	/* count number of leaf cells, feed throughs, etc. */
	track_to_track = ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);
	row_to_track = (ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) >> 1) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);
	min_xpos = MAXINTBIG;
	max_xpos = -MAXINTBIG;
	ysize = 0;
	num_leaf_cells = 0;
	num_feeds = 0;
	num_rows = 0;
	for (row = cell->placement->rows; row; row = row->next)
	{
		num_rows++;
		max_height = 0;
		for (place = row->start; place; place = place->next)
		{
			if (place->cell->type == SCLEAFCELL)
			{
				num_leaf_cells++;
				max_height = maxi(max_height,
					Sc_leaf_cell_ysize(place->cell->np));
			} else if (place->cell->type == SCFEEDCELL)
			{
				num_feeds++;
			}
			min_xpos = mini(min_xpos, place->xpos);
			max_xpos = maxi(max_xpos, place->xpos + place->cell->size);
		}
		ysize += max_height;
	}
	info->min_x = min_xpos;
	info->max_x = max_xpos;
	info->x_size = max_xpos - min_xpos;
	info->num_leaf_cells = num_leaf_cells;
	info->num_feeds = num_feeds;
	info->num_rows = num_rows;

	/* go through channels */
	num_channels = 0;
	num_tracks = 0;
	track_length = 0;
	for (chan = cell->route->channels; chan; chan = chan->next)
	{
		num_channels++;
		for (track = chan->tracks; track; track = track->next)
		{
			num_tracks++;
			if (track == chan->tracks)
			{
				ysize += row_to_track << 1;
			} else
			{
				ysize += track_to_track;
			}
			for (mem = track->nodes; mem; mem = mem->next)
			{
				track_length += mem->node->lastport->xpos -
								mem->node->firstport->xpos;
			}
		}
	}
	info->min_y = 0;
	info->max_y = ysize;
	info->y_size = ysize;
	info->area = (info->x_size / 2000) * (info->y_size / 2000);
	info->num_channels = num_channels;
	info->num_tracks = num_tracks;
	info->track_length = track_length;

	ttyputmsg(M_("************ MAKER INFORMATION"));
	ttyputmsg(M_("Size:"));
	ttyputmsg(M_("    X size             = %10d  (microns)"), info->x_size / 2000);
	ttyputmsg(M_("    Y size             = %10d  (microns)"), info->y_size / 2000);
	ttyputmsg(M_("    Area               = %10d  (square microns)"), info->area);
	ttyputmsg(M_("Limits (in microns):"));
	ttyputmsg(M_("    Minimum X position =  %d"), info->min_x / 2000);
	ttyputmsg(M_("    Maximim X position =  %d"), info->max_x / 2000);
	ttyputmsg(M_("    Minimum Y position =  %d"), info->min_y / 2000);
	ttyputmsg(M_("    Maximim Y position =  %d"), info->max_y / 2000);
	ttyputmsg(M_("Numbers of Elements:"));
	ttyputmsg(M_("    Base Cells         =  %d"), info->num_leaf_cells);
	ttyputmsg(M_("    Feed Throughs      =  %d"), info->num_feeds);
	ttyputmsg(M_("    Rows of cells      =  %d"), info->num_rows);
	ttyputmsg(M_("    Routing Channels   =  %d"), info->num_channels);
	ttyputmsg(M_("    Routing Tracks     =  %d"), info->num_tracks);
	ttyputmsg(M_("    Total Track Length =  %d  (microns)"),
		info->track_length / 2000);

	efree((CHAR *)info);
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_maker_set_up
------------------------------------------------------------------------
Description:
	Create the data structures to define the precise layout of the cell.
	Decide exactly where cells are placed, tracks are laid, via are
	positioned, etc.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_maker_set_up(cell, make_data);

Name		Type			Description
----		----			-----------
cell		*SCCELL			Pointer to cell to layout.
make_data	**SCMAKERDATA	Address to write pointer to created data.
------------------------------------------------------------------------
*/

int Sc_maker_set_up(SCCELL *cell, SCMAKERDATA **make_data)
{
	SCMAKERDATA		*data;
	SCROWLIST		*row;
	SCROUTECHANNEL	*chan;
	SCROUTETRACK	*track;
	SCROUTEPORT		*rport;
	SCNBPLACE		*place;
	SCMAKERROW		*mrow, *last_mrow;
	SCMAKERINST		*minst, *last_minst;
	SCMAKERCHANNEL	*mchan, *last_mchan;
	SCMAKERTRACK	*mtrack, *last_mtrack;
	SCMAKERNODE		*mnode;
	SCMAKERVIA		*mvia, *lastvia;
	SCMAKERPOWER	*plist, *last_plist, *next_plist;
	SCMAKERPOWERPORT	*power_port, *last_port, *next_port;
	SCNIPORT		*iport;
	SCROUTETRACKMEM	*mem, *tr1_mem, *tr2_mem;
	SCROUTECHPORT	*chport, *tr1_port, *tr2_port;
	SCCELLNUMS		cnums;
	SCROUTEEXPORT	*xport;
	int			ypos, toffset, boffset, type, port_ypos;
	int			row_to_track, min_track_to_track, max_track_to_track;
	int			deltay;

	/* create top level data structure */
	data = (SCMAKERDATA *)emalloc(sizeof(SCMAKERDATA), sc_tool->cluster);
	if (data == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	data->cell = cell;
	data->rows = NULL;
	data->channels = NULL;
	data->power = NULL;
	data->ground = NULL;
	data->minx = MAXINTBIG;
	data->maxx = -MAXINTBIG;
	data->miny = MAXINTBIG;
	data->maxy = -MAXINTBIG;

	/* create Maker Channel and Track data structures */
	row_to_track = (ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) >> 1) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);
	min_track_to_track = (ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) >> 1) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING) + (ScGetParameter(SC_PARAM_MAKE_L1_WIDTH) >> 1);
	max_track_to_track = ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);
	last_mchan = NULL;
	for (chan = cell->route->channels; chan; chan = chan->next)
	{

		/* create Maker Channel structute */
		mchan = (SCMAKERCHANNEL *)emalloc(sizeof(SCMAKERCHANNEL), sc_tool->cluster);
		if (mchan == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		mchan->number = chan->number;
		mchan->tracks = NULL;
		mchan->num_tracks = 0;
		mchan->ysize = 0;
		mchan->flags = 0;
		mchan->next = NULL;
		mchan->last = last_mchan;
		if (last_mchan)
		{
			last_mchan->next = mchan;
		} else
		{
			data->channels = mchan;
		}
		last_mchan = mchan;

		/* create Make Track structures */
		last_mtrack = NULL;
		ypos = 0;
		for (track = chan->tracks; track; track = track->next)
		{
			mtrack = (SCMAKERTRACK *)emalloc(sizeof(SCMAKERTRACK), sc_tool->cluster);
			if (mtrack == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			mtrack->number = track->number;
			mtrack->nodes = NULL;
			mtrack->track = track;
			mtrack->flags = 0;
			mtrack->next = NULL;
			mtrack->last = last_mtrack;
			if (last_mtrack)
			{
				last_mtrack->next = mtrack;
			} else
			{
				mchan->tracks = mtrack;
			}
			last_mtrack = mtrack;
			mchan->num_tracks++;
			if (mtrack->number == 0)
			{
				ypos += row_to_track;
				mtrack->ypos = ypos;
			} else
			{
				/* determine if min or max track to track spacing is used */
				deltay = min_track_to_track;
				tr1_mem = track->nodes;
				tr2_mem = track->last->nodes;
				tr1_port = tr1_mem->node->firstport;
				tr2_port = tr2_mem->node->firstport;
				while (tr1_port && tr2_port)
				{
					if (abs(tr1_port->xpos - tr2_port->xpos) <
						max_track_to_track)
					{
						deltay = max_track_to_track;
						break;
					}
					if (tr1_port->xpos < tr2_port->xpos)
					{
						tr1_port = tr1_port->next;
						if (tr1_port == NULL)
						{
							tr1_mem = tr1_mem->next;
							if (tr1_mem)
								tr1_port = tr1_mem->node->firstport;
						}
					} else
					{
						tr2_port = tr2_port->next;
						if (tr2_port == NULL)
						{
							tr2_mem = tr2_mem->next;
							if (tr2_mem)
								tr2_port = tr2_mem->node->firstport;
						}
					}
				}
				ypos += deltay;
				mtrack->ypos = ypos;
			}
			if (track->next == NULL)
				ypos += row_to_track;
		}
		mchan->ysize = ypos;
	}

	/* create Maker Rows and Instances data structures */
	mchan = data->channels;
	mchan->miny = 0;
	ypos = mchan->ysize;
	last_mrow = NULL;
	for (row = cell->placement->rows; row; row = row->next)
	{
		/* create maker row data structure */
		mrow = (SCMAKERROW *)emalloc(sizeof(SCMAKERROW), sc_tool->cluster);
		if (mrow == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		mrow->number = row->row_num;
		mrow->members = NULL;
		mrow->minx = MAXINTBIG;
		mrow->maxx = -MAXINTBIG;
		mrow->miny = MAXINTBIG;
		mrow->maxy = -MAXINTBIG;
		mrow->flags = 0;
		mrow->next = NULL;
		mrow->last = last_mrow;
		if (last_mrow)
		{
			last_mrow->next = mrow;
		} else
		{
			data->rows = mrow;
		}
		last_mrow = mrow;

		/* determine permissible top and bottom overlap */
		toffset = -MAXINTBIG;
		boffset = MAXINTBIG;
		for (place = row->start; place; place = place->next)
		{
			if (place->cell->type != SCLEAFCELL) continue;
			Sc_leaf_cell_get_nums(place->cell->np, &cnums);
			toffset = maxi(toffset, Sc_leaf_cell_ysize(place->cell->np)
				- cnums.top_active);
			boffset = mini(boffset, cnums.bottom_active);
		}
		ypos -= boffset;

		/* create maker instance structure for each member in the row */
		last_minst = NULL;
		for (place = row->start; place; place = place->next)
		{
			if (place->cell->type != SCLEAFCELL &&
				place->cell->type != SCFEEDCELL &&
				place->cell->type != SCLATERALFEED) continue;
			minst = (SCMAKERINST *)emalloc(sizeof(SCMAKERINST), sc_tool->cluster);
			if (minst == 0)
				return(Sc_seterrmsg(SC_NOMEMORY));
			minst->place = place;
			minst->row = mrow;
			minst->xpos = place->xpos;
			minst->ypos = ypos;
			minst->xsize = place->cell->size;
			if (place->cell->type == SCLEAFCELL)
			{
				minst->ysize = Sc_leaf_cell_ysize(place->cell->np);

				/* add power ports */
				for (iport = place->cell->power; iport; iport = iport->next)
				{
					power_port = (SCMAKERPOWERPORT *)emalloc(sizeof(SCMAKERPOWERPORT), sc_tool->cluster);
					if (power_port == 0)
						return(Sc_seterrmsg(SC_NOMEMORY));
					power_port->inst = minst;
					power_port->port = iport;
					if (mrow->number % 2)
					{
						power_port->xpos = minst->xpos + minst->xsize -
							iport->xpos;
					} else
					{
						power_port->xpos = minst->xpos + iport->xpos;
					}
					power_port->next = NULL;
					power_port->last = NULL;
					port_ypos = minst->ypos + Sc_leaf_port_ypos(iport->port);
					for (plist = data->power; plist; plist = plist->next)
					{
						if (plist->ypos == port_ypos) break;
					}
					if (!plist) {
						plist = (SCMAKERPOWER *)emalloc(sizeof(SCMAKERPOWER), sc_tool->cluster);
						if (plist == 0)
						{
							return(Sc_seterrmsg(SC_NOMEMORY));
						}
						plist->ports = NULL;
						plist->ypos = port_ypos;
						last_plist = NULL;
						for (next_plist = data->power; next_plist;
							next_plist = next_plist->next)
						{
							if (port_ypos < next_plist->ypos) break;
							last_plist = next_plist;
						}
						plist->next = next_plist;
						plist->last = last_plist;
						if (last_plist)
						{
							last_plist->next = plist;
						} else
						{
							data->power = plist;
						}
						if (next_plist)
						{
							next_plist->last = plist;
						}
					}
					last_port = NULL;
					for (next_port = plist->ports; next_port;
						next_port = next_port->next)
					{
						if (power_port->xpos < next_port->xpos) break;
						last_port = next_port;
					}
					power_port->next = next_port;
					power_port->last = last_port;
					if (last_port)
					{
						last_port->next = power_port;
					} else
					{
						plist->ports = power_port;
					}
					if (next_port)
					{
						next_port->last = power_port;
					}
				}

				/* add ground ports */
				for (iport = place->cell->ground; iport; iport = iport->next)
				{
					power_port = (SCMAKERPOWERPORT *)emalloc(sizeof(SCMAKERPOWERPORT), sc_tool->cluster);
					if (power_port == 0)
						return(Sc_seterrmsg(SC_NOMEMORY));
					power_port->inst = minst;
					power_port->port = iport;
					if (mrow->number % 2)
					{
						power_port->xpos = minst->xpos + minst->xsize -
							iport->xpos;
					} else
					{
						power_port->xpos = minst->xpos + iport->xpos;
					}
					power_port->next = NULL;
					power_port->last = NULL;
					port_ypos = minst->ypos + Sc_leaf_port_ypos(iport->port);
					for (plist = data->ground; plist; plist = plist->next)
					{
						if (plist->ypos == port_ypos) break;
					}
					if (!plist)
					{
						plist = (SCMAKERPOWER *)emalloc(sizeof(SCMAKERPOWER), sc_tool->cluster);
						if (plist == 0)
							return(Sc_seterrmsg(SC_NOMEMORY));
						plist->ports = NULL;
						plist->ypos = port_ypos;
						last_plist = NULL;
						for (next_plist = data->ground; next_plist;
							next_plist = next_plist->next)
						{
							if (port_ypos < next_plist->ypos) break;
							last_plist = next_plist;
						}
						plist->next = next_plist;
						plist->last = last_plist;
						if (last_plist)
						{
							last_plist->next = plist;
						} else
						{
							data->ground = plist;
						}
						if (next_plist)
						{
							next_plist->last = plist;
						}
					}
					last_port = NULL;
					for (next_port = plist->ports; next_port;
						next_port = next_port->next)
					{
						if (power_port->xpos < next_port->xpos) break;
						last_port = next_port;
					}
					power_port->next = next_port;
					power_port->last = last_port;
					if (last_port)
					{
						last_port->next = power_port;
					} else
					{
						plist->ports = power_port;
					}
					if (next_port)
					{
						next_port->last = power_port;
					}
				}
			} else if (place->cell->type == SCFEEDCELL)
			{
				minst->ysize = boffset;
			} else if (place->cell->type == SCLATERALFEED)
			{
				rport = (SCROUTEPORT *)place->cell->ports->port;
				minst->ysize = Sc_leaf_port_ypos(rport->port->port);
			} else
			{
				ttyputmsg(_("ERROR - unknown cell type in maker set up"));
				minst->ysize = 0;
			}
			minst->instance = NULL;
			minst->flags = 0;
			place->cell->tp = (CHAR *)minst;
			minst->next = NULL;
			if (last_minst)
			{
				last_minst->next = minst;
			} else
			{
				mrow->members = minst;
			}
			last_minst = minst;

			/* set limits of row */
			mrow->minx = mini(mrow->minx, minst->xpos);
			mrow->maxx = maxi(mrow->maxx, minst->xpos + minst->xsize);
			mrow->miny = mini(mrow->miny, minst->ypos);
			mrow->maxy = maxi(mrow->maxy, minst->ypos + minst->ysize);
		}
		data->minx = mini(data->minx, mrow->minx);
		data->maxx = maxi(data->maxx, mrow->maxx);
		data->miny = mini(data->miny, mrow->miny);
		data->maxy = maxi(data->maxy, mrow->maxy);
		ypos += toffset;
		mchan = mchan->next;
		mchan->miny = ypos;
		ypos += mchan->ysize;
	}

	/* create via list for all tracks */
	for (mchan = data->channels; mchan; mchan = mchan->next)
	{
		/* get bottom track and work up */
		for (mtrack = mchan->tracks; mtrack; mtrack = mtrack->next)
		{
			if (mtrack->next == NULL) break;
		}
		for ( ; mtrack; mtrack = mtrack->last)
		{
			ypos = mchan->miny + (mchan->ysize - mtrack->ypos);
			mtrack->ypos = ypos;
			for (mem = mtrack->track->nodes; mem; mem = mem->next)
			{
				mnode = (SCMAKERNODE *)emalloc(sizeof(SCMAKERNODE), sc_tool->cluster);
				if (mnode == 0)
					return(Sc_seterrmsg(SC_NOMEMORY));
				mnode->vias = NULL;
				mnode->next = mtrack->nodes;
				mtrack->nodes = mnode;
				lastvia = NULL;
				for (chport = mem->node->firstport; chport;
					chport = chport->next)
				{
					mvia = (SCMAKERVIA *)emalloc(sizeof(SCMAKERVIA), sc_tool->cluster);
					if (mvia == 0)
						return(Sc_seterrmsg(SC_NOMEMORY));
					mvia->xpos = chport->xpos;
					mvia->chport = chport;
					mvia->instance = NULL;
					mvia->flags = 0;
					mvia->xport = NULL;
					mvia->next = NULL;
					if (lastvia)
					{
						lastvia->next = mvia;
					} else
					{
						mnode->vias = mvia;
					}
					lastvia = mvia;

					/* check for power port */
					if (mvia->chport->port->place->cell->type == SCLEAFCELL)
					{
						type = Sc_leaf_port_type(mvia->chport->port->port->port);
						if (type == SCPWRPORT || type == SCGNDPORT)
							mvia->flags |= SCVIAPOWER;
					}

					/* check for export */
					for (xport = data->cell->route->exports; xport;
						xport = xport->next)
					{
						if (xport->chport == mvia->chport)
						{
							mvia->flags |= SCVIAEXPORT;
							mvia->xport = xport;
							break;
						}
					}

					data->minx = mini(data->minx, chport->xpos);
					data->maxx = maxi(data->maxx, chport->xpos);
					data->miny = mini(data->miny, chport->xpos);
					data->maxy = maxi(data->maxy, chport->xpos);
				}
			}
		}
	}

	*make_data = data;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_maker_create_layout
------------------------------------------------------------------------
Description:
	Create the actual layout in the associated VLSI layout tool using
	the passed layout data.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_maker_create_layout(data);

Name		Type			Description
----		----			-----------
data		*SCMAKERDATA	Pointer to layout data.
------------------------------------------------------------------------
*/

int Sc_maker_create_layout(SCMAKERDATA *data)
{
	CHAR			*bcell, *binst, *inst1, *inst2, *port1, *port2;
	SCMAKERROW		*row;
	SCMAKERINST		*inst, *lastpower, *lastground;
	SCMAKERINST		*firstinst, *previnst;
	SCMAKERCHANNEL	*chan;
	SCMAKERTRACK	*track;
	SCMAKERNODE		*mnode;
	SCMAKERVIA		*via, *via2;
	SCMAKERPOWER	*plist;
	SCMAKERPOWERPORT	*pport;
	SCNITREE		*node;
	SCROWLIST		*rlist;
	SCNBPLACE		*place;
	SCROUTEPORT		*rport;
	int				transpose, rotation, err, xpos, ypos;
	int				xsize, ysize, mainpwrrail;
	int				row_to_track, track_to_track;

	row_to_track = (ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) >> 1) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);
	track_to_track = ScGetParameter(SC_PARAM_MAKE_VIA_SIZE) +
		ScGetParameter(SC_PARAM_MAKE_MIN_SPACING);

	/* create new cell */
	if ((bcell = Sc_create_leaf_cell(data->cell->name)) == NULL)
		return(Sc_seterrmsg(SC_MAKER_NOCREATE_LEAF_CELL, data->cell->name));

	if ((err = Sc_setup_for_maker((ARCPROTO *)ScGetParameter(SC_PARAM_MAKE_HORIZ_ARC),
		(ARCPROTO *)ScGetParameter(SC_PARAM_MAKE_VERT_ARC)))) return(err);

	/* create instances for cell */
	for (row = data->rows; row; row = row->next)
	{
		if (row->number % 2)
		{
			/* odd row, transpose instances */
			transpose = TRUE;
			rotation = 900;
		} else
		{
			/* even row, do not transpose instances */
			transpose = FALSE;
			rotation = 0;
		}
		for (inst = row->members; inst; inst = inst->next)
		{
			node = inst->place->cell;
			if (node->type == SCLEAFCELL)
			{
				if ((binst = Sc_create_leaf_instance(node->name, node->np,
					inst->xpos, inst->xpos + inst->xsize,
					inst->ypos, inst->ypos + inst->ysize,
					transpose, rotation, bcell)) == NULL)
						return(Sc_seterrmsg(SC_MAKER_NOCREATE_LEAF_INST,
							node->name));
				inst->instance = binst;
			} else if (node->type == SCFEEDCELL)
			{
				/* feed through node */
				if ((binst = Sc_create_layer2_node(inst->xpos +
					(inst->xsize >> 1), inst->ypos + inst->ysize +
					(ScGetParameter(SC_PARAM_MAKE_L2_WIDTH) >> 1),
					ScGetParameter(SC_PARAM_MAKE_L2_WIDTH),
					ScGetParameter(SC_PARAM_MAKE_L2_WIDTH), bcell))
					== NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LEAF_FEED));
				}
				inst->instance = binst;
			} else if (node->type == SCLATERALFEED)
			{
				/* lateral feed node */
				if ((binst = Sc_create_via(inst->xpos + (inst->xsize >> 1),
					inst->ypos + inst->ysize, bcell)) == NULL)
						return(Sc_seterrmsg(SC_MAKER_NOCREATE_VIA));
				inst->instance = binst;
			}
		}
	}

	/* create vias and vertical tracks */
	for (chan = data->channels; chan; chan = chan->next)
	{
		for (track = chan->tracks; track; track = track->next)
		{
			for (mnode = track->nodes; mnode; mnode = mnode->next)
			{
				for (via = mnode->vias; via; via = via->next)
				{
					if (via->flags & SCVIAPOWER)
					{
						if ((binst = Sc_create_layer1_node(via->xpos,
							track->ypos, ScGetParameter(SC_PARAM_MAKE_L1_WIDTH),
							ScGetParameter(SC_PARAM_MAKE_L1_WIDTH), bcell))
							== NULL)
						{
							return(Sc_seterrmsg(SC_MAKER_NOCREATE_VIA));
						}
						via->instance = binst;

						/* create vertical power track */
						inst = (SCMAKERINST *)via->chport->port->place->cell->tp;
						if (Sc_create_track_layer1(inst->instance,
							via->chport->port->port->port,
							via->instance, (CHAR *)NULL,
							ScGetParameter(SC_PARAM_MAKE_L1_WIDTH),
							bcell) == NULL)
						{
							return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER2));
						}
						continue;
					}

					/* create a via if next via (if it exists) is farther */
					/* than the track to track spacing, else create a */
					/* layer2 node */
					if (via->next && !(via->next->flags & SCVIAPOWER))
					{
						if (abs(via->next->xpos - via->xpos) <
							track_to_track)
						{
							if (via->flags & SCVIAEXPORT)
							{
								via->next->flags |= SCVIASPECIAL;
							} else
							{
								via->flags |= SCVIASPECIAL;
							}
						}
					}
					if (via->flags & SCVIASPECIAL)
					{
						if ((binst = Sc_create_layer2_node(via->xpos,
							track->ypos, ScGetParameter(SC_PARAM_MAKE_L2_WIDTH),
							ScGetParameter(SC_PARAM_MAKE_L2_WIDTH), bcell))
							== NULL)
						{
							return(Sc_seterrmsg(
								SC_MAKER_NOCREATE_LEAF_FEED));
						}
					} else
					{
						if ((binst = Sc_create_via(via->xpos, track->ypos,
							bcell)) == NULL)
						{
							return(Sc_seterrmsg(SC_MAKER_NOCREATE_VIA));
						}
					}
					via->instance = binst;

					/* create vertical track */
					node = via->chport->port->place->cell;
					if (node->type == SCLEAFCELL)
					{
						inst = (SCMAKERINST *)node->tp;
						if (Sc_create_track_layer2(inst->instance,
							via->chport->port->port->port,
							via->instance, (CHAR *)NULL,
							ScGetParameter(SC_PARAM_MAKE_L2_WIDTH), bcell) == NULL)
						{
							return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER2));
						}
					} else if (node->type == SCFEEDCELL ||
						node->type == SCLATERALFEED)
					{
						inst = (SCMAKERINST *)node->tp;
						if (Sc_create_track_layer2(inst->instance,
							(CHAR *)NULL, via->instance, (CHAR *)NULL,
							ScGetParameter(SC_PARAM_MAKE_L2_WIDTH), bcell) == NULL)
						{
							return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER2));
						}
					}
				}
			}
		}
	}

	/* create horizontal tracks */
	for (chan = data->channels; chan; chan = chan->next)
	{
		for (track = chan->tracks; track; track = track->next)
		{
			for (mnode = track->nodes; mnode; mnode = mnode->next)
			{
				for (via = mnode->vias; via; via = via->next)
				{
					if (via->next)
					{
						if (via->flags & SCVIASPECIAL)
						{
							if (abs(via->next->xpos - via->xpos) < track_to_track)
							{
								if (Sc_create_track_layer2(via->instance,
									(CHAR *)NULL, via->next->instance,
									(CHAR *)NULL, ScGetParameter(SC_PARAM_MAKE_L2_WIDTH),
									bcell) == NULL)
								{
									return(Sc_seterrmsg
										(SC_MAKER_NOCREATE_LAYER1));
								}
							}
						} else
						{
							if (!(via->flags & SCVIAPOWER) &&
								!(via->next->flags & SCVIAPOWER) &&
								(via->next->xpos - via->xpos) <
								track_to_track)
							{
								if (Sc_create_track_layer2(via->instance,
									(CHAR *)NULL, via->next->instance,
									(CHAR *)NULL, ScGetParameter(SC_PARAM_MAKE_L2_WIDTH),
									bcell) == NULL)
								{
									return(Sc_seterrmsg
										(SC_MAKER_NOCREATE_LAYER1));
								}
							}
							for (via2 = via->next; via2; via2 = via2->next)
							{
								if (via2->flags & SCVIASPECIAL) continue;
								if (Sc_create_track_layer1(via->instance,
									(CHAR *)NULL, via2->instance, (CHAR *)NULL,
									ScGetParameter(SC_PARAM_MAKE_L1_WIDTH), bcell)
									== NULL)
								{
									return(Sc_seterrmsg
										(SC_MAKER_NOCREATE_LAYER1));
								}
								break;
							}
						}
					}
				}
			}
		}
	}

	/* create stitches and lateral feeds */
	for (rlist = data->cell->placement->rows; rlist; rlist = rlist->next)
	{
		for (place = rlist->start; place; place = place->next)
		{
			if (place->cell->type == SCSTITCH)
			{
				rport = (SCROUTEPORT *)place->cell->ports->port;
				inst = (SCMAKERINST *)rport->place->cell->tp;
				inst1 = inst->instance;
				port1 = rport->port->port;
				rport = (SCROUTEPORT *)place->cell->ports->next->port;
				inst = (SCMAKERINST *)rport->place->cell->tp;
				inst2 = inst->instance;
				port2 = rport->port->port;
				if (Sc_create_track_layer1(inst1, port1, inst2, port2,
					ScGetParameter(SC_PARAM_MAKE_L1_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			} else if (place->cell->type == SCLATERALFEED)
			{
				rport = (SCROUTEPORT *)place->cell->ports->port;
				inst = (SCMAKERINST *)rport->place->cell->tp;
				inst1 = inst->instance;
				port1 = rport->port->port;
				inst = (SCMAKERINST *)place->cell->tp;
				inst2 = inst->instance;
				if (Sc_create_track_layer1(inst1, port1, inst2, (CHAR *)NULL,
					ScGetParameter(SC_PARAM_MAKE_L1_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER2));
				}
			}
		}
	}

	/* export ports */
	for (chan = data->channels; chan; chan = chan->next)
	{
		for (track = chan->tracks; track; track = track->next)
		{
			for (mnode = track->nodes; mnode; mnode = mnode->next)
			{
				for (via = mnode->vias; via; via = via->next)
				{
					if (via->flags & SCVIAEXPORT)
					{
						if (Sc_create_export_port(via->instance, (CHAR *)NULL,
							via->xport->xport->name, via->xport->xport->bits
							& SCPORTTYPE, bcell) == NULL)
						{
							return(Sc_seterrmsg(SC_MAKER_NOCREATE_XPORT,
								via->xport->xport->name));
						}
					}
				}
			}
		}
	}

	/* create power buses */
	lastpower = NULL;
	xpos = data->minx - row_to_track -
		(ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH) >> 1);

	mainpwrrail = ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_RAIL);

	for (plist = data->power; plist; plist = plist->next)
	{
		ypos = plist->ypos;

		/* create main power bus node */
		if (mainpwrrail == 0)
		{
			binst = Sc_create_layer1_node(xpos, ypos,
				ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH),
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell);
		} else
		{
			binst = Sc_create_layer2_node(xpos, ypos,
				ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH),
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell);
		}
		if (binst == NULL)
			return(Sc_seterrmsg(SC_MAKER_NOCREATE_VIA));
		if (lastpower)
		{
			/* join to previous */
			if (mainpwrrail == 0)
			{
				if (Sc_create_track_layer1(binst, (CHAR *)NULL,
					(CHAR *)lastpower, (CHAR *)NULL,
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			} else
			{
				if (Sc_create_track_layer2(binst, (CHAR *)NULL,
					(CHAR *)lastpower, (CHAR *)NULL,
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			}
		}
		lastpower = (SCMAKERINST *)binst;

		for (pport = plist->ports; pport; pport = pport->next)
		{
			if (pport->last == NULL)
			{
				/* connect to main power node */
				if (Sc_create_track_layer1((CHAR *)lastpower, (CHAR *)NULL,
					pport->inst->instance, pport->port->port,
					ScGetParameter(SC_PARAM_MAKE_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			}

			/* connect to next if it exists */
			if (pport->next)
			{
				if (Sc_create_track_layer1(pport->inst->instance,
					pport->port->port, pport->next->inst->instance,
					pport->next->port->port,
					ScGetParameter(SC_PARAM_MAKE_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			}
		}
	}

	/* create ground buses */
	lastground = NULL;
	xpos = data->maxx + row_to_track +
		(ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH) >> 1);

	for (plist = data->ground; plist; plist = plist->next)
	{
		ypos = plist->ypos;

		/* create main ground bus node */
		if (mainpwrrail == 0)
		{
			binst = Sc_create_layer1_node(xpos, ypos,
				ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH),
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell);
		} else
		{
			binst = Sc_create_layer2_node(xpos, ypos,
				ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH),
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell);
		}
		if (binst == NULL)
			return(Sc_seterrmsg(SC_MAKER_NOCREATE_VIA));
		if (lastground)
		{
			/* join to previous */
			if (mainpwrrail == 0)
			{
				if (Sc_create_track_layer1(binst, (CHAR *)NULL,
					(CHAR *)lastground, (CHAR *)NULL,
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			} else
			{
				if (Sc_create_track_layer2(binst, (CHAR *)NULL,
					(CHAR *)lastground, (CHAR *)NULL,
					ScGetParameter(SC_PARAM_MAKE_MAIN_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			}
		} else
		{
			if (Sc_create_export_port(binst, (CHAR *)NULL, x_("gnd"), SCGNDPORT,
				bcell) == NULL)
			{
				return(Sc_seterrmsg(SC_MAKER_NOCREATE_XPORT, x_("gnd")));
			}
		}
		lastground = (SCMAKERINST *)binst;

		for (pport = plist->ports; pport; pport = pport->next)
		{
			if (pport->next == NULL)
			{
				/* connect to main ground node */
				if (Sc_create_track_layer1((CHAR *)lastground, (CHAR *)NULL,
					pport->inst->instance, pport->port->port,
					ScGetParameter(SC_PARAM_MAKE_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			}
			/* connect to next if it exists */
			else
			{
				if (Sc_create_track_layer1(pport->inst->instance,
					pport->port->port, pport->next->inst->instance,
					pport->next->port->port,
					ScGetParameter(SC_PARAM_MAKE_PWR_WIDTH), bcell) == NULL)
				{
					return(Sc_seterrmsg(SC_MAKER_NOCREATE_LAYER1));
				}
			}
		}
	}
	if (lastpower)
	{
		/* export as cell vdd */
		if (Sc_create_export_port((CHAR *)lastpower, (CHAR *)NULL, x_("vdd"),
			SCPWRPORT, bcell) == NULL)
		{
			return(Sc_seterrmsg(SC_MAKER_NOCREATE_XPORT, x_("vdd")));
		}
	}

	/* create overall P-wells if pwell size not zero */
	if (ScGetParameter(SC_PARAM_MAKE_PWELL_SIZE) != 0)
	{
		for (row = data->rows; row; row = row->next)
		{
			firstinst = NULL;
			previnst = NULL;
			for (inst = row->members; inst; inst = inst->next)
			{
				if (inst->place->cell->type != SCLEAFCELL) continue;
				if (!firstinst)
				{
					firstinst = inst;
				} else
				{
					previnst = inst;
				}
			}
			if (previnst)
			{
				xpos = (firstinst->xpos + previnst->xpos + previnst->xsize) >> 1;
				xsize = (previnst->xpos + previnst->xsize) - firstinst->xpos;
				ysize = ScGetParameter(SC_PARAM_MAKE_PWELL_SIZE);
				if (ysize > 0)
				{
					ypos = firstinst->ypos + ScGetParameter(SC_PARAM_MAKE_PWELL_OFFSET) +
						(ScGetParameter(SC_PARAM_MAKE_PWELL_SIZE) >> 1);
					if ((binst = Sc_create_pwell(xpos, ypos, xsize, ysize, bcell))
						== NULL)
					{
						return(Sc_seterrmsg(SC_NOCREATE_PWELL));
					}
				}

				ysize = ScGetParameter(SC_PARAM_MAKE_NWELL_SIZE);
				if (ysize > 0)
				{
					ypos = firstinst->ypos + firstinst->ysize - ScGetParameter(SC_PARAM_MAKE_NWELL_OFFSET) -
						(ScGetParameter(SC_PARAM_MAKE_NWELL_SIZE) >> 1);
					if ((binst = Sc_create_nwell(xpos, ypos, xsize, ysize, bcell))
						== NULL)
					{
						return(Sc_seterrmsg(SC_NOCREATE_PWELL));
					}
				}
			}
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_maker_data
------------------------------------------------------------------------
Description:
	Free the memory structures used by the maker.
------------------------------------------------------------------------
Calling Sequence:  Sc_free_maker_data(data);

Name		Type			Description
----		----			-----------
data		*SCMAKERDATA	Pointer to maker data.
------------------------------------------------------------------------
*/

void Sc_free_maker_data(SCMAKERDATA *data)
{
	SCMAKERROW			*row, *nextrow;
	SCMAKERINST			*inst, *nextinst;
	SCMAKERCHANNEL		*chan, *nextchan;
	SCMAKERTRACK		*track, *nexttrack;
	SCMAKERNODE			*node, *nextnode;
	SCMAKERVIA			*via, *nextvia;
	SCMAKERPOWER		*power, *nextpower;
	SCMAKERPOWERPORT	*pport, *nextpport;

	if (data)
	{
		for (row = data->rows; row; row = nextrow)
		{
			nextrow = row->next;
			for (inst = row->members; inst; inst = nextinst)
			{
				nextinst = inst->next;
				efree((CHAR *)inst);
			}
			efree((CHAR *)row);
		}
		for (chan = data->channels; chan; chan = nextchan)
		{
			nextchan = chan->next;
			for (track = chan->tracks; track; track = nexttrack)
			{
				nexttrack = track->next;
				for (node = track->nodes; node; node = nextnode)
				{
					nextnode = node->next;
					for (via = node->vias; via; via = nextvia)
					{
						nextvia = via->next;
						efree((CHAR *)via);
					}
					efree((CHAR *)node);
				}
				efree((CHAR *)track);
			}
			efree((CHAR *)chan);
		}
		for (power = data->power; power; power = nextpower)
		{
			nextpower = power->next;
			for (pport = power->ports; pport; pport = nextpport)
			{
				nextpport = pport->next;
				efree((CHAR *)pport);
			}
			efree((CHAR *)power);
		}
		for (power = data->ground; power; power = nextpower)
		{
			nextpower = power->next;
			for (pport = power->ports; pport; pport = nextpport)
			{
				nextpport = pport->next;
				efree((CHAR *)pport);
			}
			efree((CHAR *)power);
		}
		efree((CHAR *)data);
	}
}

#endif  /* SCTOOL - at top */
