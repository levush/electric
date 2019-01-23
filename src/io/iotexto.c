/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iotexto.c
 * Input/output tool: textual format output
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

#include "global.h"
#include "eio.h"

static INTBIG io_cellnumber, io_nodeinsterror, io_portarcinsterror, io_portexpinsterror,
	  io_portprotoerror, io_arcinsterror, io_geomerror, io_rtnodeerror, io_libraryerror;

/* prototypes for local routines */
static void    io_textrecurse(NODEPROTO*);
static INTBIG  io_countvars(INTBIG, VARIABLE*);
static void    io_writevars(INTBIG, VARIABLE*, NODEPROTO*);
static CHAR   *io_makestring(VARIABLE*, NODEPROTO*);
static void    io_makestringvar(void*, INTBIG, INTBIG, NODEPROTO*);
static void    io_addstring(void*, CHAR*);
static void    io_printname(CHAR*, FILE*);

BOOLEAN io_writetextlibrary(LIBRARY *lib)
{
	REGISTER CHAR *name;
	CHAR file[256], *truename;
	REGISTER INTBIG i, j, t, noc, arcinst, poc, cell, cellgroup;
	REGISTER NODEPROTO *np, *onp;
	REGISTER PORTPROTO *pp;
	REGISTER LIBRARY *olib;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER TECHNOLOGY *tech;
	REGISTER VIEW *v;
	REGISTER NODEPROTO **cells;

	(void)estrcpy(file, lib->libfile);
	name = &file[estrlen(file)-5];
	if (estrcmp(name, x_(".elib")) == 0) *name = 0;
	name = &file[estrlen(file)-4];
	if (estrcmp(name, x_(".txt")) != 0) (void)estrcat(file, x_(".txt"));
	name = truepath(file);
	io_fileout = xcreate(name, io_filetypetlib, _("Readable Dump File"), &truename);
	if (io_fileout == NULL)
	{
		if (truename != 0) ttyputerr(_("Cannot write %s"), truename);
		return(TRUE);
	}

	/* clear error counters */
	io_nodeinsterror = io_portarcinsterror = io_portexpinsterror = 0;
	io_portprotoerror = io_arcinsterror = io_geomerror = io_libraryerror = 0;
	io_rtnodeerror = 0;

	/* determine proper library order */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp1 = -1;
	io_cellnumber = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->firstinst == NONODEINST) io_textrecurse(np);
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->temp1 < 0) io_textrecurse(np);
	if (io_cellnumber > 0)
	{
		cells = (NODEPROTO **)emalloc((io_cellnumber * sizeof(NODEPROTO *)),
			io_tool->cluster);
		if (cells == 0)
		{
			ttyputnomemory();
			return(TRUE);
		}
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 >= 0 && np->temp1 < io_cellnumber)
					cells[np->temp1] = np;
			}
		}
	}

	/* determine cell groupings */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		np->temp2 = 0;
	cellgroup = 0;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->temp2 != 0) continue;
		cellgroup++;
		FOR_CELLGROUP(onp, np)
			onp->temp2 = cellgroup;
	}

	/* write header information */
	xprintf(io_fileout, x_("****library: \"%s\"\n"), lib->libname);
	xprintf(io_fileout, x_("version: %s\n"), el_version);
	xprintf(io_fileout, x_("aids: %ld\n"), el_maxtools);
	for(i=0; i<el_maxtools; i++)
	{
		xprintf(io_fileout, x_("aidname: %s\n"), el_tools[i].toolname);
		if (io_countvars(el_tools[i].numvar, el_tools[i].firstvar) != 0)
			io_writevars(el_tools[i].numvar, el_tools[i].firstvar, NONODEPROTO);
	}
	xprintf(io_fileout, x_("userbits: %ld\n"), lib->userbits);
	xprintf(io_fileout, x_("techcount: %ld\n"), el_maxtech);
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		xprintf(io_fileout, x_("techname: %s lambda: %ld\n"), tech->techname, lib->lambda[tech->techindex]);
		io_writevars(tech->numvar, tech->firstvar, NONODEPROTO);
	}
	for(v = el_views; v != NOVIEW; v = v->nextview)
		xprintf(io_fileout, x_("view: %s{%s}\n"), v->viewname, v->sviewname);
	xprintf(io_fileout, x_("cellcount: %ld\n"), io_cellnumber);
	if (lib->curnodeproto != NONODEPROTO)
		xprintf(io_fileout, x_("maincell: %ld\n"), lib->curnodeproto->temp1);

	/* write variables on the library */
	io_writevars(lib->numvar, lib->firstvar, NONODEPROTO);

	/* write the rest of the database */
	for(cell = 0; cell < io_cellnumber; cell++)
	{
		/* write the nodeproto name */
		np = cells[cell];
		xprintf(io_fileout, x_("***cell: %ld/%ld\n"), np->temp1, np->temp2);
		xprintf(io_fileout, x_("name: %s"), np->protoname);
		if (*np->cellview->sviewname != 0)
			xprintf(io_fileout, x_("{%s}"), np->cellview->sviewname);
		xprintf(io_fileout, x_("\n"));
		xprintf(io_fileout, x_("version: %ld\n"), np->version);
		xprintf(io_fileout, x_("creationdate: %ld\n"), np->creationdate);
		xprintf(io_fileout, x_("revisiondate: %ld\n"), np->revisiondate);

		/* write the nodeproto bounding box */
		xprintf(io_fileout, x_("lowx: %ld highx: %ld lowy: %ld highy: %ld\n"),
			np->lowx, np->highx, np->lowy, np->highy);

		/* cells in external libraries mention the library and stop */
		if (np->lib != lib)
		{
			xprintf(io_fileout, x_("externallibrary: \"%s\"\n"),
				np->lib->libfile);
			continue;
		}

		/* write tool information */
		xprintf(io_fileout, x_("aadirty: %ld\n"), np->adirty);
		xprintf(io_fileout, x_("userbits: %ld\n"), np->userbits);

		/* count and number the nodes, arcs, and ports */
		noc = arcinst = poc = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = noc++;
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			ai->temp1 = arcinst++;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			pp->temp1 = poc++;
		xprintf(io_fileout, x_("nodes: %ld arcs: %ld porttypes: %ld\n"), noc, arcinst, poc);

		/* write variables on the cell */
		io_writevars(np->numvar, np->firstvar, np);

		/* write the nodes in this cell */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			xprintf(io_fileout, x_("**node: %ld\n"), ni->temp1);
			if (ni->proto->primindex == 0)
				xprintf(io_fileout, x_("type: [%ld]\n"), ni->proto->temp1); else
					xprintf(io_fileout, x_("type: %s:%s\n"),
						ni->proto->tech->techname, ni->proto->protoname);
			xprintf(io_fileout, x_("lowx: %ld highx: %ld lowy: %ld highy: %ld\n"), ni->lowx,
				ni->highx, ni->lowy, ni->highy);
			if (ni->transpose != 0) t = 1; else t = 0;
			xprintf(io_fileout, x_("rotation: %d transpose: %d\n"), ni->rotation, t);
			if (ni->proto->primindex == 0)
			{
				xprintf(io_fileout, x_("descript: %ld/%ld\n"), ni->textdescript[0],
					ni->textdescript[1]);
			}
			xprintf(io_fileout, x_("userbits: %ld\n"), ni->userbits);
			io_writevars(ni->numvar, ni->firstvar, np);

			pi = ni->firstportarcinst;   pe = ni->firstportexpinst;
			for(pp = ni->proto->firstportproto, i=0; pp != NOPORTPROTO; pp = pp->nextportproto, i++)
			{
				j = 0;
				while (pi != NOPORTARCINST && pi->proto == pp)
				{
					if (j == 0) xprintf(io_fileout, x_("*port: %s\n"), pp->protoname);
					j++;
					xprintf(io_fileout, x_("arc: %ld\n"), pi->conarcinst->temp1);
					io_writevars(pi->numvar, pi->firstvar, np);
					pi = pi->nextportarcinst;
				}
				while (pe != NOPORTEXPINST && pe->proto == pp)
				{
					if (j == 0) xprintf(io_fileout, x_("*port: %s\n"), pp->protoname);
					j++;
					xprintf(io_fileout, x_("exported: %ld\n"), pe->exportproto->temp1);
					io_writevars(pe->numvar, pe->firstvar, np);
					pe = pe->nextportexpinst;
				}
			}
		}

		/* write the portprotos in this cell */
		poc = 0;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			xprintf(io_fileout, x_("**porttype: %ld\n"), poc++);
			xprintf(io_fileout, x_("subnode: %ld\n"), pp->subnodeinst->temp1);
			xprintf(io_fileout, x_("subport: %s\n"), pp->subportproto->protoname);
			xprintf(io_fileout, x_("name: %s\n"), pp->protoname);

			/* need to write both words */
			xprintf(io_fileout, x_("descript: %ld/%ld\n"), pp->textdescript[0],
				pp->textdescript[1]);
			xprintf(io_fileout, x_("userbits: %ld\n"), pp->userbits);
			io_writevars(pp->numvar, pp->firstvar, np);
		}

		/* write the arcs in this cell */
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			xprintf(io_fileout, x_("**arc: %ld\n"), ai->temp1);
			xprintf(io_fileout, x_("type: %s:%s\n"), ai->proto->tech->techname, ai->proto->protoname);
			xprintf(io_fileout, x_("width: %ld length: %ld\n"), ai->width, ai->length);
			xprintf(io_fileout, x_("userbits: %ld\n"), ai->userbits);
			for(i=0; i<2; i++)
			{
				xprintf(io_fileout, x_("*end: %ld\n"), i);
				xprintf(io_fileout, x_("node: %ld\n"), ai->end[i].nodeinst->temp1);
				xprintf(io_fileout, x_("nodeport: %s\n"), ai->end[i].portarcinst->proto->protoname);
				xprintf(io_fileout, x_("xpos: %ld ypos: %ld\n"), ai->end[i].xpos, ai->end[i].ypos);
			}
			io_writevars(ai->numvar, ai->firstvar, np);
		}
		xprintf(io_fileout, x_("celldone: %s\n"), np->protoname);
	}

	/* print any variable-related error messages */
	if (io_nodeinsterror != 0)
		ttyputmsg(x_("Warning: %ld NODEINST pointers point outside cell: not saved"), io_nodeinsterror);
	if (io_arcinsterror != 0)
		ttyputmsg(x_("Warning: %ld ARCINST pointers point outside cell: not saved"), io_arcinsterror);
	if (io_portprotoerror != 0)
		ttyputmsg(x_("Warning: %ld PORTPROTO pointers point outside cell: not saved"), io_portprotoerror);
	if (io_portarcinsterror != 0)
		ttyputmsg(x_("Warning: %ld PORTARCINST pointers could not be saved"), io_portarcinsterror);
	if (io_portexpinsterror != 0)
		ttyputmsg(x_("Warning: %ld PORTEXPINST pointers could not be saved"), io_portexpinsterror);
	if (io_geomerror != 0)
		ttyputmsg(x_("Warning: %ld GEOM pointers could not be saved"), io_geomerror);
	if (io_rtnodeerror != 0)
		ttyputmsg(x_("Warning: %ld RTNODE pointers could not be saved"), io_rtnodeerror);
	if (io_libraryerror != 0)
		ttyputmsg(x_("Warning: LIBRARY pointers could not be saved"), io_libraryerror);

	/* clean up and return */
	xclose(io_fileout);
	ttyputmsg(_("%s written"), truename);
	lib->userbits &= ~(LIBCHANGEDMAJOR | LIBCHANGEDMINOR);
	if (io_cellnumber > 0) efree((CHAR *)cells);
	return(FALSE);
}

