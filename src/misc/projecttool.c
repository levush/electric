/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: projecttool.c
 * Project management tool
 * Written by: Steven M. Rubin
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
#if PROJECTTOOL

#include "global.h"
#include "projecttool.h"
#include "database.h"
#include "edialogs.h"
#include "usr.h"
#include "usrdiacom.h"
#include "tecgen.h"

/***** command parsing *****/

static COMCOMP projcop = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to be checked-out"), M_("check-out current cell")};
static COMCOMP projcip = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to be checked-in"), M_("check-in current cell")};
static COMCOMP projoldp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell from which old version should be retrieved"),
		M_("use current cell")};
static COMCOMP projap = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to be added"), M_("add current cell")};
static COMCOMP projdp = {NOKEYWORD, topofcells, nextcells, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("cell to be deleted"), M_("delete current cell")};
static KEYWORD projopt[] =
{
	{x_("build-project"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-out"),            1,{&projcop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("check-in"),             1,{&projcip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("get-old-version"),      1,{&projoldp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("add-cell"),             1,{&projap,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("delete-cell"),          1,{&projdp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("update"),               0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("set-user"),             0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("list-cells"),           0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP proj_projp = {projopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Project management tool action"), x_("")};

/***** cell checking queue *****/

#define NOFCHECK ((FCHECK *)-1)

typedef struct Ifcheck
{
	NODEPROTO      *entry;
	INTBIG          batchnumber;
	struct Ifcheck *nextfcheck;
} FCHECK;

static FCHECK *proj_firstfcheck = NOFCHECK;
static FCHECK *proj_fcheckfree = NOFCHECK;

/***** user database information *****/

#define NOPUSER ((PUSER *)-1)
#define PUSERFILE x_("projectusers")

typedef struct Ipuser
{
	CHAR *username;
	CHAR *userpassword;
	struct Ipuser *nextpuser;
} PUSER;

static PUSER *proj_users = NOPUSER;

/***** project file information *****/

#define NOPROJECTCELL ((PROJECTCELL *)-1)

typedef struct Iprojectcell
{
	CHAR  *libname;						/* name of the library */
	CHAR  *cellname;					/* name of the cell */
	VIEW  *cellview;					/* cell view */
	INTBIG cellversion;					/* cell version */
	CHAR  *owner;						/* current owner of this cell (if checked out) */
	CHAR  *lastowner;					/* previous owner of this cell (if checked in) */
	CHAR  *comment;						/* comments for this cell */
	struct Iprojectcell *nextprojectcell;
} PROJECTCELL;

static PROJECTCELL *proj_firstprojectcell = NOPROJECTCELL;
static PROJECTCELL *proj_projectcellfree = NOPROJECTCELL;

/***** miscellaneous *****/

static TOOL         *proj_tool;					/* this tool */
static CHAR          proj_username[256];		/* current user's name */
static INTBIG        proj_lockedkey;			/* key for "PROJ_locked" */
static INTBIG        proj_pathkey;				/* key for "PROJ_path" */
static INTBIG        proj_userkey;				/* key for "PROJ_user" */
static BOOLEAN       proj_active;				/* nonzero if the system is active */
static BOOLEAN       proj_ignorechanges;		/* nonzero to ignore broadcast changes */
static TOOL         *proj_source;				/* source of changes */
static INTBIG        proj_batchnumber;			/* ID number of this batch of changes */
static PUSER        *proj_userpos;				/* current user for dialog display */
static FILE         *proj_io;					/* channel to project file */
static INTBIG       *proj_savetoolstate = 0;	/* saved tool state information */
static INTBIG        proj_filetypeproj;			/* Project disk file descriptor */
static INTBIG        proj_filetypeprojusers;	/* Project users disk file descriptor */

/* prototypes for local routines */
static void          proj_addcell(CHAR *cellname);
static FCHECK       *proj_allocfcheck(void);
static PROJECTCELL  *proj_allocprojectcell(void);
static void          proj_buildproject(LIBRARY *lib);
static void          proj_checkin(CHAR *cellname);
static void          proj_checkinmany(LIBRARY *lib);
static void          proj_checkout(CHAR *cellname, BOOLEAN showcell);
static void          proj_deletecell(CHAR *cellname);
static void          proj_endwritingprojectfile(void);
static PROJECTCELL  *proj_findcell(NODEPROTO *np);
static void          proj_freefcheck(FCHECK *f);
static void          proj_freeprojectcell(PROJECTCELL *pf);
static BOOLEAN       proj_getcomments(PROJECTCELL *pf, CHAR *direction);
static NODEPROTO    *proj_getcell(PROJECTCELL *pf, LIBRARY *lib);
static void          proj_getoldversion(CHAR *cellname);
static BOOLEAN       proj_getprojinfo(LIBRARY *lib, CHAR *path, CHAR *projfile);
static BOOLEAN       proj_getusername(BOOLEAN newone, LIBRARY *lib);
static BOOLEAN       proj_initusers(CHAR **c);
static BOOLEAN       proj_loadlist(LIBRARY *lib, void *dia);
static BOOLEAN       proj_lockprojfile(CHAR *projectpath, CHAR *projectfile);
static void          proj_marklocked(NODEPROTO *np, BOOLEAN locked);
static CHAR         *proj_nextuser(void);
static void          proj_queuecheck(NODEPROTO *cell);
static BOOLEAN       proj_readprojectfile(CHAR *pathname, CHAR *filename);
static void          proj_restoretoolstate(void);
static void          proj_showlistdialog(LIBRARY *lib);
static BOOLEAN       proj_startwritingprojectfile(CHAR *pathname, CHAR *filename);
static CHAR         *proj_templibraryname(void);
static BOOLEAN       proj_turnofftools(void);
static void          proj_unlockprojfile(CHAR *projectpath, CHAR *projectfile);
static void          proj_update(LIBRARY *lib);
static BOOLEAN       proj_usenewestversion(NODEPROTO *oldnp, NODEPROTO *newnp);
static void          proj_validatelocks(LIBRARY *lib);
static CHAR         *proj_wantpassword(INTBIG mode, CHAR *username);
static BOOLEAN       proj_writecell(NODEPROTO *np);

/* prototypes for local routines that should be in the system */
static NODEPROTO    *copyrecursively(NODEPROTO *fromnp, LIBRARY *tolib);
static NODEPROTO    *copyskeleton(NODEPROTO *fromnp, LIBRARY *tolib);

/************************ CONTROL ***********************/

void proj_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	/* ignore pass 3 initialization */
	if (thistool == 0) return;

	/* miscellaneous initialization during pass 2 */
	if (thistool == NOTOOL)
	{
		proj_lockedkey = makekey(x_("PROJ_locked"));
		proj_pathkey = makekey(x_("PROJ_path"));
		proj_userkey = makekey(x_("PROJ_user"));
		proj_username[0] = 0;
		proj_active = FALSE;
		proj_ignorechanges = FALSE;
		proj_filetypeproj = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("proj"), _("Project"));
		proj_filetypeprojusers = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("projusers"), _("Project Users"));
		return;
	}

	/* copy tool pointer during pass 1 */
	proj_tool = thistool;
}

void proj_done(void)
{
#ifdef DEBUGMEMORY
	REGISTER FCHECK *f;
	REGISTER PROJECTCELL *pf;
	REGISTER PUSER *pu;

	while (proj_users != NOPUSER)
	{
		pu = proj_users;
		proj_users = proj_users->nextpuser;
		efree((CHAR *)pu->username);
		efree((CHAR *)pu->userpassword);
		efree((CHAR *)pu);
	}

	while (proj_firstprojectcell != NOPROJECTCELL)
	{
		pf = proj_firstprojectcell;
		proj_firstprojectcell = proj_firstprojectcell->nextprojectcell;
		efree(pf->libname);
		efree(pf->cellname);
		efree(pf->owner);
		efree(pf->lastowner);
		efree(pf->comment);
		proj_freeprojectcell(pf);
	}
	while (proj_projectcellfree != NOPROJECTCELL)
	{
		pf = proj_projectcellfree;
		proj_projectcellfree = proj_projectcellfree->nextprojectcell;
		efree((CHAR *)pf);
	}

	while (proj_firstfcheck != NOFCHECK)
	{
		f = proj_firstfcheck;
		proj_firstfcheck = proj_firstfcheck->nextfcheck;
		proj_freefcheck(f);
	}
	while (proj_fcheckfree != NOFCHECK)
	{
		f = proj_fcheckfree;
		proj_fcheckfree = proj_fcheckfree->nextfcheck;
		efree((CHAR *)f);
	}
	if (proj_savetoolstate != 0)
		efree((CHAR *)proj_savetoolstate);
#endif
}

void proj_slice(void)
{
	REGISTER FCHECK *f, *nextf;
	REGISTER NODEPROTO *np;
	TOOL *tool;
	REGISTER INTBIG lowbatch, highbatch, retval, undonecells;
	REGISTER void *infstr;

	if (!proj_active) return;
	if (proj_firstfcheck == NOFCHECK) return;

	undonecells = 0;
	for(f = proj_firstfcheck; f != NOFCHECK; f = nextf)
	{
		nextf = f->nextfcheck;
		np = f->entry;

		/* make sure cell np is checked-out */
		if (getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, proj_lockedkey) != NOVARIABLE)
		{
			if (undonecells == 0)
			{
				infstr = initinfstr();
				lowbatch = highbatch = f->batchnumber;
			} else
			{
				if (f->batchnumber < lowbatch) lowbatch = f->batchnumber;
				if (f->batchnumber > highbatch) highbatch = f->batchnumber;
				addstringtoinfstr(infstr, x_(", "));
			}
			addstringtoinfstr(infstr, describenodeproto(np));
			undonecells++;
		}
		proj_freefcheck(f);
	}
	proj_firstfcheck = NOFCHECK;
	if (undonecells > 0)
	{
		ttyputerr(_("Cannot change unchecked-out %s: %s"),
			makeplural(_("cell"), undonecells),
			returninfstr(infstr));
		proj_ignorechanges = TRUE;
		for(;;)
		{
			retval = undoabatch(&tool);
			if (retval == 0) break;
			if (retval < 0) retval = -retval;
			if (retval <= lowbatch) break;
		}
		noredoallowed();
		proj_ignorechanges = FALSE;
	}
}

