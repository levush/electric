/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlexpr.c
 * Expressions handling for the VHDL front-end compiler
 * Written by: Andrew R. Kostiuk, Queen's University
 * Modified by: Steven M. Rubin, Static Free Software
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
#if VHDLTOOL

#include "global.h"
#include "vhdl.h"
#include <math.h>

extern TOKENLIST	*vhdl_nexttoken;
extern SYMBOLLIST	*vhdl_symbols;

/* prototypes for local routines */
static MRELATIONS *vhdl_parsemorerelations(INTBIG, INTBIG);
static RELATION *vhdl_parserelation(void);
static TERM *vhdl_parseterm(void);
static MTERMS *vhdl_parsemoreterms(void);
static FACTOR *vhdl_parsefactor(void);
static MFACTORS *vhdl_parsemorefactors(void);
static PRIMARY *vhdl_parseprimary(void);
static LITERAL *vhdl_parseliteral(void);
static INTBIG vhdl_parsedecimal(void);
static INTBIG vhdl_evalrelation(RELATION*);
static INTBIG vhdl_evalterm(TERM*);
static INTBIG vhdl_evalfactor(FACTOR*);
static INTBIG vhdl_evalprimary(PRIMARY*);
static INTBIG vhdl_evalname(VNAME*);
static BOOLEAN vhdl_indiscreterange(INTBIG, DBDISCRETERANGE*);
static SINGLENAME *vhdl_parsesinglename(void);
static INDEXEDNAME *vhdl_parseindexedname(INTBIG, CHAR*);
static DBNAME *vhdl_semsinglename(SINGLENAME*);
static DBNAME *vhdl_semconcatenatedname(CONCATENATEDNAME*);
static DBNAME *vhdl_semindexedname(INDEXEDNAME*);

/*
Module:  vhdl_parseexpression
------------------------------------------------------------------------
Description:
	Parse an expression of the form:

		expression ::=
			  relation {AND relation}
			| relation {OR relation}
			| relation {NAND relation}
			| relation {NOR relation}
			| relation {XOR relation}
------------------------------------------------------------------------
Calling Sequence:  exp = vhdl_parseexpression();

Name		Type		Description
----		----		-----------
exp			*EXPRESSION	Returned expression structure.
------------------------------------------------------------------------
*/
EXPRESSION  *vhdl_parseexpression(void)
{
	EXPRESSION *exp;
	INTBIG key, logop;

	exp = (EXPRESSION *)emalloc((INTBIG)sizeof(EXPRESSION), vhdl_tool->cluster);
	exp->relation = vhdl_parserelation();
	exp->next = 0;

	/* check for more terms */
	logop = NOLOGOP;
	if (vhdl_nexttoken->token == TOKEN_KEYWORD)
	{
		key = ((VKEYWORD *)(vhdl_nexttoken->pointer))->num;
		switch (key = ((VKEYWORD *)(vhdl_nexttoken->pointer))->num)
		{
			case KEY_AND:
				logop = LOGOP_AND;
				break;
			case KEY_OR:
				logop = LOGOP_OR;
				break;
			case KEY_NAND:
				logop = LOGOP_NAND;
				break;
			case KEY_NOR:
				logop = LOGOP_NOR;
				break;
			case KEY_XOR:
				logop = LOGOP_XOR;
				break;
			default:
				break;
		}
	}

	if (logop != NOLOGOP)
	{
		exp->next = vhdl_parsemorerelations((INTBIG)key, (INTBIG)logop);
	}

	return(exp);
}

/*
Module:  vhdl_parsemorerelations
------------------------------------------------------------------------
Description:
	Parse more relations of an expression of the form:

		AND | OR | NAND | NOR | XOR  relation
------------------------------------------------------------------------
Calling Sequence:  more = vhdl_parsemorerelations(keyop, logop);

Name		Type		Description
----		----		-----------
keyop		INTBIG		Key of logical operator.
logop		INTBIG		Logical operator.
more		*MRELATIONS	Returned pointer to more relations,
								0 if no more.

Note:  The logical operator must be the same throughout.
------------------------------------------------------------------------
*/
MRELATIONS  *vhdl_parsemorerelations(INTBIG key, INTBIG logop)
{
	MRELATIONS *more;

	more = 0;

	if (vhdl_keysame(vhdl_nexttoken, key))
	{
		vhdl_getnexttoken();
		more = (MRELATIONS *)emalloc((INTBIG)sizeof(MRELATIONS), vhdl_tool->cluster);
		more->log_operator = logop;
		more->relation = vhdl_parserelation();
		more->next = vhdl_parsemorerelations(key, logop);
	}
	return(more);
}

