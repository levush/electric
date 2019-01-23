/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: conlin.c
 * Linear inequality constraint system
 * Written by: Steven M. Rubin, Static Free Software
 *
 * Copyright (c) 2001 Static Free Software.
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
#include "database.h"
#include "conlin.h"

/* command completion table for this constraint solver */
static COMCOMP cli_linconsp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to solve for linear inequalities"), 0};
static KEYWORD linconiopt[] =
{
	{x_("on"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP cli_linconip = {linconiopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("mode that assumes manhattan wires"), 0};
static KEYWORD lincontopt[] =
{
	{x_("go"),                   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name-nodes"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlight-equivalent"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP cli_lincontp = {lincontopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("text-equivalent mode"), 0};
static KEYWORD linconopt[] =
{
	{x_("solve"),                1,{&cli_linconsp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("debug-toggle"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("implicit-manhattan"),   1,{&cli_linconip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("text-equivalence"),     1,{&cli_lincontp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP cli_linconp = {linconopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("linear constraint system option"), 0};

/* global data in linear constraints */
       CONSTRAINT *cli_constraint;	/* the constraint object for this solver */
       BOOLEAN     cli_manhattan;	/* true to assume Manhattan constraints */
       INTBIG      cli_properties_key;	/* variable key for "CONLIN_properties" */
       NODEPROTO  *cli_curcell;		/* the current cell being equated to text */
       BOOLEAN     cli_ownchanges;	/* true if changes are internally generated */
       INTBIG      cli_textlines;	/* number of declaration/connection lines */
       BOOLEAN     cli_texton;		/* true if text/graphics system is on */
static CHAR       *cli_linconops[] = {x_(">="), x_("=="), x_("<=")};

static BOOLEAN cli_conlindebug;
static BOOLEAN cli_didsolve;

#define NOCONNODE ((CONNODE *)-1)
typedef struct Iconnode
{
	NODEINST  *realn;			/* actual node that is constrained */
	INTBIG     value;			/* value set on this node */
	BOOLEAN    solved;			/* flag for marking this node as solved */
	BOOLEAN    loopflag;		/* flag for solving loops */
	struct Iconarc *firstconarc;/* head of list of constraints on this node */
	struct Iconnode *nextconnode;/* next in list of constraint nodes */
} CONNODE;

#define NOCONARC ((CONARC *)-1)
typedef struct Iconarc
{
	ARCINST *reala;				/* actual arc that is constrained */
	CONNODE *other;				/* other node being constrained */
	INTBIG oper;				/* operator of the constraint */
	INTBIG value;				/* value of the constraint */
	struct Iconarc *nextconarc;	/* next constraint in linked list */
} CONARC;

static CONARC *cli_conarcfree;	/* list of free constraint arc modules */
static CONNODE *cli_connodefree;/* list of free constraint node modules */

/* references to routines in "conlay.c"*/
extern void cla_adjustmatrix(NODEINST*, PORTPROTO*, XARRAY);
extern void cla_oldportposition(NODEINST*, PORTPROTO*, INTBIG*, INTBIG*);

/* prototypes for local routines */
static CONARC *cli_allocconarc(void);
static void cli_freeconarc(CONARC*);
static CONNODE *cli_allocconnode(void);
static void cli_freeconnode(CONNODE*);
static void cli_dosolve(NODEPROTO*);
static BOOLEAN cli_buildconstraints(NODEPROTO*, CONNODE**, BOOLEAN, BOOLEAN);
static void cli_addconstraint(ARCINST*, CONNODE**, INTBIG, INTBIG, INTBIG, INTBIG, BOOLEAN);
static void cli_adjustconstraints(CONNODE*, BOOLEAN);
static BOOLEAN cli_solveconstraints(CONNODE*, BOOLEAN);
static BOOLEAN cli_satisfy(CONNODE*, CONARC*);
static void cli_applyconstraints(NODEPROTO*, CONNODE*, BOOLEAN);
static void cli_deleteconstraints(CONNODE*);
static void cli_printconstraints(CONNODE*, CHAR*, CHAR*);
static void cli_nameallnodes(NODEPROTO*);

/****************************** ALLOCATION ******************************/

/*
 * routine to allocate a constraint arc module from the pool (if any) or memory
 */
CONARC *cli_allocconarc(void)
{
	REGISTER CONARC *c;

	if (cli_conarcfree == NOCONARC)
	{
		c = (CONARC *)emalloc(sizeof (CONARC), cli_constraint->cluster);
		if (c == 0) return(NOCONARC);
	} else
	{
		c = cli_conarcfree;
		cli_conarcfree = (CONARC *)c->nextconarc;
	}
	return(c);
}

/*
 * routine to return constraint arc module "c" to the pool of free modules
 */
void cli_freeconarc(CONARC *c)
{
	c->nextconarc = cli_conarcfree;
	cli_conarcfree = c;
}

/*
 * routine to allocate a constraint node module from the pool (if any) or memory
 */
CONNODE *cli_allocconnode(void)
{
	REGISTER CONNODE *c;

	if (cli_connodefree == NOCONNODE)
	{
		c = (CONNODE *)emalloc(sizeof (CONNODE), cli_constraint->cluster);
		if (c == 0) return(NOCONNODE);
	} else
	{
		c = cli_connodefree;
		cli_connodefree = (CONNODE *)c->nextconnode;
	}
	return(c);
}

/*
 * routine to return constraint node module "c" to the pool of free modules
 */
void cli_freeconnode(CONNODE *c)
{
	c->nextconnode = cli_connodefree;
	cli_connodefree = c;
}

/****************************** DATABASE HOOKS ******************************/

void cli_linconinit(CONSTRAINT *con)
{
	if (con != NOCONSTRAINT) cli_constraint = con; else
	{
		/* only function during pass 2 of initialization */
		cli_conarcfree = NOCONARC;
		cli_connodefree = NOCONNODE;
		cli_conlindebug = FALSE;
		cli_didsolve = FALSE;
		cli_curcell = NONODEPROTO;
		cli_manhattan = TRUE;

		/* get the primary key for the properties variable */
		cli_properties_key = makekey(x_("CONLIN_properties"));

		/* initialize the text/graphics equivalence system */
		cli_ownchanges = FALSE;
		cli_texton = FALSE;
	}
}

void cli_linconterm(void) {}

void cli_linconsetmode(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pp;
	REGISTER NODEPROTO *np;
	REGISTER GEOM *geom;

	if (count == 0)
	{
		ttyputusage(x_("constraint tell linear OPTION"));
		return;
	}

	if (el_curconstraint != cli_constraint)
	{
		ttyputerr(M_("Must first switch to this solver with 'constraint use'"));
		return;
	}

	l = estrlen(pp = par[0]);

	if (namesamen(pp, x_("debug-toggle"), l) == 0 && l >= 1)
	{
		cli_conlindebug = !cli_conlindebug;
		if (cli_conlindebug) ttyputverbose(M_("Linear constraint debugging on")); else
			ttyputverbose(M_("Linear constraint debugging off"));
		return;
	}

	if (namesamen(pp, x_("text-equivalence"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			ttyputusage(x_("constraint tell linear text-equivalence OPTION"));
			return;
		}
		l = estrlen(pp = par[1]);
		if (namesamen(pp, x_("name-nodes"), l) == 0 && l >= 1)
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("Must have a current cell to generate names"));
				return;
			}
			cli_nameallnodes(np);
			return;
		}
		if (namesamen(pp, x_("go"), l) == 0 && l >= 1)
		{
			np = getcurcell();
			if (np == NONODEPROTO) ttyputverbose(M_("Enabling text/graphics association")); else
			{
				if (cli_texton)
				{
					if (np == cli_curcell)
						ttyputverbose(M_("Reevaluating textual description")); else
							ttyputverbose(M_("Switching association to cell %s"), describenodeproto(np));
				} else ttyputverbose(M_("Text window will mirror cell %s"), describenodeproto(np));
			}
			cli_maketextcell(np);
			return;
		}
		if (namesamen(pp, x_("highlight-equivalent"), l) == 0 && l >= 1)
		{
			geom = (GEOM *)asktool(us_tool, x_("get-object"));
			if (geom == NOGEOM)
			{
				ttyputerr(M_("Find a single object first"));
				return;
			}
			cli_highlightequivalent(geom);
			return;
		}
		ttyputbadusage(x_("constraint tell linear text-equivalence"));
		return;
	}

	if (namesamen(pp, x_("implicit-manhattan"), l) == 0 && l >= 1)
	{
		if (count < 2) l = estrlen(pp = x_("X")); else l = estrlen(pp = par[1]);
		if (namesame(pp, x_("on")) == 0)
		{
			cli_manhattan = TRUE;
			ttyputverbose(M_("Manhattan arcs will be kept that way"));
			return;
		}
		if (namesamen(pp, x_("off"), l) == 0 && l >= 2)
		{
			cli_manhattan = FALSE;
			ttyputverbose(M_("No assumptions will be made about Manhattan arcs"));
			return;
		}
		ttyputusage(x_("constraint tell linear implicit-manhattan (on|off)"));
		return;
	}

	if (namesamen(pp, x_("solve"), l) == 0 && l >= 1)
	{
		if (count < 2)
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("No cell specified and no current cell"));
				return;
			}
		} else
		{
			np = getnodeproto(par[1]);
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("No cell named %s"), par[1]);
				return;
			}
			if (np->primindex != 0)
			{
				ttyputerr(M_("Must solve cells, not primitives"));
				return;
			}
		}

		/* do constraint work */
		cli_solvecell(np, TRUE, TRUE);
		cli_didsolve = TRUE;
		return;
	}
	ttyputbadusage(x_("constraint tell linear"));
}

/*
 * the valid "command"s are:
 *    "describearc"  return a string that describes constraints on arc in "arg1".
 */
INTBIG cli_linconrequest(CHAR *command, INTBIG arg1)
{
	REGISTER INTBIG i, len, l;
	REGISTER VARIABLE *var;
	REGISTER LINCON *conptr;
	REGISTER void *infstr;

	l = estrlen(command);

	if (namesamen(command, x_("describearc"), l) == 0)
	{
		infstr = initinfstr();
		var = getvalkey(arg1, VARCINST, VINTEGER|VISARRAY, cli_properties_key);
		if (var != NOVARIABLE)
		{
			len = getlength(var) / LINCONSIZE;
			for(i=0; i<len; i++)
			{
				conptr = &((LINCON *)var->addr)[i];
				if (i != 0) addstringtoinfstr(infstr, x_(", "));
				addtoinfstr(infstr, (CHAR)(conptr->variable<=1 ? 'X' : 'Y'));
				addstringtoinfstr(infstr, cli_linconops[conptr->oper]);
				addstringtoinfstr(infstr, frtoa(conptr->value));
			}
		}
		return((INTBIG)returninfstr(infstr));
	}
	return(0);
}

void cli_linconsolve(NODEPROTO *np)
{
	REGISTER CHANGECELL *cc;
	REGISTER CHANGEBATCH *curbatch;

	if (np != NONODEPROTO)
	{
		cli_dosolve(np);
		return;
	}

	cli_eq_solve();

	curbatch = db_getcurrentbatch();
	if (curbatch != NOCHANGEBATCH)
		for(cc = curbatch->firstchangecell; cc != NOCHANGECELL; cc = cc->nextchangecell)
			cli_dosolve(cc->changecell);
	cli_didsolve = FALSE;
}

void cli_dosolve(NODEPROTO *cell)
{
	INTBIG nlx, nhx, nly, nhy;
	REGISTER NODEINST *ni;
	REGISTER INTBIG dlx, dhx, dly, dhy;

	if (cli_conlindebug)
		ttyputmsg(M_("Re-checking linear constraints for cell %s"), describenodeproto(cell));
	if (!cli_didsolve) cli_solvecell(cell, FALSE, FALSE);

	/* get current boundary of cell */
	db_boundcell(cell, &nlx,&nhx, &nly,&nhy);
	dlx = nlx - cell->lowx;  dhx = nhx - cell->highx;
	dly = nly - cell->lowy;  dhy = nhy - cell->highy;

	/* quit if it has not changed */
	if (dlx == 0 && dhx == 0 && dly == 0 && dhy == 0) return;

	/* update the cell size */
	cell->changeaddr = (CHAR *)db_change((INTBIG)cell, NODEPROTOMOD,
		cell->lowx, cell->lowy, cell->highx, cell->highy, 0, 0);
	cell->lowx = nlx;  cell->highx = nhx;
	cell->lowy = nly;  cell->highy = nhy;

	/* now update instances of the cell */
	for(ni = cell->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
		cli_linconmodifynodeinst(ni, dlx, dly, dhx, dhy, 0, 0);
		(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
	}
}

void cli_linconnewobject(INTBIG addr, INTBIG type)
{
	REGISTER ARCINST *ai;

	switch (type&VTYPE)
	{
		case VNODEINST: cli_eq_newnode((NODEINST *)addr);   break;
		case VARCINST:
			ai = (ARCINST *)addr;
			if (getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key) != NOVARIABLE)
			{
				db_setchangecell(ai->parent);
				db_forcehierarchicalanalysis(ai->parent);
			}
			cli_eq_newarc(ai);
			break;
		case VPORTPROTO: cli_eq_newport((PORTPROTO *)addr);   break;
	}
}

void cli_linconkillobject(INTBIG addr, INTBIG type)
{
	REGISTER ARCINST *ai;

	switch (type&VTYPE)
	{
		case VNODEINST: cli_eq_killnode((NODEINST *)addr);   break;
		case VARCINST:
			ai = (ARCINST *)addr;
			if (getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key) != NOVARIABLE)
			{
				db_setchangecell(ai->parent);
				db_forcehierarchicalanalysis(ai->parent);
			}
			cli_eq_killarc(ai);
			break;
		case VPORTPROTO: cli_eq_killport((PORTPROTO *)addr);   break;
	}
}

void cli_linconmodifynodeinst(NODEINST *ni, INTBIG dlx, INTBIG dly, INTBIG dhx, INTBIG dhy,
	INTBIG drot, INTBIG dtrans)
{
	REGISTER INTSML oldang, oldtrans;
	REGISTER INTBIG oldlx, oldhx, oldly, oldhy, ox, oy, oldxA, oldyA, oldxB, oldyB, oldlen,
		thisend, thatend;
	INTBIG ax[2], ay[2], onox, onoy, nox, noy;
	XARRAY trans;
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER CHANGE *cha;

	/* if simple rotation on transposed nodeinst, reverse rotation */
	if (ni->transpose != 0 && dtrans == 0) drot = (3600 - drot) % 3600;

	/* make changes to the nodeinst */
	oldang = ni->rotation;     ni->rotation = (INTSML)((ni->rotation + drot) % 3600);
	oldtrans = ni->transpose;  ni->transpose = (INTSML)((ni->transpose + dtrans) & 1);
	oldlx = ni->lowx;          ni->lowx += dlx;
	oldhx = ni->highx;         ni->highx += dhx;
	oldly = ni->lowy;          ni->lowy += dly;
	oldhy = ni->highy;         ni->highy += dhy;
	updategeom(ni->geom, ni->parent);

	/* mark that this nodeinst has changed */
	ni->changeaddr = (CHAR *)db_change((INTBIG)ni, NODEINSTMOD, oldlx, oldly,
		oldhx, oldhy, oldang, oldtrans);

	/* look at all of the arcs on this nodeinst */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;

		/* figure where each end of the arcinst is, ignore internal arcs */
		thisend = thatend = 0;
		if (ai->end[0].nodeinst == ni) thatend++;
		if (ai->end[1].nodeinst == ni) thisend++;
		if (thisend == thatend)
		{
			/* arc is internal: compute old center of nodeinst */
			cha = (CHANGE *)ni->changeaddr;
			ox = (cha->p1 + cha->p3) / 2;
			oy = (cha->p2 + cha->p4) / 2;

			/* prepare transformation matrix */
			makeangle(drot, dtrans, trans);

			/* determine the new ends of the arcinst */
			cla_adjustmatrix(ni, ai->end[0].portarcinst->proto, trans);
			xform(ai->end[0].xpos-ox, ai->end[0].ypos-oy, &ax[0], &ay[0], trans);
			cla_adjustmatrix(ni, ai->end[1].portarcinst->proto, trans);
			xform(ai->end[1].xpos-ox, ai->end[1].ypos-oy, &ax[1], &ay[1], trans);
		} else
		{
			/* external arc: compute its position on modified nodeinst */
			cla_oldportposition(ni,ai->end[thisend].portarcinst->proto,&onox,&onoy);
			portposition(ni, ai->end[thisend].portarcinst->proto, &nox, &noy);
			ax[thisend] = ai->end[thisend].xpos + nox - onox;
			ay[thisend] = ai->end[thisend].ypos + noy - onoy;

			/* set position of other end of arcinst */
			ax[thatend] = ai->end[thatend].xpos;
			ay[thatend] = ai->end[thatend].ypos;
		}

		/* check for null arcinst motion */
		if (ax[0] == ai->end[0].xpos && ay[0] == ai->end[0].ypos &&
			ax[1] == ai->end[1].xpos && ay[1] == ai->end[1].ypos) continue;

		/* demand constraint satisfaction if arc with properties moved */
		if (getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key) != NOVARIABLE)
		{
			db_setchangecell(ai->parent);
			db_forcehierarchicalanalysis(ai->parent);
		}

		(void)db_change((INTBIG)ai, OBJECTSTART, VARCINST, 0, 0, 0, 0, 0);

		/* save old state */
		oldxA = ai->end[0].xpos;   oldyA = ai->end[0].ypos;
		oldxB = ai->end[1].xpos;   oldyB = ai->end[1].ypos;
		oldlen = ai->length;

		/* set the proper arcinst position */
		ai->end[0].xpos = ax[0];
		ai->end[0].ypos = ay[0];
		ai->end[1].xpos = ax[1];
		ai->end[1].ypos = ay[1];
		ai->length = computedistance(ax[0],ay[0], ax[1],ay[1]);
		determineangle(ai);
		updategeom(ai->geom, ai->parent);

		/* announce new arcinst change */
		ai->changeaddr = (CHAR *)db_change((INTBIG)ai, ARCINSTMOD, oldxA,
			oldyA, oldxB, oldyB, ai->width, oldlen);
		(void)db_change((INTBIG)ai, OBJECTEND, VARCINST, 0, 0, 0, 0, 0);
		cli_eq_modarc(ai);
	}
	cli_eq_modnode(ni);
}

void cli_linconmodifynodeinsts(INTBIG count, NODEINST **ni, INTBIG *dlx, INTBIG *dly,
	INTBIG *dhx, INTBIG *dhy, INTBIG *drot, INTBIG *dtrans)
{
	REGISTER INTBIG i;

	for(i=0; i<count; i++)
		cli_linconmodifynodeinst(ni[i], dlx[i], dly[i], dhx[i], dhy[i], drot[i], dtrans[i]);
}

void cli_linconmodifyarcinst(ARCINST *ai, INTBIG oldx0, INTBIG oldy0, INTBIG oldx1, INTBIG oldy1, INTBIG oldwid, INTBIG oldlen)
{
	cli_eq_modarc(ai);
}

/*
 * routine to accept a constraint property in "changedata" for arcinst "ai".
 * The value of "changetype" determines what to do: 0 means set the new property,
 * 1 means add the new property, 2 means remove all properties, 3 means print all
 * properties.  The constraint is of the form "VARIABLE OPERATOR VALUE" where:
 *   VARIABLE = "x" | "y"
 *   OPERATOR = "==" | ">=" | "<="
 *   VALUE    = numeric constant in WHOLE units
 * the routine returns nonzero if the change is ineffective or impossible
 */
BOOLEAN cli_linconsetobject(INTBIG addr, INTBIG type, INTBIG changetype, INTBIG changedata)
{
	REGISTER ARCINST *ai;
	REGISTER CHAR *str, *pack, *pt;
	REGISTER BOOLEAN error;
	REGISTER INTBIG variable, value, len, i, oper;
	REGISTER LINCON *conptr;
	REGISTER VARIABLE *var;

	if ((type&VTYPE) != VARCINST) return(TRUE);
	ai = (ARCINST *)addr;

	/* handle printing of constraints */
	if (changetype == 3)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key);
		if (var == NOVARIABLE) ttyputmsg(M_("No linear constraint properties")); else
		{
			len = getlength(var) / LINCONSIZE;
			ttyputmsg(M_("%ld %s on this arc:"), len, makeplural(M_("constraint"), len));
			for(i=0; i<len; i++)
			{
				conptr = &((LINCON *)var->addr)[i];
				ttyputmsg(M_("Constraint %ld: %s %s %s"), i+1, (conptr->variable<=1 ? x_("x") : x_("y")),
					cli_linconops[conptr->oper], frtoa(conptr->value));
			}
		}
		return(FALSE);
	}

	/* handle deletion of constraints */
	if (changetype == 2)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key);
		if (var == NOVARIABLE)
		{
			ttyputerr(M_("No constraints on this arc"));
			return(TRUE);
		}
		len = getlength(var) / LINCONSIZE;
		(void)delvalkey((INTBIG)ai, VARCINST, cli_properties_key);
		cli_eq_modarc(ai);
		db_setchangecell(ai->parent);
		db_forcehierarchicalanalysis(ai->parent);
		return(FALSE);
	}

	/* adding a new constraint: parse it */
	str = (CHAR *)changedata;

	/* eliminate blanks in the string */
	for(pack = pt = str; *pt != 0; pt++) if (*pt != ' ') *pack++ = *pt;
	*pack = 0;

	/* make sure the constraint string is valid */
	error = FALSE;
	pt = str;

	/* get the variable */
	if (namesamen(pt, x_("x"), 1) == 0)
	{
		if (ai->end[0].xpos < ai->end[1].xpos) variable = 1; else
			variable = 0;
	} else if (namesamen(pt, x_("y"), 1) == 0)
	{
		if (ai->end[0].ypos < ai->end[1].ypos) variable = 3; else
			variable = 2;
	} else
	{
		ttyputerr(M_("Cannot parse variable (should be 'X' or 'Y')"));
		error = TRUE;
	}
	pt++;

	/* get the operator */
	for(i=0; i<3; i++)
		if (namesamen(pt, cli_linconops[i], estrlen(cli_linconops[i])) == 0)
			break;
	if (i < 3)
	{
		oper = i;
		pt += estrlen(cli_linconops[i]);
	} else
	{
		ttyputerr(M_("Cannot parse operator (must be '>=', '==', or '<=')"));
		error = TRUE;
	}

	/* get the value */
	if (!error && isanumber(pt))
	{
		value = atofr(pt);
		if (value < 0)
		{
			ttyputerr(M_("Values must be positive"));
			error = TRUE;
		}
	} else
	{
		ttyputerr(M_("Invalid number on right (%s)"), pt);
		error = TRUE;
	}

	if (error)
	{
		ttyputmsg(M_("Cannot accept linear constraint '%s'"), str);
		return(TRUE);
	}

	db_setchangecell(ai->parent);
	db_forcehierarchicalanalysis(ai->parent);
	if (cli_addarcconstraint(ai, variable, oper, value, changetype)) return(TRUE);
	if (changetype == 0) ttyputverbose(M_("Constraint set")); else
		ttyputverbose(M_("Constraint added"));
	return(FALSE);
}

