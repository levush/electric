/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: network.cpp
 * Network tool: module for maintenance of connectivity information
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
#include "network.h"
#include "database.h"
#include "egraphics.h"
#include "sim.h"
#include "tecschem.h"
#include "efunction.h"
#include "edialogs.h"
#include "usr.h"
#include "usrdiacom.h"
#include <math.h>
#include <ctype.h>

/* variables */
       TOOL       *net_tool;					/* the Network tool object */
static TOOL       *net_current_source;			/* the tool currently making changes */
static BOOLEAN     net_debug;					/* true for debugging */
static BOOLEAN     net_globalwork;				/* nonzero when doing major evaluation */
static INTBIG      net_optionskey;				/* key for "NET_options" */
       INTBIG      net_node_abbrev_key;			/* key for "NET_node_abbreviations" */
       INTBIG      net_node_abbrevlen_key;		/* key for "NET_node_abbreviation_length" */
       INTBIG      net_unifystringskey;			/* key for "NET_unify_strings" */
       INTBIG      net_options;					/* cached value for "NET_options" */
       INTBIG      net_ncc_optionskey;			/* key for "NET_ncc_options" */
       INTBIG      net_ncc_comptolerancekey;	/* key for "NET_ncc_component_tolerance" */
       INTBIG      net_ncc_comptoleranceamtkey;	/* key for "NET_ncc_component_tolerance_amt" */
       INTBIG      net_ncc_matchkey;			/* key for "NET_ncc_match" */
       INTBIG      net_ncc_forcedassociationkey;/* key for "NET_ncc_forcedassociation" */
       INTBIG      net_ncc_processors_key;		/* key for "NET_ncc_num_processors" */
       INTBIG      net_ncc_function_key;		/* key for "NET_ncc_function" */
       PNET       *net_pnetfree;
       PCOMP      *net_pcompfree;
static INTBIG      net_mostj = 0;
static CHAR      **net_unifystrings;			/* list of string prefixes for unifying networks */
static INTBIG      net_unifystringcount;		/* number of strings in "net_unifystrings" */
static NETWORK   **net_primnetlist;
static void       *net_merge;					/* for polygon merging */
static BOOLEAN     net_checkresistorstate;		/* true to check for changes to resistor topology */

/* obsolete variables */
static INTBIG      net_connect_power_groundkey;	/* key for "NET_connect_PandG" */
static INTBIG      net_connect_common_namekey;	/* key for "NET_connect_common" */
static INTBIG      net_found_obsolete_variables = 0;	/* nonzero if library had above variables */

#define NUMBUSSTRINGBUFFERS 2
static INTBIG      net_busbufstringbufferpos = -1;
static CHAR      **net_busbufstringsarray[NUMBUSSTRINGBUFFERS];
static INTBIG      net_busbufstringcountarray[NUMBUSSTRINGBUFFERS];

       TRANSISTORINFO   net_transistor_p_gate;		/* info on P transistors connected at gate */
       TRANSISTORINFO   net_transistor_n_gate;		/* info on N transistors connected at gate */
       TRANSISTORINFO   net_transistor_p_active;	/* info on P transistors connected at active */
       TRANSISTORINFO   net_transistor_n_active;	/* info on N transistors connected at active */

/* working memory for "net_samenetworkname()" */
static INTBIG     net_namecompstringtotal = 0;
static CHAR     **net_namecompstrings;

/* working memory for "net_nconnect()" */
static CHAR      *net_arrayedarcname = 0;

/* working memory for "net_addnodename()" */
static INTBIG     net_nodenamelisttotal = 0;
static INTBIG     net_nodenamelistcount;
static CHAR     **net_nodenamelist;

/* working memory for "net_addnettolist()" */
static INTBIG    net_highnetscount;
static INTBIG    net_highnetstotal = 0;
static NETWORK **net_highnets;

static AREAPERIM *net_firstareaperim;

typedef struct Ibuslist
{
	ARCINST         *ai;
	PORTPROTO       *pp;
	INTBIG           width;
} BUSLIST;

static BUSLIST *net_buslists;
static INTBIG   net_buslistcount;
static INTBIG   net_buslisttotal = 0;


#define NOARCCHECK ((ARCCHECK *)-1)

typedef struct Iarccheck
{
	ARCINST *ai;
	struct Iarccheck *nextarccheck;
} ARCCHECK;

ARCCHECK *net_arccheckfree = NOARCCHECK;
ARCCHECK *net_firstarccheck;


#define NOCLEARNCC ((CLEARNCC *)-1)

typedef struct Iclearncc
{
	NODEPROTO *np;
	struct Iclearncc *nextclearncc;
} CLEARNCC;

CLEARNCC *net_clearnccfree = NOCLEARNCC;
CLEARNCC *net_firstclearncc;

/*********************** CELL CONNECTIONS ***********************/

#ifdef NEWRENUM
class NetCellConns;

struct NetDrawn
{
	INTBIG _shallowRoot;
	INTBIG _busWidth;
	NETWORK *_network;
	INTBIG _deepbase;
	BOOLEAN _isBus;
};

class NetCellConns
{
public:
	void* operator new( size_t size );
	void* operator new( size_t size, CLUSTER *cluster );
	void operator delete( void* obj );
#ifndef MACOS
	void operator delete( void *obj, CLUSTER *cluster );
#endif
	NetCellConns( NODEPROTO *np, CLUSTER *cluster );
	~NetCellConns();
	BOOLEAN complicated() { return _complicated; };
	void compare();
	void deepcompare();
	void schem();
	void makeNetworksSimple();
	void printf();
private:
	void shallowconnect( INTBIG a1, INTBIG a2 );
	void shallowfixup();
	INTBIG addGlobalname(CHAR *name);
	void makeDrawns();
	void deepconnectport( INTBIG drawnIndex, NODEINST *ni, NetCellShorts *shorts, INTBIG *pinmap, PORTPROTO *pp);
	void deepconnect( INTBIG a1, INTBIG a2 );
	void deepjoin( NODEINST *ni );
	void deepfixup();
	NODEPROTO *_np;
	INTBIG _portcount;
	INTBIG _arccount;
	INTBIG *_shallowmap;
	INTBIG _globalcount;
	CHAR **_globalnames;
	NetDrawn **_shallow2drawn;
	INTBIG _drawncount;
	NetDrawn *_drawns;
	INTBIG _deepmapcount;
	INTBIG *_deepmap;
	BOOLEAN _complicated;
};
#endif

/*********************** COMMAND PARSING ***********************/

#ifndef ALLCPLUSPLUS
extern "C"
{
#endif

static COMCOMP net_networkcellp = {NOKEYWORD,topofcells,nextcells,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Cell to re-number (default is current cell)"), 0};
static COMCOMP networknodehp = {NOKEYWORD, topofnets, nextnets, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Net to highlight"), 0};
static COMCOMP networknodenp = {NOKEYWORD, topofnets, nextnets, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Net whose connections should be listed"), 0};
static COMCOMP networknodelp = {NOKEYWORD, topofnets, nextnets, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Net, ALL of whose ports should be listed"), 0};
static KEYWORD networkeqnopt[] =
{
	{x_("include-no-component-nets"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-export-names"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-node-sizes"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("flatten-hierarchy"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("recurse"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP networkeqnp = {networkeqnopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Network-compare negating option"), 0};
static KEYWORD networkeqopt[] =
{
	{x_("flatten-hierarchy"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("include-no-component-nets"),     0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("recurse"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("highlight-other"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-export-names"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-node-sizes"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pre-analysis"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
#ifdef FORCESUNTOOLS
	{x_("analyze-cell"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
#endif
	{x_("not"),                           1,{&networkeqnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP net_networkeqp = {networkeqopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Network comparing/equating option"), M_("do comparison")};
static KEYWORD networkpgopt[] =
{
	{x_("unify-all-networks"),            0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("separate-unconnected-networks"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("identify"),                      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("validate"),                      0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP networkpgp = {networkpgopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Network power and ground equating option"), 0};
static KEYWORD networkcnopt[] =
{
	{x_("unify-always"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("unify-only-in-schematics"), 0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP networkcnp = {networkcnopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("How to handle networks with the same name"), 0};
static KEYWORD networkunopt[] =
{
	{x_("ascend-numbering"),              0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("descend-numbering"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("0-base"),                        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("1-base"),                        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP networkunp = {networkunopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Option to handle unnamed busses"), 0};
static KEYWORD networkropt[] =
{
	{x_("ignore"),                        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("include"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP networkrp = {networkropt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Option to handle reistors"), 0};
static KEYWORD networkopt[] =
{
	{x_("highlight"),                     1,{&networknodehp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-equivalent"),               1,{&networknodehp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name-connections"),              1,{&networknodenp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name-cell-objects"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name-library-objects"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("show-geometry"),                 1,{&networknodenp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("list-hierarchical-ports"),       1,{&networknodelp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("list-ports-below"),              1,{&networknodelp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("compare"),                       1,{&net_networkeqp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("power-and-ground"),              1,{&networkpgp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("like-named-nets"),               1,{&networkcnp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("re-number"),                     1,{&net_networkcellp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("default-busses"),                1,{&networkunp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("resistors"),                     1,{&networkrp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("total-re-number"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("rip-bus"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("debug-toggle"),                  0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("extract"),                       0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP net_networkp = {networkopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Network maintenance"), 0};

#ifndef ALLCPLUSPLUS
}
#endif

/* prototypes for local routines */
static NETWORK         *net_addglobalnet(NODEPROTO *subcell, INTBIG subindex, NODEPROTO *cell);
static INTBIG           net_addnametonet(CHAR*, INTBIG, NETWORK*, ARCINST*, PORTPROTO*, NODEPROTO*);
static void             net_addnettolist(NETWORK *net);
static BOOLEAN          net_addnodename(CHAR *name);
static void             net_addstring(CHAR*, INTBIG*, INTBIG*, CHAR***);
static void             net_addtobuslist(ARCINST *ai, PORTPROTO *pp, INTBIG width);
static void             net_addtotransistorinfo(TRANSISTORINFO *ti, INTBIG length, INTBIG width);
static ARCCHECK        *net_allocarccheck(void);
static CLEARNCC        *net_allocclearncc(void);
static int              net_buslistwidthascending(const void *e1, const void *e2);
static CHAR            *net_busnameofarc(ARCINST *ai);
static INTBIG           net_buswidthofarc(ARCINST *ai, NODEINST **ni, PORTPROTO **pp);
static void             net_checknetname(NETWORK *net, NetName *nn);
static void             net_checkvalidconnection(NODEINST*, ARCINST*);
static void             net_cleartransistorinfo(TRANSISTORINFO *ti);
static BOOLEAN          net_donconnect(ARCINST *ai, NETWORK *newnetwork, NODEPROTO *np);
#ifdef NEWRENUM
static void             net_ensurebusses(NETWORK*, NetName*, INTBIG);
#else
static void             net_ensurebusses(NETWORK*, INTBIG, CHAR**, INTBIG);
#endif
static void             net_ensuretempbusname(NETWORK *net);
static CHAR            *net_findnameinbus(CHAR *wantname, CHAR *busname);
static CHAR            *net_findnameofbus(ARCINST *ai, BOOLEAN justbus);
static void             net_findportsdown(NETWORK*, NODEPROTO*);
static void             net_findportsup(NETWORK*, NODEPROTO*);
static void             net_freearccheck(ARCCHECK *ac);
static void             net_freeclearncc(CLEARNCC *cn);
static void             net_geometrypolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count);
static NETWORK         *net_getunifiednetwork(CHAR *name, NODEPROTO *cell, NETWORK *notthisnet);
static void             net_highlightnet(void*, NODEPROTO*, NETWORK*);
static void             net_highlightsubnet(void *infstr, NODEPROTO *topnp, NODEINST *ni, XARRAY trans, NETWORK *net);
static void             net_initnconnect(NODEPROTO *np);
static void             net_insertstring(CHAR *key, INTBIG index, INTBIG *count, INTBIG *stringcount, CHAR ***mystrings);
static BOOLEAN          net_isanumber(CHAR *name);
static void             net_joinnetworks(NODEINST *ni);
static void             net_killnetwork(NETWORK*, NODEPROTO*);
static BOOLEAN          net_mergebuswires(NODEINST*);
static BOOLEAN          net_mergenet(NETWORK*, NETWORK*);
static INTBIG           net_nameallnets(NODEPROTO *np);
static INTBIG           net_nameallnodes(NODEPROTO *np, BOOLEAN evenpins);
#ifdef NEWRENUM
static BOOLEAN          net_namenet(NetName*, NETWORK*);
#else
static BOOLEAN          net_namenet(CHAR*, NETWORK*);
#endif
static void             net_ncccellfunctiondlog(void);
static NODEPROTO       *net_nccdlgselectedcell(void *dia, INTBIG liblist, INTBIG celllist);
static void             net_nccoptionsdlog(void);
static BOOLEAN          net_nconnect(ARCINST*, NETWORK*, NODEPROTO*);
#ifdef NEWRENUM
static BOOLEAN          net_nethasname(NETWORK *net, NetName *netname);
#else
static BOOLEAN          net_nethasname(NETWORK *net, CHAR *name);
#endif
static NETWORK         *net_newnetwork(NODEPROTO*);
static CHAR            *net_nextcells(void);
static CHAR            *net_nextcellswithfunction(void);
static void             net_optionsdlog(void);
static NETWORK        **net_parsenetwork(CHAR*);
static BOOLEAN          net_pconnect(PORTPROTO*);
static void             net_propgeometry(NODEPROTO *cell, XARRAY trans, BOOLEAN recurse);
static void             net_putarclinkonnet(NETWORK*, ARCINST*);
static void             net_putarconnet(ARCINST*, NETWORK*);
static void             net_putportonnet(PORTPROTO*, NETWORK*);
static void             net_queueclearncc(NODEPROTO *np);
static void             net_recacheunifystrings(void);
static void             net_recursivelymarkabove(NODEPROTO*);
static BOOLEAN          net_recursivelyredo(NODEPROTO*);
static void             net_reevaluatecell(NODEPROTO*);
static void             net_removebuslinks(NETWORK*);
static void             net_removeclearncc(NODEPROTO *np);
static void             net_renamenet(CHAR*, CHAR*, NODEPROTO*);
static void             net_ripbus(void);
static void             net_setglobalnet(INTBIG *special, INTBIG newindex, CHAR *netname, NETWORK *net,
							INTBIG characteristics, NODEPROTO *cell);
static void             net_setnccoverrides(void *dia);
static void             net_showgeometry(NETWORK *net);
static void             net_startglobalwork(LIBRARY*);
static BOOLEAN          net_takearcfromnet(ARCINST*);
static void             net_takearclinkfromnet(ARCINST*, NETWORK*);
static BOOLEAN          net_takeportfromnet(PORTPROTO*);
static void             net_tellschematics(void);
static BOOLEAN          net_topofcells(CHAR **c);
static void             net_totalrenumber(LIBRARY*);
static INTBIG           net_nccdebuggingdlog(INTBIG options);
static void             net_listnccforcedmatches(void);
static void             net_removenccforcedmatches(void);

/*********************** DATABASE INTERFACE ROUTINES ***********************/

void net_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	Q_UNUSED( argc );
	Q_UNUSED( argv );

	/* ignore pass 3 initialization */
	if (thistool == 0) return;

	/* take tool pointer in pass 1 */
	if (thistool != NOTOOL)
	{
		net_tool = thistool;
		return;
	}

	/* initialize flattened network representation */
	net_pcompfree = NOPCOMP;
	net_pnetfree = NOPNET;

	/* debugging off */
	net_debug = FALSE;

	/* set network options */
	net_optionskey = makekey(x_("NET_options"));
	nextchangequiet();
	net_options = NETDEFBUSBASEDESC;
	(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey, net_options, VINTEGER|VDONTSAVE);
	net_node_abbrev_key = makekey(x_("NET_node_abbreviations"));
	net_node_abbrevlen_key = makekey(x_("NET_node_abbreviation_length"));
	net_unifystringskey = makekey(x_("NET_unify_strings"));
	net_checkresistorstate = FALSE;
	net_unifystringcount = 0;

	/* get obsolete variable names */
	net_connect_power_groundkey = makekey(x_("NET_connect_PandG"));
	net_connect_common_namekey = makekey(x_("NET_connect_common"));

	/* set NCC options */
	net_ncc_optionskey = makekey(x_("NET_ncc_options"));
	nextchangequiet();
	(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
		NCCNOMERGEPARALLEL|NCCDISLOCAFTERMATCH, VINTEGER|VDONTSAVE);
	net_ncc_comptolerancekey = makekey(x_("NET_ncc_component_tolerance"));
	net_ncc_comptoleranceamtkey = makekey(x_("NET_ncc_component_tolerance_amt"));
	net_ncc_matchkey = makekey(x_("NET_ncc_match"));
	net_ncc_forcedassociationkey = makekey(x_("NET_ncc_forcedassociation"));
	net_ncc_function_key = makekey(x_("NET_ncc_function"));
	net_firstclearncc = NOCLEARNCC;

	/* node-number primitive ports (now and when technologies change) */
	registertechnologycache(net_redoprim, 0, 0);

	/* register options dialog */
	DiaDeclareHook(x_("nccopt"), &net_networkeqp, net_nccoptionsdlog);
	DiaDeclareHook(x_("netopt"), &net_networkp, net_optionsdlog);
	DiaDeclareHook(x_("netcelfun"), &net_networkcellp, net_ncccellfunctiondlog);
}

void net_done(void)
{
#ifdef DEBUGMEMORY
	REGISTER INTBIG i, j, stringcount;
	REGISTER CHAR **mystrings;
	REGISTER ARCCHECK *ac;
	REGISTER CLEARNCC *cn;

	if (net_mostj != 0)
	{
		for(j=0; j<net_mostj; j++) efree((CHAR *)net_primnetlist[j]);
		efree((CHAR *)net_primnetlist);
		net_mostj = 0;
	}
	if (net_unifystringcount > 0)
	{
		for(j=0; j<net_unifystringcount; j++) efree((CHAR *)net_unifystrings[j]);
		efree((CHAR *)net_unifystrings);
	}

	if (net_busbufstringbufferpos >= 0)
	{
		for(i=0; i<NUMBUSSTRINGBUFFERS; i++)
		{
			mystrings = net_busbufstringsarray[i];
			stringcount = net_busbufstringcountarray[i];
			for(j=0; j<stringcount; j++) efree((CHAR *)mystrings[j]);
			if (stringcount > 0) efree((CHAR *)mystrings);
		}
	}
	while (net_arccheckfree != NOARCCHECK)
	{
		ac = net_arccheckfree;
		net_arccheckfree = ac->nextarccheck;
		efree((CHAR *)ac);
	}
	while (net_clearnccfree != NOCLEARNCC)
	{
		cn = net_clearnccfree;
		net_clearnccfree = cn->nextclearncc;
		efree((CHAR *)cn);
	}

	if (net_buslisttotal > 0) efree((CHAR *)net_buslists);
	if (net_namecompstringtotal > 0) efree((CHAR *)net_namecompstrings);
	if (net_arrayedarcname != 0) efree(net_arrayedarcname);
	if (net_highnetstotal > 0) efree((CHAR *)net_highnets);
	if (net_nodenamelisttotal > 0)
	{
		for(i=0; i<net_nodenamelisttotal; i++)
			if (net_nodenamelist[i] != 0) efree(net_nodenamelist[i]);
		efree((CHAR *)net_nodenamelist);
	}
	net_freediffmemory();
	net_freeflatmemory();
#endif
}

/*
 * routine to associate the primitives ports in each technology with unique
 * network objects according to connectivity within the node
 */
void net_redoprim(void)
{
	REGISTER INTBIG j, maxj;
	REGISTER PORTPROTO *pp, *spt;
	REGISTER TECHNOLOGY *tech;
	REGISTER NODEPROTO *np;

	/* count the number of network objects that are needed */
	maxj = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if ((pp->userbits&PORTNET) == PORTNET)
			{
				maxj++;
				continue;
			}
			for(spt = np->firstportproto; spt != pp; spt = spt->nextportproto)
				if ((pp->userbits&PORTNET) == (spt->userbits&PORTNET))
			{
				pp->network = spt->network;
				break;
			}
			if (spt == pp) maxj++;
		}
	}

	/* create an array of network objects for the primitives */
	if (maxj > net_mostj)
	{
		if (net_mostj != 0)
		{
			for(j=0; j<net_mostj; j++) efree((CHAR *)net_primnetlist[j]);
			efree((CHAR *)net_primnetlist);
			net_mostj = 0;
		}
		net_primnetlist = (NETWORK **)emalloc(((sizeof (NETWORK *)) * maxj),
			net_tool->cluster);
		if (net_primnetlist == 0) return;
		for(j=0; j<maxj; j++)
		{
			net_primnetlist[j] = (NETWORK *)emalloc(sizeof (NETWORK), net_tool->cluster);
			if (net_primnetlist[j] == 0) return;
			net_primnetlist[j]->namecount = 0;
			net_primnetlist[j]->tempname = 0;
			net_primnetlist[j]->globalnet = -1;
			net_primnetlist[j]->buswidth = 1;
			net_primnetlist[j]->nextnetwork = NONETWORK;
			net_primnetlist[j]->prevnetwork = NONETWORK;
			net_primnetlist[j]->networklist = (NETWORK **)NONETWORK;
			net_primnetlist[j]->parent = NONODEPROTO;
			net_primnetlist[j]->netnameaddr = 0; /* var examine does not check namecount */
			net_primnetlist[j]->arccount = 0;
			net_primnetlist[j]->refcount = 0;
			net_primnetlist[j]->portcount = 0;
			net_primnetlist[j]->buslinkcount = 0;
			net_primnetlist[j]->numvar = 0;
		}
		net_mostj = maxj;
	}

	/* assign unique networks to unconnected primitive ports */
	j = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		for(np = tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			if ((pp->userbits&PORTNET) == PORTNET)
			{
				pp->network = net_primnetlist[j++];
				continue;
			}
			for(spt = np->firstportproto; spt != pp; spt = spt->nextportproto)
				if ((pp->userbits&PORTNET) == (spt->userbits&PORTNET))
			{
				pp->network = spt->network;
				break;
			}
			if (spt == pp) pp->network = net_primnetlist[j++];
		}
		if (!np->netd)
			np->netd = new (tech->cluster) NetCellPrivate( np, tech->cluster );
		np->netd->updateShorts();
	}
}

void net_slice(void)
{
	REGISTER VARIABLE *var;
	REGISTER CLEARNCC *cn, *nextcn;

	if (net_found_obsolete_variables != 0)
	{
		net_found_obsolete_variables = 0;
		var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_connect_power_groundkey);
		if (var != NOVARIABLE)
			(void)delvalkey((INTBIG)net_tool, VTOOL, net_connect_power_groundkey);
		var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_connect_common_namekey);
		if (var != NOVARIABLE)
			(void)delvalkey((INTBIG)net_tool, VTOOL, net_connect_common_namekey);
	}
	if (net_checkresistorstate)
	{
		net_checkresistorstate = FALSE;
		net_tellschematics();
	}

	/* clear NCC cache as requested */
	for(cn = net_firstclearncc; cn != NOCLEARNCC; cn = nextcn)
	{
		nextcn = cn->nextclearncc;
		net_nccremovematches(cn->np);
		net_freeclearncc(cn);
	}
	net_firstclearncc = NOCLEARNCC;
}

void net_examinenodeproto(NODEPROTO *np)
{
	net_reevaluatecell(np);
}

void net_set(INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp, *opp;
	REGISTER ARCINST *ai;
	REGISTER LIBRARY *lib;
	REGISTER NETWORK *net, **netlist;
	REGISTER INTBIG i, l, total, fun, tr, options, *highsiglist;
	BOOLEAN showrequest;
	REGISTER CHAR *pt;
	REGISTER VARIABLE *var;
	NETWORK *mynet[2];
	INTBIG x, y, highsigcount;
	REGISTER WINDOWPART *w;
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	static CHAR *message[1] = {x_("total-re-number")};
	REGISTER void *infstr;

	if (count == 0)
	{
		count = ttygetparam(x_("NETWORK option:"), &net_networkp, MAXPARS, par);
		if (count == 0)
		{
			ttyputerr(M_("Aborted"));
			return;
		}
	}
	l = estrlen(pt = par[0]);


	if (namesamen(pt, x_("rip-bus"), l) == 0 && l >= 2)
	{
		net_ripbus();
		return;
	}

	if (namesamen(pt, x_("highlight"), l) == 0 ||
		(namesamen(pt, x_("show-equivalent"), l) == 0 && l > 5))
	{
		/* if this was associated, show that */
		if (net_equate(0) == 0) return;

		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell"));
			return;
		}
		if (namesamen(pt, x_("show-equivalent"), l) == 0) showrequest = TRUE; else
			showrequest = FALSE;
		if (count < 2)
		{
			if (!showrequest)
			{
				netlist = net_gethighlightednets(FALSE);
				if (netlist[0] == NONETWORK) return;
			} else
			{
				netlist = net_gethighlightednets(showrequest);
				if (netlist[0] == NONETWORK) return;
			}
		} else
		{
			mynet[0] = getnetwork(par[1], np);
			if (mynet[0] == NONETWORK)
			{
				ttyputerr(_("No net called '%s'"), par[1]);
				return;
			}
			mynet[1] = NONETWORK;
			netlist = mynet;
		}

		/* first highlight the arcs on this network */
		if (!showrequest)
		{
			infstr = initinfstr();
			for(i=0; netlist[i] != NONETWORK; i++)
			{
				net = netlist[i];
				ttyputverbose(M_("Network '%s'"), describenetwork(net));
				np = net->parent;

				/* show the arcs on this network */
				net_highlightnet(infstr, np, net);

				/* handle multi-page schematic */
				if ((np->cellview->viewstate&MULTIPAGEVIEW) != 0)
				{
					/* search for an export on this net */
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						if (pp->network == net) break;
					if (pp != NOPORTPROTO)
					{
						/* search all other windows for another page of this schematic */
						for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
						{
							if (w->curnodeproto == NONODEPROTO) continue;
							if ((w->curnodeproto->cellview->viewstate&MULTIPAGEVIEW) == 0) continue;
							if (!insamecellgrp(w->curnodeproto, np)) continue;
							if (w->curnodeproto == np) continue;

							/* find any equivalent ports in this cell */
							opp = getportproto(w->curnodeproto, pp->protoname);
							if (opp == NOPORTPROTO) continue;

							/* show the arcs on this network */
							net_highlightnet(infstr, w->curnodeproto, opp->network);
						}
					}
				}
			}
			(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));
		}

		/* if there are associated VHDL or simulation windows, show in them */
		for(i=0; netlist[i] != NONETWORK; i++)
		{
			net = netlist[i];
			for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
			{
				if (w == el_curwindowpart) continue;
				switch (w->state&WINDOWTYPE)
				{
					case TEXTWINDOW:
					case POPTEXTWINDOW:
						/* must be editing this cell */
						if (w->curnodeproto == NONODEPROTO) break;
						if (!insamecellgrp(w->curnodeproto, np)) break;
						if (net->namecount > 0) pt = networkname(net, 0); else
							pt = describenetwork(net);
						if (w->curnodeproto->cellview == el_vhdlview ||
							w->curnodeproto->cellview == el_verilogview)
						{
							/* special intelligence for finding the net name in VHDL */
							infstr = initinfstr();
							addstringtoinfstr(infstr, x_("entity "));
							addstringtoinfstr(infstr, w->curnodeproto->protoname);
							addstringtoinfstr(infstr, x_(" is port("));
							us_searchtext(w, returninfstr(infstr), 0, 1);
							if (net->portcount <= 0)
								us_searchtext(w, x_("signal "), 0, 0);
							us_searchtext(w, pt, 0, 0);
						} else
						{
							/* just look for the name, starting at the top */
							us_searchtext(w, pt, 0, 1);
						}
						break;
#if SIMTOOL
					case WAVEFORMWINDOW:
						if (w->curnodeproto == NONODEPROTO) break;

						/* try converting this network to a HSPICE path */
						pt = sim_spice_signalname(net);
						highsiglist = sim_window_findtrace(pt, &highsigcount);
						if (highsigcount > 0)
						{
							sim_window_cleartracehighlight();
							for(l=0; l<highsigcount; l++)
								sim_window_addhighlighttrace(highsiglist[l]);
							sim_window_showhighlightedtraces();
							break;
						}

						/* must be simulating this cell */
						tr = 0;
						if (insamecellgrp(w->curnodeproto, np))
						{
							if (net->namecount > 0) pt = networkname(net, 0); else
								pt = describenetwork(net);
							highsiglist = sim_window_findtrace(pt, &highsigcount);
							if (highsigcount > 0) tr = highsiglist[0];
						}
						if (tr != 0)
						{
							sim_window_cleartracehighlight();
							sim_window_addhighlighttrace(tr);
							sim_window_showhighlightedtraces();
						}
						break;
#endif
				}
			}
		}
		return;
	}

	if (namesamen(pt, x_("show-geometry"), l) == 0 && l > 5)
	{
		if (count < 2)
		{
			netlist = net_gethighlightednets(FALSE);
			if (netlist[0] == NONETWORK) return;
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
			mynet[0] = getnetwork(par[1], np);
			if (mynet[0] == NONETWORK)
			{
				ttyputerr(M_("No net called '%s'"), par[1]);
				return;
			}
			mynet[1] = NONETWORK;
			netlist = mynet;
		}
		for(i=0; netlist[i] != NONETWORK; i++)
		{
			net = netlist[i];
			net_showgeometry(net);
		}
		return;
	}

	if (namesamen(pt, x_("extract"), l) == 0)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(_("No current cell to extract"));
			return;
		}
		net_conv_to_internal(np);
		return;
	}

	if (namesamen(pt, x_("default-busses"), l) == 0 && l > 3)
	{
		if (count == 1)
		{
			ttyputusage(x_("telltool network default-busses OPTION"));
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("ascend-numbering"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options & ~NETDEFBUSBASEDESC, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Default busses will be numbered in ascending order"));
			return;
		}
		if (namesamen(pt, x_("descend-numbering"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options|NETDEFBUSBASEDESC, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Default busses will be numbered in descending order"));
			return;
		}
		if (namesamen(pt, x_("0-base"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options & ~NETDEFBUSBASE1, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Default busses will be numbered starting at 0"));
			return;
		}
		if (namesamen(pt, x_("1-base"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options|NETDEFBUSBASE1, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Default busses will be numbered starting at 1"));
			return;
		}
		ttyputbadusage(x_("telltool network default-busses"));
		return;
	}

	if (namesamen(pt, x_("name-cell-objects"), l) == 0 && l >= 6)
	{
		np = getcurcell();
		if (np == NONODEPROTO)
		{
			ttyputerr(M_("No current cell"));
			return;
		}

		(void)net_nameallnodes(np, TRUE);
		(void)net_nameallnets(np);
		return;
	}

	if (namesamen(pt, x_("name-library-objects"), l) == 0 && l >= 6)
	{
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			(void)net_nameallnodes(np, TRUE);
			(void)net_nameallnets(np);
		}
		return;
	}

	if (namesamen(pt, x_("name-connections"), l) == 0 && l >= 6)
	{
		if (count < 2)
		{
			netlist = net_gethighlightednets(FALSE);
			if (netlist[0] == NONETWORK)
			{
				ttyputerr(_("Cannot determine network from highlighting"));
				return;
			}
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("No current cell"));
				return;
			}
			mynet[0] = getnetwork(par[1], np);
			if (mynet[0] == NONETWORK)
			{
				ttyputerr(M_("No net called '%s'"), par[1]);
				return;
			}
			mynet[1] = NONETWORK;
			netlist = mynet;
		}
		for(i=0; netlist[i] != NONETWORK; i++)
		{
			net = netlist[i];
			ttyputmsg(_("Network '%s':"), describenetwork(net));

			total = 0;
			for(ni = net->parent->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				np = ni->proto;
				if (np->primindex != 0)
				{
					opp = np->firstportproto;
					for(pp = opp->nextportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						if (pp->network != opp->network) break;
						opp = pp;
					}
					if (pp == NOPORTPROTO) continue;
				}

				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
					if (pi->conarcinst->network == net) break;
				if (pi == NOPORTARCINST) continue;

				if (total == 0) ttyputmsg(_("  Connects to:"));
				portposition(ni, pi->proto, &x, &y);
				ttyputmsg(_("    Node %s, port %s at (%s,%s)"), describenodeinst(ni),
					pi->proto->protoname, latoa(x, 0), latoa(y, 0));
				total++;
			}
			if (total == 0) ttyputmsg(_("  Not connected"));
		}
		return;
	}

	if (namesamen(pt, x_("power-and-ground"), l) == 0)
	{
		if (count == 1)
		{
			ttyputusage(x_("telltool network power-and-ground OPTION"));
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("unify-all-networks"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options|NETCONPWRGND, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Unconnected power and ground nets will be equated"));
			telltool(net_tool, 1, message);
			return;
		}
		if (namesamen(pt, x_("separate-unconnected-networks"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options & ~NETCONPWRGND, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Unconnected power and ground nets will not be equated"));
			telltool(net_tool, 1, message);
			return;
		}
		if (namesamen(pt, x_("identify"), l) == 0)
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				net->temp1 = 0;
			for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
			{
				if (portispower(pp) || portisground(pp))
					pp->network->temp1 = 1;
			}
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				if (ni->proto->primindex == 0) continue;
				fun = nodefunction(ni);
				if (fun != NPCONPOWER && fun != NPCONGROUND) continue;
				for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				{
					ai = pi->conarcinst;
					ai->network->temp1 = 1;
				}
			}
			infstr = initinfstr();
			total = 0;
			for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			{
				if (net->temp1 == 0) continue;
				net_highlightnet(infstr, np, net);
				total++;
			}
			(void)asktool(us_tool, x_("show-multiple"), (INTBIG)returninfstr(infstr));

			if (total == 0)
				ttyputmsg(_("This cell has no Power or Ground networks"));
			return;
		}
		if (namesamen(pt, x_("validate"), l) == 0)
		{
			ttyputmsg(_("Validating power and ground networks"));
			total = 0;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
					{
						if (portisnamedground(pp) && (pp->userbits&STATEBITS) != GNDPORT)
						{
							ttyputmsg(_("Cell %s, export %s: does not have 'GROUND' characteristic"),
								describenodeproto(np), pp->protoname);
#if 0
							pp->userbits = (pp->userbits & ~STATEBITS) | GNDPORT;
							lib->userbits |= LIBCHANGEDMAJOR;
#endif
							total++;
						}
						if (portisnamedpower(pp) && (pp->userbits&STATEBITS) != PWRPORT)
						{
							ttyputmsg(_("Cell %s, export %s: does not have 'POWER' characteristic"),
								describenodeproto(np), pp->protoname);
#if 0
							pp->userbits = (pp->userbits & ~STATEBITS) | PWRPORT;
							lib->userbits |= LIBCHANGEDMAJOR;
#endif
							total++;
						}
					}
				}
			}
			if (total == 0) ttyputmsg(_("No problems found")); else
				ttyputmsg(_("Found %ld export problems"), total);
			return;
		}

		ttyputbadusage(x_("telltool network power-and-ground"));
		return;
	}

	if (namesamen(pt, x_("like-named-nets"), l) == 0)
	{
		if (count == 1)
		{
			ttyputusage(x_("telltool network like-named-nets OPTION"));
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("unify-always"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options|NETCONCOMMONNAME, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Nets with the same name will always be equated"));
			telltool(net_tool, 1, message);
			return;
		}
		if (namesamen(pt, x_("unify-only-in-schematics"), l) == 0)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options & ~NETCONCOMMONNAME, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Nets with the same name will be equated only in schematics"));
			telltool(net_tool, 1, message);
			return;
		}
		ttyputbadusage(x_("telltool network like-named-nets"));
		return;
	}

	if (namesamen(pt, x_("compare"), l) == 0)
	{
		if (count == 1)
		{
			net_compare(FALSE, TRUE, NONODEPROTO, NONODEPROTO);
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("highlight-other"), l) == 0)
		{
			net_equate(1);
			return;
		}
		if (namesamen(pt, x_("pre-analysis"), l) == 0)
		{
			net_compare(TRUE, TRUE, NONODEPROTO, NONODEPROTO);
			return;
		}
#ifdef FORCESUNTOOLS
		if (namesamen(pt, x_("analyze-cell"), l) == 0)
		{
			(void)net_analyzecell();
			return;
		}
#endif
		if (namesamen(pt, x_("not"), l) == 0)
		{
			if (count <= 2)
			{
				ttyputusage(x_("telltool network compare not OPTION"));
				return;
			}
			l = estrlen(pt = par[2]);
			if (namesamen(pt, x_("include-no-component-nets"), l) == 0)
			{
				var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
				if (var == NOVARIABLE) options = 0; else options = var->addr;
				(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
					options & ~NCCINCLUDENOCOMPNETS, VINTEGER|VDONTSAVE);
				ttyputverbose(M_("Circuit comparison will include networks with no components on them"));
				return;
			}
			if (namesamen(pt, x_("check-export-names"), l) == 0 && l >= 7)
			{
				var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
				if (var == NOVARIABLE) options = 0; else options = var->addr;
				(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
					options & ~NCCCHECKEXPORTNAMES, VINTEGER|VDONTSAVE);
				ttyputverbose(M_("Circuit comparison will not check export names"));
				return;
			}
			if (namesamen(pt, x_("check-node-sizes"), l) == 0 && l >= 7)
			{
				var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
				if (var == NOVARIABLE) options = 0; else options = var->addr;
				(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
					options & ~NCCCHECKSIZE, VINTEGER|VDONTSAVE);
				ttyputverbose(M_("Circuit comparison will not check node sizes"));
				return;
			}
			if (namesamen(pt, x_("flatten-hierarchy"), l) == 0)
			{
				var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
				if (var == NOVARIABLE) options = 0; else options = var->addr;
				(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
					options & ~NCCHIERARCHICAL, VINTEGER|VDONTSAVE);
				ttyputverbose(M_("Circuits will be compared without flattening hierarchy"));
				return;
			}
			if (namesamen(pt, x_("recurse"), l) == 0)
			{
				var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
				if (var == NOVARIABLE) options = 0; else options = var->addr;
				(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
					options & ~NCCRECURSE, VINTEGER|VDONTSAVE);
				ttyputverbose(M_("Circuits will be compared without recursing through hierarchy"));
				return;
			}
			ttyputbadusage(x_("telltool network compare not"));
			return;
		}
		if (namesamen(pt, x_("include-no-component-nets"), l) == 0)
		{
			var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
			if (var == NOVARIABLE) options = 0; else options = var->addr;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
				options | NCCINCLUDENOCOMPNETS, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Circuit comparison will exclude networks with no components on them"));
			return;
		}
		if (namesamen(pt, x_("check-export-names"), l) == 0 && l >= 7)
		{
			var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
			if (var == NOVARIABLE) options = 0; else options = var->addr;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
				options | NCCCHECKEXPORTNAMES, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Circuit comparison will check export names"));
			return;
		}
		if (namesamen(pt, x_("check-node-sizes"), l) == 0 && l >= 7)
		{
			var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
			if (var == NOVARIABLE) options = 0; else options = var->addr;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
				options | NCCCHECKSIZE, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Circuit comparison will check node sizes"));
			return;
		}
		if (namesamen(pt, x_("flatten-hierarchy"), l) == 0)
		{
			var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
			if (var == NOVARIABLE) options = 0; else options = var->addr;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
				options | NCCHIERARCHICAL, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Circuits will be compared with hierarchy flattened"));
			return;
		}
		if (namesamen(pt, x_("recurse"), l) == 0)
		{
			var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
			if (var == NOVARIABLE) options = 0; else options = var->addr;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey,
				options | NCCRECURSE, VINTEGER|VDONTSAVE);
			ttyputverbose(M_("Circuits will be compared recursing through hierarchy"));
			return;
		}
		ttyputbadusage(x_("telltool network compare"));
		return;
	}

	if (namesamen(pt, x_("list-ports-below"), l) == 0 && l >= 6)
	{
		/* get the currently highlighted network */
		if (count < 2)
		{
			netlist = net_gethighlightednets(FALSE);
			if (netlist[0] == NONETWORK) return;
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
			mynet[0] = getnetwork(par[1], np);
			if (mynet[0] == NONETWORK)
			{
				ttyputerr(M_("No net called '%s'"), par[1]);
				return;
			}
			mynet[1] = NONETWORK;
			netlist = mynet;
		}

		for(i=0; netlist[i] != NONETWORK; i++)
		{
			net = netlist[i];
			ttyputmsg(_("Network '%s':"), describenetwork(net));

			/* find all exports on network "net" */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						pp->temp1 = 0;
			net_findportsdown(net, net->parent);
		}
		return;
	}

	if (namesamen(pt, x_("list-hierarchical-ports"), l) == 0 && l >= 6)
	{
		/* get the currently highlighted network */
		if (count < 2)
		{
			netlist = net_gethighlightednets(FALSE);
			if (netlist[0] == NONETWORK) return;
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
			mynet[0] = getnetwork(par[1], np);
			if (mynet[0] == NONETWORK)
			{
				ttyputerr(M_("No net called '%s'"), par[1]);
				return;
			}
			mynet[1] = NONETWORK;
			netlist = mynet;
		}

		for(i=0; netlist[i] != NONETWORK; i++)
		{
			net = netlist[i];
			ttyputmsg(_("Network '%s':"), describenetwork(net));

			/* find all exports on network "net" */
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						pp->temp1 = 0;
			net_findportsup(net, net->parent);
			(void)ttyputmsg(_(" Going down the hierarchy from cell %s"), describenodeproto(net->parent));
			net_findportsdown(net, net->parent);
		}
		return;
	}

	if (namesamen(pt, x_("debug-toggle"), l) == 0 && l > 3)
	{
		if (!net_debug)
		{
			net_debug = TRUE;
			ttyputmsg(M_("Network debugging on"));
		} else
		{
			net_debug = FALSE;
			ttyputmsg(M_("Network debugging off"));
		}
		return;
	}

	if (namesamen(pt, x_("resistors"), l) == 0 && l >= 3)
	{
		if (count < 2)
		{
			ttyputusage(x_("telltool network resistors (ignore | include)"));
			return;
		}
		l = estrlen(pt = par[1]);
		if (namesamen(pt, x_("ignore"), l) == 0 && l >= 2)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options | NETIGNORERESISTORS, VINTEGER|VDONTSAVE);
			net_tellschematics();
			return;
		}
		if (namesamen(pt, x_("include"), l) == 0 && l >= 2)
		{
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey,
				net_options & ~NETIGNORERESISTORS, VINTEGER|VDONTSAVE);
			net_tellschematics();
			return;
		}
		ttyputbadusage(x_("telltool network resistors"));
	}

	if (namesamen(pt, x_("re-number"), l) == 0 && l >= 3)
	{
		if (count >= 2)
		{
			np = getnodeproto(par[1]);
			if (np == NONODEPROTO)
			{
				ttyputerr(M_("No cell named %s"), par[1]);
				return;
			}
			if (np->primindex != 0)
			{
				ttyputerr(M_("Can only renumber cells, not primitives"));
				return;
			}
		} else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell"));
				return;
			}
		}

		net_reevaluatecell(np);
		ttyputverbose(M_("Cell %s re-numbered"), describenodeproto(np));
		return;
	}

	if (namesamen(pt, x_("total-re-number"), l) == 0)
	{
		net_totalrenumber(NOLIBRARY);
		ttyputverbose(M_("All libraries re-numbered"));
		return;
	}

	ttyputbadusage(x_("telltool network"));
}

/*
 * make request of the network tool:
 * "total-re-number" renumbers all libraries
 * "library-re-number" TAKES: library to renumbers
 * "re-number" TAKES: the CELL to be renumbered
 * "rename" TAKES: old STRING name, new STRING name, CELL
 * "name-nodes" TAKES: CELL with nodes; RETURNS: number of nodes named
 * "name-all-nodes" TAKES: CELL with nodes; RETURNS: number of nodes named
 */
INTBIG net_request(CHAR *command, va_list ap)
{
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER INTBIG arg1, arg2, arg3;
	REGISTER INTBIG retval;

	if (namesame(command, x_("total-re-number")) == 0)
	{
		net_totalrenumber(NOLIBRARY);
		return(0);
	}
	if (namesame(command, x_("library-re-number")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		lib = (LIBRARY *)arg1;
		net_totalrenumber(lib);
		return(0);
	}
	if (namesame(command, x_("re-number")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		np = (NODEPROTO *)arg1;
		net_reevaluatecell(np);
		return(0);
	}
	if (namesame(command, x_("name-nodes")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		np = (NODEPROTO *)arg1;
		retval = net_nameallnodes(np, FALSE);
		return(retval);
	}
	if (namesame(command, x_("name-all-nodes")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		np = (NODEPROTO *)arg1;
		retval = net_nameallnodes(np, TRUE);
		return(retval);
	}
	if (namesame(command, x_("name-nets")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);

		np = (NODEPROTO *)arg1;
		retval = net_nameallnets(np);
		return(retval);
	}
	if (namesame(command, x_("rename")) == 0)
	{
		/* get the arguments */
		arg1 = va_arg(ap, INTBIG);
		arg2 = va_arg(ap, INTBIG);
		arg3 = va_arg(ap, INTBIG);

		net_renamenet((CHAR *)arg1, (CHAR *)arg2, (NODEPROTO *)arg3);
		return(0);
	}
	return(-1);
}

/*********************** BROADCAST ROUTINES ***********************/

void net_startbatch(TOOL *source, BOOLEAN undoredo)
{
	Q_UNUSED( undoredo );

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	net_globalwork = FALSE;
	net_current_source = source;
}

void net_endbatch(void)
{
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	BOOLEAN advance;

	if (!net_globalwork) return;

	if (net_debug) ttyputmsg(M_("Network: doing entire cell rechecks"));

	/* look for all cells that need to be renumbered */
	do {
		advance = FALSE;
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&REDOCELLLIB) == 0) continue;
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if ((np->userbits&REDOCELLNET) == 0) continue;
				advance = TRUE;
				if (net_recursivelyredo(np))
				{
					ttyputmsg(_("Network information may be incorrect...should redo network numbering"));
					net_globalwork = FALSE;
					return;
				}
			}
		}
	} while (advance);
	net_globalwork = FALSE;
}

/*
 * Routine to renumber all networks.
 */
void net_totalrenumber(LIBRARY *lib)
{
	REGISTER LIBRARY *olib;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;

	/* recache net_tool options */
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_optionskey);
	if (var != NOVARIABLE)
		net_options = var->addr;
	net_recacheunifystrings();

	if (lib == NOLIBRARY)
	{
		/* renumber every library */
		net_globalwork = TRUE;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		{
			olib->userbits |= REDOCELLLIB;
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->userbits |= REDOCELLNET;
		}
	} else
	{
		/* renumber just this library */
		net_startglobalwork(lib);
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->userbits |= REDOCELLNET;
	}
	net_endbatch();
}

/*
 * routine to redo the connectivity within cell "np" (and recursively,
 * all cells below that in need of renumbering).
 * Returns true if interrupted
 */
BOOLEAN net_recursivelyredo(NODEPROTO *np)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *cnp, *subnp;

	if (stopping(STOPREASONNETWORK) != 0)
	{
		return(TRUE);
	}

	/* renumber contents view if this is icon or skeleton */
	cnp = contentsview(np);
	if (cnp != NONODEPROTO && (cnp->userbits&REDOCELLNET) != 0)
	{
		if (net_recursivelyredo(cnp)) return(TRUE);
	}

	/* first see if any lower cells need to be renumbered */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore recursive references (showing icon in contents) */
		subnp = ni->proto;
		if (isiconof(subnp, np)) continue;

		if (subnp->primindex == 0 && (subnp->userbits&REDOCELLNET) != 0)
		{
			if (net_recursivelyredo(subnp)) return(TRUE);
		}
	}
	net_reevaluatecell(np);

	/* mark this cell rechecked */
	np->userbits &= ~REDOCELLNET;
	return(FALSE);
}

/*
 * routine to redo the connectivity within cell "np".  The "network" fields
 * of the arcs and ports are set to be consistent.  This data and this routine
 * are used by other tools such as the design-rule checker and the simulator
 */
void net_reevaluatecell(NODEPROTO *np)
{
	REGISTER ARCINST *ai;
	REGISTER BUSLIST *bl;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var;
	REGISTER NETWORK *net, *nextnet;
	REGISTER NODEPROTO *subnp, *onp;
	REGISTER PORTARCINST *pi;
	REGISTER INTBIG i, width;
	NODEINST *ni;
	PORTPROTO *cpp;
	NETWORK *gnet, *cnet;
	INTBIG j, k;

	if (net_debug)
		ttyputmsg(M_("Network: rechecking cell %s"), describenodeproto(np));

	/* initialize net hash if necessary */
	net_initnetprivate(np);

	/* first delete all network objects in this cell */
	for(net = np->firstnetwork; net != NONETWORK; net = nextnet)
	{
		nextnet = net->nextnetwork;
		net_freenetwork(net, np);
	}
	np->firstnetwork = NONETWORK;
	for(i=0; i<np->globalnetcount; i++)
		np->globalnetworks[i] = NONETWORK;

#ifdef NEWRENUM
	/* handle non-complicated layout cells */
	if (np->cellview == el_layoutview)
	{
		NetCellConns *conns = new (np->lib->cluster) NetCellConns(np, np->lib->cluster);
		if (!conns->complicated())
		{
			conns->makeNetworksSimple();
			delete conns;
			goto simpleLayout;
		} else
		{
			delete conns;
			ttyputmsg(_("Layout cell %s is complicated"), describenodeproto(np));
		}
	}
#endif

	/* unmark all network data in the cell */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->network = NONETWORK;
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		pp->network = NONETWORK;

	/* initialize for propagation */
	net_initnconnect(np);

	/* make a list of all named busses */
	net_buslistcount = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->proto != sch_busarc) continue;
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) continue;
		width = net_buswidth((CHAR *)var->addr);
		net_addtobuslist(ai, NOPORTPROTO, width);
	}
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		width = net_buswidth(pp->protoname);
		if (width <= 1) continue;
		net_addtobuslist(NOARCINST, pp, width);
	}

	/* handle busses first */
	if (net_buslistcount > 0)
	{
		/* sort by bus width */
		esort(net_buslists, net_buslistcount, sizeof (BUSLIST), net_buslistwidthascending);

		/* renumber arcs, starting with the narrowest */
		for(i=0; i<net_buslistcount; i++)
		{
			bl = &net_buslists[i];
			if (bl->ai != NOARCINST)
			{
				ai = bl->ai;
				if (ai->network != NONETWORK) continue;
				net = net_newnetwork(np);
				(void)net_nconnect(ai, net, np);
			} else
			{
				pp = bl->pp;
				if (pp->network == NONETWORK) (void)net_pconnect(pp);
			}
		}
	}

	/* renumber all other bus arcs (on unnamed networks) */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network != NONETWORK) continue;
		if (ai->proto != sch_busarc) continue;
		net = net_newnetwork(np);
		(void)net_nconnect(ai, net, np);
	}

	/* finally renumber non-bus arcs */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network != NONETWORK) continue;
		if (ai->proto == sch_busarc) continue;
#ifdef NEWRENUM
		/* ignore arcs with no signals on them */
		if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC) continue;
#endif
		net = net_newnetwork(np);
		(void)net_nconnect(ai, net, np);
	}

	/* evaluate "wire_con" nodes which join nets */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != sch_wireconprim) continue;
		net_joinnetworks(ni);
	}

	/* update connectivity values on unconnected ports */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		if (pp->network == NONETWORK) (void)net_pconnect(pp);

	/* finally merge all individual signals connected in busses by subcells */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp == NONODEPROTO) continue;
		if (subnp->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnp, np)) continue;
		if (net_mergebuswires(ni)) net_recursivelymarkabove(subnp);
	}

	/* do checks where arcs touch busses */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto == sch_buspinprim)
		{
			/* see if there is a wire arc on it */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi->conarcinst->proto != sch_busarc) break;
			if (pi != NOPORTARCINST)
				net_checkvalidconnection(ni, pi->conarcinst);
		}
	}

	/* gather global nets of instances */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore primitives and recursive references (showing icon in contents) */
		if (ni->proto->primindex != 0) continue;
		if (isiconof(ni->proto, np)) continue;
		subnp = contentsview(ni->proto);
		if (subnp == NONODEPROTO) subnp = ni->proto;
		for(i=0; i<subnp->globalnetcount; i++)
		{
			if(subnp->globalnetworks[i] == NONETWORK) continue;
			gnet = net_addglobalnet(subnp, i, np);

			/* check if global net is connected to some export */
			if (!ni->proto->netd->netshorts()->globalshort()) continue;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				ai = pi->conarcinst;
				cpp = equivalentport(ni->proto, pi->proto, subnp);
				if (cpp == NOPORTPROTO) continue;
				cnet = cpp->network;
				for(j=0; j<cnet->buswidth; j++)
				{
					if((cnet->buswidth == 1 ? cnet : cnet->networklist[j])
						!= subnp->globalnetworks[i]) continue;
					if(ai->network->buswidth > 1)
					{
						if(ni->arraysize > 1 && cnet->buswidth*ni->arraysize == ai->network->buswidth)
						{
							for(k=0; k < ni->arraysize; k++)
								net_mergenet(ai->network->networklist[j + k*cnet->buswidth], gnet);
						} else net_mergenet(ai->network->networklist[j], gnet);
					} else net_mergenet(ai->network, gnet);
				}
			}
		}
	}
#ifdef NEWRENUM
#  if 0
	{
		NetCellConns *conns = new (np->lib->cluster) NetCellConns(np, np->lib->cluster);
		conns->schem();
		delete conns;
	}
#  endif
#  if 0
	for (net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->buswidth > 1 && net->namecount - net->portcount > 1)
			::printf("Cell %s bus net %s has many names\n", describenodeproto(np), describenetwork(net));
	}
#  endif
simpleLayout:
#endif
	if (!np->netd->updateShorts()) return;

	/* if this cell has an icon, mark it  */
	i = 0;
	FOR_CELLGROUP(onp, np)
	{
		if (i++ > 1000)
		{
			db_correctcellgroups(np);
			break;
		}
		if (contentsview(onp) != np) continue;
		if ((onp->userbits&REDOCELLNET) != 0) continue;
		if (net_debug)
			ttyputmsg(M_("Network: icon %s is marked"), describenodeproto(onp));
		onp->userbits |= REDOCELLNET;
	}

	/* mark instances */
	for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		/* ignore recursive references (showing icon in contents) */
		if (isiconof(ni->proto, ni->parent)) continue;

		net_startglobalwork(ni->parent->lib);
		if ((ni->parent->userbits&REDOCELLNET) != 0) continue;
		if (net_debug)
			ttyputmsg(M_("Network: parent cell %s is marked"), describenodeproto(ni->parent));
		ni->parent->userbits |= REDOCELLNET;
	}
}

void net_modifyportproto(PORTPROTO *pp, NODEINST *oldsubni, PORTPROTO *oldsubpp)
{
#ifndef NEWRENUM
	REGISTER NODEPROTO *np;
#endif
	Q_UNUSED( oldsubni );
	Q_UNUSED( oldsubpp );

	if (net_debug) ttyputmsg(M_("Network: port %s modified"), pp->protoname);

#ifdef NEWRENUM
	net_recursivelymarkabove(pp->parent);
#else
	/* stop now if the entire cell will be renumbered */
	if (net_globalwork && (pp->parent->userbits&REDOCELLNET) != 0) return;

	np = pp->parent;
	net_initnconnect(np);
	if (net_pconnect(pp))
	{
		if (net_debug) ttyputmsg(M_("Network: must recheck instances of %s"),
			describenodeproto(np));
		net_recursivelymarkabove(np);
	}
#endif
}

void net_newobject(INTBIG addr, INTBIG type)
{
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER NODEPROTO *np;

	if ((type&VTYPE) == VARCINST)
	{
		ai = (ARCINST *)addr;
#ifdef NEWRENUM
		net_recursivelymarkabove(ai->parent);
#else
		ai->network = NONETWORK;

		/* stop now if the entire cell will be renumbered */
		if (net_globalwork && (ai->parent->userbits&REDOCELLNET) != 0) return;

		/* ignore nonelectrical arcs */
		if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC) return;

		if (net_debug)
			ttyputmsg(M_("Network: arc %s created"), describearcinst(ai));

		/* remove any former network information */
		(void)net_takearcfromnet(ai);

		/* create a new network and propagate it */
		np = ai->parent;
		net_initnconnect(np);
		if (net_nconnect(ai, net_newnetwork(np), np))
		{
			if (net_debug)
				ttyputmsg(M_("Network: must recheck instances of %s"),
					describenodeproto(np));
			net_recursivelymarkabove(np);
		}
#endif
	} else if ((type&VTYPE) == VPORTPROTO)
	{
		pp = (PORTPROTO *)addr;
#ifdef NEWRENUM
		net_recursivelymarkabove(pp->parent);
#else
		pp->network = NONETWORK;

		/* stop now if the entire cell will be renumbered */
		if (net_globalwork && (pp->parent->userbits&REDOCELLNET) != 0) return;

		if (net_debug) ttyputmsg(M_("Network: port %s created"), pp->protoname);

		np = pp->parent;
		net_initnconnect(np);
		if (net_pconnect(pp))
		{
			if (net_debug)
				ttyputmsg(M_("Network: must recheck instances of %s"),
					describenodeproto(np));
			net_recursivelymarkabove(np);
		}
#endif
	} else if ((type&VTYPE) == VNODEPROTO)
	{
		np = (NODEPROTO *)addr;
		if (net_debug) ttyputmsg(M_("Network: cell %s created"),
				describenodeproto(np));
#ifdef NEWRENUM
		net_recursivelymarkabove(np);
#else
		/* queue this cell for renumbering */
		net_startglobalwork(np->lib);

		/* mark this cell to be renumbered */
		np->userbits |= REDOCELLNET;
#endif
	}
}

void net_killobject(INTBIG addr, INTBIG type)
{
#ifdef NEWRENUM
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net, *nextnet;
#else
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG i;
	REGISTER NETWORK *net, *onet, *nextnet;
#endif

	if ((type&VTYPE) == VARCINST)
	{
		ai = (ARCINST *)addr;
		np = ai->parent;
#ifdef NEWRENUM
		net_recursivelymarkabove(np);
#else
		net = ai->network;

		if (net_debug)
			ttyputmsg(M_("Network: arc %s killed"), describearcinst(ai));

		/* stop now if the entire cell will be renumbered */
		if (net_globalwork && (np->userbits&REDOCELLNET) != 0) return;

		if (ai->proto == sch_busarc)
		{
			/* for busses, must redo entire cell */
			net_recursivelymarkabove(np);
			return;
		}

		/* if arc connects to an export, renumber all above this cell */
		for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		{
			onet = pp->network;
			if (onet == net) break;
			if (onet->buswidth <= 1) continue;
			for(i=0; i<onet->buswidth; i++)
				if (onet->networklist[i] == net) break;
			if (i < onet->buswidth) break;
		}
		if (pp != NOPORTPROTO)
		{
			/* deleted arc connects to an export: must renumber above */
			net_recursivelymarkabove(np);
			return;
		}

		/* if this arc was on a multiply-named network, reevaluate the cell */
		if (net != NONETWORK && net->namecount > 1)
		{
			net_startglobalwork(np->lib);
			np->userbits |= REDOCELLNET;
			return;
		}

		/* remove network pointers to this arc */
		net = ai->network;
		(void)net_takearcfromnet(ai);

		net_initnconnect(np);

		/* renumber both sides of now unconnected network */
		for(i=0; i<2; i++)
		{
			ni = ai->end[i].nodeinst;
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
			{
				if ((pi->conarcinst->userbits&DEADA) != 0) continue;
				if (pi->conarcinst->network != net) continue;

				/* remove any former network information */
				(void)net_takearcfromnet(pi->conarcinst);

				/* create a new network and propagate it */
				if (net_nconnect(pi->conarcinst, net_newnetwork(np), np))
				{
					if (net_debug)
						ttyputmsg(M_("Network: must recheck instances of %s"), describenodeproto(ai->parent));
					net_recursivelymarkabove(np);
					return;
				}
				break;
			}
			if (pi == NOPORTARCINST)
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
			{
				if (pe->exportproto->network != net) continue;
				if (net_pconnect(pe->exportproto))
				{
					if (net_debug)
						ttyputmsg(M_("Network: must recheck instances of %s"),
							describenodeproto(np));
					net_recursivelymarkabove(np);
					return;
				}
				break;
			}
		}
#endif
	} else if ((type&VTYPE) == VPORTPROTO)
	{
		pp = (PORTPROTO *)addr;
		np = pp->parent;
		if (net_debug) ttyputmsg(M_("Network: port %s killed"), pp->protoname);

#ifdef NEWRENUM
		net_recursivelymarkabove(np);
#else
		/* stop now if the entire cell will be renumbered */
		if (net_globalwork && (np->userbits&REDOCELLNET) != 0) return;

		/* remove network pointers to this port */
		(void)net_takeportfromnet(pp);

		/* renumber all arcs connected to the node that this port used */
		for(pi = pp->subnodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if ((pi->conarcinst->userbits&DEADA) != 0) continue;

			/* remove any former network information */
			(void)net_takearcfromnet(pi->conarcinst);

			/* create a new network and propagate it */
			net_initnconnect(np);
			if (net_nconnect(pi->conarcinst, net_newnetwork(np), np))
			{
				if (net_debug)
					ttyputmsg(M_("Network: must recheck instances of %s"),
						describenodeproto(np));
				net_recursivelymarkabove(np);
				return;
			}
		}
#endif
	} else if ((type&VTYPE) == VNODEPROTO)
	{
		np = (NODEPROTO *)addr;

		/* remove references to this cell in the clear-ncc list */
		net_removeclearncc(np);

		/* delete all network objects in this cell */
		for(net = np->firstnetwork; net != NONETWORK; net = nextnet)
		{
			nextnet = net->nextnetwork;
			net_freenetwork(net, np);
		}
		np->firstnetwork = NONETWORK;
	}
}

void net_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
#ifdef NEWRENUM
	REGISTER VARIABLE *var;
	REGISTER ARCINST *ai;
	CHAR *netname;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
#else
	REGISTER VARIABLE *var;
	REGISTER NETWORK *net;
	REGISTER ARCINST *ai;
	CHAR **strings, *netname;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG count;
	REGISTER PORTPROTO *pp, *subpp;
#endif

	if ((newtype&VCREF) != 0)
	{
		if ((type&VTYPE) == VPORTPROTO)
		{
			netname = changedvariablename(type, key, newtype);
			if (namesame(netname, x_("userbits")) == 0)
			{
				/* changing characteristics on export invalidates NCC of cell */
				pp = (PORTPROTO *)addr;
				net_queueclearncc(pp->parent);
				return;
			}

			if (namesame(netname, x_("protoname")) == 0)
			{
				if (net_debug)
					ttyputmsg(M_("Network: export name %s created"), netname);

				/* stop now if the entire cell will be renumbered */
				pp = (PORTPROTO *)addr;
#ifdef NEWRENUM
				net_recursivelymarkabove(pp->parent);
#else
				subpp = pp;
				ni = subpp->subnodeinst;
				while (ni->proto->primindex == 0)
				{
					ni = subpp->subnodeinst;
					subpp = subpp->subportproto;
				}
				if (ni->proto != sch_buspinprim) return;
				if (net_globalwork && (pp->parent->userbits&REDOCELLNET) != 0) return;

				/* check name for validity */
				count = net_evalbusname(APBUS, pp->protoname, &strings, NOARCINST, pp->parent, 1);
				if (count < 0)
				{
					ttyputerr(_("Warning (cell %s): invalid network name: '%s'"), describenodeproto(pp->parent),
						pp->protoname);
					return;
				}

				/* for busses, must redo entire cell */
				if (pp->network->buswidth != count)
				{
					net_recursivelymarkabove(pp->parent);
					return;
				}
#endif
			}
		}
		return;
	}

	/* handle changes to variables on the network object */
	if ((type&VTYPE) == VTOOL && (TOOL *)addr == net_tool)
	{
		if (key == net_optionskey)
		{
			var = getvalkey(addr, type, VINTEGER, key);
			if (var != NOVARIABLE)
			{
				net_options = var->addr;
				net_checkresistorstate = TRUE;
			}
			return;
		}
		if (key == net_unifystringskey)
		{
			net_recacheunifystrings();
			return;
		}
		return;
	}

	/* handle changes to node variables */
	if ((type&VTYPE) == VNODEINST)
	{
		/* handle changes to node name variable (look for arrays) */
		if (key == el_node_name_key)
		{
			net_setnodewidth((NODEINST *)addr);
			return;
		}

		/* changes to parameters on nodes invalidates NCC information */
		var = getvalkeynoeval(addr, type, -1, key);
		if (var != NOVARIABLE)
		{
			if (TDGETISPARAM(var->textdescript) != 0)
			{
				ni = (NODEINST *)addr;
				net_queueclearncc(ni->parent);
				return;
			}
		}

		/* changes to transistor sizes invalidates NCC information */
		if (key == el_attrkey_width || key == el_attrkey_length || key == el_attrkey_area)
		{
			ni = (NODEINST *)addr;
			net_queueclearncc(ni->parent);
			return;
		}

		return;
	}

	/* handle changes to an ARCINSTs "ARC_name" variable */
	if (key != el_arc_name_key) return;
	if ((type&(VTYPE|VISARRAY)) != VARCINST) return;
	var = getvalkey(addr, type, VSTRING, key);
	if (var == NOVARIABLE || *((CHAR *)var->addr) == 0) return;

	if (net_debug)
		ttyputmsg(M_("Network: arc name %s created"), (CHAR *)var->addr);

	/* stop now if the entire cell will be renumbered */
	ai = (ARCINST *)addr;
	np = ai->parent;
#ifdef NEWRENUM
	net_recursivelymarkabove(np);
#else
	if (net_globalwork && (np->userbits&REDOCELLNET) != 0) return;

	/* check name for validity */
	count = net_evalbusname((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH,
		(CHAR *)var->addr, &strings, ai, ai->parent, 1);
	if (count < 0)
	{
		ttyputerr(_("Warning (cell %s): invalid network name: '%s'"),
			describenodeproto(np), (CHAR *)var->addr);
		return;
	}

	/* for busses, must redo entire cell */
	if (ai->proto == sch_busarc)
	{
		net_recursivelymarkabove(np);
		return;
	}

	/* if merging common net names, check this one */
	net = NONETWORK;
	if ((net_options&NETCONCOMMONNAME) != 0 || np->cellview == el_schematicview ||
		ai->proto->tech == sch_tech)
	{
		/* see if network exists in this cell */
		net = net_getunifiednetwork((CHAR *)var->addr, np, NONETWORK);
	}
	if (net == NONETWORK) net = net_newnetwork(np); else
	{
		if (net_current_source == us_tool)
			ttyputmsg(_("Network '%s' extended to this arc"), describenetwork(net));
	}

	/* propagate the network through this cell */
	net_initnconnect(np);
	if (net_nconnect(ai, net, np))
	{
		if (net_debug) ttyputmsg(M_("Network: must recheck instances of %s"),
			describenodeproto(np));
		net_recursivelymarkabove(np);
	}
#endif
}

void net_killvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG oldaddr, INTBIG oldtype,
	UINTBIG *olddescript)
{
	REGISTER ARCINST *ai;
#ifndef NEWRENUM
	REGISTER NODEPROTO *np;
#endif
	Q_UNUSED( olddescript );

	/* handle changes to node name variable (look for arrays) */
	if ((type&VTYPE) == VNODEINST)
	{
		if (key == el_node_name_key)
		{
			net_setnodewidth((NODEINST *)addr);
		}
		return;
	}

	/* only interested in the string variable "ARC_name" on ARCINSTs */
	if (key != el_arc_name_key) return;
	if ((type&(VTYPE|VISARRAY)) != VARCINST) return;
	if ((oldtype&(VTYPE|VISARRAY|VCREF)) != VSTRING) return;
	if (*((CHAR *)oldaddr) == 0) return;

	if (net_debug)
		ttyputmsg(M_("Network: arc name '%s' deleted"), (CHAR *)oldaddr);

	/* get the arc being unnamed */
	ai = (ARCINST *)addr;
#ifdef NEWRENUM
	net_recursivelymarkabove(ai->parent);
#else
	if (ai->network == NONETWORK) return;
	np = ai->parent;

	/* stop now if the entire cell will be renumbered */
	if (net_globalwork && (np->userbits&REDOCELLNET) != 0) return;

	/* no network functions on nonelectrical arcs */
	if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC) return;

	/* if this network is at all complex, must renumber cell */
	if (ai->network->namecount != 1 || ai->network->arccount != 1 ||
		ai->network->portcount != 0 || ai->network->buslinkcount != 0)
	{
		net_recursivelymarkabove(np);
		return;
	}

	/* remove any former network information */
	(void)net_takearcfromnet(ai);

	/* create a new network and propagate it */
	net_initnconnect(np);
	if (net_nconnect(ai, net_newnetwork(np), np))
	{
		if (net_debug) ttyputmsg(M_("Network: must recheck instances of %s"),
			describenodeproto(np));
		net_recursivelymarkabove(np);
	}
#endif
}

void net_readlibrary(LIBRARY *lib)
{
	REGISTER VARIABLE *var;
	Q_UNUSED( lib );

	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_optionskey);
	if (var != NOVARIABLE)
	{
		net_options = var->addr;
		net_checkresistorstate = TRUE;
	}
	net_recacheunifystrings();

	/* handle obsolete flags */
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_connect_power_groundkey);
	if (var != NOVARIABLE)
	{
		if (var->addr != 0) net_options |= NETCONPWRGND; else
			net_options &= ~NETCONPWRGND;
		net_found_obsolete_variables = 1;
	}
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_connect_common_namekey);
	if (var != NOVARIABLE)
	{
		if (var->addr != 0) net_options |= NETCONCOMMONNAME; else
			net_options &= ~NETCONCOMMONNAME;
		net_found_obsolete_variables = 1;
	}
}

void net_eraselibrary(LIBRARY *lib)
{
	Q_UNUSED( lib );
}

/*********************** NCC INVALIDATION ***********************/

/*
 * Routine to allocate a "clear NCC" object.
 * Returns NOCLEARNCC on error.
 */
CLEARNCC *net_allocclearncc(void)
{
	REGISTER CLEARNCC *cn;

	if (net_clearnccfree != NOCLEARNCC)
	{
		cn = net_clearnccfree;
		net_clearnccfree = cn->nextclearncc;
	} else
	{
		cn = (CLEARNCC *)emalloc(sizeof (CLEARNCC), net_tool->cluster);
		if (cn == 0) return(NOCLEARNCC);
	}
	return(cn);
}

/*
 * Routine to free clear-NCC object "cn".
 */
void net_freeclearncc(CLEARNCC *cn)
{
	cn->nextclearncc = net_clearnccfree;
	net_clearnccfree = cn;
}

/*
 * Routine to queue cell "np" to have its NCC cache cleared.
 */
void net_queueclearncc(NODEPROTO *np)
{
	REGISTER CLEARNCC *cn;

	/* ignore duplicates */
	for(cn = net_firstclearncc; cn != NOCLEARNCC; cn = cn->nextclearncc)
		if (cn->np == np) return;

	/* create a clear-ncc object for this request */
	cn = net_allocclearncc();
	if (cn == NOCLEARNCC) return;
	cn->np = np;

	/* put this on the list */
	cn->nextclearncc = net_firstclearncc;
	net_firstclearncc = cn;
}

/*
 * Routine to remove references to cell "np" in the clear-ncc list because the cell has
 * been deleted.
 */
void net_removeclearncc(NODEPROTO *np)
{
	REGISTER CLEARNCC *cn, *lastcn;

	lastcn = NOCLEARNCC;
	for(cn = net_firstclearncc; cn != NOCLEARNCC; cn = cn->nextclearncc)
	{
		if (cn->np == np)
		{
			if (lastcn == NOCLEARNCC) net_firstclearncc->nextclearncc = cn->nextclearncc; else
				lastcn->nextclearncc = cn->nextclearncc;
			net_freeclearncc(cn);
			return;
		}
		lastcn = cn;
	}
}

/*********************** RECURSIVE NETWORK TRACING ***********************/

/*
 * Routine to allocate an "arc check" object for network tracing.
 * Returns NOARCCHECK on error.
 */
ARCCHECK *net_allocarccheck(void)
{
	REGISTER ARCCHECK *ac;

	if (net_arccheckfree != NOARCCHECK)
	{
		ac = net_arccheckfree;
		net_arccheckfree = ac->nextarccheck;
	} else
	{
		ac = (ARCCHECK *)emalloc(sizeof (ARCCHECK), net_tool->cluster);
		if (ac == 0) return(NOARCCHECK);
	}
	return(ac);
}

/*
 * Routine to free arc check object "ac".
 */
void net_freearccheck(ARCCHECK *ac)
{
	ac->nextarccheck = net_arccheckfree;
	net_arccheckfree = ac;
}

/*
 * routine to initialize for network renumbering done through "net_nconnect".
 */
void net_initnconnect(NODEPROTO *np)
{
	REGISTER ARCINST *ai;

	/* mark all arcs as un-checked */
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		ai->temp2 = 0;
}


/*
 * routine to trace all electrical paths starting at the given arcinst "ai"
 * and set the network to "newnetwork".  The pointers "power" and "ground"
 * are the current power and ground networks in the cell.  Returns true
 * if the cell should be rechecked (because ports were modified).  Traces
 * contents ports in case of iconic connections.
 */
BOOLEAN net_nconnect(ARCINST *ai, NETWORK *newnetwork, NODEPROTO *np)
{
	REGISTER ARCCHECK *ac;
	BOOLEAN result;
#if 0
	REGISTER ARCINST *oai;

	for(oai = np->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
		oai->temp2 = 0;
#endif

	/* create an arc-check object for this request */
	ac = net_allocarccheck();
	if (ac == NOARCCHECK) return(FALSE);
	ac->ai = ai;
	ai->temp2 = 1;

	/* put this on the list */
	ac->nextarccheck = NOARCCHECK;
	net_firstarccheck = ac;

	/* process everything on the list */
	result = FALSE;
	while (net_firstarccheck != NOARCCHECK)
	{
		/* remove the top of the list */
		ac = net_firstarccheck;
		net_firstarccheck = ac->nextarccheck;

		/* process it */
		if (net_donconnect(ac->ai, newnetwork, np)) result = TRUE;

		/* free this list object */
		net_freearccheck(ac);
	}
	return(result);
}

BOOLEAN net_donconnect(ARCINST *ai, NETWORK *newnetwork, NODEPROTO *np)
{
	REGISTER ARCINST *oai;
	REGISTER NODEINST *ni;
	NODEINST *arrayni;
	PORTPROTO *arraypp;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *cpp, *copp;
	REGISTER NODEPROTO *cnp;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt, *base1, *base2, *arcname;
	static CHAR arctag[50];
	INTBIG special;
	REGISTER INTBIG ret, i, j, tempname, fun, width, buswidth, len1, len2, base;
	REGISTER BOOLEAN recheck;
	REGISTER ARCCHECK *ac;

	if (net_debug)
		ttyputmsg(M_("Setting network %ld onto arc %s"), newnetwork, describearcinst(ai));

	if (ai->network == newnetwork) return(FALSE);

	/* ignore arcs with no signals on them */
	if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC) return(FALSE);

	/* ignore if two busses have different width */
	var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
	if (var == NOVARIABLE) arcname = 0; else
		arcname = (CHAR *)var->addr;
	if (ai->proto == sch_busarc && newnetwork->buswidth > 1 &&
		((ai->network != NONETWORK && ai->network->buswidth > 1) || var != NOVARIABLE))
	{
		/* bus network */
		if (var == NOVARIABLE) width = ai->network->buswidth; else
		{
			width = net_buswidth(arcname);
			if (width == 1 && newnetwork->buswidth > 1)
			{
				/* name indicates one signal, see if it simply omitted any array specification */
				for(pt = arcname; *pt != 0; pt++)
					if (*pt == '[') break;
				if (*pt == 0)
				{
					/* use the implied width */
					if ((var->type & VDISPLAY) == 0)
					{
#if 0
						ttyputmsg(_("Warning (cell %s): INVISIBLE unindexed arcname '%s' is used for bus"),
							describenodeproto(np), arcname);
#endif
					} else
					{
						ttyputmsg(_("Warning (cell %s): unindexed arcname '%s' is used for bus"),
							describenodeproto(np), arcname);
					}
					width = newnetwork->buswidth;
				}
			}
		}
		if (width != newnetwork->buswidth)
		{
			/* different width networks meet: see if it is sensible */
			base1 = describenetwork(newnetwork);
			if (var != NOVARIABLE) base2 = (CHAR *)var->addr; else
				base2 = describenetwork(ai->network);
			for(len1 = 0; base1[len1] != 0; len1++) if (base1[len1] == '[') break;
			for(len2 = 0; base2[len2] != 0; len2++) if (base2[len2] == '[') break;
			if (len1 != len2 || namesamen(base1, base2, len1) != 0)
			{
				ttyputmsg(_("Warning (cell %s): networks '%s' and '%s' connected with different lengths"),
					describenodeproto(np), base1, base2);
			}
			return(TRUE);
		}
	}

	/* presume that recheck of cell is not necessary */
	recheck = FALSE;

	/* free previous network information */
	(void)net_takearcfromnet(ai);

	/* add this arc to the network */
	net_putarconnet(ai, newnetwork);

	/* determine the width of bus arcs by looking at adjoining cell exports */
	buswidth = net_buswidthofarc(ai, &arrayni, &arraypp);
	if (buswidth > 1 && newnetwork->buswidth > 1)
	{
		if (arrayni->arraysize > 1 && buswidth / arrayni->arraysize == newnetwork->buswidth)
			buswidth = newnetwork->buswidth;
		if (buswidth != newnetwork->buswidth)
		{
			ttyputmsg(_("Warning: cell %s has %d-wide %s connected to node %s, port %s, which is %ld-wide"),
				describenodeproto(np), newnetwork->buswidth, describearcinst(ai), describenodeinst(arrayni),
					arraypp->protoname, buswidth);
		}
	}

	/* if this arc has a name, establish it */
	if (arcname != 0 && *arcname != 0)
	{
		/* add this arc name to the network */
		if (net_debug)
			ttyputmsg(M_("Adding name '%s' to network '%s'"), arcname, describenetwork(newnetwork));
		if ((var->type&VDISPLAY) == 0) tempname = 1; else tempname = 0;
		if (buswidth > 1)
		{
			/* be sure there is an array specification, create if not */
			for(pt = arcname; *pt != 0; pt++) if (*pt == '[' || *pt == ',') break;
			if (*pt == 0)
			{
				/* use the implied width */
				if ((var->type & VDISPLAY) == 0)
				{
#if 0
					ttyputmsg(_("Warning (cell %s): INVISIBLE unindexed arcname '%s' is used for bus"),
						describenodeproto(np), arcname);
#endif
				} else
				{
					ttyputmsg(_("Warning (cell %s): unindexed arcname '%s' is used for bus"),
						describenodeproto(np), arcname);
				}
				if (net_arrayedarcname != 0) efree(net_arrayedarcname);
				if ((net_options&NETDEFBUSBASE1) == 0) base = 0; else base = 1;
				if ((net_options&NETDEFBUSBASEDESC) == 0)
				{
					(void)esnprintf(arctag, 50, x_("[%ld:%ld]"), base, buswidth-1+base);
				} else
				{
					(void)esnprintf(arctag, 50, x_("[%ld:%ld]"), buswidth-1+base, base);
				}
				net_arrayedarcname = (CHAR *)emalloc((estrlen(arcname)+estrlen(arctag)+2) * SIZEOFCHAR,
					net_tool->cluster);
				if (net_arrayedarcname == 0) return(FALSE);
				estrcpy(net_arrayedarcname, arcname);
				estrcat(net_arrayedarcname, arctag);
				arcname = net_arrayedarcname;
			}
		}
		ret = net_addnametonet(arcname, tempname, newnetwork, ai, NOPORTPROTO, np);
		if (ret > 0) recheck = TRUE;
		if (ret >= 0) net_putarclinkonnet(newnetwork, ai);
	} else
	{
		if (buswidth > 1 && newnetwork->buswidth == 1)
		{
			/* unnamed bus: generate individual unnamed signals */
			newnetwork->networklist = (NETWORK **)emalloc(((sizeof (NETWORK *)) * buswidth),
				np->lib->cluster);
			if (newnetwork->networklist != 0)
			{
				if (newnetwork->namecount > 0)
				{
					if (newnetwork->tempname)
					{
#if 0
						ttyputmsg(_("Warning (cell %s): INVISIBLE unindexed netname '%s' is used for bus"),
							describenodeproto(np), networkname(newnetwork, 0));
#endif
					} else
					{
						ttyputmsg(_("Warning (cell %s): unindexed netname '%s' is used for bus"),
							describenodeproto(np), networkname(newnetwork, 0));
					}
				}
				newnetwork->buswidth = (INTSML)buswidth;
				net_ensuretempbusname(newnetwork);
				for(i=0; i<buswidth; i++)
				{
					newnetwork->networklist[i] = net_newnetwork(np);
					newnetwork->networklist[i]->buslinkcount++;
				}
			}
			recheck = TRUE;
		}
	}

	/* initialize the special information about this network */
	special = newnetwork->globalnet;

	/* recursively set all arcs and nodes touching this */
	for(i=0; i<2; i++)
	{
		/* establish the contents relationships */
		ni = ai->end[i].nodeinst;
		if (ni == NONODEINST) continue;
		if ((ni->proto->userbits&REDOCELLNET) != 0)
		{
			recheck = TRUE;
			continue;
		}
		cnp = contentsview(ni->proto);
		if (cnp == NONODEPROTO) cnp = ni->proto;
		cpp = equivalentport(ni->proto, ai->end[i].portarcinst->proto, cnp);
		if (cpp == NOPORTPROTO)
		{
			cpp = ai->end[i].portarcinst->proto;
			if (cpp == NOPORTPROTO) continue;
		}

		/* if this network hit a "wire connection", must reevaluate the cell */
		if (ni->proto == sch_wireconprim) recheck = TRUE;

		/* if this arc end connects to an isolated port, ignore it */
		if ((cpp->userbits&PORTISOLATED) != 0) continue;

		/* do not follow nonbus wires onto a bus pin */
		if (cnp == sch_buspinprim && ai->proto != sch_busarc) continue;

		/* if this arc end is negated, ignore its node propagation */
		if ((ai->userbits&ISNEGATED) != 0)
		{
			if ((ai->userbits&REVERSEEND) == 0)
			{
				if (i == 0) continue;
			} else
			{
				if (i == 1) continue;
			}
		}

		/* check out the node on the arc */
		if (cnp == sch_globalprim)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_globalnamekey);
			if (var != NOVARIABLE)
			{
				net_setglobalnet(&special, -1, (CHAR *)var->addr, newnetwork,
					((ni->userbits&NTECHBITS) >> NTECHBITSSH) << STATEBITSSH, np);
				recheck = TRUE;
			}
		} else
		{
			fun = (cnp->userbits&NFUNCTION)>>NFUNCTIONSH;
			if (fun == NPCONPOWER)
			{
				net_setglobalnet(&special, GLOBALNETPOWER, 0, newnetwork, PWRPORT, np);
				recheck = TRUE;
			} else if (fun == NPCONGROUND)
			{
				net_setglobalnet(&special, GLOBALNETGROUND, 0, newnetwork, GNDPORT, np);
				recheck = TRUE;
			}
		}

		if ((net_options&NETCONPWRGND) != 0)
		{
			/* see if subport on the node is power or ground */
			if (portispower(cpp))
			{
				net_setglobalnet(&special, GLOBALNETPOWER, 0, newnetwork, PWRPORT, np);
				recheck = TRUE;
			} else if (portisground(cpp))
			{
				net_setglobalnet(&special, GLOBALNETGROUND, 0, newnetwork, GNDPORT, np);
				recheck = TRUE;
			}
		}

		/* numerate ports in "ni->proto" */
		PORTPROTO *pp;
		for (pp = ni->proto->firstportproto, j=0; pp != NOPORTPROTO; pp = pp->nextportproto, j++)
			pp->temp1 = j;

		/* get net number */
		NetCellShorts *shorts = ni->proto->netd->netshorts();
		INTBIG netnum = shorts->portshallowmap(ai->end[i].portarcinst->proto->temp1);
		if (shorts->globalshort()) recheck = TRUE;

		/* look at all other arcs connected to the node on this end of the arc */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (shorts->portshallowmap(pi->proto->temp1) != netnum &&
				(newnetwork->buswidth != 1 || pi->proto->network->buswidth <= 1))
				continue;

			/* select an arcinst that has not been examined */
			oai = pi->conarcinst;
			if (oai->temp2 != 0) continue;
			if (oai == ai) continue;
	
			if (oai->network == newnetwork) continue;

			/* establish the contents connection for this portarcinst */
			copp = equivalentport(ni->proto, pi->proto, cnp);
			if (copp == NOPORTPROTO) copp = pi->proto;
			if (copp->network == NONETWORK) continue;

			/* do not follow nonbus wires from a bus pin */
			if (cnp == sch_buspinprim && oai->proto != sch_busarc) continue;

			/* see if the two ports connect electrically */
			if (cpp->network != copp->network)
			{
				/* check for single signals ending up as part of another bus */
				if (newnetwork->buswidth == 1 && copp->network->buswidth > 1)
				{
					for(j=0; j<copp->network->buswidth; j++)
						if (cpp->network == copp->network->networklist[j]) break;
					if (j < copp->network->buswidth)
						recheck = TRUE;
				}
				continue;
			}

			/* if this arc end is negated, ignore its node propagation */
			if ((oai->userbits&ISNEGATED) != 0)
			{
				if ((oai->userbits&REVERSEEND) == 0)
				{
					if (oai->end[0].portarcinst == pi) continue;
				} else
				{
					if (oai->end[1].portarcinst == pi) continue;
				}
			}

			/* recurse on the nodes of this arcinst */
			if (net_debug)
				ttyputmsg(M_("Propagating network %ld from node %s to arc %s"), newnetwork,
					describenodeinst(ni), describearcinst(oai));

			/* create an arc-check object for this request */
			ac = net_allocarccheck();
			if (ac == NOARCCHECK) return(FALSE);
			ac->ai = oai;
			oai->temp2 = 1;

			/* put this on the list */
			ac->nextarccheck = net_firstarccheck;
			net_firstarccheck = ac;
		}

		/* set the port information for any exports */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (shorts->portshallowmap(pe->proto->temp1) != netnum) continue;

			/* add this port name to the network */
			net_addnametonet(pe->exportproto->protoname, 0, newnetwork,
				NOARCINST, pe->exportproto, np);

			/* network extends to export, set it */
			(void)net_takeportfromnet(pe->exportproto);
			net_putportonnet(pe->exportproto, newnetwork);

			if ((net_options&NETCONPWRGND) != 0)
			{
				/* check for power or ground ports */
				if (portispower(pe->exportproto))
				{
					net_setglobalnet(&special, GLOBALNETPOWER, 0, newnetwork, PWRPORT, np);
				} else if (portisground(pe->exportproto))
				{
					net_setglobalnet(&special, GLOBALNETGROUND, 0, newnetwork, GNDPORT, np);
				}
			}
			recheck = TRUE;
			break;
		}
	}
	return(recheck);
}

/*
 * routine to determine the network number of port "pp".  Returns true
 * if the network information has changed.
 */
BOOLEAN net_pconnect(PORTPROTO *pp)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER NODEPROTO *np;
	REGISTER NETWORK *oldnet, *net;
	REGISTER BOOLEAN ret;
	REGISTER INTBIG width;
	INTBIG special;

	/* throw away any existing network information */
	oldnet = pp->network;
	np = pp->parent;
	if (oldnet != NONETWORK) (void)net_takeportfromnet(pp);

	/* if port comes from an isolated subport, give it new number */
	if ((pp->subportproto->userbits&PORTISOLATED) != 0)
	{
		net = net_newnetwork(np);
		if (net_debug)
			ttyputmsg(M_("Network: creating new network for isolated subport %s"),
				pp->protoname);
		(void)net_addnametonet(pp->protoname, 0, net, NOARCINST, pp, np);
		net_putportonnet(pp, net);
		return(TRUE);
	}

	/* next see if that port connects to an arc */
	width = net_buswidth(pp->protoname);
	for(pi = pp->subnodeinst->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto->network == pp->subportproto->network)
	{
		/* do not follow bus pins onto non-bus arcs */
		if (pp->subnodeinst->proto == sch_buspinprim && pi->conarcinst->proto != sch_busarc)
			continue;

		net = pi->conarcinst->network;
		if (width > 1) net = NONETWORK;
		if (net == NONETWORK)
		{
			net = net_newnetwork(np);
			if (net_debug)
				ttyputmsg(M_("Network: creating new network for %s (on an arc with no net)"),
					pp->protoname);
		} else if (net_debug)
			ttyputmsg(M_("Network: adding port %s to net %s"), pp->protoname,
				describenetwork(net));
		(void)net_addnametonet(pp->protoname, 0, net, pi->conarcinst, pp, np);
		net_putportonnet(pp, net);
		if (pp->network != oldnet) ret = TRUE; else ret = FALSE;
		if (net_nconnect(pi->conarcinst, net, np)) ret = TRUE;
		return(ret);
	}

	/* finally see if that port connects to another export */
	for(pe = pp->subnodeinst->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		if (pe->exportproto == pp) continue;
		if (pe->proto->network != pp->subportproto->network) continue;
		net = pe->exportproto->network;
		if (net == NONETWORK) continue;
		net_putportonnet(pp, net);
		(void)net_addnametonet(pp->protoname, 0, net, NOARCINST, pp, pp->parent);
		if (pp->network != oldnet) return(TRUE);
		return(FALSE);
	}

	/* give up and assign a new net number */
	net = net_newnetwork(np);
	if (net_debug)
		ttyputmsg(M_("Network: creating new network for %s (gave up on all other tests)"),
			pp->protoname);
	(void)net_addnametonet(pp->protoname, 0, net, NOARCINST, pp, np);
	net_putportonnet(pp, net);
	if (np->cellview == el_schematicview || (np->cellview->viewstate&MULTIPAGEVIEW) != 0)
	{
		if (portispower(pp))
		{
			special = -1;
			net_setglobalnet(&special, GLOBALNETPOWER, 0, net, PWRPORT, np);
		} else if (portisground(pp))
		{
			special = -1;
			net_setglobalnet(&special, GLOBALNETGROUND, 0, net, GNDPORT, np);
		}
	}
	return(TRUE);
}

/*
 * Routine to update the array information on node "ni" (its name has changed).
 */
void net_setnodewidth(NODEINST *ni)
{
	REGISTER INTBIG newarraysize;
	REGISTER VARIABLE *var;

	/* see if this node can be arrayed */
	if (!isschematicview(ni->parent)) newarraysize = 0; else
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var == NOVARIABLE)
		{
			newarraysize = 0;
		} else
		{
			newarraysize = net_buswidth((CHAR *)var->addr);
		}
	}
	if (newarraysize != ni->arraysize)
	{
		ni->arraysize = newarraysize;
	}
}

/*
 * routine to add the name "name" to network "newnetwork", assuming that it
 * is connected to arc "ai" (which may be NOARCINST if it is not connected)
 * or port "pp" (which may be NOPORTPROTO if it is not an export) in cell
 * "cell".  If this addition causes a merging of networks, the routine
 * returns a positive value.  If the network name is invalid, the routine
 * returns a negative value.
 */
INTBIG net_addnametonet(CHAR *name, INTBIG tempname, NETWORK *newnetwork, ARCINST *ai,
	PORTPROTO *pp, NODEPROTO *cell)
{
	REGISTER NETWORK *net, *subnet;
	INTBIG wid, busexists;
	REGISTER INTBIG count, i, bits, dontname;
	REGISTER BOOLEAN recheck;
	REGISTER CHAR *pt, *opt;
#ifdef NEWRENUM
	CHAR *netname;
#else
	CHAR **strings, *netname;
#endif

	/* special case if this is a temporary name */
	if (tempname != 0)
	{
		/* if the net already has a real name, check it out */
		if (newnetwork->namecount > 0)
		{
			/* if the existing network name isn't temporary, ignore this temp one */
			if (newnetwork->tempname == 0) return(-1);

			/* if the new name is arrayed and the old isn't, use the new */
			for(pt = name; *pt != 0; pt++) if (*pt == '[') break;
			for(opt = networkname(newnetwork, 0); *opt != 0; opt++) if (*opt == '[') break;
			if (*opt == '[' || *pt == 0) return(-1);
			((NetName*)newnetwork->netnameaddr)->removeNet(newnetwork);
			newnetwork->namecount = 0;
		}

		/* mark the network as having a temporary name */
		newnetwork->tempname = 1;
	} else
	{
		/* this is a real name: if the net has a temporary one, kill it */
		if (newnetwork->tempname != 0 && newnetwork->namecount == 1)
		{
			((NetName*)newnetwork->netnameaddr)->removeNet(newnetwork);
			newnetwork->namecount = 0;
			newnetwork->tempname = 0;
		}
	}

	/* see if the network already has this name */
#ifdef NEWRENUM
	NetName *nn = cell->netd->findNetName(name, TRUE);
	if (!net_nethasname(newnetwork, nn))
#else
	if (!net_nethasname(newnetwork, name))
#endif
	{
		/* see if the net can be a bus */
		bits = APUNKNOWN;
		if (pp != NOPORTPROTO)
		{
			/* determine whether this port can be a bus */
			for(i=0; pp->connects[i] != NOARCPROTO; i++)
			{
				bits = (pp->connects[i]->userbits&AFUNCTION) >> AFUNCTIONSH;
				if (bits == APBUS) break;
			}
		} else if (ai != NOARCINST)
		{
			/* determine whether this arc can be a bus */
			bits = (ai->proto->userbits&AFUNCTION) >> AFUNCTIONSH;
		}

		/* check net name for validity */
		recheck = FALSE;
#ifdef NEWRENUM
		count = nn->busWidth();
		if (bits != APBUS && count != 1)
		{
			ttyputerr(_("Warning (cell %s): network '%s' cannot name a single wire"),
				describenodeproto(cell), name);
			return(-1);
		}
#else
		count = net_evalbusname(bits, name, &strings, ai, cell, 1);
		if (count < 0)
		{
			ttyputerr(_("Warning (cell %s): network name '%s' is invalid"), describenodeproto(cell), name);
			return(-1);
		}
#endif

		/* check width for consistency */
		busexists = 0;
		if (newnetwork->namecount > 0)
		{
			wid = net_buswidth(networkname(newnetwork, 0));
			if (count != wid)
			{
				ttyputerr(_("Warning (cell %s): networks '%s' and '%s' cannot connect (different width)"),
					describenodeproto(cell), name, networkname(newnetwork, 0));
				return(-1);
			}
		}
		if (count > 1 && count == newnetwork->buswidth)
		{
			/* add new names of signals in the bus to those already on the network */
			for(i=0; i<count; i++)
			{
#ifdef NEWRENUM
				(void)allocstring(&netname, nn->subName(i)->name(), el_tempcluster);
#else
				(void)allocstring(&netname, strings[i], el_tempcluster);
#endif
				subnet = newnetwork->networklist[i];
				net = NONETWORK;
				if ((net_options&NETCONCOMMONNAME) != 0 || cell->cellview == el_schematicview ||
					(ai != NOARCINST && ai->proto->tech == sch_tech) ||
						(pp != NOPORTPROTO && pp->parent->primindex != 0 && pp->parent->tech == sch_tech))
							net = net_getunifiednetwork(netname, cell, subnet);

				/* special case if this is a temporary name */
				dontname = 0;
				if (tempname != 0)
				{
					/* this is a temp name: ignore if already a real one */
					if (subnet->namecount > 0) dontname = 1; else
						subnet->tempname = 1;
				} else
				{
					/* this is a real name: if the net has a temporary one, kill it */
					if (subnet->tempname != 0 && subnet->namecount == 1)
					{
						((NetName*)subnet->netnameaddr)->removeNet(subnet);
						subnet->namecount = 0;
						subnet->tempname = 0;
					}
				}

#ifdef NEWRENUM
				if (dontname == 0) (void)net_namenet(nn->subName(i), subnet);
#else
				if (dontname == 0) (void)net_namenet(netname, subnet);
#endif
				if (net != NONETWORK && net != subnet)
				{
					if (net_mergenet(net, subnet)) recheck = TRUE;
				}
				efree(netname);
			}
			busexists++;
		}

		/* for busses, name the network and install appropriate bus links */
		if (count > 1)
		{
#ifdef NEWRENUM
			if (!net_namenet(nn, newnetwork))
				if (busexists == 0)
					net_ensurebusses(newnetwork, nn, tempname);
#else
			if (!net_namenet(name, newnetwork))
				if (busexists == 0)
					net_ensurebusses(newnetwork, count, strings, tempname);
#endif
		} else
		{
#ifdef NEWRENUM
			(void)net_namenet(nn, newnetwork);
#else
			(void)net_namenet(strings[0], newnetwork);
#endif
		}

		/* reset "tempname" field since "net_namenet" wipes it out */
		if (tempname != 0) newnetwork->tempname = 1;

		/* see if this net name is in use */
		net = NONETWORK;
		if ((net_options&NETCONCOMMONNAME) != 0 || cell->cellview == el_schematicview ||
			(ai != NOARCINST && ai->proto->tech == sch_tech) ||
				(pp != NOPORTPROTO && pp->parent->primindex != 0 && pp->parent->tech == sch_tech))
					net = net_getunifiednetwork(name, cell, newnetwork);

		/* merge the new network with an old one of the same name, if it exists */
		if (net != NONETWORK && net != newnetwork)
		{
			if (net_mergenet(net, newnetwork)) recheck = TRUE;
		}

		if (recheck) return(1);
	}
	return(0);
}

/*
 * recursive routine to re-do connectivity of entire library starting
 * at highest level cell.  Current point for re-doing connectivity is "np".
 * This routine is called when the node of a port has changed and all
 * instances of the cell need to be renumbered.  The "REDOCELLNET" flag on
 * cells prevents them from being checked multiple times.
 */
void net_recursivelymarkabove(NODEPROTO *np)
{
	net_startglobalwork(np->lib);

	/* skip this if already done */
	if ((np->userbits&REDOCELLNET) != 0) return;

	/* mark this cell to be renumbered */
	if (net_debug)
		ttyputmsg(M_("Network: cell %s is marked"), describenodeproto(np));
	np->userbits |= REDOCELLNET;

}

/*********************** NETLIST MAINTENANCE ROUTINES ***********************/

/*
 * routine to add network "net" to arc "ai"
 */
void net_putarconnet(ARCINST *ai, NETWORK *net)
{
	if (net == NONETWORK) return;
	ai->network = net;
	net->refcount++;
}

/*
 * routine to remove the network link from arc "ai".  Returns true if
 * the network has been deleted
 */
BOOLEAN net_takearcfromnet(ARCINST *ai)
{
	REGISTER NETWORK *net;

	net = ai->network;
	if (net == NONETWORK) return(FALSE);
	ai->network = NONETWORK;
	net_takearclinkfromnet(ai, net);
	net->refcount--;

	/* delete the network if all counts are zero */
	if (net->portcount <= 0 && net->refcount <= 0 && net->arccount <= 0 && net->buslinkcount <= 0)
	{
		net_killnetwork(net, ai->parent);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to add network "net" to port "pp"
 */
void net_putportonnet(PORTPROTO *pp, NETWORK *net)
{
	if (net == NONETWORK) return;
	pp->network = net;
	net->portcount++;
}

/*
 * routine to remove the network link from port "pp".  Returns true
 * if the network has been deleted
 */
BOOLEAN net_takeportfromnet(PORTPROTO *pp)
{
	REGISTER NETWORK *net;

	net = pp->network;
	if (net == NONETWORK) return(FALSE);
	pp->network = NONETWORK;
	net->portcount--;

	/* delete the network if all counts are zero */
	if (net->portcount <= 0 && net->refcount <= 0 && net->arccount <= 0 && net->buslinkcount <= 0)
	{
		net_killnetwork(net, pp->parent);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to add arc "ai" to the list of named arcs on network "net".
 */
void net_putarclinkonnet(NETWORK *net, ARCINST *ai)
{
	REGISTER ARCINST **arclist;
	REGISTER INTBIG i, newsize;

	if (net == NONETWORK) return;

	/* if there are no arcs on the net, add this */
	if (net->arccount <= 0)
	{
		if (net->arctotal == 0) net->arcaddr = (INTBIG)ai; else
			((ARCINST **)net->arcaddr)[0] = ai;
		net->arccount = 1;
		return;
	}

	/* check that the arc isn't already on the list */
	if (net->arctotal == 0)
	{
		if (net->arccount == 1 && (ARCINST *)net->arcaddr == ai) return;
	} else
	{
		for(i=0; i<net->arccount; i++)
			if (((ARCINST **)net->arcaddr)[i] == ai) return;
	}

	/* make sure there is space for the new arc pointer */
	if (net->arccount >= net->arctotal)
	{
		newsize = net->arctotal * 2;
		if (net->arccount >= newsize) newsize = net->arccount + 8;
		arclist = (ARCINST **)emalloc(((sizeof (ARCINST *)) * newsize),
			net->parent->lib->cluster);
		if (arclist == 0) return;

		/* load the array */
		for(i=0; i<net->arccount; i++)
		{
			if (net->arctotal == 0) arclist[i] = (ARCINST *)net->arcaddr; else
				arclist[i] = ((ARCINST **)net->arcaddr)[i];
		}
		if (net->arctotal > 0) efree((CHAR *)net->arcaddr);
		net->arctotal = (INTSML)newsize;
		net->arcaddr = (INTBIG)arclist;
	}

	/* add this arc and place the array on the net */
	((ARCINST **)net->arcaddr)[net->arccount] = ai;
	net->arccount++;
}

/*
 * routine to remove mention of arc "ai" on network "net"
 */
void net_takearclinkfromnet(ARCINST *ai, NETWORK *net)
{
	REGISTER INTBIG i;

	if (net == NONETWORK) return;
	if (net->arccount <= 0) return;

	/* special check if the network has no array of arcs */
	if (net->arctotal == 0)
	{
		if ((ARCINST *)net->arcaddr == ai) net->arccount--;
		return;
	}

	for(i=0; i<net->arccount; i++)
		if (((ARCINST **)net->arcaddr)[i] == ai)
	{
		/* network found in the list: remove it */
		((ARCINST **)net->arcaddr)[i] = ((ARCINST **)net->arcaddr)[net->arccount-1];
		net->arccount--;
		return;
	}
}

#ifdef NEWRENUM
/*
 * routine to ensure that there are networks with the names of each
 * individual bus member described by subnames of net name "nn".
 * They are all part of the network "net"
 */
void net_ensurebusses(NETWORK *net, NetName *nn, INTBIG tempname)
{
	REGISTER INTBIG i, dontname;
	REGISTER NETWORK *singlenet;
	CHAR *subnetname;
	INTBIG count = nn->busWidth();

	if (net->buswidth == count)
	{
		/* already have signals in place: name them */
		for(i=0; i<count; i++)
		{
			singlenet = getnetwork(nn->subName(i)->name(), net->parent);
			if (singlenet == NONETWORK)
			{
				singlenet = net->networklist[i];

				/* special case if this is a temporary name */
				dontname = 0;
				if (tempname != 0)
				{
					/* this is a temp name: ignore if already a real one */
					if (singlenet->namecount > 0) dontname = 1; else
						singlenet->tempname = 1;
				} else
				{
					/* this is a real name: if the net has a temporary one, kill it */
					if (singlenet->tempname != 0 && singlenet->namecount > 0)
					{
						((NetName*)singlenet->netnameaddr)->removeNet(singlenet);
						singlenet->namecount = 0;
						singlenet->tempname = 0;
					}
				}
				if (dontname == 0)
					(void)net_namenet(nn->subName(i), net->networklist[i]);
			}
		}
		return;
	}

	net->buswidth = (INTSML)count;
	net_ensuretempbusname(net);
	net->networklist = (NETWORK **)emalloc(((sizeof (NETWORK *)) * count),
		net->parent->lib->cluster);
	if (net->networklist == 0) return;

	/* generate network names for each array member */
	for(i=0; i<count; i++)
	{
		(void)allocstring(&subnetname, nn->subName(i)->name(), el_tempcluster);
		net->networklist[i] = getnetwork(subnetname, net->parent);
		efree(subnetname);
		if (net->networklist[i] == NONETWORK)
		{
			net->networklist[i] = net_newnetwork(net->parent);
			(void)net_namenet(nn->subName(i), net->networklist[i]);
		}
		net->networklist[i]->buslinkcount++;
		net->networklist[i]->tempname = (INTSML)tempname;
	}
}

#else
/*
 * routine to ensure that there are networks with the names of each
 * individual bus member described by the "count" signals in "strings".
 * They are all part of the network "net"
 */
void net_ensurebusses(NETWORK *net, INTBIG count, CHAR **strings, INTBIG tempname)
{
	REGISTER INTBIG i, dontname;
	REGISTER NETWORK *singlenet;
	CHAR *subnetname;

	if (net->buswidth == count)
	{
		/* already have signals in place: name them */
		for(i=0; i<count; i++)
		{
			singlenet = getnetwork(strings[i], net->parent);
			if (singlenet == NONETWORK)
			{
				singlenet = net->networklist[i];

				/* special case if this is a temporary name */
				dontname = 0;
				if (tempname != 0)
				{
					/* this is a temp name: ignore if already a real one */
					if (singlenet->namecount > 0) dontname = 1; else
						singlenet->tempname = 1;
				} else
				{
					/* this is a real name: if the net has a temporary one, kill it */
					if (singlenet->tempname != 0 && singlenet->namecount > 0)
					{
						((NetName*)singlenet->netnameaddr)->removeNet(singlenet);
						singlenet->namecount = 0;
						singlenet->tempname = 0;
					}
				}
				if (dontname == 0)
					(void)net_namenet(strings[i], net->networklist[i]);
			}
		}
		return;
	}

	net->buswidth = (INTSML)count;
	net_ensuretempbusname(net);
	net->networklist = (NETWORK **)emalloc(((sizeof (NETWORK *)) * count),
		net->parent->lib->cluster);
	if (net->networklist == 0) return;

	/* generate network names for each array member */
	for(i=0; i<count; i++)
	{
		(void)allocstring(&subnetname, strings[i], el_tempcluster);
		net->networklist[i] = getnetwork(subnetname, net->parent);
		efree(subnetname);
		if (net->networklist[i] == NONETWORK)
		{
			net->networklist[i] = net_newnetwork(net->parent);
			(void)net_namenet(strings[i], net->networklist[i]);
		}
		net->networklist[i]->buslinkcount++;
		net->networklist[i]->tempname = (INTSML)tempname;
	}
}

#endif

/*
 * routine to get "i"th name of network "net". Returns 0 if there is no "i"th name
 */
CHAR *networkname(NETWORK *net, INTBIG i)
{
	if (i < 0 || i >= net->namecount) return(0);
	NetName *nn = (net->namecount > 1 ? ((NetName**)net->netnameaddr)[i] : (NetName*)net->netnameaddr);
	return(nn->name());
}

/*
 * routine to find network "netname" in cell "cell".  Returns NONETWORK
 * if it cannot be found
 */
NETWORK *net_getnetwork(CHAR *netname, NODEPROTO *cell)
{
	REGISTER NETWORK *net;
	NetName *nn = cell->netd->findNetName( netname, FALSE );
	if (nn)
	{
		net = nn->firstNet();
		if (net != NONETWORK) return(net);
	}
	return(NONETWORK);
}

#ifdef NEWRENUM
/*
 * routine to give network "net" the name "nn".  Returns true if the network
 * already has the name.
 */
BOOLEAN net_namenet(NetName *nn, NETWORK *net)
#else
/*
 * routine to give network "net" the name "name".  Returns true if the network
 * already has the name.
 */
BOOLEAN net_namenet(CHAR *name, NETWORK *net)
#endif
{
	REGISTER INTBIG i, j, match;

	if (net->tempname != 0)
	{
		/* remove temporary name */
		if (net->namecount > 0) ((NetName*)net->netnameaddr)->removeNet(net);
		net->namecount = 0;
		net->tempname = 0;
	}
#ifdef NEWRENUM
	CHAR *name = nn->name();
#else
	NetName *nn = net->parent->netd->findNetName( name, TRUE );
#endif
	if (net->namecount == 0)
	{
		/* network is unnamed: name it */
		nn->addNet( net );
		net->namecount++;
		net->netnameaddr = (INTBIG)nn;
		return(FALSE);
	}

	/* see if the network already has this name */
	for(i=0; i<net->namecount; i++)
	{
		match = namesame(name, networkname(net, i));
		if (match == 0) return(TRUE);
		if (match < 0) break;
	}
	nn->addNet( net );
	NetName **newnameaddr = (NetName**)emalloc((net->namecount + 1)*sizeof(NetName*), net->parent->lib->cluster);
	newnameaddr[i] = nn;
	if (net->namecount == 1)
	{
		newnameaddr[1 - i] = (NetName*)net->netnameaddr;
	} else
	{
		NetName **oldnameaddr = (NetName**)net->netnameaddr;
		for(j=0; j < i; j++)
			newnameaddr[j] = oldnameaddr[j];
		for(j=i; j < net->namecount; j++)
			newnameaddr[j+1] = oldnameaddr[j];
		efree((CHAR *)oldnameaddr);
	}
	net->netnameaddr = (INTBIG)newnameaddr;
	net->namecount++;
	return(FALSE);
}

/*
 * routine to get a new network object from cell "cell" and return the
 * address.  Returns NONETWORK on error.
 */
NETWORK *net_newnetwork(NODEPROTO *cell)
{
	REGISTER NETWORK *net;

	if (cell->lib->freenetwork != NONETWORK)
	{
		/* take from list of unused networks objects in the cell */
		net = cell->lib->freenetwork;
		cell->lib->freenetwork = net->nextnetwork;
	} else
	{
		/* allocate a new network object */
		net = (NETWORK *)emalloc(sizeof(NETWORK), cell->lib->cluster);
		if (net == 0) return(NONETWORK);
		net->arctotal = 0;
	}
	net->netnameaddr = 0;
	net->namecount = 0;
	net->tempname = 0;
	net->arccount = net->refcount = net->portcount = net->buslinkcount = 0;
	net->parent = cell;
	net->globalnet = -1;
	net->buswidth = 1;
	net->firstvar = NOVARIABLE;
	net->numvar = 0;

	/* link the network into the cell */
	net->nextnetwork = cell->firstnetwork;
	net->prevnetwork = NONETWORK;
	if (cell->firstnetwork != NONETWORK) cell->firstnetwork->prevnetwork = net;
	cell->firstnetwork = net;
	return(net);
}

/*
 * routine to return network "net" to cell "cell"
 */
void net_killnetwork(NETWORK *net, NODEPROTO *cell)
{
	if (net->prevnetwork == NONETWORK) cell->firstnetwork = net->nextnetwork; else
		net->prevnetwork->nextnetwork = net->nextnetwork;
	if (net->nextnetwork != NONETWORK)
		net->nextnetwork->prevnetwork = net->prevnetwork;

	/* if this is a global network, mark the global signal "available" */
	if (net->globalnet >= 0)
	{
		if (net->globalnet < cell->globalnetcount)
		{
			cell->globalnetworks[net->globalnet] = NONETWORK;
			if (net_debug)
				ttyputmsg(M_("Global network '%s' deleted from %s"), cell->globalnetnames[net->globalnet],
					describenodeproto(cell));
			/* don't mark above if the entire cell will be renumbered */
			if (!net_globalwork || (cell->userbits&REDOCELLNET) == 0)
				net_recursivelymarkabove(cell);
		}
	}

	/* routine to remove bus links */
	net_removebuslinks(net);

	/* delete the network */
	net_freenetwork(net, cell);
}

/*
 * routine to remove bus links to network "net"
 */
void net_removebuslinks(NETWORK *net)
{
	REGISTER NETWORK *subnet;
	REGISTER INTBIG i;

	if (net->buswidth <= 1) return;

	for(i=0; i<net->buswidth; i++)
	{
		subnet = net->networklist[i];
		if (subnet == NONETWORK) continue;
		subnet->buslinkcount--;
		if (subnet->portcount <= 0 && subnet->refcount <= 0 &&
			subnet->arccount <= 0 && subnet->buslinkcount <= 0)
				net_killnetwork(subnet, subnet->parent);
	}
	efree((CHAR *)net->networklist);
	net->buswidth = 1;
}

void net_freenetwork(NETWORK *net, NODEPROTO *cell)
{
	if (net->namecount == 1)
	{
		((NetName*)net->netnameaddr)->removeNet(net);
	} else if (net->namecount > 1)
	{
		NetName **netnames = (NetName**)net->netnameaddr;
		for (INTBIG i = 0; i < net->namecount; i++)
			netnames[i]->removeNet(net);
		efree((CHAR*)netnames);
	}
	net->namecount = 0;
	net->netnameaddr = 0;
	if (net->buswidth > 1) efree((CHAR *)net->networklist);
	net->buswidth = 1;
	if (net->numvar != 0) db_freevars(&net->firstvar, &net->numvar);

	/* insert in linked list of free networks in the cell */
	net->nextnetwork = cell->lib->freenetwork;
	cell->lib->freenetwork = net;
}

/*
 * routine to replace all occurrences of "oldnet" with "newnet".  Returns
 * true if ports were changed.  NOTE: merged busses must have same width
 * and this routine cannot handle that error condition properly.  Shouldn't
 * ever happen.
 */
BOOLEAN net_mergenet(NETWORK *oldnet, NETWORK *newnet)
{
	REGISTER INTBIG i;
	REGISTER BOOLEAN ret;
	REGISTER BOOLEAN portschanged;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER NETWORK *net;

	if (net_debug)
		ttyputmsg(M_("Merging old network %ld into %ld"), oldnet, newnet);

	/* merging is easy if the nets are already the same */
	if (oldnet == newnet) return(FALSE);

	/* cannot merge busses of dissimilar width (system error) */
	if (oldnet->buswidth != newnet->buswidth)
	{
		ttyputerr(_("Warning (cell %s): cannot connect net '%s' (%d wide) and net '%s' (%d wide)"),
			describenodeproto(newnet->parent), describenetwork(oldnet), oldnet->buswidth,
				describenetwork(newnet), newnet->buswidth);
		return(TRUE);
	}

	/* if one of the nets has a temporary name, drop it */
	if (newnet->tempname != 0 && newnet->namecount > 0 && oldnet->namecount > 0)
	{
		/* new network has temporary name, old has real name */
		((NetName*)newnet->netnameaddr)->removeNet(newnet);
		newnet->namecount = 0;
		newnet->tempname = 0;
	}
	if (oldnet->tempname == 0 || newnet->namecount == 0)
	{
		/* add the names of old network on the new one */
#ifdef NEWRENUM
		if (oldnet->namecount == 1)
		{
			NetName *nn = (NetName*)oldnet->netnameaddr;
			(void)net_namenet(nn, newnet);
		} else if (oldnet->namecount > 1)
		{
			NetName **netnames = (NetName**)oldnet->netnameaddr;
			for(i=0; i<oldnet->namecount; i++)
			{
				(void)net_namenet(netnames[i], newnet);
			}
		}
#else
		for(i=0; i<oldnet->namecount; i++)
		{
			(void)net_namenet(networkname(oldnet, i), newnet);
		}
#endif
	}

	/* if old net is global, set that on new net */
	if (oldnet->globalnet >= 0)
	{
		newnet->globalnet = oldnet->globalnet;
		newnet->parent->globalnetworks[oldnet->globalnet] = newnet;
		oldnet->globalnet = -1;
	}

	/* if there are bus links on the old network, switch it to the new one */
	if (oldnet->buslinkcount != 0)
	{
		for(net = oldnet->parent->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net->buswidth <= 1) continue;
			for(i=0; i<net->buswidth; i++)
			{
				if (net->networklist[i] != oldnet) continue;
				net->networklist[i] = newnet;
				newnet->buslinkcount++;
				oldnet->buslinkcount--;

				/* delete the network if all counts are zero */
				if (oldnet->portcount <= 0 && oldnet->refcount <= 0 &&
					oldnet->arccount <= 0 && oldnet->buslinkcount <= 0)
				{
					net_killnetwork(oldnet, oldnet->parent);
					return(FALSE);
				}
			}
			if (oldnet->buslinkcount == 0) break;
		}
	}
#if 0
	/* place arc links on new network */
	for(i=0; i<oldnet->arccount; i++)
	{
		if (oldnet->arctotal == 0)
			net_putarclinkonnet(newnet, (ARCINST *)oldnet->arcaddr); else
				net_putarclinkonnet(newnet, ((ARCINST **)oldnet->arcaddr)[i]);
	}
#endif
	/* replace arc references to the old network with the new one */
	for(ai = oldnet->parent->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network == oldnet)
		{
			ret = net_takearcfromnet(ai);
			net_putarconnet(ai, newnet);
			if (ret) return(FALSE);
		}
	}

	/* replace port references to the old network with the new one */
	portschanged = FALSE;
	for(pp = oldnet->parent->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->network != oldnet) continue;
		ret = net_takeportfromnet(pp);
		net_putportonnet(pp, newnet);
		if (ret) return(TRUE);
		portschanged = TRUE;
	}

	/* handle globals inside subcells */
	if (oldnet->refcount > 0)
	{
		newnet->refcount += oldnet->refcount;
		oldnet->refcount = 0;

		/* delete the network if all counts are zero */
		if (oldnet->portcount <= 0 && oldnet->refcount <= 0 &&
			oldnet->arccount <= 0 && oldnet->buslinkcount <= 0)
		{
			net_killnetwork(oldnet, oldnet->parent);
			return(TRUE);
		}
	} else
	{
		ttyputmsg(M_("Network tool internal error in net_mergenet cell %s: Why oldnet '%s' hasn't been deleted yet ?"), 
			describenodeproto(newnet->parent), describenetwork(newnet));
	}

	return(portschanged);
}

/*
 * routine to initialize global evaluation
 */
void net_startglobalwork(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *olib;

	/* if this is not the first request for global evaluation, quit */
	if (!net_globalwork)
	{
		/* clear all library flags */
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			olib->userbits &= ~REDOCELLLIB;
		net_globalwork = TRUE;
	}

	/* if this library hasn't been started, do so */
	if ((lib->userbits&REDOCELLLIB) == 0)
	{
		lib->userbits |= REDOCELLLIB;

		/* clear all flags in this library */
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			np->userbits &= ~REDOCELLNET;
	}
}

/*
 * rename network "oldname" to "newname" in cell "np"
 */
void net_renamenet(CHAR *oldname, CHAR *newname, NODEPROTO *np)
{
	REGISTER CHAR *pt, *arcname, *startpt;
	CHAR *netname;
	REGISTER INTBIG k, found, len;
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER NETWORK *net;
	REGISTER ARCINST *ai;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	/* check for duplicate name */
	if (namesame(oldname, newname) == 0)
	{
		ttyputmsg(_("Network name has not changed in cell %s"), describenodeproto(np));
		return;
	}

	/* validate the names */
	for(pt = oldname; *pt != 0; pt++) if (*pt == '[') break;
	if (*pt == '[')
	{
		ttyputerr(_("Must rename unqualified networks (without the '[') in cell %s"),
			describenodeproto(np));
		return;
	}
	for(pt = newname; *pt != 0; pt++) if (*pt == '[') break;
	if (*pt == '[')
	{
		ttyputerr(_("New name must be unqualified (without the '[') in cell %s"),
			describenodeproto(np));
		return;
	}

	/* make sure new name is not in use */
	for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		for(k=0; k<net->namecount; k++)
		{
			startpt = net_findnameinbus(newname, networkname(net, k));
			if (startpt != 0)
			{
				ttyputerr(_("Network name '%s' already exists in cell %s"), newname,
					describenodeproto(np));
				return;
			}
		}
	}

	/* substitute in all arcs */
	len = estrlen(oldname);
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* see if the arc has a name */
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) continue;

		/* parse the network name */
		arcname = (CHAR *)var->addr;
		found = 0;
		for(;;)
		{
			startpt = net_findnameinbus(oldname, arcname);
			if (startpt == 0) break;
			infstr = initinfstr();
			for(pt = arcname; pt < startpt; pt++)
				addtoinfstr(infstr, *pt);
			addstringtoinfstr(infstr, newname);
			pt += len;
			addstringtoinfstr(infstr, pt);
			arcname = returninfstr(infstr);
			found++;
		}
		if (found > 0)
		{
			/* rename the arc */
			(void)allocstring(&netname, arcname, el_tempcluster);
			TDCOPY(descript, var->textdescript);
			startobjectchange((INTBIG)ai, VARCINST);
			var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)netname, VSTRING|VDISPLAY);
			if (var == NOVARIABLE) continue;
			modifydescript((INTBIG)ai, VARCINST, var, descript);
			endobjectchange((INTBIG)ai, VARCINST);
			efree((CHAR *)netname);
		}
	}

	/* substitute in all exports */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (namesamen(pp->protoname, oldname, len) != 0) continue;
		if (pp->protoname[len] != 0 && pp->protoname[len] != '[') continue;
		if (pp->protoname[len] == 0)
		{
			startobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
			setval((INTBIG)pp, VPORTPROTO, x_("protoname"), (INTBIG)newname, VSTRING);
			endobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
		} else
		{
			infstr = initinfstr();
			addstringtoinfstr(infstr, newname);
			addstringtoinfstr(infstr, &pp->protoname[len]);
			startobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
			setval((INTBIG)pp, VPORTPROTO, x_("protoname"), (INTBIG)returninfstr(infstr), VSTRING);
			endobjectchange((INTBIG)pp->subnodeinst, VNODEINST);
		}
	}

	ttyputmsg(_("Network '%s' renamed to '%s'"), oldname, newname);
}

/*
 * Routine to find the name "wantname" in the bus name "busname".
 * Returns the character position where the name starts (0 if not found).
 */
CHAR *net_findnameinbus(CHAR *wantname, CHAR *busname)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG len;

	len = estrlen(wantname);
	for(pt = busname; *pt != 0; pt++)
	{
		if (namesamen(pt, wantname, len) == 0)
		{
			if (pt[len] == 0 || pt[len] == '[' || pt[len] == ',')
				return(pt);
		}
		while (*pt != 0 && *pt != '[' && *pt != ',') pt++;
		while (*pt == '[')
		{
			pt++;
			while (*pt != 0 && *pt != ']') pt++;
			if (*pt == ']') pt++;
		}
		if (*pt == 0) break;
	}
	return(0);
}

/*********************** NetCellPrivate class ***********************/

void* NetCellPrivate::operator new( size_t size )
{
	ttyputerr(M_("operator NetCellPrivate(size,cluster) should be used"));
	Q_UNUSED( size );
	return (void*)0;
}

void* NetCellPrivate::operator new( size_t size, CLUSTER *cluster )
{
	return (void*)emalloc( size, cluster );
}

void NetCellPrivate::operator delete( void* obj )
{
	if(obj) efree((CHAR *)obj);
}

#ifndef MACOS
void NetCellPrivate::operator delete( void* obj, CLUSTER *cluster )
{
	Q_UNUSED( cluster );
	if(obj) efree((CHAR *)obj);
}
#endif

NetCellPrivate::NetCellPrivate( NODEPROTO *np, CLUSTER *cluster )
	: _np( np ), _cluster( cluster ), _netnamecount( 0 ), _netshorts( 0 )
{
	_netnametotal = 5;
	_netnamehash = (NetName**)emalloc(_netnametotal*sizeof(NetName*), _cluster);
	for (INTBIG i = 0; i < _netnametotal; i++)
		_netnamehash[i] = 0;
}

void net_initnetprivate(NODEPROTO *np)
{
	if (np->netd) return;
	np->netd = new (np->lib->cluster)NetCellPrivate(np, np->lib->cluster);
}

NetCellPrivate::~NetCellPrivate()
{
	if ( _netshorts ) delete _netshorts;
	if (_netnamehash)
	{
		for (INTBIG i = 0; i < _netnametotal; i++)
		{
			if (_netnamehash[i]) delete _netnamehash[i];
		}
		efree((CHAR *)_netnamehash);
	}
}

void net_freenetprivate(NODEPROTO *np)
{
	if (np != NONODEPROTO && np->netd)
	{
		delete np->netd;
		np->netd = 0;
	}
}

BOOLEAN NetCellPrivate::updateShorts()
{
	if (_netshorts && _netshorts->isConsistent()) return FALSE;
	if (_netshorts) delete _netshorts;
	_netshorts = new (_cluster) NetCellShorts(_np, _cluster);
	return TRUE;
}

#ifdef NEWRENUM
/* for some reason, using "this" in the NetName constructor tickles a compiler bug */
static NetName *newNetName( CLUSTER *cluster, NetCellPrivate *ncp, CHAR *name)
{
    return new (cluster) NetName( ncp, name );
}

NetName *NetCellPrivate::findNetName( CHAR *name, BOOLEAN insert )
{
	NetName *nn;
	CHAR *normname, *pt, **strings;
	INTBIG i, j, hash, count;

	/* find exactly */
	hash = db_namehash(name);
	i = hash % _netnametotal;
	for(j=1; j<=_netnametotal; j += 2)
	{
		nn = _netnamehash[i];
		if (!nn) break;
		if (namesame(name, nn->_name) == 0) return(nn);
		i += j;
		if (i >= _netnametotal) i -= _netnametotal;
	}

	/* clean blanks, if any */
	for (pt = name; *pt != 0; pt++)
	{
		if (*pt == ' ' || *pt == '\t') break;
	}
	if (*pt != 0)
	{
		normname = (CHAR*)emalloc( (estrlen(name) + 1) * sizeof(CHAR), _cluster);
		for (pt = name, j = 0; *pt != 0; pt++)
		{
			if (*pt == ' ' || *pt == '\t') continue;
			normname[j++] = *pt;
		}
		normname[j] = 0;
		ttyputmsg(M_("normalizeNetName {%s} -> {%s}"), name, normname);
		nn = findNetName( normname, insert );
		efree(normname);
		return (nn);
	}

	/* not found */
	if (!insert) return(0);

	/* Evaluate busname */
	count = net_evalbusname(APBUS, name, &strings, NOARCINST, _np, TRUE);

	/* Error in name - use patched */
	if (count == 1 && estrcmp(strings[0], name) != 0)
	{
		ttyputmsg(M_("patchNetName {%s} -> {%s}"), name, strings[0]);
		return findNetName( strings[0], TRUE );
	}

	/* for some reason, using "this" in the NetName constructor tickles a compiler bug */
	nn = newNetName( _cluster, this, name );
	insertNetName( nn, hash );
	if (count > 1)
		nn->setBusWidth( count, strings );
	return(nn);
}

#else
NetName *NetCellPrivate::findNetName( CHAR *name, BOOLEAN insert )
{
	NetName *nn;
	NetCellPrivate *thisone;

	INTBIG hash = db_namehash(name);
	INTBIG i = hash % _netnametotal;
	for(INTBIG j=1; j<=_netnametotal; j += 2)
	{
		nn = _netnamehash[i];
		if (!nn) break;
		if (namesame(name, nn->_name) == 0) return(nn);
		i += j;
		if (i >= _netnametotal) i -= _netnametotal;
	}
	if (!insert) return(0);

	/* for some reason, using "this" in the NetName constructor tickles a compiler bug */
	thisone = this;
	nn = new (_cluster) NetName( thisone, name );
	insertNetName( nn, hash );
	return(nn);
}
#endif

void NetCellPrivate::insertNetName( NetName *nn, INTBIG hash )
{
	/* resize and rehash if necessary */
	if ((_netnamecount+1) >= _netnametotal/2)
		rehashNetNames();

	/* insert new name */
	INTBIG i = hash % _netnametotal;
	for(INTBIG j=1; j<=_netnametotal; j += 2)
	{
		if (_netnamehash[i] == 0)
		{
			_netnamehash[i] = nn;
			_netnamecount++;
			return;
		}
		i += j;
		if (i >= _netnametotal) i -= _netnametotal;
	}
	ttyputmsg(M_("Error in NetCellPrivate::insertNetName"));
}

#ifdef NEWRENUM
NetName *NetCellPrivate::addNetName( CHAR *name )
{
	NetName *nn;
	NetCellPrivate *thisone;

	INTBIG hash = db_namehash(name);
	INTBIG i = hash % _netnametotal;
	for(INTBIG j=1; j<=_netnametotal; j += 2)
	{
		nn = _netnamehash[i];
		if (!nn) break;
		if (namesame(name, nn->_name) == 0) return(nn);
		i += j;
		if (i >= _netnametotal) i -= _netnametotal;
	}

	/* for some reason, using "this" in the NetName constructor tickles a compiler bug */
	thisone = this;
	nn = new (_cluster) NetName( thisone, name );
	insertNetName( nn, hash );
	return(nn);
}
#endif

void NetCellPrivate::rehashNetNames()
{
	INTBIG i, j, k;
	INTBIG oldnetnametotal = _netnametotal;
	NetName **oldnetnamehash = _netnamehash;
	_netnametotal = pickprime((_netnamecount + 1)*4);
	_netnamehash = (NetName**)emalloc(_netnametotal*sizeof(NetName*), _cluster);
	for (k = 0; k < _netnametotal; k++) _netnamehash[k] = 0;
	for (k = 0; k < oldnetnametotal; k++)
	{
		if (oldnetnamehash[k] == 0) continue;
		i = db_namehash(oldnetnamehash[k]->_name) % _netnametotal;
		for(j=1; j<=_netnametotal; j += 2)
		{
			if (_netnamehash[i] == 0)
			{
				_netnamehash[i] = oldnetnamehash[k];
				break;
			}
			i += j;
			if (i >= _netnametotal) i -= _netnametotal;
		}
	}
	if (oldnetnamehash) efree((CHAR *)oldnetnamehash);
}

void NetCellPrivate::docheck()
{
	INTBIG i, count;
	NETWORK *net;

	if (_netnamecount >= _netnametotal/2)
		ttyputmsg(M_("Network hash table of faceet %s is too dense"), describenodeproto(_np));
	count = 0;
	INTBIG totalnetcount = 0;
	INTBIG totalbaserefcount = 0;
	for (i = 0; i < _netnametotal; i++)
	{
		NetName *nn = _netnamehash[i];
		if (!nn) continue;
		nn->docheck();
		count++;
		if (nn->_baseNetName)
			totalnetcount += nn->_netcount;
		totalbaserefcount += nn->_baseRefCount;
	}
	if (count != _netnamecount)
		ttyputmsg(M_("Network hash table of cell %s: netcount is incorrect"), describenodeproto(_np));
	if (totalnetcount != totalbaserefcount)
		ttyputmsg(M_("In cell %s totalnetcount=%ld totalbaserefcount=%ld"), describenodeproto(_np),
			totalnetcount, totalbaserefcount);
	for (net = _np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->namecount > 1)
		{
			if (net->tempname)
				ttyputmsg(M_("Network with temporary name has other names in cell %s"), describenodeproto(_np));
			NetName **netnames = (NetName**)net->netnameaddr;
			for (i = 0; i < net->namecount; i++)
			{
				NetName *nn = netnames[i];
				if (i > 0 && namesame(netnames[i-1]->name(), nn->name()) >= 0)
						ttyputmsg(M_("Network has wrong sorting of names in cell %s: %s %s"), describenodeproto(_np),
							netnames[i-1]->name(), nn->name());
				nn->checkNet(net);
			}
		}
		else if (net->namecount == 1)
		{
			NetName *nn = (NetName*)net->netnameaddr;
			nn->checkNet(net);
		}
	}
}

#ifdef NEWRENUM
void NetCellPrivate::clearConns()
{
	for (INTBIG i = 0; i < _netnametotal; i++)
	{
		NetName *nn = _netnamehash[i];
		if (nn == 0) continue;
		nn->_conn = 0;
	}
}

void NetCellPrivate::calcConns(INTBIG *count)
{
	for (INTBIG i = 0; i < _netnametotal; i++)
	{
		NetName *nn = _netnamehash[i];
		if (nn == 0) continue;
		if (nn->_conn <= 0) continue; /* ???????? maybe wrong when count == 0 */
		nn->_conn = (*count)++;
	}
}

void NetCellPrivate::showConns(INTBIG *deepmap)
{
	for (INTBIG i = 0; i < _netnametotal; i++)
	{
		NetName *nn = _netnamehash[i];
		if (nn == 0) continue;
		if (nn->_conn <= 0) continue;
		::printf("%ld %ld\t%s\n", nn->_conn, deepmap[nn->_conn], nn->name());
	}
}
#endif

void net_checknetprivate(NODEPROTO *np)
{
	np->netd->docheck();
}

/*********************** NetName class ***********************/

void* NetName::operator new( size_t size )
{
	ttyputerr(M_("operator NetName(size,cluster) should be used"));
	Q_UNUSED( size );
	return (void*)0;
}

void* NetName::operator new( size_t size, CLUSTER *cluster )
{
	return (void*)emalloc( size, cluster );
}

void NetName::operator delete( void* obj )
{
	if(obj) efree((CHAR *)obj);
}

#ifndef MACOS
void NetName::operator delete( void* obj, CLUSTER *cluster )
{
	Q_UNUSED( cluster );
	if(obj) efree((CHAR *)obj);
}
#endif

NetName::NetName( NetCellPrivate *npd, CHAR *name )
	: _npd( npd ), _netcount( 0 ), _netaddr( (INTBIG)NONETWORK )
	  , _baseNetName( 0 ), _baseRefCount( 0 )
#ifdef NEWRENUM
	  , _busWidth( 1 ), _netNameList( 0 )
#endif
{
	allocstring(&_name, name, _npd->_cluster);
	if (estrchr(_name, ',') != 0 || estrchr(name, ':') != 0) return;
	CHAR *pt = estrchr(_name, '[');
	if (pt == 0) return;
	*pt = 0;
#  ifdef NEWRENUM
	_baseNetName = npd->addNetName( _name );
#  else
	_baseNetName = npd->findNetName( _name, TRUE );
#  endif
	*pt = '[';
}

NetName::~NetName()
{
	efree(_name);
	if (_netcount > 1)
		efree((CHAR *)_netaddr);
#ifdef NEWRENUM
	if (_busWidth > 1)
		efree((CHAR *)_netNameList);
#endif
}


NETWORK *NetName::firstNet()
{
	return (_netcount <= 1 ? (NETWORK*)_netaddr : ((NETWORK**)_netaddr)[0]);
}

void NetName::addNet(NETWORK *net)
{
	NETWORK **newaddr;

	if (_netcount == 0)
	{
		_netaddr = (INTBIG)net;
		_netcount = 1;
		if (_baseRefCount != 0)
			checkArity();
		if (_baseNetName)
		{
			_baseNetName->_baseRefCount++;
			if (_baseNetName->_baseRefCount == 1)
				_baseNetName->checkArity();
		}
		return;
	}
	if (_netcount == 1)
	{
		NETWORK *onet = (NETWORK*)_netaddr;
		if (onet == net)
		{
			ttyputmsg(M_("NetName::addNet : netname %s already assigned"), _name);
			return;
		}
		newaddr = (NETWORK**)emalloc(2*sizeof(NETWORK*), _npd->_cluster);
		newaddr[0] = (NETWORK*)_netaddr;
	} else
	{
		INTBIG i;
		NETWORK **oldaddr = (NETWORK**)_netaddr;
		for (i = 0; i < _netcount; i++)
			if(oldaddr[i] == net)
			{
				ttyputmsg(M_("NetName::addNet : netname %s already assigned"), _name);
				return;
			}
		newaddr = (NETWORK**)emalloc((_netcount+1)*sizeof(NETWORK*), _npd->_cluster);
		for (i = 0; i < _netcount; i++)
			newaddr[i] = oldaddr[i];
		efree((CHAR *)oldaddr);
	}
	newaddr[_netcount] = net;
	_netcount++;
	_netaddr = (INTBIG)newaddr;
	if (_baseNetName)
	{
		_baseNetName->_baseRefCount++;
		if (_baseNetName->_baseRefCount == 1)
			_baseNetName->checkArity();
	}
}

void NetName::removeNet(NETWORK *net)
{
	if (_netcount == 1)
	{
		if (net != (NETWORK*)_netaddr)
		{
			ttyputmsg(M_("Netame::removeNet : netname %s is not connected with this"), _name);
			return;
		}
		_netaddr = (INTBIG)NONETWORK;
		_netcount = 0;
		if (_baseNetName) _baseNetName->_baseRefCount--;
		return;
	}
	if (_netcount == 0)
	{
		ttyputmsg(M_("Netname::removeNet : netname %s not connected"), _name);
		return;
	}

	INTBIG i;
	NETWORK **oldaddr = (NETWORK**)_netaddr;

	for (i = 0; i < _netcount; i++)
		if (oldaddr[i] == net) break;
	if (i >= _netcount)
	{
		ttyputmsg(M_("Netame::removeNet : netname %s is not connected with this"), _name);
		return;
	}
	if (_netcount == 2)
	{
		_netaddr = (INTBIG)oldaddr[1-i];
		efree((CHAR *)oldaddr);
		_netcount = 1;
		if (_baseNetName) _baseNetName->_baseRefCount--;
		return;
	}
	NETWORK **newaddr = (NETWORK**)emalloc((_netcount-1)*sizeof(NETWORK*), _npd->_cluster);
	for (INTBIG j=0; j < i; j++)
		newaddr[j] = oldaddr[j];
	i++;
	for (; i < _netcount; i++)
		newaddr[i-1] = oldaddr[i];
	_netaddr = (INTBIG)newaddr;
	efree((CHAR *)oldaddr);
	_netcount--;
	if (_baseNetName) _baseNetName->_baseRefCount--;
}

void NetName::docheck()
{
	INTBIG i, j;

	if (_netcount > 1)
	{
		NETWORK **netaddr = (NETWORK**)_netaddr;
		for (i = 0; i < _netcount; i++)
		{
			NETWORK *net = netaddr[i];
			if (net == NONETWORK)
			{
				ttyputmsg(M_("NetName::docheck : netname %s has null connections"), _name);
			}
			for (j = 0; j < i; j++)
			{
				if (netaddr[j] == net)
				{
					ttyputmsg(M_("NetName::docheck : netname %s has connected to duplicate nets"), _name);
				}
			}
			net_checknetname(net, this);
		}
	}
	else if (_netcount == 1)
	{
		NETWORK *net = (NETWORK*)_netaddr;;
		if (net == NONETWORK)
		{
			ttyputmsg(M_("NetName::docheck : netname %s has null connections"), _name);
		}
		net_checknetname(net, this);
	}
	else if (_netaddr != (INTBIG)NONETWORK)
		ttyputmsg(M_("NetName::docheck : netname %s with zero _netcount has bad _netaddr"), _name);
	checkArity();
}

void NetName::checkArity()
{
	if (_baseRefCount != 0 && _netcount != 0)
		ttyputerr(_("Warning (cell %s): bus network '%s' used ambiguously"),
			describenodeproto(_npd->_np), _name);
}

static void net_checknetname(NETWORK *net, NetName *nn)
{
	if (net->namecount == 1)
	{
		if ((NetName*)net->netnameaddr == nn) return;
	}
	else if (net->namecount > 1)
	{
		NetName **netnames = (NetName**)net->netnameaddr;
		INTBIG i;
		for(i = 0; i < net->namecount; i++)
			if (netnames[i] == nn) return;
	}
	ttyputmsg(M_("net_checknetname : netname %s is not found in namelist"), nn->name());
}

void NetName::checkNet(NETWORK *net)
{
	if (_netcount == 1)
	{
		if ((NETWORK*)_netaddr == net) return;
	}
	else if (_netcount > 1)
	{
		NETWORK **nets = (NETWORK**)_netaddr;
		for (INTBIG i = 0; i < _netcount; i++)
			if (nets[i] == net) return;
	} else if ((NETWORK*)_netaddr == NONETWORK) return;
	ttyputmsg(M_("NetName::checkNet : %s net is not found in namelist"), _name);
}

#ifdef NEWRENUM
void NetName::setBusWidth( INTBIG busWidth, CHAR **strings )
{
	_netNameList = (NetName **)emalloc( busWidth * sizeof(NetName *), _npd->_cluster );
	_busWidth = busWidth;
	for (INTBIG i = 0; i < busWidth; i++)
	{
		_netNameList[i] = _npd->addNetName( strings[i] );
	}
}

void NetName::markConn()
{
	if (_busWidth <= 1)
	{
		_conn++;
		return;
	}
	for (INTBIG i = 0; i < _busWidth; i++)
		_netNameList[i]->_conn++;
}
#endif

/*********************** NetCellShorts class ***********************/

void* NetCellShorts::operator new( size_t size )
{
	ttyputerr(M_("operator NetCellShorts(size,cluster) should be used"));
	Q_UNUSED( size );
	return (void*)0;
}

void* NetCellShorts::operator new( size_t size, CLUSTER *cluster )
{
	return (void*)emalloc( size, cluster );
}

void NetCellShorts::operator delete( void* obj )
{
	if(obj) efree((CHAR *)obj);
}

#ifndef MACOS
void NetCellShorts::operator delete( void* obj, CLUSTER *cluster )
{
	Q_UNUSED( cluster );
	if(obj) efree((CHAR *)obj);
}
#endif

NetCellShorts::NetCellShorts( NODEPROTO *np, CLUSTER *cluster )
	: _np( np ), _portbeg( 0 ),
#ifdef NEWRENUM
	  _isolated( 0 ),
#endif
	  _globalnames( 0 ), _portshallowmap( 0 ), _portdeepmap( 0 ), _globalshort(FALSE)
{
	NODEPROTO *cnp;
	PORTPROTO *pp,*cpp;
	NETWORK *net;
	INTBIG index, i, k;

	/* find number of ports */
    index = 0;
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		index++;
    _portbeg = (INTBIG *)emalloc( (index + 1) * sizeof(INTBIG), cluster );
#ifdef NEWRENUM
    _isolated = (BOOLEAN *)emalloc( (index + 1) * sizeof(BOOLEAN), cluster );
#endif
	_portshallowmap = (INTBIG *)emalloc( index * sizeof(INTBIG), cluster );
	_portcount = index;

	/* get contentsview */
	cnp = contentsview(_np);
	if (cnp == NONODEPROTO) cnp = _np;

	/* calculate globalcount() */
    index = 0;
	for (i = 0; i < cnp->globalnetcount; i++)
		if (cnp->globalnetworks[i] != NONETWORK)
			index++;
	_portbeg[0] = index;
	if (globalcount() > 0)
		_globalnames = (CHAR **)emalloc( globalcount() * sizeof(CHAR*), cluster);

	/* fill _portbeg and _portshallowmap */
	clearNetworks(cnp);
    index = 0;
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
    {
		INTBIG width;
		cpp = equivalentport(_np, pp, cnp);
#ifdef NEWRENUM
		_isolated[index] = (cpp != NOPORTPROTO && (cpp->userbits&PORTISOLATED) != 0);
#endif
		if (cpp == NOPORTPROTO || cpp->network == NONETWORK)
		{
			ttyputmsg("Cell %s: equivalent port %s not found in %s", describenodeproto(_np), pp->protoname, describenodeproto(cnp));
			_portshallowmap[index] = index;
			width = pp->network->buswidth;
		} else
		{
			net = cpp->network;
			if (net->temp1 < 0)
				net->temp1 = index;
			_portshallowmap[index] = net->temp1;
			width = net->buswidth;
		}
		_portbeg[index + 1] = _portbeg[index] + width;
		index++;
    }

	/* fill _portdeepmap */
	_portdeepmap = (INTBIG *) emalloc( _portbeg[_portcount] * sizeof(INTBIG), cluster );
	clearNetworks(cnp);
	index = 0;
	for (i = 0; i < cnp->globalnetcount; i++)
	{
		net = cnp->globalnetworks[i];
		if (net == NONETWORK) continue;
		(void)allocstring(&_globalnames[index], cnp->globalnetnames[i], cluster);
		if (net->temp1 < 0)
			net->temp1 = index;
		_portdeepmap[index] = net->temp1;
		index++;
	}
	index = 0;
    for(pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, index++)
    {
		cpp = equivalentport(_np, pp, cnp);
		if (cpp == NOPORTPROTO || cpp->network == NONETWORK)
		{
			for (k = _portbeg[index]; k < _portbeg[index+1]; k++)
				_portdeepmap[k] = k;
			continue;
		}
		net = cpp->network;
		for (k = _portbeg[index]; k < _portbeg[index+1]; k++)
		{
			NETWORK *netk = (net->buswidth <= 1 ? net : net->networklist[k-_portbeg[index]]);
			if (netk->temp1 < 0)
				netk->temp1 = k;
			_portdeepmap[k] = netk->temp1;
		}
    }

	/* calculate globalshort() */
	for (i=0; i < globalcount(); i++)
		if (_portdeepmap[i] != i)
			_globalshort = TRUE;
	for(i=globalcount(); i < _portbeg[_portcount]; i++)
		if (_portdeepmap[i] < globalcount())
			_globalshort = TRUE;
}

NetCellShorts::~NetCellShorts()
{
	INTBIG i;

	if (_globalnames != 0)
	{
		for (i=0; i < globalcount(); i++)
		{
			if (_globalnames[i] != 0) efree(_globalnames[i]);
		}
		efree((CHAR*)_globalnames);
	}
    if (_portbeg != 0) efree((CHAR *)_portbeg);
    if (_portshallowmap != 0) efree((CHAR *)_portshallowmap);
    if (_portdeepmap != 0) efree((CHAR *)_portdeepmap);
}

BOOLEAN NetCellShorts::isConsistent()
{
    NETWORK *net;
	NODEPROTO *cnp;
    PORTPROTO *pp, *cpp;
    INTBIG index, i, k;

	/* check number of ports */
    index = 0;
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
		index++;
	if (portcount() != index) return FALSE;

	/* get contentsview */
	cnp = contentsview(_np);
	if (cnp == NONODEPROTO) cnp = _np;

	/* check globalcount() */
    index = 0;
	for (i = 0; i < cnp->globalnetcount; i++)
		if (cnp->globalnetworks[i] != NONETWORK)
		{
			if (index >= globalcount()) return FALSE;
			if (namesame(_globalnames[index], cnp->globalnetnames[i]) != 0) return FALSE;
			index++;
		}
	if (globalcount() != index) return FALSE;

	/* check portwidth and _portshallowmap */
	clearNetworks(cnp);
    index = 0;
    for(pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, index++)
    {
		cpp = equivalentport(_np, pp, cnp);
		if (cpp == NOPORTPROTO || cpp->network == NONETWORK)
		{
			if (portwidth(index) != 1) return FALSE;
			if (_portshallowmap[index] != index) return FALSE;
			continue;
		}
		net = cpp->network;
		if (portwidth(index) != net->buswidth) return FALSE;
		if (net->temp1 < 0)
			net->temp1 = index;
		if (_portshallowmap[index] != net->temp1) return FALSE;
    }

	/* check _portdeepmap */
	clearNetworks(cnp);
	index = 0;
	for (i = 0; i < cnp->globalnetcount; i++)
	{
		net = cnp->globalnetworks[i];
		if (net == NONETWORK) continue;
		if (net->temp1 < 0)
			net->temp1 = index;
		if (_portdeepmap[index] != net->temp1) return FALSE;
		index++;
	}
    index = 0;
    for(pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, index++)
    {
		cpp = equivalentport(_np, pp, cnp);
		if (cpp == NOPORTPROTO || cpp->network == NONETWORK)
		{
			for (k = _portbeg[index]; k < _portbeg[index+1]; k++)
				if (_portdeepmap[k] != k) return FALSE;
			continue;
		}
		net = cpp->network;
		for (k = _portbeg[index]; k < _portbeg[index+1]; k++)
		{
			NETWORK *netk = (net->buswidth <= 1 ? net : net->networklist[k-_portbeg[index]]);
			if (netk->temp1 < 0)
				netk->temp1 = k;
			if (_portdeepmap[k] != netk->temp1) return FALSE;
		}
    }

    return TRUE;
}

void NetCellShorts::printf()
{
	NODEPROTO *cnp;
    PORTPROTO *pp;
    INTBIG index, i;

	cnp = contentsview(_np);
	if (cnp == NONODEPROTO) cnp = _np;
	efprintf(stdout, "Cell %s (%ld) contents %s globalshort=%d\n", describenodeproto(_np), portcount(),
		describenodeproto(cnp), globalshort());
	for (i = 0; i < globalcount(); i++)
		efprintf(stdout, "\tGlobal %ld %s %ld %s\n", i, _globalnames[i], _portdeepmap[i], _portdeepmap[i] != i ? "!" : "");

	/* check portwidth and _portshallowmap */
	index = 0;
    for(pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto, index++)
    {
#ifdef NEWRENUM
		efprintf(stdout, "\tPort %ld %s (%ld) %ld %s %s\n", index, pp->protoname, portwidth(index), _portshallowmap[index],
			_portshallowmap[index] != index ? "!" : "", isolated(index) ? "isolated" : "");
#else
		efprintf(stdout, "\tPort %ld %s (%ld) %ld %s\n", index, pp->protoname, portwidth(index), _portshallowmap[index], _portshallowmap[index] != index ? "!" : "");
#endif
		for (i = _portbeg[index]; i < _portbeg[index+1]; i++)
			efprintf(stdout, "\t\t%ld %ld %s\n", i, _portdeepmap[i], _portdeepmap[i] != i ? "!" : "");
    }
}

INTBIG NetCellShorts::portwidth( INTBIG portno )
{
    if (portno < 0 || portno >= _portcount) return -1;
    return _portbeg[portno + 1] - _portbeg[portno];
}

void NetCellShorts::clearNetworks(NODEPROTO *np)
{
	if (np->primindex == 0)
	{
		for(NETWORK *net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
			net->temp1 = -1;
	} else
	{
		for(INTBIG i = 0; i < net_mostj; i++)
			net_primnetlist[i]->temp1 = -1;
	}
}

/****************************************************/

#ifdef NEWRENUM
void* NetCellConns::operator new( size_t size )
{
	ttyputerr(M_("operator NetCellConns(size,cluster) should be used"));
	Q_UNUSED( size );
	return (void*)0;
}

void* NetCellConns::operator new( size_t size, CLUSTER *cluster )
{
	return (void*)emalloc( size, cluster );
}

void NetCellConns::operator delete( void* obj )
{
	if(obj) efree((CHAR *)obj);
}

#ifndef MACOS
void NetCellConns::operator delete( void* obj, CLUSTER *cluster )
{
	Q_UNUSED( cluster );
	if(obj) efree((CHAR *)obj);
}
#endif

NetCellConns::NetCellConns( NODEPROTO *np, CLUSTER *cluster )
	: _np( np ), _shallowmap( 0 ), _globalcount(0), _globalnames(0),
	  _shallow2drawn( 0 ), _drawns( 0 ), _deepmapcount( 0 ), _deepmap( 0 ),
	  _complicated( FALSE )
{
	PORTPROTO *pp;
	ARCINST *ai;
	INTBIG index, i, j;
	INTBIG *pinmap;
	VARIABLE *var;
	INTBIG netnum;
	NetName *nn;

	/* find number of ports */
    index = 0;
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		pp->temp1 = index++;
		/* check bussed ports */
		nn = np->netd->findNetName( pp->protoname, TRUE );
		if (nn->busWidth() > 1)
			_complicated = TRUE;
	}
	_portcount = index;

    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* ignore arcs with no signals on them */
		if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC)
		{
			ai->temp1 = -1;
			continue;
		}
		ai->temp1 = index++;

		/* check bus arc */
		VARIABLE *var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE)
		{
			CHAR *arcname = (CHAR *)var->addr;
			if (arcname[0] != 0)
			{
				nn = np->netd->findNetName( arcname, TRUE );
				if (nn->busWidth() > 1)
					_complicated = TRUE;
			}
		}
		/* if this arc end is negated, ignore its node propagation */
		if ((ai->userbits&ISNEGATED) != 0)
			_complicated = TRUE;
	}
	_arccount = index - _portcount;
	_shallowmap = (INTBIG *)emalloc( (_portcount + _arccount) * sizeof(INTBIG), cluster );
	for (i = 0; i < _portcount + _arccount; i++)
		_shallowmap[i] = i;

	/* fill _shallowmap */
	//efprintf(stdout, "Cell %s\n", describenodeproto(np));
	for (NODEINST *ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		//efprintf(stdout, "\tNode %s (%ld)\n", describenodeinst(ni), ni->arraysize);
		if (insamecellgrp(ni->proto, _np)) continue;
		for (pp = ni->proto->firstportproto, j=0; pp != NOPORTPROTO; pp = pp->nextportproto, j++)
			pp->temp1 = j;
		if (ni->arraysize > 1)
		{
			_complicated = TRUE;
			continue;
		}
		NetCellShorts *shorts = ni->proto->netd->netshorts();
		if (shorts->globalcount() > 0)
		{
			for (i = 0; i < shorts->globalcount(); i++)
			{
				addGlobalname(shorts->globalname(i));
			}
			_complicated = TRUE;
		}
		if (ni->proto == sch_wireconprim || ni->proto == sch_buspinprim)
			_complicated = TRUE;
		if (ni->proto == sch_globalprim)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_globalnamekey);
			if (var != NOVARIABLE)
			{
				addGlobalname((CHAR *)var->addr);
			}
			_complicated = TRUE;
		}
		INTBIG fun = (ni->proto->userbits&NFUNCTION)>>NFUNCTIONSH;
		if (fun == NPCONPOWER)
		{
			addGlobalname(_("Power"));
			_complicated = TRUE;
		} else if (fun == NPCONGROUND)
		{
			addGlobalname(_("Ground"));
			_complicated = TRUE;
		}

		pinmap = (INTBIG*)emalloc( shorts->portcount()*sizeof(INTBIG), cluster );
		for (i = 0; i < shorts->portcount(); i++)
			pinmap[i] = -1;

		for(PORTEXPINST *pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (shorts->isolated(pe->proto->temp1)) continue;
			netnum = shorts->portshallowmap(pe->proto->temp1);
			//efprintf(stdout, M_("\t\tPortExpInst %s %ld %ld\n"), pe->proto->protoname, netnum, pe->exportproto->temp1);
			if (pinmap[netnum] < 0)
			{
				pinmap[netnum] = pe->exportproto->temp1;
			} else
			{
				shallowconnect( pinmap[netnum], pe->exportproto->temp1);
			}
		}
		for (PORTARCINST *pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* if port is isolated, ignore it */
			if (shorts->isolated(pi->proto->temp1)) continue;

			/* do not follow nonbus wires onto a bus pin */
			if (ni->proto == sch_buspinprim && pi->conarcinst->proto != sch_busarc) continue;

			netnum = shorts->portshallowmap(pi->proto->temp1);
			if (pi->conarcinst->temp1 < 0) continue;
			//efprintf(stdout, M_("\t\tPortArcInst %s %ld %ld\n"), pi->proto->protoname, netnum, pi->conarcinst->temp1);
			if (pinmap[netnum] < 0)
			{
				pinmap[netnum] = pi->conarcinst->temp1;
			} else
			{
				shallowconnect( pinmap[netnum], pi->conarcinst->temp1);
			}
		}
		efree((CHAR *)pinmap);
	}
	shallowfixup();
	makeDrawns();
}

NetCellConns::~NetCellConns()
{
	INTBIG i;

    if (_shallowmap != 0) efree((CHAR *)_shallowmap);
	for (i = 0; i < _globalcount; i++)
		efree(_globalnames[i]);
	if (_globalnames != 0) efree((CHAR *)_globalnames);
	if (_shallow2drawn != 0 ) efree((CHAR *)_shallow2drawn);
	if (_drawns != 0 ) efree((CHAR *)_drawns); 
	if (_deepmap != 0 ) efree((CHAR *)_deepmap); 
}

void NetCellConns::compare()
{
	PORTPROTO *pp;
	ARCINST *ai;
	NETWORK *net, **netmap;
	INTBIG *portcount, *refcount, *arccount;
	INTBIG index, i, netnum;

	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = -1;
	netmap = (NETWORK **)emalloc((_portcount + _arccount) * sizeof(NETWORK *), _np->netd->cluster());
	portcount = (INTBIG *)emalloc((_portcount + _arccount) * sizeof(INTBIG), _np->netd->cluster());
	refcount = (INTBIG *)emalloc((_portcount + _arccount) * sizeof(INTBIG), _np->netd->cluster());
	arccount = (INTBIG *)emalloc((_portcount + _arccount) * sizeof(INTBIG), _np->netd->cluster());
	for (i = 0; i < _portcount + _arccount; i++)
	{
		netmap[i] = NONETWORK;
		portcount[i] = 0;
		refcount[i] = 0;
		arccount[i] = 0;
	}

	/* check number of ports */
    index = 0;
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		net = pp->network;
		if (net == NONETWORK)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare port %s of cell %s has no network"),
				pp->protoname, describenodeproto(_np));
			return;
		}
		if (index >= _portcount + _arccount)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare cell %s has too many ports portcount=%ld"),
				describenodeproto(_np), _portcount);
			return;
		}
		pp->temp1 = index++;
		netmap[pp->temp1] = net;
	}
	if (index != _portcount)
	{
		ttyputmsg(M_("WARNING:: NetCellConns::compare cell %s portcount != index portcount=%ld index=%ld"),
			describenodeproto(_np), _portcount, index);
		return;
	}

    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		net = ai->network;
		/* ignore arcs with no signals on them */
		if (((ai->proto->userbits&AFUNCTION)>>AFUNCTIONSH) == APNONELEC)
		{
			ai->temp1 = -1;
			if (net != NONETWORK)
			{
				ttyputmsg(M_("WARNING:: NetCellConns::compare nonelectrical arc %s of cell %s has network %s"),
					describearcinst(ai), describenodeproto(_np), describenetwork(net));
				return;
				
			}
			continue;
		}
		if (net == NONETWORK)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare arcinst %s of cell %s has no network"),
				describearcinst(ai), describenodeproto(_np));
			return;
		}
		if (index >= _portcount + _arccount)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare cell %s has too many arcs portcount=%ld arccount=%ld"),
				describenodeproto(_np), _portcount, _arccount);
			return;
		}
		ai->temp1 = index++;
		netmap[ai->temp1] = net;
	}
	if (_arccount != index - _portcount)
	{
		ttyputmsg(M_("WARNING:: NetCellConns::compare cell %s arccount != index arccount=%ld index=%ld"),
			describenodeproto(_np), _arccount, index - _portcount);
		return;
	}

	for (i = 0; i < _portcount + _arccount; i++)
	{
		netnum = _shallowmap[i];
		net = netmap[i];
		if (netnum == i)
		{
			if (net->temp1 != -1)
				ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s already assigned"),
					describenetwork(net), describenodeproto(_np));
			net->temp1 = i;
		} else {
			if (net->temp1 != netnum)
				ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s net->temp1=%ld netnum=%ld i=%ld"),
					describenetwork(net), describenodeproto(_np), net->temp1, netnum, i);
		}
		if (i < _portcount)
			portcount[netnum]++;
		else
			refcount[netnum]++;
	}

    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->temp1 < 0) continue;
		net = ai->network;
		/* check bus arc */
		VARIABLE *var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE)
		{
			CHAR *arcname = (CHAR *)var->addr;
			if (arcname[0] != 0)
			{
				//::printf("Cell %s arc %s on net %s\n", describenodeproto(_np), arcname, describenetwork(net));
				arccount[net->temp1]++;
			}
		}
	}

	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->temp1 == -1)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has net->temp1=-1"),
				describenetwork(net), describenodeproto(_np));
		}
		if (net->portcount != portcount[net->temp1])
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid portcount"),
				describenetwork(net), describenodeproto(_np));
		}
		if (net->refcount != refcount[net->temp1])
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid refcount"),
				describenetwork(net), describenodeproto(_np));
		}
#if 0
		if (net->arccount != arccount[net->temp1])
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid arccount net->arccount=%ld arccount=%ld"),
				describenetwork(net), describenodeproto(_np), net->arccount, arccount[net->temp1]);
		}
#endif
		if (net->buslinkcount != 0)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid buslinkcountcount"),
				describenetwork(net), describenodeproto(_np));
		}
		if (net->globalnet != -1)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid globalnet"),
				describenetwork(net), describenodeproto(_np));
		}
		if (net->buswidth != 1)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid buswidth"),
				describenetwork(net), describenodeproto(_np));
		}
		if (net->numvar != 0)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::compare net %s of cell %s has invalid numvar"),
				describenetwork(net), describenodeproto(_np));
		}
	}
	
	efree((CHAR*) netmap);
	efree((CHAR*) portcount);
	efree((CHAR*) refcount);
	efree((CHAR*) arccount);
}

void NetCellConns::deepcompare()
{
	NETWORK *net, **netmap;
	NetDrawn *nd;
	INTBIG i, k, m;

	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = -1;
	netmap = (NETWORK **)emalloc(_deepmapcount * sizeof(NETWORK *), _np->netd->cluster());
	for (i = 0; i < _deepmapcount; i++)
	{
		netmap[i] = NONETWORK;
	}
	for (i = 0; i < _globalcount; i++)
	{
		m = _deepmap[i];
		k = net_findglobalnet(_np, _globalnames[i]);
#if 0
		ttyputmsg(_("NetCellConns::deepcompare global %ld(%ld) %s %ld)"), i, m, _globalnames[i], k);
#endif
		net = k >= 0 ? _np->globalnetworks[k] : NONETWORK;
		if (i == m)
		{
			netmap[i] = net;
			if (net == NONETWORK) continue;
			if (net->temp1 != -1)
			{
				ttyputmsg(M_("NetCellConns::deepcompare in %s: global net %s not fully connected"),
					describenodeproto(_np), describenetwork(net));
			}
			net->temp1 = m;
		} else if (netmap[m] != net)
		{
			ttyputmsg(M_("NetCellConns::deepcompare in %s: not connected %s and %s"),
				describenodeproto(_np), describenetwork(netmap[m]), describenetwork(net));
		}
	}
	for (i = 0; i < _drawncount; i++)
	{
		nd = &_drawns[i];
		if (nd->_busWidth != nd->_network->buswidth)
		{
			ttyputmsg(M_("NetCellConns::deepcompare in %s: buswidth mismatch in %s"),
				describenodeproto(_np), describenetwork(nd->_network));
		}
		for (k = 0; k < nd->_busWidth; k++)
		{
			m = _deepmap[nd->_deepbase + k];
			net = nd->_network;
			net = net->buswidth > 1 ? net->networklist[k] : net;
			if (m == nd->_deepbase + k)
			{
				netmap[m] = net;
				if (net->temp1 != -1)
				{
					ttyputmsg(M_("NetCellConns::deepcompare in %s: net %s not fully connected"),
						describenodeproto(_np), describenetwork(net));
				}
				net->temp1 = m;
			} else if (netmap[m] != net)
			{
				ttyputmsg(M_("NetCellConns::depcompare in %s: not connected %s and %s"),
					describenodeproto(_np), describenetwork(netmap[m]), describenetwork(net));
			}
		}
	}
	efree((CHAR*) netmap);
	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->buswidth == 1 && net->temp1 == -1)
		{
			ttyputmsg(M_("WARNING:: NetCellConns::deepcompare net %s of cell %s has net->temp1=-1"),
				describenetwork(net), describenodeproto(_np));
		}
	}
}

void NetCellConns::schem()
{
	PORTPROTO *pp;
	ARCINST *ai;
	NODEINST *ni;
	PORTEXPINST *pe;
	PORTARCINST *pi;
	VARIABLE *var;
	NetName *nn, *subn;
	NetDrawn *nd;
	INTBIG i, j, m;
	NETWORK *net;

	_np->netd->clearConns();

	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = -1;
	/* find number of ports */
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* check bussed ports */
		nn = _np->netd->findNetName( pp->protoname, TRUE );
		nn->markConn();
		/* give a number */
		m = _shallowmap[pp->temp1];
		nd = _shallow2drawn[m];
		if (nn->busWidth() > 1) nd->_isBus = TRUE;
		if (nd->_busWidth != 0 && nd->_busWidth != nn->busWidth())
		{
			ttyputmsg(M_("NetCellConns::schem in %s: net %s has different bus width %ld and %ld"),
				describenodeproto(_np), describenetwork(nd->_network), nd->_busWidth, nn->busWidth());
		}
		nd->_busWidth = nn->busWidth();
		if (m == pp->temp1)
		{
			nd->_network = pp->network;
#if 0
			net = pp->network;
			if (net->buswidth > 1 && net->temp1 != -1)
			{
				ttyputmsg(M_("NetCellConns::schem in %s: net %s not fully connected"),
					describenodeproto(_np), describenetwork(net));
			}
			net->temp1 = m;
#endif
		} else if (nd->_network != pp->network)
		{
			ttyputmsg(M_("NetCellConns::schem strange port %s of cell %s"),
				pp->protoname, describenodeproto(_np));
		}
	}

    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* ignore arcs with no signals on them */
		if (ai->temp1 < 0) continue;

		/* give a number */
		m = _shallowmap[ai->temp1];
		nd = _shallow2drawn[m];

		/* check bus arc */
		if (ai->proto == sch_busarc)
			nd->_isBus = TRUE;
		else if (nd->_busWidth != 1)
		{
			if (nd->_busWidth != 0)
			{
				ttyputmsg(M_("NetCellConns::schem in %s: bus net %s has nonbus arc"),
					describenodeproto(_np), describenetwork(nd->_network), nd->_busWidth);
			} else
			{
				nd->_busWidth = 1;
			}
		}
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE && (var->type&VDISPLAY) != 0)
		{
			CHAR *arcname = (CHAR *)var->addr;
			if (arcname[0] != 0)
			{
				nn = _np->netd->findNetName( arcname, TRUE );
				nn->markConn();
				if (nd->_busWidth != 0 && nd->_busWidth != nn->busWidth())
				{
					ttyputmsg(M_("NetCellConns::schem in %s: net %s has different bus width %ld and %ld"),
						describenodeproto(_np), describenetwork(nd->_network), nd->_busWidth, nn->busWidth());
				}
				nd->_busWidth = nn->busWidth();
			}
		}

		if (m == ai->temp1)
		{
			nd->_network = ai->network;
#if 0
			net = ai->network;
			if (net->buswidth > 1 && net->temp1 != -1)
			{
				ttyputmsg(M_("NetCellConns::schem in %s: net %s not fully connected"),
					describenodeproto(_np), describenetwork(net));
			}
			net->temp1 = m;
#endif
		} else if (nd->_network != ai->network)
		{
			ttyputmsg(M_("NetCellConns::sschem strange arc %s of cell %s"),
				describearcinst(ai), describenodeproto(_np));
		}
	}

	/* find bus width of unnamed busses */
	for (ni = _np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		//efprintf(stdout, "\tNode %s (%ld)\n", describenodeinst(ni), ni->arraysize);
		if (insamecellgrp(ni->proto, _np)) continue;

		if (ni->proto->primindex != 0) continue;

		for (pp = ni->proto->firstportproto, j=0; pp != NOPORTPROTO; pp = pp->nextportproto, j++)
			pp->temp1 = j;
		NetCellShorts *shorts = ni->proto->netd->netshorts();
		INTBIG nodewidth = ni->arraysize;
		if (nodewidth < 1) nodewidth = 1;

		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			nd = _shallow2drawn[pe->exportproto->temp1];
			if (nd->_busWidth == 0)
			{
				nd->_busWidth = shorts->portwidth( pe->proto->temp1 ) * nodewidth;
			}
		}
		for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->conarcinst->temp1 < 0) continue;
			nd = _shallow2drawn[pi->conarcinst->temp1];
			if (nd->_busWidth == 0)
			{
				nd->_busWidth = shorts->portwidth( pi->proto->temp1 ) * nodewidth;
			}
		}
	}

	_deepmapcount = _globalcount;
	for (i = 0; i < _drawncount; i++)
	{
		nd = &_drawns[i];
		if (_np->cellview == el_iconview && nd->_busWidth == 0)
		{
			/* sometimes there are unnamed unconnected busses in icons */
			nd->_busWidth = 1;
		}
		net = nd->_network;
		if (nd->_busWidth != net->buswidth)
		{
			ttyputmsg(M_("NetCellConns::schem in %s: net %s has different bus width from newrenum %ld and from NETWORK %ld"),
				describenodeproto(_np), describenetwork(net), nd->_busWidth, nd->_network->buswidth);
		}
		nd->_busWidth = net->buswidth;
		nd->_deepbase = _deepmapcount;
		_deepmapcount += nd->_busWidth;
	}

	_np->netd->calcConns(&_deepmapcount);
	_deepmap = (INTBIG *)emalloc(_deepmapcount * sizeof(INTBIG), _np->netd->cluster());
	for (i = 0; i < _deepmapcount; i++)
		_deepmap[i] = i;

	/* find number of ports */
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* check bussed ports */
		nn = _np->netd->findNetName( pp->protoname, TRUE );
		nd = _shallow2drawn[pp->temp1];
		for (i = 0; i < nd->_busWidth; i++)
		{
			subn = nn->busWidth() > 1 ? nn->subName(i) : nn;
			deepconnect(nd->_deepbase + i, subn->conn());
		}
	}
    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* ignore arcs with no signals on them */
		if (ai->temp1 < 0) continue;

		/* check bus arc */
		VARIABLE *var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var != NOVARIABLE && (var->type&VDISPLAY) != 0)
		{
			CHAR *arcname = (CHAR *)var->addr;
			if (arcname[0] != 0)
			{
				nn = _np->netd->findNetName( arcname, TRUE );
				nd = _shallow2drawn[ai->temp1];
				for (i = 0; i < nd->_busWidth; i++)
				{
					subn = nn->busWidth() > 1 ? nn->subName(i) : nn;
					deepconnect(nd->_deepbase + i, subn->conn());
				}
			}
		}
	}

	/* fill _shallowmap */
	//efprintf(stdout, "Cell %s\n", describenodeproto(_np));
	for (ni = _np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		//efprintf(stdout, "\tNode %s (%ld)\n", describenodeinst(ni), ni->arraysize);
		if (insamecellgrp(ni->proto, _np)) continue;
		for (pp = ni->proto->firstportproto, j=0; pp != NOPORTPROTO; pp = pp->nextportproto, j++)
			pp->temp1 = j;
		NetCellShorts *shorts = ni->proto->netd->netshorts();
#if 0
		if (ni->proto == sch_wireconprim || ni->proto == sch_buspinprim)
			continue; // ????????????????????????
#endif
		INTBIG nodewidth = ni->arraysize;
		if (nodewidth < 1) nodewidth = 1;
		INTBIG pinmapcount = shorts->globalcount() + shorts->portdeepcount()*nodewidth;
		INTBIG *pinmap = (INTBIG*)emalloc( pinmapcount*sizeof(INTBIG), _np->netd->cluster() );
		for (i = 0; i < pinmapcount; i++)
			pinmap[i] = -1;

		INTBIG fun = (ni->proto->userbits&NFUNCTION)>>NFUNCTIONSH;
		if (fun == NPCONPOWER || fun == NPCONGROUND || ni->proto == sch_globalprim)
		{
			INTBIG glob = -1;
			if (ni->proto == sch_globalprim)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, sch_globalnamekey);
				if (var != NOVARIABLE)
				{
					glob = addGlobalname((CHAR *)var->addr);
				}
			} else if (fun == NPCONGROUND)
				glob = addGlobalname(_("Ground"));
			else if (fun == NPCONPOWER)
				glob = addGlobalname(_("Power"));
			if (glob == -1)
			{
				ttyputmsg(M_("NetCellShorts::schem in %s global instance %s not found"),
					describenodeproto(_np), describenodeinst(ni));
				if (ni->proto == sch_globalprim)
				{
					ttyputmsg(M_("NetCellShorts::schem in %s global instance %s not found name %s"),
						describenodeproto(_np), describenodeinst(ni), (CHAR*)var->addr);
				}
			}
			for (i = 0; i < pinmapcount; i++)
				pinmap[i] = glob;
		} else if (ni->proto == sch_wireconprim)
		{
			deepjoin(ni);
			continue;
		} else if (ni->proto->primindex != 0)
		{
			continue;
		}
		for (i = 0; i < shorts->globalcount(); i++)
		{
			INTBIG globalnet = addGlobalname(shorts->globalname(i));
			if (globalnet < 0)
			{
				ttyputmsg(M_("NetCellShorts::schem in %s global net %s not found"),
					describenodeproto(_np), shorts->globalname(i));
				continue;
			}
			pinmap[i] = globalnet;
			m = shorts->portdeepmap(i);
			if (i != m)
			{
				ttyputmsg(M_("NetCellShorts::schem in %s global nets %s and %s are connected"),
					describenodeproto(_np), shorts->globalname(m), shorts->globalname(i));
				deepconnect(pinmap[i], pinmap[m]);
			}
		}

		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			deepconnectport(pe->exportproto->temp1, ni, shorts, pinmap, pe->proto);
		}
		for (pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* do not follow nonbus wires onto a bus pin */
			if (ni->proto == sch_buspinprim && pi->conarcinst->proto != sch_busarc) continue;

			deepconnectport(pi->conarcinst->temp1, ni, shorts, pinmap, pi->proto);
		}
		efree((CHAR *)pinmap);
	}

	deepfixup();

	//::printf("ShowConns %s\n", describenodeproto(_np));
	deepcompare();
#if 0
	for (i = 0; i < _drawncount; i++)
	{
		nd = &_drawns[i];
		::printf("%ld\tDrawn %s %ld\n", nd->_deepbase, describenetwork(nd->_network), nd->_busWidth);
		for (INTBIG k = 0; k < nd->_busWidth; k++)
			::printf("\t%ld %ld\n", nd->_deepbase + k, _deepmap[nd->_deepbase + k]);
	}
	
	_np->netd->showConns(_deepmap);
#endif
}

void NetCellConns::deepconnectport( INTBIG drawnIndex, NODEINST *ni, NetCellShorts *shorts, INTBIG *pinmap, PORTPROTO *pp)
{
	INTBIG k, l;

	INTBIG nodewidth = ni->arraysize;
	if (nodewidth < 1) nodewidth = 1;

	/* if port is isolated, ignore it */
	if (shorts->isolated(pp->temp1)) return;
	INTBIG portbeg = shorts->portbeg(pp->temp1);
	INTBIG portwidth = shorts->portwidth(pp->temp1);

	if (drawnIndex < 0) return;
	NetDrawn *nd = _shallow2drawn[drawnIndex];
	BOOLEAN deepstep = portwidth != nd->_busWidth;
	if (deepstep && nd->_busWidth != portwidth*nodewidth)
	{
		if (nodewidth > 1)
		{
			ttyputmsg(_("Warning (cell %s): bus %s cannot connect to port %s of %ld-wide node %s (different width)"),
				describenodeproto(_np), describenetwork(nd->_network), pp->protoname,
				nodewidth, describenodeinst(ni));
		} else
		{
			ttyputmsg(_("Warning (cell %s): bus %s cannot connect to port %s of node %s (different width)"),
				describenodeproto(_np), describenetwork(nd->_network), pp->protoname,
				describenodeinst(ni));
		}
		return;
	}
	for (k = 0; k < portwidth; k++)
	{
		INTBIG netnum = shorts->portdeepmap(portbeg + k);
		INTBIG deepnum = nd->_deepbase + k;
		for (l = 0; l < nodewidth; l++) {
			if (pinmap[netnum] < 0)
			{
				pinmap[netnum] = deepnum;
			} else
			{
				deepconnect( pinmap[netnum], deepnum);
			}
			if (netnum >= shorts->globalcount())
				netnum += shorts->portdeepcount();
			if (deepstep)
				deepnum += portwidth;
		} 
	}
}

/*
 * Routine to implement the "wire_con" primitive which joins two arcs and attaches
 * busses of unequal length (by repeating signals on the shorter bus until fully
 * attached to the larger bus).
 */
void NetCellConns::deepjoin(NODEINST *ni)
{
	REGISTER ARCINST *ai;
	NetDrawn *smallnd, *largend;
	REGISTER INTBIG smallnum, largenum, num, wirecount, i;
	REGISTER PORTARCINST *pi;

	/* find the narrow and wide busses on this connector */
	smallnd = largend = 0;
	smallnum = largenum = 0;
	wirecount = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		wirecount++;
		if (ai->temp1 < 0) continue;
		NetDrawn *nd = _shallow2drawn[ai->temp1];
		num = nd->_busWidth;
		if (smallnd == 0 || num <= smallnum)
		{
			smallnd = nd;
			smallnum = num;
		}
		if (largend == 0 || num > largenum)
		{
			largend = nd;
			largenum = num;
		}
	}
	if (wirecount < 2) return;
	if (wirecount > 2)
	{
		ttyputmsg(_("Cell %s, connector %s can only merge two arcs (has %ld)"),
			describenodeproto(ni->parent), describenodeinst(ni), wirecount);
		return;
	}

	for(i=0; i<largenum; i++)
	{
		deepconnect( smallnd->_deepbase + i % smallnd->_busWidth, largend->_deepbase + i );
	}
#if 0
	/* also merge the busses if they are the same length */
	if (smallnum == largenum && smallnum > 1)
	{
		(void)net_mergenet(smallai->network, largeai->network);
	}
#endif
}

void NetCellConns::makeNetworksSimple()
{
	PORTPROTO *pp;
	ARCINST *ai;
	NETWORK *net, **netmap;
	INTBIG i, netnum;
	CHAR *arcname;
	NetName *nn;

	/* first delete all network objects in this cell */
	while (_np->firstnetwork != NONETWORK)
	{
		net = _np->firstnetwork;
		_np->firstnetwork = net->nextnetwork;
		net_freenetwork(net, _np);
	}
	for(i=0; i<_np->globalnetcount; i++)
		_np->globalnetworks[i] = NONETWORK;

	/* create network objects for every equivalence class */
	netmap = (NETWORK **)emalloc((_portcount + _arccount) * sizeof(NETWORK *), _np->netd->cluster());
	for (i = 0; i < _portcount + _arccount; i++)
	{
		if (_shallowmap[i] == i)
			netmap[i] = net_newnetwork(_np);
		else
			netmap[i] = NONETWORK;
	}

	/* mark ports */
    for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		netnum = _shallowmap[pp->temp1];
		net = netmap[netnum];
		net_putportonnet(pp, net);
		nn = _np->netd->findNetName(pp->protoname, TRUE);
		net_namenet(nn, net);
		
	}
	/* mark arcs */
    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		/* ignore arcs with no signals on them */
		if (ai->temp1 < 0)
		{
			ai->network = NONETWORK;
			continue;
		}
		netnum = _shallowmap[ai->temp1];
		net = netmap[netnum];
		ai->network = net;
		net->refcount++;
		/* check arc name */
		VARIABLE *var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) continue;
		/* ignore tempnames */
		if ((var->type&VDISPLAY) == 0) continue;
		arcname = (CHAR *)var->addr;
		if (arcname[0] == 0) continue;
		nn = _np->netd->findNetName( arcname, TRUE );
		net_namenet(nn, net);
		net_putarclinkonnet(net, ai);
	}
	/* temporary arc names */
    for (ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		net = ai->network;
		if (net == NONETWORK || net->namecount > 0) continue;
		/* check arc name */
		VARIABLE *var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) continue;
		arcname = (CHAR *)var->addr;
		if (arcname[0] == 0) continue;
		nn = _np->netd->findNetName( arcname, TRUE );
		net_namenet(nn, net);
		net->tempname = TRUE;
		if (net->arccount <= 0)
			net_putarclinkonnet(net, ai);
	}
	efree((CHAR*) netmap);
}

void NetCellConns::printf()
{
	NETWORK *net, **netmap;
	INTBIG i, j;
	PORTPROTO *pp;
	CLUSTER *cluster = _np->netd->cluster();
	BOOLEAN err = _complicated;

	efprintf(stdout, "Cell %s complicated=%d\n", describenodeproto(_np), complicated());
	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
		net->temp1 = -1;
	netmap = (NETWORK **)emalloc((_portcount + _arccount) * sizeof(NETWORK *), cluster);
	for (i = 0; i < _portcount + _arccount; i++) netmap[i] = NONETWORK;
	for (pp = _np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		net = pp->network;
		if (_shallowmap[pp->temp1] == pp->temp1)
			netmap[pp->temp1] = net;
		else if (netmap[_shallowmap[pp->temp1]] != net)
		{
			efprintf(stdout, M_("\t!!!!!!!!!!!!!!!!!!!!!! %s %s are not connected in map\n"),
				describenetwork(netmap[_shallowmap[pp->temp1]]),
				describenetwork(net));
			ttyputmsg(M_("\t!!!!!!!!!!!!!!!!!!!!!! %s %s are not connected in map"),
				describenetwork(netmap[_shallowmap[pp->temp1]]),
				describenetwork(net));
			err = TRUE;
		}
		if (net->temp1 < 0)
			net->temp1 = _shallowmap[pp->temp1];
		else if (net->temp1 != _shallowmap[pp->temp1])
		{
			err = TRUE;
		}
#if 0
		efprintf(stdout, "\t%ld\tPort %ld%s %s net %s %ld (%d)\n", pp->temp1, _shallowmap[pp->temp1],
			pp->temp1 != _shallowmap[pp->temp1] ? "!" : "",
			pp->protoname, describenetwork(pp->network), net->temp1, pp->network->buswidth);
#endif
	}
	for (ARCINST *ai = _np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		net = ai->network;
		if (net == NONETWORK || ai->temp1 < 0)
		{
			if (net != NONETWORK)
			{
				err = TRUE;
				efprintf(stdout, "\tNonelectric arc %s has network %s\n", describearcinst(ai), describenetwork(net));
			}
			if (ai->temp1 != -1)
			{
				err = TRUE;
				efprintf(stdout, "!!!!!!!!! -1 !!!!!!!!!\n");
			}
			continue;
		}
		if (_shallowmap[ai->temp1] == ai->temp1)
			netmap[ai->temp1] = net;
		else if (netmap[_shallowmap[ai->temp1]] != net)
		{
			efprintf(stdout, M_("\t!!!!!!!!!!!!!!!!!!!!!! %s %s are not connected in map\n"),
				describenetwork(netmap[_shallowmap[ai->temp1]]),
				describenetwork(net));
			ttyputmsg(M_("\t!!!!!!!!!!!!!!!!!!!!!! %s %s are not connected in map"),
				describenetwork(netmap[_shallowmap[ai->temp1]]),
				describenetwork(net));
			err = TRUE;
		}
		if (net->temp1 < 0)
			net->temp1 = _shallowmap[ai->temp1];
		else if (net->temp1 != _shallowmap[ai->temp1])
			err = TRUE;
#if 0
		efprintf(stdout, "\t%ld\tArc %ld%s %s net %s %ld (%d)\n", ai->temp1, _shallowmap[ai->temp1],
			ai->temp1 != _shallowmap[ai->temp1] ? "!" : "",
			describearcinst(ai), describenetwork(ai->network), net->temp1, ai->network->buswidth);
#endif
	}
	for (i = 0, j = 0; i < _portcount + _arccount; i++)
		if (_shallowmap[i] == i) j++;
	for(net = _np->firstnetwork, i = 0; net != NONETWORK; net = net->nextnetwork)
	{
		if (net->temp1 < 0)
		{
			efprintf(stdout, "\tUnknown network %s portcount=%d refcount=%d buslinkcount=%d arccount=%d globalnet=%d\n",
				describenetwork(net),
				net->portcount, net->refcount, net->buslinkcount, net->arccount, net->globalnet);
			err = TRUE;
		}
		i++;
	}
	fprintf(stdout, "Networks count %ld shallow %ld\n\n", i, j);
	if (i != j) err = TRUE;
	if (_np->cellview == el_layoutview && err)
	{
		fprintf(stdout, "!!!!!!!!!!!!! Strange cell %s\n", describenodeproto(_np));
		ttyputmsg(M_("!!!!!!!!!!!!! Strange cell %s"), describenodeproto(_np));
	}
	efree((CHAR*)netmap);
}

void NetCellConns::shallowconnect( INTBIG a1, INTBIG a2 )
{
	INTBIG m1, m2, m;

	for (m1 = a1; _shallowmap[m1] != m1; m1 = _shallowmap[m1]);
	for (m2 = a2; _shallowmap[m2] != m2; m2 = _shallowmap[m2]);
	m = m1 < m2 ? m1 : m2;
	//efprintf(stdout, M_("\t\tShallowConnect %ld %ld -> %ld\n"), a1, a2, m);
	for (;;)
	{
		m1 = _shallowmap[a1];
		_shallowmap[a1] = m;
		if (a1 == m1) break;
		a1 = m1;
	}
	for (;;)
	{
		m2 = _shallowmap[a2];
		_shallowmap[a2] = m;
		if (a2 == m2) break;
		a2 = m2;
	}
}

void NetCellConns::shallowfixup()
{
	INTBIG i;

	for (i = 0; i < _portcount + _arccount; i++)
	{
		INTBIG m = _shallowmap[i];
		if (m == i)
			_drawncount++;
		else
			_shallowmap[i] = _shallowmap[m];
	}
}

INTBIG NetCellConns::addGlobalname(CHAR *name)
{
	CLUSTER *cluster = _np->netd->cluster();
	INTBIG i;
	CHAR **newglobalnames;

	if (_globalcount == 0)
	{
		_globalcount = 1;
		_globalnames = (CHAR**)emalloc(sizeof(CHAR*)*1, cluster);
		allocstring(&_globalnames[0], name, cluster);
		return 0;
	}
	for (i = 0; i < _globalcount; i++)
	{
		if (namesame(name, _globalnames[i]) == 0) return i;
	}
	newglobalnames = (CHAR**)emalloc(sizeof(CHAR*)*(_globalcount + 1), cluster);
	for (i = 0; i < _globalcount; i++)
	{
		newglobalnames[i] = _globalnames[i];
	}
	allocstring(&newglobalnames[_globalcount], name, cluster);
	efree((CHAR *)_globalnames);
	_globalnames = newglobalnames;
	return _globalcount++;
}

void NetCellConns::makeDrawns()
{
	INTBIG i;
	CLUSTER *cluster = _np->netd->cluster();

	_drawncount = 0;
	for (i = 0; i < _portcount + _arccount; i++)
	{
		if (_shallowmap[i] == i)
			_drawncount++;
	}
	_shallow2drawn = (NetDrawn **)emalloc((_portcount + _arccount)*sizeof(NetDrawn *), cluster);
	_drawns = (NetDrawn *)emalloc(_drawncount*sizeof(NetDrawn), cluster);
	NetDrawn *p = _drawns;
	for (i = 0; i < _portcount + _arccount; i++)
	{
		INTBIG m = _shallowmap[i];
		if (m == i)
		{
			_shallow2drawn[i] = p;
			p->_shallowRoot = i;
			p->_isBus = FALSE;
			p->_busWidth = 0;
			p->_network = NONETWORK;
			p++;
		} else
		{
			_shallow2drawn[i] = _shallow2drawn[m];
		}
	}
}

void NetCellConns::deepconnect( INTBIG a1, INTBIG a2 )
{
	INTBIG m1, m2, m;

	for (m1 = a1; _deepmap[m1] != m1; m1 = _deepmap[m1]);
	for (m2 = a2; _deepmap[m2] != m2; m2 = _deepmap[m2]);
	m = m1 < m2 ? m1 : m2;
	//efprintf(stdout, M_("\t\tShallowConnect %ld %ld -> %ld\n"), a1, a2, m);
	for (;;)
	{
		m1 = _deepmap[a1];
		_deepmap[a1] = m;
		if (a1 == m1) break;
		a1 = m1;
	}
	for (;;)
	{
		m2 = _deepmap[a2];
		_deepmap[a2] = m;
		if (a2 == m2) break;
		a2 = m2;
	}
}

void NetCellConns::deepfixup()
{
	for (INTBIG i = 0; i < _deepmapcount; i++)
	{
		INTBIG m = _deepmap[i];
		_deepmap[i] = _deepmap[m];
	}
}
#endif

/*********************** BUS NAME ROUTINES ***********************/

#define MULTIDIMENSIONALBUS 1

/*
 * routine to parse the bus name "name" found on an arc that has function
 * "funct" in cell "cell".  Returns the number of signals described by the
 * name (returns 1 dummy signal on error).  The pointer at "strings" is filled with an
 * array of individual bus names.
 */
INTBIG net_evalbusname(INTBIG funct, CHAR *name, CHAR ***strings,
	ARCINST *thisai, NODEPROTO *cell, INTBIG showerrors)
{
	REGISTER CHAR *key, *cindex, *endindex, *errorstring, *busname;
	CHAR *ptin, *savekey, ***mystrings;
	INTBIG count, *stringcount;
	REGISTER INTBIG ch1, ch2, perfect, dimension, dimstart, dimend, dimlen;
	REGISTER INTBIG indexval, endindexval, origfunct, i, index, range;
	REGISTER void *infstr;

	/* initialize */
	if (net_busbufstringbufferpos < 0)
	{
		net_busbufstringbufferpos = 0;
		for(i=0; i<NUMBUSSTRINGBUFFERS; i++) net_busbufstringcountarray[i] = 0;
	}
	mystrings = &net_busbufstringsarray[net_busbufstringbufferpos];
	stringcount = &net_busbufstringcountarray[net_busbufstringbufferpos];
	net_busbufstringbufferpos++;
	if (net_busbufstringbufferpos >= NUMBUSSTRINGBUFFERS)
		net_busbufstringbufferpos = 0;

	count = 0;
	ptin = name;
	perfect = 0;
	savekey = 0;
	for(;;)
	{
		key = getkeyword(&ptin, x_("[],"));
		if (key == NOSTRING) break;
		ch1 = tonextchar(&ptin);
		if (ch1 == ']')
		{
			if (showerrors != 0)
				ttyputmsg(_("Cell %s, network '%s': unmatched ']' in name"),
					describenodeproto(cell), name);
			break;
		}
		if (ch1 == ',' || ch1 == 0)
		{
			/* add unindexed network name "key" to list */
			if (*key == 0)
			{
				if (showerrors != 0)
					ttyputmsg(_("Cell %s, network '%s': empty network name"),
						describenodeproto(cell), name);
				break;
			}
			net_addstring(key, &count, stringcount, mystrings);
			if (ch1 == 0) { perfect = 1;   break; }
			continue;
		}

		/* '[' encountered: process array entries */
		if (savekey != 0) efree(savekey);
		if (*key != 0) (void)allocstring(&savekey, key, el_tempcluster); else
		{
			/* no name before the '[', look for an assumed bus name */
			if (thisai == NOARCINST)
			{
				(void)allocstring(&savekey, x_(""), el_tempcluster);
			} else
			{
				busname = net_busnameofarc(thisai);
				if (busname == 0)
				{
					ttyputmsg(_("Cell %s, network '%s': cannot determine bus name to use"),
						describenodeproto(cell), name);
					busname = x_("XXX");
				}
				(void)allocstring(&savekey, busname, el_tempcluster);
			}
		}

		/* loop through the indexed entries */
		dimension = 0;
		dimstart = count;
		dimend = dimlen = 0; /* valid only if dimension > 0 */
		for(;;)
		{
			cindex = getkeyword(&ptin, x_(",:]"));
			ch2 = tonextchar(&ptin);
			if (cindex == NOSTRING) break;
			if (*cindex == 0)
			{
				if (showerrors != 0)
					ttyputmsg(_("Cell %s, network '%s': empty network index"),
						describenodeproto(cell), name);
				break;
			}
			if (net_isanumber(cindex))
			{
				indexval = myatoi(cindex);
				if (indexval < 0)
				{
					if (showerrors != 0)
						ttyputmsg(_("Cell %s, network '%s': array indices cannot be negative"),
							describenodeproto(cell), name);
					break;
				}
			} else indexval = -1;
			if (ch2 == ']' || ch2 == ',')
			{
				/* add entry "indexval" in the array with name "key" */
				if (dimension > 0)
				{
					for(i=dimstart; i<dimend; i++)
					{
						infstr = initinfstr();
						formatinfstr(infstr, x_("%s[%s]"), (*mystrings)[i], cindex);
						net_insertstring(returninfstr(infstr), dimend + dimlen + (i-dimstart)*(dimlen+1),
							&count, stringcount, mystrings);
					}
					dimlen++;
				} else
				{
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s[%s]"), savekey, cindex);
					net_addstring(returninfstr(infstr), &count, stringcount, mystrings);
				}
			} else
			{
				/* ':' found, handle range of values */
				if (indexval < 0)
				{
					if (showerrors != 0)
						ttyputerr(_("Warning (cell %s): network '%s' has nonnumeric start of index range"),
							describenodeproto(cell), name);
					break;
				}
				endindex = getkeyword(&ptin, x_(",]"));
				ch2 = tonextchar(&ptin);
				if (endindex == NOSTRING) break;
				if (*endindex == 0)
				{
					if (showerrors != 0)
						ttyputmsg(_("Warning (cell %s): network '%s' has missing end of index range"),
							describenodeproto(cell), name);
					break;
				}
				if (!net_isanumber(endindex))
				{
					if (showerrors != 0)
						ttyputmsg(_("Warning (cell %s): network '%s' has nonnumeric end of index range"),
							describenodeproto(cell), name);
					break;
				}
				endindexval = myatoi(endindex);
				if (endindexval < 0)
				{
					if (showerrors != 0)
						ttyputmsg(_("Warning (cell %s): network '%s' has negative end of index range"),
							describenodeproto(cell), name);
					break;
				}
				if (endindexval == indexval)
				{
					if (showerrors != 0)
						ttyputmsg(_("Warning (cell %s): network '%s' has equal start and end indices"),
							describenodeproto(cell), name);
					break;
				}

				/* add an array from "indexval" to "endindexval" */
				if (indexval < endindexval)
				{
					for(index=indexval; index<=endindexval; index++)
					{
						if (dimension > 0)
						{
							for(i=dimstart; i<dimend; i++)
							{
								infstr = initinfstr();
								formatinfstr(infstr, x_("%s[%ld]"), (*mystrings)[dimstart+i], index);
								net_insertstring(returninfstr(infstr), dimend + dimlen + i*(dimlen+1),
									&count, stringcount, mystrings);
							}
							dimlen++;
						} else
						{
							infstr = initinfstr();
							formatinfstr(infstr, x_("%s[%ld]"), savekey, index);
							net_addstring(returninfstr(infstr), &count, stringcount, mystrings);
						}
					}
				} else
				{
					for(index=indexval; index>=endindexval; index--)
					{
						if (dimension > 0)
						{
							for(i=dimstart; i<dimend; i++)
							{
								infstr = initinfstr();
								formatinfstr(infstr, x_("%s[%ld]"), (*mystrings)[dimstart+i], index);
								net_insertstring(returninfstr(infstr), dimend + dimlen + i*(dimlen+1),
									&count, stringcount, mystrings);
							}
							dimlen++;
						} else
						{
							infstr = initinfstr();
							formatinfstr(infstr, x_("%s[%ld]"), savekey, index);
							net_addstring(returninfstr(infstr), &count, stringcount, mystrings);
						}
					}
				}
			}
			if (ch2 == ',') continue;

			/* at the "]", clean up multidimensional array */
			if (dimension > 0)
			{
				range = dimend - dimstart;
				for(i=dimend; i<count; i++)
				{
					key = (*mystrings)[i-range];
					(*mystrings)[i-range] = (*mystrings)[i];
					(*mystrings)[i] = key;
				}
				count-= range;
			}

			/* see if a "[" follows it for a multidimensional array */
			if (*ptin != '[')
			{
				perfect = 1;
				break;
			}
			ptin++;
			dimend = count;
			dimension++;
			dimlen = 0;
#ifdef MULTIDIMENSIONALBUS
			continue;
#else
			break;
#endif
		}

		if (!perfect) break;
		perfect = 0;
		/* see what follows the ']' */
		key = getkeyword(&ptin, x_(","));
		if (key == NOSTRING) break;
		ch1 = tonextchar(&ptin);
		if (*key != 0)
		{
			if (showerrors != 0)
				ttyputmsg(_("Cell %s, network '%s': missing comma between names"),
					describenodeproto(cell), name);
			break;
		}
		if (ch1 == 0) { perfect = 1;   break; }
	}
	if (savekey != 0) efree(savekey);

	/* if no strings were extracted, treat as wire */
	origfunct = funct;
	if (count == 0) funct = APUNKNOWN;

	/* see if multiple signals were found on single-signal arc */
	if ((count != 1 && funct != APBUS) || perfect == 0)
	{
		errorstring = (CHAR *)emalloc((estrlen(name)+1) * SIZEOFCHAR, el_tempcluster);
		(void)estrcpy(errorstring, name);
		for(ptin = errorstring; *ptin != 0; ptin++)
			if (*ptin == ',' || *ptin == ':' || *ptin == '[' || *ptin == ']')
				*ptin = 'X';
		if (showerrors != 0)
		{
			if (origfunct != APBUS)
				ttyputerr(_("Warning (cell %s): network '%s' cannot name a single wire, using '%s'"),
					describenodeproto(cell), name, errorstring); else
						ttyputerr(_("Warning (cell %s): network name '%s' is unintelligible, using '%s'"),
							describenodeproto(cell), name, errorstring);
		}
		count = 0;
		net_addstring(errorstring, &count, stringcount, mystrings);
		*strings = *mystrings;
		efree(errorstring);
		return(count);
	}

	/* if there are errors, build what is available */
	if (showerrors != 0)
	{
		if (perfect == 0)
			ttyputerr(_("Warning (cell %s): network name '%s' is unintelligible"),
				describenodeproto(cell), name);
	}

	*strings = *mystrings;
	return(count);
}

/*
 * Routine to follow arc "ai" and find a bus with a unique name.  This is the name to
 * use for the arc (which has been named with an empty bus name and just an index).
 */
CHAR *net_busnameofarc(ARCINST *ai)
{
	REGISTER ARCINST *oai;
	REGISTER CHAR *ch;

	if (ai != NOARCINST)
	{
		for(oai = ai->parent->firstarcinst; oai != NOARCINST; oai = oai->nextarcinst)
			oai->temp1 = 0;
		ch = net_findnameofbus(ai, TRUE);
		if (ch != 0) return(ch);
		ch = net_findnameofbus(ai, FALSE);
		if (ch != 0) return(ch);
	}
	return(0);
}

/*
 * Routine to determine the name of the bus on arc "ai".  The assumption is that
 * the bus has been partially named (i.e. "[0:2]") and that some other bus has
 * a more full name.  Only searches bus arcs if "justbus" is true.
 */
CHAR *net_findnameofbus(ARCINST *ai, BOOLEAN justbus)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *oai;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER INTBIG i, fun;
	REGISTER CHAR *ch;
	REGISTER void *infstr;

	if (ai->proto == sch_busarc)
	{
		if (ai->network != NONETWORK)
		{
			if (ai->network->namecount > 0)
			{
				infstr = initinfstr();
				for(ch = networkname(ai->network, 0); *ch != 0;  ch++)
				{
					if (*ch == '[') break;
					addtoinfstr(infstr, *ch);
				}
				return(returninfstr(infstr));
			}
		}
	}
	ai->temp1 = 1;
	for(i=0; i<2; i++)
	{
		ni = ai->end[i].nodeinst;
		fun = (ni->proto->userbits & NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPPIN && fun != NPCONTACT && fun != NPNODE && fun != NPCONNECT) continue;

		/* follow arcs out of this node */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if ((pi->proto->userbits&PORTISOLATED) != 0) continue;
			oai = pi->conarcinst;
			if (oai->temp1 != 0) continue;
			if (justbus && oai->proto != sch_busarc) continue;

			ch = net_findnameofbus(oai, justbus);
			if (ch != 0) return(ch);
		}

		/* look at exports for array names */
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (net_buswidth(pe->exportproto->protoname) > 1)
			{
				infstr = initinfstr();
				for(ch = pe->exportproto->protoname; *ch != 0;  ch++)
				{
					if (*ch == '[') break;
					addtoinfstr(infstr, *ch);
				}
				return(returninfstr(infstr));
			}
		}
	}
	return(0);
}

void net_insertstring(CHAR *key, INTBIG index, INTBIG *count, INTBIG *stringcount, CHAR ***mystrings)
{
	REGISTER CHAR **newstrings, *lastone;
	REGISTER INTBIG i;

	if (*count >= *stringcount)
	{
		newstrings = (CHAR **)emalloc(((sizeof (CHAR *)) * (*count + 1)), net_tool->cluster);
		if (newstrings == 0) return;
		for(i=0; i < *stringcount; i++)
			newstrings[i] = (*mystrings)[i];
		for(i = *stringcount; i < *count + 1; i++)
			(void)allocstring(&newstrings[i], x_(""), net_tool->cluster);
		if (*stringcount != 0)
			efree((CHAR *)*mystrings);
		*stringcount = *count + 1;
		*mystrings = newstrings;
	}

	/* rearrange */
	lastone = (*mystrings)[*count];
	for(i = *count; i > index; i--)
		(*mystrings)[i] = (*mystrings)[i-1];
	(*mystrings)[index] = lastone;

	(void)reallocstring(&(*mystrings)[index], key, net_tool->cluster);
	(*count)++;
}

void net_addstring(CHAR *key, INTBIG *count, INTBIG *stringcount, CHAR ***mystrings)
{
	REGISTER CHAR **newstrings;
	REGISTER INTBIG i;

	if (*count >= *stringcount)
	{
		newstrings = (CHAR **)emalloc(((sizeof (CHAR *)) * (*count + 1)), net_tool->cluster);
		if (newstrings == 0) return;
		for(i=0; i < *stringcount; i++)
			newstrings[i] = (*mystrings)[i];
		for(i = *stringcount; i < *count + 1; i++)
			(void)allocstring(&newstrings[i], x_(""), net_tool->cluster);
		if (*stringcount != 0)
			efree((CHAR *)*mystrings);
		*stringcount = *count + 1;
		*mystrings = newstrings;
	}
	(void)reallocstring(&(*mystrings)[*count], key, net_tool->cluster);
	(*count)++;
}

/*
 * routine to ensure that single-wire arc "ai" properly connects to the bus
 * running through bus-pin "ni".  Prints a warning message if error found.
 */
void net_checkvalidconnection(NODEINST *ni, ARCINST *ai)
{
	REGISTER PORTARCINST *pi;
	REGISTER ARCINST *oai;
	REGISTER INTBIG j, found;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;

	/* find a bus arc on this node */
	if (ai->network == NONETWORK) return;
	found = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		oai = pi->conarcinst;
		if (oai->proto != sch_busarc) continue;
		if (oai->network == NONETWORK) continue;
		if (oai->network->buswidth <= 1) continue;

		/* make sure the bus has this network on it */
		for(j=0; j<oai->network->buswidth; j++)
			if (oai->network->networklist[j] == ai->network) return;
		found++;
	}

	/* now check exported bus pins */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		pp = pe->exportproto;
		if (pp->network == NONETWORK) continue;
		if (pp->network->buswidth <= 1) continue;

		/* make sure the bus has this network on it */
		for(j=0; j<pp->network->buswidth; j++)
			if (pp->network->networklist[j] == ai->network) return;
		found++;
	}

	if (found > 0)
	{
		ttyputmsg(_("Warning (cell %s): network '%s' not a part of connected busses"),
			describenodeproto(ai->parent), describenetwork(ai->network));
		if (ai->network->namecount == 0)
			ttyputmsg(_("   (Set a network name on the '%s' arc)"), describearcinst(ai));
	}
}

/*
 * Routine to ensure that network "net" (a bus) has a proper bus name if it is named temporarily.
 */
void net_ensuretempbusname(NETWORK *net)
{
	INTBIG count, base;
	REGISTER void *infstr;
	CHAR *netname;

	if (net->buswidth <= 1) return;
	if (net->tempname == 0) return;
	if (net->namecount != 1) return;
	netname = networkname(net, 0);
	count = net_buswidth(netname);
	if (count != 1) return;
	infstr = initinfstr();
	addstringtoinfstr(infstr, netname);
	if ((net_options&NETDEFBUSBASE1) == 0) base = 0; else base = 1;
	if ((net_options&NETDEFBUSBASEDESC) == 0)
	{
		formatinfstr(infstr, x_("[%ld:%ld]"), base, net->buswidth-1+base);
	} else
	{
		formatinfstr(infstr, x_("[%ld:%ld]"), net->buswidth-1+base, base);
	}
	((NetName*)net->netnameaddr)->removeNet(net);
	net->namecount = 0;
#ifdef NEWRENUM
	NetName *nn = net->parent->netd->findNetName(returninfstr(infstr), TRUE);
	(void)net_namenet(nn, net);
#else
	(void)net_namenet(returninfstr(infstr), net);
#endif
}

/*
 * Routine to evaluate the implied bus width of the string 'name'
 */
INTBIG net_buswidth(CHAR *name)
{
	REGISTER INTBIG count;
	CHAR **strings;
	
	count = net_evalbusname(APBUS, name, &strings, NOARCINST, NONODEPROTO, 0);
	return(count);
}

/*
 * Routine to implement the "wire_con" primitive which joins two arcs and attaches
 * busses of unequal length (by repeating signals on the shorter bus until fully
 * attached to the larger bus).
 */
void net_joinnetworks(NODEINST *ni)
{
	REGISTER ARCINST *ai, *smallai, *largeai;
	REGISTER INTBIG smallnum, largenum, num, wirecount, i;
	REGISTER PORTARCINST *pi;
	REGISTER NETWORK *net, *smallnet, *largenet;

	/* find the narrow and wide busses on this connector */
	smallai = largeai = NOARCINST;
	smallnum = largenum = 0;
	wirecount = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		ai = pi->conarcinst;
		wirecount++;
		net = ai->network;
		if (net == NONETWORK) continue;
		num = net->buswidth;
		if (smallai == NOARCINST || num <= smallnum)
		{
			smallai = ai;
			smallnum = num;
		}
		if (largeai == NOARCINST || num > largenum)
		{
			largeai = ai;
			largenum = num;
		}
	}
	if (wirecount < 2) return;
	if (wirecount > 2)
	{
		ttyputmsg(_("Cell %s, connector %s can only merge two arcs (has %ld)"),
			describenodeproto(ni->parent), describenodeinst(ni), wirecount);
		return;
	}
	if (smallai == largeai) return;

	for(i=0; i<largenum; i++)
	{
		if (smallnum == 1) smallnet = smallai->network; else
		{
			smallnet = smallai->network->networklist[i % smallnum];
		}
		if (largenum == 1) largenet = largeai->network; else
		{
			largenet = largeai->network->networklist[i];
		}
		if (smallnet == largenet) continue;
		(void)net_mergenet(smallnet, largenet);
	}

	/* also merge the busses if they are the same length */
	if (smallnum == largenum && smallnum > 1)
	{
		(void)net_mergenet(smallai->network, largeai->network);
	}
}

/*
 * Routine to determine the width of bus arc "ai" by looking at cell instances that
 * connect to it.  Returns the node and port that was used to determine the size (if
 * a bus).
 */
INTBIG net_buswidthofarc(ARCINST *ai, NODEINST **arrayni, PORTPROTO **arraypp)
{
	REGISTER INTBIG buswidth, width;
	REGISTER NODEINST *ni;

	if (ai->proto != sch_busarc) return(1);
	ni = ai->end[0].nodeinst;
	if (ni->proto->primindex != 0) buswidth = 1; else
	{
		buswidth = ai->end[0].portarcinst->proto->network->buswidth;
		if (ni->arraysize > 1) buswidth *= ni->arraysize;
		if (buswidth > 1)
		{
			*arrayni = ai->end[0].nodeinst;
			*arraypp = ai->end[0].portarcinst->proto;
		}
	}

	ni = ai->end[1].nodeinst;
	if (ni->proto->primindex == 0)
	{
		width = ai->end[1].portarcinst->proto->network->buswidth;
		if (ni->arraysize > 1) width *= ni->arraysize;
		if (width > 1)
		{
			*arrayni = ai->end[1].nodeinst;
			*arraypp = ai->end[1].portarcinst->proto;
		}
		if (buswidth == 1) buswidth = width; else
			if (buswidth != width) buswidth = 1;
	}
	return(buswidth);
}

#define FINDINTERBUSSHORT 1

/*
 * Routine to locate individual signals contained within busses in
 * node instance ni which are connected inside of the contentsview
 * of ni->proto, and then to merge the two individual networks.
 */
BOOLEAN net_mergebuswires(NODEINST *ni)
{
	REGISTER NODEPROTO *np, *cnp;
	REGISTER PORTPROTO *pp, *opp, *cpp, *ocpp;
	REGISTER NETWORK *net, *onet, *cnet, *ocnet, *cmnet, *ocmnet, *topnet, *otopnet;
	REGISTER INTBIG j, k, a, nodewidth;
	REGISTER BOOLEAN recheck;

	/* primitives do not connect individual signals in busses */
	if (ni->proto->primindex != 0) return(FALSE);

	/* establish connections to contents nodeproto */
	np = ni->proto;
	nodewidth = ni->arraysize;
	if (nodewidth < 1) nodewidth = 1;
	cnp = contentsview(np);
	if (cnp == NONODEPROTO) cnp = np;

	/* presume no checking above is required */
	recheck = FALSE;

	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* see if this port can connect to a bus */
		for(j=0; pp->connects[j] != NOARCPROTO; j++)
			if (pp->connects[j] == sch_busarc) break;
		if (pp->connects[j] == NOARCPROTO) continue;

		/* make sure this port has a network attached to it at the current level */
		net = getnetonport(ni, pp);
		if (net == NONETWORK) continue;

		/* nothing to do if the network is a single wire */
		if (net->buswidth <= 1) continue;

		/* find the network inside of the contents */
		cpp = equivalentport(np, pp, cnp);
		if (cpp == NOPORTPROTO) continue;
		cnet = cpp->network;

		/* make sure the networks match width, inside and out */
		if (net->buswidth != cnet->buswidth && net->buswidth != cnet->buswidth * nodewidth)
		{
			if (nodewidth > 1)
			{
				ttyputmsg(_("Warning (cell %s): bus %s cannot connect to port %s of %ld-wide node %s (different width)"),
					describenodeproto(ni->parent), describenetwork(net), pp->protoname,
						nodewidth, describenodeinst(ni));
			} else
			{
				ttyputmsg(_("Warning (cell %s): bus %s cannot connect to port %s of node %s (different width)"),
					describenodeproto(ni->parent), describenetwork(net), pp->protoname,
						describenodeinst(ni));
			}
			continue;
		}

		/* now find another network attached to this node instance */
		for (opp = np->firstportproto; opp != NOPORTPROTO; opp = opp->nextportproto)
		{
#ifndef FINDINTERBUSSHORT
			if (opp == pp) continue;
#endif

			/* make sure this port has a different network attached to it at the current level */
			onet = getnetonport(ni, opp);
			if (onet == NONETWORK) continue;
#ifndef FINDINTERBUSSHORT
			if (onet == net) continue;
#endif

			/* find the network inside of the contents */
			ocpp = equivalentport(np, opp, cnp);
			if (ocpp == NOPORTPROTO) continue;
			ocnet = ocpp->network;

			/* make sure the networks match width, inside and out */
			if (onet->buswidth != ocnet->buswidth && onet->buswidth != ocnet->buswidth * nodewidth)
			{
				if (nodewidth > 1)
				{
					ttyputmsg(_("Warning (cell %s): bus %s cannot connect to port %s of %ld-wide node %s (different width)"),
						describenodeproto(ni->parent), describenetwork(onet), opp->protoname,
							nodewidth, describenodeinst(ni));
				} else
				{
					ttyputmsg(_("Warning (cell %s): bus %s cannot connect to port %s of node %s (different width)"),
						describenodeproto(ni->parent), describenetwork(onet), opp->protoname,
							describenodeinst(ni));
				}
				continue;
			}

#ifdef FINDINTERBUSSHORT
			/* when the same net, check for shorts that are internal to a bus */
			if (ocnet == cnet)
			{
				if (cnet->buswidth <= 1) continue;
				for(j=0; j<cnet->buswidth; j++)
				{
					cmnet = cnet->networklist[j];
					for (k=j+1; k<ocnet->buswidth; k++)
					{
						ocmnet = ocnet->networklist[k];
						if (cmnet != ocmnet) continue;

						/* merge found: propagate it to the top level */
						for(a=0; a<nodewidth; a++)
						{
							if (net->buswidth > cnet->buswidth)
								topnet = net->networklist[j + a*cnet->buswidth]; else
							{
								if (net->buswidth == 1) topnet = net; else
									topnet = net->networklist[j];
							}
							if (onet->buswidth > ocnet->buswidth)
								otopnet = onet->networklist[k + a*ocnet->buswidth]; else
							{
								if (onet->buswidth == 1) otopnet = onet; else
									otopnet = onet->networklist[k];
							}							
							(void)net_mergenet(topnet, otopnet);

							/* if neither arc is arrayed with the node, stop after 1 merge */
							if (net->buswidth == cnet->buswidth && onet->buswidth == ocnet->buswidth) break;
						}
					}
				}
				continue;
			}
#endif

			/* look for merges inside of the cell */
			for(j=0; j<cnet->buswidth; j++)
			{
				if (cnet->buswidth == 1) cmnet = cnet; else
					cmnet = cnet->networklist[j];

				for (k=0; k<ocnet->buswidth; k++)
				{
					if (ocnet->buswidth == 1) ocmnet = ocnet; else
						ocmnet = ocnet->networklist[k];
					if (cmnet != ocmnet) continue;

					/* merge found: propagate it to the top level */
					for(a=0; a<nodewidth; a++)
					{
						if (net->buswidth > cnet->buswidth)
							topnet = net->networklist[j + a*cnet->buswidth]; else
						{
							if (net->buswidth == 1) topnet = net; else
								topnet = net->networklist[j];
						}
						if (onet->buswidth > ocnet->buswidth)
							otopnet = onet->networklist[k + a*ocnet->buswidth]; else
						{
							if (onet->buswidth == 1) otopnet = onet; else
								otopnet = onet->networklist[k];
						}							
						(void)net_mergenet(topnet, otopnet);

						/* if neither arc is arrayed with the node, stop after 1 merge */
						if (net->buswidth == cnet->buswidth && onet->buswidth == ocnet->buswidth) break;
					}
				}
			}
		}
	}
	return(recheck);
}

/*
 * Routine to examine the network names on networks "net1" and "net2" and return
 * true if they share a common name.
 */
BOOLEAN net_samenetworkname(NETWORK *net1, NETWORK *net2)
{
	REGISTER CHAR *pt1, *pt2;
	CHAR **strings;
	REGISTER INTBIG n1, n2, wid1, wid2, i1, i2;

	for(n1 = 0; n1 < net1->namecount; n1++)
	{
		pt1 = networkname(net1, n1);
		wid1 = net_evalbusname(APBUS, pt1, &strings, NOARCINST, net1->parent, 0);
		if (wid1 > net_namecompstringtotal)
		{
			if (net_namecompstringtotal > 0) efree((CHAR *)net_namecompstrings);
			net_namecompstringtotal = 0;
			net_namecompstrings = (CHAR **)emalloc(wid1 * (sizeof (CHAR *)), net_tool->cluster);
			if (net_namecompstrings == 0) return(TRUE);
			net_namecompstringtotal = wid1;
		}
		if (wid1 == 1) net_namecompstrings[0] = pt1; else
		{
			for(i1=0; i1<wid1; i1++)
				(void)allocstring(&net_namecompstrings[i1], strings[i1], net_tool->cluster);
		}

		for(n2 = 0; n2 < net2->namecount; n2++)
		{
			pt2 = networkname(net2, n2);
			wid2 = net_evalbusname(APBUS, pt2, &strings, NOARCINST, net2->parent, 0);
			for(i1=0; i1<wid1; i1++)
			{
				for(i2=0; i2<wid2; i2++)
				{
					if (namesame(net_namecompstrings[i1], strings[i2]) == 0) return(TRUE);
				}
			}
		}
		if (wid1 > 1)
		{
			for(i1=0; i1<wid1; i1++)
				efree((CHAR *)net_namecompstrings[i1]);
		}
	}
	return(FALSE);
}

/*
 * Routine to add arc "ai", export "pp" and width "width" to the list of busses.
 */
void net_addtobuslist(ARCINST *ai, PORTPROTO *pp, INTBIG width)
{
	REGISTER INTBIG newtotal, i;
	REGISTER BUSLIST *newbuslists;

	if (net_buslistcount >= net_buslisttotal)
	{
		newtotal = net_buslisttotal * 2;
		if (newtotal <= net_buslistcount) newtotal = net_buslistcount + 5;
		newbuslists = (BUSLIST *)emalloc(newtotal * (sizeof (BUSLIST)), net_tool->cluster);
		if (newbuslists == 0) return;
		for(i=0; i<net_buslistcount; i++)
			newbuslists[i] = net_buslists[i];
		if (net_buslisttotal > 0) efree((CHAR *)net_buslists);
		net_buslists = newbuslists;
		net_buslisttotal = newtotal;
	}
	net_buslists[net_buslistcount].ai = ai;
	net_buslists[net_buslistcount].pp = pp;
	net_buslists[net_buslistcount].width = width;
	net_buslistcount++;
}

/*********************** NETLIST GEOMETRY ROUTINES ***********************/

/*
 * Routine to show the geometry on network "net".
 */
void net_showgeometry(NETWORK *net)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, j, widest, len, lambda, total, fun, totalWire;
	REGISTER CHAR *lname, *pad;
	REGISTER AREAPERIM *arpe, *firstarpe, **arpelist;
	TRANSISTORINFO *p_gate, *n_gate, *p_active, *n_active;
	float ratio;
	REGISTER void *infstr;

	/* gather geometry on this network */
	np = net->parent;
	firstarpe = net_gathergeometry(net, &p_gate, &n_gate, &p_active, &n_active, TRUE);

	/* copy the linked list to an array for sorting */
	total = 0;
	for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = arpe->nextareaperim)
		if (arpe->layer >= 0) total++;
	if (total == 0)
	{
		ttyputmsg(_("No geometry on network '%s' in cell %s"), describenetwork(net),
			describenodeproto(np));
		return;
	}
	arpelist = (AREAPERIM **)emalloc(total * (sizeof (AREAPERIM *)), net_tool->cluster);
	if (arpelist == 0) return;
	i = 0;
	for(arpe = firstarpe; arpe != NOAREAPERIM; arpe = arpe->nextareaperim)
		if (arpe->layer >= 0) arpelist[i++] = arpe;

	/* sort the layers */
	esort(arpelist, total, sizeof (AREAPERIM *), net_areaperimdepthascending);

	ttyputmsg(_("For network '%s' in cell %s:"), describenetwork(net),
		describenodeproto(np));
	lambda = lambdaofcell(np);
	widest = 0;
	for(i=0; i<total; i++)
	{
		arpe = arpelist[i];
		lname = layername(arpe->tech, arpe->layer);
		len = estrlen(lname);
		if (len > widest) widest = len;
	}
	totalWire = 0;
	for(i=0; i<total; i++)
	{
		arpe = arpelist[i];
		lname = layername(arpe->tech, arpe->layer);
		infstr = initinfstr();
		for(j=estrlen(lname); j<widest; j++) addtoinfstr(infstr, ' ');
		pad = returninfstr(infstr);
		if (arpe->perimeter == 0)
		{
			ttyputmsg(_("Layer %s:%s area=%7g  half-perimeter=%s"), lname, pad,
				arpe->area/(float)lambda/(float)lambda, latoa(arpe->perimeter/2, 0));
		} else
		{
			ratio = (arpe->area / (float)lambda) / (float)(arpe->perimeter/2);
			ttyputmsg(_("Layer %s:%s area=%7g  half-perimeter=%s ratio=%g"), lname,
				pad, arpe->area/(float)lambda/(float)lambda,
					latoa(arpe->perimeter/2, lambda), ratio);

			/* accumulate total wire length on all metal/poly layers */
			fun = layerfunction(arpe->tech, arpe->layer);
			if ((layerispoly(fun) && !layerisgatepoly(fun)) || layerismetal(fun))
				totalWire += arpe->perimeter / 2;
		}
		efree((CHAR *)arpelist[i]);
	}
	if (totalWire > 0.0) ttyputmsg(_("Total wire length = %s"), latoa(totalWire,lambda));
	efree((CHAR *)arpelist);
}

/*
 * Helper routine for "net_showgeometry()" to sort AREAPERIM objects by depth
 */
int net_areaperimdepthascending(const void *e1, const void *e2)
{
	REGISTER AREAPERIM *ap1, *ap2;
	REGISTER INTBIG fun1, fun2, depth1, depth2;

	ap1 = *((AREAPERIM **)e1);
	ap2 = *((AREAPERIM **)e2);
	fun1 = layerfunction(ap1->tech, ap1->layer);
	depth1 = layerfunctionheight(fun1);
	if (layeriscontact(fun1)) depth1 -= 1000;
	fun2 = layerfunction(ap2->tech, ap2->layer);
	depth2 = layerfunctionheight(fun2);
	if (layeriscontact(fun2)) depth2 -= 1000;
	return(depth2 - depth1);
}

/*
 * Helper routine for "net_reevaluatecell()" to sort BUSLIST objects by width
 */
int net_buslistwidthascending(const void *e1, const void *e2)
{
	REGISTER BUSLIST *b1, *b2;

	b1 = (BUSLIST *)e1;
	b2 = (BUSLIST *)e2;
	return(b1->width - b2->width);
}

/*
 * Routine to gather the geometry on network "net".  Must deallocate all of the AREAPERIM
 * objects created by this routine.
 */
AREAPERIM *net_gathergeometry(NETWORK *net, TRANSISTORINFO **p_gate, TRANSISTORINFO **n_gate,
	TRANSISTORINFO **p_active, TRANSISTORINFO **n_active, BOOLEAN recurse)
{
	REGISTER NODEPROTO *np;
	REGISTER NETWORK *onet;

	np = net->parent;

	/* initialize polygon merging */
	net_merge = mergenew(net_tool->cluster);

	/* mark the networks in this cell that are of interest */
	for(onet = np->firstnetwork; onet != NONETWORK; onet = onet->nextnetwork)
		onet->temp1 = 0;
	net->temp1 = 1;
	net_cleartransistorinfo(&net_transistor_p_gate);
	net_cleartransistorinfo(&net_transistor_n_gate);
	net_cleartransistorinfo(&net_transistor_p_active);
	net_cleartransistorinfo(&net_transistor_n_active);
	net_firstareaperim = NOAREAPERIM;

	/* examine circuit recursively */
	net_propgeometry(np, el_matid, recurse);

	/* get back the total geometry */
	mergeextract(net_merge, net_geometrypolygon);
	mergedelete(net_merge);

	/* store the transistor area information in the parameters */
	if (p_gate != 0) *p_gate = &net_transistor_p_gate;
	if (n_gate != 0) *n_gate = &net_transistor_n_gate;
	if (p_active != 0) *p_active = &net_transistor_p_active;
	if (n_active != 0) *n_active = &net_transistor_n_active;
	return(net_firstareaperim);
}

/*
 * Helper routine to gather the geometry in cell "cell" and below where the network's
 * "temp1" is nonzero.  Appropriate polygons are merged.  "trans" is the transformation
 * to this point in the hierarchy.
 */
void net_propgeometry(NODEPROTO *cell, XARRAY trans, BOOLEAN recurse)
{
	static POLYGON *poly = NOPOLYGON;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp;
	XARRAY rot, trn, rottrn, subrot;
	REGISTER NETWORK *net;
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG fun, nfun, ignorelayer, height, highest, polys, diffs;
	REGISTER INTBIG total, i, found;
	INTBIG length, width;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, db_cluster);

	/* include all arcs on desired networks */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network == NONETWORK || ai->network->temp1 == 0) continue;
		total = arcpolys(ai, NOWINDOWPART);
		for(i=0; i<total; i++)
		{
			shapearcpoly(ai, i, poly);
			xformpoly(poly, trans);
			mergeaddpolygon(net_merge, poly->layer, ai->proto->tech, poly);
		}
	}
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* ignore recursive references (showing icon in contents) */
		subnp = ni->proto;
		if (isiconof(subnp, cell)) continue;

		/* see if any selected networks touch this node */
		if (subnp->primindex == 0 && recurse == TRUE)
		{
			for(net = subnp->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				net->temp1 = 0;
		} else
		{
			for(pp = subnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
				pp->network->temp1 = 0;
		}
		found = 0;
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			if (pi->conarcinst->network == NONETWORK || pi->conarcinst->network->temp1 == 0) continue;
			pi->proto->network->temp1 = 1;
			found = 1;
		}
		for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		{
			if (pe->exportproto->network->temp1 == 0) continue;
			pe->proto->network->temp1 = 1;
			found = 1;
		}
		if (found == 0) continue;

		if (subnp->primindex == 0 && recurse == TRUE)
		{
			/* cell instance: recurse */
			if (subnp->cellview != el_iconview)
			{
				makerot(ni, rot);
				maketrans(ni, trn);
				transmult(trn, rot, rottrn);
				transmult(rottrn, trans, subrot);
				net_propgeometry(subnp, subrot, recurse);
			}
		} else
		{
			/* primitive: include layers that touch desired networks */
			makerot(ni, rot);
			transmult(rot, trans, subrot);
			ignorelayer = -1;
			nfun = nodefunction(ni);
			if (nfun == NPCONTACT)
			{
				/* find highest layer and ignore it */
				total = nodeEpolys(ni, 0, NOWINDOWPART);
				highest = -1;
				for(i=0; i<total; i++)
				{
					shapeEnodepoly(ni, i, poly);
					fun = layerfunction(ni->proto->tech, poly->layer);
					if ((fun&LFPSEUDO) != 0) continue;
					height = layerfunctionheight(fun);
					if (height > highest)
					{
						highest = height;
						ignorelayer = poly->layer;
					}
				}
			}
			polys = diffs = 0;
			total = nodeEpolys(ni, 0, NOWINDOWPART);
			for(i=0; i<total; i++)
			{
				shapeEnodepoly(ni, i, poly);
				if (poly->layer == ignorelayer) continue;
				fun = layerfunction(subnp->tech, poly->layer);
				if (poly->portproto == NOPORTPROTO) continue;
				if (poly->portproto->network->temp1 == 0) continue;
				if ((fun&LFPSEUDO) != 0) continue;
				if (layerispoly(fun)) polys++;
				if ((fun&LFTYPE) == LFDIFF) diffs++;
				xformpoly(poly, subrot);
				mergeaddpolygon(net_merge, poly->layer, subnp->tech, poly);
			}
			if (nfun == NPTRANMOS)
			{
				transistorsize(ni, &length, &width);
				if (polys > 0)
					net_addtotransistorinfo(&net_transistor_n_gate, length, width);
				if (diffs > 0)
					net_addtotransistorinfo(&net_transistor_n_active, length, width);
			}
			if (nfun == NPTRAPMOS)
			{
				transistorsize(ni, &length, &width);
				if (polys > 0)
					net_addtotransistorinfo(&net_transistor_p_gate, length, width);
				if (diffs > 0)
					net_addtotransistorinfo(&net_transistor_p_active, length, width);
			}
		}
	}
}

/*
 * Helper routine to clear the TRANSISTORINFO structure "ti".
 */
void net_cleartransistorinfo(TRANSISTORINFO *ti)
{
	ti->count = 0;
	ti->area = 0;
	ti->width = 0;
	ti->length = 0;
}

/*
 * Helper routine to add a "length" and "width" transistor to the TRANSISTORINFO structure "ti".
 */
void net_addtotransistorinfo(TRANSISTORINFO *ti, INTBIG length, INTBIG width)
{
	INTBIG area;

	area = length * width;
	ti->count++;
	ti->area += area;
	ti->length += length;
	ti->width += width;
}

/*
 * Helper routine that is given merged geometry from the "network geometry" command.
 */
void net_geometrypolygon(INTBIG layer, TECHNOLOGY *tech, INTBIG *x, INTBIG *y, INTBIG count)
{
	REGISTER INTBIG per, seglen, i,  lastx, lasty, thisx, thisy;
	float area, side1, side2;
	REGISTER AREAPERIM *ap;

	/* compute the perimeter */
	per = 0;
	for(i=0; i<count; i++)
	{
		if (i == 0)
		{
			lastx = x[count-1];   lasty = y[count-1];
		} else
		{
			lastx = x[i-1];   lasty = y[i-1];
		}
		seglen = computedistance(lastx, lasty, x[i], y[i]);
		per += seglen;
	}

	/* compute the area */
	area = 0.0;
	lastx = x[0];
	lasty = y[0];
	for (i=1; i<count; i++)
	{
		thisx = x[i];
		thisy = y[i];

		/* triangulate around the polygon */
		side1 = (float)(thisx - lastx);
		side2 = (float)(lasty + thisy);
		area += (side1 * side2) / 2.0f;
		lastx = thisx;
		lasty = thisy;
	}
	side1 = (float)(x[0] - lastx);
	side2 = (float)(y[0] + lasty);
	area += (side1 * side2) / 2.0f;
	area = (float)fabs(area);

	/* find an AREAPERIM with this information */
	for(ap = net_firstareaperim; ap != NOAREAPERIM; ap = ap->nextareaperim)
		if (layer == ap->layer && tech == ap->tech) break;
	if (ap == NOAREAPERIM)
	{
		ap = (AREAPERIM *)emalloc(sizeof (AREAPERIM), net_tool->cluster);
		if (ap == 0) return;
		ap->nextareaperim = net_firstareaperim;
		net_firstareaperim = ap;
		ap->area = 0.0;
		ap->perimeter = 0;
		ap->tech = tech;
		ap->layer = layer;
	}

	/* accumulate area and perimeter */
	ap->area += area;
	ap->perimeter += per;
}

/*********************** NETLIST SEARCH ROUTINES ***********************/

/*
 * helper routine for "telltool network list-hierarchical-ports" to print all
 * ports connected to net "net" in cell "cell", and recurse up the hierarchy
 */
void net_findportsup(NETWORK *net, NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER PORTPROTO *pp;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTARCINST *pi;

	if (stopping(STOPREASONPORT)) return;

	/* look at every node in the cell */
	for(pp = cell->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->network != net) continue;
		if (pp->temp1 != 0) continue;
		pp->temp1 = 1;
		(void)ttyputmsg(_("  Export %s in cell %s"), pp->protoname,
			describenodeproto(cell));

		/* ascend to higher cell and continue */
		for(ni = cell->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			/* see if there is an arc connected to this port */
			for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
				if (pi->proto->network == pp->network)
			{
				net_findportsup(pi->conarcinst->network, ni->parent);
				break;
			}
			if (pi != NOPORTARCINST) continue;

			/* try further exporting of ports */
			for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
				if (pe->proto->network == pp->network)
			{
				net_findportsup(pe->exportproto->network, ni->parent);
				break;
			}
		}
	}
}

/*
 * helper routine for "telltool network list-hierarchical-ports" to print all
 * ports connected to net "net" in cell "cell", and recurse down the hierarchy
 */
void net_findportsdown(NETWORK *net, NODEPROTO *cell)
{
	REGISTER NODEINST *ni;
	REGISTER PORTARCINST *pi;
	REGISTER NODEPROTO *cnp, *subnp;
	REGISTER PORTPROTO *cpp;

	if (stopping(STOPREASONPORT)) return;

	/* look at every node in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		/* only want complex nodes */
		subnp = ni->proto;
		if (subnp->primindex != 0) continue;

		/* ignore recursive references (showing icon in contents) */
		if (isiconof(subnp, cell)) continue;

		/* look at all wires connected to the node */
		for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		{
			/* ignore arc if not connected to net */
			if (pi->conarcinst->network != net) continue;

			if ((cnp = contentsview(pi->proto->parent)) == NONODEPROTO)
				cnp = pi->proto->parent;
			if ((cpp = equivalentport(pi->proto->parent, pi->proto, cnp)) == NOPORTPROTO)
				cpp = pi->proto;

			if (cpp->temp1 != 0) continue;
			cpp->temp1 = 1;
			(void)ttyputmsg(_("  Export %s in cell %s"), cpp->protoname, describenodeproto(cnp));

			/* descend to lower contents cell and continue */
			net_findportsdown(cpp->network, cnp);
		}
	}
}

/*
 * Routine to return an array of selected networks, terminated by NONETWORK.
 */
NETWORK **net_gethighlightednets(BOOLEAN disperror)
{
	static NETWORK *nonet[1];
	REGISTER INTBIG i, len, fun, cursimtrace, line, fromchar, tochar;
	REGISTER CHAR *netname;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER EDITOR *e;
	REGISTER GEOM **geom;
	CHAR selected[50];

	nonet[0] = NONETWORK;
	if (el_curwindowpart == NOWINDOWPART) return(nonet);

#if SIMTOOL
	/* if current window is simulation, invade structures and find highlighted net */
	if ((el_curwindowpart->state&WINDOWTYPE) == WAVEFORMWINDOW)
	{
		cursimtrace = sim_window_gethighlighttrace();
		if (cursimtrace == 0) return(nonet);
		netname = sim_window_gettracename(cursimtrace);
		return(net_parsenetwork(netname));
	}
#endif

	/* if current window is text, invade structures and find highlighted net name */
	if ((el_curwindowpart->state&WINDOWTYPE) == TEXTWINDOW ||
		(el_curwindowpart->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		/* only understand selection of net name in Point-and-Click editor */
		e = el_curwindowpart->editor;
		if ((e->state&EDITORTYPE) != PACEDITOR) return(nonet);

		/* copy and count the number of selected characters */
		len = 0;
		selected[0] = 0;
		for(line = e->curline; line <= e->endline; line++)
		{
			if (line == e->curline) fromchar = e->curchar; else fromchar = 0;
			if (line == e->endline) tochar = e->endchar; else
				tochar = estrlen(e->textarray[line])+1;
			len += tochar - fromchar;
			if (len >= 50) return(nonet);
			(void)estrncat(selected, &e->textarray[line][fromchar], tochar - fromchar);
		}
		if (selected[0] == 0) return(nonet);

		/* turn this string into a network name */
		return(net_parsenetwork(selected));
	}

	geom = (GEOM **)asktool(us_tool, x_("get-all-objects"));
	if (geom[0] == NOGEOM)
	{
		if (disperror) ttyputerr(_("Find some objects first"));
		return(nonet);
	}

	/* gather all networks connected to selected objects */
	net_highnetscount = 0;
	for(i=0; geom[i] != NOGEOM; i++)
	{
		if (!geom[i]->entryisnode)
		{
			ai = geom[i]->entryaddr.ai;
			net_addnettolist(ai->network);
		} else
		{
			ni = geom[i]->entryaddr.ni;
			fun = nodefunction(ni);
			if (fun == NPPIN || fun == NPCONTACT ||
				fun == NPCONNECT || fun == NPNODE)
			{
				if (ni->firstportarcinst != NOPORTARCINST)
				{
					ai = ni->firstportarcinst->conarcinst;
					net_addnettolist(ai->network);
				}
			}
		}
	}
	net_addnettolist(NONETWORK);
	return(net_highnets);
}

/*
 * Routine to add network "net" to the global list of networks in "net_highnets".
 */
void net_addnettolist(NETWORK *net)
{
	REGISTER INTBIG i, newtotal;
	REGISTER NETWORK **newlist;

	/* stop if already in the list */
	for(i=0; i<net_highnetscount; i++)
		if (net == net_highnets[i]) return;

	/* ensure room in the list */
	if (net_highnetscount >= net_highnetstotal)
	{
		newtotal = net_highnetstotal * 2;
		if (net_highnetscount >= newtotal) newtotal = net_highnetscount + 5;
		newlist = (NETWORK **)emalloc(newtotal * (sizeof (NETWORK *)), net_tool->cluster);
		if (newlist == 0) return;
		for(i=0; i<net_highnetscount; i++)
			newlist[i] = net_highnets[i];
		if (net_highnetstotal > 0) efree((CHAR *)net_highnets);
		net_highnets = newlist;
		net_highnetstotal = newtotal;
	}
	net_highnets[net_highnetscount] = net;
	net_highnetscount++;
}

/*
 * routine to convert the network name "name" to a valid network.
 * Returns a list of associated networks (terminated by NONETWORK).
 */
NETWORK **net_parsenetwork(CHAR *name)
{
	REGISTER NETWORK *net, *guessnet;
	REGISTER WINDOWPART *w;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pt;
	static NETWORK *retnetwork[2];

	retnetwork[0] = retnetwork[1] = NONETWORK;
	while (*name != 0 && *name <= ' ') name++;
	if (*name == 0) return(retnetwork);

	/* handle network names encoded as "NETxxxxx" */
	if (name[0] == 'N' && name[1] == 'E' && name[2] == 'T')
	{
		guessnet = (NETWORK *)myatoi(&name[3]);

		/* validate against all possible networks */
		for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		{
			if ((w->state&WINDOWTYPE) == WAVEFORMWINDOW || (w->state&WINDOWTYPE) == DISPWINDOW ||
				(w->state&WINDOWTYPE) == DISP3DWINDOW)
			{
				np = w->curnodeproto;
				if (np == NONODEPROTO) continue;

				/* does this cell have the network? */
				for(net = np->firstnetwork; net != NONETWORK; net = net->nextnetwork)
				{
					if (net == guessnet)
					{
						retnetwork[0] = net;
						return(retnetwork);
					}
				}
			}
		}
	}

	/* if there are dots in the name, check for HSPICE network specification */
	for(pt = name; *pt != 0; pt++) if (*pt == '.') break;
	if (*pt == '.')
	{
		net = sim_spice_networkfromname(name);
		if (net != NONETWORK)
		{
			retnetwork[0] = net;
			return(retnetwork);
		}
	}

	/* see if it matches a network name in the current cell */
	np = getcurcell();
	if (np != NONODEPROTO)
	{
		if ((np->cellview->viewstate&TEXTVIEW) != 0)
		{
			np = layoutview(np);
			if (np == NONODEPROTO) return(retnetwork);
		}
		return(getcomplexnetworks(name, np));
	}

	/* not found */
	return(retnetwork);
}

/*
 * Routine to specify highlighting of the arcs on network "net" in cell "np".
 * The highlighting is added to the infinite string.
 */
void net_highlightnet(void *infstr, NODEPROTO *np, NETWORK *net)
{
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *subnp;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i, j, fun;

	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		if (ai->network == NONETWORK) continue;
		if (ai->network == net)
		{
			formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
				describenodeproto(np), (INTBIG)ai->geom);
			continue;
		}

		/* handle busses according to the nature of the network being highlighted */
		if (net->buswidth <= 1)
		{
			/* network is single wire: look for its presence on a bus arc */
			if (ai->network->buswidth > 1)
			{
				for (i=0; i<ai->network->buswidth; i++)
					if (ai->network->networklist[i] == net)
				{
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
						describenodeproto(np), (INTBIG)ai->geom);
					break;
				}
			}
		} else
		{
			/* network is a bus: check the nature of this arc */
			if (ai->network->buswidth <= 1)
			{
				/* arc is single wire: see if it is on the network bus */
				for (i=0; i<net->buswidth; i++)
					if (net->networklist[i] == ai->network)
				{
					formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
						describenodeproto(np), (INTBIG)ai->geom);
					break;
				}
			} else
			{
				/* arc is bus: see if any of its signals are on network bus */
				for (i=0; i<net->buswidth; i++)
				{
					for (j=0; j<ai->network->buswidth; j++)
						if (ai->network->networklist[j] == net->networklist[i]) break;
					if (j < ai->network->buswidth) break;
				}
				if (i < net->buswidth)
				{
					formatinfstr(infstr, x_("CELL=%s LINE=%ld,%ld,%ld,%ld\n"),
						describenodeproto(np), ai->end[0].xpos, ai->end[1].xpos,
							ai->end[0].ypos, ai->end[1].ypos);
				}
			}
		}
	}

	/* now highlight all pin-type nodes on the network */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		subnp = ni->proto;
		if (subnp->primindex == 0)
		{
			/* show network where it travels through a subcell (layout only) */
			if (subnp->cellview == el_iconview) continue;

			/* see if the network hits the instance */
			net_highlightsubnet(infstr, np, ni, el_matid, net);
			continue;
		}
		fun = nodefunction(ni);
		if (fun != NPPIN && fun != NPCONTACT && fun != NPNODE && fun != NPCONNECT)
			continue;
		if (ni->firstportarcinst == NOPORTARCINST) continue;
		if (ni->firstportarcinst->conarcinst->network != net) continue;
		formatinfstr(infstr, x_("CELL=%s FROM=0%lo;-1;0\n"),
			describenodeproto(np), (INTBIG)ni->geom);
	}

	/* finally highlight all exports on the network */
	for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		if (pp->network == net)
		{
			formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;-\n"),
				describenodeproto(np), (INTBIG)pp->subnodeinst->geom, (INTBIG)pp);
		}
		if (net->buswidth <= 1)
		{
			/* network is single wire: look for its presence on a bus export */
			if (pp->network->buswidth > 1)
			{
				for (i=0; i<pp->network->buswidth; i++)
					if (pp->network->networklist[i] == net)
				{
					formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;-\n"),
						describenodeproto(np), (INTBIG)pp->subnodeinst->geom, (INTBIG)pp);
					break;
				}
			}
		} else
		{
			/* network is a bus: check the nature of this export */
			if (pp->network->buswidth <= 1)
			{
				/* export is single wire: see if it is on the network bus */
				for (i=0; i<net->buswidth; i++)
					if (net->networklist[i] == pp->network)
				{
					formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;-\n"),
						describenodeproto(np), (INTBIG)pp->subnodeinst->geom, (INTBIG)pp);
					break;
				}
			} else
			{
				/* export is bus: see if any of its signals are on network bus */
				for (i=0; i<net->buswidth; i++)
				{
					for (j=0; j<pp->network->buswidth; j++)
						if (pp->network->networklist[j] == net->networklist[i]) break;
					if (j < pp->network->buswidth) break;
				}
				if (i < net->buswidth)
				{
					formatinfstr(infstr, x_("CELL=%s TEXT=0%lo;0%lo;-\n"),
						describenodeproto(np), (INTBIG)pp->subnodeinst->geom, (INTBIG)pp);
				}
			}
		}
	}
}

/*
 * Routine to recursively highlight a subnet in a layout cell instance and add it to the infinite
 * string "infstr".  The top cell is "topnp" and the instance that is being expanded is "ni" (with
 * network "net" on that instance being highlighted).  The transformation matrix to that instance
 * (not including the instance) is "trans".
 */
void net_highlightsubnet(void *infstr, NODEPROTO *topnp, NODEINST *thisni, XARRAY trans, NETWORK *net)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;
	REGISTER ARCINST *ai;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *subnp;
	REGISTER NETWORK *subnet, **subnets;
	LISTINTBIG *li;
	INTBIG x1, y1, x2, y2, i, count;
	XARRAY rot, tran, temptrans, thistrans;

	/* make a list of networks inside this cell that connect to the outside */
	li = newintlistobj(net_tool->cluster);
	if (li == 0) return;
	for(pi = thisni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->conarcinst->network == net)
			addtointlistobj(li, (INTBIG)pi->proto->network, TRUE);
	for(pe = thisni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		if (pe->exportproto->network == net)
			addtointlistobj(li, (INTBIG)pe->proto->network, TRUE);
	subnets = (NETWORK **)getintlistobj(li, &count);
	for(i=0; i<count; i++)
	{
		/* prepare to display the net in the subcell */
		subnet = subnets[i];
		maketrans(thisni, tran);
		makerot(thisni, rot);
		transmult(tran, rot, temptrans);
		transmult(temptrans, trans, thistrans);
		np = thisni->proto;
		for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
		{
			if (ai->network != subnet) continue;
			x1 = ai->end[0].xpos;   y1 = ai->end[0].ypos;
			x2 = ai->end[1].xpos;   y2 = ai->end[1].ypos;
			xform(x1, y1, &x1, &y1, thistrans);
			xform(x2, y2, &x2, &y2, thistrans);
			formatinfstr(infstr, x_("CELL=%s LINE=%ld,%ld,%ld,%ld\n"),
				describenodeproto(topnp), x1, x2, y1, y2);
		}
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			subnp = ni->proto;
			if (subnp->primindex != 0) continue;

			/* show network where it travels through a subcell (layout only) */
			if (subnp->cellview == el_iconview) continue;

			/* see if the network hits the instance */
			net_highlightsubnet(infstr, topnp, ni, thistrans, subnet);
		}
	}
	killintlistobj(li);
}

/*
 * Helper routine to determine whether the string "name" is a number (but it may
 * end with network index characters ":", "]", or ",".
 */
BOOLEAN net_isanumber(CHAR *name)
{
	REGISTER CHAR *pt, save;
	BOOLEAN ret;

	for(pt = name; *pt != 0; pt++)
		if (*pt == ':' || *pt == ']' || *pt == ',') break;
	if (*pt == 0) return(isanumber(name));
	save = *pt;
	*pt = 0;
	ret = isanumber(name);
	*pt = save;
	return(ret);
}

/*
 * routine to get the network attached to "ni" at its port "pp"
 */
NETWORK *getnetonport(NODEINST *ni, PORTPROTO *pp)
{
	REGISTER PORTARCINST *pi;
	REGISTER PORTEXPINST *pe;

	/* see if the port is on an arc */
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
		if (pi->proto->network == pp->network)
			return(pi->conarcinst->network);

	/* see if the port is an export */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
		if (pe->proto->network == pp->network)
			return(pe->exportproto->network);

	/* sorry, this port is not on a network */
	return(NONETWORK);
}

/*
 * routine to add subcell global to cell globals
 */
static NETWORK *net_addglobalnet(NODEPROTO *subcell, INTBIG subindex, NODEPROTO *cell)
{
	NETWORK *net;
	INTBIG index;
	INTBIG special = -1;

	if (subindex < 2)
		index = (subindex<cell->globalnetcount && cell->globalnetworks[subindex] != NONETWORK ? subindex : -1);
	else
		index = net_findglobalnet(cell, subcell->globalnetnames[subindex]);
	if (index >= 0)
	{
		net = cell->globalnetworks[index];
		if(cell->globalnetchar[index] != subcell->globalnetchar[subindex])
		{
			ttyputmsg(_("Warning: global network '%s' has different characteristics:"),
				subcell->globalnetnames[subindex]);
			ttyputmsg(_("  In cell %s, it is '%s'"), describenodeproto(cell),
				describeportbits(cell->globalnetchar[index]));
			ttyputmsg(_("  In subcell %s is '%s'"), describenodeproto(subcell),
				describeportbits(subcell->globalnetchar[subindex]));
			if (cell->globalnetchar[index] == 0)
				cell->globalnetchar[index] = subcell->globalnetchar[subindex];
		}
	} else
	{
		net = net_newnetwork(cell);
		net_setglobalnet(&special, (subindex < 2 ? subindex : -1), subcell->globalnetnames[subindex], net,
			subcell->globalnetchar[subindex], cell);
	}
		
	net->refcount++;
	return(net);
}

/*
 * routine to find index of global net of cell "np" by "name".
 * returns -1 if not found
 */
INTBIG     net_findglobalnet(NODEPROTO *np, CHAR *name)
{
	INTBIG i;

	for(i=0; i<np->globalnetcount; i++)
	{
		if(namesame(name, np->globalnetnames[i]) == 0 && np->globalnetworks[i] != NONETWORK)
			return(i);
	}
	return(-1);
}

void net_setglobalnet(INTBIG *special, INTBIG newindex, CHAR *netname, NETWORK *net,
	INTBIG characteristics, NODEPROTO *np)
{
	REGISTER INTBIG i, newsize, *newchar;
	REGISTER NETWORK **newnets;
	REGISTER CHAR **newnetnames;

	if(net->buswidth != 1)
	{
		ttyputerr(_("Warning (cell %s): signal %s with buswidth %d can't be global"),
			describenodeproto(np), describenetwork(net), net->buswidth);
		return;
	}

	/* make sure power and ground are in the list */
	if (np->globalnetcount == 0)
	{
		np->globalnetworks = (NETWORK **)emalloc(2 * (sizeof (NETWORK *)), np->lib->cluster);
		if (np->globalnetworks == 0) return;
		np->globalnetchar = (INTBIG *)emalloc(2 * SIZEOFINTBIG, np->lib->cluster);
		if (np->globalnetchar == 0) return;
		np->globalnetnames = (CHAR **)emalloc(2 * (sizeof (CHAR *)), np->lib->cluster);
		if (np->globalnetnames == 0) return;
		np->globalnetworks[GLOBALNETGROUND] = NONETWORK;
		(void)allocstring(&np->globalnetnames[GLOBALNETGROUND], _("Ground"), np->lib->cluster);
		np->globalnetchar[GLOBALNETGROUND] = GNDPORT;
		np->globalnetworks[GLOBALNETPOWER] = NONETWORK;
		(void)allocstring(&np->globalnetnames[GLOBALNETPOWER], _("Power"), np->lib->cluster);
		np->globalnetchar[GLOBALNETPOWER] = PWRPORT;
		np->globalnetcount = 2;
	}

	if (newindex < 0)
	{
		/* a global net: see if it is in the list */
		for(i=2; i<np->globalnetcount; i++)
		{
			if (*np->globalnetnames[i] == 0)
			{
				newindex = i;
				np->globalnetworks[newindex] = NONETWORK;
				continue;
			}
			if (namesame(netname, np->globalnetnames[i]) == 0)
			{
				newindex = i;
				break;
			}
		}
		if (newindex < 0)
		{
			/* expand the list */
			newsize = np->globalnetcount + 1;
			newnets = (NETWORK **)emalloc(newsize * (sizeof (NETWORK *)), np->lib->cluster);
			if (newnets == 0) return;
			newchar = (INTBIG *)emalloc(newsize * SIZEOFINTBIG, np->lib->cluster);
			if (newchar == 0) return;
			newnetnames = (CHAR **)emalloc(newsize * (sizeof (CHAR *)), np->lib->cluster);
			if (newnetnames == 0) return;
			for(i=0; i<np->globalnetcount; i++)
			{
				newnets[i] = np->globalnetworks[i];
				newchar[i] = np->globalnetchar[i];
				newnetnames[i] = np->globalnetnames[i];
			}
			newindex = np->globalnetcount;
			newnets[newindex] = NONETWORK;
			newchar[newindex] = 0;
			newnetnames[newindex] = 0;
			if (np->globalnetcount > 0)
			{
				efree((CHAR *)np->globalnetworks);
				efree((CHAR *)np->globalnetchar);
				efree((CHAR *)np->globalnetnames);
			}
			np->globalnetworks = newnets;
			np->globalnetchar = newchar;
			np->globalnetnames = newnetnames;
			np->globalnetcount++;
		}
	}
	if (newindex >= 2)
	{
		if (np->globalnetnames[newindex] != 0)
			efree((CHAR *)np->globalnetnames[newindex]);
		(void)allocstring(&np->globalnetnames[newindex], netname, np->lib->cluster);
	}

	if (*special < 0)
	{
		*special = newindex;

		/* check if all done */
		if(np->globalnetworks[newindex] == net &&
			np->globalnetchar[newindex] == characteristics &&
			net->globalnet == (INTSML)newindex)
			return;
	} else
	{
		if (*special != newindex)
		{
			if (*special < np->globalnetcount)
			{
				ttyputerr(_("Warning (cell %s): global signals %s and %s are connected"),
					describenodeproto(np), np->globalnetnames[*special], np->globalnetnames[newindex]);
			} else
			{
				ttyputerr(_("Warning (cell %s): global signal %s is connected to another"),
					describenodeproto(np), np->globalnetnames[newindex]);
			}
		}
	}
	if (np->globalnetworks[newindex] != NONETWORK)
	{
		(void)net_mergenet(np->globalnetworks[newindex], net);
	}
	np->globalnetworks[newindex] = net;
	np->globalnetchar[newindex] = characteristics;
	net->globalnet = (INTSML)newindex;
	if (net_debug)
		ttyputmsg(M_("Global network '%s' added to '%s'"), np->globalnetnames[net->globalnet],
			describenodeproto(np));
	/* don't mark above if the entire cell will be renumbered */
	if (!net_globalwork || (np->userbits&REDOCELLNET) == 0)
		net_recursivelymarkabove(np);
}

/*
 * Routine to rip the currently selected bus arc out into individual wires.
 */
void net_ripbus(void)
{
	REGISTER ARCINST *ai, *aiw;
	REGISTER NODEINST *niw, *nib, *niblast;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var, *newvar;
	REGISTER INTBIG i, count, lowend;
	CHAR **strings, **localstrings;
	REGISTER INTBIG lowx, highx, sepx, lowy, highy, sepy, stublen, lowxbus, lowybus, lambda;
	INTBIG sxw, syw, sxb, syb;

	ai = (ARCINST *)us_getobject(VARCINST, FALSE);
	if (ai == NOARCINST) return;
	if (ai->proto != sch_busarc)
	{
		ttyputerr(_("Must select a bus arc to rip it into individual signals"));
		return;
	}
	net = ai->network;
	if (net == NONETWORK)
	{
		ttyputerr(_("Bus has no network information"));
		return;
	}
	if (net->namecount == 0)
	{
		ttyputerr(_("Bus has no name"));
		return;
	}
	if (net->buswidth <= 1)
	{
		ttyputerr(_("Bus must have multiple signals"));
		return;
	}

	/* determine which bus name to use */
	var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
	if (var != NOVARIABLE && *((CHAR *)var->addr) != 0)
		count = net_evalbusname(APBUS, (CHAR *)var->addr, &strings, ai, ai->parent, 1); else
			count = net_evalbusname(APBUS, networkname(net, 0), &strings, ai, ai->parent, 1);
	if (count <= 0)
	{
		ttyputerr(_("Bus has zero-width"));
		return;
	}

	/* determine length of stub wires */
	lambda = lambdaofarc(ai);
	stublen = ai->length / 3;
	stublen = (stublen + lambda/2) / lambda * lambda;

	/* determine location of individual signals */
	if (ai->end[0].xpos == ai->end[1].xpos)
	{
		lowx = ai->end[0].xpos;
		if (lowx < (ai->parent->lowx + ai->parent->highx) / 2) lowx += stublen; else
			lowx -= stublen;
		sepx = 0;

		if (ai->end[0].ypos < ai->end[1].ypos) lowend = 0; else lowend = 1;
		lowy = (ai->end[lowend].ypos + lambda - 1) / lambda * lambda;
		highy = ai->end[1-lowend].ypos / lambda * lambda;
		if (highy-lowy >= (net->buswidth-1) * lambda)
		{
			/* signals fit on grid */
			sepy = ((highy-lowy) / ((net->buswidth-1) * lambda)) * lambda;
			lowy = ((highy - lowy) - (sepy * (net->buswidth-1))) / 2 + lowy;
			lowy = (lowy + lambda/2) / lambda * lambda;
		} else
		{
			/* signals don't fit: just make them even */
			lowy = ai->end[lowend].ypos;
			highy = ai->end[1-lowend].ypos;
			sepy = (highy-lowy) / (net->buswidth-1);
		}
		lowxbus = ai->end[0].xpos;   lowybus = lowy;
	} else if (ai->end[0].ypos == ai->end[1].ypos)
	{
		lowy = ai->end[0].ypos;
		if (lowy < (ai->parent->lowy + ai->parent->highy) / 2) lowy += stublen; else
			lowy -= stublen;
		sepy = 0;

		if (ai->end[0].xpos < ai->end[1].xpos) lowend = 0; else lowend = 1;
		lowx = (ai->end[lowend].xpos + lambda - 1) / lambda * lambda;
		highx = ai->end[1-lowend].xpos / lambda * lambda;
		if (highx-lowx >= (net->buswidth-1) * lambda)
		{
			/* signals fit on grid */
			sepx = ((highx-lowx) / ((net->buswidth-1) * lambda)) * lambda;
			lowx = ((highx - lowx) - (sepx * (net->buswidth-1))) / 2 + lowx;
			lowx = (lowx + lambda/2) / lambda * lambda;
		} else
		{
			/* signals don't fit: just make them even */
			lowx = ai->end[lowend].xpos;
			highx = ai->end[1-lowend].xpos;
			sepx = (highx-lowx) / (net->buswidth-1);
		}
		lowxbus = lowx;   lowybus = ai->end[0].ypos;
	} else
	{
		ttyputerr(_("Bus must be horizontal or vertical to be ripped out"));
		return;
	}

	/* copy names to a local array */
	localstrings = (CHAR **)emalloc(count * (sizeof (CHAR *)), net_tool->cluster);
	if (localstrings == 0) return;
	for(i=0; i<count; i++)
		(void)allocstring(&localstrings[i], strings[i], net_tool->cluster);

	/* turn off highlighting */
	us_clearhighlightcount();

	defaultnodesize(sch_wirepinprim, &sxw, &syw);
	defaultnodesize(sch_buspinprim, &sxb, &syb);
	niblast = NONODEINST;
	for(i=0; i<count; i++)
	{
		/* make the wire pin */
		niw = newnodeinst(sch_wirepinprim, lowx-sxw/2, lowx+sxw/2, lowy-syw/2, lowy+syw/2, 0, 0,
			ai->parent);
		if (niw == NONODEINST) break;
		endobjectchange((INTBIG)niw, VNODEINST);

		/* make the bus pin */
		nib = newnodeinst(sch_buspinprim, lowxbus-sxb/2, lowxbus+sxb/2, lowybus-syb/2, lowybus+syb/2,
			0, 0, ai->parent);
		if (nib == NONODEINST) break;
		endobjectchange((INTBIG)nib, VNODEINST);

		/* wire them */
		aiw = newarcinst(sch_wirearc, defaultarcwidth(sch_wirearc), ai->userbits,
			niw, sch_wirepinprim->firstportproto, lowx, lowy, nib,
				sch_buspinprim->firstportproto, lowxbus, lowybus, ai->parent);
		if (aiw == NOARCINST) break;
		newvar = setvalkey((INTBIG)aiw, VARCINST, el_arc_name_key,
			(INTBIG)localstrings[i], VSTRING|VDISPLAY);
		if (newvar != NOVARIABLE)
			defaulttextsize(4, newvar->textdescript);
		endobjectchange((INTBIG)aiw, VARCINST);

		/* wire to the bus pin */
		if (i == 0)
		{
			aiw = newarcinst(sch_busarc, defaultarcwidth(sch_busarc), ai->userbits,
				ai->end[lowend].nodeinst, ai->end[lowend].portarcinst->proto,
					ai->end[lowend].xpos, ai->end[lowend].ypos,
						nib, sch_buspinprim->firstportproto, lowxbus, lowybus, ai->parent);
		} else
		{
			/* LINTED "niblast" used in proper order */
			aiw = newarcinst(sch_busarc, defaultarcwidth(sch_busarc), ai->userbits, niblast,
				sch_buspinprim->firstportproto, lowxbus-sepx, lowybus-sepy, nib,
					sch_buspinprim->firstportproto, lowxbus, lowybus, ai->parent);
		}
		if (aiw == NOARCINST) break;
		endobjectchange((INTBIG)aiw, VARCINST);

		/* advance to the next segment */
		niblast = nib;
		lowx += sepx;      lowy += sepy;
		lowxbus += sepx;   lowybus += sepy;
	}

	/* wire up the last segment */
	aiw = newarcinst(sch_busarc, defaultarcwidth(sch_busarc), ai->userbits,
		ai->end[1-lowend].nodeinst, ai->end[1-lowend].portarcinst->proto,
			ai->end[1-lowend].xpos, ai->end[1-lowend].ypos,
				niblast, sch_buspinprim->firstportproto, lowxbus-sepx, lowybus-sepy, ai->parent);
	if (aiw == NOARCINST) return;
	if (var != NOVARIABLE && *((CHAR *)var->addr) != 0)
	{
		newvar = setvalkey((INTBIG)aiw, VARCINST, el_arc_name_key, var->addr, VSTRING|VDISPLAY);
		if (newvar != NOVARIABLE)
			defaulttextsize(4, newvar->textdescript);
	}
	endobjectchange((INTBIG)aiw, VARCINST);

	/* remove original arc */
	startobjectchange((INTBIG)ai, VARCINST);
	if (killarcinst(ai))
		ttyputerr(_("Error deleting original arc"));

	/* free memory */
	for(i=0; i<count; i++)
		efree(localstrings[i]);
	efree((CHAR *)localstrings);
}

typedef struct
{
	NODEPROTO *np;
	INTBIG     origtemp1;
	INTBIG     nodenumber;
} TEMPSTRUCT;

/*
 * Routine to name all nodes in cell "np" that do not already have node names.
 * Returns the number of nodes that were named.
 */
INTBIG net_nameallnodes(NODEPROTO *np, BOOLEAN evenpins)
{
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER CHAR *name, *match, **shortnameoverride;
	REGISTER CHAR *lastname, *thisname, *warnedname;
	CHAR *shortnames[MAXNODEFUNCTION], **namelist;
	INTBIG named, count;
	REGISTER INTBIG i, len, abbrevlen, curindex, total;
	REGISTER TEMPSTRUCT *ts, *nodelist;
	INTBIG cindex, fun, highpseudo[MAXNODEFUNCTION];
	REGISTER void *infstr;

	/* do not name nodes in "icon" cells */
	if (np->cellview == el_iconview || np->cellview == el_simsnapview) return(0);

	/* get the length of cell name abbreviations */
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_node_abbrevlen_key);
	if (var == NOVARIABLE) abbrevlen = NETDEFAULTABBREVLEN; else
		abbrevlen = var->addr;

	/* get the node function abbreviations */
	for(i=0; i<MAXNODEFUNCTION; i++)
	{
		highpseudo[i] = 0;
		shortnames[i] = nodefunctionshortname(i);
	}
	var = getvalkey((INTBIG)net_tool, VTOOL, VSTRING|VISARRAY, net_node_abbrev_key);
	if (var != NOVARIABLE)
	{
		shortnameoverride = (CHAR **)var->addr;
		len = getlength(var);
		for(i=0; i<MAXNODEFUNCTION; i++)
		{
			if (i >= len) break;
			if (*shortnameoverride[i] != 0)
				shortnames[i] = shortnameoverride[i];
		}
	}

	/* get a list of existing names in this cell */
	count = net_gathernodenames(np, &namelist);

	/* issue warning if there are duplicate names */
	if (count > 1)
	{
		esort(namelist, count, sizeof (CHAR *), sort_stringascending);
		lastname = namelist[0];
		warnedname = 0;
		for(i=1; i<count; i++)
		{
			thisname = (CHAR *)namelist[i];
			if (lastname != 0 && thisname != 0)
			{
				if (namesame(lastname, thisname) == 0)
				{
					if (warnedname == 0 || namesame(warnedname, thisname) != 0)
					{
						ttyputerr(_("***Error: cell %s has multiple nodes with name '%s'"),
							describenodeproto(np), thisname);
					}
					warnedname = thisname;
				}
			}
			lastname = thisname;
		}
	}

	/* check all nodes to see if they need to be named */
	total = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		if (ni->proto->primindex == 0) total++;
	if (total > 0)
	{
		nodelist = (TEMPSTRUCT *)emalloc(total * (sizeof (TEMPSTRUCT)), net_tool->cluster);
		if (nodelist == 0) return(0);
		total = 0;
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (ni->proto->primindex != 0) continue;
			for(i=0; i<total; i++)
				if (nodelist[i].np == ni->proto) break;
			if (i < total) continue;
			nodelist[total].np = ni->proto;
			nodelist[total].origtemp1 = ni->proto->temp1;
			nodelist[total].nodenumber = 0;
			ni->proto->temp1 = (INTBIG)&nodelist[total];
			total++;
		}
		if (total == 0) efree((CHAR *)nodelist);
	}

	named = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		fun = nodefunction(ni);
		if (evenpins == 0 && ni->firstportexpinst == NOPORTEXPINST)
		{
			if (fun == NPPIN || fun == NPART) continue;
		}

		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var != NOVARIABLE)
		{
			/* check for existing pseudo-names */
			name = (CHAR *)var->addr;
			while (*name == ' ' || *name == '\t') name++;
			if (*name != 0)
			{
				if (ni->proto->primindex == 0)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, ni->proto->protoname);
					match = returninfstr(infstr);
					len = estrlen(match);
					if (len > abbrevlen) match[abbrevlen] = 0;
					len = estrlen(match);
					if (namesamen(name, match, len) == 0)
					{
						cindex = eatoi(&name[len]);
						ts = (TEMPSTRUCT *)ni->proto->temp1;
						if (cindex > ts->nodenumber) ts->nodenumber = cindex;
					}
				} else
				{
					match = shortnames[fun];
					len = estrlen(match);
					if (namesamen(name, match, len) == 0)
					{
						cindex = eatoi(&name[len]);
						if (cindex > highpseudo[fun]) highpseudo[fun] = cindex;
					}
				}
				continue;
			}
		}
		named++;
	}

	/* see if any naming needs to be done */
	if (named != 0)
	{
		/* make names for nodes without them */
		for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			fun = nodefunction(ni);
			if (evenpins == 0 && ni->firstportexpinst == NOPORTEXPINST)
			{
				if (fun == NPPIN || fun == NPART) continue;
			}

			var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
			if (var != NOVARIABLE)
			{
				name = (CHAR *)var->addr;
				while (*name == ' ' || *name == '\t') name++;
				if (*name != 0) continue;
			}
			for(;;)
			{
				if (ni->proto->primindex == 0)
				{
					ts = (TEMPSTRUCT *)ni->proto->temp1;
					curindex = ts->nodenumber;
					curindex++;
					ts->nodenumber = curindex;
					infstr = initinfstr();
					addstringtoinfstr(infstr, ni->proto->protoname);
					match = returninfstr(infstr);
					len = estrlen(match);
					if (len > abbrevlen) match[abbrevlen] = 0;
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%ld"), match, curindex);
				} else
				{
					highpseudo[fun]++;
					infstr = initinfstr();
					formatinfstr(infstr, x_("%s%ld"), shortnames[fun], highpseudo[fun]);
				}
				name = returninfstr(infstr);

				/* see if this name is in use */
				for(i=0; i<net_nodenamelistcount; i++)
					if (namesame(name, net_nodenamelist[i]) == 0) break;
				if (i >= net_nodenamelistcount) break;
			}
			var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)name, VSTRING);
			if (var != NOVARIABLE)
				if (net_addnodename((CHAR *)var->addr)) break;
		}
	}

	/* clean up the use of "temp1" */
	for(i=0; i<total; i++)
	{
		ts = &nodelist[i];
		ts->np->temp1 = ts->origtemp1;
	}
	if (total > 0) efree((CHAR *)nodelist);

	return(named);
}

/*
 * Routine to gather all named nodes in cell "np" and fill the globals
 * "net_nodenamelistcount" and "net_nodenamelist" (also sets names "namelist"
 * and returns the count).
 */
INTBIG net_gathernodenames(NODEPROTO *np, CHAR ***namelist)
{
	REGISTER NODEINST *ni;
	REGISTER VARIABLE *var;
	REGISTER CHAR *pt, *name;
	REGISTER INTBIG count, i;
	CHAR **strings;

	net_nodenamelistcount = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		if (var == NOVARIABLE) continue;
		name = (CHAR *)var->addr;
		for(pt = name; *pt != 0; pt++) if (*pt == '[') break;
		if (*pt == 0)
		{
			if (net_addnodename(name)) break;
		} else
		{
			/* array specification: break it up and include each name separately */
			count = net_evalbusname(APBUS, name, &strings, NOARCINST, np, 0);
			for(i=0; i<count; i++)
				if (net_addnodename(strings[i])) break;
		}
	}
	*namelist = net_nodenamelist;
	return(net_nodenamelistcount);
}

/*
 * Routine to add name "name" to the global list "net_nodenamelist" which is
 * "net_nodenamelistcount" long.  Returns true on error.
 */
BOOLEAN net_addnodename(CHAR *name)
{
	REGISTER INTBIG i, newtotal;
	REGISTER CHAR **newlist;

	if (net_nodenamelistcount >= net_nodenamelisttotal)
	{
		newtotal = net_nodenamelisttotal * 2;
		if (net_nodenamelistcount >= newtotal) newtotal = net_nodenamelistcount + 50;
		newlist = (CHAR **)emalloc(newtotal * (sizeof (CHAR *)), net_tool->cluster);
		if (newlist == 0) return(TRUE);
		for(i=0; i<net_nodenamelistcount; i++)
			newlist[i] = net_nodenamelist[i];
		for(i=net_nodenamelistcount; i<newtotal; i++)
			newlist[i] = 0;
		if (net_nodenamelisttotal > 0) efree((CHAR *)net_nodenamelist);
		net_nodenamelist = newlist;
		net_nodenamelisttotal = newtotal;
	}
	if (net_nodenamelist[net_nodenamelistcount] == 0)
		(void)allocstring(&net_nodenamelist[net_nodenamelistcount], name, net_tool->cluster); else
			(void)reallocstring(&net_nodenamelist[net_nodenamelistcount], name, net_tool->cluster);
	net_nodenamelistcount++;
	return(FALSE);
}

INTBIG net_nameallnets(NODEPROTO *np)
{
	REGISTER ARCINST *ai;
	REGISTER NETWORK *net;
	REGISTER VARIABLE *var;
	REGISTER CHAR *name;
	CHAR newname[50];
	INTBIG cindex, highpseudo;
	INTBIG named;

	/* do not name arcs in "icon" cells */
	if (np->cellview == el_iconview || np->cellview == el_simsnapview) return(0);

	/* find highest numbered net in the cell */
	highpseudo = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
		if (var == NOVARIABLE) continue;

		/* check for existing pseudo-names */
		name = (CHAR *)var->addr;
		if (namesamen(name, x_("net"), 3) != 0) continue;
		cindex = eatoi(&name[3]);
		if (cindex > highpseudo) highpseudo = cindex;
	}

	/* find nets with no name and name them */
	named = 0;
	for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		net = ai->network;
		if (net == NONETWORK || net->namecount > 0) continue;

		highpseudo++;
		(void)esnprintf(newname, 50, x_("net%ld"), highpseudo);
		var = setvalkey((INTBIG)ai, VARCINST, el_arc_name_key, (INTBIG)newname, VSTRING);
		if (var != NOVARIABLE)
		defaulttextsize(4, var->textdescript);
		named++;
	}
	if (named)
		net_reevaluatecell(np);
	return(named);
}

/*
 * routine to get the two cells to be compared.  These must be the only
 * two windows on the display.  Returns true if cells cannot be found.
 */
BOOLEAN net_getcells(NODEPROTO **cell1, NODEPROTO **cell2)
{
	REGISTER WINDOWPART *w;

	/* get two windows */
	*cell1 = *cell2 = NONODEPROTO;
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if ((w->state&WINDOWTYPE) != DISPWINDOW &&
			(w->state&WINDOWTYPE) != DISP3DWINDOW) continue;
		if (*cell1 == NONODEPROTO) *cell1 = w->curnodeproto; else
		{
			if (*cell2 == NONODEPROTO)
			{
				if (w->curnodeproto != *cell1) *cell2 = w->curnodeproto;
			} else
			{
				if (w->curnodeproto != NONODEPROTO) return(TRUE);
			}
		}
	}
	if (*cell2 == NONODEPROTO) return(TRUE);
	return(FALSE);
}


#ifdef NEWRENUM
/*
 * Routine to return TRUE if network "net" has the name "netname" on it.
 */
BOOLEAN net_nethasname(NETWORK *net, NetName *netname)
{
	NetName *nn;

	if (net->namecount == 1)
	{
		nn = (NetName*)net->netnameaddr;
		if (netname == nn) return(TRUE);
	} else if (net->namecount > 1)
	{
		NetName **netnames = (NetName**)net->netnameaddr;
		for(INTBIG k=0; k<net->namecount; k++)
		{
			if (netname ==  netnames[k]) return(TRUE);
		}
	}
	return(FALSE);
}
#else
/*
 * Routine to return TRUE if network "net" has the name "name" on it.
 */
BOOLEAN net_nethasname(NETWORK *net, CHAR *name)
{
	NetName *nn;

	if (net->namecount == 1)
	{
		nn = (NetName*)net->netnameaddr;
		if (namesame(name, nn->name()) == 0) return(TRUE);
	} else if (net->namecount > 1)
	{
		NetName **netnames = (NetName**)net->netnameaddr;
		for(INTBIG k=0; k<net->namecount; k++)
		{
			if (namesame(name, netnames[k]->name()) == 0) return(TRUE);
		}
	}
	return(FALSE);
}
#endif
/*
 * Routine to get the network with name "name" in cell "cell".  Accepts networks with
 * similar names, as specified by the unification strings.
 */
NETWORK *net_getunifiednetwork(CHAR *name, NODEPROTO *cell, NETWORK *notthisnet)
{
	REGISTER NETWORK *net;
	REGISTER INTBIG i, j, k, len, unified;
	REGISTER CHAR *pt, *unifystring;

#ifdef NEWRENUM
	NetName *nn = cell->netd->findNetName(name, TRUE);
	net = nn->firstNet();
	if (net != NONETWORK && net != notthisnet) return(net);

	/* see if this name is unified */
	unified = -1;
	for(i=0; i<net_unifystringcount; i++)
	{
		unifystring = net_unifystrings[i];
		len = estrlen(unifystring);
		if (namesamen(unifystring, name, len) != 0) continue;
		if (isalnum(name[len])) continue;
		for(k=len; name[k] != 0; k++) if (name[k] == '[') break;
		if (name[k] != 0) continue;
		unified = i;
		break;
	}
#else
	/* see if this name is unified */
	unified = -1;
	for(i=0; i<net_unifystringcount; i++)
	{
		unifystring = net_unifystrings[i];
		len = estrlen(unifystring);
		if (namesamen(unifystring, name, len) != 0) continue;
		if (isalnum(name[len])) continue;
		for(k=len; name[k] != 0; k++) if (name[k] == '[') break;
		if (name[k] != 0) continue;
		unified = i;
		break;
	}

	NetName *nn = cell->netd->findNetName(name, FALSE);
	if (nn)
	{
		net = nn->firstNet();
		if (net != NONETWORK && net != notthisnet) return(net);
	}
#endif
	if (unified >= 0)
	{
		unifystring = net_unifystrings[unified];
		len = estrlen(unifystring);
		for(net = cell->firstnetwork; net != NONETWORK; net = net->nextnetwork)
		{
			if (net == notthisnet) continue;
			for(j=0; j<net->namecount; j++)
			{
				pt = networkname(net, j);
				if (namesamen(unifystring, pt, len) != 0) continue;
				if (isalnum(pt[len])) continue;
				for(k=len; pt[k] != 0; k++) if (pt[k] == '[') break;
				if (pt[k] == 0) return(net);
			}
		}
	}

	/* see if a global name is specified */
	pt = _("Global");
	k = estrlen(pt);
	if (namesamen(name, pt, k) == 0 && name[k] == '-')
	{
		for(i=0; i<cell->globalnetcount; i++)
			if (namesame(&name[k+1], cell->globalnetnames[i]) == 0)
				return(cell->globalnetworks[i]);
	}
	return(NONETWORK);
}

/*
 * Routine to cache the list of network names to unify.
 */
void net_recacheunifystrings(void)
{
	REGISTER INTBIG i;
	REGISTER CHAR *pt, *start, save, **newstrings;
	REGISTER VARIABLE *var;

	/* remove the former list of unification strings */
	for(i=0; i<net_unifystringcount; i++) efree((CHAR *)net_unifystrings[i]);
	if (net_unifystringcount > 0) efree((CHAR *)net_unifystrings);
	net_unifystringcount = 0;

	/* get the current list of strings to unify */
	var = getvalkey((INTBIG)net_tool, VTOOL, VSTRING, net_unifystringskey);
	if (var == NOVARIABLE) return;

	/* parse the list */
	pt = (CHAR *)var->addr;
	for(;;)
	{
		while (*pt == ' ' || *pt == '\t') pt++;
		if (*pt == 0) break;
		start = pt;
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		save = *pt;
		*pt = 0;

		/* make sure there is room */
		newstrings = (CHAR **)emalloc((net_unifystringcount+1) * (sizeof (CHAR *)), net_tool->cluster);
		if (newstrings == 0) return;
		for(i=0; i<net_unifystringcount; i++)
			newstrings[i] = net_unifystrings[i];
		if (net_unifystringcount > 0) efree((CHAR *)net_unifystrings);
		net_unifystrings = newstrings;
		(void)allocstring(&net_unifystrings[net_unifystringcount], start, net_tool->cluster);
		net_unifystringcount++;

		*pt = save;
	}
}

/*
 * Routine to synchronize the schematic technology with the network tools idea of
 * resistor handling.
 */
void net_tellschematics(void)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG total;

	/* see how many cells there are */
	total = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			total++;
	}

	if (asktech(sch_tech, x_("ignoring-resistor-topology")) != 0)
	{
		/* currently ignoring resistors */
		if ((net_options&NETIGNORERESISTORS) == 0)
		{
			/* make it start including resistors */
			if (total > 0)
				ttyputmsg(_("Adjusting network topology to include resistors"));
			(void)asktool(us_tool, x_("clear"));
			(void)asktech(sch_tech, x_("include-resistor-topology"));
			net_totalrenumber(NOLIBRARY);
		}
	} else
	{
		/* currently including resistors */
		if ((net_options&NETIGNORERESISTORS) != 0)
		{
			/* make it start ignoring resistors */
			if (total > 0)
				ttyputmsg(_("Adjusting network topology to ignore resistors"));
			(void)asktool(us_tool, x_("clear"));
			(void)asktech(sch_tech, x_("ignore-resistor-topology"));
			net_totalrenumber(NOLIBRARY);
		}
	}
}

/****************************** NCC CONTROL DIALOG ******************************/

/* Network: NCC Control */
static DIALOGITEM net_nccoptionsdialogitems[] =
{
 /*  1 */ {0, {488,224,512,312}, BUTTON, N_("Do NCC")},
 /*  2 */ {0, {488,8,512,72}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {364,4,380,200}, CHECK, N_("Recurse through hierarchy")},
 /*  4 */ {0, {244,4,260,220}, CHECK, N_("Check export names")},
 /*  5 */ {0, {488,96,512,201}, BUTTON, N_("Preanalysis")},
 /*  6 */ {0, {432,16,448,208}, BUTTON, N_("NCC Debugging options...")},
 /*  7 */ {0, {124,4,140,220}, CHECK, N_("Merge parallel components")},
 /*  8 */ {0, {220,4,236,220}, CHECK, N_("Ignore power and ground")},
 /*  9 */ {0, {76,4,92,217}, MESSAGE, N_("For all cells:")},
 /* 10 */ {0, {191,228,387,400}, SCROLL, x_("")},
 /* 11 */ {0, {69,220,480,221}, DIVIDELINE, x_("")},
 /* 12 */ {0, {28,312,44,352}, BUTTON, N_("Set")},
 /* 13 */ {0, {4,312,20,352}, BUTTON, N_("Set")},
 /* 14 */ {0, {28,136,44,305}, EDITTEXT, x_("")},
 /* 15 */ {0, {124,232,140,281}, RADIO, N_("Yes")},
 /* 16 */ {0, {124,284,140,329}, RADIO, N_("No")},
 /* 17 */ {0, {124,332,140,396}, RADIO, N_("Default")},
 /* 18 */ {0, {316,20,332,164}, MESSAGE, N_("Size tolerance (amt):")},
 /* 19 */ {0, {316,168,332,216}, EDITTEXT, x_("")},
 /* 20 */ {0, {68,4,69,401}, DIVIDELINE, x_("")},
 /* 21 */ {0, {76,228,92,400}, MESSAGE, N_("Individual cell overrides:")},
 /* 22 */ {0, {172,12,188,209}, BUTTON, N_("Clear NCC dates this library")},
 /* 23 */ {0, {268,4,284,220}, CHECK, N_("Check component sizes")},
 /* 24 */ {0, {4,136,20,305}, EDITTEXT, x_("")},
 /* 25 */ {0, {28,4,44,132}, MESSAGE, N_("With cell:")},
 /* 26 */ {0, {4,4,20,132}, MESSAGE, N_("Compare cell:")},
 /* 27 */ {0, {292,20,308,164}, MESSAGE, N_("Size tolerance (%):")},
 /* 28 */ {0, {292,168,308,216}, EDITTEXT, x_("")},
 /* 29 */ {0, {488,336,512,400}, BUTTON, N_("Save")},
 /* 30 */ {0, {480,4,481,400}, DIVIDELINE, x_("")},
 /* 31 */ {0, {412,240,428,388}, BUTTON, N_("Remove all overrides")},
 /* 32 */ {0, {148,232,164,281}, RADIO, N_("Yes")},
 /* 33 */ {0, {148,284,164,329}, RADIO, N_("No")},
 /* 34 */ {0, {148,332,164,396}, RADIO, N_("Default")},
 /* 35 */ {0, {148,4,164,220}, CHECK, N_("Merge series transistors")},
 /* 36 */ {0, {100,232,116,281}, RADIO, N_("Yes")},
 /* 37 */ {0, {100,284,116,329}, RADIO, N_("No")},
 /* 38 */ {0, {100,332,116,396}, RADIO, N_("Default")},
 /* 39 */ {0, {100,4,116,220}, CHECK, N_("Expand hierarchy")},
 /* 40 */ {0, {171,228,187,400}, POPUP, x_("")},
 /* 41 */ {0, {392,240,408,388}, BUTTON, N_("List all overrides")},
 /* 42 */ {0, {456,4,472,216}, CHECK, N_("Show 'NCCMatch' tags")},
 /* 43 */ {0, {4,356,20,400}, BUTTON, N_("Next")},
 /* 44 */ {0, {28,356,44,400}, BUTTON, N_("Next")},
 /* 45 */ {0, {196,12,212,209}, BUTTON, N_("Clear NCC dates all libraries")},
 /* 46 */ {0, {48,4,64,400}, MESSAGE, x_("")},
 /* 47 */ {0, {340,4,356,220}, CHECK, N_("Allow no-component nets")},
 /* 48 */ {0, {460,232,476,400}, BUTTON, N_("Remove all forced matches")},
 /* 49 */ {0, {440,232,456,400}, BUTTON, N_("List all forced matches")},
 /* 50 */ {0, {388,4,404,216}, POPUP, x_("")}
};
static DIALOG net_nccoptionsdialog = {{50,75,571,486}, N_("Network Consistency Checker"), 0, 50, net_nccoptionsdialogitems, 0, 0};

/* special items for the "NCC Control" dialog: */
#define DNCO_DONCCNOW            1		/* Do NCC now (button) */
#define DNCO_RECURSIVENCC        3		/* Do NCC recursively (check) */
#define DNCO_CHECKEXPORTNAME     4		/* Check export names in NCC (check) */
#define DNCO_DOPREANALNOW        5		/* Do NCC Preanalysis now (button) */
#define DNCO_DEBUG               6		/* Debugging settings (button) */
#define DNCO_MERGEPARALLEL       7		/* Merge parallel components in NCC (check) */
#define DNCO_IGNOREPG            8		/* Ignore power/ground in NCC (check) */
#define DNCO_CELLS              10		/* List of cells (scroll) */
#define DNCO_SETSECONDCELL      12		/* Set second cell to compare (button) */
#define DNCO_SETFIRSTCELL       13		/* Set first cell to compare (button) */
#define DNCO_SECONDCELL         14		/* Second cell to compare (edit text) */
#define DNCO_MERGEPARALLELYES   15		/* Yes to Merge parallel components (radio) */
#define DNCO_MERGEPARALLELNO    16		/* No to Merge parallel components (radio) */
#define DNCO_MERGEPARALLELDEF   17		/* Default to Merge parallel components (radio) */
#define DNCO_COMPMATCHTOLAMT    19		/* Component match tolerance (amt) (edit text) */
#define DNCO_CLEARVALIDNCCDATE  22		/* Clear valid NCC dates (button) */
#define DNCO_CHECKCOMPSIZE      23		/* Check comp. size in NCC (check) */
#define DNCO_FIRSTCELL          24		/* First cell to compare (edit text) */
#define DNCO_COMPMATCHTOLPER    28		/* Component match tolerance (%) (edit text) */
#define DNCO_SAVEOPTS           29		/* Save NCC options (button) */
#define DNCO_REMOVEOVERRIDES    31		/* Remove all overrides (button) */
#define DNCO_MERGESERIESYES     32		/* Yes to Merge series transistors (radio) */
#define DNCO_MERGESERIESNO      33		/* No to Merge series transistors (radio) */
#define DNCO_MERGESERIESDEF     34		/* Default to Merge series transistors (radio) */
#define DNCO_MERGESERIES        35		/* Merge series transistors in NCC (check) */
#define DNCO_EXPANDHIERYES      36		/* Yes to expand hierarchy (radio) */
#define DNCO_EXPANDHIERNO       37		/* No to expand hierarchy (radio) */
#define DNCO_EXPANDHIERDEF      38		/* Default to expand hierarchy (radio) */
#define DNCO_EXPANDHIERARCHY    39		/* Expand hierarchy (check) */
#define DNCO_LIBRARYSEL         40		/* Library of cells (popup) */
#define DNCO_LISTOVERRIDES      41		/* List all overrides (button) */
#define DNCO_SHOWMATCHTAGS      42		/* Show 'NCCMatch' tags (check) */
#define DNCO_NEXTFIRSTCELL      43		/* Choose next "first cell" (button) */
#define DNCO_NEXTSECONDCELL     44		/* Choose next "second cell" (button) */
#define DNCO_CLEARALLNCCDATE    45		/* Clear valid NCC dates all libraries (button) */
#define DNCO_OFFSCREENWARN      46		/* Warning if cells are not displayed (message) */
#define DNCO_ALLOWNOCOMPNETS    47		/* Allow no-component networks (check) */
#define DNCO_REMOVEFORCEDMATCH  48		/* Remove all forced-matches (button) */
#define DNCO_LISTFORCEDMATCH    49		/* List all forced-matches (button) */
#define DNCO_AUTORESISTORS      50		/* Automatically check for resistor settings (popup) */

#define NUMOVERRIDES   3

static struct
{
	CHAR *description;
	int   yes, no, def;
	int   overridebit, yesbit, nobit;
} net_nccoverridebuttons[NUMOVERRIDES] =
{
	{x_("hierarchy expansion"),
		DNCO_EXPANDHIERYES,        DNCO_EXPANDHIERNO,      DNCO_EXPANDHIERDEF,
		NCCHIERARCHICALOVER,       NCCHIERARCHICAL,        0},
	{x_("parallel component merging"),
		DNCO_MERGEPARALLELYES,     DNCO_MERGEPARALLELNO,   DNCO_MERGEPARALLELDEF,
		NCCNOMERGEPARALLELOVER,    0,                      NCCNOMERGEPARALLEL},
	{x_("series transistor merging"),
		DNCO_MERGESERIESYES,       DNCO_MERGESERIESNO,     DNCO_MERGESERIESDEF,
		NCCMERGESERIESOVER,        NCCMERGESERIES,         0}
};

static CHAR     **net_librarylist;
static LIBRARY   *net_library;
static NODEPROTO *net_posnodeprotos;

BOOLEAN net_topofcells(CHAR **c)
{
	Q_UNUSED( c );
	net_posnodeprotos = net_library->firstnodeproto;
	return(TRUE);
}
CHAR *net_nextcells(void)
{
	REGISTER NODEPROTO *np;

	while (net_posnodeprotos != NONODEPROTO)
	{
		np = net_posnodeprotos;
		net_posnodeprotos = net_posnodeprotos->nextnodeproto;
		if (np->cellview == el_iconview) continue;
		return(nldescribenodeproto(np));
	}
	return(0);
}
CHAR *net_nextcellswithfunction(void)
{
	REGISTER NODEPROTO *np;
	REGISTER void *infstr;

	while (net_posnodeprotos != NONODEPROTO)
	{
		np = net_posnodeprotos;
		net_posnodeprotos = net_posnodeprotos->nextnodeproto;
		if (np->cellview == el_iconview) continue;
		if (np->temp1 == 0) return(nldescribenodeproto(np));
		infstr = initinfstr();
		formatinfstr(infstr, "%s (%s)", nldescribenodeproto(np), (CHAR *)np->temp1);
		return(returninfstr(infstr));
	}
	return(0);
}

void net_nccoptionsdlog(void)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG itemHit, i, options, oldoptions, clearvalidnccdates, tolerance,
		toleranceamt, libcount, oldcelloptions, clearallnccdates;
	REGISTER UINTBIG wanted;
	REGISTER LIBRARY *lib;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN checkoffscreen;
	REGISTER ARCINST *ai;
	REGISTER WINDOWPART *win, *winfirst, *winsecond;
	CHAR buf[10], *paramstart[5], *newlang[5];
	REGISTER NODEPROTO *np;
	NODEPROTO *cell1, *cell2;
	static NODEPROTO *firstcell = NONODEPROTO;
	static NODEPROTO *secondcell = NONODEPROTO;
	static LIBRARY *firstlibrary, *secondlibrary;
	REGISTER void *dia;
	static CHAR *resistorOptions[3] = {N_("No resistor adjustments"),
		N_("Include resistors"), N_("Exclude resistors")};

	dia = DiaInitDialog(&net_nccoptionsdialog);
	if (dia == 0) return;
	DiaUnDimItem(dia, DNCO_CLEARVALIDNCCDATE);
	DiaUnDimItem(dia, DNCO_CLEARALLNCCDATE);

	if (!net_getcells(&cell1, &cell2))
	{
		firstcell = cell1;   firstlibrary = cell1->lib;
		secondcell = cell2;  secondlibrary = cell2->lib;
	}
	winfirst = winsecond = NOWINDOWPART;
	if (firstcell != NONODEPROTO)
	{
		/* validate this cell */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			if (lib == firstlibrary) break;
		if (lib == NOLIBRARY) firstcell = NONODEPROTO;
		if (firstcell != NONODEPROTO)
		{
			for(np = firstlibrary->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (np == firstcell) break;
			if (np == NONODEPROTO) firstcell = NONODEPROTO;
		}
		if (firstcell != NONODEPROTO)
		{
			DiaSetText(dia, DNCO_FIRSTCELL, describenodeproto(firstcell));
			for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
				if (win->curnodeproto == firstcell) break;
			if (win != NOWINDOWPART) winfirst = win;
		}
	}
	if (secondcell != NONODEPROTO)
	{
		/* validate this cell */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			if (lib == secondlibrary) break;
		if (lib == NOLIBRARY) secondcell = NONODEPROTO;
		if (secondcell != NONODEPROTO)
		{
			for(np = secondlibrary->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				if (np == secondcell) break;
			if (np == NONODEPROTO) secondcell = NONODEPROTO;
		}
		if (secondcell != NONODEPROTO)
		{
			DiaSetText(dia, DNCO_SECONDCELL, describenodeproto(secondcell));
			for(win = el_topwindowpart; win != NOWINDOWPART; win = win->nextwindowpart)
				if (win->curnodeproto == secondcell) break;
			if (win != NOWINDOWPART) winsecond = win;
		}
	}
	checkoffscreen = TRUE;

	/* list the libraries */
	libcount = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if ((lib->userbits&HIDDENLIBRARY) == 0) libcount++;
	net_librarylist = (CHAR **)emalloc(libcount * (sizeof (CHAR *)), el_tempcluster);
	libcount = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if ((lib->userbits&HIDDENLIBRARY) == 0)
			net_librarylist[libcount++] = lib->libname;
	esort(net_librarylist, libcount, sizeof (CHAR *), sort_stringascending);
	DiaSetPopup(dia, DNCO_LIBRARYSEL, libcount, net_librarylist);
	for(i=0; i<libcount; i++)
		if (namesame(net_librarylist[i], el_curlib->libname) == 0) break;
	if (i < libcount) DiaSetPopupEntry(dia, DNCO_LIBRARYSEL, i);

	net_library = el_curlib;
	DiaInitTextDialog(dia, DNCO_CELLS, net_topofcells, net_nextcells, DiaNullDlogDone, 0,
		SCSELMOUSE|SCSELKEY|SCREPORT);
	(void)us_setscrolltocurrentcell(DNCO_CELLS, FALSE, TRUE, FALSE, FALSE, dia);
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_optionskey);
	if (var == NOVARIABLE) options = 0; else options = var->addr;
	oldoptions = options;
	if ((options&NCCRECURSE) != 0) DiaSetControl(dia, DNCO_RECURSIVENCC, 1);
	if ((options&NCCHIERARCHICAL) != 0) DiaSetControl(dia, DNCO_EXPANDHIERARCHY, 1);
	if ((options&NCCIGNOREPWRGND) != 0) DiaSetControl(dia, DNCO_IGNOREPG, 1);
	if ((options&NCCNOMERGEPARALLEL) == 0) DiaSetControl(dia, DNCO_MERGEPARALLEL, 1);
	if ((options&NCCMERGESERIES) != 0) DiaSetControl(dia, DNCO_MERGESERIES, 1);
	if ((options&NCCCHECKEXPORTNAMES) != 0) DiaSetControl(dia, DNCO_CHECKEXPORTNAME, 1);
	if ((options&NCCCHECKSIZE) != 0) DiaSetControl(dia, DNCO_CHECKCOMPSIZE, 1);
	if ((options&NCCHIDEMATCHTAGS) == 0) DiaSetControl(dia, DNCO_SHOWMATCHTAGS, 1);
	if ((options&NCCINCLUDENOCOMPNETS) != 0) DiaSetControl(dia, DNCO_ALLOWNOCOMPNETS, 1);

	for(i=0; i<3; i++) newlang[i] = TRANSLATE(resistorOptions[i]);
	DiaSetPopup(dia, DNCO_AUTORESISTORS, 3, newlang);
	switch (options&NCCRESISTINCLUSION)
	{
		case NCCRESISTLEAVE:   DiaSetPopupEntry(dia, DNCO_AUTORESISTORS, 0); break;
		case NCCRESISTINCLUDE: DiaSetPopupEntry(dia, DNCO_AUTORESISTORS, 1); break;
		case NCCRESISTEXCLUDE: DiaSetPopupEntry(dia, DNCO_AUTORESISTORS, 2); break;
	}

	var = getvalkey((INTBIG)net_tool, VTOOL, -1, net_ncc_comptolerancekey);
	tolerance = 0;
	if (var != NOVARIABLE)
	{
		if ((var->type&VTYPE) == VINTEGER) tolerance = var->addr * WHOLE; else
			if ((var->type&VTYPE) == VFRACT) tolerance = var->addr;
	}
	estrcpy(buf, frtoa(tolerance));
	DiaSetText(dia, DNCO_COMPMATCHTOLPER, buf);
	var = getvalkey((INTBIG)net_tool, VTOOL, VINTEGER, net_ncc_comptoleranceamtkey);
	if (var == NOVARIABLE) toleranceamt = 0; else toleranceamt = var->addr;
	DiaSetText(dia, DNCO_COMPMATCHTOLAMT, frtoa(toleranceamt));

	/* cache individual cell overrides */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, net_ncc_optionskey);
		if (var == NOVARIABLE) np->temp1 = 0; else np->temp1 = var->addr;
	}

	net_setnccoverrides(dia);
	clearvalidnccdates = clearallnccdates = 0;

	/* loop until done */
	for(;;)
	{
		/* check and warn if cells are offscreen */
		if (checkoffscreen)
		{
			if (winfirst == NOWINDOWPART || winsecond == NOWINDOWPART)
			{
				DiaSetText(dia, DNCO_OFFSCREENWARN, x_("!!! WARNING: These cells are not on the screen !!!"));
			} else
			{
				DiaSetText(dia, DNCO_OFFSCREENWARN, x_(""));
			}
			checkoffscreen = FALSE;
		}

		itemHit = DiaNextHit(dia);
		if (itemHit == DNCO_SAVEOPTS || itemHit == CANCEL) break;
		if (itemHit == DNCO_DONCCNOW || itemHit == DNCO_DOPREANALNOW)
		{
			if (firstcell != NONODEPROTO && secondcell != NONODEPROTO) break;
			DiaMessageInDialog(_("Must first specify two cells to compare"));
			continue;
		}
		if (itemHit == DNCO_MERGEPARALLEL || itemHit == DNCO_IGNOREPG ||
			itemHit == DNCO_CHECKEXPORTNAME || itemHit == DNCO_CHECKCOMPSIZE ||
			itemHit == DNCO_MERGESERIES || itemHit == DNCO_EXPANDHIERARCHY ||
			itemHit == DNCO_ALLOWNOCOMPNETS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			clearvalidnccdates = clearallnccdates = 1;
			DiaDimItem(dia, DNCO_CLEARVALIDNCCDATE);
			DiaDimItem(dia, DNCO_CLEARALLNCCDATE);
			continue;
		}
		if (itemHit == DNCO_SHOWMATCHTAGS || itemHit == DNCO_RECURSIVENCC )
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DNCO_COMPMATCHTOLAMT || itemHit == DNCO_COMPMATCHTOLPER)
		{
			if (itemHit == DNCO_COMPMATCHTOLAMT)
			{
				i = atofr(DiaGetText(dia, DNCO_COMPMATCHTOLAMT));
				if (i == toleranceamt) continue;
			} else
			{
				i = atofr(DiaGetText(dia, DNCO_COMPMATCHTOLPER));
				if (i == tolerance) continue;
			}
			clearvalidnccdates = clearallnccdates = 1;
			DiaDimItem(dia, DNCO_CLEARVALIDNCCDATE);
			DiaDimItem(dia, DNCO_CLEARALLNCCDATE);
			continue;
		}
		if (itemHit == DNCO_SETFIRSTCELL)
		{
			i = us_cellselect(_("First cell to NCC"), paramstart, 0);
			if (i == 0) continue;
			np = getnodeproto(paramstart[0]);
			if (np == NONODEPROTO) continue;
			firstcell = np;
			DiaSetText(dia, DNCO_FIRSTCELL, describenodeproto(np));
			if (firstcell != NONODEPROTO)
			{
				firstlibrary = firstcell->lib;
				for(winfirst = el_topwindowpart; winfirst != NOWINDOWPART; winfirst = winfirst->nextwindowpart)
					if (winfirst->curnodeproto == firstcell) break;
			}
			checkoffscreen = TRUE;
			continue;
		}
		if (itemHit == DNCO_FIRSTCELL)
		{
			firstcell = getnodeproto(DiaGetText(dia, DNCO_FIRSTCELL));
			if (firstcell != NONODEPROTO)
			{
				firstlibrary = firstcell->lib;
				for(winfirst = el_topwindowpart; winfirst != NOWINDOWPART; winfirst = winfirst->nextwindowpart)
					if (winfirst->curnodeproto == firstcell) break;
			}
			checkoffscreen = TRUE;
			continue;
		}
		if (itemHit == DNCO_NEXTFIRSTCELL)
		{
			/* cycle through the open windows, choosing the next "first cell" */
			if (winfirst == NOWINDOWPART) winfirst = el_topwindowpart; else
			{
				winfirst = winfirst->nextwindowpart;
				if (winfirst == NOWINDOWPART) winfirst = el_topwindowpart;
			}
			if (winfirst != NOWINDOWPART && winfirst->curnodeproto != NONODEPROTO)
			{
				firstcell = winfirst->curnodeproto;
				DiaSetText(dia, DNCO_FIRSTCELL, describenodeproto(firstcell));
				if (firstcell != NONODEPROTO) firstlibrary = firstcell->lib;
			}
			checkoffscreen = TRUE;
			continue;
		}
		if (itemHit == DNCO_SETSECONDCELL)
		{
			i = us_cellselect(_("Second cell to NCC"), paramstart, 0);
			if (i == 0) continue;
			np = getnodeproto(paramstart[0]);
			if (np == NONODEPROTO) continue;
			secondcell = np;
			DiaSetText(dia, DNCO_SECONDCELL, describenodeproto(np));
			if (secondcell != NONODEPROTO)
			{
				secondlibrary = secondcell->lib;
				for(winsecond = el_topwindowpart; winsecond != NOWINDOWPART; winsecond = winsecond->nextwindowpart)
					if (winsecond->curnodeproto == secondcell) break;
			}
			checkoffscreen = TRUE;
			continue;
		}
		if (itemHit == DNCO_SECONDCELL)
		{
			secondcell = getnodeproto(DiaGetText(dia, DNCO_SECONDCELL));
			if (secondcell != NONODEPROTO)
			{
				secondlibrary = secondcell->lib;
				for(winsecond = el_topwindowpart; winsecond != NOWINDOWPART; winsecond = winsecond->nextwindowpart)
					if (winsecond->curnodeproto == secondcell) break;
			}
			checkoffscreen = TRUE;
			continue;
		}
		if (itemHit == DNCO_NEXTSECONDCELL)
		{
			/* cycle through the open windows, choosing the next "first cell" */
			if (winsecond == NOWINDOWPART) winsecond = el_topwindowpart; else
			{
				winsecond = winsecond->nextwindowpart;
				if (winsecond == NOWINDOWPART) winsecond = el_topwindowpart;
			}
			if (winsecond != NOWINDOWPART && winsecond->curnodeproto != NONODEPROTO)
			{
				secondcell = winsecond->curnodeproto;
				DiaSetText(dia, DNCO_SECONDCELL, describenodeproto(secondcell));
				if (secondcell != NONODEPROTO) secondlibrary = secondcell->lib;
			}
			checkoffscreen = TRUE;
			continue;
		}
		if (itemHit == DNCO_LISTOVERRIDES)
		{
			net_listnccoverrides(TRUE);
			continue;
		}
		if (itemHit == DNCO_LISTFORCEDMATCH)
		{
			net_listnccforcedmatches();
			continue;
		}
		if (itemHit == DNCO_REMOVEOVERRIDES)
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
					np->temp1 = 0;
			}
			continue;
		}
		if (itemHit == DNCO_REMOVEFORCEDMATCH)
		{
			net_removenccforcedmatches();
			DiaDimItem(dia, DNCO_REMOVEFORCEDMATCH);
			continue;
		}
		if (itemHit == DNCO_DEBUG)
		{
			options = net_nccdebuggingdlog(options);
			continue;
		}
		for(i=0; i<NUMOVERRIDES; i++)
		{
			if (itemHit == net_nccoverridebuttons[i].yes ||
				itemHit == net_nccoverridebuttons[i].no ||
				itemHit == net_nccoverridebuttons[i].def)
			{
				DiaSetControl(dia, net_nccoverridebuttons[i].yes, 0);
				DiaSetControl(dia, net_nccoverridebuttons[i].no, 0);
				DiaSetControl(dia, net_nccoverridebuttons[i].def, 0);
				DiaSetControl(dia, itemHit, 1);

				np = net_nccdlgselectedcell(dia, DNCO_LIBRARYSEL, DNCO_CELLS);
				if (np == NONODEPROTO) break;
				np->temp1 &= ~(net_nccoverridebuttons[i].overridebit |
					net_nccoverridebuttons[i].yesbit | net_nccoverridebuttons[i].nobit);
				if (itemHit != net_nccoverridebuttons[i].def)
				{
					np->temp1 |= net_nccoverridebuttons[i].overridebit;
					if (itemHit == net_nccoverridebuttons[i].yes)
					{
						np->temp1 |= net_nccoverridebuttons[i].yesbit;
					} else
					{
						np->temp1 |= net_nccoverridebuttons[i].nobit;
					}
				}
				break;
			}
		}
		if (itemHit == DNCO_CELLS)
		{
			net_setnccoverrides(dia);
			continue;
		}
		if (itemHit == DNCO_CLEARVALIDNCCDATE)
		{
			/* clear valid NCC dates this library */
			clearvalidnccdates = 1;
			DiaDimItem(dia, DNCO_CLEARVALIDNCCDATE);
			continue;
		}
		if (itemHit == DNCO_CLEARALLNCCDATE)
		{
			/* clear all valid NCC dates */
			clearallnccdates = 1;
			DiaDimItem(dia, DNCO_CLEARVALIDNCCDATE);
			DiaDimItem(dia, DNCO_CLEARALLNCCDATE);
			continue;
		}
		if (itemHit == DNCO_LIBRARYSEL)
		{
			i = DiaGetPopupEntry(dia, DNCO_LIBRARYSEL);
			lib = getlibrary(net_librarylist[i]);
			if (lib == NOLIBRARY) continue;
			net_library = lib;
			DiaLoadTextDialog(dia, DNCO_CELLS, net_topofcells, net_nextcells, DiaNullDlogDone, 0);
			net_setnccoverrides(dia);
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		options = options & ~(NCCRECURSE | NCCHIERARCHICAL |
			NCCNOMERGEPARALLEL | NCCMERGESERIES |
			NCCIGNOREPWRGND |
			NCCCHECKEXPORTNAMES | NCCCHECKSIZE |
			NCCHIDEMATCHTAGS | NCCINCLUDENOCOMPNETS |
			NCCRESISTINCLUSION);
		if (DiaGetControl(dia, DNCO_RECURSIVENCC) != 0) options |= NCCRECURSE;
		if (DiaGetControl(dia, DNCO_EXPANDHIERARCHY) != 0) options |= NCCHIERARCHICAL;
		if (DiaGetControl(dia, DNCO_MERGEPARALLEL) == 0) options |= NCCNOMERGEPARALLEL;
		if (DiaGetControl(dia, DNCO_MERGESERIES) != 0) options |= NCCMERGESERIES;
		if (DiaGetControl(dia, DNCO_IGNOREPG) != 0) options |= NCCIGNOREPWRGND;
		if (DiaGetControl(dia, DNCO_CHECKEXPORTNAME) != 0) options |= NCCCHECKEXPORTNAMES;
		if (DiaGetControl(dia, DNCO_CHECKCOMPSIZE) != 0) options |= NCCCHECKSIZE;
		if (DiaGetControl(dia, DNCO_SHOWMATCHTAGS) == 0) options |= NCCHIDEMATCHTAGS;
		if (DiaGetControl(dia, DNCO_ALLOWNOCOMPNETS) != 0) options |= NCCINCLUDENOCOMPNETS;
		i = DiaGetPopupEntry(dia, DNCO_AUTORESISTORS);
		switch (i)
		{
			case 0: options |= NCCRESISTLEAVE;     break;
			case 1: options |= NCCRESISTINCLUDE;   break;
			case 2: options |= NCCRESISTEXCLUDE;   break;
		}
		if (options != oldoptions)
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_optionskey, options, VINTEGER);

		i = atofr(DiaGetText(dia, DNCO_COMPMATCHTOLPER));
		if (i != tolerance)
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_comptolerancekey, i, VFRACT);
		i = atofr(DiaGetText(dia, DNCO_COMPMATCHTOLAMT));
		if (i != toleranceamt)
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_ncc_comptoleranceamtkey, i, VINTEGER);

		/* make changes to cell overrides */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, net_ncc_optionskey);
			if (var == NOVARIABLE) oldcelloptions = 0; else oldcelloptions = var->addr;
			if (oldcelloptions == np->temp1) continue;
			if (np->temp1 == 0) (void)delvalkey((INTBIG)np, VNODEPROTO, net_ncc_optionskey); else
				setvalkey((INTBIG)np, VNODEPROTO, net_ncc_optionskey, np->temp1, VINTEGER);
		}

		/* clear valid NCC dates if requested */
		if (clearvalidnccdates != 0 || clearallnccdates != 0)
		{
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				if (clearallnccdates == 0 && lib != el_curlib) continue;
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					net_nccremovematches(np);
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
						if (var != NOVARIABLE)
						{
							if (namesamen((CHAR *)var->addr, x_("NCCmatch"), 8) == 0)
							{
								startobjectchange((INTBIG)ni, VNODEINST);
								(void)delvalkey((INTBIG)ni, VNODEINST, el_node_name_key);
								endobjectchange((INTBIG)ni, VNODEINST);
							}
						}
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_matchkey);
						if (var != NOVARIABLE)
						{
							startobjectchange((INTBIG)ni, VNODEINST);
							(void)delvalkey((INTBIG)ni, VNODEINST, net_ncc_matchkey);
							endobjectchange((INTBIG)ni, VNODEINST);
						}	
					}
					for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					{
						var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
						if (var != NOVARIABLE)
						{
							if (namesamen((CHAR *)var->addr, x_("NCCmatch"), 8) == 0)
							{
								startobjectchange((INTBIG)ai, VARCINST);
								(void)delvalkey((INTBIG)ai, VARCINST, el_arc_name_key);
								endobjectchange((INTBIG)ai, VARCINST);
							}
						}
						var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_matchkey);
						if (var != NOVARIABLE)
						{
							startobjectchange((INTBIG)ai, VARCINST);
							(void)delvalkey((INTBIG)ai, VARCINST, net_ncc_matchkey);
							endobjectchange((INTBIG)ai, VARCINST);
						}	
					}
				}
			}
			net_removeassociations();
		} else if ((options&NCCHIDEMATCHTAGS) != (oldoptions&NCCHIDEMATCHTAGS))
		{
			/* change visibility of "nccmatch" tags */
			if ((options&NCCHIDEMATCHTAGS) == 0) wanted = VDISPLAY; else wanted = 0;
			for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			{
				for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				{
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
						if (var != NOVARIABLE)
						{
							if (namesamen((CHAR *)var->addr, x_("NCCmatch"), 8) == 0)
							{
								if ((var->type&VDISPLAY) != wanted)
								{
									startobjectchange((INTBIG)ni, VNODEINST);
									var->type = (var->type & ~VDISPLAY) | wanted;
									endobjectchange((INTBIG)ni, VNODEINST);
								}
							}
						}
						var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_matchkey);
						if (var != NOVARIABLE && (var->type&VDISPLAY) != wanted)
						{
							startobjectchange((INTBIG)ni, VNODEINST);
							var->type = (var->type & ~VDISPLAY) | wanted;
							endobjectchange((INTBIG)ni, VNODEINST);
						}	
					}
					for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
					{
						var = getvalkey((INTBIG)ai, VARCINST, VSTRING, el_arc_name_key);
						if (var != NOVARIABLE)
						{
							if (namesamen((CHAR *)var->addr, x_("NCCmatch"), 8) == 0)
							{
								if ((var->type&VDISPLAY) != wanted)
								{
									startobjectchange((INTBIG)ai, VARCINST);
									var->type = (var->type & ~VDISPLAY) | wanted;
									endobjectchange((INTBIG)ai, VARCINST);
								}
							}
						}
						var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_matchkey);
						if (var != NOVARIABLE && (var->type&VDISPLAY) != wanted)
						{
							startobjectchange((INTBIG)ai, VARCINST);
							var->type = (var->type & ~VDISPLAY) | wanted;
							endobjectchange((INTBIG)ai, VARCINST);
						}	
					}
				}
			}
		}
	}
	DiaDoneDialog(dia);
	efree((CHAR *)net_librarylist);
	if (itemHit == DNCO_DONCCNOW)
	{
		net_compare(FALSE, TRUE, firstcell, secondcell);
	} else if (itemHit == DNCO_DOPREANALNOW)
	{
		net_compare(TRUE, TRUE, firstcell, secondcell);
	}
}

void net_setnccoverrides(void *dia)
{
	REGISTER INTBIG i, override;
	REGISTER NODEPROTO *np;

	np = net_nccdlgselectedcell(dia, DNCO_LIBRARYSEL, DNCO_CELLS);
	if (np == NONODEPROTO) return;

	override = np->temp1;
	for(i=0; i<NUMOVERRIDES; i++)
	{
		DiaSetControl(dia, net_nccoverridebuttons[i].yes, 0);
		DiaSetControl(dia, net_nccoverridebuttons[i].no, 0);
		DiaSetControl(dia, net_nccoverridebuttons[i].def, 0);
		if ((override&net_nccoverridebuttons[i].overridebit) == 0)
		{
			DiaSetControl(dia, net_nccoverridebuttons[i].def, 1);
		} else
		{
			if (net_nccoverridebuttons[i].yesbit != 0)
			{
				if ((override&net_nccoverridebuttons[i].yesbit) != 0)
				{
					DiaSetControl(dia, net_nccoverridebuttons[i].yes, 1);
				} else
				{
					DiaSetControl(dia, net_nccoverridebuttons[i].no, 1);
				}
			}
			if (net_nccoverridebuttons[i].nobit != 0)
			{
				if ((override&net_nccoverridebuttons[i].nobit) != 0)
				{
					DiaSetControl(dia, net_nccoverridebuttons[i].no, 1);
				} else
				{
					DiaSetControl(dia, net_nccoverridebuttons[i].yes, 1);
				}
			}
		}
	}
}

/*
 * Routine to list NCC overrides
 */
void net_listnccoverrides(BOOLEAN usetemp1)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG i, overridecount, bits;
	REGISTER CHAR *state;
	REGISTER void *infstr;
	REGISTER VARIABLE *var;

	overridecount = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (usetemp1) bits = np->temp1; else
			{
				var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, net_ncc_optionskey);
				if (var == NOVARIABLE) bits = 0; else bits = var->addr;
			}
			for(i=0; i<NUMOVERRIDES; i++)
			{
				if ((bits&net_nccoverridebuttons[i].overridebit) == 0)
					continue;
				if (net_nccoverridebuttons[i].yesbit != 0)
				{
					if ((bits&net_nccoverridebuttons[i].yesbit) != 0)
						state = _("on"); else
							state = _("off");
				} else
				{
					if ((bits&net_nccoverridebuttons[i].nobit) != 0)
						state = _("off"); else
							state = _("on");
				}
				if (overridecount == 0)
					ttyputmsg(_("- Current cell overrides:"));
				infstr = initinfstr();
				formatinfstr(infstr, _("    Cell %s forces %s %s"),
					describenodeproto(np), net_nccoverridebuttons[i].description,
						state);
				if ((bits & (NCCHIERARCHICAL|NCCHIERARCHICALOVER)) == NCCHIERARCHICALOVER)
				{
					/* forces hierarchical expansion off: see if there is a function */
					var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, net_ncc_function_key);
					if (var != NOVARIABLE)
						formatinfstr(infstr, _(" (and has function %s)"), (CHAR *)var->addr);
				}
				ttyputmsg(x_("%s"), returninfstr(infstr));
				overridecount++;
			}
		}
	}
	if (overridecount == 0) ttyputmsg(_("- No cell overrides"));
}

/*
 * Routine to list all forced matches in all libraries.
 */
void net_listnccforcedmatches(void)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;
	REGISTER INTBIG count;

	count = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_forcedassociationkey);
				if (var != NOVARIABLE)
				{
					ttyputmsg(_("Cell %s, node %s has forced match '%s'"),
						describenodeproto(np), describenodeinst(ni), (CHAR *)var->addr);
					count++;
				}
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_forcedassociationkey);
				if (var != NOVARIABLE)
				{
					ttyputmsg(_("Cell %s, arc %s has forced match '%s'"),
						describenodeproto(np), describearcinst(ai), (CHAR *)var->addr);
					count++;
				}
			}
		}
	}
	if (count == 0) ttyputmsg(_("There are no forced matches"));
}

/*
 * Routine to remove all forced matches in all libraries.
 */
void net_removenccforcedmatches(void)
{
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER VARIABLE *var;

	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, net_ncc_forcedassociationkey);
				if (var != NOVARIABLE)
					delvalkey((INTBIG)ni, VNODEINST, net_ncc_forcedassociationkey);
			}
			for(ai = np->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
			{
				var = getvalkey((INTBIG)ai, VARCINST, VSTRING, net_ncc_forcedassociationkey);
				if (var != NOVARIABLE)
					delvalkey((INTBIG)ai, VARCINST, net_ncc_forcedassociationkey);
			}
		}
	}
}

/*
 * Routine to return the cell that is currently selected in a
 * dialog.  The item with the popup of libraries is "liblist" and the item
 * with the list of cells is "celllist".  Returns NONODEPROTO if none can be determined.
 */
NODEPROTO *net_nccdlgselectedcell(void *dia, INTBIG liblist, INTBIG celllist)
{
	REGISTER INTBIG which;
	REGISTER CHAR *pt;
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	REGISTER void *infstr;

	/* get the library */
	which = DiaGetPopupEntry(dia, liblist);
	if (which < 0) return(NONODEPROTO);
	lib = getlibrary(net_librarylist[which]);
	if (lib == NOLIBRARY) return(NONODEPROTO);

	which = DiaGetCurLine(dia, celllist);
	if (which < 0) return(NONODEPROTO);
	pt = DiaGetScrollLine(dia, celllist, which);
	if (*pt == 0) return(NONODEPROTO);
	if (lib != el_curlib)
	{
		infstr = initinfstr();
		formatinfstr(infstr, x_("%s:%s"), lib->libname, pt);
		pt = returninfstr(infstr);
	}
	np = getnodeproto(pt);
	return(np);
}

/****************************** NCC FUNCTION DIALOG ******************************/

/* NCC Cell Function */
static DIALOGITEM net_nccfundialogitems[] =
{
 /*  1 */ {0, {308,184,332,264}, BUTTON, N_("OK")},
 /*  2 */ {0, {308,24,332,104}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,116,20,288}, POPUP, x_("")},
 /*  4 */ {0, {4,4,20,112}, MESSAGE, N_("Library:")},
 /*  5 */ {0, {24,4,256,288}, SCROLL, x_("")},
 /*  6 */ {0, {260,116,276,288}, POPUP, x_("")},
 /*  7 */ {0, {260,4,276,112}, MESSAGE, N_("Function:")},
 /*  8 */ {0, {280,4,296,288}, CHECK, N_("Override Expansion on Cells with Functions")}
};
static DIALOG net_nccfundialog = {{75,75,416,373}, N_("Assign primitive functions to cells"), 0, 8, net_nccfundialogitems, 0, 0};

/* special items for the "NCC Debugging" dialog: */
#define DNCF_LIBLIST      3		/* List of libraries (popup) */
#define DNCF_CELLLIST     5		/* List of cells (scroll) */
#define DNCF_FUNCLIST     6		/* List of functions (popup) */
#define DNCF_OVERRIDE     8		/* Override cell expansion (check) */

void net_ncccellfunctiondlog(void)
{
	REGISTER void *dia, *infstr;
	REGISTER INTBIG itemHit, libcount, i;
	REGISTER CHAR **functionlist;
	REGISTER BOOLEAN cellselchanged, s, g, d, b;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER LIBRARY *lib;
	REGISTER VARIABLE *var;

	dia = DiaInitDialog(&net_nccfundialog);
	if (dia == 0) return;
	DiaSetControl(dia, DNCF_OVERRIDE, 1);

	/* list the libraries */
	libcount = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
	{
		if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
		libcount++;
		for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, net_ncc_function_key);
			if (var == NOVARIABLE) np->temp1 = 0; else
				np->temp1 = var->addr;
		}
	}
	net_librarylist = (CHAR **)emalloc(libcount * (sizeof (CHAR *)), el_tempcluster);
	libcount = 0;
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if ((lib->userbits&HIDDENLIBRARY) == 0)
			net_librarylist[libcount++] = lib->libname;
	esort(net_librarylist, libcount, sizeof (CHAR *), sort_stringascending);
	DiaSetPopup(dia, DNCF_LIBLIST, libcount, net_librarylist);
	for(i=0; i<libcount; i++)
		if (namesame(net_librarylist[i], el_curlib->libname) == 0) break;
	if (i < libcount) DiaSetPopupEntry(dia, DNCF_LIBLIST, i);
	net_library = el_curlib;
	DiaInitTextDialog(dia, DNCF_CELLLIST, net_topofcells, net_nextcellswithfunction, DiaNullDlogDone, 0,
		SCSELMOUSE|SCSELKEY|SCREPORT);
	(void)us_setscrolltocurrentcell(DNCF_CELLLIST, FALSE, TRUE, FALSE, FALSE, dia);

	/* load the function names */
	functionlist = (CHAR **)emalloc(MAXNODEFUNCTION * (sizeof (CHAR *)), el_tempcluster);
	for(i=0; i<MAXNODEFUNCTION; i++)
		functionlist[i] = nodefunctionname(i, NONODEINST);
	DiaSetPopup(dia, DNCF_FUNCLIST, MAXNODEFUNCTION, functionlist);
	cellselchanged = TRUE;
	for(;;)
	{
		if (cellselchanged)
		{
			cellselchanged = FALSE;
			np = net_nccdlgselectedcell(dia, DNCF_LIBLIST, DNCF_CELLLIST);
			if (np->temp1 == 0) i = 0; else
			{
				for(i=0; i<MAXNODEFUNCTION; i++)
					if (namesame((CHAR *)np->temp1, nodefunctionname(i, NONODEINST)) == 0) break;
			}
			DiaSetPopupEntry(dia, DNCF_FUNCLIST, i);
		}
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DNCF_OVERRIDE)
		{
			DiaSetControl(dia, DNCF_OVERRIDE, 1-DiaGetControl(dia, DNCF_OVERRIDE));
			continue;
		}
		if (itemHit == DNCF_CELLLIST)
		{
			cellselchanged = TRUE;
			continue;
		}
		if (itemHit == DNCF_LIBLIST)
		{
			i = DiaGetPopupEntry(dia, DNCF_LIBLIST);
			lib = getlibrary(net_librarylist[i]);
			if (lib == NOLIBRARY) continue;
			net_library = lib;
			DiaLoadTextDialog(dia, DNCF_CELLLIST, net_topofcells, net_nextcellswithfunction, DiaNullDlogDone, 0);
			continue;
		}
		if (itemHit == DNCF_FUNCLIST)
		{
			np = net_nccdlgselectedcell(dia, DNCF_LIBLIST, DNCF_CELLLIST);
			if (np == NONODEPROTO) continue;
			i = DiaGetPopupEntry(dia, DNCF_FUNCLIST);
			if (i == 0)
			{
				np->temp1 = 0;
				i = DiaGetCurLine(dia, DNCF_CELLLIST);
				DiaSetScrollLine(dia, DNCF_CELLLIST, i, nldescribenodeproto(np));
			} else
			{
				np->temp1 = (INTBIG)nodefunctionname(i, NONODEINST);
				infstr = initinfstr();
				formatinfstr(infstr, "%s (%s)", nldescribenodeproto(np), (CHAR *)np->temp1);
				i = DiaGetCurLine(dia, DNCF_CELLLIST);
				DiaSetScrollLine(dia, DNCF_CELLLIST, i, returninfstr(infstr));

				/* vet the port names */
				switch (i)
				{
					case NPPIN:
					case NPCONTACT:
					case NPNODE:
					case NPCONNECT:
					case NPSUBSTRATE:
					case NPWELL:
					case NPART:
						ttyputmsg(_("This function has no electrical properties, so it makes no sense to assign it to a cell"));
						break;
					case NPTRANMOS:
					case NPTRADMOS:
					case NPTRAPMOS:
					case NPTRANPN:
					case NPTRAPNP:
					case NPTRANJFET:
					case NPTRAPJFET:
					case NPTRADMES:
					case NPTRAEMES:
					case NPTRANSREF:
					case NPTRANS:
						/* must have three ports named "s", "g", and "d" */
						s = g = d = FALSE;
						for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						{
							if (namesame(pp->protoname, "s") == 0) s = TRUE; else
							if (namesame(pp->protoname, "g") == 0) g = TRUE; else
							if (namesame(pp->protoname, "d") == 0) d = TRUE; else
							{
								s = g = d = FALSE;
								break;
							}
						}
						if (!s || !g || !d)
							ttyputmsg("Transistor cells must have 3 ports named 's', 'g', and 'd'");
						break;
					case NPTRA4NMOS:
					case NPTRA4DMOS:
					case NPTRA4PMOS:
					case NPTRA4NPN:
					case NPTRA4PNP:
					case NPTRA4NJFET:
					case NPTRA4PJFET:
					case NPTRA4DMES:
					case NPTRA4EMES:
					case NPTRANS4:
						/* must have four ports named "s", "g", "d", and "b" */
						s = g = d = b = FALSE;
						for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
						{
							if (namesame(pp->protoname, "s") == 0) s = TRUE; else
							if (namesame(pp->protoname, "g") == 0) g = TRUE; else
							if (namesame(pp->protoname, "d") == 0) d = TRUE; else
							if (namesame(pp->protoname, "b") == 0) b = TRUE; else
							{
								s = g = d = b = FALSE;
								break;
							}
						}
						if (!s || !g || !d || !b)
							ttyputmsg("Transistor cells must have 4 ports named 's', 'g', 'd', and 'b'");
						break;
					case NPRESIST:
					case NPCAPAC:
					case NPECAPAC:
					case NPINDUCT:
						/* must have two ports */
						i = 0;
						for(pp = np->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto) i++;
						if (i != 2)
							ttyputmsg("Cells with this function must have 2 ports");
						break;
					case NPDIODE:
					case NPDIODEZ:
					case NPMETER:
					case NPBASE:
					case NPEMIT:
					case NPCOLLECT:
					case NPBUFFER:
					case NPGATEAND:
					case NPGATEOR:
					case NPGATEXOR:
					case NPFLIPFLOP:
					case NPMUX:
					case NPCONPOWER:
					case NPCONGROUND:
					case NPSOURCE:
					case NPARRAY:
					case NPALIGN:
					case NPCCVS:
					case NPCCCS:
					case NPVCVS:
					case NPVCCS:
					case NPTLINE:
						ttyputmsg(_("This function is not known to NCC, so it is not useful to assign it to a cell"));
						break;
				}
			}
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		{
			if ((lib->userbits&HIDDENLIBRARY) != 0) continue;
			for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				var = getvalkey((INTBIG)np, VNODEPROTO, VSTRING, net_ncc_function_key);
				if (var != NOVARIABLE && np->temp1 == 0)
				{
					delvalkey((INTBIG)np, VNODEPROTO, net_ncc_function_key);
					continue;
				}
				if (np->temp1 != 0)
				{
					setvalkey((INTBIG)np, VNODEPROTO, net_ncc_function_key, np->temp1, VSTRING);
					if (DiaGetControl(dia, DNCF_OVERRIDE) != 0)
					{
						var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, net_ncc_optionskey);
						if (var == NOVARIABLE) i = 0; else i = var->addr;
						i |= NCCHIERARCHICALOVER;
						i &= ~NCCHIERARCHICAL;
						setvalkey((INTBIG)np, VNODEPROTO, net_ncc_optionskey, i, VINTEGER);
					}
				}
			}
		}
	}
	DiaDoneDialog(dia);
	efree((CHAR *)net_librarylist);
}

/****************************** NCC DEBUGGING DIALOG ******************************/

/* NCC Debugging */
static DIALOGITEM net_nccdebdialogitems[] =
{
 /*  1 */ {0, {216,217,240,297}, BUTTON, N_("OK")},
 /*  2 */ {0, {216,60,240,140}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,4,20,332}, CHECK, N_("Enable local processing after match")},
 /*  4 */ {0, {28,4,44,332}, CHECK, N_("Enable focus on 'fresh' symmetry groups")},
 /*  5 */ {0, {76,4,92,332}, CHECK, N_("Disable unimportant disambiguation messages")},
 /*  6 */ {0, {112,4,128,332}, CHECK, N_("Show gemini iterations graphically")},
 /*  7 */ {0, {136,4,152,332}, CHECK, N_("Show gemini iterations textually")},
 /*  8 */ {0, {160,4,176,332}, CHECK, N_("Show matching progress graphically")},
 /*  9 */ {0, {52,4,68,332}, CHECK, N_("Enable focus on 'promising' symmetry groups")},
 /* 10 */ {0, {184,4,200,332}, CHECK, N_("Show gemini statistics")}
};
static DIALOG net_nccdebdialog = {{75,75,324,417}, N_("NCC Debugging Settings"), 0, 10, net_nccdebdialogitems, 0, 0};

/* special items for the "NCC Debugging" dialog: */
#define DNCD_ENALOCAFTERMATCH      3		/* Enable local processing after match (check) */
#define DNCD_ENAFOCSYMGRPFRE       4		/* Enable focus on fresh symmetry groups (check) */
#define DNCD_DISUNDISMSG           5		/* Disable unimportant disambiguation messages (check) */
#define DNCD_SHOWGEMINIGRA         6		/* Show gemini iterations graphically (check) */
#define DNCD_SHOWGEMINITXT         7		/* Show gemini iterations textually (check) */
#define DNCD_SHOWGRAPROG           8		/* Show graphical matching progress (check) */
#define DNCD_ENAFOCSYMGRPPRO       9		/* Enable focus on promising symmetry groups (check) */
#define DNCD_SHOWSTATISTICS       10		/* Show statistics during run (check) */

INTBIG net_nccdebuggingdlog(INTBIG options)
{
	REGISTER void *dia;
	REGISTER INTBIG itemHit, oldoptions;

	dia = DiaInitDialog(&net_nccdebdialog);
	if (dia == 0) return(options);

	oldoptions = options;
	if ((options&NCCDISLOCAFTERMATCH) == 0) DiaSetControl(dia, DNCD_ENALOCAFTERMATCH, 1);
	if ((options&NCCENAFOCSYMGRPFRE) != 0) DiaSetControl(dia, DNCD_ENAFOCSYMGRPFRE, 1);
	if ((options&NCCENAFOCSYMGRPPRO) != 0) DiaSetControl(dia, DNCD_ENAFOCSYMGRPPRO, 1);
	if ((options&NCCSUPALLAMBREP) != 0) DiaSetControl(dia, DNCD_DISUNDISMSG, 1);
	if ((options&NCCVERBOSEGRAPHICS) != 0) DiaSetControl(dia, DNCD_SHOWGEMINIGRA, 1);
	if ((options&NCCVERBOSETEXT) != 0) DiaSetControl(dia, DNCD_SHOWGEMINITXT, 1);
	if ((options&NCCGRAPHICPROGRESS) != 0) DiaSetControl(dia, DNCD_SHOWGRAPROG, 1);
	if ((options&NCCENASTATISTICS) != 0) DiaSetControl(dia, DNCD_SHOWSTATISTICS, 1);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DNCD_ENALOCAFTERMATCH || itemHit == DNCD_ENAFOCSYMGRPFRE ||
			itemHit == DNCD_DISUNDISMSG || itemHit == DNCD_SHOWGEMINIGRA ||
			itemHit == DNCD_SHOWGEMINITXT || itemHit == DNCD_ENAFOCSYMGRPPRO ||
			itemHit == DNCD_SHOWGRAPROG || itemHit == DNCD_SHOWSTATISTICS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
	}
	if (itemHit != CANCEL)
	{
		options = options & ~(NCCDISLOCAFTERMATCH|NCCENAFOCSYMGRPFRE|NCCENAFOCSYMGRPPRO|
			NCCSUPALLAMBREP|NCCVERBOSEGRAPHICS|NCCVERBOSETEXT|NCCGRAPHICPROGRESS|NCCENASTATISTICS);
		if (DiaGetControl(dia, DNCD_ENALOCAFTERMATCH) == 0) options |= NCCDISLOCAFTERMATCH;
		if (DiaGetControl(dia, DNCD_ENAFOCSYMGRPFRE) != 0) options |= NCCENAFOCSYMGRPFRE;
		if (DiaGetControl(dia, DNCD_ENAFOCSYMGRPPRO) != 0) options |= NCCENAFOCSYMGRPPRO;
		if (DiaGetControl(dia, DNCD_DISUNDISMSG) != 0) options |= NCCSUPALLAMBREP;
		if (DiaGetControl(dia, DNCD_SHOWGEMINIGRA) != 0) options |= NCCVERBOSEGRAPHICS;
		if (DiaGetControl(dia, DNCD_SHOWGEMINITXT) != 0) options |= NCCVERBOSETEXT;
		if (DiaGetControl(dia, DNCD_SHOWGRAPROG) != 0) options |= NCCGRAPHICPROGRESS;
		if (DiaGetControl(dia, DNCD_SHOWSTATISTICS) != 0) options |= NCCENASTATISTICS;
	}
	DiaDoneDialog(dia);
	return(options);
}

/****************************** NETWORK OPTIONS DIALOG ******************************/

/* Network: Options */
static DIALOGITEM net_optionsdialogitems[] =
{
 /*  1 */ {0, {288,148,312,212}, BUTTON, N_("OK")},
 /*  2 */ {0, {288,24,312,88}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,20,44,226}, CHECK, N_("Unify Power and Ground")},
 /*  4 */ {0, {52,20,68,226}, CHECK, N_("Unify all like-named nets")},
 /*  5 */ {0, {4,4,20,165}, MESSAGE, N_("Network numbering:")},
 /*  6 */ {0, {228,36,244,201}, RADIO, N_("Ascending (0:N)")},
 /*  7 */ {0, {252,36,268,201}, RADIO, N_("Descending (N:0)")},
 /*  8 */ {0, {180,20,196,177}, MESSAGE, N_("Default starting index:")},
 /*  9 */ {0, {180,180,196,226}, POPUP, x_("")},
 /* 10 */ {0, {148,4,149,246}, DIVIDELINE, x_("")},
 /* 11 */ {0, {204,20,220,177}, MESSAGE, N_("Default order:")},
 /* 12 */ {0, {156,4,172,177}, MESSAGE, N_("For busses:")},
 /* 13 */ {0, {76,20,92,226}, CHECK, N_("Ignore Resistors")},
 /* 14 */ {0, {100,20,116,246}, MESSAGE, N_("Unify Networks that start with:")},
 /* 15 */ {0, {124,32,140,246}, EDITTEXT, x_("")}
};
static DIALOG net_optionsdialog = {{50,75,371,331}, N_("Network Options"), 0, 15, net_optionsdialogitems, 0, 0};

/* special items for the "Network Options" dialog: */
#define DNTO_UNIFYPG             3		/* Unify Power and Ground (check) */
#define DNTO_UNIFYLIKENAMEDNETS  4		/* Unify all like-named nets (check) */
#define DNTO_UNNAMEDASCEND       6		/* Number unnamed busses ascending (radio) */
#define DNTO_UNNAMEDDESCEND      7		/* Number unnamed busses descending (radio) */
#define DNTO_UNNAMEDSTARTINDEX   9		/* Starting index of unnamed busses (popup) */
#define DNTO_IGNORERESISTORS    13		/* Ignore resistors (check) */
#define DNTO_UNIFYSTRING        15		/* String for unifying network names (edit text) */

void net_optionsdlog(void)
{
	REGISTER INTBIG itemHit, i;
	CHAR *choices[3], *initunifystrings, *pt;
	REGISTER VARIABLE *var;
	REGISTER BOOLEAN redonets;
	REGISTER void *dia;

	dia = DiaInitDialog(&net_optionsdialog);
	if (dia == 0) return;
	choices[0] = x_("0");   choices[1] = x_("1");
	DiaSetPopup(dia, DNTO_UNNAMEDSTARTINDEX, 2, choices);

	/* load general network options */
	if ((net_options&NETCONPWRGND) != 0) DiaSetControl(dia, DNTO_UNIFYPG, 1);
	if ((net_options&NETCONCOMMONNAME) != 0) DiaSetControl(dia, DNTO_UNIFYLIKENAMEDNETS, 1);
	if ((net_options&NETIGNORERESISTORS) != 0) DiaSetControl(dia, DNTO_IGNORERESISTORS, 1);

	/* load bus naming options */
	if ((net_options&NETDEFBUSBASE1) != 0)
	{
		DiaSetPopupEntry(dia, DNTO_UNNAMEDSTARTINDEX, 1);
		DiaSetText(dia, DNTO_UNNAMEDASCEND, _("Ascending (1:N)"));
		DiaSetText(dia, DNTO_UNNAMEDDESCEND, _("Descending (N:1)"));
	} else
	{
		DiaSetText(dia, DNTO_UNNAMEDASCEND, _("Ascending (0:N)"));
		DiaSetText(dia, DNTO_UNNAMEDDESCEND, _("Descending (N:0)"));
	}
	if ((net_options&NETDEFBUSBASEDESC) != 0) DiaSetControl(dia, DNTO_UNNAMEDDESCEND, 1); else
		DiaSetControl(dia, DNTO_UNNAMEDASCEND, 1);
	var = getvalkey((INTBIG)net_tool, VTOOL, VSTRING, net_unifystringskey);
	if (var == NOVARIABLE) initunifystrings = x_(""); else initunifystrings = (CHAR *)var->addr;
	DiaSetText(dia, DNTO_UNIFYSTRING, initunifystrings);

	/* loop until done */
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DNTO_UNIFYPG || itemHit == DNTO_UNIFYLIKENAMEDNETS ||
			itemHit == DNTO_IGNORERESISTORS)
		{
			DiaSetControl(dia, itemHit, 1 - DiaGetControl(dia, itemHit));
			continue;
		}
		if (itemHit == DNTO_UNNAMEDDESCEND || itemHit == DNTO_UNNAMEDASCEND)
		{
			DiaSetControl(dia, DNTO_UNNAMEDASCEND, 0);
			DiaSetControl(dia, DNTO_UNNAMEDDESCEND, 0);
			DiaSetControl(dia, itemHit, 1);
			continue;
		}
		if (itemHit == DNTO_UNNAMEDSTARTINDEX)
		{
			if (DiaGetPopupEntry(dia, DNTO_UNNAMEDSTARTINDEX) != 0)
			{
				DiaSetText(dia, DNTO_UNNAMEDASCEND, _("Ascending (1:N)"));
				DiaSetText(dia, DNTO_UNNAMEDDESCEND, _("Descending (N:1)"));
			} else
			{
				DiaSetText(dia, DNTO_UNNAMEDASCEND, _("Ascending (0:N)"));
				DiaSetText(dia, DNTO_UNNAMEDDESCEND, _("Descending (N:0)"));
			}
			continue;
		}
	}

	if (itemHit != CANCEL)
	{
		redonets = FALSE;
		i = net_options & ~(NETCONPWRGND | NETCONCOMMONNAME | NETDEFBUSBASE1 | NETDEFBUSBASEDESC |
			NETIGNORERESISTORS);
		if (DiaGetControl(dia, DNTO_UNIFYPG) != 0) i |= NETCONPWRGND;
		if (DiaGetControl(dia, DNTO_UNIFYLIKENAMEDNETS) != 0) i |= NETCONCOMMONNAME;
		if (DiaGetControl(dia, DNTO_IGNORERESISTORS) != 0) i |= NETIGNORERESISTORS;
		if (DiaGetPopupEntry(dia, DNTO_UNNAMEDSTARTINDEX) != 0) i |= NETDEFBUSBASE1;
		if (DiaGetControl(dia, DNTO_UNNAMEDDESCEND) != 0) i |= NETDEFBUSBASEDESC;
		if (i != net_options)
		{
			redonets = TRUE;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_optionskey, i, VINTEGER);
			net_tellschematics();
		}
		pt = DiaGetText(dia, DNTO_UNIFYSTRING);
		if (namesame(pt, initunifystrings) != 0)
		{
			redonets = TRUE;
			(void)setvalkey((INTBIG)net_tool, VTOOL, net_unifystringskey, (INTBIG)pt, VSTRING);
		}
		if (redonets)
		{
			net_totalrenumber(NOLIBRARY);
		}
	}
	DiaDoneDialog(dia);
}
