/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iotexti.c
 * Input/output tool: textual format input
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
#include "database.h"
#include "eio.h"
#include "tecgen.h"
#include "tecart.h"
#include "tecschem.h"
#include "tecmocmos.h"
#include "tecmocmossub.h"
#include "edialogs.h"

#define REPORTRATE  100		/* number of elements between graphical update */

#define LONGJMPNOMEM         1		/* error: No Memory */
#define LONGJMPMISCOLON      2		/* error: Missing colon */
#define LONGJMPUKNKEY        3		/* error: Unknown keyword */
#define LONGJMPEOF           4		/* error: EOF too soon */
#define LONGJMPEOFVAR        5		/* error: EOF in variables */
#define LONGJMPMISCOLVAR     6		/* error: Missing ':' in variable */
#define LONGJMPMISOSBVAR     7		/* error: Missing '[' in variable */
#define LONGJMPMISOSBVARARR  8		/* error: Missing '[' in variable array */
#define LONGJMPSHORTARR      9		/* error: Short array specification */
#define LONGJMPUKNNOPROTO   10		/* error: Unknown node prototype */
#define LONGJMPMISVIEWABR   11		/* error: Missing view abbreviation */
#define LONGJMPBADVIEWABR   12		/* error: View abbreviation bad */
#define LONGJMPINCVIEWABR   13		/* error: View abbreviation inconsistent */
#define LONGJMPSECCELLNAME  14		/* error: Second cell name */
#define LONGJMPUKNTECH      15		/* error: Unknown technology */
#define LONGJMPINVARCPROTO  16		/* error: Invalid arc prototype */
#define LONGJMPUKNPORPROTO  17		/* error: Unknown port prototype name */

/* state of input (value of "io_txtindata.textlevel") */
#define INLIB       1
#define INCELL      2
#define INPORTPROTO 3
#define INNODEINST  4
#define INPOR       5
#define INARCINST   6
#define INARCEND    7

/* state of variable reading (value of "io_txtindata.varpos") */
#define INVTOOL        1
#define INVTECHNOLOGY  2
#define INVLIBRARY     3
#define INVNODEPROTO   4
#define INVNODEINST    5
#define INVPORTARCINST 6
#define INVPORTEXPINST 7
#define INVPORTPROTO   8
#define INVARCINST     9

typedef struct
{
	INTBIG      cellcount;
	INTBIG      textlevel;
	INTBIG      bitcount;
	INTBIG      maincell;
	INTBIG      curarcend;
	INTBIG      nodeinstcount;
	INTBIG      arcinstcount;
	INTBIG      varaddr;
	INTBIG      portprotocount;
	INTBIG      linenumber;
	INTBIG      curportcount;
	INTBIG      varpos;
	INTBIG      filelength;
	INTBIG      fileposition;
	INTBIG      curcellnumber;
	CHAR       *version;
	INTBIG      emajor, eminor, edetail;
	NODEINST   *curnodeinst;
	ARCINST    *curarcinst;
	PORTPROTO  *curportproto;
	NODEPROTO  *curnodeproto;
	CHAR       *cellnameread;
	LIBRARY    *curlib;
	TECHNOLOGY *curtech;
	PORTPROTO **portprotolist;		/* list of portprotos for readin */
	NODEPROTO **nodeprotolist;		/* list of cells for readin */
	ARCINST   **arclist;			/* list of arcs for readin */
	NODEINST  **nodelist;			/* list of nodes for readin */
	FILE       *textfilein;
	INTBIG      nodeinsterror;
	INTBIG      portarcinsterror;
	INTBIG      portexpinsterror;
	INTBIG      portprotoerror;
	INTBIG      arcinsterror;
	INTBIG      geomerror;
	INTBIG      rtnodeerror;
	INTBIG      libraryerror;
	INTBIG      convertmosiscmostechnologies;
} TXTINPUTDATA;

static TXTINPUTDATA io_txtindata;			/* all data pertaining to reading a file */

/* input string buffer */
static INTBIG io_maxinputline = 0;
static CHAR *io_keyword;

/* prototypes for local routines */
static void io_newlib(void);
static BOOLEAN io_getkeyword(void);
static BOOLEAN io_grabbuffers(INTBIG);
static PORTPROTO *io_getport(CHAR*, NODEPROTO*);
static void io_null(void);
static void io_newlib(void);
static void io_versn(void);
static void io_libkno(void);
static void io_libain(void);
static void io_libaib(void);
static void io_libbit(void);
static void io_libusb(void);
static void io_libte(void);
static void io_libten(void);
static void io_lambda(void);
static void io_getvar(void);
static INTBIG io_decode(CHAR*, INTBIG);
static void io_libcc(void);
static void io_libms(void);
static void io_libvie(void);
static void io_newcel(void);
static void io_celnam(void);
static void io_celver(void);
static void io_celcre(void);
static void io_celrev(void);
static void io_celext(void);
static void io_cellx(void);
static void io_celhx(void);
static void io_celly(void);
static void io_celhy(void);
static void io_tech(void);
static void io_celaad(void);
static void io_celbit(void);
static void io_celusb(void);
static void io_celnet(void);
static void io_celnoc(void);
static void io_celarc(void);
static void io_celptc(void);
static void io_celdon(void);
static void io_newno(void);
static void io_nodtyp(void);
static void io_nodlx(void);
static void io_nodhx(void);
static void io_nodly(void);
static void io_nodhy(void);
static void io_nodnam(void);
static void io_noddes(void);
static void io_nodrot(void);
static void io_nodtra(void);
static void io_nodkse(void);
static void io_nodpoc(void);
static void io_nodbit(void);
static void io_nodusb(void);
static void io_newpor(void);
static void io_porarc(void);
static void io_porexp(void);
static void io_newar(void);
static void io_arctyp(void);
static void io_arcnam(void);
static void io_arcwid(void);
static void io_arclen(void);
static void io_arcsig(void);
static void io_newend(void);
static void io_endnod(void);
static void io_endpt(void);
static void io_endxp(void);
static void io_endyp(void);
static void io_arckse(void);
static void io_arcbit(void);
static void io_arcusb(void);
static void io_arcnet(void);
static void io_newpt(void);
static void io_ptnam(void);
static void io_ptdes(void);
static void io_ptsno(void);
static void io_ptspt(void);
static void io_ptkse(void);
static void io_ptbit(void);
static void io_ptusb(void);
static void io_ptnet(void);
static TECHNOLOGY *io_gettechnology(CHAR*);

struct keyroutine
{
	CHAR *matchstring;
	void (*routine)(void);
};

/* possible keywords while reading libraries */
static struct keyroutine io_libraryroutines[] =
{
	{x_("****library"),     io_newlib},
	{x_("bits"),            io_libbit},
	{x_("lambda"),          io_lambda},
	{x_("version"),         io_versn},
	{x_("aids"),            io_libkno},
	{x_("aidname"),         io_libain},
	{x_("aidbits"),         io_libaib},
	{x_("userbits"),        io_libusb},
	{x_("techcount"),       io_libte},
	{x_("techname"),        io_libten},
	{x_("cellcount"),       io_libcc},
	{x_("maincell"),        io_libms},
	{x_("view"),            io_libvie},
	{x_("***cell"),         io_newcel},
	{x_("variables"),       io_getvar},
   {NULL, NULL}
};

/* possible keywords while reading cells */
static struct keyroutine io_cellroutines[] =
{
	{x_("bits"),            io_celbit},
	{x_("userbits"),        io_celusb},
	{x_("netnum"),          io_celnet},
	{x_("name"),            io_celnam},
	{x_("version"),         io_celver},
	{x_("creationdate"),    io_celcre},
	{x_("revisiondate"),    io_celrev},
	{x_("lowx"),            io_cellx},
	{x_("highx"),           io_celhx},
	{x_("lowy"),            io_celly},
	{x_("highy"),           io_celhy},
	{x_("externallibrary"), io_celext},
	{x_("aadirty"),         io_celaad},
	{x_("nodes"),           io_celnoc},
	{x_("arcs"),            io_celarc},
	{x_("porttypes"),       io_celptc},
	{x_("celldone"),        io_celdon},
	{x_("technology"),      io_tech},
	{x_("**node"),          io_newno},
	{x_("**arc"),           io_newar},
	{x_("***cell"),         io_newcel},
	{x_("variables"),       io_getvar},
   {NULL, NULL}
};

/* possible keywords while reading ports */
static struct keyroutine io_portroutines[] =
{
	{x_("bits"),            io_ptbit},
	{x_("userbits"),        io_ptusb},
	{x_("netnum"),          io_ptnet},
	{x_("subnode"),         io_ptsno},
	{x_("subport"),         io_ptspt},
	{x_("name"),            io_ptnam},
	{x_("descript"),        io_ptdes},
	{x_("aseen"),           io_ptkse},
	{x_("**porttype"),      io_newpt},
	{x_("**arc"),           io_newar},
	{x_("**node"),          io_newno},
	{x_("celldone"),        io_celdon},
	{x_("***cell"),         io_newcel},
	{x_("variables"),       io_getvar},
	{x_("arcbits"),         io_null},	/* used no more: ignored for compatibility */
   {NULL, NULL}
};

/* possible keywords while reading nodes */
static struct keyroutine io_noderoutines[] =
{
	{x_("bits"),            io_nodbit},
	{x_("userbits"),        io_nodusb},
	{x_("type"),            io_nodtyp},
	{x_("lowx"),            io_nodlx},
	{x_("highx"),           io_nodhx},
	{x_("lowy"),            io_nodly},
	{x_("highy"),           io_nodhy},
	{x_("rotation"),        io_nodrot},
	{x_("transpose"),       io_nodtra},
	{x_("aseen"),           io_nodkse},
	{x_("name"),            io_nodnam},
	{x_("descript"),        io_noddes},
	{x_("*port"),           io_newpor},
	{x_("**node"),          io_newno},
	{x_("**porttype"),      io_newpt},
	{x_("**arc"),           io_newar},
	{x_("celldone"),        io_celdon},
	{x_("variables"),       io_getvar},
	{x_("***cell"),         io_newcel},
	{x_("ports"),           io_nodpoc},
	{x_("exportcount"),     io_null},	/* used no more: ignored for compatibility */
   {NULL, NULL}
};