void proj_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *pp, *cellname;
	REGISTER NODEPROTO *np;

	if (count <= 0)
	{
		ttyputerr(_("Missing command to display tool"));
		return;
	}
	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("add-cell"), l) == 0)
	{
		if (count >= 2) cellname = par[1]; else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to add"));
				return;
			}
			cellname = describenodeproto(np);
		}
		proj_addcell(cellname);
		proj_active = TRUE;
		return;
	}
	if (namesamen(pp, x_("build-project"), l) == 0)
	{
		if (el_curlib == NOLIBRARY)
		{
			ttyputerr(_("No current library to enter"));
			return;
		}
		proj_buildproject(el_curlib);
		proj_active = TRUE;
		return;
	}
	if (namesamen(pp, x_("check-in"), l) == 0 && l >= 7)
	{
		if (count >= 2) cellname = par[1]; else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to check in"));
				return;
			}
			cellname = describenodeproto(np);
		}
		proj_checkin(cellname);
		proj_active = TRUE;
		return;
	}
	if (namesamen(pp, x_("check-out"), l) == 0 && l >= 7)
	{
		if (count >= 2) cellname = par[1]; else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to check out"));
				return;
			}
			cellname = describenodeproto(np);
		}
		proj_checkout(cellname, TRUE);
		proj_active = TRUE;
		return;
	}
	if (namesamen(pp, x_("delete-cell"), l) == 0)
	{
		if (count >= 2) cellname = par[1]; else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to delete"));
				return;
			}
			cellname = describenodeproto(np);
		}
		proj_deletecell(cellname);
		proj_active = TRUE;
		return;
	}
	if (namesamen(pp, x_("get-old-version"), l) == 0)
	{
		if (count >= 2) cellname = par[1]; else
		{
			np = getcurcell();
			if (np == NONODEPROTO)
			{
				ttyputerr(_("No current cell to retrieve old versions"));
				return;
			}
			cellname = describenodeproto(np);
		}
		proj_getoldversion(cellname);
		proj_active = TRUE;
		return;
	}
	if (namesamen(pp, x_("list-cells"), l) == 0)
	{
		proj_showlistdialog(el_curlib);
		return;
	}
	if (namesamen(pp, x_("set-user"), l) == 0)
	{
		(void)proj_getusername(TRUE, el_curlib);
		return;
	}
	if (namesamen(pp, x_("update"), l) == 0)
	{
		if (el_curlib == NOLIBRARY)
		{
			ttyputerr(_("No current library to update"));
			return;
		}
		proj_update(el_curlib);
		proj_active = TRUE;
		return;
	}
}

void proj_startbatch(TOOL *source, BOOLEAN undoredo)
{
	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (proj_ignorechanges) proj_source = proj_tool; else
		proj_source = source;
	proj_batchnumber = getcurrentbatchnumber();
}

void proj_modifynodeinst(NODEINST *ni, INTBIG olx, INTBIG oly, INTBIG ohx, INTBIG ohy,
	INTBIG orot, INTBIG otran)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	proj_queuecheck(ni->parent);
}

void proj_modifyarcinst(ARCINST *ai, INTBIG oxA, INTBIG oyA, INTBIG oxB, INTBIG oyB,
	INTBIG owid, INTBIG olen)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	proj_queuecheck(ai->parent);
}

void proj_modifyportproto(PORTPROTO *pp, NODEINST *oni, PORTPROTO *opp)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	proj_queuecheck(pp->parent);
}

void proj_modifydescript(INTBIG addr, INTBIG type, INTBIG key, UINTBIG *old)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
	}
}

void proj_newobject(INTBIG addr, INTBIG type)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
	}
}

void proj_killobject(INTBIG addr, INTBIG type)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
	}
}

void proj_newvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG newtype)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
		case VNODEPROTO:
			if (key != proj_lockedkey) proj_queuecheck((NODEPROTO *)addr);
			break;
	}
}

void proj_killvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG oldaddr, INTBIG oldtype,
	UINTBIG *olddescript)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
		case VNODEPROTO:
			if (key != proj_lockedkey) proj_queuecheck((NODEPROTO *)addr);
			break;
	}
}

void proj_modifyvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype,
	INTBIG aindex, INTBIG oldvalue)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
		case VNODEPROTO: proj_queuecheck((NODEPROTO *)addr);            break;
	}
}

void proj_insertvariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype, INTBIG index)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
		case VNODEPROTO: proj_queuecheck((NODEPROTO *)addr);            break;
	}
}

void proj_deletevariable(INTBIG addr, INTBIG type, INTBIG key, INTBIG vartype, INTBIG index,
	INTBIG oldvalue)
{
	if (proj_ignorechanges || proj_source == proj_tool) return;
	switch (type&VTYPE)
	{
		case VNODEINST:  proj_queuecheck(((NODEINST *)addr)->parent);   break;
		case VARCINST:   proj_queuecheck(((ARCINST *)addr)->parent);    break;
		case VPORTPROTO: proj_queuecheck(((PORTPROTO *)addr)->parent);  break;
		case VNODEPROTO: proj_queuecheck((NODEPROTO *)addr);            break;
	}
}

void proj_readlibrary(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;

	/* scan the library to see if any cells are locked */
	if (proj_ignorechanges) return;
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (getvalkey((INTBIG)np, VNODEPROTO, VINTEGER, proj_lockedkey) != NOVARIABLE)
			proj_active = TRUE;
	}
}

/************************ PROJECT MANAGEMENT ***********************/

void proj_checkin(CHAR *cellname)
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER NODEINST *ni;
	REGISTER INTBIG total;
	REGISTER BOOLEAN propagated;
	REGISTER LIBRARY *olib;
	REGISTER PROJECTCELL *pf;
	REGISTER void *infstr;

	/* find out which cell is being checked in */
	np = getnodeproto(cellname);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot identify cell '%s'"), cellname);
		return;
	}

	/* mark the cell to be checked-in */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp1 = 0;
	np->temp1 = 1;

	/* look for cells above this one that must also be checked in */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp2 = 0;
	np->temp2 = 1;
	propagated = TRUE;
	while (propagated)
	{
		propagated = FALSE;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (onp->temp2 == 1)
			{
				propagated = TRUE;
				onp->temp2 = 2;
				for(ni = onp->firstinst; ni != NONODEINST; ni = ni->nextinst)
				{
					if (ni->parent->temp2 == 0) ni->parent->temp2 = 1;
				}
			}
		}
	}
	np->temp2 = 0;
	total = 0;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (onp->temp2 == 0) continue;
		pf = proj_findcell(onp);
		if (pf == NOPROJECTCELL) continue;
		if (namesame(pf->owner, proj_username) == 0)
		{
			onp->temp1 = 1;
			total++;
		}
	}

	/* look for cells below this one that must also be checked in */
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			onp->temp2 = 0;
	np->temp2 = 1;
	propagated = TRUE;
	while (propagated)
	{
		propagated = FALSE;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (onp->temp2 == 1)
			{
				propagated = TRUE;
				onp->temp2 = 2;
				for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					if (ni->proto->primindex != 0) continue;
					if (ni->proto->temp2 == 0) ni->proto->temp2 = 1;
				}
			}
		}
	}
	np->temp2 = 0;
	for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
		for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (onp->temp2 == 0) continue;
		pf = proj_findcell(onp);
		if (pf == NOPROJECTCELL) continue;
		if (namesame(pf->owner, proj_username) == 0)
		{
			onp->temp1 = 1;
			total++;
		}
	}

	/* advise of additional cells that must be checked-in */
	if (total > 0)
	{
		total = 0;
		infstr = initinfstr();
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
		{
			if (onp == np || onp->temp1 == 0) continue;
			if (total > 0) addstringtoinfstr(infstr, x_(", "));
			addstringtoinfstr(infstr, describenodeproto(onp));
			total++;
		}
		ttyputmsg(_("Also checking in related cell(s): %s"), returninfstr(infstr));
	}

	/* check it in */
	proj_checkinmany(np->lib);
}

void proj_checkout(CHAR *cellname, BOOLEAN showcell)
{
	REGISTER NODEPROTO *np, *newvers, *oldvers;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN worked, propagated;
	REGISTER INTBIG total;
	REGISTER LIBRARY *lib, *olib;
	CHAR projectpath[256], projectfile[256], *argv[3];
	PROJECTCELL *pf;

	/* find out which cell is being checked out */
	oldvers = getnodeproto(cellname);
	if (oldvers == NONODEPROTO)
	{
		ttyputerr(_("Cannot identify cell '%s'"), cellname);
		return;
	}
	lib = oldvers->lib;

	/* make sure there is a valid user name */
	if (proj_getusername(FALSE, lib))
	{
		ttyputerr(_("No valid user"));
		return;
	}

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return;
	}

	/* lock the project file */
	if (proj_lockprojfile(projectpath, projectfile))
	{
		ttyputerr(_("Couldn't lock project file"));
		return;
	}

	/* read the project file */
	worked = FALSE;
	if (proj_readprojectfile(projectpath, projectfile))
		ttyputerr(_("Cannot read project file")); else
	{
		/* find this in the project file */
		pf = proj_findcell(oldvers);
		if (pf == NOPROJECTCELL) ttyputerr(_("This cell is not in the project")); else
		{
			/* see if it is available */
			if (*pf->owner != 0)
			{
				if (namesame(pf->owner, proj_username) == 0)
				{
					ttyputerr(_("This cell is already checked out to you"));
					proj_marklocked(oldvers, FALSE);
				} else
				{
					ttyputerr(_("This cell is already checked out to '%s'"), pf->owner);
				}
			} else
			{
				/* make sure we have the latest version */
				if (pf->cellversion > oldvers->version)
				{
					ttyputerr(_("Cannot check out %s because you don't have the latest version (yours is %ld, project has %ld)"),
						describenodeproto(oldvers), oldvers->version, pf->cellversion);
					ttyputmsg(_("Do an 'update' first"));
				} else
				{
					if (!proj_getcomments(pf, x_("out")))
					{
						if (proj_startwritingprojectfile(projectpath, projectfile))
							ttyputerr(_("Cannot write project file")); else
						{
							/* prevent tools (including this one) from seeing the change */
							(void)proj_turnofftools();

							/* remove highlighting */
							us_clearhighlightcount();

							/* make new version */
							newvers = copynodeproto(oldvers, lib, oldvers->protoname, TRUE);

							/* restore tool state */
							proj_restoretoolstate();

							if (newvers == NONODEPROTO)
								ttyputerr(_("Error making new version of cell")); else
							{
								(*el_curconstraint->solve)(newvers);

								/* replace former usage with new version */
								if (proj_usenewestversion(oldvers, newvers))
									ttyputerr(_("Error replacing instances of new %s"),
										describenodeproto(oldvers)); else
								{
									/* update record for the cell */
									(void)reallocstring(&pf->owner, proj_username, proj_tool->cluster);
									(void)reallocstring(&pf->lastowner, x_(""), proj_tool->cluster);
									proj_marklocked(newvers, FALSE);
									worked = TRUE;
								}
							}
						}
						proj_endwritingprojectfile();
					}
				}
			}
		}
	}

	/* relase project file lock */
	proj_unlockprojfile(projectpath, projectfile);

	/* if it worked, print dependencies and display */
	if (worked)
	{
		ttyputmsg(_("Cell %s checked out for your use"), describenodeproto(newvers));

		/* advise of possible problems with other checkouts higher up in the hierarchy */
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->temp1 = 0;
		newvers->temp1 = 1;
		propagated = TRUE;
		while (propagated)
		{
			propagated = FALSE;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 == 1)
				{
					propagated = TRUE;
					np->temp1 = 2;
					for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
						if (ni->parent->temp1 == 0) ni->parent->temp1 = 1;
				}
			}
		}
		newvers->temp1 = 0;
		total = 0;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 == 0) continue;
			pf = proj_findcell(np);
			if (pf == NOPROJECTCELL) continue;
			if (*pf->owner != 0 && namesame(pf->owner, proj_username) != 0)
			{
				np->temp1 = 3;
				np->temp2 = (INTBIG)pf;
				total++;
			}
		}
		if (total != 0)
		{
			ttyputmsg(_("*** Warning: the following cells are above this in the hierarchy"));
			ttyputmsg(_("*** and are checked out to others.  This may cause problems"));
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 != 3) continue;
				pf = (PROJECTCELL *)np->temp2;
				ttyputmsg(_("    %s is checked out to %s"), describenodeproto(np), pf->owner);
			}
		}

		/* advise of possible problems with other checkouts lower down in the hierarchy */
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
				np->temp1 = 0;
		newvers->temp1 = 1;
		propagated = TRUE;
		while(propagated)
		{
			propagated = FALSE;
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 == 1)
				{
					propagated = TRUE;
					np->temp1 = 2;
					for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
					{
						if (ni->proto->primindex != 0) continue;
						if (ni->proto->temp1 == 0) ni->proto->temp1 = 1;
					}
				}
			}
		}
		newvers->temp1 = 0;
		total = 0;
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 == 0) continue;
			pf = proj_findcell(np);
			if (pf == NOPROJECTCELL) continue;
			if (*pf->owner != 0 && namesame(pf->owner, proj_username) != 0)
			{
				np->temp1 = 3;
				np->temp2 = (INTBIG)pf;
				total++;
			}
		}
		if (total != 0)
		{
			ttyputmsg(_("*** Warning: the following cells are below this in the hierarchy"));
			ttyputmsg(_("*** and are checked out to others.  This may cause problems"));
			for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
				for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			{
				if (np->temp1 != 3) continue;
				pf = (PROJECTCELL *)np->temp2;
				ttyputmsg(_("    %s is checked out to %s"), describenodeproto(np), pf->owner);
			}
		}

		/* display the checked-out cell */
		if (showcell)
		{
			argv[0] = describenodeproto(newvers);
			us_editcell(1, argv);
			us_endchanges(NOWINDOWPART);
		}
	}
}

