/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: placutils.c
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
#include "placmos.h"
#include "pla.h"
#include "tecgen.h"
#include "usr.h"

/* prototypes for local routines */
static INTBIG plac_init_bits(void);

NODEINST *plac_make_Pin(NODEPROTO *cell, INTBIG X, INTBIG Y, INTBIG size, CHAR *pin_type)
{
	NODEPROTO *node;
	NODEINST  *ni;
	INTBIG x_half_wid, y_half_wid, xs, ys;

	node = getnodeproto(pin_type);
	if (node == NONODEPROTO)
	{
		if (namesame(pin_type, x_("Metal-1-Well-Con")) == 0)
			node = getnodeproto(x_("Metal-1-P-Well-Con"));
		if (namesame(pin_type, x_("Metal-1-Substrate-Con")) == 0)
			node = getnodeproto(x_("Metal-1-N-Well-Con"));

		if (namesame(pin_type, x_("Metal-1-D-Active-Con")) == 0)
			node = getnodeproto(x_("Metal-1-N-Active-Con"));
		if (namesame(pin_type, x_("Metal-1-S-Active-Con")) == 0)
			node = getnodeproto(x_("Metal-1-P-Active-Con"));

		if (namesame(pin_type, x_("Metal-1-Polysilicon-Con")) == 0)
			node = getnodeproto(x_("Metal-1-Polysilicon-1-Con"));
	}
	if (node == NONODEPROTO)
	{
		ttyputerr(_("No %s contact for technology %s"), pin_type, el_curtech->techname);
		node = gen_univpinprim;
	}

	defaultnodesize(node, &xs, &ys);
	if (size < xs) size = xs;
	if (size < ys) size = ys;
	x_half_wid = size/2;
	y_half_wid = size/2;
	ni = newnodeinst(node, X - x_half_wid, X + x_half_wid, Y - y_half_wid,
		Y + y_half_wid,0,0,cell);
	return(ni);
}

UCITEM *plac_newUCITEM(void)
{
	UCITEM *temp;

	temp = (UCITEM *)emalloc(sizeof (UCITEM), pla_tool->cluster);
	temp->nodeinst = NONODEINST;
	temp->rightitem = NOUCITEM;
	temp->bottomitem = NOUCITEM;
	return(temp);
}

void plac_wire(CHAR *type_arc, INTBIG width, NODEINST *fromnodeinst,
	PORTPROTO *fromportproto, NODEINST *tonodeinst, PORTPROTO *toportproto,
	NODEPROTO *cell)
{
	INTBIG X1, Y1, X2, Y2, w, wid;
	ARCPROTO *typ;

	wid = 0;
	portposition(fromnodeinst, fromportproto, &X1, &Y1);
	portposition(tonodeinst, toportproto, &X2, &Y2);
	typ = getarcproto(type_arc);
	if (typ == NOARCPROTO)
	{
		if (namesame(type_arc, x_("Polysilicon")) == 0)
			typ = getarcproto(x_("Polysilicon-1"));
		if (namesame(type_arc, x_("D-active")) == 0)
			typ = getarcproto(x_("N-Active"));
		if (namesame(type_arc, x_("S-active")) == 0)
			typ = getarcproto(x_("P-Active"));
	}
	if (typ == NOARCPROTO)
	{
		ttyputerr(_("Attempting to wire with unknown arc layer"));
		return;
	}
	wid = defaultarcwidth(typ);
	w = us_widestarcinst(typ, fromnodeinst, fromportproto);
	if (w > wid) wid = w;
	w = us_widestarcinst(typ, tonodeinst, toportproto);
	if (w > wid) wid = w;
	if (width > wid) wid = width;
	(void)newarcinst(typ, wid, plac_init_bits(), fromnodeinst, fromportproto,
		X1, Y1, tonodeinst, toportproto, X2, Y2, cell);
}

INTBIG plac_init_bits(void)
{
	return(FIXANG);
}

NODEINST *plac_make_instance(NODEPROTO *pla_cell, NODEPROTO *Inst_proto, INTBIG X,
	INTBIG Y, INTBIG mirror)
{
	INTBIG X1, X2, Y1, Y2, amt;
	NODEINST *node_inst;

	X1 = Inst_proto->lowx + X;
	X2 = Inst_proto->highx + X;
	Y1 = Inst_proto->lowy + Y;
	Y2 = Inst_proto->highy + Y;

	node_inst = newnodeinst(Inst_proto, X1, X2, Y1, Y2, 0, 0, pla_cell);
	if (node_inst == NONODEINST) return(NONODEINST);
	if (mirror == TRUE)
	{
		/* do a horizontal mirror */
		if (node_inst->transpose == 0) amt = 2700; else
			amt = 900;
		modifynodeinst(node_inst, 0, 0, 0, 0, amt, 1-node_inst->transpose*2);
	}
	return(node_inst);
}

#endif  /* PLATOOL - at top */
