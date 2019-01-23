/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlsemantic.c
 * Semantic Analyzer for the VHDL front-end compiler
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

extern PTREE	*vhdl_ptree;
extern INTBIG	vhdl_warnflag, vhdl_target;
extern BOOLEAN	vhdl_err;
extern TOKENLIST *vhdl_nexttoken;
SYMBOLLIST		*vhdl_symbols = 0, *vhdl_gsymbols = 0;
SYMBOLLIST      *vhdl_symbollists = 0;
DBUNITS			*vhdl_units = 0;
static CHAR		vhdl_default_name[80];
#ifdef VHDL50
  static INTBIG	vhdl_default_num;
#else
  static INTBIG	vhdl_for_level = 0;
  static INTBIG	vhdl_for_tags[10];
#endif

/* prototypes for local routines */
static void             vhdl_createdefaulttype(SYMBOLLIST*);
static void             vhdl_sempackage(PACKAGE*);
static DBINTERFACE     *vhdl_seminterface(VINTERFACE*);
static DBBODY          *vhdl_sembody(BODY*);
static DBPORTLIST      *vhdl_semformal_port_list(FPORTLIST*);
static DBBODYDECLARE   *vhdl_sembody_declare(BODYDECLARE*);
static DBCOMPONENTS    *vhdl_semcomponent(VCOMPONENT*);
static DBSIGNALS       *vhdl_sembasic_declare(BASICDECLARE*);
static DBSIGNALS       *vhdl_semobject_declare(OBJECTDECLARE*);
static DBSIGNALS       *vhdl_semsignal_declare(SIGNALDECLARE*);
static void             vhdl_semconstant_declare(CONSTANTDECLARE*);
static DBSTATEMENTS    *vhdl_semset_of_statements(STATEMENTS*);
static DBINSTANCE      *vhdl_seminstance(VINSTANCE*);
static DBSTATEMENTS    *vhdl_semgenerate(GENERATE*);
#ifdef VHDL50
  static void           vhdl_semwith(WITH*);
#endif
static void             vhdl_semuse(USE*);
static SYMBOLTREE      *vhdl_searchfsymbol(IDENTTABLE*, SYMBOLLIST*);
static SYMBOLTREE      *vhdl_searchsymboltree(IDENTTABLE*, SYMBOLTREE*);
static SYMBOLTREE      *vhdl_addsymbol(IDENTTABLE*, INTBIG, CHAR*, SYMBOLLIST*);
static SYMBOLTREE      *vhdl_addsymboltree(IDENTTABLE*, INTBIG, CHAR*, SYMBOLTREE*);
static SYMBOLLIST      *vhdl_pushsymbols(SYMBOLLIST*);
static SYMBOLLIST      *vhdl_popsymbols(SYMBOLLIST*);
static void             vhdl_semaport_check(VNAME*);
static void             vhdl_semaport_check_single_name(SINGLENAME*);
static CHAR            *vhdl_parsescalar_type(void);
static COMPOSITE       *vhdl_parsecomposite_type(void);
static CHAR            *vhdl_parserecord_type(void);
static ARRAY           *vhdl_parsearray_type(void);
static void             vhdl_semtype_declare(TYPE*);
static DBLTYPE         *vhdl_semcomposite_type(COMPOSITE*);
static DBLTYPE         *vhdl_semarray_type(ARRAY*);
static DBLTYPE         *vhdl_semconstrained_array(CONSTRAINED*);
static DBDISCRETERANGE *vhdl_semdiscrete_range(DISCRETERANGE*);
static DBDISCRETERANGE *vhdl_semrange(RANGE*);
static DBLTYPE         *vhdl_semsubtype_indication(SUBTYPEIND*);
static DBLTYPE         *vhdl_semtype_mark(VNAME*);
static DBLTYPE         *vhdl_getsymboltype(SYMBOLTREE*);
static void             vhdl_freesymboltree(SYMBOLTREE *tree);

static int vhdl_freeinggsymbols;

void vhdl_freesemantic(void)
{
	DBINTERFACE	*intf;
	SYMBOLLIST *sym, *lastsym, *nextsym;
	DBBODY *body;
	DBBODYDECLARE *bodydecl;
	DBSTATEMENTS *state;
	DBCOMPONENTS *comp;
	DBPORTLIST *port;
	DBSIGNALS *signals;
	DBINSTANCE *inst;
	DBAPORTLIST *aport;
	DBEXPRLIST *expr;

	if (vhdl_units == 0) return;
	while (vhdl_units->interfaces != 0)
	{
		intf = vhdl_units->interfaces;
		vhdl_units->interfaces = vhdl_units->interfaces->next;
		while (intf->ports != 0)
		{
			port = intf->ports;
			intf->ports = intf->ports->next;
			efree((CHAR *)port);
		}
		efree((CHAR *)intf);
	}
	while (vhdl_units->bodies != 0)
	{
		body = vhdl_units->bodies;
		vhdl_units->bodies = vhdl_units->bodies->next;
		if (body->declare != 0)
		{
			bodydecl = body->declare;
			while (bodydecl->components != 0)
			{
				comp = bodydecl->components;
				bodydecl->components = bodydecl->components->next;
				while (comp->ports != 0)
				{
					port = comp->ports;
					comp->ports = comp->ports->next;
					efree((CHAR *)port);
				}
				efree((CHAR *)comp);
			}
			while (bodydecl->bodysignals != 0)
			{
				signals = bodydecl->bodysignals;
				bodydecl->bodysignals = bodydecl->bodysignals->next;
				efree((CHAR *)signals);
			}
			efree((CHAR *)bodydecl);

			if (body->statements != 0)
			{
				state = body->statements;
				while (state->instances != 0)
				{
					inst = state->instances;
					state->instances = state->instances->next;
					while (inst->ports != 0)
					{
						aport = inst->ports;
						inst->ports = inst->ports->next;
						if (aport->name != 0)
						{
							while (aport->name->pointer != 0)
							{
								expr = (DBEXPRLIST *)aport->name->pointer;
								aport->name->pointer = (CHAR *)expr->next;
								efree((CHAR *)expr);
							}
							efree((CHAR *)aport->name);
						}
						efree((CHAR *)aport);
					}
					efree((CHAR *)inst);
				}
				efree((CHAR *)state);
			}
		}
		efree((CHAR *)body);
	}
	efree((CHAR *)vhdl_units);
	vhdl_units = 0;

	for(sym = vhdl_symbollists; sym != 0; sym = sym->next)
		if (sym->root != 0) sym->root->seen = 0;
	lastsym = 0;
	for(sym = vhdl_symbollists; sym != 0; sym = nextsym)
	{
		nextsym = sym->next;
		if (sym->root != 0 && sym->root->seen != 0)
		{
			if (lastsym == 0) vhdl_symbollists = sym->next; else
				lastsym->next = sym->next;
			efree((CHAR *)sym);
			continue;
		}
		lastsym = sym;
		if (sym->root != 0) sym->root->seen = 1;
	}
	while (vhdl_symbollists != 0)
	{
		sym = vhdl_symbollists;
		vhdl_symbollists = vhdl_symbollists->next;
		if (sym->root != 0)
		{
			if (sym == vhdl_gsymbols) vhdl_freeinggsymbols = 1; else
				vhdl_freeinggsymbols = 0;
			vhdl_freesymboltree(sym->root);
			sym->root = 0;
		}
		efree((CHAR *)sym);
	}
	vhdl_gsymbols = vhdl_symbols = 0;
}

void vhdl_freesymboltree(SYMBOLTREE *tree)
{
	DBLTYPE *dbtype;
	DBINDEXRANGE *range;

	if (tree->pointer != 0)
	{
		switch (tree->type)
		{
			case SYMBOL_PACKAGE:
				if (vhdl_freeinggsymbols != 0)
					efree((CHAR *)tree->pointer);
				break;
			case SYMBOL_TYPE:
				dbtype = (DBLTYPE *)tree->pointer;
				while (dbtype->pointer != 0)
				{
					range = (DBINDEXRANGE *)dbtype->pointer;
					dbtype->pointer = (CHAR *)range->next;
					if (range->drange != 0) efree((CHAR *)range->drange);
					efree((CHAR *)range);
				}
				efree((CHAR *)dbtype);
				break;
		}
	}
	if (tree->lptr != 0) vhdl_freesymboltree(tree->lptr);
	if (tree->rptr != 0) vhdl_freesymboltree(tree->rptr);
	efree((CHAR *)tree);
}