/* possible keywords while reading portinsts */
static struct keyroutine io_portinstroutines[] =
{
	{x_("arc"),             io_porarc},
	{x_("exported"),        io_porexp},
	{x_("*port"),           io_newpor},
	{x_("**node"),          io_newno},
	{x_("**porttype"),      io_newpt},
	{x_("**arc"),           io_newar},
	{x_("variables"),       io_getvar},
	{x_("celldone"),        io_celdon},
	{x_("***cell"),         io_newcel},
   {NULL, NULL}
};

/* possible keywords while reading arcs */
static struct keyroutine io_arcroutines[] =
{
	{x_("bits"),            io_arcbit},
	{x_("userbits"),        io_arcusb},
	{x_("netnum"),          io_arcnet},
	{x_("type"),            io_arctyp},
	{x_("width"),           io_arcwid},
	{x_("length"),          io_arclen},
	{x_("signals"),         io_arcsig},
	{x_("aseen"),           io_arckse},
	{x_("name"),            io_arcnam},
	{x_("*end"),            io_newend},
	{x_("**arc"),           io_newar},
	{x_("**node"),          io_newno},
	{x_("variables"),       io_getvar},
	{x_("celldone"),        io_celdon},
	{x_("***cell"),         io_newcel},
   {NULL, NULL}
};

/* possible keywords while reading arc ends */
static struct keyroutine io_arcendroutines[] =
{
	{x_("node"),            io_endnod},
	{x_("nodeport"),        io_endpt},
	{x_("xpos"),            io_endxp},
	{x_("ypos"),            io_endyp},
	{x_("*end"),            io_newend},
	{x_("**arc"),           io_newar},
	{x_("**node"),          io_newno},
	{x_("variables"),       io_getvar},
	{x_("celldone"),        io_celdon},
	{x_("***cell"),         io_newcel},
   {NULL, NULL}
};

BOOLEAN io_readtextlibrary(LIBRARY *lib)
{
	REGISTER LIBRARY *olib, *nextlib;
	REGISTER BOOLEAN ret;

	/* mark all libraries as "not being read", but "wanted" */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		olib->temp1 = 0;
		olib->userbits &= ~UNWANTEDLIB;
	}

	ret = io_doreadtextlibrary(lib, TRUE);

	/* delete unwanted libraries */
	for(olib = el_curlib; olib != NOLIBRARY; olib = nextlib)
	{
		nextlib = olib->nextlibrary;
		if ((olib->userbits&UNWANTEDLIB) == 0) continue;
		killlibrary(olib);
	}
	return(ret);
}