/* Project Old Version */
static DIALOGITEM proj_oldversdialogitems[] =
{
 /*  1 */ {0, {176,224,200,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {176,16,200,80}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,8,20,140}, MESSAGE, N_("Old version of cell")},
 /*  4 */ {0, {28,12,166,312}, SCROLL, x_("")},
 /*  5 */ {0, {4,140,20,312}, MESSAGE, x_("")}
};
static DIALOG proj_oldversdialog = {{108,75,318,396}, N_("Get Old Version of Cell"), 0, 5, proj_oldversdialogitems, 0, 0};

/* special items for the "get old version" dialog: */
#define DPRV_CELLLIST  4		/* Cell list (scroll) */
#define DPRV_CELLNAME  5		/* Cell name (stat text) */

void proj_getoldversion(CHAR *cellname)
{
	PROJECTCELL *pf, statpf;
	REGISTER INTBIG version;
	REGISTER time_t date;
	REGISTER INTBIG itemHit, count, i, len;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	CHAR line[256], projectpath[256], projectfile[256], **filelist, sep[2], *pt;
	REGISTER void *dia;

	/* find out which cell is being checked out */
	np = getnodeproto(cellname);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot identify cell '%s'"), cellname);
		return;
	}
	lib = np->lib;

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return;
	}

	if (proj_readprojectfile(projectpath, projectfile))
	{
		ttyputerr(_("Cannot read project file"));
		return;
	}

	pf = proj_findcell(np);
	if (pf == NOPROJECTCELL)
	{
		ttyputerr(_("Cell %s is not in the project"), describenodeproto(np));
		return;
	}

	/* find all files in the directory for this cell */
	projectfile[estrlen(projectfile)-5] = 0;
	estrcat(projectpath, projectfile);
	estrcat(projectpath, DIRSEPSTR);
	estrcat(projectpath, pf->cellname);
	estrcat(projectpath, DIRSEPSTR);
	count = filesindirectory(projectpath, &filelist);

	dia = DiaInitDialog(&proj_oldversdialog);
	if (dia == 0) return;
	DiaSetText(dia, DPRV_CELLNAME, cellname);
	DiaInitTextDialog(dia, DPRV_CELLLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1,
		SCSELMOUSE);
	for(i=0; i<count; i++)
	{
		estrcpy(line, filelist[i]);
		len = estrlen(line);
		if (estrcmp(&line[len-5], x_(".elib")) != 0) continue;
		line[len-5] = 0;
		version = eatoi(line);
		if (version <= 0) continue;
		if (version >= pf->cellversion) continue;
		for(pt = line; *pt != 0; pt++) if (*pt == '-') break;
		if (*pt != '-') continue;
		if (estrcmp(&pt[1], pf->cellview->viewname) != 0) continue;

		/* file is good, display it */
		estrcpy(projectfile, projectpath);
		estrcat(projectfile, sep);
		estrcat(projectfile, filelist[i]);
		date = filedate(projectfile);
		esnprintf(line, 256, _("Version %ld, %s"), version, timetostring(date));
		DiaStuffLine(dia, DPRV_CELLLIST, line);
	}
	DiaSelectLine(dia, DPRV_CELLLIST, -1);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
	}
	version = -1;
	i = DiaGetCurLine(dia, DPRV_CELLLIST);
	if (i >= 0)
	{
		pt = DiaGetScrollLine(dia, DPRV_CELLLIST, i);
		if (estrlen(pt) > 8) version = eatoi(&pt[8]);
	}
	DiaDoneDialog(dia);
	if (itemHit == CANCEL) return;
	if (version < 0) return;

	/* build a fake record to describe this cell */
	statpf = *pf;
	statpf.cellversion = version;

	np = proj_getcell(&statpf, lib);
	if (np == NONODEPROTO)
		ttyputerr(_("Error retrieving old version of cell"));
	proj_marklocked(np, FALSE);
	(*el_curconstraint->solve)(np);
	ttyputmsg(_("Cell %s is now in this library"), describenodeproto(np));
}

void proj_update(LIBRARY *lib)
{
	CHAR projectpath[256], projectfile[256];
	PROJECTCELL *pf;
	REGISTER INTBIG total;
	REGISTER NODEPROTO *oldnp, *newnp;

	/* make sure there is a valid user name */
	if (proj_getusername(FALSE, lib))
	{
		ttyputerr(_("No valid user"));
		return;
	}

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return;
	}

	/* lock the project file */
	if (proj_lockprojfile(projectpath, projectfile))
	{
		ttyputerr(_("Couldn't lock project file"));
		return;
	}

	/* read the project file */
	us_clearhighlightcount();
	if (proj_readprojectfile(projectpath, projectfile))
		ttyputerr(_("Cannot read project file")); else
	{
		/* check to see which cells are changed/added */
		total = 0;
		for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
		{
			oldnp = db_findnodeprotoname(pf->cellname, pf->cellview, el_curlib);
			if (oldnp != NONODEPROTO && oldnp->version >= pf->cellversion) continue;

			/* this is a new one */
			newnp = proj_getcell(pf, lib);
			if (newnp == NONODEPROTO)
			{
				if (pf->cellview == el_unknownview)
				{
					ttyputerr(_("Error bringing in %s;%ld"), pf->cellname, pf->cellversion);
				} else
				{
					ttyputerr(_("Error bringing in %s{%s};%ld"), pf->cellname, pf->cellview->sviewname,
						pf->cellversion);
				}
			} else
			{
				if (oldnp != NONODEPROTO && proj_usenewestversion(oldnp, newnp))
					ttyputerr(_("Error replacing instances of new %s"), describenodeproto(oldnp)); else
				{
					if (pf->cellview == el_unknownview)
					{
						ttyputmsg(_("Brought in cell %s;%ld"), pf->cellname, pf->cellversion);
					} else
					{
						ttyputmsg(_("Brought in cell %s{%s};%ld"), pf->cellname, pf->cellview->sviewname,
							pf->cellversion);
					}
					total++;
				}
			}
		}
	}

	/* relase project file lock */
	proj_unlockprojfile(projectpath, projectfile);

	/* make sure all cell locks are correct */
	proj_validatelocks(lib);

	/* summarize */
	if (total == 0) ttyputmsg(_("Project is up-to-date")); else
		ttyputmsg(_("Updated %ld %s"), total, makeplural(_("cell"), total));
}

void proj_buildproject(LIBRARY *lib)
{
	CHAR libraryname[256], librarypath[256], newname[256], projfile[256], *pars[2];
	REGISTER INTBIG i, extpos;
	REGISTER NODEPROTO *np;
	FILE *io;

	/* verify that a project is to be built */
	us_noyesdlog(_("Are you sure you want to create a multi-user project from this library?"), pars);
	if (namesame(pars[0], x_("yes")) != 0) return;

	/* get path prefix for cell libraries */
	estrcpy(librarypath, lib->libfile);
	extpos = -1;
	for(i = estrlen(librarypath)-1; i > 0; i--)
	{
		if (librarypath[i] == DIRSEP) break;
		if (librarypath[i] == '.')
		{
			if (extpos < 0) extpos = i;
		}
	}
	if (extpos < 0) estrcat(librarypath, x_("ELIB")); else
		estrcpy(&librarypath[extpos], x_("ELIB"));
	if (librarypath[i] == DIRSEP)
	{
		estrcpy(libraryname, &librarypath[i+1]);
		librarypath[i] = 0;
	} else
	{
		estrcpy(libraryname, librarypath);
		librarypath[0] = 0;
	}

	/* create the top-level directory for this library */
	estrcpy(newname, librarypath);
	estrcat(newname, DIRSEPSTR);
	estrcat(newname, libraryname);
	if (fileexistence(newname) != 0)
	{
		ttyputerr(_("Project directory '%s' already exists"), newname);
		return;
	}
	if (createdirectory(newname))
	{
		ttyputerr(_("Could not create project directory '%s'"), newname);
		return;
	}
	ttyputmsg(_("Making project directory '%s'..."), newname);

	/* create the project file */
	estrcpy(projfile, librarypath);
	estrcat(projfile, DIRSEPSTR);
	estrcat(projfile, libraryname);
	estrcat(projfile, x_(".proj"));
	io = xcreate(projfile, proj_filetypeproj, 0, 0);
	if (io == NULL)
	{
		ttyputerr(_("Could not create project file '%s'"), projfile);
		return;
	}

	/* turn off all tools */
	if (proj_turnofftools())
	{
		ttyputerr(_("Could not save tool state"));
		return;
	}

	/* make libraries for every cell */
	setvalkey((INTBIG)lib, VLIBRARY, proj_pathkey, (INTBIG)projfile, VSTRING);
	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		/* ignore old unused cell versions */
		if (np->newestversion != np)
		{
			if (np->firstinst == NONODEINST) continue;
			ttyputmsg(_("Warning: including old version of cell %s"), describenodeproto(np));
		}

		/* write the cell to disk in its own library */
		ttyputmsg(_("Entering cell %s"), describenodeproto(np));
		if (proj_writecell(np)) break;

		/* make an entry in the project file */
		xprintf(io, _(":%s:%s:%ld-%s.elib:::Initial checkin\n"), libraryname, np->protoname,
			np->version, np->cellview->viewname);

		/* mark this cell "checked in" and locked */
		proj_marklocked(np, TRUE);
	}

	/* restore tool state */
	proj_restoretoolstate();

	/* close project file */
	xclose(io);

	/* make sure library variables are proper */
	if (getvalkey((INTBIG)lib, VLIBRARY, VSTRING, proj_userkey) != NOVARIABLE)
		(void)delvalkey((INTBIG)lib, VLIBRARY, proj_userkey);

	/* advise the user of this library */
	ttyputmsg(_("The current library should be saved and used by new users"));
}