/*
 * routine to add a constraint to arc "ai".  The constraint is on variable
 * "variable" (0=left, 1=right, 2=down, 3=up), operation "oper" (0 for >=,
 * 1 for ==, 2 for <=) and has a value of "value".  If "add" is nonzero, add
 * this constraint, otherwise replace existing constraints.  Returns true
 * if there is an error.
 */
BOOLEAN cli_addarcconstraint(ARCINST *ai, INTBIG variable, INTBIG oper, INTBIG value,
	INTBIG add)
{
	REGISTER VARIABLE *var;
	LINCON thiscon;
	REGISTER LINCON *conptr, *conlist;
	REGISTER INTBIG len, i;

	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key);
	if (var == NOVARIABLE || add == 0)
	{
		thiscon.variable = variable;
		thiscon.oper = oper;
		thiscon.value = value;
		(void)setvalkey((INTBIG)ai, VARCINST, cli_properties_key, (INTBIG)&thiscon,
			VINTEGER|VISARRAY|(LINCONSIZE<<VLENGTHSH));
		cli_eq_modarc(ai);
		return(FALSE);
	}

	len = getlength(var) / LINCONSIZE;
	conlist = (LINCON *)emalloc(((len+1) * (sizeof (LINCON))), el_tempcluster);
	if (conlist == 0)
	{
		ttyputnomemory();
		return(TRUE);
	}
	for(i=0; i<len; i++)
	{
		conptr = &((LINCON *)var->addr)[i];
		conlist[i].variable = conptr->variable;
		conlist[i].oper = conptr->oper;
		conlist[i].value    = conptr->value;
	}
	conlist[len].variable = variable;
	conlist[len].oper = oper;
	conlist[len].value    = value;
	(void)setvalkey((INTBIG)ai, VARCINST, cli_properties_key, (INTBIG)conlist,
		VINTEGER|VISARRAY|((LINCONSIZE*(len+1))<<VLENGTHSH));
	cli_eq_modarc(ai);
	efree((CHAR *)conlist);
	return(FALSE);
}

