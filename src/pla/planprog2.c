/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: planprog2.c
 * Programmable Logic Array Generator, part II
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

/*
 * Module to generate PLA layouts.  Input in a truth table format
 * is read from a file.  The output is in the form of cell prototypes
 * defined in the current library.
 */

#include "config.h"
#if PLATOOL

#include "global.h"
#include "pla.h"
#include "planmos.h"

/* prototypes for local routines */
static void    pla_CHfromPuAND(INTBIG, CELLIST*, NODEPROTO*);
static void    pla_CVfromInput(INTBIG, INTBIG, CELLIST*, NODEPROTO*);
static void    pla_CVfromOutput(CELLIST*, CELLIST*, NODEPROTO*);
static BOOLEAN pla_ConnectFirstCell(INTBIG, CELLIST*, NODEPROTO*);
static void    pla_ConnectOnMetal(INTBIG, VDDGND*, CHAR*, NODEPROTO*);
static void    pla_ConnectThruCells(INTBIG, VDDGND*, CHAR*, NODEPROTO*);
static void    pla_DrawArcs(INTBIG, PROGTRANS*, CELLIST*, INTBIG, NODEPROTO*);
static void    pla_InstTurnPin(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, VDDGND*, VDDGND*, CHAR*, NODEPROTO*);

/* pla_AssignPort:  Find the proper port and note down its coordinates. */
PORT *pla_AssignPort(NODEINST *inst, INTBIG x, INTBIG y)
{
	PORT *prt;

	prt = (PORT *)emalloc(sizeof(PORT), pla_tool->cluster);
	if (prt == NOPORT)
	{
		ttyputnomemory();
		return(NOPORT);
	}
	if (prt == NOPORT) return(NOPORT);

	prt->port = pla_FindPort(inst, x, y);
	prt->x = x;
	prt->y = y;
	return(prt);
}

/*
 * Connect the programming transistors Horizontally from the Pullups
 * on the AND plane.
 */
