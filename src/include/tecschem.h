/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: tecschem.h
 * Schematic technology header
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

/* special cases of the NODEINST->userbits&NTECHBITS field */
#define FFTYPE              01400000		/* FlipFlop type bits */
#define FFTYPERS                   0		/*    FlipFlop is RS */
#define FFTYPEJK             0400000		/*    FlipFlop is JK */
#define FFTYPED             01000000		/*    FlipFlop is D */
#define FFTYPET             01400000		/*    FlipFlop is T */
#define FFCLOCK             06000000		/* FlipFlop clocking bits */
#define FFCLOCKMS                  0		/*    FlipFlop is Master/slave */
#define FFCLOCKP            02000000		/*    FlipFlop is Positive clock */
#define FFCLOCKN            04000000		/*    FlipFlop is Negative clock */

#define TRANNMOS                   0		/* Transistor is N channel MOS */
#define TRANDMOS             0400000		/* Transistor is Depletion MOS */
#define TRANPMOS            01000000		/* Transistor is P channel MOS */
#define TRANNPN             01400000		/* Transistor is NPN Junction */
#define TRANPNP             02000000		/* Transistor is PNP Junction */
#define TRANNJFET           02400000		/* Transistor is N Channel Junction FET */
#define TRANPJFET           03000000		/* Transistor is P Channel Junction FET */
#define TRANDMES            03400000		/* Transistor is Depletion MESFET */
#define TRANEMES            04000000		/* Transistor is Enhancement MESFET */

#define DIODENORM                  0		/* Diode is normal */
#define DIODEZENER           0400000		/* Diode is Zener */

#define CAPACNORM                  0		/* Capacitor is normal */
#define CAPACELEC            0400000		/* Capacitor is Electrolytic */

#define TWOPVCCS                   0		/* Two-port is Transconductance (VCCS) */
#define TWOPCCVS             0400000		/* Two-port is Transresistance (CCVS) */
#define TWOPVCVS            01000000		/* Two-port is Voltage gain (VCVS) */
#define TWOPCCCS            01400000		/* Two-port is Current gain (CCCS) */
#define TWOPTLINE           02000000		/* Two-port is Transmission Line */

/* all of the primitives */
extern TECHNOLOGY *sch_tech;			/* the technology */

extern NODEPROTO  *sch_wirepinprim;		/* wire pin */
extern NODEPROTO  *sch_buspinprim;		/* bus pin */
extern NODEPROTO  *sch_wireconprim;		/* wire connect */
extern NODEPROTO  *sch_bufprim;			/* general BUFFER */
extern NODEPROTO  *sch_andprim;			/* general AND */
extern NODEPROTO  *sch_orprim;			/* general OR */
extern NODEPROTO  *sch_xorprim;			/* general XOR */
extern NODEPROTO  *sch_ffprim;			/* general FLIP FLOP */
extern NODEPROTO  *sch_muxprim;			/* general MUX */
extern NODEPROTO  *sch_bboxprim;		/* black box */
extern NODEPROTO  *sch_switchprim;		/* switch */
extern NODEPROTO  *sch_offpageprim;		/* off page connector */
extern NODEPROTO  *sch_pwrprim;			/* power */
extern NODEPROTO  *sch_gndprim;			/* ground */
extern NODEPROTO  *sch_sourceprim;		/* source (voltage, current) */
extern NODEPROTO  *sch_transistorprim;	/* transistor */
extern NODEPROTO  *sch_resistorprim;	/* resistor */
extern NODEPROTO  *sch_capacitorprim;	/* capacitor */
extern NODEPROTO  *sch_diodeprim;		/* diode */
extern NODEPROTO  *sch_inductorprim;	/* inductor */
extern NODEPROTO  *sch_meterprim;		/* meter */
extern NODEPROTO  *sch_wellprim;		/* well connection */
extern NODEPROTO  *sch_substrateprim;	/* substrate connection */
extern NODEPROTO  *sch_twoportprim;		/* generic two-port block */
extern NODEPROTO  *sch_transistor4prim;	/* 4-port transistor */
extern NODEPROTO  *sch_globalprim;		/* global signal */

extern ARCPROTO   *sch_wirearc;			/* wire arc */
extern ARCPROTO   *sch_busarc;			/* bus arc */

extern INTBIG      sch_wirepinsizex;	/* X size if wire-pin primitives */
extern INTBIG      sch_wirepinsizey;	/* Y size if wire-pin primitives */

extern INTBIG      sch_meterkey;		/* key for "SCHEM_meter_type" */
extern INTBIG      sch_diodekey;		/* key for "SCHEM_diode" */
extern INTBIG      sch_capacitancekey;	/* key for "SCHEM_capacitance" */
extern INTBIG      sch_resistancekey;	/* key for "SCHEM_resistance" */
extern INTBIG      sch_inductancekey;	/* key for "SCHEM_inductance" */
extern INTBIG      sch_functionkey;		/* key for "SCHEM_function" */
extern INTBIG      sch_spicemodelkey;	/* key for "SIM_spice_model" */
extern INTBIG      sch_globalnamekey;	/* key for "SCHEM_global_name" */

/* prototypes for technology routines */
BOOLEAN sch_initprocess(TECHNOLOGY*, INTBIG);
void    sch_termprocess(void);
void    sch_setmode(INTBIG, CHAR*[]);
INTBIG  sch_request(CHAR*, va_list);
void    sch_shapenodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  sch_nodepolys(NODEINST*, INTBIG*, WINDOWPART*);
INTBIG  sch_allnodepolys(NODEINST*, POLYLIST*, WINDOWPART*, BOOLEAN);
INTBIG  sch_nodeEpolys(NODEINST*, INTBIG*, WINDOWPART*);
void    sch_shapeEnodepoly(NODEINST*, INTBIG, POLYGON*);
INTBIG  sch_allnodeEpolys(NODEINST *ni, POLYLIST *plist, WINDOWPART *win, BOOLEAN onlyreasonable);
void    sch_nodesizeoffset(NODEINST *ni, INTBIG *lx, INTBIG *ly, INTBIG *hx, INTBIG *hy);
void    sch_shapeportpoly(NODEINST*, PORTPROTO*, POLYGON*, XARRAY, BOOLEAN);
INTBIG  sch_arcpolys(ARCINST*, WINDOWPART*);
void    sch_shapearcpoly(ARCINST*, INTBIG, POLYGON*);
INTBIG  sch_allarcpolys(ARCINST*, POLYLIST*, WINDOWPART*);

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
