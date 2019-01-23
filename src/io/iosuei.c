/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iosuei.c
 * Input/output tool: SUE (Schematic User Environment) reader
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
#include "edialogs.h"
#include "egraphics.h"
#include "eio.h"
#include "tech.h"
#include "tecart.h"
#include "tecgen.h"
#include "tecschem.h"
#include "network.h"
#include "usr.h"

/* #define SHOWORIGINAL 1 */		/* uncomment to show original nodes */

#define E0     (WHOLE/8)	/*  0.125 */
#define S1     (T1+E0)		/*  1.875 */
#define F3     (K3+H0+E0)	/*  3.625 */
#define S6     (T6+E0)		/*  6.875 */
#define Q11    (K11+Q0)		/* 11.25 */

/*************** SUE EQUIVALENCES ***************/

typedef struct
{
	CHAR   *portname;
	INTBIG  xoffset;
	INTBIG  yoffset;
} SUEEXTRAWIRE;

typedef struct
{
	CHAR         *suename;
	NODEPROTO   **intproto;
	INTBIG        netateoutput;
	INTBIG        rotation;
	INTBIG        transpose;
	INTBIG        xoffset;
	INTBIG        yoffset;
	INTBIG        detailbits;
	INTBIG        numextrawires;
	SUEEXTRAWIRE *extrawires;
} SUEEQUIV;

SUEEXTRAWIRE io_suetransistorwires[] =
{
	{x_("d"),    K3,   0},
	{x_("s"),   -K3,   0},
	{x_("g"),     0,  H4}
};

SUEEXTRAWIRE io_suetransistor4wires[] =
{
	{x_("d"),    K3,   0},
	{x_("s"),   -K3,   0},
	{x_("b"),   -Q0, -H2},
	{x_("g"),     0,  H4}
};

SUEEXTRAWIRE io_sueresistorwires[] =
{
	{x_("a"),   -K3,   0},
	{x_("b"),    K3,   0}
};

SUEEXTRAWIRE io_suecapacitorwires[] =
{
	{x_("a"),     0,  T1},
	{x_("b"),     0, -T1}
};

SUEEXTRAWIRE io_suesourcewires[] =
{
	{x_("minus"), 0, -Q1},
	{x_("plus"),  0,  H1}
};

SUEEXTRAWIRE io_suetwoportwires[] =
{
	{x_("a"),  -Q11,  F3},
	{x_("b"),  -Q11, -F3},
	{x_("x"),   Q11,  F3},
	{x_("y"),   Q11, -F3}
};

SUEEQUIV io_sueequivs[] =
{
	/* name         primitive           NEG  ANG     X   Y  BITS         WIR EXTRA-WIRES */
	{x_("pmos10"),     &sch_transistorprim,  0, 900,0, -K2,  0, TRANPMOS,     3, io_suetransistorwires},
	{x_("nmos10"),     &sch_transistorprim,  0, 900,0, -K2,  0, TRANNMOS,     3, io_suetransistorwires},
	{x_("pmos4"),      &sch_transistorprim,  0, 900,0, -K2,  0, TRANPMOS,     3, io_suetransistorwires},
	{x_("nmos4"),      &sch_transistorprim,  0, 900,0, -K2,  0, TRANNMOS,     3, io_suetransistorwires},
	{x_("pmos"),       &sch_transistorprim,  0, 900,0, -K2,  0, TRANPMOS,     3, io_suetransistorwires},
	{x_("nmos"),       &sch_transistorprim,  0, 900,0, -K2,  0, TRANNMOS,     3, io_suetransistorwires},
	{x_("capacitor"),  &sch_capacitorprim,   0,   0,0,   0,  0, 0,            2, io_suecapacitorwires},
	{x_("resistor"),   &sch_resistorprim,    0, 900,0,   0,  0, 0,            2, io_sueresistorwires},
	{x_("inductor"),   &sch_inductorprim,    0,   0,0,   0,  0, 0,            0, 0},
	{x_("cccs"),       &sch_twoportprim,     0,   0,0,  Q1,-S6, TWOPCCCS,     4, io_suetwoportwires},
	{x_("ccvs"),       &sch_twoportprim,     0,   0,0,  Q1,-S6, TWOPCCVS,     4, io_suetwoportwires},
	{x_("vcvs"),       &sch_twoportprim,     0,   0,0,  Q1,-S6, TWOPVCVS,     4, io_suetwoportwires},
	{x_("vccs"),       &sch_twoportprim,     0,   0,0, -S1,-K5, TWOPVCCS,     0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0}
};

