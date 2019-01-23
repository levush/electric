/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dbmath.c
 * Database transformation and mathematics support module
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
#include "egraphics.h"
#include "efunction.h"
#include "tecart.h"
#include "tecgen.h"
#include "tecschem.h"
#include "usr.h"
#include "drc.h"
#include <math.h>
#include <float.h>

/* clipping directions */
#define LEFT    1
#define RIGHT   2
#define BOTTOM  4
#define TOP     8

static WINDOWPART  *db_windowpartfree = NOWINDOWPART;	/* top of free list of windows */
static INTBIG       db_rectx[4] = {0, 0, 1, 1};
static INTBIG       db_recty[4] = {0, 1, 1, 0};
static POLYGON     *db_staticpolygons = NOPOLYGON;

static INTBIG      *db_primearray;				/* array of small prime numbers */
static INTBIG       db_primearraytotal = 0;		/* size of small prime number array */
static INTBIG       db_maxcheckableprime;		/* max value that can be checked with array */
static POLYGON     *db_freepolygons = NOPOLYGON;
static void        *db_polygonmutex = 0;		/* mutex for polygon modules */
static void        *db_primenumbermutex = 0;	/* mutex for prime numbers */

/* prototypes for local routines */
static BOOLEAN  db_addpointtopoly(INTBIG x, INTBIG y, POLYGON *poly);
static void     db_findconnectionpoint(INTBIG, INTBIG, INTBIG, INTBIG, float, float, float, INTBIG*, INTBIG*);
static INTBIG   db_quadrant(INTBIG, INTBIG);
static void     db_closestpointtoline(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG*, INTBIG*);
static VIEW    *db_makepermanentview(CHAR*, CHAR*, INTBIG);
static void     db_clipedge(POLYGON*, POLYGON*, INTBIG, INTBIG);
static BOOLEAN  db_clip(INTBIG*, INTBIG*, INTBIG*, INTBIG*, INTBIG, INTBIG);
static BOOLEAN  db_lineintersect(INTBIG, INTBIG, INTBIG, INTBIG, POLYGON*);
static BOOLEAN  db_isprime(INTBIG);
static BOOLEAN  db_isachildof(NODEPROTO *parent, NODEPROTO *child);
static int      db_anglelistdescending(const void *e1, const void *e2);
static BOOLEAN  db_primeinit(void);
static BOOLEAN  db_extendprimetable(INTBIG length);
static void     db_deallocatepolygon(POLYGON *poly);

/*
 * Routine to free all memory associated with this module.
 */
void db_freemathmemory(void)
{
	REGISTER WINDOWPART *w;
	REGISTER POLYGON *poly;

	/* free all statically allocated polygons */
	while (db_staticpolygons != NOPOLYGON)
	{
		poly = db_staticpolygons;
		db_staticpolygons = db_staticpolygons->nextpolygon;
		freepolygon(poly);
	}

	/* cleanup cached polygons */
	while (db_freepolygons != NOPOLYGON)
	{
		poly = db_freepolygons;
		db_freepolygons = db_freepolygons->nextpolygon;
		db_deallocatepolygon(poly);
	}

	/* free all window partitions */
	while (el_topwindowpart != NOWINDOWPART)
	{
		w = el_topwindowpart;
		el_topwindowpart = el_topwindowpart->nextwindowpart;
		freewindowpart(w);
	}

	/* free unused pool of window partitions */
	while (db_windowpartfree != NOWINDOWPART)
	{
		w = db_windowpartfree;
		db_windowpartfree = db_windowpartfree->nextwindowpart;
		efree((CHAR *)w);
	}

	/* free miscellaneous allocations */
	efree((CHAR *)el_libdir);
	if (db_primearraytotal != 0) efree((CHAR *)db_primearray);
}

/************************* TRANSFORMATION *************************/

#define ZE	0			/* fractional representation of 0 */
#define P1	(1<<30)		/* fractional representation of 1 */
#define N1	(-1<<30)	/* fractional representation of -1 */

/*
 * there are eight different standard transformation matrices that cover
 * all of the Manhattan transformations:
 *
 *   +------+                   +------+
 *   |^     |   +-----------+   |      |   +-----------+
 *   |      |   |           |   |      |   |          >|
 *   |  R0  |   |    R90    |   | R180 |   |   R270    |
 *   |      |   |<          |   |      |   |           |
 *   |      |   +-----------+   |     v|   +-----------+
 *   +------+                   +------+
 *
 *                 +------+                   +------+
 * +-----------+   |     ^|   +-----------+   |      |
 * |<          |   |      |   |           |   |      |
 * |    R0T    |   | R90T |   |   R180T   |   |R270T |
 * |           |   |      |   |          >|   |      |
 * +-----------+   |      |   +-----------+   |v     |
 *                 +------+                   +------+
 *
 *  Form 0 (R0):      0 rotation, no transposition:  1  0
 *                                                   0  1
 *  Form 1 (R90):    90 rotation, no transposition:         0  1
 *                                                         -1  0
 *  Form 2 (R180):  180 rotation, no transposition: -1  0
 *                                                   0 -1
 *  Form 3 (R270):  270 rotation, no transposition:         0 -1
 *                                                          1  0
 *  Form 4 (R0T):     0 rotation, transposition:     0 -1
 *                                                  -1  0
 *  Form 5 (R90T):   90 rotation, transposition:           -1  0
 *                                                          0  1
 *  Form 6 (R180T): 180 rotation, transposition:     0  1
 *                                                   1  0
 *  Form 7 (R270T): 270 rotation, transposition:            1  0
 *                                                          0 -1
 *
 * When element [2][2] of a transformation matrix is not equal to P1, then
 * that matrix is in standard form and the value of element [2][2] selects
 * from the eight forms above.
 */

/* the rotational contents of the eight standard forms */
static INTBIG db_matstyle[8][2][2] =
{
	{{P1, ZE}, {ZE, P1}},
	{{ZE, P1}, {N1, ZE}},
	{{N1, ZE}, {ZE, N1}},
	{{ZE, N1}, {P1, ZE}},
	{{ZE, N1}, {N1, ZE}},
	{{N1, ZE}, {ZE, P1}},
	{{ZE, P1}, {P1, ZE}},
	{{P1, ZE}, {ZE, N1}}
};

/* the transpose of the eight standard forms */
static INTBIG db_mattrans[8] = {0, 3, 2, 1, 4, 5, 6, 7};

/* the determinants of the eight standard forms */
static INTBIG db_determinant[8] = {1, 1, 1, 1, -1, -1, -1, -1};

/* the product of the eight standard forms (filled in by "db_initdatabase") */
static INTBIG db_matmult[8][8];

/*
 * routine to transform the point (x, y) through the transformation matrix in
 * the array "trans" and produce a transformed co-ordinate in (newx, newy)
 */
void xform(INTBIG x, INTBIG y, INTBIG *newx, INTBIG *newy, XARRAY trans)
{
	REGISTER INTBIG px, py, t22;

	px = trans[2][0];   py = trans[2][1];   t22 = trans[2][2];
	if (t22 != P1)
	{
		switch (t22)
		{
			case 0: *newx =  x + px;  *newy =  y + py;  return;
			case 1: *newx = -y + px;  *newy =  x + py;  return;
			case 2: *newx = -x + px;  *newy = -y + py;  return;
			case 3: *newx =  y + px;  *newy = -x + py;  return;
			case 4: *newx = -y + px;  *newy = -x + py;  return;
			case 5: *newx = -x + px;  *newy =  y + py;  return;
			case 6: *newx =  y + px;  *newy =  x + py;  return;
			case 7: *newx =  x + px;  *newy = -y + py;  return;
		}
		(void)db_error(DBBADTMAT|DBXFORM);
		return;
	}
	*newx = mult(trans[0][0],x) + mult(trans[1][0],y) + px;
	*newy = mult(trans[0][1],x) + mult(trans[1][1],y) + py;
}

/*
 * Takes a box specified by lx, hx, ly, hy, transforms it by trans, and
 * returns the box in lx, hx, ly, hy. The relationship between lx, hx,
 * ly, hy is preserved - i.e hx > lx, hy > ly
 */
void xformbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, XARRAY trans)
{
	INTBIG tx, ty;
	REGISTER INTBIG lowx, highx, lowy, highy;

	lowx = *lx;   highx = *hx;
	lowy = *ly;   highy = *hy;
	xform(lowx, lowy, lx,ly, trans);   *hx = *lx;  *hy = *ly;
	xform(lowx, highy, &tx,&ty, trans);
	if (tx < *lx) *lx = tx;   if (tx > *hx) *hx = tx;
	if (ty < *ly) *ly = ty;   if (ty > *hy) *hy = ty;
	xform(highx, highy, &tx,&ty, trans);
	if (tx < *lx) *lx = tx;   if (tx > *hx) *hx = tx;
	if (ty < *ly) *ly = ty;   if (ty > *hy) *hy = ty;
	xform(highx, lowy, &tx,&ty, trans);
	if (tx < *lx) *lx = tx;   if (tx > *hx) *hx = tx;
	if (ty < *ly) *ly = ty;   if (ty > *hy) *hy = ty;
}

/*
 * routine to make a transformation matrix in the array "trans" which
 * rotates the nodeinst "ni" in its offset position.
 * The nodeinst is first translated to (0,0), then rotated to its proper
 * orientation, then translated back to its original position.
 */
void makerot(NODEINST *ni, XARRAY trans)
{
	makeangle(ni->rotation, ni->transpose, trans);

	/*
	 * the code used to be: (but caused roundoff errors)
	 * trans[2][0] = (ni->lowx+ni->highx) / 2;
	 * trans[2][1] = (ni->lowy+ni->highy) / 2;
	 * xform(-trans[2][0], -trans[2][1], &trans[2][0], &trans[2][1], trans);
	 */
	trans[2][0] = ni->lowx + ni->highx;
	trans[2][1] = ni->lowy + ni->highy;
	xform(-trans[2][0], -trans[2][1], &trans[2][0], &trans[2][1], trans);
	trans[2][0] /= 2;    trans[2][1] /= 2;
}

void makerotI(NODEINST *ni, XARRAY trans)
{
	if (ni->transpose != 0)
	{
		makeangle(ni->rotation, ni->transpose, trans);
	} else
	{
		makeangle((3600 - ni->rotation)%3600, 0, trans);
	}

	/*
	 * the code used to be: (but caused roundoff errors)
	 * trans[2][0] = (ni->lowx+ni->highx) / 2;
	 * trans[2][1] = (ni->lowy+ni->highy) / 2;
	 * xform(-trans[2][0], -trans[2][1], &trans[2][0], &trans[2][1], trans);
	 */
	trans[2][0] = ni->lowx + ni->highx;
	trans[2][1] = ni->lowy + ni->highy;
	xform(-trans[2][0], -trans[2][1], &trans[2][0], &trans[2][1], trans);
	trans[2][0] /= 2;    trans[2][1] /= 2;
}

void makeangle(INTBIG rot, INTBIG tra, XARRAY trans)
{
	REGISTER INTBIG i;

	switch (rot)
	{
		case 0:    if (tra) trans[2][2] = 4; else trans[2][2] = 0;   break;
		case 900:  if (tra) trans[2][2] = 5; else trans[2][2] = 1;   break;
		case 1800: if (tra) trans[2][2] = 6; else trans[2][2] = 2;   break;
		case 2700: if (tra) trans[2][2] = 7; else trans[2][2] = 3;   break;
		default:
			/* get rotation matrix from sine and cosine */
			trans[0][0] = trans[1][1] = cosine(rot);
			trans[0][1] = sine(rot);   trans[1][0] = -trans[0][1];

			/* re-organize if the matrix is transposed */
			if (tra)
			{
				i = trans[0][0]; trans[0][0] = -trans[0][1]; trans[0][1] = -i;
				i = trans[1][1]; trans[1][1] = -trans[1][0]; trans[1][0] = -i;
			}
			trans[2][2] = P1;   break;
	}

	/* fill in edge values */
	trans[2][0] = trans[2][1] = ZE;
	trans[0][2] = trans[1][2] = ZE;
}

/*
 * routine to make transformation matrix in "trans" for translating points
 * in nodeinst "ni" to the outside environment.
 */
void maketrans(NODEINST *ni, XARRAY trans)
{
	REGISTER NODEPROTO *np;

	np = ni->proto;
	transid(trans);

	/*
	 * the code used to be: (but caused roundoff errors)
	 * trans[2][0] = (ni->lowx + ni->highx)/2 - (np->lowx + np->highx)/2;
	 * trans[2][1] = (ni->lowy + ni->highy)/2 - (np->lowy + np->highy)/2;
	 */
	trans[2][0] = (ni->lowx + ni->highx - np->lowx - np->highx)/2;
	trans[2][1] = (ni->lowy + ni->highy - np->lowy - np->highy)/2;
}

/*
 * routine to make transformation matrix in "trans" for translating points
 * from the outside environment to the nodeinst "ni".
 */
void maketransI(NODEINST *ni, XARRAY trans)
{
	REGISTER NODEPROTO *np;

	np = ni->proto;
	transid(trans);
	/*
	 * the code used to be: (but caused roundoff errors)
	 * trans[2][0] = (np->lowx + np->highx)/2 - (ni->lowx + ni->highx)/2;
	 * trans[2][1] = (np->lowy + np->highy)/2 - (ni->lowy + ni->highy)/2;
	 */
	trans[2][0] = (np->lowx + np->highx - ni->lowx - ni->highx)/2;
	trans[2][1] = (np->lowy + np->highy - ni->lowy - ni->highy)/2;
}

/* routine to create an identity transformation matrix in the array "matr" */
void transid(XARRAY matr)
{
	/* make standard matrix with edge values zero */
	matr[2][0] = matr[2][1] = matr[0][2] = matr[1][2] = ZE;
	matr[2][2] = 0;
}

/*
 * routine to copy from the transformation matrix in the array "mats"
 * to the transformation matrix in the array "matd"
 */
void transcpy(XARRAY mats, XARRAY matd)
{
	REGISTER INTBIG i, j;

	if (mats[2][2] != P1)
	{
		/* standard matrix: copy edge values and matrix type */
		matd[0][2] = mats[0][2];   matd[1][2] = mats[1][2];
		matd[2][0] = mats[2][0];   matd[2][1] = mats[2][1];
		matd[2][2] = mats[2][2];
		return;
	}

	/* normal matrix: copy everything */
	for(i=0; i<3; i++) for(j=0; j<3; j++) matd[i][j] = mats[i][j];
}

/*
 * routine to multiply the transformation matrix in the array "mata" with
 * the transformation matrix in the array "matb" to produce the transformation
 * matrix in the array "matc"
 */
void transmult(XARRAY mata, XARRAY matb, XARRAY matc)
{
	REGISTER INTBIG sum, ma, mb, i, j, k;

	ma = mata[2][2];
	mb = matb[2][2];
	if (ma != P1 && mb != P1)
	{
		/* standard matrix: compute edges, use tables for matrix type */
		if (ma < 0 || ma > 7 || mb < 0 || mb > 7)
		{
			(void)db_error(DBBADTMAT|DBTRANSMULT);
			return;
		}
		matc[2][2] = db_matmult[ma][mb];

		i = mata[2][0];  mata[2][0] = mata[0][2];
		j = mata[2][1];  mata[2][1] = mata[1][2];
		mata[2][2] = db_mattrans[ma];
		xform(matb[0][2], matb[1][2], &matc[0][2], &matc[1][2], mata);
		mata[2][0] = i;  mata[2][1] = j;  mata[2][2] = ma;
		xform(mata[2][0], mata[2][1], &matc[2][0], &matc[2][1], matb);
		return;
	}

	/* normal matrices: make sure both are in full form */
	if (ma != P1)
	{
		for(i=0; i<2; i++) for(j=0; j<2; j++)
			mata[i][j] = db_matstyle[ma][i][j];
		mata[2][2] = P1;
	}
	if (mb != P1)
	{
		for(i=0; i<2; i++) for(j=0; j<2; j++)
			matb[i][j] = db_matstyle[mb][i][j];
		matb[2][2] = P1;
	}

	/* multiply the matrices */
	for(i=0; i<3; i++) for(j=0; j<3; j++)
	{
		sum = 0;
		for(k=0; k<3; k++) sum += mult(mata[i][k], matb[k][j]);
		matc[i][j] = sum;
	}
	mata[2][2] = ma;   matb[2][2] = mb;
}

/*
 * routine to tell whether the array "trans" is a manhattan orientation.
 * returns true if so.
 */
BOOLEAN ismanhattan(XARRAY trans)
{
	if (trans[2][2] != P1) return(TRUE);
	return(FALSE);
}

/*
 * routine to rotate label style "oldstyle" according to the rotation factor
 * in "trans", returning the new rotation factor
 */
static INTBIG db_textstyle[8] = {TEXTLEFT, TEXTBOTLEFT, TEXTBOT, TEXTBOTRIGHT,
	TEXTRIGHT, TEXTTOPRIGHT, TEXTTOP, TEXTTOPLEFT};
static INTBIG db_texttranspose[8] = {TEXTTOP, TEXTTOPRIGHT, TEXTRIGHT,
	TEXTBOTRIGHT, TEXTBOT, TEXTBOTLEFT, TEXTLEFT, TEXTTOPLEFT};
INTBIG rotatelabel(INTBIG oldstyle, INTBIG rotation, XARRAY trans)
{
	REGISTER INTBIG i;

	/* if text style is not offset, rotation not done */
	if (oldstyle == TEXTCENT || oldstyle == TEXTBOX) return(oldstyle);

	/* cannot handle nonmanhattan rotations */
	if (trans[2][2] == P1) return(oldstyle);

	/* find the label style */
	for(i=0; i<8; i++) if (db_textstyle[i] == oldstyle) break;
	if (i >= 8) return(oldstyle);

	/* handle nontransposed transformation */
	if (trans[2][2] <= 3)
		return(db_textstyle[(i + 2*trans[2][2] + rotation*2) % 8]);

	/* handle transposed transformation */
	return(db_texttranspose[(i + 2*(trans[2][2]-4) + rotation*2) % 8]);
}

/************************* TRANSCENDENTAL MATH *************************/

