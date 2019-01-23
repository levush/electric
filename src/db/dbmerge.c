/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbmerge.c
 * Box merging subsystem
 * Written by Philip Attfield, Queens University, Kingston Ontario.
 * Revised by Sid Penstone, Queens University, Kingston Ontario.
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
#include "database.h"
#include "egraphics.h"
#include "tecart.h"
#include "tecgen.h"
#include <math.h>

/***************************************************************************
 * Initially, call:
 *    mrginit()
 *
 * For every polygon, call:
 *    mrgstorebox(layer, tech, xsize, ysize, xcenter, ycenter)
 * where "layer" is an integer layer number from technology "tech"; "xsize"
 * and "ysize" are size of the box; "xcenter" and "ycenter" are the center
 * of the box
 *
 * At end of cell, call:
 *    mrgdonecell(writepolygon)
 * where "writepolygon" is a routine that will be called for every polygon with
 * five parameters:
 *   (1) the layer number (INTBIG)
 *   (2) the technology
 *   (3) the X coordinate array (INTBIG array)
 *   (4) the Y coordinate array (INTBIG array)
 *   (5) the number of coordinates in each array (INTBIG)
 *
 * When done with merging, call:
 *    mrgterm()
 ***************************************************************************
 */

#define MAXNUMPTS 106   /* maximum allowable number of points on a polygon */
static INTBIG maxnumpts = MAXNUMPTS; /* apply default (PJA:920527) */
# define PTMATCH 2      /* point match */

/* polygon direction and box edge identifiers */
#define UP          0
#define RIGHT       1
#define DOWN        2
#define LEFT        3

static POLYLISTHEAD db_mrg_polycoordlist;
static POLYCOORD   *db_mrg_free_poly_list;
static BOXPOLY     *db_mrg_free_box_list;
static BOXLISTHEAD *db_mrg_free_boxhead_list;
static BOXLISTHEAD *db_mrg_curr_cell;

/* working storage for "db_mrg_write_polygon()" */
static INTBIG *db_mrgxbuf, *db_mrgybuf;
static INTBIG  db_mrgbuflen = 0;

/* prototypes for local routines */
static BOOLEAN      db_mrg_included(BOXPOLY*, BOXPOLY*);
static void         db_mrg_remove_inclusion(BOXLISTHEAD*);
static void         db_mrg_remove_overlap(BOXLISTHEAD*);
static BOXPOLY     *db_mrg_breakbox(BOXPOLY*, BOXPOLY*, INTBIG*);
static BOXPOLY     *db_mrg_do_break(BOXPOLY*, BOXPOLY*, INTBIG*);
static void         db_mrg_write_lists(void(*)(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG));
static POLYCOORD   *db_mrg_getpoly(void);
static BOOLEAN      db_mrg_mergeboxwithpolygon(BOXPOLY*);
static INTBIG       db_mrg_check_intersect(BOXPOLY*, POLYCOORD*, INTBIG*);
static INTBIG       db_mrg_check_point(INTBIG[], INTBIG[], INTBIG[], INTBIG[]);
static void         db_mrg_insert1(INTBIG, BOXPOLY*);
static void         db_mrg_insert2(INTBIG, BOXPOLY*);
static void         db_mrg_insert3(INTBIG, BOXPOLY*);
static void         db_mrg_insert4(INTBIG, BOXPOLY*);
static void         db_mrg_insert5(INTBIG, INTBIG, BOXPOLY*);
static void         db_mrg_write_polygon(void(*)(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG), INTBIG, TECHNOLOGY*);
static INTBIG       db_mrg_succ(INTBIG);
static INTBIG       db_mrg_nxtside(INTBIG);
static INTBIG       db_mrg_prevside(INTBIG);
static void         db_mrg_assign_edge(POLYCOORD*, INTBIG[], INTBIG[]);
static void         db_mrg_housekeeper(void);
static BOXPOLY     *db_mrg_getbox(void);
static void         db_mrg_trash(BOXPOLY*);
static void         db_mrg_dump_free_box_list(void);
static void         db_mrg_assign_box(BOXPOLY*, INTBIG, INTBIG, INTBIG, INTBIG);
static void         db_mrg_killpoly(POLYCOORD*);
static void         db_mrg_dump_free_poly_list(void);
static void         db_mrg_remove_poly_list(void);
static BOXLISTHEAD *db_mrg_get_lay_head(void);
static void         db_mrg_dump_free_boxhead_list(void);
static void         db_mrg_remove_boxhead_list(void);

void mrginit(void)
{
}

void mrgterm(void)
{
	db_mrg_dump_free_poly_list();
	db_mrg_dump_free_box_list();
	db_mrg_dump_free_boxhead_list();
}

void mrgdonecell(void (*writepolygon)(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG))
{
	db_mrg_write_lists(writepolygon);
	db_mrg_remove_boxhead_list();
}

void mrgstorebox(INTBIG layer, TECHNOLOGY *tech, INTBIG length, INTBIG width,
	INTBIG xc, INTBIG yc)
{
	BOXLISTHEAD *layhead;
	BOXPOLY *creatptr;
	INTBIG left, right, top, bot;

	for(layhead = db_mrg_curr_cell; layhead != NULL; layhead = layhead->nextlayer)
		if (layhead->layer == layer && layhead->tech == tech) break;

	if (layhead == NULL)
	{
		layhead = db_mrg_get_lay_head();
		layhead->nextlayer = db_mrg_curr_cell; /* form link */
		db_mrg_curr_cell = layhead; /* modify cell layer list header */
		layhead->layer = layer;
		layhead->tech = tech;
		layhead->box = NULL;
	}

	/* layhead now points to the correct layer head descriptor */
	creatptr = db_mrg_getbox();
	creatptr->nextbox = layhead->box;
	layhead->box = creatptr; /* insert new descriptor at top of list */
	left = xc - (length/2);
	right = xc + (length/2);
	top = yc + (width/2);
	bot = yc - (width/2);

	/* assign box descriptor values */
	db_mrg_assign_box(creatptr, left, right, top, bot);
}

/************************* INTERNAL ROUTINES *************************/

/*
 * called once at initialization to set globals
 */
void db_mrgdatainit(void)
{
	db_mrg_free_poly_list = NULL;
	db_mrg_free_box_list = NULL;
	db_mrg_free_boxhead_list = NULL;
	db_mrg_curr_cell = NULL;
}

/* checks inclusion */
BOOLEAN db_mrg_included(BOXPOLY *me, BOXPOLY *him)
{
	if (him->left >= me->left && him->right <= me->right &&
		him->top <= me->top && him->bot >= me->bot)
			return(TRUE); /* him is an inclusion */
	return(FALSE); /* him is NOT an inclusion */
}

/* removes inclusions which follow on list */
void db_mrg_remove_inclusion(BOXLISTHEAD *lay_box)
{
	BOXPOLY *me, *him, *oldhim, *garbage;

	me = lay_box->box;
	garbage = NULL;

	if (me->nextbox != NULL) /* if not a singular list */
	{
		for(; me->nextbox != NULL;)
		{
			oldhim = me;
			for(him = me->nextbox; him != NULL;)
			{
				if (db_mrg_included(me, him)) /* if him is an inclusion of me */
				{
					garbage = him;
					oldhim->nextbox = him->nextbox; /* bypass him on list */
					him = him->nextbox;
					garbage->nextbox = NULL;
					db_mrg_trash(garbage);
				} else /* not an inclusion */
				{
					oldhim = him;
					him = him->nextbox;
				}
			} /* end for */

			/* catches case where everyone on list after me was an */
			me = me->nextbox;
			if (me == NULL) break;   /* inclusion */
		} /* end for */
	} /* end if */
}

void db_mrg_remove_overlap(BOXLISTHEAD *layhead)
{
	BOXPOLY *me, *him, *oldme, *garbage, *broken, *temp,
		*endptr;
	BOOLEAN bugflag;
	INTBIG rtc;

	oldme = layhead->box;
	me = oldme;
	bugflag  = FALSE;
	garbage = NULL;

	if (me->nextbox != NULL)
	{
		for(him = me->nextbox; me->nextbox != NULL;)
		{
			if (bugflag)
			{
				oldme = me;
				garbage->nextbox = NULL;
				db_mrg_trash(garbage);
				bugflag = FALSE;
			}

			/* now check rtc for results */
			broken = db_mrg_breakbox(me, him, &rtc);
			if (rtc == 1) /* if inclusion, remove me from list */
			{
				garbage = me;
				if (me == layhead->box) /* if me is first on list */
				{
					layhead->box = me->nextbox; /* bypass me on list */
					oldme = me;
					bugflag = TRUE;
				} else /* me is not the first on list */
				{
					oldme->nextbox = me->nextbox;

					/* back me up on list for correct entry on next iter. */
					me = oldme;
					garbage->nextbox = NULL;
					db_mrg_trash(garbage);
				}
			} else if (rtc == 2) /* independent... do nothing */
			{
				/* EMPTY */ 
			} else
			{
				/* rtc = 0, replace me on list with list returned by break */
				garbage = me;
				for(temp=broken; temp!=NULL; )  /*find eol */
				{
					endptr = temp;
					temp = temp->nextbox;
				}
				if (me == layhead->box) /* if me is first on list */
					layhead->box = broken;
						else oldme->nextbox = broken;
				endptr->nextbox = me->nextbox;
				me = endptr;
				garbage->nextbox = NULL;
				db_mrg_trash(garbage);
			}
			oldme=me;
			me=me->nextbox;
			him=me->nextbox;
		} /* end for */
	} /* end if */
}

BOXPOLY *db_mrg_breakbox(BOXPOLY *me, BOXPOLY *him, INTBIG *rtc)
{
	BOXPOLY *list, *oldme, *newlist;
	BOXPOLY *garbage, *temp, *endptr, *broken;
	INTBIG retc, calrtc;
	BOOLEAN notfinished = TRUE;

	newlist = NULL;
	while (notfinished)
	{
		list = db_mrg_do_break(me, him, &retc);
		if (retc == 2) /* if independent, do again with him->next */
			if (him->nextbox != NULL) him = him->nextbox; else
		{
			/* we are at the end of the him list and have nothing to return */
			if (newlist == NULL)
			{
				*rtc = 2;
				return(NULL);
			}
		} else if (retc == 1)
		{
			/* if an inclusion */
			if (newlist == NULL)
			{
				/* if empty list, let this guy be eaten up */
				*rtc = 1;
				return(NULL);
			} else
			{
				/* if we get here, we are in deep trouble */
				/* EMPTY */ 
			}
		} else /* if we got here, a break occurred..do something with list */
		{
			if (newlist == NULL) /* it has to be anyway */
			{
				newlist = list;
				if (him->nextbox == NULL) /* at end of hims, return code 0 */
				{
					*rtc = 0;
					return(newlist);
				} else /* must recurse across the list */
				{
					oldme = NULL;
					for (me=newlist; me!=NULL; )
					{
						/* do a break */
						broken = db_mrg_breakbox(me, him->nextbox, &calrtc);
						if (calrtc == 2) /* no break done, look at next guy */
						{
							oldme = me;
							me = me->nextbox;
						} else if (calrtc == 1)
						{
							/* me is an inclusion... remove me */
							garbage = me;
							if (me == newlist) /* if me is first on list */
							{
								newlist = me->nextbox;
								me = newlist; /* put me at top of loop again */
								if (me == NULL)
								{
									/* if me was only one left and got eaten */
									*rtc = 1;
									garbage->nextbox = NULL;
									db_mrg_trash(garbage);
									return(NULL);
								}
							} else /* me was not first on list */
							{
								/* oldme still behind me */
								oldme->nextbox = me->nextbox;
								me = me->nextbox;
							}
							garbage->nextbox = NULL;
							db_mrg_trash(garbage);
						} else
						{
							/* calrct=0, add list after newlist or after oldme */
							for(temp=broken;temp!=NULL;) /*get eol */
							{
								endptr=temp;
								temp=temp->nextbox;
							}
							endptr->nextbox = me->nextbox;
							if (me == newlist) /* if me is first on list */
							{
								garbage = me;
								newlist = broken;
								oldme = endptr;
								me = endptr->nextbox;
								garbage->nextbox = NULL;
								db_mrg_trash(garbage);
							} else /* me is not first on list */
							{
								garbage = me;
								oldme->nextbox = broken;
								oldme = endptr;
								me = endptr->nextbox;
								garbage->nextbox = NULL;
								db_mrg_trash(garbage);
							}
						} /* end if calrtc=0 */
					} /* end for */
					*rtc = 0;
					return(newlist);
				}
			}
		}
	} /* end while */
	return(NULL);
}

