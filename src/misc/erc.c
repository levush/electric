/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: erc.c
 * Electrical Rules Checking tool
 * Written by: Steven M. Rubin
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
#if ERCTOOL

#include "global.h"
#include "efunction.h"
#include "egraphics.h"
#include "erc.h"
#include "usr.h"
#include "edialogs.h"

/* the ERC tool table */
static KEYWORD ercopt[] =
{
	{x_("well-check"),    0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("antenna-rules"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP erc_tablep = {ercopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Electrical Rules action"), M_("show defaults")};

/* well contacts */
#define NOWELLCON ((WELLCON *)-1)

typedef struct Iwellcon
{
	INTBIG           x, y;
	INTBIG           netnum;
	INTBIG           onproperrail;
	INTBIG           fun;
	NODEPROTO       *np;
	struct Iwellcon *nextwellcon;
} WELLCON;

static WELLCON    *erc_wellconfree = NOWELLCON;
static WELLCON    *erc_firstwellcon = NOWELLCON;
static INTBIG      erc_worstpwelldist;
static INTBIG      erc_worstpwellconx, erc_worstpwellcony;
static INTBIG      erc_worstpwelledgex, erc_worstpwelledgey;
static INTBIG      erc_worstnwelldist;
static INTBIG      erc_worstnwellconx, erc_worstnwellcony;
static INTBIG      erc_worstnwelledgex, erc_worstnwelledgey;
static void       *erc_progressdialog;

/* well areas */
#define NOWELLAREA ((WELLAREA *)-1)

typedef struct Iwellarea
{
	INTBIG            lx, hx, ly, hy;
	INTBIG            netnum;
	POLYGON           *poly;
	INTBIG            layer;
	TECHNOLOGY        *tech;
	NODEPROTO         *cell;
	struct Iwellarea  *nextwellarea;
} WELLAREA;

static WELLAREA *erc_wellareafree = NOWELLAREA;
static WELLAREA *erc_firstwellarea = NOWELLAREA;

/* miscellaneous */

#define DEFERCBITS      (PWELLONGROUND|NWELLONPOWER)

       TOOL       *erc_tool;					/* this tool */
static NODEPROTO  *erc_cell;
static INTBIG      erc_globalnetnumber;			/* global network number */
static INTBIG      erc_optionskey;				/* key for "ERC_options" */
static INTBIG      erc_options;					/* value of "ERC_options" */
static INTBIG      erc_jobsize;					/* number of polygons to analyze */
static INTBIG      erc_polyscrunched;			/* number of polygons analyzed */
static BOOLEAN     erc_realrun;					/* true if this is the real analysis */

/* prototypes for local routines */
static WELLAREA    *erc_allocwellarea(void);
static WELLCON     *erc_allocwellcon(void);
static void         erc_analyzecell(NODEPROTO *cell);
static void         erc_freeallwellareas(void);
static void         erc_freeallwellcons(void);
static void         erc_gatherwells(NODEPROTO *np, XARRAY trans, void *merge, BOOLEAN gathernet);
static void         erc_getpolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);
static void         erc_optionsdlog(void);
static INTBIG       erc_welllayer(TECHNOLOGY *tech, INTBIG layer);

/************************ CONTROL ***********************/

/*
 * tool initialization
 */
void erc_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	extern COMCOMP us_debugcp;

	/* only initialize during pass 1 */
	if (thistool == NOTOOL || thistool == 0) return;

	/* copy tool pointer during pass 1 */
	erc_tool = thistool;
	erc_optionskey = makekey(x_("ERC_options"));
	nextchangequiet();
	setvalkey((INTBIG)erc_tool, VTOOL, erc_optionskey, DEFERCBITS, VINTEGER|VDONTSAVE);
	DiaDeclareHook(x_("ercopt"), &erc_tablep, erc_optionsdlog);
	DiaDeclareHook(x_("ercantennaopt"), &us_debugcp, erc_antoptionsdlog);
	erc_antennaarcratiokey = makekey(x_("ERC_antenna_arc_ratio"));
}

void erc_done(void)
{
#ifdef DEBUGMEMORY
	erc_freeallwellcons();
	erc_freeallwellareas();
	erc_antterm();
#endif
}

/*
 * Handle commands to the tool from the user.
 */
void erc_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pp;

	if (count == 0)
	{
		ttyputusage(x_("telltool erc analyze"));
		return;
	}

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("well-check"), l) == 0 && l >= 3)
	{
		/* check well and substrate layers */
		np = getcurcell();
		erc_analyzecell(np);
		return;
	}

	if (namesamen(pp, x_("antenna-rules"), l) == 0 && l >= 3)
	{
		/* do antenna-rules check */
		np = getcurcell();
		erc_antcheckcell(np);
		return;
	}

	ttyputbadusage(x_("telltool erc"));
}

