/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iobinaryo.c
 * Input/output tool: binary format output
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
#include "usr.h"
#include "edialogs.h"

#define FILECOPYBUFSIZE 4096

static BOOLEAN io_outputstage;		/* progress of database (aborting) */
static INTSML *io_namesave;			/* for saving the global namespace */
static CHAR   *io_outfilename = 0;	/* the name of the file being written */

/* prototypes for local routines */
static void io_cleanup(void);
static BOOLEAN io_copyfile(CHAR *src, CHAR *dst, BOOLEAN);
static void io_writenodeproto(NODEPROTO*, BOOLEAN);
static void io_writenodeinst(NODEINST*);
static void io_writearcinst(ARCINST*);
static void io_writenamespace(void);
static void io_restorenamespace(void);
static void io_writevariables(VARIABLE*, INTSML, INTBIG);
static void io_findxlibvariables(VARIABLE*, INTSML);
static void io_putoutvar(INTBIG*, INTBIG);
static void io_putout(void*, INTBIG);
static void io_putoutbig(void*);
static void io_putstring(CHAR*);

/*
 * Routine to free all memory associated with this module.
 */
void io_freebinoutmemory(void)
{
	if (io_outfilename != 0) efree(io_outfilename);
}

BOOLEAN io_writebinlibrary(LIBRARY *lib, BOOLEAN nobackup)
{
	INTBIG i, a, n, p, magic, aacount, techcount, nodepprotoindex,
		portprotoindex, portpprotoindex, arcprotoindex, nodeprotoindex, nodeindex,
		arcindex, geomindex, curnodeproto, cellindex,
		*curstate, cellshere, filestatus;
	BOOLEAN err, createit;
	time_t olddate;
	INTBIG year, month, mday, hour, minute, second;
	REGISTER NODEPROTO *np;
	REGISTER ARCPROTO *ap;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *olib;
	REGISTER TECHNOLOGY *tech;
	REGISTER CHAR *rename, *istrue, *dotaddr, *msg, *ext;
	CHAR *truename, *name, *arg[3], datesuffix[30];
	UCHAR1 wordsize;
	REGISTER VIEW *v;
	REGISTER void *infstr;

	/* get the name of the file to write */
	io_outputstage = FALSE;
	dotaddr = 0;
	ext = &lib->libfile[estrlen(lib->libfile)-5];
	if (estrcmp(ext, x_(".elib")) == 0)
	{
		dotaddr = ext;
		*ext = 0;
	} else
	{
		ext = &lib->libfile[estrlen(lib->libfile)-4];
		if (estrcmp(ext, x_(".txt")) == 0)
		{
			dotaddr = ext;
			*ext = 0;
		}
	}
	istrue = truepath(lib->libfile);
	infstr = initinfstr();
	addstringtoinfstr(infstr, istrue);
	addstringtoinfstr(infstr, x_(".elib"));
	err = FALSE;
	if (io_outfilename == 0) 
	{
		err |= allocstring(&io_outfilename, returninfstr(infstr), io_tool->cluster);
	} else
	{
		err |= reallocstring(&io_outfilename, returninfstr(infstr), io_tool->cluster);
	}
	if (dotaddr != 0) *dotaddr = '.';
	if (err)
	{
		DiaMessageInDialog(_("No memory! saving to 'PANIC'"));
		name = _("PANIC");
	} else
	{
		name = io_outfilename;
	}

	curstate = io_getstatebits();
	if ((curstate[0]&CHECKATWRITE) != 0)
	{
		ttyputmsg(_("Checking all libraries..."));
		us_checkdatabase(FALSE);
	}
	if (nobackup) curstate[0] &= ~(BINOUTBACKUP|CHECKATWRITE);

	rename = 0;
	createit = TRUE;
	filestatus = fileexistence(name);
	if ((lib->userbits&READFROMDISK) != 0)
	{
		/* library came from disk: write it back there unless protected */
		if (filestatus == 3) lib->userbits &= ~READFROMDISK;
	}
	if ((lib->userbits&READFROMDISK) == 0) msg = _("Library File"); else
	{
		msg = 0;

		/* if the file already exists, rename it */
		if (filestatus == 1)
			rename = (CHAR *)emalloc((estrlen(name)+15) * SIZEOFCHAR, el_tempcluster);
		if (rename != 0)
		{
			switch (curstate[0]&BINOUTBACKUP)
			{
				case BINOUTNOBACK:
					(void)estrcpy(rename, name);
					(void)estrcat(rename, x_(".XXXXXX"));
					if (io_copyfile(name, rename, TRUE))
					{
						efree(rename);
						rename = 0;
					}
					createit = FALSE;
					break;
				case BINOUTONEBACK:
					(void)estrcpy(rename, name);
					(void)estrcat(rename, x_("~"));
					if (fileexistence(rename) == 1) (void)eunlink(rename);
					(void)io_copyfile(name, rename, FALSE);
					createit = FALSE;
					efree(rename);
					rename = 0;
					break;
				case BINOUTFULLBACK:
					olddate = filedate(name);
					parsetime(olddate, &year, &month, &mday, &hour, &minute, &second);
					for(i=0; i<1000; i++)
					{
						if (i == 0)
						{
							esnprintf(datesuffix, 30, x_("-%ld-%ld-%ld"),
								year, month+1, mday);
						} else
						{
							esnprintf(datesuffix, 30, x_("-%ld-%ld-%ld--%ld"),
								year, month+1, mday, i);
						}
						(void)estrcpy(rename, name);
						rename[estrlen(rename)-5] = 0;
						(void)estrcat(rename, datesuffix);
						(void)estrcat(rename, x_(".elib"));
						if (fileexistence(rename) == 0) break;
					}
					(void)io_copyfile(name, rename, FALSE);
					createit = FALSE;
					efree(rename);
					rename = 0;
					break;
			}
		}
	}

	/* create the new library file */
	if (createit)
	{
		io_fileout = xcreate(name, io_filetypeblib, msg, &truename);
	} else
	{
		/* remove the ".elib" because "xopen()" adds it */
		i = estrlen(name);
		name[i-5] = 0;
		io_fileout = xopen(name, io_filetypeblib|FILETYPEWRITE, 0, &truename);
		if (io_outfilename == 0) 
			(void)allocstring(&io_outfilename, truename, io_tool->cluster); else
				(void)reallocstring(&io_outfilename, truename, io_tool->cluster);
		name = io_outfilename;
	}
	if (io_fileout == NULL)
	{
		if (truename != 0) DiaMessageInDialog(_("Cannot write %s"), name);
		if (rename != 0) efree(rename);
		return(TRUE);
	}

	/* if the user got a dialog, replace the library and file name */
	if (msg != 0)
	{
		arg[0] = lib->libname;
		arg[1] = truename;
		arg[2] = x_("library");
		us_rename(3, arg);
		if (io_outfilename == 0) 
			(void)allocstring(&io_outfilename, truename, io_tool->cluster); else
				(void)reallocstring(&io_outfilename, truename, io_tool->cluster);
		name = io_outfilename;
	}

	if (setjmp(io_filerror) != 0)
	{
		DiaMessageInDialog(_("Error writing %s"), name);
		xclose(io_fileout);
		io_cleanup();
		if (rename != 0)
		{
			ttyputmsg(_("Old binary file saved in %s"), rename);
			efree(rename);
		}
		return(TRUE);
	}

	/* first flush all changes so that database is clean */
	noundoallowed();

	/* initialize the file (writing version 13) */
	magic = MAGIC13;
	io_putout(&magic, 4);
	wordsize = SIZEOFINTSML;   io_putout(&wordsize, 1);
	wordsize = SIZEOFINTBIG;   io_putout(&wordsize, 1);
	wordsize = SIZEOFCHAR;     io_putout(&wordsize, 1);
	aacount = el_maxtools;
	io_putoutbig(&aacount);
	techcount = el_maxtech;
	io_putoutbig(&techcount);

	/* initialize the number of objects in the database */
	nodeindex = portprotoindex = nodeprotoindex = arcindex = nodepprotoindex =
		portpprotoindex = arcprotoindex = cellindex = 0;

	/* count and number the cells, nodes, arcs, and ports in this library */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		np->temp1 = nodeprotoindex++;
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			pp->temp1 = portprotoindex++;
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			ai->temp1 = arcindex++;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			ni->temp1 = nodeindex++;
	}
	cellshere = nodeprotoindex;

	/* prepare to locate references to cells in other libraries */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->temp2 = 0;
	}

	/* scan for all cross-library references */
	io_findxlibvariables(lib->firstvar, lib->numvar);
	for(i=0; i<el_maxtools; i++)
		io_findxlibvariables(el_tools[i].firstvar, el_tools[i].numvar);
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		io_findxlibvariables(tech->firstvar, tech->numvar);
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			io_findxlibvariables(ap->firstvar, ap->numvar);
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			io_findxlibvariables(np->firstvar, np->numvar);
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				io_findxlibvariables(pp->firstvar, pp->numvar);
		}
	}
	for(v = el_views; v != NOVIEW; v = v->nextview)
		io_findxlibvariables(v->firstvar, v->numvar);
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		io_findxlibvariables(np->firstvar, np->numvar);
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			io_findxlibvariables(pp->firstvar, pp->numvar);
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			io_findxlibvariables(ni->firstvar, ni->numvar);
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				io_findxlibvariables(pi->firstvar, pi->numvar);
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				io_findxlibvariables(pe->firstvar, pe->numvar);
			if (ni->proto->primindex == 0)
			{
				ni->proto->temp2 = 1;
			}
		}
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			io_findxlibvariables(ai->firstvar, ai->numvar);
	}

	/* count and number the cells in other libraries */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		if (olib == lib) continue;
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp2 == 0) continue;
			np->temp1 = nodeprotoindex++;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = portprotoindex++;
		}
	}

	/* count and number the primitive node and port prototypes */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			np->temp1 = -2 - nodepprotoindex++;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->temp1 = -2 - portpprotoindex++;
		}
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			ap->temp1 = -2 - arcprotoindex++;
	}

	/* write number of objects */
	io_putoutbig(&nodepprotoindex);
	io_putoutbig(&portpprotoindex);
	io_putoutbig(&arcprotoindex);
	io_putoutbig(&nodeprotoindex);
	io_putoutbig(&nodeindex);
	io_putoutbig(&portprotoindex);
	io_putoutbig(&arcindex);
	io_putoutbig(&geomindex);

	/* write the current cell */
	if (lib->curnodeproto == NONODEPROTO) curnodeproto = -1; else
		curnodeproto = lib->curnodeproto->temp1;
	io_putoutbig(&curnodeproto);

	if (io_verbose > 0)
	{
		ttyputmsg(M_("Writing %ld tools, %ld technologies"), aacount, techcount);
		ttyputmsg(M_("        %ld prim nodes, %ld prim ports, %ld arc protos"),
			nodepprotoindex, portpprotoindex, arcprotoindex);
		ttyputmsg(M_("        %ld cells %ld cells, %ld ports, %ld nodes, %ld arcs"),
			cellindex, nodeprotoindex, portprotoindex, nodeindex, arcindex);
	}

	/* write the version number */
	io_putstring(el_version);

	/* number the views and write nonstandard ones */
	for(v = el_views; v != NOVIEW; v = v->nextview) v->temp1 = 0;
	el_unknownview->temp1 = -1;
	el_layoutview->temp1 = -2;
	el_schematicview->temp1 = -3;
	el_iconview->temp1 = -4;
	el_simsnapview->temp1 = -5;
	el_skeletonview->temp1 = -6;
	el_vhdlview->temp1 = -7;
	el_netlistview->temp1 = -8;
	el_docview->temp1 = -9;
	el_netlistnetlispview->temp1 = -10;
	el_netlistalsview->temp1 = -11;
	el_netlistquiscview->temp1 = -12;
	el_netlistrsimview->temp1 = -13;
	el_netlistsilosview->temp1 = -14;
	el_verilogview->temp1 = -15;
	i = 1;
	for(v = el_views; v != NOVIEW; v = v->nextview)
		if (v->temp1 == 0) v->temp1 = i++;
	i--;
	io_putoutbig(&i);
	for(v = el_views; v != NOVIEW; v = v->nextview)
	{
		if (v->temp1 < 0) continue;
		io_putstring(v->viewname);
		io_putstring(v->sviewname);
	}

	/* write total number of arcinsts, nodeinsts, and ports in each cell */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(a = 0, ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst) a++;
		io_putoutbig(&a);
		for(n = 0, ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst) n++;
		io_putoutbig(&n);
		for(p = 0, pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) p++;
		io_putoutbig(&p);
		if (io_verbose > 0) ttyputmsg(M_("Cell %s has %ld arcs, %ld nodes, %ld ports"),
			describenodeproto(np), a, n, p);
	}

	/* write dummy numbers of arcinsts and nodeinst; count ports for external cells */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		if (olib == lib) continue;
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp2 == 0) continue;
			a = -1;
			io_putoutbig(&a);
			io_putoutbig(&a);
			for(p = 0, pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) p++;
			io_putoutbig(&p);
		}
	}

	/* write the names of technologies and primitive prototypes */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		/* write the technology name */
		io_putstring(tech->techname);

		/* count and write the number of primitive node prototypes */
		for(np = tech->firstnodeproto, i=0; np != NONODEPROTO; np = np->nextnodeproto) i++;
		io_putoutbig(&i);

		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			/* write the primitive node prototype name */
			io_putstring(np->protoname);
			for(pp = np->firstportproto, i=0; pp != NOPORTPROTO; pp = pp->nextportproto) i++;
			io_putoutbig(&i);
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				io_putstring(pp->protoname);
		}

		/* count and write the number of arc prototypes */
		for(ap = tech->firstarcproto, i=0; ap != NOARCPROTO; ap = ap->nextarcproto) i++;
		io_putoutbig(&i);

		/* write the primitive arc prototype names */
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			io_putstring(ap->protoname);
	}

	/* write the names of the tools */
	for(i=0; i<el_maxtools; i++) io_putstring(el_tools[i].toolname);

	/* write the userbits for the library */
	io_putoutbig(&lib->userbits);

	/* write the tool lambda values */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		io_putoutbig(&lib->lambda[tech->techindex]);

	/* write the global namespace */
	io_writenamespace();
	io_outputstage = TRUE;

	/* write the library variables */
	io_writevariables(lib->firstvar, lib->numvar, VLIBRARY);

	/* write the tool variables */
	for(i=0; i<el_maxtools; i++)
		io_writevariables(el_tools[i].firstvar, el_tools[i].numvar, VTOOL);

	/* write the variables on technologies */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		io_writevariables(tech->firstvar, tech->numvar, VTECHNOLOGY);

	/* write the arcproto variables */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			io_writevariables(ap->firstvar, ap->numvar, VARCPROTO);

	/* write the variables on primitive node prototypes */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			io_writevariables(np->firstvar, np->numvar, VNODEPROTO);

	/* write the variables on primitive port prototypes */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				io_writevariables(pp->firstvar, pp->numvar, VPORTPROTO);

	/* write the view variables */
	i = 0;
	for(v = el_views; v != NOVIEW; v = v->nextview) i++;
	io_putoutbig(&i);
	for(v = el_views; v != NOVIEW; v = v->nextview)
	{
		io_putoutbig(&v->temp1);
		io_writevariables(v->firstvar, v->numvar, VVIEW);
	}

	/* write all of the cells in this library */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (stopping(STOPREASONBINARY)) longjmp(io_filerror, 1);
		io_writenodeproto(np, TRUE);
	}

	/* write all of the cells in external libraries */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		if (olib == lib) continue;
		for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp2 == 0) continue;
			io_writenodeproto(np, FALSE);
		}
	}

	/* write all of the arcs and nodes in this library */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (stopping(STOPREASONBINARY)) longjmp(io_filerror, 1);
		a = n = 0;
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			io_writearcinst(ai);
			a++;
		}
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			io_writenodeinst(ni);
			n++;
		}
		if (io_verbose > 0) ttyputmsg(M_("Wrote %ld arcs and %ld nodes in cell %s"),
			a, n, describenodeproto(np));
	}

	/* close the output file */
	xclose(io_fileout);

	/* restore any damage to the database */
	io_cleanup();

	/* if the file was renamed, kill the backup copy */
	if (rename != 0)
	{
		if (eunlink(rename) < 0) ttyputerr(_("Error: cannot delete backup file %s"), rename);
		efree(rename);
	}

	if ((lib->userbits&HIDDENLIBRARY) == 0)
	{
		ttyputmsg(_("%s written on %s (%ld %s)"), name,
			timetostring(getcurrenttime()), cellshere,
				makeplural(_("cell"), nodeprotoindex));
#ifdef ONUNIX
		efprintf(stdout, _("%s written on %s (%ld %s)\n"), name,
			timetostring(getcurrenttime()), cellshere,
				makeplural(_("cell"), nodeprotoindex));
#endif
	}
	lib->userbits = (lib->userbits & ~(LIBCHANGEDMAJOR | LIBCHANGEDMINOR)) | READFROMDISK;
	return(FALSE);
}