BOOLEAN io_doreadtextlibrary(LIBRARY *lib, BOOLEAN newprogress)
{
	struct keyroutine *table;
	REGISTER CHAR *pp;
	REGISTER INTBIG i, j, reportcount, lambda, errcode;
	INTBIG num, den;
	REGISTER INTBIG oldunit;
	REGISTER NODEPROTO *np, *onp, *prevmatch;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	CHAR *filename;
	REGISTER void *infstr;

	/* initialize the input string buffers */
	if (io_maxinputline == 0)
		if (io_grabbuffers(1000)) return(TRUE);

	io_txtindata.textfilein = xopen(truepath(lib->libfile), io_filetypetlib,
		el_libdir, &filename);
	if (io_txtindata.textfilein == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	/* update the library file name if the true location is different */
	if (estrcmp(filename, lib->libfile) != 0)
		(void)reallocstring(&lib->libfile, filename, lib->cluster);

	reportcount = io_txtindata.fileposition = 0;
	if (io_verbose < 0)
	{
		io_txtindata.filelength = filesize(io_txtindata.textfilein);
		if (io_txtindata.filelength > 0)
		{
			if (newprogress)
			{
				io_inputprogressdialog = DiaInitProgress(x_(""), _("Reading library"));
				if (io_inputprogressdialog == 0)
				{
					xclose(io_txtindata.textfilein);
					return(TRUE);
				}
			}
			infstr = initinfstr();
			formatinfstr(infstr, _("Reading library %s"), lib->libname);
			DiaSetCaptionProgress(io_inputprogressdialog, returninfstr(infstr));
			DiaSetTextProgress(io_inputprogressdialog, _("Initializing..."));
			DiaSetProgress(io_inputprogressdialog, 0, io_txtindata.filelength);
		}
	} else io_txtindata.filelength = 0;

	/* the error recovery point */
	errcode = setjmp(io_filerror);
	if (errcode != 0)
	{
		switch (errcode)
		{
			case LONGJMPNOMEM:        pp = _("No Memory");                        break;
			case LONGJMPMISCOLON:     pp = _("Missing colon");                    break;
			case LONGJMPUKNKEY:       pp = _("Unknown keyword");                  break;
			case LONGJMPEOF:          pp = _("EOF too soon");                     break;
			case LONGJMPEOFVAR:       pp = _("EOF in variables");                 break;
			case LONGJMPMISCOLVAR:    pp = _("Missing ':' in variable");          break;
			case LONGJMPMISOSBVAR:    pp = _("Missing '[' in variable");          break;
			case LONGJMPMISOSBVARARR: pp = _("Missing '[' in variable array");    break;
			case LONGJMPSHORTARR:     pp = _("Short array specification");        break;
			case LONGJMPUKNNOPROTO:   pp = _("Unknown node prototype");           break;
			case LONGJMPMISVIEWABR:   pp = _("Missing view abbreviation");        break;
			case LONGJMPBADVIEWABR:   pp = _("View abbreviation bad");            break;
			case LONGJMPINCVIEWABR:   pp = _("View abbreviation inconsistent");   break;
			case LONGJMPSECCELLNAME:  pp = _("Second cell name");                 break;
			case LONGJMPUKNTECH:      pp = _("Unknown technology");               break;
			case LONGJMPINVARCPROTO:  pp = _("Invalid arc prototype");            break;
			case LONGJMPUKNPORPROTO:  pp = _("Unknown port prototype name");      break;
			default:                  pp = _("Unknown");                          break;
		}
		ttyputerr(_("Error: %s on line %ld, keyword '%s'"), pp,
			io_txtindata.linenumber, io_keyword);
		xclose(io_txtindata.textfilein);
		if (io_verbose < 0 && io_txtindata.filelength > 0 && io_inputprogressdialog != 0)
		{
			DiaDoneProgress(io_inputprogressdialog);
			io_inputprogressdialog = 0;
		}
		return(TRUE);
	}

	/* force the library name to be the proper file name without the "txt" */
	infstr = initinfstr();
	for(i=estrlen(lib->libfile)-1; i>=0; i--) if (lib->libfile[i] == DIRSEP) break;
	pp = &lib->libfile[i+1];
	j = estrlen(pp);
	if (j > 4 && namesame(&pp[j-4], x_(".txt")) == 0) j -= 4;
	for(i=0; i<j; i++) addtoinfstr(infstr, pp[i]);
	if (reallocstring(&lib->libname, returninfstr(infstr), lib->cluster))
		longjmp(io_filerror, LONGJMPNOMEM);

	io_txtindata.curlib = lib;
	eraselibrary(lib);

	/* clear error counters */
	io_txtindata.nodeinsterror = io_txtindata.portarcinsterror = 0;
	io_txtindata.portexpinsterror = io_txtindata.portprotoerror = 0;
	io_txtindata.arcinsterror = io_txtindata.geomerror = 0;
	io_txtindata.libraryerror = io_txtindata.rtnodeerror = 0;

	io_txtindata.textlevel = INLIB;
	io_txtindata.linenumber = 0;
	for(;;)
	{
		/* get keyword from file */
		if (io_getkeyword()) break;
		pp = io_keyword;   while (*pp != 0) pp++;
		if (pp[-1] != ':') longjmp(io_filerror, LONGJMPMISCOLON);
		pp[-1] = 0;

		if (reportcount++ > REPORTRATE)
		{
			reportcount = 0;
			if (io_verbose < 0 && io_txtindata.filelength > 0 && io_inputprogressdialog != 0)
				DiaSetProgress(io_inputprogressdialog, io_txtindata.fileposition, io_txtindata.filelength);
		}

		/* determine which keyword table to use */
		switch (io_txtindata.textlevel)
		{
			case INLIB:       table = io_libraryroutines;   break;
			case INCELL:      table = io_cellroutines;      break;
			case INPORTPROTO: table = io_portroutines;      break;
			case INNODEINST:  table = io_noderoutines;      break;
			case INPOR:       table = io_portinstroutines;  break;
			case INARCINST:   table = io_arcroutines;       break;
			case INARCEND:    table = io_arcendroutines;    break;
		}

		/* parse keyword and argument */
		for(i=0; table[i].matchstring != 0; i++)
			if (estrcmp(table[i].matchstring, io_keyword) == 0) break;
		if (table[i].matchstring == 0)
			longjmp(io_filerror, LONGJMPUKNKEY);

		/* get argument to routine and call it */
		if (io_getkeyword()) longjmp(io_filerror, LONGJMPEOF);
		(*(table[i].routine))();
	}
	xclose(io_txtindata.textfilein);
	if (io_verbose < 0 && io_txtindata.filelength > 0 && io_inputprogressdialog != 0)
	{
		DiaSetProgress(io_inputprogressdialog, 999, 1000);
		DiaSetTextProgress(io_inputprogressdialog, _("Cleaning up..."));
	}

	/* set the former version number on the library */
	nextchangequiet();
	(void)setval((INTBIG)io_txtindata.curlib, VLIBRARY, x_("LIB_former_version"),
		(INTBIG)io_txtindata.version, VSTRING|VDONTSAVE);

	/* fill in any lambda values that are not specified */
	oldunit = ((io_txtindata.curlib->userbits & LIBUNITS) >> LIBUNITSSH) << INTERNALUNITSSH;
	if (oldunit == (el_units&INTERNALUNITS)) num = den = 1; else
		db_getinternalunitscale(&num, &den, el_units, oldunit);
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		lambda = io_txtindata.curlib->lambda[tech->techindex];
		if (lambda != 0) lambda = muldiv(lambda, den, num); else
		{
			lambda = el_curlib->lambda[tech->techindex];
			if (lambda == 0) lambda = tech->deflambda;
		}			
		io_txtindata.curlib->lambda[tech->techindex] = lambda;
	}

	/* see if cellgroup information was included */
	for(np = io_txtindata.curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (np->temp1 == -1) break;
	if (np != NONODEPROTO)
	{
		/* missing cellgroup information, construct it from names */
		if (io_txtindata.emajor > 7 ||
			(io_txtindata.emajor == 7 && io_txtindata.eminor > 0) ||
			(io_txtindata.emajor == 7 && io_txtindata.eminor == 0 && io_txtindata.edetail > 11))
		{
			ttyputmsg(M_("Unusual!  Version %s library has no cellgroup information"), io_txtindata.version);
		}
		io_buildcellgrouppointersfromnames(io_txtindata.curlib);
	} else if (io_txtindata.curlib->firstnodeproto != NONODEPROTO)
	{
		/* convert numbers to cellgroup pointers */
		if (io_txtindata.emajor < 7 ||
			(io_txtindata.emajor == 7 && io_txtindata.eminor == 0 && io_txtindata.edetail <= 11))
		{
			ttyputmsg(M_("Unusual!  Version %s library has cellgroup information"), io_txtindata.version);
		}
		for(np = io_txtindata.curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->nextcellgrp = NONODEPROTO;
		for(np = io_txtindata.curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->nextcellgrp != NONODEPROTO) continue;
			prevmatch = np;
			for(onp = np->nextnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			{
				if (onp->temp1 != prevmatch->temp1) continue;
				prevmatch->nextcellgrp = onp;
				prevmatch = onp;
			}
			prevmatch->nextcellgrp = np;
		}
	}

	/* if converting MOSIS CMOS technologies, store lambda in the right place */
	if (io_txtindata.convertmosiscmostechnologies != 0)
		io_txtindata.curlib->lambda[mocmos_tech->techindex] =
			io_txtindata.curlib->lambda[mocmossub_tech->techindex];

	/* if this is to be the current library, adjust technologies */
	if (io_txtindata.curlib == el_curlib)
		for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
			changetechnologylambda(tech, io_txtindata.curlib->lambda[tech->techindex]);

	/* warn if the MOSIS CMOS technologies were converted */
	if (io_txtindata.convertmosiscmostechnologies != 0)
	{
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				if (ni->proto->primindex != 0 && ni->proto->tech == mocmos_tech) break;
			if (ni != NONODEINST) break;
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				if (ai->proto->tech == mocmos_tech) break;
			if (ai != NOARCINST) break;
		}
		if (np != NONODEPROTO)
			DiaMessageInDialog(
				_("Warning: library %s has older 'mocmossub' technology, converted to new 'mocmos'"),
					lib->libname);
	}

	/* print any variable-related error messages */
	if (io_txtindata.nodeinsterror != 0)
		ttyputmsg(_("Warning: %ld invalid NODEINST pointers"), io_txtindata.nodeinsterror);
	if (io_txtindata.arcinsterror != 0)
		ttyputmsg(_("Warning: %ld invalid ARCINST pointers"), io_txtindata.arcinsterror);
	if (io_txtindata.portprotoerror != 0)
		ttyputmsg(_("Warning: %ld invalid PORTPROTO pointers"), io_txtindata.portprotoerror);
	if (io_txtindata.portarcinsterror != 0)
		ttyputmsg(_("Warning: %ld PORTARCINST pointers not restored"), io_txtindata.portarcinsterror);
	if (io_txtindata.portexpinsterror != 0)
		ttyputmsg(_("Warning: %ld PORTEXPINST pointers not restored"), io_txtindata.portexpinsterror);
	if (io_txtindata.geomerror != 0)
		ttyputmsg(_("Warning: %ld GEOM pointers not restored"), io_txtindata.geomerror);
	if (io_txtindata.rtnodeerror != 0)
		ttyputmsg(_("Warning: %ld RTNODE pointers not restored"), io_txtindata.rtnodeerror);
	if (io_txtindata.libraryerror != 0)
		ttyputmsg(_("Warning: %ld LIBRARY pointers not restored"), io_txtindata.libraryerror);

	if (io_txtindata.maincell != -1 && io_txtindata.maincell < io_txtindata.cellcount)
		io_txtindata.curlib->curnodeproto = io_txtindata.nodeprotolist[io_txtindata.maincell]; else
			io_txtindata.curlib->curnodeproto = NONODEPROTO;
	if (io_txtindata.cellcount != 0) efree((CHAR *)io_txtindata.nodeprotolist);
	if (io_verbose < 0 && io_txtindata.filelength > 0 && newprogress && io_inputprogressdialog != 0)
	{
		DiaDoneProgress(io_inputprogressdialog);
		io_inputprogressdialog = 0;
	}

	/* clean up the library */
	io_fixnewlib(io_txtindata.curlib, 0);
	io_txtindata.curlib->userbits &= ~(LIBCHANGEDMAJOR | LIBCHANGEDMINOR);
	efree(io_txtindata.version);
	return(FALSE);
}

/******************** GENERAL PARSING ROUTINES ********************/

BOOLEAN io_getkeyword(void)
{
	REGISTER INTBIG c, cindex, inquote;

	/* skip leading blanks */
	for(;;)
	{
		c = xgetc(io_txtindata.textfilein);
		io_txtindata.fileposition++;
		if (c == '\n')
		{
			io_txtindata.linenumber++;
			if (io_verbose == 0 && io_txtindata.linenumber%1000 == 0)
				ttyputmsg(_("%ld lines read"), io_txtindata.linenumber);
			continue;
		}
		if (c != ' ') break;
	}

	/* if the file ended, quit now */
	if (c == EOF) return(TRUE);

	/* collect the word */
	cindex = 0;
	if (c == '"') inquote = 1; else inquote = 0;
	io_keyword[cindex++] = (CHAR)c;
	for(;;)
	{
		c = xgetc(io_txtindata.textfilein);
		if (c == EOF) return(TRUE);
		io_txtindata.fileposition++;
		if (c == '\n' || (c == ' ' && inquote == 0)) break;
		if (cindex >= io_maxinputline)
		{
			/* try to allocate more space */
			if (io_grabbuffers(io_maxinputline*2)) break;
		}
		if (c == '"' && (cindex == 0 || io_keyword[cindex-1] != '^'))
			inquote = 1 - inquote;
		io_keyword[cindex++] = (CHAR)c;
	}
	if (c == '\n')
	{
		io_txtindata.linenumber++;
		if (io_verbose == 0 && io_txtindata.linenumber%1000 == 0)
			ttyputmsg(_("%ld lines read"), io_txtindata.linenumber);
	}
	io_keyword[cindex] = 0;
	return(FALSE);
}

/*
 * routine to allocate the keyword buffer to be the specified size
 */
BOOLEAN io_grabbuffers(INTBIG size)
{
	REGISTER CHAR *p;
	REGISTER INTBIG i;

	/* first get the new buffer */
	p = (CHAR *)emalloc((size+1) * SIZEOFCHAR, io_tool->cluster);
	if (p == 0)
	{
		ttyputnomemory();
		return(TRUE);
	}

	/* if there was an old buffer, copy it */
	if (io_maxinputline != 0)
	{
		for(i=0; i<io_maxinputline; i++) p[i] = io_keyword[i];
		efree(io_keyword);
	}

	/* set the new size and buffer */
	io_maxinputline = size;
	io_keyword = p;
	return(FALSE);
}

/*
 * helper routine to parse a port prototype name "line" that should be
 * in node prototype "np".  The routine returns NOPORTPROTO if it cannot
 * figure out what port this name refers to.
 */
PORTPROTO *io_getport(CHAR *line, NODEPROTO *np)
{
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i;

	pp = getportproto(np, line);
	if (pp != NOPORTPROTO) return(pp);

	/* convert special port names */
	pp = io_convertoldportname(line, np);
	if (pp != NOPORTPROTO) return(pp);

	/* try to parse version 1 port names */
	if (io_txtindata.version[0] == '1')
	{
		/* see if database uses shortened name */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			i = estrlen(pp->protoname);
			if (namesamen(line, pp->protoname, i) == 0) return(pp);
		}

		/* see if the port name ends in a digit, fake with that */
		i = estrlen(line);
		if (line[i-2] == '-' && line[i-1] >= '0' && line[i-1] <= '9')
		{
			i = (eatoi(&line[i-1])-1) / 3;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (i-- == 0) return(pp);
		}
	}

	/* sorry, cannot figure out what port prototype this is */
	return(NOPORTPROTO);
}

/*
 * null routine for ignoring keywords
 */
void io_null(void) {}

/******************** LIBRARY PARSING ROUTINES ********************/

/*
 * a new library is introduced (keyword "****library")
 * This should be the first keyword in the file
 */
void io_newlib(void)
{
	REGISTER TECHNOLOGY *tech;

	/* set defaults */
	io_txtindata.maincell = -1;
	(void)allocstring(&io_txtindata.version, x_("1.00"), el_tempcluster);
	io_txtindata.varpos = INVTOOL;
	io_txtindata.curtech = NOTECHNOLOGY;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		io_txtindata.curlib->lambda[tech->techindex] = 0;
	io_txtindata.textlevel = INLIB;
}

/*
 * get the file's Electric version number (keyword "version")
 */
void io_versn(void)
{
	(void)reallocstring(&io_txtindata.version, io_keyword, el_tempcluster);

	/* for versions before 6.03q, convert MOSIS CMOS technology names */
	parseelectricversion(io_txtindata.version, &io_txtindata.emajor, &io_txtindata.eminor,
		&io_txtindata.edetail);
	io_txtindata.convertmosiscmostechnologies = 0;
	if (io_txtindata.emajor < 6 ||
		(io_txtindata.emajor == 6 && io_txtindata.eminor < 3) ||
		(io_txtindata.emajor == 6 && io_txtindata.eminor == 3 && io_txtindata.edetail < 17))
	{
		if ((asktech(mocmossub_tech, x_("get-state"))&MOCMOSSUBNOCONV) == 0)
			io_txtindata.convertmosiscmostechnologies = 1;
	}
#ifdef REPORTCONVERSION
	ttyputmsg(x_("Library is version %s (%ld.%ld.%ld)"), io_txtindata.version, io_txtindata.emajor,
		io_txtindata.eminor, io_txtindata.edetail);
	if (io_txtindata.convertmosiscmostechnologies != 0)
		ttyputmsg(x_("   Converting MOSIS CMOS technologies (mocmossub => mocmos)"));
#endif
}

