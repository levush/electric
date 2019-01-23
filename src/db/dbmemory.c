/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbmemory.c
 * Database virtual memory control module
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
#include "database.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef DEBUGMEMORY					/* memory clustering only if debugging memory */
#  define USECLUSTERS  1
#endif

#ifdef USECLUSTERS
#  if defined(ONUNIX)                             /* WINDOW and MAC already do filling */ 
#    define FILLBYTE       0x41                     /* byte to fill emalloc memory */
#  endif

/* the information bits in the header word */
#  define FREE		0			/* block is free */
#  define USED		1			/* block is in use */
#  define SKIP		2			/* block is dummy for re-pointing */
#  define FLAGBITS	3			/* the above flag bits */

#  define ROUNDUP (SIZEOFINTBIG+SIZEOFINTBIG) /* slop to give whole block */
#  define SPAREPAGELIMIT 20		/* spare page groups to save */
#  define PAGEBLOCK      64		/* amount to allocate at once */

typedef struct
{
	INTBIG  pagecount;
	UCHAR1 *pageaddress;
} SPAREPAGES;
static SPAREPAGES db_sparepages[SPAREPAGELIMIT];
static INTBIG db_sparepagecount = 0;
static INTBIG db_pagesize, db_pagebits;
INTBIG        db_clustercount = 0;
CLUSTER     *db_firstcluster;	/* top of list of clusters */
CLUSTER     *db_clusterfree;	/* list of free memory clusters */

UCHAR1  *db_getmorememory(CLUSTER*, INTBIG, INTBIG);
UCHAR1  *db_getpages(INTBIG);
void     db_coalescecluster(CLUSTER*);
void     db_printoneclusterarena(CLUSTER*, INTBIG, INTBIG*, INTBIG*, INTBIG*);
#endif

/* prototypes for local routines */
#ifndef HAVE_GETPAGESIZE
  static INTBIG getpagesize(void);
#endif
#ifndef HAVE_VALLOC
  static UCHAR1 *valloc(INTBIG);
#endif
#ifdef DEBUGMEMORY
  static void   db_checkmemoryblock(UCHAR1 *addr);
#  ifdef USECLUSTERS
  static void   db_checkcluster(CLUSTER *clus);
#  endif
  static FILE  *db_memerrio = 0;
  static CHAR   db_initialdirectory[256];
#endif

/*
 * In order to make efficient use of large amounts of memory, this memory
 * allocator works by "clusters" of memory.  Each cluster is a locally used
 * set of memory that is kept in common virtual pages so that they swap
 * together.  Creation of new clusters is done with "alloccluster" and calls
 * to "emalloc" take a cluster argument that determines which cluster to use.
 * Deletion of clusters is done with "freecluster" and deletion of allocated
 * memory is done with "efree".
 *
 * There are clusters for all of the following:
 *   every technology (TECHNOLOGY->cluster)
 *   every library (LIBRARY->cluster)
 *   every tool (TOOL->cluster)
 *   every constraint (CONSTRAINT->cluster)
 *   one for temporary use (el_tempcluster)
 *   one for general database use (db_cluster)
 *
 * The implementation of the cluster system is done on top of "malloc" so
 * that other memory allocation schemes will still work.  The memory arena
 * in a cluster's address space consists of 4-byte header words between each
 * block of memory.  The low two bits of the header word have flags that
 * determine the nature of the block.  Since all blocks are on even word
 * (4 byte) boundaries, this works fine.  Beware of porting this to other
 * types of machines.  The flag bits in the header word are as follows:
 *   FREE  means that the block is free to be allocated.
 *   USED  means that the block has been allocated already.
 *   SKIP  means that the block is nonexistant and this is simply a pointer
 *         to the next valid header word.  This happens at the end of the
 *         arena since it is circularly linked.  It also happens when a
 *         cluster gets full but needs more memory.  Thus, the arena of a
 *         cluster need not be contiguous.
 *
 * freed blocks are not coalesced, however when new memory is allocated,
 * successive free blocks are then coalesced.  The memory allocator returns
 * the first free block that is large enough.  It moves its start pointer to
 * the last block allocated so that it is always starting at a different
 * place.
 */

