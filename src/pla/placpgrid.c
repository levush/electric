/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: placpgrid.c
 * PLA generator for CMOS
 * Written by: Wallace Kroeker at the University of Calgary
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
#if PLATOOL

#include "global.h"
#include "pla.h"
#include "placmos.h"

/* prototypes for local routines */
static BOOLEAN plac_pmos_init_columns(INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*);
static BOOLEAN plac_pmos_init_rows(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*);
static BOOLEAN plac_pwr_strap(INTBIG, INTBIG, INTBIG, NODEPROTO*);
static BOOLEAN plac_pmos_make_one(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*, NODEPROTO*);
static BOOLEAN plac_pmos_complete_row(INTBIG, INTBIG, INTBIG, NODEPROTO*, NODEPROTO*);
static BOOLEAN plac_pmos_finish_columns(INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*, NODEPROTO*);

NODEPROTO *plac_pmos_grid(LIBRARY *library, FILE *file, CHAR cell_name[])
{
	INTBIG height, width, height_in, width_in, read_rows;
	NODEPROTO *pla_array_cell, *pmos_one;
	INTBIG X, Y;
	INTBIG row_1[PLAC_MAX_COL_SIZE], row_2[PLAC_MAX_COL_SIZE];
	INTBIG eof, i, X_offset, Y_offset, Y_M_offset, row;

	Y = 0;
	X = 3*pla_lam;
	X_offset = PLAC_X_SEP*pla_lam;
	Y_offset = PLAC_Y_SEP*pla_lam;
	Y_M_offset = PLAC_Y_MIR_SEP * pla_lam;

	pmos_one = getnodeproto(x_("pmos_one"));
	if (pmos_one == NONODEPROTO)
	{
		ttyputerr(_("Unable to find prototype called pmos_one"));
		return(NONODEPROTO);
	}

	pla_array_cell = newnodeproto(cell_name, library);

	eof = plac_read_hw(file, &height, &width, &height_in, &width_in);
	if (eof == EOF)
	{
		ttyputerr(_("Error reading height and width"));
		return(NONODEPROTO);
	}

	(void)setval((INTBIG)pla_array_cell, VNODEPROTO, x_("PLA_data_cols"), width_in, VINTEGER);
	(void)setval((INTBIG)pla_array_cell, VNODEPROTO, x_("PLA_access_rows"), height_in, VINTEGER);
	(void)setval((INTBIG)pla_array_cell, VNODEPROTO, x_("PLA_cols"), width, VINTEGER);
	(void)setval((INTBIG)pla_array_cell, VNODEPROTO, x_("PLA_rows"), height, VINTEGER);

	/* initialize the columns */
	if (plac_pmos_init_columns(width, X, Y, X_offset, pla_array_cell))
		return(NONODEPROTO);

	/* initialize the rows */
	if (plac_pmos_init_rows(height_in, X, Y, Y_offset, Y_M_offset, pla_array_cell))
		return(NONODEPROTO);

	row_1[0] = -1;
	row_2[0] = -1;
	Y = 3*pla_lam - Y_offset;
	X = X_offset + 4*pla_lam;
	row = 0;
	read_rows = 0;

	ttyputmsg(_("Finished setup, about to read and generate"));
	while ((read_rows < height_in) && (eof != EOF))
	{
		/* read in the two rows first */
		if ((read_rows < height_in) && (eof != EOF))
		{
			eof =  plac_read_rows(row_1, width, width_in, file);
			read_rows++;
		}
		if ((read_rows < height_in) && (eof != EOF))
		{
			eof =  plac_read_rows(row_2, width, width_in, file);
			read_rows++;
		}

		for(i = 0; i < width; i++)
		{
			/* place a pmos_one cell and connect to power, and cell up above */
			if (((i % 5) == 0) && (i != 0)) /* put in power strap */
			{
				if (plac_pwr_strap(i, ((X_offset*i)+(3*pla_lam)), Y,
					pla_array_cell)) return(NONODEPROTO);
			} else
			{
				/* put in bits -- make an instance of a pmos_one  */
				if ((row_1[i] == 1) && (row_1[0] != -1))
				{
					/* make an instance of a pmos_one  */
					if (plac_pmos_make_one(i, ((i*X_offset)+(6*pla_lam)), Y, 0, row,
						pmos_one, pla_array_cell)) return(NONODEPROTO);
				}
				if ((row_2[i] == 1) && (row_2[0] != -1))
				{
					/* make an instance of a pmos_one  */
					if (plac_pmos_make_one(i, ((i*X_offset)+(6*pla_lam)),
						Y-Y_M_offset, 2, row, pmos_one, pla_array_cell)) return(NONODEPROTO);
				}
			}
		}

		/* Now put the poly metal contact at on the other side of the row */
		if (plac_pmos_complete_row(row, ((X_offset*width)+(0*pla_lam)),
			Y, pmos_one, pla_array_cell)) return(NONODEPROTO);

		if ((read_rows % 4) == 0) Y = Y - Y_offset;

		row_1[0] = -1;
		row_2[0] = -1;
		row++;
		Y = Y - 2*Y_offset;
	}
	X = 3*pla_lam;
	if ((read_rows % 4) == 0) Y = Y + 2*Y_offset-3*pla_lam; else
		Y = Y + Y_offset-3*pla_lam;
	if (plac_pmos_finish_columns(width, X, Y, X_offset, pmos_one, pla_array_cell))
		return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(pla_array_cell);

	return(pla_array_cell);
}

