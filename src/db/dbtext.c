/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbtext.c
 * Database text and file support module
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
#include "egraphics.h"
#include "edialogs.h"
#include "network.h"
#include "usr.h"
#include "tecgen.h"
#ifdef WIN32
#  include <io.h>
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#define PUREUNITGIGA    0		/* giga:  x 1000000000 */
#define PUREUNITMEGA    1		/* mega:  x 1000000 */
#define PUREUNITKILO    2		/* kilo:  x 1000 */
#define PUREUNITNONE    3		/* -:     x 1 */
#define PUREUNITMILLI   4		/* milli: / 1000 */
#define PUREUNITMICRO   5		/* micro: / 1000000 */
#define PUREUNITNANO    6		/* nano:  / 1000000000 */
#define PUREUNITPICO    7		/* pico:  / 1000000000000 */
#define PUREUNITFEMTO   8		/* femto: / 1000000000000000 */

#define NUMDESCRIPTIONBUFS 4

static CHAR  *db_keywordbuffer;
static INTBIG db_keywordbufferlength = 0;

/* working memory for "db_tryfile()" */
static CHAR  *db_tryfilename;
static INTBIG db_tryfilenamelen = 0;

/* working memory for "describenodeproto()" */
static INTBIG db_descnpnodeswitch = 0;
static CHAR  *db_descnpoutput[NUMDESCRIPTIONBUFS];
static INTBIG db_descnpoutputsize[NUMDESCRIPTIONBUFS] = {0, 0};

/* working memory for "describenodeinst()" */
static INTBIG db_descninodeswitch = 0;
static CHAR  *db_descnioutput[NUMDESCRIPTIONBUFS];
static INTBIG db_descnioutputsize[NUMDESCRIPTIONBUFS] = {0, 0};

/* working memory for "describearcinst()" */
static INTBIG db_descaiarcswitch = 0;
static CHAR  *db_descaioutput[NUMDESCRIPTIONBUFS];
static INTBIG db_descaioutputsize[NUMDESCRIPTIONBUFS] = {0, 0};

/* working memory for "describearcproto()" */
static INTBIG db_descaparcswitch = 0;
static CHAR  *db_descapoutput[NUMDESCRIPTIONBUFS];
static INTBIG db_descapoutputsize[NUMDESCRIPTIONBUFS] = {0, 0};

/* working memory for "makeplural()" */
static INTBIG db_pluralbufsize = 0;
static CHAR  *db_pluralbuffer;

/* working memory for "getcomplexnetworks()" */
static LISTINTBIG *db_networklist = 0;

/* working memory for types of files */
typedef struct
{
	CHAR   *extension;
	CHAR   *winfilter;
	INTBIG  mactype;
	BOOLEAN binary;
	CHAR   *shortname;
	CHAR   *longname;
} FILETYPES;
static INTBIG     db_filetypecount = 0;
static FILETYPES *db_filetypeinfo;

/* the infinite string package */
#define INFSTRCOUNT     8				/* number of infinite strings */
#define INFSTRDEFAULT 200				/* default infinite string length */
#define NOINFSTR ((INFSTR *)-1)
typedef struct Iinfstr
{
	CHAR  *infstr;						/* the string address */
	INTBIG infstrlength;				/* the length of the string */
	INTBIG infstrptr;					/* the location of the string end */
	struct Iinfstr *nextinfstr;			/* next in linked list */
} INFSTR;
static BOOLEAN db_firstinf = FALSE;		/* initialization flag */
static INFSTR *db_firstfreeinfstr;		/* first free infinite string block */
static INFSTR *db_lastfreeinfstr;		/* last free infinite string block */
static void   *db_infstrmutex = 0;		/* mutex for infinite string modules */

/* for case-insensitive string compare ("namesame()" and "namesamen()") */
static BOOLEAN db_namesamefirst = TRUE;
static CHAR    db_namesametable[256];

/* prototypes for local routines */
static BOOLEAN db_beginsearch(CHAR**);
static void    db_makestringvar(INTBIG, INTBIG, INTBIG, INTBIG, void*);
static void    db_addstring(CHAR*, void*);
static FILE   *db_tryfile(CHAR*, CHAR*, CHAR*, CHAR*, CHAR**);
static void    db_shuffle(CHAR*, CHAR*);
static void    db_initnamesame(void);
static CHAR   *db_makeunits(float value, INTBIG units);
static INTBIG  db_getpureunit(INTBIG unittype, INTBIG unitscale);

/*
 * Routine to free all memory associated with this module.
 */
void db_freetextmemory(void)
{
	REGISTER INTBIG i;
	REGISTER INFSTR *inf;

	if (db_pluralbufsize > 0) efree(db_pluralbuffer);
	if (db_keywordbufferlength != 0) efree(db_keywordbuffer);
	if (db_tryfilenamelen != 0) efree(db_tryfilename);
	if (db_networklist != 0) killintlistobj(db_networklist);

	while (db_firstfreeinfstr != NOINFSTR)
	{
		inf = db_firstfreeinfstr;
		db_firstfreeinfstr = inf->nextinfstr;
		efree((CHAR *)inf->infstr);
		efree((CHAR *)inf);
	}

	for(i=0; i<NUMDESCRIPTIONBUFS; i++)
		if (db_descnpoutputsize[i] != 0) efree(db_descnpoutput[i]);
	for(i=0; i<NUMDESCRIPTIONBUFS; i++)
		if (db_descnioutputsize[i] != 0) efree(db_descnioutput[i]);
	for(i=0; i<NUMDESCRIPTIONBUFS; i++)
		if (db_descaioutputsize[i] != 0) efree(db_descaioutput[i]);
	for(i=0; i<NUMDESCRIPTIONBUFS; i++)
		if (db_descapoutputsize[i] != 0) efree(db_descapoutput[i]);

	/* free file type memory */
	for(i=0; i<db_filetypecount; i++)
	{
		efree((CHAR *)db_filetypeinfo[i].extension);
		efree((CHAR *)db_filetypeinfo[i].winfilter);
		efree((CHAR *)db_filetypeinfo[i].shortname);
		efree((CHAR *)db_filetypeinfo[i].longname);
	}
	if (db_filetypecount > 0) efree((CHAR *)db_filetypeinfo);
}

/************************* STRING PARSING *************************/

/*
 * routine to parse a lambda value of the form "nn.dd[u | " | cm | mil | mm | cu | nm]"
 * where the unlabeled number defaults to the current DISPLAYUNITS of the
 * current technology but trailing letters can override.  The input is in
 * the string "pp".
 */
INTBIG atola(CHAR *pp, INTBIG lambda)
{
	REGISTER INTBIG hipart, lonum, loden, retval, units;
	REGISTER INTBIG neg;
	REGISTER CHAR *ptr;
	double scale;

	/* determine default scale amount */
	ptr = pp;

	if (*ptr == '-') { neg = -1;   ptr++; } else neg = 1;
	hipart = eatoi(ptr);
	while (isdigit(*ptr)) ptr++;
	lonum = 0;   loden = 1;
	if (*ptr == '.')
	{
		ptr++;
		while (isdigit(*ptr)) { lonum = lonum * 10 + (*ptr++ - '0'); loden *= 10; }
	}

	/* determine units */
	units = el_units;
	if (ptr[0] == '"') units = (units & ~DISPLAYUNITS) | DISPUNITINCH; else
	if (ptr[0] == 'c' && ptr[1] == 'm') units = (units & ~DISPLAYUNITS) | DISPUNITCM; else
	if (ptr[0] == 'm' && ptr[1] == 'm') units = (units & ~DISPLAYUNITS) | DISPUNITMM;
	if (ptr[0] == 'm' && ptr[1] == 'i' && ptr[2] == 'l') units = (units & ~DISPLAYUNITS) | DISPUNITMIL;
	if (ptr[0] == 'u') units = (units & ~DISPLAYUNITS) | DISPUNITMIC; else
	if (ptr[0] == 'c' && ptr[1] == 'u') units = (units & ~DISPLAYUNITS) | DISPUNITCMIC; else
	if (ptr[0] == 'n' && ptr[1] == 'm') units = (units & ~DISPLAYUNITS) | DISPUNITNM;

	/* convert to database units */
	if (lambda == 0) lambda = el_curlib->lambda[el_curtech->techindex];
	scale = db_getcurrentscale(el_units&INTERNALUNITS, units, lambda);
	retval = rounddouble(((double)hipart) * scale + ((double)lonum)*scale / ((double)loden));
	return(retval*neg);
}

/*
 * routine to parse a fixed point value of the form "n.d" where
 * "d" is evaluated to the nearest 120th (the value of WHOLE).
 * The number is returned scaled by a factor of WHOLE.  The input is in
 * the string "pp".
 */
INTBIG atofr(CHAR *pp)
{
	REGISTER INTBIG i, j, k;
	REGISTER INTBIG n;

	if (*pp == '-') { n = -1;   pp++; } else n = 1;
	i = eatoi(pp) * WHOLE;
	while (isdigit(*pp)) pp++;
	if (*pp++ != '.') return(i*n);
	j = 0;   k = 1;
	while (isdigit(*pp)) { j = j * 10 + (*pp++ - '0'); k *= 10; }
	i += (j*WHOLE + k/2)/k;
	return(i*n);
}

/* routine to convert ascii to integer */
INTBIG myatoi(CHAR *pp)
{
	REGISTER INTBIG num;
	REGISTER INTBIG base, sign;

	base = 10;
	num = 0;
	sign = 1;
	if (*pp == '-')
	{
		pp++;
		sign = -1;
	}
	if (*pp == '0')
	{
		pp++;
		base = 8;
		if (*pp == 'x')
		{
			pp++;
			base = 16;
		}
	}
	for(;;)
	{
		if ((*pp >= 'a' && *pp <= 'f') || (*pp >= 'A' && *pp <= 'F'))
		{
			if (base != 16) break;
			num = num * 16;
			if (*pp >= 'a' && *pp <= 'f') num += *pp++ - 'a' + 10; else
				num += *pp++ - 'A' + 10;
			continue;
		} else if (isdigit(*pp))
		{
			if (*pp >= '8' && base == 8) break;
			num = num * base + *pp++ - '0';
			continue;
		}
		break;
	}
	return(num * sign);
}

/*
 * Routine to convert a HUGE integer (64 bits) to a string.
 * This routine does the work by hand, but it can be done with
 * special "printf" format conversions, of which these are known:
 *    Windows: "%I64d"
 *    Sun:     "%PRIx64"
 *    Linux:   "%lld"
 */
CHAR *hugeinttoa(INTHUGE a)
{
	static CHAR ret[NUMDESCRIPTIONBUFS][40];
	static INTBIG which = 0;
	REGISTER CHAR *curbuf, digit;
	REGISTER INTBIG i, neg;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	curbuf = ret[which++];
	if (which >= NUMDESCRIPTIONBUFS) which = 0;

	if (a >= 0) neg = 0; else
	{
		neg = 1;
		a = -a;
	}

	curbuf[i=39] = 0;
	while (i > 0)
	{
		digit = (CHAR)(a % 10);
		curbuf[--i] = '0' + digit;
		a /= 10;
		if (a == 0) break;
	}
	if (neg != 0)
		curbuf[--i] = '-';
	return(&curbuf[i]);
}

CHAR *explainduration(float duration)
{
	static CHAR elapsed[200];
	CHAR temp[50];
	INTBIG hours, minutes;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	elapsed[0] = 0;
	if (duration >= 3600.0)
	{
		hours = (INTBIG)(duration / 3600.0f);
		duration -= (float)(hours * 3600);
		esnprintf(temp, 50, x_("%ld hours, "), hours);
		estrcat(elapsed, temp);
	}
	if (duration >= 60.0)
	{
		minutes = (INTBIG)(duration / 60.0f);
		duration -= (float)(minutes * 60);
		esnprintf(temp, 50, x_("%ld minutes, "), minutes);
		estrcat(elapsed, temp);
	}
	esnprintf(temp, 50, x_("%g seconds"), duration);
	estrcat(elapsed, temp);
	return(elapsed);
}

/*
 * Routine to get the pure unit to use for electrical unit type "unittype", scale
 * "unitscale".
 */
INTBIG db_getpureunit(INTBIG unittype, INTBIG unitscale)
{
	switch (unittype)
	{
		case VTUNITSRES:		/* resistance */
			switch (unitscale)
			{
				case INTRESUNITKOHM: return(PUREUNITKILO);
				case INTRESUNITMOHM: return(PUREUNITMEGA);
				case INTRESUNITGOHM: return(PUREUNITGIGA);
			}
			break;

		case VTUNITSCAP:		/* capacitance */
			switch (unitscale)
			{
				case INTCAPUNITMFARAD: return(PUREUNITMILLI);
				case INTCAPUNITUFARAD: return(PUREUNITMICRO);
				case INTCAPUNITNFARAD: return(PUREUNITNANO);
				case INTCAPUNITPFARAD: return(PUREUNITPICO);
				case INTCAPUNITFFARAD: return(PUREUNITFEMTO);
			}
			break;

		case VTUNITSIND:		/* inductance */
			switch (unitscale)
			{
				case INTINDUNITMHENRY: return(PUREUNITMILLI);
				case INTINDUNITUHENRY: return(PUREUNITMICRO);
				case INTINDUNITNHENRY: return(PUREUNITNANO);
			}
			break;

		case VTUNITSCUR:		/* current */
			switch (unitscale)
			{
				case INTCURUNITMAMP: return(PUREUNITMILLI);
				case INTCURUNITUAMP: return(PUREUNITMICRO);
			}
			break;

		case VTUNITSVOLT:		/* voltage */
			switch (unitscale)
			{
				case INTVOLTUNITKVOLT: return(PUREUNITKILO);
				case INTVOLTUNITMVOLT: return(PUREUNITMILLI);
				case INTVOLTUNITUVOLT: return(PUREUNITMICRO);
			}
			break;

		case VTUNITSTIME:		/* time */
			switch (unitscale)
			{
				case INTTIMEUNITMSEC: return(PUREUNITMILLI);
				case INTTIMEUNITUSEC: return(PUREUNITMICRO);
				case INTTIMEUNITNSEC: return(PUREUNITNANO);
				case INTTIMEUNITPSEC: return(PUREUNITPICO);
				case INTTIMEUNITFSEC: return(PUREUNITFEMTO);
			}
			break;
	}
	return(PUREUNITNONE);
}

/*
 * Routine to express "value" as a string in "unittype" electrical units.
 * The scale of the units is in "unitscale".
 */
CHAR *displayedunits(float value, INTBIG unittype, INTBIG unitscale)
{
	static CHAR line[100];
	REGISTER INTBIG pureunit;
	CHAR *postfix;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	pureunit = db_getpureunit(unittype, unitscale);
	switch (pureunit)
	{
		case PUREUNITGIGA:		/* giga:  x 1000000000 */
			value /= 1000000000.0f;
			postfix = x_("g");
			break;
		case PUREUNITMEGA:		/* mega:  x 1000000 */
			value /= 1000000.0f;
			postfix = x_("meg");		/* SPICE wants "x" */
			break;
		case PUREUNITKILO:		/* kilo:  x 1000 */
			value /= 1000.0f;
			postfix = x_("k");
			break;
		case PUREUNITNONE:		/* -:     x 1 */
			postfix = x_("");
			break;
		case PUREUNITMILLI:		/* milli: / 1000 */
			value *= 1000.0f;
			postfix = x_("m");
			break;
		case PUREUNITMICRO:		/* micro: / 1000000 */
			value *= 1000000.0f;
			postfix = x_("u");
			break;
		case PUREUNITNANO:		/* nano:  / 1000000000 */
			value *= 1000000000.0f;
			postfix = x_("n");
			break;
		case PUREUNITPICO:		/* pico:  / 1000000000000 */
			value *= 1000000000000.0f;
			postfix = x_("p");
			break;
		case PUREUNITFEMTO:		/* femto: / 1000000000000000 */
			value *= 1000000000000000.0f;
			postfix = x_("f");
			break;
	}
	esnprintf(line, 100, x_("%g%s"), value, postfix);
	return(line);
}

/*
 * Routine to convert the string "str" to electrical units of type "unittype",
 * presuming that the scale is in "unitscale".
 */
float figureunits(CHAR *str, INTBIG unittype, INTBIG unitscale)
{
	float value;
	REGISTER INTBIG len, pureunit;

	len = estrlen(str);
	value = (float)eatof(str);
	pureunit = db_getpureunit(unittype, unitscale);
	if (len > 1)
	{
		if (namesame(&str[len-1], x_("g")) == 0) pureunit = PUREUNITGIGA;
		if (namesame(&str[len-1], x_("k")) == 0) pureunit = PUREUNITKILO;
		if (namesame(&str[len-1], x_("m")) == 0) pureunit = PUREUNITMILLI;
		if (namesame(&str[len-1], x_("u")) == 0) pureunit = PUREUNITMICRO;
		if (namesame(&str[len-1], x_("n")) == 0) pureunit = PUREUNITNANO;
		if (namesame(&str[len-1], x_("p")) == 0) pureunit = PUREUNITPICO;
		if (namesame(&str[len-1], x_("f")) == 0) pureunit = PUREUNITFEMTO;
	}
	if (len > 3)
	{
		if (namesame(&str[len-3], x_("meg")) == 0) pureunit = PUREUNITMEGA;
	}
	switch (pureunit)
	{
		case PUREUNITGIGA:		/* giga:  x 1000000000 */
			value *= 1000000000.0f;
			break;
		case PUREUNITMEGA:		/* mega:  x 1000000 */
			value *= 1000000.0f;
			break;
		case PUREUNITKILO:		/* kilo:  x 1000 */
			value *= 1000.0f;
			break;
		case PUREUNITMILLI:		/* milli: / 1000 */
			value /= 1000.0f;
			break;
		case PUREUNITMICRO:		/* micro: / 1000000 */
			value /= 1000000.0f;
			break;
		case PUREUNITNANO:		/* nano:  / 1000000000 */
			value /= 1000000000.0f;
			break;
		case PUREUNITPICO:		/* pico:  / 1000000000000 */
			value /= 1000000000000.0f;
			break;
		case PUREUNITFEMTO:		/* femto: / 1000000000000000 */
			value /= 1000000000000000.0f;
			break;
	}
	return(value);
}

