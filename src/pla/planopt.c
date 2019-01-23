/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: planopt.c
 * nMOS Programmable Logic Array Generator, option module
 * Written by: Sundaravarathan R. Iyengar, Schlumberger Palo Alto Research
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
 * Options to the nMOS PLA generator:
 *   set option = value  is the format for setting the various options.
 *
 * All spaces, tabs and newlines are ignored.  Also ignored are anything
 * appearing after ";" on a line or anything enclosed within "{}".
 *
 * The options are (default values within parantheses)
 *
 *  verbose        = ON | OFF  (OFF)    Print trace messages.
 *  flexible       = ON | OFF  (ON)     Make all the arcs flexible.
 *  fixedangle     = ON | OFF  (ON)     Make the arcs fixed-angle.
 *  buttingcontact = ON | OFF  (ON)     Use butting instead of buried contacts
 *  samesideoutput = ON | OFF  (ON)     Outputs and inputs on same side of PLA
 *  inputs         = integer  (0)       Number of inputs to the PLA
 *  outputs        = integer  (0)       Number of outputs from the PLA
 *  pterms         = integer  (0)       Number of product terms
 *  vddwidth       = integer  (4)       Width  of Vdd in lambda
 *  gndwidth       = integer  (4)       Width  of Ground in lambda
 *  name           = string (Anonymous) Name given to the PLA
 *
 *  These may appear in the input description file before the
 *  BEGIN directive which marks the start of the truth table description.
 *  They can also be placed in the .cadrc file in the $HOME directory.
 *  Thus options may be set in the input file to override options set in
 *  the .cadrc file.  For options set in the .cadrc file, precede each set
 *  directive by the string "electric telltool pla nmos", so that they can
 *  be distinguished unambiguously from those meant for other CAD tools.
 */

#include "config.h"
#if PLATOOL

#include "global.h"
#include "pla.h"
#include "planmos.h"

/*
 * Keywords recognized by the program.  NOTE: if this order is changed, same
 * changes should be made in the last set of #define statements in 'planmos.h'
 */
static CHAR *pla_KeyWords[] =
{
	x_("off"),
	x_("on"),
	x_("1"), x_("0"), x_("x"), x_("-"),
	x_("set"),
	x_("="),
	x_("verbose"),
	x_("flexible"),
	x_("fixedangle"),
	x_("buttingcontact"),
	x_("samesideoutput"),
	x_("inputs"),
	x_("outputs"),
	x_("pterms"),
	x_("vddwidth"),
	x_("groundwidth"),
	x_("file"),
	x_("name"),
	x_("begin"),
	x_("end"),
	x_("")
};

NODEPROTO *pla_pu_proto;		/* Pullup Prototype */
NODEPROTO *pla_in_proto;		/* Input Prototype */
NODEPROTO *pla_out_proto;		/* Output Prototype */
NODEPROTO *pla_prog_proto;		/* Programming Transistor Prototype */
NODEPROTO *pla_connect_proto;	/* Connect Prototype */

NODEPROTO *pla_cell;			/* The PLA itself */

NODEPROTO *pla_md_proto;		/* Metal-Diff contact */
NODEPROTO *pla_mp_proto;		/* Metal-Poly contact */
NODEPROTO *pla_bp_proto;		/* Metal (blue) pin */
NODEPROTO *pla_dp_proto;		/* Diff pin */
NODEPROTO *pla_pp_proto;		/* Poly pin */
NODEPROTO *pla_bc_proto;		/* Butting/Buried contact */
NODEPROTO *pla_dtran_proto;		/* Depletion mode transistor */
NODEPROTO *pla_etran_proto;		/* Enhancement mode transistor */

ARCPROTO  *pla_da_proto;		/* Diff arc */
ARCPROTO  *pla_pa_proto;		/* Poly arc */
ARCPROTO  *pla_ma_proto;		/* Metal arc */