/*
 * routine to initialize the memory cluster system.  Allocates the two
 * global clusters: "db_cluster" and "el_tempcluster"..
 */
void db_initclusters(void)
{
#ifdef	USECLUSTERS
	REGISTER INTBIG i;

	/* get the virtual page size of this system */
	db_pagesize = getpagesize();
	for(i = db_pagesize, db_pagebits = -1; i != 0; i >>= 1, db_pagebits++)
		;

	/* create the database cluster by hand since it is the first */
	db_cluster = (CLUSTER *)malloc(sizeof (CLUSTER));
	if (db_cluster == 0) error(_("Memory system cannot initialize"));
	db_cluster->clustersize = 0;
	db_cluster->flags = CLUSTERFILLING;
	(void)estrcpy(db_cluster->clustername, x_("DATABASE"));

	/* create the linked list of active clusters */
	db_firstcluster = db_cluster;
	db_cluster->nextcluster = NOCLUSTER;

	/* initialize the linked list of free clusters */
	db_clusterfree = NOCLUSTER;
#else
	db_cluster = alloccluster(x_("DATABASE"));
#endif

#ifdef DEBUGMEMORY
	estrcpy(db_initialdirectory, currentdirectory());
#endif

	/* create the temporary cluster normally */
	el_tempcluster = alloccluster(x_("TEMPORARY"));
}

/*
 * routine to allocate a new memory cluster called "name".  Returns the
 * address of the cluster for subsequent calls to "emalloc".  Returns
 * NOCLUSTER if there is an error.
 */
CLUSTER *alloccluster(CHAR *name)
{
#ifdef	USECLUSTERS
	REGISTER CLUSTER *clus;

	if (db_clusterfree == NOCLUSTER)
	{
		/* no free clusters: allocate one and put it on the free list */
		clus = (CLUSTER *)emalloc((sizeof (CLUSTER)), db_cluster);
		if (clus == 0) return(NOCLUSTER);
		clus->clustersize = 0;
	} else
	{
		/* take a cluster from the linked list of free ones */
		clus = db_clusterfree;
		db_clusterfree = clus->nextcluster;
	}

	/* initialize this cluster */
	clus->nextcluster = db_firstcluster;
	db_firstcluster = clus;
	clus->flags = CLUSTERFILLING;
	(void)estrncpy(clus->clustername, name, 29);
	clus->address = clus->clustersize = 0;
	db_clustercount++;
	return(clus);
#else
	static CLUSTER clus;
	Q_UNUSED( name );

	clus.nextcluster = NOCLUSTER;
	clus.clustersize = 0;
	(void)estrcpy(clus.clustername, x_("NOCLUSTERNAME"));
	clus.address = clus.flags = 0;
	return(&clus);
#endif
}

/*
 * routine to free cluster "clus".  No further use may be made of it.
 */
