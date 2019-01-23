/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: conlay.c
 * Hierarchical layout constraint system
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2001 Static Free Software
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
#include "conlay.h"
#include "database.h"
#include "usr.h"

/* working memory for "cla_modwithin()" */
static INTBIG    cla_workingarccount = 0;
static ARCINST **cla_workingarcs;

/* routines referenced in "conlin.c"*/
void cla_oldportposition(NODEINST*, PORTPROTO*, INTBIG*, INTBIG*);
void cla_adjustmatrix(NODEINST*, PORTPROTO*, XARRAY);

/* prototypes for local routines */
static BOOLEAN cla_modifynodeinst(NODEINST *ni, INTBIG deltalx, INTBIG deltaly, INTBIG deltahx,
				INTBIG deltahy, INTBIG dangle, INTBIG dtrans, BOOLEAN announce);
static BOOLEAN cla_modnodearcs(NODEINST*, INTBIG, INTBIG);
static void cla_modwithin(NODEINST*, INTBIG, INTBIG);
static BOOLEAN cla_modrigid(NODEINST*, INTBIG, INTBIG);
static BOOLEAN cla_modflex(NODEINST*, INTBIG, INTBIG);
static void cla_nonorthogfixang(ARCINST*, INTBIG, INTBIG, NODEINST*, INTBIG[2], INTBIG[2]);
static void cla_ensurearcinst(ARCINST*, INTBIG);
static void cla_updatearc(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void cla_domovearcinst(ARCINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
static void cla_makeoldrot(NODEINST*, XARRAY);
static void cla_makeoldtrans(NODEINST*, XARRAY);
static void cla_computecell(NODEPROTO*, BOOLEAN);
static BOOLEAN cla_lookdown(NODEPROTO*);

#define CLA_DEBUG 1			/* comment out for normal life */

/* command completion table for this constraint solver */
static KEYWORD layconopt[] =
{
	{x_("debug-toggle"),      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP cla_layconp = {layconopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("layout constraint system option"), 0};

INTBIG cla_changeclock;
#ifdef CLA_DEBUG
static BOOLEAN cla_conlaydebug;
#endif
CONSTRAINT *cla_constraint;	/* the constraint object for this solver */

/******************** CONSTRAINT SYSTEM HOOKS *************************/

void cla_layconinit(CONSTRAINT *con)
{
	/* only function during pass 1 of initialization */
	if (con == NOCONSTRAINT) return;
	cla_constraint = con;
	cla_changeclock = 10;
#ifdef CLA_DEBUG
	cla_conlaydebug = FALSE;
#endif
}

void cla_layconterm(void)
{
	if (cla_workingarccount > 0) efree((CHAR *)cla_workingarcs);
	cla_workingarccount = 0;
}

void cla_layconsetmode(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pp;

	if (count == 0)
	{
		ttyputusage(x_("constraint tell layout OPTION"));
		return;
	}

	if (el_curconstraint != cla_constraint)
	{
		ttyputerr(M_("Must first switch to this solver with 'constraint use'"));
		return;
	}

	l = estrlen(pp = par[0]);

	/* debugging switch */
	if (namesamen(pp, x_("debug-toggle"), l) == 0 && l >= 1)
	{
#ifdef CLA_DEBUG
		cla_conlaydebug = !cla_conlaydebug;
		if (cla_conlaydebug) ttyputmsg(M_("Layout constraint debugging on")); else
			ttyputmsg(M_("Layout constraint debugging off"));
#else
		ttyputmsg(M_("Sorry, constraint debugging code has been compiled out"));
#endif
		return;
	}
	ttyputbadusage(x_("constraint tell layout"));
}

/*
 * the valid "command" is "describearc" which returns a string that describes
 * the constraints on the arc in "arg1".
 */
INTBIG cla_layconrequest(CHAR *command, INTBIG arg1)
{
	REGISTER ARCINST *ai;
	REGISTER INTBIG l;
	REGISTER void *infstr;

	l = estrlen(command);

	if (namesamen(command, x_("describearc"), l) == 0)
	{
		ai = (ARCINST *)arg1;
		infstr = initinfstr();
		if ((ai->userbits&FIXED) != 0) addtoinfstr(infstr, 'R'); else
		{
			switch (ai->userbits&(FIXANG|CANTSLIDE))
			{
				case 0:                addtoinfstr(infstr, 'S');         break;
				case FIXANG:           addstringtoinfstr(infstr, x_("FS"));  break;
				case CANTSLIDE:        addtoinfstr(infstr, 'X');         break;
				case FIXANG|CANTSLIDE: addtoinfstr(infstr, 'F');         break;
			}
		}
		return((INTBIG)returninfstr(infstr));
	}
	return(0);
}

/*
 * routine to do hierarchical update on any cells that changed
 */
void cla_layconsolve(NODEPROTO *np)
{
	REGISTER CHANGECELL *cc;
	REGISTER CHANGEBATCH *curbatch;
	REGISTER CHANGE *c;

	/* if only one cell is requested, solve that */
	curbatch = db_getcurrentbatch();
	if (np != NONODEPROTO)
	{
		cla_computecell(np, FALSE);
	} else
	{
		/* solve all cells that changed */
		if (curbatch != NOCHANGEBATCH)
			for(cc = curbatch->firstchangecell; cc != NOCHANGECELL; cc = cc->nextchangecell)
				cla_computecell(cc->changecell, cc->forcedlook);
	}

	if (curbatch == NOCHANGEBATCH) return;
	for(c = curbatch->changehead; c != NOCHANGE; c = c->nextchange)
		switch (c->changetype)
	{
		case NODEINSTNEW:
		case NODEINSTKILL:
		case NODEINSTMOD:
			((NODEINST *)c->entryaddr)->changeaddr = (CHAR *)NOCHANGE;
			break;
		case ARCINSTNEW:
		case ARCINSTKILL:
		case ARCINSTMOD:
			((ARCINST *)c->entryaddr)->changeaddr = (CHAR *)NOCHANGE;
			break;
		case PORTPROTONEW:
		case PORTPROTOKILL:
		case PORTPROTOMOD:
			((PORTPROTO *)c->entryaddr)->changeaddr = (CHAR *)NOCHANGE;
			break;
		case NODEPROTONEW:
		case NODEPROTOKILL:
		case NODEPROTOMOD:
			((NODEPROTO *)c->entryaddr)->changeaddr = (CHAR *)NOCHANGE;
			break;
	}
}

/*
 * If an export is created, touch all instances of the cell
 */
void cla_layconnewobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;

	if (type == VPORTPROTO)
	{
		pp = (PORTPROTO *)addr;
		np = pp->parent;
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			(void)db_change((INTBIG)ni, NODEINSTMOD, ni->lowx, ni->lowy,
				ni->highx, ni->highy, ni->rotation, ni->transpose);
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
	}
}

/*
 * If an export is deleted, touch all instances of the cell
 */
void cla_layconkillobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;

	if (type == VPORTPROTO)
	{
		pp = (PORTPROTO *)addr;
		np = pp->parent;
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
			(void)db_change((INTBIG)ni, NODEINSTMOD, ni->lowx, ni->lowy,
				ni->highx, ni->highy, ni->rotation, ni->transpose);
			(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
		}
	}
}

/*
 * If an export is renamed, touch all instances of the cell
 */
void cla_layconnewvariable(INTBIG addr, INTBIG type, INTBIG skey, INTBIG stype)
{
	CHAR *name;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;

	if (type == VPORTPROTO)
	{
		if ((stype&VCREF) != 0)
		{
			name = changedvariablename(type, skey, stype);
			if (estrcmp(name, x_("protoname")) == 0)
			{
				pp = (PORTPROTO *)addr;
				np = pp->parent;
				for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
				{
					(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
					(void)db_change((INTBIG)ni, NODEINSTMOD, ni->lowx, ni->lowy,
						ni->highx, ni->highy, ni->rotation, ni->transpose);
					(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
				}
			}
		}
	}
}

/*
 * set layout constraints on arc instance "ai" according to "changetype".
 * the routine returns true if the arc change is not successful (or already
 * done)
 */
BOOLEAN cla_layconsetobject(INTBIG addr, INTBIG type, INTBIG changetype,
	INTBIG /*@unused@*/ changedata)
{
	REGISTER ARCINST *ai;

	if ((type&VTYPE) != VARCINST) return(TRUE);
	ai = (ARCINST *)addr;
	if (ai == NOARCINST) return(TRUE);
	switch (changetype)
	{
		case CHANGETYPERIGID:				/* arc rigid */
			if ((ai->userbits & FIXED) != 0) return(TRUE);
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|FIXED, VINTEGER);
			break;
		case CHANGETYPEUNRIGID:				/* arc un-rigid */
			if ((ai->userbits & FIXED) == 0) return(TRUE);
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~FIXED, VINTEGER);
			break;
		case CHANGETYPEFIXEDANGLE:			/* arc fixed-angle */
			if ((ai->userbits & FIXANG) != 0) return(TRUE);
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|FIXANG, VINTEGER);
			break;
		case CHANGETYPENOTFIXEDANGLE:		/* arc not fixed-angle */
			if ((ai->userbits & FIXANG) == 0) return(TRUE);
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~FIXANG, VINTEGER);
			break;
		case CHANGETYPESLIDABLE:			/* arc slidable */
			if ((ai->userbits & CANTSLIDE) == 0) return(TRUE);
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits & ~CANTSLIDE, VINTEGER);
			break;
		case CHANGETYPENOTSLIDABLE:			/* arc nonslidable */
			if ((ai->userbits & CANTSLIDE) != 0) return(TRUE);
			(void)setval((INTBIG)ai, VARCINST, x_("userbits"), ai->userbits|CANTSLIDE, VINTEGER);
			break;
		case CHANGETYPETEMPRIGID:			/* arc temporarily rigid */
			if (ai->changed == cla_changeclock + 2) return(TRUE);
			ai->changed = cla_changeclock + 2;
			break;
		case CHANGETYPETEMPUNRIGID:			/* arc temporarily un-rigid */
			if (ai->changed == cla_changeclock + 3) return(TRUE);
			ai->changed = cla_changeclock + 3;
			break;
		case CHANGETYPEREMOVETEMP:			/* remove temporarily state */
			if (ai->changed != cla_changeclock + 3 && ai->changed != cla_changeclock + 2) return(TRUE);
			ai->changed = cla_changeclock - 3;
			break;
	}
	return(FALSE);
}