/*
 * Routine to parse the version of Electric in "version" into three fields:
 * the major version number, minor version, and a detail version number.
 * The detail version number can be letters.  If it is omitted, it is
 * assumed to be 999.  If it is a number, it is beyond 1000.  For example:
 *    "6.02a"     major=6, minor=2, detail=1       (a Prerelease)
 *    "6.02z"     major=6, minor=2, detail=26      (a Prerelease)
 *    "6.02aa"    major=6, minor=2, detail=27      (a Prerelease)
 *    "6.02az"    major=6, minor=2, detail=52      (a Prerelease)
 *    "6.02ba"    major=6, minor=2, detail=53      (a Prerelease)
 *    "6.02"      major=6, minor=2, detail=999     (a Release)
 *    "6.02.1"    major=6, minor=2, detail=1001    (a PostRelease, update)
 */
void parseelectricversion(CHAR *version, INTBIG *major, INTBIG *minor, INTBIG *detail)
{
	REGISTER CHAR *pt;

	/* parse the version fields */
	pt = version;
	*major = eatoi(pt);
	while (isdigit(*pt) != 0) pt++;
	if (*pt++ != '.') { *minor = *detail = 0;   return; }
	*minor = eatoi(pt);
	while (isdigit(*pt) != 0) pt++;
	if (*pt == 0) { *detail = 999;   return; }
	if (*pt == '.')
	{
		*detail = eatoi(&pt[1]) + 1000;
	} else
	{
		*detail = 0;
		while (isalpha(*pt))
		{
			*detail = (*detail * 26) + tolower(*pt) - 'a' + 1;
			pt++;
		}
	}
}

/*
 * routine to determine which node prototype is referred to by "line"
 * and return that nodeproto.  The routine returns NONODEPROTO if the
 * prototype cannot be determined.
 */
NODEPROTO *getnodeproto(CHAR *initline)
{
	REGISTER NODEPROTO *np;
	REGISTER TECHNOLOGY *tech, *t;
	REGISTER INTBIG wantversion, save, saidtech, saidlib;
	REGISTER LIBRARY *lib, *l;
	REGISTER VIEW *wantview, *v;
	REGISTER CHAR *pt, *line, *nameend, *viewname;
	REGISTER void *infstr;

	/* make a copy of the argument so that it can be modified */
	infstr = initinfstr();
	addstringtoinfstr(infstr, initline);
	line = returninfstr(infstr);

	tech = el_curtech;   lib = el_curlib;
	saidtech = saidlib = 0;
	for(pt = line; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt != ':') pt = line; else
	{
		*pt = 0;
		t = gettechnology(line);
		if (t != NOTECHNOLOGY)
		{
			tech = t;
			saidtech++;
		}
		l = getlibrary(line);
		if (l != NOLIBRARY)
		{
			lib = l;
			saidlib++;
		}
		*pt++ = ':';
		line = pt;
	}

	/* try primitives in the technology */
	if (saidlib == 0 || saidtech != 0)
	{
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesame(line, np->protoname) == 0) return(np);
	}

	/* get the view and version information */
	for(pt = line; *pt != 0; pt++) if (*pt == ';' || *pt == '{') break;
	nameend = pt;
	wantversion = -1;
	wantview = el_unknownview;
	if (*pt == ';')
	{
		wantversion = myatoi(pt+1);
		for(pt++; *pt != 0; pt++) if (*pt == '{') break;
	}
	if (*pt == '{')
	{
		viewname = pt = (pt + 1);
		for(; *pt != 0; pt++) if (*pt == '}') break;
		if (*pt != '}') return(NONODEPROTO);
		*pt = 0;
		for(v = el_views; v != NOVIEW; v = v->nextview)
			if (namesame(v->sviewname, viewname) == 0 || namesame(v->viewname, viewname) == 0) break;
		*pt = '}';
		if (v == NOVIEW) return(NONODEPROTO);
		wantview = v;
	}
	save = *nameend;
	*nameend = 0;
	np = db_findnodeprotoname(line, wantview, lib);
	if (np == NONODEPROTO && wantview == el_unknownview)
	{
		/* search for any view */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesame(line, np->protoname) == 0) break;
	}
	*nameend = (CHAR)save;
	if (np == NONODEPROTO) return(NONODEPROTO);
	if (wantversion < 0 || np->version == wantversion) return(np);
	for(np = np->prevversion; np != NONODEPROTO; np = np->prevversion)
		if (np->version == wantversion) return(np);
	return(NONODEPROTO);
}

/*
 * routine to find technology "techname".  Returns NOTECHNOLOGY if it cannot
 * be found
 */
static COMCOMP db_technologyp = {NOKEYWORD, topoftechs, nexttechs, NOPARAMS,
	0, x_(" \t"), M_("technology"), 0};
TECHNOLOGY *gettechnology(CHAR *techname)
{
	REGISTER INTBIG i;
	REGISTER TECHNOLOGY *tech;

	i = parse(techname, &db_technologyp, FALSE);
	if (i < 0) return(NOTECHNOLOGY);
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		if (tech->techindex == i) return(tech);
	return(NOTECHNOLOGY);
}

/*
 * routine to find view "viewname".  Returns NOVIEW if it cannot be found
 */
static COMCOMP db_viewp = {NOKEYWORD, topofviews, nextviews, NOPARAMS,
	0, x_(" \t"), M_("view"), 0};
VIEW *getview(CHAR *viewname)
{
	REGISTER INTBIG i, j;
	REGISTER VIEW *v;

	i = parse(viewname, &db_viewp, FALSE);
	if (i < 0) return(NOVIEW);
	for(j=0, v = el_views; v != NOVIEW; v = v->nextview, j++)
		if (j == i) return(v);
	return(NOVIEW);
}

/*
 * routine to find network "netname" in cell "cell".  Returns NONETWORK
 * if it cannot be found
 */
NETWORK *getnetwork(CHAR *netname, NODEPROTO *cell)
{
	REGISTER INTBIG i, k;
	REGISTER CHAR *pt;
	REGISTER NETWORK *net;

	net = net_getnetwork(netname, cell);
	if (net != NONETWORK) return(net);

	/* see if a global name is specified */
	pt = _("Global");
	k = estrlen(pt);
	if (namesamen(netname, pt, k) == 0 && netname[k] == '-')
	{
		for(i=0; i<cell->globalnetcount; i++)
			if (namesame(&netname[k+1], cell->globalnetnames[i]) == 0)
				return(cell->globalnetworks[i]);
	}
	return(NONETWORK);
}

/*
 * routine to find network "netname" in cell "cell".  Returns a list of
 * networks, terminated by NONETWORK.  This routine allows for variations of the network
 * name that may occur due to simulation, VHDL compilation, etc.
 */
NETWORK **getcomplexnetworks(CHAR *name, NODEPROTO *np)
{
	REGISTER INTBIG len, i, l, k, c1, c2, addr;
	INTBIG count;
	REGISTER NETWORK *net;
	REGISTER CHAR *pt, save, *lastslash;

	/* make sure there is a list of networks */
	if (db_networklist == 0) db_networklist = newintlistobj(db_cluster);
	clearintlistobj(db_networklist);

	/* try the direct approach */
	net = getnetwork(name, np);
	if (net != NONETWORK)
	{
		addtointlistobj(db_networklist, (INTBIG)net, FALSE);
		addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
		return((NETWORK **)getintlistobj(db_networklist, &count));
	}

	/* some netlisters generate the net name as "NET" followed by the object address */
	if (name[0] == 'N' && name[1] == 'E' && name[2] == 'T')
	{
		addr = eatoi(&name[3]);
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net == (NETWORK *)addr)
			{
				addtointlistobj(db_networklist, (INTBIG)net, FALSE);
				addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
				return((NETWORK **)getintlistobj(db_networklist, &count));
			}
		}
	}

	/* if the name ends with "NV", try removing that */
	len = estrlen(name);
	if (name[len-2] == 'N' && name[len-1] == 'V')
	{
		name[len-2] = 0;
		net = getnetwork(name, np);
		name[len-2] = 'N';
		if (net != NONETWORK)
		{
			addtointlistobj(db_networklist, (INTBIG)net, FALSE);
			addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
			return((NETWORK **)getintlistobj(db_networklist, &count));
		}
	}

	/* check for prefix "v(" and "l(" HSPICE prefixes (see also "simwindow.c:sim_window_findtrace()")*/
	if ((name[0] == 'l' || name[0] == 'v') && name[1] == '(')
	{
		for(pt = &name[2]; pt != 0; pt++)
			if (*pt == ')') break;
		save = *pt;   *pt = 0;
		net = getnetwork(&name[2], np);
		*pt = save;
		if (net != NONETWORK)
		{
			addtointlistobj(db_networklist, (INTBIG)net, FALSE);
			addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
			return((NETWORK **)getintlistobj(db_networklist, &count));
		}
	}

	/* if there are underscores, see if they match original names */
	for(pt = name; *pt != 0; pt++) if (*pt == '_') break;
	if (*pt != 0)
	{
		len = estrlen(name);
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			for(k=0; k<net->namecount; k++)
			{
				pt = networkname(net, k);
				l = estrlen(pt);
				if (l == len)
				{
					for(i=0; i<len; i++)
					{
						c1 = tolower(name[i]);
						c2 = tolower(pt[i]);
						if (c1 == c2) continue;
						if (c1 == '_' && !isalnum(c2)) continue;
						break;
					}
					if (i >= len)
					{
						addtointlistobj(db_networklist, (INTBIG)net, FALSE);
						addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
						return((NETWORK **)getintlistobj(db_networklist, &count));
					}
				}
			}
		}
	}

	/* if this is a simple name, let it match anything indexed from that name */
	for(pt = name; *pt != 0; pt++) if (*pt == '[') break;
	if (*pt == 0)
	{
		l = estrlen(name);
		for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			for(k=0; k<net->namecount; k++)
			{
				pt = networkname(net, k);
				if (namesamen(pt, name, l) == 0 && pt[l] == '[')
					addtointlistobj(db_networklist, (INTBIG)net, TRUE);
			}
		}
	}

	/* if there are slashes in the name, see if the end part matches */
	lastslash = 0;
	for(pt = name; *pt != 0; pt++) if (*pt == '/') lastslash = pt;
	if (lastslash != 0)
	{
		net = getnetwork(&lastslash[1], np);
		if (net != NONETWORK)
		{
			addtointlistobj(db_networklist, (INTBIG)net, FALSE);
			addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
			return((NETWORK **)getintlistobj(db_networklist, &count));
		}
	}

	addtointlistobj(db_networklist, (INTBIG)NONETWORK, FALSE);
	return((NETWORK **)getintlistobj(db_networklist, &count));
}

/*
 * routine to determine which arc prototype is referred to by "line"
 * and return that arcproto.  The routine returns NOARCPROTO if the prototype
 * cannot be determined.
 */
ARCPROTO *getarcproto(CHAR *initline)
{
	REGISTER ARCPROTO *ap;
	REGISTER TECHNOLOGY *tech, *t;
	REGISTER CHAR *pt, *line;
	REGISTER void *infstr;

	/* make a copy of the argument so that it can be modified */
	infstr = initinfstr();
	addstringtoinfstr(infstr, initline);
	line = returninfstr(infstr);

	tech = el_curtech;
	for(pt = line; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt != ':') pt = line; else
	{
		*pt = 0;
		t = gettechnology(line);
		if (t != NOTECHNOLOGY) tech = t;
		*pt++ = ':';
	}
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		if (namesame(pt, ap->protoname) == 0) return(ap);
	return(NOARCPROTO);
}

/*
 * routine to find portproto "portname" on cell "cell".  Returns NOPORTPROTO
 * if it cannot be found
 */
