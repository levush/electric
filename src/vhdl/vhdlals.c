/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: vhdlals.c
 * ALS Code Generator for the VHDL front-end compiler
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

/* prototypes for local routines */
static BOOLEAN vhdl_addnetlist(LIBRARY*, CHAR*);
static void    vhdl_genfunction(CHAR*, BOOLEAN, BOOLEAN, INTBIG);
static void    vhdl_dumpfunction(CHAR*, CHAR**);
static void    vhdl_genals_interface(DBINTERFACE*, CHAR*);
static void    vhdl_genals_aport(void*, INTBIG*, DBNAME*);

/*
Module:  vhdl_genals
------------------------------------------------------------------------
Description:
	Generate ALS target output for the created database.  Assume
	database is semantically correct.
------------------------------------------------------------------------
Calling Sequence:  vhdl_genals(lib, basenp);
------------------------------------------------------------------------
*/
void vhdl_genals(LIBRARY *lib, NODEPROTO *basenp)
{
	CHAR *gate;
	INTBIG total;
	DBINTERFACE *top_interface, *interfacef;
	UNRESLIST *ulist;
	static CHAR *power[] =
	{
		x_("gate power(p)"),
		x_("set p=H@3"),
		x_("t: delta=0"),
		0
	};
	static CHAR *ground[] =
	{
		x_("gate ground(g)"),
		x_("set g=L@3"),
		x_("t: delta=0"),
		0
	};
	static CHAR *pMOStran[] =
	{
		x_("function PMOStran(g, a1, a2)"),
		x_("i: g, a1, a2"),
		x_("o: a1, a2"),
		x_("t: delta=1e-8"),
		0
	};
	static CHAR *pMOStranWeak[] =
	{
		x_("function pMOStranWeak(g, a1, a2)"),
		x_("i: g, a1, a2"),
		x_("o: a1, a2"),
		x_("t: delta=1e-8"),
		0
	};
	static CHAR *nMOStran[] =
	{
		x_("function nMOStran(g, a1, a2)"),
		x_("i: g, a1, a2"),
		x_("o: a1, a2"),
		x_("t: delta=1e-8"),
		0
	};
	static CHAR *nMOStranWeak[] =
	{
		x_("function nMOStranWeak(g, a1, a2)"),
		x_("i: g, a1, a2"),
		x_("o: a1, a2"),
		x_("t: delta=1e-8"),
		0
	};
	static CHAR *inverter[] =
	{
		x_("gate inverter(a,z)"),
		x_("t: delta=1.33e-9"),		/* T3=1.33 */
		x_("i: a=L o: z=H"),
		x_("t: delta=1.07e-9"),		/* T1=1.07 */
		x_("i: a=H o: z=L"),
		x_("t: delta=0"),
		x_("i: a=X o: z=X"),
		x_("load: a=1.0"),
		0
	};
	static CHAR *buffer[] =
	{
		x_("gate buffer(in,out)"),
		x_("t: delta=0.56e-9"),		/* T4*Cin=0.56 */
		x_("i: in=H o: out=H"),
		x_("t: delta=0.41e-9"),		/* T2*Cin=0.41 */
		x_("i: in=L o: out=L"),
		x_("t: delta=0"),
		x_("i: in=X o: out=X"),
		0
	};
	static CHAR *xor2[] =
	{
		/* input a,b cap = 0.xx pF, Tphl = T1 + T2*Cin, Tplh = T3 + T4*Cin */
		x_("model xor2(a,b,z)"),
		x_("g1: xor2fun(a,b,out)"),
		x_("g2: xor2buf(out,z)"),

		x_("gate xor2fun(a,b,out)"),
		x_("t: delta=1.33e-9"),		/* T3=1.33 */
		x_("i: a=L b=H o: out=H"),
		x_("i: a=H b=L o: out=H"),
		x_("t: delta=1.07e-9"),		/* T1=1.07 */
		x_("i: a=L b=L o: out=L"),
		x_("i: a=H b=H o: out=L"),
		x_("t: delta=0"),
		x_("i:         o: out=X"),
		x_("load: a=1.0 b=1.0"),

		x_("gate xor2buf(in,out)"),
		x_("t: delta=0.56e-9"),		/* T4*Cin=0.56 */
		x_("i: in=H    o: out=H"),
		x_("t: delta=0.41e-9"),		/* T2*Cin=0.41 */
		x_("i: in=L    o: out=L"),
		x_("t: delta=0"),
		x_("i: in=X    o: out=X"),
		0
	};
	static CHAR *JKFF[] =
	{
		x_("model jkff(j, k, clk, pr, clr, q, qbar)"),
		x_("n: JKFFLOP(clk, j, k, q, qbar)"),
		x_("function JKFFLOP(clk, j, k, q, qbar)"),
		x_("i: clk, j, k"),
		x_("o: q, qbar"),
		x_("t: delta=1e-8"),
		0
	};
	static CHAR *DFF[] =
	{
		x_("model dsff(d, clk, pr, q)"),
		x_("n: DFFLOP(d, clk, q)"),
		x_("function DFFLOP(d, clk, q)"),
		x_("i: d, clk"),
		x_("o: q"),
		x_("t: delta=1e-8"),
		0
	};

	vhdl_freeunresolvedlist(&vhdl_unresolved_list);
	vhdl_ident_ground = vhdl_findidentkey(x_("ground"));
	vhdl_ident_power = vhdl_findidentkey(x_("power"));

	/* print file header */
	vhdl_printoneline(x_("#*************************************************"));
	vhdl_printoneline(x_("#  ALS Netlist file"));
	vhdl_printoneline(x_("#"));
	if ((us_useroptions&NODATEORVERSION) == 0)
	{
		vhdl_printoneline(x_("#  File Creation:    %s"), timetostring(getcurrenttime()));
	}
	vhdl_printoneline(x_("#-------------------------------------------------"));
	vhdl_print(x_(""));

	/* determine top level cell which must be renamed from "basenp" */
	top_interface = vhdl_findtopinterface(vhdl_units);
	if (top_interface == NULL)
	{
		ttyputmsg(_("ERROR - Cannot find interface to rename main."));
	} else
	{
		/* clear written flag on all interfaces */
		for (interfacef = vhdl_units->interfaces; interfacef != NULL; interfacef = interfacef->next)
		{
			interfacef->flags &= ~ENTITY_WRITTEN;
		}
		vhdl_genals_interface(top_interface, basenp->protoname);
	}

	/* print closing line of output file */
	vhdl_printoneline(x_("#********* End of netlist *******************"));

	/* scan unresolved references for reality inside of Electric */
	total = 0;
	for (ulist = vhdl_unresolved_list; ulist != NULL; ulist = ulist->next)
	{
		total++;
		gate = ulist->interfacef->string;

		/* first see if this is a reference to a cell in the current library */
		if (vhdl_addnetlist(el_curlib, gate))
		{
			ulist->numref = 0;
			total--;
			continue;
		}

		/* next see if this is a reference to the behavior library */
		if (lib != NOLIBRARY && lib != el_curlib)
		{
			if (vhdl_addnetlist(lib, gate))
			{
				ulist->numref = 0;
				total--;
				continue;
			}
		}

		/* now see if this is a reference to a function primitive */
		if (estrcmp(gate, x_("PMOStran")) == 0)
		{
			vhdl_dumpfunction(gate, pMOStran);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("pMOStranWeak")) == 0)
		{
			vhdl_dumpfunction(gate, pMOStranWeak);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("nMOStran")) == 0)
		{
			vhdl_dumpfunction(gate, nMOStran);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("nMOStranWeak")) == 0)
		{
			vhdl_dumpfunction(gate, nMOStranWeak);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("inverter")) == 0)
		{
			vhdl_dumpfunction(gate, inverter);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("buffer")) == 0)
		{
			vhdl_dumpfunction(gate, buffer);
			ulist->numref = 0;
			total--;
		} else if (estrncmp(gate, x_("and"), 3) == 0 && isdigit(gate[3]))
		{
			vhdl_genfunction(x_("and"), TRUE, FALSE, eatoi(&gate[3]));
			ulist->numref = 0;
			total--;
		} else if (estrncmp(gate, x_("nand"), 4) == 0 && isdigit(gate[4]))
		{
			vhdl_genfunction(x_("and"), TRUE, TRUE, eatoi(&gate[4]));
			ulist->numref = 0;
			total--;
		} else if (estrncmp(gate, x_("or"), 2) == 0 && isdigit(gate[2]))
		{
			vhdl_genfunction(x_("or"), FALSE, FALSE, eatoi(&gate[2]));
			ulist->numref = 0;
			total--;
		} else if (estrncmp(gate, x_("nor"), 3) == 0 && isdigit(gate[3]))
		{
			vhdl_genfunction(x_("or"), FALSE, TRUE, eatoi(&gate[3]));
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("xor2")) == 0)
		{
			vhdl_dumpfunction(gate, xor2);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("power")) == 0)
		{
			vhdl_dumpfunction(gate, power);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("ground")) == 0)
		{
			vhdl_dumpfunction(gate, ground);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("jkff")) == 0)
		{
			vhdl_dumpfunction(gate, JKFF);
			ulist->numref = 0;
			total--;
		} else if (estrcmp(gate, x_("dsff")) == 0)
		{
			vhdl_dumpfunction(gate, DFF);
			ulist->numref = 0;
			total--;
		}
	}

	/* print unresolved reference list if not empty */
	if (total > 0)
	{
		ttyputmsg(_("*****  UNRESOLVED REFERENCES *****"));
		for (ulist = vhdl_unresolved_list; ulist != NULL; ulist = ulist->next)
			if (ulist->numref > 0)
				ttyputmsg(_("%s, %ld time(s)"), ulist->interfacef->string, ulist->numref);
	}
}

