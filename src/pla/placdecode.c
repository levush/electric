/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: placdecode.c
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
static NODEINST *plac_decode_bufs(NODEPROTO*, NODEINST*, NODEPROTO*, NODEINST*, NODEPROTO*, INTBIG,
					NODEINST**, INTBIG);
static void plac_dec_exp(NODEPROTO*, NODEINST*, NODEPROTO*);
static BOOLEAN plac_decode_route(NODEPROTO*, NODEINST*, NODEPROTO*, NODEINST*, NODEPROTO*, NODEINST*,
					NODEINST*, INTBIG);

NODEPROTO *plac_decode_gen(LIBRARY *library, NODEPROTO *pmos_np, NODEPROTO *nmos_np,
	CHAR *cell_name, INTBIG INPUTS)
{
	NODEPROTO *decode_np;
	NODEINST *p_ni, *n_ni, *first_buf, *last_buf;
	INTBIG X;

	decode_np = newnodeproto(cell_name, library);
	p_ni = plac_make_instance(decode_np, pmos_np, 0, 0, FALSE);
	if (p_ni == NONODEINST) return(NONODEPROTO);
	modifynodeinst(p_ni, 0, 0, 0, 0, 2700, 0);  /* rotate by 90 degrees */

	X = p_ni->geom->highx - p_ni->geom->lowx;  /* CHG */
	if (X < 0) X = -X;
	X = X + 0 +  12*pla_lam; /* SEPARATION */

	n_ni = plac_make_instance(decode_np, nmos_np, X, 0, FALSE);
	if (n_ni == NONODEINST) return(NONODEPROTO);
	modifynodeinst(n_ni, 0, 0, 0, 0, 2700, 0); /* rotate by 90 degrees */

	first_buf = plac_decode_bufs(pmos_np, p_ni, nmos_np, n_ni, decode_np, X, &last_buf, INPUTS);
	if (first_buf == NONODEINST) return(NONODEPROTO);
	if (plac_decode_route(pmos_np, p_ni, nmos_np, n_ni, decode_np, first_buf,
		last_buf, INPUTS)) return(NONODEPROTO);
	plac_dec_exp(nmos_np, n_ni, decode_np);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(decode_np);

	return(decode_np);
}

