/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: usrdiacom.h
 * Special command dialogs, headers
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

INTBIG us_3ddepthdlog();
INTBIG us_aboutdlog(void);
INTBIG us_alignmentdlog(void);
INTBIG us_annularringdlog(void);
INTBIG us_arcsizedlog(CHAR*[]);
INTBIG us_arraydlog(CHAR*[]);
INTBIG us_artlookdlog(void);
INTBIG us_attrenumdlog(void);
INTBIG us_attributesdlog(void);
INTBIG us_attrparamdlog(void);
INTBIG us_attrreportdlog(void);
BOOLEAN us_colormixdlog(CHAR *overlayernames[], INTBIG layercount, CHAR **layernames, INTBIG *printcolors);
INTBIG us_copycelldlog(CHAR*);
INTBIG us_copyrightdlog(void);
INTBIG us_createtodlog(CHAR*[]);
INTBIG us_defarcdlog(void);
INTBIG us_defnodedlog(void);
INTBIG us_deftextdlog(CHAR*);
INTBIG us_dependentlibdlog(void);
INTBIG us_editcelldlog(CHAR*);
INTBIG us_examineoptionsdlog(void);
INTBIG us_celldlog(void);
INTBIG us_celllist(void);
INTBIG us_cellselect(CHAR*, CHAR*[], INTBIG);
INTBIG us_findoptionsdlog(void);
INTBIG us_findtextdlog(void);
INTBIG us_frameoptionsdlog(void);
INTBIG us_generaloptionsdlog(void);
INTBIG us_globalsignaldlog(void);
INTBIG us_griddlog(void);
INTBIG us_helpdlog(CHAR*);
INTBIG us_highlayerlog(void);
INTBIG us_iconstyledlog(void);
INTBIG us_javaoptionsdlog(void);
INTBIG us_lambdadlog(void);
INTBIG us_librarypathdlog(void);
INTBIG us_libtotechnologydlog(void);
INTBIG us_menudlog(CHAR*[]);
INTBIG us_modtextsizedlog(void);
INTBIG us_newviewdlog(CHAR*[]);
INTBIG us_nodesizedlog(CHAR*[]);
INTBIG us_noyesalwaysdlog(CHAR*, CHAR*[]);
INTBIG us_noyescanceldlog(CHAR*, CHAR*[]);
INTBIG us_noyesdlog(CHAR*, CHAR*[]);
INTBIG us_oldlibrarydlog(void);
INTBIG us_optionsavingdlog(void);
INTBIG us_patterndlog(void);
INTBIG us_placetextdlog(void);
INTBIG us_plotoptionsdlog(void);
INTBIG us_portdisplaydlog(void);
INTBIG us_portdlog(void);
INTBIG us_purelayernodedlog(CHAR*[]);
INTBIG us_quickkeydlog(void);
INTBIG us_quitdlog(CHAR*, INTBIG);
INTBIG us_renamedlog(INTBIG type);
INTBIG us_replacedlog(void);
INTBIG us_romgendlog(void);
INTBIG us_selectoptdlog(void);
INTBIG us_selectobjectdlog(INTBIG);
INTBIG us_showdlog(BOOLEAN);
INTBIG us_spreaddlog(void);
INTBIG us_technologydlog(CHAR*, CHAR*[]);
INTBIG us_techoptdlog(void);
INTBIG us_techvarsdlog(void);
INTBIG us_tracedlog(void);
INTBIG us_variablesdlog(void);
INTBIG us_viewdlog(INTBIG);
INTBIG us_visiblelayersdlog(CHAR*);
INTBIG us_windowviewdlog(void);
INTBIG us_yesnodlog(CHAR*, CHAR*[]);

typedef struct
{
	CHAR *name;
	CHAR *help;
} HELPERS;
extern HELPERS us_castofthousands[];
extern CHAR *us_capacitancenames[], *us_resistancenames[], *us_inductancenames[],
	*us_currentnames[], *us_voltagenames[], *us_timenames[];

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
