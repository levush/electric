/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: iovhdl.c
 * Input/output tool: VHDL input
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

#include "config.h"
#if VHDLTOOL

#include "global.h"
#include "eio.h"
#include "edialogs.h"

#define MAXCHARS 300

static INTBIG io_vhdllength;
static INTBIG io_vhdlcurline;

static void io_vhdlreadfile(LIBRARY *lib, FILE *io, void *dia);

/*
 * Routine to read the VHDL file into library "lib".  Returns true on error.
 */
BOOLEAN io_readvhdllibrary(LIBRARY *lib)
{
	REGISTER FILE *io;
	CHAR *pt;
	void *dia;

	/* get the VHDL file */
	if ((io = xopen(lib->libfile, io_filetypevhdl, x_(""), &pt)) == NULL)
	{
		ttyputerr(_("File %s not found"), lib->libfile);
		return(TRUE);
	}

	/* determine file length */
	io_vhdllength = filesize(io);
	if (io_verbose < 0 && io_vhdllength > 0)
	{
		dia = DiaInitProgress(0, 0);
		if (dia == 0)
		{
			xclose(io);
			return(TRUE);
		}
		DiaSetProgress(dia, 0, io_vhdllength);
	} else dia = 0;

	io_vhdlreadfile(lib, io, dia);

	xclose(io);
	if (io_verbose < 0 && io_vhdllength > 0)
	{
		DiaSetProgress(dia, io_vhdllength, io_vhdllength);
		DiaDoneProgress(dia);
	}
	return(FALSE);
}

void io_vhdlreadfile(LIBRARY *lib, FILE *io, void *dia)
{
	CHAR *pt, *start, text[MAXCHARS], cellname[256], endtag[256];
	REGISTER CHAR save;
	enum {SECTIONNONE, SECTIONCOMPONENT, SECTIONENTITY, SECTIONARCHITECTURE} sectiontype;
	REGISTER INTBIG i, len, curLength, sectionnamelen;
	REGISTER NODEPROTO *storagecell;
	void *stringarray;
	REGISTER VARIABLE *var;

	/* read the file */
	io_vhdlcurline = 0;
	storagecell = NONODEPROTO;
	for(;;)
	{
		if (stopping(STOPREASONVHDL)) break;
		if (xfgets(text, MAXCHARS, io)) break;
		io_vhdlcurline++;
		if ((io_vhdlcurline % 100) == 0)
		{
			curLength = xtell(io);
			DiaSetProgress(dia, curLength, io_vhdllength);
		}

		if (storagecell == NONODEPROTO)
		{
			/* scan for start of cell */
			pt = text;
			while (*pt == ' ' || *pt == '\t') pt++;
			sectiontype = SECTIONNONE;
			if (namesamen(pt, x_("component"), (sectionnamelen = 9)) == 0)
				sectiontype = SECTIONCOMPONENT;
			else if (namesamen(pt, x_("entity"), (sectionnamelen = 6)) == 0)
				sectiontype = SECTIONENTITY;
			else if (namesamen(pt, x_("architecture"), (sectionnamelen = 12)) == 0)
				sectiontype = SECTIONARCHITECTURE;
			if (sectiontype != SECTIONNONE)
			{
				pt += sectionnamelen;
				if (*pt == ' ' || *pt == '\t')
				{
					while (*pt == ' ' || *pt == '\t') pt++;
					start = pt;
					while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
					save = *pt;
					*pt = 0;
					estrcpy(endtag, start);
					estrcpy(cellname, start);
					*pt = save;

					if (sectiontype == SECTIONARCHITECTURE)
					{
						while (*pt == ' ' || *pt == '\t') pt++;
						if (namesamen(pt, x_("of"), 2) == 0)
						{
							pt += 2;
							while (*pt == ' ' || *pt == '\t') pt++;
							start = pt;
							while (*pt != 0 && *pt != ' ' && *pt != '\t') pt++;
							save = *pt;
							*pt = 0;
							estrcpy(cellname, start);
							*pt = save;
						}
					}

					for(storagecell = lib->firstnodeproto; storagecell != NONODEPROTO;
						storagecell = storagecell->nextnodeproto)
							if (namesame(cellname, storagecell->protoname) == 0 &&
								storagecell->cellview == el_vhdlview) break;

					/* see if the cell already exists */
					if (storagecell != NONODEPROTO)
					{
						/* already there: start with the existing VHDL */
						stringarray = newstringarray(io_tool->cluster);
						var = getvalkey((INTBIG)storagecell, VNODEPROTO, VSTRING|VISARRAY,
							el_cell_message_key);
						if (var != NOVARIABLE)
						{
							len = getlength(var);
							for(i=0; i<len; i++)
								addtostringarray(stringarray, ((CHAR **)var->addr)[i]);
							addtostringarray(stringarray, x_(""));
						}
					} else
					{
						estrcat(cellname, x_("{vhdl}"));
						storagecell = newnodeproto(cellname, lib);
						if (storagecell == NONODEPROTO)
						{
							ttyputerr(_("Could not create cell %s"), cellname);
							return;
						}
						stringarray = newstringarray(io_tool->cluster);
					}
				}
			}
			if (storagecell == NONODEPROTO) continue;
		}

		/* save this line in the cell */
		addtostringarray(stringarray, text);

		/* see if this is the last line */
		pt = text;
		while (*pt == ' ' || *pt == '\t') pt++;
		if (namesamen(pt, x_("end"), 3) == 0)
		{
			pt += 3;
			if (*pt == ' ' || *pt == '\t')
			{
				while (*pt == ' ' || *pt == '\t') pt++;
				switch (sectiontype)
				{
					case SECTIONCOMPONENT:
						if (namesamen(pt, x_("component"), 9) == 0)
						{
							stringarraytotextcell(stringarray, storagecell, TRUE);
							killstringarray(stringarray);
							storagecell = NONODEPROTO;
						}
						break;
					case SECTIONENTITY:
					case SECTIONARCHITECTURE:
						start = pt;
						while (*pt != ';' && *pt != ' ' && *pt != '\t' && *pt != 0) pt++;
						save = *pt;   *pt = 0;
						if (namesame(start, endtag) == 0)
						{
							stringarraytotextcell(stringarray, storagecell, TRUE);
							killstringarray(stringarray);
							storagecell = NONODEPROTO;
						}
						*pt = save;
						break;
					default:
						break;
				}
			}
		}
	}
}

#endif  /* VHDLTOOL - at top */
