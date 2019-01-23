/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iolout.c
 * Input/output tool: L output
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

#include "config.h"
#include "global.h"
#include "eio.h"
#include "efunction.h"

/* node types returned by "io_l_nodetype" */
#define TRUEPIN    1
#define TRANSISTOR 2
#define INSTANCE   3
#define OTHERNODE  4

/* table that associates arc functionality with port types */
static struct
{
	INTBIG arcfunct;
	CHAR *portname;
} io_lports[] =
{
	{APMETAL1, x_("MET1")},
	{APMETAL2, x_("MET2")},
	{APPOLY1,  x_("POLY")},
	{APDIFFP,  x_("PDIFF")},
	{APDIFFN,  x_("NDIFF")},
	{APDIFFS,  x_("NWELL")},
	{APDIFFW,  x_("PWELL")},
	{0, NULL}  /* 0 */
};

/* table that associates arc functionality with contact types */
static struct
{
	INTBIG arcfunct[3];
	CHAR *contactname;
} io_lcontacts[] =
{
	{{APMETAL1, APDIFFP,  0}, x_("MPDIFF")},
	{{APMETAL1, APDIFFN,  0}, x_("MNDIFF")},
	{{APMETAL1, APPOLY1,  0}, x_("MPOLY")},
	{{APMETAL1, APMETAL2, 0}, x_("M1M2")},
	{{0,0,0}, NULL}  /* 0 */
};

/* prototypes for local routines */
static INTBIG io_l_writecell(NODEPROTO*);
static CHAR *io_l_name(NODEINST*);
static CHAR *io_l_fixname(CHAR*);
static INTBIG io_l_nodetype(NODEINST*);

BOOLEAN io_writellibrary(LIBRARY *lib)
{
	CHAR file[100], *truename;
	REGISTER CHAR *name;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *olib;

	/* create the proper disk file for the L */
	if (lib->curnodeproto == NONODEPROTO)
	{
		ttyputerr(_("Must be editing a cell to generate L output"));
		return(TRUE);
	}
	(void)estrcpy(file, lib->curnodeproto->protoname);
	(void)estrcat(file, x_(".L"));
	name = truepath(file);
	io_fileout = xcreate(name, io_filetypel, _("L File"), &truename);
	if (io_fileout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return(TRUE);
	}

	xprintf(io_fileout, x_("L:: TECH ANY\n"));

	/* write the L */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = 0;
	if (io_l_writecell(lib->curnodeproto) != 0)
		ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	/* clean up */
	xclose(io_fileout);

	/* tell the user that the file is written */
	ttyputmsg(_("%s written"), truename);

	return(FALSE);
}

/*
 * Routine to write "L" for cell "np".  Returns nonzero if back-annotation was added.
 */