BOOLEAN plac_pmos_init_columns(INTBIG width, INTBIG X, INTBIG Y, INTBIG X_offset,
	NODEPROTO *pla_array_cell)
{
	CHAR name[40];
	INTBIG i, pwr_cnt;

	pwr_cnt = 0;
	for (i = 0; i < width; i++)
	{
		plac_col_list[i].firstitem = plac_newUCITEM();
		plac_col_list[i].lastitem =  plac_col_list[i].firstitem;

		if ((i % 5) == 0)
		{
			/* put in a Power pin every 5th position */
			(void)esnprintf(name, 40, x_("PWR%ld.m-1.n"), pwr_cnt);
			plac_col_list[i].firstitem->nodeinst = plac_make_Pin(pla_array_cell,
				X_offset * i + X, Y, 14*pla_lam, x_("Metal-1-Substrate-Con"));
			if (plac_col_list[i].firstitem->nodeinst == NONODEINST) return(TRUE);
			pwr_cnt++;
		} else
		{
			/* must be a data pin, don't count power pins */
			(void)esnprintf(name, 40, x_("DATA%ld.m-1.n"), (i - pwr_cnt));
			plac_col_list[i].firstitem->nodeinst = plac_make_Pin(pla_array_cell,
				X_offset * i + X, Y, 4*pla_lam, x_("Metal-1-Pin"));
			if (plac_col_list[i].firstitem->nodeinst == NONODEINST) return(TRUE);
		}
		(void)newportproto(pla_array_cell, plac_col_list[i].firstitem->nodeinst,
			plac_col_list[i].firstitem->nodeinst->proto->firstportproto, name);
	}
	return(FALSE);
}

