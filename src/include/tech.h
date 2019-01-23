/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tech.h
 * Technology header file
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

/******************** DIMENSIONS ********************/

/* quarter unit fractions */
#define XX	  -WHOLE		/*  NULL */
#define K0	  0				/*  0.0  */
#define Q0    (WHOLE/4)		/*  0.25 */
#define H0    (WHOLE/2)		/*  0.5  */
#define T0    (H0+Q0)		/*  0.75 */
#define K1    (WHOLE)		/*  1.0  */
#define Q1    (K1+Q0)		/*  1.25 */
#define H1    (K1+H0)		/*  1.5  */
#define T1    (K1+T0)		/*  1.75 */
#define K2    (WHOLE*2)		/*  2.0  */
#define Q2    (K2+Q0)		/*  2.25 */
#define H2    (K2+H0)		/*  2.5  */
#define T2    (K2+T0)		/*  2.75 */
#define K3    (WHOLE*3)		/*  3.0  */
#define Q3    (K3+Q0)		/*  3.25 */
#define H3    (K3+H0)		/*  3.5  */
#define T3    (K3+T0)		/*  3.75 */
#define K4    (WHOLE*4)		/*  4.0  */
#define Q4    (K4+Q0)		/*  4.25 */
#define H4    (K4+H0)		/*  4.5  */
#define T4    (K4+T0)		/*  4.75 */
#define K5    (WHOLE*5)		/*  5.0  */
#define Q5    (K5+Q0)		/*  5.25 */
#define H5    (K5+H0)		/*  5.5  */
#define T5    (K5+T0)		/*  5.75 */
#define K6    (WHOLE*6)		/*  6.0  */
#define Q6    (K6+Q0)		/*  6.25 */
#define H6    (K6+H0)		/*  6.5  */
#define T6    (K6+T0)		/*  6.75 */
#define K7    (WHOLE*7)		/*  7.0  */
#define Q7    (K7+Q0)		/*  7.25 */
#define H7    (K7+H0)		/*  7.5  */
#define T7    (K7+T0)		/*  7.75 */
#define K8    (WHOLE*8)		/*  8.0  */
#define Q8    (K8+Q0)		/*  8.25 */
#define H8    (K8+H0)		/*  8.5  */
#define T8    (K8+T0)		/*  8.75 */
#define K9    (WHOLE*9)		/*  9.0  */
#define Q9    (K9+Q0)		/*  9.25 */
#define H9    (K9+H0)		/*  9.5  */
#define T9    (K9+T0)		/*  9.75 */
#define K10   (WHOLE*10)	/* 10.0  */
#define H10   (K10+H0)		/* 10.5  */
#define K11   (WHOLE*11)	/* 11.0  */
#define H11   (K11+H0)		/* 11.5  */
#define K12   (WHOLE*12)	/* 12.0  */
#define H12   (K12+H0)		/* 12.5  */
#define K13   (WHOLE*13)	/* 13.0  */
#define H13   (K13+H0)		/* 13.5  */
#define K14   (WHOLE*14)	/* 14.0  */
#define H14   (K14+H0)		/* 14.5  */
#define K15   (WHOLE*15)	/* 15.0  */
#define H15   (K15+H0)		/* 15.5  */
#define K16   (WHOLE*16)	/* 16.0  */
#define H16   (K16+H0)		/* 16.5  */
#define K17   (WHOLE*17)	/* 17.0  */
#define H17   (K17+H0)		/* 17.5  */
#define K18   (WHOLE*18)	/* 18.0  */
#define H18   (K18+H0)		/* 18.5  */
#define K19   (WHOLE*19)	/* 19.0  */
#define H19   (K19+H0)		/* 19.5  */
#define K20   (WHOLE*20)	/* 20.0  */
#define H20   (K20+H0)		/* 20.5  */
#define K21   (WHOLE*21)	/* 21.0  */
#define H21   (K21+H0)		/* 21.5  */
#define K22   (WHOLE*22)	/* 22.0  */
#define H22   (K22+H0)		/* 22.5  */
#define K23   (WHOLE*23)	/* 23.0  */
#define H23   (K23+H0)		/* 23.5  */
#define K24   (WHOLE*24)	/* 24.0  */
#define H24   (K24+H0)		/* 24.5  */
#define K25   (WHOLE*25)	/* 25.0  */
#define H25   (K25+H0)		/* 25.5  */
#define K26   (WHOLE*26)	/* 26.0  */
#define H26   (K26+H0)		/* 26.5  */
#define K27   (WHOLE*27)	/* 27.0  */
#define H27   (K27+H0)		/* 27.5  */
#define K28   (WHOLE*28)	/* 28.0  */
#define H28   (K28+H0)		/* 28.5  */
#define K29   (WHOLE*29)	/* 29.0  */
#define K30   (WHOLE*30)	/* 30.0  */
#define K31   (WHOLE*31)	/* 31.0  */
#define K32   (WHOLE*32)	/* 32.0  */
#define K35   (WHOLE*35)	/* 35.0  */
#define K37   (WHOLE*37)	/* 37.0  */
#define K38   (WHOLE*38)	/* 38.0  */
#define K39   (WHOLE*39)	/* 39.0  */
#define K44   (WHOLE*44)	/* 44.0  */
#define K55   (WHOLE*55)	/* 55.0  */
#define K57   (WHOLE*57)	/* 57.0  */
#define K135  (WHOLE*135)	/* 135.0 */