/*
Module:  vhdl_semantic
------------------------------------------------------------------------
Description:
	Start semantic analysis of the generated parse tree.  Return the
	status of the analysis (errors).
------------------------------------------------------------------------
Calling Sequence:  err = vhdl_semantic();
------------------------------------------------------------------------
*/
BOOLEAN vhdl_semantic(void)
{
	PTREE *unit;
	DBINTERFACE *interfacef, *endinterface;
	DBBODY *body, *endbody;
	SYMBOLLIST *ssymbols;

	vhdl_err = FALSE;
	estrcpy(vhdl_default_name, x_("default"));
#ifdef VHDL50
	vhdl_default_num = 0;
#endif

	/* first free previous semantic memory */
	vhdl_freesemantic();

	vhdl_units = (DBUNITS *)emalloc((INTBIG)sizeof(DBUNITS), vhdl_tool->cluster);
	vhdl_units->interfaces = endinterface = 0;
	vhdl_units->bodies = endbody = 0;

	vhdl_symbols = vhdl_pushsymbols((SYMBOLLIST *)0);
	vhdl_gsymbols = vhdl_pushsymbols((SYMBOLLIST *)0);

	/* add defaults to symbol tree */
	vhdl_createdefaulttype(vhdl_symbols);
	ssymbols = vhdl_symbols;

	vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);

	for (unit = vhdl_ptree; unit != 0; unit = unit->next)
	{
		switch (unit->type)
		{
			case UNIT_INTERFACE:
				interfacef = vhdl_seminterface((VINTERFACE *)unit->pointer);
				if (interfacef == 0) break;
				if (endinterface == 0)
				{
					vhdl_units->interfaces = endinterface = interfacef;
				} else
				{
					endinterface->next = interfacef;
					endinterface = interfacef;
				}
				vhdl_symbols = vhdl_pushsymbols(ssymbols);
				break;
			case UNIT_BODY:
				body = vhdl_sembody((BODY *)unit->pointer);
				if (endbody == 0)
				{
					vhdl_units->bodies = endbody = body;
				} else
				{
					endbody->next = body;
					endbody = body;
				}
				vhdl_symbols = vhdl_pushsymbols(ssymbols);
				break;
			case UNIT_PACKAGE:
				vhdl_sempackage((PACKAGE *)unit->pointer);
				break;
#ifdef VHDL50
			case UNIT_WITH:
				vhdl_semwith((WITH *)unit->pointer);
				break;
#endif
			case UNIT_USE:
				vhdl_semuse((USE *)unit->pointer);
				break;
			case UNIT_FUNCTION:
			default:
				break;
		}
	}

	return(vhdl_err);
}

/*
Module:  vhdl_createdefaulttype
------------------------------------------------------------------------
Description:
	Create the default type symbol tree.
------------------------------------------------------------------------
Calling Sequence:  vhdl_createdefaulttype(symbols);

Name		Type		Description
----		----		-----------
symbols		*SYMBOLLIST	Pointer to current symbol list.
------------------------------------------------------------------------
*/
void vhdl_createdefaulttype(SYMBOLLIST *symbols)
{
	IDENTTABLE *ikey;
	CHAR *bitname;

	/* type BIT */
	allocstring(&bitname, x_("BIT"), vhdl_tool->cluster);
	if ((ikey = vhdl_findidentkey(bitname)) == 0)
	{
		ikey = vhdl_makeidentkey(bitname);
		if (ikey == 0) return;
	} else efree(bitname);
	vhdl_addsymbol(ikey, (INTBIG)SYMBOL_TYPE, (CHAR *)0, symbols);

	/* type "std_logic" */
	allocstring(&bitname, x_("std_logic"), vhdl_tool->cluster);
	if ((ikey = vhdl_findidentkey(bitname)) == 0)
	{
		ikey = vhdl_makeidentkey(bitname);
		if (ikey == 0) return;
	} else efree(bitname);
	vhdl_addsymbol(ikey, (INTBIG)SYMBOL_TYPE, (CHAR *)0, symbols);
}

/*
Module:  vhdl_sempackage
------------------------------------------------------------------------
Description:
	Semantic analysis of a package declaration.
------------------------------------------------------------------------
Calling Sequence:  vhdl_sempackage(package);

Name		Type		Description
----		----		-----------
package		*PACKAGE	Pointer to a package.
------------------------------------------------------------------------
*/
void vhdl_sempackage(PACKAGE *package)
{
	DBPACKAGE *dbpackage;
	PACKAGEDPART *part;

	if (package == 0) return;
	dbpackage = 0;

	/* search to see if package name is unique */
	if (vhdl_searchsymbol((IDENTTABLE *)package->name->pointer, vhdl_gsymbols))
	{
		vhdl_reporterrormsg(package->name, _("Symbol previously defined"));
	} else
	{
		dbpackage = (DBPACKAGE *)emalloc((INTBIG)sizeof(DBPACKAGE), vhdl_tool->cluster);
		dbpackage->name = (IDENTTABLE *)package->name->pointer;
		dbpackage->root = 0;
		vhdl_addsymbol(dbpackage->name, (INTBIG)SYMBOL_PACKAGE, (CHAR *)dbpackage, vhdl_gsymbols);
	}

	/* check package parts */
	vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);
	for (part = package->declare; part != 0; part = part->next)
	{
		vhdl_sembasic_declare(part->item);
	}
	if (dbpackage)
	{
		dbpackage->root = vhdl_symbols->root;
	}
	vhdl_symbols = vhdl_popsymbols(vhdl_symbols);
}

/*
Module:  vhdl_seminterface
------------------------------------------------------------------------
Description:
	Semantic analysis of an interface declaration.
------------------------------------------------------------------------
Calling Sequence:  dbinterface = vhdl_seminterface(interfacef);

Name		Type			Description
----		----			-----------
interfacef	*VINTERFACE		Pointer to interface parse structure.
dbinterface	*DBINTERFACE	Resultant database interface.
------------------------------------------------------------------------
*/
DBINTERFACE  *vhdl_seminterface(VINTERFACE *interfacef)
{
	DBINTERFACE *dbinter;
	SYMBOLLIST *endsymbol;

	dbinter = 0;
	if (interfacef == 0) return(dbinter);
	if (vhdl_searchsymbol((IDENTTABLE *)interfacef->name->pointer, vhdl_gsymbols))
	{
		vhdl_reporterrormsg(interfacef->name, _("Entity previously defined"));
	} else
	{
		dbinter = (DBINTERFACE *)emalloc((INTBIG)sizeof(DBINTERFACE), vhdl_tool->cluster);
		dbinter->name = (IDENTTABLE *)interfacef->name->pointer;
		dbinter->ports = 0;
		dbinter->flags = 0;
		dbinter->bodies = 0;
		dbinter->symbols = 0;
		dbinter->next = 0;
		vhdl_addsymbol(dbinter->name, (INTBIG)SYMBOL_ENTITY, (CHAR *)dbinter, vhdl_gsymbols);
		vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);
		dbinter->ports = vhdl_semformal_port_list(interfacef->ports);

		/* remove last symbol tree */
		endsymbol = vhdl_symbols;
		while (endsymbol->last->last)
		{
			endsymbol = endsymbol->last;
		}
		endsymbol->last = 0;
		dbinter->symbols = vhdl_symbols;
	}
	return(dbinter);
}

/*
Module:  vhdl_sembody
------------------------------------------------------------------------
Description:
	Semantic analysis of an body declaration.
------------------------------------------------------------------------
Calling Sequence:  dbbody = vhdl_sembody(body);

Name		Type		Description
----		----		-----------
body		*BODY		Pointer to body parse structure.
dbbody		*DBBODY		Resultant database body.
------------------------------------------------------------------------
*/
DBBODY *vhdl_sembody(BODY *body)
{
	DBBODY *dbbody;
	SYMBOLTREE *symbol;
	SYMBOLLIST *temp_symbols, *endsymbol;

	dbbody = 0;
	if (body == 0) return(dbbody);
	if (vhdl_searchsymbol((IDENTTABLE *)body->name->pointer, vhdl_gsymbols))
	{
		vhdl_reporterrormsg(body->name, _("Body previously defined"));
		return(dbbody);
	}

	/* create dbbody */
	dbbody = (DBBODY *)emalloc((INTBIG)sizeof(DBBODY), vhdl_tool->cluster);
	dbbody->classnew = body->classnew;
	dbbody->name = (IDENTTABLE *)body->name->pointer;
	dbbody->entity = 0;
	dbbody->declare = 0;
	dbbody->statements = 0;
	dbbody->parent = 0;
	dbbody->same_parent = 0;
	dbbody->next = 0;
	vhdl_addsymbol(dbbody->name, (INTBIG)SYMBOL_BODY, (CHAR *)dbbody, vhdl_gsymbols);

	/* check if interface declared */
	if ((symbol = vhdl_searchsymbol((IDENTTABLE *)body->entity->identifier->pointer,
		vhdl_gsymbols)) == 0)
	{
		vhdl_reporterrormsg((TOKENLIST *)body->entity, _("Reference to undefined entity"));
		return(dbbody);
	} else if (symbol->type != SYMBOL_ENTITY)
	{
		vhdl_reporterrormsg((TOKENLIST *)body->entity, _("Symbol is not an entity"));
		return(dbbody);
	} else
	{
		dbbody->entity = symbol->value;
		dbbody->parent = (DBINTERFACE *)symbol->pointer;
		if (symbol->pointer)
		{
			/* add interfacef-body reference to list */
			dbbody->same_parent = ((DBINTERFACE *)(symbol->pointer))->bodies;
			((DBINTERFACE *)(symbol->pointer))->bodies = dbbody;
		}
	}

	/* create new symbol tree */
	temp_symbols = vhdl_symbols;
	if (symbol->pointer)
	{
		endsymbol = vhdl_symbols;
		while (endsymbol->last)
		{
			endsymbol = endsymbol->last;
		}
		endsymbol->last = ((DBINTERFACE *)(symbol->pointer))->symbols;
	}
	vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);

	/* check body declaration */
	dbbody->declare = vhdl_sembody_declare(body->body_declare);

	/* check statements */
	dbbody->statements = vhdl_semset_of_statements(body->statements);

	/* delete current symbol table */
	vhdl_symbols = temp_symbols;
	endsymbol->last = 0;

	return(dbbody);
}

