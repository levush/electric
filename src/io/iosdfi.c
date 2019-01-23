/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iosdfi.c
 * Input/output tool: SDF (Standard Delay Format) 2.1 reader
 * Written by: Russell L. Wright
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
#include "egraphics.h"
#include "efunction.h"
#include "database.h"
#include "edialogs.h"
#include "eio.h"
#include "usr.h"

/* define if hierarchy name is to match that used by ALS */
#define USEALSNAMES 1

/******************** TREE STRUCTURE FOR SDF FILE ********************/

#define MAXLINE    500		/* max characters on SDF input line */
#define MAXDEPTH    50		/* max depth of SDF nesting */

#define PARAMBRANCH  1		/* parameter is a subtree */
#define PARAMATOM    2		/* parameter is atomic */

typedef struct
{
	CHAR   *keyword;
	INTBIG  lineno;
	INTBIG  paramtotal;
	INTBIG  parameters;
	INTBIG *paramtype;
	void  **paramvalue;
} LISPTREE;

static LISPTREE *sdfi_treestack[MAXDEPTH];
static INTBIG    sdfi_treedepth;
static LISPTREE *sdfi_treepos;
static LISPTREE *sdfi_freelisptree = 0;

static INTBIG sdfi_filesize;
static INTBIG sdfi_lineno;
static CHAR  *sdfi_design = NULL;		/* DESIGN keyword */
static CHAR  *sdfi_sdfversion = NULL;	/* SDFVERSION keyword */
static CHAR  *sdfi_date = NULL;			/* DATE keyword */
static CHAR  *sdfi_vendor = NULL;		/* VENDOR keyword */
static CHAR  *sdfi_program = NULL;		/* PROGRAM keyword */
static CHAR  *sdfi_version = NULL;		/* VERSION keyword */
static CHAR  *sdfi_voltage = NULL;		/* VOLTAGE keyword */
static CHAR  *sdfi_process = NULL;		/* PROCESS keyword */
static CHAR  *sdfi_temperature = NULL;	/* TEMPERATURE keyword */
static CHAR  *sdfi_timescale = NULL;	/* TIMESCALE keyword */
static CHAR  *sdfi_hiername = NULL;		/* current cell instance hierarchy name */
static CHAR  *sdfi_celltype = NULL;		/* current CELLTYPE */
static CHAR  *sdfi_instance = NULL;		/* current INSTANCE */
static CHAR  *sdfi_divider = NULL;		/* hierarchy divider - if DIVIDER keyword not in SDF file defaults to . */
static INTBIG sdfi_timeconvert = 1000;	/* multiplier to convert time values to picoseconds */
								/* if TIMESCALE keyword not in SDF file default time unit is 1ns */
static INTBIG SDF_instance_key = 0;
static INTBIG SDF_absolute_port_delay_key = 0;
static INTBIG SDF_absolute_iopath_delay_key = 0;
static NODEPROTO *sdfi_curcell = NONODEPROTO;

/* prototypes for local routines */
static BOOLEAN    sdfi_addparameter(LISPTREE *tree, INTBIG type, void *value);
static LISPTREE  *sdfi_alloclisptree(void);
static void       sdfi_killptree(LISPTREE *lt);
static BOOLEAN    sdfi_pushkeyword(CHAR *pt);
static LISPTREE  *sdfi_readfile(FILE *fp, void *dia);
static BOOLEAN    sdfi_parsetree(LISPTREE *lt);
static BOOLEAN    sdfi_parsedelayfile(LISPTREE *lt);
static BOOLEAN    sdfi_parsecell(LISPTREE *lt);
static BOOLEAN    sdfi_parsedelay(LISPTREE *lt, NODEINST *ni);
static BOOLEAN    sdfi_parseabsolute(LISPTREE *lt, NODEINST *ni);
static BOOLEAN    sdfi_parseincrement(LISPTREE *lt, NODEINST *ni);
static BOOLEAN    sdfi_parseport(LISPTREE *lt, NODEINST *ni);
static BOOLEAN    sdfi_parseiopath(LISPTREE *lt, NODEINST *ni);
static BOOLEAN    sdfi_parsetimescale(LISPTREE *lt);
static NODEINST  *sdfi_getcellinstance(CHAR *celltype, CHAR *instance);
static CHAR      *sdfi_converttops(CHAR *tstring);
static void       sdfi_setstring(CHAR **string, CHAR *value);
static void       sdfi_clearglobals(void);

/*
 * Routine to free all memory associated with this module.
 */
void io_freesdfimemory(void)
{
	LISPTREE *lt;

	/* deallocate lisp-tree objects */
	while (sdfi_freelisptree != 0)
	{
		lt = sdfi_freelisptree;
		sdfi_freelisptree = (LISPTREE *)sdfi_freelisptree->paramvalue;
		efree((CHAR *)lt);
	}

	/* free globals */
	sdfi_clearglobals();
}