/******************** ANALYSIS ********************/

/*
 * Routine to analyze cell "cell" and build a list of ERC errors.
 */
void erc_analyzecell(NODEPROTO *cell)
{
	REGISTER INTBIG errors, minsep, dist, layertype, largestnetnum, welltype,
		desiredcontact, perareabit, lambda;
	REGISTER NODEPROTO *np;
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *lib;
	REGISTER BOOLEAN con;
	REGISTER CHAR *abortseq, *errormsg, *nocontacterror;
	CHAR errmsg[200];
	REGISTER WELLCON *wc, *owc;
	REGISTER VARIABLE *var;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER WELLAREA *wa, *owa;
	REGISTER void *err;
	float elapsed;
	INTBIG edge;
	REGISTER void *infstr;
	REGISTER void *polymerge, *submerge;

	initerrorlogging(_("ERC"));
	if (cell == NONODEPROTO)
	{
		termerrorlogging(TRUE);
		ttyputerr(_("Must be editing a cell before analyzing wells"));
		return;
	}

	/* make sure network tool is on */
	if ((net_tool->toolstate&TOOLON) == 0)
	{
		termerrorlogging(TRUE);
		ttyputerr(_("Network tool must be running...turning it on"));
		toolturnon(net_tool);
		return;
	}

	/* announce start of analysis */
	infstr = initinfstr();
	addstringtoinfstr(infstr, _("Checking Wells and Substrates..."));
	abortseq = getinterruptkey();
	if (abortseq != 0)
		formatinfstr(infstr, _(" (type %s to abort)"), abortseq);
	ttyputmsg(returninfstr(infstr));
	erc_progressdialog = DiaInitProgress(_("Preparing to analyze..."), 0);
	if (erc_progressdialog == 0)
	{
		termerrorlogging(TRUE);
		return;
	}
	DiaSetProgress(erc_progressdialog, 0, 1);

	/* begin counting elapsed time */
	starttimer();

	/* initialize to analyze cell */
	erc_freeallwellcons();
	erc_freeallwellareas();
	erc_worstpwelldist = erc_worstnwelldist = 0;
	erc_cell = cell;
	var = getvalkey((INTBIG)erc_tool, VTOOL, VINTEGER, erc_optionskey);
	if (var == NOVARIABLE) erc_options = DEFERCBITS; else
		erc_options = var->addr;

	/* put global network information on the ports of this cell */
	erc_globalnetnumber = 1;
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		pp->temp1 = 0;
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->temp1 != 0) continue;
		pp->temp1 = erc_globalnetnumber++;
		for(opp = pp->nextportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
		{
			if (opp->temp1 != 0) continue;
			if (opp->network == pp->network) opp->temp1 = pp->temp1;
		}
	}

	/* reset count of primitives and merging information */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = np->temp2 = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;

	/* run through the work and count the number of polygons */
	erc_polyscrunched = 0;
	erc_realrun = FALSE;
	erc_gatherwells(cell, el_matid, 0, FALSE);
	erc_jobsize = erc_polyscrunched;

	/* now gather all of the geometry into the polygon merging system */
	DiaSetTextProgress(erc_progressdialog, _("Gathering geometry..."));
	polymerge = mergenew(erc_tool->cluster);
	erc_realrun = TRUE;
	erc_polyscrunched = 0;
	erc_gatherwells(cell, el_matid, polymerge, TRUE);

	/* delete intermediate merge information */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp2 == 0) continue;
			submerge = (void *)np->temp2;
			mergedelete(submerge);
		}
	}

	/* determine the number of polygon points in the analysis */
	erc_polyscrunched = 0;
	erc_realrun = FALSE;
	mergeextract(polymerge, erc_getpolygon);
	erc_jobsize = erc_polyscrunched;

	/* do the analysis */
	DiaSetTextProgress(erc_progressdialog, _("Analyzing..."));
	erc_realrun = TRUE;
	erc_polyscrunched = 0;
	mergeextract(polymerge, erc_getpolygon);
	mergedelete(polymerge);
	DiaSetProgress(erc_progressdialog, 100, erc_jobsize);

	/* number the well areas according to topology of contacts in them */
	if (erc_firstwellcon == NOWELLCON) largestnetnum = 0; else
	{
		largestnetnum = erc_firstwellcon->netnum;
		for(wc = erc_firstwellcon; wc != NOWELLCON; wc = wc->nextwellcon)
			if (wc->netnum > largestnetnum)
				largestnetnum = wc->netnum;
	}
	for(wa = erc_firstwellarea; wa != NOWELLAREA; wa = wa->nextwellarea)
	{
		wa->netnum = ++largestnetnum;
		welltype = erc_welllayer(wa->tech, wa->layer);
		if (welltype != 1 && welltype != 2) continue;
		if (welltype == 1)
		{
			/* P-well */
			desiredcontact = NPWELL;
			perareabit = PWELLCONPERAREA;
			nocontacterror = _("No P-Well contact found in this area");
		} else
		{
			/* N-Well */
			desiredcontact = NPSUBSTRATE;
			perareabit = NWELLCONPERAREA;
			nocontacterror = _("No N-Well contact found in this area");
		}

		/* find a contact in the area */
		for(wc = erc_firstwellcon; wc != NOWELLCON; wc = wc->nextwellcon)
		{
			if (wc->fun != desiredcontact) continue;
			if (wc->x < wa->lx || wc->x > wa->hx ||
				wc->y < wa->ly || wc->y > wa->hy) continue;
			if (!isinside(wc->x, wc->y, wa->poly)) continue;
			wa->netnum = wc->netnum;
			break;
		}

		/* if no contact, issue appropriate errors */
		if (wc == NOWELLCON)
		{
			if ((erc_options&PWELLCONOPTIONS) == perareabit)
			{
				err = logerror(nocontacterror, erc_cell, 0);
				addpolytoerror(err, wa->poly);
			}
		}
	}

	/* make sure all of the contacts are on the same net */
	DiaSetTextProgress(erc_progressdialog, _("Reporting errors..."));
	for(wc = erc_firstwellcon; wc != NOWELLCON; wc = wc->nextwellcon)
	{
		if (wc->netnum == 0)
		{
			if (wc->fun == NPWELL) errormsg = _("P-Well contact is floating"); else
				errormsg = _("N-Well contact is floating");
			err = logerror(errormsg, erc_cell, 0);
			addpointtoerror(err, wc->x, wc->y);
			continue;
		}
		if (wc->onproperrail == 0)
		{
			if (wc->fun == NPWELL)
			{
				if ((erc_options&PWELLONGROUND) != 0)
				{
					err = logerror(_("P-Well contact not connected to ground"), erc_cell, 0);
					addpointtoerror(err, wc->x, wc->y);
				}
			} else
			{
				if ((erc_options&NWELLONPOWER) != 0)
				{
					err = logerror(_("N-Well contact not connected to power"), erc_cell, 0);
					addpointtoerror(err, wc->x, wc->y);
				}
			}
		}
		for(owc = wc->nextwellcon; owc != NOWELLCON; owc = owc->nextwellcon)
		{
			if (owc->netnum == 0) continue;
			if (owc->fun != wc->fun) continue;
			if (owc->netnum == wc->netnum) continue;
			if (wc->fun == NPWELL) errormsg = _("P-Well contacts are not connected"); else
				errormsg = _("N-Well contacts are not connected");
			err = logerror(errormsg, erc_cell, 0);
			addpointtoerror(err, wc->x, wc->y);
			addpointtoerror(err, owc->x, owc->y);
			break;
		}
	}

	/* if just 1 N-Well contact is needed, see if it is there */
	if ((erc_options&NWELLCONOPTIONS) == NWELLCONONCE)
	{
		for(wc = erc_firstwellcon; wc != NOWELLCON; wc = wc->nextwellcon)
			if (wc->fun == NPSUBSTRATE) break;
		if (wc == NOWELLCON)
		{
			(void)logerror(_("No N-Well contact found in this cell"), erc_cell, 0);
		}
	}

	/* if just 1 P-Well contact is needed, see if it is there */
	if ((erc_options&PWELLCONOPTIONS) == PWELLCONONCE)
	{
		for(wc = erc_firstwellcon; wc != NOWELLCON; wc = wc->nextwellcon)
			if (wc->fun == NPWELL) break;
		if (wc == NOWELLCON)
		{
			(void)logerror(_("No P-well contact found in this cell"), erc_cell, 0);
		}
	}

	/* make sure the wells are separated properly */
	for(wa = erc_firstwellarea; wa != NOWELLAREA; wa = wa->nextwellarea)
	{
		for(owa = wa->nextwellarea; owa != NOWELLAREA; owa = owa->nextwellarea)
		{
			if (wa->tech != owa->tech || wa->layer != owa->layer) continue;
			if (wa->netnum == owa->netnum && wa->netnum >= 0) con = TRUE; else
				con = FALSE;
			minsep = drcmindistance(wa->tech, cell->lib, wa->layer, 0, wa->layer, 0,
				con, FALSE, &edge, 0);
			if (minsep < 0) continue;
			if (wa->lx > owa->hx+minsep || owa->lx > wa->hx+minsep ||
				wa->ly > owa->hy+minsep || owa->ly > wa->hy+minsep) continue;
			dist = polyseparation(wa->poly, owa->poly);
			if (dist < minsep)
			{
				layertype = erc_welllayer(wa->tech, wa->layer);
				if (layertype == 0) continue;
				lambda = lambdaofcell(cell);
				switch (layertype)
				{
					case 1:
						esnprintf(errmsg, 200, _("P-Well areas too close (are %s, should be %s)"),
							latoa(dist, lambda), latoa(minsep, lambda));
						break;
					case 2:
						esnprintf(errmsg, 200, _("N-Well areas too close (are %s, should be %s)"),
							latoa(dist, lambda), latoa(minsep, lambda));
						break;
					case 3:
						esnprintf(errmsg, 200, _("P-Select areas too close (are %s, should be %s)"),
							latoa(dist, lambda), latoa(minsep, lambda));
						break;
					case 4:
						esnprintf(errmsg, 200, _("N-Select areas too close (are %s, should be %s)"),
							latoa(dist, lambda), latoa(minsep, lambda));
						break;
				}
				err = logerror(errmsg, erc_cell, 0);
				addpolytoerror(err, wa->poly);
				addpolytoerror(err, owa->poly);
			}
		}
	}

	/* show the farthest distance from a well contact */
	if (erc_worstpwelldist > 0 || erc_worstnwelldist > 0)
	{
		(void)asktool(us_tool, x_("clear"));
		lambda = lambdaofcell(cell);
		if (erc_worstpwelldist > 0)
		{
			(void)asktool(us_tool, x_("show-line"), erc_worstpwellconx, erc_worstpwellcony,
				erc_worstpwelledgex, erc_worstpwelledgey, erc_cell);
			ttyputmsg(_("Farthest distance from a P-Well contact is %s"),
				latoa(erc_worstpwelldist, lambda));
		}
		if (erc_worstnwelldist > 0)
		{
			(void)asktool(us_tool, x_("show-line"), erc_worstnwellconx, erc_worstnwellcony,
				erc_worstnwelledgex, erc_worstnwelledgey, erc_cell);
			ttyputmsg(_("Farthest distance from a N-Well contact is %s"),
				latoa(erc_worstnwelldist, lambda));
		}
	}

	/* report the number of transistors */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 == 0) continue;
			switch ((np->userbits&NFUNCTION) >> NFUNCTIONSH)
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
				case NPTRANS:
					ttyputmsg(_("%ld nodes of type %s"), np->temp1, describenodeproto(np));
					break;
			}
		}
	}

	/* report the number of networks */
	DiaDoneProgress(erc_progressdialog);
	ttyputmsg(_("%ld networks found"), erc_globalnetnumber);

	/* stop counting time */
	elapsed = endtimer();
	if (elapsed > 60.0 && (us_useroptions&BEEPAFTERLONGJOB) != 0)
		ttybeep(SOUNDBEEP, TRUE);

	/* report the number of errors found */
	errors = numerrors();
	if (errors == 0)
	{
		ttyputmsg(_("No Well errors found (%s)"), explainduration(elapsed));
	} else
	{
		ttyputmsg(_("FOUND %ld WELL %s (%s)"), errors, makeplural(_("ERROR"), errors),
			explainduration(elapsed));
	}
	termerrorlogging(TRUE);
}