BOOLEAN plac_pmos_init_rows(INTBIG height_in, INTBIG X, INTBIG Y, INTBIG Y_offset,
	INTBIG Y_M_offset, NODEPROTO *pla_array_cell)
{
	INTBIG limit, i;
	PORTPROTO *ni_col_pp, *last_col_pp;
	UCITEM *new_item;

	/* Y = Y - Y_offset; */
	limit = (height_in/2) + (height_in % 2);

	for (i = 0; i < limit; i++)
	{
		new_item = plac_newUCITEM();

		if (((i % 2) == 0) && (i != 0)) Y = Y - Y_offset;

		/* put in Substrate contact first */
		new_item->nodeinst = plac_make_Pin(pla_array_cell, X,
			Y-Y_M_offset+10*pla_lam, 14*pla_lam, x_("Metal-1-Substrate-Con"));
		if (new_item->nodeinst == NONODEINST) return(TRUE);

		/* wire in the first power strap */
		/* connect to last column object */
		/* only put this in a column list */
		ni_col_pp = new_item->nodeinst->proto->firstportproto;
		last_col_pp = plac_col_list[0].lastitem->nodeinst->proto->firstportproto;
		plac_wire(x_("Metal-1"), 4*pla_lam, plac_col_list[0].lastitem->nodeinst,
			last_col_pp, new_item->nodeinst, ni_col_pp, pla_array_cell);

		plac_col_list[0].lastitem->bottomitem = new_item;
		plac_col_list[0].lastitem = plac_col_list[0].lastitem->bottomitem;

		plac_row_list[i][0].firstitem = plac_newUCITEM();
		plac_row_list[i][0].lastitem =  plac_row_list[i][0].firstitem;
		plac_row_list[i][2].firstitem = plac_newUCITEM();
		plac_row_list[i][2].lastitem =  plac_row_list[i][2].firstitem;

		plac_row_list[i][0].firstitem->nodeinst = plac_make_Pin(pla_array_cell,
			X-7*pla_lam, Y-5*pla_lam, 6*pla_lam, x_("Metal-1-Polysilicon-Con"));
		if (plac_row_list[i][0].firstitem->nodeinst == NONODEINST) return(TRUE);
		plac_row_list[i][2].firstitem->nodeinst = plac_make_Pin(pla_array_cell,
			X-7*pla_lam, Y-15*pla_lam, 6*pla_lam, x_("Metal-1-Polysilicon-Con"));
		if (plac_row_list[i][2].firstitem->nodeinst == NONODEINST) return(TRUE);

		Y = Y - 2*Y_offset;
	}
	return(FALSE);
}

BOOLEAN plac_pwr_strap(INTBIG i, INTBIG X, INTBIG Y, NODEPROTO *pla_array_cell)
{
	UCITEM *new_item;
	PORTPROTO *ni_col_pp, *last_col_pp;

	new_item = plac_newUCITEM();

	/* put in Substrate contact first */
	new_item->nodeinst = plac_make_Pin(pla_array_cell, X, Y+7*pla_lam,
		14*pla_lam, x_("Metal-1-Substrate-Con"));
	if (new_item->nodeinst == NONODEINST) return(TRUE);

	/* wire in the first power strap and connect to last column object */
	ni_col_pp = new_item->nodeinst->proto->firstportproto;
	last_col_pp = plac_col_list[i].lastitem->nodeinst->proto->firstportproto;

	plac_wire(x_("Metal-1"), 4*pla_lam, plac_col_list[i].lastitem->nodeinst,
		last_col_pp, new_item->nodeinst, ni_col_pp, pla_array_cell);

	/* only put this in a column list */
	plac_col_list[i].lastitem->bottomitem = new_item;
	plac_col_list[i].lastitem = plac_col_list[i].lastitem->bottomitem;
	return(FALSE);
}

BOOLEAN plac_pmos_make_one(INTBIG i, INTBIG X, INTBIG Y, INTBIG row_pos, INTBIG row,
	NODEPROTO *uc_one, NODEPROTO *pla_array_cell)
{
	NODEINST *ni;
	PORTPROTO *ni_col_pp, *last_col_pp;

	ni = plac_make_instance(pla_array_cell, uc_one, X, Y, FALSE);
	if (ni == NONODEINST) return(TRUE);

	/* find ports that need to be connected and connect to last column object */
	ni_col_pp = getportproto(ni->proto, x_("IN.m-1.n"));
	if (plac_col_list[i].lastitem->nodeinst->proto != uc_one) /* a Metal pin */
		last_col_pp = plac_col_list[i].firstitem->nodeinst->proto->firstportproto; else
			last_col_pp = getportproto(plac_col_list[i].lastitem->nodeinst->proto, x_("OUT.m-1.s"));

	plac_wire(x_("Metal-1"), 4*pla_lam, plac_col_list[i].lastitem->nodeinst, last_col_pp,
		ni, ni_col_pp, pla_array_cell);

	/* connect to last GATE object */
	if (plac_row_list[row][row_pos].lastitem->nodeinst->proto != uc_one)
		last_col_pp = plac_row_list[row][row_pos].lastitem->nodeinst->proto->firstportproto; else
			last_col_pp =  getportproto(plac_row_list[row][row_pos].lastitem->nodeinst->proto,
				x_("GATE.p.e"));
	ni_col_pp = getportproto(ni->proto, x_("GATE.p.w"));

	plac_wire(x_("Polysilicon"), 0, plac_row_list[row][row_pos].lastitem->nodeinst,
		last_col_pp, ni, ni_col_pp, pla_array_cell);

	/* put in column list */
	plac_col_list[i].lastitem->bottomitem = plac_newUCITEM();
	plac_col_list[i].lastitem = plac_col_list[i].lastitem->bottomitem;
	plac_col_list[i].lastitem->nodeinst = ni;

	/* put in row list */
	plac_row_list[row][row_pos].lastitem->rightitem = plac_col_list[i].lastitem;
	plac_row_list[row][row_pos].lastitem = plac_row_list[row][row_pos].lastitem->rightitem;
	return(FALSE);
}