/*
 * routine to clean-up the internal database after it has been modified
 * for binary output
 */
void io_cleanup(void)
{
	/* if binary output didn't get far, no cleanup needed */
	if (!io_outputstage) return;

	/* restore damage to the global namespace */
	io_restorenamespace();
}

/******************** COMPONENT OUTPUT ********************/

void io_writenodeproto(NODEPROTO *np, BOOLEAN thislib)
{
	INTBIG i;
	REGISTER PORTPROTO *pp;
	REGISTER LIBRARY *instlib;
	static INTBIG nullptr = -1;

	/* write cell information */
	io_putstring(np->protoname);
	io_putoutbig(&np->nextcellgrp->temp1);
	if (np->nextcont != NONODEPROTO) io_putoutbig(&np->nextcont->temp1); else
		io_putoutbig(&nullptr);
	io_putoutbig(&np->cellview->temp1);
	i = np->version;   io_putoutbig(&i);
	io_putoutbig(&np->creationdate);
	io_putoutbig(&np->revisiondate);

	/* write the nodeproto bounding box */
	io_putoutbig(&np->lowx);
	io_putoutbig(&np->highx);
	io_putoutbig(&np->lowy);
	io_putoutbig(&np->highy);

	if (!thislib)
	{
		instlib = np->lib;
		io_putstring(instlib->libfile);
	}

	/* write the number of portprotos on this nodeproto */
	i = 0;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) i++;
	io_putoutbig(&i);

	/* write the portprotos on this nodeproto */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (thislib)
		{
			/* write the connecting subnodeinst for this portproto */
			if (pp->subnodeinst != NONODEINST) i = pp->subnodeinst->temp1; else
			{
				ttyputmsg(_("ERROR: Library %s, cell %s, export %s has no node"),
					np->lib->libname, describenodeproto(np), pp->protoname);
				i = -1;
			}	
			io_putoutbig(&i);

			/* write the portproto index in the subnodeinst */
			if (pp->subnodeinst != NONODEINST && pp->subportproto != NOPORTPROTO)
			{
				i = pp->subportproto->temp1;
			} else
			{
				if (pp->subportproto != NOPORTPROTO)
					ttyputmsg(_("ERROR: Library %s, cell %s, export %s has no subport"),
						np->lib->libname, describenodeproto(np), pp->protoname);
				i = -1;
			}
			io_putoutbig(&i);
		}

		/* write the portproto name */
		io_putstring(pp->protoname);

		if (thislib)
		{
			/* write the text descriptor */
			io_putoutbig(&pp->textdescript[0]);
			io_putoutbig(&pp->textdescript[1]);

			/* write the portproto tool information */
			io_putoutbig(&pp->userbits);

			/* write variable information */
			io_writevariables(pp->firstvar, pp->numvar, VPORTPROTO);
		}
	}

	if (thislib)
	{
		/* write tool information */
		io_putoutbig(&np->adirty);
		io_putoutbig(&np->userbits);

		/* write variable information */
		io_writevariables(np->firstvar, np->numvar, VNODEPROTO);
	}

	if (io_verbose > 0) ttyputmsg(M_("Wrote cell %s"), describenodeproto(np));
}

