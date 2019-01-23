/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: romgen.java
 * User interface tool: main module
 * Written by: David Harris (David_Harris@hmc.edu)
 * Based on code developed by Frank Lee <chlee@hmc.edu> and Jason Imada <jimada@hmc.edu> 
 *
 * Copyright (c) 2003 Static Free Software.
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
 * The romgen.java code creates layout for a pseudo-nMOS ROM array in Electric.
 *
 * To compile romgen.java, place the file in the
 * lib/java subdirectory of your Electric install directory and
 * compile it with:
 *
 *    javac romgen.java
 *
 * to produce the romgen.class file in the same subdirectory.
 * Note that this should be done automatically by the "Makefile".
 *
 * The ROM generator can be invoked from inside of Electric with the
 * "Tools / Generator / ROM..." command.  You can also invoke it
 * directly in the Java interpreter by typing:
 *     romgen.main("romtable.txt", "rom", 600)
 * where "romtable.txt" is the name of the personality file, "rom"
 * is the prefix for cell names that will be generated, and 600 is
 * the feature size (twice lambda) measured in nanometers.
 *
 * The personality file has this format:
 * The first line lists the degree of folding.  For example,
 * a 256-word x 10-bit ROM with a folding degree of 4 will
 * be implemented as a 64 x 40 array with 4:1 column multiplexers
 * to return 10 bits of data while occupying more of a square form
 * factor. The number of words and degree of folding should be
 * powers of 2.  The remaining lines of the file list the contents
 * of each word.  The parser is pretty picky.  There should
 * be a carriage return after the list word, but no other blank
 * lines in the file.
 *
 * The tool may be slow, especially for large ROMs.  When done, there may be some
 * extraneous and bad pins left over; using "Edit / Cleanup Cell / Cleanup
 * Pins Everywhere" and "Info / Check and Repair Libraries" will fix these
 * (though this isn't strictly necessary).
 *
 * The ROMs produced should pass DRC and ERC and simulate with IRSIM.  One
 * was successfully fabricated Spring of 2002 by Genevieve Breed and Matthew
 * Erler in the MOSIS 0.6 micron AMI process.
 *
 * Happy Hacking!
 */

/* libraries */
import java.io.*;
import COM.staticfreesoft.*;

public class romgen {

public static int globalbits;
public static int folds;
public static int lambda = (int) (0.3*2000);

//constants for setting export type
final static public int STATEBITS = 036000000000;
final static public int INPORT    = 020000000000;
final static public int OUTPORT   = 022000000000;
final static public int BIDIRPORT = 024000000000;
final static public int PWRPORT   = 026000000000;
final static public int GNDPORT   = 030000000000;
		
///////////////////romarraygen start
public static int[][] romarraygen(String romfile) {
	boolean end = false;
	int[][] returnarray = new int[1][1];
	//romfile = romfile+".txt";

	try {
		BufferedReader in = new BufferedReader(new FileReader(romfile));
		try {
  			int w = -1;
			int bits = 0;
  			StringBuffer sb;
			StringBuffer allfile = new StringBuffer();
  			while (!end) {
  				w++;
  				String temp = in.readLine();
  				if (temp == null) {
  					end = true;
  				}
  				else {
  					sb = new StringBuffer(temp);
					if (w==1) {
						bits = sb.length();
					}
					if (w==0) {
						folds = Integer.parseInt(temp,10);
					}
					else {
						allfile.append(sb);
					}
	  			}
			}
			w--;
			//set globalbits
			globalbits = w/folds;
			returnarray = new int[bits][w];
			for (int r=0; r<w; r++) {
				for (int s=0; s<bits; s++) {
					if (allfile.charAt(r*bits + s) == '1') {
						returnarray[s][w-r-1] = 1;
					}
					else {
						returnarray[s][w-r-1] = 0; 
					}
				}
			}
		} catch (IOException e)  {
		}
	} catch(FileNotFoundException e)  {
		System.out.println(e.toString());
	}
	return returnarray;
}
//////////////////romarraygen end

/////////////////generateplane
public static int[][] generateplane(int bits) {
	int lines = (int) (Math.pow(2.0, (double) bits));
	char[][] wordlines = new char[lines][bits];
	for (int i = 0; i<lines; i++) {
		int len = Integer.toBinaryString(i).length();
		int leadingz = bits - len;
		int h;
		for (h=0; h<leadingz; h++) {
			wordlines[i][h] = '0';
		}
		for (int j=h; j<bits; j++) {
			wordlines[i][j] = Integer.toBinaryString(i).charAt(j-h);
		}
	}
	int[][] x =  new int[lines][bits];
	for (int j = 0; j<lines; j++) {
		for (int k = 0; k<bits; k++) {
			x[j][k] = Character.getNumericValue(wordlines[j][k]);
		}
	}
	int[][] wcomp = new int[lines][2*bits];
	for (int j = 0; j<lines; j++) {
		for (int k = 0; k<bits; k++) {
			wcomp[j][(2*k)] = x[j][k];
			int complement;
			if (x[j][k] == 1) {
				complement = 0;
			}
			else {
				complement = 1;
			}
			wcomp[j][(2*k)+1] = complement;
		}
	}
	int[][] wcompb = new int[lines][2*bits];
	for (int j = 0; j<lines; j++) {
		for (int k = 0; k<(2*bits); k++) {
			wcompb[j][k] = wcomp[j][(2*bits)-1-k];
		}
	}
	return wcompb;
}
////////////////generateplane end

////////////////romfold begin
public static int[][] romfold(int[][] romarray) {
	int roma = romarray.length*folds;
	int romb = romarray[1].length/folds;
	int[][] foldedrom = new int[roma][romb];
	
	for (int i=0; i<romarray.length; i++) {
		for (int j=0; j<folds; j++) {
			for (int k=0; k<romb; k++) {
				foldedrom[folds*i+j][k] = romarray[i][j*romb+k];
			}
		}
	}
	return foldedrom;
}
////////////////romfold end

////////////////createExport end
public static void createExport(Electric.PortProto p, int exporttype) {
	int userbits = ((Integer)Electric.getVal(p, "userbits")).intValue();
	Electric.setVal(p,"userbits",new Integer((userbits&~STATEBITS)|exporttype),0);
}
////////////////createExport end

/////////////main start
public static int main(String romfile, String romcell, int featuresize) {

	Electric.NodeInst ap1, ap2, ap3, ap4;
	Electric.PortProto p, apport1, apport2, apport3, apport4;
	Integer[] appos1, appos2, appos3, appos4;

	lambda = featuresize;
	int[][] romarray = romarraygen(romfile);

	String dpr  = new String(romcell+"_decoderpmos");
	String dnr  = new String(romcell+"_decodernmos");
	String dpm  = new String(romcell+"_decoderpmosmux");
	String dnm  = new String(romcell+"_decodernmosmux");
	String invt = new String(romcell+"_ininvertertop");
	String invb = new String(romcell+"_ininverterbot");
	String romname =  new String(romcell+"_rom");
	String rp =   new String(romcell+"_romplane");
	String ip =   new String(romcell+"_inverterplane");
	String mp =   new String(romcell+"_muxplane");


	if (folds > 1) {
		romarray = romfold(romarray);
	}
	romplane(lambda, romarray, rp);

	int bits =
		(new Double(Math.ceil(Math.log(globalbits)/Math.log((double)2.0)))).intValue();
	int words = (int) (Math.pow(2.0, (double) bits));
	int foldbits =
		(new Double(Math.ceil(Math.log(folds)/Math.log((double) 2.0)))).intValue();

	boolean top = true;
	boolean bot = false;

	decoderpmos(lambda,bits,dpr,top);
	decodernmos(lambda,bits,dnr,top);
	inverterplane(lambda,romarray.length,folds, ip);
	ininverterplane(lambda,bits,invt,top,bits);

	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	
//////////////decoderpmos	
	Electric.NodeProto decp = Electric.getNodeProto(dpr+"{lay}");
	int[] decpbox = {((Integer)Electric.getVal(decp, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(decp, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(decp, "lowy")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(decp, "highy")).intValue()+lambda/2};
 	Electric.PortProto[] decpin = new Electric.PortProto[words];
 	Electric.PortProto[] decpout = new Electric.PortProto[words];
	Electric.PortProto[] decpbit = new Electric.PortProto[2*bits];
	Electric.PortProto decpvdd = (Electric.PortProto)Electric.getVal(decp, "vdd");
	Electric.PortProto decpvddb = (Electric.PortProto)Electric.getVal(decp, "vddb");
	for (int i=0; i<words; i++) {
		decpin[i] = (Electric.PortProto)Electric.getVal(decp, "wordin"+i);
		decpout[i] = (Electric.PortProto)Electric.getVal(decp, "word"+i);
	}
	for (int i=0; i<bits; i++) {
		decpbit[2*i] = (Electric.PortProto)Electric.getVal(decp, "top_in"+i);
		decpbit[(2*i)+1] = (Electric.PortProto)Electric.getVal(decp, "top_in"+i+"_b");
	}

	//////////////decodernmos	
	Electric.NodeProto decn = Electric.getNodeProto(dnr+"{lay}");
	int[] decnbox = {((Integer)Electric.getVal(decn, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(decn, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(decn, "lowy")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(decn, "highy")).intValue()+lambda/2};
 	Electric.PortProto[] decnout = new Electric.PortProto[words];
 	Electric.PortProto[] decnin = new Electric.PortProto[words];
 	Electric.PortProto[] decnbit = new Electric.PortProto[2*bits];
	
	for (int i=0; i<words; i++) {
		decnin[i] = (Electric.PortProto)Electric.getVal(decn, "mid"+i);
		decnout[i] = (Electric.PortProto)Electric.getVal(decn, "word"+i);
	}
	for (int i=0; i<bits; i++) {
		decnbit[2*i] = (Electric.PortProto)Electric.getVal(decn, "top_in"+i);
		decnbit[(2*i)+1] = (Electric.PortProto)Electric.getVal(decn, "top_in"+i+"_b");
	}

//////////////////////romplane
	Electric.NodeProto romp = Electric.getNodeProto(rp+"{lay}");
	int[] rompbox = {((Integer)Electric.getVal(romp, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(romp, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(romp, "lowy")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(romp, "highy")).intValue()+lambda/2};
	Electric.PortProto[] rompin = new Electric.PortProto[globalbits];
	Electric.PortProto[] rompout = new Electric.PortProto[romarray.length];
	Electric.PortProto[] rompgnd = new Electric.PortProto[romarray.length/2];
	Electric.PortProto rompvdd = (Electric.PortProto) Electric.getVal(romp,"vdd");
	Electric.PortProto rompgndc = (Electric.PortProto) Electric.getVal(romp,"gndc");
	for (int i=0; i<globalbits; i++) {
		rompin[i] = (Electric.PortProto)Electric.getVal(romp, "wordline_"+i);
	}
	for (int i=0; i<romarray.length; i++) {
		rompout[i] = (Electric.PortProto)Electric.getVal(romp, "out_"+i);
	}
	for (int i=0; i<romarray.length/2; i++) {
		rompgnd[i] = (Electric.PortProto)Electric.getVal(romp, "romgnd"+i);
	}

//////////////////////inverterplane
	Electric.NodeProto invp = Electric.getNodeProto(ip+"{lay}");
	int[] invpbox = {((Integer)Electric.getVal(invp, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(invp, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(invp, "lowy")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(invp, "highy")).intValue()+lambda/2};
	Electric.PortProto[] invin = new Electric.PortProto[romarray.length];
	Electric.PortProto[] invout = new Electric.PortProto[romarray.length];
	Electric.PortProto[] invgnd = new Electric.PortProto[romarray.length/2];
	Electric.PortProto invvddc = (Electric.PortProto)Electric.getVal(invp, ("vdd"));
	Electric.PortProto invgndc = (Electric.PortProto)Electric.getVal(invp, ("gnd"));
	for (int i=0; i<romarray.length/folds; i++) {
		invin[i] = (Electric.PortProto)Electric.getVal(invp, ("invin"+i));
		invout[i] = (Electric.PortProto)Electric.getVal(invp, ("invout"+i));
	}
	
	int invplanegnd = romarray.length/folds;
	if (folds == 1) {
		invplanegnd = invplanegnd / 2;
	}

	for (int i=0; i<invplanegnd; i++) {
		invgnd[i] = (Electric.PortProto)Electric.getVal(invp, ("invgnd"+i));
	}

//////////////////////ininverterplane top
	Electric.NodeProto ininvtp = Electric.getNodeProto(invt+"{lay}");
	int[] ininvtpbox =
		{((Integer)Electric.getVal(ininvtp, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(ininvtp, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(ininvtp, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(ininvtp, "highy")).intValue()+lambda/2};
	Electric.PortProto[] ivttop  = new Electric.PortProto[bits];
	Electric.PortProto[] ivtbot = new Electric.PortProto[bits];
	Electric.PortProto[] ivtbar = new Electric.PortProto[bits];
	Electric.PortProto ivtvdd = (Electric.PortProto)Electric.getVal(ininvtp, ("vdd"));
	Electric.PortProto ivtgnd = (Electric.PortProto)Electric.getVal(ininvtp, ("gnd"));
	for (int i=0; i<bits; i++) {
		ivttop[i] = (Electric.PortProto)Electric.getVal(ininvtp, ("in_top"+i));
		ivtbot[i] = (Electric.PortProto)Electric.getVal(ininvtp, ("in_bot"+i));
		ivtbar[i] = (Electric.PortProto)Electric.getVal(ininvtp, ("in_b"+i));
	}

	//create new layout named "rom{lay}" in current library
	Electric.NodeProto rom = Electric.newNodeProto(romname+"{lay}", Electric.curLib());

//////////calculate pplane offset
	int offset = (2*bits*(8*lambda)) + (16*lambda);
	int rompoffset = (8*lambda)*2*bits + (12*lambda) + offset;
	int rompoffsety = 8*lambda*(globalbits+1);
	int foldoffsetx = (2*(bits-foldbits)*(8*lambda));
	int muxpoffsety = -6*lambda;
	int foldoffsety = -8*lambda*(folds+1);
	int ininvtoffset = (globalbits+2)*8*lambda+48*lambda;

	int invpoffsety = -8*lambda*(folds+1)-16*lambda;
	if (folds == 1) {
		invpoffsety = invpoffsety + 24*lambda;
	}

	Electric.NodeInst nplane =
		Electric.newNodeInst(decn, decnbox[0]+offset, decnbox[1]+offset, decnbox[2],
							 decnbox[3], 0, 0, rom);
	Electric.NodeInst pplane =
		Electric.newNodeInst(decp, decpbox[0], decpbox[1], decpbox[2], decpbox[3],
							 0, 0, rom);
	Electric.NodeInst rompln =
		Electric.newNodeInst(romp, rompbox[0]+rompoffset, rompbox[1]+rompoffset,
							 rompbox[2]+rompoffsety, rompbox[3]+rompoffsety,
							 0, 2700, rom);
	Electric.NodeInst invpln =
		Electric.newNodeInst(invp, invpbox[0]+rompoffset, invpbox[1]+rompoffset,
							 invpbox[2]+invpoffsety,invpbox[3]+invpoffsety, 0, 0, rom);
	Electric.NodeInst ininvtop1 =
		Electric.newNodeInst(ininvtp,ininvtpbox[0],ininvtpbox[1],
							 ininvtpbox[2]+ininvtoffset,ininvtpbox[3]+ininvtoffset,
							 0, 0, rom);
	Electric.NodeInst ininvtop2 =
		Electric.newNodeInst(ininvtp,ininvtpbox[0]+offset,ininvtpbox[1]+offset,
							 ininvtpbox[2]+ininvtoffset,
							 ininvtpbox[3]+ininvtoffset, 0, 0, rom);

//////////////exports on top level
	for (int i=0; i<bits; i++) {
		ap1 = ininvtop1;
		apport1 = ivttop[i];
		p = Electric.newPortProto(rom, ap1, apport1, ("sel"+i));
		createExport(p, INPORT);
	}
	for (int i=0; i<romarray.length/folds; i++) {
		ap1 = invpln;
		apport1 = invout[i];
		p = Electric.newPortProto(rom, ap1, apport1, ("out"+i));
		createExport(p, OUTPORT);
	}


	ap2 = rompln;
	apport2 = rompvdd;
	p = Electric.newPortProto(rom, ap2, apport2, ("vdd"));
	createExport(p, PWRPORT);

	ap1 = nplane;
	apport1 = (Electric.PortProto)Electric.getVal(decn, "gnd");
	appos1 = Electric.portPosition(ap1, apport1);
	ap2 = rompln;
	apport2 = rompgndc;
	appos2 = Electric.portPosition(ap2, apport2);
	Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),rom);

	apport2 = (Electric.PortProto)Electric.getVal(romp, "gnd");
	
// decnout, decpin
	for (int i=0; i<words; i++) {
		ap1 = pplane;
		apport1 = decpout[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = nplane;
		apport2 = decnin[i];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
	}

	for (int i=0; i<words; i++) {
		ap1 = nplane;
		apport1 = decnout[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = rompln;
		apport2 = rompin[i];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
	}

///////connect rompgnd to invgnd
	if (folds > 1) {
		for (int i=0; i<romarray.length/folds; i++) {
			ap1 = invpln;
			apport1 = invgnd[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = rompln;
			apport2 = rompgnd[i*folds/2];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
	} else {
		for (int i=0; i<romarray.length/(2*folds); i++) {
			ap1 = invpln;
			apport1 = invgnd[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = rompln;
			apport2 = rompgnd[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
	}

///////connect top ininv1 to ininv2
	for (int i=0; i<bits; i++) {
		ap1 = ininvtop1;
		apport1 = ivttop[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = ininvtop2;
		apport2 = ivttop[i];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
	}

///////connect top ininv1 to ndecoder
	for (int i=0; i<bits; i++) {
		ap1 = ininvtop1;
		apport1 = ivtbot[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = pplane;
		apport2 = decpbit[i*2];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
		ap1 = ininvtop1;
		apport1 = ivtbar[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = pplane;
		apport2 = decpbit[(i*2)+1];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
	}

///////connect top ininv2 to pdecoder
	for (int i=0; i<bits; i++) {
		ap1 = ininvtop2;
		apport1 = ivtbot[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = nplane;
		apport2 = decnbit[i*2];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
		ap1 = ininvtop2;
		apport1 = ivtbar[i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = nplane;
		apport2 = decnbit[(i*2)+1];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
	}


////////connect two top decoder inverterplanes and decoder together (vdd)
	ap1 = ininvtop1;
	apport1 = ivtvdd;
	appos1 = Electric.portPosition(ap1, apport1); 
	ap2 = ininvtop2;
	apport2 = ivtvdd;
	appos2 = Electric.portPosition(ap2, apport2);
	ap3 = pplane;
	apport3 = decpvdd;
	appos3 = Electric.portPosition(ap3, apport3);
	Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),rom);
	Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap3, apport3, appos3[0].intValue(),
						appos3[1].intValue(),rom);
	
////////connect two top decoder inverterplanes and romplane together (gnd)
	ap1 = ininvtop1;
	apport1 = ivtgnd;
	appos1 = Electric.portPosition(ap1, apport1); 
	ap2 = ininvtop2;
	apport2 = ivtgnd;
	appos2 = Electric.portPosition(ap2, apport2);
	ap3 = rompln;
	apport3 = rompgndc;
	appos3 = Electric.portPosition(ap3, apport3);

	Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),rom);
	Electric.newArcInst(m1arc, 4*lambda, 0, ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(), ap3, apport3, appos3[0].intValue(),
						appos3[1].intValue(),rom);
	p = Electric.newPortProto(rom, ap2, apport2, ("gnd"));
	createExport(p, GNDPORT);


////////connect decoder inverter vdd to rom vdd
	ap1 = ininvtop2;
	apport1 = ivtvdd;
	appos1 = Electric.portPosition(ap1, apport1); 
	ap2 = rompln;
	apport2 = rompvdd;
	appos2 = Electric.portPosition(ap2, apport2); 

	Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),rom);
	
	//begin (folds > 1)
	if (folds > 1) {
		decoderpmos(lambda,foldbits,dpm,bot);
		decodernmos(lambda,foldbits,dnm,bot);
		ininverterplane(lambda,foldbits,invb, bot,bits);
		muxplane(lambda,folds, romarray.length, mp);

		//////////////decodernmosmux
		Electric.NodeProto decpmux = Electric.getNodeProto(dpm+"{lay}");
		int[] decpmuxbox =
			{((Integer)Electric.getVal(decpmux, "lowx")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(decpmux, "highx")).intValue()+lambda/2,
			 ((Integer)Electric.getVal(decpmux, "lowy")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(decpmux, "highy")).intValue()+lambda/2};
	 	Electric.PortProto[] decpmuxin = new Electric.PortProto[folds];
	 	Electric.PortProto[] decpmuxout = new Electric.PortProto[folds];
	 	Electric.PortProto[] decpmuxbit = new Electric.PortProto[2*foldbits];
	 	Electric.PortProto decpmuxvdd = 
			(Electric.PortProto)Electric.getVal(decpmux,"vdd");
		Electric.PortProto decpmuxvddb =
			(Electric.PortProto)Electric.getVal(decpmux, "vddb");

		for (int i=0; i<folds; i++) {
			decpmuxin[i] = (Electric.PortProto)Electric.getVal(decpmux, "wordin"+i);
			decpmuxout[i] = (Electric.PortProto)Electric.getVal(decpmux, "word"+i);
		}
		for (int i=0; i<foldbits; i++) {
			decpmuxbit[2*i] = (Electric.PortProto)Electric.getVal(decpmux, "bot_in"+i);
			decpmuxbit[(2*i)+1] =
				(Electric.PortProto)Electric.getVal(decpmux, "bot_in"+i+"_b");
		}

		//////////////decoderpmosmux
		Electric.NodeProto decnmux = Electric.getNodeProto(dnm+"{lay}");
		int[] decnmuxbox =
			{((Integer)Electric.getVal(decnmux, "lowx")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(decnmux, "highx")).intValue()+lambda/2,
			 ((Integer)Electric.getVal(decnmux, "lowy")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(decnmux, "highy")).intValue()+lambda/2};
	 	Electric.PortProto[] decnmuxout = new Electric.PortProto[folds];
	 	Electric.PortProto[] decnmuxin = new Electric.PortProto[folds];
	 	Electric.PortProto[] decnmuxbit = new Electric.PortProto[2*foldbits];
		for (int i=0; i<folds; i++) {
			decnmuxin[i] = (Electric.PortProto)Electric.getVal(decnmux, "mid"+i);
			decnmuxout[i] = (Electric.PortProto)Electric.getVal(decnmux, "word"+i);
		}
		for (int i=0; i<foldbits; i++) {
			decnmuxbit[2*i] = (Electric.PortProto)Electric.getVal(decnmux, "bot_in"+i);
			decnmuxbit[(2*i)+1] =
				(Electric.PortProto)Electric.getVal(decnmux, "bot_in"+i+"_b");
		}
		
		//////////////////////muxplane
		Electric.NodeProto muxp = Electric.getNodeProto(mp+"{lay}");
		int[] muxpbox =
			{((Integer)Electric.getVal(muxp, "lowx")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(muxp, "highx")).intValue()+lambda/2,
			 ((Integer)Electric.getVal(muxp, "lowy")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(muxp, "highy")).intValue()+lambda/2};
		Electric.PortProto[] muxin = new Electric.PortProto[romarray.length];
		Electric.PortProto[] muxout = new Electric.PortProto[romarray.length/folds];
		Electric.PortProto[] muxsel = new Electric.PortProto[folds];
		for (int i=0; i<romarray.length; i++) {
			muxin[i] = (Electric.PortProto)Electric.getVal(muxp, ("muxin"+i));
		}
		for (int i=0; i<romarray.length/folds; i++) {
			muxout[i] = (Electric.PortProto)Electric.getVal(muxp, ("muxout"+i));
		}
		for (int i=0; i<folds; i++) {
			muxsel[i] = (Electric.PortProto)Electric.getVal(muxp, ("sel"+i));
		}

		//////////////////////ininverterplane bottom
		Electric.NodeProto ininvbp = Electric.getNodeProto(invb+"{lay}");
		int[] ininvbpbox =
			{((Integer)Electric.getVal(ininvbp, "lowx")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(ininvbp, "highx")).intValue()+lambda/2,
			 ((Integer)Electric.getVal(ininvbp, "lowy")).intValue()-lambda/2,
			 ((Integer)Electric.getVal(ininvbp, "highy")).intValue()+lambda/2};
		Electric.PortProto[] ivbtop  = new Electric.PortProto[foldbits];
		Electric.PortProto[] ivbbot = new Electric.PortProto[foldbits];
		Electric.PortProto[] ivbbar = new Electric.PortProto[foldbits];
		Electric.PortProto ivbvdd =
			(Electric.PortProto)Electric.getVal(ininvbp, ("vdd"));
		Electric.PortProto ivbgnd =
			(Electric.PortProto)Electric.getVal(ininvbp, ("gnd"));
		for (int i=0; i<foldbits; i++) {
			ivbtop[i] = (Electric.PortProto)Electric.getVal(ininvbp, ("in_top"+i));
			ivbbot[i] = (Electric.PortProto)Electric.getVal(ininvbp, ("in_bot"+i));
			ivbbar[i] = (Electric.PortProto)Electric.getVal(ininvbp, ("in_b"+i));
		}

		Electric.NodeInst muxpln =
			Electric.newNodeInst(muxp, muxpbox[0]+rompoffset, muxpbox[1]+rompoffset,
								 muxpbox[2]+muxpoffsety, muxpbox[3]+muxpoffsety,
								 0, 2700, rom);
		Electric.NodeInst pplnmx =
			Electric.newNodeInst(decpmux, decpmuxbox[0]+foldoffsetx,
								 decpmuxbox[1]+foldoffsetx,
								 decpmuxbox[2]+muxpoffsety+foldoffsety,
								 decpmuxbox[3]+muxpoffsety+foldoffsety, 0, 0, rom);
		Electric.NodeInst nplnmx =
			Electric.newNodeInst(decnmux, decnmuxbox[0]+foldoffsetx+offset,
								 decnbox[1]+foldoffsetx+offset,
								 decnmuxbox[2]+muxpoffsety+foldoffsety,
								 decnmuxbox[3]+muxpoffsety+foldoffsety, 0, 0, rom);
		Electric.NodeInst ininvbot1 =
			Electric.newNodeInst(ininvbp,ininvbpbox[0]+foldoffsetx,
								 ininvbpbox[1]+foldoffsetx, ininvbpbox[2]+invpoffsety,
								 ininvbpbox[3]+invpoffsety, 0, 0, rom);
		Electric.NodeInst ininvbot2 =
			Electric.newNodeInst(ininvbp, ininvbpbox[0]+foldoffsetx+offset,
								 ininvbpbox[1]+foldoffsetx+offset,
								 ininvbpbox[2]+invpoffsety,
								 ininvbpbox[3]+invpoffsety, 0, 0, rom);

		for (int i=0; i<foldbits; i++) {
			ap1 = ininvbot1;
			apport1 = ivbbot[i];
			p = Electric.newPortProto(rom, ap1, apport1, ("colsel"+i));
			createExport(p, INPORT);
		}

		ap1 = nplane;
		apport1 = (Electric.PortProto)Electric.getVal(decn, "gnd");
		appos1 = Electric.portPosition(ap1, apport1);
		ap3 = pplnmx;
		apport3 = decpmuxvdd;
		appos3 = Electric.portPosition(ap3, apport3);
		ap4 = pplane;
		apport4 = decpvddb;
		appos4 = Electric.portPosition(ap4, apport4);
		Electric.newArcInst(m1arc, 4*lambda, 0, ap4, apport4, appos4[0].intValue(),
							appos4[1].intValue(), ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),rom);

		ap3 = nplnmx;
		apport3 = (Electric.PortProto)Electric.getVal(decnmux, "gnd");
		appos3 = Electric.portPosition(ap3, apport3);

		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),rom);

		// decnmuxout, decpmuxin
		for (int i=0; i<folds; i++) {
			ap1 = pplnmx;
			apport1 = decpmuxout[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = nplnmx;
			apport2 = decnmuxin[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}

		for (int i=0; i<folds; i++) {
			ap1 = nplnmx;
			apport1 = decnmuxout[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = muxpln;
			apport2 = muxsel[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
		
///////connect rompout to muxin
		for (int i=0; i<romarray.length; i++) {
			ap1 = rompln;
			apport1 = rompout[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = muxpln;
			apport2 = muxin[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
///////connect muxout to invin
		for (int i=0; i<romarray.length/folds; i++) {
			ap1 = invpln;
			apport1 = invin[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = muxpln;
			apport2 = muxout[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
		
		///////connect bot ininv1 to ininv2
		for (int i=0; i<foldbits; i++) {
			ap1 = ininvbot1;
			apport1 = ivbbot[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = ininvbot2;
			apport2 = ivbbot[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
	
		///////connect bot ininv1 to nmuxdecoder
		for (int i=0; i<foldbits; i++) {
			ap1 = ininvbot1;
			apport1 = ivbtop[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = pplnmx;
			apport2 = decpmuxbit[i*2];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2, appos2[0].intValue(),
								appos2[1].intValue(),rom);
			ap1 = ininvbot1;
			apport1 = ivbbar[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = pplnmx;
			apport2 = decpmuxbit[(i*2)+1];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
		///////connect bot ininv2 to pmuxdecoder
		for (int i=0; i<foldbits; i++) {
			ap1 = ininvbot2;
			apport1 = ivbtop[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = nplnmx;
			apport2 = decnmuxbit[i*2];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
			ap1 = ininvbot2;
			apport1 = ivbbar[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = nplnmx;
			apport2 = decnmuxbit[(i*2)+1];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}

	////////connect two mux decoder inverterplanes and mux decoder together (vdd)
		ap1 = ininvbot1;
		apport1 = ivbvdd;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = ininvbot2;
		apport2 = ivbvdd;
		appos2 = Electric.portPosition(ap2, apport2); 
		ap3 = pplnmx;
		apport3 = decpmuxvddb;
		appos3 = Electric.portPosition(ap3, apport3);
		
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);

		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),rom);

	////////connect two mux decoder inverterplanes and inverterplane together (gnd)
		ap1 = ininvbot1;
		apport1 = ivbgnd;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = ininvbot2;
		apport2 = ivbgnd;
		appos2 = Electric.portPosition(ap2, apport2);
		ap3 = invpln;
		apport3 = invgndc;
		appos3 = Electric.portPosition(ap3, apport3);
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
		Electric.newArcInst(m2arc, 4*lambda, 0, ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);

		////////connect mux decoder to inverter vdd
		ap1 = invpln;
		apport1 = invvddc;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = ininvbot2;
		apport2 = ivbvdd;
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
		

	}
	//end (folds > 1)

	//begin (folds == 1)
	if (folds == 1) {
		for (int i=0; i<romarray.length; i++) {
			ap1 = invpln;
			apport1 = invin[i];
			appos1 = Electric.portPosition(ap1, apport1); 
			ap2 = rompln;
			apport2 = rompout[i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2, appos2[0].intValue(),
								appos2[1].intValue(),rom);
		}
	
		//connect vdd of decoderpmos to vdd of inverterplane
		Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
		Electric.PortProto m1m2cport =
			(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
		int[] m1m2cbox = {-5*lambda/2, 5*lambda/2, -5*lambda/2, 5*lambda/2}; 

		Electric.NodeInst vddbot = new Electric.NodeInst();
		
		int vddoffsetx = offset - 4*lambda;
		int vddoffsety = invpoffsety - 26*lambda;

		vddbot =
			Electric.newNodeInst(m1m2c,m1m2cbox[0]+vddoffsetx,m1m2cbox[1]+vddoffsetx,
								 m1m2cbox[2]+vddoffsety,m1m2cbox[3]+vddoffsety,
								 0,0,rom);

		ap1 = invpln;
		apport1 = invvddc;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = vddbot;
		apport2 = m1m2cport;
		appos2 = Electric.portPosition(ap2, apport2); 
		ap3 = pplane;
		apport3 = decpvddb;
		appos3 = Electric.portPosition(ap3, apport3); 
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2, appos2[0].intValue(),
							appos2[1].intValue(),rom);
		Electric.newArcInst(m1arc, 4*lambda, 0, ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),ap3,apport3, appos3[0].intValue(),
							appos3[1].intValue(),rom);
		
	
	
	}
	
	
	return 1;
	}

////////////////////romplane start
public static void romplane(int lambda, int romarray[][], String rp)
{
	int i, m, o;
	int x, y;
	Electric.NodeInst ap1, ap2, ap3, ap4, gnd1, gnd2, intgnd;
	Electric.PortProto p, apport1, apport2, apport3, apport4, gndport1, gndport2,
					   intgndport;
	Integer[] appos1, appos2, appos3, appos4, gndpos1, gndpos2, intgndpos;

	int inputs = romarray[0].length;
	int wordlines = romarray.length;
	
	Electric.NodeInst[][] andtrans = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[] pulluptrans = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst[] nwellc = new Electric.NodeInst[(wordlines+2)/2];
	Electric.NodeInst[][] minpins = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[][] diffpins = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[][] gndpins = new Electric.NodeInst[wordlines/2][inputs+2];
	Electric.NodeInst[] gnd_2pins = new Electric.NodeInst[wordlines/2];
	Electric.NodeInst[] m1polypins = new Electric.NodeInst[inputs+2];
	Electric.NodeInst[] m1m2pins = new Electric.NodeInst[inputs+2];
	Electric.NodeInst[] m1m2_2pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst[] m1m2_3pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst[] m1m2_4pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst[] mpac_1pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst[] mpac_2pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst gndpex[] = new Electric.NodeInst[1];
	Electric.NodeInst gndm1ex[] = new Electric.NodeInst[1];
	Electric.NodeInst gnd1pin = new Electric.NodeInst();
	Electric.NodeInst vdd1pin = new Electric.NodeInst();
	Electric.NodeInst vdd2pin = new Electric.NodeInst();

	Electric.PortProto[] nwellcports = new Electric.PortProto[(wordlines+2)/2];
	Electric.PortProto[][] minports = new Electric.PortProto[wordlines+2][inputs+2];
	Electric.PortProto[][] gndports = new Electric.PortProto[wordlines/2][inputs+2];
	Electric.PortProto[] gnd_2ports = new Electric.PortProto[wordlines/2];
	Electric.PortProto[][] diffports = new Electric.PortProto[wordlines/2][inputs+2];
	Electric.PortProto[] m1polyports = new Electric.PortProto[inputs+2];
	Electric.PortProto[] m1m2ports = new Electric.PortProto[inputs+2];
	Electric.PortProto[] m1m2_2ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto[] m1m2_3ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto[] m1m2_4ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto[] mpac_1ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto[] mpac_2ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto gndpexport[] = new Electric.PortProto[1];
	Electric.PortProto gndm1export[] = new Electric.PortProto[1];
	Electric.PortProto gnd1port = new Electric.PortProto();
	Electric.PortProto vdd1port = new Electric.PortProto();
	Electric.PortProto vdd2port = new Electric.PortProto();

	/* get pointers to primitives */
	Electric.NodeProto nmos = Electric.getNodeProto("N-Transistor");
	Electric.PortProto nmosg1port = Electric.getPortProto(nmos, "n-trans-poly-right");
	Electric.PortProto nmosg2port = Electric.getPortProto(nmos, "n-trans-poly-left");
	Electric.PortProto nmosd1port = Electric.getPortProto(nmos, "n-trans-diff-top");
	Electric.PortProto nmosd2port = Electric.getPortProto(nmos, "n-trans-diff-bottom");
	int[] nmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};
				
	Electric.NodeProto pmos = Electric.getNodeProto("P-Transistor");
	Electric.PortProto pmosg1port = Electric.getPortProto(pmos, "p-trans-poly-right");
	Electric.PortProto pmosg2port = Electric.getPortProto(pmos, "p-trans-poly-left");
	Electric.PortProto pmosd1port = Electric.getPortProto(pmos, "p-trans-diff-top");
	Electric.PortProto pmosd2port = Electric.getPortProto(pmos, "p-trans-diff-bottom");
	int bbb = 15; 
	int ccc = 23;
	int[] pmosbox = {-bbb*lambda/2, bbb*lambda/2, -ccc*lambda/2, ccc*lambda/2};

	Electric.NodeProto ppin = Electric.getNodeProto("Polysilicon-1-Pin");
	Electric.PortProto ppinport =
		(Electric.PortProto)Electric.getVal(ppin, "firstPortProto");
	int[] ppinbox = {((Integer)Electric.getVal(ppin, "lowx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "lowy")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highy")).intValue()};
	
	Electric.NodeProto napin = Electric.getNodeProto("N-Active-Pin");
	
	Electric.NodeProto m1pin = Electric.getNodeProto("Metal-1-Pin");
	Electric.PortProto m1pinport =
		(Electric.PortProto)Electric.getVal(m1pin, "firstPortProto");
	int[] m1pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m2pin = Electric.getNodeProto("Metal-2-Pin");
	Electric.PortProto m2pinport =
		(Electric.PortProto)Electric.getVal(m2pin, "firstPortProto");
	int[] m2pinbox = {((Integer)Electric.getVal(m2pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m2pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m2pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m2pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto diffpin = Electric.getNodeProto("Active-Pin");
	Electric.PortProto diffpinport =
		(Electric.PortProto)Electric.getVal(diffpin, "firstPortProto");
	int[] diffpinbox =
		{((Integer)Electric.getVal(diffpin, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(diffpin, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highy")).intValue()+lambda/2};
				
	Electric.NodeProto nwnode = Electric.getNodeProto("N-Well-Node");
	int[] nwnodebox =
		{((Integer)Electric.getVal(nwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(nwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto pwnode = Electric.getNodeProto("P-Well-Node");
	int[] pwnodebox =
		{((Integer)Electric.getVal(pwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(pwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highy")).intValue()+lambda/2};
				
	Electric.NodeProto psnode = Electric.getNodeProto("P-Select-Node");
	int[] psnodebox =
		{((Integer)Electric.getVal(psnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(psnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(psnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(psnode, "highy")).intValue()+lambda/2};	   
	
	Electric.NodeProto mnac = Electric.getNodeProto("Metal-1-N-Active-Con");
	Electric.PortProto mnacport =
		(Electric.PortProto)Electric.getVal(mnac, "firstPortProto");
	int aaa = 17;
	int[] mnacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};
	
	Electric.NodeProto mpac = Electric.getNodeProto("Metal-1-P-Active-Con");
	Electric.PortProto mpacport =
		(Electric.PortProto)Electric.getVal(mpac, "firstPortProto");
	int[] mpacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};

	Electric.NodeProto mpwc = Electric.getNodeProto("Metal-1-P-Well-Con");
	Electric.PortProto mpwcport =
		(Electric.PortProto)Electric.getVal(mpwc, "firstPortProto");
	int[] mpwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};
	
	Electric.NodeProto mnwc = Electric.getNodeProto("Metal-1-N-Well-Con");
	Electric.PortProto mnwcport =
		(Electric.PortProto)Electric.getVal(mnwc, "firstPortProto");
	int nwellx = 29;
	int nwelly = 17;
	int[] mnwcbox ={-nwellx*lambda/2,nwellx*lambda/2,-nwelly*lambda/2,nwelly*lambda/2};

	Electric.NodeProto mpc = Electric.getNodeProto("Metal-1-Polysilicon-1-Con");
	Electric.PortProto mpcport =
		(Electric.PortProto)Electric.getVal(mpc, "firstPortProto");
	int mx = 5;
	int[] mpcbox = {-mx*lambda/2, mx*lambda/2, -mx*lambda/2, mx*lambda/2}; 

	Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
	Electric.PortProto m1m2cport =
		(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
	int[] m1m2cbox = {-mx*lambda/2, mx*lambda/2, -mx*lambda/2, mx*lambda/2}; 
	
	Electric.NodeProto nsnode = Electric.getNodeProto("N-Select-Node");
	int nselectx = 8; 
	int nselecty = 8;
	int[] nsnodebox =
		{-nselectx*lambda/2, nselectx*lambda/2, -nselecty*lambda/2, nselecty*lambda/2};

	Electric.ArcProto parc = Electric.getArcProto("Polysilicon-1");
	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	Electric.ArcProto ndiffarc = Electric.getArcProto("N-Active");
	Electric.ArcProto pdiffarc = Electric.getArcProto("P-Active");

	/* create a cell called "romplane{lay}" in the current library */
	Electric.NodeProto romplane =
		Electric.newNodeProto(rp+"{lay}", Electric.curLib());

	Electric.NodeInst pwellnode =
		Electric.newNodeInst(pwnode,-4*lambda,(8*lambda*(inputs+2)),
									-4*lambda,3*8*lambda*(wordlines)/2,0,0,romplane);
	
	int ptranssize = 20;	
	
	Electric.NodeInst pselectnode =
		Electric.newNodeInst(psnode,-28*lambda,(ptranssize-28)*lambda,4*lambda,
							 (4+3*8*wordlines/2)*lambda,0,0,romplane);
	Electric.NodeInst nselectnode =
		Electric.newNodeInst(nsnode,0*lambda,(8*lambda*inputs),4*lambda,
							 (4+3*8*wordlines/2)*lambda,0,0,romplane);
	Electric.NodeInst nwellnode =
		Electric.newNodeInst(nwnode,-38*lambda,(ptranssize-38)*lambda,20*lambda,
							 (4+3*8*wordlines/2)*lambda,0,0,romplane);

	// Create instances of objects on rom plane
	x = 0;
	for (i=0; i<inputs+1; i++) {
		x += 8*lambda;
		y = 0;
		if (i < inputs)  {
			andtrans[0][i] =
				Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x, ppinbox[2],
									 ppinbox[3], 0, 0, romplane);
			m1polypins[i] =
				Electric.newNodeInst(mpc, mpcbox[0]+x, mpcbox[1]+x, mpcbox[2],
									 mpcbox[3], 0, 0, romplane);
			m1m2pins[i] =
				Electric.newNodeInst(m1m2c, m1m2cbox[0]+x, m1m2cbox[1]+x,
									 m1m2cbox[2], m1m2cbox[3], 0, 0, romplane);
			ap1 = m1m2pins[i];
			apport1 = m1m2cport;
			p = Electric.newPortProto(romplane,ap1,apport1,("wordline_"+(inputs-i-1)));
			createExport(p, INPORT);
		}
		for (m=0; m<wordlines; m++) {
			y += 8*lambda;
			if (m%2 == 1) {
				if (i%2 == 1) {
					gndpins[m/2][i] =
						Electric.newNodeInst(mnac, mnacbox[0]+x-4*lambda,
											 mnacbox[1]+x-4*lambda, mnacbox[2]+y,
											 mnacbox[3]+y, 0, 0, romplane);
					gndports[m/2][i] = mnacport;
				} else {
					if ( i == inputs) {
						gndpins[m/2][i] =
							Electric.newNodeInst(mpwc, mpwcbox[0]+x-4*lambda,
												 mpwcbox[1]+x-4*lambda, mpwcbox[2]+y,
												 mpwcbox[3]+y, 0, 0, romplane);
						gndports[m/2][i] = mpwcport;
					} else {
						gndpins[m/2][i] =
							Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
							m1pinbox[1]+x-4*lambda, m1pinbox[2]+y, m1pinbox[3]+y,
							0, 0, romplane);
						gndports[m/2][i] = m1pinport;
					}
				}
				if ( i == 0 ) {
					gnd_2pins[m/2] =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x-12*lambda,
							m1pinbox[1]+x-12*lambda, m1pinbox[2]+y, m1pinbox[3]+y,
							0, 0, romplane);
					gnd_2ports[m/2] = m1pinport;
					if ( m == 1 )  {
					gndm1ex[m/2] =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x-12*lambda,
											 m1pinbox[1]+x-12*lambda,
											 m1pinbox[2]+y-16*lambda,
											 m1pinbox[3]+y-16*lambda, 0, 0, romplane);
					gndm1export[m/2] = m1pinport;
					gnd1pin =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x-8*lambda,
											 m1pinbox[1]+x-8*lambda,
											 m1pinbox[2]+y-16*lambda,
											 m1pinbox[3]+y-16*lambda, 0, 0, romplane);
					gnd1port = m1pinport;
					vdd2pin =
						Electric.newNodeInst(m2pin, m2pinbox[0]+x-32*lambda,
											 m2pinbox[1]+x-32*lambda,
											 m2pinbox[2]+y-16*lambda,
											 m2pinbox[3]+y-16*lambda, 0, 0, romplane);
					vdd2port = m2pinport;
					}
				}
				y+= 8*lambda;
			}
			if (i < inputs) {
				if (romarray[m][i] == 1) {
					// create a transistor
					andtrans[m+1][i] =
						Electric.newNodeInst(nmos, nmosbox[0]+x, nmosbox[1]+x,
											 nmosbox[2]+y, nmosbox[3]+y,1,0,romplane);
				} else {
					andtrans[m+1][i] =
						Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
											 ppinbox[2]+y,ppinbox[3]+y,0,0,romplane);
				}
			}
			boolean transcont = false;
			if (i < inputs) transcont = (romarray[m][i] == 1);
			if (i > 1) transcont |= (romarray[m][i-1] == 1);
			if (i%2 == 0 && transcont){
				minpins[m][i] =
					Electric.newNodeInst(mnac, mnacbox[0]+x-4*lambda,
										 mnacbox[1]+x-4*lambda, mnacbox[2]+y,
										 mnacbox[3]+y, 0, 0, romplane);			
				diffpins[m][i] =
					Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
										 m1pinbox[1]+x-4*lambda, m1pinbox[2]+y,
										 m1pinbox[3]+y, 0, 0, romplane);
				minports[m][i] = mnacport;
			} else {
				minpins[m][i] =
					Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
										 m1pinbox[1]+x-4*lambda, m1pinbox[2]+y,
										 m1pinbox[3]+y, 0, 0, romplane);
				if ((transcont) || ((i==1) && (romarray[m][0] == 1))) {
					diffpins[m][i] =
						Electric.newNodeInst(diffpin, diffpinbox[0]+x-4*lambda,
											 diffpinbox[1]+x-4*lambda, diffpinbox[2]+y,
											 diffpinbox[3]+y, 0, 0, romplane);		
				} else {
					diffpins[m][i] =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
							m1pinbox[1]+x-4*lambda,m1pinbox[2]+y,m1pinbox[3]+y,
							0, 0, romplane);
				}					
				minports[m][i] = m1pinport;
			}
			if (i == inputs) {
				ap1 = minpins[m][i];
				apport1 = minports[m][i];
				p = Electric.newPortProto(romplane, ap1, apport1, ("out_"+m));
				createExport(p, OUTPORT);
			}
			if (i == 0)  {
				if ( m%2 ==1 ) {
					nwellc[m/2] =
						Electric.newNodeInst(mnwc, m1m2cbox[0]+x-46*lambda,
											 mnwcbox[1]+x-46*lambda, mnwcbox[2]+y,
											 mnwcbox[3]+y, 0, 0, romplane);
					nwellcports[m/2] = mnwcport;
				}
				m1m2_2pins[m] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-4*lambda,
										 m1m2cbox[1]+x-4*lambda, m1m2cbox[2]+y,
										 m1m2cbox[3]+y, 0, 0, romplane);
				m1m2_2ports[m] = m1m2cport;
				m1m2_3pins[m] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-20*lambda,
										 m1m2cbox[1]+x-20*lambda, m1m2cbox[2]+y,
										 m1m2cbox[3]+y, 0, 0, romplane);
				m1m2_3ports[m] = m1m2cport;
				mpac_1pins[m] =
					Electric.newNodeInst(mpac, mpacbox[0]+x-20*lambda,
										 mpacbox[1]+x-20*lambda, mpacbox[2]+y,
										 mpacbox[3]+y, 0, 0, romplane);
				mpac_1ports[m] = mpacport;
				pulluptrans[m] =
					Electric.newNodeInst(pmos, pmosbox[0]+x-26*lambda,
										 pmosbox[1]+x-26*lambda, pmosbox[2]+y,
										 pmosbox[3]+y, 1, 0, romplane);
				mpac_2pins[m] =
					Electric.newNodeInst(mpac, mpacbox[0]+x-32*lambda,
										 mpacbox[1]+x-32*lambda, mpacbox[2]+y,
										 mpacbox[3]+y, 0, 0, romplane);
				mpac_2ports[m] = mpacport;
				m1m2_4pins[m] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-32*lambda,
										 m1m2cbox[1]+x-32*lambda, m1m2cbox[2]+y,
										 m1m2cbox[3]+y, 0, 0, romplane);
				m1m2_4ports[m] = m1m2cport;
				if ( m == 0) {
					gndpex[m] =
						Electric.newNodeInst(mpc, mpcbox[0]+x-26*lambda,
											 mpcbox[1]+x-26*lambda,
											 mpcbox[2]+y-8*lambda,
											 mpcbox[3]+y-8*lambda, 0, 0, romplane);
					gndpexport[m] = mpcport;			
				}
			} 
		}
	}

	// finished placing objects, start wiring arcs

	for (i=0; i<inputs; i++) {
		ap1 = andtrans[0][i];
		apport1 = ppinport;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = m1polypins[i];
		apport2 = mpcport;
		appos2 = Electric.portPosition(ap2, apport2); 
		ap3 = m1m2pins[i];
		apport3 = m1m2cport;
		appos3 = Electric.portPosition(ap3, apport3); 
		Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
		Electric.newArcInst(m1arc, 4*lambda, 0, ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(), ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),romplane);
	}

	for (i=0; i<inputs; i++){
		ap1 = andtrans[0][i];
		apport1 = ppinport;
		appos1 = Electric.portPosition(ap1, apport1); 
		for (m=1; m<wordlines+1; m++) {
			ap2 = andtrans[m][i];
			if (romarray[m-1][i] == 1) {
				apport2 = nmosg1port;
				apport3 = nmosg2port;
			} else {
				apport2 = ppinport;
				apport3 = ppinport;
			}
			appos2 = Electric.portPosition(ap2, apport2);
			appos3 = Electric.portPosition(ap2, apport3);
			Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),romplane);
			ap1 = ap2;
			apport1 = apport3;
			appos1 = appos3;
		}
	}
	// connect m1 wordline lines
	for (m=0; m<wordlines; m++) {
		ap1 = minpins[m][0];
		apport1 = minports[m][0];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (i=1; i<inputs+1; i++) {
			ap2 = minpins[m][i];
			apport2 = minports[m][i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),romplane);
			ap1 = ap2;
			apport1 = apport2;
			appos1 = appos2;
		}
	}
	
	// connect transistors to wordline lines
	for (m=0; m<wordlines; m++) {
		for (i=0; i<inputs; i++) {
			if (romarray[m][i] == 1) {
				// connect transistor
				ap1 = andtrans[m+1][i];
				gnd1 = ap1;
				if (i%2 == 0) {
					apport1 = nmosd1port;
					gndport1 = nmosd2port;
					ap2 = minpins[m][i];
					gnd2 = gndpins[m/2][i+1];
					intgnd = diffpins[m][i+1];
				} else {
					apport1 = nmosd2port;
					gndport1 = nmosd1port;
					ap2 = minpins[m][i+1];
					gnd2 = gndpins[m/2][i];
					intgnd = diffpins[m][i];
				}
				appos1 = Electric.portPosition(ap1, apport1);
				gndpos1 = Electric.portPosition(gnd1, gndport1);
				apport2 = mnacport;
				appos2 = Electric.portPosition(ap2, apport2);
				gndport2 = mnacport;
				gndpos2 = Electric.portPosition(gnd2, gndport2);
				intgndport = diffpinport;
				intgndpos = Electric.portPosition(intgnd, intgndport);
				Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, apport1,
									appos1[0].intValue(), appos1[1].intValue(),
									ap2, apport2, appos2[0].intValue(),
									appos2[1].intValue(),romplane);
				Electric.newArcInst(ndiffarc, 16*lambda, 0, gnd1, gndport1,
									gndpos1[0].intValue(), gndpos1[1].intValue(),
									intgnd, intgndport, intgndpos[0].intValue(),
									intgndpos[1].intValue(),romplane);
			Electric.newArcInst(ndiffarc, 16*lambda, 0, intgnd, intgndport,
								intgndpos[0].intValue(), intgndpos[1].intValue(),
								gnd2, gndport2, gndpos2[0].intValue(),
								gndpos2[1].intValue(),romplane);
			}
		}
	}
	
	// connect ground lines
	for (m=0; m<wordlines/2; m++) {
		ap1 = gndpins[m][0];
		apport1 = gndports[m][0];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (i=1; i<inputs+1; i++) {
			ap2 = gndpins[m][i];
			apport2 = gndports[m][i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),romplane);
			if (i == inputs) {
				p = Electric.newPortProto(romplane, ap1, apport1, ("romgnd"+m));
				createExport(p, GNDPORT);
			}
			ap1 = ap2;
			apport1 = apport2;
			appos1 = appos2;
		}
	}
	
	// extend the gnd plane
	for (m=0; m<wordlines/2; m++) {
 		ap1 = gndpins[m][0];
		apport1 = gndports[m][0];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = gnd_2pins[m];
		apport2 = gnd_2ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
	}

	// tie up all the gndlines
	ap1 = gnd_2pins[0];
	apport1 = gnd_2ports[0];
	appos1 = Electric.portPosition(ap1, apport1);
	for (m=0; m<wordlines/2; m++) {
		ap2 = gnd_2pins[m];
		apport2 = gnd_2ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
		if (m == (wordlines/2 - 1)) {
			p = Electric.newPortProto(romplane, ap2, apport2, ("gnd"));
			createExport(p, GNDPORT);
		}
	}
	
	ap2 = gndm1ex[0];
	apport2 = gndm1export[0];
	appos2 = Electric.portPosition(ap2, apport2); 
	Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),romplane);
	ap1 = gnd1pin;
	apport1 = gnd1port;
	appos1 = Electric.portPosition(ap1, apport1);
	Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),romplane);
	p = Electric.newPortProto(romplane, ap1, apport1, ("gndc"));
	createExport(p, GNDPORT);

	ap1 = gndpex[0];
	apport1 = gndpexport[0];
	appos1 = Electric.portPosition(ap1, apport1);
	Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),romplane);

	ap2 = pulluptrans[0];
	apport2 = pmosg1port;
	appos2 = Electric.portPosition(ap2, apport2);
	Electric.newArcInst(parc, 3*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),romplane);

	// connect m1m2contact from romplane to m1m2contact before pull-up trans
	for (m=0; m<wordlines; m++) {
		ap1 = minpins[m][0];
		apport1 = minports[m][0];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = m1m2_2pins[m];
		apport2 = m1m2_2ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		ap3 = m1m2_3pins[m];
		apport3 = m1m2_3ports[m];
		appos3 = Electric.portPosition(ap3, apport3); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
		Electric.newArcInst(m2arc, 4*lambda, 0, ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(), ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),romplane);
	}
	
	// connect m1m2contact from romplane to mpac of pull-up trans
	for (m=0; m<wordlines; m++) {
		ap1 = m1m2_3pins[m];
		apport1 = m1m2_3ports[m];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = mpac_1pins[m];
		apport2 = mpac_1ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
	}
	
	//connect pull-up transistors to the mpac
	for (m=0; m<wordlines; m++)  {
		ap1 = pulluptrans[m];
		ap4 = ap1;
		apport4 = pmosd1port;
		apport1 = pmosd2port;
		ap2= mpac_1pins[m];
		ap3= mpac_2pins[m];
		apport2 = mpacport;
		apport3 = mpacport;
		appos1 = Electric.portPosition(ap1, apport1);
		appos4 = Electric.portPosition(ap4, apport4);
		appos2 = Electric.portPosition(ap2, apport2);
		appos3 = Electric.portPosition(ap3, apport3);
		Electric.newArcInst(pdiffarc, 15*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
		Electric.newArcInst(pdiffarc, 15*lambda, 0, ap4, apport4, appos4[0].intValue(),
							appos4[1].intValue(), ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),romplane);
	}
		
	// connect mpac of pull-up trans to m1m2c
	for (m=0; m<wordlines; m++) {
		ap1 = m1m2_4pins[m];
		apport1 = m1m2_4ports[m];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = mpac_2pins[m];
		apport2 = mpac_2ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
	}
	
	// connect mpac of pull-up trans to m1m2c
	for (m=0; m<wordlines; m++) {
		if (m%2 ==1) {
		ap1 = nwellc[m/2];
		apport1 = nwellcports[m/2];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = mpac_2pins[m];
		apport2 = mpac_2ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
		}
	}
	
	// tie up all the vddlines
	ap1 = m1m2_4pins[0];
	apport1 = m1m2_4ports[0];
	appos1 = Electric.portPosition(ap1, apport1);	
	for (m=0; m<wordlines; m++) {
 		ap2 = m1m2_4pins[m];
		apport2 = m1m2_4ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
	}
	
	ap2 = vdd2pin;
	apport2 = vdd2port;
	appos2 = Electric.portPosition(ap2, apport2);
	Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),romplane);

	p = Electric.newPortProto(romplane, ap2, apport2, ("vdd"));
	createExport(p, PWRPORT);

	// connect poly for the pull-up transistor
	for (m=0; m<wordlines-1; m++) {
	 	ap1 = pulluptrans[m];
		apport1 = pmosg2port;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = pulluptrans[m+1];
		apport2 = pmosg1port;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc, 3*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),romplane);
	}
}
////////////////////romplane end

////////////////////decoder nmos
public static void decodernmos(int lambda, int bits, String cellname,
						boolean top) {	

	int[][] romplane = generateplane(bits);
	int i, m, o;
	int x, y;
	Electric.NodeInst ap1, ap2, ap3, gnd1, gnd2, vdd1, vdd2;
	Electric.PortProto p, apport1, apport2, apport3, gndport1, gndport2,
						vddport1, vddport2;
	Integer[] appos1, appos2, appos3, gndpos1, gndpos2, vddpos1, vddpos2;

	int inputs = romplane[0].length;
	int wordlines = romplane.length;
	
	Electric.NodeInst[][] ortrans = new Electric.NodeInst[wordlines+3][inputs+2];
	Electric.NodeInst[][] minpins = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[][] diffpins = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[][] gndpins = new Electric.NodeInst[wordlines/2][inputs+2];
	Electric.NodeInst[][] vddpins = new Electric.NodeInst[wordlines][inputs/2];
	Electric.NodeInst[] pwrpins = new Electric.NodeInst[inputs/2];
	Electric.NodeInst[][] m1m2pins = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[] pwcpins = new Electric.NodeInst[wordlines+2];
	
	
	Electric.PortProto[][] minports = new Electric.PortProto[wordlines+2][inputs+2];
	Electric.PortProto[][] gndports = new Electric.PortProto[wordlines/2][inputs+2];
	Electric.PortProto[][] vddports = new Electric.PortProto[wordlines][inputs/2];
	Electric.PortProto[] pwrports = new Electric.PortProto[inputs/2];
	Electric.PortProto[][] diffports = new Electric.PortProto[wordlines/2][inputs+2];
	Electric.PortProto[][] m1m2ports = new Electric.PortProto[wordlines+2][inputs+2];
	Electric.PortProto[] pwcports = new Electric.PortProto[wordlines+2];


	/* get pointers to primitives */
	Electric.NodeProto nmos = Electric.getNodeProto("N-Transistor");
	Electric.PortProto nmosg1port = Electric.getPortProto(nmos, "n-trans-poly-right");
	Electric.PortProto nmosg2port = Electric.getPortProto(nmos, "n-trans-poly-left");
	Electric.PortProto nmosd1port = Electric.getPortProto(nmos, "n-trans-diff-top");
	Electric.PortProto nmosd2port = Electric.getPortProto(nmos, "n-trans-diff-bottom");
	int[] nmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
 					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};
		
	Electric.NodeProto pmos = Electric.getNodeProto("P-Transistor");
	Electric.PortProto pmosg1port = Electric.getPortProto(pmos, "p-trans-poly-right");
	Electric.PortProto pmosg2port = Electric.getPortProto(pmos, "p-trans-poly-left");
	Electric.PortProto pmosd1port = Electric.getPortProto(pmos, "p-trans-diff-top");
	Electric.PortProto pmosd2port = Electric.getPortProto(pmos, "p-trans-diff-bottom");
	int[] pmosbox = {((Integer)Electric.getVal(pmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(pmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(pmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(pmos, "highy")).intValue()};

	Electric.NodeProto ppin = Electric.getNodeProto("Polysilicon-1-Pin");
	Electric.PortProto ppinport =
		(Electric.PortProto)Electric.getVal(ppin, "firstPortProto");
	int[] ppinbox = {((Integer)Electric.getVal(ppin, "lowx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "lowy")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highy")).intValue()};
	
	Electric.NodeProto napin = Electric.getNodeProto("N-Active-Pin");
	
	Electric.NodeProto m1pin = Electric.getNodeProto("Metal-1-Pin");
	Electric.PortProto m1pinport =
		(Electric.PortProto)Electric.getVal(m1pin, "firstPortProto");
	int[] m1pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m2pin = Electric.getNodeProto("Metal-2-Pin");
	Electric.PortProto m2pinport =
		(Electric.PortProto)Electric.getVal(m2pin, "firstPortProto");
	int[] m2pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	

	Electric.NodeProto nwnode = Electric.getNodeProto("N-Well-Node");
	int[] nwnodebox =
		{((Integer)Electric.getVal(nwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(nwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highy")).intValue()+lambda/2};
	Electric.NodeProto pwnode = Electric.getNodeProto("P-Well-Node");
	int[] pwnodebox =
		{((Integer)Electric.getVal(pwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(pwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highy")).intValue()+lambda/2};
	
	int mx = 5;
	Electric.NodeProto mpc = Electric.getNodeProto("Metal-1-Polysilicon-1-Con");
	Electric.PortProto mpcport =
		(Electric.PortProto)Electric.getVal(mpc, "firstPortProto");
	int[] mpcbox = {-mx*lambda/2, mx*lambda/2, -mx*lambda/2, mx*lambda/2}; 

	Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
	Electric.PortProto m1m2cport =
		(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
	int[] m1m2cbox = {-5*lambda/2, 5*lambda/2, -5*lambda/2, 5*lambda/2}; 

	Electric.NodeProto diffpin = Electric.getNodeProto("Active-Pin");
	Electric.PortProto diffpinport =
		(Electric.PortProto)Electric.getVal(diffpin, "firstPortProto");
	int[] diffpinbox =
		{((Integer)Electric.getVal(diffpin, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(diffpin, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto mnac = Electric.getNodeProto("Metal-1-N-Active-Con");
	Electric.PortProto mnacport =
		(Electric.PortProto)Electric.getVal(mnac, "firstPortProto");
	int aaa = 17;
	int[] mnacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};
	//centers around 6 goes up by multiples of 2

	Electric.NodeProto mpac = Electric.getNodeProto("Metal-1-P-Active-Con");
	Electric.PortProto mpacport =
		(Electric.PortProto)Electric.getVal(mpac, "firstPortProto");
	int[] mpacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};
	//centers around 6 goes up by multiples of 2

	Electric.NodeProto mpwc = Electric.getNodeProto("Metal-1-P-Well-Con");
	Electric.PortProto mpwcport =
		(Electric.PortProto)Electric.getVal(mpwc, "firstPortProto");
	int[] mpwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};

	Electric.NodeProto mnwc = Electric.getNodeProto("Metal-1-N-Well-Con");
	Electric.PortProto mnwcport =
		(Electric.PortProto)Electric.getVal(mnwc, "firstPortProto");
	int[] mnwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};

	Electric.ArcProto parc = Electric.getArcProto("Polysilicon-1");
	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	Electric.ArcProto ndiffarc = Electric.getArcProto("N-Active");
	Electric.ArcProto pdiffarc = Electric.getArcProto("P-Active");

	/* create a cell called cellname+"{lay}" in the current library */
	Electric.NodeProto decn =
		Electric.newNodeProto(cellname+"{lay}", Electric.curLib());


	Electric.NodeProto nsnode = Electric.getNodeProto("N-Select-Node");
	int nselectx = 8; 
	int nselecty = 8;
	int[] nsnodebox =
		{-nselectx*lambda/2, nselectx*lambda/2, -nselecty*lambda/2, nselecty*lambda/2};

	Electric.NodeInst pwellnode =
		Electric.newNodeInst(pwnode,0,(8*lambda*(2*bits+1)),
									0,8*lambda*(wordlines+1),0,0,decn);
	Electric.NodeInst nselectnode =
		Electric.newNodeInst(nsnode,0,(8*lambda*(2*bits+1)),
									0,8*lambda*(wordlines+1),0,0,decn);
	
	// Create instances of objects on decoder nmos plane
	x = 0;
	for (i=0; i<inputs+1; i++) {
		x += 8*lambda;
		y = 0;
		if ( i%2 ==1)	
		{
			x += 0*lambda;
		}
		if (i < inputs)  {
			if ( top == true)   {
				ortrans[0][i] =
					Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
											   ppinbox[2], ppinbox[3], 0, 0, decn);
			}else {    
				ortrans[0][i] =
					Electric.newNodeInst(mpc, mpcbox[0]+x, mpcbox[1]+x,
											  mpcbox[2], mpcbox[3], 0, 0, decn);
			}
		}
		for (m=0; m<wordlines; m++) {
			y += 8*lambda;
			if (i%2 == 1) {
				vddpins[m][i/2] =
					Electric.newNodeInst(mnac, mnacbox[0]+x-4*lambda,
										 mnacbox[1]+x-4*lambda,
										 mnacbox[2]+y, mnacbox[3]+y, 0, 0, decn);
				vddports[m][i/2] = mnacport;
				if (m == (wordlines-1)) {
					pwrpins[i/2] =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
											 m1pinbox[1]+x-4*lambda,
											 m1pinbox[2]+y+(8*lambda),
											 m1pinbox[3]+y+(8*lambda), 0, 0, decn);
					pwrports[i/2] = m1pinport;
				}
			}
			if (i < inputs) {
				if (romplane[m][i] == 1) {
					// create a transistor
					ortrans[m+1][i] =
						Electric.newNodeInst(nmos, nmosbox[0]+x, nmosbox[1]+x,
											 nmosbox[2]+y, nmosbox[3]+y, 1, 0, decn);
				} else {
					ortrans[m+1][i] =
						Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
											 ppinbox[2]+y, ppinbox[3]+y, 0, 0, decn);
				}
				if ( m == wordlines-1) {
					if (top == true) {
							ortrans[m+2][i] =
								Electric.newNodeInst(mpc, mpcbox[0]+x, mpcbox[1]+x,
													 mpcbox[2]+y+16*lambda,
													 mpcbox[3]+y+16*lambda,
													 0, 0, decn);
						}else {
							ortrans[m+2][i] =
								Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
													 ppinbox[2]+y+4*lambda, ppinbox[3]+y+4*lambda,
													 0, 0, decn);	
						}
					}
			}
			boolean transcont = false;
			if (i < inputs) transcont = (romplane[m][i] == 1);
			if (i > 1) transcont |= (romplane[m][i-1] == 1);
			if (i%2 == 0 && transcont){
				minpins[m][i] =
					Electric.newNodeInst(mnac, mnacbox[0]+x-4*lambda,
										 mnacbox[1]+x-4*lambda, mnacbox[2]+y,
										 mnacbox[3]+y, 0, 0, decn);			
				minports[m][i] = mnacport;
				m1m2pins[m][i] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-4*lambda,
										 m1m2cbox[1]+x-4*lambda, m1m2cbox[2]+y,
										 m1m2cbox[3]+y, 0, 0, decn);
				m1m2ports[m][i] = m1m2cport;
			} else {
				minpins[m][i] =
					Electric.newNodeInst(m2pin, m2pinbox[0]+x-4*lambda,
										 m2pinbox[1]+x-4*lambda, m2pinbox[2]+y,
										 m2pinbox[3]+y, 0, 0, decn);
				minports[m][i] = m2pinport;
				if (i==0) {
					m1m2pins[m][i] =
						Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-4*lambda,
											 m1m2cbox[1]+x-4*lambda, m1m2cbox[2]+y,
											 m1m2cbox[3]+y, 0, 0, decn);
					m1m2ports[m][i] = m1m2cport;
				} else {
					m1m2pins[m][i] =
						Electric.newNodeInst(m2pin, m2pinbox[0]+x-4*lambda,
											 m2pinbox[1]+x-4*lambda, m2pinbox[2]+y,
											 m2pinbox[3]+y, 0, 0, decn);
					m1m2ports[m][i] = m2pinport;
				}
			}
			if (i==0) {
				ap1 = m1m2pins[m][i];
				apport1 = m1m2ports[m][i];
				p = Electric.newPortProto(decn, ap1, apport1, ("mid"+m));
				createExport(p, INPORT);
			}
		}
	}

//finished making instances, start making arcs
	ap1 = pwrpins[0];
	apport1 = pwrports[0];
	appos1 = Electric.portPosition(ap1, apport1);
	for (i=1; i<inputs/2; i++) {
		ap2 = pwrpins[i];
		apport2 = pwrports[i];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0,
							ap1, apport1, appos1[0].intValue(), appos1[1].intValue(),
							ap2, apport2, appos2[0].intValue(), appos2[1].intValue(),
							decn);
		ap1 = ap2;
		apport1 = apport2;
		appos1 = appos2;
	}
	p = Electric.newPortProto(decn, ap1, apport1, ("gnd"));
	createExport(p, GNDPORT);

	m = wordlines - 1;
	for (i=0; i<inputs/2; i++) {
		ap1 = vddpins[m][i];
		apport1 = vddports[m][i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = pwrpins[i];
		apport2 = pwrports[i];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0,
							ap1, apport1, appos1[0].intValue(), appos1[1].intValue(),
							ap2, apport2, appos2[0].intValue(), appos2[1].intValue(),
							decn);
	}

	// connect polysilicon gates
	for (i=0; i<inputs; i++)
	{
		ap1 = ortrans[wordlines+1][i];
			if (top == true)  {
				apport1 = mpcport;
			}else {
				apport1 = ppinport;
			}
		appos1 = Electric.portPosition(ap1, apport1); 
		if (i%2 == 0) {
			p = Electric.newPortProto(decn, ap1, apport1, ("top_in"+(i/2)));
			createExport(p, INPORT);			
		} else {
			p = Electric.newPortProto(decn, ap1, apport1, ("top_in"+((i-1)/2)+"_b"));
			createExport(p, INPORT);			
		}
	
		ap1 = ortrans[0][i];
		if (top == true)  {
				apport1 = ppinport;
			}else {
				apport1 = mpcport;
			}
		appos1 = Electric.portPosition(ap1, apport1); 

		if (i%2 == 0) {
			p = Electric.newPortProto(decn, ap1, apport1, ("bot_in"+(i/2)));
			createExport(p, INPORT);
		} else {
			p = Electric.newPortProto(decn, ap1, apport1, ("bot_in"+((i-1)/2)+"_b"));
			createExport(p, INPORT);
		}

		for (m=1; m<wordlines+1; m++) {
			ap2 = ortrans[m][i];
			if (romplane[m-1][i] == 1) {
				apport2 = nmosg1port;
				apport3 = nmosg2port;
			} else {
				apport2 = ppinport;
				apport3 = ppinport;
			}
			appos2 = Electric.portPosition(ap2, apport2);
			appos3 = Electric.portPosition(ap2, apport3);
			Electric.newArcInst(parc, 2*lambda, 0,
								ap1,apport1,appos1[0].intValue(),appos1[1].intValue(),
								ap2,apport2,appos2[0].intValue(),appos2[1].intValue(),
								decn);
			ap1 = ap2;
			apport1 = apport3;
			appos1 = appos3;
		}
		
		ap2 = ortrans[wordlines+1][i];
		if ( top == true) {
			apport2 = mpcport;
			apport3 = mpcport;
		}else {
			apport2 = ppinport;
			apport3 = ppinport;
		}
		appos2 = Electric.portPosition(ap2, apport2);
		appos3 = Electric.portPosition(ap2, apport3);
		Electric.newArcInst(parc, 2*lambda, 0,
							ap1, apport1, appos1[0].intValue(), appos1[1].intValue(),
							ap2, apport2, appos2[0].intValue(), appos2[1].intValue(),
							decn);
	}

	// connect m2 wordline lines
	for (m=0; m<wordlines; m++) {
		ap1 = m1m2pins[m][0];
		apport1 = m1m2ports[m][0];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (i=1; i<inputs+1; i++) {
			ap2 = m1m2pins[m][i];
			apport2 = m1m2ports[m][i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1,
								appos1[0].intValue(), appos1[1].intValue(),
								ap2, apport2, appos2[0].intValue(),
								appos2[1].intValue(),decn);
			ap1 = ap2;
			apport1 = apport2;
			appos1 = appos2;
		}
		p = Electric.newPortProto(decn, ap1, apport1, ("word"+m));
		createExport(p, OUTPORT);
	}

	// connect transistors to wordline lines
	for (m=0; m<wordlines; m++) {
		for (i=0; i<inputs; i++) {
			if (romplane[m][i] == 1) {
				// connect transistor
				ap1 = ortrans[m+1][i];
				vdd1 = ap1;
				if (i%2 == 0) {
					apport1 = nmosd1port;
					vddport1 = nmosd2port;
					ap2 = minpins[m][i];
					ap3 = m1m2pins[m][i];
					vdd2 = vddpins[m][i/2];
				} else {
					apport1 = nmosd2port;
					vddport1 = nmosd1port;
					ap2 = minpins[m][i+1];
					ap3 = m1m2pins[m][i+1];
					vdd2 = vddpins[m][i/2];
				}
				apport2 = mnacport;
				apport3 = m1m2cport;
				vddport2 = mnacport;

				appos1 = Electric.portPosition(ap1, apport1);
				vddpos1 = Electric.portPosition(vdd1, vddport1);
				appos2 = Electric.portPosition(ap2, apport2);
				appos3 = Electric.portPosition(ap3, apport3);
				vddpos2 = Electric.portPosition(vdd2, vddport2);
				
				//ndiffarc size centers around 12 and goes up by multiples of 2
				Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, apport1,
									appos1[0].intValue(), appos1[1].intValue(), ap2,
									apport2, appos2[0].intValue(),
									appos2[1].intValue(),decn);
				Electric.newArcInst(ndiffarc, 16*lambda, 0, vdd1, vddport1,
									vddpos1[0].intValue(), vddpos1[1].intValue(), vdd2,
									vddport2, vddpos2[0].intValue(),
									vddpos2[1].intValue(),decn);

				Electric.newArcInst(m1arc, 4*lambda, 0, ap2, apport2,
									appos2[0].intValue(), appos2[1].intValue(), ap3,
									apport3, appos3[0].intValue(),
									appos3[1].intValue(),decn);
			}
		}
	}

	// connect vdd lines
	for (i=0; i<inputs/2; i++) {
		ap1 = vddpins[0][i];
		apport1 = vddports[0][i];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (m=1; m<wordlines; m++) {
			ap2 = vddpins[m][i];
			apport2 = vddports[m][i];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1,
								appos1[0].intValue(), appos1[1].intValue(),
								ap2, apport2, appos2[0].intValue(),
								appos2[1].intValue(),decn);
			ap1 = ap2;
			apport1 = apport2;
			appos1 = appos2;
		}
	}
}
////////////////////decodernmos end

////////////////////decoderpmos 
public static void decoderpmos(int lambda, int bits, String cellname, boolean top) {

	int[][] romplane = generateplane(bits);
	int i, m, o;
	int x, y;
	Electric.NodeInst ap1, ap2, ap3, apx, apy;
	Electric.PortProto p, apport1, apport2, apport3, apportx, apporty;
	Integer[] appos1, appos2, appos3, apposx, apposy;
	int inputs = romplane[0].length;
	int wordlines = romplane.length;
	
	Electric.NodeInst[][] andtrans = new Electric.NodeInst[wordlines+2][inputs+2];
	Electric.NodeInst[][] minpins = new Electric.NodeInst[wordlines+3][inputs+2];
	Electric.NodeInst[] m1m2pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst[] m2pins = new Electric.NodeInst[wordlines+2];
	Electric.NodeInst vddpin = new Electric.NodeInst();
	Electric.NodeInst vddipin = new Electric.NodeInst();
	Electric.NodeInst vddbpin = new Electric.NodeInst();
	Electric.NodeInst vddcpin = new Electric.NodeInst();

	Electric.PortProto[][] minports = new Electric.PortProto[wordlines+2][inputs+2];
	Electric.PortProto[] m1m2ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto[] m2ports = new Electric.PortProto[wordlines+2];
	Electric.PortProto vddport = new Electric.PortProto();
	Electric.PortProto vddiport = new Electric.PortProto();
	Electric.PortProto vddbport = new Electric.PortProto();
	Electric.PortProto vddcport = new Electric.PortProto();

	/* get pointers to primitives */
	Electric.NodeProto nmos = Electric.getNodeProto("N-Transistor");
	Electric.PortProto nmosg1port = Electric.getPortProto(nmos, "n-trans-poly-right");
	Electric.PortProto nmosg2port = Electric.getPortProto(nmos, "n-trans-poly-left");
	Electric.PortProto nmosd1port = Electric.getPortProto(nmos, "n-trans-diff-top");
	Electric.PortProto nmosd2port = Electric.getPortProto(nmos, "n-trans-diff-bottom");
	int[] nmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};
				
	Electric.NodeProto pmos = Electric.getNodeProto("P-Transistor");
	Electric.PortProto pmosg1port = Electric.getPortProto(pmos, "p-trans-poly-right");
	Electric.PortProto pmosg2port = Electric.getPortProto(pmos, "p-trans-poly-left");
	Electric.PortProto pmosd1port = Electric.getPortProto(pmos, "p-trans-diff-top");
	Electric.PortProto pmosd2port = Electric.getPortProto(pmos, "p-trans-diff-bottom");
	int[] pmosbox = {((Integer)Electric.getVal(pmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(pmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(pmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(pmos, "highy")).intValue()};

	Electric.NodeProto ppin = Electric.getNodeProto("Polysilicon-1-Pin");
	Electric.PortProto ppinport =
		(Electric.PortProto)Electric.getVal(ppin, "firstPortProto");
	int[] ppinbox = {((Integer)Electric.getVal(ppin, "lowx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "lowy")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highy")).intValue()};
	
	Electric.NodeProto napin = Electric.getNodeProto("N-Active-Pin");
	
	Electric.NodeProto m1pin = Electric.getNodeProto("Metal-1-Pin");
	Electric.PortProto m1pinport =
		(Electric.PortProto)Electric.getVal(m1pin, "firstPortProto");
	int[] m1pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m2pin = Electric.getNodeProto("Metal-2-Pin");
	Electric.PortProto m2pinport =
		(Electric.PortProto)Electric.getVal(m2pin, "firstPortProto");
	int[] m2pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};

	int mx = 5;
	Electric.NodeProto mpc = Electric.getNodeProto("Metal-1-Polysilicon-1-Con");
	Electric.PortProto mpcport =
		(Electric.PortProto)Electric.getVal(mpc, "firstPortProto");
	int[] mpcbox = {-mx*lambda/2, mx*lambda/2, -mx*lambda/2, mx*lambda/2}; 
	
	Electric.NodeProto diffpin = Electric.getNodeProto("Active-Pin");
	Electric.PortProto diffpinport =
		(Electric.PortProto)Electric.getVal(diffpin, "firstPortProto");
	int[] diffpinbox =
		{((Integer)Electric.getVal(diffpin, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(diffpin, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
	Electric.PortProto m1m2cport =
		(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
	int[] m1m2cbox = {-5*lambda/2, 5*lambda/2, -5*lambda/2, 5*lambda/2}; 

	Electric.NodeProto mnac = Electric.getNodeProto("Metal-1-N-Active-Con");
	Electric.PortProto mnacport =
		(Electric.PortProto)Electric.getVal(mnac, "firstPortProto");
	int[] mnacbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};
	//centers around 6 goes up by multiples of 2

	Electric.NodeProto mpac = Electric.getNodeProto("Metal-1-P-Active-Con");
	Electric.PortProto mpacport =
		(Electric.PortProto)Electric.getVal(mpac, "firstPortProto");
	int[] mpacbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};
	//centers around 6 goes up my multiple of 2
	


	Electric.NodeProto mnwc = Electric.getNodeProto("Metal-1-N-Well-Con");
	Electric.PortProto mnwcport =
		(Electric.PortProto)Electric.getVal(mnwc, "firstPortProto");
	int[] mnwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};

	Electric.NodeProto mpwc = Electric.getNodeProto("Metal-1-P-Well-Con");
	Electric.PortProto mpwcport =
		(Electric.PortProto)Electric.getVal(mpwc, "firstPortProto");
	int[] mpwcbox = {-2*lambda, 2*lambda, -2*lambda, 2*lambda};

	Electric.ArcProto parc = Electric.getArcProto("Polysilicon-1");
	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	Electric.ArcProto ndiffarc = Electric.getArcProto("N-Active");
	Electric.ArcProto pdiffarc = Electric.getArcProto("P-Active");

	Electric.NodeProto nwnode = Electric.getNodeProto("N-Well-Node");
	int[] nwnodebox =
		{((Integer)Electric.getVal(nwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(nwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highy")).intValue()+lambda/2};
	Electric.NodeProto pwnode = Electric.getNodeProto("P-Well-Node");
	int[] pwnodebox =
		{((Integer)Electric.getVal(pwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(pwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highy")).intValue()+lambda/2};

	/* create a cell called cellname+"{lay}" in the current library */
	Electric.NodeProto decp =
		Electric.newNodeProto(cellname+"{lay}", Electric.curLib());
	
	Electric.NodeProto psnode = Electric.getNodeProto("P-Select-Node");
	int pselectx = 8; 
	int pselecty = 8;
	int[] psnodebox =
		{-pselectx*lambda/2, pselectx*lambda/2, -pselecty*lambda/2, pselecty*lambda/2};

	Electric.NodeInst nwellnode =
		Electric.newNodeInst(nwnode,0,(8*lambda*(2*bits)),
									0,8*lambda*(wordlines+1),0,0,decp);
	Electric.NodeInst pselectnode =
		Electric.newNodeInst(psnode,0,(8*lambda*(2*bits)),
									0,8*lambda*(wordlines+1),0,0,decp);

	// Create instances of objects on decoder pmos plane
	x = 0;
	for (i=0; i<inputs+1; i++) {
		x += 8*lambda;
		y = 0;
		
		if (i < inputs)  {
			if ( top == true)   {
				andtrans[0][i] =
					Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
											   ppinbox[2], ppinbox[3], 0, 0, decp);
				
			}else {    
				andtrans[0][i] =
					Electric.newNodeInst(mpc, mpcbox[0]+x, mpcbox[1]+x,
											  mpcbox[2], mpcbox[3], 0, 0, decp);
			}
		}
		for (m=0; m<wordlines; m++) {
			y += 8*lambda;
			if (i < inputs) {
				if (romplane[m][i] == 1) {
					// create a transistor
					andtrans[m+1][i] =
						Electric.newNodeInst(pmos, pmosbox[0]+x, pmosbox[1]+x,
											 pmosbox[2]+y, pmosbox[3]+y, 1, 0, decp);
				} else {
					andtrans[m+1][i] =
						Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
											 ppinbox[2]+y, ppinbox[3]+y, 0, 0, decp);
				}
				if ( m == wordlines-1) {
					if (top == true) {
						andtrans[m+2][i] =
							Electric.newNodeInst(mpc, mpcbox[0]+x, mpcbox[1]+x,
												 mpcbox[2]+y+16*lambda,
												 mpcbox[3]+y+16*lambda,0,0,decp);
					}else {
						andtrans[m+2][i] =
							Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
												 ppinbox[2]+y+4*lambda, ppinbox[3]+y+4*lambda,
												 0, 0, decp);	
					}
				}
			}

			boolean transcont = false;
			if (i < inputs) transcont = (romplane[m][i] == 1);
			if (i == 0) {
				m1m2pins[m] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-4*lambda,
										 m1m2cbox[1]+x-4*lambda, m1m2cbox[2]+y,
										 m1m2cbox[3]+y, 0, 0, decp);			
				m1m2ports[m] = m1m2cport;
			}
			if (i == (inputs)) {
				m2pins[m] =
					Electric.newNodeInst(m2pin, m2pinbox[0]+x-4*lambda,
										 m2pinbox[1]+x-4*lambda, m2pinbox[2]+y,
										 m2pinbox[3]+y, 0, 0, decp);			
				m2ports[m] = m2pinport;
			}
			if (i >= 1) transcont |= (romplane[m][i-1] == 1);
			if (transcont){
				minpins[m][i] =
					Electric.newNodeInst(mpac, mpacbox[0]+x-4*lambda,
										 mpacbox[1]+x-4*lambda, mpacbox[2]+y,
										 mpacbox[3]+y, 0, 0, decp);			
				minports[m][i] = mpacport;
			} else {
				if ( i == inputs) {
					minpins[m][i] =
						Electric.newNodeInst(mnwc, mnwcbox[0]+x-4*lambda,
											 mnwcbox[1]+x-4*lambda, mnwcbox[2]+y,
											 mnwcbox[3]+y, 0, 0, decp);
					minports[m][i] = mnwcport;
				} else {
					minpins[m][i] =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
											 m1pinbox[1]+x-4*lambda, m1pinbox[2]+y,
											 m1pinbox[3]+y, 0, 0, decp);
					minports[m][i] = m1pinport;
				}
			}
			if (i == inputs) {
				vddpin =
					Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
										 m1pinbox[1]+x-4*lambda,
										 m1pinbox[2]+y+8*lambda,
										 m1pinbox[3]+y+8*lambda, 0, 0, decp);
				vddport = m1pinport;
				if (m == 0) {
					vddbpin =
						Electric.newNodeInst(m1pin, m1pinbox[0]+x+4*lambda,
											 m1pinbox[1]+x+4*lambda,
											 m1pinbox[2]+y+0*lambda,
											 m1pinbox[3]+y+0*lambda, 0, 0, decp);
						vddbport = m1pinport;
				}
				if (m == wordlines-1) {
					vddcpin =
						Electric.newNodeInst(m1m2c, m1m2cbox[0]+x+4*lambda,
											 m1m2cbox[1]+x+4*lambda,
											 m1m2cbox[2]+y+8*lambda,
											 m1m2cbox[3]+y+8*lambda, 0, 0, decp);	
					vddcport = m1m2cport;
				}
			}
		}
	}

	// connect polysilicon gates
	for (i=0; i<inputs; i++)
	{
		ap1 = andtrans[wordlines+1][i];
			if (top == true)  {
				apport1 = mpcport;
			}else {
				apport1 = ppinport;
			}
		appos1 = Electric.portPosition(ap1, apport1); 
		if (i%2 == 0) {
			p = Electric.newPortProto(decp, ap1, apport1, ("top_in"+(i/2)));
			createExport(p, INPORT);
		} else {
			p = Electric.newPortProto(decp, ap1, apport1, ("top_in"+((i-1)/2)+"_b"));
			createExport(p, INPORT);
		}
		ap1 = andtrans[0][i];
		if (top == true)  {
				apport1 = ppinport;
			}else {
				apport1 = mpcport;
			}
		appos1 = Electric.portPosition(ap1, apport1); 
		if (i%2 == 0) {
			p = Electric.newPortProto(decp, ap1, apport1, ("bot_in"+(i/2)));
			createExport(p, INPORT);
		} else {
			p = Electric.newPortProto(decp, ap1, apport1, ("bot_in"+((i-1)/2)+"_b"));
			createExport(p, INPORT);
		}
		for (m=1; m<wordlines+1; m++) {
			ap2 = andtrans[m][i];
			if (romplane[m-1][i] == 1) {
				apport2 = pmosg1port;
				apport3 = pmosg2port;
			} else {
				apport2 = ppinport;
				apport3 = ppinport;
			}
			appos2 = Electric.portPosition(ap2, apport2);
			appos3 = Electric.portPosition(ap2, apport3);
			Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),decp);
			ap1 = ap2;
			apport1 = apport3;
			appos1 = appos3;
		}
		
		ap2 = andtrans[wordlines+1][i];
		if ( top == true) {
			apport2 = mpcport;
			apport3 = mpcport;
		}else {
			apport2 = ppinport;
			apport3 = ppinport;
		}
		appos2 = Electric.portPosition(ap2, apport2);
		appos3 = Electric.portPosition(ap2, apport3);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),decp);
	}

	// connect m1 wordline lines
	for (m=0; m<wordlines; m++) {
		ap1 = minpins[m][0];
		apport1 = minports[m][0];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (i=1; i<inputs+1; i++) {
			ap2 = minpins[m][i];
			apport2 = minports[m][i];
			appos2 = Electric.portPosition(ap2, apport2); 
			if (romplane[m][i-1] != 1) {
				Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
									appos1[1].intValue(),ap2,apport2,
									appos2[0].intValue(), appos2[1].intValue(),decp);
			}
			ap1 = ap2;
			apport1 = apport2;
			appos1 = appos2;
		}
	}

	// connect transistors to wordline lines
	for (m=0; m<wordlines; m++) {
		for (i=0; i<inputs; i++) {
			if (romplane[m][i] == 1) {
				ap1 = andtrans[m+1][i];
				apport1 = pmosd1port;
				apport2 = pmosd2port;
				appos1 = Electric.portPosition(ap1, apport1);
				appos2 = Electric.portPosition(ap1, apport2);
				apx = minpins[m][i];
				apy = minpins[m][i+1];
				apportx = mpacport;
				apporty = mpacport;
				apposx = Electric.portPosition(apx, apportx);
				apposy = Electric.portPosition(apy, apporty);
				// pdiffarc size centers around 12 and goes up by multiples of 2
				Electric.newArcInst(pdiffarc,16*lambda,0,ap1,apport1,
									appos1[0].intValue(), appos1[1].intValue(),apx,
									apportx,apposx[0].intValue(),apposx[1].intValue(),
									decp);
				Electric.newArcInst(pdiffarc,16*lambda,0,ap1,apport2,
									appos2[0].intValue(), appos2[1].intValue(),apy,
									apporty,apposy[0].intValue(),apposy[1].intValue(),
									decp);
			}
		}
	}

//	 connect ground lines
	i = inputs;
	ap1 = minpins[0][i];
	apport1 = minports[0][i];
	appos1 = Electric.portPosition(ap1, apport1); 
	for (m=1; m<wordlines; m++) {
		ap2 = minpins[m][i];
		apport2 = minports[m][i];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),decp);
		ap1 = ap2;
		apport1 = apport2;
		appos1 = appos2;
	}

	ap1 = vddpin;
	apport1 = vddport;
	appos1 = Electric.portPosition(ap1, apport1); 
	ap2 = minpins[wordlines-1][inputs];
	apport2 = minports[wordlines-1][inputs];
	appos2 = Electric.portPosition(ap2, apport2); 
	Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
						appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
						appos2[1].intValue(),decp);
	
	ap1 = vddcpin;
	apport1 = vddcport;
	appos1 = Electric.portPosition(ap1, apport1);
	ap2 = vddpin;
	apport2 = vddport;
	appos2 = Electric.portPosition(ap2, apport2); 
	Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
						appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
						appos2[1].intValue(),decp);
	p = Electric.newPortProto(decp, ap1, apport1, "vdd");
	createExport(p, PWRPORT);

	ap1 = vddbpin;
	apport1 = vddbport;
	appos1 = Electric.portPosition(ap1, apport1); 
	ap2 = minpins[0][inputs];
	apport2 = minports[0][inputs];
	appos2 = Electric.portPosition(ap2, apport2); 
	Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
						appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
						appos2[1].intValue(),decp);
	p = Electric.newPortProto(decp, ap1, apport1, "vddb");
	createExport(p, PWRPORT);
	
//	 connect metal 2 lines
	for (m=0; m<wordlines; m++) {
		ap1 = m1m2pins[m];
		apport1 = m1m2ports[m];
		appos1 = Electric.portPosition(ap1,apport1);
		ap2 = m2pins[m];
		apport2 = m2ports[m];
		appos2 = Electric.portPosition(ap2, apport2); 
		ap3 = minpins[m][0];
		apport3 = minports[m][0];
		appos3 = Electric.portPosition(ap3, apport3);
		Electric.newArcInst(m2arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),decp);
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap3,apport3,appos3[0].intValue(),
							appos3[1].intValue(),decp);
		p = Electric.newPortProto(decp, ap2, apport2, ("word"+m));
		createExport(p, OUTPORT);
		p = Electric.newPortProto(decp, ap1, apport1, ("wordin"+m));
		createExport(p, INPORT);
	}
}
////////////////////decoderpmos end

private static int[][] generatemuxarray(int folds, int romoutputs) {
	int muxes = romoutputs/folds;
	int[][] muxarray = new int[romoutputs][folds];
	for (int i=0; i<muxes; i++) {
		for (int j=0; j<folds; j++) {
			for (int k=0; k<folds; k++) {
				if (j == k) {
					muxarray[i*folds+j][k] = 1;
				} else {
					muxarray[i*folds+j][k] = 0;
				}
			}
		}
	}
	StringBuffer sb = new StringBuffer();
	for (int i=0; i<romoutputs; i++) {
		sb = new StringBuffer();
		for (int j=0; j<folds; j++) {
			sb.append(Integer.toString(muxarray[i][j]));
		}
	}
	return muxarray;
}



public static void muxplane(int lambda, int folds, int romoutputs, String mp) {
	int[][] muxarray = generatemuxarray(folds,romoutputs);
	int muxnumber = folds;
	int selects = folds;
	int outputbits = romoutputs;

	int i, m, o;
	int x, y;
	Electric.NodeInst ap1, ap2, ap3, apx, apy;
	Electric.PortProto p, apport1, apport2, apport3, apportx, apporty;
	Integer[] appos1, appos2, appos3, apposx, apposy;

	Electric.NodeInst[][] ntrans = new Electric.NodeInst[outputbits+2][selects+2];
	Electric.NodeInst[][] minpins = new Electric.NodeInst[outputbits+2][selects+2];
	Electric.NodeInst[] m1m2pins2 = new Electric.NodeInst[outputbits+2];
	Electric.NodeInst[] m1m2pins = new Electric.NodeInst[selects+2];
	Electric.NodeInst[] m2pins = new Electric.NodeInst[outputbits+2];
	Electric.NodeInst[] m1polypins = new Electric.NodeInst[outputbits+2];

	Electric.PortProto[][] minports = new Electric.PortProto[outputbits+2][selects+2];
	Electric.PortProto[] m1m2ports2 = new Electric.PortProto[outputbits+2];
	Electric.PortProto[] m1m2ports = new Electric.PortProto[selects+2];
	Electric.PortProto[] m2ports = new Electric.PortProto[outputbits+2];
	Electric.PortProto[] m1polyports = new Electric.PortProto[outputbits+2];
	
	/* get pointers to primitives */
	Electric.NodeProto nmos = Electric.getNodeProto("N-Transistor");
	Electric.PortProto nmosg1port = Electric.getPortProto(nmos, "n-trans-poly-right");
	Electric.PortProto nmosg2port = Electric.getPortProto(nmos, "n-trans-poly-left");
	Electric.PortProto nmosd1port = Electric.getPortProto(nmos, "n-trans-diff-top");
	Electric.PortProto nmosd2port = Electric.getPortProto(nmos, "n-trans-diff-bottom");
	int[] nmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};
				
	Electric.NodeProto pmos = Electric.getNodeProto("P-Transistor");
	Electric.PortProto pmosg1port = Electric.getPortProto(pmos, "p-trans-poly-right");
	Electric.PortProto pmosg2port = Electric.getPortProto(pmos, "p-trans-poly-left");
	Electric.PortProto pmosd1port = Electric.getPortProto(pmos, "p-trans-diff-top");
	Electric.PortProto pmosd2port = Electric.getPortProto(pmos, "p-trans-diff-bottom");
	int[] pmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};

	Electric.NodeProto ppin = Electric.getNodeProto("Polysilicon-1-Pin");
	Electric.PortProto ppinport =
		(Electric.PortProto)Electric.getVal(ppin, "firstPortProto");
	int[] ppinbox = {((Integer)Electric.getVal(ppin, "lowx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "lowy")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highy")).intValue()};
	
	Electric.NodeProto napin = Electric.getNodeProto("N-Active-Pin");
	
	Electric.NodeProto m1pin = Electric.getNodeProto("Metal-1-Pin");
	Electric.PortProto m1pinport =
		(Electric.PortProto)Electric.getVal(m1pin, "firstPortProto");
	int[] m1pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m2pin = Electric.getNodeProto("Metal-2-Pin");
	Electric.PortProto m2pinport =
		(Electric.PortProto)Electric.getVal(m2pin, "firstPortProto");
	int[] m2pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};

	Electric.NodeProto diffpin = Electric.getNodeProto("Active-Pin");
	Electric.PortProto diffpinport =
		(Electric.PortProto)Electric.getVal(diffpin, "firstPortProto");
	int[] diffpinbox =
		{((Integer)Electric.getVal(diffpin, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(diffpin, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
	Electric.PortProto m1m2cport =
		(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
	int[] m1m2cbox = {-5*lambda/2, 5*lambda/2, -5*lambda/2, 5*lambda/2}; 
	
	Electric.NodeProto mnac = Electric.getNodeProto("Metal-1-N-Active-Con");
	Electric.PortProto mnacport =
		(Electric.PortProto)Electric.getVal(mnac, "firstPortProto");
	int mult = 17;
	int[] mnacbox = {-1*mult*lambda/2, mult*lambda/2, -1*mult*lambda/2, mult*lambda/2};
	
	Electric.NodeProto mpac = Electric.getNodeProto("Metal-1-N-Active-Con");
	
	Electric.NodeProto mpwc = Electric.getNodeProto("Metal-1-P-Well-Con");
	Electric.PortProto mpwcport =
		(Electric.PortProto)Electric.getVal(mpwc, "firstPortProto");
	int[] mpwcbox = {-2*lambda, 2*lambda, -2*lambda, 2*lambda};

	Electric.NodeProto mpc = Electric.getNodeProto("Metal-1-Polysilicon-1-Con");
	Electric.PortProto mpcport =
		(Electric.PortProto)Electric.getVal(mpc, "firstPortProto");
	int mx = 5;
	int[] mpcbox = {-mx*lambda/2, mx*lambda/2, -mx*lambda/2, mx*lambda/2}; 

	Electric.ArcProto parc = Electric.getArcProto("Polysilicon-1");
	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	Electric.ArcProto ndiffarc = Electric.getArcProto("N-Active");

	Electric.NodeProto pwnode = Electric.getNodeProto("P-Well-Node");
	int[] pwnodebox =
		{((Integer)Electric.getVal(pwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(pwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highy")).intValue()+lambda/2};


	/* create a cell called "muxplane{lay}" in the current library */
	Electric.NodeProto muxplane =
		Electric.newNodeProto(mp+"{lay}", Electric.curLib());

	Electric.NodeInst pwellnode =
		Electric.newNodeInst(pwnode,-8*lambda,lambda*8*(folds+1),
									-8*lambda,8*lambda*3*romoutputs/2,0,0,muxplane);

	// Create instances of objects in mux plane
	x = 0;
	for (i=0; i<selects+1; i++) {
		x += 8*lambda;
		y = 0;
		if (i < selects)  {  
			ntrans[0][i] =
				Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x, ppinbox[2],
									 ppinbox[3], 0, 0, muxplane);
			m1polypins[i] =
				Electric.newNodeInst(mpc, mpcbox[0]+x, mpcbox[1]+x, mpcbox[2],
									 mpcbox[3], 0, 0, muxplane);
			m1m2pins[i] =
				Electric.newNodeInst(m1m2c, m1m2cbox[0]+x, m1m2cbox[1]+x, m1m2cbox[2],
									 m1m2cbox[3], 0, 0, muxplane);
			ap1 = m1m2pins[i];
			apport1 = m1m2cport;
			p = Electric.newPortProto(muxplane, ap1, apport1, ("sel"+(selects-i-1)));
			createExport(p, INPORT);
		}

		for (m=0; m<outputbits; m++) {
			y += 8*lambda;
			if (m%2 == 1) {
				y += 8*lambda;
			}
			if (i < selects) {
				if (muxarray[m][i] == 1) {
					// create a transistor
					ntrans[m+1][i] =
						Electric.newNodeInst(nmos, nmosbox[0]+x, nmosbox[1]+x,
											 nmosbox[2]+y, nmosbox[3]+y,1,0,muxplane);
				} else {
					ntrans[m+1][i] =
						Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
											 ppinbox[2]+y,ppinbox[3]+y,0,0,muxplane);
				}
			}
			boolean transcont = false;
			if (i < selects) transcont = (muxarray[m][i] == 1);
			if (i == (selects)) {
				m1m2pins2[m] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-4*lambda,
										 m1m2cbox[1]+x-4*lambda, m1m2cbox[2]+y,
										 m1m2cbox[3]+y, 0, 0, muxplane);			
				m1m2ports2[m] = m1m2cport;		
			}
			if (i >= 1) transcont |= (muxarray[m][i-1] == 1);
			if (transcont){
				minpins[m][i] =
					Electric.newNodeInst(mnac, mnacbox[0]+x-4*lambda,
										 mnacbox[1]+x-4*lambda, mnacbox[2]+y,
										 mnacbox[3]+y, 0, 0, muxplane);
				minports[m][i] = mnacport;
			} else {
				minpins[m][i] =
					Electric.newNodeInst(m1pin, m1pinbox[0]+x-4*lambda,
										 m1pinbox[1]+x-4*lambda, m1pinbox[2]+y,
										 m1pinbox[3]+y, 0, 0, muxplane);
				minports[m][i] = m1pinport;
			}
		}
	}

	// finished placing objects, now wire arcs

	// connect polysilicon gates
	for (i=0; i<selects; i++) {
		ap1 = ntrans[0][i];
		apport1 = ppinport;
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = m1polypins[i];
		apport2 = mpcport;
		appos2 = Electric.portPosition(ap2, apport2); 
		ap3 = m1m2pins[i];
		apport3 = m1m2cport;
		appos3 = Electric.portPosition(ap3, apport3); 
		Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(),ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),muxplane);
		Electric.newArcInst(m1arc, 4*lambda, 0, ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(),muxplane);
	}
	
	// connect polysilicon gates
	for (i=0; i<selects; i++)
	{
		ap1 = ntrans[0][i];
		apport1 = ppinport;
		appos1 = Electric.portPosition(ap1, apport1); 
		for (m=1; m<outputbits+1; m++) {
			ap2 = ntrans[m][i];
			if (muxarray[m-1][i] == 1) {
				apport2 = nmosg1port;
				apport3 = nmosg2port;
			} else {
				apport2 = ppinport;
				apport3 = ppinport;
			}
			appos2 = Electric.portPosition(ap2, apport2);
			appos3 = Electric.portPosition(ap2, apport3);
			Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),muxplane);
			ap1 = ap2;
			apport1 = apport3;
			appos1 = appos3;
		}
	}

	// connect m1 wordline lines
	for (m=0; m<outputbits; m++) {
		ap1 = minpins[m][0];
		apport1 = minports[m][0];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (i=1; i<selects+1; i++) {
			ap2 = minpins[m][i];
			apport2 = minports[m][i];
			appos2 = Electric.portPosition(ap2, apport2); 
			if (muxarray[m][i-1] != 1) {
				Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1,
									appos1[0].intValue(), appos1[1].intValue(),
									ap2, apport2, appos2[0].intValue(),
									appos2[1].intValue(),muxplane);
			}
			if (i == 1) {
				p = Electric.newPortProto(muxplane, ap1, apport1, ("muxin"+m));
				createExport(p, INPORT);
			}
			ap1 = ap2;
			apport1 = apport2;
			appos1 = appos2;
		}
	}
	

	// connect transistors to wordline lines
	for (m=0; m<outputbits; m++) {
		for (i=0; i<selects; i++) {
			if (muxarray[m][i] == 1) {
				// connect transistor
				ap1 = ntrans[m+1][i];
				apport1 = nmosd1port;
				apport2 = nmosd2port;
				appos1 = Electric.portPosition(ap1, apport1);
				appos2 = Electric.portPosition(ap1, apport2);
				apx = minpins[m][i];
				apy = minpins[m][i+1];
				apportx = mnacport;
				apporty = mnacport;
				apposx = Electric.portPosition(apx, apportx);
				apposy = Electric.portPosition(apy, apporty);
				Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, apport1,
									appos1[0].intValue(), appos1[1].intValue(),
									apx, apportx, apposx[0].intValue(),
									apposx[1].intValue(),muxplane);
				Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, apport2,
									appos2[0].intValue(), appos2[1].intValue(),
									apy, apporty, apposy[0].intValue(),
									apposy[1].intValue(),muxplane);
			}
		}
	}

	for(int j = 0 ; j < outputbits; j++) {
		i = selects;
		ap1 = minpins[j][i];
		apport1 = minports[j][i];
		appos1 = Electric.portPosition(ap1, apport1); 
		ap2 = m1m2pins2[j];
		apport2 = m1m2ports2[j];
		appos2 = Electric.portPosition(ap2, apport2); 
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),muxplane);
	}

//	 connect mux together
	for(int j = 0 ; j < outputbits/muxnumber; j++) {
		ap1 = m1m2pins2[j*muxnumber];
		apport1 = m1m2ports2[j*muxnumber];
		appos1 = Electric.portPosition(ap1, apport1); 
		for (m=1+j*muxnumber; m<muxnumber+j*muxnumber; m++) {
			ap2 = m1m2pins2[m];
			apport2 = m1m2ports2[m];
			appos2 = Electric.portPosition(ap2, apport2); 
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(),ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),muxplane);
		}
		p = Electric.newPortProto(muxplane, ap1, apport1, "muxout"+j);
		createExport(p, OUTPORT);
	}

}
	