SUEEQUIV io_sueequivs4[] =
{
	/* name         primitive           NEG  ANG     X   Y  BITS         WIR EXTRA-WIRES */
	{x_("pmos10"),     &sch_transistor4prim, 0,   0,1, -K2,  0, TRANPMOS,     4, io_suetransistor4wires},
	{x_("nmos10"),     &sch_transistor4prim, 0, 900,0, -K2,  0, TRANNMOS,     4, io_suetransistor4wires},
	{x_("pmos4"),      &sch_transistor4prim, 0,   0,1, -K2,  0, TRANPMOS,     4, io_suetransistor4wires},
	{x_("nmos4"),      &sch_transistor4prim, 0, 900,0, -K2,  0, TRANNMOS,     4, io_suetransistor4wires},
	{x_("pmos"),       &sch_transistor4prim, 0,   0,1, -K2,  0, TRANPMOS,     4, io_suetransistor4wires},
	{x_("nmos"),       &sch_transistor4prim, 0, 900,0, -K2,  0, TRANNMOS,     4, io_suetransistor4wires},
	{x_("capacitor"),  &sch_capacitorprim,   0,   0,0,   0,  0, 0,            2, io_suecapacitorwires},
	{x_("resistor"),   &sch_resistorprim,    0, 900,0,   0,  0, 0,            2, io_sueresistorwires},
	{x_("inductor"),   &sch_inductorprim,    0,   0,0,   0,  0, 0,            0, 0},
	{x_("cccs"),       &sch_twoportprim,     0,   0,0,  Q1,-S6, TWOPCCCS,     4, io_suetwoportwires},
	{x_("ccvs"),       &sch_twoportprim,     0,   0,0,  Q1,-S6, TWOPCCVS,     4, io_suetwoportwires},
	{x_("vcvs"),       &sch_twoportprim,     0,   0,0,  Q1,-S6, TWOPVCVS,     4, io_suetwoportwires},
	{x_("vccs"),       &sch_twoportprim,     0,   0,0, -S1,-K5, TWOPVCCS,     0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/*************** SUE WIRES ***************/

#define NOSUEWIRE ((SUEWIRE *)-1)

typedef struct Isuewire
{
	INTBIG           x[2], y[2];
	NODEINST        *ni[2];
	PORTPROTO       *pp[2];
	ARCPROTO        *proto;
	struct Isuewire *nextsuewire;
} SUEWIRE;

SUEWIRE *io_suefreewire = NOSUEWIRE;

/*************** SUE NETWORKS ***************/

#define NOSUENET ((SUENET *)-1)

typedef struct Isuenet
{
	INTBIG          x, y;
	CHAR           *label;
	struct Isuenet *nextsuenet;
} SUENET;

SUENET *io_suefreenet = NOSUENET;

/*************** MISCELLANEOUS ***************/

#define MAXLINE       300			/* maximum characters on an input line */
#define MAXKEYWORDS    50			/* maximum keywords on an input line */
#define MAXICONPOINTS  25			/* maximum coordinates on an "icon_line" line */

static INTBIG io_suelineno;
static INTBIG io_suefilesize;
static CHAR   io_suelastline[MAXLINE];
static CHAR   io_sueorigline[MAXLINE];
static CHAR   io_suecurline[MAXLINE];
static CHAR **io_suedirectories;
       INTBIG io_suenumdirectories = 0;
static FILE  *io_suefilein;

/* prototypes for local routines */
static BOOLEAN    io_sueadddirectory(CHAR *directory);
static void       io_suecleardirectories(void);
static CHAR      *io_suefindbusname(ARCINST *ai);
static BOOLEAN    io_suefindnode(INTBIG *x, INTBIG *y, INTBIG ox, INTBIG oy, NODEPROTO *np,
					NODEINST **ni, PORTPROTO **pp, NODEINST *notthisnode, INTBIG lambda);
static NODEINST  *io_suefindpinnode(INTBIG x, INTBIG y, NODEPROTO *np, PORTPROTO **thepp);
static void       io_suefreenets(SUENET *firstsn);
static void       io_suefreewires(SUEWIRE *firstsw);
static INTBIG     io_suegetnextline(CHAR **keywords, INTBIG curlydepth, void *dia);
static NODEPROTO *io_suegetnodeproto(LIBRARY *lib, CHAR *protoname);
static void       io_suekillnet(SUENET *sn);
static void       io_suekillwire(SUEWIRE *sw);
static INTBIG     io_suemakex(INTBIG x, INTBIG lambda);
static INTBIG     io_suemakey(INTBIG y, INTBIG lambda);
static PORTPROTO *io_suenewexport(NODEPROTO *cell, NODEINST *ni, PORTPROTO *pp, CHAR *thename);
static SUENET    *io_suenewnet(void);
static SUEWIRE   *io_suenewwire(void);
static void       io_sueplacenets(SUENET *firstsuenet, NODEPROTO *cell);
static CHAR      *io_sueparseexpression(CHAR *expression);
static void       io_sueparseparameters(CHAR **keywords, INTBIG count, INTBIG *x, INTBIG *y, INTBIG lambda,
					INTBIG *rot, INTBIG *trn, INTBIG *type, CHAR **thename, CHAR **thelabel, CHAR **thetext,
					void *dia);
static void       io_sueplacewires(SUEWIRE *firstsuewire, SUENET *firstsuenet, NODEPROTO *cell, INTBIG lambda);
static NODEPROTO *io_suereadfile(LIBRARY *lib, CHAR *cellname, void *dia);
static NODEPROTO *io_suereadfromdisk(LIBRARY *lib, CHAR *name, void *dia);
static CHAR      *io_suesearchbusname(ARCINST *ai);
static PORTPROTO *io_suewiredport(NODEINST *ni, INTBIG *x, INTBIG *y, INTBIG ox, INTBIG oy);

/*
 * Routine to free all memory associated with this module.
 */
void io_freesuememory(void)
{
	SUEWIRE *sw;
	SUENET *sn;

	while (io_suefreewire != NOSUEWIRE)
	{
		sw = io_suefreewire;
		io_suefreewire = io_suefreewire->nextsuewire;
		efree((CHAR *)sw);
	}
	while (io_suefreenet != NOSUENET)
	{
		sn = io_suefreenet;
		io_suefreenet = io_suefreenet->nextsuenet;
		efree((CHAR *)sn);
	}
	io_suecleardirectories();
}

BOOLEAN io_readsuelibrary(LIBRARY *lib)
{
	REGISTER INTBIG len, i, filecount;
	REGISTER NODEPROTO *topcell;
	CHAR cellname[300], dirname[300], topdirname[300], truesuefile[300],
		*filename, **filelist;
	void *dia;

	/* open the file */
	io_suefilein = xopen(lib->libfile, io_filetypesue, x_(""), &filename);
	if (io_suefilein == 0)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	/* determine the cell name */
	estrcpy(truesuefile, filename);
	estrcpy(cellname, filename);
	len = estrlen(cellname);
	if (namesame(&cellname[len-4], x_(".sue")) == 0)
		cellname[len-4] = 0;
	for(i = estrlen(cellname)-1; i>0; i--)
		if (cellname[i] == DIRSEP) break;
	i++;
	estrcpy(cellname, &cellname[i]);

	/* initialize the number of directories that need to be searched */
	io_suecleardirectories();

	/* determine the current directory */
	estrcpy(topdirname, filename);
	len = estrlen(topdirname);
	for(i = len-1; i>0; i--)
		if (topdirname[i] == DIRSEP) break;
	topdirname[i+1] = 0;
	if (io_sueadddirectory(topdirname)) return(TRUE);

	/* find all subdirectories that start with "suelib_" and include them in the search */
	filecount = filesindirectory(topdirname, &filelist);
	for(i=0; i<filecount; i++)
	{
		if (namesamen(filelist[i], x_("suelib_"), 7) != 0) continue;
		estrcpy(dirname, topdirname);
		estrcat(dirname, filelist[i]);
		if (fileexistence(dirname) != 2) continue;
		estrcat(dirname, DIRSEPSTR);
		if (io_sueadddirectory(dirname)) return(TRUE);
	}

	/* see if the current directory is inside of a SUELIB */
	len = estrlen(topdirname);
	for(i = len-2; i>0; i--)
		if (topdirname[i] == DIRSEP) break;
	i++;
	if (namesamen(&topdirname[i], x_("suelib_"), 7) == 0)
	{
		topdirname[i] = 0;
		filecount = filesindirectory(topdirname, &filelist);
		for(i=0; i<filecount; i++)
		{
			if (namesamen(filelist[i], x_("suelib_"), 7) != 0) continue;
			estrcpy(dirname, topdirname);
			estrcat(dirname, filelist[i]);
			if (fileexistence(dirname) != 2) continue;
			estrcat(dirname, DIRSEPSTR);
			if (io_sueadddirectory(dirname)) return(TRUE);
		}
	}

	/* prepare for input */
	io_suefilesize = filesize(io_suefilein);
	dia = DiaInitProgress(_("Reading SUE file..."), 0);
	if (dia == 0)
	{
		xclose(io_suefilein);
		return(TRUE);
	}
	io_suelineno = 0;

	/* read the file */
	DiaSetProgress(dia, 0, io_suefilesize);
	topcell = io_suereadfile(lib, cellname, dia);
	if (topcell != NONODEPROTO)
		lib->curnodeproto = topcell;
	(void)asktool(net_tool, x_("total-re-number"));

	/* clean up */
	DiaDoneProgress(dia);
	xclose(io_suefilein);
	ttyputmsg(_("%s read"), truesuefile);

	return(FALSE);
}

/*
 * Routine to read the SUE file in "f"/
 */
NODEPROTO *io_suereadfile(LIBRARY *lib, CHAR *cellname, void *dia)
{
	CHAR *keywords[MAXKEYWORDS], *thename, *thelabel, *thetext, *namestring;
	CHAR suevarname[200], *pt, *cpt, *startkey, save;
	CHAR **argnames, **argvalues, **newargnames, **newargvalues;
	INTBIG outline[MAXICONPOINTS*2], x, y, type, dx, dy, px, py, pinx, piny, xoff, yoff,
		numargs, namestrlen, rot, trn, rotation, transpose, *curstate;
	REGISTER INTBIG count, i, j, lx, hx, ly, hy, curly, xshrink, yshrink,
		varcount, varindex, varoffset, pos, invertoutput, lambda, detailbits,
		keycount, cx, cy, p1x, p1y, p2x, p2y, start, extent, numextrawires, len,
		iconx, icony, newaddr, newtype, xpos, ypos, numericlambda, bits;
	REGISTER BOOLEAN halvesize, placeicon, varissize, isparam;
	XARRAY trans;
	double rextent, rstart;
	float f;
	REGISTER NODEPROTO *cell, *schemcell, *iconcell, *proto, *np, *cnp;
	REGISTER PORTPROTO *pp, *ppt;
	PORTPROTO *npp;
	REGISTER VARIABLE *var;
	REGISTER TECHNOLOGY *inischemtech;
	REGISTER NODEINST *ni;
	NODEINST *nni;
	REGISTER ARCINST *ai;
	REGISTER SUEWIRE *sw, *firstsuewire;
	REGISTER SUENET *sn, *firstsuenet;
	REGISTER SUEEXTRAWIRE *extrawires;
	REGISTER SUEEQUIV *curequivs;
	REGISTER void *infstr;

	firstsuewire = NOSUEWIRE;
	firstsuenet = NOSUENET;
	schemcell = iconcell = cell = NONODEPROTO;
	io_suelastline[0] = 0;
	numargs = 0;
	namestrlen = 0;
	lambda = lib->lambda[sch_tech->techindex];
	inischemtech = defschematictechnology(el_curtech);
	numericlambda = lib->lambda[inischemtech->techindex];
	for(;;)
	{
	   /* get the next line of text */
		count = io_suegetnextline(keywords, 0, dia);
		if (count < 0) break;
		if (count == 0) continue;

		/* handle "proc" for defining views */
		if (namesame(keywords[0], x_("proc")) == 0)
		{
			/* write any wires from the last proc */
			if (cell != NONODEPROTO)
			{
				io_sueplacewires(firstsuewire, firstsuenet, cell, lambda);
				io_suefreewires(firstsuewire);
				io_sueplacenets(firstsuenet, cell);
				io_suefreenets(firstsuenet);
				firstsuewire = NOSUEWIRE;
				firstsuenet = NOSUENET;
			}

			if (count < 2)
			{
				ttyputerr(_("Cell %s, line %ld: 'proc' is missing arguments: %s"),
					cellname, io_suelineno, io_sueorigline);
				continue;
			}

			if (namesamen(keywords[1], x_("SCHEMATIC_"), 10) == 0)
			{
				/* create the schematic cell */
				infstr = initinfstr();
				if (namesame(&keywords[1][10], x_("[get_file_name]")) == 0)
					addstringtoinfstr(infstr, cellname); else
						addstringtoinfstr(infstr, &keywords[1][10]);
				addstringtoinfstr(infstr, x_("{sch}"));
				schemcell = cell = us_newnodeproto(returninfstr(infstr), lib);
				placeicon = FALSE;
			} else if (namesamen(keywords[1], x_("ICON_"), 5) == 0)
			{
				/* create the icon cell */
				infstr = initinfstr();
				if (namesame(&keywords[1][5], x_("[get_file_name]")) == 0)
					addstringtoinfstr(infstr, cellname); else
						addstringtoinfstr(infstr, &keywords[1][5]);
				addstringtoinfstr(infstr, x_("{ic}"));
				iconcell = cell = us_newnodeproto(returninfstr(infstr), lib);
			} else
			{
				ttyputerr(_("Cell %s, line %ld: unknown 'proc' statement: %s"),
					cellname, io_suelineno, io_sueorigline);
			}
			continue;
		}

		/* handle "make" for defining components */
		if (namesame(keywords[0], x_("make")) == 0)
		{
			if (count < 2)
			{
				ttyputerr(_("Cell %s, line %ld: 'make' is missing arguments: %s"),
					cellname, io_suelineno, io_sueorigline);
				continue;
			}

			/* extract parameters */
			io_sueparseparameters(&keywords[2], count-2, &x, &y, lambda, &rot, &trn,
				&type, &thename, &thelabel, &thetext, dia);

			/* save the name string */
			if (thename != 0)
			{
				len = estrlen(thename) + 1;
				if (len > namestrlen)
				{
					/* LINTED "namestring" used in proper order */
					if (namestrlen > 0) efree(namestring);
					namestring = (CHAR *)emalloc(len * SIZEOFCHAR, el_tempcluster);
					namestrlen = len;
				}
				estrcpy(namestring, thename);
				thename = namestring;
			}

			/* ignore self-references */
			if (namesame(keywords[1], cellname) == 0)
			{
				if (x != 0 || y != 0)
				{
					/* queue icon placement */
					iconx = x;   icony = y;
					placeicon = TRUE;
				}
				continue;
			}

			/* special case for network names: queue them */
			if (namesame(keywords[1], x_("name_net_m")) == 0 ||
				namesame(keywords[1], x_("name_net_s")) == 0 ||
				namesame(keywords[1], x_("name_net")) == 0)
			{
				sn = io_suenewnet();
				sn->x = x;
				sn->y = y;
				(void)allocstring(&sn->label, thename, io_tool->cluster);
				sn->nextsuenet = firstsuenet;
				firstsuenet = sn;
				continue;
			}

			/* first check for special names */
			proto = NONODEPROTO;
			invertoutput = 0;
			rotation = transpose = 0;
			xoff = yoff = 0;
			xshrink = yshrink = 0;
			detailbits = 0;
			numextrawires = 0;
			extrawires = 0;
			type = 0;
			if (namesame(keywords[1], x_("inout")) == 0)
			{
				proto = sch_offpageprim;
				makeangle(rot, trn, trans);
				xform(K2, 0, &xoff, &yoff, trans);
				xoff = muldiv(xoff, lambda, WHOLE);
				yoff = muldiv(yoff, lambda, WHOLE);
				type = BIDIRPORT;
			} else if (namesame(keywords[1], x_("input")) == 0)
			{
				proto = sch_offpageprim;
				makeangle(rot, trn, trans);
				xform(-K2, 0, &xoff, &yoff, trans);
				xoff = muldiv(xoff, lambda, WHOLE);
				yoff = muldiv(yoff, lambda, WHOLE);
				type = INPORT;
			} else if (namesame(keywords[1], x_("output")) == 0)
			{
				proto = sch_offpageprim;
				makeangle(rot, trn, trans);
				xform(K2, 0, &xoff, &yoff, trans);
				xoff = muldiv(xoff, lambda, WHOLE);
				yoff = muldiv(yoff, lambda, WHOLE);
				type = OUTPORT;
			} else if (namesame(keywords[1], x_("rename_net")) == 0)
			{
				proto = sch_wirepinprim;
			} else if (namesame(keywords[1], x_("global")) == 0)
			{
				if (net_buswidth(thename) > 1) proto = sch_buspinprim; else
				{
					proto = sch_wirepinprim;
					if (namesame(thename, x_("gnd")) == 0)
					{
						makeangle(rot, trn, trans);
						xform(0, -K2, &xoff, &yoff, trans);
						xoff = muldiv(xoff, lambda, WHOLE);
						yoff = muldiv(yoff, lambda, WHOLE);
						proto = sch_gndprim;
						type = GNDPORT;
					}
					if (namesame(thename, x_("vdd")) == 0)
					{
						proto = sch_pwrprim;
						type = PWRPORT;
					}
				}
			} else if (namesame(keywords[1], x_("join_net")) == 0)
			{
				proto = sch_wireconprim;
				xshrink = -K2;
				makeangle(rot, trn, trans);
				xform(Q1, 0, &xoff, &yoff, trans);
				xoff = muldiv(xoff, lambda, WHOLE);
				yoff = muldiv(yoff, lambda, WHOLE);
			}

			/* now check for internal associations to known primitives */
			if (proto == NONODEPROTO)
			{
				curstate = io_getstatebits();
				if ((curstate[1]&SUEUSE4PORTTRANS) != 0) curequivs = io_sueequivs4; else
					curequivs = io_sueequivs;
				for(i=0; curequivs[i].suename != 0; i++)
					if (namesame(keywords[1], curequivs[i].suename) == 0) break;
				if (curequivs[i].suename != 0)
				{
					proto = *curequivs[i].intproto;
					invertoutput = curequivs[i].netateoutput;
					rotation = curequivs[i].rotation;
					transpose = curequivs[i].transpose;
					makeangle(rot, trn, trans);
					xform(curequivs[i].xoffset, curequivs[i].yoffset, &xoff, &yoff, trans);
					xoff = muldiv(xoff, lambda, WHOLE);
					yoff = muldiv(yoff, lambda, WHOLE);
					
					if (transpose != 0)
					{
						trn = 1 - trn;
						rot = rotation - rot;
						if (rot < 0) rot += 3600;
					} else
					{
						rot += rotation;
						if (rot >= 3600) rot -= 3600;
					}
					detailbits = curequivs[i].detailbits;
					numextrawires = curequivs[i].numextrawires;
					extrawires = curequivs[i].extrawires;
#ifdef SHOWORIGINAL
					defaultnodesize(proto, &px, &py);
					lx = x - px/2 + xoff;   hx = lx + px;
					ly = y - py/2 + yoff;   hy = ly + py;
					ni = newnodeinst(proto, lx, hx, ly, hy, trn, rot, cell);
					if (ni == NONODEINST) continue;
					ni->userbits |= detailbits;
					endobjectchange((INTBIG)ni, VNODEINST);
					proto = NONODEPROTO;
					invertoutput = 0;
					rotation = transpose = 0;
					xoff = yoff = 0;
					detailbits = 0;
					numextrawires = 0;
					extrawires = 0;
#endif
				}
			}

			/* now check for references to cells */
			if (proto == NONODEPROTO)
			{
				/* find node or read it from disk */
				proto = io_suegetnodeproto(lib, keywords[1]);
				if (proto == NONODEPROTO)
					proto = io_suereadfromdisk(lib, keywords[1], dia);

				/* set proper offsets for the cell */
				if (proto != NONODEPROTO)
				{
					np = iconview(proto);
					if (np != NONODEPROTO) proto = np;
					xoff = (proto->lowx + proto->highx) / 2;
					yoff = (proto->lowy + proto->highy) / 2;
					makeangle(rot, trn, trans);
					xform(xoff, yoff, &xoff, &yoff, trans);
				}
			}

			/* ignore "title" specifications */
			if (namesamen(keywords[1], x_("title_"), 6) == 0) continue;

			/* stop now if SUE node is unknown */
			if (proto == NONODEPROTO)
			{
				ttyputmsg(_("Cannot make '%s' in cell %s"), keywords[1], describenodeproto(cell));
				continue;
			}

			/* create the instance */
			defaultnodesize(proto, &px, &py);
			px -= muldiv(xshrink, lambda, WHOLE);
			py -= muldiv(yshrink, lambda, WHOLE);
			lx = x - px/2 + xoff;   hx = lx + px;
			ly = y - py/2 + yoff;   hy = ly + py;
			ni = newnodeinst(proto, lx, hx, ly, hy, trn, rot, cell);
			if (ni == NONODEINST) continue;
			ni->userbits |= detailbits;
			ni->temp1 = invertoutput;
			if (proto->primindex == 0 && proto->cellview == el_iconview)
				ni->userbits |= NEXPAND;
			endobjectchange((INTBIG)ni, VNODEINST);
			if (cell->tech == gen_tech)
				cell->tech = whattech(cell);

			/* add any extra wires to the node */
			for(i=0; i<numextrawires; i++)
			{
				pp = getportproto(proto, extrawires[i].portname);
				if (pp == NOPORTPROTO) continue;
				portposition(ni, pp, &x, &y);
				makeangle(ni->rotation, ni->transpose, trans);
				px = muldiv(extrawires[i].xoffset, lambda, WHOLE);
				py = muldiv(extrawires[i].yoffset, lambda, WHOLE);
				xform(px, py, &dx, &dy, trans);
				defaultnodesize(sch_wirepinprim, &px, &py);
				pinx = x + dx;   piny = y + dy;
				nni = io_suefindpinnode(pinx, piny, cell, &npp);
				if (nni == NONODEINST)
				{
					lx = pinx - px/2;   hx = lx + px;
					ly = piny - py/2;   hy = ly + py;
					nni = newnodeinst(sch_wirepinprim, lx, hx, ly, hy, 0, 0, cell);
					if (nni == NONODEINST) continue;
					npp = nni->proto->firstportproto;
				}
				bits = us_makearcuserbits(sch_wirearc);
				if (x != pinx && y != piny) bits &= ~FIXANG;
				ai = newarcinst(sch_wirearc, 0, bits, ni, pp, x, y, nni, npp, pinx, piny, cell);
				if (ai == NOARCINST)
				{
					ttyputerr(_("Error adding extra wires to node %s"), keywords[1]);
					break;
				}
			}

			/* handle names assigned to the node */
			if (thename != 0)
			{
				/* export a port if this is an input, output, inout */
				if (proto == sch_offpageprim && thename != 0)
				{
					pp = proto->firstportproto;
					if (namesame(keywords[1], x_("output")) == 0) pp = pp->nextportproto;
					ppt = io_suenewexport(cell, ni, pp, thename);
					if (ppt == NOPORTPROTO)
					{
						ttyputmsg(_("Cell %s, line %ld, could not create port %s"), 
							cellname, io_suelineno, thename);
					} else
					{
						defaulttextdescript(ppt->textdescript, NOGEOM);
						defaulttextsize(1, ppt->textdescript);
						ppt->userbits = (ppt->userbits & ~STATEBITS) | type;
						endobjectchange((INTBIG)ppt, VPORTPROTO);
					}
				} else
				{
					/* just name the node */
					var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)thename,
						VSTRING|VDISPLAY);
					if (var != NOVARIABLE)
						defaulttextsize(3, var->textdescript);
					net_setnodewidth(ni);
				}
			}

			/* count the variables */
			varcount = 0;
			for(i=2; i<count; i += 2)
			{
				if (keywords[i][0] != '-') continue;
				if (namesame(keywords[i], x_("-origin")) == 0 ||
					namesame(keywords[i], x_("-orient")) == 0 ||
					namesame(keywords[i], x_("-type")) == 0 ||
					namesame(keywords[i], x_("-name")) == 0) continue;
				varcount++;
			}

			/* add variables */
			varindex = 1;
			varoffset = (ni->highy - ni->lowy) / (varcount+1);
			for(i=2; i<count; i += 2)
			{
				if (keywords[i][0] != '-') continue;
				if (namesame(keywords[i], x_("-origin")) == 0 ||
					namesame(keywords[i], x_("-orient")) == 0 ||
					namesame(keywords[i], x_("-type")) == 0 ||
					namesame(keywords[i], x_("-name")) == 0) continue;
				varissize = FALSE;
				halvesize = FALSE;
				isparam = FALSE;
				if (namesame(&keywords[i][1], x_("w")) == 0)
				{
					estrcpy(suevarname, x_("ATTR_width"));
					varissize = TRUE;
					xpos = 2;
					ypos = -4;
				} else if (namesame(&keywords[i][1], x_("l")) == 0)
				{
					estrcpy(suevarname, x_("ATTR_length"));
					varissize = TRUE;
					xpos = -2;
					ypos = -4;
					halvesize = TRUE;
				} else
				{
					esnprintf(suevarname, 200, x_("ATTR_%s"), &keywords[i][1]);
					for(pt = suevarname; *pt != 0; pt++) if (*pt == ' ')
					{
						ttyputmsg(_("Cell %s, line %ld, bad variable name (%s)"), 
							cellname, io_suelineno, suevarname);
						break;
					}
					xpos = 0;
					pos = (ni->highy - ni->lowy) / 2 - varindex * varoffset;
					ypos = pos * 4 / lambda;
					isparam = TRUE;
				}
				newtype = 0;
				if (keywords[i][1] == 'W' && keywords[i][2] != 0)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, &keywords[i][2]);
					addstringtoinfstr(infstr, x_(":"));
					addstringtoinfstr(infstr, io_sueparseexpression(keywords[i+1]));
					newaddr = (INTBIG)returninfstr(infstr);
					newtype = VSTRING;
				} else
				{
					pt = keywords[i+1];
					len = estrlen(pt) - 1;
					if (tolower(pt[len]) == 'u')
					{
						pt[len] = 0;
						if (isanumber(pt))
						{
							newaddr = scalefromdispunit((float)eatof(pt), DISPUNITMIC) * WHOLE / numericlambda;
							newtype = VFRACT;
						}
						pt[len] = 'u';
					}
					if (newtype == 0 && isanumber(pt))
					{
						newtype = VINTEGER;
						newaddr = eatoi(pt);
						for(cpt = pt; *cpt != 0; cpt++) if (*cpt == '.' || *cpt == 'e' || *cpt == 'E')
						{
							f = (float)eatof(pt);
							j = (INTBIG)(f * WHOLE);
							if (j / WHOLE == f)
							{
								newtype = VFRACT;
								newaddr = j;
							} else
							{
								newtype = VFLOAT;
								newaddr = castint(f);
							}
							break;
						}
					}
					if (newtype == 0)
					{
						newaddr = (INTBIG)io_sueparseexpression(pt);
						newtype = VSTRING;
					}
				} 
#if LANGJAVA
				/* see if the string should be Java code */
				if (newtype == VSTRING)
				{
					for(cpt = (CHAR *)newaddr; *cpt != 0; cpt++)
					{
						if (*cpt == '@') break;
						if (tolower(*cpt) == 'p' && cpt[1] == '(') break;
					}
					if (*cpt != 0)
						newtype = VFLOAT|VJAVA;
				}
#endif
				var = setval((INTBIG)ni, VNODEINST, suevarname, newaddr,
					newtype|VDISPLAY);
				if (var != NOVARIABLE)
				{
					defaulttextdescript(var->textdescript, ni->geom);
					varindex++;
					TDSETOFF(var->textdescript, xpos, ypos);
					if (halvesize)
						TDSETSIZE(var->textdescript, TDGETSIZE(var->textdescript)/2);
					if (isparam)
					{
						TDSETISPARAM(var->textdescript, VTISPARAMETER);
						TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALUE);

						/* make sure the parameter exists in the cell definition */
						cnp = contentsview(ni->proto);
						if (cnp == NONODEPROTO) cnp = ni->proto;
						var = getval((INTBIG)cnp, VNODEPROTO, -1, suevarname);
						if (var == NOVARIABLE)
						{
							var = setval((INTBIG)cnp, VNODEPROTO, suevarname, newaddr,
								newtype|VDISPLAY);
							if (var != NOVARIABLE)
							{
								TDSETISPARAM(var->textdescript, VTISPARAMETER);
								TDSETDISPPART(var->textdescript, VTDISPLAYNAMEVALINH);
							}
						}
					}
				}
			}
			continue;
		}

		/* handle "make_wire" for defining arcs */
		if (namesame(keywords[0], x_("make_wire")) == 0)
		{
			sw = io_suenewwire();
			sw->x[0] = io_suemakex(eatoi(keywords[1]), lambda);
			sw->y[0] = io_suemakey(eatoi(keywords[2]), lambda);
			sw->x[1] = io_suemakex(eatoi(keywords[3]), lambda);
			sw->y[1] = io_suemakey(eatoi(keywords[4]), lambda);
			sw->nextsuewire = firstsuewire;
			firstsuewire = sw;
			continue;
		}

		/* handle "icon_term" for defining ports in icons */
		if (namesame(keywords[0], x_("icon_term")) == 0)
		{
			io_sueparseparameters(&keywords[1], count-1, &x, &y, lambda, &rot, &trn,
				&type, &thename, &thelabel, &thetext, dia);
			estrcpy(suevarname, thename);

			proto = sch_buspinprim;
			defaultnodesize(proto, &px, &py);
			lx = x - px/2;   hx = lx + px;
			ly = y - py/2;   hy = ly + py;
			ni = newnodeinst(proto, lx, hx, ly, hy, trn, rot%3600, cell);
			if (ni == NONODEINST) continue;
			endobjectchange((INTBIG)ni, VNODEINST);

			ppt = io_suenewexport(cell, ni, proto->firstportproto, suevarname);
			if (ppt == NOPORTPROTO)
			{
				ttyputmsg(_("Cell %s, line %ld, could not create port %s"), 
					cellname, io_suelineno, suevarname);
			} else
			{
				defaulttextdescript(ppt->textdescript, NOGEOM);
				defaulttextsize(1, ppt->textdescript);
				ppt->userbits = (ppt->userbits & ~STATEBITS) | type;
				endobjectchange((INTBIG)ppt, VPORTPROTO);
			}
			continue;
		}

		/* handle "icon_arc" for defining icon curves */
		if (namesame(keywords[0], x_("icon_arc")) == 0)
		{
			if (count != 9)
			{
				ttyputerr(_("Cell %s, line %ld: needs 9 arguments, has %ld: %s"),
					cellname, io_suelineno, count, io_sueorigline);
				continue;
			}
			start = 0;   extent = 359;
			p1x = io_suemakex(eatoi(keywords[1]), lambda);
			p1y = io_suemakey(eatoi(keywords[2]), lambda);
			p2x = io_suemakex(eatoi(keywords[3]), lambda);
			p2y = io_suemakey(eatoi(keywords[4]), lambda);
			if (namesame(keywords[5], x_("-start")) == 0) start = eatoi(keywords[6]);
			if (namesame(keywords[7], x_("-extent")) == 0) extent = eatoi(keywords[8]);
			lx = mini(p1x, p2x);   hx = maxi(p1x, p2x);
			ly = mini(p1y, p2y);   hy = maxi(p1y, p2y);

			ni = newnodeinst(art_circleprim, lx, hx, ly, hy, 0, 0, cell);
			if (ni == NONODEINST) continue;
			if (extent != 359)
			{
				if (extent < 0)
				{
					start += extent;
					extent = -extent;
				}
				rextent = extent+1;   rextent = rextent * EPI / 180.0;
				rstart = start * EPI / 180.0;
				setarcdegrees(ni, rstart, rextent);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}

		/* handle "icon_line" for defining icon outlines */
		if (namesame(keywords[0], x_("icon_line")) == 0)
		{
			for(i=1; i<count; i++)
			{
				if (namesame(keywords[i], x_("-tags")) == 0) break;
				if ((i%2) != 0) outline[i-1] = io_suemakex(eatoi(keywords[i]), lambda); else
					outline[i-1] = io_suemakey(eatoi(keywords[i]), lambda);
			}
			keycount = i / 2;

			/* determine bounds of icon */
			lx = hx = outline[0];
			ly = hy = outline[1];
			for(i=1; i<keycount; i++)
			{
				if (outline[i*2]   < lx) lx = outline[i*2];
				if (outline[i*2]   > hx) hx = outline[i*2];
				if (outline[i*2+1] < ly) ly = outline[i*2+1];
				if (outline[i*2+1] > hy) hy = outline[i*2+1];
			}
			ni = newnodeinst(art_openedpolygonprim, lx, hx, ly, hy, 0, 0, cell);
			if (ni == NONODEINST) return(NONODEPROTO);
			cx = (lx + hx) / 2;  cy = (ly + hy) / 2;
			for(i=0; i<keycount; i++)
			{
				outline[i*2] -= cx;   outline[i*2+1] -= cy;
			}
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)outline,
				VINTEGER|VISARRAY|((keycount*2)<<VLENGTHSH));
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}

		/* handle "icon_setup" for defining variables */
		if (namesame(keywords[0], x_("icon_setup")) == 0)
		{
			/* extract parameters */
			if (namesame(keywords[1], x_("$args")) != 0)
			{
				ttyputerr(_("Cell %s, line %ld: has unrecognized 'icon_setup'"),
					cellname, io_suelineno);
				continue;
			}
			pt = keywords[2];
			if (*pt == '{') pt++;
			for(;;)
			{
				while (*pt == ' ') pt++;
				if (*pt == '}' || *pt == 0) break;

				/* collect up to a space or close curly */
				startkey = pt;
				curly = 0;
				for(;;)
				{
					if (curly == 0)
					{
						if (*pt == 0 || *pt == ' ' || *pt == '}') break;
					}
					if (*pt == '{') curly++;
					if (*pt == '}') curly--;
					if (*pt == 0) break;
					pt++;
				}
				save = *pt;
				*pt = 0;

				/* parse the keyword pair in "startkey" */
				i = numargs+1;
				newargnames = (CHAR **)emalloc(i * (sizeof (CHAR *)), el_tempcluster);
				newargvalues = (CHAR **)emalloc(i * (sizeof (CHAR *)), el_tempcluster);
				for(i=0; i<numargs; i++)
				{
					/* LINTED "argnames" used in proper order */
					newargnames[i] = argnames[i];

					/* LINTED "argvalues" used in proper order */
					newargvalues[i] = argvalues[i];
				}
				if (numargs > 0)
				{
					efree((CHAR *)argnames);
					efree((CHAR *)argvalues);
				}
				argnames = newargnames;
				argvalues = newargvalues;
				startkey++;
				for(cpt = startkey; *cpt != 0; cpt++) if (*cpt == ' ') break;
				if (*cpt != 0) *cpt++ = 0;
				(void)allocstring(&argnames[numargs], startkey, el_tempcluster);
				while (*cpt == ' ') cpt++;
				if (*cpt == '{') cpt++;
				startkey = cpt;
				for(cpt = startkey; *cpt != 0; cpt++) if (*cpt == '}') break;
				if (*cpt != 0) *cpt++ = 0;
				(void)allocstring(&argvalues[numargs], startkey, el_tempcluster);
				numargs++;

				*pt = save;
			}
			continue;
		}

		/* handle "icon_property" for defining icon strings */
		if (namesame(keywords[0], x_("icon_property")) == 0)
		{
			/* extract parameters */
			io_sueparseparameters(&keywords[1], count-1, &x, &y, lambda, &rot, &trn,
				&type, &thename, &thelabel, &thetext, dia);
			if (thelabel == 0) continue;

			/* substitute parameters */
			infstr = initinfstr();
			for(pt = thelabel; *pt != 0; pt++)
			{
				if (*pt == '$')
				{
					for(i=0; i<numargs; i++)
						if (namesamen(&pt[1], argnames[i], estrlen(argnames[i])) == 0) break;
					if (i < numargs)
					{
						addstringtoinfstr(infstr, argvalues[i]);
						pt += estrlen(argnames[i]);
						continue;
					}
				}
				addtoinfstr(infstr, *pt);
			}
			thelabel = returninfstr(infstr);

			ni = newnodeinst(gen_invispinprim, x, x, y, y, 0, 0, cell);
			if (ni == NONODEINST) continue;
			var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)thelabel, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
			{
				defaulttextsize(2, var->textdescript);
				TDSETPOS(var->textdescript, VTPOSCENT);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}

		/* handle "make_text" for placing strings */
		if (namesame(keywords[0], x_("make_text")) == 0)
		{
			/* extract parameters */
			io_sueparseparameters(&keywords[1], count-1, &x, &y, lambda, &rot, &trn,
				&type, &thename, &thelabel, &thetext, dia);
			if (thetext == 0) continue;

			ni = newnodeinst(gen_invispinprim, x, x, y, y, 0, 0, cell);
			if (ni == NONODEINST) continue;
			var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)thetext, VSTRING|VDISPLAY);
			if (var != NOVARIABLE)
			{
				defaulttextsize(2, var->textdescript);
				TDSETPOS(var->textdescript, VTPOSCENT);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}

		/* ignore known keywords */
		if (namesame(keywords[0], x_("icon_title")) == 0 ||
			namesame(keywords[0], x_("make_line")) == 0 ||
			namesame(keywords[0], x_("}")) == 0)
		{
			continue;
		}

		ttyputerr(_("Cell %s, line %ld: unknown keyword (%s): %s"), cellname,
			io_suelineno, keywords[0], io_sueorigline);
	}

	/* place an icon instance in the schematic if requested */
	if (placeicon && schemcell != NONODEPROTO &&
		iconcell != NONODEPROTO)
	{
		px = iconcell->highx - iconcell->lowx;
		py = iconcell->highy - iconcell->lowy;
		lx = iconx - px/2;   hx = lx + px;
		ly = icony - py/2;   hy = ly + py;
		ni = newnodeinst(iconcell, lx, hx, ly, hy, 0, 0, schemcell);
		if (ni != NONODEINST)
		{
			ni->userbits |= NEXPAND;
			endobjectchange((INTBIG)ni, VNODEINST);
		}
	}

	for(i=0; i<numargs; i++)
	{
		efree(argnames[i]);
		efree(argvalues[i]);
	}
	if (numargs > 0)
	{
		efree((CHAR *)argnames);
		efree((CHAR *)argvalues);
	}
	if (namestrlen > 0) efree(namestring);

	/* cleanup the current cell */
	if (cell != NONODEPROTO)
	{
		io_sueplacewires(firstsuewire, firstsuenet, cell, lambda);
		io_suefreewires(firstsuewire);
		io_sueplacenets(firstsuenet, cell);
		io_suefreenets(firstsuenet);
	}

	/* make sure cells are the right size */
	if (schemcell != NONODEPROTO) (*el_curconstraint->solve)(schemcell);
	if (iconcell != NONODEPROTO) (*el_curconstraint->solve)(iconcell);

	/* return the cell */
	if (schemcell != NONODEPROTO) return(schemcell);
	return(iconcell);

}

