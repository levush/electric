/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iobinaryi.c
 * Input/output tool: binary format input
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
#include "tech.h"
#include "tecgen.h"
#include "tecart.h"
#include "tecschem.h"
#include "tecmocmos.h"
#include "tecmocmossub.h"
#include "edialogs.h"
#include "usr.h"

#define REPORTINC   2000			/* bytes between graphical updates */

#define LONGJMPABORTED 1			/* for "Aborted" */
#define LONGJMPEOF     2			/* for "Premature end of file" */

typedef struct
{
	BOOLEAN      toolbitsmessed;	/* 0: tool order is correct */
	INTBIG       aabcount;			/* number of toolbits for old files */
	INTBIG       bytecount;			/* position within input file */
	CHAR       **realname;			/* variable names */
	INTBIG       namecount;			/* number of variable names */
	INTBIG       filelength;		/* length of the file */
	INTBIG       reported;			/* reported progress in file */
	UCHAR1       sizeofsmall;		/* number of bytes in file for an INTSML */
	UCHAR1       sizeofbig;			/* number of bytes in file for an INTBIG */
	UCHAR1       sizeofchar;		/* number of bytes in file for a CHAR */

	/* totals of objects in the file */
	INTBIG       magic;				/* file's magic number */
	INTBIG       emajor, eminor, edetail;	/* library's version information */
	INTBIG       aacount;			/* number of tools */
	INTBIG       techcount;			/* number of technologies */
	INTBIG       nodepprotoindex;	/* number of primitive node prototypes */
	INTBIG       portpprotoindex;	/* number of primitive port prototypes */
	INTBIG       arcprotoindex;		/* number of arc prototypes */
	INTBIG       nodeprotoindex;	/* number of cells */
	INTBIG       nodeindex;			/* number of node instances */
	INTBIG       portprotoindex;	/* index of port prototypes */
	INTBIG       portprotolimit;	/* total number of port prototypes */
	INTBIG       arcindex;			/* number of arc instances */
	INTBIG       geomindex;			/* number of geometry modules */
	INTBIG       cellindex;			/* number of cells */
	INTBIG       curnodeproto;		/* current cell */

	/* input error message arrays */
	CHAR       **techerror;			/* name of unknown technologies */
	CHAR       **toolerror;			/* name of unknown tools */
	BOOLEAN     *nodepprotoerror;	/* flag for unknown prim. NODEPROTOs */
	CHAR       **nodepprotoorig;	/* name of unknown prim. NODEPROTOs */
	INTBIG      *nodepprototech;	/* tech. assoc. with prim. NODEPROTOs */
	CHAR       **portpprotoerror;	/* name of unknown PORTPROTOs */
	CHAR       **arcprotoerror;		/* name of unknown ARCPROTOs */
	INTBIG       swap_bytes;		/* are the bytes swapped? 1 if so */
	BOOLEAN      converttextdescriptors;	/* convert textdescript fields */
	BOOLEAN      alwaystextdescriptors;	/* always have textdescript fields */
	INTBIG       swapped_floats;	/* number of swappped floating-point variables */
	INTBIG       swapped_doubles;	/* number of swappped double-precision variables */
	INTBIG       clipped_integers;	/* number of clipped integers */
	FILE        *filein;			/* channel for input */
	INTBIG      *newnames;			/* for adding new names to global namespace */
	jmp_buf      filerror;			/* nonlocal jump when I/O fails */
	NODEINST   **nodelist;			/* list of nodes for readin */
	INTBIG      *nodecount;			/* number of nodeinsts in each cell */
	ARCINST    **arclist;			/* list of arcs for readin */
	INTBIG      *arccount;			/* number of arcinsts in each cell */
	NODEPROTO  **nodeprotolist;		/* list of cells for readin */
	FAKECELL   **fakecelllist;		/* list of fakecells for readin */
	TECHNOLOGY **techlist;			/* list of technologies for readin */
	INTBIG      *toollist;			/* list of tools for readin */
	PORTPROTO  **portprotolist;		/* list of portprotos for readin */
	INTBIG      *portcount;			/* number of portprotos in each cell */
	PORTPROTO  **portpprotolist;	/* list of primitive portprotos for readin */
	NODEPROTO  **nodepprotolist;	/* list of primitive nodeprotos for readin */
	ARCPROTO   **arcprotolist;		/* list of arcprotos for readin */
} BININPUTDATA;

static BININPUTDATA io_binindata;				/* all data pertaining to reading a file */
static CHAR1       *io_tempstring;				/* for reading temporary strings */
static INTBIG       io_tempstringlength = 0;	/* length of temporary string */
static CHAR         io_mainlibdirectory[500];	/* root directory where binary file lives */
       void        *io_inputprogressdialog;		/* for showing input progress */

/* prototypes for local routines */
static CHAR       *io_doreadlibrary(LIBRARY*);
static BOOLEAN     io_readnodeproto(NODEPROTO*, LIBRARY*);
static BOOLEAN     io_readnodeinst(NODEINST*);
static void        io_readgeom(BOOLEAN*, INTBIG*);
static BOOLEAN     io_readarcinst(ARCINST*);
static BOOLEAN     io_readnamespace(void);
static void        io_ignorevariables(void);
static INTBIG      io_readvariables(INTBIG, INTBIG);
static INTBIG      io_getinvar(INTBIG*, INTBIG);
static INTBIG      io_arrangetoolbits(INTBIG);
static INTBIG      io_convertnodeproto(NODEPROTO**, CHAR**);
static BOOLEAN     io_convertportproto(PORTPROTO**, BOOLEAN);
static void        io_convertarcproto(ARCPROTO**);
static ARCPROTO   *io_getarcprotolist(INTBIG);
static PORTPROTO  *io_getportpprotolist(INTBIG);
static NODEPROTO  *io_getnodepprotolist(INTBIG);
static TECHNOLOGY *io_gettechlist(INTBIG);
static void        io_getin(void *, INTBIG, INTBIG, BOOLEAN);
static CHAR       *io_getstring(CLUSTER*);
static CHAR       *io_gettempstring(void);
static void        io_getinbig(void *data);
static void        io_getinubig(void *data);
static void        io_readexternalnodeproto(LIBRARY *lib, INTBIG i);
static void        io_ensureallports(LIBRARY *lib);
static void        io_binfindallports(LIBRARY *lib, PORTPROTO *pp, INTBIG *lx, INTBIG *hx,
						INTBIG *ly, INTBIG *hy, BOOLEAN *first, ARCPROTO **onlyarc, XARRAY trans);
static void        io_fixexternalvariables(VARIABLE *firstvar, INTSML numvar);

/*
 * Routine to free all memory associated with this module.
 */
void io_freebininmemory(void)
{
	if (io_tempstringlength != 0) efree((CHAR *)io_tempstring);
}

BOOLEAN io_readbinlibrary(LIBRARY *lib)
{
	REGISTER BOOLEAN ret;
	REGISTER INTBIG len, filelen;
	REGISTER LIBRARY *olib, *nextlib;

	/* mark all libraries as "not being read", but "wanted" */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
	{
		olib->temp1 = 0;
		olib->userbits &= ~UNWANTEDLIB;
	}

	/* remember the path to this first library */
	estrcpy(io_mainlibdirectory, truepath(lib->libfile));
	filelen = estrlen(io_mainlibdirectory);
	len = estrlen(skippath(io_mainlibdirectory));
	if (len < filelen)
	{
		io_mainlibdirectory[filelen-len-1] = 0;
	} else
	{
		io_mainlibdirectory[0] = 0;
	}


	ret = io_doreadbinlibrary(lib, TRUE);
	if (ret == 0 && (lib->userbits&HIDDENLIBRARY) == 0)
	{
		ttyputmsg(_("Read library %s on %s"), lib->libname,
			timetostring(getcurrenttime()));
#ifdef ONUNIX
		efprintf(stdout, _("Read library %s on %s\n"), lib->libname,
			timetostring(getcurrenttime()));
#endif
	}

	/* delete unwanted libraries */
	for(olib = el_curlib; olib != NOLIBRARY; olib = nextlib)
	{
		nextlib = olib->nextlibrary;
		if ((olib->userbits&UNWANTEDLIB) == 0) continue;
		killlibrary(olib);
	}
	return(ret);
}

BOOLEAN io_doreadbinlibrary(LIBRARY *lib, BOOLEAN newprogress)
{
	REGISTER CHAR *result;
	CHAR *filename;
	REGISTER void *infstr;

	/* find the library file */
	result = truepath(lib->libfile);

	io_binindata.filein = xopen(result, io_filetypeblib, el_libdir, &filename);
	if (io_binindata.filein == NULL)
	{
		ttyputerr(_("File '%s' not found"), result);
		return(TRUE);
	}

	/* update the library file name if the true location is different */
	if (estrcmp(filename, lib->libfile) != 0)
		(void)reallocstring(&lib->libfile, filename, lib->cluster);

	/* prepare status dialog */
	if (io_verbose < 0)
	{
		io_binindata.filelength = filesize(io_binindata.filein);
		if (io_binindata.filelength > 0)
		{
			if (newprogress)
			{
				io_inputprogressdialog = DiaInitProgress(x_(""), _("Reading library"));
				if (io_inputprogressdialog == 0)
				{
					xclose(io_binindata.filein);
					return(TRUE);
				}
			}
			DiaSetProgress(io_inputprogressdialog, 0, io_binindata.filelength);
			infstr = initinfstr();
			formatinfstr(infstr, _("Reading library %s"), lib->libname);
			DiaSetCaptionProgress(io_inputprogressdialog, returninfstr(infstr));
			DiaSetTextProgress(io_inputprogressdialog, _("Initializing..."));
		}
	} else io_binindata.filelength = 0;
	io_binindata.reported = 0;
	io_binindata.bytecount = 0;

	/* assume regular format */
	io_binindata.swap_bytes = 0;
	io_binindata.swapped_floats = 0;
	io_binindata.swapped_doubles = 0;
	io_binindata.clipped_integers = 0;

	/* mark that the library is being read */
	lib->temp1 = 1;

	/* now read the file */
	result = io_doreadlibrary(lib);

	/* mark that the library is not being read */
	lib->temp1 = 0;

	/* clean up */
	xclose(io_binindata.filein);
	if (io_verbose < 0 && io_binindata.filelength > 0 && newprogress && io_inputprogressdialog != 0)
	{
		DiaDoneProgress(io_inputprogressdialog);
		io_inputprogressdialog = 0;
		io_binindata.filelength = 0;
	}
	if (io_binindata.swapped_floats != 0)
		ttyputverbose(M_("Read %ld swapped float variables; may be incorrect"),
			io_binindata.swapped_floats);
	if (io_binindata.swapped_doubles != 0)
		ttyputverbose(M_("Read %ld swapped double variables; may be incorrect"),
			io_binindata.swapped_doubles);
	if (io_binindata.clipped_integers != 0)
		ttyputverbose(M_("Read %ld clipped integers; may be incorrect"),
			io_binindata.clipped_integers);
	if (*result == 0) return(FALSE);
	ttyputerr(_("Error reading library %s: %s"), lib->libname, result);
	return(TRUE);
}