/*
 * routine to delete a constraint from arc "ai".  The constraint is on variable
 * "variable" (0=left, 1=right, 2=down, 3=up), operation "oper" (0 for >=,
 * 1 for ==, 2 for <=) and has a value of "value".
 */
void cli_deletearcconstraint(ARCINST *ai, INTBIG variable, INTBIG oper, INTBIG value)
{
	REGISTER VARIABLE *var;
	REGISTER LINCON *conptr, *conlist;
	REGISTER INTBIG len, i, j, k;

	var = getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key);
	if (var == NOVARIABLE) return;

	/* search for the constraint */
	len = getlength(var) / LINCONSIZE;
	for(i=0; i<len; i++)
	{
		conptr = &((LINCON *)var->addr)[i];
		if (conptr->variable != variable) continue;
		if (conptr->oper != oper) continue;
		if (conptr->value != value) continue;
		break;
	}
	if (i >= len) return;

	/* if this is the only constraint, delete the variable */
	if (len == 1)
	{
		(void)delvalkey((INTBIG)ai, VARCINST, cli_properties_key);
		cli_eq_modarc(ai);
		return;
	}

	conlist = (LINCON *)emalloc(((len-1) * (sizeof (LINCON))), el_tempcluster);
	if (conlist == 0)
	{
		ttyputnomemory();
		return;
	}
	k = 0;
	for(j=0; j<len; j++)
	{
		if (j == i) continue;
		conptr = &((LINCON *)var->addr)[j];
		conlist[k].variable = conptr->variable;
		conlist[k].oper = conptr->oper;
		conlist[k].value    = conptr->value;
		k++;
	}
	(void)setvalkey((INTBIG)ai, VARCINST, cli_properties_key, (INTBIG)conlist,
		VINTEGER|VISARRAY|((LINCONSIZE*(len-1))<<VLENGTHSH));
	efree((CHAR *)conlist);
	cli_eq_modarc(ai);
}

