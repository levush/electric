/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlquisc.c
 * QUISC Code Generator for the VHDL front-end compiler
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
#include "sc1.h"
#include "usr.h"

extern INTBIG		vhdl_externentities, vhdl_warnflag;
extern DBUNITS		*vhdl_units;
extern SYMBOLLIST	*vhdl_gsymbols;

#define QNODE_SNAME		0
#define QNODE_INAME		1
#define QNODE_EXPORT	0x0001
#define QNODE_POWER		0x0002
#define QNODE_GROUND	0x0004

typedef struct Iqnode
{
	IDENTTABLE		*name;
	INTBIG			name_type;	/* type of name - simple or indexed */
	INTBIG			start, end;	/* range if array */
	INTBIG			size;		/* size of array if indexed */
	struct Iqport	**table;	/* array of pointers if indexed */
	INTBIG			flags;		/* export flag */
	INTBIG			mode;		/* port mode if exported */
	struct Iqport	*ports;		/* list of ports */
	struct Iqnode	*next;		/* next in list of nodes */
} QNODE;

typedef struct Iqport
{
	CHAR			*instname;	/* name of instance */
	CHAR			*portname;	/* name of port */
	BOOLEAN          namealloc;	/* true if port name is allocated */
	struct Iqport	*next;		/* next in port list */
} QPORT;

static IDENTTABLE	*vhdl_ident_ground, *vhdl_ident_power;

/* prototypes for local routines */
static void vhdl_genquisc_interface(DBINTERFACE*);
static INTBIG vhdl_querysize(DBNAME*);
static void vhdl_addidentaport(DBNAME*, DBPORTLIST*, INTBIG, DBINSTANCE*, QNODE**);
static void vhdl_addindexedaport(DBNAME*, DBPORTLIST*, INTBIG, DBINSTANCE*, QNODE**);
static QPORT *vhdl_createqport(CHAR*, DBPORTLIST*, INTBIG);
static void vhdl_addporttonode(QPORT*, IDENTTABLE*, INTBIG, INTBIG, QNODE**);

/*
Module:  vhdl_genquisc
------------------------------------------------------------------------
Description:
	Generate QUISC target output for the created parse tree.  Assume
	parse tree is semantically correct.
------------------------------------------------------------------------
Calling Sequence:  vhdl_genquisc();
------------------------------------------------------------------------
*/
void vhdl_genquisc(void)
{
	CHAR *pn, *key;
	INTBIG total;
	REGISTER LIBRARY *celllib;
	REGISTER NODEPROTO *np;
	DBINTERFACE *top_interface, *interfacef;
	UNRESLIST *ulist;
	REGISTER void *infstr;

	vhdl_freeunresolvedlist(&vhdl_unresolved_list);
	vhdl_ident_ground = vhdl_findidentkey(x_("ground"));
	vhdl_ident_power = vhdl_findidentkey(x_("power"));

	/* print file header */
	vhdl_printoneline(x_("!*************************************************"));
	vhdl_printoneline(x_("!  QUISC Command file"));
	vhdl_printoneline(x_("!"));
	if ((us_useroptions&NODATEORVERSION) == 0)
		vhdl_printoneline(x_("!  File Creation:    %s"), timetostring(getcurrenttime()));
	vhdl_printoneline(x_("!-------------------------------------------------"));
	vhdl_print(x_(""));

	/* determine top level cell */
	top_interface = vhdl_findtopinterface(vhdl_units);
	if (top_interface == NULL)
		ttyputmsg(_("ERROR - Cannot find top interface.")); else
	{
		/* clear written flag on all entities */
		for (interfacef = vhdl_units->interfaces; interfacef != NULL;
			interfacef = interfacef->next) interfacef->flags &= ~ENTITY_WRITTEN;
		vhdl_genquisc_interface(top_interface);
	}

	/* print closing line of output file */
	vhdl_printoneline(x_("!********* End of command file *******************"));

	/* scan unresolved references for reality inside of Electric */
	celllib = (LIBRARY *)asktool(sc_tool, x_("cell-lib"));
	if (celllib == 0) celllib = NOLIBRARY;
	total = 0;
	for (ulist = vhdl_unresolved_list; ulist != NULL; ulist = ulist->next)
	{
		/* see if this is a reference to a cell in the current library */
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			infstr = initinfstr();
			for(pn = np->protoname; *pn != 0; pn++)
				if (isalnum(*pn)) addtoinfstr(infstr, *pn); else
					addtoinfstr(infstr, '_');
			key = returninfstr(infstr);
			if (namesame(ulist->interfacef->string, key) == 0) break;
		}
		if (np == NONODEPROTO && celllib != NOLIBRARY)
		{
			for(np = celllib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				infstr = initinfstr();
				for(pn = np->protoname; *pn != 0; pn++)
					if (isalnum(*pn)) addtoinfstr(infstr, *pn); else
						addtoinfstr(infstr, '_');
				key = returninfstr(infstr);
				if (namesame(ulist->interfacef->string, key) == 0) break;
			}
		}
		if (np != NONODEPROTO)
		{
			ulist->numref = 0;
			continue;
		}
		total++;
	}

	/* print unresolved reference list */
	if (total > 0)
	{
		ttyputmsg(_("*****  UNRESOLVED REFERENCES *****"));
		for (ulist = vhdl_unresolved_list; ulist != NULL; ulist = ulist->next)
			if (ulist->numref > 0)
				ttyputmsg(_("%s, %ld time(s)"), ulist->interfacef->string, ulist->numref);
	}
}