NODEINST *plac_decode_bufs(NODEPROTO *pmos_np, NODEINST *p_ni, NODEPROTO *nmos_np,
	NODEINST *n_ni, NODEPROTO *decode_np, INTBIG X, NODEINST **last_buf, INTBIG INPUTS)
{
	NODEINST *first_p_ni, *last_p_ni, *last_n_ni, *newno, *newno_2;
	PORTPROTO *pwr_e_pp, *pwr_w_pp, *gnd_e_pp, *gnd_w_pp, *p_pp, *n_pp;
	NODEPROTO *buf_np;
	VARIABLE *var;
	CHAR a_name[60], i_name[60], side;
	INTBIG rows, diff, limit, x, i, cnt, Y, X_offset, amt_1, amt_2;
	INTBIG lowx, lowy, highy;

	newno = NONODEINST;
	first_p_ni = last_p_ni = last_n_ni = NONODEINST;
	var = getval((INTBIG)pmos_np, VNODEPROTO, VINTEGER, x_("PLA_access_rows"));
	if (var != NOVARIABLE) rows = (INTBIG)((INTBIG *)var->addr); else
	{
		ttyputerr(_("ACCESS_rows defaulting to 2"));
		rows = 2;
	}

	if (INPUTS == TRUE) side = 'e'; else side = 'w';
	lowx = p_ni->geom->lowx;   /* highx = p_ni->geom->highx; */
	lowy = p_ni->geom->lowy;   highy = p_ni->geom->highy;
	buf_np = getnodeproto(x_("decoder_inv1"));
	if (buf_np != NONODEPROTO)
	{
		diff = (INTBIG)(buf_np->highy - buf_np->lowy);
		if (diff < 0) diff = -diff;
		limit = rows / 4;
		if ((limit == 0) || ((rows % 4) != 0))  limit = limit + 1;
		cnt = 0;
		if (INPUTS == TRUE) Y = lowy - diff; else Y = highy;
		Y -= 13*pla_lam;
		if ((rows %4) == 0) X_offset = lowx - 18*pla_lam; else
			X_offset = lowx - 38*pla_lam;
		(void)esnprintf(a_name, 60, x_("PWR.m-2.w"));
		(void)esnprintf(i_name, 60, x_("PWR.m-2.e"));
		pwr_w_pp = getportproto(buf_np, a_name);
		pwr_e_pp = getportproto(buf_np, i_name);
		(void)esnprintf(a_name, 60, x_("GND.m-2.w"));
		(void)esnprintf(i_name, 60, x_("GND.m-2.e"));
		gnd_w_pp = getportproto(buf_np, a_name);
		gnd_e_pp = getportproto(buf_np, i_name);
		for (i= limit; i > 0; i--)
		{
			newno = plac_make_instance(decode_np, buf_np,
				i*50*pla_lam + X_offset, Y, FALSE);
			if (newno == NONODEINST) return(NONODEINST);
			newno_2 = plac_make_instance(decode_np, buf_np,
				X + i*50*pla_lam + X_offset, Y, FALSE);
			if (newno_2 == NONODEINST) return(NONODEINST);
			if (INPUTS != TRUE)
			{
				if (newno_2->transpose == 0) amt_2 = 900; else amt_2 = 2700;
				if (newno->transpose == 0) amt_1 = 900; else amt_1 = 2700;

				/* rotate by 90 degrees */
				modifynodeinst(newno_2, 0, 0, 0, 0, 1800, 0);
				modifynodeinst(newno, 0, 0, 0, 0, 1800, 0);

				/* mirror */
				modifynodeinst(newno_2, 0, 0, 0, 0, amt_2, 1-newno_2->transpose*2);
				modifynodeinst(newno, 0, 0, 0, 0, amt_1, 1-newno->transpose*2);
			}
			if ((last_p_ni == NONODEINST) && (last_n_ni == NONODEINST))
			{
				(void)newportproto(decode_np, newno_2, pwr_e_pp, x_("PWR.m-2.e"));
				(void)newportproto(decode_np, newno_2, gnd_e_pp, x_("GND.m-2.e"));
				first_p_ni = last_p_ni =  newno;
				last_n_ni = *last_buf = newno_2;
			} else
			{
				/* get wired */
				if ((pwr_w_pp != NOPORTPROTO) && (pwr_e_pp != NOPORTPROTO))
				{
					plac_wire(x_("Metal-2"), 14*pla_lam, last_p_ni,
						pwr_w_pp, newno, pwr_e_pp, decode_np);
					plac_wire(x_("Metal-2"), 14*pla_lam, last_n_ni,
						pwr_w_pp, newno_2, pwr_e_pp, decode_np);
				}
				if ((gnd_w_pp != NOPORTPROTO) && (gnd_e_pp != NOPORTPROTO))
				{
					plac_wire(x_("Metal-2"), 14*pla_lam, last_n_ni,
						gnd_w_pp, newno, gnd_e_pp, decode_np);
					plac_wire(x_("Metal-2"), 14*pla_lam, last_n_ni,
						gnd_w_pp, newno_2, gnd_e_pp, decode_np);
				}
				last_p_ni = newno;
				last_n_ni = newno_2;
			}
			for (x = cnt; x < cnt+4; x++)
			{
				(void)esnprintf(a_name, 60, x_("ACCESS%ld.m-1.%c"), x, side);
				(void)esnprintf(i_name, 60, x_("LINE%ld.p.n"), x % 4);
				p_pp = getportproto(pmos_np, a_name);
				n_pp = getportproto(buf_np, i_name);
				if ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
					plac_wire(x_("Polysilicon"), 2*pla_lam, p_ni, p_pp,
						newno, n_pp, decode_np);
				p_pp = getportproto(nmos_np, a_name);
				if ((n_pp != NOPORTPROTO) && (p_pp != NOPORTPROTO))
					plac_wire(x_("Polysilicon"), 2*pla_lam, n_ni, p_pp,
						newno_2, n_pp, decode_np);
			}
			cnt += 4;
		}
	}
	if ((pwr_w_pp != NOPORTPROTO) && (pwr_e_pp != NOPORTPROTO))
	{
		plac_wire(x_("Metal-2"), 14*pla_lam, last_n_ni, pwr_w_pp,
			first_p_ni, pwr_e_pp, decode_np);
		(void)newportproto(decode_np, last_p_ni, pwr_w_pp, x_("PWR.m-2.w"));
	}
	if ((gnd_w_pp != NOPORTPROTO) && (gnd_e_pp != NOPORTPROTO))
	{
		plac_wire(x_("Metal-2"), 14*pla_lam, last_n_ni, gnd_w_pp,
			first_p_ni, gnd_e_pp, decode_np);
		(void)newportproto(decode_np, last_p_ni, gnd_w_pp, x_("GND.m-2.w"));
	}
	return(last_p_ni);
}

