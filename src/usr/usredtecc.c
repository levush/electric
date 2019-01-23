/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usredtecc.c
 * User interface technology editor: interactive technology library editing
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
#include "egraphics.h"
#include "efunction.h"
#include "edialogs.h"
#include "usr.h"
#include "drc.h"
#include "tecgen.h"
#include "tecart.h"
#include "usredtec.h"
#include "usrdiacom.h"

#define MAXNAMELEN 25		/* max chars in a new name */

INTBIG us_teceddrclayers = 0;
CHAR **us_teceddrclayernames = 0;

/* the known technology variables */
TECHVAR us_knownvars[] =
{
	{x_("DRC_ecad_deck"),               NOTECHVAR, 0, 0, 0.0, x_(""), VSTRING|VISARRAY,
		x_("Dracula design-rule deck")},
	{x_("IO_cif_polypoints"),           NOTECHVAR, 0, 0, 0.0, x_(""), VINTEGER,
		x_("Maximum points in a CIF polygon")},
	{x_("IO_cif_resolution"),           NOTECHVAR, 0, 0, 0.0, x_(""), VINTEGER,
		x_("Minimum resolution of CIF coordinates")},
	{x_("IO_gds_polypoints"),           NOTECHVAR, 0, 0, 0.0, x_(""), VINTEGER,
		x_("Maximum points in a GDS-II polygon")},
	{x_("SIM_spice_min_resistance"),    NOTECHVAR, 0, 0, 0.0, x_(""), VFLOAT,
		x_("Minimum resistance of SPICE elements")},
	{x_("SIM_spice_min_capacitance"),   NOTECHVAR, 0, 0, 0.0, x_(""), VFLOAT,
		x_("Minimum capacitance of SPICE elements")},
	{x_("SIM_spice_mask_scale"),        NOTECHVAR, 0, 0, 0.0, x_(""), VFLOAT,
		x_("Scaling factor for SPICE decks")},
	{x_("SIM_spice_header_level1"),     NOTECHVAR, 0, 0, 0.0, x_(""), VSTRING|VISARRAY,
		x_("Level 1 header for SPICE decks")},
	{x_("SIM_spice_header_level2"),     NOTECHVAR, 0, 0, 0.0, x_(""), VSTRING|VISARRAY,
		x_("Level 2 header for SPICE decks")},
	{x_("SIM_spice_header_level3"),     NOTECHVAR, 0, 0, 0.0, x_(""), VSTRING|VISARRAY,
		x_("Level 3 header for SPICE decks")},
	{x_("SIM_spice_model_file"),        NOTECHVAR, 0, 0, 0.0, x_(""), VSTRING,
		x_("Disk file with SPICE header cards")},
	{x_("SIM_spice_trailer_file"),      NOTECHVAR, 0, 0, 0.0, x_(""), VSTRING,
		x_("Disk file with SPICE trailer cards")},
	{NULL, NULL,0, 0, 0.0, NULL, 0, NULL}  /* 0 */
};

/* these must correspond to the layer functions in "efunction.h" */
LIST us_teclayer_functions[] =
{
	{x_("unknown"),           x_("LFUNKNOWN"),     LFUNKNOWN},
	{x_("metal-1"),           x_("LFMETAL1"),      LFMETAL1},
	{x_("metal-2"),           x_("LFMETAL2"),      LFMETAL2},
	{x_("metal-3"),           x_("LFMETAL3"),      LFMETAL3},
	{x_("metal-4"),           x_("LFMETAL4"),      LFMETAL4},
	{x_("metal-5"),           x_("LFMETAL5"),      LFMETAL5},
	{x_("metal-6"),           x_("LFMETAL6"),      LFMETAL6},
	{x_("metal-7"),           x_("LFMETAL7"),      LFMETAL7},
	{x_("metal-8"),           x_("LFMETAL8"),      LFMETAL8},
	{x_("metal-9"),           x_("LFMETAL9"),      LFMETAL9},
	{x_("metal-10"),          x_("LFMETAL10"),     LFMETAL10},
	{x_("metal-11"),          x_("LFMETAL11"),     LFMETAL11},
	{x_("metal-12"),          x_("LFMETAL12"),     LFMETAL12},
	{x_("poly-1"),            x_("LFPOLY1"),       LFPOLY1},
	{x_("poly-2"),            x_("LFPOLY2"),       LFPOLY2},
	{x_("poly-3"),            x_("LFPOLY3"),       LFPOLY3},
	{x_("gate"),              x_("LFGATE"),        LFGATE},
	{x_("diffusion"),         x_("LFDIFF"),        LFDIFF},
	{x_("implant"),           x_("LFIMPLANT"),     LFIMPLANT},
	{x_("contact-1"),         x_("LFCONTACT1"),    LFCONTACT1},
	{x_("contact-2"),         x_("LFCONTACT2"),    LFCONTACT2},
	{x_("contact-3"),         x_("LFCONTACT3"),    LFCONTACT3},
	{x_("contact-4"),         x_("LFCONTACT4"),    LFCONTACT4},
	{x_("contact-5"),         x_("LFCONTACT5"),    LFCONTACT5},
	{x_("contact-6"),         x_("LFCONTACT6"),    LFCONTACT6},
	{x_("contact-7"),         x_("LFCONTACT7"),    LFCONTACT7},
	{x_("contact-8"),         x_("LFCONTACT8"),    LFCONTACT8},
	{x_("contact-9"),         x_("LFCONTACT9"),    LFCONTACT9},
	{x_("contact-10"),        x_("LFCONTACT10"),   LFCONTACT10},
	{x_("contact-11"),        x_("LFCONTACT11"),   LFCONTACT11},
	{x_("contact-12"),        x_("LFCONTACT12"),   LFCONTACT12},
	{x_("plug"),              x_("LFPLUG"),        LFPLUG},
	{x_("overglass"),         x_("LFOVERGLASS"),   LFOVERGLASS},
	{x_("resistor"),          x_("LFRESISTOR"),    LFRESISTOR},
	{x_("capacitor"),         x_("LFCAP"),         LFCAP},
	{x_("transistor"),        x_("LFTRANSISTOR"),  LFTRANSISTOR},
	{x_("emitter"),           x_("LFEMITTER"),     LFEMITTER},
	{x_("base"),              x_("LFBASE"),        LFBASE},
	{x_("collector"),         x_("LFCOLLECTOR"),   LFCOLLECTOR},
	{x_("substrate"),         x_("LFSUBSTRATE"),   LFSUBSTRATE},
	{x_("well"),              x_("LFWELL"),        LFWELL},
	{x_("guard"),             x_("LFGUARD"),       LFGUARD},
	{x_("isolation"),         x_("LFISOLATION"),   LFISOLATION},
	{x_("bus"),               x_("LFBUS"),         LFBUS},
	{x_("art"),               x_("LFART"),         LFART},
	{x_("control"),           x_("LFCONTROL"),     LFCONTROL},

	{x_("p-type"),            x_("LFPTYPE"),       LFPTYPE},
	{x_("n-type"),            x_("LFNTYPE"),       LFNTYPE},
	{x_("depletion"),         x_("LFDEPLETION"),   LFDEPLETION},
	{x_("enhancement"),       x_("LFENHANCEMENT"), LFENHANCEMENT},
	{x_("light"),             x_("LFLIGHT"),       LFLIGHT},
	{x_("heavy"),             x_("LFHEAVY"),       LFHEAVY},
	{x_("pseudo"),            x_("LFPSEUDO"),      LFPSEUDO},
	{x_("nonelectrical"),     x_("LFNONELEC"),     LFNONELEC},
	{x_("connects-metal"),    x_("LFCONMETAL"),    LFCONMETAL},
	{x_("connects-poly"),     x_("LFCONPOLY"),     LFCONPOLY},
	{x_("connects-diff"),     x_("LFCONDIFF"),     LFCONDIFF},
	{x_("inside-transistor"), x_("LFINTRANS"),     LFINTRANS},
	{NULL, NULL, 0}
};

/* these must correspond to the layer functions in "efunction.h" */
LIST us_tecarc_functions[] =
{
	{x_("unknown"),             x_("APUNKNOWN"),  APUNKNOWN},
	{x_("metal-1"),             x_("APMETAL1"),   APMETAL1},
	{x_("metal-2"),             x_("APMETAL2"),   APMETAL2},
	{x_("metal-3"),             x_("APMETAL3"),   APMETAL3},
	{x_("metal-4"),             x_("APMETAL4"),   APMETAL4},
	{x_("metal-5"),             x_("APMETAL5"),   APMETAL5},
	{x_("metal-6"),             x_("APMETAL6"),   APMETAL6},
	{x_("metal-7"),             x_("APMETAL7"),   APMETAL7},
	{x_("metal-8"),             x_("APMETAL8"),   APMETAL8},
	{x_("metal-9"),             x_("APMETAL9"),   APMETAL9},
	{x_("metal-10"),            x_("APMETAL10"),  APMETAL10},
	{x_("metal-11"),            x_("APMETAL11"),  APMETAL11},
	{x_("metal-12"),            x_("APMETAL12"),  APMETAL12},
	{x_("polysilicon-1"),       x_("APPOLY1"),    APPOLY1},
	{x_("polysilicon-2"),       x_("APPOLY2"),    APPOLY2},
	{x_("polysilicon-3"),       x_("APPOLY3"),    APPOLY3},
	{x_("diffusion"),           x_("APDIFF"),     APDIFF},
	{x_("p-Diffusion"),         x_("APDIFFP"),    APDIFFP},
	{x_("n-Diffusion"),         x_("APDIFFN"),    APDIFFN},
	{x_("substrate-Diffusion"), x_("APDIFFS"),    APDIFFS},
	{x_("well-Diffusion"),      x_("APDIFFW"),    APDIFFW},
	{x_("bus"),                 x_("APBUS"),      APBUS},
	{x_("unrouted"),            x_("APUNROUTED"), APUNROUTED},
	{x_("nonelectrical"),       x_("APNONELEC"),  APNONELEC},
	{NULL, NULL, 0}
};

static GRAPHICS us_edtechigh = {LAYERH, HIGHLIT, SOLIDC, SOLIDC,
	{0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,
	0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF}, NOVARIABLE, 0};

