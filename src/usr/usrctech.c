/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrctech.c
 * User interface tool: technology translation module
 * Written by: Steven M. Rubin, Static Free Software
 * Schematic conversion written by: Nora Ryan, Schlumberger Palo Alto Research
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

#include "global.h"
#include "efunction.h"
#include "egraphics.h"
#include "tech.h"
#include "tecgen.h"
#include "tecschem.h"
#include "usr.h"
#include "drc.h"
#include "usredtec.h"		/* for technology documentation */
#include <math.h>

#define TRAN_PIN   -1
#define TRAN_CELL -2

/* prototypes for local routines */
static PORTPROTO *us_convport(NODEINST*, NODEINST*, PORTPROTO*);
static void       us_dumpfields(CHAR***, INTBIG, INTBIG, FILE*, CHAR*);
static ARCPROTO  *us_figurenewaproto(ARCPROTO*, TECHNOLOGY*);
static NODEPROTO *us_figurenewnproto(NODEINST*, TECHNOLOGY*);
static PORTPROTO *us_tranconvpp(NODEINST*, PORTPROTO*);
static PORTPROTO *us_trangetproto(NODEINST*, INTBIG);
static INTBIG     us_tranismos(NODEINST*);
static void       us_tranplacenode(NODEINST*, NODEPROTO*, NODEPROTO*, TECHNOLOGY*, TECHNOLOGY*);
static NODEPROTO *us_tran_linkage(CHAR*, VIEW*, NODEPROTO*);
static void       us_tran_logmakearcs(NODEPROTO*, NODEPROTO*);
static NODEINST  *us_tran_logmakenode(NODEPROTO*, NODEINST*, INTSML, INTSML, NODEPROTO*, TECHNOLOGY*);
static void       us_tran_logmakenodes(NODEPROTO*, NODEPROTO*, TECHNOLOGY*);
static NODEPROTO *us_tran_makelayoutcells(NODEPROTO*, CHAR*, TECHNOLOGY*, TECHNOLOGY*, VIEW*);
static BOOLEAN    us_tran_makelayoutparts(NODEPROTO*, NODEPROTO*, TECHNOLOGY*, TECHNOLOGY*, VIEW*);
static void       us_tran_makemanhattan(NODEPROTO*);

/*
 * this routine converts cell "oldcell" to one of technology "newtech".
 * Returns the address of the new cell (NONODEPROTO on error).
 */
NODEPROTO *us_convertcell(NODEPROTO *oldcell, TECHNOLOGY *newtech)
{
	NODEPROTO *newcell, *np;
	NODEPROTO *(*localconversion)(NODEPROTO*, TECHNOLOGY*);
	REGISTER TECHNOLOGY *oldtech, *tech;
	REGISTER ARCINST *ai;
	VARIABLE *var;

	/* cannot convert text-only views */
	if ((oldcell->cellview->viewstate&TEXTVIEW) != 0)
	{
		ttyputerr(_("Cannot convert textual views: only layout and schematic"));
		return(NONODEPROTO);
	}

	/* separate code if converting to the schematic technology */
	if (newtech == sch_tech)
	{
		/*
		 * Look to see if the technology has its own routine.
		 * Note that it may call some of the functions in this file!
		 */
		var = getval((INTBIG)el_curtech, VTECHNOLOGY, VADDRESS, x_("TECH_schematic_conversion"));
		if (var != NOVARIABLE)
		{
			localconversion = (NODEPROTO *(*)(NODEPROTO*, TECHNOLOGY*))var->addr;
			newcell = (*(localconversion))(oldcell, newtech);
			if (newcell == NONODEPROTO) return(NONODEPROTO);
		} else    /* Convert to schematic here */
		{
			/* create cell in new technology */
			newcell = us_tran_linkage(oldcell->protoname, el_schematicview, oldcell);
			if (newcell == NONODEPROTO) return(NONODEPROTO);

			/* create the parts in this cell */
			us_tran_logmakenodes(oldcell, newcell, newtech);
			us_tran_logmakearcs(oldcell, newcell);

			/* now make adjustments for manhattan-ness */
			us_tran_makemanhattan(newcell);

			/* set "FIXANG" if reasonable */
			for(ai = newcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				if (ai->end[0].xpos == ai->end[1].xpos &&
					ai->end[0].ypos == ai->end[1].ypos) continue;
				if ((figureangle(ai->end[0].xpos, ai->end[0].ypos, ai->end[1].xpos,
					ai->end[1].ypos)%450) == 0) ai->userbits |= FIXANG;
			}
		}

		/* after adjusting contents, must re-solve to get proper cell size */
		(*el_curconstraint->solve)(newcell);
	} else
	{
		/* do general conversion between technologies */
		oldtech = oldcell->tech;

		/* reset flag that primitive cannot be converted */
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->temp1 = 0;
		newcell = us_tran_makelayoutcells(oldcell, oldcell->protoname,
			oldtech, newtech, el_layoutview);
	}
	return(newcell);
}

/*
 * routine to create a new cell called "newcellname" that is to be the
 * equivalent to an old cell in "cell".  The view type of the new cell is
 * in "newcellview" and the view type of the old cell is in "cellview"
 */
NODEPROTO *us_tran_linkage(CHAR *newcellname, VIEW *newcellview, NODEPROTO *cell)
{
	NODEPROTO *newcell;
	REGISTER CHAR *cellname;
	REGISTER void *infstr;

	/* create the new cell */
	if (newcellview->sviewname[0] == 0) cellname = newcellname; else
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, newcellname);
		addtoinfstr(infstr, '{');
		addstringtoinfstr(infstr, newcellview->sviewname);
		addtoinfstr(infstr, '}');
		cellname = returninfstr(infstr);
	}
	newcell = us_newnodeproto(cellname, cell->lib);
	if (newcell == NONODEPROTO)
		ttyputmsg(_("Could not create cell: %s"), cellname); else
			ttyputmsg(_("Creating new cell: %s"), cellname);
	return(newcell);
}

/********************** CODE FOR CONVERSION TO SCHEMATIC **********************/