void freecluster(CLUSTER *clus)
{
#ifdef	USECLUSTERS
	REGISTER CLUSTER *lastclus, *thisclus;
	REGISTER INTBIG count, *start, *nextptr, *ptr, head;

	/* do not free a null pointer */
	if (clus == NOCLUSTER) return;

#  ifdef DEBUGMEMORY
	db_checkcluster(clus);
#  endif

	/* run through cluster and make sure everything is "freed" */
	if (clus->clustersize != 0)
	{
		start = ptr = (INTBIG *)clus->address;
		for(;;)
		{
			/* get the head of a block in this cluster */
			head = *ptr;

			/* if block is "skip", skip it */
			if ((head&FLAGBITS) == SKIP)
			{
				ptr = (INTBIG *)(head & ~FLAGBITS);
				if (ptr == start) break;
				continue;
			}

			/* if block is still in use, free it */
			if ((head&FLAGBITS) == USED)
			{
				*ptr &= ~FLAGBITS;
				head = *ptr;
			}

			/* coalesce this and any subsequent used or free blocks */
			for(;;)
			{
				nextptr = (INTBIG *)head;

				/* stop coalescing if the next block is a skip */
				if ((*nextptr & FLAGBITS) == SKIP) break;
				if ((*nextptr & FLAGBITS) == USED) *nextptr &= ~FLAGBITS;

				/* if the coalesced block is the start, update start pointer */
				if (nextptr == start)
				{
					clus->address = *nextptr;
					start = (INTBIG *)*nextptr;
				}
				head = *nextptr;
			}
			*ptr = head;

			/* go on to the next one */
			ptr = (INTBIG *)head;
			if (ptr == start) break;
		}
	}

	/* this loop takes time (list should be doubly linked!!!) */
	lastclus = NOCLUSTER;
	for(count=0, thisclus = db_firstcluster; thisclus != NOCLUSTER;
		thisclus = thisclus->nextcluster, count++)
	{
		if (thisclus == clus)
		{
			if (lastclus == NOCLUSTER) db_firstcluster = clus->nextcluster; else
				lastclus->nextcluster = clus->nextcluster;
			break;
		}

		/* really don't need this check any more!!! */
		if (count > db_clustercount+10)
		{
			ttyputerr(_("Warning: freecluster loop, memory arena questionable"));
			ttyputerr(_(" Recommended action: save and quit"));
			return;
		}
		lastclus = thisclus;
	}

	/* put the cluster in the free list */
	clus->nextcluster = db_clusterfree;
	db_clusterfree = clus;
	db_clustercount--;
#else
	Q_UNUSED( clus );
#endif
}

/*
 * this is the cluster memory allocation routine that should replace calls
 * to "malloc".  The "amount" parameter is the number of bytes to allocate
 * and it is treated the same as with "malloc".  The "cluster" parameter is
 * the memory cluster to use for this memory.  Returns the address of the
 * memory if successful, 0 if not.
 */