PORTPROTO *getportproto(NODEPROTO *cell, CHAR *portname)
{
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i, j;

	if (cell->portprotohashtablesize > 0)
	{
		i = db_namehash(portname) % cell->portprotohashtablesize;
		for(j=1; j<=cell->portprotohashtablesize; j += 2)
		{
			pp = cell->portprotohashtable[i];
			if (pp == NOPORTPROTO) break;
			if (namesame(portname, pp->protoname) == 0) return(pp);
			i += j;
			if (i >= cell->portprotohashtablesize) i -= cell->portprotohashtablesize;
		}
	} else
	{
		for (pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			if (namesame(portname, pp->protoname) == 0) return(pp);
	}
	return(NOPORTPROTO);
}

/*
 * routine to find library "libname".  Returns NOLIBRARY if it cannot be found
 */
LIBRARY *getlibrary(CHAR *libname)
{
	REGISTER LIBRARY *lib;

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if (namesame(libname, lib->libname) == 0) return(lib);
	return(NOLIBRARY);
}

/*
 * routine to find tool "toolname".  Returns NOTOOL if it cannot be found
 */
TOOL *gettool(CHAR *toolname)
{
	REGISTER INTBIG i;

	for(i=0; i<el_maxtools; i++)
		if (namesame(toolname, el_tools[i].toolname) == 0) return(&el_tools[i]);
	return(NOTOOL);
}

static struct
{
	CHAR  *name;
	CHAR  *symbol;
	INTBIG value;
} db_colors[] =
{
	{N_("none"),           x_("ALLOFF"),  ALLOFF},
	{N_("transparent-1"),  x_("COLORT1"), COLORT1},
	{N_("transparent-2"),  x_("COLORT2"), COLORT2},
	{N_("transparent-3"),  x_("COLORT3"), COLORT3},
	{N_("transparent-4"),  x_("COLORT4"), COLORT4},
	{N_("transparent-5"),  x_("COLORT5"), COLORT5},
	{N_("overlappable-1"), x_("COLORT1"), COLORT1},
	{N_("overlappable-2"), x_("COLORT2"), COLORT2},
	{N_("overlappable-3"), x_("COLORT3"), COLORT3},
	{N_("overlappable-4"), x_("COLORT4"), COLORT4},
	{N_("overlappable-5"), x_("COLORT5"), COLORT5},
	{N_("white"),          x_("WHITE"),   WHITE},
	{N_("black"),          x_("BLACK"),   BLACK},
	{N_("red"),            x_("RED"),     RED},
	{N_("blue"),           x_("BLUE"),    BLUE},
	{N_("green"),          x_("GREEN"),   GREEN},
	{N_("cyan"),           x_("CYAN"),    CYAN},
	{N_("magenta"),        x_("MAGENTA"), MAGENTA},
	{N_("yellow"),         x_("YELLOW"),  YELLOW},
	{N_("gray"),           x_("GRAY"),    GRAY},
	{N_("orange"),         x_("ORANGE"),  ORANGE},
	{N_("purple"),         x_("PURPLE"),  PURPLE},
	{N_("brown"),          x_("BROWN"),   BROWN},
	{N_("light-gray"),     x_("LGRAY"),   LGRAY},
	{N_("dark-gray"),      x_("DGRAY"),   DGRAY},
	{N_("light-red"),      x_("LRED"),    LRED},
	{N_("dark-red"),       x_("DRED"),    DRED},
	{N_("light-green"),    x_("LGREEN"),  LGREEN},
	{N_("dark-green"),     x_("DGREEN"),  DGREEN},
	{N_("light-blue"),     x_("LBLUE"),   LBLUE},
	{N_("dark-blue"),      x_("DBLUE"),   DBLUE},
	{NULL, NULL, 0}
};

/*
 * Routine to convert the color name "colorname" to a color.  Returns negative on error.
 */
INTBIG getecolor(CHAR *colorname)
{
	REGISTER INTBIG i;
	REGISTER CHAR *truecolorname;

	truecolorname = TRANSLATE(colorname);
	for(i=0; db_colors[i].name != 0; i++)
		if (namesame(truecolorname, TRANSLATE(db_colors[i].name)) == 0)
			return(db_colors[i].value);
	return(-1);
}

/*
 * Routine to convert color "color" to a full name (i.e. "light-gray") in "colorname" and a
 * symbol name (i.e. "LGRAY") in "colorsymbol".  Returns true if the color is unknown.
 */
BOOLEAN ecolorname(INTBIG color, CHAR **colorname, CHAR **colorsymbol)
{
	REGISTER INTBIG i;

	for(i=0; db_colors[i].name != 0; i++)
		if (db_colors[i].value == color)
	{
		*colorname = TRANSLATE(db_colors[i].name);
		*colorsymbol = db_colors[i].symbol;
		return(FALSE);
	}
	return(TRUE);
}

/*
 * routine to parse a set of commands in "list" against the keyword in
 * "keyword" and return the index in the list of the keyword.  A return of
 * -1 indicates failure to parse the command and an error message will be
 * issued if "noise" is nonzero.
 */
INTBIG parse(CHAR *keyword, COMCOMP *list, BOOLEAN noise)
{
	REGISTER INTBIG i, j;
	BOOLEAN (*toplist)(CHAR**);
	INTBIG w, bst;
	REGISTER CHAR *pp, *(*nextinlist)(void);
	REGISTER void *infstr;

	us_pathiskey = list->ifmatch;
	toplist = list->toplist;
	nextinlist = list->nextcomcomp;

	(void)(*toplist)(&keyword);
	for(i=0, w=0; (pp = (*nextinlist)()) != 0; w++)
	{
		j = stringmatch(pp, keyword);
		if (j == -2) return(w);
		if (j < 0) continue;
		if (j > i)
		{
			i = j;   bst = w;
		} else if (j == i) bst = -1;
	}

	/* if nothing found, give an error */
	if (i == 0)
	{
		if (noise != 0) ttyputerr(_("Unknown command: %s"), keyword);
		return(-1);
	}

	/* if there is unambiguous match, return it */
	if (bst >= 0) return(bst);

	/* print ambiguities */
	if (noise != 0)
	{
		infstr = initinfstr();
		(void)(*toplist)(&keyword);
		for( ; (pp = (*nextinlist)()) != 0; )
		{
			if ((j = stringmatch(pp, keyword)) < 0) continue;
			if (j < i) continue;
			addtoinfstr(infstr, ' ');
			addstringtoinfstr(infstr, pp);
		}
		ttyputerr(_("%s ambiguous:%s"), keyword, returninfstr(infstr));
	}
	return(-1);
}

/*
 * routine to report the amount of match that string "keyword" and "input"
 * have in common.  Returns the number of characters that match.  Returns -2
 * if they are equal, -1 if there are extra characters at the end of "input"
 * which make the match erroneous.  Ignores case distinction.
 */
INTBIG stringmatch(CHAR *keyword, CHAR *input)
{
	REGISTER INTBIG j;
	REGISTER CHAR c, d;

	for(j=0; (c = input[j]) != 0; j++)
	{
		if (isupper(c)) c = tolower(c);
		d = keyword[j];  if (isupper(d)) d = tolower(d);
		if (c != d) break;
	}
	if (c != 0) return(-1);
	if (keyword[j] == 0) return(-2);
	return(j);
}

/************************* COMMAND COMPLETION CODE *************************/

static INTBIG  db_filestrlen;
static CHAR    db_filekey[100];
static CHAR    db_directorypath[256];
static CHAR    db_fileextension[10];
static INTBIG  db_filecount, db_filetotal;
static CHAR  **db_filesindir;

void requiredextension(CHAR *extension)
{
	(void)estrcpy(db_fileextension, extension);
}

BOOLEAN topoffile(CHAR **a)
{
	db_fileextension[0] = 0;
	return(db_beginsearch(a));
}

BOOLEAN topoflibfile(CHAR **a)
{
	(void)estrcpy(db_fileextension, x_(".elib"));
	return(db_beginsearch(a));
}

BOOLEAN db_beginsearch(CHAR **a)
{
	INTBIG i;
	static CHAR file[256];
	CHAR *pt;

	/* build the full file name */
	(void)estrcpy(file, truepath(*a));
	*a = file;

	/* search for directory specifications */
	for(i=estrlen(file)-1; i > 0; i--) if (file[i] == DIRSEP) break;
	if (file[i] == DIRSEP) i++;
	(void)estrcpy(db_filekey, &file[i]);
	db_filestrlen = estrlen(db_filekey);
	file[i] = 0;
	estrcpy(db_directorypath, file);
	db_filecount = 0;
	db_filetotal = filesindirectory(file, &db_filesindir);

	/* advance pointer to after the directory separator */
	pt = *a;
	for(i=estrlen(pt)-1; i > 0; i--) if (pt[i] == DIRSEP) break;
	if (i > 0) *a = &pt[i+1];
	return(TRUE);
}

CHAR *nextfile(void)
{
	CHAR *pt;
	static CHAR testfile[256];

	for(;;)
	{
		if (db_filecount >= db_filetotal) break;
		pt = db_filesindir[db_filecount];
		db_filecount++;

		/* see if the file is valid */
		if (pt[0] == '.') continue;
		if (estrncmp(db_filekey, pt, db_filestrlen) != 0) continue;
		(void)estrcpy(testfile, db_directorypath);
		(void)estrcat(testfile, pt);
		if (fileexistence(testfile) == 2)
		{
			estrcpy(testfile, pt);
			estrcat(testfile, DIRSEPSTR);
			return(testfile);
		}
		if (db_fileextension[0] != 0)
		{
			if (estrcmp(&pt[estrlen(pt)-estrlen(db_fileextension)], db_fileextension) != 0)
				continue;
		}
		return(pt);
	}
	return(0);
}

/*
 * routines to do command completion on technology names
 */
static TECHNOLOGY *db_postechcomcomp;
BOOLEAN topoftechs(CHAR **c) { Q_UNUSED( c ); db_postechcomcomp = el_technologies; return(TRUE); }
CHAR *nexttechs(void)
{
	REGISTER CHAR *retname;

	if (db_postechcomcomp == NOTECHNOLOGY) return(0);
	retname = db_postechcomcomp->techname;
	db_postechcomcomp = db_postechcomcomp->nexttechnology;
	return(retname);
}

/*
 * routines to do command completion on view names
 */
static VIEW *db_posviewcomcomp;
BOOLEAN topofviews(CHAR **c) { Q_UNUSED( c ); db_posviewcomcomp = el_views; return(TRUE); }
CHAR *nextviews(void)
{
	REGISTER CHAR *retname;

	if (db_posviewcomcomp == NOVIEW) return(0);
	retname = db_posviewcomcomp->viewname;
	db_posviewcomcomp = db_posviewcomcomp->nextview;
	return(retname);
}

/*
 * routines to do command completion on library names
 */
static LIBRARY *db_poslibcomcomp;
BOOLEAN topoflibs(CHAR **c)
{
	Q_UNUSED( c );
	db_poslibcomcomp = el_curlib;
	return(TRUE);
}
CHAR *nextlibs(void)
{
	REGISTER CHAR *retname;

	for(;;)
	{
		if (db_poslibcomcomp == NOLIBRARY) return(0);
		if ((db_poslibcomcomp->userbits&HIDDENLIBRARY) == 0)
		{
			retname = db_poslibcomcomp->libname;
			db_poslibcomcomp = db_poslibcomcomp->nextlibrary;
			break;
		} else
		{
			db_poslibcomcomp = db_poslibcomcomp->nextlibrary;
		}
	}
	return(retname);
}

/*
 * routines to do command completion on tool names
 */
static INTBIG db_poscomcomp;
BOOLEAN topoftools(CHAR **c) { Q_UNUSED( c ); db_poscomcomp = 0; return(TRUE); }
CHAR *nexttools(void)
{
	if (db_poscomcomp >= el_maxtools) return(0);
	return(el_tools[db_poscomcomp++].toolname);
}

/*
 * routines to do command completion on cell names
 */
static NODEPROTO *db_posnodeprotos;
BOOLEAN topofcells(CHAR **c)
{
	REGISTER CHAR *pt;
	REGISTER LIBRARY *lib;

	/* by default, assume the current library */
	db_posnodeprotos = el_curlib->firstnodeproto;

	/* see if a library specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		lib = getlibrary(*c);
		*pt++ = ':';
		if (lib != NOLIBRARY)
		{
			*c = pt;
			db_posnodeprotos = lib->firstnodeproto;
		}
	}
	return(TRUE);
}
CHAR *nextcells(void)
{
	REGISTER CHAR *ret;

	if (db_posnodeprotos != NONODEPROTO)
	{
		ret = describenodeproto(db_posnodeprotos);
		db_posnodeprotos = db_posnodeprotos->nextnodeproto;
		return(ret);
	}
	return(0);
}

/*
 * routines to do command completion on arc names
 */
static ARCPROTO *db_posarcs;
BOOLEAN topofarcs(CHAR **c)
{
	REGISTER CHAR *pt;
	REGISTER TECHNOLOGY *t;

	/* by default, assume the current technology */
	db_posarcs = el_curtech->firstarcproto;

	/* see if a technology specification was given */
	for(pt = *c; *pt != 0; pt++) if (*pt == ':') break;
	if (*pt == ':')
	{
		*pt = 0;
		t = gettechnology(*c);
		*pt++ = ':';
		if (t != NOTECHNOLOGY)
		{
			*c = pt;
			db_posarcs = t->firstarcproto;
		}
	}
	return(TRUE);
}
CHAR *nextarcs(void)
{
	REGISTER CHAR *ret;

	if (db_posarcs != NOARCPROTO)
	{
		ret = describearcproto(db_posarcs);
		db_posarcs = db_posarcs->nextarcproto;
		return(ret);
	}
	return(0);
}

/*
 * routines to do command completion on network names
 */
static NETWORK *db_posnets;
static INTBIG db_posinnet;

BOOLEAN topofnets(CHAR **c)
{
	REGISTER NODEPROTO *np;
	Q_UNUSED( c );

	db_posnets = NONETWORK;
	np = getcurcell();
	if (np == NONODEPROTO) return(FALSE);
	db_posnets = np->firstnetwork;
	if (db_posnets == NONETWORK) return(FALSE);
	db_posinnet = 0;
	return(TRUE);
}
CHAR *nextnets(void)
{
	REGISTER CHAR *ret;

	for(;;)
	{
		if (db_posnets == NONETWORK) return(0);
		if (db_posinnet >= db_posnets->namecount)
		{
			db_posnets = db_posnets->nextnetwork;
			if (db_posnets == NONETWORK) return(0);
			db_posinnet = 0;
		} else
		{
			ret = networkname(db_posnets, db_posinnet);
			db_posinnet++;
			break;
		}
	}
	return(ret);
}

/************************* OUTPUT PREPARATION *************************/

/*
 * routine to return the full name of node "ni", including its local name.
 * Technology considerations are ignored.
 */
CHAR *ntdescribenodeinst(NODEINST *ni)
{
	REGISTER TECHNOLOGY *curtech;
	REGISTER CHAR *ret;

	if (ni == NONODEINST) return(x_("***NONODEINST***"));

	if (ni->proto->primindex == 0) return(describenodeinst(ni));
	curtech = el_curtech;
	el_curtech = ni->proto->tech;
	ret = describenodeinst(ni);
	el_curtech = curtech;
	return(ret);
}

/* routine to return the name of nodeinst "ni" */
CHAR *describenodeinst(NODEINST *ni)
{
	REGISTER CHAR *name, *protoname;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len;

	if (ni == NONODEINST) return(x_("***NONODEINST***"));

	/* see if there is a local name on the node */
	protoname = describenodeproto(ni->proto);
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
	if (var == NOVARIABLE) return(protoname);

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* get output buffer */
	db_descninodeswitch++;
	if (db_descninodeswitch >= NUMDESCRIPTIONBUFS) db_descninodeswitch = 0;

	/* make sure buffer has enough room */
	len = estrlen(protoname) + estrlen((CHAR *)var->addr) + 3;
	if (len > db_descnioutputsize[db_descninodeswitch])
	{
		if (db_descnioutputsize[db_descninodeswitch] != 0)
			efree(db_descnioutput[db_descninodeswitch]);
		db_descnioutputsize[db_descninodeswitch] = 0;
		db_descnioutput[db_descninodeswitch] = (CHAR *)emalloc(len * SIZEOFCHAR, db_cluster);
		if (db_descnioutput[db_descninodeswitch] == 0) return(x_(""));
		db_descnioutputsize[db_descninodeswitch] = len;
	}

	/* store the name */
	name = db_descnioutput[db_descninodeswitch];
	(void)estrcpy(name, protoname);
	(void)estrcat(name, x_("["));
	(void)estrcat(name, (CHAR *)var->addr);
	(void)estrcat(name, x_("]"));
	return(name);
}

/* routine to return the name of arcinst "ai" */
CHAR *describearcinst(ARCINST *ai)
{
	REGISTER CHAR *name, *pname;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len;

	if (ai == NOARCINST) return(x_("***NOARCINST***"));

	/* get local arc name */
	pname = describearcproto(ai->proto);
	var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
	if (var == NOVARIABLE) return(pname);

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* get output buffer */
	db_descaiarcswitch++;
	if (db_descaiarcswitch >= NUMDESCRIPTIONBUFS) db_descaiarcswitch = 0;

	/* make sure buffer has enough room */
	len = estrlen(pname) + estrlen((CHAR *)var->addr) + 3;
	if (len > db_descaioutputsize[db_descaiarcswitch])
	{
		if (db_descaioutputsize[db_descaiarcswitch] != 0)
			efree(db_descaioutput[db_descaiarcswitch]);
		db_descaioutputsize[db_descaiarcswitch] = 0;
		db_descaioutput[db_descaiarcswitch] = (CHAR *)emalloc(len * SIZEOFCHAR, db_cluster);
		if (db_descaioutput[db_descaiarcswitch] == 0) return(x_(""));
		db_descaioutputsize[db_descaiarcswitch] = len;
	}

	name = db_descaioutput[db_descaiarcswitch];
	(void)estrcpy(name, pname);
	(void)estrcat(name, x_("["));
	(void)estrcat(name, (CHAR *)var->addr);
	(void)estrcat(name, x_("]"));
	return(name);
}

/*
 * routine to return the full name of cell "np", including its view type
 * (if any) and version (if not most recent).  Library considerations are
 * ignored.
 */
CHAR *nldescribenodeproto(NODEPROTO *np)
{
	REGISTER LIBRARY *curlib;
	REGISTER CHAR *ret;

	if (np == NONODEPROTO) return(x_("***NONODEPROTO***"));

	if (np->primindex != 0) return(np->protoname);

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	curlib = el_curlib;
	el_curlib = np->lib;
	ret = describenodeproto(np);
	el_curlib = curlib;
	return(ret);
}

/*
 * routine to return the full name of cell "np", including its library name
 * (if different from the current), view type (if any), and version (if not
 * most recent).
 */
CHAR *describenodeproto(NODEPROTO *np)
{
	CHAR line[50];
	REGISTER CHAR *name;
	REGISTER INTBIG len;

	if (np == NONODEPROTO) return(x_("***NONODEPROTO***"));

	/* simple tests for direct name use */
	if (np->primindex != 0)
	{
		/* if a primitive in the current technology, simply use name */
		if (np->tech == el_curtech) return(np->protoname);
	} else
	{
		/* if view unknown, version recent, library current, simply use name */
		if (*np->cellview->sviewname == 0 && np->newestversion == np && np->lib == el_curlib)
			return(np->protoname);
	}

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* get output buffer */
	db_descnpnodeswitch++;
	if (db_descnpnodeswitch >= NUMDESCRIPTIONBUFS) db_descnpnodeswitch = 0;

	if (np->primindex != 0)
	{
		len = estrlen(np->protoname) + estrlen(np->tech->techname) + 2;
	} else
	{
		/* compute size of buffer */
		if (np->protoname == 0) return(x_("***BOGUS***"));
		len = estrlen(np->protoname) + 1;
		if (np->lib != el_curlib) len += estrlen(np->lib->libname) + 1;
		if (np->newestversion != np)
		{
			(void)esnprintf(line, 50, x_(";%ld"), np->version);
			len += estrlen(line);
		}
		if (*np->cellview->sviewname != 0) len += estrlen(np->cellview->sviewname) + 2;
	}

	/* make sure buffer has enough room */
	if (len > db_descnpoutputsize[db_descnpnodeswitch])
	{
		if (db_descnpoutputsize[db_descnpnodeswitch] != 0)
			efree(db_descnpoutput[db_descnpnodeswitch]);
		db_descnpoutputsize[db_descnpnodeswitch] = 0;
		db_descnpoutput[db_descnpnodeswitch] = (CHAR *)emalloc(len * SIZEOFCHAR, db_cluster);
		if (db_descnpoutput[db_descnpnodeswitch] == 0) return(x_(""));
		db_descnpoutputsize[db_descnpnodeswitch] = len;
	}

	/* construct complete name */
	name = db_descnpoutput[db_descnpnodeswitch];
	if (np->primindex != 0)
	{
		(void)estrcpy(name, np->tech->techname);
		(void)estrcat(name, x_(":"));
		(void)estrcat(name, np->protoname);
	} else
	{
		if (np->lib != el_curlib)
		{
			(void)estrcpy(name, np->lib->libname);
			(void)estrcat(name, x_(":"));
			(void)estrcat(name, np->protoname);
		} else (void)estrcpy(name, np->protoname);
		if (np->newestversion != np) (void)estrcat(name, line);
		if (*np->cellview->sviewname != 0)
		{
			(void)estrcat(name, x_("{"));
			(void)estrcat(name, np->cellview->sviewname);
			(void)estrcat(name, x_("}"));
		}
	}
	return(name);
}

/* routine to return the name of arcproto "ap" */
CHAR *describearcproto(ARCPROTO *ap)
{
	REGISTER CHAR *name;
	REGISTER INTBIG len;

	if (ap == NOARCPROTO) return(x_("***NOARCPROTO***"));

	if (ap->tech == el_curtech) return(ap->protoname);

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* get output buffer */
	db_descaparcswitch++;
	if (db_descaparcswitch >= NUMDESCRIPTIONBUFS) db_descaparcswitch = 0;

	/* make sure buffer has enough room */
	len = estrlen(ap->tech->techname) + estrlen(ap->protoname) + 2;
	if (len > db_descapoutputsize[db_descaparcswitch])
	{
		if (db_descapoutputsize[db_descaparcswitch] != 0)
			efree(db_descapoutput[db_descaparcswitch]);
		db_descapoutputsize[db_descaparcswitch] = 0;
		db_descapoutput[db_descaparcswitch] = (CHAR *)emalloc(len * SIZEOFCHAR, db_cluster);
		if (db_descapoutput[db_descaparcswitch] == 0) return(x_(""));
		db_descapoutputsize[db_descaparcswitch] = len;
	}

	name = db_descapoutput[db_descaparcswitch];
	(void)estrcpy(name, ap->tech->techname);
	(void)estrcat(name, x_(":"));
	(void)estrcat(name, ap->protoname);
	return(name);
}

/* routine to return the name of the object whose geometry module is "geom" */
CHAR *geomname(GEOM *geom)
{
	if (geom == NOGEOM) return(x_("***NOGEOM***"));
	if (geom->entryisnode) return(describenodeinst(geom->entryaddr.ni));
	return(describearcinst(geom->entryaddr.ai));
}

/*
 * routine to convert network "net" into a string
 */
CHAR *describenetwork(NETWORK *net)
{
	REGISTER NODEPROTO *np;
	static CHAR gennetname[50];
	REGISTER INTBIG i, namecount;
	REGISTER void *infstr;

	if (net == NONETWORK) return(x_("***NONETWORK***"));

	if (net->globalnet >= 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, _("Global"));
		addtoinfstr(infstr, '-');
		if (net->globalnet == GLOBALNETGROUND) addstringtoinfstr(infstr, _("Ground")); else
		if (net->globalnet == GLOBALNETPOWER) addstringtoinfstr(infstr, _("Power")); else
		{
			np = net->parent;
			if (net->globalnet >= np->globalnetcount)
				addstringtoinfstr(infstr, _("UNKNOWN")); else
					addstringtoinfstr(infstr, np->globalnetnames[net->globalnet]);
		}
		return(returninfstr(infstr));
	}
	if (net->namecount == 0)
	{
		esnprintf(gennetname, 50, _("UNNAMED%ld"), (INTBIG)net);
		return(gennetname);
	}
	if (net->namecount == 1) return(networkname(net, 0));

	infstr = initinfstr();
	namecount = net->namecount;
	if (namecount > 10) namecount = 10;
	for(i=0; i<net->namecount; i++)
	{
		if (i != 0) addtoinfstr(infstr, '/');
		addstringtoinfstr(infstr, networkname(net, i));
	}
	if (net->namecount > namecount)
		addstringtoinfstr(infstr, x_("/..."));
	return(returninfstr(infstr));
}

CHAR *describeportbits(INTBIG userbits)
{
	switch (userbits&STATEBITS)
	{
		case INPORT:      return(_("Input"));
		case OUTPORT:     return(_("Output"));
		case BIDIRPORT:   return(_("Bidirectional"));
		case PWRPORT:     return(_("Power"));
		case GNDPORT:     return(_("Ground"));
		case CLKPORT:     return(_("Clock"));
		case C1PORT:      return(_("Clock Phase 1"));
		case C2PORT:      return(_("Clock Phase 2"));
		case C3PORT:      return(_("Clock Phase 3"));
		case C4PORT:      return(_("Clock Phase 4"));
		case C5PORT:      return(_("Clock Phase 5"));
		case C6PORT:      return(_("Clock Phase 6"));
		case REFOUTPORT:  return(_("Reference Output"));
		case REFINPORT:   return(_("Reference Input"));
		case REFBASEPORT: return(_("Reference Base"));
	}
	return(x_("Unknown"));
}

/*
 * routine to name the variable at "addr" of type "type".  It is assumed
 * to be an object that can hold other variables
 */
CHAR *describeobject(INTBIG addr, INTBIG type)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER GEOM *geom;
	REGISTER LIBRARY *lib;
	REGISTER TECHNOLOGY *tech;
	REGISTER TOOL *tool;
	REGISTER RTNODE *rtn;
	REGISTER VIEW *v;
	REGISTER WINDOWPART *win;
	REGISTER WINDOWFRAME *wf;
	REGISTER GRAPHICS *gra;
	REGISTER CONSTRAINT *con;
	REGISTER POLYGON *poly;
	REGISTER void *infstr;

	infstr = initinfstr();
	switch (type&VTYPE)
	{
		case VNODEINST:
			ni = (NODEINST *)addr;
			formatinfstr(infstr, x_("NodeInstance(%s)"), describenodeinst(ni));
			break;
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (np->primindex == 0) formatinfstr(infstr, x_("Cell(%s)"), describenodeproto(np)); else
				formatinfstr(infstr, x_("Primitive(%s)"), describenodeproto(np));
			break;
		case VPORTARCINST:
			pi = (PORTARCINST *)addr;
			if (pi == NOPORTARCINST) addstringtoinfstr(infstr, x_("PortArcInstance(NULL)")); else
				formatinfstr(infstr, x_("PortArcInstance(%ld)"), (INTBIG)pi);
			break;
		case VPORTEXPINST:
			pe = (PORTEXPINST *)addr;
			if (pe == NOPORTEXPINST) addstringtoinfstr(infstr, x_("PortExpInstance(NULL)")); else
				formatinfstr(infstr, x_("PortExpInstance(%ld)"), (INTBIG)pe);
			break;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			if (pp == NOPORTPROTO) addstringtoinfstr(infstr, x_("PortPrototype(NULL)")); else
				formatinfstr(infstr, x_("PortPrototype(%s)"), pp->protoname);
			break;
		case VARCINST:
			ai = (ARCINST *)addr;
			formatinfstr(infstr, x_("ArcInstance(%s)"), describearcinst(ai));
			break;
		case VARCPROTO:
			ap = (ARCPROTO *)addr;
			formatinfstr(infstr, x_("ArcPrototype(%s:%s)"), ap->tech->techname, ap->protoname);
			break;
		case VGEOM:
			geom = (GEOM *)addr;
			if (geom == NOGEOM) addstringtoinfstr(infstr, x_("Geom(NULL)")); else
				formatinfstr(infstr, x_("Geom(%ld)"), (INTBIG)geom);
			break;
		case VLIBRARY:
			lib = (LIBRARY *)addr;
			if (lib == NOLIBRARY) addstringtoinfstr(infstr, x_("Library(NULL)")); else
				formatinfstr(infstr, x_("Library(%s)"), lib->libname);
			break;
		case VTECHNOLOGY:
			tech = (TECHNOLOGY *)addr;
			if (tech == NOTECHNOLOGY) addstringtoinfstr(infstr, x_("Technology(NULL)")); else
				formatinfstr(infstr, x_("Technology(%s)"), tech->techname);
			break;
		case VTOOL:
			tool = (TOOL *)addr;
			if (tool == NOTOOL) addstringtoinfstr(infstr, x_("Tool(NULL)")); else
				formatinfstr(infstr, x_("Tool(%s)"), tool->toolname);
			break;
		case VRTNODE:
			rtn = (RTNODE *)addr;
			if (rtn == NORTNODE) addstringtoinfstr(infstr, x_("Rtnode(NULL)")); else
				formatinfstr(infstr, x_("Rtnode(%ld)"), (INTBIG)rtn);
			break;
		case VVIEW:
			v = (VIEW *)addr;
			if (v == NOVIEW) addstringtoinfstr(infstr, x_("View(NULL)")); else
				formatinfstr(infstr, x_("View(%s)"), v->viewname);
			break;
		case VWINDOWPART:
			win = (WINDOWPART *)addr;
			if (win != NOWINDOWPART) formatinfstr(infstr, x_("WindowPart(%s)"), win->location); else
				formatinfstr(infstr, x_("WindowPart(%ld)"), (INTBIG)win);
			break;
		case VGRAPHICS:
			gra = (GRAPHICS *)addr;
			if (gra == NOGRAPHICS) addstringtoinfstr(infstr, x_("Graphics(NULL)")); else
				formatinfstr(infstr, x_("Graphics(%ld)"), (INTBIG)gra);
			break;
		case VCONSTRAINT:
			con = (CONSTRAINT *)addr;
			if (con == NOCONSTRAINT) addstringtoinfstr(infstr, x_("Constraint(NULL)")); else
				formatinfstr(infstr, x_("Constraint(%s)"), con->conname);
			break;
		case VWINDOWFRAME:
			wf = (WINDOWFRAME *)addr;
			formatinfstr(infstr, x_("WindowFrame(%ld)"), (INTBIG)wf);
			break;
		case VPOLYGON:
			poly = (POLYGON *)addr;
			formatinfstr(infstr, x_("Polygon(%ld)"), (INTBIG)poly);
			break;
		default:
			addstringtoinfstr(infstr, x_("UNKNOWN(?)"));
			break;
	}
	return(returninfstr(infstr));
}

