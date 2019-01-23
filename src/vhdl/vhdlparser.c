/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlparser.c
 * This file contains the Parser for the VHDL front-end compiler
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
#include <setjmp.h>

#define MAXTEMP		10

extern CHAR		vhdl_delimiterstr[];
extern CHAR		vhdl_doubledelimiterstr[];
BOOLEAN			vhdl_err;
PTREE			*vhdl_ptree = NULL;
TOKENLIST		*vhdl_nexttoken;
static jmp_buf	vhdl_notoken_err;
INTBIG			vhdl_errorcount;

/* prototypes for local routines */
static VINTERFACE *vhdl_parseinterface(void);
static BODY *vhdl_parsebody(INTBIG);
static BODYDECLARE *vhdl_parsebody_declare(void);
static VCOMPONENT *vhdl_parsecomponent(void);
static BASICDECLARE *vhdl_parsebasic_declare(void);
static OBJECTDECLARE *vhdl_parseobject_declare(void);
static SIGNALDECLARE *vhdl_parsesignal_declare(void);
static CONSTANTDECLARE *vhdl_parseconstant_declare(void);
static IDENTLIST *vhdl_parseident_list(void);
static STATEMENTS *vhdl_parseset_of_statements(void);
static GENERATE *vhdl_parsegenerate(TOKENLIST*, INTBIG);
static RANGE *vhdl_parserange(void);
static RANGESIMPLE *vhdl_parserange_simple(void);
static VINSTANCE *vhdl_parseinstance(void);
static FPORTLIST *vhdl_parseformal_port_list(void);
static INTBIG vhdl_parseport_mode(void);
static APORTLIST *vhdl_parseactual_port_list(void);
static PACKAGE *vhdl_parsepackage(void);
static PACKAGEDPART *vhdl_parsepackage_declare_part(void);
static void vhdl_parsetosemicolon(void);
#ifdef VHDL50
  static WITH *vhdl_parsewith(void);
#endif
static USE *vhdl_parseuse(void);
static void vhdl_freeports(FPORTLIST*);
static void vhdl_freeidentlist(IDENTLIST*);
static void vhdl_freesimpleexpr(SIMPLEEXPR*);
static void vhdl_freeterm(TERM*);
static void vhdl_freefactor(FACTOR*);
static void vhdl_freeprimary(PRIMARY*);
static void vhdl_freename(VNAME*);
static void vhdl_freesinglename(SINGLENAME*);
static void vhdl_freeindexedname(INDEXEDNAME*);
static void vhdl_freesubtypeind(SUBTYPEIND*);
static void vhdl_freeexpression(EXPRESSION*);
static void vhdl_freerelation(RELATION*);
static void vhdl_freebasicdeclare(BASICDECLARE*);
static void vhdl_freesetofstatements(STATEMENTS*);

/*
Module:  vhdl_parser
------------------------------------------------------------------------
Description:
	Parse the passed token list using the parse tables, report on any
	syntax errors and create the required syntax trees.
------------------------------------------------------------------------
Calling Sequence:  err = vhdl_parser(tlist);

Name		Type		Description
----		----		-----------
tlist		*TOKENLIST	List of tokens.
------------------------------------------------------------------------
*/
BOOLEAN vhdl_parser(TOKENLIST *tlist)
{
	INTBIG type;
	CHAR *pointer;
	PTREE *newunit, *endunit;

	/* start by clearing former parse information */
	vhdl_freeparsermemory();

	vhdl_err = FALSE;
	endunit = NULL;
	vhdl_nexttoken = tlist;
	if (setjmp(vhdl_notoken_err))
	{
		return(vhdl_err);
	}
	while (vhdl_nexttoken != NOTOKENLIST)
	{
		if (vhdl_nexttoken->token == TOKEN_KEYWORD)
		{
			type = NOUNIT;
			switch (((VKEYWORD *)(vhdl_nexttoken->pointer))->num)
			{
				case KEY_LIBRARY:
					vhdl_parsetosemicolon();
					break;
				case KEY_ENTITY:
					type = UNIT_INTERFACE;
					pointer = (CHAR *)vhdl_parseinterface();
					break;
#ifdef VHDL50
				case KEY_ARCHITECTURAL:
#else
				case KEY_ARCHITECTURE:
#endif
					type = UNIT_BODY;
					pointer = (CHAR *)vhdl_parsebody((INTBIG)BODY_ARCHITECTURAL);
					break;
				case KEY_PACKAGE:
					type = UNIT_PACKAGE;
					pointer = (CHAR *)vhdl_parsepackage();
					break;
#ifdef VHDL50
				case KEY_WITH:
					type = UNIT_WITH;
					pointer = (CHAR *)vhdl_parsewith();
					break;
#endif
				case KEY_USE:
					type = UNIT_USE;
					pointer = (CHAR *)vhdl_parseuse();
					break;
				default:
					vhdl_reporterrormsg(vhdl_nexttoken, _("No entry keyword - entity, architectural, behavioral"));
					vhdl_nexttoken = vhdl_nexttoken->next;
					break;
			}
			if (type != NOUNIT)
			{
				newunit = (PTREE *)emalloc((INTBIG)sizeof(PTREE), vhdl_tool->cluster);
				newunit->type = type;
				newunit->pointer = pointer;
				newunit->next = NULL;
				if (endunit == NULL)
				{
					vhdl_ptree = endunit = newunit;
				} else
				{
					endunit->next = newunit;
					endunit = newunit;
				}
			}
		} else
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("No entry keyword - entity, architectural, behavioral"));
			vhdl_nexttoken = vhdl_nexttoken->next;
		}
	}
	return(vhdl_err);
}

void vhdl_reporterrormsg(TOKENLIST *tlist, CHAR *err_msg)
{
	TOKENLIST *tstart;
	CHAR buffer[MAXVHDLLINE], stemp[MAXVHDLLINE], *sptr;
	INTBIG pointer, i;

	vhdl_err = TRUE;
	vhdl_errorcount++;
	if (vhdl_errorcount == 30)
		ttyputmsg(_("TOO MANY ERRORS...PRINTING NO MORE"));
	if (vhdl_errorcount >= 30) return;
	if (tlist == NULL)
	{
		ttyputmsg(_("ERROR %s"), err_msg);
		return;
	}
	ttyputmsg(_("ERROR on line %ld, %s:"), tlist->line_num, err_msg);

	/* back up to start of line */
	for (tstart = tlist; tstart->last != NOTOKENLIST; tstart = tstart->last)
	{
		if (tstart->last->line_num != tlist->line_num) break;
	}

	/* form line in buffer */
	*buffer = 0;
	for ( ; tstart != NOTOKENLIST && tstart->line_num == tlist->line_num;
		tstart = tstart->next)
	{
		i = estrlen(buffer);
		if (i > MAXVHDLLINE/2) break;
		if (tstart == tlist) pointer = i;
		if (tstart->token < TOKEN_ARROW)
		{
			esnprintf(stemp, MAXVHDLLINE, x_("%c"), vhdl_delimiterstr[tstart->token]);
			estrcat(buffer, stemp);
		} else if (tstart->token < TOKEN_UNKNOWN)
		{
			sptr = &vhdl_doubledelimiterstr[2 * (tstart->token - TOKEN_ARROW)];
			esnprintf(stemp, MAXVHDLLINE, x_("%c%c"), sptr[0], sptr[1]);
			estrcat(buffer, stemp);
		} else switch (tstart->token)
		{
			case TOKEN_STRING:
				esnprintf(stemp, MAXVHDLLINE, x_("\"%s\" "), tstart->pointer);
				estrcat(buffer, stemp);
				break;
			case TOKEN_KEYWORD:
				esnprintf(stemp, MAXVHDLLINE, x_("%s"), ((VKEYWORD *)(tstart->pointer))->name);
				estrcat(buffer, stemp);
				break;
			case TOKEN_IDENTIFIER:
				esnprintf(stemp, MAXVHDLLINE, x_("%s"),((IDENTTABLE *)(tstart->pointer))->string);
				estrcat(buffer, stemp);
				break;
			case TOKEN_CHAR:
				esnprintf(stemp, MAXVHDLLINE, x_("%c"), (CHAR)((INTBIG)tstart->pointer));
				estrcat(buffer, stemp);
			case TOKEN_DECIMAL:
				esnprintf(stemp, MAXVHDLLINE, x_("%s"), tstart->pointer);
				estrcat(buffer, stemp);
				break;
			default:
				if (tstart->pointer != 0)
				{
					esnprintf(stemp, MAXVHDLLINE, x_("%ld"), (INTBIG)tstart->pointer);
					estrcat(buffer, stemp);
				}
				break;
		}
		if (tstart->space) estrcat(buffer, x_(" "));
	}

	/* print out line */
	ttyputmsg(x_("%s"), buffer);

	/* print out pointer */
	for (i = 0; i < pointer; i++) buffer[i] = ' ';
	buffer[pointer] = 0;
	ttyputmsg(x_("%s^"), buffer);
}