#ifdef DEBUGMEMORY
INTBIG *_emalloc(INTBIG amount, CLUSTER *cluster, CHAR *file, INTBIG line)
#else
INTBIG *emalloc(INTBIG amount, CLUSTER *cluster)
#endif
{
	REGISTER UCHAR1 *addr;
#ifdef	DEBUGMEMORY
	REGISTER INTBIG *pt, filelen, endf;
#endif
#ifdef	USECLUSTERS
	REGISTER INTBIG *ptr, want, head, size, end, *nextptr, *start, atend;
#endif

	if (cluster == NOCLUSTER) ttyputerr(_("Warning: allocated %ld bytes from null cluster!"), amount);

	/* block must be nonzero length */
	if (amount <= 0) amount = 1;

	/* block must be a word amount */
	amount = (amount + SIZEOFINTBIG - 1) & ~(SIZEOFINTBIG - 1);
#ifdef	DEBUGMEMORY
	filelen = estrlen(file);
	for(endf = filelen-1; endf > 0; endf--)
		if (file[endf] == DIRSEP) break;
	if (file[endf] == DIRSEP) file += endf + 1;
	filelen = (estrlen(file)*SIZEOFCHAR+SIZEOFINTBIG) & ~(SIZEOFINTBIG - 1);
	amount += 4 * SIZEOFINTBIG + filelen;
#endif

#ifdef	USECLUSTERS
	if (cluster != NOCLUSTER)
	{
		/* make sure the cluster has memory */
		if (cluster->clustersize == 0)
		{
			if (db_getmorememory(cluster, 1, 0) == 0) return(0);
		}

		/* search for a block that is large enough */
		start = ptr = (INTBIG *)cluster->address;
		atend = 0;
		for(;;)
		{
			/* get the head of a block in this cluster */
			head = *ptr;

			/* if the list has been completely examined with no luck, expand it */
			if (atend != 0)
			{
				/* look for a SKIP block */
				while ((head&FLAGBITS) != SKIP)
				{
					ptr = (INTBIG *)(head & ~FLAGBITS);
					head = *ptr;
				}

				/* determine the number of pages to allocate */
				want = (amount + db_pagesize + 7) >> db_pagebits;

				/* get the memory */
				ptr = (INTBIG *)db_getmorememory(cluster, want, (INTBIG)ptr);
				if (ptr == 0) return(0);
				head = *ptr;

				/* mark this cluster as "growing" */
				cluster->flags |= CLUSTERFILLING;
			}

			/* if block is not free, skip it */
			if ((head&FLAGBITS) != FREE)
			{
#  ifdef DEBUGMEMORY
				if ((head&FLAGBITS) == USED)
					db_checkmemoryblock((UCHAR1 *)(&ptr[1]));
#  endif
				ptr = (INTBIG *)(head & ~FLAGBITS);
				if (ptr == start) atend++;
				continue;
			}

			/* coalesce this and any subsequent free blocks */
			for(;;)
			{
				nextptr = (INTBIG *)head;

				/* stop coalescing if the next block is not free */
				if ((*nextptr & FLAGBITS) != FREE) break;

				/* stop coalescing if there is enough memory in this area */
				if (head - (INTBIG)ptr - (INTBIG)SIZEOFINTBIG >= amount) break;

				/* if the coalesced block is the start, update start pointer */
				if (nextptr == start)
				{
					cluster->address = *nextptr;
					start = (INTBIG *)*nextptr;
				}
				head = *nextptr;
			}
			*ptr = head;

			/* compute the size of this block */
			size = head - (INTBIG)ptr - SIZEOFINTBIG;

			/* if the block is not large enough, go on to the next one */
			if (size < amount)
			{
				ptr = (INTBIG *)head;
				if (ptr == start) atend++;

#  if 0
				/*
				 * if this block is not large enough and the cluster is growing,
				 * don't search it all, just grow the cluster more
				 */
				if ((cluster->flags&CLUSTERFILLING) != 0) atend++;
#  endif
				continue;
			}

			/* compute the address of the returned memory */
			addr = (UCHAR1 *)((INTBIG)ptr + SIZEOFINTBIG);

			/* give the entire block if the request matches close enough */
			if (amount + (INTBIG)ROUNDUP >= size)
			{
				*ptr = head | USED;
				cluster->address = (INTBIG)ptr;
			} else
			{
				/* allocate a part of this block for the requested memory */
				end = (INTBIG)addr + amount;
				*(INTBIG *)end = *ptr;
				*ptr = end | USED;
				cluster->address = (INTBIG)ptr;
			}
			break;
		}
	} else 
#endif /* USECLUSTERS */
	{
		addr = (UCHAR1 *)malloc((size_t)amount);
		if (addr == 0) return(0);
	}

#ifdef FILLBYTE
	memset(addr, FILLBYTE, amount);
#endif
#ifdef DEBUGMEMORY
	pt = &((INTBIG *)addr)[(amount-filelen-4)/SIZEOFINTBIG-1];
	((INTBIG *)addr)[0] = (INTBIG)pt;
	((INTBIG *)addr)[1] = 0xC0CEC0CE;
	*pt++ = 0xC0CEC0CE;
	*pt++ = line;
	estrcpy((CHAR *)pt, file);
	addr += SIZEOFINTBIG * 2;
#endif
	return((INTBIG *)addr);
}

/*
 * routine to free memory obtained with "emalloc".  The memory is kept
 * in the cluster and may be re-used by subsequent calls to "emalloc"
 */
