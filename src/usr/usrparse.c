/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrparse.c
 * User interface tool: command parsing and execution
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
#include "usr.h"
#include "usrdiacom.h"
#include "edialogs.h"

#define MAXLINE 400		/* maximum length of command file input lines */

/* working memory for "us_parsecommand()" */
static CHAR *us_parseownbuild[MAXPARS];
static INTBIG us_parsemaxownbuild = -1;

/* working memory for "us_evaluatevariable()" */
static INTBIG *us_evaluatevarlist;
static INTBIG  us_evaluatevarlimit = 0;

static DIALOGHOOK *us_firstdialoghook = NODIALOGHOOK;

/* global variables for parsing */
       KEYWORD *us_pathiskey;				/* the current keyword list */

/* local variables for parsing */
static INTBIG   us_pacurchar;				/* current character on current line */
static CHAR    *us_pattyline = 0;			/* current line image */
static CHAR    *us_paextra = 0;				/* extra chars that that can be appended as command completion */
static INTBIG   us_patruelength;			/* size of above two buffers */
static INTBIG   us_paparamcount;			/* number of parameters read in */
static INTBIG   us_paparamtotal;			/* total number of expected parameters with root parameter and null parameter */
static COMCOMP *us_paparamtype[MAXKEYWORD];		/* expected type of each parameter */
static CHAR    *us_paparamstart[MAXKEYWORD];		/* character position of each parameter */
static INTBIG   us_paparamadded[MAXKEYWORD];		/* number of parameters added here */
static BOOLEAN	us_painquote;				/* unclosed string at end of currnt line */
static COMCOMP  us_panullcomcomp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
				    0, x_(""), M_("too many parameters"), 0};
static INTBIG   us_pathisind;				/* pointer into the current keyword list */
static USERCOM *us_usercomstack = NOUSERCOM;    /* stack of temporary USERCOM's */

/* prototypes for local routines */
static BOOLEAN  us_addusercom(USERCOM*, CHAR*);
static USERCOM *us_fleshcommand(USERCOM*);
static void     us_dopopupmenu(POPUPMENU*);
static void     us_domacro(CHAR*, INTBIG, CHAR*[]);
static BOOLEAN  us_expandvarlist(INTBIG **varlist, INTBIG *varlimit);
static BOOLEAN  us_pahandlechar(void*, INTBIG);
static void     us_pabackup(INTBIG to_curchar);
static void     us_expandttybuffers(INTBIG);

/*
 * Routine to free all memory associated with this module.
 */
void us_freeparsememory(void)
{
	REGISTER USERCOM *uc;
	REGISTER INTBIG i;
	REGISTER MACROPACK *p;
	REGISTER DIALOGHOOK *dh;

	if (us_lastcom != NOUSERCOM) us_freeusercom(us_lastcom);
	us_lastcom = NOUSERCOM;
	while (us_usercomstack != NOUSERCOM)
	{
		uc = us_usercomstack;
		us_usercomstack = us_usercomstack->nextcom;
		us_freeusercom(uc);
	}

	while (us_usercomfree != NOUSERCOM)
	{
		uc = us_usercomfree;
		us_usercomfree = us_usercomfree->nextcom;
		efree((CHAR *)uc);
	}

	for(i=0; i<=us_parsemaxownbuild; i++)
		efree((CHAR *)us_parseownbuild[i]);

	/* free macro packages */
	while (us_macropacktop != NOMACROPACK)
	{
		p = us_macropacktop;
		us_macropacktop = us_macropacktop->nextmacropack;
		efree((CHAR *)p->packname);
		efree((CHAR *)p);
	}

	if (us_evaluatevarlimit > 0) efree((CHAR *)us_evaluatevarlist);

	/* free hooks to dialogs in other tools */
	while (us_firstdialoghook != NODIALOGHOOK)
	{
		dh = us_firstdialoghook;
		us_firstdialoghook = dh->nextdialoghook;
		if (dh->terminputkeyword != 0) efree((CHAR *)dh->terminputkeyword);
		efree((CHAR *)dh);
	}
	if (us_paextra != 0) efree(us_paextra);
	if (us_pattyline != 0) efree(us_pattyline);
}

/*
 * routine to allocate a new user command from the pool (if any) or memory
 */
USERCOM *us_allocusercom(void)
{
	REGISTER USERCOM *uc;

	if (us_usercomfree == NOUSERCOM)
	{
		uc = (USERCOM *)emalloc((sizeof (USERCOM)), us_tool->cluster);
		if (uc == 0) return(NOUSERCOM);
	} else
	{
		/* take usercom from free list */
		uc = us_usercomfree;
		us_usercomfree = (USERCOM *)uc->nextcom;
	}
	uc->active = -1;
	uc->comname = 0;
	uc->menu = NOPOPUPMENU;
	uc->message = 0;
	uc->nodeglyph = NONODEPROTO;
	uc->arcglyph = NOARCPROTO;
	uc->count = 0;
	uc->nextcom = NOUSERCOM;
	return(uc);
}

/*
 * routine to return user command "uc" to the pool of free commands
 */
void us_freeusercom(USERCOM *uc)
{
	REGISTER INTBIG i;

	uc->nextcom = us_usercomfree;
	for(i=0; i<uc->count; i++) efree(uc->word[i]);
	if (uc->comname != 0) efree(uc->comname);
	if (uc->message != 0) efree(uc->message);
	us_usercomfree = uc;
}

/*
 * routine to establish a new macro package with name "name"
 */
MACROPACK *us_newmacropack(CHAR *name)
{
	REGISTER MACROPACK *p;

	p = (MACROPACK *)emalloc((sizeof (MACROPACK)), us_tool->cluster);
	if (p == 0) return(NOMACROPACK);
	if (allocstring(&p->packname, name, us_tool->cluster)) return(NOMACROPACK);
	p->nextmacropack = us_macropacktop;
	us_macropacktop = p;
	return(p);
}

/*
 * routine to add user command "uc" to macro "mac".  Returns true upon error
 */
BOOLEAN us_addusercom(USERCOM *uc, CHAR *mac)
{
	REGISTER INTBIG i, parnumber, parsed, len, count, total, j, tot, hasspace;
	REGISTER CHAR *pt, **manystrings;
	COMCOMP *comarray[MAXPARS];
	CHAR *build[MAXPARS], line[30];
	REGISTER VARIABLE *var;
	extern COMCOMP us_macparamp;
	REGISTER void *infstr, *subinfstr;

	var = us_getmacro(mac);
	if (var == NOVARIABLE) return(TRUE);
	len = getlength(var);

	/* make room for one more command */
	manystrings = (CHAR **)emalloc(((len+1) * (sizeof (CHAR *))), el_tempcluster);
	if (manystrings == 0) return(TRUE);

	/* load the existing commands */
	for(i=0; i<len; i++)
		(void)allocstring(&manystrings[i], ((CHAR **)var->addr)[i], el_tempcluster);

	/* build the new command */
	parsed = 0;
	tot = 0;
	infstr = initinfstr();
	addstringtoinfstr(infstr, uc->comname);
	addtoinfstr(infstr, ' ');
	for(i=0; i<uc->count; i++)
	{
		/* look for parameter substitution */
		hasspace = 0;
		for(pt = uc->word[i]; *pt != 0; pt++)
		{
			if (*pt == ' ' || *pt == '\t') hasspace++;
			if (*pt != '%') continue;
			if (!isdigit(pt[1])) continue;

			/* formal parameter found: determine command completion for it */
			parnumber = pt[1] - '0';
			if (parsed == 0)
			{
				tot = us_fillcomcomp(uc, comarray);
				parsed++;
			}
			count = us_parsecommand(manystrings[2], build);
			total = maxi(count, parnumber);
			subinfstr = initinfstr();
			for(j=0; j<total; j++)
			{
				if (j == parnumber-1 && tot > i && comarray[i] != NOCOMCOMP)
				{
					(void)esnprintf(line, 30, x_("0%lo"), (UINTBIG)comarray[i]);
					addstringtoinfstr(subinfstr, line);
				} else if (j < count) addstringtoinfstr(subinfstr, build[j]); else
				{
					(void)esnprintf(line, 30, x_("0%lo"), (UINTBIG)&us_macparamp);
					addstringtoinfstr(subinfstr, line);
				}
				addtoinfstr(subinfstr, ' ');
			}
			(void)reallocstring(&manystrings[2], returninfstr(subinfstr), el_tempcluster);
		}

		if (hasspace == 0) addstringtoinfstr(infstr, uc->word[i]); else
		{
			/* must quote this parameter: it has spaces */
			addtoinfstr(infstr, '"');
			for(pt = uc->word[i]; *pt != 0; pt++)
			{
				if (*pt == '"') addtoinfstr(infstr, '"');
				addtoinfstr(infstr, *pt);
			}
			addtoinfstr(infstr, '"');
		}
		addtoinfstr(infstr, ' ');
	}

	manystrings[len] = returninfstr(infstr);
	(void)setvalkey((INTBIG)us_tool, VTOOL, var->key, (INTBIG)manystrings,
		VSTRING|VISARRAY|((len+1)<<VLENGTHSH)|VDONTSAVE);
	for(i=0; i<len; i++) efree(manystrings[i]);
	efree((CHAR *)manystrings);
	return(FALSE);
}

/*
 * routine to examine the command in "uc" and fill the command completion array
 * "comarray" with appropriate entries for parsing the string.  Returns the
 * number of valid entries in the array.
 */