BOXPOLY *db_mrg_do_break(BOXPOLY *me, BOXPOLY *him, INTBIG *rtc)
{
	BOXPOLY *broken, *breakk, *temp;

	/* check for disjointedness */
	if (me->right <= him->left || me->top <= him->bot || me->left >= him->right ||
		me->bot >= him->top)
	{
		*rtc = 2;
		return(NULL);
	}

	/* check for inclusion */
	if (me->top <= him->top && me->left >= him->left && me->bot >= him->bot &&
		me->right <= him->right)
	{
		*rtc = 1;
		return(NULL);
	}

	/* check for identical boxes */
	if (me->top == him->top && me->bot == him->bot && me->left == him->left &&
		me->right == him->right)
	{
		*rtc = 1;
		return(NULL);
	}

	if (me->top > him->top && me->left < him->left && me->bot < him->bot
		&& me->right < him->right && me->right > him->left)
	{
		temp = db_mrg_getbox();
		db_mrg_assign_box(temp,me->left,me->right,me->top,him->top);
		broken = temp;
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,me->left,him->left,him->top,him->bot);
		breakk = db_mrg_getbox();
		temp->nextbox = breakk;
		db_mrg_assign_box(breakk,me->left,me->right,him->bot,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->bot < him->bot && me->left < him->left && me->right > him->right
		&& me->top > him->bot && me->top < him->top)
	{
		temp = db_mrg_getbox();
		db_mrg_assign_box(temp,me->left,him->left,me->top,me->bot);
		broken = temp;
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,him->left,him->right,him->bot,me->bot);
		breakk = db_mrg_getbox();
		temp->nextbox = breakk;
		db_mrg_assign_box(breakk,him->right,me->right,me->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->right > him->right && me->top > him->top && me->bot < him->bot
		&& me->left < him->right && me->left > him->left)
	{
		temp = db_mrg_getbox();
		db_mrg_assign_box(temp,me->left,me->right,me->top,him->top);
		broken = temp;
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,him->right,me->right,him->top,him->bot);
		breakk = db_mrg_getbox();
		temp->nextbox = breakk;
		db_mrg_assign_box(breakk,me->left,me->right,him->bot,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->left < him->left && me->right > him->right && me->top > him->top
		&& me->bot > him->bot && me->bot < him->top)
	{
		temp = db_mrg_getbox();
		db_mrg_assign_box(temp,me->left,him->left,me->top,me->bot);
		broken = temp;
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,him->left,him->right,me->top,him->top);
		breakk = db_mrg_getbox();
		temp->nextbox = breakk;
		db_mrg_assign_box(breakk,him->right,me->right,me->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (him->left <= me->left && him->top >= me->top && him->bot <= me->bot
		&& me->right > him->right && me->left < him->right)
	{
		broken = db_mrg_getbox();
		db_mrg_assign_box(broken,him->right,me->right,me->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->left < him->left && him->top >= me->top && him->bot <= me->bot
		&& him->right >= me->right && me->right > him->left)
	{
		broken = db_mrg_getbox();
		db_mrg_assign_box(broken,me->left,him->left,me->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (him->left <= me->left && him->right >= me->right && him->bot <= me->bot
		&& me->bot < him->top && me->top > him->top)
	{
		broken = db_mrg_getbox();
		db_mrg_assign_box(broken,me->left,me->right,me->top,him->top);
		*rtc = 0;
		return(broken);
	}

	if (him->left <= me->left && him->right >= me->right && him->top >= me->top
		&& me->top > him->bot && me->bot < him->bot)
	{
		broken = db_mrg_getbox();
		db_mrg_assign_box(broken,me->left,me->right,him->bot,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->top > him->top && me->left < him->left && me->bot < him->top &&
		me->bot >= him->bot && me->right > him->left && me->right <= him->right)
	{
		temp = db_mrg_getbox();
		broken = temp;
		db_mrg_assign_box(temp,me->left,me->right,me->top,him->top);
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,me->left,him->left,him->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->top > him->top && me->right > him->right && me->left >= him->left
		&& me->left < him->right && me->bot < him->top && me->bot >= him->bot)
	{
		temp = db_mrg_getbox();
		broken = temp;
		db_mrg_assign_box(temp,me->left,me->right,me->top,him->top);
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,him->right,me->right,him->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->right > him->right && me->bot < him->bot && me->left >= him->left
		&& me->left < him->right && me->top > him->bot && me->top <= him->top)
	{
		temp = db_mrg_getbox();
		broken = temp;
		db_mrg_assign_box(temp,him->right,me->right,me->top,him->bot);
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,me->left,me->right,him->bot,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->left < him->left && me->bot < him->bot && me->top > him->bot &&
		me->top <= him->top && me->right > him->left && me->right <= him->right)
	{
		temp = db_mrg_getbox();
		broken = temp;
		db_mrg_assign_box(temp,me->left,him->left,me->top,him->bot);
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,me->left,me->right,him->bot,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->left < him->left && me->right > him->right
		&& him->top >= me->top && him->bot <= me->bot)
	{
		temp = db_mrg_getbox();
		broken = temp;
		db_mrg_assign_box(temp,me->left,him->left,me->top,me->bot);
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,him->right,me->right,me->top,me->bot);
		*rtc = 0;
		return(broken);
	}

	if (me->top > him->top && me->bot < him->bot
		&& me->left >= him->left && me->right <= him->right)
	{
		temp = db_mrg_getbox();
		broken = temp;
		db_mrg_assign_box(temp,me->left,me->right,me->top,him->top);
		temp = db_mrg_getbox();
		broken->nextbox = temp;
		db_mrg_assign_box(temp,me->left,me->right,him->bot,me->bot);
		*rtc = 0;
		return(broken);
	}

	/* give the user a warning if this routine did not catch some case */
	ttyputerr(_("Warning: merging done incorrectly"));
	ttyputerr(x_("me: left = %ld, right = %ld, top = %ld, bot = %ld"),
		me->left, me->right, me->top, me->bot);
	ttyputerr(x_("him: left = %ld, right = %ld, top = %ld, bot = %ld"),
		him->left, him->right, him->top, him->bot);

	/* although this is in error, it will enable a completion of the write */
	*rtc = 1;
	return(NULL);
}

void db_mrg_write_lists(void (*writepolygon)(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG))
{
	BOXLISTHEAD *lay_box;
	BOXPOLY *boxscanptr, *garbage, *oldboxscan;
	BOOLEAN lay_not_done, merged;

	for(lay_box=db_mrg_curr_cell; lay_box!=NULL; lay_box=lay_box->nextlayer) /* for each layer... */
	{
		db_mrg_remove_inclusion(lay_box); /* remove inclusions down list */
		db_mrg_remove_overlap(lay_box); /* break into nonoverlapping boxes */
		db_mrg_remove_inclusion(lay_box);
		lay_not_done = TRUE;
/*		(*setlayer)(lay_box->layer, lay_box->tech); */
		db_mrg_polycoordlist.firstpoly = NULL;
		db_mrg_polycoordlist.numedge = 0;
		while (lay_not_done)
		{
			db_mrg_polycoordlist.modified = 0;
			for(boxscanptr=lay_box->box; boxscanptr != NULL; )
			{
				/* passing through list of boxes */
				merged = db_mrg_mergeboxwithpolygon(boxscanptr);
				if (merged) /* if list was modified */
				{
					/* set flag that we made a modification */
					db_mrg_polycoordlist.modified = 1;
					merged = FALSE; /* reset indicator flag */

					/* if box is first on list */
					if (boxscanptr == lay_box->box)
					{
						garbage = boxscanptr;
						lay_box->box = boxscanptr->nextbox;
						boxscanptr = lay_box->box;
						garbage->nextbox = NULL;
						db_mrg_trash(garbage);

						/* if NULL list, we are done */
						if (lay_box->box == NULL)
						{
							lay_not_done = FALSE;
							break;
						}
					} else
					{
						/* not the first on list */
						garbage = boxscanptr;

						/* LINTED "oldboxscan" used in proper order */
						oldboxscan->nextbox = boxscanptr->nextbox;
						boxscanptr = boxscanptr->nextbox;
						garbage->nextbox = NULL;
						db_mrg_trash(garbage);
					}
				} else
				{
					/* no merge done, check next box */
					oldboxscan = boxscanptr;
					boxscanptr = boxscanptr->nextbox;
				}

				/* allow maxnumpts as a max */
				if (db_mrg_polycoordlist.numedge > maxnumpts) break;
			} /* end for */

			/*
			 * write out box if: (1) entire pass yielded no merges
			 * (2) end of list (3) too many points on list
			 */
			if ((!db_mrg_polycoordlist.modified) || (!lay_not_done) ||
				(db_mrg_polycoordlist.numedge > maxnumpts))
			{
				db_mrg_write_polygon(writepolygon, lay_box->layer, lay_box->tech);
				db_mrg_remove_poly_list(); /* dispose of polygon records */

				/* re-initialize for next pass */
				db_mrg_polycoordlist.firstpoly = NULL;
				db_mrg_polycoordlist.numedge = 0;
				db_mrg_polycoordlist.modified = 0;
			}
		} /* end while */
	}
}

/* Macro to insert edge on polygon */
# define INSERT_EDGE(pred,tmp,copy,succ,dmaep1,dmaep2) \
  tmp = db_mrg_getpoly(); \
  tmp->nextpoly = succ; \
  if (pred != NULL) {pred->nextpoly = tmp;} \
  db_mrg_assign_edge(tmp, dmaep1, dmaep2); \
  copy = tmp;

/* Macro to remove n edges (nedge) from polygon */
# define REMOVE_EDGE(pred,start,tmp,nedge) \
  for (i = 0; i < nedge; i++) { \
	  tmp = start->nextpoly; \
	  pred->nextpoly = tmp; \
	  if (start == db_mrg_polycoordlist.firstpoly) { \
		  db_mrg_polycoordlist.firstpoly = tmp; \
	  } \
	  db_mrg_killpoly(start); \
	  start = tmp; \
  }

BOOLEAN db_mrg_mergeboxwithpolygon(BOXPOLY *boxptr)
{
	POLYCOORD *polyptr,*creatptr,*backptr;
	BOOLEAN pair, ok, one_con, err;
	INTBIG i, firstcon, connect, auxx, ed_chk, j, ed_con;

	pair = FALSE; /* flag to indicate a pair */
	one_con = FALSE; /* flas to intdicate that intersection was found */
	boxptr->numsides = 0;
	ok = FALSE;
	backptr = NULL;

	/* initialise seen field if list exists */
	if (db_mrg_polycoordlist.firstpoly != NULL)
	{
		/* initialize pointer to list */
		polyptr = db_mrg_polycoordlist.firstpoly;
		for(i=0; i!=db_mrg_polycoordlist.numedge;)
		{
			polyptr->seen = 1;
			i++;
			polyptr = polyptr->nextpoly;
		}
	}
	for(i=0; i<4; i++)
	{
		boxptr->numint[i] = 0;
		boxptr->polyint[i]=boxptr->auxpolyint[i] = NULL;
	}
	if (db_mrg_polycoordlist.firstpoly == NULL) /* if nothing on list */
	{
		/* make first edge (at head) */
		INSERT_EDGE(backptr,creatptr,backptr,NULL,boxptr->ll,boxptr->ul)
		db_mrg_polycoordlist.firstpoly = creatptr;

		/* second edge */
		INSERT_EDGE(backptr,creatptr,backptr,NULL,boxptr->ul,boxptr->ur)

		/* third edge */
		INSERT_EDGE(backptr,creatptr,backptr,NULL,boxptr->ur,boxptr->lr)

		/* fourth edge (and form ring) */
		INSERT_EDGE(backptr,creatptr,backptr,db_mrg_polycoordlist.firstpoly,boxptr->lr,boxptr->ll)

		db_mrg_housekeeper(); /* set up edge numbers */;
		return(TRUE);  /* all done */
	} else /* we have some edges already */
	{
		for(polyptr=db_mrg_polycoordlist.firstpoly; polyptr->seen; /* check for intersection to all edges */
			polyptr=polyptr->nextpoly)
		{
			polyptr->seen = 0; /* indicate that we have been here */
			connect = db_mrg_check_intersect(boxptr, polyptr, &ed_con);
			if (connect == PTMATCH) /* if a point intersection was found */
				return(FALSE); /* point intersections are not permitted */
			if (connect) /* if connection was found */
			{
				one_con = TRUE; /* found at least 1 connection */

				/* > 2 connections on one side */
				if (boxptr->numint[ed_con] == 2) return(FALSE);
				if (pair && boxptr->polyint[ed_con] != NULL) /* 2 pairs found */
					return(FALSE);
				if (boxptr->polyint[ed_con] == NULL) /* if first connection */
				{
					/* link edge to polygon descriptor */
					boxptr->polyint[ed_con] = polyptr;
					boxptr->numint[ed_con] += 1; /* increment count for side */

					/* increment number sides intersected */
					boxptr->numsides += 1;
				} else /* second connection for one "first" edge */
				{
					/* link to poly descriptor */
					boxptr->auxpolyint[ed_con] = polyptr;

					/* increment side connection count */
					boxptr->numint[ed_con] += 1;
					pair = TRUE; /* we have found a pair */
				}
			} /* end if connect */
		} /* end for */

		if (!one_con) /* if no connections found */
			return(FALSE);
		else /* found at least one connection */
		{
			/* based on #sides of box which touched polygon */
			switch (boxptr->numsides)
			{
				case 4: /* intersects on 4 edges */
					if (pair) /* intersects 1 edge twice */
					{
						/* find edge which intersects twice */
						for(i=0; boxptr->numint[i]!=2; i++)
							;
						firstcon = i;
						if ((boxptr->polyint[db_mrg_nxtside(i)]->indx) ==
							db_mrg_succ(boxptr->polyint[i]->indx))
								auxx = 0;  /* check the statement below */
						else if((boxptr->polyint[db_mrg_nxtside(i)]->indx) ==
							db_mrg_succ(boxptr->auxpolyint[i]->indx))
								auxx = 1;
						else /* neither edge connection had next side with correct index number */
							return(FALSE);
						if (auxx) /* if auxiliary pointer is the start */
						{
							for(ed_chk=db_mrg_nxtside(i); ed_chk!=i;
								ed_chk=db_mrg_nxtside(ed_chk))
									if ((boxptr->polyint[db_mrg_nxtside(ed_chk)]->indx) ==
										db_mrg_succ(boxptr->polyint[ed_chk]->indx))
											continue;
									else
										/* successor had wrong index number */
										return(FALSE);

							/* if we got here all is well */
							db_mrg_insert5(firstcon, auxx, boxptr);
						} else /* auxx = 0 */
						{
							for(ed_chk=db_mrg_nxtside(i); ed_chk != i;
								ed_chk=db_mrg_nxtside(ed_chk))
							{
								/* check for last edge */
								if (ed_chk != db_mrg_prevside(firstcon))
								{
									if (boxptr->polyint[db_mrg_nxtside(
										ed_chk)]->indx == db_mrg_succ(
											boxptr->polyint[ed_chk]->indx))
												continue;
									return(FALSE);	/* index numbers out of sequence */
								} else
								{
									if ((boxptr->auxpolyint[db_mrg_nxtside(
										ed_chk)]->indx) == (db_mrg_succ(
										boxptr->polyint[ed_chk]->indx)))
											continue;
									return(FALSE);	/* index numbers out of sequence */
								}
							}
							db_mrg_insert5(firstcon, auxx, boxptr);
						}
					} else /* intersects 4 edges, 1 time each edge */
					{
						ok = FALSE;

						/* find "start" edge of intersect sequence */
						for(i=0;i<4;i++)
						{
							err = FALSE;
							ed_chk = i;
							for(j = 0; j < 3; j++)
							{
								if ((boxptr->polyint[db_mrg_nxtside(
									ed_chk)]->indx) != (db_mrg_succ(
										boxptr->polyint[ed_chk]->indx)))
											err = TRUE; /* error found */
								ed_chk = db_mrg_nxtside(ed_chk);
							}
							if (!err)
							{
								ok = TRUE;
								break;
							}
						} /* end for */
						if (ok) /* check if we found a correct sequence */
						{
							firstcon = i;
							db_mrg_insert4(firstcon,boxptr);
						} else
							return(FALSE); /* index out of sequence */
					}
					break;

				case 3:
					/* if a pair of contacts on 1 edge of box ... loop found */
					if (pair) return(FALSE);
						else
					{
						/* i is index of edge which doesn't intersect polygon */
						for(i = 0; i < 4; i++)
							if (boxptr->polyint[i] == NULL) break;

						/* index of first non null side */
						firstcon = db_mrg_nxtside(i);
						for(j = firstcon; j != db_mrg_prevside(i);
							j = db_mrg_nxtside(j))
								if ((boxptr->polyint[db_mrg_nxtside(j)]->indx) !=
									(db_mrg_succ(boxptr->polyint[j]->indx)))
										return(FALSE);
						db_mrg_insert3(firstcon,boxptr);
					}
					break;

				case 2: /* box has 2 edges which intersect polygon */
					if (pair) /* the usual */
						return(FALSE);
					else
					{
						for(i = 0; i < 4; i++) /* find a NULL edge */
							if (boxptr->polyint[i] == NULL) break;
						for(j = i; boxptr->polyint[j] == NULL;
							j = db_mrg_nxtside(j))
								;
						firstcon = j; /* first edge connection */

						/* have detected nonsadjacent pair of intersections */
						if (boxptr->polyint[db_mrg_nxtside(firstcon)] == NULL)
							return(FALSE);
						if ((boxptr->polyint[db_mrg_nxtside(j)]->indx) !=
							(db_mrg_succ(boxptr->polyint[j]->indx)))
								return(FALSE); /* index numbers out of sequence... */
						db_mrg_insert2(firstcon,boxptr);
					}
					break;

				case 1: /* one edge intersects */
					if (pair) /* if a pair, we have a loop */
						return(FALSE);
					else
					{
						/* find edge which intersects */
						for(i=0; boxptr->polyint[i]==NULL; i++)
							;
						firstcon = i;
						db_mrg_insert1(firstcon,boxptr);
					}
					break;

				default:
					break;
			} /* end switch */

			db_mrg_housekeeper();
			return(TRUE); /* successful addition of box to polygon */
		} /* end else */
	} /* end have edges already */
}

/* returns 0 if no intersection,else 1 (line intersection) or 2 (point intersection) */
INTBIG db_mrg_check_intersect(BOXPOLY *boxptr, POLYCOORD *polyptr, INTBIG *ed_con)
{
	switch (polyptr->ed_or)
	{
		case UP:
			if (boxptr->right == polyptr->ed_val) /* may be colinear */
			{
				if ((boxptr->bot >= polyptr->ed_start[1] &&
					boxptr->bot < polyptr->ed_end[1]) ||
						(boxptr->top <= polyptr->ed_end[1] &&
							boxptr->top > polyptr->ed_start[1]) ||
								(boxptr->bot < polyptr->ed_start[1] &&
									boxptr->top > polyptr->ed_end[1]))
				{
					*ed_con = DOWN;
					return(1);
				}
				return(db_mrg_check_point(polyptr->ed_start,boxptr->ur,
					polyptr->ed_end,boxptr->lr)); /* check for pt connect */
			}
			return(0);

		case RIGHT:
			if (boxptr->bot == polyptr->ed_val)
			{
				if ((boxptr->left >= polyptr->ed_start[0] &&
					boxptr->left < polyptr->ed_end[0]) ||
						(boxptr->right > polyptr->ed_start[0] &&
							boxptr->right <= polyptr->ed_end[0]) ||
								(boxptr->left < polyptr->ed_start[0] &&
									boxptr->right > polyptr->ed_end[0]))
				{
					*ed_con = LEFT;
					return(1);
				}
				return(db_mrg_check_point(polyptr->ed_start,boxptr->lr,
					polyptr->ed_end,boxptr->ll));
			}
			return(0);

		case DOWN:
			if (boxptr->left == polyptr->ed_val)
			{
				if ((boxptr->bot >= polyptr->ed_end[1] &&
					boxptr->bot < polyptr->ed_start[1]) ||
						(boxptr->top > polyptr->ed_end[1] &&
							boxptr->top <= polyptr->ed_start[1]) ||
								(boxptr->top > polyptr->ed_start[1] &&
									boxptr->bot < polyptr->ed_end[1]))
				{
					*ed_con = UP;
					return(1);
				}
				return(db_mrg_check_point(polyptr->ed_start,boxptr->ll,
					polyptr->ed_end,boxptr->ul));
			}
			return(0);

		case LEFT:
			if (boxptr->top == polyptr->ed_val)
			{
				if ((boxptr->left >= polyptr->ed_end[0] &&
					boxptr->left < polyptr->ed_start[0]) ||
						(boxptr->right > polyptr->ed_end[0] &&
							boxptr->right <= polyptr->ed_start[0]) ||
								(boxptr->left < polyptr->ed_end[0] &&
									boxptr->right > polyptr->ed_start[0]))
				{
					*ed_con = RIGHT;
					return(1);
				}
				return(db_mrg_check_point(polyptr->ed_start,boxptr->ul,
					polyptr->ed_end,boxptr->ur));
			}
			return(0);
	} /* end switch */
	return(0);
}

/* PoinT EQual macro */

# define PTEQ(x,y) \
(((x)[0] == (y)[0]) && ((x)[1] == (y)[1]))

/* checks if points a1, a2 match or if points b1, b2 match */
INTBIG db_mrg_check_point(INTBIG pointa1[], INTBIG pointa2[], INTBIG pointb1[],
	INTBIG pointb2[])
{
	if (PTEQ(pointa1, pointa2) || PTEQ(pointb1, pointb2)) {
		return(PTMATCH); /* at least one pair matches */
	}
	return(0); /* neither pair matches */
}

void db_mrg_insert1(INTBIG firstcon, BOXPOLY *boxptr)
{
	POLYCOORD *polyptr, *nextedge, *creatptr, *backptr;
	INTBIG *p0, *p1, *p2, *p3;

	/* get pointer to edge of intersection */
	polyptr = boxptr->polyint[firstcon];
	nextedge = polyptr->nextpoly;
	for(backptr = db_mrg_polycoordlist.firstpoly; backptr->nextpoly != polyptr;
		backptr = backptr->nextpoly) /* gat pointer to edge before polyptr */
			;

	switch (polyptr->ed_or) { /* based on polygon edge orientation */
		case UP:
			/*  p3 --- p1  |
				|      \/  ^
				|      \/  ^
				p2 --- p0  |
			*/
			p0 = boxptr->lr;
			p1 = boxptr->ur;
			p2 = boxptr->ll;
			p3 = boxptr->ul;
			break;

		case RIGHT:
			/*  p2 --- p3
				|      |
				|      |
				p0 -<- p1
				--->>>---
			 */
			p0 = boxptr->ll;
			p1 = boxptr->lr;
			p2 = boxptr->ul;
			p3 = boxptr->ur;
			break;

		case DOWN:
			/*  | p0 --- p2
			   \/ ^      |
			   \/ ^      |
				| p1 --- p3
			 */
			p0 = boxptr->ul;
			p1 = boxptr->ll;
			p2 = boxptr->ur;
			p3 = boxptr->lr;
			break;

		case LEFT:
			/* ---<<<---
			   p1 ->- p0
			   |      |
			   |      |
			   p3 --- p2
			 */
			p0 = boxptr->ur;
			p1 = boxptr->ul;
			p2 = boxptr->lr;
			p3 = boxptr->ll;
			break;

		default:
			break;
	} /* end switch */
	/*
	  Generic code to handle case of 1 box edge touching 1 polygon edge
	  p0: point on box which might intersect start point of edge
	  p1: point on box which might intersect end point of edge
	  p2: point on box which is diagonally opposite p1
	  p3: point on box which is diagonally opposite p0
	  */

	/* if first point matches */
	if PTEQ(polyptr->ed_start, p0) {
		/* if second point matches */
		if PTEQ(polyptr->ed_end, p1) {
			/* both points match (adjust edge start/end points) */
			db_mrg_assign_edge(backptr, backptr->ed_start, p2);
			db_mrg_assign_edge(polyptr, p2, p3);
			db_mrg_assign_edge(nextedge, p3, nextedge->ed_end);
		}
		else { /* only first point matches (adjust 2 edges, add 2 edges) */
			db_mrg_assign_edge(backptr, backptr->ed_start, p2);
			db_mrg_assign_edge(polyptr, p2, p3);
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p3, p1)
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p1, nextedge->ed_start)
		}
	}
	else { /* first point does not match */
		/* if second point matches */
		if PTEQ(polyptr->ed_end, p1) {
			/* only second point matches (adjust 2 edges, add 2 edges) */
			db_mrg_assign_edge(polyptr,polyptr->ed_start, p0);
			db_mrg_assign_edge(nextedge, p3,nextedge->ed_end);
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p0, p2)
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p2, p3)
		}
		else {
			/* neither point matches (adjust first edge, add 4 new edges) */
			db_mrg_assign_edge(polyptr,polyptr->ed_start, p0);
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p0, p2)
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p2, p3)
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p3, p1)
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge, p1, nextedge->ed_start)
		}
	}
}

