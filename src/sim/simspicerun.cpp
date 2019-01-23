/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: simspicerun.cpp
 * Companion file to simspice.cpp
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
#if SIMTOOL

#include "global.h"
#include "sim.h"
#include "efunction.h"
#include "usr.h"
#include "network.h"
#include "edialogs.h"
#include <math.h>
#ifdef WIN32
#include <float.h>
#endif

#define SP_BUF_SZ 1024

/******************** SPICE NUMBER OUTPUT QUEUE ********************/

#define NONUMBERS  ((NUMBERS *)-1)

typedef struct Inumbers
{
	double time;
	float *list;
	INTBIG count;
	struct Inumbers *nextnumbers;
} NUMBERS;

static NUMBERS *sim_spice_numbersfree = NONUMBERS;

/******************** HSPICE NAME ASSOCIATIONS ********************/

#define NOPA0LINE ((PA0LINE *)-1)

typedef struct Ipa0line
{
	INTBIG   number;
	CHAR    *string;
	struct Ipa0line *nextpa0line;
} PA0LINE;

static PA0LINE *sim_pa0linefree = NOPA0LINE;
static PA0LINE *sim_pa0linefirst = NOPA0LINE;


static CHAR	    sim_spice_cellname[100] = {x_("")};		/* Name extracted from .SPO file */
static FILE    *sim_spice_streamfromsim;
static EProcess *sim_spice_process;
static NUMBERS *sim_spice_numbers = NONUMBERS;		/* current list */
static CHAR   **sim_spice_signames;
static INTSML  *sim_spice_sigtypes;
static double  *sim_spice_time;
static float   *sim_spice_val;
static INTBIG   sim_spice_signals = 0;				/* entries in "signames/sigtypes" */
static INTBIG   sim_spice_iter;
static INTBIG   sim_spice_limit = 0;				/* entries in "time/val" */
static INTBIG   sim_spice_filepos;
static INTBIG   sim_spice_filelen;
static BOOLEAN  sim_spice_tr0binary;				/* true if tr0 file is binary */
static UCHAR1    sim_spice_btr0buf[8192];			/* buffer for binary tr0 data */
static INTBIG   sim_spice_btr0size;					/* size of binary tr0 buffer */
static INTBIG   sim_spice_btr0pos;					/* position in binary tr0 buffer */

/* prototypes for local routines */
static NUMBERS *sim_spice_allocnumbers(INTBIG);
static void     sim_spice_freenumbers(NUMBERS*);
static void     sim_spice_terminate(FILE*, CHAR*, CHAR*);
static INTSML   sim_spice_getfromsimulator(void);
static BOOLEAN  sim_spice_getlinefromsimulator(CHAR *line);
static BOOLEAN  sim_spice_topofsignals(CHAR**);
static CHAR    *sim_spice_nextsignals(void);
static BOOLEAN  sim_spice_plotvalues(NODEPROTO *np);
static BOOLEAN  sim_spice_ensurespace(INTBIG);
static INTBIG   sim_spice_parseoutput(INTBIG running, FILE *outputfile, INTBIG parsemode, void *dia);
static INTBIG   sim_spice_findsignalname(CHAR *name);
static BOOLEAN  sim_spice_charhandlerschem(WINDOWPART *w, INTSML chr, INTBIG special);
static BOOLEAN  sim_spice_charhandlerwave(WINDOWPART *w, INTSML chr, INTBIG special);
static BOOLEAN  sim_spice_readbtr0block(BOOLEAN firstbyteread);
static void     sim_spice_resetbtr0(void);
static INTBIG   sim_spice_gethspicefloat(double *val);
static PA0LINE *sim_allocpa0line(void);
static void     sim_freepa0line(PA0LINE *pl);
static void     sim_spice_removesignal(void);
#ifdef WIN32
static double   sim_spice_swapendian(double v);
#endif
static void     sim_spice_addarguments(EProcess &process, CHAR *deckname, CHAR *cellname);

/*
 * Routine to free all memory allocated by this module.
 */
void sim_freespicerun_memory(void)
{
	REGISTER PA0LINE *pl;

	while (sim_pa0linefirst != NOPA0LINE)
	{
		pl = sim_pa0linefirst;
		sim_pa0linefirst = pl->nextpa0line;
		sim_freepa0line(pl);
	}
	while (sim_pa0linefree != NOPA0LINE)
	{
		pl = sim_pa0linefree;
		sim_pa0linefree = pl->nextpa0line;
		efree((CHAR *)pl);
	}
}

/*
 * routine to allocate a new numbers module from the pool (if any) or memory
 */
NUMBERS *sim_spice_allocnumbers(INTBIG total)
{
	REGISTER NUMBERS *d;

	if (sim_spice_numbersfree == NONUMBERS)
	{
		d = (NUMBERS *)emalloc(sizeof (NUMBERS), sim_tool->cluster);
		if (d == 0) return(NONUMBERS);
	} else
	{
		d = sim_spice_numbersfree;
		sim_spice_numbersfree = (NUMBERS *)d->nextnumbers;
	}

	d->list = (float *)emalloc(((sizeof (float)) * total), sim_tool->cluster);
	if (d->list == 0) return(NONUMBERS);
	d->count = total;
	return(d);
}

/*
 * routine to return numbers module "d" to the pool of free modules
 */
void sim_spice_freenumbers(NUMBERS *d)
{
	efree((CHAR *)d->list);
	d->nextnumbers = sim_spice_numbersfree;
	sim_spice_numbersfree = d;
}

/******************** SPICE EXECUTION AND PLOTTING ********************/

#define MAXTRACES   7
#define MAXLINE   200

PA0LINE *sim_allocpa0line(void)
{
	REGISTER PA0LINE *pl;

	if (sim_pa0linefree == NOPA0LINE)
	{
		pl = (PA0LINE *)emalloc(sizeof (PA0LINE), sim_tool->cluster);
		if (pl == 0) return(NOPA0LINE);
	} else
	{
		pl = sim_pa0linefree;
		sim_pa0linefree = pl->nextpa0line;
	}
	pl->string = 0;
	return(pl);
}

void sim_freepa0line(PA0LINE *pl)
{
	if (pl->string != 0) efree((CHAR *)pl->string);
	pl->nextpa0line = sim_pa0linefree;
	sim_pa0linefree = pl;
}

/*
 * routine to execute SPICE with "infile" as the input deck file, "outfile"
 * as the output listing file.  If "infile" is null, do not run the simulator,
 * but just read "outfile" as the output listing.  If "infile" is not null,
 * run SPICE on that file, presuming that it came from cell "cell".
 */