/*
 * get the number of tools (keyword "aids")
 */
void io_libkno(void)
{
	io_txtindata.bitcount = 0;
}

/*
 * get the name of the tool (keyword "aidname")
 */
void io_libain(void)
{
	REGISTER TOOL *tool;

	tool = gettool(io_keyword);
	if (tool == NOTOOL) io_txtindata.bitcount = -1; else
		io_txtindata.bitcount = tool->toolindex + 1;
}

/*
 * get the number of toolbits (keyword "aidbits")
 */
void io_libaib(void)
{
	io_txtindata.bitcount = 0;
}

/*
 * get tool information for the library (keyword "bits")
 */
void io_libbit(void)
{
	if (io_txtindata.bitcount == 0)
		io_txtindata.curlib->userbits = eatoi(io_keyword);
	io_txtindata.bitcount++;
}

/*
 * get the number of toolbits (keyword "userbits")
 */
void io_libusb(void)
{
	io_txtindata.curlib->userbits = eatoi(io_keyword);

	/* this library came as readable dump, so don't automatically save it to disk */
	io_txtindata.curlib->userbits &= ~READFROMDISK;
}

/*
 * get the number of technologies (keyword "techcount")
 */
void io_libte(void)
{
	io_txtindata.varpos = INVTECHNOLOGY;
	io_txtindata.bitcount = 0;
}

/*
 * get the name of the technology (keyword "techname")
 */
void io_libten(void)
{
	REGISTER TECHNOLOGY *tech;

	tech = gettechnology(io_keyword);
	if (tech == NOTECHNOLOGY) io_txtindata.bitcount = -1; else
		io_txtindata.bitcount = tech->techindex;
}

/*
 * get lambda values for each technology in library (keyword "lambda")
 */
void io_lambda(void)
{
	REGISTER INTBIG lam;

	if (io_txtindata.bitcount >= el_maxtech ||
		io_txtindata.bitcount < 0) return;
	lam = eatoi(io_keyword);

	/* for version 4.0 and earlier, scale lambda by 20 */
	if (eatoi(io_txtindata.version) <= 4) lam *= 20;
	io_txtindata.curlib->lambda[io_txtindata.bitcount++] = lam;
}

/*
 * get variables on current object (keyword "variables")
 */
void io_getvar(void)
{
	REGISTER INTBIG i, j, count, type, len, naddr, ntype, thisnaddr, *store, key, ret;
	REGISTER float *fstore;
	REGISTER INTSML *sstore;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER CHAR *pt, *start, *varname;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *t;
	REGISTER void *infstr;

	naddr = -1;
	switch (io_txtindata.varpos)
	{
		case INVTOOL:				/* keyword applies to tools */
			if (io_txtindata.bitcount < 0) break;
			naddr = (INTBIG)&el_tools[io_txtindata.bitcount-1];
			ntype = VTOOL;
			break;
		case INVTECHNOLOGY:			/* keyword applies to technologies */
			if (io_txtindata.bitcount < 0) break;
			for(t = el_technologies; t != NOTECHNOLOGY; t = t->nexttechnology)
				if (t->techindex == io_txtindata.bitcount-1) break;
			naddr = (INTBIG)t;
			ntype = VTECHNOLOGY;
			break;
		case INVLIBRARY:			/* keyword applies to library */
			naddr = (INTBIG)io_txtindata.curlib;
			ntype = VLIBRARY;
			break;
		case INVNODEPROTO:			/* keyword applies to nodeproto */
			naddr = (INTBIG)io_txtindata.curnodeproto;    ntype = VNODEPROTO;
			break;
		case INVNODEINST:			/* keyword applies to nodeinst */
			naddr = (INTBIG)io_txtindata.curnodeinst;    ntype = VNODEINST;
			break;
		case INVPORTARCINST:		/* keyword applies to portarcinsts */
			naddr = (INTBIG)io_txtindata.varaddr;    ntype = VPORTARCINST;
			break;
		case INVPORTEXPINST:		/* keyword applies to portexpinsts */
			naddr = (INTBIG)io_txtindata.varaddr;    ntype = VPORTEXPINST;
			break;
		case INVPORTPROTO:			/* keyword applies to portproto */
			naddr = (INTBIG)io_txtindata.curportproto;    ntype = VPORTPROTO;
			break;
		case INVARCINST:			/* keyword applies to arcinst */
			naddr = (INTBIG)io_txtindata.curarcinst;    ntype = VARCINST;
			break;
	}

	/* find out how many variables to read */
	count = eatoi(io_keyword);
	for(i=0; i<count; i++)
	{
		/* read the first keyword with the name, type, and descriptor */
		if (io_getkeyword()) longjmp(io_filerror, LONGJMPEOFVAR);
		if (io_keyword[estrlen(io_keyword)-1] != ':')
			longjmp(io_filerror, LONGJMPMISCOLVAR);

		/* get the variable name */
		infstr = initinfstr();
		for(pt = io_keyword; *pt != 0; pt++)
		{
			if (*pt == '^' && pt[1] != 0)
			{
				pt++;
				addtoinfstr(infstr, *pt);
			}
			if (*pt == '(' || *pt == '[' || *pt == ':') break;
			addtoinfstr(infstr, *pt);
		}
		varname = returninfstr(infstr);
		key = makekey(varname);

		/* see if the variable is valid */
		thisnaddr = naddr;
		if (isdeprecatedvariable(naddr, ntype, varname)) thisnaddr = -1;

		/* get optional length */
		if (*pt == '(')
		{
			len = myatoi(&pt[1]);
			while (*pt != '[' && *pt != ':' && *pt != 0) pt++;
		} else len = -1;

		/* get type */
		if (*pt != '[') longjmp(io_filerror, LONGJMPMISOSBVAR);
		type = myatoi(&pt[1]);

		/* get the descriptor */
		while (*pt != ',' && *pt != ':' && *pt != 0) pt++;
		TDCLEAR(descript);
		if (*pt == ',')
		{
			descript[0] = myatoi(&pt[1]);
			for(pt++; *pt != 0 && *pt != '/' && *pt != ']'; pt++) ;
			if (*pt == '/')
				descript[1] = myatoi(pt+1);
		}

		/* get value */
		if (io_getkeyword()) longjmp(io_filerror, LONGJMPEOFVAR);
		if ((type&VISARRAY) != 0)
		{
			if (len < 0)
			{
				len = (type&VLENGTH) >> VLENGTHSH;
				store = emalloc((len * SIZEOFINTBIG), el_tempcluster);
				if (store == 0) longjmp(io_filerror, LONGJMPNOMEM);
				fstore = (float *)store;
				sstore = (INTSML *)store;
			} else
			{
				store = emalloc(((len+1) * SIZEOFINTBIG), el_tempcluster);
				if (store == 0) longjmp(io_filerror, LONGJMPNOMEM);
				fstore = (float *)store;
				sstore = (INTSML *)store;
				store[len] = -1;
				if ((type&VTYPE) == VFLOAT) fstore[len] = castfloat(-1);
				if ((type&VTYPE) == VSHORT) sstore[len] = -1;
			}
			pt = io_keyword;
			if (*pt++ != '[')
				longjmp(io_filerror, LONGJMPMISOSBVARARR);
			for(j=0; j<len; j++)
			{
				/* string arrays must be handled specially */
				if ((type&VTYPE) == VSTRING)
				{
					while (*pt != '"' && *pt != 0) pt++;
					if (*pt != 0)
					{
						start = pt++;
						for(;;)
						{
							if (pt[0] == '^' && pt[1] != 0)
							{
								pt += 2;
								continue;
							}
							if (pt[0] == '"' || pt[0] == 0) break;
							pt++;
						}
						if (*pt != 0) pt++;
					}
				} else
				{
					start = pt;
					while (*pt != ',' && *pt != ']' && *pt != 0) pt++;
				}
				if (*pt == 0)
					longjmp(io_filerror, LONGJMPSHORTARR);
				*pt++ = 0;
				if ((type&VTYPE) == VGENERAL)
				{
					if ((j&1) == 0)
					{
						store[j+1] = myatoi(start);
					} else
					{
						store[j-1] = io_decode(start, store[j]);
					}
				} else
				{
					if ((type&VTYPE) == VFLOAT)
					{
						fstore[j] = castfloat(io_decode(start, type));
					} else if ((type&VTYPE) == VSHORT)
					{
						sstore[j] = (INTSML)io_decode(start, type);
					} else
					{
						store[j] = io_decode(start, type);
					}
				}
			}
			if (thisnaddr != -1)
			{
				nextchangequiet();
				var = setvalkey(thisnaddr, ntype, key, (INTBIG)store, type);
				if (var == NOVARIABLE) longjmp(io_filerror, LONGJMPNOMEM);
				TDCOPY(var->textdescript, descript);

				/* handle updating of technology caches */
				if (ntype == VTECHNOLOGY)
					changedtechnologyvariable(key);
			}
			efree((CHAR *)store);
		} else
		{
			ret = io_decode(io_keyword, type);
			if (thisnaddr != -1)
			{
				nextchangequiet();
				var = setvalkey(thisnaddr, ntype, key, ret, type);
				if (var == NOVARIABLE) longjmp(io_filerror, LONGJMPNOMEM);
				TDCOPY(var->textdescript, descript);

				/* handle updating of technology caches */
				if (ntype == VTECHNOLOGY)
					changedtechnologyvariable(key);
			}
		}
	}
}