void pla_CHfromPuAND(INTBIG orRight, CELLIST *PullupAND, NODEPROTO *pla)
{
	PROGTRANS *connect, *ftrans, *ttrans, cgnd;
	CELLIST connectcell, *currow, *prevrow;
	VDDGND *carg, *parg, *corg, *porg;
	PORT *fport, *tport;
	INTBIG gndY;

	prevrow = NOCELLIST;
	carg = parg = corg = porg = NOVDDGND;

	for(currow = PullupAND; currow != NOCELLIST; currow = currow->nextcell)
	{
		if (pla_verbose)
			ttyputmsg(M_("connecting right from pullup# %ld"), currow->findex);

		if (pla_ConnectFirstCell(PULLUPSIDE, currow, pla))
			ttyputerr(M_("CHfromPuAND: cannot connect to first trans"));

		/* Find the end instance on the AND plane */
		connect = currow->lcoltrans;
		while(connect->trans->proto->primindex != 0 ||
			namesame(connect->trans->proto->protoname, nmosConnect) != 0)
				connect = connect->rprogtrans;
		if (connect == NOPROGTRANS)
			ttyputerr(M_("CHfromPuAND: cannot find connect cell"));
				else pla_DrawArcs(PULLUPSIDE, connect, currow, METAL, pla);

		carg = pla_MkVddGnd(ANDRIGHTGND, &parg);
		carg->inst = connect->trans;
		carg->port = connect->diffport;

		/* Make the connect transistor look like a CELLIST structure,
		 * because that is what pla_DrawArcs expects.
		 */
		connectcell.findex = currow->findex;
		connectcell.lcoltrans = connect;

		/* Connect on ground only if there are odd number of pterms and this
		 * is an odd row OR there are even number of pterms and this is an
		 * even row.
		 */
		if ((pla_pterms%2  &&   (currow->findex)%2)   ||
			(!(pla_pterms%2) && !((currow->findex)%2)))
		{
			/* Instance ground pin on the far right side */
			gndY = connect->diffport->y - 2*pla_lam;
			cgnd.trans = newnodeinst(pla_md_proto, orRight, orRight+pla_GndWidth,
				gndY, gndY+4*pla_lam, NOTRANS, NOROT, pla);
			if (cgnd.trans == NONODEINST)
				ttyputerr(M_("CHfromPuAND: cannot instance ground pin"));

			cgnd.diffport = pla_AssignPort(cgnd.trans, orRight+pla_halfGnd, gndY+2*pla_lam);
			cgnd.diffport->port = pla_md_proto->firstportproto;

			corg = pla_MkVddGnd(ORRIGHTGND, &porg);
			corg->inst = cgnd.trans;
			corg->port = cgnd.diffport;

			pla_DrawArcs(PULLUPSIDE, &cgnd, &connectcell, POLY, pla);

			/* Now connect the transistors on the prev row on diffusion */
			ftrans = connect;
			fport  = ftrans->diffport;
			if (prevrow != NOCELLIST)
			{
				connect = prevrow->lcoltrans;
				while(connect->trans->proto->primindex != 0 ||
					namesame(connect->trans->proto->protoname, nmosConnect) != 0)
						connect = connect->rprogtrans;
				if (connect == NOPROGTRANS)
					ttyputerr(M_("Cannot find connect cell on prev row"));
				if (connect != NOPROGTRANS)
					for(ttrans = connect->rprogtrans; ttrans != NOPROGTRANS; ttrans = ttrans->rprogtrans)
				{
					tport = ttrans->diffport;
					if (newarcinst(pla_da_proto, 4*pla_lam, pla_userbits, ftrans->trans,
						fport->port, fport->x, fport->y, ttrans->trans, tport->port, tport->x,
							tport->y, pla) == NOARCINST)
								ttyputerr(M_("Cannot connect diff for prev row transistors"));
					ftrans = ttrans;
					fport  = tport;
				}
				tport = cgnd.diffport;
				if (newarcinst(pla_da_proto, 4*pla_lam, pla_userbits,
					ftrans->trans, fport->port, fport->x, fport->y,
						cgnd.trans, tport->port, tport->x, tport->y, pla) == NOARCINST)
							ttyputerr(M_("cannot connect to gnd for prev row transistors"));
			}
		} else pla_DrawArcs(PULLUPSIDE, &cgnd, &connectcell, POLY, pla);
		prevrow = currow;
	}
}

/*
 * Connect Vertically from the Input cells.
 * Each Programming transistor has four ports.
 * Each Input cell has two  ports for connecting it into the PLA.
 * inputcell->port1 connects the left arm of the cell to the PLA and
 * inputcell->port2 connects the right arm.  These two will connect
 * into the poly ports of the programming transistors on the corresponding
 * columns.  There is also a diff arc to be drawn from the input cell to
 * the diff port of the programming transistor closest to it.  Note that the
 * left arm and the right arm cannot contain a programming transistor
 * at the same distance from the input cell.
 */
void pla_CVfromInput(INTBIG inY, INTBIG andY, CELLIST *inptr, NODEPROTO *pla)
{
	PROGTRANS trans;
	VDDGND *catg, *patg;

	catg = patg = NOVDDGND;

	inY = inY - (pla_VddWidth - 4*pla_lam); /* Compensate for Extra Vdd Width */
	while(inptr != NOCELLIST)
	{
		if (inptr->lcoltrans == NOPROGTRANS && inptr->rcoltrans == NOPROGTRANS)
		{
			ttyputmsg(_("*** Warning. Input %ld is never used at all."), inptr->findex);
			inptr = inptr->nextcell;
			continue;
		}
		if (inptr->lcoltrans == NOPROGTRANS)
			ttyputmsg(_("*** Warning. Input %ld is never used in negated form."), inptr->findex);
		if (inptr->rcoltrans == NOPROGTRANS)
			ttyputmsg(_("*** Warning. Input %ld is never used in unnegated form."), inptr->findex);

		if (pla_verbose)
			ttyputmsg(M_("connecting up from input# %ld"), inptr->findex);

		/* Place a contact to ground at the top of the PLA right above the
		 * current input cell.
		 */
		trans.trans = newnodeinst(pla_md_proto, (inptr->gport->x)-2*pla_lam,
			(inptr->gport->x)+2*pla_lam, inY+andY+3*pla_lam, inY+andY+3*pla_lam+pla_GndWidth,
				NOTRANS, NOROT, pla);
		if (trans.trans == NONODEINST)
			ttyputerr(M_("CVfromInput: cannot instance ground pin"));

		trans.diffport = pla_AssignPort(trans.trans, inptr->gport->x,
			inY+andY+3*pla_lam+pla_halfGnd);

		/* Reset the port because, pla_AssignPort would have failed to find
		 * a port name for this unexported gndpin.
		 */
		trans.diffport->port = pla_md_proto->firstportproto;

		catg = pla_MkVddGnd(ANDTOPGND, &patg);
		catg->inst = trans.trans;
		catg->port = trans.diffport;

		/* We also need to connect the input cell to the first transistor */
		if (pla_ConnectFirstCell(INPUTSIDE, inptr, pla))
			ttyputerr(M_("CVfromInput: cannot connect to first transistors"));

		pla_DrawArcs(INPUTSIDE, &trans, inptr, POLY, pla);

		inptr = inptr->nextcell;
	}
}