void efree(CHAR *caddr)
{
#ifdef	USECLUSTERS
	REGISTER INTBIG *ptr;
#endif
	REGISTER UCHAR1 *addr;

	/* don't free null pointers */
	addr = (UCHAR1 *)caddr;
	if (addr == ((UCHAR1 *)-1)) return;

#ifdef DEBUGMEMORY
	addr -= SIZEOFINTBIG * 2;
	db_checkmemoryblock((UCHAR1 *)addr);
#endif

#ifdef USECLUSTERS
	/* get header word before this block */
	ptr = (INTBIG *)addr;
	if ((ptr[-1]&FLAGBITS) != USED)
	{
		error(_("Warning: freed block not properly allocated"));
		return;
	}

	/* mark this block free */
	ptr[-1] &= ~FLAGBITS;

#  if 0
	/* mark this cluster as "not growing" */
	cluster->flags &= ~CLUSTERFILLING;
#  endif
#else
	free(addr);
#endif
}

void db_checkallmemoryfree(void)
{
#ifdef DEBUGMEMORY
#  ifdef	USECLUSTERS
	REGISTER INTBIG i;
	REGISTER CLUSTER *clus;

	freecluster(el_tempcluster);
	for(i=0; el_constraints[i].conname != 0; i++)
		freecluster(el_constraints[i].cluster);
	for(i = el_maxtools-1; i >= 0; i--)
		freecluster(el_tools[i].cluster);
	while (db_clusterfree != NOCLUSTER)
	{
		clus = db_clusterfree;
		db_clusterfree = db_clusterfree->nextcluster;
		efree((CHAR *)clus);
	}
	freecluster(db_cluster);
	if (db_memerrio != 0) fclose(db_memerrio);
#  endif
#endif
}

#ifdef DEBUGMEMORY
void db_checkmemoryblock(UCHAR1 *addr)
{
	REGISTER INTBIG *botptr, *endptr;

	botptr = (INTBIG *)addr;
	endptr = (INTBIG *)(*botptr++);
	if (*botptr != 0xC0CEC0CE)
		ttyputerr(_("Error: Memory bottom bound overwritten (allocated from %s, line %ld)"),
			&endptr[2], endptr[1]);
	if (*endptr != 0xC0CEC0CE)
		ttyputerr(_("Error: Memory top bound overwritten (allocated from %s, line %ld)"),
			&endptr[2], endptr[1]);
}

#  ifdef USECLUSTERS
void db_checkcluster(CLUSTER *clus)
{
	REGISTER INTBIG *start, *ptr, head, *endptr, line, errors, size, i;
	CHAR *modulename, dataarray[21], errorfile[300], *dataptr;

	start = ptr = (INTBIG *)clus->address;
	size = clus->clustersize;
	errors = 0;
	if (size != 0)
	{
		for(;;)
		{
			/* get the head of a block in this cluster */
			head = *ptr;

			/* if block is not free, skip it */
			if ((head&FLAGBITS) != FREE)
			{
				/* error: */
				if ((head&FLAGBITS) == USED)
				{
					endptr = (INTBIG *)ptr[1];
					if (db_memerrio == 0)
					{
						estrcpy(errorfile, db_initialdirectory);
						estrcat(errorfile, x_("MemoryErrors.txt"));
						db_memerrio = efopen(errorfile, x_("w"));
					}
					if (errors == 0)
						efprintf(db_memerrio, _("***Errors from cluster %s:\n"), clus->clustername);
					errors++;
					modulename = (CHAR *)&endptr[2];
					line = endptr[1];
					dataptr = (CHAR *)&ptr[3];
					for(i=0; i<20; i++) if (dataptr[i] < ' ' || dataptr[i] >= 0177) dataarray[i] = ' '; else
						dataarray[i] = dataptr[i];
					dataarray[20] = 0;
					efprintf(db_memerrio, _("  Block allocated from %s, line %ld ('%s': 0%o 0%o 0%o 0%o)\n"), modulename, line,
						dataarray, dataptr[0]&0xFF, dataptr[1]&0xFF, dataptr[2]&0xFF, dataptr[3]&0xFF);
				}

				ptr = (INTBIG *)(head & ~FLAGBITS);
				if (ptr == start) break;
				continue;
			}

			/* go on to the next one */
			ptr = (INTBIG *)head;
			if (ptr == start) break;
		}
	}
}
#  endif
#endif