/*
 * Routine to examine a SUE expression and add "@" in front of variable names.
 */
CHAR *io_sueparseexpression(CHAR *expression)
{
	REGISTER void *infstr;
	REGISTER CHAR *keyword;

	infstr = initinfstr();
	while (*expression != 0)
	{
		keyword = getkeyword(&expression, x_(" \t,+-*/()"));
		if (keyword == NOSTRING) break;
		if (*keyword != 0)
		{
			if (isdigit(keyword[0]))
			{
				addstringtoinfstr(infstr, keyword);
			} else
			{
				if (*expression != '(')
					addtoinfstr(infstr, '@');
				addstringtoinfstr(infstr, keyword);
			}
			if (*expression != 0)
				addtoinfstr(infstr, *expression++);
		}
	}
	return(returninfstr(infstr));
}

/*
 * Routine to parse the "count" parameters in "keywords" and fill in the values
 * that are found.  Fills in:
 * "-origin"  placed into "x" and "y"
 * "-orient"  placed into "rot" and "trn"
 * "-type"    placed into "type"
 * "-name"    placed into "thename".
 * "-label"   placed into "thelabel".
 * "-text"    placed into "thetext".
 */
void io_sueparseparameters(CHAR **keywords, INTBIG count, INTBIG *x, INTBIG *y, INTBIG lambda,
	INTBIG *rot, INTBIG *trn, INTBIG *type, CHAR **thename, CHAR **thelabel, CHAR **thetext,
	void *dia)
{
	CHAR *pt;
	REGISTER INTBIG textloc, i;
	REGISTER void *infstr;

	*x = *y = 0;
	*rot = 0;
	*trn = 0;
	*type = 0;
	*thename = 0;
	*thelabel = 0;
	*thetext = 0;
	for(i=0; i<count; i += 2)
	{
		if (namesame(keywords[i], x_("-origin")) == 0)
		{
			pt = keywords[i+1];
			if (*pt == '{') pt++;
			*x = eatoi(pt);
			while (*pt != ' ' && *pt != '\t' && *pt != 0) pt++;
			while (*pt == ' ' || *pt == '\t') pt++;
			*y = eatoi(pt);
		}
		if (namesame(keywords[i], x_("-orient")) == 0)
		{
			if (namesame(keywords[i+1], x_("R90"))  == 0) { *rot = 900;  } else
			if (namesame(keywords[i+1], x_("R270")) == 0) { *rot = 2700; } else
			if (namesame(keywords[i+1], x_("RXY"))  == 0) { *rot = 1800; } else
			if (namesame(keywords[i+1], x_("RY"))   == 0) { *rot = 900;  *trn = 1; } else
			if (namesame(keywords[i+1], x_("R90X")) == 0) { *rot = 0;    *trn = 1; } else
			if (namesame(keywords[i+1], x_("R90Y")) == 0) { *rot = 1800; *trn = 1; } else
			if (namesame(keywords[i+1], x_("RX"))   == 0) { *rot = 2700; *trn = 1; }
		}
		if (namesame(keywords[i], x_("-type")) == 0)
		{
			if (namesame(keywords[i+1], x_("input")) == 0) *type = INPORT; else
			if (namesame(keywords[i+1], x_("output")) == 0) *type = OUTPORT; else
			if (namesame(keywords[i+1], x_("inout")) == 0) *type = BIDIRPORT;
		}
		if (namesame(keywords[i], x_("-name")) == 0 ||
			namesame(keywords[i], x_("-label")) == 0 ||
			namesame(keywords[i], x_("-text")) == 0)
		{
			if (namesame(keywords[i], x_("-name")) == 0) textloc = 0; else
				if (namesame(keywords[i], x_("-label")) == 0) textloc = 1; else
					textloc = 2;
			infstr = initinfstr();
			pt = keywords[i+1];
			if (pt[0] != '{') addstringtoinfstr(infstr, pt); else
			{
				pt++;
				for(;;)
				{
					while (*pt != 0)
					{
						if (*pt == '}') break;
						addtoinfstr(infstr, *pt);
						pt++;
					}
					if (*pt == '}') break;
					addtoinfstr(infstr, ' ');

					count = io_suegetnextline(keywords, 1, dia);
					if (count < 0) break;
					if (count == 0) continue;
					pt = keywords[0];
					i = -1;
				}
			}
			switch (textloc)
			{
				case 0: *thename = returninfstr(infstr);    break;
				case 1: *thelabel = returninfstr(infstr);   break;
				case 2: *thetext = returninfstr(infstr);    break;
			}
		}
	}
	*x = io_suemakex(*x, lambda);
	*y = io_suemakey(*y, lambda);
	*rot = (3600 - *rot) % 3600;
}