/*
Module:  vhdl_semformal_port_list
------------------------------------------------------------------------
Description:
	Check the semantic of the passed formal port list.
------------------------------------------------------------------------
Calling Sequence::  dbports = vhdl_semformal_port_list(ports);

Name		Type		Description
----		----		-----------
ports		*FPORTLIST	Pointer to start of formal port list.
dbports		*DBPORTLIST	Pointer to database port list.
------------------------------------------------------------------------
*/
DBPORTLIST *vhdl_semformal_port_list(FPORTLIST *port)
{
	IDENTLIST *names;
	SYMBOLTREE *symbol;
	DBPORTLIST *dbports, *newport, *endport;

	dbports = endport = 0;

	for (; port != 0; port = port->next)
	{
		/* check the mode of the port */
		switch (port->mode)
		{
			case MODE_IN:
			case MODE_DOTOUT:
			case MODE_OUT:
			case MODE_INOUT:
			case MODE_LINKAGE:
				break;
			default:
				vhdl_reporterrormsg(port->names->identifier, _("Unknown port mode"));
				break;
		}

		/* check the type */
		if ((symbol = vhdl_searchsymbol(vhdl_getnameident(port->type), vhdl_symbols)) == 0 ||
			symbol->type != SYMBOL_TYPE)
		{
			vhdl_reporterrormsg(vhdl_getnametoken(port->type), _("Unknown port mode"));
		}

		/* check for uniqueness of port names */
		for (names = port->names; names != 0; names = names->next)
		{
			if (vhdl_searchfsymbol((IDENTTABLE *)names->identifier->pointer, vhdl_symbols))
			{
				vhdl_reporterrormsg(names->identifier, _("Duplicate port name in port list"));
			} else
			{
				/* add to port list */
				newport = (DBPORTLIST *)emalloc((INTBIG)sizeof(DBPORTLIST), vhdl_tool->cluster);
				newport->name = (IDENTTABLE *)names->identifier->pointer;
				newport->mode = port->mode;
				if (symbol)
				{
					newport->type = (DBLTYPE *)symbol->pointer;
				} else
				{
					newport->type = 0;
				}
				newport->flags = 0;
				newport->next = 0;
				if (endport == 0)
				{
					dbports = endport = newport;
				} else
				{
					endport->next = newport;
					endport = newport;
				}
				vhdl_addsymbol(newport->name, (INTBIG)SYMBOL_FPORT, (CHAR *)newport, vhdl_symbols);
			}
		}
	}

	return(dbports);
}

/*
Module:  vhdl_sembody_declare
------------------------------------------------------------------------
Description:
	Semantic analysis of the body declaration portion of a body.
------------------------------------------------------------------------
Calling Sequence:  dbdeclare = vhdl_sembody_declare(declare);

Name		Type			Description
----		----			-----------
declare		*BODYDECLARE	Pointer to body declare.
dbdeclare	*DBBODYDECLARE	Pointer to generated body declare.
------------------------------------------------------------------------
*/
DBBODYDECLARE *vhdl_sembody_declare(BODYDECLARE *declare)
{
	DBBODYDECLARE *dbdeclare;
	DBCOMPONENTS *endcomponent, *newcomponent;
	DBSIGNALS *endsignal, *newsignals;

	dbdeclare = 0;
	if (declare == 0) return(dbdeclare);
	dbdeclare = (DBBODYDECLARE *)emalloc((INTBIG)sizeof(DBBODYDECLARE), vhdl_tool->cluster);
	dbdeclare->components = endcomponent = 0;
	dbdeclare->bodysignals = endsignal = 0;

	for (; declare != 0; declare = declare->next)
	{
		switch (declare->type)
		{
			case BODYDECLARE_BASIC:
				newsignals = vhdl_sembasic_declare((BASICDECLARE *)declare->pointer);
				if (newsignals)
				{
					if (endsignal == 0)
					{
						dbdeclare->bodysignals = endsignal = newsignals;
					} else
					{
						endsignal->next = newsignals;
						endsignal = newsignals;
					}
					while (endsignal->next)
					{
						endsignal = endsignal->next;
					}
				}
				break;
			case BODYDECLARE_COMPONENT:
				newcomponent = vhdl_semcomponent((VCOMPONENT *)declare->pointer /*, dbdeclare */);
				if (newcomponent)
				{
					if (endcomponent == 0)
					{
						dbdeclare->components = endcomponent = newcomponent;
					} else
					{
						endcomponent->next = newcomponent;
						endcomponent = newcomponent;
					}
				}
				break;
			case BODYDECLARE_RESOLUTION:
			case BODYDECLARE_LOCAL:
			default:
				break;
		}
	}

	return(dbdeclare);
}

/*
Module:  vhdl_semcomponent
------------------------------------------------------------------------
Description:
	Semantic analysis of body's component.
------------------------------------------------------------------------
Calling Sequence:  dbcomp = vhdl_semcomponent(compo);

Name		Type			Description
----		----			-----------
compo		*VCOMPONENT		Pointer to component parse.
dbcomp		*DBCOMPONENTS	Pointer to created component.
------------------------------------------------------------------------
*/
DBCOMPONENTS *vhdl_semcomponent(VCOMPONENT *compo)
{
	DBCOMPONENTS *dbcomp;

	dbcomp = 0;
	if (compo == 0) return(dbcomp);
	if (vhdl_searchfsymbol((IDENTTABLE *)compo->name->pointer, vhdl_symbols))
	{
		vhdl_reporterrormsg(compo->name, _("Identifier previously defined"));
		return(dbcomp);
	}
	dbcomp = (DBCOMPONENTS *)emalloc((INTBIG)sizeof(DBCOMPONENTS), vhdl_tool->cluster);
	dbcomp->name = (IDENTTABLE *)compo->name->pointer;
	dbcomp->ports = 0;
	dbcomp->next = 0;
	vhdl_addsymbol(dbcomp->name, (INTBIG)SYMBOL_COMPONENT, (CHAR *)dbcomp, vhdl_symbols);
	vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);
	dbcomp->ports = vhdl_semformal_port_list(compo->ports);
	vhdl_symbols = vhdl_popsymbols(vhdl_symbols);
	return(dbcomp);
}

/*
Module:  vhdl_sembasic_declare
------------------------------------------------------------------------
Description:
	Semantic analysis of basic declaration.
------------------------------------------------------------------------
Calling Sequence:  dbsignal = vhdl_sembasic_declare(declare);

Name		Type			Description
----		----			-----------
declare		*BASICDECLARE	Pointer to basic declaration structure.
dbsignal	*DBSIGNALS		Pointer to new signal, 0 if not.
------------------------------------------------------------------------
*/
DBSIGNALS *vhdl_sembasic_declare(BASICDECLARE *declare)
{
	DBSIGNALS *dbsignal;

	dbsignal = 0;
	if (declare == 0) return(dbsignal);
	switch (declare->type)
	{
		case BASICDECLARE_OBJECT:
			dbsignal = vhdl_semobject_declare((OBJECTDECLARE *)declare->pointer);
			break;
		case BASICDECLARE_TYPE:
			vhdl_semtype_declare((TYPE *)declare->pointer);
			break;
		case BASICDECLARE_SUBTYPE:
		case BASICDECLARE_CONVERSION:
		case BASICDECLARE_ATTRIBUTE:
		case BASICDECLARE_ATT_SPEC:
		default:
			break;
	}
	return(dbsignal);
}

/*
Module:  vhdl_semobject_declare
------------------------------------------------------------------------
Description:
	Semantic analysis of object declaration.
------------------------------------------------------------------------
Calling Sequence:  dbsignal = vhdl_semobject_declare(declare);

Name		Type			Description
----		----			-----------
declare		*OBJECTDECLARE	Pointer to object declaration structure.
dbsignal	*DBSIGNALS		Pointer to new signals, 0 if not.
------------------------------------------------------------------------
*/
DBSIGNALS *vhdl_semobject_declare(OBJECTDECLARE *declare)
{
	DBSIGNALS *signals;

	signals = 0;
	if (declare == 0) return(signals);
	switch (declare->type)
	{
		case OBJECTDECLARE_SIGNAL:
			signals = vhdl_semsignal_declare((SIGNALDECLARE *)declare->pointer);
			break;
		case OBJECTDECLARE_CONSTANT:
			vhdl_semconstant_declare((CONSTANTDECLARE *)declare->pointer);
			break;
		case OBJECTDECLARE_VARIABLE:
		case OBJECTDECLARE_ALIAS:
		default:
			break;
	}
	return(signals);
}