void cli_linconmodifyportproto(PORTPROTO *pp, NODEINST *oni, PORTPROTO *opp)
{
	cli_eq_modport(pp);
}

void cli_linconmodifynodeproto(NODEPROTO *np) {}
void cli_linconmodifydescript(INTBIG addr, INTBIG type, INTBIG key, UINTBIG *descript) {}
void cli_linconnewlib(LIBRARY *lib) {}
void cli_linconkilllib(LIBRARY *lib) {}

void cli_linconnewvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
	if ((newtype&VCREF) != 0) return;
	if (type == VARCINST)
	{
		if (key == cli_properties_key)
		{
			/* mark the parent of this arc as changed */
			db_setchangecell(((ARCINST *)addr)->parent);
			db_forcehierarchicalanalysis(((ARCINST *)addr)->parent);
		}
	}
	cli_eq_newvar(addr, type, key);
}

void cli_linconkillvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG oldaddr,
	INTBIG oldtype, UINTBIG *olddescript)
{
	if ((oldtype&VCREF) != 0) return;
	if ((type&VTYPE) == VARCINST)
	{
		if (key == cli_properties_key)
		{
			/* mark the parent of this arc as changed */
			db_setchangecell(((ARCINST *)addr)->parent);
			db_forcehierarchicalanalysis(((ARCINST *)addr)->parent);
		}
	}
	cli_eq_killvar(addr, type, key);
}