void sim_spice_execute(CHAR *infile, CHAR *outfile, NODEPROTO *cell)
{
	CHAR line[MAXLINE+5];
	REGISTER INTBIG i, important, parsemode, filetype, simstate;
	NODEPROTO *np;
	FILE *io;
	NUMBERS *num;
	REGISTER FILE *outputfile;
	REGISTER PA0LINE *pl;
	VARIABLE *var;
	CHAR *host, *path, *name, *truename, *pa0file, *pa0truefile, *pt;
	REGISTER void *infstr, *dia;
	EProcess process;

	/* determine execution/parsing mode */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_dontrunkey);
	if (var == NOVARIABLE) parsemode = SIMRUNNO; else
		parsemode = var->addr;

	/*
	 * Determine the "sim_spice_state".  If we're doing a write-execute-parse,
	 * then we've already done this.  If we're just doing a parse, we have not.
	 */
	var = getvalkey((INTBIG)sim_tool, VTOOL, VINTEGER, sim_spice_statekey);
	if (var != NOVARIABLE) sim_spice_state = var->addr; else sim_spice_state = 0;

	outputfile = NULL;
	sim_spice_filelen = -1;
	if (*infile == 0)
	{
		/* parsing output into simulation window: make sure none is active */
		simstate = sim_window_isactive(&np);
		if ((simstate&SIMWINDOWWAVEFORM) != 0)
		{
#ifndef ACCUMALATE_TRACES
			ttyputerr(_("Only one simulation window can run at a time"));
			ttyputmsg(_("Terminate the current one before reading SPICE output"));
			return;
#endif
		}
		if ((simstate&SIMWINDOWSCHEMATIC) != 0)
			sim_window_stopsimulation();

		/* no input deck, simply use output listing */
		filetype = sim_filetypespiceout;
		switch (sim_spice_state & SPICEOUTPUT)
		{
			case SPICEOUTPUTNORM:
				switch (sim_spice_state & SPICETYPE)
				{
					case SPICE2:          filetype = sim_filetypespiceout;    break;
					case SPICE3:          filetype = sim_filetypespiceout;    break;
					case SPICEHSPICE:     filetype = sim_filetypehspiceout;   break;
					case SPICEPSPICE:     filetype = sim_filetypespiceout;    break;
					case SPICEGNUCAP:     filetype = sim_filetypespiceout;    break;
					case SPICESMARTSPICE: filetype = sim_filetypespiceout;    break;
				}
				break;
			case SPICEOUTPUTRAW:      filetype = sim_filetyperawspiceout;  break;
			case SPICEOUTPUTRAWSMART: filetype = sim_filetypesrawspiceout; break;
		}
		sim_spice_streamfromsim = xopen(truepath(outfile), filetype, x_(""), &truename);
		if (sim_spice_streamfromsim == NULL)
		{
			ttyputerr(_("Cannot read %s"), truename);
			return;
		}
		sim_spice_filelen = filesize(sim_spice_streamfromsim);

		/* for HSPICE, get the ".pa0" file with extra name information */
		if ((sim_spice_state & SPICETYPE) == SPICEHSPICE)
		{
			/* free all existing "pa0" information */
			while (sim_pa0linefirst != NOPA0LINE)
			{
				pl = sim_pa0linefirst;
				sim_pa0linefirst = pl->nextpa0line;
				sim_freepa0line(pl);
			}

			infstr = initinfstr();
			addstringtoinfstr(infstr, truepath(outfile));
			pa0file = returninfstr(infstr);
			i = estrlen(pa0file) - 4;
			if (namesame(&pa0file[i], x_(".tr0")) == 0)
				estrcpy(&pa0file[i], x_(".pa0"));
			io = xopen(pa0file, el_filetypetext, 0, &pa0truefile);
			if (io != NULL)
			{
				for(;;)
				{
					if (xfgets(line, MAXLINE, io)) break;
					pt = line;
					while (*pt == ' ') pt++;
					if (*pt == 0) continue;
					pl = sim_allocpa0line();
					if (pl == NOPA0LINE) break;
					pl->number = eatoi(pt);
					while (*pt != ' ' && *pt != 0) pt++;
					while (*pt == ' ') pt++;
					(void)allocstring(&pl->string, pt, sim_tool->cluster);
					for(pt = pl->string; *pt != 0; pt++)
						if (*pt == ' ') break;
					*pt = 0;
					pl->nextpa0line = sim_pa0linefirst;
					sim_pa0linefirst = pl;
				}
				xclose(io);
			}
		}
	} else
	{
		/* input deck exists, run SPICE on it */
		sim_spice_streamfromsim = 0;

		switch (sim_spice_state & SPICETYPE)
		{
			case SPICE2:          name = x_("spice");      break;
			case SPICE3:          name = x_("spice3");     break;
			case SPICEHSPICE:     name = x_("hspice");     break;
			case SPICEPSPICE:     name = x_("pspice");     break;
			case SPICEGNUCAP:     name = x_("gnucap");     break;
			case SPICESMARTSPICE: name = x_("smartspice"); break;
		}
		path = egetenv(x_("ELECTRIC_SPICELOC"));
		if (path == NULL) path = SPICELOC;
		host = egetenv(x_("ELECTRIC_REMOTESPICEHOST"));
		if (host != NULL)
		{
			/* spice on a remote machine */
			process.addArgument( x_("/usr/ucb/rsh") );
			process.addArgument( host );
			process.addArgument( path );
		} else
		{
			/* spice on the local machine */
			process.addArgument( path );
		}
		sim_spice_addarguments(process, infile, cell->protoname);

		BOOLEAN quiet = (parsemode == SIMRUNYESQPARSE || parsemode == SIMRUNYESQ) &&
			(sim_spice_state & SPICETYPE) == SPICEHSPICE;
		process.setCommunication( FALSE, !quiet, quiet );
		if (!process.start( infile ))
			ttyputerr("Can't run SPICE");
		if (parsemode == SIMRUNYESQ && (sim_spice_state & SPICETYPE) == SPICEHSPICE)
			parsemode = SIMRUNYES;

		/* prepare output file if requested */
		if (*outfile != 0)
		{
			CHAR *truename;

			outputfile = xcreate(outfile, sim_filetypespiceout, _("SPICE Output File"), &truename);
			if (outputfile == NULL)
			{
				if (truename != 0) ttyputerr(_("Cannot write SPICE output file %s"), truename);
			}
		}
	}

	/* free any former data */
	if (sim_spice_signals > 0)
	{
		for(i=0; i<sim_spice_signals; i++) efree(sim_spice_signames[i]);
		efree((CHAR *)sim_spice_signames);
		efree((CHAR *)sim_spice_sigtypes);
		sim_spice_signals = 0;
	}
	while (sim_spice_numbers != NONUMBERS)
	{
		num = sim_spice_numbers;
		sim_spice_numbers = sim_spice_numbers->nextnumbers;
		sim_spice_freenumbers(num);
	}

	/* show progress if possible */
	dia = 0;
	if (sim_spice_filelen > 0)
	{
		dia = DiaInitProgress(0, 0);
		if (dia == 0) return;
		DiaSetProgress(dia, 0, sim_spice_filelen);
	}
	sim_spice_filepos = 0;

	/* if executing and not parsing, simply echo execution progress */
	sim_spice_process = &process;
	if ((parsemode == SIMRUNYES || parsemode == SIMRUNYESQ) && *infile != 0)
	{
		for(;;)
		{
			if (sim_spice_getlinefromsimulator(line)) break;
			if (stopping(STOPREASONSPICE)) break;
			if (parsemode == SIMRUNYESQ) continue;
			ttyputmsg(x_("%s"), line);
		}
		important = -1;
	} else
	{
		important = sim_spice_parseoutput(*infile != 0, outputfile, parsemode, dia);
	}
	sim_spice_process = 0;

	/* clean up */
	if (sim_spice_filelen > 0) DiaDoneProgress(dia);
	(void)sim_spice_terminate(outputfile, outfile, infile);
	if (*infile != 0)
	{
		process.kill();
		ttyputmsg(_("SPICE execution complete"));
	}
	if (stopping(STOPREASONSPICE)) return;

	/* plot the list of numbers */
	if (important < 0) return;
	if (sim_spice_plotvalues(cell)) ttyputerr(_("Problem making the plot"));
}

/*
 * Routine to add execution parameters to the array "sim_spice_execpars" starting at index "index".
 * The name of the cell is in "cellname".
 */
void sim_spice_addarguments(EProcess &process, CHAR *deckname, CHAR *cellname)
{
	REGISTER VARIABLE *var;
	REGISTER CHAR *runargs;
	REGISTER void *infstr;

	var = getvalkey((INTBIG)sim_tool, VTOOL, VSTRING, sim_spice_runargskey);
	if (var == NOVARIABLE)
	{
		process.addArgument( deckname );
	} else
	{
		/* parse the arguments */
		runargs = (CHAR *)var->addr;
		for(;;)
		{
			infstr = initinfstr();
			for( ; *runargs != 0 && *runargs != ' '; runargs++)
			{
				if (*runargs == '%') addstringtoinfstr(infstr, cellname); else
					addtoinfstr(infstr, *runargs);
			}
			process.addArgument( returninfstr(infstr) );
			while (*runargs == ' ') runargs++;
			if (*runargs == 0) break;
		}
	}
}

/*
 * Parse the SPICE output file.  If "running" is nonzero, echo commands (send them
 * to "outputfile" if it is valid).
 * Returns the number of important traces in the data (-1 on error).
 */