void cla_layconmodifynodeinst(NODEINST *ni, INTBIG dlx, INTBIG dly, INTBIG dhx, INTBIG dhy,
	INTBIG drot, INTBIG dtrans)
{
	/* advance the change clock */
	cla_changeclock += 4;

	/* change the nodeinst */
	if (cla_modifynodeinst(ni, dlx, dly, dhx, dhy, drot, dtrans, FALSE))
		db_forcehierarchicalanalysis(ni->parent);

	/* change the arcs on the nodeinst */
	if (cla_modnodearcs(ni, drot, dtrans))
		db_forcehierarchicalanalysis(ni->parent);
}

void cla_layconmodifynodeinsts(INTBIG count, NODEINST **nis, INTBIG *dlxs, INTBIG *dlys,
	INTBIG *dhxs, INTBIG *dhys, INTBIG *drots, INTBIG *dtranss)
{
	REGISTER INTBIG i;
	REGISTER NODEPROTO *parent;

	/* advance the change clock */
	cla_changeclock += 4;

	/* change the nodeinst */
	parent = NONODEPROTO;
	for(i=0; i<count; i++)
	{
		if (cla_modifynodeinst(nis[i], dlxs[i], dlys[i], dhxs[i], dhys[i],
			drots[i], dtranss[i], FALSE)) parent = nis[i]->parent;
	}

	/* change the arcs on the nodeinst */
	for(i=0; i<count; i++)
	{
		if (cla_modnodearcs(nis[i], drots[i], dtranss[i]))
			parent = nis[i]->parent;
	}
	if (parent != NONODEPROTO) db_forcehierarchicalanalysis(parent);
}

void cla_layconmodifyarcinst(ARCINST /*@unused@*/ *ai, INTBIG /*@unused@*/ oldx0,
	INTBIG /*@unused@*/ oldy0, INTBIG /*@unused@*/ oldx1,
	INTBIG /*@unused@*/ oldy1, INTBIG /*@unused@*/ oldwid, INTBIG /*@unused@*/ oldlen) {}
void cla_layconmodifyportproto(PORTPROTO /*@unused@*/ *pp, NODEINST /*@unused@*/ *oni,
	PORTPROTO /*@unused@*/ *opp) {}
void cla_layconmodifynodeproto(NODEPROTO /*@unused@*/ *np) {}
void cla_layconnewlib(LIBRARY /*@unused@*/ *lib) {}
void cla_layconkilllib(LIBRARY /*@unused@*/ *lib) {}
void cla_layconkillvariable(INTBIG /*@unused@*/ addr, INTBIG /*@unused@*/ type,
	INTBIG /*@unused@*/ key, INTBIG /*@unused@*/ saddr,
	INTBIG /*@unused@*/ stype, UINTBIG /*@unused@*/ *olddes) {}
void cla_layconinsertvariable(INTBIG /*@unused@*/ addr, INTBIG /*@unused@*/ type,
	INTBIG /*@unused@*/ key, INTBIG /*@unused@*/ aindex) {}
void cla_laycondeletevariable(INTBIG /*@unused@*/ addr, INTBIG /*@unused@*/ type,
	INTBIG /*@unused@*/ key, INTBIG /*@unused@*/ aindex, INTBIG /*@unused@*/ oldval) {}
void cla_layconsetvariable(void) {}

/******************** TEXT MODIFICATION CODE *************************/

/* #define TEXTPARTOFOBJECTBOUNDS 1 */

void cla_layconmodifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG stype,
	INTBIG aindex, INTBIG oldval)
{
#ifdef TEXTPARTOFOBJECTBOUNDS
	REGISTER CHAR *name;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;

	/* look for modified text descriptor on exports */
	if (type == VPORTPROTO)
	{
		if ((stype&VCREF) != 0)
		{
			name = changedvariablename(type, key, stype);
			if (estrcmp(name, x_("textdescript")) == 0)
			{
				if (aindex == 1)
				{
					pp = (PORTPROTO *)addr;
					ni = pp->subnodeinst;
					updategeom(ni->geom, ni->parent);
				}
			}
		}
	}
#endif
}

void cla_layconmodifydescript(INTBIG addr, INTBIG type, INTBIG key, UINTBIG *olddes)
{
#ifdef TEXTPARTOFOBJECTBOUNDS
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;

	switch (type)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			updategeom(ni->geom, ni->parent);
			break;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			ni = pp->subnodeinst;
			updategeom(ni->geom, ni->parent);
			break;
		case VARCINST:
			ai = (ARCINST *)addr;
			updategeom(ai->geom, ai->parent);
			break;
	}
#endif
}

/******************** NODE MODIFICATION CODE *************************/

/*
 * The meaning of cla_changeclock for object modification:
 *
 * ai->changed <  cla_changeclock-2  unmodified         arcs
 * ai->changed == cla_changeclock-2  unmodified rigid   arcs
 * ai->changed == cla_changeclock-1  unmodified unrigid arcs
 * ai->changed == cla_changeclock      modified rigid   arcs
 * ai->changed == cla_changeclock+1    modified unrigid arcs
 * ni->changed <  cla_changeclock-1  unmodified         nodes
 * ni->changed == cla_changeclock-1  size-changed       nodes
 * ni->changed == cla_changeclock    position-changed   nodes
 */

/*
 * routine to modify nodeinst "ni" by "deltalx" in low X, "deltaly" in low Y,
 * "deltahx" in high X, "deltahy" in high Y, and "dangle" tenth-degrees.  If
 * "announce" is true, report "start" and "end" changes on the node.
 * If the nodeinst is a portproto of the current cell and has any arcs
 * connected to it, the routine returns nonzero to indicate that the outer
 * cell has ports that moved (the nodeinst has exports).
 */