////////////////////inverterplane 
public static void inverterplane(int lambda, int outs, int folds, String ip) {	
	
	int i, m, o;
	int x, y;
	Electric.NodeInst ap1, ap2, ap3, gnd1, gnd2, vdd1, vdd2;
	Electric.PortProto p, apport1, apport2, apport3, trans1port, trans2port,
					   gndport1, gndport2, vddport1, vddport2;
	Integer[] appos1, appos2, appos3, trans1pos, trans2pos, gndpos1, gndpos2,
			  vddpos1, vddpos2;

	Electric.NodeInst[] ntrans = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] ptrans = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] inpins = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] polypins = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] intppins = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] nmospins = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] pmospins = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] gndpins = new Electric.NodeInst[outs/2];
	Electric.NodeInst[] pwellpins = new Electric.NodeInst[outs/2];
	Electric.NodeInst[] vddpins = new Electric.NodeInst[outs/2];
	Electric.NodeInst[] midvddpins = new Electric.NodeInst[outs/2];
	Electric.NodeInst[] gndpins2 = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] pwellpins2 = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] vddpins2 = new Electric.NodeInst[outs/folds];
	Electric.NodeInst[] midvddpins2 = new Electric.NodeInst[outs/folds];
	Electric.NodeInst gndc = new Electric.NodeInst();
	Electric.NodeInst nwellc = new Electric.NodeInst();
	Electric.NodeInst nwellm = new Electric.NodeInst();
	
	Electric.PortProto[] ntransports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] ptransports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] inports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] polyports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] intpports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] nmosports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] pmosports = new Electric.PortProto[outs/folds];
	Electric.PortProto[] pwellports = new Electric.PortProto[outs/2];
	Electric.PortProto[] gndports = new Electric.PortProto[outs/2];
	Electric.PortProto[] vddports = new Electric.PortProto[outs/2];
	Electric.PortProto[] midvddports = new Electric.PortProto[outs/2];
	Electric.PortProto[] pwellports2 = new Electric.PortProto[outs/folds];
	Electric.PortProto[] gndports2 = new Electric.PortProto[outs/folds];
	Electric.PortProto[] vddports2 = new Electric.PortProto[outs/folds];
	Electric.PortProto[] midvddports2 = new Electric.PortProto[outs/folds];
	Electric.PortProto gndcport = new Electric.PortProto();
	Electric.PortProto nwellcport = new Electric.PortProto();
	Electric.PortProto nwellmport = new Electric.PortProto();


	/* get pointers to primitives */
	Electric.NodeProto nmos = Electric.getNodeProto("N-Transistor");
	Electric.PortProto nmosg1port = Electric.getPortProto(nmos, "n-trans-poly-right");
	Electric.PortProto nmosg2port = Electric.getPortProto(nmos, "n-trans-poly-left");
	Electric.PortProto nmosd1port = Electric.getPortProto(nmos, "n-trans-diff-top");
	Electric.PortProto nmosd2port = Electric.getPortProto(nmos, "n-trans-diff-bottom");
	int[] nmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};
				
	Electric.NodeProto pmos = Electric.getNodeProto("P-Transistor");
	Electric.PortProto pmosg1port = Electric.getPortProto(pmos, "p-trans-poly-right");
	Electric.PortProto pmosg2port = Electric.getPortProto(pmos, "p-trans-poly-left");
	Electric.PortProto pmosd1port = Electric.getPortProto(pmos, "p-trans-diff-top");
	Electric.PortProto pmosd2port = Electric.getPortProto(pmos, "p-trans-diff-bottom");
	int[] pmosbox = {((Integer)Electric.getVal(pmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(pmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(pmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(pmos, "highy")).intValue()};

	Electric.NodeProto ppin = Electric.getNodeProto("Polysilicon-1-Pin");
	Electric.PortProto ppinport =
		(Electric.PortProto)Electric.getVal(ppin, "firstPortProto");
	int[] ppinbox = {((Integer)Electric.getVal(ppin, "lowx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "lowy")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highy")).intValue()};
	
	Electric.NodeProto napin = Electric.getNodeProto("N-Active-Pin");
	
	Electric.NodeProto m1pin = Electric.getNodeProto("Metal-1-Pin");
	Electric.PortProto m1pinport =
		(Electric.PortProto)Electric.getVal(m1pin, "firstPortProto");
	int[] m1pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m2pin = Electric.getNodeProto("Metal-2-Pin");
	Electric.PortProto m2pinport =
		(Electric.PortProto)Electric.getVal(m2pin, "firstPortProto");
	int[] m2pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};

	Electric.NodeProto nwnode = Electric.getNodeProto("N-Well-Node");
	int[] nwnodebox =
		{((Integer)Electric.getVal(nwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(nwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto pwnode = Electric.getNodeProto("P-Well-Node");
	int[] pwnodebox =
		{((Integer)Electric.getVal(pwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(pwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
	Electric.PortProto m1m2cport =
		(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
	int[] m1m2cbox = {-5*lambda/2, 5*lambda/2, -5*lambda/2, 5*lambda/2}; 

	Electric.NodeProto diffpin = Electric.getNodeProto("Active-Pin");
	Electric.PortProto diffpinport =
		(Electric.PortProto)Electric.getVal(diffpin, "firstPortProto");
	int[] diffpinbox = 
		{((Integer)Electric.getVal(diffpin, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(diffpin, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highy")).intValue()+lambda/2};

	Electric.NodeProto mnac = Electric.getNodeProto("Metal-1-N-Active-Con");
	Electric.PortProto mnacport =
		(Electric.PortProto)Electric.getVal(mnac, "firstPortProto");
	int aaa = 17;
	int[] mnacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};

	Electric.NodeProto mpac = Electric.getNodeProto("Metal-1-P-Active-Con");
	Electric.PortProto mpacport =
		(Electric.PortProto)Electric.getVal(mpac, "firstPortProto");
	int[] mpacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};   //

	Electric.NodeProto mpc = Electric.getNodeProto("Metal-1-Polysilicon-1-Con");
	Electric.PortProto mpcport =
		(Electric.PortProto)Electric.getVal(mpc, "firstPortProto");
	int mx = -7;
	int my = 5;
	int[] mpcbox = {-mx*lambda/2, mx*lambda/2, -my*lambda/2, my*lambda/2}; 

	Electric.NodeProto mpwc = Electric.getNodeProto("Metal-1-P-Well-Con");
	Electric.PortProto mpwcport =
		(Electric.PortProto)Electric.getVal(mpwc, "firstPortProto");
	int[] mpwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};
	
	Electric.NodeProto mnwc = Electric.getNodeProto("Metal-1-N-Well-Con");
	Electric.PortProto mnwcport =
		(Electric.PortProto)Electric.getVal(mnwc, "firstPortProto");
	int[] mnwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};

	Electric.ArcProto parc = Electric.getArcProto("Polysilicon-1");
	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	Electric.ArcProto ndiffarc = Electric.getArcProto("N-Active");
	Electric.ArcProto pdiffarc = Electric.getArcProto("P-Active");

	/* create a cell called "inverterplane{lay}" in the current library */
	Electric.NodeProto invp =
		Electric.newNodeProto(ip+"{lay}", Electric.curLib());

	Electric.NodeInst pwellnode =
		Electric.newNodeInst(pwnode,-32*lambda,(3*lambda*8*outs/2)+8*lambda,
									-18*lambda,16*lambda,0,0,invp);

	Electric.NodeInst nwellnode =
		Electric.newNodeInst(nwnode,-32*lambda,(3*lambda*8*outs/2)+8*lambda,
									-34*lambda,-18*lambda,0,0,invp);

	// Create instances of objects on inverter plane
	x = 0*lambda;
	for (i=0; i<outs; i++) {
		x += 8*lambda;
		y = 0;
		if (folds > 1 ) {
			if ( i % folds == 1) {
				pwellpins2[i/folds] =
					Electric.newNodeInst(mpwc, mpwcbox[0]+x, mpwcbox[1]+x,
										 mpwcbox[2]+y, mpwcbox[3]+y, 0, 0, invp);
				pwellports2[i/folds] = mpwcport;
				gndpins2[i/folds] =
					Electric.newNodeInst(mnac, mnacbox[0]+x, mnacbox[1]+x,
										 mnacbox[2]+y-10*lambda,
										 mnacbox[3]+y-10*lambda, 0, 0, invp);
				gndports2[i/folds] = mnacport;
				midvddpins2[i/folds] =
					Electric.newNodeInst(mpac, mpacbox[0]+x, mpacbox[1]+x,
										 mpacbox[2]+y-26*lambda,
										 mpacbox[3]+y-26*lambda, 0, 0, invp);
				midvddports2[i/folds] = mpacport;
				vddpins2[i/folds] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x, m1m2cbox[1]+x,
										 m1m2cbox[2]+y-26*lambda,
										 m1m2cbox[3]+y-26*lambda, 0, 0, invp);
				vddports2[i/folds] = m1m2cport;
				if (i == 1) {
					gndc =
						Electric.newNodeInst(m1m2c, m1m2cbox[0]+x, m1m2cbox[1]+x,
											 m1m2cbox[2]+y-10*lambda,
											 m1m2cbox[3]+y-10*lambda, 0, 0, invp);
					gndcport = m1m2cport;
					p = Electric.newPortProto(invp, gndc, gndcport, ("gnd"));
					createExport(p, GNDPORT);
				}
			}
		} else  {
			if ( i%2 == 1){
				//place gnd, intvdd, vddpins
//				pwellpins[i/2] =
//					Electric.newNodeInst(mpwc, mpwcbox[0]+x, mpwcbox[1]+x,
//										 mpwcbox[2]+y-10*lambda,
//										 mpwcbox[3]+y-10*lambda, 0, 0, invp);
//				pwellports[i/2] = mpwcport;
				gndpins[i/2] =
					Electric.newNodeInst(mnac, mnacbox[0]+x, mnacbox[1]+x,
										 mnacbox[2]+y-10*lambda,
										 mnacbox[3]+y-10*lambda, 0, 0, invp);
				gndports[i/2] = mnacport;
				midvddpins[i/2] =
					Electric.newNodeInst(mpac, mpacbox[0]+x, mpacbox[1]+x,
										 mpacbox[2]+y-26*lambda,
										 mpacbox[3]+y-26*lambda, 0, 0, invp);
				midvddports[i/2] = mpacport;
				vddpins[i/2] =
					Electric.newNodeInst(m1m2c, m1m2cbox[0]+x, m1m2cbox[1]+x,
										 m1m2cbox[2]+y-26*lambda,
										 m1m2cbox[3]+y-26*lambda, 0, 0, invp);
				vddports[i/2] = m1m2cport;
			
				if (i == 1) {
					nwellc =
						Electric.newNodeInst(mnwc, mnwcbox[0]+x-24*lambda,
											 mnwcbox[1]+x-24*lambda,
											 mnwcbox[2]+y-26*lambda,
										 	 mnwcbox[3]+y-26*lambda, 0, 0, invp); 
					nwellcport = mnwcport;

					nwellm =
						Electric.newNodeInst(m1m2c, m1m2cbox[0]+x-24*lambda,
											 m1m2cbox[1]+x-24*lambda,
											 m1m2cbox[2]+y-26*lambda,
											 m1m2cbox[3]+y-26*lambda, 0, 0, invp);
					nwellmport = m1m2cport;

					gndc =
						Electric.newNodeInst(m1m2c, m1m2cbox[0]+x, m1m2cbox[1]+x,
											 m1m2cbox[2]+y-10*lambda,
											 m1m2cbox[3]+y-10*lambda, 0, 0, invp);
					gndcport = m1m2cport;
					p = Electric.newPortProto(invp, gndc, gndcport, ("gnd"));
					createExport(p, GNDPORT);
				}
			}
		}
		if (i%2 == 1) {
			x += 8*lambda;
		}
		if (folds > 1) {
			if (i%folds == 0)  {
				inpins[i/folds] =
					Electric.newNodeInst(mpc, mpcbox[0]+x-6*lambda,
										 mpacbox[1]+x-6*lambda, mpcbox[2]+y,
										 mpcbox[3]+y, 0, 0, invp);
				inports[i/folds] = mpcport;
				polypins[i/folds] =
					Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
										 ppinbox[2]+y-6*lambda, ppinbox[3]+y-6*lambda,
										 0, 0, invp);
				polyports[i/folds] = ppinport;
				nmospins[i/folds] =
					Electric.newNodeInst(mnac, mnacbox[0]+x, mnacbox[1]+x,
										 mnacbox[2]+y-10*lambda, mnacbox[3]+y-10*lambda,
										 0, 0, invp);
				nmosports[i/folds] = mnacport;
				pmospins[i/folds] =
					Electric.newNodeInst(mpac, mpacbox[0]+x, mpacbox[1]+x,
										 mpacbox[2]+y-26*lambda, mpacbox[3]+y-26*lambda,
										 0, 0, invp);
				pmosports[i/folds] = mpacport; 
			}
		} else {
		//place inpins, polypins, nmospins, pmospins
			inpins[i] =
				Electric.newNodeInst(mpc, mpcbox[0]+x-6*lambda, mpacbox[1]+x-6*lambda,
									 mpcbox[2]+y, mpcbox[3]+y, 0, 0, invp);
			inports[i] = mpcport;
			polypins[i] =
				Electric.newNodeInst(ppin, ppinbox[0]+x, ppinbox[1]+x,
									 ppinbox[2]+y-6*lambda, ppinbox[3]+y-6*lambda,
									 0, 0, invp);
			polyports[i] = ppinport;
			nmospins[i] =
				Electric.newNodeInst(mnac, mnacbox[0]+x, mnacbox[1]+x,
									 mnacbox[2]+y-10*lambda, mnacbox[3]+y-10*lambda,
									 0, 0, invp);
			nmosports[i] = mnacport;
			pmospins[i] =
				Electric.newNodeInst(mpac, mpacbox[0]+x, mpacbox[1]+x,
									 mpacbox[2]+y-26*lambda, mpacbox[3]+y-26*lambda,
									 0, 0, invp);
			pmosports[i] = mpacport;
		}
		
		if (folds > 1) {
			if ( i%folds == 0) {
				int off = 4*lambda;
				ptrans[i/folds] =
					Electric.newNodeInst(pmos, pmosbox[0]+x+off, pmosbox[1]+x+off,
										 pmosbox[2]+y-26*lambda,
										 pmosbox[3]+y-26*lambda, 1, 0, invp);
				ntrans[i/folds] =
					Electric.newNodeInst(nmos, nmosbox[0]+x+off, nmosbox[1]+x+off,
										 nmosbox[2]+y-10*lambda,
										 nmosbox[3]+y-10*lambda, 1, 0, invp);
				intppins[i/folds] =
					Electric.newNodeInst(ppin, ppinbox[0]+x+off, ppinbox[1]+x+off,
										 ppinbox[2]+y-6*lambda, ppinbox[3]+y-6*lambda,
										 0, 0, invp);
				intpports[i/folds] = ppinport;
			}
		} else  {
			int off = 0;
			if (i%2 == 0) {
				off = 4*lambda;
			}else  {
				off = -4*lambda;
			}
			ptrans[i] =
				Electric.newNodeInst(pmos, pmosbox[0]+x+off, pmosbox[1]+x+off,
									 pmosbox[2]+y-26*lambda, pmosbox[3]+y-26*lambda,
									 1, 0, invp);
			ntrans[i] =
				Electric.newNodeInst(nmos, nmosbox[0]+x+off, nmosbox[1]+x+off,
									 nmosbox[2]+y-10*lambda, nmosbox[3]+y-10*lambda,
									 1, 0, invp);
			intppins[i] =
				Electric.newNodeInst(ppin, ppinbox[0]+x+off, ppinbox[1]+x+off,
									 ppinbox[2]+y-6*lambda, ppinbox[3]+y-6*lambda,
									 0, 0, invp);
			intpports[i] = ppinport;
		}
	}

	// connect transistors to diffusion lines
	if ( folds > 1) {
		for (i=0; i<outs/folds; i++) {
			ap2 = nmospins[i];
			ap3 = gndpins2[i];
			ap1 = ntrans[i];
			apport1 = nmosd1port;
			gndport1 = nmosd2port;
			appos1 = Electric.portPosition(ap1, apport1);
			gndpos1 = Electric.portPosition(ap1, gndport1);
			apport2 = mnacport;
			appos2 = Electric.portPosition(ap2, apport2);
			apport3 = mnacport;
			appos3 = Electric.portPosition(ap3, apport3);
			//ndiffarc size centers around 12 and goes up by multiples of 2
			Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, apport1,
								appos1[0].intValue(), appos1[1].intValue(),
								ap2, apport2, appos2[0].intValue(),
								appos2[1].intValue(),invp);
			Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, gndport1,
								gndpos1[0].intValue(), gndpos1[1].intValue(),
								ap3, apport3, appos3[0].intValue(),
								appos3[1].intValue(),invp);
		}
	}else {
		for (i=0; i<outs; i++) {
			if (i%2 == 0) {
				ap2 = nmospins[i];
				ap3 = gndpins[i/2];
			} else {
				ap2 = gndpins[i/2];
				ap3 = nmospins[i];
			}
			ap1 = ntrans[i];
			apport1 = nmosd1port;
			gndport1 = nmosd2port;
			appos1 = Electric.portPosition(ap1, apport1);
			gndpos1 = Electric.portPosition(ap1, gndport1);
			apport2 = mnacport;
			appos2 = Electric.portPosition(ap2, apport2);
			apport3 = mnacport;
			appos3 = Electric.portPosition(ap3, apport3);
			Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, apport1,
								appos1[0].intValue(), appos1[1].intValue(),
								ap2, apport2, appos2[0].intValue(),
								appos2[1].intValue(),invp);
			Electric.newArcInst(ndiffarc, 16*lambda, 0, ap1, gndport1,
								gndpos1[0].intValue(), gndpos1[1].intValue(),
								ap3, apport3, appos3[0].intValue(),
								appos3[1].intValue(),invp);
		}
	}

	if (folds >1) {
		for (i=0; i<outs/folds; i++) {
			ap2 = pmospins[i];
			ap3 = midvddpins2[i];
			ap1 = ptrans[i];
			ap1 = ptrans[i];
			apport1 = pmosd1port;
			vddport1 = pmosd2port;
			appos1 = Electric.portPosition(ap1, apport1);
			vddpos1 = Electric.portPosition(ap1, vddport1);
			apport2 = mpacport;
			appos2 = Electric.portPosition(ap2, apport2);
			apport3 = mpacport;
			appos3 = Electric.portPosition(ap3, apport3);
			Electric.newArcInst(pdiffarc, 16*lambda, 0, ap1, apport1,
								appos1[0].intValue(), appos1[1].intValue(),
								ap2, apport2, appos2[0].intValue(),
								appos2[1].intValue(),invp);
			Electric.newArcInst(pdiffarc, 16*lambda, 0, ap1, vddport1,
								vddpos1[0].intValue(), vddpos1[1].intValue(),
								ap3, apport3, appos3[0].intValue(),
								appos3[1].intValue(),invp);
			}
	}else {
	// connect transistors to diffusion lines
		for (i=0; i<outs; i++) {
			if (i%2 == 0) {
				ap2 = pmospins[i];
				ap3 = midvddpins[i/2];
			} else {
				ap2 = midvddpins[i/2];
				ap3 = pmospins[i];
			}
			ap1 = ptrans[i];
			apport1 = pmosd1port;
			vddport1 = pmosd2port;
			appos1 = Electric.portPosition(ap1, apport1);
			vddpos1 = Electric.portPosition(ap1, vddport1);
			apport2 = mpacport;
			appos2 = Electric.portPosition(ap2, apport2);
			apport3 = mpacport;
			appos3 = Electric.portPosition(ap3, apport3);
			Electric.newArcInst(pdiffarc, 16*lambda, 0, ap1, apport1,
								appos1[0].intValue(), appos1[1].intValue(),
								ap2, apport2, appos2[0].intValue(),
								appos2[1].intValue(),invp);
			Electric.newArcInst(pdiffarc, 16*lambda, 0, ap1, vddport1,
								vddpos1[0].intValue(), vddpos1[1].intValue(),
								ap3, apport3, appos3[0].intValue(),
								appos3[1].intValue(),invp);
		}
	}

//	metal-1 mpac to mnac
	for (i=0; i<outs/folds; i++) {
		ap1 = nmospins[i];
		apport1 = nmosports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = pmospins[i];
		apport2 = pmosports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),invp);
	}

//	poly inpins to polypins
	for (i=0; i<outs/folds; i++) {
		ap1 = inpins[i];
		apport1 = inports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = polypins[i];
		apport2 = polyports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),invp);
	}

//	poly polypins to intppins
	for (i=0; i<outs/folds; i++) {
		ap1 = polypins[i];
		apport1 = polyports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = intppins[i];
		apport2 = intpports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),invp);
	}