/*
 * Connect the programming transistors vertically from the output cells
 * Each output cell corresponds to two columns of output.  Connect
 * them one at a time to the OR plane and the corresponding Pullup
 * cell.
 */
void pla_CVfromOutput(CELLIST *outptr, CELLIST *puptr, NODEPROTO *pla)
{
	PROGTRANS *endtrans = NOPROGTRANS;
	PORT *fport, *tport;
	INTBIG out, i;

	while(outptr != NOCELLIST)
	{
		if (pla_ConnectFirstCell(OUTPUTSIDE, outptr, pla))
			ttyputerr(M_("CVfromOutput: cannot connect to first transistors"));

		for(i = 1; i >= 0; i--)
		{
			out = 2 * outptr->findex - i;
			switch(i)
			{
				case 1:
					if (outptr->lcoltrans == NOPROGTRANS)
					ttyputerr(_("*** Warning: Output %ld is never set"), out);
					break;
				case 0:
					if (outptr->rcoltrans == NOPROGTRANS && out <= pla_outputs)
					ttyputerr(_("*** Warning: Output %ld is never set"), out+i);
					break;
			}
		}
		if (pla_verbose)
			ttyputmsg(M_("connecting up from output cell# %ld"), outptr->findex);

		pla_DrawArcs(OUTPUTSIDE, endtrans, outptr, METAL, pla);

		/* Now connect to the pullup on the appropriate column */
		if (outptr->lcoltrans != NOPROGTRANS)
		{
			for(endtrans = outptr->lcoltrans; endtrans->uprogtrans != NOPROGTRANS;
				endtrans = endtrans->uprogtrans)
					;
			fport = endtrans->metalport;
			tport = puptr->lport;
			if (newarcinst(pla_ma_proto, 3*pla_lam, pla_userbits,
				endtrans->trans, fport->port, fport->x, fport->y,
					puptr->cellinst, tport->port, tport->x, tport->y, pla) == NOARCINST)
						ttyputerr(M_("Cannot connect to pullup from L column"));
			puptr = puptr->nextcell;
		}

		if (outptr->rcoltrans != NOPROGTRANS)
		{
			for(endtrans = outptr->rcoltrans; endtrans->uprogtrans != NOPROGTRANS;
				endtrans = endtrans->uprogtrans)
					;
			fport = endtrans->metalport;
			tport = puptr->lport;
			if (newarcinst(pla_ma_proto, 3*pla_lam, pla_userbits,
				endtrans->trans, fport->port, fport->x, fport->y,
					puptr->cellinst, tport->port, tport->x, tport->y, pla) == NOARCINST)
						ttyputerr(M_("Cannot connect to pullup from R column"));
			puptr = puptr->nextcell;
		}

		outptr = outptr->nextcell;
	}
}

/*
 * Connect the given cell to the first programming transistor in that
 * column or row.
 */
