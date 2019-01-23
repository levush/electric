/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: placmos.h
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

#define PLAC_MAX_COL_SIZE 500	 /* maximum number of columns */
#define PLAC_X_SEP         10	 /* Lambda seperation between grid lines in array */
#define PLAC_Y_MIR_SEP     10	 /* Lambda seperation between grid line */
#define PLAC_Y_SEP         10	 /* Lambda seperation between grid lines in array */


#define NOUCITEM ((UCITEM *)-1)

typedef struct IUCitem
{
	INTBIG          value;
	NODEINST       *nodeinst;
	struct IUCitem *rightitem;
	struct IUCitem *bottomitem;
} UCITEM;


#define NOUCROW ((UCROW *)-1)

/* Row and column pointers */
typedef struct IUCrow
{
	UCITEM *firstitem;
	UCITEM *lastitem;
} UCROW;

extern UCROW plac_col_list[PLAC_MAX_COL_SIZE];
extern UCROW plac_row_list[PLAC_MAX_COL_SIZE][3];

/* prototypes for intramodule routines */
NODEINST *plac_make_instance(NODEPROTO*, NODEPROTO*, INTBIG, INTBIG, INTBIG);
void plac_wire(CHAR*, INTBIG, NODEINST*, PORTPROTO*, NODEINST*, PORTPROTO*, NODEPROTO*);
NODEINST *plac_make_Pin(NODEPROTO*, INTBIG, INTBIG, INTBIG, CHAR*);
NODEPROTO *plac_pmos_grid(LIBRARY*, FILE*, CHAR[]);
NODEPROTO *plac_nmos_grid(LIBRARY*, FILE*, CHAR[]);
NODEPROTO *plac_decode_gen(LIBRARY*, NODEPROTO*, NODEPROTO*, CHAR*, INTBIG);
NODEPROTO *plac_or_plane(LIBRARY*, NODEPROTO*, CHAR*, INTBIG);
NODEPROTO *plac_make_pla(LIBRARY*, NODEPROTO*, NODEPROTO*, CHAR*);
INTBIG plac_read_hw(FILE*, INTBIG*, INTBIG*, INTBIG*, INTBIG*);
INTBIG plac_read_rows(INTBIG[], INTBIG, INTBIG, FILE*);
UCITEM *plac_newUCITEM(void);