CHAR *io_doreadlibrary(LIBRARY *lib)
{
	REGISTER INTBIG te;
	REGISTER BOOLEAN imosconv, showerror;
	REGISTER BOOLEAN *geomtype;
	REGISTER INTBIG i, thisone, *geommoreup, top, advise, errorcode,
		bot, look, nodeindex, arcindex, geomindex, oldunit, convertmosiscmostechnologies;
	INTBIG num, den, j, count, cou, arcinstpos, nodeinstpos, portprotopos;
	REGISTER CHAR *name, *sname, *thecellname;
	REGISTER NODEPROTO *np, *onp, *lastnp, *newestversion;
	REGISTER NODEINST *ni;
	GEOM *geom;
	REGISTER ARCPROTO *ap;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER TECHNOLOGY *tech;
	REGISTER PORTEXPINST **portexpinstlist;
	REGISTER PORTARCINST **portarcinstlist;
	REGISTER TOOL *tool;
	CHAR *version;
	UCHAR1 chmagic[4];
	REGISTER VIEW *v;
	REGISTER LIBRARY *savelib, *olib;
	REGISTER void *infstr;

	errorcode = setjmp(io_binindata.filerror);
	if (errorcode != 0)
	{
		if (errorcode == LONGJMPABORTED) return(_("Aborted"));
		if (errorcode == LONGJMPEOF) return(_("Premature end of file"));
		return(_("Unknown error"));
	}

	/* read magic number */
	io_getin(&io_binindata.magic, 4, SIZEOFINTBIG, TRUE);
	if (io_binindata.magic != MAGIC1 && io_binindata.magic != MAGIC2 &&
		io_binindata.magic != MAGIC3 && io_binindata.magic != MAGIC4 &&
		io_binindata.magic != MAGIC5 && io_binindata.magic != MAGIC6 &&
		io_binindata.magic != MAGIC7 && io_binindata.magic != MAGIC8 &&
		io_binindata.magic != MAGIC9 && io_binindata.magic != MAGIC10 &&
		io_binindata.magic != MAGIC11 && io_binindata.magic != MAGIC12 &&
		io_binindata.magic != MAGIC13)
	{
		/* try swapping the bytes */
		chmagic[0] = (UCHAR1)(io_binindata.magic & 0xFF);
		chmagic[1] = (UCHAR1)((io_binindata.magic >> 8) & 0xFF);
		chmagic[2] = (UCHAR1)((io_binindata.magic >> 16) & 0xFF);
		chmagic[3] = (UCHAR1)((io_binindata.magic >> 24) & 0xFF);
		io_binindata.magic = (chmagic[0] << 24) | (chmagic[1] << 16) |
			(chmagic[2] << 8) | chmagic[3];
		if (io_binindata.magic != MAGIC1 && io_binindata.magic != MAGIC2 &&
			io_binindata.magic != MAGIC3 && io_binindata.magic != MAGIC4 &&
			io_binindata.magic != MAGIC5 && io_binindata.magic != MAGIC6 &&
			io_binindata.magic != MAGIC7 && io_binindata.magic != MAGIC8 &&
			io_binindata.magic != MAGIC9 && io_binindata.magic != MAGIC10 &&
			io_binindata.magic != MAGIC11 && io_binindata.magic != MAGIC12 &&
			io_binindata.magic != MAGIC13)
				return(_("Bad file format")); else
		{
			io_binindata.swap_bytes = 1;
			ttyputverbose(M_("File has swapped bytes in the header. Will attempt to read it."));
		}
	}
	if (io_verbose > 0) switch (io_binindata.magic)
	{
		case MAGIC1:  ttyputmsg(M_("This library is 12 versions old"));  break;
		case MAGIC2:  ttyputmsg(M_("This library is 11 versions old"));  break;
		case MAGIC3:  ttyputmsg(M_("This library is 10 versions old"));   break;
		case MAGIC4:  ttyputmsg(M_("This library is 9 versions old"));   break;
		case MAGIC5:  ttyputmsg(M_("This library is 8 versions old"));   break;
		case MAGIC6:  ttyputmsg(M_("This library is 7 versions old"));   break;
		case MAGIC7:  ttyputmsg(M_("This library is 6 versions old"));   break;
		case MAGIC8:  ttyputmsg(M_("This library is 5 versions old"));   break;
		case MAGIC9:  ttyputmsg(M_("This library is 4 versions old"));   break;
		case MAGIC10: ttyputmsg(M_("This library is 3 version old"));    break;
		case MAGIC11: ttyputmsg(M_("This library is 2 version old"));    break;
		case MAGIC12: ttyputmsg(M_("This library is 1 version old"));    break;
	}

	/* determine size of "big" and "small" integers on disk */
	if (io_binindata.magic <= MAGIC10)
	{
		io_getin(&io_binindata.sizeofsmall, 1, 1, FALSE);
		io_getin(&io_binindata.sizeofbig, 1, 1, FALSE);
	} else
	{
		io_binindata.sizeofsmall = 2;
		io_binindata.sizeofbig = 4;
	}
	if (io_binindata.magic <= MAGIC11)
	{
		io_getin(&io_binindata.sizeofchar, 1, 1, FALSE);
	} else
	{
		io_binindata.sizeofchar = 1;
	}
	if (io_binindata.sizeofsmall > SIZEOFINTSML || io_binindata.sizeofbig > SIZEOFINTBIG)
		ttyputmsg(_("Warning: file has larger integers than memory: clipping may occur."));
	if (io_binindata.sizeofchar > SIZEOFCHAR)
		ttyputmsg(_("Warning: file has unicode text: not all characters are preserved."));

	/* get count of objects in the file */
	io_getinbig(&io_binindata.aacount);
	io_getinbig(&io_binindata.techcount);
	io_getinbig(&io_binindata.nodepprotoindex);
	io_getinbig(&io_binindata.portpprotoindex);
	io_getinbig(&io_binindata.arcprotoindex);
	io_getinbig(&io_binindata.nodeprotoindex);
	io_getinbig(&io_binindata.nodeindex);
	io_getinbig(&io_binindata.portprotoindex);
	io_binindata.portprotolimit = io_binindata.portprotoindex;
	io_getinbig(&io_binindata.arcindex);
	io_getinbig(&io_binindata.geomindex);
	if (io_binindata.magic <= MAGIC9 && io_binindata.magic >= MAGIC11)
	{
		/* versions 9 through 11 stored a "cell count" */
		io_getinbig(&io_binindata.cellindex);
	} else
	{
		io_binindata.cellindex = io_binindata.nodeprotoindex;
	}
	io_getinbig(&io_binindata.curnodeproto);
	if (io_verbose > 0)
	{
		ttyputmsg(M_("Reading %ld tools, %ld technologies"), io_binindata.aacount,
			io_binindata.techcount);
		ttyputmsg(M_("        %ld prim nodes, %ld prim ports, %ld arc protos"),
			io_binindata.nodepprotoindex, io_binindata.portpprotoindex,
			io_binindata.arcprotoindex);
		ttyputmsg(M_("        %ld cells, %ld cells, %ld ports, %ld nodes, %ld arcs"),
			io_binindata.cellindex, io_binindata.nodeprotoindex,
			io_binindata.portprotoindex, io_binindata.nodeindex,
			io_binindata.arcindex);
	}

	/* get the Electric version (version 8 and later) */
	if (io_binindata.magic <= MAGIC8) version = io_getstring(el_tempcluster); else
		(void)allocstring(&version, x_("3.35"), el_tempcluster);
	parseelectricversion(version, &io_binindata.emajor, &io_binindata.eminor, &io_binindata.edetail);

	/* for versions before 6.03q, convert MOSIS CMOS technology names */
	convertmosiscmostechnologies = 0;
	if (io_binindata.emajor < 6 ||
		(io_binindata.emajor == 6 && io_binindata.eminor < 3) ||
		(io_binindata.emajor == 6 && io_binindata.eminor == 3 && io_binindata.edetail < 17))
	{
		if ((asktech(mocmossub_tech, x_("get-state"))&MOCMOSSUBNOCONV) == 0)
			convertmosiscmostechnologies = 1;
	}

	/* for versions before 6.04c, convert text descriptor values */
	io_binindata.converttextdescriptors = FALSE;
	if (io_binindata.emajor < 6 ||
		(io_binindata.emajor == 6 && io_binindata.eminor < 4) ||
		(io_binindata.emajor == 6 && io_binindata.eminor == 4 && io_binindata.edetail < 3))
	{
		io_binindata.converttextdescriptors = TRUE;
	}

	/* for versions 6.05x and later, always have text descriptor values */
	io_binindata.alwaystextdescriptors = TRUE;
	if (io_binindata.emajor < 6 ||
		(io_binindata.emajor == 6 && io_binindata.eminor < 5) ||
		(io_binindata.emajor == 6 && io_binindata.eminor == 5 && io_binindata.edetail < 24))
	{
		io_binindata.alwaystextdescriptors = FALSE;
	}
	

#ifdef REPORTCONVERSION
	ttyputmsg(x_("Library is version %s (%ld.%ld.%ld)"), version, io_binindata.emajor,
		io_binindata.eminor, io_binindata.edetail);
	if (convertmosiscmostechnologies != 0)
		ttyputmsg(x_("   Converting MOSIS CMOS technologies (mocmossub => mocmos)"));
#endif

	/* get the newly created views (version 9 and later) */
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
	if (io_binindata.magic <= MAGIC9)
	{
		io_getinbig(&j);
		for(i=0; i<j; i++)
		{
			name = io_getstring(db_cluster);
			sname = io_getstring(db_cluster);
			v = getview(name);
			if (v == NOVIEW)
			{
				v = allocview();
				if (v == NOVIEW) return(_("No memory"));
				v->viewname = name;
				v->sviewname = sname;
				if (namesamen(name, x_("Schematic-Page-"), 15) == 0)
					v->viewstate |= MULTIPAGEVIEW;
				v->nextview = el_views;
				el_views = v;
			} else
			{
				efree(name);
				efree(sname);
			}
			v->temp1 = i + 1;
		}
	}

	/* get the number of toolbits to ignore */
	if (io_binindata.magic <= MAGIC3 && io_binindata.magic >= MAGIC6)
	{
		/* versions 3, 4, 5, and 6 find this in the file */
		io_getinbig(&io_binindata.aabcount);
	} else
	{
		/* versions 1 and 2 compute this (versions 7 and later ignore it) */
		io_binindata.aabcount = io_binindata.aacount;
	}

	/* erase the current database */
	if (io_verbose > 0) ttyputmsg(M_("Erasing old library"));
	eraselibrary(lib);

	/* allocate pointers */
	io_binindata.nodelist = (NODEINST **)emalloc(((sizeof (NODEINST *)) * io_binindata.nodeindex), el_tempcluster);
	if (io_binindata.nodelist == 0) return(_("No memory"));
	io_binindata.nodecount = emalloc((SIZEOFINTBIG * io_binindata.nodeprotoindex), el_tempcluster);
	if (io_binindata.nodecount == 0) return(_("No memory"));
	io_binindata.nodeprotolist = (NODEPROTO **)emalloc(((sizeof (NODEPROTO *)) * io_binindata.nodeprotoindex), el_tempcluster);
	if (io_binindata.nodeprotolist == 0) return(_("No memory"));
	io_binindata.portprotolist = (PORTPROTO **)emalloc(((sizeof (PORTPROTO *)) * io_binindata.portprotoindex), el_tempcluster);
	if (io_binindata.portprotolist == 0) return(_("No memory"));
	io_binindata.portcount = emalloc((SIZEOFINTBIG * io_binindata.nodeprotoindex), el_tempcluster);
	if (io_binindata.portcount == 0) return(_("No memory"));
	io_binindata.portpprotolist = (PORTPROTO **)emalloc(((sizeof (PORTPROTO *)) * io_binindata.portpprotoindex), el_tempcluster);
	if (io_binindata.portpprotolist == 0) return(_("No memory"));
	io_binindata.portpprotoerror = (CHAR **)emalloc(((sizeof (CHAR *)) * io_binindata.portpprotoindex), el_tempcluster);
	if (io_binindata.portpprotoerror == 0) return(_("No memory"));
	portexpinstlist = (PORTEXPINST **)emalloc(((sizeof (PORTEXPINST *)) * io_binindata.portprotoindex), el_tempcluster);
	if (portexpinstlist == 0) return(_("No memory"));
	portarcinstlist = (PORTARCINST **)emalloc(((sizeof (PORTARCINST *)) * io_binindata.arcindex * 2), el_tempcluster);
	if (portarcinstlist == 0) return(_("No memory"));
	io_binindata.arclist = (ARCINST **)emalloc(((sizeof (ARCINST *)) * io_binindata.arcindex), el_tempcluster);
	if (io_binindata.arclist == 0) return(_("No memory"));
	io_binindata.arccount = emalloc((SIZEOFINTBIG * io_binindata.nodeprotoindex), el_tempcluster);
	if (io_binindata.arccount == 0) return(_("No memory"));
	io_binindata.nodepprotolist = (NODEPROTO **)emalloc(((sizeof (NODEPROTO *)) * io_binindata.nodepprotoindex), el_tempcluster);
	if (io_binindata.nodepprotolist == 0) return(_("No memory"));
	io_binindata.nodepprototech = emalloc((SIZEOFINTBIG * io_binindata.nodepprotoindex), el_tempcluster);
	if (io_binindata.nodepprototech == 0) return(_("No memory"));
	io_binindata.nodepprotoerror = (BOOLEAN *)emalloc(((sizeof (BOOLEAN)) * io_binindata.nodepprotoindex), el_tempcluster);
	if (io_binindata.nodepprotoerror == 0) return(_("No memory"));
	io_binindata.nodepprotoorig = (CHAR **)emalloc(((sizeof (CHAR *)) * io_binindata.nodepprotoindex), el_tempcluster);
	if (io_binindata.nodepprotoorig == 0) return(_("No memory"));
	io_binindata.arcprotolist = (ARCPROTO **)emalloc(((sizeof (ARCPROTO *)) * io_binindata.arcprotoindex), el_tempcluster);
	if (io_binindata.arcprotolist == 0) return(_("No memory"));
	io_binindata.arcprotoerror = (CHAR **)emalloc(((sizeof (CHAR *)) * io_binindata.arcprotoindex), el_tempcluster);
	if (io_binindata.arcprotoerror == 0) return(_("No memory"));
	io_binindata.techlist = (TECHNOLOGY **)emalloc(((sizeof (TECHNOLOGY *)) * io_binindata.techcount), el_tempcluster);
	if (io_binindata.techlist == 0) return(_("No memory"));
	io_binindata.techerror = (CHAR **)emalloc(((sizeof (CHAR *)) * io_binindata.techcount), el_tempcluster);
	if (io_binindata.techerror == 0) return(_("No memory"));
	io_binindata.toollist = emalloc((SIZEOFINTBIG * io_binindata.aacount), el_tempcluster);
	if (io_binindata.toollist == 0) return(_("No memory"));
	io_binindata.toolerror = (CHAR **)emalloc(((sizeof (CHAR *)) * io_binindata.aacount), el_tempcluster);
	if (io_binindata.toolerror == 0) return(_("No memory"));

	/* versions 9 to 11 allocate cell pointers */
	if (io_binindata.magic <= MAGIC9 && io_binindata.magic >= MAGIC11)
	{
		io_binindata.fakecelllist = (FAKECELL **)emalloc(((sizeof (FAKECELL *)) * io_binindata.cellindex), el_tempcluster);
		if (io_binindata.fakecelllist == 0) return(_("No memory"));
	}

	/* versions 4 and earlier allocate geometric pointers */
	if (io_binindata.magic > MAGIC5)
	{
		geomtype = (BOOLEAN *)emalloc(((sizeof (BOOLEAN)) * io_binindata.geomindex), el_tempcluster);
		if (geomtype == 0) return(_("No memory"));
		geommoreup = emalloc((SIZEOFINTBIG * io_binindata.geomindex), el_tempcluster);
		if (geommoreup == 0) return(_("No memory"));
		if (geomtype == 0 || geommoreup == 0) return(_("No memory"));
	}

	/* get number of arcinsts and nodeinsts in each cell */
	if (io_binindata.magic != MAGIC1)
	{
		/* versions 2 and later find this in the file */
		nodeinstpos = arcinstpos = portprotopos = 0;
		for(i=0; i<io_binindata.nodeprotoindex; i++)
		{
			io_getinbig(&io_binindata.arccount[i]);
			io_getinbig(&io_binindata.nodecount[i]);
			io_getinbig(&io_binindata.portcount[i]);
			if (io_binindata.arccount[i] >= 0 || io_binindata.nodecount[i] >= 0)
			{
				arcinstpos += io_binindata.arccount[i];
				nodeinstpos += io_binindata.nodecount[i];
			}
			portprotopos += io_binindata.portcount[i];
		}

		/* verify that the number of node instances is equal to the total in the file */
		if (nodeinstpos != io_binindata.nodeindex)
		{
			ttyputerr(_("Error: cells have %ld nodes but library has %ld"),
				nodeinstpos, io_binindata.nodeindex);
			return(_("Bad file"));
		}
		if (arcinstpos != io_binindata.arcindex)
		{
			ttyputerr(_("Error: cells have %ld arcs but library has %ld"),
				arcinstpos, io_binindata.arcindex);
			return(_("Bad file"));
		}
		if (portprotopos != io_binindata.portprotoindex)
		{
			ttyputerr(_("Error: cells have %ld ports but library has %ld"),
				portprotopos, io_binindata.portprotoindex);
			return(_("Bad file"));
		}
	} else
	{
		/* version 1 computes this information */
		io_binindata.arccount[0] = io_binindata.arcindex;
		io_binindata.nodecount[0] = io_binindata.nodeindex;
		io_binindata.portcount[0] = io_binindata.portprotoindex;
		for(i=1; i<io_binindata.nodeprotoindex; i++)
			io_binindata.arccount[i] = io_binindata.nodecount[i] = io_binindata.portcount[i] = 0;
	}

	/* allocate all cells in the library */
	/* versions 9 to 11 allocate fakecells now */
	if (io_binindata.magic <= MAGIC9 && io_binindata.magic >= MAGIC11)
	{
		for(i=0; i<io_binindata.cellindex; i++)
		{
			io_binindata.fakecelllist[i] = (FAKECELL *)emalloc(sizeof (FAKECELL), io_tool->cluster);
			if (io_binindata.fakecelllist[i] == NOFAKECELL) return(_("No memory"));
		}
	}

	/* allocate all cells in the library */
	lib->numnodeprotos = 0;
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		if (io_binindata.arccount[i] < 0 && io_binindata.nodecount[i] < 0)
		{
			/* this cell is from an external library */
			io_binindata.nodeprotolist[i] = 0;
		} else
		{
			io_binindata.nodeprotolist[i] = allocnodeproto(lib->cluster);
			if (io_binindata.nodeprotolist[i] == NONODEPROTO) return(_("No memory"));
			io_binindata.nodeprotolist[i]->cellview = el_unknownview;
			io_binindata.nodeprotolist[i]->newestversion = io_binindata.nodeprotolist[i];
			lib->numnodeprotos++;
		}
	}

	/* allocate the nodes, arcs, and ports in each cell */
	nodeinstpos = arcinstpos = portprotopos = 0;
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		np = io_binindata.nodeprotolist[i];
		if (np == 0)
		{
			/* for external references, clear the port proto list */
			for(j=0; j<io_binindata.portcount[i]; j++)
				io_binindata.portprotolist[portprotopos+j] = NOPORTPROTO;
			portprotopos += io_binindata.portcount[i];
			continue;
		}

		/* allocate node instances in this cell */
		for(j=0; j<io_binindata.nodecount[i]; j++)
		{
			io_binindata.nodelist[nodeinstpos+j] = allocnodeinst(lib->cluster);
			if (io_binindata.nodelist[nodeinstpos+j] == NONODEINST) return(_("No memory"));
		}
		if (io_binindata.nodecount[i] == 0) np->firstnodeinst = NONODEINST; else
			np->firstnodeinst = io_binindata.nodelist[nodeinstpos];
		for(j=0; j<io_binindata.nodecount[i]; j++)
		{
			ni = io_binindata.nodelist[j+nodeinstpos];
			geom = allocgeom(lib->cluster);
			if (geom == NOGEOM) return(_("No memory"));
			ni->geom = geom;

			/* compute linked list of nodes in this cell */
			if (j == 0) ni->prevnodeinst = NONODEINST; else
				ni->prevnodeinst = io_binindata.nodelist[j+nodeinstpos-1];
			if (j+1 == io_binindata.nodecount[i]) ni->nextnodeinst = NONODEINST; else
				ni->nextnodeinst = io_binindata.nodelist[j+nodeinstpos+1];
		}
		nodeinstpos += io_binindata.nodecount[i];

		/* allocate port prototypes in this cell */
		for(j=0; j<io_binindata.portcount[i]; j++)
		{
			thisone = j + portprotopos;
			io_binindata.portprotolist[thisone] = allocportproto(lib->cluster);
			if (io_binindata.portprotolist[thisone] == NOPORTPROTO) return(_("No memory"));
			portexpinstlist[thisone] = allocportexpinst(lib->cluster);
			if (portexpinstlist[thisone] == NOPORTEXPINST) return(_("No memory"));
			io_binindata.portprotolist[thisone]->subportexpinst = portexpinstlist[thisone];
		}
		portprotopos += io_binindata.portcount[i];

		/* allocate arc instances and port arc instances in this cell */
		for(j=0; j<io_binindata.arccount[i]; j++)
		{
			io_binindata.arclist[arcinstpos+j] = allocarcinst(lib->cluster);
			if (io_binindata.arclist[arcinstpos+j] == NOARCINST) return(_("No memory"));
		}
		for(j=0; j<io_binindata.arccount[i]*2; j++)
		{
			portarcinstlist[arcinstpos*2+j] = allocportarcinst(lib->cluster);
			if (portarcinstlist[arcinstpos*2+j] == NOPORTARCINST) return(_("No memory"));
		}
		if (io_binindata.arccount[i] == 0) np->firstarcinst = NOARCINST; else
			np->firstarcinst = io_binindata.arclist[arcinstpos];
		for(j=0; j<io_binindata.arccount[i]; j++)
		{
			thisone = j + arcinstpos;
			ai = io_binindata.arclist[thisone];
			geom = allocgeom(lib->cluster);
			if (geom == NOGEOM) return(_("No memory"));
			ai->geom = geom;
			ai->end[0].portarcinst = portarcinstlist[thisone*2];
			ai->end[1].portarcinst = portarcinstlist[thisone*2+1];

			/* compute linked list of arcs in this cell */
			if (j == 0) ai->prevarcinst = NOARCINST; else
				ai->prevarcinst = io_binindata.arclist[j+arcinstpos-1];
			if (j+1 == io_binindata.arccount[i]) ai->nextarcinst = NOARCINST; else
				ai->nextarcinst = io_binindata.arclist[j+arcinstpos+1];
		}
		arcinstpos += io_binindata.arccount[i];

		if (io_verbose > 0)
			ttyputmsg(M_("Allocated %ld arcs, %ld nodes, %ld ports for cell %ld"),
				io_binindata.arccount[i], io_binindata.nodecount[i], io_binindata.portcount[i], i);
	}

	efree((CHAR *)portarcinstlist);
	efree((CHAR *)portexpinstlist);

	/* setup pointers for technologies and primitives */
	io_binindata.nodepprotoindex = 0;
	io_binindata.portpprotoindex = 0;
	io_binindata.arcprotoindex = 0;
	for(te=0; te<io_binindata.techcount; te++)
	{
		/* associate the technology name with the true technology */
		name = io_gettempstring();
		if (name == 0) return(_("No memory"));
		tech = NOTECHNOLOGY;
		if (convertmosiscmostechnologies != 0)
		{
			if (namesame(name, x_("mocmossub")) == 0) tech = gettechnology(x_("mocmos")); else
				if (namesame(name, x_("mocmos")) == 0) tech = gettechnology(x_("mocmosold"));
		}
		if (tech == NOTECHNOLOGY) tech = gettechnology(name);

		/* conversion code for old technologies */
		imosconv = FALSE;
		if (tech == NOTECHNOLOGY)
		{
			if (namesame(name, x_("imos")) == 0)
			{
				tech = gettechnology(x_("mocmos"));
				if (tech != NOTECHNOLOGY) imosconv = TRUE;
			} else if (namesame(name, x_("logic")) == 0) tech = sch_tech;
		}

		if (tech == NOTECHNOLOGY)
		{
			if (namesame(name, x_("epic8c")) == 0 || namesame(name, x_("epic7c")) == 0)
				tech = gettechnology(x_("epic7s"));
		}

		if (tech == NOTECHNOLOGY)
		{
			tech = el_technologies;
			(void)allocstring(&io_binindata.techerror[te], name, el_tempcluster);
		} else io_binindata.techerror[te] = 0;
		io_binindata.techlist[te] = tech;

		/* get the number of primitive node prototypes */
		io_getinbig(&count);
		for(j=0; j<count; j++)
		{
			io_binindata.nodepprotoorig[io_binindata.nodepprotoindex] = 0;
			io_binindata.nodepprotoerror[io_binindata.nodepprotoindex] = FALSE;
			name = io_gettempstring();
			if (name == 0) return(_("No memory"));
			if (imosconv) name += 6;
			for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (estrcmp(np->protoname, name) == 0) break;
			if (np == NONODEPROTO)
			{
				/* automatic conversion of "Active-Node" in to "P-Active-Node" (MOSIS CMOS) */
				if (estrcmp(name, x_("Active-Node")) == 0)
				{
					for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
						if (estrcmp(np->protoname, x_("P-Active-Node")) == 0) break;
				}
			}
			if (np == NONODEPROTO)
			{
				if (io_verbose > 0)
					ttyputmsg(M_("No node exactly named %s in technology %s"), name, tech->techname);
				advise = 1;

				/* look for substring name match at start of name */
				i = estrlen(name);
				for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					if (namesamen(np->protoname, name, mini(estrlen(np->protoname), i)) == 0)
						break;

				/* look for substring match at end of name */
				if (np == NONODEPROTO)
				{
					for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					{
						thisone = estrlen(np->protoname);
						if (i >= thisone) continue;
						if (namesame(&np->protoname[thisone-i], name) == 0) break;
					}
				}

				/* special cases: convert "message" and "cell-center" nodes */
				if (np == NONODEPROTO)
				{
					np = io_convertoldprimitives(tech, name);
					if (np != NONODEPROTO) advise = 0;
				}

				/* give up and use first primitive in this technology */
				if (np == NONODEPROTO) np = tech->firstnodeproto;

				/* construct the error message */
				if (advise != 0)
				{
					infstr = initinfstr();
					if (io_binindata.techerror[te] != 0) addstringtoinfstr(infstr, io_binindata.techerror[te]); else
						addstringtoinfstr(infstr, tech->techname);
					addtoinfstr(infstr, ':');
					addstringtoinfstr(infstr, name);
					(void)allocstring(&io_binindata.nodepprotoorig[io_binindata.nodepprotoindex], returninfstr(infstr), el_tempcluster);
					io_binindata.nodepprotoerror[io_binindata.nodepprotoindex] = TRUE;
				}
			}
			io_binindata.nodepprototech[io_binindata.nodepprotoindex] = te;
			io_binindata.nodepprotolist[io_binindata.nodepprotoindex] = np;

			/* get the number of primitive port prototypes */
			io_getinbig(&cou);
			for(i=0; i<cou; i++)
			{
				io_binindata.portpprotoerror[io_binindata.portpprotoindex] = 0;
				name = io_gettempstring();
				if (name == 0) return(_("No memory"));
				pp = getportproto(np, name);

				/* convert special port names */
				if (pp == NOPORTPROTO)
					pp = io_convertoldportname(name, np);

				if (pp == NOPORTPROTO)
				{
					pp = np->firstportproto;
					if (!io_binindata.nodepprotoerror[io_binindata.nodepprotoindex])
					{
						infstr = initinfstr();
						addstringtoinfstr(infstr, name);
						addstringtoinfstr(infstr, _(" on "));
						if (io_binindata.nodepprotoorig[io_binindata.nodepprotoindex] != 0)
							addstringtoinfstr(infstr, io_binindata.nodepprotoorig[io_binindata.nodepprotoindex]); else
						{
							if (io_binindata.techerror[te] != 0)
								addstringtoinfstr(infstr, io_binindata.techerror[te]); else
									addstringtoinfstr(infstr, tech->techname);
							addtoinfstr(infstr, ':');
							addstringtoinfstr(infstr, np->protoname);
						}
						(void)allocstring(&io_binindata.portpprotoerror[io_binindata.portpprotoindex],
							returninfstr(infstr), el_tempcluster);
					}
				}
				io_binindata.portpprotolist[io_binindata.portpprotoindex++] = pp;
			}
			io_binindata.nodepprotoindex++;
		}

		/* get the number of arc prototypes */
		io_getinbig(&count);
		for(j=0; j<count; j++)
		{
			io_binindata.arcprotoerror[io_binindata.arcprotoindex] = 0;
			name = io_gettempstring();
			if (name == 0) return(_("No memory"));
			if (imosconv) name += 6;
			for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
				if (estrcmp(ap->protoname, name) == 0) break;
			if (ap == NOARCPROTO)
			{
				if (tech == art_tech)
				{
					if (estrcmp(name, x_("Dash-1")) == 0) ap = art_dottedarc; else
					if (estrcmp(name, x_("Dash-2")) == 0) ap = art_dashedarc; else
					if (estrcmp(name, x_("Dash-3")) == 0) ap = art_thickerarc;
				}
			}
			if (ap == NOARCPROTO)
			{
				ap = tech->firstarcproto;
				infstr = initinfstr();
				if (io_binindata.techerror[te] != 0)
					addstringtoinfstr(infstr, io_binindata.techerror[te]); else
						addstringtoinfstr(infstr, tech->techname);
				addtoinfstr(infstr, ':');
				addstringtoinfstr(infstr, name);
				(void)allocstring(&io_binindata.arcprotoerror[io_binindata.arcprotoindex], returninfstr(infstr), el_tempcluster);
			}
			io_binindata.arcprotolist[io_binindata.arcprotoindex++] = ap;
		}
	}

	/* setup pointers for tools */
	io_binindata.toolbitsmessed = FALSE;
	for(i=0; i<io_binindata.aacount; i++)
	{
		name = io_gettempstring();
		if (name == 0) return(_("No memory"));
		io_binindata.toolerror[i] = 0;
		for(j=0; j<el_maxtools; j++)
			if (estrcmp(name, el_tools[j].toolname) == 0) break;
		if (j >= el_maxtools)
		{
			(void)allocstring(&io_binindata.toolerror[i], name, el_tempcluster);
			j = -1;
		}
		if (i != j) io_binindata.toolbitsmessed = TRUE;
		io_binindata.toollist[i] = j;
	}
	if (io_binindata.magic <= MAGIC3 && io_binindata.magic >= MAGIC6)
	{
		/* versions 3, 4, 5, and 6 must ignore toolbits associations */
		for(i=0; i<io_binindata.aabcount; i++) (void)io_gettempstring();
	}

	/* get the library userbits */
	if (io_binindata.magic <= MAGIC7)
	{
		/* version 7 and later simply read the relevant data */
		io_getinubig(&lib->userbits);
	} else
	{
		/* version 6 and earlier must sift through the information */
		if (io_binindata.aabcount >= 1) io_getinubig(&lib->userbits);
		for(i=1; i<io_binindata.aabcount; i++) io_getinubig(&j);
	}
	lib->userbits &= ~(LIBCHANGEDMAJOR | LIBCHANGEDMINOR);
	lib->userbits |= READFROMDISK;

	/* set the lambda values in the library */
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		lib->lambda[tech->techindex] = el_curlib->lambda[tech->techindex];
	for(i=0; i<io_binindata.techcount; i++)
	{
		io_getinbig(&j);
		if (io_binindata.techerror[i] != 0) continue;
		tech = io_binindata.techlist[i];

		/* for version 4 or earlier, scale lambda by 20 */
		if (eatoi(version) <= 4) j *= 20;

		/* account for any differences of internal unit in this library */
		oldunit = ((lib->userbits & LIBUNITS) >> LIBUNITSSH) << INTERNALUNITSSH;
		if (oldunit != (el_units&INTERNALUNITS))
		{
			db_getinternalunitscale(&num, &den, el_units, oldunit);
			j = muldiv(j, den, num);
		}

		/* if this is to be the current library, adjust technologies */
		if (lib == el_curlib)
			changetechnologylambda(tech, j);
		lib->lambda[tech->techindex] = j;
	}

	/* read the global namespace */
	io_binindata.realname = 0;
	io_binindata.newnames = 0;
	if (io_readnamespace()) return(_("No memory"));

	/* read the library variables */
	if (io_verbose > 0) ttyputmsg(M_("Reading library variables"));
	if (io_readvariables((INTBIG)lib, VLIBRARY) < 0) return(_("Reading library variables"));

	/* read the tool variables */
	if (io_verbose > 0) ttyputmsg(M_("Reading tool variables"));
	for(i=0; i<io_binindata.aacount; i++)
	{
		j = io_binindata.toollist[i];
		if (j < 0) io_ignorevariables(); else
			if (io_readvariables((INTBIG)&el_tools[j], VTOOL) < 0)
				return(_("Reading tool variables"));
	}

	/* read the technology variables */
	if (io_verbose > 0) ttyputmsg(M_("Reading technology/primitive variables"));
	for(te=0; te<io_binindata.techcount; te++)
	{
		tech = io_binindata.techlist[te];
		j = io_readvariables((INTBIG)tech, VTECHNOLOGY);
		if (j < 0) return(_("Reading technology/primitive variables"));
		if (j > 0) (void)io_gettechlist(te);
	}

	/* read the arcproto variables */
	for(i=0; i<io_binindata.arcprotoindex; i++)
	{
		ap = io_binindata.arcprotolist[i];
		j = io_readvariables((INTBIG)ap, VARCPROTO);
		if (j < 0) return(_("Reading arcproto variables"));
		if (j > 0) (void)io_getarcprotolist(i);
	}

	/* read the primitive nodeproto variables */
	for(i=0; i<io_binindata.nodepprotoindex; i++)
	{
		np = io_binindata.nodepprotolist[i];
		j = io_readvariables((INTBIG)np, VNODEPROTO);
		if (j < 0) return(_("Reading primitive variables"));
		if (j > 0) (void)io_getnodepprotolist(i);
	}

	/* read the primitive portproto variables */
	for(i=0; i<io_binindata.portpprotoindex; i++)
	{
		pp = io_binindata.portpprotolist[i];
		j = io_readvariables((INTBIG)pp, VPORTPROTO);
		if (j < 0) return(_("Reading export variables"));
		if (j > 0) (void)io_getportpprotolist(i);
	}

	/* read the view variables (version 9 and later) */
	if (io_binindata.magic <= MAGIC9)
	{
		io_getinbig(&count);
		for(i=0; i<count; i++)
		{
			io_getinbig(&j);
			for(v = el_views; v != NOVIEW; v = v->nextview)
				if (v->temp1 == j) break;
			if (v == NOVIEW)
			{
				ttyputmsg(_("View index %ld not found"), j);
				io_ignorevariables();
				continue;
			}
			if (io_readvariables((INTBIG)v, VVIEW) < 0)
				return(_("Reading view variables"));
		}
	}

	/* read the cells (version 9 to 11) */
	if (io_binindata.magic <= MAGIC9 && io_binindata.magic >= MAGIC11)
	{
		for(i=0; i<io_binindata.cellindex; i++)
		{
			thecellname = io_getstring(lib->cluster);
			io_ignorevariables();

			allocstring(&io_binindata.fakecelllist[i]->cellname, thecellname, io_tool->cluster);
		}
	}

	/* read the cells */
	io_binindata.portprotoindex = 0;
	lastnp = NONODEPROTO;
	lib->firstnodeproto = NONODEPROTO;
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		np = io_binindata.nodeprotolist[i];
		if (np == 0) continue;
		if (io_readnodeproto(np, lib)) return(_("Reading nodeproto"));
		if (lastnp == NONODEPROTO)
		{
			lib->firstnodeproto = np;
			np->prevnodeproto = NONODEPROTO;
		} else
		{
			np->prevnodeproto = lastnp;
			lastnp->nextnodeproto = np;
		}
		lastnp = np;
		np->nextnodeproto = NONODEPROTO;
	}
	lib->tailnodeproto = lastnp;

	/* add in external cells */
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		np = io_binindata.nodeprotolist[i];
		if (np != 0) continue;
		io_readexternalnodeproto(lib, i);
	}

	/* now that external cells are resolved, fix all variables that may have used them */
	io_fixexternalvariables(lib->firstvar, lib->numvar);
	for(i=0; i<io_binindata.aacount; i++)
	{
		j = io_binindata.toollist[i];
		if (j < 0) continue;
		tool = &el_tools[j];
		io_fixexternalvariables(tool->firstvar, tool->numvar);
	}
	for(te=0; te<io_binindata.techcount; te++)
	{
		tech = io_binindata.techlist[te];
		io_fixexternalvariables(tech->firstvar, tech->numvar);
	}
	for(i=0; i<io_binindata.arcprotoindex; i++)
	{
		ap = io_binindata.arcprotolist[i];
		io_fixexternalvariables(ap->firstvar, ap->numvar);
	}
	for(i=0; i<io_binindata.nodepprotoindex; i++)
	{
		np = io_binindata.nodepprotolist[i];
		io_fixexternalvariables(np->firstvar, np->numvar);
	}
	for(i=0; i<io_binindata.portpprotoindex; i++)
	{
		pp = io_binindata.portpprotolist[i];
		io_fixexternalvariables(pp->firstvar, pp->numvar);
	}
	for(v = el_views; v != NOVIEW; v = v->nextview)
		io_fixexternalvariables(v->firstvar, v->numvar);
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		np = io_binindata.nodeprotolist[i];
		io_fixexternalvariables(np->firstvar, np->numvar);
	}

	/* convert port references in external cells */
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		np = io_binindata.nodeprotolist[i];

		/* ignore this cell if it is in a library being read */
		olib = np->lib;
		showerror = TRUE;
		if (olib != lib && olib->temp1 != 0) showerror = FALSE;

		/* convert its ports */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp2 == 0) continue;
			if (io_convertportproto(&pp->subportproto, showerror))
			{
				if (showerror)
					ttyputmsg(_("  While reading library %s, on cell %s, port %s (lib %s temp1 is %ld)"),
						lib->libname, describenodeproto(np), pp->protoname, np->lib->libname,
							np->lib->temp1);
			}
			pp->temp2 = 0;
		}
	}
	if (io_verbose > 0) ttyputmsg(M_("Done reading cells"));

	/* read the cell contents: arcs and nodes */
	nodeindex = arcindex = geomindex = 0;
	for(i=0; i<io_binindata.nodeprotoindex; i++)
	{
		if (stopping(STOPREASONBINARY)) return(_("Library incomplete"));
		np = io_binindata.nodeprotolist[i];
		if (io_verbose != 0)
		{
			savelib = el_curlib;   el_curlib = lib;
			if (io_verbose < 0 && io_binindata.filelength > 0 && io_inputprogressdialog != 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("Reading %s"), describenodeproto(np));
				DiaSetTextProgress(io_inputprogressdialog, returninfstr(infstr));
			} else ttyputmsg(_("Reading %s"), describenodeproto(np));
			el_curlib = savelib;
		}
		if (io_binindata.magic > MAGIC5)
		{
			/* versions 4 and earlier must read some geometric information */
			j = geomindex;
			io_readgeom(&geomtype[j], &geommoreup[j]);   j++;
			io_readgeom(&geomtype[j], &geommoreup[j]);   j++;
			top = j;   io_readgeom(&geomtype[j], &geommoreup[j]);   j++;
			bot = j;   io_readgeom(&geomtype[j], &geommoreup[j]);   j++;
			for(;;)
			{
				io_readgeom(&geomtype[j], &geommoreup[j]);   j++;
				if (geommoreup[j-1] == top) break;
			}
			geomindex = j;
			for(look = bot; look != top; look = geommoreup[look])
				if (!geomtype[look])
			{
				io_binindata.arclist[arcindex]->parent = np;
				if (io_readarcinst(io_binindata.arclist[arcindex]))
					return(_("Reading arc"));
				arcindex++;
			} else
			{
				io_binindata.nodelist[nodeindex]->parent = np;
				if (io_readnodeinst(io_binindata.nodelist[nodeindex]))
					return(_("Reading node"));
				nodeindex++;
			}
		} else
		{
			/* version 5 and later find the arcs and nodes in linear order */
			for(j=0; j<io_binindata.arccount[i]; j++)
			{
				io_binindata.arclist[arcindex]->parent = np;
				if (io_readarcinst(io_binindata.arclist[arcindex]))
					return(_("Reading arc"));
				arcindex++;
			}
			for(j=0; j<io_binindata.nodecount[i]; j++)
			{
				io_binindata.nodelist[nodeindex]->parent = np;
				if (io_readnodeinst(io_binindata.nodelist[nodeindex]))
					return(_("Reading node"));
				nodeindex++;
			}
		}
	}

	if (io_verbose < 0 && io_binindata.filelength > 0 && io_inputprogressdialog != 0)
	{
		DiaSetProgress(io_inputprogressdialog, 999, 1000);
		DiaSetTextProgress(io_inputprogressdialog, _("Cleaning up..."));
	}

	/* transform indices to pointers */
	if (io_binindata.curnodeproto >= 0 && io_binindata.curnodeproto < io_binindata.nodeprotoindex)
		lib->curnodeproto = io_binindata.nodeprotolist[io_binindata.curnodeproto]; else
			lib->curnodeproto = NONODEPROTO;

	/* set version pointers (this may be slow for libraries with many cells) */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		newestversion = NONODEPROTO;
		for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (onp == np) continue;
			if (onp->cellview != np->cellview) continue;
			if (namesame(onp->protoname, np->protoname) != 0) continue;
			if (onp->version <= np->version) continue;
			if (newestversion != NONODEPROTO && newestversion->version >= onp->version)
				continue;
			newestversion = onp;
		}
		if (newestversion != NONODEPROTO)
		{
			/* cell "np" is an old one */
			np->prevversion = newestversion->prevversion;
			newestversion->prevversion = np;
			np->newestversion = newestversion;
		}
	}

	/* create proper cell lists (version 9 to 11) */
	if (io_binindata.magic <= MAGIC9 && io_binindata.magic >= MAGIC11)
		io_buildcellgrouppointersfromnames(lib);

	/* link up the nodes and arcs to their geometry modules */
	for(i=0; i<io_binindata.nodeindex; i++)
	{
		ni = io_binindata.nodelist[i];
		geom = ni->geom;
		geom->entryisnode = TRUE;  geom->entryaddr.ni = ni;
		linkgeom(geom, ni->parent);
	}
	for(i=0; i<io_binindata.arcindex; i++)
	{
		ai = io_binindata.arclist[i];
		(void)setshrinkvalue(ai, FALSE);
		geom = ai->geom;
		geom->entryisnode = FALSE;  geom->entryaddr.ai = ai;
		linkgeom(geom, ai->parent);
	}

	/* create ports on cells that were not found in other libraries */
	io_ensureallports(lib);

	/* store the version number in the library */
	nextchangequiet();
	(void)setval((INTBIG)lib, VLIBRARY, x_("LIB_former_version"),
		(INTBIG)version, VSTRING|VDONTSAVE);
	efree(version);

	/* create those pointers not on the disk file */
	if (io_verbose < 0 && io_binindata.filelength > 0)
		io_fixnewlib(lib, io_inputprogressdialog); else
			io_fixnewlib(lib, 0);

	/* look for any database changes that did not matter */
	for(te=0; te<io_binindata.techcount; te++)
		if (io_binindata.techerror[te] != 0)
	{
		if (io_verbose > 0)
			ttyputmsg(M_("Unknown technology %s not used so all is well"),
				io_binindata.techerror[te]);
		efree(io_binindata.techerror[te]);
	}
	for(i=0; i<io_binindata.aacount; i++)
		if (io_binindata.toolerror[i] != 0)
	{
		if (io_verbose > 0)
			ttyputmsg(M_("Unknown tool %s not used so all is well"),
				io_binindata.toolerror[i]);
		efree(io_binindata.toolerror[i]);
	}
	for(i=0; i<io_binindata.nodepprotoindex; i++)
		if (io_binindata.nodepprotoorig[i] != 0)
	{
		if (io_verbose > 0 && io_binindata.nodepprotoerror[i])
			ttyputmsg(M_("Unknown node %s not used so all is well"),
				io_binindata.nodepprotoorig[i]);
		efree(io_binindata.nodepprotoorig[i]);
	}
	for(i=0; i<io_binindata.portpprotoindex; i++)
		if (io_binindata.portpprotoerror[i] != 0)
	{
		if (io_verbose > 0) ttyputmsg(M_("Unknown port %s not used so all is well"),
			io_binindata.portpprotoerror[i]);
		efree(io_binindata.portpprotoerror[i]);
	}
	for(i=0; i<io_binindata.arcprotoindex; i++)
		if (io_binindata.arcprotoerror[i] != 0)
	{
		if (io_verbose > 0) ttyputmsg(M_("Unknown arc %s not used so all is well"),
			io_binindata.arcprotoerror[i]);
		efree(io_binindata.arcprotoerror[i]);
	}

	/* warn if the MOSIS CMOS technologies were converted */
	if (convertmosiscmostechnologies != 0)
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

	/* free the memory used here */
	if (io_binindata.realname != 0)
	{
		for(i=0; i<io_binindata.namecount; i++) efree(io_binindata.realname[i]);
		efree((CHAR *)io_binindata.realname);
	}
	if (io_binindata.newnames != 0) efree((CHAR *)io_binindata.newnames);
	if (io_binindata.magic > MAGIC5)
	{
		/* versions 4 and earlier used geometric data */
		efree((CHAR *)geomtype);
		efree((CHAR *)geommoreup);
	}
	efree((CHAR *)io_binindata.nodelist);
	efree((CHAR *)io_binindata.nodecount);
	efree((CHAR *)io_binindata.nodeprotolist);
	efree((CHAR *)io_binindata.arclist);
	efree((CHAR *)io_binindata.arccount);
	efree((CHAR *)io_binindata.nodepprotolist);
	efree((CHAR *)io_binindata.nodepprototech);
	efree((CHAR *)io_binindata.portprotolist);
	efree((CHAR *)io_binindata.portcount);
	efree((CHAR *)io_binindata.portpprotolist);
	efree((CHAR *)io_binindata.arcprotolist);
	efree((CHAR *)io_binindata.techlist);
	efree((CHAR *)io_binindata.toollist);
	efree((CHAR *)io_binindata.techerror);
	efree((CHAR *)io_binindata.toolerror);
	efree((CHAR *)io_binindata.nodepprotoerror);
	efree((CHAR *)io_binindata.nodepprotoorig);
	efree((CHAR *)io_binindata.portpprotoerror);
	efree((CHAR *)io_binindata.arcprotoerror);
	if (io_binindata.magic <= MAGIC9 && io_binindata.magic >= MAGIC11)
		efree((CHAR *)io_binindata.fakecelllist);

	return(x_(""));
}

