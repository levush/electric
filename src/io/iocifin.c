/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iocifin.c
 * Input/output tool: CIF input
 * Written by: Robert Winstanley, University of Calgary
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
#include "database.h"
#include "eio.h"
#include "usr.h"
#include "iocifpars.h"
#include "edialogs.h"
#include <math.h>

#define RTOD (45. / atan(1.))

/**************************** CIF CELLS ****************************/

#define NOCIFCELL ((CIFCELL *) -1)

typedef struct Icifcell
{
	INTBIG           cindex;		/* cell index given in the define statement */
	INTBIG           l, r, t, b;	/* bounding box of cell */
	NODEPROTO       *addr;			/* the address of the cif cell */
} CIFCELL;

static CIFCELL **io_cifcells = (CIFCELL **)NOSTRING; /* hash table of 'cifcells' */
static INTBIG    io_cifcells_size = 0; 	/* allocated size of io_cifcells */
static INTBIG    io_cifcells_num = 0;	/* number of used entries in io_cifcells */
static CIFCELL  *io_curcell;			/* the current cell */
static BOOLEAN   io_cifroundwires;		/* nonzero if wires should be rounded */

/**************************** CIF LISTS ****************************/

#define NOCIFLIST ((CIFLIST *) -1)

/* values for CIFLIST->identity */
#define C_START		0
#define C_END		1
#define C_WIRE		2
#define C_FLASH		3
#define C_BOX		4
#define C_POLY		5
#define C_COMMAND	6
#define C_GNAME		7
#define C_LABEL		8
#define C_CALL		9

typedef struct Iciflist
{
	INTBIG           identity;		/* specifies the nature of the entry */
	CHAR            *member;		/* will point to member's structure */
	struct Iciflist *next;			/* next entry in list */
} CIFLIST;

static CIFLIST *io_ciflist = NOCIFLIST;	/* head of the list */
static CIFLIST *io_curlist;				/* current location in list */

/**************************** MEMBER STRUCTURES ****************************/

typedef struct Icfstart
{
	INTBIG cindex;					/* cell index */
	CHAR  *name;					/* cell name */
	INTBIG l, r, t, b;				/* bounding box of cell */
} CFSTART;

typedef struct Icbox
{
	INTBIG lay;						/* the corresponding layer number */
	INTBIG length, width;			/* dimensions of box */
	INTBIG cenx, ceny;				/* center point of box */
	INTBIG xrot, yrot;				/* box direction */
} CBOX;

typedef struct Icpolygon
{
	INTBIG  lay;					/* the corresponding layer number */
	INTBIG *x, *y;					/* list of points */
	INTBIG  lim;					/* number of points in list */
} CPOLY;

typedef struct Icgeoname
{
	INTBIG lay;						/* the corresponding layer number */
	INTBIG x, y;					/* location of name */
	CHAR  *geoname;					/* the geo name */
} CGNAME;

typedef struct Iclabel
{
	INTBIG x, y;					/* location of label */
	CHAR  *label;					/* the label */
} CLABEL;

typedef struct Iccall
{
	INTBIG          cindex;			/* index of cell called */
	CHAR           *name;			/* name of cell called */
	struct Ictrans *list;			/* list of transformations */
} CCALL;

/* values for the transformation type */
#define MIRX	1					/* mirror in x */
#define MIRY	2					/* mirror in y */
#define TRANS	3					/* translation */
#define ROT		4					/* rotation */

#define NOCTRANS ((CTRANS *) -1)

typedef struct Ictrans
{
	INTBIG          type;			/* type of transformation */
	INTBIG          x, y;			/* not required for the mirror types */
	struct Ictrans *next;			/* next element in list */
} CTRANS;

static CTRANS *io_curctrans;				/* current transformation description */

/**************************** MISCELLANEOUS ****************************/

static CHAR       *io_curnodeprotoname;		/* name of the current cell */
static NODEPROTO  *io_incell;				/* address of cell being defined */
static NODEPROTO **io_cifnodes = 0;			/* nodeprotos associated with CIF names */
static INTBIG      io_varlength;			/* the length of all attribute arrays */
static VARIABLE   *io_cifnames;				/* cif names for the current technology */
static INTBIG      io_cifnotfoundcount;		/* number of unknown layer names */
static CHAR      **io_cifnotfoundlist;		/* unknown layer name list */
static INTBIG      io_cifnotfoundlimit = 0;	/* size of unknown layer name list */