/*
 * routine to help order the library for proper nonforward references
 * in the outout
 */
void io_textrecurse(NODEPROTO *np)
{
	REGISTER NODEINST *ni;

	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto->primindex != 0) continue;
		if (ni->proto->temp1 == -1) io_textrecurse(ni->proto);
	}

	/* add this cell to the list */
	np->temp1 = io_cellnumber++;
}

/*
 * routine to return the number of permanent variables on an object
 */
INTBIG io_countvars(INTBIG numvar, VARIABLE *firstvar)
{
	REGISTER INTBIG i, j;

	i = 0;
	for(j=0; j<numvar; j++)
		if ((firstvar[j].type & VDONTSAVE) == 0) i++;
	return(i);
}

/*
 * routine to write the variables on an object.  The current cell is
 * "curnodeproto" such that any references to objects in a cell must be in
 * this cell.
 */
void io_writevars(INTBIG numvar, VARIABLE *firstvar, NODEPROTO *curnodeproto)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG i;
	REGISTER VARIABLE *var;

	i = io_countvars(numvar, firstvar);
	if (i == 0) return;

	xprintf(io_fileout, x_("variables: %ld\n"), i);
	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		if ((var->type & VDONTSAVE) != 0) continue;
		pt = io_makestring(var, curnodeproto);
		if (pt == 0) pt = x_("");
		io_printname((CHAR *)var->key, io_fileout);
		if ((var->type&(VLENGTH|VISARRAY)) != VISARRAY)
		{
			xprintf(io_fileout, x_("[0%o,0%o/0%o]: "), var->type, var->textdescript[0],
				var->textdescript[1]);
		} else
		{
			xprintf(io_fileout, x_("(%ld)[0%o,0%o/0%o]: "), getlength(var), var->type,
				var->textdescript[0], var->textdescript[1]);
		}
		xprintf(io_fileout, x_("%s\n"), pt);
	}
}

