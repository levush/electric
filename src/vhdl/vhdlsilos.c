/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlsilos.c
 * SILOS Code Generator for the VHDL front-end compiler
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
#include "usr.h"

extern INTBIG		vhdl_externentities, vhdl_warnflag;
extern DBUNITS		*vhdl_units;
extern SYMBOLLIST	*vhdl_gsymbols;
static DBINTERFACE  *vhdl_top_interface;

/* prototypes for local routines */
static void vhdl_gensilos_interface(DBINTERFACE*, CHAR*);
static void vhdl_gensilos_aport(void*, DBINTERFACE*, DBNAME*);
static BOOLEAN vhdl_gensilos_isport(DBINTERFACE*, DBNAME*);

/*
Module:  vhdl_gensilos
------------------------------------------------------------------------
Description:
	Generate SILOS target output for the created database.  Assume
	database is semantically correct.
------------------------------------------------------------------------
Calling Sequence:  vhdl_gensilos();
------------------------------------------------------------------------
*/
void vhdl_gensilos(void)
{
	DBINTERFACE *interfacef;
	UNRESLIST *ulist;

	vhdl_freeunresolvedlist(&vhdl_unresolved_list);
	(void)vhdl_findidentkey(x_("ground"));
	(void)vhdl_findidentkey(x_("power"));

	/* print file header */
	vhdl_printoneline(x_("$*************************************************"));
	vhdl_printoneline(x_("$  SILOS Netlist file"));
	vhdl_printoneline(x_("$"));
	if ((us_useroptions&NODATEORVERSION) == 0)
		vhdl_printoneline(x_("$  File Creation:    %s"), timetostring(getcurrenttime()));
	vhdl_printoneline(x_("$-------------------------------------------------"));
	vhdl_print(x_(""));

	/* determine top level cell */
	vhdl_top_interface = vhdl_findtopinterface(vhdl_units);
	if (vhdl_top_interface == NULL)
	{
		ttyputmsg(x_("ERROR - Cannot find top entity"));
	} else
	{
		/* clear written flag on all interfaces */
		for (interfacef = vhdl_units->interfaces; interfacef != NULL;
			interfacef = interfacef->next)
		{
			interfacef->flags &= ~ENTITY_WRITTEN;
		}
		vhdl_print(x_(".TITLE %s"), vhdl_top_interface->name->string);
		vhdl_gensilos_interface(vhdl_top_interface, vhdl_top_interface->name->string);
	}

	/* print closing line of output file */
	vhdl_printoneline(x_("$********* End of netlist file *******************"));

	/* print unresolved reference list is not empty */
	if (vhdl_unresolved_list)
	{
		ttyputmsg(_("*****  UNRESOLVED REFERENCES *****"));
		for (ulist = vhdl_unresolved_list; ulist != NULL; ulist = ulist->next)
			if (ulist->numref > 0)
				ttyputmsg(_("%s, %ld time(s)"), ulist->interfacef->string, ulist->numref);
	}
}

/*
Module:  vhdl_gensilos_interface
------------------------------------------------------------------------
Description:
	Recursively generate the SILOS description for the specified model
	by first generating the lowest interface instantiation and working
	back to the top (i.e. bottom up).
------------------------------------------------------------------------
Calling Sequence:  vhdl_gensilos_interface(interfacef, name);

Name		Type			Description
----		----			-----------
interfacef	*DBINTERFACE	Pointer to interface.
name		*char			Pointer to string name of interface.
------------------------------------------------------------------------
*/
void vhdl_gensilos_interface(DBINTERFACE *interfacef, CHAR *name)
{
	DBINSTANCE *inst;
	SYMBOLTREE *symbol;
	DBPORTLIST *port;
	DBAPORTLIST *aport;
	INTBIG i;
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	DBNAMELIST *cat;
	CHAR temp[30];
	REGISTER void *infstr;

	/* go through interface's architectural body and call generate interfaces*/
	/* for any interface called by an instance which has not been already */
	/* generated */

	/* check written flag */
	if (interfacef->flags & ENTITY_WRITTEN) return;

	/* set written flag */
	interfacef->flags |= ENTITY_WRITTEN;

	/* check all instances of corresponding architectural body */
	/* and write if non-primitive instances */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL; inst = inst->next)
		{
			symbol = vhdl_searchsymbol(inst->compo->name, vhdl_gsymbols);
			if (symbol == NULL)
			{
				if (vhdl_externentities)
				{
					if (vhdl_warnflag)
					{
						ttyputmsg(_("WARNING - interface %s not found, assumed external."),
							inst->compo->name->string);
					}
					vhdl_unresolved(inst->compo->name, &vhdl_unresolved_list);
				} else
				{
					ttyputmsg(_("ERROR - interface %s not found."), inst->compo->name->string);
				}
				continue;
			} else if (symbol->pointer == NULL)
			{
				/* Should have gate entity */
				/* should be automatically added at end of .net file */
				/* EMPTY */ 
			} else
			{
				vhdl_gensilos_interface((DBINTERFACE *)symbol->pointer, inst->compo->name->string);
			}
		}
	}

	/* write this interface , unless this is the top level, in that case
	   only write the instances. */
	vhdl_print(x_(""));
	if (interfacef != vhdl_top_interface)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_(".MACRO  "));
		addstringtoinfstr(infstr, name);

		/* write port list of interface */
		for (port = interfacef->ports; port != NULL; port = port->next)
		{
			if (port->type == NULL || port->type->type == DBTYPE_SINGLE)
			{
				addstringtoinfstr(infstr, x_(" "));
				addstringtoinfstr(infstr, port->name->string);
			} else
			{
				irange = (DBINDEXRANGE *)port->type->pointer;
				drange = irange->drange;
				if (drange->start > drange->end)
				{
					for (i = drange->start; i >= drange->end; i--)
					{
						addstringtoinfstr(infstr, x_(" "));
						addstringtoinfstr(infstr, port->name->string);
						(void)esnprintf(temp, 30, x_("__%ld"), i);
						addstringtoinfstr(infstr, temp);
					}
				} else
				{
					for (i = drange->start; i <= drange->end; i++)
					{
						addstringtoinfstr(infstr, x_(" "));
						addstringtoinfstr(infstr, port->name->string);
						(void)esnprintf(temp, 30, x_("__%ld"), i);
						addstringtoinfstr(infstr, temp);
					}
				}
			}
		}
		vhdl_print(returninfstr(infstr));
	}

	/* write all instances */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL; inst = inst->next)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("("));
			addstringtoinfstr(infstr, inst->name->string);
			addstringtoinfstr(infstr, x_(" "));
			addstringtoinfstr(infstr, inst->compo->name->string);

			/* print instance port list */
			for (aport = inst->ports; aport != NULL; aport = aport->next)
			{
				if (aport->name)
				{
					if (aport->name->type == DBNAME_CONCATENATED)
					{
						/* concatenated name */
						for (cat = (DBNAMELIST *)aport->name->pointer; cat; cat = cat->next)
						{
							vhdl_gensilos_aport(infstr, interfacef, cat->name);
						}
					} else
					{
						vhdl_gensilos_aport(infstr, interfacef, aport->name);
					}
				} else
				{
					/* check if formal port is of array type */
					if (aport->port->type && aport->port->type->type == DBTYPE_ARRAY)
					{
						irange = (DBINDEXRANGE *)aport->port->type->pointer;
						drange = irange->drange;
						if (drange->start > drange->end)
						{
							for (i = drange->start; i >= drange->end; i--)
								addstringtoinfstr(infstr, x_(" .SKIP"));
						} else
						{
							for (i = drange->start; i <= drange->end; i++)
								addstringtoinfstr(infstr, x_(" .SKIP"));
						}
					} else addstringtoinfstr(infstr, x_(" .SKIP"));
				}
			}
			vhdl_print(returninfstr(infstr));
		}
	}

	if (interfacef != vhdl_top_interface) vhdl_print(x_(".EOM")); else
		vhdl_print(x_(""));
}