/* inserts box into polygon where 2 edges touched */
void db_mrg_insert2(INTBIG firstcon, BOXPOLY *boxptr)
{
	POLYCOORD *polyptr, *nextedge, *creatptr, *backptr;
	INTBIG *p0, *p1, *p2;

	/* assign polyptr to first edge where intersection occurs */
	polyptr = boxptr->polyint[firstcon];
	nextedge = polyptr->nextpoly; /* get pointer to next edge */

	switch (polyptr->ed_or) { /* based on polygon edge direction */
		case RIGHT:
		/*
			p2-p1 |
			|  |  ^
			p0-|  ^
				  |
			-->>--|
		 */
			p0 = boxptr->ll;
			p1 = boxptr->ur;
			p2 = boxptr->ul;
			break;

		case LEFT:
		/*
			--<<--
			| |--p0
		   \/ |   |
		   \/ |   |
			| p1-p2
		 */
			p0 = boxptr->ur;
			p1 = boxptr->ll;
			p2 = boxptr->lr;
			break;

		case UP:
		/*
			--<<--|
				  |
			p1--| ^
			|   | ^
			|   | |
			p2-p0 |
		 */
			p0 = boxptr->lr;
			p1 = boxptr->ul;
			p2 = boxptr->ll;
			break;

		case DOWN:
		/*
			 | p0--p2
			\/ |   |
			\/ |   |
			 | |---p1
			 |
			 -->>----
		 */
			p0 = boxptr->ul;
			p1 = boxptr->lr;
			p2 = boxptr->ur;
			break;

		default:
			break;
	} /* end switch */
	/*
	  Generic code to handle case of 2 box edges touching 2 polygon edges
	  p0: point on box which might intersect start point of firsst polygon edge
	  p1: point on box which might intersect end point of second polygon edge
	  p2: point on box which is diagonally opposite "corner" of intersection of polygon edges
	  */

	/* if first point matches */
	if PTEQ(polyptr->ed_start, p0) {
		/* if second point matches */
		if PTEQ(nextedge->ed_end, p1) {
			/* both points match */
			db_mrg_assign_edge(polyptr,polyptr->ed_start,p2);
			db_mrg_assign_edge(nextedge,p2,nextedge->ed_end);
		}
		else {
			/* first matches only */
			for(backptr = db_mrg_polycoordlist.firstpoly;
				backptr->nextpoly != polyptr;
				backptr = backptr->nextpoly)
					;
			db_mrg_assign_edge(backptr,backptr->ed_start,p2);
			db_mrg_assign_edge(polyptr,p2,p1);
			db_mrg_assign_edge(nextedge,p1,nextedge->ed_end);
		}
	}
	else { /* first point doesnt match */
		/* second point matches */
		if PTEQ(nextedge->ed_end, p1) {
			db_mrg_assign_edge(polyptr,polyptr->ed_start,p0);
			db_mrg_assign_edge(nextedge,p0,p2);
			nextedge = nextedge->nextpoly;
			db_mrg_assign_edge(nextedge,p2,nextedge->ed_end);
		}
		else {
			/* neither point touches */
			db_mrg_assign_edge(polyptr,polyptr->ed_start,p0);
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge,p0,p2)
			INSERT_EDGE(polyptr,creatptr,polyptr,nextedge,p2,p1)
			db_mrg_assign_edge(nextedge,p1,nextedge->ed_end);
		}
	}
}

void db_mrg_insert3(INTBIG firstcon, BOXPOLY *boxptr)
{
	POLYCOORD *polyptr, *nextedge, *garbage, *backptr, *temp;
	INTBIG i;
	INTBIG *p0, *p1;

	/* assign polyptr to first edge */
	polyptr = boxptr->polyint[firstcon];
	nextedge = polyptr->nextpoly;
	nextedge = nextedge->nextpoly; /* third edge which intersects */
	for(backptr = db_mrg_polycoordlist.firstpoly; backptr->nextpoly != polyptr;
		backptr = backptr->nextpoly)  /* get back pointer */
			;

	switch (polyptr->ed_or) { /* based on polygon edge direction */
		case RIGHT:
		/*
			---<<---|
					|
			p1----| |
			|     | ^
			|     | ^
			p0----| |
					|
			--->>---|
		 */
			p0 = boxptr->ll;
			p1 = boxptr->ul;
			break;

		case LEFT:
		/*
		   |---<<---
		   |
		   | |----p0
		  \/ |     |
		  \/ |     |
		   | |----p1
		   |
		   |--->>---
		 */
			p0 = boxptr->ur;
			p1 = boxptr->lr;
			break;

		case UP:
		/*
		  |---<<----|
		  |         |
		  | |-----| |
		 \/ |     | ^
		 \/ |     | ^
		  | p1---p0 |
		  |         |
		 */
			p0 = boxptr->lr;
			p1 = boxptr->ll;
			break;

		case DOWN:
		/*
		  |         |
		  | p0---p1 |
		 \/ |     | ^
		 \/ |     | ^
		  | |-----| |
		  |         |
		  |--->>----|
		 */
			p0 = boxptr->ul;
			p1 = boxptr->ur;
			break;

		default:
			break;
	} /* end switch */
	/*
	  Generic code to handle case of 3 box edges touching 3 polygon edges
	  p0: point on box which might intersect start point of first polygon edge
	  p1: point on box which might intersect end point of third polygon edge
	  */

	/* if first point matches */
	if PTEQ(polyptr->ed_start, p0) {
		/* if second point matches */
		if PTEQ(nextedge->ed_end, p1) {
		  /* both points match */
			nextedge = nextedge->nextpoly;
			db_mrg_assign_edge(backptr,backptr->ed_start,
							   nextedge->ed_end);
			REMOVE_EDGE(backptr,polyptr,garbage,4) /* remove 4 edges beginning with polyptr */
		}
		else { /* first point matches only */
			db_mrg_assign_edge(backptr,backptr->ed_start,p1);
			db_mrg_assign_edge(nextedge,p1,nextedge->ed_end);
			REMOVE_EDGE(backptr,polyptr,garbage,2) /* remove 2 edges beginning with polyptr */
		}
	}
	else { /* first point doesn't match */
		if PTEQ(nextedge->ed_end, p1) {
			/* second point matches only */
			garbage = polyptr->nextpoly;
			nextedge = nextedge->nextpoly;
			db_mrg_assign_edge(polyptr,polyptr->ed_start,p0);
			db_mrg_assign_edge(nextedge,p0,nextedge->ed_end);
			REMOVE_EDGE(polyptr,garbage,temp,2) /* remove 2 edges beginning with garbage */
		}
		else { /* neither point matches */
			db_mrg_assign_edge(polyptr,polyptr->ed_start,p0);
			temp = polyptr->nextpoly;
			db_mrg_assign_edge(temp,p0,p1);
			db_mrg_assign_edge(nextedge,p1,nextedge->ed_end);
		}
	}
}

/* inserts box where each edge intersected once */
void db_mrg_insert4(INTBIG firstcon, BOXPOLY *boxptr)
{
	INTBIG i;
	POLYCOORD *polyptr, *nextedge, *garbage, *temp, *backptr;
	INTBIG *p0;
	INTBIG descendp; /* for the below/above/rightof/leftof case */

	polyptr = boxptr->polyint[firstcon];
	nextedge = boxptr->polyint[db_mrg_prevside(firstcon)];
	for(backptr = db_mrg_polycoordlist.firstpoly; backptr->nextpoly != polyptr;
		backptr = backptr->nextpoly)  /* get back pointer */
			;

	switch (polyptr->ed_or) { /* based on polygon edge direction */
		case RIGHT:
		/*
		   |----<<---|
		   |         |
		  \/ |-----| |
		  \/ |     | ^
		   | |     | ^
			 p0----| |
					 |
			 --->>---|
		 */
			p0 = boxptr->ll;
			descendp = ((nextedge->ed_end[1] < polyptr->ed_start[1]) ? 1 : 0);
			break;

		case LEFT:
		/*
		   |---<<---
		   |
		   | |----p0
		  \/ |     | |
		  \/ |     | ^
		   | |-----| ^
		   |         |
		   |--->>----|
		 */
			p0 = boxptr->ur;
			descendp = ((nextedge->ed_end[1] > polyptr->ed_start[1]) ? 1 : 0);
			break;

		case UP:
		/*
		  |---<<----|
		  |         |
		  | |-----| |
		 \/ |     | ^
		 \/ |     | ^
		  | |----p0 |
		  |         |
		  |--->>-
		 */
			p0 = boxptr->lr;
			descendp = ((nextedge->ed_end[0] > polyptr->ed_start[0]) ? 1 : 0);
			break;

		case DOWN:
		/*
			  -<<---|
		  |         |
		  | p0---p1 |
		 \/ |     | ^
		 \/ |     | ^
		  | |-----| |
		  |         |
		  |--->>----|
		 */
			p0 = boxptr->ul;
			descendp = ((nextedge->ed_end[0] < polyptr->ed_start[0]) ? 1 : 0);
			break;

		default:
			break;
	} /* end switch */
	/*
	  Generic code to handle case where 4 polygon edges perfectly enclose
	  (not necessarily fullythough) box
	  p0: start point on box which intersects first edge on polygon
	  */

	/* if first point matches */
	if PTEQ(polyptr->ed_start, p0) {
		db_mrg_assign_edge(backptr,backptr->ed_start,nextedge->ed_end);
		REMOVE_EDGE(backptr,polyptr,garbage,4) /* remove 4 edges beginning with polyptr */
	}
	else if PTEQ(nextedge->ed_end, p0) { /* if second point */
		nextedge = nextedge->nextpoly;
		db_mrg_assign_edge(polyptr,polyptr->ed_start,nextedge->ed_end);
		temp = polyptr->nextpoly;
		REMOVE_EDGE(polyptr,temp,garbage,4) /* remove 4 edges following polyptr */
	}
	else if (descendp) {
		/* this is a weird case (next->end "below"/"above"/"rightof"/"leftof" p0) */
		db_mrg_assign_edge(nextedge,p0,nextedge->ed_end);
		db_mrg_assign_edge(polyptr,polyptr->ed_start,p0);
		temp = polyptr->nextpoly;
		REMOVE_EDGE(polyptr,temp,garbage,2) /* remove 2 edges following polyptr */
	}
	else { /* more normal case */
		db_mrg_assign_edge(polyptr,polyptr->ed_start,p0);
		temp = polyptr->nextpoly;
		db_mrg_assign_edge(nextedge,p0,nextedge->ed_end);
		REMOVE_EDGE(polyptr,temp,garbage,2) /* remove 2 edges following polyptr */
	}
}

/* write polygon */
void db_mrg_write_polygon(void (*writepolygon)(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG),
	INTBIG layer, TECHNOLOGY *tech)
{
	POLYCOORD *polyptr;
	INTBIG i;

	if (db_mrg_polycoordlist.numedge > db_mrgbuflen)
	{
		/* verify adequate buffer space */
		if (db_mrgbuflen != 0)
		{
			efree((CHAR *)db_mrgxbuf);
			efree((CHAR *)db_mrgybuf);
		}
		db_mrgbuflen = db_mrg_polycoordlist.numedge;
		db_mrgxbuf = (INTBIG *)emalloc((db_mrgbuflen * SIZEOFINTBIG), db_cluster);
		db_mrgybuf = (INTBIG *)emalloc((db_mrgbuflen * SIZEOFINTBIG), db_cluster);
	}

	polyptr = db_mrg_polycoordlist.firstpoly;
	for(i=0; i<db_mrg_polycoordlist.numedge; i++)
	{
		db_mrgxbuf[i] = polyptr->ed_start[0];
		db_mrgybuf[i] = polyptr->ed_start[1];
		polyptr = polyptr->nextpoly; /* increment pointer */
	}
	(*writepolygon)(layer, tech, db_mrgxbuf, db_mrgybuf, db_mrg_polycoordlist.numedge);
}

/* returns successor polygon edge # */
INTBIG db_mrg_succ(INTBIG aindex)
{
	if (aindex == (db_mrg_polycoordlist.numedge - 1))
		return(0);
	return(aindex + 1);
}

/* returns index number of next edge on box, in CCW order */
INTBIG db_mrg_nxtside(INTBIG aindex)
{
	if (aindex == 0) return(3);
	return(aindex - 1);
}

/* returns index number of previous edge on box */
INTBIG db_mrg_prevside(INTBIG aindex)
{
	if (aindex == 3) return(0);
	return(aindex+1);
}

/* assigns characteristics to polygon edge */
void db_mrg_assign_edge(POLYCOORD *polyptr, INTBIG start[], INTBIG end[])
{
	INTBIG i;

	if (start[0] == end[0]) /* if same in X */
	{
		polyptr->ed_val = start[0];
		if (start[1] < end[1]) polyptr->ed_or = UP;
			else polyptr->ed_or = DOWN;
	} else /* same in Y */
	{
		polyptr->ed_val = start[1];
		if (start[0] < end[0]) polyptr->ed_or = RIGHT;
			else polyptr->ed_or = LEFT;
	}
	for(i = 0; i < 2; i++)
	{
		polyptr->ed_start[i] = start[i];
		polyptr->ed_end[i] = end[i];
	}
}

/* corrects for co-linear edges, and updates coordinate indices */
void db_mrg_housekeeper(void)
{
	POLYCOORD *polyptr, *garbage, *nextedge;
	INTBIG indx;

	for(polyptr = db_mrg_polycoordlist.firstpoly;
		polyptr->nextpoly != db_mrg_polycoordlist.firstpoly;
			polyptr = polyptr->nextpoly)
	polyptr->seen = 1;
	polyptr->seen = 1; /* do last on one ring */
	polyptr = db_mrg_polycoordlist.firstpoly;
	for(nextedge = polyptr->nextpoly; polyptr->seen;)
	{
		if (polyptr->ed_or == nextedge->ed_or) /* if edge orientation is same */
		{
			garbage = nextedge;
			polyptr->ed_end[0] = nextedge->ed_end[0]; /* copy end point */
			polyptr->ed_end[1] = nextedge->ed_end[1];

			/* if about to remove list head */
			if (nextedge == db_mrg_polycoordlist.firstpoly)
				db_mrg_polycoordlist.firstpoly = nextedge->nextpoly;
			polyptr->nextpoly = nextedge->nextpoly;
			nextedge = nextedge->nextpoly;
			garbage->nextpoly = NULL;
			db_mrg_killpoly(garbage);
		} else
		{
			/* not colinear, set as seen */
			polyptr->seen = 0;
			polyptr = nextedge;
			nextedge = nextedge->nextpoly;
		}
	} /* colinearities removed */

	/* update sequence numbers */
	indx = 0;
	for(polyptr = db_mrg_polycoordlist.firstpoly; polyptr->nextpoly !=
		db_mrg_polycoordlist.firstpoly; polyptr = polyptr->nextpoly)
	{
		polyptr->indx = indx;
		indx += 1;
	}
	polyptr->indx = indx; /* do last one */
	db_mrg_polycoordlist.numedge = indx + 1; /* set number of edges */
}