/******************** DIMENSIONS CONVERTED TO EDGES ********************/

#define CENTER      0,  0

/* right of center by this amount */
#define CENTERR0H   0, H0	/* 0.5 */
#define CENTERR1    0, K1	/* 1.0 */
#define CENTERR1H   0, H1	/* 1.5 */
#define CENTERR2    0, K2	/* 2.0 */
#define CENTERR2H   0, H2	/* 2.5 */
#define CENTERR3    0, K3	/* 3.0 */
#define CENTERR3H   0, H3	/* 3.5 */
#define CENTERR4    0, K4	/* 4.0 */
#define CENTERR4H   0, H4	/* 4.5 */
#define CENTERR5    0, K5	/* 5.0 */
#define CENTERR5H   0, H5	/* 5.5 */

/* up from center by this amount */
#define CENTERU0H   0, H0	/* 0.5 */
#define CENTERU1    0, K1	/* 1.0 */
#define CENTERU1H   0, H1	/* 1.5 */
#define CENTERU2    0, K2	/* 2.0 */
#define CENTERU2H   0, H2	/* 2.5 */
#define CENTERU3    0, K3	/* 3.0 */
#define CENTERU3H   0, H3	/* 3.5 */
#define CENTERU4    0, K4	/* 4.0 */
#define CENTERU4H   0, H4	/* 4.5 */
#define CENTERU5    0, K5	/* 5.0 */
#define CENTERU5H   0, H5	/* 5.5 */

/* left of center by this amount */
#define CENTERL0H   0,-H0	/* 0.5 */
#define CENTERL1    0,-K1	/* 1.0 */
#define CENTERL1H   0,-H1	/* 1.5 */
#define CENTERL2    0,-K2	/* 2.0 */
#define CENTERL2H   0,-H2	/* 2.5 */
#define CENTERL3    0,-K3	/* 3.0 */
#define CENTERL3H   0,-H3	/* 3.5 */
#define CENTERL4    0,-K4	/* 4.0 */
#define CENTERL4H   0,-H4	/* 4.5 */
#define CENTERL5    0,-K5	/* 5.0 */
#define CENTERL5H   0,-H5	/* 5.5 */

/* down from center by this amount */
#define CENTERD0H   0,-H0	/* 0.5 */
#define CENTERD1    0,-K1	/* 1.0 */
#define CENTERD1H   0,-H1	/* 1.5 */
#define CENTERD2    0,-K2	/* 2.0 */
#define CENTERD2H   0,-H2	/* 2.5 */
#define CENTERD3    0,-K3	/* 3.0 */
#define CENTERD3H   0,-H3	/* 3.5 */
#define CENTERD4    0,-K4	/* 4.0 */
#define CENTERD4H   0,-H4	/* 4.5 */
#define CENTERD5    0,-K5	/* 5.0 */
#define CENTERD5H   0,-H5	/* 5.5 */