void cli_linconmodifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG stype, INTBIG aindex,
	INTBIG value)
{
	if (type == VARCINST)
	{
		if (key == cli_properties_key)
		{
			/* mark the parent of this arc as changed */
			db_setchangecell(((ARCINST *)addr)->parent);
			db_forcehierarchicalanalysis(((ARCINST *)addr)->parent);
		}
	}
}

void cli_linconinsertvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex) {}

void cli_lincondeletevariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG aindex,
	INTBIG oldval) {}

void cli_linconsetvariable(void) {}

/***************************** CONSTRAINT SOLVING *****************************/

/*
 * routine to solve the linear constraints in cell "np".  If "minimum" is
 * true, re-solve for minimum spacing even if the network is satisfied.
 * If "noanchors" is true, do not presume anchoring of the components that
 * just changed
 */
void cli_solvecell(NODEPROTO *np, BOOLEAN minimum, BOOLEAN noanchors)
{
	CONNODE *firstx, *firsty;
	REGISTER BOOLEAN failure, xnotsolved, ynotsolved;

	/* build constraint network in both axes */
	xnotsolved = cli_buildconstraints(np, &firstx, FALSE, noanchors);
	ynotsolved = cli_buildconstraints(np, &firsty, TRUE, noanchors);

	/* quit now if all constraints are solved */
	if (!xnotsolved && !ynotsolved && !minimum)
	{
		if (cli_conlindebug) ttyputmsg(M_("All constraints are met"));
		cli_deleteconstraints(firstx);
		cli_deleteconstraints(firsty);
		return;
	}

	/* adjust constraint network for node center offsets */
	cli_adjustconstraints(firstx, FALSE);
	cli_adjustconstraints(firsty, TRUE);

	if (cli_conlindebug)
	{
		cli_printconstraints(firstx, M_("Horizontal constraints:"), x_("X"));
		if (!xnotsolved) ttyputmsg(M_("HORIZONTAL CONSTRAINTS ARE ALL MET"));
		cli_printconstraints(firsty, M_("Vertical constraints:"), x_("Y"));
		if (!ynotsolved) ttyputmsg(M_("VERTICAL CONSTRAINTS ARE ALL MET"));
	}

	/* solve the networks and determine the success */
	failure = FALSE;
	if (xnotsolved || minimum)
	{
		if (cli_conlindebug) ttyputmsg(M_("Solving X constraints:"));
		if (cli_solveconstraints(firstx, minimum))
		{
			ttyputmsg(M_("X constraint network cannot be solved!"));
			failure = TRUE;
		}
	}
	if (ynotsolved || minimum)
	{
		if (cli_conlindebug) ttyputmsg(M_("Solving Y constraints:"));
		if (cli_solveconstraints(firsty, minimum))
		{
			ttyputmsg(M_("Y constraint network cannot be solved!"));
			failure = TRUE;
		}
	}

	/* if networks are solved properly, apply them to the circuit */
	if (!failure)
	{
		/* make the changes */
		if (xnotsolved || minimum) cli_applyconstraints(np, firstx, FALSE);
		if (ynotsolved || minimum) cli_applyconstraints(np, firsty, TRUE);
	}

	/* erase the constraint networks */
	cli_deleteconstraints(firstx);
	cli_deleteconstraints(firsty);
}

/*
 * routine to convert the constraints found in cell "np" into a network of
 * internal constraint nodes headed by "first".  If "axis" is false, build
 * the X constraints, otherwise build the Y constraints.  If "noanchors" is
 * true, do not presume presolution of changed nodes.  The routine returns
 * false if the constraints are already met.
 */