/*
Module:  vhdl_parseinterface
------------------------------------------------------------------------
Description:
	Parse an interface description of the form:

	ENTITY identifier IS PORT (formal_port_list);
	END [identifier] ;
------------------------------------------------------------------------
*/
VINTERFACE  *vhdl_parseinterface(void)
{
	TOKENLIST *name;
	VINTERFACE *interfacef;
	FPORTLIST *ports;

	vhdl_getnexttoken();

	/* check for entity IDENTIFIER */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
	} else
	{
		name = vhdl_nexttoken;
	}

#ifndef VHDL50

	/* check for keyword IS */
	vhdl_getnexttoken();
	if (!vhdl_keysame(vhdl_nexttoken, KEY_IS))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IS"));
	}

	/* check for keyword PORT */
	vhdl_getnexttoken();
	if (!vhdl_keysame(vhdl_nexttoken, KEY_PORT))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword PORT"));
	}

#endif

	/* check for opening bracket of FORMAL_PORT_LIST */
	vhdl_getnexttoken();
	if (vhdl_nexttoken->token != TOKEN_LEFTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a left bracket"));
	}

	/* gather FORMAL_PORT_LIST */
	vhdl_getnexttoken();
	if ((ports = vhdl_parseformal_port_list()) == (FPORTLIST *)PARSE_ERR)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Interface must have ports"));
	}

	/* check for closing bracket of FORMAL_PORT_LIST */
	if (vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a right bracket"));
	}

	vhdl_getnexttoken();
#ifdef VHDL50
	/* check for keyword IS */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_IS))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IS"));
	}
#else
	/* check for SEMICOLON */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
#endif
	else vhdl_getnexttoken();

	/* check for keyword END */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword END"));
	}

	/* check for optional entity IDENTIFIER */
	vhdl_getnexttoken();
	if (vhdl_nexttoken->token == TOKEN_IDENTIFIER)
	{
		if (vhdl_nexttoken->pointer != name->pointer)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Unmatched entity identifier names"));
		}
		vhdl_getnexttoken();
	}

	/* check for closing SEMICOLON */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_nexttoken = vhdl_nexttoken->next;

	/* allocate an entity parse tree */
	interfacef = (VINTERFACE *)emalloc((INTBIG)sizeof(VINTERFACE), vhdl_tool->cluster);
	interfacef->name = name;
	interfacef->ports = ports;
	interfacef->interfacef = NULL;
	return(interfacef);
}

/*
Module:  vhdl_parsebody
------------------------------------------------------------------------
Description:
	Parse a body.  The syntax is of the form:

	ARCHITECTURE identifier OF simple_name IS
		body_declaration_part
	BEGIN
		set_of_statements
	END [identifier] ;
------------------------------------------------------------------------
Calling Sequence:  body = vhdl_parsebody(vclass);

Name		Type		Description
----		----		-----------
vclass		INTBIG		Body class (ARCHITECTURAL or BEHAVIORAL).
body		*BODY		Pointer to created body structure.
------------------------------------------------------------------------
*/
BODY  *vhdl_parsebody(INTBIG vclass)
{
	TOKENLIST *body_name;
	SIMPLENAME *entity_name;
	BODYDECLARE *body_declare;
	STATEMENTS *statements;
	BODY *body;

	vhdl_getnexttoken();

#ifdef VHDL50
	/* first should be keyword BODY */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_BODY))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword BODY"));
	}
	vhdl_getnexttoken();
#endif

	/* next is body_name (identifier) */
	body_name = NULL;
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
	} else
	{
		body_name = vhdl_nexttoken;
	}
	vhdl_getnexttoken();

	/* check for keyword OF */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_OF))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword OF"));
	}
	vhdl_getnexttoken();

	/* next is design entity reference for this body (simple_name) */
	if ((entity_name = vhdl_parsesimplename()) == (SIMPLENAME *)PARSE_ERR)
	{
		/* EMPTY */ 
	}

	/* check for keyword IS */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_IS))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IS"));
	}
	vhdl_getnexttoken();

	/* body declaration part */
	if ((body_declare = vhdl_parsebody_declare()) == (BODYDECLARE *)PARSE_ERR)
	{
		/* EMPTY */ 
	}

	/* should be at keyword BEGIN */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_BEGIN))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword BEGIN"));
	}
	vhdl_getnexttoken();

	/* statements of body */
	if ((statements = vhdl_parseset_of_statements()) == (STATEMENTS *)PARSE_ERR)
	{
		/* EMPTY */ 
	}

	/* should be at keyword END */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword END"));
	}
	vhdl_getnexttoken();

	/* optional body name */
	if (vhdl_nexttoken->token == TOKEN_IDENTIFIER)
	{
		if (vhdl_nexttoken->pointer != body_name->pointer)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Body name mismatch"));
		}
		vhdl_getnexttoken();
	}

	/* should be at final semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_nexttoken = vhdl_nexttoken->next;

	/* create body parse tree */
	body = (BODY *)emalloc((INTBIG)sizeof(BODY), vhdl_tool->cluster);
	body->classnew = vclass;
	body->name = body_name;
	body->entity = entity_name;
	body->body_declare = body_declare;
	body->statements = statements;
	return(body);
}

/*
Module:  vhdl_parsebody_declare
------------------------------------------------------------------------
Description:
	Parse the body declaration and return pointer to the parse tree.
	Return PARSE_ERR if parsing error encountered.

		body_declaration_part :== {body_declaration_item}

		body_delaration_item :==
			  basic_declaration
			| component_declaration
			| resolution_mechanism_declaration
			| local_function_declaration
------------------------------------------------------------------------
Calling Sequence:  body_declare = vhdl_parsebody_declare();

Name			Type			Description
----			----			-----------
body_declare	*BODYDECLARE	Pointer to parse tree if successful,
								PARSE_ERR if parsing error encountered.
------------------------------------------------------------------------
*/
BODYDECLARE	*vhdl_parsebody_declare(void)
{
	BODYDECLARE *body, *endbody, *newbody;
	INTBIG type;
	CHAR *pointer;

	body = endbody = NULL;
	while (!vhdl_keysame(vhdl_nexttoken, KEY_BEGIN))
	{
		/* check for component declaration */
		if (vhdl_keysame(vhdl_nexttoken, KEY_COMPONENT))
		{
			type = BODYDECLARE_COMPONENT;
			if ((pointer = (CHAR *)vhdl_parsecomponent()) == (CHAR *)PARSE_ERR)
			{
				/* EMPTY */ 
			}
		}
		/* check for resolution declaration */
		else if (vhdl_keysame(vhdl_nexttoken, KEY_RESOLVE))
		{
			type = BODYDECLARE_RESOLUTION;
			pointer = NULL;
			vhdl_getnexttoken();
		}
		/* check for local function declaration */
		else if (vhdl_keysame(vhdl_nexttoken, KEY_FUNCTION))
		{
			type = BODYDECLARE_LOCAL;
			pointer = NULL;
			vhdl_getnexttoken();
		}
		/* should be basic declaration */
		else
		{
			type = BODYDECLARE_BASIC;
			if ((pointer = (CHAR *)vhdl_parsebasic_declare()) == (CHAR *)PARSE_ERR)
			{
				/* EMPTY */ 
			}
		}
		newbody = (BODYDECLARE *)emalloc((INTBIG)sizeof(BODYDECLARE), vhdl_tool->cluster);
		newbody->type = type;
		newbody->pointer = pointer;
		newbody->next = NULL;
		if (endbody == NULL)
		{
			body = endbody = newbody;
		} else
		{
			endbody->next = newbody;
			endbody = newbody;
		}
	}
	return(body);
}

/*
Module:  vhdl_parsecomponent
------------------------------------------------------------------------
Description:
	Parse a component declaration and return a pointer to the parse
	tree.  Return NULL if a terminal error occurs.  The format of a
	component declaration is:

		component_declaration :==
	  COMPONENT identifier PORT (local_port_list);
	  END COMPONENT ;

	Note:  Treat local_port_list as a formal_port_list.
------------------------------------------------------------------------
Calling Sequence:  component = vhdl_parsecomponent();

Name		Type		Description
----		----		-----------
component	*VCOMPONENT	Pointer to a component declaration,
								PARSE_ERR if parsing error encountered.
------------------------------------------------------------------------
*/
VCOMPONENT  *vhdl_parsecomponent(void)
{
	FPORTLIST *ports;
	VCOMPONENT *compo;
	TOKENLIST *entity;

	compo =  NULL;
	vhdl_getnexttoken();

	/* should be component identifier */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		entity = NULL;
	} else
	{
		entity = vhdl_nexttoken;
	}
	vhdl_getnexttoken();

