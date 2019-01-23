/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: planfacets.c
 * Programmable Logic Array Generator: Defining the cells that make up a PLA
 * Written by: Steven M. Rubin, Static Free Software
 * Written by: Sundaravarathan R. Iyengar, Schlumberger Palo Alto Research
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
#include "planmos.h"

/* prototypes for local routines */
static NODEPROTO *pla_nmos_InPullup(void);
static NODEPROTO *pla_nmos_OutPullup(void);

PORTPROTO *pla_FindPort(NODEINST *node, INTBIG x, INTBIG y)
{
	REGISTER PORTPROTO *pp;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, pla_tool->cluster);
	if (node != NONODEINST)
	{
		for(pp = node->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			shapeportpoly(node, pp, poly, FALSE);
			if (isinside(x, y, poly)) return(pp);
		}
	}
	return(NOPORTPROTO);
}

NODEPROTO *pla_nmos_Connect(void)
{
	NODEPROTO *cell;
	NODEINST *mp_contact, *md_contact, *poly_pin;
	ARCINST *pa;

	cell = newnodeproto(nmosConnect, el_curlib);
	if (cell == NONODEPROTO) return(NONODEPROTO);

	mp_contact = newnodeinst(pla_mp_proto, 0, 4*pla_lam, 3*pla_lam, 7*pla_lam, NOTRANS, NOROT,
		cell);
	poly_pin = newnodeinst(pla_pp_proto, 6*pla_lam+pla_GndWidth, 8*pla_lam+pla_GndWidth,
		5*pla_lam, 7*pla_lam, NOTRANS, NOROT, cell);
	if (mp_contact == NONODEINST || poly_pin == NONODEINST) return(NONODEPROTO);

	pa = newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, poly_pin, pla_pp_proto->firstportproto,
		7*pla_lam+pla_GndWidth, 6*pla_lam, mp_contact, pla_mp_proto->firstportproto, 2*pla_lam,
			6*pla_lam, cell);
	if (pa == NOARCINST) return(NONODEPROTO);

	/* Extend the AndEnd of the metal-poly pin by one lambda */
	if (newarcinst(pla_ma_proto, 4*pla_lam, FIXANG, mp_contact, pla_mp_proto->firstportproto,
		pla_lam, 5*pla_lam, mp_contact, pla_mp_proto->firstportproto, 2*pla_lam, 5*pla_lam,
			cell) == NOARCINST) return(NONODEPROTO);

	/* Instance a metal-diff contact for ground connection and export it */
	md_contact = newnodeinst(pla_md_proto, 7*pla_lam, 7*pla_lam+pla_GndWidth,
		0, 4*pla_lam, NOTRANS, NOROT, cell);
	if (md_contact == NONODEINST) return(NONODEPROTO);

	if ((newportproto(cell, md_contact, pla_md_proto->firstportproto, x_("DiffGnd")) == NOPORTPROTO) ||
		(newportproto(cell, mp_contact, pla_mp_proto->firstportproto, x_("AndEnd")) == NOPORTPROTO) ||
		(newportproto(cell, poly_pin, pla_pp_proto->firstportproto, x_("OrEnd")) == NOPORTPROTO))
			return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(cell);
	return(cell);
}

NODEPROTO *pla_nmos_InPullup(void)
{
	NODEPROTO *pu;
	NODEINST *dtran, *bc, *dpin, *ppin1, *ppin2;
	ARCINST *pa, *da;
	PORTPROTO *dtran_dport, *dtran_pport, *bc_dport, *bc_pport;

	pu = newnodeproto(x_("nmos_InPullup"),el_curlib);
	if (pu == NONODEPROTO) return(NONODEPROTO);

	dtran = newnodeinst(pla_dtran_proto, pla_lam, 7*pla_lam, 5*pla_lam, 11*pla_lam, NOTRANS, NOROT,
		pu);
	if (dtran == NONODEINST) return(NONODEPROTO);

	dtran_dport = pla_FindPort(dtran, 4*pla_lam, 6*pla_lam);
	dtran_pport = pla_FindPort(dtran, 2*pla_lam, 8*pla_lam);
	if (dtran_pport == NOPORTPROTO || dtran_dport == NOPORTPROTO) return(NONODEPROTO);

	if (!pla_buttingcontact) return(NONODEPROTO);
	bc = newnodeinst(pla_bc_proto, pla_lam, 7*pla_lam, 0, 4*pla_lam, NOTRANS, 1800, pu);
	if (bc == NONODEINST) return(NONODEPROTO);

	bc_dport = pla_FindPort(bc, 6*pla_lam, 2*pla_lam);
	bc_pport = pla_FindPort(bc, 3*pla_lam, pla_lam);
	if (bc_pport == NOPORTPROTO || bc_dport == NOPORTPROTO) return(NONODEPROTO);

	/* export the bc_dport, bc_pport for external connections */
	if (newportproto(pu, bc, bc_dport, x_("PullDNpt")) == NOPORTPROTO ||
		newportproto(pu, bc, bc_pport, x_("Outpt")) == NOPORTPROTO) return(NONODEPROTO);

	/* have to create pins to bend the arcs */
	dpin = newnodeinst(pla_dp_proto, 3*pla_lam, 7*pla_lam, 2*pla_lam, 6*pla_lam, NOTRANS, NOROT, pu);
	ppin1 = newnodeinst(pla_pp_proto, 0, 2*pla_lam, 7*pla_lam, 9*pla_lam, NOTRANS, NOROT, pu);
	if (dpin == NONODEINST || ppin1 == NONODEINST) return(NONODEPROTO);

	/* connect the pins to the dtran */
	pa = newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, dtran, dtran_pport, 2*pla_lam, 8*pla_lam,
		ppin1, pla_pp_proto->firstportproto, pla_lam, 8*pla_lam, pu);
	da = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dtran, dtran_dport, 4*pla_lam,6*pla_lam,
		dpin, pla_dp_proto->firstportproto, 4*pla_lam, 4*pla_lam, pu);
	if (pa == NOARCINST || da == NOARCINST) return(NONODEPROTO);

	/* now connect these pins to the butting contact */
	ppin2 = newnodeinst(pla_pp_proto, 0, 2*pla_lam, 0, 2*pla_lam, NOTRANS, NOROT, pu);
	if (ppin2 == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin1, pla_pp_proto->firstportproto,
		pla_lam, 8*pla_lam, ppin2, pla_pp_proto->firstportproto, pla_lam, pla_lam, pu) == NOARCINST)
			return(NONODEPROTO);

	pa = newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin2, pla_pp_proto->firstportproto,
		pla_lam, pla_lam, bc, bc_pport, 3*pla_lam, pla_lam, pu);
	da = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		6*pla_lam, 3*pla_lam, bc, bc_dport, 6*pla_lam, 2*pla_lam, pu);
	if (pa == NOARCINST || da == NOARCINST) return(NONODEPROTO);

	/*
	 * export the diffusion top of the dtran for Vdd connection
	 * and the right poly end
	 */
	dtran_dport = pla_FindPort(dtran, 4*pla_lam, 10*pla_lam);
	if (dtran_dport == NOPORTPROTO) return(NONODEPROTO);
	if (newportproto(pu, dtran, dtran_dport, x_("Vddpt")) == NOPORTPROTO) return(NONODEPROTO);

	dtran_pport = pla_FindPort(dtran, 6*pla_lam, 8*pla_lam);
	if (dtran_pport == NOPORTPROTO) return(NONODEPROTO);
	if (newportproto(pu, dtran, dtran_pport, x_("Invpt")) == NOPORTPROTO) return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(pu);
	return(pu);
}