/* inserts box into polygon where intersected 5* */
void db_mrg_insert5(INTBIG firstcon, INTBIG auxx, BOXPOLY *boxptr)
{
	POLYCOORD *polyptr, *nextedge, *garbage, *temp;
	INTBIG i;

	if (auxx) { /* if auxptr points to first intersection */
		polyptr = boxptr->auxpolyint[firstcon];
		nextedge = boxptr->polyint[firstcon];
	} else {
		polyptr = boxptr->polyint[firstcon];
		nextedge = boxptr->auxpolyint[firstcon];
	}

	/* insertion position is independent of edge orientation */
	db_mrg_assign_edge(polyptr,polyptr->ed_start,nextedge->ed_end);
	temp = polyptr->nextpoly;
	REMOVE_EDGE(polyptr,temp,garbage,4) /* remove 4 edges following polyptr */
}

/* some support stuff to do box poly descriptor mem management. */
BOXPOLY *db_mrg_getbox(void)
{
	BOXPOLY *b;

	if (db_mrg_free_box_list == NULL) {
		b = (BOXPOLY *)emalloc(sizeof(BOXPOLY), db_cluster);
		if (b == 0) return(NULL);
		b->nextbox = NULL;
		return(b);
	}
	b = db_mrg_free_box_list;
	db_mrg_free_box_list = b->nextbox;
	b->nextbox = NULL;
	return(b);
}

void db_mrg_trash(BOXPOLY *b)
{
	b->nextbox = db_mrg_free_box_list;
	db_mrg_free_box_list = b;
}

void db_mrg_dump_free_box_list(void)
{
	BOXPOLY *b;

	while (db_mrg_free_box_list != NULL) {
		b = db_mrg_free_box_list;
		db_mrg_free_box_list = b->nextbox;
		b->nextbox = NULL;
		efree((CHAR *)b);
	}
}

void db_mrg_assign_box(BOXPOLY *ptr, INTBIG left, INTBIG right, INTBIG top, INTBIG bot)
{
	ptr->ul[0] = ptr->ll[0] = ptr->left = left;
	ptr->ur[0] = ptr->lr[0] = ptr->right = right;
	ptr->ul[1] = ptr->ur[1] = ptr->top = top;
	ptr->ll[1] = ptr->lr[1] = ptr->bot = bot;
	if ((ptr->right - ptr->left) > (ptr->top - ptr->bot)) {
		ptr->ishor = 1;
	}
	else {
		ptr->ishor = 0;
	}
}

POLYCOORD *db_mrg_getpoly(void)
{
	POLYCOORD *p;

	if (db_mrg_free_poly_list == NULL) {
		p = (POLYCOORD *)emalloc(sizeof(POLYCOORD), db_cluster);
		if (p == 0) return(NULL);
		p->nextpoly = NULL;
		return(p);
	}
	p = db_mrg_free_poly_list;
	db_mrg_free_poly_list = p->nextpoly;
	p->nextpoly = NULL;
	return(p);
}

void db_mrg_killpoly(POLYCOORD *p)
{
	p->nextpoly = db_mrg_free_poly_list;
	db_mrg_free_poly_list = p;
}

void db_mrg_dump_free_poly_list(void)
{
	POLYCOORD *p;

	while (db_mrg_free_poly_list != NULL) {
		p = db_mrg_free_poly_list;
		db_mrg_free_poly_list = p->nextpoly;
		p->nextpoly = NULL;
		efree((CHAR *)p);
	}
}

void db_mrg_remove_poly_list(void)
{
	POLYCOORD *polyptr,*endptr;

	/* find end of list and break ring */
	for(endptr = db_mrg_polycoordlist.firstpoly;
		endptr->nextpoly != db_mrg_polycoordlist.firstpoly;
		endptr = endptr->nextpoly)
			;

	/* ring is broken now */
	endptr->nextpoly = NULL;
	while (db_mrg_polycoordlist.firstpoly != NULL) {
		polyptr = db_mrg_polycoordlist.firstpoly;
		db_mrg_polycoordlist.firstpoly = polyptr->nextpoly;
		polyptr->nextpoly = NULL;
		db_mrg_killpoly(polyptr);
	}
}

BOXLISTHEAD *db_mrg_get_lay_head(void)
{
	BOXLISTHEAD *bl;

	if (db_mrg_free_boxhead_list == NULL) {
		bl = (BOXLISTHEAD *)emalloc(sizeof(BOXLISTHEAD), db_cluster);
		if (bl == 0) return(NULL);
		bl->nextlayer = NULL;
		return(bl);
	}
	bl = db_mrg_free_boxhead_list;
	db_mrg_free_boxhead_list = bl->nextlayer;
	bl->nextlayer = NULL;
	return(bl);
}

void db_mrg_dump_free_boxhead_list(void)
{
	BOXLISTHEAD *bl;

	while (db_mrg_free_boxhead_list != NULL) {
		bl = db_mrg_free_boxhead_list;
		db_mrg_free_boxhead_list = bl->nextlayer;
		bl->nextlayer = NULL;
		bl->box = NULL;
		efree((CHAR *)bl);
	}
}

void db_mrg_remove_boxhead_list(void)
{
	BOXLISTHEAD *bl,*endptr;

	if (db_mrg_curr_cell != NULL) {
		endptr = NULL;
		for(bl = db_mrg_curr_cell; bl != NULL;) {
			bl->box = NULL;
			endptr = bl;
			bl = bl->nextlayer;
		}
		endptr->nextlayer = db_mrg_free_boxhead_list;
		db_mrg_free_boxhead_list = db_mrg_curr_cell;
		db_mrg_curr_cell = NULL;
	}
}

/***************************************************************************
 * Initially, call:
 *    void *merge = mergenew(cluster);
 * where "cluster" is the memory arena for this merging operation.
 * The returned value is used in subsequent calls.
 *
 * For every polygon, call:
 *    mergeaddpolygon(merge, layer, tech, poly);
 * where "layer" is an integer layer number from technology "tech"; "poly"
 * is a polygon to be added; "merge" is the returned value from "mergenew()".
 *
 * You can also subtract a polygon by calling:
 *    mergesubpolygon(merge, layer, tech, poly);
 *
 * To combine two different merges, use:
 *    mergeaddmerge(merge, addmerge, trans)
 * to add the merge information in "addmerge" (transformed by "trans")
 * to the merge in "merge"
 *
 * At end of merging, call:
 *    mergeextract(merge, writepolygon)
 * where "writepolygon" is a routine that will be called for every polygon with
 * five parameters:
 *   (1) the layer number (INTBIG)
 *   (2) the technology
 *   (3) the X coordinate array (INTBIG array)
 *   (4) the Y coordinate array (INTBIG array)
 *   (5) the number of coordinates in each array (INTBIG)
 *
 * And then call:
 *    mergedelete(merge)
 * to deallocate the memory associated with "merge".
 *
 * You can also call
 *    mergedescribe(merge) to return a string describing it
 *
 ***************************************************************************
 */

/***** MERGE POLYGON *****/

#define NOMPOLY ((MPOLY *)-1)

typedef struct Impoly
{
	INTBIG         layer;				/* layer of this polygon */
	TECHNOLOGY    *tech;				/* technology of this polygon */
	INTBIG         count;				/* number of points in this polygon */
	INTBIG         limit;				/* space available in this polygon */
	INTBIG        *x;					/* X coordinates of this polygon */
	INTBIG        *y;					/* Y coordinates of this polygon */
	INTBIG        *seen;				/* whether the point has been visited */
	INTBIG        *intersect;			/* intersection information for the points */
	INTBIG         lx, hx, ly, hy;		/* bounding box of this polygon */
	struct Impoly *nextmpoly;			/* next polygon in linked list */
} MPOLY;

/***** MERGE OBJECT *****/

#define NOMERGE ((MERGE *)-1)

typedef struct Imerge
{
	CLUSTER *clus;					/* memory cluster associated with this merge */
	MPOLY   *mpolylist;				/* the list of active MPOLYs */
	MPOLY   *newmp;					/* temporary MPOLY for merging */
	BOOLEAN  loopwarned;			/* true if warned about loop errors */

	/* for figuring out what to do at an intersection */
	INTBIG  *intpol;				/* polygon with intersection */
	INTBIG  *intpt;					/* point with intersection */
	INTBIG  *intleaveangle;			/* angle of intersection */
	INTBIG   intcount;				/* number of intersection */
	INTBIG   inttotal;				/* size of intersection arrays */

	/* for walking around polygon edges */
	INTBIG   seen;					/* timestamp */

	/* working memory for db_mergeaddintersectionpoints() */
	LISTINTBIG *yorder;
	LISTINTBIG *Oyorder;
	LISTINTBIG *ylist;
	LISTINTBIG *queue;
	LISTINTBIG *Oqueue;

	/* next in linked list */
	struct Imerge *nextmerge;
} MERGE;

static MPOLY  *db_mergesortpoly;			/* use of this is not parallelized yet!!! */
static MPOLY  *db_mpolyfree = NOMPOLY;		/* the list of unused MPOLYs */
static MERGE  *db_mergefree = NOMERGE;		/* the list of unused MERGEs */
static void   *db_polymergemutex = 0;		/* mutex for polygon merging */
static INTBIG  db_mergeshowrightmost = 0;

static BOOLEAN db_mergeaddholetopoly(MERGE *merge, MPOLY *holemplist, MPOLY *newmp);
static void    db_mergeaddintersectionpoint(MERGE *merge, INTBIG polygon, INTBIG point);
static BOOLEAN db_mergeaddintersectionpoints(MERGE *merge, MPOLY *mp, MPOLY *omp);
static BOOLEAN db_mergeaddthisintersectionpoint(MERGE *merge, MPOLY *mp, INTBIG i,
				INTBIG ix, INTBIG iy, INTBIG intvalue);
static MERGE  *db_mergealloc(CLUSTER *cluster);
static MPOLY  *db_mergeallocpoly(MERGE *merge);
static BOOLEAN db_mergecheckqueue(MERGE *merge, MPOLY *mp, LISTINTBIG *queue, LISTINTBIG *yorder,
				INTBIG startqueuepos, INTBIG fromline, MPOLY *omp, LISTINTBIG *Oqueue,
				LISTINTBIG *Oyorder, INTBIG ofromline, INTBIG intvalue);
static BOOLEAN db_mergecopypoly(MERGE *merge, MPOLY *smp, MPOLY *dmp);
static BOOLEAN db_mergeensurespace(MERGE *merge, MPOLY *mp, INTBIG count);
static MPOLY  *db_mergefindpolyandmerge(MERGE *merge, MPOLY *mp, BOOLEAN keepnew, BOOLEAN subtract);
static INTBIG  db_mergefindstartingpoint(MPOLY *mp, MPOLY *omp);
static void    db_mergefreepoly(MPOLY *mp);
static BOOLEAN db_mergeinsertpoint(MERGE *merge, MPOLY *newmp, INTBIG *insert, INTBIG x, INTBIG y);
static BOOLEAN db_mergeisinside(INTBIG x, INTBIG y, MPOLY *mp);
static INTBIG  db_mergelinesintersect(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty,
				INTBIG ofx, INTBIG ofy, INTBIG otx, INTBIG oty, INTBIG *ix, INTBIG *iy);
static BOOLEAN db_mergepoly(MERGE *merge, MPOLY *mp, MPOLY *omp, BOOLEAN subtract);
static void    db_mergeshowmpoly(INTBIG count, INTBIG *x, INTBIG *y,
				NODEPROTO *prim, INTBIG showXoff, INTBIG showYoff, INTBIG layer, TECHNOLOGY *tech);
static void    db_mergesimplifyandbbox(MPOLY *mp);
static int     db_mergesortiny(const void *e1, const void *e2);
static BOOLEAN db_mergewalkedge(MERGE *merge, MPOLY *mp, INTBIG i, MPOLY *omp, MPOLY *newmp, BOOLEAN subtract);

/* debugging this module:
 * uncomment "SHOWRESULTS" to generate a step-by-step illustration of each new polygon that is merged
 * uncomment "SHOWMERGERESULTS" to generate a step-by-step illustration of each new merge that is merged
 *       set "DESIREDLAYER" to the layer you want to watch
 *       uncomment "SHOWTICKS" to draw tick marks around illustrations
 * uncomment "DEBUGWALK" to generate a step-by-step illustration of each new point traversed while adding a polygon
 */
/* #define SHOWRESULTS     1 */		/* uncomment to show results of each "polygon" merge */
/* #define SHOWMERGERESULTS 1*/		/* uncomment to show results of each "merge" merge */
/* #define SHOWTICKS       1 */		/* uncomment to show coordinate values with results */
/* #define DEBUGWALK       1 */		/* uncomment to debug polygon point traversal */
/* #define SHOWHOLEGROWTH  1 */		/* uncomment to show results of hole growth in a merge */
/* #define DESIREDLAYER    11 */	/* uncomment to select a specific layer (11=n-select) */
#define RESULTSPACING 100		/* distance between result graphics */

/*
 * Routine to free all memory associated with this module.
 */
void db_freemergememory(void)
{
	REGISTER MPOLY *mp;
	REGISTER MERGE *merge;

	/* free memory used by manhattan merger */
	if (db_mrgbuflen != 0)
	{
		efree((CHAR *)db_mrgxbuf);
		efree((CHAR *)db_mrgybuf);
	}

	/* free all allocated merge structures */
	while (db_mergefree != NOMERGE)
	{
		merge = db_mergefree;
		db_mergefree = merge->nextmerge;
		if (merge->newmp != NOMPOLY) db_mergefreepoly(merge->newmp);
		if (merge->inttotal > 0)
		{
			efree((CHAR *)merge->intpol);
			efree((CHAR *)merge->intpt);
			efree((CHAR *)merge->intleaveangle);
		}

		killintlistobj(merge->yorder);
		killintlistobj(merge->Oyorder);
		killintlistobj(merge->queue);
		killintlistobj(merge->Oqueue);
		killintlistobj(merge->ylist);
		efree((CHAR *)merge);
	}

	/* free memory used by polygonal merger */
	while (db_mpolyfree != NOMPOLY)
	{
		mp = db_mpolyfree;
		db_mpolyfree = mp->nextmpoly;
		if (mp->limit > 0)
		{
			efree((CHAR *)mp->x);
			efree((CHAR *)mp->y);
			efree((CHAR *)mp->seen);
			efree((CHAR *)mp->intersect);
		}
		efree((CHAR *)mp);
	}
}

/*
 * Routine to create a new "merge" object.  After this call, multiple calls to
 * "mergeaddpolygon()" may be made, followed by a single call to "mergeextract()"
 * to obtain the merged geometry, and a call to "mergedelete()" to free this object.
 * Returns zero on error.
 */
void *mergenew(CLUSTER *cluster)
{
	MERGE *merge;

	/* make sure mutual-exclusion object is created */
	if (ensurevalidmutex(&db_polymergemutex, TRUE)) return(0);

	merge = db_mergealloc(cluster);
	if (merge == NOMERGE) return(0);
	merge->mpolylist = NOMPOLY;
	merge->seen = 0;
	merge->loopwarned = FALSE;
	return((void *)merge);
}

/*
 * Routine to free memory associated with merge object "vmerge".
 */
void mergedelete(void *vmerge)
{
	REGISTER MERGE *merge;
	REGISTER MPOLY *mp;

	merge = (MERGE *)vmerge;
	while (merge->mpolylist != NOMPOLY)
	{
		mp = merge->mpolylist;
		merge->mpolylist = mp->nextmpoly;
		db_mergefreepoly(mp);
	}

	if (db_multiprocessing) emutexlock(db_polymergemutex);
	merge->nextmerge = db_mergefree;
	db_mergefree = merge;
	if (db_multiprocessing) emutexunlock(db_polymergemutex);
}

/*
 * Routine to add polygon "poly" to the merged collection.  The polygon is on layer
 * "layer" of technology "tech".
 */