/******************** COMPONENT INPUT ********************/

/* routine to read node prototype.  returns true upon error */
BOOLEAN io_readnodeproto(NODEPROTO *np, LIBRARY *lib)
{
	INTBIG i, portcount, k;
	REGISTER INTBIG j;
	REGISTER CHAR *cellname;
	REGISTER PORTPROTO *pp, *lpt;
	REGISTER VIEW *v;

	/* read the cell information (version 9 and later) */
	if (io_binindata.magic <= MAGIC9)
	{
		if (io_binindata.magic >= MAGIC11)
		{
			/* only versions 9 to 11 */
			io_getinbig(&k);
			allocstring(&np->protoname, io_binindata.fakecelllist[k]->cellname, lib->cluster);
		} else
		{
			/* version 12 or later */
			np->protoname = io_getstring(db_cluster);
			io_getinbig(&k);
			np->nextcellgrp = io_binindata.nodeprotolist[k];
			io_getinbig(&k);
//			np->nextcont = io_binindata.nodeprotolist[k];
		}
		io_getinbig(&np->cellview);
		for(v = el_views; v != NOVIEW; v = v->nextview)
			if (v->temp1 == (INTBIG)np->cellview) break;
		if (v == NOVIEW) v = el_unknownview;
		np->cellview = v;
		io_getinbig(&i);
		np->version = i;
		io_getinubig(&np->creationdate);
		io_getinubig(&np->revisiondate);

		/* copy cell information to the nodeproto */
		np->lib = lib;
	}

	/* versions 8 and earlier read a cell name */
	if (io_binindata.magic >= MAGIC8)
	{
		cellname = io_getstring(lib->cluster);
		if (cellname == 0) return(TRUE);
		if (io_verbose > 0) ttyputmsg(_("Reading cell %s"), cellname);

		/* copy cell information to the nodeproto */
		np->lib = lib;
		np->protoname = cellname;
	}

	/* set the nodeproto primitive index */
	np->primindex = 0;

	/* read the nodeproto bounding box */
	io_getinbig(&np->lowx);
	io_getinbig(&np->highx);
	io_getinbig(&np->lowy);
	io_getinbig(&np->highy);

	/* reset the first nodeinst index */
	np->firstinst = NONODEINST;

	/* zap the technology field */
	np->tech = NOTECHNOLOGY;

	/* read the library list of nodeproto indices (versions 5 or older) */
	if (io_binindata.magic >= MAGIC5)
	{
		io_getinbig(&np->prevnodeproto);
		if (io_convertnodeproto(&np->prevnodeproto, 0) != 0)
			ttyputmsg(_("...while reading cell %s"), np->protoname);
		io_getinbig(&np->nextnodeproto);
		if (io_convertnodeproto(&np->nextnodeproto, 0) != 0)
			ttyputmsg(_("...while reading cell %s"), np->protoname);
	}

	/* read the number of portprotos on this nodeproto */
	io_getinbig(&portcount);
	np->numportprotos = portcount;

	/* read the portprotos on this nodeproto */
	lpt = NOPORTPROTO;
	for(j=0; j<portcount; j++)
	{
		/* set pointers to portproto */
		pp = io_binindata.portprotolist[io_binindata.portprotoindex++];
		if (lpt == NOPORTPROTO) np->firstportproto = pp; else
			lpt->nextportproto = pp;
		lpt = pp;

		/* set the parent pointer */
		pp->parent = np;

		/* read the connecting subnodeinst for this portproto */
		io_getinbig(&i);
		if (i >= 0 && i < io_binindata.nodeindex)
			pp->subnodeinst = io_binindata.nodelist[i]; else
		{
			ttyputerr(_("Warning: corrupt data on port.  Do a 'Check and Repair Library'"));
			pp->subnodeinst = NONODEINST;
		}			

		/* read the sub-port prototype of the subnodeinst */
		io_getinbig(&i);
		pp->subportproto = (PORTPROTO *)i;
		if (i >= 0 && io_binindata.portprotolist[i] == NOPORTPROTO)
		{
			pp->temp2 = 1;
		} else
		{
			(void)io_convertportproto(&pp->subportproto, TRUE);
			pp->temp2 = 0;
		}

		/* read the portproto name and text descriptor */
		pp->protoname = io_getstring(lib->cluster);
		if (pp->protoname == 0) return(TRUE);
		if (io_binindata.magic <= MAGIC9)
		{
			if (io_binindata.converttextdescriptors)
			{
				/* conversion is done later */
				io_getinubig(&pp->textdescript[0]);
				pp->textdescript[1] = 0;
			} else
			{
				io_getinubig(&pp->textdescript[0]);
				io_getinubig(&pp->textdescript[1]);
			}
		}

		/* ignore the "seen" bits (versions 8 and older) */
		if (io_binindata.magic > MAGIC9) io_getinubig(&k);

		/* read the portproto tool information */
		if (io_binindata.magic <= MAGIC7)
		{
			/* version 7 and later simply read the relevant data */
			io_getinubig(&pp->userbits);

			/* versions 7 and 8 ignore net number */
			if (io_binindata.magic >= MAGIC8) io_getinbig(&k);
		} else
		{
			/* version 6 and earlier must sift through the information */
			if (io_binindata.aabcount >= 1) io_getinubig(&pp->userbits);
			for(i=1; i<io_binindata.aabcount; i++) io_getinubig(&k);
		}

		/* read variable information */
		if (io_readvariables((INTBIG)pp, VPORTPROTO) < 0) return(TRUE);
	}

	/* set final pointer in portproto list */
	if (lpt == NOPORTPROTO) np->firstportproto = NOPORTPROTO; else
		lpt->nextportproto = NOPORTPROTO;

	/* read the cell's geometry modules */
	if (io_binindata.magic > MAGIC5)
	{
		/* versions 4 and older have geometry module pointers (ignore it) */
		io_getinbig(&i);
		io_getinbig(&i);
		io_getinbig(&i);
		io_getinbig(&i);
		io_getinbig(&i);
	}

	/* read tool information */
	io_getinubig(&np->adirty);
	np->adirty = io_arrangetoolbits((INTBIG)np->adirty);
	if (io_binindata.magic <= MAGIC7)
	{
		/* version 7 and later simply read the relevant data */
		io_getinubig(&np->userbits);

		/* versions 7 and 8 ignore net number */
		if (io_binindata.magic >= MAGIC8) io_getinbig(&k);
	} else
	{
		/* version 6 and earlier must sift through the information */
		if (io_binindata.aabcount >= 1) io_getinubig(&np->userbits);
		for(i=1; i<io_binindata.aabcount; i++) io_getinubig(&k);
	}
	/* build the dummy geometric structure for this cell */
	if (geomstructure(np)) return(TRUE);

	/* read variable information */
	if (io_readvariables((INTBIG)np, VNODEPROTO) < 0) return(TRUE);
	return(FALSE);
}