void io_getnodedisplayposition(NODEINST *ni, INTBIG *xpos, INTBIG *ypos);

/*
 * Similar to "us_getnodedisplayposition()"
 */
void io_getnodedisplayposition(NODEINST *ni, INTBIG *xpos, INTBIG *ypos)
{
	REGISTER NODEPROTO *np;
	INTBIG cox, coy;
	REGISTER INTBIG dx, dy, cx, cy;
	REGISTER VARIABLE *var;
	XARRAY trans;

	np = ni->proto;
	if ((us_useroptions&CENTEREDPRIMITIVES) == 0)
	{
		corneroffset(ni, np, ni->rotation, ni->transpose, &cox, &coy, FALSE);
		*xpos = ni->lowx+cox;
		*ypos = ni->lowy+coy;
	} else
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
		cx = cy = 0;
		if (var != NOVARIABLE)
		{
			cx = ((INTBIG *)var->addr)[0];
			cy = ((INTBIG *)var->addr)[1];
		}
		dx = cx + (ni->lowx+ni->highx)/2 - (np->lowx+np->highx)/2;
		dy = cy + (ni->lowy+ni->highy)/2 - (np->lowy+np->highy)/2;
		makerot(ni, trans);
		xform(dx, dy, &cox, &coy, trans);
		*xpos = cox;
		*ypos = coy;
	}
}