NODEPROTO *pla_nmos_Input(void)
{
	NODEPROTO *cell, *pu;
	NODEINST *pu1,  *pu2, *ppin, *etran1, *etran2,
		*ppin1, *ppin2, *dpin1, *dpin2, *md_contact, *md_contact1;
	ARCINST *ai;

	/* make sure Input Pullup is defined */
	pu = pla_nmos_InPullup();
	if (pu == NONODEPROTO) return(NONODEPROTO);

	/*
	 * Now create a diffusion to metal contact wide enough to accommodate
	 * the Vdd line.  Assume that the global variables pla_VddWidth and
	 * pla_GndWidth contain the widths of  Vdd and Gnd lines.  The
	 * Y size of the Input cell is these two widths plus the heights
	 * two inverters.  These inverters are identical.  The difference
	 * in heights (see code below) comes from the savings resulting
	 * from the fact that these two are interconnected.
	 */
	cell = newnodeproto(x_("nmos_Input"), el_curlib);
	if (cell == NONODEPROTO) return(NONODEPROTO);

	md_contact = newnodeinst(pla_md_proto, 2*pla_lam, 8*pla_lam,
		17*pla_lam, 17*pla_lam+pla_VddWidth, NOTRANS, NOROT, cell);
	if (md_contact == NONODEINST) return(NONODEPROTO);

	/* export this for vdd connection */
	if (newportproto(cell, md_contact, pla_md_proto->firstportproto, x_("Vdd")) ==
		NOPORTPROTO) return(NONODEPROTO);

	/* Instance the pullups on either side (top and bottom) of this contact */
	pu1 = newnodeinst(pu, 0, 7*pla_lam, 7*pla_lam, 18*pla_lam, NOTRANS, NOROT, cell);
	pu2 = newnodeinst(pu, 0, 7*pla_lam, 16*pla_lam+pla_VddWidth,
		27*pla_lam+pla_VddWidth, TRANS, 2700, cell);
	if (pu1 == NONODEINST || pu2 == NONODEINST) return(NONODEPROTO);

	/* connect these pullups to the Vdd line */
	ai = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, pu1, pla_FindPort(pu1,4*pla_lam,17*pla_lam),
		4*pla_lam, 17*pla_lam, md_contact, pla_md_proto->firstportproto, 4*pla_lam, 18*pla_lam,
			cell);
	if (ai == NOARCINST) return(NONODEPROTO);

	ai = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, pu2,
		pla_FindPort(pu2,4*pla_lam,17*pla_lam+pla_VddWidth), 4*pla_lam, 17*pla_lam+pla_VddWidth,
			md_contact, pla_md_proto->firstportproto, 4*pla_lam, (17-1)*pla_lam+pla_VddWidth, cell);
	if (ai == NOARCINST) return(NONODEPROTO);

	/*
	 * Create two pulldown transistors to be combined with the pullups
	 * to make two inverters. They appear on the right of the pullups.
	 * etran1 is centered at (10,6). etran2 is centered at
	 * (10,17+11-2+pla_VddWidth/pla_lam)
	 */
	etran1 = newnodeinst(pla_etran_proto, 4*pla_lam, 16*pla_lam,
		3*pla_lam, 9*pla_lam, NOTRANS, 900, cell);
	etran2 = newnodeinst(pla_etran_proto, 4*pla_lam, 16*pla_lam,
		(17+11-3-3)*pla_lam+pla_VddWidth, (17+11-3+3)*pla_lam+pla_VddWidth, NOTRANS, 900, cell);
	if (etran1 == NONODEINST || etran2 == NONODEINST) return(NONODEPROTO);

	/* connect the pullups to the pulldowns */
	ai = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, pu1, pla_FindPort(pu1,5*pla_lam,9*pla_lam),
		5*pla_lam, 9*pla_lam, etran1, pla_FindPort(etran1,8*pla_lam,9*pla_lam), 8*pla_lam,
			9*pla_lam, cell);
	if (ai == NOARCINST) return(NONODEPROTO);

	ai = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, pu2,
		pla_FindPort(pu2,5*pla_lam,25*pla_lam+pla_VddWidth), 5*pla_lam, 25*pla_lam+pla_VddWidth,
			etran2, pla_FindPort(etran2,8*pla_lam,25*pla_lam+pla_VddWidth), 8*pla_lam,
				25*pla_lam+pla_VddWidth, cell);
	if (ai == NOARCINST) return(NONODEPROTO);

	/*
	 * Instance diff pins on the pulldowns so that they can be connected
	 * together to ground. The postion of dpin2 is again adjusted with
	 * respect to pla_VddWidth.
	 */
	dpin1 = newnodeinst(pla_dp_proto, 12*pla_lam, 15*pla_lam,
		6*pla_lam, 9*pla_lam, NOTRANS, NOROT, cell);
	dpin2 = newnodeinst(pla_dp_proto, 12*pla_lam, 15*pla_lam,
		(17+5)*pla_lam+pla_VddWidth, (17+5+3)*pla_lam+pla_VddWidth, NOTRANS, NOROT, cell);
	if (dpin1 == NONODEINST || dpin2 == NONODEINST) return(NONODEPROTO);

	/* Connect the diff pins to the respective pulldowns */
	if (newarcinst(pla_da_proto, 3*pla_lam, FIXANG, etran1,
		pla_FindPort(etran1,12*pla_lam,(INTBIG)(7.5*pla_lam)), 12*pla_lam, (INTBIG)(7.5*pla_lam),
			dpin1, pla_dp_proto->firstportproto, (INTBIG)(13.5*pla_lam),(INTBIG)(7.5*pla_lam),
				cell) == NOARCINST) return(NONODEPROTO);
	if (newarcinst(pla_da_proto, 3*pla_lam, FIXANG, etran2, pla_FindPort(etran2,12*pla_lam,
		(INTBIG)((17+11-4.5)*pla_lam)+pla_VddWidth), 12*pla_lam,
			(INTBIG)((17+11-4.5)*pla_lam)+pla_VddWidth, dpin2, pla_dp_proto->firstportproto,
				(INTBIG)(13.5*pla_lam), (INTBIG)((17+11-4.5)*pla_lam)+pla_VddWidth, cell) == NOARCINST)
					return(NONODEPROTO);

	/* Now connect these d pins together */
	if (newarcinst(pla_da_proto, 3*pla_lam, FIXANG, dpin1, pla_dp_proto->firstportproto,
		(INTBIG)(13.5*pla_lam), (INTBIG)(7.5*pla_lam), dpin2, pla_dp_proto->firstportproto,
			(INTBIG)(13.5*pla_lam), (INTBIG)((17+11-4.5)*pla_lam)+pla_VddWidth, cell) == NOARCINST)
				return(NONODEPROTO);

	/*
	 * Connect the inverters together on the poly layer to output
	 * a noninverted signal
	 *
	 * First instance a poly pin to bend the poly line from the bottom
	 * pullup to the top pulldown.  Then draw arcs to pullup and pulldown
	 */
	if ((ppin = newnodeinst(pla_pp_proto, 9*pla_lam, 11*pla_lam,
		14*pla_lam, 16*pla_lam, NOTRANS, NOROT, cell)) == NONODEINST)
			return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, pu1, pla_FindPort(pu1, 6*pla_lam, 15*pla_lam),
		6*pla_lam, 15*pla_lam, ppin, pla_pp_proto->firstportproto, 10*pla_lam, 15*pla_lam,
			cell) == NOARCINST) return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin, pla_pp_proto->firstportproto,
		10*pla_lam, 15*pla_lam, etran2, pla_FindPort(etran2,10*pla_lam, (17+3)*pla_lam+pla_VddWidth),
			10*pla_lam, (17+3)*pla_lam+pla_VddWidth, cell) == NOARCINST)
				return(NONODEPROTO);

	/*
	 * Now pull this connection to the top to form the inverted output
	 * from the Input cell.
	 */
	if ((ppin1 = newnodeinst(pla_pp_proto, 9*pla_lam, 11*pla_lam,
		(17+11+4-2)*pla_lam+pla_VddWidth+pla_GndWidth,
			(17+11+4)  *pla_lam+pla_VddWidth+pla_GndWidth, NOTRANS, NOROT, cell)) == NONODEINST)
				return(NONODEPROTO);
	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin1, pla_pp_proto->firstportproto,
		10*pla_lam, (17+11+4-1)*pla_lam+pla_VddWidth+pla_GndWidth, etran2,
			pla_FindPort(etran2,10*pla_lam, (17+11+2)*pla_lam+pla_VddWidth),
				10*pla_lam, (17+11+2)*pla_lam+pla_VddWidth, cell) == NOARCINST)
					return(NONODEPROTO);

	/*
	 * Pull a poly connection from the butting/buried contact of
	 * pu2 to form the noninverted (doubly inverted) output.
	 */
	if ((ppin2 = newnodeinst(pla_pp_proto, pla_lam, 3*pla_lam,
		(17+11+4-2)*pla_lam+pla_VddWidth+pla_GndWidth,
			(17+11+4)*pla_lam+pla_VddWidth+pla_GndWidth, NOTRANS, NOROT, cell)) == NONODEINST)
				return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin2, pla_pp_proto->firstportproto,
		2*pla_lam, (17+11+4-1)*pla_lam+pla_VddWidth+pla_GndWidth, pu2, pla_FindPort(pu2,2*pla_lam,
			(17+11-3)*pla_lam+pla_VddWidth), 2*pla_lam, (17+11-3)*pla_lam+pla_VddWidth,
				cell) == NOARCINST) return(NONODEPROTO);

	/* Pull a diff arc from the pulldowns to a ground pin at the top */
	if ((md_contact = newnodeinst(pla_md_proto, 12*pla_lam, 16*pla_lam,
		(17+11+4)*pla_lam+pla_VddWidth, (17+11+4)*pla_lam+pla_VddWidth+pla_GndWidth,
			NOTRANS, NOROT, cell)) == NONODEINST) return(NONODEPROTO);
	if (newarcinst(pla_da_proto, 3*pla_lam, FIXANG, md_contact, pla_md_proto->firstportproto,
		(INTBIG)(13.5*pla_lam), (17+11+5)*pla_lam+pla_VddWidth, dpin2, pla_dp_proto->firstportproto,
			(INTBIG)(13.5*pla_lam), (INTBIG)((17+11-4.5)*pla_lam)+pla_VddWidth, cell) == NOARCINST)
				return(NONODEPROTO);

	/*
	 * Instance a metal-ground contact pin between the two poly connections
	 * going into the PLA for running the diff line vertically
	 */
	if ((md_contact1 = newnodeinst(pla_md_proto, 4*pla_lam, 8*pla_lam,
		(17+11+4)*pla_lam+pla_VddWidth, (17+11+4)*pla_lam+pla_VddWidth+pla_GndWidth,
			NOTRANS, NOROT, cell)) == NONODEINST) return(NONODEPROTO);

	/* Connect the two md contacts together in metal */
	if (newarcinst(pla_ma_proto, pla_GndWidth, FIXANG, md_contact, pla_md_proto->firstportproto,
		14*pla_lam, (17+11+4)*pla_lam+pla_VddWidth+pla_halfGnd, md_contact1,
			pla_md_proto->firstportproto, 6*pla_lam, (17+11+4)*pla_lam+pla_VddWidth+pla_halfGnd,
				cell) == NOARCINST) return(NONODEPROTO);

	/* Export some of the pins and ports for external connections */
	if (newportproto(cell, md_contact, pla_md_proto->firstportproto, x_("Gnd")) == NOPORTPROTO ||
		newportproto(cell, ppin1, pla_pp_proto->firstportproto, x_("InvInput")) == NOPORTPROTO ||
		newportproto(cell, ppin2, pla_pp_proto->firstportproto, x_("NonInvInput")) == NOPORTPROTO ||
		newportproto(cell, etran1, pla_FindPort(etran1, 10*pla_lam, pla_lam), x_("Input")) == NOPORTPROTO  ||
		newportproto(cell, md_contact1,pla_md_proto->firstportproto, x_("DiffGnd")) == NOPORTPROTO)
			return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(cell);
	return(cell);
}