INTBIG us_fillcomcomp(USERCOM *uc, COMCOMP *comarray[])
{
	REGISTER INTBIG j, k, com, pos, curtotal, count;
	REGISTER CHAR *pt;
	CHAR *build[MAXPARS];
	COMCOMP *nextparams[MAXPARS+5];
	REGISTER VARIABLE *var;

	comarray[0] = NOCOMCOMP;
	com = uc->active;

	/* if the command is not valid, exit now */
	if (com < 0) return(0);

	/* handle normal commands */
	if (com < us_longcount)
	{
		/* initialize the command completion array with the default parameters */
		for(curtotal=0; curtotal<us_lcommand[com].params; curtotal++)
			comarray[curtotal] = us_lcommand[com].par[curtotal];
	} else
	{
		/* ignore popup menus */
		if (uc->menu != NOPOPUPMENU) return(0);

		/* handle macros */
		var = us_getmacro(uc->comname);
		if (var == NOVARIABLE) return(0);
		pt = ((CHAR **)var->addr)[2];
		count = us_parsecommand(pt, build);
		for(curtotal=0; curtotal<count; curtotal++)
			comarray[curtotal] = (COMCOMP *)myatoi(build[curtotal]);
	}
	comarray[curtotal] = NOCOMCOMP;

	/* now run through the parameters, inserting whatever is appropriate */
	for(pos=0; comarray[pos] != NOCOMCOMP; pos++)
	{
		if (pos >= uc->count) break;

		/* load the necessary global and get the next parameters */
		us_pathiskey = comarray[pos]->ifmatch;
		k = (*comarray[pos]->params)(uc->word[pos], nextparams, ' ');

		/* clip to the number of allowable parameters */
		if (k+curtotal >= MAXPARS-1) k = MAXPARS-2-curtotal;

		/* spread open the list */
		for(j = MAXPARS-k-1; j >= curtotal; j--) comarray[j+k] = comarray[j];

		/* fill in the parameter types */
		for(j = 0; j < k; j++) comarray[curtotal+j] = nextparams[j];
		curtotal += k;
	}
	return(curtotal);
}

/*
 * routine to execute user command "item".  The command is echoed if "print"
 * is true.  If "macro" is true, the command is saved in the
 * macro.  If "fromuser" is true, the command was issued from the
 * terminal and is saved in the 1-command stack "us_lastcom"
 * "item" is saved in "us_usercomstack" and is properly deleted on shutdown.
 */
void us_executesafe(USERCOM *item, BOOLEAN print, BOOLEAN macro, BOOLEAN fromuser)
{
	item->nextcom = us_usercomstack;
	us_usercomstack = item;
	us_execute(item, print, macro, fromuser);
	if (us_usercomstack == item)
		us_usercomstack = item->nextcom;
	else
		us_usercomstack = NOUSERCOM;
}

/*
 * routine to execute user command "item".  The command is echoed if "print"
 * is true.  If "macro" is true, the command is saved in the
 * macro.  If "fromuser" is true, the command was issued from the
 * terminal and is saved in the 1-command stack "us_lastcom"
 */
void us_execute(USERCOM *item, BOOLEAN print, BOOLEAN macro, BOOLEAN fromuser)
{
	REGISTER INTBIG i, j, runit, act, ismacend, count, wasmacro;
	REGISTER USERCOM *uc, *ucnew;
	CHAR *userpars[MAXPARS];
	REGISTER VARIABLE *var, *varmacro;
	REGISTER void *infstr;

	/* for segmentation fault correction */
	us_state &= ~COMMANDFAILED;

	/* stop if interrupted */
	if (stopping(STOPREASONEXECUTE)) return;

	/* see if the status display should be cleared */
	if ((us_state&DIDINPUT) != 0)
	{
		ttynewcommand();
		us_state &= ~DIDINPUT;
	}

	/* see if the command is valid */
	act = item->active;
	if (act < 0)
	{
		us_unknowncommand();
		return;
	}

	/* copy the command */
	uc = us_allocusercom();
	if (uc != NOUSERCOM)
	{
		uc->active = act;
		(void)allocstring(&uc->comname, item->comname, us_tool->cluster);
		uc->menu = item->menu;
		uc->count = item->count;
		for(j=0; j<item->count; j++)
			if (allocstring(&uc->word[j], item->word[j], us_tool->cluster))
		{
			uc = NOUSERCOM;
			break;
		}
		if (uc != NOUSERCOM)
		{
			uc->nextcom = us_usercomstack;
			us_usercomstack = uc;
		}
	}
	if (uc == NOUSERCOM)
	{
		/*
		 * when there is no memory, things are basically wedged.  This
		 * little hack lets the command be executed but may cause all
		 * kinds of hell in other places.
		 */
		ttyputnomemory();
		us_usercomstack = NOUSERCOM;
		uc = item;
	}

	/* if this is a macro definition, see if the command is to be executed */
	runit = 1;
	ismacend = 0;
	wasmacro = 0;
	if (macro)
	{
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
		if (var != NOVARIABLE)
		{
			/* see if macro was begun with the "no-execute" option */
			varmacro = us_getmacro((CHAR *)var->addr);
			if (varmacro != NOVARIABLE)
			{
				wasmacro++;
				count = us_parsecommand(((CHAR **)varmacro->addr)[0], userpars);
				for(i=0; i<count; i++)
					if (namesame(userpars[i], x_("noexecute")) == 0)
				{
					runit = 0;
					break;
				}
			}

			/* see if a "macdone" was encountered */
			if ((us_state&MACROTERMINATED) != 0) runit = 0;

			/* always execute the "macend" command */
			if (act < us_longcount &&
				estrcmp(us_lcommand[act].name, x_("macend")) == 0) ismacend = runit = 1;
		}
	}

	ucnew = NOUSERCOM;
	if (runit)
	{
		/* expand "%" macros, "$" variables, and "^" quotes in command line */
		ucnew = us_fleshcommand(uc);
		if (ucnew != uc)
		{
			ucnew->nextcom = us_usercomstack;
			us_usercomstack = ucnew;
		}

		/* print the command if requested */
		if (print)
		{
			infstr = initinfstr();
			addtoinfstr(infstr, '-');
			addstringtoinfstr(infstr, ucnew->comname);
			us_appendargs(infstr, ucnew);
			ttyputmsg(x_("%s"), returninfstr(infstr));
		}

		/* handle normal commands */
		if (act < us_longcount)
		{
			/* make copy of parameter pointers in case user changes it */
			for(i=0; i<ucnew->count; i++) userpars[i] = ucnew->word[i];

			/* execute the command */
			(*us_lcommand[act].routine)(ucnew->count, userpars);
		} else
		{
			/* if command is a popup menu, execute it differently */
			if (ucnew->menu != NOPOPUPMENU) us_dopopupmenu(ucnew->menu); else
			{
				/* presume macro */
				us_domacro(ucnew->comname, ucnew->count, ucnew->word);
			}
		}
	} else ttyputmsg(_("(command not executed)"));
	if (ucnew != uc && ucnew != NOUSERCOM)
	{
		if (us_usercomstack == ucnew)
			us_usercomstack = ucnew->nextcom;
		else
			us_usercomstack = NOUSERCOM;
		us_freeusercom(ucnew);
	}

	/* save the command in a macro if requested */
	if (wasmacro != 0 && (us_state&COMMANDFAILED) == 0 && ismacend == 0)
	{
		var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
		if (var != NOVARIABLE)
			if (us_addusercom(uc, (CHAR *)var->addr))
				ttyputnomemory();
	}

	/* if command had an error, set flag to not save it */
	if ((us_state&COMMANDFAILED) != 0) fromuser = FALSE;

	/* if command was "iterate", don't save it in iteration stack */
	if (fromuser)
	{
		if (act < us_longcount && estrcmp(us_lcommand[act].name, x_("iterate")) == 0)
			fromuser = FALSE;
	}

	/* if command is to be saved for iteration, do so */
	if (us_usercomstack == uc)
		us_usercomstack = uc->nextcom;
	else
		us_usercomstack = NOUSERCOM;
	if (fromuser)
	{
		if (us_lastcom != NOUSERCOM) us_freeusercom(us_lastcom);
		us_lastcom = uc;
	} else
	{
		us_freeusercom(uc);
	}
}

/*
 * routine to convert command "uc" into a fully fleshed-out command (with all
 * "%" macros, "$" variables, and "^" quoted characters properly converted).
 */