#ifndef VHDL50
	/* Need keyword PORT */
	if (!vhdl_keysame(vhdl_nexttoken,KEY_PORT))
	   vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword PORT"));
	else vhdl_getnexttoken();
#endif

	/* should be left bracket, start of port list */
	if (vhdl_nexttoken->token != TOKEN_LEFTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a left bracket"));
	}
	vhdl_getnexttoken();

	/* go through port list */
	if ((ports = vhdl_parseformal_port_list()) == (FPORTLIST *)PARSE_ERR)
	{
		/* EMPTY */ 
	}

	/* should be pointing to RIGHTBRACKET */
	if (vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a right bracket"));
	}
	vhdl_getnexttoken();

#ifndef VHDL50
	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	/* Need "END COMPONENT" */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	   vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword END"));
	vhdl_getnexttoken();

	if (!vhdl_keysame(vhdl_nexttoken, KEY_COMPONENT))
	   vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword COMPONENT"));
	vhdl_getnexttoken();
#endif

	/* should be at terminating semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();
	compo = (VCOMPONENT *)emalloc((INTBIG)sizeof(VCOMPONENT), vhdl_tool->cluster);
	compo->name = entity;
	compo->ports = ports;
	return(compo);
}

/*
Module:  vhdl_parsebasic_declare
------------------------------------------------------------------------
Description:
	Parse a basic declaration and return a pointer to the parse tree.
	The form of a basic declaration is:

		basic_declaration :==
			  object_declaration
			| type_declaration
			| subtype_declaration
			| conversion_declaration
			| attribute_declaration
			| attribute_specification
------------------------------------------------------------------------
Calling Sequence:  basic = vhdl_parsebasic_declare();

Name		Type			Description
----		----			-----------
basic		*BASICDECLARE	Pointer to basic_declaration parse tree,
								PARSE_ERR if unrecoverable parsing error.
------------------------------------------------------------------------
*/
BASICDECLARE  *vhdl_parsebasic_declare(void)
{
	BASICDECLARE *basic;
	INTBIG type;
	CHAR *pointer;

	basic = NULL;
	type = NOBASICDECLARE;
	if (vhdl_keysame(vhdl_nexttoken, KEY_TYPE))
	{
		type = BASICDECLARE_TYPE;
		pointer = (CHAR *)vhdl_parsetype();
	}
	else if (vhdl_keysame(vhdl_nexttoken, KEY_SUBTYPE))
	{
		/* EMPTY */ 
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_CONVERT))
	{
		/* EMPTY */ 
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_ATTRIBUTE))
	{
		/* EMPTY */ 
	} else if (vhdl_nexttoken->token == TOKEN_IDENTIFIER)
	{
		/* EMPTY */ 
	} else
	{
		type = BASICDECLARE_OBJECT;
		pointer = (CHAR *)vhdl_parseobject_declare();
	}
	if (type != NOBASICDECLARE)
	{
		basic = (BASICDECLARE *)emalloc((INTBIG)sizeof(BASICDECLARE), vhdl_tool->cluster);
		basic->type = type;
		basic->pointer = pointer;
	} else vhdl_getnexttoken();	/* Bug fix , D.J.Yurach, June, 1988 */
	return(basic);
}

/*
Module:  vhdl_parseobject_declare
------------------------------------------------------------------------
Description:
	Parse an object declaration and return the pointer to its parse tree.
	An object declaration has the form:

		object_declaration :==
			  constant_declaration
			| signal_declaration
			| variable_declaration
			| alias_declaration
------------------------------------------------------------------------
Calling Sequence:  object = vhdl_parseobject_declaration();

Name		Type			Description
----		----			-----------
object		*OBJECTDECLARE	Pointer to object declaration parse tree.
------------------------------------------------------------------------
*/
OBJECTDECLARE  *vhdl_parseobject_declare(void)
{
	OBJECTDECLARE *object;
	INTBIG type;
	CHAR *pointer;

	object = NULL;
	type = NOOBJECTDECLARE;
	if (vhdl_keysame(vhdl_nexttoken, KEY_CONSTANT))
	{
		type = OBJECTDECLARE_CONSTANT;
		pointer = (CHAR *)vhdl_parseconstant_declare();
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_SIGNAL))
	{
		type = OBJECTDECLARE_SIGNAL;
		pointer = (CHAR *)vhdl_parsesignal_declare();
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_VARIABLE))
	{
		/* EMPTY */ 
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_ALIAS))
	{
		/* EMPTY */ 
	} else
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Invalid object declaration"));
	}
	if (type != NOOBJECTDECLARE)
	{
		object = (OBJECTDECLARE *)emalloc((INTBIG)sizeof(OBJECTDECLARE), vhdl_tool->cluster);
		object->type = type;
		object->pointer = pointer;
	} else
	{
		vhdl_getnexttoken();
	}
	return(object);
}

/*
Module:  vhdl_parsesignal_declare
------------------------------------------------------------------------
Description:
	Parse a signal declaration and return the pointer to the parse tree.
	The form of a signal declaration is:

		signal_declaration :==
			SIGNAL identifier_list : subtype_indication;
------------------------------------------------------------------------
Calling Sequence:  signal = vhdl_parsesignal_declare();

Name		Type			Description
----		----			-----------
signal		*SIGNALDECLARE	Pointer to signal declaration parse tree.
------------------------------------------------------------------------
*/
SIGNALDECLARE  *vhdl_parsesignal_declare(void)
{
	SIGNALDECLARE *signal;
	IDENTLIST *signal_list;
	SUBTYPEIND *ind;

	vhdl_getnexttoken();

	/* parse identifier list */
	signal_list = vhdl_parseident_list();

	/* should be at colon */
	if (vhdl_nexttoken->token != TOKEN_COLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a colon"));
	}
	vhdl_getnexttoken();

	/* parse subtype indication */
	ind = vhdl_parsesubtype_indication();

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	signal = (SIGNALDECLARE *)emalloc((INTBIG)sizeof(SIGNALDECLARE), vhdl_tool->cluster);
	signal->names = signal_list;
	signal->subtype = ind;
	return(signal);
}

/*
Module:  vhdl_parseconstant_declare
------------------------------------------------------------------------
Description:
	Parse a constant declaration and return the pointer to the parse tree.
	The form of a constant declaration is:

		constant_declaration :==
			CONSTANT identifier : subtype_indication := expression ;
------------------------------------------------------------------------
Calling Sequence:  constant = vhdl_parseconstant_declare();

Name		Type				Description
----		----				-----------
constant	*CONSTANTDECLARE	Pointer to constant declaration parse tree.
------------------------------------------------------------------------
*/
CONSTANTDECLARE  *vhdl_parseconstant_declare(void)
{
	CONSTANTDECLARE *constant;
	SUBTYPEIND *ind;
	TOKENLIST *ident;
	EXPRESSION *expr;

	constant = NULL;
	vhdl_getnexttoken();

	/* parse identifier  */
	/* Note that the standard allows identifier_list here,
	 * but we don't support it!
	 */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		ident = NULL;
	} else
	{
		ident = vhdl_nexttoken;
	}
	vhdl_getnexttoken();

	/* should be at colon */
	if (vhdl_nexttoken->token != TOKEN_COLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a colon"));
	}
	vhdl_getnexttoken();

	/* parse subtype indication */
	ind = vhdl_parsesubtype_indication();

	/* should be at assignment symbol */
	if (vhdl_nexttoken->token != TOKEN_VARASSIGN)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting variable assignment symbol"));
	}
	vhdl_getnexttoken();

	/* should be at expression */
	expr = vhdl_parseexpression();

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	constant = (CONSTANTDECLARE *)emalloc((INTBIG)sizeof(CONSTANTDECLARE), vhdl_tool->cluster);
	constant->identifier = ident;
	constant->subtype = ind;
	constant->expression = expr;

	return(constant);
}

/*******************************************************************
Module:  vhdl_parseident_list
--------------------------------------------------------------------
Description:
	Parse an identifier list and return its parse tree.  The form
	of an identifier list is:

		identifier_list :==
			identifier {, identifier}
--------------------------------------------------------------------
Calling Sequence:  ilist = vhdl_parseident_list();

Name		Type		Description
----		----		-----------
ilist		*IDENTLIST	Pointer to identifier list.
--------------------------------------------------------------------
*/
IDENTLIST  *vhdl_parseident_list(void)
{
	IDENTLIST *ilist, *ilistend, *newilist;

	/* must be at least one identifier */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		vhdl_getnexttoken();
		return(NULL);
	}
	newilist = (IDENTLIST *)emalloc((INTBIG)sizeof(IDENTLIST), vhdl_tool->cluster);
	newilist->identifier = vhdl_nexttoken;
	newilist->next = NULL;
	ilist = ilistend = newilist;

	/* continue while a comma is next */
	vhdl_getnexttoken();
	while (vhdl_nexttoken->token == TOKEN_COMMA)
	{
		vhdl_getnexttoken();
		/* should be another identifier */
		if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
			vhdl_getnexttoken();
			return(NULL);
		}
		newilist = (IDENTLIST *)emalloc((INTBIG)sizeof(IDENTLIST), vhdl_tool->cluster);
		newilist->identifier = vhdl_nexttoken;
		newilist->next = NULL;
		ilistend->next = newilist;
		ilistend = newilist;
		vhdl_getnexttoken();
	}
	return(ilist);
}