/* routine to read node prototype for external references */
void io_readexternalnodeproto(LIBRARY *lib, INTBIG i)
{
	INTBIG portcount, k, vers, lowx, highx, lowy, highy, len, filelen, filetype;
	UINTBIG creation, revision;
	REGISTER INTBIG j, newcell, index;
	REGISTER LIBRARY *elib;
	REGISTER BOOLEAN failed;
	REGISTER CHAR *protoname, *libname, *pt;
	CHAR *filename, *libfile, *oldline2, *libfilepath, *libfilename,
		*dummycellname, **localportnames, *cellname;
	FILE *io;
	REGISTER PORTPROTO *pp, *lpt;
	REGISTER NODEPROTO *np, *onp, *npdummy;
	REGISTER NODEINST *ni;
	REGISTER VIEW *v;
	REGISTER FAKECELL *fc;
	BININPUTDATA savebinindata;
	REGISTER void *infstr;

	/* read the cell information */
	if (io_binindata.magic >= MAGIC11)
	{
		/* version 11 and earlier */
		io_getinbig(&k);
		fc = io_binindata.fakecelllist[k];
		cellname = fc->cellname;
	} else
	{
		cellname = io_getstring(db_cluster);
		io_getinbig(&k);
		io_getinbig(&k);
	}
	io_getinbig(&k);
	for(v = el_views; v != NOVIEW; v = v->nextview)
		if (v->temp1 == k) break;
	if (v == NOVIEW) v = el_unknownview;
	io_getinbig(&vers);
	io_getinubig(&creation);
	io_getinubig(&revision);

	/* read the nodeproto bounding box */
	io_getinbig(&lowx);
	io_getinbig(&highx);
	io_getinbig(&lowy);
	io_getinbig(&highy);

	/* get the path to the library file */
	libfile = io_getstring(el_tempcluster);
	filelen = estrlen(libfile);

	/* see if this library is already read in */
	infstr = initinfstr();
	addstringtoinfstr(infstr, skippath(libfile));
	libname = returninfstr(infstr);
	len = estrlen(libname);
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

	filetype = io_filetypeblib;
	if (len > 5 && namesame(&libname[len-5], x_(".elib")) == 0)
	{
		libname[len-5] = 0;
	} else if (len > 4 && namesame(&libname[len-4], x_(".txt")) == 0)
	{
		libname[len-4] = 0;
		filetype = io_filetypetlib;
	}
	elib = getlibrary(libname);
	if (elib == NOLIBRARY)
	{
		/* library does not exist: see if file is there */
		io = xopen(libfilename, filetype, io_mainlibdirectory, &filename);
		if (io == 0)
		{
			/* try the path specified in the reference */
			io = xopen(libfilename, filetype, truepath(libfilepath), &filename);
			if (io == 0)
			{
				/* try the library area */
				io = xopen(libfilename, filetype, el_libdir, &filename);
			}
		}
		if (io != 0)
		{
			xclose(io);
			ttyputmsg(_("Reading referenced library %s on %s"), filename,
				timetostring(getcurrenttime()));
#ifdef ONUNIX
			efprintf(stdout, _("Reading referenced library %s on %s\n"), filename,
				timetostring(getcurrenttime()));
#endif
		} else
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Reference library '%s'"), libname);
			pt = fileselect(returninfstr(infstr), filetype, x_(""));
			if (pt != 0) filename = pt;
		}
		elib = newlibrary(libname, filename);
		if (elib == NOLIBRARY) { efree(libfile);   return; }

		/* read the external library */
		savebinindata = io_binindata;
		if (io_verbose < 0 && io_binindata.filelength > 0 && io_inputprogressdialog != 0)
		{
			(void)allocstring(&oldline2, DiaGetTextProgress(io_inputprogressdialog), el_tempcluster);
		}

		len = estrlen(elib->libfile);
		io_libinputrecursivedepth++;
		io_libinputreadmany++;
		if (len > 4 && namesame(&elib->libfile[len-4], x_(".txt")) == 0)
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
		io_binindata = savebinindata;
		if (io_verbose < 0 && io_binindata.filelength > 0 && io_inputprogressdialog != 0)
		{
			DiaSetProgress(io_inputprogressdialog, io_binindata.bytecount, io_binindata.filelength);
			infstr = initinfstr();
			formatinfstr(infstr, _("Reading library %s"), lib->libname);
			DiaSetCaptionProgress(io_inputprogressdialog, returninfstr(infstr));
			DiaSetTextProgress(io_inputprogressdialog, oldline2);
			efree(oldline2);
		}
	}

	/* read the portproto names on this nodeproto */
	io_getinbig(&portcount);
	if (portcount > 0)
		localportnames = (CHAR **)emalloc(portcount * (sizeof (CHAR *)), el_tempcluster);
	for(j=0; j<portcount; j++)
	{
		/* read the portproto name */
		protoname = io_gettempstring();
		if (protoname == 0) break;
		(void)allocstring(&localportnames[j], protoname, el_tempcluster);
	}

	/* find this cell in the external library */
	npdummy = NONODEPROTO;
	for(index=0; ; index++)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("%sFROM%s"), cellname, elib->libname);
		if (index > 0) formatinfstr(infstr, x_(".%ld"), index);
		(void)allocstring(&dummycellname, returninfstr(infstr), el_tempcluster);
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesame(np->protoname, dummycellname) == 0) break;
		if (np == NONODEPROTO) break;
		efree((CHAR *)dummycellname);
	}
	for(np = elib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->lib != elib)
		{
			ttyputerr(_("ERROR: Bad cell in library %s"), elib->libname);
			continue;
		}
		if (np->cellview != v) continue;
		if (namesame(np->protoname, dummycellname) == 0) npdummy = np;
		if (np->version != vers) continue;
		if (namesame(np->protoname, cellname) != 0) continue;
		break;
	}
	if (np == NONODEPROTO) np = npdummy;
	if (np == NONODEPROTO)
	{
		/* cell not found in library: issue warning */
		infstr = initinfstr();
		addstringtoinfstr(infstr, cellname);
		if (v != el_unknownview)
			formatinfstr(infstr, x_("{%s}"), v->sviewname);
		ttyputerr(_("Cannot find cell %s in library %s"),
			returninfstr(infstr), elib->libname);
	}

	/* if cell found, check that size is unchanged */
	if (np != NONODEPROTO)
	{
#if 0
		if (np->lowx != lowx || np->highx != highx ||
			np->lowy != lowy || np->highy != highy)
#else
		if (np->highx-np->lowx != highx-lowx ||
			np->highy-np->lowy != highy-lowy)
#endif
		{
			ttyputerr(_("Error: cell %s in library %s has changed size since its use in library %s"),
				nldescribenodeproto(np), elib->libname, lib->libname);
			ttyputerr(_("   Cell %s in library %s is now %sx%s but the instance in library %s is %sx%s"),
				nldescribenodeproto(np), elib->libname, latoa(np->highx-np->lowx,0),
				latoa(np->highy-np->lowy,0), lib->libname, latoa(highx-lowx,0), latoa(highy-lowy,0));
			np = NONODEPROTO;
		}
	}

	/* if cell found, check that ports match */
	if (np != NONODEPROTO)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			pp->temp1 = 0;
		for(j=0; j<portcount; j++)
		{
			protoname = localportnames[j];
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (namesame(protoname, pp->protoname) == 0) break;
			if (pp == NOPORTPROTO)
			{
				ttyputerr(_("Error: cell %s in library %s must have port %s"),
					describenodeproto(np), elib->libname, protoname);
				np = NONODEPROTO;
				break;
			}
			pp->temp1 = 1;
		}
	}