/* in from left edge by this amount */
#define LEFTEDGE  -H0,  0	/* 0.0 */
#define LEFTIN0Q  -H0, Q0	/* 0.25 */
#define LEFTIN0H  -H0, H0	/* 0.5  */
#define LEFTIN0T  -H0, T0	/* 0.75 */
#define LEFTIN1   -H0, K1	/* 1.0  */
#define LEFTIN1Q  -H0, Q1	/* 1.25 */
#define LEFTIN1H  -H0, H1	/* 1.5  */
#define LEFTIN1T  -H0, T1	/* 1.75 */
#define LEFTIN2   -H0, K2	/* 2.0  */
#define LEFTIN2Q  -H0, Q2	/* 2.25 */
#define LEFTIN2H  -H0, H2	/* 2.5  */
#define LEFTIN2T  -H0, T2	/* 2.75 */
#define LEFTIN3   -H0, K3	/* 3.0  */
#define LEFTIN3Q  -H0, Q3	/* 3.25 */
#define LEFTIN3H  -H0, H3	/* 3.5  */
#define LEFTIN3T  -H0, T3	/* 3.75 */
#define LEFTIN4   -H0, K4	/* 4.0  */
#define LEFTIN4Q  -H0, Q4	/* 4.25 */
#define LEFTIN4H  -H0, H4	/* 4.5  */
#define LEFTIN4T  -H0, T4	/* 4.75 */
#define LEFTIN5   -H0, K5	/* 5.0  */
#define LEFTIN5Q  -H0, Q5	/* 5.25 */
#define LEFTIN5H  -H0, H5	/* 5.5  */
#define LEFTIN5T  -H0, T5	/* 5.75 */
#define LEFTIN6   -H0, K6	/* 6.0  */
#define LEFTIN6Q  -H0, Q6	/* 6.25 */
#define LEFTIN6H  -H0, H6	/* 6.5  */
#define LEFTIN6T  -H0, T6	/* 6.75 */
#define LEFTIN7   -H0, K7	/* 7.0  */
#define LEFTIN7Q  -H0, Q7	/* 7.25 */
#define LEFTIN7H  -H0, H7	/* 7.5  */
#define LEFTIN7T  -H0, T7	/* 7.75 */
#define LEFTIN8   -H0, K8	/* 8.0  */
#define LEFTIN8Q  -H0, Q8	/* 8.25 */
#define LEFTIN8H  -H0, H8	/* 8.5  */
#define LEFTIN8T  -H0, T8	/* 8.75 */
#define LEFTIN9   -H0, K9	/* 9.0  */
#define LEFTIN9Q  -H0, Q9	/* 9.25 */
#define LEFTIN9H  -H0, H9	/* 9.5  */
#define LEFTIN9T  -H0, T9	/* 9.75 */
#define LEFTIN10  -H0, K10	/* 10.0 */
#define LEFTIN10H -H0, H10	/* 10.5 */
#define LEFTIN11  -H0, K11	/* 11.0 */
#define LEFTIN11H -H0, H11	/* 11.5 */
#define LEFTIN12  -H0, K12	/* 12.0 */
#define LEFTIN12H -H0, H12	/* 12.5 */
#define LEFTIN13  -H0, K13	/* 13.0 */
#define LEFTIN13H -H0, H13	/* 13.5 */
#define LEFTIN14  -H0, K14	/* 14.0 */
#define LEFTIN14H -H0, H14	/* 14.5 */
#define LEFTIN15  -H0, K15	/* 15.0 */
#define LEFTIN15H -H0, H15	/* 15.5 */
#define LEFTIN16  -H0, K16	/* 16.0 */
#define LEFTIN16H -H0, H16	/* 16.5 */
#define LEFTIN17  -H0, K17	/* 17.0 */
#define LEFTIN17H -H0, H17	/* 17.5 */
#define LEFTIN18  -H0, K18	/* 18.0 */
#define LEFTIN18H -H0, H18	/* 18.5 */
#define LEFTIN19  -H0, K19	/* 19.0 */
#define LEFTIN19H -H0, H19	/* 19.5 */
#define LEFTIN27H -H0, H27	/* 27.5 */