/*  Options accessible to the designer */
BOOLEAN pla_verbose, pla_buttingcontact, pla_samesideoutput;
INTBIG  pla_userbits;
INTBIG  pla_inputs, pla_outputs, pla_pterms;
INTBIG  pla_VddWidth;			/* VDD width in use */
INTBIG  pla_GndWidth;			/* Vdd and Gnd Widths in use */
CHAR    pla_infile[100];		/* Name of the input file */
CHAR    pla_name[32];			/* Name of the PLA cell */

/*   Miscellaneous global variables  */
INTBIG    pla_halfVdd, pla_halfGnd;   /* To save repeated division by 2 */
VDDGND   *pla_alv;				/* And left Vdd points */
VDDGND   *pla_atg;				/* And top Gnd points */
VDDGND   *pla_arg;				/* And right Gnd points */
VDDGND   *pla_otv;				/* Or top Vdd points */
VDDGND   *pla_org;				/* Or right Gnd points */
VDDGND   *pla_icg;				/* Input cell Gnd points */
VDDGND   *pla_icv;				/* Input cell Vdd points */
VDDGND   *pla_ocg;				/* Output cell Gnd points */
VDDGND   *pla_ocv;				/* Output cell Vdd points */
FILE     *pla_fin;				/* Pointer to the input file */

/* prototypes for local routines */
static void    pla_Flag(INTBIG);
static INTBIG  pla_GetVal(void);
static BOOLEAN pla_ReadOffEqualSign(void);
static void    pla_Recover(void);
static void    pla_SetFlag(void);
static void    pla_SetName(void);
static BOOLEAN pla_SkipChars(void);
static CHAR   *pla_onoffmessage(INTBIG onoff);

/* Set the given option to ON or OFF */
void pla_Flag(INTBIG option)
{
	INTBIG onoroff;
	CHAR keyword[16];

	onoroff = pla_GetKeyWord(keyword);
	if (onoroff != ERRORRET)
	{
		if (onoroff != ON && onoroff != OFF)
		{
			ttyputerr(M_("'%s' may be set to ON or OFF only"), pla_KeyWords[option]);
			return;
		}
		switch(option)
		{
			case VERBOSE:
				if (onoroff == 0) pla_verbose = FALSE; else
					pla_verbose = TRUE;
				if (pla_verbose)
					ttyputmsg(M_("'verbose' set to %s"), pla_onoffmessage(pla_verbose));
				break;
			case FLEXIBLE:
				if (onoroff == ON) pla_userbits &= ~FIXED; else
					pla_userbits |= FIXED;
				if (pla_verbose)
					ttyputmsg(M_("'flexible' set to %s"), pla_onoffmessage(pla_userbits&FIXED));
				break;
			case FIXEDANGLE:
				if (onoroff == ON) pla_userbits |= FIXANG; else
					pla_userbits &= ~FIXANG;
				if (pla_verbose)
					ttyputmsg(M_("'fixedangle' set to %s"), pla_onoffmessage(pla_userbits&FIXANG));
				break;
			case BUTTINGCONTACT:
				if (onoroff == 0) pla_buttingcontact = FALSE; else
					pla_buttingcontact = TRUE;
				if (pla_verbose)
					ttyputmsg(M_("'buttingcontact' set to %s"), pla_onoffmessage(pla_buttingcontact));
				break;
			case SAMESIDEOUTPUT:
				pla_samesideoutput = TRUE;
				if (pla_verbose)
					ttyputmsg(M_("'samesideoutput' set to %s"), pla_onoffmessage(pla_samesideoutput));
				break;
		}
	}
}

/*
 * Get a keyword from the input stream.  The keywords are defined in
 * pla_KeyWords[] in pla.c.  Return ERRORRET on end-of-file else return OK.
 * The keyoword fround is returned via the input parameter.
 */
