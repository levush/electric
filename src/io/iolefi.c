/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iolefi.c
 * Input/output tool: LEF (Library Exchange Format) reader
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
 * Note that this reader was built by examining LEF files and reverse-engineering them.
 * It does not claim to be compliant with the LEF specification, but it also does not
 * claim to define a new specification.  It is merely incomplete.
 */

#include "config.h"
#include "global.h"
#include "egraphics.h"
#include "efunction.h"
#include "edialogs.h"
#include "iolefdef.h"
#include "tecart.h"
#include "tecgen.h"
#include "eio.h"
#include "usr.h"

/*************** LEF PATHS ***************/

#define NOLEFPATH ((LEFPATH *)-1)

typedef struct Ilefpath
{
	INTBIG     x[2], y[2];
	NODEINST  *ni[2];
	INTBIG     width;
	ARCPROTO  *arc;
	struct Ilefpath *nextlefpath;
} LEFPATH;

LEFPATH *io_leffreepath = NOLEFPATH;

/*************** MISCELLANEOUS ***************/

#define MAXLINE 2500

static INTBIG  io_leffilesize;
static INTBIG  io_leflineno;
static CHAR    io_lefline[MAXLINE];
static INTBIG  io_leflinepos;
       VIADEF *io_leffirstviadef = NOVIADEF;
static void   *io_lefprogressdialog;

/* prototypes for local routines */
static void       io_leffreepaths(LEFPATH *firstlp);
static ARCPROTO  *io_lefgetarc(INTBIG afunc);
static CHAR      *io_lefgetkeyword(FILE *f);
static NODEPROTO *io_lefgetnode(INTBIG lfunc);
static BOOLEAN    io_lefignoretosemicolon(FILE *f, CHAR *command);
static void       io_lefkillpath(LEFPATH *lp);
static LEFPATH   *io_lefnewpath(void);
static PORTPROTO *io_lefnewport(NODEPROTO *cell, NODEINST *ni, PORTPROTO *pp, CHAR *thename);
static void       io_lefparselayer(CHAR *layername, INTBIG *arcfunc, INTBIG *layerfunc);
static BOOLEAN    io_lefreadfile(FILE *f, LIBRARY *lib);
static BOOLEAN    io_lefreadlayer(FILE *f, LIBRARY *lib);
static BOOLEAN    io_lefreadmacro(FILE *f, LIBRARY *lib);
static BOOLEAN    io_lefreadvia(FILE *f, LIBRARY *lib);
static BOOLEAN    io_lefreadobs(FILE *f, NODEPROTO *cell);
static BOOLEAN    io_lefreadpin(FILE *f, NODEPROTO *cell);
static BOOLEAN    io_lefreadport(FILE *f, NODEPROTO *cell, CHAR *portname, INTBIG portbits);
static void       io_leffreevias(void);

/*
 * Routine to free all memory associated with this module.
 */
void io_freelefimemory(void)
{
	io_leffreevias();
}