BOOLEAN cla_modifynodeinst(NODEINST *ni, INTBIG deltalx, INTBIG deltaly, INTBIG deltahx,
	INTBIG deltahy, INTBIG dangle, INTBIG dtrans, BOOLEAN announce)
{
	REGISTER INTBIG oldlx, oldly, oldhx, oldhy, change;
	REGISTER INTSML oldang, oldtrans;

	/* determine whether this is a position or size change */
	if (deltalx == deltahx && deltaly == deltahy)
	{
		if (deltalx == 0 && deltaly == 0 && dangle == 0 && dtrans == 0) change = -1; else
			change = 0;
	} else change = -1;

	/* reject if this change has already been done */
	if (ni->changed >= cla_changeclock+change) return(FALSE);

	/* if simple rotation on transposed nodeinst, reverse rotation */
	if (ni->transpose != 0 && dtrans == 0) dangle = (3600 - dangle) % 3600;

	if (ni->changed < cla_changeclock-1 && announce)
		(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);

	/* make changes to the nodeinst */
	oldang = ni->rotation;     ni->rotation = (INTSML)((ni->rotation + dangle) % 3600);
	oldtrans = ni->transpose;  ni->transpose = (INTSML)((ni->transpose + dtrans) & 1);
	oldlx = ni->lowx;          ni->lowx += deltalx;
	oldhx = ni->highx;         ni->highx += deltahx;
	oldly = ni->lowy;          ni->lowy += deltaly;
	oldhy = ni->highy;         ni->highy += deltahy;
	updategeom(ni->geom, ni->parent);

#ifdef CLA_DEBUG
	if (cla_conlaydebug)
		ttyputmsg(M_("Change node %s by X(%s %s) Y(%s %s) r=%ld t=%ld"),
			describenodeinst(ni), latoa(deltalx, 0), latoa(deltahx, 0), latoa(deltaly, 0),
				latoa(deltahy, 0), dangle, dtrans);
#endif

	/* mark that this nodeinst has changed */
	if (ni->changed < cla_changeclock-1)
	{
		ni->changeaddr = (CHAR *)db_change((INTBIG)ni, NODEINSTMOD, oldlx,
			oldly, oldhx, oldhy, oldang, oldtrans);
		if (announce)
			(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
	}

	ni->changed = cla_changeclock + change;

	/* see if this nodeinst is a port of the current cell */
	if (ni->firstportexpinst == NOPORTEXPINST) return(FALSE);
	return(TRUE);
}

/*
 * routine to modify all of the arcs connected to nodeinst "ni" (which has
 * been rotated by "dangle" tenth-degrees).  If the routine returns nonzero,
 * some ports on the current cell have moved and the cell must be
 * re-examined for portproto locations.
 */
BOOLEAN cla_modnodearcs(NODEINST *ni, INTBIG dangle, INTBIG dtrans)
{
	REGISTER BOOLEAN examinecell;

	/* assume cell needs no further looks */
	examinecell = FALSE;

	/* next look at arcs that run within this nodeinst */
	cla_modwithin(ni, dangle, dtrans);

	/* next look at the rest of the rigid arcs on this nodeinst */
	if (cla_modrigid(ni, dangle, dtrans)) examinecell = TRUE;

	/* finally, look at rest of the flexible arcs on this nodeinst */
	if (cla_modflex(ni, dangle, dtrans)) examinecell = TRUE;

	return(examinecell);
}

/*
 * routine to modify the arcs that run within nodeinst "ni"
 */
void cla_modwithin(NODEINST *ni, INTBIG dangle, INTBIG dtrans)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER INTBIG ox, oy, i, total;
	INTBIG nox, noy, onox, onoy;
	XARRAY trans;

	/* ignore all this stuff if the node just got created */
	if (((CHANGE *)ni->changeaddr)->changetype == NODEINSTNEW) return;

	/* build a list of arcs with both ends on this nodeinst */
	total = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore if arcinst is not within the node */
		ai = pi->conarcinst;
		if (ai->end[0].nodeinst != ai->end[1].nodeinst) continue;
		if (ai->changed == cla_changeclock) continue;
		total++;
	}
	if (total == 0) return;
	if (total > cla_workingarccount)
	{
		if (cla_workingarccount > 0) efree((CHAR *)cla_workingarcs);
		cla_workingarccount = 0;
		cla_workingarcs = (ARCINST **)emalloc(total * (sizeof (ARCINST *)), db_cluster);
		if (cla_workingarcs == 0) return;
		cla_workingarccount = total;
	}
	total = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore if arcinst is not within the node */
		ai = pi->conarcinst;
		if (ai->end[0].nodeinst != ai->end[1].nodeinst) continue;
		if (ai->changed == cla_changeclock) continue;
		for(i=0; i<total; i++) if (cla_workingarcs[i] == ai) break;
		if (i >= total)
			cla_workingarcs[total++] = ai;
	}

	/* look for arcs with both ends on this nodeinst */
	for(i=0; i<total; i++)
	{
		ai = cla_workingarcs[i];
		if ((ai->userbits&DEADA) != 0) continue;

		/* prepare transformation matrix */
		makeangle(dangle, dtrans, trans);

		/* compute old center of nodeinst */
		ox = (((CHANGE *)ni->changeaddr)->p1 + ((CHANGE *)ni->changeaddr)->p3) / 2;
		oy = (((CHANGE *)ni->changeaddr)->p2 + ((CHANGE *)ni->changeaddr)->p4) / 2;

		/* determine the new ends of the arcinst */
		cla_adjustmatrix(ni, ai->end[0].portarcinst->proto, trans);
		xform(ai->end[0].xpos-ox, ai->end[0].ypos-oy, &nox, &noy, trans);
		cla_adjustmatrix(ni, ai->end[1].portarcinst->proto, trans);
		xform(ai->end[1].xpos-ox, ai->end[1].ypos-oy, &onox, &onoy, trans);
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Internal arc %s moves 0:%s,%s  1:%s,%s"),
				describearcinst(ai), latoa(nox, 0), latoa(noy, 0), latoa(onox, 0), latoa(onoy, 0));
#endif
		/* move the arcinst */
		cla_domovearcinst(ai, nox, noy, onox, onoy, 0);
	}
}

/*
 * routine to modify the rigid arcs that run from nodeinst "ni".  The nodeinst
 * has changed "dangle" tenth-degrees.  If any nodes that are ports in the
 * current cell change position, the routine returns nonzero to indicate
 * that instances of the current cell must be examined for arcinst motion.
 */
BOOLEAN cla_modrigid(NODEINST *ni, INTBIG dangle, INTBIG dtrans)
{
	REGISTER PORTPROTO *opt;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai, **arclist;
	REGISTER NODEINST *ono;
	INTBIG ax[2], ay[2], onox, onoy, dx, dy;
	XARRAY trans;
	REGISTER INTBIG othx, othy, ox, oy, i, total, thisend, thatend;
	REGISTER INTBIG nextangle;
	REGISTER BOOLEAN examinecell, locked;

	/* build a list of the rigid arcs on this nodeinst */
	total = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore if arcinst is not rigid */
		ai = pi->conarcinst;
		if (ai->changed == cla_changeclock-1 || ai->changed == cla_changeclock+1) continue;
		if (ai->changed != cla_changeclock-2 && (ai->userbits&FIXED) == 0) continue;
		total++;
	}
	if (total == 0) return(FALSE);
	arclist = (ARCINST **)emalloc(total * (sizeof (ARCINST *)), el_tempcluster);
	if (arclist == 0) return(FALSE);
	i = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore if arcinst is not rigid */
		ai = pi->conarcinst;
		if (ai->changed == cla_changeclock-1 || ai->changed == cla_changeclock+1) continue;
		if (ai->changed != cla_changeclock-2 && (ai->userbits&FIXED) == 0) continue;
		arclist[i++] = ai;
	}

	/* if simple rotation on transposed nodeinst, reverse rotation */
	nextangle = dangle;
	if (((CHANGE *)ni->changeaddr)->changetype != NODEINSTNEW &&
		((CHANGE *)ni->changeaddr)->p6 != 0 && dtrans != 0)
			nextangle = (3600 - dangle) % 3600;

	/* prepare transformation matrix and angle/transposition information */
	makeangle(nextangle, dtrans, trans);

	/* look for rigid arcs on this nodeinst */
	examinecell = FALSE;
	for(i=0; i<total; i++)
	{
		ai = arclist[i];
		if ((ai->userbits&DEADA) != 0) continue;
		ai->userbits &= ~FIXEDMOD;

		/* if rigid arcinst has already been changed check its connectivity */
		if (ai->changed == cla_changeclock)
		{
			cla_ensurearcinst(ai, 0);
			continue;
		}

		/* find out which end of the arcinst is where, ignore internal arcs */
		thisend = thatend = 0;
		if (ai->end[0].nodeinst == ni) thatend++;
		if (ai->end[1].nodeinst == ni) thisend++;
		if (thisend == thatend) continue;

		if (((CHANGE *)ni->changeaddr)->changetype == NODEINSTNEW) ox = oy = 0; else
		{
			ox = (((CHANGE *)ni->changeaddr)->p1 + ((CHANGE *)ni->changeaddr)->p3) / 2;
			oy = (((CHANGE *)ni->changeaddr)->p2 + ((CHANGE *)ni->changeaddr)->p4) / 2;
			cla_adjustmatrix(ni, ai->end[thisend].portarcinst->proto, trans);
		}

		/* figure out the new location of this arcinst connection */
		xform(ai->end[thisend].xpos-ox, ai->end[thisend].ypos-oy,
			&ax[thisend], &ay[thisend], trans);

		ono = ai->end[thatend].nodeinst;
		opt = ai->end[thatend].portarcinst->proto;
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Modify rigid arc %s from %s to %s"),
				describearcinst(ai), describenodeinst(ni), describenodeinst(ono));