void io_writenodeinst(NODEINST *ni)
{
	INTBIG i, x, y;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	/* write the nodeproto pointer */
	io_putoutbig(&ni->proto->temp1);

	/* write descriptive information */
	io_putoutbig(&ni->lowx);
	io_putoutbig(&ni->lowy);
	io_putoutbig(&ni->highx);
	io_putoutbig(&ni->highy);

	/* if writing a cell reference, include the anchor point */
	if (ni->proto->primindex == 0)
	{
		/* write the anchor point of the cell */
		io_getnodedisplayposition(ni, &x, &y);
		io_putoutbig(&x);
		io_putoutbig(&y);
	}

	if (ni->transpose != 0) i = 1; else i = 0;
	io_putoutbig(&i);
	i = ni->rotation;
	io_putoutbig(&i);

	io_putoutbig(&ni->textdescript[0]);
	io_putoutbig(&ni->textdescript[1]);

	/* count the arc ports */
	i = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst) i++;
	io_putoutbig(&i);

	/* write the arc ports */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		/* write the arcinst index (and the particular end on that arc) */
		ai = pi->conarcinst;
		if (ai->end[0].portarcinst == pi) i = 0; else i = 1;
		i = (ai->temp1 << 1) + (ai->end[0].portarcinst == pi ? 0 : 1);
		io_putoutbig(&i);

		/* write the portinst prototype */
		io_putoutbig(&pi->proto->temp1);

		/* write the variable information */
		io_writevariables(pi->firstvar, pi->numvar, VPORTARCINST);
	}

	/* count the exports */
	i = 0;
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst) i++;
	io_putoutbig(&i);

	/* write the exports */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		io_putoutbig(&pe->exportproto->temp1);

		/* write the portinst prototype */
		io_putoutbig(&pe->proto->temp1);

		/* write the variable information */
		io_writevariables(pe->firstvar, pe->numvar, VPORTEXPINST);
	}

	/* write the tool information */
	io_putoutbig(&ni->userbits);

	/* write variable information */
	io_writevariables(ni->firstvar, ni->numvar, VNODEINST);
}