void mergeaddpolygon(void *vmerge, INTBIG layer, TECHNOLOGY *tech, POLYGON *poly)
{
	REGISTER MERGE *merge;
	REGISTER MPOLY *mp, *omp, *ump, *lastmp;
	REGISTER INTBIG i;
	REGISTER BOOLEAN done;
	INTBIG lx, hx, ly, hy;

	/* copy the polygon into a local structure */
	if (poly->count == 0) return;
	merge = (MERGE *)vmerge;
	mp = db_mergeallocpoly(merge);
	if (db_mergeensurespace(merge, mp, poly->count)) return;
	if (poly->style == FILLEDRECT || poly->style == CLOSEDRECT)
	{
		getbbox(poly, &lx, &hx, &ly, &hy);
		makerectpoly(lx, hx, ly, hy, poly);
	}
	if (areapoints(poly->count, poly->xv, poly->yv) < 0)
	{
		/* reverse direction of points while copying */
		for(i=0; i<poly->count; i++)
		{
			mp->x[i] = poly->xv[poly->count-i-1];
			mp->y[i] = poly->yv[poly->count-i-1];
		}
	} else
	{
		for(i=0; i<poly->count; i++)
		{
			mp->x[i] = poly->xv[i];
			mp->y[i] = poly->yv[i];
		}
	}
	mp->count = poly->count;
	mp->layer = layer;
	mp->tech = tech;
	db_mergesimplifyandbbox(mp);

	/* now look for merges */
	omp = db_mergefindpolyandmerge(merge, mp, TRUE, FALSE);
	if (omp == NOMPOLY)
	{
		/* not merged: just add to the list */
		mp->nextmpoly = merge->mpolylist;
		merge->mpolylist = mp;
	} else
	{
		/* merged: delete this */
		db_mergefreepoly(mp);

		/* see if any other merging can be done */
		done = FALSE;
		while (!done)
		{
			done = TRUE;
			mp = db_mergefindpolyandmerge(merge, omp, FALSE, FALSE);
			if (mp != NOMPOLY)
			{
				lastmp = NOMPOLY;
				for(ump = merge->mpolylist; ump != NOMPOLY; ump = ump->nextmpoly)
				{
					if (ump == mp) break;
					lastmp = ump;
				}
				if (lastmp == NOMPOLY) merge->mpolylist = mp->nextmpoly; else
					lastmp->nextmpoly = mp->nextmpoly;
				db_mergefreepoly(mp);
				done = FALSE;
			}
		}
	}

#ifdef SHOWRESULTS

#  ifdef DESIREDLAYER
	if (layer == DESIREDLAYER)
#  endif
	{
		REGISTER NODEPROTO *cell;
		static INTBIG db_mergeshowxoff = 0;

		db_mergeshowxoff += el_curlib->lambda[el_curtech->techindex] * RESULTSPACING;
		cell = getcurcell();
		if (cell == NONODEPROTO) return;
		db_mergeshowmpoly(poly->count, poly->xv, poly->yv, getnodeproto(x_("metal-2-node")),
			db_mergeshowxoff, 0, layer, tech);

		/* show the merge it gets added to */
		mergeshow((void *)merge, getnodeproto(x_("metal-1-node")), db_mergeshowxoff, 0, x_("After adding polygon"));
	}
#endif
}

/*
 * Routine to subtract polygon "poly" from the merged collection.  The polygon is on layer
 * "layer" of technology "tech".
 */
void mergesubpolygon(void *vmerge, INTBIG layer, TECHNOLOGY *tech, POLYGON *poly)
{
	REGISTER MERGE *merge;
	REGISTER MPOLY *mp, *omp;
	REGISTER INTBIG i;
	INTBIG lx, hx, ly, hy;

	/* copy the polygon into a local structure */
	if (poly->count == 0) return;
	merge = (MERGE *)vmerge;
	mp = db_mergeallocpoly(merge);
	if (db_mergeensurespace(merge, mp, poly->count)) return;
	if (poly->style == FILLEDRECT || poly->style == CLOSEDRECT)
	{
		getbbox(poly, &lx, &hx, &ly, &hy);
		makerectpoly(lx, hx, ly, hy, poly);
	}

	/* make the points go in the opposite direction for subtraction */
	if (areapoints(poly->count, poly->xv, poly->yv) > 0)
	{
		/* reverse direction of points while copying */
		for(i=0; i<poly->count; i++)
		{
			mp->x[i] = poly->xv[poly->count-i-1];
			mp->y[i] = poly->yv[poly->count-i-1];
		}
	} else
	{
		for(i=0; i<poly->count; i++)
		{
			mp->x[i] = poly->xv[i];
			mp->y[i] = poly->yv[i];
		}
	}
	mp->count = poly->count;
	mp->layer = layer;
	mp->tech = tech;
	db_mergesimplifyandbbox(mp);

	/* merging will subtract since "mp" is reversed */
	omp = db_mergefindpolyandmerge(merge, mp, TRUE, TRUE);

	/* now delete this */
	db_mergefreepoly(mp);
}

/*
 * Routine to add merge object "addmerge" to the merged collection "merge", transforming
 * it by "trans".
 */
void mergeaddmerge(void *vmerge, void *vaddmerge, XARRAY trans)
{
	REGISTER MERGE *merge, *addmerge;
	REGISTER MPOLY *amp, *mp, *omp, *ump, *lastmp;
	REGISTER BOOLEAN done;
	REGISTER INTBIG i, save, revi;
#ifdef SHOWMERGERESULTS
	static INTBIG db_mergeshowxoff = 0;
#endif

	merge = (MERGE *)vmerge;
	addmerge = (MERGE *)vaddmerge;
	for(amp = addmerge->mpolylist; amp != NOMPOLY; amp = amp->nextmpoly)
	{
		mp = db_mergeallocpoly(merge);
		if (mp == NOMPOLY) return;
		mp->layer = amp->layer;
		mp->tech = amp->tech;
		db_mergecopypoly(merge, amp, mp);

		/* transform it */
		for(i=0; i<mp->count; i++)
			xform(mp->x[i], mp->y[i], &mp->x[i], &mp->y[i], trans);

		/* reverse direction of points if transform made it necessary */
		if (areapoints(mp->count, mp->x, mp->y) < 0)
		{
			for(i=0; i<mp->count/2; i++)
			{
				revi = mp->count-i-1;
				save = mp->x[i];   mp->x[i] = mp->x[revi];   mp->x[revi] = save;
				save = mp->y[i];   mp->y[i] = mp->y[revi];   mp->y[revi] = save;
			}
		}
		db_mergesimplifyandbbox(mp);
#ifdef SHOWMERGERESULTS
		db_mergeshowmpoly(mp->count, mp->x, mp->y, getnodeproto(x_("metal-2-node")),
			db_mergeshowxoff, 0, mp->layer, mp->tech);
#endif
		/* now look for merges */
		omp = db_mergefindpolyandmerge(merge, mp, TRUE, FALSE);
		if (omp == NOMPOLY)
		{
			/* not merged: just add to the list */
			mp->nextmpoly = merge->mpolylist;
			merge->mpolylist = mp;
		} else
		{
			/* merged: delete this */
			db_mergefreepoly(mp);

			/* see if any other merging can be done */
			done = FALSE;
			while (done == 0)
			{
				done = TRUE;
				mp = db_mergefindpolyandmerge(merge, omp, FALSE, FALSE);
				if (mp != NOMPOLY)
				{
					lastmp = NOMPOLY;
					for(ump = merge->mpolylist; ump != NOMPOLY; ump = ump->nextmpoly)
					{
						if (ump == mp) break;
						lastmp = ump;
					}
					if (lastmp == NOMPOLY) merge->mpolylist = mp->nextmpoly; else
						lastmp->nextmpoly = mp->nextmpoly;
					db_mergefreepoly(mp);
					done = FALSE;
				}
			}
		}
#ifdef SHOWMERGERESULTS
		mergeshow(merge, getnodeproto(x_("metal-1-node")), db_mergeshowxoff, 0, x_("after adding"));
		db_mergeshowxoff += el_curlib->lambda[el_curtech->techindex] * RESULTSPACING;
#endif
	}
}

/*
 * Routine to report all of the merged polygons in "vmerge" through
 * the callback routine "writepolygon()".  The routine is given 5 parameters for
 * each merged polygon:
 *   (1) the layer
 *   (2) the technology
 *   (3) the X coordinates
 *   (4) the Y coordinates
 *   (5) the number of coordinates
 */
void mergeextract(void *vmerge, void (*writepolygon)(INTBIG, TECHNOLOGY*, INTBIG*, INTBIG*, INTBIG))
{
	REGISTER MERGE *merge;
	REGISTER MPOLY *mp;

	merge = (MERGE *)vmerge;
	for(mp = merge->mpolylist; mp != NOMPOLY; mp = mp->nextmpoly)
	{
		db_mergesimplifyandbbox(mp);
		(*writepolygon)(mp->layer, mp->tech, mp->x, mp->y, mp->count);
	}
}

/*
 * Routine to return the bounding box of the merged polygon "vmerge" in (lx,hx,ly,hy).
 * Returns FALSE if there is no valid polygon.
 */
BOOLEAN mergebbox(void *vmerge, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER MERGE *merge;
	REGISTER MPOLY *mp;
	REGISTER BOOLEAN valid;
	REGISTER INTBIG i;

	merge = (MERGE *)vmerge;
	valid = FALSE;
	for(mp = merge->mpolylist; mp != NOMPOLY; mp = mp->nextmpoly)
	{
		db_mergesimplifyandbbox(mp);
		if (mp->count < 3) continue;
		for(i=0; i<mp->count; i++)
		{
			if (valid)
			{
				if (mp->x[i] < *lx) *lx = mp->x[i];
				if (mp->x[i] > *hx) *hx = mp->x[i];
				if (mp->y[i] < *ly) *ly = mp->y[i];
				if (mp->y[i] > *hy) *hy = mp->y[i];
			} else
			{
				*lx = *hx = mp->x[i];
				*ly = *hy = mp->y[i];
				valid = TRUE;
			}
		}
	}
	return(valid);
}

/*
 * Routine used only in debugging this module.
 * It returns a string describing the merge object "vmerge".
 */
CHAR *mergedescribe(void *vmerge)
{
	REGISTER MERGE *merge;
	REGISTER MPOLY *mp;
	void *infstr;

	merge = (MERGE *)vmerge;
	infstr = initinfstr();
	for (mp = merge->mpolylist; mp != NOMPOLY; mp = mp->nextmpoly)
	{
		if (mp != merge->mpolylist) addstringtoinfstr(infstr, x_(", "));
		formatinfstr(infstr, x_("%ld PTS on LAYER %ld"), mp->count, mp->layer);
	}
	return(returninfstr(infstr));
}

/*
 * Routine used only in debugging this module.
 * It displays "vmerge" using polygons of "prim", offset at (showXoff,showYoff).
 */
void mergeshow(void *vmerge, NODEPROTO *prim, INTBIG showXoff, INTBIG showYoff, CHAR *title)
{
	REGISTER NODEPROTO *cell;
	REGISTER MPOLY *mp;
	REGISTER MERGE *merge;
	REGISTER NODEINST *ni;
	CHAR buf[200];
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, lx, hx, ly, hy, cx, cy, *newlist;

	merge = (MERGE *)vmerge;
	cell = getcurcell();
	if (cell == NONODEPROTO) return;
	for(mp = merge->mpolylist; mp != NOMPOLY; mp = mp->nextmpoly)
	{
#ifdef DESIREDLAYER
		if (mp->layer != DESIREDLAYER) continue;
#endif
		lx = hx = mp->x[0];
		ly = hy = mp->y[0];
		for(i=1; i<mp->count; i++)
		{
			if (mp->x[i] < lx) lx = mp->x[i];
			if (mp->x[i] > hx) hx = mp->x[i];
			if (mp->y[i] < ly) ly = mp->y[i];
			if (mp->y[i] > hy) hy = mp->y[i];
		}
		cx = (lx+hx) / 2;   cy = (ly+hy) / 2;
		ni = newnodeinst(prim, lx+showXoff, hx+showXoff, ly+showYoff, hy+showYoff, 0, 0, cell);
		newlist = (INTBIG *)emalloc(mp->count * 2 * SIZEOFINTBIG, el_tempcluster);
		if (newlist == 0) return;
		for(i=0; i<mp->count; i++)
		{
			newlist[i*2] = mp->x[i] - cx;
			newlist[i*2+1] = mp->y[i] - cy;
		}
		(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
			VINTEGER|VISARRAY|((mp->count*2)<<VLENGTHSH));
		esnprintf(buf, 200, x_("%ld/%s"), mp->layer, mp->tech->techname);
		setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)buf, VSTRING|VDISPLAY);
		endobjectchange((INTBIG)ni, VNODEINST);
	}

	/* get bounds of the merge */
	for(mp = merge->mpolylist; mp != NOMPOLY; mp = mp->nextmpoly)
	{
		if (mp == merge->mpolylist)
		{
			lx = hx = mp->x[0];
			ly = hy = mp->y[0];
		}
		for(i=0; i<mp->count; i++)
		{
			if (mp->x[i] < lx) lx = mp->x[i];
			if (mp->x[i] > hx) hx = mp->x[i];
			if (mp->y[i] < ly) ly = mp->y[i];
			if (mp->y[i] > hy) hy = mp->y[i];
		}
	}
	cx = (lx + hx) / 2;
	ni = newnodeinst(gen_invispinprim, cx+showXoff, cx+showXoff, hy+showYoff, hy+showYoff, 0, 0, cell);
	var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)title, VSTRING|VDISPLAY);
	if (var != NOVARIABLE)
	{
		TDSETPOS(var->textdescript, VTPOSUP);
		TDSETSIZE(var->textdescript, TXTSETQLAMBDA(64));
	}

#ifdef SHOWTICKS
	for(mp = merge->mpolylist; mp != NOMPOLY; mp = mp->nextmpoly)
	{
		REGISTER INTBIG j, lambda;
		extern NODEPROTO *gen_invispinprim;

		lambda = el_curlib->lambda[el_curtech->techindex];
		for(i=0; i<mp->count; i++)
		{
			/* see if the X value has already been done */
			for(j=0; j<i; j++) if (mp->x[j] == mp->x[i]) break;
			if (j < i) continue;
			for(omp = merge->mpolylist; omp != mp; omp = omp->nextmpoly)
			{
				for(j=0; j<omp->count; j++)
					if (omp->x[j] == mp->x[i]) break;
				if (j < omp->count) break;
			}
			if (omp != mp) continue;

			/* show the X coordinate */
			ni = newnodeinst(gen_invispinprim, mp->x[i]+showXoff, mp->x[i]+showXoff,
				ly-lambda*2+showYoff, ly-lambda*2+showYoff,
					0, 0, cell);
			if (ni != NONODEINST)
			{
				REGISTER VARIABLE *var;
				var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)latoa(mp->x[i], 0),
					VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
				{
					TDSETPOS(var->textdescript, VTPOSDOWN);
					TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				}
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}

		for(i=0; i<mp->count; i++)
		{
			/* see if the Y value has already been done */
			for(j=0; j<i; j++) if (mp->y[j] == mp->y[i]) break;
			if (j < i) continue;
			for(omp = merge->mpolylist; omp != mp; omp = omp->nextmpoly)
			{
				for(j=0; j<omp->count; j++)
					if (omp->y[j] == mp->y[i]) break;
				if (j < omp->count) break;
			}
			if (omp != mp) continue;

			/* show the Y coordinate */
			ni = newnodeinst(gen_invispinprim, lx-lambda*2+showXoff,
				lx-lambda*2+showXoff, mp->y[i]+showYoff, mp->y[i]+showYoff, 0, 0, cell);
			if (ni != NONODEINST)
			{
				REGISTER VARIABLE *var;
				var = setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)latoa(mp->y[i], 0),
					VSTRING|VDISPLAY);
				if (var != NOVARIABLE)
				{
					TDSETPOS(var->textdescript, VTPOSLEFT);
					TDSETSIZE(var->textdescript, TXTSETQLAMBDA(4));
				}
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
	}
#endif
}

/***** POLYGON MERGING SUPPORT *****/

/*
 * Routine used only in debugging this module.
 * It displays "count" points in (x,y) using polygons of "prim", offset at (showXoff,showYoff).
 */
void db_mergeshowmpoly(INTBIG count, INTBIG *x, INTBIG *y,
	NODEPROTO *prim, INTBIG showXoff, INTBIG showYoff, INTBIG layer, TECHNOLOGY *tech)
{
	REGISTER INTBIG lx, hx, ly, hy, i, cx, cy;
	REGISTER NODEINST *ni;
	REGISTER INTBIG *newlist;
	REGISTER NODEPROTO *cell;
	CHAR buf[200];

	cell = getcurcell();
	if (cell == NONODEPROTO) return;
	lx = hx = x[0];
	ly = hy = y[0];
	for(i=1; i<count; i++)
	{
		if (x[i] < lx) lx = x[i];
		if (x[i] > hx) hx = x[i];
		if (y[i] < ly) ly = y[i];
		if (y[i] > hy) hy = y[i];
	}
	if (hx+showXoff > db_mergeshowrightmost) db_mergeshowrightmost = hx+showXoff;
	cx = (lx+hx) / 2;   cy = (ly+hy) / 2;
	ni = newnodeinst(prim, lx+showXoff, hx+showXoff, ly+showYoff, hy+showYoff, 0, 0, cell);
	newlist = (INTBIG *)emalloc(count * 2 * SIZEOFINTBIG, el_tempcluster);
	if (newlist == 0) return;
	for(i=0; i<count; i++)
	{
		newlist[i*2] = x[i] - cx;
		newlist[i*2+1] = y[i] - cy;
	}
	(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
		VINTEGER|VISARRAY|((count*2)<<VLENGTHSH));
	esnprintf(buf, 200, x_("%ld/%s"), layer, tech->techname);
	setvalkey((INTBIG)ni, VNODEINST, el_node_name_key, (INTBIG)buf, VSTRING|VDISPLAY);
	endobjectchange((INTBIG)ni, VNODEINST);
}

/*
 * Routine to merge "mp" into "merge".  If "subtract" is true, this is a removal.
 */
MPOLY *db_mergefindpolyandmerge(MERGE *merge, MPOLY *mp, BOOLEAN keepnew, BOOLEAN subtract)
{
	REGISTER MPOLY *omp;

	for(omp = merge->mpolylist; omp != NOMPOLY; omp = omp->nextmpoly)
	{
		if (omp == mp) continue;

		/* reject immediately if they are on different layers */
		if (omp->tech != mp->tech || omp->layer != mp->layer) continue;

		/* reject immediately if the bounding boxes don't intersect */
		if (omp->lx > mp->hx || omp->hx < mp->lx) continue;
		if (omp->ly > mp->hy || omp->hy < mp->ly) continue;

		/* try to merge the polygons */
		if (keepnew)
		{
			if (db_mergepoly(merge, mp, omp, subtract)) break;
		} else
		{
			if (db_mergepoly(merge, omp, mp, subtract)) break;
		}
	}
	return(omp);
}