USERCOM *us_fleshcommand(USERCOM *uc)
{
	REGISTER USERCOM *ucnew;
	REGISTER BOOLEAN err;
	REGISTER INTBIG j, l, letter, aindex, indexsave, hasparams, count, chr;
	REGISTER VARIABLE *var, *varrunning, *varbuilding;
	VARIABLE localvar;
	REGISTER CHAR *pp, *closeparen, *pars;
	CHAR *qual, *tmpstring, *params[MAXPARS];
	INTBIG objaddr, objtype;
	REGISTER void *infstr;

	/* look for macro/variable/quote expansion possibilities */
	hasparams = 0;
	for(j=0; j<uc->count; j++)
	{
		for(pp = uc->word[j]; *pp != 0; pp++)
			if (*pp == '%' || *pp == '$' || *pp == '^') hasparams++;
		if (hasparams != 0) break;
	}
	if (hasparams == 0) return(uc);

	/* make a new user command */
	ucnew = us_allocusercom();
	if (ucnew == NOUSERCOM) return(uc);
	ucnew->active = uc->active;
	(void)allocstring(&ucnew->comname, uc->comname, us_tool->cluster);
	ucnew->menu = uc->menu;
	ucnew->count = uc->count;

	/* run through each parameter */
	err = FALSE;
	for(j=0; j<ucnew->count; j++)
	{
		/* copy the original parameter */
		err |= allocstring(&tmpstring, uc->word[j], el_tempcluster);

		/* expand macro if it has parameters */
		for(infstr = initinfstr(), pp = tmpstring; *pp != 0; pp++)
		{
			/* handle escape character */
			if (*pp == '^')
			{
				addtoinfstr(infstr, *pp++);
				if (*pp != 0) addtoinfstr(infstr, *pp);
				continue;
			}

			/* if not a command interpreter macro character, just copy it */
			if (*pp != '%') { addtoinfstr(infstr, *pp);  continue; }
			pp++;

			/* handle command interpreter variables */
			if (isalpha(*pp))
			{
				/* handle array indices */
				aindex = -1;
				indexsave = l = 0;
				if (pp[1] == '[') for(l=2; pp[l] != 0; l++) if (pp[l] == ']')
				{
					aindex = myatoi(&pp[2]);
					indexsave = pp[1];
					pp[1] = 0;
					break;
				}

				var = getval((INTBIG)us_tool, VTOOL, -1, us_commandvarname(*pp));
				if (var != NOVARIABLE)
					addstringtoinfstr(infstr, describevariable(var, aindex, -1));
				if (aindex >= 0)
				{
					pp[1] = (CHAR)indexsave;
					pp += l;
				}
				continue;
			}

			/* if "%" form is parameter substitution, do that */
			if (isdigit(*pp))
			{
				/* substitute macro parameter */
				varrunning = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrorunningkey);
				varbuilding = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
				if (varrunning != NOVARIABLE && (varbuilding == NOVARIABLE ||
					namesame((CHAR *)varrunning->addr, (CHAR *)varbuilding->addr) != 0))
				{
					/* find the number of parameters to this macro */
					var = us_getmacro((CHAR *)varrunning->addr);
					if (var == NOVARIABLE) count = 0; else
						count = us_parsecommand(((CHAR **)var->addr)[1], params);
					letter = *pp - '1';
					if (letter < count)
					{
						/* remove enclosing quotes on parameter */
						pars = params[letter];
						if (pars[0] == '"' && pars[estrlen(pars)-1] == '"')
						{
							pars[estrlen(pars)-1] = 0;
							pars++;
						}

						/* add the parameter, removing quotes */
						for(; *pars != 0; pars++)
						{
							if (pars[0] == '"' && pars[1] == '"') pars++;
							addtoinfstr(infstr, *pars);
						}

						/* skip the default clause, if any */
						if (pp[1] == '[')
							for(pp += 2; *pp != ']' && *pp != 0; pp++)
								;
						continue;
					}
				}

				/* use the default clause, if any */
				if (pp[1] == '[')
					for(pp += 2; *pp != ']' && *pp != 0; pp++) addtoinfstr(infstr, *pp);
				continue;
			}

			/* give up on substitution: leave the construct as is */
			addtoinfstr(infstr, *pp);
		}
		if (reallocstring(&tmpstring, returninfstr(infstr), el_tempcluster)) err = TRUE;

		/* substitute any "$" variable references in the user command */
		for(infstr = initinfstr(), pp = tmpstring; *pp != 0; pp++)
		{
			/* handle escape character */
			if (*pp == '^')
			{
				addtoinfstr(infstr, *pp++);
				if (*pp != 0) addtoinfstr(infstr, *pp);
				continue;
			}

			/* if not a variable character, just copy it */
			if (*pp != '$') { addtoinfstr(infstr, *pp);  continue; }

			/* if variable expression is in parenthesis, determine extent */
			pp++;
			closeparen = 0;
			if (*pp == '(')
			{
				for(l=0; pp[l] != 0; l++) if (pp[l] == ')') break;
				if (pp[l] == ')')
				{
					closeparen = &pp[l];
					pp[l] = 0;
					pp++;
				}
			}

			/* get index of variable (if specified) */
			aindex = -1;
			l = estrlen(pp);
			if (pp[l-1] == ']') for(l--; l >= 0; l--) if (pp[l] == '[')
			{
				aindex = myatoi(&pp[l+1]);
				break;
			}
			if (!us_evaluatevariable(pp, &objaddr, &objtype, &qual))
			{
				if (*qual != 0) var = getval(objaddr, objtype, -1, qual); else
				{
					localvar.addr = objaddr;
					localvar.type = objtype;
					TDCLEAR(localvar.textdescript);
					var = &localvar;
				}
				if (var != NOVARIABLE) addstringtoinfstr(infstr, describevariable(var, aindex, -1));
			}
			if (closeparen != 0)
			{
				*closeparen = ')';
				pp = closeparen;
			} else break;
		}
		if (reallocstring(&tmpstring, returninfstr(infstr), el_tempcluster)) err = TRUE;

		/* remove any "^" escape references in the user command */
		for(infstr = initinfstr(), pp = tmpstring; *pp != 0; pp++)
		{
			if (*pp != '^') addtoinfstr(infstr, *pp); else
			{
				pp++;
				if (*pp == 0)
				{
					ttyputerr(_("HEY! '^' at end of line"));
					break;
				}
				chr = *pp;

				/* check for numeric equivalent of the form "^010" */
				if (chr == '0')
				{
					pp++;
					chr = 0;
					while (*pp >= '0' && *pp <= '7')
					{
						chr = (chr * 8) + (*pp - '0');
						pp++;
					}
					pp--;
				}
				addtoinfstr(infstr, (CHAR)chr);
			}
		}
		if (allocstring(&ucnew->word[j], returninfstr(infstr), us_tool->cluster)) err = TRUE;
		efree(tmpstring);
	}
	if (err) ttyputnomemory();
	return(ucnew);
}

/*
 * routine to expand a variable path in the string "name" and return a full
 * description of how to find that variable by setting "objaddr" and "objtype"
 * to the address and type of the object containing the variable "qual".
 * The routine returns true if the variable path is nonsensical.
 */
