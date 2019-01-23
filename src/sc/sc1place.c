/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: sc1place.c
 * Placement modules for the QUISC Silicon Compiler
 * Written by: Andrew R. Kostiuk, Queen's University
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
#if SCTOOL

#include <math.h>
#include <setjmp.h>
#include "global.h"
#include "sc1.h"

/***** blerbs for showing what placement algorithms is used *****/

static CHAR *sc_placeblerb[] =
{
	M_("************ QUISC PLACEMENT MODULE - VERSION 1.00"),
	M_("Cluster Tree Creation:"),
	M_("    o  Maxi-cut Algorithm"),
	M_("    o  Minimum use for best pair choice for equal weight"),
	M_("Cluster Tree Sorting:"),
	M_("    o  Top-down cluster tree sort"),
	M_("    o  Bottom-up cluster tree sort"),
	M_("Net Balancing:"),
	M_("    o  Independent routing channels"),
	M_("    o  Cross refencing of trunks to improve speed"),
	M_("    o  Effective calculation of rows in costing"),
	0
} ;


/*************** default values for placement control ***************/

#define DEFAULT_STATS_FLAG			0	/* do not display statistics */
#define DEFAULT_SORT_FLAG			1	/* sort cluster tree */
#define DEFAULT_NET_BALANCE_FLAG	0	/* no net balance */
#define DEFAULT_NET_BALANCE_LIMIT	2	/* movement limit */
#define DEFAULT_VERTICAL_COST		2	/* vertical cost multiplier */


/************************* File variables *************************/

static SCCLUSTER     *sc_gcluster;		/* global list of cluster groups */
static SCCLUSTERTREE *sc_gcluster_tree;	/* global root of cluster tree */
static int            sc_currentcost;	/* cost of current cluster tree */

static SCPLACECONTROL sc_placecontrol =
{
	DEFAULT_STATS_FLAG,
	DEFAULT_SORT_FLAG,
	DEFAULT_NET_BALANCE_FLAG,
	DEFAULT_NET_BALANCE_LIMIT,
	DEFAULT_VERTICAL_COST
};

/************************* External variables *************************/

extern jmp_buf sc_please_stop;
extern SCCELL *sc_curcell;

/* prototypes for local routines */
static int  Sc_place_set_control(int, CHAR*[]);
static void Sc_place_show_control(void);
static int  Sc_place_create_clusters(SCCLUSTER**, SCCELL*);
static void Sc_place_total_cell_size(SCNITREE*, int*, int*, int*);
static int  Sc_place_create_cluster_tree(SCCLUSTERTREE**, SCCLUSTER*[], int, SCCELL*);
static int  Sc_place_create_ctree_recurse(SCCLUSTERTREE**, SCCLUSTERTREE*, SCCELL*);
static void Sc_place_ctree_add_parents(SCCLUSTERTREE*, SCCLUSTERTREE*);
static int  Sc_place_ctree_num_connects(SCCLUSTERTREE*, SCCLCONNECT**, SCCELL*);
static void Sc_clear_ext_nodes(SCEXTNODE*);
static void Sc_set_ext_nodes_by_ctree(SCCLUSTERTREE*, int);
static int  Sc_count_ext_nodes(SCCLUSTERTREE*, int);
static int  Sc_place_ctree_sort_connects(SCCLCONNECT*, SCCLCONNECT**);
static int  Sc_place_ctree_pair(SCCLUSTERTREE*, SCCLCONNECT*, SCCLUSTERTREE**);
static int  Sc_place_best_pair(SCCLCONNECT*, SCCLCONNECT**);
static void Sc_place_print_cluster_tree(SCCLUSTERTREE*, int);
static int  Sc_place_sort_cluster_tree(SCCLUSTERTREE*, SCCELL*);
static int  Sc_place_sort_swapper_top_down(SCCLUSTERTREE*, SCNBTRUNK*, SCCELL*);
static int  Sc_place_sort_swapper_bottom_up(SCCLUSTERTREE*, SCNBTRUNK*, SCCELL*);
static void Sc_place_switch_subtrees(SCCLUSTERTREE*);
static int  Sc_place_cost_cluster_tree(SCCLUSTERTREE*, SCNBTRUNK*, SCCELL*);
static void Sc_place_cost_cluster_tree_2(SCCLUSTERTREE*, SCNBTRUNK*, int*);
static int  Sc_place_create_placelist(SCCLUSTERTREE*, SCCELL*);
static int  Sc_place_add_cluster_to_row(SCCLUSTER*, SCROWLIST*, SCPLACE*);
static void Sc_free_cluster_tree(SCCLUSTERTREE*);
static int  Sc_place_net_balance(SCROWLIST*, SCCELL*);
static int  Sc_place_nb_all_cells(SCCELL*, SCCHANNEL*);
static void Sc_place_nb_do_cell(SCNBPLACE*, SCCHANNEL*, SCCELL*);
static int  Sc_place_nb_cost(SCROWLIST*, SCCHANNEL*, SCCELL*);
static void Sc_place_nb_remove(SCNBPLACE*, SCROWLIST*);
static void Sc_place_nb_insert_before(SCNBPLACE*, SCNBPLACE*, SCROWLIST*);
static void Sc_place_nb_insert_after(SCNBPLACE*, SCNBPLACE*, SCROWLIST*);
static void Sc_place_nb_rebalance_rows(SCROWLIST*, SCPLACE*);
static void Sc_place_number_placement(SCROWLIST*);
static void Sc_place_show_placement(SCROWLIST*);
static void Sc_place_reorder_rows(SCROWLIST*);