static INTBIG db_sinetable[] =
{
	0x00000000,0x001C9870,0x003930DA,0x0055C939,0x00726187,0x008EF9BE,
	0x00AB91D9,0x00C829D1,0x00E4C1A1,0x01015944,0x011DF0B3,0x013A87E9,
	0x01571EE0,0x0173B593,0x01904BFC,0x01ACE214,0x01C977D7,0x01E60D3F,
	0x0202A246,0x021F36E6,0x023BCB19,0x02585EDB,0x0274F224,0x029184F0,
	0x02AE1739,0x02CAA8F9,0x02E73A2A,0x0303CAC6,0x03205AC9,0x033CEA2C,
	0x035978E9,0x037606FB,0x0392945D,0x03AF2107,0x03CBACF6,0x03E83822,
	0x0404C287,0x04214C1E,0x043DD4E3,0x045A5CCE,0x0476E3DB,0x04936A04,
	0x04AFEF43,0x04CC7393,0x04E8F6ED,0x0505794D,0x0521FAAB,0x053E7B04,
	0x055AFA50,0x0577788B,0x0593F5AE,0x05B071B4,0x05CCEC98,0x05E96653,
	0x0605DEE0,0x06225639,0x063ECC59,0x065B4139,0x0677B4D5,0x06942726,
	0x06B09827,0x06CD07D2,0x06E97621,0x0705E30F,0x07224E97,0x073EB8B1,
	0x075B215A,0x0777888A,0x0793EE3D,0x07B0526C,0x07CCB513,0x07E9162B,
	0x080575AF,0x0821D399,0x083E2FE3,0x085A8A88,0x0876E382,0x08933ACB,
	0x08AF905E,0x08CBE436,0x08E8364B,0x0904869A,0x0920D51B,0x093D21CB,
	0x09596CA2,0x0975B59B,0x0991FCB0,0x09AE41DD,0x09CA851B,0x09E6C664,
	0x0A0305B4,0x0A1F4304,0x0A3B7E4E,0x0A57B78E,0x0A73EEBD,0x0A9023D5,
	0x0AAC56D2,0x0AC887AE,0x0AE4B662,0x0B00E2EA,0x0B1D0D3F,0x0B39355C,
	0x0B555B3C,0x0B717ED9,0x0B8DA02C,0x0BA9BF32,0x0BC5DBE3,0x0BE1F63A,
	0x0BFE0E33,0x0C1A23C6,0x0C3636EF,0x0C5247A7,0x0C6E55EB,0x0C8A61B2,
	0x0CA66AF9,0x0CC271BA,0x0CDE75EE,0x0CFA7790,0x0D16769C,0x0D32730A,
	0x0D4E6CD6,0x0D6A63FA,0x0D865870,0x0DA24A34,0x0DBE393E,0x0DDA258A,
	0x0DF60F12,0x0E11F5D0,0x0E2DD9C0,0x0E49BADB,0x0E65991B,0x0E81747C,
	0x0E9D4CF8,0x0EB92288,0x0ED4F529,0x0EF0C4D3,0x0F0C9181,0x0F285B2F,
	0x0F4421D6,0x0F5FE570,0x0F7BA5F9,0x0F97636B,0x0FB31DC0,0x0FCED4F2,
	0x0FEA88FD,0x100639DA,0x1021E784,0x103D91F6,0x1059392A,0x1074DD1A,
	0x10907DC2,0x10AC1B1A,0x10C7B51F,0x10E34BCA,0x10FEDF16,0x111A6EFD,
	0x1135FB7B,0x11518488,0x116D0A21,0x11888C3F,0x11A40ADD,0x11BF85F6,
	0x11DAFD83,0x11F67180,0x1211E1E7,0x122D4EB2,0x1248B7DD,0x12641D61,
	0x127F7F39,0x129ADD60,0x12B637D0,0x12D18E83,0x12ECE175,0x130830A0,
	0x13237BFE,0x133EC38A,0x135A073E,0x13754715,0x1390830A,0x13ABBB17,
	0x13C6EF37,0x13E21F64,0x13FD4B99,0x141873D0,0x14339805,0x144EB831,
	0x1469D44F,0x1484EC59,0x14A0004B,0x14BB101F,0x14D61BD0,0x14F12358,
	0x150C26B1,0x152725D7,0x154220C4,0x155D1772,0x157809DC,0x1592F7FE,
	0x15ADE1D0,0x15C8C74F,0x15E3A875,0x15FE853B,0x16195D9E,0x16343197,
	0x164F0122,0x1669CC38,0x168492D5,0x169F54F3,0x16BA128E,0x16D4CB9E,
	0x16EF8020,0x170A300D,0x1724DB61,0x173F8217,0x175A2428,0x1774C190,
	0x178F5A49,0x17A9EE4E,0x17C47D99,0x17DF0826,0x17F98DEF,0x18140EEF,
	0x182E8B20,0x1849027D,0x18637501,0x187DE2A7,0x18984B69,0x18B2AF42,
	0x18CD0E2D,0x18E76824,0x1901BD23,0x191C0D23,0x19365821,0x19509E15,
	0x196ADEFC,0x19851AD1,0x199F518C,0x19B9832B,0x19D3AFA7,0x19EDD6FA,
	0x1A07F921,0x1A221615,0x1A3C2DD2,0x1A564052,0x1A704D90,0x1A8A5587,
	0x1AA45831,0x1ABE558A,0x1AD84D8C,0x1AF24032,0x1B0C2D77,0x1B261556,
	0x1B3FF7C9,0x1B59D4CC,0x1B73AC59,0x1B8D7E6A,0x1BA74AFC,0x1BC11208,
	0x1BDAD38A,0x1BF48F7D,0x1C0E45DB,0x1C27F69F,0x1C41A1C4,0x1C5B4745,
	0x1C74E71C,0x1C8E8146,0x1CA815BC,0x1CC1A479,0x1CDB2D79,0x1CF4B0B6,
	0x1D0E2E2B,0x1D27A5D4,0x1D4117AA,0x1D5A83A9,0x1D73E9CC,0x1D8D4A0E,
	0x1DA6A46A,0x1DBFF8DA,0x1DD9475A,0x1DF28FE4,0x1E0BD274,0x1E250F04,
	0x1E3E4590,0x1E577612,0x1E70A086,0x1E89C4E6,0x1EA2E32D,0x1EBBFB56,
	0x1ED50D5D,0x1EEE193C,0x1F071EEE,0x1F201E6E,0x1F3917B8,0x1F520AC6,
	0x1F6AF794,0x1F83DE1C,0x1F9CBE59,0x1FB59846,0x1FCE6BDF,0x1FE7391F,
	0x20000000,0x2018C07E,0x20317A93,0x204A2E3B,0x2062DB71,0x207B8230,
	0x20942272,0x20ACBC34,0x20C54F70,0x20DDDC21,0x20F66242,0x210EE1CF,
	0x21275AC2,0x213FCD17,0x215838C8,0x21709DD2,0x2188FC2E,0x21A153D9,
	0x21B9A4CD,0x21D1EF05,0x21EA327D,0x22026F30,0x221AA519,0x2232D432,
	0x224AFC78,0x22631DE5,0x227B3875,0x22934C23,0x22AB58EA,0x22C35EC5,
	0x22DB5DAF,0x22F355A4,0x230B469E,0x2323309A,0x233B1392,0x2352EF81,
	0x236AC463,0x23829234,0x239A58ED,0x23B2188B,0x23C9D108,0x23E18261,
	0x23F92C8F,0x2410CF90,0x24286B5D,0x243FFFF2,0x24578D4B,0x246F1362,
	0x24869233,0x249E09BA,0x24B579F1,0x24CCE2D4,0x24E4445F,0x24FB9E8C,
	0x2512F157,0x252A3CBB,0x254180B4,0x2558BD3D,0x256FF251,0x25871FEC,
	0x259E4609,0x25B564A3,0x25CC7BB7,0x25E38B3E,0x25FA9336,0x26119398,
	0x26288C61,0x263F7D8B,0x26566713,0x266D48F4,0x26842329,0x269AF5AD,
	0x26B1C07C,0x26C88392,0x26DF3EEA,0x26F5F27F,0x270C9E4D,0x27234250,
	0x2739DE82,0x275072DF,0x2766FF63,0x277D840A,0x279400CE,0x27AA75AC,
	0x27C0E29F,0x27D747A1,0x27EDA4B0,0x2803F9C6,0x281A46DF,0x28308BF7,
	0x2846C909,0x285CFE10,0x28732B08,0x28894FED,0x289F6CBB,0x28B5816C,
	0x28CB8DFD,0x28E19269,0x28F78EAC,0x290D82C1,0x29236EA4,0x29395251,
	0x294F2DC3,0x296500F6,0x297ACBE5,0x29908E8C,0x29A648E7,0x29BBFAF2,
	0x29D1A4A7,0x29E74604,0x29FCDF02,0x2A126F9F,0x2A27F7D6,0x2A3D77A3,
	0x2A52EF00,0x2A685DEB,0x2A7DC45F,0x2A932256,0x2AA877CE,0x2ABDC4C2,
	0x2AD3092E,0x2AE8450D,0x2AFD785B,0x2B12A314,0x2B27C534,0x2B3CDEB6,
	0x2B51EF96,0x2B66F7D1,0x2B7BF761,0x2B90EE43,0x2BA5DC73,0x2BBAC1EC,
	0x2BCF9EAA,0x2BE472A9,0x2BF93DE5,0x2C0E005A,0x2C22BA03,0x2C376ADC,
	0x2C4C12E2,0x2C60B210,0x2C754862,0x2C89D5D4,0x2C9E5A61,0x2CB2D606,
	0x2CC748BF,0x2CDBB288,0x2CF0135C,0x2D046B37,0x2D18BA16,0x2D2CFFF4,
	0x2D413CCD,0x2D55709D,0x2D699B61,0x2D7DBD14,0x2D91D5B1,0x2DA5E536,
	0x2DB9EB9E,0x2DCDE8E5,0x2DE1DD07,0x2DF5C801,0x2E09A9CD,0x2E1D8269,
	0x2E3151D0,0x2E4517FE,0x2E58D4EF,0x2E6C88A0,0x2E80330C,0x2E93D430,
	0x2EA76C08,0x2EBAFA8F,0x2ECE7FC2,0x2EE1FB9C,0x2EF56E1B,0x2F08D73A,
	0x2F1C36F5,0x2F2F8D48,0x2F42DA30,0x2F561DA9,0x2F6957AE,0x2F7C883D,
	0x2F8FAF50,0x2FA2CCE5,0x2FB5E0F8,0x2FC8EB84,0x2FDBEC86,0x2FEEE3FA,
	0x3001D1DC,0x3014B629,0x302790DD,0x303A61F3,0x304D2969,0x305FE73B,
	0x30729B64,0x308545E1,0x3097E6AE,0x30AA7DC8,0x30BD0B2B,0x30CF8ED3,
	0x30E208BD,0x30F478E4,0x3106DF46,0x31193BDD,0x312B8EA8,0x313DD7A2,
	0x315016C7,0x31624C14,0x31747785,0x31869916,0x3198B0C5,0x31AABE8D,
	0x31BCC26A,0x31CEBC5A,0x31E0AC58,0x31F29261,0x32046E72,0x32164086,
	0x3228089A,0x3239C6AC,0x324B7AB6,0x325D24B6,0x326EC4A8,0x32805A89,
	0x3291E654,0x32A36807,0x32B4DF9F,0x32C64D17,0x32D7B06C,0x32E9099A,
	0x32FA589F,0x330B9D77,0x331CD81D,0x332E0890,0x333F2ECB,0x33504ACB,
	0x33615C8C,0x3372640C,0x33836146,0x33945438,0x33A53CDD,0x33B61B34,
	0x33C6EF37,0x33D7B8E5,0x33E87838,0x33F92D2F,0x3409D7C6,0x341A77FA,
	0x342B0DC6,0x343B9929,0x344C1A1E,0x345C90A2,0x346CFCB2,0x347D5E4B,
	0x348DB56A,0x349E020A,0x34AE442A,0x34BE7BC5,0x34CEA8D9,0x34DECB62,
	0x34EEE35C,0x34FEF0C6,0x350EF39B,0x351EEBD9,0x352ED97C,0x353EBC81,
	0x354E94E4,0x355E62A4,0x356E25BB,0x357DDE29,0x358D8BE8,0x359D2EF7,
	0x35ACC751,0x35BC54F5,0x35CBD7DE,0x35DB500A,0x35EABD75,0x35FA201D,
	0x360977FF,0x3618C517,0x36280762,0x36373EDD,0x36466B86,0x36558D58,
	0x3664A452,0x3673B070,0x3682B1B0,0x3691A80D,0x36A09386,0x36AF7417,
	0x36BE49BD,0x36CD1475,0x36DBD43D,0x36EA8911,0x36F932EE,0x3707D1D2,
	0x371665BA,0x3724EEA2,0x37336C88,0x3741DF69,0x37504742,0x375EA410,
	0x376CF5D1,0x377B3C80,0x3789781D,0x3797A8A3,0x37A5CE10,0x37B3E860,
	0x37C1F793,0x37CFFBA3,0x37DDF48F,0x37EBE255,0x37F9C4F0,0x38079C5E,
	0x3815689D,0x382329AA,0x3830DF81,0x383E8A21,0x384C2987,0x3859BDB0,
	0x38674698,0x3874C43E,0x3882369F,0x388F9DB8,0x389CF986,0x38AA4A07,
	0x38B78F38,0x38C4C916,0x38D1F79F,0x38DF1AD0,0x38EC32A7,0x38F93F21,
	0x3906403A,0x391335F2,0x39202045,0x392CFF30,0x3939D2B1,0x39469AC6,
	0x3953576B,0x3960089F,0x396CAE5E,0x397948A7,0x3985D777,0x39925ACA,
	0x399ED2A0,0x39AB3EF4,0x39B79FC6,0x39C3F512,0x39D03ED5,0x39DC7D0E,
	0x39E8AFBA,0x39F4D6D6,0x3A00F260,0x3A0D0256,0x3A1906B6,0x3A24FF7C,
	0x3A30ECA6,0x3A3CCE33,0x3A48A41F,0x3A546E69,0x3A602D0D,0x3A6BE00A,
	0x3A77875E,0x3A832305,0x3A8EB2FF,0x3A9A3747,0x3AA5AFDD,0x3AB11CBD,
	0x3ABC7DE6,0x3AC7D355,0x3AD31D08,0x3ADE5AFC,0x3AE98D30,0x3AF4B3A2,
	0x3AFFCE4E,0x3B0ADD33,0x3B15E04E,0x3B20D79E,0x3B2BC320,0x3B36A2D3,
	0x3B4176B3,0x3B4C3EBE,0x3B56FAF3,0x3B61AB50,0x3B6C4FD1,0x3B76E876,
	0x3B81753C,0x3B8BF621,0x3B966B22,0x3BA0D43E,0x3BAB3173,0x3BB582BE,
	0x3BBFC81E,0x3BCA0190,0x3BD42F13,0x3BDE50A4,0x3BE86641,0x3BF26FE9,
	0x3BFC6D99,0x3C065F4F,0x3C10450A,0x3C1A1EC7,0x3C23EC85,0x3C2DAE41,
	0x3C3763F9,0x3C410DAC,0x3C4AAB58,0x3C543CFA,0x3C5DC291,0x3C673C1B,
	0x3C70A996,0x3C7A0B01,0x3C836058,0x3C8CA99B,0x3C95E6C7,0x3C9F17DB,
	0x3CA83CD5,0x3CB155B3,0x3CBA6273,0x3CC36314,0x3CCC5793,0x3CD53FEF,
	0x3CDE1C27,0x3CE6EC37,0x3CEFB01F,0x3CF867DD,0x3D01136E,0x3D09B2D2,
	0x3D124607,0x3D1ACD0A,0x3D2347DB,0x3D2BB677,0x3D3418DD,0x3D3C6F0B,
	0x3D44B8FF,0x3D4CF6B9,0x3D552835,0x3D5D4D73,0x3D656671,0x3D6D732D,
	0x3D7573A6,0x3D7D67D9,0x3D854FC7,0x3D8D2B6C,0x3D94FAC7,0x3D9CBDD8,
	0x3DA4749B,0x3DAC1F10,0x3DB3BD36,0x3DBB4F0A,0x3DC2D48B,0x3DCA4DB8,
	0x3DD1BA8F,0x3DD91B0E,0x3DE06F35,0x3DE7B701,0x3DEEF272,0x3DF62185,
	0x3DFD443A,0x3E045A8F,0x3E0B6482,0x3E126213,0x3E19533F,0x3E203806,
	0x3E271065,0x3E2DDC5C,0x3E349BEA,0x3E3B4F0C,0x3E41F5C2,0x3E48900A,
	0x3E4F1DE3,0x3E559F4C,0x3E5C1443,0x3E627CC7,0x3E68D8D6,0x3E6F2871,
	0x3E756B94,0x3E7BA23F,0x3E81CC72,0x3E87EA29,0x3E8DFB65,0x3E940024,
	0x3E99F865,0x3E9FE426,0x3EA5C367,0x3EAB9627,0x3EB15C63,0x3EB7161C,
	0x3EBCC34F,0x3EC263FC,0x3EC7F822,0x3ECD7FBF,0x3ED2FAD2,0x3ED8695B,
	0x3EDDCB58,0x3EE320C8,0x3EE869AB,0x3EEDA5FE,0x3EF2D5C1,0x3EF7F8F3,
	0x3EFD0F93,0x3F0219A0,0x3F071719,0x3F0C07FD,0x3F10EC4A,0x3F15C401,
	0x3F1A8F1F,0x3F1F4DA5,0x3F23FF90,0x3F28A4E1,0x3F2D3D96,0x3F31C9AE,
	0x3F364928,0x3F3ABC04,0x3F3F2241,0x3F437BDD,0x3F47C8D8,0x3F4C0931,
	0x3F503CE7,0x3F5463FA,0x3F587E68,0x3F5C8C30,0x3F608D52,0x3F6481CE,
	0x3F6869A1,0x3F6C44CC,0x3F70134E,0x3F73D526,0x3F778A53,0x3F7B32D4,
	0x3F7ECEA9,0x3F825DD1,0x3F85E04B,0x3F895617,0x3F8CBF34,0x3F901BA1,
	0x3F936B5D,0x3F96AE69,0x3F99E4C2,0x3F9D0E69,0x3FA02B5D,0x3FA33B9E,
	0x3FA63F2A,0x3FA93601,0x3FAC2023,0x3FAEFD8F,0x3FB1CE44,0x3FB49242,
	0x3FB74988,0x3FB9F416,0x3FBC91EB,0x3FBF2306,0x3FC1A768,0x3FC41F10,
	0x3FC689FC,0x3FC8E82E,0x3FCB39A3,0x3FCD7E5C,0x3FCFB659,0x3FD1E198,
	0x3FD4001A,0x3FD611DE,0x3FD816E3,0x3FDA0F29,0x3FDBFAB1,0x3FDDD978,
	0x3FDFAB80,0x3FE170C7,0x3FE3294E,0x3FE4D513,0x3FE67417,0x3FE8065A,
	0x3FE98BDA,0x3FEB0498,0x3FEC7094,0x3FEDCFCC,0x3FEF2242,0x3FF067F4,
	0x3FF1A0E2,0x3FF2CD0C,0x3FF3EC73,0x3FF4FF14,0x3FF604F1,0x3FF6FE0A,
	0x3FF7EA5D,0x3FF8C9EB,0x3FF99CB4,0x3FFA62B7,0x3FFB1BF5,0x3FFBC86D,
	0x3FFC681F,0x3FFCFB0A,0x3FFD8130,0x3FFDFA8F,0x3FFE6728,0x3FFEC6FA,
	0x3FFF1A06,0x3FFF604B,0x3FFF99CA,0x3FFFC681,0x3FFFE672,0x3FFFF99D,
	0x40000000};

/*
 * routine to return a fixed-point sine value for "n" (in tenth-degrees from 0 to
 * 3599)
 */
INTBIG sine(INTBIG n)
{
	if (n <= 900) return(db_sinetable[n]);
	if (n <= 1800) return(db_sinetable[1800-n]);
	if (n <= 2700) return(-db_sinetable[n-1800]);
	return(-db_sinetable[3600-n]);
}

/*
 * routine to return a fixed-point cosine value for "n" (in tenth-degrees from 0 to
 * 3599)
 */
INTBIG cosine(INTBIG n)
{
	if (n <= 900) return(db_sinetable[900-n]);
	if (n <= 1800) return(-db_sinetable[n-900]);
	if (n <= 2700) return(-db_sinetable[2700-n]);
	return(db_sinetable[n-2700]);
}

/*
 * routine to return the angle from point (x1,y1) to point (x2,y2) in tenth-degrees
 * where right-pointing is 0 and positive angles are counter-clockwise.
 */
INTBIG figureangle(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	double dx, dy, ang;
	INTBIG iang;

	dx = x2 - x1;
	dy = y2 - y1;
	if (dx == 0.0 && dy == 0.0)
	{
		ttyputerr(_("Warning: domain violation while figuring angle"));
		return(0);
	}
	ang = atan2(dy, dx);
	if (ang < 0.0) ang += EPI*2.0;
	iang = rounddouble(ang * 1800.0 / EPI);
	return(iang);
}

/*
 * routine to return the angle from point (x1,y1) to point (x2,y2) in tenth-degrees
 * where right-pointing is 0 and positive angles are counter-clockwise.
 */
double ffigureangle(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2)
{
	double dx, dy, ang;

	dx = x2 - x1;
	dy = y2 - y1;
	if (dx == 0.0 && dy == 0.0)
	{
		ttyputerr(_("Warning: domain violation while figuring angle"));
		return(0);
	}
	ang = atan2(dy, dx);
	if (ang < 0.0) ang += EPI*2.0;
	return(ang);
}

/************************* PRIME NUMBERS *************************/

/* This table contains the first few hundred prime numbers */
static INTBIG db_small_primes[] =
{
	2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59,
	61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127,
	131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191,
	193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257,
	263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331,
	337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401,
	409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467,
	479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563,
	569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631,
	641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709,
	719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797,
	809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877,
	881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967,
	971, 977, 983, 991, 997, 1009, 1013, 1019, 1021, 1031, 1033,
	1039, 1049, 1051, 1061, 1063, 1069, 1087, 1091, 1093, 1097,
	1103, 1109, 1117, 1123, 1129, 1151, 1153, 1163, 1171, 1181,
	1187, 1193, 1201, 1213, 1217, 1223, 1229, 1231, 1237, 1249,
	1259, 1277, 1279, 1283, 1289, 1291, 1297, 1301, 1303, 1307,
	1319, 1321, 1327, 1361, 1367, 1373, 1381, 1399, 1409, 1423,
	1427, 1429, 1433, 1439, 1447, 1451, 1453, 1459, 1471, 1481,
	1483, 1487, 1489, 1493, 1499, 1511, 1523, 1531, 1543, 1549,
	1553, 1559, 1567, 1571, 1579, 1583, 1597, 1601, 1607, 1609,
	1613, 1619, 1621, 1627, 1637, 1657, 1663, 1667, 1669, 1693,
	1697, 1699, 1709, 1721, 1723, 1733, 1741, 1747, 1753, 1759,
	1777, 1783, 1787, 1789, 1801, 1811, 1823, 1831, 1847, 1861,
	1867, 1871, 1873, 1877, 1879, 1889, 1901, 1907, 1913, 1931,
	1933, 1949, 1951, 1973, 1979, 1987, 1993, 1997, 1999, 2003,
	2011, 2017, 2027, 2029, 2039, 2053, 2063, 2069, 2081, 2083,
	2087, 2089, 2099, 2111, 2113, 2129, 2131, 2137, 2141, 2143,
	2153, 2161, 2179, 2203, 2207, 2213, 2221, 2237, 2239, 2243,
	2251, 2267, 2269, 2273, 2281, 2287, 2293, 2297, 2309, 2311,
	2333, 2339, 2341, 2347, 2351, 2357, 2371, 2377, 2381, 2383,
	2389, 2393, 2399, 2411, 2417, 2423, 2437, 2441, 2447, 2459,
	2467, 2473, 2477, 2503, 2521, 2531, 2539, 2543, 2549, 2551,
	2557, 2579, 2591, 2593, 2609, 2617, 2621, 2633, 2647, 2657,
	2659, 2663, 2671, 2677, 2683, 2687, 2689, 2693, 2699, 2707,
	2711, 2713, 2719, 2729, 2731, 2741, 2749, 2753, 2767, 2777,
	2789, 2791, 2797, 2801, 2803, 2819, 2833, 2837, 2843, 2851,
	2857, 2861, 2879, 2887, 2897, 2903, 2909, 2917, 2927, 2939,
	2953, 2957, 2963, 2969, 2971, 2999, 3001, 3011, 3019, 3023,
	3037, 3041, 3049, 3061, 3067, 3079, 3083, 3089, 3109, 3119,
	3121, 3137, 3163, 3167, 3169, 3181, 3187, 3191, 3203, 3209,
	3217, 3221, 3229, 3251, 3253, 3257, 3259, 3271, 3299, 3301,
	3307, 3313, 3319, 3323, 3329, 3331, 3343, 3347, 3359, 3361,
	3371, 3373, 3389, 3391, 3407, 3413, 3433, 3449, 3457, 3461,
	3463, 3467, 3469, 3491, 3499, 3511, 3517, 3527, 3529, 3533,
	3539, 3541, 3547, 3557, 3559, 3571, 3581, 3583, 3593, 3607,
	3613, 3617, 3623, 3631, 3637, 3643, 3659, 3671, 3673, 3677,
	3691, 3697, 3701, 3709, 3719, 3727, 3733, 3739, 3761, 3767,
	3769, 3779, 3793, 3797, 3803, 3821, 3823, 3833, 3847, 3851,
	3853, 3863, 3877, 3881, 3889, 3907, 3911, 3917, 3919, 3923,
	3929, 3931, 3943, 3947, 3967, 3989, 4001, 4003, 4007, 4013,
	4019, 4021, 4027, 4049, 4051, 4057, 4073, 4079, 4091, 4093,
	4099, 4111, 4127, 4129, 4133, 4139, 4153, 4157, 4159, 4177,
	4201, 4211, 4217, 4219, 4229, 4231, 4241, 4243, 4253, 4259,
	4261, 4271, 4273, 4283, 4289, 4297, 4327, 4337, 4339, 4349,
	4357, 4363, 4373, 4391, 4397, 4409, 4421, 4423, 4441, 4447,
	4451, 4457, 4463
};
#define NUMSMALLPRIMES (sizeof(db_small_primes) / SIZEOFINTBIG)

/*
 * Routine to initialize the prime number tables.  Returns true on error.
 */
BOOLEAN db_primeinit(void)
{
	REGISTER INTBIG i;

	if (db_primearraytotal != 0) return(FALSE);

	if (db_multiprocessing) emutexlock(db_primenumbermutex);
	if (db_primearraytotal == 0)
	{
		db_primearray = (INTBIG *)emalloc(NUMSMALLPRIMES * SIZEOFINTBIG, db_cluster);
		if (db_primearray == 0)
		{
			if (db_multiprocessing) emutexunlock(db_primenumbermutex);
			return(TRUE);
		}
		for(i=0; i<NUMSMALLPRIMES; i++)
			db_primearray[i] = db_small_primes[i];
		db_primearraytotal = NUMSMALLPRIMES;
		db_maxcheckableprime = db_primearray[NUMSMALLPRIMES-1] *
			db_primearray[NUMSMALLPRIMES-1];
	}
	if (db_multiprocessing) emutexunlock(db_primenumbermutex);
	return(FALSE);
}

/*
 * Routine to extend the tables of prime numbers so that prime number "length" is
 * present.  Returns true on error.
 */
BOOLEAN db_extendprimetable(INTBIG length)
{
	REGISTER INTBIG newtotal, i, *newprimearray;

	if (db_primeinit()) return(TRUE);
	if (length >= db_primearraytotal)
	{
		if (db_multiprocessing) emutexlock(db_primenumbermutex);
		newtotal = db_primearraytotal * 2;
		if (length >= newtotal) newtotal = length + 100;
		newprimearray = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, db_cluster);
		if (newprimearray == 0)
		{
			if (db_multiprocessing) emutexunlock(db_primenumbermutex);
			return(TRUE);
		}
		for(i=0; i<db_primearraytotal; i++)
			newprimearray[i] = db_primearray[i];

		/* fill the extra elements */
		for(i=db_primearraytotal; i<newtotal; i++)
			newprimearray[i] = pickprime(newprimearray[i-1] + 2);

		efree((CHAR *)db_primearray);
		db_primearray = newprimearray;
		db_primearraytotal = newtotal;
		db_maxcheckableprime = db_primearray[newtotal-1] *
			db_primearray[newtotal-1];
		if (db_maxcheckableprime <= 0) db_maxcheckableprime = MAXINTBIG;
		if (db_multiprocessing) emutexunlock(db_primenumbermutex);
	}
	return(FALSE);	
}

/*
 * A fast and simple way to check if a number is prime - it
 * shouldn't be divisible by any prime less than it's square
 * root.
 */
BOOLEAN db_isprime(INTBIG n)
{
	REGISTER INTBIG i, m;

	if ((n % 2) == 0) return(FALSE);
	if (db_primeinit()) return(FALSE);

	/* if we can't check it, assume it is not prime */
	if (n >= db_maxcheckableprime) return(FALSE);

	for(i = 0; i < db_primearraytotal; i++)
	{
		m = db_primearray[i];
		if (m * m >= n) break;
		if (n % m == 0) return(FALSE);
	}
	return(TRUE);
}

/*
 * Routine to return the "n"th prime number.
 * Prime #0 is 2, #1 is 3, #2 is 5...
 */
INTBIG getprime(INTBIG n)
{
	if (n >= db_primearraytotal)
	{
		if (db_extendprimetable(n)) return(0);
	}
	return(db_primearray[n]);
}

/*
 * Returns the first prime over N, by checking all odd numbers
 * over N till it finds one.
 */
INTBIG pickprime(INTBIG n)
{
	if (db_primeinit()) return(0);
	if (n >= db_maxcheckableprime) return(0);
	if (n % 2 == 0) n++;
	while (!db_isprime(n))
	{
		n += 2;
	}
	return(n);
}

/************************* MISCELLANEOUS MATHEMATICS *************************/

/*
 * integer square-root.
 */
INTBIG intsqrt(INTBIG val)
{
	REGISTER INTBIG guess, i;

	/* can only take square root of a positive number */
	if (val <= 0) return(val);

	/* compute initial guess of square root */
	guess = 0;
	i = val;
	while (i != 0)
	{
		guess = (guess << 1) | 1;
		i = i >> 2;
	}

	/* six iterations of Newton's approximation to square root */
	for(i=0; i<6; i++) guess = (guess + val / guess) >> 1;

	return(guess);
}

/*
 * routine to compute the euclidean distance between point (x1,y1) and
 * point (x2,y2).  This may require determining the hypotenuse of a triangle
 * by finding square roots, in which case an integer square-root subroutine
 * that uses Newton's method is called.
 */
INTBIG computedistance(INTBIG x1,INTBIG y1, INTBIG x2,INTBIG y2)
{
	REGISTER INTBIG dx, dy, res;
	double fdx, fdy;

	dx = abs(x1 - x2);
	dy = abs(y1 - y2);
	if (dx == 0 || dy == 0) return(dx + dy);

	/* nonmanhattan distance: need to do square root */
	if (dx <= 32767 && dy <= 32767) return(intsqrt(dx*dx + dy*dy));

	/* do it in floating point */
	fdx = dx;   fdy = dy;
	res = rounddouble(sqrt(fdx*fdx + fdy*fdy));
	return(res);
}

/*
 * routine to return true if the point (x,y) is on the line segment from
 * (x1,y1) to (x2,y2)
 */
BOOLEAN isonline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG x, INTBIG y)
{
	/* trivial rejection if point not in the bounding box of the line */
	if (x < mini(x1,x2)) return(FALSE);
	if (x > maxi(x1,x2)) return(FALSE);
	if (y < mini(y1,y2)) return(FALSE);
	if (y > maxi(y1,y2)) return(FALSE);

	/* handle manhattan cases specially */
	if (x1 == x2)
	{
		if (x == x1) return(TRUE);
		return(FALSE);
	}
	if (y1 == y2)
	{
		if (y == y1) return(TRUE);
		return(FALSE);
	}

	/* handle nonmanhattan */
	if (muldiv(x-x1, y2-y1, x2-x1) == y-y1) return(TRUE);
	return(FALSE);
}

/*
 * routine to determine the intersection of two lines and return that point
 * in (x,y).  The first line is at "ang1" tenth-degrees and runs through (x1,y1)
 * and the second line is at "ang2" tenth-degrees and runs through (x2,y2).  The
 * routine returns a negative value if the lines do not intersect.
 */
INTBIG intersect(INTBIG x1, INTBIG y1, INTBIG ang1, INTBIG x2, INTBIG y2,
	INTBIG ang2, INTBIG *x, INTBIG *y)
{
	double fa1, fb1, fc1, fa2, fb2, fc2, fang1, fang2, fswap, fy;

	/* cannot handle lines if they are at the same angle */
	if (ang1 == ang2) return(-1);

	/* also at the same angle if off by 180 degrees */
	if (mini(ang1, ang2) + 1800 == maxi(ang1, ang2)) return(-1);

	fang1 = ang1 / 1800.0 * EPI;
	fang2 = ang2 / 1800.0 * EPI;
	fa1 = sin(fang1);   fb1 = -cos(fang1);
	fc1 = -fa1 * ((double)x1) - fb1 * ((double)y1);
	fa2 = sin(fang2);   fb2 = -cos(fang2);
	fc2 = -fa2 * ((double)x2) - fb2 * ((double)y2);
	if (fabs(fa1) < fabs(fa2))
	{
		fswap = fa1;   fa1 = fa2;   fa2 = fswap;
		fswap = fb1;   fb1 = fb2;   fb2 = fswap;
		fswap = fc1;   fc1 = fc2;   fc2 = fswap;
	}
	fy = (fa2 * fc1 / fa1 - fc2) / (fb2 - fa2*fb1/fa1);
	*y = rounddouble(fy);
	*x = rounddouble((-fb1 * fy - fc1) / fa1);
	return(0);
}