INTBIG pla_GetKeyWord(CHAR *keyword)
{
	CHAR *savekeywordaddr;
	INTBIG k, c;

	savekeywordaddr = keyword;

	if (pla_SkipChars()) return(ERRORRET);
	while((c = xgetc(pla_fin)) != EOF)
	{
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || (c == '=')) *keyword++ = (CHAR)c; else
		{
			*keyword = 0;
			xungetc((CHAR)c, pla_fin);
			break;
		}
	}

	if (c == EOF) return(ERRORRET);

	for(k = 0; k < MAXKEYWORDS; k++)
		if (namesame(savekeywordaddr, pla_KeyWords[k]) == 0) return(k);
	return(UNDEFINED);
}

/* Get a number from the input stream */
INTBIG pla_GetVal(void)
{
	CHAR keyword[16];
	INTBIG n;

	if (pla_GetKeyWord(keyword) != ERRORRET)
	{
		if ((n = eatoi(keyword)) == 0)
		{
			ttyputerr(M_("Inappropriate numbers in input"));
			return(ERRORRET);
		}
		return(n);
	}
	return(ERRORRET);
}

BOOLEAN pla_ProcessFlags(void)
{
	CHAR keyword[16];
	INTBIG position;

	while((position = pla_GetKeyWord(keyword)) != ERRORRET)
	{
		/*
		 * pla_GetKeyWord returns the position of the input word in the KeyWords
		 * list.  It also returns the keyword found in the array 'keyword'.
		 */
		switch(position)
		{
			case SET:		/* sets one flag and returns */
				pla_SetFlag();
				break;
			case BEGIN:		/* PLA definition begins */
				return(FALSE);
			case UNDEFINED:
				ttyputerr(M_("undefined string in input: '%s'"), keyword);
				pla_Recover();
				break;
			default:		/* Error.  We shouldn't be here */
				ttyputerr(M_("unknown error while reading '%s'"), keyword);
				pla_Recover();
				break;
		}
	}
	return(TRUE);
}

BOOLEAN pla_ReadOffEqualSign(void)
{
	CHAR keyword[16];

	if (pla_GetKeyWord(keyword) != ERRORRET)
	{
		if (namesame(keyword, x_("=")) == 0) return(FALSE);
		ttyputerr(M_("equal sign expected. Read '%s' instead."), keyword);
		pla_Recover();
	}
	return(TRUE);
}

void pla_Recover(void)
{
	CHAR keyword[16];

	ttyputmsg(M_("Skipping text until next 'set' or 'begin' directive ..."));

	while(pla_GetKeyWord(keyword) != ERRORRET)
	{
		if (namesame(keyword, x_("set")) == 0)
		{
			xungetc('t', pla_fin); xungetc('e', pla_fin);
			xungetc('s', pla_fin);
			return;
		}
		if (namesame(keyword, x_("begin")) == 0)
		{
			xungetc('n', pla_fin); xungetc('i', pla_fin);
			xungetc('g', pla_fin);
			xungetc('e', pla_fin); xungetc('b', pla_fin);
			return;
		}
	}
}