/***********************************************************************
Module:  Sc_place
------------------------------------------------------------------------
Description:
	Place the nodes in the current cell in optimal position for routing
	based upon number of connections between cells.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointers to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place(int count, CHAR *pars[])
{
	int			err, l, numcl, i;
	SCPLACE		*place;
	SCROWLIST		*row;
	SCCLUSTER		*cl, **cllist;

	/* parameter check */
	if (count)
	{
		l = estrlen(pars[0]);
		l = maxi(l, 2);
		if (namesamen(pars[0], x_("set-control"), l) == 0)
			return(Sc_place_set_control(count - 1, &pars[1]));
		if (namesamen(pars[0], x_("show-control"), l) == 0)
		{
			Sc_place_show_control();
			return(SC_NOERROR);
		}
		return(Sc_seterrmsg(SC_PLACE_XCMD, pars[0]));
	}

	/* check to see if currently working in a cell */
	if (sc_curcell == NULL)
		return(Sc_seterrmsg(SC_NOCELL));

	/* create placement structure */
	(void)Sc_free_placement(sc_curcell->placement);
	place = (SCPLACE *)emalloc(sizeof(SCPLACE), sc_tool->cluster);
	if (place == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	sc_curcell->placement = place;
	place->num_inst = 0;
	place->size_inst = 0;
	place->avg_size = 0;
	place->avg_height = 0;
	place->num_rows = ScGetParameter(SC_PARAM_PLACE_NUM_ROWS);
	place->size_rows = 0;
	place->rows = NULL;
	place->plist = NULL;
	place->endlist = NULL;

	/* DO PLACEMENT */
	for (i = 0; sc_placeblerb[i]; i++)
		ttyputverbose(TRANSLATE(sc_placeblerb[i]));
	if (sc_placecontrol.stats_flag)
		Sc_place_show_control();
	ttyputmsg(_("Starting PLACEMENT..."));
	(void)Sc_cpu_time(TIME_RESET);

	/* create clusters of cells */
	sc_gcluster = NULL;
	if ((err = Sc_place_create_clusters(&sc_gcluster, sc_curcell)))
		return(err);

	if (sc_gcluster == NULL)
	{
		ttyputmsg(_("ERROR - No cells found to place.  Aborting."));
		return(SC_NOERROR);
	}

	/* place clusters in a binary group tree */
	sc_gcluster_tree = NULL;
	numcl = 0;
	for (cl = sc_gcluster; cl; cl = cl->next)
		numcl++;
	cllist = (SCCLUSTER **)emalloc(sizeof(SCCLUSTER *) * (numcl + 1), sc_tool->cluster);
	if (cllist == NULL)
		return(Sc_seterrmsg(SC_NOMEMORY));
	i = 0;
	for (cl = sc_gcluster; cl; cl = cl->next)
		cllist[i++] = cl;
	cllist[i] = NULL;
	if ((err = Sc_place_create_cluster_tree(&sc_gcluster_tree, cllist, numcl,
		sc_curcell)))
			return(err);
	efree((CHAR *)cllist);
	ttyputverbose(M_("    Time to create Cluster Tree  =  %s."),
		Sc_cpu_time(TIME_REL));
	if (sc_placecontrol.stats_flag)
	{
		ttyputmsg(M_("************ Initial placement of Clusters"));
		Sc_place_print_cluster_tree(sc_gcluster_tree, 0);
	}
	place->size_rows = place->size_inst / place->num_rows;

	/* place clusters in list by sorting groups */
	if (sc_placecontrol.sort_flag)
	{
		if ((err = Sc_place_sort_cluster_tree(sc_gcluster_tree, sc_curcell)))
			return(err);
		if (sc_placecontrol.stats_flag)
		{
			ttyputmsg(M_("************ Placement of Clusters after Sorting"));
			Sc_place_print_cluster_tree(sc_gcluster_tree, 0);
		}
	}

	/* create first row structure */
	row = (SCROWLIST *)emalloc(sizeof(SCROWLIST), sc_tool->cluster);
	if (row == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	row->start = NULL;
	row->end = NULL;
	row->row_num = 0;
	row->row_size = 0;
	row->next = NULL;
	row->last = NULL;
	sc_curcell->placement->rows = row;

	/* create cell placement list from sorted cluster list */
	if ((err = Sc_place_create_placelist(sc_gcluster_tree, sc_curcell)))
		return(err);

	/* number placement */
	Sc_place_number_placement(sc_curcell->placement->rows);
	if (sc_placecontrol.stats_flag)
	{
		ttyputmsg(M_("************ Placement before Net Balancing"));
		Sc_place_show_placement(sc_curcell->placement->rows);
	}
	Sc_free_cluster_tree(sc_gcluster_tree);

	/* do net balance algorithm */
	if (sc_placecontrol.net_balance_flag)
	{
		if ((err = Sc_place_net_balance(sc_curcell->placement->rows,
			sc_curcell)))
				return(err);
		ttyputmsg(M_("    Time to do Net Balancing =  %s."),
			Sc_cpu_time(TIME_REL));
		if (sc_placecontrol.stats_flag)
		{
			ttyputmsg(M_("************ Placement after Net Balancing"));
			Sc_place_show_placement(sc_curcell->placement->rows);
		}
	}

	/* print process time for placement */
	Sc_place_reorder_rows(sc_curcell->placement->rows);
	ttyputmsg(_("Done (time = %s)"), Sc_cpu_time(TIME_ABS));

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_set_control
------------------------------------------------------------------------
Description:
	Set the placement control structure values.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_set_control(count, pars);

Name		Type		Description
----		----		-----------
count		int			Number of parameters.
pars		*char[]		Array of pointers to parameters.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_set_control(int count, CHAR *pars[])
{
	int		numcmd, l, n;

	if (count)
	{
		for (numcmd = 0; numcmd < count; numcmd++)
		{
			l = estrlen(pars[numcmd]);
			n = maxi(l, 2);
			if (namesamen(pars[numcmd], x_("sort"), n) == 0)
			{
				sc_placecontrol.sort_flag = TRUE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("rows"), n) == 0)
			{
				numcmd++;
				if (numcmd < count)
					ScSetParameter(SC_PARAM_PLACE_NUM_ROWS, eatoi(pars[numcmd]));
				continue;
			}

			if (namesamen(pars[numcmd], x_("net-balance"), n) == 0)
			{
				sc_placecontrol.net_balance_flag = TRUE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("limit-net-balance"), n) == 0)
			{
				numcmd++;
				if (numcmd < count)
					sc_placecontrol.net_balance_limit = eatoi(pars[numcmd]);
				continue;
			}
			if (namesamen(pars[numcmd], x_("default"), n) == 0)
			{
				sc_placecontrol.stats_flag = DEFAULT_STATS_FLAG;
				sc_placecontrol.sort_flag = DEFAULT_SORT_FLAG;
				ScSetParameter(SC_PARAM_PLACE_NUM_ROWS, DEFAULT_NUM_OF_ROWS);
				sc_placecontrol.net_balance_flag = DEFAULT_NET_BALANCE_FLAG;
				sc_placecontrol.net_balance_limit = DEFAULT_NET_BALANCE_LIMIT;
				sc_placecontrol.vertical_cost = DEFAULT_VERTICAL_COST;
				continue;
			}
			n = maxi(l, 4);
			if (namesamen(pars[numcmd], x_("verbose"), n) == 0)
			{
				sc_placecontrol.stats_flag = TRUE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("vertical-cost"), n) == 0)
			{
				numcmd++;
				if (numcmd < count)
					sc_placecontrol.vertical_cost = eatoi(pars[numcmd]);
				continue;
			}
			n = maxi(l, 5);
			if (namesamen(pars[numcmd], x_("no-verbose"), n) == 0)
			{
				sc_placecontrol.stats_flag = FALSE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("no-sort"), n) == 0)
			{
				sc_placecontrol.sort_flag = FALSE;
				continue;
			}
			if (namesamen(pars[numcmd], x_("no-net-balance"), n) == 0)
			{
				sc_placecontrol.net_balance_flag = FALSE;
				continue;
			}
			return(Sc_seterrmsg(SC_PLACE_SET_XCMD, pars[numcmd]));
		}
	} else
	{
		return(Sc_seterrmsg(SC_PLACE_SET_NOCMD));
	}

	Sc_place_show_control();
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_show_control
------------------------------------------------------------------------
Description:
	Print the placement control structure values.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_show_control();
------------------------------------------------------------------------
*/

void Sc_place_show_control(void)
{
	CHAR	*str1;

	ttyputverbose(M_("************ PLACEMENT CONTROL STRUCTURE"));
	if (sc_placecontrol.stats_flag)
	{
		str1 = M_("TRUE");
	} else
	{
		str1 = M_("FALSE");
	}
	ttyputverbose(M_("Print statistics  =  %-s"), str1);
	if (sc_placecontrol.sort_flag)
	{
		str1 = M_("TRUE");
	} else
	{
		str1 = M_("FALSE");
	}
	ttyputverbose(M_("Sort cluster tree =  %-s"), str1);
	ttyputverbose(M_("Number of rows    =  %d"), ScGetParameter(SC_PARAM_PLACE_NUM_ROWS));
	if (sc_placecontrol.net_balance_flag)
	{
		str1 = M_("TRUE");
	} else
	{
		str1 = M_("FALSE");
	}
	ttyputverbose(M_("Net balancing     =  %-s"), str1);
	ttyputverbose(M_("Net balace limit  =  %d"),
		sc_placecontrol.net_balance_limit);
	ttyputverbose(M_("Vertical cost X   =  %d"),
		sc_placecontrol.vertical_cost);
}