BOOLEAN io_readsdflibrary(LIBRARY *lib)
{
	FILE *fp;
	CHAR *filename;
	REGISTER LISPTREE *lt;
	void *dia;

	/* make sure there is a current cell */
	sdfi_curcell = getcurcell();
	if (sdfi_curcell == NONODEPROTO)
	{
		ttyputerr(_("No current cell with which to merge data, aborting SDF input"));
		return(TRUE);
	}

	/* init some global variables */
	sdfi_clearglobals();
	sdfi_setstring(&sdfi_divider, x_("."));	/* default hierarchy divider */
	sdfi_timeconvert = 1000;	/* multiplier to convert delay values to picoseconds */
	if (SDF_instance_key == 0) SDF_instance_key = makekey(x_("SDF_instance"));
	if (SDF_absolute_port_delay_key == 0) SDF_absolute_port_delay_key = makekey(x_("SDF_absolute_port_delay"));
	if (SDF_absolute_iopath_delay_key == 0) SDF_absolute_iopath_delay_key = makekey(x_("SDF_absolute_iopath_delay"));

	/* open the file */
	fp = xopen(lib->libfile, io_filetypesdf, x_(""), &filename);
	if (fp == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	/* prepare for input */
	sdfi_filesize = filesize(fp);
	dia = DiaInitProgress(_("Reading SDF file..."), 0);
	if (dia == 0)
	{
		xclose(fp);
		return(TRUE);
	}
	DiaSetProgress(dia, 0, sdfi_filesize);

	/* read the file */
	lt = sdfi_readfile(fp, dia);
	DiaDoneProgress(dia);
	xclose(fp);
	if (lt == 0)
	{
		ttyputerr(_("Error reading file"));
		return(TRUE);
	}
	ttyputmsg(_("SDF file %s is read"), lib->libfile);

	/* parse the tree */
	if (sdfi_parsetree(lt))
	{
		sdfi_killptree(lt);
		ttyputerr(_("Errors occured during SDF file parsing"));
		return(TRUE);
	}
	ttyputmsg(_("Successfully parsed SDF file"));

	/* release tree memory */
	sdfi_killptree(lt);

	/* add SDF header info to current cell */
	if (sdfi_sdfversion != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_sdfversion"), (INTBIG)sdfi_sdfversion, VSTRING);

	if (sdfi_design != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_design"), (INTBIG)sdfi_design, VSTRING);

	if (sdfi_date != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_date"), (INTBIG)sdfi_date, VSTRING);

	if (sdfi_vendor != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_vendor"), (INTBIG)sdfi_vendor, VSTRING);

	if (sdfi_program != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_program"), (INTBIG)sdfi_program, VSTRING);

	if (sdfi_version != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_version"), (INTBIG)sdfi_version, VSTRING);

	if (sdfi_divider != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_divider"), (INTBIG)sdfi_divider, VSTRING);

	if (sdfi_voltage != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_voltage"), (INTBIG)sdfi_voltage, VSTRING);

	if (sdfi_process != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_process"), (INTBIG)sdfi_process, VSTRING);

	if (sdfi_temperature != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_temperature"), (INTBIG)sdfi_temperature, VSTRING);

	if (sdfi_timescale != NULL)
		(void)setval((INTBIG)sdfi_curcell, VNODEPROTO, x_("SDF_timescale"), (INTBIG)sdfi_timescale, VSTRING);

	ttyputmsg(_("Back-annotation information has been added (library must be saved)"));

	return(FALSE);
}

/*
 * Routine to read the FPGA file in "f" and create a LISPTREE structure which is returned.
 * Returns zero on error.
 */
LISPTREE *sdfi_readfile(FILE *f, void *dia)
{
	CHAR line[MAXLINE];
	REGISTER CHAR save, *pt, *ptend;
	REGISTER INTBIG filepos;
	LISPTREE *treetop;

	/* make the tree top */
	treetop = (LISPTREE *)emalloc((sizeof (LISPTREE)), io_tool->cluster);
	if (treetop == 0) return(0);
	if (allocstring(&treetop->keyword, x_("TOP"), io_tool->cluster)) return(0);
	treetop->paramtotal = 0;
	treetop->parameters = 0;

	/* initialize current position and stack */
	sdfi_treepos = treetop;
	sdfi_treedepth = 0;
	sdfi_lineno = 0;

	for(;;)
	{
		/* get the next line of text */
		if (xfgets(line, MAXLINE, f)) break;
		sdfi_lineno++;
		if ((sdfi_lineno%50) == 0)
		{
			filepos = xtell(f);
			DiaSetProgress(dia, filepos, sdfi_filesize);
		}

		/* stop now if it is a C++ style '//' comment */
		/* TDB - process C style comments */
		for(pt = line; *pt != 0; pt++) if (*pt != ' ' && *pt != '\t') break;
		if (*pt == '/' && *pt++ == '/') { --pt; continue; }

		/* keep parsing it */
		pt = line;
		for(;;)
		{
			/* skip spaces */
			while (*pt == ' ' || *pt == '\t') pt++;
			if (*pt == 0) break;

			/* check for special characters */
			if (*pt == ')')
			{
				save = pt[1];
				pt[1] = 0;
				if (sdfi_pushkeyword(pt)) return(0);
				pt[1] = save;
				pt++;
				continue;
			}

			/* gather a keyword */
			ptend = pt;
			for(;;)
			{
				if (*ptend == ')' || *ptend == ' ' || *ptend == '\t' || *ptend == 0) break;
				if (*ptend == '"')
				{
					ptend++;
					for(;;)
					{
						if (*ptend == 0 || *ptend == '"') break;
						ptend++;
					}
					if (*ptend == '"') ptend++;
					break;
				}
				ptend++;
			}
			save = *ptend;   *ptend = 0;
			if (sdfi_pushkeyword(pt)) return(0);
			*ptend = save;
			pt = ptend;
		}
	}

	if (sdfi_treedepth != 0)
	{
		ttyputerr(_("Not enough close parenthesis in file"));
		return(0);
	}
	return(treetop);
}

/*
 * Routine to add the next keyword "keyword" to the lisp tree in the globals.
 * Returns true on error.
 */
BOOLEAN sdfi_pushkeyword(CHAR *keyword)
{
	REGISTER LISPTREE *newtree;
	CHAR *savekey;
	REGISTER CHAR *pt;

	if (keyword[0] == '(')
	{
		if (sdfi_treedepth >= MAXDEPTH)
		{
			ttyputerr(_("Nesting too deep (more than %d)"), MAXDEPTH);
			return(TRUE);
		}

		/* create a new tree branch */
		newtree = sdfi_alloclisptree();
		newtree->parameters = 0;
		newtree->paramtotal = 0;
		newtree->lineno = sdfi_lineno;

		/* add branch to previous branch */
		if (sdfi_addparameter(sdfi_treepos, PARAMBRANCH, newtree)) return(TRUE);

		/* add keyword */
		pt = &keyword[1];
		while (*pt == ' ' && *pt == '\t') pt++;
		if (allocstring(&newtree->keyword, pt, io_tool->cluster)) return(TRUE);

		/* push tree onto stack */
		sdfi_treestack[sdfi_treedepth] = sdfi_treepos;
		sdfi_treedepth++;
		sdfi_treepos = newtree;
		return(FALSE);
	}

	if (estrcmp(keyword, x_(")")) == 0)
	{
		/* pop tree stack */
		if (sdfi_treedepth <= 0)
		{
			ttyputerr(_("Too many close parenthesis"));
			return(TRUE);
		}
		sdfi_treedepth--;
		sdfi_treepos = sdfi_treestack[sdfi_treedepth];
		return(FALSE);
	}

	/* just add the atomic keyword */
	if (keyword[0] == '"' && keyword[estrlen(keyword)-1] == '"')
	{
		keyword++;
		keyword[estrlen(keyword)-1] = 0;
	}
	if (allocstring(&savekey, keyword, io_tool->cluster)) return(TRUE);
	if (sdfi_addparameter(sdfi_treepos, PARAMATOM, savekey)) return(TRUE);
	return(FALSE);
}

/*
 * Routine to add a parameter of type "type" and value "value" to the tree element "tree".
 * Returns true on memory error.
 */
BOOLEAN sdfi_addparameter(LISPTREE *tree, INTBIG type, void *value)
{
	REGISTER INTBIG *newparamtypes;
	REGISTER INTBIG i, newlimit;
	REGISTER void **newparamvalues;

	if (tree->parameters >= tree->paramtotal)
	{
		newlimit = tree->paramtotal * 2;
		if (newlimit <= 0)
		{
			/* intelligent determination of parameter needs */
			newlimit = 3;
		}
		newparamtypes = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, io_tool->cluster);
		if (newparamtypes == 0) return(TRUE);
		newparamvalues = (void **)emalloc(newlimit * (sizeof (void *)), io_tool->cluster);
		if (newparamvalues == 0) return(TRUE);
		for(i=0; i<tree->parameters; i++)
		{
			newparamtypes[i] = tree->paramtype[i];
			newparamvalues[i] = tree->paramvalue[i];
		}
		if (tree->paramtotal > 0)
		{
			efree((CHAR *)tree->paramtype);
			efree((CHAR *)tree->paramvalue);
		}
		tree->paramtype = newparamtypes;
		tree->paramvalue = newparamvalues;
		tree->paramtotal = newlimit;
	}
	tree->paramtype[tree->parameters] = type;
	tree->paramvalue[tree->parameters] = value;
	tree->parameters++;

	return(FALSE);
}

LISPTREE *sdfi_alloclisptree(void)
{
	LISPTREE *lt;

	if (sdfi_freelisptree != 0)
	{
		lt = sdfi_freelisptree;
		sdfi_freelisptree = (LISPTREE *)lt->paramvalue;
	} else
	{
		lt = (LISPTREE *)emalloc(sizeof (LISPTREE), io_tool->cluster);
		if (lt == 0) return(0);
	}
	return(lt);
}

void sdfi_killptree(LISPTREE *lt)
{
	REGISTER INTBIG i;

	if (lt->keyword != 0) efree(lt->keyword);
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] == PARAMBRANCH)
		{
			sdfi_killptree((LISPTREE *)lt->paramvalue[i]);
		} else
		{
			efree((CHAR *)lt->paramvalue[i]);
		}
	}
	if (lt->parameters > 0)
	{
		efree((CHAR *)lt->paramtype);
		efree((CHAR *)lt->paramvalue);
	}

	/* put it back on the free list */
	lt->paramvalue = (void **)sdfi_freelisptree;
	sdfi_freelisptree = lt;
}

/*
 * Routine to parse the entire tree and annotate current cell.
 */
BOOLEAN sdfi_parsetree(LISPTREE *lt)
{
	REGISTER INTBIG i, total;
	REGISTER LISPTREE *sublt;

	/* look through top level for "delayfile" */
	total = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		sublt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(sublt->keyword, x_("delayfile")) != 0) continue;

		/* create the primitive */
		if (sdfi_parsedelayfile(sublt)) return(TRUE);
		total++;
	}
	if (total == 0)
	{
		ttyputerr(_("No DELAYFILE definition found"));
		return(TRUE);
	}
	ttyputmsg(_("Found %ld DELAYFILE %s"), total, makeplural(_("definition"), total));
	return(FALSE);
}

/*
 * Routine to parse delayfile definition.
 */
BOOLEAN sdfi_parsedelayfile(LISPTREE *lt)
{
	REGISTER INTBIG i;
	REGISTER LISPTREE *scanlt, *ltsdfversion, *ltdesign, *ltdate, *ltvendor, *ltprogram,
		*ltversion, *ltdivider, *ltvoltage, *ltprocess, *lttemperature, *lttimescale;

	/* find all the pieces of delayfile */
	ltsdfversion = ltdesign = ltdate = ltvendor = ltprogram = ltversion = ltdivider = 0;
	ltvoltage = ltprocess = lttemperature = lttimescale = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("sdfversion")) == 0)
		{
			if (ltsdfversion != 0)
			{
				ttyputerr(_("Multiple 'sdfversion' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_sdfversion, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found SDFVERSION: %s"), sdfi_sdfversion);
			ltsdfversion = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("design")) == 0)
		{
			if (ltdesign != 0)
			{
				ttyputerr(_("Multiple 'design' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_design, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found DESIGN: %s"), sdfi_design);

			/* check to see if design matches current cell */
			if (namesame(sdfi_curcell->protoname, sdfi_design) != 0)
			{
				ttyputerr(_("Current cell (%s) does not match DESIGN (%s).  Aborting SDF input"),
					sdfi_curcell->protoname, sdfi_design);
				return(TRUE);
			}
			ltdesign = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("date")) == 0)
		{
			if (ltdate != 0)
			{
				ttyputerr(_("Multiple 'date' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_date, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found DATE: %s"), sdfi_date);
			ltdate = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("vendor")) == 0)
		{
			if (ltvendor != 0)
			{
				ttyputerr(_("Multiple 'vendor' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_vendor, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found VENDOR: %s"), sdfi_vendor);
			ltvendor = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("program")) == 0)
		{
			if (ltprogram != 0)
			{
				ttyputerr(_("Multiple 'program' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_program, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found PROGRAM: %s"), sdfi_program);
			ltprogram = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("version")) == 0)
		{
			if (ltversion != 0)
			{
				ttyputerr(_("Multiple 'version' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_version, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found VERSION: %s"), sdfi_version);
			ltversion = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("divider")) == 0)
		{
			if (ltdivider != 0)
			{
				ttyputerr(_("Multiple 'divider' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}

			if (io_verbose > 0) ttyputmsg(_("Found DIVIDER: %s"), (CHAR *)scanlt->paramvalue[0]);
			sdfi_setstring(&sdfi_divider, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found DIVIDER: %s"), sdfi_divider);
			if (namesame(sdfi_divider, x_(".")) != 0 && namesame(sdfi_divider, x_("/")) != 0)
			{
				ttyputerr(_("illegal DIVIDER - %s (line %ld)"), sdfi_divider, scanlt->lineno);
				return(TRUE);
			}
			ltdivider = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("voltage")) == 0)
		{
			if (ltvoltage != 0)
			{
				ttyputerr(_("Multiple 'voltage' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_voltage, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found VOLTAGE: %s"), sdfi_voltage);
			ltvoltage = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("process")) == 0)
		{
			if (ltprocess != 0)
			{
				ttyputerr(_("Multiple 'process' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_process, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found PROCESS: %s"), sdfi_process);
			ltprocess = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("temperature")) == 0)
		{
			if (lttemperature != 0)
			{
				ttyputerr(_("Multiple 'temperature' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_temperature, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found TEMPERATURE: %s"), sdfi_temperature);
			lttemperature = scanlt;
			continue;
		}

		if (namesame(scanlt->keyword, x_("timescale")) == 0)
		{
			if (lttimescale != 0)
			{
				ttyputerr(_("Multiple 'timescale' sections for a delayfile (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_timescale, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found TIMESCALE: %s"), sdfi_timescale);
			lttimescale = scanlt;
			if (sdfi_parsetimescale(scanlt)) return(TRUE);
			continue;
		}

		if (namesame(scanlt->keyword, x_("cell")) == 0)
		{
			if (sdfi_parsecell(scanlt)) return(TRUE);
		}
	}

	return(FALSE);
}

/*
 * Routine to parse timescale definition.
 */
BOOLEAN sdfi_parsetimescale(LISPTREE *lt)
{
	CHAR t1[200];

	if (lt->parameters == 1)
	{
		estrcpy(t1, (CHAR *)lt->paramvalue[0]);
	} else if (lt->parameters == 2)
	{
		estrcpy(t1, (CHAR *)lt->paramvalue[0]);
		estrcat(t1, (CHAR *)lt->paramvalue[1]);
	} else
	{
		ttyputerr(_("illegal TIMESCALE - too many parameters (line %ld)"), lt->lineno);
		return(TRUE);
	}

	if      (namesame(t1,   x_("1us")) == 0 || namesame(t1,   x_("1.0us")) == 0) sdfi_timeconvert = 1000000;
	else if (namesame(t1,  x_("10us")) == 0 || namesame(t1,  x_("10.0us")) == 0) sdfi_timeconvert = 10000000;
	else if (namesame(t1, x_("100us")) == 0 || namesame(t1, x_("100.0us")) == 0) sdfi_timeconvert = 100000000;
	else if (namesame(t1,   x_("1ns")) == 0 || namesame(t1,   x_("1.0ns")) == 0) sdfi_timeconvert = 1000;
	else if (namesame(t1,  x_("10ns")) == 0 || namesame(t1,  x_("10.0ns")) == 0) sdfi_timeconvert = 10000;
	else if (namesame(t1, x_("100ns")) == 0 || namesame(t1, x_("100.0ns")) == 0) sdfi_timeconvert = 100000;
	else if (namesame(t1,   x_("1ps")) == 0 || namesame(t1,   x_("1.0ps")) == 0) sdfi_timeconvert = 1;
	else if (namesame(t1,  x_("10ps")) == 0 || namesame(t1,  x_("10.0ps")) == 0) sdfi_timeconvert = 10;
	else if (namesame(t1, x_("100ps")) == 0 || namesame(t1, x_("100.0ps")) == 0) sdfi_timeconvert = 100;
	else
	{
		ttyputerr(_("illegal TIMESCALE - unknown unit or value (line %ld)"), lt->lineno);
		return(TRUE);
	}

	if (io_verbose > 0) ttyputmsg(_("timescale conversion = %ld"), sdfi_timeconvert);

	return(FALSE);
}

/*
 * Routine to parse cell definition.
 */
BOOLEAN sdfi_parsecell(LISPTREE *lt)
{
	REGISTER INTBIG i, j;
	INTBIG sl, len;
	REGISTER LISPTREE *scanlt, *ltcelltype, *ltinstance;
	NODEINST *ni;
	VARIABLE *var;
	void *sa;
	CHAR **sastr;
	REGISTER void *infstr;

	/* find all the pieces of cell */
	ltcelltype = ltinstance = 0;
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("celltype")) == 0)
		{
			if (ltcelltype != 0)
			{
				ttyputerr(_("Multiple 'celltype' sections for a cell (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_celltype, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found CELLTYPE: %s"), sdfi_celltype);
			ltcelltype = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("instance")) == 0)
		{
			if (ltinstance != 0)
			{
				ttyputerr(_("Multiple 'instance' sections for a cell (line %ld)"), scanlt->lineno);
				return(TRUE);
			}
			sdfi_setstring(&sdfi_instance, (CHAR *)scanlt->paramvalue[0]);
			if (io_verbose > 0) ttyputmsg(_("Found INSTANCE: %s"), sdfi_instance);

			ni = sdfi_getcellinstance(sdfi_celltype, sdfi_instance);

			/* store SDF instance name on node */
			if (ni != NONODEINST)
			{
				sa = newstringarray(io_tool->cluster);
				if (sa == 0) return(TRUE);
				infstr = initinfstr();
				addstringtoinfstr(infstr, sdfi_instance);
				addtostringarray(sa, returninfstr(infstr));
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING|VISARRAY, SDF_instance_key);
				if (var != NOVARIABLE)
				{
					len = getlength(var);
					for(j=0; j<len; j++) addtostringarray(sa, ((CHAR **)var->addr)[j]);
				}
				sastr = getstringarray(sa, &sl);
				(void)setvalkey((INTBIG)ni, VNODEINST, SDF_instance_key, (INTBIG)sastr,
					VSTRING|VISARRAY|(sl<<VLENGTHSH));
				killstringarray(sa);
			} else
			{
				ttyputerr(_("No matching node for CELLTYPE: %s, INSTANCE: %s. Aborting SDF input"),
					sdfi_celltype, sdfi_instance);
				return(TRUE);
			}

			ltinstance = scanlt;
			continue;
		}
		if (namesame(scanlt->keyword, x_("delay")) == 0)
		{
			if (sdfi_parsedelay(scanlt, ni)) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Routine to parse delay definition.
 */
BOOLEAN sdfi_parsedelay(LISPTREE *lt, NODEINST *ni)
{
	REGISTER INTBIG i;
	REGISTER LISPTREE *scanlt;

	/* find all the pieces of delay */
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("absolute")) == 0)
		{
			if (sdfi_parseabsolute(scanlt, ni)) return(TRUE);
		}
		if (namesame(scanlt->keyword, x_("increment")) == 0)
		{
			if (sdfi_parseincrement(scanlt, ni)) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * Routine to parse delay definition.
 */
BOOLEAN sdfi_parseabsolute(LISPTREE *lt, NODEINST *ni)
{
	REGISTER INTBIG i;
	REGISTER LISPTREE *scanlt;

	/* find all the pieces of absolute */
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("port")) == 0)
		{
			if (sdfi_parseport(scanlt, ni)) return(TRUE);
		}
		if (namesame(scanlt->keyword, x_("iopath")) == 0)
		{
			if (sdfi_parseiopath(scanlt, ni)) return(TRUE);
		}
	}

	return(FALSE);
}

/*
 * Routine to parse delay definition.
 */
BOOLEAN sdfi_parseincrement(LISPTREE *lt, NODEINST *ni)
{
	REGISTER INTBIG i;
	REGISTER LISPTREE *scanlt;

	/* find all the pieces of increment */
	for(i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH) continue;
		scanlt = (LISPTREE *)lt->paramvalue[i];
		if (namesame(scanlt->keyword, x_("port")) == 0)
		{
			if (sdfi_parseport(scanlt, ni)) return(TRUE);
		}
		if (namesame(scanlt->keyword, x_("iopath")) == 0)
		{
			if (sdfi_parseiopath(scanlt, ni)) return(TRUE);
		}
	}

	return(FALSE);
}

/*
 * Routine to parse port definition.
 */
BOOLEAN sdfi_parseport(LISPTREE *lt, NODEINST *ni)
{
	REGISTER INTBIG i, j, rvalues;
	INTBIG sl, len;
	REGISTER LISPTREE *scanlt;
	PORTARCINST *pi;
	VARIABLE *var;
	CHAR *portname, **sastr;
	void *sa;
	REGISTER void *infstr;

	/* rvalue_list for transition delays */
	struct
	{
		CHAR tname[3];
		CHAR *tvalue;
	} rvl[] =
	{
		{x_("01"), NULL},	/* 0->1 */
		{x_("10"), NULL},	/* 1->0 */
		{x_("0Z"), NULL},	/* 0->Z */
		{x_("Z1"), NULL},	/* Z->1 */
		{x_("1Z"), NULL},	/* 1->Z */
		{x_("Z0"), NULL},	/* Z->0 */
		{x_("0X"), NULL},	/* 0->X */
		{x_("X1"), NULL},	/* X->1 */
		{x_("1X"), NULL},	/* 1->X */
		{x_("X0"), NULL},	/* X->0 */
		{x_("XZ"), NULL},	/* X->Z */
		{x_("ZX"), NULL}	/* Z->X */
	};

	portname = 0;
	for(i=0; i<12; i++) rvl[i].tvalue = 0;

	/* find all the pieces of port */
	rvalues = lt->parameters - 1;
	for (i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH)
		{
			sdfi_setstring(&portname, (CHAR *)lt->paramvalue[i]);
		} else
		{
			scanlt = (LISPTREE *)lt->paramvalue[i];
			if (scanlt->parameters == 0)
			{
				if (rvalues == 6 || rvalues == 12)
				{	/* first six or all twelve transitions */
					sdfi_setstring(&rvl[i-1].tvalue, (CHAR *)scanlt->keyword);
				}
				else if (rvalues == 3)
				{
					if (i == 1)
					{	/* rising transitions */
						sdfi_setstring(&rvl[0].tvalue, (CHAR *)scanlt->keyword);	/* 0->1 */
						sdfi_setstring(&rvl[3].tvalue, (CHAR *)scanlt->keyword);	/* Z->1 */
					}
					if (i == 2)
					{	/* falling transitions */
						sdfi_setstring(&rvl[1].tvalue, (CHAR *)scanlt->keyword);	/* 1->0 */
						sdfi_setstring(&rvl[5].tvalue, (CHAR *)scanlt->keyword);	/* Z->0 */
					}
					if (i == 3)
					{	/* "Z" transitions */
						sdfi_setstring(&rvl[2].tvalue, (CHAR *)scanlt->keyword);	/* 0->Z */
						sdfi_setstring(&rvl[4].tvalue, (CHAR *)scanlt->keyword);	/* 1->Z */
					}
				} else if (rvalues == 2)
				{
					if (i == 1)
					{			/* "rising" delay */
						sdfi_setstring(&rvl[0].tvalue, (CHAR *)scanlt->keyword);	/* 0->1 */
						sdfi_setstring(&rvl[2].tvalue, (CHAR *)scanlt->keyword);	/* 0->Z */
						sdfi_setstring(&rvl[3].tvalue, (CHAR *)scanlt->keyword);	/* Z->1 */
					}
					if (i == 2)
					{			/* "falling" delay */
						sdfi_setstring(&rvl[1].tvalue, (CHAR *)scanlt->keyword);	/* 1->0 */
						sdfi_setstring(&rvl[4].tvalue, (CHAR *)scanlt->keyword);	/* 1->Z */
						sdfi_setstring(&rvl[5].tvalue, (CHAR *)scanlt->keyword);	/* Z->0 */
					}
				}
				else if (rvalues == 1)
				{	/* all twelve transitions */
					for (j=0; j<12; j++) sdfi_setstring(&rvl[j].tvalue, (CHAR *)scanlt->keyword);
				} else
				{						/* transitions in order */
					sdfi_setstring(&rvl[i-1].tvalue, (CHAR *)scanlt->keyword);
				}
			}
		}
	}

	if (io_verbose > 0)
	{
		ttyputmsg(_("PORT %s transition delay table"), portname);
		for (i=0; i<12; i++)
		{
			if (rvl[i].tvalue != NULL)
			{
				ttyputmsg(_("transition: %s, delay: %s"), rvl[i].tname, rvl[i].tvalue);
			}
		}
	}

	sa = newstringarray(io_tool->cluster);
	if (sa == 0) return(TRUE);

	/* create string of transition delay values */
	infstr = initinfstr();
	addstringtoinfstr(infstr, sdfi_hiername);
	for (i=0; i<12; i++)
	{
		if (rvl[i].tvalue != NULL)
		{
			addstringtoinfstr(infstr, x_(" "));
			addstringtoinfstr(infstr, rvl[i].tname);
			addstringtoinfstr(infstr, x_("("));
			addstringtoinfstr(infstr, sdfi_converttops(rvl[i].tvalue));
			addstringtoinfstr(infstr, x_(")"));
		}
	}
	addtostringarray(sa, returninfstr(infstr));

	/* find port and add/update SDF_absolute_port_delay string array with delay info */
	for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (estrcmp(pi->proto->protoname, portname) == 0)
		{
			var = getvalkey((INTBIG)pi, VPORTARCINST, VSTRING|VISARRAY, SDF_absolute_port_delay_key);
			if (var != NOVARIABLE)
			{
				len = getlength(var);
				for(i=0; i<len; i++) addtostringarray(sa, ((CHAR **)var->addr)[i]);
			}
			sastr = getstringarray(sa, &sl);
			(void)setvalkey((INTBIG)pi, VPORTARCINST, SDF_absolute_port_delay_key, (INTBIG)sastr,
				VSTRING|VISARRAY|(sl<<VLENGTHSH));
			break;
		}
	}
	killstringarray(sa);

	if (portname != 0) efree((CHAR *)portname);
	for(i=0; i<12; i++) if (rvl[i].tvalue != 0) efree((CHAR *)rvl[i].tvalue);
	return(FALSE);
}