/*
Module:  vhdl_parsesubtype_indication
------------------------------------------------------------------------
Description:
	Parse a subtype indication of the form:

		subtype_indication :==
			type_mark [constraint]
------------------------------------------------------------------------
Calling Sequence:  ind = vhdl_parsesubtype_indication();

Name		Type		Description
----		----		-----------
ind			*SUBTYPEIND	Pointer to subtype indication parse tree.
------------------------------------------------------------------------
*/
SUBTYPEIND  *vhdl_parsesubtype_indication(void)
{
	SUBTYPEIND *ind;
	VNAME *type;

	type = vhdl_parsename();
	ind = (SUBTYPEIND *)emalloc((INTBIG)sizeof(SUBTYPEIND), vhdl_tool->cluster);
	ind->type = type;
	ind->constraint = NULL;
	return(ind);
}

/*
Module:  vhdl_parseset_of_statements
------------------------------------------------------------------------
Description:
	Parse the body statements and return pointer to the parse tree.
	The form of body statements are:

		set_of_statements :==
			architectural_statement {architectural_statement}

		architectural_statement :==
			  generate_statement
			| signal_assignment_statement
			| architectural_if_statement
			| architectural_case_statement
			| component_instantiation_statement
			| null_statement
------------------------------------------------------------------------
Calling Sequence:  statements = vhdl_parseset_of_statements();

Name		Type		Description
----		----		-----------
statements	*STATEMENTS	Pointer to statements parse tree.
------------------------------------------------------------------------
*/
STATEMENTS	*vhdl_parseset_of_statements(void)
{
	STATEMENTS *statements, *endstate, *newstate;
	INTBIG type;
	CHAR *pointer;
	TOKENLIST *label;

	statements = endstate = NULL;
	while (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		type = NOARCHSTATE;

#ifdef VHDL50
		/* Note that IEEE requires label before IF or GENERATE */
		/* check for if statement */
		if (vhdl_keysame(vhdl_nexttoken, KEY_IF))
		{
			/* could be architectural if or if generate scheme */
			type = ARCHSTATE_GENERATE;
			pointer = (CHAR *)vhdl_parsegenerate(NULL, (INTBIG)GENSCHEME_IF);
		}

		/* check for generate statement */
		else if (vhdl_keysame(vhdl_nexttoken, KEY_FOR))
		{
			type = ARCHSTATE_GENERATE;
			pointer = (CHAR *)vhdl_parsegenerate(NULL, (INTBIG)GENSCHEME_FOR);
		}

		else
#endif
		/* check for case statement */
		if (vhdl_keysame(vhdl_nexttoken, KEY_CASE))
		{
			/* EMPTY */ 
		}

		/* check for null statement */
		else if (vhdl_keysame(vhdl_nexttoken, KEY_NULL))
		{
			type = ARCHSTATE_NULL;
			pointer = NULL;
			vhdl_getnexttoken();
			/* should be a semicolon */
			if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
			{
				vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
			}
			vhdl_getnexttoken();
		}

		/* check for label */
		else if (vhdl_nexttoken->token == TOKEN_IDENTIFIER && vhdl_nexttoken->next &&
			vhdl_nexttoken->next->token == TOKEN_COLON)
		{
			label = vhdl_nexttoken;
			vhdl_getnexttoken();
			vhdl_getnexttoken();
			/* check for generate statement */
			if (vhdl_keysame(vhdl_nexttoken, KEY_IF))
			{
				type = ARCHSTATE_GENERATE;
				pointer = (CHAR *)vhdl_parsegenerate(label, (INTBIG)GENSCHEME_IF);
			}
			else if (vhdl_keysame(vhdl_nexttoken, KEY_FOR))
			{
				type = ARCHSTATE_GENERATE;
				pointer = (CHAR *)vhdl_parsegenerate(label, (INTBIG)GENSCHEME_FOR);
			}
			/* should be component_instantiation_declaration */
			else
			{
				vhdl_nexttoken = label;
				type = ARCHSTATE_INSTANCE;
				pointer = (CHAR *)vhdl_parseinstance();
			}
		}

		/* IEEE requires label on component_instantiation */
#ifdef VHDL50
		/* check for component_instantiation without a label */
		else if (vhdl_nexttoken->token == TOKEN_IDENTIFIER && vhdl_nexttoken->next &&
			vhdl_nexttoken->next->token == TOKEN_LEFTBRACKET)
		{
			type = ARCHSTATE_INSTANCE;
			pointer = (CHAR *)vhdl_parseinstance();
		}
#endif

		/* should have signal assignment */
		else
		{
			/* EMPTY */ 
		}

		/* add statement if found */
		if (type != NOARCHSTATE)
		{
			newstate = (STATEMENTS *)emalloc((INTBIG)sizeof(STATEMENTS), vhdl_tool->cluster);
			newstate->type = type;
			newstate->pointer = pointer;
			newstate->next = NULL;
			if (endstate == NULL)
			{
				statements = endstate = newstate;
			} else
			{
				endstate->next = newstate;
				endstate = newstate;
			}
		} else
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Invalid ARCHITECTURAL statement"));
			vhdl_nexttoken = vhdl_nexttoken->next;
			break;
		}
	}

	return(statements);
}

/*
Module:  vhdl_parsegenerate
------------------------------------------------------------------------
Description:
	Parse a generate statement of the form:

		generate_statement ::=
			label:
				generate_scheme GENERATE
					set_of_statements
				END GENERATE [label];

		generate_scheme ::=
			  FOR generate_parameter_specification
			| IF condition

		generate_parameter_specification ::=
			identifier IN discrete_range
------------------------------------------------------------------------
Calling Sequence:  gen = vhdl_parsegenerate_for(label, gscheme);

Name		Type		Description
----		----		-----------
label		*TOKENLIST	Pointer to optional label.
gscheme		INTBIG		Generate scheme (FOR or IF).
gen			*GENERATE	Returned generate statement structure.
------------------------------------------------------------------------
*/
GENERATE  *vhdl_parsegenerate(TOKENLIST *label, INTBIG gscheme)
{
	GENERATE *gen;
	GENSCHEME *scheme;
	STATEMENTS *states;

	gen = NULL;

	if (gscheme == GENSCHEME_FOR)
	{
		/* should be past label and at keyword FOR */
		if (!vhdl_keysame(vhdl_nexttoken, KEY_FOR))
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword FOR"));
		}
	} else
	{
		/* should be past label and at keyword IF */
		if (!vhdl_keysame(vhdl_nexttoken, KEY_IF))
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IF"));
		}
	}
	scheme = (GENSCHEME *)emalloc((INTBIG)sizeof(GENSCHEME), vhdl_tool->cluster);
	if (gscheme == GENSCHEME_FOR)
	{
		scheme->scheme = GENSCHEME_FOR;
	} else
	{
		scheme->scheme = GENSCHEME_IF;
	}
	scheme->identifier = NULL;
	scheme->range = NULL;
	scheme->condition = NULL;		/* for IF scheme only */
	vhdl_getnexttoken();

	if (gscheme == GENSCHEME_FOR)
	{
		/* should be generate parameter specification */
		if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		} else
		{
			scheme->identifier = vhdl_nexttoken;
		}
		vhdl_getnexttoken();

		/* should be keyword IN */
		if (!vhdl_keysame(vhdl_nexttoken, KEY_IN))
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IN"));
		}
		vhdl_getnexttoken();

		/* should be discrete range */
		scheme->range = vhdl_parsediscrete_range();
	} else
	{
		scheme->condition = vhdl_parseexpression();
	}

	/* should be keyword GENERATE */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_GENERATE))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword GENERATE"));
	}
	vhdl_getnexttoken();

	/* set of statements */
	states = vhdl_parseset_of_statements();

	/* should be at keyword END */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword END"));
	}
	vhdl_getnexttoken();

	/* should be at keyword GENERATE */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_GENERATE))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword GENERATE"));
	}
	vhdl_getnexttoken();

	/* check if label should be present */
	if (label)
	{
#ifndef VHDL50
		/* For correct IEEE syntax, label is always true, but trailing
		 * label is optional.
		 */
		if (vhdl_nexttoken->token == TOKEN_IDENTIFIER)
		{
			if (label->pointer != vhdl_nexttoken->pointer)
				vhdl_reporterrormsg(vhdl_nexttoken, _("Label mismatch"));
			vhdl_getnexttoken();
		}
#else
		if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		else
			if (label->pointer != vhdl_nexttoken->pointer)
				vhdl_reporterrormsg(vhdl_nexttoken, _("Label mismatch"));
		vhdl_getnexttoken();
#endif
	}

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	/* create generate statement structure */
	gen = (GENERATE *)emalloc((INTBIG)sizeof(GENERATE), vhdl_tool->cluster);
	gen->label = label;
	gen->gen_scheme = scheme;
	gen->statements = states;
	return(gen);
}