/*
 * Routine to find cell "protoname" in library "lib".
 */
NODEPROTO *io_suegetnodeproto(LIBRARY *lib, CHAR *protoname)
{
	REGISTER NODEPROTO *np;

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesame(protoname, np->protoname) == 0) return(np);
	return(NONODEPROTO);
}

/*
 * Routine to create a port called "thename" on port "pp" of node "ni" in cell "cell".
 * The name is modified if it already exists.
 */
PORTPROTO *io_suenewexport(NODEPROTO *cell, NODEINST *ni, PORTPROTO *pp, CHAR *thename)
{
	REGISTER PORTPROTO *ppt;
	REGISTER INTBIG i;
	REGISTER CHAR *portname, *pt;
	CHAR numbuf[20];
	REGISTER void *infstr;

	portname = thename;
	for(i=0; ; i++)
	{
		ppt = getportproto(cell, portname);
		if (ppt == NOPORTPROTO)
		{
			ppt = newportproto(cell, ni, pp, portname);
			break;
		}

		/* make space for modified name */
		infstr = initinfstr();
		for(pt = thename; *pt != 0 && *pt != '['; pt++)
			addtoinfstr(infstr, *pt);
		esnprintf(numbuf, 20, x_("-%ld"), i);
		addstringtoinfstr(infstr, numbuf);
		for( ; *pt != 0; pt++)
			addtoinfstr(infstr, *pt);
		portname = returninfstr(infstr);
	}
	return(ppt);
}