/* Sets one flag and returns */
void pla_SetFlag(void)
{
	CHAR keyword[16];
	INTBIG position;

	position = pla_GetKeyWord(keyword);
	if (position != ERRORRET)
	{
		switch(position)
		{
			case SET:
				ttyputerr(M_("Set must be followed by an option name"));
				pla_Recover();
				break;
			case EQUAL:
				ttyputerr(M_("option name missing"));
				pla_Recover();
				break;
			case INPUTS:
				if (!pla_ReadOffEqualSign()) pla_inputs = pla_GetVal();
				if (pla_verbose) ttyputmsg(M_("'inputs' set to %ld"), pla_inputs);
				break;
			case OUTPUTS:
				if (!pla_ReadOffEqualSign()) pla_outputs = pla_GetVal();
				if (pla_verbose) ttyputmsg(M_("'outputs' set to %ld"), pla_outputs);
				break;
			case PTERMS:
				if (!pla_ReadOffEqualSign()) pla_pterms = pla_GetVal();
				if (pla_verbose) ttyputmsg(M_("'pterms' set to %ld"), pla_pterms);
				break;
			case VDDWIDTH:
				if (!pla_ReadOffEqualSign())
					pla_VddWidth = pla_lam * pla_GetVal();
				if (pla_VddWidth < 4 * pla_lam) pla_VddWidth = 4 * pla_lam;
				pla_halfVdd = pla_VddWidth / 2;
				if (pla_verbose) ttyputmsg(M_("'Vdd Width' set to %s"), latoa(pla_VddWidth, 0));
				break;
			case GNDWIDTH:
				if (!pla_ReadOffEqualSign())
					pla_GndWidth = pla_lam * pla_GetVal();
				if (pla_GndWidth < 4 * pla_lam) pla_GndWidth = 4 * pla_lam;
				pla_halfGnd = pla_GndWidth / 2;
				if (pla_verbose) ttyputmsg(M_("'Gnd Width' set to %s"), latoa(pla_GndWidth, 0));
				break;
			case VERBOSE:
			case FLEXIBLE:
			case FIXEDANGLE:
			case BUTTINGCONTACT:
			case SAMESIDEOUTPUT:
				if (!pla_ReadOffEqualSign()) pla_Flag(position);
				break;
			case NAME:
				if (!pla_ReadOffEqualSign()) pla_SetName();
				break;
			case ONE:
			case ZERO:
			case DONTCAREX:
			case DONTCAREM:
			case UNDEFINED:
			case BEGIN:
			case ON:
			case OFF:
				ttyputerr(M_("invalid option name '%s'."), keyword);
				pla_Recover();
				break;
		}
		return;
	}
}

void pla_SetName(void)
{
	CHAR keyword[32];

	if (pla_GetKeyWord(keyword) != ERRORRET) (void)estrcpy(pla_name, keyword);
	if (pla_verbose)
		ttyputmsg(M_("Name of the PLA set to '%s'"), pla_name);
}

BOOLEAN pla_SkipChars(void)
{
	INTSML c;

	while((c = xgetc(pla_fin)) != EOF)
	{
		/*
		 * if within the English alphabet, or it is a number or an '=' sign
		 * return success.
		 */
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || (c == '='))
		{
			xungetc((CHAR)c, pla_fin);
			return(FALSE);
		}
		if (c == '{' || c == ';')  /* Skip over Comments */
		{
			switch(c)
			{
				case '{':
					while((c = xgetc(pla_fin)) != EOF && c != '}') ;
					break;
				case ';':
					while((c = xgetc(pla_fin)) != EOF && c != '\n') ;
					break;
			}
			if (c == EOF) return(FALSE);
		}
	}
	return(TRUE);
}

void pla_displayoptions(void)
{
	ttyputmsg(M_("nMOS PLA options and current values:"));
	ttyputmsg(M_("       Input file = %s"), pla_infile);
	ttyputmsg(M_(" Number of Inputs = %ld"), pla_inputs);
	ttyputmsg(M_("Number of Outputs = %ld"), pla_outputs);
	ttyputmsg(M_(" Number of Pterms = %ld"), pla_pterms);
	ttyputmsg(M_("	       Verbose = %s"), pla_onoffmessage(pla_verbose));
	ttyputmsg(M_("	      Flexible = %s"), pla_onoffmessage(pla_userbits&FIXED));
	ttyputmsg(M_("       Fixedangle = %s"), pla_onoffmessage(pla_userbits&FIXANG));
	ttyputmsg(M_("   Buttingcontact = %s"), pla_onoffmessage(pla_buttingcontact));
	ttyputmsg(M_("   Samesideoutput = %s"), pla_onoffmessage(pla_samesideoutput));
	ttyputmsg(M_("	     VDD Width = %s"), latoa(pla_VddWidth, 0));
	ttyputmsg(M_("	     Gnd Width = %s"), latoa(pla_GndWidth, 0));
	ttyputmsg(M_("  Name of the PLA = %s"), pla_name);
}

CHAR *pla_onoffmessage(INTBIG onoff)
{
	if (onoff != 0) return(M_("ON"));
	return(M_("OFF"));
}