#endif

		/* figure out the new location of that arcinst connection */
		xform(ai->end[thatend].xpos-ox, ai->end[thatend].ypos-oy, &ax[thatend],
			&ay[thatend], trans);
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Other end of arc moves to (%s,%s)"),
				latoa(ax[thatend], 0), latoa(ay[thatend], 0));
#endif

		/* see if other nodeinst has changed */
		locked = FALSE;
		if (ono->changed == cla_changeclock) locked = TRUE; else
		{
			if ((ono->userbits&NILOCKED) != 0) locked = TRUE; else
			{
				if (ono->proto->primindex == 0)
				{
					if ((ono->parent->userbits&NPILOCKED) != 0) locked = TRUE;
				} else
				{
					if ((us_useroptions&NOPRIMCHANGES) != 0 &&
						(ono->proto->userbits&LOCKEDPRIM) != 0) locked = TRUE;
				}
			}
		}
		if (!locked)
		{
			/* compute port motion within the other nodeinst (is this right? !!!) */
			cla_oldportposition(ono, opt, &onox, &onoy);
			portposition(ono, opt, &dx, &dy);
			othx = dx - onox;   othy = dy - onoy;

			/* figure out the new location of the other nodeinst */
			onox = (ono->lowx + ono->highx) / 2;
			onoy = (ono->lowy + ono->highy) / 2;
			xform(onox-ox, onoy-oy, &dx, &dy, trans);
			dx = dx - onox - othx;
			dy = dy - onoy - othy;

			/* move the other nodeinst */
			nextangle = dangle;
			if (dtrans != 0 && ono->transpose != ((CHANGE *)ni->changeaddr)->p6)
				nextangle = (3600 - nextangle) % 3600;

			/* ignore null motion on nodes that have already been examined */
			if (dx != 0 || dy != 0 || nextangle != 0 || dtrans != 0 || ono->changed != cla_changeclock-1)
			{
				ai->userbits |= FIXEDMOD;
				if (cla_modifynodeinst(ono, dx, dy, dx, dy, nextangle, dtrans, TRUE))
					examinecell = TRUE;
			}
		}

		/* move the arcinst */
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Now arc runs from (%s,%s) to (%s,%s)"),
				latoa(ax[0], 0), latoa(ay[0], 0), latoa(ax[1], 0), latoa(ay[1], 0));
#endif
		cla_domovearcinst(ai, ax[0],ay[0], ax[1],ay[1], 0);
	}

	/* re-scan rigid arcs and recursively modify arcs on other nodes */
	for(i=0; i<total; i++)
	{
		ai = arclist[i];
		if ((ai->userbits&DEADA) != 0) continue;

		/* only want arcinst that was just explored */
		if ((ai->userbits & FIXEDMOD) == 0) continue;

		/* get the other nodeinst */
		if (ai->end[0].nodeinst == ni) ono = ai->end[1].nodeinst; else
			ono = ai->end[0].nodeinst;

		nextangle = dangle;
		if (dtrans != 0 && ((CHANGE *)ono->changeaddr)->p6 != ((CHANGE *)ni->changeaddr)->p6)
			nextangle = (3600 - nextangle) % 3600;
		if (cla_modnodearcs(ono, nextangle, dtrans)) examinecell = TRUE;
	}
	efree((CHAR *)arclist);
	return(examinecell);
}

/*
 * routine to modify the flexible arcs connected to nodeinst "ni".  If any
 * nodes that are ports move, the routine returns nonzero to indicate that
 * instances of the current cell must be examined for arcinst motion
 */
BOOLEAN cla_modflex(NODEINST *ni, INTBIG dangle, INTBIG dtrans)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai, **arclist;
	REGISTER NODEINST *ono;
	REGISTER INTBIG i, total, ox, oy, odx, ody, thisend, thatend, mangle;
	REGISTER INTBIG nextangle;
	REGISTER BOOLEAN examinecell;
	XARRAY trans;
	INTBIG ax[2], ay[2], dx, dy, lx, hx, ly, hy;
	static POLYGON *poly = NOPOLYGON;

	/* build a list of the flexible arcs on this nodeinst */
	total = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore if arcinst is not flexible */
		ai = pi->conarcinst;
		if (ai->changed == cla_changeclock-2 || ai->changed == cla_changeclock) continue;
		if (ai->changed != cla_changeclock-1 && (ai->userbits&FIXED) != 0) continue;
		total++;
	}
	if (total == 0) return(FALSE);
	arclist = (ARCINST **)emalloc(total * (sizeof (ARCINST *)), el_tempcluster);
	if (arclist == 0) return(FALSE);
	i = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* ignore if arcinst is not flexible */
		ai = pi->conarcinst;
		if (ai->changed == cla_changeclock-2 || ai->changed == cla_changeclock) continue;
		if (ai->changed != cla_changeclock-1 && (ai->userbits&FIXED) != 0) continue;
		arclist[i++] = ai;
	}

	/* if simple rotation on transposed nodeinst, reverse rotation */
	nextangle = dangle;
	if (((CHANGE *)ni->changeaddr)->changetype != NODEINSTNEW &&
		((CHANGE *)ni->changeaddr)->p6 != 0 && dtrans != 0)
			nextangle = (3600 - dangle) % 3600;

	/* prepare transformation matrix and angle/transposition information */
	makeangle(nextangle, dtrans, trans);

	/* look at all of the flexible arcs on this nodeinst */
	examinecell = FALSE;
	for(i=0; i<total; i++)
	{
		ai = arclist[i];
		if ((ai->userbits&DEADA) != 0) continue;

		/* if flexible arcinst has been changed, verify its connectivity */
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
		{
			ttyputmsg(M_("Considering arc %s (clock=%ld, curclock=%ld)"),
				describearcinst(ai), ai->changed, cla_changeclock+1);
		}
#endif
		if (ai->changed >= cla_changeclock+1)
		{
			cla_ensurearcinst(ai, 1);
			continue;
		}

		/* figure where each end of the arcinst is, ignore internal arcs */
		thisend = thatend = 0;
		if (ai->end[0].nodeinst == ni) thatend++;
		if (ai->end[1].nodeinst == ni) thisend++;
		if (thisend == thatend) continue;

		/* if nodeinst motion stays within port area, ignore the arcinst */
		if ((ai->userbits&CANTSLIDE) == 0 &&
			db_stillinport(ai, thisend, ai->end[thisend].xpos, ai->end[thisend].ypos))
				continue;

		if (((CHANGE *)ni->changeaddr)->changetype == NODEINSTNEW) ox = oy = 0; else
		{
			ox = (((CHANGE *)ni->changeaddr)->p1 + ((CHANGE *)ni->changeaddr)->p3) / 2;
			oy = (((CHANGE *)ni->changeaddr)->p2 + ((CHANGE *)ni->changeaddr)->p4) / 2;
			cla_adjustmatrix(ni, ai->end[thisend].portarcinst->proto, trans);
		}

		/* figure out the new location of this arcinst connection */
		xform(ai->end[thisend].xpos-ox, ai->end[thisend].ypos-oy,
			&ax[thisend], &ay[thisend], trans);

		/* make sure the arc end is still in the port */
		(void)needstaticpolygon(&poly, 4, db_cluster);
		shapeportpoly(ai->end[thisend].nodeinst, ai->end[thisend].portarcinst->proto,
			poly, FALSE);
		if (!isinside(ax[thisend], ay[thisend], poly))
		{
			getbbox(poly, &lx, &hx, &ly, &hy);
			if (ay[thisend] >= ly && ay[thisend] <= hy)
			{
				/* extend arc horizontally to fit in port */
				if (ax[thisend] < lx) ax[thisend] = lx; else
					if (ax[thisend] > hx) ax[thisend] = hx;
			} else if (ax[thisend] >= lx && ax[thisend] <= hx)
			{
				/* extend arc vertically to fit in port */
				if (ay[thisend] < ly) ay[thisend] = ly; else
					if (ay[thisend] > hy) ay[thisend] = hy;
			} else
			{
				/* extend arc arbitrarily to fit in port */
				closestpoint(poly, &ax[thisend], &ay[thisend]);
			}
		}

		/* get other end of arcinst and its position */
		ono = ai->end[thatend].nodeinst;
		ax[thatend] = ai->end[thatend].xpos;
		ay[thatend] = ai->end[thatend].ypos;
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
		{
			ttyputmsg(M_("Modify flexible arc %s from %s to %s"),
				describearcinst(ai), describenodeinst(ni), describenodeinst(ono));
		}
#endif

		/* see if other nodeinst has changed */
		mangle = 1;
		if ((ai->userbits&FIXANG) == 0) mangle = 0; else
		{
			if ((ono->userbits&NILOCKED) != 0) mangle = 0; else
			{
				if (ono->proto->primindex == 0)
				{
					if ((ono->parent->userbits&NPILOCKED) != 0) mangle = 0;
				} else
				{
					if ((us_useroptions&NOPRIMCHANGES) != 0 &&
						(ono->proto->userbits&LOCKEDPRIM) != 0) mangle = 0;
				}
			}
		}
		if (mangle != 0)
		{
			/* other nodeinst untouched, mangle it */
			dx = ax[thisend] - ai->end[thisend].xpos;
			dy = ay[thisend] - ai->end[thisend].ypos;
			odx = ax[thatend] - ai->end[thatend].xpos;
			ody = ay[thatend] - ai->end[thatend].ypos;
			if (ai->end[thisend].xpos == ai->end[thatend].xpos)
			{
				/* null arcinst must not be explicitly horizontal */
				if (ai->end[thisend].ypos != ai->end[thatend].ypos ||
					((ai->userbits&AANGLE) >> AANGLESH) == 90 ||
						((ai->userbits&AANGLE) >> AANGLESH) == 270)
				{
					/* vertical arcinst: see if it really moved in X */
					if (dx == odx) dx = odx = 0;

					/* move horizontal, shrink vertical */
					ax[thatend] += dx-odx;

					/* see if next nodeinst need not be moved */
					if (dx != odx && (ai->userbits&CANTSLIDE) == 0 &&
						db_stillinport(ai, thatend, ax[thatend], ay[thatend]))
							dx = odx = 0;

					/* if other node already moved, don't move it any more */
					if (ono->changed >= cla_changeclock) dx = odx = 0;

#ifdef	CLA_DEBUG
					if (cla_conlaydebug)
						ttyputmsg(M_("  Moving other end of arc to (%s,%s), other node by (%s,0)"),
							latoa(ax[thatend], 0), latoa(ay[thatend], 0), latoa(dx-odx, 0));
#endif
					if (dx != odx)
					{
						if (cla_modifynodeinst(ono, dx-odx, 0, dx-odx, 0, 0, 0, TRUE))
							examinecell = TRUE;
					}
					cla_domovearcinst(ai, ax[0],ay[0], ax[1],ay[1], 1);
					if (dx != odx)
						if (cla_modnodearcs(ono, 0, 0)) examinecell = TRUE;
					continue;
				}
			}
			if (ai->end[thisend].ypos == ai->end[thatend].ypos)
			{
				/* horizontal arcinst: see if it really moved in Y */
				if (dy == ody) dy = ody = 0;

				/* shrink horizontal, move vertical */
				ay[thatend] += dy-ody;

				/* see if next nodeinst need not be moved */
				if (dy != ody && (ai->userbits&CANTSLIDE) == 0 &&
					db_stillinport(ai, thatend, ax[thatend], ay[thatend]))
						dy = ody = 0;

				/* if other node already moved, don't move it any more */
				if (ono->changed >= cla_changeclock) dx = odx = 0;

#ifdef	CLA_DEBUG
				if (cla_conlaydebug)
					ttyputmsg(M_("  Moving other end of arc to (%s,%s), other node by (0,%s)"),
						latoa(ax[thatend], 0), latoa(ay[thatend], 0), latoa(dy-ody, 0));
#endif
				if (dy != ody)
				{
					if (cla_modifynodeinst(ono, 0, dy-ody, 0, dy-ody, 0, 0, TRUE))
						examinecell = TRUE;
				}
				cla_domovearcinst(ai, ax[0],ay[0], ax[1],ay[1], 1);
				if (dy != ody) if (cla_modnodearcs(ono, 0, 0)) examinecell = TRUE;
				continue;
			}

			/***** THIS CODE HANDLES ALL-ANGLE RIGIDITY WITH THE FIXED-ANGLE CONSTRAINT *****/

			/* special code to handle nonorthogonal fixed-angles */
			cla_nonorthogfixang(ai, thisend, thatend, ono, ax, ay);
			dx = ax[thatend] - ai->end[thatend].xpos;
			dy = ay[thatend] - ai->end[thatend].ypos;

			/* change the arc */
			cla_updatearc(ai, ax[0],ay[0], ax[1],ay[1], 1);

			/* if other node already moved, don't move it any more */
			if (ono->changed >= cla_changeclock) dx = dy = 0;

			if (dx != 0 || dy != 0)
			{
				if (cla_modifynodeinst(ono, dx, dy, dx, dy, 0, 0, TRUE))
					examinecell = TRUE;
				if (cla_modnodearcs(ono, 0, 0)) examinecell = TRUE;
			}
			continue;
		}

		/* other node has changed or arc is funny, just use its position */
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("  ! Moving arc %s to (%s,%s)-(%s,%s)"), describearcinst(ai),
				latoa(ax[0], 0), latoa(ay[0], 0), latoa(ax[1], 0), latoa(ay[1], 0));