INTBIG sim_spice_parseoutput(INTBIG running, FILE *outputfile, INTBIG parsemode, void *dia)
{
	CHAR line[MAXLINE+5], *ptr, *start;
	REGISTER INTBIG i, j, k, l, important, nodcnt, numnoi, rowcount, retval,
		multiplier;
	BOOLEAN past_end, first, datamode, knows;
	float *numbers;
	INTBIG numbers_limit;
	double *inputbuffer;
	REGISTER PA0LINE *pl;
	double lastt, time, value;
	NUMBERS *num, *numend;
	REGISTER void *infstr;

	/* parse SPICE rawfile output */
	if ((sim_spice_state & SPICEOUTPUT) == SPICEOUTPUTRAW)
	{
		first = TRUE;
		numend = NONUMBERS;
		sim_spice_signals = -1;
		rowcount = -1;
		for(;;)
		{
			if (stopping(STOPREASONSPICE)) return(-1);
			if (sim_spice_getlinefromsimulator(line)) break;
			if (sim_spice_filelen > 0)
				DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);
			if (first)
			{
				/* check the first line for HSPICE format possibility */
				first = FALSE;
				if (estrlen(line) >= 20 && line[16] == '9' && line[17] == '0' &&
					line[18] == '0' && line[19] == '7')
				{
					ttyputerr(_("This is an HSPICE file, not a RAWFILE file"));
					ttyputerr(_("Change the SPICE format and reread"));
					return(-1);
				}
			}

			if (running != 0)
			{
				if (outputfile == NULL)
				{
					if (parsemode == SIMRUNYESPARSE) ttyputmsg(x_("%s"), line);
				} else sim_spice_xprintf(outputfile, FALSE, x_("%s\n"), line);
			}

			/* find the ":" separator */
			for(ptr = line; *ptr != 0; ptr++) if (*ptr == ':') break;
			if (*ptr == 0) continue;
			*ptr++ = 0;
			while (*ptr == ' ' || *ptr == '\t') ptr++;

			if (namesame(line, x_("Plotname")) == 0)
			{
				if (sim_spice_cellname[0] == '\0')
					estrcpy(sim_spice_cellname, ptr);
				continue;
			}

			if (namesame(line, x_("No. Variables")) == 0)
			{
				sim_spice_signals = eatoi(ptr) - 1;
				continue;
			}

			if (namesame(line, x_("No. Points")) == 0)
			{
				rowcount = eatoi(ptr);
				continue;
			}

			if (namesame(line, x_("Variables")) == 0)
			{
				if (sim_spice_signals < 0)
				{
					ttyputerr(_("Missing variable count in file"));
					return(-1);
				}
				sim_spice_signames = (CHAR **)emalloc(sim_spice_signals * (sizeof (CHAR *)),
					sim_tool->cluster);
				if (sim_spice_signames == 0) return(-1);
				for(i=0; i<=sim_spice_signals; i++)
				{
					if (stopping(STOPREASONSPICE)) return(-1);
					if (sim_spice_getlinefromsimulator(line))
					{
						ttyputerr(_("Error: end of file during signal names"));
						return(-1);
					}
					ptr = line;
					while (*ptr == ' ' || *ptr == '\t') ptr++;
					if (myatoi(ptr) != i)
						ttyputerr(_("Warning: Variable %ld has number %ld"),
							i, myatoi(ptr));
					while (*ptr != 0 && *ptr != ' ' && *ptr != '\t') ptr++;
					while (*ptr == ' ' || *ptr == '\t') ptr++;
					start = ptr;
					while (*ptr != 0 && *ptr != ' ' && *ptr != '\t') ptr++;
					*ptr = 0;
					if (i == 0)
					{
						if (namesame(start, x_("time")) != 0)
							ttyputerr(_("Warning: the first variable should be time, is '%s'"),
								start);
					} else
					{
						(void)allocstring(&sim_spice_signames[i-1], start, sim_tool->cluster);
					}
				}
				continue;
			}
			if (namesame(line, x_("Values")) == 0)
			{
				if (sim_spice_signals < 0)
				{
					ttyputerr(_("Missing variable count in file"));
					return(-1);
				}
				if (rowcount < 0)
				{
					ttyputerr(_("Missing point count in file"));
					return(-1);
				}
				for(j=0; j<rowcount; j++)
				{
					num = sim_spice_allocnumbers(sim_spice_signals);
					if (num == NONUMBERS) return(-1);
					if (numend == NONUMBERS) sim_spice_numbers = num; else
						numend->nextnumbers = num;
					num->nextnumbers = NONUMBERS;
					numend = num;

					if (sim_spice_filelen > 0)
						DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

					for(i=0; i<=sim_spice_signals; i++)
					{
						if (stopping(STOPREASONSPICE)) return(-1);
						if (sim_spice_getlinefromsimulator(line))
						{
							ttyputerr(_("Error: end of file during data points (read %ld out of %ld)"),
								j, rowcount);
							return(-1);
						}
						ptr = line;
						if (i == 0)
						{
							if (myatoi(line) != j)
								ttyputerr(_("Warning: data point %ld has number %ld"),
									j, myatoi(line));
							while(*ptr != 0 && *ptr != ' ' && *ptr != '\t') ptr++;
						}
						while (*ptr == ' ' || *ptr == '\t') ptr++;
						if (i == 0) num->time = eatof(ptr); else
							num->list[i-1] = (float)eatof(ptr);
					}
				}
			}
			if (namesame(line, x_("Binary")) == 0)
			{
				if (sim_spice_signals < 0)
				{
					ttyputerr(_("Missing variable count in file"));
					return(-1);
				}
				if (rowcount < 0)
				{
					ttyputerr(_("Missing point count in file"));
					return(-1);
				}
				inputbuffer = (double *)emalloc(sim_spice_signals * (sizeof (double)), sim_tool->cluster);
				if (inputbuffer == 0) return(-1);
 
				/* read the data */
				for(j=0; j<rowcount; j++)
				{
					num = sim_spice_allocnumbers(sim_spice_signals);
					if (num == NONUMBERS) return(-1);
					if (numend == NONUMBERS) sim_spice_numbers = num; else
						numend->nextnumbers = num;
					num->nextnumbers = NONUMBERS;
					numend = num;

					if (sim_spice_filelen > 0 )
						DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

					if (stopping(STOPREASONSPICE)) return(-1);
					i = xfread((UCHAR1 *)(&num->time), sizeof(double), 1, sim_spice_streamfromsim);
					if (i != 1)
					{
						ttyputerr(_("Error: end of file during data points (read %ld out of %ld)"),
							j, rowcount);
						return(-1);
					}
					i = xfread((UCHAR1 *)inputbuffer, sizeof(double), sim_spice_signals, sim_spice_streamfromsim);
					if (i != sim_spice_signals)
					{
						ttyputerr(_("Error: end of file during data points (read %ld out of %ld)"),
							j, rowcount);
						return(-1);
					}
					for(i=0; i<sim_spice_signals; i++)
						num->list[i] = (float)inputbuffer[i];
				}
				efree((CHAR *)inputbuffer);
			}
		}

		if (sim_spice_signals > 0)
		{
			sim_spice_sigtypes = (INTSML *)emalloc(sim_spice_signals * SIZEOFINTSML, sim_tool->cluster);
			if (sim_spice_sigtypes == 0) return(-1);
		}
		return(sim_spice_signals);
	}

	/* parse SmartSPICE rawfile output */
	if ((sim_spice_state & SPICEOUTPUT) == SPICEOUTPUTRAWSMART)
	{
		first = TRUE;
		numend = NONUMBERS;
		sim_spice_signals = -1;
		rowcount = -1;
		for(;;)
		{
			if (stopping(STOPREASONSPICE)) return(-1);
			if (sim_spice_getlinefromsimulator(line)) break;
			if (sim_spice_filelen > 0)
				DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);
			if (first)
			{
				/* check the first line for HSPICE format possibility */
				first = FALSE;
				if (estrlen(line) >= 20 && line[16] == '9' && line[17] == '0' &&
					line[18] == '0' && line[19] == '7')
				{
					ttyputerr(_("This is an HSPICE file, not a RAWFILE file"));
					ttyputerr(_("Change the SPICE format and reread"));
					return(-1);
				}
			}

			if (running != 0)
			{
				if (outputfile == NULL)
				{
					if (parsemode == SIMRUNYESPARSE) ttyputmsg(x_("%s"), line);
				} else sim_spice_xprintf(outputfile, FALSE, x_("%s\n"), line);
			}

			/* find the ":" separator */
			for(ptr = line; *ptr != 0; ptr++) if (*ptr == ':') break;
			if (*ptr == 0) continue;
			*ptr++ = 0;
			while (*ptr == ' ' || *ptr == '\t') ptr++;

			if (namesame(line, x_("Plotname")) == 0)
			{
				if (sim_spice_cellname[0] == '\0')
					estrcpy(sim_spice_cellname, ptr);
				continue;
			}

			if (namesame(line, x_("No. Variables")) == 0)
			{
				sim_spice_signals = eatoi(ptr) - 1;
				continue;
			}

			if (namesame(line, x_("No. Points")) == 0)
			{
				rowcount = eatoi(ptr);
				continue;
			}

			if (namesame(line, x_("Variables")) == 0)
			{
				if (sim_spice_signals < 0)
				{
					ttyputerr(_("Missing variable count in file"));
					return(-1);
				}
				sim_spice_signames = (CHAR **)emalloc(sim_spice_signals * (sizeof (CHAR *)),
					sim_tool->cluster);
				if (sim_spice_signames == 0) return(-1);
				for(i=0; i<=sim_spice_signals; i++)
				{
					if (stopping(STOPREASONSPICE)) return(-1);
					if (i != 0)
					{
						if (sim_spice_getlinefromsimulator(line))
						{
							ttyputerr(_("Error: end of file during signal names"));
							return(-1);
						}
						ptr = line;
					}
					while (*ptr == ' ' || *ptr == '\t') ptr++;
					if (myatoi(ptr) != i)
						ttyputerr(_("Warning: Variable %ld has number %ld"),
							i, myatoi(ptr));
					while (*ptr != 0 && *ptr != ' ' && *ptr != '\t') ptr++;
					while (*ptr == ' ' || *ptr == '\t') ptr++;
					start = ptr;
					while (*ptr != 0 && *ptr != ' ' && *ptr != '\t') ptr++;
					*ptr = 0;
					if (i == 0)
					{
						if (namesame(start, x_("time")) != 0)
							ttyputerr(_("Warning: the first variable should be time, is '%s'"),
								start);
					} else
					{
						(void)allocstring(&sim_spice_signames[i-1], start, sim_tool->cluster);
					}
				}
				continue;
			}
			if (namesame(line, x_("Values")) == 0)
			{
				if (sim_spice_signals < 0)
				{
					ttyputerr(_("Missing variable count in file"));
					return(-1);
				}
				if (rowcount < 0)
				{
					ttyputerr(_("Missing point count in file"));
					return(-1);
				}
				for(j=0; j<rowcount; j++)
				{
					num = sim_spice_allocnumbers(sim_spice_signals);
					if (num == NONUMBERS) return(-1);
					if (numend == NONUMBERS) sim_spice_numbers = num; else
						numend->nextnumbers = num;
					num->nextnumbers = NONUMBERS;
					numend = num;

					if (sim_spice_filelen > 0)
						DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

					for(i=0; i<=sim_spice_signals; i++)
					{
						if (stopping(STOPREASONSPICE)) return(-1);
						if (sim_spice_getlinefromsimulator(line))
						{
							ttyputerr(_("Error: end of file during data points (read %ld out of %ld)"),
								j, rowcount);
							return(-1);
						}
						ptr = line;
						if (i == 0)
						{
							if (myatoi(line) != j)
								ttyputerr(_("Warning: data point %ld has number %ld"),
									j, myatoi(line));
							while(*ptr != 0 && *ptr != ' ' && *ptr != '\t') ptr++;
						}
						while (*ptr == ' ' || *ptr == '\t') ptr++;
						if (i == 0) num->time = eatof(ptr); else
							num->list[i-1] = (float)eatof(ptr);
					}
				}
			}
			if (namesame(line, x_("Binary")) == 0)
			{
				if (sim_spice_signals < 0)
				{
					ttyputerr(_("Missing variable count in file"));
					return(-1);
				}
				if (rowcount < 0)
				{
					ttyputerr(_("Missing point count in file"));
					return(-1);
				}
				inputbuffer = (double *)emalloc(sim_spice_signals * (sizeof (double)), sim_tool->cluster);
				if (inputbuffer == 0) return(-1);

				/* read the data */
				for(j=0; j<rowcount; j++)
				{
					num = sim_spice_allocnumbers(sim_spice_signals);
					if (num == NONUMBERS) return(-1);
					if (numend == NONUMBERS) sim_spice_numbers = num; else
						numend->nextnumbers = num;
					num->nextnumbers = NONUMBERS;
					numend = num;

					if (sim_spice_filelen > 0 )
						DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

					if (stopping(STOPREASONSPICE)) return(-1);
					i = xfread((UCHAR1 *)(&num->time), sizeof(double), 1, sim_spice_streamfromsim);
					if (i != 1)
					{
						ttyputerr(_("Error: end of file during data points (read %ld out of %ld)"),
							j, rowcount);
						return(-1);
					}
#ifdef WIN32
					num->time = sim_spice_swapendian(num->time);
					if (_finite(num->time) == 0) num->time = 0.0;
#endif
					i = xfread((UCHAR1 *)inputbuffer, sizeof(double), sim_spice_signals, sim_spice_streamfromsim);
					if (i != sim_spice_signals)
					{
						ttyputerr(_("Error: end of file during data points (read only %ld of %ld signals on row %ld out of %ld)"),
							i, sim_spice_signals, j, rowcount);
						return(-1);
					}
					for(i=0; i<sim_spice_signals; i++)
					{
#if defined(_BSD_SOURCE) || defined(_GNU_SOURCE)
						if (isinf(inputbuffer[i]) != 0) inputbuffer[i] = 0.0; else
							if (isnan(inputbuffer[i]) != 0) inputbuffer[i] = 0.0;
#endif
#ifdef WIN32
						/* convert to little-endian */
						inputbuffer[i] = sim_spice_swapendian(inputbuffer[i]);
						if (_finite(inputbuffer[i]) == 0) inputbuffer[i] = 0.0;
#endif
						num->list[i] = (float)inputbuffer[i];
					}
				}
				efree((CHAR *)inputbuffer);
			}
		}

		if (sim_spice_signals > 0)
		{
			sim_spice_sigtypes = (INTSML *)emalloc(sim_spice_signals * SIZEOFINTSML, sim_tool->cluster);
			if (sim_spice_sigtypes == 0) return(-1);
		}
		return(sim_spice_signals);
	}

	/* handle SPICE 2 format output */
	if ((sim_spice_state & SPICETYPE) == SPICE2 || (sim_spice_state & SPICETYPE) == SPICEGNUCAP)
	{
		datamode = FALSE;
		first = TRUE;
		numend = NONUMBERS;
		past_end = FALSE;
		sim_spice_signames = 0;
		numbers_limit = 256;
		numbers =  (float*)emalloc(numbers_limit * (sizeof (float)), sim_tool->cluster);
		if (numbers == NULL)
		{
			ttyputnomemory();
			return(-1);
		}
		for(;;)
		{
			if (stopping(STOPREASONSPICE)) break;
			if (sim_spice_getlinefromsimulator(line)) break;
			if (sim_spice_filelen > 0)
				DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);
			if (first)
			{
				/* check the first line for HSPICE format possibility */
				first = FALSE;
				if (estrlen(line) >= 20 && line[16] == '9' && line[17] == '0' &&
					line[18] == '0' && line[19] == '7')
				{
					ttyputerr(_("This is an HSPICE file, not a SPICE2 or Gnucap file"));
					ttyputerr(_("Change the SPICE format and reread"));
					efree((CHAR*)numbers);
					return(-1);
				}
			}
			if (running != 0)
			{
				if (outputfile == NULL)
				{
					if (parsemode == SIMRUNYESPARSE)
						ttyputmsg(x_("%s"), (isprint(*line) ? line : &line[1]));
				} else sim_spice_xprintf(outputfile, FALSE, x_("%s\n"), line);
			}

			/* look for a cell name */
			if ((sim_spice_cellname[0] == '\0') && (namesamen(line, x_("*** SPICE deck for cell "),25) == 0))
				if ((i = esscanf(line+25,x_("%s"),sim_spice_cellname)) != 1)
					sim_spice_cellname[0] = '\0';
			if (namesamen(line, x_(".END"), 4) == 0 && namesamen(line, x_(".ENDS"), 5) != 0)
			{
				past_end = TRUE;
				continue;
			}
			if (namesamen(line, x_("#Time"), 5) == 0)
			{
				sim_spice_signals = 0;
				ptr = line + 5;
				for(;;)
				{
					while (isspace(*ptr)) ptr++;
					if (*ptr == 0) break;
					CHAR *pt = ptr;
					while (!isspace(*pt)) pt++;
					CHAR save = *pt;
					*pt = 0;
					CHAR **newsignames = (CHAR **)emalloc((sim_spice_signals + 1) * (sizeof (CHAR *)), sim_tool->cluster);
					for (i = 0; i < sim_spice_signals; i++) newsignames[i] = sim_spice_signames[i];
					(void)allocstring(&newsignames[sim_spice_signals], ptr, sim_tool->cluster);
					if (sim_spice_signames != 0) efree((CHAR*)sim_spice_signames);
					sim_spice_signames = newsignames;
					sim_spice_signals++;
					*pt = save;
					ptr = pt;
					while (*ptr != ' ' && *ptr != 0) ptr++;
				}
				if (sim_spice_signals == sim_spice_printlistlen)
				{
					for (i = 0; i < sim_spice_signals; i++)
					{
						efree(sim_spice_signames[i]);
						(void)allocstring(&sim_spice_signames[i], sim_spice_printlist[i], sim_tool->cluster);
					}
				}
				past_end = TRUE;
				continue;
			}
			if (past_end && !datamode)
			{
				if ((isspace(line[0]) || line[0] == '-') && isdigit(line[1]))
					datamode = TRUE;
			}
			if (past_end && datamode)
			{
				if (!((isspace(line[0]) || line[0] == '-') && isdigit(line[1])))
				{
					datamode = FALSE;
					past_end = FALSE;
				}
			}
			if (datamode)
			{
				ptr = line;
				sim_spice_signals = 0;
				for(;;)
				{
					while (isspace(*ptr)) ptr++;
					if (*ptr == 0) break;
					CHAR *pt = ptr;
					while (isalnum(*pt) || *pt == '.' || *pt == '+' || *pt == '-') pt++;
					CHAR save = *pt;
					*pt = 0;
					if (sim_spice_signals >= numbers_limit)
					{
						float *newnumbers =  (float*)emalloc(numbers_limit * 2 * (sizeof (float)), sim_tool->cluster);
						if (newnumbers == NULL)
						{
							ttyputnomemory();
							return(-1);
						}
						for (i = 0; i < sim_spice_signals; i++) newnumbers[i] = numbers[i];
						efree((CHAR*)numbers);
						numbers = newnumbers;
						numbers_limit *= 2;

					}
					numbers[sim_spice_signals++] = (float)figureunits(ptr, VTUNITSNONE, 0);
					*pt = save;
					ptr = pt;
					while (*ptr != ' ' && *ptr != 0) ptr++;
				}
				if (sim_spice_signals > 1)
				{
					sim_spice_signals--;
					num = sim_spice_allocnumbers(sim_spice_signals);
					if (num == NONUMBERS) break;
					num->time = numbers[0];
					for(i=0; i<sim_spice_signals; i++) num->list[i] = numbers[i+1];
					if (numend == NONUMBERS) sim_spice_numbers = num; else
						numend->nextnumbers = num;
					num->nextnumbers = NONUMBERS;
					numend = num;
				}
			}
		}
		efree((CHAR*)numbers);

		/* generate dummy names */
		if (sim_spice_signames == 0)
		{
			sim_spice_signames = (CHAR **)emalloc(sim_spice_signals * (sizeof (CHAR *)), sim_tool->cluster);
			if (sim_spice_signames == 0)
			{
				/* terminate execution so we can restart simulator */
				return(-1);
			}
			for(i=0; i<sim_spice_signals; i++)
			{
				(void)esnprintf(line, MAXLINE+5, x_("Signal %ld"), i+1);
				(void)allocstring(&sim_spice_signames[i], line, sim_tool->cluster);
			}
		}
		sim_spice_sigtypes = (INTSML *)emalloc(sim_spice_signals * SIZEOFINTSML, sim_tool->cluster);
		if (sim_spice_sigtypes == 0)
		{
			/* terminate execution so we can restart simulator */
			return(-1);
		}
		return(sim_spice_signals);
	}
	
	/* handle SPICE 3 / PSPICE format output */
	if ((sim_spice_state & SPICETYPE) == SPICE3 ||
		(sim_spice_state & SPICETYPE) == SPICEPSPICE)
	{
		numend = NONUMBERS;
		first = TRUE;
		knows = FALSE;
		for(;;)
		{
			if (stopping(STOPREASONSPICE)) break;
			if (sim_spice_getlinefromsimulator(line)) break;
			if (sim_spice_filelen > 0)
				DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

			if (first)
			{
				/* check the first line for HSPICE format possibility */
				first = FALSE;
				if (estrlen(line) >= 20 && line[16] == '9' && line[17] == '0' &&
					line[18] == '0' && line[19] == '7')
				{
					ttyputerr(_("This is an HSPICE file, not a SPICE3/PSPICE file"));
					ttyputerr(_("Change the SPICE format and reread"));
					return(-1);
				}
			}

			/* skip first word if there is an "=" in the line */
			for(ptr = line; *ptr != 0; ptr++) if (*ptr == '=') break;
			if (*ptr == 0) ptr = line; else ptr += 3;

			/* read the data values */
			lastt = 0.0;
			for(;;)
			{
				while (*ptr == ' ' || *ptr == '\t') ptr++;
				if (*ptr == 0 || *ptr == ')') break;
				if (sim_spice_signals == 0)
				{
					num = sim_spice_allocnumbers(MAXTRACES);
					num->time = eatof(ptr);
					if (numend == NONUMBERS) sim_spice_numbers = num; else
					{
						numend->nextnumbers = num;
						if (num->time <= lastt && !knows)
						{
							ttyputerr(_("First trace (should be 'time') is not increasing in value"));
							knows = TRUE;
						}
					}
					lastt = num->time;
					num->nextnumbers = NONUMBERS;
					numend = num;
				} else
				{
					if (num == NONUMBERS) ttyputmsg(_("Line %ld of data has too many values"),
						sim_spice_signals); else
					{
						if (sim_spice_signals <= MAXTRACES)
							num->list[sim_spice_signals-1] = (float)eatof(ptr);
						num = num->nextnumbers;
					}
				}
				while (*ptr != ' ' && *ptr != '\t' && *ptr != 0) ptr++;
			}

			/* see if there is an ")" at the end of the line */
			if (line[estrlen(line)-1] == ')')
			{
				/* advance to the next value for subsequent reads */
				if (sim_spice_signals != 0 && num != NONUMBERS)
					ttyputmsg(_("Line %ld of data has too few values"), sim_spice_signals);
				sim_spice_signals++;
				num = sim_spice_numbers;
			}

			if (running != 0)
			{
				if (outputfile == NULL)
				{
					if (parsemode == SIMRUNYESPARSE)
						ttyputmsg(x_("%s"), (isprint(*line) ? line : &line[1]));
				} else sim_spice_xprintf(outputfile, FALSE, x_("%s\n"), line);
			}
		}

		/* generate dummy names */
		sim_spice_signames = (CHAR **)emalloc(sim_spice_signals * (sizeof (CHAR *)), sim_tool->cluster);
		sim_spice_sigtypes = (INTSML *)emalloc(sim_spice_signals * SIZEOFINTSML, sim_tool->cluster);
		if (sim_spice_signames == 0 || sim_spice_sigtypes == 0)
		{
			/* terminate execution so we can restart simulator */
			return(-1);
		}
		for(i=0; i<sim_spice_signals; i++)
		{
			(void)esnprintf(line, MAXLINE+5, x_("Signal %ld"), i+1);
			(void)allocstring(&sim_spice_signames[i], line, sim_tool->cluster);
		}
		return(sim_spice_signals);
	}
	
	/* handle HSPICE format output */
	if ((sim_spice_state & SPICETYPE) == SPICEHSPICE)
	{
		/* get number of nodes, special items, and conditions */
		line[4] = 0;
		for(j=0; j<4; j++) line[j] = (CHAR)sim_spice_getfromsimulator();
		nodcnt = eatoi(line);
		for(j=0; j<4; j++) line[j] = (CHAR)sim_spice_getfromsimulator();
		numnoi = eatoi(line);
		for(j=0; j<4; j++) line[j] = (CHAR)sim_spice_getfromsimulator();
		/* cndcnt = atoi(line); */

		/*
		 * Although this isn't documented anywhere, it appears that the 4th
		 * value on the line is a multiplier for the first, which allows
		 * there to be more than 10000 nodes.
		 */
		for(j=0; j<4; j++) line[j] = (CHAR)sim_spice_getfromsimulator();
		multiplier = eatoi(line);
		if (multiplier != 0) nodcnt += multiplier * 10000;

		sim_spice_signals = numnoi + nodcnt - 1;

		/* get version number (known to work with 9007, 9601) */
		for(j=0; j<4; j++) line[j] = (CHAR)sim_spice_getfromsimulator();
		j = eatoi(line);

		/* ignore the unused/title information (4+72 characters over line break) */
		for(j=0; j<76; j++)
		{
			k = sim_spice_getfromsimulator();
			if (!sim_spice_tr0binary && k == '\n') j--;
		}

		/* ignore the date/time information (16 characters) */
		for(j=0; j<16; j++) (void)sim_spice_getfromsimulator();

		/* ignore the copywrite information (72 characters over line break) */
		for(j=0; j<72; j++)
		{
			k = sim_spice_getfromsimulator();
			if (!sim_spice_tr0binary && k == '\n') j--;
		}

		/* get number of sweeps */
		line[4] = 0;
		for(j=0; j<4; j++) line[j] = (CHAR)sim_spice_getfromsimulator();
		/* sweepcnt = atoi(line); */

		/* ignore the Monte Carlo information (76 characters over line break) */
		for(j=0; j<76; j++)
		{
			k = sim_spice_getfromsimulator();
			if (!sim_spice_tr0binary && k == '\n') j--;
		}
		if (sim_spice_filelen > 0) DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

		/* get the type of each signal */
		important = numnoi;
		sim_spice_signames = (CHAR **)emalloc(sim_spice_signals * (sizeof (CHAR *)), sim_tool->cluster);
		sim_spice_sigtypes = (INTSML *)emalloc(sim_spice_signals * SIZEOFINTSML, sim_tool->cluster);
		if (sim_spice_signames == 0 || sim_spice_sigtypes == 0)
		{
			/* terminate execution so we can restart simulator */
			return(-1);
		}
		line[8] = 0;
		for(k=0; k<=sim_spice_signals; k++)
		{
			for(j=0; j<8; j++)
			{
				l = sim_spice_getfromsimulator();
				line[j] = (CHAR)l;
				if (!sim_spice_tr0binary && l == '\n') j--;
			}
			if (k == 0) continue;
			if (k < nodcnt) l = k + numnoi - 1; else l = k - nodcnt;
			sim_spice_sigtypes[l] = eatoi(line);
		}
		line[16] = 0;
		for(k=0; k<=sim_spice_signals; k++)
		{
			j = 0;
			for(;;)
			{
				l = sim_spice_getfromsimulator();
				if (l == '\n') continue;
				if (l == ' ') break;
				line[j++] = (CHAR)l;
				if (j >= 16) break;
				if (j >= MAXLINE) break;
			}
			line[j] = 0;
			l = (j+15) / 16 * 16 - 1;
			for(; j<l; j++)
			{
				i = sim_spice_getfromsimulator();
				if (!sim_spice_tr0binary && i == '\n') { j--;   continue; }
			}
			if (k == 0) continue;

			/* convert name if there is a colon in it */
			for(j=0; line[j] != 0; j++)
			{
				if (line[j] == ':') break;
				if (!isdigit(line[j])) break;
			}
			if (line[j] == ':')
			{
				l = eatoi(line);
				for(pl = sim_pa0linefirst; pl != NOPA0LINE; pl = pl->nextpa0line)
					if (l == pl->number) break;
				if (pl != NOPA0LINE)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, pl->string);
					addstringtoinfstr(infstr, &line[j+1]);
					estrcpy(line, returninfstr(infstr));
				}
			}

			if (k < nodcnt) l = k + numnoi - 1; else l = k - nodcnt;
			(void)allocstring(&sim_spice_signames[l], line, sim_tool->cluster);
		}

		if (!sim_spice_tr0binary)
		{
			/* finish line, ensure the end-of-header */
			for(j=0; ; j++)
			{
				l = sim_spice_getfromsimulator();
				if (l == '\n') break;
				if (j < 4) line[j] = (CHAR)l;
			}
		} else
		{
			/* gather end-of-header string */
			for(j=0; j<4; j++)
				line[j] = (CHAR)sim_spice_getfromsimulator();
		}
		line[4] = 0;
		if (estrcmp(line, x_("$&%#")) != 0)
		{
			ttyputerr(_("HSPICE header improperly terminated"));
			return(-1);
		}
		sim_spice_resetbtr0();

		/* now read the data */
		sim_spice_numbers = numend = NONUMBERS;
		for(;;)
		{
			if (stopping(STOPREASONSPICE)) break;
			if (sim_spice_filelen > 0) DiaSetProgress(dia, sim_spice_filepos, sim_spice_filelen);

			/* get the first number, see if it terminates */
			retval = sim_spice_gethspicefloat(&time);
			if (retval > 0)
			{
				if (sim_spice_tr0binary) break;
				ttyputerr(_("EOF too soon"));
				return(0);
			}
			if (retval < 0) break;

			/* get a row of numbers */
			num = NONUMBERS;
			for(k=1; k<=sim_spice_signals; k++)
			{
				retval = sim_spice_gethspicefloat(&value);
				if (retval > 0)
				{
					if (sim_spice_tr0binary && k == 1) break;
					ttyputerr(_("EOF too soon"));
					return(0);
				}
				if (retval < 0) break;
				if (k == 1)
				{
					/* first valid data point: allocate the structure */
					num = sim_spice_allocnumbers(sim_spice_signals);
					if (numend == NONUMBERS) sim_spice_numbers = num; else
						numend->nextnumbers = num;
					num->nextnumbers = NONUMBERS;
					numend = num;
					num->time = time;
				}
				if (k < nodcnt) l = k + numnoi - 1; else l = k - nodcnt;
				num->list[l] = (float)value;
			}
			if (num == NONUMBERS) break;
		}
		return(important);
	}

	/* unknown format */
	return(-1);
}