//	poly intppins to ntrans
	for (i=0; i<outs/folds; i++) {
		ap1 = intppins[i];
		apport1 = intpports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = ntrans[i];
		apport2 = nmosg2port;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),invp);
	}

//	poly ntrans to ptrans
	for (i=0; i<outs/folds; i++) {
		ap1 = ntrans[i];
		apport1 = nmosg1port;
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = ptrans[i];
		apport2 = pmosg2port;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc, 2*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
							appos2[1].intValue(),invp);
	}

	if (folds > 1 )  {
		for (i=0; i < outs/folds; i++) {
			ap1 = pwellpins2[i];
			apport1 = pwellports2[i];
			appos1 = Electric.portPosition(ap1, apport1);
			ap2 = gndpins2[i];
			apport2 = gndports2[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),invp);
		}
		for (i=0; i < outs/folds; i++) {
			ap1 = midvddpins2[i];
			apport1 = midvddports2[i];
			appos1 = Electric.portPosition(ap1, apport1);
			ap2 = vddpins2[i];
			apport2 = vddports2[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								
								appos2[0].intValue(), appos2[1].intValue(),invp);
		}

	//	metal 2 vddpins
		ap1 = vddpins2[0];
		apport1 = vddports2[0];
		appos1 = Electric.portPosition(ap1, apport1);
		for (i=1; i<outs/folds; i++) {
			ap2 = vddpins2[i];
			apport2 = vddports2[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),invp);
		}
		p = Electric.newPortProto(invp, ap1, apport1, ("vdd"));
		createExport(p, PWRPORT);

		for (i=0; i<outs/folds; i++) {
			ap1 = gndpins2[i];
			apport1 = gndports2[i];
			p = Electric.newPortProto(invp, ap1, apport1, ("invgnd" + i));
			createExport(p, GNDPORT);
		}
		ap1 = gndpins2[0];
		apport1 = gndports2[0];
		appos1 = Electric.portPosition(ap1, apport1);

	} else {
//		for (i=0; i<outs/2; i++) {
//			ap1 = pwellpins[i];
//			apport1 = pwellports[i];
//			appos1 = Electric.portPosition(ap1, apport1);
//			ap2 = gndpins[i];
//			apport2 = gndports[i];
//			appos2 = Electric.portPosition(ap2, apport2);
//			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
//								appos1[1].intValue(), ap2, apport2,
//								appos2[0].intValue(), appos2[1].intValue(),invp);
//		}

	//	metal 1 midvddpins to vddpins
		for (i=0; i<outs/2; i++) {
			ap1 = midvddpins[i];
			apport1 = midvddports[i];
			appos1 = Electric.portPosition(ap1, apport1);
			ap2 = vddpins[i];
			apport2 = vddports[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m1arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),invp);
		}

		ap1 = vddpins[0];
		apport1 = vddports[0];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = nwellm;
		apport2 = nwellmport;
		appos2 = Electric.portPosition(ap2, apport2);
		ap3 = nwellc;
		apport3 = nwellcport;
		appos3 = Electric.portPosition(ap3, apport3);
		Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
							appos1[1].intValue(), ap2, apport2,
							appos2[0].intValue(), appos2[1].intValue(),invp);
		Electric.newArcInst(m1arc, 4*lambda, 0, ap3, apport3, appos3[0].intValue(),
							appos3[1].intValue(), ap2, apport2,
							appos2[0].intValue(), appos2[1].intValue(),invp);
		


	//	metal 2 vddpins
		ap1 = vddpins[0];
		apport1 = vddports[0];
		appos1 = Electric.portPosition(ap1, apport1);
		for (i=1; i<outs/2; i++) {
			ap2 = vddpins[i];
			apport2 = vddports[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m2arc, 4*lambda, 0, ap1, apport1, appos1[0].intValue(),
								appos1[1].intValue(), ap2, apport2,
								appos2[0].intValue(), appos2[1].intValue(),invp);
		}
		p = Electric.newPortProto(invp, ap1, apport1, ("vdd"));
		createExport(p, PWRPORT);

		for (i=0; i<outs/2; i++) {
			ap1 = gndpins[i];
			apport1 = gndports[i];
			p = Electric.newPortProto(invp, ap1, apport1, ("invgnd" + i));
			createExport(p, GNDPORT);
		}

		ap1 = gndpins[0];
		apport1 = gndports[0];
		appos1 = Electric.portPosition(ap1, apport1);
	}

	ap2 = gndc;
	apport2 = gndcport;
	appos2 = Electric.portPosition(ap2, apport2);
	Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
						appos1[1].intValue(), ap2, apport2, appos2[0].intValue(),
						appos2[1].intValue(),invp);
	for (i=0; i<outs/folds; i++) {
		ap1 = inpins[i];
		apport1 = inports[i];
		p = Electric.newPortProto(invp, ap1, apport1, ("invin" + i));
		createExport(p, INPORT);
	}

	for (i=0; i<outs/folds; i++) {
		ap1 = pmospins[i];
		apport1 = pmosports[i];
		p = Electric.newPortProto(invp,ap1,apport1,("invout"+((outs/folds - 1) - i)));
		createExport(p, OUTPORT);
	}

}
////////////////////inverterplane end