/*
Module:  vhdl_semsignal_declare
------------------------------------------------------------------------
Description:
	Semantic analysis of signal declaration.
------------------------------------------------------------------------
Calling Sequence:  signals = vhdl_semsignal_declare(signal);

Name		Type			Description
----		----			-----------
signal		*SIGNALDECLARE	Pointer to signal declaration.
signals		*DBSIGNALS		Pointer to new signals.
------------------------------------------------------------------------
*/
DBSIGNALS *vhdl_semsignal_declare(SIGNALDECLARE *signal)
{
	DBSIGNALS *signals, *newsignal;
	IDENTLIST *sig, *type;
	SYMBOLTREE *symbol;

	signals = 0;
	if (signal == 0) return(signals);

	/* check for valid type */
	type = (IDENTLIST *)vhdl_getnameident(signal->subtype->type);
	if ((symbol = vhdl_searchsymbol((IDENTTABLE *)type, vhdl_symbols)) == 0 ||
		symbol->type != SYMBOL_TYPE)
	{
		vhdl_reporterrormsg(vhdl_getnametoken(signal->subtype->type), _("Bad type"));
	}

	/* check each signal in signal list for uniqueness */
	for (sig = signal->names; sig != 0; sig = sig->next)
	{
		if (vhdl_searchsymbol((IDENTTABLE *)sig->identifier->pointer, vhdl_symbols))
		{
			vhdl_reporterrormsg(sig->identifier, _("Signal previously defined"));
		} else
		{
			newsignal = (DBSIGNALS *)emalloc((INTBIG)sizeof(DBSIGNALS), vhdl_tool->cluster);
			newsignal->name = (IDENTTABLE *)sig->identifier->pointer;
			if (symbol)
			{
				newsignal->type = (DBLTYPE *)symbol->pointer;
			} else
			{
				newsignal->type = 0;
			}
			newsignal->next = signals;
			signals = newsignal;
			vhdl_addsymbol(newsignal->name, (INTBIG)SYMBOL_SIGNAL, (CHAR *)newsignal, vhdl_symbols);
		}
	}

	return(signals);
}

/*
Module:  vhdl_semconstant_declare
------------------------------------------------------------------------
Description:
	Semantic analysis of constant declaration.
------------------------------------------------------------------------
Calling Sequence:  vhdl_semconstant_declare(constant);

Name		Type				Description
----		----				-----------
constant	*CONSTANTDECLARE	Pointer to constant declare structure.
------------------------------------------------------------------------
*/
void vhdl_semconstant_declare(CONSTANTDECLARE *constant)
{
	INTBIG value;

	if (constant == 0) return;

	/* check if name exists in top level of symbol tree */
	if (vhdl_searchfsymbol((IDENTTABLE *)constant->identifier->pointer, vhdl_symbols))
	{
		vhdl_reporterrormsg(constant->identifier, _("Symbol previously defined"));
	} else
	{
		value = vhdl_evalexpression(constant->expression);
		vhdl_addsymbol((IDENTTABLE *)constant->identifier->pointer, (INTBIG)SYMBOL_CONSTANT,
			(CHAR *)value, vhdl_symbols);
	}
}

/*
Module:  vhdl_semset_of_statements
------------------------------------------------------------------------
Description:
	Semantic analysis of architectural set of statements in a body.
------------------------------------------------------------------------
Calling Sequence:  dbstates = vhdl_semset_of_statements(state);

Name		Type			Description
----		----			-----------
state		*STATEMENTS		Pointer to architectural statements.
dbstates	*DBSTATEMENTS	Pointer to created statements.
------------------------------------------------------------------------
*/
DBSTATEMENTS *vhdl_semset_of_statements(STATEMENTS *state)
{
	DBSTATEMENTS *dbstates, *newstate;
	DBINSTANCE *newinstance, *endinstance;

	dbstates = 0;
	if (state == 0) return(dbstates);
	dbstates = (DBSTATEMENTS *)emalloc((INTBIG)sizeof(DBSTATEMENTS), vhdl_tool->cluster);
	dbstates->instances = endinstance = 0;
	for (; state != 0; state = state->next)
	{
		switch (state->type)
		{
			case ARCHSTATE_INSTANCE:
				newinstance = vhdl_seminstance((VINSTANCE *)state->pointer);
				if (endinstance == 0)
				{
					dbstates->instances = endinstance = newinstance;
				} else
				{
					endinstance->next = newinstance;
					endinstance = newinstance;
				}
				break;
			case ARCHSTATE_GENERATE:
				newstate = vhdl_semgenerate((GENERATE *)state->pointer);
				if (newstate != 0)
				{
					for (newinstance = newstate->instances; newinstance != 0;
						newinstance = newinstance->next)
					{
						if (endinstance == 0)
						{
							dbstates->instances = endinstance = newinstance;
						} else
						{
							endinstance->next = newinstance;
							endinstance = newinstance;
						}
					}
					efree((CHAR *)newstate);
				}
				break;
			case ARCHSTATE_SIG_ASSIGN:
			case ARCHSTATE_IF:
			case ARCHSTATE_CASE:
			default:
				break;
		}
	}
	return(dbstates);
}