/*
 * routine to convert variable "var" to a string for printing in the text file.
 * returns zero on error
 */
CHAR *io_makestring(VARIABLE *var, NODEPROTO *curnodeproto)
{
	REGISTER INTBIG i, len;
	CHAR line[50];
	REGISTER CHAR *pt;
	REGISTER void *infstr;

	if (var == NOVARIABLE) return(0);

	infstr = initinfstr();
	if ((var->type&VISARRAY) != 0)
	{
		len = getlength(var);
		for(i=0; i<len; i++)
		{
			if (i == 0) addtoinfstr(infstr, '['); else
				addtoinfstr(infstr, ',');

			if ((var->type&VTYPE) == VGENERAL)
			{
				if ((i&1) == 0)
				{
					(void)esnprintf(line, 50, x_("0%lo"), ((INTBIG *)var->addr)[i+1]);
					addstringtoinfstr(infstr, line);
				} else
				{
					io_makestringvar(infstr, ((INTBIG *)var->addr)[i], ((INTBIG *)var->addr)[i-1],
						curnodeproto);
				}
			} else
			{
				switch ((var->type&VTYPE))
				{
					case VCHAR:
						io_makestringvar(infstr, var->type, ((INTBIG)((CHAR *)var->addr)[i]),
							curnodeproto);
						break;

					case VSHORT:
						io_makestringvar(infstr, var->type, ((INTBIG)((INTSML *)var->addr)[i]),
							curnodeproto);
						break;

					case VDOUBLE:
						io_makestringvar(infstr, var->type, ((INTBIG)((double *)var->addr)[i]),
							curnodeproto);
						break;

					default:
						io_makestringvar(infstr, var->type, ((INTBIG *)var->addr)[i], curnodeproto);
						break;
				}
			}
		}
		addtoinfstr(infstr, ']');
	} else io_makestringvar(infstr, var->type, var->addr, curnodeproto);
	pt = returninfstr(infstr);
	return(pt);
}