BOOLEAN us_evaluatevariable(CHAR *name, INTBIG *objaddr, INTBIG *objtype, CHAR **qual)
{
	REGISTER INTBIG i, len;
	INTBIG lx, hx, ly, hy, x, y;
	REGISTER CHAR *pp, save, *type;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER LIBRARY *lib;
	REGISTER PORTPROTO *port;
	REGISTER HIGHLIGHT *high;
	REGISTER TECHNOLOGY *tech;
	REGISTER VARIABLE *var, *highvar;
	REGISTER VIEW *view;
	REGISTER NETWORK *net;
	REGISTER WINDOWPART *win;
	GEOM *fromgeom, *togeom;
	PORTPROTO *fromport, *toport;
	HIGHLIGHT thishigh;
	REGISTER INTBIG varcount, *list;
	REGISTER void *infstr;

	/* look for specifier with ":" */
	for(pp = name;
		*pp != 0 && *pp != ':' && *pp != '.' && *pp != ')' && *pp != '['; pp++)
			;
	if (*pp == ':')
	{
		i = pp - name;
		type = name;
		for(name = ++pp;
			*name != 0 && *name != '.' && *name != ')' && *name != '['; name++)
				;
		save = *name;    *name = 0;
		if (namesamen(type, x_("technology"), i) == 0 && i >= 1)
		{
			*objtype = VTECHNOLOGY;
			if (estrcmp(pp, x_("~")) == 0) tech = el_curtech; else
				tech = gettechnology(pp);
			*name = save;
			if (tech == NOTECHNOLOGY) return(TRUE);
			*objaddr = (INTBIG)tech;
		} else if (namesamen(type, x_("library"), i) == 0 && i >= 1)
		{
			*objtype = VLIBRARY;
			if (estrcmp(pp, x_("~")) == 0) lib = el_curlib; else lib = getlibrary(pp);
			*name = save;
			if (lib == NOLIBRARY) return(TRUE);
			*objaddr = (INTBIG)lib;
		} else if (namesamen(type, x_("tool"), i) == 0 && i >= 2)
		{
			*objtype = VTOOL;
			if (estrcmp(pp, x_("~")) == 0) i = us_tool->toolindex; else
				for(i=0; i<el_maxtools; i++)
					if (namesame(el_tools[i].toolname, pp) == 0) break;
			*name = save;
			if (i >= el_maxtools) return(TRUE);
			*objaddr = (INTBIG)&el_tools[i];
		} else if (namesamen(type, x_("arc"), i) == 0 && i >= 2)
		{
			*objtype = VARCPROTO;
			if (estrcmp(pp, x_("~")) == 0) ap = us_curarcproto; else
				for(ap = el_curtech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
					if (namesame(ap->protoname, pp) == 0) break;
			*name = save;
			if (ap == NOARCPROTO) return(TRUE);
			*objaddr = (INTBIG)ap;
		} else if (namesamen(type, x_("node"), i) == 0 && i >= 2)
		{
			*objtype = VNODEPROTO;
			if (estrcmp(pp, x_("~")) == 0) np = us_curnodeproto; else
				np = getnodeproto(pp);
			*name = save;
			if (np == NONODEPROTO) return(TRUE);
			*objaddr = (INTBIG)np;
		} else if (namesamen(type, x_("primitive"), i) == 0 && i >= 1)
		{
			*objtype = VNODEPROTO;
			if (estrcmp(pp, x_("~")) == 0) np = us_curnodeproto; else
				np = getnodeproto(pp);
			if (np != NONODEPROTO && np->primindex == 0) np = NONODEPROTO;
			*name = save;
			if (np == NONODEPROTO) return(TRUE);
			*objaddr = (INTBIG)np;
		} else if (namesamen(type, x_("cell"), i) == 0 && i >= 1)
		{
			*objtype = VNODEPROTO;
			if (estrcmp(pp, x_("~")) == 0) np = getcurcell(); else
			{
				np = getnodeproto(pp);
				if (np != NONODEPROTO && np->primindex != 0) np = NONODEPROTO;
			}
			*name = save;
			if (np == NONODEPROTO) return(TRUE);
			*objaddr = (INTBIG)np;
		} else if (namesamen(type, x_("view"), i) == 0 && i >= 1)
		{
			*objtype = VVIEW;
			view = getview(pp);
			*name = save;
			if (view == NOVIEW) return(TRUE);
			*objaddr = (INTBIG)view;
		} else if (namesamen(type, x_("window"), i) == 0 && i >= 1)
		{
			*objtype = VWINDOWPART;
			if (estrcmp(pp, x_("~")) == 0) win = el_curwindowpart; else
			{
				for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
					if (namesame(pp, win->location) == 0) break;
			}
			*name = save;
			if (win == NOWINDOWPART) return(TRUE);
			*objaddr = (INTBIG)win;
		} else if (namesamen(type, x_("net"), i) == 0 && i >= 2)
		{
			*objtype = VNETWORK;
			np = getcurcell();
			if (np == NONODEPROTO) return(TRUE);
			net = getnetwork(pp, np);
			*name = save;
			if (net == NONETWORK) return(TRUE);
			*objaddr = (INTBIG)net;
		} else
		{
			*name = save;
			return(TRUE);
		}
		if (*name == '.') name++;
	} else
	{
		if (*name == '~')
		{
			name++;

			/* handle cases that do not need current selection */
			if (*name == 'x' || *name == 'y')
			{
				/* special case when evaluating cursor coordinates */
				if (us_demandxy(&x, &y)) return(TRUE);
				np = el_curlib->curnodeproto;
				gridalign(&x, &y, 1, np);
				*objtype = VINTEGER;
				if (*name == 'x') *objaddr = x; else *objaddr = y;
				name++;
			} else if (*name == 't')
			{
				/* figure out the current technology */
				*objtype = VSTRING;
				*objaddr = (INTBIG)x_("");
				np = el_curlib->curnodeproto;
				if (np != NONODEPROTO && (np->cellview->viewstate&TEXTVIEW) == 0)
					*objaddr = (INTBIG)us_techname(np);
				name++;
			} else if (*name == 'c')
			{
				list = us_getallhighlighted();
				for(i=0; list[i+1] != 0; i += 2) ;
				*objtype = VINTEGER;
				*objaddr = i / 2;
				name++;
			} else if (*name == 'h')
			{
				name++;
				if (us_getareabounds(&lx, &hx, &ly, &hy) == NONODEPROTO) return(TRUE);
				*objtype = VINTEGER;
				switch (*name)
				{
					case 'l': *objaddr = lx;   name++;   break;
					case 'r': *objaddr = hx;   name++;   break;
					case 'b': *objaddr = ly;   name++;   break;
					case 't': *objaddr = hy;   name++;   break;
					default:  *objtype = VUNKNOWN;       break;
				}
			} else if (*name == 'o')
			{
				name++;
				if (us_gettwoobjects(&fromgeom, &fromport, &togeom, &toport)) return(TRUE);
				*objaddr = (INTBIG)togeom->entryaddr.blind;
				if (togeom->entryisnode) *objtype = VNODEINST; else
					*objtype = VARCINST;
			} else if (namesamen(name, x_("nodes"), 5) == 0)
			{
				/* construct a list of the nodes in this cell */
				name += 5;
				np = getcurcell();
				if (np == NONODEPROTO) return(TRUE);
				varcount = 0;
				for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (varcount >= us_evaluatevarlimit)
						if (us_expandvarlist(&us_evaluatevarlist, &us_evaluatevarlimit)) return(TRUE);
					us_evaluatevarlist[varcount*2] = (INTBIG)ni;
					us_evaluatevarlist[varcount*2+1] = VNODEINST;
					varcount++;
				}
				*objaddr = (INTBIG)us_evaluatevarlist;
				*objtype = VGENERAL | VISARRAY | ((varcount*2) << VLENGTHSH) | VDONTSAVE;
			} else if (namesamen(name, x_("arcs"), 4) == 0)
			{
				/* construct a list of the arcs in this cell */
				name += 4;
				np = getcurcell();
				if (np == NONODEPROTO) return(TRUE);
				varcount = 0;
				for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
				{
					if (varcount >= us_evaluatevarlimit)
						if (us_expandvarlist(&us_evaluatevarlist, &us_evaluatevarlimit)) return(TRUE);
					us_evaluatevarlist[varcount*2] = (INTBIG)ai;
					us_evaluatevarlist[varcount*2+1] = VARCINST;
					varcount++;
				}
				*objaddr = (INTBIG)us_evaluatevarlist;
				*objtype = VGENERAL | VISARRAY | ((varcount*2) << VLENGTHSH) | VDONTSAVE;
			} else if (namesamen(name, x_("ports"), 5) == 0)
			{
				/* construct a list of the ports in this cell */
				name += 5;
				np = getcurcell();
				if (np == NONODEPROTO) return(TRUE);
				varcount = 0;
				for(port = np->firstportproto; port != NOPORTPROTO; port = port->nextportproto)
				{
					if (varcount >= us_evaluatevarlimit)
						if (us_expandvarlist(&us_evaluatevarlist, &us_evaluatevarlimit)) return(TRUE);
					us_evaluatevarlist[varcount*2] = (INTBIG)port;
					us_evaluatevarlist[varcount*2+1] = VPORTPROTO;
					varcount++;
				}
				*objaddr = (INTBIG)us_evaluatevarlist;
				*objtype = VGENERAL | VISARRAY | ((varcount*2) << VLENGTHSH) | VDONTSAVE;
			} else if (namesamen(name, x_("nets"), 4) == 0)
			{
				/* construct a list of the networks in this cell */
				name += 4;
				np = getcurcell();
				if (np == NONODEPROTO) return(TRUE);
				varcount = 0;
				for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				{
					if (varcount >= us_evaluatevarlimit)
						if (us_expandvarlist(&us_evaluatevarlist, &us_evaluatevarlimit)) return(TRUE);
					us_evaluatevarlist[varcount*2] = (INTBIG)net;
					us_evaluatevarlist[varcount*2+1] = VNETWORK;
					varcount++;
				}
				*objaddr = (INTBIG)us_evaluatevarlist;
				*objtype = VGENERAL | VISARRAY | ((varcount*2) << VLENGTHSH) | VDONTSAVE;
			} else if (namesamen(name, x_("cells"), 6) == 0)
			{
				/* construct a list of the cells in this library */
				name += 6;
				varcount = 0;
				for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					if (varcount >= us_evaluatevarlimit)
						if (us_expandvarlist(&us_evaluatevarlist, &us_evaluatevarlimit)) return(TRUE);
					us_evaluatevarlist[varcount*2] = (INTBIG)np;
					us_evaluatevarlist[varcount*2+1] = VNODEPROTO;
					varcount++;
				}
				*objaddr = (INTBIG)us_evaluatevarlist;
				*objtype = VGENERAL | VISARRAY | ((varcount*2) << VLENGTHSH) | VDONTSAVE;
			} else if (namesamen(name, x_("libs"), 4) == 0)
			{
				/* construct a list of the libraries */
				name += 4;
				varcount = 0;
				for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				{
					if (varcount >= us_evaluatevarlimit)
						if (us_expandvarlist(&us_evaluatevarlist, &us_evaluatevarlimit)) return(TRUE);
					us_evaluatevarlist[varcount*2] = (INTBIG)lib;
					us_evaluatevarlist[varcount*2+1] = VLIBRARY;
					varcount++;
				}
				*objaddr = (INTBIG)us_evaluatevarlist;
				*objtype = VGENERAL | VISARRAY | ((varcount*2) << VLENGTHSH) | VDONTSAVE;
			} else
			{
				/* basic "~" with nothing after it requires the current selection, so get it */
				list = us_getallhighlighted();

				/* if there are no objects selected, stop now */
				if (list[1] == 0) return(TRUE);

				highvar = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
				if (highvar == NOVARIABLE) return(TRUE);
				if (us_makehighlight(((CHAR **)highvar->addr)[0], &thishigh)) return(TRUE);
				high = &thishigh;

				/* copy the selected object to the parameters */
				*objaddr = list[0];
				*objtype = list[1];

				/* get variable name if one is selected */
				if ((high->status&HIGHTYPE) == HIGHTEXT && high->fromvar != NOVARIABLE)
				{
					*qual = makename(high->fromvar->key);
					if (*name == 0 || *name == ')' || *name == '[') return(FALSE);
					return(TRUE);
				}
			}
			if (*name == '.') name++;
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO) return(TRUE);
			*objaddr = (INTBIG)np;
			*objtype = VNODEPROTO;
		}
	}

	/* add in qualifiers */
	save = *name;
	for(pp = name; *name != 0;)
	{
		/* advance to the end of the next qualifying name */
		while (*name != 0 && *name != '.' && *name != ')' && *name != '[') name++;

		/* save the terminating character */
		save = *name;   *name = 0;

		/* special case if this is an array */
		if (((*objtype) & VISARRAY) != 0 && ((*objtype)&VTYPE) == VGENERAL)
		{
			len = (((*objtype) & VLENGTH) >> VLENGTHSH) / 2;
			for(i=0; i<len; i++)
			{
				/* climb down the context chain */
				var = getval(((INTBIG *)*objaddr)[i*2], ((INTBIG *)*objaddr)[i*2+1], -1, pp);

				/* if chain cannot be climbed, path is invalid */
				if (var == NOVARIABLE) ((INTBIG *)*objaddr)[i*2+1] = VUNKNOWN; else
				{
					/* keep on going down the path */
					((INTBIG *)*objaddr)[i*2] = var->addr;
					((INTBIG *)*objaddr)[i*2+1] = var->type;
				}
			}
			*name++ = save;
		} else
		{
			/* if this is the end, return now */
			if (save != '.') break;

			/* climb down the context chain */
			var = getval(*objaddr, *objtype, -1, pp);
			*name++ = save;

			/* if chain cannot be climbed, path is invalid */
			if (var == NOVARIABLE) return(TRUE);

			/* keep on going down the path */
			*objaddr = var->addr;   *objtype = var->type;
		}

		pp = name;
	}

	/* get the final qualifying name */
	if (((*objtype)&VTYPE) == VGENERAL) *qual = x_(""); else
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, pp);
		*qual = returninfstr(infstr);
	}
	*name = save;
	return(FALSE);
}

/*
 * routine to maintain a static list of address/type pairs
 */
BOOLEAN us_expandvarlist(INTBIG **varlist, INTBIG *varlimit)
{
	INTBIG *newlist, newlimit, i;

	newlimit = *varlimit * 2;
	if (newlimit <= 0) newlimit = *varlimit + 10;
	newlist = (INTBIG *)emalloc(newlimit * 2 * SIZEOFINTBIG, us_tool->cluster);
	if (newlist == 0) return(TRUE);
	for(i=0; i < (*varlimit)*2; i++) newlist[i] = (*varlist)[i];
	if (*varlimit != 0) efree((CHAR *)*varlist);
	*varlist = newlist;
	*varlimit = newlimit;
	return(FALSE);
}

/*
 * routine to ensure that the INTBIG arrays "addr" and "type" are at least "len"
 * long (current size is "limit").  Reallocates if not.  Returns true on error.
 */
BOOLEAN us_expandaddrtypearray(INTBIG *limit, INTBIG **addr, INTBIG **type, INTBIG len)
{
	if (len <= *limit) return(FALSE);

	if (*limit > 0) { efree((CHAR *)*addr);   efree((CHAR *)*type); }
	*limit = 0;
	*addr = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
	if (*addr == 0) return(TRUE);
	*type = (INTBIG *)emalloc(len * SIZEOFINTBIG, us_tool->cluster);
	if (*type == 0) return(TRUE);
	*limit = len;
	return(FALSE);
}

/*
 * routine to execute popup menu "pm"
 */