/*
 * Helper routine to combine two polygons "mp" (the first polygon) and "omp" (the
 * second polygon).  If they merge then the routine returns true and places the
 * merged polygon in "omp".  If "subtract" is true, just subtract "mp" from "omp".
 */
BOOLEAN db_mergepoly(MERGE *merge, MPOLY *mp, MPOLY *omp, BOOLEAN subtract)
{
	REGISTER INTBIG i, oi, j, startpol, startpt, intpoints;
	MPOLY *mps[2], *holepolylist, *holepoly;
#ifdef DEBUGWALK
	void *infstr;
#endif

	/* reset intersection point information */
	for(i=0; i<mp->count; i++) mp->intersect[i] = 0;
	for(oi=0; oi<omp->count; oi++) omp->intersect[oi] = 0;

	/* create additional points on each polygon at the intersections */
	if (db_mergeaddintersectionpoints(merge, mp, omp)) return(FALSE);
	if (db_mergeaddintersectionpoints(merge, mp, mp)) return(FALSE);
	if (db_mergeaddintersectionpoints(merge, omp, omp)) return(FALSE);
#ifdef DEBUGWALK
	infstr = initinfstr();
	if (subtract) addstringtoinfstr(infstr, x_("Subtracting poly with intersection points added: "));
		else addstringtoinfstr(infstr, x_("First poly with intersection points added: "));
	for(i=0; i<mp->count; i++)
		formatinfstr(infstr, x_(" (%s,%s)"), latoa(mp->x[i], 0), latoa(mp->y[i], 0));
	ttyputmsg(x_("%s"), returninfstr(infstr));

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Second poly with intersection points added: "));
	for(i=0; i<omp->count; i++)
		formatinfstr(infstr, x_(" (%s,%s)"), latoa(omp->x[i], 0), latoa(omp->y[i], 0));
	ttyputmsg(x_("%s"), returninfstr(infstr));
#endif

	/* if there are no intersection points, see if one polygon is inside the other */
	intpoints = 0;
	for(i=0; i<mp->count; i++) if (mp->intersect[i] == 2) { intpoints++;   break; }
	for(oi=0; oi<omp->count; oi++) if (omp->intersect[oi] == 2) { intpoints++;   break; }
	if (intpoints == 0)
	{
		/* figure out which polygon is smaller */
		if (fabs(areapoints(mp->count, mp->x, mp->y)) <
			areapoints(omp->count, omp->x, omp->y))
		{
			/* if polygons don't intersect, stop */
			if (!db_mergeisinside(mp->x[0], mp->y[0], omp)) return(FALSE);

			/* first polygon is smaller and covered by second */
			return(TRUE);
		} else
		{
			/* if polygons don't intersect, stop */
			if (!db_mergeisinside(omp->x[0], omp->y[0], mp)) return(FALSE);

			if (subtract)
			{
				/* subtract polygon covers merge: delete the merge */
				omp->count = 0;
			} else
			{
				/* add polygon is larger, copy and use it */
				if (db_mergecopypoly(merge, mp, omp)) return(FALSE);
				db_mergesimplifyandbbox(omp);
			}
			return(TRUE);
		}
	}

	/* find a starting point (one with no intersections and not inside the other) */
	if (subtract)
	{
		/* subtracting: only want starting points on "omp" */
		oi = db_mergefindstartingpoint(omp, mp);
		if (oi < 0)
		{
			/* all points are intersections: see if subtract is larger */
			if (fabs(areapoints(mp->count, mp->x, mp->y)) >
				areapoints(omp->count, omp->x, omp->y))
			{
				/* subtract polygon covers merge: delete the merge */
				omp->count = 0;
			}
			db_mergesimplifyandbbox(omp);
			return(TRUE);
		}
		startpol = 1;
		startpt = oi;
	} else
	{
		/* adding: find starting point on either polygon */
		i = db_mergefindstartingpoint(mp, omp);
		if (i >= 0)
		{
			startpol = 0;
			startpt = i;
		} else
		{
			oi = db_mergefindstartingpoint(omp, mp);
			if (oi < 0)
			{
				/* all points are intersections: return larger polygon */
				if (areapoints(mp->count, mp->x, mp->y) >
					areapoints(omp->count, omp->x, omp->y))
				{
					/* add polygon larger than merge: copy and use it */
					if (db_mergecopypoly(merge, mp, omp)) return(FALSE);
				}
				db_mergesimplifyandbbox(omp);
				return(TRUE);
			}
			startpol = 1;
			startpt = oi;
		}
	}

	/* walk around the polygons */
	for(i=0; i<mp->count; i++) mp->seen[i] = 0;
	for(oi=0; oi<omp->count; oi++) omp->seen[oi] = 0;
	mps[0] = mp;   mps[1] = omp;
	if (merge->newmp == NOMPOLY) merge->newmp = db_mergeallocpoly(merge);
	merge->newmp->count = 0;
	merge->newmp->layer = mp->layer;
	merge->newmp->tech = mp->tech;
	if (db_mergewalkedge(merge, mps[startpol], startpt, mps[1-startpol], merge->newmp, subtract))
		return(FALSE);

	/* handle hole processing */
	holepolylist = NOMPOLY;
	if (subtract)
	{
		/* when subtracting, make sure all points on the original polygon are processed */
		for(oi=0; oi<omp->count; oi++)
		{
			if (omp->seen[oi] != 0) continue;
			if (omp->intersect[oi] != 0) continue;
			if (db_mergeisinside(omp->x[oi], omp->y[oi], mp)) continue;
			holepoly = db_mergeallocpoly(merge);
			holepoly->layer = mp->layer;
			holepoly->tech = mp->tech;
			if (db_mergewalkedge(merge, omp, oi, mp, holepoly, TRUE)) return(FALSE);
			holepoly->nextmpoly = holepolylist;
			holepolylist = holepoly;
		}

		/* see if there are any points that didn't get seen and are not in the other polygon */
		for(oi=0; oi<omp->count; oi++)
		{
			if (omp->seen[oi] != 0) continue;
			for(j=0; j<omp->count; j++)
				if (omp->seen[j] != 0 && omp->x[j] == omp->x[oi] && omp->y[j] == omp->y[oi]) break;
			if (j < omp->count) continue;
			if (db_mergeisinside(omp->x[oi], omp->y[oi], mp)) continue;
			holepoly = db_mergeallocpoly(merge);
			holepoly->layer = mp->layer;
			holepoly->tech = mp->tech;
			if (db_mergewalkedge(merge, omp, oi, mp, holepoly, TRUE)) return(FALSE);
			holepoly->nextmpoly = holepolylist;
			holepolylist = holepoly;
		}

		/* carve holes from polygon */
		if (db_mergeaddholetopoly(merge, holepolylist, merge->newmp)) return(FALSE);
	} else
	{
		/* handle hole processing if adding */

		/* see if there are any nonintersection points that didn't get seen and are not in the other polygon */
		for(i=0; i<mp->count; i++)
		{
			if (mp->seen[i] != 0) continue;
			if (mp->intersect[i] != 0) continue;
			if (db_mergeisinside(mp->x[i], mp->y[i], omp)) continue;
			holepoly = db_mergeallocpoly(merge);
			holepoly->layer = mp->layer;
			holepoly->tech = mp->tech;
			if (db_mergewalkedge(merge, mp, i, omp, holepoly, FALSE)) return(FALSE);
			holepoly->nextmpoly = holepolylist;
			holepolylist = holepoly;
		}
		for(oi=0; oi<omp->count; oi++)
		{
			if (omp->seen[oi] != 0) continue;
			if (omp->intersect[oi] != 0) continue;
			if (db_mergeisinside(omp->x[oi], omp->y[oi], mp)) continue;
			holepoly = db_mergeallocpoly(merge);
			holepoly->layer = mp->layer;
			holepoly->tech = mp->tech;
			if (db_mergewalkedge(merge, omp, oi, mp, holepoly, FALSE)) return(FALSE);
			holepoly->nextmpoly = holepolylist;
			holepolylist = holepoly;
		}

		/* see if there are any points that didn't get seen and are not in the other polygon */
		for(i=0; i<mp->count; i++)
		{
			if (mp->seen[i] != 0) continue;
			for(j=0; j<mp->count; j++)
				if (mp->seen[j] != 0 && mp->x[j] == mp->x[i] && mp->y[j] == mp->y[i]) break;
			if (j < mp->count) continue;
			if (db_mergeisinside(mp->x[i], mp->y[i], omp)) continue;
			holepoly = db_mergeallocpoly(merge);
			holepoly->layer = mp->layer;
			holepoly->tech = mp->tech;
			if (db_mergewalkedge(merge, mp, i, omp, holepoly, FALSE)) return(FALSE);
			holepoly->nextmpoly = holepolylist;
			holepolylist = holepoly;
		}
		for(oi=0; oi<omp->count; oi++)
		{
			if (omp->seen[oi] != 0) continue;
			for(j=0; j<omp->count; j++)
				if (omp->seen[j] != 0 && omp->x[j] == omp->x[oi] && omp->y[j] == omp->y[oi]) break;
			if (j < omp->count) continue;
			if (db_mergeisinside(omp->x[oi], omp->y[oi], mp)) continue;
			holepoly = db_mergeallocpoly(merge);
			holepoly->layer = mp->layer;
			holepoly->tech = mp->tech;
			if (db_mergewalkedge(merge, omp, oi, mp, holepoly, FALSE)) return(FALSE);
			holepoly->nextmpoly = holepolylist;
			holepolylist = holepoly;
		}

		/* carve holes from polygon */
		if (db_mergeaddholetopoly(merge, holepolylist, merge->newmp)) return(FALSE);
	}

	/* copy result back to second polygon */
	if (db_mergecopypoly(merge, merge->newmp, omp)) return(FALSE);
	db_mergesimplifyandbbox(omp);
	return(TRUE);
}

/*
 * Routine to find a good starting point on polygon "mp" (the other one is "omp").
 * Looks for points with no intersection marks, preferably on the outside edge,
 * and not inside of the other polygon
 */
INTBIG db_mergefindstartingpoint(MPOLY *mp, MPOLY *omp)
{
	REGISTER INTBIG i, lx, hx, ly, hy;

	lx = hx = mp->x[0];
	ly = hy = mp->y[0];
	for(i=1; i<mp->count; i++)
	{
		if (mp->x[i] < lx) lx = mp->x[i];
		if (mp->x[i] > hx) hx = mp->x[i];
		if (mp->y[i] < ly) ly = mp->y[i];
		if (mp->y[i] > hy) hy = mp->y[i];
	}

	/* look for points with no intersection, on edge, and not inside other */
	for(i=0; i<mp->count; i++)
	{
		if (mp->intersect[i] != 0) continue;
		if (mp->x[i] != lx && mp->x[i] != hx &&
			mp->y[i] != ly && mp->y[i] != hy) continue;
		if (db_mergeisinside(mp->x[i], mp->y[i], omp)) continue;
		return(i);
	}

	/* accept points that aren't on the edge */
	for(i=0; i<mp->count; i++)
	{
		if (mp->intersect[i] != 0) continue;
		if (db_mergeisinside(mp->x[i], mp->y[i], omp)) continue;
		return(i);
	}

	/* nothing found */
	return(-1);
}

/*
 * Helper routine to add hole polygon "holemp" to the main polygon "newmp".
 * Finds the closest points on the two and adds the points.
 * Returns true on error.
 */
BOOLEAN db_mergeaddholetopoly(MERGE *merge, MPOLY *holemplist, MPOLY *newmp)
{
	REGISTER INTBIG j, k, l, dist, bestdist, hint, nint, nx, ny, hfx, hfy, htx, hty,
		nfx, nfy, ntx, nty, lastj, lastk, solved;
	INTBIG bestpos;
	MPOLY *holemp, *besthole, *lasthole, *bestlasthole;

	/* keep adding holes until there are no more */
	while (holemplist != NOMPOLY)
	{
		solved = 0;
		lasthole = NOMPOLY;
		besthole = NOMPOLY;

		/* mark points on the main polygon that are duplicated (can't attach holes there) */
		for(k=0; k<newmp->count; k++) newmp->seen[k] = 0;
		for(k=0; k<newmp->count; k++)
		{
			for(j=k+1; j<newmp->count; j++)
			{
				if (newmp->x[k] == newmp->x[j] && newmp->y[k] == newmp->y[j])
					newmp->seen[k] = newmp->seen[j] = 1;
			}
		}

		/* examine every hole to find the next one to add */
		for(holemp = holemplist; holemp != NOMPOLY; holemp = holemp->nextmpoly)
		{
			/* find point on hole that is closest to a polygon point */
			for(j=0; j<holemp->count; j++)
			{
				/* get coordinates of the hole segment */
				if (j == 0) lastj = holemp->count - 1; else
					lastj = j - 1;
				hfx = holemp->x[lastj];   hfy = holemp->y[lastj];
				htx = holemp->x[j];       hty = holemp->y[j];
				for(k=0; k<newmp->count; k++)
				{
					/* get coordinates of the main polygon segment */
					if (k == 0) lastk = newmp->count - 1; else
						lastk = k - 1;
					nfx = newmp->x[lastk];   nfy = newmp->y[lastk];
					ntx = newmp->x[k];       nty = newmp->y[k];

					/* see if they are colinear */
					if (hfx == htx && hfx == nfx && nfx == ntx)
					{
						/* vertically aligned */
						if (mini(hfy, hty) < maxi(nfy, nty) && maxi(hfy, hty) > mini(nfy, nty))
						{
							bestpos = k;
							l = lastj;
							for(;;)
							{
								l++;
								if (l >= holemp->count) l = 0;
								if (db_mergeinsertpoint(merge, newmp, &bestpos, holemp->x[l], holemp->y[l])) return(TRUE);
#ifdef SHOWHOLEGROWTH
								ttyputmsg(x_(" Adding carveout point (%s,%s)"), latoa(holemp->x[l], 0), latoa(holemp->y[l], 0));
#endif
								if (l == lastj) break;
							}
							besthole = holemp;
							bestlasthole = lasthole;
							solved = 1;
							break;
						}
					}
					if (hfy == hty && hfy == nfy && nfy == nty)
					{
						if (mini(hfx, htx) < maxi(nfx, ntx) && maxi(hfx, htx) > mini(nfx, ntx))
						{
							bestpos = k;
							l = lastj;
							for(;;)
							{
								l++;
								if (l >= holemp->count) l = 0;
								if (db_mergeinsertpoint(merge, newmp, &bestpos, holemp->x[l], holemp->y[l])) return(TRUE);
								if (l == lastj) break;
							}
							besthole = holemp;
							bestlasthole = lasthole;
							solved = 1;
							break;
						}
					}

					/* not colinear: look for proximity */
					if (newmp->seen[k] == 0)
					{
						dist = computedistance(holemp->x[j], holemp->y[j], newmp->x[k], newmp->y[k]);

						/* LINTED "bestdist" used in proper order */
						if (besthole == NOMPOLY || dist < bestdist)
						{
							bestdist = dist;
							hint = j;
							nint = k;
							besthole = holemp;
							bestlasthole = lasthole;
						}
					}
				}
				if (solved != 0) break;
			}
			lasthole = holemp;
		}
		if (solved == 0)
		{
			if (besthole == NOMPOLY)
			{
				ttyputerr(x_("ERROR merging polygon: cannot remove hole"));
				break;
			}
			bestpos = nint+1;
			nx = newmp->x[nint];   ny = newmp->y[nint];
#ifdef SHOWHOLEGROWTH
			ttyputmsg(x_("Hole point %d (%s,%s) closest to polygon point %ld (%s,%s)"),hint, latoa(besthole->x[hint], 0),
				latoa(besthole->y[hint], 0), nint, latoa(nx), latoa(ny));
#endif
			if (nx != besthole->x[hint] || ny != besthole->y[hint])
			{
#ifdef SHOWHOLEGROWTH
				ttyputmsg(x_(" Adding bridge point (%s,%s)"), latoa(besthole->x[hint], 0), latoa(besthole->y[hint], 0));
#endif
				if (db_mergeinsertpoint(merge, newmp, &bestpos, besthole->x[hint], besthole->y[hint])) return(TRUE);
			}
			j = hint;
			for(;;)
			{
				j++;
				if (j >= besthole->count) j = 0;
				if (db_mergeinsertpoint(merge, newmp, &bestpos, besthole->x[j], besthole->y[j])) return(TRUE);
#ifdef SHOWHOLEGROWTH
				ttyputmsg(x_(" Adding point (%s,%s)"), latoa(besthole->x[j], 0), latoa(besthole->y[j], 0));
#endif
				if (j == hint) break;
			}
			if (nx != besthole->x[hint] || ny != besthole->y[hint])
			{
#ifdef SHOWHOLEGROWTH
				ttyputmsg(x_(" Adding bridge point (%s,%s)"), latoa(nx, 0), latoa(ny, 0));
#endif
				if (db_mergeinsertpoint(merge, newmp, &bestpos, nx, ny)) return(TRUE);
			}
		}

#ifdef SHOWHOLEGROWTH
		{
			REGISTER NODEPROTO *cell, *prim;
			REGISTER NODEINST *ni;
			REGISTER INTBIG i, lx, hx, ly, hy, cx, cy, *newlist;
			static INTBIG db_mergeshowxoff = 0;

			db_mergeshowxoff += el_curlib->lambda[el_curtech->techindex] * RESULTSPACING;
			cell = getcurcell();
			if (cell == NONODEPROTO) return(TRUE);
			prim = getnodeproto(x_("metal-2-node"));
			lx = hx = besthole->x[0];
			ly = hy = besthole->y[0];
			for(i=1; i<besthole->count; i++)
			{
				if (besthole->x[i] < lx) lx = besthole->x[i];
				if (besthole->x[i] > hx) hx = besthole->x[i];
				if (besthole->y[i] < ly) ly = besthole->y[i];
				if (besthole->y[i] > hy) hy = besthole->y[i];
			}
			cx = (lx+hx) / 2;   cy = (ly+hy) / 2;
			ni = newnodeinst(prim, lx+db_mergeshowxoff, hx+db_mergeshowxoff, ly, hy, 0, 0, cell);
			newlist = (INTBIG *)emalloc(besthole->count * 2 * SIZEOFINTBIG, el_tempcluster);
			if (newlist == 0) return(TRUE);
			for(i=0; i<besthole->count; i++)
			{
				newlist[i*2] = besthole->x[i] - cx;
				newlist[i*2+1] = besthole->y[i] - cy;
			}
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
				VINTEGER|VISARRAY|((besthole->count*2)<<VLENGTHSH));
			efree((CHAR *)newlist);
			endobjectchange((INTBIG)ni, VNODEINST);

			prim = getnodeproto(x_("metal-1-node"));
			lx = hx = newmp->x[0];
			ly = hy = newmp->y[0];
			for(i=1; i<newmp->count; i++)
			{
				if (newmp->x[i] < lx) lx = newmp->x[i];
				if (newmp->x[i] > hx) hx = newmp->x[i];
				if (newmp->y[i] < ly) ly = newmp->y[i];
				if (newmp->y[i] > hy) hy = newmp->y[i];
			}
			cx = (lx+hx) / 2;   cy = (ly+hy) / 2;
			ni = newnodeinst(prim, lx+db_mergeshowxoff, hx+db_mergeshowxoff, ly, hy, 0, 0, cell);
			newlist = (INTBIG *)emalloc(newmp->count * 2 * SIZEOFINTBIG, el_tempcluster);
			if (newlist == 0) return(TRUE);
			for(i=0; i<newmp->count; i++)
			{
				newlist[i*2] = newmp->x[i] - cx;
				newlist[i*2+1] = newmp->y[i] - cy;
			}
			(void)setvalkey((INTBIG)ni, VNODEINST, el_trace_key, (INTBIG)newlist,
				VINTEGER|VISARRAY|((newmp->count*2)<<VLENGTHSH));
			efree((CHAR *)newlist);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
#endif
		if (bestlasthole == NOMPOLY) holemplist = besthole->nextmpoly; else
			bestlasthole->nextmpoly = besthole->nextmpoly;
		db_mergefreepoly(besthole);
	}
	return(FALSE);
}