/*
 * routine to determine the intersection of two lines and return that point
 * in (x,y).  The first line is at "ang1" radians and runs through (x1,y1)
 * and the second line is at "ang2" radians and runs through (x2,y2).  The
 * routine returns a negative value if the lines do not intersect.
 */
INTBIG fintersect(INTBIG x1, INTBIG y1, double fang1, INTBIG x2, INTBIG y2,
	double fang2, INTBIG *x, INTBIG *y)
{
	double fa1, fb1, fc1, fa2, fb2, fc2, fswap, fy, fmin, fmax;

	/* cannot handle lines if they are at the same angle */
	if (doublesequal(fang1, fang2)) return(-1);

	/* also at the same angle if off by 180 degrees */
	if (fang1 < fang2) { fmin = fang1; fmax = fang2; } else
	{ fmin = fang2; fmax = fang1; }
	if (doublesequal(fmin + EPI, fmax)) return(-1);

	fa1 = sin(fang1);   fb1 = -cos(fang1);
	fc1 = -fa1 * ((double)x1) - fb1 * ((double)y1);
	fa2 = sin(fang2);   fb2 = -cos(fang2);
	fc2 = -fa2 * ((double)x2) - fb2 * ((double)y2);
	if (fabs(fa1) < fabs(fa2))
	{
		fswap = fa1;   fa1 = fa2;   fa2 = fswap;
		fswap = fb1;   fb1 = fb2;   fb2 = fswap;
		fswap = fc1;   fc1 = fc2;   fc2 = fswap;
	}
	fy = (fa2 * fc1 / fa1 - fc2) / (fb2 - fa2*fb1/fa1);
	*y = rounddouble(fy);
	*x = rounddouble((-fb1 * fy - fc1) / fa1);
	return(0);
}

/*
 * Routine to determine whether the line segment from (fx1,fy1) to (tx1,ty1)
 * intersects the line segment from (fx2,fy2) to (tx2,ty2).  Returns true if
 * they do and sets the intersection point to (ix,iy).
 */