/*
Module:  vhdl_genquisc_interface
------------------------------------------------------------------------
Description:
	Recursively generate the QUISC description for the specified model
	by first generating the lowest interface instantiation and working
	back to the top (i.e. bottom up).
------------------------------------------------------------------------
Calling Sequence:  vhdl_genquisc_interface(interfacef);

Name		Type			Description
----		----			-----------
interfacef	*DBINTERFACE	Pointer to interface.
------------------------------------------------------------------------
*/
void vhdl_genquisc_interface(DBINTERFACE *interfacef)
{
	QNODE *qnodes, *lastnode, *newnode;
	QPORT *qport, *qport2, **itable;
	DBINSTANCE *inst;
	SYMBOLTREE *symbol;
	DBPORTLIST *fport;
	DBSIGNALS *signal;
	DBAPORTLIST *aport;
	INTBIG i, offset, size, indexc;
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	DBNAMELIST *cat;
	CHAR buff[256], *inout;

	/* go through interface's architectural body and call generate interface */
	/* for any interface called by an instance which has not been already */
	/* generated */

	/* check written flag */
	if (interfacef->flags & ENTITY_WRITTEN) return;

	/* set written flag */
	interfacef->flags |= ENTITY_WRITTEN;

	/* check all instants of corresponding architectural body */
	/* and write if non-primitive interfaces */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL; inst = inst->next)
		{
			symbol = vhdl_searchsymbol(inst->compo->name, vhdl_gsymbols);
			if (symbol == NULL || symbol->pointer == NULL)
			{
				if (vhdl_externentities)
				{
					if (vhdl_warnflag)
						ttyputmsg(_("WARNING - interface %s not found, assumed external."),
							inst->compo->name->string);
					vhdl_unresolved(inst->compo->name, &vhdl_unresolved_list);
				} else
					ttyputmsg(_("ERROR - interface %s not found."), inst->compo->name->string);
				continue;
			} else vhdl_genquisc_interface((DBINTERFACE *)symbol->pointer);
		}
	}

	/* write this entity */
	vhdl_print(x_("create cell %s"), (INTBIG)interfacef->name->string);

	/* write out instances as components */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL; inst = inst->next)
			vhdl_print(x_("create instance %s %s"), (INTBIG)inst->name->string,
				(INTBIG)inst->compo->name->string);
	}

	/* create export list */
	qnodes = lastnode = NULL;
	for (fport = interfacef->ports; fport != NULL; fport = fport->next)
	{
		if (fport->type == NULL || fport->type->type == DBTYPE_SINGLE)
		{
			newnode = (QNODE *)emalloc(sizeof(QNODE), vhdl_tool->cluster);
			newnode->name = fport->name;
			newnode->name_type = QNODE_SNAME;
			newnode->size = 0;
			newnode->start = 0;
			newnode->end = 0;
			newnode->table = (QPORT **)NULL;
			newnode->flags = QNODE_EXPORT;
			newnode->mode = fport->mode;
			newnode->ports = NULL;
			newnode->next = NULL;
			if (lastnode == NULL) qnodes = lastnode = newnode; else
			{
				lastnode->next = newnode;
				lastnode = newnode;
			}
		} else
		{
			newnode = (QNODE *)emalloc(sizeof(QNODE), vhdl_tool->cluster);
			newnode->name = fport->name;
			newnode->name_type = QNODE_INAME;
			newnode->flags = QNODE_EXPORT;
			newnode->mode = fport->mode;
			newnode->ports = NULL;
			newnode->next = NULL;
			if (lastnode == NULL) qnodes = lastnode = newnode; else
			{
				lastnode->next = newnode;
				lastnode = newnode;
			}
			irange = (DBINDEXRANGE *)fport->type->pointer;
			drange = irange->drange;
			newnode->start = drange->start;
			newnode->end = drange->end;
			if (drange->start > drange->end)
			{
				size = drange->start - drange->end + 1;
			} else if (drange->start < drange->end)
			{
				size = drange->end - drange->start + 1;
			} else
			{
				size = 1;
			}
			newnode->size = size;
			newnode->table = (QPORT **)emalloc(sizeof(QNODE *) * size, vhdl_tool->cluster);
			itable = newnode->table;
			for (i = 0; i < size; i++) itable[i] = NULL;
		}
	}

	/* add local signals */
	if (interfacef->bodies && interfacef->bodies->declare)
	{
		for (signal = interfacef->bodies->declare->bodysignals; signal; signal = signal->next)
		{
			if (signal->type == NULL || signal->type->type == DBTYPE_SINGLE)
			{
				newnode = (QNODE *)emalloc(sizeof(QNODE), vhdl_tool->cluster);
				newnode->name = signal->name;
				newnode->name_type = QNODE_SNAME;
				newnode->size = 0;
				newnode->start = 0;
				newnode->end = 0;
				newnode->table = (QPORT **)NULL;
				if (signal->name == vhdl_ident_power)
				{
					newnode->flags = QNODE_POWER;
				} else if (signal->name == vhdl_ident_ground)
				{
					newnode->flags = QNODE_GROUND;
				} else
				{
					newnode->flags = 0;
				}
				newnode->mode = 0;
				newnode->ports = NULL;
				newnode->next = NULL;
				if (lastnode == NULL)
				{
					qnodes = lastnode = newnode;
				} else
				{
					lastnode->next = newnode;
					lastnode = newnode;
				}
			} else
			{
				newnode = (QNODE *)emalloc(sizeof(QNODE), vhdl_tool->cluster);
				newnode->name = signal->name;
				newnode->name_type = QNODE_INAME;
				newnode->flags = 0;
				newnode->mode = 0;
				newnode->ports = NULL;
				newnode->next = NULL;
				if (lastnode == NULL)
				{
					qnodes = lastnode = newnode;
				} else
				{
					lastnode->next = newnode;
					lastnode = newnode;
				}
				irange = (DBINDEXRANGE *)signal->type->pointer;
				drange = irange->drange;
				newnode->start = drange->start;
				newnode->end = drange->end;
				if (drange->start > drange->end)
				{
					size = drange->start - drange->end + 1;
				} else if (drange->start < drange->end)
				{
					size = drange->end - drange->start + 1;
				} else
				{
					size = 1;
				}
				newnode->size = size;
				newnode->table = (QPORT **)emalloc(sizeof(QPORT *) * size, vhdl_tool->cluster);
				itable = newnode->table;
				for (i = 0; i < size; i++)
				{
					itable[i] = NULL;
				}
			}
		}
	}

	/* write out connects */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL; inst = inst->next)
		{
			/* check all instance ports for connections */
			for (aport = inst->ports; aport != NULL; aport = aport->next)
			{
				if (aport->name == NULL) continue;

				/* get names of all members of actual port */
				switch (aport->name->type)
				{
					case DBNAME_IDENTIFIER:
						vhdl_addidentaport(aport->name, aport->port, (INTBIG)0, inst, &qnodes);
						break;
					case DBNAME_INDEXED:
						vhdl_addindexedaport(aport->name, aport->port, (INTBIG)0, inst, &qnodes);
						break;
					case DBNAME_CONCATENATED:
						offset = 0;
						for (cat = (DBNAMELIST *)aport->name->pointer; cat; cat = cat->next)
						{
							if (cat->name->type == DBNAME_IDENTIFIER)
							{
								vhdl_addidentaport(cat->name, aport->port, (INTBIG)offset,
									inst, &qnodes);
							} else
							{
								vhdl_addindexedaport(cat->name, aport->port, (INTBIG)offset,
									inst, &qnodes);
							}
							offset += vhdl_querysize(cat->name);
						}
						break;
					default:
						ttyputmsg(_("ERROR - unknown name type on actual port."));
						break;
				}
			}
		}
	}

	/* print out connections */
	for (newnode = qnodes; newnode; newnode = newnode->next)
	{
		if (newnode->name_type == QNODE_SNAME)
		{
			qport = newnode->ports;
			if (qport)
			{
				for (qport2 = qport->next; qport2; qport2 = qport2->next)
				{
					vhdl_print(x_("connect %s %s %s %s"), (INTBIG)qport->instname,
						(INTBIG)qport->portname, (INTBIG)qport2->instname,(INTBIG)qport2->portname);
				}
				if (newnode->flags & QNODE_POWER)
				{
					vhdl_print(x_("connect %s %s power"), (INTBIG)qport->instname,
						(INTBIG)qport->portname);
				}
				if (newnode->flags & QNODE_GROUND)
				{
					vhdl_print(x_("connect %s %s ground"), (INTBIG)qport->instname,
						(INTBIG)qport->portname);
				}
			}
		} else
		{
			for (i = 0; i < newnode->size; i++)
			{
				qport = newnode->table[i];
				if (qport)
				{
					for (qport2 = qport->next; qport2; qport2 = qport2->next)
					{
						vhdl_print(x_("connect %s %s %s %s"), (INTBIG)qport->instname,
							(INTBIG)qport->portname, (INTBIG)qport2->instname,
								(INTBIG)qport2->portname);
					}
				}
			}
		}
	}

	/* print out exports */
	for (newnode = qnodes; newnode; newnode = newnode->next)
	{
		if (newnode->flags & QNODE_EXPORT)
		{
			if (newnode->name_type == QNODE_SNAME)
			{
				qport = newnode->ports;
				if (qport)
				{
					switch (newnode->mode)
					{
						case DBMODE_IN:  inout = x_(" input");    break;
						case DBMODE_OUT: inout = x_(" output");   break;
						default:         inout = x_("");          break;
					}
					vhdl_print(x_("export %s %s %s%s"), (INTBIG)qport->instname,
						(INTBIG)qport->portname, (INTBIG)newnode->name->string, inout);
				} else
				{
					esnprintf(buff, 256, x_("%s"), newnode->name->string);
					ttyputmsg(_("ERROR - no export for %s"), buff);
				}
			} else
			{
				for (i = 0; i < newnode->size; i++)
				{
					if (newnode->start > newnode->end)
					{
						indexc = newnode->start - i;
					} else
					{
						indexc = newnode->start + i;
					}
					qport = newnode->table[i];
					if (qport)
					{
						switch (newnode->mode)
						{
							case DBMODE_IN:  inout = x_(" input");    break;
							case DBMODE_OUT: inout = x_(" output");   break;
							default:         inout = x_("");          break;
						}
						vhdl_print(x_("export %s %s %s[%ld]%s"), (INTBIG)qport->instname,
							(INTBIG)qport->portname, (INTBIG)newnode->name->string,
								(INTBIG)indexc, inout);
					} else
					{
						esnprintf(buff, 256, x_("%s[%ld]"), newnode->name->string, (INTBIG)indexc);
						ttyputmsg(_("ERROR - no export for %s"), buff);
					}
				}
			}
		}
	}

	/* extract entity */
	vhdl_print(x_("extract"));

	/* print out non-exported node name assignments */
	for (newnode = qnodes; newnode; newnode = newnode->next)
	{
		if (!(newnode->flags & QNODE_EXPORT))
		{
			if (newnode->name_type == QNODE_SNAME)
			{
				qport = newnode->ports;
				if (qport)
				{
					vhdl_print(x_("set node-name %s %s %s"), (INTBIG)qport->instname,
						(INTBIG)qport->portname, (INTBIG)newnode->name->string);
				}
			} else
			{
				for (i = 0; i < newnode->size; i++)
				{
					if (newnode->start > newnode->end)
					{
						indexc = newnode->start - i;
					} else
					{
						indexc = newnode->start + i;
					}
					qport = newnode->table[i];
					if (qport)
					{
						vhdl_print(x_("set node-name %s %s %s[%ld]"),
							(INTBIG)qport->instname, (INTBIG)qport->portname,
								(INTBIG)newnode->name->string, (INTBIG)indexc);
					}
				}
			}
		}
	}

	vhdl_print(x_(""));


	/* deallocate QNODEs */
	while (qnodes != 0)
	{
		lastnode = qnodes;
		qnodes = qnodes->next;
		for(i=0; i<lastnode->size; i++)
		{
			while (lastnode->table[i] != 0)
			{
				qport = lastnode->table[i];
				lastnode->table[i] = qport->next;
				if (qport->namealloc) efree((CHAR *)qport->portname);
				efree((CHAR *)qport);
			}
		}
		if (lastnode->size > 0) efree((CHAR *)lastnode->table);
		while (lastnode->ports != 0)
		{
			qport = lastnode->ports;
			lastnode->ports = qport->next;
			if (qport->namealloc) efree((CHAR *)qport->portname);
			efree((CHAR *)qport);
		}
		efree((CHAR *)lastnode);
	}
}