#endif
		cla_domovearcinst(ai, ax[0],ay[0], ax[1],ay[1], 1);
	}
	efree((CHAR *)arclist);
	return(examinecell);
}

/*
 * routine to determine the motion of a nonorthogonal arcinst "ai" given that
 * its "thisend" end has moved to (ax[thisend],ay[thisend]) and its other end must be
 * determined in (ax[thatend],ay[thatend]).  The node on the other end is "ono".
 */
void cla_nonorthogfixang(ARCINST *ai, INTBIG thisend, INTBIG thatend, NODEINST *ono,
	INTBIG ax[2], INTBIG ay[2])
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *oai, *bestai;
	REGISTER INTBIG bestdist;
	INTBIG ix, iy;

	/* look for longest other arc on "ono" to determine proper end position */
	bestdist = -1;
	for(pi = ono->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		oai = pi->conarcinst;
		if (oai == ai) continue;
		if (oai->length < bestdist) continue;
		bestdist = oai->length;
		bestai = oai;
	}

	/* if no other arcs, allow that end to move the same as this end */
	if (bestdist < 0)
	{
		ax[thatend] += ax[thisend] - ai->end[thisend].xpos;
		ay[thatend] += ay[thisend] - ai->end[thisend].ypos;
		return;
	}

	/* compute intersection of arc "bestai" with new moved arc "ai" */
	if (intersect(ax[thisend],ay[thisend], ((ai->userbits&AANGLE) >> AANGLESH) * 10,
		bestai->end[0].xpos, bestai->end[0].ypos,
			((bestai->userbits&AANGLE) >> AANGLESH) * 10, &ix, &iy) < 0)
	{
		ax[thatend] += ax[thisend] - ai->end[thisend].xpos;
		ay[thatend] += ay[thisend] - ai->end[thisend].ypos;
		return;
	}
	ax[thatend] = ix;
	ay[thatend] = iy;
}

/*
 * routine to ensure that arcinst "ai" is still connected properly at each
 * end.  If it is not, it must be jogged or adjusted.  The nature of the
 * arcinst is in "arctyp" (0 for rigid, 1 for flexible)
 */
void cla_ensurearcinst(ARCINST *ai, INTBIG arctyp)
{
	REGISTER BOOLEAN inside0, inside1;
	INTBIG lx0, lx1, hx0, hx1, ly0, ly1, hy0, hy1, fx, fy, tx, ty;
	static POLYGON *poly0 = NOPOLYGON, *poly1 = NOPOLYGON;

	/* if nothing is outside port, quit */
	inside0 = db_stillinport(ai, 0, ai->end[0].xpos, ai->end[0].ypos);
	inside1 = db_stillinport(ai, 1, ai->end[1].xpos, ai->end[1].ypos);
	if (inside0 && inside1) return;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly0, 4, cla_constraint->cluster);
	(void)needstaticpolygon(&poly1, 4, cla_constraint->cluster);

	/* get area of the ports */
	shapeportpoly(ai->end[0].nodeinst, ai->end[0].portarcinst->proto, poly0, FALSE);
	shapeportpoly(ai->end[1].nodeinst, ai->end[1].portarcinst->proto, poly1, FALSE);

	/* if arcinst is not fixed-angle, run it directly to the port centers */
	if ((ai->userbits&FIXANG) == 0)
	{
		getcenter(poly0, &fx, &fy);
		getcenter(poly1, &tx, &ty);
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Ensuring rigid arc %s"), describearcinst(ai));
#endif
		cla_domovearcinst(ai, fx, fy, tx, ty, arctyp);
		return;
	}

	/* get bounding boxes of polygons */
	getbbox(poly0, &lx0, &hx0, &ly0, &hy0);
	getbbox(poly1, &lx1, &hx1, &ly1, &hy1);

	/* if manhattan path runs between the ports, adjust the arcinst */
	if (lx0 <= hx1 && lx1 <= hx0)
	{
		/* arcinst runs vertically */
		tx = fx = (maxi(lx0,lx1) + mini(hx0,hx1)) / 2;
		fy = (ly0+hy0) / 2;   ty = (ly1+hy1) / 2;
		closestpoint(poly0, &fx, &fy);
		closestpoint(poly1, &tx, &ty);
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Ensuring manhattan arc %s"), describearcinst(ai));
#endif
		cla_domovearcinst(ai, fx, fy, tx, ty, arctyp);
		return;
	}
	if (ly0 <= hy1 && ly1 <= hy0)
	{
		/* arcinst runs horizontally */
		ty = fy = (maxi(ly0,ly1) + mini(hy0,hy1)) / 2;
		fx = (lx0+hx0) / 2;   tx = (lx1+hx1) / 2;
		closestpoint(poly0, &fx, &fy);
		closestpoint(poly1, &tx, &ty);
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("Ensuring manhattan arc %s"), describearcinst(ai));
#endif
		cla_domovearcinst(ai, fx, fy, tx, ty, arctyp);
		return;
	}

	/* give up and jog the arcinst */
	getcenter(poly0, &fx, &fy);
	getcenter(poly1, &tx, &ty);
	cla_domovearcinst(ai, fx, fy, tx, ty, arctyp);
}