/*
 * routine to make a string from the value in "addr" which has a type in
 * "type".
 */
void io_makestringvar(void *infstr, INTBIG type, INTBIG addr, NODEPROTO *curnodeproto)
{
	CHAR line[100];
	REGISTER INTBIG cindex;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER GEOM *geom;
	REGISTER RTNODE *rtn;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER TOOL *tool;

	if ((type&(VCODE1|VCODE2)) != 0) type = VSTRING;
	switch (type&VTYPE)
	{
		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
		case VFRACT:
			(void)esnprintf(line, 100, x_("%ld"), addr);
			addstringtoinfstr(infstr, line);
			break;
		case VADDRESS:
			(void)esnprintf(line, 100, x_("0%lo"), addr);
			addstringtoinfstr(infstr, line);
			break;
		case VCHAR:
			addtoinfstr(infstr, (CHAR)addr);
			break;
		case VSTRING:
			addtoinfstr(infstr, '"');
			io_addstring(infstr, (CHAR *)addr);
			addtoinfstr(infstr, '"');
			break;
		case VFLOAT:
			(void)esnprintf(line, 100, x_("%f"), castfloat(addr));
			addstringtoinfstr(infstr, line);
			break;
		case VDOUBLE:
			(void)esnprintf(line, 100, x_("%f"), castfloat(addr));
			addstringtoinfstr(infstr, line);
			break;
		case VNODEINST:
			ni = (NODEINST *)addr;
			cindex = -1;
			if (ni != NONODEINST)
			{
				if (ni->parent == curnodeproto) cindex = ni->temp1; else
					io_nodeinsterror++;
			}
			(void)esnprintf(line, 100, x_("%ld"), cindex);
			addstringtoinfstr(infstr, line);
			break;
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (np == NONODEPROTO)
			{
				addstringtoinfstr(infstr, x_("-1"));
				break;
			}
			if (np->primindex == 0)
			{
				(void)esnprintf(line, 100, x_("%ld"), np->temp1);
				addstringtoinfstr(infstr, line);
			} else
			{
				io_addstring(infstr, np->tech->techname);
				addtoinfstr(infstr, ':');
				io_addstring(infstr, np->protoname);
			}
			break;
		case VPORTARCINST:
			io_portarcinsterror++;
			pi = (PORTARCINST *)addr;
			if (pi == NOPORTARCINST)
			{
				addstringtoinfstr(infstr, x_("NOPORTARCINST"));
				break;
			}
			(void)esnprintf(line, 100, x_("portarc%ld"), (INTBIG)pi);
			addstringtoinfstr(infstr, line);
			break;
		case VPORTEXPINST:
			io_portexpinsterror++;
			pe = (PORTEXPINST *)addr;
			if (pe == NOPORTEXPINST)
			{
				addstringtoinfstr(infstr, x_("NOPORTEXPINST"));
				break;
			}
			(void)esnprintf(line, 100, x_("portexp%ld"), (INTBIG)pe);
			addstringtoinfstr(infstr, line);
			break;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			cindex = -1;
			if (pp != NOPORTPROTO)
			{
				if (pp->parent == curnodeproto) cindex = pp->temp1; else
					io_portprotoerror++;
			}
			(void)esnprintf(line, 100, x_("%ld"), cindex);
			addstringtoinfstr(infstr, line);
			break;
		case VARCINST:
			ai = (ARCINST *)addr;
			cindex = -1;
			if (ai != NOARCINST)
			{
				if (ai->parent == curnodeproto) cindex = ai->temp1; else
					io_arcinsterror++;
			}
			(void)esnprintf(line, 100, x_("%ld"), cindex);
			addstringtoinfstr(infstr, line);
			break;
		case VARCPROTO:
			ap = (ARCPROTO *)addr;
			if (ap == NOARCPROTO)
			{
				addstringtoinfstr(infstr, x_("NOARCPROTO"));
				break;
			}
			io_addstring(infstr, ap->tech->techname);
			addtoinfstr(infstr, ':');
			io_addstring(infstr, ap->protoname);
			break;
		case VGEOM:
			io_geomerror++;
			geom = (GEOM *)addr;
			if (geom == NOGEOM)
			{
				addstringtoinfstr(infstr, x_("NOGEOM"));
				break;
			}
			(void)esnprintf(line, 100, x_("geom%ld"), (INTBIG)geom);
			addstringtoinfstr(infstr, line);
			break;
		case VLIBRARY:
			io_libraryerror++;
			lib = (LIBRARY *)addr;
			if (lib == NOLIBRARY)
			{
				addstringtoinfstr(infstr, x_("NOLIBRARY"));
				break;
			}
			addtoinfstr(infstr, '"');
			io_addstring(infstr, lib->libname);
			addtoinfstr(infstr, '"');
			break;
		case VTECHNOLOGY:
			tech = (TECHNOLOGY *)addr;
			if (tech == NOTECHNOLOGY)
			{
				addstringtoinfstr(infstr, x_("NOTECHNOLOGY"));
				break;
			}
			io_addstring(infstr, tech->techname);
			break;
		case VTOOL:
			tool = (TOOL *)addr;
			if (tool == NOTOOL)
			{
				addstringtoinfstr(infstr, x_("NOTOOL"));
				break;
			}
			io_addstring(infstr, tool->toolname);
			break;
		case VRTNODE:
			io_rtnodeerror++;
			rtn = (RTNODE *)addr;
			if (rtn == NORTNODE)
			{
				addstringtoinfstr(infstr, x_("NORTNODE"));
				break;
			}
			(void)esnprintf(line, 100, x_("rtn%ld"), (INTBIG)rtn);
			addstringtoinfstr(infstr, line);
			break;
		case VNETWORK:
		case VVIEW:
		case VWINDOWPART:
		case VGRAPHICS:
		case VCONSTRAINT:
		case VGENERAL:
		case VWINDOWFRAME:
		case VPOLYGON:
			break;
	}
}

/*
 * routine to add the string "str" to the infinite string and to quote the
 * special characters '[', ']', '"', and '^'.
 */
void io_addstring(void *infstr, CHAR *str)
{

	while (*str != 0)
	{
		if (*str == '[' || *str == ']' || *str == '"' || *str == '^')
			addtoinfstr(infstr, '^');
		addtoinfstr(infstr, *str++);
	}
}

/*
 * routine to print the variable name in "name" on file "file".  The
 * conversion performed is to quote with a backslash any of the characters
 * '(', '[', or '^'.
 */
void io_printname(CHAR *name, FILE *file)
{
	REGISTER CHAR *pt;

	for(pt = name; *pt != 0; pt++)
	{
		if (*pt == '^' || *pt == '[' || *pt == '(') xputc('^', file);
		xputc(*pt, file);
	}
}