void us_dopopupmenu(POPUPMENU *pm)
{
	REGISTER INTBIG i;
	REGISTER POPUPMENUITEM *mi, *miret;
	POPUPMENU *cpm;
	REGISTER USERCOM *uc;
	BOOLEAN butstate, verbose;
	REGISTER void *infstr;

	/* initialize the popup menu */
	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		if (mi->maxlen < 0) mi->value = 0; else
		{
			mi->value = (CHAR *)emalloc((mi->maxlen+1) * SIZEOFCHAR, el_tempcluster);
			if (mi->value == 0)
			{
				ttyputnomemory();
				return;
			}
			(void)estrcpy(mi->value, x_("*"));
		}
	}

	/* display and select from the menu */
	butstate = TRUE;
	cpm = pm;
	miret = us_popupmenu(&cpm, &butstate, TRUE, -1, -1, 0);

	/* now analyze the results */
	for(i=0; i<pm->total; i++)
	{
		mi = &pm->list[i];
		if (mi->changed)
		{
			/* build the new command with the argument */
			uc = mi->response;
			infstr = initinfstr();
			addstringtoinfstr(infstr, uc->comname);
			us_appendargs(infstr, uc);
			addtoinfstr(infstr, ' ');
			addstringtoinfstr(infstr, mi->value);
			uc = us_makecommand(returninfstr(infstr));
			if (uc != NOUSERCOM)
			{
				if (miret != mi) verbose = TRUE; else
					verbose = FALSE;
				us_executesafe(uc, verbose, FALSE, FALSE);
				us_freeusercom(uc);
			}
		}
		if (mi->maxlen >= 0) efree(mi->value);
	}

	/* now execute the selected command if it wasn't already done */
	if (miret == NOPOPUPMENUITEM || miret == 0) return;
	if (miret->changed) return;
	uc = miret->response;
	if (uc == NOUSERCOM) return;
	us_execute(uc, FALSE, FALSE, FALSE);
}

/*
 * routine to execute macro "mac" with parameters in the "count" entries of
 * array "pp"
 * DJY: Modified 10/87 to save old macro information before executing new macro
 * This is necessary if recursive macro calls are used.
 */
void us_domacro(CHAR *mac, INTBIG count, CHAR *pp[])
{
	REGISTER INTBIG i, total;
	BOOLEAN verbose;
	REGISTER USERCOM *uc;
	CHAR *runsave, *build[MAXPARS], *paramsave;
	REGISTER CHAR *pt, **varmacroaddr;
	REGISTER INTBIG len, varmacrokey;
	REGISTER VARIABLE *varmacro, *varbuilding, *varrunning;
	REGISTER void *infstr;

	/* make sure the macro being executed is not currently being defined */
	varbuilding = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
	if (varbuilding != NOVARIABLE && namesame((CHAR *)varbuilding->addr, mac) == 0)
	{
		us_abortcommand(_("End macro definitions before executing them"));
		return;
	}

	/* get the macro to be run */
	varmacro = us_getmacro(mac);
	if (varmacro == NOVARIABLE || (len = getlength(varmacro)) < 4)
	{
		us_abortcommand(_("Cannot find macro '%s'"), mac);
		return;
	}
	varmacroaddr = (CHAR **)varmacro->addr;
	varmacrokey = varmacro->key;

	/* find out whether the macro should be verbose */
	total = us_parsecommand(varmacroaddr[0], build);
	verbose = FALSE;
	for(i=0; i<total; i++) if (namesame(build[i], x_("verbose")) == 0) verbose = TRUE;

	/* save former parameters to this macro, set new ones */
	pt = varmacroaddr[1];
	if (*pt != 0) (void)allocstring(&paramsave, pt, el_tempcluster); else
		paramsave = 0;
	infstr = initinfstr();
	for(i=0; i<count; i++)
	{
		if (i != 0) addtoinfstr(infstr, ' ');
		addtoinfstr(infstr, '"');
		for(pt = pp[i]; *pt != 0; pt++)
		{
			if (*pt == '"') addtoinfstr(infstr, '^');
			addtoinfstr(infstr, *pt);
		}
		addtoinfstr(infstr, '"');
	}
	nextchangequiet();
	(void)setindkey((INTBIG)us_tool, VTOOL, varmacrokey, 1, (INTBIG)returninfstr(infstr));

	/* save currently running macro, set this as current */
	varrunning = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrorunningkey);
	if (varrunning == NOVARIABLE) runsave = 0; else
		(void)allocstring(&runsave, (CHAR *)varrunning->addr, el_tempcluster);
	nextchangequiet();
	(void)setvalkey((INTBIG)us_tool, VTOOL, us_macrorunningkey, (INTBIG)mac, VSTRING|VDONTSAVE);

	/* execute the macro */
	us_state &= ~MACROTERMINATED;
	for(i=3; i<len; i++)
	{
		uc = us_makecommand(varmacroaddr[i]);
		if (uc == NOUSERCOM) continue;
		us_executesafe(uc, verbose, FALSE, FALSE);
		us_freeusercom(uc);
		if ((us_state&MACROTERMINATED) != 0) break;
	}
	us_state &= ~MACROTERMINATED;

	/* restore currently running macro */
	if (runsave == 0)
	{
		nextchangequiet();
		(void)delvalkey((INTBIG)us_tool, VTOOL, us_macrorunningkey);
	} else
	{
		nextchangequiet();
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_macrorunningkey, (INTBIG)runsave, VSTRING|VDONTSAVE);
		efree(runsave);
	}

	/* restore the current macro parameters */
	if (paramsave == 0)
	{
		nextchangequiet();
		(void)setindkey((INTBIG)us_tool, VTOOL, varmacrokey, 1, (INTBIG)x_(""));
	} else
	{
		nextchangequiet();
		(void)setindkey((INTBIG)us_tool, VTOOL, varmacrokey, 1, (INTBIG)paramsave);
		efree(paramsave);
	}
}

/*
 * routine to execute the commands in the file pointed to by stream "infile".
 * If "verbose" is false, execute the commands silently.  "headerstring"
 * is the string that must exist at the start of every line of the command
 * file that is to be processed.
 */
void us_docommands(FILE *infile, BOOLEAN verbose, CHAR *headerstring)
{
	REGISTER INTBIG headercount, lastquiet=0;
	REGISTER CHAR *pp;
	CHAR line[MAXLINE], *savemacro;
	REGISTER USERCOM *uc, *savelastc;
	REGISTER VARIABLE *varbuilding;

	/* see how many characters are in the header string */
	headercount = estrlen(headerstring);

	/* shut-up the status terminal if requested */
	if (!verbose) lastquiet = ttyquiet(1);

	/* save iteration and macro state */
	savelastc = us_lastcom;      us_lastcom = NOUSERCOM;
	varbuilding = getvalkey((INTBIG)us_tool, VTOOL, VSTRING, us_macrobuildingkey);
	if (varbuilding != NOVARIABLE)
	{
		(void)allocstring(&savemacro, (CHAR *)varbuilding->addr, el_tempcluster);
		nextchangequiet();
		(void)delvalkey((INTBIG)us_tool, VTOOL, us_macrobuildingkey);
	} else savemacro = 0;

	/* loop through the command file */
	for(;;)
	{
		if (stopping(STOPREASONCOMFILE)) break;
		if (xfgets(line, MAXLINE, infile)) break;
		pp = line;   while (*pp == ' ' || *pp == '\t') pp++;
		if (namesamen(pp, headerstring, headercount) != 0) continue;
		pp += headercount;

		/* skip initial blanks and tabs */
		while (*pp == ' ' || *pp == '\t') pp++;

		/* ignore comment lines */
		if (*pp == 0 || *pp == ';') continue;

		/* parse the line into separate strings */
		uc = us_makecommand(pp);
		if (uc == NOUSERCOM) break;
		us_executesafe(uc, verbose, TRUE, TRUE);
		us_freeusercom(uc);
	}

	/* restore the world as it was */
	if (us_lastcom != NOUSERCOM) us_freeusercom(us_lastcom);
	us_lastcom = savelastc;
	if (savemacro != 0)
	{
		nextchangequiet();
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_macrobuildingkey, (INTBIG)savemacro, VSTRING|VDONTSAVE);
		efree(savemacro);
	}
	if (!verbose) (void)ttyquiet(lastquiet);
}

/*
 * routine to parse the line in "pp" into space-separated parameters and
 * to store them in the array "build".  Returns the number of strings put
 * into "build".
 */
INTBIG us_parsecommand(CHAR *pp, CHAR *build[])
{
	REGISTER CHAR *ppsave, c, *tstart, *tend, *ch, *initstring;
	CHAR tempbuf[300];
	REGISTER INTBIG count, inquote, tp;

	/* parse the line into up to MAXPARS fields (command plus parameters) */
	initstring = pp;
	for(count=0; count<MAXPARS; count++)
	{
		/* skip the delimiters before the keyword */
		while (*pp == ' ' || *pp == '\t') pp++;
		if (*pp == 0) break;

		/* remember the start of the keyword */
		ppsave = pp;

		/* scan to the end of the keyword (considering quotation) */
		inquote = 0;
		tstart = tend = 0;
		for(;;)
		{
			if (*pp == 0) break;
			if (*pp == '^' && pp[1] != 0)
			{
				pp += 2;
				continue;
			}
			if (inquote == 0)
			{
				if (*pp == '"') inquote = 1; else
				{
					if (pp[0] == 'N' && pp[1] == '_' && pp[2] == '(' && pp[3] == '"')
					{
						inquote = 1;
						tstart = pp;
						pp += 3;
					}
				}
			} else
			{
				if (*pp == '"')
				{
					inquote = 0;
					if (tstart != 0 && pp[1] == ')')
					{
						tend = pp;
						pp++;
					}
				}
			}
			if (inquote == 0 && (*pp == ' ' || *pp == '\t')) break;
			pp++;
		}

		if (inquote != 0 || (tstart != 0 && tend == 0))
			ttyputerr(_("Unbalanced quote in commandfile line '%s'"), initstring);

		/* save the keyword */
		c = *pp;   *pp = 0;
		if (tstart != 0 && tend != 0)
		{
			tp = 0;
			tempbuf[tp++] = '"';
			for(ch = ppsave; ch != tstart; ch++) tempbuf[tp++] = *ch;
			tstart += 4;
			*tend = 0;
			for(ch = TRANSLATE(tstart); *ch != 0; ch++) tempbuf[tp++] = *ch;
			*tend = '"';
			for(ch = tend+2; *ch != 0; ch++) tempbuf[tp++] = *ch;
			tempbuf[tp++] = '"';
			tempbuf[tp] = 0;
			ppsave = tempbuf;
		}
		if (count > us_parsemaxownbuild)
		{
			(void)allocstring(&us_parseownbuild[count], ppsave, us_tool->cluster);
			us_parsemaxownbuild = count;
		} else (void)reallocstring(&us_parseownbuild[count], ppsave, us_tool->cluster);
		build[count] = us_parseownbuild[count];
		*pp = c;
	}
	return(count);
}