/* in from bottom edge by this amount */
#define BOTEDGE   -H0,  0	/* 0.0  */
#define BOTIN0Q   -H0, Q0	/* 0.25 */
#define BOTIN0H   -H0, H0	/* 0.5  */
#define BOTIN0T   -H0, T0	/* 0.75 */
#define BOTIN1    -H0, K1	/* 1.0  */
#define BOTIN1Q   -H0, Q1	/* 1.25 */
#define BOTIN1H   -H0, H1	/* 1.5  */
#define BOTIN1T   -H0, T1	/* 1.75 */
#define BOTIN2    -H0, K2	/* 2.0  */
#define BOTIN2Q   -H0, Q2	/* 2.25 */
#define BOTIN2H   -H0, H2	/* 2.5  */
#define BOTIN2T   -H0, T2	/* 2.75 */
#define BOTIN3    -H0, K3	/* 3.0  */
#define BOTIN3Q   -H0, Q3	/* 3.25 */
#define BOTIN3H   -H0, H3	/* 3.5  */
#define BOTIN3T   -H0, T3	/* 3.75 */
#define BOTIN4    -H0, K4	/* 4.0  */
#define BOTIN4Q   -H0, Q4	/* 4.25 */
#define BOTIN4H   -H0, H4	/* 4.5  */
#define BOTIN4T   -H0, T4	/* 4.75 */
#define BOTIN5    -H0, K5	/* 5.0  */
#define BOTIN5Q   -H0, Q5	/* 5.25 */
#define BOTIN5H   -H0, H5	/* 5.5  */
#define BOTIN5T   -H0, T5	/* 5.75 */
#define BOTIN6    -H0, K6	/* 6.0  */
#define BOTIN6Q   -H0, Q6	/* 6.25 */
#define BOTIN6H   -H0, H6	/* 6.5  */
#define BOTIN6T   -H0, T6	/* 6.75 */
#define BOTIN7    -H0, K7	/* 7.0  */
#define BOTIN7Q   -H0, Q7	/* 7.25 */
#define BOTIN7H   -H0, H7	/* 7.5  */
#define BOTIN7T   -H0, T7	/* 7.75 */
#define BOTIN8    -H0, K8	/* 8.0  */
#define BOTIN8Q   -H0, Q8	/* 8.25 */
#define BOTIN8H   -H0, H8	/* 8.5  */
#define BOTIN8T   -H0, T8	/* 8.75 */
#define BOTIN9    -H0, K9	/* 9.0  */
#define BOTIN9Q   -H0, Q9	/* 9.25 */
#define BOTIN9H   -H0, H9	/* 9.5  */
#define BOTIN9T   -H0, T9	/* 9.75 */
#define BOTIN10   -H0, K10	/* 10.0 */
#define BOTIN10H  -H0, H10	/* 10.5 */
#define BOTIN11   -H0, K11	/* 11.0 */
#define BOTIN11H  -H0, H11	/* 11.5 */
#define BOTIN12   -H0, K12	/* 12.0 */
#define BOTIN12H  -H0, H12	/* 12.5 */
#define BOTIN13   -H0, K13	/* 13.0 */
#define BOTIN13H  -H0, H13	/* 13.5 */
#define BOTIN14   -H0, K14	/* 14.0 */
#define BOTIN14H  -H0, H14	/* 14.5 */
#define BOTIN15   -H0, K15	/* 15.0 */
#define BOTIN15H  -H0, H15	/* 15.5 */
#define BOTIN16   -H0, K16	/* 16.0 */
#define BOTIN16H  -H0, H16	/* 16.5 */
#define BOTIN17   -H0, K17	/* 17.0 */
#define BOTIN17H  -H0, H17	/* 17.5 */
#define BOTIN18   -H0, K18	/* 18.0 */
#define BOTIN18H  -H0, H18	/* 18.5 */
#define BOTIN19   -H0, K19	/* 19.0 */
#define BOTIN19H  -H0, H19	/* 19.5 */
#define BOTIN27H  -H0, H27	/* 27.5 */

