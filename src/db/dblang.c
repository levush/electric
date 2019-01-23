/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dblang.c
 * Interpretive language interface module
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
#include "dblang.h"

#define EVALUATIONCACHESIZE 20
typedef struct
{
	CHAR     *code;					/* the interpreted code that is being cached */
	INTBIG    oaddr, otype;			/* address/type of object this code sits on */
	UINTBIG   type;					/* type of the result of the code */
	UINTBIG   reqtype;				/* requested type of the result of the code */
	INTBIG    usagestamp;			/* timestamp for when this cache entry was last used */
	UINTBIG   changetimestamp;		/* indication of changes to the database that may affect evaluation */
	UINTBIG   traversaltimestamp;	/* indication of hierarchy traversal that may affect evaluation */
	INTBIG    value;				/* evaluated value of the code */
	VARIABLE  var;					/* variable with the evaluation of the code */
} EVALUATIONCACHE;

static EVALUATIONCACHE db_evaluationcache[EVALUATIONCACHESIZE];
static INTBIG db_evaluationusagetime = 0;
static void   *db_languagecachemutex = 0;		/* mutex for caching language evaluation */

/* prototypes for local routines */
#if LANGLISP || LANGTCL || LANGJAVA || LANGMM
static VARIABLE *db_enterqueryincache(CHAR *code, INTBIG type, INTBIG reqtype, INTBIG value, INTBIG oaddr, INTBIG otype);
#endif
static BOOLEAN db_codehasvariables(CHAR *code, INTBIG language);

/****************************** LANGUAGE INTERFACE ******************************/

void db_initlanguages(void)
{
	REGISTER INTBIG i;

	for(i=0; i<EVALUATIONCACHESIZE; i++)
	{
		db_evaluationcache[i].code = 0;
		db_evaluationcache[i].usagestamp = 0;
	}
	(void)ensurevalidmutex(&db_languagecachemutex, TRUE);
}

/*
 * routine to return a string describing the currently available languages
 */
CHAR *languagename(void)
{
	CHAR *pt;
	REGISTER void *infstr;

	infstr = initinfstr();
#if	LANGJAVA
	addstringtoinfstr(infstr, x_(", Java"));
#endif
#if LANGLISP
	addstringtoinfstr(infstr, x_(", ELK Lisp"));
#endif
#if LANGTCL
	addstringtoinfstr(infstr, x_(", TCL "));
	addstringtoinfstr(infstr, TCL_PATCH_LEVEL);
#endif
	pt = returninfstr(infstr);
	if (*pt == 0) return(NOSTRING);
	return(&pt[2]);
}

/*
 * routine to load the code in the file "program" into the language interpreter
 * specified by "language".  Returns true on error.
 */
BOOLEAN loadcode(CHAR *program, INTBIG language)
{
#if LANGTCL
	REGISTER INTBIG code;
#endif

	switch (language)
	{
#if LANGLISP
		case VLISP:
			lsp_init();
			(void)General_Load(Make_String(program, estrlen(program)), The_Environment);
			return(FALSE);
#endif

#if LANGTCL
		case VTCL:
			code = Tcl_EvalFile(tcl_interp, program);
			if (code != TCL_OK)
				ttyputerr(x_("%s"), tcl_interp->result);
			break;
#endif

#if LANGJAVA
		case VJAVA:
			ttyputerr(_("Unable to load code into Java"));
			break;
#endif
	}
	return(TRUE);
}

/*
 * Routine to return TRUE if "code" (in language "language") has variables in it (and so may evaluate
 * differently with different contexts).
 */
BOOLEAN db_codehasvariables(CHAR *code, INTBIG language)
{
#if LANGJAVA
	REGISTER INTBIG i;
#endif

	switch (language)
	{
#if LANGLISP
		case VLISP:
			break;
#endif

#if LANGTCL
		case VTCL:
			break;
#endif

#if LANGJAVA
		case VJAVA:
			/* if there is an "@" or a "(" with letter or digit in front, this has variables */
			for(i=0; code[i] != 0; i++)
			{
				if (code[i] == '@') return(TRUE);
				if (code[i] == '(')
				{
					while (i > 0 && code[i-1] == ' ') i--;
					if (i <= 0) continue;
					if (isalnum(code[i-1])) return(TRUE);
				}
			}
			return(FALSE);
#endif

#if LANGMM
		case VMATHEMATICA:
			break;
#endif
	}
	return(TRUE);
}

/*
 * routine to evaluate string "code" in the specified "language", trying to match
 * "type" as a return type.  Returns a variable that describes the result (NOVARIABLE on error).
 */
