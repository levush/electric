/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: efunction.h
 * Node, arc, and layer function table
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

/*
 * when changing this table, also change:
 * dbtech.c:db_nodefunname[]
 * dbtech.c:nodefunction(), transistorsize(), isfet()
 * erc:erc_analyzecell()
 * ioedifo.c
 * netflat.c:net_getfunction()
 * simsilos.c:sim_silostype()
 * simspice.c:sim_spice_nodetype()
 * simtexsim.c:sim_writetexcell(), sim_texproto()
 * usrctech.c:us_tran_logmakenodes(), us_tranismos()
 * usrmenu.c:us_groupfunction()
 * vhdl.c:vhdl_primname()
 */

/* the value of (NODEPROTO->userbits&NFUNCTION)>>NFUNCTIONSH (node functions) */
#define NPUNKNOWN           0	/* node is unknown type */
#define NPPIN               1	/* node is a single-layer pin */
#define NPCONTACT           2	/* node is a two-layer contact (one point) */
#define NPNODE              3	/* node is a single-layer node */
#define NPCONNECT           4	/* node connects all ports */
#define NPTRANMOS           5	/* node is MOS enhancement transistor */
#define NPTRADMOS           6	/* node is MOS depletion transistor */
#define NPTRAPMOS           7	/* node is MOS complementary transistor */
#define NPTRANPN            8	/* node is NPN junction transistor */
#define NPTRAPNP            9	/* node is PNP junction transistor */
#define NPTRANJFET         10	/* node is N-channel junction transistor */
#define NPTRAPJFET         11	/* node is P-channel junction transistor */
#define NPTRADMES          12	/* node is MESFET depletion transistor */
#define NPTRAEMES          13	/* node is MESFET enhancement transistor */
#define NPTRANSREF         14	/* node is prototype-defined transistor */
#define NPTRANS            15	/* node is undetermined transistor */
#define NPTRA4NMOS         16	/* node is 4-port MOS enhancement transistor */
#define NPTRA4DMOS         17	/* node is 4-port MOS depletion transistor */
#define NPTRA4PMOS         18	/* node is 4-port MOS complementary transistor */
#define NPTRA4NPN          19	/* node is 4-port NPN junction transistor */
#define NPTRA4PNP          20	/* node is 4-port PNP junction transistor */
#define NPTRA4NJFET        21	/* node is 4-port N-channel junction transistor */
#define NPTRA4PJFET        22	/* node is 4-port P-channel junction transistor */
#define NPTRA4DMES         23	/* node is 4-port MESFET depletion transistor */
#define NPTRA4EMES         24	/* node is 4-port MESFET enhancement transistor */
#define NPTRANS4           25	/* node is E2L transistor */
#define NPRESIST           26	/* node is resistor */
#define NPCAPAC            27	/* node is capacitor */
#define NPECAPAC           28	/* node is electrolytic capacitor */
#define NPDIODE            29	/* node is diode */
#define NPDIODEZ           30	/* node is zener diode */
#define NPINDUCT           31	/* node is inductor */
#define NPMETER            32	/* node is meter */
#define NPBASE             33	/* node is transistor base */
#define NPEMIT             34	/* node is transistor emitter */
#define NPCOLLECT          35	/* node is transistor collector */
#define NPBUFFER           36	/* node is buffer */
#define NPGATEAND          37	/* node is AND gate */
#define NPGATEOR           38	/* node is OR gate */
#define NPGATEXOR          39	/* node is XOR gate */
#define NPFLIPFLOP         40	/* node is flip-flop */
#define NPMUX              41	/* node is multiplexor */
#define NPCONPOWER         42	/* node is connected to power */
#define NPCONGROUND        43	/* node is connected to ground */
#define NPSOURCE           44	/* node is source */
#define NPSUBSTRATE        45	/* node is connected to substrate */
#define NPWELL             46	/* node is connected to well */
#define NPART              47	/* node is pure artwork */
#define NPARRAY            48	/* node is an array */
#define NPALIGN            49	/* node is an alignment object */
#define NPCCVS             50	/* node is a current-controlled voltage source */
#define NPCCCS             51	/* node is a current-controlled current source */
#define NPVCVS             52	/* node is a voltage-controlled voltage source */
#define NPVCCS             53	/* node is a voltage-controlled current source */
#define NPTLINE            54	/* node is a transmission line */

#define MAXNODEFUNCTION    55	/* the number of functions above */

/*
 * when changing this table, also change:
 * dbtech.c:     arcfunctionname()
 * iolout.c:     io_lports[] and io_lcontacts[]
 * ercantenna.c: erc_antcheckcell()
 * sc1electric:  Sc_setup_for_maker()
 * simsim.c:     sim_prop()
 * simspice.c:   sim_spice_arcisdiff()
 * usredtecc.c:  us_tecarc_functions[]
 */