/* in from top edge by this amount */
#define TOPEDGE    H0,  0	/* 0.0  */
#define TOPIN0Q    H0,-Q0	/* 0.25 */
#define TOPIN0H    H0,-H0	/* 0.5  */
#define TOPIN0T    H0,-T0	/* 0.75 */
#define TOPIN1     H0,-K1	/* 1.0  */
#define TOPIN1Q    H0,-Q1	/* 1.25 */
#define TOPIN1H    H0,-H1	/* 1.5  */
#define TOPIN1T    H0,-T1	/* 1.75 */
#define TOPIN2     H0,-K2	/* 2.0  */
#define TOPIN2Q    H0,-Q2	/* 2.25 */
#define TOPIN2H    H0,-H2	/* 2.5  */
#define TOPIN2T    H0,-T2	/* 2.75 */
#define TOPIN3     H0,-K3	/* 3.0  */
#define TOPIN3Q    H0,-Q3	/* 3.25 */
#define TOPIN3H    H0,-H3	/* 3.5  */
#define TOPIN3T    H0,-T3	/* 3.75 */
#define TOPIN4     H0,-K4	/* 4.0  */
#define TOPIN4Q    H0,-Q4	/* 4.25 */
#define TOPIN4H    H0,-H4	/* 4.5  */
#define TOPIN4T    H0,-T4	/* 4.75 */
#define TOPIN5     H0,-K5	/* 5.0  */
#define TOPIN5Q    H0,-Q5	/* 5.25 */
#define TOPIN5H    H0,-H5	/* 5.5  */
#define TOPIN5T    H0,-T5	/* 5.75 */
#define TOPIN6     H0,-K6	/* 6.0  */
#define TOPIN6Q    H0,-Q6	/* 6.25 */
#define TOPIN6H    H0,-H6	/* 6.5  */
#define TOPIN6T    H0,-T6	/* 6.75 */
#define TOPIN7     H0,-K7	/* 7.0  */
#define TOPIN7Q    H0,-Q7	/* 7.25 */
#define TOPIN7H    H0,-H7	/* 7.5  */
#define TOPIN7T    H0,-T7	/* 7.75 */
#define TOPIN8     H0,-K8	/* 8.0  */
#define TOPIN8Q    H0,-Q8	/* 8.25 */
#define TOPIN8H    H0,-H8	/* 8.5  */
#define TOPIN8T    H0,-T8	/* 8.75 */
#define TOPIN9     H0,-K9	/* 9.0  */
#define TOPIN9Q    H0,-Q9	/* 9.25 */
#define TOPIN9H    H0,-H9	/* 9.5  */
#define TOPIN9T    H0,-T9	/* 9.75 */
#define TOPIN10    H0,-K10	/* 10.0 */
#define TOPIN10H   H0,-H10	/* 10.5 */
#define TOPIN11    H0,-K11	/* 11.0 */
#define TOPIN11H   H0,-H11	/* 11.5 */
#define TOPIN12    H0,-K12	/* 12.0 */
#define TOPIN12H   H0,-H12	/* 12.5 */
#define TOPIN13    H0,-K13	/* 13.0 */
#define TOPIN13H   H0,-H13	/* 13.5 */
#define TOPIN14    H0,-K14	/* 14.0 */
#define TOPIN14H   H0,-H14	/* 14.5 */
#define TOPIN15    H0,-K15	/* 15.0 */
#define TOPIN15H   H0,-H15	/* 15.5 */
#define TOPIN16    H0,-K16	/* 16.0 */
#define TOPIN16H   H0,-H16	/* 16.5 */
#define TOPIN17    H0,-K17	/* 17.0 */
#define TOPIN17H   H0,-H17	/* 17.5 */
#define TOPIN18    H0,-K18	/* 18.0 */
#define TOPIN18H   H0,-H18	/* 18.5 */
#define TOPIN19    H0,-K19	/* 19.0 */
#define TOPIN19H   H0,-H19	/* 19.5 */
#define TOPIN27H   H0,-H27	/* 27.5 */