BOOLEAN pla_ConnectFirstCell(INTBIG from, CELLIST *cell, NODEPROTO *pla)
{
	PROGTRANS *trans;
	PORT *fport, *tport;
	ARCPROTO *arctype;
	INTBIG columns, width;

	switch(from)
	{
		case INPUTSIDE:
			columns = 2;
			width = 2*pla_lam;
			arctype = pla_pa_proto;
			break;
		case OUTPUTSIDE:
			columns = ((cell->findex)*2 <= pla_outputs) ? 2 : 1;
			width = 3*pla_lam;
			arctype = pla_ma_proto;
			break;
		case PULLUPSIDE:
			columns = 1;
			width = 3*pla_lam;
			arctype = pla_ma_proto;
			break;
	}
	while(columns > 0)
	{
		trans = (columns == 1) ? cell->lcoltrans : cell->rcoltrans;
		if (trans == NOPROGTRANS) break;
		switch(from)
		{
			case INPUTSIDE:
				fport = cell->gport;	/* Connect the ground points first */
				tport = trans->diffport;
				if (newarcinst(pla_da_proto, 4*pla_lam, pla_userbits,
					cell->cellinst, fport->port, fport->x, fport->y,
						trans->trans, tport->port, tport->x, tport->y, pla) == NOARCINST)
							return(TRUE);
				tport = trans->polyblport;
				break;

			case PULLUPSIDE:
			case OUTPUTSIDE:
				tport = trans->metalport;
				break;
		}
		fport = (columns == 1) ? cell->lport : cell->rport;
		if (newarcinst(arctype, width, pla_userbits,
			cell->cellinst, fport->port, fport->x, fport->y,
				trans->trans, tport->port, tport->x, tport->y, pla) == NOARCINST)
					return(TRUE);

		columns--;
	}
	return(FALSE);
}

/* Draw a metal arc between two programming transistors. */
void pla_ConnectOnMetal(INTBIG which, VDDGND *ptr, CHAR *mesg, NODEPROTO *pla)
{
	VDDGND *prevptr;
	PORT *fport, *tport;
	INTBIG width;

	prevptr = ptr;
	ptr = ptr->nextinst;
	fport = prevptr->port;
	tport = ptr->port;

	width = ((which == GND) ? pla_GndWidth : pla_VddWidth);
	if (newarcinst(pla_ma_proto, width, pla_userbits, prevptr->inst, fport->port, fport->x,
		fport->y, ptr->inst, tport->port, tport->x, tport->y, pla) == NOARCINST)
			ttyputerr(x_("%s"), mesg);
}

/* Connect the programming transistors on both the planes */
void pla_ConnectPlanes(INTBIG inY, INTBIG andY, INTBIG orRight, CELLIST *Inputs,
	CELLIST *Outputs, CELLIST *PullupAND, CELLIST *PullupOR, NODEPROTO *pla)
{
	pla_CVfromInput(inY, andY, Inputs, pla);
	pla_CVfromOutput(Outputs, PullupOR, pla);
	pla_CHfromPuAND(orRight, PullupAND, pla);
}

/* Connect the Vdd/Gnd points in a string of cells */
void pla_ConnectThruCells(INTBIG which, VDDGND *ptr, CHAR *mesg, NODEPROTO *pla)
{
	while(ptr->nextinst != NOVDDGND)
	{
		pla_ConnectOnMetal(which, ptr, mesg, pla);
		ptr = ptr->nextinst;
	}
}

/*
 * Draw appropriate arcs between the start cell and the end transistor
 * connecting through the intervening programming transistors.
 */