void io_writearcinst(ARCINST *arcinst)
{
	INTBIG i;

	/* write the arcproto pointer */
	io_putoutbig(&arcinst->proto->temp1);

	/* write basic arcinst information */
	io_putoutbig(&arcinst->width);

	/* write the arcinst end information */
	for(i=0; i<2; i++)
	{
		io_putoutbig(&arcinst->end[i].xpos);
		io_putoutbig(&arcinst->end[i].ypos);

		/* write the arcinst's connecting nodeinst index */
		io_putoutbig(&arcinst->end[i].nodeinst->temp1);
	}

	/* write the arcinst's tool information */
	io_putoutbig(&arcinst->userbits);

	/* write variable information */
	io_writevariables(arcinst->firstvar, arcinst->numvar, VARCINST);
}

/******************** VARIABLE ROUTINES ********************/

/* routine to write the global namespace */
void io_writenamespace(void)
{
	REGISTER INTBIG i;
	INTBIG val;

	val = el_numnames;
	io_putoutbig(&val);
	if (el_numnames == 0) return;

	for(i=0; i<el_numnames; i++) io_putstring(el_namespace[i]);

	io_namesave = (INTSML *)emalloc(el_numnames*SIZEOFINTSML, el_tempcluster);
	if (io_namesave == 0)
	{
		ttyputerr(_("No memory! destroying internal variables to save file"));
		ttyputerr(_("Saved file is good, memory NOT: quit when save finishes!"));
		for(i=0; i<el_numnames; i++) *((INTSML *)el_namespace[i]) = (INTSML)i;
	} else for(i=0; i<el_numnames; i++)
	{
		io_namesave[i] = *((INTSML *)el_namespace[i]);
		*((INTSML *)el_namespace[i]) = (INTSML)i;
	}
}