#ifdef USECLUSTERS
/*
 * internal routine to allocate "size" blocks of memory for cluster "clus" and
 * place links.  If "insert" is nonzero, this is the address in the list to
 * insert this additional memory.  Otherwise, this is the first allocation and
 * should be placed in the cluster's "address" field.  Returns the address of
 * this new memory (zero if error).
 */
UCHAR1 *db_getmorememory(CLUSTER *clus, INTBIG size, INTBIG insert)
{
	REGISTER INTBIG *ptr, lastword, *pt;
	REGISTER UCHAR1 *addr;

	/* allocate the pages of memory */
	addr = db_getpages(size);
	if (addr == 0)
	{
		error(_("Attempt to extend %s cluster for a %ld byte block failed"),
			clus->clustername, size);
		return(0);
	}
	clus->clustersize += size;

	/* put the linked list pointers in this cluster */
	lastword = db_pagesize / SIZEOFINTBIG * size - 1;
	ptr = (INTBIG *)addr;
	ptr[0] = (INTBIG)&ptr[lastword];
	if (insert == 0)
	{
		/* first allocation: close the address loop and store it in cluster */
		clus->address = (INTBIG)addr;
		ptr[lastword] = (INTBIG)addr | SKIP;
	} else
	{
		/* additional allocation: insert at "insert" */
		pt = (INTBIG *)insert;
		ptr[lastword] = *pt | SKIP;
		*pt = (INTBIG)addr | SKIP;
	}

	return(addr);
}

/*
 * routine to get "size" page-aligned pages of memory and return the address.
 * Returns 0 if there is an allocation error.  This routine was modified by
 * J.P. Polonovski to be more robust when memory is low.
 */
UCHAR1 *db_getpages(INTBIG size)
{
	REGISTER UCHAR1 *addr;
	REGISTER INTBIG i, j, newcount, smallest, amount;
	REGISTER INTBIG pass;
	static INTBIG old_status = 1, old_size = 0;

	/* look for spare pages in the table */
	for(i=0; i<db_sparepagecount; i++) if (db_sparepages[i].pagecount >= size)
	{
		addr = db_sparepages[i].pageaddress;
		if (db_sparepages[i].pagecount > size)
		{
			db_sparepages[i].pageaddress += size * db_pagesize;
			db_sparepages[i].pagecount -= size;
		} else
		{
			db_sparepagecount--;
			for(j=i; j<db_sparepagecount; j++)
			{
				db_sparepages[j].pagecount = db_sparepages[j+1].pagecount;
				db_sparepages[j].pageaddress = db_sparepages[j+1].pageaddress;
			}
		}
		old_status = (INTBIG)addr;
		old_size = size;
		return(addr);
	}

	/* determine how many blocks to allocate */
	pass = 0;
	amount = PAGEBLOCK;
	if (size > amount) amount = size;

	/* special modification if last request failed */
	if (old_status == 0)
	{
		if (size >= old_size || old_size == 1)
		{
			if (old_size != 1)
				ttyputerr(_("Not able to allocate %ld byte from memory"), size * db_pagesize); else
					ttyputerr(x_("SAVE QUICKLY THERE IS NO MORE MEMORY!!"));
			return(0);
		} else amount = size / 2;
	}

	addr = (UCHAR1 *)valloc((unsigned)(amount*db_pagesize));
	if (addr == 0)
	{
		if (old_status == 0 && old_size >= size) return(0);

		for(;;)
		{
			if (pass == 0) ttyputerr(_("!! Memory Low !! Save soon !!"));
			pass++;
			amount /= 2;
			if (size > amount)
			{
				amount = size;
				addr = (UCHAR1 *)valloc((unsigned) (amount * db_pagesize));
				if (addr == 0)
				{
					ttyputerr(_("!! No memory left !!"));
					old_size = amount;
					old_status = 0;
					return(0);
				}
				break;
			}
			addr = (UCHAR1 *)valloc((unsigned) (amount * db_pagesize));
			if (addr == 0) continue; else
			{
				ttyputerr(_("!! Only %ld bytes of memory left !!"), amount * db_pagesize);
				break;
			}
		}
	}

	/* if there is excess, save it in the spare page table */
	if (size < amount)
	{
		if (db_sparepagecount >= SPAREPAGELIMIT)
		{
			/* no room in table, look for smallest entry and overwrite it */
			smallest = db_sparepages[newcount = 0].pagecount + 1;
			for(i=1; i<db_sparepagecount; i++)
				if (db_sparepages[i].pagecount < smallest)
					smallest = db_sparepages[newcount = i].pagecount;
			ttyputmsg(_("No room for spare page memory, deleting %ld blocks"), smallest);
		} else
		{
			/* take a new entry in the table */
			newcount = db_sparepagecount;
			db_sparepagecount++;
		}

		/* load the page table with the excess */
		db_sparepages[newcount].pagecount = amount - size;
		db_sparepages[newcount].pageaddress = addr + size * db_pagesize;
	}
	old_status = (INTBIG)addr;
	old_size = amount;
	return(addr);
}
#endif