/*
 * routine to parse the string in "line" into a user command which is
 * allocated and returned (don't forget to free it when done).  Returns
 * NOUSERCOM upon error.
 */
USERCOM *us_buildcommand(INTBIG count, CHAR *par[])
{
	REGISTER USERCOM *uc;
	REGISTER INTBIG i, inquote;
	REGISTER CHAR *in, *out;
	extern COMCOMP us_userp;

	/* build the command structure */
	i = parse(par[0], &us_userp, TRUE);
	uc = us_allocusercom();
	if (uc == NOUSERCOM) return(NOUSERCOM);
	uc->active = i;
	uc->menu = us_getpopupmenu(par[0]);
	(void)allocstring(&uc->comname, par[0], us_tool->cluster);
	uc->count = count-1;
	for(i=1; i<count; i++)
	{
		if (allocstring(&uc->word[i-1], par[i], us_tool->cluster)) return(NOUSERCOM);

		/* remove quotes from the keyword */
		inquote = 0;
		for(in = out = uc->word[i-1]; *in != 0; in++)
		{
			if (in[0] == '"')
			{
				if (inquote == 0)
				{
					inquote = 1;
					continue;
				}
				if (in[1] == '"')
				{
					*out++ = '"';
					in++;
					continue;
				}
				inquote = 0;
				continue;
			}
			*out++ = *in;
		}
		*out = 0;
	}
	return(uc);
}

/*
 * routine to parse the command in the string "line" and return a user
 * command object with that command.  This essentially combines
 * "us_parsecommand" and "us_buildcommand" into one routine.  Returns NOUSERCOM
 * if the command cannot be parsed.  The command object must be freed when done.
 */
USERCOM *us_makecommand(CHAR *line)
{
	CHAR *build[MAXPARS];
	REGISTER INTBIG count;
	REGISTER USERCOM *uc;

	count = us_parsecommand(line, build);
	if (count <= 0) return(NOUSERCOM);
	uc = us_buildcommand(count, build);
	if (uc == NOUSERCOM) ttyputnomemory();
	return(uc);
}

/*
 * routine to obtain the variable with the macro called "name".  Returns
 * NOVARIABLE if not found.
 */
VARIABLE *us_getmacro(CHAR *name)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("USER_macro_"));
	addstringtoinfstr(infstr, name);
	return(getval((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, returninfstr(infstr)));
}

/*
 * routine to return the macro package named "name"
 */
MACROPACK *us_getmacropack(CHAR *name)
{
	REGISTER MACROPACK *pack;

	for(pack = us_macropacktop; pack != NOMACROPACK; pack = pack->nextmacropack)
		if (namesame(pack->packname, name) == 0) return(pack);
	return(NOMACROPACK);
}

/*
 * routine to return address of popup menu whose name is "name"
 */
POPUPMENU *us_getpopupmenu(CHAR *name)
{
	REGISTER POPUPMENU *pm;

	for(pm = us_firstpopupmenu; pm != NOPOPUPMENU; pm = pm->nextpopupmenu)
		if (namesame(name, pm->name) == 0) return(pm);
	return(NOPOPUPMENU);
}

/*
 * routine to append the parameters of user command "uc" to the infinite string.
 */
void us_appendargs(void *infstr, USERCOM *uc)
{
	REGISTER INTBIG k;

	for(k=0; k<uc->count; k++)
	{
		addtoinfstr(infstr, ' ');
		addstringtoinfstr(infstr, uc->word[k]);
	}
}

/*
 * routine to copy the string "str" to an infinite string and return it.
 * This is needed when a local string is being returned from a subroutine and
 * it will be destroyed on return.  It is also used to preserve strings from
 * dialogs which get destroyed when the dialog is closed.
 */
CHAR *us_putintoinfstr(CHAR *str)
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, str);
	return(returninfstr(infstr));
}

/***************************** GENERAL DIALOG CONTROL *****************************/

/*
 * Routine to declare a dialog routine in "routine" that will be called when
 * "ttygetparam()" is given the command-completion module "getlinecomcomp".
 * Also, if "terminputkeyword" is not zero, this is the keyword for the
 * "terminal input" command that invokes this dialog routine.
 */
void DiaDeclareHook(CHAR *terminputkeyword, COMCOMP *getlinecomp, void (*routine)(void))
{
	DIALOGHOOK *dh;

	dh = (DIALOGHOOK *)emalloc(sizeof (DIALOGHOOK), us_tool->cluster);
	if (dh == 0) return;
	if (terminputkeyword == 0) dh->terminputkeyword = 0; else
		(void)allocstring(&dh->terminputkeyword, terminputkeyword, us_tool->cluster);
	dh->getlinecomp = getlinecomp;
	dh->routine = routine;
	dh->nextdialoghook = us_firstdialoghook;
	us_firstdialoghook = dh;
}

/*
 * Routine to find the command-completion object associated with the "terminal input"
 * keyword in "keyword".
 */
COMCOMP *us_getcomcompfromkeyword(CHAR *keyword)
{
	DIALOGHOOK *dh;

	for(dh = us_firstdialoghook; dh != NODIALOGHOOK; dh = dh->nextdialoghook)
	{
		if (dh->terminputkeyword == 0) continue;
		if (namesame(keyword, dh->terminputkeyword) == 0) return(dh->getlinecomp);
	}
	return(NOCOMCOMP);
}

/* List Dialog */
static DIALOGITEM us_listdialogitems[] =
{
 /*  1 */ {0, {160,216,184,280}, BUTTON, N_("OK")},
 /*  2 */ {0, {96,216,120,280}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {56,8,200,206}, SCROLL, x_("")},
 /*  4 */ {0, {8,8,54,275}, MESSAGE, x_("")}
};
DIALOG us_listdialog = {{50,75,259,369}, x_(""), 0, 4, us_listdialogitems, 0, 0};

/* special items for the "ttygetline" dialog: */
#define DLST_LIST     3		/* line (scroll) */
#define DLST_PROMPT   4		/* prompt (stat text) */

/*
 * routine to get a string with "parameter" information (does command completion)
 */