/*
 * routine to update arc "ai" so that its ends run from (fx,fy) to (tx,ty).
 * The type of the arc is in "arctyp" for marking the change.
 */
void cla_updatearc(ARCINST *ai, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG arctyp)
{
	REGISTER INTBIG oldxA, oldyA, oldxB, oldyB, oldlen;

	(void)db_change((INTBIG)ai, OBJECTSTART, VARCINST, 0, 0, 0, 0, 0);

	/* save old arc state */
	oldxA = ai->end[0].xpos;   oldyA = ai->end[0].ypos;
	oldxB = ai->end[1].xpos;   oldyB = ai->end[1].ypos;
	oldlen = ai->length;

	/* set the proper arcinst position */
#ifdef CLA_DEBUG
	if (cla_conlaydebug)
		ttyputmsg(M_("Change arc %s from (%s,%s)-(%s,%s) to (%s,%s)-(%s,%s)"),
			describearcinst(ai), latoa(ai->end[0].xpos, 0), latoa(ai->end[0].ypos, 0),
				latoa(ai->end[1].xpos, 0), latoa(ai->end[1].ypos, 0),
					latoa(fx, 0), latoa(fy, 0), latoa(tx, 0), latoa(ty, 0));
#endif
	ai->end[0].xpos = fx;
	ai->end[0].ypos = fy;
	ai->end[1].xpos = tx;
	ai->end[1].ypos = ty;
	ai->length = computedistance(fx,fy, tx,ty);
	determineangle(ai);
	(void)setshrinkvalue(ai, TRUE);
	updategeom(ai->geom, ai->parent);

	/* see if the arc has changed before */
	if (((CHANGE *)ai->changeaddr) == NOCHANGE)
	{
		/* announce new arcinst change */
		ai->changeaddr = (CHAR *)db_change((INTBIG)ai, ARCINSTMOD, oldxA,
			oldyA, oldxB, oldyB, ai->width, oldlen);
		ai->changed = cla_changeclock + arctyp;
	}
	(void)db_change((INTBIG)ai, OBJECTEND, VARCINST, 0, 0, 0, 0, 0);
}