void pla_DrawArcs(INTBIG from, PROGTRANS *endtrans, CELLIST *startcell,
	INTBIG layer, NODEPROTO *pla)
{
	PROGTRANS *ftrans, *ttrans;
	PORT *fport, *tport;
	ARCPROTO *arctype;
	INTBIG columns, width;

	switch(from)
	{
		case INPUTSIDE:
		case OUTPUTSIDE:
			columns = 2;
			break;
		case PULLUPSIDE:
			columns = 1;
			break;
	}
	while(columns > 0)
	{
		ftrans = (columns == 1) ? startcell->lcoltrans : startcell->rcoltrans;
		columns--;
		while(ftrans != NOPROGTRANS)
		{
			if ((from == PULLUPSIDE && ftrans->rprogtrans == NOPROGTRANS) ||
				((from == INPUTSIDE || from == OUTPUTSIDE) &&
					ftrans->uprogtrans == NOPROGTRANS))  /* We are done */
						break;
			switch(layer)
			{
				case METAL:
					arctype = pla_ma_proto;
					width   = 3*pla_lam;
					break;
				case POLY:
					arctype = pla_pa_proto;
					width   = 2*pla_lam;
					break;
				case DIFF:
					arctype = pla_da_proto;
					width   = 4*pla_lam;
					break;
			}
			switch(from)
			{
				case INPUTSIDE:
					/* Connect on Diff (Gnd) before connecting the transistors */
					ttrans = ftrans->uprogtrans;
					fport  = ftrans->diffport;
					tport  = ttrans->diffport;
					if (newarcinst(pla_da_proto, 4*pla_lam, pla_userbits,
						ftrans->trans, fport->port, fport->x, fport->y,
						ttrans->trans, tport->port, tport->x, tport->y, pla) == NOARCINST)
					{
						ttyputerr(M_("DrawArcs: no diff ground arc on input side."));
						ttyputerr(M_("  frow=%ld, fcol=%ld, trow=%ld, tcol=%ld"),
							ftrans->row, ftrans->column, ttrans->row, ttrans->column);
					}
					fport  = ftrans->polytrport;
					tport  = ttrans->polyblport;
					break;
				case OUTPUTSIDE:
					ttrans = ftrans->uprogtrans;
					fport  = ftrans->metalport;
					tport  = ttrans->metalport;
					break;
				case PULLUPSIDE:
					ttrans = ftrans->rprogtrans;
					switch(layer)
					{
						case METAL:
							fport = ftrans->metalport;
							tport = ttrans->metalport;
							break;
						case POLY:
							/* Connect on Diff (Gnd) before connecting the
							 * transistors.  If there are odd number of pterms
							 * then, draw the horizontal diffusion ground runs
							 * only for odd numbered rows, because for these
							 * rows the programming transistors on the OR plane
							 * would be turned clockwise by 90 and their
							 * diffports would be in line with the diff ports of
							 * connect cells on the same rows.  For the same
							 * reason, if there are even number of pterms then
							 * connect only for even numbered rows.
							 */
							if ((pla_pterms%2 && (startcell->findex)%2) ||
								(!pla_pterms%2 && !((startcell->findex)%2)))
							{
								ttrans = ftrans->rprogtrans;
								fport  = ftrans->diffport;
								tport  = ttrans->diffport;
								if (newarcinst(pla_da_proto, 4*pla_lam, pla_userbits, ftrans->trans,
									fport->port, fport->x, fport->y, ttrans->trans,
										tport->port, tport->x, tport->y, pla) == NOARCINST)
								{
									ttyputerr(M_("no diff ground arc on pullup"));
									ttyputerr(M_(" frow=%ld, fcol=%ld, trow=%ld, tcol=%ld"),
										ftrans->row, ftrans->column, ttrans->row, ttrans->column);
								}
							}
							fport = ftrans->polytrport;
							tport = ttrans->polyblport;
							break;
					}
					break;
			}
			if (newarcinst(arctype, width, pla_userbits,
				ftrans->trans, fport->port, fport->x, fport->y,
				ttrans->trans, tport->port, tport->x, tport->y, pla)
					== NOARCINST)
			{
				ttyputerr(M_("DrawArcs: no connecting arc."));
				ttyputerr(M_("frow=%ld, fcol=%ld, trow=%ld, tcol=%ld, layer=%s"),
					ftrans->row, ftrans->column, ttrans->row, ttrans->column,
						(layer == METAL) ? M_("Metal") : ((layer == POLY) ? M_("Poly") : M_("Diff")));
			}

			/* Break out of the loop if we are connecting from the Pullup side
			 * and we have reached the connect cell
			 */
			if (from == PULLUPSIDE && layer == METAL && ttrans->trans->proto->primindex == 0 &&
				namesame(ttrans->trans->proto->protoname, nmosConnect) == 0) break;
			ftrans = ttrans;
		}

		/* Now connect the last transistor to the endinst on diffusion */
		if (ftrans != NOPROGTRANS &&(from == INPUTSIDE  ||
			(from == PULLUPSIDE && layer == POLY &&
				((pla_pterms%2 &&   (startcell->findex)%2)     ||
					(!pla_pterms%2 && !((startcell->findex)%2))))))
		{
			/* ftrans will not be NIL if went through the while loop above
			 * at least once, because, we break out of the loop when ttrans
			 * becomes NIL.  If we are connecting from the Pullup side,
			 * then create this arc only if we are connecting on poly;
			 * even then only when pterms is odd and current row is odd or
			 * pterms is even and the current row is even too. If we were
			 * connecting on Metal from the Pullups, then the endinst would be
			 * a Connet cell and not a diff-ground pin/pullup.  If we are
			 * coming up from the output side then the end instances are
			 * pullups rather than ground pins.
			 */
			fport = ftrans->diffport;
			if (newarcinst(pla_da_proto, 4*pla_lam, pla_userbits,
				ftrans->trans, fport->port, fport->x, fport->y, endtrans->trans,
					endtrans->diffport->port, endtrans->diffport->x, endtrans->diffport->y, pla)
						== NOARCINST)
							ttyputerr(M_("DrawArcs: cannot connect to ground pin"));
		}
	}
}