BOOLEAN io_readleflibrary(LIBRARY *lib)
{
	FILE *fp;
	REGISTER BOOLEAN ret;
	CHAR *filename;

	/* open the file */
	fp = xopen(lib->libfile, io_filetypelef, x_(""), &filename);
	if (fp == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	/* remove any vias in the globals */
	io_leffreevias();

	/* prepare for input */
	io_leffilesize = filesize(fp);
	io_lefprogressdialog = DiaInitProgress(_("Reading LEF file..."), 0);
	if (io_lefprogressdialog == 0)
	{
		xclose(fp);
		return(TRUE);
	}
	DiaSetProgress(io_lefprogressdialog, 0, io_leffilesize);
	io_leflineno = 0;
	io_leflinepos = 0;
	io_lefline[0] = 0;

	/* read the file */
	ret = io_lefreadfile(fp, lib);

	/* clean up */
	DiaDoneProgress(io_lefprogressdialog);
	xclose(fp);
	if (!ret) ttyputmsg(_("LEF file %s is read"), lib->libfile); else
		ttyputmsg(_("Error reading LEF file %s"), lib->libfile);
	return(FALSE);
}

void io_leffreevias(void)
{
	VIADEF *vd;

	while (io_leffirstviadef != NOVIADEF)
	{
		vd = io_leffirstviadef;
		io_leffirstviadef = io_leffirstviadef->nextviadef;
		efree((CHAR *)vd->vianame);
		efree((CHAR *)vd);
	}
}

/*
 * Routine to read the LEF file in "f".
 */
BOOLEAN io_lefreadfile(FILE *f, LIBRARY *lib)
{
	REGISTER CHAR *key;

	for(;;)
	{
		/* get the next keyword */
		key = io_lefgetkeyword(f);
		if (key == 0) break;
		if (namesame(key, x_("LAYER")) == 0)
		{
			if (io_lefreadlayer(f, lib)) return(TRUE);
		}
		if (namesame(key, x_("MACRO")) == 0)
		{
			if (io_lefreadmacro(f, lib)) return(TRUE);
		}
		if (namesame(key, x_("VIA")) == 0)
		{
			if (io_lefreadvia(f, lib)) return(TRUE);
		}
	}
	return(FALSE);
}

void io_lefparselayer(CHAR *layername, INTBIG *arcfunc, INTBIG *layerfunc)
{
	REGISTER INTBIG laynum, j;

	*arcfunc = APUNKNOWN;
	*layerfunc = LFUNKNOWN;

	j = 0;
	if (namesamen(layername, x_("METAL"), 5) == 0) j = 5; else
		if (namesamen(layername, x_("MET"), 3) == 0) j = 3;
	if (j != 0)
	{
		laynum = eatoi(&layername[j]);
		switch (laynum)
		{
			case 1:  *arcfunc = APMETAL1;   *layerfunc = LFMETAL1;   break;
			case 2:  *arcfunc = APMETAL2;   *layerfunc = LFMETAL2;   break;
			case 3:  *arcfunc = APMETAL3;   *layerfunc = LFMETAL3;   break;
			case 4:  *arcfunc = APMETAL4;   *layerfunc = LFMETAL4;   break;
			case 5:  *arcfunc = APMETAL5;   *layerfunc = LFMETAL5;   break;
			case 6:  *arcfunc = APMETAL6;   *layerfunc = LFMETAL6;   break;
			case 7:  *arcfunc = APMETAL7;   *layerfunc = LFMETAL7;   break;
			case 8:  *arcfunc = APMETAL8;   *layerfunc = LFMETAL8;   break;
			case 9:  *arcfunc = APMETAL9;   *layerfunc = LFMETAL9;   break;
			case 10: *arcfunc = APMETAL10;  *layerfunc = LFMETAL10;  break;
			case 11: *arcfunc = APMETAL11;  *layerfunc = LFMETAL11;  break;
			case 12: *arcfunc = APMETAL12;  *layerfunc = LFMETAL12;  break;
		}
		return;
	}

	if (namesamen(layername, x_("POLY"), 4) == 0)
	{
		laynum = eatoi(&layername[4]);
		switch (laynum)
		{
			case 1: *arcfunc = APPOLY1;   *layerfunc = LFPOLY1;   break;
			case 2: *arcfunc = APPOLY2;   *layerfunc = LFPOLY2;   break;
			case 3: *arcfunc = APPOLY3;   *layerfunc = LFPOLY3;   break;
		}
		return;
	}

	if (namesame(layername, x_("PDIFF")) == 0)
	{
		*arcfunc = APDIFFP;
		*layerfunc = LFDIFF | LFPTYPE;
		return;
	}
	if (namesame(layername, x_("NDIFF")) == 0)
	{
		*arcfunc = APDIFFN;
		*layerfunc = LFDIFF | LFNTYPE;
		return;
	}
	if (namesame(layername, x_("DIFF")) == 0)
	{
		*arcfunc = APDIFF;
		*layerfunc = LFDIFF;
		return;
	}

	if (namesame(layername, x_("CONT")) == 0)
	{
		*layerfunc = LFCONTACT1;
		return;
	}
	if (namesamen(layername, x_("VIA"), 3) == 0)
	{
		laynum = eatoi(&layername[3]);
		switch (laynum)
		{
			case 0:
			case 1:
			case 12: *layerfunc = LFCONTACT2;   break;

			case 2:
			case 23: *layerfunc = LFCONTACT3;   break;

			case 3:
			case 34: *layerfunc = LFCONTACT4;   break;

			case 4:
			case 45: *layerfunc = LFCONTACT5;   break;

			case 5:
			case 56: *layerfunc = LFCONTACT6;   break;

			case 6:
			case 67: *layerfunc = LFCONTACT7;   break;

			case 7:
			case 78: *layerfunc = LFCONTACT8;   break;

			case 8:
			case 89: *layerfunc = LFCONTACT9;   break;

			case 9:  *layerfunc = LFCONTACT10;  break;

			case 10: *layerfunc = LFCONTACT11;  break;

			case 11: *layerfunc = LFCONTACT12;  break;
		}
	}
}

BOOLEAN io_lefreadlayer(FILE *f, LIBRARY *lib)
{
	REGISTER CHAR *layername, *key;
	CHAR curkey[200];
	REGISTER INTBIG defwidth;
	INTBIG afunc, layerfunc;
	REGISTER ARCPROTO *ap;
	float v;

	layername = io_lefgetkeyword(f);
	if (layername == 0)
	{
		ttyputerr(_("EOF parsing LAYER header"));
		return(TRUE);
	}
	io_lefparselayer(layername, &afunc, &layerfunc);
	if (afunc == APUNKNOWN) ap = NOARCPROTO; else
	{
		for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
			if (((INTBIG)((ap->userbits&AFUNCTION)>>AFUNCTIONSH)) == afunc) break;
	}

	for(;;)
	{
		/* get the next keyword */
		key = io_lefgetkeyword(f);
		if (key == 0)
		{
			ttyputerr(_("EOF parsing LAYER"));
			return(TRUE);
		}

		if (namesame(key, x_("END")) == 0)
		{
			key = io_lefgetkeyword(f);
			break;
		}

		if (namesame(key, x_("WIDTH")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading WIDTH"));
				return(TRUE);
			}
			v = (float)eatof(key);
			defwidth = scalefromdispunit(v, DISPUNITMIC);
			if (ap != NOARCPROTO)
			{
				setval((INTBIG)ap, VARCPROTO, x_("IO_lef_width"), defwidth, VINTEGER);
			}
			if (io_lefignoretosemicolon(f, x_("WIDTH"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("TYPE")) == 0 || namesame(key, x_("SPACING")) == 0 ||
			namesame(key, x_("PITCH")) == 0 || namesame(key, x_("DIRECTION")) == 0 ||
			namesame(key, x_("CAPACITANCE")) == 0 || namesame(key, x_("RESISTANCE")) == 0)
		{
			estrcpy(curkey, key);
			if (io_lefignoretosemicolon(f, curkey)) return(TRUE);
			continue;
		}
	}
	return(FALSE);
}

BOOLEAN io_lefreadmacro(FILE *f, LIBRARY *lib)
{
	REGISTER CHAR *cellname, *key;
	REGISTER INTBIG ox, oy, blx, bhx, bly, bhy, dx, dy;
	INTBIG sx, sy;
	float x, y;
	REGISTER NODEPROTO *cell;
	REGISTER NODEINST *ni;
	CHAR curkey[200];
	REGISTER void *infstr;

	cellname = io_lefgetkeyword(f);
	if (cellname == 0)
	{
		ttyputerr(_("EOF parsing MACRO header"));
		return(TRUE);
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, cellname);
	addstringtoinfstr(infstr, x_("{sk}"));
	cellname = returninfstr(infstr);
	cell = us_newnodeproto(cellname, lib);
	if (cell == NONODEPROTO)
	{
		ttyputerr(_("Cannot create cell '%s'"), cellname);
		return(TRUE);
	}

	for(;;)
	{
		/* get the next keyword */
		key = io_lefgetkeyword(f);
		if (key == 0)
		{
			ttyputerr(_("EOF parsing MACRO"));
			return(TRUE);
		}

		if (namesame(key, x_("END")) == 0)
		{
			key = io_lefgetkeyword(f);
			break;
		}

		if (namesame(key, x_("SOURCE")) == 0 || namesame(key, x_("FOREIGN")) == 0 ||
			namesame(key, x_("SYMMETRY")) == 0 || namesame(key, x_("SITE")) == 0 ||
			namesame(key, x_("CLASS")) == 0 || namesame(key, x_("LEQ")) == 0 ||
			namesame(key, x_("POWER")) == 0)
		{
			estrcpy(curkey, key);
			if (io_lefignoretosemicolon(f, curkey)) return(TRUE);
			continue;
		}

		if (namesame(key, x_("ORIGIN")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading ORIGIN X"));
				return(TRUE);
			}
			x = (float)eatof(key);
			ox = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading ORIGIN Y"));
				return(TRUE);
			}
			y = (float)eatof(key);
			oy = scalefromdispunit(y, DISPUNITMIC);
			if (io_lefignoretosemicolon(f, x_("ORIGIN"))) return(TRUE);

			/* create or move the cell-center node */
			for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				if (ni->proto == gen_cellcenterprim) break;
			if (ni == NONODEINST)
			{
				defaultnodesize(gen_cellcenterprim, &sx, &sy);
				blx = ox - sx/2;   bhx = blx + sx;
				bly = oy - sy/2;   bhy = bly + sy;
				ni = newnodeinst(gen_cellcenterprim, blx, bhx, bly, bhy, 0, 0, cell);
				if (ni == NONODEINST)
				{
					ttyputerr(_("Line %ld: Cannot create cell center node"), io_leflineno);
					return(TRUE);
				}
				ni->userbits |= HARDSELECTN|NVISIBLEINSIDE;
			} else
			{
				startobjectchange((INTBIG)ni, VNODEINST);
				dx = ox - (ni->lowx + ni->highx) / 2;
				dy = oy - (ni->lowy + ni->highy) / 2;
				modifynodeinst(ni, dx, dy, dx, dy, 0, 0);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}

		if (namesame(key, x_("SIZE")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading SIZE X"));
				return(TRUE);
			}
			x = (float)eatof(key);
			sx = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading SIZE 'BY'"));
				return(TRUE);
			}
			if (namesame(key, x_("BY")) != 0)
			{
				ttyputerr(_("Line %ld: Expected 'by' in SIZE"), io_leflineno);
				return(TRUE);
			}

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading SIZE Y"));
				return(TRUE);
			}
			y = (float)eatof(key);
			sy = scalefromdispunit(y, DISPUNITMIC);
			if (io_lefignoretosemicolon(f, x_("SIZE"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("PIN")) == 0)
		{
			if (io_lefreadpin(f, cell)) return(TRUE);
			continue;
		}

		if (namesame(key, x_("OBS")) == 0)
		{
			if (io_lefreadobs(f, cell)) return(TRUE);
			continue;
		}

		ttyputerr(_("Line %ld: Unknown MACRO keyword (%s)"), io_leflineno, key);
		return(TRUE);
	}
	return(FALSE);
}

BOOLEAN io_lefreadvia(FILE *f, LIBRARY *lib)
{
	REGISTER CHAR *vianame, *key;
	REGISTER INTBIG lx, hx, ly, hy, i, lay1found, lay2found;
	INTBIG afunc, layerfunc;
	NODEPROTO *np;
	ARCPROTO *ap;
	float v;
	BOOLEAN ignoredefault;
	REGISTER PORTPROTO *pp;
	REGISTER VIADEF *vd;

	/* get the via name */
	vianame = io_lefgetkeyword(f);
	if (vianame == 0) return(TRUE);

	/* create a new via definition */
	vd = (VIADEF *)emalloc(sizeof (VIADEF), io_tool->cluster);
	if (vd == 0) return(TRUE);
	(void)allocstring(&vd->vianame, vianame, io_tool->cluster);
	vd->sx = vd->sy = 0;
	vd->via = NONODEPROTO;
	vd->lay1 = vd->lay2 = NOARCPROTO;
	vd->nextviadef = io_leffirstviadef;
	io_leffirstviadef = vd;

	ignoredefault = TRUE;
	for(;;)
	{
		/* get the next keyword */
		key = io_lefgetkeyword(f);
		if (key == 0) return(TRUE);
		if (ignoredefault)
		{
			ignoredefault = FALSE;
			if (namesame(key, x_("DEFAULT")) == 0) continue;
		}
		if (namesame(key, x_("END")) == 0)
		{
			key = io_lefgetkeyword(f);
			break;
		}
		if (namesame(key, x_("RESISTANCE")) == 0)
		{
			if (io_lefignoretosemicolon(f, key)) return(TRUE);
			continue;
		}
		if (namesame(key, x_("LAYER")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0) return(TRUE);
			io_lefparselayer(key, &afunc, &layerfunc);
			if (afunc == APUNKNOWN) ap = NOARCPROTO; else
			{
				for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
					if (((INTBIG)((ap->userbits&AFUNCTION)>>AFUNCTIONSH)) == afunc) break;
			}
			if (ap != NOARCPROTO)
			{
				if (vd->lay1 == NOARCPROTO) vd->lay1 = ap; else
					vd->lay2 = ap;
			}
			if (io_lefignoretosemicolon(f, x_("LAYER"))) return(TRUE);
			continue;
		}
		if (namesame(key, x_("RECT")) == 0)
		{
			/* handle definition of a via rectangle */
			key = io_lefgetkeyword(f);
			if (key == 0) return(TRUE);
			v = (float)eatof(key);
			lx = scalefromdispunit(v, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0) return(TRUE);
			v = (float)eatof(key);
			ly = scalefromdispunit(v, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0) return(TRUE);
			v = (float)eatof(key);
			hx = scalefromdispunit(v, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0) return(TRUE);
			v = (float)eatof(key);
			hy = scalefromdispunit(v, DISPUNITMIC);

			/* accumulate largest layer size */
			if (hx-lx > vd->sx) vd->sx = hx - lx;
			if (hy-ly > vd->sy) vd->sy = hy - ly;

			if (io_lefignoretosemicolon(f, x_("RECT"))) return(TRUE);
			continue;
		}
	}
	if (vd->lay1 != NOARCPROTO && vd->lay2 != NOARCPROTO)
	{
		for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (((np->userbits&NFUNCTION)>>NFUNCTIONSH) != NPCONTACT) continue;
			pp = np->firstportproto;
			lay1found = lay2found = 0;
			for(i=0; pp->connects[i] != NOARCPROTO; i++)
			{
				if (pp->connects[i] == vd->lay1) lay1found = 1;
				if (pp->connects[i] == vd->lay2) lay2found = 1;
			}
			if (lay1found != 0 && lay2found != 0) break;
		}
		vd->via = np;
	}
	return(FALSE);
}

BOOLEAN io_lefreadpin(FILE *f, NODEPROTO *cell)
{
	REGISTER CHAR *key;
	REGISTER INTBIG portbits, usebits;
	CHAR curkey[200], pinname[200];

	/* get the pin name */
	key = io_lefgetkeyword(f);
	if (key == 0)
	{
		ttyputerr(_("EOF parsing PIN name"));
		return(TRUE);
	}
	estrcpy(pinname, key);

	portbits = usebits = 0;
	for(;;)
	{
		key = io_lefgetkeyword(f);
		if (key == 0)
		{
			ttyputerr(_("EOF parsing PIN"));
			return(TRUE);
		}

		if (namesame(key, x_("END")) == 0)
		{
			key = io_lefgetkeyword(f);
			break;
		}

		if (namesame(key, x_("SHAPE")) == 0 || namesame(key, x_("CAPACITANCE")) == 0 ||
			namesame(key, x_("ANTENNASIZE")) == 0)
		{
			estrcpy(curkey, key);
			if (io_lefignoretosemicolon(f, curkey)) return(TRUE);
			continue;
		}

		if (namesame(key, x_("USE")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading USE clause"));
				return(TRUE);
			}
			if (namesame(key, x_("POWER")) == 0) usebits = PWRPORT; else
			if (namesame(key, x_("GROUND")) == 0) usebits = GNDPORT; else
			if (namesame(key, x_("CLOCK")) == 0) usebits = CLKPORT; else
			{
				ttyputerr(_("Line %ld: Unknown USE keyword (%s)"),
					io_leflineno, key);
			}
			if (io_lefignoretosemicolon(f, x_("USE"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("DIRECTION")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading DIRECTION clause"));
				return(TRUE);
			}
			if (namesame(key, x_("INPUT")) == 0) portbits = INPORT; else
			if (namesame(key, x_("OUTPUT")) == 0) portbits = OUTPORT; else
			if (namesame(key, x_("INOUT")) == 0) portbits = BIDIRPORT; else
			{
				ttyputerr(_("Line %ld: Unknown DIRECTION keyword (%s)"),
					io_leflineno, key);
			}
			if (io_lefignoretosemicolon(f, x_("DIRECTION"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("PORT")) == 0)
		{
			if (usebits != 0) portbits = usebits;
			if (io_lefreadport(f, cell, pinname, portbits))
				return(TRUE);
			continue;
		}

		ttyputerr(_("Line %ld: Unknown PIN keyword (%s)"), io_leflineno, key);
		return(TRUE);
	}
	return(FALSE);
}

BOOLEAN io_lefreadport(FILE *f, NODEPROTO *cell, CHAR *portname, INTBIG portbits)
{
	REGISTER CHAR *key;
	REGISTER INTBIG i, j, lx, hx, ly, hy, intx, inty, first,
		intwidth, lastintx, lastinty, sea, cx, cy;
	INTBIG afunc, layerfunc, sx, sy;
	REGISTER INTBIG afunc1, afunc2, found1, found2;
	REGISTER ARCPROTO *ap, *ap1, *ap2;
	REGISTER NODEPROTO *pin, *plnp;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER GEOM *geom;
	float width, x, y;
	REGISTER LEFPATH *lefpaths, *lp, *olp;

	ap = NOARCPROTO;
	plnp = NONODEPROTO;
	lefpaths = NOLEFPATH;
	first = 1;
	for(;;)
	{
		key = io_lefgetkeyword(f);
		if (key == 0)
		{
			ttyputerr(_("EOF parsing PORT"));
			return(TRUE);
		}

		if (namesame(key, x_("END")) == 0)
		{
			break;
		}

		if (namesame(key, x_("LAYER")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading LAYER clause"));
				return(TRUE);
			}
			io_lefparselayer(key, &afunc, &layerfunc);
			if (afunc == APUNKNOWN) ap = NOARCPROTO; else
				ap = io_lefgetarc(afunc);
			plnp = io_lefgetnode(layerfunc);
			if (io_lefignoretosemicolon(f, x_("LAYER"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("WIDTH")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading WIDTH clause"));
				return(TRUE);
			}
			width = (float)eatof(key);
			intwidth = scalefromdispunit(width, DISPUNITMIC);
			if (io_lefignoretosemicolon(f, x_("WIDTH"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("RECT")) == 0)
		{
			if (plnp == NONODEPROTO)
			{
				ttyputerr(_("Line %ld: No layers for RECT"), io_leflineno);
				return(TRUE);
			}
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT low X"));
				return(TRUE);
			}
			x = (float)eatof(key);
			lx = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT low Y"));
				return(TRUE);
			}
			y = (float)eatof(key);
			ly = scalefromdispunit(y, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT high X"));
				return(TRUE);
			}
			x = (float)eatof(key);
			hx = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT high Y"));
				return(TRUE);
			}
			y = (float)eatof(key);
			hy = scalefromdispunit(y, DISPUNITMIC);

			if (io_lefignoretosemicolon(f, x_("RECT"))) return(TRUE);

			/* make the pin */
			if (hx < lx) { j = hx;   hx = lx;   lx = j; }
			if (hy < ly) { j = hy;   hy = ly;   ly = j; }
			ni = newnodeinst(plnp, lx, hx, ly, hy, 0, 0, cell);
			if (ni == NONODEINST)
			{
				ttyputerr(_("Line %ld: Cannot create pin for RECT"), io_leflineno);
				return(TRUE);
			}
			endobjectchange((INTBIG)ni, VNODEINST);

			if (first != 0)
			{
				/* create the port on the first pin */
				first = 0;
				pp = io_lefnewport(cell, ni, plnp->firstportproto, portname);
				pp->userbits = (pp->userbits & ~STATEBITS) | portbits;
			}
			continue;
		}

		if (namesame(key, x_("PATH")) == 0)
		{
			if (ap == NOARCPROTO)
			{
				ttyputerr(_("Line %ld: No layers for PATH"), io_leflineno);
				return(TRUE);
			}
			for(i=0; ; i++)
			{
				key = io_lefgetkeyword(f);
				if (key == 0)
				{
					ttyputerr(_("EOF reading PATH clause"));
					return(TRUE);
				}
				if (estrcmp(key, x_(";")) == 0) break;
				x = (float)eatof(key);

				key = io_lefgetkeyword(f);
				if (key == 0)
				{
					ttyputerr(_("EOF reading PATH clause"));
					return(TRUE);
				}
				y = (float)eatof(key);

				/* plot this point */
				intx = scalefromdispunit(x, DISPUNITMIC);
				inty = scalefromdispunit(y, DISPUNITMIC);
				if (i != 0)
				{
					/* queue path */
					lp = io_lefnewpath();
					if (lp == NOLEFPATH) return(TRUE);

					/* LINTED "lastintx" used in proper order */
					lp->x[0] = lastintx;     lp->x[1] = intx;

					/* LINTED "lastinty" used in proper order */
					lp->y[0] = lastinty;     lp->y[1] = inty;
					lp->ni[0] = NONODEINST;  lp->ni[1] = NONODEINST;
					lp->width = intwidth;
					lp->arc = ap;
					lp->nextlefpath = lefpaths;
					lefpaths = lp;
				}
				lastintx = intx;   lastinty = inty;
			}
			continue;
		}

		if (namesame(key, x_("VIA")) == 0)
		{
			/* get the coordinates */
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading VIA clause"));
				return(TRUE);
			}
			x = (float)eatof(key);
			intx = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading VIA clause"));
				return(TRUE);
			}
			y = (float)eatof(key);
			inty = scalefromdispunit(y, DISPUNITMIC);

			/* find the proper via */
			key = io_lefgetkeyword(f);
			afunc1 = afunc2 = APUNKNOWN;
			if (namesame(key, x_("via1")) == 0)
			{
				afunc1 = APMETAL1;
				afunc2 = APMETAL2;
			} else if (namesame(key, x_("via2")) == 0)
			{
				afunc1 = APMETAL2;
				afunc2 = APMETAL3;
			} else if (namesame(key, x_("via3")) == 0)
			{
				afunc1 = APMETAL3;
				afunc2 = APMETAL4;
			} else if (namesame(key, x_("via4")) == 0)
			{
				afunc1 = APMETAL4;
				afunc2 = APMETAL5;
			} else
			{
				ttyputerr(_("Line %ld: Unknown VIA type (%s)"), io_leflineno, key);
				return(TRUE);
			}
			ap1 = io_lefgetarc(afunc1);
			if (ap1 == NOARCPROTO) return(TRUE);
			ap2 = io_lefgetarc(afunc2);
			if (ap2 == NOARCPROTO) return(TRUE);
			for(pin = el_curtech->firstnodeproto; pin != NONODEPROTO; pin = pin->nextnodeproto)
			{
				if (((pin->userbits&NFUNCTION)>>NFUNCTIONSH) != NPCONTACT) continue;
				pp = pin->firstportproto;
				found1 = found2 = 0;
				for(i=0; pp->connects[i] != NOARCPROTO; i++)
				{
					if (pp->connects[i] == ap1) found1 = 1;
					if (pp->connects[i] == ap2) found2 = 1;
				}
				if (found1 != 0 && found2 != 0) break;
			}
			if (pin == NONODEPROTO)
			{
				ttyputerr(_("Line %ld: No Via in current technology connects %s to %s"),
					io_leflineno, arcfunctionname(afunc1),
					arcfunctionname(afunc2));
				return(TRUE);
			}
			if (io_lefignoretosemicolon(f, x_("VIA"))) return(TRUE);

			/* create the via */
			defaultnodesize(pin, &sx, &sy);
			lx = intx - sx / 2;   hx = lx + sx;
			ly = inty - sy / 2;   hy = ly + sy;
			ni = newnodeinst(pin, lx, hx, ly, hy, 0, 0, cell);
			if (ni == NONODEINST)
			{
				ttyputerr(_("Line %ld: Cannot create VIA for PATH"), io_leflineno);
				return(TRUE);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}

		ttyputerr(_("Line %ld: Unknown PORT keyword (%s)"), io_leflineno, key);
		return(TRUE);
	}

	/* look for paths that end at vias */
	for(lp = lefpaths; lp != NOLEFPATH; lp = lp->nextlefpath)
	{
		for(i=0; i<2; i++)
		{
			if (lp->ni[i] != NONODEINST) continue;
			sea = initsearch(lp->x[i], lp->x[i], lp->y[i], lp->y[i], cell);
			for(;;)
			{
				geom = nextobject(sea);
				if (geom == NOGEOM) break;
				if (!geom->entryisnode) continue;
				ni = geom->entryaddr.ni;
				cx = (ni->lowx + ni->highx) / 2;
				cy = (ni->lowy + ni->highy) / 2;
				if (cx != lp->x[i] || cy != lp->y[i]) continue;
				lp->ni[i] = ni;
				break;
			}
			if (lp->ni[i] == NONODEINST) continue;

			/* use this via at other paths which meet here */
			for(olp = lefpaths; olp != NOLEFPATH; olp = olp->nextlefpath)
			{
				for(j=0; j<2; j++)
				{
					if (olp->ni[j] != NONODEINST) continue;
					if (olp->x[j] != lp->x[i] || olp->y[j] != lp->y[i]) continue;
					olp->ni[j] = lp->ni[i];
				}
			}
		}
	}

	/* create pins at all other path ends */
	for(lp = lefpaths; lp != NOLEFPATH; lp = lp->nextlefpath)
	{
		for(i=0; i<2; i++)
		{
			if (lp->ni[i] != NONODEINST) continue;
			pin = getpinproto(lp->arc);
			if (pin == NONODEPROTO) continue;
			defaultnodesize(pin, &sx, &sy);
			lx = lp->x[i] - sx / 2;   hx = lx + sx;
			ly = lp->y[i] - sy / 2;   hy = ly + sy;
			lp->ni[i] = newnodeinst(pin, lx, hx, ly, hy, 0, 0, cell);
			if (lp->ni[i] == NONODEINST)
			{
				ttyputerr(_("Line %ld: Cannot create pin for PATH"), io_leflineno);
				return(TRUE);
			}
			endobjectchange((INTBIG)lp->ni[i], VNODEINST);

			/* use this pin at other paths which meet here */
			for(olp = lefpaths; olp != NOLEFPATH; olp = olp->nextlefpath)
			{
				for(j=0; j<2; j++)
				{
					if (olp->ni[j] != NONODEINST) continue;
					if (olp->x[j] != lp->x[i] || olp->y[j] != lp->y[i]) continue;
					olp->ni[j] = lp->ni[i];
				}
			}
		}
	}

	/* now instantiate the paths */
	for(lp = lefpaths; lp != NOLEFPATH; lp = lp->nextlefpath)
	{
		ai = newarcinst(lp->arc, lp->width, us_makearcuserbits(lp->arc),
			lp->ni[0], lp->ni[0]->proto->firstportproto, lp->x[0], lp->y[0],
				lp->ni[1], lp->ni[1]->proto->firstportproto, lp->x[1], lp->y[1], cell);
		if (ai == NOARCINST)
		{
			ttyputerr(_("Line %ld: Cannot create arc for PATH"), io_leflineno);
			return(TRUE);
		}
	}
	io_leffreepaths(lefpaths);

	return(FALSE);
}

/*
 * Routine to create a port called "thename" on port "pp" of node "ni" in cell "cell".
 * The name is modified if it already exists.
 */
PORTPROTO *io_lefnewport(NODEPROTO *cell, NODEINST *ni, PORTPROTO *pp, CHAR *thename)
{
	REGISTER PORTPROTO *ppt;
	REGISTER INTBIG i, leng;
	REGISTER CHAR *newname, *portname;

	portname = thename;
	newname = 0;
	for(i=0; ; i++)
	{
		ppt = getportproto(cell, portname);
		if (ppt == NOPORTPROTO)
		{
			ppt = newportproto(cell, ni, pp, portname);
			break;
		}

		/* make space for modified name */
		if (newname == 0)
		{
			leng = estrlen(thename)+10;
			newname = (CHAR *)emalloc(leng * SIZEOFCHAR, el_tempcluster);
		}
		esnprintf(newname, leng, x_("%s-%ld"), thename, i);
		portname = newname;
	}
	if (newname != 0) efree(newname);
	return(ppt);
}

BOOLEAN io_lefreadobs(FILE *f, NODEPROTO *cell)
{
	REGISTER CHAR *key;
	INTBIG lfunc, arcfunc;
	REGISTER INTBIG lx, hx, ly, hy, j;
	float x, y;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;

	np = NONODEPROTO;
	for(;;)
	{
		key = io_lefgetkeyword(f);
		if (key == 0)
		{
			ttyputerr(_("EOF parsing OBS"));
			return(TRUE);
		}

		if (namesame(key, x_("END")) == 0) break;

		if (namesame(key, x_("LAYER")) == 0)
		{
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading LAYER clause"));
				return(TRUE);
			}
			io_lefparselayer(key, &arcfunc, &lfunc);
			if (lfunc == LFUNKNOWN)
			{
				ttyputerr(_("Line %ld: Unknown layer name (%s)"), io_leflineno, key);
				return(TRUE);
			}
			np = io_lefgetnode(lfunc);
			if (np == NONODEPROTO) return(TRUE);
			if (io_lefignoretosemicolon(f, x_("LAYER"))) return(TRUE);
			continue;
		}

		if (namesame(key, x_("RECT")) == 0)
		{
			if (np == NONODEPROTO)
			{
				ttyputerr(_("Line %ld: No layers for RECT"), io_leflineno);
				return(TRUE);
			}
			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT low X"));
				return(TRUE);
			}
			x = (float)eatof(key);
			lx = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT low Y"));
				return(TRUE);
			}
			y = (float)eatof(key);
			ly = scalefromdispunit(y, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT high X"));
				return(TRUE);
			}
			x = (float)eatof(key);
			hx = scalefromdispunit(x, DISPUNITMIC);

			key = io_lefgetkeyword(f);
			if (key == 0)
			{
				ttyputerr(_("EOF reading RECT high Y"));
				return(TRUE);
			}
			y = (float)eatof(key);
			hy = scalefromdispunit(y, DISPUNITMIC);

			if (io_lefignoretosemicolon(f, x_("RECT"))) return(TRUE);

			/* make the pin */
			if (hx < lx) { j = hx;   hx = lx;   lx = j; }
			if (hy < ly) { j = hy;   hy = ly;   ly = j; }
			ni = newnodeinst(np, lx, hx, ly, hy, 0, 0, cell);
			if (ni == NONODEINST)
			{
				ttyputerr(_("Line %ld: Cannot create node for RECT"), io_leflineno);
				return(TRUE);
			}
			endobjectchange((INTBIG)ni, VNODEINST);
			continue;
		}
	}
	return(FALSE);
}

BOOLEAN io_lefignoretosemicolon(FILE *f, CHAR *command)
{
	REGISTER CHAR *key;

	/* ignore up to the next semicolon */
	for(;;)
	{
		key = io_lefgetkeyword(f);
		if (key == 0)
		{
			ttyputerr(_("EOF parsing %s"), command);
			return(TRUE);
		}
		if (estrcmp(key, x_(";")) == 0) break;
	}
	return(FALSE);
}

CHAR *io_lefgetkeyword(FILE *f)
{
	REGISTER CHAR *ret;
	REGISTER INTBIG filepos, i;

	/* keep reading from file until something is found on a line */
	while (io_lefline[io_leflinepos] == 0)
	{
		/* read a line from the file, exit at EOF */
		if (xfgets(io_lefline, MAXLINE, f)) return(0);
		io_leflineno++;
		if ((io_leflineno%50) == 0)
		{
			filepos = xtell(f);
			DiaSetProgress(io_lefprogressdialog, filepos, io_leffilesize);
		}

		/* remove comments from the line */
		for(i=0; io_lefline[i] != 0; i++)
			if (io_lefline[i] == '#')
		{
			io_lefline[i] = 0;
			break;
		}

		/* look for the first text on the line */
		io_leflinepos = 0;
		while (io_lefline[io_leflinepos] == ' ' || io_lefline[io_leflinepos] == '\t')
			io_leflinepos++;
	}

	/* remember where the keyword begins */
	ret = &io_lefline[io_leflinepos];

	/* scan to the end of the keyword */
	while (io_lefline[io_leflinepos] != 0 && io_lefline[io_leflinepos] != ' ' &&
		io_lefline[io_leflinepos] != '\t') io_leflinepos++;
	if (io_lefline[io_leflinepos] != 0) io_lefline[io_leflinepos++] = 0;

	/* advance to the start of the next keyword */
	while (io_lefline[io_leflinepos] == ' ' || io_lefline[io_leflinepos] == '\t')
		io_leflinepos++;
	return(ret);
}

NODEPROTO *io_lefgetnode(INTBIG lfunc)
{
	REGISTER NODEPROTO *np;

	np = getpurelayernode(el_curtech, -1, lfunc);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Line %ld: Layer not in current technology"),
			io_leflineno);
		return(NONODEPROTO);
	}
	return(np);
}

ARCPROTO *io_lefgetarc(INTBIG afunc)
{
	REGISTER ARCPROTO *ap;

	for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if ((INTBIG)((ap->userbits&AFUNCTION)>>AFUNCTIONSH) == afunc) break;
	if (ap == NOARCPROTO)
	{
		ttyputerr(_("Line %ld: Layer %s not in current technology"),
			io_leflineno, arcfunctionname(afunc));
		return(NOARCPROTO);
	}
	return(ap);
}

LEFPATH *io_lefnewpath(void)
{
	LEFPATH *lp;

	if (io_leffreepath != NOLEFPATH)
	{
		lp = io_leffreepath;
		io_leffreepath = lp->nextlefpath;
	} else
	{
		lp = (LEFPATH *)emalloc(sizeof(LEFPATH), io_tool->cluster);
		if (lp == 0) lp = NOLEFPATH;
	}
	return(lp);
}

void io_lefkillpath(LEFPATH *lp)
{
	lp->nextlefpath = io_leffreepath;
	io_leffreepath = lp;
}

void io_leffreepaths(LEFPATH *firstlp)
{
	LEFPATH *lp, *nextlp;

	for(lp = firstlp; lp != NOLEFPATH; lp = nextlp)
	{
		nextlp = lp->nextlefpath;
		io_lefkillpath(lp);
	}
}