void us_tran_logmakenodes(NODEPROTO *cell, NODEPROTO *newcell, TECHNOLOGY *newtech)
{
	NODEINST *ni, *schemni;
	NODEPROTO *onp;
	PORTEXPINST *pexp;
	REGISTER PORTPROTO *pp, *pp2;
	REGISTER PORTARCINST *pi;
	REGISTER VARIABLE *var;
	INTSML rotate, trans;
	INTBIG type, len, wid, lambda, xoff, yoff, i, size;
	UINTBIG descript[TEXTDESCRIPTSIZE];

	/*
	 * for each node, create a new node in the newcell, of the correct
	 * logical type.  Also, store a pointer to the new node in the old
	 * node's temp1.  This is used in the arc translation part of the
	 * program to find the new ends of each arc.
	 */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		type = us_tranismos(ni);
		switch (type)
		{
			case TRAN_PIN:
				/* compute new x, y coordinates */
				schemni = us_tran_logmakenode(sch_wirepinprim, ni, 0, 0, newcell, newtech);
				break;
			case TRAN_CELL:
				FOR_CELLGROUP(onp, ni->proto)
					if (onp->cellview == el_schematicview) break;
				if (onp == NONODEPROTO)
				{
					onp = us_convertcell(ni->proto, newtech);
					if (onp == NONODEPROTO) break;
				}
				schemni = us_tran_logmakenode(onp, ni, ni->transpose, ni->rotation,
					newcell, newtech);
				break;
			case NPUNKNOWN:	/* could not match it */
				 schemni = NONODEINST;
				 break;
			default:		/* always a transistor */
				rotate = ni->rotation;
				trans = ni->transpose;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->proto == ni->proto->firstportproto) break;
				if (pi != NOPORTARCINST)
				{
					if (trans == 0) rotate += 900; else rotate += 2700;
					trans = 1 - trans;
				}
				rotate += 2700;
				while (rotate >= 3600) rotate -= 3600;
				schemni = us_tran_logmakenode(sch_transistorprim, ni, trans, rotate,
					newcell, newtech);

				/* set the type of the transistor */
				switch (type)
				{
					case NPTRAPMOS:  schemni->userbits |= TRANPMOS;    break;
					case NPTRANMOS:  schemni->userbits |= TRANNMOS;    break;
					case NPTRADMOS:  schemni->userbits |= TRANDMOS;    break;
					case NPTRAPNP:   schemni->userbits |= TRANPNP;     break;
					case NPTRANPN:   schemni->userbits |= TRANNPN;     break;
					case NPTRANJFET: schemni->userbits |= TRANNJFET;   break;
					case NPTRAPJFET: schemni->userbits |= TRANPJFET;   break;
					case NPTRADMES:  schemni->userbits |= TRANDMES;    break;
					case NPTRAEMES:  schemni->userbits |= TRANEMES;    break;
				}

				/* add in the size */
				transistorsize(ni, &len, &wid);
				if (len >= 0 && wid >= 0)
				{
					TDCLEAR(descript);
					defaulttextsize(3, descript);
					lambda = lambdaofnode(ni);
					if (type == NPTRAPMOS || type == NPTRANMOS || type == NPTRADMOS ||
						type == NPTRANJFET || type == NPTRAPJFET ||
						type == NPTRADMES || type == NPTRAEMES)
					{
						/* set length/width */
						us_getlenwidoffset(schemni, descript, &xoff, &yoff);
						var = setvalkey((INTBIG)schemni, VNODEINST, el_attrkey_length,
							len*WHOLE/lambda, VFRACT|VDISPLAY);
						if (var != NOVARIABLE)
						{
							TDCOPY(var->textdescript, descript);
							size = TDGETSIZE(var->textdescript);
							i = TXTGETPOINTS(size);
							if (i > 3) size = TXTSETPOINTS(i-2); else
							{
								i = TXTGETQLAMBDA(size);
								if (i > 3) size = TXTSETQLAMBDA(i-2);
							}
							TDSETSIZE(var->textdescript, size);
							TDSETOFF(var->textdescript, TDGETXOFF(descript)-xoff,
								TDGETYOFF(descript)-yoff);
						}
						var = setvalkey((INTBIG)schemni, VNODEINST, el_attrkey_width,
							wid*WHOLE/lambda, VFRACT|VDISPLAY);
						if (var != NOVARIABLE)
						{
							TDCOPY(var->textdescript, descript);
							TDSETOFF(var->textdescript, TDGETXOFF(descript)+xoff,
								TDGETYOFF(descript)+yoff);
						}
					} else
					{
						/* set area */
						var = setvalkey((INTBIG)schemni, VNODEINST, el_attrkey_area,
							len*WHOLE/lambda, VFRACT|VDISPLAY);
						if (var != NOVARIABLE)
							TDCOPY(var->textdescript, descript);
					}
				}
		}

		/* store the new node in the old node */
		ni->temp1 = (INTBIG)schemni;

		/* reexport ports */
		if (schemni != NONODEINST)
		{
			for(pexp = ni->firstportexpinst; pexp != NOPORTEXPINST; pexp = pexp->nextportexpinst)
			{
				pp = us_tranconvpp(ni, pexp->proto);
				if (pp == NOPORTPROTO) continue;
				pp2 = newportproto(newcell, schemni, pp, pexp->exportproto->protoname);
				if (pp2 == NOPORTPROTO) return;
				pp2->userbits = (pp2->userbits & ~STATEBITS) |
					(pexp->exportproto->userbits & STATEBITS);
				TDCOPY(pp2->textdescript, pexp->exportproto->textdescript);
				if (copyvars((INTBIG)pexp->exportproto, VPORTPROTO, (INTBIG)pp2, VPORTPROTO, FALSE))
					return;
			}
			endobjectchange((INTBIG)schemni, VNODEINST);
		}
	}
}

NODEINST *us_tran_logmakenode(NODEPROTO *prim, NODEINST *orig, INTSML trn, INTSML rot,
	NODEPROTO *newcell, TECHNOLOGY *newtech)
{
	REGISTER INTBIG cx, cy, scaleu, scaled;
	INTBIG sx, sy;
	REGISTER NODEINST *newni;
	REGISTER TECHNOLOGY *oldtech;

	scaleu = el_curlib->lambda[newtech->techindex];
	oldtech = orig->proto->tech;
	scaled = el_curlib->lambda[oldtech->techindex];
	cx = muldiv((orig->lowx+orig->highx)/2, scaleu, scaled) - (prim->highx-prim->lowx)/2;
	cy = muldiv((orig->lowy+orig->highy)/2, scaleu, scaled) - (prim->highy-prim->lowy)/2;
	defaultnodesize(prim, &sx, &sy);
	newni = newnodeinst(prim, cx, cx+sx, cy, cy+sy, trn, rot, newcell);
	return(newni);
}

void us_tran_logmakearcs(NODEPROTO *cell, NODEPROTO *newcell)
{
	PORTPROTO *oldpp1, *oldpp2, *newpp1, *newpp2;
	ARCINST *oldai, *newai;
	NODEINST *newni1, *newni2;
	INTBIG x1, x2, y1, y2, bits;

	/*
	 * for each arc in cell, find the ends in the new technology, and
	 * make a new arc to connect them in the new cell.
	 */
	for(oldai = cell->firstarcinst; oldai != NOARCINST; oldai = oldai->nextarcinst)
	{
		newni1 = (NODEINST *)oldai->end[0].nodeinst->temp1;
		newni2 = (NODEINST *)oldai->end[1].nodeinst->temp1;
		if (newni1 == NONODEINST || newni2 == NONODEINST) continue;
		oldpp1 = oldai->end[0].portarcinst->proto;
		oldpp2 = oldai->end[1].portarcinst->proto;

		/* find the logical portproto for the first end node */
		newpp1 = us_tranconvpp(oldai->end[0].nodeinst, oldpp1);
		if (newpp1 == NOPORTPROTO) continue;

		/* find the logical portproto for the second end node */
		newpp2 = us_tranconvpp(oldai->end[1].nodeinst, oldpp2);
		if (newpp2 == NOPORTPROTO) continue;

		/* find the endpoints of the arc */
		portposition(newni1, newpp1, &x1, &y1);
		portposition(newni2, newpp2, &x2, &y2);

		/* create the new arc */
		bits = us_makearcuserbits(sch_wirearc) & ~(FIXANG|FIXED);
		newai = newarcinst(sch_wirearc, defaultarcwidth(sch_wirearc), bits,
			newni1, newpp1, x1, y1, newni2, newpp2, x2, y2, newcell);
		if (newai == NOARCINST) break;
		endobjectchange((INTBIG)newai, VARCINST);
	}
}