/*
Module:  vhdl_seminstance
------------------------------------------------------------------------
Description:
	Semantic analysis of instance for an architectural body.
------------------------------------------------------------------------
Calling Sequence:  dbinst = vhdl_seminstance(inst);

Name		Type		Description
----		----		-----------
inst		*VINSTANCE	Pointer to instance structure.
dbinst		*DBINSTANCE	Pointer to created instance.
------------------------------------------------------------------------
*/
DBINSTANCE *vhdl_seminstance(VINSTANCE *inst)
{
	DBINSTANCE *dbinst;
	SYMBOLTREE *symbol;
	DBCOMPONENTS *compo;
	INTBIG iport_num, cport_num;
	APORTLIST *aplist;
	DBPORTLIST *plist;
	CHAR sbuffer[80], *sptr;
	IDENTTABLE *ikey;
	DBAPORTLIST *dbaport, *enddbaport;
#ifndef VHDL50
	INTBIG i;
	CHAR temp[20];
#endif

	dbinst = 0;
	if (inst == 0) return(dbinst);
#ifdef VHDL50
	/* check that instance name unique */
	if (!inst->name)
	{
		if (vhdl_warnflag)
			ttyputmsg(_("WARNING - no name for node instance, adding default"));

		/* create instance name */
		esnprintf(sbuffer, 80, x_("%s_%ld"), vhdl_default_name, vhdl_default_num++);
#else
	/* If inside a "for generate" make unique instance name
	 * from instance label and vhdl_for_tags[]
	 */
	if (vhdl_for_level > 0)
	{
		if (!inst->name) estrcpy(sbuffer, x_("no_name")); else
			estrcpy(sbuffer,((IDENTTABLE *)(inst->name->pointer))->string);
		for (i=1; i<=vhdl_for_level; ++i)
		{
			esnprintf(temp, 20, x_("_%ld"), (INTBIG)vhdl_for_tags[i]);
			estrcat(sbuffer,temp);
		}
#endif
		/* add to global name space */
		if ((ikey = vhdl_findidentkey(sbuffer)) == 0)
		{
			(void)allocstring(&sptr, sbuffer, vhdl_tool->cluster);
			ikey = vhdl_makeidentkey(sptr);
			if (ikey == 0) return((DBINSTANCE *)0);
		}
	} else
	{
		ikey = (IDENTTABLE *)inst->name->pointer;
	}
	dbinst = (DBINSTANCE *)emalloc((INTBIG)sizeof(DBINSTANCE), vhdl_tool->cluster);
	dbinst->name = ikey;
	dbinst->compo = 0;
	dbinst->ports = enddbaport = 0;
	dbinst->next = 0;
	if (vhdl_searchsymbol(dbinst->name, vhdl_symbols))
	{
		vhdl_reporterrormsg(inst->name, _("Instance name previously defined"));
	} else
	{
		vhdl_addsymbol(dbinst->name, (INTBIG)SYMBOL_INSTANCE, (CHAR *)dbinst, vhdl_symbols);
	}

	/* check that instance entity is among component list */
	compo = 0;
	symbol = vhdl_searchsymbol((IDENTTABLE *)inst->entity->identifier->pointer, vhdl_symbols);
	if (symbol == 0)
	{
		vhdl_reporterrormsg(inst->entity->identifier, _("Instance references undefined component"));
	} else if (symbol->type != SYMBOL_COMPONENT)
	{
		vhdl_reporterrormsg(inst->entity->identifier, _("Symbol is not a component reference"));
	} else
	{
		compo = (DBCOMPONENTS *)symbol->pointer;
		dbinst->compo = compo;

		/* check that number of ports match */
		iport_num = 0;
		for (aplist = inst->ports; aplist != 0; aplist = aplist->next)
		{
			iport_num++;
		}
		cport_num = 0;
		for (plist = compo->ports; plist != 0; plist = plist->next)
		{
			cport_num++;
		}
		if (iport_num != cport_num)
		{
			vhdl_reporterrormsg(vhdl_getnametoken((VNAME *)inst->ports->pointer),
				_("Instance has different number of ports that component"));
			return((DBINSTANCE *)0);
		}
	}

	/* check that ports of instance are either signals or entity port */
	/* note 0 ports are allowed for position placement */
	if (compo)
	{
		plist = compo->ports;
	} else
	{
		plist = 0;
	}
	for (aplist = inst->ports; aplist != 0; aplist = aplist->next)
	{
		dbaport = (DBAPORTLIST *)emalloc((INTBIG)sizeof(DBAPORTLIST), vhdl_tool->cluster);
		dbaport->name = 0;
		dbaport->port = plist;
		if (plist)
		{
			plist = plist->next;
		}
		dbaport->flags = 0;
		dbaport->next = 0;
		if (enddbaport == 0)
		{
			dbinst->ports = enddbaport = dbaport;
		} else
		{
			enddbaport->next = dbaport;
			enddbaport = dbaport;
		}
		if (aplist->pointer == 0) continue;
		dbaport->name = vhdl_semname((VNAME *)aplist->pointer);

		/* check that name is reference to a signal or formal port */
		vhdl_semaport_check((VNAME *)aplist->pointer);
	}

	return(dbinst);
}

/*
Module:  vhdl_semgenerate
------------------------------------------------------------------------
Description:
	Semantic analysis of generate statement.
------------------------------------------------------------------------
Calling Sequence:  dbstates = vhdl_semgenerate(gen);

Name		Type			Description
----		----			-----------
gen			*GENERATE		Pointer to generate statement.
dbstates	*DBSTATEMENTS	Pointer to generated statements.
------------------------------------------------------------------------
*/
DBSTATEMENTS *vhdl_semgenerate(GENERATE *gen)
{
	DBSTATEMENTS *dbstates, *oldstates;
	DBINSTANCE *inst, *endinst;
	GENSCHEME *scheme;
	SYMBOLTREE *symbol;
	INTBIG temp;
	DBDISCRETERANGE *drange;

	dbstates = 0;
	if (gen == 0) return(dbstates);

	/* check label */
#ifdef VHDL50
	if (gen->label != 0)
	{
#else
	/* For IEEE standard, check label only if not inside a for generate */
	/* Not a perfect implementation, but label is not used for anything */
	/* in this situation.  This is easier to check in the parser...     */
	if (gen->label != 0 && vhdl_for_level == 0)
	{
#endif
		/* check label for uniqueness */
		if (vhdl_searchsymbol((IDENTTABLE *)gen->label->pointer, vhdl_symbols))
		{
			vhdl_reporterrormsg(gen->label, _("Symbol previously defined"));
		} else
		{
			vhdl_addsymbol((IDENTTABLE *)gen->label->pointer, (INTBIG)SYMBOL_LABEL, (CHAR *)0,
				vhdl_symbols);
			estrcpy(vhdl_default_name, ((IDENTTABLE *)(gen->label->pointer))->string);
#ifdef VHDL50
			vhdl_default_num = 0;
#endif
		}
	}

	/* check generation scheme */
	scheme = gen->gen_scheme;
	if (scheme == 0)
		return(dbstates);
	switch (scheme->scheme)
	{
		case GENSCHEME_FOR:

#ifndef VHDL50
			/* Increment vhdl_for_level and clear tag */
			vhdl_for_tags[++vhdl_for_level] = 0;
#endif
			/* create new local symbol table */
			vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);

			/* add identifier as a variable symbol */
			symbol = vhdl_addsymbol((IDENTTABLE *)scheme->identifier->pointer, (INTBIG)SYMBOL_VARIABLE,
				(CHAR *)0, vhdl_symbols);

			/* determine direction of discrete range (ascending or descending) */
			drange = vhdl_semdiscrete_range(scheme->range);
			if (drange->start > drange->end)
			{
				temp = drange->end;
				drange->end = drange->start;
				drange->start = temp;
			}
			oldstates = 0;
			for (temp = drange->start; temp <= drange->end; temp++)
			{
				symbol->pointer = (CHAR *)temp;
				dbstates = vhdl_semset_of_statements(gen->statements);
#ifndef VHDL50
				++vhdl_for_tags[vhdl_for_level];
#endif
				if (dbstates)
				{
					if (oldstates == 0)
					{
						oldstates = dbstates;
						endinst = dbstates->instances;
						if (endinst)
						{
							while (endinst->next)
							{
								endinst = endinst->next;
							}
						}
					} else
					{
						for (inst = dbstates->instances; inst != 0; inst = inst->next)
						{
							if (endinst == 0)
							{
								oldstates->instances = endinst = inst;
							} else
							{
								endinst->next = inst;
								endinst = inst;
							}
						}
						efree((CHAR *)dbstates);
					}
				}
			}
			efree((CHAR *)drange);
			dbstates = oldstates;

			/* restore old symbol table */
			vhdl_symbols = vhdl_popsymbols(vhdl_symbols);
#ifndef VHDL50
			--vhdl_for_level;
#endif
			break;

		case GENSCHEME_IF:
			if (vhdl_evalexpression(scheme->condition))
			{
				dbstates = vhdl_semset_of_statements(gen->statements);
			}
		default:
			break;
	}
	return(dbstates);
}

#ifdef VHDL50
/*
Module:  vhdl_semwith
------------------------------------------------------------------------
Description:
	Semantic analysis of a with statement. Attempt to add package name
	to symbol list.
------------------------------------------------------------------------
Calling Sequence:  vhdl_semwith(with);

Name		Type		Description
----		----		-----------
with		*WITH		Pointer to with parse structure.
------------------------------------------------------------------------
*/
void vhdl_semwith(WITH *with)
{
	SYMBOLTREE *symbol;

	for ( ; with != 0; with = with->next)
	{
		symbol = vhdl_searchsymbol((IDENTTABLE *)with->unit->pointer, vhdl_gsymbols);
		if (symbol == 0)
		{
			vhdl_reporterrormsg(with->unit, _("Symbol is undefined"));
			continue;
		}
		if (symbol->type != SYMBOL_PACKAGE)
		{
			vhdl_reporterrormsg(with->unit, _("Symbol is not a PACKAGE"));
		} else
		{
			vhdl_addsymbol(symbol->value, (INTBIG)SYMBOL_PACKAGE, symbol->pointer, vhdl_symbols);
		}
	}
	return;
}
#endif

/*
Module:  vhdl_semuse
------------------------------------------------------------------------
Description:
	Semantic analysis of a use statement. Add package symbols to symbol
	list.
------------------------------------------------------------------------
Calling Sequence:  vhdl_semuse(use);

Name		Type		Description
----		----		-----------
use			*USE		Pointer to use parse structure.
------------------------------------------------------------------------
*/
void vhdl_semuse(USE *use)
{
	SYMBOLTREE *symbol;

	for ( ; use != 0; use = use->next)
	{
#ifndef VHDL50
		/* Note this code was lifted with minor mods from vhdl_semwith()
		 * which is not a distinct function in IEEE version.
		 * It seems a little redundant as written, but I don't
		 * really understand what Andy was doing here.....
		 */
		symbol = vhdl_searchsymbol((IDENTTABLE *)use->unit->pointer, vhdl_gsymbols);
		if (symbol == 0)
		{
			continue;
		}
		if (symbol->type != SYMBOL_PACKAGE)
		{
			vhdl_reporterrormsg(use->unit, _("Symbol is not a PACKAGE"));
		} else
		{
			vhdl_addsymbol(symbol->value, (INTBIG)SYMBOL_PACKAGE, symbol->pointer, vhdl_symbols);
		}
#endif
		symbol = vhdl_searchsymbol((IDENTTABLE *)use->unit->pointer, vhdl_gsymbols);
		if (symbol == 0)
		{
			vhdl_reporterrormsg(use->unit, _("Symbol is undefined"));
			continue;
		}
		if (symbol->type != SYMBOL_PACKAGE)
		{
			vhdl_reporterrormsg(use->unit, _("Symbol is not a PACKAGE"));
		} else
		{
			vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);
			vhdl_symbols->root = ((DBPACKAGE *)(symbol->pointer))->root;
			vhdl_symbols = vhdl_pushsymbols(vhdl_symbols);
		}
	}
}

/*
Module:  vhdl_searchsymbol
------------------------------------------------------------------------
Description:
	Search the symbol list for a symbol of the passed value.  If found,
	return a pointer to the node, if not found, return 0.  Note that
	all symbol trees of the list are checked, from last to first.
------------------------------------------------------------------------
Calling Sequence:  ptr = vhdl_searchsymbol(ident, sym_list);

Name		Type		Description
----		----		-----------
ident		*IDENTTABLE	Pointer to global name space (unique).
sym_list	*SYMBOLLIST	Pointer to last (current) symbol list.
ptr			*SYMBOLTREE	Pointer to tree node if found, else 0.
------------------------------------------------------------------------
*/
SYMBOLTREE *vhdl_searchsymbol(IDENTTABLE *ident, SYMBOLLIST *sym_list)
{
	SYMBOLTREE *node;

	for ( ; sym_list != 0; sym_list = sym_list->last)
	{
		if ((node = vhdl_searchsymboltree(ident, sym_list->root)))
		{
			return(node);
		}
	}
	return((SYMBOLTREE *)0);
}