void proj_addcell(CHAR *cellname)
{
	REGISTER NODEPROTO *np;
	REGISTER LIBRARY *lib;
	CHAR projectpath[256], projectfile[256], libname[256];
	PROJECTCELL *pf, *lastpf;

	/* find out which cell is being added */
	np = getnodeproto(cellname);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot identify cell '%s'"), cellname);
		return;
	}
	lib = np->lib;
	if (np->newestversion != np)
	{
		ttyputerr(_("Cannot add an old version of the cell"));
		return;
	}

	/* make sure there is a valid user name */
	if (proj_getusername(FALSE, lib))
	{
		ttyputerr(_("No valid user"));
		return;
	}

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return;
	}

	/* lock the project file */
	if (proj_lockprojfile(projectpath, projectfile))
	{
		ttyputerr(_("Couldn't lock project file"));
		return;
	}

	/* read the project file */
	if (proj_readprojectfile(projectpath, projectfile))
		ttyputerr(_("Cannot read project file")); else
	{
		/* find this in the project file */
		lastpf = NOPROJECTCELL;
		for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
		{
			if (estrcmp(pf->cellname, np->protoname) == 0 &&
				pf->cellview == np->cellview) break;
			lastpf = pf;
		}
		if (pf != NOPROJECTCELL) ttyputerr(_("This cell is already in the project")); else
		{
			if (proj_startwritingprojectfile(projectpath, projectfile))
				ttyputerr(_("Cannot write project file")); else
			{
				if (proj_writecell(np))
					ttyputerr(_("Error writing cell file")); else
				{
					/* create new entry for this cell */
					pf = proj_allocprojectcell();
					if (pf == 0)
						ttyputerr(_("Cannot add project record")); else
					{
						estrcpy(libname, projectfile);
						libname[estrlen(libname)-5] = 0;
						(void)allocstring(&pf->libname, libname, proj_tool->cluster);
						(void)allocstring(&pf->cellname, np->protoname, proj_tool->cluster);
						pf->cellview = np->cellview;
						pf->cellversion = np->version;
						(void)allocstring(&pf->owner, x_(""), proj_tool->cluster);
						(void)allocstring(&pf->lastowner, proj_username, proj_tool->cluster);
						(void)allocstring(&pf->comment, _("Initial checkin"), proj_tool->cluster);

						/* link it in */
						pf->nextprojectcell = NOPROJECTCELL;
						if (lastpf == NOPROJECTCELL) proj_firstprojectcell = pf; else
							lastpf->nextprojectcell = pf;

						/* mark this cell "checked in" and locked */
						proj_marklocked(np, TRUE);

						ttyputmsg(_("Cell %s added to the project"), describenodeproto(np));
					}
				}

				/* save new project file */
				proj_endwritingprojectfile();
			}
		}
	}

	/* relase project file lock */
	proj_unlockprojfile(projectpath, projectfile);
}

void proj_deletecell(CHAR *cellname)
{
	REGISTER NODEPROTO *np, *onp;
	REGISTER NODEINST *ni;
	REGISTER LIBRARY *lib, *olib;
	CHAR projectpath[256], projectfile[256], *pt;
	PROJECTCELL *pf, *lastpf;
	REGISTER void *infstr;

	/* find out which cell is being deleted */
	np = getnodeproto(cellname);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot identify cell '%s'"), cellname);
		return;
	}
	lib = np->lib;

	/* make sure the cell is not being used */
	if (np->firstinst != NONODEINST)
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				onp->temp1 = 0;
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
			ni->parent->temp1++;
		ttyputerr(_("Cannot delete cell %s because it is still being used by:"), cellname);
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(onp = olib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
				if (onp->temp1 != 0)
					ttyputmsg(_("   Cell %s has %ld instance(s)"), describenodeproto(onp), onp->temp1);
		return;
	}

	/* make sure there is a valid user name */
	if (proj_getusername(FALSE, lib))
	{
		ttyputerr(_("No valid user"));
		return;
	}

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return;
	}

	/* lock the project file */
	if (proj_lockprojfile(projectpath, projectfile))
	{
		ttyputerr(_("Couldn't lock project file"));
		return;
	}

	/* read the project file */
	if (proj_readprojectfile(projectpath, projectfile))
		ttyputerr(_("Cannot read project file")); else
	{
		/* make sure the user has no cells checked-out */
		for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
			if (namesame(pf->owner, proj_username) == 0) break;
		if (pf != NOPROJECTCELL)
		{
			ttyputerr(_("Before deleting a cell from the project, you must check-in all of your work."));
			ttyputerr(_("This is because the deletion may be dependent upon changes recently made."));
			infstr = initinfstr();
			for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
			{
				if (namesame(pf->owner, proj_username) != 0) continue;
				addstringtoinfstr(infstr, pf->cellname);
				if (pf->cellview != el_unknownview)
				{
					addstringtoinfstr(infstr, x_("{"));
					addstringtoinfstr(infstr, pf->cellview->sviewname);
					addstringtoinfstr(infstr, x_("}"));
				}
				addstringtoinfstr(infstr, x_(", "));
			}
			pt = returninfstr(infstr);
			pt[estrlen(pt)-2] = 0;
			ttyputerr(_("These cells are checked out to you: %s"), pt);
		} else
		{
			/* find this in the project file */
			lastpf = NOPROJECTCELL;
			for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
			{
				if (estrcmp(pf->cellname, np->protoname) == 0 &&
					pf->cellview == np->cellview) break;
				lastpf = pf;
			}
			if (pf == NOPROJECTCELL) ttyputerr(_("This cell is not in the project")); else
			{
				if (proj_startwritingprojectfile(projectpath, projectfile))
					ttyputerr(_("Cannot write project file")); else
				{
					/* unlink it */
					if (lastpf == NOPROJECTCELL)
						proj_firstprojectcell = pf->nextprojectcell; else
							lastpf->nextprojectcell = pf->nextprojectcell;

					/* delete the entry */
					efree(pf->libname);
					efree(pf->cellname);
					efree(pf->owner);
					efree(pf->lastowner);
					efree(pf->comment);
					proj_freeprojectcell(pf);

					/* mark this cell unlocked */
					proj_marklocked(np, FALSE);

					/* save new project file */
					proj_endwritingprojectfile();

					ttyputmsg(_("Cell %s deleted from the project"), describenodeproto(np));
				}
			}
		}
	}

	/* relase project file lock */
	proj_unlockprojfile(projectpath, projectfile);
}

/************************ USER DATABASE ***********************/

/* Project User */
static DIALOGITEM proj_usersdialogitems[] =
{
 /*  1 */ {0, {28,184,52,312}, BUTTON, N_("Select User")},
 /*  2 */ {0, {188,212,212,276}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,4,220,172}, SCROLL, x_("")},
 /*  4 */ {0, {8,60,24,112}, MESSAGE, N_("Users:")},
 /*  5 */ {0, {136,184,160,312}, BUTTON, N_("Delete User")},
 /*  6 */ {0, {64,184,88,312}, BUTTON, N_("Change Password")},
 /*  7 */ {0, {100,184,124,312}, BUTTON, N_("New User")}
};
static DIALOG proj_usersdialog = {{50,75,279,398}, N_("User Name"), 0, 7, proj_usersdialogitems, 0, 0};

/* special items for the "User selection" dialog: */
#define DPRU_USERLIST  3		/* User list (scroll) */
#define DPRU_DELUSER   5		/* Delete user (button) */
#define DPRU_CHPASS    6		/* Change password (button) */
#define DPRU_NEWUSER   7		/* New user (button) */

/*
 * Routine to obtain the user's name.  If "newone" is true, force input of a name,
 * otherwise, only ask if unknown.  Returns true if user name is invalid.
 */
BOOLEAN proj_getusername(BOOLEAN newone, LIBRARY *lib)
{
	REGISTER CHAR *pt, *pwd;
	REGISTER INTBIG itemHit, writeback, i;
	REGISTER PUSER *pu, *lastpu;
	REGISTER VARIABLE *var;
	CHAR *truename, userline[256], pwdline[256], projectpath[256], projectfile[256];
	FILE *io;
	REGISTER void *infstr, *dia;

	/* if name exists and not forcing a new user name, stop now */
	if (!newone && proj_username[0] != 0) return(FALSE);

	/* read user database */
	proj_users = NOPUSER;
	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, PUSERFILE);
	io = xopen(returninfstr(infstr), proj_filetypeprojusers, x_(""), &truename);
	if (io == NULL) ttyputmsg(_("Creating new user database")); else
	{
		/* read the users file */
		lastpu = NOPUSER;
		for(;;)
		{
			if (xfgets(userline, 256, io)) break;
			for(pt = userline; *pt != 0; pt++) if (*pt == ':') break;
			if (*pt != ':')
			{
				ttyputerr(_("Missing ':' in user file: %s"), userline);
				break;
			}
			*pt++ = 0;
			pu = (PUSER *)emalloc(sizeof (PUSER), proj_tool->cluster);
			if (pu == 0) break;
			(void)allocstring(&pu->username, userline, proj_tool->cluster);
			(void)allocstring(&pu->userpassword, pt, proj_tool->cluster);
			pu->nextpuser = NOPUSER;
			if (proj_users == NOPUSER) proj_users = lastpu = pu; else
				lastpu->nextpuser = pu;
			lastpu = pu;
		}
		xclose(io);
	}

	/* if not forcing a new user name, see if it is on the library */
	if (!newone)
	{
		/* if already on the library, get it */
		var = getvalkey((INTBIG)lib, VLIBRARY, VSTRING, proj_userkey);
		if (var != NOVARIABLE)
		{
			estrcpy(proj_username, (CHAR *)var->addr);
			for(pu = proj_users; pu != NOPUSER; pu = pu->nextpuser)
				if (estrcmp(proj_username, pu->username) == 0) break;
			if (pu != NOPUSER)
			{
				pwd = proj_wantpassword(1, proj_username);
				if (estrcmp(pwd, pu->userpassword) == 0) return(FALSE);
				ttyputmsg(_("Incorrect password"));
				proj_username[0] = 0;
				return(TRUE);
			}
			proj_username[0] = 0;
			(void)delvalkey((INTBIG)lib, VLIBRARY, proj_userkey);
		}
	}


	/* show the users dialog */
	writeback = 0;
	dia = DiaInitDialog(&proj_usersdialog);
	if (dia == 0) return(TRUE);
	DiaInitTextDialog(dia, DPRU_USERLIST, proj_initusers, proj_nextuser, DiaNullDlogDone, 0,
		SCSELMOUSE | SCSELKEY | SCDOUBLEQUIT);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL) break;
		if (itemHit == OK)
		{
			/* validate the user name */
			i = DiaGetCurLine(dia, DPRU_USERLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DPRU_USERLIST, i);
			for(pu = proj_users; pu != NOPUSER; pu = pu->nextpuser)
				if (estrcmp(pt, pu->username) == 0) break;
			if (pu == NOPUSER) continue;
			pwd = proj_wantpassword(1, pt);
			if (estrcmp(pwd, pu->userpassword) == 0)
			{
				estrcpy(proj_username, pu->username);
				(void)setvalkey((INTBIG)lib, VLIBRARY, proj_userkey,
					(INTBIG)proj_username, VSTRING);
				break;
			}
			ttyputmsg(_("Incorrect password"));
			continue;
		}
		if (itemHit == DPRU_DELUSER)
		{
			/* delete user */
			i = DiaGetCurLine(dia, DPRU_USERLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DPRU_USERLIST, i);
			lastpu = NOPUSER;
			for(pu = proj_users; pu != NOPUSER; pu = pu->nextpuser)
			{
				if (estrcmp(pt, pu->username) == 0) break;
				lastpu = pu;
			}
			if (pu == NOPUSER) continue;
			pwd = proj_wantpassword(1, pt);
			if (estrcmp(pwd, pu->userpassword) != 0)
			{
				ttyputerr(_("Incorrect password"));
				continue;
			}
			if (lastpu == NOPUSER) proj_users = pu->nextpuser; else
				lastpu->nextpuser = pu->nextpuser;
			efree(pu->username);
			efree(pu->userpassword);
			efree((CHAR *)pu);
			DiaLoadTextDialog(dia, DPRU_USERLIST, proj_initusers, proj_nextuser,
				DiaNullDlogDone, 0);
			writeback = 1;
			continue;
		}
		if (itemHit == DPRU_CHPASS)
		{
			/* change password */
			i = DiaGetCurLine(dia, DPRU_USERLIST);
			if (i < 0) continue;
			pt = DiaGetScrollLine(dia, DPRU_USERLIST, i);
			for(pu = proj_users; pu != NOPUSER; pu = pu->nextpuser)
				if (estrcmp(pt, pu->username) == 0) break;
			if (pu == NOPUSER) continue;
			pwd = proj_wantpassword(1, pt);
			if (estrcmp(pwd, pu->userpassword) != 0)
			{
				ttyputerr(_("Incorrect password"));
				continue;
			}
			pwd = proj_wantpassword(2, pt);
			estrcpy(pwdline, pwd);
			pwd = proj_wantpassword(3, pt);
			if (estrcmp(pwdline, pwd) != 0)
			{
				ttyputerr(_("Failed to type password the same way twice"));
				continue;
			}
			(void)reallocstring(&pu->userpassword, pwdline, proj_tool->cluster);
			writeback = 1;
			continue;
		}
		if (itemHit == DPRU_NEWUSER)
		{
			/* new user */
			pt = proj_wantpassword(0, x_(""));
			estrcpy(userline, pt);
			pwd = proj_wantpassword(1, userline);
			estrcpy(pwdline, pwd);
			pwd = proj_wantpassword(3, userline);
			if (estrcmp(pwdline, pwd) != 0)
			{
				ttyputerr(_("Failed to type password the same way twice"));
				continue;
			}

			pu = (PUSER *)emalloc(sizeof (PUSER), proj_tool->cluster);
			if (pu == 0) continue;
			(void)allocstring(&pu->username, userline, proj_tool->cluster);
			(void)allocstring(&pu->userpassword, pwdline, proj_tool->cluster);
			pu->nextpuser = proj_users;
			proj_users = pu;
			DiaLoadTextDialog(dia, DPRU_USERLIST, proj_initusers, proj_nextuser,
				DiaNullDlogDone, 0);
			writeback = 1;
			continue;
		}
	}
	DiaDoneDialog(dia);

	if (itemHit == CANCEL) return(TRUE);

	/* write the file back if necessary */
	if (writeback != 0)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, el_libdir);
		addstringtoinfstr(infstr, PUSERFILE);
		io = xopen(returninfstr(infstr), proj_filetypeprojusers | FILETYPEWRITE, x_(""), &truename);
		if (io == NULL)
		{
			ttyputmsg(_("Cannot save users database"));
			return(TRUE);
		}

		/* write the users file */
		for(pu = proj_users; pu != NOPUSER; pu = pu->nextpuser)
			xprintf(io, x_("%s:%s\n"), pu->username, pu->userpassword);
		xclose(io);

		/* if switching users, update all locks in the library */
		if (proj_getprojinfo(lib, projectpath, projectfile))
		{
			ttyputerr(_("Cannot find project file"));
			return(FALSE);
		}
		if (proj_readprojectfile(projectpath, projectfile))
		{
			ttyputerr(_("Cannot read project file"));
			return(FALSE);
		}
		proj_validatelocks(lib);
	}
	return(FALSE);
}