INTBIG ttygetparam(CHAR *prompt, COMCOMP *parameter, INTBIG keycount, CHAR *paramstart[])
{
	INTBIG filetype, itemHit;
	BOOLEAN multiplefiles;
	REGISTER CHAR *pt, *next;
	DIALOGHOOK *dh;
	extern COMCOMP us_arrayxp, us_editcellp, us_spreaddp, us_artlookp,
		us_defnodexsp, us_lambdachp, us_replacep, us_visiblelayersp, us_portlp,
		us_copycellp, us_showp, us_gridalip, us_defarcsp, us_portep, us_menup,
		us_yesnop, us_viewn1p, us_noyesp, us_colorpvaluep, us_viewfp, us_nodetptlp,
		us_colorreadp, us_colorwritep, us_varvep, us_nodetp, us_showop, us_librarywp,
		us_windowrnamep, us_technologyup, us_technologyedlp, us_librarydp, us_helpp,
		us_technologyctdp, us_textdsp, us_librarywriteformatp, us_colorhighp,
		us_showdp, us_renameop, us_renamenp, us_findnamep, us_librarynp, us_libraryrp,
		us_nodetcaip, us_bindgkeyp, us_sizep, us_sizewp, us_defnodep, us_showup,
		us_librarykp, us_findobjap, us_window3dp, us_gridp, us_technologytp, us_textfp,
		us_renamecp, us_findnnamep, us_findexportp, us_noyesalwaysp, us_varop,
		us_copycelldp, us_findnodep, us_technologycnnp, us_showep, us_purelayerp,
		us_textsp, us_viewdp, us_interpretcp, us_findarcp, us_varvmp, us_showrp,
		us_varbdp, us_portcp, us_showfp, us_varbs1p, us_findintp, us_viewc1p,
		us_defnodenotp, us_nodetptmp;
	REGISTER void *infstr, *dia;
	Q_UNUSED( keycount );

	/* is this necessary?  if not, us_pathiskey does not need to be global */
	us_pathiskey = parameter->ifmatch;

	for(dh = us_firstdialoghook; dh != NODIALOGHOOK; dh = dh->nextdialoghook)
		if (parameter == dh->getlinecomp)
	{
		(*dh->routine)();
		return(0);
	}
	if (parameter == &us_window3dp)           return(us_3ddepthdlog());
	if (parameter == &us_showep)              return(us_aboutdlog());
	if (parameter == &us_gridalip)            return(us_alignmentdlog());
	if (parameter == &us_nodetcaip)           return(us_annularringdlog());
	if (parameter == &us_sizewp)              return(us_arcsizedlog(paramstart));
	if (parameter == &us_arrayxp)             return(us_arraydlog(paramstart));
	if (parameter == &us_artlookp)            return(us_artlookdlog());
	if (parameter == &us_varvmp)              return(us_attrenumdlog());
	if (parameter == &us_varop)               return(us_attributesdlog());
	if (parameter == &us_varbdp)              return(us_attrparamdlog());
	if (parameter == &us_varbs1p)             return(us_attrreportdlog());
	if (parameter == &us_copycellp)           return(us_copycelldlog(prompt));
	if (parameter == &us_defnodenotp)         return(us_copyrightdlog());
	if (parameter == &us_defarcsp)            return(us_defarcdlog());
	if (parameter == &us_defnodexsp)          return(us_defnodedlog());
	if (parameter == &us_textdsp)             return(us_deftextdlog(prompt));
	if (parameter == &us_technologyedlp)      return(us_dependentlibdlog());
	if (parameter == &us_editcellp)           return(us_editcelldlog(prompt));
	if (parameter == &us_librarynp)           return(us_examineoptionsdlog());
	if (parameter == &us_libraryrp)           return(us_findoptionsdlog());
	if (parameter == &us_defnodep)            return(us_celldlog());
	if (parameter == &us_showfp)              return(us_celllist());
	if (parameter == &us_showdp)              return(us_cellselect(prompt, paramstart, 0));
	if (parameter == &us_viewc1p)             return(us_cellselect(prompt, paramstart, 1));
	if (parameter == &us_showup)              return(us_cellselect(_("Cell to delete"), paramstart, 2));
	if (parameter == &us_textfp)              return(us_findtextdlog());
	if (parameter == &us_viewfp)              return(us_frameoptionsdlog());
	if (parameter == &us_showrp)              return(us_generaloptionsdlog());
	if (parameter == &us_gridp)               return(us_griddlog());
	if (parameter == &us_portcp)              return(us_globalsignaldlog());
	if (parameter == &us_helpp)               return(us_helpdlog(prompt));
	if (parameter == &us_colorhighp)          return(us_highlayerlog());
	if (parameter == &us_copycelldp)          return(us_iconstyledlog());
	if (parameter == &us_interpretcp)         return(us_javaoptionsdlog());
	if (parameter == &us_lambdachp)           return(us_lambdadlog());
	if (parameter == &us_librarydp)           return(us_librarypathdlog());
	if (parameter == &us_technologycnnp)      return(us_libtotechnologydlog());
	if (parameter == &us_textsp)              return(us_modtextsizedlog());
	if (parameter == &us_menup)               return(us_menudlog(paramstart));
	if (parameter == &us_viewn1p)             return(us_newviewdlog(paramstart));
	if (parameter == &us_sizep)               return(us_nodesizedlog(paramstart));
	if (parameter == &us_noyesp)              return(us_noyesdlog(prompt, paramstart));
	if (parameter == &us_noyesalwaysp)        return(us_noyesalwaysdlog(prompt, paramstart));
	if (parameter == &us_librarykp)           return(us_oldlibrarydlog());
	if (parameter == &us_librarywp)           return(us_optionsavingdlog());
	if (parameter == &us_colorpvaluep)        return(us_patterndlog());
	if (parameter == &us_nodetptlp)           return(us_placetextdlog());
	if (parameter == &us_librarywriteformatp) return(us_plotoptionsdlog());
	if (parameter == &us_portlp)              return(us_portdisplaydlog());
	if (parameter == &us_portep)              return(us_portdlog());
	if (parameter == &us_bindgkeyp)           return(us_quickkeydlog());
	if (parameter == &us_renameop)            return(us_renamedlog(VLIBRARY));
	if (parameter == &us_renamenp)            return(us_renamedlog(VTECHNOLOGY));
	if (parameter == &us_renamecp)            return(us_renamedlog(VPORTPROTO));
	if (parameter == &us_findnamep)           return(us_renamedlog(VNODEPROTO));
	if (parameter == &us_findintp)            return(us_renamedlog(VNETWORK));
	if (parameter == &us_replacep)            return(us_replacedlog());
	if (parameter == &us_nodetptmp)           return(us_romgendlog());
	if (parameter == &us_findobjap)           return(us_selectoptdlog());
	if (parameter == &us_findnnamep)          return(us_selectobjectdlog(0));
	if (parameter == &us_findexportp)         return(us_selectobjectdlog(1));
	if (parameter == &us_findnodep)           return(us_selectobjectdlog(2));
	if (parameter == &us_findarcp)            return(us_selectobjectdlog(3));
	if (parameter == &us_showp)               return(us_showdlog(FALSE));
	if (parameter == &us_showop)              return(us_showdlog(TRUE));
	if (parameter == &us_spreaddp)            return(us_spreaddlog());
	if (parameter == &us_technologyup)        return(us_technologydlog(prompt, paramstart));
	if (parameter == &us_technologytp)        return(us_techoptdlog());
	if (parameter == &us_technologyctdp)      return(us_techvarsdlog());
	if (parameter == &us_nodetp)              return(us_tracedlog());
	if (parameter == &us_purelayerp)          return(us_purelayernodedlog(paramstart));
	if (parameter == &us_varvep)              return(us_variablesdlog());
	if (parameter == &us_viewdp)              return(us_viewdlog(1));
	if (parameter == &us_visiblelayersp)      return(us_visiblelayersdlog(prompt));
	if (parameter == &us_windowrnamep)        return(us_windowviewdlog());
	if (parameter == &us_yesnop)              return(us_yesnodlog(prompt, paramstart));
	if (parameter == &us_colorwritep || parameter == &us_colorreadp)
	{
		/* copy "prompt" to a temporary so that it can be modified */
		infstr = initinfstr();
		addstringtoinfstr(infstr, prompt);
		prompt = returninfstr(infstr);

		/* file dialog prompts have the form "type/prompt" */
		filetype = el_filetypetext;
		multiplefiles = FALSE;
		for(pt = prompt; *pt != 0; pt++) if (*pt == '/') break;
		if (*pt == '/')
		{
			*pt = 0;
			if (namesamen(prompt, x_("multi-"), 6) == 0)
			{
				multiplefiles = TRUE;
				prompt += 6;
			}
			filetype = getfiletype(prompt);
			if (filetype == 0) filetype = el_filetypetext;
			*pt++ = '/';
			prompt = pt;
		}
		if (parameter == &us_colorwritep)
		{
			/* file output dialog prompts have the form "type/prompt|default" */
			filetype |= FILETYPEWRITE;
			pt = prompt;
			while (*pt != 0 && *pt != '|') pt++;
			if (*pt == 0)
			{
				next = (CHAR *)fileselect(prompt, filetype, x_(""));
			} else
			{
				*pt = 0;
				next = (CHAR *)fileselect(prompt, filetype, &pt[1]);
				*pt = '|';
			}
			if (next == 0 || *next == 0) return(0);
			setdefaultcursortype(WAITCURSOR);
			paramstart[0] = next;
			return(1);
		} else
		{
			/* do dialog to get file name */
			if (multiplefiles)
			{
				/* allow selection of multiple files */
				next = multifileselectin(prompt, filetype);
			} else
			{
				/* allow selection of one file */
				next = (CHAR *)fileselect(prompt, filetype, x_(""));
			}
			if (next == 0 || *next == 0) return(0);
			setdefaultcursortype(WAITCURSOR);
			paramstart[0] = next;
			return(1);
		}
	}

	/* the general case: display the dialog box */
	dia = DiaInitDialog(&us_listdialog);
	if (dia == 0) return(0);
	DiaInitTextDialog(dia, DLST_LIST, parameter->toplist, parameter->nextcomcomp, DiaNullDlogDone,
		0, SCSELMOUSE|SCSELKEY|SCDOUBLEQUIT|SCHORIZBAR);
	DiaSetText(dia, DLST_PROMPT, prompt);

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}
	paramstart[0] = us_putintoinfstr(DiaGetScrollLine(dia, DLST_LIST, DiaGetCurLine(dia, DLST_LIST)));
	DiaDoneDialog(dia);
	if (itemHit == CANCEL) return(0);
	return(1);
}

/***************************** GET FULL COMMAND *****************************/

/* Full Input Dialog */
static DIALOGITEM us_ttyfulldialogitems[] =
{
 /*  1 */ {0, {160,328,184,392}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,328,128,392}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {24,8,56,408}, EDITTEXT, x_("")},
 /*  4 */ {0, {3,8,19,408}, MESSAGE, x_("")},
 /*  5 */ {0, {88,8,212,294}, SCROLL, x_("")},
 /*  6 */ {0, {72,56,88,270}, MESSAGE, N_("Type '?' for a list of options")}
};
static DIALOG us_ttyfulldialog = {{50,75,271,493}, N_("Command Completion"), 0, 6, us_ttyfulldialogitems, 0, 0};

/* special items for the "ttygetline" dialog: */
#define DGTF_LINE     3		/* line (edit text) */
#define DGTF_PROMPT   4		/* prompt (stat text) */
#define DGTF_OPTIONS  5		/* options (scroll) */

/*
 * Routine to get a string with "parameter" information (does command completion).
 * Returns the number of strings parsed into "paramstart" (-1 if cancelled or
 * on error).
 */
INTBIG ttygetfullparam(CHAR *prompt, COMCOMP *parameter, INTBIG keycount,
	CHAR *paramstart[])
{
	INTBIG i, itemHit;
	CHAR *line;
	REGISTER void *infstr, *dia;

	/* initialize for command completion */
	us_expandttybuffers(80);
	us_pacurchar = 0;
	us_pattyline[0] = 0;
	us_paparamstart[us_paparamcount = 0] = us_pattyline;
	us_paparamtype[us_paparamcount] = parameter;
	us_paparamtype[us_paparamcount+1] = &us_panullcomcomp;
	us_paparamtotal = 2;
	us_painquote = FALSE;

	/* display the dialog box */
	dia = DiaInitDialog(&us_ttyfulldialog);
	if (dia == 0) return(-1);
	DiaInitTextDialog(dia, DGTF_OPTIONS, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1, 0);

	/* load header message */
	infstr = initinfstr();
	if (*parameter->noise != 0)
	{
		addstringtoinfstr(infstr, parameter->noise);
		if (parameter->def != 0)
			formatinfstr(infstr, x_(" (%s)"), parameter->def);
	} else addstringtoinfstr(infstr, prompt);
	DiaSetText(dia, DGTF_PROMPT, returninfstr(infstr));

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;

		if (itemHit == DGTF_LINE)
		{
			line = DiaGetText(dia, DGTF_LINE);
			for(i=0; line[i] == us_pattyline[i] && line[i] != 0; i++);
			if (us_pattyline[i] != 0) us_pabackup(i);
			for(; line[i] != 0; i++) us_pahandlechar(dia, line[i]);
			if (estrcmp(us_pattyline, line) != 0) DiaSetText(dia, DGTF_LINE, us_pattyline);
		}
	}

	/* parse the line if not cancelled */
	if (itemHit == CANCEL) us_paparamcount = -1; else
	{
		us_paparamcount++;
		for(i=1; i<us_paparamcount; i++) (us_paparamstart[i])[-1] = 0;
		if (*us_paparamstart[us_paparamcount-1] == 0) us_paparamcount--;
		if(us_paparamcount>keycount) us_paparamcount = keycount;
		for(i=0; i<us_paparamcount; i++) paramstart[i] = us_paparamstart[i];
	}
	DiaDoneDialog(dia);
	return(us_paparamcount);
}