NODEPROTO *pla_nmos_OutPullup(void)
{
	NODEPROTO *pu;
	INTBIG sizeX, sizeY;			/* Dimensions of the cell */
	NODEINST *dtran, *dpin, *bc;
	PORTPROTO *dtran_dport, *dtran_pport, *bc_pport, *bc_dport;

	sizeX = 10 * pla_lam;
	sizeY =  9 * pla_lam;

	pu = newnodeproto(x_("nmos_OutPullup"), el_curlib);
	if (pu == NONODEPROTO) return(NONODEPROTO);

	/* Instance a depletion load pullup in pu */
	dtran = newnodeinst(pla_dtran_proto, 3*pla_lam, sizeX-pla_lam,
		3*pla_lam, sizeY, NOTRANS, 900, pu);
	if (dtran == NONODEINST) return(NONODEPROTO);

	/* Extend the Left Poly side by 3 lambdas */
	dtran_dport = pla_FindPort(dtran, 4*pla_lam, 6*pla_lam);
	if (dtran_dport == NOPORTPROTO) return(NONODEPROTO);

	dpin = newnodeinst(pla_dp_proto, 0, 2*pla_lam, 5*pla_lam, 7*pla_lam, NOTRANS, NOROT, pu);
	if (dpin == NONODEINST) return(NONODEPROTO);

	/* Export this pin for external connections */
	if (newportproto(pu, dpin, pla_dp_proto->firstportproto, x_("VddPt")) == NOPORTPROTO)
		return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		pla_lam, 6*pla_lam, dtran, dtran_dport, 4*pla_lam, 6*pla_lam, pu) == NOARCINST)
			return(NONODEPROTO);

	/* Extend Right Poly side by 1 lambda */
	dtran_dport = pla_FindPort(dtran, 8*pla_lam, 6*pla_lam);
	if (dtran_dport == NOPORTPROTO) return(NONODEPROTO);

	dpin = newnodeinst(pla_dp_proto, 8*pla_lam, sizeX, 5*pla_lam, 7*pla_lam, NOTRANS, NOROT, pu);
	if (dpin == NONODEINST) return(NONODEPROTO);

	/* Export this pin for external connections */
	if (newportproto(pu, dpin, pla_dp_proto->firstportproto, x_("EtranPt")) == NOPORTPROTO)
		return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		9*pla_lam, 6*pla_lam, dtran, dtran_dport, 8*pla_lam, 6*pla_lam, pu) == NOARCINST)
			return(NONODEPROTO);

	/* Instance butting/buried contact under the dtran */
	dtran_pport = pla_FindPort(dtran, 6*pla_lam, 4*pla_lam);
	if (dtran_pport == NOPORTPROTO) return(NONODEPROTO);

	if (!pla_buttingcontact) return(NONODEPROTO);

	bc = newnodeinst(pla_bc_proto, 4*pla_lam, 10*pla_lam, 0, 4*pla_lam, NOTRANS, 1800, pu);
	if (bc == NONODEINST) return(NONODEPROTO);

	bc_pport = pla_FindPort(bc, 6*pla_lam, 2*pla_lam);
	bc_dport = pla_FindPort(bc, 9*pla_lam, 2*pla_lam);
	if (bc_dport == NOPORTPROTO || bc_pport == NOPORTPROTO) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		9*pla_lam, 6*pla_lam, bc, bc_dport, 9*pla_lam, 2*pla_lam, pu) == NOARCINST)
			return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, dtran, dtran_pport, 6*pla_lam, 4*pla_lam,
		bc, bc_pport,  6*pla_lam, 2*pla_lam, pu) == NOARCINST) return(NONODEPROTO);

	/* Export bc_dport and bc_pport for external connections */
	if (newportproto(pu, bc, bc_pport, x_("Output2")) == NOPORTPROTO ||
		newportproto(pu, bc, bc_dport, x_("DiffPt")) == NOPORTPROTO) return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(pu);
	return(pu);
}