INTBIG io_decode(CHAR *name, INTBIG type)
{
	REGISTER INTBIG thistype, cindex;
	REGISTER CHAR *out, *retur;
	REGISTER NODEPROTO *np;

	thistype = type;
	if ((thistype&(VCODE1|VCODE2)) != 0) thistype = VSTRING;

	switch (thistype&VTYPE)
	{
		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
		case VFRACT:
		case VADDRESS:
			return(myatoi(name));
		case VCHAR:
			return((INTBIG)name);
		case VSTRING:
			if (*name == '"') name++;
			retur = name;
			for(out = name; *name != 0; name++)
			{
				if (*name == '^' && name[1] != 0)
				{
					name++;
					*out++ = *name;
					continue;
				}
				if (*name == '"') break;
				*out++ = *name;
			}
			*out = 0;
			return((INTBIG)retur);
		case VFLOAT:
		case VDOUBLE:
			return(castint((float)eatof(name)));
		case VNODEINST:
			cindex = myatoi(name);
			if (cindex >= 0 && cindex < io_txtindata.nodeinstcount)
				return((INTBIG)io_txtindata.nodelist[cindex]);
			io_txtindata.nodeinsterror++;
			return(-1);
		case VNODEPROTO:
			cindex = myatoi(name);
			if (cindex == -1) return(-1);

			/* see if there is a ":" in the type */
			for(out = name; *out != 0; out++) if (*out == ':') break;
			if (*out == 0)
			{
				cindex = eatoi(name);
				if (cindex == -1) return(-1);
				return((INTBIG)io_txtindata.nodeprotolist[cindex]);
			}

			/* parse primitive nodeproto name */
			np = getnodeproto(name);
			if (np == NONODEPROTO)
				longjmp(io_filerror, LONGJMPUKNNOPROTO);
			if (np->primindex == 0)
				longjmp(io_filerror, LONGJMPUKNNOPROTO);
			return((INTBIG)np);
		case VPORTARCINST:
			io_txtindata.portarcinsterror++;   break;
		case VPORTEXPINST:
			io_txtindata.portexpinsterror++;   break;
		case VPORTPROTO:
			cindex = myatoi(name);
			if (cindex >= 0 && cindex < io_txtindata.portprotocount)
				return((INTBIG)io_txtindata.portprotolist[cindex]);
			io_txtindata.portprotoerror++;
			return(-1);
		case VARCINST:
			cindex = myatoi(name);
			if (cindex >= 0 && cindex < io_txtindata.arcinstcount)
				return((INTBIG)io_txtindata.arclist[cindex]);
			io_txtindata.arcinsterror++;
			return(-1);
		case VARCPROTO:
			return((INTBIG)getarcproto(name));
		case VGEOM:
			io_txtindata.geomerror++;   break;
		case VLIBRARY:
			io_txtindata.libraryerror++;   break;
		case VTECHNOLOGY:
			return((INTBIG)io_gettechnology(name));
		case VTOOL:
			return((INTBIG)gettool(name));
		case VRTNODE:
			io_txtindata.rtnodeerror++;   break;
	}
	return(-1);
}

/*
 * get the number of cells in this library (keyword "cellcount")
 */
void io_libcc(void)
{
	REGISTER INTBIG i;

	io_txtindata.varpos = INVLIBRARY;

	io_txtindata.cellcount = eatoi(io_keyword);
	if (io_txtindata.cellcount == 0)
	{
		io_txtindata.curlib->firstnodeproto = NONODEPROTO;
		io_txtindata.curlib->tailnodeproto = NONODEPROTO;
		io_txtindata.curlib->numnodeprotos = 0;
		return;
	}

	/* allocate a list of node prototypes for this library */
	io_txtindata.nodeprotolist = (NODEPROTO **)emalloc(((sizeof (NODEPROTO *)) * io_txtindata.cellcount),
		el_tempcluster);
	if (io_txtindata.nodeprotolist == 0) longjmp(io_filerror, LONGJMPNOMEM);
	for(i=0; i<io_txtindata.cellcount; i++)
	{
		io_txtindata.nodeprotolist[i] = allocnodeproto(io_txtindata.curlib->cluster);
		if (io_txtindata.nodeprotolist[i] == NONODEPROTO)
			longjmp(io_filerror, LONGJMPNOMEM);
		io_txtindata.nodeprotolist[i]->cellview = el_unknownview;
		io_txtindata.nodeprotolist[i]->newestversion = io_txtindata.nodeprotolist[i];
		io_txtindata.nodeprotolist[i]->primindex = 0;
		io_txtindata.nodeprotolist[i]->firstportproto = NOPORTPROTO;
		io_txtindata.nodeprotolist[i]->firstnodeinst = NONODEINST;
		io_txtindata.nodeprotolist[i]->firstarcinst = NOARCINST;
	}
}

/*
 * get the main cell of this library (keyword "maincell")
 */
void io_libms(void)
{
	io_txtindata.maincell = eatoi(io_keyword);
}

/*
 * get a view (keyword "view")
 */
void io_libvie(void)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len;
	REGISTER VIEW *v;

	pt = io_keyword;
	while (*pt != 0 && *pt != '{') pt++;
	if (*pt != '{') longjmp(io_filerror, LONGJMPMISVIEWABR);
	*pt++ = 0;
	len = estrlen(pt);
	if (pt[len-1] != '}') longjmp(io_filerror, LONGJMPBADVIEWABR);
	pt[len-1] = 0;
	v = getview(io_keyword);
	if (v == NOVIEW)
	{
		v = allocview();
		if (v == NOVIEW) longjmp(io_filerror, LONGJMPNOMEM);
		(void)allocstring(&v->viewname, io_keyword, db_cluster);
		(void)allocstring(&v->sviewname, pt, db_cluster);
		if (namesamen(io_keyword, x_("Schematic-Page-"), 15) == 0)
			v->viewstate |= MULTIPAGEVIEW;
		v->nextview = el_views;
		el_views = v;
	} else
	{
		if (namesame(v->sviewname, pt) != 0)
			longjmp(io_filerror, LONGJMPINCVIEWABR);
	}
	pt[-1] = '{';   pt[len-1] = '}';
}

/******************** CELL PARSING ROUTINES ********************/

/*
 * initialize for a new cell (keyword "***cell")
 */
void io_newcel(void)
{
	REGISTER CHAR *pt;

	io_txtindata.curcellnumber = eatoi(io_keyword);
	for(pt = io_keyword; *pt != 0; pt++) if (*pt == '/') break;
	io_txtindata.curnodeproto = io_txtindata.nodeprotolist[io_txtindata.curcellnumber];
	if (*pt == '/') io_txtindata.curnodeproto->temp1 = eatoi(pt+1); else
		io_txtindata.curnodeproto->temp1 = -1;
	io_txtindata.textlevel = INCELL;
	io_txtindata.varpos = INVNODEPROTO;
}

/*
 * get the name of the current cell (keyword "name")
 */
void io_celnam(void)
{
	REGISTER CHAR *pt;
	REGISTER VIEW *v;
	REGISTER void *infstr;

	if (io_verbose != 0)
	{
		if (io_verbose < 0 && io_txtindata.filelength > 0 && io_inputprogressdialog != 0)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Reading %s"), io_keyword);
			DiaSetTextProgress(io_inputprogressdialog, returninfstr(infstr));
		} else ttyputmsg(_("Reading %s"), io_keyword);
	}
	io_txtindata.curnodeproto->cellview = el_unknownview;
	pt = io_keyword;
	while (*pt != 0 && *pt != '{') pt++;
	if (*pt == '{')
	{
		*pt = 0;
		if (allocstring(&io_txtindata.curnodeproto->protoname, io_keyword,
			io_txtindata.curlib->cluster))
				longjmp(io_filerror, LONGJMPNOMEM);
		pt++;
		pt[estrlen(pt)-1] = 0;
		for(v = el_views; v != NOVIEW; v = v->nextview)
			if (*v->sviewname != 0 && namesame(pt, v->sviewname) == 0) break;
		if (v != NOVIEW) io_txtindata.curnodeproto->cellview = v;
	} else
	{
		if (allocstring(&io_txtindata.curnodeproto->protoname, io_keyword,
			io_txtindata.curlib->cluster))
				longjmp(io_filerror, LONGJMPNOMEM);
	}
	io_txtindata.curnodeproto->lib = io_txtindata.curlib;
}

/*
 * get the version of the current cell (keyword "version")
 */
void io_celver(void)
{
	io_txtindata.curnodeproto->version = eatoi(io_keyword);
	db_insertnodeproto(io_txtindata.curnodeproto);
}

/*
 * get the creation date of the current cell (keyword "creationdate")
 */
void io_celcre(void)
{
	io_txtindata.curnodeproto->creationdate = eatoi(io_keyword);
}

/*
 * get the revision date of the current cell (keyword "revisiondate")
 */
void io_celrev(void)
{
	io_txtindata.curnodeproto->revisiondate = eatoi(io_keyword);
}

/*
 * get the external library file (keyword "externallibrary")
 */