void io_restorenamespace(void)
{
	REGISTER INTBIG i;

	if (el_numnames == 0) return;
	for(i=0; i<el_numnames; i++)
		*((INTSML *)el_namespace[i]) = io_namesave[i];
	efree((CHAR *)io_namesave);
}

/*
 * Routine to scan the variables on an object (which are in "firstvar" and "numvar")
 * for NODEPROTO references.  Any found are marked (by setting "temp2" to 1).
 * This is used to gather cross-library references.
 */
void io_findxlibvariables(VARIABLE *firstvar, INTSML numvar)
{
	REGISTER INTBIG i;
	REGISTER INTBIG len, type, j;
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *np, **nparray;

	for(i=0; i<numvar; i++)
	{
		var = &firstvar[i];
		type = var->type;
		if ((type&VDONTSAVE) != 0) continue;
		if ((type&VTYPE) != VNODEPROTO) continue;
		if ((type&VISARRAY) != 0)
		{
			len = (type&VLENGTH) >> VLENGTHSH;
			nparray = (NODEPROTO **)var->addr;
			if (len == 0) for(len=0; nparray[len] != NONODEPROTO; len++) ;
			for(j=0; j<len; j++)
			{
				np = nparray[j];
				if (np != NONODEPROTO && np->primindex == 0)
				{
					np->temp2 = 1;
				}
			}
		} else
		{
			np = (NODEPROTO *)var->addr;
			if (np != NONODEPROTO && np->primindex == 0)
			{
				np->temp2 = 1;
			}
		}
	}
}