/*
Module:  vhdl_parsediscrete_range
------------------------------------------------------------------------
Description:
	Parse a discrete range of the form:

		discrete_range ::= subtype_indication | range
------------------------------------------------------------------------
Calling Sequence:  range = vhdl_parsediscrete_range();

Name		Type			Description
----		----			-----------
range		*DISCRETERANGE	Returned discrete range structure.
------------------------------------------------------------------------
*/
DISCRETERANGE  *vhdl_parsediscrete_range(void)
{
	DISCRETERANGE *drange;

	drange = (DISCRETERANGE *)emalloc((INTBIG)sizeof(DISCRETERANGE), vhdl_tool->cluster);
	/* currently only support ranege option */
	drange->type = DISCRETERANGE_RANGE;
	drange->pointer = (CHAR *)vhdl_parserange();
	return(drange);
}

/*
Module:  vhdl_parserange
------------------------------------------------------------------------
Description:
	Parse a range of the form:

		range :==
	  simple_expression direction simple_expression

	direction ::=  TO  |  DOWNTO

------------------------------------------------------------------------
Calling Sequence:  range = vhdl_parserange();

Name		Type		Description
----		----		-----------
range		*RANGE		Returned range structure.
------------------------------------------------------------------------
*/
RANGE  *vhdl_parserange(void)
{
	RANGE *range;

	range = (RANGE *)emalloc((INTBIG)sizeof(RANGE), vhdl_tool->cluster);
	/* currently support only simple expression range option */
	range->type = RANGE_SIMPLE_EXPR;
	range->pointer = (CHAR *)vhdl_parserange_simple();
	return(range);
}

/*
Module:  vhdl_parserange_simple
------------------------------------------------------------------------
Description:
	Parse a simple expression range of the form:

		simple_expression .. simple_expression
------------------------------------------------------------------------
Calling Sequence:  srange = vhdl_parserange_simple();

Name		Type			Description
----		----			-----------
srange		*RANGESIMPLE	Returned simple expression range.
------------------------------------------------------------------------
*/
RANGESIMPLE  *vhdl_parserange_simple(void)
{
	RANGESIMPLE *srange;

	srange = (RANGESIMPLE *)emalloc((INTBIG)sizeof(RANGESIMPLE), vhdl_tool->cluster);
	srange->start = vhdl_parsesimpleexpression();

#ifdef VHDL50
	/* should be at double dot */
	if (vhdl_nexttoken->token != TOKEN_DOUBLEDOT)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting double dot (..)"));
	}
	vhdl_getnexttoken();
#else
	/* Need keyword TO or DOWNTO */
	if (vhdl_keysame(vhdl_nexttoken, KEY_TO) || vhdl_keysame(vhdl_nexttoken, KEY_DOWNTO))
	   vhdl_getnexttoken();
	else
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword TO or DOWNTO"));
		vhdl_getnexttoken(); /* absorb the token anyway (probably "..") */
	}
#endif

	srange->end = vhdl_parsesimpleexpression();
	return(srange);
}

/*
Module:  vhdl_parseinstance
------------------------------------------------------------------------
Description:
	Parse a component instantiation statement of the form:

		component_instantiation_statement :==
				label : simple_name PORT MAP(actual_port_list);
------------------------------------------------------------------------
Calling Sequence:  inst = vhdl_parseinstance();

Name		Type		Description
----		----		-----------
inst		*VINSTANCE	Pointer to instance parse tree.
------------------------------------------------------------------------
*/
VINSTANCE  *vhdl_parseinstance(void)
{
	VINSTANCE *inst;
	TOKENLIST *name;
	SIMPLENAME *entity;
	APORTLIST *ports;

	inst = NULL;

	/* check for identifier */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		vhdl_getnexttoken();
		return(inst);
	}
	name = vhdl_nexttoken;
	vhdl_getnexttoken();

	/* if colon, previous token was the label */
	if (vhdl_nexttoken->token == TOKEN_COLON)
	{
		vhdl_getnexttoken();
	} else
	{
		vhdl_nexttoken = name;
		name = NULL;
	}

	/* should be at component reference */
	entity = vhdl_parsesimplename();

#ifndef VHDL50
	/* Require PORT MAP */
	if (vhdl_keysame(vhdl_nexttoken, KEY_PORT))
	   vhdl_getnexttoken();
	else vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword PORT"));

	if (vhdl_keysame(vhdl_nexttoken, KEY_MAP))
	   vhdl_getnexttoken();
	else vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword MAP"));
#endif

	/* should be at left bracket */
	if (vhdl_nexttoken->token != TOKEN_LEFTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a left bracket"));
	}
	vhdl_getnexttoken();
	ports = vhdl_parseactual_port_list();

	/* should be at right bracket */
	if (vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a right bracket"));
	}
	vhdl_getnexttoken();

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	inst = (VINSTANCE *)emalloc((INTBIG)sizeof(VINSTANCE), vhdl_tool->cluster);
	inst->name = name;
	inst->entity = entity;
	inst->ports = ports;
	return(inst);
}

/*
Module:  vhdl_parseformal_port_list
------------------------------------------------------------------------
Description:
	Parse a formal port list.  A formal port list has the form:

		formal_port_list ::=
			port_declaration {; port_declaration}

		port_declaration ::=
			identifier_list : port_mode type_mark

		identifier_list  ::= identifier {, identifier}

		port_mode        ::= [in] | [dot] out | inout | linkage

		type_mark	 ::= name
------------------------------------------------------------------------
Calling Sequence:  port_list = vhdl_parseformal_port_list();

Name		Type		Description
----		----		-----------
port_list	*FPORTLIST	Pointer to formal port list parse tree.
------------------------------------------------------------------------
*/
FPORTLIST  *vhdl_parseformal_port_list(void)
{
	FPORTLIST *ports, *endport, *newport;
	IDENTLIST *ilist;
	INTBIG mode;
	VNAME *type;

	ports = NULL;

	/* must be at least one port declaration */
	ilist = vhdl_parseident_list();
	if (ilist == NULL) return((FPORTLIST *)PARSE_ERR);

	/* should be at colon */
	if (vhdl_nexttoken->token != TOKEN_COLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a colon"));
		return((FPORTLIST *)PARSE_ERR);
	}
	vhdl_getnexttoken();
	/* Get port mode */
	mode = vhdl_parseport_mode();
	/* should be at type_mark */
	type = vhdl_parsename();

	/* create port declaration */
	ports = (FPORTLIST *)emalloc((INTBIG)sizeof(FPORTLIST), vhdl_tool->cluster);
	if (ports == 0) return((FPORTLIST *)PARSE_ERR);
	ports->names = ilist;
	ports->mode = mode;
	ports->type = type;
	ports->next = NULL;
	endport = ports;

	while (vhdl_nexttoken->token == TOKEN_SEMICOLON)
	{
		vhdl_getnexttoken();
		ilist = vhdl_parseident_list();
		if (ilist == NULL) return((FPORTLIST *)PARSE_ERR);

		/* should be at colon */
		if (vhdl_nexttoken->token != TOKEN_COLON)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a colon"));
			return((FPORTLIST *)PARSE_ERR);
		}
		vhdl_getnexttoken();
		/* Get port mode */
		mode = vhdl_parseport_mode();
		/* should be at type_mark */
		type = vhdl_parsename();
		newport = (FPORTLIST *)emalloc((INTBIG)sizeof(FPORTLIST), vhdl_tool->cluster);
		if (newport == 0) return((FPORTLIST *)PARSE_ERR);
		newport->names = ilist;
		newport->mode = mode;
		newport->type = type;
		newport->next = NULL;
		endport->next = newport;
		endport = newport;
	}

	return(ports);
}