void io_celext(void)
{
	INTBIG len, filetype, filelen;
	REGISTER LIBRARY *elib;
	REGISTER CHAR *libname, *pt;
	CHAR *filename, *libfile, *oldline2, *libfilename, *libfilepath;
	FILE *io;
	REGISTER BOOLEAN failed;
	CHAR *cellname;
	REGISTER NODEPROTO *np, *onp;
	REGISTER NODEINST *ni;
	TXTINPUTDATA savetxtindata;
	REGISTER void *infstr;

	/* get the path to the library file */
	libfile = io_keyword;
	if (libfile[0] == '"')
	{
		libfile++;
		len = estrlen(libfile) - 1;
		if (libfile[len] == '"') libfile[len] = 0;
	}

	/* see if this library is already read in */
	infstr = initinfstr();
	addstringtoinfstr(infstr, skippath(libfile));
	libname = returninfstr(infstr);
	len = estrlen(libname);
	filelen = estrlen(libfile);
	if (len < filelen)
	{
		libfilename = &libfile[filelen-len-1];
		*libfilename++ = 0;
		libfilepath = libfile;
	} else
	{
		libfilename = libfile;
		libfilepath = x_("");
	}

	filetype = io_filetypetlib;
	if (len > 5 && namesame(&libname[len-5], x_(".elib")) == 0)
	{
		libname[len-5] = 0;
		filetype = io_filetypeblib;
	} else
	{
		if (len > 4 && namesame(&libname[len-4], x_(".txt")) == 0) libname[len-4] = 0;
	}
	elib = getlibrary(libname);
	if (elib == NOLIBRARY)
	{
		/* library does not exist: see if file is there */
		io = xopen(libfilename, filetype, truepath(libfilepath), &filename);
		if (io == 0)
		{
			/* try the library area */
			io = xopen(libfilename, filetype, el_libdir, &filename);
		}
		if (io != 0)
		{
			xclose(io);
			ttyputmsg(_("Reading referenced library %s"), libname);
		} else
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Reference library '%s'"), libname);
			pt = fileselect(returninfstr(infstr), filetype, x_(""));
			if (pt != 0)
			{
				estrcpy(libfile, pt);
				filename = libfile;
			}
		}
		elib = newlibrary(libname, filename);
		if (elib == NOLIBRARY) return;

		/* read the external library */
		savetxtindata = io_txtindata;
		if (io_verbose < 0 && io_txtindata.filelength > 0 && io_inputprogressdialog != 0)
		{
			(void)allocstring(&oldline2, DiaGetTextProgress(io_inputprogressdialog), el_tempcluster);
		}

		len = estrlen(libfilename);
		io_libinputrecursivedepth++;
		io_libinputreadmany++;
		if (len > 4 && namesame(&libfilename[len-4], x_(".txt")) == 0)
		{
			/* ends in ".txt", presume text file */
			failed = io_doreadtextlibrary(elib, FALSE);
		} else
		{
			/* all other endings: presume binary file */
			failed = io_doreadbinlibrary(elib, FALSE);
		}
		io_libinputrecursivedepth--;
		if (failed) elib->userbits |= UNWANTEDLIB; else
		{
			/* queue this library for announcement through change control */
			io_queuereadlibraryannouncement(elib);
		}
		io_txtindata = savetxtindata;
		if (io_verbose < 0 && io_txtindata.filelength > 0 && io_inputprogressdialog != 0)
		{
			DiaSetProgress(io_inputprogressdialog, io_txtindata.fileposition, io_txtindata.filelength);
			infstr = initinfstr();
			formatinfstr(infstr, _("Reading library %s"), io_txtindata.curlib->libname);
			DiaSetCaptionProgress(io_inputprogressdialog, returninfstr(infstr));
			DiaSetTextProgress(io_inputprogressdialog, oldline2);
			efree(oldline2);
		}
	}

	/* find this cell in the external library */
	cellname = io_txtindata.curnodeproto->protoname;
	for(np = elib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (namesame(np->protoname, cellname) != 0) continue;
		if (np->cellview != io_txtindata.curnodeproto->cellview) continue;
		if (np->version != io_txtindata.curnodeproto->version) continue;
		break;
	}
	if (np == NONODEPROTO)
	{
		/* cell not found in library: issue warning */
		infstr = initinfstr();
		addstringtoinfstr(infstr, cellname);
		if (io_txtindata.curnodeproto->cellview != el_unknownview)
		{
			addtoinfstr(infstr, '{');
			addstringtoinfstr(infstr, io_txtindata.curnodeproto->cellview->sviewname);
			addtoinfstr(infstr, '}');
		}
		ttyputerr(_("Cannot find cell %s in library %s..creating dummy version"),
			returninfstr(infstr), elib->libname);
	} else
	{
		/* cell found: make sure it is valid */
		if (np->revisiondate != io_txtindata.curnodeproto->revisiondate ||
			np->lowx != io_txtindata.curnodeproto->lowx ||
			np->highx != io_txtindata.curnodeproto->highx ||
			np->lowy != io_txtindata.curnodeproto->lowy ||
			np->highy != io_txtindata.curnodeproto->highy)
		{
			ttyputerr(_("Warning: cell %s in library %s has been modified"),
				describenodeproto(np), elib->libname);
			np = NONODEPROTO;
		}
	}
	if (np != NONODEPROTO)
	{
		/* get rid of existing cell and plug in the external reference */
		onp = io_txtindata.nodeprotolist[io_txtindata.curcellnumber];
		db_retractnodeproto(onp);
		freenodeproto(onp);
		io_txtindata.nodeprotolist[io_txtindata.curcellnumber] = np;
	} else
	{
		/* rename the cell */
		np = io_txtindata.curnodeproto;
		infstr = initinfstr();
		formatinfstr(infstr, x_("%sFROM%s"), cellname, elib->libname);
		db_retractnodeproto(np);

		cellname = returninfstr(infstr);
		(void)allocstring(&np->protoname, cellname, io_txtindata.curlib->cluster);
		db_insertnodeproto(np);

		/* create an artwork "Crossed box" to define the cell size */
		ni = allocnodeinst(io_txtindata.curlib->cluster);
		ni->proto = art_crossedboxprim;
		ni->parent = np;
		ni->nextnodeinst = np->firstnodeinst;
		np->firstnodeinst = ni;
		ni->lowx = np->lowx;   ni->highx = np->highx;
		ni->lowy = np->lowy;   ni->highy = np->highy;
		ni->geom = allocgeom(io_txtindata.curlib->cluster);
		ni->geom->entryisnode = TRUE;   ni->geom->entryaddr.ni = ni;
		linkgeom(ni->geom, np);
	}
}

/*
 * get the boundary of the current cell
 */
void io_cellx(void)
{
	io_txtindata.curnodeproto->lowx = eatoi(io_keyword);
}

void io_celhx(void)
{
	io_txtindata.curnodeproto->highx = eatoi(io_keyword);
}

void io_celly(void)
{
	io_txtindata.curnodeproto->lowy = eatoi(io_keyword);
}

void io_celhy(void)
{
	io_txtindata.curnodeproto->highy = eatoi(io_keyword);
}

/*
 * get the default technology for objects in this cell (keyword "technology")
 */
void io_tech(void)
{
	REGISTER TECHNOLOGY *tech;

	tech = io_gettechnology(io_keyword);
	if (tech == NOTECHNOLOGY) longjmp(io_filerror, LONGJMPUKNTECH);
	io_txtindata.curtech = tech;
}

/*
 * get the tool dirty word for the current cell (keyword "aadirty")
 */
void io_celaad(void)
{
	io_txtindata.curnodeproto->adirty = eatoi(io_keyword);
	io_txtindata.bitcount = 0;
}

/*
 * get tool information for current cell (keyword "bits")
 */
void io_celbit(void)
{
	if (io_txtindata.bitcount == 0) io_txtindata.curnodeproto->userbits = eatoi(io_keyword);
	io_txtindata.bitcount++;
}

/*
 * get tool information for current cell (keyword "userbits")
 */
void io_celusb(void)
{
	io_txtindata.curnodeproto->userbits = eatoi(io_keyword);
}

/*
 * get tool information for current cell (keyword "netnum")
 */
void io_celnet(void) {}

/*
 * get the number of node instances in the current cell (keyword "nodes")
 */
void io_celnoc(void)
{
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	REGISTER GEOM **geomlist;

	io_txtindata.nodeinstcount = eatoi(io_keyword);
	if (io_txtindata.nodeinstcount == 0) return;
	io_txtindata.nodelist = (NODEINST **)emalloc(((sizeof (NODEINST *)) * io_txtindata.nodeinstcount),
		el_tempcluster);
	if (io_txtindata.nodelist == 0) longjmp(io_filerror, LONGJMPNOMEM);
	geomlist = (GEOM **)emalloc(((sizeof (GEOM *)) * io_txtindata.nodeinstcount), el_tempcluster);
	if (geomlist == 0) longjmp(io_filerror, LONGJMPNOMEM);
	for(i=0; i<io_txtindata.nodeinstcount; i++)
	{
		ni = allocnodeinst(io_txtindata.curlib->cluster);
		geomlist[i] = allocgeom(io_txtindata.curlib->cluster);
		if (ni == NONODEINST || geomlist[i] == NOGEOM)
			longjmp(io_filerror, LONGJMPNOMEM);
		io_txtindata.nodelist[i] = ni;
		ni->parent = io_txtindata.curnodeproto;
		ni->firstportarcinst = NOPORTARCINST;
		ni->firstportexpinst = NOPORTEXPINST;
		ni->geom = geomlist[i];

		/* compute linked list of nodes in this cell */
		if (io_txtindata.curnodeproto->firstnodeinst != NONODEINST)
			io_txtindata.curnodeproto->firstnodeinst->prevnodeinst = ni;
		ni->nextnodeinst = io_txtindata.curnodeproto->firstnodeinst;
		ni->prevnodeinst = NONODEINST;
		io_txtindata.curnodeproto->firstnodeinst = ni;
	}
	efree((CHAR *)geomlist);
}

/*
 * get the number of arc instances in the current cell (keyword "arcs")
 */
void io_celarc(void)
{
	REGISTER INTBIG i;
	REGISTER ARCINST *ai;
	REGISTER GEOM **geomlist;

	io_txtindata.arcinstcount = eatoi(io_keyword);
	if (io_txtindata.arcinstcount == 0) return;
	io_txtindata.arclist = (ARCINST **)emalloc(((sizeof (ARCINST *)) * io_txtindata.arcinstcount),
		el_tempcluster);
	if (io_txtindata.arclist == 0) longjmp(io_filerror, LONGJMPNOMEM);
	geomlist = (GEOM **)emalloc(((sizeof (GEOM *)) * io_txtindata.arcinstcount), el_tempcluster);
	if (geomlist == 0) longjmp(io_filerror, LONGJMPNOMEM);
	for(i=0; i<io_txtindata.arcinstcount; i++)
	{
		ai = allocarcinst(io_txtindata.curlib->cluster);
		geomlist[i] = allocgeom(io_txtindata.curlib->cluster);
		if (ai == NOARCINST || geomlist[i] == NOGEOM)
			longjmp(io_filerror, LONGJMPNOMEM);
		io_txtindata.arclist[i] = ai;
		ai->parent = io_txtindata.curnodeproto;
		ai->geom = geomlist[i];

		/* compute linked list of arcs in this cell */
		if (io_txtindata.curnodeproto->firstarcinst != NOARCINST)
			io_txtindata.curnodeproto->firstarcinst->prevarcinst = ai;
		ai->nextarcinst = io_txtindata.curnodeproto->firstarcinst;
		ai->prevarcinst = NOARCINST;
		io_txtindata.curnodeproto->firstarcinst = ai;
	}
	efree((CHAR *)geomlist);
}

/*
 * get the number of port prototypes in the current cell (keyword "porttypes")
 */