/*
Module:  vhdl_searchfsymbol
------------------------------------------------------------------------
Description:
	Search the symbol list for the first symbol of the passed value.  If
	found, return a pointer to the node, if not found, return 0.
	Note that only the first symbol tree of the list is checked.
------------------------------------------------------------------------
Calling Sequence:  ptr = vhdl_searchfsymbol(ident, sym_list);

Name		Type		Description
----		----		-----------
ident		*IDENTTABLE	Pointer to global name space (unique).
sym_list	*SYMBOLLIST	Pointer to last (current) symbol list.
ptr			*SYMBOLTREE	Pointer to tree node if found, else 0.
------------------------------------------------------------------------
*/
SYMBOLTREE *vhdl_searchfsymbol(IDENTTABLE *ident, SYMBOLLIST *sym_list)
{
	SYMBOLTREE *node;

	if (sym_list)
	{
		if ((node = vhdl_searchsymboltree(ident, sym_list->root)))
		{
			return(node);
		}
	}
	return((SYMBOLTREE *)0);
}

/*
Module:  vhdl_searchsymboltree
------------------------------------------------------------------------
Description:
	Recursive search of a symbol tree for a particular value.  Return
	0 if not found or address of tree node if found.
------------------------------------------------------------------------
Calling Sequence:  node = vhdl_searchsymboltree(value, nptr);

Name		Type		Description
----		----		-----------
value		*IDENTTABLE	Pointer to identifier in namespace.
nptr		*SYMBOLTREE	Pointer to current node in tree.
node		*SYMBOLTREE	Returned pointer to node, 0 if not
								found.
------------------------------------------------------------------------
*/
SYMBOLTREE *vhdl_searchsymboltree(IDENTTABLE *value, SYMBOLTREE *nptr)
{
	if (nptr == 0) return((SYMBOLTREE *)0);
	if (value < nptr->value) return(vhdl_searchsymboltree(value, nptr->lptr));
	if (value > nptr->value) return(vhdl_searchsymboltree(value, nptr->rptr));
	return(nptr);
}

/*
Module:  vhdl_addsymbol
------------------------------------------------------------------------
Description:
	Add a symbol to the symbol tree at the current symbol list.
	Return pointer to created symbol.
------------------------------------------------------------------------
Calling Sequence:  symbol = vhdl_addsymbol(value, type, pointer, sym_list);

Name		Type		Description
----		----		-----------
value		*IDENTTABLE	Pointer to identifier in namespace.
type		INTBIG		Type of symbol.
pointer		*char		Generic pointer to symbol.
sym_list	*SYMBOLLIST	Pointer to symbol list.
symbol		*SYMBOLTREE	Pointer to created symbol.
------------------------------------------------------------------------
*/
SYMBOLTREE *vhdl_addsymbol(IDENTTABLE *value, INTBIG type, CHAR *pointer, SYMBOLLIST *sym_list)
{
	SYMBOLTREE *symbol;

	if (sym_list->root)
	{
		symbol = vhdl_addsymboltree(value, (INTBIG)type, pointer, sym_list->root);
	} else
	{
		sym_list->root = (SYMBOLTREE *)emalloc((INTBIG)sizeof(SYMBOLTREE), vhdl_tool->cluster);
		sym_list->root->value = value;
		sym_list->root->type = type;
		sym_list->root->pointer = pointer;
		sym_list->root->lptr = 0;
		sym_list->root->rptr = 0;
		symbol = sym_list->root;
	}
	return(symbol);
}

/*
Module:  vhdl_addsymboltree
------------------------------------------------------------------------
Description:
	Add passed item to the indicated symbol tree.  Assume the item does
	have a root.  Return the pointer to the created symbol.
------------------------------------------------------------------------
Calling Sequence:  symbol = vhdl_addsymboltree(ident, value, pointer, nptr);

Name		Type		Description
----		----		-----------
ident		*IDENTTABLE	Pointer to identifier table (unique).
type		INTBIG		Type of symbol.
pointer		*char		Generic pointer to entity.
nptr		*SYMBOLTREE	Pointer to current tree node.
symbol		*SYMBOLTREE	Pointer to created symbol.
------------------------------------------------------------------------
*/
SYMBOLTREE *vhdl_addsymboltree(IDENTTABLE *value, INTBIG type, CHAR *pointer, SYMBOLTREE *nptr)
{
	SYMBOLTREE **tptr, *newnode;

	if (value < nptr->value)
	{
		if (nptr->lptr)
		{
			newnode = vhdl_addsymboltree(value, (INTBIG)type, pointer, nptr->lptr);
			return(newnode);
		}
		tptr = &nptr->lptr;
	} else if (value > nptr->value)
	{
		if (nptr->rptr)
		{
			newnode = vhdl_addsymboltree(value, (INTBIG)type, pointer, nptr->rptr);
			return(newnode);
		}
		tptr = &nptr->rptr;
	} else
	{
		ttyputmsg(_("ERROR - symbol %s already exists"), value->string);
		return((SYMBOLTREE *)0);
	}

	/* create new node */
	newnode = (SYMBOLTREE *)emalloc((INTBIG)sizeof(SYMBOLTREE), vhdl_tool->cluster);
	newnode->value = value;
	newnode->type = type;
	newnode->pointer = pointer;
	newnode->lptr = newnode->rptr = 0;
	*tptr = newnode;
	return(newnode);
}

/*
Module:  vhdl_pushsymbols
------------------------------------------------------------------------
Description:
	Add a new symbol tree to the symbol list.
------------------------------------------------------------------------
Calling Sequence:  new_sym_list = vhdl_pushsymbols(old_sym_list);

Name			Type		Description
----			----		-----------
old_sym_list	*SYMBOLLIST	Pointer to old symbol list.
new_sym_list	*SYMBOLLIST	Returned pointer to new symbol list.
------------------------------------------------------------------------
*/
SYMBOLLIST *vhdl_pushsymbols(SYMBOLLIST *old_sym_list)
{
	SYMBOLLIST *new_sym_list;

	new_sym_list = (SYMBOLLIST *)emalloc((INTBIG)sizeof(SYMBOLLIST), vhdl_tool->cluster);
	new_sym_list->root = 0;
	new_sym_list->last = old_sym_list;

	/* save in global list */
	new_sym_list->next = vhdl_symbollists;
	vhdl_symbollists = new_sym_list;
	return(new_sym_list);
}

/*
Module:  vhdl_popsymbols
------------------------------------------------------------------------
Description:
	Pop off the top most symbol list and return next symbol list.
------------------------------------------------------------------------
Calling Sequence:  new_sym_list = vhdl_popsymbols(old_sym_list);

Name			Type		Description
----			----		-----------
old_sym_list	*SYMBOLLIST	Pointer to old symbol list.
new_sym_list	*SYMBOLLIST	Returned pointer to new symbol list.
------------------------------------------------------------------------
*/
SYMBOLLIST *vhdl_popsymbols(SYMBOLLIST *old_sym_list)
{
	SYMBOLLIST *new_sym_list;

	if (!old_sym_list)
	{
		ttyputmsg(_("ERROR - trying to pop nonexistant symbol list."));
		return((SYMBOLLIST *)0);
	}
	new_sym_list = old_sym_list->last;
	return(new_sym_list);
}

/*
Module:  vhdl_semaport_check
------------------------------------------------------------------------
Description:
	Check that the passed name which is a reference on an actual port
	list is a signal of formal port.
------------------------------------------------------------------------
Calling Sequence:  vhdl_semaport_check(name);

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to name parse structure.
------------------------------------------------------------------------
*/
void vhdl_semaport_check(VNAME *name)
{
	CONCATENATEDNAME *cat;

	switch (name->type)
	{
		case NAME_SINGLE:
			vhdl_semaport_check_single_name((SINGLENAME *)name->pointer);
			break;
		case NAME_CONCATENATE:
			for (cat = (CONCATENATEDNAME *)name->pointer; cat; cat = cat->next)
			{
				vhdl_semaport_check_single_name(cat->name);
			}
			break;
		default:
			break;
	}
	return;
}

/*
Module:  vhdl_semaport_check_single_name
------------------------------------------------------------------------
Description:
	Check that the passed single name references a signal or formal port.
------------------------------------------------------------------------
Calling Sequence:  vhdl_semaport_check_single_name(sname);

Name		Type		Description
----		----		-----------
sname		*SINGLENAME	Pointer to single name structure.
------------------------------------------------------------------------
*/
void vhdl_semaport_check_single_name(SINGLENAME *sname)
{
	SIMPLENAME *simname;
	IDENTTABLE *ident;
	SYMBOLTREE *symbol;
	INDEXEDNAME *iname;

	switch (sname->type)
	{
		case SINGLENAME_SIMPLE:
			simname = (SIMPLENAME *)sname->pointer;
			ident = (IDENTTABLE *)simname->identifier->pointer;
			if ((symbol = vhdl_searchsymbol(ident, vhdl_symbols)) == 0 ||
				(symbol->type != SYMBOL_FPORT && symbol->type != SYMBOL_SIGNAL))
			{
				vhdl_reporterrormsg(simname->identifier,
					_("Instance port has reference to unknown port"));
			}
			break;
		case SINGLENAME_INDEXED:
			iname = (INDEXEDNAME *)sname->pointer;
			ident = (IDENTTABLE *)vhdl_getprefixident(iname->prefix);
			if ((symbol = vhdl_searchsymbol(ident, vhdl_symbols)) == 0 ||
				(symbol->type != SYMBOL_FPORT && symbol->type != SYMBOL_SIGNAL))
			{
				vhdl_reporterrormsg(vhdl_getprefixtoken(iname->prefix),
					_("Instance port has reference to unknown port"));
			}
			break;
		default:
			break;
	}
	return;
}