////////////////////ininverterplane
public static void ininverterplane(int lambda, int outs, String layoutname, boolean top,
					   int lengthbits) {	
	int i, m, o;
	int x, y;
	Electric.NodeInst ap1, ap2, ap3, gnd1, gnd2, vdd1, vdd2;
	Electric.PortProto p, apport1, apport2, apport3, trans1port, trans2port,
					   gndport1, gndport2, vddport1, vddport2;
	Integer[] appos1, appos2, appos3, trans1pos, trans2pos, gndpos1, gndpos2,
			  vddpos1, vddpos2;

	Electric.NodeInst[] ntrans = new Electric.NodeInst[outs];
	Electric.NodeInst[] ptrans = new Electric.NodeInst[outs];
	Electric.NodeInst[] inpins = new Electric.NodeInst[outs];
	Electric.NodeInst[] inpins2 = new Electric.NodeInst[outs];
	Electric.NodeInst[] outpins = new Electric.NodeInst[outs];
	Electric.NodeInst[] outpins2 = new Electric.NodeInst[outs];
	Electric.NodeInst[] polypins = new Electric.NodeInst[outs];
	Electric.NodeInst[] intppins = new Electric.NodeInst[outs];
	Electric.NodeInst[] polypins2 = new Electric.NodeInst[outs];
	Electric.NodeInst[] intppins2 = new Electric.NodeInst[outs];
	Electric.NodeInst[] gndpins = new Electric.NodeInst[outs];
	Electric.NodeInst[] midvddpins = new Electric.NodeInst[outs];
	Electric.NodeInst[] nmospins = new Electric.NodeInst[outs];
	Electric.NodeInst[] gndpins2 = new Electric.NodeInst[outs];
	Electric.NodeInst[] vddpins = new Electric.NodeInst[outs];
	Electric.NodeInst[] pmospins = new Electric.NodeInst[outs];
	Electric.NodeInst vddc = new Electric.NodeInst();
	Electric.NodeInst nwellc = new Electric.NodeInst();
	Electric.NodeInst gndc = new Electric.NodeInst();
	Electric.NodeInst pwellc = new Electric.NodeInst();

	Electric.PortProto[] ntransports = new Electric.PortProto[outs];
	Electric.PortProto[] ptransports = new Electric.PortProto[outs];
	Electric.PortProto[] inports = new Electric.PortProto[outs];
	Electric.PortProto[] outports = new Electric.PortProto[outs];
	Electric.PortProto[] inports2 = new Electric.PortProto[outs];
	Electric.PortProto[] outports2 = new Electric.PortProto[outs];
	Electric.PortProto[] polyports = new Electric.PortProto[outs];
	Electric.PortProto[] intpports = new Electric.PortProto[outs];
	Electric.PortProto[] polyports2 = new Electric.PortProto[outs];
	Electric.PortProto[] intpports2 = new Electric.PortProto[outs];
	Electric.PortProto[] nmosports = new Electric.PortProto[outs];
	Electric.PortProto[] pmosports = new Electric.PortProto[outs];
	Electric.PortProto[] gndports = new Electric.PortProto[outs];
	Electric.PortProto[] gndports2 = new Electric.PortProto[outs];
	Electric.PortProto[] vddports = new Electric.PortProto[outs];
	Electric.PortProto[] midvddports = new Electric.PortProto[outs];
	Electric.PortProto vddcport = new Electric.PortProto();
	Electric.PortProto nwellport = new Electric.PortProto();
	Electric.PortProto gndcport = new Electric.PortProto();
	Electric.PortProto pwellport = new Electric.PortProto();
	

	/* get pointers to primitives */
	Electric.NodeProto nmos = Electric.getNodeProto("N-Transistor");
	Electric.PortProto nmosg1port = Electric.getPortProto(nmos, "n-trans-poly-right");
	Electric.PortProto nmosg2port = Electric.getPortProto(nmos, "n-trans-poly-left");
	Electric.PortProto nmosd1port = Electric.getPortProto(nmos, "n-trans-diff-top");
	Electric.PortProto nmosd2port = Electric.getPortProto(nmos, "n-trans-diff-bottom");
	int[] nmosbox = {((Integer)Electric.getVal(nmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(nmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(nmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(nmos, "highy")).intValue()};
				
	Electric.NodeProto pmos = Electric.getNodeProto("P-Transistor");
	Electric.PortProto pmosg1port = Electric.getPortProto(pmos, "p-trans-poly-right");
	Electric.PortProto pmosg2port = Electric.getPortProto(pmos, "p-trans-poly-left");
	Electric.PortProto pmosd1port = Electric.getPortProto(pmos, "p-trans-diff-top");
	Electric.PortProto pmosd2port = Electric.getPortProto(pmos, "p-trans-diff-bottom");
	int[] pmosbox = {((Integer)Electric.getVal(pmos, "lowx")).intValue()-lambda/2,
					 ((Integer)Electric.getVal(pmos, "highx")).intValue()+lambda/2,
					 ((Integer)Electric.getVal(pmos, "lowy")).intValue(),
					 ((Integer)Electric.getVal(pmos, "highy")).intValue()};

	Electric.NodeProto ppin = Electric.getNodeProto("Polysilicon-1-Pin");
	Electric.PortProto ppinport =
		(Electric.PortProto)Electric.getVal(ppin, "firstPortProto");
	int[] ppinbox = {((Integer)Electric.getVal(ppin, "lowx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highx")).intValue(),
					 ((Integer)Electric.getVal(ppin, "lowy")).intValue(),
					 ((Integer)Electric.getVal(ppin, "highy")).intValue()};
	
	Electric.NodeProto napin = Electric.getNodeProto("N-Active-Pin");
	
	Electric.NodeProto m1pin = Electric.getNodeProto("Metal-1-Pin");
	Electric.PortProto m1pinport =
		(Electric.PortProto)Electric.getVal(m1pin, "firstPortProto");
	int[] m1pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto m2pin = Electric.getNodeProto("Metal-2-Pin");
	Electric.PortProto m2pinport =
		(Electric.PortProto)Electric.getVal(m2pin, "firstPortProto");
	int[] m2pinbox = {((Integer)Electric.getVal(m1pin, "lowx")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highx")).intValue()+lambda/2,
					  ((Integer)Electric.getVal(m1pin, "lowy")).intValue()-lambda/2,
					  ((Integer)Electric.getVal(m1pin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto nwnode = Electric.getNodeProto("N-Well-Node");
	int[] nwnodebox =
		{((Integer)Electric.getVal(nwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(nwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(nwnode, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto pwnode = Electric.getNodeProto("P-Well-Node");
	int[] pwnodebox =
		{((Integer)Electric.getVal(pwnode, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(pwnode, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(pwnode, "highy")).intValue()+lambda/2};

	Electric.NodeProto m1m2c = Electric.getNodeProto("Metal-1-Metal-2-Con");
	Electric.PortProto m1m2cport =
		(Electric.PortProto)Electric.getVal(m1m2c, "firstPortProto");
	int[] m1m2cbox = {-5*lambda/2, 5*lambda/2, -5*lambda/2, 5*lambda/2}; 

	Electric.NodeProto diffpin = Electric.getNodeProto("Active-Pin");
	Electric.PortProto diffpinport =
		(Electric.PortProto)Electric.getVal(diffpin, "firstPortProto");
	int[] diffpinbox =
		{((Integer)Electric.getVal(diffpin, "lowx")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highx")).intValue()+lambda/2,
		 ((Integer)Electric.getVal(diffpin, "lowy")).intValue()-lambda/2,
		 ((Integer)Electric.getVal(diffpin, "highy")).intValue()+lambda/2};
	
	Electric.NodeProto mnac = Electric.getNodeProto("Metal-1-N-Active-Con");
	Electric.PortProto mnacport =
		(Electric.PortProto)Electric.getVal(mnac, "firstPortProto");
	int aaa = 17;
	int[] mnacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};
	//centers around 6 goes up by multiples of 2
	
	Electric.NodeProto mpac = Electric.getNodeProto("Metal-1-P-Active-Con");
	Electric.PortProto mpacport =
		(Electric.PortProto)Electric.getVal(mpac, "firstPortProto");
	int[] mpacbox = {-aaa*lambda/2, aaa*lambda/2, -aaa*lambda/2, aaa*lambda/2};
	//centers around 6 goes up by multiples of 2


	int mx = -7;
	int my = 5;
	Electric.NodeProto mpc = Electric.getNodeProto("Metal-1-Polysilicon-1-Con");
	Electric.PortProto mpcport =
		(Electric.PortProto)Electric.getVal(mpc, "firstPortProto");
	int[] mpcbox = {-mx*lambda/2, mx*lambda/2, -my*lambda/2, my*lambda/2}; 

	Electric.NodeProto mpwc = Electric.getNodeProto("Metal-1-P-Well-Con");
	Electric.PortProto mpwcport =
		(Electric.PortProto)Electric.getVal(mpwc, "firstPortProto");
	int[] mpwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};
	
	Electric.NodeProto mnwc = Electric.getNodeProto("Metal-1-N-Well-Con");
	Electric.PortProto mnwcport =
		(Electric.PortProto)Electric.getVal(mnwc, "firstPortProto");
	int[] mnwcbox = {-17*lambda/2, 17*lambda/2, -17*lambda/2, 17*lambda/2};

	Electric.ArcProto parc = Electric.getArcProto("Polysilicon-1");
	Electric.ArcProto m1arc = Electric.getArcProto("Metal-1");
	Electric.ArcProto m2arc = Electric.getArcProto("Metal-2");
	Electric.ArcProto ndiffarc = Electric.getArcProto("N-Active");
	Electric.ArcProto pdiffarc = Electric.getArcProto("P-Active");

	/* create a cell called layoutname+lay}" in the current library */
	Electric.NodeProto ininvp =
		Electric.newNodeProto(layoutname+"{lay}", Electric.curLib());

	Electric.NodeInst pwellnode =
		Electric.newNodeInst(pwnode,-8*lambda,(4*lambda*8*lengthbits)+24*lambda,
									-18*lambda,10*lambda,0,0,ininvp);
	Electric.NodeInst nwellnode =
		Electric.newNodeInst(nwnode,-8*lambda,(4*lambda*8*lengthbits)+24*lambda,
									-36*lambda,-18*lambda,0,0,ininvp);


	// Create instances of objects on input inverter plane
	x = 0;
	for (i=0; i<outs; i++) {
		x += 8*lambda;
		y = 0;
		//place inpins, polypins, gndpins, midvddpins
		gndpins[i] =
			Electric.newNodeInst(mnac,mnacbox[0]+x,mnacbox[1]+x,mnacbox[2]+y-10*lambda,
								 mnacbox[3]+y-10*lambda,0,0,ininvp);
		gndports[i] = mnacport;
		midvddpins[i] =
			Electric.newNodeInst(mpac,mpacbox[0]+x,mpacbox[1]+x,mpacbox[2]+y-26*lambda,
								 mpacbox[3]+y-26*lambda,0,0,ininvp);
		midvddports[i] = mpacport;
		
		int off = 4*lambda;
		ptrans[i] =
			Electric.newNodeInst(pmos,pmosbox[0]+x+off,pmosbox[1]+x+off,
								 pmosbox[2]+y-26*lambda,pmosbox[3]+y-26*lambda,
								 1,0,ininvp);
		ntrans[i] =
			Electric.newNodeInst(nmos,nmosbox[0]+x+off,nmosbox[1]+x+off,
								 nmosbox[2]+y-10*lambda,nmosbox[3]+y-10*lambda,
								 1,0,ininvp);

		//place gnd, intvdd, vddpins
		x += 8*lambda;
		polypins[i] =
			Electric.newNodeInst(ppin,ppinbox[0]+x-8*lambda,ppinbox[1]+x-8*lambda,
								 ppinbox[2]+y-6*lambda,ppinbox[3]+y-6*lambda,
								 0,0,ininvp);
		polyports[i] = ppinport;
		inpins[i] =
			Electric.newNodeInst(mpc,mpcbox[0]+x-14*lambda,mpacbox[1]+x-14*lambda,
								 mpcbox[2]+y,mpcbox[3]+y,0,0,ininvp);
		inports[i] = mpcport;
		if ( top == true ) {
			inpins2[i] =
				Electric.newNodeInst(m1m2c,m1m2cbox[0]+x-8*lambda,
									 m1m2cbox[1]+x-8*lambda,
									 m1m2cbox[2]+y+(8*lambda*(i+1)),
									 m1m2cbox[3]+y+(8*lambda*(i+1)),0,0,ininvp);
			inports2[i] = m1m2cport;
		}	
		intppins[i] =
			Electric.newNodeInst(ppin,ppinbox[0]+x-off,ppinbox[1]+x-off,
								 ppinbox[2]+y-6*lambda,ppinbox[3]+y-6*lambda,
								 0,0,ininvp);
		intpports[i] = ppinport;
		pmospins[i] =
			Electric.newNodeInst(mpac,mpacbox[0]+x,mpacbox[1]+x,mpacbox[2]+y-26*lambda,
								 mpacbox[3]+y-26*lambda,0,0,ininvp);
		pmosports[i] = mpacport;
		vddpins[i] = Electric.newNodeInst(m1m2c,m1m2cbox[0]+x-8*lambda,
										  m1m2cbox[1]+x-8*lambda,
										  m1m2cbox[2]+y-26*lambda,
										  m1m2cbox[3]+y-26*lambda,0,0,ininvp);
		vddports[i] = m1m2cport;
		nmospins[i] =
			Electric.newNodeInst(mnac,mnacbox[0]+x,mnacbox[1]+x,mnacbox[2]+y-10*lambda,
								 mnacbox[3]+y-10*lambda,0,0,ininvp);
		nmosports[i] = mnacport;
		gndpins2[i] =
			Electric.newNodeInst(m1m2c,m1m2cbox[0]+x-8*lambda,m1m2cbox[1]+x-8*lambda,
								 m1m2cbox[2]+y-10*lambda,m1m2cbox[3]+y-10*lambda,
								 0,0,ininvp);
		gndports2[i] = m1m2cport;
		outpins[i] =
			Electric.newNodeInst(mpc,mpcbox[0]+x-14*lambda,mpacbox[1]+x-14*lambda,
								 mpcbox[2]+y-36*lambda,mpcbox[3]+y-36*lambda,
								 0,0,ininvp);
		outports[i] = mpcport;
		if ( top == false ) {
			outpins2[i] =
				Electric.newNodeInst(m1m2c,m1m2cbox[0]+x-8*lambda,
									 m1m2cbox[1]+x-8*lambda,
									 m1m2cbox[2]+y-(8*lambda*(i+1))-36*lambda,
									 m1m2cbox[3]+y-(8*lambda*(i+1))-36*lambda,
									 0,0,ininvp);
			outports2[i] = m1m2cport;
		}
		polypins2[i] =
			Electric.newNodeInst(ppin,ppinbox[0]+x-8*lambda,ppinbox[1]+x-8*lambda,
								 ppinbox[2]+y-30*lambda,ppinbox[3]+y-30*lambda,
								 0,0,ininvp);
		polyports2[i] = ppinport;
		intppins2[i] =
			Electric.newNodeInst(ppin,ppinbox[0]+x-off,ppinbox[1]+x-off,
								 ppinbox[2]+y-30*lambda,ppinbox[3]+y-30*lambda,
								 0,0,ininvp);
		intpports2[i] = ppinport;
		
		if (i == (outs-1)) {
			if (top == true) {
				vddc =
					Electric.newNodeInst(m2pin,m2pinbox[0]+x+12*lambda,
										 m2pinbox[1]+x+12*lambda,
										 m2pinbox[2]+y-26*lambda,
										 m2pinbox[3]+y-26*lambda,0,0,ininvp);
				vddcport = m2pinport;
				gndc =
					Electric.newNodeInst(m1m2c,m1m2cbox[0]+x+12*lambda,
										 m1m2cbox[1]+x+12*lambda,
										 m1m2cbox[2]+y-10*lambda,
										 m1m2cbox[3]+y-10*lambda,0,0,ininvp);
				gndcport = m1m2cport;
				pwellc =
					Electric.newNodeInst(mpwc,mpwcbox[0]+x+12*lambda,
										 mpwcbox[1]+x+12*lambda,mpwcbox[2]+y-10*lambda,
										 mpwcbox[3]+y-10*lambda,0,0,ininvp);
				pwellport = mpwcport;
			}
			else {
				vddc =
					Electric.newNodeInst(m1m2c,m1m2cbox[0]+x+12*lambda,
										 m1m2cbox[1]+x+12*lambda,
										 m1m2cbox[2]+y-26*lambda,
										 m1m2cbox[3]+y-26*lambda,0,0,ininvp);
				vddcport = m1m2cport;
				nwellc =
					Electric.newNodeInst(mnwc,mnwcbox[0]+x+12*lambda,
										 mnwcbox[1]+x+12*lambda,mnwcbox[2]+y-26*lambda,
										 mnwcbox[3]+y-26*lambda,0,0,ininvp);
				nwellport = mnwcport;
				gndc =
					Electric.newNodeInst(m2pin,m2pinbox[0]+x+12*lambda,
										 m2pinbox[1]+x+12*lambda,
										 m2pinbox[2]+y-10*lambda,
										 m2pinbox[3]+y-10*lambda,0,0,ininvp);
				gndcport = m2pinport;
			}
		}
	}

	// connect transistors to diffusion lines
	for (i=0; i<outs; i++) {
		ap2 = gndpins[i];
		ap3 = nmospins[i];
		ap1 = ntrans[i];
		apport1 = nmosd1port;
		gndport1 = nmosd2port;
		appos1 = Electric.portPosition(ap1, apport1);
		gndpos1 = Electric.portPosition(ap1, gndport1);
		apport2 = mnacport;
		appos2 = Electric.portPosition(ap2, apport2);
		apport3 = mnacport;
		appos3 = Electric.portPosition(ap3, apport3);
		// ndiffarc size centers around 12 and goes up by multiples of 2
		Electric.newArcInst(ndiffarc,16*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
		Electric.newArcInst(ndiffarc,16*lambda,0,ap1,gndport1,gndpos1[0].intValue(),
							gndpos1[1].intValue(),ap3,apport3,appos3[0].intValue(),
							appos3[1].intValue(),ininvp);
	}

	// connect transistors to diffusion lines
	for (i=0; i<outs; i++) {
		ap2 = midvddpins[i];
		ap3 = pmospins[i];
		ap1 = ptrans[i];
		apport1 = pmosd1port;
		vddport1 = pmosd2port;
		appos1 = Electric.portPosition(ap1, apport1);
		vddpos1 = Electric.portPosition(ap1, vddport1);
		apport2 = mpacport;
		appos2 = Electric.portPosition(ap2, apport2);
		apport3 = mpacport;
		appos3 = Electric.portPosition(ap3, apport3);
		// pdiffarc size centers around 12 and goes up by multiples of 2
		Electric.newArcInst(pdiffarc,16*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
		Electric.newArcInst(pdiffarc,16*lambda,0,ap1,vddport1,vddpos1[0].intValue(),
							vddpos1[1].intValue(),ap3,apport3,appos3[0].intValue(),
							appos3[1].intValue(),ininvp);
	}

//	metal-1 mpac to mnac
	for (i=0; i<outs; i++) {
		ap1 = nmospins[i];
		apport1 = nmosports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = pmospins[i];
		apport2 = pmosports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

	if (top == true) {
		ap1 = gndc;
		apport1 = gndcport;
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = pwellc;
		apport2 = pwellport;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	} else {
		ap1 = vddc;
		apport1 = vddcport;
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = nwellc;
		apport2 = nwellport;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

	if (top == true) {
		for (i=0; i<outs; i++) {
			ap1 = inpins[i];
			apport1 = inports[i];
			appos1 = Electric.portPosition(ap1, apport1);
			ap2 = inpins2[i];
			apport2 = inports2[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),ininvp);
		}
	} else {
		for (i=0; i<outs; i++) {
			ap1 = outpins[i];
			apport1 = outports[i];
			appos1 = Electric.portPosition(ap1, apport1);
			ap2 = outpins2[i];
			apport2 = outports2[i];
			appos2 = Electric.portPosition(ap2, apport2);
			Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
								appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
								appos2[1].intValue(),ininvp);
		}
	}

//	poly inpins to polypins
	for (i=0; i<outs; i++) {
		ap1 = inpins[i];
		apport1 = inports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = polypins[i];
		apport2 = polyports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	poly polypins to intppins
	for (i=0; i<outs; i++) {
		ap1 = polypins[i];
		apport1 = polyports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = intppins[i];
		apport2 = intpports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	poly intppins to ntrans
	for (i=0; i<outs; i++) {
		ap1 = intppins[i];
		apport1 = intpports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = ntrans[i];
		apport2 = nmosg2port;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	poly ntrans to ptrans
	for (i=0; i<outs; i++) {
		ap1 = ntrans[i];
		apport1 = nmosg1port;
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = ptrans[i];
		apport2 = pmosg2port;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}
	
	//	poly outpins to polypins
	for (i=0; i<outs; i++) {
		ap1 = outpins[i];
		apport1 = outports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = polypins2[i];
		apport2 = polyports2[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	poly polypins to intppins
	for (i=0; i<outs; i++) {
		ap1 = polypins2[i];
		apport1 = polyports2[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = intppins2[i];
		apport2 = intpports2[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	poly outtppins to ptrans
	for (i=0; i<outs; i++) {
		ap1 = intppins2[i];
		apport1 = intpports2[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = ptrans[i];
		apport2 = pmosg1port;
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(parc,2*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	metal 1 pmospins to vddpins
	for (i=0; i<outs; i++) {
		ap1 = midvddpins[i];
		apport1 = midvddports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = vddpins[i];
		apport2 = vddports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	metal 1 nmospins to nmospins2
	for (i=0; i<outs; i++) {
		ap1 = gndpins[i];
		apport1 = gndports[i];
		appos1 = Electric.portPosition(ap1, apport1);
		ap2 = gndpins2[i];
		apport2 = gndports2[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m1arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	metal 2 nmospins
	ap1 = gndpins2[0];
	apport1 = gndports2[0];
	appos1 = Electric.portPosition(ap1, apport1);
	for (i=1; i<outs; i++) {
		ap2 = gndpins2[i];
		apport2 = gndports2[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m2arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

//	metal 2 vddpins
	ap1 = vddpins[0];
	apport1 = vddports[0];
	appos1 = Electric.portPosition(ap1, apport1);
	for (i=1; i<outs; i++) {
		ap2 = vddpins[i];
		apport2 = vddports[i];
		appos2 = Electric.portPosition(ap2, apport2);
		Electric.newArcInst(m2arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
							appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
							appos2[1].intValue(),ininvp);
	}

	ap1 = vddpins[0];
	apport1 = vddports[0];
	appos1 = Electric.portPosition(ap1, apport1);
	ap2 = vddc;
	apport2 = vddcport;
	appos2 = Electric.portPosition(ap2, apport2);
	Electric.newArcInst(m2arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
						appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
						appos2[1].intValue(),ininvp);
	p = Electric.newPortProto(ininvp, ap2, apport2, ("vdd"));
	createExport(p, PWRPORT);

	ap1 = gndpins2[0];
	apport1 = gndports2[0];
	appos1 = Electric.portPosition(ap1, apport1);
	ap2 = gndc;
	apport2 = gndcport;
	appos2 = Electric.portPosition(ap2, apport2);
	Electric.newArcInst(m2arc,4*lambda,0,ap1,apport1,appos1[0].intValue(),
						appos1[1].intValue(),ap2,apport2,appos2[0].intValue(),
						appos2[1].intValue(),ininvp);
	p = Electric.newPortProto(ininvp, ap2, apport2, ("gnd"));
	createExport(p, GNDPORT);

	if ( top == true) {
		for (i=0; i<outs; i++) {
			ap1 = inpins2[i];
			apport1 = inports2[i];
			 p = Electric.newPortProto(ininvp, ap1, apport1, ("in_top" + i));
			createExport(p, INPORT);
		}
	}else {
		for (i=0; i<outs; i++) {
			ap1 = inpins[i];
			apport1 = inports[i];
			p = Electric.newPortProto(ininvp, ap1, apport1, ("in_top" + i));
			createExport(p, INPORT);
		}
	}
	if ( top == true ) {
		for (i=0; i<outs; i++) {
			ap1 = outpins[i];
			apport1 = outports[i];
			p = Electric.newPortProto(ininvp, ap1, apport1, ("in_bot" + i));
			createExport(p, INPORT);
		}
	} else  {
		for (i=0; i<outs; i++) {
			ap1 = outpins2[i];
			apport1 = outports2[i];
			p = Electric.newPortProto(ininvp, ap1, apport1, ("in_bot" + i));
			createExport(p, INPORT);
		}
	}

	for (i=0; i<outs; i++) {
		ap1 = pmospins[i];
		apport1 = pmosports[i];
		p = Electric.newPortProto(ininvp, ap1, apport1, ("in_b" + i));
		createExport(p, INPORT);
	}

}
////////////////////ininverterplane end

}