void io_celptc(void)
{
	REGISTER INTBIG i;

	io_txtindata.portprotocount = eatoi(io_keyword);
	io_txtindata.curnodeproto->numportprotos = io_txtindata.portprotocount;
	if (io_txtindata.portprotocount == 0) return;
	io_txtindata.portprotolist = (PORTPROTO **)emalloc(((sizeof (PORTPROTO *)) *
		io_txtindata.portprotocount), el_tempcluster);
	if (io_txtindata.portprotolist == 0) longjmp(io_filerror, LONGJMPNOMEM);
	for(i=0; i<io_txtindata.portprotocount; i++)
	{
		io_txtindata.portprotolist[i] = allocportproto(io_txtindata.curlib->cluster);
		if (io_txtindata.portprotolist[i] == NOPORTPROTO)
			longjmp(io_filerror, LONGJMPNOMEM);
		io_txtindata.portprotolist[i]->parent = io_txtindata.curnodeproto;
	}

	/* link the portprotos */
	io_txtindata.curnodeproto->firstportproto = io_txtindata.portprotolist[0];
	for(i=1; i<io_txtindata.portprotocount; i++)
		io_txtindata.portprotolist[i-1]->nextportproto = io_txtindata.portprotolist[i];
	io_txtindata.portprotolist[io_txtindata.portprotocount-1]->nextportproto = NOPORTPROTO;
}

/*
 * close the current cell (keyword "celldone")
 */
void io_celdon(void)
{
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER GEOM *geom;

	/* verify cell name */
	if (namesame(io_keyword, io_txtindata.curnodeproto->protoname) != 0)
		ttyputmsg(_("Warning: cell '%s' wants to be '%s'"),
			io_txtindata.curnodeproto->protoname, io_keyword);

	/* silly hack: convert arcinst->end->portarcinst pointers */
	for(i=0; i<io_txtindata.nodeinstcount; i++)
	{
		ni = io_txtindata.nodelist[i];
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			ai = pi->conarcinst;
			if (ai->end[0].nodeinst != ni)
			{
				ai->end[1].portarcinst = pi;
				continue;
			}
			if (ai->end[1].nodeinst != ni)
			{
				ai->end[0].portarcinst = pi;
				continue;
			}
			if ((PORTPROTO *)ai->end[0].portarcinst == pi->proto) ai->end[0].portarcinst = pi;
			if ((PORTPROTO *)ai->end[1].portarcinst == pi->proto) ai->end[1].portarcinst = pi;
		}
	}

	/* create geometry structure in the cell */
	if (geomstructure(io_txtindata.curnodeproto))
		longjmp(io_filerror, LONGJMPNOMEM);

	/* fill geometry module for each nodeinst */
	for(i=0; i<io_txtindata.nodeinstcount; i++)
	{
		ni = io_txtindata.nodelist[i];
		geom = ni->geom;
		geom->entryisnode = TRUE;  geom->entryaddr.ni = ni;
		linkgeom(geom, io_txtindata.curnodeproto);
	}

	/* fill geometry modules for each arcinst */
	for(i=0; i<io_txtindata.arcinstcount; i++)
	{
		ai = io_txtindata.arclist[i];
		(void)setshrinkvalue(ai, FALSE);
		geom = ai->geom;
		geom->entryisnode = FALSE;  geom->entryaddr.ai = ai;
		linkgeom(geom, io_txtindata.curnodeproto);
	}

	/* free the lists of objects in this cell */
	if (io_txtindata.nodeinstcount != 0) efree((CHAR *)io_txtindata.nodelist);
	if (io_txtindata.portprotocount != 0) efree((CHAR *)io_txtindata.portprotolist);
	if (io_txtindata.arcinstcount != 0) efree((CHAR *)io_txtindata.arclist);
}

/******************** NODE INSTANCE PARSING ROUTINES ********************/

/*
 * initialize for a new node instance (keyword "**node")
 */
void io_newno(void)
{
	io_txtindata.curnodeinst = io_txtindata.nodelist[eatoi(io_keyword)];
	io_txtindata.textlevel = INNODEINST;
	io_txtindata.varpos = INVNODEINST;
}

/*
 * get the type of the current nodeinst (keyword "type")
 */
void io_nodtyp(void)
{
	REGISTER NODEPROTO *np;
	REGISTER TECHNOLOGY *tech;
	REGISTER CHAR *pt, *line;
	CHAR orig[50];
	static INTBIG orignodenamekey = 0;

	line = io_keyword;
	if (*line == '[')
	{
		io_txtindata.curnodeinst->proto = io_txtindata.nodeprotolist[eatoi(&line[1])];
	} else
	{
		for(pt = line; *pt != 0; pt++) if (*pt == ':') break;
		if (*pt == ':')
		{
			*pt = 0;
			tech = io_gettechnology(line);
			if (tech == NOTECHNOLOGY) longjmp(io_filerror, LONGJMPUKNTECH);
			*pt++ = ':';
			line = pt;
		} else tech = io_txtindata.curtech;
		if (io_txtindata.curtech == NOTECHNOLOGY) io_txtindata.curtech = tech;
		for(np = tech->firstnodeproto; np != NONODEPROTO;
			np = np->nextnodeproto) if (namesame(np->protoname, line) == 0)
		{
			io_txtindata.curnodeinst->proto = np;
			return;
		}

		/* convert "Active-Node" to "P-Active-Node" (MOSIS CMOS) */
		if (estrcmp(line, x_("Active-Node")) == 0)
		{
			for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (estrcmp(np->protoname, x_("P-Active-Node")) == 0)
			{
				io_txtindata.curnodeinst->proto = np;
				return;
			}
		}

		/* convert "message" and "cell-center" nodes */
		io_txtindata.curnodeinst->proto = io_convertoldprimitives(tech, line);
		if (io_txtindata.curnodeinst->proto == gen_invispinprim)
		{
			(void)estrcpy(orig, x_("artwork:"));
			(void)estrcat(orig, line);
			if (orignodenamekey == 0) orignodenamekey = makekey(x_("NODE_original_name"));
			nextchangequiet();
			(void)setvalkey((INTBIG)io_txtindata.curnodeinst, VNODEINST,
				orignodenamekey, (INTBIG)orig, VSTRING|VDONTSAVE);
		}
		if (io_txtindata.curnodeinst->proto != NONODEPROTO) return;

		longjmp(io_filerror, LONGJMPUKNNOPROTO);
	}
}

/*
 * get the bounding box information for the current node instance
 */
void io_nodlx(void)
{
	io_txtindata.curnodeinst->lowx = eatoi(io_keyword);
}

void io_nodhx(void)
{
	io_txtindata.curnodeinst->highx = eatoi(io_keyword);
}

void io_nodly(void)
{
	io_txtindata.curnodeinst->lowy = eatoi(io_keyword);
}

void io_nodhy(void)
{
	io_txtindata.curnodeinst->highy = eatoi(io_keyword);
}

/*
 * get the instance name of the current node instance (keyword "name")
 */
void io_nodnam(void)
{
	nextchangequiet();
	(void)setvalkey((INTBIG)io_txtindata.curnodeinst, VNODEINST, el_node_name_key,
		(INTBIG)io_keyword, VSTRING|VDISPLAY);
}

/*
 * get the text descriptor of the current node instance (keyword "descript")
 */
void io_noddes(void)
{
	REGISTER CHAR *pt;

	io_txtindata.curnodeinst->textdescript[0] = eatoi(io_keyword);
	for(pt = io_keyword; *pt != 0 && *pt != '/'; pt++) ;
	if (*pt == 0)
	{
		io_txtindata.curnodeinst->textdescript[1] = 0;
	} else
	{
		io_txtindata.curnodeinst->textdescript[1] = eatoi(pt+1);
	}
}

/*
 * get the rotation for the current nodeinst (keyword "rotation");
 */
void io_nodrot(void)
{
	io_txtindata.curnodeinst->rotation = eatoi(io_keyword);
}

/*
 * get the transposition for the current nodeinst (keyword "transpose")
 */
void io_nodtra(void)
{
	REGISTER INTSML tvalue;

	tvalue = eatoi(io_keyword);

	/* in 7.01 and higher, allow mirror bits */
	if (io_txtindata.emajor > 7 ||
		(io_txtindata.emajor == 7 && io_txtindata.eminor >= 1))
	{
		/* new version: allow mirror bits */
		io_txtindata.curnodeinst->transpose = 0;
		if ((tvalue&1) != 0)
		{
			/* the old-style transpose bit */
			io_txtindata.curnodeinst->transpose = 1;
		} else
		{
			/* check for new style mirror bits */
			if ((tvalue&2) != 0)
			{
				if ((tvalue&4) != 0)
				{
					/* mirror in X and Y */
					io_txtindata.curnodeinst->rotation = (io_txtindata.curnodeinst->rotation + 1800) % 3600;
				} else
				{
					/* mirror in X */
					io_txtindata.curnodeinst->rotation = (io_txtindata.curnodeinst->rotation + 900) % 3600;
					io_txtindata.curnodeinst->transpose = 1;
				}
			} else if ((tvalue&4) != 0)
			{
				/* mirror in Y */
				io_txtindata.curnodeinst->rotation = (io_txtindata.curnodeinst->rotation + 2700) % 3600;
				io_txtindata.curnodeinst->transpose = 1;
			}
		}
	} else
	{
		/* old version: just treat it as a transpose */
		io_txtindata.curnodeinst->transpose = tvalue;
	}
}

/*
 * get the tool seen bits for the current nodeinst (keyword "aseen")
 */
void io_nodkse(void)
{
	io_txtindata.bitcount = 0;
}

/*
 * get the port count for the current nodeinst (keyword "ports")
 */
void io_nodpoc(void)
{
	io_txtindata.curportcount = eatoi(io_keyword);
}

/*
 * get tool information for current nodeinst (keyword "bits")
 */
void io_nodbit(void)
{
	if (io_txtindata.bitcount == 0) io_txtindata.curnodeinst->userbits = eatoi(io_keyword);
	io_txtindata.bitcount++;
}

/*
 * get tool information for current nodeinst (keyword "userbits")
 */
void io_nodusb(void)
{
	io_txtindata.curnodeinst->userbits = eatoi(io_keyword);
}

/*
 * initialize for a new portinst on the current nodeinst (keyword "*port")
 */