/* Routine to convert SUE X coordinate "x" to Electric coordinates */
INTBIG io_suemakex(INTBIG x, INTBIG lambda)
{
	return(x * lambda / 8);
}

/* Routine to convert SUE Y coordinate "y" to Electric coordinates */
INTBIG io_suemakey(INTBIG y, INTBIG lambda)
{
	return(-y * lambda / 8);
}

/*
 * Routine to place all SUE nets into the cell (they are in a linked
 * list headed by "firstsuenet").
 */
void io_sueplacenets(SUENET *firstsuenet, NODEPROTO *cell)
{
	SUENET *sn;
	REGISTER INTBIG pass;
	REGISTER BOOLEAN isbus;
	REGISTER ARCINST *ai, *bestai;
	REGISTER INTBIG cx, cy, dist, bestdist, sea;
	REGISTER GEOM *geom;
	REGISTER CHAR *netname, *busname;
	REGISTER void *infstr;

	/* 3 passes: qualified labels, unqualified busses, unqualified wires */
	for(pass=0; pass<3; pass++)
	{
		for(sn = firstsuenet; sn != NOSUENET; sn = sn->nextsuenet)
		{
			/* unqualified labels (starting with "[") happen second */
			if (*sn->label == '[')
			{
				/* unqualified label: pass 2 or 3 only */
				if (pass == 0) continue;
			} else
			{
				/* qualified label: pass 1 only */
				if (pass != 0) continue;
			}

			/* see if this is a bus */
			if (net_buswidth(sn->label) > 1) isbus = TRUE; else isbus = FALSE;

			sea = initsearch(sn->x, sn->x, sn->y, sn->y, cell);
			bestai = NOARCINST;
			for(;;)
			{
				geom = nextobject(sea);
				if (geom == NOGEOM) break;
				if (geom->entryisnode) continue;
				ai = geom->entryaddr.ai;
				if (isbus)
				{
					if (ai->proto != sch_busarc) continue;
				} else
				{
					if (ai->proto == sch_busarc) continue;
				}
				cx = (ai->end[0].xpos + ai->end[1].xpos) / 2;
				cy = (ai->end[0].ypos + ai->end[1].ypos) / 2;
				dist = computedistance(cx, cy, sn->x, sn->y);

				/* LINTED "bestdist" used in proper order */
				if (bestai == NOARCINST || dist < bestdist)
				{
					bestai = ai;
					bestdist = dist;
				}
			}
			if (bestai != NOARCINST)
			{
				if (pass == 1)
				{
					/* only allow busses */
					if (bestai->proto != sch_busarc) continue;
				} else if (pass == 2)
				{
					/* disallow busses */
					if (bestai->proto == sch_busarc) continue;
				}
				netname = sn->label;
				if (*netname == '[')
				{
					/* find the proper name of the network */
					busname = io_suefindbusname(bestai);
					if (busname != 0)
					{
						infstr = initinfstr();
						addstringtoinfstr(infstr, busname);
						addstringtoinfstr(infstr, netname);
						netname = returninfstr(infstr);
					}
				}
				us_setarcname(bestai, netname);
			}
		}
	}
}

