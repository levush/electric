/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: planprog1.c
 * Programmable Logic Array Generator, part I
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
static PROGTRANS *pla_FindPColTrans(INTBIG, PROGTRANS*);
static CELLIST   *pla_PlaceInputs(INTBIG, INTBIG, INTBIG, NODEPROTO*);
static CELLIST   *pla_PlaceOutputs(INTBIG, INTBIG, INTBIG, NODEPROTO*);
static CELLIST   *pla_PlacePullupsonAND(INTBIG, INTBIG, INTBIG, NODEPROTO*);
static CELLIST   *pla_PlacePullupsonOR(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*);
static PROGTRANS *pla_Program(INTBIG, INTBIG, INTBIG, NODEPROTO*);
static PROGTRANS *pla_ProgramAND(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static PROGTRANS *pla_ProgramOR(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static BOOLEAN    pla_ProgramPLA(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, NODEPROTO*);

/*
 * Make the PLA.  First define all the primitives and then
 * call pla_ProgramPLA to program the planes.
 */
BOOLEAN pla_Make(void)
{
	INTBIG inX, inY,               /* Dimensions of Input cell */
		  puX, puY,               /* Dimensions of Double Pullup cell */
		  outX, outY,             /* Dimensions of Output cell */
		  progX, progY,           /* Dimensions of Programming Transistor */
		  connectX, connectY;     /* Dimensions of Connect cell */
	INTBIG pos;
	CHAR keyword[32], *filename;
	TECHNOLOGY *tech;

	if (pla_fin != NULL) xclose(pla_fin);
	pla_fin = xopen(pla_infile, pla_filetypeplatab, x_(""), &filename);
	if (pla_fin == NULL)
	{
		ttyputerr(_("File '%s' not found, use 'telltool pla nmos file FILE'"), pla_infile);
		return(TRUE);
	}
	if (pla_verbose)
		ttyputmsg(M_("File '%s' has been opened for reading"), pla_infile);

	/* Set the Technology to the Default Technology (nmos) */
	tech = gettechnology(PLADEFTECH);
	if (tech == NOTECHNOLOGY)
	{
		ttyputerr(M_("'%s' is an undefined technology"), PLADEFTECH);
		return(TRUE);
	}
	if (el_curtech != tech)
		ttyputmsg(_("Warning: current technology should be %s"), tech->techname);

	pla_lam = el_curlib->lambda[tech->techindex];

	/* Compute the widths of Vdd and Gnd lines */
	if (pla_VddWidth < 4*pla_lam) pla_VddWidth = 4 * pla_lam;
	if (pla_GndWidth < 4*pla_lam) pla_GndWidth = 4 * pla_lam;
	pla_halfVdd = pla_VddWidth / 2;
	pla_halfGnd = pla_GndWidth / 2;

	/* Search for the prototypes for PLA generation */
	pla_md_proto = getnodeproto(x_("nmos:metal-diffusion-con"));
	pla_mp_proto = getnodeproto(x_("nmos:metal-polysilicon-con"));
	pla_dp_proto = getnodeproto(x_("nmos:diffusion-pin"));
	pla_bp_proto = getnodeproto(x_("nmos:metal-pin"));
	pla_pp_proto = getnodeproto(x_("nmos:polysilicon-pin"));
	pla_etran_proto = getnodeproto(x_("nmos:transistor"));
	pla_dtran_proto = getnodeproto(x_("nmos:implant-transistor"));
	if (pla_buttingcontact) pla_bc_proto = getnodeproto(x_("nmos:butting-con"));
		else pla_bc_proto = getnodeproto(x_("nmos:buried-con-cross-t"));
	pla_ma_proto = getarcproto(x_("nmos:metal"));
	pla_da_proto = getarcproto(x_("nmos:diffusion"));
	pla_pa_proto = getarcproto(x_("nmos:polysilicon"));
	if (pla_md_proto    == NONODEPROTO || pla_dp_proto  == NONODEPROTO ||
		pla_mp_proto    == NONODEPROTO || pla_pp_proto  == NONODEPROTO ||
		pla_da_proto    == NOARCPROTO  || pla_pa_proto  == NOARCPROTO  ||
		pla_dtran_proto == NONODEPROTO || pla_ma_proto  == NOARCPROTO  ||
		pla_etran_proto == NONODEPROTO || pla_bc_proto  == NONODEPROTO ||
		pla_bp_proto    == NONODEPROTO)
	{
		ttyputerr(M_("Cannot find primitive prototypes of nmos technology"));
		return(TRUE);
	}

	/* Create all the cell prototypes.  Cells are defined in planfacets.c */
	pla_pu_proto = pla_nmos_Pullup();
	if (pla_pu_proto  == NONODEPROTO)
	{
		ttyputerr(_("Cannot create pullup cell"));
		return(TRUE);
	}
	puX = pla_pu_proto->highx - pla_pu_proto->lowx;
	puY = pla_pu_proto->highy - pla_pu_proto->lowy;

	pla_in_proto = pla_nmos_Input();
	if (pla_in_proto == NONODEPROTO)
	{
		ttyputerr(_("Cannot create input cell"));
		return(TRUE);
	}
	inX = pla_in_proto->highx - pla_in_proto->lowx;
	inY = pla_in_proto->highy - pla_in_proto->lowy;

	pla_connect_proto = pla_nmos_Connect();
	if (pla_connect_proto == NONODEPROTO)
	{
		ttyputerr(_("Cannot create connect cell"));
		return(TRUE);
	}
	connectX = pla_connect_proto->highx - pla_connect_proto->lowx;
	connectY = pla_connect_proto->highy - pla_connect_proto->lowy;

	pla_prog_proto = pla_nmos_Program();
	if (pla_prog_proto== NONODEPROTO)
	{
		ttyputerr(_("Cannot create program cell"));
		return(TRUE);
	}
	progX = pla_prog_proto->highx - pla_prog_proto->lowx;
	progY = pla_prog_proto->highy - pla_prog_proto->lowy;

	pla_out_proto = pla_nmos_Output();
	if (pla_out_proto == NONODEPROTO)
	{
		ttyputerr(_("Cannot create output cell"));
		return(TRUE);
	}
	outX = pla_out_proto->highx - pla_out_proto->lowx;
	outY = pla_out_proto->highy - pla_out_proto->lowy;

	if (pla_verbose)
	{
		ttyputmsg(M_("Cells have been defined with sizes given below:"));
		ttyputmsg(M_("Pullup  = %sx%s"), latoa(puX, 0), latoa(puY, 0));
		ttyputmsg(M_("Input   = %sx%s"), latoa(inX, 0), latoa(inY, 0));
		ttyputmsg(M_("Output  = %sx%s"), latoa(outX, 0), latoa(outY, 0));
		ttyputmsg(M_("Program = %sx%s"), latoa(progX, 0), latoa(progY, 0));
		ttyputmsg(M_("Connect = %sx%s"), latoa(connectX, 0), latoa(connectY, 0));
		ttyputmsg(M_("VDD Width = %s, Gnd Width = %s"), latoa(pla_VddWidth, 0), latoa(pla_GndWidth, 0));
	}

	/* Create the pla prototype with these dimensions.
	 * First find a name for the PLA.
	 */
	for(;;)
	{
		if (pla_ProcessFlags()) break;

		pla_cell = NONODEPROTO;
		pla_alv = pla_atg = pla_arg = pla_otv = NOVDDGND;
		pla_org = pla_icg = pla_icv = pla_ocg = pla_ocv = NOVDDGND;

		if (pla_inputs == 0)
		{
			ttyputerr(_("No inputs set"));
			return(TRUE);
		}
		if (pla_outputs == 0)
		{
			ttyputerr(_("No outputs set"));
			return(TRUE);
		}
		if (pla_pterms == 0)
		{
			ttyputerr(_("No product terms set"));
			return(TRUE);
		}
		if (namesame(pla_name, DEFNAME) == 0)
			(void)esnprintf(pla_name, 32, x_("PLA-%ld-%ld-%ld"), pla_inputs, pla_pterms, pla_outputs);

		pla_cell = newnodeproto(pla_name, el_curlib);
		if (pla_cell == NONODEPROTO)
		{
			ttyputerr(_("Cannot create '%s' cell"), pla_name);
			return(TRUE);
		}

		ttyputmsg(_("Defining '%s'"), pla_name);
		if (pla_verbose)
			ttyputmsg(M_("Beginning to program the AND and OR planes."));
		if (pla_ProgramPLA(inX, inY, puX, puY, outX, outY, connectX,
			connectY, pla_cell))
				ttyputerr(_("PLA '%s' is incompletely defined"), pla_name);

		while((pos = pla_GetKeyWord(keyword)) != ERRORRET)
			if (pos == END) break;
	}
	return(FALSE);
}

/*
 * pla_FindPColTrans: Given a pterm, and the address of an input cell,
 * it tries to find a point below the row indicated by the pterm,
 * which needs to be connected to the programming transistor to be
 * placed on the pterm row.
 */
PROGTRANS *pla_FindPColTrans(INTBIG pterm, PROGTRANS *trans)
{
	INTBIG column;

	column = trans->column;

	while (trans->uprogtrans != NOPROGTRANS)
	{
		if (trans->column != column)
			ttyputerr(M_("FindPColTrans: Column mismatch: wanted %ld, got %ld/%ld"),
				column, trans->column, trans->row);
		trans = trans->uprogtrans;
	}

	if (trans->row >= pterm)
	{
		ttyputerr(M_("FindPColTrans: Error.  Gone past row %ld in column %ld"), pterm, trans->column);
		ttyputerr(M_("   Last progtrans in this column is at row=%ld"), trans->row);
		return(NOPROGTRANS);
	}
	return(trans);
}

/*
 * Place the Input cells one for each input.  The left most
 * cell is placed at X = X length of pullup cells and
 * Y = 0.  Also we have to remember the addresses of the cells
 * for later processing.
 */
CELLIST *pla_PlaceInputs(INTBIG puX, INTBIG inX, INTBIG inY, NODEPROTO *pla)
{
	CELLIST *ptr, *prevptr, *firstptr;
	VDDGND *cicv, *picv, *cicg, *picg;
	INTBIG i;
	INTBIG llXpos, llYpos, VddYpos, GndYpos;
	CHAR portname[16];

	llXpos = puX;
	llYpos = 0 - (pla_VddWidth - 4*pla_lam);
	VddYpos = llYpos + 17*pla_lam + pla_halfVdd;
	GndYpos = llYpos + inY - pla_halfGnd;

	prevptr = NOCELLIST;
	firstptr= NOCELLIST;
	cicv = picv = cicg = picg = NOVDDGND;

	if (pla_verbose) ttyputmsg(M_("Placing the input cells"));

	for(i = 1; i <= pla_inputs; i++)
	{
		if (pla_verbose) ttyputmsg(M_("Placing input cell %ld"), i);
		ptr = (CELLIST *)emalloc(sizeof(CELLIST), pla_tool->cluster);
		if (ptr == NOCELLIST) return(NOCELLIST);
		ptr->findex = i;    /* remember which input this cell corresponds to */
		ptr->cellinst = newnodeinst(pla_in_proto, llXpos, llXpos+inX,
			llYpos, llYpos+inY, NOTRANS, NOROT, pla);
		if (ptr->cellinst == NONODEINST) return(NOCELLIST);

		/* Record the Gnd and Vdd points for later connection */
		cicg = pla_MkVddGnd(INCELLGND, &picg);
		cicg->inst = ptr->cellinst;
		cicg->port = pla_AssignPort(ptr->cellinst, llXpos+6*pla_lam, GndYpos);
		if (cicg->port->port == NOPORTPROTO) return(NOCELLIST);

		cicv = pla_MkVddGnd(INCELLVDD, &picv);
		cicv->inst = ptr->cellinst;
		cicv->port = pla_AssignPort(ptr->cellinst, llXpos+5*pla_lam, VddYpos);
		if (cicv->port->port == NOPORTPROTO) return(NOCELLIST);

		/* Find the ports that connect into the PLA */
		ptr->lport = pla_AssignPort(ptr->cellinst, llXpos+2*pla_lam, llYpos+inY-pla_lam);
		ptr->rport = pla_AssignPort(ptr->cellinst, llXpos+10*pla_lam, llYpos+inY-pla_lam);
		ptr->gport = pla_AssignPort(ptr->cellinst, llXpos+6*pla_lam, GndYpos);

		if (ptr->lport->port == NOPORTPROTO ||
			ptr->rport->port == NOPORTPROTO || ptr->gport->port == NOPORTPROTO)
				return(NOCELLIST);

		/* Initialize the progtrans ends */
		ptr->rcoltrans = ptr->lcoltrans = NOPROGTRANS;

		/* Export the input point giving it a name */
		(void)esnprintf(portname, 16, x_("Input%ld"), i);
		if (newportproto(pla, ptr->cellinst, pla_FindPort(ptr->cellinst,llXpos+10*pla_lam,
			llYpos+pla_lam), portname) == NOPORTPROTO) return(NOCELLIST);

		if (prevptr != NOCELLIST) prevptr->nextcell = ptr;
		prevptr = ptr;

		if (firstptr == NOCELLIST) firstptr = ptr;

		llXpos = llXpos + inX;
	}
	if (ptr != NOCELLIST) ptr->nextcell = NOCELLIST;

	return(firstptr);
}

CELLIST *pla_PlaceOutputs(INTBIG orLeft, INTBIG outX, INTBIG outY, NODEPROTO *pla)
{
	CELLIST *ptr, *prevptr, *firstptr;
	VDDGND *cocv, *pocv, *cocg, *pocg;
	INTBIG o;
	INTBIG llXpos, llYpos, VddYpos, GndYpos;
	CHAR portname[16];

	llXpos = orLeft - pla_lam;
	llYpos = -(pla_VddWidth + 4 * pla_lam);
	VddYpos = llYpos + 7 * pla_lam + pla_halfVdd;
	GndYpos = llYpos + outY - (8 * pla_lam + pla_halfGnd);

	prevptr = firstptr = NOCELLIST;
	cocv = pocv = cocg = pocg = NOVDDGND;

	if (pla_verbose) ttyputmsg(M_("Placing the output cells"));

	for(o = 1; o <= (pla_outputs+pla_outputs%2)/2; o++)
	{
		if (pla_verbose) ttyputmsg(M_("Placing output cell %ld"), o);

		ptr = (CELLIST *)emalloc(sizeof(CELLIST), pla_tool->cluster);
		if (ptr == NOCELLIST) return(NOCELLIST);
		ptr->findex = o;
		ptr->cellinst = newnodeinst(pla_out_proto, llXpos, llXpos+outX,
			llYpos, llYpos+outY, NOTRANS, NOROT, pla);
		if (ptr->cellinst == NONODEINST) return(NOCELLIST);

		/*
		 * Connect the cells together in metal on the Gnd line if more than
		 * one cell is placed
		 */
		/* Remember the Gnd and Vdd points for later connection */
		cocg = pla_MkVddGnd(OUTCELLGND, &pocg);
		cocg->inst = ptr->cellinst;
		cocg->port = pla_AssignPort(ptr->cellinst, llXpos+outX-5*pla_lam, GndYpos);
		if (cocg->port->port == NOPORTPROTO) return(NOCELLIST);

		cocv = pla_MkVddGnd(OUTCELLVDD, &pocv);
		cocv->inst = ptr->cellinst;
		cocv->port = pla_AssignPort(ptr->cellinst, llXpos+5*pla_lam, VddYpos);
		if (cocv->port->port == NOPORTPROTO) return(NOCELLIST);

		/* Find the ports that connect into the PLA */
		ptr->lport = pla_AssignPort(ptr->cellinst, llXpos+5*pla_lam, llYpos+outY-2*pla_lam);
		ptr->rport = pla_AssignPort(ptr->cellinst, llXpos+outX-2*pla_lam, llYpos+outY-2*pla_lam);
		ptr->gport = NOPORT;

		if (ptr->lport->port == NOPORTPROTO || ptr->rport->port == NOPORTPROTO)
			return(NOCELLIST);

		/* Initialize the progtrans ends */
		ptr->rcoltrans = ptr->lcoltrans = NOPROGTRANS;

		/* Export the output ports */
		(void)esnprintf(portname, 16, x_("Output%ld"), 2*o-1);
		if (newportproto(pla, ptr->cellinst,
			pla_FindPort(ptr->cellinst,llXpos+pla_lam,llYpos+pla_lam), portname) == NOPORTPROTO)
				return(NOCELLIST);
		(void)esnprintf(portname, 16, x_("Output%ld"), 2*o);
		if (newportproto(pla, ptr->cellinst,
			pla_FindPort(ptr->cellinst,llXpos+11*pla_lam, llYpos+pla_lam), portname) == NOPORTPROTO)
				return(NOCELLIST);

		if (prevptr != NOCELLIST) prevptr->nextcell = ptr;
		prevptr = ptr;

		if (firstptr == NOCELLIST) firstptr = ptr;

		llXpos = llXpos + outX + pla_lam;
	}
	if (ptr != NOCELLIST) ptr->nextcell = NOCELLIST;

	return(firstptr);
}

CELLIST *pla_PlacePullupsonAND(INTBIG puX, INTBIG puY, INTBIG inY, NODEPROTO *pla)
{
	CELLIST *ptr, *prevptr, *firstptr;
	VDDGND *calv, *palv;
	INTBIG llXpos, llYpos;
	INTBIG p;

	llXpos = 0;
	llYpos = inY + 2*pla_lam - (pla_VddWidth - 4*pla_lam);

	prevptr = NOCELLIST;
	firstptr= NOCELLIST;
	calv = palv = NOVDDGND;

	if (pla_verbose)
		ttyputmsg(M_("Placing the pullup cells on the left edge of the AND-plane"));

	for(p = 1; p <= pla_pterms; p++)
	{
		if (pla_verbose)
			ttyputmsg(M_("Placing pullup cell %ld in AND-plane"), p);

		ptr = (CELLIST *)emalloc(sizeof(CELLIST), pla_tool->cluster);
		if (ptr == NOCELLIST) return(NOCELLIST);
		ptr->findex = p;
		ptr->cellinst = newnodeinst(pla_pu_proto, llXpos, llXpos+puX,
			llYpos, llYpos+puY, NOTRANS, NOROT, pla);
		if (ptr->cellinst == NONODEINST) return(NOCELLIST);

		/* Remember the Vdd points for later connection */
		calv = pla_MkVddGnd(ANDLEFTVDD, &palv);
		calv->inst = ptr->cellinst;
		calv->port = pla_AssignPort(ptr->cellinst, pla_halfVdd, llYpos+4*pla_lam);
		if (calv->port->port == NOPORTPROTO) return(NOCELLIST);

		/* Mark the diffusion end of the pu for connections across the plane */
		ptr->lport = pla_AssignPort(ptr->cellinst, llXpos+puX-2*pla_lam,llYpos+3*pla_lam);
		if (ptr->lport->port == NOPORTPROTO) return(NOCELLIST);

		ptr->rport = NOPORT;
		ptr->rcoltrans = ptr->lcoltrans = NOPROGTRANS;

		if (prevptr != NOCELLIST) prevptr->nextcell = ptr;
		prevptr = ptr;

		if (firstptr == NOCELLIST) firstptr = ptr;

		llYpos = llYpos + puY;
	}
	if (ptr != NOCELLIST) ptr->nextcell = NOCELLIST;

	return(firstptr);
}

CELLIST *pla_PlacePullupsonOR(INTBIG andY, INTBIG orLeft, INTBIG inY, INTBIG puX,
	INTBIG puY, NODEPROTO *pla)
{
	CELLIST *ptr, *prevptr, *firstptr;
	VDDGND *cotv, *potv;
	INTBIG llXpos, llYpos, VddYpos, cX, cY, bc_portY;
	INTBIG i;

	inY = inY - (pla_VddWidth - 4*pla_lam); /* Compensate for Extra Vdd Width */
	cX = orLeft + 5*pla_lam;	      /* Center coordinates */
	cY = inY + andY + puX/2 + (pla_GndWidth-4*pla_lam);

	llXpos = cX - puX/2;
	llYpos = cY - puY/2;
	VddYpos = cY + puX/2 - pla_halfVdd;
	bc_portY = cY - puX/2 + 2*pla_lam;

	prevptr = firstptr = NOCELLIST;
	cotv = potv = NOVDDGND;

	if (pla_verbose)
		ttyputmsg(M_("Placing the pullups on the %s edge of the OR-plane"),
			pla_samesideoutput ? M_("top") : M_("bottom"));

	for(i = 1; i <= pla_outputs; i++)
	{
		if (pla_verbose)
			ttyputmsg(M_("Placing pullup cell %ld in OR-plane"), i);

		ptr = (CELLIST *)emalloc(sizeof(CELLIST), pla_tool->cluster);
		if (ptr == NOCELLIST) return(NOCELLIST);
		ptr->findex = i;
		ptr->cellinst = newnodeinst(pla_pu_proto, llXpos, llXpos+puX,
			llYpos, llYpos+puY, NOTRANS, 2700, pla);
		if (ptr->cellinst == NONODEINST) return(NOCELLIST);

		/* Remember the Vdd points for later connection */
		cotv = pla_MkVddGnd(ORTOPVDD, &potv);
		cotv->inst = ptr->cellinst;
		cotv->port = pla_AssignPort(ptr->cellinst, llXpos+puX/2, VddYpos);
		if (cotv->port->port == NOPORTPROTO) return(NOCELLIST);

		/* Mark the diffusion end of the pu for connections across the plane */
		ptr->lport = pla_AssignPort(ptr->cellinst, llXpos+puX/2-pla_lam, bc_portY);
		if (ptr->lport->port == NOPORTPROTO) return(NOCELLIST);

		ptr->rport = NOPORT;
		ptr->rcoltrans = ptr->lcoltrans = NOPROGTRANS;

		if (prevptr != NOCELLIST) prevptr->nextcell = ptr;
		prevptr = ptr;

		if (firstptr == NOCELLIST) firstptr = ptr;

		llXpos = llXpos + puY;
	}
	if (ptr != NOCELLIST) ptr->nextcell = NOCELLIST;

	return(firstptr);
}

/*
 * Place a programming transistor at the given position and orientation.
 * Also record its ports for later use.
 */
PROGTRANS *pla_Program(INTBIG lowx, INTBIG lowy, INTBIG orientation, NODEPROTO *pla)
{
	INTBIG highx, highy, Ylen, Xlen, centerx, centery;
	INTBIG rot;
	NODEINST *inst;
	PROGTRANS *trans;

	Ylen = pla_prog_proto->highy - pla_prog_proto->lowy;
	Xlen = pla_prog_proto->highx - pla_prog_proto->lowx;

	centerx = lowx + Xlen/2;
	centery = lowy + Ylen/2;

	/* Find the location of the programming transistor */
	highx = lowx + Xlen;
	highy = lowy + Ylen;

	switch(orientation)
	{
		case LEFT:  rot = 1800;     break;
		case RIGHT: rot = NOROT;    break;
		case UP:    rot = 900;      break;
		case DOWN:  rot = 2700;     break;
	}

	/*
	 * Now instance the pla_prog_proto at the coordinates computed above
	 * in the calculated orientation
	 */
	trans = (PROGTRANS *)emalloc(sizeof(PROGTRANS), pla_tool->cluster);
	if (trans == NOPROGTRANS)
	{
		ttyputerr(M_("Program: ran out of storage for PROGTRANS"));
		return(NOPROGTRANS);
	} else if (trans == NOPROGTRANS) return(NOPROGTRANS);

	inst = newnodeinst(pla_prog_proto, lowx, highx, lowy, highy, NOTRANS, rot, pla);
	if (inst == NONODEINST)
		ttyputerr(M_("Program: cannot instance programming trans"));

	trans->trans = inst;

	/*
	 * Find the four ports of the programming transistor.
	 * ports 1 is the diff arc port.  ports 3 and 4 are poly arc
	 * ports. port2 is the metal arc port.
	 */
	switch(orientation)
	{
		case LEFT:
			trans->metalport = pla_AssignPort(inst, lowx+2*pla_lam, lowy+Ylen/2);
			trans->diffport = pla_AssignPort(inst, highx-2*pla_lam, lowy+Ylen/2);
			trans->polyblport = pla_AssignPort(inst, lowx+Xlen/2, lowy+pla_lam);
			trans->polytrport = pla_AssignPort(inst, lowx+Xlen/2, highy-pla_lam);
			break;
		case RIGHT:
			trans->diffport = pla_AssignPort(inst, lowx+2*pla_lam, lowy+Ylen/2);
			trans->metalport = pla_AssignPort(inst,highx-2*pla_lam, lowy+Ylen/2);
			trans->polyblport = pla_AssignPort(inst, lowx+Xlen/2, lowy+pla_lam);
			trans->polytrport = pla_AssignPort(inst, lowx+Xlen/2, highy-pla_lam);
			break;
		case UP:
			trans->diffport = pla_AssignPort(inst, centerx, centery-Xlen/2+2*pla_lam);
			trans->metalport = pla_AssignPort(inst, centerx, centery+Xlen/2-2*pla_lam);
			trans->polyblport = pla_AssignPort(inst, centerx-Ylen/2+pla_lam, centery);
			trans->polytrport = pla_AssignPort(inst, centerx+Ylen/2-pla_lam, centery);
			break;
		case DOWN:
			trans->metalport = pla_AssignPort(inst,centerx, centery-Xlen/2+2*pla_lam);
			trans->diffport = pla_AssignPort(inst,centerx, centery+Xlen/2-2*pla_lam);
			trans->polyblport = pla_AssignPort(inst,centerx-Ylen/2+pla_lam, centery);
			trans->polytrport = pla_AssignPort(inst,centerx+Ylen/2-pla_lam, centery);
			break;
	}
	if ((trans->diffport->port   == NOPORTPROTO ||
		trans->metalport->port  == NOPORTPROTO ||
		trans->polyblport->port == NOPORTPROTO ||
		trans->polytrport->port == NOPORTPROTO))
			ttyputerr(M_("Program: cannot find transistor ports"));

	trans->uprogtrans = trans->rprogtrans = NOPROGTRANS;

	return(trans);
}

PROGTRANS *pla_ProgramAND(INTBIG in, INTBIG arm, INTBIG pterm, INTBIG puX, INTBIG puY,
	INTBIG inX, INTBIG inY)
{
	INTBIG llXpos, llYpos;  /* Lower Left X,Y position of the point
						   where an instance of the programming
						   cell is to be placed */

	/*
	 * NOTE:  We are not taking care of situations where if there are
	 * a large number of inputs, pterms and outputs, extra
	 * Vdd and Gnd lines have to run within the PLA.
	 */
	inY = inY - (pla_VddWidth - 4*pla_lam); /* Compensate for Extra Vdd Width */
	llYpos = inY + (pterm-1) * puY + pla_lam; /* 1 pla_lam from top edge of
											   Input cell */
	if (arm == TRUE)
	{
		llXpos = puX+(in-1)*inX+4*pla_lam;
		return(pla_Program(llXpos, llYpos, RIGHT, pla_cell));
	}

	/* FALSE: */
	llXpos = puX+(in-1)*inX-4*pla_lam;
	return(pla_Program(llXpos, llYpos, LEFT, pla_cell));
}

PROGTRANS *pla_ProgramOR(INTBIG out, INTBIG row, INTBIG orLeft, INTBIG puY, INTBIG inY)
{
	INTBIG llXpos, llYpos;

	/*
	 * NOTE:  We are not taking care of situations where if there are
	 * a large number of inputs, pterms and outputs, extra
	 * Vdd and Gnd lines have to run within the PLA.
	 */
	inY = inY - (pla_VddWidth - 4*pla_lam); /* Compensate for Extra Vdd Width */
	llXpos = orLeft - 2*pla_lam + (out-1)*8*pla_lam;
	llYpos = (row-1)*puY + inY + 2*pla_lam;

	/* The programming transistor next to the pullup must always be oriented UP*/
	if (pla_pterms % 2)		/* Odd number of pterms */
	{
		if (row % 2)		/*  Odd pterm */
			return(pla_Program(llXpos, llYpos, UP, pla_cell));
		else				/*  Even pterm */
			return(pla_Program(llXpos, llYpos, DOWN, pla_cell));
	} else
	{
		if (row % 2)		/*  Odd pterm */
			return(pla_Program(llXpos, llYpos, DOWN, pla_cell));
		else				/* Even pterm */
			return(pla_Program(llXpos, llYpos, UP, pla_cell));
	}
}

/*
 * Now it is time to program the PLA by instancing the various cells
 * we have created.  The origin of the structure is set to the Lower
 * Left (LL) corner in line with the bottom of the Input cells and the
 * Pullup cells on the AND plane.  This means that the LL corner of
 * Output cells will be on the negative  side of Y axis.
 */
BOOLEAN pla_ProgramPLA(INTBIG inX, INTBIG inY, INTBIG puX, INTBIG puY, INTBIG outX, INTBIG outY,
	INTBIG connectX, INTBIG connectY, NODEPROTO *pla)
{
	INTBIG i, o, p, out,		/* Loop control variables */
		val,				/* Temporary */
		andX, andY, orX,	/* Dimensions of the planes */
		orLeft,				/* Left edge of OR plane */
		orRight,			/* Right edge of OR plane */
		CurRow, CurCol;		/* Current Row and Column positions */
	CHAR keyword[16];		/* Input holder */
	NODEINST *connect;		/* Metal-Poly Contact */
	CELLIST *PullupAND,		/* List of Pullups on AND plane */
		*PullupOR,			/* List of Pullups on OR plane */
		*Inputs,			/* List of Input cells */
		*Outputs,			/* List of Output cells */
		*inptr,				/* To keep track of current column in AND */
		*ptermptr,			/* To keep track of current row */
		*outptr;			/* To keep track of current column in OR */
	PROGTRANS *CurTrans,	/* Current transistor being placed */
		*PRowTrans,			/* Trans on the same row, previous column */
		*PColTrans;			/* Trans on the same column, bottom row*/

	/*
	 * Let us first figure out the dimensions of the planes.
	 * The Input cells can be placed side by side without any gaps
	 * between them.  The required gap has been wired in to the cell itself.
	 * Hence, andX = inX * number of inputs.  Similarly,
	 * andY = orY = puY * number of pterms, orX = outX * number of outputs.
	 */
	andX = inX  * pla_inputs;
	andY = puY  * pla_pterms;
	orX  = outX * (pla_outputs+pla_outputs%2)/2;
	orLeft = puX + andX + connectX;
	orRight = orLeft + orX + 3*pla_lam;  /* 3 is the metal-metal separation */

	if (pla_verbose)
	{
		ttyputmsg(M_("Dimensions of the planes:"));
		ttyputmsg(M_("  andX = %s,    andY = %s"), latoa(andX, 0), latoa(andY, 0));
		ttyputmsg(M_("   orX = %s,     orY = %s"), latoa(orX, 0), latoa(andY, 0));
		ttyputmsg(M_("orLeft = %s, orRight = %s"), latoa(orLeft, 0), latoa(orRight, 0));
	}

	/* Place all the input, output and pullup cells */
	PullupAND = pla_PlacePullupsonAND(puX, puY, inY, pla);
	if (PullupAND == NOCELLIST) return(TRUE);
	PullupOR  = pla_PlacePullupsonOR(andY, orLeft, inY, puX, puY, pla);
	if (PullupOR == NOCELLIST) return(TRUE);
	Inputs    = pla_PlaceInputs(puX, inX, inY, pla);
	if (Inputs == NOCELLIST) return(TRUE);
	Outputs   = pla_PlaceOutputs(orLeft, outX, outY, pla);
	if (Outputs == NOCELLIST) return(TRUE);

	/* Initialize all the pointers */
	ptermptr = PullupAND;
	inptr    = Inputs;
	outptr   = Outputs;
	CurTrans = PRowTrans = NOPROGTRANS;

	/* Place the programming transistors */
	for(p = 1; p <= pla_pterms; p++)
	{
		if (ptermptr == NOCELLIST) return(TRUE);

		if (pla_verbose) ttyputmsg(M_("Programming row %ld."), p);

		CurRow   = ptermptr->findex;
		if (CurRow != p) return(TRUE);

		/* Program the AND plane */
		for(i = 1; i <= pla_inputs; i++)
		{
			if (inptr == NOCELLIST) return(TRUE);

			CurCol = inptr->findex;
			if (CurCol != i) return(TRUE);

			PColTrans = NOPROGTRANS;

			/*
			 * Read a character from the input file.  Depending on its
			 * value (0 or 1 or X or x or -) place a enhancement mode
			 * transistor at the appropriate place in the AND plane.
			 */
			if ((val = pla_GetKeyWord(keyword)) == ERRORRET)
			{
				ttyputerr(_("End-of-File while programming AND plane"));
				return(TRUE);
			}
			switch(val)
			{
				case ONE:
					CurTrans = pla_ProgramAND(i, TRUE,  p, puX,puY, inX,inY);
					break;
				case ZERO:
					CurTrans = pla_ProgramAND(i, FALSE, p, puX,puY, inX,inY);
					break;
				case DONTCAREX:  /* Do nothing for don't cares */
				case DONTCAREM: CurTrans = NOPROGTRANS;
					break;
				default:
					ttyputerr(_("Undefined characters '%s' in programming section"), keyword);
					break;
			}

		   /*
			* This point is not being programmed.
			* It could also mean error. However, that would have been
			* pointed out by pla_ProgramAND.
			*/
			if (CurTrans == NOPROGTRANS)
			{
				if (pla_verbose)
					ttyputmsg(M_("row=%ld, ANDcolumn=%ld not programmed."), CurRow, CurCol);
				inptr = inptr->nextcell;
				continue;
			}

			if (pla_verbose) ttyputmsg(M_("row=%ld, ANDcolumn=%ld programmed."), CurRow, CurCol);

			CurTrans->row = CurRow;
			CurTrans->column = CurCol;

		   /*
			* This is the first transistor on this row.
			* Connect it to the Pullup.
			*/
			if (PRowTrans == NOPROGTRANS) ptermptr->lcoltrans = CurTrans;
				else PRowTrans->rprogtrans = CurTrans;

			PRowTrans = CurTrans;

			switch(val)
			{
				case ONE:
					if (inptr->rcoltrans == NOPROGTRANS)
						inptr->rcoltrans = CurTrans; else
							PColTrans = pla_FindPColTrans(p, inptr->rcoltrans);
					break;
				case ZERO:
					if (inptr->lcoltrans == NOPROGTRANS)
						inptr->lcoltrans = CurTrans; else
							PColTrans = pla_FindPColTrans(p, inptr->lcoltrans);
					break;
			}
			if (PColTrans != NOPROGTRANS) PColTrans->uprogtrans = CurTrans;
			inptr = inptr->nextcell;
		}

		/*
		 * Before we program the OR plane, it is necessary to place the
		 * cells connecting it to the AND plane and the Ground line.
		 *
		 *  PRowTrans points to the last transistor placed on this row.
		 */
		/* compensate for extra Vdd Width */
		inY = inY - (pla_VddWidth-4*pla_lam);
		connect = newnodeinst(pla_connect_proto, orLeft-connectX, orLeft,
			inY+(p-1)*puY, inY+(p-1)*puY+connectY, NOTRANS, NOROT, pla);

		if (connect == NONODEINST) return(TRUE);

		/*
		 * Instance a progtrans structure and place the connect cell in it
		 * so that the connections can be made more uniformly
		 */
		CurTrans = (PROGTRANS *)emalloc(sizeof(PROGTRANS), pla_tool->cluster);
		if (CurTrans == NOPROGTRANS)
		{
			ttyputnomemory();
			return(TRUE);
		} else if (CurTrans == NOPROGTRANS) return(TRUE);

		/* Fill in the details for this connect cell */
		CurTrans->row = CurRow;
		CurTrans->column = -1;	/* Undefined */
		CurTrans->trans = connect;
		CurTrans->diffport = pla_AssignPort(connect, orLeft-pla_lam-pla_halfGnd,
			inY+(p-1)*puY+2*pla_lam);
		CurTrans->metalport = pla_AssignPort(connect, orLeft-connectX+2*pla_lam,
			inY+(p-1)*puY+5*pla_lam);
		CurTrans->polytrport = pla_AssignPort(connect, orLeft-pla_lam,
			inY+(p-1)*puY+6*pla_lam);
		CurTrans->polyblport = NOPORT;

		inY = inY + (pla_VddWidth - 4*pla_lam); /* Uncompensate */

		if ((CurTrans->metalport->port == NOPORTPROTO ||
			CurTrans->diffport->port == NOPORTPROTO || CurTrans->polytrport->port == NOPORTPROTO))
				return(TRUE);

		CurTrans->uprogtrans = CurTrans->rprogtrans = NOPROGTRANS;

		if (PRowTrans == NOPROGTRANS)
		{
			/*
			 * Error.  This means that no programming transistors were
			 * placed on this row in the AND plane.
			 */
			ttyputerr(_("*** Warning: Nothing on AND plane on row %ld"), p);
			ptermptr->lcoltrans = CurTrans;
		} else PRowTrans->rprogtrans = CurTrans;

		/* Now connect these two together on metal */
		if (newarcinst(pla_ma_proto, 3*pla_lam, pla_userbits, PRowTrans->trans,
			PRowTrans->metalport->port, PRowTrans->metalport->x, PRowTrans->metalport->y,
				CurTrans->trans, CurTrans->metalport->port, CurTrans->metalport->x,
					CurTrans->metalport->y, pla) == NOARCINST)
						return(TRUE);

		PRowTrans = CurTrans;

		for(o = 1; o <= (pla_outputs+pla_outputs%2)/2; o++)
		{
			if (outptr == NOCELLIST) return(TRUE);

			/*
			 * Each output cell corresponds to two vertical columns of output.
			 *  So run in a loop twice.
			 */
			if (o != outptr->findex) return(TRUE);

			for(out = 1; out >= 0; out--)
			{
				CurCol = 2 * o - out;
				if (CurCol <= pla_outputs)
				{
					PColTrans = NOPROGTRANS;

					if ((val = pla_GetKeyWord(keyword)) == ERRORRET)
					{
						ttyputerr(_("End-of-File while programming OR plane"));
						return(TRUE);
					}
					switch(val)
					{
						case ONE:
							CurTrans = pla_ProgramOR(CurCol, p, orLeft, puY,inY);
							break;
						case ZERO:	/* 0 is undefined here */
						case DONTCAREX:	/* Do nothing for don't cares */
						case DONTCAREM:
							CurTrans = NOPROGTRANS;
							break;
						default:
							ttyputerr(_("Bad characters in programming section"));
							break;
					}
					if (CurTrans == NOPROGTRANS)
					{
						/* This point is not being programmed */
						if (pla_verbose)
							ttyputmsg(M_("row=%ld, ORcolumn=%ld not programmed."), CurRow, CurCol);
						continue;
					}

					if (pla_verbose)
						ttyputmsg(M_("row=%ld, ORcolumn=%ld programmed."), CurRow, CurCol);

					CurTrans->row = CurRow;
					CurTrans->column = CurCol;

					PRowTrans->rprogtrans = CurTrans;
					PRowTrans = CurTrans;

					switch(out)
					{
						case 1:
							if (outptr->lcoltrans == NOPROGTRANS)
								outptr->lcoltrans = CurTrans; else
									PColTrans = pla_FindPColTrans(p, outptr->lcoltrans);
							break;
						case 0:
							if (outptr->rcoltrans == NOPROGTRANS)
								outptr->rcoltrans = CurTrans; else
									PColTrans = pla_FindPColTrans(p, outptr->rcoltrans);
							break;
					}
					if (PColTrans != NOPROGTRANS)
						PColTrans->uprogtrans = CurTrans;
				}
			}
			outptr = outptr->nextcell;
		}

		/* Before we switch to a new row, let us reassign the pointers */
		inptr = Inputs;
		outptr = Outputs;
		ptermptr = ptermptr->nextcell;
		PRowTrans = NOPROGTRANS;
	}

	/*
	 * Let us now draw poly, diff and metal arcs to interconnect the
	 * programming trasistors, pullup, input and output cells.
	 */
	pla_ConnectPlanes(inY, andY, orRight, Inputs, Outputs, PullupAND, PullupOR, pla);

	pla_RunVddGnd(puX, andX, pla);
	return(FALSE);
}

#endif  /* PLATOOL - at top */
