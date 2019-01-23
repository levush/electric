/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlnetlisp.c
 * NETLISP Code Generator for the VHDL front-end compiler
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

extern INTBIG		vhdl_externentities, vhdl_warnflag, vhdl_target;
extern DBUNITS		*vhdl_units;
extern SYMBOLLIST	*vhdl_gsymbols;
static IDENTTABLE	*vhdl_ident_ground, *vhdl_ident_power;
static DBINTERFACE	*vhdl_top_interface;

/* prototypes for local routines */
static void vhdl_gennet_interface(DBINTERFACE*, CHAR*);
static void vhdl_gennet_aport(void*, DBNAME*);

/*
Module:  vhdl_gennet
------------------------------------------------------------------------
Description:
	Generate ALS target output for the created database.  Assume
	database is semantically correct.
------------------------------------------------------------------------
Calling Sequence:  vhdl_gennet();
------------------------------------------------------------------------
*/
void vhdl_gennet(INTBIG target)
{
	DBINTERFACE *interfacef;
	UNRESLIST *ulist;
	Q_UNUSED( target );

	vhdl_freeunresolvedlist(&vhdl_unresolved_list);
	vhdl_ident_ground = vhdl_findidentkey(x_("ground"));
	vhdl_ident_power = vhdl_findidentkey(x_("power"));

	/* print file header */
	vhdl_printoneline(x_(";*************************************************"));
	vhdl_printoneline(x_(";  RNL Netlisp file"));
	vhdl_printoneline(x_(";"));
	if ((us_useroptions&NODATEORVERSION) == 0)
		vhdl_printoneline(x_(";  File Creation:    %s"), timetostring(getcurrenttime()));
	vhdl_printoneline(x_(";-------------------------------------------------"));
	vhdl_print(x_("(load \"library.net\")"));

	/* determine top level cell which must be renamed main */
	vhdl_top_interface = vhdl_findtopinterface(vhdl_units);
	if (vhdl_top_interface == NULL)
	{
		ttyputmsg(x_("ERROR - Cannot find top level interface."));
	} else
	{
		/* clear written flag on all interfaces */
		for (interfacef = vhdl_units->interfaces; interfacef != NULL;
			interfacef = interfacef->next)
		{
			interfacef->flags &= ~ENTITY_WRITTEN;
		}
		vhdl_gennet_interface(vhdl_top_interface, x_("main"));
	}

	/* print closing line of output file */
	vhdl_printoneline(x_(";********* End of netlist file *******************"));

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
Module:  vhdl_gennet_interface
------------------------------------------------------------------------
Description:
	Recursively generate the netlisp description for the specified model
	by first generating the lowest interface instantiation and working
	back to the top (i.e. bottom up).
------------------------------------------------------------------------
Calling Sequence:  vhdl_gennet_interface(interfacef, name);

Name		Type			Description
----		----			-----------
interfacef	*DBINTERFACE	Pointer to interface.
name		*char			Pointer to string name of interface.
------------------------------------------------------------------------
*/
void vhdl_gennet_interface(DBINTERFACE *interfacef, CHAR *name)
{
	DBINSTANCE *inst;
	SYMBOLTREE *symbol;
	DBPORTLIST *port;
	DBAPORTLIST *aport;
	INTBIG generic, power_flag, ground_flag, i;
	IDENTTABLE *ident;
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	DBNAMELIST *cat;
	DBSIGNALS *signal;
	CHAR temp[30];
	REGISTER void *infstr;

	/* go through interface's architectural body and call generate interfaces*/
	/* for any interface called by an instance which has not been already */
	/* generated */

	/* check written flag */
	if (interfacef->flags & ENTITY_WRITTEN) return;

	/* set written flag */
	interfacef->flags |= ENTITY_WRITTEN;

	/* check all instants of corresponding architectural body */
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
				vhdl_gennet_interface((DBINTERFACE *)symbol->pointer, inst->compo->name->string);
			}
		}
	}

	/* write this interface */
	generic = 0;
	power_flag = ground_flag = FALSE;
	infstr = initinfstr();
	if (interfacef == vhdl_top_interface) addstringtoinfstr(infstr, x_("(node ")); else
	{
		addstringtoinfstr(infstr, x_("(macro "));
		addstringtoinfstr(infstr, name);
		addstringtoinfstr(infstr, x_("("));
	}

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
					if (vhdl_target == TARGET_RSIM) (void)esnprintf(temp, 30, x_("_%ld"), i); else
						(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
				}
			} else
			{
				for (i = drange->start; i <= drange->end; i++)
				{
					addstringtoinfstr(infstr, x_(" "));
					addstringtoinfstr(infstr, port->name->string);
					if (vhdl_target == TARGET_RSIM) (void)esnprintf(temp, 30, x_("_%ld"), i); else
						(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
				}
			}
		}
	}
	addstringtoinfstr(infstr, x_(")"));
	vhdl_print(returninfstr(infstr));

	/* Write out local node definitions */
	infstr = initinfstr();
	if (interfacef == vhdl_top_interface) addstringtoinfstr(infstr, x_("(node open_s"));
		else addstringtoinfstr(infstr, x_("(local open_s"));
	if (interfacef->bodies && interfacef->bodies->declare)
	{
		for (signal = interfacef->bodies->declare->bodysignals; signal; signal = signal->next)
		{
			if (signal->type == NULL || signal->type->type == DBTYPE_SINGLE)
			{
				addstringtoinfstr(infstr, x_(" "));
				addstringtoinfstr(infstr, signal->name->string);
			} else
			{
				irange = (DBINDEXRANGE *)signal->type->pointer;
				drange = irange->drange;
				if (drange->start > drange->end)
				{
					for (i = drange->start; i >= drange->end; i--)
					{
						addstringtoinfstr(infstr, x_(" "));
						addstringtoinfstr(infstr, signal->name->string);
						if (vhdl_target == TARGET_RSIM) (void)esnprintf(temp, 30, x_("_%ld"), i); else
							(void)esnprintf(temp, 30, x_("[%ld]"), i);
						addstringtoinfstr(infstr, temp);
					}
				} else
				{
					for (i = drange->start; i <= drange->end; i++)
					{
						addstringtoinfstr(infstr, x_(" "));
						addstringtoinfstr(infstr, signal->name->string);
						if (vhdl_target == TARGET_RSIM) (void)esnprintf(temp, 30, x_("_%ld"), i); else
							(void)esnprintf(temp, 30, x_("[%ld]"), i);
						addstringtoinfstr(infstr, temp);
					}
				}
			}
		}
	}
	addstringtoinfstr(infstr, x_(")"));
	vhdl_print(returninfstr(infstr));

	/* write all instances */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL; inst = inst->next)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, x_("("));
			addstringtoinfstr(infstr, inst->compo->name->string);
			addstringtoinfstr(infstr, x_(" "));

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
							ident = cat->name->name;
							if (ident == vhdl_ident_power) power_flag = TRUE; else
								if (ident == vhdl_ident_ground) ground_flag = TRUE;
							vhdl_gennet_aport(infstr, cat->name);
						}
					} else
					{
						ident = aport->name->name;
						if (ident == vhdl_ident_power) power_flag = TRUE; else
							if (ident == vhdl_ident_ground) ground_flag = TRUE;
						vhdl_gennet_aport(infstr, aport->name);
					}
				} else
				{
					/* "Open" port connection */
					/* check if formal port is of array type */
					if (aport->port->type && aport->port->type->type == DBTYPE_ARRAY)
					{
						irange = (DBINDEXRANGE *)aport->port->type->pointer;
						drange = irange->drange;
						if (drange->start > drange->end)
						{
							for (i = drange->start; i >= drange->end; i--)
							{
								addstringtoinfstr(infstr, x_(" open_s."));
								(void)esnprintf(temp, 30, x_("%ld"), generic++);
								addstringtoinfstr(infstr, temp);
							}
						} else
						{
							for (i = drange->start; i <= drange->end; i++)
							{
								addstringtoinfstr(infstr, x_(" open_s."));
								(void)esnprintf(temp, 30, x_("%ld"), generic++);
								addstringtoinfstr(infstr, temp);
							}
						}
					} else
					{
						addstringtoinfstr(infstr, x_(" open_s."));
						(void)esnprintf(temp, 30, x_("%ld"), generic++);
						addstringtoinfstr(infstr, temp);
					}
				}
			}
			addstringtoinfstr(infstr, x_(")"));
			vhdl_print(returninfstr(infstr));
		}
	}

	/* check for power and ground flags */
	if (power_flag)
	{
		vhdl_print(x_("(connect power vdd)"));
	} else if (ground_flag)
	{
		vhdl_print(x_("(connect ground gnd)"));
	}
	if (interfacef != vhdl_top_interface) vhdl_print(x_(")"));
}