/*
Module:  vhdl_parserelation
------------------------------------------------------------------------
Description:
	Parse a relation of the form:

		relation ::=
			simple_expression [relational_operator simple_expression]

		relational_operator ::=
			=  |  /=  |  <  |  <=  |  >  |  >=
------------------------------------------------------------------------
Calling Sequence:  relation = vhdl_parserelation();

Name		Type		Description
----		----		-----------
relation	*RELATION	Pointer to returned relation structure.
------------------------------------------------------------------------
*/
RELATION  *vhdl_parserelation(void)
{
	RELATION *relation;
	INTBIG relop;

	relop = NORELOP;
	relation = (RELATION *)emalloc((INTBIG)sizeof(RELATION), vhdl_tool->cluster);
	relation->simple_expr = vhdl_parsesimpleexpression();
	relation->rel_operator = NORELOP;
	relation->simple_expr2 = 0;

	switch (vhdl_nexttoken->token)
	{
		case TOKEN_EQ:
			relop = RELOP_EQ;
			break;
		case TOKEN_NE:
			relop = RELOP_NE;
			break;
		case TOKEN_LT:
			relop = RELOP_LT;
			break;
		case TOKEN_LE:
			relop = RELOP_LE;
			break;
		case TOKEN_GT:
			relop = RELOP_GT;
			break;
		case TOKEN_GE:
			relop = RELOP_GE;
			break;
		default:
			break;
	}

	if (relop != NORELOP)
	{
		relation->rel_operator = relop;
		vhdl_getnexttoken();
		relation->simple_expr2 = vhdl_parsesimpleexpression();
	}

	return(relation);
}

/*
Module:  vhdl_parsesimpleexpression
------------------------------------------------------------------------
Description:
	Parse a simple expression of the form:

		simple_expression ::=
			[sign] term {adding_operator term}
------------------------------------------------------------------------
Calling Sequence:  exp = vhdl_parsesimpleexpression();

Name		Type		Description
----		----		-----------
exp			*SIMPLEEXPR	Returned simple expression structure.
------------------------------------------------------------------------
*/
SIMPLEEXPR  *vhdl_parsesimpleexpression(void)
{
	SIMPLEEXPR *exp;

	exp = (SIMPLEEXPR *)emalloc((INTBIG)sizeof(SIMPLEEXPR), vhdl_tool->cluster);

	/* check for optional sign */
	if (vhdl_nexttoken->token == TOKEN_PLUS)
	{
		exp->sign = 1;
		vhdl_getnexttoken();
	} else if (vhdl_nexttoken->token == TOKEN_MINUS)
	{
		exp->sign = -1;
		vhdl_getnexttoken();
	} else
	{
		exp->sign = 1;			/* default sign */
	}

	/* next is a term */
	exp->term = vhdl_parseterm();

	/* check for more terms */
	exp->next = vhdl_parsemoreterms();

	return(exp);
}

/*
Module:  vhdl_parseterm
------------------------------------------------------------------------
Description:
	Parse a term of the form:

		term ::=
			factor {multiplying_operator factor}
------------------------------------------------------------------------
Calling Sequence:  term = vhdl_parseterm();

Name		Type		Description
----		----		-----------
term		*TERM		Returned term structure.
------------------------------------------------------------------------
*/
TERM  *vhdl_parseterm(void)
{
	TERM *term;

	term = (TERM *)emalloc((INTBIG)sizeof(TERM), vhdl_tool->cluster);
	term->factor = vhdl_parsefactor();
	term->next = vhdl_parsemorefactors();
	return(term);
}

/*
Module:  vhdl_parsemoreterms
------------------------------------------------------------------------
Description:
	Parse more terms of a simple expression of the form:

		adding_operator term
------------------------------------------------------------------------
Calling Sequence:  more = vhdl_parsemoreterms();

Name		Type		Description
----		----		-----------
more		*MTERMS		Returned pointer to more terms,
								0 if no more.
------------------------------------------------------------------------
*/
MTERMS  *vhdl_parsemoreterms(void)
{
	MTERMS *more;
	INTBIG addop;

	more = 0;
	addop = NOADDOP;
	if (vhdl_nexttoken->token == TOKEN_PLUS)
	{
		addop = ADDOP_ADD;
	} else if (vhdl_nexttoken->token == TOKEN_MINUS)
	{
		addop = ADDOP_SUBTRACT;
	}
	if (addop != NOADDOP)
	{
		vhdl_getnexttoken();
		more = (MTERMS *)emalloc((INTBIG)sizeof(MTERMS), vhdl_tool->cluster);
		more->add_operator = addop;
		more->term = vhdl_parseterm();
		more->next = vhdl_parsemoreterms();
	}
	return(more);
}

/*
Module:  vhdl_parsefactor
------------------------------------------------------------------------
Description:
	Parse a factor of the form:

		factor :==
			  primary [** primary]
			| ABS primary
			| NOT primary
------------------------------------------------------------------------
Calling Sequence:  factor = vhdl_parsefactor();

Name		Type		Description
----		----		-----------
factor		*FACTOR		Returned factor structure.
------------------------------------------------------------------------
*/
FACTOR  *vhdl_parsefactor(void)
{
	FACTOR *factor;
	INTBIG miscop;
	PRIMARY *primary, *primary2;

	factor = 0;
	primary = primary2 = 0;
	miscop = NOMISCOP;
	if (vhdl_keysame(vhdl_nexttoken, KEY_ABS))
	{
		miscop = MISCOP_ABS;
		vhdl_getnexttoken();
		primary = vhdl_parseprimary();
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_NOT))
	{
		miscop = MISCOP_NOT;
		vhdl_getnexttoken();
		primary = vhdl_parseprimary();
	} else
	{
		primary = vhdl_parseprimary();
		if (vhdl_nexttoken->token == TOKEN_DOUBLESTAR)
		{
			miscop = MISCOP_POWER;
			vhdl_getnexttoken();
			primary2 = vhdl_parseprimary();
		}
	}
	factor = (FACTOR *)emalloc((INTBIG)sizeof(FACTOR), vhdl_tool->cluster);
	factor->primary = primary;
	factor->misc_operator = miscop;
	factor->primary2 = primary2;
	return(factor);
}