/*
 * Routine to handle next char
 */
static BOOLEAN us_pahandlechar(void *dia, INTBIG c)
{
	REGISTER INTBIG i, j, k, nofill, noshoall, isbreak, nmatch;
	REGISTER BOOLEAN topval;
	REGISTER INTBIG (*setparams)(CHAR*, COMCOMP*[], CHAR);
	REGISTER BOOLEAN (*toplist)(CHAR**);
	REGISTER CHAR *initialline, *pt, realc, *breakch, *(*nextinlist)(void);
	CHAR *compare;
	REGISTER COMCOMP *thiscomcomp;
	COMCOMP *nextparams[MAXKEYWORD];
	REGISTER void *infstr;

	/* continue parsing word number "us_paparamcount" */
	thiscomcomp = us_paparamtype[us_paparamcount];
	initialline = us_paparamstart[us_paparamcount];

	if ((thiscomcomp->interpret&NOSHOALL) != 0) noshoall = 1; else
		noshoall = 0;
	if ((thiscomcomp->interpret&NOFILL) != 0) nofill = 1; else
		nofill = 0;
	us_pathiskey = thiscomcomp->ifmatch;
	if (*initialline == '$')
	{
		/* special case for variables */
		toplist = us_topofvars;
		nextinlist = us_nextvars;
	} else
	{
		toplist = thiscomcomp->toplist;
		nextinlist = thiscomcomp->nextcomcomp;
	}
	setparams = thiscomcomp->params;
	breakch = thiscomcomp->breakchrs;

	/* provide help if requested */
	if (c == '?' && !us_painquote)
	{
		/* initialize scrolled list of choices */
		DiaLoadTextDialog(dia, 5, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);

		compare = initialline;
		topval = (*toplist)(&compare);
		if (*thiscomcomp->noise != 0)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, thiscomcomp->noise);
			if (thiscomcomp->def != 0)
			{
				addstringtoinfstr(infstr, x_(" ("));
				addstringtoinfstr(infstr, thiscomcomp->def);
				addtoinfstr(infstr, ')');
			}
			if (topval) addtoinfstr(infstr, ':');
			DiaStuffLine(dia, 5, returninfstr(infstr));
		}

		if (*compare != 0 || noshoall == 0)
		{
			for(i=0; (pt = (*nextinlist)()); )
			{
				j = stringmatch(pt, compare);
				if (j == -1) continue;
				if (j == -2) j = estrlen(pt);
				if (j > i) i = j;
			}
			if (i == 0 && *compare != 0) ttybeep(SOUNDBEEP, TRUE); else
			{
				compare = initialline;
				for((void)(*toplist)(&compare); (pt = (*nextinlist)()); )
				{
					j = stringmatch(pt, compare);
					if (j == -1) continue;
					if (j == i || j == -2) DiaStuffLine(dia, 5, pt);
				}
			}
		}
		DiaSelectLine(dia, 5, -1);
		return(FALSE);
	}

	/* check for quoted strings */
	if (c == '"') us_painquote = !us_painquote;

	/* see if this is a break character */
	isbreak = 0;
	realc = 0;
	if (!us_painquote)
		for(pt = breakch; *pt != 0; pt++)
			if (c == *pt) { isbreak++;  break; }
	if (isbreak != 0) { realc = (CHAR)c;   c = ' '; } 

	/* ignore break characters at the start of a keyword */
	if ((c == ESCKEY || isbreak != 0) && *initialline == 0) return(FALSE);

	/* fill out keyword if a break character is reached */
	if (c == ESCKEY || isbreak != 0 || c == '\n' || c == '\r')
	{
		compare = initialline;
		if (*compare != 0)
		{
			i = nmatch = 0;
			us_paextra[0] = 0;
			for((void)(*toplist)(&compare); (pt = (*nextinlist)());)
			{
				j = stringmatch(pt, compare);
				if (j == -1) continue;
				if (j == -2)
				{
					us_paextra[0] = 0;
					i = estrlen(pt);
					nmatch = 1;
					break;
				}
				if (j > i)
				{
					if ((INTBIG)estrlen(&pt[j]) >= us_patruelength)
						us_expandttybuffers(estrlen(&pt[j])+10);
					estrcpy(us_paextra, &pt[j]);
					i = j;
					nmatch = 1;
				} else if (j == i)
				{
					for(j=0; us_paextra[j]; j++) if (pt[i+j] != us_paextra[j]) us_paextra[j] = 0;
					nmatch++;
				}
			}

			/* fill out the keyword if possible */
			if ((nofill == 0 || c == ESCKEY) && us_paextra[0] != 0)
			{
				if (us_pacurchar+(INTBIG)estrlen(us_paextra) >= us_patruelength)
					us_expandttybuffers(us_pacurchar+estrlen(us_paextra)+10);
				(void)estrcat(&us_pattyline[us_pacurchar], us_paextra);
				us_pacurchar += estrlen(us_paextra);
			}

			/* print an error bell if there is ambiguity */
			if (nmatch != 1 && nofill == 0 && c != ESCKEY) ttybeep(SOUNDBEEP, TRUE);
		};

		/* if an escape was typed, stop after filling in the keyword */
		if (c == ESCKEY) return(FALSE);

		/* if normal break character was typed, advance to next keyword */
		if (isbreak != 0)
		{
			/* find out number of parameters to this keyword */
			k = (*setparams)(initialline, nextparams, realc);

			/* clip to the number of allowable parameters */
			if (k+us_paparamtotal > MAXKEYWORD)
			{
				ttyputmsg(_("Full number of parameters is more than %d"), MAXKEYWORD);
				k = MAXKEYWORD - us_paparamcount;
			}

			/* spread open the list */
			for(j=us_paparamtotal-1; j > us_paparamcount; j--)
				us_paparamtype[j+k] = us_paparamtype[j];
			us_paparamtotal += k;

			/* fill in the parameter types */
			for(j = 0; j < k; j++)
				us_paparamtype[us_paparamcount+j+1] = nextparams[j];
			us_paparamadded[us_paparamcount] = k;
			us_paparamcount++;

			pt = &us_pattyline[us_pacurchar];
			if (us_pacurchar >= us_patruelength)
				us_expandttybuffers(us_pacurchar+10);
			*pt = realc;   pt[1] = 0;
			us_pacurchar++;
			us_paparamstart[us_paparamcount] = &us_pattyline[us_pacurchar];
			return(FALSE);
		}

		/* if this is the end of the line, signal so */
		if (c == '\n' || c == '\r') return(TRUE);
	}

	/* normal character: add it to the input string */
	if (us_pacurchar >= us_patruelength) us_expandttybuffers(us_pacurchar+10);
	pt = &us_pattyline[us_pacurchar];
	*pt = (CHAR)c;   pt[1] = 0;
	us_pacurchar++;

	return(FALSE);
}

/*
 * Routine to return to previous state when number of matching chars diminishes to "to_curchar"
 */
static void us_pabackup(INTBIG to_curchar)
{
	REGISTER INTBIG j, k;

	while (us_pacurchar > to_curchar) {
		/* back up one character */
		if (us_pattyline[--us_pacurchar] == '"') us_painquote = !us_painquote;
		us_pattyline[us_pacurchar] = 0;

		/* special case if just backed up over a word boundary */
		if (&us_pattyline[us_pacurchar] < us_paparamstart[us_paparamcount])
		{
			/* remove any added parameters from the last keyword */
			k = us_paparamadded[us_paparamcount-1];
			us_paparamtotal -= k;
			for(j=us_paparamcount; j < us_paparamtotal; j++) us_paparamtype[j] = us_paparamtype[j+k];

			us_paparamcount--;
		}
	}
}

/*
 * Routine to expand the buffers "us_pattyline" and "us_paextra" to "amt" long
 */
static void us_expandttybuffers(INTBIG amt)
{
	REGISTER CHAR *newline, *newextra;
	REGISTER INTBIG i;

	/* stop now if request is already satisfied */
	if (amt <= us_patruelength) return;

	/* allocate new buffers */
	newline = (CHAR *)emalloc((amt+1) * SIZEOFCHAR, us_tool->cluster);
	if (newline == 0) return;
	newextra = (CHAR *)emalloc((amt+1) * SIZEOFCHAR, us_tool->cluster);
	if (newextra == 0) return;

	/* preserve any old buffers */
	if (us_pattyline != 0)
	{
		for(i=0; i<us_patruelength; i++)
		{
			newline[i] = us_pattyline[i];
			newextra[i] = us_paextra[i];
		}
		if (us_paparamstart != 0)
			for(i=0; i<=us_paparamcount; i++)
				us_paparamstart[i] = us_paparamstart[i] - us_pattyline + newline;
		efree(us_pattyline);
		efree(us_paextra);
	}

	/* set the new buffers */
	us_pattyline = newline;
	us_paextra = newextra;
	us_patruelength = amt;
}

/*
 * internal routines to search keyword lists for command completion:
 */
BOOLEAN us_patoplist(CHAR **a)
{
	Q_UNUSED( a );

	us_pathisind = 0;
	if (us_pathiskey == NOKEYWORD) return(FALSE);
	if (us_pathiskey[0].name == 0) return(FALSE);
	return(TRUE);
}

CHAR *us_panextinlist(void)
{
	if (us_pathiskey == NOKEYWORD) return(0);
	return(us_pathiskey[us_pathisind++].name);
}

INTBIG us_paparams(CHAR *word, COMCOMP *arr[], CHAR breakc)
{
	REGISTER INTBIG i, count, ind;
	Q_UNUSED( breakc );

	if (*word == 0 || us_pathiskey == NOKEYWORD) return(0);
	for(ind=0; us_pathiskey[ind].name != 0; ind++)
		if (namesame(word, us_pathiskey[ind].name) == 0) break;
	if (us_pathiskey[ind].name == 0) return(0);
	count = us_pathiskey[ind].params;
	for(i=0; i<count; i++) arr[i] = us_pathiskey[ind].par[i];
	return(count);
}