/*
 * Routine to start at "ai" and search all wires until it finds a named bus.
 * Returns zero if no bus name is found.
 */
CHAR *io_suefindbusname(ARCINST *ai)
{
	REGISTER ARCINST *oai;
	REGISTER CHAR *busname, *pt;
	REGISTER VARIABLE *var;
	static CHAR pseudobusname[50];
	REGISTER INTBIG index, len;

	for(oai = ai->parent->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
		oai->temp1 = 0;
	busname = io_suesearchbusname(ai);
	if (busname == 0)
	{
		for(index=1; ; index++)
		{
			esnprintf(pseudobusname, 50, x_("NET%ld"), index);
			len = estrlen(pseudobusname);
			for(oai = ai->parent->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
			{
				var = getvalkey((INTBIG)oai, VARCINST, VSTRING, el_arc_name_key);
				if (var == NOVARIABLE) continue;
				pt = (CHAR *)var->addr;
				if (namesame(pseudobusname, pt) == 0) break;
				if (namesamen(pseudobusname, pt, len) == 0 &&
					pt[len] == '[') break;
			}
			if (oai == NOARCINST) break;
		}
		busname = pseudobusname;
	}
	return(busname);
}

CHAR *io_suesearchbusname(ARCINST *ai)
{
	REGISTER ARCINST *oai;
	REGISTER CHAR *busname;
	REGISTER INTBIG i;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	ai->temp1 = 1;
	if (ai->proto == sch_busarc)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE)
		{
			infstr = initinfstr();
			for(busname = (CHAR *)var->addr; *busname != 0; busname++)
			{
				if (*busname == '[') break;
				addtoinfstr(infstr, *busname);
			}
			return(returninfstr(infstr));
		}
	}
	for(i=0; i<2; i++)
	{
		ni = ai->end[i].nodeinst;
		if (ni->proto != sch_wirepinprim && ni->proto != sch_buspinprim &&
			ni->proto != sch_offpageprim) continue;
		if (ni->proto == sch_buspinprim || ni->proto == sch_offpageprim)
		{
			/* see if there is an arrayed port here */
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				pp = pe->exportproto;
				for(busname = pp->protoname; *busname != 0; busname++)
					if (*busname == '[') break;
				if (*busname != 0)
				{
					infstr = initinfstr();
					for(busname = pp->protoname; *busname != 0; busname++)
					{
						if (*busname == '[') break;
						addtoinfstr(infstr, *busname);
					}
					return(returninfstr(infstr));
				}
			}
		}
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			oai = pi->conarcinst;
			if (oai->temp1 != 0) continue;
			busname = io_suesearchbusname(oai);
			if (busname != 0) return(busname);
		}
	}
	return(0);
}

/*
 * Routine to place all SUE wires into the cell (they are in a linked
 * list headed by "firstsuewire").
 */