NODEPROTO *pla_nmos_Output(void)
{
	NODEPROTO *cell, *pu;
	NODEINST  *pu1,  *pu2, *dpin1, *dpin2, *dpin, *etran1, *etran2,
		*ppin1, *ppin2, *md_cont1, *md_cont2;
	INTBIG sizeX,		/* Dimensions of the cell */
		WorkPtY;		/* Y value of a point below which work is done */

	/* define the pullup */
	pu = pla_nmos_OutPullup();
	if (pu == NONODEPROTO) return(NONODEPROTO);

	sizeX = 15 * pla_lam;

	cell = newnodeproto(nmosOutput, el_curlib);
	if (cell == NONODEPROTO) return(NONODEPROTO);

	/* Instance the OutPullup twice in cell */
	pu1 = newnodeinst(pu, (INTBIG)(1.5*pla_lam), (INTBIG)(1.5*pla_lam+pu->highx-pu->lowx),
		(INTBIG)((5+2-0.5)*pla_lam)+pla_VddWidth,
			(INTBIG)((5+2-0.5)*pla_lam+pu->highy-pu->lowy)+pla_VddWidth, TRANS, 1800, cell);
	pu2 = newnodeinst(pu, 5*pla_lam, sizeX, 0, pu->highy-pu->lowy, NOTRANS, NOROT, cell);
	if (pu1 == NONODEINST || pu2 == NONODEINST) return(NONODEPROTO);

	/*
	 * Create two poly pins near the left most edge of the cell
	 * and connect them together to make Output1.
	 */
	ppin1 = newnodeinst(pla_pp_proto, 0, 2*pla_lam, (7+3)*pla_lam+pla_VddWidth,
		(7+5)*pla_lam+pla_VddWidth, NOTRANS, NOROT, cell);
	ppin2 = newnodeinst(pla_pp_proto, 0, 2*pla_lam, 0, 2*pla_lam, NOTRANS, NOROT, cell);
	if (ppin1 == NONODEINST || ppin2 == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin1, pla_pp_proto->firstportproto,
		pla_lam, (7+4)*pla_lam+pla_VddWidth, ppin2, pla_pp_proto->firstportproto,
			pla_lam, pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/* Connect ppin1 to pu1 on poly */
	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin1, pla_pp_proto->firstportproto,
		pla_lam, (7+4)*pla_lam+pla_VddWidth, pu1, pla_FindPort(pu1, 4*pla_lam, (7+4)*pla_lam+
			pla_VddWidth), 4*pla_lam, (7+4)*pla_lam+pla_VddWidth, cell) == NOARCINST)
				return(NONODEPROTO);

	/* Export ppin2 as Output1 Point */
	if (newportproto(cell, ppin2, pla_pp_proto->firstportproto, x_("Output1")) == NOPORTPROTO)
		return(NONODEPROTO);

	/*
	 * Instance a Vdd Connection point in between the two pullups and
	 * connect the pullups to it.  First, find the Vdd points of the
	 * two pullups.
	 */
	md_cont1 = newnodeinst(pla_md_proto, 3*pla_lam, 7*pla_lam, 7*pla_lam, 7*pla_lam+pla_VddWidth,
		NOTRANS, NOROT, cell);
	if (md_cont1 == NONODEINST) return(NONODEPROTO);

	/* Export this contact for external Vdd connection */
	if (newportproto(cell, md_cont1, pla_md_proto->firstportproto, x_("Vdd")) == NOPORTPROTO)
		return(NONODEPROTO);

	/* Extend the Diff wires from the pullups to md_cont1 */
	dpin1 = newnodeinst(pla_dp_proto, 3*pla_lam, 5*pla_lam, (7-1)*pla_lam+pla_VddWidth,
		(7-1+2)*pla_lam+pla_VddWidth, NOTRANS, NOROT, cell);
	dpin2 = newnodeinst(pla_dp_proto, 3*pla_lam, 5*pla_lam,
		5*pla_lam, 7*pla_lam, NOTRANS, NOROT, cell);
	if (dpin1 == NONODEINST || dpin2 == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin1, pla_dp_proto->firstportproto,
		4*pla_lam, 7*pla_lam+pla_VddWidth, pu1, pla_FindPort(pu1, 8*pla_lam, 7*pla_lam+pla_VddWidth),
			8*pla_lam, 7*pla_lam+pla_VddWidth, cell) == NOARCINST)
				return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin2, pla_dp_proto->firstportproto,
		4*pla_lam, 6*pla_lam, pu2, pla_FindPort(pu2, 6*pla_lam, 6*pla_lam),
			6*pla_lam, 6*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/* Connect these dpins to the Vdd Contact (md_cont1) */
	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin1, pla_dp_proto->firstportproto,
		4*pla_lam, 7*pla_lam+pla_VddWidth, md_cont1, pla_md_proto->firstportproto,
			4*pla_lam, 7*pla_lam+pla_halfVdd, cell) == NOARCINST)
				return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin2, pla_dp_proto->firstportproto,
		4*pla_lam, 6*pla_lam, md_cont1, pla_md_proto->firstportproto,
			4*pla_lam, 7*pla_lam+pla_halfVdd, cell) == NOARCINST)
				return(NONODEPROTO);

	/* Now instance two enhancement transistors to complete inverter layouts */
	WorkPtY = 7 * pla_lam + pla_VddWidth + pu->highx - pu->lowx;

	etran1 = newnodeinst(pla_etran_proto, 0, 12*pla_lam, WorkPtY+9*pla_lam,
		WorkPtY+(9+6)*pla_lam, NOTRANS, 900, cell);
	etran2 = newnodeinst(pla_etran_proto, 5*pla_lam, 17*pla_lam, WorkPtY+(4-1)*pla_lam,
		WorkPtY+(4+6-1)*pla_lam, NOTRANS, 900, cell);
	if (etran1 == NONODEINST || etran2 == NONODEINST) return(NONODEPROTO);

	/* Connect these together on diffusion */
	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, etran1,
		pla_FindPort(etran1, 8*pla_lam,WorkPtY+9*pla_lam), 8*pla_lam, WorkPtY+9*pla_lam,
			etran2, pla_FindPort(etran2, 9*pla_lam,WorkPtY+9*pla_lam),
				9*pla_lam, WorkPtY+9*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/*  Instance a diff pin on the left edge of etran1 so that etran1
	 *  can be connected to the butting/buried contact of pu1.
	 */
	dpin1 = newnodeinst(pla_dp_proto, 2*pla_lam, 4*pla_lam,
		WorkPtY+8*pla_lam, WorkPtY+10*pla_lam, NOTRANS, NOROT, cell);
	if (dpin1 == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin1, pla_dp_proto->firstportproto,
		3*pla_lam, WorkPtY+9*pla_lam, etran1, pla_FindPort(etran1, 4*pla_lam,WorkPtY+9*pla_lam),
			4*pla_lam, WorkPtY+9*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin1, pla_dp_proto->firstportproto,
		3*pla_lam, WorkPtY+9*pla_lam, pu1, pla_FindPort(pu1, 3*pla_lam, WorkPtY-2*pla_lam),
			3*pla_lam, WorkPtY-2*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/*
	 * Now we are going to connect etran2 to pu2 on diffusion.
	 * Instance a dpin on the rigth edge of dtran2 and connect them
	 * together.  Then connect dpin to pu2.
	 */
	dpin = newnodeinst(pla_dp_proto, sizeX-2*pla_lam, sizeX,
		WorkPtY+8*pla_lam, WorkPtY+10*pla_lam, NOTRANS, NOROT, cell);
	if (dpin == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		sizeX-pla_lam, WorkPtY+9*pla_lam, etran2,
			pla_FindPort(etran2, sizeX-2*pla_lam, WorkPtY+9*pla_lam),
				sizeX-2*pla_lam, WorkPtY+9*pla_lam, cell) == NOARCINST)
					return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		sizeX-pla_lam, WorkPtY+9*pla_lam, pu2, pla_FindPort(pu2, sizeX-pla_lam, 6*pla_lam),
			sizeX-pla_lam, 6*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/*
	 * It is time to make a Ground contact.  Instance a ground
	 * contact above and in between the two etrans and connect it
	 * to the ground side of the etrans on diff.
	 */
	WorkPtY = WorkPtY + 12*pla_lam;

	md_cont1 = newnodeinst(pla_md_proto, 8*pla_lam, 12*pla_lam, WorkPtY+2*pla_lam,
		WorkPtY+2*pla_lam+pla_GndWidth, NOTRANS, NOROT, cell);
	if (md_cont1 == NONODEINST) return(NONODEPROTO);

	/*  Export this as the external Ground contact */
	if (newportproto(cell, md_cont1, pla_md_proto->firstportproto, x_("Gnd")) == NOPORTPROTO)
		return(NONODEPROTO);

	/* Now connect etran1 to Ground contact */
	if (newarcinst(pla_da_proto, 2*pla_lam, FIXANG, etran1,
		pla_FindPort(etran1, 8*pla_lam, WorkPtY+3*pla_lam), 8*pla_lam, WorkPtY+3*pla_lam,
			md_cont1, pla_md_proto->firstportproto, 9*pla_lam, WorkPtY+3*pla_lam, cell) == NOARCINST)
				return(NONODEPROTO);

	/*
	 * Let us snake the poly outputs from etrans to top where it will
	 * be available for the PLA
	 */
	ppin1 = newnodeinst(pla_pp_proto, 10*pla_lam, 12*pla_lam, WorkPtY-pla_lam, WorkPtY+pla_lam,
		NOTRANS, NOROT, cell);
	ppin2 = newnodeinst(pla_pp_proto, 13*pla_lam, 15*pla_lam, WorkPtY-pla_lam, WorkPtY+pla_lam,
		NOTRANS, NOROT, cell);
	if (ppin1 == NONODEINST || ppin2 == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin1, pla_pp_proto->firstportproto,
		11*pla_lam, WorkPtY, ppin2, pla_pp_proto->firstportproto, 14*pla_lam, WorkPtY,
			cell) == NOARCINST) return(NONODEPROTO);

	/* Connect ppin1 to etran2 */
	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin1, pla_pp_proto->firstportproto,
		11*pla_lam, WorkPtY, etran2, pla_FindPort(etran2, 11*pla_lam, WorkPtY-pla_lam),
			11*pla_lam, WorkPtY-pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/*
	 * Instance two metal-poly contacts to allow tapping signals
	 * from within the PLA.  Connect these to the poly extensions
	 * made above.
	 */
	WorkPtY = WorkPtY + 2 * pla_lam + pla_GndWidth;

	md_cont1 = newnodeinst(pla_mp_proto, 3*pla_lam, 7*pla_lam, WorkPtY+4*pla_lam, WorkPtY+8*pla_lam,
		NOTRANS, NOROT, cell);
	md_cont2 = newnodeinst(pla_mp_proto, 11*pla_lam, 15*pla_lam, WorkPtY+4*pla_lam,
		WorkPtY+8*pla_lam, NOTRANS, NOROT, cell);
	if (md_cont1 == NONODEINST || md_cont2 == NONODEINST) return(NONODEPROTO);

	/* Export these for external connections */
	if (newportproto(cell, md_cont1, pla_mp_proto->firstportproto, x_("Input1")) == NOPORTPROTO ||
		newportproto(cell, md_cont2, pla_mp_proto->firstportproto, x_("Input2")) == NOPORTPROTO)
			return(NONODEPROTO);

	/* Connect these to the etrans */
	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, ppin2, pla_pp_proto->firstportproto,
		14*pla_lam, WorkPtY-(pla_GndWidth+2*pla_lam), md_cont2, pla_mp_proto->firstportproto,
			14*pla_lam, WorkPtY+5*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	if (newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, etran1,
		pla_FindPort(etran1,6*pla_lam,WorkPtY-pla_GndWidth+3*pla_lam), 6*pla_lam,
			WorkPtY-pla_GndWidth+3*pla_lam, md_cont1, pla_mp_proto->firstportproto,
				6*pla_lam, WorkPtY+5*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	/* Export the Output point of pu2 as Output2 */
	if (newportproto(cell, pu2, pla_FindPort(pu2, 11*pla_lam, pla_lam), x_("Output2")) == NOPORTPROTO)
		return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(cell);
	return(cell);
}