/*
 * Routine to parse iopath definition.
 */
BOOLEAN sdfi_parseiopath(LISPTREE *lt, NODEINST *ni)
{
	REGISTER INTBIG i, j, rvalues;
	INTBIG sl, len;
	REGISTER LISPTREE *scanlt;
	VARIABLE *var;
	CHAR *iportname, *oportname, **sastr;
	void *sa;
	REGISTER void *infstr;

	/* rvalue_list for transition delays */
	struct
	{
		CHAR tname[3];
		CHAR *tvalue;
	} rvl[] =
	{
		{x_("01"), NULL},	/* 0->1 */
		{x_("10"), NULL},	/* 1->0 */
		{x_("0Z"), NULL},	/* 0->Z */
		{x_("Z1"), NULL},	/* Z->1 */
		{x_("1Z"), NULL},	/* 1->Z */
		{x_("Z0"), NULL},	/* Z->0 */
		{x_("0X"), NULL},	/* 0->X */
		{x_("X1"), NULL},	/* X->1 */
		{x_("1X"), NULL},	/* 1->X */
		{x_("X0"), NULL},	/* X->0 */
		{x_("XZ"), NULL},	/* X->Z */
		{x_("ZX"), NULL}	/* Z->X */
	};

	/* find all the pieces of port */
	iportname = oportname = 0;
	for(i=0; i<12; i++) rvl[i].tvalue = 0;

	rvalues = lt->parameters - 2;
	for (i=0; i<lt->parameters; i++)
	{
		if (lt->paramtype[i] != PARAMBRANCH)
		{
			if (i == 0) sdfi_setstring(&iportname, (CHAR *)lt->paramvalue[i]);
			if (i == 1) sdfi_setstring(&oportname, (CHAR *)lt->paramvalue[i]);
		} else
		{
			scanlt = (LISPTREE *)lt->paramvalue[i];
			if (scanlt->parameters == 0)
			{
				if (rvalues == 6 || rvalues == 12)
				{	/* first six or all twelve transitions */
					sdfi_setstring(&rvl[i-2].tvalue, (CHAR *)scanlt->keyword);
				} else if (rvalues == 3)
				{
					if (i == 2)
					{	/* rising transitions */
						sdfi_setstring(&rvl[0].tvalue, (CHAR *)scanlt->keyword);	/* 0->1 */
						sdfi_setstring(&rvl[3].tvalue, (CHAR *)scanlt->keyword);	/* Z->1 */
					}
					if (i == 3)
					{	/* falling transitions */
						sdfi_setstring(&rvl[1].tvalue, (CHAR *)scanlt->keyword);	/* 1->0 */
						sdfi_setstring(&rvl[5].tvalue, (CHAR *)scanlt->keyword);	/* Z->0 */
					}
					if (i == 4)
					{	/* "Z" transitions */
						sdfi_setstring(&rvl[2].tvalue, (CHAR *)scanlt->keyword);	/* 0->Z */
						sdfi_setstring(&rvl[4].tvalue, (CHAR *)scanlt->keyword);	/* 1->Z */
					}
				} else if (rvalues == 2)
				{
					if (i == 2)
					{	/* "rising" delay */
						sdfi_setstring(&rvl[0].tvalue, (CHAR *)scanlt->keyword);	/* 0->1 */
						sdfi_setstring(&rvl[2].tvalue, (CHAR *)scanlt->keyword);	/* 0->Z */
						sdfi_setstring(&rvl[3].tvalue, (CHAR *)scanlt->keyword);	/* Z->1 */
					}
					if (i == 3)
					{	/* "falling" delay */
						sdfi_setstring(&rvl[1].tvalue, (CHAR *)scanlt->keyword);	/* 1->0 */
						sdfi_setstring(&rvl[4].tvalue, (CHAR *)scanlt->keyword);	/* 1->Z */
						sdfi_setstring(&rvl[5].tvalue, (CHAR *)scanlt->keyword);	/* Z->0 */
					}
				} else if (rvalues == 1)
				{	/* all twelve transitions */
					for (j=0; j<12; j++) sdfi_setstring(&rvl[j].tvalue, (CHAR *)scanlt->keyword);
				} else
				{	/* transitions in order */
					sdfi_setstring(&rvl[i-2].tvalue, (CHAR *)scanlt->keyword);
				}
			}
		}
	}

	if (io_verbose > 0)
	{
		ttyputmsg(_("IOPATH %s -> %s transition delay table"), iportname, oportname);
		for (i=0; i<12; i++)
		{
			if (rvl[i].tvalue != NULL)
			{
				ttyputmsg(_("transition: %s, delay: %s"), rvl[i].tname, rvl[i].tvalue);
			}
		}
	}

	sa = newstringarray(io_tool->cluster);
	if (sa == 0) return(TRUE);

	/* create string of transition delay values */
	infstr = initinfstr();
	addstringtoinfstr(infstr, sdfi_hiername);
	addstringtoinfstr(infstr, x_(" "));
	addstringtoinfstr(infstr, iportname);
	addstringtoinfstr(infstr, x_(":"));
	addstringtoinfstr(infstr, oportname);
	for (i=0; i<12; i++)
	{
		if (rvl[i].tvalue != NULL)
		{
			addstringtoinfstr(infstr, x_(" "));
			addstringtoinfstr(infstr, rvl[i].tname);
			addstringtoinfstr(infstr, x_("("));
			addstringtoinfstr(infstr, sdfi_converttops(rvl[i].tvalue));
			addstringtoinfstr(infstr, x_(")"));
		}
	}
	addtostringarray(sa, returninfstr(infstr));

	/* find port and add/update SDF_absolute_iopath_delay string array with delay info */
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING|VISARRAY, SDF_absolute_iopath_delay_key);
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(i=0; i<len; i++) addtostringarray(sa, ((CHAR **)var->addr)[i]);
	}
	sastr = getstringarray(sa, &sl);
	(void)setvalkey((INTBIG)ni, VNODEINST, SDF_absolute_iopath_delay_key, (INTBIG)sastr,
		VSTRING|VISARRAY|(sl<<VLENGTHSH));
	killstringarray(sa);

	if (iportname != 0) efree((CHAR *)iportname);
	if (oportname != 0) efree((CHAR *)oportname);
	for(i=0; i<12; i++) if (rvl[i].tvalue != 0) efree((CHAR *)rvl[i].tvalue);
	return(FALSE);
}