void db_printclusterarena(CHAR *which)
{
#ifdef USECLUSTERS
	REGISTER CLUSTER *clus;
	REGISTER INTBIG i;
	INTBIG freesize, usedsize, blocksused;

	if (*which == 0)
	{
		freesize = usedsize = blocksused = 0;
		for(clus=db_firstcluster; clus!=NOCLUSTER; clus=clus->nextcluster)
			db_printoneclusterarena(clus, FALSE, &freesize, &usedsize, &blocksused);
		ttyputmsg(_("%4d blocks%8d used%8d free    TOTAL"), blocksused, usedsize, freesize);

		/* print spare pages summary */
		if (db_sparepagecount == 0) ttyputmsg(_("NO spare pages")); else
			for(i=0; i<db_sparepagecount; i++)
				ttyputmsg(_("%ld spare pages in location %ld"), db_sparepages[i].pagecount, i);
	} else
	{
		for(clus=db_firstcluster; clus!=NOCLUSTER; clus=clus->nextcluster)
			if (namesame(which, clus->clustername) == 0)
		{
			db_coalescecluster(clus);
			db_printoneclusterarena(clus, TRUE, &freesize, &usedsize, &blocksused);
			return;
		}
		ttyputmsg(_("No cluster called %s"), which);
	}
#else
	Q_UNUSED( which );
	ttyputmsg(_("Cluster option is not in use"));
#endif
}

#ifdef	USECLUSTERS
/*
 * routine to cycle through memory cluster "cluster" and accumulate the number
 * of free bytes in "freesize", used bytes in "usedsize" and total virtual
 * pages used in "blocksused".   If "verbose" is positive, print everything
 * in the cluster; if "verbose" is negative print nothing; if "verbose" is
 * zero, print a summary only
 */