/*
 * Recursive helper routine to gather all well areas in cell "np"
 * (transformed by "trans").  If "merge" is nonzero, merge geometry into it.
 * If "gathernet" is true, follow networks, looking for special primitives and nets. 
 */
void erc_gatherwells(NODEPROTO *np, XARRAY trans, void *merge, BOOLEAN gathernet)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER NETWORK *net;
	REGISTER GEOM *geom;
	REGISTER INTBIG i, tot, nfun, updatepct, sea;
	static INTBIG checkstop = 0;
	XARRAY localrot, localtran, final, subrot;
	static POLYGON *poly = NOPOLYGON;
	REGISTER WELLCON *wc;

	(void)needstaticpolygon(&poly, 4, erc_tool->cluster);

	/* grab global network information from the ports */
	if (erc_realrun)
	{
		if (gathernet)
		{
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				net->temp1 = 0;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->network->temp1 = pp->temp1;
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			{
				if (net->temp1 == 0) net->temp1 = erc_globalnetnumber++;
			}
		}
	}

	/* do a spatial search so that polygon merging grows sensibly */
	updatepct = 0;
	sea = initsearch(np->lowx, np->highx, np->lowy, np->highy, np);
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		checkstop++;
		if ((checkstop%10) == 0)
		{
			if (stopping(STOPREASONERC)) { termsearch(sea);   return; }
		}
		if (!geom->entryisnode)
		{
			ai = geom->entryaddr.ai;
			tot = arcpolys(ai, NOWINDOWPART);
			erc_polyscrunched += tot;
			if (erc_realrun && merge != 0)
			{
				if (erc_jobsize > 0)
				{
					if (updatepct++ > 25)
					{
						DiaSetProgress(erc_progressdialog, erc_polyscrunched, erc_jobsize);
						updatepct = 0;
					}
				}
				for(i=0; i<tot; i++)
				{
					shapearcpoly(ai, i, poly);
					if (erc_welllayer(poly->tech, poly->layer) == 0) continue;
					xformpoly(poly, trans);
					mergeaddpolygon(merge, poly->layer, poly->tech, poly);
				}
			}
			continue;
		}

		ni = geom->entryaddr.ni;
		subnp = ni->proto;
		makerot(ni, localrot);
		transmult(localrot, trans, final);
		if (subnp->primindex == 0)
		{
			if (erc_realrun && gathernet)
			{
				if (stopping(STOPREASONERC)) { termsearch(sea);   return; }

				/* propagate global network numbers into this instance */
				for(pp = subnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					pp->temp1 = 0;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					pi->proto->temp1 = pi->conarcinst->network->temp1;
				for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					pe->proto->temp1 = pe->exportproto->network->temp1;
				for(pp = subnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				{
					if (pp->temp1 != 0) continue;
					for(opp = subnp->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
						if (opp != pp && opp->temp1 != 0 && opp->network == pp->network) break;
					if (opp != NOPORTPROTO) pp->temp1 = opp->temp1; else
						pp->temp1 = erc_globalnetnumber++;
				}
			}
			maketrans(ni, localtran);
			transmult(localtran, final, subrot);

			/* recursive polygon merge is faster */
			if (merge != 0)
			{
				void *submerge;

				submerge = (void *)subnp->temp2;
				if (submerge == 0)
				{
					/* gather the subcell's merged information */
					if (erc_realrun) submerge = mergenew(erc_tool->cluster); else
						submerge = (void *)1;
					erc_gatherwells(subnp, el_matid, submerge, FALSE);
					subnp->temp2 = (INTBIG)submerge;
				}

				if (erc_realrun)
					mergeaddmerge(merge, submerge, subrot);
			}
			if (gathernet)
				erc_gatherwells(subnp, subrot, 0, TRUE);
		} else
		{
			if (erc_realrun && gathernet)
			{
				nfun = nodefunction(ni);
				
				/* gather count of nodes found */
				ni->proto->temp1++;

				if (nfun == NPWELL || nfun == NPSUBSTRATE)
				{
					wc = erc_allocwellcon();
					if (wc == NOWELLCON) { termsearch(sea);   return; }
					xform((ni->lowx + ni->highx) / 2, (ni->lowy + ni->highy) / 2,
						&wc->x, &wc->y, final);
					wc->np = subnp;
					wc->fun = nfun;
					if (ni->firstportarcinst != NOPORTARCINST)
					{
						net = ni->firstportarcinst->conarcinst->network;
					} else if (ni->firstportexpinst != NOPORTEXPINST)
					{
						net = ni->firstportexpinst->exportproto->network;
					} else net = NONETWORK;
					wc->onproperrail = 0;
					if (net == NONETWORK) wc->netnum = 0; else
					{
						wc->netnum = net->temp1;
						if (nfun == NPWELL)
						{
							/* PWell: must be on ground */
							for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
							{
#if 1
								if (!portisnamedground(pp) &&
									(pp->userbits&STATEBITS) != GNDPORT) continue;
#else
								if ((pp->userbits&STATEBITS) != GNDPORT) continue;
#endif
								if (pp->network == net) break;
							}
							if (pp != NOPORTPROTO) wc->onproperrail = 1;
						} else
						{
							/* NWell: must be on power */
							for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
							{
#if 1
								if (!portisnamedpower(pp) &&
									(pp->userbits&STATEBITS) != PWRPORT) continue;
#else
								if ((pp->userbits&STATEBITS) != PWRPORT) continue;
#endif
								if (pp->network == net) break;
							}
							if (pp != NOPORTPROTO) wc->onproperrail = 1;
						}
					}
					wc->nextwellcon = erc_firstwellcon;
					erc_firstwellcon = wc;
				}
			}

			tot = nodepolys(ni, 0, NOWINDOWPART);
			erc_polyscrunched += tot;
			if (erc_realrun && merge != 0)
			{
				if (erc_jobsize > 0)
				{
					if (updatepct++ > 25)
					{
						DiaSetProgress(erc_progressdialog, erc_polyscrunched, erc_jobsize);
						updatepct = 0;
					}
				}
				for(i=0; i<tot; i++)
				{
					shapenodepoly(ni, i, poly);
					if (erc_welllayer(poly->tech, poly->layer) == 0) continue;
					xformpoly(poly, final);
					mergeaddpolygon(merge, poly->layer, poly->tech, poly);
				}
			}
		}
	}
}

/*
 * Coroutine of the polygon merging package that is given a merged polygon with
 * "count" points in (x,y) with area "area".
 */
void erc_getpolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	REGISTER INTBIG i, first, welltype;
	REGISTER INTBIG dist, bestdist, desiredcontact, prev, xp, yp;
	REGISTER WELLCON *wc, *bestwc;
	REGISTER WELLAREA *wa;
	static INTBIG checkstop = 0;

	erc_polyscrunched += count;
	if (!erc_realrun) return;

	checkstop++;
	if ((checkstop%10) == 0)
	{
		if (stopping(STOPREASONERC)) return;
	} else
	{
		if (el_pleasestop != 0) return;
	}
	DiaSetProgress(erc_progressdialog, erc_polyscrunched, erc_jobsize);

	/* save the polygon */
	wa = erc_allocwellarea();
	wa->poly = allocpolygon(count, erc_tool->cluster);
	for(i=0; i<count; i++)
	{
		wa->poly->xv[i] = x[i];
		wa->poly->yv[i] = y[i];
	}
	wa->poly->count = count;
	wa->poly->style = FILLED;
	wa->layer = layer;
	wa->tech = tech;
	getbbox(wa->poly, &wa->lx, &wa->hx, &wa->ly, &wa->hy);
	wa->nextwellarea = erc_firstwellarea;
	erc_firstwellarea = wa;

	/* special checks for the well areas */
	welltype = erc_welllayer(tech, layer);
	if (welltype != 1 && welltype != 2) return;
	if (welltype == 1) desiredcontact = NPWELL; else
		desiredcontact = NPSUBSTRATE;

	/* stop now if distance to edge not desired */
	if ((erc_options&FINDEDGEDIST) == 0) return;

	/* find the worst distance to the edge of the area */
	for(i=0; i<count*2; i++)
	{
		/* figure out which point is being analyzed */
		if (i < count)
		{
			if (i == 0) prev = count-1; else prev = i-1;
			xp = (x[prev] + x[i]) / 2;
			yp = (y[prev] + y[i]) / 2;
		} else
		{
			xp = x[i-count];
			yp = y[i-count];
		}

		/* find the closest contact to this point */
		first = 1;
		bestdist = 0;
		for(wc = erc_firstwellcon; wc != NOWELLCON; wc = wc->nextwellcon)
		{
			if (wc->fun != desiredcontact) continue;
			if (wc->x < wa->lx || wc->x > wa->hx ||
				wc->y < wa->ly || wc->y > wa->hy) continue;
			if (!isinside(wc->x, wc->y, wa->poly)) continue;
			dist = computedistance(wc->x, wc->y, xp, yp);
			if (first != 0 || dist < bestdist)
			{
				bestdist = dist;
				bestwc = wc;
			}
			first = 0;
		}
		if (first != 0) continue;

		/* accumulate worst distances to edges */
		if (welltype == 1)
		{
			if (bestdist > erc_worstpwelldist)
			{
				erc_worstpwelldist = bestdist;
				erc_worstpwellconx = bestwc->x;
				erc_worstpwellcony = bestwc->y;
				erc_worstpwelledgex = xp;
				erc_worstpwelledgey = yp;
			}
		} else
		{
			if (bestdist > erc_worstnwelldist)
			{
				erc_worstnwelldist = bestdist;
				erc_worstnwellconx = bestwc->x;
				erc_worstnwellcony = bestwc->y;
				erc_worstnwelledgex = xp;
				erc_worstnwelledgey = yp;
			}
		}
	}
}