/*
 * Routine to read the next floating point number from the HSPICE file into "val".
 * Returns positive on error, negative on EOF, zero if OK.
 */
INTBIG sim_spice_gethspicefloat(double *val)
{
	CHAR line[12];
	REGISTER INTBIG i, j, l;
	union
	{
		UCHAR1 c[4];
		float x;
	} cast;

	if (!sim_spice_tr0binary)
	{
		line[11] = 0;
		for(j=0; j<11; j++)
		{
			l = sim_spice_getfromsimulator();
			if (l == EOF) return(1);
			line[j] = (CHAR)l;
			if (l == '\n') j--;
		}
		if (estrcmp(line, x_("0.10000E+31")) == 0) return(-1);
		*val = eatof(line);
		return(0);
	}

	/* binary format */
	for(i=0; i<4; i++)
	{
		l = sim_spice_getfromsimulator();
		if (l == EOF) return(1);
#ifdef BYTES_SWAPPED
		cast.c[i] = (UCHAR1)l;
#else
		cast.c[3-i] = (UCHAR1)l;
#endif
	}
	*val = cast.x;
	if (*val == 0.10000E+31) return(-1);
	return(0);
}

#ifdef WIN32
/*
 * Routine to swap the bytes in a double value.
 */
double sim_spice_swapendian(double v)
{
	union
	{
		UCHAR1 ch[8];
		double f;
	} cast;
	UCHAR1 swapbyte;

	cast.f = v;
	swapbyte = cast.ch[7]; cast.ch[7] = cast.ch[0]; cast.ch[0] = swapbyte;
	swapbyte = cast.ch[6]; cast.ch[6] = cast.ch[1]; cast.ch[1] = swapbyte;
	swapbyte = cast.ch[5]; cast.ch[5] = cast.ch[2]; cast.ch[2] = swapbyte;
	swapbyte = cast.ch[4]; cast.ch[4] = cast.ch[3]; cast.ch[3] = swapbyte;
	return(cast.f);
}
#endif