/*
 * routine to search library "lib" for a netlist that matches "name".  If found,
 * add it to the current output netlist and return nonzero.  If not found, return false.
 */
BOOLEAN vhdl_addnetlist(LIBRARY *lib, CHAR *name)
{
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pn, *key;
	REGISTER VARIABLE *var;
	REGISTER INTBIG len, i;
	REGISTER void *infstr;

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (np->cellview != el_netlistalsview) continue;
		infstr = initinfstr();
		for(pn = np->protoname; *pn != 0; pn++)
			if (isalnum(*pn)) addtoinfstr(infstr, *pn); else
				addtoinfstr(infstr, '_');
		key = returninfstr(infstr);
		if (namesame(name, key) != 0) continue;

		/* add it to the netlist */
		var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING|VISARRAY, el_cell_message_key);
		if (var == NOVARIABLE) continue;
		len = getlength(var);
		vhdl_print(x_(""));
		for(i=0; i<len; i++) vhdl_printoneline(((CHAR **)var->addr)[i]);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * inputs cap = 0.xx pF, Tphl = T1 + T2*Cin, Tplh = T3 + T4*Cin
 */
void vhdl_genfunction(CHAR *name, BOOLEAN isand, BOOLEAN isneg, INTBIG inputs)
{
	CHAR modelname[20], line[20];
	REGISTER INTBIG i;
	REGISTER void *infstr;
	Q_UNUSED( name );

	/* generate model name */
	if (!isneg) modelname[0] = 0; else (void)estrcpy(modelname, x_("n"));
	if (isand) (void)estrcat(modelname, x_("and")); else (void)estrcat(modelname, x_("or"));
	(void)esnprintf(line, 20, x_("%ld"), inputs);
	(void)estrcat(modelname, line);

	vhdl_print(x_(""));
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("# Built-in model for "));
	addstringtoinfstr(infstr, modelname);
	vhdl_printoneline(returninfstr(infstr));

	/* write header line */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("model "));
	addstringtoinfstr(infstr, modelname);
	addstringtoinfstr(infstr, x_("("));
	for(i=1; i<=inputs; i++)
	{
		if (i > 1) addstringtoinfstr(infstr, x_(","));
		(void)esnprintf(line, 20, x_("a%ld"), i);
		addstringtoinfstr(infstr, line);
	}
	addstringtoinfstr(infstr, x_(",z)"));
	vhdl_print(returninfstr(infstr));

	/* write function line */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("g1: "));
	addstringtoinfstr(infstr, modelname);
	addstringtoinfstr(infstr, x_("fun("));
	for(i=1; i<=inputs; i++)
	{
		if (i > 1) addstringtoinfstr(infstr, x_(","));
		(void)esnprintf(line, 20, x_("a%ld"), i);
		addstringtoinfstr(infstr, line);
	}
	if (!isneg) addstringtoinfstr(infstr, x_(",out)")); else
		addstringtoinfstr(infstr, x_(",z)"));
	vhdl_print(returninfstr(infstr));

	/* write buffer line if not negated */
	if (!isneg)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("g2: "));
		addstringtoinfstr(infstr, modelname);
		addstringtoinfstr(infstr, x_("buf(out,z)"));
		vhdl_print(returninfstr(infstr));
	}

	/* write function header */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("gate "));
	addstringtoinfstr(infstr, modelname);
	addstringtoinfstr(infstr, x_("fun("));
	for(i=1; i<=inputs; i++)
	{
		if (i > 1) addstringtoinfstr(infstr, x_(","));
		(void)esnprintf(line, 20, x_("a%ld"), i);
		addstringtoinfstr(infstr, line);
	}
	addstringtoinfstr(infstr, x_(",z)"));
	vhdl_print(returninfstr(infstr));
	vhdl_print(x_("t: delta=1.33e-9"));		/* T3=1.33 */
	for(i=1; i<=inputs; i++)
	{
		infstr = initinfstr();
		(void)esnprintf(line, 20, x_("i: a%ld"), i);
		addstringtoinfstr(infstr, line);
		if (isand) addstringtoinfstr(infstr, x_("=L o: z=H")); else
			addstringtoinfstr(infstr, x_("=H o: z=L"));
		vhdl_print(returninfstr(infstr));
	}
	vhdl_print(x_("t: delta=1.07e-9"));		/* T1=1.07 */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("i:"));
	for(i=1; i<=inputs; i++)
	{
		if (isand) (void)esnprintf(line, 20, x_(" a%ld=H"), i); else
			(void)esnprintf(line, 20, x_(" a%ld=L"), i);
		addstringtoinfstr(infstr, line);
	}
	if (isand) addstringtoinfstr(infstr, x_(" o: z=L")); else
		addstringtoinfstr(infstr, x_(" o: z=H"));
	vhdl_print(returninfstr(infstr));
	vhdl_print(x_("t: delta=0"));
	vhdl_print(x_("i: o: z=X"));

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("load:"));
	for(i=1; i<=inputs; i++)
	{
		(void)esnprintf(line, 20, x_(" a%ld=1.0"), i);
		addstringtoinfstr(infstr, line);
	}
	vhdl_print(returninfstr(infstr));

	/* write buffer gate if not negated */
	if (!isneg)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("gate "));
		addstringtoinfstr(infstr, modelname);
		addstringtoinfstr(infstr, x_("buf(in,out)"));
		vhdl_print(returninfstr(infstr));
		vhdl_print(x_("t: delta=0.56e-9"));		/* T4*Cin=0.56 */
		vhdl_print(x_("i: in=H    o: out=L"));
		vhdl_print(x_("t: delta=0.41e-9"));		/* T2*Cin=0.41 */
		vhdl_print(x_("i: in=L    o: out=H"));
		vhdl_print(x_("t: delta=0"));
		vhdl_print(x_("i: in=X    o: out=X"));
	}
}