/* in from right edge by this amount */
#define RIGHTEDGE  H0,  0	/* 0.0  */
#define RIGHTIN0Q  H0,-Q0	/* 0.25 */
#define RIGHTIN0H  H0,-H0	/* 0.5  */
#define RIGHTIN0T  H0,-T0	/* 0.75 */
#define RIGHTIN1   H0,-K1	/* 1.0  */
#define RIGHTIN1Q  H0,-Q1	/* 1.25 */
#define RIGHTIN1H  H0,-H1	/* 1.5  */
#define RIGHTIN1T  H0,-T1	/* 1.75 */
#define RIGHTIN2   H0,-K2	/* 2.0  */
#define RIGHTIN2Q  H0,-Q2	/* 2.25 */
#define RIGHTIN2H  H0,-H2	/* 2.5  */
#define RIGHTIN2T  H0,-T2	/* 2.75 */
#define RIGHTIN3   H0,-K3	/* 3.0  */
#define RIGHTIN3Q  H0,-Q3	/* 3.25 */
#define RIGHTIN3H  H0,-H3	/* 3.5  */
#define RIGHTIN3T  H0,-T3	/* 3.75 */
#define RIGHTIN4   H0,-K4	/* 4.0  */
#define RIGHTIN4Q  H0,-Q4	/* 4.25 */
#define RIGHTIN4H  H0,-H4	/* 4.5  */
#define RIGHTIN4T  H0,-T4	/* 4.75 */
#define RIGHTIN5   H0,-K5	/* 5.0  */
#define RIGHTIN5Q  H0,-Q5	/* 5.25 */
#define RIGHTIN5H  H0,-H5	/* 5.5  */
#define RIGHTIN5T  H0,-T5	/* 5.75 */
#define RIGHTIN6   H0,-K6	/* 6.0  */
#define RIGHTIN6Q  H0,-Q6	/* 6.25 */
#define RIGHTIN6H  H0,-H6	/* 6.5  */
#define RIGHTIN6T  H0,-T6	/* 6.75 */
#define RIGHTIN7   H0,-K7	/* 7.0  */
#define RIGHTIN7Q  H0,-Q7	/* 7.25 */
#define RIGHTIN7H  H0,-H7	/* 7.5  */
#define RIGHTIN7T  H0,-T7	/* 7.75 */
#define RIGHTIN8   H0,-K8	/* 8.0  */
#define RIGHTIN8Q  H0,-Q8	/* 8.25 */
#define RIGHTIN8H  H0,-H8	/* 8.5  */
#define RIGHTIN8T  H0,-T8	/* 8.75 */
#define RIGHTIN9   H0,-K9	/* 9.0  */
#define RIGHTIN9Q  H0,-Q9	/* 9.25 */
#define RIGHTIN9H  H0,-H9	/* 9.5  */
#define RIGHTIN9T  H0,-T9	/* 9.75 */
#define RIGHTIN10  H0,-K10	/* 10.0 */
#define RIGHTIN10H H0,-H10	/* 10.5 */
#define RIGHTIN11  H0,-K11	/* 11.0 */
#define RIGHTIN11H H0,-H11	/* 11.5 */
#define RIGHTIN12  H0,-K12	/* 12.0 */
#define RIGHTIN12H H0,-H12	/* 12.5 */
#define RIGHTIN13  H0,-K13	/* 13.0 */
#define RIGHTIN13H H0,-H13	/* 13.5 */
#define RIGHTIN14  H0,-K14	/* 14.0 */
#define RIGHTIN14H H0,-H14	/* 14.5 */
#define RIGHTIN15  H0,-K15	/* 15.0 */
#define RIGHTIN15H H0,-H15	/* 15.5 */
#define RIGHTIN16  H0,-K16	/* 16.0 */
#define RIGHTIN16H H0,-H16	/* 16.5 */
#define RIGHTIN17  H0,-K17	/* 17.0 */
#define RIGHTIN17H H0,-H17	/* 17.5 */
#define RIGHTIN18  H0,-K18	/* 18.0 */
#define RIGHTIN18H H0,-H18	/* 18.5 */
#define RIGHTIN19  H0,-K19	/* 19.0 */
#define RIGHTIN19H H0,-H19	/* 19.5 */
#define RIGHTIN27H H0,-H27	/* 27.5 */

/*********************** LOOPING THROUGH POLYGONS ON NODE/ARC ************************/

typedef struct Ipolyloop
{
	INTBIG      realpolys;				/* polygon count without displayable variables */
	WINDOWPART *curwindowpart;			/* window used in getting polygons from node/arc */

	/* for multi-cut contacts */
	INTBIG      moscutlx, moscuthx, moscutly, moscuthy;
	INTBIG      moscutbasex, moscutbasey, moscutsizex, moscutsizey;
	INTBIG      moscutsep, moscuttopedge, moscutleftedge, moscutrightedge;
	INTBIG      moscuttotal, moscutsx, moscutsy;

	/* for serpentine transistors */
	VARIABLE   *serpentvar;

	/* for circular arcs */
	INTBIG      arcpieces, anglebase, anglerange;
	INTBIG      centerx, centery, radius;

	/* for displayable variables */
	INTBIG      numvar;
	VARIABLE   *firstvar;
	INTBIG      ndisplayindex;
	INTBIG      ndisplaysubindex;
} POLYLOOP;