/*
Module:  vhdl_querysize
------------------------------------------------------------------------
Description:
	Return the size (in number of elements) of the passed name.
------------------------------------------------------------------------
Calling Sequence:  size = vhdl_querysize(name);

Name		Type		Description
----		----		-----------
name		*DBNAME		Pointer to the name
size		INTBIG		Returned number of elements, 0 default.
------------------------------------------------------------------------
*/
INTBIG vhdl_querysize(DBNAME *name)
{
	INTBIG size;
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;

	size = 0;
	if (name)
	{
		switch (name->type)
		{
			case DBNAME_IDENTIFIER:
				if (name->dbtype)
				{
					switch (name->dbtype->type)
					{
						case DBTYPE_SINGLE:
							size = 1;
							break;
						case DBTYPE_ARRAY:
							irange = (DBINDEXRANGE *)name->dbtype->pointer;
							if (irange)
							{
								drange = irange->drange;
								if (drange)
								{
									if (drange->start > drange->end)
									{
										size = drange->start - drange->end;
									} else
									{
										size = drange->end - drange->start;
									}
									size++;
								}
							}
							break;
						default:
							break;
					}
				} else
				{
					size = 1;
				}
				break;
			case DBNAME_INDEXED:
				size = 1;
				break;
			default:
				break;
		}
	}
	return(size);
}