/* terminate SPICE execution */
void sim_spice_terminate(FILE *outputfile, CHAR *outfile, CHAR *infile)
{
	Q_UNUSED( infile );
	if (sim_spice_streamfromsim != 0) xclose(sim_spice_streamfromsim);

	if (outputfile != NULL)
	{
		xclose(outputfile);
		ttyputverbose(M_("%s written"), outfile);
	}
}

/*
 * Routine to reset the binary tr0 block pointer (done between the header and
 * the data).
 */
void sim_spice_resetbtr0(void)
{
	sim_spice_btr0size = 0;
	sim_spice_btr0pos = 0;
}

/*
 * Routine to read the next block of tr0 data.  Skips the first byte if "firstbyteread"
 * is true.  Returns true on EOF.
 */
BOOLEAN sim_spice_readbtr0block(BOOLEAN firstbyteread)
{
	UCHAR1 val;
	UCHAR1 uval;
	REGISTER INTBIG i, amtread, blocks, bytes, trailer;

	/* read the first word of a binary tr0 block */
	if (!firstbyteread)
	{
		if (xfread(&val, 1, 1, sim_spice_streamfromsim) != 1) return(TRUE);
		sim_spice_filepos++;
	}
	for(i=0; i<3; i++)
		if (xfread(&val, 1, 1, sim_spice_streamfromsim) != 1) return(TRUE);

	/* read the number of 8-byte blocks */
	blocks = 0;
	for(i=0; i<4; i++)
	{
		if (xfread(&uval, 1, 1, sim_spice_streamfromsim) != 1) return(TRUE);
		blocks = (blocks << 8) | uval;
	}

	/* skip the dummy word */
	for(i=0; i<4; i++)
		if (xfread(&val, 1, 1, sim_spice_streamfromsim) != 1) return(TRUE);

	/* read the number of bytes */
	bytes = 0;
	for(i=0; i<4; i++)
	{
		if (xfread(&uval, 1, 1, sim_spice_streamfromsim) != 1) return(TRUE);
		bytes = (bytes << 8) | uval;
	}

	/* now read the data */
	amtread = xfread(sim_spice_btr0buf, 1, bytes, sim_spice_streamfromsim);
	if (amtread != bytes) return(TRUE);

	/* read the trailer count */
	trailer = 0;
	for(i=0; i<4; i++)
	{
		if (xfread(&uval, 1, 1, sim_spice_streamfromsim) != 1) return(TRUE);
		trailer = (trailer << 8) | uval;
	}
	if (trailer != bytes) return(TRUE);

	/* set pointers for the buffer */
	sim_spice_btr0pos = 0;
	sim_spice_btr0size= bytes;
	sim_spice_filepos += 19 + bytes;
	return(FALSE);
}