/* prototypes for local routines */
static BOOLEAN    io_interpret(FILE*);
static BOOLEAN    io_listtonodes(LIBRARY*);
static NODEPROTO *io_nodes_start(LIBRARY*, void*);
static BOOLEAN    io_nodes_box(void);
static BOOLEAN    io_nodes_poly(void);
static BOOLEAN    io_nodes_call(void);
static void       io_rotatelayer(INTBIG*, INTBIG*, INTBIG);
static BOOLEAN    io_initfind(void);
static NODEPROTO *io_findprotonode(INTBIG);
static CIFCELL   *io_findcifcell(INTBIG);
static void       io_insertcifcell(CIFCELL *cifcell);
static CIFCELL   *io_newcifcell(INTBIG);
static CIFLIST   *io_newciflist(INTBIG);
static void       io_placeciflist(INTBIG);
static CTRANS    *io_newctrans(void);
static void       io_ciffreeciflist(void);
static void       io_ciffreecifcells(void);

/*
 * Routine to free all memory associated with this module.
 */
void io_freecifinmemory(void)
{
	io_ciffreeciflist();
	io_ciffreecifcells();
	if (io_cifnodes != 0) efree((CHAR *)io_cifnodes);
}

BOOLEAN io_readciflibrary(LIBRARY *lib)
{
	REGISTER FILE *f;
	CHAR *filename;
	REGISTER INTBIG i, *curstate;

	curstate = io_getstatebits();
	if ((curstate[0]&CIFINSQUARE) != 0) io_cifroundwires = FALSE;
		else io_cifroundwires = TRUE;

	/* initialize all lists and the searching routines */
	io_ciffreecifcells();
	io_ciffreeciflist();

	if (io_initfind()) return(TRUE);

	/* get the cif file */
	f = xopen(lib->libfile, io_filetypecif, x_(""), &filename);
	if (f == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	if (setjmp(io_filerror))
	{
		ttyputerr(_("Error reading CIF"));
		return(TRUE);
	}

	/* initialize list of not-found layers */
	io_cifnotfoundcount = 0;

	/* parse the cif and create a listing */
	if (io_interpret(f)) return(TRUE);

	/* instantiate the cif as nodes */
	if (io_listtonodes(lib)) return(TRUE);

	/* clean up */
	(void)io_doneinterpreter();
	for(i=0; i<io_cifnotfoundcount; i++) efree((CHAR *)io_cifnotfoundlist[i]);

	return(FALSE);
}

BOOLEAN io_interpret(FILE *file)
{
	INTBIG comcount, left, right, bottom, top;

	if (io_initparser())
	{
		ttyputerr(_("Error initializing parser"));
		return(TRUE);
	}
	if (io_initinterpreter())
	{
		ttyputerr(_("Error initializing interpreter"));
		return(TRUE);
	}
	if (io_infromfile(file)) return(TRUE);
	comcount = io_parsefile();		/* read in the cif */
	(void)io_doneparser();
	if (stopping(STOPREASONCIF)) return(TRUE);
	ttyputverbose(M_("Total CIF commands: %ld"), comcount);

	if (io_fatalerrors() > 0) return(TRUE);

	io_iboundbox(&left, &right, &bottom, &top);

	/* construct a list: first step in the conversion */
	(void)io_createlist();
	return(FALSE);
}

/**************************** NODE CONVERSION ****************************/

BOOLEAN io_listtonodes(LIBRARY *lib)
{
	REGISTER void *dia;

	if (io_verbose < 0)
	{
		dia = DiaInitProgress(0, 0);
		if (dia == 0) return(TRUE);
		DiaSetProgress(dia, 999, 1000);
	}
	io_incell = NONODEPROTO;
	for(io_curlist = io_ciflist; io_curlist != NOCIFLIST; io_curlist = io_curlist->next)
	{
		if (stopping(STOPREASONCIF))
		{
			if (io_verbose < 0) DiaDoneDialog(dia);
			return(TRUE);
		}
		if (io_incell != NONODEPROTO || io_curlist->identity == C_START)
			switch (io_curlist->identity)
		{
			case C_START:
				io_incell = io_nodes_start(lib, dia);
				if (io_incell == NONODEPROTO)
				{
					if (io_verbose < 0) DiaDoneProgress(dia);
					return(TRUE);
				}
				break;
			case C_END:
				/* make cell size right */
				db_boundcell(io_incell, &io_incell->lowx, &io_incell->highx,
					&io_incell->lowy, &io_incell->highy);
				lib->curnodeproto = io_incell;
				io_incell = NONODEPROTO;
				break;
			case C_BOX:
				if (io_nodes_box())
				{
					if (io_verbose < 0) DiaDoneProgress(dia);
					return(TRUE);
				}
				break;
			case C_POLY:
				if (io_nodes_poly())
				{
					if (io_verbose < 0) DiaDoneProgress(dia);
					return(TRUE);
				}
				break;
			case C_CALL:
				if (io_nodes_call())
				{
					if (io_verbose < 0) DiaDoneProgress(dia);
					return(TRUE);
				}
				break;
		}
	}
	if (io_verbose < 0) DiaDoneProgress(dia);
	return(FALSE);
}

NODEPROTO *io_nodes_start(LIBRARY *lib, void *dia)
{
	CIFCELL *cifcell;
	CFSTART *cs;
	CHAR *opt;
	REGISTER void *infstr;

	cs = (CFSTART *)io_curlist->member;
	if ((cifcell = io_newcifcell(cs->cindex)) == NOCIFCELL) return(NONODEPROTO);
	cifcell->l = cs->l;   cifcell->r = cs->r;
	cifcell->b = cs->b;   cifcell->t = cs->t;
	io_curnodeprotoname = cs->name;

	/* remove illegal characters */
	for(opt = io_curnodeprotoname; *opt != 0; opt++)
		if (*opt <= ' ' || *opt == ':' || *opt == ';' || *opt >= 0177)
			*opt = 'X';
	cifcell->addr = us_newnodeproto(io_curnodeprotoname, lib);
	if (cifcell->addr == NONODEPROTO)
	{
		ttyputerr(_("Cannot create the cell %s"), io_curnodeprotoname);
		return(NONODEPROTO);
	}

	if (io_verbose < 0)
	{
		infstr = initinfstr();
		formatinfstr(infstr, _("Reading %s"), io_curnodeprotoname);
		DiaSetTextProgress(dia, returninfstr(infstr));
	} else if (io_verbose > 0) ttyputmsg(_("Reading %s"), io_curnodeprotoname);
	return(cifcell->addr);
}

BOOLEAN io_nodes_box(void)
{
	NODEPROTO *node;
	CBOX *cb;
	CHAR *layname;
	INTBIG l, w, lx, ly, hx, hy, r;

	cb = (CBOX *)io_curlist->member;
	node = io_findprotonode(cb->lay);
	if (node == NONODEPROTO)
	{
		layname = layername(el_curtech, cb->lay);
		ttyputerr(_("Cannot find primitive to use for layer '%s' (number %ld)"),
			layname, cb->lay);
		return(TRUE);
	}
	l = cb->length;        w = cb->width;
	lx = cb->cenx - l/2;   ly = cb->ceny - w/2;
	hx = cb->cenx + l/2;   hy = cb->ceny + w/2;
	r = figureangle(0, 0, (INTBIG)cb->xrot, (INTBIG)cb->yrot);
	if (newnodeinst(node, scalefromdispunit((float)lx, DISPUNITCMIC),
		scalefromdispunit((float)hx, DISPUNITCMIC), scalefromdispunit((float)ly, DISPUNITCMIC),
		scalefromdispunit((float)hy, DISPUNITCMIC), 0, r, io_curcell->addr) == NONODEINST)
	{
		layname = layername(el_curtech, cb->lay);
		ttyputerr(_("Problems creating a box on layer %s in cell %s"), layname,
			describenodeproto(io_curcell->addr));
		return(TRUE);
	}
	return(FALSE);
}

BOOLEAN io_nodes_poly(void)
{
	REGISTER INTBIG lx, ly, hx, hy, i, *trace, cx, cy, *pt;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *newni;
	REGISTER CPOLY *cp;

	cp = (CPOLY *)io_curlist->member;
	if (cp->lim == 0) return(FALSE);
	np = io_findprotonode(cp->lay);
	lx = hx = cp->x[0];   ly = hy = cp->y[0];
	for(i=1; i<cp->lim; i++)
	{
		if (cp->x[i] < lx) lx = cp->x[i];
		if (cp->x[i] > hx) hx = cp->x[i];
		if (cp->y[i] < ly) ly = cp->y[i];
		if (cp->y[i] > hy) hy = cp->y[i];
	}
	newni = newnodeinst(np, scalefromdispunit((float)lx, DISPUNITCMIC),
		scalefromdispunit((float)hx, DISPUNITCMIC), scalefromdispunit((float)ly, DISPUNITCMIC),
			scalefromdispunit((float)hy, DISPUNITCMIC), 0, 0, io_curcell->addr);
	if (newni == NONODEINST)
	{
		ttyputerr(_("Problems creating a polygon on layer %ld in cell %s"), cp->lay,
			describenodeproto(io_curcell->addr));
		return(TRUE);
	}

	/* store the trace information */
	pt = trace = emalloc((cp->lim*2*SIZEOFINTBIG), io_tool->cluster);
	if (trace == 0) return(TRUE);
	cx = (hx + lx) / 2;   cy = (hy + ly) / 2;
	for(i=0; i<cp->lim; i++)
	{
		*pt++ = scalefromdispunit((float)(cp->x[i] - cx), DISPUNITCMIC);
		*pt++ = scalefromdispunit((float)(cp->y[i] - cy), DISPUNITCMIC);
	}

	/* store the trace information */
	(void)setvalkey((INTBIG)newni, VNODEINST, el_trace_key, (INTBIG)trace,
		VINTEGER|VISARRAY|((cp->lim*2)<<VLENGTHSH));

	/* free the polygon memory */
	efree((CHAR *)trace);
	efree((CHAR *)cp->x);   efree((CHAR *)cp->y);
	cp->lim = 0;
	return(FALSE);
}

BOOLEAN io_nodes_call(void)
{
	CIFCELL *cell;
	CCALL *cc;
	CTRANS *ctrans;
	INTBIG l, r, t, b, rot, trans, hlen, hwid, cenx, ceny, temp;
	INTBIG deg;

	cc = (CCALL *)io_curlist->member;
	if ((cell = io_findcifcell(cc->cindex)) == NOCIFCELL)
	{
		ttyputerr(_("Referencing an undefined cell"));
		return(TRUE);
	}
	rot = trans = 0;
	l = cell->l;    r = cell->r;    b = cell->b;    t = cell->t;
	for(ctrans = cc->list; ctrans != NOCTRANS; ctrans = ctrans->next)
		switch (ctrans->type)
	{
		case MIRX:
			temp = l;   l = -r;   r = -temp;
			rot = (trans) ? ((rot+2700) % 3600) : ((rot+900) % 3600);
			trans = 1 - trans;
			break;
		case MIRY:
			temp = t;   t = -b;   b = -temp;
			rot = (trans) ? ((rot+900) % 3600) : ((rot+2700) % 3600);
			trans = 1 - trans;
			break;
		case TRANS:
			l += ctrans->x;   r += ctrans->x;
			b += ctrans->y;   t += ctrans->y;
			break;
		case ROT:
			deg = figureangle(0L, 0L, ctrans->x, ctrans->y);
			if (deg != 0)
			{
				hlen = abs(((l-r)/2));   hwid = abs(((b-t)/2));
				cenx = (l+r)/2;   ceny = (b+t)/2;
				io_rotatelayer(&cenx, &ceny, deg);
				l = cenx - hlen;   r = cenx + hlen;
				b = ceny - hwid;   t = ceny + hwid;
				rot += ((trans) ? -deg : deg);
			}
	}
	while(rot >= 3600) rot -= 3600;
	while(rot < 0) rot += 3600;
	if (newnodeinst((NODEPROTO *)(cell->addr), scalefromdispunit((float)l, DISPUNITCMIC),
		scalefromdispunit((float)r, DISPUNITCMIC), scalefromdispunit((float)b, DISPUNITCMIC),
			scalefromdispunit((float)t, DISPUNITCMIC), trans, rot, io_curcell->addr) ==
				NONODEINST)
	{
		ttyputerr(_("Problems creating an instance of cell %s in cell %s"),
			describenodeproto(cell->addr), describenodeproto(io_curcell->addr));
		return(TRUE);
	}
	return(FALSE);
}

void io_rotatelayer(INTBIG *cenx, INTBIG *ceny, INTBIG deg)
{
	double vlen, vang, fx, fy, factx, facty, fact;
	INTBIG temp;

	/* trivial test to prevent atan2 domain errors */
	if (*cenx == 0 && *ceny == 0) return;
	switch (deg)	/* do the manhattan cases directly (SRP)*/
	{
		case 0:
		case 3600:	/* just in case */
			break;
		case 900:
			temp = *cenx;   *cenx = -*ceny;   *ceny = temp;
			break;
		case 1800:
			*cenx = -*cenx;   *ceny = -*ceny;
			break;
		case 2700:
			temp = *cenx;   *cenx = *ceny;   *ceny = -temp;
			break;
		default: /* this old code only permits rotation by integer angles (SRP)*/
			for(factx=1.; fabs((*cenx/factx)) > 1000; factx *= 10.) ;
			for(facty=1.; fabs((*ceny/facty)) > 1000; facty *= 10.) ;
			fact = (factx > facty) ? facty : factx;
			fx = *cenx / fact;		  fy = *ceny / fact;
			vlen = fact * sqrt((double)(fx*fx + fy*fy));
			vang = (deg + figureangle(0L,0L,*cenx, *ceny)) / 10.0 / RTOD;
			*cenx = (INTBIG)(vlen * cos(vang));
			*ceny = (INTBIG)(vlen * sin(vang));
			break;
	}
}

/**************************** SEARCHING ROUTINES ****************************/

BOOLEAN io_initfind(void)
{
	REGISTER CHAR *ch, *ch2;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	NODEINST nodeinst;
	REGISTER INTBIG total, i, j, k;
	static POLYGON *poly = NOPOLYGON;

	/* get the array of CIF names */
	io_cifnames = getval((INTBIG)el_curtech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_cif_layer_names"));
	if (io_cifnames == NOVARIABLE)
	{
		ttyputerr(_("There are no CIF layer names assigned in the %s technology"),
			el_curtech->techname);
		return(TRUE);
	}
	io_varlength = getlength(io_cifnames);
	if (io_varlength < el_curtech->layercount)
	{
		ttyputerr(_("Warning: CIF layer information is bad for technology %s.  Use 'CIF Options' to fix it"),
			el_curtech->techname);
		return(TRUE);
	}

	/* create the array of nodes associated with the array of CIF names */
	total = el_curtech->layercount;
	if (io_cifnodes != 0) efree((CHAR *)io_cifnodes);
	io_cifnodes = (NODEPROTO **)emalloc((total * (sizeof (NODEPROTO *))), io_tool->cluster);
	if (io_cifnodes == 0) return(TRUE);
	for(i=0; i<total; i++) io_cifnodes[i] = NONODEPROTO;

	/* create the polygon */
	(void)needstaticpolygon(&poly, 4, io_tool->cluster);

	/* run through the node prototypes in this technology */
	for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		ni = &nodeinst;   initdummynode(ni);
		ni->proto = np;
		ni->lowx  = np->lowx;
		ni->highx = np->highx;
		ni->lowy  = np->lowy;
		ni->highy = np->highy;
		i = nodepolys(ni, 0, NOWINDOWPART);
		if (i != 1) continue;
		shapenodepoly(ni, 0, poly);
		if (poly->layer < 0) continue;
		ch = ((CHAR **)io_cifnames->addr)[poly->layer];
		if (*ch != 0) io_cifnodes[poly->layer] = np;
	}

	/* make sure every CIF string has an equivalent node */
	for(i=0; i<total; i++)
	{
		ch = ((CHAR **)io_cifnames->addr)[i];
		if (*ch == 0) continue;
		if (io_cifnodes[i] != NONODEPROTO) continue;

		/* first look for a layer with the same CIF name */
		for (k=0; k<total; k++)
		{
			np = io_cifnodes[k];
			if (np == NONODEPROTO) continue;
			ni = &nodeinst;   initdummynode(ni);
			ni->proto = np;
			ni->lowx  = np->lowx;
			ni->highx = np->highx;
			ni->lowy  = np->lowy;
			ni->highy = np->highy;
			(void)nodepolys(ni, 0, NOWINDOWPART);
			shapenodepoly(ni, 0, poly);
			ch2 = ((CHAR **)io_cifnames->addr)[poly->layer];
			if (estrcmp(ch, ch2) != 0) continue;
			io_cifnodes[i] = np;
			break;
		}
		if (k < total) continue;

		/* search for ANY node with this layer */
		for(np = el_curtech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			ni = &nodeinst;   initdummynode(ni);
			ni->proto = np;
			ni->lowx  = np->lowx;
			ni->highx = np->highx;
			ni->lowy  = np->lowy;
			ni->highy = np->highy;
			j = nodepolys(ni, 0, NOWINDOWPART);
			for(k=0; k<j; k++)
			{
				shapenodepoly(ni, 0, poly);
				if (poly->layer == i) break;
			}
			if (k >= j) continue;
			io_cifnodes[i] = np;
			break;
		}
	}

	return(FALSE);
}

INTBIG io_findlayernum(CHAR *name)
{
	REGISTER INTBIG i, newlimit;
	REGISTER CHAR **newlist;

	for(i=0; i<io_varlength; i++)
		if (estrcmp(name, ((CHAR **)io_cifnames->addr)[i]) == 0) return(i);

	/* CIF name not found: see if it is in the list of unfound layers */
	for(i=0; i<io_cifnotfoundcount; i++)
		if (estrcmp(name, io_cifnotfoundlist[i]) == 0) return(-1);

	/* add to the list */
	if (io_cifnotfoundcount >= io_cifnotfoundlimit)
	{
		newlimit = io_cifnotfoundlimit * 2;
		if (newlimit <= 0) newlimit = 10;
		if (newlimit < io_cifnotfoundcount) newlimit = io_cifnotfoundcount;
		newlist = (CHAR **)emalloc(newlimit * (sizeof (CHAR *)), io_tool->cluster);
		if (newlist == 0) return(-1);
		for(i=0; i<io_cifnotfoundcount; i++)
			newlist[i] = io_cifnotfoundlist[i];
		if (io_cifnotfoundlimit > 0) efree((CHAR *)io_cifnotfoundlist);
		io_cifnotfoundlist = newlist;
		io_cifnotfoundlimit = newlimit;
	}
	(void)allocstring(&io_cifnotfoundlist[io_cifnotfoundcount], name, io_tool->cluster);
	io_cifnotfoundcount++;
	ttyputmsg(_("Layer %s not found"), name);
	return(-1);
}

NODEPROTO *io_findprotonode(INTBIG num)
{
	return(io_cifnodes[num]);
}

CIFCELL *io_findcifcell(INTBIG cindex)
{
	CIFCELL *cifcell;
	REGISTER INTBIG i, j;

	i = cindex % io_cifcells_size;
	for(j=1; j<=io_cifcells_size; j += 2)
	{
		cifcell = io_cifcells[i];
		if (cifcell == NOCIFCELL) break;
		if (cindex == cifcell->cindex) return(cifcell);
		i += j;
		if (i >= io_cifcells_size) i -= io_cifcells_size;
	}
	return(NOCIFCELL);
}

void io_insertcifcell(CIFCELL *cifcell)
{
	REGISTER INTBIG i, j;

	i = cifcell->cindex % io_cifcells_size;
	for(j=1; j<=io_cifcells_size; j += 2)
	{
		if (io_cifcells[i] == NOCIFCELL)
		{
			io_cifcells[i] = cifcell;
			break;
		}
		i += j;
		if (i >= io_cifcells_size) i -= io_cifcells_size;
	}
}

CIFCELL *io_newcifcell(INTBIG cindex)
{
	CIFCELL *newcc, *cc;
	CIFCELL **old_cifcells;
	INTBIG old_cifcells_size;
	INTBIG new_cifcells_size;
	INTBIG i;

	newcc = (CIFCELL *)emalloc((sizeof (CIFCELL)), io_tool->cluster);
	if (newcc == (CIFCELL *) 0)
	{
		ttyputerr(_("Not enough memory allocated for CIFCELL"));
		return(NOCIFCELL);
	}
	io_cifcells_num++;
	if (io_cifcells_num >= io_cifcells_size/2)
	{
		old_cifcells = io_cifcells;
		old_cifcells_size = io_cifcells_size;
		new_cifcells_size = pickprime(io_cifcells_num * 4);
		io_cifcells = (CIFCELL **)emalloc(new_cifcells_size * (sizeof (CIFCELL *)), io_tool->cluster);
		if (io_cifcells == (CIFCELL **) 0)
		{
			ttyputerr(_("Not enough memory allocated for CIFCELL"));
			return(NOCIFCELL);
		}
		for (i = 0; i < new_cifcells_size; i++) io_cifcells[i] = NOCIFCELL;
		io_cifcells_size = new_cifcells_size;
		for (i = 0; i < old_cifcells_size; i++)
		{
			cc = old_cifcells[i];
			if (cc != NOCIFCELL) io_insertcifcell(cc);	
		}
		if (old_cifcells_size > 0) efree((CHAR *) old_cifcells);
	}
	newcc->addr = NONODEPROTO;
	newcc->cindex = cindex;
	io_insertcifcell(newcc);
	
	io_curcell = newcc;
	return(newcc);
}

void io_ciffreecifcells(void)
{
	CIFCELL *cc;
	INTBIG i;

	for (i = 0; i < io_cifcells_size; i++)
	{
		cc = io_cifcells[i];
		if (cc != NOCIFCELL)
			efree((CHAR *)cc);
	}
	if (io_cifcells_size > 0) efree((CHAR *)io_cifcells);
	io_cifcells_size = 0;
	io_cifcells_num = 0;
	io_cifcells = (CIFCELL **)NOSTRING;
}

CIFLIST *io_newciflist(INTBIG id)
{
	CIFLIST *newcl;
	CCALL *cc;
	CFSTART *cs;

	newcl = (CIFLIST *)emalloc((sizeof (CIFLIST)), io_tool->cluster);
	if (newcl == (CIFLIST *) 0)
	{
		ttyputerr(_("Not enough memory allocated for CIFLIST"));
		return(NOCIFLIST);
	}
	newcl->next = NOCIFLIST;
	newcl->identity = id;
	switch (id)
	{
		case C_START:
			cs = (CFSTART *)emalloc((sizeof (CFSTART)), io_tool->cluster);
			if (cs == 0) return(NOCIFLIST);
			newcl->member = (CHAR *)cs;
			cs->name = 0;
			break;
		case C_BOX:
			newcl->member = (CHAR *)emalloc((sizeof (CBOX)), io_tool->cluster);
			if (newcl->member == (CHAR *) 0) return(NOCIFLIST);
			break;
		case C_POLY:
			newcl->member = (CHAR *)emalloc((sizeof (CPOLY)), io_tool->cluster);
			if (newcl->member == (CHAR *) 0) return(NOCIFLIST);
			break;
		case C_GNAME:
			newcl->member = (CHAR *)emalloc((sizeof (CGNAME)), io_tool->cluster);
			if (newcl->member == (CHAR *) 0) return(NOCIFLIST);
			break;
		case C_LABEL:
			newcl->member = (CHAR *)emalloc((sizeof (CLABEL)), io_tool->cluster);
			if (newcl->member == (CHAR *) 0) return(NOCIFLIST);
			break;
		case C_CALL:
			cc = (CCALL *)emalloc((sizeof (CCALL)), io_tool->cluster);
			if (cc == 0) return(NOCIFLIST);
			newcl->member = (CHAR *)cc;
			cc->name = 0;
			break;
	}
	return(newcl);
}

void io_ciffreeciflist(void)
{
	REGISTER CIFLIST *cl;
	REGISTER CCALL *cc;
	REGISTER CTRANS *cct;
	REGISTER CFSTART *cs;

	while (io_ciflist != NOCIFLIST)
	{
		cl = io_ciflist;
		io_ciflist = io_ciflist->next;
		switch (cl->identity)
		{
			case C_CALL:
				cc = (CCALL *)cl->member;
				while (cc->list != NOCTRANS)
				{
					cct = cc->list;
					cc->list = cc->list->next;
					efree((CHAR *)cct);
				}
				if (cc->name != 0) efree((CHAR *)cc->name);
				efree((CHAR *)cc);
				break;
			case C_START:
				cs = (CFSTART *)cl->member;
				if (cs->name != 0) efree((CHAR *)cs->name);
				efree((CHAR *)cs);
				break;
			case C_BOX:
			case C_POLY:
			case C_GNAME:
			case C_LABEL:
				efree((CHAR *)cl->member);
				break;
		}
		efree((CHAR *)cl);
	}
}

void io_placeciflist(INTBIG id)
{
	CIFLIST *cl;

	if ((cl = io_newciflist(id)) == NOCIFLIST) return;
	if (io_ciflist == NOCIFLIST) io_ciflist = io_curlist = cl; else
	{
		while(io_curlist->next != NOCIFLIST)
			io_curlist = io_curlist->next;
		io_curlist->next = cl;
		io_curlist = io_curlist->next;
	}
}

CTRANS *io_newctrans(void)
{
	CTRANS *newct;
	CCALL *cc;

	newct = (CTRANS *)emalloc((sizeof (CTRANS)), io_tool->cluster);
	if (newct == (CTRANS *) 0)
	{
		ttyputerr(_("Not enough memory allocated for CTRANS"));
		return(NOCTRANS);
	}
	newct->next = NOCTRANS;
	cc = (CCALL *)io_curlist->member;
	if (cc->list == NOCTRANS) cc->list = newct; else
		io_curctrans->next = newct;
	io_curctrans = newct;
	return(newct);
}

void io_outputwire(INTBIG lay, INTBIG width, CHAR *wpath)
{
	/* convert wires to boxes and flashes  */
	INTBIG i;
	INTBIG llt, lrt, lbm, ltp, lim;
	point prev, curr;

	lim = io_pathlength((path)wpath);
	prev = io_removepoint((path)wpath);

	/* do not use roundflashes with zero-width wires */
	if (width != 0 && io_cifroundwires)
	{
		io_bbflash(lay, width, prev, &llt, &lrt, &lbm, &ltp);
		io_outputflash(lay, width, prev);
	}
	for (i = 1; i < lim; i++)
	{
		INTBIG len;
		INTBIG xr, yr;
		point center;

		curr = io_removepoint((path)wpath);

		/* do not use roundflashes with zero-width wires */
		if (width != 0 && io_cifroundwires)
		{
			io_bbflash(lay, width, curr, &llt, &lrt, &lbm, &ltp);
			io_outputflash(lay, width, curr);
		}
		xr = curr.x-prev.x;   yr = curr.y-prev.y;
		len = computedistance(0, 0, xr, yr);
		if (!io_cifroundwires) len += width;
		center.x = (curr.x+prev.x)/2;   center.y = (curr.y+prev.y)/2;
		io_bbbox(lay, len, width, center, xr, yr, &llt, &lrt, &lbm, &ltp);
		io_outputbox(lay, len, width, center, xr, yr);
		prev = curr;
	}
}

void io_outputflash(INTBIG lay, INTBIG diameter, point center)
{
	/* flash approximated by an octagon */
	INTBIG radius = diameter/2;
	float fcx = (float)center.x;
	float fcy = (float)center.y;
	float offset = (((float)diameter)/2.0f)*0.414213f;
	CHAR *fpath = io_makepath();
	point temp;

	temp.x = center.x-radius;
	temp.y = roundfloat(fcy+offset);
	if (io_appendpoint((path)fpath, temp)) return;
	temp.y = roundfloat(fcy-offset);
	if (io_appendpoint((path)fpath, temp)) return;
	temp.x = roundfloat(fcx-offset);
	temp.y = center.y-radius;
	if (io_appendpoint((path)fpath, temp)) return;
	temp.x = roundfloat(fcx+offset);
	if (io_appendpoint((path)fpath, temp)) return;
	temp.x = center.x+radius;
	temp.y = roundfloat(fcy-offset);
	if (io_appendpoint((path)fpath, temp)) return;
	temp.y = roundfloat(fcy+offset);
	if (io_appendpoint((path)fpath, temp)) return;
	temp.x = roundfloat(fcx+offset);
	temp.y = center.y+radius;
	if (io_appendpoint((path)fpath, temp)) return;
	temp.x = roundfloat(fcx-offset);
	if (io_appendpoint((path)fpath, temp)) return;

	io_outputpolygon(lay, fpath);
	io_freepath((path)fpath);
}

void io_outputbox(INTBIG lay, INTBIG length, INTBIG width, point center,
	INTBIG xrotation, INTBIG yrotation)
{
	CBOX *cb;

	if (length == 0 && width == 0) return;	/* ignore null boxes */
	io_placeciflist(C_BOX);
	cb = (CBOX *)io_curlist->member;
	cb->lay = lay;
	cb->length = length;	cb->width = width;
	cb->cenx = center.x;	cb->ceny = center.y;
	cb->xrot = xrotation;	cb->yrot = yrotation;
}

void io_outputpolygon(INTBIG lay, CHAR *ppath)
{
	INTBIG i;
	INTBIG lim;
	CPOLY *cp;
	point temp;

	lim = io_pathlength((path)ppath);
	if (lim < 3) return;

	io_placeciflist(C_POLY);
	cp = (CPOLY *)io_curlist->member;
	cp->lay = lay;
	cp->x = emalloc((lim * SIZEOFINTBIG), io_tool->cluster);
	if (cp->x == 0)
	{
		ttyputnomemory();
		cp->lim = 0;
		return;
	}
	cp->y = emalloc((lim * SIZEOFINTBIG), io_tool->cluster);
	if (cp->y == 0)
	{
		ttyputnomemory();
		cp->lim = 0;
		return;
	}

	cp->lim = lim;
	for (i = 0; i < lim; i++)
	{
		temp = io_removepoint((path)ppath);
		cp->x[i] = temp.x;
		cp->y[i] = temp.y;
	}
}

void io_outputusercommand(INTBIG command, CHAR *text)
{
}

void io_outputgeoname(CHAR *name, point pt, INTBIG lay)
{
	CGNAME *cg;

	io_placeciflist(C_GNAME);
	cg = (CGNAME *)io_curlist->member;
	cg->lay = lay;
	(void)allocstring(&cg->geoname, name, io_tool->cluster);
	cg->x = pt.x;   cg->y = pt.y;
}

void io_outputlabel(CHAR *name, point pt)
{
	CLABEL *cl;

	io_placeciflist(C_LABEL);
	cl = (CLABEL *)io_curlist->member;
	(void)allocstring(&cl->label, name, io_tool->cluster);
	cl->x = pt.x;   cl->y = pt.y;
}

void io_outputcall(INTBIG number, CHAR *name, CHAR *list)
{
	CCALL *cc;
	tentry temp;
	INTBIG i;

	io_placeciflist(C_CALL);
	cc = (CCALL *)io_curlist->member;
	cc->cindex = number;
	(void)allocstring(&cc->name, name, io_tool->cluster);
	cc->list = io_curctrans = NOCTRANS;
	for(i = io_tlistlength((tlist)list); i>0; i--)
	{
		if (io_newctrans() == NOCTRANS) return;
		temp = io_removetentry((tlist)list);
		switch (temp.kind)
		{
			case MIRROR:
				if (temp.guts.mi.xcoord) io_curctrans->type = MIRX; else
					io_curctrans->type = MIRY;
				break;
			case TRANSLATE:
				io_curctrans->type = TRANS;
				io_curctrans->x = temp.guts.tr.xt;
				io_curctrans->y = temp.guts.tr.yt;
				break;
			case ROTATE:
				io_curctrans->type = ROT;
				io_curctrans->x = temp.guts.ro.xrot;
				io_curctrans->y = temp.guts.ro.yrot;
				break;
		}
	}
}

void io_outputds(INTBIG number, CHAR *name, INTBIG l, INTBIG r, INTBIG b, INTBIG t)
{
	CFSTART *cs;

	io_placeciflist(C_START);
	cs = (CFSTART *)io_curlist->member;
	cs->cindex = number;
	(void)allocstring(&cs->name, name, io_tool->cluster);
	cs->l = l;   cs->r = r;
	cs->b = b;   cs->t = t;
}

void io_outputdf(void)
{
	io_placeciflist(C_END);
}