NODEPROTO *pla_nmos_Program(void)
{
	NODEPROTO *cell;
	NODEINST *etran, *md_contact, *dpin;

	cell = newnodeproto(nmosProgram, el_curlib);
	if (cell == NONODEPROTO) return(NONODEPROTO);

	if ((etran = newnodeinst(pla_etran_proto, 2*pla_lam, 10*pla_lam,
		pla_lam, 7*pla_lam, NOTRANS, 900, cell)) == NONODEINST) return(NONODEPROTO);

	if ((md_contact = newnodeinst(pla_md_proto, 8*pla_lam, 12*pla_lam,
		2*pla_lam, 6*pla_lam, NOTRANS, NOROT, cell)) == NONODEINST) return(NONODEPROTO);

	if ((dpin = newnodeinst(pla_dp_proto, 0, 4*pla_lam, 2*pla_lam, 6*pla_lam,
		NOTRANS, NOROT, cell)) == NONODEINST) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 4*pla_lam, FIXANG, dpin, pla_dp_proto->firstportproto,
		2*pla_lam, 4*pla_lam, etran, pla_FindPort(etran, 4*pla_lam, 4*pla_lam),
			4*pla_lam, 4*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	if (newarcinst(pla_da_proto, 4*pla_lam, FIXANG, etran, pla_FindPort(etran, 8*pla_lam, 4*pla_lam),
		8*pla_lam, 4*pla_lam, md_contact, pla_md_proto->firstportproto,
			10*pla_lam, 4*pla_lam, cell) == NOARCINST) return(NONODEPROTO);

	if (newportproto(cell, dpin, pla_dp_proto->firstportproto, x_("DiffPt")) == NOPORTPROTO ||
		newportproto(cell, etran, pla_FindPort(etran,6*pla_lam,7*pla_lam), x_("Up")) == NOPORTPROTO ||
		newportproto(cell, etran, pla_FindPort(etran,6*pla_lam,pla_lam), x_("Down")) == NOPORTPROTO ||
		newportproto(cell, md_contact, pla_md_proto->firstportproto, x_("MdPt")) == NOPORTPROTO)
			return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(cell);
	return(cell);
}