/*
Module:  vhdl_addidentaport
------------------------------------------------------------------------
Description:
	Add the actual port of identifier name type to the node list.
------------------------------------------------------------------------
Calling Sequence:  vhdl_addidentaport(name, port, offset, inst, qnodes);

Name		Type		Description
----		----		-----------
name		*DBNAME		Pointer to name.
port		*DBPORTLIST	Pointer to port on component.
offset		INTBIG		Offset in bits if of array type.
inst		*DBINSTANCE	Pointer to instance of component.
qnodes		**QNODE		Address of start of node list.
------------------------------------------------------------------------
*/
void vhdl_addidentaport(DBNAME *name, DBPORTLIST *port, INTBIG offset, DBINSTANCE *inst,
	QNODE **qnodes)
{
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	INTBIG i, delta, offset2;
	QPORT *newport;

	if (name->dbtype && name->dbtype->type == DBTYPE_ARRAY)
	{
		irange = (DBINDEXRANGE *)name->dbtype->pointer;
		if (irange)
		{
			drange = irange->drange;
			if (drange)
			{
				if (drange->start > drange->end)
				{
					delta = -1;
				} else if (drange->start < drange->end)
				{
					delta = 1;
				} else
				{
					delta = 0;
				}
				i = drange->start - delta;
				offset2 = 0;
				do
				{
					i += delta;
					newport = vhdl_createqport(inst->name->string, port, (INTBIG)(offset + offset2));
					vhdl_addporttonode(newport, name->name, (INTBIG)QNODE_INAME, (INTBIG)i, qnodes);
					offset2++;
				} while (i != drange->end);
			}
		}
	} else
	{
		newport = vhdl_createqport(inst->name->string, port, (INTBIG)offset);
		vhdl_addporttonode(newport, name->name, (INTBIG)QNODE_SNAME, (INTBIG)0, qnodes);
	}
}