void cla_domovearcinst(ARCINST *ai, INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty, INTBIG arctyp)
{
	REGISTER NODEPROTO *np, *pnt;
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ar1, *ar2, *ar3;
	REGISTER PORTPROTO *pp, *fpt, *tpt;
	REGISTER NODEINST *no1, *no2, *fno, *tno;
	REGISTER INTBIG oldxA, oldyA, oldxB, oldyB, wid, oldbits, lx, hx, ly, hy;
	INTBIG psx, psy;

	/* check for null arcinst motion */
	if (fx == ai->end[0].xpos && fy == ai->end[0].ypos &&
		tx == ai->end[1].xpos && ty == ai->end[1].ypos)
	{
		/* only ignore null motion on fixed-angle requests */
		if (arctyp != 0) return;
	}

	/* if the angle is the same or doesn't need to be, simply make the change */
	if ((ai->userbits&FIXANG) == 0 ||
		((ai->userbits&FIXED) != 0 && ai->changed != cla_changeclock-1) ||
		ai->changed == cla_changeclock-2 ||
		(fx == tx && fy == ty) ||
		((((INTBIG)ai->userbits&AANGLE) >> AANGLESH)*10)%1800 == figureangle(fx, fy, tx, ty)%1800)
	{
		cla_updatearc(ai, fx, fy, tx, ty, arctyp);
		return;
	}
#ifdef	CLA_DEBUG
	if (cla_conlaydebug)
	{
		ttyputmsg(M_("Jogging arc %s (0%o) to run from (%s,%s) to (%s,%s)"),
			describearcinst(ai), ai, latoa(fx, 0), latoa(fy, 0), latoa(tx, 0), latoa(ty, 0));
	}
#endif

	/* manhattan arcinst becomes nonmanhattan: remember facts about it */
	fno = ai->end[0].nodeinst;   fpt = ai->end[0].portarcinst->proto;
	tno = ai->end[1].nodeinst;   tpt = ai->end[1].portarcinst->proto;
	ap = ai->proto;   pnt = ai->parent;   wid = ai->width;
	oldbits = ai->userbits;

	/* figure out what nodeinst proto connects these arcs */
	np = getpinproto(ap);
	defaultnodesize(np, &psx, &psy);

	/* replace it with three arcs and two nodes */
	if (ai->end[0].xpos == ai->end[1].xpos)
	{
		/* arcinst was vertical */
		oldyA = oldyB = (ty+fy) / 2;
		oldxA = fx;   oldxB = tx;
		lx = oldxB - psx/2; hx = lx + psx;
		ly = oldyB - psy/2; hy = ly + psy;
		no1 = db_newnodeinst(np, lx, hx, ly, hy, 0, 0, pnt);
		lx = oldxA - psx/2; hx = lx + psx;
		ly = oldyA - psy/2; hy = ly + psy;
		no2 = db_newnodeinst(np, lx, hx, ly, hy, 0, 0, pnt);
	} else
	{
		/* assume horizontal arcinst */
		oldyA = fy;   oldyB = ty;
		oldxA = oldxB = (tx+fx) / 2;
		lx = oldxB - psx/2; hx = lx + psx;
		ly = oldyB - psy/2; hy = ly + psy;
		no1 = db_newnodeinst(np, lx, hx, ly, hy, 0, 0, pnt);
		lx = oldxA - psx/2; hx = lx + psx;
		ly = oldyA - psy/2; hy = ly + psy;
		no2 = db_newnodeinst(np, lx, hx, ly, hy, 0, 0, pnt);
	}
	if (no1 == NONODEINST || no2 == NONODEINST)
	{
		ttyputerr(_("Problem creating jog pins"));
		return;
	}
	(void)db_change((INTBIG)no1, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
	(void)db_change((INTBIG)no2, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
	pp = np->firstportproto;
	ar1 = db_newarcinst(ap, wid, oldbits, fno, fpt, fx,fy, no2, pp, oldxA,oldyA, pnt);
	if ((oldbits&ISNEGATED) != 0) oldbits &= ~ISNEGATED;
	ar2 = db_newarcinst(ap, wid, oldbits, no2, pp, oldxA,oldyA, no1, pp, oldxB,oldyB, pnt);
	ar3 = db_newarcinst(ap, wid, oldbits, no1, pp, oldxB,oldyB, tno, tpt, tx,ty, pnt);
	if (ar1 == NOARCINST || ar2 == NOARCINST || ar3 == NOARCINST)
	{
		ttyputerr(_("Problem creating jog arcs"));
		return;
	}
	(void)copyvars((INTBIG)ai, VARCINST, (INTBIG)ar2, VARCINST, FALSE);
	(void)db_change((INTBIG)ar1, OBJECTEND, VARCINST, 0, 0, 0, 0, 0);
	(void)db_change((INTBIG)ar2, OBJECTEND, VARCINST, 0, 0, 0, 0, 0);
	(void)db_change((INTBIG)ar3, OBJECTEND, VARCINST, 0, 0, 0, 0, 0);
	ar1->changed = cla_changeclock + arctyp;
	ar2->changed = cla_changeclock + arctyp;
	ar3->changed = cla_changeclock + arctyp;

	/* now kill the arcinst */
	(void)db_change((INTBIG)ai, OBJECTSTART, VARCINST, 0, 0, 0, 0, 0);
	if ((CHANGE *)ai->changeaddr != NOCHANGE)
	{
		ai->end[0].xpos = ((CHANGE *)ai->changeaddr)->p1;
		ai->end[0].ypos = ((CHANGE *)ai->changeaddr)->p2;
		ai->end[1].xpos = ((CHANGE *)ai->changeaddr)->p3;
		ai->end[1].ypos = ((CHANGE *)ai->changeaddr)->p4;
		ai->length = computedistance(ai->end[0].xpos, ai->end[0].ypos,
			ai->end[1].xpos, ai->end[1].ypos);
		ai->width = ((CHANGE *)ai->changeaddr)->p5;
		determineangle(ai);
	}
	db_killarcinst(ai);
}

/*
 * routine to adjust the transformation matrix "trans" by placing translation
 * information for nodeinst "ni", port "pp".
 *
 * there are only two types of nodeinst changes: internal and external.
 * The internal changes are scaling and port motion changes that
 * are usually caused by other changes within the cell.  The external
 * changes are rotation and transposition.  These two changes never
 * occur at the same time.  There is also translation change that
 * can occur at any time and is of no importance here.  What is
 * important is that the transformation matrix "trans" handles
 * the external changes and internal changes.  External changes are already
 * set by the "makeangle" subroutine and internal changes are
 * built into the matrix here.
 */
void cla_adjustmatrix(NODEINST *ni, PORTPROTO *pp, XARRAY trans)
{
	REGISTER INTBIG ox, oy;
	INTBIG onox, onoy, dx, dy;

	if (((CHANGE *)ni->changeaddr)->p5 == ni->rotation &&
		((CHANGE *)ni->changeaddr)->p6 == ni->transpose)
	{
		/* nodeinst did not rotate: adjust for port motion */
		cla_oldportposition(ni, pp, &onox, &onoy);
		portposition(ni, pp, &dx, &dy);
		ox = (((CHANGE *)ni->changeaddr)->p1 + ((CHANGE *)ni->changeaddr)->p3) / 2;
		oy = (((CHANGE *)ni->changeaddr)->p2 + ((CHANGE *)ni->changeaddr)->p4) / 2;
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
		{
			ttyputmsg(M_("Node %s port %s was at (%s,%s), is at (%s,%s)"), describenodeinst(ni),
				pp->protoname, latoa(onox, 0), latoa(onoy, 0), latoa(dx, 0), latoa(dy, 0));
			ttyputmsg(M_("    Node moved (%s,%s)"), latoa(ox, 0), latoa(oy, 0));
		}
#endif
		trans[2][0] = dx - onox + ox;   trans[2][1] = dy - onoy + oy;
	} else
	{
		/* nodeinst rotated: adjust for nodeinst motion */
		trans[2][0] = (ni->lowx + ni->highx) / 2;
		trans[2][1] = (ni->lowy + ni->highy) / 2;
	}
}

/*
 * routine to compute the position of portproto "pp" on nodeinst "ni" and
 * place the center of the area in the parameters "x" and "y".  The position
 * is the "old" position, as determined by any changes that may have occured
 * to the nodeinst (and any sub-nodes).
 */
void cla_oldportposition(NODEINST *ni, PORTPROTO *pp, INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG lox, hix, loy, hiy;
	XARRAY localtran, subrot, temptr;
	static POLYGON *poly = NOPOLYGON;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly, 4, cla_constraint->cluster);

	/* descend to the primitive node */
	cla_makeoldrot(ni, subrot);
	while (ni->proto->primindex == 0)
	{
		cla_makeoldtrans(ni, localtran);
		transmult(localtran, subrot, temptr);
		if (((CHANGE *)pp->changeaddr) != NOCHANGE &&
			((CHANGE *)pp->changeaddr)->changetype == PORTPROTOMOD)
		{
			/* special code for moved port prototypes */
			ni = (NODEINST *)((CHANGE *)pp->changeaddr)->p1;
			pp = (PORTPROTO *)((CHANGE *)pp->changeaddr)->p2;
		} else
		{
			ni = pp->subnodeinst;
			pp = pp->subportproto;
		}
		cla_makeoldrot(ni, localtran);
		transmult(localtran, temptr, subrot);
	}

	/* save the actual extents of the nodeinst */
	lox = ni->lowx;   hix = ni->highx;
	loy = ni->lowy;   hiy = ni->highy;

	/* if the node changed, reset it temporarily */
	if (((CHANGE *)ni->changeaddr) != NOCHANGE &&
		((CHANGE *)ni->changeaddr)->changetype == NODEINSTMOD)
	{
		ni->lowx = ((CHANGE *)ni->changeaddr)->p1;
		ni->highx = ((CHANGE *)ni->changeaddr)->p3;
		ni->lowy = ((CHANGE *)ni->changeaddr)->p2;
		ni->highy = ((CHANGE *)ni->changeaddr)->p4;
	}

	/* now get the polygon describing the port */
	shapetransportpoly(ni, pp, poly, subrot);

	/* reset the bounds of the nodeinst */
	ni->lowx = lox;   ni->highx = hix;
	ni->lowy = loy;   ni->highy = hiy;

	/* compute the center of the port */
	getcenter(poly, x, y);
}

void cla_makeoldrot(NODEINST *ni, XARRAY trans)
{
	REGISTER INTBIG nlx, nly, nhx, nhy;
	REGISTER INTSML nr, nt;

	/* save values */
	nlx = ni->lowx;         nly = ni->lowy;
	nhx = ni->highx;        nhy = ni->highy;
	nr  = ni->rotation;     nt  = ni->transpose;

	/* set to previous values if they changed */
	if (((CHANGE *)ni->changeaddr) != NOCHANGE &&
		((CHANGE *)ni->changeaddr)->changetype == NODEINSTMOD)
	{
		ni->lowx      = ((CHANGE *)ni->changeaddr)->p1;
		ni->lowy      = ((CHANGE *)ni->changeaddr)->p2;
		ni->highx     = ((CHANGE *)ni->changeaddr)->p3;
		ni->highy     = ((CHANGE *)ni->changeaddr)->p4;
		ni->rotation  = (INTSML)((CHANGE *)ni->changeaddr)->p5;
		ni->transpose = (INTSML)((CHANGE *)ni->changeaddr)->p6;
	}

	/* create the former rotation matrix */
	makerot(ni, trans);

	/* restore values */
	ni->lowx     = nlx;     ni->lowy      = nly;
	ni->highx    = nhx;     ni->highy     = nhy;
	ni->rotation = nr;      ni->transpose = nt;
}

void cla_makeoldtrans(NODEINST *ni, XARRAY trans)
{
	REGISTER INTBIG nlx, nly, nhx, nhy, ntlx, ntly, nthx, nthy;
	REGISTER NODEPROTO *np;

	/* save values */
	np = ni->proto;
	nlx = ni->lowx;      nly = ni->lowy;
	nhx = ni->highx;     nhy = ni->highy;
	ntlx = np->lowx;      ntly = np->lowy;
	nthx = np->highx;     nthy = np->highy;

	/* set to previous values if they changed */
	if (((CHANGE *)ni->changeaddr) != NOCHANGE &&
		((CHANGE *)ni->changeaddr)->changetype == NODEINSTMOD)
	{
		ni->lowx  = ((CHANGE *)ni->changeaddr)->p1;
		ni->lowy  = ((CHANGE *)ni->changeaddr)->p2;
		ni->highx = ((CHANGE *)ni->changeaddr)->p3;
		ni->highy = ((CHANGE *)ni->changeaddr)->p4;
	}
	if (((CHANGE *)np->changeaddr) != NOCHANGE &&
		((CHANGE *)np->changeaddr)->changetype == NODEPROTOMOD)
	{
		np->lowx  = ((CHANGE *)np->changeaddr)->p1;
		np->highx = ((CHANGE *)np->changeaddr)->p2;
		np->lowy  = ((CHANGE *)np->changeaddr)->p3;
		np->highy = ((CHANGE *)np->changeaddr)->p4;
	}

	/* create the former translation matrix */
	maketrans(ni, trans);

	/* restore values */
	ni->lowx  = nlx;     ni->lowy  = nly;
	ni->highx = nhx;     ni->highy = nhy;
	np->lowx  = ntlx;     np->lowy  = ntly;
	np->highx = nthx;     np->highy = nthy;
}

/*
 * routine to re-compute the bounds of the cell "cell" (because an object
 * has been added or removed from it) and store these bounds in the nominal
 * size and the size of each instantiation of the cell.  It is also necessary
 * to re-position each instantiation of the cell in its proper position list.
 * If "forcedlook" is true, the cell is re-examined regardless of
 * whether its size changed.
 */
void cla_computecell(NODEPROTO *cell, BOOLEAN forcedlook)
{
	REGISTER NODEINST *ni, *lni;
	REGISTER NODEPROTO *np, *oneparent;
	INTBIG nlx, nhx, nly, nhy, offx, offy;
	XARRAY trans;
	REGISTER INTBIG dlx, dly, dhx, dhy, flx, fhx, fly, fhy;
	REGISTER BOOLEAN mixed;
	REGISTER CHANGE *c;
	REGISTER LIBRARY *lib;

#ifdef	CLA_DEBUG
	if (cla_conlaydebug)
		ttyputmsg(M_("In computecell on cell %s (fl=%ld)"),
			cell->protoname, forcedlook);
#endif
	/* get current boundary of cell */
	db_boundcell(cell, &nlx,&nhx, &nly,&nhy);

	/* quit if it has not changed */
	if (cell->lowx == nlx && cell->highx == nhx && cell->lowy == nly &&
		cell->highy == nhy && !forcedlook) return;

	/* advance the change clock */
	cla_changeclock += 4;

	/* get former size of cell from change information */
	flx = cell->lowx;   fhx = cell->highx;
	fly = cell->lowy;   fhy = cell->highy;
	c = (CHANGE *)cell->changeaddr;
	if (c != NOCHANGE && c->changetype == NODEPROTOMOD)
	{
		/* modification changes carry original size */
		flx = c->p1;   fhx = c->p2;
		fly = c->p3;   fhy = c->p4;
	}

	/* update the cell size */
	cell->lowx = nlx;   cell->highx = nhx;
	cell->lowy = nly;   cell->highy = nhy;
	cell->changeaddr = (CHAR *)db_change((INTBIG)cell, NODEPROTOMOD, flx, fhx, fly, fhy, 0, 0);

	/* see if all instances of this cell are in the same location */
	mixed = FALSE;
	oneparent = NONODEPROTO;
	lni = NONODEINST;
	for(ni = cell->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		oneparent = ni->parent;
		if (lni != NONODEINST) if (ni->parent != lni->parent) mixed = TRUE;
		lni = ni;
	}

	/* if there are no constrained instances of the cell, no change */
	if (oneparent == NONODEPROTO) return;

	/* if all parent cells the same, make changes to the instances */
	if (!mixed && !forcedlook)
	{
#ifdef	CLA_DEBUG
		if (cla_conlaydebug)
			ttyputmsg(M_("   Recomputing uniform parents of cell %s"),
				describenodeproto(cell));
#endif
		dlx = cell->lowx - flx;   dhx = cell->highx - fhx;
		dly = cell->lowy - fly;   dhy = cell->highy - fhy;
		for(ni = cell->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			makeangle(ni->rotation, ni->transpose, trans);
			xform(dhx+dlx, dhy+dly, &offx, &offy, trans);
			nlx = (dlx-dhx+offx)/2;  nhx = offx - nlx;
			nly = (dly-dhy+offy)/2;  nhy = offy - nly;
			if (cla_modifynodeinst(ni, nlx, nly, nhx, nhy, 0, 0, TRUE)) forcedlook = TRUE;
		}
		for(ni = cell->firstinst; ni != NONODEINST; ni = ni->nextinst)
			if (cla_modnodearcs(ni, 0, 0)) forcedlook = TRUE;
		cla_computecell(oneparent, forcedlook);
		return;
	}

	/*
	 * if instances are scattered or port motion has occured, examine
	 * entire database in proper recursive order and adjust cell sizes
	 */
#ifdef	CLA_DEBUG
	if (cla_conlaydebug)
		ttyputmsg(M_("   Performing complex tree examination"));
#endif
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->userbits &= ~(CELLMOD|CELLNOMOD);
	cell->userbits |= CELLMOD;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		/* only want cells with no instances as roots of trees */
		if (np->firstinst != NONODEINST) continue;

		/* now look recursively at the nodes in this cell */
		(void)cla_lookdown(np);
	}
}