void vhdl_dumpfunction(CHAR *name, CHAR **model)
{
	REGISTER INTBIG i;
	REGISTER void *infstr;

	vhdl_print(x_(""));
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("# Built-in model for "));
	addstringtoinfstr(infstr, name);
	vhdl_printoneline(returninfstr(infstr));
	for(i=0; model[i] != 0; i++) vhdl_printoneline(model[i]);
}

/*
Module:  vhdl_genals_interface
------------------------------------------------------------------------
Description:
	Recursively generate the ALS description for the specified model
	by first generating the lowest interface instantiation and working
	back to the top (i.e. bottom up).
------------------------------------------------------------------------
Calling Sequence:  vhdl_genals_interface(interfacef, name);

Name		Type			Description
----		----			-----------
interfacef	*DBINTERFACE	Pointer to interface.
name		*char			Pointer to string name of interface.
------------------------------------------------------------------------
*/
void vhdl_genals_interface(DBINTERFACE *interfacef, CHAR *name)
{
	DBINSTANCE *inst;
	SYMBOLTREE *symbol;
	DBPORTLIST *port;
	DBAPORTLIST *aport;
	INTBIG first, generic, power_flag, ground_flag, i;
	CHAR temp[30], *pt;
	IDENTTABLE *ident;
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	DBNAMELIST *cat;
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
						ttyputmsg(_("WARNING - interface %s not found, assumed external."),
							inst->compo->name->string);
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
				vhdl_genals_interface((DBINTERFACE *)symbol->pointer, inst->compo->name->string);
			}
		}
	}

	/* write this interface */
	generic = 0;
	power_flag = ground_flag = FALSE;
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("model "));
	for(pt = name; *pt != 0; pt++)
		if (isalnum(*pt)) addtoinfstr(infstr, *pt); else
	{
		addtoinfstr(infstr, '_');
	}
	addstringtoinfstr(infstr, x_("("));

	/* write port list of interface */
	first = TRUE;
	for (port = interfacef->ports; port != NULL; port = port->next)
	{
		if (port->type == NULL || port->type->type == DBTYPE_SINGLE)
		{
			if (!first) addstringtoinfstr(infstr, x_(", "));
			addstringtoinfstr(infstr, port->name->string);
			first = FALSE;
		} else
		{
			irange = (DBINDEXRANGE *)port->type->pointer;
			drange = irange->drange;
			if (drange->start > drange->end)
			{
				for (i = drange->start; i >= drange->end; i--)
				{
					if (!first) addstringtoinfstr(infstr, x_(", "));
					addstringtoinfstr(infstr, port->name->string);
					(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
					first = FALSE;
				}
			} else
			{
				for (i = drange->start; i <= drange->end; i++)
				{
					if (!first) addstringtoinfstr(infstr, x_(", "));
					addstringtoinfstr(infstr, port->name->string);
					(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
					first = FALSE;
				}
			}
		}
	}
	addstringtoinfstr(infstr, x_(")"));
	vhdl_print(returninfstr(infstr));

	/* write all instances */
	if (interfacef->bodies && interfacef->bodies->statements)
	{
		for (inst = interfacef->bodies->statements->instances; inst != NULL;
			inst = inst->next)
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, inst->name->string);
			addstringtoinfstr(infstr, x_(": "));
			addstringtoinfstr(infstr, inst->compo->name->string);
			addstringtoinfstr(infstr, x_("("));

			/* print instance port list */
			first = TRUE;
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
							vhdl_genals_aport(infstr, (INTBIG *)&first, cat->name);
						}
					} else
					{
						ident = aport->name->name;
						if (ident == vhdl_ident_power) power_flag = TRUE; else
							if (ident == vhdl_ident_ground) ground_flag = TRUE;
						vhdl_genals_aport(infstr, (INTBIG *)&first, aport->name);
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
							{
								if (!first) addstringtoinfstr(infstr, x_(", "));
								(void)esnprintf(temp, 30, x_("n%ld"), generic++);
								addstringtoinfstr(infstr, temp);
								first = FALSE;
							}
						} else
						{
							for (i = drange->start; i <= drange->end; i++)
							{
								if (!first) addstringtoinfstr(infstr, x_(", "));
								(void)esnprintf(temp, 30, x_("n%ld"), generic++);
								addstringtoinfstr(infstr, temp);
								first = FALSE;
							}
						}
					} else
					{
						if (!first) addstringtoinfstr(infstr, x_(", "));
						(void)esnprintf(temp, 30, x_("n%ld"), generic++);
						addstringtoinfstr(infstr, temp);
						first = FALSE;
					}
				}
			}
			addstringtoinfstr(infstr, x_(")"));
			vhdl_print(returninfstr(infstr));
		}
	}

	/* check for power and ground flags */
	if (power_flag) vhdl_print(x_("set power = H@3"));
		else if (ground_flag) vhdl_print(x_("set ground = L@3"));
	vhdl_print(x_(""));
}

