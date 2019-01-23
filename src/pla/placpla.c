/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: placpla.c
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

UCROW plac_col_list[PLAC_MAX_COL_SIZE];
UCROW plac_row_list[PLAC_MAX_COL_SIZE][3];

/* Plus Output options eg. column decode etc. */
NODEPROTO *plac_or_plane(LIBRARY *library, NODEPROTO *or_np, CHAR *cell_name,
	INTBIG OUTPUTS)
{
	NODEPROTO *or_plane_np, *buf_np;
	NODEINST *new_ni, *pwr_ni_1, *pwr_ni_2, *gnd_ni_1, *gnd_ni_2;
	NODEINST *last_buf_ni, *n_ni;
	PORTPROTO *pwr_e_pp, *pwr_w_pp, *gnd_1_e_pp, *gnd_1_w_pp, *gnd_2_e_pp,
		*gnd_2_w_pp, *p_pp, *n_pp, *out_pp;
	VARIABLE *var;
	CHAR a_name[60], i_name[60], side;
	INTBIG columns, diff, limit, x, i, cnt, Y, X_offset;
	INTBIG pwr_X, pwr_Y, amt_1, lowx, lowy, highx, highy;

	or_plane_np = newnodeproto(cell_name, library);
	n_ni = plac_make_instance(or_plane_np, or_np, 0, 0, FALSE);
	if (n_ni == NONODEINST) return(NONODEPROTO);

	new_ni = NONODEINST;
	last_buf_ni = NONODEINST;
	var = getval((INTBIG)or_np, VNODEPROTO, VINTEGER, x_("PLA_data_cols"));
	if (var != NOVARIABLE) columns = (INTBIG)((INTBIG *)var->addr); else
	{
		ttyputerr(M_("DATA_cols defaulting to 2"));
		columns = 2;
	}

	if (OUTPUTS != TRUE) side = 'n'; else side = 's';
	lowx = n_ni->geom->lowx;   highx = n_ni->geom->highx;
	lowy = n_ni->geom->lowy;   highy = n_ni->geom->highy;
	buf_np = getnodeproto(x_("io-inv-4"));
	if (buf_np != NONODEPROTO)
	{
		diff = (INTBIG)(buf_np->highy - buf_np->lowy);
		if (diff < 0) diff = -diff;
		limit = columns / 4;
		if ((limit == 0) || ((columns % 4) != 0))  limit = limit + 1;
		cnt = 0;
		if (OUTPUTS == TRUE) Y = lowy - diff; else Y = highy;
		Y -= 5*pla_lam;
		X_offset = lowx + 17*pla_lam;
		(void)esnprintf(a_name, 60, x_("PWR.m-2.w"));
		(void)esnprintf(i_name, 60, x_("PWR.m-2.e"));
		pwr_w_pp = getportproto(buf_np, a_name);
		pwr_e_pp = getportproto(buf_np, i_name);
		(void)esnprintf(a_name, 60, x_("GND.m-2.sw"));
		(void)esnprintf(i_name, 60, x_("GND.m-2.se"));
		gnd_1_w_pp = getportproto(buf_np, a_name);
		gnd_1_e_pp = getportproto(buf_np, i_name);
		(void)esnprintf(a_name, 60, x_("GND.m-2.nw"));
		(void)esnprintf(i_name, 60, x_("GND.m-2.ne"));
		gnd_2_w_pp = getportproto(buf_np, a_name);
		gnd_2_e_pp = getportproto(buf_np, i_name);
		for (i = 0; i < limit; i++)
		{
			new_ni = plac_make_instance(or_plane_np, buf_np,
				i*50*pla_lam + X_offset, Y, FALSE);
			if (new_ni == NONODEINST) return(NONODEPROTO);
			if (OUTPUTS != TRUE)
			{
				if (new_ni->transpose == 0) amt_1 = 900; else amt_1 = 2700;

				/* rotate by 90 degrees */
				modifynodeinst(new_ni, 0, 0, 0, 0, 1800, 0);

				/* mirror */
				modifynodeinst(new_ni, 0, 0, 0, 0, amt_1, 1-new_ni->transpose*2);
			}
			if (last_buf_ni == NONODEINST)
			{
				(void)newportproto(or_plane_np, new_ni, pwr_w_pp, x_("PWR.m-2.w"));
				(void)newportproto(or_plane_np, new_ni, gnd_1_w_pp, x_("GND.m-2.w"));

				/* Put in GND bar pin (metal-2) */
				portposition(new_ni, gnd_1_w_pp, &pwr_X, &pwr_Y);
				gnd_ni_1 = plac_make_Pin(or_plane_np, lowx-13*pla_lam, pwr_Y,
					6*pla_lam, x_("Metal-1-Metal-2-Con"));
				if (gnd_ni_1 == NONODEINST) return(NONODEPROTO);
				plac_wire(x_("Metal-2"), 14*pla_lam, new_ni, gnd_1_w_pp, gnd_ni_1,
					gnd_ni_1->proto->firstportproto, or_plane_np);
				portposition(new_ni, gnd_2_w_pp, &pwr_X, &pwr_Y);

				gnd_ni_2 = plac_make_Pin(or_plane_np, lowx-13*pla_lam, pwr_Y,
					6*pla_lam, x_("Metal-1-Metal-2-Con"));
				if (gnd_ni_2 == NONODEINST) return(NONODEPROTO);
				plac_wire(x_("Metal-2"), 14*pla_lam, new_ni, gnd_2_w_pp, gnd_ni_2,
					gnd_ni_2->proto->firstportproto, or_plane_np);
				plac_wire(x_("Metal-2"), 14*pla_lam, gnd_ni_1,
					gnd_ni_1->proto->firstportproto, gnd_ni_2,
						gnd_ni_2->proto->firstportproto, or_plane_np);
				last_buf_ni = new_ni;
			} else
			{
				/* get wired */
				if ((pwr_w_pp != NOPORTPROTO) && (pwr_e_pp != NOPORTPROTO))
					plac_wire(x_("Metal-2"), 14*pla_lam, last_buf_ni,
						pwr_e_pp, new_ni, pwr_w_pp, or_plane_np);
				if ((gnd_1_w_pp != NOPORTPROTO) && (gnd_1_e_pp != NOPORTPROTO))
					plac_wire(x_("Metal-2"), 14*pla_lam, last_buf_ni,
						gnd_1_e_pp, new_ni, gnd_1_w_pp, or_plane_np);
				if ((gnd_2_w_pp != NOPORTPROTO) && (gnd_2_e_pp != NOPORTPROTO))
					plac_wire(x_("Metal-2"), 14*pla_lam, last_buf_ni,
						gnd_2_e_pp, new_ni, gnd_2_w_pp, or_plane_np);
				last_buf_ni = new_ni;
			}
			for (x = cnt; x < cnt+4; x++)
			{
				(void)esnprintf(a_name, 60, x_("DATA%ld.m-1.%c"), x, side);
				(void)esnprintf(i_name, 60, x_("in%ld.m-1.n"), x % 4);
				p_pp = getportproto(or_np, a_name);
				n_pp = getportproto(buf_np, i_name);
				(void)esnprintf(a_name, 60, x_("out%ld-bar.m-1.s"), x % 4);
				out_pp = getportproto(buf_np, a_name);
				(void)esnprintf(a_name, 60, x_("out%ld.m-1.%c"), cnt + (x % 4), side);
				if (out_pp != NOPORTPROTO)
					(void)newportproto(or_plane_np, new_ni, out_pp, a_name);
				if ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
					plac_wire(x_("Metal-1"), 4*pla_lam, n_ni, p_pp,
						new_ni, n_pp, or_plane_np);
			}
			cnt += 4;
		}
		if (pwr_e_pp != NOPORTPROTO)
		{
			portposition(last_buf_ni, pwr_e_pp, &pwr_X, &pwr_Y);
			pwr_ni_1 = plac_make_Pin(or_plane_np, highx+13*pla_lam, pwr_Y,
				14*pla_lam, x_("Metal-2-Pin"));
			if (pwr_ni_1 == NONODEINST) return(NONODEPROTO);
			plac_wire(x_("Metal-2"), 14*pla_lam, last_buf_ni, pwr_e_pp, pwr_ni_1,
				pwr_ni_1->proto->firstportproto, or_plane_np);
			(void)newportproto(or_plane_np, last_buf_ni, pwr_e_pp, x_("PWR.m-2.e"));
		}
		if (gnd_1_w_pp != NOPORTPROTO)
			(void)newportproto(or_plane_np, last_buf_ni, gnd_1_e_pp, x_("GND.m-2.e"));
	}

	/* OK PUT in the PULLUPS */
	new_ni = NONODEINST;
	last_buf_ni = NONODEINST;
	if (OUTPUTS != TRUE) side = 's'; else side = 'n';
	buf_np = getnodeproto(x_("pullups"));
	if (buf_np != NONODEPROTO)
	{
		diff = buf_np->highy - buf_np->lowy;
		if (diff < 0) diff = -diff;
		cnt = 0;
		if (OUTPUTS != TRUE) Y = lowy - diff; else Y = highy;
		Y += 16*pla_lam;
		X_offset = lowx + 27*pla_lam;
		(void)esnprintf(a_name, 60, x_("PWR.m-2.w"));
		(void)esnprintf(i_name, 60, x_("PWR.m-2.e"));
		pwr_w_pp = getportproto(buf_np, a_name);
		pwr_e_pp = getportproto(buf_np, i_name);
		(void)esnprintf(a_name, 60, x_("GND.m-1.sw"));
		(void)esnprintf(i_name, 60, x_("GND.m-1.se"));
		gnd_1_w_pp = getportproto(buf_np, a_name);
		gnd_1_e_pp = getportproto(buf_np, i_name);
		for (i= 0; i < limit; i++)
		{
			new_ni = plac_make_instance(or_plane_np, buf_np,
				i*50*pla_lam + X_offset, Y, FALSE);
			if (new_ni == NONODEINST) return(NONODEPROTO);
			if (OUTPUTS != TRUE)
			{
				if (new_ni->transpose == 0) amt_1 = 900; else amt_1 = 2700;

				/* rotate by 90 degrees */
				modifynodeinst(new_ni, 0, 0, 0, 0, 1800, 0);

				/* mirror */
				modifynodeinst(new_ni, 0, 0, 0, 0, amt_1, 1-new_ni->transpose*2);
			}
			if (last_buf_ni == NONODEINST)
			{
				(void)newportproto(or_plane_np, new_ni, pwr_w_pp, x_("PWR0.m-2.w"));
				(void)newportproto(or_plane_np, new_ni, gnd_1_w_pp, x_("GND0.m-1.w"));
				portposition(new_ni, gnd_1_w_pp, &pwr_X, &pwr_Y);
				gnd_ni_1 = plac_make_Pin(or_plane_np, lowx-13*pla_lam, pwr_Y,
					6*pla_lam, x_("Metal-1-Metal-2-Con"));
				if (gnd_ni_1 == NONODEINST) return(NONODEPROTO);
				plac_wire(x_("Metal-1"), 4*pla_lam, new_ni, gnd_1_w_pp,
					gnd_ni_1, gnd_ni_1->proto->firstportproto, or_plane_np);
				plac_wire(x_("Metal-2"), 14*pla_lam, gnd_ni_1,
					gnd_ni_1->proto->firstportproto, gnd_ni_2,
						gnd_ni_2->proto->firstportproto, or_plane_np);

				(void)esnprintf(a_name, 60, x_("GND%ld.m-1.%c"), i, side);
				(void)esnprintf(i_name, 60, x_("GND%ld.m-1.%c"), i+1, side);
				gnd_2_w_pp = getportproto(or_np, a_name);
				gnd_2_e_pp = getportproto(or_np, i_name);

				if ((gnd_1_w_pp != NOPORTPROTO) && (gnd_2_w_pp != NOPORTPROTO))
					plac_wire(x_("Metal-1"), 4*pla_lam, new_ni,
						gnd_1_w_pp, n_ni, gnd_2_w_pp, or_plane_np);
				if ((gnd_2_w_pp != NOPORTPROTO) && (gnd_2_e_pp != NOPORTPROTO))
					plac_wire(x_("Metal-1"), 4*pla_lam, new_ni,
						gnd_1_e_pp, n_ni, gnd_2_e_pp, or_plane_np);
				last_buf_ni = new_ni;
			} else
			{
				/* get wired */
				if ((pwr_w_pp != NOPORTPROTO) && (pwr_e_pp != NOPORTPROTO))
					plac_wire(x_("Metal-2"), 14*pla_lam, last_buf_ni,
						pwr_e_pp, new_ni, pwr_w_pp, or_plane_np);
				(void)esnprintf(a_name, 60, x_("GND%ld.m-1.%c"), i, side);
				(void)esnprintf(i_name, 60, x_("GND%ld.m-1.%c"), i+1, side);
				gnd_2_w_pp = getportproto(or_np, a_name);
				gnd_2_e_pp = getportproto(or_np, i_name);
				if ((gnd_1_w_pp != NOPORTPROTO) && (gnd_2_w_pp != NOPORTPROTO))
					plac_wire(x_("Metal-1"), 4*pla_lam, new_ni,
						gnd_1_w_pp, n_ni, gnd_2_w_pp, or_plane_np);
				if ((gnd_1_e_pp != NOPORTPROTO) && (gnd_2_e_pp != NOPORTPROTO))
					plac_wire(x_("Metal-1"), 4*pla_lam, new_ni,
						gnd_1_e_pp, n_ni, gnd_2_e_pp, or_plane_np);
				last_buf_ni = new_ni;
			}
			for (x = cnt; x < cnt+4; x++)
			{
				(void)esnprintf(a_name, 60, x_("DATA%ld.m-1.%c"), x, side);
				(void)esnprintf(i_name, 60, x_("PULLUP%ld.m-1.s"), x % 4);
				p_pp = getportproto(or_np, a_name);
				n_pp = getportproto(buf_np, i_name);
				if ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
					plac_wire(x_("Metal-1"), 4*pla_lam, n_ni, p_pp,
						new_ni, n_pp, or_plane_np);
			}
			cnt += 4;
		}
		if (pwr_e_pp != NOPORTPROTO)
		{
			portposition(last_buf_ni, pwr_e_pp, &pwr_X, &pwr_Y);
			pwr_ni_2 = plac_make_Pin(or_plane_np, highx+13*pla_lam,
				pwr_Y, 14*pla_lam, x_("Metal-2-Pin"));
			if (pwr_ni_2 == NONODEINST) return(NONODEPROTO);
			plac_wire(x_("Metal-2"), 14*pla_lam, last_buf_ni, pwr_e_pp,
				pwr_ni_2, pwr_ni_2->proto->firstportproto, or_plane_np);
			plac_wire(x_("Metal-2"), 14*pla_lam, pwr_ni_1,
				pwr_ni_1->proto->firstportproto, pwr_ni_2,
					pwr_ni_2->proto->firstportproto, or_plane_np);
			(void)newportproto(or_plane_np, last_buf_ni, pwr_e_pp, x_("PWR0.m-2.e"));
		}
		if (gnd_1_e_pp != NOPORTPROTO)
			(void)newportproto(or_plane_np, last_buf_ni, gnd_1_e_pp, x_("GND0.m-1.e"));
		x = 0;
		(void)esnprintf(a_name, 60, x_("ACCESS%ld.p.w"), x);
		p_pp = getportproto(or_np, a_name);
		while (p_pp != NOPORTPROTO)
		{
			(void)newportproto(or_plane_np, n_ni, p_pp, a_name);
			x++;
			(void)esnprintf(a_name, 60, x_("ACCESS%ld.p.w"), x);
			p_pp = getportproto(or_np, a_name);
		}
	}

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(or_plane_np);

	return(or_plane_np);
}