/***********************************************************************
Module:  Sc_place_create_clusters
------------------------------------------------------------------------
Description:
	Create "clusters" of cells of size one.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_create_clusters(clusters, cell);

Name		Type		Description
----		----		-----------
clusters	**SCCLUSTER	Address of where to write start of list.
cell		*SCCELL		Pointer to complex cell.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_create_clusters(SCCLUSTER **clusters, SCCELL *cell)
{
	int		num, size, height;
	int		i, avg_size, avg_height, warn;
	SCCLUSTER	*cluster, *clusterlist;
	SCNITREE	*node;

	/* find total 'size' and number of all the cells */
	size = 0;  num = 0;
	Sc_place_total_cell_size(cell->nilist, &num, &size, &height);
	if (!num)
	{
		ttyputmsg(_("WARNING - No leaf cells found for placement."));
		return(SC_NOERROR);
	}
	avg_size = size / num;
	avg_height = height / num;
	if (sc_placecontrol.stats_flag)
	{
		ttyputmsg(M_("************ Cell Statistics"));
		ttyputmsg(M_("    Number of cells         = %d"), num);
		ttyputmsg(M_("    Total length            = %d"), size);
		ttyputmsg(M_("    Average size of cells   = %d"), avg_size);
		ttyputmsg(M_("    Average height of cells = %d"), avg_height);
	}
	cell->placement->num_inst = num;
	cell->placement->size_inst = size;
	cell->placement->avg_size = avg_size;
	cell->placement->avg_height = avg_height;

	/* create cluster list */
	i = 0;
	clusterlist = NULL;
	warn = FALSE;
	for (node = cell->nilist; node; node = node->next)
	{
		if (node->type != SCLEAFCELL)
		{
			if (node->type == SCCOMPLEXCELL)
				warn = TRUE;
			continue;
		}
		cluster = (SCCLUSTER *)emalloc(sizeof(SCCLUSTER), sc_tool->cluster);
		if (cluster == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		cluster->node = node;
		cluster->size = node->size;
		cluster->number = i++;
		cluster->last = NULL;
		cluster->next = clusterlist;
		if (clusterlist)
			clusterlist->last = cluster;
		clusterlist = cluster;
	}
	if (warn)
	{
		ttyputmsg(_("WARNING - At least one complex cell found during Create_Clusters."));
		ttyputmsg(_("        - Probable cause:  Forgot to do 'PULL' command."));
	}

	*clusters = clusterlist;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_total_cell_size
------------------------------------------------------------------------
Description:
	Determine the number of cells and total linear size of cells.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_tottal_cell_size(ilist, num, width, height);

Name		Type		Description
----		----		-----------
ilist		*SCNITREE	Pointer to start of instance list.
num			*int		Address to write number of cells.
width		*int		Address to write total width of cells.
height		*int		address to write total height of cells.
------------------------------------------------------------------------
*/

void Sc_place_total_cell_size(SCNITREE *ilist, int *num, int *width, int *height)
{
	*num = 0;
	*width = 0;
	*height = 0;
	for ( ; ilist; ilist = ilist->next)
	{
		if (ilist->type == SCLEAFCELL)
		{
			(*num)++;
			*width += ilist->size;
			*height += Sc_leaf_cell_ysize(ilist->np);
		}
	}
	return;
}

/***********************************************************************
Module:  Sc_place_create_cluster_tree
------------------------------------------------------------------------
Description:
	Recursively create the cluster tree from a group of clusters.  At
	each "node" in the tree, the goal is to pair groups of clusters
	which are strongly connected together.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_create_cluster_tree(ctree, clusters, size,
						cell);

Name		Type			Description
----		----			-----------
ctree		**SCCLUSTERTREE	Address of pointer to cluster tree node.
clusters	*SCCLUSTER[]	Array of pointer to the clusters.
size		int				Size of array (i.e. number of clusters).
cell		*SCCELL			Pointer to cell being placed.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_create_cluster_tree(SCCLUSTERTREE **ctree, SCCLUSTER *clusters[],
	int size, SCCELL *cell)
{
	int			i, err;
	SCCLUSTERTREE	*node, *nstart;

	/* create a cluster tree node for each cluster */
	nstart = NULL;
	for (i = 0; i < size; i++)
	{
		node = (SCCLUSTERTREE *)emalloc(sizeof(SCCLUSTERTREE), sc_tool->cluster);
		if (node == 0)
		{
			return(Sc_seterrmsg(SC_NOMEMORY));
		}
		node->cluster = clusters[i];
		node->parent = NULL;
		node->next = nstart;
		nstart = node;
		node->lptr = NULL;
		node->rptr = NULL;
	}

	/* recursively create cluster tree */
	if ((err = Sc_place_create_ctree_recurse(ctree, nstart, cell)))
		return(err);
	Sc_place_ctree_add_parents(*ctree, (SCCLUSTERTREE *)NULL);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_create_ctree_recurse
------------------------------------------------------------------------
Description:
	Recursively create the cluster tree from the bottom up by pairing
	strongly connected tree nodes together.  When only one tree node
	exists, this is the root and can be written to the indicated
	address.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_create_ctree_recurse(aroot, nodes, cell);

Name		Type			Description
----		----			-----------
aroot		**SCCLUSTERTREE	Address of where to write the tree root.
nodes		*SCCLUSTERTREE	Pointer to start of tree nodes.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_create_ctree_recurse(SCCLUSTERTREE **aroot, SCCLUSTERTREE *nodes,
	SCCELL *cell)
{
	int			err;
	SCCLUSTERTREE	*nstart;
	SCCLCONNECT		*nconnects;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	/* if no node, end */
	if (nodes == NULL)
		return(SC_NOERROR);

	/* if one node, write to root and end */
	if (nodes->next == NULL)
	{
		*aroot = nodes;
		return(SC_NOERROR);
	}

	/* pair nodes in groups */
	/* create list of connections between nodes */
	if ((err = Sc_place_ctree_num_connects(nodes, &nconnects, cell)))
		return(err);

	/* sort number of connects from largest to smallest */
	if ((err = Sc_place_ctree_sort_connects(nconnects, &nconnects)))
		return(err);

	/* pair by number of connects */
	if ((err = Sc_place_ctree_pair(nodes, nconnects, &nstart)))
		return(err);

	/* recurse up a level */
	return(Sc_place_create_ctree_recurse(aroot, nstart, cell));
}

/***********************************************************************
Module:  Sc_place_ctree_add_parents
________________________________________________________________________
Description:
	Add the parent pointer to the cluster tree by doing a preorder
	transversal.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_ctree_add_parents(node, parent);

Name		Type			Description
----		----			-----------
node		*SCCLUSTERTREE	Pointer to current node in transversal.
parent		*SCCLUSTERTREE	Pointer to parent node.
------------------------------------------------------------------------
*/

void Sc_place_ctree_add_parents(SCCLUSTERTREE *node, SCCLUSTERTREE *parent)
{
	if (node == NULL) return;
	node->parent = parent;
	Sc_place_ctree_add_parents(node->lptr, node);
	Sc_place_ctree_add_parents(node->rptr, node);
}

/***********************************************************************
Module:  Sc_place_ctree_num_connects
------------------------------------------------------------------------
Description:
	Create a list of the number of connections from all groups to all
	other groups.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_ctree_num_connects(nodes, clist, cell);

Name		Type			Description
----		----			-----------
nodes		*SCCLUSTERTREE	List of current nodes.
clist		**clist			Address of where to write resultant list.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_ctree_num_connects(SCCLUSTERTREE *nodes, SCCLCONNECT **clist, SCCELL *cell)
{
	SCCLUSTERTREE	*nextnode;
	int			common, node_num;
	SCCLCONNECT		*newcon, *start, *end;

	start = end = NULL;
	node_num = 0;

	/* clear flags on all extracted nodes */
	Sc_clear_ext_nodes(cell->ex_nodes);

	/* go through list of nodes */
	for ( ; nodes; nodes = nodes->next)
	{
		/* check all other node */
		for (nextnode = nodes->next; nextnode; nextnode = nextnode->next)
		{
			node_num += 2;

			/* mark all extracted nodes used by first node */
			Sc_set_ext_nodes_by_ctree(nodes, node_num);

			/* count number of common extracted nodes */
			common = Sc_count_ext_nodes(nextnode, node_num);

			if (common)
			{
				newcon = (SCCLCONNECT *)emalloc(sizeof(SCCLCONNECT), sc_tool->cluster);
				if (newcon == NULL)
					return(Sc_seterrmsg(SC_NOMEMORY));
				newcon->node[0] = nodes;
				newcon->node[1] = nextnode;
				newcon->count = common;
				newcon->last = end;
				newcon->next = NULL;
				if (end)
				{
					end->next = newcon;
				} else
				{
					start = newcon;
				}
				end = newcon;
			}
		}
	}

	*clist = start;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_clear_ext_nodes
------------------------------------------------------------------------
Description:
	Set the flags field of all extracted nodes to clear.
------------------------------------------------------------------------
Calling Sequence:  Sc_clear_ext_nodes(nodes);

Name		Type		Description
----		----		-----------
nodes		*SCEXTNODE	Start of list of extracted nodes.
------------------------------------------------------------------------
*/

void Sc_clear_ext_nodes(SCEXTNODE *nodes)
{
	for ( ; nodes; nodes = nodes->next)
		nodes->flags &= ~SCEXTNODECLUSE;
}

/***********************************************************************
Module:  Sc_set_ext_nodes_by_ctree
------------------------------------------------------------------------
Description:
	Mark all extracted nodes references by any member of all the
	clusters in the indicated cluster tree.
------------------------------------------------------------------------
Calling Sequence:  Sc_set_ext_nodes_by_ctree(node, marker);

Name		Type			Description
----		----			-----------
node		*SCCLUSTERTREE	Pointer to cluster tree node.
marker		int				Value to set flags field to.
------------------------------------------------------------------------
*/

void Sc_set_ext_nodes_by_ctree(SCCLUSTERTREE *node, int marker)
{
	SCNIPORT		*port;

	if (node == NULL) return;

	Sc_set_ext_nodes_by_ctree(node->lptr, marker);

	/* process node if cluster */
	if (node->cluster)
	{
		/* check every port of member */
		for (port = node->cluster->node->ports; port; port = port->next)
			port->ext_node->flags = marker;
	}

	Sc_set_ext_nodes_by_ctree(node->rptr, marker);
}

/***********************************************************************
Module:  Sc_count_ext_nodes
------------------------------------------------------------------------
Description:
	Return the number of extracted nodes which have flag bit set only
	and is accessed by subtree.
------------------------------------------------------------------------
Calling Sequence:  count = Sc_count_ext_nodes(node, marker);

Name		Type			Description
----		----			-----------
node		*SCCLUSTERTREE	Start of cluster tree node.
marker		int				Value to look for.
------------------------------------------------------------------------
*/

int Sc_count_ext_nodes(SCCLUSTERTREE *node, int marker)
{
	SCNIPORT		*port;
	int			count;

	if (node == NULL)
		return(0);

	count = Sc_count_ext_nodes(node->lptr, marker);

	/* process node if cluster */
	if (node->cluster)
	{
		/* check every port of member */
		for (port = node->cluster->node->ports; port; port = port->next)
		{
			if (port->ext_node->flags == marker)
			{
				count++;
				port->ext_node->flags |= SCEXTNODEGROUP1;
			}
		}
	}

	count += Sc_count_ext_nodes(node->rptr, marker);

	return(count);
}

/***********************************************************************
Module:  Sc_place_ctree_sort_connects
------------------------------------------------------------------------
Description:
	Sort the passed list on number of connections from largest to
	smallest.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_ctree_sort_connects(list, gstart);

Name		Type			Description
----		----			-----------
list		*SCCLCONNECT	Pointer to start of connection list.
gstart		**SCCLCONNECT	Address of where to write new start.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_ctree_sort_connects(SCCLCONNECT *list, SCCLCONNECT **gstart)
{
	SCCLCONNECT	*pold, *pp, *plast;

	/* order placement list highest to lowest */
	if (list)
	{
		pold = list;
		for (pp = list->next; pp;)
		{
			if (pp->count > pold->count)
			{
				pold->next = pp->next;
				if (pp->next)
					pp->next->last = pold;
				for (plast = pold->last; plast; plast = plast->last)
				{
					if (plast->count >= pp->count)
						break;
				}
				if (!plast)
				{
					pp->next = list;
					list->last = pp;
					list = pp;
					pp->last = NULL;
				} else
				{
					pp->next = plast->next;
					pp->next->last = pp;
					pp->last = plast;
					plast->next = pp;
				}
				pp = pold->next;
			} else
			{
				pold = pp;
				pp = pp->next;
			}
		}
	}

	*gstart = list;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_ctree_pair
------------------------------------------------------------------------
Description:
	Pair up the given nodes by using the information in the connection
	list.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_ctree_pair(nodes, nconnects, nstart);

Name		Type			Description
----		----			-----------
nodes		*SCCLUSTERTREE	Pointer to start of list of nodes.
nconnects	*SCCLCONNECT	Pointer to start of list of connections.
nstart		**SCCLUSTERTREE	Address of where to write new list.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_ctree_pair(SCCLUSTERTREE *nodes, SCCLCONNECT *nconnects,
	SCCLUSTERTREE **nstart)
{
	SCCLUSTERTREE	*tptr, *newstart, *newnode;
	SCCLCONNECT		*connect, *bconnect, *nconnect;
	int				err;

	/* clear the placed flag in all tree nodes */
	for (tptr = nodes; tptr; tptr = tptr->next)
		tptr->bits &= ~SCBITS_PLACED;

	/* go through connection list */
	newstart = NULL;
	for (connect = nconnects; connect;)
	{
		/* if either placed, continue */
		if (connect->node[0]->bits & SCBITS_PLACED ||
			connect->node[1]->bits & SCBITS_PLACED)
		{
			connect = connect->next;
			continue;
		}

		/* get best choice */
		if ((err = Sc_place_best_pair(connect, &bconnect)))
			return(err);

		/* create new cluster tree node */
		newnode = (SCCLUSTERTREE *)emalloc(sizeof(SCCLUSTERTREE), sc_tool->cluster);
		if (newnode == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		newnode->cluster = NULL;
		newnode->bits = 0;
		newnode->parent = NULL;
		newnode->lptr = bconnect->node[0];
		newnode->lptr->parent = newnode;
		bconnect->node[0]->bits |= SCBITS_PLACED;
		newnode->rptr = bconnect->node[1];
		newnode->rptr->parent = newnode;
		bconnect->node[1]->bits |= SCBITS_PLACED;
		newnode->next = newstart;
		newstart = newnode;

		/* remove from list */
		if (connect == bconnect)
		{
			nconnect = connect->next;
			efree((CHAR *)connect);
			connect = nconnect;
		} else
		{
			bconnect->last->next = bconnect->next;
			if (bconnect->next)
				bconnect->next->last = bconnect->last;
			efree((CHAR *)bconnect);
		}
	}

	/* if no connections, arbitrarily combine two clusters */
	if (!nconnects)
	{
		/* create new cluster tree node */
		newnode = (SCCLUSTERTREE *)emalloc(sizeof(SCCLUSTERTREE), sc_tool->cluster);
		if (newnode == 0)
		{
			return(Sc_seterrmsg(SC_NOMEMORY));
		}
		newnode->cluster = NULL;
		newnode->bits = 0;
		newnode->parent = NULL;
		newnode->lptr = nodes;
		newnode->lptr->parent = newnode;
		nodes->bits |= SCBITS_PLACED;
		newnode->rptr = nodes->next;
		newnode->rptr->parent = newnode;
		nodes->next->bits |= SCBITS_PLACED;
		newnode->next = newstart;
		newstart = newnode;
	}

	/* add any remaining tree nodes as singular nodes */
	for (tptr = nodes; tptr; tptr = tptr->next)
	{
		if (!(tptr->bits & SCBITS_PLACED))
		{
			/* create new cluster tree node */
			newnode = (SCCLUSTERTREE *)emalloc(sizeof(SCCLUSTERTREE), sc_tool->cluster);
			if (newnode == 0)
			{
				return(Sc_seterrmsg(SC_NOMEMORY));
			}
			newnode->cluster = NULL;
			newnode->bits = 0;
			newnode->parent = NULL;
			newnode->lptr = tptr;
			newnode->lptr->parent = newnode;
			tptr->bits |= SCBITS_PLACED;
			newnode->rptr = NULL;
			newnode->next = newstart;
			newstart = newnode;
		}
	}

	*nstart = newstart;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_best_pair
------------------------------------------------------------------------
Description:
	Return a pointer to the cluster connection list which has both
	members unplaced, has the same weight as the one top on the list,
	and appears the smallest number of times.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_best_pair(connect, bconnect);

Name		Type			Description
----		----			-----------
connect		*SCCLCONNECT	Start of sorted list.
bconnect	**SCCLCONNECT	Address to write pointer to best pair.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_best_pair(SCCLCONNECT *connect, SCCLCONNECT **bconnect)
{
	SCCLCONNECT		*nconnect;
	struct Itemp
	{
		struct Iscclustertree	*node;		/* cluster tree node */
		int			count;		/* number of times seen */
		struct Iscclconnect	*ref;		/* first reference */
		struct Itemp		*next;		/* next in list */
	} *nlist, *slist, *blist;
	int			minuse;

	slist = NULL;
	for (nconnect = connect; nconnect; nconnect = nconnect->next)
	{
		if (nconnect->count < connect->count)
			break;
		if (nconnect->node[0]->bits & SCBITS_PLACED ||
			nconnect->node[1]->bits & SCBITS_PLACED)
				continue;

		/* check if nodes previously counted */
		for (nlist = slist; nlist; nlist = nlist->next)
		{
			if (nlist->node == nconnect->node[0])
				break;
		}
		if (nlist)
		{
			nlist->count++;
		} else
		{
			nlist = (struct Itemp *)emalloc(sizeof(struct Itemp), sc_tool->cluster);
			if (nlist == NULL)
				return(Sc_seterrmsg(SC_NOMEMORY));
			nlist->node = nconnect->node[0];
			nlist->count = 1;
			nlist->ref = nconnect;
			nlist->next = slist;
			slist = nlist;
		}
		for (nlist = slist; nlist; nlist = nlist->next)
		{
			if (nlist->node == nconnect->node[1])
				break;
		}
		if (nlist)
		{
			nlist->count++;
		} else
		{
			nlist = (struct Itemp *)emalloc(sizeof(struct Itemp), sc_tool->cluster);
			if (nlist == NULL)
				return(Sc_seterrmsg(SC_NOMEMORY));
			nlist->node = nconnect->node[1];
			nlist->count = 1;
			nlist->ref = nconnect;
			nlist->next = slist;
			slist = nlist;
		}
	}

	/* find the minimum count */
	minuse = slist->count;
	blist = slist;
	for (nlist = slist->next; nlist; nlist = nlist->next)
	{
		if (nlist->count < minuse)
		{
			minuse = nlist->count;
			blist = nlist;
		}
	}

	*bconnect = blist->ref;
	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_print_cluster_tree
------------------------------------------------------------------------
Description:
	Print the cluster placement tree by doing an inorder transversal.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_print_cluster_tree(node, level);

Name		Type			Description
----		----			-----------
node		*SCCLUSTERTREE	Pointer to cluster tree node.
level		int				Current level of tree (0 = root).
------------------------------------------------------------------------
*/

void Sc_place_print_cluster_tree(SCCLUSTERTREE *node, int level)
{
	static CHAR		spacer[80], trailer[80];
	int			i;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (node == NULL)
		return;

	Sc_place_print_cluster_tree(node->lptr, level + 1);

	/* process node */
	i = level << 2;
	spacer[i] = 0;
	while (i)
		spacer[--i] = ' ';
	i = 36 - (level << 2);
	if (i > 0)
	{
		trailer[i] = 0;
		while (i)
			trailer[--i] = '-';
	} else
	{
		trailer[0] = 0;
	}
	if (node->cluster)
	{
		ttyputmsg(_("%sCell %s"), spacer, node->cluster->node->name);
	} else
	{
		ttyputmsg(x_("%s%-2d**%s"), spacer, level, trailer);
	}

	Sc_place_print_cluster_tree(node->rptr, level + 1);
}

/***********************************************************************
Module:  Sc_place_sort_cluster_tree
------------------------------------------------------------------------
Description:
	Sort the cluster tree into a list by sorting groups.
	Sorting attempts to optimize the placement of groups by
	minimizing length of connections between groups and locating groups
	close to any specified ports.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_sort_cluster_tree(ctree, cell);

Name		Type			Description
----		----			-----------
ctree		*SCCLUSTERTREE	Pointer to root of cluster tree.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_sort_cluster_tree(SCCLUSTERTREE *ctree, SCCELL *cell)
{
	SCNBTRUNK	*trunks, *newtrunk, *nexttrunk;
	SCEXTNODE	*enode;
	int		err;

	trunks = NULL;

	/* create a list of trunks from the extracted nodes */
	for (enode = cell->ex_nodes; enode; enode = enode->next)
	{
		newtrunk = (SCNBTRUNK *)emalloc(sizeof(SCNBTRUNK), sc_tool->cluster);
		if (newtrunk == NULL)
			return(Sc_seterrmsg(SC_NOMEMORY));
		newtrunk->ext_node = enode;
		newtrunk->minx = 0;
		newtrunk->maxx = 0;
		newtrunk->next = trunks;
		trunks = newtrunk;
		enode->ptr = (CHAR *)newtrunk;		/* back reference pointer */
	}

	sc_currentcost = Sc_place_cost_cluster_tree(sc_gcluster_tree, trunks, cell);
	if (sc_placecontrol.stats_flag)
		ttyputverbose(M_("***** Cost of placement before cluster sorting = %d"),
			sc_currentcost);

	/* call top-down swapper */
	if ((err = Sc_place_sort_swapper_top_down(ctree, trunks, cell)))
		return(err);
	ttyputverbose(M_("    Time for Top-Down Sort       =  %s."),
		Sc_cpu_time(TIME_REL));

	/* call bottom-up swapper */
	if ((err = Sc_place_sort_swapper_bottom_up(ctree, trunks, cell)))
		return(err);
	ttyputverbose(M_("    Time for Bottom_Up Sort      =  %s."),
		Sc_cpu_time(TIME_REL));

	if (sc_placecontrol.stats_flag)
		ttyputverbose(M_("***** Cost of placement after cluster sorting = %d"),
			sc_currentcost);

	for (newtrunk = trunks; newtrunk; newtrunk = nexttrunk)
	{
		nexttrunk = newtrunk->next;
		efree((CHAR *)newtrunk);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_sort_swapper_top_down
------------------------------------------------------------------------
Description:
	Do preorder transversal of cluster tree, swapping groups to try
	and sort tree into a more efficient placement.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_sort_swapper_top_down(ctree, trunks, cell);

Name		Type			Description
----		----			-----------
ctree		*SCCLUSTERTREE	Root of cluster tree.
trunks		*SCNBTRUCK		List of trunks for costing.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_sort_swapper_top_down(SCCLUSTERTREE *ctree, SCNBTRUNK *trunks, SCCELL *cell)
{
	int			err, cost2;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (ctree == NULL)
		return(SC_NOERROR);

	/* process tree node if there are two subtrees */
	if (ctree->lptr && ctree->rptr)
	{
		/* swap groups */
		Sc_place_switch_subtrees(ctree);

		/* check new cost */
		cost2 = Sc_place_cost_cluster_tree(sc_gcluster_tree, trunks, cell);

		/* swap back if old cost is less than new */
		if (sc_currentcost < cost2)
		{
			Sc_place_switch_subtrees(ctree);
		} else
		{
			sc_currentcost = cost2;
		}
	}

	if ((err = Sc_place_sort_swapper_top_down(ctree->lptr, trunks, cell)))
		return(err);

	if ((err = Sc_place_sort_swapper_top_down(ctree->rptr, trunks, cell)))
		return(err);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_sort_swapper_bottom_up
------------------------------------------------------------------------
Description:
	Do a postorder transversal of cluster tree, swapping groups to try
	and sort tree into a more efficient placement.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_sort_swapper_bottom_up(ctree, trunks, cell);

Name		Type			Description
----		----			-----------
ctree		*SCCLUSTERTREE	Root of cluster tree.
trunks		*SCNBTRUCK		List of trunks for costing.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_sort_swapper_bottom_up(SCCLUSTERTREE *ctree, SCNBTRUNK *trunks, SCCELL *cell)
{
	int			err, cost2;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (ctree == NULL)
		return(SC_NOERROR);

	if ((err = Sc_place_sort_swapper_bottom_up(ctree->lptr, trunks, cell)))
		return(err);

	if ((err = Sc_place_sort_swapper_bottom_up(ctree->rptr, trunks, cell)))
		return(err);

	/* process tree node if there are two subtrees */
	if (ctree->lptr && ctree->rptr)
	{
		/* swap groups */
		Sc_place_switch_subtrees(ctree);

		/* check new cost */
		cost2 = Sc_place_cost_cluster_tree(sc_gcluster_tree, trunks, cell);

		/* swap back if old cost is less than new */
		if (sc_currentcost < cost2)
		{
			Sc_place_switch_subtrees(ctree);
		} else
		{
			sc_currentcost = cost2;
		}
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_switch_subtrees
------------------------------------------------------------------------
Description:
	Switch the subtrees recursively to perform a mirror image operation
	along "main" axis.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_switch_subtrees(node);

Name		Type			Description
----		----			-----------
node		*SCCLUSTERTREE	Pointer to top tree node.
------------------------------------------------------------------------
*/

void Sc_place_switch_subtrees(SCCLUSTERTREE *node)
{
	SCCLUSTERTREE	*temp;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (node == NULL)
		return;

	temp = node->lptr;
	node->lptr = node->rptr;
	node->rptr = temp;
	Sc_place_switch_subtrees(node->lptr);
	Sc_place_switch_subtrees(node->rptr);
}

/***********************************************************************
Module:  Sc_place_cost_cluster_tree
------------------------------------------------------------------------
Description:
	Return the "cost" of the indicated cluster tree sort.  Cost is a
	function of the length of connections between clusters and placement
	to ports.
------------------------------------------------------------------------
Calling Sequence:  cost = Sc_place_cost_cluster_tree(ctree, trunks, cell);

Name		Type			Description
----		----			-----------
ctree		*SCCLUSTERTREE	Pointer to cluster tree node.
trunks		*SCNBTRUNK		Pointer to trunks to use to cost.
cell		*SCCELL			Pointer to parent cell.
cost		int				Returned cost.
------------------------------------------------------------------------
*/

int Sc_place_cost_cluster_tree(SCCLUSTERTREE *ctree, SCNBTRUNK *trunks, SCCELL *cell)
{
	int		cost, pos, row;
	SCNBTRUNK	*ntrunk;
	SCPORT	*pport;

	/* clear trunks to record lengths */
	for (ntrunk = trunks; ntrunk; ntrunk = ntrunk->next)
	{
		ntrunk->minx = -1;
		ntrunk->maxx = -1;
	}

	/* set trunks lengths */
	pos = 0;
	Sc_place_cost_cluster_tree_2(ctree, trunks, &pos);

	/* calculate cost */
	cost = 0;
	for (ntrunk = trunks; ntrunk; ntrunk = ntrunk->next)
	{
		if (ntrunk->minx < 0)
			continue;
		cost += ntrunk->maxx - ntrunk->minx;
	}

	for (pport = cell->ports; pport; pport = pport->next)
	{
		if (!(pport->bits & SCPORTDIRMASK))
			continue;
		ntrunk = (SCNBTRUNK *)pport->node->ports->ext_node->ptr;
		if (!ntrunk) continue;
		if (pport->bits & SCPORTDIRUP)
		{
			/* add distance to top row */
			row = ntrunk->maxx / cell->placement->size_rows;
			if ((row + 1) < cell->placement->num_rows)
			{
				cost += (cell->placement->num_rows - row - 1) *
					cell->placement->avg_height *
					sc_placecontrol.vertical_cost;
			}
		}
		if (pport->bits & SCPORTDIRDOWN)
		{
			/* add distance to bottom row */
			row = ntrunk->minx / cell->placement->size_rows;
			if (row)
			{
				cost += row * cell->placement->avg_height
					* sc_placecontrol.vertical_cost;
			}
		}
		if (pport->bits & SCPORTDIRLEFT)
		{
			/* EMPTY */ 
		}
		if (pport->bits & SCPORTDIRRIGHT)
		{
			/* EMPTY */ 
		}
	}

	return(cost);
}

/***********************************************************************
Module:  Sc_place_cost_cluster_tree_2
------------------------------------------------------------------------
Description:
	Set the limits of the trunks by doing an inorder transversal of
	the cluster tree.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_cost_cluster_tree_2(ctree, trunks, pos);

Name		Type			Description
----		----			-----------
ctree		*SCCLUSTERTREE	Pointer to cluster tree node.
trunks		*SCNBTRUNK		Pointer to trunks to use to cost.
pos			*int			Address of current position.
------------------------------------------------------------------------
*/

void Sc_place_cost_cluster_tree_2(SCCLUSTERTREE *ctree, SCNBTRUNK *trunks, int *pos)
{
	SCNBTRUNK		*ntrunk;
	SCNIPORT		*port;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (ctree == NULL)
		return;

	/* do all nodes left */
	Sc_place_cost_cluster_tree_2(ctree->lptr, trunks, pos);

	/* process node */
	if (ctree->cluster)
	{
		for (port = ctree->cluster->node->ports; port; port = port->next)
		{
			ntrunk = (SCNBTRUNK *)port->ext_node->ptr;
			if (!ntrunk) continue;
			if (ntrunk->minx < 0)
				ntrunk->minx = *pos + port->xpos;
			ntrunk->maxx = *pos + port->xpos;
		}
		*pos += ctree->cluster->size;
	}

	/* do all nodes right */
	Sc_place_cost_cluster_tree_2(ctree->rptr, trunks, pos);
}

/***********************************************************************
Module:  Sc_place_create_placelist
------------------------------------------------------------------------
Description:
	Create the placement list by simply taking the clusters from the
	sorted cluster list and placing members in a snake pattern.
	Do an inorder transversal to create placelist.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_create_placelist(ctree, cell);

Name		Type			Description
----		----			-----------
ctree		*SCCLUSTERTREE	Pointer to start of cluster tree.
cell		*SCCELL			Pointer to parent cell.
err			int				Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_create_placelist(SCCLUSTERTREE *ctree, SCCELL *cell)
{
	int			err;
	SCROWLIST		*row;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (ctree == NULL)
		return(SC_NOERROR);

	if ((err = Sc_place_create_placelist(ctree->lptr, cell)))
		return(err);

	/* add clusters to placement list */
	if (ctree->cluster)
	{
		row = cell->placement->rows;
		while (row->next)
			row = row->next;
		(void)Sc_place_add_cluster_to_row(ctree->cluster, row, cell->placement);
	}

	if ((err = Sc_place_create_placelist(ctree->rptr, cell)))
		return(err);

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_add_cluster_to_row
------------------------------------------------------------------------
Description:
	Add the members of the passed cluster to the indicated row.
	Add new rows as necessary and also maintain a global placement
	bidirectional list.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_add_cluster_to_row(cluster, row, place);

Name		Type		Description
----		----		-----------
cluster		*SCCLUSTER	Pointer to cluster to add.
row			*SCROWLIST	Pointer to the current row.
place		*SCPLACE	Pointer to placement information.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_add_cluster_to_row(SCCLUSTER *cluster, SCROWLIST *row, SCPLACE *place)
{
	SCNBPLACE		*newplace;
	SCROWLIST		*row2;
	int			old_condition, new_condition;

	if (cluster->node->type != SCLEAFCELL)
		return(SC_NOERROR);
	newplace = (SCNBPLACE *)emalloc(sizeof(SCNBPLACE), sc_tool->cluster);
	if (newplace == 0)
		return(Sc_seterrmsg(SC_NOMEMORY));
	newplace->cell = cluster->node;
	newplace->xpos = 0;
	cluster->node->tp = (CHAR *)newplace;
	newplace->next = NULL;
	newplace->last = place->endlist;
	if (place->endlist == NULL)
	{
		place->plist = place->endlist = newplace;
	} else
	{
		place->endlist->next = newplace;
		place->endlist = newplace;
	}
	old_condition = place->size_rows - row->row_size;
	new_condition = place->size_rows - (row->row_size + cluster->node->size);
	if ((row->row_num + 1) < place->num_rows &&
		abs(old_condition) < abs(new_condition))
	{
		row2 = (SCROWLIST *)emalloc(sizeof(SCROWLIST), sc_tool->cluster);
		if (row2 == 0)
			return(Sc_seterrmsg(SC_NOMEMORY));
		row2->start = NULL;
		row2->end = NULL;
		row2->row_num = row->row_num + 1;
		row2->row_size = 0;
		row2->next = NULL;
		row2->last = row;
		row->next = row2;
		row = row2;
	}

	/* add to row */
	if (row->row_num % 2)
	{
		/* odd row */
		if (row->end == NULL)
			row->end = newplace;
		row->start = newplace;
	} else
	{
		/* even row */
		if (row->start == NULL)
			row->start = newplace;
		row->end = newplace;
	}
	row->row_size += cluster->node->size;

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_free_cluster_tree
------------------------------------------------------------------------
Description:
	Do a post-order transversal, freeing up the members of the
	cluster tree.
------------------------------------------------------------------------
Calling Sequence:  Sc_free_cluster_tree(node);

Name		Type		Description
----		----		-----------
node		*SCCLUSTERTREE	Pointer to current node.
------------------------------------------------------------------------
*/

void Sc_free_cluster_tree(SCCLUSTERTREE *node)
{
	if (node == NULL)
		return;
	Sc_free_cluster_tree(node->lptr);
	Sc_free_cluster_tree(node->rptr);
	if (node->cluster) efree((CHAR *)node->cluster);
	efree((CHAR *)node);
}

/***********************************************************************
Module:  Sc_place_net_balance
------------------------------------------------------------------------
Description:
	To a net balancing on the placelist.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_net_balance(row, cell);

Name		Type		Description
----		----		-----------
row			*SCROWLIST	Pointer to start of row list.
cell		*SCCELL		Pointer to parent cell.
err			int			Returned error code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_net_balance(SCROWLIST *row, SCCELL *cell)
{
	SCEXTNODE	*nlist;
	SCCHANNEL	*channel, *endchan, *newchan, *nextnewchan;
	SCNBTRUNK	*trunks, *ntrunk, *oldtrunk, *sametrunk, *nextntrunk;
	int		i, err;
	Q_UNUSED( row );

	/* create channel list */
	channel = endchan = NULL;
	i = 0;
	sametrunk = NULL;
	do
	{
		newchan = (SCCHANNEL *)emalloc(sizeof(SCCHANNEL), sc_tool->cluster);
		if (newchan == NULL)
			return(Sc_seterrmsg(SC_NOMEMORY));
		newchan->number = i;
		trunks = oldtrunk = NULL;

		/* create trunk list for each channel */
		for (nlist = cell->ex_nodes; nlist; nlist = nlist->next)
		{
			ntrunk = (SCNBTRUNK *)emalloc(sizeof(SCNBTRUNK), sc_tool->cluster);
			if (ntrunk == NULL)
				return(Sc_seterrmsg(SC_NOMEMORY));
			ntrunk->ext_node = nlist;
			ntrunk->minx = 0;
			ntrunk->maxx = 0;
			ntrunk->same = NULL;
			if (sametrunk == NULL)
			{
				nlist->ptr = (CHAR *)ntrunk;
			} else
			{
				sametrunk->same = ntrunk;
				sametrunk = sametrunk->next;
			}
			ntrunk->next = NULL;
			if (oldtrunk == NULL)
			{
				trunks = oldtrunk = ntrunk;
			} else
			{
				oldtrunk->next = ntrunk;
				oldtrunk = ntrunk;
			}
		}
		newchan->trunks = trunks;
		newchan->last = endchan;
		newchan->next = NULL;
		if (endchan)
		{
			endchan->next = newchan;
			endchan = newchan;
		} else
		{
			channel = endchan = newchan;
		}
		sametrunk = trunks;
		i++;
	} while ((i + 1) < cell->placement->num_rows);

	/* report current placement evaluation */
	if (sc_placecontrol.stats_flag)
		ttyputmsg(M_("Evaluation before Net-Balancing  = %d"),
			Sc_place_nb_cost(cell->placement->rows, channel, cell));

	/* do the net balance for each cell */
	if ((err = Sc_place_nb_all_cells(cell, channel)))
		return(err);

	/* number placement */
	Sc_place_nb_rebalance_rows(cell->placement->rows, cell->placement);
	Sc_place_number_placement(cell->placement->rows);

	/* report new evaluation */
	if (sc_placecontrol.stats_flag)
		ttyputmsg(M_("Evaluation after Net-Balancing   = %d"),
			Sc_place_nb_cost(cell->placement->rows, channel, cell));

	for (newchan = channel; newchan; newchan = nextnewchan)
	{
		nextnewchan = newchan->next;
		for (ntrunk = newchan->trunks; ntrunk; ntrunk = nextntrunk)
		{
			nextntrunk = ntrunk->next;
			efree((CHAR *)ntrunk);
		}
		efree((CHAR *)newchan);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_nb_all_cells
------------------------------------------------------------------------
Description:
	Do a net balance for each cell on at a time.  Use the SCNITREE to
	insure that each cell is processed.
------------------------------------------------------------------------
Calling Sequence:  err = Sc_place_nb_all_cells(cell, channels);

Name		Type		Description
----		----		-----------
cell		*SCCELL		Pointer to parent cell.
channels	*SCCHANNEL	Pointer to start of channel list.
err			int			Returned err code, 0 = no error.
------------------------------------------------------------------------
*/

int Sc_place_nb_all_cells(SCCELL *cell, SCCHANNEL *channels)
{
	SCNITREE	*ilist;

	/* process cell */
	for (ilist = cell->nilist; ilist; ilist = ilist->next)
	{
		if (ilist->type == SCLEAFCELL)
			Sc_place_nb_do_cell((SCNBPLACE *)ilist->tp, channels, cell);
	}

	return(SC_NOERROR);
}

/***********************************************************************
Module:  Sc_place_nb_do_cell
------------------------------------------------------------------------
Description:
	Do a net balance for the indicated instance.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_nb_do_cell(place, channels, cell);

Name		Type		Description
----		----		-----------
place		*SCNBPLACE	Pointer to place of instance.
channels	*SCCHANNEL	Pointer to channel list of trunks.
cell		*SCCELL		Parent complex cell.
------------------------------------------------------------------------
*/

void Sc_place_nb_do_cell(SCNBPLACE *place, SCCHANNEL *channels, SCCELL *cell)
{
	int		min_cost, cost, pos, npos;
	SCNBPLACE	*old_last, *old_next, *nplace;
	SCROWLIST	*rows;

	if (Sc_stop())
		longjmp(sc_please_stop, 1);

	if (place == NULL) return;

	/* find cost at present location and set as current minimum */
	rows = cell->placement->rows;
	min_cost = Sc_place_nb_cost(rows, channels, cell);
	pos = 0;

	/* temporarily remove from list */
	old_last = place->last;
	old_next = place->next;
	Sc_place_nb_remove(place, rows);

	/* check locations backwards for nb_limit */
	npos = -1;
	for (nplace = old_last; npos >= -sc_placecontrol.net_balance_limit;
		nplace = nplace->last)
	{
		if (nplace)
		{
			/* temporarily insert in list */
			Sc_place_nb_insert_before(place, nplace, rows);

			/* check new cost */
			if ((cost = Sc_place_nb_cost(rows, channels, cell)) < min_cost)
			{
				min_cost = cost;
				pos = npos;
			}

			/* remove place from list */
			Sc_place_nb_remove(place, rows);
		} else
		{
			break;
		}
		npos--;
	}

	/* check forward locations for nb_limit */
	npos = 1;
	for (nplace = old_next; npos < sc_placecontrol.net_balance_limit;
		nplace = nplace->next)
	{
		if (nplace)
		{
			/* temporarily insert in list */
			Sc_place_nb_insert_after(place, nplace, rows);

			/* check new cost */
			if ((cost = Sc_place_nb_cost(rows, channels, cell)) < min_cost)
			{
				min_cost = cost;
				pos = npos;
			}

			/* remove place from list */
			Sc_place_nb_remove(place, rows);
		} else
		{
			break;
		}
		npos++;
	}

	/* move if necessary */
	if (pos > 0)
	{
		while(pos-- > 1)
		{
			old_next = old_next->next;
		}
		Sc_place_nb_insert_after(place, old_next, rows);
	} else if (pos < 0)
	{
		while(pos++ < -1)
		{
			old_last = old_last->last;
		}
		Sc_place_nb_insert_before(place, old_last, rows);
	} else
	{
		if (old_last)
		{
			Sc_place_nb_insert_after(place, old_last, rows);
		} else
		{
			Sc_place_nb_insert_before(place, old_next, rows);
		}
	}
}

/***********************************************************************
Module:  Sc_place_nb_cost
------------------------------------------------------------------------
Description:
	Return cost of the indicated placement.
------------------------------------------------------------------------
Calling Sequence:  cost = Sc_place_nb_cost(rows, channels, cell);

Name		Type		Description
----		----		-----------
rows		*SCROWLIST	Pointer to start of list or rows.
channels	*SCCHANNEL	Pointer to list of channels.
cell		*SCCELL		Pointer to parent cell.
cost		int			Returned cost.
------------------------------------------------------------------------
*/

int Sc_place_nb_cost(SCROWLIST *rows, SCCHANNEL *channels, SCCELL *cell)
{
	int		cost, dis, above, i, fcount, count, fminx, fmaxx;
	int		row_num, max_rowsize, pos;
	SCNBPLACE	*nplace;
	SCCHANNEL	*nchan;
	SCNBTRUNK	*ntrunk, *strunk, *ftrunk;
	SCNIPORT	*port;

	/* initialize all trunks */
	for (nchan = channels; nchan; nchan = nchan->next)
	{
		for (ntrunk = nchan->trunks; ntrunk; ntrunk = ntrunk->next)
		{
			ntrunk->minx = MAXINTBIG;
			ntrunk->maxx = -MAXINTBIG;
		}
	}

	/* check all rows */
	nchan = channels;
	above = TRUE;
	dis = 0;
	row_num = 0;
	max_rowsize = cell->placement->size_rows + (cell->placement->avg_size >>1);
	for (nplace = rows->start; nplace; nplace = nplace->next)
	{
		/* check for room in current row */
		if (row_num % 2)
		{
			/* odd row */
			if ((dis - nplace->cell->size) < 0)
			{
				if ((row_num + 1) < cell->placement->num_rows)
				{
					row_num++;
					dis = 0;
					if (above ^= TRUE)
						nchan = nchan->next;
				}
			}
		} else
		{
			/* even row */
			if ((dis + nplace->cell->size) > max_rowsize)
			{
				if ((row_num + 1) < cell->placement->num_rows)
				{
					row_num++;
					dis = max_rowsize;
					if (above ^= TRUE)
						nchan = nchan->next;
				}
			}
		}

		/* check all ports on instance */
		for (port = nplace->cell->ports; port; port = port->next)
		{
			/* find the correct trunk */
			ntrunk = (SCNBTRUNK *)port->ext_node->ptr;
			if (!ntrunk) continue;
			for (i = nchan->number; i; i--)
				ntrunk = ntrunk->same;
			if (ntrunk->minx == MAXINTBIG)
			{
				if (!above && ntrunk->same)
					ntrunk = ntrunk->same;
			}
			if (row_num % 2)
			{
				pos = dis - port->xpos;
			} else
			{
				pos = dis + port->xpos;
			}
			ntrunk->minx = mini(ntrunk->minx, pos);
			ntrunk->maxx = maxi(ntrunk->maxx, pos);
		}
		if (row_num % 2)
		{
			dis -= nplace->cell->size;
		} else
		{
			dis += nplace->cell->size;
		}
	}

	/* calculate cost */
	cost = 0;

	/* calculate horizontal costs */
	for (nchan = channels; nchan; nchan = nchan->next)
	{
		for (ntrunk = nchan->trunks; ntrunk; ntrunk = ntrunk->next)
		{
			if (ntrunk->minx != MAXINTBIG)
				cost += abs(ntrunk->maxx - ntrunk->minx);
		}
	}

	/* calculate vertical cost */
	for (ntrunk = channels->trunks; ntrunk; ntrunk = ntrunk->next)
	{
		ftrunk = NULL;
		fcount = count = 0;
		for (strunk = ntrunk; strunk; strunk = strunk->same)
		{
			if (strunk->minx != MAXINTBIG)
			{
				if (ftrunk == NULL)
				{
					ftrunk = strunk;
					fminx = strunk->minx;
					fmaxx = strunk->maxx;
					fcount = count;
				} else
				{
					/* add new vertical */
					cost += (count - fcount) * cell->placement->avg_height *
						sc_placecontrol.vertical_cost;
					fcount = count;

					/* additional horizontal */
					if (fmaxx < strunk->minx)
					{
						cost += abs(strunk->minx - fmaxx);
						fmaxx = strunk->maxx;
					} else if (fminx > strunk->maxx)
					{
						cost += abs(fminx - strunk->maxx);
						fminx = strunk->minx;
					} else
					{
						if (fminx > strunk->minx)
							fminx = strunk->minx;
						if (fmaxx < strunk->maxx)
							fmaxx = strunk->maxx;
					}

				}
			}
			count++;
		}
	}

	return(cost);
}

/***********************************************************************
Module:  Sc_place_nb_remove
------------------------------------------------------------------------
Description:
	Remove the indicated placed instance and clean up the rows
	structures.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_nb_remove(place, rows);

Name		Type		Description
----		----		-----------
place		*SCNBPLACE	Pointer to place to be removed.
rows		*SCROWLIST	Pointer to start of row list.
------------------------------------------------------------------------
*/

void Sc_place_nb_remove(SCNBPLACE *place, SCROWLIST *rows)
{
	SCROWLIST	*row;
	SCNBPLACE	*old_next, *old_last;

	old_next = place->next;
	old_last = place->last;
	if (place->last)
		place->last->next = old_next;
	if (place->next)
		place->next->last = old_last;

	/* check if row change */
	for (row = rows; row; row = row->next)
	{
		if (row->start == place)
		{
			if (row->row_num % 2)
			{
				row->start = old_last;
			} else
			{
				row->start = old_next;
			}
		}
		if (row->end == place)
		{
			if (row->row_num % 2)
			{
				row->end = old_next;
			} else
			{
				row->end = old_last;
			}
		}
	}
}

/***********************************************************************
Module:  Sc_place_nb_insert_before
------------------------------------------------------------------------
Description:
	Insert the indicated place before the indicated second place and
	clear up the row markers if necessary.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_nb_insert_before(place, oldplace, rows);

Name		Type		Description
----		----		-----------
place		*SCNBPLACE	Pointer to place to be inserted.
oldplace	*SCNBPLACE	Pointer to place to be inserted before.
rows		*SCROWLIST	Start of list of row markers.
------------------------------------------------------------------------
*/

void Sc_place_nb_insert_before(SCNBPLACE *place, SCNBPLACE *oldplace, SCROWLIST *rows)
{
	SCROWLIST	*row;

	place->next = oldplace;
	if (oldplace)
	{
		place->last = oldplace->last;
		if (oldplace->last)
			oldplace->last->next = place;
		oldplace->last = place;
	} else
	{
		place->last = NULL;
	}

	/* check if row change */
	for (row = rows; row; row = row->next)
	{
		if (row->start == oldplace)
		{
			if (row->row_num % 2)
			{
				/* EMPTY */ 
			} else
			{
				row->start = place;
			}
		}
		if (row->end == oldplace)
		{
			if (row->row_num % 2)
				row->end = place;
		}
	}
}

/***********************************************************************
Module:  Sc_place_nb_insert_after
------------------------------------------------------------------------
Description:
	Insert the indicated place after the indicated second place and
	clear up the row markers if necessary.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_nb_insert_after(place, oldplace, rows);

Name		Type		Description
----		----		-----------
place		*SCNBPLACE	Pointer to place to be inserted.
oldplace	*SCNBPLACE	Pointer to place to be inserted after.
rows		*SCROWLIST	Start of list of row markers.
------------------------------------------------------------------------
*/

void Sc_place_nb_insert_after(SCNBPLACE *place, SCNBPLACE *oldplace, SCROWLIST *rows)
{
	SCROWLIST	*row;

	place->last = oldplace;
	if (oldplace)
	{
		place->next = oldplace->next;
		if (oldplace->next)
			oldplace->next->last = place;
		oldplace->next = place;
	} else
	{
		place->next = NULL;
	}

	/* check if row change */
	for (row = rows; row; row = row->next)
	{
		if (row->start == oldplace)
		{
			if (row->row_num % 2)
				row->start = place;
		}
		if (row->end == oldplace)
		{
			if (row->row_num % 2)
			{
				/* EMPTY */ 
			} else
			{
				row->end = place;
			}
		}
	}
}

/***********************************************************************
Module:  Sc_place_nb_rebalance_rows
------------------------------------------------------------------------
Description:
	Check balancing for rows as there has been a change in placement.
------------------------------------------------------------------------
Callling Sequence:  Sc_place_nb_rebalance_rows(rows, place);

Name		Type		Description
----		----		-----------
rows		*SCROWLIST	Pointer to start of row list.
place		*SCPLACE	Pointer to global placement structure.
------------------------------------------------------------------------
*/

void Sc_place_nb_rebalance_rows(SCROWLIST *rows, SCPLACE *place)
{
	int		max_rowsize;
	SCNBPLACE	*nplace;

	max_rowsize = place->size_rows + (place->avg_size >> 1);
	rows->row_size = 0;
	for (nplace = rows->start; nplace; nplace = nplace->next)
	{
		if ((rows->row_num + 1) < place->num_rows &&
			(rows->row_size + nplace->cell->size) > max_rowsize)
		{
			rows = rows->next;
			rows->row_size = 0;
			if (rows->row_num % 2)
			{
				rows->end = nplace;
			} else
			{
				rows->start = nplace;
			}
		}
		rows->row_size += nplace->cell->size;
		if (rows->row_num % 2)
		{
			rows->start = nplace;
		} else
		{
			rows->end = nplace;
		}
	}
}

/***********************************************************************
Module:  Sc_place_number_placement
------------------------------------------------------------------------
Description:
	Number the x position of all the cells in their rows.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_number_placement(rows);

Name		Type		Description
----		----		-----------
rows		*SCROWLIST	Pointer to the start of the rows.
------------------------------------------------------------------------
*/

void Sc_place_number_placement(SCROWLIST *rows)
{
	SCROWLIST	*row;
	int		xpos;
	SCNBPLACE	*nplace;

	for (row = rows; row; row = row->next)
	{
		xpos = 0;
		nplace = row->start;
		while (nplace)
		{
			nplace->xpos = xpos;
			xpos += nplace->cell->size;
			if (nplace == row->end)
			{
				break;
			}
			if (row->row_num % 2)
			{
				nplace = nplace->last;
			} else
			{
				nplace = nplace->next;
			}
		}
	}
}

/***********************************************************************
Module:  Sc_place_show_placement
------------------------------------------------------------------------
Description:
	Print the cells in their rows of placement.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_show_placement(rows);

Name		Type		Description
----		----		-----------
rows		*SCROWLIST	Pointer to the start of the rows.
------------------------------------------------------------------------
*/

void Sc_place_show_placement(SCROWLIST *rows)
{
	SCROWLIST	*row;
	SCNBPLACE	*inst;

	for (row = rows; row; row = row->next)
	{
		ttyputmsg(M_("For Row #%d, size %d:"), row->row_num, row->row_size);
		for (inst = row->start; inst != row->end;)
		{
			ttyputmsg(x_("    %8d    %-s"), inst->xpos, inst->cell->name);
			if (row->row_num % 2)
			{
				inst = inst->last;
			} else
			{
				inst = inst->next;
			}
		}
		ttyputmsg(x_("    %8d    %-s"), inst->xpos, inst->cell->name);
	}
}

/***********************************************************************
Module:  Sc_place_reorder_rows
------------------------------------------------------------------------
Description:
	Clean up the placement rows structure by reversing the pointers
	of odd rows and breaking the snake pattern by row.
------------------------------------------------------------------------
Calling Sequence:  Sc_place_reorder_rows(rows);

Name		Type		Description
----		----		-----------
rows		*SCROWLIST	Pointer to start of row list.
------------------------------------------------------------------------
*/

void Sc_place_reorder_rows(SCROWLIST *rows)
{
	SCROWLIST	*row;
	SCNBPLACE	*place, *tplace;

	for (row = rows; row; row = row->next)
	{
		if (row->row_num % 2)
		{
			/* odd row */
			for (place = row->start; place; place =place->next)
			{
				tplace = place->next;
				place->next = place->last;
				place->last = tplace;
				if (place == row->end)
					break;
			}
			row->start->last = NULL;
			row->end->next = NULL;
		} else
		{
			/* even row */
			row->start->last = NULL;
			row->end->next = NULL;
		}
	}
}

#endif  /* SCTOOL - at top */