/*
 * routine to convert a lambda number to ascii
 */
#define OUTBUFS 12
CHAR *latoa(INTBIG i, INTBIG lambda)
{
	static INTBIG latoaswitch = 0;
	static CHAR output[OUTBUFS][20];
	double scale, number;
	REGISTER INTBIG len;
	REGISTER CHAR *cur;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* get output buffer */
	cur = output[latoaswitch++];
	if (latoaswitch >= OUTBUFS) latoaswitch = 0;

	/* determine scaling */
	if (lambda == 0) lambda = el_curlib->lambda[el_curtech->techindex];
	scale = db_getcurrentscale(el_units&INTERNALUNITS, el_units&DISPLAYUNITS, lambda);
	number = ((double)i) / scale;
	(void)esnprintf(cur, 20, x_("%.4f"), number);

	/* remove trailing zeros */
	len = estrlen(cur);
	while (cur[len-1] == '0') cur[--len] = 0;
	if (cur[len-1] == '.') cur[--len] = 0;

	switch (el_units&DISPLAYUNITS)
	{
		case DISPUNITINCH: estrcat(cur, x_("\""));  break;
		case DISPUNITCM:   estrcat(cur, x_("cm"));  break;
		case DISPUNITMM:   estrcat(cur, x_("mm"));  break;
		case DISPUNITMIL:  estrcat(cur, x_("mil")); break;
		case DISPUNITMIC:  estrcat(cur, x_("u"));   break;
		case DISPUNITCMIC: estrcat(cur, x_("cu"));  break;
		case DISPUNITNM:   estrcat(cur, x_("nm"));  break;
	}
	return(cur);
}

/*
 * routine to convert a fractional number to ascii
 */
#define FRTOANUMBUFS 10

CHAR *frtoa(INTBIG i)
{
	static INTBIG latoaswitch = 0;
	static CHAR output[FRTOANUMBUFS][30];
	REGISTER INTBIG fra;
	CHAR temp[3];
	REGISTER CHAR *pp, *cur, *start;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* get output buffer */
	start = cur = &output[latoaswitch++][0];
	if (latoaswitch >= FRTOANUMBUFS) latoaswitch = 0;

	/* handle negative values */
	if (i < 0)
	{
		*cur++ = '-';
		i = -i;
	}

	/* get the part to the left of the decimal point */
	(void)esnprintf(cur, 30, x_("%ld"), i/WHOLE);

	/* see if there is anything to the right of the decimal point */
	if ((i%WHOLE) != 0)
	{
		(void)estrcat(cur, x_("."));
		fra = i % WHOLE;
		fra = (fra*100 + WHOLE/2) / WHOLE;
		(void)esnprintf(temp, 3, x_("%02ld"), fra);
		(void)estrcat(cur, temp);
		pp = cur;   while (*pp != 0) pp++;
		while (*--pp == '0') *pp = 0;
		if (*pp == '.') *pp = 0;
	}
	return(start);
}

/*
 * routine to determine whether or not the string in "pp" is a number.
 * Returns true if it is.
 */
BOOLEAN isanumber(CHAR *pp)
{
	INTBIG xflag, founddigits;

	/* ignore the minus sign */
	if (*pp == '+' || *pp == '-') pp++;

	/* special case for hexadecimal prefix */
	if (*pp == '0' && (pp[1] == 'x' || pp[1] == 'X'))
	{
		pp += 2;
		xflag = 1;
	} else xflag = 0;

	/* there must be something to check */
	if (*pp == 0) return(FALSE);

	founddigits = 0;
	if (xflag != 0)
	{
		while (isxdigit(*pp))
		{
			pp++;
			founddigits = 1;
		}
	} else
	{
		while (isdigit(*pp) || *pp == '.')
		{
			if (*pp != '.') founddigits = 1;
			pp++;
		}
	}
	if (founddigits == 0) return(FALSE);
	if (*pp == 0) return(TRUE);

	/* handle exponent of floating point numbers */
	if (xflag != 0 || founddigits == 0 || (*pp != 'e' && *pp != 'E')) return(FALSE);
	pp++;
	if (*pp == '+' || *pp == '-') pp++;
	if (*pp == 0) return(FALSE);
	while (isdigit(*pp)) pp++;
	if (*pp == 0) return(TRUE);

	return(FALSE);
}

/*
 * routine to convert relative or absolute font values to absolute, in the
 * window "w"
 */
INTBIG truefontsize(INTBIG font, WINDOWPART *w, TECHNOLOGY *tech)
{
	REGISTER INTBIG pixperlam, lambda, height;
	REGISTER LIBRARY *lib;

	/* keep special font codes */
	if (font == TXTEDITOR || font == TXTMENU) return(font);

	/* absolute font sizes are easy */
	if ((font&TXTPOINTS) != 0) return((font&TXTPOINTS) >> TXTPOINTSSH);

	/* detemine default, min, and max size of font */
	if (w->curnodeproto == NONODEPROTO) lib = el_curlib; else
		lib = w->curnodeproto->lib;
	if (tech == NOTECHNOLOGY) tech = el_curtech;
	lambda = lib->lambda[tech->techindex];
	if ((font&TXTQLAMBDA) != 0)
	{
		height = TXTGETQLAMBDA(font);
		height = height * lambda / 4;
		pixperlam = applyyscale(w, height);
		return(pixperlam);
	}
	return(applyyscale(w, lambda));
}

/*
 * routine to set the default text descriptor into "td".  This text will be
 * placed on "geom".
 */
void defaulttextdescript(UINTBIG *descript, GEOM *geom)
{
	REGISTER VARIABLE *txtvar;
	REGISTER INTBIG dx, dy, goleft, goright, goup, godown;
	INTBIG *defdescript;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	static INTBIG user_default_text_style_key = 0, user_default_text_smart_style_key = 0;

	if (user_default_text_style_key == 0)
		user_default_text_style_key = makekey(x_("USER_default_text_style"));
	if (user_default_text_smart_style_key == 0)
		user_default_text_smart_style_key = makekey(x_("USER_default_text_smart_style"));

	txtvar = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, user_default_text_style_key);
	if (txtvar == NOVARIABLE)
	{
		TDSETPOS(descript, VTPOSCENT);
	} else
	{
		defdescript = (INTBIG *)txtvar->addr;
		TDSETPOS(descript, TDGETPOS(defdescript));
		TDSETSIZE(descript, TDGETSIZE(defdescript));
		TDSETFACE(descript, TDGETFACE(defdescript));
		TDSETITALIC(descript, TDGETITALIC(defdescript));
		TDSETBOLD(descript, TDGETBOLD(defdescript));
		TDSETUNDERLINE(descript, TDGETUNDERLINE(defdescript));
	}
	if (geom != NOGEOM)
	{
		/* set text size */
		if (geom->entryisnode)
		{
			ni = geom->entryaddr.ni;
			if (ni->proto == gen_invispinprim)
			{
				defaulttextsize(2, descript);
			} else
			{
				defaulttextsize(3, descript);
			}
		} else
		{
			defaulttextsize(4, descript);
		}

		/* handle smart text placement relative to attached object */
		txtvar = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, user_default_text_smart_style_key);
		if (txtvar != NOVARIABLE)
		{
			/* figure out location of object relative to environment */
			dx = dy = 0;
			if (geom->entryisnode)
			{
				ni = geom->entryaddr.ni;
				if (ni->firstportarcinst != NOPORTARCINST)
				{
					ai = ni->firstportarcinst->conarcinst;
					dx = (ai->end[0].xpos+ai->end[1].xpos)/2 -
						(ni->lowx+ni->highx)/2;
					dy = (ai->end[0].ypos+ai->end[1].ypos)/2 -
						(ni->lowy+ni->highy)/2;
				}
			}

			/* first move placement horizontally */
			goleft = goright = goup = godown = 0;
			if ((txtvar->addr&03) == 1)
			{
				/* place label inside (towards center) */
				if (dx > 0) goright++; else
					if (dx < 0) goleft++;
			} else if ((txtvar->addr&03) == 2)
			{
				/* place label outside (away from center) */
				if (dx > 0) goleft++; else
					if (dx < 0) goright++;
			}

			/* next move placement vertically */
			if (((txtvar->addr>>2)&03) == 1)
			{
				/* place label inside (towards center) */
				if (dy > 0) goup++; else
					if (dy < 0) godown++;
			} else if (((txtvar->addr>>2)&03) == 2)
			{
				/* place label outside (away from center) */
				if (dy > 0) godown++; else
					if (dy < 0) goup++;
			}
			if (goleft != 0)
			{
				switch (TDGETPOS(descript))
				{
					case VTPOSCENT:
					case VTPOSRIGHT:
					case VTPOSLEFT:
						TDSETPOS(descript, VTPOSLEFT);
						break;
					case VTPOSUP:
					case VTPOSUPLEFT:
					case VTPOSUPRIGHT:
						TDSETPOS(descript, VTPOSUPLEFT);
						break;
					case VTPOSDOWN:
					case VTPOSDOWNLEFT:
					case VTPOSDOWNRIGHT:
						TDSETPOS(descript, VTPOSDOWNLEFT);
						break;
				}
			}
			if (goright != 0)
			{
				switch (TDGETPOS(descript))
				{
					case VTPOSCENT:
					case VTPOSRIGHT:
					case VTPOSLEFT:
						TDSETPOS(descript, VTPOSRIGHT);
						break;
					case VTPOSUP:
					case VTPOSUPLEFT:
					case VTPOSUPRIGHT:
						TDSETPOS(descript, VTPOSUPRIGHT);
						break;
					case VTPOSDOWN:
					case VTPOSDOWNLEFT:
					case VTPOSDOWNRIGHT:
						TDSETPOS(descript, VTPOSDOWNRIGHT);
						break;
				}
			}
			if (goup != 0)
			{
				switch (TDGETPOS(descript))
				{
					case VTPOSCENT:
					case VTPOSUP:
					case VTPOSDOWN:
						TDSETPOS(descript, VTPOSUP);
						break;
					case VTPOSRIGHT:
					case VTPOSUPRIGHT:
					case VTPOSDOWNRIGHT:
						TDSETPOS(descript, VTPOSUPRIGHT);
						break;
					case VTPOSLEFT:
					case VTPOSUPLEFT:
					case VTPOSDOWNLEFT:
						TDSETPOS(descript, VTPOSUPLEFT);
						break;
				}
			}
			if (godown != 0)
			{
				switch (TDGETPOS(descript))
				{
					case VTPOSCENT:
					case VTPOSUP:
					case VTPOSDOWN:
						TDSETPOS(descript, VTPOSDOWN);
						break;
					case VTPOSRIGHT:
					case VTPOSUPRIGHT:
					case VTPOSDOWNRIGHT:
						TDSETPOS(descript, VTPOSDOWNRIGHT);
						break;
					case VTPOSLEFT:
					case VTPOSUPLEFT:
					case VTPOSDOWNLEFT:
						TDSETPOS(descript, VTPOSDOWNLEFT);
						break;
				}
			}
		}
	}
}