VARIABLE *doquerry(CHAR *code, INTBIG language, UINTBIG type)
{
#if LANGTCL
	INTBIG result;
#endif
#if LANGLISP
	ELKObject obj;
#endif
#if LANGJAVA
	CHAR *str;
	INTBIG methodreturntype, reqtype;
#endif
#if	LANGMM
	CHAR *str;
	static INTBIG retarray[1];
	long ival;
	double fval;
	REGISTER void *infstr;
#endif
#if LANGTCL | LANGLISP | LANGJAVA | LANGMM
	INTBIG addr;
#endif
	REGISTER INTBIG index;
	INTBIG oaddr, otype;
	WINDOWPART *owin;
	REGISTER VARIABLE *retvar;
	REGISTER BOOLEAN hasvariables;
	REGISTER EVALUATIONCACHE *ec;

	/* see if this code has variables in it (if not, caching is simpler) */
	hasvariables = db_codehasvariables(code, language);
	db_evaluationusagetime++;

	/* search cache of evaluations */
	if (db_multiprocessing) emutexlock(db_languagecachemutex);
	if (!getcurrentvariableenvironment(&oaddr, &otype, &owin)) oaddr = otype = 0;
	if (!hasvariables)
	{
		for(index=0; index<EVALUATIONCACHESIZE; index++)
		{
			ec = &db_evaluationcache[index];
			if (ec->code == 0) continue;
			if (estrcmp(ec->code, code) != 0) continue;
			if (ec->reqtype != type) continue;
			if (hasvariables)
			{
				if (ec->changetimestamp != db_changetimestamp) continue;
				if (ec->traversaltimestamp != db_traversaltimestamp) continue;
				if (ec->oaddr != oaddr) continue;
				if (ec->otype != otype) continue;
			}

			/* found in cache: return value */
			retvar = &ec->var;
			ec->usagestamp = db_evaluationusagetime;
			if (db_multiprocessing) emutexunlock(db_languagecachemutex);
			return(retvar);
		}
	}

	/* do the evaluation */
	if (db_multiprocessing) emutexunlock(db_languagecachemutex);
	switch (language)
	{
#if LANGLISP
		case VLISP:
			/* make sure Lisp is initialized */
			lsp_init();

			/* convert the string to a Lisp form */
			obj = lsp_makeobject(code);
			if (EQ(obj, Eof)) break;

			/* evaluate the string */
			obj = Eval(obj);

			/* convert the evaluation to a string */
			if (lsp_describeobject(obj, type, &addr)) return(NOVARIABLE);
			retvar = db_enterqueryincache(code, type, type, addr, oaddr, otype);
			return(retvar);
#endif

#if LANGTCL
		case VTCL:
			/* evaluate the string */
			result = Tcl_Eval(tcl_interp, code);
			if (result != TCL_OK)
			{
				ttyputerr(x_("TCL: %s"), tcl_interp->result);
				return(NOVARIABLE);
			}

			/* convert the result to the desired type */
			addr = tcl_converttoelectric(tcl_interp->result, type);
			retvar = db_enterqueryincache(code, type, type, addr, oaddr, otype);
			return(retvar);
#endif

#if LANGJAVA
		case VJAVA:
			/* evaluate the string */
			java_init();
			str = java_query(code, &methodreturntype);
			if (str == 0) return(NOVARIABLE);
			getsimpletype(str, &methodreturntype, &addr, 0);
			reqtype = type;
			type = (type & ~VTYPE) | (methodreturntype & VTYPE);
			retvar = db_enterqueryincache(code, type, reqtype, addr, oaddr, otype);
			return(retvar);
#endif

#if LANGMM
		case VMATHEMATICA:
			/* make sure Mathematica is initialized */
			if (db_mathematicainit() != 0) break;

			/* send the string to Mathematica */
			MLPutFunction(db_mathematicalink, x_("EvaluatePacket"), 1);
				MLPutFunction(db_mathematicalink, x_("ToExpression"),1);
					MLPutString(db_mathematicalink, code);
			MLEndPacket(db_mathematicalink);
			if (MLError(db_mathematicalink) != MLEOK)
			{
				ttyputerr(x_("Mathematica error: %s"), MLErrorMessage(db_mathematicalink));
				break;
			}

			/* get the return expression */
			db_mathematicaprocesspackets(0);
			switch ((*type)&VTYPE)
			{
				case VSTRING:
					infstr = initinfstr();
					(void)db_mathematicagetstring(0);
					addr = (INTBIG)returninfstr(infstr);
					break;

				case VINTEGER:
				case VSHORT:
				case VBOOLEAN:
				case VADDRESS:
					switch (MLGetType(db_mathematicalink))
					{
						case MLTKSTR:
							MLGetString(db_mathematicalink, &str);
							addr = myatoi(str);
							break;
						case MLTKINT:
							MLGetLongInteger(db_mathematicalink, &ival);
							addr = ival;
							break;
						case MLTKREAL:
							MLGetReal(db_mathematicalink, &fval);
							addr = fval;
							break;
						default:
							MLNewPacket(db_mathematicalink);
							return(NOVARIABLE);
					}
					break;

				case VFLOAT:
				case VDOUBLE:
					switch (MLGetType(db_mathematicalink))
					{
						case MLTKSTR:
							MLGetString(db_mathematicalink, &str);
							addr = castint((float)myatoi(str));
							break;
						case MLTKINT:
							MLGetLongInteger(db_mathematicalink, &ival);
							addr = castint((float)ival);
							break;
						case MLTKREAL:
							MLGetReal(db_mathematicalink, &fval);
							addr = castint((float)fval);
							break;
						default:
							MLNewPacket(db_mathematicalink);
							return(NOVARIABLE);
					}
					break;

				case VFRACT:
					switch (MLGetType(db_mathematicalink))
					{
						case MLTKSTR:
							MLGetString(db_mathematicalink, &str);
							addr = myatoi(str) * WHOLE;
							break;
						case MLTKINT:
							MLGetLongInteger(db_mathematicalink, &ival);
							addr = ival * WHOLE;
							break;
						case MLTKREAL:
							MLGetReal(db_mathematicalink, &fval);
							addr = fval * WHOLE;
							break;
						default:
							MLNewPacket(db_mathematicalink);
							return(NOVARIABLE);
					}
					break;

				default:
					MLNewPacket(db_mathematicalink);
					return(NOVARIABLE);
			}

			if (((*type)&VISARRAY) != 0)
			{
				retarray[0] = addr;
				addr = (INTBIG)retarray;
			}
			retvar = db_enterqueryincache(code, *type, *type, addr, oaddr, otype);
			return(retvar);
#endif
	}
	return(NOVARIABLE);
}