void io_writevariables(VARIABLE *firstvar, INTSML numvar, INTBIG type)
{
	REGISTER INTBIG i, j, datasize;
	INTBIG num, ty, len, addr;

	for(num=i=0; i<numvar; i++)
		if ((firstvar[i].type&VDONTSAVE) == 0) num++;
	io_putoutbig(&num);
	for(i=0; i<numvar; i++)
	{
		if ((firstvar[i].type&VDONTSAVE) != 0) continue;
		io_putout((void *)firstvar[i].key, SIZEOFINTSML);
		ty = firstvar[i].type;
		io_putoutbig(&ty);
		io_putoutbig(&firstvar[i].textdescript[0]);
		io_putoutbig(&firstvar[i].textdescript[1]);
		addr = firstvar[i].addr;
		if ((ty&VISARRAY) != 0)
		{
			len = (ty&VLENGTH) >> VLENGTHSH;
			if ((ty&VTYPE) == VCHAR)
			{
				datasize = 1;
				if (len == 0)
					for(len=0; ((((CHAR *)addr)[len])&0377) != 0377; len++)
						;
			} else if ((ty&VTYPE) == VDOUBLE)
			{
				datasize = SIZEOFINTBIG*2;
				if (len == 0)
					for(len=0; ((INTBIG *)addr)[len*2] != -1 &&
						((INTBIG *)addr)[len*2+1] != -1; len++)
							;
			} else if ((ty&VTYPE) == VSHORT)
			{
				datasize = 2;
				if (len == 0)
					for(len=0; ((INTSML *)addr)[len] != -1; len++)
						;
			} else
			{
				datasize = SIZEOFINTBIG;
				if (len == 0) for(len=0; ((INTBIG *)addr)[len] != -1; len++)
					;
			}
			io_putoutbig(&len);
			if ((ty&VTYPE) == VGENERAL)
			{
				for(j=0; j<len; j += 2)
				{
					io_putoutbig(((INTBIG *)(addr + (j+1)*datasize)));
					io_putoutvar((INTBIG *)(addr + j*datasize), *(INTBIG *)(addr + (j+1)*datasize));
				}
			} else
			{
				for(j=0; j<len; j++) io_putoutvar((INTBIG *)(addr + j*datasize), ty);
			}
		} else io_putoutvar(&addr, ty);
	}
}