/* the value of (ARCPROTO->userbits&AFUNCTION)>>AFUNCTIONSH (arc functions) */
#define APUNKNOWN           0	/* arc is unknown type */
#define APMETAL1            1	/* arc is metal, layer 1 */
#define APMETAL2            2	/* arc is metal, layer 2 */
#define APMETAL3            3	/* arc is metal, layer 3 */
#define APMETAL4            4	/* arc is metal, layer 4 */
#define APMETAL5            5	/* arc is metal, layer 5 */
#define APMETAL6            6	/* arc is metal, layer 6 */
#define APMETAL7            7	/* arc is metal, layer 7 */
#define APMETAL8            8	/* arc is metal, layer 8 */
#define APMETAL9            9	/* arc is metal, layer 9 */
#define APMETAL10          10	/* arc is metal, layer 10 */
#define APMETAL11          11	/* arc is metal, layer 11 */
#define APMETAL12          12	/* arc is metal, layer 12 */
#define APPOLY1            13	/* arc is polysilicon, layer 1 */
#define APPOLY2            14	/* arc is polysilicon, layer 2 */
#define APPOLY3            15	/* arc is polysilicon, layer 3 */
#define APDIFF             16	/* arc is diffusion */
#define APDIFFP            17	/* arc is P-type diffusion */
#define APDIFFN            18	/* arc is N-type diffusion */
#define APDIFFS            19	/* arc is substrate diffusion */
#define APDIFFW            20	/* arc is well diffusion */
#define APBUS              21	/* arc is multi-wire bus */
#define APUNROUTED         22	/* arc is unrouted specification */
#define APNONELEC          23	/* arc is nonelectrical */

/*
 * when changing this table, also change:
 * dbtech.c:         layerismetal(), layerispoly(), layeriscontact(), layerfunctionheight()
 * ercantenna.c:     erc_antcheckcell()
 * iodefi.c:         io_defgetlayernodes()
 * iolefi.c:         io_lefparselayer()
 * iolefo.c:         io_lefoutlayername()
 * iopsoutcolor.cpp: io_pscolor_getLayerMap()
 * simspice.c:       sim_spice_layerisdiff()
 * usredtecc.c:      us_teclayer_functions[]
 * usrcom1.c:        technologyclfopt[]
 */

/* the value of TECHNOLOGY->TECH_layer_function (layer functions) */
#define LFNUMLAYERS       044	/* number of layers below */
#define LFUNKNOWN           0	/* unknown layer */
#define LFMETAL1           01	/* metal layer 1 */
#define LFMETAL2           02	/* metal layer 2 */
#define LFMETAL3           03	/* metal layer 3 */
#define LFMETAL4           04	/* metal layer 4 */
#define LFMETAL5           05	/* metal layer 5 */
#define LFMETAL6           06	/* metal layer 6 */
#define LFMETAL7           07	/* metal layer 7 */
#define LFMETAL8          010	/* metal layer 8 */
#define LFMETAL9          011	/* metal layer 9 */
#define LFMETAL10         012	/* metal layer 10 */
#define LFMETAL11         013	/* metal layer 11 */
#define LFMETAL12         014	/* metal layer 12 */
#define LFPOLY1           015	/* polysilicon layer 1 */
#define LFPOLY2           016	/* polysilicon layer 2 */
#define LFPOLY3           017	/* polysilicon layer 3 */
#define LFGATE            020	/* polysilicon gate layer */
#define LFDIFF            021	/* diffusion layer */
#define LFIMPLANT         022	/* implant layer */
#define LFCONTACT1        023	/* contact layer 1 */
#define LFCONTACT2        024	/* contact layer 2 */
#define LFCONTACT3        025	/* contact layer 3 */
#define LFCONTACT4        026	/* contact layer 4 */
#define LFCONTACT5        027	/* contact layer 5 */
#define LFCONTACT6        030	/* contact layer 6 */
#define LFCONTACT7        031	/* contact layer 7 */
#define LFCONTACT8        032	/* contact layer 8 */
#define LFCONTACT9        033	/* contact layer 9 */
#define LFCONTACT10       034	/* contact layer 10 */
#define LFCONTACT11       035	/* contact layer 11 */
#define LFCONTACT12       036	/* contact layer 12 */
#define LFPLUG            037	/* sinker (diffusion-to-buried plug) */
#define LFOVERGLASS       040	/* overglass layer */
#define LFRESISTOR        041	/* resistor layer */
#define LFCAP             042	/* capacitor layer */
#define LFTRANSISTOR      043	/* transistor layer */
#define LFEMITTER         044	/* emitter layer */
#define LFBASE            045	/* base layer */
#define LFCOLLECTOR       046	/* collector layer */
#define LFSUBSTRATE       047	/* substrate layer */
#define LFWELL            050	/* well layer */
#define LFGUARD           051	/* guard layer */
#define LFISOLATION       052	/* isolation layer */
#define LFBUS             053	/* bus layer */
#define LFART             054	/* artwork layer */
#define LFCONTROL         055	/* control layer */

#define LFTYPE            077	/* all above layers */
#define LFPTYPE          0100	/* layer is P-type */
#define LFNTYPE          0200	/* layer is N-type */
#define LFDEPLETION      0400	/* layer is depletion */
#define LFENHANCEMENT   01000	/* layer is enhancement */
#define LFLIGHT         02000	/* layer is light doped */
#define LFHEAVY         04000	/* layer is heavy doped */
#define LFPSEUDO       010000	/* layer is pseudo */
#define LFNONELEC      020000	/* layer is nonelectrical */
#define LFCONMETAL     040000	/* layer contacts metal */
#define LFCONPOLY     0100000	/* layer contacts polysilicon */
#define LFCONDIFF     0200000	/* layer contacts diffusion */
#define LFTRANS1      0400000	/* layer is transparent number 1 */
#define LFTRANS2     01000000	/* layer is transparent number 2 */
#define LFTRANS3     02000000	/* layer is transparent number 3 */
#define LFTRANS4     04000000	/* layer is transparent number 4 */
#define LFTRANS5    010000000	/* layer is transparent number 5 */
#define LFINTRANS   020000000	/* layer inside transistor */