#if 0
	if (np != NONODEPROTO)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->temp1 != 0) continue;
			ttyputerr(_("Error: cell %s in library %s, should not have port %s"),
				describenodeproto(np), elib->libname, pp->protoname);
			np = NONODEPROTO;
			break;
		}
	}
#endif

	/* if cell found, warn if minor modification was made */
	if (np != NONODEPROTO)
	{
		if (np->revisiondate != revision)
		{
			ttyputerr(_("Warning: cell %s in library %s has changed since its use in library %s"),
				describenodeproto(np), elib->libname, lib->libname);
		}
	}

	/* make new cell if needed */
	if (np != NONODEPROTO) newcell = 0; else
	{
		/* create a cell that meets these specs */
		newcell = 1;
		ttyputerr(_("...Creating dummy version of cell in library %s"), lib->libname);
		np = allocnodeproto(lib->cluster);
		if (np == NONODEPROTO) return;
		np->primindex = 0;
		np->lowx = lowx;
		np->highx = highx;
		np->lowy = lowy;
		np->highy = highy;
		np->firstinst = NONODEINST;
		np->firstnodeinst = NONODEINST;
		np->firstarcinst = NOARCINST;
		np->tech = NOTECHNOLOGY;
		np->lib = lib;
		np->firstportproto = NOPORTPROTO;
		np->adirty = 0;
		np->cellview = v;
		np->creationdate = creation;
		np->revisiondate = revision;
		setval((INTBIG)np, VNODEPROTO, x_("IO_true_library"), (INTBIG)elib->libname, VSTRING);

		/* rename cell */
		(void)allocstring(&np->protoname, dummycellname, lib->cluster);

		/* determine version number of this cell */
		np->version = 1;
		FOR_CELLGROUP(onp, np)
		{
			if (onp->cellview == v && namesame(onp->protoname, np->protoname) == 0 &&
				onp->version >= np->version)
					np->version = onp->version + 1;
		}

		/* insert in the library and cell structures */
		if (i != 0)		/* why not always? */
		{
			/* NEW CELL-based CODE */
			db_insertnodeproto(np);
		}

		/* create initial R-tree data */
		if (geomstructure(np)) return;

		/* create an artwork "Crossed box" to define the cell size */
		ni = allocnodeinst(lib->cluster);
		ni->proto = art_crossedboxprim;
		ni->parent = np;
		ni->nextnodeinst = np->firstnodeinst;
		np->firstnodeinst = ni;
		ni->lowx = lowx;   ni->highx = highx;
		ni->lowy = lowy;   ni->highy = highy;
		ni->geom = allocgeom(lib->cluster);
		ni->geom->entryisnode = TRUE;   ni->geom->entryaddr.ni = ni;
		linkgeom(ni->geom, np);
	}
	io_binindata.nodeprotolist[i] = np;
	efree(dummycellname);

	/* read the portprotos on this nodeproto */
	lpt = NOPORTPROTO;
	for(j=0; j<portcount; j++)
	{
		/* read the portproto name */
		protoname = localportnames[j];
		pp = getportproto(np, protoname);
		if (pp == NOPORTPROTO)
		{
			if (newcell == 0)
				ttyputerr(_("Cannot find port %s on cell %s in library %s"), 
					protoname, describenodeproto(np), elib->libname);
			pp = allocportproto(lib->cluster);
			(void)allocstring(&pp->protoname, protoname, lib->cluster);
			pp->parent = np;
			if (lpt == NOPORTPROTO) np->firstportproto = pp; else
				lpt->nextportproto = pp;
			lpt = pp;
			pp->temp2 = 0;
		}
		io_binindata.portprotolist[io_binindata.portprotoindex] = pp;
		io_binindata.portprotoindex++;
	}

	/* free memory */
	for(j=0; j<portcount; j++)
		efree(localportnames[j]);
	if (portcount > 0) efree((CHAR *)localportnames);
	efree(libfile);
}