/******************** MISCELLANEOUS ********************/

/* globally used arcprotos from the Generic technology */
#define AUNIV      (0|(1<<16))
#define AINVIS     (1|(1<<16))
#define AUNROUTED  (2|(1<<16))
#define ALLGEN     AUNIV,AINVIS,AUNROUTED

extern POLYLOOP    tech_oneprocpolyloop;

/* technology prototypes */
BOOLEAN   tech_doinitprocess(TECHNOLOGY*);
BOOLEAN   tech_doaddportsandvars(TECHNOLOGY*);
INTBIG    tech_initcurvedarc(ARCINST*, INTBIG, POLYLOOP*);
BOOLEAN   tech_curvedarcpiece(ARCINST*, INTBIG, POLYGON*, TECH_ARCS**, POLYLOOP*);
void      tech_addheadarrow(POLYGON*, INTBIG, INTBIG, INTBIG, INTBIG);
void      tech_adddoubleheadarrow(POLYGON*, INTBIG, INTBIG*, INTBIG*, INTBIG);
void      tech_add2linebody(POLYGON*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG);
INTBIG    tech_moscutcount(NODEINST*, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG*, POLYLOOP*);
INTBIG    tech_inittrans(INTBIG, NODEINST*, POLYLOOP*);
BOOLEAN   tech_pinusecount(NODEINST*, WINDOWPART*);
INTBIG    tech_displayablenvars(NODEINST*, WINDOWPART*, POLYLOOP*);
VARIABLE *tech_filldisplayablenvar(NODEINST*, POLYGON*, WINDOWPART*, VARIABLE**, POLYLOOP*);
INTBIG    tech_displayablecellvars(NODEPROTO *np, WINDOWPART *win, POLYLOOP *pl);
VARIABLE *tech_filldisplayablecellvar(NODEPROTO *np, POLYGON *poly, WINDOWPART *win,
			VARIABLE **varnoeval, POLYLOOP *pl);
INTBIG    tech_displayableportvars(PORTPROTO *pp, WINDOWPART *win, POLYLOOP *pl);
VARIABLE *tech_filldisplayableportvar(PORTPROTO *pp, POLYGON *poly, WINDOWPART *win,
			VARIABLE **varnoeval, POLYLOOP *pl);
void      tech_filltrans(POLYGON*, TECH_POLYGON**, TECH_SERPENT*, NODEINST*, INTBIG,
			INTBIG, TECH_PORTS*, POLYLOOP*);
void      tech_moscutpoly(NODEINST*, INTBIG, INTBIG[], POLYLOOP*);
void      tech_fillpoly(POLYGON*, TECH_POLYGON*, NODEINST*, INTBIG, INTBIG);
void      tech_filltransport(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, TECH_NODES*, INTBIG,
			INTBIG, INTBIG, INTBIG, INTBIG);
void      tech_fillportpoly(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, TECH_NODES*, INTBIG, INTBIG);
void      tech_resetnegated(ARCINST*);
INTBIG    tech_displayableavars(ARCINST*, WINDOWPART*, POLYLOOP*);
VARIABLE *tech_filldisplayableavar(ARCINST*, POLYGON*, WINDOWPART*, VARIABLE**, POLYLOOP*);
void      tech_makearrow(ARCINST*, POLYGON*);
INTBIG    tech_getextendfactor(INTBIG, INTBIG);
void      tech_makeendpointpoly(INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG, INTBIG,
			INTBIG, INTBIG, POLYGON*);
void      tech_convertmocmoslib(LIBRARY*);
INTBIG    tech_nodepolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl);
void      tech_shapenodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl);
INTBIG    tech_nodeEpolys(NODEINST *ni, INTBIG *reasonable, WINDOWPART *win, POLYLOOP *pl);
void      tech_shapeEnodepoly(NODEINST *ni, INTBIG box, POLYGON *poly, POLYLOOP *pl);
INTBIG    tech_arcpolys(ARCINST *ai, WINDOWPART *win, POLYLOOP *pl);
void      tech_shapearcpoly(ARCINST *ai, INTBIG box, POLYGON *poly, POLYLOOP *pl);
void      tech_nodeprotosizeoffset(NODEPROTO *np, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy,
			INTBIG lambda);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
