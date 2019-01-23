/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: placio.c
 * PLA generator for CMOS
 * Written by: Wallace Kroeker at the University of Calgary
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
#include "placmos.h"

extern COMCOMP us_yesnop, us_colorreadp;		/* from usrcom.c */

/*
 * routine called for the command "telltool pla cmos generate-pla"
 */
void plac_generate(void)
{
	LIBRARY  *plac_lib;
	FILE     *and_array_file, *or_array_file;
	NODEPROTO *pmos_np, *nmos_np, *or_np, *or_plane_np, *decode_np;
	CHAR cell_name[100], and_file_name[100], *pt1, *params[MAXPARS], *filename;
	INTBIG INPUTS = FALSE, OUTPUTS = FALSE, count;
	REGISTER void *infstr;

	/* FUDGE library/lambda for now */
	plac_lib = el_curlib;
	pla_lam = plac_lib->lambda[el_curtech->techindex];

	/* prompt the user for some information */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("platab/"));
	addstringtoinfstr(infstr, _("AND Plane Input File"));
	count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS, params);
	if (count == 0)
	{
		ttyputerr(_("Aborted: no array input file name given"));
		return;
	}
	(void)estrcpy(and_file_name, params[0]);
	and_array_file = xopen(and_file_name, pla_filetypeplatab, x_(""), &filename);
	if (and_array_file == NULL)
	{
		ttyputerr(_("Aborted: unable to open file %s"), and_file_name);
		return;
	}

	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("platab/"));
	addstringtoinfstr(infstr, _("OR Plane Input File"));
	count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS, params);
	if (count == 0)
	{
		ttyputerr(_("Aborted: no array input file name given"));
		return;
	}
	or_array_file = xopen(params[0], pla_filetypeplatab, x_(""), &filename);
	if (or_array_file == NULL)
	{
		ttyputerr(_("Aborted: unable to open file %s"), params[0]);
		return;
	}

	count = ttygetparam(_("INPUTS to the TOP of the AND plane? [y] "), &us_yesnop, MAXPARS, params);
	if (count >= 1 && params[0][0] == 'n') INPUTS = FALSE; else
		INPUTS = TRUE;

	count = ttygetparam(_("OUTPUTS from the BOTTOM of the OR plane? [y] "), &us_yesnop, MAXPARS, params);
	if (count >= 1 && params[0][0] == 'n') OUTPUTS = FALSE; else
		OUTPUTS = TRUE;

	/* last prompt of user, for cell name */
	pt1 = ttygetline(_("Cell name: "));
	if (pt1 == 0 || *pt1 == 0)
	{
		ttyputerr(_("Aborted: no array cell name given"));
		return;
	}
	(void)estrcpy(cell_name, pt1);

	/* generate the AND plane (Decode unit of a ROM) using cells from library */
	infstr = initinfstr();
	addstringtoinfstr(infstr, cell_name);
	addstringtoinfstr(infstr, x_("_p_cell"));
	pmos_np = plac_pmos_grid(plac_lib, and_array_file, returninfstr(infstr));
	xclose(and_array_file);
	if (pmos_np == NONODEPROTO) return;

	infstr = initinfstr();
	addstringtoinfstr(infstr, cell_name);
	addstringtoinfstr(infstr, x_("_n_cell"));
	and_array_file = xopen(and_file_name, pla_filetypeplatab, x_(""), &filename);
	if (and_array_file == NULL)
	{
		ttyputerr(_("Aborted: unable to open file %s"), and_file_name);
		return;
	}
	nmos_np = plac_nmos_grid(plac_lib, and_array_file, returninfstr(infstr));
	xclose(and_array_file);
	if (nmos_np == NONODEPROTO) return;

	infstr = initinfstr();
	addstringtoinfstr(infstr, cell_name);
	addstringtoinfstr(infstr, x_("_decode"));
	decode_np = plac_decode_gen(plac_lib, pmos_np, nmos_np, returninfstr(infstr), INPUTS);
	if (decode_np == NONODEPROTO) return;

	/** Generate the OR plane **/
	infstr = initinfstr();
	addstringtoinfstr(infstr, cell_name);
	addstringtoinfstr(infstr, x_("_or_cell"));
	or_np = plac_nmos_grid(plac_lib, or_array_file, returninfstr(infstr));
	xclose(or_array_file);
	if (or_np == NONODEPROTO) return;

	infstr = initinfstr();
	addstringtoinfstr(infstr, cell_name);
	addstringtoinfstr(infstr, x_("_or_plane"));
	or_plane_np = plac_or_plane(plac_lib,or_np,returninfstr(infstr),OUTPUTS);
	if (or_plane_np == NONODEPROTO) return;
	(void)plac_make_pla(plac_lib, decode_np, or_plane_np, cell_name);
}

