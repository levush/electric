/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: conlinprs.c
 * Linear inequality constraint system: text file parsing
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
 * Component syntax:
 *   declare INST1[DECLOPTS], INST2[DECLOPTS], ...,INSTN[DECLOPTS] PROTOTYPE;
 *      DECLOPTS: size=(X,Y)
 *                location=(X,Y)
 *                rotation=R[t]
 *                VARIABLE=VALUE
 *
 * Export syntax:
 *   export [EXPOPTS] PORTNAME is COMPONENT{:PORT};
 *      EXPOPTS: type={INPUT|OUTPUT|BIDIRECTIONAL|POWER|GROUND|
 *                     CLOCK|CLOCK1|CLOCK2|CLOCK3|CLOCK4|CLOCK5|CLOCK6}
 *               VARIABLE=VALUE
 *
 * Connection syntax:
 *   connect [CONNOPTS] COMPONENT1{:PORT1}
 *                      {DIR AMT {or more|less}} {[DX,DY]} to
 *                      COMPONENT2{:PORT2};
 *      CONNOPTS: layer=LAYER
 *                width=WIDTH
 *                VARIABLE=VALUE
 */
#include "global.h"
#include "conlin.h"

/* prototypes for local routines */
static BOOLEAN cli_doparseexport(CHAR*, BOOLEAN, EXPORT*);
static BOOLEAN cli_doparseconn(CHAR*, BOOLEAN, CONNECTION*);
static BOOLEAN cli_doparsecomp(CHAR**, CHAR*, BOOLEAN, COMPONENTDEC*);
static BOOLEAN cli_parsetwovalue(INTBIG*, INTBIG*, CHAR*, CHAR**, BOOLEAN);
static ATTR *cli_getattr(CHAR*, CHAR**, CHAR*, BOOLEAN);
static void cli_freeattr(ATTR*);

/*
 * routine to determine the type of text in "line"
 */
INTBIG cli_linetype(CHAR *line)
{
	while (*line == ' ' || *line == '\t') line++;
	if (*line == 0 || *line == ';') return(LINECOMM);
	if (namesamen(line, x_("declare"), 7) == 0) return(LINEDECL);
	if (namesamen(line, x_("connect"), 4) == 0) return(LINECONN);
	if (namesamen(line, x_("export"), 6) == 0) return(LINEEXPORT);
	if (namesamen(line, x_("begincell"), 9) == 0) return(LINEBEGIN);
	if (namesamen(line, x_("endcell"), 7) == 0) return(LINEEND);
	return(LINEUNKN);
}

/*
 * routine to parse a "begincell" command and return the cell name.  This
 * cell name must be deallocated when done.  If "quiet" is true, do not
 * print error messages.  Returns -1 if the line cannot be parsed
 */
CHAR *cli_parsebegincell(CHAR *str, BOOLEAN quiet)
{
	CHAR *pt;
	REGISTER CHAR *st;

	pt = str;

	/* get the "begincell" keyword */
	st = getkeyword(&pt, x_(" \t"));
	if (st == NOSTRING) return(NOSTRING);
	if (namesame(st, x_("begincell")) != 0)
	{
		if (!quiet) ttyputerr(M_("Missing 'begincell' keyword in '%s'"), str);
		return(NOSTRING);
	}

	/* get the cell name */
	st = getkeyword(&pt, x_(" \t;"));
	if (st == NOSTRING) return(NOSTRING);

	/* make sure it ends with a ";" */
	if (tonextchar(&pt) != ';')
	{
		if (!quiet) ttyputerr(M_("Missing ';' in '%s'"), str);
		return(NOSTRING);
	}
	if (tonextchar(&pt) != 0)
	{
		if (!quiet) ttyputerr(M_("Unexpected text after ';' in '%s'"), str);
		return(NOSTRING);
	}
	(void)allocstring(&pt, st, el_tempcluster);
	return(pt);
}

/*
 * routine to parse an "export" command and return a structure describing
 * it.  This must be deallocated when done.  If "quiet" is true, do not
 * print error messages.  Returns NOEXPORT if the line cannot be parsed.
 */
EXPORT *cli_parseexport(CHAR *str, BOOLEAN quiet)
{
	REGISTER EXPORT *e;

	/* allocate an export object */
	e = (EXPORT *)emalloc(sizeof (EXPORT), el_tempcluster);
	if (e == 0)
	{
		if (!quiet) ttyputnomemory();
		return(NOEXPORT);
	}
	e->portname = e->component = e->subport = 0;
	e->flag = 0;
	e->bits = 0;
	e->firstattr = NOATTR;

	if (cli_doparseexport(str, quiet, e))
	{
		cli_deleteexport(e);
		return(NOEXPORT);
	}
	return(e);
}