void io_newpor(void)
{
	REGISTER INTBIG i, cindex;
	REGISTER PORTPROTO *pp;

	if (io_txtindata.version[0] == '1')
	{
		/* version 1 files used an index here */
		cindex = eatoi(io_keyword);

		/* dividing is a heuristic used to decypher version 1 files */
		for(pp = io_txtindata.curnodeinst->proto->firstportproto, i=0; pp != NOPORTPROTO;
			pp = pp->nextportproto) i++;
		if (i < io_txtindata.curportcount)
		{
			for(;;)
			{
				cindex /= 3;
				if (cindex < i) break;
			}
		}

		for(io_txtindata.curportproto = io_txtindata.curnodeinst->proto->firstportproto, i=0;
			io_txtindata.curportproto != NOPORTPROTO;
				io_txtindata.curportproto = io_txtindata.curportproto->nextportproto, i++)
					if (i == cindex) break;
	} else
	{
		/* version 2 and later use port prototype names */
		io_txtindata.curportproto = io_getport(io_keyword, io_txtindata.curnodeinst->proto);
	}
	io_txtindata.textlevel = INPOR;
}

/*
 * get an arc connection for the current nodeinst (keyword "arc")
 */
void io_porarc(void)
{
	REGISTER PORTARCINST *pi;

	pi = allocportarcinst(io_txtindata.curlib->cluster);
	if (pi == NOPORTARCINST) longjmp(io_filerror, LONGJMPNOMEM);
	pi->proto = io_txtindata.curportproto;
	db_addportarcinst(io_txtindata.curnodeinst, pi);
	pi->conarcinst = io_txtindata.arclist[eatoi(io_keyword)];
	io_txtindata.varpos = INVPORTARCINST;
	io_txtindata.varaddr = (INTBIG)pi;
}

/*
 * get an export site for the current nodeinst (keyword "exported")
 */
void io_porexp(void)
{
	REGISTER PORTEXPINST *pe;

	pe = allocportexpinst(io_txtindata.curlib->cluster);
	if (pe == NOPORTEXPINST) longjmp(io_filerror, LONGJMPNOMEM);
	pe->proto = io_txtindata.curportproto;
	db_addportexpinst(io_txtindata.curnodeinst, pe);
	pe->exportproto = io_txtindata.portprotolist[eatoi(io_keyword)];
	io_txtindata.varpos = INVPORTEXPINST;
	io_txtindata.varaddr = (INTBIG)pe;
}

/******************** ARC INSTANCE PARSING ROUTINES ********************/

/*
 * initialize for a new arc instance (keyword "**arc")
 */
void io_newar(void)
{
	io_txtindata.curarcinst = io_txtindata.arclist[eatoi(io_keyword)];
	io_txtindata.textlevel = INARCINST;
	io_txtindata.varpos = INVARCINST;
}

/*
 * get the type of the current arc instance (keyword "type")
 */
void io_arctyp(void)
{
	REGISTER ARCPROTO *ap;
	REGISTER CHAR *pt, *line;
	REGISTER TECHNOLOGY *tech;

	line = io_keyword;
	for(pt = line; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		tech = io_gettechnology(line);
		if (tech == NOTECHNOLOGY) longjmp(io_filerror, LONGJMPUKNTECH);
		*pt++ = ':';
		line = pt;
	} else tech = io_txtindata.curtech;
	if (io_txtindata.curtech == NOTECHNOLOGY) io_txtindata.curtech = tech;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if (namesame(line, ap->protoname) == 0)
	{
		io_txtindata.curarcinst->proto = ap;
		return;
	}

	/* convert old Artwork names */
	if (tech == art_tech)
	{
		if (estrcmp(line, x_("Dash-1")) == 0)
		{
			io_txtindata.curarcinst->proto = art_dottedarc;
			return;
		}
		if (estrcmp(line, x_("Dash-2")) == 0)
		{
			io_txtindata.curarcinst->proto = art_dashedarc;
			return;
		}
		if (estrcmp(line, x_("Dash-3")) == 0)
		{
			io_txtindata.curarcinst->proto = art_thickerarc;
			return;
		}
	}

	/* special hack: try the generic technology if name is not found */
	for(ap = gen_tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if (namesame(line, ap->protoname) == 0)
	{
		io_txtindata.curarcinst->proto = ap;
		return;
	}

	longjmp(io_filerror, LONGJMPINVARCPROTO);
}

/*
 * get the instance name of the current arc instance (keyword "name")
 */
void io_arcnam(void)
{
	nextchangequiet();
	(void)setvalkey((INTBIG)io_txtindata.curarcinst, VARCINST, el_arc_name_key,
		(INTBIG)io_keyword, VSTRING|VDISPLAY);
}

/*
 * get the width of the current arc instance (keyword "width")
 */
void io_arcwid(void)
{
	io_txtindata.curarcinst->width = eatoi(io_keyword);
}

/*
 * get the length of the current arc instance (keyword "length")
 */
void io_arclen(void)
{
	io_txtindata.curarcinst->length = eatoi(io_keyword);
}

/*
 * get the signals information of the current arc instance (keyword "signals")
 */
void io_arcsig(void) {}

/*
 * initialize for an end of the current arcinst (keyword "*end")
 */
void io_newend(void)
{
	io_txtindata.curarcend = eatoi(io_keyword);
	io_txtindata.textlevel = INARCEND;
}

/*
 * get the node at the current end of the current arcinst (keyword "node")
 */
void io_endnod(void)
{
	io_txtindata.curarcinst->end[io_txtindata.curarcend].nodeinst = io_txtindata.nodelist[eatoi(io_keyword)];
}

/*
 * get the porttype at the current end of current arcinst (keyword "nodeport")
 */
void io_endpt(void)
{
	REGISTER PORTPROTO *pp;

	pp = io_getport(io_keyword,io_txtindata.curarcinst->end[io_txtindata.curarcend].nodeinst->proto);
	if (pp == NOPORTPROTO) longjmp(io_filerror, LONGJMPUKNPORPROTO);
	io_txtindata.curarcinst->end[io_txtindata.curarcend].portarcinst = (PORTARCINST *)pp;
}

/*
 * get the coordinates of the current end of the current arcinst
 */
void io_endxp(void)
{
	io_txtindata.curarcinst->end[io_txtindata.curarcend].xpos = eatoi(io_keyword);
}

void io_endyp(void)
{
	io_txtindata.curarcinst->end[io_txtindata.curarcend].ypos = eatoi(io_keyword);
}

/*
 * get the tool information for the current arcinst (keyword "aseen")
 */
void io_arckse(void)
{
	io_txtindata.bitcount = 0;
}

/*
 * get tool information for current arcinst (keyword "bits")
 */
void io_arcbit(void)
{
	if (io_txtindata.bitcount == 0) io_txtindata.curarcinst->userbits = eatoi(io_keyword);
	io_txtindata.bitcount++;
}

/*
 * get tool information for current arcinst (keyword "userbits")
 */
void io_arcusb(void)
{
	io_txtindata.curarcinst->userbits = eatoi(io_keyword);
}

/*
 * get tool information for current arcinst (keyword "netnum")
 */
void io_arcnet(void) {}

/******************** PORT PROTOTYPE PARSING ROUTINES ********************/

/*
 * initialize for a new port prototype (keyword "**porttype")
 */
void io_newpt(void)
{
	io_txtindata.curportproto = io_txtindata.portprotolist[eatoi(io_keyword)];
	io_txtindata.textlevel = INPORTPROTO;
	io_txtindata.varpos = INVPORTPROTO;
}

/*
 * get the name for the current port prototype (keyword "name")
 */
void io_ptnam(void)
{
	if (allocstring(&io_txtindata.curportproto->protoname, io_keyword, io_txtindata.curlib->cluster))
		longjmp(io_filerror, LONGJMPNOMEM);
}

/*
 * get the text descriptor for the current port prototype (keyword "descript")
 */
void io_ptdes(void)
{
	REGISTER CHAR *pt;

	io_txtindata.curportproto->textdescript[0] = eatoi(io_keyword);
	for(pt = io_keyword; *pt != 0 && *pt != '/'; pt++) ;
	if (*pt == 0)
	{
		io_txtindata.curportproto->textdescript[1] = 0;
	} else
	{
		io_txtindata.curportproto->textdescript[1] = eatoi(pt+1);
	}
}

/*
 * get the sub-nodeinst for the current port prototype (keyword "subnode")
 */
void io_ptsno(void)
{
	io_txtindata.curportproto->subnodeinst = io_txtindata.nodelist[eatoi(io_keyword)];
}

/*
 * get the sub-portproto for the current port prototype (keyword "subport")
 */
void io_ptspt(void)
{
	REGISTER PORTPROTO *pp;

	pp = io_getport(io_keyword, io_txtindata.curportproto->subnodeinst->proto);
	if (pp == NOPORTPROTO) longjmp(io_filerror, LONGJMPUKNPORPROTO);
	io_txtindata.curportproto->subportproto = pp;
}

/*
 * get the tool seen for the current port prototype (keyword "aseen")
 */
void io_ptkse(void)
{
	io_txtindata.bitcount = 0;
}

/*
 * get the tool data for the current port prototype (keyword "bits")
 */
void io_ptbit(void)
{
	if (io_txtindata.bitcount == 0) io_txtindata.curportproto->userbits = eatoi(io_keyword);
	io_txtindata.bitcount++;
}

/*
 * get the tool data for the current port prototype (keyword "userbits")
 */
void io_ptusb(void)
{
	io_txtindata.curportproto->userbits = eatoi(io_keyword);
}

/*
 * get the tool data for the current port prototype (keyword "netnum")
 */
void io_ptnet(void) {}

/*
 * routine to convert the technology name in "line" to a technology.
 * also handles conversion of the old technology name "logic"
 */
TECHNOLOGY *io_gettechnology(CHAR *line)
{
	REGISTER TECHNOLOGY *tech;

	tech = NOTECHNOLOGY;
	if (io_txtindata.convertmosiscmostechnologies != 0)
	{
		if (namesame(line, x_("mocmossub")) == 0) tech = gettechnology(x_("mocmos")); else
			if (namesame(line, x_("mocmos")) == 0) tech = gettechnology(x_("mocmosold"));
	}
	if (tech == NOTECHNOLOGY) tech = gettechnology(line);
	if (tech != NOTECHNOLOGY) return(tech);
	if (namesame(line, x_("logic")) == 0) tech = sch_tech;
	return(tech);
}

/*
 * Routine to free all memory associated with this module.
 */
void io_freetextinmemory(void)
{
	if (io_maxinputline != 0)
		efree(io_keyword);
}