/*
 * routine called for the command "telltool pla cmos decoder"
 */
void plac_dec(void)
{
	LIBRARY *plac_lib;
	FILE    *and_array_file;
	NODEPROTO *pmos_np, *nmos_np;
	CHAR cell_name[40], temp_name[40], and_file_name[100], *params[MAXPARS];
	INTBIG INPUTS = FALSE, count;
	CHAR *pt1, *filename;
	REGISTER void *infstr;

	/* FUDGE library/lambda for now */
	plac_lib = el_curlib;
	pla_lam = plac_lib->lambda[el_curtech->techindex];

	/* prompt the user for some information */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("platab/"));
	addstringtoinfstr(infstr, _("Decoder Plane Input File"));
	count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS, params);
	if (count == 0)
	{
		ttyputerr(_("Aborted: no array input file name given"));
		return;
	}
	(void)estrcpy(and_file_name, params[0]);

	count = ttygetparam(_("INPUTS to the TOP of the AND plane? [y] "), &us_yesnop, MAXPARS, params);
	if (count >= 1 && params[0][0] == 'n') INPUTS = FALSE; else
		INPUTS = TRUE;

	/* last prompt of user, for cell name */
	pt1 = ttygetline(_("Cell name: "));
	if (pt1 == 0 || *pt1 == 0)
	{
		ttyputerr(_("Aborted: no array cell name given"));
		return;
	}
	(void)estrcpy(cell_name, pt1);

	/* generate the AND plane (Decode unit of a ROM) using cells from library */
	(void)estrcpy(temp_name, cell_name);
	(void)estrcat(temp_name, x_("_p_cell"));
	and_array_file = xopen(and_file_name, pla_filetypeplatab, x_(""), &filename);
	if (and_array_file == NULL)
	{
		ttyputerr(_("Aborted: unable to open file %s"), and_file_name);
		return;
	}
	pmos_np = plac_pmos_grid(plac_lib, and_array_file, temp_name);
	xclose(and_array_file);
	if (pmos_np == NONODEPROTO) return;
	(void)estrcpy(temp_name, cell_name);
	(void)estrcat(temp_name, x_("_n_cell"));
	and_array_file = xopen(and_file_name, pla_filetypeplatab, x_(""), &filename);
	if (and_array_file == NULL)
	{
		ttyputerr(_("Aborted: unable to open file %s"), and_file_name);
		return;
	}
	nmos_np = plac_nmos_grid(plac_lib, and_array_file, temp_name);
	xclose(and_array_file);
	if (nmos_np == NONODEPROTO) return;
	(void)estrcpy(temp_name, cell_name);
	(void)estrcat(temp_name, x_("_decode"));
	(void)plac_decode_gen(plac_lib, pmos_np, nmos_np, temp_name, INPUTS);
}

/*
 * routine called for the command "telltool pla cmos p-plane"
 */
