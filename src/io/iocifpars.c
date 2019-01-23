/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iocifpars.c
 * CIF parsing
 * Written by: Robert W. Hon, Schlumberger Palo Alto Research
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
#include "iocifpars.h"
#include "edialogs.h"
#include <math.h>

/********************* interp.c: interpreter *********************/

#define CONVERT 0					/* 1=> convert wires to boxes */

/* global state for the interpreter */
static float io_cifscalefactor;		/* A/B from DS */
static stentry *io_cifcurrent;		/* current symbol being defined */

static INTBIG io_cifinstance;		/* inst count for default names */
static INTBIG io_cifbinst;			/* inst count for default names */
static INTBIG io_cifstatesince;		/* # statements since 91 com */
static CHAR   io_cifnamepending;	/* 91 pending */
static CHAR   io_cifnamed;			/* symbol has been named */
static CHAR  *io_cifnstored;		/* name saved from 91 */

static CHAR   io_cifdefinprog;		/* definition in progress flag */
static INTBIG io_cifignore;			/* ignore statements until DF */
static CHAR   io_cifendcom;			/* end command flag */

static INTBIG io_cifcurlayer;		/* current layer */
static INTBIG io_cifbackuplayer;	/* place to save layer during def */

static INTBIG io_nulllayererrors;	/* null layer errors encountered */

typedef struct ite					/* items in item tree */
{
	INTBIG      it_type;			/* redundant indicator of obj type */
	struct ite *it_rel;				/* links for tree */
	struct ite *it_norel;			/* links for tree */
	struct ite *it_same;			/* links for tree */
	CHAR       *it_what;			/* pointer into symbol structure */
	INTBIG      it_level;			/* level of nest, from root (= 0) */
	INTBIG      it_context;			/* what trans context to use */
	INTBIG      it_left;			/* bb on chip */
	INTBIG      it_right;			/* bb on chip */
	INTBIG      it_bottom;			/* bb on chip */
	INTBIG      it_top;				/* bb on chip */
} itementry;

static itementry *io_cifittop;		/* top of free stack */
static itementry *io_cifilist;		/* head of item list */

static linkedpoint  *io_cifpfree;
static linkedtentry *io_ciftfree = NIL;