/*
 * Routine to get the next character from the simulator (file or pipe).
 * Returns EOF at end of file.
 */
INTSML sim_spice_getfromsimulator(void)
{
	INTBIG i;
	UCHAR1 val;
#ifdef USE_BUFFER
	static UCHAR1 buf[SP_BUF_SZ];
	static UCHAR1 *bufp = buf;
	static int n=0;
#endif

	if (sim_spice_streamfromsim != 0)
	{
		if ((sim_spice_state & SPICETYPE) == SPICEHSPICE)
		{
			if (sim_spice_filepos == 0)
			{
				/* start of HSPICE file: see if it is binary or ascii */
				i = xfread(&val, 1, 1, sim_spice_streamfromsim);
				if (i != 1) return(EOF);
				sim_spice_filepos++;
				if (val == 0)
				{
					sim_spice_tr0binary = TRUE;
					if (sim_spice_readbtr0block(TRUE)) return(EOF);
				} else
				{
					sim_spice_tr0binary = FALSE;
					return(val);
				}
			}
			if (sim_spice_tr0binary)
			{
				if (sim_spice_btr0pos >= sim_spice_btr0size)
				{
					if (sim_spice_readbtr0block(FALSE))
						return(EOF);
				}
				val = sim_spice_btr0buf[sim_spice_btr0pos];
				sim_spice_btr0pos++;
				return(val&0xFF);
			}
		}
#ifdef USE_BUFFER
		if (n == 0)
		{
			n = xfread(buf, 1, SP_BUF_SZ, sim_spice_streamfromsim);
			bufp = buf;
		}

		if (--n >=0)
		{
			val = *bufp++;
			i = 1;
		} else i = 0;
#else
		i = xfread(&val, 1, 1, sim_spice_streamfromsim);
#endif
	} else
	{
		return(sim_spice_process->getChar());
	}
	if (i != 1) return(EOF);
	sim_spice_filepos++;
	return(val);
}

/*
 * Routine to get the next line of text from the simulator (file or pipe).
 * Returns true at end of file.
 */
BOOLEAN sim_spice_getlinefromsimulator(CHAR *line)
{
	REGISTER CHAR *ptr;
	REGISTER INTSML ch;

	ptr = line;
	for(;;)
	{
		if (stopping(STOPREASONSPICE)) break;
		ch = sim_spice_getfromsimulator();
		if (ch == EOF) return(TRUE);
		if (ch == '\n' || ch == '\r' || ptr - line >= MAXLINE) break;
		*ptr++ = (CHAR)ch;
	}
	*ptr = 0;
	return(FALSE);
}

BOOLEAN sim_spice_topofsignals(CHAR **c)
{
	Q_UNUSED( c );
	sim_spice_iter = 0;
	return(TRUE);
}

CHAR *sim_spice_nextsignals(void)
{
	CHAR *nextname;

	if (sim_spice_iter >= sim_spice_signals) return(0);
	nextname = sim_spice_signames[sim_spice_iter];
	sim_spice_iter++;
	return(nextname);
}

static COMCOMP sim_spice_picktrace = {NOKEYWORD, sim_spice_topofsignals,
	sim_spice_nextsignals, NOPARAMS, 0, x_(" \t"), N_("pick a signal to display"), x_("")};

extern "C" { extern COMCOMP us_yesnop, us_showdp; }

/*
 * routine to display the numbers in "sim_spice_numbers".  For each entry of the
 * linked list is another time sample, with "sim_spice_signals" trace points.  The
 * names of these signals is in "sim_spice_signames". Returns true on error.
 */
BOOLEAN sim_spice_plotvalues(NODEPROTO *np)
{
	REGISTER INTBIG i, numtotal, j, k, tr, position, numtraces;
	INTBIG oldsigcount;
	double min, max;
	CHAR *pars[3], **oldsignames, *pt, *start;
	REGISTER NODEPROTO *plotnp;
	REGISTER VARIABLE *var;
	REGISTER NUMBERS *num;
	void *sa;
	REGISTER void *infstr;

	/* count the number of values */
	for(num = sim_spice_numbers, numtotal=0; num != NONUMBERS; num = num->nextnumbers)
		numtotal++;
	if (numtotal == 0) return(FALSE);

	/* we have to establish a link for the plot cell */
	plotnp = np;

	/* if we're already simulating a cell, use that one */
	if (sim_simnt != NONODEPROTO) plotnp = sim_simnt; else
		if (sim_spice_cellname[0] != '\0') plotnp = getnodeproto(sim_spice_cellname);
	if (plotnp != NONODEPROTO && plotnp->primindex != 0) plotnp = NONODEPROTO;
	if (plotnp != NONODEPROTO && np == NONODEPROTO)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Is this a simulation of cell %s?"),
			describenodeproto(plotnp));
		i = ttygetparam(returninfstr(infstr), &us_yesnop, 3, pars);
		if (i == 1)
		{
			if (pars[0][0] == 'n') plotnp = NONODEPROTO;
		}
	}

	/* one final chance to pick a cell */
	if (plotnp == NONODEPROTO)
	{
		i = ttygetparam(_("Please select a cell to associate with this plot: "),
			&us_showdp, 3, pars);
		if (i > 0) plotnp = getnodeproto(pars[0]);
	}
	if (plotnp == NONODEPROTO) return(TRUE);

	/* get memory for the values */
	if (sim_spice_ensurespace(numtotal)) return(TRUE);

	j = 0;
	for(num = sim_spice_numbers; num != NONUMBERS; num = num->nextnumbers)
	{
		sim_spice_time[j++] = num->time;
	}

	/* find former signal order */
	oldsigcount = 0;
	numtraces = 1;
	var = getvalkey((INTBIG)plotnp, VNODEPROTO, VSTRING|VISARRAY, sim_window_signalorder_key);
	if (var != NOVARIABLE)
	{
		oldsigcount = getlength(var);
		if (oldsigcount > 0)
		{
			sa = newstringarray(sim_tool->cluster);
			for(i=0; i<oldsigcount; i++)
			{
				pt = ((CHAR **)var->addr)[i];
				addtostringarray(sa, pt);

				start = pt;
				if (*pt == '-') pt++;
				for( ; *pt != 0; pt++)
					if (!isdigit(*pt) || *pt == ':') break;
				if (*pt != ':') continue;
				{
					position = eatoi(start) + 1;
					if (position > numtraces) numtraces = position;
				}
			}
			oldsignames = getstringarray(sa, &oldsigcount);
		}
	}

	/* make the simulation window */
	if (sim_window_create(numtraces, plotnp, sim_spice_charhandlerwave,
		sim_spice_charhandlerschem, SPICE)) return(TRUE);
	sim_window_state = (sim_window_state & ~SIMENGINECUR) | SIMENGINECURSPICE;

	for (i = 0; i<sim_spice_signals; i++)
	{
		if (namesame(sim_spice_signames[i], x_("vdd")) == 0 ||
			namesame(sim_spice_signames[i], x_("v(vdd)")) == 0)
		{
			if (sim_spice_numbers != NONUMBERS && i < sim_spice_numbers->count)
				sim_window_setvdd( sim_spice_numbers->list[i] );
		}
	}

	/* add in saved signals */
	for(j=0; j<oldsigcount; j++)
	{
		/* see if the name is a bus, and ignore it */
		for(pt = oldsignames[j]; *pt != 0; pt++) if (*pt == '\t') break;
		if (*pt == '\t') continue;

		/* a single signal */
		position = 0;
		pt = oldsignames[j];
		if (*pt == '-') pt++;
		for( ; *pt != 0; pt++)
			if (!isdigit(*pt) || *pt == ':') break;
		if (*pt != ':') pt = oldsignames[j]; else
		{
			position = eatoi(oldsignames[j]);
			pt++;
		}
		for(i=0; i<sim_spice_signals; i++)
		{
			if (namesame(sim_spice_signames[i], pt) != 0) continue;
			tr = sim_window_newtrace(position, sim_spice_signames[i], 0);
			k = 0;
			for(num = sim_spice_numbers; num != NONUMBERS; num = num->nextnumbers)
				sim_spice_val[k++] = num->list[i];
			sim_window_loadanatrace(tr, numtotal, sim_spice_time, sim_spice_val);
			break;
		}
	}

	sim_window_renumberlines();
	sim_window_auto_anarange();

	/* clean up */
	min = sim_spice_time[0];   max = sim_spice_time[numtotal-1];
	if (min >= max)
	{
		ttyputmsg(_("Invalid time range"));
		return(TRUE);
	}
	sim_window_settimerange(0, min, max);
	sim_window_setmaincursor((max-min)*0.25f + min);
	sim_window_setextensioncursor((max-min)*0.75f + min);
	if (oldsigcount > 0) killstringarray(sa);
	sim_window_redraw();
	return(FALSE);
}