/* Instance a turning pin while laying out the Vdd/Gnd busses */
void pla_InstTurnPin(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, INTBIG px, INTBIG py,
	VDDGND *tp, VDDGND *next, CHAR *mesg, NODEPROTO *pla)
{
	tp->inst = newnodeinst(pla_bp_proto, lx, hx, ly, hy, NOTRANS, NOROT, pla);
	if (tp->inst == NONODEINST) ttyputerr(x_("%s"), mesg);

	tp->port = pla_AssignPort(tp->inst, px, py);
	tp->port->port = pla_bp_proto->firstportproto;
	tp->nextinst = next;
}

/* Create an element in a list of Vdd/Gnd points */
VDDGND *pla_MkVddGnd(INTBIG which, VDDGND **prevptr)
{
	VDDGND *ptr;

	ptr = (VDDGND *)emalloc(sizeof(VDDGND), pla_tool->cluster);
	if (ptr == NOVDDGND) ttyputnomemory(); else
		if (ptr == NOVDDGND) return(ptr);

	ptr->nextinst = NOVDDGND;
	switch (which)
	{
		case INCELLGND:
			if (pla_icg == NOVDDGND)
			pla_icg = ptr;
			break;
		case INCELLVDD:
			if (pla_icv == NOVDDGND)
			pla_icv = ptr;
			break;
		case OUTCELLGND:
			if (pla_ocg == NOVDDGND)
			pla_ocg = ptr;
			break;
		case OUTCELLVDD:
			if (pla_ocv == NOVDDGND)
			pla_ocv = ptr;
			break;
		case ANDLEFTVDD:
			if (pla_alv == NOVDDGND)
			pla_alv = ptr;
			break;
		case ANDTOPGND:
			if (pla_atg == NOVDDGND)
			pla_atg = ptr;
			break;
		case ANDRIGHTGND:
			if (pla_arg == NOVDDGND)
			pla_arg = ptr;
			break;
		case ORTOPVDD:
			if (pla_otv == NOVDDGND)
			pla_otv = ptr;
			break;
		case ORRIGHTGND:
			if (pla_org == NOVDDGND)
			pla_org = ptr;
			break;
		default:
			return(ptr);
	}
	if (*prevptr != NOVDDGND) (*prevptr)->nextinst = ptr;
	*prevptr = ptr;
	return(ptr);
}