/* prototypes for local routines */
static CHAR io_endseen(void);
static void io_idefstart(INTBIG, INTBIG, INTBIG);
static void io_idefend(void);
static void io_ideletedef(INTBIG);
static void io_ilayer(CHAR*);
static void io_icomment(CHAR*);
static void io_iusercommand(INTBIG, CHAR*);
static void io_iend(void);
static void io_iwire(INTBIG, CHAR*);
static void io_iwire(INTBIG, CHAR*);
static void io_iflash(INTBIG, point);
static void io_ipolygon(CHAR*);
static void io_scalepath(CHAR*, CHAR*);
static void io_ibox(INTBIG, INTBIG, point, INTBIG, INTBIG);
static void io_icall(INTBIG, CHAR*);
static void io_isymname(CHAR*);
static void io_iinstname(CHAR*);
static void io_igeoname(CHAR*, point, INTBIG);
static void io_ilabel(CHAR*, point);
static CHAR *io_makesymname(INTBIG);
static CHAR *io_makeinstname(INTBIG);
static void io_newstentry(stentry*, INTBIG);
static stentry *io_lookupsym(INTBIG);
static void io_freesymbols(void);
static void io_initmm(point);
static void io_donemm(void);
static void io_minmax(point);
static INTBIG io_minx(void);
static INTBIG io_miny(void);
static INTBIG io_maxx(void);
static INTBIG io_maxy(void);
static void io_findcallbb(symcall*);
static void io_findbb(stentry*);
static CHAR io_initutil(void);
static void io_freestorage(void);
static void io_freedef(objectptr);
static void io_clear(transform);
static BOOLEAN io_swapcontext(INTBIG);
static BOOLEAN io_increfcount(INTBIG);
static BOOLEAN io_decrefcount(INTBIG);
static void io_pushtrans(void);
static void io_poptrans(void);
static void io_matmult(transform, transform, transform);
static void io_mmult(transform, transform, transform);
static void io_applylocal(transform);
static void io_assign(transform, transform);
static void io_rotate(INTBIG, INTBIG);
static void io_translate(INTBIG, INTBIG);
static void io_mirror(CHAR);
static void io_dumpstack(void);
static void io_printmat(transform);
static void io_bbpolygon(INTBIG, path, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static void io_bbwire(INTBIG, INTBIG, path, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
static CHAR io_inititems(void);
static BOOLEAN io_doneitems(void);
static itementry *io_newnode(void);
static void io_freenode(itementry*);
static void io_sendlist(itementry*);
static void io_dumpdef(stentry*);
static void io_shipcontents(stentry*);
static void io_outitem(itementry*);
static void io_item(objectptr);
static CHAR io_initinput(void);
static CHAR io_doneinput(void);
static CHAR io_getch(void);
static CHAR io_peek(void);
static CHAR io_endoffile(void);
static INTBIG io_flush(CHAR);
static void io_identify(void);
static CHAR io_inittypes(void);
static void io_freeciftfree(void);
static CHAR io_donetypes(void);
static linkedpoint *io_getpnode(void);
static void io_freepnode(linkedpoint*);
static void io_copypath(path, path);
static void io_freetlist(tlist);
static void io_dupetlist(tlist, tlist);
static linkedtentry *io_gettnode(void);
static void io_freetnode(linkedtentry*);
static void io_appendtentry(tlist, tentry);
static CHAR io_initerror(void);
static CHAR io_doneerror(void);
static void io_report(CHAR*, INTBIG);
static void io_blank(void);
static void io_sep(void);
static CHAR io_semi(void);
static CHAR io_sp(void);
static CHAR *io_utext(void);
static CHAR *io_name(void);
static INTBIG io_cardinal(void);
static INTBIG io_signed(void);
static void io_logit(INTBIG);
static point io_getpoint(void);
static void io_getpath(path);
static CHAR io_inittrans(void);
static CHAR io_donetrans(void);
static tmatrix io_getlocal(void);
static point io_transpoint(point);
static void io_freeonedef(struct cifhack *cur);
static CHAR *io_maketlist(void);
static INTBIG io_parsestatement(void);

/*
 * Routine to free all memory associated with this module.
 */
void io_freecifparsmemory(void)
{
	io_freeciftfree();
}

INTBIG io_initinterpreter(void)
{
	io_nulllayererrors = 0;
	io_cifdefinprog = 0;
	io_cifignore = 0;
	io_cifnamepending = 0;
	io_cifendcom = 0;
	io_cifcurlayer = -1;
	io_cifinstance = 1;
	return(io_initutil() || io_inititems() || io_inittrans());
}

INTBIG io_doneinterpreter(void)
{
	if (io_nulllayererrors != 0)
	{
		io_report(_("output on null layer"), FATALSEMANTIC);
	}

	io_freestorage();
	return(io_donetrans() || io_doneitems());
}

CHAR io_endseen(void)
{
	return(io_cifendcom);
}

void io_idefstart(INTBIG symbol,INTBIG mtl,INTBIG div)
{
	io_cifstatesince++;
	io_cifcurrent = io_lookupsym(symbol);
	if (io_cifcurrent->st_defined)
	{
		/* redefining this symbol */
		CHAR mess[300];
		(void)esnprintf(mess, 300, _("attempt to redefine symbol %lu (ignored)"), symbol);
		io_report(mess, ADVISORY);
		io_cifignore = 1;
		return;
	}

	io_cifdefinprog = 1;
	if (mtl != 0 && div != 0) io_cifscalefactor = ((float) mtl)/((float) div); else
	{
		io_report(_("illegal scale factor, ignored"), ADVISORY);
		io_cifscalefactor = 1.0;
	}
	io_cifbackuplayer = io_cifcurlayer;	/* save current layer */
	io_cifbinst = io_cifinstance;		/* save instance count */
	io_cifcurlayer = -1;
	io_cifnamed = 0;					/* symbol not named */
	io_cifinstance = 0;					/* no calls in symbol yet */
}

void io_idefend(void)
{
	CHAR *s;

	io_cifstatesince++;
	if (io_cifignore)
	{
		io_cifignore = 0;
		return;
	}
	io_cifdefinprog = 0;
	io_cifcurlayer = io_cifbackuplayer;		/* restore old layer */
	io_cifinstance = io_cifbinst;
	if (!io_cifnamed)
	{
		s = io_makesymname(io_cifcurrent->st_symnumber);
		(void)allocstring(&io_cifcurrent->st_name, s, io_tool->cluster);
	}
	io_cifcurrent->st_defined = 1;
}


void io_ideletedef(INTBIG n)
{
	io_cifstatesince++;
	io_report(_("DD not supported (ignored)"), ADVISORY);
}

void io_ilayer(CHAR *lname)
{

	io_cifstatesince++;
	if (io_cifignore) return;
	io_cifcurlayer = io_findlayernum(lname);
}

void io_icomment(CHAR *contents)
{
}

void io_iusercommand(INTBIG command, CHAR *text)
{
	io_cifstatesince++;
	if (io_cifignore) return;
	io_outputusercommand(command, text);
}

void io_iend(void)
{
	io_cifstatesince++;
	io_cifendcom = 1;
	if (io_cifnamepending)
	{
		efree((CHAR *)io_cifnstored);
		io_report(_("no instance to match name command"), ADVISORY);
		io_cifnamepending = 0;
	}
}


#if CONVERT
/* convert wires to boxes and flashes */

void io_iwire(INTBIG width, CHAR *wpath)
{
	INTBIG i, lim;
	INTBIG lt, rt, bm, tp;
	point prev, curr;

	lim = io_pathlength((path)wpath);
	io_cifstatesince++;
	if (io_cifignore) return;
	if (io_cifcurlayer == -1)
	{
		io_nulllayererrors++;
		return;
	}

	prev = io_removepoint((path)wpath);
	io_iflash(width, prev); io_cifstatesince--;
	for (i = 1; i < lim; i++)
	{
		INTBIG len, xr, yr;
		point center;

		curr = io_removepoint((path)wpath);
		io_iflash(width, curr); io_cifstatesince--;
		xr = curr.x-prev.x;   yr = curr.y-prev.y;
		len = computedistance(0, 0, xr, yr);
		center.x = (curr.x+prev.x)/2;   center.y = (curr.y+prev.y)/2;
		io_ibox(len, width, center, xr, yr); io_cifstatesince--;
		prev = curr;
	}
}

#else
/* regular wires */

void io_iwire(INTBIG width, CHAR *a)
{
	wire *obj;
	INTBIG length = io_pathlength((path)a);
	CHAR *bbpath, *spath, *tpath;
	INTBIG i;

	io_cifstatesince++;
	if (io_cifignore) return;
	if (io_cifcurlayer == -1)
	{
		io_nulllayererrors++;
		return;
	}
	obj = (wire *)emalloc((length-1)*sizeof(point)+sizeof(wire), io_tool->cluster);
	if (obj == NIL)
	{
		io_report(_("no space: io_iwire"), FATALINTERNAL);
		return;
	}

	tpath = a;
	spath = NIL;			/* path in case of scaling */
	obj->wi_type = WIRE;
	obj->wi_layer = io_cifcurlayer;
	obj->wi_numpts = length;
	if (io_cifdefinprog && io_cifscalefactor != 1.0)
	{
		spath = io_makepath();		/* create a new path */
		io_scalepath(a, spath);		/* scale all points */
		width = (INTBIG)(io_cifscalefactor * (float)width);
		tpath = spath;
	}
	obj->wi_width = width;

	bbpath = io_makepath();		/* get a new path for bb use */
	io_copypath((path)tpath, (path)bbpath);
	io_bbwire(io_cifcurlayer, width, (path)bbpath, &(obj->wi_bb.l), &(obj->wi_bb.r),
		&(obj->wi_bb.b), &(obj->wi_bb.t));
	io_freepath((path)bbpath);
	for (i = 0; i < length; i++) obj->wi_p[i] = io_removepoint((path)tpath);

	if (io_cifdefinprog)
	{
		/* insert into symbol's guts */
		obj->wi_next = io_cifcurrent->st_guts;
		io_cifcurrent->st_guts = (objectptr) obj;
	} else io_item((objectptr)obj);		/* stick into item list */

	if (spath != NIL) io_freepath((path)spath);
}

#endif


void io_iflash(INTBIG diameter,point center)
{
	flash *obj;

	io_cifstatesince++;
	if (io_cifignore) return;
	if (io_cifcurlayer == -1)
	{
		io_nulllayererrors++;
		return;
	}
	if (diameter == 0)
	{
		io_report(_("flash with null diamter, ignored"), ADVISORY);
		return;
	}

	obj = (flash *)emalloc(sizeof(flash), io_tool->cluster);
	if (obj == NIL)
	{
		io_report(_("no space: io_iflash"), FATALINTERNAL);
		return;
	}
	obj->fl_type = FLASH;
	obj->fl_layer = io_cifcurlayer;
	if (io_cifdefinprog && io_cifscalefactor != 1.0)
	{
		diameter = (UINTBIG) (io_cifscalefactor * (float)diameter);
		center.x = (INTBIG) (io_cifscalefactor * (float)center.x);
		center.y = (INTBIG) (io_cifscalefactor * (float)center.y);
	}
	obj->fl_diameter = diameter;
	obj->fl_center = center;

	io_bbflash(io_cifcurlayer, diameter, center, &(obj->fl_bb.l), &(obj->fl_bb.r),
		&(obj->fl_bb.b), &(obj->fl_bb.t));

	if (io_cifdefinprog)
	{
		/* insert into symbol's guts */
		obj->fl_next = io_cifcurrent->st_guts;
		io_cifcurrent->st_guts = (objectptr) obj;
	} else io_item((objectptr)obj);		/* stick into item list */
}

void io_ipolygon(CHAR *a)
{
	polygon *obj;
	INTBIG length = io_pathlength((path)a);
	CHAR *bbpath,*spath,*tpath;
	INTBIG i;

	io_cifstatesince++;
	if (io_cifignore) return;
	if (io_cifcurlayer == -1)
	{
		io_nulllayererrors++;
		return;
	}
	if (length < 3)
	{
		io_report(_("polygon with < 3 pts in path, ignored"), ADVISORY);
		return;
	}

	obj = (polygon *)emalloc((length-1)*sizeof(point)+sizeof(polygon), io_tool->cluster);
	if (obj == NIL)
	{
		io_report(_("no space: io_ipolygon"), FATALINTERNAL);
		return;
	}

	tpath = a;
	spath = NIL;			/* path in case of scaling */
	obj->po_type = POLY;
	obj->po_layer = io_cifcurlayer;
	obj->po_numpts = length;
	if (io_cifdefinprog && io_cifscalefactor != 1.0)
	{
		spath = io_makepath();		/* create a new path */
		io_scalepath(a, spath);		/* scale all points */
		tpath = spath;
	}

	bbpath = io_makepath();		/* get a new path for bb use */
	io_copypath((path)tpath, (path)bbpath);
	io_bbpolygon(io_cifcurlayer, (path)bbpath, &(obj->po_bb.l), &(obj->po_bb.r),
		&(obj->po_bb.b), &(obj->po_bb.t));
	io_freepath((path)bbpath);
	for (i = 0; i < length; i++) obj->po_p[i] = io_removepoint((path)tpath);

	if (io_cifdefinprog)
	{
		/* insert into symbol's guts */
		obj->po_next = io_cifcurrent->st_guts;
		io_cifcurrent->st_guts = (objectptr) obj;
	} else io_item((objectptr)obj);		/* stick into item list */

	if (spath != NIL) io_freepath((path)spath);
}

static void io_scalepath(CHAR *src, CHAR *dest)
{
	INTBIG i, limit;

	limit = io_pathlength((path)src);
	for (i = 0; i < limit; i++)
	{
		point temp;

		temp = io_removepoint((path)src);

		temp.x = (INTBIG) (io_cifscalefactor*(float)temp.x);
		temp.y = (INTBIG) (io_cifscalefactor*(float)temp.y);
		if (io_appendpoint((path)dest, temp)) break;
	}
}

void io_ibox(INTBIG length, INTBIG width, point center, INTBIG xr, INTBIG yr)
{
	INTBIG tl, tr, tb, tt, halfw, halfl;

	io_cifstatesince++;
	if (io_cifignore) return;
	if (io_cifcurlayer == -1)
	{
		io_nulllayererrors++;
		return;
	}
	if (length == 0 || width == 0)
	{
		io_report(_("box with null length or width specified, ignored"), ADVISORY);
		return;
	}

	if (io_cifdefinprog && io_cifscalefactor != 1.0)
	{
		length = (INTBIG) (io_cifscalefactor * (float)length);
		width = (INTBIG) (io_cifscalefactor * (float)width);
		center.x = (INTBIG) (io_cifscalefactor * (float)center.x);
		center.y = (INTBIG) (io_cifscalefactor * (float)center.y);
	}

	io_bbbox(io_cifcurlayer, length, width, center, xr, yr, &tl, &tr, &tb, &tt);

	/* check for manhattan box */
	halfw = width/2; halfl = length/2;
	if (
		(yr == 0 && (length%2) == 0 && (width%2) == 0 &&
		(center.x-halfl) == tl && (center.x+halfl) == tr &&
		(center.y-halfw) == tb && (center.y+halfw) == tt)
		||
		(xr == 0 && (length%2) == 0 && (width%2) == 0 &&
		(center.x-halfw) == tl && (center.x+halfw) == tr &&
		(center.y-halfl) == tb && (center.y+halfl) == tt)
	)
	{
		mbox *obj;
		obj = (mbox *)emalloc(sizeof(mbox), io_tool->cluster);
		if (obj == NIL)
		{
			io_report(_("no space: io_ibox"), FATALINTERNAL);
			return;
		}
		obj->mb_type = MBOX;
		obj->mb_layer = io_cifcurlayer;
		if (yr == 0)
		{
			obj->mb_bb.l = tl;
			obj->mb_bb.r = tr;
			obj->mb_bb.b = tb;
			obj->mb_bb.t = tt;
		} else
		{
			/* this assumes that bb is unaffected by rotation */
			obj->mb_bb.l = center.x-halfw;
			obj->mb_bb.r = center.x+halfw;
			obj->mb_bb.b = center.y-halfl;
			obj->mb_bb.t = center.y+halfl;
		}
		if (io_cifdefinprog)
		{
			/* insert into symbol's guts */
			obj->mb_next = io_cifcurrent->st_guts;
			io_cifcurrent->st_guts = (objectptr) obj;
		} else io_item((objectptr)obj);		/* stick into item list */
	} else
	{
		box *obj;

		obj = (box *)emalloc(sizeof(box), io_tool->cluster);
		if (obj == NIL)
		{
			io_report(_("no space: io_ibox"), FATALINTERNAL);
			return;
		}
		obj->bo_type = RECTANGLE;
		obj->bo_layer = io_cifcurlayer;
		obj->bo_length = length;
		obj->bo_width = width;
		obj->bo_center = center;
		obj->bo_xrot = xr;
		obj->bo_yrot = yr;

		obj->bo_bb.l = tl;
		obj->bo_bb.r = tr;
		obj->bo_bb.b = tb;
		obj->bo_bb.t = tt;
		if (io_cifdefinprog)
		{
			/* insert into symbol's guts */
			obj->bo_next = io_cifcurrent->st_guts;
			io_cifcurrent->st_guts = (objectptr) obj;
		} else io_item((objectptr)obj);		/* stick into item list */
	}
}

void io_icall(INTBIG symbol, CHAR *list)
{
	INTBIG i,j;
	tentry temp;
	symcall *obj;		/* we need a new object */
	CHAR *newtlist;

	if (io_cifignore) return;
	j = io_tlistlength((tlist)list);
	if (j == 0) newtlist = NIL; else
		newtlist = io_maketlist();

	io_pushtrans();		/* get new frame of reference */
	for (i = 1; i <=j; i++)
	{
		/* build up incremental transformations */
		temp = io_removetentry((tlist)list);
		switch (temp.kind)
		{
			case MIRROR:
				io_mirror(temp.guts.mi.xcoord); break;
			case TRANSLATE:
				if (io_cifdefinprog && io_cifscalefactor != 1.0)
				{
					temp.guts.tr.xt = (INTBIG)(io_cifscalefactor*(float)temp.guts.tr.xt);
					temp.guts.tr.yt = (INTBIG)(io_cifscalefactor*(float)temp.guts.tr.yt);
				}
				io_translate(temp.guts.tr.xt, temp.guts.tr.yt);
				break;
		   case ROTATE:
				io_rotate(temp.guts.ro.xrot, temp.guts.ro.yrot); break;
		   default:
				io_report(_("interpreter: no such transformation"), FATALINTERNAL);
		}
		io_appendtentry((tlist)newtlist, temp);	/* copy the list */
	}

	obj = (symcall *)emalloc(sizeof(symcall), io_tool->cluster);
	if (obj == NIL)
	{
		io_report(_("no space: io_icall"), FATALINTERNAL);
		io_poptrans();
		return;
	}
	obj->sy_type = CALL;
	obj->sy_tm = io_getlocal();
	io_poptrans();		/* return to previous state */

	obj->sy_symnumber = symbol;
	obj->sy_unid = NIL;
	obj->sy_tlist = newtlist;

	io_cifinstance++;		/* increment the instance count for names */
	if (io_cifnamepending)
	{
		if (io_cifstatesince != 0)
			io_report(_("statements between name and instance"), ADVISORY);
		obj->sy_name = io_cifnstored;
		io_cifnamepending = 0;
	} else
	{
		CHAR *s;
		s = io_makeinstname(io_cifinstance);
		(void)allocstring(&obj->sy_name, s, io_tool->cluster);
	}
	if (io_cifdefinprog)
	{
		/* insert into guts of symbol */
		obj->sy_next = io_cifcurrent->st_guts;
		io_cifcurrent->st_guts = (objectptr)obj;
		io_cifcurrent->st_ncalls++;
	} else io_item((objectptr)obj);
}

void io_isymname(CHAR *name)
{
	io_cifstatesince++;
	if (io_cifignore) return;
	if (!io_cifdefinprog)
	{
		io_report(_("no symbol to name"), FATALSEMANTIC);
		return;
	}
	if (*name == '\0')
	{
		io_report(_("null symbol name ignored"), ADVISORY);
		return;
	}
	if (io_cifnamed)
	{
		io_report(_("symbol is already named, new name ignored"), FATALSEMANTIC);
		return;
	}
	io_cifnamed = 1;
	(void)allocstring(&io_cifcurrent->st_name, name, io_tool->cluster);
}

void io_iinstname(CHAR *name)
{
	if (io_cifignore) return;
	if (*name == '\0')
	{
		io_report(_("null instance name ignored"), ADVISORY);
		return;
	}
	if (io_cifnamepending)
	{
		efree((CHAR *)io_cifnstored);
		io_report(_("there is already a name pending, new name replaces it"), ADVISORY);
	}
	io_cifnamepending = 1;
	io_cifstatesince = 0;
	(void)allocstring(&io_cifnstored, name, io_tool->cluster);
}

void io_igeoname(CHAR *name, point pt, INTBIG lay)
{
	gname *obj;
	point temp;

	io_cifstatesince++;
	if (io_cifignore) return;
	if (*name == '\0')
	{
		io_report(_("null geometry name ignored"), ADVISORY);
		return;
	}
	obj = (gname *)emalloc(sizeof(gname), io_tool->cluster);
	if (obj == NIL)
	{
		io_report(_("no space: io_igeoname"), FATALINTERNAL);
		return;
	}

	obj->gn_type = NAME;
	obj->gn_layer = lay;
	if (io_cifdefinprog && io_cifscalefactor != 1.0)
	{
		pt.x = (INTBIG)(io_cifscalefactor * (float)pt.x);
		pt.y = (INTBIG)(io_cifscalefactor * (float)pt.y);
	}
	obj->gn_pos = pt;
	(void)allocstring(&obj->gn_name, name, io_tool->cluster);

	io_pushtrans();
	temp = io_transpoint(pt);
	io_poptrans();
	obj->gn_bb.l = temp.x;
	obj->gn_bb.r = temp.x;
	obj->gn_bb.b = temp.y;
	obj->gn_bb.t = temp.y;

	if (io_cifdefinprog)
	{
		/* insert into symbol's guts */
		obj->gn_next = io_cifcurrent->st_guts;
		io_cifcurrent->st_guts = (objectptr) obj;
	} else io_item((objectptr)obj);		/* stick into item list */
}

void io_ilabel(CHAR *name, point pt)
{
	label *obj;
	point temp;

	io_cifstatesince++;
	if (io_cifignore) return;
	if (*name == '\0')
	{
		io_report(_("null label ignored"), ADVISORY);
		return;
	}
	obj = (label *)emalloc(sizeof(label), io_tool->cluster);
	if (obj == NIL)
	{
		io_report(_("no space: io_ilabel"), FATALINTERNAL);
		return;
	}

	obj->la_type = LABEL;
	if (io_cifdefinprog && io_cifscalefactor != 1.0)
	{
		pt.x = (INTBIG)(io_cifscalefactor * (float)pt.x);
		pt.y = (INTBIG)(io_cifscalefactor * (float)pt.y);
	}
	obj->la_pos = pt;
	(void)allocstring(&obj->la_name, name, io_tool->cluster);

	io_pushtrans();
	temp = io_transpoint(pt);
	io_poptrans();
	obj->la_bb.l = temp.x;
	obj->la_bb.r = temp.x;
	obj->la_bb.b = temp.y;
	obj->la_bb.t = temp.y;

	if (io_cifdefinprog)
	{
		/* insert into symbol's guts */
		obj->la_next = io_cifcurrent->st_guts;
		io_cifcurrent->st_guts = (objectptr) obj;
	} else io_item((objectptr)obj);		/* stick into item list */
}

/********************* util.c: storage manager begin *********************/

CHAR *io_makesymname(INTBIG n)
{
	static CHAR s[50];

	(void)esnprintf(s, 50, x_("SYM%lu"), n);
	return(s);
}

CHAR *io_makeinstname(INTBIG n)
{
	static CHAR s[50];

	(void)esnprintf(s, 50, x_("INST<%lu>"), n);
	return(s);
}

/********************* symbol table begin *********************/

#define PRIME 199		/* hash table size */
#define hash(x) (x % PRIME)	/* hash function */
static stentry *io_cifstable[PRIME];	/* symbol table */

/* initialize the entry pointed to by ptr */
static void io_newstentry(stentry *ptr, INTBIG num)
{
	ptr->st_symnumber = num;
	ptr->st_overflow = NIL;
	ptr->st_expanded = 0;
	ptr->st_frozen = 0;
	ptr->st_defined = 0;
	ptr->st_dumped = 0;		/* added for winstanley */
	ptr->st_ncalls = 0;		/* ditto */
	ptr->st_bbvalid = 0;
	ptr->st_name = NIL;
	ptr->st_guts = NIL;
}

/* io_lookupsym(sym), if none, make
 * a blank entry. return a pointer to whichever
 */
stentry *io_lookupsym(INTBIG sym)
{
	stentry *cur, *old;
	INTBIG hadd = hash(sym);

	if (io_cifstable[hadd] == NIL)
	{
		/* create a new entry */
		io_cifstable[hadd] = (stentry *)emalloc(sizeof(stentry), io_tool->cluster);
		if (io_cifstable[hadd] == NIL)
		{
			io_report(_("no space: io_lookupsym"), FATALINTERNAL);
			return(0);
		}
		io_newstentry(io_cifstable[hadd], sym);
		return(io_cifstable[hadd]);
	}

	/* otherwise find stentry w/correct symbolnumber */
	cur = io_cifstable[hadd];
	while (cur != NIL)
		if (cur->st_symnumber == sym)
			return(cur);	/* found a match */
				else {old = cur; cur = cur->st_overflow;}

	/* no matching symbol numbers, so create a new one */
	old->st_overflow = (stentry *)emalloc(sizeof(stentry), io_tool->cluster);
	if (old->st_overflow == NIL)
	{
		io_report(_("no space: io_lookupsym"), FATALINTERNAL);
		return(0);
	}
	io_newstentry(old->st_overflow, sym);
	return(old->st_overflow);
}

void io_freesymbols(void)
{
	stentry *cur, *temp;
	INTBIG i;

	for (i=0; i<PRIME; i++)
	{
		for (cur = io_cifstable[i]; cur != NIL; cur = temp)
		{
			temp = cur->st_overflow;

			/* deallocate name string */
			if (cur->st_name != NIL) efree(cur->st_name);
			io_freedef(cur->st_guts);	/* deallocate contents */
			efree((CHAR *)cur);	/* deallocate this symbol table entry */
		}
	}
}

/********************* some reentrant min-max stuff *********************/

#define MAXMMSTACK 50		/* max depth of minmax stack */
static INTBIG io_cifmmleft[MAXMMSTACK];
static INTBIG io_cifmmright[MAXMMSTACK];
static INTBIG io_cifmmbottom[MAXMMSTACK];
static INTBIG io_cifmmtop[MAXMMSTACK];
static INTBIG io_cifmmptr;			/* stack pointer */

void io_initmm(point foo)
{
	if (++io_cifmmptr >= MAXMMSTACK)
	{
		io_report(_("io_initmm: out of stack"), FATALINTERNAL);
		return;
	}
	io_cifmmleft[io_cifmmptr] = foo.x;   io_cifmmright[io_cifmmptr] = foo.x;
	io_cifmmbottom[io_cifmmptr] = foo.y; io_cifmmtop[io_cifmmptr] = foo.y;
}

void io_donemm(void)
{
	if (io_cifmmptr < 0) io_report(_("io_donemm: pop from empty stack"), FATALINTERNAL);
		else io_cifmmptr--;
}

void io_minmax(point foo)
{
	if (foo.x > io_cifmmright[io_cifmmptr]) io_cifmmright[io_cifmmptr] = foo.x;
		else {if (foo.x < io_cifmmleft[io_cifmmptr]) io_cifmmleft[io_cifmmptr] = foo.x;}
	if (foo.y > io_cifmmtop[io_cifmmptr]) io_cifmmtop[io_cifmmptr] = foo.y;
		else {if (foo.y < io_cifmmbottom[io_cifmmptr]) io_cifmmbottom[io_cifmmptr] = foo.y;}
}

INTBIG io_minx(void)
{
	return(io_cifmmleft[io_cifmmptr]);
}

INTBIG io_miny(void)
{
	return(io_cifmmbottom[io_cifmmptr]);
}

INTBIG io_maxx(void)
{
	return(io_cifmmright[io_cifmmptr]);
}

INTBIG io_maxy(void)
{
	return(io_cifmmtop[io_cifmmptr]);
}

void io_findcallbb(symcall *object)		/* find the bb for this particular call */
{
	point temp;
	point comperror;	/* for compiler error */
	stentry *thisst;

	thisst = io_lookupsym(object->sy_symnumber);

	if (!thisst->st_defined)
	{
		CHAR mess[50];
		(void)esnprintf(mess, 50, _("call to undefined symbol %lu"), thisst->st_symnumber);
		io_report(mess, FATALSEMANTIC);
		return;
	}
	if (thisst->st_expanded)
	{
		CHAR mess[50];
		(void)esnprintf(mess, 50, _("recursive call on symbol %lu"), thisst->st_symnumber);
		io_report(mess, FATALSEMANTIC);
		return;
	} else thisst->st_expanded = 1;		/* mark as under expansion */

	if (!thisst->st_frozen) thisst->st_frozen = 1;

	io_findbb(thisst);		/* get the bb of the symbol in its stentry */
	object->sy_unid = thisst;	/* get this symbol's id */

	io_pushtrans();			/* set up a new frame of reference */
	io_applylocal(&(object->sy_tm));
	temp.x = thisst->st_bb.l;   temp.y = thisst->st_bb.b;	/* ll */
	comperror = io_transpoint(temp);
	io_initmm(comperror);
	temp.x = thisst->st_bb.r;
	comperror = io_transpoint(temp);
	io_minmax(comperror);
	temp.y = thisst->st_bb.t;	/* ur */
	comperror = io_transpoint(temp);
	io_minmax(comperror);
	temp.x = thisst->st_bb.l;
	comperror = io_transpoint(temp);
	io_minmax(comperror);

	object->sy_bb.l = io_minx();   object->sy_bb.r = io_maxx();
	object->sy_bb.b = io_miny();   object->sy_bb.t = io_maxy();
	io_donemm();		/* object now has transformed bb of the symbol */
	io_poptrans();

	thisst->st_expanded = 0;
}

static void io_findbb(stentry *sym)		/* find bb for sym */
{
	point temp;
	CHAR first = 1;
	struct cifhack *ob = (struct cifhack *)sym->st_guts;

	if (sym->st_bbvalid) return;			/* already done */
	if (ob == NIL)			/* empty symbol */
	{
		CHAR mess[100];
		(void)esnprintf(mess, 100, _("symbol %lu has no geometry in it"),
			sym->st_symnumber);
		io_report(mess, ADVISORY);
		sym->st_bb.l = 0;   sym->st_bb.r = 0;
		sym->st_bb.b = 0;   sym->st_bb.t = 0;
		sym->st_bbvalid = 1;
		return;
	}

	while (ob != NIL)
	{
		/* find bb for symbol calls, all primitive are done already */
		if (ob->kl_type == CALL) io_findcallbb((symcall *)ob);
		temp.x = ob->kl_bb.l;   temp.y = ob->kl_bb.b;
		if (first) {first = 0; io_initmm(temp);}
			else io_minmax(temp);
		temp.x = ob->kl_bb.r;   temp.y = ob->kl_bb.t;
		io_minmax(temp);
		ob = ob->kl_next;
	}
	sym->st_bb.l = io_minx();   sym->st_bb.r = io_maxx();
	sym->st_bb.b = io_miny();   sym->st_bb.t = io_maxy();
	sym->st_bbvalid = 1;
	io_donemm();
}

CHAR io_initutil(void)
{
	INTBIG i;

	for (i = 0; i < PRIME; i++) io_cifstable[i] = NIL;
	io_cifmmptr = -1;			/* minmax stack pointer */
	return(0);
}

/********************* storage deallocator begin **********/

void io_freestorage(void)
{
	io_freesymbols();		/* walk symbol table, free all definitions */
}

void io_freedef(objectptr head)
{
	struct cifhack *cur, *temp;

	for(cur = (struct cifhack *)head; cur != NIL; cur = temp)
	{
		temp = cur->kl_next;
		io_freeonedef(cur);
	}
}

void io_freeonedef(struct cifhack *cur)
{
	switch (cur->kl_type)
	{
		case RECTANGLE:
		case POLY:
		case WIRE:
		case MBOX:
		case FLASH:
			efree((CHAR *)cur);
			break;
		case CALL:
			if (((symcall *)cur)->sy_tlist != 0)
				io_freetlist((tlist) ((symcall *)cur)->sy_tlist);
			efree(((symcall *)cur)->sy_name);
			efree((CHAR *)cur);
			break;
		case NAME:
			efree(((gname *)cur)->gn_name);
			efree((CHAR *)cur);
			break;
		case LABEL:
			efree(((label *)cur)->la_name);
			efree((CHAR *)cur);
			break;
		default:
			io_report(_("unknown def contents"), FATALSEMANTIC);
	}
}

/********************* storage deallocator end ************/
/********************* trans.c: begin *********************/

#define TIDENT 0
#define TROTATE 1
#define TTRANSLATE 2
#define TMIRROR 4

#define CONTSIZE 50		/* initial number of contexts */

typedef struct nba
{
	CHAR *sn_inst, *sn_sym;
	struct nba *sn_prev, *sn_next;
} snode;

typedef struct
{
	transform base;				/* bottom of stack */
	INTBIG    refcount;			/* not inuse if = 0 */
	transform ctop;				/* current top */
	snode     *nbase, *ntop;	/* name stack base,top */
} context;

static context  *io_cifcarray;		/* pointer to the first context */
static INTBIG    io_cifclength;		/* length of context array */
static INTBIG    io_cifcurcontext;	/* current context */
static transform io_cifstacktop;	/* the top of stack */

CHAR io_inittrans(void)
{
	INTBIG i;

	io_cifstacktop = (transform)emalloc(sizeof(tmatrix), io_tool->cluster);
	if (io_cifstacktop == NIL)
	{
		io_report(_("no space: io_inittrans"), FATALINTERNAL);
		return(1);
	}
	io_clear(io_cifstacktop);
	io_cifstacktop->next = NIL;
	io_cifstacktop->prev = NIL;
	io_cifstacktop->multiplied = 1;

	io_cifcarray = (context *)emalloc((CONTSIZE*sizeof(context)), io_tool->cluster);
	if (io_cifcarray == NIL)
	{
		io_report(_("no space: intitrans"), FATALINTERNAL);
		return(1);
	}
	io_cifcarray[0].base = io_cifstacktop;
	io_cifcarray[0].refcount = 1;	/* this context will never go away */
	io_cifcarray[0].ctop = io_cifstacktop;
	io_cifcarray[0].nbase = NIL;
	io_cifcarray[0].ntop = NIL;
	io_cifclength = CONTSIZE;
	for (i=1; i < CONTSIZE; i++)
	{
		io_cifcarray[i].base = NIL;
		io_cifcarray[i].refcount = 0;
		io_cifcarray[i].ctop = NIL;
		io_cifcarray[i].nbase = NIL;
		io_cifcarray[i].ntop = NIL;
	}
	io_cifcurcontext = 0;		/* where we are now */
	return(0);
}

CHAR io_donetrans(void)
{
	transform temp, old;

	/* free all contexts, this version only uses one stack */
	for (temp = io_cifstacktop->prev; temp != NIL; temp = old)
	{
		old = temp->prev;
		efree((CHAR *)temp);
	}
	for (temp = io_cifstacktop->next; temp != NIL; temp = old)
	{
		old = temp->next;
		efree((CHAR *)temp);
	}
	efree((CHAR *)io_cifstacktop);
	efree((CHAR *)io_cifcarray);
	return(0);
}

/**************** initialize a matrix, but don't reset links ****************/

static void io_clear(transform mat)
{
	mat->a11 = 1.0;   mat->a12 = 0.0;
	mat->a21 = 0.0;   mat->a22 = 1.0;
	mat->a31 = 0.0;   mat->a32 = 0.0;   mat->a33 = 1.0;
	mat->type = TIDENT;   mat->multiplied = 0;
}

BOOLEAN io_swapcontext(INTBIG id)
{
	if (id >= io_cifclength || io_cifcarray[id].refcount == 0)
	{
		io_report(_("illegal swap context"), FATALINTERNAL);
		return(TRUE);
	}
	if (io_cifcurcontext == id) return(FALSE);
	io_cifcarray[io_cifcurcontext].ctop = io_cifstacktop;		/* save current context */
	io_cifcurcontext = id;
	io_cifstacktop = io_cifcarray[io_cifcurcontext].ctop;
	return(FALSE);
}

BOOLEAN io_increfcount(INTBIG id)
{
	if (id >= io_cifclength)
	{
		io_report(_("illegal context: io_increfcount"), FATALINTERNAL);
		return(TRUE);
	}
	io_cifcarray[id].refcount++;
	return(FALSE);
}

BOOLEAN io_decrefcount(INTBIG id)
{
	if (id >= io_cifclength || io_cifcarray[id].refcount == 0)
	{
		io_report(_("illegal context: io_decrefcount"), FATALINTERNAL);
		return(TRUE);
	}
	io_cifcarray[id].refcount--;
	return(FALSE);
}

void io_pushtrans(void)
{
	if (io_cifstacktop->next == NIL)
	{
		io_cifstacktop->next = (transform)emalloc(sizeof(tmatrix), io_tool->cluster);
		if (io_cifstacktop->next == NIL)
			io_report(_("no space: io_pushtrans"), FATALINTERNAL);
		io_clear(io_cifstacktop->next);
		io_cifstacktop->next->prev = io_cifstacktop;
		io_cifstacktop = io_cifstacktop->next;
		io_cifstacktop->next = NIL;
	} else
	{
		io_cifstacktop = io_cifstacktop->next;
		io_clear(io_cifstacktop);
	}
}

void io_poptrans(void)
{
	if (io_cifstacktop->prev != NIL) io_cifstacktop = io_cifstacktop->prev;
		else io_report(_("pop, empty trans stack"), FATALINTERNAL);
}

static void io_matmult(transform l, transform r, transform result)
{
	if (l == NIL || r == NIL || result == NIL)
		io_report(_("null arg to io_matmult"), FATALINTERNAL);
	if (result->multiplied)
	{
		io_dumpstack();
		io_report(_("can't re-mult tmatrix"), FATALINTERNAL);
		return;
	}
	if (!r->multiplied)
	{
		tmatrix temp;

		temp.multiplied = 0;
		io_matmult(r, r->prev, &temp);
		io_mmult(l, &temp, result);
	} else io_mmult(l, r, result);
}

static void io_mmult(transform l,transform r,transform result)
{
	if (l == NIL || r == NIL || result == NIL)
	{
		io_report(_("null arg to io_mmult"), FATALINTERNAL);
		return;
	}
	if (l->type == TIDENT) io_assign(r, result); else
		if (r->type == TIDENT) io_assign(l, result); else
	{
		tmatrix temp;

		temp.a11 = l->a11 * r->a11 + l->a12 * r->a21;
		temp.a12 = l->a11 * r->a12 + l->a12 * r->a22;
		temp.a21 = l->a21 * r->a11 + l->a22 * r->a21;
		temp.a22 = l->a21 * r->a12 + l->a22 * r->a22;
		temp.a31 = l->a31 * r->a11 + l->a32 * r->a21 + l->a33 * r->a31;
		temp.a32 = l->a31 * r->a12 + l->a32 * r->a22 + l->a33 * r->a32;
		temp.a33 = l->a33*r->a33;
		temp.type = l->type | r->type;
		io_assign(&temp,result);
	}
	if (result->a33 != 1.0)
	{
		/* divide by a33 */
		result->a11 /= result->a33;
		result->a12 /= result->a33;
		result->a21 /= result->a33;
		result->a22 /= result->a33;
		result->a31 /= result->a33;
		result->a32 /= result->a33;
		result->a33 = 1.0;
	}
	result->multiplied = 1;
}

tmatrix io_getlocal(void)
{
	return(*io_cifstacktop);
}

void io_applylocal(transform tm)
{
	io_assign(tm,io_cifstacktop);
}

static void io_assign(transform src,transform dest)
{
	dest->a11 = src->a11;
	dest->a12 = src->a12;
	dest->a21 = src->a21;
	dest->a22 = src->a22;
	dest->a31 = src->a31;
	dest->a32 = src->a32;
	dest->a33 = src->a33;
	dest->type = src->type;
	dest->multiplied = src->multiplied;
}

void io_rotate(INTBIG xrot,INTBIG yrot)
{
	float si = (float)yrot;
	float co = (float)xrot;
	float temp;

	if (yrot == 0 && xrot >= 0) return;

	io_cifstacktop->type |= TROTATE;
	if (xrot == 0)
	{
		temp = io_cifstacktop->a11;
		io_cifstacktop->a11 = -io_cifstacktop->a12;
		io_cifstacktop->a12 = temp;

		temp = io_cifstacktop->a21;
		io_cifstacktop->a21 = -io_cifstacktop->a22;
		io_cifstacktop->a22 = temp;

		temp = io_cifstacktop->a31;
		io_cifstacktop->a31 = -io_cifstacktop->a32;
		io_cifstacktop->a32 = temp;
		if (yrot < 0) io_cifstacktop->a33 = -io_cifstacktop->a33;
	} else
		if (yrot == 0) io_cifstacktop->a33 = -io_cifstacktop->a33;	/* xrot < 0 */
	else
	{
		temp = io_cifstacktop->a11*co - io_cifstacktop->a12*si;
		io_cifstacktop->a12 = io_cifstacktop->a11*si + io_cifstacktop->a12*co;
		io_cifstacktop->a11 = temp;
		temp = io_cifstacktop->a21*co - io_cifstacktop->a22*si;
		io_cifstacktop->a22 = io_cifstacktop->a21*si + io_cifstacktop->a22*co;
		io_cifstacktop->a21 = temp;
		temp = io_cifstacktop->a31*co - io_cifstacktop->a32*si;
		io_cifstacktop->a32 = io_cifstacktop->a31*si + io_cifstacktop->a32*co;
		io_cifstacktop->a31 = temp;
		io_cifstacktop->a33 = (float)computedistance(0, 0, (INTBIG)co, (INTBIG)si);
	}
}

void io_translate(INTBIG xtrans, INTBIG ytrans)
{
	if (xtrans != 0 || ytrans != 0)
	{
		io_cifstacktop->a31 += io_cifstacktop->a33*xtrans;
		io_cifstacktop->a32 += io_cifstacktop->a33*ytrans;
		io_cifstacktop->type |= TTRANSLATE;
	}
}

void io_mirror(CHAR xcoord)
{
	if (xcoord)
	{
		io_cifstacktop->a11 = -io_cifstacktop->a11;
		io_cifstacktop->a21 = -io_cifstacktop->a21;
		io_cifstacktop->a31 = -io_cifstacktop->a31;
	} else
	{
		io_cifstacktop->a12 = -io_cifstacktop->a12;
		io_cifstacktop->a22 = -io_cifstacktop->a22;
		io_cifstacktop->a32 = -io_cifstacktop->a32;
	}
	io_cifstacktop->type |= TMIRROR;
}

point io_transpoint(point foo)
{
	point ans;

	if (!io_cifstacktop->multiplied)
	{
		io_matmult(io_cifstacktop,io_cifstacktop->prev,io_cifstacktop);
	}
	switch (io_cifstacktop->type)
	{
		case TIDENT:
			return(foo);
		case TTRANSLATE:
			ans.x = roundfloat(io_cifstacktop->a31);
			ans.y = roundfloat(io_cifstacktop->a32);
			ans.x += foo.x;   ans.y += foo.y;
			return(ans);
		case TMIRROR:
			ans.x = (io_cifstacktop->a11 < 0) ? -foo.x : foo.x;
			ans.y = (io_cifstacktop->a22 < 0) ? -foo.y : foo.y;
			return(ans);
		case TROTATE:
			ans.x = roundfloat(((float) foo.x)*io_cifstacktop->a11+((float) foo.y)*io_cifstacktop->a21);
			ans.y = roundfloat(((float) foo.x)*io_cifstacktop->a12+((float) foo.y)*io_cifstacktop->a22);
			return(ans);
		default:
			ans.x = roundfloat(io_cifstacktop->a31 + ((float) foo.x)*io_cifstacktop->a11+
				((float) foo.y)*io_cifstacktop->a21);
			ans.y = roundfloat(io_cifstacktop->a32 + ((float) foo.x)*io_cifstacktop->a12+
				((float) foo.y)*io_cifstacktop->a22);
	}
	return(ans);
}

void io_dumpstack(void)
{
	transform foo = io_cifstacktop;
	ttyputmsg(_("current context is %u"), io_cifcurcontext);
	while (foo != NIL)
	{
		io_printmat(foo);
		foo = foo->prev;
	}
}

static void io_printmat(transform t)
{
	ttyputmsg(x_("type: %ld, mult: %d"), t->type, t->multiplied);
	ttyputmsg(x_("%f %f"), t->a11, t->a12);
	ttyputmsg(x_("%f %f"), t->a21, t->a22);
	ttyputmsg(x_("%f %f %f"), t->a31, t->a32, t->a33);
}

/************ bbs.c: bounding box finder for primitive geometry ************/

void io_bbflash(INTBIG lay, INTBIG diameter, point center, INTBIG *l, INTBIG *r,
	INTBIG *b, INTBIG *t)
{
	io_bbbox(lay, diameter, diameter, center, 1, 0, l, r, b, t);
}

void io_bbbox(INTBIG lay, INTBIG length, INTBIG width, point center, INTBIG xr,
	INTBIG yr, INTBIG *l, INTBIG *r, INTBIG *b, INTBIG *t)
{
	INTBIG dx,dy;
	point temp;

	dx = length/2;
	dy = width/2;

	io_pushtrans();	/* newtrans */
	io_rotate(xr, yr);
	io_translate(center.x, center.y);
	temp.x = dx;   temp.y = dy;
	io_initmm(io_transpoint(temp));
	temp.y = -dy;
	io_minmax(io_transpoint(temp));
	temp.x = -dx;
	io_minmax(io_transpoint(temp));
	temp.y = dy;
	io_minmax(io_transpoint(temp));
	io_poptrans();
	*l = io_minx();
	*r = io_maxx();
	*b = io_miny();
	*t = io_maxy();
	io_donemm();
}

void io_bbpolygon(INTBIG lay, path ppath, INTBIG *l, INTBIG *r, INTBIG *b, INTBIG *t)
{
	INTBIG i, limit;

	limit = io_pathlength(ppath);

	io_pushtrans();	/* newtrans */
	io_initmm(io_transpoint(io_removepoint(ppath)));
	for (i = 1; i < limit; i++)
	{
		io_minmax(io_transpoint(io_removepoint(ppath)));
	}
	io_poptrans();
	*l = io_minx();
	*r = io_maxx();
	*b = io_miny();
	*t = io_maxy();
	io_donemm();
}

void io_bbwire(INTBIG lay, INTBIG width, path ppath, INTBIG *l, INTBIG *r,
	INTBIG *b, INTBIG *t)
{
	INTBIG i, limit;
	INTBIG half = (width+1)/2;

	limit = io_pathlength(ppath);

	io_pushtrans();	/* newtrans */
	io_initmm(io_transpoint(io_removepoint(ppath)));
	for (i = 1; i < limit; i++)
	{
		io_minmax(io_transpoint(io_removepoint(ppath)));
	}
	io_poptrans();
	*l = io_minx()-half;
	*r = io_maxx()+half;
	*b = io_miny()-half;
	*t = io_maxy()+half;
	io_donemm();
}

/********************* items.c: item tree stuff *********************/

CHAR io_inititems(void)
{
	io_cifittop = NIL;
	io_cifilist = NIL;
	return(0);
}

BOOLEAN io_doneitems(void)
{
	itementry *cur, *next;

	for (cur = io_cifittop; cur != NIL; cur = next)
	{
		next = cur->it_same;
		io_freeonedef((struct cifhack *)cur->it_what);
		efree((CHAR *)cur);
	}
	for (cur = io_cifilist; cur != NIL; cur = next)
	{
		next = cur->it_same;
		io_freeonedef((struct cifhack *)cur->it_what);
		efree((CHAR *)cur);
	}
	return(FALSE);
}

/*
 * allocate and free item nodes, come not from interpreter storage,
 * since they don't need to be saved with compiled stuff.
 */
static itementry *io_newnode(void)		/* return a new itementry */
{
	itementry *ans;

	if (io_cifittop == NIL)
	{
		ans = (itementry *)emalloc(sizeof(itementry), io_tool->cluster);
		if (ans == NIL)
		{
			io_report(_("io_newnode: no space"), FATALINTERNAL);
			return(NIL);
		} else return(ans);
	}
	ans = io_cifittop;
	io_cifittop = io_cifittop->it_same;
	return(ans);
}

static void io_freenode(itementry *ptr)
{
	ptr->it_same = io_cifittop;
	io_cifittop = ptr;
}

static void io_sendlist(itementry *list)
{
	itementry *save, *h;

	h = list;
	while (h != NIL)
	{
		save = h->it_same;
		io_outitem(h);
		h = save;
	}
}

static void io_dumpdef(stentry *sym)
{
	struct cifhack *ro;

	if (sym->st_dumped) return;		/* already done */
	if (sym->st_ncalls > 0)			/* dump all children */
	{
		INTBIG count = sym->st_ncalls;

		ro = (struct cifhack *) sym->st_guts;
		while ((ro != NIL) && (count > 0))
		{
			if (ro->kl_type == CALL)
			{
				io_dumpdef(((symcall *) ro)->sy_unid);
				count--;
			}
			ro = ro->kl_next;
		}
	}
	io_shipcontents(sym);
	sym->st_dumped = 1;
}

static void io_shipcontents(stentry *sym)
{
	struct cifhack *ro;

	ro = (struct cifhack *) sym->st_guts;
	io_outputds(sym->st_symnumber, sym->st_name,
		sym->st_bb.l, sym->st_bb.r, sym->st_bb.b, sym->st_bb.t);
	while (ro != NIL)
	{
		switch (ro->kl_type)
		{
			CHAR *ppath, *t_list;
			INTBIG length, i;
			point temp;

			case POLY:
				length = ((polygon *)ro)->po_numpts;
				ppath = io_makepath();
				for (i = 0; i < length; i++)
					if (io_appendpoint((path)ppath, ((polygon *)ro)->po_p[i])) return;
				io_outputpolygon(((polygon *)ro)->po_layer, ppath);
				io_freepath((path)ppath);
				break;
			case WIRE:
				length = ((wire *)ro)->wi_numpts;
				ppath = io_makepath();
				for (i = 0; i < length; i++)
					if (io_appendpoint((path)ppath, ((wire *)ro)->wi_p[i])) return;
				io_outputwire(((wire *)ro)->wi_layer, ((wire *)ro)->wi_width,
					ppath);
				io_freepath((path)ppath);
				break;
			case FLASH:
				io_outputflash(((flash *)ro)->fl_layer, ((flash *)ro)->fl_diameter,
					((flash *)ro)->fl_center);
				break;
			case RECTANGLE:
				io_outputbox(((box *)ro)->bo_layer, ((box *)ro)->bo_length,
					((box *)ro)->bo_width, ((box *)ro)->bo_center,
						((box *)ro)->bo_xrot, ((box *)ro)->bo_yrot);
				break;
			case MBOX:
				temp.x = (((mbox *)ro)->mb_bb.r + ((mbox *) ro)->mb_bb.l)/2;
				temp.y = (((mbox *)ro)->mb_bb.t + ((mbox *) ro)->mb_bb.b)/2;
				io_outputbox(((mbox *)ro)->mb_layer,
					((mbox *)ro)->mb_bb.r-((mbox *)ro)->mb_bb.l,
						((mbox *)ro)->mb_bb.t-((mbox *)ro)->mb_bb.b, temp, 1, 0);
				break;
			case CALL:
				t_list = io_maketlist();
				io_dupetlist((tlist)((symcall *)ro)->sy_tlist, (tlist)t_list);
				io_outputcall(((symcall *)ro)->sy_symnumber,
					((symcall *)ro)->sy_unid->st_name, t_list);
				io_freetlist((tlist)t_list);
				break;
			case NAME:
				io_outputgeoname(((gname *)ro)->gn_name,
					((gname *)ro)->gn_pos, ((gname *)ro)->gn_layer);
				break;
			case LABEL:
				io_outputlabel(((label *)ro)->la_name, ((label *)ro)->la_pos);
				break;
		}
		ro = ro->kl_next;
	}
	io_outputdf();
}

void io_iboundbox(INTBIG *l, INTBIG *r, INTBIG *b, INTBIG *t)
{
	itementry *h;
	struct cifhack *obj;
	point temp;
	point comperror;	/* hack for compiler error */
	CHAR first;

	h = io_cifilist;
	first = 1;

	if (h == NIL)
	{
		io_report(_("item list is empty!"), ADVISORY);
		return;
	}

	(void)io_swapcontext(0);
	io_pushtrans();
	while (h != NIL)
	{
		obj = (struct cifhack *)h->it_what;
		temp.x = obj->kl_bb.l;
		temp.y = obj->kl_bb.b;
		comperror = io_transpoint(temp);
		io_initmm(comperror);
		temp.x = obj->kl_bb.r;
		comperror = io_transpoint(temp);
		io_minmax(comperror);
		temp.y = obj->kl_bb.t;
		comperror = io_transpoint(temp);
		io_minmax(comperror);
		temp.x = obj->kl_bb.l;
		comperror = io_transpoint(temp);
		io_minmax(comperror);

		h->it_left = io_minx();
		h->it_right = io_maxx();
		h->it_bottom = io_miny();
		h->it_top = io_maxy();
		io_donemm();
		temp.x = h->it_left;   temp.y = h->it_bottom;
		if (first) {first = 0; io_initmm(temp);} else io_minmax(temp);
		temp.x = h->it_right;   temp.y = h->it_top;
		io_minmax(temp);
		h = h->it_same;
	}
	*l = io_minx();
	*r = io_maxx();
	*b = io_miny();
	*t = io_maxy();
	io_donemm();
	io_poptrans();
	(void)io_swapcontext(0);		/* always leave in context 0 */
}

BOOLEAN io_createlist(void)
{
	if (!io_endseen()) ttyputmsg(_("missing End command, assumed"));
	if (io_fatalerrors() > 0) return(TRUE);
	io_sendlist(io_cifilist);		/* sendlist deletes nodes */
	io_cifilist = NIL;
	(void)io_swapcontext(0);
	return(FALSE);
}

static void io_outitem(itementry *thing)		/* spit out an item */
{
	switch (thing->it_type)
	{
		CHAR *ppath, *t_list;
		INTBIG length, i;
		point temp;

		case POLY:
			(void)io_swapcontext(thing->it_context);
			length = ((polygon *) (thing->it_what))->po_numpts;
			ppath = io_makepath();
			for (i = 0; i < length; i++)
				if (io_appendpoint((path)ppath, ((polygon *)(thing->it_what))->po_p[i])) return;
			io_outputpolygon(((polygon *) (thing->it_what))->po_layer, ppath);
			(void)io_decrefcount(thing->it_context);
			io_freepath((path)ppath);
			break;

		case WIRE:
			(void)io_swapcontext(thing->it_context);
			length = ((wire *) (thing->it_what))->wi_numpts;
			ppath = io_makepath();
			for (i = 0; i < length; i++)
				if (io_appendpoint((path)ppath, ((wire *) (thing->it_what))->wi_p[i])) return;
			io_outputwire(((wire *) (thing->it_what))->wi_layer,
				((wire *) (thing->it_what))->wi_width, ppath);
			(void)io_decrefcount(thing->it_context);
			io_freepath((path)ppath);
			break;

		case FLASH:
			(void)io_swapcontext(thing->it_context);
			io_outputflash(((flash *) (thing->it_what))->fl_layer,
				((flash *) (thing->it_what))->fl_diameter, ((flash *) (thing->it_what))->fl_center);
			(void)io_decrefcount(thing->it_context);
			break;

		case RECTANGLE:
			(void)io_swapcontext(thing->it_context); /* get right frame of ref */
			io_outputbox(((box *) (thing->it_what))->bo_layer,
				((box *) (thing->it_what))->bo_length, ((box *) (thing->it_what))->bo_width,
					((box *) (thing->it_what))->bo_center, ((box *) (thing->it_what))->bo_xrot,
						((box *) (thing->it_what))->bo_yrot);
			(void)io_decrefcount(thing->it_context);
			break;

		case MBOX:
			(void)io_swapcontext(thing->it_context); /* get right frame of ref */
			temp.x = (((mbox *) (thing->it_what))->mb_bb.r+
				((mbox *) (thing->it_what))->mb_bb.l)/2;
			temp.y = (((mbox *) (thing->it_what))->mb_bb.t+
				((mbox *) (thing->it_what))->mb_bb.b)/2;
			io_outputbox(
				((mbox *) (thing->it_what))->mb_layer,
				((mbox *) (thing->it_what))->mb_bb.r-
				((mbox *) (thing->it_what))->mb_bb.l,
				((mbox *) (thing->it_what))->mb_bb.t-
				((mbox *) (thing->it_what))->mb_bb.b,
				temp, 1, 0);
			(void)io_decrefcount(thing->it_context);
			break;

		case CALL:
			(void)io_swapcontext(thing->it_context);
			io_pushtrans();
			io_applylocal(&(((symcall *) (thing->it_what))->sy_tm));
			t_list = io_maketlist();
			io_dupetlist((tlist)((symcall *) (thing->it_what))->sy_tlist, (tlist)t_list);
			io_dumpdef(((symcall *) (thing->it_what))->sy_unid);
			io_outputcall(((symcall *) (thing->it_what))->sy_symnumber,
				((symcall *) (thing->it_what))->sy_unid->st_name, t_list);
			io_freetlist((tlist)t_list);
			(void)io_swapcontext(thing->it_context);
			io_poptrans();
			(void)io_decrefcount(thing->it_context);
			break;

		case NAME:
			(void)io_swapcontext(thing->it_context);
			io_outputgeoname(((gname *) (thing->it_what))->gn_name,
				((gname *) (thing->it_what))->gn_pos, ((gname *) (thing->it_what))->gn_layer);
			(void)io_decrefcount(thing->it_context);
			break;

		case LABEL:
			(void)io_swapcontext(thing->it_context);
			io_outputlabel(((label *) (thing->it_what))->la_name,
				((label *) (thing->it_what))->la_pos);
			(void)io_decrefcount(thing->it_context);
			break;
	}
	io_freenode(thing);
}

void io_item(objectptr object)		/* a bare item has been found */
{
	struct cifhack *obj;
	itementry *newitem;

	if (object == NIL)
	{
		io_report(_("item: null object"), FATALINTERNAL);
		return;
	}
	obj = (struct cifhack *)object;

	newitem = io_newnode();
	newitem->it_type = obj->kl_type;
	newitem->it_rel = NIL;
	newitem->it_norel = NIL;
	newitem->it_same = io_cifilist;		/* hook into linked list */
	io_cifilist = newitem;
	newitem->it_what = object;
	newitem->it_level = 0;		/* bare item */
	newitem->it_context = 0;		/* outermost context */
	(void)io_increfcount(0);

	/* symbol calls only */
	if (obj->kl_type == CALL) io_findcallbb((symcall *)object);
}

/********************* parser.c: begin *********************/

/*
 *	same as xp.c except removed
 *	efree()'s before realloc()'s
 *
 *	xp.c
 *	cif 2.0 parser
 *	experimental parser that includes names
 *	consists of 4 modules -- parser,error,types,input
 *
 *	exports:
 *		io_initparser()
 *		io_doneparser()
 *		io_parsestatement()
 *
 *		io_makepath()
 *		io_freepath(path)
 *		io_appendpoint(path,pt)
 *		io_copypath(path,path)
 *		io_removepoint(path)
 *		io_pathlength(path)
 *		io_maketlist()
 *		io_freetlist(tlist)
 *		io_appendtentry(tlist,tentry)
 *		io_removetentry(tlist)
 *		io_tlistlength(tlist)
 *		io_dupetlist(src,dest)
 *
 *		io_fatalerrors()
 *		io_report("message", kind)
 *
 *		io_infromfile(file)
 *
 *	inter-module routines:
 *		init/done types
 *		init/done error
 *		io_identify(), io_getch(), io_peek()
 *		ttyputmsg()
 *
 *	imports:
 *		io_idefstart(...)
 *		io_idefend()
 *		io_ideletedef(nsym)
 *		io_ilayer(...)
 *		io_iflash(...)
 *		io_ipolygon(...)
 *		io_ibox(...)
 *		io_icall(...)
 *		io_icomment(...)
 *		io_iusercommand(...)
 *		io_iend()
 *
 *		io_isymname(name)
 *		io_iinstname(name)
 *		io_igeoname(...)
 *		io_ilabel(...)
 *
 */

/********************* input.c: input module *********************/

#define LINELEN 200							/* max chars to save in line buffer */

static FILE   *io_cifinfile;				/* input file */
static INTBIG  io_ciffilelength;			/* file length */
static INTBIG  io_cifcharsread;				/* characters read from file */
static INTBIG  io_cifnextchar;				/* lookahead character */
static INTBIG  io_ciflinecount;				/* line count */
static CHAR    io_ciflinebuffer[LINELEN];	/* identify line buffer */
static CHAR   *io_ciflineptr;				/* where we are in line buffer */
static INTBIG  io_cifcharcount;				/* number of chars in buffer */
static CHAR    io_cifresetbuffer;			/* flag to reset buffer */
static void   *io_cifprogressdialog;		/* for showing input progress */

static CHAR io_initinput(void)
{
	io_ciflinecount = 1;
	io_cifcharcount = 0;
	io_cifresetbuffer = 1;
	return(0);
}

static CHAR io_doneinput(void)
{
	xclose(io_cifinfile);
	if (io_verbose < 0 && io_ciffilelength > 0) DiaDoneProgress(io_cifprogressdialog);
	return(0);
}

BOOLEAN io_infromfile(FILE *file)
{
	io_cifinfile = file;
	if (io_verbose < 0)
	{
		io_ciffilelength = filesize(io_cifinfile);
		if (io_ciffilelength > 0)
		{
			io_cifprogressdialog = DiaInitProgress(0, 0);
			if (io_cifprogressdialog == 0) return(TRUE);
			DiaSetProgress(io_cifprogressdialog, 0, io_ciffilelength);
		}
	} else io_ciffilelength = 0;
	io_cifnextchar = xgetc(io_cifinfile);
	io_cifcharsread = 1;
	return(FALSE);
}

static CHAR io_getch(void)
{
	REGISTER INTBIG c;

	if (io_cifresetbuffer)
	{
		io_cifresetbuffer = 0;
		io_ciflineptr = &io_ciflinebuffer[0];
		io_cifcharcount = 0;
	}

	if ((c = io_cifnextchar) != EOF)
	{
		if (c == '\n') io_ciflinecount++;
		if (io_cifcharcount < LINELEN-1)
		{
			if (c != '\n')
			{
				io_cifcharcount++; *io_ciflineptr++ = (CHAR)c; *io_ciflineptr = '\0';
			} else io_cifresetbuffer = 1;
		}
		io_cifnextchar = xgetc(io_cifinfile);
		io_cifcharsread++;
		if (io_verbose < 0 && io_ciffilelength > 0 && (io_cifcharsread%2000) == 0)
			DiaSetProgress(io_cifprogressdialog, io_cifcharsread, io_ciffilelength);
	}
	return((CHAR)c);
}

static CHAR io_peek(void)
{
	return((CHAR)io_cifnextchar);
}

CHAR io_endoffile(void)
{
	return(io_cifnextchar == EOF);
}

static INTBIG io_flush(CHAR breakchar)
{
	REGISTER INTBIG c;

	while ((c = io_peek()) != EOF && c != breakchar) (void) io_getch();
	return(c);
}

static void io_identify(void)
{
	CHAR temp[200];

	if (io_cifcharcount > 0)
	{
		(void)esnprintf(temp, 200, x_("line %ld: %s"), io_ciflinecount-io_cifresetbuffer, io_ciflinebuffer);
		ttyputmsg(temp);
	}
}

/********************* types.c: data types *********************/

static void io_freeciftfree(void)
{
	linkedtentry *ct;

	while (io_ciftfree != 0)
	{
		ct = io_ciftfree;
		io_ciftfree = io_ciftfree->tnext;
		efree((CHAR *)ct);
	}
}

static CHAR io_inittypes(void)
{
	io_cifpfree = NIL;
	io_freeciftfree();
	return(0);
}

static CHAR io_donetypes(void)
{
	while (io_cifpfree != NIL)
	{
		linkedpoint *temp;

		temp = io_cifpfree->pnext;
		efree((CHAR *)io_cifpfree);
		io_cifpfree = temp;
	}
	while (io_ciftfree != NIL)
	{
		linkedtentry *temp;

		temp = io_ciftfree->tnext;
		efree((CHAR *)io_ciftfree);
		io_ciftfree = temp;
	}
	return(0);
}

CHAR *io_makepath(void)
{
	path a;

	a = (path)emalloc(sizeof(*a), io_tool->cluster);
	if (a == NIL)
	{
		io_report(_("free storage exhausted"), FATALINTERNAL);
		return(0);
	}
	a->pfirst = NIL;
	a->plast = NIL;
	a->plength = 0;
	return((CHAR *)a);
}

void io_freepath(path a)
{
	linkedpoint *node;

	node = a->pfirst;
	while (node != NIL)
	{
		linkedpoint *saved;
		saved = node->pnext;
		io_freepnode(node);
		node = saved;
	}
	efree((CHAR *)a);		/* should change to avoid overhead */
}

static linkedpoint *io_getpnode(void)
{
	linkedpoint *ans;

	if (io_cifpfree == NIL)
	{
		return((linkedpoint *)emalloc(sizeof(linkedpoint), io_tool->cluster));
	}
	ans = io_cifpfree;
	io_cifpfree = io_cifpfree->pnext;
	return(ans);
}

static void io_freepnode(linkedpoint *a)
{
	a->pnext = io_cifpfree;
	io_cifpfree = a;
}

/* returns nonzero on memory error */
BOOLEAN io_appendpoint(path a, point p)
{
	linkedpoint *temp;

	temp = a->plast;
	a->plast = io_getpnode();
	if (a->plast == 0) return(TRUE);
	if (temp != NIL) temp->pnext = a->plast;
	a->plast->pvalue = p;
	a->plast->pnext = NIL;
	if (a->pfirst == NIL) a->pfirst = a->plast;
	a->plength += 1;
	return(FALSE);
}

void io_copypath(path src, path dest)
{
	linkedpoint *temp;

	temp = src->pfirst;
	if (src == dest) return;
	while (temp != NIL)
	{
		if (io_appendpoint(dest, temp->pvalue)) break;
		temp = temp->pnext;
	}
}

point io_removepoint(path a)
{
	linkedpoint *temp;
	point ans;

	if (a->pfirst == NIL)
	{
		/* added code to initialize return value with dummy numbers */
		ans.x = ans.y = 0;
		return(ans);
	}
	temp = a->pfirst->pnext;
	ans = a->pfirst->pvalue;
	io_freepnode(a->pfirst);
	a->pfirst = temp;
	if (a->pfirst == NIL) a->plast = NIL;
	a->plength -= 1;
	return(ans);
}

INTBIG io_pathlength(path a)
{
	return(a->plength);
}

CHAR *io_maketlist(void)
{
	tlist a;

	a = (tlist)emalloc(sizeof(*a), io_tool->cluster);
	if (a == NIL)
	{
		io_report(_("free storage exhausted"), FATALINTERNAL);
		return(0);
	}
	a->tfirst = NIL;
	a->tlast = NIL;
	a->tlength = 0;
	return((CHAR *) a);
}

void io_freetlist(tlist a)
{
	linkedtentry *node;

	node = a->tfirst;
	while (node != NIL)
	{
		linkedtentry *saved;
		saved = node->tnext;
		io_freetnode(node);
		node = saved;
	}
	efree((CHAR *)a);		/* should change to avoid overhead */
}

void io_dupetlist(tlist src, tlist dest)
{
	linkedtentry *node;

	if (src == NIL || dest == NIL) return;
	node = src->tfirst;
	while (node != NIL)
	{
		io_appendtentry(dest, node->tvalue);
		node = node->tnext;
	}
}

static linkedtentry *io_gettnode(void)
{
	linkedtentry *ans;

	if (io_ciftfree == NIL)
	{
		ans = (linkedtentry *)emalloc(sizeof(linkedtentry), io_tool->cluster);
		if (ans == 0) return(0);
		return(ans);
	}
	ans = io_ciftfree;
	io_ciftfree = io_ciftfree->tnext;
	return(ans);
}

static void io_freetnode(linkedtentry *a)
{
	a->tnext = io_ciftfree;
	io_ciftfree = a;
}

void io_appendtentry(tlist a, tentry p)
{
	linkedtentry *temp, *newt;

	newt = io_gettnode();
	if (newt == 0) return;

	temp = a->tlast;
	a->tlast = newt;
	if (temp != NIL) temp->tnext = a->tlast;
	a->tlast->tvalue = p;
	a->tlast->tnext = NIL;
	if (a->tfirst == NIL) a->tfirst = a->tlast;
	a->tlength += 1;
}

tentry io_removetentry(tlist a)
{
	linkedtentry *temp;
	tentry ans;

	if (a->tfirst == NIL)
	{
		/* added extra code to initialize "ans" to a dummy value */
		ans.kind = TRANSLATE;
		ans.guts.tr.xt = ans.guts.tr.yt = 0;
		return(ans);
	}
	temp = a->tfirst->tnext;
	ans = a->tfirst->tvalue;
	io_freetnode(a->tfirst);
	a->tfirst = temp;
	if (a->tfirst == NIL) a->tlast = NIL;
	a->tlength -= 1;
	return(ans);
}

INTBIG io_tlistlength(tlist a)
{
	if (a == NIL) return(0);
	return(a->tlength);
}

/********************* error.c: error reporter *********************/

static INTBIG io_cifecounts[OTHER+1];

static CHAR io_initerror(void)
{
	INTBIG i;

	for (i = 0; i <= OTHER; i++) io_cifecounts[i] = 0;
	return(0);
}

static CHAR io_doneerror(void)
{
	return(0);
}

INTBIG io_fatalerrors(void)
{
	return(io_cifecounts[FATALINTERNAL]+io_cifecounts[FATALSYNTAX]+
		io_cifecounts[FATALSEMANTIC]+io_cifecounts[FATALOUTPUT]);
}

void io_report(CHAR *mess, INTBIG kind)
{
	io_identify();
	io_cifecounts[kind]++;

	switch (kind)
	{
		case FATALINTERNAL: ttyputmsg(_("Fatal internal error: %s"), mess);  break;
		case FATALSYNTAX:   ttyputmsg(_("Syntax error: %s"), mess);          break;
		case FATALSEMANTIC: ttyputmsg(_("Error: %s"), mess);                 break;
		case FATALOUTPUT:   ttyputmsg(_("Output error: %s"), mess);          break;
		case ADVISORY:      ttyputmsg(_("Warning: %s"), mess);               break;
		default:            ttyputmsg(x_("%s"), mess);                        break;
	}

	if (kind == FATALINTERNAL) longjmp(io_filerror, 1);
}


/********************* parser.c: parser *********************/

#define BIGSIGNED ((0X7FFFFFFF-9)/10)

#define ERRORCHECK if (io_ciferrorfound) goto recover

/*	specific syntax errors	*/

#ifdef NOERROR
#  undef NOERROR
#endif
#define NOERROR 100
#define NUMTOOBIG 101
#define NOUNSIGNED 102
#define NOSIGNED 103
#define NOSEMI 104
#define NOPATH 105
#define BADTRANS 106
#define BADUSER 107
#define BADCOMMAND 108
#define INTERNAL 109
#define BADDEF 110
#define NOLAYER 111
#define BADCOMMENT 112
#define BADAXIS 113
#define NESTDEF 114
#define NESTDD 115
#define NODEFSTART 116
#define NESTEND 117
#define NOSPACE 118
#define NONAME 119

static CHAR   io_ciferrorfound;			/* flag for error encountered */
static INTBIG io_ciferrortype;			/* what it was */
static CHAR   io_cifendisseen;			/* end command flag */

INTBIG io_initparser(void)
{
	io_ciferrorfound = 0;
	io_ciferrortype = NOERROR;
	io_cifdefinprog = 0;
	io_cifendisseen = 0;
	return(io_initinput() || io_inittypes() || io_initerror());
}

INTBIG io_doneparser(void)
{
	if (!io_cifendisseen) io_report(_("missing End command"), FATALSYNTAX);
	return(io_doneinput() || io_donetypes() || io_doneerror());
}

static void io_blank(void)
{
	REGISTER CHAR c;

	for(;;)
	{
		switch (c = io_peek())
		{
			case '(':
			case ')':
			case ';':
			case '-':
			case EOF:
				return;
			default:
				if (isdigit(c) || isupper(c)) return;
					else (void) io_getch();
		}
	}
}

static void io_sep(void)
{
	REGISTER CHAR c;

	for(;;)
	{
		switch (c = io_peek())
		{
			case '(':
			case ')':
			case ';':
			case '-':
			case EOF:
				return;
			default:
				if (isdigit(c)) return;
					else (void) io_getch();
		}
	}
}

static CHAR io_semi(void)
{
	CHAR ans = 0;

	io_blank();
	if (io_peek() == ';') {(void) io_getch(); ans = 1; io_blank();}
	return(ans);
}

static CHAR io_sp(void)
{
	CHAR ans = 0; CHAR c;

	while ((c = io_peek()) == ' ' || c == '\t')
	{
		(void) io_getch();
		ans = 1;
	}
	return(ans);
}

static CHAR *io_utext(void)
{
	CHAR *user, *olduser;
	INTBIG maxsize = 80;
	INTBIG size = 0;

	if ((user = (CHAR *)emalloc(maxsize * SIZEOFCHAR, io_tool->cluster)) == NIL)
	{
		io_report(_("free storage exhausted"), FATALINTERNAL);
		return((CHAR *)0);
	}
	*user = '\0';
	while (!io_endoffile() && io_peek() != ';')
	{
		*(user+size++) = io_getch();
		if (size >= maxsize)
		{
			INTBIG i;
			maxsize += 80;
			olduser = user;
			user = (CHAR *)emalloc(maxsize * SIZEOFCHAR, io_tool->cluster);
			if (user == NIL)
				io_report(_("free storage exhausted"), FATALINTERNAL);
			for(i=0; i<maxsize-80; i++) user[i] = olduser[i];
			efree(olduser);
		}
		*(user+size) = '\0';
	}
	return(user);
}

static CHAR *io_name(void)
{
	CHAR *ntext, *oldntext;
	INTBIG maxsize = 80;
	INTBIG size = 0;
	CHAR c; CHAR nochar = 1;

	if ((ntext = (CHAR *)emalloc(maxsize * SIZEOFCHAR, io_tool->cluster)) == NIL)
	{
		io_report(_("free storage exhausted"), FATALINTERNAL);
		return((CHAR *) 0);
	}
	*ntext = '\0';
	while (!io_endoffile() && (c = io_peek()) != ';' &&
/*		(c < 'a' || c > 'z') &&       ...uncomment to disallow lowercase */
		c != ' ' && c != '\t' && c != '{' && c != '}')
	{
		nochar = 0;
/*		if (c >= 'a' && c <= 'z') c = (c-'a')+'A';      ...uncomment to make all names upper case */
		(void) io_getch();
		*(ntext+size++) = c;
		if (size >= maxsize)
		{
			INTBIG i;
			maxsize += 80;
			oldntext = ntext;
			if ((ntext = (CHAR *)emalloc(maxsize * SIZEOFCHAR, io_tool->cluster)) == NIL)
				io_report(_("free storage exhausted"), FATALINTERNAL);
			for(i=0; i<maxsize-80; i++) ntext[i] = oldntext[i];
			efree(oldntext);
		}
		*(ntext+size) = '\0';
	}
	if (nochar) io_logit(NONAME);
	return(ntext);
}

static INTBIG io_cardinal(void)
{
	CHAR somedigit = 0;
	REGISTER INTBIG ans = 0;
	INTBIG c;

	(void)io_sp();

	while (ans < BIGSIGNED && (c = io_peek()) >= '0' && c <= '9')
	{
		ans *= 10; ans += io_getch() - '0';
		somedigit = 1;
	}

	if (!somedigit)
	{
		io_logit(NOUNSIGNED);
		return(0);
	}
	if ((c = io_peek()) >= '0' && c <= '9')
	{
		io_logit(NUMTOOBIG);
		return(0XFFFFFFFF);
	}
	return(ans);
}

static INTBIG io_signed(void)
{
	CHAR somedigit = 0;
	CHAR sign = 0;
	REGISTER INTBIG ans = 0;
	CHAR c;

	io_sep();

	if (io_peek() == '-') {sign = 1; (void) io_getch();}

	while (ans < BIGSIGNED && (c = io_peek()) >= '0' && c <= '9')
	{
		ans *= 10; ans += io_getch() - '0';
		somedigit = 1;
	}

	if (!somedigit) {io_logit(NOSIGNED); return(0);}
	if ((c = io_peek()) >= '0' && c <= '9')
	{
		io_logit(NUMTOOBIG);
		return(sign ? -0X7FFFFFFF:0X7FFFFFFF);
	}
	return(sign ? -ans:ans);
}

static void io_logit(INTBIG thing)
{
	io_ciferrorfound = 1;
	io_ciferrortype = thing;
}

static point io_getpoint(void)
{
	point ans;

	ans.x = io_signed();
	ans.y = io_signed();
	return(ans);
}

static void io_getpath(path a)
{
	CHAR c;
	point temp;

	io_sep();

	while (((c = io_peek()) >= '0' && c <= '9') || c == '-')
	{
		temp = io_getpoint();	/* hack because of compiler error */
		if (io_appendpoint(a, temp)) break;
		io_sep();
	}
	if (io_pathlength(a) == 0) io_logit(NOPATH);
}

INTBIG io_parsestatement(void)
{
	CHAR curchar;
	INTBIG command = NULLCOMMAND;
	CHAR *curpath = NIL;
	CHAR *curtlist = NIL;
	INTBIG xrotate, yrotate;
	point center;
	INTBIG length, width;
	INTBIG diameter;
	CHAR lname[5], *oldcomment;
	INTBIG symbolnumber, multiplier, divisor;
	CHAR *comment = NIL;
	CHAR *usertext = NIL;
	CHAR *nametext = NIL;
	INTBIG usercommand;
	point namepoint;

	io_blank();		/* flush initial junk */

	switch (curchar = io_getch())
	{
		case 'P':
			command = POLYCOM;
			curpath = io_makepath();
			io_getpath((path)curpath); ERRORCHECK;
			break;

		case 'B':
			command = BOXCOM;
			xrotate = 1; yrotate = 0;
			length = io_cardinal(); ERRORCHECK;
			width = io_cardinal(); ERRORCHECK;
			center = io_getpoint(); ERRORCHECK; io_sep();
			if (((curchar = io_peek()) >= '0' && curchar <= '9') || curchar == '-')
			{
				xrotate = io_signed(); ERRORCHECK;
				yrotate = io_signed(); ERRORCHECK;
			}
			break;

		case 'R':
			command = FLASHCOM;
			diameter = io_cardinal(); ERRORCHECK;
			center = io_getpoint(); ERRORCHECK;
			break;

		case 'W':
			command = WIRECOM;
			width = io_cardinal(); ERRORCHECK;
			curpath = io_makepath();
			io_getpath((path)curpath); ERRORCHECK;
			break;

		case 'L':
			{
				INTBIG i;
				command = LAYER;
				io_blank();
				for (i = 0; i<4; i++)
					if (((curchar = io_peek()) >= 'A' && curchar <= 'Z') || isdigit(curchar))
						lname[i] = io_getch();
							else break;
				lname[i] = '\0';
				if (i == 0) {io_ciferrorfound = 1; io_ciferrortype = NOLAYER; goto recover;}
			}
			break;

		case 'D':
			io_blank();
			switch (io_getch())
			{
				case 'S':
					command = DEFSTART;
					symbolnumber = io_cardinal(); ERRORCHECK;
					io_sep(); multiplier = divisor = 1;
					if (isdigit(io_peek()))
					{
						multiplier = io_cardinal(); ERRORCHECK;
						divisor = io_cardinal(); ERRORCHECK;
					}
					if (io_cifdefinprog)
					{
						io_ciferrorfound = 1;
						io_ciferrortype = NESTDEF;
						goto recover;
					}
					io_cifdefinprog = 1;
					break;
				case 'F':
					command = DEFEND;
					if (!io_cifdefinprog)
					{
						io_ciferrorfound = 1;
						io_ciferrortype = NODEFSTART;
						goto recover;
					}
					io_cifdefinprog = 0;
					break;
				case 'D':
					command = DELETEDEF;
					symbolnumber = io_cardinal(); ERRORCHECK;
					if (io_cifdefinprog)
					{
						io_ciferrorfound = 1;
						io_ciferrortype = NESTDD;
						goto recover;
					}
					break;
				default:
					io_ciferrorfound = 1;
					io_ciferrortype = BADDEF;
					goto recover;
			}
			break;

		case 'C':
			command = CALLCOM;
			symbolnumber = io_cardinal(); ERRORCHECK;
			io_blank();
			curtlist = io_maketlist();
			for(;;)
			{
				switch (io_peek())
				{
					tentry trans;

					case 'T':
						(void) io_getch();
						trans.kind = TRANSLATE;
						trans.guts.tr.xt = io_signed(); ERRORCHECK;
						trans.guts.tr.yt = io_signed(); ERRORCHECK;
						io_appendtentry((tlist)curtlist, trans);
						break;

					case 'M':
						trans.kind = MIRROR;
						(void) io_getch(); io_blank();
						switch (io_getch())
						{
							case 'X': trans.guts.mi.xcoord = 1; break;
							case 'Y': trans.guts.mi.xcoord = 0; break;
							default:  io_ciferrorfound = 1; io_ciferrortype = BADAXIS; goto recover;
						}
						io_appendtentry((tlist)curtlist, trans);
						break;

					case 'R':
						trans.kind = ROTATE;
						(void) io_getch();
						trans.guts.ro.xrot = io_signed(); ERRORCHECK;
						trans.guts.ro.yrot = io_signed(); ERRORCHECK;
						io_appendtentry((tlist)curtlist, trans);
						break;

					case ';':
						goto localexit;

					default:
						io_ciferrorfound = 1; io_ciferrortype = BADTRANS; goto recover;
				}
				io_blank();		/* between transformation commands */
			}	/* end of while (1) loop */
			localexit:
			break;

		case '(':
			{
				INTBIG level = 1;
				INTBIG maxsize = 80;
				INTBIG size = 0;

				command = COMMENT;
				if ((comment = (CHAR *)emalloc(maxsize * SIZEOFCHAR, io_tool->cluster)) == NIL)
				{
					io_report(_("free storage exhausted"), FATALINTERNAL);
					break;
				}

				*comment = '\0';
				while (level)
				{
					switch (curchar = io_getch())
					{
						case '(':
							level++;
							*(comment+size++) = '(';
							break;
						case ')':
							level--;
							if (level) *(comment+size++) = ')';
							break;
						case EOF:
							io_ciferrorfound = 1; io_ciferrortype = BADCOMMENT; goto recover;
						default:
							*(comment+size++) = curchar;
					}
					if (size >= maxsize)
					{
						INTBIG i;

						maxsize += 80;
						oldcomment = comment;
						comment = (CHAR *)emalloc(maxsize * SIZEOFCHAR, io_tool->cluster);
						if (comment == NIL)
						{
							io_report(_("free storage exhausted"), FATALINTERNAL);
							break;
						}
						for(i=0; i<maxsize-80; i++) comment[i] = oldcomment[i];
						efree(oldcomment);
					}
					*(comment+size) = '\0';
				}
			}
			break;

		case 'E':
			command = END;
			io_blank();
			if (io_cifdefinprog)
			{
				io_ciferrorfound = 1;
				io_ciferrortype = NESTEND;
				goto recover;
			}
			if (!io_endoffile()) io_report(_("more text follows end command"), ADVISORY);
			io_cifendisseen = 1;
			io_iend();
			goto alldone;

		case ';':
			command = NULLCOMMAND;
			goto alldone;

		case EOF:
			command = ENDFILE;
			goto alldone;

		default:
			if (isdigit(curchar))
			{
				if ((usercommand = curchar - '0') == 9 &&
					((curchar = io_peek()) == ' ' || curchar == '\t' ||
						curchar == '1' || curchar == '2' || curchar == '3'))
							switch(io_getch())
				{
					case ' ':
					case '\t':
						(void) io_sp(); nametext = io_name(); ERRORCHECK;
						command = SYMNAME;
						break;
					case '1':
					case '2':
					case '3':
						if (!io_sp())
						{
							io_ciferrorfound = 1; io_ciferrortype = NOSPACE;
							goto recover;
						}
						nametext = io_name(); ERRORCHECK;
						switch(curchar)
						{
							case '1':
								command = INSTNAME;
								break;
							case '2':
							{
								INTBIG i;

								command = GEONAME;
								namepoint = io_getpoint(); ERRORCHECK;
								io_blank();
								for (i = 0; i<4; i++)
									if (((curchar = io_peek()) >= 'A' &&
										curchar <= 'Z') || isdigit(curchar))
											lname[i] = io_getch();
												else break;
								lname[i] = '\0';
								break;
							}
							case '3':
								command = LABELCOM;
								namepoint = io_getpoint(); ERRORCHECK;
								break;
						}
						break;
				} else
				{
					command = USERS;
					usertext = io_utext();
					if (io_endoffile())
					{
						io_ciferrorfound = 1; io_ciferrortype = BADUSER; goto recover;
					}
				}
			} else
			{
				io_ciferrorfound = 1;
				io_ciferrortype = BADCOMMAND;
				goto recover;
			}
	}

	/*
	 * by now we have a syntactically valid command
	 * although it might be missing a semi-colon
	 */
	switch(command)
	{
		case WIRECOM:
			io_iwire(width, curpath);
			break;
		case DEFSTART:
			io_idefstart(symbolnumber, multiplier, divisor);
			break;
		case DEFEND:
			io_idefend();
			break;
		case DELETEDEF:
			io_ideletedef(symbolnumber);
			break;
		case CALLCOM:
			io_icall(symbolnumber, curtlist);
			break;
		case LAYER:
			io_ilayer(lname);
			break;
		case FLASHCOM:
			io_iflash(diameter, center);
			break;
		case POLYCOM:
			io_ipolygon(curpath);
			break;
		case BOXCOM:
			io_ibox(length, width, center, xrotate, yrotate);
			break;
		case COMMENT:
			io_icomment(comment);
			break;
		case USERS:
			io_iusercommand(usercommand, usertext);
			break;
		case SYMNAME:
			io_isymname(nametext);
			break;
		case INSTNAME:
			io_iinstname(nametext);
			break;
		case GEONAME:
			io_igeoname(nametext, namepoint, (INTBIG)lname);
			break;
		case LABELCOM:
			io_ilabel(nametext, namepoint);
			break;
		default:
			io_ciferrorfound = 1;
			io_ciferrortype = INTERNAL;
			goto recover;
	}
	if (!io_semi()) {io_ciferrorfound = 1; io_ciferrortype = NOSEMI; goto recover;}

alldone:		/* everyone exits here */
	if (curpath != NIL) io_freepath((path)curpath);
	if (curtlist != NIL) io_freetlist((tlist)curtlist);
	if (comment != NIL) efree((CHAR *) comment);
	if (usertext != NIL) efree((CHAR *) usertext);
	if (nametext != NIL) efree((CHAR *) nametext);
	return(command);

recover:		/* errors come here */
	switch (io_ciferrortype)
	{
		case NUMTOOBIG:  io_report(_("number too large"), FATALSYNTAX); break;
		case NOUNSIGNED: io_report(_("unsigned integer expected"), FATALSYNTAX); break;
		case NOSIGNED:   io_report(_("signed integer expected"), FATALSYNTAX); break;
		case NOSEMI:     io_report(_("missing ';' inserted"), FATALSYNTAX); break;
		case NOPATH:     io_report(_("no points in path"), FATALSYNTAX); break;
		case BADTRANS:   io_report(_("no such transformation command"), FATALSYNTAX); break;
		case BADUSER:    io_report(_("end of file inside user command"), FATALSYNTAX); break;
		case BADCOMMAND: io_report(_("unknown command encountered"), FATALSYNTAX); break;
		case INTERNAL:   io_report(_("parser can't find i routine"), FATALINTERNAL); break;
		case BADDEF:     io_report(_("no such define command"), FATALSYNTAX); break;
		case NOLAYER:    io_report(_("layer name expected"), FATALSYNTAX); break;
		case BADCOMMENT: io_report(_("end of file inside a comment"), FATALSYNTAX); break;
		case BADAXIS:    io_report(_("no such axis in mirror command"), FATALSYNTAX); break;
		case NESTDEF:    io_report(_("symbol definitions can't nest"), FATALSYNTAX); break;
		case NODEFSTART: io_report(_("DF without DS"), FATALSYNTAX); break;
		case NESTDD:     io_report(_("DD can't appear inside symbol definition"), FATALSYNTAX); break;
		case NOSPACE:    io_report(_("missing space in name command"), FATALSYNTAX); break;
		case NONAME:     io_report(_("no name in name command"), FATALSYNTAX); break;
		case NESTEND:    io_report(_("End command inside symbol definition"), FATALSYNTAX); break;
		case NOERROR:    io_report(_("error signaled but not reported"), FATALINTERNAL); break;
		default:         io_report(_("uncaught error"), FATALSYNTAX);
	}
	if (io_ciferrortype != INTERNAL && io_ciferrortype != NOSEMI && io_flush(';') == EOF)
		io_report(_("unexpected end of input file"), FATALSYNTAX);
			else io_blank();
	command = SYNTAXERROR;
	io_ciferrorfound = 0;
	io_ciferrortype = NOERROR;
	goto alldone;
}

INTBIG io_parsefile(void)
{
	INTBIG com;
	INTBIG comcount = 1;

	for(;;)
	{
		if (stopping(STOPREASONCIF)) break;
		com = io_parsestatement();
		if (com == END || com == ENDFILE) break;
		comcount++;
	}
	return(comcount);
}