BOOLEAN plac_pmos_complete_row(INTBIG row, INTBIG X, INTBIG Y, NODEPROTO *uc_one,
	NODEPROTO *pla_array_cell)
{
	UCITEM *new_item;
	PORTPROTO *last_col_pp, *ni_col_pp;
	CHAR name[40];

	new_item = plac_newUCITEM();
	new_item->nodeinst = plac_make_Pin(pla_array_cell, X,
		Y+2*pla_lam, 6*pla_lam, x_("Metal-1-Polysilicon-Con"));
	if (new_item->nodeinst == NONODEINST) return(TRUE);

	/* connect to last GATE object */
	if (plac_row_list[row][0].lastitem->nodeinst->proto != uc_one)
		last_col_pp = plac_row_list[row][0].lastitem->nodeinst->proto->firstportproto; else
			last_col_pp =  getportproto(plac_row_list[row][0].lastitem->nodeinst->proto, x_("GATE.p.e"));
	ni_col_pp = new_item->nodeinst->proto->firstportproto;
	plac_wire(x_("Polysilicon"), 0, plac_row_list[row][0].lastitem->nodeinst,
		last_col_pp, new_item->nodeinst, ni_col_pp, pla_array_cell);
	plac_row_list[row][0].lastitem = new_item;

	/* Now export port at the beginning and end of this half of row grouping */
	(void)esnprintf(name, 40, x_("ACCESS%ld.m-1.w"), (row * 2));
	(void)newportproto(pla_array_cell, plac_row_list[row][0].firstitem->nodeinst,
		plac_row_list[row][0].firstitem->nodeinst->proto->firstportproto, name);
	(void)esnprintf(name, 40, x_("ACCESS%ld.m-1.e"), (row * 2));
	(void)newportproto(pla_array_cell, plac_row_list[row][0].lastitem->nodeinst,
		plac_row_list[row][0].lastitem->nodeinst->proto->firstportproto, name);
	(void)esnprintf(name, 40, x_("ACCESS%ld.p.w"), (row * 2));
	(void)newportproto(pla_array_cell, plac_row_list[row][0].firstitem->nodeinst,
		plac_row_list[row][0].firstitem->nodeinst->proto->firstportproto, name);
	(void)esnprintf(name, 40, x_("ACCESS%ld.p.e"), (row * 2));
	(void)newportproto(pla_array_cell, plac_row_list[row][0].lastitem->nodeinst,
		plac_row_list[row][0].lastitem->nodeinst->proto->firstportproto, name);

	new_item = plac_newUCITEM();
	new_item->nodeinst = plac_make_Pin(pla_array_cell, X, Y-8*pla_lam,
		6*pla_lam, x_("Metal-1-Polysilicon-Con"));
	if (new_item->nodeinst == NONODEINST) return(TRUE);

	/* connect to last GATE object */
	if (plac_row_list[row][2].lastitem->nodeinst->proto != uc_one)
		last_col_pp = plac_row_list[row][2].lastitem->nodeinst->proto->firstportproto; else
			last_col_pp =  getportproto(plac_row_list[row][2].lastitem->nodeinst->proto, x_("GATE.p.e"));
	ni_col_pp = new_item->nodeinst->proto->firstportproto;
	plac_wire(x_("Polysilicon"), 0, plac_row_list[row][2].lastitem->nodeinst,
		last_col_pp, new_item->nodeinst, ni_col_pp, pla_array_cell);
	plac_row_list[row][2].lastitem = new_item;

	(void)esnprintf(name, 40, x_("ACCESS%ld.m-1.w"), (row * 2 + 1));
	(void)newportproto(pla_array_cell, plac_row_list[row][2].firstitem->nodeinst,
		plac_row_list[row][2].firstitem->nodeinst->proto->firstportproto, name);
	(void)esnprintf(name, 40, x_("ACCESS%ld.m-1.e"), (row * 2 + 1));
	(void)newportproto(pla_array_cell, plac_row_list[row][2].lastitem->nodeinst,
		plac_row_list[row][2].lastitem->nodeinst->proto->firstportproto, name);
	(void)esnprintf(name, 40, x_("ACCESS%ld.p.w"), (row * 2 + 1));
	(void)newportproto(pla_array_cell, plac_row_list[row][2].firstitem->nodeinst,
		plac_row_list[row][2].firstitem->nodeinst->proto->firstportproto, name);
	(void)esnprintf(name, 40, x_("ACCESS%ld.p.e"), (row * 2 + 1));
	(void)newportproto(pla_array_cell, plac_row_list[row][2].lastitem->nodeinst,
		plac_row_list[row][2].lastitem->nodeinst->proto->firstportproto, name);
	return(FALSE);
}