/*
Module:  vhdl_gensilos_aport
------------------------------------------------------------------------
Description:
	Add the actual port for a single name to the infinite string.
	Silos currently does not permit array or bus references in the
	.MACRO header, so we will replace any of the form XXX[n] with
	the form XXX__n. We have to do the same thing for instance port
	names within the macro.
------------------------------------------------------------------------
Calling Sequence:  vhdl_gensilos_aport(interfacef, name);

Name		Type			Description
----		----			-----------
interfacef	*DBINTERFACE    Pointer to interface  containing this
name		*DBNAME			Pointer to single name.
------------------------------------------------------------------------
*/
void vhdl_gensilos_aport(void *infstr, DBINTERFACE *interfacef, DBNAME *name)
{
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	INTBIG i;
	CHAR temp[30];

	if (name->type == DBNAME_INDEXED)
	{
		addstringtoinfstr(infstr, x_(" "));
		addstringtoinfstr(infstr, name->name->string);
		if (vhdl_gensilos_isport(interfacef, name))
			(void)esnprintf(temp, 30, x_("__%ld"), ((DBEXPRLIST *)(name->pointer))->value); else
				(void)esnprintf(temp, 30, x_("[%ld]"), ((DBEXPRLIST *)(name->pointer))->value);
		addstringtoinfstr(infstr, temp);
	} else
	{
		if (name->dbtype && name->dbtype->type == DBTYPE_ARRAY)
		{
			irange = (DBINDEXRANGE *)name->dbtype->pointer;
			drange = irange->drange;
			if (drange->start > drange->end)
			{
				for (i = drange->start; i >= drange->end; i--)
				{
					addstringtoinfstr(infstr, x_(" "));
					addstringtoinfstr(infstr, name->name->string);
					if (vhdl_gensilos_isport(interfacef, name))
						(void)esnprintf(temp, 30, x_("__%ld"), i); else
							(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
				}
			} else
			{
				for (i = drange->start; i <= drange->end; i++)
				{
					addstringtoinfstr(infstr, x_(" "));
					addstringtoinfstr(infstr, name->name->string);
					if (vhdl_gensilos_isport(interfacef, name))
						(void)esnprintf(temp, 30, x_("__%ld"), i); else
							(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
				}
			}
		} else
		{
			addstringtoinfstr(infstr, x_(" "));
			addstringtoinfstr(infstr, name->name->string);
		}
	}
}

/**********************************************************************
Module: vhdl_gensilos_isport
----------------------------------------------------------------------
 Search for a match  in the identifier table  between the name of
 the port on an instance and
 a name used in the macro header , if so, return true, else false.
----------------------------------------------------------------------
Calling Sequence: vhdl_gensilos_isport(interfacef, name)

Name		Type			Description
----		----			-----------
interfacef	*DBINTERFACE	Pointer to interface containing the instance
name		*DBNAME			Pointer to name structure of this port
---------------------------------------------------------------------
*/
BOOLEAN vhdl_gensilos_isport(DBINTERFACE *interfacef, DBNAME *name)
{
	DBPORTLIST *port;

	if (interfacef == vhdl_top_interface) return(FALSE);	/* no header if this is the top entity */

	/* we compare the IDENTTABLE pointers of the interface and the instance port */
	for(port = interfacef->ports; port != NULL; port = port->next)
	{
		if((IDENTTABLE *)(name->name) == (IDENTTABLE *)(port->name)) return(TRUE);
	}
	return(FALSE);
}

#endif  /* VHDLTOOL - at top */