void io_putoutvar(INTBIG *addr, INTBIG ty)
{
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER GEOM *geom;
	REGISTER NODEPROTO *np;
	INTBIG j;
	static INTBIG nullptr = -1;

	if ((ty&(VCODE1|VCODE2)) != 0) ty = VSTRING;
	switch (ty&VTYPE)
	{
		case VADDRESS:
		case VFRACT:
		case VINTEGER:
			io_putoutbig(addr);
			break;
		case VFLOAT:
			io_putout(addr, sizeof(float));
			break;
		case VDOUBLE:
			io_putout(addr, sizeof(double));
			break;
		case VBOOLEAN:
		case VCHAR:
			io_putout(addr, sizeof (CHAR));
			break;
		case VSTRING:
			io_putstring((CHAR *)*addr);
			break;
		case VTECHNOLOGY:
			if ((TECHNOLOGY *)*addr != NOTECHNOLOGY)
			{
				j = ((TECHNOLOGY *)*addr)->techindex;
				io_putoutbig(&j);
			} else io_putoutbig(&nullptr);
			break;
		case VNODEINST:
			if ((NODEINST *)*addr != NONODEINST)
				io_putoutbig(&((NODEINST *)*addr)->temp1); else
					io_putoutbig(&nullptr);
			break;
		case VNODEPROTO:
			np = (NODEPROTO *)*addr;
			if (np != NONODEPROTO) io_putoutbig(&np->temp1); else
				io_putoutbig(&nullptr);
			break;
		case VARCPROTO:
			if ((ARCPROTO *)*addr != NOARCPROTO)
				io_putoutbig(&((ARCPROTO *)*addr)->temp1); else
					io_putoutbig(&nullptr);
			break;
		case VPORTPROTO:
			if ((PORTPROTO *)*addr != NOPORTPROTO)
				io_putoutbig(&((PORTPROTO *)*addr)->temp1); else
					io_putoutbig(&nullptr);
			break;
		case VARCINST:
			if ((ARCINST *)*addr != NOARCINST)
				io_putoutbig(&((ARCINST *)*addr)->temp1); else
					io_putoutbig(&nullptr);
			break;
		case VGEOM:
			geom = (GEOM *)*addr;
			if (geom->entryisnode) j = 1; else j = 0;
			io_putoutbig(&j);
			if ((INTBIG)geom->entryaddr.blind == -1) io_putoutbig(&nullptr); else
			{
				if (geom->entryisnode)
					io_putoutbig(&geom->entryaddr.ni->temp1); else
						io_putoutbig(&geom->entryaddr.ai->temp1);
			}
			break;
		case VPORTARCINST:
			pi = (PORTARCINST *)*addr;
			if (pi != NOPORTARCINST)
			{
				ai = pi->conarcinst;
				j = (ai->temp1 << 1) + (ai->end[0].portarcinst == pi ? 0 : 1);
				io_putoutbig(&j);
			} else io_putoutbig(&nullptr);
			break;
		case VPORTEXPINST:
			if ((PORTEXPINST *)*addr != NOPORTEXPINST)
				io_putoutbig(&((PORTEXPINST *)*addr)->exportproto->temp1); else
					io_putoutbig(&nullptr);
			break;
		case VLIBRARY:
			if ((LIBRARY *)*addr != NOLIBRARY)
				io_putstring(((LIBRARY *)*addr)->libname); else
					io_putstring(x_("noname"));
			break;
		case VTOOL:
			if ((TOOL *)*addr != NOTOOL)
				io_putoutbig(&((TOOL *)*addr)->toolindex); else
					io_putoutbig(&nullptr);
			break;
		case VSHORT:
			io_putout(addr, SIZEOFINTSML);
			break;
	}
}

/******************** I/O ROUTINES ********************/

/*
 * Routine to copy file "src" to file "dst".  Displays an message and returns TRUE on error.
 */
BOOLEAN io_copyfile(CHAR *src, CHAR *dst, BOOLEAN tempfile)
{
	FILE *srcf, *dstf;
	UCHAR1 buf[FILECOPYBUFSIZE];
	CHAR *truesrcname, *truedstname;
	REGISTER INTBIG total, written, filetype;

	srcf = xopen(src, io_filetypeblib, 0, &truesrcname);
	if (srcf == NULL)
	{
		ttyputerr(_("Error: cannot open old library file %s"), src);
		return(TRUE);
	}
	filetype = io_filetypeblib;
	if (tempfile) filetype |= FILETYPETEMPFILE;
	dstf = xcreate(dst, filetype, 0, &truedstname);
	if (dstf == NULL)
	{
		xclose(srcf);
		ttyputerr(_("Error: cannot create new library file %s"), dst);
		return(TRUE);
	}
	for(;;)
	{
		total = xfread(buf, 1, FILECOPYBUFSIZE, srcf);
		if (total <= 0) break;
		written = xfwrite(buf, 1, total, dstf);
		if (written != total)
		{
			ttyputerr(_("Error: only wrote %ld out of %ld bytes to file %s"),
				written, total, truedstname);
			xclose(srcf);
			xclose(dstf);
			return(TRUE);
		}
	}
	xclose(srcf);
	xclose(dstf);
	return(FALSE);
}

void io_putoutbig(void *data)
{
	io_putout(data, SIZEOFINTBIG);
}

void io_putout(void *data, INTBIG size)
{
	REGISTER INTBIG ret;

	if (size == 0)
	{
		ttyputmsg(_("Warning: null length data item; database may be bad"));
		return;
	}
	ret = xfwrite((UCHAR1 *)data, size, 1, io_fileout);
	if (ret == 0) longjmp(io_filerror, 1);
}

void io_putstring(CHAR *name)
{
	INTBIG len;

	len = estrlen(name);
	io_putoutbig(&len);
	if (len != 0) io_putout(name, len * SIZEOFCHAR);
}