/*
 * routine to ensure that there is space for "count" time samples in the global arrays
 * "sim_spice_time" and "sim_spice_val".  Returns true on error.
 */
BOOLEAN sim_spice_ensurespace(INTBIG count)
{
	REGISTER INTBIG take;

	if (count <= sim_spice_limit) return(FALSE);
	if (sim_spice_limit > 0)
	{
		efree((CHAR *)sim_spice_time);
		efree((CHAR *)sim_spice_val);
		sim_spice_limit = 0;
	}
	take = count + 10;
	sim_spice_time = (double *)emalloc(take * (sizeof (double)), sim_tool->cluster);
	if (sim_spice_time == 0) return(TRUE);
	sim_spice_val = (float *)emalloc(take * (sizeof (float)), sim_tool->cluster);
	if (sim_spice_val == 0) return(TRUE);
	sim_spice_limit = take;
	return(FALSE);
}

BOOLEAN sim_spice_charhandlerschem(WINDOWPART *w, INTSML chr, INTBIG special)
{
	CHAR *par[3];

	/* special characters are not handled here */
	if (special != 0)
		return(us_charhandler(w, chr, special));

	switch (chr)
	{
		case '?':		/* help */
			ttyputmsg(_("These keys may be typed in the SPICE waveform window:"));
			ttyputinstruction(x_(" d"), 6, _("move down the hierarchy"));
			ttyputinstruction(x_(" u"), 6, _("move up the hierarchy"));
			ttyputinstruction(x_(" a"), 6, _("add selected network to waveform window in new frame"));
			ttyputinstruction(x_(" o"), 6, _("overlay selected network, add to current frame"));
			ttyputinstruction(x_(" r"), 6, _("remove selected network from waveform window"));
			return(FALSE);
		case 'a':		/* add trace */
			sim_spice_addhighlightednet(0, FALSE);
			return(FALSE);
		case 'o':		/* overlap trace */
			sim_spice_addhighlightednet(0, TRUE);
			return(FALSE);
		case 'r':		/* remove trace */
			sim_spice_removesignal();
			return(FALSE);
		case 'd':		/* move down the hierarchy */
			us_editcell(0, par);
			return(FALSE);
		case 'u':		/* move up the hierarchy */
			us_outhier(0, par);
			return(FALSE);
	}
	return(us_charhandler(w, chr, special));
}

BOOLEAN sim_spice_charhandlerwave(WINDOWPART *w, INTSML chr, INTBIG special)
{
	CHAR *par[3];
	REGISTER INTBIG tr, i, j, thispos, frame;
	REGISTER NUMBERS *num;

	/* special characters are not handled here */
	if (special != 0)
		return(us_charhandler(w, chr, special));

	/* see if there are special functions for SPICE waveform simulation */
	switch (chr)
	{
		case '?':		/* help */
			ttyputmsg(_("These keys may be typed in the SPICE Waveform window:"));
			ttyputinstruction(x_(" 9"), 6, _("show entire vertical range"));
			ttyputinstruction(x_(" 7"), 6, _("zoom vertical range in"));
			ttyputinstruction(x_(" 0"), 6, _("zoom vertical range out"));
			ttyputinstruction(x_(" 8"), 6, _("shift vertical range up"));
			ttyputinstruction(x_(" 2"), 6, _("shift vertical range down"));
			ttyputmsg(_("(Use standard window scaling commands for the horizontal axis)"));
			ttyputinstruction(x_(" a"), 6, _("add signal to simulation window in a new frame"));
			ttyputinstruction(x_(" o"), 6, _("overlay signal, added to current frame"));
			ttyputinstruction(x_(" r"), 6, _("remove selected signal from simulation window"));
			ttyputinstruction(x_(" e"), 6, _("remove all traces"));
			ttyputinstruction(x_(" p"), 6, _("preserve snapshot of simulation window in database"));
			return(FALSE);
		case '9':		/* show entire vertical range */
			tr = sim_window_gethighlighttrace();
			sim_window_auto_anarange() ;
			sim_window_redraw();
			return(FALSE);
		case '7':		/* zoom vertical range in */
			tr = sim_window_gethighlighttrace();
			frame = sim_window_gettraceframe(tr);
			sim_window_zoom_frame(frame);
			sim_window_redraw();
			return(FALSE);
		case '0':		/* zoom vertical range out */
			tr = sim_window_gethighlighttrace();
			frame = sim_window_gettraceframe(tr);
			sim_window_zoomout_frame(frame);
			sim_window_redraw();
			return(FALSE);
		case '8':		/* shift vertical range up */
			tr = sim_window_gethighlighttrace();
			frame = sim_window_gettraceframe(tr);
			sim_window_shiftup_frame(frame);
			sim_window_redraw();
			return(FALSE);
		case '2':		/* shift vertical range down */
			tr = sim_window_gethighlighttrace();
			frame = sim_window_gettraceframe(tr);
			sim_window_shiftdown_frame(frame);
			sim_window_redraw();
			return(FALSE);
		case 'p':		/* preserve image in {simulation_snapshot} view */
			sim_window_savegraph();
			return(FALSE);
		case 'a':		/* add trace */
			i = ttygetparam(_("Signal to add"), &sim_spice_picktrace, 3, par);
			if (i == 0) return(FALSE);
			i = sim_spice_findsignalname(par[0]);
			if (i < 0) return(FALSE);

			/* create a new trace in the last slot */
			tr = sim_window_newtrace(-1, sim_spice_signames[i], 0);
			j = 0;
			for(num = sim_spice_numbers; num != NONUMBERS; num = num->nextnumbers) {
				sim_spice_val[j++] = num->list[i];
			}
			sim_window_loadanatrace(tr, j, sim_spice_time, sim_spice_val);
			sim_window_cleartracehighlight();
			sim_window_addhighlighttrace(tr);
			sim_window_auto_anarange();
			sim_window_redraw();
			return(FALSE);
		case 'o':		/* overlap trace */
			tr = sim_window_gethighlighttrace();
			if (tr == 0) return(FALSE);
			thispos = sim_window_gettraceframe(tr);
			i = ttygetparam(_("Signal to overlay"), &sim_spice_picktrace, 3, par);
			if (i == 0) return(FALSE);
			i = sim_spice_findsignalname(par[0]);
			if (i < 0) return(FALSE);

			/* create a new trace in this slot */
			tr = sim_window_newtrace(thispos, sim_spice_signames[i], 0);
			j = 0;
			for(num = sim_spice_numbers; num != NONUMBERS; num = num->nextnumbers) {
				sim_spice_val[j++] = num->list[i];
			}
			sim_window_loadanatrace(tr, j, sim_spice_time, sim_spice_val);
			sim_window_cleartracehighlight();
			sim_window_addhighlighttrace(tr);
			sim_window_auto_anarange();
			sim_window_redraw();
			return(FALSE);
		case 'r':		/* remove trace */
		case DELETEKEY:
			sim_spice_removesignal();
			return(FALSE);
		case 'e':
			sim_window_killalltraces(FALSE);
			sim_window_redraw();
			return(FALSE);
	}
	return(us_charhandler(w, chr, special));
}

/*
 * Remove the highlighted signal from the waveform window.
 */
void sim_spice_removesignal(void)
{
	REGISTER INTBIG tr, trl, *trs, i, thispos, pos, lines;

	trs = sim_window_gethighlighttraces();
	if (trs[0] == 0) return;
	sim_window_cleartracehighlight();
	for(i=0; trs[i] != 0; i++)
	{
		tr = trs[i];

		/* see if any other traces use this line */
		thispos = sim_window_gettraceframe(tr);
		sim_window_inittraceloop();
		for(;;)
		{
			trl = sim_window_nexttraceloop();
			if (trl == tr) continue;
			if (trl == 0) break;
			pos = sim_window_gettraceframe(trl);
			if (pos == thispos) break;
		}
		lines = sim_window_getnumframes();
		if (trl == 0 && lines > 1)
		{
			/* no other traces on this line, delete it */
			sim_window_inittraceloop();
			for(;;)
			{
				trl = sim_window_nexttraceloop();
				if (trl == 0) break;
				pos = sim_window_gettraceframe(trl);
				if (pos > thispos) sim_window_settraceframe(trl, pos-1);
			}
			sim_window_setnumframes(lines-1);
		}

		/* kill trace, redraw */
		sim_window_killtrace(tr);
	}
	sim_window_redraw();
}

/*
 * Routine to add the highlighted signal to the waveform window.
 */