INTBIG io_l_writecell(NODEPROTO *np)
{
	REGISTER NODEINST *subno, *oni;
	REGISTER ARCINST *subar, *oar;
	REGISTER NODEPROTO *subnt, *onp;
	REGISTER PORTPROTO *pp;
	PORTPROTO *gateleft, *gateright, *activetop, *activebottom;
	REGISTER PORTARCINST *pi, *opi;
	REGISTER INTBIG i, j, k, l, r, thisend, thatend, tot, nature, enature, ot, segcount, backannotate;
	REGISTER INTBIG segdist;
	REGISTER CHAR *type, *lay, *dir, *lastdir;
	CHAR line[50];
	INTBIG xpos, ypos, len, wid, lx, hx, ly, hy;
	REGISTER ARCPROTO **aplist;

	/* if there are any sub-cells that have not been written, write them */
	backannotate = 0;
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
	{
		subnt = subno->proto;
		if (subnt->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnt, np)) continue;

		/* convert body cells to contents cells */
		onp = contentsview(subnt);
		if (onp != NONODEPROTO) subnt = onp;

		/* don't recurse if this cell has already been written */
		if (subnt->temp1 != 0) continue;

		/* recurse to the bottom */
		if (io_l_writecell(subnt) != 0) backannotate = 1;
	}
	np->temp1 = 1;

	/* make sure that all nodes have names on them */
	if (asktool(net_tool, x_("name-nodes"), (INTBIG)np) != 0) backannotate++;

	/* write the cell header */
	xprintf(io_fileout, x_("\n"));
	if (np->cellview == el_layoutview) xprintf(io_fileout, x_("LAYOUT "));
	if (np->cellview == el_schematicview) xprintf(io_fileout, x_("SCHEMATIC "));
	if (np->cellview == el_iconview) xprintf(io_fileout, x_("ICON "));
	if (np->cellview == el_skeletonview) xprintf(io_fileout, x_("BBOX "));
	xprintf(io_fileout, x_("CELL %s ( )\n{\n"), io_l_fixname(np->protoname));

	/* write the bounding box */
	xprintf(io_fileout, x_("#bbox: ll= (%s,%s) ur= (%s,%s)\n"),
		latoa(np->lowx, 0), latoa(np->lowy, 0), latoa(np->highx, 0), latoa(np->highy, 0));

	/* write the ports */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		portposition(pp->subnodeinst, pp->subportproto, &xpos, &ypos);
		switch (pp->userbits&STATEBITS)
		{
			case GNDPORT:   type = x_("GND");    break;
			case PWRPORT:   type = x_("VDD");    break;
			case INPORT:    type = x_("IN");     break;
			case OUTPORT:   type = x_("OUT");    break;
			case BIDIRPORT: type = x_("INOUT");  break;
			default:        type = x_("");       break;
		}
		lay = pp->connects[0]->protoname;
		for(i=0; io_lports[i].arcfunct != 0; i++)
			if ((INTBIG)(((pp->connects[0]->userbits&AFUNCTION)>>AFUNCTIONSH)) == io_lports[i].arcfunct)
		{
			lay = io_lports[i].portname;   break;
		}
		xprintf(io_fileout, x_("\t%s %s %s (%s,%s) ;\n"), type, lay,
			io_l_fixname(pp->protoname), latoa(xpos, 0), latoa(ypos, 0));
	}
	xprintf(io_fileout, x_("\n"));

	/* number all components in the cell */
	i = 1;
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
		subno->temp1 = i++;

	/* write the components */
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
	{
		if (io_l_nodetype(subno) == TRUEPIN) continue;
		subnt = subno->proto;

		/* determine type of component */
		type = subnt->protoname;
		if (subnt->primindex == 0)
		{
			/* ignore recursive references (showing icon in contents) */
			if (isiconof(subnt, np)) continue;

			/* convert body cells to contents cells */
			onp = contentsview(subnt);
			if (onp != NONODEPROTO) subnt = onp;
			type = subnt->protoname;
			xprintf(io_fileout, x_("\tINST %s %s"), type, io_l_name(subno));
		} else
		{
			i = nodefunction(subno);
			if (i == NPPIN)
			{
				/* if pin is an export, don't write separate node statement */
				if (subno->firstportexpinst != NOPORTEXPINST) continue;
				for(j=0; io_lports[j].arcfunct != 0; j++)
					if ((INTBIG)(((subno->proto->firstportproto->connects[0]->userbits&
						AFUNCTION)>>AFUNCTIONSH)) == io_lports[j].arcfunct) break;
				if (io_lports[j].arcfunct != 0) type = io_lports[j].portname; else
					type = x_("???");
				(void)esnprintf(line, 50, x_("NODE %s"), type);
				type = line;
			}

			/* special type names for well/substrate contacts */
			if (i == NPWELL) type = x_("MNSUB");
			if (i == NPSUBSTRATE) type = x_("MPSUB");

			/* special type names for contacts */
			if (i == NPCONTACT || i == NPCONNECT)
			{
				aplist = subnt->firstportproto->connects;
				for(k=0; io_lcontacts[k].arcfunct[0] != 0; k++)
				{
					for(j=0; j<3; j++)
					{
						if (io_lcontacts[k].arcfunct[j] == 0) continue;
						for(l=0; aplist[l] != NOARCPROTO; l++)
							if ((INTBIG)(((aplist[l]->userbits&AFUNCTION) >> AFUNCTIONSH)) ==
								io_lcontacts[k].arcfunct[j]) break;
						if (aplist[l] == NOARCPROTO) break;
					}
					if (j < 3) continue;
					type = io_lcontacts[k].contactname;
					break;
				}
			}

			/* special type names for transistors */
			if (i == NPTRANMOS) type = x_("TN");
			if (i == NPTRADMOS) type = x_("TD");
			if (i == NPTRAPMOS) type = x_("TP");

			/* write the type and name */
			xprintf(io_fileout, x_("\t%s %s"), type, io_l_name(subno));
		}

		/* write rotation */
		if (subno->rotation != 0 || subno->transpose != 0)
		{
			r = subno->rotation;
			if (subno->transpose != 0)
			{
				xprintf(io_fileout, x_(" RX"));
				r = (r+2700) % 3600;
			}
			xprintf(io_fileout, x_(" R%s"), frtoa(r*WHOLE/10));
		}

		/* write size if nonstandard */
		if (subno->highx-subno->lowx != subnt->highx-subnt->lowx ||
			subno->highy-subno->lowy != subnt->highy-subnt->lowy)
		{
			if (i == NPTRANMOS || i == NPTRADMOS || i == NPTRAPMOS)
				transistorsize(subno, &len, &wid); else
			{
				nodesizeoffset(subno, &lx, &ly, &hx, &hy);
				len = subno->highy-hy - (subno->lowy+ly);
				wid = subno->highx-hx - (subno->lowx+lx);
			}
			xprintf(io_fileout, x_(" W=%s L=%s"), latoa(wid, 0), latoa(len, 0));
		}

		/* write location */
		if (subnt->primindex != 0)
		{
			xprintf(io_fileout, x_(" AT (%s,%s) ;\n"),
				latoa((subno->lowx+subno->highx)/2, 0), latoa((subno->lowy+subno->highy)/2, 0));
		} else
		{
			xprintf(io_fileout, x_(" AT (%s,%s) ;\n"),
				latoa((subno->lowx+subno->highx-subnt->lowx-subnt->highx)/2, 0),
					latoa((subno->lowy+subno->highy-subnt->lowy-subnt->highy)/2, 0));
		}
	}
	xprintf(io_fileout, x_("\n"));

	/* write all arcs connected to nodes */
	for(subar = np->firstarcinst; subar != NOARCINST; subar = subar->nextarcinst)
		subar->temp1 = 0;
	for(subno = np->firstnodeinst; subno != NONODEINST; subno = subno->nextnodeinst)
	{
		nature = io_l_nodetype(subno);
		if (nature == TRUEPIN) continue;
		for(pi = subno->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			subar = pi->conarcinst;
			if (subar->temp1 != 0) continue;
			xprintf(io_fileout, x_("\tWIRE"));
			for(i=0; io_lports[i].arcfunct != 0; i++)
				if ((INTBIG)(((subar->proto->userbits&AFUNCTION)>>AFUNCTIONSH)) == io_lports[i].arcfunct)
			{
				xprintf(io_fileout, x_(" %s"), io_lports[i].portname);
				break;
			}

			/* write the wire width if nonstandard */
			if (subar->width != defaultarcwidth(subar->proto))
				xprintf(io_fileout, x_(" W=%s"), latoa(subar->width-arcwidthoffset(subar), 0));

			/* write the starting node name (use port name if pin is an export) */
			if (subno->firstportexpinst != NOPORTEXPINST && nodefunction(subno) == NPPIN)
				xprintf(io_fileout, x_(" %s"),
						io_l_fixname(subno->firstportexpinst->exportproto->protoname)); else
							xprintf(io_fileout, x_(" %s"), io_l_name(subno));

			/* qualify node name with port name if a transistor or instance */
			if (nature == TRANSISTOR)
			{
				transistorports(subno, &gateleft, &gateright, &activetop, &activebottom);
				if (pi->proto == gateleft) xprintf(io_fileout, x_(".gl"));
				if (pi->proto == activetop) xprintf(io_fileout, x_(".d"));
				if (pi->proto == gateright) xprintf(io_fileout, x_(".gr"));
				if (pi->proto == activebottom) xprintf(io_fileout, x_(".s"));
			} else if (nature == INSTANCE)
				xprintf(io_fileout, x_(".%s"), io_l_fixname(pi->proto->protoname));

			/* prepare to run along the wire to a terminating node */
			if (subar->end[0].portarcinst == pi) thatend = 1; else thatend = 0;
			lastdir = x_("");
			segdist = -1;
			segcount = 0;
			for(;;)
			{
				/* get information about this segment (arc "subar") */
				subar->temp1 = 1;
				thisend = 1 - thatend;
				dir = lastdir;
				if (subar->end[thatend].xpos == subar->end[thisend].xpos)
				{
					if (subar->end[thatend].ypos > subar->end[thisend].ypos) dir = x_("UP"); else
						if (subar->end[thatend].ypos < subar->end[thisend].ypos) dir = x_("DOWN");
				} else if (subar->end[thatend].ypos == subar->end[thisend].ypos)
				{
					if (subar->end[thatend].xpos > subar->end[thisend].xpos) dir = x_("RIGHT"); else
						if (subar->end[thatend].xpos < subar->end[thisend].xpos) dir = x_("LEFT");
				}

				/* if segment is different from last, write out last one */
				if (estrcmp(dir, lastdir) != 0 && *lastdir != 0)
				{
					xprintf(io_fileout, x_(" %s"), lastdir);
					if (segdist >= 0) xprintf(io_fileout, x_("=%s"), latoa(segdist, 0));
					segdist = -1;
					segcount++;
				}

				/* remember this segment's direction and length */
				lastdir = dir;
				oni = subar->end[thatend].nodeinst;
				enature = io_l_nodetype(oni);
				if ((nature != TRANSISTOR || segcount > 0) && enature != TRANSISTOR)
				{
					if (segdist < 0) segdist = 0;
					segdist += subar->length;
				}

				/* if other node not a pin, stop now */
				if (enature != TRUEPIN) break;

				/* end the loop if more than 1 wire out of next node "oni" */
				tot = 0;
				for(opi = oni->firstportarcinst; opi != NOPORTARCINST; opi = opi->nextportarcinst)
				{
					if (opi->conarcinst->temp1 != 0) continue;
					tot++;
					oar = opi->conarcinst;
					if (oar->end[0].portarcinst == opi) ot = 1; else ot = 0;
				}
				if (tot != 1) break;
				subar = oar;
				thatend = ot;
			}
			if (*lastdir != 0)
			{
				xprintf(io_fileout, x_(" %s"), lastdir);
				if (segdist >= 0) xprintf(io_fileout, x_("=%s"), latoa(segdist, 0));
			} else xprintf(io_fileout, x_(" TO"));

			/* write the terminating node name (use port name if pin is an export) */
			if (oni->firstportexpinst != NOPORTEXPINST && nodefunction(oni) == NPPIN)
				xprintf(io_fileout, x_(" %s"),
					io_l_fixname(oni->firstportexpinst->exportproto->protoname)); else
						xprintf(io_fileout, x_(" %s"), io_l_name(oni));

			/* qualify node name with port name if a transistor or an instance */
			opi = subar->end[thatend].portarcinst;
			if (enature == TRANSISTOR)
			{
				transistorports(oni, &gateleft, &gateright, &activetop, &activebottom);
				if (opi->proto == gateleft) xprintf(io_fileout, x_(".gl"));
				if (opi->proto == activetop) xprintf(io_fileout, x_(".d"));
				if (opi->proto == gateright) xprintf(io_fileout, x_(".gr"));
				if (opi->proto == activebottom) xprintf(io_fileout, x_(".s"));
			} else if (enature == INSTANCE)
				xprintf(io_fileout, x_(".%s"), io_l_fixname(opi->proto->protoname));
			xprintf(io_fileout, x_(" ;\n"));
		}
	}

	/* write any unmentioned wires (shouldn't be any) */
	for(subar = np->firstarcinst; subar != NOARCINST; subar = subar->nextarcinst)
	{
		if (subar->temp1 != 0) continue;
		xprintf(io_fileout, x_("# WIRE %s not described!!\n"), describearcinst(subar));
	}
	xprintf(io_fileout, x_("}\n"));
	return(backannotate);
}