NODEPROTO *pla_nmos_Pullup(void)
{
	NODEPROTO *pu;
	NODEINST *dtran, *bc, *vdd_line, *md;
	ARCINST *pa, *da;
	PORTPROTO *dtran_pport, *dtran_dport, *bc_pport, *bc_dport, *vpname, *dpname;
	INTBIG sizeX;

	if (pla_buttingcontact) sizeX = 10*pla_lam + pla_VddWidth; else
		sizeX = (10+2)*pla_lam + pla_VddWidth;
	pu = newnodeproto(nmosPullup, el_curlib);
	if (pu == NONODEPROTO) return(NONODEPROTO);

	dtran = newnodeinst(pla_dtran_proto, pla_lam+pla_VddWidth, 7*pla_lam+pla_VddWidth,
		-pla_lam,  7*pla_lam, NOTRANS, 900, pu);
	if (dtran == NONODEINST) return(NONODEPROTO);
	dtran_dport = pla_FindPort(dtran, 7*pla_lam+pla_VddWidth, 3*pla_lam);
	dtran_pport = pla_FindPort(dtran, 5*pla_lam+pla_VddWidth, 1*pla_lam);
	if (dtran_pport == NOPORTPROTO || dtran_dport == NOPORTPROTO)
		return(NONODEPROTO);
	if (pla_buttingcontact)
	{
		bc = newnodeinst(pla_bc_proto, 4*pla_lam+pla_VddWidth, sizeX,
			1*pla_lam,  5*pla_lam, NOTRANS, 1800, pu);
		if (bc == NONODEINST) return(NONODEPROTO);
		bc_dport = pla_FindPort(bc, 8*pla_lam+pla_VddWidth, 3*pla_lam);
		bc_pport = pla_FindPort(bc, 5*pla_lam+pla_VddWidth, 3*pla_lam);
		if (bc_pport == NOPORTPROTO || bc_dport == NOPORTPROTO) return(NONODEPROTO);
		pa = newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, dtran, dtran_pport,5*pla_lam+pla_VddWidth,
			pla_lam, bc, bc_pport, 5*pla_lam+pla_VddWidth, 3*pla_lam, pu);
		da = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, dtran, dtran_dport,
			7*pla_lam+pla_VddWidth,3*pla_lam, bc, bc_dport, 8*pla_lam+pla_VddWidth, 3*pla_lam, pu);
		if (pa == NOARCINST || da == NOARCINST) return(NONODEPROTO);
	} else
	{
		bc = newnodeinst(pla_bc_proto, 4*pla_lam+pla_VddWidth, 8*pla_lam+pla_VddWidth,
			0, 6*pla_lam, NOTRANS, 1800, pu);
		if (bc == NONODEINST) return(NONODEPROTO);
		bc_dport = pla_FindPort(bc, 7*pla_lam+pla_VddWidth, 3*pla_lam);
		bc_pport = pla_FindPort(bc, 5*pla_lam+pla_VddWidth, pla_lam);
		if (bc_pport == NOPORTPROTO || bc_dport == NOPORTPROTO) return(NONODEPROTO);
		pa = newarcinst(pla_pa_proto, 2*pla_lam, FIXANG, dtran, dtran_pport,5*pla_lam+pla_VddWidth,
			pla_lam, bc, bc_pport, 5*pla_lam+pla_VddWidth, pla_lam,pu);
		da = newarcinst(pla_da_proto,  2*pla_lam, FIXANG, dtran, dtran_dport,
			7*pla_lam+pla_VddWidth,3*pla_lam, bc, bc_dport, 7*pla_lam+pla_VddWidth, 3*pla_lam, pu);
		if (pa == NOARCINST || da == NOARCINST) return(NONODEPROTO);

		/*
		 * Now add an extension on the right side of the pullup for
		 * connections into the pla only if buried contacts were used.
		 */
		md = newnodeinst(pla_md_proto, 8*pla_lam+pla_VddWidth, sizeX,
			pla_lam, 5*pla_lam, NOTRANS, NOROT, pu);
		if (md == NONODEINST) return(NONODEPROTO);

		if (newarcinst(pla_da_proto, 4*pla_lam, FIXANG, bc, bc_dport, 7*pla_lam+pla_VddWidth,
			3*pla_lam, md, pla_md_proto->firstportproto, 10*pla_lam+pla_VddWidth, 3*pla_lam,
				pu) == NOARCINST)
					return(NONODEPROTO);
	}

	/* Now place the Vdd connection on the left */
	vdd_line = newnodeinst(pla_md_proto, 0, pla_VddWidth, -pla_lam, 7*pla_lam, NOTRANS, NOROT, pu);
	if (vdd_line == NONODEINST) return(NONODEPROTO);

	/* Now connect the pullup to the vdd_line */
	dtran_dport = pla_FindPort(dtran, pla_lam+pla_VddWidth, 3*pla_lam);
	if (dtran_dport == NOPORTPROTO) return(NONODEPROTO);

	da = newarcinst(pla_da_proto, 2*pla_lam, FIXANG, vdd_line,pla_md_proto->firstportproto,
		pla_halfVdd, 3*pla_lam, dtran, dtran_dport, pla_lam+pla_VddWidth, 3*pla_lam, pu);
	if (da == NOARCINST) return(NONODEPROTO);

	/*
	 * give names to vdd line and diffusion side of the butting/buried
	 * contact so that they are accessible to outside world
	 */
	vpname = newportproto(pu, vdd_line, pla_md_proto->firstportproto, x_("Vdd"));
	dpname = pla_buttingcontact ? newportproto(pu, bc, bc_dport, x_("Pullup")) :
		newportproto(pu, md, pla_md_proto->firstportproto, x_("Pullup"));
	if (vpname == NOPORTPROTO || dpname == NOPORTPROTO)
		return(NONODEPROTO);

	/* ensure that the cell is the proper size */
	(*el_curconstraint->solve)(pu);
	return(pu);
}

#endif /* PLATOOL - at top */