/*
Module:  vhdl_addindexedaport
------------------------------------------------------------------------
Description:
	Add the actual port of indexed name type to the node list.
------------------------------------------------------------------------
Calling Sequence:  vhdl_addindexedaport(name, port, offset, inst, qnodes);

Name		Type		Description
----		----		-----------
name		*DBNAME		Pointer to name.
port		*DBPORTLIST	Pointer to port on component.
offset		INTBIG		Offset in bits if of array type.
inst		*DBINSTANCE	Pointer to instance of component.
qnodes		**QNODE		Address of start of node list.
------------------------------------------------------------------------
*/
void vhdl_addindexedaport(DBNAME *name, DBPORTLIST *port, INTBIG offset,
	DBINSTANCE *inst, QNODE **qnodes)
{
	QPORT *newport;
	INTBIG indexc;

	newport = vhdl_createqport(inst->name->string, port, (INTBIG)offset);
	indexc = ((DBEXPRLIST *)name->pointer)->value;
	vhdl_addporttonode(newport, name->name, (INTBIG)QNODE_INAME, (INTBIG)indexc, qnodes);
}

/*
Module:  vhdl_createqport
------------------------------------------------------------------------
Description:
	Create a qport for the indicated port.
------------------------------------------------------------------------
Calling Sequence:  qport = vhdl_createqport(iname, port, offset);

Name		Type		Description
----		----		-----------
iname		*char		Name of instance.
port		*DBPORTLIST	Pointer to port on component.
offset		INTBIG		Offset if array.
qport		*QPORT		Address of created QPORT.
------------------------------------------------------------------------
*/
QPORT *vhdl_createqport(CHAR *iname, DBPORTLIST *port, INTBIG offset)
{
	QPORT *newport;
	CHAR buff[80];

	newport = (QPORT *)emalloc(sizeof(QPORT), vhdl_tool->cluster);
	newport->instname = iname;
	newport->next = NULL;
	if (port->type && port->type->type == DBTYPE_ARRAY)
	{
		esnprintf(buff, 80, x_("%s[%ld]"), port->name->string, (INTBIG)offset);
		(void)allocstring(&(newport->portname), buff, vhdl_tool->cluster);
		newport->namealloc = TRUE;
	} else
	{
		newport->portname = port->name->string;
		newport->namealloc = FALSE;
	}

	return(newport);
}