void plac_dec_exp(NODEPROTO *nmos_np, NODEINST *n_ni, NODEPROTO *decode_np)
{
	PORTPROTO *pp;
	CHAR name[100], side;
	INTBIG x = 0;

	side = 'n';
	(void)esnprintf(name, 100, x_("DATA%ld.m-1.%c"), x, side);
	pp = getportproto(nmos_np, name);
	while (pp != NOPORTPROTO)
	{
		(void)newportproto(decode_np, n_ni, pp, name);
		x = x + 1;
		(void)esnprintf(name, 100, x_("DATA%ld.m-1.%c"), x, side);
		pp = getportproto(nmos_np, name);
	}
}

BOOLEAN plac_decode_route(NODEPROTO *pmos_np, NODEINST *p_ni, NODEPROTO *nmos_np,
	NODEINST *n_ni, NODEPROTO *decode_np, NODEINST *first_buf, NODEINST *last_buf, INTBIG INPUTS)
{
	INTBIG cnt, pwr_cnt, in_cnt, sep;
	CHAR name[100];
	PORTPROTO *p_pp, *n_pp, *pwr_pp;
	NODEINST *pm_ni_1, *pm_ni_2;
	INTBIG p_X, p_Y, n_X, n_Y, Y;
	INTBIG init_offset;
	VARIABLE *var;
	INTBIG columns;
	CHAR side, ac_side;
	INTBIG O_Y;

	/*** Make connection between N and P planes ***/
	(void)esnprintf(name, 100, x_("DATA%d.m-1.n"), 0);
	p_pp = getportproto(pmos_np, name);
	(void)esnprintf(name, 100, x_("DATA%d.m-1.s"), 0);
	n_pp = getportproto(nmos_np, name);
	cnt = 0;
	while ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
	{
		/* Connect them up here */
		plac_wire(x_("Metal-1"), 4*pla_lam, p_ni, p_pp, n_ni, n_pp, decode_np);

		/* now find the next ports */
		cnt += 1;
		(void)esnprintf(name, 100, x_("DATA%ld.m-1.n"), cnt);
		p_pp = getportproto(pmos_np, name);
		(void)esnprintf(name, 100, x_("DATA%ld.m-1.s"), cnt);
		n_pp = getportproto(nmos_np, name);
	}

	/*** Make connection between PWR and P-plane ***/
	(void)esnprintf(name, 100, x_("DATA%d.m-1.s"), 0);
	p_pp = getportproto(pmos_np, name);
	(void)esnprintf(name, 100, x_("PWR%d.m-1.s"), 0);
	n_pp = getportproto(pmos_np, name);
	if ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
		plac_wire(x_("Metal-1"), 4*pla_lam, p_ni, p_pp, p_ni, n_pp, decode_np);
	portposition(p_ni, n_pp, &p_X, &p_Y);
	pm_ni_1 = plac_make_Pin(decode_np, p_X-13*pla_lam, p_Y,
		6*pla_lam, x_("Metal-1-Metal-2-Con"));
	if (pm_ni_1 == NONODEINST) return(TRUE);
	plac_wire(x_("Metal-1"), 4*pla_lam, p_ni, n_pp, pm_ni_1,
		pm_ni_1->proto->firstportproto, decode_np);
	if (INPUTS != TRUE) /* Buffers are at the top of the cell */
	{
		pwr_pp = getportproto(first_buf->proto, x_("PWR.m-2.w"));
		pm_ni_2 = plac_make_Pin(decode_np, p_X-13*pla_lam,
			p_Y+24*pla_lam, 14*pla_lam, x_("Metal-2-Pin"));
		if (pm_ni_2 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-2"), 14*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		plac_wire(x_("Metal-2"), 14*pla_lam, first_buf, pwr_pp,
			pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
	}
	n_pp = p_pp;
	(void)esnprintf(name, 100, x_("DATA%d.m-1.s"), 1);
	p_pp = getportproto(pmos_np, name);
	cnt = 1;
	pwr_cnt = 0;
	while ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
	{
		/* Connect them up here */
		plac_wire(x_("Metal-1"), 4*pla_lam, p_ni, p_pp, p_ni, n_pp, decode_np);

		/* now find the next ports */
		cnt += 1;
		n_pp = p_pp;
		if ((cnt % 4) == 0)
		{
			pwr_cnt += 1;
			(void)esnprintf(name, 100, x_("PWR%ld.m-1.s"), pwr_cnt);
			p_pp = getportproto(pmos_np, name);
			if (p_pp != NOPORTPROTO)
				plac_wire(x_("Metal-1"), 4*pla_lam, p_ni, p_pp, p_ni, n_pp, decode_np);
			portposition(p_ni, p_pp, &p_X, &p_Y);
			pm_ni_2 = plac_make_Pin(decode_np, p_X-13*pla_lam, p_Y,
				6*pla_lam, x_("Metal-1-Metal-2-Con"));
			if (pm_ni_2 == NONODEINST) return(TRUE);
			plac_wire(x_("Metal-1"), 4*pla_lam, p_ni, p_pp, pm_ni_2,
				pm_ni_2->proto->firstportproto, decode_np);
			plac_wire(x_("Metal-2"), 14*pla_lam, pm_ni_1,
				pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
			pm_ni_1 = pm_ni_2;
			n_pp = p_pp;
		}
		(void)esnprintf(name, 100, x_("DATA%ld.m-1.s"), cnt);
		p_pp = getportproto(pmos_np, name);
	}
	if (INPUTS == TRUE) /* Buffers are at the top of the cell */
	{
		pwr_pp = getportproto(first_buf->proto, x_("PWR.m-2.w"));
		pm_ni_2 = plac_make_Pin(decode_np, p_X-13*pla_lam,
			p_Y-24*pla_lam, 14*pla_lam, x_("Metal-2-Pin"));
		if (pm_ni_2 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-2"), 14*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		plac_wire(x_("Metal-2"), 14*pla_lam, first_buf, pwr_pp,
			pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
	}

	/*** Make connection between GND and N-plane ***/
	(void)esnprintf(name, 100, x_("GND%d.m-1.n"), 0);
	n_pp = getportproto(nmos_np, name);
	portposition(n_ni, n_pp, &p_X, &p_Y);
	pm_ni_1 = plac_make_Pin(decode_np, p_X+13*pla_lam, p_Y,
		6*pla_lam, x_("Metal-1-Metal-2-Con"));
	if (pm_ni_1 == NONODEINST) return(TRUE);
	plac_wire(x_("Metal-1"), 4*pla_lam, n_ni, n_pp, pm_ni_1,
		pm_ni_1->proto->firstportproto, decode_np);
	if (INPUTS != TRUE) /* Buffers are at the top of the cell */
	{
		pwr_pp = getportproto(last_buf->proto, x_("GND.m-2.e"));
		pm_ni_2 = plac_make_Pin(decode_np, p_X+13*pla_lam,
			p_Y+90*pla_lam, 14*pla_lam, x_("Metal-2-Pin"));
		if (pm_ni_2 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-2"), 14*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		plac_wire(x_("Metal-2"), 14*pla_lam, last_buf, pwr_pp,
			pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
	}
	cnt = 1;
	(void)esnprintf(name, 100, x_("GND%ld.m-1.n"), cnt);
	n_pp = getportproto(nmos_np, name);
	while (n_pp != NOPORTPROTO)
	{
		/* Connect them up here */
		portposition(n_ni, n_pp, &p_X, &p_Y);
		pm_ni_2 = plac_make_Pin(decode_np, p_X+13*pla_lam, p_Y,
			6*pla_lam, x_("Metal-1-Metal-2-Con"));
		if (pm_ni_2 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-1"), 4*pla_lam, n_ni, n_pp, pm_ni_2,
			pm_ni_2->proto->firstportproto, decode_np);
		plac_wire(x_("Metal-2"), 14*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		pm_ni_1 = pm_ni_2;
		cnt++;
		(void)esnprintf(name, 100, x_("GND%ld.m-1.n"), cnt);
		n_pp = getportproto(nmos_np, name);
	}
	if (INPUTS == TRUE) /* Buffers are at the bottom of the cell */
	{
		pwr_pp = getportproto(last_buf->proto, x_("GND.m-2.e"));
		pm_ni_2 = plac_make_Pin(decode_np, p_X+13*pla_lam,
			p_Y-90*pla_lam, 14*pla_lam, x_("Metal-2-Pin"));
		if (pm_ni_2 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-2"), 14*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		plac_wire(x_("Metal-2"), 14*pla_lam, last_buf, pwr_pp,
			pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
	}

	if (INPUTS == TRUE)
	{
		side = 'n';
		ac_side = 'w';
	} else
	{
		side = 's';
		ac_side = 'e';
	}
	var = getval((INTBIG)pmos_np, VNODEPROTO, VINTEGER, x_("PLA_access_rows"));
	if (var != NOVARIABLE)
	{
		columns = (INTBIG)((INTBIG *)var->addr);
		columns = columns/2;
	} else
	{
		ttyputerr(_("DATA_cols defaulting to 2"));
		columns = 2;
	}

	init_offset = 8*pla_lam;
	sep = 7*pla_lam;
	cnt = 0;
	(void)esnprintf(name, 100, x_("ACCESS%ld.m-1.%c"), cnt, ac_side);
	p_pp = getportproto(pmos_np, name);
	n_pp = getportproto(nmos_np, name);
	while ((p_pp != NOPORTPROTO) && (n_pp != NOPORTPROTO))
	{
		/* wire it */
		in_cnt = cnt/2;
		portposition(p_ni, p_pp, &p_X, &p_Y);
		portposition(n_ni, n_pp, &n_X, &n_Y);
		if (INPUTS == TRUE)
		{
			Y = n_Y + init_offset + in_cnt*sep;
			O_Y = Y + (columns - in_cnt - 1)*sep;
		} else
		{
			Y = n_Y - init_offset - in_cnt*sep;
			O_Y = Y - (columns - in_cnt - 1)*sep;
		}
		pm_ni_1 = plac_make_Pin(decode_np, p_X, Y, 6*pla_lam,
			x_("Metal-1-Polysilicon-Con"));
		if (pm_ni_1 == NONODEINST) return(TRUE);
		pm_ni_2 = plac_make_Pin(decode_np, n_X, Y, 6*pla_lam,
			x_("Metal-1-Polysilicon-Con"));
		if (pm_ni_2 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-1"), 4*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		plac_wire(x_("Polysilicon"), 2*pla_lam, p_ni, p_pp, pm_ni_1,
			pm_ni_1->proto->firstportproto, decode_np);
		plac_wire(x_("Polysilicon"), 2*pla_lam, n_ni, n_pp, pm_ni_2,
			pm_ni_2->proto->firstportproto, decode_np);

		pm_ni_1 = plac_make_Pin(decode_np, n_X, O_Y, 4*pla_lam, x_("Metal-1-Pin"));
		if (pm_ni_1 == NONODEINST) return(TRUE);
		plac_wire(x_("Metal-1"), 4*pla_lam, pm_ni_1,
			pm_ni_1->proto->firstportproto, pm_ni_2, pm_ni_2->proto->firstportproto, decode_np);
		(void)esnprintf(name, 100, x_("INPUT%ld.m-1.%c"), in_cnt, side);
		(void)newportproto(decode_np, pm_ni_1, pm_ni_1->proto->firstportproto, name);
		cnt += 2;  /* Only route every second one */
		(void)esnprintf(name, 100, x_("ACCESS%ld.m-1.%c"), cnt, ac_side);
		p_pp = getportproto(pmos_np, name);
		n_pp = getportproto(nmos_np, name);
	}
	return(FALSE);
}

#endif  /* PLATOOL - at top */