/********************* This was the module "types.c" *********************/

/*
Module:  vhdl_parsetype
------------------------------------------------------------------------
Description:
	Parse a type declaration of the form:

		type_declaration ::=
			TYPE identifier IS type_definition ;
------------------------------------------------------------------------
Calling Sequence:  type = vhdl_parsetype();

Name		Type		Description
----		----		-----------
type		*TYPE		Pointer to type declaration structure.
------------------------------------------------------------------------
*/
TYPE *vhdl_parsetype(void)
{
	TYPE *type;
	TOKENLIST *ident;
	INTBIG type_define;
	CHAR *pointer;

	type = NULL;

	/* should be at keyword TYPE */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_TYPE))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword TYPE"));
		vhdl_getnexttoken();
		return(type);
	}
	vhdl_getnexttoken();

	/* should be at type identifier */
	if (vhdl_nexttoken->token != TOKEN_IDENTIFIER)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting an identifier"));
		vhdl_getnexttoken();
		return(type);
	}
	ident = vhdl_nexttoken;
	vhdl_getnexttoken();

	/* should be keyword IS */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_IS))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword IS"));
		vhdl_getnexttoken();
		return(type);
	}
	vhdl_getnexttoken();

	/* parse type definition */
	if (vhdl_keysame(vhdl_nexttoken, KEY_ARRAY))
	{
		type_define = TYPE_COMPOSITE;
		pointer = (CHAR *)vhdl_parsecomposite_type();
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_RECORD))
	{
		type_define = TYPE_COMPOSITE;
		pointer = (CHAR *)vhdl_parsecomposite_type();
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_RANGE))
	{
		type_define = TYPE_SCALAR;
		pointer = vhdl_parsescalar_type();
	} else if (vhdl_nexttoken->token == TOKEN_LEFTBRACKET)
	{
		type_define = TYPE_SCALAR;
		pointer = vhdl_parsescalar_type();
	} else
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Invalid type definition"));
		vhdl_getnexttoken();
		return(type);
	}

	/* should be at semicolon */
	if (vhdl_nexttoken->token != TOKEN_SEMICOLON)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a semicolon"));
		vhdl_getnexttoken();
		return(type);
	}
	vhdl_getnexttoken();

	type = (TYPE *)emalloc((INTBIG)sizeof(TYPE), vhdl_tool->cluster);
	type->identifier = ident;
	type->type = type_define;
	type->pointer = pointer;

	return(type);
}

CHAR *vhdl_parsescalar_type(void) { return((CHAR *)NULL); }

/*
Module:  vhdl_parsecomposite_type
------------------------------------------------------------------------
Description:
	Parse a composite type definition of the form:

		composite_type_definition ::=
			  array_type_definition
			| record_type_definition
------------------------------------------------------------------------
*/
COMPOSITE *vhdl_parsecomposite_type(void)
{
	COMPOSITE *compo;
	INTBIG type;
	CHAR *pointer;

	compo = NULL;

	/* should be ARRAY or RECORD keyword */
	if (vhdl_keysame(vhdl_nexttoken, KEY_ARRAY))
	{
		type = COMPOSITE_ARRAY;
		pointer = (CHAR *)vhdl_parsearray_type();
	} else if (vhdl_keysame(vhdl_nexttoken, KEY_RECORD))
	{
		type = COMPOSITE_RECORD;
		pointer = vhdl_parserecord_type();
	} else
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Invalid composite type"));
		vhdl_getnexttoken();
		return(compo);
	}

	compo = (COMPOSITE *)emalloc((INTBIG)sizeof(COMPOSITE), vhdl_tool->cluster);
	compo->type = type;
	compo->pointer = pointer;

	return(compo);
}

CHAR *vhdl_parserecord_type() { return((CHAR *)NULL); }

/*
Module:  vhdl_parsearray_type
------------------------------------------------------------------------
Description:
	Parse an array type definition of the form:

		array_type_definition ::=
			  unconstrained_array_definition
			| constrained_array_definition

		unconstrained_array_definition ::=
			ARRAY (index_subtype_definition {, index_subtype_definition})
				OF subtype_indication

		constrained_array_definition ::=
			ARRAY index_constraint OF subtype_indication

		index_constraint ::= (discrete_range {, discrete_range})

NOTE:  Only currently supporting constrained array definitions.
------------------------------------------------------------------------
Calling Sequence:  array = vhdl_parsearray_type();

Name		Type		Description
----		----		-----------
array		*ARRAY		Pointer to array type definition.
------------------------------------------------------------------------
*/
ARRAY *vhdl_parsearray_type(void)
{
	ARRAY *array;
	INDEXCONSTRAINT *iconstraint, *endconstraint, *newconstraint;
	SUBTYPEIND *subtype;
	CONSTRAINED *constr;

	array = NULL;

	/* should be keyword ARRAY */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_ARRAY))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword ARRAY"));
		vhdl_getnexttoken();
		return(array);
	}
	vhdl_getnexttoken();

	/* index_constraint */
	/* should be left bracket */
	if (vhdl_nexttoken->token != TOKEN_LEFTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a left bracket"));
		vhdl_getnexttoken();
		return(array);
	}
	vhdl_getnexttoken();

	/* should at least one discrete range */
	iconstraint = (INDEXCONSTRAINT *)emalloc((INTBIG)sizeof(INDEXCONSTRAINT), vhdl_tool->cluster);
	iconstraint->discrete = vhdl_parsediscrete_range();
	iconstraint->next = NULL;
	endconstraint = iconstraint;

	/* continue while comma */
	while (vhdl_nexttoken->token == TOKEN_COMMA)
	{
		vhdl_getnexttoken();
		newconstraint = (INDEXCONSTRAINT *)emalloc((INTBIG)sizeof(INDEXCONSTRAINT), vhdl_tool->cluster);
		newconstraint->discrete = vhdl_parsediscrete_range();
		newconstraint->next = NULL;
		endconstraint->next = newconstraint;
		endconstraint = newconstraint;
	}

	/* should be at right bracket */
	if (vhdl_nexttoken->token != TOKEN_RIGHTBRACKET)
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting a right bracket"));
		vhdl_getnexttoken();
		return(array);
	}
	vhdl_getnexttoken();

	/* should be at keyword OF */
	if (!vhdl_keysame(vhdl_nexttoken, KEY_OF))
	{
		vhdl_reporterrormsg(vhdl_nexttoken, _("Expecting keyword OF"));
		vhdl_getnexttoken();
		return(array);
	}
	vhdl_getnexttoken();

	/* subtype_indication */
	subtype = vhdl_parsesubtype_indication();

	/* create array type definition */
	array = (ARRAY *)emalloc((INTBIG)sizeof(ARRAY), vhdl_tool->cluster);
	array->type = ARRAY_CONSTRAINED;
	constr = (CONSTRAINED *)emalloc((INTBIG)sizeof(CONSTRAINED), vhdl_tool->cluster);
	array->pointer = (CHAR *)constr;
	constr->constraint = iconstraint;
	constr->subtype = subtype;

	return(array);
}

/*
Module:  vhdl_semtype_declare
------------------------------------------------------------------------
Description:
	Semantic analysis of a type declaration.
------------------------------------------------------------------------
Calling Sequence:  vhdl_semtype_declare(type);

Name		Type		Description
----		----		-----------
type		*TYPE		Pointer to type parse tree.
------------------------------------------------------------------------
*/
void vhdl_semtype_declare(TYPE *type)
{
	DBLTYPE *dbtype;

	dbtype = NULL;
	if (type == NULL) return;

	/* check that type name is distict */
	if (vhdl_searchsymbol((IDENTTABLE *)type->identifier->pointer, vhdl_symbols))
	{
		vhdl_reporterrormsg(type->identifier, _("Identifier previously defined"));
		return;
	}

	/* check type definition */
	switch (type->type)
	{
		case TYPE_SCALAR:
			break;
		case TYPE_COMPOSITE:
			dbtype = vhdl_semcomposite_type((COMPOSITE *)type->pointer);
			break;
		default:
			break;
	}

	/* add symbol to list */
	if (dbtype)
	{
		dbtype->name = (IDENTTABLE *)type->identifier->pointer;
		vhdl_addsymbol(dbtype->name, (INTBIG)SYMBOL_TYPE, (CHAR *)dbtype, vhdl_symbols);
	}
}