/*
Module:  vhdl_parseport_mode
------------------------------------------------------------------------
Description:
	Parse a port mode description of the form:

		port_mode :== [in] | [ dot ] out | inout | linkage
------------------------------------------------------------------------
Calling Sequence:  mode = vhdl_parseport_mode();

Name		Type		Description
----		----		-----------
mode		INTBIG		Type of mode (default to in).
------------------------------------------------------------------------
*/
INTBIG vhdl_parseport_mode(void)
{
	INTBIG mode;

	mode = MODE_IN;
	if (vhdl_nexttoken->token == TOKEN_KEYWORD)
	{
		switch (((VKEYWORD *)(vhdl_nexttoken->pointer))->num)
		{
			case KEY_IN:
				vhdl_getnexttoken();
				break;
#ifdef VHDL50
			case KEY_DOT:
				vhdl_getnexttoken();
				if (!vhdl_keysame(vhdl_nexttoken, KEY_OUT))
					vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword OUT")); else
						mode = MODE_DOTOUT;
				vhdl_getnexttoken();
				break;
#endif
			case KEY_OUT:
				mode = MODE_OUT;
				vhdl_getnexttoken();
				break;
			case KEY_INOUT:
				mode = MODE_INOUT;
				vhdl_getnexttoken();
				break;
			case KEY_LINKAGE:
				mode = MODE_LINKAGE;
				vhdl_getnexttoken();
				break;
			default:
				break;
		}
	}
	return(mode);
}

/*
Module:  vhdl_parseactual_port_list
------------------------------------------------------------------------
Description:
	Parse an actual port list of the form:

		actual_port_list ::=
			port_association {, port_association}

		port_association ::=
		name
		| OPEN

------------------------------------------------------------------------
Calling Sequence:  aplist = vhdl_parseactual_port_list();

Name		Type		Description
----		----		-----------
aplist		*APORTLIST	Pointer to actual port list structure.
------------------------------------------------------------------------
*/
APORTLIST  *vhdl_parseactual_port_list(void)
{
	APORTLIST *aplist, *lastport, *newport;

	aplist = lastport = NULL;

	/* should be at least one port association */
	aplist = (APORTLIST *)emalloc((INTBIG)sizeof(APORTLIST), vhdl_tool->cluster);
	aplist->type = APORTLIST_NAME;
	if (vhdl_nexttoken->token != TOKEN_COMMA &&
		vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
	{
#ifndef VHDL50
		if (vhdl_keysame(vhdl_nexttoken, KEY_OPEN))
		{
			aplist->pointer = NULL;
			vhdl_getnexttoken();
		} else
#endif
		{
			aplist->pointer = (CHAR *)vhdl_parsename();
			if (vhdl_nexttoken->token == TOKEN_ARROW)
			{
				vhdl_getnexttoken();
				aplist->pointer = (CHAR *)vhdl_parsename();
			}
		}

	}
#ifdef VHDL50
	else
	{
		aplist->pointer = NULL;
	}
#else
	else vhdl_reporterrormsg(vhdl_nexttoken, _("No identifier in port list"));
#endif

	aplist->next = NULL;
	lastport = aplist;
	while (vhdl_nexttoken->token == TOKEN_COMMA)
	{
		vhdl_getnexttoken();
		newport = (APORTLIST *)emalloc((INTBIG)sizeof(APORTLIST), vhdl_tool->cluster);
		newport->type = APORTLIST_NAME;
		if (vhdl_nexttoken->token != TOKEN_COMMA &&
			vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
		{
#ifndef VHDL50
			if (vhdl_keysame(vhdl_nexttoken, KEY_OPEN))
			{
				newport->pointer = NULL;
				vhdl_getnexttoken();
			} else
#endif
			{
				newport->pointer = (CHAR *)vhdl_parsename();
				if (vhdl_nexttoken->token == TOKEN_ARROW)
				{
					vhdl_getnexttoken();
					newport->pointer = (CHAR *)vhdl_parsename();
				}
			}
		}
#ifdef VHDL50
		else
		{
			newport->pointer = NULL;
		}
#else
		else vhdl_reporterrormsg(vhdl_nexttoken, _("No identifier in port list"));
#endif

		newport->next = NULL;
		lastport->next = newport;
		lastport = newport;
	}
	return(aplist);
}

/*
 * ignore up to the next semicolon.
 */
void vhdl_parsetosemicolon(void)
{
	for(;;)
	{
		vhdl_getnexttoken();
		if (vhdl_nexttoken->token == TOKEN_SEMICOLON)
		{
			vhdl_getnexttoken();
			break;
		}
	}
}

/*
Module:  vhdl_parsepackage
------------------------------------------------------------------------
Description:
	Parse a package declaration of the form:

		package_declaration ::=
			PACKAGE identifier IS
				package_declarative_part
			END [simple_name] ;
------------------------------------------------------------------------
Calling Sequence:  package = vhdl_parsepackage();

Name		Type		Description
----		----		-----------
package		*PACKAGE	Pointer to package declaration.
------------------------------------------------------------------------
*/
PACKAGE  *vhdl_parsepackage(void)
{
	PACKAGE *package;
	PACKAGEDPART *declare_part;
	TOKENLIST *identifier;

	package = NULL;

	/* should be at keyword package */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_PACKAGE))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword PACKAGE"));
		vhdl_getnexttoken();
		return(package);
	}
	vhdl_getnexttoken();

	/* should be package identifier */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		vhdl_getnexttoken();
		return(package);
	}
	identifier = vhdl_nexttoken;
	vhdl_getnexttoken();

	/* should be at keyword IS */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_IS))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IS"));
		vhdl_getnexttoken();
		return(package);
	}
	vhdl_getnexttoken();

	/* package declarative part */
	declare_part = vhdl_parsepackage_declare_part();

	/* should be at keyword END */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword END"));
		vhdl_getnexttoken();
		return(package);
	}
	vhdl_getnexttoken();

	/* check for optional end identifier */
	if (vhdl_nexttoken->token == TOKEN_IDENTIFIER)
	{
		if (vhdl_nexttoken->pointer != identifier->pointer)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Name mismatch"));
			vhdl_getnexttoken();
			return(package);
		}
		vhdl_getnexttoken();
	}

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
		vhdl_getnexttoken();
		return(package);
	}
	vhdl_getnexttoken();

	/* create package structure */
	package = (PACKAGE *)emalloc((INTBIG)sizeof(PACKAGE), vhdl_tool->cluster);
	package->name = identifier;
	package->declare = declare_part;

	return(package);
}

/*
Module:  vhdl_parsepackage_declare_part
------------------------------------------------------------------------
Description:
	Parse a package declarative part of the form:

		package_declarative_part ::=
			package_declarative_item {package_declarative_item}

		package_declarative_item ::=
			  basic_declaration
			| function_declaration

Note:  Currently only support basic declarations.
------------------------------------------------------------------------
Calling Sequence:  declare_part = vhdl_parsepackage_declare_part();

Name			Type			Description
----			----			-----------
declare_part	*PACKAGEDPART	Pointer to package declarative part.
------------------------------------------------------------------------
*/
PACKAGEDPART *vhdl_parsepackage_declare_part(void)
{
	PACKAGEDPART *dpart, *endpart, *newpart;
	BASICDECLARE *ditem;

	dpart = NULL;

	/* should be at least one */
	if (vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("No Package declarative part"));
		return(dpart);
	}
	ditem = vhdl_parsebasic_declare();
	dpart = (PACKAGEDPART *)emalloc((INTBIG)sizeof(PACKAGEDPART), vhdl_tool->cluster);
	dpart->item = ditem;
	dpart->next = NULL;
	endpart = dpart;

	while (!vhdl_keysame(vhdl_nexttoken, KEY_END))
	{
		ditem = vhdl_parsebasic_declare();
		newpart = (PACKAGEDPART *)emalloc((INTBIG)sizeof(PACKAGEDPART), vhdl_tool->cluster);
		newpart->item = ditem;
		newpart->next = NULL;
		endpart->next = newpart;
		endpart = newpart;
	}

	return(dpart);
}

#ifdef VHDL50
/*
Module:  vhdl_parsewith  (Note: not present in IEEE Standard)
------------------------------------------------------------------------
Description:
	Parse a with clause of the form:

		with_clause ::= WITH unit {, unit} ;

		unit ::=  simple_name | STANDARD
------------------------------------------------------------------------
Calling Sequence:  with = vhdl_parsewith();

Name		Type		Description
----		----		-----------
with		*WITH		Pointer to with clause structure
------------------------------------------------------------------------
*/
WITH *vhdl_parsewith(void)
{
	WITH *with, *endwith, *newwith;

	with = NULL;

	/* should be at keyword WITH */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_WITH))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword WITH"));
		vhdl_getnexttoken();
		return(with);
	}
	vhdl_getnexttoken();

	/* must be at least one unit */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER
		&& !vhdl_keysame(vhdl_nexttoken, KEY_STANDARD))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Bad unit name for with clause"));
		vhdl_getnexttoken();
		return(with);
	}
	with = (WITH *)emalloc((INTBIG)sizeof(WITH), vhdl_tool->cluster);
	with->unit = vhdl_nexttoken;
	with->next = NULL;
	endwith = with;
	vhdl_getnexttoken();

	while (vhdl_nexttoken->token == TOKEN_COMMA)
	{
		vhdl_getnexttoken();
		if (vhdl_nexttoken->token != TOKEN_IDENTIFIER
			&& !vhdl_keysame(vhdl_nexttoken, KEY_STANDARD))
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Bad unit name for with clause"));
			vhdl_getnexttoken();
			return(with);
		}
		newwith = (WITH *)emalloc((INTBIG)sizeof(WITH), vhdl_tool->cluster);
		newwith->unit = vhdl_nexttoken;
		newwith->next = NULL;
		endwith->next = newwith;
		endwith = newwith;
		vhdl_getnexttoken();
	}

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	return(with);
}
#endif