/*
 * language interpreter.  Called the first time with "language" set to the desired
 * interpreter.  On repeated calls, "language" is zero.  Returns true when
 * termination is requested
 */
BOOLEAN languageconverse(INTBIG language)
{
	static INTBIG curlang;
#if LANGTCL
	static BOOLEAN gotPartial;
	CHAR *cmd, *tstr, *promptCmd;
	REGISTER INTBIG code;
	static Tcl_DString command;
#endif
#if LANGLISP
	ELKObject pred;
#endif
#if LANGMM
	CHAR *mstr;
#endif
#if LANGJAVA
	CHAR *jstr;
#endif

	/* on the first call, initialize the interpreter */
	if (language != 0)
	{
		curlang = language;
		switch (curlang)
		{
			case VLISP:
#if LANGLISP
				ttyputmsg(_("ELK Lisp 3.0, type %s to quit"), getmessageseofkey());
				lsp_init();
#else
				ttyputerr(_("LISP Interpreter is not installed"));
				return(TRUE);
#endif
				break;

			case VTCL:
#if LANGTCL
				ttyputmsg(_("TCL Interpreter, type %s to quit"), getmessageseofkey());
				Tcl_DStringInit(&command);
#else
				ttyputerr(_("TCL Interpreter is not installed"));
				return(TRUE);
#endif
				break;

			case VJAVA:
#if LANGJAVA
				jstr = java_init();
				if (jstr == 0) return(TRUE);
				ttyputmsg(_("%s Interpreter, type %s to quit"), jstr, getmessageseofkey());
#else
				ttyputerr(_("Java Interpreter is not installed"));
				return(TRUE);
#endif
				break;

#if LANGMM
			case VMATHEMATICA:
				if (db_mathematicainit() != 0) return(TRUE);
				ttyputmsg(_("Mathematica reader, type %s to quit"), getmessageseofkey());
				break;
#endif
		}
	}

	switch (curlang)
	{
#if LANGLISP
		case VLISP:
			pred = Eval(Intern(x_("the-top-level")));
			(void)Funcall(pred, Null, 0);
			break;
#endif

#if LANGTCL
		case VTCL:
			promptCmd = Tcl_GetVar(tcl_interp, (CHAR *)(gotPartial ? x_("tcl_prompt2") : x_("tcl_prompt1")),
				TCL_GLOBAL_ONLY);
			if (promptCmd == NULL && !gotPartial) promptCmd = x_("puts -nonewline \"% \""); 
			if (promptCmd != NULL)
			{
				code = Tcl_Eval(tcl_interp, promptCmd);
				if (code != TCL_OK)
				{
					ttyputerr(x_("%s (script that generates prompt)"), tcl_interp->result);
					promptCmd = NULL;
				}
			}

			/* get input using queued output as prompt */
			*tcl_outputloc = 0;
			tstr = ttygetlinemessages(tcl_outputbuffer);
			tcl_outputloc = tcl_outputbuffer;
			if (tstr == 0) return(TRUE);

			/* evaluate and print result */
			cmd = Tcl_DStringAppend(&command, tstr, -1);
			if ((tstr[0] != 0) && !Tcl_CommandComplete(cmd))
			{
				gotPartial = TRUE;
				return(FALSE);
			}
			gotPartial = FALSE;
			code = Tcl_RecordAndEval(tcl_interp, cmd, 0);
			Tcl_DStringFree(&command);
			if (code != TCL_OK)
			{
				ttyputerr(x_("%s"), tcl_interp->result);
			} else if (*tcl_interp->result != 0)
			{
				ttyputmsg(x_("%s"), tcl_interp->result);
			}
			return(FALSE);
#endif

#if LANGJAVA
		case VJAVA:
			jstr = ttygetlinemessages(x_("> "));
			if (jstr == 0) return(TRUE);

			/* send the string to Java */
			if (java_evaluate(jstr)) return(TRUE);
			return(FALSE);
#endif

#if LANGMM
		case VMATHEMATICA:
			mstr = ttygetlinemessages(x_("> "));
			if (mstr == 0) return(TRUE);

			/* send the string to Mathematica */
			MLPutFunction(db_mathematicalink, x_("ToExpression"), 1);
				MLPutString(db_mathematicalink, mstr);
			MLEndPacket(db_mathematicalink);

			if (MLError(db_mathematicalink) != MLEOK)
			{
				ttyputerr(_("Mathematica error: %s"), MLErrorMessage(db_mathematicalink));
				return(TRUE);
			}

			/* handle the result */
			db_mathematicaprocesspackets(1);
			return(FALSE);
#endif
	}
	return(TRUE);
}