BOOLEAN cla_lookdown(NODEPROTO *start)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG dlx, dhx, dly, dhy, flx, fhx, fly, fhy;
	REGISTER BOOLEAN forcedlook, foundone;
	INTBIG nlx, nhx, nly, nhy, offx, offy;
	XARRAY trans;

	/* first look recursively to the bottom to see if this cell changed */
	for(ni = start->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->proto->primindex == 0) ni->userbits |= MARKN; else
			ni->userbits &= ~MARKN;

	foundone = TRUE;
	while (foundone)
	{
		foundone = FALSE;
		for(ni = start->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if ((ni->userbits & MARKN) == 0) continue;
			ni->userbits &= ~MARKN;
			np = ni->proto;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(np, start)) continue;

			/* if this nodeinst is to change, mark the parent cell also */
			if ((np->userbits & CELLMOD) != 0) start->userbits |= CELLMOD;

			/* don't look inside if the cell is certified */
			if ((np->userbits & (CELLNOMOD|CELLMOD)) != 0) continue;

			/* look inside nodeinst to see if it changed */
			if (cla_lookdown(np)) start->userbits |= CELLMOD;
			foundone = TRUE;
		}
	}

	/* if this cell did not change, certify so and quit */
	if ((start->userbits & CELLMOD) == 0)
	{
		start->userbits |= CELLNOMOD;
		return(FALSE);
	}

	/* mark those nodes that must change */
	for(ni = start->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		ni->userbits &= ~(MARKN|TOUCHN);
		if (np->primindex != 0) continue;
		if (isiconof(np, start)) continue;
		if ((np->userbits & CELLMOD) == 0) continue;
		ni->userbits |= MARKN | TOUCHN;
	}
#ifdef	CLA_DEBUG
	if (cla_conlaydebug)
		ttyputmsg(M_("      Complex tree search at cell %s"), describenodeproto(start));
#endif
	/* modify the nodes in this cell that changed */
	forcedlook = FALSE;
	foundone = TRUE;
	while (foundone)
	{
		foundone = FALSE;
		for(ni = start->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if ((ni->userbits & MARKN) == 0) continue;
			ni->userbits &= ~MARKN;
			np = ni->proto;

			/* determine original size of cell */
			if ((CHANGE *)np->changeaddr != NOCHANGE &&
				((CHANGE *)np->changeaddr)->changetype == NODEPROTOMOD)
			{
				/* modification changes carry original size */
				flx = ((CHANGE *)np->changeaddr)->p1;
				fhx = ((CHANGE *)np->changeaddr)->p2;
				fly = ((CHANGE *)np->changeaddr)->p3;
				fhy = ((CHANGE *)np->changeaddr)->p4;
			} else
			{
				/* creation changes have no original size: use current size */
				flx = np->lowx;   fhx = np->highx;
				fly = np->lowy;   fhy = np->highy;
			}
			if (ni->highx-ni->lowx == np->highx-np->lowx && ni->highy-ni->lowy == np->highy-np->lowy)
				nlx = nhx = nly = nhy = 0; else
			{
				dlx = np->lowx - flx;   dhx = np->highx - fhx;
				dly = np->lowy - fly;   dhy = np->highy - fhy;
				makeangle(ni->rotation, ni->transpose, trans);
				xform(dhx+dlx, dhy+dly, &offx, &offy, trans);
				nlx = (dlx-dhx+offx)/2;  nhx = offx - nlx;
				nly = (dly-dhy+offy)/2;  nhy = offy - nly;
			}
			if (cla_modifynodeinst(ni, nlx, nly, nhx, nhy, 0, 0, TRUE)) forcedlook = TRUE;
			foundone = TRUE;
		}
	}

	/* now change the arcs in the nodes in this cell that changed */
	foundone = TRUE;
	while (foundone)
	{
		foundone = FALSE;
		for(ni = start->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if ((ni->userbits & TOUCHN) == 0) continue;
			ni->userbits &= ~TOUCHN;
			if (cla_modnodearcs(ni, 0, 0)) forcedlook = TRUE;
			foundone = TRUE;
		}
	}

	/* now change the size of this cell */
	db_boundcell(start, &nlx,&nhx, &nly,&nhy);

	/* quit if it has not changed */
	if (start->lowx == nlx && start->highx == nhx && start->lowy == nly &&
		start->highy == nhy && !forcedlook)
	{
		start->userbits |= CELLNOMOD;
		return(FALSE);
	}

	/* update the cell size */
	start->changeaddr = (CHAR *)db_change((INTBIG)start, NODEPROTOMOD,
		start->lowx, start->highx, start->lowy, start->highy, 0, 0);
	start->lowx = nlx;  start->highx = nhx;
	start->lowy = nly;  start->highy = nhy;
	return(TRUE);
}
