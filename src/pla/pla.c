/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: pla.c
 * Programmable Logic Array Generator
 * Written by: Sundaravarathan R. Iyengar, Schlumberger Palo Alto Research
 * Last modified by: Steven M. Rubin, Static Free Software
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
#if PLATOOL

#include "global.h"
#include "pla.h"
#include "planmos.h"

/* the PLA tool table */
static KEYWORD planmosonoffopt[] =
{
	{x_("on"),          0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("off"),         0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP planmosonoffp = {planmosonoffopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("State of this switch"), 0};
static COMCOMP planmosoptip = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Number of inputs to nMOS PLA"), 0};
static COMCOMP planmosoptop = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Number of outputs to nMOS PLA"), 0};
static COMCOMP planmosoptpp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Number of p-terms to nMOS PLA"), 0};
static COMCOMP planmosoptvp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Width of VDD wires in nMOS PLA"), 0};
static COMCOMP planmosoptgp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Width of Ground wires in nMOS PLA"), 0};
static COMCOMP planmosoptfp = {NOKEYWORD,topoffile,nextfile,NOPARAMS,
	INPUTOPT, x_(" \t"), M_("File name with nMOS PLA personality table"), 0};
static COMCOMP planmosoptbp = {NOKEYWORD, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("Name given to nMOS PLA"), 0};
static KEYWORD planmosoopt[] =
{
	{x_("file"),             1,{&planmosoptfp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("inputs"),           1,{&planmosoptip,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("outputs"),          1,{&planmosoptop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("pterms"),           1,{&planmosoptpp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("verbose"),          1,{&planmosonoffp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("flexible"),         1,{&planmosonoffp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("fixedangle"),       1,{&planmosonoffp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("buttingcontact"),   1,{&planmosonoffp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("samesideoutput"),   1,{&planmosonoffp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("vddwidth"),         1,{&planmosoptvp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("groundwidth"),      1,{&planmosoptgp,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("name"),             1,{&planmosoptbp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP planmosop = {planmosoopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("nMOS PLA switch setting option"), 0};
static KEYWORD placmosopt[] =
{
	{x_("n-plane"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("p-plane"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("decoder"),        0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("generate-pla"),   0,{NOKEY,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
static COMCOMP placmosp = {placmosopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	INPUTOPT, x_(" \t"), M_("CMOS PLA action"), 0};
static KEYWORD plaopt[] =
{
	{x_("nmos"),    1,{&planmosop,NOKEY,NOKEY,NOKEY,NOKEY}},
	{x_("cmos"),    1,{&placmosp,NOKEY,NOKEY,NOKEY,NOKEY}},
	TERMKEY
};
COMCOMP pla_plap = {plaopt, NOTOPLIST, NONEXTLIST, NOPARAMS,
	0, x_(" \t"), M_("Select a PLA generator to use"), 0};

TOOL     *pla_tool;				/* the PLA tool object */
INTBIG    pla_lam;				/* Feature size in internal units */
INTBIG    pla_filetypeplatab;	/* PLA table disk file descriptor */

/* PLA access routine called from electric user interface */
void pla_init(INTBIG *argc, CHAR1 *argv[], TOOL *thistool)
{
	Q_UNUSED( argc );
	Q_UNUSED( argv );

	/* only initialize during pass 1 */
	if (thistool == NOTOOL || thistool == 0) return;
	pla_tool = thistool;

	/* initialize the nMOS PLA generator */
	pla_fin = NULL;
	pla_verbose = FALSE;
	pla_userbits = FIXANG;
	pla_buttingcontact = TRUE;
	pla_samesideoutput = FALSE;
	pla_VddWidth = 0;
	pla_GndWidth = 0;
	(void)estrcpy(pla_infile, INFILE);
	(void)estrcpy(pla_name, DEFNAME);
	pla_alv = pla_atg = pla_arg = pla_otv = NOVDDGND;
	pla_org = pla_icg = pla_icv = pla_ocg = pla_ocv = NOVDDGND;

	/* setup file descriptor */
	pla_filetypeplatab = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("platab"), _("PLA table"));
}

/*
 * This routine is called each time the user issues the command,
 */
void pla_set(INTBIG count, CHAR *par[])
{
	REGISTER INTBIG len;
	REGISTER CHAR *pp;

	if (count == 0)
	{
		count = ttygetparam(M_("PLA option:"), &pla_plap, MAXPARS, par);
		if (count == 0)
		{
			ttyputerr(M_("Aborted"));
			return;
		}
	}
	len = estrlen(pp = par[0]);

	if (namesamen(pp, x_("nmos"), len) == 0)
	{
		if (count < 3)
		{
			pla_displayoptions();
			return;
		}
		pla_setoption(par[1], par[2]);
		return;
	}

	if (namesamen(pp, x_("cmos"), len) == 0)
	{
		if (count == 1)
		{
			count = ttygetparam(M_("CMOS PLA option:"), &placmosp, MAXPARS-1, &par[1]) + 1;
			if (count == 1)
			{
				ttyputerr(M_("Aborted"));
				return;
			}
		}

		len = estrlen(pp = par[1]);
		if (namesamen(pp, x_("generate-pla"), len) == 0)
		{
			plac_generate();
			return;
		}
		if (namesamen(pp, x_("decoder"), len) == 0)
		{
			plac_dec();
			return;
		}
		if (namesamen(pp, x_("n-plane"), len) == 0)
		{
			plac_n_generate();
			return;
		}
		if (namesamen(pp, x_("p-plane"), len) == 0)
		{
			plac_p_generate();
			return;
		}
		ttyputbadusage(x_("telltool pla cmos"));
		return;
	}

	/* Invalid control to the PLA generator.  Print a small summary */
	ttyputbadusage(x_("telltool pla"));
}

void pla_slice(void)
{
	if (pla_Make())
	{
		ttyputerr(_("nMOS PLA '%s' is incompletely defined"), pla_name);
		if (pla_fin != NULL) xclose(pla_fin);
		toolturnoff(pla_tool, FALSE);
		setactivity(_("Incomplete nMOS PLA generated"));
		return;
	}
	xclose(pla_fin);
	toolturnoff(pla_tool, FALSE);
	setactivity(_("nMOS PLA generated"));
}

void pla_done(void) {}

#endif  /* PLATOOL - at stop */