/* Project Password */
static DIALOGITEM proj_passworddialogitems[] =
{
 /*  1 */ {0, {72,12,96,76}, BUTTON, N_("OK")},
 /*  2 */ {0, {72,132,96,196}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {12,8,28,205}, MESSAGE, N_("Password for")},
 /*  4 */ {0, {40,48,56,165}, EDITTEXT, x_("")}
};
static DIALOG proj_passworddialog = {{298,75,404,292}, N_("User / Password"), 0, 4, proj_passworddialogitems, 0, 0};

/* special items for the "Password prompt" dialog: */
#define DPRP_USERNAME  3		/* User name (stat text) */
#define DPRP_PASSWORD  4		/* Password (edit text) */

/*
 * Routine to prompt for a user name/password, according to the "mode" and return
 * the string.  "mode" is:
 *   0 to prompt for a user name
 *   1 to prompt for an existing password
 *   2 to prompt for a new password
 *   3 to prompt for a password verification
 */
CHAR *proj_wantpassword(INTBIG mode, CHAR *username)
{
	REGISTER INTBIG itemHit;
	static CHAR typedstuff[256];
	REGISTER void *dia;

	dia = DiaInitDialog(&proj_passworddialog);
	if (dia == 0) return(x_(""));
	switch (mode)
	{
		case 0:
			DiaSetText(dia, DPRP_USERNAME, _("New User Name"));
			break;
		case 1:
			esnprintf(typedstuff, 256, _("Password for %s"), username);
			DiaSetText(dia, DPRP_USERNAME, typedstuff);
			break;
		case 2:
			esnprintf(typedstuff, 256, _("New Password for %s"), username);
			DiaSetText(dia, DPRP_USERNAME, typedstuff);
			break;
		case 3:
			esnprintf(typedstuff, 256, _("Verify Password for %s"), username);
			DiaSetText(dia, DPRP_USERNAME, typedstuff);
			break;
	}
	if (mode != 0) DiaOpaqueEdit(dia, DPRP_PASSWORD);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
	}
	estrcpy(typedstuff, DiaGetText(dia, DPRP_PASSWORD));
	DiaDoneDialog(dia);
	if (mode != 0)
		myencrypt(typedstuff, x_("BicIsSchediwy"));
	return(typedstuff);
}

BOOLEAN proj_initusers(CHAR **c)
{
	proj_userpos = proj_users;
	return(TRUE);
}

CHAR *proj_nextuser(void)
{
	REGISTER CHAR *ret;

	if (proj_userpos == NOPUSER) return(0);
	ret = proj_userpos->username;
	proj_userpos = proj_userpos->nextpuser;
	return(ret);
}

/************************ PROJECT DATABASE ***********************/

void proj_checkinmany(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	CHAR projectpath[256], projectfile[256];
	PROJECTCELL *pf;
	REGISTER LIBRARY *olib;

	/* make sure there is a valid user name */
	if (proj_getusername(FALSE, lib))
	{
		ttyputerr(_("No valid user"));
		return;
	}

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return;
	}

	/* lock the project file */
	if (proj_lockprojfile(projectpath, projectfile))
	{
		ttyputerr(_("Couldn't lock project file"));
		return;
	}

	/* read the project file */
	if (proj_readprojectfile(projectpath, projectfile))
		ttyputerr(_("Cannot read project file")); else
	{
		for(olib = el_curlib; olib != NOLIBRARY; olib = olib->nextlibrary)
			for(np = olib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		{
			if (np->temp1 == 0) continue;
			if (stopping(STOPREASONCHECKIN)) break;

			/* find this in the project file */
			pf = proj_findcell(np);
			if (pf == NOPROJECTCELL)
				ttyputerr(_("Cell %s is not in the project"), describenodeproto(np)); else
			{
				/* see if it is available */
				if (estrcmp(pf->owner, proj_username) != 0)
					ttyputerr(_("Cell %s is not checked out to you"), describenodeproto(np)); else
				{
					if (!proj_getcomments(pf, x_("in")))
					{
						/* prepare to write it back */
						if (proj_startwritingprojectfile(projectpath, projectfile))
							ttyputerr(_("Cannot write project file")); else
						{
							/* write the cell out there */
							if (proj_writecell(np))
								ttyputerr(_("Error writing cell %s"), describenodeproto(np)); else
							{
								(void)reallocstring(&pf->owner, x_(""), proj_tool->cluster);
								(void)reallocstring(&pf->lastowner, proj_username, proj_tool->cluster);
								pf->cellversion = np->version;
								proj_marklocked(np, TRUE);
								ttyputmsg(_("Cell %s checked in"), describenodeproto(np));
							}
							proj_endwritingprojectfile();
						}
					}
				}
			}
		}
	}

	/* relase project file lock */
	proj_unlockprojfile(projectpath, projectfile);
}

/*
 * Routine to obtain information about the project associated with library "lib".
 * The path to the project is placed in "path" and the name of the project file
 * in that directory is placed in "projfile".  Returns true on error.
 */
BOOLEAN proj_getprojinfo(LIBRARY *lib, CHAR *path, CHAR *projfile)
{
	REGISTER VARIABLE *var;
	CHAR *params[2], *truename;
	FILE *io;
	REGISTER INTBIG i;
	extern COMCOMP us_colorreadp;
	REGISTER void *infstr;

	/* see if there is a variable in the current library with the project path */
	var = getvalkey((INTBIG)lib, VLIBRARY, VSTRING, proj_pathkey);
	if (var != NOVARIABLE)
	{
		estrcpy(path, (CHAR *)var->addr);
		io = xopen(path, proj_filetypeproj, x_(""), &truename);
		if (io != 0) xclose(io); else
			var = NOVARIABLE;
	}
	if (var == NOVARIABLE)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("proj/"));
		addstringtoinfstr(infstr, _("Project File"));
		i = ttygetparam(returninfstr(infstr), &us_colorreadp, 1, params);
		if (i == 0) return(TRUE);
		estrcpy(path, params[0]);
		setvalkey((INTBIG)lib, VLIBRARY, proj_pathkey, (INTBIG)path, VSTRING);
	}

	for(i = estrlen(path)-1; i>0; i--) if (path[i] == DIRSEP) break;
	if (path[i] == DIRSEP)
	{
		estrcpy(projfile, &path[i+1]);
		path[i+1] = 0;
	} else
	{
		estrcpy(projfile, path);
		path[0] = 0;
	}
	return(FALSE);
}

/* Project List */
static DIALOGITEM proj_listdialogitems[] =
{
 /*  1 */ {0, {204,312,228,376}, BUTTON, N_("Done")},
 /*  2 */ {0, {4,4,196,376}, SCROLL, x_("")},
 /*  3 */ {0, {260,4,324,376}, MESSAGE, x_("")},
 /*  4 */ {0, {240,4,256,86}, MESSAGE, N_("Comments:")},
 /*  5 */ {0, {204,120,228,220}, BUTTON, N_("Check It Out")},
 /*  6 */ {0, {236,4,237,376}, DIVIDELINE, x_("")},
 /*  7 */ {0, {204,8,228,108}, BUTTON, N_("Check It In")},
 /*  8 */ {0, {204,232,228,296}, BUTTON, N_("Update")}
};
static DIALOG proj_listdialog = {{50,75,383,462}, N_("Project Management"), 0, 8, proj_listdialogitems, 0, 0};

/* special items for the "Project list" dialog: */
#define DPRL_PROJLIST  2		/* Project list (scroll) */
#define DPRL_COMMENTS  3		/* Comments (stat text) */
#define DPRL_CHECKOUT  5		/* Check out (button) */
#define DPRL_CHECKIN   7		/* Check in (button) */
#define DPRL_UPDATE    8		/* Update (button) */