/*
 * Routine to get a NODEINST for specified cell instance.
 */
NODEINST *sdfi_getcellinstance(CHAR *celltype, CHAR *instance)
{
	NODEINST *ni;
	VARIABLE *var;
	CHAR *pt, **instlist, *str, tmp[256];
	INTBIG i, count = 1;
	REGISTER void *infstr;

	ni = NONODEINST;

	/* at top level of hierarchy */
	if (estrstr(instance, sdfi_divider) == NULL)
	{
		for(ni = sdfi_curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var != NOVARIABLE)
			{
				if (estrcmp((CHAR *)var->addr, instance) == 0) break;
			}
		}
		if (var != NOVARIABLE)
		{
			infstr = initinfstr();
#ifndef USEALSNAMES
			addstringtoinfstr(infstr, x_(":"));
#else
			addstringtoinfstr(infstr, x_("."));
			addstringtoinfstr(infstr, sdfi_curcell->protoname);
			addstringtoinfstr(infstr, x_("."));
			addstringtoinfstr(infstr, (CHAR *)var->addr);
#endif
			sdfi_setstring(&sdfi_hiername, returninfstr(infstr));
		}
		return(ni);
	}

	/* count number of hierarchy levels */
	(void)esnprintf(tmp, 256, x_("%s"), instance);
	if (namesame(sdfi_divider, x_(".")) == 0) for (pt = tmp; *pt != 0; pt++) if (*pt == '.') count++;
	if (namesame(sdfi_divider, x_("/")) == 0) for (pt = tmp; *pt != 0; pt++) if (*pt == '/') count++;

	/* separate out each hiearchy level */
	instlist = (CHAR **)emalloc(count * (sizeof(CHAR *)), io_tool->cluster);
	pt = instance;
	for (i=0; i<count; i++)
	{
		str = getkeyword(&pt, sdfi_divider);
		if (allocstring(&instlist[i], str, io_tool->cluster)) return(NONODEINST);
		(void)tonextchar(&pt);
	}

	/* find the NODEINST corresponding to bottom level of hierarchy */
	i = 0;
	for(ni = sdfi_curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE)
		{
			if (estrcmp((CHAR *)var->addr, instlist[i]) == 0)
			{
				if (++i == count) break;
				ni = ni->proto->firstnodeinst;
			}
		}
	}

	/* create hierarchy name - don't include last part, which is NODE_name of NODEINST */
	infstr = initinfstr();