#define MAXADJUST 5

void us_tran_makemanhattan(NODEPROTO *newcell)
{
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER INTBIG e, count, i, j;
	REGISTER INTBIG dist, bestdist, bestx, besty, xp, yp;
	INTBIG x[MAXADJUST], y[MAXADJUST];

	/* adjust this cell */
	for(ni = newcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex == 0) continue;
		if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) != NPPIN) continue;

		/* see if this pin can be adjusted so that all wires are manhattan */
		count = 0;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->end[0].nodeinst == ni)
			{
				if (ai->end[1].nodeinst == ni) continue;
				e = 1;
			} else e = 0;
			x[count] = ai->end[e].xpos;   y[count] = ai->end[e].ypos;
			count++;
			if (count >= MAXADJUST) break;
		}
		if (count == 0) continue;

		/* now adjust for all these points */
		xp = (ni->lowx + ni->highx) / 2;   yp = (ni->lowy + ni->highy) / 2;
		bestdist = MAXINTBIG;
		for(i=0; i<count; i++) for(j=0; j<count; j++)
		{
			dist = abs(xp - x[i]) + abs(yp - y[j]);
			if (dist > bestdist) continue;
			bestdist = dist;
			bestx = x[i];   besty = y[j];
		}

		/* if there was a better place, move the node */
		if (bestdist != MAXINTBIG)
			modifynodeinst(ni, bestx-xp, besty-yp, bestx-xp, besty-yp, 0, 0);
	}
}

/* find the logical portproto corresponding to the mos portproto of ni */
PORTPROTO *us_tranconvpp(NODEINST *ni, PORTPROTO *mospp)
{
	PORTPROTO *schempp, *pp;
	NODEINST *schemni;
	INTBIG port;

	schemni = (NODEINST *)ni->temp1;

	switch (us_tranismos(schemni))
	{
		case TRAN_PIN:
			schempp = schemni->proto->firstportproto;
			break;
		case TRAN_CELL:
			schempp = getportproto(schemni->proto, mospp->protoname);
			break;
		default: /* transistor */
			for(port = 1, pp = ni->proto->firstportproto; pp != NOPORTPROTO;
				pp = pp->nextportproto, port++)
					if (pp == mospp) break;	 /* partic. port in MOS */
			schempp = us_trangetproto(schemni, port);
			break;
	}
	return(schempp);
}

/*
 * this routine figures out if the current nodeinst is a MOS component
 * (a wire or transistor).  If it's a transistor, return corresponding
 * define from efunction.h; if it's a passive connector, return TRAN_PIN;
 * if it's a cell, return TRAN_CELL; else return NPUNKNOWN.
 */
INTBIG us_tranismos(NODEINST *ni)
{
	INTBIG fun;

	if (ni->proto->primindex == 0) return(TRAN_CELL);
	fun = (ni->proto->userbits & NFUNCTION) >> NFUNCTIONSH;
	switch(fun)
	{
		case NPTRANMOS:   case NPTRA4NMOS:
		case NPTRADMOS:   case NPTRA4DMOS:
		case NPTRAPMOS:   case NPTRA4PMOS:
		case NPTRANPN:    case NPTRA4NPN:
		case NPTRAPNP:    case NPTRA4PNP:
		case NPTRANJFET:  case NPTRA4NJFET:
		case NPTRAPJFET:  case NPTRA4PJFET:
		case NPTRADMES:   case NPTRA4DMES:
		case NPTRAEMES:   case NPTRA4EMES:
		case NPTRANSREF:
			return(fun);
		case NPPIN:
		case NPCONTACT:
		case NPNODE:
		case NPCONNECT:
		case NPSUBSTRATE:
		case NPWELL:
			return(TRAN_PIN);
	}
	return(NPUNKNOWN);
}

PORTPROTO *us_trangetproto(NODEINST *ni, INTBIG port)
{
	PORTPROTO *pp;
	INTBIG count = port, i;

	if (count == 4) count = 3; else
		if (count == 3) count = 1;

	for(i = 1, pp = ni->proto->firstportproto; pp != NOPORTPROTO && i < count;
		pp = pp->nextportproto, i++) /* get portproto for schematic */
				;
	return(pp);
}

/************************* CODE FOR CONVERSION TO LAYOUT *************************/

/*
 * routine to recursively descend from cell "oldcell" and find subcells that
 * have to be converted.  When all subcells have been converted, convert this
 * one into a new one called "newcellname".  The technology for the old cell
 * is "oldtech" and the technology to use for the new cell is "newtech".  The
 * old view type is "oldview" and the new view type is "nview".
 */
NODEPROTO *us_tran_makelayoutcells(NODEPROTO *oldcell, CHAR *newcellname,
	TECHNOLOGY *oldtech, TECHNOLOGY *newtech, VIEW *nview)
{
	REGISTER NODEPROTO *newcell, *rnp;
	REGISTER INTBIG bits;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	CHAR *str;

	/* first convert the sub-cells */
	for(ni = oldcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore primitives */
		if (ni->proto->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ni->proto, oldcell)) continue;

		/* ignore cells with associations */
		FOR_CELLGROUP(rnp, ni->proto)
			if (rnp->cellview == nview) break;
		if (rnp != NONODEPROTO) continue;

		/* make up a name for this cell */
		(void)allocstring(&str, ni->proto->protoname, el_tempcluster);

		(void)us_tran_makelayoutcells(ni->proto, str, oldtech, newtech, nview);
		efree(str);
	}

	/* create the cell and fill it with parts */
	newcell = us_tran_linkage(newcellname, nview, oldcell);
	if (newcell == NONODEPROTO) return(NONODEPROTO);
	if (us_tran_makelayoutparts(oldcell, newcell, oldtech, newtech, nview))
	{
		/* adjust for maximum Manhattan-ness */
		us_tran_makemanhattan(newcell);

		/* reset shrinkage values and constraints to defaults (is this needed? !!!) */
		for(ai = newcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			bits = us_makearcuserbits(ai->proto);
			if ((bits&FIXED) != 0) ai->userbits |= FIXED;
			if ((bits&FIXANG) != 0) ai->userbits |= FIXANG;
			(void)setshrinkvalue(ai, FALSE);
		}
	}

	return(newcell);
}

/*
 * routine to create a new cell in "newcell" from the contents of an old cell
 * in "oldcell".  The technology for the old cell is "oldtech" and the
 * technology to use for the new cell is "newtech".
 */
