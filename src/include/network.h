/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: network.h
 * Network tool: header file for fully instantiated networks
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

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
extern "C"
{
#endif

/* the meaning of tool:network.NET_options */
#define NETCONPWRGND                   1		/* set to connect power and ground */
#define NETCONCOMMONNAME               2		/* set to connect commonly named nets */
#define NETDEFBUSBASE1               010		/* set to have default bus base at 1 (instead of 0) */
#define NETDEFBUSBASEDESC            020		/* set to have default busses numbered descending */
#define NETIGNORERESISTORS           040		/* set to ignore resistors */
#define NETDEFAULTABBREVLEN            8		/* default length of cell name abbreviations on nodes */

#define HASHTYPE                 INTHUGE		/* the size of hash values */

/* the meaning of NODEPROTO->temp1 by various network tools */
#define NETNCCCHECKSTATE              03		/* state of NCC check for this cell */
#define NETNCCNOTCHECKED               0		/*    NCC not checked for this cell */
#define NETNCCCHECKEDGOOD             01		/*    NCC checked successfully for this cell */
#define NETNCCCHECKEDBAD              02		/*    NCC checked unsuccessfully for this cell */
#define NETGLOBALCHECKED              04		/* global analysis flag for this cell */
#define NETCELLCODE           0777777770		/* coding for this cell when matching without expansion */
#define NETCELLCODESH                  3		/* right shift of NETCELLCODE */

extern INTBIG      net_options;					/* cached value for "NET_options" */
extern INTBIG      net_unifystringskey;			/* key for "NET_unify_strings" */
extern INTBIG      net_unifystringskey;			/* key for "NET_component_tolerance" */
extern INTBIG      net_node_abbrev_key;			/* key for "NET_node_abbreviations" */
extern INTBIG      net_node_abbrevlen_key;		/* key for "NET_node_abbreviation_length" */
extern INTBIG      net_ncc_optionskey;			/* key for "NET_ncc_options" */
extern INTBIG      net_ncc_comptolerancekey;	/* key for "NET_ncc_component_tolerance" */
extern INTBIG      net_ncc_comptoleranceamtkey;	/* key for "NET_ncc_component_tolerance_amt" */
extern INTBIG      net_ncc_matchkey;			/* key for "NET_ncc_match" */
extern INTBIG      net_ncc_forcedassociationkey;/* key for "NET_ncc_forcedassociation" */
extern INTBIG      net_ncc_processors_key;		/* key for "NET_ncc_num_processors" */
extern INTBIG      net_ncc_function_key;		/* key for "NET_ncc_function" */
extern INTBIG      net_ncc_options;				/* options to use in NCC */

/* the meaning of tool:network.NET_ncc_options */
#define NCCHIERARCHICAL               01		/* set to expand hierarchy during NCC */
#define NCCCHECKEXPORTNAMES           02		/* set to check export names during NCC */
#define NCCVERBOSETEXT                04		/* set to dump information during NCC */
#define NCCVERBOSEGRAPHICS           010		/* set to display information during NCC */
#define NCCIGNOREPWRGND              020		/* set to ignore power/ground during NCC */
#define NCCRECURSE                   040		/* set to do NCC recursively */
#define NCCHIDEMATCHTAGS            0200		/* set to hide "NCCMatch" tags */
#define NCCINCLUDENOCOMPNETS        0400		/* set to include no-component nets */
#define NCCCHECKSIZE               01000		/* set to check component size during NCC */
#define NCCNOMERGEPARALLEL         04000		/* set to not merge parallel components during NCC */
#define NCCNOMERGEPARALLELOVER    010000		/* set to override parallel component nonmerge for a cell */
#define NCCMERGESERIES            020000		/* set to merge series transistors during NCC */
#define NCCMERGESERIESOVER        040000		/* set to override series transistors merge for a cell */
#define NCCHIERARCHICALOVER      0100000		/* set to override hierarchy expansion during NCC */
#define NCCEXPERIMENTAL          0200000		/* set to use experimental NCC */
#define NCCGRAPHICPROGRESS       0400000		/* set to show NCC progress graphically */
#define NCCDISLOCAFTERMATCH     01000000		/* set to disable local processing after NCC match */
#define NCCENAFOCSYMGRPFRE      02000000		/* set to enable focus on "fresh" symmetry groups during NCC */
#define NCCSUPALLAMBREP         04000000		/* set to supress all ambiguous messages during NCC */
#define NCCENAFOCSYMGRPPRO     010000000		/* set to enable focus on "promising" symmetry groups during NCC */
#define NCCENASTATISTICS       020000000		/* set to enable statistics during NCC */
#define NCCRESISTINCLUSION    0140000000		/* how to treat resistors during NCC */
#define NCCRESISTLEAVE                 0		/*    leave current state alone */
#define NCCRESISTINCLUDE       040000000		/*    make sure they are included */
#define NCCRESISTEXCLUDE      0100000000		/*    make sure they are excluded */

/*********************** NET CELL PRIVATE ************************/

#ifdef __cplusplus

#if 1
#  define NEWRENUM                            /* new network renumbering algorithm */
#endif

class NetCellPrivate;                       /* private network data */
class NetName;                              /* name of network */
class NetCellShorts;                        /* electrical shortcuts between exports inside cell */

class NetCellPrivate
{
	friend class NetName;
public:
	void* operator new( size_t size );
	void* operator new( size_t size, CLUSTER *cluster );
	void operator delete( void* obj );
#ifndef MACOS
	void operator delete( void *obj, CLUSTER *cluster );
#endif
	NetCellPrivate( NODEPROTO *np, CLUSTER *cluster );
	~NetCellPrivate();
	BOOLEAN updateShorts();
	NetCellShorts *netshorts() { return _netshorts; };
	NetName *findNetName( CHAR *name, BOOLEAN insert );
	void docheck();
#ifdef NEWRENUM
	void clearConns();
	void calcConns(INTBIG *count);
	void showConns(INTBIG *deepmap);
#endif
	CLUSTER *cluster() { return _cluster; };
private:
	void rehashNetNames();
	void insertNetName( NetName *nn, INTBIG hash );
#ifdef NEWRENUM
	NetName *addNetName( CHAR *name );
#endif
	NODEPROTO *_np;
	CLUSTER *_cluster;
	NetName **_netnamehash;
	INTBIG _netnametotal;
	INTBIG _netnamecount;
	NetCellShorts *_netshorts;
};

class NetCellShorts
{
public:
	void* operator new( size_t size );
	void* operator new( size_t size, CLUSTER *cluster );
	void operator delete( void* obj );
#ifndef MACOS
	void operator delete( void *obj, CLUSTER *cluster );
#endif
	NetCellShorts( NODEPROTO *np, CLUSTER *cluster );
	~NetCellShorts();
	BOOLEAN isConsistent();
	void printf();
	INTBIG portwidth( INTBIG portno );
	INTBIG portcount() { return _portcount; };
	INTBIG globalcount() { return _portbeg[0]; };
	INTBIG portdeepcount() { return _portbeg[_portcount] - _portbeg[0]; };
#ifdef NEWRENUM
	INTBIG portbeg(INTBIG i) { return _portbeg[i]; }
	CHAR *globalname(INTBIG i) { return _globalnames[i]; }
	BOOLEAN isolated(INTBIG portno) { return _isolated[portno]; };
	INTBIG portdeepmap(INTBIG portno) { return _portdeepmap[portno]; };
#endif
	INTBIG portshallowmap(INTBIG portno) { return _portshallowmap[portno]; };
	BOOLEAN globalshort() { return _globalshort; };
private:
	static void clearNetworks(NODEPROTO *np);
	NODEPROTO *_np;
	INTBIG _portcount;
	INTBIG *_portbeg;
#ifdef NEWRENUM
	BOOLEAN *_isolated;
#endif
	CHAR **_globalnames;
	INTBIG *_portshallowmap;
	INTBIG *_portdeepmap;
	BOOLEAN _globalshort;
};

/*********************** NET NAME ************************/

class NetName
{
	friend class NetCellPrivate;
public:
	void* operator new( size_t size );
	void* operator new( size_t size, CLUSTER *cluster );
	void operator delete( void* obj );
#ifndef MACOS
	void operator delete( void *obj, CLUSTER *cluster );
#endif
	NetName( NetCellPrivate *npd, CHAR *name );
	~NetName();
	NETWORK *firstNet();
	void addNet(NETWORK *net);
	void removeNet(NETWORK *net);
	CHAR *name() { return _name; };
	void checkArity();
#ifdef NEWRENUM
	INTBIG busWidth() { return _busWidth; };
	class NetName *subName(INTBIG i) { return _netNameList[i]; };
	void setBusWidth( INTBIG busWidth, CHAR **strings );
	INTBIG conn() { return _conn; }
	void markConn();
#endif
private:
	void docheck();
	void checkNet(NETWORK *net);
	NetCellPrivate *_npd;
	CHAR *_name;
	INTBIG _netcount;
	INTBIG _netaddr;
	class NetName *_baseNetName;
	INTBIG _baseRefCount;
#ifdef NEWRENUM
	INTBIG    _busWidth;			/* width of bus */
	class NetName **_netNameList;			/* list of single-wire netnames on bus */
	INTBIG _conn;                   /* connection index */
#endif
};        

#endif

/*********************** PCOMP MODULES ***********************/

#define NOPCOMP ((PCOMP *)-1)

/* meaning of PCOMP->state */
#define NEGATEDPORT         1			/* if port is negated */
#define EXPORTEDPORT        2			/* if port is exported */

/* meaning of PCOMP->flags */
#define COMPUNIQUEHASH      1			/* if component has unique hash */
#define COMPHASWIDLEN       2			/* if component has width and length size info */
#define COMPHASAREA         4			/* if component has area size info */
#define COMPDELETED       010			/* if component has been deleted */
#define COMPPARALLELSEEN  020			/* if component has been seen in parallel merge */
#define COMPTEMPFLAG      040			/* flag used in parallel merging */
#define COMPLOCALFLAG    0100			/* flag used in NCC */
#define COMPMATCHED      0200			/* if component has been matched */

typedef struct Ipcomp
{
	INTBIG           numactual;			/* number of components associated with this */
	void            *actuallist;		/* actual components */
	NODEINST        *topactual;			/* actual component at topmost level */
	NODEINST       **hierpath;			/* hierarchical path to this */
	INTBIG          *hierindex;			/* hierarchical path index to this */
	INTBIG           hierpathcount;		/* length of hierarchical path */
	INTSML           flags;				/* state bits */
	INTSML           function;			/* component function */
	INTSML           wirecount;			/* number of unconnected ports (wires) */
	INTSML           truewirecount;		/* number of unconnected ports, excluding ignored Pwr&Gnd */
	INTBIG           timestamp;			/* time stamp for entry into symmetry group */
	INTBIG           forcedassociation;	/* extra factor used to force associations */
	HASHTYPE         hashvalue;			/* hash value for NCC */
	CHAR            *hashreason;		/* the explanation of the hash value */
	float            length;			/* length/size of component */
	float            width;				/* width of component (if FET) */
	INTSML          *portindices;		/* normalized indices for each wire */
	PORTPROTO      **portlist;			/* PORTPROTOs on each wire */
	struct Ipnet   **netnumbers;		/* initial netnumbers for each wire */
	INTSML          *state;				/* information about the connection */
	void            *symgroup;			/* symmetry group (used during NCC) */
	struct Ipcomp   *nextpcomp;			/* next in list of pseudocomponents */
} PCOMP;
extern PCOMP *net_pcompfree;

/*********************** PNET MODULES ***********************/

/* #define PATHTOPNET 1 */			/* uncomment to preserve and show the path to a net */

#define NOPNET ((PNET *)-1)

/* meaning of PNET->flags */
#define POWERNET         1			/* if net is power */
#define GROUNDNET        2			/* if net is ground */
#define EXPORTEDNET      4			/* if net is exported */
#define GLOBALNET      010			/* if net is a global signal */
#define NETUNIQUEHASH  020			/* if net has unique hash */
#define NETLOCALFLAG   040			/* flag used in NCC */
#define NETMATCHED    0100			/* if network has been matched */

typedef struct Ipnet
{
	INTSML         flags;			/* state bits */
	INTSML         realportcount;	/* number of top-level exports on this net */
	void          *realportlist;	/* top-level export(s) on this net */
	NETWORK       *network;			/* real network that this references */
#ifdef PATHTOPNET
	NODEINST     **hierpath;		/* hierarchical path to this */
	INTBIG        *hierindex;		/* hierarchical path index to this */
	INTBIG         hierpathcount;	/* length of hierarchical path */
#endif
	HASHTYPE       hashvalue;		/* hash value for NCC */
	CHAR          *hashreason;		/* the explanation of the hash value */
	INTBIG         timestamp;		/* time stamp for entry into symmetry group */
	INTBIG         nodecount;		/* number of pnodes on this network */
	INTBIG         nodetotal;		/* allocated space for pnodes and wires on this network */
	INTBIG         forcedassociation;/* extra factor used to force associations */
	PCOMP        **nodelist;		/* list of pnodes on this network */
	INTBIG        *nodewire;		/* list of which wire on the pnode attaches to network */
	void          *symgroup;		/* symmetry group (used during NCC) */
	struct Ipnet  *nextpnet;		/* next in list */
} PNET;
extern PNET *net_pnetfree;

/*********************** AREAPERIM and TRANSISTORINFO MODULES ***********************/

#define NOAREAPERIM ((AREAPERIM *)-1)

typedef struct Iareaperim
{
	float              area;
	INTBIG             perimeter;
	INTBIG             layer;
	TECHNOLOGY        *tech;
	struct Iareaperim *nextareaperim;
} AREAPERIM;

typedef struct
{
	INTBIG count;			/* number of transistors found */
	INTBIG area;			/* sum of area of transistors */
	INTBIG width;			/* sum of width of transistors */
	INTBIG length;			/* sum of length of transistors */
} TRANSISTORINFO;

extern TRANSISTORINFO   net_transistor_p_gate;		/* info on P transistors connected at gate */
extern TRANSISTORINFO   net_transistor_n_gate;		/* info on N transistors connected at gate */
extern TRANSISTORINFO   net_transistor_p_active;	/* info on P transistors connected at active */
extern TRANSISTORINFO   net_transistor_n_active;	/* info on N transistors connected at active */

/* prototypes for tool interface */
void net_init(INTBIG*, CHAR1*[], TOOL*);
void net_done(void);
void net_set(INTBIG, CHAR*[]);
INTBIG net_request(CHAR*, va_list);
void net_examinenodeproto(NODEPROTO*);
void net_slice(void);
void net_startbatch(TOOL*, BOOLEAN);
void net_endbatch(void);
void net_modifyportproto(PORTPROTO*, NODEINST*, PORTPROTO*);
void net_newobject(INTBIG, INTBIG);
void net_killobject(INTBIG, INTBIG);
void net_newvariable(INTBIG, INTBIG, INTBIG, INTBIG);
void net_killvariable(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, UINTBIG*);
void net_readlibrary(LIBRARY*);
void net_eraselibrary(LIBRARY*);

/* prototypes for intratool interface */
int        net_areaperimdepthascending(const void *e1, const void *e2);
NETWORK  **net_gethighlightednets(BOOLEAN disperror);
void       net_freeallpcomp(PCOMP*);
void       net_freeallpnet(PNET*);
void       net_freepcomp(PCOMP*);
void       net_freepnet(PNET*);
void       net_initnetflattening(void);
PCOMP     *net_makepseudo(NODEPROTO*, INTBIG*, INTBIG*, INTBIG*, INTBIG*, PNET**,
			BOOLEAN, BOOLEAN, BOOLEAN, BOOLEAN, BOOLEAN);
AREAPERIM *net_gathergeometry(NETWORK *net, TRANSISTORINFO **p_gate, TRANSISTORINFO **n_gate,
			TRANSISTORINFO **p_active, TRANSISTORINFO **n_active, BOOLEAN recurse);
BOOLEAN    net_equate(BOOLEAN);
INTBIG	   net_getequivalentnet(NETWORK *net, NETWORK ***equiv, INTBIG *numequiv, BOOLEAN allowchild);
BOOLEAN    net_compare(BOOLEAN preanalyze, BOOLEAN interactive, NODEPROTO *cell1, NODEPROTO *cell2);
INTBIG     net_buswidth(CHAR*);
INTBIG     net_evalbusname(INTBIG, CHAR*, CHAR***, ARCINST*, NODEPROTO*, INTBIG);
void       net_freediffmemory(void);
void       net_freeflatmemory(void);
BOOLEAN    net_samenetworkname(NETWORK *net1, NETWORK *net2);
INTBIG     net_mergeparallel(PCOMP **pcomp, PNET *pnet, INTBIG *components);
BOOLEAN    net_comparewirelist(PCOMP *p1, PCOMP *p2, BOOLEAN useportnames);
void       net_dumpnetwork(PCOMP *pclist, PNET *pnlist);
void       net_removeextraneous(PCOMP **pcomplist, PNET **pnetlist, INTBIG *comp);
CHAR      *net_describepnet(PNET *pn);
CHAR      *net_describepcomp(PCOMP *pc);
HASHTYPE   net_getcomphash(PCOMP *pc, INTBIG verbose);
HASHTYPE   net_getnethash(PNET *pn, INTBIG verbose);
NETWORK   *net_getnetwork(CHAR *netname, NODEPROTO *cell);
void       net_redoprim(void);
void       net_initnetprivate(NODEPROTO*);
void       net_freenetprivate(NODEPROTO*);
void	   net_checknetprivate(NODEPROTO*);
void       net_initdiff(void);
void       net_showcomphash(WINDOWPART *win, PCOMP *pc, HASHTYPE hashvalue, INTBIG hashindex, INTBIG verbose);
void       net_shownethash(WINDOWPART *win, PNET *pn, HASHTYPE hashvalue, INTBIG hashindex, INTBIG verbose);
void       net_freenetwork(NETWORK*, NODEPROTO*);
void       net_conv_to_internal(NODEPROTO *np);
void       net_removeassociations(void);
void       net_fillinnetpointers(PCOMP *pcomplist, PNET *pnetlist);
BOOLEAN    net_nccalreadydone(NODEPROTO *cell1, NODEPROTO *cell2);
void       net_nccremovematches(NODEPROTO *np);
INTBIG     net_ncchasmatch(NODEPROTO *np);
void	   net_nccmatchinfo(NODEPROTO *np, NODEPROTO **cellmatch, UINTBIG *celldate);
void       net_parsenccresult(NODEPROTO *np, VARIABLE *var, NODEPROTO **cellmatch,
			UINTBIG *celldate);
VARIABLE  *net_nccfindmatch(NODEPROTO *np, NODEPROTO *onp, UINTBIG *matchdate);
void       net_setnodewidth(NODEINST *ni);
INTBIG     net_gathernodenames(NODEPROTO *np, CHAR ***namelist);
BOOLEAN    net_getcells(NODEPROTO**, NODEPROTO**);
void       net_listnccoverrides(BOOLEAN usetemp1);
BOOLEAN    net_componentequalvalue(float v1, float v2);
INTBIG     net_findglobalnet(NODEPROTO *np, CHAR *name);
#ifdef FORCESUNTOOLS
BOOLEAN    net_analyzecell(void);
void       net_freeexpdiffmemory(void);
INTBIG     net_doexpgemini(PCOMP *pcomp1, PNET *pnet1, PCOMP *pcomp2, PNET *pnet2,
			BOOLEAN checksize, BOOLEAN checkexportnames, BOOLEAN ignorepwrgnd);
BOOLEAN    net_equateexp(BOOLEAN noise);
INTBIG     net_expanalyzesymmetrygroups(BOOLEAN reporterrors, BOOLEAN checksize,
				BOOLEAN checkexportname, BOOLEAN ignorepwrgnd, INTBIG *errorcount);
#endif

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