/*
Module:  vhdl_genals_aport
------------------------------------------------------------------------
Description:
	Add the actual port for a single name to the infinite string.
------------------------------------------------------------------------
Calling Sequence:  vhdl_genals_aport(first, name);

Name		Type		Description
----		----		-----------
first		*INTBIG		Addres of first flag.
name		*DBNAME		Pointer to single name.
------------------------------------------------------------------------
*/
void vhdl_genals_aport(void *infstr, INTBIG *first, DBNAME *name)
{
	DBINDEXRANGE *irange;
	DBDISCRETERANGE *drange;
	INTBIG i;
	CHAR temp[30];

	if (name->type == DBNAME_INDEXED)
	{
		if (!(*first)) addstringtoinfstr(infstr, x_(", "));
		addstringtoinfstr(infstr, name->name->string);
		(void)esnprintf(temp, 30, x_("[%ld]"), ((DBEXPRLIST *)(name->pointer))->value);
		addstringtoinfstr(infstr, temp);
		*first = FALSE;
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
					if (!(*first)) addstringtoinfstr(infstr, x_(", "));
					addstringtoinfstr(infstr, name->name->string);
					(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
					*first = FALSE;
				}
			} else
			{
				for (i = drange->start; i <= drange->end; i++)
				{
					if (!(*first)) addstringtoinfstr(infstr, x_(", "));
					addstringtoinfstr(infstr, name->name->string);
					(void)esnprintf(temp, 30, x_("[%ld]"), i);
					addstringtoinfstr(infstr, temp);
					*first = FALSE;
				}
			}
		} else
		{
			if (!(*first)) addstringtoinfstr(infstr, x_(", "));
			addstringtoinfstr(infstr, name->name->string);
			*first = FALSE;
		}
	}
}

#endif  /* VHDLTOOL - at top */