/*
Module:  vhdl_parseuse
------------------------------------------------------------------------
Description:
	Parse a use clause of the form:

		use_clause ::= USE unit {,unit} ;

	unit ::= package_name.ALL

------------------------------------------------------------------------
Calling Sequence:  use = vhdl_parseuse();

Name	Type		Description
----	----		-----------
use		*USE		Pointer to use clause structure
------------------------------------------------------------------------
*/
USE *vhdl_parseuse(void)
{
	USE *use, *enduse, *newuse;

	use = NULL;

	/* should be at keyword USE */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_USE))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword USE"));
		vhdl_getnexttoken();
		return(use);
	}
	vhdl_getnexttoken();

	/* must be at least one unit */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Bad unit name for use clause"));
		vhdl_getnexttoken();
		return(use);
	}
	use = (USE *)emalloc((INTBIG)sizeof(USE), vhdl_tool->cluster);
	use->unit = vhdl_nexttoken;
	use->next = NULL;
	enduse = use;
	vhdl_getnexttoken();
#ifndef VHDL50
	/* IEEE version uses form unit.ALL only */
	for(;;)
	{
		if (vhdl_nexttoken->token != TOKEN_PERIOD)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting period"));
			break;
		}
		vhdl_getnexttoken();

		if (vhdl_keysame(vhdl_nexttoken, KEY_ALL))
		{
			vhdl_getnexttoken();
			break;
		}
		if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Bad unit name for use clause"));
			break;
		}
		vhdl_getnexttoken();
	}
#endif

	while (vhdl_nexttoken->token == TOKEN_COMMA)
	{
		vhdl_getnexttoken();
		if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
		{
			vhdl_reporterrormsg(vhdl_nexttoken, _("Bad unit name for use clause"));
			vhdl_getnexttoken();
			return(use);
		}
		newuse = (USE *)emalloc((INTBIG)sizeof(USE), vhdl_tool->cluster);
		newuse->unit = vhdl_nexttoken;
		newuse->next = NULL;
		enduse->next = newuse;
		enduse = newuse;
		vhdl_getnexttoken();
#ifndef VHDL50
		/* IEEE version uses form unit.ALL only */
		if (vhdl_nexttoken->token == TOKEN_PERIOD)
			vhdl_getnexttoken();
		else vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting period"));

		if (vhdl_keysame(vhdl_nexttoken, KEY_ALL))
			vhdl_getnexttoken();
		else vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword ALL"));
#endif
	}

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
	}
	vhdl_getnexttoken();

	return(use);
}

/*
Module:  vhdl_keysame
------------------------------------------------------------------------
Description:
	Compare the two keywords, the first as part of a token.  Return
	true if the same, else return a false.
------------------------------------------------------------------------
Calling Sequence:  same = vhdl_keysame(tokenptr, key)

Name		Type		Description
----		----		-----------
tokenptr	*TOKENLIST	Pointer to the token entity.
key			INTBIG		Value of key to be compared.
same		INTBIG		!= 0 if same, = 0 if not same.
------------------------------------------------------------------------
*/
BOOLEAN vhdl_keysame(TOKENLIST *tokenptr, INTBIG key)
{
	if (tokenptr->token != TOKEN_KEYWORD)
	{
		return(FALSE);
	}
	if (((VKEYWORD *)(tokenptr->pointer))->num == key)
	{
		return(TRUE);
	}
	return(FALSE);
}

/*
Module:  vhdl_getnexttoken
------------------------------------------------------------------------
Description:
	Get the next token if possible.  Return != 0 if error (i.e. no
	tokens left.
------------------------------------------------------------------------
Calling Sequence:  err = vhdl_getnexttoken();

Name		Type		Description
----		----		-----------
err			INTBIG		== 0, next token available.
								!= 0, no more tokens available.
------------------------------------------------------------------------
*/
void vhdl_getnexttoken(void)
{
	if (vhdl_nexttoken->next == NOTOKENLIST)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Unexpected termination within block"));
		longjmp(vhdl_notoken_err, 1);
	}
	vhdl_nexttoken = vhdl_nexttoken->next;
}

/*********************************** MEMORY FREEING ***********************************/

void vhdl_freeparsermemory(void)
{
	PTREE *par, *nextpar;
	VINTERFACE *interfacef;
	BODY *body;
	BODYDECLARE *bodydec, *nextbodydec;
	VCOMPONENT *compo;
	PACKAGE *package;
	PACKAGEDPART *dpart, *nextdpart;
	USE *use, *nextuse;
#ifdef VHDL50
	WITH *with, *nextwith;
#endif

	for(par = vhdl_ptree; par != NULL; par = nextpar)
	{
		nextpar = par->next;
		switch (par->type)
		{
			case UNIT_INTERFACE:
				interfacef = (VINTERFACE *)par->pointer;
				vhdl_freeports(interfacef->ports);
				efree((CHAR *)interfacef);
				break;
			case UNIT_BODY:
				body = (BODY *)par->pointer;
				if (body->entity != NULL) efree((CHAR *)body->entity);
				for(bodydec = body->body_declare; bodydec != NULL; bodydec = nextbodydec)
				{
					nextbodydec = bodydec->next;
					switch (bodydec->type)
					{
						case BODYDECLARE_COMPONENT:
							compo = (VCOMPONENT *)bodydec->pointer;
							vhdl_freeports(compo->ports);
							efree((CHAR *)compo);
							break;
						case BODYDECLARE_BASIC:
							vhdl_freebasicdeclare((BASICDECLARE *)bodydec->pointer);
							break;
					}
					efree((CHAR *)bodydec);
				}
				vhdl_freesetofstatements(body->statements);
				efree((CHAR *)body);
				break;
			case UNIT_PACKAGE:
				package = (PACKAGE *)par->pointer;
				if (package == NULL) break;
				dpart = (PACKAGEDPART *)package->declare;
				for(dpart = (PACKAGEDPART *)package->declare; dpart != NULL; dpart = nextdpart)
				{
					nextdpart = dpart->next;
					vhdl_freebasicdeclare((BASICDECLARE *)dpart->item);
					efree((CHAR *)dpart);
				}
				efree((CHAR *)package);
				break;
#ifdef VHDL50
			case UNIT_WITH:
				for(with = (WITH *)par->pointer; with != NULL; with = nextwith)
				{
					nextwith = with->next;
					efree((CHAR *)with);
				}
				break;
#endif
			case UNIT_USE:
				for(use = (USE *)par->pointer; use != NULL; use = nextuse)
				{
					nextuse = use->next;
					efree((CHAR *)use);
				}
				break;
		}
		efree((CHAR *)par);
	}
	vhdl_ptree = NULL;
}

/* free the structure created by "vhdl_parseformal_port_list" */
void vhdl_freeports(FPORTLIST *ports)
{
	FPORTLIST *nextports;

	for( ; ports != NULL && ports != ((FPORTLIST *)-1); ports = nextports)
	{
		nextports = ports->next;
		vhdl_freeidentlist(ports->names);
		vhdl_freename(ports->type);
		efree((CHAR *)ports);
	}
}

/* free the structure created by "vhdl_parseident_list" */
void vhdl_freeidentlist(IDENTLIST *ilist)
{
	IDENTLIST *nextilist;

	for(; ilist != NULL; ilist = nextilist)
	{
		nextilist = ilist->next;
		efree((CHAR *)ilist);
	}
}

 /* free structure created by "vhdl_parsesimpleexpression()" */
void vhdl_freesimpleexpr(SIMPLEEXPR *sexpr)
{
	MTERMS *more, *nextmore;

	vhdl_freeterm(sexpr->term);
	for(more = sexpr->next; more != 0; more = nextmore)
	{
		nextmore = more->next;
		vhdl_freeterm(more->term);
		efree((CHAR *)more);
	}
	efree((CHAR *)sexpr);
}