/* routine to read a node instance.  returns true upon error */
BOOLEAN io_readnodeinst(NODEINST *ni)
{
	INTBIG i, k;
	REGISTER INTBIG j, arcindex;
	CHAR *inst;
	REGISTER PORTARCINST *pi, *lpo;
	REGISTER PORTEXPINST *pe, *lpe;
	PORTPROTO *pp;
	REGISTER ARCINST *ai;
	static INTBIG orignodenamekey = 0;

	/* read the nodeproto index */
	io_getinbig(&ni->proto);
	if (io_convertnodeproto(&ni->proto, &inst) != 0)
		ttyputmsg(_("...while reading node instance"));
	if (inst != 0)
	{
		if (orignodenamekey == 0) orignodenamekey = makekey(x_("NODE_original_name"));
		nextchangequiet();
		(void)setvalkey((INTBIG)ni, VNODEINST, orignodenamekey, (INTBIG)inst, VSTRING|VDONTSAVE);
	}

	/* read the descriptive information */
	io_getinbig(&ni->lowx);
	io_getinbig(&ni->lowy);
	io_getinbig(&ni->highx);
	io_getinbig(&ni->highy);

	/* ignore anchor point for cell references (version 13 and later) */
	if (io_binindata.magic <= MAGIC13)
	{
		if (ni->proto->primindex == 0)
		{
			io_getinbig(&i);
			io_getinbig(&i);
		}
	}

	/* read rotation and transposition */
	io_getinbig(&i);
	io_getinbig(&k);
	ni->rotation = (INTSML)k;

	/* in 7.01 and higher, allow mirror bits */
	if (io_binindata.emajor > 7 || (io_binindata.emajor == 7 && io_binindata.eminor >= 1))
	{
		/* new version: allow mirror bits */
		ni->transpose = 0;
		if ((i&1) != 0)
		{
			/* the old-style transpose bit */
			ni->transpose = 1;
		} else
		{
			/* check for new style mirror bits */
			if ((i&2) != 0)
			{
				if ((i&4) != 0)
				{
					/* mirror in X and Y */
					ni->rotation = (ni->rotation + 1800) % 3600;
				} else
				{
					/* mirror in X */
					ni->rotation = (ni->rotation + 900) % 3600;
					ni->transpose = 1;
				}
			} else if ((i&4) != 0)
			{
				/* mirror in Y */
				ni->rotation = (ni->rotation + 2700) % 3600;
				ni->transpose = 1;
			}
		}
	} else
	{
		/* old version: just treat it as a transpose */
		if (i != 0) ni->transpose = 1; else
			ni->transpose = 0;
	}

	/* versions 9 and later get text descriptor for cell name */
	if (io_binindata.magic <= MAGIC9)
	{
		if (io_binindata.converttextdescriptors)
		{
			/* conversion is done later */
			io_getinubig(&ni->textdescript[0]);
			ni->textdescript[1] = 0;
		} else
		{
			io_getinubig(&ni->textdescript[0]);
			io_getinubig(&ni->textdescript[1]);
		}
	}

	/* read the nodeinst name (versions 1, 2, or 3 only) */
	if (io_binindata.magic >= MAGIC3)
	{
		inst = io_getstring(el_tempcluster);
		if (inst == 0) return(TRUE);
		if (*inst != 0)
		{
			nextchangequiet();
			if (setvalkey((INTBIG)ni, VNODEINST, el_node_name_key,
				(INTBIG)inst, VSTRING|VDISPLAY) == NOVARIABLE) return(TRUE);
		}
		efree(inst);
	}

	/* ignore the geometry index (versions 4 or older) */
	if (io_binindata.magic > MAGIC5) io_getinbig(&i);

	/* read the arc ports */
	io_getinbig(&i);
	lpo = NOPORTARCINST;
	for(j=0; j<i; j++)
	{
		/* read the arcinst information (and the particular end on the arc) */
		io_getinbig(&k);
		arcindex = k >> 1;
		if (k < 0 || arcindex >= io_binindata.arcindex) return(TRUE);
		ai = io_binindata.arclist[arcindex];
		pi = ai->end[k&1].portarcinst;
		pi->conarcinst = ai;

		/* link in the portarcinst */
		if (j == 0) ni->firstportarcinst = pi; else lpo->nextportarcinst = pi;
		lpo = pi;

		/* read the arcinst index */
		io_getinbig(&pi->proto);
		(void)io_convertportproto(&pi->proto, TRUE);

		/* read variable information */
		if (io_readvariables((INTBIG)pi, VPORTARCINST) < 0) return(TRUE);
	}

	/* setup the last portinst pointer */
	if (i == 0) ni->firstportarcinst = NOPORTARCINST; else
		lpo->nextportarcinst = NOPORTARCINST;

	/* read the exports */
	io_getinbig(&i);
	lpe = NOPORTEXPINST;
	for(j=0; j<i; j++)
	{
		/* read the export index */
		io_getinbig(&pp);
		(void)io_convertportproto(&pp, TRUE);
		pe = pp->subportexpinst;
		pe->exportproto = pp;

		if (j == 0) ni->firstportexpinst = pe; else lpe->nextportexpinst = pe;
		lpe = pe;

		/* read the export prototype */
		io_getinbig(&pe->proto);
		(void)io_convertportproto(&pe->proto, TRUE);
		pp->subportproto = pe->proto;

		/* read variable information */
		if (io_readvariables((INTBIG)pe, VPORTEXPINST) < 0) return(TRUE);
	}

	/* setup the last portinst pointer */
	if (i == 0) ni->firstportexpinst = NOPORTEXPINST; else
		lpe->nextportexpinst = NOPORTEXPINST;

	/* ignore the "seen" bits (versions 8 and older) */
	if (io_binindata.magic > MAGIC9) io_getinubig(&k);

	/* read the tool information */
	if (io_binindata.magic <= MAGIC7)
	{
		/* version 7 and later simply read the relevant data */
		io_getinubig(&ni->userbits);
	} else
	{
		/* version 6 and earlier must sift through the information */
		if (io_binindata.aabcount >= 1) io_getinubig(&ni->userbits);
		for(i=1; i<io_binindata.aabcount; i++) io_getinubig(&k);
	}

	/* read variable information */
	if (io_readvariables((INTBIG)ni, VNODEINST) < 0) return(TRUE);

	return(FALSE);
}

/* routine to read (and mostly ignore) a geometry module */
void io_readgeom(BOOLEAN *isnode, INTBIG *moreup)
{
	INTBIG i, type;

	io_getinbig(&type);				/* read entrytype */
	if (type != 0) *isnode = TRUE; else
		*isnode = FALSE;
	if (*isnode) io_getinbig(&i);/* skip entryaddr */
	io_getinbig(&i);				/* skip lowx */
	io_getinbig(&i);				/* skip highx */
	io_getinbig(&i);				/* skip lowy */
	io_getinbig(&i);				/* skip highy */
	io_getinbig(&i);				/* skip moreleft */
	io_getinbig(&i);				/* skip ll */
	io_getinbig(&i);				/* skip moreright */
	io_getinbig(&i);				/* skip lr */
	io_getinbig(moreup);			/* read moreup */
	io_getinbig(&i);				/* skip lu */
	io_getinbig(&i);				/* skip moredown */
	io_getinbig(&i);				/* skip ld */
	io_ignorevariables();			/* skip variables */
}

BOOLEAN io_readarcinst(ARCINST *ai)
{
	INTBIG i, j, index;
	REGISTER CHAR *inst;

	/* read the arcproto pointer */
	io_getinbig(&ai->proto);
	io_convertarcproto(&ai->proto);

	/* read the arc length (versions 5 or older) */
	if (io_binindata.magic >= MAGIC5) io_getinbig(&ai->length);

	/* read the arc width */
	io_getinbig(&ai->width);

	/* ignore the signals value (versions 6, 7, or 8) */
	if (io_binindata.magic <= MAGIC6 && io_binindata.magic >= MAGIC8)
		io_getinbig(&i);

	/* read the arcinst name (versions 3 or older) */
	if (io_binindata.magic >= MAGIC3)
	{
		inst = io_getstring(el_tempcluster);
		if (inst == 0) return(TRUE);
		if (*inst != 0)
		{
			nextchangequiet();
			if (setvalkey((INTBIG)ai, VARCINST, el_arc_name_key,
				(INTBIG)inst, VSTRING|VDISPLAY) == NOVARIABLE) return(TRUE);
		}
		efree(inst);
	}

	/* read the arcinst end information */
	for(i=0; i<2; i++)
	{
		io_getinbig(&ai->end[i].xpos);
		io_getinbig(&ai->end[i].ypos);

		/* read the arcinst's connecting nodeinst index */
		io_getinbig(&ai->end[i].nodeinst);
		index = (INTBIG)ai->end[i].nodeinst;
		if (index >= 0 && index < io_binindata.nodeindex)
			ai->end[i].nodeinst = io_binindata.nodelist[index]; else
		{
			ttyputerr(_("Warning: corrupt data on arc.  Do a 'Check and Repair Library'"));
			ai->end[i].nodeinst = NONODEINST;
		}			
	}

	/* compute the arc length (versions 6 or newer) */
	if (io_binindata.magic <= MAGIC6) ai->length = computedistance(ai->end[0].xpos,
		ai->end[0].ypos, ai->end[1].xpos, ai->end[1].ypos);

	/* ignore the geometry index (versions 4 or older) */
	if (io_binindata.magic > MAGIC5) io_getinbig(&i);

	/* ignore the "seen" bits (versions 8 and older) */
	if (io_binindata.magic > MAGIC9) io_getinubig(&i);

	/* read the arcinst's tool information */
	if (io_binindata.magic <= MAGIC7)
	{
		/* version 7 and later simply read the relevant data */
		io_getinubig(&ai->userbits);

		/* versions 7 and 8 ignore net number */
		if (io_binindata.magic >= MAGIC8) io_getinbig(&j);
	} else
	{
		/* version 6 and earlier must sift through the information */
		if (io_binindata.aabcount >= 1) io_getinubig(&ai->userbits);
		for(i=1; i<io_binindata.aabcount; i++) io_getinubig(&j);
	}

	/* read variable information */
	if (io_readvariables((INTBIG)ai, VARCINST) < 0) return(TRUE);
	return(FALSE);
}

/******************** VARIABLE ROUTINES ********************/

/* routine to read the global namespace.  returns true upon error */
BOOLEAN io_readnamespace(void)
{
	REGISTER INTBIG i;

	io_getinbig(&io_binindata.namecount);
	if (io_verbose > 0) ttyputmsg(M_("Reading %ld variable names"), io_binindata.namecount);
	if (io_binindata.namecount == 0) return(FALSE);

	/* read in the namespace */
	io_binindata.newnames = (INTBIG *)emalloc((SIZEOFINTBIG * io_binindata.namecount), el_tempcluster);
	if (io_binindata.newnames == 0) return(TRUE);
	io_binindata.realname = (CHAR **)emalloc(((sizeof (CHAR *)) * io_binindata.namecount), el_tempcluster);
	if (io_binindata.realname == 0) return(TRUE);
	for(i=0; i<io_binindata.namecount; i++)
	{
		io_binindata.realname[i] = io_getstring(el_tempcluster);
		if (io_binindata.realname[i] == 0) return(TRUE);
		io_binindata.newnames[i] = 0;
	}
	return(FALSE);
}

/*
 * routine to ignore one set of object variables on readin
 */
void io_ignorevariables(void)
{
	NODEINST node;

	initdummynode(&node);
	(void)io_readvariables((INTBIG)&node, VNODEINST);

	/* this next line is not strictly legal!!! */
	if (node.numvar != 0) db_freevars(&node.firstvar, &node.numvar);
}