#ifndef USEALSNAMES
	addstringtoinfstr(infstr, x_(":"));
#else
	addstringtoinfstr(infstr, x_("."));
	addstringtoinfstr(infstr, sdfi_curcell->protoname);
	addstringtoinfstr(infstr, x_("."));
#endif
	for (i=0; i<count-1; i++)
	{
		addstringtoinfstr(infstr, instlist[i]);
#ifndef USEALSNAMES
		if (i != (count - 2)) addstringtoinfstr(infstr, x_(":"));
#else
		if (i != (count - 2)) addstringtoinfstr(infstr, x_("."));
#endif
	}
#ifdef USEALSNAMES
	addstringtoinfstr(infstr, x_("."));
	addstringtoinfstr(infstr, (CHAR *)var->addr);
#endif
	sdfi_setstring(&sdfi_hiername, returninfstr(infstr));

	return(ni);
}

/*
 * Routine to convert time value string to picoseconds.
 */
CHAR *sdfi_converttops(CHAR *tstring)
{
	INTBIG d1, d2, d3;
	INTBIG i;
	CHAR *pt, *str, t1[100], t2[100], t3[100];
	static CHAR ts[256];

	/* time value is not a triple */
	if (estrstr(tstring, x_(":")) == NULL)
	{
		if (estrstr(tstring, x_(".")) == NULL) d1 = eatoi(tstring) * sdfi_timeconvert;
			else d1 = (INTBIG)(eatof(tstring) * (double)sdfi_timeconvert);
		(void)esnprintf(ts, 256, x_("%ld"), d1);
		return(ts);
	}

	/* time value is a triple */
	pt = tstring;
	for (i=0; i<3; i++)
	{
		str = getkeyword(&pt, x_(":"));
		if (i == 0) estrcpy(t1, str);
		if (i == 1) estrcpy(t2, str);
		if (i == 2) estrcpy(t3, str);
		(void)tonextchar(&pt);
	}

	if (estrstr(t1, x_(".")) == NULL) d1 = eatoi(t1) * sdfi_timeconvert;
		else d1 = (INTBIG)(eatof(t1) * (double)sdfi_timeconvert);

	if (estrstr(t2, x_(".")) == NULL) d2 = eatoi(t2) * sdfi_timeconvert;
		else d2 = (INTBIG)(eatof(t2) * (double)sdfi_timeconvert);

	if (estrstr(t3, x_(".")) == NULL) d3 = eatoi(t3) * sdfi_timeconvert;
		else d3 = (INTBIG)(eatof(t3) * (double)sdfi_timeconvert);

	(void)esnprintf(ts, 256, x_("%ld:%ld:%ld"), d1, d2, d3);
	return(ts);
}