BOOLEAN us_tran_makelayoutparts(NODEPROTO *oldcell, NODEPROTO *newcell,
	TECHNOLOGY *oldtech, TECHNOLOGY *newtech, VIEW *nview)
{
	REGISTER NODEPROTO *newnp;
	REGISTER NODEINST *ni, *end1, *end2;
	ARCPROTO *ap, *newap;
	ARCINST *ai;
	REGISTER PORTPROTO *mospp1, *mospp2, *schempp1, *schempp2;
	INTBIG x1, y1, x2, y2, lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2, tx1, ty1, tx2, ty2;
	REGISTER INTBIG newwid, newbits, oldlambda, newlambda, defwid, curwid;
	REGISTER INTBIG badarcs, i, j;
	REGISTER BOOLEAN univarcs;
	static POLYGON *poly1 = NOPOLYGON, *poly2 = NOPOLYGON;

	/* get a polygon */
	(void)needstaticpolygon(&poly1, 4, us_tool->cluster);
	(void)needstaticpolygon(&poly2, 4, us_tool->cluster);

	/* get lambda values */
	oldlambda = el_curlib->lambda[oldtech->techindex];
	newlambda = el_curlib->lambda[newtech->techindex];

	/* first convert the nodes */
	for(ni = oldcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(ni = oldcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* handle sub-cells */
		if (ni->proto->primindex == 0)
		{
			FOR_CELLGROUP(newnp, ni->proto)
				if (newnp->cellview == nview) break;
			if (newnp == NONODEPROTO)
			{
				ttyputerr(_("No equivalent cell for %s"), describenodeproto(ni->proto));
				continue;
			}
			us_tranplacenode(ni, newnp, newcell, oldtech, newtech);
			continue;
		}

		/* handle primitives */
		if (ni->proto == gen_cellcenterprim) continue;
		newnp = us_figurenewnproto(ni, newtech);
		us_tranplacenode(ni, newnp, newcell, oldtech, newtech);
	}

	/*
	 * for each arc in cell, find the ends in the new technology, and
	 * make a new arc to connect them in the new cell
	 */
	badarcs = 0;
	univarcs = FALSE;
	for(ai = oldcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* get the nodes and ports on the two ends of the arc */
		end1 = (NODEINST *)ai->end[0].nodeinst->temp1;
		end2 = (NODEINST *)ai->end[1].nodeinst->temp1;
		if (end1 == 0 || end2 == 0) continue;
		mospp1 = ai->end[0].portarcinst->proto;
		mospp2 = ai->end[1].portarcinst->proto;
		schempp1 = us_convport(ai->end[0].nodeinst, end1, mospp1);
		schempp2 = us_convport(ai->end[1].nodeinst, end2, mospp2);

		/* set bits in arc prototypes that can make the connection */
		for(ap = newtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			ap->userbits &= ~CANCONNECT;
		for(i=0; schempp1->connects[i] != NOARCPROTO; i++)
		{
			for(j=0; schempp2->connects[j] != NOARCPROTO; j++)
			{
				if (schempp1->connects[i] != schempp2->connects[j]) continue;
				schempp1->connects[i]->userbits |= CANCONNECT;
				break;
			}
		}

		/* compute arc type and see if it is acceptable */
		newap = us_figurenewaproto(ai->proto, newtech);
		if (newap->tech == newtech && (newap->userbits&CANCONNECT) == 0)
		{
			/* not acceptable: see if there are any valid ones */
			for(newap = newtech->firstarcproto; newap != NOARCPROTO; newap = newap->nextarcproto)
				if ((newap->userbits&CANCONNECT) != 0) break;

			/* none are valid: use universal */
			if (newap == NOARCPROTO) newap = gen_universalarc;
		}

		/* determine new arc width */
		newbits = ai->userbits;
		if (newap == gen_universalarc)
		{
			newwid = 0;
			univarcs = TRUE;
			newbits &= ~(FIXED | FIXANG);
		} else
		{
			defwid = ai->proto->nominalwidth - arcprotowidthoffset(ai->proto);
			curwid = ai->width - arcwidthoffset(ai);
			newwid = muldiv(newap->nominalwidth - arcprotowidthoffset(newap), curwid, defwid) +
				arcprotowidthoffset(newap);
			if (newwid <= 0) newwid = defaultarcwidth(newap);
		}

		/* find the endpoints of the arc */
		x1 = muldiv(ai->end[0].xpos, newlambda, oldlambda);
		y1 = muldiv(ai->end[0].ypos, newlambda, oldlambda);
		shapeportpoly(end1, schempp1, poly1, FALSE);
		x2 = muldiv(ai->end[1].xpos, newlambda, oldlambda);
		y2 = muldiv(ai->end[1].ypos, newlambda, oldlambda);
		shapeportpoly(end2, schempp2, poly2, FALSE);

		/* see if the new arc can connect without end adjustment */
		if (!isinside(x1, y1, poly1) || !isinside(x2, y2, poly2))
		{
			/* arc cannot be run exactly ... presume port centers */
			portposition(end1, schempp1, &x1, &y1);
			portposition(end2, schempp2, &x2, &y2);
			if ((newbits & FIXANG) != 0)
			{
				/* old arc was fixed-angle so look for a similar-angle path */
				reduceportpoly(poly1, end1, schempp1, newwid-arcprotowidthoffset(newap), -1);
				getbbox(poly1, &lx1, &hx1, &ly1, &hy1);
				reduceportpoly(poly2, end2, schempp2, newwid-arcprotowidthoffset(newap), -1);
				getbbox(poly2, &lx2, &hx2, &ly2, &hy2);
				if (!arcconnects(((ai->userbits&AANGLE) >> AANGLESH) * 10, lx1, hx1, ly1, hy1,
					lx2, hx2, ly2, hy2, &tx1, &ty1, &tx2, &ty2)) badarcs++; else
				{
					x1 = tx1;   y1 = ty1;
					x2 = tx2;   y2 = ty2;
				}
			}
		}
		/* create the new arc */
		if (newarcinst(newap, newwid, newbits, end1, schempp1, x1, y1,
			end2, schempp2, x2, y2, newcell) == NOARCINST)
		{
			ttyputmsg(_("Cell %s: can't run arc from node %s port %s at (%s,%s)"),
				describenodeproto(newcell), describenodeinst(end1),
					schempp1->protoname, latoa(x1, 0), latoa(y1, 0));
			ttyputmsg(_("   to node %s port %s at (%s,%s)"), describenodeinst(end2),
				schempp2->protoname, latoa(x2, 0), latoa(y2, 0));
		}
	}

	/* print warning if arcs were made nonmanhattan */
	if (badarcs != 0)
		ttyputmsg(_("WARNING: %ld %s made not-fixed-angle in cell %s"), badarcs,
			makeplural(_("arc"), badarcs), describenodeproto(newcell));
	return(univarcs);
}

void us_tranplacenode(NODEINST *ni, NODEPROTO *newnp, NODEPROTO *newcell,
	TECHNOLOGY *oldtech, TECHNOLOGY *newtech)
{
	INTBIG lx, ly, hx, hy, nlx, nly, nhx, nhy, bx, by, length, width;
	REGISTER INTBIG i, len, newsx, newsy, x1, y1, newlx, newhx, newly, newhy, *newtrace,
		oldlambda, newlambda, thissizex, thissizey, defsizex, defsizey;
	XARRAY trans;
	REGISTER INTSML trn;
	REGISTER PORTEXPINST *pexp;
	REGISTER NODEINST *newni;
	REGISTER PORTPROTO *pp, *pp2;
	REGISTER VARIABLE *var;

	oldlambda = el_curlib->lambda[oldtech->techindex];
	newlambda = el_curlib->lambda[newtech->techindex];

	/* scale edge offsets if this is a primitive */
	trn = ni->transpose;
	if (ni->proto->primindex != 0)
	{
		/* get offsets for new node type */
		nodeprotosizeoffset(newnp, &nlx, &nly, &nhx, &nhy, NONODEPROTO);

		/* special case for schematic transistors: get size from description */
		if (((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH) == NPTRANS)
		{
			transistorsize(ni, &length, &width);
			if (length < 0) length = newnp->highy - newnp->lowy - nly - nhy;
			if (width < 0) width = newnp->highx - newnp->lowx - nlx - nhx;
			lx = (ni->lowx + ni->highx - width) / 2;
			hx = (ni->lowx + ni->highx + width) / 2;
			ly = (ni->lowy + ni->highy - length) / 2;
			hy = (ni->lowy + ni->highy + length) / 2;
			trn = 1 - trn;

			/* compute scaled size for new node */
			newsx = muldiv(hx - lx, newlambda, oldlambda);
			newsy = muldiv(hy - ly, newlambda, oldlambda);
		} else
		{
			/* determine this node's percentage of the default node's size */
			nodeprotosizeoffset(ni->proto, &lx, &ly, &hx, &hy, NONODEPROTO);
			defsizex = (ni->proto->highx - hx) - (ni->proto->lowx + lx);
			defsizey = (ni->proto->highy - hy) - (ni->proto->lowy + ly);

			nodesizeoffset(ni, &lx, &ly, &hx, &hy);
			thissizex = (ni->highx - hx) - (ni->lowx + lx);
			thissizey = (ni->highy - hy) - (ni->lowy + ly);

			/* compute size of new node that is the same percentage of its default */
			newsx = muldiv((newnp->highx - nhx) - (newnp->lowx + nlx), thissizex, defsizex);
			newsy = muldiv((newnp->highy - nhy) - (newnp->lowy + nly), thissizey, defsizey);

			/* determine location of new node */
			lx = ni->lowx + lx;   hx = ni->highx - hx;
			ly = ni->lowy + ly;   hy = ni->highy - hy;
		}

		/* compute center of old node */
		x1 = muldiv((hx + lx) / 2, newlambda, oldlambda);
		y1 = muldiv((hy + ly) / 2, newlambda, oldlambda);

		/* compute bounds of the new node */
		newlx = x1 - newsx/2 - nlx;   newhx = newlx + newsx + nlx + nhx;
		newly = y1 - newsy/2 - nly;   newhy = newly + newsy + nly + nhy;
	} else
	{
		x1 = (newnp->highx+newnp->lowx)/2 - (ni->proto->highx+ni->proto->lowx)/2;
		y1 = (newnp->highy+newnp->lowy)/2 - (ni->proto->highy+ni->proto->lowy)/2;
		makeangle(ni->rotation, ni->transpose, trans);
		xform(x1, y1, &bx, &by, trans);
		newlx = ni->lowx + bx;   newhx = ni->highx + bx;
		newly = ni->lowy + by;   newhy = ni->highy + by;
		newlx += ((newhx-newlx) - (newnp->highx-newnp->lowx)) / 2;
		newhx = newlx + newnp->highx - newnp->lowx;
		newly += ((newhy-newly) - (newnp->highy-newnp->lowy)) / 2;
		newhy = newly + newnp->highy - newnp->lowy;
	}

	/* create the node */
	newni = newnodeinst(newnp, newlx, newhx, newly, newhy, trn, ni->rotation, newcell);
	if (newni == NONODEINST) return;
	newni->userbits |= (ni->userbits & (NEXPAND | WIPED | NSHORT));
	ni->temp1 = (INTBIG)newni;
	(void)copyvars((INTBIG)ni, VNODEINST, (INTBIG)newni, VNODEINST, FALSE);

	/* copy "trace" information if there is any */
	var = gettrace(ni);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		newtrace = emalloc((len * SIZEOFINTBIG), el_tempcluster);
		if (newtrace == 0) return;
		for(i=0; i<len; i++)
			newtrace[i] = muldiv(((INTBIG *)var->addr)[i], newlambda, oldlambda);
		(void)setvalkey((INTBIG)newni, VNODEINST, el_trace_key, (INTBIG)newtrace,
			VINTEGER|VISARRAY|(len<<VLENGTHSH));
		efree((CHAR *)newtrace);
	}
	endobjectchange((INTBIG)newni, VNODEINST);

	/* re-export any ports on the node */
	for(pexp = ni->firstportexpinst; pexp != NOPORTEXPINST; pexp = pexp->nextportexpinst)
	{
		pp = us_convport(ni, newni, pexp->proto);
		pp2 = newportproto(newcell, newni, pp, pexp->exportproto->protoname);
		if (pp2 == NOPORTPROTO) return;
		pp2->userbits = (pp2->userbits & ~STATEBITS) | (pexp->exportproto->userbits & STATEBITS);
		TDCOPY(pp2->textdescript, pexp->exportproto->textdescript);
		if (copyvars((INTBIG)pexp->exportproto, VPORTPROTO, (INTBIG)pp2, VPORTPROTO, FALSE))
			return;
	}
}

/*
 * routine to determine the port to use on node "newni" assuming that it should
 * be the same as port "oldpp" on equivalent node "ni"
 */
PORTPROTO *us_convport(NODEINST *ni, NODEINST *newni, PORTPROTO *oldpp)
{
	REGISTER PORTPROTO *pp, *npp;
	REGISTER INTBIG oldfun, newfun;

	if (newni->proto->primindex == 0)
	{
		/* cells can associate by comparing names */
		pp = getportproto(newni->proto, oldpp->protoname);
		if (pp != NOPORTPROTO) return(pp);
	}

	/* if functions are different, handle some special cases */
	oldfun = (ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
	newfun = (newni->proto->userbits&NFUNCTION) >> NFUNCTIONSH;
	if (oldfun != newfun)
	{
		if (oldfun == NPTRANS && isfet(newni->geom))
		{
			/* converting from stick-figure to layout */
			pp = ni->proto->firstportproto;   npp = newni->proto->firstportproto;
			if (pp == oldpp) return(npp);
			pp = pp->nextportproto;           npp = npp->nextportproto;
			if (pp == oldpp) return(npp);
			pp = pp->nextportproto;           npp = npp->nextportproto->nextportproto;
			if (pp == oldpp) return(npp);
		}
	}

	/* associate by position in port list */
	for(pp = ni->proto->firstportproto, npp = newni->proto->firstportproto;
		pp != NOPORTPROTO && npp != NOPORTPROTO;
			pp = pp->nextportproto, npp = npp->nextportproto)
				if (pp == oldpp) return(npp);

	/* special case again: one-port capacitors are OK */
	if (oldfun == NPCAPAC && newfun == NPCAPAC) return(newni->proto->firstportproto);

	/* association has failed: assume the first port */
	ttyputmsg(_("No port association between %s, port %s and %s"),
		describenodeproto(ni->proto), oldpp->protoname,
			describenodeproto(newni->proto));
	return(newni->proto->firstportproto);
}

/*
 * routine to determine the equivalent prototype in technology "newtech" for
 * node prototype "oldnp".
 */
ARCPROTO *us_figurenewaproto(ARCPROTO *oldap, TECHNOLOGY *newtech)
{
	REGISTER INTBIG type;
	REGISTER ARCPROTO *ap;

	/* schematic wires become universal arcs */
	if (oldap == sch_wirearc) return(gen_universalarc);

	/* determine the proper association of this node */
	type = (oldap->userbits & AFUNCTION) >> AFUNCTIONSH;
	for(ap = newtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if ((INTBIG)((ap->userbits&AFUNCTION) >> AFUNCTIONSH) == type) break;
	if (ap == NOARCPROTO)
	{
		ttyputmsg(_("No equivalent arc for %s"), describearcproto(oldap));
		return(oldap);
	}
	return(ap);
}

/*
 * routine to determine the equivalent prototype in technology "newtech" for
 * node prototype "oldnp".
 */
NODEPROTO *us_figurenewnproto(NODEINST *oldni, TECHNOLOGY *newtech)
{
	REGISTER INTBIG type, i, j, k;
	REGISTER ARCPROTO *ap, *oap;
	REGISTER INTBIG important, funct;
	REGISTER NODEPROTO *np, *rnp, *oldnp;
	REGISTER NODEINST *ni;
	NODEINST node;
	static POLYGON *poly = NOPOLYGON;

	/* get a polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* easy translation if complex or already in the proper technology */
	oldnp = oldni->proto;
	if (oldnp->primindex == 0 || oldnp->tech == newtech) return(oldnp);

	/* if this is a layer node, check the layer functions */
	type = nodefunction(oldni);
	if (type == NPNODE)
	{
		/* get the polygon describing the first box of the old node */
		(void)nodepolys(oldni, 0, NOWINDOWPART);
		shapenodepoly(oldni, 0, poly);
		important = LFTYPE | LFPSEUDO | LFNONELEC;
		funct = layerfunction(oldnp->tech, poly->layer) & important;

		/* now search for that function in the other technology */
		for(np = newtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (((np->userbits&NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
			ni = &node;   initdummynode(ni);
			ni->proto = np;
			(void)nodepolys(ni, 0, NOWINDOWPART);
			shapenodepoly(ni, 0, poly);
			if ((layerfunction(newtech, poly->layer)&important) == funct)
				return(np);
		}
	}

	/* see if one node in the new technology has the same function */
	for(i = 0, np = newtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if ((INTBIG)((np->userbits&NFUNCTION) >> NFUNCTIONSH) == type)
	{
		rnp = np;   i++;
	}
	if (i == 1) return(rnp);

	/* if there are too many matches, determine which is proper from arcs */
	if (i > 1)
	{
		for(np = newtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if ((INTBIG)((np->userbits&NFUNCTION) >> NFUNCTIONSH) != type) continue;

			/* see if this node has equivalent arcs */
			for(j=0; oldnp->firstportproto->connects[j] != NOARCPROTO; j++)
			{
				oap = oldnp->firstportproto->connects[j];
				if (oap->tech == gen_tech) continue;

				for(k=0; np->firstportproto->connects[k] != NOARCPROTO; k++)
				{
					ap = np->firstportproto->connects[k];
					if (ap->tech == gen_tech) continue;
					if ((ap->userbits&AFUNCTION) == (oap->userbits&AFUNCTION)) break;
				}
				if (np->firstportproto->connects[k] == NOARCPROTO) break;
			}
			if (oldnp->firstportproto->connects[j] == NOARCPROTO) break;
		}
		if (np != NONODEPROTO)
		{
			rnp = np;
			i = 1;
		}
	}

	/* give up if it still cannot be determined */
	if (i != 1)
	{
		if (oldnp->temp1 == 0)
			ttyputmsg(_("Node %s (function %s) has no equivalent in the %s technology"),
				describenodeproto(oldnp), nodefunctionname(type, oldni),
					newtech->techname);
		oldnp->temp1 = 1;
		return(oldnp);
	}
	return(rnp);
}

/************************* CODE FOR PRINTING TECHNOLOGIES *************************/

extern LIST us_teclayer_functions[];
extern LIST us_tecarc_functions[];

#define MAXCOLS 10

void us_printtechnology(TECHNOLOGY *tech)
{
	FILE *f;
	CHAR *name, *fieldname, *colorsymbol, thefield[50], *truename, **fields[MAXCOLS];
	REGISTER CHAR **names, **colors, **styles, **cifs, **gdss,
		**funcs, **layers, **layersizes, **extensions, **angles, **wipes, **ports,
		**portsizes, **portangles, **connections;
	REGISTER INTBIG i, j, k, l, m, tot, base;
	REGISTER INTBIG saveunit;
	REGISTER INTBIG func, area, bits, lambda;
	INTBIG lx, hx, ly, hy;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *cifvar, *gdsvar, *funcvar, *var;
	GRAPHICS *gra;
	REGISTER TECH_ARCLAY *arclay;
	REGISTER TECH_NODES *nodestr;
	TECH_POLYGON *lay;
	NODEINST node;
	ARCINST arc;
	static POLYGON *poly = NOPOLYGON;
	REGISTER void *infstr;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	infstr = initinfstr();
	addstringtoinfstr(infstr, tech->techname);
	addstringtoinfstr(infstr, x_(".doc"));
	name = returninfstr(infstr);
	f = xcreate(name, el_filetypetext, _("Technology Documentation File"), &truename);
	if (f == NULL)
	{
		if (truename != 0) us_abortcommand(_("Cannot write %s"), truename);
		return;
	}
	ttyputverbose(M_("Writing: %s"), name);

	/****************************** dump layers ******************************/

	/* get layer variables */
	cifvar = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_cif_layer_names"));
	gdsvar = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_gds_layer_numbers"));
	funcvar = getval((INTBIG)tech, VTECHNOLOGY, VINTEGER|VISARRAY, x_("TECH_layer_function"));

	/* allocate space for all layer fields */
	names = (CHAR **)emalloc((tech->layercount+1) * (sizeof (CHAR *)), el_tempcluster);
	colors = (CHAR **)emalloc((tech->layercount+1) * (sizeof (CHAR *)), el_tempcluster);
	styles = (CHAR **)emalloc((tech->layercount+1) * (sizeof (CHAR *)), el_tempcluster);
	cifs = (CHAR **)emalloc((tech->layercount+1) * (sizeof (CHAR *)), el_tempcluster);
	gdss = (CHAR **)emalloc((tech->layercount+1) * (sizeof (CHAR *)), el_tempcluster);
	funcs = (CHAR **)emalloc((tech->layercount+1) * (sizeof (CHAR *)), el_tempcluster);
	if (names == 0 || colors == 0 || styles == 0 || cifs == 0 || gdss == 0 || funcs == 0)
		return;

	/* load the header */
	(void)allocstring(&names[0], x_("Layer"), el_tempcluster);
	(void)allocstring(&colors[0], x_("Color"), el_tempcluster);
	(void)allocstring(&styles[0], x_("Style"), el_tempcluster);
	(void)allocstring(&cifs[0], x_("CIF"), el_tempcluster);
	(void)allocstring(&gdss[0], x_("GDS"), el_tempcluster);
	(void)allocstring(&funcs[0], x_("Function"), el_tempcluster);

	/* compute each layer */
	for(i=0; i<tech->layercount; i++)
	{
		(void)allocstring(&names[i+1], layername(tech, i), el_tempcluster);

		gra = tech->layers[i];
		if (ecolorname(gra->col, &fieldname, &colorsymbol)) fieldname = x_("?");
		(void)allocstring(&colors[i+1], fieldname, el_tempcluster);

		if (el_curwindowpart == NOWINDOWPART) fieldname = x_("unkwn."); else
		{
			switch (gra->colstyle&(NATURE|INVISIBLE))
			{
				case SOLIDC:              fieldname = x_("solid");    break;
				case PATTERNED:           fieldname = x_("pat.");     break;
				case INVISIBLE|SOLIDC:    fieldname = x_("INVsol");   break;
				case INVISIBLE|PATTERNED: fieldname = x_("INVpat");   break;
			}
		}
		(void)allocstring(&styles[i+1], fieldname, el_tempcluster);

		if (cifvar == NOVARIABLE) fieldname = x_("---"); else
			fieldname = ((CHAR **)cifvar->addr)[i];
		(void)allocstring(&cifs[i+1], fieldname, el_tempcluster);

		if (gdsvar == NOVARIABLE) fieldname = x_("---"); else
			fieldname = ((CHAR **)gdsvar->addr)[i];
		(void)allocstring(&gdss[i+1], fieldname, el_tempcluster);

		if (funcvar == NOVARIABLE) fieldname = x_("---"); else
		{
			func = ((INTBIG *)funcvar->addr)[i];
			infstr = initinfstr();
			us_tecedaddfunstring(infstr, func);
			fieldname = returninfstr(infstr);
		}
		(void)allocstring(&funcs[i+1], fieldname, el_tempcluster);
	}

	/* write the layer information */
	fields[0] = names;   fields[1] = colors;   fields[2] = styles;
	fields[3] = cifs;    fields[4] = gdss;     fields[5] = funcs;
	us_dumpfields(fields, 6, tech->layercount+1, f, x_("LAYERS"));
	for(i=0; i<=tech->layercount; i++)
	{
		efree(names[i]);
		efree(colors[i]);
		efree(styles[i]);
		efree(cifs[i]);
		efree(gdss[i]);
		efree(funcs[i]);
	}
	efree((CHAR *)names);
	efree((CHAR *)colors);
	efree((CHAR *)styles);
	efree((CHAR *)cifs);
	efree((CHAR *)gdss);
	efree((CHAR *)funcs);

	/****************************** dump arcs ******************************/

	/* allocate space for all arc fields */
	ai = &arc;   initdummyarc(ai);
	ai->end[0].xpos = -2000;   ai->end[0].ypos = 0;
	ai->end[1].xpos = 2000;    ai->end[1].ypos = 0;
	ai->length = 4000;
	ai->userbits |= NOEXTEND;
	tot = 1;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ai->proto = ap;
		if (tech->arcpolys != 0) j = (*(tech->arcpolys))(ai, NOWINDOWPART); else
			j = tech->arcprotos[ap->arcindex]->laycount;
		tot += j;
	}
	names = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	layers = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	layersizes = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	extensions = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	angles = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	wipes = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	funcs = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	if (names == 0 || layers == 0 || layersizes == 0 || extensions == 0 || angles == 0 ||
		wipes == 0 || funcs == 0) return;

	/* load the header */
	(void)allocstring(&names[0], x_("Arc"), el_tempcluster);
	(void)allocstring(&layers[0], x_("Layer"), el_tempcluster);
	(void)allocstring(&layersizes[0], x_("Size"), el_tempcluster);
	(void)allocstring(&extensions[0], x_("Extend"), el_tempcluster);
	(void)allocstring(&angles[0], x_("Angle"), el_tempcluster);
	(void)allocstring(&wipes[0], x_("Wipes"), el_tempcluster);
	(void)allocstring(&funcs[0], x_("Function"), el_tempcluster);

	tot = 1;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		(void)allocstring(&names[tot], ap->protoname, el_tempcluster);

		var = getvalkey((INTBIG)ap, VARCPROTO, VINTEGER, us_arcstylekey);
		if (var != NOVARIABLE) bits = var->addr; else
			bits = ap->userbits;
		if ((bits&WANTNOEXTEND) == 0) fieldname = x_("yes"); else fieldname = x_("no");
		(void)allocstring(&extensions[tot], fieldname, el_tempcluster);
		(void)esnprintf(thefield, 50, x_("%ld"), (ap->userbits&AANGLEINC) >> AANGLEINCSH);
		(void)allocstring(&angles[tot], thefield, el_tempcluster);
		if ((ap->userbits&CANWIPE) == 0) fieldname = x_("no"); else fieldname = x_("yes");
		(void)allocstring(&wipes[tot], fieldname, el_tempcluster);
		func = (ap->userbits&AFUNCTION) >> AFUNCTIONSH;
		(void)allocstring(&funcs[tot], us_tecarc_functions[func].name, el_tempcluster);

		ai->proto = ap;
		if (tech->arcpolys != 0) j = (*(tech->arcpolys))(ai, NOWINDOWPART); else
			tech_oneprocpolyloop.realpolys = j = tech->arcprotos[ap->arcindex]->laycount;
		for(k=0; k<j; k++)
		{
			ai->width = defaultarcwidth(ap);
			if (tech->shapearcpoly != 0) (*(tech->shapearcpoly))(ai, k, poly); else
			{
				arclay = &tech->arcprotos[ap->arcindex]->list[k];
				makearcpoly(ai->length, defaultarcwidth(ap)-arclay->off*lambdaofarc(ai)/WHOLE,
					ai, poly, arclay->style);
				poly->layer = arclay->lay;
			}
			(void)allocstring(&layers[tot], layername(tech, poly->layer), el_tempcluster);
			area = (INTBIG)(fabs(areapoly(poly)) / 4000.0);
			saveunit = el_units & DISPLAYUNITS;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITMIC;
			(void)allocstring(&layersizes[tot], latoa(area, 0), el_tempcluster);
			el_units = (el_units & ~DISPLAYUNITS) | saveunit;
			if (k > 0)
			{
				(void)allocstring(&names[tot], x_(""), el_tempcluster);
				(void)allocstring(&extensions[tot], x_(""), el_tempcluster);
				(void)allocstring(&angles[tot], x_(""), el_tempcluster);
				(void)allocstring(&wipes[tot], x_(""), el_tempcluster);
				(void)allocstring(&funcs[tot], x_(""), el_tempcluster);
			}
			tot++;
		}
	}

	/* write the arc information */
	fields[0] = names;        fields[1] = layers;   fields[2] = layersizes;
	fields[3] = extensions;   fields[4] = angles;   fields[5] = wipes;
	fields[6] = funcs;
	us_dumpfields(fields, 7, tot, f, x_("ARCS"));
	for(i=0; i<tot; i++)
	{
		efree(names[i]);
		efree(layers[i]);
		efree(layersizes[i]);
		efree(extensions[i]);
		efree(angles[i]);
		efree(wipes[i]);
		efree(funcs[i]);
	}
	efree((CHAR *)names);
	efree((CHAR *)layers);
	efree((CHAR *)layersizes);
	efree((CHAR *)extensions);
	efree((CHAR *)angles);
	efree((CHAR *)wipes);
	efree((CHAR *)funcs);

	/****************************** dump nodes ******************************/

	/* allocate space for all node fields */
	ni = &node;   initdummynode(ni);
	tot = 1;
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		ni->proto = np;
		if (tech->nodepolys != 0) j = (*(tech->nodepolys))(ni, 0, NOWINDOWPART); else
			j = tech->nodeprotos[np->primindex-1]->layercount;
		l = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			m = 0;
			for(k=0; pp->connects[k] != NOARCPROTO; k++)
				if (pp->connects[k]->tech == tech) m++;
			if (m == 0) m = 1;
			l += m;
		}
		tot += maxi(j, l);
	}
	names = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	funcs = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	layers = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	layersizes = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	ports = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	portsizes = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	portangles = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	connections = (CHAR **)emalloc(tot * (sizeof (CHAR *)), el_tempcluster);
	if (names == 0 || funcs == 0 || layers == 0 || layersizes == 0 || ports == 0 ||
		portsizes == 0 || portangles == 0 || connections == 0) return;

	/* load the header */
	(void)allocstring(&names[0], x_("Node"), el_tempcluster);
	(void)allocstring(&funcs[0], x_("Function"), el_tempcluster);
	(void)allocstring(&layers[0], x_("Layers"), el_tempcluster);
	(void)allocstring(&layersizes[0], x_("Size"), el_tempcluster);
	(void)allocstring(&ports[0], x_("Ports"), el_tempcluster);
	(void)allocstring(&portsizes[0], x_("Size"), el_tempcluster);
	(void)allocstring(&portangles[0], x_("Angle"), el_tempcluster);
	(void)allocstring(&connections[0], x_("Connections"), el_tempcluster);

	tot = 1;
	for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		base = tot;
		(void)allocstring(&names[tot], np->protoname, el_tempcluster);

		func = (np->userbits&NFUNCTION) >> NFUNCTIONSH;
		(void)allocstring(&funcs[tot], nodefunctionname(func, NONODEINST), el_tempcluster);

		ni->proto = np;
		ni->lowx = np->lowx;   ni->highx = np->highx;
		ni->lowy = np->lowy;   ni->highy = np->highy;
		if (tech->nodepolys != 0) j = (*(tech->nodepolys))(ni, 0, NOWINDOWPART); else
			tech_oneprocpolyloop.realpolys = j = tech->nodeprotos[np->primindex-1]->layercount;
		for(k=0; k<j; k++)
		{
			if (tech->shapenodepoly != 0) (*(tech->shapenodepoly))(ni, k, poly); else
			{
				nodestr = tech->nodeprotos[np->primindex-1];
				lambda = el_curlib->lambda[tech->techindex];
				if (nodestr->special == SERPTRANS)
				{
					tech_filltrans(poly, &lay, nodestr->gra, ni,
						lambda, k, (TECH_PORTS *)0, &tech_oneprocpolyloop);
				} else
				{
					lay = &nodestr->layerlist[k];
					tech_fillpoly(poly, lay, ni, lambda, FILLED);
				}
			}
			(void)allocstring(&layers[tot], layername(tech, poly->layer), el_tempcluster);
			getbbox(poly, &lx, &hx, &ly, &hy);
			saveunit = el_units & DISPLAYUNITS;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITMIC;
			(void)esnprintf(thefield, 50, x_("%s x %s"), latoa(hx-lx, 0), latoa(hy-ly, 0));
			(void)allocstring(&layersizes[tot], thefield, el_tempcluster);
			el_units = (el_units & ~DISPLAYUNITS) | saveunit;
			if (k > 0)
			{
				(void)allocstring(&names[tot], x_(""), el_tempcluster);
				(void)allocstring(&funcs[tot], x_(""), el_tempcluster);
			}
			tot++;
		}
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			(void)allocstring(&ports[base], pp->protoname, el_tempcluster);
			shapeportpoly(ni, pp, poly, FALSE);
			getbbox(poly, &lx, &hx, &ly, &hy);
			saveunit = el_curtech->userbits & DISPLAYUNITS;
			el_units = (el_units & ~DISPLAYUNITS) | DISPUNITMIC;
			(void)esnprintf(thefield, 50, x_("%s x %s"), latoa(hx-lx, 0), latoa(hy-ly, 0));
			(void)allocstring(&portsizes[base], thefield, el_tempcluster);
			el_units = (el_units & ~DISPLAYUNITS) | saveunit;
			if (((pp->userbits&PORTARANGE) >> PORTARANGESH) == 180) (void)estrcpy(thefield, x_("")); else
				(void)esnprintf(thefield, 50, x_("%ld"), (pp->userbits&PORTANGLE) >> PORTANGLESH);
			(void)allocstring(&portangles[base], thefield, el_tempcluster);
			m = 0;
			for(k=0; pp->connects[k] != NOARCPROTO; k++)
			{
				if (pp->connects[k]->tech != tech) continue;
				(void)allocstring(&connections[base], pp->connects[k]->protoname, el_tempcluster);
				if (m != 0)
				{
					(void)allocstring(&ports[base], x_(""), el_tempcluster);
					(void)allocstring(&portsizes[base], x_(""), el_tempcluster);
					(void)allocstring(&portangles[base], x_(""), el_tempcluster);
				}
				m++;
				base++;
			}
			if (m == 0) (void)allocstring(&connections[base++], x_("<NONE>"), el_tempcluster);
		}
		for( ; base < tot; base++)
		{
			(void)allocstring(&ports[base], x_(""), el_tempcluster);
			(void)allocstring(&portsizes[base], x_(""), el_tempcluster);
			(void)allocstring(&portangles[base], x_(""), el_tempcluster);
			(void)allocstring(&connections[base], x_(""), el_tempcluster);
		}
		for( ; tot < base; tot++)
		{
			(void)allocstring(&names[tot], x_(""), el_tempcluster);
			(void)allocstring(&funcs[tot], x_(""), el_tempcluster);
			(void)allocstring(&layers[tot], x_(""), el_tempcluster);
			(void)allocstring(&layersizes[tot], x_(""), el_tempcluster);
		}
	}

	/* write the node information */
	fields[0] = names;        fields[1] = funcs;    fields[2] = layers;
	fields[3] = layersizes;   fields[4] = ports;    fields[5] = portsizes;
	fields[6] = portangles;   fields[7] = connections;
	us_dumpfields(fields, 8, tot, f, x_("NODES"));
	for(i=0; i<tot; i++)
	{
		efree(names[i]);
		efree(funcs[i]);
		efree(layers[i]);
		efree(layersizes[i]);
		efree(ports[i]);
		efree(portsizes[i]);
		efree(portangles[i]);
		efree(connections[i]);
	}
	efree((CHAR *)names);
	efree((CHAR *)funcs);
	efree((CHAR *)layers);
	efree((CHAR *)layersizes);
	efree((CHAR *)ports);
	efree((CHAR *)portsizes);
	efree((CHAR *)portangles);
	efree((CHAR *)connections);

	xclose(f);
}

void us_dumpfields(CHAR ***fields, INTBIG count, INTBIG length, FILE *f, CHAR *title)
{
	INTBIG widths[MAXCOLS];
	REGISTER INTBIG len, i, j, k, totwid, stars;

	totwid = 0;
	for(i=0; i<count; i++)
	{
		widths[i] = 8;
		for(j=0; j<length; j++)
		{
			len = estrlen(fields[i][j]);
			if (len > widths[i]) widths[i] = len;
		}
		widths[i]++;
		totwid += widths[i];
	}

	stars = (totwid - estrlen(title) - 2) / 2;
	for(i=0; i<stars; i++) xprintf(f, x_("*"));
	xprintf(f, x_(" %s "), title);
	for(i=0; i<stars; i++) xprintf(f, x_("*"));
	xprintf(f, x_("\n"));

	for(j=0; j<length; j++)
	{
		for(i=0; i<count; i++)
		{
			xprintf(f, x_("%s"), fields[i][j]);
			if (i == count-1) continue;
			for(k=estrlen(fields[i][j]); k<widths[i]; k++) xprintf(f, x_(" "));
		}
		xprintf(f, x_("\n"));
	}
	xprintf(f, x_("\n"));
}