void io_sueplacewires(SUEWIRE *firstsuewire, SUENET *firstsuenet, NODEPROTO *cell, INTBIG lambda)
{
	SUEWIRE *sw, *osw;
	SUENET *sn;
	REGISTER INTBIG i, j, wid, px, py, lx, hx, ly, hy, sea, bits;
	REGISTER BOOLEAN propagatedbus, isbus;
	INTBIG xsize, ysize, x, y, ox, oy;
	REGISTER NODEPROTO *proto;
	REGISTER GEOM *geom;
	REGISTER PORTEXPINST *pe;
	NODEINST *ni;
	REGISTER ARCPROTO *ap;
	REGISTER NODEINST *bottomni, *oni;
	PORTPROTO *pp;
	REGISTER PORTPROTO *bottompp, *opp;
	REGISTER ARCINST *ai;

	/* mark all wire ends as "unassigned", all wire types as unknown */
	for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
	{
		sw->ni[0] = sw->ni[1] = NONODEINST;
		sw->proto = NOARCPROTO;
	}

	/* examine all network names and assign wire types appropriately */
	for(sn = firstsuenet; sn != NOSUENET; sn = sn->nextsuenet)
	{
		for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
		{
			for(i=0; i<2; i++)
			{
				if (sw->x[i] == sn->x && sw->y[i] == sn->y)
				{
					if (net_buswidth(sn->label) > 1) sw->proto = sch_busarc; else
						sw->proto = sch_wirearc;
				}
			}
		}
	}

	/* find connections that are exactly on existing nodes */
	for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
	{
		for(i=0; i<2; i++)
		{
			if (sw->ni[i] != NONODEINST) continue;
			for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				pp = io_suewiredport(ni, &sw->x[i], &sw->y[i], sw->x[1-i], sw->y[1-i]);
				if (pp == NOPORTPROTO) continue;
				sw->ni[i] = ni;
				sw->pp[i] = pp;

				/* determine whether this port is a bus */
				isbus = FALSE;
				bottomni = ni;   bottompp = pp;
				while (bottomni->proto->primindex == 0)
				{
					bottomni = bottompp->subnodeinst;
					bottompp = bottompp->subportproto;
				}
				if (bottomni->proto == sch_wireconprim) continue;
				if (!isbus && ni->proto == sch_offpageprim)
				{
					/* see if there is a bus port on this primitive */
					for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
					{
						if (net_buswidth(pe->exportproto->protoname) > 1) isbus = TRUE;
					}
				}

				if (isbus)
				{
					sw->proto = sch_busarc;
				} else
				{
					if (sw->proto == NOARCPROTO)
						sw->proto = sch_wirearc;
				}
			}
		}
	}

	/* now iteratively extend bus wires to connections with others */
	propagatedbus = TRUE;
	while (propagatedbus)
	{
		propagatedbus = FALSE;
		for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
		{
			if (sw->proto != sch_busarc) continue;
			for(osw = firstsuewire; osw != NOSUEWIRE; osw = osw->nextsuewire)
			{
				if (osw->proto != NOARCPROTO) continue;
				for(i=0; i<2; i++)
				{
					for(j=0; j<2; j++)
					{
						if (sw->x[i] == osw->x[j] && sw->y[i] == osw->y[j])
						{
							/* common point found: continue the bus request */
							osw->proto = sch_busarc;
							propagatedbus = TRUE;
						}
					}
				}
			}
		}
	}

	/* now make pins where wires meet */
	for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
	{
		for(i=0; i<2; i++)
		{
			if (sw->ni[i] != NONODEINST) continue;
			if (sw->proto == sch_busarc) proto = sch_buspinprim; else
				proto = sch_wirepinprim;

			/* look at all other wires at this point and figure out type of pin to make */
			for(osw = firstsuewire; osw != NOSUEWIRE; osw = osw->nextsuewire)
			{
				if (osw == sw) continue;
				for(j=0; j<2; j++)
				{
					if (sw->x[i] != osw->x[j] || sw->y[i] != osw->y[j]) continue;
					if (osw->ni[j] != NONODEINST)
					{
						sw->ni[i] = osw->ni[j];
						sw->pp[i] = osw->pp[j];
						break;
					}
					if (osw->proto == sch_busarc) proto = sch_buspinprim;
				}
				if (sw->ni[i] != NONODEINST) break;
			}

			/* make the pin if it doesn't exist */
			if (sw->ni[i] == NONODEINST)
			{
				/* common point found: make a pin */
				defaultnodesize(proto, &xsize , &ysize);
				sw->ni[i] = newnodeinst(proto, sw->x[i] - xsize/2,
					sw->x[i] + xsize/2, sw->y[i] - ysize/2,
					sw->y[i] + ysize/2, 0, 0, cell);
				endobjectchange((INTBIG)sw->ni[i], VNODEINST);
				sw->pp[i] = proto->firstportproto;
			}

			/* put that node in all appropriate locations */
			for(osw = firstsuewire; osw != NOSUEWIRE; osw = osw->nextsuewire)
			{
				if (osw == sw) continue;
				for(j=0; j<2; j++)
				{
					if (sw->x[i] != osw->x[j] || sw->y[i] != osw->y[j]) continue;
					if (osw->ni[j] != NONODEINST) continue;
					osw->ni[j] = sw->ni[i];
					osw->pp[j] = sw->pp[i];
				}
			}
		}
	}

	/* make pins at all of the remaining wire ends */
	for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
	{
		for(i=0; i<2; i++)
		{
			if (sw->ni[i] != NONODEINST) continue;
			if (!io_suefindnode(&sw->x[i], &sw->y[i], sw->x[1-i], sw->y[1-i], cell,
				&sw->ni[i], &sw->pp[i], sw->ni[1-i], lambda))
			{
				if (sw->proto == sch_busarc) proto = sch_buspinprim; else
					proto = sch_wirepinprim;
				defaultnodesize(proto, &xsize , &ysize);
				sw->ni[i] = newnodeinst(proto, sw->x[i] - xsize/2,
					sw->x[i] + xsize/2, sw->y[i] - ysize/2, sw->y[i] + ysize/2,
					0, 0, cell);
				endobjectchange((INTBIG)sw->ni[i], VNODEINST);
				sw->pp[i] = sw->ni[i]->proto->firstportproto;
			}
		}
	}

	/* now make the connections */
	for(sw = firstsuewire; sw != NOSUEWIRE; sw = sw->nextsuewire)
	{
		if (sw->proto == NOARCPROTO) sw->proto = sch_wirearc;
		wid = defaultarcwidth(sw->proto);

		/* if this is a bus, make sure it can connect */
		if (sw->proto == sch_busarc)
		{
			for(i=0; i<2; i++)
			{
				for(j=0; sw->pp[i]->connects[j] != NOARCPROTO; j++)
					if (sw->pp[i]->connects[j] == sch_busarc) break;
				if (sw->pp[i]->connects[j] == NOARCPROTO)
				{
					/* this end cannot connect: fake the connection */
					px = (sw->x[0] + sw->x[1]) / 2;
					py = (sw->y[0] + sw->y[1]) / 2;
					defaultnodesize(sch_buspinprim, &xsize , &ysize);
					lx = px - xsize/2;   hx = lx + xsize;
					ly = py - ysize/2;   hy = ly + ysize;
					ni = newnodeinst(sch_buspinprim, lx, hx, ly, hy, 0, 0, cell);
					if (ni == NONODEINST) break;
					endobjectchange((INTBIG)ni, VNODEINST);
					pp = ni->proto->firstportproto;
					ai = newarcinst(gen_unroutedarc, defaultarcwidth(gen_unroutedarc),
						us_makearcuserbits(gen_unroutedarc), ni, pp, px, py,
							sw->ni[i], sw->pp[i], sw->x[i], sw->y[i], cell);
					if (ai == NOARCINST)
					{
						ttyputerr(_("Error making fake connection"));
						break;
					}
					endobjectchange((INTBIG)ai, VARCINST);
					sw->ni[i] = ni;
					sw->pp[i] = pp;
					sw->x[i] = px;
					sw->y[i] = py;
				}
			}
		}

		ai = newarcinst(sw->proto, wid, us_makearcuserbits(sw->proto),
			sw->ni[0], sw->pp[0], sw->x[0], sw->y[0],
			sw->ni[1], sw->pp[1], sw->x[1], sw->y[1], cell);
		if (ai == NOARCINST)
		{
			ttyputerr(_("Could not run a wire from %s to %s in cell %s"),
				describenodeinst(sw->ni[0]), describenodeinst(sw->ni[1]),
					describenodeproto(cell));
			continue;
		}

		/* negate the wire if requested */
		if (sw->ni[0]->temp1 != 0 &&
			estrcmp(sw->pp[0]->protoname, x_("y")) == 0)
		{
			ai->userbits |= ISNEGATED;
		} else if (sw->ni[1]->temp1 != 0 &&
			estrcmp(sw->pp[1]->protoname, x_("y")) == 0)
		{
			ai->userbits |= ISNEGATED | REVERSEEND;
		}
		endobjectchange((INTBIG)ai, VARCINST);
	}

	/* now look for implicit connections where "offpage" connectors touch */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != sch_offpageprim) continue;
		if (ni->firstportarcinst != NOPORTARCINST) continue;
		pp = ni->proto->firstportproto->nextportproto;
		portposition(ni, pp, &x, &y);
		sea = initsearch(x, x, y, y, cell);
		for(;;)
		{
			geom = nextobject(sea);
			if (geom == NOGEOM) break;
			if (!geom->entryisnode) continue;
			oni = geom->entryaddr.ni;
			if (oni == ni) continue;
			for(opp = oni->proto->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
			{
				portposition(oni, opp, &ox, &oy);
				if (ox != x || oy != y) continue;
				for(i=0; i<3; i++)
				{
					switch (i)
					{
						case 0: ap = sch_busarc;      break;
						case 1: ap = sch_wirearc;     break;
						case 2: ap = gen_unroutedarc; break;
					}
					for(j=0; pp->connects[j] != NOARCPROTO; j++)
						if (pp->connects[j] == ap) break;
					if (pp->connects[j] == NOARCPROTO) continue;
					for(j=0; opp->connects[j] != NOARCPROTO; j++)
						if (opp->connects[j] == ap) break;
					if (opp->connects[j] == NOARCPROTO) continue;
					break;
				}

				wid = defaultarcwidth(ap);
				bits = us_makearcuserbits(ap);
				ai = newarcinst(ap, wid, bits, ni, pp, x, y, oni, opp, x, y, cell);
				if (ai != NOARCINST)
					endobjectchange((INTBIG)ai, VARCINST);
				break;
			}
			if (opp != NOPORTPROTO) break;
		}
	}
}

/*
 * Routine to find the port on node "ni" that attaches to the wire from (x,y) to (ox,oy).
 * Returns NOPORTPROTO if not found.
 */
PORTPROTO *io_suewiredport(NODEINST *ni, INTBIG *x, INTBIG *y, INTBIG ox, INTBIG oy)
{
	REGISTER PORTPROTO *pp, *bestpp;
	REGISTER INTBIG dist, bestdist;
	INTBIG px, py;
	static POLYGON *poly = NOPOLYGON;

	/* make sure there is a polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);
	for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		shapeportpoly(ni, pp, poly, FALSE);
		if (isinside(*x, *y, poly)) return(pp);
	}
	if ((ni->lowx+ni->highx) / 2 != *x ||
		(ni->lowy+ni->highy) / 2 != *y) return(NOPORTPROTO);

	/* find port that is closest to OTHER end */
	bestdist = MAXINTBIG;
	bestpp = NOPORTPROTO;
	for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		portposition(ni, pp, &px, &py);
		dist = computedistance(px, py, ox, oy);
		if (dist > bestdist) continue;
		bestdist = dist;
		bestpp = pp;
	}
	portposition(ni, bestpp, x, y);
	return(bestpp);
}

/*
 * Routine to find the pin at (x, y) and return it.
 */
NODEINST *io_suefindpinnode(INTBIG x, INTBIG y, NODEPROTO *np, PORTPROTO **thepp)
{
	REGISTER GEOM *geom;
	REGISTER INTBIG sea;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	INTBIG px, py;

	*thepp = NOPORTPROTO;
	sea = initsearch(x, x, y, y, np);
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		if (!geom->entryisnode) continue;
		ni = geom->entryaddr.ni;

		/* find closest port */
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			/* make sure there is a polygon */
			portposition(ni, pp, &px, &py);
			if (px == x && py == y)
			{
				*thepp = pp;
				termsearch(sea);
				return(ni);
			}
		}
	}
	return(NONODEINST);
}