void sdfi_setstring(CHAR **string, CHAR *value)
{
	if (*string == 0)
		(void)allocstring(string, value, io_tool->cluster); else
			(void)reallocstring(string, value, io_tool->cluster);
}

void sdfi_clearglobals(void)
{
	if (sdfi_design != NULL)      efree((CHAR *)sdfi_design);       sdfi_design = NULL;
	if (sdfi_sdfversion != NULL)  efree((CHAR *)sdfi_sdfversion);   sdfi_sdfversion = NULL;
	if (sdfi_date != NULL)        efree((CHAR *)sdfi_date);         sdfi_date = NULL;
	if (sdfi_vendor != NULL)      efree((CHAR *)sdfi_vendor);       sdfi_vendor = NULL;
	if (sdfi_program != NULL)     efree((CHAR *)sdfi_program);      sdfi_program = NULL;
	if (sdfi_version != NULL)     efree((CHAR *)sdfi_version);      sdfi_version = NULL;
	if (sdfi_voltage != NULL)     efree((CHAR *)sdfi_voltage);      sdfi_voltage = NULL;
	if (sdfi_process != NULL)     efree((CHAR *)sdfi_process);      sdfi_process = NULL;
	if (sdfi_temperature != NULL) efree((CHAR *)sdfi_temperature);  sdfi_temperature = NULL;
	if (sdfi_timescale != NULL)   efree((CHAR *)sdfi_timescale);    sdfi_timescale = NULL;
	if (sdfi_hiername  != NULL)   efree((CHAR *)sdfi_hiername);     sdfi_hiername = NULL;
	if (sdfi_celltype != NULL)    efree((CHAR *)sdfi_celltype);     sdfi_celltype = NULL;
	if (sdfi_instance != NULL)    efree((CHAR *)sdfi_instance);     sdfi_instance = NULL;
	if (sdfi_divider != NULL)     efree((CHAR *)sdfi_divider);      sdfi_divider = NULL;
}
