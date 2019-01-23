/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: ioecado.c
 * Input/output aid: ecad output
 * Written by: Steven M. Rubin, Static Free Software
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
 * ----------------- FORMAT ----------------------
 * ADD SO14 'IC1' R0 (0 0)
 * ADD SO16 'IC2' R0 (0 0)
 * ADD SO24L 'IC3' R0 (0 0)
 * ;
 * Signal 'A0'       'IC1'      '5' \
 *                   'IC2'      '10' \
 *                   'IC3'      '8' \
 * ;
 * Signal 'A1'       'IC1'      '6' \
 *                   'IC2'      '11' \
 *                   'IC3'      '5' \
 * ;
 */
/*
 * ----------------- ECAD NETLIST FORMAT ----------------------
 .cmp (t=pkg_type) ref_des (x=0, y=0)


  	.cmp (t=LED) LED1
 	.cmp (t=RES100) R1

Net_name : ref_des pad; \
           ref_des pad;

 	A0 : LED1 1; R1 2;
 	A1 : LED1 2; R1 1;

*/

#include "config.h"
#include "global.h"
#include "eio.h"

#define NOECADNET ((ECADNET *)-1)

typedef struct Inextecadnet
{
	CHAR *netname;
	CHAR *packagename;
	CHAR *pinname;
	struct Inextecadnet *nextecadnet;
} ECADNET;

static ECADNET *io_ecadfirstnet;

/* prototypes for local routines */
static INTBIG io_ecaddumpcell(NODEPROTO *curcell, CHAR *prefix);
static void io_ecadmarkcells(NODEPROTO *curcell);

BOOLEAN io_writeecadlibrary(LIBRARY *lib)
{
	CHAR file[100], *truename;
	REGISTER INTBIG i, sorted, maxnetname, maxpackagename;
	REGISTER CHAR *name;
	REGISTER NODEPROTO *curcell;
	REGISTER ECADNET *en, *lasten, *nexten;
	REGISTER PORTPROTO *pp;

	/* create the proper disk file for the ECAD */
	curcell = lib->curnodeproto;
	if (curcell == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell to generate ECAD output"));
		return(TRUE);
	}
	(void)estrcpy(file, curcell->protoname);
	(void)estrcat(file, x_(".enl"));
	name = truepath(file);
	io_fileout = xcreate(name, io_filetypeecad, _("ECAD File"), &truename);
	if (io_fileout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return(TRUE);
	}

	/* recurse through the circuit, gathering network connections */
	io_ecadfirstnet = NOECADNET;
	for(pp = curcell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		pp->temp1 = 0;
	if (io_ecaddumpcell(curcell, x_("")) != 0)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));
	xprintf(io_fileout, x_("| end of part list\n"));  /* cm - removed before netlist */

	/* warn the user if nets not found */
	if (io_ecadfirstnet == NOECADNET)
	{
		ttyputmsg(_("Packages with attribute 'ref_des' not found or their ports are without 'pin' attribute"));
	}

	/* sort the signals */
	sorted = 0;
	while (sorted == 0)
	{
		sorted = 1;
		lasten = NOECADNET;
		for(en = io_ecadfirstnet; en != NOECADNET; en = en->nextecadnet)
		{
			nexten = en->nextecadnet;
			if (nexten == NOECADNET) break;
			if (namesame(en->netname, nexten->netname) > 0)
			{
				if (lasten == NOECADNET) io_ecadfirstnet = nexten; else
					lasten->nextecadnet = nexten;
				en->nextecadnet = nexten->nextecadnet;
				nexten->nextecadnet = en;
				en = nexten;
				sorted = 0;
			}
			lasten = en;
		}
	}

	/* determine longest name */
	maxnetname = maxpackagename = 0;
	for(en = io_ecadfirstnet; en != NOECADNET; en = en->nextecadnet)
	{
		i = estrlen(en->netname);
		if (i > maxnetname) maxnetname = i;
		i = estrlen(en->packagename);
		if (i > maxpackagename) maxpackagename = i;
	}
	maxnetname += 4;
	maxpackagename += 4;

	/* write networks */
	lasten = NOECADNET;
	for(en = io_ecadfirstnet; en != NOECADNET; en = en->nextecadnet)
	{
		if (lasten == NOECADNET || namesame(lasten->netname, en->netname) != 0)
		   {
			if (lasten != NOECADNET)
			xprintf(io_fileout, x_(";\n"));
			xprintf(io_fileout, x_("%s : "), en->netname);            /* cm - write netname on first line of net */
		   }
		else
		   {
			xprintf(io_fileout, x_("      "));                           /* cm - was net name on each line */
		   }
		for(i=estrlen(en->netname); i<maxnetname; i++)
			xprintf(io_fileout, x_(" "));

		/* write "Part" column */
		xprintf(io_fileout, x_("%s"), en->packagename);                 /* cm - removed single quotes */
		for(i=estrlen(en->packagename); i<maxpackagename; i++)
			xprintf(io_fileout, x_(" "));

		/* write "Pad" column (an attribute with a number) */
		xprintf(io_fileout, x_("%s \\\n"), en->pinname);                   /* cm - removed single quotes */

      lasten = en;
	}

	if (lasten != NOECADNET) xprintf(io_fileout, x_(";    |end of net section.\n")); /* cm - end of net section */

	/* free memory */
	while (io_ecadfirstnet != NOECADNET)
	   {
		en = io_ecadfirstnet;
		io_ecadfirstnet = en->nextecadnet;
		efree(en->netname);
		efree(en->packagename);
		efree(en->pinname);
		efree((CHAR *)en);
	   }

	/* clean up */
	xclose(io_fileout);

	/* tell the user that the file is written */
	ttyputmsg(_("%s written"), truename);
	return(FALSE);
}