#if LANGLISP || LANGTCL || LANGJAVA || LANGMM
/*
 * Routine to enter code "code" which returns "addr" of type "type" into the
 * cache of evaluations.  The code sits on object "oaddr" of type "otype".
 */
VARIABLE *db_enterqueryincache(CHAR *code, INTBIG type, INTBIG reqtype, INTBIG addr, INTBIG oaddr, INTBIG otype)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG oldeststamp, i;
	REGISTER EVALUATIONCACHE *ec, *oldestec;

	/* lock the shared resource (cache) */
	if (db_multiprocessing) emutexlock(db_languagecachemutex);

	/* if the cache is full, clear the oldest */
	oldestec = &db_evaluationcache[0];
	oldeststamp = oldestec->usagestamp;
	for(i=0; i<EVALUATIONCACHESIZE; i++)
	{
		ec = &db_evaluationcache[i];
		if (ec->usagestamp >= oldeststamp) continue;
		oldestec = ec;
		oldeststamp = ec->usagestamp;
	}
	if (oldestec->code != 0)
	{
		efree((CHAR *)oldestec->code);
		if ((oldestec->type&VTYPE) == VSTRING)
			efree((CHAR *)oldestec->value);
	}

	/* load the cache entry */
	oldestec->changetimestamp = db_changetimestamp;
	oldestec->traversaltimestamp = db_traversaltimestamp;
	oldestec->oaddr = oaddr;
	oldestec->otype = otype;
	(void)allocstring(&oldestec->code, code, db_cluster);
	oldestec->type = type;
	oldestec->reqtype = reqtype;
	oldestec->usagestamp = db_evaluationusagetime;

	if ((type&VTYPE) == VSTRING)
	{
		(void)allocstring((CHAR **)&oldestec->value, (CHAR *)addr, db_cluster);
	} else
	{
		oldestec->value = addr;
	}

	/* get the variable that describes it */
	var = &oldestec->var;
	var->addr = oldestec->value;
	var->type = type;

	/* unlock the shared resource (cache) */
	if (db_multiprocessing) emutexunlock(db_languagecachemutex);

	return(var);
}
#endif

/*
 * routine called to shutdown interpreter when the program exits
 */
void db_termlanguage(void)
{
	REGISTER INTBIG i;

	for(i=0; i<EVALUATIONCACHESIZE; i++)
	{
		if (db_evaluationcache[i].code == 0) continue;
		efree((CHAR *)db_evaluationcache[i].code);
		if ((db_evaluationcache[i].type&VTYPE) == VSTRING)
			efree((CHAR *)db_evaluationcache[i].value);
	}
#if LANGMM
	if (db_mathematicalink != NULL)
	{
		MLPutFunction(db_mathematicalink, x_("Exit"), 0);
		MLEndPacket(db_mathematicalink);
		MLClose(db_mathematicalink);
	}
#endif
#if LANGJAVA
	java_freememory();
#endif
}