/*
 * routine to read a set of object variables.  returns negative upon error and
 * otherwise returns the number of variables read
 */
INTBIG io_readvariables(INTBIG addr, INTBIG type)
{
	REGISTER INTBIG i, j, datasize, ty, keyval;
	REGISTER INTBIG ret;
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN invalid;
	INTBIG count, newtype, len, newaddr, cou, defineddescript;
	UINTBIG newdescript[TEXTDESCRIPTSIZE];
	INTSML key;

	io_getinbig(&count);
	if (io_verbose > 0 && count > 0)
		 ttyputmsg(M_("Reading %ld variables on %s object"),
			count, describeobject(addr, type));
	for(i=0; i<count; i++)
	{
		io_getin(&key, io_binindata.sizeofsmall, SIZEOFINTSML, FALSE);
		io_getinbig(&newtype);
		ty = newtype;
		if (io_verbose > 0 && (type == VTOOL || type == VTECHNOLOGY))
			ttyputmsg(M_("   Reading variable %ld: %s (type=0%o)"), i, io_binindata.realname[key], ty);

		/* version 9 and later reads text description on displayable variables */
		defineddescript = 0;
		if (io_binindata.magic <= MAGIC9)
		{
			if (io_binindata.alwaystextdescriptors)
			{
				io_getinubig(&newdescript[0]);
				io_getinubig(&newdescript[1]);
				defineddescript = 1;
			} else
			{
				if ((ty&VDISPLAY) != 0)
				{
					if (io_binindata.converttextdescriptors)
					{
						/* conversion is done later */
						io_getinubig(&newdescript[0]);
						newdescript[1] = 0;
					} else
					{
						io_getinubig(&newdescript[0]);
						io_getinubig(&newdescript[1]);
					}
					defineddescript = 1;
				}
			}
		}
		if (defineddescript == 0)
		{
			TDCLEAR(newdescript);
			defaulttextdescript(newdescript, NOGEOM);
		}
		if ((ty&VISARRAY) != 0)
		{
			io_getinbig(&len);
			cou = len;
			if ((ty&VLENGTH) == 0) cou++;
			if ((ty&VTYPE) == VCHAR) datasize = SIZEOFCHAR; else
				if ((ty&VTYPE) == VDOUBLE) datasize = SIZEOFINTBIG*2; else
					if ((ty&VTYPE) == VSHORT) datasize = 2; else
						datasize = SIZEOFINTBIG;
			newaddr = (INTBIG)emalloc((cou*datasize), el_tempcluster);
			if (newaddr == 0) return(-1);
			if ((ty&VTYPE) == VGENERAL)
			{
				for(j=0; j<len; j += 2)
				{
					io_getinbig((INTBIG *)(newaddr + (j+1)*datasize));
					ret = io_getinvar((INTBIG *)(newaddr + j*datasize),
						*(INTBIG *)(newaddr + (j+1)*datasize));
					if (ret < 0) return(-1);
					if (ret > 0)
					{
						ttyputmsg(_("...while reading variable '%s' on %s object"),
							io_binindata.realname[key], us_variabletypename(type));
					}
				}
			} else
			{
				for(j=0; j<len; j++)
				{
					ret = io_getinvar((INTBIG *)(newaddr + j*datasize), ty);
					if (ret < 0) return(-1);
					if (ret > 0)
					{
						ttyputmsg(_("...while reading variable '%s' on %s object"),
							io_binindata.realname[key], us_variabletypename(type));
					}
				}
			}
			if ((ty&VLENGTH) == 0)
				for(j=0; j<datasize; j++)
					((CHAR *)newaddr)[len*datasize+j] = -1;
		} else
		{
			if ((newtype&VTYPE) == VDOUBLE) newtype = (newtype & ~VTYPE) | VFLOAT;
			ret = io_getinvar(&newaddr, ty);
			if (ret < 0) return(-1);
			if (ret > 0)
			{
				ttyputmsg(_("...while reading variable '%s' on %s object"),
					io_binindata.realname[key], us_variabletypename(type));
			}
		}

		/* copy this variable into database */
		if (key < 0 || key >= io_binindata.namecount)
		{
			ttyputmsg(_("Bad variable index (%ld, limit is %ld) on %s object"),
				key, io_binindata.namecount, describeobject(addr, type));
			return(-1);
		}

		/* see if the variable is deprecated */
		invalid = isdeprecatedvariable(addr, type, io_binindata.realname[key]);
		if (!invalid)
		{
			if (io_binindata.newnames[key] == 0)
				io_binindata.newnames[key] = makekey(io_binindata.realname[key]);
			keyval = io_binindata.newnames[key];
			nextchangequiet();

			var = setvalkey(addr, type, keyval, newaddr, newtype);
			if (var == NOVARIABLE) return(-1);
			TDCOPY(var->textdescript, newdescript);

			/* handle updating of technology caches */
			if (type == VTECHNOLOGY)
				changedtechnologyvariable(keyval);
		}

		/* free the memory allocated for the creation of this variable */
		if ((ty&VTYPE) == VSTRING || (ty&(VCODE1|VCODE2)) != 0)
		{
			if ((ty&VISARRAY) == 0) efree((CHAR *)newaddr); else
				for(j=0; j<len; j++) efree((CHAR *)((INTBIG *)newaddr)[j]);
		}
		if ((ty&VISARRAY) != 0) efree((CHAR *)newaddr);
	}
	return(count);
}

/*
 * Helper routine to read a variable at address "addr" of type "ty".
 * Returns zero if OK, negative on memory error, positive if there were
 * correctable problems in the read.
 */
INTBIG io_getinvar(INTBIG *addr, INTBIG ty)
{
	INTBIG i;
	REGISTER CHAR *pp;
	REGISTER ARCINST *ai;
	PORTPROTO *ppr;
	double doub;

	if ((ty&(VCODE1|VCODE2)) != 0) ty = VSTRING;
	switch (ty&VTYPE)
	{
		case VADDRESS:
		case VINTEGER:
		case VFRACT:
			io_getinbig(addr);
			if ((ty&VTYPE) == VFLOAT)
			{
				if (io_binindata.swap_bytes != 0) io_binindata.swapped_floats++;
			}
			break;
		case VFLOAT:
			io_getin(addr, sizeof(float), sizeof(float), FALSE);
			if (io_binindata.swap_bytes != 0) io_binindata.swapped_doubles++;
			break;
		case VDOUBLE:
			io_getin(&doub, sizeof(double), sizeof(double), FALSE);
			*addr = castint((float)doub);
			if (io_binindata.swap_bytes != 0) io_binindata.swapped_doubles++;
			break;
		case VSHORT:
			io_getin(addr, io_binindata.sizeofsmall, SIZEOFINTSML, TRUE);
			break;
		case VBOOLEAN:
		case VCHAR:
			io_getin(addr, 1, 1, FALSE);
			break;
		case VSTRING:
			pp = io_getstring(el_tempcluster);
			if (pp == 0) return(-1);
			*addr = (INTBIG)pp;
			break;
		case VNODEINST:
			io_getinbig(addr);
			if (*addr < 0 || *addr >= io_binindata.nodeindex) *addr = -1; else
				*addr = (INTBIG)io_binindata.nodelist[*addr];
			break;
		case VNODEPROTO:
			io_getinbig(addr);
			if (io_convertnodeproto((NODEPROTO **)addr, 0) != 0) return(1);
			break;
		case VARCPROTO:
			io_getinbig(addr);
			io_convertarcproto((ARCPROTO **)addr);
			break;
		case VPORTPROTO:
			io_getinbig(addr);
			(void)io_convertportproto((PORTPROTO **)addr, TRUE);
			break;
		case VARCINST:
			io_getinbig(addr);
			if (*addr < 0 || *addr >= io_binindata.arcindex) *addr = -1; else
				*addr = (INTBIG)io_binindata.arclist[*addr];
			break;
		case VGEOM:
			*addr = -1;
			io_getinbig(&i);
			if (io_binindata.magic <= MAGIC5)
			{
				/* versions 5 and later store extra information */
				if (i != 0)
				{
					io_getinbig(addr);
					if (*addr < 0 || *addr >= io_binindata.nodeindex) *addr = -1;
						else *addr = (INTBIG)io_binindata.nodelist[*addr]->geom;
				} else
				{
					io_getinbig(addr);
					if (*addr < 0 || *addr >= io_binindata.arcindex) *addr = -1;
						else *addr = (INTBIG)io_binindata.arclist[*addr]->geom;
				}
			}
			break;
		case VTECHNOLOGY:
			io_getinbig(addr);
			if (*addr != -1) *addr = (INTBIG)io_gettechlist(*addr);
			break;
		case VPORTARCINST:
			io_getinbig(addr);
			if (*addr != -1)
			{
				ai = io_binindata.arclist[(*addr) >> 1];
				i = (*addr) & 1;
				*addr = (INTBIG)ai->end[i].portarcinst;
			}
			break;
		case VPORTEXPINST:
			io_getinbig(addr);
			if (*addr != -1)
			{
				ppr = (PORTPROTO *)*addr;
				(void)io_convertportproto(&ppr, TRUE);
				*addr = (INTBIG)ppr->subportexpinst;
			}
			break;
		case VLIBRARY:
			pp = io_getstring(el_tempcluster);
			if (pp == 0) return(-1);
			*addr = (INTBIG)getlibrary(pp);
			break;
		case VTOOL:
			io_getinbig(addr);
			if (*addr < 0 || *addr >= io_binindata.aacount) *addr = -1; else
			{
				i = io_binindata.toollist[*addr];
				if (i < 0 || i >= el_maxtools)
				{
					i = 0;
					if (io_binindata.toolerror[*addr] != 0)
					{
						ttyputerr(_("WARNING: no tool called '%s', using 'user'"),
							io_binindata.toolerror[*addr]);
						efree(io_binindata.toolerror[*addr]);
						io_binindata.toolerror[*addr] = 0;
					}
				}
				*addr = (INTBIG)&el_tools[i];
			}
			break;
		case VRTNODE:
			*addr = -1;
			break;
   }
   return(0);
}

/*
 * routine to fix variables that make reference to external cells.
 */
void io_fixexternalvariables(VARIABLE *firstvar, INTSML numvar)
{
	REGISTER INTBIG i;
	REGISTER INTBIG j, k, type, len;
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
				if ((INTBIG)np >= 0 && (INTBIG)np < io_binindata.nodeprotoindex)
				{
					k = (INTBIG)np;
					nparray[j] = io_binindata.nodeprotolist[k];
				}
			}
		} else
		{
			np = (NODEPROTO *)var->addr;
			if ((INTBIG)np >= 0 && (INTBIG)np < io_binindata.nodeprotoindex)
			{
				k = (INTBIG)np;
				var->addr = (INTBIG)io_binindata.nodeprotolist[k];
			}
		}
	}
}

/******************** INPUT HELPER ROUTINES ********************/

/*
 * routine to return the proper tool bits word given the arrangement
 * of tools.  These words have one bit per tool according to
 * the tool's position which may get re-arranged.
 */
INTBIG io_arrangetoolbits(INTBIG val)
{
	REGISTER INTBIG out, i, j;

	/* if the bits are not re-arranged, result is simple */
	if (!io_binindata.toolbitsmessed) return(val);

	/* re-arrange the bits in the word */
	out = 0;
	for(i=0; i<io_binindata.aacount; i++)
	{
		j = io_binindata.toollist[i];
		if (j < 0) continue;
		if ((val & (1 << i)) != 0) out |= 1 << j;
	}
	return(out);
}

/*
 * routine to convert the nodeproto index at "np" to a true nodeproto pointer.
 * Sets "origname" to a string with the original name of this prototype (if there were
 * conversion problems in the file).  Sets "origname" to zero if the pointer is fine.
 */
INTBIG io_convertnodeproto(NODEPROTO **np, CHAR **origname)
{
	REGISTER INTBIG i, nindex, error;

	if (*np == NONODEPROTO) return(0);
	i = (INTBIG)*np;
	if (origname != 0) *origname = 0;
	error = 0;
	if (i < 0)
	{
		nindex = -i - 2;
		if (nindex >= io_binindata.nodepprotoindex)
		{
			ttyputerr(_("Error: want primitive node index %ld when limit is %ld"),
				nindex, io_binindata.nodepprotoindex);
			nindex = 0;
			error = 1;
		}
		*np = io_getnodepprotolist(nindex);
		if (origname != 0) *origname = io_binindata.nodepprotoorig[nindex];
	} else
	{
		if (i >= io_binindata.nodeprotoindex)
		{
			ttyputerr(_("Error: want cell index %ld when limit is %ld"),
				i, io_binindata.nodeprotoindex);
			*np = io_getnodepprotolist(0);
			error = 1;
		} else
		{
			*np = io_binindata.nodeprotolist[i];
			if (*np == 0)
			{
				/* reference to cell in another library: unresolved now */
				*np = (NODEPROTO *)i;
			}
		}
	}
	return(error);
}

BOOLEAN io_convertportproto(PORTPROTO **pp, BOOLEAN showerror)
{
	REGISTER INTBIG i, pindex;
	REGISTER BOOLEAN err;

	if (*pp == NOPORTPROTO) return(FALSE);
	i = (INTBIG)*pp;
	err = FALSE;
	if (i < 0)
	{
		pindex = -i - 2;
		if (pindex >= io_binindata.portpprotoindex)
		{
			if (showerror)
				ttyputerr(_("Error: want primitive port index %ld when limit is %ld"),
					pindex, io_binindata.portpprotoindex);
			pindex = 0;
			err = TRUE;
		}
		*pp = io_getportpprotolist(pindex);
	} else
	{
		if (i >= io_binindata.portprotolimit)
		{
			if (showerror)
				ttyputerr(_("Error: want port index %ld when limit is %ld"),
					i, io_binindata.portprotolimit);
			i = 0;
			err = TRUE;
		}
		*pp = io_binindata.portprotolist[i];
	}
	return(err);
}

void io_convertarcproto(ARCPROTO **ap)
{
	REGISTER INTBIG i, aindex;

	if (*ap == NOARCPROTO) return;
	i = (INTBIG)*ap;
	aindex = -i - 2;
	if (aindex >= io_binindata.arcprotoindex || aindex < 0)
	{
		ttyputerr(_("Want primitive arc index %ld when range is 0 to %ld"),
			aindex, io_binindata.arcprotoindex);
		aindex = 0;
	}
	*ap = io_getarcprotolist(aindex);
}

/*
 * Routine to ensure that all ports in library "lib" are valid.  Invalid ports are
 * caused when references to cells outside of this library cannot be found.  When
 * that happens, dummy cells are generated with invalid ports.  At this point, the
 * ports can be defined by examining their usage.
 */