#define DEFNODETEXTSIZE      TXTSETQLAMBDA(4)
#define DEFARCTEXTSIZE       TXTSETQLAMBDA(4)
#define DEFEXPORTSIZE        TXTSETQLAMBDA(8)
#define DEFNONLAYOUTTEXTSIZE TXTSETQLAMBDA(4)
#define DEFINSTTEXTSIZE      TXTSETQLAMBDA(16)
#define DEFCELLTEXTSIZE      TXTSETQLAMBDA(4)

/*
 * Routine to determine the size of text to use for a particular type of text:
 * 3:  node name
 * 4:  arc name
 * 1:  export label
 * 2:  nonlayout text (text on an invisible pin, cell variables)
 * 5:  cell instance name
 * 6:  cell text
 */
void defaulttextsize(INTBIG texttype, UINTBIG *descript)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i;
	static INTBIG node_size_key = 0, arc_size_key = 0, export_size_key = 0,
		nonlayout_size_key = 0, instance_size_key = 0, cell_size_key = 0;

	if (node_size_key == 0)
		node_size_key = makekey(x_("USER_default_node_text_size"));
	if (arc_size_key == 0)
		arc_size_key = makekey(x_("USER_default_arc_text_size"));
	if (export_size_key == 0)
		export_size_key = makekey(x_("USER_default_export_text_size"));
	if (nonlayout_size_key == 0)
		nonlayout_size_key = makekey(x_("USER_default_nonlayout_text_size"));
	if (instance_size_key == 0)
		instance_size_key = makekey(x_("USER_default_instance_text_size"));
	if (cell_size_key == 0)
		cell_size_key = makekey(x_("USER_default_facet_text_size"));

	for(i=0; i<TEXTDESCRIPTSIZE; i++) descript[i] = 0;
	TDSETSIZE(descript, TXTSETQLAMBDA(4));
	switch (texttype)
	{
		case 3:
			var = getvalkey((INTBIG)us_tool, VTOOL, -1, node_size_key);
			if (var == NOVARIABLE) TDSETSIZE(descript, DEFNODETEXTSIZE); else
			{
				if ((var->type&VISARRAY) != 0) TDCOPY(descript, (UINTBIG *)var->addr); else
					TDSETSIZE(descript, var->addr);
			}
			break;
		case 4:
			var = getvalkey((INTBIG)us_tool, VTOOL, -1, arc_size_key);
			if (var == NOVARIABLE) TDSETSIZE(descript, DEFARCTEXTSIZE); else
			{
				if ((var->type&VISARRAY) != 0) TDCOPY(descript, (UINTBIG *)var->addr); else
					TDSETSIZE(descript, var->addr);
			}
			break;
		case 1:
			var = getvalkey((INTBIG)us_tool, VTOOL, -1, export_size_key);
			if (var == NOVARIABLE) TDSETSIZE(descript, DEFEXPORTSIZE); else
			{
				if ((var->type&VISARRAY) != 0) TDCOPY(descript, (UINTBIG *)var->addr); else
					TDSETSIZE(descript, var->addr);
			}				
			break;
		case 2:
			var = getvalkey((INTBIG)us_tool, VTOOL, -1, nonlayout_size_key);
			if (var == NOVARIABLE) TDSETSIZE(descript, DEFNONLAYOUTTEXTSIZE); else
			{
				if ((var->type&VISARRAY) != 0) TDCOPY(descript, (UINTBIG *)var->addr); else
					TDSETSIZE(descript, var->addr);
			}
			break;
		case 5:
			var = getvalkey((INTBIG)us_tool, VTOOL, -1, instance_size_key);
			if (var == NOVARIABLE) TDSETSIZE(descript, DEFINSTTEXTSIZE); else
			{
				if ((var->type&VISARRAY) != 0) TDCOPY(descript, (UINTBIG *)var->addr); else
					TDSETSIZE(descript, var->addr);
			}
			break;
		case 6:
			var = getvalkey((INTBIG)us_tool, VTOOL, -1, cell_size_key);
			if (var == NOVARIABLE) TDSETSIZE(descript, DEFCELLTEXTSIZE); else
			{
				if ((var->type&VISARRAY) != 0) TDCOPY(descript, (UINTBIG *)var->addr); else
					TDSETSIZE(descript, var->addr);
			}
			break;
	}
}

/*
 * Routine to get the X offset field of text descriptor words "td".  This routine
 * implements the macro "TDGETXOFF".
 */
INTBIG gettdxoffset(UINTBIG *td)
{
	REGISTER INTBIG offset, scale;

	offset = (td[0] & VTXOFF) >> VTXOFFSH;
	if ((td[0]&VTXOFFNEG) != 0) offset = -offset;
	scale = TDGETOFFSCALE(td) + 1;
	return(offset * scale);
}

/*
 * Routine to get the Y offset field of text descriptor words "td".  This routine
 * implements the macro "TDGETYOFF".
 */
INTBIG gettdyoffset(UINTBIG *td)
{
	REGISTER INTBIG offset, scale;

	offset = (td[0] & VTYOFF) >> VTYOFFSH;
	if ((td[0]&VTYOFFNEG) != 0) offset = -offset;
	scale = TDGETOFFSCALE(td) + 1;
	return(offset * scale);
}

/*
 * Routine to set the X offset field of text descriptor words "td" to "value".
 * This routine implements the macro "TDSETOFF".
 */
void settdoffset(UINTBIG *td, INTBIG x, INTBIG y)
{
	REGISTER INTBIG scale;

	td[0] = td[0] & ~(VTXOFF|VTYOFF|VTXOFFNEG|VTYOFFNEG);
	if (x < 0)
	{
		x = -x;
		td[0] |= VTXOFFNEG;
	}
	if (y < 0)
	{
		y = -y;
		td[0] |= VTYOFFNEG;
	}
	scale = maxi(x,y) >> VTOFFMASKWID;
	x /= (scale + 1);
	y /= (scale + 1);
	td[0] |= (x << VTXOFFSH);
	td[0] |= (y << VTYOFFSH);
	TDSETOFFSCALE(td, scale);
}

/*
 * Routine to validate the variable offset (x,y) and clip or grid it
 * to acceptable values.
 */
void propervaroffset(INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG scale, realx, realy, negx, negy;

	negx = negy = 1;
	realx = *x;   realy = *y;
	if (realx < 0) { realx = -realx;   negx = -1; }
	if (realy < 0) { realy = -realy;   negy = -1; }
	if (realx > 077777) realx = 077777;
	if (realy > 077777) realy = 077777;
	scale = (maxi(realx, realy) >> VTOFFMASKWID) + 1;
	realx /= scale;
	realy /= scale;
	*x = realx * scale * negx;
	*y = realy * scale * negy;
}

/*
 * Routine to clear the text descriptor array in "t".
 * This routine implements the macro "TDCLEAR".
 */
void tdclear(UINTBIG *t)
{
	REGISTER INTBIG i;

	for(i=0; i<TEXTDESCRIPTSIZE; i++) t[i] = 0;
}

/*
 * Routine to copy the text descriptor array in "s" to "d".
 * This routine implements the macro "TDCOPY".
 */
void tdcopy(UINTBIG *d, UINTBIG *s)
{
	REGISTER INTBIG i;

	for(i=0; i<TEXTDESCRIPTSIZE; i++) d[i] = s[i];
}

/*
 * Routine to compare the text descriptor arrays in "t1" to "t2"
 * and return true if they are different.
 * This routine implements the macro "TDDIFF".
 */
BOOLEAN tddiff(UINTBIG *t1, UINTBIG *t2)
{
	REGISTER INTBIG i;

	for(i=0; i<TEXTDESCRIPTSIZE; i++)
		if (t1[i] != t2[i]) return(TRUE);
	return(FALSE);
}

/*
 * routine to make a printable string from variable "val", array index
 * "aindex".  If "aindex" is negative, print the entire array.  If "purpose" is
 * zero, the conversion is for human reading and should be easy to understand.
 * If "purpose" is positive, the conversion is for machine reading and should
 * be easy to parse.  If "purpose" is negative, the conversion is for
 * parameter substitution and should be easy to understand but not hard to
 * parse (a combination of the two).  If the variable is displayable and the name
 * is to be displayed, that part is added.
 */
CHAR *describedisplayedvariable(VARIABLE *var, INTBIG aindex, INTBIG purpose)
{
	REGISTER CHAR *name, *parstr;
	REGISTER INTBIG len;
	REGISTER VARIABLE *parval;
	REGISTER void *infstr;

	if (var == NOVARIABLE)
	{
		if (purpose <= 0) return(x_("***UNKNOWN***")); else return(x_(""));
	}

	infstr = initinfstr();

	/* add variable name if requested */
	if ((var->type&VDISPLAY) != 0)
	{
		if ((var->type&VISARRAY) == 0) len = 1; else
			len = getlength(var);
		name = truevariablename(var);
		switch (TDGETDISPPART(var->textdescript))
		{
			case VTDISPLAYNAMEVALUE:
				if (len > 1 && aindex >= 0)
				{
					formatinfstr(infstr, x_("%s[%ld]="), name, aindex);
				} else
				{
					formatinfstr(infstr, x_("%s="), name);
				}
				break;
			case VTDISPLAYNAMEVALINH:
				/* setvariablewindow(el_curwindowpart); */
				parval = getparentvalkey(var->key, 1);
				/* setvariablewindow(NOWINDOWPART); */
				if (parval == NOVARIABLE) parstr = x_("?"); else
					parstr = describevariable(parval, -1, purpose);
				if (len > 1 && aindex >= 0)
				{
					formatinfstr(infstr, x_("%s[%ld]=%s;def="), name, aindex, parstr);
				} else
				{
					formatinfstr(infstr, x_("%s=%s;def="), name, parstr);
				}
				break;
			case VTDISPLAYNAMEVALINHALL:
				/* setvariablewindow(el_curwindowpart); */
				parval = getparentvalkey(var->key, 0);
				/* setvariablewindow(NOWINDOWPART); */
				if (parval == NOVARIABLE) parstr = x_("?"); else
					parstr = describevariable(parval, -1, purpose);
				if (len > 1 && aindex >= 0)
				{
					formatinfstr(infstr, x_("%s[%ld]=%s;def="), name, aindex, parstr);
				} else
				{
					formatinfstr(infstr, x_("%s=%s;def="), name, parstr);
				}
				break;
		}
	}
	addstringtoinfstr(infstr, describevariable(var, aindex, purpose));
	return(returninfstr(infstr));
}

/*
 * Routine to return the true name of variable "var".  Leading "ATTR_" or "ATTRP_"
 * markers are removed.
 */
CHAR *truevariablename(VARIABLE *var)
{
	REGISTER CHAR *name;
	REGISTER INTBIG i, len;

	name = makename(var->key);
	if (estrncmp(name, x_("ATTR_"), 5) == 0)
		return(name + 5);
	if (estrncmp(name, x_("ATTRP_"), 6) == 0)
	{
		len = estrlen(name);
		for(i=len-1; i>=0; i--) if (name[i] == '_') break;
		return(name + i);
	}
	return(name);
}

/*
 * routine to make a printable string from variable "val", array index
 * "aindex".  If "aindex" is negative, print the entire array.  If "purpose" is
 * zero, the conversion is for human reading and should be easy to understand.
 * If "purpose" is positive, the conversion is for machine reading and should
 * be easy to parse.  If "purpose" is negative, the conversion is for
 * parameter substitution and should be easy to understand but not hard to
 * parse (a combination of the two).
 */
CHAR *describevariable(VARIABLE *var, INTBIG aindex, INTBIG purpose)
{
	REGISTER INTBIG i, len, *addr, units;
	REGISTER void *infstr;

	if (var == NOVARIABLE)
	{
		if (purpose <= 0) return(x_("***UNKNOWN***")); else return(x_(""));
	}

	infstr = initinfstr();

	units = TDGETUNITS(var->textdescript);
	if ((var->type & (VCODE1|VCODE2)) != 0)
	{
		/* special case for code: it is a string, the type applies to the result */
		db_makestringvar(VSTRING, var->addr, purpose, units, infstr);
	} else
	{
		if ((var->type&VISARRAY) != 0)
		{
			/* compute the array length */
			len = getlength(var);
			addr = (INTBIG *)var->addr;

			/* if asking for a single entry, get it */
			if (aindex >= 0)
			{
				/* special case when the variable is a general array of objects */
				if ((var->type&VTYPE) == VGENERAL)
				{
					/* index the array in pairs */
					aindex *= 2;
					if (aindex < len)
						db_makestringvar(addr[aindex+1], addr[aindex], purpose, units, infstr);
				} else
				{
					/* normal array indexing */
					if (aindex < len)
						switch ((var->type&VTYPE))
					{
						case VCHAR:
							db_makestringvar(var->type,
								((INTBIG)((CHAR *)addr)[aindex]), purpose, units, infstr);
							break;

						case VDOUBLE:
							db_makestringvar(var->type,
								((INTBIG)((double *)addr)[aindex]), purpose, units, infstr);
							break;

						case VSHORT:
							db_makestringvar(var->type,
								((INTBIG)((INTSML *)addr)[aindex]), purpose, units, infstr);
							break;

						default:
							db_makestringvar(var->type, addr[aindex], purpose, units, infstr);
							break;
					}
				}
			} else
			{
				/* in an array, quote strings */
				if (purpose < 0) purpose = 0;
				addtoinfstr(infstr, '[');
				for(i=0; i<len; i++)
				{
					if (i != 0) addtoinfstr(infstr, ',');

					/* special case when the variable is a general array of objects */
					if ((var->type&VTYPE) == VGENERAL)
					{
						/* index the array in pairs */
						if (i+1 < len)
							db_makestringvar(addr[i+1], addr[i], purpose, units, infstr);
						i++;
					} else
					{
						/* normal array indexing */
						switch ((var->type&VTYPE))
						{
							case VCHAR:
								db_makestringvar(var->type,
									((INTBIG)((CHAR *)addr)[i]), purpose, units, infstr);
								break;

							case VDOUBLE:
								db_makestringvar(var->type,
									((INTBIG)((double *)addr)[i]), purpose, units, infstr);
								break;

							case VSHORT:
								db_makestringvar(var->type,
									((INTBIG)((INTSML *)addr)[i]), purpose, units, infstr);
								break;

							default:
								db_makestringvar(var->type, addr[i], purpose, units, infstr);
								break;
						}
					}
				}
				addtoinfstr(infstr, ']');
			}
		} else db_makestringvar(var->type, var->addr, purpose, units, infstr);
	}
	return(returninfstr(infstr));
}

/*
 * routine to make a string from the value in "addr" which has a type in
 * "type".  "purpose" is an indicator of the purpose of this conversion:
 * zero indicates conversion for humans to read, positive indicates
 * conversion for a program to read (more terse) and negative indicates human
 * reading for parameter substitution (don't quote strings).  Returns true
 * if there is a memory allocation error.
 */