void proj_showlistdialog(LIBRARY *lib)
{
	REGISTER INTBIG itemHit, i, j;
	REGISTER PROJECTCELL *pf;
	REGISTER void *infstr, *dia;

	if (proj_getusername(FALSE, lib))
	{
		ttyputerr(_("No valid user"));
		return;
	}
	proj_active = TRUE;

	/* show the dialog */
	dia = DiaInitDialog(&proj_listdialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DPRL_PROJLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone,
		-1, SCSELMOUSE | SCREPORT);

	/* load project information into the scroll area */
	if (proj_loadlist(lib, dia))
	{
		DiaDoneDialog(dia);
		return;
	}

	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK) break;
		if (itemHit == DPRL_UPDATE)
		{
			/* update project */
			proj_update(lib);
			(void)proj_loadlist(lib, dia);
			us_endbatch();
			continue;
		}
		if (itemHit == DPRL_CHECKOUT || itemHit == DPRL_CHECKIN ||
			itemHit == DPRL_PROJLIST)
		{
			/* figure out which cell is selected */
			i = DiaGetCurLine(dia, DPRL_PROJLIST);
			if (i < 0) continue;
			for(j=0, pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell, j++)
				if (i == j) break;
			if (pf == NOPROJECTCELL) continue;

			if (itemHit == DPRL_CHECKOUT)
			{
				/* check it out */
				infstr = initinfstr();
				addstringtoinfstr(infstr, pf->cellname);
				if (pf->cellview != el_unknownview)
				{
					addtoinfstr(infstr, '{');
					addstringtoinfstr(infstr, pf->cellview->sviewname);
					addtoinfstr(infstr, '}');
				}
				proj_checkout(returninfstr(infstr), FALSE);
				(void)proj_loadlist(lib, dia);
				us_endbatch();
				continue;
			}
			if (itemHit == DPRL_CHECKIN)
			{
				/* check it in */
				infstr = initinfstr();
				addstringtoinfstr(infstr, pf->cellname);
				if (pf->cellview != el_unknownview)
				{
					addtoinfstr(infstr, '{');
					addstringtoinfstr(infstr, pf->cellview->sviewname);
					addtoinfstr(infstr, '}');
				}
				proj_checkin(returninfstr(infstr));
				(void)proj_loadlist(lib, dia);
				continue;
			}
			if (itemHit == DPRL_PROJLIST)
			{
				if (*pf->comment != 0) DiaSetText(dia, DPRL_COMMENTS, pf->comment);
				if (*pf->owner == 0) DiaUnDimItem(dia, DPRL_CHECKOUT); else
					DiaDimItem(dia, DPRL_CHECKOUT);
				if (estrcmp(pf->owner, proj_username) == 0) DiaUnDimItem(dia, DPRL_CHECKIN); else
					DiaDimItem(dia, DPRL_CHECKIN);
				continue;
			}
		}
	}
	DiaDoneDialog(dia);
}

/*
 * Routine to display the current project information in the list dialog.
 * Returns true on error.
 */
BOOLEAN proj_loadlist(LIBRARY *lib, void *dia)
{
	REGISTER BOOLEAN failed, uptodate;
	REGISTER INTBIG whichline, thisline;
	REGISTER PROJECTCELL *pf, *curpf;
	CHAR line[256], projectpath[256], projectfile[256];
	REGISTER NODEPROTO *np, *curcell;

	/* get location of project file */
	if (proj_getprojinfo(lib, projectpath, projectfile))
	{
		ttyputerr(_("Cannot find project file"));
		return(TRUE);
	}

	/* lock the project file */
	if (proj_lockprojfile(projectpath, projectfile))
	{
		ttyputerr(_("Couldn't lock project file"));
		return(TRUE);
	}

	/* read the project file */
	failed = FALSE;
	if (proj_readprojectfile(projectpath, projectfile))
	{
		ttyputerr(_("Cannot read project file"));
		failed = TRUE;
	}

	/* relase project file lock */
	proj_unlockprojfile(projectpath, projectfile);
	if (failed) return(TRUE);

	/* find current cell */
	curcell = getcurcell();

	/* show what is in the project file */
	DiaLoadTextDialog(dia, DPRL_PROJLIST, DiaNullDlogList, DiaNullDlogItem, DiaNullDlogDone, -1);
	whichline = -1;
	thisline = 0;
	uptodate = TRUE;
	for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
	{
		/* see if project is up-to-date */
		np = db_findnodeprotoname(pf->cellname, pf->cellview, el_curlib);
		if (np == NONODEPROTO || np->version < pf->cellversion) uptodate = FALSE;

		/* remember this line if it describes the current cell */
		if (curcell != NONODEPROTO && namesame(curcell->protoname, pf->cellname) == 0 &&
			curcell->cellview == pf->cellview)
		{
			whichline = thisline;
			curpf = pf;
		}

		/* describe this project cell */
		if (pf->cellview == el_unknownview)
		{
			esnprintf(line, 256, x_("%s;%ld"), pf->cellname, pf->cellversion);
		} else
		{
			esnprintf(line, 256, x_("%s{%s};%ld"), pf->cellname, pf->cellview->sviewname,
				pf->cellversion);
		}
		if (*pf->owner == 0)
		{
			estrcat(line, _(" AVAILABLE"));
			if (*pf->lastowner != 0)
			{
				estrcat(line, _(", last mod by "));
				estrcat(line, pf->lastowner);
			}
		} else
		{
			if (estrcmp(pf->owner, proj_username) == 0)
			{
				estrcat(line, _(" EDITABLE, checked out to you"));
			} else
			{
				estrcat(line, _(" UNAVAILABLE, checked out to "));
				estrcat(line, pf->owner);
			}
		}
		DiaStuffLine(dia, DPRL_PROJLIST, line);
		thisline++;
	}
	DiaSelectLine(dia, DPRL_PROJLIST, whichline);

	DiaDimItem(dia, DPRL_CHECKOUT);
	DiaDimItem(dia, DPRL_CHECKIN);
	if (whichline >= 0)
	{
		if (*curpf->comment != 0) DiaSetText(dia, DPRL_COMMENTS, curpf->comment);
		if (*curpf->owner == 0) DiaUnDimItem(dia, DPRL_CHECKOUT); else
			DiaDimItem(dia, DPRL_CHECKOUT);
		if (estrcmp(curpf->owner, proj_username) == 0) DiaUnDimItem(dia, DPRL_CHECKIN); else
			DiaDimItem(dia, DPRL_CHECKIN);
	}

	if (uptodate) DiaDimItem(dia, DPRL_UPDATE); else
	{
		ttyputmsg(_("Your library does not contain the most recent additions to the project."));
		ttyputmsg(_("You should do an 'Update' to make it current."));
		DiaUnDimItem(dia, DPRL_UPDATE);
	}
	return(FALSE);
}

/* Project Comments */
static DIALOGITEM proj_commentsdialogitems[] =
{
 /*  1 */ {0, {104,244,128,308}, BUTTON, N_("OK")},
 /*  2 */ {0, {104,48,128,112}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {4,8,20,363}, MESSAGE, N_("Reason for checking out cell")},
 /*  4 */ {0, {28,12,92,358}, EDITTEXT, x_("")}
};
static DIALOG proj_commentsdialog = {{108,75,248,447}, N_("Project Comments"), 0, 4, proj_commentsdialogitems, 0, 0};

/* special items for the "Project list" dialog: */
#define DPRC_COMMENT_L  3		/* Comment label (stat text) */
#define DPRC_COMMENT    4		/* Comment (edit text) */

/*
 * Routine to obtain comments about a checkin/checkout for cell "pf".  "direction" is
 * either "in" or "out".  Returns true if the dialog is cancelled.
 */
BOOLEAN proj_getcomments(PROJECTCELL *pf, CHAR *direction)
{
	REGISTER INTBIG itemHit;
	static CHAR line[256];
	REGISTER void *dia;

	dia = DiaInitDialog(&proj_commentsdialog);
	if (dia == 0) return(TRUE);
	esnprintf(line, 256, _("Reason for checking-%s cell %s"), direction, pf->cellname);
	DiaSetText(dia, DPRC_COMMENT_L, line);
	DiaSetText(dia, -DPRC_COMMENT, pf->comment);
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == CANCEL || itemHit == OK) break;
	}
	if (itemHit == OK)
		(void)reallocstring(&pf->comment, DiaGetText(dia, DPRC_COMMENT), proj_tool->cluster);
	DiaDoneDialog(dia);
	if (itemHit == CANCEL) return(TRUE);
	return(FALSE);
}

/************************ PROJECT FILE ***********************/

