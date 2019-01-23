/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbmult.c
 * Extended precision multiplication and division
 * Written by: Mark Brinsmead at the University of Calgary
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

/****************************** MULT ******************************/

/*
 * mult(a, b) returns (a*b)>>30, as a 32 bit result, where a, b are 32 bit
 * integers.
 */
INTBIG mult(INTBIG a, INTBIG b)
{
	INTHUGE c;

	c = a;
	c *= b;
	c >>= 30;
	return((INTBIG)c);
}

/****************************** MULDIV ******************************/

/*
 * return a*b/c, where a, b, c are 32 bit integers
 */
INTBIG muldiv(INTBIG ai, INTBIG bi, INTBIG ci)
{
	INTHUGE v;

	v = ai;
	v *= bi;
	v /= ci;
	return((INTBIG)v);
}