/*
 * routine to parse the bulk of an "export" statement, assuming that "str"
 * points to the beginning of the line and "quiet" is the verbose error flag.
 * Fills the "e" structure and returns false if successful.
 */
BOOLEAN cli_doparseexport(CHAR *str, BOOLEAN quiet, EXPORT *e)
{
	CHAR *pt, *opt;
	REGISTER CHAR *st;
	REGISTER ATTR *a;
	REGISTER INTBIG i;

	pt = str;

	/* get the "export" keyword */
	st = getkeyword(&pt, x_(" \t"));
	if (st == NOSTRING) return(TRUE);
	if (namesame(st, x_("export")) != 0)
	{
		if (!quiet) ttyputerr(M_("Missing 'export' keyword in '%s'"), str);
		return(TRUE);
	}

	/* get options */
	if (seenextchar(&pt) == '[')
	{
		(void)tonextchar(&pt);
		for(;;)
		{
			/* get name of option */
			opt = getkeyword(&pt, x_(" \t="));
			if (opt == NOSTRING) return(TRUE);
			if (*opt == 0)
			{
				if (!quiet) ttyputerr(M_("Missing option name in '%s'"), str);
				return(TRUE);
			}
			if (tonextchar(&pt) != '=')
			{
				if (!quiet) ttyputerr(M_("Missing '=' after %s option"), opt);
				return(TRUE);
			}

			if (namesame(opt, x_("type")) == 0)
			{
				st = getkeyword(&pt, x_(" \t],"));
				if (st == NOSTRING) return(TRUE);
				if (*st == 0)
				{
					if (!quiet) ttyputerr(M_("Missing type in '%s'"), str);
					return(TRUE);
				}
				if (namesame(st, x_("input"))        == 0) e->bits |= INPORT; else
				if (namesame(st, x_("output"))       == 0) e->bits |= OUTPORT; else
				if (namesame(st, x_("bidirectional"))== 0) e->bits |= BIDIRPORT; else
				if (namesame(st, x_("power"))        == 0) e->bits |= PWRPORT; else
				if (namesame(st, x_("ground"))       == 0) e->bits |= GNDPORT; else
				if (namesame(st, x_("clock"))        == 0) e->bits |= CLKPORT; else
				if (namesame(st, x_("clock1"))       == 0) e->bits |= C1PORT; else
				if (namesame(st, x_("clock2"))       == 0) e->bits |= C2PORT; else
				if (namesame(st, x_("clock3"))       == 0) e->bits |= C3PORT; else
				if (namesame(st, x_("clock4"))       == 0) e->bits |= C4PORT; else
				if (namesame(st, x_("clock5"))       == 0) e->bits |= C5PORT; else
				if (namesame(st, x_("clock6"))       == 0) e->bits |= C6PORT; else
				if (namesame(st, x_("refout"))       == 0) e->bits |= REFOUTPORT; else
				if (namesame(st, x_("refin"))        == 0) e->bits |= REFINPORT; else
				if (namesame(st, x_("refbase"))      == 0) e->bits |= REFBASEPORT; else
				{
					if (!quiet) ttyputerr(M_("Invalid port type in '%s'"), str);
					return(TRUE);
				}
				e->flag |= EXPCHAR;
			} else
			{
				/* arbitrary keyword processing */
				a = cli_getattr(opt, &pt, str, quiet);
				if (a == NOATTR) return(TRUE);
				a->nextattr = e->firstattr;
				e->firstattr = a;
				e->flag |= EXPATTR;
			}

			/* clean up after option parsing */
			i = tonextchar(&pt);
			if (i == ']') break;
			if (i != ',')
			{
				if (!quiet) ttyputerr(M_("Missing ',' separating options in '%s'"), str);
				return(TRUE);
			}
		}
	}

	/* get the exported port name */
	st = getkeyword(&pt, x_(" \t;"));
	if (st == NOSTRING) return(TRUE);
	(void)allocstring(&e->portname, st, el_tempcluster);

	/* get the "is" keyword */
	st = getkeyword(&pt, x_(" \t"));
	if (st == NOSTRING) return(TRUE);
	if (namesame(st, x_("is")) != 0)
	{
		if (!quiet) ttyputerr(M_("Missing 'is' keyword in '%s'"), str);
		return(TRUE);
	}

	/* get the component name */
	st = getkeyword(&pt, x_(" \t;:"));
	if (st == NOSTRING) return(TRUE);
	(void)allocstring(&e->component, st, el_tempcluster);

	/* see if there is a ":" next */
	if (seenextchar(&pt) == ':')
	{
		/* get the subport name */
		(void)tonextchar(&pt);
		st = getkeyword(&pt, x_(" \t;"));
		if (st == NOSTRING) return(TRUE);
		(void)allocstring(&e->subport, st, el_tempcluster);
	}

	/* make sure it ends with a ";" */
	if (tonextchar(&pt) != ';')
	{
		if (!quiet) ttyputerr(M_("Missing ';' in '%s'"), str);
		return(TRUE);
	}
	if (tonextchar(&pt) != 0)
	{
		if (!quiet) ttyputerr(M_("Unexpected text after ';' in '%s'"), str);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to parse the connection line in the string "str" and return a
 * CONNECTION structure that describes it.  If "quiet" is true, do not
 * print error messages.  Returns NOCONNECTION on error.
 */
CONNECTION *cli_parseconn(CHAR *str, BOOLEAN quiet)
{
	REGISTER CONNECTION *dcl;

	/* allocate a declaration object */
	dcl = (CONNECTION *)emalloc(sizeof (CONNECTION), el_tempcluster);
	if (dcl == 0)
	{
		if (!quiet) ttyputnomemory();
		return(NOCONNECTION);
	}
	dcl->firstcons = NOCONS;
	dcl->flag = 0;
	dcl->firstattr = NOATTR;

	/* parse the remainder of the connection declaration */
	if (cli_doparseconn(str, quiet, dcl))
	{
		/* parsing failed */
		cli_deleteconnection(dcl);
		return(NOCONNECTION);
	}

	/* parse succeeded */
	return(dcl);
}

/*
 * routine to parse the bulk of a "connect" statement, assuming that "str"
 * points to the beginning of the line and "quiet" is the verbose error flag.
 * Fills the "dcl" structure and returns false if successful.
 */
BOOLEAN cli_doparseconn(CHAR *str, BOOLEAN quiet, CONNECTION *dcl)
{
	REGISTER CHAR *st, *opt;
	CHAR *pt;
	REGISTER CONS *cons, *lastcons;
	REGISTER INTBIG i;
	REGISTER ATTR *a;

	pt = str;

	st = getkeyword(&pt, x_(" \t"));
	if (st == NOSTRING) return(TRUE);
	if (namesame(st, x_("connect")) != 0)
	{
		if (!quiet) ttyputerr(M_("Missing 'connect' keyword in '%s'"), str);
		return(TRUE);
	}

	/* get options */
	if (seenextchar(&pt) == '[')
	{
		(void)tonextchar(&pt);
		for(;;)
		{
			/* get name of option */
			opt = getkeyword(&pt, x_(" \t="));
			if (opt == NOSTRING) return(TRUE);
			if (*opt == 0)
			{
				if (!quiet) ttyputerr(M_("Missing option name in '%s'"), str);
				return(TRUE);
			}
			if (tonextchar(&pt) != '=')
			{
				if (!quiet) ttyputerr(M_("Missing '=' after %s option"), opt);
				return(TRUE);
			}

			if (namesame(opt, x_("layer")) == 0)
			{
				st = getkeyword(&pt, x_(" \t],"));
				if (st == NOSTRING) return(TRUE);
				if (*st == 0)
				{
					if (!quiet) ttyputerr(M_("Missing layer in '%s'"), str);
					return(TRUE);
				}
				(void)allocstring(&dcl->layer, st, el_tempcluster);
				dcl->flag |= LAYERVALID;
			} else if (namesame(opt, x_("width")) == 0)
			{
				st = getkeyword(&pt, x_(" \t],"));
				if (st == NOSTRING) return(TRUE);
				if (*st == 0)
				{
					if (!quiet) ttyputerr(M_("Missing width in '%s'"), str);
					return(TRUE);
				}
				dcl->width = atola(st, 0);
				dcl->flag |= WIDTHVALID;
			} else
			{
				/* arbitrary keyword processing */
				a = cli_getattr(opt, &pt, str, quiet);
				if (a == NOATTR) return(TRUE);
				a->nextattr = dcl->firstattr;
				dcl->firstattr = a;
				dcl->flag |= ATTRVALID;
			}

			/* clean up after option parsing */
			i = tonextchar(&pt);
			if (i == ']') break;
			if (i != ',')
			{
				if (!quiet) ttyputerr(M_("Missing ',' separating options in '%s'"), str);
				return(TRUE);
			}
		}
	}

	/* get the name of the first node */
	st = getkeyword(&pt, x_(" \t;:"));
	if (st == NOSTRING) return(TRUE);
	if (*st == 0)
	{
		if (!quiet) ttyputerr(M_("Incorrect prototype syntax in '%s'"), str);
		return(TRUE);
	}
	(void)allocstring(&dcl->end1, st, el_tempcluster);
	dcl->flag |= END1VALID;
	if (seenextchar(&pt) == ':')
	{
		/* parse the port prototype for end 1 */
		(void)tonextchar(&pt);
		st = getkeyword(&pt, x_(" \t;"));
		if (st == NOSTRING) return(TRUE);
		if (*st == 0)
		{
			if (!quiet) ttyputerr(M_("Incorrect port syntax in '%s'"), str);
			return(TRUE);
		}
		(void)allocstring(&dcl->port1, st, el_tempcluster);
		dcl->flag |= PORT1VALID;
	}

	/* look for the constraints */
	lastcons = NOCONS;
	for(;;)
	{
		st = getkeyword(&pt, x_(" \t;["));
		if (st == NOSTRING) return(TRUE);

		/* termination with the keyword "to" */
		if (namesame(st, x_("to")) == 0) break;

		/* handle the "[X,Y]" construct */
		if (*st == 0)
		{
			if (tonextchar(&pt) != '[')
			{
				if (!quiet) ttyputerr(M_("Incorrect constraint syntax in '%s'"), str);
				return(TRUE);
			}
			st = getkeyword(&pt, x_(" \t,"));
			if (st == NOSTRING) return(TRUE);
			dcl->xoff = atola(st, 0);
			dcl->flag |= OFFSETVALID;
			if (tonextchar(&pt) != ',')
			{
				if (!quiet) ttyputerr(M_("Missing comma in arc extent '%s'"), str);
				return(TRUE);
			}
			st = getkeyword(&pt, x_(" \t]"));
			dcl->yoff = atola(st, 0);
			if (tonextchar(&pt) != ']')
			{
				if (!quiet) ttyputerr(M_("Missing ']' in arc extent '%s'"), str);
				return(TRUE);
			}
			continue;
		}

		/* handle constraint directions */
		if (namesame(st, x_("left")) != 0 && namesame(st, x_("right")) != 0 &&
			namesame(st, x_("up")) != 0 && namesame(st, x_("down")) != 0)
		{
			if (!quiet) ttyputerr(M_("Invalid relationship '%s' in '%s'"), st, str);
			return(TRUE);
		}

		/* create a CONS object for this constraint */
		cons = (CONS *)emalloc(sizeof (CONS), el_tempcluster);
		if (cons == 0)
		{
			if (!quiet) ttyputnomemory();
			return(TRUE);
		}
		cons->nextcons = NOCONS;
		if (lastcons == NOCONS) dcl->firstcons = cons; else
			lastcons->nextcons = cons;
		lastcons = cons;
		(void)allocstring(&cons->direction, st, el_tempcluster);

		/* get amount */
		st = getkeyword(&pt, x_(" \t;["));
		if (st == NOSTRING) return(TRUE);
		cons->amount = atofr(st);

		/* see if an "or" follows */
		cons->flag = EQUAL;
		if (seenextchar(&pt) == 'o')
		{
			st = getkeyword(&pt, x_(" \t;["));
			if (st == NOSTRING) return(TRUE);
			if (namesame(st, x_("or")) != 0)
			{
				if (!quiet) ttyputerr(M_("Unknown qualifier '%s' in '%s'"), st, str);
				return(TRUE);
			}
			st = getkeyword(&pt, x_(" \t;["));
			if (st == NOSTRING) return(TRUE);
			cons->flag = 999;
			if (namesame(st, x_("more")) == 0) cons->flag = GEQ;
			if (namesame(st, x_("less")) == 0) cons->flag = LEQ;
			if (cons->flag == 999)
			{
				if (!quiet) ttyputerr(M_("Unknown qualifier '%s' in '%s'"), st, str);
				return(TRUE);
			}
		}
	}

	/* get the name of the second node */
	st = getkeyword(&pt, x_(" \t;:["));
	if (st == NOSTRING) return(TRUE);
	if (*st == 0)
	{
		if (!quiet) ttyputerr(M_("Incorrect prototype syntax in '%s'"), str);
		return(TRUE);
	}
	(void)allocstring(&dcl->end2, st, el_tempcluster);
	dcl->flag |= END2VALID;
	if (seenextchar(&pt) == ':')
	{
		/* parse the port prototype for end 2 */
		(void)tonextchar(&pt);
		st = getkeyword(&pt, x_(" \t;["));
		if (st == NOSTRING) return(TRUE);
		if (*st == 0)
		{
			if (!quiet) ttyputerr(M_("Incorrect port syntax in '%s'"), str);
			return(TRUE);
		}
		(void)allocstring(&dcl->port2, st, el_tempcluster);
		dcl->flag |= PORT2VALID;
	}
	if (tonextchar(&pt) != ';')
	{
		if (!quiet) ttyputerr(M_("Missing semicolon in '%s'"), str);
		return(TRUE);
	}
	i = tonextchar(&pt);
	if (i == 0) return(FALSE);
	if (!quiet) ttyputerr(M_("Line '%s' should end with a semicolon"), str);
	return(TRUE);
}

/*
 * routine to parse the component line in the string "str" and return a
 * COMPONENTDEC structure that describes it.  If "quiet" is true, do not
 * print error messages.  Returns NOCOMPONENTDEC on error.
 */
COMPONENTDEC *cli_parsecomp(CHAR *str, BOOLEAN quiet)
{
	REGISTER CHAR *st;
	CHAR *pt;
	REGISTER COMPONENTDEC *dcl;

	/* allocate a declaration object */
	dcl = (COMPONENTDEC *)emalloc(sizeof (COMPONENTDEC), el_tempcluster);
	if (dcl == 0)
	{
		if (!quiet) ttyputnomemory();
		return(NOCOMPONENTDEC);
	}
	dcl->firstcomponent = NOCOMPONENT;
	dcl->protoname = 0;

	/* initialize pointer into string */
	pt = str;

	/* look for "declare" keyword */
	st = getkeyword(&pt, x_(" \t"));
	if (st == NOSTRING || namesame(st, x_("declare")) != 0)
	{
		if (!quiet) ttyputerr(M_("Missing 'declare' keyword in '%s'"), str);
		efree((CHAR *)dcl);
		return(NOCOMPONENTDEC);
	}

	/* parse the remainder of the component declaration */
	if (cli_doparsecomp(&pt, str, quiet, dcl))
	{
		/* parsing failed */
		cli_deletecomponentdec(dcl);
		return(NOCOMPONENTDEC);
	}

	/* parse succeeded */
	return(dcl);
}

/*
 * routine to parse the bulk of a "declare" statement, assuming that "pt"
 * points to the text after "declare".  "str" is the entire line and "quiet"
 * is the verbose error flag.  Fills the "dcl" structure and returns false
 * if successful
 */
BOOLEAN cli_doparsecomp(CHAR **pt, CHAR *str, BOOLEAN quiet, COMPONENTDEC *dcl)
{
	REGISTER CHAR *st, *opt;
	REGISTER COMPONENT *compo, *listpos;
	REGISTER INTBIG i;
	REGISTER ATTR *a;

	/* parse the instances */
	listpos = NOCOMPONENT;
	dcl->count = 0;
	for(;;)
	{
		st = getkeyword(pt, x_(" \t,;["));
		if (st == NOSTRING) return(TRUE);
		if (*st == 0)
		{
			if (!quiet) ttyputerr(M_("Missing component name in '%s'"), str);
			return(TRUE);
		}

		/* allocate a structure for the declared component */
		compo = (COMPONENT *)emalloc(sizeof (COMPONENT), el_tempcluster);
		if (compo == 0)
		{
			if (!quiet) ttyputnomemory();
			return(TRUE);
		}
		compo->flag = 0;
		compo->firstattr = NOATTR;

		/* place the component name in the structure */
		(void)allocstring(&compo->name, st, el_tempcluster);

		/* link the component object into the declaration */
		if (listpos == NOCOMPONENT) dcl->firstcomponent = compo; else
			listpos->nextcomponent = compo;
		compo->nextcomponent = NOCOMPONENT;
		listpos = compo;
		dcl->count++;

		/* look for options */
		if (seenextchar(pt) == '[')
		{
			(void)tonextchar(pt);
			for(;;)
			{
				/* get name of option */
				opt = getkeyword(pt, x_(" \t="));
				if (opt == NOSTRING) return(TRUE);
				if (*opt == 0)
				{
					if (!quiet) ttyputerr(M_("Missing option name in '%s'"), str);
					return(TRUE);
				}
				if (tonextchar(pt) != '=')
				{
					if (!quiet) ttyputerr(M_("Missing '=' after %s option"), opt);
					return(TRUE);
				}

				if (namesame(opt, x_("size")) == 0)
				{
					if (cli_parsetwovalue(&compo->sizex, &compo->sizey, x_("size"), pt, quiet))
						return(TRUE);
					compo->flag |= COMPSIZE;
				} else if (namesame(opt, x_("location")) == 0)
				{
					if (cli_parsetwovalue(&compo->locx, &compo->locy, x_("location"), pt, quiet))
						return(TRUE);
					compo->flag |= COMPLOC;
				} else if (namesame(opt, x_("rotation")) == 0)
				{
					st = getkeyword(pt, x_(" \t,]"));
					if (st == NOSTRING) return(TRUE);
					compo->rot = atofr(st)*10/WHOLE;
					i = estrlen(st)-1;
					if (st[i] == 't' || st[i] == 'T') compo->trans = 1; else
						compo->trans = 0;
					compo->flag |= COMPROT;
				} else
				{
					/* arbitrary keyword processing */
					a = cli_getattr(opt, pt, str, quiet);
					if (a == NOATTR) return(TRUE);
					a->nextattr = compo->firstattr;
					compo->firstattr = a;
					compo->flag |= COMPATTR;
				}

				/* clean up after option parsing */
				i = tonextchar(pt);
				if (i == ']') break;
				if (i != ',')
				{
					if (!quiet) ttyputerr(M_("Missing ',' separating options in '%s'"), str);
					return(TRUE);
				}
			}
		}

		/* more instances to parse? */
		if (seenextchar(pt) != ',') break;
		(void)tonextchar(pt);
	}

	/* take last string as prototype name */
	st = getkeyword(pt, x_(" \t;"));
	if (st == NOSTRING) return(TRUE);
	if (*st == 0)
	{
		if (!quiet) ttyputerr(M_("Missing prototype name at start of '%s'"), str);
		return(TRUE);
	}
	(void)allocstring(&dcl->protoname, st, el_tempcluster);

	/* make sure line terminates properly */
	if (tonextchar(pt) != ';')
	{
		if (!quiet) ttyputerr(M_("Missing semicolon in '%s'"), str);
		return(TRUE);
	}
	i = tonextchar(pt);
	if (i == 0) return(FALSE);
	if (!quiet) ttyputerr(M_("Line '%s' should end with a semicolon"), str);
	return(TRUE);
}

/*
 * routine to parse the expression "(X,Y)" at "pt" in the string, placing
 * the values in "x" and "y".  The name of this option is in "name" and the
 * verbose error message flag is in "quiet".  Returns true on error
 */
BOOLEAN cli_parsetwovalue(INTBIG *x, INTBIG *y, CHAR *name, CHAR **pt, BOOLEAN quiet)
{
	REGISTER CHAR *st;

	if (tonextchar(pt) != '(')
	{
		if (!quiet) ttyputerr(M_("Missing '(' after %s option"), name);
		return(TRUE);
	}
	st = getkeyword(pt, x_(" \t,)]"));
	if (st == NOSTRING) return(TRUE);
	*x = atola(st, 0);
	if (tonextchar(pt) != ',')
	{
		if (!quiet) ttyputerr(M_("Missing ',' in %s option"), name);
		return(TRUE);
	}
	st = getkeyword(pt, x_(" \t)]"));
	if (st == NOSTRING) return(TRUE);
	*y = atola(st, 0);
	if (tonextchar(pt) != ')')
	{
		if (!quiet) ttyputerr(M_("Missing ')' in size option"));
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to parse the "VALUE" part of a "VARIABLE=VALUE" clause.  The
 * string is at "pt", the "VARIABLE" is in "opt", the original string is
 * in "str", and the error verbose flag is in "quiet".  Returns the attribute
 * strucutre (NOATTR on error).
 */
ATTR *cli_getattr(CHAR *opt, CHAR **pt, CHAR *str, BOOLEAN quiet)
{
	REGISTER ATTR *a;
	REGISTER CHAR *st;

	a = (ATTR *)emalloc(sizeof (ATTR), el_tempcluster);
	if (a == 0) return(NOATTR);

	/* set keyword name */
	(void)allocstring(&a->name, opt, el_tempcluster);

	/* get value */
	if (seenextchar(pt) == '"')
	{
		/* quoted string */
		(void)tonextchar(pt);
		st = getkeyword(pt, x_("\""));
		if (st == NOSTRING) return(NOATTR);
		(void)allocstring((CHAR **)&a->value, st, el_tempcluster);
		a->type = VSTRING;
		(void)tonextchar(pt);
	} else
	{
		st = getkeyword(pt, x_(" \t,]"));
		if (st == NOSTRING) return(NOATTR);
		if (*st == 0)
		{
			if (!quiet) ttyputerr(M_("Missing value in '%s'"), str);
			return(NOATTR);
		}
		if (isanumber(st))
		{
			a->value = myatoi(st);
			a->type = VINTEGER;
		} else
		{
			(void)allocstring((CHAR **)&a->value, st, el_tempcluster);
			a->type = VSTRING;
		}
	}
	return(a);
}

/***************************** HIGHER-LEVEL *****************************/

/*
 * routine to find the node with the name "name" in the current graphics cell.
 * Returns that node (NONODEINST if not found).
 */
NODEINST *cli_findnodename(CHAR *name)
{
	REGISTER NODEINST *ni, *numni;
	REGISTER VARIABLE *var;

	for(ni = cli_curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var == NOVARIABLE) continue;
		if (namesame(name, (CHAR *)var->addr) == 0) return(ni);
	}

	/* look for the special case of "NODEdddd" */
	if (namesamen(name, x_("node"), 4) != 0) return(NONODEINST);
	numni = (NODEINST *)myatoi(&name[4]);
	for(ni = cli_curcell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni == numni) break;
	return(ni);
}

/*
 * routine to find the arc described by the line "line".  Returns that arc
 * (NOARCINST if not found).
 */
ARCINST *cli_findarcname(CHAR *line)
{
	NODEINST *nA, *nB;
	PORTPROTO *pA, *pB;
	ARCPROTO *ap;
	REGISTER CONNECTION *dcl;
	REGISTER ARCINST *ai, *pai;
	REGISTER INTBIG aend, bend;
	INTBIG wid;

	/* parse the line and get the endpoints */
	dcl = cli_parseconn(line, FALSE);
	if (dcl == NOCONNECTION) return(NOARCINST);
	if (cli_pickwire(dcl, &nA, &pA, &nB, &pB, &ap, &wid, NOARCINST, NONODEINST, NONODEINST))
	{
		cli_deleteconnection(dcl);
		return(NOARCINST);
	}

	/* look for this arc */
	pai = NOARCINST;
	for(ai = cli_curcell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		aend = bend = -1;
		if (ai->end[0].nodeinst == nA) aend = 0; else
			if (ai->end[1].nodeinst == nA) aend = 1;
		if (ai->end[1].nodeinst == nB) bend = 1; else
			if (ai->end[0].nodeinst == nB) bend = 0;
		if (aend < 0 || bend < 0) continue;
		if (aend == bend)
		{
			ttyputmsg(M_("To and From found on end %ld of arc %s"), aend, describearcinst(ai));
			continue;
		}
		if ((dcl->flag&PORT1VALID) != 0 &&
			ai->end[aend].portarcinst->proto != pA) continue;
		if ((dcl->flag&PORT2VALID) != 0 &&
			ai->end[bend].portarcinst->proto != pB) continue;
		if ((dcl->flag&LAYERVALID) != 0 && ai->proto != ap) continue;

		/* save a possible arc */
		if (pai == NOARCINST) pai = ai;

		/* check the width, just to be sure */
		if (ai->width != wid) continue;
		break;
	}
	cli_deleteconnection(dcl);
	if (ai == NOARCINST) ai = pai;
	return(ai);
}

/*
 * routine to evaluate the wire described by the structure "conn" and fill the
 * reference parameters "nA", "pA", "nB", "pB", "ap", and "wid".  If "ai" is
 * NOARCINST, use creation defaults, otherwise take defaults from that arcinst.
 * If "allownamechange" is nonzero, allow unrecognized node names (but set the
 * nodeinst pointer to NONODEINST).  Returns true if there is an error.
 */
BOOLEAN cli_pickwire(CONNECTION *dcl, NODEINST **nA, PORTPROTO **pA, NODEINST **nB, PORTPROTO **pB,
	ARCPROTO **ap, INTBIG *wid, ARCINST *defai, NODEINST *defnA, NODEINST *defnB)
{
	REGISTER INTBIG i, j;

	/* find node A */
	if ((dcl->flag&END1VALID) == 0)
	{
		if (defai != NOARCINST) *nA = defai->end[0].nodeinst; else
		{
			ttyputerr(M_("No end 0 specified"));
			return(TRUE);
		}
	} else
	{
		*nA = cli_findnodename(dcl->end1);
		if (*nA == NONODEINST)
		{
			if (defnA == NONODEINST)
			{
				ttyputerr(M_("Cannot find node %s"), dcl->end1);
				return(TRUE);
			}
			*nA = defnA;
		}
	}

	/* find node B */
	if ((dcl->flag&END2VALID) == 0)
	{
		if (defai != NOARCINST) *nB = defai->end[1].nodeinst; else
		{
			ttyputerr(M_("No end 1 specified"));
			return(TRUE);
		}
	} else
	{
		*nB = cli_findnodename(dcl->end2);
		if (*nB == NONODEINST)
		{
			if (defnB == NONODEINST)
			{
				ttyputerr(M_("Cannot find node %s"), dcl->end2);
				return(TRUE);
			}
			*nB = defnB;
		}
	}

	/* find the port on node A */
	if ((dcl->flag&PORT1VALID) == 0)
	{
		if (defai != NOARCINST) *pA = defai->end[0].portarcinst->proto; else
			*pA = (*nA)->proto->firstportproto;
	} else
	{
		*pA = getportproto((*nA)->proto, dcl->port1);
		if (*pA == NOPORTPROTO)
		{
			ttyputerr(M_("Cannot find port %s"), dcl->port1);
			return(TRUE);
		}
	}

	/* find the port on node B */
	if ((dcl->flag&PORT2VALID) == 0)
	{
		if (defai != NOARCINST) *pB = defai->end[1].portarcinst->proto; else
			*pB = (*nB)->proto->firstportproto;
	} else
	{
		*pB = getportproto((*nB)->proto, dcl->port2);
		if (*pB == NOPORTPROTO)
		{
			ttyputerr(M_("Cannot find port %s"), dcl->port2);
			return(TRUE);
		}
	}

	/* find layer for running the wire */
	if ((dcl->flag&LAYERVALID) == 0)
	{
		if (defai != NOARCINST) *ap = defai->proto; else
		{
			/* find a layer to use */
			for(i=0; (*pA)->connects[i] != NOARCPROTO; i++)
			{
				for(j=0; (*pB)->connects[j] != NOARCPROTO; j++)
					if ((*pA)->connects[i] == (*pB)->connects[j]) break;
				if ((*pB)->connects[j] != NOARCPROTO) break;
			}
			if ((*pA)->connects[i] == NOARCPROTO)
			{
				ttyputerr(M_("Cannot find connecting layer"));
				return(TRUE);
			}
			*ap = (*pA)->connects[i];
		}
	} else
	{
		*ap = getarcproto(dcl->layer);
		if (*ap == NOARCPROTO)
		{
			ttyputerr(M_("Cannot find connecting layer %s"), dcl->layer);
			return(TRUE);
		}
	}

	/* find the proper width */
	if ((dcl->flag&WIDTHVALID) == 0)
	{
		if (defai != NOARCINST) *wid = defai->width; else
			*wid = defaultarcwidth(*ap);
	} else *wid = dcl->width;

	return(FALSE);
}

/***************************** UTILITIES *****************************/

void cli_deletecomponentdec(COMPONENTDEC *dec)
{
	REGISTER COMPONENT *compo, *nextcomp;

	for(compo = dec->firstcomponent; compo != NOCOMPONENT; compo = nextcomp)
	{
		nextcomp = compo->nextcomponent;
		cli_freeattr(compo->firstattr);
		efree(compo->name);
		efree((CHAR *)compo);
	}
	if (dec->protoname != 0) efree(dec->protoname);
	efree((CHAR *)dec);
}

void cli_deleteconnection(CONNECTION *dec)
{
	REGISTER CONS *cons, *nextcons;

	for(cons = dec->firstcons; cons != NOCONS; cons = nextcons)
	{
		nextcons = cons->nextcons;
		efree((CHAR *)cons);
	}
	if ((dec->flag&LAYERVALID) != 0) efree(dec->layer);
	if ((dec->flag&END1VALID) != 0) efree(dec->end1);
	if ((dec->flag&PORT1VALID) != 0) efree(dec->port1);
	if ((dec->flag&END2VALID) != 0) efree(dec->end2);
	if ((dec->flag&PORT2VALID) != 0) efree(dec->port2);
	cli_freeattr(dec->firstattr);
	efree((CHAR *)dec);
}

void cli_deleteexport(EXPORT *e)
{
	if (e->portname != 0) efree(e->portname);
	if (e->component != 0) efree(e->component);
	if (e->subport != 0) efree(e->subport);
	cli_freeattr(e->firstattr);
	efree((CHAR *)e);
}

void cli_freeattr(ATTR *a)
{
	REGISTER ATTR *nexta;

	for( ; a != NOATTR; a = nexta)
	{
		nexta = a->nextattr;
		efree(a->name);
		if ((a->type&VTYPE) == VSTRING) efree((CHAR *)a->value);
		efree((CHAR *)a);
	}
}