/*
Module:  vhdl_addporttonode
------------------------------------------------------------------------
Description:
	Add the port to the node list.
------------------------------------------------------------------------
Calling Sequence:  vhdl_addporttonode(port, ident, type, indexc, qnodes);

Name		Type		Description
----		----		-----------
port		*QPORT		Port to add.
ident		*IDENTTABLE	Name of node to add to.
type		INTBIG		If simple or indexed.
qnodes		**QNODE		Address of pointer to start of list.
------------------------------------------------------------------------
*/
void vhdl_addporttonode(QPORT *port, IDENTTABLE *ident, INTBIG type, INTBIG indexc,
	QNODE **qnodes)
{
	QNODE *node;
	INTBIG tindex;

	for (node = *qnodes; node; node = node->next)
	{
		if (node->name == ident)
		{
			if (node->name_type == type)
			{
				if (type == QNODE_SNAME)
				{
					port->next = node->ports;
					node->ports = port;
					break;
				} else
				{
					if (node->start > node->end)
					{
						tindex = node->start - indexc;
					} else
					{
						tindex = indexc - node->start;
					}
					if (tindex < node->size)
					{
						port->next = node->table[tindex];
						node->table[tindex] = port;
						break;
					}
				}
			}
		}
	}
	if (!node)
	{
		ttyputmsg(_("WARNING node %s not found."), node->name->string);
	}
}

#endif  /* VHDLTOOL - at top */