BOOLEAN cli_buildconstraints(NODEPROTO *np, CONNODE **first, BOOLEAN axis, BOOLEAN noanchors)
{
	REGISTER VARIABLE *var;
	REGISTER ARCINST *ai;
	REGISTER INTBIG i, rev, len, opf, opt, gothor, gotver;
	BOOLEAN notsolved;
	REGISTER INTBIG vaf, vat, lambda;
	REGISTER LINCON *conptr;
	REGISTER NODEINST *ni;
	REGISTER CONNODE *cn;

	/* get the value of lambda */
	lambda = lambdaofcell(np);

	/* mark all real nodes as un-converted to constraint nodes */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = -1;

	/* initialize the list of constraint nodes */
	*first = NOCONNODE;

	/* look at all constraints in the cell */
	notsolved = FALSE;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VINTEGER|VISARRAY, cli_properties_key);
		if (var == NOVARIABLE) len = 0; else len = getlength(var) / LINCONSIZE;
		gothor = gotver = 0;
		for(i=0; i<len; i++)
		{
			conptr = &((LINCON *)var->addr)[i];

			/* place "from" and "to" nodes in the correct orientation */
			switch (conptr->variable)
			{
				case CLLEFT:
					gothor++;
					if (axis) continue;
					rev = 1;
					break;
				case CLRIGHT:
					gothor++;
					if (axis) continue;
					rev = 0;
					break;
				case CLDOWN:
					gotver++;
					if (!axis) continue;
					rev = 1;
					break;
				case CLUP:
					gotver++;
					if (!axis) continue;
					rev = 0;
					break;
			}

			if (rev != 0)
			{
				opt = conptr->oper;       vat = conptr->value*lambda/WHOLE;
				opf = 2 - conptr->oper;   vaf = -vat;
			} else
			{
				opf = conptr->oper;       vaf = conptr->value*lambda/WHOLE;
				opt = 2 - conptr->oper;   vat = -vaf;
			}

			/* place the constraint in the list */
			cli_addconstraint(ai, first, opf, vaf, opt, vat, noanchors);

			/* see if this constraint is already met */
			switch (conptr->variable)
			{
				case CLLEFT:
				switch (conptr->oper)
				{
					case CLEQUALS:
						if (ai->end[0].xpos - ai->end[1].xpos != vat) notsolved = TRUE;
						break;
					case CLGEQUALS:
						if (ai->end[0].xpos - ai->end[1].xpos < vat) notsolved = TRUE;
						break;
					case CLLEQUALS:
						if (ai->end[0].xpos - ai->end[1].xpos > vat) notsolved = TRUE;
						break;
				}
				break;

				case CLRIGHT:
				switch (conptr->oper)
				{
					case CLEQUALS:
						if (ai->end[1].xpos - ai->end[0].xpos != vaf) notsolved = TRUE;
						break;
					case CLGEQUALS:
						if (ai->end[1].xpos - ai->end[0].xpos < vaf) notsolved = TRUE;
						break;
					case CLLEQUALS:
						if (ai->end[1].xpos - ai->end[0].xpos > vaf) notsolved = TRUE;
						break;
				}
				break;

				case CLDOWN:
				switch (conptr->oper)
				{
					case CLEQUALS:
						if (ai->end[0].ypos - ai->end[1].ypos != vat) notsolved = TRUE;
						break;
					case CLGEQUALS:
						if (ai->end[0].ypos - ai->end[1].ypos < vat) notsolved = TRUE;
						break;
					case CLLEQUALS:
						if (ai->end[0].ypos - ai->end[1].ypos > vat) notsolved = TRUE;
						break;
				}
				break;

				case CLUP:
				switch (conptr->oper)
				{
					case CLEQUALS:
						if (ai->end[1].ypos - ai->end[0].ypos != vaf) notsolved = TRUE;
						break;
					case CLGEQUALS:
						if (ai->end[1].ypos - ai->end[0].ypos < vaf) notsolved = TRUE;
						break;
					case CLLEQUALS:
						if (ai->end[1].ypos - ai->end[0].ypos > vaf) notsolved = TRUE;
						break;
				}
				break;
			}
		}

		/* if Manhattan mode set, add implicit constraints */
		if (cli_manhattan)
		{
			if (!axis)
			{
				/* handle vertical arcs */
				if (gothor == 0 && gotver != 0)
				{
					cli_addconstraint(ai, first, CLEQUALS, 0, CLEQUALS, 0, noanchors);
					if (ai->end[0].xpos != ai->end[1].xpos) notsolved = TRUE;
				}
			} else
			{
				/* handle horizontal arcs */
				if (gotver == 0 && gothor != 0)
				{
					cli_addconstraint(ai, first, CLEQUALS, 0, CLEQUALS, 0, noanchors);
					if (ai->end[0].ypos != ai->end[1].ypos) notsolved = TRUE;
				}
			}
		}
	}

	/* now assign proper values to each node */
	for(cn = *first; cn != NOCONNODE; cn = cn->nextconnode)
		if (!axis) cn->value = (cn->realn->highx + cn->realn->lowx) / 2; else
			cn->value = (cn->realn->highy + cn->realn->lowy) / 2;

	return(notsolved);
}

/*
 * routine to add a constraint between the nodes connected by arc "ai".  The
 * constraint is added to the list "first" and has operator "opf", value "vaf"
 * on the from end (0) and operator "opt", value "vat" on the to end (1).
 * If "noanchors" is true, do not presume anchoring and presolution of
 * nodes.
 */
void cli_addconstraint(ARCINST *ai, CONNODE **first, INTBIG opf, INTBIG vaf, INTBIG opt,
	INTBIG vat, BOOLEAN noanchors)
{
	REGISTER CONARC *caf, *cat;
	REGISTER CONNODE *cnf, *cnt;
	REGISTER CHANGE *ch;
	REGISTER CHANGEBATCH *curbatch;

	/* store this constraint as two constraint arcs */
	caf = cli_allocconarc();
	cat = cli_allocconarc();
	if (caf == NOCONARC || cat == NOCONARC)
	{
		ttyputnomemory();
		return;
	}

	/* get current change batch */
	curbatch = db_getcurrentbatch();

	/* get the "from" constraint node */
	cnf = (CONNODE *)ai->end[0].nodeinst->temp1;
	if (cnf == NOCONNODE)
	{
		cnf = cli_allocconnode();
		if (cnf == NOCONNODE)
		{
			ttyputnomemory();
			return;
		}
		cnf->realn = ai->end[0].nodeinst;
		cnf->solved = FALSE;
		if (!noanchors)
		{
			if (curbatch != NOCHANGEBATCH)
				for(ch = curbatch->changehead; ch != NOCHANGE; ch = ch->nextchange)
					if (ch->changetype == NODEINSTMOD && (NODEINST *)ch->entryaddr == cnf->realn)
						cnf->solved = TRUE;
		}
		cnf->firstconarc = NOCONARC;
		cnf->nextconnode = *first;
		*first = cnf;
		ai->end[0].nodeinst->temp1 = (INTBIG)cnf;
	}

	/* get the "to" constraint node */
	cnt = (CONNODE *)ai->end[1].nodeinst->temp1;
	if (cnt == NOCONNODE)
	{
		cnt = cli_allocconnode();
		if (cnt == NOCONNODE)
		{
			ttyputnomemory();
			return;
		}
		cnt->realn = ai->end[1].nodeinst;
		cnt->solved = FALSE;
		if (!noanchors)
		{
			if (curbatch != NOCHANGEBATCH)
				for(ch = curbatch->changehead; ch != NOCHANGE; ch = ch->nextchange)
					if (ch->changetype == NODEINSTMOD && (NODEINST *)ch->entryaddr == cnt->realn)
						cnt->solved = TRUE;
		}
		cnt->firstconarc = NOCONARC;
		cnt->nextconnode = *first;
		*first = cnt;
		ai->end[1].nodeinst->temp1 = (INTBIG)cnt;
	}

	/* append constraint arc "cat" to constraint node "cnt" */
	cat->nextconarc = cnt->firstconarc;
	cnt->firstconarc = cat;
	cat->other = cnf;

	/* append constraint arc "caf" to constraint node "cnf" */
	caf->nextconarc = cnf->firstconarc;
	cnf->firstconarc = caf;
	caf->other = cnt;

	caf->oper = opf;
	caf->value = vaf;
	cat->oper = opt;
	cat->value = vat;
	cat->reala = caf->reala = ai;
}