void db_makestringvar(INTBIG type, INTBIG addr, INTBIG purpose, INTBIG units, void *infstr)
{
	CHAR line[100];
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
	REGISTER NETWORK *net;
	REGISTER VIEW *view;
	REGISTER WINDOWPART *win;
	REGISTER GRAPHICS *gra;
	REGISTER VARIABLE *var;
	REGISTER CONSTRAINT *con;
	REGISTER WINDOWFRAME *wf;
	REGISTER POLYGON *poly;

	switch (type&VTYPE)
	{
		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
			if (units == 0)
			{
				(void)esnprintf(line, 100, x_("%ld"), addr);
				addstringtoinfstr(infstr, line);
				return;
			}
			addstringtoinfstr(infstr, db_makeunits((float)addr, units));
			return;
		case VADDRESS:
			if (purpose == 0) addstringtoinfstr(infstr, x_("Address="));
			(void)esnprintf(line, 100, x_("0%lo"), addr);
			addstringtoinfstr(infstr, line);
			return;
		case VCHAR:
			addtoinfstr(infstr, (CHAR)addr);
			return;
		case VSTRING:
			if ((CHAR *)addr == NOSTRING || (CHAR *)addr == 0)
			{
				addstringtoinfstr(infstr, x_("***NOSTRING***"));
				return;
			}
			if (purpose >= 0) addtoinfstr(infstr, '"');
			if (purpose <= 0)  addstringtoinfstr(infstr, (CHAR *)addr); else
				db_addstring((CHAR *)addr, infstr);
			if (purpose >= 0)addtoinfstr(infstr, '"');
			return;
		case VFLOAT:
		case VDOUBLE:
			if (units == 0)
			{
				(void)esnprintf(line, 100, x_("%g"), castfloat(addr));
				addstringtoinfstr(infstr, line);
				return;
			}
			addstringtoinfstr(infstr, db_makeunits(castfloat(addr), units));
			return;
		case VNODEINST:
			ni = (NODEINST *)addr;
			if (ni == NONODEINST)
			{
				addstringtoinfstr(infstr, x_("***NONODEINST***"));
				return;
			}
			if (purpose == 0)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
				if (var != NOVARIABLE)
				{
					addstringtoinfstr(infstr, (CHAR *)var->addr);
					return;
				}
				(void)esnprintf(line, 100, x_("node%ld"), (INTBIG)ni);
				addstringtoinfstr(infstr, line);
				return;
			}
			(void)esnprintf(line, 100, x_("node%ld"), (INTBIG)ni);
			addstringtoinfstr(infstr, line);
			return;
		case VNODEPROTO:
			np = (NODEPROTO *)addr;
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, describenodeproto(np));
				return;
			}
			db_addstring(describenodeproto(np), infstr);
			return;
		case VPORTARCINST:
			pi = (PORTARCINST *)addr;
			if (pi == NOPORTARCINST)
			{
				addstringtoinfstr(infstr, x_("***NOPORTARCINST***"));
				return;
			}
			(void)esnprintf(line, 100, x_("portarc%ld"), (INTBIG)pi);
			addstringtoinfstr(infstr, line);
			return;
		case VPORTEXPINST:
			pe = (PORTEXPINST *)addr;
			if (pe == NOPORTEXPINST)
			{
				addstringtoinfstr(infstr, x_("***NOPORTEXPINST***"));
				return;
			}
			(void)esnprintf(line, 100, x_("portexp%ld"), (INTBIG)pe);
			addstringtoinfstr(infstr, line);
			return;
		case VPORTPROTO:
			pp = (PORTPROTO *)addr;
			if (pp == NOPORTPROTO)
			{
				addstringtoinfstr(infstr, x_("***NOPORTPROTO***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, pp->protoname);
				return;
			}
			db_addstring(pp->protoname, infstr);
			return;
		case VARCINST:
			ai = (ARCINST *)addr;
			if (ai == NOARCINST)
			{
				addstringtoinfstr(infstr, x_("***NOARCINST***"));
				return;
			}
			(void)esnprintf(line, 100, x_("arc%ld"), (INTBIG)ai);
			addstringtoinfstr(infstr, line);
			return;
		case VARCPROTO:
			ap = (ARCPROTO *)addr;
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, describearcproto(ap));
				return;
			}
			db_addstring(describearcproto(ap), infstr);
			return;
		case VGEOM:
			geom = (GEOM *)addr;
			if (geom == NOGEOM)
			{
				addstringtoinfstr(infstr, x_("***NOGEOM***"));
				return;
			}
			(void)esnprintf(line, 100, x_("geom%ld"), (INTBIG)geom);
			addstringtoinfstr(infstr, line);
			return;
		case VLIBRARY:
			lib = (LIBRARY *)addr;
			if (lib == NOLIBRARY)
			{
				addstringtoinfstr(infstr, x_("***NOLIBRARY***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, lib->libname);
				return;
			}
			db_addstring(lib->libname, infstr);
			return;
		case VTECHNOLOGY:
			tech = (TECHNOLOGY *)addr;
			if (tech == NOTECHNOLOGY)
			{
				addstringtoinfstr(infstr, x_("***NOTECHNOLOGY***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, tech->techname);
				return;
			}
			db_addstring(tech->techname, infstr);
			return;
		case VTOOL:
			tool = (TOOL *)addr;
			if (tool == NOTOOL)
			{
				addstringtoinfstr(infstr, x_("***NOTOOL***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, tool->toolname);
				return;
			}
			db_addstring(tool->toolname, infstr);
			return;
		case VRTNODE:
			rtn = (RTNODE *)addr;
			if (rtn == NORTNODE)
			{
				addstringtoinfstr(infstr, x_("***NORTNODE***"));
				return;
			}
			(void)esnprintf(line, 100, x_("rtn%ld"), (INTBIG)rtn);
			addstringtoinfstr(infstr, line);
			return;
		case VFRACT:
			addstringtoinfstr(infstr, frtoa(addr));
			return;
		case VNETWORK:
			net = (NETWORK *)addr;
			if (net == NONETWORK)
			{
				addstringtoinfstr(infstr, x_("***NONETWORK***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, describenetwork(net));
				return;
			}
			db_addstring(describenetwork(net), infstr);
			return;
		case VVIEW:
			view = (VIEW *)addr;
			if (view == NOVIEW)
			{
				addstringtoinfstr(infstr, x_("***NOVIEW***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, view->viewname);
				return;
			}
			db_addstring(view->viewname, infstr);
			return;
		case VWINDOWPART:
			win = (WINDOWPART *)addr;
			if (win == NOWINDOWPART)
			{
				addstringtoinfstr(infstr, x_("***NOWINDOWPART***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, win->location);
				return;
			}
			db_addstring(win->location, infstr);
			return;
		case VGRAPHICS:
			gra = (GRAPHICS *)addr;
			if (gra == NOGRAPHICS)
			{
				addstringtoinfstr(infstr, x_("***NOGRAPHICS***"));
				return;
			}
			(void)esnprintf(line, 100, x_("gra%ld"), (INTBIG)gra);
			addstringtoinfstr(infstr, line);
			return;
		case VCONSTRAINT:
			con = (CONSTRAINT *)addr;
			if (con == NOCONSTRAINT)
			{
				addstringtoinfstr(infstr, x_("***NOCONSTRAINT***"));
				return;
			}
			if (purpose <= 0)
			{
				addstringtoinfstr(infstr, con->conname);
				return;
			}
			db_addstring(con->conname, infstr);
			return;
		case VGENERAL:
			(void)esnprintf(line, 100, x_("***%ld-LONG-GENERAL-ARRAY***"),
				((type&VLENGTH)>>VLENGTHSH) / 2);
			addstringtoinfstr(infstr, line);
			return;
		case VWINDOWFRAME:
			wf = (WINDOWFRAME *)addr;
			if (wf == NOWINDOWFRAME)
			{
				addstringtoinfstr(infstr, x_("***NOWINDOWFRAME***"));
				return;
			}
			(void)esnprintf(line, 100, x_("wf%ld"), (INTBIG)wf);
			addstringtoinfstr(infstr, line);
			return;
		case VPOLYGON:
			poly = (POLYGON *)addr;
			if (poly == NOPOLYGON)
			{
				addstringtoinfstr(infstr, x_("***NOPOLYGON***"));
				return;
			}
			(void)esnprintf(line, 100, x_("poly%ld"), (INTBIG)poly);
			addstringtoinfstr(infstr, line);
			return;
	}
}

CHAR *db_makeunits(float value, INTBIG units)
{
	static CHAR line[100];

	switch (units)
	{
		case VTUNITSRES:
			return(displayedunits(value, units,
				(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH));
		case VTUNITSCAP:
			return(displayedunits(value, units,
				(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH));
		case VTUNITSIND:
			return(displayedunits(value, units,
				(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH));
		case VTUNITSCUR:
			return(displayedunits(value, units,
				(us_electricalunits&INTERNALCURUNITS) >> INTERNALCURUNITSSH));
		case VTUNITSVOLT:
			return(displayedunits(value, units,
				(us_electricalunits&INTERNALVOLTUNITS) >> INTERNALVOLTUNITSSH));
		case VTUNITSTIME:
			return(displayedunits(value, units,
				(us_electricalunits&INTERNALTIMEUNITS) >> INTERNALTIMEUNITSSH));
	}
	esnprintf(line, 100, x_("%g"), value);
	return(line);
}

/*
 * routine to add the string "str" to the infinite string and to quote the
 * special characters '[', ']', '"', and '^'.  The routine returns nonzero
 * if there is memory problem with the infinite string package.
 */
void db_addstring(CHAR *str, void *infstr)
{
	while (*str != 0)
	{
		if (*str == '[' || *str == ']' || *str == '"' || *str == '^')
			addtoinfstr(infstr, '^');
		addtoinfstr(infstr, *str++);
	}
}

/*
 * Routine to describe a simple variable "var".  There can be no arrays, code,
 * or pointers to Electric objects in this variable.
 */
CHAR *describesimplevariable(VARIABLE *var)
{
	static CHAR line[50];

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	switch (var->type&VTYPE)
	{
		case VSTRING:
			return((CHAR *)var->addr);
		case VINTEGER:
			esnprintf(line, 50, x_("%ld"), var->addr);
			return(line);
		case VFRACT:
			estrcpy(line, frtoa(var->addr));
			return(line);
		case VSHORT:
			esnprintf(line, 50, x_("%ld"), var->addr&0xFFFF);
			return(line);
		case VBOOLEAN:
			esnprintf(line, 50, x_("%ld"), var->addr&0xFF);
			return(line);
		case VFLOAT:
		case VDOUBLE:
			esnprintf(line, 50, x_("%g"), castfloat(var->addr));
			return(line);
	}
	return(x_(""));
}

/*
 * Routine to determine the variable type of "pt"; either an integer, float,
 * or string; and return the type and value in "type" and "addr".  If "units"
 * is nonzero, this value is converted into that type of unit.
 */
void getsimpletype(CHAR *pt, INTBIG *type, INTBIG *addr, INTBIG units)
{
	REGISTER CHAR *opt;
	REGISTER INTBIG i;
	float f;

	if (!isanumber(pt))
	{
		*type = VSTRING;
		*addr = (INTBIG)pt;
		return;
	}
	switch (units)
	{
		case VTUNITSRES:
			*addr = castint(figureunits(pt, VTUNITSRES,
				(us_electricalunits&INTERNALRESUNITS) >> INTERNALRESUNITSSH));
			*type = VFLOAT;
			return;
		case VTUNITSCAP:
			*addr = castint(figureunits(pt, VTUNITSCAP,
				(us_electricalunits&INTERNALCAPUNITS) >> INTERNALCAPUNITSSH));
			*type = VFLOAT;
			return;
		case VTUNITSIND:
			*addr = castint(figureunits(pt, VTUNITSIND,
				(us_electricalunits&INTERNALINDUNITS) >> INTERNALINDUNITSSH));
			*type = VFLOAT;
			return;
		case VTUNITSCUR:
			*addr = castint(figureunits(pt, VTUNITSCUR,
				(us_electricalunits&INTERNALCURUNITS) >> INTERNALCURUNITSSH));
			*type = VFLOAT;
			return;
		case VTUNITSVOLT:
			*addr = castint(figureunits(pt, VTUNITSVOLT,
				(us_electricalunits&INTERNALVOLTUNITS) >> INTERNALVOLTUNITSSH));
			*type = VFLOAT;
			return;
		case VTUNITSTIME:
			*addr = castint(figureunits(pt, VTUNITSTIME,
				(us_electricalunits&INTERNALTIMEUNITS) >> INTERNALTIMEUNITSSH));
			*type = VFLOAT;
			return;
	}

	*type = VINTEGER;
	*addr = eatoi(pt);
	for(opt = pt; *opt != 0; opt++) if (*opt == '.' || *opt == 'e' || *opt == 'E')
	{
		f = (float)eatof(pt);
		i = (INTBIG)(f * WHOLE);
		if (i / WHOLE == f)
		{
			*type = VFRACT;
			*addr = i;
		} else
		{
			*type = VFLOAT;
			*addr = castint(f);
		}
		break;
	}
}

/*
 * routine to make an abbreviation for the string "pt" in upper case if
 * "upper" is true
 */
CHAR *makeabbrev(CHAR *pt, BOOLEAN upper)
{
	REGISTER void *infstr;

	/* generate an abbreviated name for this prototype */
	infstr = initinfstr();
	while (*pt != 0)
	{
		if (isalpha(*pt))
		{
			if (isupper(*pt))
			{
				if (upper) addtoinfstr(infstr, *pt); else
					addtoinfstr(infstr, (CHAR)(tolower(*pt)));
			} else
			{
				if (!upper) addtoinfstr(infstr, *pt); else
					addtoinfstr(infstr, (CHAR)(toupper(*pt)));
			}
			while (isalpha(*pt)) pt++;
		}
		while (!isalpha(*pt) && *pt != 0) pt++;
	}
	return(returninfstr(infstr));
}

/*
 * routine to report the name of the internal unit in "units".
 */
CHAR *unitsname(INTBIG units)
{
	switch (units & INTERNALUNITS)
	{
		case INTUNITHNM:   return(_("half-nanometer"));
		case INTUNITHDMIC: return(_("half-decimicron"));
	}
	return(x_("?"));
}

/************************* INTERNATIONALIZATION *************************/

CHAR *db_stoppingreason[] =
{
	N_("Contour gather"),		/* STOPREASONCONTOUR */
	N_("DRC"),					/* STOPREASONDRC */
	N_("Playback"),				/* STOPREASONPLAYBACK */
	N_("Binary"),				/* STOPREASONBINARY */
	N_("CIF"),					/* STOPREASONCIF */
	N_("DXF"),					/* STOPREASONDXF */
	N_("EDIF"),					/* STOPREASONEDIF */
	N_("VHDL"),					/* STOPREASONVHDL */
	N_("Compaction"),			/* STOPREASONCOMPACT */
	N_("ERC"),					/* STOPREASONERC */
	N_("Check-in"),				/* STOPREASONCHECKIN */
	N_("Lock wait"),			/* STOPREASONLOCK */
	N_("Network comparison"),	/* STOPREASONNCC */
	N_("Port exploration"),		/* STOPREASONPORT */
	N_("Routing"),				/* STOPREASONROUTING */
	N_("Silicon compiler"),		/* STOPREASONSILCOMP */
	N_("Display"),				/* STOPREASONDISPLAY */
	N_("Simulation"),			/* STOPREASONSIMULATE */
	N_("Deck generation"),		/* STOPREASONDECK */
	N_("SPICE"),				/* STOPREASONSPICE */
	N_("Check"),				/* STOPREASONCHECK */
	N_("Array"),				/* STOPREASONARRAY */
	N_("Iteration"),			/* STOPREASONITERATE */
	N_("Replace"),				/* STOPREASONREPLACE */
	N_("Spread"),				/* STOPREASONSPREAD */
	N_("Execution"),			/* STOPREASONEXECUTE */
	N_("Command file"),			/* STOPREASONCOMFILE */
	N_("Selection"),			/* STOPREASONSELECT */
	N_("Tracking"),				/* STOPREASONTRACK */
	N_("Network evaluation"),	/* STOPREASONNETWORK */
	0
};

/*
 * Routine to translate internal strings in the database.
 */
void db_inittranslation(void)
{
	REGISTER INTBIG i;

	/* pretranslate the reasons for stopping */
	for(i=0; db_stoppingreason[i] != 0; i++)
		db_stoppingreason[i] = TRANSLATE(db_stoppingreason[i]);
}

/*
 * Routine to ensure that dialog "dia" is translated.
 */
void DiaTranslate(DIALOG *dia)
{
	REGISTER INTBIG j;
	DIALOGITEM *item;

	if (dia->translated != 0) return;
	dia->translated = 1;
	if (dia->movable != 0) dia->movable = TRANSLATE(dia->movable);
	for(j=0; j<dia->items; j++)
	{
		item = &dia->list[j];
		switch (item->type&ITEMTYPE)
		{
			case BUTTON:
			case DEFBUTTON:
			case CHECK:
			case RADIO:
			case MESSAGE:
			case EDITTEXT:
				if (*item->msg != 0)
					item->msg = TRANSLATE(item->msg);
				break;
		}
	}
}

/*
 * routine to convert "word" to its plural form, depending on the value
 * of "amount" (if "amount" is zero or multiple, pluralize the word).
 */
CHAR *makeplural(CHAR *word, INTBIG amount)
{
	INTBIG needed, len;

	if (amount == 1) return(word);
	len = estrlen(word);
	if (len == 0) return(word);

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	needed = len + 2;
	if (needed > db_pluralbufsize)
	{
		if (db_pluralbufsize > 0) efree(db_pluralbuffer);
		db_pluralbufsize = 0;
		db_pluralbuffer = (CHAR *)emalloc(needed * SIZEOFCHAR, db_cluster);
		if (db_pluralbuffer == 0) return(word);
		db_pluralbufsize = needed;
	}
	estrcpy(db_pluralbuffer, word);
	if (isupper(word[len-1])) estrcat(db_pluralbuffer, x_("S")); else
		estrcat(db_pluralbuffer, x_("s"));
	return(db_pluralbuffer);
}

/************************* STRING MANIPULATION *************************/

/*
 * this routine dynamically allocates a string to hold "str" and places
 * that string at "*addr".  The routine returns true if the allocation
 * cannot be done.  Memory is allocated from virtual cluster "cluster".
 */
#ifdef DEBUGMEMORY
BOOLEAN _allocstring(CHAR **addr, CHAR *str, CLUSTER *cluster, CHAR *module, INTBIG line)
{
	*addr = (CHAR *)_emalloc((estrlen(str)+1) * SIZEOFCHAR, cluster, module, line);
	if (*addr == 0)
	{
		*addr = NOSTRING;
		(void)db_error(DBNOMEM|DBALLOCSTRING);
		return(TRUE);
	}
	(void)estrcpy(*addr, str);
	return(FALSE);
}
#else
BOOLEAN allocstring(CHAR **addr, CHAR *str, CLUSTER *cluster)
{
	*addr = (CHAR *)emalloc((estrlen(str)+1) * SIZEOFCHAR, cluster);
	if (*addr == 0)
	{
		*addr = NOSTRING;
		(void)db_error(DBNOMEM|DBALLOCSTRING);
		return(TRUE);
	}
	(void)estrcpy(*addr, str);
	return(FALSE);
}
#endif

/*
 * this routine assumes that there is a dynamically allocated string at
 * "*addr" and that it is to be replaced with another dynamically allocated
 * string from "str".  The routine returns true if the allocation
 * cannot be done.  Memory is allocated from virtual cluster "cluster".
 */
#ifdef DEBUGMEMORY
BOOLEAN _reallocstring(CHAR **addr, CHAR *str, CLUSTER *cluster, CHAR *module, INTBIG line)
{
	efree(*addr);
	return(_allocstring(addr, str, cluster, module, line));
}
#else
BOOLEAN reallocstring(CHAR **addr, CHAR *str, CLUSTER *cluster)
{
	efree(*addr);
	return(allocstring(addr, str, cluster));
}
#endif

/*
 * Routine to convert character "c" to a case-insensitive character, with
 * special characters all ordered properly before the letters.
 */
void db_initnamesame(void)
{
	REGISTER INTBIG i;

	db_namesamefirst = FALSE;
	for(i=0; i<256; i++)
	{
		db_namesametable[i] = (CHAR)i;
		if (isupper(i)) db_namesametable[i] = tolower(i);
		db_namesametable['[']  = 1;
		db_namesametable['\\'] = 2;
		db_namesametable[']']  = 3;
		db_namesametable['^']  = 4;
		db_namesametable['_']  = 5;
		db_namesametable['`']  = 6;
		db_namesametable['{']  = 7;
		db_namesametable['|']  = 8;
		db_namesametable['}']  = 9;
		db_namesametable['~']  = 10;
	}
}

/*
 * name matching routine: ignores case
 */
INTBIG namesame(CHAR *pt1, CHAR *pt2)
{
	REGISTER INTBIG c1, c2;

	if (db_namesamefirst) db_initnamesame();
	for(;;)
	{
#ifdef _UNICODE
		c1 = *pt1++;   c2 = *pt2++;
		if (c1 < 256) c1 = db_namesametable[c1];
		if (c2 < 256) c2 = db_namesametable[c2];
#else
		c1 = db_namesametable[*pt1++ & 0377];
		c2 = db_namesametable[*pt2++ & 0377];
#endif
		if (c1 != c2) return(c1-c2);
		if (c1 == 0) break;
	}
	return(0);
}

INTBIG namesamen(CHAR *pt1, CHAR *pt2, INTBIG count)
{
	REGISTER INTBIG c1, c2, i;

	if (db_namesamefirst) db_initnamesame();
	for(i=0; i<count; i++)
	{
#ifdef _UNICODE
		c1 = *pt1++;   c2 = *pt2++;
		if (c1 < 256) c1 = db_namesametable[c1];
		if (c2 < 256) c2 = db_namesametable[c2];
#else
		c1 = db_namesametable[*pt1++ & 0377];
		c2 = db_namesametable[*pt2++ & 0377];
#endif
		if (c1 != c2) return(c1-c2);
		if (c1 == 0) break;
	}
	return(0);
}

/*
 * Routine to compare two names "name1" and "name2" and return an
 * integer giving their sorting order (0 if equal, nonzero according
 * to order) in the same manner as any other string compare EXCEPT
 * that it considers numbers properly, so that the string "in10" comes
 * after the string "in9".
 */
INTBIG namesamenumeric(CHAR *name1, CHAR *name2)
{
	REGISTER CHAR *pt1, *pt2, pt1save, pt2save, *number1, *number2,
		*nameend1, *nameend2;
	REGISTER INTBIG pt1index, pt2index, pt1number, pt2number;
	REGISTER INTBIG compare;

	number1 = 0;
	for(pt1 = name1; *pt1 != 0; pt1++)
	{
		if (*pt1 == '[') break;
		if (isdigit(*pt1) == 0) number1 = 0; else
		{
			if (number1 == 0) number1 = pt1;
		}
	}
	for(pt1 = name1; *pt1 != 0; pt1++)
		if (*pt1 == '[' && isdigit(pt1[1])) break;
	number2 = 0;
	for(pt2 = name2; *pt2 != 0; pt2++)
	{
		if (*pt2 == '[') break;
		if (isdigit(*pt2) == 0) number2 = 0; else
		{
			if (number2 == 0) number2 = pt2;
		}
	}
	for(pt2 = name2; *pt2 != 0; pt2++)
		if (*pt2 == '[' && isdigit(pt2[1])) break;
	nameend1 = pt1;   nameend2 = pt2;
	pt1number = pt2number = 0;
	pt1index = pt2index = 0;
	if (number1 != 0 && number2 != 0)
	{
		pt1number = myatoi(number1);
		pt2number = myatoi(number2);
		if (pt1number != pt2number) compare = 0; else
		{
			/* make sure the text is the same if the numbers are the same */
			pt1save = *nameend1;   *nameend1 = 0;
			pt2save = *nameend2;   *nameend2 = 0;
			compare = namesame(name1, name2);
			*nameend2 = pt2save;   *nameend1 = pt1save;
		}
		if (compare == 0)
		{
			nameend1 = number1;
			nameend2 = number2;
		}
	}
	if (*pt1 == '[') pt1index = myatoi(&pt1[1]);
	if (*pt2 == '[') pt2index = myatoi(&pt2[1]);

	pt1save = *nameend1;   *nameend1 = 0;
	pt2save = *nameend2;   *nameend2 = 0;
	compare = namesame(name1, name2);
	*nameend2 = pt2save;   *nameend1 = pt1save;
	if (compare != 0) return(compare);
	if (pt1number != pt2number) return(pt1number - pt2number);
	return(pt1index - pt2index);
}

#ifdef _UNICODE
/*
 * Convert "string" from wide to 8-bit.
 */
CHAR1 *estring1byte(CHAR *string)
{
	REGISTER INTBIG len;
	static CHAR1 *retbuf;
	static INTBIG retbufsize = 0;

	len = estrlen(string) + 1;
	if (len * 3 > retbufsize)
	{
		if (retbufsize > 0) efree((CHAR *)retbuf);
		retbufsize = 0;
		retbuf = (CHAR1 *)emalloc(len*3, us_tool->cluster);
		if (retbuf == 0) return(b_(""));
	}
	wcstombs(retbuf, string, retbufsize);
	return(retbuf);	
}

#define STRLEN1BYTE strlen

/*
 * Convert "string" from 8-bit to wide.
 */
CHAR *estring2byte(CHAR1 *string)
{
	REGISTER INTBIG len;
	static CHAR *retbuf;
	static INTBIG retbufsize = 0;

	len = STRLEN1BYTE(string) + 1;
	if (len > retbufsize)
	{
		if (retbufsize > 0) efree((CHAR *)retbuf);
		retbufsize = 0;
		retbuf = (CHAR *)emalloc(len*SIZEOFCHAR, us_tool->cluster);
		if (retbuf == 0) return(x_(""));
	}
	mbtowc(retbuf, string, len);
	return(retbuf);	
}
#endif

/******************** STRING PARSING ROUTINES ********************/

/*
 * routine to scan off the next keyword in the string at "*ptin".  The string
 * is terminated by any of the characters in "brk".  The string is returned
 * (-1 if error) and the string pointer is advanced to the break character
 */
CHAR *getkeyword(CHAR **ptin, CHAR *brk)
{
	REGISTER CHAR *pt2, *b, *pt;
	REGISTER INTBIG len;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	/* skip leading blanks */
	pt = *ptin;
	while (*pt == ' ' || *pt == '\t') pt++;

	/* remember starting point */
	pt2 = pt;
	for(;;)
	{
		if (*pt2 == 0) break;
		for(b = brk; *b != 0; b++) if (*pt2 == *b) break;
		if (*b != 0) break;
		pt2++;
	}
	len = pt2 - pt;
	if (len+1 > db_keywordbufferlength)
	{
		if (db_keywordbufferlength != 0) efree(db_keywordbuffer);
		db_keywordbufferlength = 0;
		db_keywordbuffer = (CHAR *)emalloc((len+1) * SIZEOFCHAR, el_tempcluster);
		if (db_keywordbuffer == 0)
		{
			ttyputnomemory();
			return(NOSTRING);
		}
		db_keywordbufferlength = len+1;
	}
	(void)estrncpy(db_keywordbuffer, pt, len);
	db_keywordbuffer[len] = 0;
	*ptin = pt2;
	return(db_keywordbuffer);
}

/*
 * routine to return the next nonblank character in the string pointed
 * to by "ptin".  The pointer is advanced past that character.
 */
CHAR tonextchar(CHAR **ptin)
{
	REGISTER CHAR *pt, ret;

	/* skip leading blanks */
	pt = *ptin;
	while (*pt == ' ' || *pt == '\t') pt++;
	ret = *pt;
	if (ret != 0) pt++;
	*ptin = pt;
	return(ret);
}

/*
 * routine to return the next nonblank character in the string pointed
 * to by "ptin".  The pointer is advanced to that character.
 */
CHAR seenextchar(CHAR **ptin)
{
	REGISTER CHAR *pt;

	/* skip leading blanks */
	pt = *ptin;
	while (*pt == ' ' || *pt == '\t') pt++;
	*ptin = pt;
	return(*pt);
}

/******************** INFINITE STRING PACKAGE ********************/

/*
 * this package provides the ability to build an arbitrarily large string.
 * The first routine called must be "initinfstr()" which returns a "string object".
 * After that, make any number of calls to "addtoinfstr()", "addstringtoinfstr()",
 * or "formatinfstr()".  At any time, call "getinfstr()" to return the string.
 * When done, call "doneinfstr".
 */

/*
 * Routine to initialize a new infinite string.  Returns the address of the
 * infinite string object (zero on error).
 */
void *initinfstr(void)
{
	REGISTER INTBIG i;
	REGISTER INFSTR *inf;

	if (!db_firstinf)
	{
		/* first time through: build a free-list with many blocks */
		if (ensurevalidmutex(&db_infstrmutex, TRUE)) return(0);
		db_firstinf = TRUE;
		db_firstfreeinfstr = NOINFSTR;
		db_lastfreeinfstr = NOINFSTR;
		for(i=0; i<INFSTRCOUNT; i++)
		{
			inf = (INFSTR *)emalloc(sizeof (INFSTR), db_cluster);
			if (inf == 0)
			{
				(void)db_error(DBNOMEM|DBINITINFSTR);
				return(0);
			}
			inf->infstr = (CHAR *)emalloc((INFSTRDEFAULT+1) * SIZEOFCHAR, db_cluster);
			if (inf->infstr == 0)
			{
				(void)db_error(DBNOMEM|DBINITINFSTR);
				return(0);
			}
			inf->infstrlength = INFSTRDEFAULT;

			/* append it to the end of the list */
			inf->nextinfstr = NOINFSTR;
			if (db_lastfreeinfstr == NOINFSTR)
			{
				db_firstfreeinfstr = inf;
			} else
			{
				db_lastfreeinfstr->nextinfstr = inf;
			}
			db_lastfreeinfstr = inf;
		}
	}

	/* see if there is a free one on the list */
	inf = NOINFSTR;
	if (db_multiprocessing) emutexlock(db_infstrmutex);
	if (db_firstfreeinfstr != NOINFSTR)
	{
		inf = db_firstfreeinfstr;
		db_firstfreeinfstr = inf->nextinfstr;
		if (db_firstfreeinfstr == NOINFSTR)
			db_lastfreeinfstr = NOINFSTR;
	}
	if (db_multiprocessing) emutexunlock(db_infstrmutex);

	/* if none in free list, allocate a new one */
	if (inf == NOINFSTR)
	{
		inf = (INFSTR *)emalloc(sizeof (INFSTR), db_cluster);
		if (inf == 0)
		{
			(void)db_error(DBNOMEM|DBINITINFSTR);
			return(0);
		}
	}
	inf->infstrptr = 0;
	inf->infstr[inf->infstrptr] = 0;
	return(inf);
}

void addtoinfstr(void *vinf, CHAR ch)
{
	REGISTER CHAR *str;
	REGISTER INFSTR *inf;

	if (vinf == 0) return;
	inf = (INFSTR *)vinf;
	if (inf->infstrptr >= inf->infstrlength)
	{
		str = (CHAR *)emalloc((inf->infstrlength*2+1) * SIZEOFCHAR, db_cluster);
		if (str == 0)
		{
			(void)db_error(DBNOMEM|DBADDTOINFSTR);
			return;
		}
		inf->infstrlength *= 2;
		(void)estrcpy(str, inf->infstr);
		efree(inf->infstr);
		inf->infstr = str;
	}
	inf->infstr[inf->infstrptr++] = ch;
	inf->infstr[inf->infstrptr] = 0;
}

void addstringtoinfstr(void *vinf, CHAR *pp)
{
	REGISTER CHAR *str;
	REGISTER INTBIG l, i, ori;
	REGISTER INFSTR *inf;

	if (vinf == 0) return;
	inf = (INFSTR *)vinf;

	if (pp == 0) l = 0; else
		l = estrlen(pp);
	if (inf->infstrptr+l >= inf->infstrlength)
	{
		ori = inf->infstrlength;
		while (inf->infstrptr+l >= inf->infstrlength)
			inf->infstrlength *= 2;
		str = (CHAR *)emalloc((inf->infstrlength+1) * SIZEOFCHAR, db_cluster);
		if (str == 0)
		{
			inf->infstrlength = ori;
			(void)db_error(DBNOMEM|DBADDSTRINGTOINFSTR);
			return;
		}
		(void)estrcpy(str, inf->infstr);
		efree(inf->infstr);
		inf->infstr = str;
	}
	for(i=0; i<l; i++) inf->infstr[inf->infstrptr++] = pp[i];
	inf->infstr[inf->infstrptr] = 0;
}

/*
 * Routine to do a variable-arguments "sprintf" to line "line" which
 * is "len" in size.
 */
void evsnprintf(CHAR *line, INTBIG len, CHAR *msg, va_list ap)
{
#ifdef HAVE_VSNPRINTF
#  ifdef _UNICODE
#    ifdef WIN32
	(void)_vsnwprintf(line, len-1, msg, ap);
#    else
	(void)vswprintf(line, len-1, msg, ap);
#    endif
#  else
	(void)vsnprintf(line, len-1, msg, ap);
#  endif
#else
	(void)evsprintf(line, msg, ap);
#endif
}

void formatinfstr(void *vinf, CHAR *msg, ...)
{
	va_list ap;
	CHAR line[8192];

	var_start(ap, msg);
	evsnprintf(line, 8192, msg, ap);
	va_end(ap);
	addstringtoinfstr(vinf, line);
}

CHAR *returninfstr(void *vinf)
{
	REGISTER CHAR *retval;
	REGISTER INFSTR *inf;

	if (vinf == 0) return(x_(""));
	inf = (INFSTR *)vinf;

	/* get the inifinite string */
	retval = inf->infstr;

	/* append it to the end of the list */
	if (db_multiprocessing) emutexlock(db_infstrmutex);
	inf->nextinfstr = NOINFSTR;
	if (db_lastfreeinfstr == NOINFSTR)
	{
		db_firstfreeinfstr = inf;
	} else
	{
		db_lastfreeinfstr->nextinfstr = inf;
	}
	db_lastfreeinfstr = inf;
	if (db_multiprocessing) emutexunlock(db_infstrmutex);

	/* return the string */
	return(retval);
}

/****************************** FILES IN CELLS ******************************/

typedef struct
{
	CHAR   **strings;
	INTBIG   stringcount;
	INTBIG   stringlimit;
	CLUSTER *cluster;
} STRINGARRAY;

/*
 * Routine to create an object for gathering arrays of strings.
 * Returns a pointer that can be used with "addtostringarray", "keepstringarray", "stringarraytotextcell", and
 * "killstringarray"
 * Returns zero one error.
 */
void *newstringarray(CLUSTER *cluster)
{
	STRINGARRAY *sa;

	sa = (STRINGARRAY *)emalloc(sizeof (STRINGARRAY), cluster);
	if (sa == 0) return(0);
	sa->stringlimit = 0;
	sa->stringcount = 0;
	sa->cluster = cluster;
	return((void *)sa);
}

void killstringarray(void *vsa)
{
	STRINGARRAY *sa;

	sa = (STRINGARRAY *)vsa;
	if (sa == 0) return;
	clearstrings(sa);
	if (sa->stringlimit > 0) efree((CHAR *)sa->strings);
	efree((CHAR *)sa);
}

void clearstrings(void *vsa)
{
	STRINGARRAY *sa;
	REGISTER INTBIG i;

	sa = (STRINGARRAY *)vsa;
	if (sa == 0) return;
	for(i=0; i<sa->stringcount; i++) efree(sa->strings[i]);
	sa->stringcount = 0;
}

void addtostringarray(void *vsa, CHAR *string)
{
	REGISTER CHAR **newbuf;
	REGISTER INTBIG i, newlimit;
	STRINGARRAY *sa;

	sa = (STRINGARRAY *)vsa;
	if (sa == 0) return;
	if (sa->stringcount >= sa->stringlimit)
	{
		newlimit = sa->stringlimit * 2;
		if (newlimit <= 0) newlimit = 10;
		if (newlimit < sa->stringcount) newlimit = sa->stringcount;
		newbuf = (CHAR **)emalloc(newlimit * (sizeof (CHAR *)), sa->cluster);
		if (newbuf == 0) return;
		for(i=0; i<sa->stringcount; i++) newbuf[i] = sa->strings[i];
		if (sa->stringlimit > 0) efree((CHAR *)sa->strings);
		sa->strings = newbuf;
		sa->stringlimit += 10;
	}
	if (allocstring(&sa->strings[sa->stringcount], string, sa->cluster)) return;
	sa->stringcount++;
}

/*
 * routine called when done adding lines to string array "vsa".  The collection of lines is
 * stored in the "FACET_message" variable on the cell "np".  It is made permanent if
 * "permanent" is true.
 */
void stringarraytotextcell(void *vsa, NODEPROTO *np, BOOLEAN permanent)
{
	STRINGARRAY *sa;
	REGISTER INTBIG type;

	sa = (STRINGARRAY *)vsa;
	if (sa == 0) return;
	if (sa->stringcount <= 0) return;
	type = VSTRING|VISARRAY|(sa->stringcount<<VLENGTHSH);
	if (!permanent) type |= VDONTSAVE;
	(void)setvalkey((INTBIG)np, VNODEPROTO, el_cell_message_key, (INTBIG)sa->strings, type);
}

/*
 * routine called when done adding lines to string array "vsa".  The collection of lines is
 * returned to you.
 */
CHAR **getstringarray(void *vsa, INTBIG *count)
{
	STRINGARRAY *sa;

	sa = (STRINGARRAY *)vsa;
	if (sa == 0) { *count = 0;   return(0); }
	*count = sa->stringcount;
	return(sa->strings);
}

/************************* TIME HANDLING *************************/

/*
 * Time functions are centralized here to account for different time
 * systems on different computers.
 */

#define TRUECTIME ctime
#define TRUEFOPEN fopen

/*
 * return the current time.
 */
CHAR *ectime(time_t *curtime)
{
#ifdef _UNICODE
#  ifdef WIN32
	return(_wctime(curtime));
#  else
	CHAR1 *result;

	result = TRUECTIME(curtime);
	return(string2byte(result));
#  endif
#else
	return(TRUECTIME(curtime));
#endif
}

/*
 * open the file.
 */
FILE *efopen(CHAR *filename, CHAR *mode)
{
#ifdef _UNICODE
#  ifdef WIN32
	return(_wfopen(filename, mode));
#  else
	REGISTER CHAR1 *filename1, *mode1;

	filename1 = string1byte(filename);
	mode1 = string1byte(mode);
	return(TRUEFOPEN(filename1, mode1));
#  endif
#else
	return(TRUEFOPEN(filename, mode));
#endif
}

/*
 * Routine to return the current time in seconds since January 1, 1970.
 */
time_t getcurrenttime(void)
{
	time_t curtime;

	(void)time(&curtime);
	curtime -= machinetimeoffset();
	return(curtime);
}

/*
 * Routine to convert the time "curtime" to a string describing the date.
 * This string does *NOT* have a carriage-return after it.
 */
CHAR *timetostring(time_t curtime)
{
	static CHAR timebuf[30];

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	curtime += machinetimeoffset();
	estrcpy(timebuf, ectime(&curtime));
	timebuf[24] = 0;
	return(timebuf);
}

/*
 * Routine to parse the time in "curtime" and convert it to the year,
 * month (0 = January), and day of month.
 */
void parsetime(time_t curtime, INTBIG *year, INTBIG *month, INTBIG *mday,
	INTBIG *hour, INTBIG *minute, INTBIG *second)
{
	struct tm *tm;

	curtime += machinetimeoffset();
	tm = localtime(&curtime);
	*month = tm->tm_mon;
	*mday = tm->tm_mday;
	*year = tm->tm_year + 1900;
	*hour = tm->tm_hour;
	*minute = tm->tm_min;
	*second = tm->tm_sec;
}

INTBIG parsemonth(CHAR *monthname)
{
	REGISTER INTBIG mon;
	static CHAR *name[] = {x_("Jan"), x_("Feb"), x_("Mar"), x_("Apr"), x_("May"), x_("Jun"), x_("Jul"),
		x_("Aug"), x_("Sep"), x_("Oct"), x_("Nov"), x_("Dec")};

	for (mon=0; mon<12; mon++)
		if (namesame(name[mon], monthname) == 0) break;
	if (mon < 12) return(mon+1);
	return(0);
}

/******************** SUBROUTINES FOR FILE I/O ****************/

INTBIG setupfiletype(CHAR *extension, CHAR *winfilter, INTBIG mactype, BOOLEAN binary,
	CHAR *shortname, CHAR *longname)
{
	FILETYPES *newlist;
	INTBIG oldcount, i;

	oldcount = db_filetypecount;
	db_filetypecount++;
	newlist = (FILETYPES *)emalloc(db_filetypecount * (sizeof (FILETYPES)), db_cluster);
	for(i=0; i<oldcount; i++)
	{
		newlist[i].extension = db_filetypeinfo[i].extension;
		newlist[i].winfilter = db_filetypeinfo[i].winfilter;
		newlist[i].mactype   = db_filetypeinfo[i].mactype;
		newlist[i].binary    = db_filetypeinfo[i].binary;
		newlist[i].shortname = db_filetypeinfo[i].shortname;
		newlist[i].longname  = db_filetypeinfo[i].longname;
	}
	if (oldcount > 0) efree((CHAR *)db_filetypeinfo);
	db_filetypeinfo = newlist;
	(void)allocstring(&db_filetypeinfo[oldcount].extension, extension, db_cluster);
	(void)allocstring(&db_filetypeinfo[oldcount].winfilter, winfilter, db_cluster);
	db_filetypeinfo[oldcount].mactype = mactype;
	db_filetypeinfo[oldcount].binary = binary;
	(void)allocstring(&db_filetypeinfo[oldcount].shortname, shortname, db_cluster);
	(void)allocstring(&db_filetypeinfo[oldcount].longname, longname, db_cluster);
	return(oldcount);
}

/*
 * Routine to return the extension, short name, and long name of file type "filetype".
 */
void describefiletype(INTBIG filetype, CHAR **extension, CHAR **winfilter, INTBIG *mactype,
	BOOLEAN *binary, CHAR **shortname, CHAR **longname)
{
	REGISTER INTBIG index;

	index = filetype & FILETYPE;
	*extension = db_filetypeinfo[index].extension;
	*winfilter = db_filetypeinfo[index].winfilter;
	*mactype   = db_filetypeinfo[index].mactype;
	*binary    = db_filetypeinfo[index].binary;
	*shortname = db_filetypeinfo[index].shortname;
	*longname  = db_filetypeinfo[index].longname;
}

/*
 * Routine to return the filetype associated with short name "shortname".
 * Returns zero on error.
 */
INTBIG getfiletype(CHAR *shortname)
{
	REGISTER INTBIG i;

	for(i=0; i<db_filetypecount; i++)
		if (namesame(shortname, db_filetypeinfo[i].shortname) == 0) return(i);
	return(0);
}

/*
 * routine to find the actual file name in a path/file specification.  The
 * input path name is in "file" and a pointer to the file name is returned.
 * Note that this routine handles the directory separating characters for
 * all operating systems ('\' on Windows, '/' on UNIX, and ':' on Macintosh)
 * because the strings may be from other systems.
 */
CHAR *skippath(CHAR *file)
{
	REGISTER CHAR *pt;

	pt = &file[estrlen(file)-1];
	while (pt != file && pt[-1] != '\\' && pt[-1] != '/' && pt[-1] != ':') pt--;
	return(pt);
}

/*
 * routine to return the size of file whose stream is "f"
 */
INTBIG filesize(FILE *f)
{
	INTBIG savepos, endpos;

	savepos = ftell(f);
	(void)fseek(f, 0L, 2);	/* SEEK_END */
	endpos = ftell(f);
	(void)fseek(f, savepos, 0);	/* SEEK_SET */
	return(endpos);
}

/*
 * routine to open a file whose name is "file".  The type of the file is in "filetype"
 * (a descriptor created by "setupfiletype()").  The file may be in the directory "otherdir".
 * The stream is returned (NULL if it can't be found), and the true name of the file (with
 * extensions and proper directory) is returned in "truename".
 */
FILE *xopen(CHAR *file, INTBIG filetype, CHAR *otherdir, CHAR **truename)
{
	REGISTER FILE *f;
	REGISTER CHAR *pp;
	INTBIG mactype;
	BOOLEAN binary;
	CHAR rarg[3], *extension, *winfilter, *shortname, *longname;

	/* determine extension to use */
	describefiletype(filetype&FILETYPE, &extension, &winfilter, &mactype, &binary, &shortname, &longname);

	/* determine argument to "fopen" */
	if ((filetype&FILETYPEWRITE) != 0) estrcpy(rarg, x_("w")); else
		if ((filetype&FILETYPEAPPEND) != 0) estrcpy(rarg, x_("a")); else
			estrcpy(rarg, x_("r"));
	if (binary != 0) estrcat(rarg, x_("b"));

	/* remove "~" from the file name */
	pp = truepath(file);

	/* add the extension and look for the file */
	if (*extension != 0)
	{
		f = db_tryfile(pp, rarg, extension, (CHAR *)0, truename);
		if (f != NULL) return(f);
	}

	/* try the file directly */
	f = db_tryfile(pp, rarg, (CHAR *)0, (CHAR *)0, truename);
	if (f != NULL) return(f);

	/* if no other directory given, stop now */
	if (otherdir == 0 || *otherdir == 0) return(NULL);

	/* if directory path is in file name, stop now */
	if (*pp == '/') return(NULL);

	/* try the file in the other directory with the extension */
	f = db_tryfile(pp, rarg, extension, otherdir, truename);
	if (f != NULL) return(f);

	/* try the file in the other directory with no extension */
	f = db_tryfile(pp, rarg, (CHAR *)0, otherdir, truename);
	if (f != NULL) return(f);
	return(NULL);
}

/*
 * routine to try to find a file with name "pp", read with argument "rarg",
 * with extension "extension" (if it is nonzero) and in directory "otherdir"
 * (if it is nonzero).  Returns the file descriptor, and a pointer to the
 * actual file name in "truename"
 */
FILE *db_tryfile(CHAR *pp, CHAR *rarg, CHAR *extension, CHAR *otherdir, CHAR **truename)
{
	REGISTER INTBIG len;
	REGISTER CHAR *pt;

	len = estrlen(pp) + 1;
	if (extension != 0) len += estrlen(extension) + 1;
	if (otherdir != 0) len += estrlen(otherdir) + 1;
	if (len > db_tryfilenamelen)
	{
		if (db_tryfilenamelen != 0) efree(db_tryfilename);
		db_tryfilenamelen = 0;
		db_tryfilename = (CHAR *)emalloc(len * SIZEOFCHAR, db_cluster);
		if (db_tryfilename == 0) return(NULL);
		db_tryfilenamelen = len;
	}
	db_tryfilename[0] = 0;
	if (otherdir != 0)
	{
		(void)estrcat(db_tryfilename, otherdir);
		len = estrlen(db_tryfilename);
		if (db_tryfilename[len-1] != DIRSEP)
		{
			db_tryfilename[len++] = DIRSEP;
			db_tryfilename[len] = 0;
		}
	}
	(void)estrcat(db_tryfilename, pp);
	if (extension != 0)
	{
		(void)estrcat(db_tryfilename, x_("."));
		(void)estrcat(db_tryfilename, extension);
	}

	/* now extend this to the full path name */
	pt = fullfilename(db_tryfilename);
	len = estrlen(pt) + 1;
	if (len > db_tryfilenamelen)
	{
		efree(db_tryfilename);
		db_tryfilenamelen = 0;
		db_tryfilename = (CHAR *)emalloc(len * SIZEOFCHAR, db_cluster);
		if (db_tryfilename == 0) return(NULL);
		db_tryfilenamelen = len;
	}
	estrcpy(db_tryfilename, pt);
	*truename = db_tryfilename;
	return(efopen(db_tryfilename, rarg));
}

/*
 * Routine to create the file "name" and return a stream pointer.  "filetype" is the type
 * of file being created (a descriptor created by "setupfiletype()").  If "prompt" is zero, the path
 * is already fully set and the user should not be further querried.  Otherwise,
 * "prompt" is the string to use when describing this file (and the final name selected
 * by the user is returned in "truename").  The routine returns zero on error.  However,
 * to differentiate errors in file creation from user aborts, the "truename" is set to zero
 * if the user aborts the command.
 */
FILE *xcreate(CHAR *name, INTBIG filetype, CHAR *prompt, CHAR **truename)
{
	REGISTER FILE *f;
	REGISTER CHAR *pt, *next;
	CHAR warg[3], *extension, *winfilter, *shortname, *longname;
	static CHAR truenamelocal[256];
	INTBIG mactype;
	BOOLEAN binary;

	/* determine extension to use */
	describefiletype(filetype&FILETYPE, &extension, &winfilter, &mactype, &binary, &shortname, &longname);
	estrcpy(warg, x_("w"));
	if (binary) estrcat(warg, x_("b"));

	if ((filetype&FILETYPETEMPFILE) != 0)
	{
#ifdef HAVE_MKSTEMP
		int chn;
		chn = mkstemp(name);
		if (chn < 0) return(0);
		f = fdopen(chn, warg); 
#else
#  ifdef HAVE_MKTEMP
		mktemp(name);
#  endif
		f = efopen(name, warg);
#endif
		*truename = name;
	} else
	{
		pt = truepath(name);
		if (prompt != 0 && (us_tool->toolstate&USEDIALOGS) != 0)
		{
			next = (CHAR *)fileselect(prompt, filetype | FILETYPEWRITE, pt);
			if (next == 0 || *next == 0)
			{
				if (truename != 0) *truename = 0;
				return(NULL);
			}
			setdefaultcursortype(NULLCURSOR);		/* the hourglass cursor */
			pt = next;
		}
		if (truename != 0)
		{
			(void)estrcpy(truenamelocal, pt);
			*truename = truenamelocal;
		}

		/* determine argument to "fopen" */
		f = efopen(pt, warg);
	}
#ifdef	MACOS
	if (f != NULL)
	{
		void mac_settypecreator(CHAR*, INTBIG, INTBIG);

		if (mactype == 'TEXT')
		{
			mac_settypecreator(pt, 'TEXT', 'ttxt');
		} else
		{
			mac_settypecreator(pt, mactype, 'Elec');
		}
	}
#endif
	return(f);
}

/*
 * Routine to append to the file "name" and return a stream pointer
 */
FILE *xappend(CHAR *name)
{
	return(efopen(truepath(name), x_("a")));
}

/*
 * Routine to close stream "f"
 */
void xclose(FILE *f)
{
	(void)fclose(f);
}

/*
 * Routine to flush stream "f"
 */
void xflushbuf(FILE *f)
{
	(void)fflush(f);
}

/*
 * Routine to report the EOF condition on stream "f"
 */
BOOLEAN xeof(FILE *f)
{
	if (feof(f) != 0) return(TRUE);
	return(FALSE);
}

/*
 * Routine to seek to position "pos", nature "nat", on stream "f"
 */
void xseek(FILE *f, INTBIG pos, INTBIG nat)
{
	fseek(f, pos, nat);
}

/*
 * Routine to return the current position in stream "f"
 */
INTBIG xtell(FILE *f)
{
	return(ftell(f));
}

/*
 * Routine to write the formatted message "s" with parameters "p1" through "p9"
 * to stream "f".
 */
void xprintf(FILE *f, CHAR *s, ...)
{
	va_list ap;

	var_start(ap, s);
	(void)evfprintf(f, s, ap);
	va_end(ap);
}

/*
 * Routine to get the next character in stream "f"
 */
INTSML xgetc(FILE *f)
{
	return(getc(f));
}

/*
 * Routine to "unget" the character "c" back to stream "f"
 */
void xungetc(CHAR c, FILE *f)
{
	(void)ungetc(c, f);
}

/*
 * Routine to put the character "c" into stream "f"
 */
void xputc(CHAR c, FILE *f)
{
	putc(c, f);
}

/*
 * Routine to put the string "s" into stream "f"
 */
void xputs(CHAR *s, FILE *f)
{
	(void)efputs(s, f);
}

/*
 * Routine to read "count" elements of size "size" from stream "f" into the buffer
 * at "buf".  Returns the number of objects read (ideally, "size").
 */
INTBIG xfread(UCHAR1 *buf, INTBIG size, INTBIG count, FILE *f)
{
	REGISTER INTBIG ret;

	for(;;)
	{
		ret = fread(buf, size, count, f);
		if (ret == count || feof(f) != 0) break;
		clearerr(f);
	}
	return(ret);
}

/*
 * Routine to write "count" elements of size "size" to stream "f" from the buffer
 * at "buf".  Returns the number of bytes written.
 */
INTBIG xfwrite(UCHAR1 *buf, INTBIG size, INTBIG count, FILE *f)
{
	REGISTER INTBIG ret;

	for(;;)
	{
		ret = fwrite(buf, size, count, f);
		if (ret == count || feof(f) != 0) break;
		clearerr(f);
	}
	return(ret);
}

/*
 * routine to read a line of text from a file.  The file is in stream "file"
 * and the text is placed in the array "line" which is only "limit" characters
 * long.  The routine returns false if sucessful, true if end-of-file is
 * reached.
 */
BOOLEAN xfgets(CHAR *line, INTBIG limit, FILE *file)
{
	REGISTER CHAR *pp;
	REGISTER INTBIG c, total;

	pp = line;
	total = 1;
	for(;;)
	{
		c = xgetc(file);
		if (c == EOF)
		{
			if (pp == line) return(TRUE);
			break;
		}
		*pp = (CHAR)c;
		if (*pp == '\n' || *pp == '\r') break;
		pp++;
		if ((++total) >= limit) break;
	}
	*pp = 0;
	return(FALSE);
}

/******************** SUBROUTINES FOR ENCRYPTION ****************/

/*
 * A one-rotor machine designed along the lines of Enigma but considerably trivialized
 */
# define ROTORSZ 256		/* a power of two */
# define MASK    (ROTORSZ-1)
void myencrypt(CHAR *text, CHAR *key)
{
	INTBIG ic, i, k, temp, n1, n2, nr1, nr2;
	UINTBIG random;
	INTBIG seed;
	CHAR *pt, t1[ROTORSZ], t2[ROTORSZ], t3[ROTORSZ], deck[ROTORSZ], readable[ROTORSZ];
	
	/* first setup the machine */
	estrcpy(readable, x_("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-"));
	seed = 123;
	for (i=0; i<13; i++) seed = seed*key[i] + i;
	for(i=0; i<ROTORSZ; i++)
	{
		t1[i] = (CHAR)i;
		t3[i] = 0;
		deck[i] = (CHAR)i;
	}
	for(i=0; i<ROTORSZ; i++)
	{
		seed = 5*seed + key[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK) % (k+1);
		random >>= 8;
		temp = t1[k];
		t1[k] = t1[ic];
		t1[ic] = (CHAR)temp;
		if (t3[k] != 0) continue;
		ic = (random&MASK) % k;
		while (t3[ic] != 0) ic = (ic+1) % k;
		t3[k] = (CHAR)ic;
		t3[ic] = (CHAR)k;
	}
	for(i=0; i<ROTORSZ; i++) t2[t1[i]&MASK] = (CHAR)i;

	/* now run the machine */
	n1 = 0;
	n2 = 0;
	nr2 = 0;
	for(pt = text; *pt; pt++)
	{
		nr1 = deck[n1]&MASK;
		nr2 = deck[nr1]&MASK;
		i = t2[(t3[(t1[(*pt+nr1)&MASK]+nr2)&MASK]-nr2)&MASK]-nr1;
		*pt = readable[i&63];
		n1++;
		if (n1 == ROTORSZ)
		{
			n1 = 0;
			n2++;
			if (n2 == ROTORSZ) n2 = 0;
			db_shuffle(deck, key);
		}
	}
}

void db_shuffle(CHAR *deck, CHAR *key)
{
	INTBIG i, ic, k, temp;
	UINTBIG random;
	static INTBIG seed = 123;

	for(i=0; i<ROTORSZ; i++)
	{
		seed = 5*seed + key[i%13];
		random = seed % 65521;
		k = ROTORSZ-1 - i;
		ic = (random&MASK) % (k+1);
		temp = deck[k];
		deck[k] = deck[ic];
		deck[ic] = (CHAR)temp;
	}
}