/*
Module:  vhdl_parsemorefactors
------------------------------------------------------------------------
Description:
	Parse more factors of a term of the form:

		multiplying_operator factor
------------------------------------------------------------------------
Calling Sequence:  more = vhdl_parsemorefactors();

Name		Type		Description
----		----		-----------
more		*MFACTORS	Returned pointer to more factors,
								0 if no more.
------------------------------------------------------------------------
*/
MFACTORS  *vhdl_parsemorefactors(void)
{
	MFACTORS *more;
	INTBIG mulop;

	more = 0;
	mulop = NOMULOP;
	if (vhdl_nexttoken->token == TOKEN_STAR)
	{
		mulop = MULOP_MULTIPLY;
	} else if (vhdl_nexttoken->token == TOKEN_SLASH)
	{
		mulop = MULOP_DIVIDE;
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_MOD))
	{
		mulop = MULOP_MOD;
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_REM))
	{
		mulop = MULOP_REM;
	}
	if (mulop != NOMULOP)
	{
		vhdl_getnexttoken();
		more = (MFACTORS *)emalloc((INTBIG)sizeof(MFACTORS), vhdl_tool->cluster);
		more->mul_operator = mulop;
		more->factor = vhdl_parsefactor();
		more->next = vhdl_parsemorefactors();
	}
	return(more);
}