NODEPROTO *plac_make_pla(LIBRARY *library, NODEPROTO *decode_np, NODEPROTO *or_plane_np,
	CHAR *name)
{
	NODEPROTO *pla_np;
	NODEINST *decode_ni, *or_ni;
	INTBIG x;
	PORTPROTO *dec_pp, *or_pp;
	CHAR a_name[40];
	INTBIG dy, dec_X, dec_Y, or_X, or_Y;

	pla_np = newnodeproto(name, library);
	decode_ni = plac_make_instance(pla_np, decode_np, 0, 0, FALSE);
	if (decode_ni == NONODEINST) return(NONODEPROTO);
	or_ni = plac_make_instance(pla_np, or_plane_np, decode_ni->geom->highx+27*pla_lam, 0, FALSE);
	if (or_ni == NONODEINST) return(NONODEPROTO);

	x = 0;
	(void)esnprintf(a_name, 40, x_("ACCESS%ld.p.w"), x);
	or_pp = getportproto(or_plane_np, a_name);
	(void)esnprintf(a_name, 40, x_("DATA%ld.m-1.n"), x);
	dec_pp = getportproto(decode_np, a_name);

	portposition(or_ni, or_pp, &or_X, &or_Y);
	portposition(decode_ni, dec_pp, &dec_X, &dec_Y);
	dy = dec_Y - or_Y;
	if (or_Y != dec_Y) modifynodeinst(or_ni, 0, dy, 0, dy, 0, 0);

	while ((or_pp != NOPORTPROTO) && (dec_pp != NOPORTPROTO))
	{
		plac_wire(x_("Metal-1"), 4*pla_lam, or_ni, or_pp, decode_ni, dec_pp, pla_np);
		x++;
		(void)esnprintf(a_name, 40, x_("ACCESS%ld.p.w"), x);
		or_pp = getportproto(or_plane_np, a_name);
		(void)esnprintf(a_name, 40, x_("DATA%ld.m-1.n"), x);
		dec_pp = getportproto(decode_np, a_name);
	}

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(pla_np);

	return(pla_np);
}

#endif /* PLATOOL - at top */