/*
 * Run the Vdd and Gnd busses
 * At this point we have 9 lists, 4 of them with Vdd points and
 * five with Gnd points.
 *  icg and icv start with the first input cell on the left.
 *  ocg and ocv start with the first output cell on the left.
 *  alv starts at the bottom and ends at the top.
 *  otv starts at the left and ends at the right.
 *  arg and org start at the bottom and end at the top.
 *
 *  Let us first connect the Vdd Bus.  Start with otv and connect
 *  to the end on the right.  Start again with otv but connect to
 *  the left after instancing a pin over the Pullups on AND.
 *  Connect through the Pullups on AND, through the Input cells
 *  and finally through the Outptut cells.
 */
void pla_RunVddGnd(INTBIG puX, INTBIG andX, NODEPROTO *pla)
{
	VDDGND turnpin, turnpin1, forkpin;
	INTBIG lowx, highx, lowy, highy, portx, porty;

	if (pla_verbose) ttyputmsg(M_("Running the Vdd and Gnd lines"));

	pla_ConnectThruCells(VDD, pla_otv, M_("RunVddGnd: cannot connect on Vdd at Or Top"), pla);

	/* Export this as the external VDD connection point */
	if (newportproto(pla, pla_otv->inst, pla_otv->port->port, x_("VDD")) == NOPORTPROTO)
		ttyputerr(M_("RunVddGnd: cannot export VDD point"));

	/* Now instance a ground pin above the Pullups on the AND plane
	 * and connect it to both the pu on OR and AND
	 */
	lowx = 0;
	highx = pla_VddWidth;
	lowy = (pla_otv->port->y) - pla_halfVdd;
	highy = lowy + pla_VddWidth;
	portx = pla_alv->port->x;
	porty = pla_otv->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin, pla_otv,
		M_("RunVddGnd: cannot instance Vdd turnpin over pu AND"), pla);

	pla_ConnectOnMetal(VDD, &turnpin, M_("RunVddGnd: cannot connect turnpin to pu on top of OR"), pla);

	turnpin.nextinst = pla_alv;
	pla_ConnectOnMetal(VDD, &turnpin, M_("RunVddGnd: cannot connect turnpin to pu on Left AND/Top"), pla);

	/* Now connect through the pu on AND */
	pla_ConnectThruCells(VDD, pla_alv, M_("RunVddGnd: cannot connect on Vdd at And Left"), pla);

	/* Instance a turnpin near the first input cell and connect it to
	 * both the pullups and the input cells
	 */
	lowy = (pla_icv->port->y) - pla_halfVdd;
	highy = lowy + pla_VddWidth;
	porty = pla_icv->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin, pla_alv,
		M_("RunVddGnd: cannot instance Vdd turnpin near Input cell"), pla);

	pla_ConnectOnMetal(VDD, &turnpin, M_("RunVddGnd: cannot connect turnpin to pu on Left AND/Bot"),pla);

	turnpin.nextinst = pla_icv;
	pla_ConnectOnMetal(VDD, &turnpin, M_("RunVddGnd: cannot connect turnpin to incell on left"), pla);

	/* Connect through the input cells */
	pla_ConnectThruCells(VDD, pla_icv, M_("RunVddGnd: cannot connect on Vdd at Input cells"), pla);

	/* Instance yet two more turnpins on the right of the input cells */
	lowx  = puX + andX;
	highx = lowx + pla_VddWidth;
	lowy  = (pla_icv->port->y) - pla_halfVdd;
	highy = lowy + pla_VddWidth;
	portx = lowx + (INTBIG) ((highx-lowx)/2);
	porty = pla_icv->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin, pla_icv,
		M_("RunVddGnd: cannot instance top Vdd turnpin after Input Cell"), pla);

	pla_ConnectOnMetal(VDD, &turnpin, M_("RunVddGnd: cannot connect turnpin to last input cell"), pla);

	lowy = (pla_ocv->port->y) - pla_halfVdd;
	highy = lowy + pla_VddWidth;
	porty = pla_ocv->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin1, &turnpin,
		M_("RunVddGnd: cannot instance bot Vdd turnpin after Input Cell"), pla);

	pla_ConnectOnMetal(VDD, &turnpin1,
		M_("RunVddGnd: cannot connect the two turnpins on left of input"), pla);

	turnpin1.nextinst = pla_ocv;
	pla_ConnectOnMetal(VDD, &turnpin1, M_("RunVddGnd: cannot connect through output cells"), pla);

	/*  Let us lay out the Ground line now.  Begin with the input cells. */
	pla_ConnectThruCells(GND, pla_icg,
		M_("RunVddGnd: cannot connect on Gnd at Input"), pla);

	/* Instance a fork pin at the end of the input cells.  One arm from
	 * this fork goes up between the planes and the other through the
	 * output cells.
	 */
	lowx  = (pla_arg->port->x) - pla_halfGnd;
	highx = lowx + pla_GndWidth;
	lowy  = (pla_icg->port->y) - pla_halfGnd;
	highy = lowy + pla_GndWidth;
	portx = pla_arg->port->x;
	porty = pla_icg->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &forkpin, pla_arg,
		M_("RunVddGnd: cannot instance Gnd fork pin"), pla);
	pla_ConnectThruCells(GND, &forkpin, M_("RunVddGnd: cannot connect through And Right Gnds"), pla);

	forkpin.nextinst = pla_icg;
	pla_ConnectOnMetal(GND, &forkpin, M_("RunVddGnd: cannot connect input cells to fork pin"), pla);

	/* Instance a turn pin at the top of the planes */
	lowy  = (pla_atg->port->y) - pla_halfGnd;
	highy = lowy + pla_GndWidth;
	porty = pla_atg->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin, pla_arg,
		M_("RunVddGnd: cannot instance Gnd on AND top"), pla);

	pla_ConnectOnMetal(GND, &turnpin,
		M_("RunVddGnd: cannot connect turnpin to Gnd on Right AND/Top"), pla);

	turnpin.nextinst = pla_atg;
	pla_ConnectOnMetal(GND, &turnpin, M_("RunVddGnd: cannot connect turnpin to And Top Gnds"), pla);

	pla_ConnectThruCells(GND, pla_atg, M_("RunVddGnd: cannot connect through And Top Gnds"), pla);

	/* Connect through the other arm of the Gnd fork.
	 * Begin by instancing a turnpin below the fork pin
	 */
	lowx  = (forkpin.port->x) - pla_halfGnd;
	highx = lowx + pla_GndWidth;
	lowy  = (pla_ocg->port->y) - pla_halfGnd;
	highy = lowy + pla_GndWidth;
	portx = forkpin.port->x;
	porty = pla_ocg->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin, &forkpin,
		M_("RunVddGnd: cannot instance Gnd turnpin before Output Cell"), pla);

	pla_ConnectOnMetal(GND, &turnpin, M_("RunVddGnd: cannot connect forkpin to turnpin"), pla);

	turnpin.nextinst = pla_ocg;
	pla_ConnectThruCells(GND, &turnpin, M_("RunVddGnd: cannot connect through output cells"), pla);

	/* Instance a turn pin after the output cells */
	lowx  = (pla_org->port->x) - pla_halfGnd;
	highx = lowx + pla_GndWidth;
	lowy  = (pla_ocg->port->y) - pla_halfGnd;
	highy = lowy + pla_GndWidth;
	portx = pla_org->port->x;
	porty = pla_ocg->port->y;

	pla_InstTurnPin(lowx, highx, lowy, highy, portx, porty, &turnpin, pla_ocg,
		M_("RunVddGnd: cannot instance Gnd turnpin after Output Cell"), pla);

	pla_ConnectOnMetal(GND, &turnpin,
		M_("RunVddGnd: cannot connect Output cells to turnpin on right"), pla);

	turnpin.nextinst = pla_org;
	pla_ConnectThruCells(GND, &turnpin, M_("RunVddGnd: cannot connect through Or Right Gnds"), pla);

	/* Export this as the external GND connection point */
	if (newportproto(pla, pla_org->inst, pla_org->port->port, x_("GND")) == NOPORTPROTO)
		ttyputerr(M_("RunVddGnd: cannot export GND point"));
}

#endif  /* PLATOOL - at top */