/*
 * Helper routine to look for intersection points between the two polygons "mp" and "omp"
 * (which may be the same polygon, when looking for self-intersections).  The intersection
 * points are added to the polygon and marked (1 for self-intersection, 2 for intersection
 * with the other polygon).  Returns nonzero on error.
 */
int db_mergesortiny(const void *e1, const void *e2)
{
	REGISTER INTBIG i1, i2, lasti1, lasti2, y1, y2;

	i1 = *((INTBIG *)e1);
	i2 = *((INTBIG *)e2);
	if (i1 == 0) lasti1 = db_mergesortpoly->count - 1; else
		lasti1 = i1 - 1;
	if (i2 == 0) lasti2 = db_mergesortpoly->count - 1; else
		lasti2 = i2 - 1;
	y1 = mini(db_mergesortpoly->y[i1], db_mergesortpoly->y[lasti1]);
	y2 = mini(db_mergesortpoly->y[i2], db_mergesortpoly->y[lasti2]);
	return(y1 - y2);
}

BOOLEAN db_mergeaddintersectionpoints(MERGE *merge, MPOLY *mp, MPOLY *omp)
{
	REGISTER INTBIG i, j, y, toty, oi, fy, ty, ofy, oty, last, olast,
		intvalue, cury, fromline, ofromline, startpos;
	REGISTER BOOLEAN keepon;

	if (mp == omp) intvalue = 1; else intvalue = 2;

	/* get list of line segments, sorted by low y */
	if (ensureintlistobj(merge->yorder, mp->count)) return(TRUE);
	if (ensureintlistobj(merge->Oyorder, omp->count)) return(TRUE);
	merge->yorder->count = mp->count;
	merge->Oyorder->count = omp->count;
	for(i=0; i<merge->yorder->count; i++) merge->yorder->list[i] = i;
	db_mergesortpoly = mp;
	esort(merge->yorder->list, merge->yorder->count, SIZEOFINTBIG, db_mergesortiny);
	if (omp == mp)
	{
		for(oi=0; oi<merge->Oyorder->count; oi++) merge->Oyorder->list[oi] = merge->yorder->list[oi];
	} else
	{
		for(oi=0; oi<merge->Oyorder->count; oi++) merge->Oyorder->list[oi] = oi;
		db_mergesortpoly = omp;
		esort(merge->Oyorder->list, merge->Oyorder->count, SIZEOFINTBIG, db_mergesortiny);
	}

	/* get ordered list of relevant Y coordinates */
	toty = merge->yorder->count;
	if (omp != mp) toty += merge->Oyorder->count;
	if (ensureintlistobj(merge->ylist, toty)) return(TRUE);
	j = 0;
	for(i=0; i<merge->yorder->count; i++) merge->ylist->list[j++] = mp->y[i];
	if (omp != mp)
		for(oi=0; oi<merge->Oyorder->count; oi++) merge->ylist->list[j++] = omp->y[oi];
	esort(merge->ylist->list, toty, SIZEOFINTBIG, sort_intbigascending);
	j = 0;
	for(i=0; i<toty; i++)
	{
		if (j != 0 && merge->ylist->list[i] == merge->ylist->list[j-1]) continue;
		merge->ylist->list[j++] = merge->ylist->list[i];
	}
	toty = j;

	/* clear the queue of active lines in each polygon */
	keepon = TRUE;
	while (keepon)
	{
		keepon = FALSE;
		fromline = 0;
		merge->queue->count = 0;
		ofromline = 0;
		merge->Oqueue->count = 0;

		/* loop through relevant y coordinates */
		for(y=0; y<toty; y++)
		{
			cury = merge->ylist->list[y];

			/* remove anything at or below "cury" from the active queues */
			for(j=0; j<merge->queue->count; j++)
			{
				i = merge->queue->list[j];
				if (i == 0) last = mp->count - 1; else
					last = i - 1;
				if (mp->y[i] < cury && mp->y[last] < cury)
				{
					merge->queue->list[j] = merge->queue->list[merge->queue->count-1];
					merge->queue->count--;
					j--;
					continue;
				}
			}
			for(j=0; j<merge->Oqueue->count; j++)
			{
				oi = merge->Oqueue->list[j];
				if (oi == 0) olast = omp->count - 1; else
					olast = oi - 1;
				if (omp->y[oi] < cury && omp->y[olast] < cury)
				{
					merge->Oqueue->list[j] = merge->Oqueue->list[merge->Oqueue->count-1];
					merge->Oqueue->count--;
					j--;
					continue;
				}
			}

			/* add any new objects */
			while (fromline < merge->yorder->count)
			{
				i = merge->yorder->list[fromline];
				if (i == 0) last = mp->count - 1; else
					last = i - 1;
				fy = mp->y[last];   ty = mp->y[i];
				if (fy > cury && ty > cury) break;

				if (addtointlistobj(merge->queue, i, FALSE)) return(TRUE);
				fromline++;

				/* check and make merges of all new lines against existing list */
				startpos = merge->queue->count-1;
				while (db_mergecheckqueue(merge,
					mp, merge->queue, merge->yorder, startpos, fromline,
					omp, merge->Oqueue, merge->Oyorder, ofromline, intvalue));
			}
			if (merge->queue->count == 0 && fromline >= merge->yorder->count) break;

			while (ofromline < merge->Oyorder->count)
			{
				oi = merge->Oyorder->list[ofromline];
				if (oi == 0) olast = omp->count - 1; else
					olast = oi - 1;
				ofy = omp->y[olast];   oty = omp->y[oi];
				if (ofy > cury && oty > cury) break;
				if (addtointlistobj(merge->Oqueue, oi, FALSE)) return(TRUE);
				ofromline++;

				/* check and make merges of all new lines against existing list */
				startpos = merge->Oqueue->count-1;
				while (db_mergecheckqueue(merge,
					omp, merge->Oqueue, merge->Oyorder, startpos, ofromline,
					mp, merge->queue, merge->yorder, fromline, intvalue));
			}
			if (merge->Oqueue->count == 0 && ofromline >= merge->Oyorder->count) break;
		}
	}
	return(FALSE);
}

/*
 * Routine to examine queued lines starting at "startqueuepos" and add
 * intersection points as necessary.  When a point has been added, returns TRUE.
 */