/*
 * Routine to return nonzero if layer "layer" in technology "tech" is a well/select layer.
 * Returns:
 *   1: P-Well
 *   2: N-Well
 *   3: P-Select
 *   4: N-Select
 */
INTBIG erc_welllayer(TECHNOLOGY *tech, INTBIG layer)
{
	REGISTER INTBIG fun, funpure;

	fun = layerfunction(tech, layer);
	if ((fun&LFPSEUDO) != 0) return(0);
	funpure = fun & LFTYPE;
	if (funpure == LFWELL)
	{
		if ((fun&LFPTYPE) != 0) return(1);
		return(2);
	}
	if (funpure == LFSUBSTRATE || funpure == LFIMPLANT)
	{
		if ((fun&LFPTYPE) != 0) return(3);
		return(4);
	}
	return(0);
}

void erc_freeallwellcons(void)
{
	REGISTER WELLCON *wc;

	while (erc_firstwellcon != NOWELLCON)
	{
		wc = erc_firstwellcon;
		erc_firstwellcon = wc->nextwellcon;
		efree((CHAR *)wc);
	}
}

WELLCON *erc_allocwellcon(void)
{
	REGISTER WELLCON *wc;

	if (erc_wellconfree != NOWELLCON)
	{
		wc = erc_wellconfree;
		erc_wellconfree = wc->nextwellcon;
	} else
	{
		wc = (WELLCON *)emalloc(sizeof (WELLCON), erc_tool->cluster);
		if (wc == 0) return(NOWELLCON);
	}
	return(wc);
}