BOOLEAN plac_pmos_finish_columns(INTBIG width, INTBIG X, INTBIG Y, INTBIG X_offset,
	NODEPROTO *nmos_one, NODEPROTO *pla_array_cell)
{
	CHAR name[40];
	INTBIG i, pwr_cnt;
	UCITEM *new_item;
	PORTPROTO *last_col_pp, *ni_col_pp;

	pwr_cnt = 0;
	for (i = 0; i < width; i++)
	{
		new_item = plac_newUCITEM();

		if ((i % 5) == 0)	/* put in a power pin every 5th position */
		{
			(void)esnprintf(name, 40, x_("PWR%ld.m-1.s"), pwr_cnt);
			new_item->nodeinst = plac_make_Pin(pla_array_cell, X_offset * i + X, Y,
				14*pla_lam, x_("Metal-1-Substrate-Con"));
			if (new_item->nodeinst == NONODEINST) return(TRUE);
			last_col_pp = plac_col_list[i].lastitem->nodeinst->proto->firstportproto;
			pwr_cnt++;
		} else
		{
			/* must be a data pin, don't count power pins */
			(void)esnprintf(name, 40, x_("DATA%ld.m-1.s"), (i - pwr_cnt));
			new_item->nodeinst = plac_make_Pin(pla_array_cell,
				X_offset * i + X, Y, 4*pla_lam, x_("Metal-1-Pin"));
			if (new_item->nodeinst == NONODEINST) return(TRUE);
			if (plac_col_list[i].lastitem->nodeinst->proto != nmos_one)
				last_col_pp = plac_col_list[i].lastitem->nodeinst->proto->firstportproto; else
					last_col_pp = getportproto(plac_col_list[i].lastitem->nodeinst->proto,
						x_("OUT.m-1.s"));
		}

		/* wire in the first power strap */
		ni_col_pp = new_item->nodeinst->proto->firstportproto;

		/* connect to last column object */
		plac_wire(x_("Metal-1"), 4*pla_lam, plac_col_list[i].lastitem->nodeinst,
			last_col_pp, new_item->nodeinst, ni_col_pp, pla_array_cell);

		/* only put this in a column list */
		plac_col_list[i].lastitem->bottomitem = new_item;
		plac_col_list[i].lastitem = plac_col_list[i].lastitem->bottomitem;

		(void)newportproto(pla_array_cell, new_item->nodeinst,
			new_item->nodeinst->proto->firstportproto, name);
	}
	return(FALSE);
}

#endif  /* PLATOOL - at top */