void sim_spice_addhighlightednet(CHAR *name, BOOLEAN overlay)
{
	REGISTER ARCINST *ai;
	NODEPROTO *np;
	REGISTER INTBIG i, tr, frameno, j;
	REGISTER CHAR *pt;
	REGISTER NUMBERS *num;

	if ((sim_window_isactive(&np) & SIMWINDOWWAVEFORM) == 0)
	{
		ttyputerr(_("Not displaying a waveform"));
		return;
	}

	if (name != 0) pt = name; else
	{
		ai = (ARCINST *)asktool(us_tool, x_("get-arc"));
		if (ai == NOARCINST)
		{
			ttyputerr(_("Select an arc first"));
			return;
		}
		if (ai->network == NONETWORK)
		{
			ttyputerr(_("This arc has no network information"));
			return;
		}
		pt = sim_spice_signalname(ai->network);
		if (pt == 0)
		{
			ttyputerr(_("Cannot get SPICE signal for network %s"), pt);
			return;
		}
	}
	i = sim_spice_findsignalname(pt);
	if (i < 0)
	{
		ttyputerr(_("Cannot find network %s in the simulation data"), pt);
		return;
	}

	/* figure out where to show the new signal */
	if (overlay)
	{
		frameno = sim_window_getcurframe();
		if (frameno < 0) overlay = FALSE;
	}
	if (!overlay) frameno = -1; else
	{
		frameno = sim_window_getcurframe();
		sim_window_cleartracehighlight();
	}

	/* create a new trace in this slot */
	tr = sim_window_newtrace(frameno, sim_spice_signames[i], 0);
	j = 0;
	for(num = sim_spice_numbers; num != NONUMBERS; num = num->nextnumbers)
	{
		sim_spice_val[j++] = num->list[i];
	}
	sim_window_loadanatrace(tr, j, sim_spice_time, sim_spice_val);
	sim_window_auto_anarange();
	sim_window_redraw();
	sim_window_cleartracehighlight();
	sim_window_addhighlighttrace(tr);
}

/*
 * Routine to return the NETWORK associated with HSPICE signal name "name".
 */
NETWORK *sim_spice_networkfromname(CHAR *name)
{
	NODEPROTO *np;
	REGISTER NODEPROTO *cnp;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt, *start;
	REGISTER NODEINST *ni;
	REGISTER NETWORK *net;

	/* get simulation cell, quit if not simulating */
	if (sim_window_isactive(&np) == 0) return(NONETWORK);

	/* parse the names, separated by dots */
	start = name;
	for(;;)
	{
		for(pt = start; *pt != 0 && *pt != '.'; pt++) ;
		if (*pt == 0) break;

		/* get node name that is pushed into */
		*pt = 0;
		if (*start == 'x') start++;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var == NOVARIABLE) continue;
			if (namesame(start, (CHAR *)var->addr) == 0) break;
		}
		*pt++ = '.';
		if (ni == NONODEINST) return(NONETWORK);
		start = pt;
		np = ni->proto;
		cnp = contentsview(np);
		if (cnp != NONODEPROTO) np = cnp;
	}

	/* find network "start" in cell "np" */
	net = getnetwork(start, np);
	return(net);
}

/*
 * Routine to return the SPICE network name of network "net".
 */
CHAR *sim_spice_signalname(NETWORK *net)
{
	NODEPROTO *np;
	REGISTER CHAR *prevstr, *signame;
	CHAR **nodenames;
	REGISTER NODEPROTO *netpar;
	REGISTER VARIABLE *varname;
	REGISTER PORTPROTO *pp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEINST *ni;
	REGISTER NETWORK *busnet;
	REGISTER INTBIG sigcount, i, busidx;
	REGISTER void *infstr;
	INTBIG index;

	/* get simulation cell, quit if not simulating */
	if (sim_window_isactive(&np) == 0) return(0);

	/* shift this network up the hierarchy as far as it is exported */
	for(;;)
	{
		/* if this network is at the current level of hierarchy, stop going up the hierarchy */
		if (net->parent == np) break;

		/* find the instance that is the proper parent of this cell */
		ni = descentparent(net->parent, &index, NOWINDOWPART, 0);
		if (ni == NONODEINST) break;

		/* see if the network is exported from this level */
		for(pp = net->parent->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			if (pp->network == net) break;
		if (pp != NOPORTPROTO)
			pp = equivalentport(net->parent, pp, ni->proto);

		/* see if network is exported from this level as part of a bus */
		if (pp == NOPORTPROTO && net->buswidth <= 1 && net->buslinkcount > 0) {
			/* find bus that this singel wire net is part of */
			busidx = -1;
			for (busnet = net->parent->firstnetwork; busnet != NONETWORK; busnet = busnet->nextnetwork) {
				if (busnet->buswidth > 1) { // must be bus
					for (i=0; i<busnet->buswidth; i++) {
						if (net == busnet->networklist[i]) { 
							busidx = i; // net part of bus
							break;
						}
					}
				}
				if (busidx != -1) break;
			}
			/* make sure won't access array out of bounds */
			if (busidx == -1) busidx = 0;
			/* see if bus exported */
			for(pp = busnet->parent->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				if (pp->network == busnet) {
					break;
				}
			if (pp != NOPORTPROTO) {
				pp = equivalentport(busnet->parent, pp, ni->proto);
			}
		}
		if (pp == NOPORTPROTO) break;

		/* exported: find the network in the higher level */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			if (pi->proto == pp)
		{
			/* if bussed, grab correct net in bus, otherwise just grab net */
			if (pi->conarcinst->network->buswidth > 1 && pi->conarcinst->network->buswidth > busidx) {
				net = pi->conarcinst->network->networklist[busidx];
			} else {
				net = pi->conarcinst->network;
			}
			break;
		}
		if (pi == NOPORTARCINST)
		{
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				if (pe->proto == pp)
			{
				/* if bussed, grab correct net in bus, otherwise just grab net */
				if (pe->exportproto->network->buswidth > 1 && pe->exportproto->network->buswidth > busidx) {
					net = pe->exportproto->network->networklist[busidx];
				} else {
					net = pe->exportproto->network;
				}
				break;
			}
			if (pe == NOPORTEXPINST) break;
		}
	}

	/* construct a path to the top-level of simulation from the net's current level */
	netpar = net->parent;
	prevstr = x_("");
	for(;;)
	{
		if (insamecellgrp(netpar, np)) break;

		/* find the instance that is the proper parent of this cell */
		ni = descentparent(netpar, &index, NOWINDOWPART, 0);
		if (ni == NONODEINST) break;

		varname = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (varname == 0)
		{
			ttyputerr(_("Back annotation is missing"));
			return(0);
		}
		infstr = initinfstr();
		addtoinfstr(infstr, 'x');
		signame = (CHAR *)varname->addr;
		if (ni->arraysize > 1)
		{
			sigcount = net_evalbusname(APBUS, signame, &nodenames,
				NOARCINST, NONODEPROTO, 0);
			if (index >= 0 && index < sigcount)
				signame = nodenames[index];
		}
		addstringtoinfstr(infstr, signame);
		if (*prevstr != 0)
		{
			addtoinfstr(infstr, '.');
			addstringtoinfstr(infstr, prevstr);
		}
		prevstr = returninfstr(infstr);
		netpar = ni->parent;
	}

	infstr = initinfstr();
	addstringtoinfstr(infstr, prevstr);
	if (*prevstr != 0) addtoinfstr(infstr, '.');
	if (net->namecount <= 0) addstringtoinfstr(infstr, describenetwork(net)); else
		addstringtoinfstr(infstr, networkname(net, 0));
	return(returninfstr(infstr));
}

INTBIG sim_spice_findsignalname(CHAR *name)
{
	REGISTER INTBIG i, len;

	for(i=0; i<sim_spice_signals; i++)
		if (namesame(name, sim_spice_signames[i]) == 0) return(i);

	/* try looking for signal names wrapped in "v()" or "i()" */
	len = strlen(name);
	for(i=0; i<sim_spice_signals; i++)
	{
		if (namesamen(name, &sim_spice_signames[i][2], len) == 0)
		{
			if (sim_spice_signames[i][0] != 'i' && sim_spice_signames[i][0] != 'v')
				continue;
			if (sim_spice_signames[i][1] != '(') continue;
			if (sim_spice_signames[i][len+2] != ')') continue;
			if (sim_spice_signames[i][len+3] != 0) continue;
			return(i);
		}
	}
	return(-1);
}

/*
 * Routine that feeds the current signals into the explorer window.
 */
void sim_spicereportsignals(WINDOWPART *simwin, void *(*addbranch)(CHAR*, void*),
	void *(*findbranch)(CHAR*, void*), void *(*addleaf)(CHAR*, void*), CHAR *(*nodename)(void*))
{
	REGISTER INTBIG i, j, k, len;
	REGISTER CHAR *pt, *signame, save;
	REGISTER void *curlevel, *nextlevel;

	for(i=0; i<sim_spice_signals; i++)
	{
		signame = sim_spice_signames[i];
		len = strlen(signame);
		for(j=len-2; j>0; j--)
			if (signame[j] == '.') break;
		if (j <= 0)
		{
			/* no "." qualifiers */
			(*addleaf)(signame, 0);
		} else
		{
			/* is down the hierarchy */
			signame[j] = 0;

			/* now find the location for this branch */
			curlevel = 0;
			pt = signame;
			for(;;)
			{
				for(k=0; pt[k] != 0; k++) if (pt[k] == '.') break;
				save = pt[k];
				pt[k] = 0;
				nextlevel = (*findbranch)(pt, curlevel);
				if (nextlevel == 0)
					nextlevel = (*addbranch)(pt, curlevel);
				curlevel = nextlevel;
				if (save == 0) break;
				pt[k] = save;
				pt = &pt[k+1];
			}
			(*addleaf)(&signame[j+1], curlevel);
			signame[j] = '.';
		}
	}
}

#endif  /* SIMTOOL - at top */