void erc_freeallwellareas(void)
{
	REGISTER WELLAREA *wa;

	while (erc_firstwellarea != NOWELLAREA)
	{
		wa = erc_firstwellarea;
		erc_firstwellarea = wa->nextwellarea;
		freepolygon(wa->poly);
		efree((CHAR *)wa);
	}
}

WELLAREA *erc_allocwellarea(void)
{
	REGISTER WELLAREA *wa;

	if (erc_wellareafree != NOWELLAREA)
	{
		wa = erc_wellareafree;
		erc_wellareafree = wa->nextwellarea;
	} else
	{
		wa = (WELLAREA *)emalloc(sizeof (WELLAREA), erc_tool->cluster);
		if (wa == 0) return(NOWELLAREA);
	}
	return(wa);
}

/******************** OPTIONS DIALOG ********************/

/* Well Check Options Dialog */
static DIALOGITEM erc_optionsdialogitems[] =
{
 /*  1 */ {0, {156,296,180,360}, BUTTON, N_("OK")},
 /*  2 */ {0, {156,108,180,172}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {32,8,48,228}, RADIO, N_("Must have contact in every area")},
 /*  4 */ {0, {56,8,72,228}, RADIO, N_("Must have at least 1 contact")},
 /*  5 */ {0, {80,8,96,228}, RADIO, N_("Do not check for contacts")},
 /*  6 */ {0, {8,44,24,144}, MESSAGE, N_("For P-Well:")},
 /*  7 */ {0, {32,248,48,480}, RADIO, N_("Must have contact in every area")},
 /*  8 */ {0, {56,248,72,480}, RADIO, N_("Must have at least 1 contact")},
 /*  9 */ {0, {80,248,96,480}, RADIO, N_("Do not check for contacts")},
 /* 10 */ {0, {8,284,24,384}, MESSAGE, N_("For N-Well:")},
 /* 11 */ {0, {104,8,120,228}, CHECK, N_("Must connect to Ground")},
 /* 12 */ {0, {104,248,120,480}, CHECK, N_("Must connect to Power")},
 /* 13 */ {0, {128,60,144,448}, CHECK, N_("Find farthest distance from contact to edge")}
};
static DIALOG erc_optionsdialog = {{50,75,239,564}, N_("Well Check Options"), 0, 13, erc_optionsdialogitems, 0, 0};