/*
 * Routine to write out all cells in the current library that are marked (temp1 != 0).
 * Returns nonzero if backannotation has been added.
 */
INTBIG io_ecaddumpcell(NODEPROTO *curcell, CHAR *prefix)
{
	CHAR *name;
	CHAR line[50], *newprefix, *tempname;
	REGISTER NODEPROTO *np, *subnp, *subcnp;
	REGISTER NETWORK *net;
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *varpart, *var, *varpackagetype;
	REGISTER PORTARCINST *pi;
	REGISTER PORTPROTO *pp;
	REGISTER ECADNET *en;
	REGISTER INTBIG backannotate;
	REGISTER void *infstr;

	/* make sure that all arcs have network names on them */
	backannotate = 0;
	if (asktool(net_tool, x_("name-nets"), (INTBIG)curcell) != 0) backannotate++;

	/* mark those cells that will be included in this dump */
	io_ecadmarkcells(curcell);

	/* write the symbols */
	for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->temp1 == 0) continue;

		/* if this is the main entry cell, pickup net names */
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			net->temp1 = 0;
		if (np == curcell)
		{
			for(pp = curcell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (pp->temp1 == 0) continue;
				pp->network->temp1 = pp->temp1;
			}
		}

		/* dump symbols in this cell */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* see if this node has a part name */
			if (ni->proto->primindex != 0) continue;
			subnp = ni->proto;

			/* ignore recursive references (showing icon in contents) */
			if (isiconof(subnp, np)) continue;

			varpart = getval((INTBIG)ni, VNODEINST, -1, x_("ATTR_ref_des"));   /* cm - changed for ecad */
			if (varpart == NOVARIABLE)
			{
				subcnp = contentsview(subnp);
				if (subcnp != NONODEPROTO) subnp = subcnp;

				/* compute new prefix for nets in the subcell */
				infstr = initinfstr();
				addstringtoinfstr(infstr, prefix);
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var == NOVARIABLE)
				{
					esnprintf(line, 50, x_("NODE%ld."), (INTBIG)ni);
					addstringtoinfstr(infstr, line);
				} else
				{
					addstringtoinfstr(infstr, (CHAR *)var->addr);
					addtoinfstr(infstr, '.');
				}
				(void)allocstring(&newprefix, returninfstr(infstr), el_tempcluster);

				/* store net names on ports for use inside the subcell */
				for(pp = subnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					pp->temp1 = 0;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					pp = equivalentport(ni->parent, pi->proto, subnp);
					if (pp == NOPORTPROTO || pp->temp1 != 0) continue;
					net = pi->conarcinst->network;
					infstr = initinfstr();
					addstringtoinfstr(infstr, prefix);
					addstringtoinfstr(infstr, networkname(net, 0));
					(void)allocstring(&tempname, returninfstr(infstr), io_tool->cluster);
					pp->temp1 = (INTBIG)tempname;
				}

				/* recurse */
				if (io_ecaddumpcell(subnp, newprefix) != 0) backannotate++;

				/* remark the cells at the current level and clean up memory */
				io_ecadmarkcells(curcell);
				efree(newprefix);
				for(pp = subnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					if (pp->temp1 != 0) efree((CHAR *)pp->temp1);
			} else
			{
				/* found package at bottom of hierarchy */
				(void)allocstring(&name, describevariable(varpart, -1, -1), el_tempcluster);
				varpackagetype = getval((INTBIG)ni, VNODEINST, -1, x_("ATTR_pkg_type"));
				if (varpackagetype == NOVARIABLE)
				{
					xprintf(io_fileout, x_("Error no pkg_type attribute for %s%s"), prefix, name);
				} else
				{
					xprintf(io_fileout, x_(".cmp (t=%s) %s%s (x=0, y=0)"), describevariable(varpackagetype, -1, -1),
						prefix, name);
				}
				xprintf(io_fileout, x_("\n"));

				/* record all networks */
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					/* get "Pad" information */
					infstr = initinfstr();
					addstringtoinfstr(infstr, x_("ATTRP_"));
					addstringtoinfstr(infstr, pi->proto->protoname);
					addstringtoinfstr(infstr, x_("_pin"));
					var = getval((INTBIG)ni, VNODEINST, -1, returninfstr(infstr));
					if (var == NOVARIABLE)
                  {
						/* "Pad" not found on instance: look for it inside of prototype */
						var = getval((INTBIG)pi->proto, VPORTPROTO, -1, x_("ATTR_pin"));
						if (var == NOVARIABLE) continue;
                  }
					/* create a new net connection */
					en = (ECADNET *)emalloc(sizeof (ECADNET), io_tool->cluster);
					if (en == 0) return(backannotate);
					net = pi->conarcinst->network;
					if (net->temp1 != 0)
					{
						(void)allocstring(&en->netname, (CHAR *)net->temp1, io_tool->cluster);
					} else if (net->namecount > 0)
					{
						infstr = initinfstr();
						addstringtoinfstr(infstr, prefix);
						addstringtoinfstr(infstr, networkname(net, 0));
						(void)allocstring(&en->netname, returninfstr(infstr), io_tool->cluster);
					} else
					{
						infstr = initinfstr();
						formatinfstr(infstr, x_("%sNET%ld"), prefix, (INTBIG)net);
						(void)allocstring(&en->netname, returninfstr(infstr), io_tool->cluster);
					}
					infstr = initinfstr();
					addstringtoinfstr(infstr, prefix);
					addstringtoinfstr(infstr, name);
					(void)allocstring(&en->packagename, returninfstr(infstr), io_tool->cluster);
					(void)allocstring(&en->pinname, describevariable(var, -1, -1), io_tool->cluster);
					en->nextecadnet = io_ecadfirstnet;
					io_ecadfirstnet = en;
				}
				efree((CHAR *)name);
			}
		}
	}
	return(backannotate);
}

/*
 * Routine to mark the cells in the current library that should be included
 * along with "curcell".  The cells are marked by setting their "temp1" field
 * nonzero.  These cells include all that are part of a multipage schematic.
 */
void io_ecadmarkcells(NODEPROTO *curcell)
{
	REGISTER NODEPROTO *np;

	/* mark those cells that will be included in this dump */
	for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->temp1 = 0;
	if ((curcell->cellview->viewstate&MULTIPAGEVIEW) != 0 ||
		namesamen(curcell->cellview->viewname, x_("schematic-page-"), 15) == 0)
	{
		/* original cell is a multipage: allow all multipage in this cell */
		FOR_CELLGROUP(np, curcell)
			np->temp1 = 1;
	} else
	{
		np->temp1 = 1;
	}
}