void plac_p_generate(void)
{
	LIBRARY *plac_lib;
	FILE *array_file;
	CHAR *pt1, *filename;
	CHAR cell_name[40], array_file_name[100], *params[MAXPARS];
	INTBIG count;
	REGISTER void *infstr;

	/* FUDGE library/lambda for now */
	plac_lib = el_curlib;
	pla_lam = plac_lib->lambda[el_curtech->techindex];

	/* prompt the user for some information */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("platab/"));
	addstringtoinfstr(infstr, _("Input File"));
	count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS, params);
	if (count == 0)
	{
		ttyputerr(_("Aborted: no array input file name given"));
		return;
	}
	(void)estrcpy(array_file_name, params[0]);

	/* last prompt of user, for cell name */
	pt1 = ttygetline(_("Cell name: "));
	if (pt1 == 0 || *pt1 == 0)
	{
		ttyputerr(_("Aborted: no array cell name given"));
		return;
	}
	(void)estrcpy(cell_name, pt1);

	/* generate pla_array_cell using cells from library */
	array_file = xopen(array_file_name, pla_filetypeplatab, x_(""), &filename);
	(void)plac_pmos_grid(plac_lib, array_file, cell_name);
	xclose(array_file);
}

/*
 * routine called for the command "telltool pla cmos n-plane"
 */
void plac_n_generate(void)
{
	LIBRARY *plac_lib;
	FILE *array_file;
	CHAR *pt1, *filename;
	CHAR cell_name[40], array_file_name[100], *params[MAXPARS];
	INTBIG count;
	REGISTER void *infstr;

	/* FUDGE library/lambda for now */
	plac_lib = el_curlib;
	pla_lam = plac_lib->lambda[el_curtech->techindex];

	/* prompt the user for some information */
	infstr = initinfstr();
	addstringtoinfstr(infstr, x_("platab/"));
	addstringtoinfstr(infstr, _("Input File"));
	count = ttygetparam(returninfstr(infstr), &us_colorreadp, MAXPARS, params);
	if (count == 0)
	{
		ttyputerr(_("Aborted: no array input file name given"));
		return;
	}
	(void)estrcpy(array_file_name, params[0]);

	/* last prompt of user, for cell name */
	pt1 = ttygetline(_("Cell name: "));
	if (pt1 == 0 || *pt1 == 0)
	{
		ttyputerr(_("Aborted: no array cell name given"));
		return;
	}
	(void)estrcpy(cell_name, pt1);

	/* generate pla_array_cell using cells from library */
	array_file = xopen(array_file_name, pla_filetypeplatab, x_(""), &filename);
	(void)plac_nmos_grid(plac_lib, array_file, cell_name);
	xclose(array_file);
}

INTBIG plac_read_rows(INTBIG row[], INTBIG width, INTBIG width_in, FILE *file)
{
	INTBIG i, eof, read, value;

	read = 0;
	eof = ~EOF;
	for (i = 0; ((i < width) && (eof != EOF)); i++)
	{
		/* Ground Strapping slot */
		if ((i % 5) == 0) row[i] = -2; else
			if (read < width_in)
		{
			eof = efscanf(file, x_("%ld"), &value);
			row[i] = value;
			read++;
		} else row[i] = -1;
	}

	/* hit end of file too soon ! */
	if ((i < width) && (eof == EOF))
		row[0] = -1;
	return(eof);
}

/* read in array size and place perimeter metal pins and metal-poly contacts */
INTBIG plac_read_hw(FILE *file, INTBIG *height, INTBIG *width, INTBIG *height_in,
	INTBIG *width_in)
{
	INTBIG eof, localhei, localwid;

	eof = efscanf(file, x_("%ld %ld"), &localhei, &localwid);
	*height = localhei;   *width = localwid;

	ttyputmsg(_("height = %ld, width = %ld"), *height, *width);

	*width_in = *width;
	*height_in = *height;
	*height = (((((*height - 1)/4)+1)*5)+1);
	if (*height > PLAC_MAX_COL_SIZE)
	{
		ttyputerr(_("PLA height exceeded"));
		return(EOF);
	}
	*width = (((((*width - 1)/4)+1)*5)+1);
	if (*width > PLAC_MAX_COL_SIZE)
	{
		ttyputerr(_("PLA width exceeded"));
		return(EOF);
	}
	return(eof);
}

#endif  /* PLATOOL - at top */