/*
Module:  vhdl_parseprimary
------------------------------------------------------------------------
Description:
	Parse a primary of the form:

		primary ::=
			  name
			| literal
			| aggregate
			| concatenation
			| function_call
			| type_conversion
			| qualified_expression
			| (expression)
------------------------------------------------------------------------
Calling Sequence:  primary = vhdl_parseprimary();

Name		Type		Description
----		----		-----------
primary		*PRIMARY	Returned primary structure.
------------------------------------------------------------------------
*/
PRIMARY  *vhdl_parseprimary(void)
{
	PRIMARY *primary;
	INTBIG type;
	CHAR *pointer;

	type = NOPRIMARY;
	primary = 0;
	switch (vhdl_nexttoken->token)
	{
		case TOKEN_DECIMAL:
		case TOKEN_BASED:
		case TOKEN_STRING:
		case TOKEN_BIT_STRING:
			type = PRIMARY_LITERAL;
			pointer = (CHAR *)vhdl_parseliteral();
			break;
		case TOKEN_IDENTIFIER:
			type = PRIMARY_NAME;
			pointer = (CHAR *)vhdl_parsename();
			break;
		case TOKEN_LEFTBRACKET:
			/* should be an expression in brackets */
			vhdl_getnexttoken();
			type = PRIMARY_EXPRESSION;
			pointer = (CHAR *)vhdl_parseexpression();

			/* should be at right bracket */
			if (vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
			{
				vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a right bracket"));
			}
			vhdl_getnexttoken();
			break;
		default:
			break;
	}
	if (type != NOPRIMARY)
	{
		primary = (PRIMARY *)emalloc((INTBIG)sizeof(PRIMARY), vhdl_tool->cluster);
		primary->type = type;
		primary->pointer = pointer;
	}
	return(primary);
}

/*
Module:  vhdl_parseliteral
------------------------------------------------------------------------
Description:
	Parse a literal of the form:

		literal ::=
			  numeric_literal
			| enumeration_literal
			| string_literal
			| bit_string_literal
------------------------------------------------------------------------
Calling Sequence:  literal = vhdl_parseliteral();

Name		Type		Description
----		----		-----------
literal		*LITERAL	Pointer to returned literal structure.
------------------------------------------------------------------------
*/
LITERAL  *vhdl_parseliteral(void)
{
	LITERAL *literal;
	INTBIG type;
	CHAR *pointer;

	literal = 0;
	type = NOLITERAL;
	switch(vhdl_nexttoken->token)
	{
		case TOKEN_DECIMAL:
			type = LITERAL_NUMERIC;
			pointer = (CHAR *)vhdl_parsedecimal();
			break;
		case TOKEN_BASED:
			/* type = LITERAL_NUMERIC;
			pointer = vhdl_parsebased(); */
			break;
		case TOKEN_STRING:
			break;
		case TOKEN_BIT_STRING:
			break;
		default:
			break;
	}
	if (type != NOLITERAL)
	{
		literal = (LITERAL *)emalloc((INTBIG)sizeof(LITERAL), vhdl_tool->cluster);
		literal->type = type;
		literal->pointer = pointer;
	}
	return(literal);
}

/*
Module:  vhdl_parsedecimal
------------------------------------------------------------------------
Description:
	Parse a decimal literal of the form:

		decimal_literal ::= integer [.integer] [exponent]

		integer ::= digit {[underline] digit}
		exponent ::= E [+] integer | E - integer

	Note:  Currently only integer supported.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_parsedecimal();

Name		Type		Description
----		----		-----------
value		INTBIG		Value of decimal literal.
------------------------------------------------------------------------
*/
INTBIG vhdl_parsedecimal(void)
{
	INTBIG value;

	value = 0;
	/* only supports integers */
	value = (INTBIG)eatoi(vhdl_nexttoken->pointer);
	vhdl_getnexttoken();
	return(value);
}

/*
Module:  vhdl_evalexpression
------------------------------------------------------------------------
Description:
	Return the value of an expression.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalexpression(expr);

Name		Type		Description
----		----		-----------
expr		*EPRESSION	Pointer to expression structure.
value		INTBIG		Returned value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalexpression(EXPRESSION *expr)
{
	INTBIG value, value2;
	MRELATIONS *more;

	if (expr == 0) return(0);
	value = vhdl_evalrelation(expr->relation);
	if (expr->next)
	{
		if (value) value = 1;
	}
	for (more = expr->next; more != 0; more = more->next)
	{
		value2 = vhdl_evalrelation(more->relation);
		if (value2) value2 = 1;
		switch (more->log_operator)
		{
			case LOGOP_AND:
				value &= value2;
				break;
			case LOGOP_OR:
				value |= value2;
				break;
			case LOGOP_NAND:
				value = !(value & value2);
				break;
			case LOGOP_NOR:
				value = !(value | value2);
				break;
			case LOGOP_XOR:
				value ^= value2;
				break;
			default:
				break;
		}
	}
	return(value);
}

/*
Module:  vhdl_evalrelation
------------------------------------------------------------------------
Description:
	Evaluate a relation.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalrelation(relation);

Name		Type		Description
----		----		-----------
relation	*RELATION	Pointer to relation structure.
value		INTBIG		Returned evaluated value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalrelation(RELATION *relation)
{
	INTBIG value, value2;

	if (relation == 0) return(0);
	value = vhdl_evalsimpleexpr(relation->simple_expr);
	if (relation->rel_operator != NORELOP)
	{
		value2 = vhdl_evalsimpleexpr(relation->simple_expr2);
		switch (relation->rel_operator)
		{
			case RELOP_EQ:
				if (value == value2) value = TRUE; else
					value = FALSE;
				break;
			case RELOP_NE:
				if (value != value2) value = TRUE; else
					value = FALSE;
				break;
			case RELOP_LT:
				if (value < value2) value = TRUE; else
					value = FALSE;
				break;
			case RELOP_LE:
				if (value <= value2) value = TRUE; else
					value = FALSE;
				break;
			case RELOP_GT:
				if (value > value2) value = TRUE; else
					value = FALSE;
				break;
			case RELOP_GE:
				if (value >= value2) value = TRUE; else
					value = FALSE;
				break;
			default:
				break;
		}
	}
	return(value);
}

/*
Module:  vhdl_evalsimpleexpr
------------------------------------------------------------------------
Description:
	Return the value of a simple expression.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalsimpleexpr(expr);

Name		Type		Description
----		----		-----------
expr		*SIMPLEEXPR	Pointer to a simple expression.
value		INTBIG		Returned value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalsimpleexpr(SIMPLEEXPR *expr)
{
	INTBIG value, value2;
	MTERMS *more;

	if (expr == 0) return(0);
	value = vhdl_evalterm(expr->term) * expr->sign;
	for (more = expr->next; more != 0; more = more->next)
	{
		value2 = vhdl_evalterm(more->term);
		switch (more->add_operator)
		{
			case ADDOP_ADD:
				value += value2;
				break;
			case ADDOP_SUBTRACT:
				value -= value2;
				break;
			default:
				break;
		}
	}
	return(value);
}

/*
Module:  vhdl_evalterm
------------------------------------------------------------------------
Description:
	Return the value of a term.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalterm(term);

Name		Type		Description
----		----		-----------
term		*TERM		Pointer to a term.
value		INTBIG		Returned value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalterm(TERM *term)
{
	INTBIG value, value2;
	MFACTORS *more;

	if (term == 0) return(0);
	value = vhdl_evalfactor(term->factor);
	for (more = term->next; more != 0; more = more->next)
	{
		value2 = vhdl_evalfactor(more->factor);
		switch (more->mul_operator)
		{
			case MULOP_MULTIPLY:
				value *= value2;
				break;
			case MULOP_DIVIDE:
				value /= value2;
				break;
			case MULOP_MOD:
				value %= value2;
				break;
			case MULOP_REM:
				value -= (INTBIG)(value / value2) * value2;
				break;
			default:
				break;
		}
	}
	return(value);
}

/*
Module:  vhdl_evalfactor
------------------------------------------------------------------------
Description:
	Return the value of a factor.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalfactor(factor);

Name		Type		Description
----		----		-----------
factor		*FACTOR		Pointer to a factor.
value		INTBIG		Returned value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalfactor(FACTOR *factor)
{
	INTBIG value, value2;

	if (factor == 0) return(0);
	value = vhdl_evalprimary(factor->primary);
	switch (factor->misc_operator)
	{
		case MISCOP_POWER:
			value2 = vhdl_evalprimary(factor->primary2);
			while (value2--)
			{
				value += value;
			}
			break;
		case MISCOP_ABS:
			value = abs(value);
			break;
		case MISCOP_NOT:
			if (value) value = 0; else
				value = 1;
			break;
		default:
			break;
	}
	return(value);
}

/*
Module:  vhdl_evalprimary
------------------------------------------------------------------------
Description:
	Evaluate the value of a primary and return.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalprimary(primary);

Name		Type		Description
----		----		-----------
primary		*PRIMARY	Pointer to primary structure.
value		INTBIG		Returned evaluated value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalprimary(PRIMARY *primary)
{
	INTBIG value;
	LITERAL *literal;

	if (primary == 0) return(0);
	value = 0;
	switch (primary->type)
	{
		case PRIMARY_LITERAL:
			if ((literal = (LITERAL *)primary->pointer) == 0) break;
			switch (literal->type)
			{
				case LITERAL_NUMERIC:
					value = (INTBIG)literal->pointer;
					break;
				case LITERAL_ENUMERATION:
				case LITERAL_STRING:
				case LITERAL_BIT_STRING:
				default:
					break;
			}
			break;
		case PRIMARY_NAME:
			value = vhdl_evalname((VNAME *)primary->pointer);
			break;
		case PRIMARY_EXPRESSION:
			value = vhdl_evalexpression((EXPRESSION *)primary->pointer);
			break;
		case PRIMARY_AGGREGATE:
		case PRIMARY_CONCATENATION:
		case PRIMARY_FUNCTION_CALL:
		case PRIMARY_TYPE_CONVERSION:
		case PRIMARY_QUALIFIED_EXPR:
		default:
			break;
	}
	return(value);
}

/*
Module:  vhdl_evalname
------------------------------------------------------------------------
Description:
	Evaluate and return the value of a name.
------------------------------------------------------------------------
Calling Sequence:  value = vhdl_evalname(name);

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to name.
value		INTBIG		Returned value, 0 if no value.
------------------------------------------------------------------------
*/
INTBIG vhdl_evalname(VNAME *name)
{
	INTBIG value;
	SYMBOLTREE *symbol;

	if (name == 0) return(0);
	value = 0;
	if ((symbol = vhdl_searchsymbol(vhdl_getnameident(name), vhdl_symbols)) == 0)
	{
		vhdl_reporterrormsg(vhdl_getnametoken(name), _("Symbol is undefined"));
		return(value);
	}
	if (symbol->type == SYMBOL_VARIABLE)
	{
		value = (INTBIG)symbol->pointer;
	} else if (symbol->type == SYMBOL_CONSTANT)
	{
		value = (INTBIG)symbol->pointer;
	} else
	{
		vhdl_reporterrormsg(vhdl_getnametoken(name), _("Cannot evaluate value of symbol"));
		return(value);
	}
	return(value);
}

/*
Module:  vhdl_indiscreterange
------------------------------------------------------------------------
Description:
	Return TRUE if value is in discrete range, else return FALSE.
------------------------------------------------------------------------
Calling Sequence:  in_range = vhdl_indiscreterange(value, discrete);

Name		Type				Description
----		----				-----------
value		INTBIG				Value to be checked.
discrete	*DBDISCRETERANGE	Pointer to db discrete range structure.
in_range	INTBIG				Returned value, TRUE if value in
									discrete range, else FALSE.
------------------------------------------------------------------------
*/
BOOLEAN vhdl_indiscreterange(INTBIG value, DBDISCRETERANGE *discrete)
{
	BOOLEAN in_range;
	INTBIG start, end, temp;

	in_range = FALSE;
	if (discrete == 0)
		return(in_range);
	start = discrete->start;
	end = discrete->end;
	if (start > end)
	{
		temp = end;
		end = start;
		start = temp;
	}
	if (value >= start && value <= end)
		in_range = TRUE;
	return(in_range);
}

/******************** this used to be the file "names.c" ********************/

/*
Module:  vhdl_parsename
------------------------------------------------------------------------
Description:
	Parse a name.  The form of a name is:

		name :==
			  single_name
			| concatenated_name
			| attribute_name
------------------------------------------------------------------------
Calling Sequence:  name = vhdl_parsename();

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to name parse tree.
------------------------------------------------------------------------
*/
VNAME  *vhdl_parsename(void)
{
	VNAME *name;
	INTBIG type;
	CHAR *pointer, *pointer2;
	CONCATENATEDNAME *concat, *concat2;

	name = NULL;
	type = NONAME;
	pointer = (CHAR *)vhdl_parsesinglename();

	switch (vhdl_nexttoken->token)
	{
		case TOKEN_AMPERSAND:
			type = NAME_CONCATENATE;
			concat = (CONCATENATEDNAME *)emalloc((INTBIG)sizeof(CONCATENATEDNAME), vhdl_tool->cluster);
			concat->name = (SINGLENAME *)pointer;
			concat->next = NULL;
			pointer = (CHAR *)concat;
			while (vhdl_nexttoken->token == TOKEN_AMPERSAND)
			{
				vhdl_getnexttoken();
				pointer2 = (CHAR *)vhdl_parsesinglename();
				concat2 = (CONCATENATEDNAME *)emalloc((INTBIG)sizeof(CONCATENATEDNAME), vhdl_tool->cluster);
				concat->next = concat2;
				concat2->name = (SINGLENAME *)pointer2;
				concat2->next = NULL;
				concat = concat2;
			}
			break;
		case TOKEN_APOSTROPHE:
			break;
		default:
			type = NAME_SINGLE;
		break;
	}

	if (type != NONAME)
	{
		name = (VNAME *)emalloc((INTBIG)sizeof(VNAME), vhdl_tool->cluster);
		name->type = type;
		name->pointer = pointer;
	} else
	{
		vhdl_getnexttoken();
	}
	return(name);
}

/*
Module:  vhdl_parsesinglename
------------------------------------------------------------------------
Description:
	Parse a single name.  Single names are of the form:

		single_name :==
			  simple_name
			| selected_name
			| indexed_name
			| slice_name
------------------------------------------------------------------------
Calling Sequence:  sname = vhdl_parsesinglename();

Name		Type		Description
----		----		-----------
sname		*SINGLENAME	Pointer to single name structure.
------------------------------------------------------------------------
*/
SINGLENAME  *vhdl_parsesinglename(void)
{
	SINGLENAME *sname, *sname2;
	INTBIG type;
	CHAR *pointer;
	VNAME *nptr;

	type = NOSINGLENAME;
	sname = NULL;
	pointer = (CHAR *)vhdl_parsesimplename();

	if (vhdl_nexttoken->last->space)
	{
		type = SINGLENAME_SIMPLE;
	} else
	{
		switch (vhdl_nexttoken->token)
		{
			case TOKEN_PERIOD:
				break;
			case TOKEN_LEFTBRACKET:
				/* could be a indexed_name or a slice_name */
				/* but support only indexed names */
				vhdl_getnexttoken();
				type = SINGLENAME_INDEXED;
				nptr = (VNAME *)emalloc((INTBIG)sizeof(VNAME), vhdl_tool->cluster);
				nptr->type = NAME_SINGLE;
				sname2 = (SINGLENAME *)emalloc((INTBIG)sizeof(SINGLENAME), vhdl_tool->cluster);
				nptr->pointer = (CHAR *)sname2;
				sname2->type = SINGLENAME_SIMPLE;
				sname2->pointer = pointer;
				pointer = (CHAR *)vhdl_parseindexedname((INTBIG)PREFIX_NAME, (CHAR *)nptr);
				/* should be at right bracket */
				if (vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
				{
					vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a right bracket"));
				}
				vhdl_getnexttoken();
				break;
			default:
				type = SINGLENAME_SIMPLE;
				break;
		}
	}

	if (type != NOSINGLENAME)
	{
		sname = (SINGLENAME *)emalloc((INTBIG)sizeof(SINGLENAME), vhdl_tool->cluster);
		sname->type = type;
		sname->pointer = pointer;
	} else
	{
		vhdl_getnexttoken();
	}
	return(sname);
}

/*
Module:  vhdl_parsesimplename
------------------------------------------------------------------------
Description:
	Parse a simple name of the form:

		simple_name ::= identifier
------------------------------------------------------------------------
Calling Sequence:  sname = vhdl_parsesimplename();

Name		Type		Description
----		----		-----------
sname		*SIMPLENAME	Pointer to simple name structure.
------------------------------------------------------------------------
*/
SIMPLENAME  *vhdl_parsesimplename(void)
{
	SIMPLENAME *sname;

	sname = NULL;
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		vhdl_getnexttoken();
		return(sname);
	}
	sname = (SIMPLENAME *)emalloc((INTBIG)sizeof(SIMPLENAME), vhdl_tool->cluster);
	sname->identifier = vhdl_nexttoken;
	vhdl_getnexttoken();
	return(sname);
}

/*
Module:  vhdl_parseindexedname
------------------------------------------------------------------------
Description:
	Parse an indexed name given its prefix and now at the index.  The
	form of an indexed name is:

		indexed_name ::= prefix(expression{, expression})
------------------------------------------------------------------------
Calling Sequence:  pointer = vhdl_parseindexedname(pre_type, pre_ptr);

Name		Type			Description
----		----			-----------
pre_type	INTBIG			Type of prefix (VNAME or FUNCTION CALL).
pre_ptr		*char			Pointer to prefix structure.
pointer		*INDEXEDNAME	Returned pointer to indexed name.
------------------------------------------------------------------------
*/
INDEXEDNAME  *vhdl_parseindexedname(INTBIG pre_type, CHAR *pre_ptr)
{
	PREFIX *prefix;
	INDEXEDNAME *ind;
	EXPRLIST *elist, *newelist;

	prefix = (PREFIX *)emalloc((INTBIG)sizeof(PREFIX), vhdl_tool->cluster);
	prefix->type = pre_type;
	prefix->pointer = pre_ptr;
	ind = (INDEXEDNAME *)emalloc((INTBIG)sizeof(INDEXEDNAME), vhdl_tool->cluster);
	ind->prefix = prefix;
	ind->expr_list = (EXPRLIST *)emalloc((INTBIG)sizeof(EXPRLIST), vhdl_tool->cluster);
	ind->expr_list->expression = vhdl_parseexpression();
	ind->expr_list->next = NULL;
	elist = ind->expr_list;

	/* continue while at a comma */
	while (vhdl_nexttoken->token == TOKEN_COMMA)
	{
		vhdl_getnexttoken();
		newelist = (EXPRLIST *)emalloc((INTBIG)sizeof(EXPRLIST), vhdl_tool->cluster);
		newelist->expression = vhdl_parseexpression();
		newelist->next = NULL;
		elist->next = newelist;
		elist = newelist;
	}

	return(ind);
}

/*
Module:  vhdl_getnameident
------------------------------------------------------------------------
Description:
	Given a pointer to a name, return its reference to an global
	namespace entry.
------------------------------------------------------------------------
Calling Sequence:  itable = vhdl_getnameident(name);

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to name structure.
itable		*IDENTTABLE	Returned pointer to global name space,
								NULL if not found.
------------------------------------------------------------------------
*/
IDENTTABLE  *vhdl_getnameident(VNAME *name)
{
	IDENTTABLE *itable;
	SINGLENAME *singl;

	itable = NULL;
	if (name == NULL) return(itable);
	switch (name->type)
	{
		case NAME_SINGLE:
			singl = (SINGLENAME *)(name->pointer);
			switch (singl->type)
			{
				case SINGLENAME_SIMPLE:
					itable = (IDENTTABLE *)((SIMPLENAME *)(singl->pointer))->identifier->pointer;
					break;
				case SINGLENAME_INDEXED:
					itable = vhdl_getprefixident(((INDEXEDNAME *)(singl->pointer))->prefix);
					break;
				case SINGLENAME_SELECTED:
				case SINGLENAME_SLICE:
				default:
					break;
			}
			break;
		case NAME_CONCATENATE:
		case NAME_ATTRIBUTE:
		default:
			break;
	}
	return(itable);
}

/*
Module:  vhdl_getprefixident
------------------------------------------------------------------------
Description:
	Given a pointer to a prefix, return its reference to an global
	namespace entry.
------------------------------------------------------------------------
Calling Sequence:  itable = vhdl_getprefixident(prefix);

Name		Type		Description
----		----		-----------
prefix		*PREFIX		Pointer to prefix structure.
itable		*IDENTTABLE	Returned pointer to global name space,
								NULL if not found.
------------------------------------------------------------------------
*/
IDENTTABLE  *vhdl_getprefixident(PREFIX *prefix)
{
	IDENTTABLE *itable;

	itable = NULL;
	if (prefix == NULL) return(itable);
	switch (prefix->type)
	{
		case PREFIX_NAME:
			itable = vhdl_getnameident((VNAME *)prefix->pointer);
			break;
		case PREFIX_FUNCTION_CALL:
		default:
			break;
	}
	return(itable);
}

/*
Module:  vhdl_getnametoken
------------------------------------------------------------------------
Description:
	Given a pointer to a name, return its reference to a token.
------------------------------------------------------------------------
Calling Sequence:  token = vhdl_getnametoken(name);

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to name structure.
token		*TOKENLIST	Returned pointer to token,
								NULL if not found.
------------------------------------------------------------------------
*/
TOKENLIST  *vhdl_getnametoken(VNAME *name)
{
	TOKENLIST *token;
	SINGLENAME *singl;

	token = NULL;
	if (name == NULL) return(token);
	switch (name->type)
	{
		case NAME_SINGLE:
			singl = (SINGLENAME *)(name->pointer);
			switch (singl->type)
			{
				case SINGLENAME_SIMPLE:
					token = ((SIMPLENAME *)(singl->pointer))->identifier;
					break;
				case SINGLENAME_SELECTED:
					break;
				case SINGLENAME_INDEXED:
					token = vhdl_getprefixtoken(((INDEXEDNAME *)(singl->pointer))->prefix);
					break;
				case SINGLENAME_SLICE:
				default:
					break;
			}
			break;
		case NAME_CONCATENATE:
		case NAME_ATTRIBUTE:
		default:
			break;
	}
	return(token);
}

/*
Module:  vhdl_getprefixtoken
------------------------------------------------------------------------
Description:
	Given a pointer to a prefix, return its reference to a token.
------------------------------------------------------------------------
Calling Sequence:  token = vhdl_getprefixtoken(prefix);

Name		Type		Description
----		----		-----------
prefix		*PREFIX		Pointer to prefix structure.
token		*TOKENLIST	Returned pointer to token,
								NULL if not found.
------------------------------------------------------------------------
*/
TOKENLIST  *vhdl_getprefixtoken(PREFIX *prefix)
{
	TOKENLIST *token;

	token = NULL;
	if (prefix == NULL) return(token);
	switch (prefix->type)
	{
		case PREFIX_NAME:
			token = vhdl_getnametoken((VNAME *)prefix->pointer);
			break;
		case PREFIX_FUNCTION_CALL:
		default:
			break;
	}
	return(token);
}

/*
Module:  vhdl_semname
------------------------------------------------------------------------
Description:
	Semantic analysis of a name.
------------------------------------------------------------------------
Calling Sequence:  dbname = vhdl_semname(name);

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to name structure.
dbname		*DBNAME		pointer to created db name.
------------------------------------------------------------------------
*/
DBNAME *vhdl_semname(VNAME *name)
{
	DBNAME *dbname;

	dbname = NULL;
	if (name == NULL) return(dbname);
	switch (name->type)
	{
		case NAME_SINGLE:
			dbname = vhdl_semsinglename((SINGLENAME *)name->pointer);
			break;
		case NAME_CONCATENATE:
			dbname = vhdl_semconcatenatedname((CONCATENATEDNAME *)name->pointer);
			break;
		case NAME_ATTRIBUTE:
		default:
			break;
	}
	return(dbname);
}

/*
Module:  vhdl_semsinglename
------------------------------------------------------------------------
Description:
	Semantic analysis of a single name.
------------------------------------------------------------------------
Calling Sequence:  dbname = vhdl_semsinglename(name);

Name		Type		Description
----		----		-----------
name		*SINGLENAME	Pointer to single name structure.
dbname		*DBNAME		Pointer to generated db name.
------------------------------------------------------------------------
*/
DBNAME *vhdl_semsinglename(SINGLENAME *name)
{
	DBNAME *dbname;

	dbname = NULL;
	if (name == NULL) return(dbname);
	switch (name->type)
	{
		case SINGLENAME_SIMPLE:
			dbname = (DBNAME *)emalloc((INTBIG)sizeof(DBNAME), vhdl_tool->cluster);
			dbname->name = (IDENTTABLE *)((SIMPLENAME *)(name->pointer))->identifier->pointer;
			dbname->type = DBNAME_IDENTIFIER;
			dbname->pointer = NULL;
			dbname->dbtype = vhdl_gettype(dbname->name);
			break;
		case SINGLENAME_INDEXED:
			dbname = vhdl_semindexedname((INDEXEDNAME *)name->pointer);
			break;
		case SINGLENAME_SLICE:
		case SINGLENAME_SELECTED:
		default:
			break;
	}
	return(dbname);
}

/*
Module:  vhdl_semconcatenatedname
------------------------------------------------------------------------
Description:
	Semantic analysis of a concatenated name.
------------------------------------------------------------------------
Calling Sequence:  dbname = vhdl_semconcatenatedname(name);

Name		Type				Description
----		----				-----------
name		*CONCATENATEDNAME	Pointer to concatenated name structure.
dbname		*DBNAME				Pointer to generated db name.
------------------------------------------------------------------------
*/
DBNAME  *vhdl_semconcatenatedname(CONCATENATEDNAME *name)
{
	CONCATENATEDNAME *cat;
	DBNAME *dbname;
	DBNAMELIST *end, *newnl;

	dbname = NULL;
	if (name == NULL) return(dbname);
	dbname = (DBNAME *)emalloc((INTBIG)sizeof(DBNAME), vhdl_tool->cluster);
	dbname->name = NULL;
	dbname->type = DBNAME_CONCATENATED;
	dbname->pointer = NULL;
	dbname->dbtype = NULL;
	end = NULL;
	for (cat = name; cat != NULL; cat = cat->next)
	{
		newnl = (DBNAMELIST *)emalloc((INTBIG)sizeof(DBNAMELIST), vhdl_tool->cluster);
		newnl->name = vhdl_semsinglename(cat->name);
		newnl->next = NULL;
		if (end)
		{
			end->next = newnl;
			end = newnl;
		} else
		{
			end = newnl;
			dbname->pointer = (CHAR *)newnl;
		}
	}
	return(dbname);
}

/*
Module:  vhdl_semindexedname
------------------------------------------------------------------------
Description:
	Semantic analysis of an indexed name.
------------------------------------------------------------------------
Calling Sequence:  dbname = vhdl_semindexedname(name);

Name		Type			Description
----		----			-----------
name		*INDEXEDNAME	Pointer to indexed name structure.
dbname		*DBNAME			Pointer to generated name.
------------------------------------------------------------------------
*/
DBNAME  *vhdl_semindexedname(INDEXEDNAME *name)
{
	DBNAME *dbname;
	EXPRLIST *expr;
	DBINDEXRANGE *indexr;
	DBEXPRLIST *dbexpr, *nexpr, *endexpr;
	INTBIG value;
	DBLTYPE *type;

	dbname = NULL;
	if (name == NULL) return(dbname);

	/* must be an array type */
	type = vhdl_gettype(vhdl_getprefixident(name->prefix));
	if (type == NULL)
	{
		vhdl_reporterrormsg(vhdl_getprefixtoken(name->prefix), _("No type specified"));
		return(dbname);
	}
	if (type->type != DBTYPE_ARRAY)
	{
		vhdl_reporterrormsg(vhdl_getprefixtoken(name->prefix), _("Must be of constrained array type"));
		return(dbname);
	}
	dbname = (DBNAME *)emalloc((INTBIG)sizeof(DBNAME), vhdl_tool->cluster);
	dbname->name = vhdl_getprefixident(name->prefix);
	dbname->type = DBNAME_INDEXED;
	dbname->pointer = NULL;
	dbname->dbtype = type;

	/* evaluate any expressions */
	indexr = (DBINDEXRANGE *)type->pointer;
	dbexpr = endexpr = NULL;
	for (expr = name->expr_list; expr && indexr; expr = expr->next)
	{
		value = vhdl_evalexpression(expr->expression);
		if (!vhdl_indiscreterange((INTBIG)value, indexr->drange))
		{
			vhdl_reporterrormsg(vhdl_getprefixtoken(name->prefix), _("Index is out of range"));
			return(dbname);
		}
		nexpr = (DBEXPRLIST *)emalloc((INTBIG)sizeof(DBEXPRLIST), vhdl_tool->cluster);
		nexpr->value = value;
		nexpr->next = NULL;
		if (endexpr == NULL)
		{
			dbexpr = endexpr = nexpr;
		} else
		{
			endexpr->next = nexpr;
			endexpr = nexpr;
		}
		indexr = indexr->next;
	}
	dbname->pointer = (CHAR *)dbexpr;
	return(dbname);
}

#endif  /* VHDLTOOL - at top */