BOOLEAN db_mergecheckqueue(MERGE *merge,
	MPOLY *mp, LISTINTBIG *queue, LISTINTBIG *yorder, INTBIG startqueuepos, INTBIG fromline,
	MPOLY *omp, LISTINTBIG *Oqueue, LISTINTBIG *Oyorder, INTBIG ofromline, INTBIG intvalue)
{
	REGISTER INTBIG i, j, k, m, ic, fx, fy, tx, ty, ofx, ofy, otx, oty, last, olast, oi, icount,
		ix, iy, hasint, ohasint;
	INTBIG ixa[2], iya[2];

	/* check new lines in this queue */
	for(m=startqueuepos; m < queue->count; m++)
	{
		i = queue->list[m];
		if (i == 0) last = mp->count - 1; else
			last = i - 1;
		fx = mp->x[last];   tx = mp->x[i];
		fy = mp->y[last];   ty = mp->y[i];

		/* check against everything in the other queue */
		for(j=0; j < Oqueue->count; j++)
		{
			oi = Oqueue->list[j];
			if (oi == 0) olast = omp->count - 1; else
				olast = oi - 1;
			if (mp == omp && (i == oi || last == oi || olast == i)) continue;
			ofx = omp->x[olast];   otx = omp->x[oi];
			ofy = omp->y[olast];   oty = omp->y[oi];
			icount = db_mergelinesintersect(fx,fy, tx,ty, ofx,ofy, otx,oty, ixa,iya);
			for(ic=0; ic<icount; ic++)
			{
				ix = ixa[ic];   iy = iya[ic];

				/* mark points that intersect */
				hasint = 0;
				if (ix == fx && iy == fy)   { mp->intersect[last] = intvalue;    hasint++; }
				if (ix == tx && iy == ty)   { mp->intersect[i] = intvalue;       hasint++; }
				if (hasint == 0)
				{
					if (db_mergeaddthisintersectionpoint(merge, mp, i, ix, iy, intvalue)) return(FALSE);
					for(k=fromline; k<yorder->count; k++)
						if (yorder->list[k] >= i) yorder->list[k]++;
					if (mp == omp)
					{
						for(k=ofromline; k<Oyorder->count; k++)
							if (Oyorder->list[k] >= i) Oyorder->list[k]++;
					}
					for(k=0; k < queue->count; k++)
						if (queue->list[k] >= i) queue->list[k]++;
					if (mp == omp)
					{
						for(k=0; k < Oqueue->count; k++)
							if (Oqueue->list[k] >= i) Oqueue->list[k]++;
					}
					if (addtointlistobj(queue, i, FALSE)) return(FALSE);
					return(TRUE);
				}

				ohasint = 0;
				if (ix == ofx && iy == ofy) { omp->intersect[olast] = intvalue;  ohasint++; }
				if (ix == otx && iy == oty) { omp->intersect[oi] = intvalue;     ohasint++; }
				if (ohasint == 0)
				{
					if (db_mergeaddthisintersectionpoint(merge, omp, oi, ix, iy, intvalue)) return(FALSE);
					for(k=ofromline; k<Oyorder->count; k++)
						if (Oyorder->list[k] >= oi) Oyorder->list[k]++;
					if (mp == omp)
					{
						for(k=fromline; k<yorder->count; k++)
							if (yorder->list[k] >= oi) yorder->list[k]++;
					}
					for(k=0; k < Oqueue->count; k++)
						if (Oqueue->list[k] >= oi) Oqueue->list[k]++;
					if (mp == omp)
					{
						for(k=0; k < queue->count; k++)
							if (queue->list[k] >= i) queue->list[k]++;
					}
					if (addtointlistobj(Oqueue, oi, FALSE)) return(FALSE);
					return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}

BOOLEAN db_mergeaddthisintersectionpoint(MERGE *merge, MPOLY *mp, INTBIG i, INTBIG ix, INTBIG iy, INTBIG intvalue)
{
	/* add intersection point to the first polygon */
	if (db_mergeensurespace(merge, mp, mp->count+1)) return(TRUE);
	memmove(&mp->x[i+1], &mp->x[i], (mp->count-i)*SIZEOFINTBIG);
	memmove(&mp->y[i+1], &mp->y[i], (mp->count-i)*SIZEOFINTBIG);
	memmove(&mp->intersect[i+1], &mp->intersect[i], (mp->count-i)*SIZEOFINTBIG);
	mp->count++;
	mp->x[i] = ix;
	mp->y[i] = iy;
	mp->intersect[i] = intvalue;
	return(FALSE);
}

/*
 * Helper routine to walk around the edge of polygon "mp", starting at point "i".  If it hits
 * polygon "omp", merge it in.  Resulting polygon is in "newmp".  Returns true on error.
 */
BOOLEAN db_mergewalkedge(MERGE *merge, MPOLY *mp, INTBIG i, MPOLY *omp, MPOLY *newmp, BOOLEAN subtract)
{
	MPOLY *mps[2];
	REGISTER MPOLY *mp1, *mp2;
	REGISTER INTBIG enterangle, leaveangle;
	REGISTER INTBIG startpol, startpt, last, next, pol, pt, unseenpt,
		unseenthis, smallestthisangle, smallestotherangle;
	INTBIG insert;

#ifdef DEBUGWALK
	ttyputmsg(x_("Walking around edge, polygon 0 has %ld points, polygon 1 has %ld"), mp->count, omp->count);
#endif
	merge->seen++;
	if (merge->seen == 0) merge->seen++;
	mps[0] = mp;   mps[1] = omp;
	startpol = 0;   startpt = i;
	pol = startpol;   pt = startpt;
	insert = 0;
	for(;;)
	{
		/* include this point */
		mp1 = mps[pol];
		mp2 = mps[1-pol];
		mp1->seen[pt] = merge->seen;
#ifdef DEBUGWALK
		ttyputmsg(x_("  Add (%s,%s) from polygon %ld"), latoa(mps[pol]->x[pt], 0), latoa(mps[pol]->y[pt], 0), pol);
#endif
		if (db_mergeinsertpoint(merge, newmp, &insert, mps[pol]->x[pt], mps[pol]->y[pt]))
			return(TRUE);
		if (newmp->count == 5000)
		{
			if (!merge->loopwarned)
			{
#ifdef SHOWRESULTS
				REGISTER INTBIG lambda;
				NODEINST *ni;
				NODEPROTO *cell, *prim;

				lambda = el_curlib->lambda[el_curtech->techindex];
				prim = getnodeproto(x_("metal-3-node"));
				cell = getcurcell();
				if (cell != NONODEPROTO)
				{
					ni = newnodeinst(prim, db_mergeshowrightmost+lambda, db_mergeshowrightmost+lambda*5,
						cell->lowy, cell->highy, 0, 0, cell);
				}
				endobjectchange((INTBIG)ni, VNODEINST);
#endif
				ttyputerr(_("Warning: loop detected while merging polygons on layer %s"),
					layername(mp->tech, mp->layer));
				merge->loopwarned = TRUE;
			}
			return(TRUE);
		}

		/* see if there are intersection points on this or the other polygon */
		merge->intcount = 0;
		for(i=0; i<mp1->count; i++)
		{
			if (i == pt) continue;
			if (mp1->x[i] != mp1->x[pt] || mp1->y[i] != mp1->y[pt]) continue;
			db_mergeaddintersectionpoint(merge, pol, i);
		}
		for(i=0; i<mp2->count; i++)
		{
			if (mp2->x[i] != mp1->x[pt] || mp2->y[i] != mp1->y[pt]) continue;
			db_mergeaddintersectionpoint(merge, 1-pol, i);
		}

		/* if point not on other polygon or elsewhere on this, keep walking around edge */
		if (merge->intcount == 0)
		{
#ifdef DEBUGWALK
			ttyputmsg(x_("      Not an intersection point"));
#endif
			pt++;
			if (pt >= mp1->count) pt = 0;
			if (pol == startpol && pt == startpt) break;
			continue;
		}

		/* intersection point: figure out what to do */
		if (pt == 0) last = mp1->count-1; else last = pt-1;
		enterangle = figureangle(mp1->x[pt], mp1->y[pt], mp1->x[last], mp1->y[last]);

		if (pt == mp1->count-1) next = 0; else next = pt+1;
		leaveangle = figureangle(mp1->x[pt], mp1->y[pt], mp1->x[next], mp1->y[next]);
		if (leaveangle > enterangle) leaveangle -= 3600;
		leaveangle = enterangle - leaveangle;

#ifdef DEBUGWALK
		ttyputmsg(x_("      Intersection point!  enter at (%s,%s)=%ld, leave at (%s,%s)=%ld"),
			latoa(mp1->x[last], 0), latoa(mp1->y[last], 0), enterangle/10, latoa(mp1->x[next], 0), latoa(mp1->y[next], 0), leaveangle/10);
#endif
		for(i=0; i<merge->intcount; i++)
		{
			if (merge->intpt[i] == mps[merge->intpol[i]]->count-1) next = 0; else next = merge->intpt[i]+1;
			merge->intleaveangle[i] = figureangle(mp1->x[pt], mp1->y[pt],
				mps[merge->intpol[i]]->x[next], mps[merge->intpol[i]]->y[next]);
			if (merge->intleaveangle[i] > enterangle) merge->intleaveangle[i] -= 3600;
			merge->intleaveangle[i] = enterangle - merge->intleaveangle[i];
			if (merge->intleaveangle[i] == 0) merge->intleaveangle[i] = 7200;
#ifdef DEBUGWALK
			ttyputmsg(x_("        Point on %s polygon goes to (%s,%s), angle=%ld"),
				(merge->intpol[i] == pol ? x_("this") : x_("other")), latoa(mps[merge->intpol[i]]->x[next], 0),
				latoa(mps[merge->intpol[i]]->y[next], 0), merge->intleaveangle[i]/10);
#endif
		}

		smallestthisangle = 7200;
		unseenthis = -1;
		for(i=0; i<merge->intcount; i++)
		{
			if (merge->intpol[i] != pol) continue;
			if (merge->intleaveangle[i] > smallestthisangle) continue;
			smallestthisangle = merge->intleaveangle[i];
			unseenthis = merge->intpt[i];
		}

		smallestotherangle = 7200;
		unseenpt = -1;
		for(i=0; i<merge->intcount; i++)
		{
			if (merge->intpol[i] == pol) continue;
			if (merge->intleaveangle[i] > smallestotherangle) continue;
			smallestotherangle = merge->intleaveangle[i];
			unseenpt = merge->intpt[i];
		}
		if (subtract)
		{
			if (smallestthisangle != 7200) smallestthisangle = 3600 - smallestthisangle;
			leaveangle = 3600 - leaveangle;
			if (smallestotherangle != 7200) smallestotherangle = 3600 - smallestotherangle;
		}
#ifdef DEBUGWALK
		ttyputmsg(x_("        So smallestTHISangle=%ld, smallestOTHERangle=%ld, leaveangle=%ld"),
			smallestthisangle, smallestotherangle, leaveangle);
#endif
		if (smallestthisangle < leaveangle && smallestthisangle < smallestotherangle)
		{
			/* staying on the same polygon but jumping to a new point */
#ifdef DEBUGWALK
			ttyputmsg(x_("        Stay on same polygon, jump to new point"));
#endif
			pt = unseenthis;
			if (pol == startpol && pt == startpt) break;
			mp1->seen[pt] = merge->seen;

			/* advance to the next point */
			pt++;
			if (pt >= mp1->count) pt = 0;
			if (pol == startpol && pt == startpt) break;
			continue;
		}
		if (leaveangle <= smallestthisangle && leaveangle <= smallestotherangle)
		{
			/* stay on the current polygon */
#ifdef DEBUGWALK
			ttyputmsg(x_("        Stay on same polygon"));
#endif
			pt++;
			if (pt >= mp1->count) pt = 0;
		} else
		{
			/* switch to the other polygon */
#ifdef DEBUGWALK
			ttyputmsg(x_("        Switch to other polygon"));
#endif
			pt = unseenpt;
			pol = 1 - pol;
			if (pol == startpol && pt == startpt) break;
			mp2->seen[pt] = merge->seen;

			/* advance to the next point */
			pt++;
			if (pt >= mp2->count) pt = 0;
		}
		if (pol == startpol && pt == startpt) break;
	}
	return(FALSE);
}

/*
 * Helper routine to insert the point (x, y) at "insert" in polygon "mp".
 * Returns true on error.
 */
BOOLEAN db_mergeinsertpoint(MERGE *merge, MPOLY *mp, INTBIG *insert, INTBIG x, INTBIG y)
{
	REGISTER INTBIG index;

	if (db_mergeensurespace(merge, mp, mp->count+1)) return(TRUE);
	index = *insert;
	if (index <= 0 || mp->x[index-1] != x || mp->y[index-1] != y)
	{
		memmove(&mp->x[index+1], &mp->x[index], (mp->count - index)*SIZEOFINTBIG);
		memmove(&mp->y[index+1], &mp->y[index], (mp->count - index)*SIZEOFINTBIG);
		mp->x[index] = x;
		mp->y[index] = y;
		mp->count++;
		(*insert)++;
	}
	return(FALSE);
}

/*
 * Routine to add polygon "polygon" and point "point" to the global arrays of 
 * intersection points.
 */
void db_mergeaddintersectionpoint(MERGE *merge, INTBIG polygon, INTBIG point)
{
	REGISTER INTBIG i, newtotal, *newpol, *newpt, *newangle;

	if (merge->intcount >= merge->inttotal)
	{
		newtotal = merge->inttotal * 2;
		if (newtotal <= 0) newtotal = 10;
		newpol = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, merge->clus);
		if (newpol == 0) return;
		newpt = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, merge->clus);
		if (newpt == 0) return;
		newangle = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, merge->clus);
		if (newangle == 0) return;
		for(i=0; i<merge->intcount; i++)
		{
			newpol[i] = merge->intpol[i];
			newpt[i] = merge->intpt[i];
			newangle[i] = merge->intleaveangle[i];
		}
		if (merge->inttotal > 0)
		{
			efree((CHAR *)merge->intpol);
			efree((CHAR *)merge->intpt);
			efree((CHAR *)merge->intleaveangle);
		}
		merge->intpol = newpol;
		merge->intpt = newpt;
		merge->intleaveangle = newangle;
		merge->inttotal = newtotal;
	}
	merge->intpol[merge->intcount] = polygon;
	merge->intpt[merge->intcount] = point;
	merge->intcount++;
}

/*
 * Helper routine to determine whether the line segement (fx,fy)-(tx,ty)
 * intersects the line segment (ofx,ofy)-(otx,oty).  If it does, the intersection
 * point(s) are placed in (ix,iy) and the routine returns the number of intersections
 * (parallel lines that are offset can have 2 intersection points).
 */
INTBIG db_mergelinesintersect(INTBIG fx, INTBIG fy, INTBIG tx, INTBIG ty,
	INTBIG ofx, INTBIG ofy, INTBIG otx, INTBIG oty, INTBIG *ix, INTBIG *iy)
{
	double fa1, fb1, fc1, fa2, fb2, fc2, fswap, fiy, ang, oang, hang, ohang;
	REGISTER INTBIG maxx, maxy, minx, miny, omaxx, omaxy, ominx, ominy, intpoints;

	/* quick check to see if lines don't intersect */
	if (fx > tx) { maxx = fx;   minx = tx; } else { maxx = tx;   minx = fx; }
	if (fy > ty) { maxy = fy;   miny = ty; } else { maxy = ty;   miny = fy; }
	if (ofx > otx) { omaxx = ofx;   ominx = otx; } else { omaxx = otx;   ominx = ofx; }
	if (ofy > oty) { omaxy = ofy;   ominy = oty; } else { omaxy = oty;   ominy = ofy; }
	if (minx > omaxx) return(0);
	if (maxx < ominx) return(0);
	if (miny > omaxy) return(0);
	if (maxy < ominy) return(0);

	/* determine the angle of the lines */
	ang = atan2((double)(ty-fy), (double)(tx-fx));
	if (ang < 0.0) ang += EPI*2.0;
	oang = atan2((double)(oty-ofy), (double)(otx-ofx));
	if (oang < 0.0) oang += EPI*2.0;

	/* special case if they are at the same angle */
	intpoints = 0;
	hang = fmod(ang, EPI);
	ohang = fmod(oang, EPI);
	if (fabs(hang - ohang) < 0.0001)
	{
		/* lines are parallel: see if they make intersections on the other */
		if (fx >= ominx && fx <= omaxx && fy >= ominy && fy <= omaxy)
		{
			/* see if (fx,fy) is an intersection point on the other line */
			if ((fx != otx || fy != oty) && (fx != ofx || fy != ofy))
			{
				if (isonline(ofx, ofy, otx, oty, fx, fy))
				{
					ix[intpoints] = fx;
					iy[intpoints] = fy;
					intpoints++;
				}

			}
		}
		if (tx >= ominx && tx <= omaxx && ty >= ominy && ty <= omaxy)
		{
			/* see if (fx,fy) is an intersection point on the other line */
			if ((tx != otx || ty != oty) && (tx != ofx || ty != ofy))
			{
				if (isonline(ofx, ofy, otx, oty, tx, ty))
				{
					ix[intpoints] = tx;
					iy[intpoints] = ty;
					intpoints++;
				}

			}
		}

		if (otx >= minx && otx <= maxx && oty >= miny && oty <= maxy)
		{
			/* see if (fx,fy) is an intersection point on the other line */
			if ((tx != tx || oty != ty) && (otx != fx || oty != fy))
			{
				if (isonline(fx, fy, tx, ty, otx, oty))
				{
					ix[intpoints] = otx;
					iy[intpoints] = oty;
					intpoints++;
				}

			}
		}
		if (ofx >= minx && ofx <= maxx && ofy >= miny && ofy <= maxy)
		{
			/* see if (fx,fy) is an intersection point on the other line */
			if ((ofx != tx || ofy != ty) && (ofx != fx || ofy != fy))
			{
				if (isonline(fx, fy, tx, ty, ofx, ofy))
				{
					ix[intpoints] = ofx;
					iy[intpoints] = ofy;
					intpoints++;
				}

			}
		}
		return(intpoints);
	}

	fa1 = sin(ang);   fb1 = -cos(ang);
	fc1 = -fa1 * ((double)fx) - fb1 * ((double)fy);
	fa2 = sin(oang);   fb2 = -cos(oang);
	fc2 = -fa2 * ((double)ofx) - fb2 * ((double)ofy);
	if (fabs(fa1) < fabs(fa2))
	{
		fswap = fa1;   fa1 = fa2;   fa2 = fswap;
		fswap = fb1;   fb1 = fb2;   fb2 = fswap;
		fswap = fc1;   fc1 = fc2;   fc2 = fswap;
	}
	fiy = (fa2 * fc1 / fa1 - fc2) / (fb2 - fa2*fb1/fa1);
	*iy = rounddouble(fiy);
	*ix = rounddouble((-fb1 * fiy - fc1) / fa1);

	/* make sure the intersection is on the lines */
	if (*ix < minx || *ix < ominx) return(FALSE);
	if (*ix > maxx || *ix > omaxx) return(FALSE);
	if (*iy < miny || *iy < ominy) return(FALSE);
	if (*iy > maxy || *iy > omaxy) return(FALSE);
	return(1);
}

/*
 * Helper routine to simplify polygon "mp", removing redundant and colinear points,
 * and recomputing its bounding box.
 */
void db_mergesimplifyandbbox(MPOLY *mp)
{
	REGISTER INTBIG i, j, prev, next;

	/* remove redundant points */
	for(i=1; i<mp->count; i++)
	{
		if (mp->x[i] == mp->x[i-1] && mp->y[i] == mp->y[i-1])
		{
			for(j=i; j<mp->count; j++)
			{
				mp->x[j-1] = mp->x[j];
				mp->y[j-1] = mp->y[j];
			}
			mp->count--;
			i--;
			continue;
		}
	}
	while (mp->count >= 2 && mp->x[0] == mp->x[mp->count-1] && mp->y[0] == mp->y[mp->count-1])
		mp->count--;

	/* remove colinear points (only manhattan!!!) */
	for(i=0; i<mp->count; i++)
	{
		if (i == 0) prev = mp->count-1; else prev = i-1;
		if (i == mp->count-1) next = 0; else next = i+1;
		if ((mp->x[prev] == mp->x[i] && mp->x[i] == mp->x[next]) ||
			(mp->y[prev] == mp->y[i] && mp->y[i] == mp->y[next]))
		{
			for(j=i+1; j<mp->count; j++)
			{
				mp->x[j-1] = mp->x[j];
				mp->y[j-1] = mp->y[j];
			}
			mp->count--;
			i--;
			continue;
		}
	}

	/* get bounding box */
	mp->lx = mp->hx = mp->x[0];
	mp->ly = mp->hy = mp->y[0];
	for(i=1; i<mp->count; i++)
	{
		if (mp->x[i] < mp->lx) mp->lx = mp->x[i];
		if (mp->x[i] > mp->hx) mp->hx = mp->x[i];
		if (mp->y[i] < mp->ly) mp->ly = mp->y[i];
		if (mp->y[i] > mp->hy) mp->hy = mp->y[i];
	}
}

/*
 * Helper routine to return true if point (x,y) is inside of polygon "mp".
 */
BOOLEAN db_mergeisinside(INTBIG x, INTBIG y, MPOLY *mp)
{
	REGISTER INTBIG i, ang, lastp, tang, thisp;

	/* general polygon containment by summing angles to vertices */
	ang = 0;
	if (x == mp->x[mp->count-1] && y == mp->y[mp->count-1]) return(TRUE);
	lastp = figureangle(x, y, mp->x[mp->count-1], mp->y[mp->count-1]);
	for(i=0; i<mp->count; i++)
	{
		if (x == mp->x[i] && y == mp->y[i]) return(TRUE);
		thisp = figureangle(x, y, mp->x[i], mp->y[i]);
		tang = lastp - thisp;
		if (tang < -1800) tang += 3600;
		if (tang > 1800) tang -= 3600;
		ang += tang;
		lastp = thisp;
	}
	if (abs(ang) <= mp->count) return(FALSE);
	return(TRUE);
}

/*
 * Helper routine to copy polygon "smp" to polygon "dmp".
 * Returns true on error.
 */
BOOLEAN db_mergecopypoly(MERGE *merge, MPOLY *smp, MPOLY *dmp)
{
	REGISTER INTBIG i;

	if (db_mergeensurespace(merge, dmp, smp->count)) return(TRUE);
	for(i=0; i<smp->count; i++)
	{
		dmp->x[i] = smp->x[i];
		dmp->y[i] = smp->y[i];
	}
	dmp->count = smp->count;
	dmp->layer = smp->layer;
	return(FALSE);
}

/*
 * Helper routine to make sure that polygon "mp" has room for "count" points.
 * Returns true on error.
 */
BOOLEAN db_mergeensurespace(MERGE *merge, MPOLY *mp, INTBIG count)
{
	REGISTER INTBIG i, *newx, *newy, *newseen, *newintersect, newlimit;

	if (count <= mp->limit) return(FALSE);
	newlimit = mp->limit * 2;
	if (newlimit < count) newlimit = count+4;
	newx = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, merge->clus);
	if (newx == 0) return(TRUE);
	newy = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, merge->clus);
	if (newy == 0) return(TRUE);
	newseen = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, merge->clus);
	if (newseen == 0) return(TRUE);
	newintersect = (INTBIG *)emalloc(newlimit * SIZEOFINTBIG, merge->clus);
	if (newintersect == 0) return(TRUE);
	for(i=0; i<mp->count; i++)
	{
		newx[i] = mp->x[i];
		newy[i] = mp->y[i];
		newseen[i] = mp->seen[i];
		newintersect[i] = mp->intersect[i];
	}
	if (mp->limit > 0)
	{
		efree((CHAR *)mp->x);
		efree((CHAR *)mp->y);
		efree((CHAR *)mp->seen);
		efree((CHAR *)mp->intersect);
	}
	mp->limit = newlimit;
	mp->x = newx;
	mp->y = newy;
	mp->seen = newseen;
	mp->intersect = newintersect;
	return(FALSE);
}

MERGE *db_mergealloc(CLUSTER *cluster)
{
	REGISTER MERGE *merge;

	merge = NOMERGE;
	if (db_multiprocessing) emutexlock(db_polymergemutex);
	if (db_mergefree != NOMERGE)
	{
		merge = db_mergefree;
		db_mergefree = db_mergefree->nextmerge;
	}
	if (db_multiprocessing) emutexunlock(db_polymergemutex);
	if (merge == NOMERGE)
	{
		merge = (MERGE *)emalloc(sizeof (MERGE), cluster);
		if (merge == 0) return(NOMERGE);
		merge->clus = cluster;
		merge->newmp = NOMPOLY;
		merge->inttotal = 0;
		merge->yorder = newintlistobj(cluster);
		merge->Oyorder = newintlistobj(cluster);
		merge->queue = newintlistobj(cluster);
		merge->Oqueue = newintlistobj(cluster);
		merge->ylist = newintlistobj(cluster);
	}
	return(merge);
}

/*
 * Helper routine to allocate a new polygon.
 * Returns NOMPOLY on error.
 */
MPOLY *db_mergeallocpoly(MERGE *merge)
{
	REGISTER MPOLY *mp;

	mp = NOMPOLY;
	if (db_multiprocessing) emutexlock(db_polymergemutex);
	if (db_mpolyfree != NOMPOLY)
	{
		mp = db_mpolyfree;
		db_mpolyfree = mp->nextmpoly;
	}
	if (db_multiprocessing) emutexunlock(db_polymergemutex);

	if (mp == NOMPOLY)
	{
		mp = (MPOLY *)emalloc(sizeof (MPOLY), merge->clus);
		if (mp == 0) return(NOMPOLY);
		mp->limit = 0;
	}
	mp->count = 0;
	return(mp);
}

/*
 * Helper routine to free polygon "mp".
 */
void db_mergefreepoly(MPOLY *mp)
{
	if (db_multiprocessing) emutexlock(db_polymergemutex);
	mp->nextmpoly = db_mpolyfree;
	db_mpolyfree = mp;
	if (db_multiprocessing) emutexunlock(db_polymergemutex);
}