CHAR *io_l_name(NODEINST *ni)
{
	REGISTER VARIABLE *var;
	static CHAR line[30];

	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
	if (var != NOVARIABLE) return(io_l_fixname((CHAR *)var->addr));
	ttyputerr(_("L generation warning: no name on node %s"), describenodeinst(ni));
	(void)esnprintf(line, 30, x_("c%ld"), ni->temp1);
	return(line);
}

CHAR *io_l_fixname(CHAR *name)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG i;
	static CHAR *badname[] = {x_("VDD"), x_("GND"), 0};
	REGISTER void *infstr;

	/* check for reserved names */
	for(i=0; badname[i] != 0; i++)
		if (estrcmp(name, badname[i]) == 0)
	{
		/* this is a reserved name: change it */
		infstr = initinfstr();
		addstringtoinfstr(infstr, name);
		addstringtoinfstr(infstr, x_("XXX"));
		return(returninfstr(infstr));
	}

	/* check for special characters */
	for(pt = name; *pt != 0; pt++) if (isalnum(*pt) == 0) break;
	if (*pt != 0)
	{
		/* name has special characters: remove them */
		infstr = initinfstr();
		for(pt = name; *pt != 0; pt++)
			if (isalnum(*pt) != 0) addtoinfstr(infstr, *pt);
		return(returninfstr(infstr));
	}

	/* name is fine: use it as is */
	return(name);
}

/*
 * Routine to determine the type of node "ni".  Returns:
 *    TRUEPIN    if a true pin (exactly two connections)
 *    TRANSISTOR if a transistor
 *    INSTANCE   if a cell instance
 *    OTHERNODE  otherwise.
 */
INTBIG io_l_nodetype(NODEINST *ni)
{
	REGISTER INTBIG i;
	REGISTER PORTARCINST *pi;

	if (ni->proto->primindex == 0) return(INSTANCE);
	i = nodefunction(ni);
	if (i == NPTRANMOS || i == NPTRADMOS || i == NPTRAPMOS) return(TRANSISTOR);
	if (i != NPPIN) return(OTHERNODE);
	i = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		i++;
	if (i != 2) return(OTHERNODE);
	return(TRUEPIN);
}