void db_printoneclusterarena(CLUSTER *cluster, INTBIG verbose, INTBIG *freesize,
	INTBIG *usedsize, INTBIG *blocksused)
{
	REGISTER INTBIG addr, *ptr, value, *start, freethis, usedthis;

	freethis = usedthis = 0;
	start = ptr = (INTBIG *)cluster->address;
	if (cluster->clustersize != 0) for(;;)
	{
		value = *ptr;
		addr = value & ~FLAGBITS;
		switch (value & FLAGBITS)
		{
			case FREE:
				if (verbose > 0) ttyputmsg(x_("  Free block at 0%o is %ld long"),
					ptr, addr-(INTBIG)ptr-SIZEOFINTBIG);
				freethis += addr - (INTBIG)ptr - SIZEOFINTBIG;
				break;
			case USED:
				if (verbose > 0) ttyputmsg(x_("  Used block at 0%o is %ld long"),
					ptr, addr-(INTBIG)ptr-SIZEOFINTBIG);
				usedthis += addr - (INTBIG)ptr - SIZEOFINTBIG;
				break;
			case SKIP:
				if (verbose > 0) ttyputmsg(x_("  Skip block at 0%o"), ptr);
				break;
		}
		ptr = (INTBIG *)addr;
		if (ptr == start) break;
	}
	if (verbose >= 0)
		ttyputmsg(x_("%4d blocks, %6d used, %6d free in cluster %s"),
			cluster->clustersize, usedthis, freethis, cluster->clustername);
	*freesize += freethis;   *usedsize += usedthis;
	*blocksused += cluster->clustersize;
}

/* coalesce a cluster */
void db_coalescecluster(CLUSTER *cluster)
{
	REGISTER INTBIG *ptr, head, *nextptr, *start;

	if (cluster->clustersize == 0) return;
	start = ptr = (INTBIG *)cluster->address;
	for(;;)
	{
		/* get the head of a block in this cluster */
		head = *ptr;

		/* if block is not free, skip it */
		if ((head&FLAGBITS) != FREE)
		{
			ptr = (INTBIG *)(head & ~FLAGBITS);
			if (ptr == start) break;
			continue;
		}

		/* coalesce this and any subsequent free blocks */
		for(;;)
		{
			nextptr = (INTBIG *)head;

			/* stop coalescing if the next block is not free */
			if ((*nextptr & FLAGBITS) != FREE) break;

			/* if the coalesced block is the start, update start pointer */
			if (nextptr == start)
			{
				cluster->address = *nextptr;
				start = (INTBIG *)*nextptr;
			}
			head = *nextptr;
		}
		*ptr = head;

		/* go on to the next one */
		ptr = (INTBIG *)head;
		if (ptr == start) break;
	}
}

#ifndef HAVE_GETPAGESIZE

INTBIG getpagesize(void)
{
#  ifdef MACOS
	return(4096);
#  endif

#  ifdef WIN32
	SYSTEM_INFO si;

	GetSystemInfo(&si);
	return(si.dwPageSize);
#  endif

#  ifdef ONUNIX
#    ifdef NBPG
	/* General page-size constant SHOULD be defined for all hp systems */
	return(NBPG);
#    else
#      ifdef NBPG_M320
	/* try this, works for bobcat 320. */
	return(NBPG_M320);
#      else
	/* default to 4K */
	return(4096);
#      endif /* NBPG_M320 */
#    endif /* NBPG */
#  endif /* ONUNIX */
}
#endif	/* HAVE_GETPAGESIZE */

#ifndef HAVE_VALLOC
/*
 * An allocation routine which wastes the least amount of space possible
 * while returning memory page-aligned if there is a page or more
 * of memory to allocate.  Page size is in the static INTBIG db_pagesize.
 * Can waste up to db_pagesize - 1.
 * Space allocated with valloc is generally not free'd.
 */
UCHAR1 *valloc(INTBIG nbytes)
{
#  if defined(MACOS) && !defined(MACOSX)
	UCHAR1 *ptr;
	INTBIG left, dummy;

	left = MaxMem((Size *)&dummy);
	if (left-MUSTALLOW >= nbytes) ptr = (UCHAR1 *)malloc(nbytes); else ptr = 0;
	return(ptr);
#  else
	UCHAR1 *ptr;
	INTBIG page;

	page = getpagesize();
	ptr = malloc(nbytes + page - 1);
	if (ptr == 0) return(0);
	return(UCHAR1 *)(((INTBIG)ptr + page -1) & ~(page-1));
#  endif
}
#endif	/* HAVE_VALLOC */

#endif	/* USECLUSTERS */