void pla_setoption(CHAR *option, CHAR *value)
{
	INTBIG k, val, len;

	len = estrlen(option);
	if (namesame(option, NULLSTR) == 0)
	{
		ttyputerr(M_("Set must be followed by an option name"));
		return;
	}

	if (namesame(value, NULLSTR) == 0)
	{
		ttyputerr(M_("Options must be set to some value; not nil."));
		return;
	}
	for(k = 0; k < MAXKEYWORDS; k++)
		if (namesamen(option, pla_KeyWords[k], len) == 0) break;
	if (k >= MAXKEYWORDS)
	{
		ttyputbadusage(x_("telltool pla nmos"));
		return;
	}
	if (k >= VERBOSE && k <= SAMESIDEOUTPUT)
	{
		if (namesamen(value, x_("on"), len) == 0) val = 1; else
			if (namesamen(value, x_("off"), len) == 0) val = 0; else
		{
			ttyputerr(M_("Value must be ON or OFF"));
			return;
		}
		switch(k)
		{
			case VERBOSE:
				if (val != 0) pla_verbose = TRUE; else
					pla_verbose = FALSE;
				ttyputverbose(M_("'verbose' set to %s"), pla_onoffmessage(pla_verbose));
				break;
			case FLEXIBLE:
				if (val == ON) pla_userbits &= ~FIXED; else
					pla_userbits |= FIXED;
				ttyputverbose(M_("'flexible' set to %s"), pla_onoffmessage(pla_userbits&FIXED));
				break;
			case FIXEDANGLE:
				if (val == ON) pla_userbits |= FIXANG; else
					pla_userbits &= ~FIXANG;
				ttyputverbose(M_("'fixedangle' set to %s"), pla_onoffmessage(pla_userbits&FIXANG));
				break;
			case BUTTINGCONTACT:
				if (val != 0) pla_buttingcontact = TRUE; else
					pla_buttingcontact = FALSE;
				ttyputverbose(M_("'buttingcontact' set to %s"), pla_onoffmessage(pla_buttingcontact));
				break;
			case SAMESIDEOUTPUT:
				if (val != 0) pla_samesideoutput = TRUE; else
					pla_samesideoutput = FALSE;
				ttyputverbose(M_("'samesideoutput' set to %s"), pla_onoffmessage(pla_samesideoutput));
				break;
		}
		return;
	}

	if (k >= INPUTS && k <= GNDWIDTH)
	{
		val = eatoi(value);       /* Note:  this is incorrect because value */
		switch(k)		/*	may contain nondigit characters */
		{
			case INPUTS:
				pla_inputs = val;
				ttyputverbose(M_("Inputs set to %ld"), pla_inputs);
				break;
			case OUTPUTS:
				pla_outputs = val;
				ttyputverbose(M_("Outputs set to %ld"), pla_outputs);
				break;
			case PTERMS:
				pla_pterms = val;
				ttyputverbose(M_("Product terms set to %ld"), pla_pterms);
				break;
			case VDDWIDTH:
				if (val < 4) ttyputerr(M_("Vdd Width must be at least 4 lambda")); else
				{
					pla_VddWidth = val*pla_lam;
					pla_halfVdd  = pla_VddWidth / 2;
					ttyputverbose(M_("VDD width set to %s"), latoa(pla_VddWidth, 0));
				}
				break;
			case GNDWIDTH:
				if (val < 4) ttyputerr(M_("Ground Width must be at least 4 lambda")); else
				{
					pla_GndWidth = val*pla_lam;
					pla_halfGnd  = pla_GndWidth / 2;
					ttyputverbose(M_("Gnd width set to %s"), latoa(pla_GndWidth, 0));
				}
				break;
		}
		return;
	}
	if (k == FILENAME)
	{
		(void)estrncpy(pla_infile, value, 100);
		ttyputverbose(M_("File set to %s"), pla_infile);
	}
	if (k == NAME)
	{
		(void)estrncpy(pla_name, value, 32);
		ttyputverbose(M_("Name set to %s"), pla_name);
	}
}

#endif  /* PLATOOL - at top */