/*
Module:  vhdl_semcomposite_type
------------------------------------------------------------------------
Description:
	Semantic analysis of a composite type definition.
------------------------------------------------------------------------
Calling Sequence:  dbtype = vhdl_semcomposite_type(composite);

Name		Type		Description
----		----		-----------
composite	*COMPOSITE	Pointer to composite type structure.
dbtype		*DBLTYPE	Generated db type.
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_semcomposite_type(COMPOSITE *composite)
{
	DBLTYPE *dbtype;

	dbtype = NULL;
	if (composite == NULL) return(dbtype);
	switch (composite->type)
	{
		case COMPOSITE_ARRAY:
			dbtype = vhdl_semarray_type((ARRAY *)composite->pointer);
			break;
		case COMPOSITE_RECORD:
			break;
		default:
			break;
	}
	return(dbtype);
}

/*
Module:  vhdl_semarray_type
------------------------------------------------------------------------
Description:
	Semantic analysis of an array composite type definition.
------------------------------------------------------------------------
Calling Sequence:  dbtype = vhdl_semarray_type(array);

Name		Type		Description
----		----		-----------
array		*ARRAY		Pointer to composite array type structure.
dbtype		*DBLTYPE	Pointer to generated type.
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_semarray_type(ARRAY *array)
{
	DBLTYPE *dbtype;

	dbtype = NULL;
	if (array == NULL) return(dbtype);
	switch (array->type)
	{
		case ARRAY_UNCONSTRAINED:
			break;
		case ARRAY_CONSTRAINED:
			dbtype = vhdl_semconstrained_array((CONSTRAINED *)array->pointer);
			break;
		default:
			break;
	}
	return(dbtype);
}

/*
Module:  vhdl_semconstrained_array
------------------------------------------------------------------------
Description:
	Semantic analysis of a composite constrained array type definition.
------------------------------------------------------------------------
Calling Sequence:  dbtype = vhdl_semconstrained_array(constr);

Name		Type			Description
----		----			-----------
constr		*CONSTRAINED	Pointer to constrained array structure.
dbtype		*DBLTYPE		Pointer to generated type.
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_semconstrained_array(CONSTRAINED *constr)
{
	DBLTYPE *dbtype;
	INDEXCONSTRAINT *indexc;
	DBINDEXRANGE *newrange, *endrange;

	dbtype = NULL;
	if (constr == NULL) return(dbtype);
	dbtype = (DBLTYPE *)emalloc((INTBIG)sizeof(DBLTYPE), vhdl_tool->cluster);
	dbtype->name = NULL;
	dbtype->type = DBTYPE_ARRAY;
	endrange = NULL;
	dbtype->pointer = NULL;
	dbtype->subtype = NULL;

	/* check index constraint */
	for (indexc = constr->constraint; indexc != NULL; indexc = indexc->next)
	{
		newrange = (DBINDEXRANGE *)emalloc((INTBIG)sizeof(DBINDEXRANGE), vhdl_tool->cluster);
		newrange->drange = vhdl_semdiscrete_range(indexc->discrete);
		newrange->next = NULL;
		if (endrange == NULL)
		{
			endrange = newrange;
			dbtype->pointer = (CHAR *)newrange;
		} else
		{
			endrange->next = newrange;
			endrange = newrange;
		}
	}
	/* check subtype indication */
	dbtype->subtype = vhdl_semsubtype_indication(constr->subtype);

	return(dbtype);
}

/*
Module:  vhdl_semdiscrete_range
------------------------------------------------------------------------
Description:
	Semantic analysis of a discrete range.
------------------------------------------------------------------------
Calling Sequence:  dbrange = vhdl_semdiscrete_range(discrete);

Name		Type				Description
----		----				-----------
discrete	*DISCRETERANGE		Pointer to a discrete range structure.
dbrange		*DBDISCRETERANGE	Pointer to generated range.
------------------------------------------------------------------------
*/
DBDISCRETERANGE *vhdl_semdiscrete_range(DISCRETERANGE *discrete)
{
	DBDISCRETERANGE *dbrange;

	dbrange = NULL;
	if (discrete == NULL) return(dbrange);
	switch (discrete->type)
	{
		case DISCRETERANGE_SUBTYPE:
			break;
		case DISCRETERANGE_RANGE:
			dbrange = vhdl_semrange((RANGE *)discrete->pointer);
			break;
		default:
			break;
	}
	return(dbrange);
}

/*
Module:  vhdl_semrange
------------------------------------------------------------------------
Description:
	Semantic analysis of a range.
------------------------------------------------------------------------
Calling Sequence:  dbrange = vhdl_semrange(range);

Name		Type				Description
----		----				-----------
range		*RANGE				Pointer to a range structure.
dbrange		*DBDISCRETERANGE	Pointer to generated range.
------------------------------------------------------------------------
*/
DBDISCRETERANGE *vhdl_semrange(RANGE *range)
{
	DBDISCRETERANGE *dbrange;
	RANGESIMPLE *rsimp;

	dbrange = NULL;
	if (range == NULL) return(dbrange);
	switch (range->type)
	{
		case RANGE_ATTRIBUTE:
			break;
		case RANGE_SIMPLE_EXPR:
			rsimp = (RANGESIMPLE *)range->pointer;
			if (rsimp)
			{
				dbrange = (DBDISCRETERANGE *)emalloc((INTBIG)sizeof(DBDISCRETERANGE), vhdl_tool->cluster);
				dbrange->start = vhdl_evalsimpleexpr(rsimp->start);
				dbrange->end = vhdl_evalsimpleexpr(rsimp->end);
			}
			break;
		default:
			break;
	}
	return(dbrange);
}

/*
Module:  vhdl_semsubtype_indication
------------------------------------------------------------------------
Description:
	Semantic analysis of a sybtype indication.
------------------------------------------------------------------------
Calling Sequence:  dbtype = vhdl_semsubtype_indication(subtype);

Name		Type		Description
----		----		-----------
subtype		*SUBTYPEIND	Pointer to subtype indication.
dbtype		*DBLTYPE	Pointer to db type;
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_semsubtype_indication(SUBTYPEIND *subtype)
{
	DBLTYPE *dbtype;

	dbtype = NULL;
	if (subtype == NULL) return(dbtype);
	dbtype = vhdl_semtype_mark(subtype->type);
	return(dbtype);
}

/*
Module:  vhdl_semtype_mark
------------------------------------------------------------------------
Description:
	Semantic type mark.
------------------------------------------------------------------------
Calling Sequence:  dbtype = vhdl_semtype_mark(name);

Name		Type		Description
----		----		-----------
name		*VNAME		Pointer to type name.
dbtype		*DBLTYPE	Pointer to db type.
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_semtype_mark(VNAME *name)
{
	DBLTYPE *dbtype;
	SYMBOLTREE *symbol;

	dbtype = NULL;
	if (name == NULL) return(dbtype);
	if ((symbol = vhdl_searchsymbol(vhdl_getnameident(name), vhdl_symbols)) == NULL ||
		symbol->type != SYMBOL_TYPE)
	{
		vhdl_reporterrormsg(vhdl_getnametoken(name), _("Bad type"));
	} else
	{
		dbtype = (DBLTYPE *)symbol->pointer;
	}
	return(dbtype);
}

/*
Module:  vhdl_getsymboltype
------------------------------------------------------------------------
Description:
	Return a pointer to the type of a symbol.  If no type exists,
	return a NULL.
------------------------------------------------------------------------
Calling Sequence:  type = vhdl_getsymboltype(symbol);

Name		Type		Description
----		----		-----------
symbol		*SYMBOLTREE	Pointer to symbol.
type		*DBLTYPE	Pointer to returned type, NULL if no type exists.
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_getsymboltype(SYMBOLTREE *symbol)
{
	DBLTYPE *type;
	DBPORTLIST *fport;
	DBSIGNALS *signal;

	type = NULL;
	if (symbol == NULL) return(type);
	switch (symbol->type)
	{
		case SYMBOL_FPORT:
			if ((fport = (DBPORTLIST *)symbol->pointer) == NULL) break;
			type = fport->type;
			break;
		case SYMBOL_SIGNAL:
			if ((signal = (DBSIGNALS *)symbol->pointer) == NULL) break;
			type = signal->type;
			break;
		case SYMBOL_TYPE:
			type = (DBLTYPE *)symbol->pointer;
			break;
		default:
			break;
	}
	return(type);
}

/*
Module:  vhdl_gettype
------------------------------------------------------------------------
Description:
	Return the type of an identifier.  Return NULL if no type.
------------------------------------------------------------------------
Calling Sequence:  type = vhdl_gettype(ident);

Name		Type		Description
----		----		-----------
ident		*IDENTTABLE	Pointer to identifier.
type		*DBLTYPE	Returned type, NULL if no type.
------------------------------------------------------------------------
*/
DBLTYPE *vhdl_gettype(IDENTTABLE *ident)
{
	DBLTYPE *type;
	SYMBOLTREE *symbol;

	type = NULL;
	if (ident)
	{
		if ((symbol = vhdl_searchsymbol(ident, vhdl_symbols)))
		{
			type = vhdl_getsymboltype(symbol);
		}
	}

	return(type);
}

#endif  /* VHDLTOOL - at top */