void cli_adjustconstraints(CONNODE *first, BOOLEAN axis)
{
	REGISTER CONNODE *cn;
	REGISTER CONARC *ca;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *nif, *nit;
	REGISTER INTBIG centerf, centert, diff;
	REGISTER INTBIG fromend, toend;

	for(cn = first; cn != NOCONNODE; cn = cn->nextconnode)
	{
		for(ca = cn->firstconarc; ca != NOCONARC; ca = ca->nextconarc)
		{
			ai = ca->reala;
			if (ai->end[0].nodeinst == cn->realn)
			{
				fromend = 0;   toend = 1;
			} else
			{
				fromend = 1;   toend = 0;
			}
			nif = cn->realn;   nit = ca->other->realn;

			/* adjust constraint to get arc to node center */
			if (!axis)
			{
				/* adjust for X axis constraints */
				centerf = (nif->lowx + nif->highx) / 2;
				centert = (nit->lowx + nit->highx) / 2;
				if (centerf != ai->end[fromend].xpos)
				{
					diff = ai->end[fromend].xpos - centerf;
					ca->value += diff;
				}
				if (centert != ai->end[toend].xpos)
				{
					diff = centert - ai->end[toend].xpos;
					ca->value += diff;
				}
			} else
			{
				/* adjust for Y axis constraints */
				centerf = (nif->lowy + nif->highy) / 2;
				centert = (nit->lowy + nit->highy) / 2;
				if (centerf != ai->end[fromend].ypos)
				{
					diff = ai->end[fromend].ypos - centerf;
					ca->value += diff;
				}
				if (centert != ai->end[toend].ypos)
				{
					diff = centert - ai->end[toend].ypos;
					ca->value += diff;
				}
			}
		}
	}
}

/*
 * routine to solve the constraint network in "first".  If "minimum" is true,
 * reduce to minimum distances (rahter than allowing larger values)
 */