BOOLEAN segintersect(INTBIG fx1, INTBIG fy1, INTBIG tx1, INTBIG ty1,
	INTBIG fx2, INTBIG fy2, INTBIG tx2, INTBIG ty2, INTBIG *ix, INTBIG *iy)
{
	REGISTER INTBIG ang1, ang2;

	ang1 = figureangle(fx1, fy1, tx1, ty1);
	ang2 = figureangle(fx2, fy2, tx2, ty2);
	if (intersect(fx1, fy1, ang1, fx2, fy2, ang2, ix, iy) >= 0)
	{
		if (*ix < mini(fx1, tx1) || *ix > maxi(fx1, tx1) ||
			*iy < mini(fy1, ty1) || *iy > maxi(fy1, ty1)) return(FALSE);
		if (*ix < mini(fx2, tx2) || *ix > maxi(fx2, tx2) ||
			*iy < mini(fy2, ty2) || *iy > maxi(fy2, ty2)) return(FALSE);
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to find the intersection points between the circle at (icx,icy) with point (isx,isy)
 * and the line from (lx1,ly1) to (lx2,ly2).  Returns the two points in (ix1,iy1) and (ix2,iy2).
 * Allows intersection tolerance of "tolerance".
 * Returns the number of intersection points (0 if none, 1 if tangent, 2 if intersecting).
 */
INTBIG circlelineintersection(INTBIG icx, INTBIG icy, INTBIG isx, INTBIG isy,
	INTBIG lx1, INTBIG ly1, INTBIG lx2, INTBIG ly2, INTBIG *ix1, INTBIG *iy1,
	INTBIG *ix2, INTBIG *iy2, INTBIG tolerance)
{
	double fx, fy, radius, adjacent, angle, intangle, a1, a2, segx, segy, m, b, mi, bi;

	/*
	 * construct a line that is perpendicular to the intersection line and passes
	 * through the circle center.  It meets the intersection line at (segx, segy)
	 */
	if (ly1 == ly2)
	{
		segx = icx;
		segy = ly1;
	} else if (lx1 == lx2)
	{
		segx = lx1;
		segy = icy;
	} else
	{
		/* compute equation of the line */
		fx = lx1 - lx2;   fy = ly1 - ly2;
		m = fy / fx;
		b = -lx1;   b *= m;   b += ly1;

		/* compute perpendicular to line through the point */
		mi = -1.0/m;
		bi = -icx;  bi *= mi;  bi += icy;

		/* compute intersection of the lines */
		segx = (bi-b) / (m-mi);
		segy = m * segx + b;
	}

	/* special case when line passes through the circle center */
	if (segx == icx && segy == icy)
	{
		fx = isx - icx;   fy = isy - icy;
		radius = sqrt(fx*fx + fy*fy);

		fx = lx2 - lx1;   fy = ly2 - ly1;
		if (fx == 0.0 && fy == 0.0)
		{
			ttyputerr(_("Domain error doing circle/line intersection"));
			return(0);
		}
		angle = atan2(fy, fx);

		*ix1 = icx + rounddouble(cos(angle) * radius);
		*iy1 = icy + rounddouble(sin(angle) * radius);

		*ix2 = icx + rounddouble(cos(-angle) * radius);
		*iy2 = icy + rounddouble(sin(-angle) * radius);
	} else
	{
		/*
		 * construct a right triangle with the three points: (icx, icy), (segx, segy), and (ix1,iy1)
		 * the right angle is at the point (segx, segy) and the hypotenuse is the circle radius
		 * The unknown point is (ix1, iy1), the intersection of the line and the circle.
		 * To find it, determine the angle at the point (icx, icy)
		 */
		fx = isx - icx;   fy = isy - icy;
		radius = sqrt(fx*fx + fy*fy);

		fx = segx - icx;   fy = segy - icy;
		adjacent = sqrt(fx*fx + fy*fy);

		/* if they are within tolerance, accept */
		if (fabs(adjacent - radius) < tolerance)
		{
			*ix1 = rounddouble(segx);
			*iy1 = rounddouble(segy);
			return(1);
		}

		/* if the point is outside of the circle, quit */
		if (adjacent > radius) return(0);

		/* for zero radius, use circle center */
		if (radius == 0.0)
		{
			*ix1 = *ix2 = icx;
			*iy1 = *iy2 = icy;
		} else
		{
			/*
			 * now determine the angle from the center to the point (segx, segy) and offset that angle
			 * by "angle".  Then project it by "radius" to get the two intersection points
			 */
			angle = acos(adjacent / radius);
			fx = segx - icx;   fy = segy - icy;
			if (fx == 0.0 && fy == 0.0)
			{
				ttyputerr(_("Domain error doing line/circle intersection"));
				return(0);
			}
			intangle = atan2(fy, fx);
			a1 = intangle - angle;   a2 = intangle + angle;
			*ix1 = icx + rounddouble(cos(a1) * radius);
			*iy1 = icy + rounddouble(sin(a1) * radius);

			*ix2 = icx + rounddouble(cos(a2) * radius);
			*iy2 = icy + rounddouble(sin(a2) * radius);
		}
	}

	if (*ix1 == *ix2 && *iy1 == *iy2) return(1);
	return(2);
}

/*
 * routine to find the two tangent points on a circle that connect to the point
 * (x,y).  The circle is at (cx,cy) with a point at (sx,sy).  The points are returned in
 * (ix1,iy1) and (ix2, iy2).  Returns true if tangent points cannot be found
 * (if the point (x,y) is inside of the circle).
 */
BOOLEAN circletangents(INTBIG x, INTBIG y, INTBIG cx, INTBIG cy, INTBIG sx, INTBIG sy,
	INTBIG *ix1, INTBIG *iy1, INTBIG *ix2, INTBIG *iy2)
{
	double in, offset, dang, ang1, ang2, dx, dy, hypotenuse, radius;

	/* determine circle radius, make sure tangents can be determined */
	dx = sx - cx;   dy = sy - cy;
	radius = sqrt(dx*dx + dy*dy);
	dx = x - cx;   dy = y - cy;
	hypotenuse = sqrt(dx*dx + dy*dy);
	if (hypotenuse + 1.0 <= radius) return(TRUE);
	if (hypotenuse < radius) radius = hypotenuse;

	/*
	 * construct a right triangle, where:
	 *   the line from the circle center to the point (x,y) is the hypotenuse
	 *   the line from the circle center to an intersection point is side of length "r"
	 * then the angle between these two lines is arccosine(r/hypotenuse)
	 */
	in = radius / hypotenuse;
	offset = acos(in);

	dx = (double)x-cx;   dy = (double)y-cy;
	if (dx == 0.0 && dy == 0.0)
	{
		ttyputerr(_("Domain error doing circle tangents"));
		return(TRUE);
	}
	dang = atan2(dy, dx);
	ang1 = dang - offset;
	if (ang1 < 0.0) ang1 += EPI*2.0;
	ang2 = dang + offset;
	if (ang2 < 0.0) ang2 += EPI*2.0;
	if (ang2 >= EPI*2.0) ang2 -= EPI*2.0;
	*ix1 = cx + rounddouble(cos(ang1)*radius);
	*iy1 = cy + rounddouble(sin(ang1)*radius);
	*ix2 = cx + rounddouble(cos(ang2)*radius);
	*iy2 = cy + rounddouble(sin(ang2)*radius);
	return(FALSE);
}

/*
 * routine to convert an arc to line segments.  The arc goes counterclockwise from
 * (sx,sy) to (ex,ey), centered about the point (cx,cy).  The polygon "poly" is loaded
 * with the line-approximation.  There is an arc segment every "arcres" tenth-degrees and
 * no segment sags from its true curvature by more than "arcsag".
 */
void arctopoly(INTBIG cx, INTBIG cy, INTBIG sx, INTBIG sy, INTBIG ex, INTBIG ey, POLYGON *poly,
	INTBIG arcres, INTBIG arcsag)
{
	REGISTER INTBIG as, ae, ang, degreespersegment, segmentcount, pointcount,
		curangle, pointnum, angleincrement;
	REGISTER INTBIG radius;
	double fradius, sag;

	radius = computedistance(cx, cy, sx, sy);
	as = figureangle(cx, cy, sx, sy);
	ae = figureangle(cx, cy, ex, ey);
	if (ae > as) ang = ae - as; else
		if (as > ae) ang = ae + 3600 - as; else
			ang = 0;

	/* determine number of degrees per segment */
	fradius = radius;
	sag = arcsag;
	if (fradius - sag <= 0.0) degreespersegment = ang; else
		degreespersegment = rounddouble(2.0 * acos((fradius - sag) / fradius) * 1800.0 / EPI);
	if (degreespersegment > arcres) degreespersegment = arcres;

	if (degreespersegment <= 0) degreespersegment = 1;
	segmentcount = ang / degreespersegment;
	if (segmentcount < 1) segmentcount = 1;
	angleincrement = ang / segmentcount;
	pointcount = segmentcount + 1;
	curangle = as;

	if (poly->limit < pointcount) (void)extendpolygon(poly, pointcount);
	poly->count = pointcount;
	poly->style = OPENED;

	for(pointnum = 0; pointnum < pointcount; pointnum++)
	{
		if (pointnum == 0)
		{
			poly->xv[pointnum] = sx;
			poly->yv[pointnum] = sy;
		} else if (pointnum == pointcount-1)
		{
			poly->xv[pointnum] = ex;
			poly->yv[pointnum] = ey;
		} else
		{
			poly->xv[pointnum] = cx + mult(cosine(curangle), radius);
			poly->yv[pointnum] = cy + mult(sine(curangle), radius);
		}

		curangle += angleincrement;
		if (curangle >= 3600) curangle -= 3600;
	}
}

/*
 * routine to convert a circle to line segments.  The circle is at (cx,cy) and has radius
 * "radius".  The polygon "poly" is loaded  with the line-approximation.  There is an arc
 * segment every "arcres" tenth-degrees and no segment sags from its true curvature by
 * more than "arcsag".
 */
void circletopoly(INTBIG cx, INTBIG cy, INTBIG radius, POLYGON *poly, INTBIG arcres, INTBIG arcsag)
{
	REGISTER INTBIG degreespersegment, segmentcount, pointcount,
		curangle, pointnum, angleincrement;
	REGISTER INTBIG sx, sy;
	double fradius, sag;

	sx = cx + radius;   sy = cy;

	/* determine number of degrees per segment */
	fradius = radius;
	sag = arcsag;
	degreespersegment = rounddouble(2.0 * acos((fradius - sag) / fradius) * 1800.0 / EPI);
	if (degreespersegment > arcres) degreespersegment = arcres;

	if (degreespersegment <= 0) degreespersegment = 1;
	segmentcount = 3600 / degreespersegment;
	if (segmentcount < 1) segmentcount = 1;
	angleincrement = 3600 / segmentcount;
	pointcount = segmentcount + 1;

	if (poly->limit < pointcount) (void)extendpolygon(poly, pointcount);
	poly->count = pointcount;
	poly->style = OPENED;

	curangle = 0;
	for(pointnum = 0; pointnum < pointcount; pointnum++)
	{
		if (pointnum == 0)
		{
			poly->xv[pointnum] = sx;
			poly->yv[pointnum] = sy;
		} else if (pointnum == pointcount-1)
		{
			poly->xv[pointnum] = sx;
			poly->yv[pointnum] = sy;
		} else
		{
			poly->xv[pointnum] = cx + mult(cosine(curangle), radius);
			poly->yv[pointnum] = cy + mult(sine(curangle), radius);
		}

		curangle += angleincrement;
		if (curangle >= 3600) curangle -= 3600;
	}
}

/*
 * Routine to determine whether an arc at angle "ang" can connect the two ports
 * whose bounding boxes are "lx1<=X<=hx1" and "ly1<=Y<=hy1" for port 1 and
 * "lx2<=X<=hx2" and "ly2<=Y<=hy2" for port 2.  Returns true if a line can
 * be drawn at that angle between the two ports and returns connection points
 * in (x1,y1) and (x2,y2)
 */
BOOLEAN arcconnects(INTBIG ang, INTBIG lx1, INTBIG hx1, INTBIG ly1, INTBIG hy1, INTBIG lx2,
	INTBIG hx2, INTBIG ly2, INTBIG hy2, INTBIG *x1, INTBIG *y1, INTBIG *x2, INTBIG *y2)
{
	float a, b, d, lx, hx, ly, hy, low1, high1, low2, high2;

	/* first try simple solutions */
	if ((ang%1800) == 0)
	{
		/* horizontal angle: simply test Y coordinates */
		if (ly1 > hy2 || ly2 > hy1) return(FALSE);
		*y1 = *y2 = (maxi(ly1, ly2) + mini(hy1, hy2)) / 2;
		*x1 = (lx1+hx1) / 2;
		*x2 = (lx2+hx2) / 2;
		return(TRUE);
	}
	if ((ang%1800) == 900)
	{
		/* vertical angle: simply test X coordinates */
		if (lx1 > hx2 || lx2 > hx1) return(FALSE);
		*x1 = *x2 = (maxi(lx1, lx2) + mini(hx1, hx2)) / 2;
		*y1 = (ly1+hy1) / 2;
		*y2 = (ly2+hy2) / 2;
		return(TRUE);
	}

	/* construct an imaginary line at the proper angle that runs through (0,0) */
	a = (float)sine(ang);      a /= 1073741824.0;
	b = (float)-cosine(ang);   b /= 1073741824.0;

	/* get the range of distances from the line to port 1 */
	lx = (float)lx1;   hx = (float)hx1;   ly = (float)ly1;   hy = (float)hy1;
	d = lx*a + ly*b;   low1 = high1 = d;
	d = hx*a + ly*b;   if (d < low1) low1 = d;   if (d > high1) high1 = d;
	d = hx*a + hy*b;   if (d < low1) low1 = d;   if (d > high1) high1 = d;
	d = lx*a + hy*b;   if (d < low1) low1 = d;   if (d > high1) high1 = d;

	/* get the range of distances from the line to port 2 */
	lx = (float)lx2;   hx = (float)hx2;   ly = (float)ly2;   hy = (float)hy2;
	d = lx*a + ly*b;   low2 = high2 = d;
	d = hx*a + ly*b;   if (d < low2) low2 = d;   if (d > high2) high2 = d;
	d = hx*a + hy*b;   if (d < low2) low2 = d;   if (d > high2) high2 = d;
	d = lx*a + hy*b;   if (d < low2) low2 = d;   if (d > high2) high2 = d;

	/* if the ranges do not overlap, a line cannot be drawn */
	if (low1 > high2 || low2 > high1) return(FALSE);

	/* the line can be drawn: determine equation (aX + bY = d) */
	d = ((low1 > low2 ? low1 : low2) + (high1 < high2 ? high1 : high2)) / 2.0f;

	/* determine intersection with polygon 1 */
	db_findconnectionpoint(lx1, hx1, ly1, hy1, a, b, d, x1, y1);
	db_findconnectionpoint(lx2, hx2, ly2, hy2, a, b, d, x2, y2);
	return(TRUE);
}

/*
 * Routine to find a point inside the rectangle bounded by (lx<=X<=hx, ly<=Y<=hy)
 * that satisfies the equation aX + bY = d.  Returns the point in (x,y).
 */
void db_findconnectionpoint(INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy, float a, float b,
	float d, INTBIG *x, INTBIG *y)
{
	float in, out;

	if (a != 0.0)
	{
		in = (float)ly;   *y = ly;   out = (d - b * in) / a;   *x = (INTBIG)out;
		if (*x >= lx && *x <= hx) return;
		in = (float)hy;   *y = hy;   out = (d - b * in) / a;   *x = (INTBIG)out;
		if (*x >= lx && *x <= hx) return;
	}
	if (b != 0.0)
	{
		in = (float)lx;   *x = lx;   out = (d - a * in) / b;   *y = (INTBIG)out;
		if (*y >= ly && *y <= hy) return;
		in = (float)hx;   *x = hx;   out = (d - a * in) / b;   *y = (INTBIG)out;
		if (*y >= ly && *y <= hy) return;
	}

	/* not the right solution, but nothing else works */
	*x = (lx+hx) / 2;  *y = (ly+hy) / 2;
}

/*
 * compute the quadrant of the point x,y   2 | 1
 * Standard quadrants are used:            -----
 *                                         3 | 4
 */
INTBIG db_quadrant(INTBIG x, INTBIG y)
{
	if (x > 0)
	{
		if (y >= 0) return(1);
		return(4);
	}
	if (y > 0) return(2);
	return(3);
}

/*
 * routine to compute the bounding box of the arc that runs clockwise from
 * (xs,ys) to (xe,ye) and is centered at (xc,yc).  The bounding box is
 * placed in the reference parameters "lx", "hx", "ly", and "hy".
 */
void arcbbox(INTBIG xs, INTBIG ys, INTBIG xe, INTBIG ye, INTBIG xc, INTBIG yc, INTBIG *lx,
	INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER INTBIG x1, y1, x2, y2, radius;
	REGISTER INTBIG q1, q2;

	/* determine radius and compute bounds of full circle */
	radius = computedistance(xc, yc, xs, ys);
	*lx = xc - radius;
	*ly = yc - radius;
	*hx = xc + radius;
	*hy = yc + radius;

	/* compute quadrant of two endpoints */
	x1 = xs - xc;    x2 = xe - xc;
	y1 = ys - yc;    y2 = ye - yc;
	q1 = db_quadrant(x1, y1);
	q2 = db_quadrant(x2, y2);

	/* see if the two endpoints are in the same quadrant */
	if (q1 == q2)
	{
		/* if the arc runs a full circle, use the MBR of the circle */
		if (q1 == 1 || q1 == 2)
		{
			if (x1 > x2) return;
		} else
		{
			if (x1 < x2) return;
		}

		/* use the MBR of the two arc points */
		*lx = mini(xs, xe);
		*hx = maxi(xs, xe);
		*ly = mini(ys, ye);
		*hy = maxi(ys, ye);
		return;
	}

	switch (q1)
	{
		case 1: switch (q2)
		{
			case 2:	/* 3 quadrants clockwise from Q1 to Q2 */
			*hy = maxi(y1,y2) + yc;
			break;

			case 3:	/* 2 quadrants clockwise from Q1 to Q3 */
			*lx = x2 + xc;
			*hy = y1 + yc;
			break;

			case 4:	/* 1 quadrant clockwise from Q1 to Q4 */
			*lx = mini(x1,x2) + xc;
			*ly = y2 + yc;
			*hy = y1 + yc;
			break;
		}
		break;

		case 2: switch (q2)
		{
			case 1:	/* 1 quadrant clockwise from Q2 to Q1 */
			*lx = x1 + xc;
			*ly = mini(y1,y2) + yc;
			*hx = x2 + xc;
			break;

			case 3:	/* 3 quadrants clockwise from Q2 to Q3 */
			*lx = mini(x1,x2) + xc;
			break;

			case 4:	/* 2 quadrants clockwise from Q2 to Q4 */
			*lx = x1 + xc;
			*ly = y2 + yc;
			break;
		}
		break;

		case 3: switch (q2)
		{
			case 1:	/* 2 quadrants clockwise from Q3 to Q1 */
			*ly = y1 + yc;
			*hx = x2 + xc;
			break;

			case 2:	/* 1 quadrant clockwise from Q3 to Q2 */
			*ly = y1 + yc;
			*hx = maxi(x1,x2) + xc;
			*hy = y2 + yc;
			break;

			case 4:	/* 3 quadrants clockwise from Q3 to Q4 */
			*ly = mini(y1,y2) + yc;
			break;
		}
		break;

		case 4: switch (q2)
		{
			case 1:	/* 3 quadrants clockwise from Q4 to Q1 */
			*hx = maxi(x1,x2) + xc;
			break;

			case 2:	/* 2 quadrants clockwise from Q4 to Q2 */
			*hx = x1 + xc;
			*hy = y2 + yc;
			break;

			case 3:	/* 1 quadrant clockwise from Q4 to Q3 */
			*lx = x2 + xc;
			*hx = x1 + xc;
			*hy = maxi(y1,y2) + yc;
			break;
		}
		break;
	}
}

/*
 * Routine to find the two possible centers for a circle whose radius is
 * "r" and has two points (x01,y01) and (x02,y02) on the edge.  The two
 * center points are returned in (x1,y1) and (x2,y2).  The distance between
 * the points (x01,y01) and (x02,y02) is in "d".  The routine returns
 * false if successful, true if there is no intersection.  This code
 * was written by John Mohammed of Schlumberger.
 */
BOOLEAN findcenters(INTBIG r, INTBIG x01, INTBIG y01, INTBIG x02, INTBIG y02, INTBIG d,
	INTBIG *x1, INTBIG *y1, INTBIG *x2, INTBIG *y2)
{
	float r2, delta_1, delta_12, delta_2;

	/* quit now if the circles concentric */
	if (x01 == x02 && y01 == y02) return(TRUE);

	/* find the intersections, if any */
	r2 = (float)r;   r2 *= r;
	delta_1 = -d / 2.0f;
	delta_12 = delta_1 * delta_1;

	/* quit if there are no intersections */
	if (r2 < delta_12) return(TRUE);

	/* compute the intersection points */
	delta_2 = (float)sqrt(r2 - delta_12);
	*x1 = x02 + (INTBIG)(((delta_1 * (x02 - x01)) + (delta_2 * (y02 - y01))) / d);
	*y1 = y02 + (INTBIG)(((delta_1 * (y02 - y01)) + (delta_2 * (x01 - x02))) / d);
	*x2 = x02 + (INTBIG)(((delta_1 * (x02 - x01)) + (delta_2 * (y01 - y02))) / d);
	*y2 = y02 + (INTBIG)(((delta_1 * (y02 - y01)) + (delta_2 * (x02 - x01))) / d);
	return(FALSE);
}

/*
 * routine to determine the point on the line segment that runs from
 * (x1,y1) to (x2,y2) that is closest to the point (*x,*y).  The value in
 * (*x,*y) is adjusted to be that close point and the distance is returned.
 */
INTBIG closestpointtosegment(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG dist1, dist2;
	INTBIG xi, yi;

	/* find closest point on line */
	db_closestpointtoline(x1,y1, x2,y2, *x,*y, &xi, &yi);

	/* see if that intersection point is actually on the segment */
	if (xi >= mini(x1, x2) && xi <= maxi(x1, x2) &&
		yi >= mini(y1, y2) && yi <= maxi(y1, y2))
	{
		/* it is: get the distance and return this point */
		dist1 = computedistance(*x, *y, xi, yi);
		*x = xi;   *y = yi;
		return(dist1);
	}

	/* intersection not on segment: choose one endpoint as the closest */
	dist1 = computedistance(*x, *y, x1, y1);
	dist2 = computedistance(*x, *y, x2, y2);
	if (dist2 < dist1)
	{
		*x = x2;   *y = y2;   return(dist2);
	}
	*x = x1;   *y = y1;
	return(dist1);
}

/*
 * routine to determine the point on the line that runs from (x1,y1) to
 * (x2,y2) that is closest to the point (xp,yp).  This point is returned
 * in (*xi,*yi).
 */
void db_closestpointtoline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG xp, INTBIG yp,
	INTBIG *xi, INTBIG *yi)
{
	float m, b, mi, bi, t;
	INTBIG newx, newy;

	/* special case for horizontal line */
	if (y1 == y2)
	{
		*xi = xp;
		*yi = y1;
		return;
	}

	/* special case for vertical line */
	if (x1 == x2)
	{
		*xi = x1;
		*yi = yp;
		return;
	}

	/* compute equation of the line */
	m = (float)(y1-y2);   m /= x1-x2;
	b = (float)-x1;   b *= m;   b += y1;

	/* compute perpendicular to line through the point */
	mi = -1.0f / m;
	bi = (float)-xp;  bi *= mi;  bi += yp;

	/* compute intersection of the lines */
	t = (bi-b) / (m-mi);
	newx = (INTBIG)t;
	newy = (INTBIG)(m * t + b);
	*xi = newx;   *yi = newy;
}

/*
 * routine to compute the distance between point (x,y) and the line that runs
 * from (x1,y1) to (x2,y2).
 */
INTBIG disttoline(INTBIG x1, INTBIG y1, INTBIG x2, INTBIG y2, INTBIG x, INTBIG y)
{
	REGISTER INTBIG ang;
	REGISTER INTBIG dist, ix, iy;
	INTBIG oix, oiy;

	/* get point on line from (x1,y1) to (x2,y2) close to (x,y) */
	if (x1 == x2 && y1 == y2)
	{
		return(computedistance(x, y, x1, y1));
	}
	ang = figureangle(x1,y1, x2,y2);
	(void)intersect(x1, y1, ang, x, y, (ang+900)%3600, &oix, &oiy);
	ix = oix;   iy = oiy;
	if (x1 == x2) ix = x1;
	if (y1 == y2) iy = y1;

	/* make sure (ix,iy) is on the segment from (x1,y1) to (x2,y2) */
	if (ix < mini(x1,x2) || ix > maxi(x1,x2) ||
		iy < mini(y1,y2) || iy > maxi(y1,y2))
	{
		if (abs(ix-x1) + abs(iy-y1) < abs(ix-x2) + abs(iy-y2))
		{
			ix = x1;   iy = y1;
		} else
		{
			ix = x2;   iy = y2;
		}
	}
	dist = computedistance(ix, iy, x, y);
	return(dist);
}

/*
 * Routine to scale "value" from database units to the requested display unit "dispunit".
 */
float scaletodispunit(INTBIG value, INTBIG dispunit)
{
	REGISTER float scalefactor;

	scalefactor = db_getcurrentscale(el_units&INTERNALUNITS, dispunit,
		el_curlib->lambda[el_curtech->techindex]);
	return((float)value / scalefactor);
}

/*
 * Routine to scale "value" from square database units to the requested square display unit "dispunit".
 */
float scaletodispunitsq(INTBIG value, INTBIG dispunit)
{
	REGISTER float scalefactor;

	scalefactor = db_getcurrentscale(el_units&INTERNALUNITS, dispunit,
		el_curlib->lambda[el_curtech->techindex]);
	return((float)value / scalefactor / scalefactor);
}

/*
 * Routine to scale "value" from the requested display unit "dispunit" to database units.
 */
INTBIG scalefromdispunit(float value, INTBIG dispunit)
{
	REGISTER float scalefactor;

	scalefactor = db_getcurrentscale(el_units&INTERNALUNITS, dispunit,
		el_curlib->lambda[el_curtech->techindex]);
	return(roundfloat(value * scalefactor));
}

/*
 * Routine to return the scaling factor between database units and display units.
 * The current value of lambda is provided for use when that is the unit.
 */
float db_getcurrentscale(INTBIG intunit, INTBIG dispunit, INTBIG lambda)
{
	switch (intunit&INTERNALUNITS)
	{
		case INTUNITHNM:
			switch (dispunit&DISPLAYUNITS)
			{
				case DISPUNITINCH: return(50800000.0f);
				case DISPUNITCM:   return(20000000.0f);
				case DISPUNITMM:   return(2000000.0f);
				case DISPUNITMIL:  return(50800.0f);
				case DISPUNITMIC:  return(2000.0f);
				case DISPUNITCMIC: return(20.0f);
				case DISPUNITNM:   return(2.0f);
			}
			break;
		case INTUNITHDMIC:
			switch (dispunit&DISPLAYUNITS)
			{
				case DISPUNITINCH: return(508000.0f);
				case DISPUNITCM:   return(200000.0f);
				case DISPUNITMM:   return(20000.0f);
				case DISPUNITMIL:  return(508.0f);
				case DISPUNITMIC:  return(20.0f);
				case DISPUNITCMIC: return(0.2f);
				case DISPUNITNM:   return(0.02f);
			}
			break;
	}
	return((float)lambda);
}

void db_getinternalunitscale(INTBIG *numerator, INTBIG *denominator, INTBIG oldunits, INTBIG newunits)
{
	if ((newunits&INTERNALUNITS) == INTUNITHNM)
	{
		/* scaling from half-decimicrons to half-nanometers (multiply by 100) */
		*numerator = 100;   *denominator = 1;
	} else
	{
		/* scaling from half-nanometers to half-decimicrons (divide by 100) */
		*numerator = 1;   *denominator = 100;
	}
}

/*
 * routine to crop the box in the reference parameters (lx-hx, ly-hy)
 * against the box in (bx-ux, by-uy).  If the box is cropped into oblivion,
 * the routine returns 1.  If the boxes overlap but cannot be cleanly cropped,
 * the routine returns -1.  Otherwise the box is cropped and zero is returned
 */
INTBIG cropbox(INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy, INTBIG bx, INTBIG ux, INTBIG by,
	INTBIG uy)
{
	REGISTER INTBIG crops;

	/* if the two boxes don't touch, just return */
	if (bx >= *hx || by >= *hy || ux <= *lx || uy <= *ly) return(0);

	/* if the box to be cropped is within the other, say so */
	if (bx <= *lx && ux >= *hx && by <= *ly && uy >= *hy) return(1);

	/* reduce (lx-hx,ly-hy) by (bx-ux,by-uy) */
	crops = 0;
	if (bx <= *lx && ux >= *hx)
	{
		/* it covers in X...crop in Y */
		if (uy >= *hy) *hy = by;
		if (by <= *ly) *ly = uy;
		crops++;
	}
	if (by <= *ly && uy >= *hy)
	{
		/* it covers in Y...crop in X */
		if (ux >= *hx) *hx = bx;
		if (bx <= *lx) *lx = ux;
		crops++;
	}
	if (crops == 0) return(-1);
	return(0);
}

/************************* SIMPLE MATHEMATICS *************************/

INTBIG mini(INTBIG a, INTBIG b)
{
	return(a<b ? a : b);
}

INTBIG maxi(INTBIG a, INTBIG b)
{
	return(a>b ? a : b);
}

INTBIG roundfloat(float a)
{
	if (a < 0.0) return((INTBIG)(a-0.5));
	return((INTBIG)(a+0.5));
}

INTBIG rounddouble(double a)
{
	if (a < 0.0) return((INTBIG)(a-0.5));
	return((INTBIG)(a+0.5));
}

/*
 * Routine to return true if "a" is equal to "b" within the epsilon of floating
 * point arithmetic.
 */
BOOLEAN floatsequal(float a, float b)
{
	if (fabs(a-b) <= FLT_EPSILON) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return true if "a" is equal to "b" within the epsilon of double floating
 * point arithmetic.
 */
BOOLEAN doublesequal(double a, double b)
{
	if (fabs(a-b) <= DBL_EPSILON) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return true if "a" is less than "b" within the epsilon of floating
 * point arithmetic.
 */
BOOLEAN floatslessthan(float a, float b)
{
	if (a+FLT_EPSILON < b) return(TRUE);
	return(FALSE);
}

/*
 * Routine to return true if "a" is less than "b" within the epsilon of double floating
 * point arithmetic.
 */
BOOLEAN doubleslessthan(double a, double b)
{
	if (a+DBL_EPSILON < b) return(TRUE);
	return(FALSE);
}

/*
 * Routine to truncate "a" to the lowest integral value.
 */
float floatfloor(float a)
{
	INTBIG i;

	i = (INTBIG)(a + FLT_EPSILON);
	return((float)i);
}

/*
 * Routine to truncate "a" to the lowest integral value.
 */
double doublefloor(double a)
{
	INTBIG i;

	i = (INTBIG)(a + DBL_EPSILON);
	return((float)i);
}

/* routine to convert a floating value to integer by casting! */
INTBIG castint(float a)
{
	union {
		INTBIG i;
		float x;
	} cast;

	cast.x = a;
	return(cast.i);
}

/* routine to convert an integer value to floating point by casting! */
float castfloat(INTBIG j)
{
	union {
		INTBIG i;
		float x;
	} cast;

	cast.i = j;
	return(cast.x);
}

/************************* 3D MATHEMATICS *************************/

void vectoradd3d(float *a, float *b, float *sum)
{
	sum[0] = a[0] + b[0];
	sum[1] = a[1] + b[1];
	sum[2] = a[2] + b[2];
}

void vectorsubtract3d(float *a, float *b, float *diff)
{
	diff[0] = a[0] - b[0];
	diff[1] = a[1] - b[1];
	diff[2] = a[2] - b[2];
}

void vectormultiply3d(float *a, float b, float *res)
{
	res[0] = a[0] * b;
	res[1] = a[1] * b;
	res[2] = a[2] * b;
}

void vectordivide3d(float *a, float b, float *res)
{
	res[0] = a[0] / b;
	res[1] = a[1] / b;
	res[2] = a[2] / b;
}

float vectormagnitude3d(float *a)
{
	return((float)sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]));
}

float vectordot3d(float *a, float *b)
{
	return(a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

void vectornormalize3d(float *a)
{
	float mag;

	mag = (float)sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	if (mag > 1.0e-11f)
	{
		a[0] /= mag;
		a[1] /= mag;
		a[2] /= mag;
	}
}

void vectorcross3d(float *a, float *b, float *res)
{
	res[0] = a[1] * b[2] - b[1] * a[2];
	res[1] = a[2] * b[0] - b[2] * a[0];
	res[2] = a[0] * b[1] - b[0] * a[1];
}

void matrixid3d(float xform[4][4])
{
	REGISTER INTBIG i, j;

	for(i=0; i<4; i++)
	{
		for(j=0; j<4; j++)
		{
			if (i == j) xform[i][j] = 1.0; else
				xform[i][j] = 0.0;
		}
	}
}

void matrixmult3d(float a[4][4], float b[4][4], float res[4][4])
{
	REGISTER INTBIG row, col, i;

	for (row = 0; row < 4; row++)
	{
		for (col = 0; col < 4; col++)
		{
			res[row][col] = 0.0f;
			for (i = 0; i < 4; i++)
			{
				res[row][col] += a[i][col] * b[row][i];
			}
		}
	}
}

/*
 * Routine to build the invert of the transformation.  Returns nonzero if a
 * singularity is detected.
 */
void matrixinvert3d(float in[4][4], float out[4][4])
{
	float a[4][8], temp;
	short i, j, k, l;

	/* copy into the matrix */
	for(i=0; i<4; i++)
	{
		for(j=0; j<8; j++)
		{
			if (j < 4) a[i][j] = in[i][j]; else
				if (j == i+4) a[i][j] = 1.0; else
					a[i][j] = 0.0;
		}
	}

	/* do the inversion */
	for(i=0; i<4; i++)
	{
		for(l=i; l<4; l++)
			if (a[l][i] != 0.0) break;
		if (l >= 4) return;
		if (l != i) for(k=0; k<8; k++)
		{
			temp = a[l][k];
			a[l][k] = a[i][k];
			a[i][k] = temp;
		}

		for(k=7; k>=i; k--) a[i][k] = a[i][k] / a[i][i];
		for(l=0; l<4; l++)
			if (l != i)
				for(k=7; k>=i; k--) a[l][k] = a[l][k] - a[i][k] * a[l][i];
	}

	/* copy back from the matrix */
	l = 0;
	for(i=0; i<4; i++) for(j=0; j<4; j++)
		out[i][j] = a[i][j+4];
}

void matrixxform3d(float *vec, float mat[4][4], float *res)
{
	res[0] = mat[0][0] * vec[0] + mat[0][1] * vec[1] + mat[0][2] * vec[2] + mat[0][3] * vec[3];
	res[1] = mat[1][0] * vec[0] + mat[1][1] * vec[1] + mat[1][2] * vec[2] + mat[1][3] * vec[3];
	res[2] = mat[2][0] * vec[0] + mat[2][1] * vec[1] + mat[2][2] * vec[2] + mat[2][3] * vec[3];
	res[3] = mat[3][0] * vec[0] + mat[3][1] * vec[1] + mat[3][2] * vec[2] + mat[2][3] * vec[3];
}

/************************* POLYGON MANIPULATION *************************/

/*
 * routine to allocate a polygon with "points" points from cluster "clus"
 * and return the address.  Returns NOPOLYGON if an error occurs.
 */
#ifdef DEBUGMEMORY
POLYGON *_allocpolygon(INTBIG points, CLUSTER *cluster, CHAR *filename, INTBIG lineno)
#else
POLYGON *allocpolygon(INTBIG points, CLUSTER *cluster)
#endif
{
	REGISTER POLYGON *poly;

	if (db_multiprocessing) emutexlock(db_polygonmutex);
	poly = NOPOLYGON;
	if (db_freepolygons != NOPOLYGON)
	{
		poly = db_freepolygons;
		db_freepolygons = poly->nextpolygon;
		if (poly->count < points) (void)extendpolygon(poly, points);
	}
	if (db_multiprocessing) emutexunlock(db_polygonmutex);

	if (poly == NOPOLYGON)
	{
#ifdef DEBUGMEMORY
		poly = (POLYGON *)_emalloc((sizeof (POLYGON)), cluster, filename, lineno);
		if (poly == 0) return((POLYGON *)db_error(DBNOMEM|DBALLOCPOLYGON));
		poly->xv = _emalloc((SIZEOFINTBIG*points), cluster, filename, lineno);
		if (poly->xv == 0) return((POLYGON *)db_error(DBNOMEM|DBALLOCPOLYGON));
		poly->yv = _emalloc((SIZEOFINTBIG*points), cluster, filename, lineno);
		if (poly->yv == 0) return((POLYGON *)db_error(DBNOMEM|DBALLOCPOLYGON));
#else
		poly = (POLYGON *)emalloc((sizeof (POLYGON)), cluster);
		if (poly == 0) return((POLYGON *)db_error(DBNOMEM|DBALLOCPOLYGON));
		poly->xv = emalloc((SIZEOFINTBIG*points), cluster);
		if (poly->xv == 0) return((POLYGON *)db_error(DBNOMEM|DBALLOCPOLYGON));
		poly->yv = emalloc((SIZEOFINTBIG*points), cluster);
		if (poly->yv == 0) return((POLYGON *)db_error(DBNOMEM|DBALLOCPOLYGON));
#endif
		poly->cluster = cluster;
		poly->limit = points;
	}

	poly->string = NOSTRING;
	poly->desc = NOGRAPHICS;
	poly->portproto = NOPORTPROTO;
	poly->count = 0;
	poly->style = FILLED;
	TDCLEAR(poly->textdescript);
	TDSETSIZE(poly->textdescript, TXTSETPOINTS(8));
	poly->tech = el_curtech;
	poly->layer = 0;
	poly->nextpolygon = NOPOLYGON;
	poly->numvar = 0;
	return(poly);
}

/*
 * routine to ensure that polygon "poly" is valid and has "points" points.
 * If not allocated, it is created from cluster "clus" and remembered as a
 * static polygon to be freed later.  Returns TRUE on error.
 */
BOOLEAN needstaticpolygon(POLYGON **poly, INTBIG points, CLUSTER *clus)
{
	/* if the polygon is already allocated, stop */
	if (*poly != NOPOLYGON)
	{
		if ((*poly)->count < points) (void)extendpolygon(*poly, points);
		return(FALSE);
	}

	/* lock the allocation and saving of the pointers */
	if (db_multiprocessing) emutexlock(db_polygonmutex);
	if (*poly == NOPOLYGON)
	{
		*poly = allocpolygon(points, clus);
		if (*poly != NOPOLYGON)
		{
			(*poly)->nextpolygon = db_staticpolygons;
			db_staticpolygons = *poly;
		}
	}
	if (db_multiprocessing) emutexunlock(db_polygonmutex);
	if (*poly == NOPOLYGON) return(TRUE);
	if ((*poly)->count < points) (void)extendpolygon(*poly, points);
	return(FALSE);
}

/*
 * routine to extend polygon "poly" to be "newcount" points.  Returns
 * true if there is an allocation error
 */
BOOLEAN extendpolygon(POLYGON *poly, INTBIG newcount)
{
	REGISTER INTBIG *oldxp, *oldyp, i;

	/* if polygon can already handle this many points, quit */
	if (newcount <= poly->limit) return(FALSE);

	/* save the old coordinates */
	oldxp = poly->xv;   oldyp = poly->yv;

	/* get space for the new ones */
	poly->xv = emalloc((SIZEOFINTBIG*newcount), poly->cluster);
	if (poly->xv == 0)
		return(db_error(DBNOMEM|DBEXTENDPOLYGON)!=0);
	poly->yv = emalloc((SIZEOFINTBIG*newcount), poly->cluster);
	if (poly->yv == 0)
		return(db_error(DBNOMEM|DBEXTENDPOLYGON)!=0);

	for(i=0; i<poly->limit; i++)
	{
		poly->xv[i] = oldxp[i];
		poly->yv[i] = oldyp[i];
	}

	poly->limit = newcount;
	efree((CHAR *)oldxp);
	efree((CHAR *)oldyp);
	return(FALSE);
}

/*
 * routine to free the memory associated with polygon "poly"
 */
void freepolygon(POLYGON *poly)
{
	if (db_multiprocessing) emutexlock(db_polygonmutex);
	poly->nextpolygon = db_freepolygons;
	db_freepolygons = poly;
	if (db_multiprocessing) emutexunlock(db_polygonmutex);
}

#ifdef DEBUGMEMORY
POLYLIST *_allocpolylist(CLUSTER *cluster, CHAR *filename, INTBIG lineno)
#else
POLYLIST *allocpolylist(CLUSTER *cluster)
#endif
{
	REGISTER POLYLIST *plist;

#ifdef DEBUGMEMORY
	plist = (POLYLIST *)_emalloc(sizeof(POLYLIST), cluster, filename, lineno);
#else
	plist = (POLYLIST *)emalloc(sizeof(POLYLIST), cluster);
#endif
	if (plist == 0) return(0);
	plist->polylisttotal = 0;
	return(plist);
}

/*
 * routine to accumulate a list of polygons at least "tot" long in the
 * polygon structure "list".  The routine returns true if there is no
 * more memory.
 */
#ifdef DEBUGMEMORY
BOOLEAN _ensurepolylist(POLYLIST *list, INTBIG tot, CLUSTER *cluster, CHAR *filename, INTBIG lineno)
#else
BOOLEAN ensurepolylist(POLYLIST *list, INTBIG tot, CLUSTER *cluster)
#endif
{
	REGISTER POLYGON **newpolylist;
	REGISTER INTBIG j, *newlx, *newhx, *newly, *newhy;

	/* make sure the list is the right size */
	if (tot > list->polylisttotal)
	{
#ifdef DEBUGMEMORY
		newpolylist = (POLYGON **)_emalloc(tot * (sizeof (POLYGON *)), dr_tool->cluster, filename, lineno);
		newlx = (INTBIG *)_emalloc(tot * SIZEOFINTBIG, dr_tool->cluster, filename, lineno);
		newhx = (INTBIG *)_emalloc(tot * SIZEOFINTBIG, dr_tool->cluster, filename, lineno);
		newly = (INTBIG *)_emalloc(tot * SIZEOFINTBIG, dr_tool->cluster, filename, lineno);
		newhy = (INTBIG *)_emalloc(tot * SIZEOFINTBIG, dr_tool->cluster, filename, lineno);
#else
		newpolylist = (POLYGON **)emalloc(tot * (sizeof (POLYGON *)), dr_tool->cluster);
		newlx = (INTBIG *)emalloc(tot * SIZEOFINTBIG, dr_tool->cluster);
		newhx = (INTBIG *)emalloc(tot * SIZEOFINTBIG, dr_tool->cluster);
		newly = (INTBIG *)emalloc(tot * SIZEOFINTBIG, dr_tool->cluster);
		newhy = (INTBIG *)emalloc(tot * SIZEOFINTBIG, dr_tool->cluster);
#endif
		if (newpolylist == 0 || newlx == 0 || newhx == 0 || newly == 0 || newhy == 0)
		{
			ttyputerr(_("DRC error allocating memory for polygons"));
			return(TRUE);
		}
		for(j=0; j<tot; j++) newpolylist[j] = NOPOLYGON;
		if (list->polylisttotal != 0)
		{
			for(j=0; j<list->polylisttotal; j++)
				newpolylist[j] = list->polygons[j];
			efree((CHAR *)list->polygons);
			efree((CHAR *)list->lx);
			efree((CHAR *)list->hx);
			efree((CHAR *)list->ly);
			efree((CHAR *)list->hy);
		}
		list->polygons = newpolylist;
		list->lx = newlx;
		list->hx = newhx;
		list->ly = newly;
		list->hy = newhy;
		list->polylisttotal = tot;
	}

	/* make sure there is a polygon for each entry in the list */
	for(j=0; j<tot; j++) if (list->polygons[j] == NOPOLYGON)
	{
#ifdef DEBUGMEMORY
		list->polygons[j] = _allocpolygon(4, cluster, filename, lineno);
#else
		list->polygons[j] = allocpolygon(4, cluster);
#endif
	}
	return(FALSE);
}

void freepolylist(POLYLIST *list)
{
	REGISTER INTBIG i;

	for(i=0; i<list->polylisttotal; i++)
		if (list->polygons[i] != NOPOLYGON)
			freepolygon(list->polygons[i]);
	if (list->polylisttotal > 0)
	{
		efree((CHAR *)list->polygons);
		efree((CHAR *)list->lx);
		efree((CHAR *)list->hx);
		efree((CHAR *)list->ly);
		efree((CHAR *)list->hy);
	}
	efree((CHAR *)list);
}

void db_deallocatepolygon(POLYGON *poly)
{
	efree((CHAR *)poly->xv);
	efree((CHAR *)poly->yv);
	efree((CHAR *)poly);
}

/* routine to transform polygon "poly" through the transformation "trans" */
void xformpoly(POLYGON *poly, XARRAY trans)
{
	REGISTER INTBIG i, x, y, det, t22, px, py;

	t22 = trans[2][2];
	if (poly->style == FILLEDRECT || poly->style == CLOSEDRECT)
	{
		/* rectangles must be fleshed-out if nonmanhattan */
		if (t22 == P1) maketruerect(poly);
	} else if (poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC)
	{
		/* special hack to reverse points in arcs if node is transposed */
		if (t22 != P1) det = db_determinant[t22]; else
			det = mult(trans[0][0], trans[1][1]) - mult(trans[0][1], trans[1][0]);
		if (det < 0) for(i=0; i<poly->count; i += 3)
		{
			x = poly->xv[i+1];              y = poly->yv[i+1];
			poly->xv[i+1] = poly->xv[i+2];  poly->yv[i+1] = poly->yv[i+2];
			poly->xv[i+2] = x;              poly->yv[i+2] = y;
		}
	}

	/* transform the polygon */
	px = trans[2][0];   py = trans[2][1];
	if (t22 != P1)
	{
		for(i=0; i<poly->count; i++)
		{
			x = poly->xv[i];  y = poly->yv[i];
			switch (t22)
			{
				case 0: poly->xv[i] =  x + px;  poly->yv[i] =  y + py;  break;
				case 1: poly->xv[i] = -y + px;  poly->yv[i] =  x + py;  break;
				case 2: poly->xv[i] = -x + px;  poly->yv[i] = -y + py;  break;
				case 3: poly->xv[i] =  y + px;  poly->yv[i] = -x + py;  break;
				case 4: poly->xv[i] = -y + px;  poly->yv[i] = -x + py;  break;
				case 5: poly->xv[i] = -x + px;  poly->yv[i] =  y + py;  break;
				case 6: poly->xv[i] =  y + px;  poly->yv[i] =  x + py;  break;
				case 7: poly->xv[i] =  x + px;  poly->yv[i] = -y + py;  break;
			}
		}
	} else
	{
		for(i=0; i<poly->count; i++)
		{
			x = poly->xv[i];  y = poly->yv[i];
			poly->xv[i] = mult(trans[0][0],x) + mult(trans[1][0],y) + px;
			poly->yv[i] = mult(trans[0][1],x) + mult(trans[1][1],y) + py;
		}
	}
}

/*
 * routine to ensure that polygon "poly" has true vertices, and is not described
 * as a "rectangle" with only two diagonal points.
 */
void maketruerect(POLYGON *poly)
{
	REGISTER INTBIG lx, ux, ly, uy;

	if (poly->style != FILLEDRECT && poly->style != CLOSEDRECT) return;

	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	lx = poly->xv[0];
	ux = poly->xv[1];
	ly = poly->yv[0];
	uy = poly->yv[1];

	poly->xv[0] = poly->xv[1] = lx;
	poly->xv[2] = poly->xv[3] = ux;
	poly->yv[0] = poly->yv[3] = ly;
	poly->yv[1] = poly->yv[2] = uy;
	poly->count = 4;
	if (poly->style == FILLEDRECT) poly->style = FILLED; else
		poly->style = CLOSED;
}

/*
 * routine to return true if the polygon in "poly" is an orthogonal box.
 * If the polygon is an orthogonal box, the bounds of that box are placed
 * in the reference integers "xl", "xh", "yl", and "yh".
 */
BOOLEAN isbox(POLYGON *poly, INTBIG *xl, INTBIG *xh, INTBIG *yl, INTBIG *yh)
{
	/* rectangular styles are always boxes */
	if (poly->style == FILLEDRECT || poly->style == CLOSEDRECT)
	{
		if (poly->xv[0] > poly->xv[1])
		{
			*xl = poly->xv[1];
			*xh = poly->xv[0];
		} else
		{
			*xl = poly->xv[0];
			*xh = poly->xv[1];
		}
		if (poly->yv[0] > poly->yv[1])
		{
			*yl = poly->yv[1];
			*yh = poly->yv[0];
		} else
		{
			*yl = poly->yv[0];
			*yh = poly->yv[1];
		}
		return(TRUE);
	}

	/* closed boxes must have exactly four points */
	if (poly->count == 4)
	{
		/* only closed polygons and text can be boxes */
		if ((poly->style < TEXTCENT || poly->style > TEXTBOX) &&
			poly->style != CROSSED && poly->style != FILLED && poly->style != CLOSED) return(FALSE);
	} else if (poly->count == 5)
	{
		if (poly->style < OPENED || poly->style > OPENEDO1) return(FALSE);
		if (poly->xv[0] != poly->xv[4] || poly->yv[0] != poly->yv[4]) return(FALSE);
	} else return(FALSE);

	/* make sure the polygon is rectangular and orthogonal */
	if (poly->xv[0] == poly->xv[1] && poly->xv[2] == poly->xv[3] &&
		poly->yv[0] == poly->yv[3] && poly->yv[1] == poly->yv[2])
	{
		if (poly->xv[0] < poly->xv[2])
		{
			*xl = poly->xv[0];  *xh = poly->xv[2];
		} else
		{
			*xl = poly->xv[2];  *xh = poly->xv[0];
		}
		if (poly->yv[0] < poly->yv[1])
		{
			*yl = poly->yv[0];  *yh = poly->yv[1];
		} else
		{
			*yl = poly->yv[1];  *yh = poly->yv[0];
		}
		return(TRUE);
	}
	if (poly->xv[0] == poly->xv[3] && poly->xv[1] == poly->xv[2] &&
		poly->yv[0] == poly->yv[1] && poly->yv[2] == poly->yv[3])
	{
		if (poly->xv[0] < poly->xv[1])
		{
			*xl = poly->xv[0];  *xh = poly->xv[1];
		} else
		{
			*xl = poly->xv[1];  *xh = poly->xv[0];
		}
		if (poly->yv[0] < poly->yv[2])
		{
			*yl = poly->yv[0];  *yh = poly->yv[2];
		} else
		{
			*yl = poly->yv[2];  *yh = poly->yv[0];
		}
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to return true if (X,Y) is inside of polygon "poly"
 */
BOOLEAN isinside(INTBIG x, INTBIG y, POLYGON *poly)
{
	INTBIG xl, xh, yl, yh;
	REGISTER INTBIG ang, lastp, tang, thisp, startangle, endangle;
	REGISTER INTBIG i, odist, dist, angrange, startdist, enddist, wantdist;

	switch (poly->style)
	{
		case FILLED:
		case CLOSED:
		case FILLEDRECT:
		case CLOSEDRECT:
		case CROSSED:
		case TEXTCENT:
		case TEXTTOP:
		case TEXTBOT:
		case TEXTLEFT:
		case TEXTRIGHT:
		case TEXTTOPLEFT:
		case TEXTBOTLEFT:
		case TEXTTOPRIGHT:
		case TEXTBOTRIGHT:
		case TEXTBOX:
			/* check rectangular case for containment */
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				if (x < xl || x > xh || y < yl || y > yh) return(FALSE);
				return(TRUE);
			}

			/* general polygon containment by summing angles to vertices */
			ang = 0;
			if (x == poly->xv[poly->count-1] && y == poly->yv[poly->count-1]) return(TRUE);
			lastp = figureangle(x, y, poly->xv[poly->count-1], poly->yv[poly->count-1]);
			for(i=0; i<poly->count; i++)
			{
				if (x == poly->xv[i] && y == poly->yv[i]) return(TRUE);
				thisp = figureangle(x, y, poly->xv[i], poly->yv[i]);
				tang = lastp - thisp;
				if (tang < -1800) tang += 3600;
				if (tang > 1800) tang -= 3600;
				ang += tang;
				lastp = thisp;
			}
			if (abs(ang) <= poly->count) return(FALSE);
			return(TRUE);

		case CROSS:
		case BIGCROSS:
			/* polygon is only a single point */
			getcenter(poly, &xl, &yl);
			if (xl == x && yl == y) return(TRUE);
			return(FALSE);

		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
		case VECTORS:
			/* first look for trivial inclusion by being a vertex */
			for(i=0; i<poly->count; i++)
				if (poly->xv[i] == x && poly->yv[i] == y) return(TRUE);

			/* see if the point is on one of the edges */
			if (poly->style == VECTORS)
			{
				for(i=0; i<poly->count; i += 2)
					if (isonline(poly->xv[i],poly->yv[i], poly->xv[i+1], poly->yv[i+1], x, y))
						return(TRUE);
			} else
			{
				for(i=1; i<poly->count; i++)
					if (isonline(poly->xv[i-1],poly->yv[i-1], poly->xv[i], poly->yv[i], x, y))
						return(TRUE);
			}
			return(FALSE);

		case CIRCLE: case THICKCIRCLE:
		case DISC:
			dist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			odist = computedistance(poly->xv[0], poly->yv[0], x, y);
			if (odist < dist) return(TRUE);
			return(FALSE);

		case CIRCLEARC: case THICKCIRCLEARC:
			/* first see if the point is at the proper angle from the center of the arc */
			ang = figureangle(poly->xv[0], poly->yv[0], x, y);
			endangle = figureangle(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			startangle = figureangle(poly->xv[0], poly->yv[0], poly->xv[2], poly->yv[2]);
			if (endangle > startangle)
			{
				if (ang < startangle || ang > endangle) return(FALSE);
				angrange = endangle - startangle;
			} else
			{
				if (ang < startangle && ang > endangle) return(FALSE);
				angrange = 3600 - startangle + endangle;
			}

			/* now see if the point is the proper distance from the center of the arc */
			dist = computedistance(poly->xv[0], poly->yv[0], x, y);
			if (ang == startangle || angrange == 0)
			{
				wantdist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			} else if (ang == endangle)
			{
				wantdist = computedistance(poly->xv[0], poly->yv[0], poly->xv[2], poly->yv[2]);
			} else
			{
				startdist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
				enddist = computedistance(poly->xv[0], poly->yv[0], poly->xv[2], poly->yv[2]);
				if (enddist == startdist) wantdist = startdist; else
				{
					wantdist = startdist + (ang - startangle) / angrange *
						(enddist - startdist);
				}
			}
			if (dist == wantdist) return(TRUE);
			return(FALSE);
	}

	/* I give up */
	return(FALSE);
}

/*
 * Routine to compute the minimum size of polygon "poly".
 * Only works with manhattan geometry.
 */
INTBIG polyminsize(POLYGON *poly)
{
	INTBIG xl, xh, yl, yh;

	if (isbox(poly, &xl, &xh, &yl, &yh))
	{
		if (xh-xl < yh-yl) return(xh-xl);
		return(yh-yl);
	}
	return(0);
}

/*
 * Routine to determine whether polygon "poly" intersects the rectangle defined by
 * lx <= X <= hx and ly <= Y <= hy.  Returns true if so.
 */
BOOLEAN polyinrect(POLYGON *poly, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER INTBIG i, count;
	INTBIG sx, sy, ex, ey, plx, phx, ply, phy;
	REGISTER POLYGON *mypoly;

	switch (poly->style)
	{
		case FILLED:
			if (isinside(lx, ly, poly)) return(TRUE);
			if (isinside(lx, hy, poly)) return(TRUE);
			if (isinside(hx, hy, poly)) return(TRUE);
			if (isinside(hx, ly, poly)) return(TRUE);
			/* FALLTHROUGH */ 
		case CLOSED:
		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
		case OPENEDO1:
			for(i=0; i<poly->count; i++)
			{
				if (i == 0)
				{
					if (poly->style != CLOSED) continue;
					sx = poly->xv[poly->count-1];
					sy = poly->yv[poly->count-1];
				} else
				{
					sx = poly->xv[i-1];   sy = poly->yv[i-1];
				}
				ex = poly->xv[i];   ey = poly->yv[i];
				if (!clipline(&sx, &sy, &ex, &ey, lx, hx, ly, hy)) return(TRUE);
			}
			return(FALSE);
		case VECTORS:
			for(i=0; i<poly->count; i += 2)
			{
				sx = poly->xv[i];     sy = poly->yv[i];
				ex = poly->xv[i+1];   ey = poly->yv[i+1];
				if (!clipline(&sx, &sy, &ex, &ey, lx, hx, ly, hy)) return(TRUE);
			}
			return(FALSE);
		case CIRCLE:    case THICKCIRCLE:
		case DISC:
		case CIRCLEARC: case THICKCIRCLEARC:
			mypoly = allocpolygon(poly->count, db_cluster);
			mypoly->count = poly->count;
			mypoly->style = poly->style;
			for(i=0; i<poly->count; i++)
			{
				mypoly->xv[i] = poly->xv[i];
				mypoly->yv[i] = poly->yv[i];
			}
			cliparc(mypoly, lx, hx, ly, hy);
			count = mypoly->count;
			freepolygon(mypoly);
			if (count != 0) return(TRUE);
			return(FALSE);
	}
	getbbox(poly, &plx, &phx, &ply, &phy);
	if (phx < lx || plx > hx || phy < ly || ply > hy) return(FALSE);
	return(TRUE);
}

/*
 * compute and return the area of the polygon in "poly".  The calculation
 * may return a negative value if the polygon points are counter-clockwise
 */
float areapoly(POLYGON *poly)
{
	REGISTER INTBIG sign;
	float area;
	INTBIG xl, xh, yl, yh;

	switch (poly->style)
	{
		case FILLED:
		case CLOSED:
		case FILLEDRECT:
		case CLOSEDRECT:
		case CROSSED:
		case TEXTCENT:
		case TEXTTOP:
		case TEXTBOT:
		case TEXTLEFT:
		case TEXTRIGHT:
		case TEXTTOPLEFT:
		case TEXTBOTLEFT:
		case TEXTTOPRIGHT:
		case TEXTBOTRIGHT:
		case TEXTBOX:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				area = (float)(xh-xl);
				area *= (float)(yh-yl);

				/* now determine the sign of the area */
				if (poly->xv[0] == poly->xv[1])
				{
					/* first line is vertical */
					sign = (poly->xv[2] - poly->xv[1]) * (poly->yv[1] - poly->yv[0]);
				} else
				{
					/* first line is horizontal */
					sign = (poly->xv[1] - poly->xv[0]) * (poly->yv[1] - poly->yv[2]);
				}
				if (sign < 0) area = -area;
				return(area);
			}

			return(areapoints(poly->count, poly->xv, poly->yv));
	}
	return(0.0);
}

/*
 * reverse the edge order of a polygon
 */
void reversepoly(POLYGON *poly)
{
	REGISTER INTBIG i, invi;
	REGISTER INTBIG mx, my;

	for (i = 0, invi = poly->count-1; i<invi; i++, invi--)
	{
		mx = poly->xv[invi];
		my = poly->yv[invi];
		poly->xv[invi] = poly->xv[i];
		poly->yv[invi] = poly->yv[i];
		poly->xv[i] = mx;
		poly->yv[i] = my;
	}
}

/*
 * Compute and return the area of the polygon in the "count" points (x,y).
 * The calculation may return a negative value if the polygon points are
 * counter-clockwise.
 */
float areapoints(INTBIG count, INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG i, x0, y0, x1, y1;
	float p1, p2, partial, area;

	area = 0.0;
	x0 = x[0];
	y0 = y[0];
	for (i=1; i<count; i++)
	{
		x1 = x[i];
		y1 = y[i];

		/* triangulate around the polygon */
		p1 = (float)(x1 - x0);
		p2 = (float)(y0 + y1);
		partial = p1 * p2;
		area += partial / 2.0f;
		x0 = x1;
		y0 = y1;
	}
	p1 = (float)(x[0] - x0);
	p2 = (float)(y[0] + y1);
	partial = p1 * p2;
	area += partial / 2.0f;
	return(area);
}

/*
 * routine to return the center point of polygon "poly" in "(x,y)"
 */
void getcenter(POLYGON *poly, INTBIG *x, INTBIG *y)
{
	REGISTER INTBIG i;
	INTBIG xl, xh, yl, yh;

	switch (poly->style)
	{
		case FILLED:
		case FILLEDRECT:
		case CROSSED:
		case CROSS:
		case BIGCROSS:
		case TEXTCENT:
		case TEXTTOP:
		case TEXTBOT:
		case TEXTLEFT:
		case TEXTRIGHT:
		case TEXTTOPLEFT:
		case TEXTBOTLEFT:
		case TEXTTOPRIGHT:
		case TEXTBOTRIGHT:
		case TEXTBOX:
			if (isbox(poly, &xl, &xh, &yl, &yh))
			{
				*x = (xl + xh) / 2;
				*y = (yl + yh) / 2;
				return;
			}

			*x = *y = 0;
			for(i=0; i<poly->count; i++)
			{
				*x += poly->xv[i];   *y += poly->yv[i];
			}
			*x /= poly->count;   *y /= poly->count;
			return;

		case CLOSED:
			/* merely use the center point */
			*x = poly->xv[poly->count/2];   *y = poly->yv[poly->count/2];
			return;

		case CLOSEDRECT:
			/* merely use the first point */
			*x = poly->xv[0];   *y = poly->yv[0];
			return;

		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
			/* if there are a odd number of lines, use center of middle one */
			if ((poly->count & 1) == 0)
			{
				*x = (poly->xv[poly->count/2] + poly->xv[poly->count/2-1]) / 2;
				*y = (poly->yv[poly->count/2] + poly->yv[poly->count/2-1]) / 2;
				return;
			}
			/* FALLTHROUGH */ 

		case VECTORS:
			/* if there are a odd number of lines, use center of middle one */
			if ((poly->count & 3) == 0)
			{
				*x = (poly->xv[poly->count/2] + poly->xv[poly->count/2-1]) / 2;
				*y = (poly->yv[poly->count/2] + poly->yv[poly->count/2-1]) / 2;
				return;
			}
			*x = poly->xv[poly->count/2];   *y = poly->yv[poly->count/2];
			return;

		case DISC:
			*x = poly->xv[0];   *y = poly->yv[0];
			return;

		case CIRCLE:    case THICKCIRCLE:
		case CIRCLEARC: case THICKCIRCLEARC:
			*x = poly->xv[1];   *y = poly->yv[1];
			return;
	}
}

/*
 * routine to report the distance of point (x,y) to polygon "poly".  The
 * routine returns a negative amount if the point is a direct hit on or in
 * the polygon (the more negative, the closer to the center)
 */
INTBIG polydistance(POLYGON *poly, INTBIG x, INTBIG y)
{
	INTBIG lx, hx, ly, hy;
	REGISTER INTBIG sty, pang, sang, eang;
	REGISTER INTBIG i, dist, cx, cy, bestdist, odist, sdist, edist;

	/* handle single point polygons */
	sty = poly->style;
	if (sty == CROSS || sty == BIGCROSS || poly->count == 1)
	{
		getcenter(poly, &lx, &ly);
		if (lx == x && ly == y) return(-MAXINTBIG);
		return(computedistance(lx, ly, x, y));
	}

	/* handle polygons that are filled in */
	if (sty == FILLED || sty == FILLEDRECT || sty == CROSSED ||
		(sty >= TEXTCENT && sty <= TEXTBOX))
	{
		/* give special returned value if point is a direct hit */
		if (isinside(x, y, poly))
		{
			getcenter(poly, &lx, &ly);
			return(computedistance(lx, ly, x, y) - MAXINTBIG);
		}

		/* if polygon is a box, use M.B.R. information */
		if (isbox(poly, &lx, &hx, &ly, &hy))
		{
			if (x > hx) cx = x - hx; else if (x < lx) cx = lx - x; else cx = 0;
			if (y > hy) cy = y - hy; else if (y < ly) cy = ly - y; else cy = 0;
			if (cx == 0 || cy == 0) return(cx + cy);
			return(computedistance(0, 0, cx, cy));
		}

		/* point is outside of irregular polygon: fall into to next case */
		if (sty == FILLEDRECT) sty = CLOSEDRECT; else sty = CLOSED;
	}

	/* handle closed outline figures */
	if (sty == CLOSED)
	{
		bestdist = MAXINTBIG;
		for(i=0; i<poly->count; i++)
		{
			if (i == 0)
			{
				lx = poly->xv[poly->count-1];
				ly = poly->yv[poly->count-1];
			} else
			{
				lx = poly->xv[i-1];
				ly = poly->yv[i-1];
			}
			hx = poly->xv[i];   hy = poly->yv[i];

			/* compute distance of close point to (x,y) */
			dist = disttoline(lx,ly, hx,hy, x,y);
			if (dist < bestdist) bestdist = dist;
		}
		return(bestdist);
	}

	/* handle closed rectangle figures */
	if (sty == CLOSEDRECT)
	{
		bestdist = MAXINTBIG;
		for(i=0; i<4; i++)
		{
			lx = poly->xv[db_rectx[i]];
			ly = poly->yv[db_recty[i]];
			hx = poly->xv[db_rectx[(i+1)&3]];
			hy = poly->yv[db_recty[(i+1)&3]];

			/* compute distance of close point to (x,y) */
			dist = disttoline(lx,ly, hx,hy, x,y);
			if (dist < bestdist) bestdist = dist;
		}
		return(bestdist);
	}

	/* handle opened outline figures */
	if (sty >= OPENED && sty <= OPENEDO1)
	{
		bestdist = MAXINTBIG;
		for(i=1; i<poly->count; i++)
		{
			lx = poly->xv[i-1];   ly = poly->yv[i-1];
			hx = poly->xv[i];     hy = poly->yv[i];

			/* compute distance of close point to (x,y) */
			dist = disttoline(lx,ly, hx,hy, x,y);
			if (dist < bestdist) bestdist = dist;
		}
		return(bestdist);
	}

	/* handle outline vector lists */
	if (sty == VECTORS)
	{
		bestdist = MAXINTBIG;
		for(i=0; i<poly->count; i += 2)
		{
			lx = poly->xv[i];     ly = poly->yv[i];
			hx = poly->xv[i+1];   hy = poly->yv[i+1];

			/* compute distance of close point to (x,y) */
			dist = disttoline(lx,ly, hx,hy, x,y);
			if (dist < bestdist) bestdist = dist;
		}
		return(bestdist);
	}

	/* handle circular objects */
	if (sty == CIRCLE || sty == THICKCIRCLE || sty == DISC)
	{
		odist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
		dist = computedistance(poly->xv[0], poly->yv[0], x, y);
		if (sty == DISC && dist < odist) return(dist-MAXINTBIG);
		return(abs(dist-odist));
	}
	if (sty == CIRCLEARC || sty == THICKCIRCLEARC)
	{
		/* determine closest point to ends of arc */
		sdist = computedistance(x, y, poly->xv[1], poly->yv[1]);
		edist = computedistance(x, y, poly->xv[2], poly->yv[2]);
		dist = mini(sdist,edist);

		/* see if the point is in the segment of the arc */
		pang = figureangle(poly->xv[0], poly->yv[0], x, y);
		sang = figureangle(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
		eang = figureangle(poly->xv[0], poly->yv[0], poly->xv[2], poly->yv[2]);
		if (eang > sang)
		{
			if (pang < eang && pang > sang) return(dist);
		} else
		{
			if (pang < eang || pang > sang) return(dist);
		}

		/* point in arc: determine distance */
		odist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
		dist = computedistance(poly->xv[0], poly->yv[0], x, y);
		return(abs(dist-odist));
	}

	/* grid polygons cover everything */
	if (sty == GRIDDOTS) return(-1);

	/* can't figure out others: use distance to polygon center */
	getcenter(poly, &lx, &ly);
	return(computedistance(lx, ly, x, y));
}

/*
 * Routine to return the distance between polygons "poly1" and "poly2".
 * Returns zero if they touch or overlap.
 */
INTBIG polyseparation(POLYGON *poly1, POLYGON *poly2)
{
	INTBIG cx, cy;
	REGISTER INTBIG i, pd, minpd;

	/* stop now if they touch */
	if (polyintersect(poly1, poly2)) return(0);

	/* look at all points on polygon 1 */
	for(i=0; i<poly1->count; i++)
	{
		cx = poly1->xv[i];   cy = poly1->yv[i];
		closestpoint(poly2, &cx, &cy);
		pd = computedistance(poly1->xv[i], poly1->yv[i], cx, cy);
		if (pd <= 0) return(0);
		if (i == 0) minpd = pd; else
		{
			if (pd < minpd) minpd = pd;
		}
	}

	/* look at all points on polygon 2 */
	for(i=0; i<poly2->count; i++)
	{
		cx = poly2->xv[i];   cy = poly2->yv[i];
		closestpoint(poly1, &cx, &cy);
		pd = computedistance(poly2->xv[i], poly2->yv[i], cx, cy);
		if (pd <= 0) return(0);
		if (pd < minpd) minpd = pd;
	}
	return(minpd);
}

/*
 * routine to return true if polygons "poly1" and "poly2" intersect (that is,
 * if any of their lines intersect).
 */
BOOLEAN polyintersect(POLYGON *poly1, POLYGON *poly2)
{
	INTBIG lx1, hx1, ly1, hy1, lx2, hx2, ly2, hy2;
	REGISTER INTBIG i, px, py, tx, ty;

	/* quit now if bounding boxes don't overlap */
	getbbox(poly1, &lx1, &hx1, &ly1, &hy1);
	getbbox(poly2, &lx2, &hx2, &ly2, &hy2);
	if (hx1 < lx2 || hx2 < lx1 || hy1 < ly2 || hy2 < ly1) return(FALSE);

	/* make sure these polygons are not the "RECT" type */
	maketruerect(poly1);
	maketruerect(poly2);

	/* check each line in polygon 1 */
	for(i=0; i<poly1->count; i++)
	{
		if (i == 0)
		{
			if (poly1->style == OPENED || poly1->style == OPENEDT1 ||
				poly1->style == OPENEDT2 || poly1->style == OPENEDT3 ||
				poly1->style == VECTORS) continue;
			px = poly1->xv[poly1->count-1];
			py = poly1->yv[poly1->count-1];
		} else
		{
			px = poly1->xv[i-1];
			py = poly1->yv[i-1];
		}
		tx = poly1->xv[i];
		ty = poly1->yv[i];
		if (poly1->style == VECTORS && (i&1) != 0) i++;
		if (px == tx && py == ty) continue;

		/* compare this line with polygon 2 */
		if (mini(px,tx) > hx2 || maxi(px,tx) < lx2 ||
			mini(py,ty) > hy2 || maxi(py,ty) < ly2) continue;
		if (db_lineintersect(px,py, tx,ty, poly2)) return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to return true if the line segment from (px1,py1) to (tx1,ty1)
 * intersects any line in polygon "poly"
 */
BOOLEAN db_lineintersect(INTBIG px1, INTBIG py1, INTBIG tx1, INTBIG ty1, POLYGON *poly)
{
	REGISTER INTBIG i, px2, py2, tx2, ty2, ang, ang1, ang2;
	INTBIG ix, iy;

	for(i=0; i<poly->count; i++)
	{
		if (i == 0)
		{
			if (poly->style == OPENED || poly->style == OPENEDT1 ||
				poly->style == OPENEDT2 || poly->style == OPENEDT3 ||
				poly->style == VECTORS) continue;
			px2 = poly->xv[poly->count-1];
			py2 = poly->yv[poly->count-1];
		} else
		{
			px2 = poly->xv[i-1];
			py2 = poly->yv[i-1];
		}
		tx2 = poly->xv[i];
		ty2 = poly->yv[i];
		if (poly->style == VECTORS && (i&1) != 0) i++;
		if (px2 == tx2 && py2 == ty2) continue;

		/* special case: this line is vertical */
		if (px2 == tx2)
		{
			/* simple bounds check */
			if (mini(px1,tx1) > px2 || maxi(px1,tx1) < px2) continue;

			if (px1 == tx1)
			{
				if (mini(py1,ty1) > maxi(py2,ty2) ||
					maxi(py1,ty1) < mini(py2,ty2)) continue;
				return(TRUE);
			}
			if (py1 == ty1)
			{
				if (mini(py2,ty2) > py1 || maxi(py2,ty2) < py1) continue;
				return(TRUE);
			}
			ang = figureangle(px1,py1, tx1,ty1);
			(void)intersect(px2,py2, 900, px1,py1, ang, &ix, &iy);
			if (ix != px2 || iy < mini(py2,ty2) || iy > maxi(py2,ty2)) continue;
			return(TRUE);
		}

		/* special case: this line is horizontal */
		if (py2 == ty2)
		{
			/* simple bounds check */
			if (mini(py1,ty1) > py2 || maxi(py1,ty1) < py2) continue;

			if (py1 == ty1)
			{
				if (mini(px1,tx1) > maxi(px2,tx2) ||
					maxi(px1,tx1) < mini(px2,tx2)) continue;
				return(TRUE);
			}
			if (px1 == tx1)
			{
				if (mini(px2,tx2) > px1 || maxi(px2,tx2) < px1) continue;
				return(TRUE);
			}
			ang = figureangle(px1,py1, tx1,ty1);
			(void)intersect(px2,py2, 0, px1,py1, ang, &ix, &iy);
			if (iy != py2 || ix < mini(px2,tx2) || ix > maxi(px2,tx2)) continue;
			return(TRUE);
		}

		/* simple bounds check */
		if (mini(px1,tx1) > maxi(px2,tx2) || maxi(px1,tx1) < mini(px2,tx2) ||
			mini(py1,ty1) > maxi(py2,ty2) || maxi(py1,ty1) < mini(py2,ty2)) continue;

		/* general case of line intersection */
		ang1 = figureangle(px1,py1, tx1,ty1);
		ang2 = figureangle(px2,py2, tx2,ty2);
		(void)intersect(px2, py2, ang2, px1, py1, ang1, &ix, &iy);
		if (ix < mini(px2,tx2) || ix > maxi(px2,tx2) ||
			iy < mini(py2,ty2) || iy > maxi(py2,ty2) ||
			ix < mini(px1,tx1) || ix > maxi(px1,tx1) ||
			iy < mini(py1,ty1) || iy > maxi(py1,ty1)) continue;
		return(TRUE);
	}
	return(FALSE);
}

/*
 * routine to determine the closest point on polygon "poly" to the point
 * (*x, *y) and return that point in (*x, *y).  Thus, the coordinate value
 * is adjusted to the closest point in or on the polygon
 */
void closestpoint(POLYGON *poly, INTBIG *x, INTBIG *y)
{
	INTBIG lx, hx, ly, hy, nx, ny;
	REGISTER INTBIG i, dist, gx, gy, bestdist;

	switch (poly->style)
	{
		case CROSS:
		case BIGCROSS:
			/* single point polygons simply use the center */
			getcenter(poly, x, y);
			return;

		case FILLED:
		case FILLEDRECT:
		case CROSSED:
		case TEXTCENT:
		case TEXTTOP:
		case TEXTBOT:
		case TEXTLEFT:
		case TEXTRIGHT:
		case TEXTTOPLEFT:
		case TEXTBOTLEFT:
		case TEXTTOPRIGHT:
		case TEXTBOTRIGHT:
		case TEXTBOX:
			/* filled polygon: check for regularity first */
			if (isbox(poly, &lx, &hx, &ly, &hy))
			{
				if (*x < lx) *x = lx;   if (*x > hx) *x = hx;
				if (*y < ly) *y = ly;   if (*y > hy) *y = hy;
				return;
			}
			if (poly->style == FILLED || poly->style == FILLEDRECT)
			{
				if (isinside(*x, *y, poly)) return;
			}

			/* FALLTHROUGH */ 

		case CLOSED:
			/* check outline of description */
			bestdist = MAXINTBIG;
			for(i=0; i<poly->count; i++)
			{
				nx = *x;   ny = *y;
				if (i == 0)
				{
					lx = poly->xv[poly->count-1];
					ly = poly->yv[poly->count-1];
				} else
				{
					lx = poly->xv[i-1];   ly = poly->yv[i-1];
				}
				dist = closestpointtosegment(lx,ly, poly->xv[i], poly->yv[i], &nx, &ny);
				if (dist > bestdist) continue;
				bestdist = dist;
				gx = nx;   gy = ny;
			}
			*x = gx;   *y = gy;
			return;

		case CLOSEDRECT:
			/* check outline of description */
			bestdist = MAXINTBIG;
			for(i=0; i<4; i++)
			{
				nx = *x;   ny = *y;
				lx = poly->xv[db_rectx[i]];
				ly = poly->yv[db_recty[i]];
				hx = poly->xv[db_rectx[(i+1)&3]];
				hy = poly->yv[db_recty[(i+1)&3]];
				dist = closestpointtosegment(lx,ly, hx, hy, &nx, &ny);
				if (dist > bestdist) continue;
				bestdist = dist;
				gx = nx;   gy = ny;
			}
			*x = gx;   *y = gy;
			return;

		case OPENED:
		case OPENEDT1:
		case OPENEDT2:
		case OPENEDT3:
			/* check outline of description */
			bestdist = MAXINTBIG;
			for(i=1; i<poly->count; i++)
			{
				nx = *x;   ny = *y;
				dist = closestpointtosegment(poly->xv[i-1], poly->yv[i-1],
					poly->xv[i], poly->yv[i], &nx, &ny);
				if (dist > bestdist) continue;
				bestdist = dist;
				gx = nx;   gy = ny;
			}
			*x = gx;   *y = gy;
			return;

		case VECTORS:
			/* check outline of description */
			bestdist = MAXINTBIG;
			for(i=0; i<poly->count; i = i + 2)
			{
				nx = *x;   ny = *y;
				dist = closestpointtosegment(poly->xv[i], poly->yv[i],
					poly->xv[i+1], poly->yv[i+1], &nx, &ny);
				if (dist > bestdist) continue;
				bestdist = dist;
				gx = nx;   gy = ny;
			}
			*x = gx;   *y = gy;
			return;
	}
}

/*
 * routine to determine the minimum bounding rectangle of the polygon in
 * "poly" and return it in the reference parameters "lx", "hx", "ly", and "hy"
 */
void getbbox(POLYGON *poly, INTBIG *lx, INTBIG *hx, INTBIG *ly, INTBIG *hy)
{
	REGISTER INTBIG i, dist;

	/* rectangular styles store information differently */
	if (poly->style == FILLEDRECT || poly->style == CLOSEDRECT)
	{
		*lx = mini(poly->xv[0], poly->xv[1]);
		*hx = maxi(poly->xv[0], poly->xv[1]);
		*ly = mini(poly->yv[0], poly->yv[1]);
		*hy = maxi(poly->yv[0], poly->yv[1]);
		return;
	}

	/* special code for circles */
	if (poly->style == CIRCLE || poly->style == THICKCIRCLE || poly->style == DISC)
	{
		dist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
		*lx = poly->xv[0] - dist;
		*hx = poly->xv[0] + dist;
		*ly = poly->yv[0] - dist;
		*hy = poly->yv[0] + dist;
		return;
	}

	/* special code for arcs */
	if (poly->style == CIRCLEARC || poly->style == THICKCIRCLEARC)
	{
		arcbbox(poly->xv[1], poly->yv[1], poly->xv[2], poly->yv[2],
			poly->xv[0], poly->yv[0], lx, hx, ly, hy);
		return;
	}

	/* just compute the bounds of the points */
	*lx = *hx = poly->xv[0];   *ly = *hy = poly->yv[0];
	for(i=1; i<poly->count; i++)
	{
		if (poly->xv[i] < *lx) *lx = poly->xv[i];
		if (poly->xv[i] > *hx) *hx = poly->xv[i];
		if (poly->yv[i] < *ly) *ly = poly->yv[i];
		if (poly->yv[i] > *hy) *hy = poly->yv[i];
	}
}

/*
 * routine to compare two polygons and return true if they are the same
 */
BOOLEAN polysame(POLYGON *poly1, POLYGON *poly2)
{
	INTBIG xl1, xh1, yl1, yh1, xl2, xh2, yl2, yh2;
	REGISTER INTBIG i;

	/* polygons must have the same number of points */
	if (poly1->count != poly2->count) return(FALSE);

	/* if both are boxes, compare their extents */
	if (isbox(poly1, &xl1, &xh1, &yl1, &yh1))
	{
		if (isbox(poly2, &xl2, &xh2, &yl2, &yh2))
		{
			/* compare box extents */
			if (xl1 == xl2 && xh1 == xh2 && yl1 == yl2 && yh1 == yh2) return(TRUE);
		}
		return(FALSE);
	} else if (isbox(poly2, &xl2, &xh2, &yl2, &yh2)) return(FALSE);

	/* compare these boxes the hard way */
	for(i=0; i<poly1->count; i++)
		if (poly1->xv[i] != poly2->xv[i] || poly1->yv[i] != poly2->yv[i]) return(FALSE);
	return(TRUE);
}

/*
 * routine to copy polygon "spoly" to polygon "dpoly"
 */
void polycopy(POLYGON *dpoly, POLYGON *spoly)
{
	REGISTER INTBIG i;

	if (dpoly->limit < spoly->count) (void)extendpolygon(dpoly, spoly->count);
	for(i=0; i<spoly->count; i++)
	{
		dpoly->xv[i] = spoly->xv[i];
		dpoly->yv[i] = spoly->yv[i];
	}
	dpoly->count = spoly->count;
	dpoly->style = spoly->style;
	dpoly->string = spoly->string;
	TDCOPY(dpoly->textdescript, spoly->textdescript);
	dpoly->tech = spoly->tech;
	dpoly->desc = spoly->desc;
	dpoly->layer = spoly->layer;
	dpoly->portproto = spoly->portproto;
}

/*
 * routine to make a polygon that describes the rectangle running from
 * (lx,ly) to (hx,hy).
 */
void makerectpoly(INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, POLYGON *poly)
{
	if (poly->limit < 4) (void)extendpolygon(poly, 4);
	poly->xv[0] = poly->xv[1] = lx;
	poly->xv[2] = poly->xv[3] = ux;
	poly->yv[0] = poly->yv[3] = ly;
	poly->yv[1] = poly->yv[2] = uy;
	poly->count = 4;
}

/*
 * routine to make a rectangle polygon that describes the rectangle running from
 * (lx,ly) to (hx,hy).
 */
void maketruerectpoly(INTBIG lx, INTBIG ux, INTBIG ly, INTBIG uy, POLYGON *poly)
{
	if (poly->limit < 2) (void)extendpolygon(poly, 2);
	poly->xv[0] = lx;
	poly->xv[1] = ux;
	poly->yv[0] = ly;
	poly->yv[1] = uy;
	poly->count = 2;
}

/*
 * routine to reduce polygon "poly" by the proper amount given that it is the
 * description of port "pp" on nodeinst "ni" and will be connected to an arc
 * with width "wid" (this arc width should be the offset width, not the actual
 * width).  The angle of the arc is "angle" (negative if angle should not be
 * considered).  The polygon is modified.
 */
void reduceportpoly(POLYGON *poly, NODEINST *ni, PORTPROTO *pp, INTBIG wid, INTBIG angle)
{
	INTBIG lx, ly, hx, hy, cx, cy, bx, by, ux, uy, plx, phx, ply, phy;
	REGISTER INTBIG j, realwid, dist, swapi;
	XARRAY trans, localtran, t1;

	/* find bottommost node */
	makerot(ni, trans);
	while (ni->proto->primindex == 0)
	{
		maketrans(ni, localtran);
		transmult(localtran, trans, t1);
		ni = pp->subnodeinst;
		pp = pp->subportproto;
		makerot(ni, localtran);
		transmult(localtran, t1, trans);
	}

	/* do not reduce port if not filled */
	if (poly->style != FILLED && poly->style != FILLEDRECT &&
		poly->style != CROSSED && poly->style != DISC) return;

	/* do not reduce port areas on polygonally defined nodes */
	if (gettrace(ni) != NOVARIABLE) return;

	/* determine amount to reduce port */
	realwid = wid / 2;

	/* get bounding box of port polygon */
	if (!isbox(poly, &bx, &ux, &by, &uy))
	{
		/* special case: nonrectangular port */
		if (poly->style == DISC)
		{
			/* shrink discs */
			dist = computedistance(poly->xv[0], poly->yv[0], poly->xv[1], poly->yv[1]);
			dist = maxi(0, dist-realwid);
			poly->xv[1] = poly->xv[0] + dist;
			poly->yv[1] = poly->yv[0];
			return;
		}

		/* cannot handle polygons yet */
		return;
	}

	/* determine the center of the port polygon */
	getcenter(poly, &cx, &cy);

	/* compute the area of the nodeinst */
	nodesizeoffset(ni, &plx, &ply, &phx, &phy);
	xform(ni->lowx+plx, ni->lowy+ply, &lx, &ly, trans);
	xform(ni->highx-phx, ni->highy-phy, &hx, &hy, trans);
	if (lx > hx) { swapi = lx;  lx = hx;  hx = swapi; }
	if (ly > hy) { swapi = ly;  ly = hy;  hy = swapi; }

	/* do not reduce in X if arc is horizontal */
	if (angle != 0 && angle != 1800)
	{
		/* determine reduced port area */
		lx = maxi(bx, lx + realwid);   hx = mini(ux, hx - realwid);
		if (hx < lx) hx = lx = (hx + lx) / 2;

		/* only clip in X if the port area is within of the reduced node X area */
		if (ux >= lx && bx <= hx)
			for(j=0; j<poly->count; j++)
		{
			if (poly->xv[j] < lx) poly->xv[j] = lx;
			if (poly->xv[j] > hx) poly->xv[j] = hx;
		}
	}

	/* do not reduce in Y if arc is vertical */
	if (angle != 900 && angle != 2700)
	{
		/* determine reduced port area */
		ly = maxi(by, ly + realwid);   hy = mini(uy, hy - realwid);
		if (hy < ly) hy = ly = (hy + ly) / 2;

		/* only clip in Y if the port area is inside of the reduced node Y area */
		if (uy >= ly && by <= hy)
			for(j=0; j<poly->count; j++)
		{
			if (poly->yv[j] < ly) poly->yv[j] = ly;
			if (poly->yv[j] > hy) poly->yv[j] = hy;
		}
	}
}

/*
 * Routine that splits the polygon "which" horizontally at "yl".
 * The part above is placed in "abovep" and the part below in "belowp"
 * Returns true on error.
 */
BOOLEAN polysplithoriz(INTBIG yl, POLYGON *which, POLYGON **abovep, POLYGON **belowp)
{
	INTBIG xt, yt, xn, yn, i, pind, x, y;
	POLYGON *above, *below;

	/* create two new polygons and delete the original */
	above = allocpolygon(4, which->cluster);
	if (above == 0) return(TRUE);
	below = allocpolygon(4, which->cluster);
	if (below == 0) return(TRUE);
	above->count = below->count = 0;

	/* run through points in polygon to be split */
	for(i=0; i<which->count; i++)
	{
		if (i == 0) pind = which->count-1; else pind = i-1;
		xt = which->xv[pind];  yt = which->yv[pind];
		xn = which->xv[i];     yn = which->yv[i];

		/* if point is on line, put in both polygons */
		if (yn == yl)
		{
			if (db_addpointtopoly(xn, yn, above)) return(TRUE);
			if (db_addpointtopoly(xn, yn, below)) return(TRUE);
			continue;
		}

		/* if both points are on same side, put in appropriate polygon */
		if (yn >= yl && yt >= yl)
		{
			if (db_addpointtopoly(xn, yn, above)) return(TRUE);
			continue;
		}
		if (yn <= yl && yt <= yl)
		{
			if (db_addpointtopoly(xn, yn, below)) return(TRUE);
			continue;
		}

		/* lines cross, intersect them */
		y = yl;
		if (yn == yl) x = xn; else
			if (yt == yl) x = xt; else
		{
			x = xt + muldiv(y-yt, xn-xt, yn-yt);
		}

		/* insert new point in polygons */
		if (yt <= yl && yn >= yl)
		{
			if (db_addpointtopoly(x, y, above)) return(TRUE);
			if (db_addpointtopoly(x, y, below)) return(TRUE);
			if (db_addpointtopoly(xn, yn, above)) return(TRUE);
			continue;
		}
		if (yt >= yl && yn <= yl)
		{
			if (db_addpointtopoly(x, y, above)) return(TRUE);
			if (db_addpointtopoly(x, y, below)) return(TRUE);
			if (db_addpointtopoly(xn, yn, below)) return(TRUE);
			continue;
		}
	}

	*abovep = above;
	*belowp = below;
	return(FALSE);
}

/*
 * Routine that splits the polygon "which" vertically at "xl".
 * The part to the left is placed in "leftp" and the part to the right in "rightp".
 * Returns true on error.
 */
BOOLEAN polysplitvert(INTBIG xl, POLYGON *which, POLYGON **leftp, POLYGON **rightp)
{
	INTBIG xt, yt, xn, yn, i, pind, x, y;
	POLYGON *left, *right;

	/* create two new polygons and delete the original */
	left = allocpolygon(4, which->cluster);
	if (left == 0) return(TRUE);
	right = allocpolygon(4, which->cluster);
	if (right == 0) return(TRUE);
	left->count = right->count = 0;

	/* run through points in polygon to be split */
	for(i=0; i<which->count; i++)
	{
		if (i == 0) pind = which->count-1; else pind = i-1;
		xt = which->xv[pind];  yt = which->yv[pind];
		xn = which->xv[i];     yn = which->yv[i];

		/* if point is on line, put in both polygons */
		if (xn == xl)
		{
			if (db_addpointtopoly(xn, yn, left)) return(TRUE);
			if (db_addpointtopoly(xn, yn, right)) return(TRUE);
			continue;
		}

		/* if both points are on same side, put in appropriate polygon */
		if (xn <= xl && xt <= xl)
		{
			if (db_addpointtopoly(xn, yn, left)) return(TRUE);
			continue;
		}
		if (xn >= xl && xt >= xl)
		{
			if (db_addpointtopoly(xn, yn, right)) return(TRUE);
			continue;
		}

		/* lines cross, intersect them */
		x = xl;
		if (xn == xl) y = yn; else
			if (xt == xl) y = yt; else
		{
			y = yt + muldiv(x-xt, yn-yt, xn-xt);
		}

		/* insert new point in polygons */
		if (xt >= xl && xn <= xl)
		{
			if (db_addpointtopoly(x, y, left)) return(TRUE);
			if (db_addpointtopoly(x, y, right)) return(TRUE);
			if (db_addpointtopoly(xn, yn, left)) return(TRUE);
			continue;
		}
		if (xt <= xl && xn >= xl)
		{
			if (db_addpointtopoly(x, y, left)) return(TRUE);
			if (db_addpointtopoly(x, y, right)) return(TRUE);
			if (db_addpointtopoly(xn, yn, right)) return(TRUE);
			continue;
		}
	}

	*leftp = left;
	*rightp = right;
	return(FALSE);
}

BOOLEAN db_addpointtopoly(INTBIG x, INTBIG y, POLYGON *poly)
{
	if (poly->count >= poly->limit)
	{
		if (extendpolygon(poly, poly->limit*2)) return(TRUE);
	}
	poly->xv[poly->count] = x;    poly->yv[poly->count] = y;
	poly->count++;
	return(FALSE);
}

/************************* CLIPPING *************************/

/*
 * Routine to clip a line from (fx,fy) to (tx,ty) in the rectangle lx <= X <= hx and ly <= Y <= hy.
 * Returns true if the line is not visible.
 */
BOOLEAN clipline(INTBIG *fx, INTBIG *fy, INTBIG *tx, INTBIG *ty, INTBIG lx, INTBIG hx,
	INTBIG ly, INTBIG hy)
{
	REGISTER INTBIG t, fc, tc;

	for(;;)
	{
		/* compute code bits for "from" point */
		fc = 0;
		if (*fx < lx) fc |= LEFT; else
			if (*fx > hx) fc |= RIGHT;
		if (*fy < ly) fc |= BOTTOM; else
			if (*fy > hy) fc |= TOP;

		/* compute code bits for "to" point */
		tc = 0;
		if (*tx < lx) tc |= LEFT; else
			if (*tx > hx) tc |= RIGHT;
		if (*ty < ly) tc |= BOTTOM; else
			if (*ty > hy) tc |= TOP;

		/* look for trivial acceptance or rejection */
		if (fc == 0 && tc == 0) return(FALSE);
		if (fc == tc || (fc & tc) != 0) return(TRUE);

		/* make sure the "from" side needs clipping */
		if (fc == 0)
		{
			t = *fx;   *fx = *tx;   *tx = t;
			t = *fy;   *fy = *ty;   *ty = t;
			t = fc;    fc = tc;     tc = t;
		}

		if ((fc&LEFT) != 0)
		{
			if (*tx == *fx) return(TRUE);
			t = muldiv(*ty - *fy, lx - *fx, *tx - *fx);
			*fy += t;
			*fx = lx;
		}
		if ((fc&RIGHT) != 0)
		{
			if (*tx == *fx) return(TRUE);
			t = muldiv(*ty - *fy, hx - *fx, *tx - *fx);
			*fy += t;
			*fx = hx;
		}
		if ((fc&BOTTOM) != 0)
		{
			if (*ty == *fy) return(TRUE);
			t = muldiv(*tx - *fx, ly - *fy, *ty - *fy);
			*fx += t;
			*fy = ly;
		}
		if ((fc&TOP) != 0)
		{
			if (*ty == *fy) return(TRUE);
			t = muldiv(*tx - *fx, hy - *fy, *ty - *fy);
			*fx += t;
			*fy = hy;
		}
	}
}

typedef struct
{
	double angle;
	INTBIG x, y;
} ANGLELIST;
	
/*
 * Helper routine for "esort" that makes angles go in descending order (used by "cliparc()").
 */
int db_anglelistdescending(const void *e1, const void *e2)
{
	REGISTER ANGLELIST *c1, *c2;
	REGISTER double diff;

	c1 = (ANGLELIST *)e1;
	c2 = (ANGLELIST *)e2;
	diff = c2->angle - c1->angle;
	if (diff < 0.0) return(-1);
	if (diff > 0.0) return(1);
	return(0);
}

/*
 * Routine to clips curved polygon (CIRCLE, THICKCIRCLE, DISC, CIRCLEARC, or THICKCIRCLEARC)
 * against the rectangle lx <= X <= hx and ly <= Y <= hy.
 * Adjusts the polygon to contain the visible portions.
 */
void cliparc(POLYGON *in, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	INTBIG plx, ply, phx, phy, ix1, iy1, ix2, iy2;
	REGISTER INTBIG xc, yc, xp, yp, midx, midy;
	double radius, midangle, dx, dy;
	REGISTER INTBIG i, j, intcount, ints, initialcount, prev;
	ANGLELIST anglelist[10];

	getbbox(in, &plx, &phx, &ply, &phy);

	/* if not clipped, stop now */
	if (plx >= lx && phx <= hx && ply >= ly && phy <= hy) return;

	/* if totally invisible, blank the polygon */
	if (plx > hx || phx < lx || ply > hy || phy < ly)
	{
		in->count = 0;
		return;
	}

	/* initialize list of relevant points */
	xc = in->xv[0];   yc = in->yv[0];
	xp = in->xv[1];   yp = in->yv[1];
	intcount = 0;

	if (in->style == CIRCLEARC || in->style == THICKCIRCLEARC)
	{
		/* include arc endpoints */
		anglelist[intcount].x = xp;       anglelist[intcount].y = yp;
		dx = (double)(xp-xc);      dy = (double)(yp-yc);
		if (dx == 0.0 && dy == 0.0)
		{
			ttyputerr(_("Domain error doing circle/circle tangents"));
			in->count = 0;
			return;
		}
		anglelist[intcount].angle = atan2(dy, dx);
		intcount++;
		anglelist[intcount].x = in->xv[2];
		anglelist[intcount].y = in->yv[2];
		dx = (double)(anglelist[intcount].x-xc);
		dy = (double)(anglelist[intcount].y-yc);
		if (dx == 0.0 && dy == 0.0)
		{
			ttyputerr(_("Domain error doing circle/circle tangents"));
			in->count = 0;
			return;
		}
		anglelist[intcount].angle = atan2(dy, dx);
		intcount++;
	}
	initialcount = intcount;

	/* find intersection points along left edge */
	ints = circlelineintersection(xc, yc, xp, yp, lx, ly, lx, hy,
		&ix1, &iy1, &ix2, &iy2, 0);
	if (ints > 0) { anglelist[intcount].x = ix1;   anglelist[intcount++].y = iy1; }
	if (ints > 1) { anglelist[intcount].x = ix2;   anglelist[intcount++].y = iy2; }

	/* find intersection points along top edge */
	ints = circlelineintersection(xc, yc, xp, yp, lx, hy, hx, hy,
		&ix1, &iy1, &ix2, &iy2, 0);
	if (ints > 0) { anglelist[intcount].x = ix1;   anglelist[intcount++].y = iy1; }
	if (ints > 1) { anglelist[intcount].x = ix2;   anglelist[intcount++].y = iy2; }

	/* find intersection points along right edge */
	ints = circlelineintersection(xc, yc, xp, yp, hx, hy, hx, ly,
		&ix1, &iy1, &ix2, &iy2, 0);
	if (ints > 0) { anglelist[intcount].x = ix1;   anglelist[intcount++].y = iy1; }
	if (ints > 1) { anglelist[intcount].x = ix2;   anglelist[intcount++].y = iy2; }

	/* find intersection points along bottom edge */
	ints = circlelineintersection(xc, yc, xp, yp, hx, ly, lx, ly,
		&ix1, &iy1, &ix2, &iy2, 0);
	if (ints > 0) { anglelist[intcount].x = ix1;   anglelist[intcount++].y = iy1; }
	if (ints > 1) { anglelist[intcount].x = ix2;   anglelist[intcount++].y = iy2; }

	/* if there are no intersections, arc is invisible */
	if (intcount == initialcount) { in->count = 0;   return; }

	/* determine angle of intersection points */
	for(i=initialcount; i<intcount; i++)
	{
		if (anglelist[i].y == yc && anglelist[i].x == xc)
		{
			ttyputerr(_("Warning: instability ahead"));
			in->count = 0;
			return;
		}
		dx = (double)(anglelist[i].x-xc);  dy = (double)(anglelist[i].y-yc);
		if (dx == 0.0 && dy == 0.0)
		{
			ttyputerr(_("Domain error doing circle/circle tangents"));
			in->count = 0;
			return;
		}
		anglelist[i].angle = atan2(dy, dx);
	}

	/* reject points not on the arc */
	if (in->style == CIRCLEARC || in->style == THICKCIRCLEARC)
	{
		j = 2;
		for(i=2; i<intcount; i++)
		{
			if (anglelist[0].angle > anglelist[1].angle)
			{
				if (anglelist[i].angle > anglelist[0].angle ||
					anglelist[i].angle < anglelist[1].angle) continue;
			} else
			{
				if (anglelist[i].angle > anglelist[0].angle &&
					anglelist[i].angle < anglelist[1].angle) continue;
			}
			anglelist[j].x = anglelist[i].x;
			anglelist[j].y = anglelist[i].y;
			anglelist[j].angle = anglelist[i].angle;
			j++;
		}
		intcount = j;

		/* make sure the start of the arc is the first point */
		for(i=0; i<intcount; i++)
			if (anglelist[i].angle > anglelist[0].angle)
				anglelist[i].angle -= EPI*2.0;
	} else
	{
		/* make sure all angles are negative */
		for(i=0; i<intcount; i++)
			if (anglelist[i].angle > 0.0) anglelist[i].angle -= EPI*2.0;
	}

	/* sort by angle */
	esort(anglelist, intcount, sizeof (ANGLELIST), db_anglelistdescending);

	/* for full circles, add in starting point to complete circle */
	if (in->style != CIRCLEARC && in->style != THICKCIRCLEARC)
	{
		anglelist[intcount].x = anglelist[0].x;
		anglelist[intcount].y = anglelist[0].y;
		anglelist[intcount].angle = anglelist[0].angle - EPI*2.0;
		intcount++;
	}

	/* now examine each segment and add it, if it is in the window */
	radius = (double)computedistance(xc, yc, xp, yp);
	in->count = 0;
	for(i=1; i<intcount; i++)
	{
		prev = i-1;
		midangle = (anglelist[prev].angle + anglelist[i].angle) / 2.0;
		while (midangle < -EPI) midangle += EPI * 2.0;
		midx = xc + rounddouble(radius * cos(midangle));
		midy = yc + rounddouble(radius * sin(midangle));
		if (midx < lx || midx > hx || midy < ly || midy > hy) continue;

		/* add this segment */
		if (in->limit < in->count+3) (void)extendpolygon(in, in->count+3);
		in->xv[in->count] = xc;                 in->yv[in->count++] = yc;
		in->xv[in->count] = anglelist[prev].x;  in->yv[in->count++] = anglelist[prev].y;
		in->xv[in->count] = anglelist[i].x;     in->yv[in->count++] = anglelist[i].y;
	}
	if (in->style == THICKCIRCLE) in->style = THICKCIRCLEARC; else
		if (in->style == CIRCLE) in->style = CIRCLEARC;
}

void clippoly(POLYGON *in, INTBIG lx, INTBIG hx, INTBIG ly, INTBIG hy)
{
	REGISTER INTBIG i;
	REGISTER INTBIG pre;
	REGISTER POLYGON *a, *b, *swap;
	REGISTER POLYGON *out;

	/* see if any points are outside */
	pre = 0;
	for(i=0; i<in->count; i++)
	{
		if (in->xv[i] < lx) pre |= LEFT; else
			if (in->xv[i] > hx) pre |= RIGHT;
		if (in->yv[i] < ly) pre |= BOTTOM; else
			if (in->yv[i] > hy) pre |= TOP;
	}
	if (pre == 0) return;

	/* get polygon */
	out = allocpolygon(4, db_cluster);

	/* clip on all four sides */
	a = in;   b = out;
	out->style = in->style;

	if ((pre & LEFT) != 0)
	{
		db_clipedge(a, b, LEFT, lx);
		swap = a;   a = b;   b = swap;
	}
	if ((pre & RIGHT) != 0)
	{
		db_clipedge(a, b, RIGHT, hx);
		swap = a;   a = b;   b = swap;
	}
	if ((pre & TOP) != 0)
	{
		db_clipedge(a, b, TOP, hy);
		swap = a;   a = b;   b = swap;
	}
	if ((pre & BOTTOM) != 0)
	{
		db_clipedge(a, b, BOTTOM, ly);
		swap = a;   a = b;   b = swap;
	}

	/* remove redundant points from polygon */
	for(pre=0, i=0; i<a->count; i++)
	{
		if (i > 0 && a->xv[i-1] == a->xv[i] && a->yv[i-1] == a->yv[i]) continue;
		if (pre >= b->limit) (void)extendpolygon(b, pre+1);
		b->xv[pre] = a->xv[i];   b->yv[pre++] = a->yv[i];
	}

	/* if polygon is not opened, remove redundancy on wrap-around */
	if (in->style != OPENED)
		while (pre != 0 && b->xv[0] == b->xv[pre-1] && b->yv[0] == b->yv[pre-1]) pre--;

	b->count = pre;

	/* copy the polygon back if it in the wrong place */
	if (b != in)
	{
		if (in->limit < b->count) (void)extendpolygon(in, b->count);
		for(i=0; i<b->count; i++)
		{
			in->xv[i] = b->xv[i];
			in->yv[i] = b->yv[i];
		}
		in->count = b->count;
	}
	freepolygon(out);
}

/*
 * routine to clip polygon "in" against line "edge" (1:left, 2:right,
 * 4:bottom, 8:top) and place clipped result in "out".
 */
void db_clipedge(POLYGON *in, POLYGON *out, INTBIG edge, INTBIG value)
{
	REGISTER INTBIG outcount;
	REGISTER INTBIG i, pre, firstx, firsty;
	INTBIG x1, y1, x2, y2;

	/* look at all the lines */
	outcount = 0;
	if (in->style != OPENED)
	{
		for(i=0; i<in->count; i++)
		{
			if (i == 0) pre = in->count-1; else pre = i-1;
			x1 = in->xv[pre];  y1 = in->yv[pre];
			x2 = in->xv[i];    y2 = in->yv[i];
			if (db_clip(&x1, &y1, &x2, &y2, edge, value)) continue;
			if (outcount+1 >= out->limit) (void)extendpolygon(out, outcount+2);
			if (outcount != 0)
			{
				if (x1 != out->xv[outcount-1] || y1 != out->yv[outcount-1])
				{
					out->xv[outcount] = x1;  out->yv[outcount++] = y1;
				}
			} else { firstx = x1;  firsty = y1; }
			out->xv[outcount] = x2;  out->yv[outcount++] = y2;
		}
		if (outcount != 0 && (out->xv[outcount-1] != firstx || out->yv[outcount-1] != firsty))
		{
			out->xv[outcount] = firstx;   out->yv[outcount++] = firsty;
		}
	} else
	{
		for(i=0; i<in->count-1; i++)
		{
			x1 = in->xv[i];  y1 = in->yv[i];
			x2 = in->xv[i+1];y2 = in->yv[i+1];
			if (db_clip(&x1, &y1, &x2, &y2, edge, value)) continue;
			if (out->limit < outcount+2) (void)extendpolygon(out, outcount+2);
			out->xv[outcount] = x1;  out->yv[outcount++] = y1;
			out->xv[outcount] = x2;  out->yv[outcount++] = y2;
		}
	}
	out->count = outcount;
}

/*
 * Routine to do clipping on the vector from (x1,y1) to (x2,y2).
 * If the vector is completely invisible, true is returned.
 */
BOOLEAN db_clip(INTBIG *xp1, INTBIG *yp1, INTBIG *xp2, INTBIG *yp2, INTBIG codebit, INTBIG value)
{
	REGISTER INTBIG t, x1, y1, x2, y2, c1, c2;
	REGISTER BOOLEAN flip;

	x1 = *xp1;   y1 = *yp1;
	x2 = *xp2;   y2 = *yp2;

	c1 = c2 = 0;
	if (codebit == LEFT)
	{
		if (x1 < value) c1 = codebit;
		if (x2 < value) c2 = codebit;
	} else if (codebit == BOTTOM)
	{
		if (y1 < value) c1 = codebit;
		if (y2 < value) c2 = codebit;
	} else if (codebit == RIGHT)
	{
		if (x1 > value) c1 = codebit;
		if (x2 > value) c2 = codebit;
	} else if (codebit == TOP)
	{
		if (y1 > value) c1 = codebit;
		if (y2 > value) c2 = codebit;
	}

	if (c1 == c2) return(c1 != 0);
	if (c1 == 0)
	{
		t = x1;   x1 = x2;   x2 = t;
		t = y1;   y1 = y2;   y2 = t;
		flip = TRUE;
	} else flip = FALSE;
	if (codebit == LEFT || codebit == RIGHT)
	{
		t = muldiv(y2-y1, value-x1, x2-x1);
		y1 += t;
		x1 = value;
	} else if (codebit == BOTTOM || codebit == TOP)
	{
		t = muldiv(x2-x1, value-y1, y2-y1);
		x1 += t;
		y1 = value;
	}
	if (flip)
	{
		*xp1 = x2;   *yp1 = y2;
		*xp2 = x1;   *yp2 = y1;
	} else
	{
		*xp1 = x1;   *yp1 = y1;
		*xp2 = x2;   *yp2 = y2;
	}
	return(0);
}

/************************* WINDOWPART CONTROL *************************/

/*
 * routine to make a new window at location "l".  If "protow" is not NOWINDOWPART,
 * use it for other information.  Returns NOWINDOWPART upon error
 */
WINDOWPART *newwindowpart(CHAR *l, WINDOWPART *protow)
{
	REGISTER WINDOWPART *w;
	REGISTER VARIABLE *var;
	REGISTER INTBIG lambda;

	if (db_windowpartfree == NOWINDOWPART)
	{
		w = (WINDOWPART *)emalloc((sizeof (WINDOWPART)), db_cluster);
		if (w == 0)
		{
			db_donextchangequietly = FALSE;
			return(NOWINDOWPART);
		}
	} else
	{
		/* take window from free list */
		w = db_windowpartfree;
		db_windowpartfree = w->nextwindowpart;
	}

	if (protow != NOWINDOWPART)
	{
		copywindowpart(w, protow);
	} else
	{
		lambda = el_curlib->lambda[el_curtech->techindex];
		w->uselx = -lambda * 10;
		w->usehx =  lambda * 10;
		w->usely = -lambda * 10;
		w->usehy =  lambda * 10;
		w->screenlx = -lambda * 10;
		w->screenhx =  lambda * 10;
		w->screenly = -lambda * 10;
		w->screenhy =  lambda * 10;
		w->hratio = w->vratio = 50;
		computewindowscale(w);
		w->curnodeproto = NONODEPROTO;
		w->topnodeproto = NONODEPROTO;
		w->inplacedepth = 0;
		transid(w->intocell);
		transid(w->outofcell);
		var = getval((INTBIG)us_tool, VTOOL, VFRACT|VISARRAY, x_("USER_default_grid"));
		if (var == NOVARIABLE)
		{
			w->gridx = w->gridy = WHOLE;
		} else
		{
			w->gridx = ((INTBIG *)var->addr)[0];
			w->gridy = ((INTBIG *)var->addr)[1];
		}
		w->state = DISPWINDOW;
		w->frame = getwindowframe(FALSE);
		w->charhandler = 0;
		w->buttonhandler = 0;
		w->changehandler = 0;
		w->termhandler = 0;
		w->redisphandler = 0;
	}
	if (allocstring(&w->location, l, db_cluster))
	{
		db_donextchangequietly = FALSE;
		return(NOWINDOWPART);
	}
	w->editor = NOEDITOR;
	w->expwindow = (void *)-1;
	w->numvar = 0;

	if (db_enterwindowpart(w)) return(NOWINDOWPART);

	/* handle change control and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		(void)db_change((INTBIG)w, OBJECTNEW, VWINDOWPART, 0, 0, 0, 0, 0);
	}
	db_donextchangequietly = FALSE;
	return(w);
}

/*
 * Routine to enter window "w" into the database.  Returns true on error.
 */
BOOLEAN db_enterwindowpart(WINDOWPART *w)
{
	WINDOWFRAME *wf;

	/* make a new frame if requested */
	if ((w->state & HASOWNFRAME) != 0)
	{
		/* create a new frame */
		wf = newwindowframe(FALSE, 0);
		if (wf == NOWINDOWFRAME) wf = getwindowframe(FALSE);
		w->frame = wf;
		if (wf != NOWINDOWFRAME)
		{
			/* recreating old frame: set size */
			sizewindowframe(wf, w->framehx, w->framehy);
		}
		w->state &= ~HASOWNFRAME;
	}

	/* insert window in linked list */
	w->nextwindowpart = el_topwindowpart;
	w->prevwindowpart = NOWINDOWPART;
	if (el_topwindowpart != NOWINDOWPART) el_topwindowpart->prevwindowpart = w;
	el_topwindowpart = w;
	return(FALSE);
}

void computewindowscale(WINDOWPART *w)
{
	float denominator;

	w->scalex = (float)(w->usehx - w->uselx);
	denominator = (float)w->screenhx;
	denominator -= (float)w->screenlx;
	if (denominator != 0.0) w->scalex /= denominator;

	w->scaley = (float)(w->usehy - w->usely);
	denominator = (float)w->screenhy;
	denominator -= (float)w->screenly;
	if (denominator != 0.0) w->scaley /= denominator;
}

INTBIG applyxscale(WINDOWPART *w, INTBIG val)
{
	return(roundfloat((float)val * w->scalex));
}

INTBIG applyyscale(WINDOWPART *w, INTBIG val)
{
	return(roundfloat((float)val * w->scaley));
}

/*
 * routine to kill window "w"
 */
void killwindowpart(WINDOWPART *w)
{
	db_retractwindowpart(w);

	/* handle change control and broadcast */
	if (!db_donextchangequietly && !db_dochangesquietly)
	{
		(void)db_change((INTBIG)w, OBJECTKILL, VWINDOWPART, 0, 0, 0, 0, 0);
	}
	db_donextchangequietly = FALSE;
}

void db_retractwindowpart(WINDOWPART *w)
{
	REGISTER WINDOWPART *ow;

	if (w->termhandler != 0) (*w->termhandler)(w);

	/* remove the window from the linked list */
	if (w->prevwindowpart != NOWINDOWPART)
		w->prevwindowpart->nextwindowpart = w->nextwindowpart;
	if (w->nextwindowpart != NOWINDOWPART)
		w->nextwindowpart->prevwindowpart = w->prevwindowpart;
	if (w == el_topwindowpart) el_topwindowpart = w->nextwindowpart;

	/* see if this window lived by itself in a frame */
	for(ow = el_topwindowpart; ow != NOWINDOWPART; ow = ow->nextwindowpart)
		if (ow->frame == w->frame) break;
	if (ow == NOWINDOWPART)
	{
		/* delete the frame too */
		w->framelx = 0;   w->framely = 0;
		getwindowframesize(w->frame, &w->framehx, &w->framehy);
		w->state |= HASOWNFRAME;
		killwindowframe(w->frame);
	}
}

/*
 * routine to return window "w" to the pool of free windows
 */
void freewindowpart(WINDOWPART *w)
{
	if ((w->state&WINDOWTYPE) == TEXTWINDOW ||
		(w->state&WINDOWTYPE) == POPTEXTWINDOW)
	{
		us_editorterm(w);
	} else if ((w->state&WINDOWTYPE) == EXPLORERWINDOW)
	{
		us_deleteexplorerstruct(w);
	}
	efree(w->location);
	w->nextwindowpart = db_windowpartfree;
	db_windowpartfree = w;
}

/*
 * routine to copy the contents of window "from" to window "to"
 */
void copywindowpart(WINDOWPART *to, WINDOWPART *from)
{
	REGISTER INTBIG i;

	to->uselx = from->uselx;                to->usehx = from->usehx;
	to->usely = from->usely;                to->usehy = from->usehy;
	to->screenlx = from->screenlx;          to->screenhx = from->screenhx;
	to->screenly = from->screenly;          to->screenhy = from->screenhy;
	to->hratio = from->hratio;              to->vratio = from->vratio;
	to->scalex = from->scalex;              to->scaley = from->scaley;
	to->curnodeproto = from->curnodeproto;
	to->topnodeproto = from->topnodeproto;
	to->inplacedepth = from->inplacedepth;
	for(i=0; i<from->inplacedepth; i++) to->inplacestack[i] = from->inplacestack[i];
	transcpy(from->intocell, to->intocell);
	transcpy(from->outofcell, to->outofcell);
	to->gridx = from->gridx;                to->gridy = from->gridy;
	to->state = from->state;
	to->frame = from->frame;
	to->editor = from->editor;
	to->expwindow = from->expwindow;
	to->buttonhandler = from->buttonhandler;
	to->charhandler = from->charhandler;
	to->changehandler = from->changehandler;
	to->termhandler = from->termhandler;
	to->redisphandler = from->redisphandler;
}

/*************************************** INTBIG OBJECTS ***************************************/

LISTINTBIG *newintlistobj(CLUSTER *clus)
{
	REGISTER LISTINTBIG *li;

	li = (LISTINTBIG *)emalloc(sizeof (LISTINTBIG), clus);
	if (li == 0) return(0);
	li->count = li->total = 0;
	li->cluster = clus;
	return(li);
}

void killintlistobj(LISTINTBIG *li)
{
	if (li->total > 0) efree((CHAR *)li->list);
	efree((CHAR *)li);
}

void clearintlistobj(LISTINTBIG *li)
{
	li->count = 0;
}

INTBIG *getintlistobj(LISTINTBIG *li, INTBIG *count)
{
	*count = li->count;
	return(li->list);
}

/*
 * Routine to ensure that the integer list "li" has "needed" entries in it.  Returns TRUE on error.
 */
BOOLEAN ensureintlistobj(LISTINTBIG *li, INTBIG needed)
{
	if (needed > li->total)
	{
		if (li->total > 0) efree((CHAR *)li->list);
		li->total = 0;
		li->list = (INTBIG *)emalloc(needed * SIZEOFINTBIG, li->cluster);
		if (li->list == 0) return(TRUE);
		li->total = needed;
	}
	return(FALSE);
}

/*
 * Routine to add value "i" to the end of the global list "li".  If "unique" is TRUE,
 * does not add duplicate values.  Returns true on error.
 */
BOOLEAN addtointlistobj(LISTINTBIG *li, INTBIG value, BOOLEAN unique)
{
	REGISTER INTBIG newtotal, m, *newqueue;

	/* if a unique list is needed, check for duplications */
	if (unique)
	{
		for(m=0; m<li->count; m++)
			if (li->list[m] == value) return(FALSE);
	}

	if (li->count >= li->total)
	{
		newtotal = li->total * 2;
		if (li->count >= newtotal) newtotal = li->count + 50;
		newqueue = (INTBIG *)emalloc(newtotal * SIZEOFINTBIG, li->cluster);
		if (newqueue == 0) return(TRUE);
		for(m=0; m < li->count; m++) newqueue[m] = li->list[m];
		if (li->total > 0) efree((CHAR *)li->list);
		li->total = newtotal;
		li->list = newqueue;
	}
	li->list[li->count++] = value;
	return(FALSE);
}

/*************************************** STRING OBJECTS ***************************************/

/*
 * Routine to create a new string object from memory cluster "clus".  Returns zero on error.
 */
STRINGOBJ *newstringobj(CLUSTER *clus)
{
	REGISTER STRINGOBJ *so;

	so = (STRINGOBJ *)emalloc(sizeof (STRINGOBJ), clus);
	if (so == 0) return(0);
	so->total = 0;
	so->cluster = clus;
	return(so);
}

/*
 * Routine to delete new string object "so".
 */
void killstringobj(STRINGOBJ *so)
{
	if (so->total > 0) efree((CHAR *)so->string);
	efree((CHAR *)so);
}

/*
 * Routine to clear the string in string object "so".
 */
void clearstringobj(STRINGOBJ *so)
{
	so->count = 0;
}

/*
 * Routine to return the string in string object "so".
 */
CHAR *getstringobj(STRINGOBJ *so)
{
	if (so->count == 0) return(x_(""));
	return(so->string);
}

/*
 * Routine to add "string" to the string in string object "so".
 */
void addstringtostringobj(CHAR *string, STRINGOBJ *so)
{
	REGISTER INTBIG len, newtotal;
	REGISTER CHAR *newstring;

	len = estrlen(string);
	if (so->count + len >= so->total)
	{
		newtotal = so->total * 2;
		if (so->count + len > newtotal) newtotal = so->count + len + 10;
		newstring = (CHAR *)emalloc(newtotal * SIZEOFCHAR, so->cluster);
		if (newstring == 0) return;
		if (so->count > 0) strncpy(newstring, so->string, so->count);
		if (so->total > 0) efree((CHAR *)so->string);
		so->string = newstring;
		so->total = newtotal;
	}
	estrcpy(&so->string[so->count], string);
	so->count += len;
}

/************************* MISCELLANEOUS *************************/

/*
 * routine to return the value of lambda to use for object "geom"
 */
INTBIG figurelambda(GEOM *geom)
{
	if (!geom->entryisnode)
		return(lambdaofarc(geom->entryaddr.ai));
	return(lambdaofnode(geom->entryaddr.ni));
}

/*
 * Routine to determine the proper value of lambda to use, given that
 * the work involves NODEPROTO "np".
 */
INTBIG lambdaofcell(NODEPROTO *np)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *lib;

	tech = np->tech;
	if (np->primindex != 0) return(el_curlib->lambda[tech->techindex]);

	/* cell: figure out the proper technology and library */
	lib = np->lib;
	return(lib->lambda[tech->techindex]);
}

/*
 * Routine to determine the proper value of lambda to use, given that
 * the work involves ARCINST "ai".
 */
INTBIG lambdaofarc(ARCINST *ai)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;

	np = ai->parent;
	if (np == NONODEPROTO) tech = ai->proto->tech; else
	{
		tech = np->tech;
		if (tech == NOTECHNOLOGY) tech = whattech(np);
	}
	if (ai->parent == NONODEPROTO) lib = el_curlib; else
		lib = ai->parent->lib;
	return(lib->lambda[tech->techindex]);
}

/*
 * Routine to determine the proper value of lambda to use, given that
 * the work involves NODEINST "ni".
 */
INTBIG lambdaofnode(NODEINST *ni)
{
	REGISTER TECHNOLOGY *tech;
	REGISTER LIBRARY *lib;
	REGISTER NODEPROTO *np;
	REGISTER INTBIG lambda;

	np = ni->parent;
	if (np == NONODEPROTO) np = ni->proto;
	tech = np->tech;
	if (tech == NOTECHNOLOGY) tech = whattech(np);
	if (ni->parent == NONODEPROTO) lib = el_curlib; else
		lib = ni->parent->lib;
	lambda = lib->lambda[tech->techindex];
	return(lambda);
}

/*
 * routine to determine the technology of a cell.  Gives preference to
 * the layout technologies (i.e. ignores "schematic" and "artwork" unless they
 * are the only elements in the cell).  Someday this code should be
 * generalized to read special codes on the technologies that indicate
 * "layout" versus "symbolic".
 */
#define MAXTECHNOLOGIES 100		/* assumed maximum number of technologies (not hard limit) */

TECHNOLOGY *whattech(NODEPROTO *cell)
{
	REGISTER INTBIG best, bestlayout, usecountneed, i, *usecount, *allocatedusecount;
	REGISTER BOOLEAN foundicons;
	INTBIG staticusecount[MAXTECHNOLOGIES];
	REGISTER NODEPROTO *np, *onp, *cv, *cnp;
	REGISTER NODEINST *ni;
	REGISTER ARCINST *ai;
	REGISTER ARCPROTO *ap;
	REGISTER TECHNOLOGY *tech, *besttech, *bestlayouttech, *rettech;

	/* primitives know their technology */
	if (cell->primindex != 0) return(cell->tech);

	/* ensure proper memory for usage counts */
	usecountneed = 0;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
		if (tech->techindex > usecountneed) usecountneed = tech->techindex;
	usecountneed++;
	if (usecountneed > MAXTECHNOLOGIES)
	{
		allocatedusecount = (INTBIG *)emalloc(usecountneed * SIZEOFINTBIG, db_cluster);
		if (allocatedusecount == 0) return(gen_tech);
		usecount = allocatedusecount;
	} else
	{
		usecount = staticusecount;
		allocatedusecount = 0;
	}

	/* zero counters for every technology */
	for(i=0; i<usecountneed; i++) usecount[i] = 0;

	/* count technologies of all primitive nodes in the cell */
	for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
	{
		np = ni->proto;
		if (np == NONODEPROTO) continue;
		if (np->primindex == 0)
		{
			cnp = contentsview(np);
			if (cnp != NONODEPROTO) np = cnp;
		}
		if (np->tech != NOTECHNOLOGY)
			usecount[np->tech->techindex]++;
	}

	/* count technologies of all arcs in the cell */
	for(ai = cell->firstarcinst; ai != NOARCINST; ai = ai->nextarcinst)
	{
		ap = ai->proto;
		if (ap == NOARCPROTO) continue;
		usecount[ap->tech->techindex]++;
	}

	/* find a concensus */
	best = 0;         besttech = NOTECHNOLOGY;
	bestlayout = 0;   bestlayouttech = NOTECHNOLOGY;
	for(tech = el_technologies; tech != NOTECHNOLOGY; tech = tech->nexttechnology)
	{
		/* always ignore the generic technology */
		if (tech == gen_tech) continue;

		/* find the most popular of ALL technologies */
		if (usecount[tech->techindex] > best)
		{
			best = usecount[tech->techindex];
			besttech = tech;
		}

		/* find the most popular of the layout technologies */
		if (tech == sch_tech || tech == art_tech) continue;
		if (usecount[tech->techindex] > bestlayout)
		{
			bestlayout = usecount[tech->techindex];
			bestlayouttech = tech;
		}
	}

	/* presume generic */
	if (cell->cellview == el_iconview)
	{
		/* in icons, if there is any artwork, use it */
		if (usecount[art_tech->techindex] > 0) return(art_tech);

		/* in icons, if there is nothing, presume artwork */
		if (besttech == NOTECHNOLOGY) return(art_tech);

		/* use artwork as a default */
		rettech = art_tech;
	} else if (cell->cellview == el_schematicview)
	{
		/* in schematic, if there are any schematic components, use it */
		if (usecount[sch_tech->techindex] > 0) return(sch_tech);

		/* in schematic, if there is nothing, presume schematic */
		if (besttech == NOTECHNOLOGY) return(sch_tech);

		/* use schematic as a default */
		rettech = sch_tech;
	} else
	{
		/* use the current layout technology as the default */
		rettech = el_curlayouttech;
	}

	/* if a layout technology was voted the most, return it */
	if (bestlayouttech != NOTECHNOLOGY) rettech = bestlayouttech; else
	{
		/* if any technology was voted the most, return it */
		if (besttech != NOTECHNOLOGY) rettech = besttech; else
		{
			/* if this is an icon, presume the technology of its contents */
			cv = contentsview(cell);
			if (cv != NONODEPROTO)
			{
				if (cv->tech == NOTECHNOLOGY)
					cv->tech = whattech(cv);
				rettech = cv->tech;
			} else
			{
				/* look at the contents of the sub-cells */
				foundicons = FALSE;
				for(ni = cell->firstnodeinst; ni != NONODEINST; ni = ni->nextnodeinst)
				{
					np = ni->proto;
					if (np == NONODEPROTO) continue;
					if (np->primindex != 0) continue;

					/* ignore recursive references (showing icon in contents) */
					if (isiconof(np, cell)) continue;

					/* see if the cell has an icon */
					if (np->cellview == el_iconview) foundicons = TRUE;

					/* do not follow into another library */
					if (np->lib != cell->lib) continue;
					onp = contentsview(np);
					if (onp != NONODEPROTO) np = onp;
					tech = whattech(np);
					if (tech == gen_tech) continue;
					rettech = tech;
					break;
				}
				if (ni == NONODEINST)
				{
					/* could not find instances that give information: were there icons? */
					if (foundicons) rettech = sch_tech;
				}
			}
		}
	}
	if (allocatedusecount != 0) efree((CHAR *)allocatedusecount);

	/* give up and report the generic technology */
	return(rettech);
}

/*
 * routine to return the parent node prototype of geometry object "g"
 */
NODEPROTO *geomparent(GEOM *g)
{
	if (g == NOGEOM) return(NONODEPROTO);
	if (g->entryisnode) return(g->entryaddr.ni->parent);
	return(g->entryaddr.ai->parent);
}

/*
 * routine to recursively determine whether the nodeproto "parent" is
 * a descendant of nodeproto "child" (which would make the relationship
 * "child insideof parent" recursive).  If the routine returns zero,
 * the parent is not a descendent of the child and all is well.  If
 * true returned, recursion exists.
 */
BOOLEAN isachildof(NODEPROTO *parent, NODEPROTO *child)
{
	REGISTER NODEPROTO *np;

	/* if the child is primitive there is no recursion */
	if (child->primindex != 0) return(FALSE);

	/* special case: allow an icon to be inside of the contents for illustration */
	if (isiconof(child, parent))
	{
		if (child->cellview == el_iconview && parent->cellview != el_iconview)
			return(FALSE);
	}

	/* make sure the child is not an icon */
	np = contentsview(child);
	if (np != NONODEPROTO) child = np;

	return(db_isachildof(parent, child));
}

BOOLEAN db_isachildof(NODEPROTO *parent, NODEPROTO *child)
{
	REGISTER NODEINST *ni;
	REGISTER NODEPROTO *np;

	/* if the child is primitive there is no recursion */
	if (child->primindex != 0) return(FALSE);

	/* if they are the same, that is recursion */
	if (child == parent) return(TRUE);

	/* look through every instance of the parent cell */
	for(ni = parent->firstinst; ni != NONODEINST; ni = ni->nextinst)
	{
		/* if two instances in a row have same parent, skip this one */
		if (ni->nextinst != NONODEINST && ni->nextinst->parent == ni->parent)
			continue;

		/* recurse to see if the grandparent belongs to the child */
		if (db_isachildof(ni->parent, child)) return(TRUE);
	}

	/* if this has an icon, look at it's instances */
	np = iconview(parent);
	if (np != NONODEPROTO)
	{
		for(ni = np->firstinst; ni != NONODEINST; ni = ni->nextinst)
		{
			/* if two instances in a row have same parent, skip this one */
			if (ni->nextinst != NONODEINST && ni->nextinst->parent == ni->parent)
				continue;

			/* special case: allow an icon to be inside of the contents for illustration */
			if (isiconof(ni->proto, parent))
			{
				if (parent->cellview != el_iconview) continue;
			}

			/* recurse to see if the grandparent belongs to the child */
			if (db_isachildof(ni->parent, child)) return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * routine to tell the nodeproto that connects arcs of type "ap"
 */
NODEPROTO *getpinproto(ARCPROTO *ap)
{
	REGISTER NODEPROTO *np;
	REGISTER PORTPROTO *pp;
	REGISTER INTBIG i;
	static INTBIG defaultpinkey = 0;
	REGISTER VARIABLE *var;

	if (ap == NOARCPROTO) return(NONODEPROTO);

	/* see if there is a default on this arc proto */
	if (defaultpinkey == 0) defaultpinkey = makekey(x_("ARC_Default_Pin"));
	var = getvalkey((INTBIG)ap, VARCPROTO, VNODEPROTO, defaultpinkey);
	if (var != NOVARIABLE)
	{
		/* default found: test it for validity */
		np = (NODEPROTO *)var->addr;
		pp = np->firstportproto;
		for(i=0; pp->connects[i] != NOARCPROTO; i++)
			if (pp->connects[i] == ap) return(np);
	}

	/* first look for nodeprotos that are actually declared as pins */
	for(np = ap->tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		if (((np->userbits & NFUNCTION) >> NFUNCTIONSH) != NPPIN) continue;
		pp = np->firstportproto;
		for(i=0; pp->connects[i] != NOARCPROTO; i++)
			if (pp->connects[i] == ap) return(np);
	}

	/* now look for a nodeproto that can connect to this type of arc */
	for(np = ap->tech->firstnodeproto; np != NONODEPROTO; np = np->nextnodeproto)
	{
		pp = np->firstportproto;
		if (pp == NOPORTPROTO) continue;
		if (pp->nextportproto != NOPORTPROTO) continue;
		for(i=0; pp->connects[i] != NOARCPROTO; i++)
			if (pp->connects[i] == ap) return(np);
	}
	ttyputerr(_("No node to connect %s arcs"), describearcproto(ap));
	return(NONODEPROTO);
}

/*
 * Routine to return TRUE if node "ni" is a pin that is between
 * two arcs such that it can be deleted without affecting the geometry of the two arcs.
 */
BOOLEAN isinlinepin(NODEINST *ni, ARCINST ***thearcs)
{
	static ARCINST *reconar[2];
	INTBIG dx[2], dy[2];
	REGISTER INTBIG i, j;
	REGISTER ARCINST *ai;
	REGISTER PORTARCINST *pi;
	REGISTER VARIABLE *var0, *var1;

	if ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH != NPPIN) return(FALSE);

	/* see if the pin is connected to two arcs along the same slope */
	j = 0;
	for(pi = ni->firstportarcinst; pi != NOPORTARCINST; pi = pi->nextportarcinst)
	{
		if (j >= 2) { j = 0;   break; }
		reconar[j] = ai = pi->conarcinst;
		for(i=0; i<2; i++) if (ai->end[i].nodeinst != ni)
		{
			dx[j] = ai->end[i].xpos - ai->end[1-i].xpos;
			dy[j] = ai->end[i].ypos - ai->end[1-i].ypos;
		}
		j++;
	}
	if (j != 2) return(FALSE);

	/* must connect to two arcs of the same type and width */
	if (reconar[0]->proto != reconar[1]->proto) return(FALSE);
	if (reconar[0]->width != reconar[1]->width) return(FALSE);

	/* arcs must be along the same angle, and not be curved */
	if (dx[0] != 0 || dy[0] != 0 || dx[1] != 0 || dy[1] != 0)
	{
		if ((dx[0] != 0 || dy[0] != 0) && (dx[1] != 0 || dy[1] != 0) &&
			figureangle(0, 0, dx[0], dy[0]) !=
			figureangle(dx[1], dy[1], 0, 0)) return(FALSE);
	}
	if (getvalkey((INTBIG)reconar[0], VARCINST, VINTEGER, el_arc_radius_key) != NOVARIABLE)
		return(FALSE);
	if (getvalkey((INTBIG)reconar[1], VARCINST, VINTEGER, el_arc_radius_key) != NOVARIABLE)
		return(FALSE);

	/* the arcs must not have network names on them */
	var0 = getvalkey((INTBIG)reconar[0], VARCINST, VSTRING, el_arc_name_key);
	var1 = getvalkey((INTBIG)reconar[1], VARCINST, VSTRING, el_arc_name_key);
	if (var0 != NOVARIABLE && var1 != NOVARIABLE)
	{
		if ((var0->type&VDISPLAY) != 0 && (var1->type&VDISPLAY) != 0)
			return(FALSE);
	}
	thearcs = (ARCINST ***)reconar;
	return(TRUE);
}

/*
 * Routine to return TRUE if node "ni" is an invisible pin that has text in a different
 * location.  If so, (x,y) is set to the location of that text.
 */
BOOLEAN invisiblepinwithoffsettext(NODEINST *ni, INTBIG *x, INTBIG *y, BOOLEAN repair)
{
	REGISTER INTBIG i, j, lambda;
	REGISTER PORTEXPINST *pe;
	REGISTER PORTPROTO *pp;
	REGISTER VARIABLE *var;
	static POLYGON *poly = NOPOLYGON;

	/* get polygon */
	(void)needstaticpolygon(&poly, 4, us_tool->cluster);

	/* look for pins that are invisible and have text in different location */
	if ((ni->proto->userbits&NFUNCTION) >> NFUNCTIONSH != NPPIN) return(FALSE);
	if (ni->firstportarcinst != NOPORTARCINST) return(FALSE);

	/* stop now if this isn't invisible */
	if (ni->proto != gen_invispinprim)
	{
		j = nodepolys(ni, 0, NOWINDOWPART);
		if (j > 0)
		{
			shapenodepoly(ni, 0, poly);
			if (poly->style != TEXTCENT && poly->style != TEXTTOP &&
				poly->style != TEXTBOT && poly->style != TEXTLEFT &&
				poly->style != TEXTRIGHT && poly->style != TEXTTOPLEFT &&
				poly->style != TEXTBOTLEFT && poly->style != TEXTTOPRIGHT &&
				poly->style != TEXTBOTRIGHT && poly->style != TEXTBOX) return(FALSE);
		}
	}

	/* invisible: look for offset text */
	for(pe = ni->firstportexpinst; pe != NOPORTEXPINST; pe = pe->nextportexpinst)
	{
		pp = pe->exportproto;
		if (TDGETXOFF(pp->textdescript) != 0 || TDGETYOFF(pp->textdescript) != 0)
		{
			lambda = lambdaofnode(ni);
			*x = (ni->lowx + ni->highx) / 2 + muldiv(TDGETXOFF(pp->textdescript), lambda, 4);
			*y = (ni->lowy + ni->highy) / 2 + muldiv(TDGETYOFF(pp->textdescript), lambda, 4);
			if (repair)
			{
				(void)startobjectchange((INTBIG)ni, VNODEINST);
				TDSETOFF(pp->textdescript, 0, 0);
				(void)setind((INTBIG)pp, VPORTPROTO, x_("textdescript"), 0, pp->textdescript[0]);
				(void)setind((INTBIG)pp, VPORTPROTO, x_("textdescript"), 1, pp->textdescript[1]);
				(void)endobjectchange((INTBIG)ni, VNODEINST);
			}
			return(TRUE);
		}
	}
	for(i=0; i<ni->numvar; i++)
	{
		var = &ni->firstvar[i];
		if ((var->type&VDISPLAY) != 0 &&
			(TDGETXOFF(var->textdescript) != 0 || TDGETYOFF(var->textdescript) != 0))
		{
			lambda = lambdaofnode(ni);
			*x = (ni->lowx + ni->highx) / 2 + muldiv(TDGETXOFF(var->textdescript), lambda, 4);
			*y = (ni->lowy + ni->highy) / 2 + muldiv(TDGETYOFF(var->textdescript), lambda, 4);
			if (repair)
			{
				(void)startobjectchange((INTBIG)ni, VNODEINST);
				TDSETOFF(var->textdescript, 0, 0);
				modifydescript((INTBIG)ni, VNODEINST, var, var->textdescript);
				(void)endobjectchange((INTBIG)ni, VNODEINST);
			}
			return(TRUE);
		}
	}
	return(FALSE);
}

/*
 * routine to return the variable with "trace" information on nodeinst "ni".
 * Returns NOVARIABLE if there is no trace information
 */
VARIABLE *gettrace(NODEINST *ni)
{
	REGISTER VARIABLE *var;

	/* now look for the variable */
	var = getvalkey((INTBIG)ni, VNODEINST, VINTEGER|VISARRAY, el_trace_key);
	if (var == NOVARIABLE) return(NOVARIABLE);

	/* return the trace variable */
	return(var);
}

/*
 * routine to return the "grab point" of cell "np" and place it in the
 * reference integers (grabx, graby).  The default is the lower-left corner
 */
void grabpoint(NODEPROTO *np, INTBIG *grabx, INTBIG *graby)
{
	REGISTER VARIABLE *var;

	*grabx = np->lowx;
	*graby = np->lowy;
	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
	if (var != NOVARIABLE)
	{
		*grabx = ((INTBIG *)var->addr)[0];
		*graby = ((INTBIG *)var->addr)[1];
	}
}

/*
 * routine to determine the offset of the lower-left corner of nodeinst "ni"
 * (which may be NONODEINST), nodeproto "np" given that it is rotated "rot"
 * and transposed "trn".  The offset from the actual corner to the effective
 * corner is placed in "x" and "y".
 */
void corneroffset(NODEINST *ni, NODEPROTO *np, INTBIG rot, INTBIG trn, INTBIG *x, INTBIG *y,
	BOOLEAN centerbased)
{
	INTBIG lx, ly, hx, hy, tx, ty, bx, by, pxs, pys;
	XARRAY mat;
	REGISTER INTBIG px, py, cx, cy;
	REGISTER VARIABLE *var;

	/* build transformation matrix */
	makeangle(rot, trn, mat);

	/* see if there is a real grab-point */
	var = getvalkey((INTBIG)np, VNODEPROTO, VINTEGER|VISARRAY, el_prototype_center_key);
	if (var != NOVARIABLE)
	{
		/* use grab point if specified */
		cx = (np->lowx + np->highx) / 2;
		cy = (np->lowy + np->highy) / 2;
		px = np->lowx - cx;   py = np->lowy - cy;
		xform(((INTBIG *)var->addr)[0]-cx, ((INTBIG *)var->addr)[1]-cy, &bx, &by, mat);
	} else
	{
		/* no grab point: find corner that is lowest and most to the left */
		if (ni == NONODEINST)
		{
			nodeprotosizeoffset(np, &lx, &ly, &hx, &hy, NONODEPROTO);
			if (np->primindex != 0)
			{
				defaultnodesize(np, &pxs, &pys);
				cx = cy = 0;
				px = -pxs/2;   py = -pys/2;
				lx = lx - pxs/2;   hx = pxs/2 - hx;
				ly = ly - pys/2;   hy = pys/2 - hy;
			} else
			{
				cx = (np->lowx + np->highx) / 2;
				cy = (np->lowy + np->highy) / 2;
				px = np->lowx - cx;   py = np->lowy - cy;
				lx = lx + np->lowx - cx;   hx = np->highx - hx - cx;
				ly = ly + np->lowy - cy;   hy = np->highy - hy - cy;
			}
		} else
		{
			nodesizeoffset(ni, &lx, &ly, &hx, &hy);
			cx = (ni->lowx + ni->highx) / 2;
			cy = (ni->lowy + ni->highy) / 2;
			px = ni->lowx - cx;   py = ni->lowy - cy;
			lx = lx + ni->lowx - cx;   hx = ni->highx - hx - cx;
			ly = ly + ni->lowy - cy;   hy = ni->highy - hy - cy;
		}
		xform(lx, ly, &tx, &ty, mat);
		bx = tx;   by = ty;
		xform(lx, hy, &tx, &ty, mat);
		if ((tx-bx) + (ty-by) < 0) { bx = tx;   by = ty; }
		xform(hx, hy, &tx, &ty, mat);
		if ((tx-bx) + (ty-by) < 0) { bx = tx;   by = ty; }
		xform(hx, ly, &tx, &ty, mat);
		if ((tx-bx) + (ty-by) < 0) { bx = tx;   by = ty; }
		if (centerbased && np->primindex != 0)
			bx = by = 0;
	}

	/* now compute offset between lower-left corner and this point */
	*x = bx - px;   *y = by - py;
}

#ifdef HAVE_QSORT

/*
 * Routine to sort "entries" objects in "data" that are "structsize" bytes large.
 * The routine "numericorder" is called with two elements and returns their ordering.
 */
void esort(void *data, INTBIG entries, INTBIG structsize,
	int (*numericorder)(const void *e1, const void *e2))
{
	/* sort the data */
	qsort(data, entries, structsize, numericorder);
}

#else		/* for systems that don't have "qsort" */
void db_sortquick(int start, int endv, int len, int structsize, void *data,
	int (*numericorder)(const void *e1, const void *e2));

void esort(void *data, INTBIG entries, INTBIG structsize,
	int (*numericorder)(const void *e1, const void *e2))
{
	db_sortquick(0, entries-1, entries-1, structsize, data, numericorder);
}

/* Helper routine for "qsort" to order the list */
void db_sortquick(int start, int endv, int len, int structsize, void *data,
	int (*numericorder)(const void *e1, const void *e2))
{
	REGISTER int endv1, i, j, inorder, mright, mleft;
	REGISTER UCHAR1 *d1, *d2, *dr, swap, *chrdata;

	chrdata = (UCHAR1 *)data;
	if (endv-start > 9)
	{
		/* do a quicksort */
		mright = endv+1;
		mleft = start;
		dr = &chrdata[start*structsize];
		while (mleft < mright)
		{
			mleft = mleft+1;
			for(;;)
			{
				if (mleft >= len) break;
				d2 = &chrdata[mleft*structsize];
				if (numericorder(d2, dr) >= 0) break;
				mleft++;
			}
			mright--;
			for(;;)
			{
				if (mright <= 0) break;
				d2 = &chrdata[mright*structsize];
				if (numericorder(d2, dr) <= 0) break;
				mright--;
			}
			if (mleft < mright)
			{
				d1 = &chrdata[mleft*structsize];
				d2 = &chrdata[mright*structsize];
				for(j=0; j<structsize; j++)
				{
					swap = d1[j];   d1[j] = d2[j];   d2[j] = swap;
				}
			}
		}
		d1 = &chrdata[start*structsize];
		d2 = &chrdata[mright*structsize];
		for(j=0; j<structsize; j++)
		{
			swap = d1[j];   d1[j] = d2[j];   d2[j] = swap;
		}
		db_sortquick(start, mright-1, len, structsize, data, numericorder);
		db_sortquick(mleft, endv, len, structsize, data, numericorder);
	} else
	{
		/* do a bubble sort */
		inorder = 0;
		while (inorder == 0)
		{
			inorder = 1;
			endv1 = endv-1;
			for(i = start; i <= endv1; i++)
			{
				d1 = &chrdata[i*structsize];
				d2 = &chrdata[(i+1)*structsize];
				if (numericorder(d1, d2) <= 0) continue;
				for(j=0; j<structsize; j++)
				{
					swap = d1[j];   d1[j] = d2[j];   d2[j] = swap;
				}
				inorder = 0;
			}
		}
	}
}
#endif

/*
 * Helper routine for "esort" that makes big integers go in ascending order.
 */
int sort_intbigascending(const void *e1, const void *e2)
{
	REGISTER INTBIG c1, c2;

	c1 = *((INTBIG *)e1);
	c2 = *((INTBIG *)e2);
	return(c1 - c2);
}

/*
 * Helper routine for "esort" that makes big integers go in ascending order.
 */
int sort_intbigdescending(const void *e1, const void *e2)
{
	REGISTER INTBIG c1, c2;

	c1 = *((INTBIG *)e1);
	c2 = *((INTBIG *)e2);
	return(c2 - c1);
}

/*
 * Helper routine for "esort" that makes strings go in ascending order.
 */
int sort_stringascending(const void *e1, const void *e2)
{
	REGISTER CHAR *c1, *c2;

	c1 = *((CHAR **)e1);
	c2 = *((CHAR **)e2);
	return(namesame(c1, c2));
}

/*
 * Helper routine for "esort" that makes cells go in ascending name order.
 */
int sort_cellnameascending(const void *e1, const void *e2)
{
	REGISTER NODEPROTO *c1, *c2;

	c1 = *((NODEPROTO **)e1);
	c2 = *((NODEPROTO **)e2);
	return(namesame(nldescribenodeproto(c1), nldescribenodeproto(c2)));
}

/*
 * Helper routine for "esort" that makes exports go in ascending name order.
 */
int sort_exportnameascending(const void *e1, const void *e2)
{
	REGISTER PORTPROTO *c1, *c2;

	c1 = *((PORTPROTO **)e1);
	c2 = *((PORTPROTO **)e2);
	return(namesamenumeric(c1->protoname, c2->protoname));
}

/*
 * Helper routine for "esort" that makes exports go in descending name order.
 */
int sort_exportnamedescending(const void *e1, const void *e2)
{
	REGISTER PORTPROTO *c1, *c2;

	c1 = *((PORTPROTO **)e1);
	c2 = *((PORTPROTO **)e2);
	return(namesamenumeric(c2->protoname, c1->protoname));
}

/*
 * routine to initialize the database
 */
void db_initdatabase(void)
{
	REGISTER INTBIG x, y, i, j, k;
	INTBIG matc[2][2];

	/* create interlocks */
	if (ensurevalidmutex(&db_polygonmutex, TRUE)) return;
	if (ensurevalidmutex(&db_primenumbermutex, TRUE)) return;

	/* initialize change control */
	db_initializechanges();

	/* miscellaneous initialization */
	db_windowpartfree = NOWINDOWPART;
	db_lasterror = DBNOERROR;
	el_curwindowpart = NOWINDOWPART;
	el_units = DISPUNITLAMBDA | INTUNITHNM;
	db_printerrors = TRUE;
	db_mrgdatainit();

	/* setup disk file descriptors */
	el_filetypetext = setupfiletype(x_(""), x_("*.*"), MACFSTAG('TEXT'), FALSE, x_("text"), _("Text"));

	/* prepare language translation */
	db_inittranslation();

	/* transformation initialization */
	for(x=0; x<8; x++) for(y=0; y<8; y++)
	{
		for(i=0; i<2; i++) for(j=0; j<2; j++)
			for(k=0, matc[i][j]=0; k<2; k++)
				matc[i][j] += mult(db_matstyle[x][i][k], db_matstyle[y][k][j]);
		for(k=0; k<8; k++)
		{
			if (matc[0][0] != db_matstyle[k][0][0] ||
				matc[0][1] != db_matstyle[k][0][1] ||
				matc[1][0] != db_matstyle[k][1][0] ||
				matc[1][1] != db_matstyle[k][1][1]) continue;
			db_matmult[x][y] = k;
			break;
		}
	}

	/* initialize variables */
	el_numnames = 0;

	/* initialize globals that key to variables */
	el_node_name_key = makekey(x_("NODE_name"));
	el_arc_name_key = makekey(x_("ARC_name"));
	el_arc_radius_key = makekey(x_("ARC_radius"));
	el_trace_key = makekey(x_("trace"));
	el_cell_message_key = makekey(x_("FACET_message"));
	el_schematic_page_size_key = makekey(x_("FACET_schematic_page_size"));
	el_transistor_width_key = makekey(x_("transistor_width"));
	el_prototype_center_key = makekey(x_("prototype_center"));
	el_essential_bounds_key = makekey(x_("FACET_essentialbounds"));
	el_node_size_default_key = makekey(x_("NODE_size_default"));
	el_arc_width_default_key = makekey(x_("ARC_width_default"));
	el_attrkey_area = makekey(x_("ATTR_area"));
	el_attrkey_length = makekey(x_("ATTR_length"));
	el_attrkey_width = makekey(x_("ATTR_width"));
	el_attrkey_M = makekey(x_("ATTR_M"));
	el_techstate_key = makekey(x_("TECH_state"));
	db_tech_node_width_offset_key = makekey(x_("TECH_node_width_offset"));
	db_tech_layer_function_key = makekey(x_("TECH_layer_function"));
	db_tech_layer_names_key = makekey(x_("TECH_layer_names"));
	db_tech_arc_width_offset_key = makekey(x_("TECH_arc_width_offset"));

	/* DRC keys need to be created early since they're used in "dbtech.c" */
	dr_max_distanceskey               = makekey(x_("DRC_max_distances"));
	dr_wide_limitkey                  = makekey(x_("DRC_wide_limit"));
	dr_min_widthkey                   = makekey(x_("DRC_min_width"));
	dr_min_width_rulekey              = makekey(x_("DRC_min_width_rule"));
	dr_min_node_sizekey               = makekey(x_("DRC_min_node_size"));
	dr_min_node_size_rulekey          = makekey(x_("DRC_min_node_size_rule"));
	dr_connected_distanceskey         = makekey(x_("DRC_min_connected_distances"));
	dr_connected_distances_rulekey    = makekey(x_("DRC_min_connected_distances_rule"));
	dr_unconnected_distanceskey       = makekey(x_("DRC_min_unconnected_distances"));
	dr_unconnected_distances_rulekey  = makekey(x_("DRC_min_unconnected_distances_rule"));
	dr_connected_distancesWkey        = makekey(x_("DRC_min_connected_distances_wide"));
	dr_connected_distancesW_rulekey   = makekey(x_("DRC_min_connected_distances_wide_rule"));
	dr_unconnected_distancesWkey      = makekey(x_("DRC_min_unconnected_distances_wide"));
	dr_unconnected_distancesW_rulekey = makekey(x_("DRC_min_unconnected_distances_wide_rule"));
	dr_connected_distancesMkey        = makekey(x_("DRC_min_connected_distances_multi"));
	dr_connected_distancesM_rulekey   = makekey(x_("DRC_min_connected_distances_multi_rule"));
	dr_unconnected_distancesMkey      = makekey(x_("DRC_min_unconnected_distances_multi"));
	dr_unconnected_distancesM_rulekey = makekey(x_("DRC_min_unconnected_distances_multi_rule"));
	dr_edge_distanceskey              = makekey(x_("DRC_min_edge_distances"));
	dr_edge_distances_rulekey         = makekey(x_("DRC_min_edge_distances_rule"));
#ifdef SURROUNDRULES
	dr_surround_layer_pairskey        = makekey(x_("DRC_surround_layer_pairs"));
	dr_surround_distanceskey          = makekey(x_("DRC_surround_distances"));
	dr_surround_rulekey               = makekey(x_("DRC_surround_rule"));
#endif

	/* create identity matrix */
	transid(el_matid);

	/* initialize the views */
	el_views = NOVIEW;
	el_unknownview = db_makepermanentview(x_("unknown"), x_(""), 0);
	el_simsnapview = db_makepermanentview(x_("simulation-snapshot"), x_("sim"), 0);
	el_netlistnetlispview = db_makepermanentview(x_("netlist-netlisp-format"), x_("net-netlisp"), TEXTVIEW);
	el_netlistrsimview = db_makepermanentview(x_("netlist-rsim-format"), x_("net-rsim"), TEXTVIEW);
	el_netlistsilosview = db_makepermanentview(x_("netlist-silos-format"), x_("net-silos"), TEXTVIEW);
	el_netlistquiscview = db_makepermanentview(x_("netlist-quisc-format"), x_("net-quisc"), TEXTVIEW);
	el_netlistalsview = db_makepermanentview(x_("netlist-als-format"), x_("net-als"), TEXTVIEW);
	el_netlistview = db_makepermanentview(x_("netlist"), x_("net"), TEXTVIEW);
	el_vhdlview = db_makepermanentview(x_("VHDL"), x_("vhdl"), TEXTVIEW);
	el_verilogview = db_makepermanentview(x_("Verilog"), x_("ver"), TEXTVIEW);
	el_skeletonview = db_makepermanentview(x_("skeleton"), x_("sk"), 0);
	el_compview = db_makepermanentview(x_("compensated"), x_("comp"), 0);
	el_docview = db_makepermanentview(x_("documentation"), x_("doc"), TEXTVIEW);
	el_iconview = db_makepermanentview(x_("icon"), x_("ic"), 0);
	el_schematicview = db_makepermanentview(x_("schematic"), x_("sch"), 0);
	el_layoutview = db_makepermanentview(x_("layout"), x_("lay"), 0);
}

/*
 * routine to build one of the permanent views by allocating it, giving it the full
 * name "fname" and abbreviation "sname", and including "state" in its "viewstate".
 */
VIEW *db_makepermanentview(CHAR *fname, CHAR *sname, INTBIG state)
{
	REGISTER VIEW *v;

	/* create the view */
	v = allocview();

	/* name it and assign state */
	(void)allocstring(&v->viewname, fname, db_cluster);
	(void)allocstring(&v->sviewname, sname, db_cluster);
	v->viewstate = state | PERMANENTVIEW;

	/* link into the global list */
	v->nextview = el_views;
	el_views = v;

	/* return the view */
	return(v);
}
