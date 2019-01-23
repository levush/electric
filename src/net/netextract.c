/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: netextract.c
 * Network tool: module for node extraction
 * Written by: David Groulx and Brett Bissinger, Harvey Mudd College
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
 *
 * Things to do:
 *      Detect transistors
 *      Remove the well/select layers
 *      Find implicit connections created by PL nodes that cross or end in a "T"
 */
#include "global.h"
#include "network.h"
#include "efunction.h"
#include "usr.h"
#include "tecgen.h"

static void      build_node_arc_list( NODEINST**, INTBIG*, TECHNOLOGY*, INTBIG*, ARCINST**, NODEINST**, NODEPROTO* );
static void      add_node( NODEINST* node, NODEINST** list );
static void      remove_node( NODEINST*, NODEINST** list, INTBIG *count );
static void      add_arc( ARCINST* arc, ARCINST** list );
static void      via_to_contact( NODEINST**, INTBIG*, TECHNOLOGY*, INTBIG*, ARCINST**, NODEINST**, NODEPROTO* );
static BOOLEAN   contains_node( NODEINST*, NODEINST* );

/*
 * Routine to convert cell "cell" from pure-layer nodes to real nodes and arcs.
 */
void net_conv_to_internal( NODEPROTO *cell )
{
	NODEINST *nodes;  /* head of the new node list */
	ARCINST *arcs;  /* head of the new arc list */
	NODEINST *db_node; /* for editing arcs to point to inst in the databade, also for */
	                   /* call to endchangeobject */
	NODEINST *nodelooper, **plnodelist, *ni;
	NODEINST node;
	NODEPROTO *prim;
	static POLYGON *poly = NOPOLYGON;
	ARCINST *arclooper;
	ARCINST *db_arc;  /* store return of newarcinst(), use in call to endchangeobject() */
	INTBIG nodecounter, *equivlayers, i, j;
	VARIABLE *var;
	CHAR **ciflayers;
	TECHNOLOGY *tech;

	/* count the number of PL nodes */
	tech = NOTECHNOLOGY;
	nodecounter = 0;
	for(nodelooper = cell->firstnodeinst; nodelooper != NONODEINST; nodelooper = nodelooper->nextnodeinst)
	{
		if (((nodelooper->proto->userbits & NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
		nodecounter++;
		if (tech == NOTECHNOLOGY) tech = nodelooper->proto->tech; else
		{
			if (tech != nodelooper->proto->tech)
			{
				ttyputerr(_("Cannot determine technology to use (found %s and %s)"),
					tech->techname, nodelooper->proto->tech->techname);
				return;
			}
		}
	}
	if (nodecounter == 0)
	{
		ttyputmsg(_("There are no pure-layer nodes to convert"));
		return;
	}

	/* save the PL nodes in a list */
	plnodelist = (NODEINST **)emalloc(nodecounter * (sizeof (NODEINST *)), net_tool->cluster);
	if (plnodelist == 0) return;
	nodecounter = 0;
	for(nodelooper = cell->firstnodeinst; nodelooper != NONODEINST; nodelooper = nodelooper->nextnodeinst)
	{
		if (((nodelooper->proto->userbits & NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
		plnodelist[nodecounter++] = nodelooper;
	}
	ttyputmsg(_("There are %ld pure-layer nodes"), nodecounter);

	/* setup equivalent layers to handle CIF layer unification */
	equivlayers = (INTBIG *)emalloc(tech->layercount * SIZEOFINTBIG, net_tool->cluster);
	if (equivlayers == 0) return;
	var = getval((INTBIG)tech, VTECHNOLOGY, VSTRING|VISARRAY, x_("IO_cif_layer_names"));
	if (var == NOVARIABLE) ciflayers = 0; else
		ciflayers = (CHAR **)var->addr;
	for(i=0; i<tech->layercount; i++)
	{
		equivlayers[i] = i;
		if (ciflayers != 0)
		{
			for(j=0; j<i; j++)
			{
				if (namesame(ciflayers[j], ciflayers[i]) != 0) continue;
				equivlayers[i] = j;
				break;
			}
		}
	}

	/* precache the layer number in each pure-layer node prototype */
	(void)needstaticpolygon(&poly, 4, net_tool->cluster);
	for(prim = tech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		if (((prim->userbits&NFUNCTION) >> NFUNCTIONSH) != NPNODE) continue;
		ni = &node;   initdummynode(ni);
		ni->proto = prim;
		ni->lowx = 0;   ni->highx = 100;
		ni->lowy = 0;   ni->highy = 100;
		(void)nodepolys(ni, 0, NOWINDOWPART);
		shapenodepoly(ni, 0, poly);
		prim->temp1 = equivlayers[poly->layer];
	}

	/* initialize list of new nodes and arcs */
	nodes = NONODEINST;
	arcs = NOARCINST;

	/* find contacts */
	via_to_contact( plnodelist, &nodecounter,
					tech,
					equivlayers,
		            &arcs,
					&nodes,
					cell );

	/* convert to arcs and pins */
	build_node_arc_list( plnodelist, &nodecounter,
						 tech,
						 equivlayers,
		                 &arcs, 
						 &nodes, 
						 cell );

#if 0		/* not handling this yet */
	arc_crossings( &arcs, &nodes, cell );

	t_crossings( &arcs, &nodes, cell );
#endif

	/* create all the nodes in the lists, and store the pointers */
	/* in a new list for use in the arcs */
	for( ; nodes != NONODEINST; nodes = nodes->nextnodeinst )
	{
		db_node = newnodeinst( nodes->proto, 
					            nodes->lowx, 
								nodes->highx, 
								nodes->lowy, 
								nodes->highy, 
								nodes->transpose, 
								nodes->rotation, 
								nodes->parent );
		if (db_node == NONODEINST) continue;
		endobjectchange( (INTBIG)db_node, VNODEINST );

		/* search through all arcs and change their ends to point */
		/* to NODEINST*'s in the database */
		arclooper = arcs;
		for( ; arclooper != NOARCINST; arclooper = arclooper->nextarcinst )
		{
			if( arclooper->end[0].nodeinst == nodes )
			{
				arclooper->end[0].nodeinst = db_node;
			}

			if( arclooper->end[1].nodeinst == nodes )
			{
				arclooper->end[1].nodeinst = db_node;
			}
		}
	}

	/* create arc insts */
	for( ; arcs != NOARCINST; arcs = arcs->nextarcinst )
	{
		db_arc = newarcinst( arcs->proto,
			 			   arcs->width,
				               arcs->userbits,
							   arcs->end[0].nodeinst,
							   arcs->end[0].nodeinst->proto->firstportproto,
							   arcs->end[0].xpos,
							   arcs->end[0].ypos,
							   arcs->end[1].nodeinst,
							   arcs->end[1].nodeinst->proto->firstportproto,
							   arcs->end[1].xpos,
							   arcs->end[1].ypos,
							   arcs->parent );
		if (db_arc == NOARCINST)
		{
			ttyputerr(_("Error creating %s arc"), arcs->proto->protoname);
			continue;
		}
		endobjectchange( (INTBIG)db_arc, VARCINST );
	}

	/* deallocate the list of pure-layer nodes */
	efree((CHAR *)plnodelist);
	efree((CHAR *)equivlayers);
}

/********************************** EXTRACTION CASES **********************************/

#define MAXORIGLAYERS 10

typedef struct
{
	INTBIG cutlayer;					/* the layer number of the cut */
	INTBIG otherlayercount;				/* number of other layers associated with contact */
	INTBIG otherlayers[MAXORIGLAYERS];	/* other contact layers */
	NODEPROTO *propercontact;			/* contact primitive */
} CONTACT;

void via_to_contact( NODEINST** parselist, INTBIG *parselistcount,
					   TECHNOLOGY *tech,
					   INTBIG *equivlayers,
					   ARCINST** arcs, 
					   NODEINST** nodes, 
					   NODEPROTO* cell )
{
	/* locate vias an turn them into nodes, removing them from the parselist */
	NODEINST *looper;
	NODEINST node;
	NODEINST *contact_search;
	NODEINST *contact;
	NODEINST *type[MAXORIGLAYERS], *ni;
	NODEPROTO *thisismycontact, *prim;
	CONTACT *contactlist;
	INTBIG i, j, k, fun, thiscontact, lx, hx, ly, hy, lowx, highx, lowy, highy,
		contactcount, totlayers, otherlayercount, typeindex[MAXORIGLAYERS];
	static POLYGON *poly = NOPOLYGON;
	Q_UNUSED( arcs );

	/* allocate a polygon */
	(void)needstaticpolygon(&poly, 4, net_tool->cluster);

	/* make a catalog of the contacts and their layers */
	contactcount = 0;
	for(prim = tech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		fun = (prim->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPCONTACT && fun != NPWELL && fun != NPSUBSTRATE) continue;
		contactcount++;
	}
	if (contactcount == 0) return;
	contactlist = (CONTACT *)emalloc(contactcount * (sizeof (CONTACT)), net_tool->cluster);
	if (contactlist == 0) return;
	contactcount = 0;
	for(prim = tech->firstnodeproto; prim != NONODEPROTO; prim = prim->nextnodeproto)
	{
		fun = (prim->userbits&NFUNCTION) >> NFUNCTIONSH;
		if (fun != NPCONTACT && fun != NPWELL && fun != NPSUBSTRATE) continue;
		ni = &node;   initdummynode(ni);
		ni->proto = prim;
		ni->lowx = prim->lowx;   ni->highx = prim->highx;
		ni->lowy = prim->lowy;   ni->highy = prim->highy;
		totlayers = nodepolys(ni, 0, NOWINDOWPART);
		otherlayercount = 0;
		for(i=0; i<totlayers; i++)
		{
			shapenodepoly(ni, i, poly);
			if (poly->tech != tech) continue;
			fun = layerfunction(tech, poly->layer) & LFTYPE;
			if (!layeriscontact(fun)) contactlist[contactcount].cutlayer = equivlayers[poly->layer]; else
			{
				if (otherlayercount >= MAXORIGLAYERS) continue;
				contactlist[contactcount].otherlayers[otherlayercount] = equivlayers[poly->layer];
				otherlayercount++;
			}
		}
		contactlist[contactcount].propercontact = prim;
		contactlist[contactcount].otherlayercount = otherlayercount;
		contactcount++;
	}

	/* loop though looking for nodes that are of cut type */
	for(i=0; i < (*parselistcount); i++)
	{
		looper = parselist[i];

		/* look for layers that match one of the contact descriptions */
		for(thiscontact=0; thiscontact<contactcount; thiscontact++)
		{
			/* current node must match the cut layer */
			if (looper->proto->temp1 != contactlist[thiscontact].cutlayer)
				continue;

			/* find other layers near the cut, store them in "type" */
			for(k=0; k<contactlist[thiscontact].otherlayercount; k++)
				type[k] = NONODEINST;
			for(j=0; j < (*parselistcount); j++)
			{
				contact_search = parselist[j];
				for(k=0; k<contactlist[thiscontact].otherlayercount; k++)
				{
					if (contact_search->proto->temp1 != contactlist[thiscontact].otherlayers[k])
						continue;
					if (contains_node(contact_search, looper))
					{
						if (type[k] != NONODEINST)
						{
							/* want the smallest enclosing PL node on this layer */
							if ((type[k]->highx-type[k]->lowx) * (type[k]->highy-type[k]->lowy) <
								(contact_search->highx-contact_search->lowx) * (contact_search->highy-contact_search->lowy))
									continue;
						}
						type[k] = contact_search;
						typeindex[k] = j;
					}
				}
			}

			/* see if all other layers were found */
			for(k=0; k<contactlist[thiscontact].otherlayercount; k++)
				if (type[k] == NONODEINST) break;
			if (k >= contactlist[thiscontact].otherlayercount) break;
		}
		if (thiscontact >= contactcount) continue;
			
		/* find the correct contact proto */
		thisismycontact = contactlist[thiscontact].propercontact;

		/* the contact size is the size of the largest non-well layer */
		j = 0;
		for(k=0; k<contactlist[thiscontact].otherlayercount; k++)
		{
			fun = layerfunction(tech, type[k]->proto->temp1) & LFTYPE;
			if (fun == LFWELL || fun == LFIMPLANT) continue;
			if (j == 0)
			{
				lowx = type[k]->lowx;
				highx = type[k]->highx;
				lowy = type[k]->lowy;
				highy = type[k]->highy;
			} else
			{
				if (type[k]->lowx < lowx) lowx = type[k]->lowx;
				if (type[k]->highx > highx) highx = type[k]->highx;
				if (type[k]->lowy < lowy) lowy = type[k]->lowy;
				if (type[k]->highy > highy) highy = type[k]->highy;
			}
			j++;
		}

		/* create the new contact */
		contact = allocnodeinst(cell->lib->cluster);
		nodeprotosizeoffset(thisismycontact, &lx, &ly, &hx, &hy, cell);
		contact->highx = highx + hx;
		contact->lowx = lowx - lx;
		contact->highy = highy + hy;
		contact->lowy = lowy - ly;
		contact->parent = cell;
		contact->proto = thisismycontact;

		/* insert it into nodes */
		add_node( contact, nodes );

		/* remove PL nodes from the list */
		for(k=0; k<contactlist[thiscontact].otherlayercount; k++)
		{
			fun = layerfunction(tech, type[k]->proto->temp1) & LFTYPE;
			if (fun == LFWELL || fun == LFIMPLANT) continue;
			remove_node(type[k], parselist, parselistcount);
			if (typeindex[k] < i) i--;
		}

		/* remove looper from parselist */
		remove_node(looper, parselist, parselistcount);

		/* kill infinite loop by putting looper back to where it was before the contact */
		i--;
	} /*outside of for */
}


typedef struct
{
	INTBIG layercount;				/* number of layers associated with arc */
	INTBIG layers[MAXORIGLAYERS];	/* arc layers */
	ARCPROTO *properarc;			/* arc prototype */
} ARCDESC;

/* break down each PL node into 2 nodes and 1 arc. */
void  build_node_arc_list( NODEINST** parselist,
							 INTBIG   *parselistcount,
							 TECHNOLOGY *tech,
							 INTBIG *equivlayers,
							 ARCINST** arcs, 
							 NODEINST** nodes, 
							 NODEPROTO* cell )
{
	/* data that will be created and added to the list */
	NODEINST* nodeinst1;
	NODEINST* nodeinst2, *arc_search;
	ARCINST* arcinst1, *ai;
	ARCINST arc;
	INTBIG lowx1, highx1, lowy1, highy1, px1, py1;
	INTBIG lowx2, highx2, lowy2, highy2, px2, py2;
	INTBIG width, hx, lx, hy, ly;
	ARCPROTO* arctech, *ap;
	NODEPROTO* nodetech;
	NODEINST *thisPLnode, *type[MAXORIGLAYERS];
	PORTPROTO *pp1, *pp2;
	INTBIG i, j, k, arccount, thisarc, totlayers, layercount;
	ARCDESC *arclist;
	static POLYGON *poly = NOPOLYGON;

	/* allocate a polygon */
	(void)needstaticpolygon(&poly, 4, net_tool->cluster);

	/* make a catalog of the arcs and their layers */
	arccount = 0;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
		arccount++;
	arclist = (ARCDESC *)emalloc(arccount * (sizeof (ARCDESC)), net_tool->cluster);
	if (arclist == 0) return;
	arccount = 0;
	for(ap = tech->firstarcproto; ap != NOARCPROTO; ap = ap->nextarcproto)
	{
		ai = &arc;   initdummyarc(ai);
		ai->proto = ap;
		totlayers = arcpolys(ai, NOWINDOWPART);
		layercount = 0;
		for(i=0; i<totlayers; i++)
		{
			shapearcpoly(ai, i, poly);
			if (poly->tech != tech) continue;
			if (layercount >= MAXORIGLAYERS) continue;
			arclist[arccount].layers[layercount] = equivlayers[poly->layer];
			layercount++;
		}
		arclist[arccount].properarc = ap;
		arclist[arccount].layercount = layercount;
		arccount++;
	}

	for(i=0; i < *parselistcount; i++)
	{
		thisPLnode = parselist[i];
		/* parsing a normal node */

		/* find this layer in the list */
		for(thisarc = 0; thisarc < arccount; thisarc++)
		{
			/* must match the first layer of the arc */
			if (thisPLnode->proto->temp1 != arclist[thisarc].layers[0]) continue;

			/* found first layer: see if there are others */
			for(k=1; k<arclist[thisarc].layercount; k++) type[k] = NONODEINST;
			type[0] = thisPLnode;
			for(k=1; k<arclist[thisarc].layercount; k++)
			{
				for(j=0; j < (*parselistcount); j++)
				{
					arc_search = parselist[j];
					if (arc_search->proto->temp1 != arclist[thisarc].layers[k])
						continue;
					if (contains_node(arc_search, thisPLnode))
						type[k] = arc_search;
				}
			}
			for(k=1; k<arclist[thisarc].layercount; k++)
				if (type[k] == NONODEINST) break;
			if (k >= arclist[thisarc].layercount) break;
		}
		if (thisarc >= arccount) continue;

		/* determine if the node is horizontal or vertical */
		if( thisPLnode->highx - thisPLnode->lowx > thisPLnode->highy - thisPLnode->lowy )
		{
			/* horizontal node, new nodes at left and right */
			
			/* arc */
			width = thisPLnode->highy - thisPLnode->lowy;

			/* left node */
			lowx1 = thisPLnode->lowx;
			highx1 = thisPLnode->lowx + 
				                 ( thisPLnode->highy - thisPLnode->lowy );
			lowy1 = thisPLnode->lowy;
			highy1 = thisPLnode->highy;

			/* right node */
			lowx2 = thisPLnode->highx - 
				                 ( thisPLnode->highy - thisPLnode->lowy );
			highx2 = thisPLnode->highx;
			lowy2 = thisPLnode->lowy;
			highy2 = thisPLnode->highy;
		}
		else
		{
			/* vertical node, new nodes at top and bottom */

			/* arc */
			width = thisPLnode->highx - thisPLnode->lowx;

			/* top node */
			lowx1 = thisPLnode->lowx;
			highx1 = thisPLnode->highx;
			lowy1 = thisPLnode->highy - 
				                 ( thisPLnode->highx - thisPLnode->lowx );
			highy1 = thisPLnode->highy;

			/* bottom node */
			lowx2 = thisPLnode->lowx;
			highx2 = thisPLnode->highx;
			lowy2 = thisPLnode->lowy;
			highy2 = thisPLnode->lowy +
				                  ( thisPLnode->highx - thisPLnode->lowx );
		}

		/* determine the arc technology */
		arctech = arclist[thisarc].properarc;

		/* determine the pin technology */
		nodetech = getpinproto(arctech);
		if (nodetech == NONODEPROTO) continue;

		/* fill in data */
		px1 = (lowx1+highx1)/2;
		py1 = (lowy1+highy1)/2;
		for(nodeinst1 = *nodes; nodeinst1 != NONODEINST; nodeinst1 = nodeinst1->nextnodeinst)
		{
			for(pp1 = nodeinst1->proto->firstportproto; pp1 != NOPORTPROTO; pp1 = pp1->nextportproto)
			{
				shapeportpoly(nodeinst1, pp1, poly, FALSE);
				if (isinside(px1, py1, poly)) break;
			}
			if (pp1 != NOPORTPROTO) break;
		}
		if (nodeinst1 == NONODEINST)
		{
			nodeinst1 = allocnodeinst(cell->lib->cluster);
			nodeprotosizeoffset(nodetech, &lx, &ly, &hx, &hy, cell);
			nodeinst1->highx = highx1 + hx;
			nodeinst1->lowx = lowx1 - lx;
			nodeinst1->highy = highy1 + hy;
			nodeinst1->lowy = lowy1 - ly;
			nodeinst1->proto = nodetech;
			nodeinst1->parent = cell;
			pp1 = nodetech->firstportproto;

			add_node(nodeinst1, nodes);
		}

		px2 = (lowx2+highx2)/2;
		py2 = (lowy2+highy2)/2;
		for(nodeinst2 = *nodes; nodeinst2 != NONODEINST; nodeinst2 = nodeinst2->nextnodeinst)
		{
			for(pp2 = nodeinst2->proto->firstportproto; pp2 != NOPORTPROTO; pp2 = pp2->nextportproto)
			{
				shapeportpoly(nodeinst2, pp2, poly, FALSE);
				if (isinside(px2, py2, poly)) break;
			}
			if (pp2 != NOPORTPROTO) break;
		}
		if (nodeinst2 == NONODEINST)
		{
			nodeinst2 = allocnodeinst(cell->lib->cluster);
			nodeprotosizeoffset(nodetech, &lx, &ly, &hx, &hy, cell);
			nodeinst2->highx = highx2 + hx;
			nodeinst2->lowx = lowx2 - lx;
			nodeinst2->highy = highy2 + hy;
			nodeinst2->lowy = lowy2 - ly;
			nodeinst2->proto = nodetech;
			nodeinst2->parent = cell;
			pp2 = nodetech->firstportproto;

			add_node(nodeinst2, nodes);
		}

		/* allocate space for new things */
		arcinst1 = allocarcinst(cell->lib->cluster);

		arcinst1->width = width + arcprotowidthoffset(arctech);
		arcinst1->end[0].nodeinst = nodeinst1;
		arcinst1->end[0].xpos = px1;
		arcinst1->end[0].ypos = py1;
		arcinst1->end[1].nodeinst = nodeinst2;
		arcinst1->end[1].xpos = px2;
		arcinst1->end[1].ypos = py2;
		arcinst1->proto = arctech;
		arcinst1->userbits = us_makearcuserbits(arctech);
		arcinst1->parent = cell;

		add_arc( arcinst1, arcs );

		remove_node(thisPLnode, parselist, parselistcount);
		i--;
	}

	/* free arc structure */
	efree((CHAR *)arclist);
}

/*********************************** NOT YET USED ***********************************/

#if 0		/* not used yet */

NODEINST* arc_to_PL( ARCINST* arc, NODEPROTO* cell )
{
	/* create a new node to return */
	NODEINST* newnode;
	
	newnode = allocnodeinst(cell->lib->cluster);
	
	/* get the max x */
	if( arc->end[0].nodeinst->highx >= arc->end[1].nodeinst->highx )
	{
		newnode->highx = arc->end[0].nodeinst->highx;
	}
	else
	{
		newnode->highx = arc->end[1].nodeinst->highx;
	}

	/* get the low x */
	if( arc->end[0].nodeinst->lowx <= arc->end[1].nodeinst->lowx )
	{
		newnode->lowx = arc->end[0].nodeinst->lowx;
	}
	else
	{
		newnode->lowx = arc->end[1].nodeinst->lowx;
	}
	
	/* get the max y */
	if( arc->end[0].nodeinst->highy >= arc->end[1].nodeinst->highy )
	{
		newnode->highy = arc->end[0].nodeinst->highy;
	}
	else
	{
		newnode->highy = arc->end[1].nodeinst->highy;
	}
	
	/* get the low y */
	if( arc->end[0].nodeinst->lowy <= arc->end[1].nodeinst->lowy )
	{
		newnode->lowy = arc->end[0].nodeinst->lowy;
	}
	else
	{
		newnode->lowy = arc->end[1].nodeinst->lowy;
	}

	return newnode;
}

void arc_crossings( ARCINST** arcs, NODEINST** nodes, NODEPROTO* cell )
{
	ARCINST* a1;
	ARCINST* a2;
	NODEINST* n1;  /* new PL associated with a1 */
	NODEINST* n2;  /* associated new PL for a2 */
	NODEINST* newnode;
	ARCINST* newarc1;
	ARCINST* newarc2;
	ARCINST* newarc3;
	ARCINST* newarc4;
	INTBIG newxpos;
	INTBIG newypos;
	BOOLEAN cross_found;

	ARCINST* delete_arc_list = NOARCINST;

	
	for( a1 = *arcs; a1 != NOARCINST; a1 = a1->nextarcinst )
	{
		for( a2 = a1->nextarcinst; a2 != NOARCINST; a2 = a2->nextarcinst )
		{
			/* check if the arcs are of the same type */
			if( a1->proto == a2->proto )
			{
				cross_found = FALSE;

				/* calculate coords of each, store them like pl nodes for comparison */
				n1 = arc_to_PL( a1, cell );
				n2 = arc_to_PL( a2, cell );


				/* n1 horizontal, n2 vertical */
				if( n1->highx > n2->highx && 
					n1->lowx < n2->lowx &&
					n2->highy > n1->highy &&
					n2->lowy < n1->lowy )
				{

					ttyputmsg(_("overlap condition met"));
					/* create the new node */
					newnode = allocnodeinst(cell->lib->cluster);

					newnode->highx = n2->highx;
					newnode->lowx = n2->lowx;
					newnode->highy = n1->highy;
					newnode->lowy = n1->lowy;
					newnode->transpose = 0; 
					newnode->rotation = 0;
					newnode->parent = a1->parent;
					newnode->proto = a1->end[0].nodeinst->proto;
					cross_found = TRUE;
				}


				/* n1 vertical, n2 horizontal */
				else if( n2->highx > n1->highx && 
						 n2->lowx < n1->lowx &&
						 n1->highy > n2->highy &&
						 n1->lowy < n2->lowy )
				{
					ttyputmsg(_("overlap condition met"));



					/* create the new node */
					newnode = allocnodeinst(cell->lib->cluster);

					newnode->highx = n1->highx;
					newnode->lowx = n1->lowx;
					newnode->highy = n2->highy;
					newnode->lowy = n2->lowy;
					newnode->transpose = 0; 
					newnode->rotation = 0;
					newnode->parent = a1->parent;
					newnode->proto = a1->end[0].nodeinst->proto;
					cross_found = TRUE;
				} /* if( n1 instersects n2 )  */

				if( cross_found )
				{
				/* create 4 new arcs and delete these 2 */
				newarc1 = allocarcinst(cell->lib->cluster);
				newarc2 = allocarcinst(cell->lib->cluster);
				newarc3 = allocarcinst(cell->lib->cluster);
				newarc4 = allocarcinst(cell->lib->cluster);

				
				/* fill out the new arcs */
				newxpos = ( newnode->highx + newnode->lowx ) / 2;
				newypos = ( newnode->highy + newnode->lowy ) / 2;

				newarc1->parent = a1->parent;
				newarc2->parent = a1->parent;
				newarc3->parent = a1->parent;
				newarc4->parent = a1->parent;

				newarc1->proto = a1->proto;
				newarc2->proto = a1->proto;
				newarc3->proto = a1->proto;
				newarc4->proto = a1->proto;

				newarc1->end[0] = a1->end[0];
				newarc1->end[1].nodeinst = newnode;
				newarc1->end[1].xpos = newxpos;
				newarc1->end[1].ypos = newypos;
				
				newarc2->end[0] = a1->end[1];
				newarc2->end[1].nodeinst = newnode;
				newarc2->end[1].xpos = newxpos;
				newarc2->end[1].ypos = newypos;
				
				newarc3->end[0] = a2->end[0];
				newarc3->end[1].nodeinst = newnode;
				newarc3->end[1].xpos = newxpos;
				newarc3->end[1].ypos = newypos;
				
				newarc4->end[0] = a2->end[1];
				newarc4->end[1].nodeinst = newnode;
				newarc4->end[1].xpos = newxpos;
				newarc4->end[1].ypos = newypos;

					
				newarc1->width = newarc1->end[0].nodeinst->highx - 
								 newarc1->end[0].nodeinst->lowx;
				newarc2->width = newarc2->end[0].nodeinst->highx - 
								 newarc2->end[0].nodeinst->lowx;
				newarc3->width = newarc3->end[0].nodeinst->highx - 
								 newarc3->end[0].nodeinst->lowx;
				newarc4->width = newarc4->end[0].nodeinst->highx - 
								 newarc4->end[0].nodeinst->lowx;

				
				/* add arcs to the remove list */
				
				/*add_arc( a1, &delete_arc_list ); */
				/*add_arc( a2, &delete_arc_list ); */
				

				/* add the new node and the 4 arcs to the beginnings of the lists */
				newarc1->prevarcinst = NOARCINST;
				newarc1->nextarcinst = newarc2;
				newarc2->prevarcinst = newarc1;
				newarc2->nextarcinst = newarc3;
				newarc3->prevarcinst = newarc2;
				newarc3->nextarcinst = newarc4;
				newarc4->prevarcinst = newarc3;
				newarc4->nextarcinst = *arcs;

				if( *arcs != NOARCINST )
				{
					(*arcs)->prevarcinst = newarc4;
				}

				*arcs = newarc1;


				if( *nodes == NONODEINST )
				{
					newnode->prevnodeinst = NONODEINST;
					newnode->nextnodeinst = NONODEINST;
					*nodes = newnode;
				}
				else
				{
					newnode->prevnodeinst = NONODEINST;
					newnode->nextnodeinst = *nodes;
					(*nodes)->prevnodeinst = newnode;
					(*nodes) = newnode;
				}


				cross_found = FALSE;
				}
				

			} /* if( same proto ) */
		}  /* for a2 */
	} /* for a1 */

	/* loop through arcs list and remove all the arcs in the delete list */
	if( delete_arc_list == NOARCINST )
	{
		/* should delete arc list here */
	}

	for( ; delete_arc_list != NOARCINST; delete_arc_list = delete_arc_list->nextarcinst )
	{
	/*	remove_arc( delete_arc_list, arcs ); */
	}
}

void make_arc_geom( ARCINST** arcs, NODEPROTO* cell )
{
	GEOM* geom;
	ARCINST* arcinst;

	for(arcinst = *arcs ; arcinst != NOARCINST; arcinst = arcinst->nextarcinst )
	{
		geom = allocgeom(cell->lib->cluster);
		
		/* find the high x */
		if( arcinst->end[0].nodeinst->highx > arcinst->end[1].nodeinst->highx )
		{
			geom->highx = arcinst->end[0].nodeinst->highx;
		}
		else
		{
			geom->highx = arcinst->end[1].nodeinst->highx;
		}

		/* find the low x */
		if( arcinst->end[0].nodeinst->lowx < arcinst->end[1].nodeinst->lowx )
		{
			geom->lowx = arcinst->end[0].nodeinst->lowx;
		}
		else
		{
			geom->lowx = arcinst->end[1].nodeinst->lowx;
		}

		/* find the high y */
		if( arcinst->end[0].nodeinst->highy > arcinst->end[1].nodeinst->highy )
		{
			geom->highy = arcinst->end[0].nodeinst->highy;
		}
		else
		{
			geom->highy = arcinst->end[1].nodeinst->highy;
		}

		/* find the low y */
		if( arcinst->end[0].nodeinst->lowy < arcinst->end[1].nodeinst->lowy )
		{
			geom->lowy = arcinst->end[0].nodeinst->lowy;
		}
		else
		{
			geom->lowy = arcinst->end[1].nodeinst->lowy;
		}
		
		
		/* store in the arc */
		arcinst->geom = geom;
	}
}

void t_crossings( ARCINST** arcs, NODEINST** nodes, NODEPROTO* cell )
{
	ARCINST* looper;
	ARCINST* comparearc;
	NODEINST* newnode;
	ARCINST* newarc1;
	ARCINST* newarc2;
	ARCINST* newarc3;
	INTBIG xpos;
	INTBIG ypos;

	/* since geometries are null in the arcs currently, create them for all of them */
	make_arc_geom( arcs, cell );

	/* for each arc, run through every node and see if any are intersecting part of the arcs body */
	for( comparearc = *arcs; comparearc != NOARCINST; comparearc = comparearc->nextarcinst )
	{
		for( looper = *arcs; looper != NOARCINST; looper = looper->nextarcinst )
		{
			if( comparearc->proto == looper->proto ) 
			{
				/* check both nodes of looper to see if any lie within comparearc */

				/* check if comparearc is horizontal or vertical */
				if( ( comparearc->geom->highx - comparearc->geom->lowx ) >
					( comparearc->geom->highy - comparearc->geom->lowy ) )
				{
					/* comparearc horizontal */

					
					if( ( looper->end[0].nodeinst->highy < comparearc->geom->highy ) &&
						( looper->end[0].nodeinst->lowy > comparearc->geom->lowy ) )
					{
						newnode = allocnodeinst(cell->lib->cluster);
						newarc1 = allocarcinst(cell->lib->cluster);
						newarc2 = allocarcinst(cell->lib->cluster);
						newarc3 = allocarcinst(cell->lib->cluster);

						newnode->highx = looper->geom->highx;
						newnode->lowx = looper->geom->lowx;
						newnode->highy = comparearc->geom->highy;
						newnode->lowy = comparearc->geom->lowy;

						xpos = ( newnode->highx + newnode->lowx ) / 2;
						ypos = ( newnode->highy + newnode->lowy ) / 2;

						/* connect up new arcs */
						newarc1->end[0] = comparearc->end[0];
						newarc1->end[1].nodeinst = newnode;
						newarc1->end[1].xpos = xpos;
						newarc1->end[1].ypos = ypos;
						newarc2->end[0] = comparearc->end[1];
						newarc2->end[1].nodeinst = newnode;
						newarc2->end[1].xpos = xpos;
						newarc2->end[1].ypos = ypos;
						newarc3->end[0] = looper->end[1];
						newarc3->end[1].nodeinst = newnode;
						newarc3->end[1].xpos = xpos;
						newarc3->end[1].ypos = ypos;


						/* add to the current lists */
						add_node( newnode, nodes );
						add_arc( newarc1, arcs );
						add_arc( newarc2, arcs );
						add_arc( newarc3, arcs );
					}
		
					else if( ( looper->end[1].nodeinst->highy < comparearc->geom->highy ) &&
						     ( looper->end[1].nodeinst->lowy > comparearc->geom->lowy ) )
					{
						newnode = allocnodeinst(cell->lib->cluster);
						newarc1 = allocarcinst(cell->lib->cluster);
						newarc2 = allocarcinst(cell->lib->cluster);
						newarc3 = allocarcinst(cell->lib->cluster);

						newnode->highx = looper->geom->highx;
						newnode->lowx = looper->geom->lowx;
						newnode->highy = comparearc->geom->highy;
						newnode->lowy = looper->geom->lowy;

						xpos = ( newnode->highx + newnode->lowx ) / 2;
						ypos = ( newnode->highy + newnode->lowy ) / 2;

						/* connect up new arcs */
						newarc1->end[0] = comparearc->end[0];
						newarc1->end[1].nodeinst = newnode;
						newarc1->end[1].xpos = xpos;
						newarc1->end[1].ypos = ypos;
						newarc2->end[0] = comparearc->end[1];
						newarc2->end[1].nodeinst = newnode;
						newarc2->end[1].xpos = xpos;
						newarc2->end[1].ypos = ypos;
						newarc3->end[0] = looper->end[1];
						newarc3->end[1].nodeinst = newnode;
						newarc3->end[1].xpos = xpos;
						newarc3->end[1].ypos = ypos;


						/* add to the current lists */
						add_node( newnode, nodes );
						add_arc( newarc1, arcs );
						add_arc( newarc2, arcs );
						add_arc( newarc3, arcs );
					}
				}
				else
				{
					/* comparearc vertical */
					if( ( looper->end[0].nodeinst->highx < comparearc->geom->highx ) &&
						( looper->end[0].nodeinst->lowx > comparearc->geom->lowx ) )
					{
						newnode = allocnodeinst(cell->lib->cluster);
						newarc1 = allocarcinst(cell->lib->cluster);
						newarc2 = allocarcinst(cell->lib->cluster);
						newarc3 = allocarcinst(cell->lib->cluster);

						newnode->highx = comparearc->geom->highx;
						newnode->lowx = comparearc->geom->lowx;
						newnode->highy = looper->geom->highy;
						newnode->lowy = looper->geom->lowy;

						xpos = ( newnode->highx + newnode->lowx ) / 2;
						ypos = ( newnode->highy + newnode->lowy ) / 2;

						/* connect up new arcs */
						newarc1->end[0] = comparearc->end[0];
						newarc1->end[1].nodeinst = newnode;
						newarc1->end[1].xpos = xpos;
						newarc1->end[1].ypos = ypos;
						newarc2->end[0] = comparearc->end[1];
						newarc2->end[1].nodeinst = newnode;
						newarc2->end[1].xpos = xpos;
						newarc2->end[1].ypos = ypos;
						newarc3->end[0] = looper->end[1];
						newarc3->end[1].nodeinst = newnode;
						newarc3->end[1].xpos = xpos;
						newarc3->end[1].ypos = ypos;

						/* add to the current lists */
						add_node( newnode, nodes );
						add_arc( newarc1, arcs );
						add_arc( newarc2, arcs );
						add_arc( newarc3, arcs );
					}

					else if( ( looper->end[1].nodeinst->highx < comparearc->geom->highx ) &&
						     ( looper->end[1].nodeinst->lowx > comparearc->geom->lowx ) )
					{
						newnode = allocnodeinst(cell->lib->cluster);
						newarc1 = allocarcinst(cell->lib->cluster);
						newarc2 = allocarcinst(cell->lib->cluster);
						newarc3 = allocarcinst(cell->lib->cluster);

						newnode->highx = comparearc->geom->highx;
						newnode->lowx = comparearc->geom->lowx;
						newnode->highy = looper->geom->highy;
						newnode->lowy = looper->geom->lowy;

						xpos = ( newnode->highx + newnode->lowx ) / 2;
						ypos = ( newnode->highy + newnode->lowy ) / 2;

						/* connect up new arcs */
						newarc1->end[0] = comparearc->end[0];
						newarc1->end[1].nodeinst = newnode;
						newarc1->end[1].xpos = xpos;
						newarc1->end[1].ypos = ypos;
						newarc2->end[0] = comparearc->end[1];
						newarc2->end[1].nodeinst = newnode;
						newarc2->end[1].xpos = xpos;
						newarc2->end[1].ypos = ypos;
						newarc3->end[0] = looper->end[1];
						newarc3->end[1].nodeinst = newnode;
						newarc3->end[1].xpos = xpos;
						newarc3->end[1].ypos = ypos;

						/* add to the current lists */
						add_node( newnode, nodes );
						add_arc( newarc1, arcs );
						add_arc( newarc2, arcs );
						add_arc( newarc3, arcs );
					}
				}
			} /* same tech */
		} /* for looping nodes */
	} /* looping arcs */
}

void remove_arc( ARCINST* arc, ARCINST** list )
{
	ARCINST* looper;
	looper = *list;

	for( ; looper != NOARCINST; looper = looper->nextarcinst )
	{
		ttyputmsg(_("in the for loop"));

		if( arc == looper )
		{
			/* case 1: list has only one member */
			if( looper->prevarcinst == NOARCINST &&
				looper->nextarcinst == NOARCINST )
			{
				*list = NOARCINST;
			}

			/* case 2: node is at the beginning of the list */
			if( looper->prevarcinst == NOARCINST )
			{
				looper->nextarcinst->prevarcinst = NOARCINST;
				*list = looper->nextarcinst;
			}

			/* case 3: node is at the end of the list */
			else if( looper->nextarcinst == NOARCINST )
			{
				looper->prevarcinst->nextarcinst = NOARCINST;
			}

			/* case 4: node is in the middle of the list */
			else
			{
				looper->prevarcinst->nextarcinst =
					looper->nextarcinst;
				looper->nextarcinst->prevarcinst =
					looper->prevarcinst;
			}
			return;
		}
	}
}

#endif

/********************************** SUPPORT **********************************/

void add_arc( ARCINST* arc, ARCINST** list )
{
	arc->nextarcinst = *list;
	*list = arc;
}


void add_node( NODEINST* node, NODEINST** list )
{
	node->nextnodeinst = *list;
	*list = node;
}


void remove_node( NODEINST* node, NODEINST** list, INTBIG *count)
{
	INTBIG i, j;

	for(i=0; i < *count; i++)
		if (list[i] == node) break;
	if (i < *count)
	{
		(*count)--;
		for(j=i; j < *count; j++) list[j] = list[j+1];
		startobjectchange((INTBIG)node, VNODEINST);
		(void)killnodeinst(node);
	}
}


BOOLEAN contains_node( NODEINST* outside, NODEINST* inside )
{
	if (outside->highx > inside->highx &&
		outside->lowx < inside->lowx &&
		outside->lowy < inside->lowy &&
		outside->highy > inside->highy )
	{
		return (TRUE);
	}
	return (FALSE);
}