void io_ensureallports(LIBRARY *lib)
{
	REGISTER NODEPROTO *np, *onp, *pin;
	REGISTER PORTPROTO *pp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	ARCPROTO *onlyarc;
	REGISTER TECHNOLOGY *tech;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG thisend;
	INTBIG lx, hx, ly, hy, x, y;
	BOOLEAN first;
	XARRAY rot, trn, trans;

	/* look at every port in the library */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if (pp->subnodeinst != NONODEINST) continue;

			/* undefined port: figure out what connects to it */
			for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
				for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
					ap->temp1 = 0;
			first = TRUE;
			onlyarc = NOARCPROTO;
			for(onp = lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			{
				for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto != np) continue;
					makerotI(ni, rot);
					maketransI(ni, trn);
					transmult(rot, trn, trans);

					/* found an instance of the external cell */
					for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					{
						if (pi->proto != pp) continue;
						ai = pi->conarcinst;
						if (!first && ai->proto != onlyarc) onlyarc = NOARCPROTO;
						if (first) onlyarc = ai->proto;
						ai->proto->temp1 = 1;
						if (ai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
						x = ai->end[thisend].xpos;
						y = ai->end[thisend].ypos;
						xform(x, y, &x, &y, trans);
						if (first)
						{
							lx = hx = x;
							ly = hy = y;
							first = FALSE;
						} else
						{
							if (x < lx) lx = x;
							if (x > hx) hx = x;
							if (y < ly) ly = y;
							if (y > hy) hy = y;
						}
					}
					for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					{
						if (pe->proto != pp) continue;
						io_binfindallports(lib, pe->exportproto, &lx, &hx, &ly, &hy, &first,
							&onlyarc, trans);
					}
				}
			}

			if (first)
			{
				/* nothing found to tell where the port should go: put it in the middle */
				lx = hx = (np->lowx + np->highx) / 2;
				ly = hy = (np->lowy + np->highy) / 2;
			}

			/* found where the port should go: create it */
			if (onlyarc == NOARCPROTO) pin = gen_univpinprim; else
				pin = getpinproto(onlyarc);
			lx = hx = (lx + hx) / 2;
			ly = hy = (ly + hy) / 2;

			ni = allocnodeinst(lib->cluster);
			ni->proto = pin;
			ni->parent = np;
			ni->nextnodeinst = np->firstnodeinst;
			np->firstnodeinst = ni;
			ni->lowx = lx;   ni->highx = hx;
			ni->lowy = ly;   ni->highy = hy;
			ni->geom = allocgeom(lib->cluster);
			ni->geom->entryisnode = TRUE;   ni->geom->entryaddr.ni = ni;
			linkgeom(ni->geom, np);

			pe = allocportexpinst(lib->cluster);
			pe->exportproto = pp;
			pe->proto = ni->proto->firstportproto;
			ni->firstportexpinst = pe;
			pp->subnodeinst = ni;
			pp->subportexpinst = pe;
			pp->connects = pe->proto->connects;
			pp->subportproto = pe->proto;
			pp->userbits = pe->proto->userbits;
			TDCLEAR(pp->textdescript);
			defaulttextsize(1, pp->textdescript);
			defaulttextdescript(pp->textdescript, NOGEOM);
		}
	}
}

/*
 * Helper routine for "io_ensureallports" to find all exports in library "lib" attached
 * to port "pp" and gather the bounding box of all arc connections in "lx", "hx", "ly", and
 * "hy".  Sets "first" to false once a wire is found, and sets "onlyarc" to the type of arc
 * found.  Uses "prevtrans" as the transformation matrix to this point.
 */
void io_binfindallports(LIBRARY *lib, PORTPROTO *pp, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy,
	BOOLEAN *first, ARCPROTO **onlyarc, XARRAY prevtrans)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER INTBIG thisend;
	INTBIG x, y;
	XARRAY rot, trn, thistran, trans;

	/* look at all nodes that use the instance with this port */
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto != pp->parent) continue;

			/* find an arc at this port */
			makerotI(ni, rot);
			maketransI(ni, trn);
			transmult(rot, trn, thistran);
			transmult(thistran, prevtrans, trans);
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if (pi->proto != pp) continue;
				ai = pi->conarcinst;
				if (!(*first) && ai->proto != *onlyarc) *onlyarc = NOARCPROTO;
				if (*first) *onlyarc = ai->proto;
				ai->proto->temp1 = 1;
				if (ai->end[0].portarcinst == pi) thisend = 0; else thisend = 1;
				x = ai->end[thisend].xpos;
				y = ai->end[thisend].ypos;
				xform(x, y, &x, &y, trans);
				if (*first)
				{
					*lx = *hx = x;
					*ly = *hy = y;
					*first = FALSE;
				} else
				{
					if (x < *lx) *lx = x;
					if (x > *hx) *hx = x;
					if (y < *ly) *ly = y;
					if (y > *hy) *hy = y;
				}
			}
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				if (pe->proto != pp) continue;
				io_binfindallports(lib, pe->exportproto, lx, hx, ly, hy, first,
					onlyarc, trans);
			}
		}
	}
}

/******************** DATA ACCESS ROUTINES ********************/

ARCPROTO *io_getarcprotolist(INTBIG i)
{
	REGISTER ARCPROTO *ap;
	REGISTER CHAR *name;
	REGISTER void *infstr;

	if (io_binindata.arcprotoerror[i] != 0)
	{
		ap = NOARCPROTO;
		while (ap == NOARCPROTO)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Cannot find arc %s', use which instead [%s] "),
				io_binindata.arcprotoerror[i], io_binindata.arcprotolist[i]->protoname);
			name = ttygetline(returninfstr(infstr));
			if (name == 0 || *name == 0) break;
			for(ap = io_binindata.arcprotolist[i]->tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
				if (namesame(ap->protoname, name) == 0) break;
		}
		if (ap != NOARCPROTO) io_binindata.arcprotolist[i] = ap;
		efree(io_binindata.arcprotoerror[i]);
		io_binindata.arcprotoerror[i] = 0;
	}
	return(io_binindata.arcprotolist[i]);
}

PORTPROTO *io_getportpprotolist(INTBIG i)
{
	if (io_binindata.portpprotoerror[i] != 0)
	{
		ttyputerr(_("WARNING: port %s not found, using %s"), io_binindata.portpprotoerror[i],
			io_binindata.portpprotolist[i]->protoname);
		efree(io_binindata.portpprotoerror[i]);
		io_binindata.portpprotoerror[i] = 0;
	}
	return(io_binindata.portpprotolist[i]);
}

NODEPROTO *io_getnodepprotolist(INTBIG i)
{
	REGISTER NODEPROTO *np;
	REGISTER CHAR *name;
	REGISTER void *infstr;

	(void)io_gettechlist(io_binindata.nodepprototech[i]);
	if (io_binindata.nodepprotoerror[i])
	{
		np = NONODEPROTO;
		while (np == NONODEPROTO)
		{
			infstr = initinfstr();
			formatinfstr(infstr, _("Cannot find primitive '%s', use which instead [%s] "),
				io_binindata.nodepprotoorig[i], io_binindata.nodepprotolist[i]->protoname);
			name = ttygetline(returninfstr(infstr));
			if (name == 0 || *name == 0) break;
			for(np = io_binindata.nodepprotolist[i]->tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (namesame(np->protoname, name) == 0) break;
		}
		if (np != NONODEPROTO) io_binindata.nodepprotolist[i] = np;
		io_binindata.nodepprotoerror[i] = FALSE;
	}
	return(io_binindata.nodepprotolist[i]);
}

TECHNOLOGY *io_gettechlist(INTBIG i)
{
	extern COMCOMP us_noyesp;
	REGISTER INTBIG count;
	CHAR *pars[10];

	if (io_binindata.techerror[i] != 0)
	{
		ttyputerr(_("WARNING: technology '%s' does not exist, using '%s'"),
			io_binindata.techerror[i], io_binindata.techlist[i]->techname);
		efree(io_binindata.techerror[i]);
		io_binindata.techerror[i] = 0;
		count = ttygetparam(_("Is this OK? [n] "), &us_noyesp, 2, pars);
		if (count <= 0 || namesamen(pars[0], x_("yes"), estrlen(pars[0])) != 0)
			longjmp(io_binindata.filerror, LONGJMPABORTED);
		ttyputerr(_("WARNING: saving this library with a substitute technology may corrupt it!"));
	}
	return(io_binindata.techlist[i]);
}

/******************** I/O ROUTINES ********************/

void io_getinbig(void *data)
{
	io_getin(data, io_binindata.sizeofbig, SIZEOFINTBIG, TRUE);
}

void io_getinubig(void *data)
{
	io_getin(data, io_binindata.sizeofbig, SIZEOFINTBIG, FALSE);
}

/*
 * routine to read "disksize" bytes of data from disk and store them in the "memorysize"-
 * long object at "data".  If "signextend" is nonzero, do sign-extension if the
 * memory object is larger.
 */
void io_getin(void *data, INTBIG disksize, INTBIG memorysize, BOOLEAN signextend)
{
	REGISTER INTBIG ret;
	UCHAR1 swapbyte;
	UCHAR1 buf[128];
	INTBIG i;

	/* sanity check */
	if (disksize == 0)
	{
		ttyputmsg(_("Warning: null length data; database may be bad (byte %ld)"),
			io_binindata.bytecount);
		return;
	}

	/* check for direct transfer */
	if (disksize == memorysize && io_binindata.swap_bytes == 0)
	{
		/* just peel it off the disk */
		ret = xfread((UCHAR1*)data, 1, disksize, io_binindata.filein);
		if (ret == 0) longjmp(io_binindata.filerror, LONGJMPEOF);
	} else
	{
		/* not a simple read, use a buffer */
		ret = xfread(buf, 1, disksize, io_binindata.filein);
		if (ret == 0) longjmp(io_binindata.filerror, LONGJMPEOF);
		if (io_binindata.swap_bytes > 0)
		{
			switch (disksize)
			{
				case 2:
					swapbyte = buf[0]; buf[0] = buf[1]; buf[1] = swapbyte;
					break;
				case 4:
					swapbyte = buf[3]; buf[3] = buf[0]; buf[0] = swapbyte;
					swapbyte = buf[2]; buf[2] = buf[1]; buf[1] = swapbyte;
					break;
				case 8:
					swapbyte = buf[7]; buf[7] = buf[0]; buf[0] = swapbyte;
					swapbyte = buf[6]; buf[6] = buf[1]; buf[1] = swapbyte;
					swapbyte = buf[5]; buf[5] = buf[2]; buf[2] = swapbyte;
					swapbyte = buf[4]; buf[4] = buf[3]; buf[3] = swapbyte;
					break;
			}
		}
		if (disksize == memorysize)
		{
			for(i=0; i<memorysize; i++) ((UCHAR1 *)data)[i] = buf[i];
		} else
		{
			if (disksize > memorysize)
			{
				/* trouble! disk has more bits than memory.  check for clipping */
				for(i=0; i<memorysize; i++) ((UCHAR1 *)data)[i] = buf[i];
				for(i=memorysize; i<disksize; i++)
					if (buf[i] != 0 && buf[i] != (UCHAR1)0xFF)
						io_binindata.clipped_integers++;
			} else
			{
				/* disk has smaller integer */
				if (!signextend || (buf[disksize-1] & 0x80) == 0)
				{
					for(i=disksize; i<memorysize; i++) buf[i] = 0;
				} else
				{
					for(i=disksize; i<memorysize; i++) buf[i] = (UCHAR1)0xFF;
				}
				for(i=0; i<memorysize; i++) ((UCHAR1 *)data)[i] = buf[i];
			}
		}
	}
	io_binindata.bytecount += disksize;
	if (io_verbose < 0 && io_binindata.filelength > 0 && io_inputprogressdialog != 0)
	{
		if (io_binindata.bytecount > io_binindata.reported + REPORTINC)
		{
			DiaSetProgress(io_inputprogressdialog, io_binindata.bytecount, io_binindata.filelength);
			io_binindata.reported = io_binindata.bytecount;
		}
	}
}

CHAR *io_getstring(CLUSTER *cluster)
{
	INTBIG len;
	REGISTER INTBIG oldswapbytes;
	CHAR *name;
	REGISTER CHAR *tempstr;

	if (SIZEOFCHAR != io_binindata.sizeofchar)
	{
		/* disk and memory don't match: read into temporary string */
		tempstr = io_gettempstring();
		if (allocstring(&name, tempstr, cluster)) return(0);
	} else
	{
		/* disk and memory match: read the data */
		io_getinbig(&len);
		name = (CHAR *)emalloc((len+1) * SIZEOFCHAR, cluster);
		if (name == 0) return(0);
		if (len != 0)
		{
			oldswapbytes = io_binindata.swap_bytes;
			io_binindata.swap_bytes = 0;     /* prevent swapping */
			io_getin(name, len * SIZEOFCHAR, len * SIZEOFCHAR, FALSE);
			io_binindata.swap_bytes = oldswapbytes;
		}
		name[len] = 0;
	}
	return(name);
}

CHAR *io_gettempstring(void)
{
	INTBIG len;
	REGISTER INTBIG oldswapbytes, maxbytesperchar, i;

	io_getinbig(&len);
	if (len+1 > io_tempstringlength)
	{
		if (io_tempstringlength != 0) efree((CHAR *)io_tempstring);
		io_tempstringlength = len+1;
		maxbytesperchar = maxi(SIZEOFCHAR, io_binindata.sizeofchar);
		io_tempstring = (CHAR1 *)emalloc(io_tempstringlength * maxbytesperchar, io_tool->cluster);
		if (io_tempstring == 0) { io_tempstringlength = 0; return(0); }
	}
	oldswapbytes = io_binindata.swap_bytes;
	if (io_binindata.swap_bytes != 0 && len != 0) io_binindata.swap_bytes = 0;
	if (len != 0) io_getin(io_tempstring, len * io_binindata.sizeofchar,
		len * io_binindata.sizeofchar, FALSE);
	io_binindata.swap_bytes = oldswapbytes;
	if (SIZEOFCHAR != io_binindata.sizeofchar)
	{
		/* size of strings on disk different than in memory: adjust */
		if (SIZEOFCHAR == 1)
		{
			/* disk has unicode, memory has 1-byte: reduce the data */
			for(i=0; i<len; i++)
				io_tempstring[i] = io_tempstring[i*2];
			io_tempstring[len] = 0;
		} else
		{
			/* disk has 1-byte, memory uses unicode: expand the data */
			for(i=len-1; i>=0; i--)
				((CHAR *)io_tempstring)[i] = io_tempstring[i] & 0xFF;
			((CHAR *)io_tempstring)[len] = 0;
		}
	} else
	{
		((CHAR *)io_tempstring)[len] = 0;
	}
	return((CHAR *)io_tempstring);
}