/*
Module:  vhdl_gennet_aport
------------------------------------------------------------------------
Description:
	Add the actual port for a single name to the infinite string.
------------------------------------------------------------------------
Calling Sequence:  vhdl_gennet_aport(name);

Name		Type		Description
----		----		-----------
name		*DBNAME		Pointer to single name.
------------------------------------------------------------------------
*/
void vhdl_gennet_aport(void *infstr, DBNAME *name)
{
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	INTBIG i;
	CHAR temp[30];

	if (name->type == DBNAME_INDEXED)
	{
		addstringtoinfstr(infstr, x_(" "));
		addstringtoinfstr(infstr, name->name->string);
		if (vhdl_target == TARGET_RSIM)
			(void)esnprintf(temp, 30, x_("_%ld"), ((DBEXPRLIST *)(name->pointer))->value); else
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
					if (vhdl_target == TARGET_RSIM) (void)esnprintf(temp, 30, x_("_%ld"), i); else
						(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
				}
			} else
			{
				for (i = drange->start; i <= drange->end; i++)
				{
					addstringtoinfstr(infstr, x_(" "));
					addstringtoinfstr(infstr, name->name->string);
					if (vhdl_target == TARGET_RSIM) (void)esnprintf(temp, 30, x_("_%ld"), i); else
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

#endif  /* VHDLTOOL - at top */