/*
 * Routine to find the node at (x, y) and return it.
 */
BOOLEAN io_suefindnode(INTBIG *x, INTBIG *y, INTBIG ox, INTBIG oy,
	NODEPROTO *np, NODEINST **rni, PORTPROTO **rpp, NODEINST *notthisnode, INTBIG lambda)
{
	REGISTER GEOM *geom;
	REGISTER INTBIG sea;
	REGISTER NODEINST *ni, *bestni;
	INTBIG plx, phx, ply, phy;
	REGISTER INTBIG dist, bestdist, thisx, thisy, bestx, besty, slop;
	REGISTER PORTPROTO *pp, *bestpp;
	static POLYGON *poly = NOPOLYGON;

	slop = lambda * 10;
	sea = initsearch(*x-slop, *x+slop, *y-slop, *y+slop, np);
	bestpp = NOPORTPROTO;
	for(;;)
	{
		geom = nextobject(sea);
		if (geom == NOGEOM) break;
		if (!geom->entryisnode) continue;
		ni = geom->entryaddr.ni;
		if (ni == notthisnode) continue;

		/* ignore pins */
		if (ni->proto == sch_wirepinprim) continue;

		/* find closest port */
		for(pp = ni->proto->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			/* make sure there is a polygon */
			(void)needstaticpolygon(&poly, 4, io_tool->cluster);

			/* get the polygon describing the port */
			shapeportpoly(ni, pp, poly, FALSE);
			getbbox(poly, &plx, &phx, &ply, &phy);

			/* find out if the line crosses the polygon */
			if (*x == ox)
			{
				/* line is vertical: look for intersection with polygon */
				if (ox < plx || ox > phx) continue;
				thisx = ox;
				thisy = (ply+phy)/2;
			} else if (*y == oy)
			{
				/* line is horizontal: look for intersection with polygon */
				if (oy < ply || oy > phy) continue;
				thisx = (plx+phx)/2;
				thisy = oy;
			} else
			{
				if (!isinside(ox, oy, poly)) continue;
				thisx = ox;
				thisy = oy;
			}

			dist = computedistance(ox, oy, thisx, thisy);

			/* LINTED "bestdist" used in proper order */
			if (bestpp == NOPORTPROTO || dist < bestdist)
			{
				bestpp = pp;
				bestni = ni;
				bestdist = dist;
				bestx = thisx;
				besty = thisy;
			}
		}
	}

	/* report the hit */
	if (bestpp == NOPORTPROTO) return(FALSE);
	*rni = bestni;   *rpp = bestpp;
	*x = bestx;      *y = besty;
	return(TRUE);
}

/*
 * Routine to find the SUE file "name" on disk, and read it into library "lib".
 * Returns NONODEPROTO if the file is not found or not read properly.
 */
NODEPROTO *io_suereadfromdisk(LIBRARY *lib, CHAR *name, void *dia)
{
	REGISTER INTBIG i, savelineno, filepos, savefilesize;
	CHAR suevarname[200], subfilename[300], savesuelastline[MAXLINE], saveorigline[MAXLINE],
		lastprogressmsg[MAXLINE], savecurline[MAXLINE], *truename;
	REGISTER FILE *f, *savefp;
	REGISTER NODEPROTO *proto;

	/* look for another "sue" file that describes this cell */
	for(i=0; i<io_suenumdirectories; i++)
	{
		estrcpy(subfilename, io_suedirectories[i]);
		estrcat(subfilename, name);
		estrcat(subfilename, x_(".sue"));
		f = xopen(subfilename, io_filetypesue, x_(""), &truename);
		if (f != 0)
		{
			savefp = io_suefilein;
			io_suefilein = f;
			for(i=0; i<MAXLINE; i++) savecurline[i] = io_suecurline[i];
			estrcpy(saveorigline, io_sueorigline);
			estrcpy(savesuelastline, io_suelastline);
			estrcpy(lastprogressmsg, DiaGetTextProgress(dia));
			estrcpy(suevarname, _("Reading "));
			estrcat(suevarname, name);
			estrcat(suevarname, x_("..."));
			DiaSetTextProgress(dia, suevarname);

			estrcpy(subfilename, name);
			savefilesize = io_suefilesize;
			savelineno = io_suelineno;
			io_suefilesize = filesize(io_suefilein);
			DiaSetProgress(dia, 0, io_suefilesize);
			io_suelineno = 0;
			(void)io_suereadfile(lib, subfilename, dia);
			xclose(io_suefilein);
			io_suefilein = savefp;
			io_suefilesize = savefilesize;
			io_suelineno = savelineno;
			estrcpy(io_suelastline, savesuelastline);
			estrcpy(io_sueorigline, saveorigline);
			for(i=0; i<MAXLINE; i++) io_suecurline[i] = savecurline[i];
			filepos = xtell(io_suefilein);
			DiaSetTextProgress(dia, lastprogressmsg);
			DiaSetProgress(dia, filepos, io_suefilesize);

			/* now try to find the cell in the library */
			proto = io_suegetnodeproto(lib, subfilename);
			return(proto);
		}
	}
	return(NONODEPROTO);
}

/*
 * Routine to read the next line from file "io_suefilein" and break
 * it up into space-separated keywords.  Returns the number
 * of keywords (-1 on EOF)
 */
INTBIG io_suegetnextline(CHAR **keywords, INTBIG curlydepth, void *dia)
{
	CHAR *pt;
	REGISTER INTBIG filepos, keypos, lineno;
	REGISTER BOOLEAN inblank;

	for(lineno=0; ; lineno++)
	{
		if (io_suelastline[0] == 0)
		{
			if (xfgets(io_suelastline, MAXLINE, io_suefilein)) return(-1);
			io_suelineno++;
			if ((io_suelineno%50) == 0)
			{
				filepos = xtell(io_suefilein);
				DiaSetProgress(dia, filepos, io_suefilesize);
			}
		}
		if (lineno == 0)
		{
			/* first line: use it */
			estrcpy(io_suecurline, io_suelastline);
		} else
		{
			/* subsequent line: use it only if a continuation */
			if (io_suelastline[0] != '+') break;
			estrcat(io_suecurline, &io_suelastline[1]);
		}
		io_suelastline[0] = 0;
	}
	estrcpy(io_sueorigline, io_suecurline);

	/* parse the line */
	inblank = TRUE;
	keypos = 0;
	for(pt=io_suecurline; *pt != 0; pt++)
	{
		if (*pt == '{') curlydepth++;
		if (*pt == '}') curlydepth--;
		if ((*pt == ' ' || *pt == '\t') && curlydepth == 0)
		{
			*pt = 0;
			inblank = TRUE;
		} else
		{
			if (inblank)
			{
				keywords[keypos++] = pt;
			}
			inblank = FALSE;
		}
	}
	return(keypos);
}

/*
 * Routine to add "directory" to the list of places where SUE files may be found.
 */
BOOLEAN io_sueadddirectory(CHAR *directory)
{
	REGISTER INTBIG newnumdir, i;
	CHAR **newdir;

	newnumdir = io_suenumdirectories + 1;
	newdir = (CHAR **)emalloc(newnumdir * (sizeof (CHAR *)), io_tool->cluster);
	if (newdir == 0) return(TRUE);
	for(i=0; i<io_suenumdirectories; i++)
		newdir[i] = io_suedirectories[i];
	if (allocstring(&newdir[io_suenumdirectories], directory, io_tool->cluster)) return(TRUE);
	if (io_suenumdirectories > 0) efree((CHAR *)io_suedirectories);
	io_suedirectories = newdir;
	io_suenumdirectories = newnumdir;
	return(FALSE);
}

void io_suecleardirectories(void)
{
	REGISTER INTBIG i;

	for(i=0; i<io_suenumdirectories; i++)
		efree((CHAR *)io_suedirectories[i]);
	if (io_suenumdirectories > 0) efree((CHAR *)io_suedirectories);
	io_suenumdirectories = 0;
}

/*************************** OBJECT MANAGEMENT ***************************/

SUEWIRE *io_suenewwire(void)
{
	SUEWIRE *sw;

	if (io_suefreewire != NOSUEWIRE)
	{
		sw = io_suefreewire;
		io_suefreewire = sw->nextsuewire;
	} else
	{
		sw = (SUEWIRE *)emalloc(sizeof(SUEWIRE), io_tool->cluster);
	}
	return(sw);
}

void io_suekillwire(SUEWIRE *sw)
{
	sw->nextsuewire = io_suefreewire;
	io_suefreewire = sw;
}

void io_suefreewires(SUEWIRE *firstsw)
{
	SUEWIRE *sw, *nextsw;

	for(sw = firstsw; sw != NOSUEWIRE; sw = nextsw)
	{
		nextsw = sw->nextsuewire;
		io_suekillwire(sw);
	}
}

SUENET *io_suenewnet(void)
{
	SUENET *sn;

	if (io_suefreenet != NOSUENET)
	{
		sn = io_suefreenet;
		io_suefreenet = sn->nextsuenet;
	} else
	{
		sn = (SUENET *)emalloc(sizeof(SUENET), io_tool->cluster);
	}
	sn->label = 0;
	return(sn);
}

void io_suekillnet(SUENET *sn)
{
	sn->nextsuenet = io_suefreenet;
	io_suefreenet = sn;
	if (sn->label != 0) efree(sn->label);
}

void io_suefreenets(SUENET *firstsn)
{
	SUENET *sn, *nextsn;

	for(sn = firstsn; sn != NOSUENET; sn = nextsn)
	{
		nextsn = sn->nextsuenet;
		io_suekillnet(sn);
	}
}