/* special items for the "ERC Options" dialog: */
#define DERO_PWELLEACH   3		/* PWell contact in each area (radio) */
#define DERO_PWELLONCE   4		/* PWell contact once (radio) */
#define DERO_PWELLNOT    5		/* PWell contact not needed (radio) */
#define DERO_NWELLEACH   7		/* NWell contact in each area (radio) */
#define DERO_NWELLONCE   8		/* NWell contact once (radio) */
#define DERO_NWELLNOT    9		/* NWell contact not needed (radio) */
#define DERO_PWELLGND   11		/* PWell must connect to ground (check) */
#define DERO_NWELLVDD   12		/* NWell must connect to power (check) */
#define DERO_FINDDIST   13		/* Find farthest dist. from contact to edge (check) */

void erc_optionsdlog(void)
{
	REGISTER INTBIG itemHit, ercoptions, oldoptions;
	REGISTER VARIABLE *var;
	REGISTER void *dia;

	dia = DiaInitDialog(&erc_optionsdialog);
	if (dia == 0) return;
	var = getvalkey((INTBIG)erc_tool, VTOOL, VINTEGER, erc_optionskey);
	if (var == NOVARIABLE) ercoptions = DEFERCBITS; else
		ercoptions = var->addr;
	oldoptions = ercoptions;
	switch (ercoptions & PWELLCONOPTIONS)
	{
		case PWELLCONPERAREA: DiaSetControl(dia, DERO_PWELLEACH, 1);   break;
		case PWELLCONONCE:    DiaSetControl(dia, DERO_PWELLONCE, 1);   break;
		case PWELLCONIGNORE:  DiaSetControl(dia, DERO_PWELLNOT, 1);    break;
	}
	switch (ercoptions & NWELLCONOPTIONS)
	{
		case NWELLCONPERAREA: DiaSetControl(dia, DERO_NWELLEACH, 1);   break;
		case NWELLCONONCE:    DiaSetControl(dia, DERO_NWELLONCE, 1);   break;
		case NWELLCONIGNORE:  DiaSetControl(dia, DERO_NWELLNOT, 1);    break;
	}
	if ((ercoptions&PWELLONGROUND) != 0) DiaSetControl(dia, DERO_PWELLGND, 1);
	if ((ercoptions&NWELLONPOWER) != 0) DiaSetControl(dia, DERO_NWELLVDD, 1);
	if ((ercoptions&FINDEDGEDIST) != 0) DiaSetControl(dia, DERO_FINDDIST, 1);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DERO_PWELLEACH || itemHit == DERO_PWELLONCE ||
			itemHit == DERO_PWELLNOT)
		{
			DiaSetControl(dia, DERO_PWELLEACH, 0);
			DiaSetControl(dia, DERO_PWELLONCE, 0);
			DiaSetControl(dia, DERO_PWELLNOT, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DERO_NWELLEACH || itemHit == DERO_NWELLONCE ||
			itemHit == DERO_NWELLNOT)
		{
			DiaSetControl(dia, DERO_NWELLEACH, 0);
			DiaSetControl(dia, DERO_NWELLONCE, 0);
			DiaSetControl(dia, DERO_NWELLNOT, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DERO_PWELLGND || itemHit == DERO_NWELLVDD ||
			itemHit == DERO_FINDDIST)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		/* change state of ERC */
		ercoptions = 0;
		if (DiaGetControl(dia, DERO_PWELLEACH) != 0) ercoptions |= PWELLCONPERAREA; else
			if (DiaGetControl(dia, DERO_PWELLONCE) != 0) ercoptions |= PWELLCONONCE; else
				ercoptions |= PWELLCONIGNORE;
		if (DiaGetControl(dia, DERO_NWELLEACH) != 0) ercoptions |= NWELLCONPERAREA; else
			if (DiaGetControl(dia, DERO_NWELLONCE) != 0) ercoptions |= NWELLCONONCE; else
				ercoptions |= NWELLCONIGNORE;
		if (DiaGetControl(dia, DERO_PWELLGND) != 0) ercoptions |= PWELLONGROUND;
		if (DiaGetControl(dia, DERO_NWELLVDD) != 0) ercoptions |= NWELLONPOWER;
		if (DiaGetControl(dia, DERO_FINDDIST) != 0) ercoptions |= FINDEDGEDIST;
		if (ercoptions != oldoptions)
			(void)setvalkey((INTBIG)erc_tool, VTOOL, erc_optionskey, ercoptions, VINTEGER);
	}
	DiaDoneDialog(dia);
}

#endif  /* ERCTOOL - at top */