/* prototypes for local routines */
static void       us_teceditdrc(void);
static void       us_teceditcolormap(void);
static void       us_teceditcreat(INTBIG, CHAR*[]);
static void       us_teceditidentify(BOOLEAN);
static void       us_teceditinquire(void);
static void       us_teceditmodobject(INTBIG, CHAR*[]);
static void       us_tecedlayer3dheight(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayer3dthick(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerprintcol(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayercolor(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerdrcminwid(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerstyle(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayercif(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerdxf(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayergds(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerspires(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerspicap(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerspiecap(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerfunction(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerletters(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerpatterncontrol(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedlayerpattern(NODEINST*);
static void       us_tecedlayertype(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedmodport(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedarcfunction(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedarcfixang(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedarcwipes(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedarcnoextend(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedarcinc(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecednodefunction(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecednodeserpentine(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecednodesquare(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecednodewipes(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecednodelockable(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecednodemulticut(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedinfolambda(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedinfodescript(NODEINST*, INTBIG, CHAR*[]);
static void       us_tecedsetnode(NODEINST*, CHAR*);
static NODEPROTO *us_tecedentercell(CHAR*);
static void       us_tecedredolayergraphics(NODEPROTO*);
static void       us_tecedloadlibmap(LIBRARY*);
static INTBIG     us_teceditparsefun(CHAR*);
static BOOLEAN    us_tecedgetdrc(CHAR *str, BOOLEAN *connected, BOOLEAN *wide, BOOLEAN *multi,
					INTBIG *widrule, BOOLEAN *edge, INTBIG *amt, INTBIG *layer1, INTBIG *layer2,
					CHAR **rule, INTBIG maxlayers, CHAR **layernames);
static void       us_tecedrenamesequence(CHAR *varname, CHAR *oldname, CHAR *newname);
static void       us_reorderprimdlog(CHAR *type, CHAR *prefix, CHAR *varname);
static NODEINST  *us_tecedlayersetpattern(NODEINST *ni, INTSML color);
static INTSML     us_tecedlayergetpattern(NODEINST *ni);
static void       us_teceditsetlayerpattern(NODEPROTO *np, GRAPHICS *desc);

/*
 * the entry routine for all technology editing
 */
void us_tecedentry(INTBIG count, CHAR *par[])
{
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *lib;
	LIBRARY **dependentlibs;
	NODEPROTO **sequence;
	REGISTER CHAR *pp, **dependentlist;
	CHAR *cellname, *newpar[2];
	UINTSML stip[16];
	REGISTER INTBIG i, l;
	REGISTER INTBIG dependentlibcount;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	if (count == 0)
	{
		ttyputusage(x_("technology edit OPTION"));
		return;
	}

	l = estrlen(pp = par[0]);
	if (namesamen(pp, x_("library-to-tech-and-c"), l) == 0 && l >= 21)
	{
		if (count <= 1) pp = 0; else
			pp = par[1];
		us_tecfromlibinit(el_curlib, pp, 1);
		return;
	}
	if (namesamen(pp, x_("library-to-tech-and-java"), l) == 0 && l >= 21)
	{
		if (count <= 1) pp = 0; else
			pp = par[1];
		us_tecfromlibinit(el_curlib, pp, -1);
		return;
	}
	if (namesamen(pp, x_("library-to-tech"), l) == 0)
	{
		if (count <= 1) pp = 0; else
			pp = par[1];
		us_tecfromlibinit(el_curlib, pp, 0);
		return;
	}
	if (namesamen(pp, x_("tech-to-library"), l) == 0)
	{
		if (count == 1) tech = el_curtech; else
		{
			tech = gettechnology(par[1]);
			if (tech == NOTECHNOLOGY)
			{
				us_abortcommand(_("Technology '%s' unknown"), par[1]);
				return;
			}
		}
		if ((tech->userbits&NONSTANDARD) != 0)
		{
			us_abortcommand(_("Cannot convert technology '%s', it is nonstandard"), tech->techname);
			return;
		}

		/* see if there is already such a library */
		for(lib = el_curlib; lib != NOLIBRARY; lib = lib->nextlibrary)
			if (namesame(lib->libname, tech->techname) == 0) break;
		if (lib != NOLIBRARY)
			ttyputmsg(_("Already a library called %s, using that"), lib->libname); else
				lib = us_tecedmakelibfromtech(tech);
		if (lib != NOLIBRARY)
		{
			newpar[0] = x_("use");
			newpar[1] = lib->libname;
			us_library(2, newpar);
			us_tecedloadlibmap(lib);
		}
		return;
	}
	if (namesamen(pp, x_("reorder-arcs"), l) == 0 && l >= 9)
	{
		us_reorderprimdlog(_("Arcs"), x_("arc-"), x_("EDTEC_arcsequence"));
		return;
	}
	if (namesamen(pp, x_("reorder-nodes"), l) == 0 && l >= 9)
	{
		us_reorderprimdlog(_("Nodes"), x_("node-"), x_("EDTEC_nodesequence"));
		return;
	}
	if (namesamen(pp, x_("reorder-layers"), l) == 0 && l >= 9)
	{
		us_reorderprimdlog(_("Layers"), x_("layer-"), x_("EDTEC_layersequence"));
		return;
	}
	if (namesamen(pp, x_("inquire-layer"), l) == 0 && l >= 2)
	{
		us_teceditinquire();
		return;
	}
	if (namesamen(pp, x_("place-layer"), l) == 0)
	{
		if (count < 2)
		{
			ttyputusage(x_("technology edit place-layer SHAPE"));
			return;
		}

		us_teceditcreat(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("change"), l) == 0 && l >= 2)
	{
		/* in outline edit, create a point */
		if ((el_curwindowpart->state&WINDOWOUTLINEEDMODE) != 0)
		{
			newpar[0] = x_("trace");
			newpar[1] = x_("add-point");
			us_node(2, newpar);
			return;
		}

		us_teceditmodobject(count-1, &par[1]);
		return;
	}
	if (namesamen(pp, x_("edit-node"), l) == 0 && l >= 6)
	{
		if (count < 2)
		{
			ttyputusage(x_("technology edit edit-node NODENAME"));
			return;
		}
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("node-"));
		addstringtoinfstr(infstr, par[1]);
		(void)allocstring(&cellname, returninfstr(infstr), el_tempcluster);

		/* first make sure all fields exist */
		np = getnodeproto(cellname);
		if (np != NONODEPROTO)
		{
			us_tecedmakenode(np, NPUNKNOWN, FALSE, FALSE, FALSE, FALSE, 0);
			(*el_curconstraint->solve)(np);
		}

		np = us_tecedentercell(cellname);
		efree(cellname);
		if (np == NONODEPROTO) return;
		us_tecedmakenode(np, NPUNKNOWN, FALSE, FALSE, FALSE, FALSE, 0);
		(*el_curconstraint->solve)(np);
		np->userbits |= TECEDITCELL;
		(void)us_tecedentercell(describenodeproto(np));
		return;
	}
	if (namesamen(pp, x_("edit-arc"), l) == 0 && l >= 6)
	{
		if (count < 2)
		{
			ttyputusage(x_("technology edit edit-arc ARCNAME"));
			return;
		}
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("arc-"));
		addstringtoinfstr(infstr, par[1]);
		(void)allocstring(&cellname, returninfstr(infstr), el_tempcluster);
		np = us_tecedentercell(cellname);
		efree(cellname);
		if (np == NONODEPROTO) return;
		us_tecedmakearc(np, APUNKNOWN, 1, 1, 0, 90);
		(*el_curconstraint->solve)(np);
		np->userbits |= TECEDITCELL;
		(void)us_tecedentercell(describenodeproto(np));
		return;
	}
	if (namesamen(pp, x_("edit-layer"), l) == 0 && l >= 6)
	{
		if (count < 2)
		{
			ttyputusage(x_("technology edit edit-layer LAYERNAME"));
			return;
		}
		infstr = initinfstr();
		addstringtoinfstr(infstr, x_("layer-"));
		addstringtoinfstr(infstr, par[1]);
		(void)allocstring(&cellname, returninfstr(infstr), el_tempcluster);

		/* first make sure all fields exist */
		for(i=0; i<16; i++) stip[i] = 0;
		np = getnodeproto(cellname);
		if (np != NONODEPROTO)
		{
			us_tecedmakelayer(np, COLORT1, stip, SOLIDC, x_("XX"), LFUNKNOWN, x_("x"), x_(""),
				x_(""), 0.0, 0.0, 0.0, 0, 0, 0);
			(*el_curconstraint->solve)(np);
		}

		np = us_tecedentercell(cellname);
		efree(cellname);
		if (np == NONODEPROTO) return;
		us_tecedmakelayer(np, COLORT1, stip, SOLIDC, x_("XX"), LFUNKNOWN, x_("x"), x_(""),
			x_(""), 0.0, 0.0, 0.0, 0, 0, 0);
		(*el_curconstraint->solve)(np);
		np->userbits |= TECEDITCELL;
		(void)us_tecedentercell(describenodeproto(np));
		return;
	}
	if (namesamen(pp, x_("edit-subsequent"), l) == 0 && l >= 6)
	{
		np = us_needcell();
		if (np == NONODEPROTO) return;
		if (namesamen(np->protoname, x_("node-"), 5) == 0)
		{
			dependentlibcount = us_teceditgetdependents(el_curlib, &dependentlibs);
			i = us_teceditfindsequence(dependentlibs, dependentlibcount, x_("node-"),
				x_("EDTEC_nodesequence"), &sequence);
			if (i == 0) return;
			for(l=0; l<i; l++) if (sequence[l] == np)
			{
				if (l == i-1) np = sequence[0]; else np = sequence[l+1];
				(void)us_tecedentercell(describenodeproto(np));
				break;
			}
			efree((CHAR *)sequence);
			return;
		}
		if (namesamen(np->protoname, x_("arc-"), 4) == 0)
		{
			dependentlibcount = us_teceditgetdependents(el_curlib, &dependentlibs);
			i = us_teceditfindsequence(dependentlibs, dependentlibcount, x_("arc-"),
				x_("EDTEC_arcsequence"), &sequence);
			if (i == 0) return;
			for(l=0; l<i; l++) if (sequence[l] == np)
			{
				if (l == i-1) np = sequence[0]; else np = sequence[l+1];
				(void)us_tecedentercell(describenodeproto(np));
				break;
			}
			efree((CHAR *)sequence);
			return;
		}
		if (namesamen(np->protoname, x_("layer-"), 6) == 0)
		{
			dependentlibcount = us_teceditgetdependents(el_curlib, &dependentlibs);
			i = us_teceditfindsequence(dependentlibs, dependentlibcount, x_("layer-"),
				x_("EDTEC_layersequence"), &sequence);
			if (i == 0) return;
			for(l=0; l<i; l++) if (sequence[l] == np)
			{
				if (l == i-1) np = sequence[0]; else np = sequence[l+1];
				(void)us_tecedentercell(describenodeproto(np));
				break;
			}
			efree((CHAR *)sequence);
			return;
		}
		ttyputerr(_("Must be editing a layer, node, or arc to advance to the next"));
		return;
	}
	if (namesamen(pp, x_("edit-colors"), l) == 0 && l >= 6)
	{
		us_teceditcolormap();
		return;
	}
	if (namesamen(pp, x_("edit-design-rules"), l) == 0 && l >= 6)
	{
		us_teceditdrc();
		return;
	}
	if (namesamen(pp, x_("edit-misc-information"), l) == 0 && l >= 6)
	{
		/* first make sure all fields exist */
		np = getnodeproto(x_("factors"));
		if (np != NONODEPROTO)
		{
			us_tecedmakeinfo(np, 2000, el_curlib->libname);
			(*el_curconstraint->solve)(np);
		}

		/* now edit the cell */
		np = us_tecedentercell(x_("factors"));
		if (np == NONODEPROTO) return;
		us_tecedmakeinfo(np, 2000, el_curlib->libname);
		(*el_curconstraint->solve)(np);
		(void)us_tecedentercell(describenodeproto(np));
		return;
	}
	if (namesamen(pp, x_("identify-layers"), l) == 0 && l >= 10)
	{
		us_teceditidentify(FALSE);
		return;
	}
	if (namesamen(pp, x_("identify-ports"), l) == 0 && l >= 10)
	{
		us_teceditidentify(TRUE);
		return;
	}
	if (namesamen(pp, x_("dependent-libraries"), l) == 0 && l >= 2)
	{
		if (count < 2)
		{
			/* display dependent library names */
			var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_dependent_libraries"));
			if (var == NOVARIABLE) ttyputmsg(_("There are no dependent libraries")); else
			{
				i = getlength(var);
				ttyputmsg(_("%ld dependent %s:"), i, makeplural(x_("library"), i));
				for(l=0; l<i; l++)
				{
					pp = ((CHAR **)var->addr)[l];
					lib = getlibrary(pp);
					ttyputmsg(x_("    %s%s"), pp, (lib == NOLIBRARY ? _(" (not read in)") : x_("")));
				}
			}
			return;
		}

		/* clear list if just "-" is given */
		if (count == 2 && estrcmp(par[1], x_("-")) == 0)
		{
			var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_dependent_libraries"));
			if (var != NOVARIABLE)
				(void)delval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_dependent_libraries"));
			return;
		}

		/* create a list */
		dependentlist = (CHAR **)emalloc((count-1) * (sizeof (CHAR *)), el_tempcluster);
		if (dependentlist == 0) return;
		for(i=1; i<count; i++) dependentlist[i-1] = par[i];
		(void)setval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_dependent_libraries"), (INTBIG)dependentlist,
			VSTRING|VISARRAY|((count-1)<<VLENGTHSH));
		efree((CHAR *)dependentlist);
		return;
	}
	if (namesamen(pp, x_("compact-current-cell"), l) == 0 && l >= 2)
	{
		if (el_curwindowpart == NOWINDOWPART) np = NONODEPROTO; else
			np = el_curwindowpart->curnodeproto;
		if (np != NONODEPROTO) us_tecedcompact(np); else
			ttyputmsg(_("No current cell to compact"));
		return;
	}
	ttyputbadusage(x_("technology edit"));
}

/*
 * Routine to compact the current technology-edit cell
 */
void us_tecedcompact(NODEPROTO *cell)
{
	REGISTER EXAMPLE *nelist, *ne;
	REGISTER SAMPLE *ns;
	REGISTER BOOLEAN first;
	REGISTER INTBIG i, numexamples, xoff, yoff, examplenum, leftheight, height,
		lx, hx, ly, hy, topy, separation;
	REGISTER NODEINST *ni;

	if (namesame(cell->protoname, x_("factors")) == 0)
	{
		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* move the option text */
		us_tecedfindspecialtext(cell, us_tecedmisctexttable);
		for(i=0; us_tecedmisctexttable[i].funct != 0; i++)
		{
			ni = us_tecedmisctexttable[i].ni;
			if (ni == NONODEINST) continue;
			xoff = us_tecedmisctexttable[i].x - (ni->lowx + ni->highx) / 2;
			yoff = us_tecedmisctexttable[i].y - (ni->lowy + ni->highy) / 2;
			if (xoff == 0 && yoff == 0) continue;
			startobjectchange((INTBIG)ni, VNODEINST);
			modifynodeinst(ni, xoff, yoff, xoff, yoff, 0, 0);
			endobjectchange((INTBIG)ni, VNODEINST);
		}
		us_pophighlight(FALSE);
		return;
	}
	if (namesamen(cell->protoname, x_("layer-"), 6) == 0)
	{
		ttyputmsg("Cannot compact technology-edit layer cells");
		return;
	}
	if (namesamen(cell->protoname, x_("arc-"), 4) == 0)
	{
		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* move the option text */
		us_tecedfindspecialtext(cell, us_tecedarctexttable);
		for(i=0; us_tecedarctexttable[i].funct != 0; i++)
		{
			ni = us_tecedarctexttable[i].ni;
			if (ni == NONODEINST) continue;
			xoff = us_tecedarctexttable[i].x - (ni->lowx + ni->highx) / 2;
			yoff = us_tecedarctexttable[i].y - (ni->lowy + ni->highy) / 2;
			if (xoff == 0 && yoff == 0) continue;
			startobjectchange((INTBIG)ni, VNODEINST);
			modifynodeinst(ni, xoff, yoff, xoff, yoff, 0, 0);
			endobjectchange((INTBIG)ni, VNODEINST);
		}

		/* compute bounds of arc contents */
		first = TRUE;
		for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			/* ignore the special text nodes */
			for(i=0; us_tecedarctexttable[i].funct != 0; i++)
				if (us_tecedarctexttable[i].ni == ni) break;
			if (us_tecedarctexttable[i].funct != 0) continue;

			if (first)
			{
				first = FALSE;
				lx = ni->lowx;   hx = ni->highx;
				ly = ni->lowy;   hy = ni->highy;
			} else
			{
				if (ni->lowx < lx) lx = ni->lowx;
				if (ni->highx > hx) hx = ni->highx;
				if (ni->lowy < ly) ly = ni->lowy;
				if (ni->highy > hy) hy = ni->highy;
			}
		}

		/* now rearrange the geometry */
		if (!first)
		{
			xoff = -(lx + hx) / 2;
			yoff = -hy;
			if (xoff != 0 || yoff != 0)
			{
				for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					/* ignore the special text nodes */
					for(i=0; us_tecedarctexttable[i].funct != 0; i++)
						if (us_tecedarctexttable[i].ni == ni) break;
					if (us_tecedarctexttable[i].funct != 0) continue;

					startobjectchange((INTBIG)ni, VNODEINST);
					modifynodeinst(ni, xoff, yoff, xoff, yoff, 0, 0);
					endobjectchange((INTBIG)ni, VNODEINST);
				}
			}
		}
		us_pophighlight(FALSE);
		return;
	}
	if (namesamen(cell->protoname, x_("node-"), 5) == 0)
	{
		/* save highlighting */
		us_pushhighlight();
		us_clearhighlightcount();

		/* move the option text */
		us_tecedfindspecialtext(cell, us_tecednodetexttable);
		for(i=0; us_tecednodetexttable[i].funct != 0; i++)
		{
			ni = us_tecednodetexttable[i].ni;
			if (ni == NONODEINST) continue;
			xoff = us_tecednodetexttable[i].x - (ni->lowx + ni->highx) / 2;
			yoff = us_tecednodetexttable[i].y - (ni->lowy + ni->highy) / 2;
			if (xoff == 0 && yoff == 0) continue;
			startobjectchange((INTBIG)ni, VNODEINST);
			modifynodeinst(ni, xoff, yoff, xoff, yoff, 0, 0);
			endobjectchange((INTBIG)ni, VNODEINST);
		}

		/* move the examples */
		nelist = us_tecedgetexamples(cell, TRUE);
		if (nelist == NOEXAMPLE) return;
		numexamples = 0;
		for(ne = nelist; ne != NOEXAMPLE; ne = ne->nextexample) numexamples++;
		examplenum = 0;
		topy = 0;
		separation = mini(nelist->hx - nelist->lx, nelist->hy - nelist->ly);
		for(ne = nelist; ne != NOEXAMPLE; ne = ne->nextexample)
		{
			/* handle left or right side */
			yoff = topy - ne->hy;
			if ((examplenum&1) == 0)
			{
				/* do left side */
				if (examplenum == numexamples-1)
				{
					/* last one is centered */
					xoff = -(ne->lx + ne->hx) / 2;
				} else
				{
					xoff = -ne->hx - separation/2;
				}
				leftheight = ne->hy - ne->ly;
			} else
			{
				/* do right side */
				xoff = -ne->lx + separation/2;
				height = ne->hy - ne->ly;
				if (leftheight > height) height = leftheight;
				topy -= height + separation;
			}
			examplenum++;

			/* adjust every node in the example */
			if (xoff == 0 && yoff == 0) continue;
			for(ns = ne->firstsample; ns != NOSAMPLE; ns = ns->nextsample)
			{
				ni = ns->node;
				startobjectchange((INTBIG)ni, VNODEINST);
				modifynodeinst(ni, xoff, yoff, xoff, yoff, 0, 0);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
		us_tecedfreeexamples(nelist);
		us_pophighlight(FALSE);
		return;
	}
	ttyputmsg(_("Cannot compact technology-edit cell %s: unknown type"),
		describenodeproto(cell));
}

/*
 * Routine for editing the DRC tables.
 */
void us_teceditdrc(void)
{
	REGISTER INTBIG i, changed, nodecount;
	NODEPROTO **nodesequence;
	LIBRARY *liblist[1];
	REGISTER VARIABLE *var;
	REGISTER DRCRULES *rules;

	/* get the current list of layer and node names */
	us_tecedgetlayernamelist();
	liblist[0] = el_curlib;
	nodecount = us_teceditfindsequence(liblist, 1, x_("node-"), x_("EDTEC_nodesequence"), &nodesequence);

	/* create a RULES structure */
	rules = dr_allocaterules(us_teceddrclayers, nodecount, x_("EDITED TECHNOLOGY"));
	if (rules == NODRCRULES) return;
	for(i=0; i<us_teceddrclayers; i++)
		(void)allocstring(&rules->layernames[i], us_teceddrclayernames[i], el_tempcluster);
	for(i=0; i<nodecount; i++)
		(void)allocstring(&rules->nodenames[i], &nodesequence[i]->protoname[5], el_tempcluster);
	if (nodecount > 0) efree((CHAR *)nodesequence);

	/* get the text-list of design rules and convert them into arrays */
	var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_DRC"));
	us_teceditgetdrcarrays(var, rules);

	/* edit the design-rule arrays */
	changed = dr_rulesdlog(NOTECHNOLOGY, rules);

	/* if changes were made, convert the arrays back into a text-list */
	if (changed != 0)
	{
		us_tecedloaddrcmessage(rules, el_curlib);
	}

	/* free the arrays */
	dr_freerules(rules);
}

/*
 * Routine to update tables to reflect that cell "oldname" is now called "newname".
 * If "newname" is not valid, any rule that refers to "oldname" is removed.
 * 
 */
void us_tecedrenamecell(CHAR *oldname, CHAR *newname)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i, len;
	REGISTER BOOLEAN valid;
	INTBIG count;
	REGISTER CHAR *origstr, *firstkeyword, *keyword;
	CHAR *str, **strings;
	REGISTER void *infstr, *sa;

	/* if this is a layer, rename the layer sequence array */
	if (namesamen(oldname, x_("layer-"), 6) == 0 && namesamen(newname, x_("layer-"), 6) == 0)
	{
		us_tecedrenamesequence(x_("EDTEC_layersequence"), &oldname[6], &newname[6]);
	}

	/* if this is an arc, rename the arc sequence array */
	if (namesamen(oldname, x_("arc-"), 4) == 0 && namesamen(newname, x_("arc-"), 4) == 0)
	{
		us_tecedrenamesequence(x_("EDTEC_arcsequence"), &oldname[4], &newname[4]);
	}

	/* if this is a node, rename the node sequence array */
	if (namesamen(oldname, x_("node-"), 5) == 0 && namesamen(newname, x_("node-"), 5) == 0)
	{
		us_tecedrenamesequence(x_("EDTEC_nodesequence"), &oldname[5], &newname[5]);
	}

	/* see if there are design rules in the current library */
	var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_DRC"));
	if (var == NOVARIABLE) return;

	/* examine the rules and convert the name */
	len = getlength(var);
	sa = newstringarray(us_tool->cluster);
	for(i=0; i<len; i++)
	{
		/* parse the DRC rule */
		str = ((CHAR **)var->addr)[i];
		origstr = str;
		firstkeyword = getkeyword(&str, x_(" "));
		if (firstkeyword == NOSTRING) return;

		/* pass wide wire limitation through */
		if (*firstkeyword == 'l')
		{
			addtostringarray(sa, origstr);
			continue;
		}

		/* rename nodes in the minimum node size rule */
		if (*firstkeyword == 'n')
		{
			if (namesamen(oldname, x_("node-"), 5) == 0 &&
				namesame(&oldname[5], &firstkeyword[1]) == 0)
			{
				/* substitute the new name */
				if (namesamen(newname, x_("node-"), 5) == 0)
				{
					infstr = initinfstr();
					addstringtoinfstr(infstr, x_("n"));
					addstringtoinfstr(infstr, &newname[5]);
					addstringtoinfstr(infstr, str);
					addtostringarray(sa, returninfstr(infstr));
				}
				continue;
			}
			addtostringarray(sa, origstr);
			continue;
		}

		/* rename layers in the minimum layer size rule */
		if (*firstkeyword == 's')
		{
			valid = TRUE;
			infstr = initinfstr();
			formatinfstr(infstr, x_("%s "), firstkeyword);
			keyword = getkeyword(&str, x_(" "));
			if (keyword == NOSTRING) return;
			if (namesamen(oldname, x_("layer-"), 6) == 0 &&
				namesame(&oldname[6], keyword) == 0)
			{
				if (namesamen(newname, x_("layer-"), 6) != 0) valid = FALSE; else
					addstringtoinfstr(infstr, &newname[6]);
			} else
				addstringtoinfstr(infstr, keyword);
			addstringtoinfstr(infstr, str);
			str = returninfstr(infstr);
			if (valid) addtostringarray(sa, str);
			continue;
		}

		/* layer width rule: substitute layer names */
		infstr = initinfstr();
		formatinfstr(infstr, x_("%s "), firstkeyword);
		valid = TRUE;

		/* get the first layer name and convert it */
		keyword = getkeyword(&str, x_(" "));
		if (keyword == NOSTRING) return;
		if (namesamen(oldname, x_("layer-"), 6) == 0 &&
			namesame(&oldname[6], keyword) == 0)
		{
			/* substitute the new name */
			if (namesamen(newname, x_("layer-"), 6) != 0) valid = FALSE; else
				addstringtoinfstr(infstr, &newname[6]);
		} else
			addstringtoinfstr(infstr, keyword);
		addtoinfstr(infstr, ' ');

		/* get the second layer name and convert it */
		keyword = getkeyword(&str, x_(" "));
		if (keyword == NOSTRING) return;
		if (namesamen(oldname, x_("layer-"), 6) == 0 &&
			namesame(&oldname[6], keyword) == 0)
		{
			/* substitute the new name */
			if (namesamen(newname, x_("layer-"), 6) != 0) valid = FALSE; else
				addstringtoinfstr(infstr, &newname[6]);
		} else
			addstringtoinfstr(infstr, keyword);

		addstringtoinfstr(infstr, str);
		str = returninfstr(infstr);
		if (valid) addtostringarray(sa, str);
	}
	strings = getstringarray(sa, &count);
	setval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_DRC"), (INTBIG)strings,
		VSTRING|VISARRAY|(count<<VLENGTHSH));
	killstringarray(sa);
}

/*
 * Routine to rename the layer/arc/node sequence arrays to account for a name change.
 * The sequence array is in variable "varname", and the item has changed from "oldname" to
 * "newname".
 */
void us_tecedrenamesequence(CHAR *varname, CHAR *oldname, CHAR *newname)
{
	REGISTER VARIABLE *var;
	CHAR **strings;
	REGISTER INTBIG i, len;
	INTBIG count;
	void *sa;

	var = getval((INTBIG)el_curlib, VLIBRARY, VSTRING|VISARRAY, varname);
	if (var == NOVARIABLE) return;

	strings = (CHAR **)var->addr;
	len = getlength(var);
	sa = newstringarray(us_tool->cluster);
	for(i=0; i<len; i++)
	{
		if (namesame(strings[i], oldname) == 0)
			addtostringarray(sa, newname); else
				addtostringarray(sa, strings[i]);
	}
	strings = getstringarray(sa, &count);
	setval((INTBIG)el_curlib, VLIBRARY, varname, (INTBIG)strings,
		VSTRING|VISARRAY|(count<<VLENGTHSH));
	killstringarray(sa);
}

void us_tecedgetlayernamelist(void)
{
	REGISTER INTBIG i, dependentlibcount;
	REGISTER NODEPROTO *np;
	NODEPROTO **sequence;
	LIBRARY **dependentlibs;

	/* free any former layer name information */
	if (us_teceddrclayernames != 0)
	{
		for(i=0; i<us_teceddrclayers; i++) efree(us_teceddrclayernames[i]);
		efree((CHAR *)us_teceddrclayernames);
		us_teceddrclayernames = 0;
	}

	dependentlibcount = us_teceditgetdependents(el_curlib, &dependentlibs);
	us_teceddrclayers = us_teceditfindsequence(dependentlibs, dependentlibcount, x_("layer-"),
		x_("EDTEC_layersequence"), &sequence);

	/* build and fill array of layers for DRC parsing */
	us_teceddrclayernames = (CHAR **)emalloc(us_teceddrclayers * (sizeof (CHAR *)), us_tool->cluster);
	if (us_teceddrclayernames == 0) return;
	for(i = 0; i<us_teceddrclayers; i++)
	{
		np = sequence[i];
		(void)allocstring(&us_teceddrclayernames[i], &np->protoname[6], us_tool->cluster);
	}
}

/*
 * Routine to create arrays describing the design rules in the variable "var" (which is
 * from "EDTEC_DRC" on a library).  The arrays are stored in "rules".
 */
void us_teceditgetdrcarrays(VARIABLE *var, DRCRULES *rules)
{
	REGISTER INTBIG i, l;
	INTBIG amt;
	BOOLEAN connected, wide, multi, edge;
	INTBIG widrule, layer1, layer2, j;
	REGISTER CHAR *str, *pt;
	CHAR *rule;

	/* get the design rules */
	if (var == NOVARIABLE) return;

	l = getlength(var);
	for(i=0; i<l; i++)
	{
		/* parse the DRC rule */
		str = ((CHAR **)var->addr)[i];
		while (*str == ' ') str++;
		if (*str == 0) continue;

		/* special case for node minimum size rule */
		if (*str == 'n')
		{
			str++;
			for(pt = str; *pt != 0; pt++) if (*pt == ' ') break;
			if (*pt == 0)
			{
				ttyputmsg(_("Bad node size rule (line %ld): %s"), i+1, str);
				continue;
			}
			*pt = 0;
			for(j=0; j<rules->numnodes; j++)
				if (namesame(str, rules->nodenames[j]) == 0) break;
			*pt = ' ';
			if (j >= rules->numnodes)
			{
				ttyputmsg(_("Unknown node (line %ld): %s"), i+1, str);
				continue;
			}
			while (*pt == ' ') pt++;
			rules->minnodesize[j*2] = atofr(pt);
			while (*pt != 0 && *pt != ' ') pt++;
			while (*pt == ' ') pt++;
			rules->minnodesize[j*2+1] = atofr(pt);
			while (*pt != 0 && *pt != ' ') pt++;
			while (*pt == ' ') pt++;
			if (*pt != 0) reallocstring(&rules->minnodesizeR[j], pt, el_tempcluster);
			continue;
		}

		/* parse the layer rule */
		if (us_tecedgetdrc(str, &connected, &wide, &multi, &widrule, &edge,
			&amt, &layer1, &layer2, &rule, rules->numlayers, rules->layernames))
		{
			ttyputmsg(_("DRC line %ld is: %s"), i+1, str);
			continue;
		}

		/* set the layer spacing */
		if (widrule == 1)
		{
			rules->minwidth[layer1] = amt;
			if (*rule != 0)
				(void)reallocstring(&rules->minwidthR[layer1], rule, el_tempcluster);
		} else if (widrule == 2)
		{
			rules->widelimit = amt;
		} else
		{
			if (layer1 > layer2) { j = layer1;  layer1 = layer2;  layer2 = j; }
			j = (layer1+1) * (layer1/2) + (layer1&1) * ((layer1+1)/2);
			j = layer2 + rules->numlayers * layer1 - j;
			if (edge)
			{
				rules->edgelist[j] = amt;
				if (*rule != 0)
					(void)reallocstring(&rules->edgelistR[j], rule, el_tempcluster);
			} else if (wide)
			{
				if (connected)
				{
					rules->conlistW[j] = amt;
					if (*rule != 0)
						(void)reallocstring(&rules->conlistWR[j], rule, el_tempcluster);
				} else
				{
					rules->unconlistW[j] = amt;
					if (*rule != 0)
						(void)reallocstring(&rules->unconlistWR[j], rule, el_tempcluster);
				}
			} else if (multi)
			{
				if (connected)
				{
					rules->conlistM[j] = amt;
					if (*rule != 0)
						(void)reallocstring(&rules->conlistMR[j], rule, el_tempcluster);
				} else
				{
					rules->unconlistM[j] = amt;
					if (*rule != 0)
						(void)reallocstring(&rules->unconlistMR[j], rule, el_tempcluster);
				}
			} else
			{
				if (connected)
				{
					rules->conlist[j] = amt;
					if (*rule != 0)
						(void)reallocstring(&rules->conlistR[j], rule, el_tempcluster);
				} else
				{
					rules->unconlist[j] = amt;
					if (*rule != 0)
						(void)reallocstring(&rules->unconlistR[j], rule, el_tempcluster);
				}
			}
		}
	}
}

/*
 * routine to parse DRC line "str" and fill the factors "connected" (set nonzero
 * if rule is for connected layers), "amt" (rule distance), "layer1" and "layer2"
 * (the layers).  Presumes that there are "maxlayers" layer names in the
 * array "layernames".  Returns true on error.
 */
BOOLEAN us_tecedgetdrc(CHAR *str, BOOLEAN *connected, BOOLEAN *wide, BOOLEAN *multi, INTBIG *widrule,
	BOOLEAN *edge, INTBIG *amt, INTBIG *layer1, INTBIG *layer2, CHAR **rule, INTBIG maxlayers,
	CHAR **layernames)
{
	REGISTER CHAR *pt;
	REGISTER INTBIG save;

	*connected = *wide = *multi = *edge = FALSE;
	for( ; *str != 0; str++)
	{
		if (tolower(*str) == 'c')
		{
			*connected = TRUE;
			continue;
		}
		if (tolower(*str) == 'w')
		{
			*wide = TRUE;
			continue;
		}
		if (tolower(*str) == 'm')
		{
			*multi = TRUE;
			continue;
		}
		if (tolower(*str) == 'e')
		{
			*edge = TRUE;
			continue;
		}
		break;
	}
	*widrule = 0;
	if (tolower(*str) == 's')
	{
		*widrule = 1;
		str++;
	} else if (tolower(*str) == 'l')
	{
		*widrule = 2;
		str++;
	}

	/* get the distance */
	pt = str;
	while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
	while (*pt == ' ' || *pt == '\t') pt++;
	*amt = atofr(str);

	/* get the first layer */
	if (*widrule != 2)
	{
		str = pt;
		if (*str == 0)
		{
			ttyputerr(_("Cannot find layer names on DRC line"));
			return(TRUE);
		}
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		if (*pt == 0)
		{
			ttyputerr(_("Cannot find layer name on DRC line"));
			return(TRUE);
		}
		save = *pt;
		*pt = 0;
		for(*layer1 = 0; *layer1 < maxlayers; (*layer1)++)
			if (namesame(str, layernames[*layer1]) == 0) break;
		*pt++ = (CHAR)save;
		if (*layer1 >= maxlayers)
		{
			ttyputerr(_("First DRC layer name unknown"));
			return(TRUE);
		}
		while (*pt == ' ' || *pt == '\t') pt++;
	}

	/* get the second layer */
	if (*widrule == 0)
	{
		str = pt;
		while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
		save = *pt;
		*pt = 0;
		for(*layer2 = 0; *layer2 < maxlayers; (*layer2)++)
			if (namesame(str, layernames[*layer2]) == 0) break;
		*pt = (CHAR)save;
		if (*layer2 >= maxlayers)
		{
			ttyputerr(_("Second DRC layer name unknown"));
			return(TRUE);
		}
	}

	while (*pt == ' ' || *pt == '\t') pt++;
	*rule = pt;
	return(FALSE);
}

/*
 * Helper routine to examine the arrays describing the design rules and create
 * the variable "EDTEC_DRC" on library "lib".
 */
void us_tecedloaddrcmessage(DRCRULES *rules, LIBRARY *lib)
{
	REGISTER INTBIG drccount, drcindex, i, k, j;
	REGISTER CHAR **drclist;
	REGISTER void *infstr;

	/* determine the number of lines in the text-version of the design rules */
	drccount = 0;
	for(i=0; i<rules->utsize; i++)
	{
		if (rules->conlist[i] >= 0) drccount++;
		if (rules->unconlist[i] >= 0) drccount++;
		if (rules->conlistW[i] >= 0) drccount++;
		if (rules->unconlistW[i] >= 0) drccount++;
		if (rules->conlistM[i] >= 0) drccount++;
		if (rules->unconlistM[i] >= 0) drccount++;
		if (rules->edgelist[i] >= 0) drccount++;
	}
	for(i=0; i<rules->numlayers; i++)
	{
		if (rules->minwidth[i] >= 0) drccount++;
	}
	for(i=0; i<rules->numnodes; i++)
	{
		if (rules->minnodesize[i*2] > 0 || rules->minnodesize[i*2+1] > 0) drccount++;
	}

	/* load the arrays */
	if (drccount != 0)
	{
		drccount++;
		drclist = (CHAR **)emalloc((drccount * (sizeof (CHAR *))), el_tempcluster);
		if (drclist == 0) return;
		drcindex = 0;

		/* write the width limit */
		infstr = initinfstr();
		formatinfstr(infstr, x_("l%s"), frtoa(rules->widelimit));
		(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);

		/* write the minimum width for each layer */
		for(i=0; i<rules->numlayers; i++)
		{
			if (rules->minwidth[i] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("s%s %s %s"), frtoa(rules->minwidth[i]),
					rules->layernames[i], rules->minwidthR[i]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
		}

		/* write the minimum size for each node */
		for(i=0; i<rules->numnodes; i++)
		{
			if (rules->minnodesize[i*2] <= 0 && rules->minnodesize[i*2+1] <= 0) continue;
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("n%s %s %s %s"), rules->nodenames[i],
					frtoa(rules->minnodesize[i*2]), frtoa(rules->minnodesize[i*2+1]),
					rules->minnodesizeR[i]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
		}

		/* now do the distance rules */
		k = 0;
		for(i=0; i<rules->numlayers; i++) for(j=i; j<rules->numlayers; j++)
		{
			if (rules->conlist[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("c%s %s %s %s"), frtoa(rules->conlist[k]),
					rules->layernames[i], rules->layernames[j],
						rules->conlistR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			if (rules->unconlist[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("%s %s %s %s"), frtoa(rules->unconlist[k]),
					rules->layernames[i], rules->layernames[j],
						rules->unconlistR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			if (rules->conlistW[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("cw%s %s %s %s"), frtoa(rules->conlistW[k]),
					rules->layernames[i], rules->layernames[j],
						rules->conlistWR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			if (rules->unconlistW[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("w%s %s %s %s"), frtoa(rules->unconlistW[k]),
					rules->layernames[i], rules->layernames[j],
						rules->unconlistWR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			if (rules->conlistM[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("cm%s %s %s %s"), frtoa(rules->conlistM[k]),
					rules->layernames[i], rules->layernames[j],
						rules->conlistMR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			if (rules->unconlistM[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("m%s %s %s %s"), frtoa(rules->unconlistM[k]),
					rules->layernames[i], rules->layernames[j],
						rules->unconlistMR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			if (rules->edgelist[k] >= 0)
			{
				infstr = initinfstr();
				formatinfstr(infstr, x_("e%s %s %s %s"), frtoa(rules->edgelist[k]),
					rules->layernames[i], rules->layernames[j],
						rules->edgelistR[k]);
				(void)allocstring(&drclist[drcindex++], returninfstr(infstr), el_tempcluster);
			}
			k++;
		}

		(void)setval((INTBIG)lib, VLIBRARY, x_("EDTEC_DRC"), (INTBIG)drclist,
			VSTRING|VISARRAY|(drccount<<VLENGTHSH));
		for(i=0; i<drccount; i++) efree(drclist[i]);
		efree((CHAR *)drclist);
	} else
	{
		/* no rules: remove the variable */
		if (getval((INTBIG)lib, VLIBRARY, VSTRING|VISARRAY, x_("EDTEC_DRC")) != NOVARIABLE)
			(void)delval((INTBIG)lib, VLIBRARY, x_("EDTEC_DRC"));
	}
}

/*
 * routine for manipulating color maps
 */
void us_teceditcolormap(void)
{
	REGISTER INTBIG i, k, total, dependentlibcount, *printcolors;
	INTBIG func, drcminwid, height3d, thick3d, printcol[5];
	CHAR *layerlabel[5], *layerabbrev[5], *cif, *gds, *layerletters, *dxf, **layernames, line[50];
	LIBRARY **dependentlibs;
	GRAPHICS desc;
	NODEPROTO **sequence;
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;
	REGISTER VARIABLE *var;
	float spires, spicap, spiecap;
	REGISTER INTBIG *mapptr, *newmap;
	REGISTER VARIABLE *varred, *vargreen, *varblue;

	if (us_needwindow()) return;

	/* load the color map of the technology */
	us_tecedloadlibmap(el_curlib);

	dependentlibcount = us_teceditgetdependents(el_curlib, &dependentlibs);
	total = us_teceditfindsequence(dependentlibs, dependentlibcount, x_("layer-"),
		x_("EDTEC_layersequence"), &sequence);
	printcolors = (INTBIG *)emalloc(total*5*SIZEOFINTBIG, us_tool->cluster);
	if (printcolors == 0) return;
	layernames = (CHAR **)emalloc(total * (sizeof (CHAR *)), us_tool->cluster);
	if (layernames == 0) return;

	/* now fill in real layer names if known */
	for(i=0; i<5; i++) layerlabel[i] = 0;
	for(i=0; i<total; i++)
	{
		np = sequence[i];
		cif = layerletters = gds = 0;
		if (us_teceditgetlayerinfo(np, &desc, &cif, &func, &layerletters,
			&dxf, &gds, &spires, &spicap, &spiecap, &drcminwid, &height3d,
				&thick3d, printcol)) return;
		for(k=0; k<5; k++) printcolors[i*5+k] = printcol[k];
		layernames[i] = &np->protoname[6];
		switch (desc.bits)
		{
			case LAYERT1: k = 0;   break;
			case LAYERT2: k = 1;   break;
			case LAYERT3: k = 2;   break;
			case LAYERT4: k = 3;   break;
			case LAYERT5: k = 4;   break;
			default:      k = -1;  break;
		}
		if (k >= 0)
		{
			if (layerlabel[k] == 0)
			{
				layerlabel[k] = &np->protoname[6];
				layerabbrev[k] = (CHAR *)emalloc(2 * SIZEOFCHAR, el_tempcluster);
				layerabbrev[k][0] = *layerletters;
				layerabbrev[k][1] = 0;
			}
		}
		if (gds != 0) efree(gds);
		if (cif != 0) efree(cif);
		if (layerletters != 0) efree(layerletters);
	}

	/* set defaults */
	if (layerlabel[0] == 0)
	{
		layerlabel[0] = _("Ovrlap 1");
		(void)allocstring(&layerabbrev[0], x_("1"), el_tempcluster);
	}
	if (layerlabel[1] == 0)
	{
		layerlabel[1] = _("Ovrlap 2");
		(void)allocstring(&layerabbrev[1], x_("2"), el_tempcluster);
	}
	if (layerlabel[2] == 0)
	{
		layerlabel[2] = _("Ovrlap 3");
		(void)allocstring(&layerabbrev[2], x_("3"), el_tempcluster);
	}
	if (layerlabel[3] == 0)
	{
		layerlabel[3] = _("Ovrlap 4");
		(void)allocstring(&layerabbrev[3], x_("4"), el_tempcluster);
	}
	if (layerlabel[4] == 0)
	{
		layerlabel[4] = _("Ovrlap 5");
		(void)allocstring(&layerabbrev[4], x_("5"), el_tempcluster);
	}

	/* run the color mixing palette */
	if (us_colormixdlog(layerlabel, total, layernames, printcolors))
	{
		/* update all of the layer cells */
		for(i=0; i<total; i++)
		{
			np = sequence[i];
			for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
			{
				var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
				if (var == NOVARIABLE) continue;
				if (var->addr != LAYERPRINTCOL) continue;
				(void)esnprintf(line, 50, x_("%s%ld,%ld,%ld, %ld,%s"), TECEDNODETEXTPRINTCOL,
					printcolors[i*5], printcolors[i*5+1], printcolors[i*5+2],
						printcolors[i*5+3], (printcolors[i*5+4]==0 ? x_("off") : x_("on")));
				startobjectchange((INTBIG)ni, VNODEINST);
				(void)setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)line,
					VSTRING|VDISPLAY);
				endobjectchange((INTBIG)ni, VNODEINST);
			}
		}
	}
	for(i=0; i<5; i++) efree(layerabbrev[i]);

	/* save the map on the library */
	varred = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_red_key);
	vargreen = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_green_key);
	varblue = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER|VISARRAY, us_colormap_blue_key);
	if (varred != NOVARIABLE && vargreen != NOVARIABLE && varblue != NOVARIABLE)
	{
		newmap = emalloc(256*3*SIZEOFINTBIG, el_tempcluster);
		mapptr = newmap;
		for(i=0; i<256; i++)
		{
			*mapptr++ = ((INTBIG *)varred->addr)[i];
			*mapptr++ = ((INTBIG *)vargreen->addr)[i];
			*mapptr++ = ((INTBIG *)varblue->addr)[i];
		}
		(void)setval((INTBIG)el_curlib, VLIBRARY, x_("EDTEC_colormap"), (INTBIG)newmap,
			VINTEGER|VISARRAY|((256*3)<<VLENGTHSH));
		efree((CHAR *)newmap);
	}
	efree((CHAR *)layernames);
	efree((CHAR *)printcolors);
}

/*
 * routine for creating a new layer with shape "pp"
 */
void us_teceditcreat(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG l;
	REGISTER CHAR *name, *pp;
	CHAR *subpar[3];
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np, *savenp, *cell;
	HIGHLIGHT high;
	REGISTER VARIABLE *var;

	l = estrlen(pp = par[0]);
	np = NONODEPROTO;
	if (namesamen(pp, x_("port"), l) == 0 && l >= 1) np = gen_portprim;
	if (namesamen(pp, x_("highlight"), l) == 0 && l >= 1) np = art_boxprim;
	if (namesamen(pp, x_("rectangle-filled"), l) == 0 && l >= 11) np = art_filledboxprim;
	if (namesamen(pp, x_("rectangle-outline"), l) == 0 && l >= 11) np = art_boxprim;
	if (namesamen(pp, x_("rectangle-crossed"), l) == 0 && l >= 11) np = art_crossedboxprim;
	if (namesamen(pp, x_("polygon-filled"), l) == 0 && l >= 9) np = art_filledpolygonprim;
	if (namesamen(pp, x_("polygon-outline"), l) == 0 && l >= 9) np = art_closedpolygonprim;
	if (namesamen(pp, x_("lines-solid"), l) == 0 && l >= 7) np = art_openedpolygonprim;
	if (namesamen(pp, x_("lines-dotted"), l) == 0 && l >= 8) np = art_openeddottedpolygonprim;
	if (namesamen(pp, x_("lines-dashed"), l) == 0 && l >= 8) np = art_openeddashedpolygonprim;
	if (namesamen(pp, x_("lines-thicker"), l) == 0 && l >= 7) np = art_openedthickerpolygonprim;
	if (namesamen(pp, x_("circle-outline"), l) == 0 && l >= 8) np = art_circleprim;
	if (namesamen(pp, x_("circle-filled"), l) == 0 && l >= 8) np = art_filledcircleprim;
	if (namesamen(pp, x_("circle-half"), l) == 0 && l >= 8) np = art_circleprim;
	if (namesamen(pp, x_("circle-arc"), l) == 0 && l >= 8) np = art_circleprim;
	if (namesamen(pp, x_("text"), l) == 0 && l >= 1) np = gen_invispinprim;

	if (np == NONODEPROTO)
	{
		ttyputerr(_("Unrecoginzed shape: '%s'"), pp);
		return;
	}

	/* make sure the cell is right */
	cell = us_needcell();
	if (cell == NONODEPROTO) return;
	if (namesamen(cell->protoname, x_("node-"), 5) != 0 &&
		namesamen(cell->protoname, x_("arc-"), 4) != 0)
	{
		us_abortcommand(_("Must be editing a node or arc to place geometry"));
		if ((us_tool->toolstate&NODETAILS) == 0)
			ttyputmsg(_("Use 'edit-node' or 'edit-arc' options"));
		return;
	}
	if (np == gen_portprim &&
		namesamen(cell->protoname, x_("node-"), 5) != 0)
	{
		us_abortcommand(_("Can only place ports in node descriptions"));
		if ((us_tool->toolstate&NODETAILS) == 0)
			ttyputmsg(_("Use the 'edit-node' options"));
		return;
	}

	/* create the node */
	us_clearhighlightcount();
	savenp = us_curnodeproto;
	us_curnodeproto = np;
	subpar[0] = x_("wait-for-down");
	us_create(1, subpar);
	us_curnodeproto = savenp;

	var = getvalkey((INTBIG)us_tool, VTOOL, VSTRING|VISARRAY, us_highlightedkey);
	if (var == NOVARIABLE) return;
	(void)us_makehighlight(((CHAR **)var->addr)[0], &high);
	if (high.fromgeom == NOGEOM) return;
	if (!high.fromgeom->entryisnode) return;
	ni = high.fromgeom->entryaddr.ni;
	(void)setvalkey((INTBIG)ni, VNODEINST, us_edtec_option_key, LAYERPATCH, VINTEGER);

	/* postprocessing on the nodes */
	if (namesamen(pp, x_("port"), l) == 0 && l >= 1)
	{
		/* a port layer */
		if (count == 1)
		{
			name = ttygetline(M_("Port name: "));
			if (name == 0 || name[0] == 0)
			{
				us_abortedmsg();
				return;
			}
		} else name = par[1];
		var = setval((INTBIG)ni, VNODEINST, x_("EDTEC_portname"), (INTBIG)name, VSTRING|VDISPLAY);
		if (var != NOVARIABLE) defaulttextdescript(var->textdescript, ni->geom);
		if ((us_tool->toolstate&NODETAILS) == 0)
			ttyputmsg(_("Use 'change' option to set arc connectivity and port angle"));
	}
	if (namesamen(pp, x_("highlight"), l) == 0 && l >= 1)
	{
		/* a highlight layer */
		us_teceditsetpatch(ni, &us_edtechigh);
		(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_layer"), (INTBIG)NONODEPROTO, VNODEPROTO);
		ttyputmsg(_("Keep highlight a constant distance from the example edge"));
	}
	if (namesamen(pp, x_("circle-half"), l) == 0 && l >= 8)
		setarcdegrees(ni, 0.0, 180.0*EPI/180.0);
	if ((us_tool->toolstate&NODETAILS) == 0)
	{
		if (namesamen(pp, x_("rectangle-"), 10) == 0)
			ttyputmsg(_("Use 'change' option to set a layer for this shape"));
		if (namesamen(pp, x_("polygon-"), 8) == 0)
		{
			ttyputmsg(_("Use 'change' option to set a layer for this shape"));
			ttyputmsg(_("Use 'outline edit' to describe polygonal shape"));
		}
		if (namesamen(pp, x_("lines-"), 6) == 0)
			ttyputmsg(_("Use 'change' option to set a layer for this line"));
		if (namesamen(pp, x_("circle-"), 7) == 0)
			ttyputmsg(_("Use 'change' option to set a layer for this circle"));
		if (namesamen(pp, x_("text"), l) == 0 && l >= 1)
		{
			ttyputmsg(_("Use 'change' option to set a layer for this text"));
			ttyputmsg(_("Use 'var textedit ~.ART_message' command to set text"));
			ttyputmsg(_("Then use 'var change ~.ART_message display' command"));
		}
	}
	if (namesamen(pp, x_("circle-arc"), l) == 0 && l >= 8)
	{
		setarcdegrees(ni, 0.0, 45.0*EPI/180.0);
		if ((us_tool->toolstate&NODETAILS) == 0)
			ttyputmsg(_("Use 'setarc' command to set portion of circle"));
	}
	if ((ni->proto->userbits&HOLDSTRACE) != 0)
	{
		/* give it real points if it holds an outline */
		subpar[0] = x_("trace");
		subpar[1] = x_("init-points");
		us_node(2, subpar);
	}
}

/*
 * routine to highlight information about all layers (or ports if "doports" is true)
 */
void us_teceditidentify(BOOLEAN doports)
{
	REGISTER NODEPROTO *np;
	REGISTER INTBIG total, qtotal, i, j, bestrot, indent;
	REGISTER INTBIG xsep, ysep, *xpos, *ypos, dist, bestdist, *style;
	INTBIG lx, hx, ly, hy;
	REGISTER EXAMPLE *nelist;
	REGISTER SAMPLE *ns, **whichsam;
	static POLYGON *poly = NOPOLYGON;
	extern GRAPHICS us_hbox;

	np = us_needcell();
	if (np == NONODEPROTO) return;

	if (doports)
	{
		if (namesamen(np->protoname, x_("node-"), 5) != 0)
		{
			us_abortcommand(_("Must be editing a node to identify ports"));
			if ((us_tool->toolstate&NODETAILS) == 0)
				ttyputmsg(M_("Use the 'edit-node' option"));
			return;
		}
	} else
	{
		if (namesamen(np->protoname, x_("node-"), 5) != 0 &&
			namesamen(np->protoname, x_("arc-"), 4) != 0)
		{
			us_abortcommand(_("Must be editing a node or arc to identify layers"));
			if ((us_tool->toolstate&NODETAILS) == 0)
				ttyputmsg(M_("Use 'edit-node' or 'edit-arc' options"));
			return;
		}
	}

	/* get examples */
	if (namesamen(np->protoname, x_("node-"), 5) == 0)
		nelist = us_tecedgetexamples(np, TRUE); else
			nelist = us_tecedgetexamples(np, FALSE);
	if (nelist == NOEXAMPLE) return;

	/* count the number of appropriate samples in the main example */
	total = 0;
	for(ns = nelist->firstsample; ns != NOSAMPLE; ns = ns->nextsample)
	{
		if (!doports)
		{
			if (ns->layer != gen_portprim) total++;
		} else
		{
			if (ns->layer == gen_portprim) total++;
		}
	}
	if (total == 0)
	{
		us_tecedfreeexamples(nelist);
		us_abortcommand(_("There are no %s to identify"), (!doports ? _("layers") : _("ports")));
		return;
	}

	/* make arrays for position and association */
	xpos = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (xpos == 0) return;
	ypos = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (ypos == 0) return;
	style = (INTBIG *)emalloc(total * SIZEOFINTBIG, el_tempcluster);
	if (style == 0) return;
	whichsam = (SAMPLE **)emalloc(total * (sizeof (SAMPLE *)), el_tempcluster);
	if (whichsam == 0) return;

	/* fill in label positions */
	qtotal = (total+3) / 4;
	ysep = (el_curwindowpart->screenhy-el_curwindowpart->screenly) / qtotal;
	xsep = (el_curwindowpart->screenhx-el_curwindowpart->screenlx) / qtotal;
	indent = (el_curwindowpart->screenhy-el_curwindowpart->screenly) / 15;
	for(i=0; i<qtotal; i++)
	{
		/* label on the left side */
		xpos[i] = el_curwindowpart->screenlx + indent;
		ypos[i] = el_curwindowpart->screenly + ysep * i + ysep/2;
		style[i] = TEXTLEFT;
		if (i+qtotal < total)
		{
			/* label on the top side */
			xpos[i+qtotal] = el_curwindowpart->screenlx + xsep * i + xsep/2;
			ypos[i+qtotal] = el_curwindowpart->screenhy - indent;
			style[i+qtotal] = TEXTTOP;
		}
		if (i+qtotal*2 < total)
		{
			/* label on the right side */
			xpos[i+qtotal*2] = el_curwindowpart->screenhx - indent;
			ypos[i+qtotal*2] = el_curwindowpart->screenly + ysep * i + ysep/2;
			style[i+qtotal*2] = TEXTRIGHT;
		}
		if (i+qtotal*3 < total)
		{
			/* label on the bottom side */
			xpos[i+qtotal*3] = el_curwindowpart->screenlx + xsep * i + xsep/2;
			ypos[i+qtotal*3] = el_curwindowpart->screenly + indent;
			style[i+qtotal*3] = TEXTBOT;
		}
	}

	/* fill in sample associations */
	i = 0;
	for(ns = nelist->firstsample; ns != NOSAMPLE; ns = ns->nextsample)
	{
		if (!doports)
		{
			if (ns->layer != gen_portprim) whichsam[i++] = ns;
		} else
		{
			if (ns->layer == gen_portprim) whichsam[i++] = ns;
		}
	}

	/* rotate through all configurations, finding least distance */
	bestdist = MAXINTBIG;
	for(i=0; i<total; i++)
	{
		/* find distance from each label to its sample center */
		dist = 0;
		for(j=0; j<total; j++)
			dist += computedistance(xpos[j], ypos[j], whichsam[j]->xpos, whichsam[j]->ypos);
		if (dist < bestdist)
		{
			bestdist = dist;
			bestrot = i;
		}

		/* rotate the samples */
		ns = whichsam[0];
		for(j=1; j<total; j++) whichsam[j-1] = whichsam[j];
		whichsam[total-1] = ns;
	}

	/* rotate back to the best orientation */
	for(i=0; i<bestrot; i++)
	{
		ns = whichsam[0];
		for(j=1; j<total; j++) whichsam[j-1] = whichsam[j];
		whichsam[total-1] = ns;
	}

	/* get polygon */
	(void)needstaticpolygon(&poly, 2, us_tool->cluster);

	/* draw the highlighting */
	us_clearhighlightcount();
	for(i=0; i<total; i++)
	{
		ns = whichsam[i];
		poly->xv[0] = xpos[i];
		poly->yv[0] = ypos[i];
		poly->count = 1;
		if (ns->layer == NONODEPROTO)
		{
			poly->string = x_("HIGHLIGHT");
		} else if (ns->layer == gen_cellcenterprim)
		{
			poly->string = x_("GRAB");
		} else if (ns->layer == gen_portprim)
		{
			poly->string = us_tecedgetportname(ns->node);
			if (poly->string == 0) poly->string = x_("?");
		} else poly->string = &ns->layer->protoname[6];
		poly->desc = &us_hbox;
		poly->style = style[i];
		TDCLEAR(poly->textdescript);
		TDSETSIZE(poly->textdescript, TXTSETQLAMBDA(4));
		poly->tech = el_curtech;
		us_hbox.col = HIGHLIT;

		nodesizeoffset(ns->node, &lx, &ly, &hx, &hy);
		switch (poly->style)
		{
			case TEXTLEFT:
				poly->xv[1] = ns->node->lowx+lx;
				poly->yv[1] = (ns->node->lowy + ns->node->highy) / 2;
				poly->style = TEXTBOTLEFT;
				break;
			case TEXTRIGHT:
				poly->xv[1] = ns->node->highx-hx;
				poly->yv[1] = (ns->node->lowy + ns->node->highy) / 2;
				poly->style = TEXTBOTRIGHT;
				break;
			case TEXTTOP:
				poly->xv[1] = (ns->node->lowx + ns->node->highx) / 2;
				poly->yv[1] = ns->node->highy-hy;
				poly->style = TEXTTOPLEFT;
				break;
			case TEXTBOT:
				poly->xv[1] = (ns->node->lowx + ns->node->highx) / 2;
				poly->yv[1] = ns->node->lowy+ly;
				poly->style = TEXTBOTLEFT;
				break;
		}
		us_showpoly(poly, el_curwindowpart);

		/* now draw the vector polygon */
		poly->count = 2;
		poly->style = VECTORS;
		us_showpoly(poly, el_curwindowpart);
	}

	/* free rotation arrays */
	efree((CHAR *)xpos);
	efree((CHAR *)ypos);
	efree((CHAR *)style);
	efree((CHAR *)whichsam);

	/* free all examples */
	us_tecedfreeexamples(nelist);
}

/*
 * routine to print information about selected object
 */
void us_teceditinquire(void)
{
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pt1, *pt2, *pt;
	REGISTER VARIABLE *var;
	REGISTER INTBIG opt;
	REGISTER HIGHLIGHT *high;

	high = us_getonehighlight();
	if ((high->status&HIGHTYPE) != HIGHTEXT && (high->status&HIGHTYPE) != HIGHFROM)
	{
		us_abortcommand(_("Must select a single object for inquiry"));
		return;
	}
	if ((high->status&HIGHTYPE) == HIGHFROM && !high->fromgeom->entryisnode)
	{
		/* describe currently highlighted arc */
		ai = high->fromgeom->entryaddr.ai;
		if (ai->proto != gen_universalarc)
		{
			ttyputmsg(_("This is an unimportant %s arc"), describearcproto(ai->proto));
			return;
		}
		if (ai->end[0].nodeinst->proto != gen_portprim ||
			ai->end[1].nodeinst->proto != gen_portprim)
		{
			ttyputmsg(_("This arc makes an unimportant connection"));
			return;
		}
		pt1 = us_tecedgetportname(ai->end[0].nodeinst);
		pt2 = us_tecedgetportname(ai->end[1].nodeinst);
		if (pt1 == 0 || pt2 == 0)
			ttyputmsg(_("This arc connects two port objects")); else
				ttyputmsg(_("This arc connects ports '%s' and '%s'"), pt1, pt2);
		return;
	}
	ni = high->fromgeom->entryaddr.ni;
	np = ni->parent;
	opt = us_tecedgetoption(ni);
	if (opt < 0)
	{
		ttyputmsg(_("This object has no relevance to technology editing"));
		return;
	}

	switch (opt)
	{
		case ARCFIXANG:
			ttyputmsg(_("This object defines the fixed-angle factor of %s"), describenodeproto(np));
			break;
		case ARCFUNCTION:
			ttyputmsg(_("This object defines the function of %s"), describenodeproto(np));
			break;
		case ARCINC:
			ttyputmsg(_("This object defines the prefered angle increment of %s"), describenodeproto(np));
			break;
		case ARCNOEXTEND:
			ttyputmsg(_("This object defines the arc extension of %s"), describenodeproto(np));
			break;
		case ARCWIPESPINS:
			ttyputmsg(_("This object defines the arc coverage of %s"), describenodeproto(np));
			break;
		case CENTEROBJ:
			ttyputmsg(_("This object identifies the grab point of %s"), describenodeproto(np));
			break;
		case LAYER3DHEIGHT:
			ttyputmsg(_("This object defines the 3D height of %s"), describenodeproto(np));
			break;
		case LAYER3DTHICK:
			ttyputmsg(_("This object defines the 3D thickness of %s"), describenodeproto(np));
			break;
		case LAYERPRINTCOL:
			ttyputmsg(_("This object defines the print colors of %s"), describenodeproto(np));
			break;
		case LAYERCIF:
			ttyputmsg(_("This object defines the CIF name of %s"), describenodeproto(np));
			break;
		case LAYERCOLOR:
			ttyputmsg(_("This object defines the color of %s"), describenodeproto(np));
			break;
		case LAYERDXF:
			ttyputmsg(_("This object defines the DXF name(s) of %s"), describenodeproto(np));
			break;
		case LAYERDRCMINWID:
			ttyputmsg(_("This object defines the minimum DRC width of %s (OBSOLETE)"), describenodeproto(np));
			break;
		case LAYERFUNCTION:
			ttyputmsg(_("This object defines the function of %s"), describenodeproto(np));
			break;
		case LAYERGDS:
			ttyputmsg(_("This object defines the Calma GDS-II number of %s"), describenodeproto(np));
			break;
		case LAYERLETTERS:
			ttyputmsg(_("This object defines the letters to use for %s"), describenodeproto(np));
			break;
		case LAYERPATCONT:
			ttyputmsg(_("This object provides control of the stipple pattern in %s"), describenodeproto(np));
			break;
		case LAYERPATTERN:
			ttyputmsg(_("This is one of the bitmap squares in %s"), describenodeproto(np));
			break;
		case LAYERSPICAP:
			ttyputmsg(_("This object defines the SPICE capacitance of %s"), describenodeproto(np));
			break;
		case LAYERSPIECAP:
			ttyputmsg(_("This object defines the SPICE edge capacitance of %s"), describenodeproto(np));
			break;
		case LAYERSPIRES:
			ttyputmsg(_("This object defines the SPICE resistance of %s"), describenodeproto(np));
			break;
		case LAYERSTYLE:
			ttyputmsg(_("This object defines the style of %s"), describenodeproto(np));
			break;
		case LAYERPATCH:
		case HIGHLIGHTOBJ:
			np = us_tecedgetlayer(ni);
			if (np == 0)
				ttyputerr(_("This is an object with no valid layer!")); else
			{
				if (np == NONODEPROTO) ttyputmsg(_("This is a highlight box")); else
					ttyputmsg(_("This is a '%s' layer"), &np->protoname[6]);
				var = getval((INTBIG)ni, VNODEINST, VSTRING, x_("EDTEC_minbox"));
				if (var != NOVARIABLE)
					ttyputmsg(_("   It is at minimum size"));
			}
			break;
		case NODEFUNCTION:
			ttyputmsg(_("This object defines the function of %s"), describenodeproto(np));
			break;
		case NODELOCKABLE:
			ttyputmsg(_("This object tells if %s can be locked (used in array technologies)"),
				describenodeproto(np));
			break;
		case NODEMULTICUT:
			ttyputmsg(_("This object tells the separation between multiple contact cuts in %s"),
				describenodeproto(np));
			break;
		case NODESERPENTINE:
			ttyputmsg(_("This object tells if %s is a serpentine transistor"), describenodeproto(np));
			break;
		case NODESQUARE:
			ttyputmsg(_("This object tells if %s is square"), describenodeproto(np));
			break;
		case NODEWIPES:
			ttyputmsg(_("This object tells if %s disappears when conencted to one or two arcs"),
				describenodeproto(np));
			break;
		case PORTOBJ:
			pt = us_tecedgetportname(ni);
			if (pt == 0) ttyputmsg(_("This is a port object")); else
				ttyputmsg(_("This is port '%s'"), pt);
			break;
		case TECHDESCRIPT:
			ttyputmsg(_("This object contains the technology description"));
			break;
		case TECHLAMBDA:
			ttyputmsg(_("This object defines the value of lambda"));
			break;
		default:
			ttyputerr(_("This object has unknown information"));
			break;
	}
}

/*
 * Routine to return a brief description of node "ni" for use in the status area.
 */
CHAR *us_teceddescribenode(NODEINST *ni)
{
	REGISTER INTBIG opt;
	REGISTER NODEPROTO *np;
	REGISTER void *infstr;
	REGISTER CHAR *pt;

	opt = us_tecedgetoption(ni);
	if (opt < 0) return(0);
	switch (opt)
	{
		case ARCFIXANG:      return(_("Arc fixed-angle factor"));
		case ARCFUNCTION:    return(_("Arc function"));
		case ARCINC:         return(_("Arc angle increment"));
		case ARCNOEXTEND:    return(_("Arc extension"));
		case ARCWIPESPINS:   return(_("Arc coverage"));
		case CENTEROBJ:      return(_("Grab point"));
		case LAYER3DHEIGHT:  return(_("3D height"));
		case LAYER3DTHICK:   return(_("3D thickness"));
		case LAYERPRINTCOL:  return(_("Print colors"));
		case LAYERCIF:       return(_("CIF names"));
		case LAYERCOLOR:     return(_("Layer color"));
		case LAYERDXF:       return(_("DXF name(s)"));
		case LAYERFUNCTION:  return(_("Layer function"));
		case LAYERGDS:       return(_("GDS-II number(s)"));
		case LAYERLETTERS:   return(_("Layer letters"));
		case LAYERPATCONT:   return(_("Pattern control"));
		case LAYERPATTERN:   return(_("Stipple pattern element"));
		case LAYERSPICAP:    return(_("Spice capacitance"));
		case LAYERSPIECAP:   return(_("Spice edge capacitance"));
		case LAYERSPIRES:    return(_("Spice resistance"));
		case LAYERSTYLE:     return(_("Srawing style"));
		case LAYERPATCH:
		case HIGHLIGHTOBJ:
			np = us_tecedgetlayer(ni);
			if (np == 0) return(_("Unknown layer"));
			if (np == NONODEPROTO) return(_("Highlight box"));
			infstr = initinfstr();
			formatinfstr(infstr, _("Layer %s"), &np->protoname[6]);
			return(returninfstr(infstr));
		case NODEFUNCTION:   return(_("Node function"));
		case NODELOCKABLE:   return(_("Node lockability"));
		case NODEMULTICUT:   return(_("Multicut separation"));
		case NODESERPENTINE: return(_("Serpentine transistor"));
		case NODESQUARE:     return(_("Square node"));
		case NODEWIPES:      return(_("Disappearing pin"));
		case PORTOBJ:
			pt = us_tecedgetportname(ni);
			if (pt == 0) return(_("Unnamed export"));
			infstr = initinfstr();
			formatinfstr(infstr, _("Export %s"), pt);
			return(returninfstr(infstr));
		case TECHDESCRIPT:   return(_("Technology description"));
		case TECHLAMBDA:     return(_("Lambda value"));
	}
	return(0);
}

/*
 * routine for modifying the selected object.  If two are selected, connect them.
 */
void us_teceditmodobject(INTBIG count, CHAR *par[])
{
	REGISTER NODEINST *ni;
	GEOM *fromgeom, *togeom, **list;
	PORTPROTO *fromport, *toport;
	INTBIG opt, textcount, i, found;
	REGISTER HIGHLIGHT *high;
	HIGHLIGHT newhigh;
	CHAR *newpar[2], **textinfo;

	/* special case for pattern changes */
	list = us_gethighlighted(WANTNODEINST|WANTARCINST, &textcount, &textinfo);
	if (textcount == 0)
	{
		found = 0;
		for(i=0; list[i] != NOGEOM; i++)
		{
			if (!list[i]->entryisnode) continue;
			ni = list[i]->entryaddr.ni;
			opt = us_tecedgetoption(ni);
			if (opt == LAYERPATTERN) found++;
		}
		if (found > 0)
		{
			/* change them */
			us_clearhighlightcount();
			for(i=0; list[i] != NOGEOM; i++)
			{
				if (!list[i]->entryisnode) continue;
				ni = list[i]->entryaddr.ni;
				opt = us_tecedgetoption(ni);
				if (opt == LAYERPATTERN)
					us_tecedlayerpattern(ni);
			}
			return;
		}
	}

	/* if two are highlighted, connect them */
	if (!us_gettwoobjects(&fromgeom, &fromport, &togeom, &toport))
	{
		newpar[0] = x_("angle");   newpar[1] = x_("0");
		us_create(2, newpar);
		return;
	}

	/* make sure something is highlighted */
	high = us_getonehighlight();
	if (high == NOHIGHLIGHT) return;

	/* must be one node highlighted */
	ni = (NODEINST *)us_getobject(VNODEINST, TRUE);
	if (ni == NONODEINST) return;

	/* determine technology editor relevance */
	opt = us_tecedgetoption(ni);

	/* special case for port modification: reset highlighting by hand */
	if (opt == PORTOBJ)
	{
		/* pick up old highlight values and then remove highlighting */
		newhigh = *high;
		us_clearhighlightcount();

		/* modify the port */
		us_tecedmodport(ni, count, par);

		/* set new highlighting variable */
		newhigh.fromvar = getval((INTBIG)ni, VNODEINST, VSTRING, x_("EDTEC_portname"));
		if (newhigh.fromvar == NOVARIABLE)
			newhigh.fromvar = getvalkey((INTBIG)ni, VNODEINST, VSTRING, el_node_name_key);
		us_addhighlight(&newhigh);
		return;
	}

	/* ignore if no parameter given */
	if (count <= 0) return;

	/* handle other cases */
	us_pushhighlight();
	us_clearhighlightcount();
	switch (opt)
	{
		case ARCFIXANG:      us_tecedarcfixang(ni, count, par);      break;
		case ARCFUNCTION:    us_tecedarcfunction(ni, count, par);    break;
		case ARCINC:         us_tecedarcinc(ni, count, par);         break;
		case ARCNOEXTEND:    us_tecedarcnoextend(ni, count, par);    break;
		case ARCWIPESPINS:   us_tecedarcwipes(ni, count, par);       break;
		case LAYER3DHEIGHT:  us_tecedlayer3dheight(ni, count, par);  break;
		case LAYER3DTHICK:   us_tecedlayer3dthick(ni, count, par);   break;
		case LAYERPRINTCOL:  us_tecedlayerprintcol(ni, count, par);  break;
		case LAYERCIF:       us_tecedlayercif(ni, count, par);       break;
		case LAYERCOLOR:     us_tecedlayercolor(ni, count, par);     break;
		case LAYERDXF:       us_tecedlayerdxf(ni, count, par);       break;
		case LAYERDRCMINWID: us_tecedlayerdrcminwid(ni, count, par); break;
		case LAYERFUNCTION:  us_tecedlayerfunction(ni, count, par);  break;
		case LAYERGDS:       us_tecedlayergds(ni, count, par);       break;
		case LAYERLETTERS:   us_tecedlayerletters(ni, count, par);   break;
		case LAYERPATCONT:   us_tecedlayerpatterncontrol(ni, count, par);   break;
		case LAYERPATCH:     us_tecedlayertype(ni, count, par);      break;
		case LAYERSPICAP:    us_tecedlayerspicap(ni, count, par);    break;
		case LAYERSPIECAP:   us_tecedlayerspiecap(ni, count, par);   break;
		case LAYERSPIRES:    us_tecedlayerspires(ni, count, par);    break;
		case LAYERSTYLE:     us_tecedlayerstyle(ni, count, par);     break;
		case NODEFUNCTION:   us_tecednodefunction(ni, count, par);   break;
		case NODELOCKABLE:   us_tecednodelockable(ni, count, par);   break;
		case NODEMULTICUT:   us_tecednodemulticut(ni, count, par);   break;
		case NODESERPENTINE: us_tecednodeserpentine(ni, count, par); break;
		case NODESQUARE:     us_tecednodesquare(ni, count, par);     break;
		case NODEWIPES:      us_tecednodewipes(ni, count, par);      break;
		case TECHDESCRIPT:   us_tecedinfodescript(ni, count, par);   break;
		case TECHLAMBDA:     us_tecedinfolambda(ni, count, par);     break;
		default:             us_abortcommand(_("Cannot modify this object"));   break;
	}
	us_pophighlight(FALSE);
}

/***************************** OBJECT MODIFICATION *****************************/

void us_tecedlayer3dheight(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Enter the height of the layer when viewed in 3D"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXT3DHEIGHT);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayer3dthick(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Enter the thickness of the layer when viewed in 3D"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXT3DTHICK);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerprintcol(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;
	INTBIG len, r, g, b, o, f;
	REGISTER VARIABLE *var;

	if (count < 2)
	{
		ttyputverbose(_("Enter the print colors of the layer"));
		return;
	}
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
	us_teceditgetprintcol(var, &r, &g, &b, &o, &f);
	len = estrlen(par[0]);
	if (namesamen(par[0], x_("red"), len) == 0) r = myatoi(par[1]); else
	if (namesamen(par[0], x_("green"), len) == 0) g = myatoi(par[1]); else
	if (namesamen(par[0], x_("blue"), len) == 0) b = myatoi(par[1]); else
	if (namesamen(par[0], x_("opacity"), len) == 0) o = myatoi(par[1]); else
	if (namesamen(par[0], x_("foreground"), len) == 0)
	{
		if (namesame(par[1], x_("on")) == 0) f = 1; else
			if (namesame(par[1], x_("off")) == 0) f = 0; else
				f = myatoi(par[1]);
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTPRINTCOL);
	formatinfstr(infstr, x_("%ld,%ld,%ld, %ld,%s"), r, g, b, o, (f==0 ? x_("off") : x_("on")));
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerdrcminwid(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Enter the minimum DRC width (negative for none)"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTDRCMINWID);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayercolor(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("New color required"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTCOLOR);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));

	/* redraw the demo layer in this cell */
	us_tecedredolayergraphics(ni->parent);
}

void us_tecedlayerstyle(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Layer style required"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTSTYLE);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));

	/* redraw the demo layer in this cell */
	us_tecedredolayergraphics(ni->parent);
}

void us_tecedlayercif(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTCIF);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerdxf(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTDXF);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayergds(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTGDS);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerspires(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Enter the SPICE layer resistance"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTSPICERES);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerspicap(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Enter the SPICE layer capacitance"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTSPICECAP);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerspiecap(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Enter the SPICE layer edge capacitance"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTSPICEECAP);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerfunction(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER INTBIG func, newfunc;
	REGISTER CHAR *str;
	REGISTER VARIABLE *var;
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Layer function required"));
		return;
	}

	/* get the existing function */
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
	func = 0;
	if (var != NOVARIABLE)
	{
		str = (CHAR *)var->addr;
		while (*str != 0 && *str != ':') str++;
		if (*str == ':') str++;
		while (*str == ' ') str++;
		if (*str != 0) func = us_teceditparsefun(str);
	}

	/* add in the new function */
	newfunc = us_teceditparsefun(par[0]);
	if (newfunc <= LFTYPE) func = newfunc; else func |= newfunc;

	/* build the string corresponding to this function */
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTFUNCTION);
	us_tecedaddfunstring(infstr, func);

	/* rewrite the node with the new function */
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerletters(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np;
	REGISTER CHAR *pt;
	REGISTER INTBIG i;
	CHAR *cif, *layerletters, *dxf, *gds;
	GRAPHICS desc;
	float spires, spicap, spiecap;
	INTBIG func, drcminwid, height3d, thick3d, printcol[5];
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Layer letter(s) required"));
		return;
	}

	/* check layer letters for uniqueness */
	for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (namesamen(np->protoname, x_("layer-"), 6) != 0) continue;
		cif = layerletters = gds = 0;
		if (us_teceditgetlayerinfo(np, &desc, &cif, &func, &layerletters,
			&dxf, &gds, &spires, &spicap, &spiecap, &drcminwid, &height3d,
				&thick3d, printcol)) return;
		if (gds != 0) efree(gds);
		if (cif != 0) efree(cif);
		if (layerletters == 0) continue;

		/* check these layer letters for uniqueness */
		for(pt = layerletters; *pt != 0; pt++)
		{
			for(i=0; par[0][i] != 0; i++) if (par[0][i] == *pt)
			{
				us_abortcommand(_("Cannot use letter '%c', it is in %s"), *pt, describenodeproto(np));
				efree(layerletters);
				return;
			}
		}
		efree(layerletters);
	}

	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTLETTERS);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedlayerpatterncontrol(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER INTBIG len;
	REGISTER NODEINST *pni;
	REGISTER INTBIG opt;
	UINTSML color;
	static GRAPHICS desc;
	static BOOLEAN didcopy = FALSE;
	CHAR *cif, *layerletters, *gds, *dxf;
	INTBIG func, drcminwid, height3d, thick3d, printcol[5];
	float spires, spicap, spiecap;

	len = estrlen(par[0]);
	if (namesamen(par[0], x_("Clear Pattern"), len) == 0 && len > 2)
	{
		us_clearhighlightcount();
		for(pni = ni->parent->firstnodeinst; pni != NONODEINST; pni = pni->nextnodeinst)
		{
			opt = us_tecedgetoption(pni);
			if (opt != LAYERPATTERN) continue;
			color = us_tecedlayergetpattern(pni);
			if (color != 0)
				us_tecedlayersetpattern(pni, 0);
		}

		/* redraw the demo layer in this cell */
		us_tecedredolayergraphics(ni->parent);
		return;
	}
	if (namesamen(par[0], x_("Invert Pattern"), len) == 0 && len > 1)
	{
		us_clearhighlightcount();
		for(pni = ni->parent->firstnodeinst; pni != NONODEINST; pni = pni->nextnodeinst)
		{
			opt = us_tecedgetoption(pni);
			if (opt != LAYERPATTERN) continue;
			color = us_tecedlayergetpattern(pni);
			us_tecedlayersetpattern(pni, (INTSML)(~color));
		}

		/* redraw the demo layer in this cell */
		us_tecedredolayergraphics(ni->parent);
		return;
	}
	if (namesamen(par[0], x_("Copy Pattern"), len) == 0 && len > 2)
	{
		us_clearhighlightcount();
		if (us_teceditgetlayerinfo(ni->parent, &desc, &cif, &func, &layerletters, &dxf,
			&gds, &spires, &spicap, &spiecap, &drcminwid, &height3d, &thick3d, printcol)) return;
		didcopy = TRUE;
		return;
	}
	if (namesamen(par[0], x_("Paste Pattern"), len) == 0 && len > 1)
	{
		us_clearhighlightcount();
		us_teceditsetlayerpattern(ni->parent, &desc);

		/* redraw the demo layer in this cell */
		us_tecedredolayergraphics(ni->parent);
		return;
	}
}

/*
 * Routine to return the color in layer-pattern node "ni" (off is 0, on is 0xFFFF).
 */
INTSML us_tecedlayergetpattern(NODEINST *ni)
{
	REGISTER VARIABLE *var;
	REGISTER INTSML color;

	if (ni->proto == art_boxprim) return(0);
	if (ni->proto == art_filledboxprim)
	{
		var = getvalkey((INTBIG)ni, VNODEINST, VSHORT|VISARRAY, art_patternkey);
		if (var == NOVARIABLE) color = (INTSML)0xFFFF; else color = ((INTSML *)var->addr)[0];
		return(color);
	}
	return(0);
}

/*
 * Routine to set layer-pattern node "ni" to be color "color" (off is 0, on is 0xFFFF).
 * Returns the address of the node (may be different than "ni" if it had to be replaced).
 */
NODEINST *us_tecedlayersetpattern(NODEINST *ni, INTSML color)
{
	REGISTER NODEINST *newni;
	INTBIG i;
	UINTSML col[16];

	if (ni->proto == art_boxprim)
	{
		if (color == 0) return(ni);
		startobjectchange((INTBIG)ni, VNODEINST);
		newni = replacenodeinst(ni, art_filledboxprim, FALSE, FALSE);
		if (newni == NONODEINST) return(ni);
		endobjectchange((INTBIG)newni, VNODEINST);
		ni = newni;
	} else if (ni->proto == art_filledboxprim)
	{
		startobjectchange((INTBIG)ni, VNODEINST);
		for(i=0; i<16; i++) col[i] = color;
		(void)setvalkey((INTBIG)ni, VNODEINST, art_patternkey, (INTBIG)col,
			VSHORT|VISARRAY|(16<<VLENGTHSH));
		endobjectchange((INTBIG)ni, VNODEINST);
	}
	return(ni);
}

/*
 * Routine to toggle the color of layer-pattern node "ni" (called when the user does a
 * "technology edit" click on the node).
 */
void us_tecedlayerpattern(NODEINST *ni)
{
	HIGHLIGHT high;
	UINTSML color;

	color = us_tecedlayergetpattern(ni);
	ni = us_tecedlayersetpattern(ni, (INTSML)(~color));

	high.status = HIGHFROM;
	high.cell = ni->parent;
	high.fromgeom = ni->geom;
	high.fromport = NOPORTPROTO;
	high.frompoint = 0;
	high.fromvar = NOVARIABLE;
	high.fromvarnoeval = NOVARIABLE;
	us_addhighlight(&high);

	/* redraw the demo layer in this cell */
	us_tecedredolayergraphics(ni->parent);
}

/*
 * routine to modify the layer information in node "ni".
 */
void us_tecedlayertype(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER NODEPROTO *np;
	CHAR *cif, *layerletters, *dxf, *gds;
	REGISTER CHAR *name;
	GRAPHICS desc;
	float spires, spicap, spiecap;
	INTBIG func, drcminwid, height3d, thick3d, printcol[5];
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a layer name"));
		return;
	}

	np = us_needcell();
	if (np == NONODEPROTO) return;
	if (namesame(par[0], x_("SET-MINIMUM-SIZE")) == 0)
	{
		if (namesamen(np->protoname, x_("node-"), 5) != 0)
		{
			us_abortcommand(_("Can only set minimum size in node descriptions"));
			if ((us_tool->toolstate&NODETAILS) == 0) ttyputmsg(_("Use 'edit-node' option"));
			return;
		}
		startobjectchange((INTBIG)ni, VNODEINST);
		(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_minbox"), (INTBIG)x_("MIN"), VSTRING|VDISPLAY);
		endobjectchange((INTBIG)ni, VNODEINST);
		return;
	}

	if (namesame(par[0], x_("CLEAR-MINIMUM-SIZE")) == 0)
	{
		if (getval((INTBIG)ni, VNODEINST, VSTRING, x_("EDTEC_minbox")) == NOVARIABLE)
		{
			ttyputmsg(_("Minimum size is not set on this layer"));
			return;
		}
		startobjectchange((INTBIG)ni, VNODEINST);
		(void)delval((INTBIG)ni, VNODEINST, x_("EDTEC_minbox"));
		endobjectchange((INTBIG)ni, VNODEINST);
		return;
	}

	/* find the actual cell with that layer specification */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("layer-"));
	addstringtoinfstr(infstr, par[0]);
	name = returninfstr(infstr);
	np = getnodeproto(name);
	if (np == NONODEPROTO)
	{
		ttyputerr(_("Cannot find layer primitive %s"), name);
		return;
	}

	/* get the characteristics of that layer */
	cif = layerletters = gds = 0;
	if (us_teceditgetlayerinfo(np, &desc, &cif, &func, &layerletters,
		&dxf, &gds, &spires, &spicap, &spiecap, &drcminwid, &height3d,
			&thick3d, printcol)) return;
	if (gds != 0) efree(gds);
	if (cif != 0) efree(cif);
	if (layerletters != 0) efree(layerletters);

	startobjectchange((INTBIG)ni, VNODEINST);
	us_teceditsetpatch(ni, &desc);
	(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_layer"), (INTBIG)np, VNODEPROTO);
	endobjectchange((INTBIG)ni, VNODEINST);
}

/*
 * routine to modify port characteristics
 */
void us_tecedmodport(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER INTBIG total, i, len, j, yes;
	BOOLEAN changed;
	REGISTER BOOLEAN *yesno;
	REGISTER NODEPROTO *np, **conlist;
	REGISTER VARIABLE *var;

	/* build an array of arc connections */
	for(total = 0, np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesamen(np->protoname, x_("arc-"), 4) == 0) total++;
	conlist = (NODEPROTO **)emalloc(total * (sizeof (NODEPROTO *)), el_tempcluster);
	if (conlist == 0) return;
	for(total = 0, np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
		if (namesamen(np->protoname, x_("arc-"), 4) == 0) conlist[total++] = np;
	yesno = (BOOLEAN *)emalloc(total * (sizeof (BOOLEAN)), el_tempcluster);
	if (yesno == 0) return;
	for(i=0; i<total; i++) yesno[i] = FALSE;

	/* put current list into the array */
	var = getval((INTBIG)ni, VNODEINST, VNODEPROTO|VISARRAY, x_("EDTEC_connects"));
	if (var != NOVARIABLE)
	{
		len = getlength(var);
		for(j=0; j<len; j++)
		{
			for(i=0; i<total; i++)
				if (conlist[i] == ((NODEPROTO **)var->addr)[j]) break;
			if (i < total) yesno[i] = TRUE;
		}
	}

	/* parse the command parameters */
	changed = FALSE;
	for(i=0; i<count-1; i += 2)
	{
		/* search for an arc name */
		for(np = el_curlib->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
			if (namesamen(np->protoname, x_("arc-"), 4) == 0 &&
				namesame(&np->protoname[4], par[i]) == 0) break;
		if (np != NONODEPROTO)
		{
			for(j=0; j<total; j++) if (conlist[j] == np)
			{
				if (*par[i+1] == 'y' || *par[i+1] == 'Y') yesno[j] = TRUE; else
					yesno[j] = FALSE;
				changed = TRUE;
				break;
			}
			continue;
		}

		if (namesame(par[i], x_("PORT-ANGLE")) == 0)
		{
			(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_portangle"), myatoi(par[i+1]), VINTEGER);
			continue;
		}
		if (namesame(par[i], x_("PORT-ANGLE-RANGE")) == 0)
		{
			(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_portrange"), myatoi(par[i+1]), VINTEGER);
			continue;
		}
	}

	/* store list back if it was changed */
	if (changed)
	{
		yes = 0;
		for(i=0; i<total; i++)
		{
			if (!yesno[i]) continue;
			conlist[yes++] = conlist[i];
		}
		if (yes == 0 && var != NOVARIABLE)
			(void)delval((INTBIG)ni, VNODEINST, x_("EDTEC_connects")); else
		{
			(void)setval((INTBIG)ni, VNODEINST, x_("EDTEC_connects"), (INTBIG)conlist,
				VNODEPROTO|VISARRAY|(yes<<VLENGTHSH));
		}
	}
	efree((CHAR *)conlist);
	efree((CHAR *)yesno);
}

void us_tecedarcfunction(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a layer function"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTFUNCTION);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedarcfixang(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Fixed-angle: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedarcwipes(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Wipes pins: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedarcnoextend(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Extend arcs: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedarcinc(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires an angle increment in degrees"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Angle increment: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecednodefunction(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a node function"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, TECEDNODETEXTFUNCTION);
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecednodeserpentine(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Serpentine transistor: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecednodesquare(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Square node: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecednodewipes(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Invisible with 1 or 2 arcs: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecednodelockable(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a yes or no"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Lockable: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecednodemulticut(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a separation distance (0 for none)"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Multicut separation: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedinfolambda(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a value of lambda"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Lambda: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

void us_tecedinfodescript(NODEINST *ni, INTBIG count, CHAR *par[])
{
	REGISTER void *infstr;

	if (par[0][0] == 0)
	{
		us_abortcommand(_("Requires a technology description"));
		return;
	}
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("Description: "));
	addstringtoinfstr(infstr, par[0]);
	us_tecedsetnode(ni, returninfstr(infstr));
}

/****************************** UTILITIES ******************************/

void us_tecedsetnode(NODEINST *ni, CHAR *chr)
{
	UINTBIG descript[TEXTDESCRIPTSIZE];
	REGISTER VARIABLE *var;
	CHAR *newmsg;

	allocstring(&newmsg, chr, el_tempcluster);
	startobjectchange((INTBIG)ni, VNODEINST);
	var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
	if (var == NOVARIABLE) TDCLEAR(descript); else TDCOPY(descript, var->textdescript);
	var = setvalkey((INTBIG)ni, VNODEINST, art_messagekey, (INTBIG)newmsg, VSTRING|VDISPLAY);
	if (var != NOVARIABLE) modifydescript((INTBIG)ni, VNODEINST, var, descript);
	endobjectchange((INTBIG)ni, VNODEINST);
	efree((CHAR *)newmsg);
}

/*
 * routine to call up the cell "cellname" (either create it or reedit it)
 * returns NONODEPROTO if there is an error or the cell exists
 */
NODEPROTO *us_tecedentercell(CHAR *cellname)
{
	REGISTER NODEPROTO *np;
	CHAR *newpar[2];

	np = getnodeproto(cellname);
	if (np != NONODEPROTO && np->primindex == 0)
	{
		newpar[0] = x_("editcell");
		newpar[1] = cellname;
		telltool(us_tool, 2, newpar);
		return(NONODEPROTO);
	}

	/* create the cell */
	np = newnodeproto(cellname, el_curlib);
	if (np == NONODEPROTO) return(NONODEPROTO);

	/* now edit the cell */
	newpar[0] = x_("editcell");
	newpar[1] = cellname;
	telltool(us_tool, 2, newpar);
	return(np);
}

/*
 * routine to redraw the demo layer in "layer" cell "np"
 */
void us_tecedredolayergraphics(NODEPROTO *np)
{
	REGISTER VARIABLE *var;
	REGISTER NODEINST *ni;
	GRAPHICS desc;
	REGISTER NODEPROTO *onp;
	CHAR *cif, *layerletters, *dxf, *gds;
	INTBIG func, drcminwid, height3d, thick3d, printcol[5];
	float spires, spicap, spiecap;

	/* find the demo patch in this cell */
	var = getval((INTBIG)np, VNODEPROTO, VNODEINST, x_("EDTEC_colornode"));
	if (var == NOVARIABLE) return;
	ni = (NODEINST *)var->addr;

	/* get the current description of this layer */
	cif = layerletters = gds = 0;
	if (us_teceditgetlayerinfo(np, &desc, &cif, &func, &layerletters,
		&dxf, &gds, &spires, &spicap, &spiecap, &drcminwid, &height3d,
			&thick3d, printcol)) return;
	if (gds != 0) efree(gds);
	if (cif != 0) efree(cif);
	if (layerletters != 0) efree(layerletters);

	/* modify the demo patch to reflect the color and pattern */
	startobjectchange((INTBIG)ni, VNODEINST);
	us_teceditsetpatch(ni, &desc);
	endobjectchange((INTBIG)ni, VNODEINST);

	/* now do this to all layers in all cells! */
	for(onp = el_curlib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (namesamen(onp->protoname, x_("arc-"), 4) != 0 &&
			namesamen(onp->protoname, x_("node-"), 5) != 0) continue;
		for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			if (us_tecedgetoption(ni) != LAYERPATCH) continue;
			var = getval((INTBIG)ni, VNODEINST, VNODEPROTO, x_("EDTEC_layer"));
			if (var == NOVARIABLE) continue;
			if ((NODEPROTO *)var->addr != np) continue;
			us_teceditsetpatch(ni, &desc);
		}
	}
}

void us_teceditsetpatch(NODEINST *ni, GRAPHICS *desc)
{
	REGISTER INTBIG i;
	UINTBIG pattern[16];
	UINTSML spattern[16];

	(void)setvalkey((INTBIG)ni, VNODEINST, art_colorkey, desc->col, VINTEGER);
	if ((desc->colstyle&NATURE) == PATTERNED)
	{
		if ((desc->colstyle&OUTLINEPAT) == 0)
		{
			for(i=0; i<16; i++) pattern[i] = desc->raster[i];
			(void)setvalkey((INTBIG)ni, VNODEINST, art_patternkey, (INTBIG)pattern,
				VINTEGER|VISARRAY|(16<<VLENGTHSH));
		} else
		{
			for(i=0; i<16; i++) spattern[i] = desc->raster[i];
			(void)setvalkey((INTBIG)ni, VNODEINST, art_patternkey, (INTBIG)spattern,
				VSHORT|VISARRAY|(16<<VLENGTHSH));
		}
	} else
	{
		if (getvalkey((INTBIG)ni, VNODEINST, -1, art_patternkey) != NOVARIABLE)
			(void)delvalkey((INTBIG)ni, VNODEINST, art_patternkey);
	}
}

/*
 * routine to load the color map associated with library "lib"
 */
void us_tecedloadlibmap(LIBRARY *lib)
{
	REGISTER VARIABLE *var;
	REGISTER INTBIG i;
	REGISTER INTBIG *mapptr;
	INTBIG redmap[256], greenmap[256], bluemap[256];

	var = getval((INTBIG)lib, VLIBRARY, VINTEGER|VISARRAY, x_("EDTEC_colormap"));
	if (var != NOVARIABLE)
	{
		mapptr = (INTBIG *)var->addr;
		for(i=0; i<256; i++)
		{
			redmap[i] = *mapptr++;
			greenmap[i] = *mapptr++;
			bluemap[i] = *mapptr++;
		}

		/* disable option tracking */
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_ignoreoptionchangeskey, 1,
			VINTEGER|VDONTSAVE);

		startobjectchange((INTBIG)us_tool, VTOOL);
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_red_key, (INTBIG)redmap,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_green_key, (INTBIG)greenmap,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		(void)setvalkey((INTBIG)us_tool, VTOOL, us_colormap_blue_key, (INTBIG)bluemap,
			VINTEGER|VISARRAY|(256<<VLENGTHSH));
		endobjectchange((INTBIG)us_tool, VTOOL);

		/* re-enable option tracking */
		var = getvalkey((INTBIG)us_tool, VTOOL, VINTEGER, us_ignoreoptionchangeskey);
		if (var != NOVARIABLE)
			(void)delvalkey((INTBIG)us_tool, VTOOL, us_ignoreoptionchangeskey);
	}
}

/*
 * routine to parse the layer cell in "np" and fill these reference descriptors:
 *   "desc" (a GRAPHICS structure)
 *   "cif" (the name of the CIF layer)
 *   "dxf" (the name of the DXF layer)
 *   "func" (the integer function number)
 *   "layerletters" (the letters associated with this layer),
 *   "gds" (the Calma GDS-II layer number)
 *   "spires" (the SPICE resistance)
 *   "spicap" (the SPICE capacitance)
 *   "spiecap" (the SPICE edge capacitance)
 *   "drcminwid" (the DRC minimum width)
 *   "height3d" (the 3D height)
 *   "thick3d" (the 3D thickness)
 *   "printcol" (the printer colors)
 * All of the reference parameters except "func", "spires", "spicap", and "spiecap"
 * get allocated.  Returns true on error.
 */
BOOLEAN us_teceditgetlayerinfo(NODEPROTO *np, GRAPHICS *desc, CHAR **cif, INTBIG *func,
	CHAR **layerletters, CHAR **dxf, CHAR **gds, float *spires, float *spicap,
	float *spiecap, INTBIG *drcminwid, INTBIG *height3d, INTBIG *thick3d, INTBIG *printcol)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG patterncount, i, color, len;
	REGISTER INTBIG lowx, highx, lowy, highy, x, y;
	REGISTER CHAR *str;
	REGISTER VARIABLE *var, *varkey;

	/* create and initialize the GRAPHICS structure */
	desc->colstyle = SOLIDC;
	desc->bwstyle = PATTERNED;
	for(i=0; i<16; i++) desc->raster[i] = 0;

	/* look at all nodes in the layer description cell */
	patterncount = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		varkey = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
		if (varkey == NOVARIABLE) continue;
		var = getvalkey((INTBIG)ni, VNODEINST, VSTRING, art_messagekey);
		if (var == NOVARIABLE) str = ""; else
		{
			str = (CHAR *)var->addr;
			while (*str != 0 && *str != ':') str++;
			if (*str == ':') str++;
			while (*str == ' ') str++;
		}

		switch (varkey->addr)
		{
			case LAYERPATTERN:
				if (patterncount == 0)
				{
					lowx = ni->lowx;   highx = ni->highx;
					lowy = ni->lowy;   highy = ni->highy;
				} else
				{
					if (ni->lowx < lowx) lowx = ni->lowx;
					if (ni->highx > highx) highx = ni->highx;
					if (ni->lowy < lowy) lowy = ni->lowy;
					if (ni->highy > highy) highy = ni->highy;
				}
				patterncount++;
				break;
			case LAYERCOLOR:
				color = getecolor(str);
				if (color < 0)
				{
					ttyputerr(_("Unknown color '%s' in %s"), str, describenodeproto(np));
					return(TRUE);
				}
				desc->col = color;
				switch (color)
				{
					case COLORT1: desc->bits = LAYERT1; break;
					case COLORT2: desc->bits = LAYERT2; break;
					case COLORT3: desc->bits = LAYERT3; break;
					case COLORT4: desc->bits = LAYERT4; break;
					case COLORT5: desc->bits = LAYERT5; break;
					default:      desc->bits = LAYERO;  break;
				}
				break;
			case LAYERSTYLE:
				if (namesame(str, x_("solid")) == 0)
					desc->colstyle = desc->bwstyle = SOLIDC;
				if (namesame(str, x_("patterned")) == 0)
					desc->colstyle = desc->bwstyle = PATTERNED;
				if (namesame(str, x_("patterned/outlined")) == 0)
					desc->colstyle = desc->bwstyle = PATTERNED | OUTLINEPAT;
				break;
			case LAYERCIF:
				if (allocstring(cif, str, us_tool->cluster)) return(TRUE);
				break;
			case LAYERDXF:
				if (allocstring(dxf, str, us_tool->cluster)) return(TRUE);
				break;
			case LAYERGDS:
				if (namesame(str, x_("-1")) == 0) str = x_("");
				if (allocstring(gds, str, us_tool->cluster)) return(TRUE);
				break;
			case LAYERFUNCTION:
				*func = us_teceditparsefun(str);
				break;
			case LAYERLETTERS:
				if (allocstring(layerletters, str, us_tool->cluster)) return(TRUE);
				break;
			case LAYERSPIRES:
				*spires = (float)eatof(str);
				break;
			case LAYERSPICAP:
				*spicap = (float)eatof(str);
				break;
			case LAYERSPIECAP:
				*spiecap = (float)eatof(str);
				break;
			case LAYERDRCMINWID:
				*drcminwid = atofr(str);
				break;
			case LAYER3DHEIGHT:
				*height3d = myatoi(str);
				break;
			case LAYER3DTHICK:
				*thick3d = myatoi(str);
				break;
			case LAYERPRINTCOL:
				us_teceditgetprintcol(var, &printcol[0], &printcol[1], &printcol[2],
					&printcol[3], &printcol[4]);
				break;
		}
	}

	if (patterncount != 16*16 && patterncount != 16*8)
	{
		ttyputerr(_("Incorrect number of pattern boxes in %s (has %ld, not %d)"),
			describenodeproto(np), patterncount, 16*16);
		return(TRUE);
	}

	/* construct the pattern */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != art_filledboxprim) continue;
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
		if (var == NOVARIABLE) continue;
		if (var->addr != LAYERPATTERN) continue;
		var = getvalkey((INTBIG)ni, VNODEINST, VSHORT|VISARRAY, art_patternkey);
		if (var != NOVARIABLE)
		{
			len = getlength(var);
			for(i=0; i<len; i++) if (((INTSML *)var->addr)[i] != 0) break;
			if (i >= len) continue;
		}
		x = (ni->lowx - lowx) / ((highx-lowx) / 16);
		y = (highy - ni->highy) / ((highy-lowy) / 16);
		desc->raster[y] |= (1 << (15-x));
	}
	if (patterncount == 16*8)
	{
		/* older, half-height pattern: replicate it */
		for(y=0; y<8; y++)
			desc->raster[y+8] = desc->raster[y];
	}
	return(FALSE);
}

void us_teceditgetprintcol(VARIABLE *var, INTBIG *r, INTBIG *g, INTBIG *b, INTBIG *o, INTBIG *f)
{
	REGISTER CHAR *pt;

	/* set default values */
	*r = *g = *b = *o = *f = 0;
	if (var == NOVARIABLE) return;

	/* skip the header */
	pt = (CHAR *)var->addr;
	while (*pt != 0 && *pt != ':') pt++;
	if (*pt == ':') pt++;

	/* get red */
	while (*pt == ' ') pt++;
	*r = myatoi(pt);
	while (*pt != 0 && *pt != ',') pt++;
	if (*pt == ',') pt++;

	/* get green */
	while (*pt == ' ') pt++;
	*g = myatoi(pt);
	while (*pt != 0 && *pt != ',') pt++;
	if (*pt == ',') pt++;

	/* get blue */
	while (*pt == ' ') pt++;
	*b = myatoi(pt);
	while (*pt != 0 && *pt != ',') pt++;
	if (*pt == ',') pt++;

	/* get opacity */
	while (*pt == ' ') pt++;
	*o = myatoi(pt);
	while (*pt != 0 && *pt != ',') pt++;
	if (*pt == ',') pt++;

	/* get foreground */
	while (*pt == ' ') pt++;
	if (namesamen(pt, x_("on"), 2) == 0) *f = 1; else
		if (namesamen(pt, x_("off"), 3) == 0) *f = 0; else
			*f = myatoi(pt);
}

/*
 * Routine to set the layer-pattern squares of cell "np" to the bits in "desc".
 */
void us_teceditsetlayerpattern(NODEPROTO *np, GRAPHICS *desc)
{
	REGISTER NODEINST *ni;
	REGISTER INTBIG patterncount;
	REGISTER INTBIG lowx, highx, lowy, highy, x, y;
	REGISTER INTSML wantcolor, color;
	REGISTER VARIABLE *var;

	/* look at all nodes in the layer description cell */
	patterncount = 0;
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto == art_boxprim || ni->proto == art_filledboxprim)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
			if (var == NOVARIABLE) continue;
			if (var->addr != LAYERPATTERN) continue;
			if (patterncount == 0)
			{
				lowx = ni->lowx;   highx = ni->highx;
				lowy = ni->lowy;   highy = ni->highy;
			} else
			{
				if (ni->lowx < lowx) lowx = ni->lowx;
				if (ni->highx > highx) highx = ni->highx;
				if (ni->lowy < lowy) lowy = ni->lowy;
				if (ni->highy > highy) highy = ni->highy;
			}
			patterncount++;
		}
	}

	if (patterncount != 16*16 && patterncount != 16*8)
	{
		ttyputerr(_("Incorrect number of pattern boxes in %s (has %ld, not %d)"),
			describenodeproto(np), patterncount, 16*16);
		return;
	}

	/* set the pattern */
	for(ni = np->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		if (ni->proto != art_boxprim && ni->proto != art_filledboxprim) continue;
		var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
		if (var == NOVARIABLE) continue;
		if (var->addr != LAYERPATTERN) continue;

		x = (ni->lowx - lowx) / ((highx-lowx) / 16);
		y = (highy - ni->highy) / ((highy-lowy) / 16);
		if ((desc->raster[y] & (1 << (15-x))) == 0) wantcolor = 0;
			else wantcolor = (INTSML)0xFFFF;

		color = us_tecedlayergetpattern(ni);
		if (color != wantcolor)
			us_tecedlayersetpattern(ni, wantcolor);
	}
}

/*
 * routine to parse the layer function string "str" and return the
 * actual function codes
 */
INTBIG us_teceditparsefun(CHAR *str)
{
	REGISTER INTBIG func, save, i;
	REGISTER CHAR *pt;

	func = 0;
	for(;;)
	{
		/* find the next layer function name */
		pt = str;
		while (*pt != 0 && *pt != ',') pt++;

		/* parse the name */
		save = *pt;
		*pt = 0;
		for(i=0; us_teclayer_functions[i].name != 0; i++)
			if (namesame(str, us_teclayer_functions[i].name) == 0) break;
		*pt = (CHAR)save;
		if (us_teclayer_functions[i].name == 0)
		{
			ttyputerr(_("Unknown layer function: %s"), str);
			return(0);
		}

		/* mix in the layer function */
		if (us_teclayer_functions[i].value <= LFTYPE)
		{
			if (func != 0)
			{
				ttyputerr(_("Cannot be both function %s and %s"),
					us_teclayer_functions[func&LFTYPE].name, us_teclayer_functions[i].name);
				func = 0;
			}
			func = us_teclayer_functions[i].value;
		} else func |= us_teclayer_functions[i].value;

		/* advance to the next layer function name */
		if (*pt == 0) break;
		str = pt + 1;
	}
	return(func);
}

/*
 * routine to return the option index of node "ni"
 */
INTBIG us_tecedgetoption(NODEINST *ni)
{
	REGISTER VARIABLE *var, *var2;
	REGISTER NODEPROTO *np;

	/* port objects are readily identifiable */
	if (ni->proto == gen_portprim) return(PORTOBJ);

	/* center objects are also readily identifiable */
	if (ni->proto == gen_cellcenterprim) return(CENTEROBJ);

	var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER, us_edtec_option_key);
	if (var == NOVARIABLE) return(-1);
	if (var->addr == LAYERPATCH)
	{
		/* may be a highlight object */
		var2 = getval((INTBIG)ni, VNODEINST, VNODEPROTO, x_("EDTEC_layer"));
		if (var2 != NOVARIABLE)
		{
			np = (NODEPROTO *)var2->addr;
			if (np == NONODEPROTO) return(HIGHLIGHTOBJ);
		}
	}
	return(var->addr);
}

/*
 * Routine called when cell "np" has been deleted (and it may be a layer cell because its name
 * started with "layer-").
 */
void us_teceddeletelayercell(NODEPROTO *np)
{
	REGISTER VARIABLE *var;
	REGISTER NODEPROTO *onp;
	REGISTER NODEINST *ni;
	REGISTER BOOLEAN warned, isnode;
	REGISTER CHAR *layername;
	static INTBIG edtec_layer_key = 0;
	REGISTER void *infstr;

	/* may have deleted layer cell in technology library */
	if (edtec_layer_key == 0) edtec_layer_key = makekey(x_("EDTEC_layer"));
	layername = &np->protoname[6];
	warned = FALSE;
	for(onp = np->lib->firstnodeproto; onp != NONODEPROTO; onp = onp->nextnodeproto)
	{
		if (namesamen(onp->protoname, x_("node-"), 5) == 0) isnode = TRUE; else
			if (namesamen(onp->protoname, x_("arc-"), 4) == 0) isnode = FALSE; else
				continue;
		for(ni = onp->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
		{
			var = getvalkey((INTBIG)ni, VNODEINST, VNODEPROTO, edtec_layer_key);
			if (var == NOVARIABLE) continue;
			if ((NODEPROTO *)var->addr == np) break;
		}
		if (ni != NONODEINST)
		{
			if (warned) addtoinfstr(infstr, ','); else
			{
				infstr = initinfstr();
				formatinfstr(infstr, _("Warning: layer %s is used in"), layername);
				warned = TRUE;
			}
			if (isnode) formatinfstr(infstr, _(" node %s"), &onp->protoname[5]); else
				formatinfstr(infstr, _(" arc %s"), &onp->protoname[4]);
		}
	}
	if (warned)
		ttyputmsg(x_("%s"), returninfstr(infstr));

	/* see if this layer is mentioned in the design rules */
	us_tecedrenamecell(np->protoname, x_(""));
}

/*
 * Routine called when cell "np" has been deleted (and it may be a node cell because its name
 * started with "node-").
 */
void us_teceddeletenodecell(NODEPROTO *np)
{
	/* see if this node is mentioned in the design rules */
	us_tecedrenamecell(np->protoname, x_(""));
}

/******************** SUPPORT FOR "usredtecp.c" ROUTINES ********************/

/*
 * routine to return the actual bounding box of layer node "ni" in the
 * reference variables "lx", "hx", "ly", and "hy"
 */
void us_tecedgetbbox(NODEINST *ni, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER INTBIG twolambda;

	*lx = ni->geom->lowx;
	*hx = ni->geom->highx;
	*ly = ni->geom->lowy;
	*hy = ni->geom->highy;
	if (ni->proto != gen_portprim) return;
	twolambda = lambdaofnode(ni) * 2;
	*lx += twolambda;   *hx -= twolambda;
	*ly += twolambda;   *hy -= twolambda;
}

void us_tecedpointout(NODEINST *ni, NODEPROTO *np)
{
	REGISTER WINDOWPART *w;
	CHAR *newpar[2];

	for(w = el_topwindowpart; w != NOWINDOWPART; w = w->nextwindowpart)
		if (w->curnodeproto == np) break;
	if (w == NOWINDOWPART)
	{
		newpar[0] = describenodeproto(np);
		us_editcell(1, newpar);
	}
	if (ni != NONODEINST)
	{
		us_clearhighlightcount();
		(void)asktool(us_tool, x_("show-object"), (INTBIG)ni->geom);
	}
}

/*
 * routine to swap entries "p1" and "p2" of the port list in "tlist"
 */
void us_tecedswapports(INTBIG *p1, INTBIG *p2, TECH_NODES *tlist)
{
	REGISTER INTBIG temp, *templ;
	REGISTER CHAR *tempc;

	templ = tlist->portlist[*p1].portarcs;
	tlist->portlist[*p1].portarcs = tlist->portlist[*p2].portarcs;
	tlist->portlist[*p2].portarcs = templ;

	tempc = tlist->portlist[*p1].protoname;
	tlist->portlist[*p1].protoname = tlist->portlist[*p2].protoname;
	tlist->portlist[*p2].protoname = tempc;

	temp = tlist->portlist[*p1].initialbits;
	tlist->portlist[*p1].initialbits = tlist->portlist[*p2].initialbits;
	tlist->portlist[*p2].initialbits = temp;

	temp = tlist->portlist[*p1].lowxmul;
	tlist->portlist[*p1].lowxmul = tlist->portlist[*p2].lowxmul;
	tlist->portlist[*p2].lowxmul = (INTSML)temp;
	temp = tlist->portlist[*p1].lowxsum;
	tlist->portlist[*p1].lowxsum = tlist->portlist[*p2].lowxsum;
	tlist->portlist[*p2].lowxsum = (INTSML)temp;

	temp = tlist->portlist[*p1].lowymul;
	tlist->portlist[*p1].lowymul = tlist->portlist[*p2].lowymul;
	tlist->portlist[*p2].lowymul = (INTSML)temp;
	temp = tlist->portlist[*p1].lowysum;
	tlist->portlist[*p1].lowysum = tlist->portlist[*p2].lowysum;
	tlist->portlist[*p2].lowysum = (INTSML)temp;

	temp = tlist->portlist[*p1].highxmul;
	tlist->portlist[*p1].highxmul = tlist->portlist[*p2].highxmul;
	tlist->portlist[*p2].highxmul = (INTSML)temp;
	temp = tlist->portlist[*p1].highxsum;
	tlist->portlist[*p1].highxsum = tlist->portlist[*p2].highxsum;
	tlist->portlist[*p2].highxsum = (INTSML)temp;

	temp = tlist->portlist[*p1].highymul;
	tlist->portlist[*p1].highymul = tlist->portlist[*p2].highymul;
	tlist->portlist[*p2].highymul = (INTSML)temp;
	temp = tlist->portlist[*p1].highysum;
	tlist->portlist[*p1].highysum = tlist->portlist[*p2].highysum;
	tlist->portlist[*p2].highysum = (INTSML)temp;

	/* finally, swap the actual identifiers */
	temp = *p1;   *p1 = *p2;   *p2 = temp;
}

CHAR *us_tecedsamplename(NODEPROTO *layernp)
{
	if (layernp == gen_portprim) return(x_("PORT"));
	if (layernp == gen_cellcenterprim) return(x_("GRAB"));
	if (layernp == NONODEPROTO) return(x_("HIGHLIGHT"));
	return(&layernp->protoname[6]);
}

/* Technology Edit Reorder */
static DIALOGITEM us_tecedredialogitems[] =
{
 /*  1 */ {0, {376,208,400,288}, BUTTON, N_("OK")},
 /*  2 */ {0, {344,208,368,288}, BUTTON, N_("Cancel")},
 /*  3 */ {0, {28,4,404,200}, SCROLL, x_("")},
 /*  4 */ {0, {4,4,20,284}, MESSAGE, x_("")},
 /*  5 */ {0, {168,208,192,268}, BUTTON, N_("Up")},
 /*  6 */ {0, {212,208,236,268}, BUTTON, N_("Down")},
 /*  7 */ {0, {136,208,160,280}, BUTTON, N_("Far Up")},
 /*  8 */ {0, {244,208,268,280}, BUTTON, N_("Far Down")}
};
static DIALOG us_tecedredialog = {{75,75,488,373}, N_("Reorder Technology Primitives"), 0, 8, us_tecedredialogitems, 0, 0};

/* special items for the "Reorder Primitives" dialog: */
#define DTER_LIST           3		/* List of primitives (scroll) */
#define DTER_TITLE          4		/* Primitive title (message) */
#define DTER_UP             5		/* Move Up (button) */
#define DTER_DOWN           6		/* Move Down (button) */
#define DTER_FARUP          7		/* Move Far Up (button) */
#define DTER_FARDOWN        8		/* Move Far Down (button) */

void us_reorderprimdlog(CHAR *type, CHAR *prefix, CHAR *varname)
{
	REGISTER INTBIG itemHit, i, j, total, len, amt;
	CHAR line[100], **seqname;
	REGISTER BOOLEAN changed;
	LIBRARY *thelib[1];
	NODEPROTO **sequence, *np;
	REGISTER void *dia;

	dia = DiaInitDialog(&us_tecedredialog);
	if (dia == 0) return;
	DiaInitTextDialog(dia, DTER_LIST, DiaNullDlogList, DiaNullDlogItem,
		DiaNullDlogDone, -1, SCSELMOUSE);
	esnprintf(line, 100, _("%s in technology %s"), type, el_curlib->libname);
	DiaSetText(dia, DTER_TITLE, line);
	thelib[0] = el_curlib;
	total = us_teceditfindsequence(thelib, 1, prefix, varname, &sequence);
	len = strlen(prefix);
	for(i=0; i<total; i++)
		DiaStuffLine(dia, DTER_LIST, &sequence[i]->protoname[len]);
	DiaSelectLine(dia, DTER_LIST, 0);

	changed = FALSE;
	for(;;)
	{
		itemHit = DiaNextHit(dia);
		if (itemHit == OK || itemHit == CANCEL) break;
		if (itemHit == DTER_UP || itemHit == DTER_FARUP)
		{
			/* shift up */
			if (itemHit == DTER_UP) amt = 1; else amt = 10;
			for(j=0; j<amt; j++)
			{
				i = DiaGetCurLine(dia, DTER_LIST);
				if (i <= 0) break;
				np = sequence[i];
				sequence[i] = sequence[i-1];
				sequence[i-1] = np;
				DiaSetScrollLine(dia, DTER_LIST, i, &sequence[i]->protoname[len]);
				DiaSetScrollLine(dia, DTER_LIST, i-1, &sequence[i-1]->protoname[len]);
			}
			changed = TRUE;
			continue;
		}
		if (itemHit == DTER_DOWN || itemHit == DTER_FARDOWN)
		{
			/* shift down */
			if (itemHit == DTER_DOWN) amt = 1; else amt = 10;
			for(j=0; j<amt; j++)
			{
				i = DiaGetCurLine(dia, DTER_LIST);
				if (i >= total-1) continue;
				np = sequence[i];
				sequence[i] = sequence[i+1];
				sequence[i+1] = np;
				DiaSetScrollLine(dia, DTER_LIST, i, &sequence[i]->protoname[len]);
				DiaSetScrollLine(dia, DTER_LIST, i+1, &sequence[i+1]->protoname[len]);
			}
			changed = TRUE;
			continue;
		}
	}

	/* preserve order */
	if (itemHit == OK && changed)
	{
		seqname = (CHAR **)emalloc(total * (sizeof (CHAR *)), el_tempcluster);
		for(i=0; i<total; i++)
			seqname[i] = &sequence[i]->protoname[len];
		setval((INTBIG)el_curlib, VLIBRARY, varname, (INTBIG)seqname,
			VSTRING|VISARRAY|(total<<VLENGTHSH));
		efree((CHAR *)seqname);
	}
	efree((CHAR *)sequence);
	DiaDoneDialog(dia);
}