BOOLEAN proj_readprojectfile(CHAR *pathname, CHAR *filename)
{
	CHAR *truename, projline[256], *pt, *start, fullname[256], origprojline[256];
	FILE *io;
	REGISTER BOOLEAN err;
	PROJECTCELL *pf, *pftail, *nextpf, *opf;

	/* delete previous project database */
	for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = nextpf)
	{
		nextpf = pf->nextprojectcell;
		efree(pf->libname);
		efree(pf->cellname);
		efree(pf->owner);
		efree(pf->lastowner);
		efree(pf->comment);
		proj_freeprojectcell(pf);
	}

	/* read the project file */
	proj_firstprojectcell = pftail = NOPROJECTCELL;
	estrcpy(fullname, pathname);
	estrcat(fullname, filename);
	io = xopen(fullname, proj_filetypeproj, x_(""), &truename);
	if (io == 0)
	{
		ttyputerr(_("Couldn't read project file '%s'"), fullname);
		return(TRUE);
	}

	err = FALSE;
	for(;;)
	{
		if (xfgets(projline, 256, io)) break;
		estrcpy(origprojline, projline);
		pf = proj_allocprojectcell();
		if (pf == 0) break;

		pt = projline;
		if (*pt++ != ':')
		{
			ttyputerr(_("Missing initial ':' in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}

		/* get library name */
		for(start = pt; *pt != 0; pt++) if (*pt == ':') break;
		if (*pt != ':')
		{
			ttyputerr(_("Missing ':' after library name in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}
		*pt++ = 0;
		(void)allocstring(&pf->libname, start, proj_tool->cluster);

		/* get cell name */
		for(start = pt; *pt != 0; pt++) if (*pt == ':') break;
		if (*pt != ':')
		{
			ttyputerr(_("Missing ':' after cell name in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}
		*pt++ = 0;
		(void)allocstring(&pf->cellname, start, proj_tool->cluster);

		/* get version */
		pf->cellversion = eatoi(pt);
		for( ; *pt != 0; pt++) if (*pt == '-') break;
		if (*pt++ != '-')
		{
			ttyputerr(_("Missing '-' after version number in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}

		/* get view */
		for(start = pt; *pt != 0; pt++)
			if (pt[0] == '.' && pt[1] == 'e' && pt[2] == 'l' && pt[3] == 'i' && pt[4] == 'b' &&
				pt[5] == ':') break;
		if (*pt == 0)
		{
			ttyputerr(_("Missing '.elib' after view name in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}
		*pt = 0;
		pt += 6;
		pf->cellview = getview(start);

		/* get owner */
		for(start = pt; *pt != 0; pt++) if (*pt == ':') break;
		if (*pt != ':')
		{
			ttyputerr(_("Missing ':' after owner name in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}
		*pt++ = 0;
		(void)allocstring(&pf->owner, start, proj_tool->cluster);

		/* get last owner */
		for(start = pt; *pt != 0; pt++) if (*pt == ':') break;
		if (*pt != ':')
		{
			ttyputerr(_("Missing ':' after last owner name in project file: '%s'"), origprojline);
			err = TRUE;
			break;
		}
		*pt++ = 0;
		(void)allocstring(&pf->lastowner, start, proj_tool->cluster);

		/* get comments */
		(void)allocstring(&pf->comment, pt, proj_tool->cluster);

		/* check for duplication */
		for(opf = proj_firstprojectcell; opf != NOPROJECTCELL; opf = opf->nextprojectcell)
		{
			if (namesame(opf->cellname, pf->cellname) != 0) continue;
			if (opf->cellview != pf->cellview) continue;
			ttyputerr(_("Error in project file: view '%s' of cell '%s' exists twice (versions %ld and %ld)"),
				pf->cellview->viewname, pf->cellname, pf->cellversion, opf->cellversion);
		}

		/* link it in */
		pf->nextprojectcell = NOPROJECTCELL;
		if (proj_firstprojectcell == NOPROJECTCELL) proj_firstprojectcell = pftail = pf; else
		{
			pftail->nextprojectcell = pf;
			pftail = pf;
		}
	}
	xclose(io);
	return(err);
}

BOOLEAN proj_startwritingprojectfile(CHAR *pathname, CHAR *filename)
{
	CHAR *truename, fullname[256];

	/* read the project file */
	estrcpy(fullname, pathname);
	estrcat(fullname, filename);
	proj_io = xopen(fullname, proj_filetypeproj | FILETYPEWRITE, x_(""), &truename);
	if (proj_io == 0)
	{
		ttyputerr(_("Couldn't write project file '%s'"), fullname);
		return(TRUE);
	}
	return(FALSE);
}

void proj_endwritingprojectfile(void)
{
	PROJECTCELL *pf;

	for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
		xprintf(proj_io, x_(":%s:%s:%ld-%s.elib:%s:%s:%s\n"), pf->libname, pf->cellname,
			pf->cellversion, pf->cellview->viewname, pf->owner, pf->lastowner, pf->comment);
	xclose(proj_io);
	noundoallowed();
}

PROJECTCELL *proj_allocprojectcell(void)
{
	PROJECTCELL *pf;

	if (proj_projectcellfree != NOPROJECTCELL)
	{
		pf = proj_projectcellfree;
		proj_projectcellfree = proj_projectcellfree->nextprojectcell;
	} else
	{
		pf = (PROJECTCELL *)emalloc(sizeof (PROJECTCELL), proj_tool->cluster);
		if (pf == 0) return(0);
	}
	return(pf);
}

void proj_freeprojectcell(PROJECTCELL *pf)
{
	pf->nextprojectcell = proj_projectcellfree;
	proj_projectcellfree = pf;
}

PROJECTCELL *proj_findcell(NODEPROTO *np)
{
	PROJECTCELL *pf;

	for(pf = proj_firstprojectcell; pf != NOPROJECTCELL; pf = pf->nextprojectcell)
		if (estrcmp(pf->cellname, np->protoname) == 0 && pf->cellview == np->cellview)
			return(pf);
	return(NOPROJECTCELL);
}

/************************ LOCKING ***********************/

#define MAXTRIES 10
#define NAPTIME  5

BOOLEAN proj_lockprojfile(CHAR *projectpath, CHAR *projectfile)
{
	CHAR lockfilename[256];
	REGISTER INTBIG i, j;

	esnprintf(lockfilename, 256, x_("%s%sLOCK"), projectpath, projectfile);
	for(i=0; i<MAXTRIES; i++)
	{
		if (lockfile(lockfilename)) return(FALSE);
		if (i == 0) ttyputmsg(_("Project file locked.  Waiting...")); else
			ttyputmsg(_("Still waiting (will try %d more times)..."), MAXTRIES-i);
		for(j=0; j<NAPTIME; j++)
		{
			gotosleep(60);
			if (stopping(STOPREASONLOCK)) return(TRUE);
		}
	}
	return(TRUE);
}

void proj_unlockprojfile(CHAR *projectpath, CHAR *projectfile)
{
	CHAR lockfilename[256];

	esnprintf(lockfilename, 256, x_("%s%sLOCK"), projectpath, projectfile);
	unlockfile(lockfilename);
}

void proj_validatelocks(LIBRARY *lib)
{
	REGISTER NODEPROTO *np;
	PROJECTCELL *pf;

	for(np = lib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		pf = proj_findcell(np);
		if (pf == NOPROJECTCELL)
		{
			/* cell not in the project: writable */
			proj_marklocked(np, FALSE);
		} else
		{
			if (np->version < pf->cellversion)
			{
				/* cell is an old version: writable */
				proj_marklocked(np, FALSE);
			} else
			{
				if (namesame(pf->owner, proj_username) == 0)
				{
					/* cell checked out to current user: writable */
					proj_marklocked(np, FALSE);
				} else
				{
					/* cell checked out to someone else: not writable */
					proj_marklocked(np, TRUE);
				}
			}
		}
	}
}

void proj_marklocked(NODEPROTO *np, BOOLEAN locked)
{
	REGISTER NODEPROTO *onp;

	if (!locked)
	{
		FOR_CELLGROUP(onp, np)
		{
			if (onp->cellview != np->cellview) continue;
			if (getvalkey((INTBIG)onp, VNODEPROTO, VINTEGER, proj_lockedkey) != NOVARIABLE)
				(void)delvalkey((INTBIG)onp, VNODEPROTO, proj_lockedkey);
		}
	} else
	{
		FOR_CELLGROUP(onp, np)
		{
			if (onp->cellview != np->cellview) continue;
			if (onp->newestversion == onp)
			{
				if (getvalkey((INTBIG)onp, VNODEPROTO, VINTEGER, proj_lockedkey) == NOVARIABLE)
					setvalkey((INTBIG)onp, VNODEPROTO, proj_lockedkey, 1, VINTEGER);
			} else
			{
				if (getvalkey((INTBIG)onp, VNODEPROTO, VINTEGER, proj_lockedkey) != NOVARIABLE)
					(void)delvalkey((INTBIG)onp, VNODEPROTO, proj_lockedkey);
			}
		}
	}
}

/************************ CHANGE TRACKING ***********************/

void proj_queuecheck(NODEPROTO *cell)
{
	REGISTER FCHECK *f;

	/* see if the cell is already queued */
	for(f = proj_firstfcheck; f != NOFCHECK; f = f->nextfcheck)
		if (f->entry == cell && f->batchnumber == proj_batchnumber) return;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	f = proj_allocfcheck();
	if (f == NOFCHECK)
	{
		ttyputnomemory();
		return;
	}
	f->entry = cell;
	f->batchnumber = proj_batchnumber;
	f->nextfcheck = proj_firstfcheck;
	proj_firstfcheck = f;
}

FCHECK *proj_allocfcheck(void)
{
	REGISTER FCHECK *f;

	/* code cannot be called by multiple procesors: uses globals */
	NOT_REENTRANT;

	if (proj_fcheckfree == NOFCHECK)
	{
		f = (FCHECK *)emalloc((sizeof (FCHECK)), proj_tool->cluster);
		if (f == 0) return(NOFCHECK);
	} else
	{
		f = proj_fcheckfree;
		proj_fcheckfree = f->nextfcheck;
	}

	return(f);
}

void proj_freefcheck(FCHECK *f)
{
	f->nextfcheck = proj_fcheckfree;
	proj_fcheckfree = f;
}

/************************ COPYING CELLS IN AND OUT OF DATABASE ***********************/

/*
 * Routine to get the latest version of the cell described by "pf" and return
 * the newly created cell.  Returns NONODEPROTO on error.
 */
NODEPROTO *proj_getcell(PROJECTCELL *pf, LIBRARY *lib)
{
	CHAR celllibname[256], celllibpath[256], cellprojfile[256], cellname[256],
		*templibname;
	REGISTER LIBRARY *flib;
	REGISTER NODEPROTO *newnp;
	REGISTER INTBIG oldverbose, ret;

	/* create the library */
	if (proj_getprojinfo(lib, celllibpath, cellprojfile))
	{
		ttyputerr(_("Cannot find project info on library %s"), lib->libname);
		return(NONODEPROTO);
	}
	esnprintf(celllibname, 256, x_("%ld-%s.elib"), pf->cellversion, pf->cellview->viewname);
	cellprojfile[estrlen(cellprojfile)-5] = 0;
	estrcat(celllibpath, cellprojfile);
	estrcat(celllibpath, DIRSEPSTR);
	estrcat(celllibpath, pf->cellname);
	estrcat(celllibpath, DIRSEPSTR);
	estrcat(celllibpath, celllibname);

	/* prevent tools (including this one) from seeing the change */
	(void)proj_turnofftools();

	templibname = proj_templibraryname();
	flib = newlibrary(templibname, celllibpath);
	if (flib == NOLIBRARY)
	{
		ttyputerr(_("Cannot create library %s"), celllibpath);
		proj_restoretoolstate();
		return(NONODEPROTO);
	}
	oldverbose = asktool(io_tool, x_("verbose"), 0);
	ret = asktool(io_tool, x_("read"), (INTBIG)flib, (INTBIG)x_("binary"));
	(void)asktool(io_tool, x_("verbose"), oldverbose);
	if (ret != 0)
	{
		ttyputerr(_("Cannot read library %s"), celllibpath);
		killlibrary(flib);
		proj_restoretoolstate();
		return(NONODEPROTO);
	}
	if (flib->curnodeproto == NONODEPROTO)
	{
		ttyputerr(_("Cannot find cell in library %s"), celllibpath);
		killlibrary(flib);
		proj_restoretoolstate();
		return(NONODEPROTO);
	}
	esnprintf(cellname, 256, x_("%s;%ld"), flib->curnodeproto->protoname, flib->curnodeproto->version);
	if (flib->curnodeproto->cellview != el_unknownview)
	{
		estrcat(cellname, x_("{"));
		estrcat(cellname, flib->curnodeproto->cellview->sviewname);
		estrcat(cellname, x_("}"));
	}
	newnp = copynodeproto(flib->curnodeproto, lib, cellname, TRUE);
	if (newnp == NONODEPROTO)
	{
		ttyputerr(_("Cannot copy cell %s from new library"), describenodeproto(flib->curnodeproto));
		killlibrary(flib);
		proj_restoretoolstate();
		return(NONODEPROTO);
	}
	(*el_curconstraint->solve)(newnp);

	/* must do this explicitly because the library kill flushes change batches */
	/* (void)asktool(net_tool, "re-number", (INTBIG)newnp); */

	/* kill the library */
	killlibrary(flib);

	/* restore tool state */
	proj_restoretoolstate();

	/* return the new cell */
	return(newnp);
}

BOOLEAN proj_usenewestversion(NODEPROTO *oldnp, NODEPROTO *newnp)
{
	INTBIG lx, hx, ly, hy;
	REGISTER WINDOWPART *w;
	REGISTER LIBRARY *lib;
	REGISTER NODEINST *ni, *newni, *nextni;

	/* prevent tools (including this one) from seeing the change */
	(void)proj_turnofftools();

	/* replace them all */
	for(ni = oldnp->firstinst; ni != NONODEINST; ni = nextni)
	{
		nextni = ni->nextinst;
		newni = replacenodeinst(ni, newnp, FALSE, FALSE);
		if (newni == NONODEINST)
		{
			ttyputerr(_("Failed to update instance of %s in %s"), describenodeproto(newnp),
				describenodeproto(ni->parent));
			proj_restoretoolstate();
			return(TRUE);
		}
	}

	/* redraw windows that updated */
	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
	{
		if (w->curnodeproto == NONODEPROTO) continue;
		if (w->curnodeproto != newnp) continue;
		w->curnodeproto = newnp;

		/* redisplay the window with the new cell */
		us_fullview(newnp, &lx, &hx, &ly, &hy);
		us_squarescreen(w, NOWINDOWPART, FALSE, &lx, &hx, &ly, &hy, 0);
		startobjectchange((INTBIG)w, VWINDOWPART);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenlx"), lx, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenhx"), hx, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenly"), ly, VINTEGER);
		(void)setval((INTBIG)w, VWINDOWPART, x_("screenhy"), hy, VINTEGER);
		us_gridset(w, w->state);
		endobjectchange((INTBIG)w, VWINDOWPART);
	}

	/* update status display if necessary */
	if (us_curnodeproto == oldnp) us_setnodeproto(newnp);

	/* replace library references */
	for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
		if (lib->curnodeproto == oldnp)
			(void)setval((INTBIG)lib, VLIBRARY, x_("curnodeproto"), (INTBIG)newnp, VNODEPROTO);

	if (killnodeproto(oldnp))
		ttyputerr(_("Could not delete old version"));

	/* restore tool state */
	proj_restoretoolstate();
	return(FALSE);
}

BOOLEAN proj_writecell(NODEPROTO *np)
{
	REGISTER LIBRARY *flib;
	REGISTER INTBIG retval;
	CHAR libname[256], libfile[256], projname[256], *templibname;
	REGISTER NODEPROTO *npcopy;
	INTBIG filestatus;

	if (proj_getprojinfo(np->lib, libfile, projname))
	{
		ttyputerr(_("Cannot find project info on library %s"), np->lib->libname);
		return(TRUE);
	}
	projname[estrlen(projname)-5] = 0;
	estrcat(libfile, projname);
	estrcat(libfile, DIRSEPSTR);
	estrcat(libfile, np->protoname);

	/* make the directory if necessary */
	filestatus = fileexistence(libfile);
	if (filestatus == 1 || filestatus == 3)
	{
		ttyputerr(_("Could not create cell directory '%s'"), libfile);
		return(TRUE);
	}
	if (filestatus == 0)
	{
		if (createdirectory(libfile))
		{
			ttyputerr(_("Could not create cell directory '%s'"), libfile);
			return(TRUE);
		}
	}

	estrcat(libfile, DIRSEPSTR);
	esnprintf(libname, 256, x_("%ld-%s.elib"), np->version, np->cellview->viewname);
	estrcat(libfile, libname);

	/* prevent tools (including this one) from seeing the change */
	(void)proj_turnofftools();

	templibname = proj_templibraryname();
	flib = newlibrary(templibname, libfile);
	if (flib == NOLIBRARY)
	{
		ttyputerr(_("Cannot create library %s"), libfile);
		proj_restoretoolstate();
		return(TRUE);
	}
	npcopy = copyrecursively(np, flib);
	if (npcopy == NONODEPROTO)
	{
		ttyputerr(_("Could not place %s in a library"), describenodeproto(np));
		killlibrary(flib);
		proj_restoretoolstate();
		return(TRUE);
	}

	flib->curnodeproto = npcopy;
	flib->userbits |= READFROMDISK;
	makeoptionstemporary(flib);
	retval = asktool(io_tool, x_("write"), (INTBIG)flib, (INTBIG)x_("binary"));
	restoreoptionstate(flib);
	if (retval != 0)
	{
		ttyputerr(_("Could not save library with %s in it"), describenodeproto(np));
		killlibrary(flib);
		proj_restoretoolstate();
		return(TRUE);
	}
	killlibrary(flib);

	/* restore tool state */
	proj_restoretoolstate();

	return(FALSE);
}

CHAR *proj_templibraryname(void)
{
	static CHAR libname[256];
	REGISTER LIBRARY *lib;
	REGISTER INTBIG i;

	for(i=1; ; i++)
	{
		esnprintf(libname, 256, x_("projecttemp%ld"), i);
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			if (namesame(libname, lib->libname) == 0) break;
		if (lib == NOLIBRARY) break;
	}
	return(libname);
}

/*
 * Routine to save the state of all tools and turn them off.
 */
BOOLEAN proj_turnofftools(void)
{
	REGISTER INTBIG i;
	REGISTER TOOL *tool;

	/* turn off all tools for this operation */
	if (proj_savetoolstate == 0)
	{
		proj_savetoolstate = (INTBIG *)emalloc(el_maxtools * SIZEOFINTBIG, el_tempcluster);
		if (proj_savetoolstate == 0) return(TRUE);
	}
	for(i=0; i<el_maxtools; i++)
	{
		tool = &el_tools[i];
		proj_savetoolstate[i] = tool->toolstate;
		if (tool == us_tool || tool == proj_tool || tool == net_tool) continue;
		tool->toolstate &= ~TOOLON;
	}
	proj_ignorechanges = TRUE;
	return(FALSE);
}

/*
 * Routine to restore the state of all tools that were reset by "proj_turnofftools()".
 */
void proj_restoretoolstate(void)
{
	REGISTER INTBIG i;

	if (proj_savetoolstate == 0) return;
	for(i=0; i<el_maxtools; i++)
		el_tools[i].toolstate = proj_savetoolstate[i];
	proj_ignorechanges = FALSE;
}

/************************ DATABASE OPERATIONS ***********************/

NODEPROTO *copyrecursively(NODEPROTO *fromnp, LIBRARY *tolib)
{
	REGISTER NODEPROTO *np, *onp, *newfromnp;
	REGISTER NODEINST *ni;
	REGISTER CHAR *newname;
	CHAR versnum[20];
	REGISTER void *infstr;

	/* must copy subcells */
	for(ni = fromnp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np->primindex != 0) continue;

		/* see if there is already a cell with this name and view */
		for(onp = tolib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
			if (namesame(onp->protoname, np->protoname) == 0 &&
				onp->cellview == np->cellview) break;
		if (onp != NONODEPROTO) continue;

		onp = copyskeleton(np, tolib);
		if (onp == NONODEPROTO)
		{
			ttyputerr(_("Copy of subcell %s failed"), describenodeproto(np));
			return(NONODEPROTO);
		}
	}

	/* copy the cell if it is not already done */
	for(newfromnp = tolib->firstnodeproto; newfromnp != NONODEPROTO; newfromnp = newfromnp->nextnodeproto)
		if (namesame(newfromnp->protoname, fromnp->protoname) == 0 &&
			newfromnp->cellview == fromnp->cellview && newfromnp->version == fromnp->version) break;
	if (newfromnp == NONODEPROTO)
	{
		infstr = initinfstr();
		addstringtoinfstr(infstr, fromnp->protoname);
		addtoinfstr(infstr, ';');
		esnprintf(versnum, 20, x_("%ld"), fromnp->version);
		addstringtoinfstr(infstr, versnum);
		if (fromnp->cellview != el_unknownview)
		{
			addtoinfstr(infstr, '{');
			addstringtoinfstr(infstr, fromnp->cellview->sviewname);
			addtoinfstr(infstr, '}');
		}
		newname = returninfstr(infstr);
		newfromnp = copynodeproto(fromnp, tolib, newname, TRUE);
		if (newfromnp == NONODEPROTO) return(NONODEPROTO);

		/* ensure that the copied cell is the right size */
		(*el_curconstraint->solve)(newfromnp);
	}

	return(newfromnp);
}

NODEPROTO *copyskeleton(NODEPROTO *fromnp, LIBRARY *tolib)
{
	CHAR *newname;
	REGISTER INTBIG newang, newtran;
	REGISTER INTBIG i, xc, yc;
	INTBIG newx, newy;
	XARRAY trans, localtrans, ntrans;
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp, *rpp;
	REGISTER NODEINST *ni, *newni;
	REGISTER void *infstr;

	/* cannot skeletonize text-only views */
	if ((fromnp->cellview->viewstate&TEXTVIEW) != 0) return(NONODEPROTO);

	infstr = initinfstr();
	addstringtoinfstr(infstr, fromnp->protoname);
	if (fromnp->cellview != el_unknownview)
	{
		addtoinfstr(infstr, '{');
		addstringtoinfstr(infstr, fromnp->cellview->sviewname);
		addtoinfstr(infstr, '}');
	}
	newname = returninfstr(infstr);
	np = newnodeproto(newname, tolib);
	if (np == NONODEPROTO) return(NONODEPROTO);

	/* place all exports in the new cell */
	for(pp = fromnp->firstportproto; pp != NOPORTPROTO; pp = pp->nextportproto)
	{
		/* make a transformation matrix for the node that has exports */
		ni = pp->subnodeinst;
		rpp = pp->subportproto;
		newang = ni->rotation;
		newtran = ni->transpose;
		makerot(ni, trans);
		while (ni->proto->primindex == 0)
		{
			maketrans(ni, localtrans);
			transmult(localtrans, trans, ntrans);
			ni = rpp->subnodeinst;
			rpp = rpp->subportproto;
			if (ni->transpose == 0) newang = ni->rotation + newang; else
				newang = ni->rotation + 3600 - newang;
			newtran = (newtran + ni->transpose) & 1;
			makerot(ni, localtrans);
			transmult(localtrans, ntrans, trans);
		}

		/* create this node */
		xc = (ni->lowx + ni->highx) / 2;   yc = (ni->lowy + ni->highy) / 2;
		xform(xc, yc, &newx, &newy, trans);
		newx -= (ni->highx - ni->lowx) / 2;
		newy -= (ni->highy - ni->lowy) / 2;
		newang = newang % 3600;   if (newang < 0) newang += 3600;
		newni = newnodeinst(ni->proto, newx, newx+ni->highx-ni->lowx,
			newy, newy+ni->highy-ni->lowy, newtran, newang, np);
		if (newni == NONODEINST) return(NONODEPROTO);
		endobjectchange((INTBIG)newni, VNODEINST);

		/* export the port from the node */
		(void)newportproto(np, newni, rpp, pp->protoname);
	}

	/* make sure cell is the same size */
	i = (fromnp->highy+fromnp->lowy)/2 - (gen_invispinprim->highy-gen_invispinprim->lowy)/2;
	(void)newnodeinst(gen_invispinprim, fromnp->lowx, fromnp->lowx+gen_invispinprim->highx-gen_invispinprim->lowx,
		i, i+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, np);

	i = (fromnp->highy+fromnp->lowy)/2 - (gen_invispinprim->highy-gen_invispinprim->lowy)/2;
	(void)newnodeinst(gen_invispinprim, fromnp->highx-(gen_invispinprim->highx-gen_invispinprim->lowx), fromnp->highx,
		i, i+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, np);

	i = (fromnp->highx+fromnp->lowx)/2 - (gen_invispinprim->highx-gen_invispinprim->lowx)/2;
	(void)newnodeinst(gen_invispinprim, i, i+gen_invispinprim->highx-gen_invispinprim->lowx,
		fromnp->lowy, fromnp->lowy+gen_invispinprim->highy-gen_invispinprim->lowy, 0, 0, np);

	i = (fromnp->highx+fromnp->lowx)/2 - (gen_invispinprim->highx-gen_invispinprim->lowx)/2;
	(void)newnodeinst(gen_invispinprim, i, i+gen_invispinprim->highx-gen_invispinprim->lowx,
		fromnp->highy-(gen_invispinprim->highy-gen_invispinprim->lowy), fromnp->highy, 0, 0,np);

	(*el_curconstraint->solve)(np);
	return(np);
}

#endif  /* PROJECTTOOL - at top */