BOOLEAN cli_solveconstraints(CONNODE *first, BOOLEAN minimum)
{
	REGISTER CONNODE *cn, *cno;
	REGISTER CONARC *ca;
	REGISTER BOOLEAN satisfied;

	/* if there are no constraints, all is well */
	if (first == NOCONNODE) return(FALSE);

	/* fill in the values */
	for (;;)
	{
		/* choose a node that is solved and constrains to one that is not */
		for(cn = first; cn != NOCONNODE; cn = cn->nextconnode)
		{
			if (!cn->solved) continue;
			for(ca = cn->firstconarc; ca != NOCONARC; ca = ca->nextconarc)
				if (!ca->other->solved) break;
			if (ca != NOCONARC) break;
		}

		/* if no constraints relate solved to unsolved, take any unsolved */
		if (cn == NOCONNODE)
		{
			for(cn = first; cn != NOCONNODE; cn = cn->nextconnode)
				if (!cn->solved) break;
			if (cn == NOCONNODE) break;

			/* mark this constraint node as solved */
			cn->solved = TRUE;
		}

		if (cli_conlindebug) ttyputmsg(M_("  Working from node %s at %ld"),
			describenodeinst(cn->realn), cn->value);

		/* check all constraint arcs to other nodes */
		for(ca = cn->firstconarc; ca != NOCONARC; ca = ca->nextconarc)
		{
			/* see if other node meets the constraint */
			satisfied = FALSE;
			switch (ca->oper)
			{
				case CLEQUALS:
					if (ca->other->value == cn->value + ca->value) satisfied = TRUE;
					break;
				case CLGEQUALS:
					if (ca->other->value >= cn->value + ca->value) satisfied = TRUE;
					break;
				case CLLEQUALS:
					if (ca->other->value <= cn->value + ca->value) satisfied = TRUE;
					break;
			}

			/* if the constraint meets perfectly, ignore it */
			if (ca->other->value == cn->value + ca->value)
			{
				if (cli_conlindebug)
					ttyputmsg(M_("    Constraint to %s is met perfectly"),
						describenodeinst(ca->other->realn));
				ca->other->solved = TRUE;
				continue;
			}

			/* if the constraint is ok and doesn't have to be exact, ignore it */
			if (satisfied && !minimum)
			{
				if (cli_conlindebug)
					ttyputmsg(M_("    Constraint to %s is met adequately"),
						describenodeinst(ca->other->realn));
				ca->other->solved = TRUE;
				continue;
			}

			/* if other node is unsolved, set a value on it */
			if (!ca->other->solved)
			{
				ca->other->value = cn->value + ca->value;
				if (cli_conlindebug)
					ttyputmsg(M_("    Constraint to unsolved %s adjusts it to %ld"),
						describenodeinst(ca->other->realn), ca->other->value);
				ca->other->solved = TRUE;
				continue;
			}

			/* other node solved: solve for this node's value */
			for(cno = first; cno != NOCONNODE; cno = cno->nextconnode)
				cno->loopflag = FALSE;
			cn->loopflag = TRUE;
			if (cli_conlindebug)
				ttyputmsg(M_("    Constraint to solved %s takes some work:"),
					describenodeinst(ca->other->realn));
			if (cli_satisfy(cn, ca)) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * routine to iteratively force constraint arc "ca" on constrant node "cn"
 * to be satisfied
 */
BOOLEAN cli_satisfy(CONNODE *cn, CONARC *ca)
{
	REGISTER CONNODE *cno;
	REGISTER CONARC *cao;
	REGISTER BOOLEAN satisfied;

	/* get the constraint node on the other end */
	cno = ca->other;
	cno->loopflag = TRUE;

	/* set new value for that constraint node */
	cno->value = cn->value + ca->value;
	if (cli_conlindebug) ttyputmsg(M_("      Setting %s to %ld"),
		describenodeinst(cno->realn), cno->value);

	/* see if that constraint node is now satisfied */
	for(cao=cno->firstconarc; cao!=NOCONARC; cao=cao->nextconarc)
	{
		if (!cao->other->solved) continue;
		satisfied = FALSE;
		switch (cao->oper)
		{
			case CLEQUALS:
				if (cao->other->value == cno->value + cao->value) satisfied = TRUE;
				break;
			case CLGEQUALS:
				if (cao->other->value >= cno->value + cao->value) satisfied = TRUE;
				break;
			case CLLEQUALS:
				if (cao->other->value <= cno->value + cao->value) satisfied = TRUE;
				break;
		}
		if (!satisfied)
		{
			/* if this is a loop, quit now */
			if (cao->other->loopflag) return(TRUE);

			/* attempt to recursively satisfy the constraint */
			if (cli_satisfy(cno, cao)) return(TRUE);
		}
	}
	return(FALSE);
}

void cli_applyconstraints(NODEPROTO *np, CONNODE *first, BOOLEAN axis)
{
	REGISTER CONNODE *cn;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG lowestcon, lowestcir, center, adjust, oldlx, oldhx,
		oldly, oldhy, oldxA, oldyA, oldxB, oldyB, oldlen;

	/* if there are no constraints, circuit is solved */
	if (first == NOCONNODE) return;

	/* look for the lowest value in the constraints and the circuit */
	lowestcon = lowestcir = 0;

	/* mark this cell as changed so its size will be adjusted */
	db_setchangecell(first->realn->parent);

	/* now adjust the nodes in the circuit */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp1 = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		ni->temp1 = 0;
	for(cn = first; cn != NOCONNODE; cn = cn->nextconnode)
	{
		ni = cn->realn;
		if (!axis) center = (ni->lowx + ni->highx) / 2; else
			center = (ni->lowy + ni->highy) / 2;
		adjust = (cn->value - lowestcon) - (center - lowestcir);
		if (adjust == 0) continue;
		ni->temp1 = adjust;
		oldlx = ni->lowx;     oldhx = ni->highx;
		oldly = ni->lowy;     oldhy = ni->highy;
		(void)db_change((INTBIG)ni, OBJECTSTART, VNODEINST, 0, 0, 0, 0, 0);
		if (!axis)
		{
			ni->lowx += adjust;   ni->highx += adjust;
		} else
		{
			ni->lowy += adjust;   ni->highy += adjust;
		}
		updategeom(ni->geom, ni->parent);
		ni->changeaddr = (CHAR *)db_change((INTBIG)ni, NODEINSTMOD, oldlx,
			oldly, oldhx, oldhy, ni->rotation, ni->transpose);
		(void)db_change((INTBIG)ni, OBJECTEND, VNODEINST, 0, 0, 0, 0, 0);
		cli_eq_modnode(ni);
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			pi->conarcinst->temp1 = 1;
	}

	/* finally adjust the arcs in the circuit */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 == 0) continue;
		oldxA = ai->end[0].xpos;   oldyA = ai->end[0].ypos;
		oldxB = ai->end[1].xpos;   oldyB = ai->end[1].ypos;
		oldlen = ai->length;
		(void)db_change((INTBIG)ai, OBJECTSTART, VARCINST, 0, 0, 0, 0, 0);
		if (!axis)
		{
			ai->end[0].xpos += ai->end[0].nodeinst->temp1;
			ai->end[1].xpos += ai->end[1].nodeinst->temp1;
		} else
		{
			ai->end[0].ypos += ai->end[0].nodeinst->temp1;
			ai->end[1].ypos += ai->end[1].nodeinst->temp1;
		}
		determineangle(ai);
		updategeom(ai->geom, ai->parent);
		ai->length = computedistance(ai->end[0].xpos,ai->end[0].ypos,
			ai->end[1].xpos,ai->end[1].ypos);
		ai->changeaddr = (CHAR *)db_change((INTBIG)ai, ARCINSTMOD, oldxA,
			oldyA, oldxB, oldyB, ai->width, oldlen);
		(void)db_change((INTBIG)ai, OBJECTEND, VARCINST, 0, 0, 0, 0, 0);
		cli_eq_modarc(ai);
	}
}

void cli_deleteconstraints(CONNODE *first)
{
	REGISTER CONNODE *cn, *nextcn;
	REGISTER CONARC *ca, *nextca;

	for(cn = first; cn != NOCONNODE; cn = nextcn)
	{
		for(ca = cn->firstconarc; ca != NOCONARC; ca = nextca)
		{
			nextca = ca->nextconarc;
			cli_freeconarc(ca);
		}
		nextcn = cn->nextconnode;
		cli_freeconnode(cn);
	}
}

void cli_printconstraints(CONNODE *first, CHAR *message, CHAR *axis)
{
	REGISTER CONNODE *cn;
	REGISTER CONARC *ca;

	ttyputmsg(x_("%s"), message);
	for(cn = first; cn != NOCONNODE; cn = cn->nextconnode)
	{
		ttyputmsg(M_("  Node %s at %ld anchored=%d has these constraints:"),
			describenodeinst(cn->realn), cn->value, cn->solved);
		for(ca = cn->firstconarc; ca != NOCONARC; ca = ca->nextconarc)
			ttyputmsg(M_("    to node %s (%s %s %ld)"), describenodeinst(ca->other->realn), axis,
				cli_linconops[ca->oper], ca->value);
	}
}

/*
 * routine to name all of the nodes in cell "np"
 */
#define NOABBREV ((ABBREV *)-1)

typedef struct Iabbrev
{
	CHAR   *name;
	INTBIG  aindex;
	struct Iabbrev *nextabbrev;
} ABBREV;

void cli_nameallnodes(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER CHAR *pt;
	REGISTER BOOLEAN upper;
	REGISTER VARIABLE *var;
	REGISTER ABBREV *firstabbrev, *a, *nexta;
	CHAR line[20], *newname;
	REGISTER void *infstr;

	firstabbrev = NOABBREV;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore this if it has a name */
		if (getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key) != NOVARIABLE) continue;

		if (ni->proto->primindex == 0) upper = TRUE; else upper = FALSE;
		pt = makeabbrev(ni->proto->protoname, upper);

		/* if the name is null, assign a name */
		if (estrlen(pt) == 0)
		{
			if (ni->proto->primindex == 0) pt = M_("CELL"); else pt = M_("node");
		}

		/* find an abbreviation module for this */
		for(a = firstabbrev; a != NOABBREV; a = a->nextabbrev)
			if (namesame(a->name, pt) == 0) break;
		if (a == NOABBREV)
		{
			a = (ABBREV *)emalloc(sizeof (ABBREV), el_tempcluster);
			if (a == 0) break;
			(void)allocstring(&a->name, pt, el_tempcluster);
			a->nextabbrev = firstabbrev;
			firstabbrev = a;
			a->aindex = 1;
		}

		/* assign a name to the node */
		infstr = initinfstr();
		addstringtoinfstr(infstr, a->name);
		(void)esnprintf(line, 20, x_("%ld"), a->aindex++);
		addstringtoinfstr(infstr, line);
		allocstring(&newname, returninfstr(infstr), el_tempcluster);
		var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)newname, VSTRING|VDISPLAY);
		efree(newname);
		if (var != NOVARIABLE)
			defaulttextsize(3, var->textdescript);
	}

	/* free the abbreviation keys */
	for(a = firstabbrev; a != NOABBREV; a = nexta)
	{
		nexta = a->nextabbrev;
		efree(a->name);
		efree((CHAR *)a);
	}
}