/* free structure created by vhdl_parseterm() */
void vhdl_freeterm(TERM *term)
{
	MFACTORS *more, *nextmore;

	vhdl_freefactor(term->factor);
	for(more = term->next; more != 0; more = nextmore)
	{
		nextmore = more->next;
		vhdl_freefactor(more->factor);
		efree((CHAR *)more);
	}
	efree((CHAR *)term);
}

/* free structure created by vhdl_parsefactor() */
void vhdl_freefactor(FACTOR *factor)
{
	vhdl_freeprimary(factor->primary);
	if (factor->misc_operator == MISCOP_POWER)
		vhdl_freeprimary(factor->primary2);
	efree((CHAR *)factor);
}

/* free structure created by vhdl_parseprimary() */
void vhdl_freeprimary(PRIMARY *primary)
{
	if (primary == 0) return;
	switch (primary->type)
	{
		case PRIMARY_LITERAL:
			if (primary->pointer != 0) efree((CHAR *)primary->pointer);
			break;
		case PRIMARY_NAME:
			vhdl_freename((VNAME *)primary->pointer);
			break;
		case PRIMARY_EXPRESSION:
			vhdl_freeexpression((EXPRESSION *)primary->pointer);
			break;
	}
	efree((CHAR *)primary);
}

/* free structure created by "vhdl_parsename()" */
void vhdl_freename(VNAME *name)
{
	CONCATENATEDNAME *concat, *nextconcat;

	if (name == 0) return;
	if (name->type == NAME_CONCATENATE)
	{
		for(concat = (CONCATENATEDNAME *)name->pointer; concat != NULL; concat = nextconcat)
		{
			nextconcat = concat->next;
			vhdl_freesinglename(concat->name);
			efree((CHAR *)concat);
		}
	} else vhdl_freesinglename((SINGLENAME *)name->pointer);
	efree((CHAR *)name);
}

/* free structure created by "vhdl_parsesinglename()" */
void vhdl_freesinglename(SINGLENAME *sname)
{
	VNAME *nptr;
	INDEXEDNAME *in;
	PREFIX *prefix;
	SINGLENAME *sname2;

	if (sname == 0) return;
	if (sname->type == SINGLENAME_INDEXED)
	{
		in = (INDEXEDNAME *)sname->pointer;
		prefix = in->prefix;
		nptr = (VNAME *)prefix->pointer;
		sname2 = (SINGLENAME *)nptr->pointer;
		if (sname2->pointer != 0) efree((CHAR *)sname2->pointer);
		efree((CHAR *)sname2);
		efree((CHAR *)nptr);
		vhdl_freeindexedname(in);
	} else
	{
		if (sname->pointer != 0) efree((CHAR *)sname->pointer);
	}
	efree((CHAR *)sname);
}

/* free structure created by "vhdl_parseindexedname()" */
void vhdl_freeindexedname(INDEXEDNAME *ind)
{
	EXPRLIST *elist, *nextelist;

	efree((CHAR *)ind->prefix);
	for(elist = ind->expr_list; elist != NULL; elist = nextelist)
	{
		nextelist = elist->next;
		vhdl_freeexpression(elist->expression);
		efree((CHAR *)elist);
	}
	efree((CHAR *)ind);
}

/* free structure created by "vhdl_parsesubtype_indication()" */
void vhdl_freesubtypeind(SUBTYPEIND *subtypeind)
{
	vhdl_freename(subtypeind->type);
	efree((CHAR *)subtypeind);
}

/* free the structure created by "vhdl_parseexpression()" */
void vhdl_freeexpression(EXPRESSION *exp)
{
	MRELATIONS *more, *nextmore;

	vhdl_freerelation(exp->relation);
	for(more = exp->next ; more != NULL; more = nextmore)
	{
		nextmore = more->next;
		vhdl_freerelation(more->relation);
		efree((CHAR *)more);
	}
	efree((CHAR *)exp);
}

/* free the structure created by "vhdl_parserelation()" */
void vhdl_freerelation(RELATION *rel)
{
	vhdl_freesimpleexpr(rel->simple_expr);
	if (rel->simple_expr2 != 0) vhdl_freesimpleexpr(rel->simple_expr2);
	efree((CHAR *)rel);
}

/* free the structure created by "vhdl_parsebasic_declare()" */
void vhdl_freebasicdeclare(BASICDECLARE *basic)
{
	TYPE *type;
	COMPOSITE *compos;
	ARRAY *array;
	INDEXCONSTRAINT *icon, *nexticon;
	CONSTRAINED *constr;
	DISCRETERANGE *drange;
	RANGE *range;
	RANGESIMPLE *srange;
	SUBTYPEIND *subtypeind;
	OBJECTDECLARE *objdec;
	CONSTANTDECLARE *constant;
	SIGNALDECLARE *sigdecl;

	if (basic == NULL) return;
	if (basic->type == BASICDECLARE_TYPE)
	{
		type = (TYPE *)basic->pointer;
		if (type != NULL)
		{
			if (type->type == TYPE_COMPOSITE)
			{
				compos = (COMPOSITE *)type->pointer;
				if (compos != NULL)
				{
					if (compos->type == COMPOSITE_ARRAY)
					{
						array = (ARRAY *)compos->pointer;
						if (array != NULL)
						{
							constr = (CONSTRAINED *)array->pointer;
							for(icon = constr->constraint; icon != NULL; icon = nexticon)
							{
								nexticon = icon->next;
								drange = (DISCRETERANGE *)icon->discrete;
								range = (RANGE *)drange->pointer;
								srange = (RANGESIMPLE *)range->pointer;
								vhdl_freesimpleexpr(srange->start);
								vhdl_freesimpleexpr(srange->end);
								efree((CHAR *)srange);
								efree((CHAR *)range);
								efree((CHAR *)drange);
								efree((CHAR *)icon);
							}
							subtypeind = (SUBTYPEIND *)constr->subtype;
							vhdl_freename(subtypeind->type);
							efree((CHAR *)subtypeind);
							efree((CHAR *)array->pointer);
							efree((CHAR *)array);
						}
					}
					efree((CHAR *)compos);
				}
			}
			efree((CHAR *)type);
		}
	} else if (basic->type == BASICDECLARE_OBJECT)
	{
		objdec = (OBJECTDECLARE *)basic->pointer;
		if (objdec != NULL)
		{
			switch (objdec->type)
			{
				case OBJECTDECLARE_CONSTANT:
					constant = (CONSTANTDECLARE *)objdec->pointer;
					vhdl_freesubtypeind(constant->subtype);
					vhdl_freeexpression(constant->expression);
					efree((CHAR *)constant);
					break;
				case OBJECTDECLARE_SIGNAL:
					sigdecl = (SIGNALDECLARE *)objdec->pointer;
					vhdl_freesubtypeind(sigdecl->subtype);
					vhdl_freeidentlist(sigdecl->names);
					efree((CHAR *)sigdecl);
					break;
			}
			efree((CHAR *)objdec);
		}
	}
	efree((CHAR *)basic);
}

/* free structure created by vhdl_parseset_of_statements() */
void vhdl_freesetofstatements(STATEMENTS *statements)
{
	STATEMENTS *nextstatement;
	VINSTANCE *inst;
	APORTLIST *aplist, *nextaplist;
	GENERATE *gen;
	GENSCHEME *scheme;
	DISCRETERANGE *drange;
	RANGE *range;
	RANGESIMPLE *srange;

	for(; statements != NULL; statements = nextstatement)
	{
		nextstatement = statements->next;
		switch (statements->type)
		{
			case ARCHSTATE_GENERATE:
				gen = (GENERATE *)statements->pointer;
				scheme = gen->gen_scheme;
				if (scheme->scheme == GENSCHEME_FOR)
				{
					drange = scheme->range;
					range = (RANGE *)drange->pointer;
					srange = (RANGESIMPLE *)range->pointer;
					vhdl_freesimpleexpr(srange->start);
					vhdl_freesimpleexpr(srange->end);
					efree((CHAR *)srange);
					efree((CHAR *)range);
					efree((CHAR *)drange);
				} else
				{
					vhdl_freeexpression(scheme->condition);
				}
				vhdl_freesetofstatements(gen->statements);
				efree((CHAR *)scheme);
				efree((CHAR *)gen);
				break;
			case ARCHSTATE_INSTANCE:
				inst = (VINSTANCE *)statements->pointer;
				if (inst == NULL) break;
				if (inst->entity != NULL) efree((CHAR *)inst->entity);
				for(aplist = inst->ports; aplist != NULL; aplist = nextaplist)
				{
					nextaplist = aplist->next;
					if (aplist->pointer != NULL) vhdl_freename((VNAME *)aplist->pointer);
					efree((CHAR *)aplist);
				}
				efree((CHAR *)inst);
				break;
		}
		efree((CHAR *)statements);
	}
}

#endif  /* VHDLTOOL - at top */
