/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dblangjava.cpp
 * Java interface module
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
#if LANGJAVA

#include "global.h"
#include "dblang.h"
#include "usr.h"

#define MAXLINE 300
/* #define OLDJAVA 1 */		/* uncomment to force JDK 1.1 */
#ifdef WIN32
#  define PATHSEP ';'
#else
#  define PATHSEP ':'
#endif

INTBIG          java_inited = 0;
JavaVM         *java_virtualmachine;			/* denotes a Java VM */
JNIEnv         *java_environment;				/* pointer to native method interface */
jclass          java_electricclass;
jclass          java_electriceoutclass;
jfieldID        java_addressID;
jfieldID        java_xarrayVID;
jobject         java_nullobject;
CHAR           *java_arraybuffer;
INTBIG          java_arraybuffersize = 0;

/* for the Bean Shell */
jobject         java_bshInterpreterObject;
jmethodID       java_bshEvalMID;

CHAR            java_outputbuffer[MAXLINE+1];
INTBIG          java_outputposition = 0;

jclass          java_classstring;
jclass          java_classint;
jclass          java_classfloat;
jclass          java_classdouble;
jclass          java_classarrayint;
jmethodID       java_midIntValue, java_midIntInit;
jmethodID       java_midFloatValue, java_midFloatInit;
jmethodID       java_midDoubleValue;

jclass          java_classnodeinst;
jclass          java_classnodeproto;
jclass          java_classportarcinst;
jclass          java_classportexpinst;
jclass          java_classportproto;
jclass          java_classarcinst;
jclass          java_classarcproto;
jclass          java_classgeom;
jclass          java_classlibrary;
jclass          java_classtechnology;
jclass          java_classtool;
jclass          java_classrtnode;
jclass          java_classnetwork;
jclass          java_classview;
jclass          java_classwindowpart;
jclass          java_classwindowframe;
jclass          java_classgraphics;
jclass          java_classconstraint;
jclass          java_classpolygon;
jclass          java_classxarray;

extern "C"
{
	JNIEXPORT jobject JNICALL Java_Electric_curLib(JNIEnv *env, jobject obj);
	JNIEXPORT jobject JNICALL Java_Electric_curTech(JNIEnv *env, jobject obj);
	JNIEXPORT jobject JNICALL Java_Electric_getValNodeinst(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValNodeproto(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValPortarcinst(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValPortexpinst(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValPortproto(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValArcinst(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValArcproto(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValGeom(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValLibrary(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValTechnology(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValTool(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValRTNode(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValNetwork(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValView(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValWindowpart(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValWindowframe(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValGraphics(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValConstraint(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getValPolygon(JNIEnv *env, jobject obj,
		jobject object, jstring qual);
	JNIEXPORT jobject JNICALL Java_Electric_getParentVal(JNIEnv *env, jobject obj,
		jstring jname, jobject jdefault, jint jheight);
	JNIEXPORT void JNICALL Java_Electric_setVal(JNIEnv *env, jobject obj,
		jobject jobj, jstring jqual, jobject jattr, jint jbits);
	JNIEXPORT void JNICALL Java_Electric_setInd(JNIEnv *env, jobject obj,
		jobject jobj, jstring jname, jint index, jobject jattr);
	JNIEXPORT void JNICALL Java_Electric_delVal(JNIEnv *env, jobject obj,
		jobject jobj, jstring jname);
	JNIEXPORT jint JNICALL Java_Electric_initSearch(JNIEnv *env, jobject obj,
		jint lx, jint hx, jint ly, jint hy, jobject cell);
	JNIEXPORT jobject JNICALL Java_Electric_nextObject(JNIEnv *env, jobject obj,
		jint sea);
	JNIEXPORT void JNICALL Java_Electric_termSearch(JNIEnv *env, jobject obj,
		jint sea);
	JNIEXPORT jobject JNICALL Java_Electric_getTool(JNIEnv *env, jobject obj,
		jstring name);
	JNIEXPORT jint JNICALL Java_Electric_maxTool(JNIEnv *env, jobject obj);
	JNIEXPORT jobject JNICALL Java_Electric_indexTool(JNIEnv *env, jobject obj,
		jint index);
	JNIEXPORT void JNICALL Java_Electric_toolTurnOn(JNIEnv *env, jobject obj,
		jobject tool);
	JNIEXPORT void JNICALL Java_Electric_toolTurnOff(JNIEnv *env, jobject obj,
		jobject tool);
	JNIEXPORT void JNICALL Java_Electric_tellTool(JNIEnv *env, jobject obj,
		jobject tool, jint argc, jobjectArray jargv);
	JNIEXPORT jobject JNICALL Java_Electric_getLibrary(JNIEnv *env, jobject obj,
		jstring name);
	JNIEXPORT jobject JNICALL Java_Electric_newLibrary(JNIEnv *env, jobject obj,
		jstring libname, jstring libfile);
	JNIEXPORT void JNICALL Java_Electric_killLibrary(JNIEnv *env, jobject obj,
		jobject lib);
	JNIEXPORT void JNICALL Java_Electric_eraseLibrary(JNIEnv *env, jobject obj,
		jobject lib);
	JNIEXPORT void JNICALL Java_Electric_selectLibrary(JNIEnv *env, jobject obj,
		jobject lib);
	JNIEXPORT jobject JNICALL Java_Electric_getNodeProto(JNIEnv *env, jobject obj,
		jstring name);
	JNIEXPORT jobject JNICALL Java_Electric_newNodeProto(JNIEnv *env, jobject obj,
		jstring name, jobject lib);
	JNIEXPORT jint JNICALL Java_Electric_killNodeProto(JNIEnv *env, jobject obj,
		jobject cell);
	JNIEXPORT jobject JNICALL Java_Electric_copyNodeProto(JNIEnv *env, jobject obj,
		jobject cell, jobject lib, jstring name);
	JNIEXPORT jobject JNICALL Java_Electric_iconView(JNIEnv *env, jobject obj,
		jobject cell);
	JNIEXPORT jobject JNICALL Java_Electric_contentsView(JNIEnv *env, jobject obj,
		jobject cell);
	JNIEXPORT jobject JNICALL Java_Electric_newNodeInst(JNIEnv *env, jobject obj,
		jobject proto, jint lx, jint hx, jint ly, jint hy, jint trans, jint rot, jobject cell);
	JNIEXPORT void JNICALL Java_Electric_modifyNodeInst(JNIEnv *env, jobject obj,
		jobject node, jint dlx, jint dly, jint dhx, jint dhy, jint drot, jint dtrans);
	JNIEXPORT jint JNICALL Java_Electric_killNodeInst(JNIEnv *env, jobject obj,
		jobject node);
	JNIEXPORT jobject JNICALL Java_Electric_replaceNodeInst(JNIEnv *env, jobject obj,
		jobject node, jobject proto);
	JNIEXPORT jint JNICALL Java_Electric_nodeFunction(JNIEnv *env, jobject obj,
		jobject node);
	JNIEXPORT jint JNICALL Java_Electric_nodePolys(JNIEnv *env, jobject obj,
		jobject node);
	JNIEXPORT jobject JNICALL Java_Electric_shapeNodePoly(JNIEnv *env, jobject obj,
		jobject node, jint jindex);
	JNIEXPORT jint JNICALL Java_Electric_nodeEPolys(JNIEnv *env, jobject obj,
		jobject node);
	JNIEXPORT jobject JNICALL Java_Electric_shapeENodePoly(JNIEnv *env, jobject obj,
		jobject node, jint jindex);
	JNIEXPORT jobject JNICALL Java_Electric_makeRot(JNIEnv *env, jobject obj,
		jobject node);
	JNIEXPORT jobject JNICALL Java_Electric_makeTrans(JNIEnv *env, jobject obj,
		jobject node);
	JNIEXPORT jintArray JNICALL Java_Electric_nodeProtoSizeOffset(JNIEnv *env, jobject obj,
		jobject jnp);
	JNIEXPORT jobject JNICALL Java_Electric_newArcInst(JNIEnv *env, jobject obj,
		jobject proto, jint wid, jint bits, jobject node1, jobject port1, jint x1, jint y1,
		jobject node2, jobject port2, jint x2, jint y2, jobject cell);
	JNIEXPORT jint JNICALL Java_Electric_modifyArcInst(JNIEnv *env, jobject obj,
		jobject arc, jint dwid, jint dx1, jint dy1, jint dx2, jint dy2);
	JNIEXPORT jint JNICALL Java_Electric_killArcInst(JNIEnv *env, jobject obj,
		jobject arc);
	JNIEXPORT jobject JNICALL Java_Electric_replaceArcInst(JNIEnv *env, jobject obj,
		jobject arc, jobject proto);
	JNIEXPORT jint JNICALL Java_Electric_arcPolys(JNIEnv *env, jobject obj,
		jobject arc);
	JNIEXPORT jobject JNICALL Java_Electric_shapeArcPoly(JNIEnv *env, jobject obj,
		jobject arc, jint jindex);
	JNIEXPORT jint JNICALL Java_Electric_arcProtoWidthOffset(JNIEnv *env, jobject obj,
		jobject jap);
	JNIEXPORT jobject JNICALL Java_Electric_newPortProto(JNIEnv *env, jobject obj,
		jobject cell, jobject node, jobject port, jstring name);
	JNIEXPORT jobjectArray JNICALL Java_Electric_portPosition(JNIEnv *env, jobject obj,
		jobject jni, jobject jpp);
	JNIEXPORT jobject JNICALL Java_Electric_getPortProto(JNIEnv *env, jobject obj,
		jobject cell, jstring name);
	JNIEXPORT jint JNICALL Java_Electric_killPortProto(JNIEnv *env, jobject obj,
		jobject cell, jobject port);
	JNIEXPORT jint JNICALL Java_Electric_movePortProto(JNIEnv *env, jobject obj,
		jobject cell, jobject oldport, jobject newnode, jobject newport);
	JNIEXPORT jobject JNICALL Java_Electric_shapePortPoly(JNIEnv *env, jobject obj,
		jobject jnode, jobject jport);
	JNIEXPORT jint JNICALL Java_Electric_undoABatch(JNIEnv *env, jobject obj);
	JNIEXPORT void JNICALL Java_Electric_noUndoAllowed(JNIEnv *env, jobject obj);
	JNIEXPORT void JNICALL Java_Electric_flushChanges(JNIEnv *env, jobject obj);
	JNIEXPORT jobject JNICALL Java_Electric_getView(JNIEnv *env, jobject obj,
		jstring name);
	JNIEXPORT jobject JNICALL Java_Electric_newView(JNIEnv *env, jobject obj,
		jstring name, jstring sname);
	JNIEXPORT jint JNICALL Java_Electric_killView(JNIEnv *env, jobject obj,
		jobject view);
	JNIEXPORT jobject JNICALL Java_Electric_getArcProto(JNIEnv *env, jobject obj,
		jstring name);
	JNIEXPORT jobject JNICALL Java_Electric_getTechnology(JNIEnv *env, jobject obj,
		jstring name);
	JNIEXPORT jobject JNICALL Java_Electric_getPinProto(JNIEnv *env, jobject obj,
		jobject arcproto);
	JNIEXPORT jobject JNICALL Java_Electric_getNetwork(JNIEnv *env, jobject obj,
		jstring name, jobject cell);
	JNIEXPORT jstring JNICALL Java_Electric_layerName(JNIEnv *env, jobject obj,
		jobject jtech, jint jlayer);
	JNIEXPORT jint JNICALL Java_Electric_layerFunction(JNIEnv *env, jobject obj,
		jobject jtech, jint jlayer);
	JNIEXPORT jint JNICALL Java_Electric_maxDRCSurround(JNIEnv *env, jobject obj,
		jobject jtech, jobject jlib, jint jlayer);
	JNIEXPORT jint JNICALL Java_Electric_DRCMinDistance(JNIEnv *env, jobject obj,
		jobject jtech, jobject jlib, jint jlayer1, jint jlayer2, jint jconnected);
	JNIEXPORT jint JNICALL Java_Electric_DRCMinWidth(JNIEnv *env, jobject obj,
		jobject jtech, jobject jlib, jint jlayer);
	JNIEXPORT void JNICALL Java_Electric_xformPoly(JNIEnv *env, jobject obj,
		jobject jpoly, jobject jtrans);
	JNIEXPORT jobject JNICALL Java_Electric_transMult(JNIEnv *env, jobject obj,
		jobject jtransa, jobject jtransb);
	JNIEXPORT void JNICALL Java_Electric_freePolygon(JNIEnv *env, jobject obj,
		jobject jpoly);
	JNIEXPORT void JNICALL Java_Electric_beginTraverseHierarchy(JNIEnv *env, jobject obj);
	JNIEXPORT void JNICALL Java_Electric_downHierarchy(JNIEnv *env, jobject obj,
		jobject jni, jint jindex);
	JNIEXPORT void JNICALL Java_Electric_upHierarchy(JNIEnv *env, jobject obj);
	JNIEXPORT void JNICALL Java_Electric_endTraverseHierarchy(JNIEnv *env, jobject obj);
	JNIEXPORT jobject JNICALL Java_Electric_getTraversalPath(JNIEnv *env, jobject obj);

	JNIEXPORT void JNICALL Java_Electric_eoutWriteOne(JNIEnv *env, jobject obj,
		jint byte);
	JNIEXPORT void JNICALL Java_Electric_eoutWriteString(JNIEnv *env, jobject obj,
		jstring string);
}

static JNINativeMethod electricmethods[] =
{
	/***************** DATABASE EXAMINATION ROUTINES *****************/
	{b_("curLib"),
		b_("()LCOM/staticfreesoft/Electric$Library;"),
		(void *)Java_Electric_curLib},
	{b_("curTech"),
		b_("()LCOM/staticfreesoft/Electric$Technology;"),
		(void *)Java_Electric_curTech},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValNodeinst},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValNodeproto},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$PortArcInst;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValPortarcinst},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$PortExpInst;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValPortexpinst},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$PortProto;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValPortproto},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$ArcInst;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValArcinst},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$ArcProto;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValArcproto},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Geom;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValGeom},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Library;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValLibrary},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Technology;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValTechnology},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Tool;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValTool},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$RTNode;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValRTNode},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Network;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValNetwork},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$View;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValView},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$WindowPart;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValWindowpart},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$WindowFrame;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValWindowframe},
	{b_("getVal"),
		 b_("(LCOM/staticfreesoft/Electric$Graphics;Ljava/lang/String;)Ljava/lang/Object;"),
		 (void *)Java_Electric_getValGraphics},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Constraint;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValConstraint},
	{b_("getVal"),
		b_("(LCOM/staticfreesoft/Electric$Polygon;Ljava/lang/String;)Ljava/lang/Object;"),
		(void *)Java_Electric_getValPolygon},
	{b_("getParentVal"),
		b_("(Ljava/lang/String;Ljava/lang/Object;I)Ljava/lang/Object;"),
		(void *)Java_Electric_getParentVal},
	{b_("setVal"),
		b_("(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/Object;I)V"),
		(void *)Java_Electric_setVal},
	{b_("setInd"),
		b_("(Ljava/lang/Object;Ljava/lang/String;ILjava/lang/Object;)V"),
		(void *)Java_Electric_setInd},
	{b_("delVal"),
		b_("(Ljava/lang/Object;Ljava/lang/String;)V"),
		(void *)Java_Electric_delVal},
	{b_("initSearch"),
		b_("(IIIILCOM/staticfreesoft/Electric$NodeProto;)I"),
		(void *)Java_Electric_initSearch},
	{b_("nextObject"),
		b_("(I)Ljava/lang/Object;"),
		(void *)Java_Electric_nextObject},
	{b_("termSearch"),
		b_("(I)V"),
		(void *)Java_Electric_termSearch},

	/***************** TOOL ROUTINES *****************/
	{b_("getTool"),
		b_("(Ljava/lang/String;)LCOM/staticfreesoft/Electric$Tool;"),
		(void *)Java_Electric_getTool},
	{b_("maxTool"),
		b_("()I"),
		(void *)Java_Electric_maxTool},
	{b_("indexTool"),
		b_("(I)LCOM/staticfreesoft/Electric$Tool;"),
		(void *)Java_Electric_indexTool},
	{b_("toolTurnOn"),
		b_("(LCOM/staticfreesoft/Electric$Tool;)V"),
		(void *)Java_Electric_toolTurnOn},
	{b_("toolTurnOff"),
		b_("(LCOM/staticfreesoft/Electric$Tool;)V"),
		(void *)Java_Electric_toolTurnOff},
	{b_("tellTool"),
		b_("(LCOM/staticfreesoft/Electric$Tool;I[Ljava/lang/String;)V"),
		(void *)Java_Electric_tellTool},

	/***************** LIBRARY ROUTINES *****************/
	{b_("getLibrary"),
		b_("(Ljava/lang/String;)LCOM/staticfreesoft/Electric$Library;"),
		(void *)Java_Electric_getLibrary},
	{b_("newLibrary"),
		b_("(Ljava/lang/String;Ljava/lang/String;)LCOM/staticfreesoft/Electric$Library;"),
		(void *)Java_Electric_newLibrary},
	{b_("killLibrary"),
		b_("(LCOM/staticfreesoft/Electric$Library;)V"),
		(void *)Java_Electric_killLibrary},
	{b_("eraseLibrary"),
		b_("(LCOM/staticfreesoft/Electric$Library;)V"),
		(void *)Java_Electric_eraseLibrary},
	{b_("selectLibrary"),
		b_("(LCOM/staticfreesoft/Electric$Library;)V"),
		(void *)Java_Electric_selectLibrary},

	/***************** NODEPROTO ROUTINES *****************/
	{b_("getNodeProto"),
		b_("(Ljava/lang/String;)LCOM/staticfreesoft/Electric$NodeProto;"),
		(void *)Java_Electric_getNodeProto},
	{b_("newNodeProto"),
		b_("(Ljava/lang/String;LCOM/staticfreesoft/Electric$Library;)LCOM/staticfreesoft/Electric$NodeProto;"),
		(void *)Java_Electric_newNodeProto},
	{b_("killNodeProto"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;)I"),
		(void *)Java_Electric_killNodeProto},
	{b_("copyNodeProto"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;LCOM/staticfreesoft/Electric$Library;Ljava/lang/String;)LCOM/staticfreesoft/Electric$NodeProto;"),
		(void *)Java_Electric_copyNodeProto},
	{b_("iconView"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;)LCOM/staticfreesoft/Electric$NodeProto;"),
		(void *)Java_Electric_iconView},
	{b_("contentsView"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;)LCOM/staticfreesoft/Electric$NodeProto;"),
		(void *)Java_Electric_contentsView},

	/***************** NODEINST ROUTINES *****************/
	{b_("newNodeInst"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;IIIIIILCOM/staticfreesoft/Electric$NodeProto;)LCOM/staticfreesoft/Electric$NodeInst;"),
		(void *)Java_Electric_newNodeInst},
	{b_("modifyNodeInst"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;IIIIII)V"),
		(void *)Java_Electric_modifyNodeInst},
	{b_("killNodeInst"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;)I"),
		(void *)Java_Electric_killNodeInst},
	{b_("replaceNodeInst"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$NodeProto;)LCOM/staticfreesoft/Electric$NodeInst;"),
		(void *)Java_Electric_replaceNodeInst},
	{b_("nodeFunction"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;)I"),
		(void *)Java_Electric_nodeFunction},
	{b_("nodePolys"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;)I"),
		(void *)Java_Electric_nodePolys},
	{b_("shapeNodePoly"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;I)LCOM/staticfreesoft/Electric$Polygon;"),
		(void *)Java_Electric_shapeNodePoly},
	{b_("nodeEPolys"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;)I"),
		(void *)Java_Electric_nodeEPolys},
	{b_("shapeENodePoly"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;I)LCOM/staticfreesoft/Electric$Polygon;"),
		(void *)Java_Electric_shapeENodePoly},
	{b_("makeRot"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;)LCOM/staticfreesoft/Electric$XArray;"),
		(void *)Java_Electric_makeRot},
	{b_("makeTrans"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;)LCOM/staticfreesoft/Electric$XArray;"),
		(void *)Java_Electric_makeTrans},
	{b_("nodeProtoSizeOffset"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;)[I"),
		(void *)Java_Electric_nodeProtoSizeOffset},

	/***************** ARCINST ROUTINES *****************/
	{b_("newArcInst"),
		b_("(LCOM/staticfreesoft/Electric$ArcProto;IILCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$PortProto;IILCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$PortProto;IILCOM/staticfreesoft/Electric$NodeProto;)LCOM/staticfreesoft/Electric$ArcInst;"),
		(void *)Java_Electric_newArcInst},
	{b_("modifyArcInst"),
		 b_("(LCOM/staticfreesoft/Electric$ArcInst;IIIII)I"),
		 (void *)Java_Electric_modifyArcInst},
	{b_("killArcInst"),
		b_("(LCOM/staticfreesoft/Electric$ArcInst;)I"),
		(void *)Java_Electric_killArcInst},
	{b_("replaceArcInst"),
		 b_("(LCOM/staticfreesoft/Electric$ArcInst;LCOM/staticfreesoft/Electric$ArcProto;)LCOM/staticfreesoft/Electric$ArcInst;"),
		 (void *)Java_Electric_replaceArcInst},
	{b_("arcPolys"),
		b_("(LCOM/staticfreesoft/Electric$ArcInst;)I"),
		(void *)Java_Electric_arcPolys},
	{b_("shapeArcPoly"),
		b_("(LCOM/staticfreesoft/Electric$ArcInst;I)LCOM/staticfreesoft/Electric$Polygon;"),
		(void *)Java_Electric_shapeArcPoly},
	{b_("arcProtoWidthOffset"),
		b_("(LCOM/staticfreesoft/Electric$ArcProto;)I"),
		(void *)Java_Electric_arcProtoWidthOffset},

	/***************** PORTPROTO ROUTINES *****************/
	{b_("newPortProto"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;LCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$PortProto;Ljava/lang/String;)LCOM/staticfreesoft/Electric$PortProto;"),
		(void *)Java_Electric_newPortProto},
	{b_("portPosition"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$PortProto;)[Ljava/lang/Integer;"),
		(void *)Java_Electric_portPosition},
	{b_("getPortProto"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;Ljava/lang/String;)LCOM/staticfreesoft/Electric$PortProto;"),
		(void *)Java_Electric_getPortProto},
	{b_("killPortProto"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;LCOM/staticfreesoft/Electric$PortProto;)I"),
		(void *)Java_Electric_killPortProto},
	{b_("movePortProto"),
		b_("(LCOM/staticfreesoft/Electric$NodeProto;LCOM/staticfreesoft/Electric$PortProto;LCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$PortProto;)I"),
		(void *)Java_Electric_movePortProto},
	{b_("shapePortPoly"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$PortProto;)LCOM/staticfreesoft/Electric$Polygon;"),
		(void *)Java_Electric_shapePortPoly},

	/***************** CHANGE CONTROL ROUTINES *****************/
	{b_("undoABatch"),
		b_("()I"),
		(void *)Java_Electric_undoABatch},
	{b_("noUndoAllowed"),
		 b_("()V"),
		 (void *)Java_Electric_noUndoAllowed},
	{b_("flushChanges"),
		 b_("()V"),
		 (void *)Java_Electric_flushChanges},

	/***************** VIEW ROUTINES *****************/
	{b_("getView"),
		b_("(Ljava/lang/String;)LCOM/staticfreesoft/Electric$View;"),
		(void *)Java_Electric_getView},
	{b_("newView"),
		b_("(Ljava/lang/String;Ljava/lang/String;)LCOM/staticfreesoft/Electric$View;"),
		(void *)Java_Electric_newView},
	{b_("killView"),
		b_("(LCOM/staticfreesoft/Electric$View;)I"),
		(void *)Java_Electric_killView},

	/***************** MISCELLANEOUS ROUTINES *****************/
	{b_("getArcProto"),
		b_("(Ljava/lang/String;)LCOM/staticfreesoft/Electric$ArcProto;"),
		(void *)Java_Electric_getArcProto},
	{b_("getTechnology"),
		b_("(Ljava/lang/String;)LCOM/staticfreesoft/Electric$Technology;"),
		(void *)Java_Electric_getTechnology},
	{b_("getPinProto"),
		b_("(LCOM/staticfreesoft/Electric$ArcProto;)LCOM/staticfreesoft/Electric$NodeProto;"),
		(void *)Java_Electric_getPinProto},
	{b_("getNetwork"),
		b_("(Ljava/lang/String;LCOM/staticfreesoft/Electric$NodeProto;)LCOM/staticfreesoft/Electric$Network;"),
		(void *)Java_Electric_getNetwork},
	{b_("layerName"),
		b_("(LCOM/staticfreesoft/Electric$Technology;I)Ljava/lang/String;"),
		(void *)Java_Electric_layerName},
	{b_("layerFunction"),
		b_("(LCOM/staticfreesoft/Electric$Technology;I)I"),
		(void *)Java_Electric_layerFunction},
	{b_("maxDRCSurround"),
		b_("(LCOM/staticfreesoft/Electric$Technology;LCOM/staticfreesoft/Electric$Library;I)I"),
		(void *)Java_Electric_maxDRCSurround},
	{b_("DRCMinDistance"),
		b_("(LCOM/staticfreesoft/Electric$Technology;LCOM/staticfreesoft/Electric$Library;III)I"),
		(void *)Java_Electric_DRCMinDistance},
	{b_("DRCMinWidth"),
		b_("(LCOM/staticfreesoft/Electric$Technology;LCOM/staticfreesoft/Electric$Library;I)I"),
		(void *)Java_Electric_DRCMinWidth},
	{b_("xformPoly"),
		b_("(LCOM/staticfreesoft/Electric$Polygon;LCOM/staticfreesoft/Electric$XArray;)V"),
		(void *)Java_Electric_xformPoly},
	{b_("transMult"),
		b_("(LCOM/staticfreesoft/Electric$XArray;LCOM/staticfreesoft/Electric$XArray;)LCOM/staticfreesoft/Electric$XArray;"),
		(void *)Java_Electric_transMult},
	{b_("freePolygon"),
		b_("(LCOM/staticfreesoft/Electric$Polygon;)V"),
		(void *)Java_Electric_freePolygon},
	{b_("beginTraverseHierarchy"),
		b_("()V"),
		(void *)Java_Electric_beginTraverseHierarchy},
	{b_("downHierarchy"),
		b_("(LCOM/staticfreesoft/Electric$NodeInst;I)V"),
		(void *)Java_Electric_downHierarchy},
	{b_("upHierarchy"),
		b_("()V"),
		(void *)Java_Electric_upHierarchy},
	{b_("endTraverseHierarchy"),
		b_("()V"),
		(void *)Java_Electric_endTraverseHierarchy},
	{b_("getTraversalPath"),
		b_("()[LCOM/staticfreesoft/Electric$NodeInst;"),
		(void *)Java_Electric_getTraversalPath}
};

static JNINativeMethod electriceoutmethods[] =
{
	{b_("write"),
		b_("(I)V"),
		(void *)Java_Electric_eoutWriteOne},
	{b_("ewrite"),
		b_("(Ljava/lang/String;)V"),
		(void *)Java_Electric_eoutWriteString}
};

/* prototypes for local routines */
static jint JNICALL java_vfprintf(FILE *fp, const CHAR1 *format, va_list args);
static void JNICALL java_exit(jint code);
static jobject java_getval(jobject obj, INTBIG type, jstring jname);
static void java_addcharacter(CHAR chr);
static void java_getobjectaddrtype(jobject obj, INTBIG *addr, INTBIG *type, CHAR **description);
static BOOLEAN java_allocarraybuffer(INTBIG size);
static jobject java_makejavaobject(INTBIG addr, INTBIG type, jclass javaclass, INTBIG desttype);
static BOOLEAN java_reportexceptions(void);
static CHAR *java_finddbmirror(void); /* added by Mike Wessler on 5/20/02 */
static CHAR *java_findbeanshell(void);
static void java_dumpclass(jclass theClass);

/****************************** ELECTRIC INTERFACE ******************************/

/*
 * Routine to free all memory associated with this module.
 */
void java_freememory(void)
{
	if (java_arraybuffersize > 0)
		efree((CHAR *)java_arraybuffer);
}

CHAR *java_init(void)
{
	CHAR *userclasspath, *allocatedpath;
	CHAR1 *properties[10];
	static CHAR *bsh, *classpath, *dbmirror;
	INTBIG ac, err, i, noptions;
	jclass interpreterClass, classClz;
	jmethodID mid;
	jstring string;
	REGISTER void *infstr;
	static CHAR *bshInit[] =
	{
		x_("E = new COM.staticfreesoft.Electric();"),
		x_("Object P(String par) { return E.getParentVal(\"ATTR_\"+par, new Integer(0), 1); }"),
		x_("Object PD(String par, int def) { return E.getParentVal(\"ATTR_\"+par, new Integer(def), 1); }"),
		x_("Object PD(String par, float def) { return E.getParentVal(\"ATTR_\"+par, new Float(def), 1); }"),
		x_("Object PD(String par, Object def) { return E.getParentVal(\"ATTR_\"+par, def, 1); }"),
		x_("Object PAR(String par) { return E.getParentVal(\"ATTR_\"+par, new Integer(0), 0); }"),
		x_("Object PARD(String par, int def) { return E.getParentVal(\"ATTR_\"+par, new Integer(def), 0); }"),
		x_("Object PARD(String par, float def) { return E.getParentVal(\"ATTR_\"+par, new Float(def), 0); }"),
		x_("Object PARD(String par, Object def) { return E.getParentVal(\"ATTR_\"+par, def, 0); }"),
		0
	};
	JDK1_1InitArgs vm1args;
	JavaVMInitArgs vm2args;
	JavaVMOption joptions[10];

	if (java_inited != 0)
	{
		if (bsh != 0) return(x_("Java Bean Shell"));
		return(x_("Java"));
	}

	/* construct the class path */
	infstr = initinfstr();

	/* add the environment variable CLASSPATH if it exists */
	userclasspath = egetenv(x_("CLASSPATH"));
	if (userclasspath != 0)
		formatinfstr(infstr, x_("%c%s"), PATHSEP, userclasspath);

	/* add the Bean Shell to the class path if it exists */
	bsh = java_findbeanshell();
	if (bsh != 0)
		formatinfstr(infstr, x_("%c%sjava%c%s"), PATHSEP, el_libdir, DIRSEP, bsh);

	/* add the dbmirror to the class path if it exists (MW 5/20/02) */
	dbmirror = java_finddbmirror();
	if (dbmirror != 0)
		formatinfstr(infstr, x_("%c%sjava%c%s"), PATHSEP, el_libdir, DIRSEP, dbmirror);

	/* add Electric's java directory to the class path */
	formatinfstr(infstr, x_("%c%sjava"), PATHSEP, el_libdir);

	/* save the class path */
	(void)allocstring(&classpath, returninfstr(infstr), db_cluster);

#ifdef OLDJAVA
	/* see if JDK 1.1 exists */
	vm1args.version = 0x00010001;
	err = JNI_GetDefaultJavaVMInitArgs(&vm1args);
#else
	/* presume that JDK 1.1 does not exist */
	err = 1;
#endif
	if (err != 0)
	{
		/* use the newer JDK, 1.2 */
		vm2args.version = 0x00010002;
/*		vm2args.version = 0x00010004; */
		err = JNI_GetDefaultJavaVMInitArgs(&vm2args);
		noptions = 0;
		infstr = initinfstr();
		formatinfstr(infstr, x_("-Djava.class.path=%s"), classpath);
		(void)allocstring(&allocatedpath, returninfstr(infstr), db_cluster);
		joptions[noptions++].optionString = allocatedpath;
/*		joptions[noptions++].optionString = "-Xmx327680"; */
/*		joptions[noptions++].optionString = b_("-Xmx268435456");*/
		/*stop piddling around. Let's give this baby some REAL memory (1.2GB)*/
		joptions[noptions++].optionString = b_("-Xmx500m");
/*		joptions[noptions++].optionString= "-verbose:gc"; */
		joptions[noptions].optionString = b_("vfprintf");
		joptions[noptions++].extraInfo = (void *)java_vfprintf;
		joptions[noptions].optionString = b_("exit");
		joptions[noptions++].extraInfo = (void *)java_exit;

		/* use compiler? */
		/* TODO: compiler flag */

		vm2args.options = joptions;
		vm2args.nOptions = noptions;
		vm2args.ignoreUnrecognized = 1;
		err = JNI_CreateJavaVM(&java_virtualmachine, (void **)&java_environment, &vm2args);
		if (err != 0)
		{  
			ttyputerr(_("Cannot initialize Java VM (err %d)"), err);
			return(0);
		}
		efree(allocatedpath);
	} else
	{
		/* use the older JDK, 1.1 */
		infstr = initinfstr();
		addstringtoinfstr(infstr, classpath);
		addtoinfstr(infstr, PATHSEP);
		addtoinfstr(infstr, '.');
		addstringtoinfstr(infstr, classpath);
		vm1args.classpath = returninfstr(infstr);

		/* see if compiler should be disabled */
		ac = 0;
		if ((us_javaflags&JAVANOCOMPILER) != 0)
			properties[ac++] = b_("java.compiler=NONE");
		properties[ac] = 0;
		vm1args.properties = properties;

		/* set overrides */
		vm1args.vfprintf = java_vfprintf;
		vm1args.exit = java_exit;

		/* load and initialize a Java VM, return a JNI interface pointer in env */
		err = JNI_CreateJavaVM(&java_virtualmachine, (void **)&java_environment, &vm1args);
		if (err != 0)
		{
			ttyputerr(_("Cannot create Java VM (error %ld)"), err);
			return(0);
		}
	}
	efree(classpath);

	java_inited = 1;

	/* get the basic Electric class and register native methods */
	java_electricclass = java_environment->FindClass(b_("COM/staticfreesoft/Electric"));
	if (java_electricclass == 0)
	{
		ttyputerr(_("Cannot find COM.staticfreesoft.Electric class"));
		return(0);
	}
	if (java_environment->RegisterNatives(java_electricclass, electricmethods,
		sizeof(electricmethods)/sizeof(JNINativeMethod)) < 0)
			ttyputerr(_("Failed to register native interfaces"));

	/* override the "write" method of out "eout" class */
	java_electriceoutclass = java_environment->FindClass(b_("COM/staticfreesoft/Electric$eout"));
	if (java_electriceoutclass == 0)
	{
		ttyputerr(_("Cannot find COM.staticfreesoft.Electric$eout class"));
		return(0);
	}
	if (java_environment->RegisterNatives(java_electriceoutclass, electriceoutmethods, 2) < 0)
		ttyputerr(_("Failed to register output interface"));
	jobject object = java_environment->AllocObject(java_electriceoutclass);
	mid = java_environment->GetMethodID(java_electriceoutclass, b_("takeover"), b_("()V"));
	java_environment->CallVoidMethod(object, mid);

	/* get the attribute pointer to the "address" field on each class */
	java_addressID = java_environment->GetFieldID(java_electricclass, b_("address"), b_("J"));

	/* create the null object */
	java_nullobject = java_environment->NewStringUTF(b_("NULL"));
	java_environment->NewGlobalRef(java_nullobject);

	/* get the basic subclasses */
	java_classstring = java_environment->FindClass(b_("java/lang/String"));
	if (java_classstring == 0)
	{
		ttyputerr(_("Cannot find java.lang.String class"));
		return(0);
	}
	java_classint = java_environment->FindClass(b_("java/lang/Integer"));
	if (java_classint == 0) return(0);
	java_classfloat = java_environment->FindClass(b_("java/lang/Float"));
	if (java_classfloat == 0) return(0);
	java_classdouble = java_environment->FindClass(b_("java/lang/Double"));
	if (java_classdouble == 0) return(0);
	java_classarrayint = java_environment->FindClass(b_("[I"));
	if (java_classarrayint == 0)
	{
		ttyputerr(_("Cannot find int[] class"));
		return(0);
	}

	/* get the methods on the basic subclasses */
	java_midIntValue = java_environment->GetMethodID(java_classint, b_("intValue"), b_("()I"));
	java_midFloatValue = java_environment->GetMethodID(java_classfloat, b_("floatValue"), b_("()F"));
	java_midDoubleValue = java_environment->GetMethodID(java_classdouble, b_("doubleValue"), b_("()D"));
	java_midIntInit = java_environment->GetMethodID(java_classint, b_("<init>"), b_("(I)V"));
	java_midFloatInit = java_environment->GetMethodID(java_classfloat, b_("<init>"), b_("(F)V"));

	/* get the basic Electric subclasses */
	java_classnodeinst = java_environment->FindClass(b_("COM/staticfreesoft/Electric$NodeInst"));
	if (java_classnodeinst == 0) return(0);
	java_classnodeproto = java_environment->FindClass(b_("COM/staticfreesoft/Electric$NodeProto"));
	if (java_classnodeproto == 0) return(0);
	java_classportarcinst = java_environment->FindClass(b_("COM/staticfreesoft/Electric$PortArcInst"));
	if (java_classportarcinst == 0) return(0);
	java_classportexpinst = java_environment->FindClass(b_("COM/staticfreesoft/Electric$PortExpInst"));
	if (java_classportexpinst == 0) return(0);
	java_classportproto = java_environment->FindClass(b_("COM/staticfreesoft/Electric$PortProto"));
	if (java_classportproto == 0) return(0);
	java_classarcinst = java_environment->FindClass(b_("COM/staticfreesoft/Electric$ArcInst"));
	if (java_classarcinst == 0) return(0);
	java_classarcproto = java_environment->FindClass(b_("COM/staticfreesoft/Electric$ArcProto"));
	if (java_classarcproto == 0) return(0);
	java_classgeom = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Geom"));
	if (java_classgeom == 0) return(0);
	java_classlibrary = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Library"));
	if (java_classlibrary == 0) return(0);
	java_classtechnology = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Technology"));
	if (java_classtechnology == 0) return(0);
	java_classtool = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Tool"));
	if (java_classtool == 0) return(0);
	java_classrtnode = java_environment->FindClass(b_("COM/staticfreesoft/Electric$RTNode"));
	if (java_classrtnode == 0) return(0);
	java_classnetwork = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Network"));
	if (java_classnetwork == 0) return(0);
	java_classview = java_environment->FindClass(b_("COM/staticfreesoft/Electric$View"));
	if (java_classview == 0) return(0);
	java_classwindowpart = java_environment->FindClass(b_("COM/staticfreesoft/Electric$WindowPart"));
	if (java_classwindowpart == 0) return(0);
	java_classwindowframe = java_environment->FindClass(b_("COM/staticfreesoft/Electric$WindowFrame"));
	if (java_classwindowframe == 0) return(0);
	java_classgraphics = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Graphics"));
	if (java_classgraphics == 0) return(0);
	java_classconstraint = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Constraint"));
	if (java_classconstraint == 0) return(0);
	java_classpolygon = java_environment->FindClass(b_("COM/staticfreesoft/Electric$Polygon"));
	if (java_classpolygon == 0) return(0);
	java_classxarray = java_environment->FindClass(b_("COM/staticfreesoft/Electric$XArray"));
	if (java_classxarray == 0) return(0);

	/* get the attribute pointer to the fields on the XARRAY class */
	java_xarrayVID = java_environment->GetFieldID(java_classxarray, b_("v"), b_("[I"));

	/* if there is a Bean Shell, create an interpreter */
	java_bshInterpreterObject = 0;
	if (bsh != 0)
	{
		interpreterClass = java_environment->FindClass(b_("bsh/Interpreter"));
		if (interpreterClass != 0)
		{
			mid = java_environment->GetMethodID(interpreterClass, b_("<init>"), b_("()V"));
			java_bshInterpreterObject = java_environment->NewObject(interpreterClass, mid);
			if (java_bshInterpreterObject != 0)
			{
				classClz = java_environment->GetObjectClass(java_bshInterpreterObject);
				java_bshEvalMID = java_environment->GetMethodID(classClz, b_("eval"),
					b_("(Ljava/lang/String;)Ljava/lang/Object;"));
				if (java_bshEvalMID == 0) java_bshInterpreterObject = 0;
			}
		}
		for(i=0; bshInit[i] != 0; i++)
		{
#ifdef _UNICODE
			string = java_environment->NewString(bshInit[i], estrlen(bshInit[i]));
#else
			string = java_environment->NewStringUTF(bshInit[i]);
#endif
			java_environment->CallObjectMethod(java_bshInterpreterObject, java_bshEvalMID, string);
		}

		return(x_("Java Bean Shell"));
	}
	return(x_("Java"));
}

/*
 * Routine to add additional native function definitions to Java.  The name of the method is
 * "methodname".  The description of its parameters and return values is "methodsignature".
 * The actual routine is "methodfunction".  Returns TRUE on error.
 *
 * The parameter "methodsignature" has the form: (PARAMETERS)RETURNTYPE
 * where PARAMETERS is a list of 0 or more type descriptions for each parameter
 * and RETURNTYPE is a single type description for the return value.
 *
 * The type descriptions can be these Java types:
 *    "I"                                  for integer
 *    "V"                                  for void (a null return value)
 *    "Ljava/lang/String;"                 for a string
 *    "Ljava/lang/Object;"                 for a general Java object
 *    "LCOM/staticfreesoft/Electric$TYPE;" for an Electric object of type TYPE
 *                                         where TYPE is NodeInst, NodeProto, ArcInst, Tool, etc.
 * Some examples of method signatures:
 * A routine that takes two Integers and returns a string would have this signature:
 *   "(II)Ljava/lang/String;"
 * A routine that takes a NodeInst and a Technology and returns nothing would have this signature:
 *   "(LCOM/staticfreesoft/Electric$NodeInst;LCOM/staticfreesoft/Electric$Technology;)V"
 *
 * The routine that is passed (the "methodfunction" parameter) must have this form:
 *    JNIEXPORT RETURNTYPE JNICALL methodfunction(JNIEnv *env, PARAMETERS)
 *    {
 *       return(RETURNVALUE);
 *    }
 *
 * If a PARAMETER is an integer, declare it "jint" and simply assign it to an "int".
 * If a PARAMETER is a string, declare it "jstring" and use this code to access it:
 *    char *cstring = (CHAR *)java_environment->GetStringUTFChars(PARAMETER, &isCopy);
 * where "isCopy" is a "jboolean", and insert this code when done with the string:
 *    if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(PARAMETER, cstring);
 * If a PARAMETER is an Electric object, declare it "jobject" and use this code to access it:
 * 	  ai = (ARCINST *)java_environment->GetLongField(PARAMETER, java_addressID);
 *
 * If the routine returns nothing, set the RETURNTYPE to "void".
 * If the routine returns an integer, set the RETURNTYPE to "jint".
 * If the routine returns a string, set the RETURNTYPE to "jstring" and create it with:
 * 	  jstring javastring = java_environment->NewStringUTF(cstring);
 * If the routine returns an Electric type, set the RETURNTYPE to "jobject" and create it with:
 *    jobject returnvalue = java_makeobject((INTBIG)np, VNODEPROTO);
 *
 * BE WARNED!  This interface is C++, so any module that uses this must also be C++ and not C
 * (the extension on the file name must be ".cpp" and not ".c").
 *
 * Also, your routine should include "dblang.h" in order to access this interface.
 */
BOOLEAN java_addprivatenatives(CHAR1 *methodname, CHAR1 *methodsignature, void *methodfunction)
{
	JNINativeMethod electriceoutmethods;

	electriceoutmethods.name = methodname;
	electriceoutmethods.signature = methodsignature;
	electriceoutmethods.fnPtr = methodfunction;

	if (java_environment->RegisterNatives(java_electriceoutclass, &electriceoutmethods, 1) < 0)
		return(TRUE);
	return(FALSE);
}

/*
 * Routine to evaluate the string "str".  Returns true on error.
 */
BOOLEAN java_evaluate(CHAR *str)
{
	CHAR *evaluation;
	INTBIG type, err;

	/* special command to exit the JVM */
	if (estrcmp(str, x_("exit")) == 0)
	{
		err = java_virtualmachine->DestroyJavaVM();
		if (err != 0)
		{
			ttyputmsg(x_("Error %ld exiting JVM"), err);
			return(FALSE);
		}
		java_inited = 0;
		ttyputmsg(_("The JVM has exited"));
		return(TRUE);
	}

	/* evaluate the string */
	evaluation = java_query(str, &type);
	if (evaluation == 0) return(FALSE);
	switch (type)
	{
		case METHODRETURNSERROR:
			ttyputmsg(x_("ERROR: %s"), evaluation);
			break;
		case METHODRETURNSVOID:
			ttyputmsg(x_("(void)"));
			break;
		case METHODRETURNSOBJECT:
			ttyputmsg(x_("(object)%s"), evaluation);
			break;
		case METHODRETURNSBOOLEAN:
			ttyputmsg(x_("(boolean)%s"), evaluation);
			break;
		case METHODRETURNSBYTE:
			ttyputmsg(x_("(byte)%s"), evaluation);
			break;
		case METHODRETURNSCHAR:
			ttyputmsg(x_("(char)%s"), evaluation);
			break;
		case METHODRETURNSSHORT:
			ttyputmsg(x_("(short)%s"), evaluation);
			break;
		case METHODRETURNSINT:
			ttyputmsg(x_("(int)%s"), evaluation);
			break;
		case METHODRETURNSLONG:
			ttyputmsg(x_("(long)%s"), evaluation);
			break;
		case METHODRETURNSFLOAT:
			ttyputmsg(x_("(float)%s"), evaluation);
			break;
		case METHODRETURNSDOUBLE:
			ttyputmsg(x_("(double)%s"), evaluation);
			break;
	}
	return(FALSE);
}

CHAR *java_query(CHAR *str, INTBIG *methodreturntype)
{
	CHAR *pt, *methodName, *thisMethodName, *className, *returnClassName;
	static CHAR resultString[100];
	jclass theClass, classClz, methodClass, returnClass, returnClassClz;
	static jclass modifierClass = 0;
	REGISTER INTBIG len, i, gotMethod;
	INTBIG addr, type;
	CHAR sig[300], *sigpt, *description;
	int modifiers;
	jfloat fvalue;
	jlong lvalue;
	jdouble dvalue;
	jboolean bvalue;
	jobject ovalue, result, thisMethod;
	jboolean isCopy;
	jthrowable throwObj;
	jmethodID getmethodsmid, mid;
	jobjectArray methodlist, methodParams;
	jstring jMethodName, jReturnClassName, string;
	REGISTER void *infstr;

	/* if not evaluating, just return the input */
	if ((us_javaflags&JAVANOEVALUATE) != 0)
	{
		getsimpletype(str, &type, &addr, 0);
		switch (type)
		{
			case VINTEGER: *methodreturntype = METHODRETURNSINT;     break;
			case VFLOAT:   *methodreturntype = METHODRETURNSFLOAT;   break;
			case VSTRING:  *methodreturntype = METHODRETURNSCHAR;    break;
			default:       *methodreturntype = METHODRETURNSOBJECT;  break;
		}
		return(str);
	}

	/* see if this request escapes the Bean shell */
	while (*str == ' ' || *str == '\t') str++;
	if (*str == 0) return(0);

	/* if Bean Shell exists, use it to evaluate */
	if (*str == '!') str++; else
	{
		if (java_bshInterpreterObject != 0)
		{
			/* do substitution of "@" symbol (becomes "P()") */
			for(pt = str; *pt != 0; pt++)
				if (*pt == '@') break;
			if (*pt != 0)
			{
				infstr = initinfstr();
				for(pt = str; *pt != 0; pt++)
				{
					if (*pt != '@') addtoinfstr(infstr, *pt); else
					{
						addstringtoinfstr(infstr, x_("P(\""));
						for(;;)
						{
							pt++;
							if (isalnum(*pt) || *pt == '$' || *pt == '_')
								addtoinfstr(infstr, *pt); else break;
						}
						addstringtoinfstr(infstr, x_("\")"));
						pt--;
					}
				}
				str = returninfstr(infstr);
			}
#ifdef _UNICODE
			string = java_environment->NewString(str, estrlen(str));
#else
			string = java_environment->NewStringUTF(str);
#endif

			/* clear exceptions */
			java_environment->ExceptionClear();

			/* evaluate the string */
			result = java_environment->CallObjectMethod(java_bshInterpreterObject, java_bshEvalMID, string);
			java_environment->DeleteLocalRef(string);

			/* report errors if they occurred */
			throwObj = java_environment->ExceptionOccurred();
			if (throwObj != NULL)
			{
				java_environment->ExceptionClear();
				*methodreturntype = METHODRETURNSERROR;
				classClz = java_environment->GetObjectClass(throwObj);
				mid = java_environment->GetMethodID(classClz, b_("getMessage"),
					b_("()Ljava/lang/String;"));
				string = (jstring)java_environment->CallObjectMethod(throwObj, mid, NULL);

				/* "string" was zero (when out of memory?) */
				if (string == 0)
				{
					ttyputmsg(x_("No memory in Java interface!"));
					return(x_("error"));
				}

				description = (CHAR *)java_environment->GetStringUTFChars(string, &isCopy);
				return(description);
			}

			/* valid result: return it */
			if (result == 0)
			{
				*methodreturntype = METHODRETURNSVOID;
				return(x_(""));
			}
			java_getobjectaddrtype(result, &addr, &type, &description);
			switch (type)
			{
				case VINTEGER: *methodreturntype = METHODRETURNSINT;     break;
				case VFLOAT:   *methodreturntype = METHODRETURNSFLOAT;   break;
				case VSTRING:  *methodreturntype = METHODRETURNSCHAR;    break;
				default:       *methodreturntype = METHODRETURNSOBJECT;  break;
			}
			return(description);
		}
	}

	/* parse string into CLASS.METHOD (must be static and take no parameters) */
	className = str;
	while (*className == ' ' || *className == '\t') className++;
	len = estrlen(className);
	for(i=len-1; i>0; i--)
		if (className[i] == '.') break;
	if (i == 0)
	{
		ttyputerr(_("Cannot determine class and method of '%s'"), str);
		return(0);
	}
	methodName = &className[i+1];
	className[i] = 0;
	for(pt = className; *pt != 0; pt++) if (*pt == '.') *pt = '/';

	/* find the class */
	theClass = java_environment->FindClass(string1byte(className));
	if (theClass == 0)
	{
		ttyputerr(_("Cannot find class '%s'"), className);
		className[i] = '.';
		return(0);
	}
	className[i] = '.';

	/* get the list of methods on "theClass" */
	classClz = java_environment->GetObjectClass(theClass);
	getmethodsmid = java_environment->GetMethodID(classClz, b_("getMethods"),
		b_("()[Ljava/lang/reflect/Method;"));
	methodlist = (jobjectArray)java_environment->CallObjectMethod(theClass, getmethodsmid, NULL);

	/* look at all methods */
	len = java_environment->GetArrayLength(methodlist);
	for(i=0; i<len; i++)
	{
		/* get a method */
		thisMethod = java_environment->GetObjectArrayElement(methodlist, i);

		/* get the method's name */
		methodClass = java_environment->GetObjectClass(thisMethod);
		mid = java_environment->GetMethodID(methodClass, b_("getName"), b_("()Ljava/lang/String;"));
		jMethodName = (jstring)java_environment->CallObjectMethod(thisMethod, mid, NULL);
		thisMethodName = (CHAR *)java_environment->GetStringUTFChars(jMethodName, &isCopy);
		gotMethod = estrcmp(methodName, thisMethodName);
		if (isCopy == JNI_TRUE)
			java_environment->ReleaseStringUTFChars((jstring)jMethodName, thisMethodName);
		if (gotMethod != 0) continue;

		/* found method "thisMethod"...it must have zero parameters */
		mid = java_environment->GetMethodID(methodClass, b_("getParameterTypes"), b_("()[Ljava/lang/Class;"));
		methodParams = (jobjectArray)java_environment->CallObjectMethod(thisMethod, mid, NULL);
		if (java_environment->GetArrayLength(methodParams) != 0) continue;

		/* the method must be static */
		mid = java_environment->GetMethodID(methodClass, b_("getModifiers"), b_("()I"));
		modifiers = java_environment->CallIntMethod(thisMethod, mid, NULL);
		if (modifierClass == 0)
			modifierClass = java_environment->FindClass(b_("java/lang/reflect/Modifier"));
		mid = java_environment->GetStaticMethodID(modifierClass, b_("isStatic"), b_("(I)Z"));
		if (java_environment->CallStaticBooleanMethod(modifierClass, mid, modifiers) == JNI_FALSE) continue;

		/* get the return type of the method */
		mid = java_environment->GetMethodID(methodClass, b_("getReturnType"), b_("()Ljava/lang/Class;"));
		returnClass = (jclass)java_environment->CallObjectMethod(thisMethod, mid, NULL);
		returnClassClz = java_environment->GetObjectClass(returnClass);
		mid = java_environment->GetMethodID(returnClassClz, b_("toString"), b_("()Ljava/lang/String;"));
		jReturnClassName = (jstring)java_environment->CallObjectMethod(returnClass, mid, NULL);
		returnClassName = (CHAR *)java_environment->GetStringUTFChars(jReturnClassName, &isCopy);

		/* convert that return-type to a signature */
		sigpt = sig;
		*sigpt++ = '(';
		*sigpt++ = ')';
		if (estrcmp(returnClassName, x_("void")) == 0)
		{
			*sigpt++ = 'V';
			*methodreturntype = METHODRETURNSVOID;
		} else if (estrncmp(returnClassName, x_("class "), 6) == 0)
		{
			*sigpt++ = 'L';
			for(pt = &returnClassName[6]; *pt != 0; pt++)
				if (*pt == '.') *sigpt++ = '/'; else
					*sigpt++ = *pt;
			*sigpt++ = ';';
			*methodreturntype = METHODRETURNSOBJECT;
		} else if (estrcmp(returnClassName, x_("boolean")) == 0)
		{
			*sigpt++ = 'Z';
			*methodreturntype = METHODRETURNSBOOLEAN;
		} else if (estrcmp(returnClassName, x_("byte")) == 0)
		{
			*sigpt++ = 'B';
			*methodreturntype = METHODRETURNSBYTE;
		} else if (estrcmp(returnClassName, x_("char")) == 0)
		{
			*sigpt++ = 'C';
			*methodreturntype = METHODRETURNSCHAR;
		} else if (estrcmp(returnClassName, x_("short")) == 0)
		{
			*sigpt++ = 'S';
			*methodreturntype = METHODRETURNSSHORT;
		} else if (estrcmp(returnClassName, x_("int")) == 0)
		{
			*sigpt++ = 'I';
			*methodreturntype = METHODRETURNSINT;
		} else if (estrcmp(returnClassName, x_("long")) == 0)
		{
			*sigpt++ = 'J';
			*methodreturntype = METHODRETURNSLONG;
		} else if (estrcmp(returnClassName, x_("float")) == 0)
		{
			*sigpt++ = 'F';
			*methodreturntype = METHODRETURNSFLOAT;
		} else if (estrcmp(returnClassName, x_("double")) == 0)
		{
			*sigpt++ = 'D';
			*methodreturntype = METHODRETURNSDOUBLE;
		} else
		{
			ttyputerr(_("%s returns unrecognized type '%s'"), str, returnClassName);
			return(0);
		}
		*sigpt = 0;
		if (isCopy == JNI_TRUE)
			java_environment->ReleaseStringUTFChars((jstring)jReturnClassName, returnClassName);

		mid = java_environment->GetStaticMethodID(theClass, methodName, sig);
		if (mid == 0)
		{
			ttyputerr(_("Error getting method pointer to %s (signature '%s')"), str, sig);
			return(0);
		}

		/* run the method and print the result */
		switch (*methodreturntype)
		{
			case METHODRETURNSVOID:
				java_environment->CallStaticVoidMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				return(x_(""));
			case METHODRETURNSOBJECT:
				ovalue = java_environment->CallStaticObjectMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				java_getobjectaddrtype(ovalue, &addr, &type, &description);
				return(description);
			case METHODRETURNSBOOLEAN:
				bvalue = java_environment->CallStaticBooleanMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%s"), bvalue ? _("True") : _("False"));
				return(resultString);
			case METHODRETURNSBYTE:
				i = java_environment->CallStaticByteMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%ld"), i);
				return(resultString);
			case METHODRETURNSCHAR:
				i = java_environment->CallStaticCharMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("'%c'"), i);
				return(resultString);
			case METHODRETURNSSHORT:
				i = java_environment->CallStaticShortMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%ld"), i);
				return(resultString);
			case METHODRETURNSINT:
				i = java_environment->CallStaticIntMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%ld"), i);
				return(resultString);
			case METHODRETURNSLONG:
				lvalue = java_environment->CallStaticLongMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%ld"), lvalue);
				return(resultString);
			case METHODRETURNSFLOAT:
				fvalue = java_environment->CallStaticFloatMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%g"), fvalue);
				return(resultString);
			case METHODRETURNSDOUBLE:
				dvalue = java_environment->CallStaticDoubleMethod(theClass, mid);
				if (java_reportexceptions()) return(0);
				esnprintf(resultString, 100, x_("%g"), dvalue);
				return(resultString);
		}
	}
	ttyputerr(_("Cannot find static method '%s' that takes no parameters"), str);
	return(0);
}

BOOLEAN java_reportexceptions(void)
{
	if (java_environment->ExceptionOccurred() == NULL) return(FALSE);
	java_environment->ExceptionDescribe();
	java_environment->ExceptionClear();
	return(TRUE);
}

/****************************** ELECTRIC DATABASE EXAMINATION ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_curLib(JNIEnv *env, jobject obj)
{
	jobject jlib;

	/* convert C result to Java */
	jlib = java_makeobject((INTBIG)el_curlib, VLIBRARY);
	return(jlib);
}

JNIEXPORT jobject JNICALL Java_Electric_curTech(JNIEnv *env, jobject obj)
{
	jobject jtech;

	/* convert C result to Java */
	jtech = java_makeobject((INTBIG)el_curtech, VTECHNOLOGY);
	return(jtech);
}

JNIEXPORT jobject JNICALL Java_Electric_getValNodeinst(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VNODEINST, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValNodeproto(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VNODEPROTO, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValPortarcinst(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VPORTARCINST, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValPortexpinst(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VPORTEXPINST, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValPortproto(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VPORTPROTO, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValArcinst(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VARCINST, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValArcproto(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VARCPROTO, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValGeom(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VGEOM, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValLibrary(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VLIBRARY, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValTechnology(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VTECHNOLOGY, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValTool(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VTOOL, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValRTNode(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VRTNODE, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValNetwork(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VNETWORK, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValView(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VVIEW, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValWindowpart(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VWINDOWPART, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValWindowframe(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VWINDOWFRAME, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValGraphics(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VGRAPHICS, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValConstraint(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VCONSTRAINT, jname));
}

JNIEXPORT jobject JNICALL Java_Electric_getValPolygon(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	return(java_getval(jobj, VPOLYGON, jname));
}

jobject java_getval(jobject jobj, INTBIG type, jstring jname)
{
	CHAR *name;
	jboolean isCopy;
	jobject retval;
	INTBIG addr;
	REGISTER VARIABLE *var;

	/* convert Java parameters to C */
	if (jobj == 0) return(java_nullobject);
	addr = (INTBIG)java_environment->GetLongField(jobj, java_addressID);
	name = (CHAR *)(INTBIG)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	var = getval(addr, type, -1, name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	if (var == NOVARIABLE) return(java_nullobject);
	retval = java_makeobject(var->addr, var->type);
	return(retval);
}

JNIEXPORT jobject JNICALL Java_Electric_getParentVal(JNIEnv *env, jobject obj,
	jstring jname, jobject jdefault, jint jheight)
{
	CHAR *name;
	jboolean isCopy;
	VARIABLE *var;
	jobject retval;
	INTBIG height;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);
	height = jheight;

	/* make the Electric call */
	var = getparentval((CHAR *)name, height);

	/* convert C result to Java */
	if (var == NOVARIABLE) retval = jdefault; else
		retval = java_makeobject(var->addr, var->type);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);
	return(retval);
}

JNIEXPORT void JNICALL Java_Electric_setVal(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname, jobject jattr, jint jbits)
{
	CHAR *name;
	jboolean isCopy;
	INTBIG addr, type, bits, newaddr, newtype;
	VARIABLE *var;

	/* convert Java parameters to C */
	if (jobj == 0) return;
	if (jattr == 0) return;
	java_getobjectaddrtype(jobj, &addr, &type, 0);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);
	java_getobjectaddrtype(jattr, &newaddr, &newtype, 0);
	bits = jbits;

	/* make the Electric call */
	var = getval(addr, type, -1, name);
	if (var != NOVARIABLE)
	{
		if (((var->type&VTYPE) == VSHORT || (var->type&VTYPE) == VBOOLEAN) && newtype == VINTEGER)
			newtype = var->type&VTYPE;
	}
	(void)setval(addr, type, name, newaddr, newtype|bits);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);
}

JNIEXPORT void JNICALL Java_Electric_setInd(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname, jint jindex, jobject jattr)
{
	CHAR *name;
	jboolean isCopy;
	INTBIG addr, type, index, newaddr, newtype;

	/* convert Java parameters to C */
	if (jobj == 0) return;
	if (jattr == 0) return;
	java_getobjectaddrtype(jobj, &addr, &type, 0);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);
	index = jindex;
	java_getobjectaddrtype(jattr, &newaddr, &newtype, 0);

	/* make the Electric call */
	(void)setind(addr, type, name, index, newaddr);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);
}

JNIEXPORT void JNICALL Java_Electric_delVal(JNIEnv *env, jobject obj,
	jobject jobj, jstring jname)
{
	CHAR *name;
	jboolean isCopy;
	INTBIG addr, type;

	/* convert Java parameters to C */
	if (jobj == 0) return;
	java_getobjectaddrtype(jobj, &addr, &type, 0);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	(void)delval(addr, type, name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);
}

JNIEXPORT jint JNICALL Java_Electric_initSearch(JNIEnv *env, jobject obj,
	jint jlx, jint jhx, jint jly, jint jhy, jobject jcell)
{
	NODEPROTO *cell;
	INTBIG sea, lx, hx, ly, hy;
	jint jsea;

	/* convert Java parameters to C */
	lx = jlx;   hx = jhx;
	ly = jly;   hy = jhy;
	if (jcell == 0) return(0);
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);

	/* make the Electric call */
	sea = initsearch(lx, hx, ly, hy, cell);

	/* convert C result to Java */
	jsea = sea;
	return(jsea);
}

JNIEXPORT jobject JNICALL Java_Electric_nextObject(JNIEnv *env, jobject obj,
	jint jsea)
{
	GEOM *geom;
	jobject jgeom;
	INTBIG sea;

	/* convert Java parameters to C */
	if (jsea == 0) return(java_nullobject);
	sea = jsea;

	/* make the Electric call */
	geom = nextobject(sea);

	/* convert C result to Java */
	jgeom = java_makeobject((INTBIG)geom, VGEOM);
	return(jgeom);
}

JNIEXPORT void JNICALL Java_Electric_termSearch(JNIEnv *env, jobject obj,
	jint jsea)
{
	INTBIG sea;

	/* convert Java parameters to C */
	if (jsea == 0) return;
	sea = jsea;

	/* make the Electric call */
	termsearch(sea);
}

/****************************** ELECTRIC TOOL ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_getTool(JNIEnv *env, jobject obj,
	jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	TOOL *tool;
	jobject jtool;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	tool = gettool(name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jtool = java_makeobject((INTBIG)tool, VTOOL);
	return(jtool);
}

JNIEXPORT jint JNICALL Java_Electric_maxTool(JNIEnv *env, jobject obj)
{
	jint jmaxtool;

	/* convert C result to Java */
	jmaxtool = el_maxtools;
	return(jmaxtool);
}

JNIEXPORT jobject JNICALL Java_Electric_indexTool(JNIEnv *env, jobject obj,
	jint jindex)
{
	INTBIG index;
	TOOL *tool;
	jobject jtool;

	/* convert Java parameters to C */
	index = jindex;

	/* make the Electric call */
	if (index < 0 || index >= el_maxtools) tool = NOTOOL; else tool = &el_tools[index];

	/* convert C result to Java */
	jtool = java_makeobject((INTBIG)tool, VTOOL);
	return(jtool);
}

JNIEXPORT void JNICALL Java_Electric_toolTurnOn(JNIEnv *env, jobject obj,
	jobject jtool)
{
	TOOL *tool;

	/* convert Java parameters to C */
	if (jtool == 0) return;
	tool = (TOOL *)(INTBIG)java_environment->GetLongField(jtool, java_addressID);

	/* make the Electric call */
	toolturnon(tool);
}

JNIEXPORT void JNICALL Java_Electric_toolTurnOff(JNIEnv *env, jobject obj,
	jobject jtool)
{
	TOOL *tool;

	/* convert Java parameters to C */
	if (jtool == 0) return;
	tool = (TOOL *)(INTBIG)java_environment->GetLongField(jtool, java_addressID);

	/* make the Electric call */
	toolturnoff(tool, TRUE);
}

JNIEXPORT void JNICALL Java_Electric_tellTool(JNIEnv *env, jobject obj,
	jobject jtool, jint jargc, jobjectArray jargv)
{
	jboolean *isCopy;
	CHAR **argv;
	jstring str;
	INTBIG argc, i;
	TOOL *tool;

	/* convert Java parameters to C */
	if (jtool == 0) return;
	if (jargc == 0) return;
	tool = (TOOL *)(INTBIG)java_environment->GetLongField(jtool, java_addressID);
	argc = jargc;
	isCopy = (jboolean *)emalloc(argc * (sizeof (jboolean)), el_tempcluster);
	argv = (CHAR **)emalloc(argc * (sizeof (CHAR *)), el_tempcluster);
	for(i=0; i<argc; i++)
	{
		str = (jstring)java_environment->GetObjectArrayElement(jargv, i);
		argv[i] = (CHAR *)java_environment->GetStringUTFChars(str, &isCopy[i]);
	}

	/* make the Electric call */
	telltool(tool, argc, argv);
	for(i=0; i<argc; i++)
	{
		str = (jstring)java_environment->GetObjectArrayElement(jargv, i);
		if (isCopy[i] == JNI_TRUE) java_environment->ReleaseStringUTFChars(str, argv[i]);
	}
	efree((CHAR *)isCopy);
	efree((CHAR *)argv);
}

/****************************** ELECTRIC LIBRARY ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_getLibrary(JNIEnv *env, jobject obj,
	jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	LIBRARY *lib;
	jobject jlib;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	lib = getlibrary(name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jlib = java_makeobject((INTBIG)lib, VLIBRARY);
	return(jlib);
}

JNIEXPORT jobject JNICALL Java_Electric_newLibrary(JNIEnv *env, jobject obj,
	jstring jlibname, jstring jlibfile)
{
	jboolean isCopy1, isCopy2;
	CHAR *libname, *libfile;
	LIBRARY *lib;
	jobject jlib;

	/* convert Java parameters to C */
	libname = (CHAR *)java_environment->GetStringUTFChars(jlibname, &isCopy1);
	libfile = (CHAR *)java_environment->GetStringUTFChars(jlibfile, &isCopy2);

	/* make the Electric call */
	lib = newlibrary(libname, libfile);
	if (isCopy1 == JNI_TRUE) java_environment->ReleaseStringUTFChars(jlibname, libname);
	if (isCopy2 == JNI_TRUE) java_environment->ReleaseStringUTFChars(jlibfile, libfile);

	/* convert C result to Java */
	jlib = java_makeobject((INTBIG)lib, VLIBRARY);
	return(jlib);
}

JNIEXPORT void JNICALL Java_Electric_killLibrary(JNIEnv *env, jobject obj,
	jobject jlib)
{
	LIBRARY *lib;

	/* convert Java parameters to C */
	if (jlib == 0) return;
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);

	/* make the Electric call */
	killlibrary(lib);
}

JNIEXPORT void JNICALL Java_Electric_eraseLibrary(JNIEnv *env, jobject obj,
	jobject jlib)
{
	LIBRARY *lib;

	/* convert Java parameters to C */
	if (jlib == 0) return;
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);

	/* make the Electric call */
	eraselibrary(lib);
}

JNIEXPORT void JNICALL Java_Electric_selectLibrary(JNIEnv *env, jobject obj,
	jobject jlib)
{
	LIBRARY *lib;

	/* convert Java parameters to C */
	if (jlib == 0) return;
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);

	/* make the Electric call */
	selectlibrary(lib, TRUE);
}

/****************************** ELECTRIC LIBRARY ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_getNodeProto(JNIEnv *env, jobject obj,
	jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	NODEPROTO *np;
	jobject jnp;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	np = getnodeproto(name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jnp = java_makeobject((INTBIG)np, VNODEPROTO);
	return(jnp);
}

JNIEXPORT jobject JNICALL Java_Electric_newNodeProto(JNIEnv *env, jobject obj,
	jstring jname, jobject jlib)
{
	jboolean isCopy;
	CHAR *name;
	NODEPROTO *np;
	LIBRARY *lib;
	jobject jnp;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);
	if (jlib == 0) return(java_nullobject);
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);

	/* make the Electric call */
	np = newnodeproto(name, lib);
	if (np != NONODEPROTO) endobjectchange((INTBIG)np, VNODEPROTO);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jnp = java_makeobject((INTBIG)np, VNODEPROTO);
	return(jnp);
}

JNIEXPORT jint JNICALL Java_Electric_killNodeProto(JNIEnv *env, jobject obj,
	jobject jcell)
{
	BOOLEAN ret;
	jint jret;
	NODEPROTO *cell;

	/* convert Java parameters to C */
	if (jcell == 0) return(1);
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);

	/* make the Electric call */
	startobjectchange((INTBIG)cell, VNODEPROTO);
	ret = killnodeproto(cell);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

JNIEXPORT jobject JNICALL Java_Electric_copyNodeProto(JNIEnv *env, jobject obj,
	jobject jcell, jobject jlib, jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	NODEPROTO *np, *cell;
	LIBRARY *lib;
	jobject jnp;

	/* convert Java parameters to C */
	if (jcell == 0) return(java_nullobject);
	if (jlib == 0) return(java_nullobject);
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	np = copynodeproto(cell, lib, name, FALSE);
	if (np != NONODEPROTO) endobjectchange((INTBIG)np, VNODEPROTO);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jnp = java_makeobject((INTBIG)np, VNODEPROTO);
	return(jnp);
}

JNIEXPORT jobject JNICALL Java_Electric_iconView(JNIEnv *env, jobject obj,
	jobject jcell)
{
	jobject jiconcell;
	NODEPROTO *cell, *iconcell;

	/* convert Java parameters to C */
	if (jcell == 0) return(java_nullobject);
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);

	/* make the Electric call */
	iconcell = iconview(cell);

	/* convert C result to Java */
	jiconcell = java_makeobject((INTBIG)iconcell, VNODEPROTO);
	return(jiconcell);
}

JNIEXPORT jobject JNICALL Java_Electric_contentsView(JNIEnv *env, jobject obj,
	jobject jcell)
{
	jobject jcontentscell;
	NODEPROTO *cell, *contentscell;

	/* convert Java parameters to C */
	if (jcell == 0) return(java_nullobject);
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);

	/* make the Electric call */
	contentscell = contentsview(cell);

	/* convert C result to Java */
	jcontentscell = java_makeobject((INTBIG)contentscell, VNODEPROTO);
	return(jcontentscell);
}

/****************************** ELECTRIC NODEINST ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_newNodeInst(JNIEnv *env, jobject obj,
	jobject jproto, jint jlx, jint jhx, jint jly, jint jhy, jint jtrans, jint jrot, jobject jcell)
{
	jobject jni;
	NODEINST *ni;
	NODEPROTO *cell, *proto;
	INTBIG lx, hx, ly, hy;
	INTBIG rot, trans;

	/* convert Java parameters to C */
	if (jproto == 0) return(java_nullobject);
	if (jcell == 0) return(java_nullobject);
	proto = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jproto, java_addressID);
	lx = jlx;   hx = jhx;
	ly = jly;   hy = jhy;
	trans = jtrans;
	rot = jrot;
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);

	/* make the Electric call */
	ni = newnodeinst(proto, lx, hx, ly, hy, trans, rot, cell);
	if (ni != NONODEINST) endobjectchange((INTBIG)ni, VNODEINST);

	/* convert C result to Java */
	jni = java_makeobject((INTBIG)ni, VNODEINST);
	return(jni);
}

JNIEXPORT void JNICALL Java_Electric_modifyNodeInst(JNIEnv *env, jobject obj,
	jobject jni, jint jdlx, jint jdly, jint jdhx, jint jdhy, jint jdrot, jint jdtrans)
{
	NODEINST *ni;
	INTBIG dlx, dhx, dly, dhy;
	INTBIG drot, dtrans;

	/* convert Java parameters to C */
	if (jni == 0) return;
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	dlx = jdlx;   dhx = jdhx;
	dly = jdly;   dhy = jdhy;
	drot = jdrot;
	dtrans = jdtrans;

	/* make the Electric call */
	startobjectchange((INTBIG)ni, VNODEINST);
	modifynodeinst(ni, dlx, dly, dhx, dhy, drot, dtrans);
	endobjectchange((INTBIG)ni, VNODEINST);
}

JNIEXPORT jint JNICALL Java_Electric_killNodeInst(JNIEnv *env, jobject obj,
	jobject jni)
{
	NODEINST *ni;
	BOOLEAN ret;
	jint jret;

	/* convert Java parameters to C */
	if (jni == 0) return(1);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);

	/* make the Electric call */
	startobjectchange((INTBIG)ni, VNODEINST);
	ret = killnodeinst(ni);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

JNIEXPORT jobject JNICALL Java_Electric_replaceNodeInst(JNIEnv *env, jobject obj,
	jobject jni, jobject jproto)
{
	NODEINST *ni, *newni;
	NODEPROTO *proto;
	jobject jnewni;

	/* convert Java parameters to C */
	if (jni == 0) return(java_nullobject);
	if (jproto == 0) return(java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	proto = (NODEPROTO*)(INTBIG)java_environment->GetLongField(jproto, java_addressID);

	/* make the Electric call */
	startobjectchange((INTBIG)ni, VNODEINST);
	newni = replacenodeinst(ni, proto, FALSE, FALSE);
	if (newni != NONODEINST) endobjectchange((INTBIG)newni, VNODEINST);

	/* convert C result to Java */
	jnewni = java_makeobject((INTBIG)newni, VNODEINST);
	return(jnewni);
}

JNIEXPORT jint JNICALL Java_Electric_nodeFunction(JNIEnv *env, jobject obj,
	jobject jni)
{
	NODEINST *ni;
	INTBIG fun;
	jint jfun;

	/* convert Java parameters to C */
	if (jni == 0) return(0);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);

	/* make the Electric call */
	fun = nodefunction(ni);

	/* convert C result to Java */
	jfun = fun;
	return(jfun);
}

JNIEXPORT jint JNICALL Java_Electric_nodePolys(JNIEnv *env, jobject obj,
	jobject jni)
{
	NODEINST *ni;
	INTBIG count;
	jint jcount;

	/* convert Java parameters to C */
	if (jni == 0) return(0);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);

	/* make the Electric call */
	count = nodepolys(ni, 0, NOWINDOWPART);

	/* convert C result to Java */
	jcount = count;
	return(jcount);
}

JNIEXPORT jobject JNICALL Java_Electric_shapeNodePoly(JNIEnv *env, jobject obj,
	jobject jni, jint jindex)
{
	NODEINST *ni;
	INTBIG index;
	jobject jpoly;
	POLYGON *poly;

	/* convert Java parameters to C */
	if (jni == 0) return(java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	index = jindex;

	/* make the Electric call */
	poly = allocpolygon(4, db_cluster);
	shapenodepoly(ni, index, poly);

	/* convert C result to Java */
	jpoly = java_makeobject((INTBIG)poly, VPOLYGON);
	return(jpoly);
}

JNIEXPORT jint JNICALL Java_Electric_nodeEPolys(JNIEnv *env, jobject obj,
	jobject jni)
{
	NODEINST *ni;
	INTBIG count;
	jint jcount;

	/* convert Java parameters to C */
	if (jni == 0) return(0);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);

	/* make the Electric call */
	count = nodeEpolys(ni, 0, NOWINDOWPART);

	/* convert C result to Java */
	jcount = count;
	return(jcount);
}

JNIEXPORT jobject JNICALL Java_Electric_shapeENodePoly(JNIEnv *env, jobject obj,
	jobject jni, jint jindex)
{
	NODEINST *ni;
	INTBIG index;
	jobject jpoly;
	POLYGON *poly;

	/* convert Java parameters to C */
	if (jni == 0) return(java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	index = jindex;

	/* make the Electric call */
	poly = allocpolygon(4, db_cluster);
	shapeEnodepoly(ni, index, poly);

	/* convert C result to Java */
	jpoly = java_makeobject((INTBIG)poly, VPOLYGON);
	return(jpoly);
}

JNIEXPORT jobject JNICALL Java_Electric_makeRot(JNIEnv *env, jobject obj,
	jobject jni)
{
	NODEINST *ni;
	XARRAY trans;
	jobject object;
	jintArray vArray;

	/* convert Java parameters to C */
	if (jni == 0) return(java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);

	/* make the Electric call */
	makerot(ni, trans);

	/* convert C result to Java */
	object = java_environment->AllocObject(java_classxarray);
	vArray = java_environment->NewIntArray(9);
	java_environment->SetIntArrayRegion(vArray, (jsize)0, (jsize)9, (jint *)&trans[0][0]);
	java_environment->SetObjectField(object, java_xarrayVID, (jobject)vArray);
	return(object);
}

JNIEXPORT jobject JNICALL Java_Electric_makeTrans(JNIEnv *env, jobject obj,
	jobject jni)
{
	NODEINST *ni;
	XARRAY trans;
	jobject object;
	jintArray vArray;

	/* convert Java parameters to C */
	if (jni == 0) return(java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);

	/* make the Electric call */
	maketrans(ni, trans);

	/* convert C result to Java */
	object = java_environment->AllocObject(java_classxarray);
	vArray = java_environment->NewIntArray(9);
	java_environment->SetIntArrayRegion(vArray, (jsize)0, (jsize)9, (jint *)&trans[0][0]);
	java_environment->SetObjectField(object, java_xarrayVID, (jobject)vArray);
	return(object);
}

JNIEXPORT jintArray JNICALL Java_Electric_nodeProtoSizeOffset(JNIEnv *env, jobject obj,
	jobject jnp)
{
	NODEPROTO *np;
	INTBIG lx, hx, ly, hy;
	jintArray retarray;
	jint *retarrayptr;
	jboolean isCopy;

	/* convert Java parameters to C */
	if (jnp == 0) return((jintArray)java_nullobject);
	np = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jnp, java_addressID);

	/* make the Electric call */
	nodeprotosizeoffset(np, &lx, &ly, &hx, &hy, NONODEPROTO);

	/* convert C result to Java */
	retarray = java_environment->NewIntArray(4);
	retarrayptr = java_environment->GetIntArrayElements(retarray, &isCopy);
	retarrayptr[0] = lx;
	retarrayptr[1] = ly;
	retarrayptr[2] = hx;
	retarrayptr[3] = hy;
	return(retarray);
}

/****************************** ELECTRIC ARCINST ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_newArcInst(JNIEnv *env, jobject obj,
	jobject jproto, jint jwid, jint jbits, jobject jni1, jobject jpp1, jint jx1, jint jy1,
	jobject jni2, jobject jpp2, jint jx2, jint jy2, jobject jcell)
{
	jobject jai;
	ARCINST *ai;
	NODEPROTO *cell;
	NODEINST *ni1, *ni2;
	PORTPROTO *pp1, *pp2;
	ARCPROTO *proto;
	INTBIG wid, bits, x1, y1, x2, y2;

	/* convert Java parameters to C */
	if (jproto == 0) return(java_nullobject);
	if (jni1 == 0 || jpp1 == 0) return(java_nullobject);
	if (jni2 == 0 || jpp2 == 0) return(java_nullobject);
	if (jcell == 0) return(java_nullobject);
	proto = (ARCPROTO *)(INTBIG)java_environment->GetLongField(jproto, java_addressID);
	wid = jwid;   bits = jbits;
	ni1 = (NODEINST *)(INTBIG)java_environment->GetLongField(jni1, java_addressID);
	pp1 = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp1, java_addressID);
	x1 = jx1;   y1 = jy1;
	ni2 = (NODEINST *)(INTBIG)java_environment->GetLongField(jni2, java_addressID);
	pp2 = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp2, java_addressID);
	x2 = jx2;   y2 = jy2;
	cell = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jcell, java_addressID);

	/* make the Electric call */
	ai = newarcinst(proto, wid, bits, ni1, pp1, x1, y1, ni2, pp2, x2, y2, cell);
	if (ai != NOARCINST) endobjectchange((INTBIG)ai, VARCINST);

	/* convert C result to Java */
	jai = java_makeobject((INTBIG)ai, VARCINST);
	return(jai);
}

JNIEXPORT jint JNICALL Java_Electric_modifyArcInst(JNIEnv *env, jobject obj,
	jobject jai, jint jdwid, jint jdx1, jint jdy1, jint jdx2, jint jdy2)
{
	ARCINST *ai;
	INTBIG dwid, dx1, dy1, dx2, dy2;
	BOOLEAN ret;
	jint jret;

	/* convert Java parameters to C */
	if (jai == 0) return(1);
	ai = (ARCINST *)(INTBIG)java_environment->GetLongField(jai, java_addressID);
	dwid = jdwid;
	dx1 = jdx1;   dy1 = jdy1;
	dx2 = jdx2;   dy2 = jdy2;

	/* make the Electric call */
	startobjectchange((INTBIG)ai, VARCINST);
	ret = modifyarcinst(ai, dwid, dx1, dy1, dx2, dy2);
	endobjectchange((INTBIG)ai, VARCINST);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

JNIEXPORT jint JNICALL Java_Electric_killArcInst(JNIEnv *env, jobject obj,
	jobject jai)
{
	ARCINST *ai;
	BOOLEAN ret;
	jint jret;

	/* convert Java parameters to C */
	if (jai == 0) return(1);
	ai = (ARCINST *)(INTBIG)java_environment->GetLongField(jai, java_addressID);

	/* make the Electric call */
	startobjectchange((INTBIG)ai, VARCINST);
	ret = killarcinst(ai);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

JNIEXPORT jobject JNICALL Java_Electric_replaceArcInst(JNIEnv *env, jobject obj,
	jobject jai, jobject jap)
{
	ARCINST *ai, *newai;
	ARCPROTO *ap;
	jobject jnewai;

	/* convert Java parameters to C */
	if (jai == 0) return(java_nullobject);
	if (jap == 0) return(java_nullobject);
	ai = (ARCINST *)(INTBIG)java_environment->GetLongField(jai, java_addressID);
	ap = (ARCPROTO *)(INTBIG)java_environment->GetLongField(jap, java_addressID);

	/* make the Electric call */
	startobjectchange((INTBIG)ai, VARCINST);
	newai = replacearcinst(ai, ap);
	if (newai != NOARCINST) endobjectchange((INTBIG)newai, VARCINST);

	/* convert C result to Java */
	jnewai = java_makeobject((INTBIG)newai, VARCINST);
	return(jnewai);
}

JNIEXPORT jint JNICALL Java_Electric_arcPolys(JNIEnv *env, jobject obj,
	jobject jai)
{
	ARCINST *ai;
	INTBIG count;
	jint jcount;

	/* convert Java parameters to C */
	if (jai == 0) return(0);
	ai = (ARCINST *)(INTBIG)java_environment->GetLongField(jai, java_addressID);

	/* make the Electric call */
	count = arcpolys(ai, NOWINDOWPART);

	/* convert C result to Java */
	jcount = count;
	return(jcount);
}

JNIEXPORT jobject JNICALL Java_Electric_shapeArcPoly(JNIEnv *env, jobject obj,
	jobject jai, jint jindex)
{
	ARCINST *ai;
	INTBIG index;
	jobject jpoly;
	POLYGON *poly;

	/* convert Java parameters to C */
	if (jai == 0) return(java_nullobject);
	ai = (ARCINST *)(INTBIG)java_environment->GetLongField(jai, java_addressID);
	index = jindex;

	/* make the Electric call */
	poly = allocpolygon(4, db_cluster);
	shapearcpoly(ai, index, poly);

	/* convert C result to Java */
	jpoly = java_makeobject((INTBIG)poly, VPOLYGON);
	return(jpoly);
}

JNIEXPORT jint JNICALL Java_Electric_arcProtoWidthOffset(JNIEnv *env, jobject obj,
	jobject jap)
{
	ARCPROTO *ap;
	INTBIG offset;
	jint joffset;

	/* convert Java parameters to C */
	if (jap == 0) return(0);
	ap = (ARCPROTO *)(INTBIG)java_environment->GetLongField(jap, java_addressID);

	/* make the Electric call */
	offset = arcprotowidthoffset(ap);

	/* convert C result to Java */
	joffset = offset;
	return(joffset);
}

/****************************** ELECTRIC PORTPROTO ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_newPortProto(JNIEnv *env, jobject obj,
	jobject jnp, jobject jni, jobject jpp, jstring jname)
{
	NODEPROTO *np;
	jboolean isCopy;
	NODEINST *ni;
	PORTPROTO *pp, *newpp;
	CHAR *name;
	jobject jnewpp;

	/* convert Java parameters to C */
	if (jnp == 0) return(java_nullobject);
	if (jni == 0) return(java_nullobject);
	if (jpp == 0) return(java_nullobject);
	np = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jnp, java_addressID);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	pp = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp, java_addressID);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	newpp = newportproto(np, ni, pp, name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jnewpp = java_makeobject((INTBIG)newpp, VPORTPROTO);
	return(jnewpp);
}

JNIEXPORT jobjectArray JNICALL Java_Electric_portPosition(JNIEnv *env, jobject obj,
	jobject jni, jobject jpp)
{
	NODEINST *ni;
	PORTPROTO *pp;
	INTBIG x, y;
	jobjectArray retarray;

	/* convert Java parameters to C */
	if (jni == 0) return((jobjectArray)java_nullobject);
	if (jpp == 0) return((jobjectArray)java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	pp = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp, java_addressID);

	/* make the Electric call */
	portposition(ni, pp, &x, &y);

	/* convert C result to Java */
	retarray = java_environment->NewObjectArray(2, java_classint, NULL);
	java_environment->SetObjectArrayElement(retarray, 0, java_makeobject(x, VINTEGER));
	java_environment->SetObjectArrayElement(retarray, 1, java_makeobject(y, VINTEGER));
	return(retarray);
}

JNIEXPORT jobject JNICALL Java_Electric_getPortProto(JNIEnv *env, jobject obj,
	jobject jnp, jstring jname)
{
	NODEPROTO *np;
	PORTPROTO *pp;
	jboolean isCopy;
	CHAR *name;
	jobject jpp;

	/* convert Java parameters to C */
	if (jnp == 0) return(java_nullobject);
	np = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jnp, java_addressID);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	pp = getportproto(np, name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jpp = java_makeobject((INTBIG)pp, VPORTPROTO);
	return(jpp);
}

JNIEXPORT jint JNICALL Java_Electric_killPortProto(JNIEnv *env, jobject obj,
	jobject jnp, jobject jpp)
{
	NODEPROTO *np;
	PORTPROTO *pp;
	BOOLEAN ret;
	jint jret;

	/* convert Java parameters to C */
	if (jnp == 0) return(1);
	if (jpp == 0) return(1);
	np = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jnp, java_addressID);
	pp = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp, java_addressID);

	/* make the Electric call */
	ret = killportproto(np, pp);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

JNIEXPORT jint JNICALL Java_Electric_movePortProto(JNIEnv *env, jobject obj,
	jobject jnp, jobject joldpp, jobject jni, jobject jpp)
{
	NODEPROTO *np;
	NODEINST *ni;
	PORTPROTO *pp, *oldpp;
	BOOLEAN ret;
	jint jret;

	/* convert Java parameters to C */
	if (jnp == 0) return(1);
	if (joldpp == 0) return(1);
	if (jni == 0) return(1);
	if (jpp == 0) return(1);
	np = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jnp, java_addressID);
	oldpp = (PORTPROTO *)(INTBIG)java_environment->GetLongField(joldpp, java_addressID);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	pp = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp, java_addressID);

	/* make the Electric call */
	ret = moveportproto(np, oldpp, ni, pp);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

JNIEXPORT jobject JNICALL Java_Electric_shapePortPoly(JNIEnv *env, jobject obj,
	jobject jni, jobject jpp)
{
	NODEINST *ni;
	PORTPROTO *pp;
	jobject jpoly;
	POLYGON *poly;

	/* convert Java parameters to C */
	if (jni == 0 || jpp == 0) return(java_nullobject);
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	pp = (PORTPROTO *)(INTBIG)java_environment->GetLongField(jpp, java_addressID);

	/* make the Electric call */
	poly = allocpolygon(4, db_cluster);
	shapeportpoly(ni, pp, poly, FALSE);

	/* convert C result to Java */
	jpoly = java_makeobject((INTBIG)poly, VPOLYGON);
	return(jpoly);
}

/****************************** ELECTRIC CHANGE CONTROL ROUTINES ******************************/

JNIEXPORT jint JNICALL Java_Electric_undoABatch(JNIEnv *env, jobject obj)
{
	TOOL *tool;
	INTBIG ret;
	jint jret;

	/* make the Electric call */
	ret = undoabatch(&tool);

	/* convert C result to Java */
	jret = ret;
	return(jret);
}

JNIEXPORT void JNICALL Java_Electric_noUndoAllowed(JNIEnv *env, jobject obj)
{
	/* make the Electric call */
	noundoallowed();
}

JNIEXPORT void JNICALL Java_Electric_flushChanges(JNIEnv *env, jobject obj)
{
	/* make the Electric call */
	(*el_curconstraint->solve)(NONODEPROTO);
	us_endchanges(NOWINDOWPART);
}

/****************************** ELECTRIC VIEW ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_getView(JNIEnv *env, jobject obj,
	jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	VIEW *view;
	jobject jview;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	view = getview(name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jview = java_makeobject((INTBIG)view, VVIEW);
	return(jview);
}

JNIEXPORT jobject JNICALL Java_Electric_newView(JNIEnv *env, jobject obj,
	jstring jname, jstring jsname)
{
	jboolean isCopy1, isCopy2;
	CHAR *name, *sname;
	VIEW *view;
	jobject jview;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy1);
	sname = (CHAR *)java_environment->GetStringUTFChars(jsname, &isCopy2);

	/* make the Electric call */
	view = newview(name, sname);
	if (isCopy1 == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);
	if (isCopy2 == JNI_TRUE) java_environment->ReleaseStringUTFChars(jsname, sname);

	/* convert C result to Java */
	jview = java_makeobject((INTBIG)view, VVIEW);
	return(jview);
}

JNIEXPORT jint JNICALL Java_Electric_killView(JNIEnv *env, jobject obj,
	jobject jview)
{
	VIEW *view;
	BOOLEAN ret;
	jint jret;

	/* convert Java parameters to C */
	if (jview == 0) return(1);
	view = (VIEW *)(INTBIG)java_environment->GetLongField(jview, java_addressID);

	/* make the Electric call */
	ret = killview(view);

	/* convert C result to Java */
	jret = ret?1:0;
	return(jret);
}

/****************************** ELECTRIC MISCELLANEOUS ROUTINES ******************************/

JNIEXPORT jobject JNICALL Java_Electric_getArcProto(JNIEnv *env, jobject obj,
	jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	ARCPROTO *ap;
	jobject jap;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	ap = getarcproto(name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jap = java_makeobject((INTBIG)ap, VARCPROTO);
	return(jap);
}

JNIEXPORT jobject JNICALL Java_Electric_getTechnology(JNIEnv *env, jobject obj,
	jstring jname)
{
	jboolean isCopy;
	CHAR *name;
	TECHNOLOGY *tech;
	jobject jtech;

	/* convert Java parameters to C */
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);

	/* make the Electric call */
	tech = gettechnology(name);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jtech = java_makeobject((INTBIG)tech, VTECHNOLOGY);
	return(jtech);
}

JNIEXPORT jobject JNICALL Java_Electric_getPinProto(JNIEnv *env, jobject obj,
	jobject jap)
{
	ARCPROTO *ap;
	NODEPROTO *np;
	jobject jnp;

	/* convert Java parameters to C */
	if (jap == 0) return(java_nullobject);
	ap = (ARCPROTO *)(INTBIG)java_environment->GetLongField(jap, java_addressID);

	/* make the Electric call */
	np = getpinproto(ap);

	/* convert C result to Java */
	jnp = java_makeobject((INTBIG)np, VNODEPROTO);
	return(jnp);
}

JNIEXPORT jobject JNICALL Java_Electric_getNetwork(JNIEnv *env, jobject obj,
	jstring jname, jobject jnp)
{
	NETWORK *net;
	NODEPROTO *np;
	jobject jnet;
	jboolean isCopy;
	CHAR *name;

	/* convert Java parameters to C */
	if (jnp == 0) return(java_nullobject);
	name = (CHAR *)java_environment->GetStringUTFChars(jname, &isCopy);
	np = (NODEPROTO *)(INTBIG)java_environment->GetLongField(jnp, java_addressID);

	/* make the Electric call */
	net = getnetwork(name, np);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jname, name);

	/* convert C result to Java */
	jnet = java_makeobject((INTBIG)net, VNETWORK);
	return(jnet);
}

JNIEXPORT jstring JNICALL Java_Electric_layerName(JNIEnv *env, jobject obj,
	jobject jtech, jint jlayer)
{
	TECHNOLOGY *tech;
	INTBIG layer;
	CHAR *name;
	jstring jname;

	/* convert Java parameters to C */
	if (jtech == 0) return(0);
	tech = (TECHNOLOGY *)(INTBIG)java_environment->GetLongField(jtech, java_addressID);
	layer = jlayer;

	/* make the Electric call */
	name = layername(tech, layer);

	/* convert C result to Java */
	jname = java_environment->NewStringUTF(string1byte(name));
	return(jname);
}

JNIEXPORT jint JNICALL Java_Electric_layerFunction(JNIEnv *env, jobject obj,
	jobject jtech, jint jlayer)
{
	TECHNOLOGY *tech;
	INTBIG layer, fun;
	jint jfun;

	/* convert Java parameters to C */
	if (jtech == 0) return(0);
	tech = (TECHNOLOGY *)(INTBIG)java_environment->GetLongField(jtech, java_addressID);
	layer = jlayer;

	/* make the Electric call */
	fun = layerfunction(tech, layer);

	/* convert C result to Java */
	jfun = fun;
	return(jfun);
}

JNIEXPORT jint JNICALL Java_Electric_maxDRCSurround(JNIEnv *env, jobject obj,
	jobject jtech, jobject jlib, jint jlayer)
{
	TECHNOLOGY *tech;
	LIBRARY *lib;
	INTBIG layer, dist;
	jint jdist;

	/* convert Java parameters to C */
	if (jtech == 0 || jlib == 0) return(0);
	tech = (TECHNOLOGY *)(INTBIG)java_environment->GetLongField(jtech, java_addressID);
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);
	layer = jlayer;

	/* make the Electric call */
	dist = maxdrcsurround(tech, lib, layer);

	/* convert C result to Java */
	jdist = dist;
	return(jdist);
}

JNIEXPORT jint JNICALL Java_Electric_DRCMinDistance(JNIEnv *env, jobject obj,
	jobject jtech, jobject jlib, jint jlayer1, jint jlayer2, jint jconnected)
{
	TECHNOLOGY *tech;
	LIBRARY *lib;
	INTBIG layer1, layer2, dist, edge;
	BOOLEAN connected;
	jint jdist;

	/* convert Java parameters to C */
	if (jtech == 0 || jlib == 0) return(0);
	tech = (TECHNOLOGY *)(INTBIG)java_environment->GetLongField(jtech, java_addressID);
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);
	layer1 = jlayer1;
	layer2 = jlayer2;
	connected = jconnected != 0 ? TRUE : FALSE;

	/* make the Electric call */
	dist = drcmindistance(tech, lib, layer1, 0, layer2, 0, connected, FALSE, &edge, 0);

	/* convert C result to Java */
	jdist = dist;
	return(jdist);
}

JNIEXPORT jint JNICALL Java_Electric_DRCMinWidth(JNIEnv *env, jobject obj,
	jobject jtech, jobject jlib, jint jlayer)
{
	TECHNOLOGY *tech;
	LIBRARY *lib;
	INTBIG layer, dist;
	jint jdist;

	/* convert Java parameters to C */
	if (jtech == 0 || jlib == 0) return(0);
	tech = (TECHNOLOGY *)(INTBIG)java_environment->GetLongField(jtech, java_addressID);
	lib = (LIBRARY *)(INTBIG)java_environment->GetLongField(jlib, java_addressID);
	layer = jlayer;

	/* make the Electric call */
	dist = drcminwidth(tech, lib, layer, 0);

	/* convert C result to Java */
	jdist = dist;
	return(jdist);
}

JNIEXPORT void JNICALL Java_Electric_xformPoly(JNIEnv *env, jobject obj,
	jobject jpoly, jobject jtrans)
{
	XARRAY trans;
	POLYGON *poly;
	INTBIG i;
	jintArray vArray;

	/* convert Java parameters to C */
	if (jpoly == 0 || jtrans == 0) return;
	poly = (POLYGON *)(INTBIG)java_environment->GetLongField(jpoly, java_addressID);
	vArray = (jintArray)java_environment->GetObjectField(jtrans, java_xarrayVID);
	java_environment->GetIntArrayRegion(vArray, 0, 9, (jint *)&trans[0][0]);

	/* make the Electric call */
	for(i=0; i<poly->count; i++)
		xform(poly->xv[i], poly->yv[i], &poly->xv[i], &poly->yv[i], trans);
}

JNIEXPORT jobject JNICALL Java_Electric_transMult(JNIEnv *env, jobject obj,
	jobject jtransa, jobject jtransb)
{
	XARRAY transa, transb, transc;
	jintArray vArray;
	jobject object;

	/* convert Java parameters to C */
	if (jtransa == 0 || jtransb == 0) return(0);
	vArray = (jintArray)java_environment->GetObjectField(jtransa, java_xarrayVID);
	java_environment->GetIntArrayRegion(vArray, 0, 9, (jint *)&transa[0][0]);
	vArray = (jintArray)java_environment->GetObjectField(jtransb, java_xarrayVID);
	java_environment->GetIntArrayRegion(vArray, 0, 9, (jint *)&transb[0][0]);

	/* make the Electric call */
	transmult(transa, transb, transc);

	/* convert C result to Java */
	object = java_environment->AllocObject(java_classxarray);
	vArray = java_environment->NewIntArray(9);
	java_environment->SetIntArrayRegion(vArray, (jsize)0, (jsize)9, (jint *)&transc[0][0]);
	java_environment->SetObjectField(object, java_xarrayVID, (jobject)vArray);
	return(object);
}

JNIEXPORT void JNICALL Java_Electric_freePolygon(JNIEnv *env, jobject obj,
	jobject jpoly)
{
	POLYGON *poly;

	/* convert Java parameters to C */
	if (jpoly == 0) return;
	poly = (POLYGON *)(INTBIG)java_environment->GetLongField(jpoly, java_addressID);

	/* make the Electric call */
	freepolygon(poly);
}

JNIEXPORT void JNICALL Java_Electric_beginTraverseHierarchy(JNIEnv *env, jobject obj)
{
	/* make the Electric call */
	begintraversehierarchy();
}

JNIEXPORT void JNICALL Java_Electric_downHierarchy(JNIEnv *env, jobject obj,
	jobject jni, jint jindex)
{
	NODEINST *ni;
	NODEPROTO *np, *cnp;
	INTBIG index;

	/* convert Java parameters to C */
	if (jni == 0) return;
	ni = (NODEINST *)(INTBIG)java_environment->GetLongField(jni, java_addressID);
	index = jindex;

	/* make the Electric call */
	np = ni->proto;
	cnp = contentsview(np);
	if (cnp != NONODEPROTO) np = cnp;
	downhierarchy(ni, cnp, index);
}

JNIEXPORT void JNICALL Java_Electric_upHierarchy(JNIEnv *env, jobject obj)
{
	/* make the Electric call */
	uphierarchy();
}

JNIEXPORT void JNICALL Java_Electric_endTraverseHierarchy(JNIEnv *env, jobject obj)
{
	/* make the Electric call */
	endtraversehierarchy();
}

JNIEXPORT jobject JNICALL Java_Electric_getTraversalPath(JNIEnv *env, jobject obj)
{
	NODEINST **nilist;
	INTBIG depth, i, *indexlist;
	jobjectArray retarray;

	/* make the Electric call */
	gettraversalpath(el_curlib->curnodeproto, NOWINDOWPART, &nilist, &indexlist, &depth, 0);

	/* convert C result to Java */
	retarray = java_environment->NewObjectArray((depth+1), java_classnodeinst, NULL);
	for(i=0; i<depth; i++)
		java_environment->SetObjectArrayElement(retarray, i,
			java_makeobject((INTBIG)nilist[i], VNODEINST));
	java_environment->SetObjectArrayElement(retarray, depth,
		java_makeobject((INTBIG)NONODEINST, VNODEINST));
	return(retarray);
}

/****************************** SUPPORT ******************************/

/*
 * Helper routine to buffer characters that are sent to standard output.
 * Writes a line when carriage-return is typed.
 */
void java_addcharacter(CHAR chr)
{
	if (chr != '\n')
		java_outputbuffer[java_outputposition++] = chr;

	if (chr == '\n' || java_outputposition >= MAXLINE)
	{
		if (java_outputposition == 0)
			java_outputbuffer[java_outputposition++] = ' ';
		java_outputbuffer[java_outputposition] = 0;
		ttyputmsg(x_("%s"), java_outputbuffer);
		java_outputposition = 0;
	}
}

/*
 * Routine that is called when a single character is written to the
 * standard output.
 */
JNIEXPORT void JNICALL Java_Electric_eoutWriteOne(JNIEnv *env, jobject obj,
	jint byte)
{
	java_addcharacter((CHAR)byte);
}

/*
 * Routine that is called when a string is written to the
 * standard output.
 */
JNIEXPORT void JNICALL Java_Electric_eoutWriteString(JNIEnv *env, jobject obj,
	jstring jstr)
{
	CHAR *str;
	INTBIG i, len;
	jboolean isCopy;

	str = (CHAR *)java_environment->GetStringUTFChars(jstr, &isCopy);
	len = estrlen(str);
	for(i=0; i<len; i++) java_addcharacter(str[i]);
	if (isCopy == JNI_TRUE) java_environment->ReleaseStringUTFChars(jstr, str);
}

jint JNICALL java_vfprintf(FILE *fp, const CHAR1 *format, va_list args)
{
	us_ttyprint(FALSE, string2byte((CHAR1 *)format), args);
	return(1);
}

/*
 * Coroutine that is called when Java exits.
 */
void JNICALL java_exit(jint code)
{
	ttyputmsg(_("Exiting Java with code %d"), code);
}

/*
 * Routine to ensure that the global "java_arraybuffer" has at least "size" bytes in
 * it.  Returns true on error.
 */
BOOLEAN java_allocarraybuffer(INTBIG size)
{
	if (size <= java_arraybuffersize) return(FALSE);
	if (java_arraybuffersize > 0)
		efree((CHAR *)java_arraybuffer);
	java_arraybuffersize = 0;
	java_arraybuffer = (CHAR *)emalloc(size, db_cluster);
	if (java_arraybuffer == 0) return(TRUE);
	java_arraybuffersize = size;
	return(FALSE);
}

/*
 * Routine to convert the Java object "obj" into an "address/type" pair
 * in "addr" and "type".
 */
void java_getobjectaddrtype(jobject obj, INTBIG *addr, INTBIG *type, CHAR **description)
{
	CHAR *rstr;
	double dvalue;
	float fvalue;
	jboolean isCopy;
	INTBIG arrlen, i;
	jint *jarrdata;
	static CHAR mydescr[300];
	REGISTER void *infstr;

	/* defaults */
	*addr = 0;
	*type = VUNKNOWN;
	if (description != 0) *description = mydescr;

	/* Integer */
	if (java_environment->IsInstanceOf(obj, java_classint))
	{
		*addr = java_environment->CallIntMethod(obj, java_midIntValue);
		*type = VINTEGER;
		if (description != 0)
			esnprintf(mydescr, 300, x_("%ld"), *addr);
		return;
	}

	/* Float */
	if (java_environment->IsInstanceOf(obj, java_classfloat))
	{
		fvalue = java_environment->CallFloatMethod(obj, java_midFloatValue);
		*addr = castint(fvalue);
		*type = VFLOAT;
		if (description != 0)
			esnprintf(mydescr, 300, x_("%g"), fvalue);
		return;
	}

	/* Double */
	if (java_environment->IsInstanceOf(obj, java_classdouble))
	{
		dvalue = java_environment->CallDoubleMethod(obj, java_midDoubleValue);
		*addr = castint((float)dvalue);
		*type = VFLOAT;
		if (description != 0)
			esnprintf(mydescr, 300, x_("%g"), dvalue);
		return;
	}

	/* String */
	if (java_environment->IsInstanceOf(obj, java_classstring))
	{
		rstr = (CHAR *)java_environment->GetStringUTFChars((jstring)obj, &isCopy);
		infstr = initinfstr();
		addstringtoinfstr(infstr, rstr);
		if (isCopy == JNI_TRUE)
			java_environment->ReleaseStringUTFChars((jstring)obj, rstr);
		*addr = (INTBIG)returninfstr(infstr);
		*type = VSTRING;
		if (description != 0)
			*description = (CHAR *)*addr;
		return;
	}

	/* the Electric classes */
	if (java_environment->IsInstanceOf(obj, java_classnodeinst))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VNODEINST;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Nodeinst(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classnodeproto))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VNODEPROTO;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Nodeproto(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classportarcinst))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VPORTARCINST;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Portarcinst(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classportexpinst))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VPORTEXPINST;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Portexpinst(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classportproto))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VPORTPROTO;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Portproto(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classarcinst))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VARCINST;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Arcinst(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classarcproto))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VARCPROTO;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Arcproto(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classgeom))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VGEOM;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Geom(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classlibrary))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VLIBRARY;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Library(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classtechnology))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VTECHNOLOGY;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Technology(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classtool))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VTOOL;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Tool(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classrtnode))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VRTNODE;
		if (description != 0)
			esnprintf(mydescr, 300, x_("RTnode(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classnetwork))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VNETWORK;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Network(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classview))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VVIEW;
		if (description != 0)
			esnprintf(mydescr, 300, x_("View(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classwindowpart))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VWINDOWPART;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Windowpart(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classwindowframe))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VWINDOWFRAME;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Windowframe(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classgraphics))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VGRAPHICS;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Graphics(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classconstraint))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VCONSTRAINT;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Constraint(%ld)"), *addr);
		return;
	}
	if (java_environment->IsInstanceOf(obj, java_classpolygon))
	{
		*addr = (INTBIG)java_environment->GetLongField(obj, java_addressID);
		*type = VPOLYGON;
		if (description != 0)
			esnprintf(mydescr, 300, x_("Polygon(%ld)"), *addr);
		return;
	}

	/* int arrays */
	if (java_environment->IsInstanceOf(obj, java_classarrayint))
	{
		arrlen = java_environment->GetArrayLength((jintArray)obj);
		jarrdata = java_environment->GetIntArrayElements((jintArray)obj, &isCopy);
		if (java_allocarraybuffer(arrlen*SIZEOFINTBIG)) return;
		for(i=0; i<arrlen; i++)
			((INTBIG *)java_arraybuffer)[i] = jarrdata[i];
		if (isCopy == JNI_TRUE)
			java_environment->ReleaseIntArrayElements((jintArray)obj, jarrdata, JNI_ABORT);
		*addr = (INTBIG)java_arraybuffer;
		*type = VINTEGER | VISARRAY | (arrlen << VLENGTHSH);
		if (description != 0)
			esnprintf(mydescr, 300, x_("Integer Array"));
		return;
	}

	/* unknown object: get its name and print an error */
	jclass objectClass = java_environment->GetObjectClass(obj);
	jclass objClazz = java_environment->GetObjectClass(objectClass);
	jmethodID mid = java_environment->GetMethodID(objClazz, b_("toString"),b_("()Ljava/lang/String;"));
	jstring jMethodName = (jstring)java_environment->CallObjectMethod(objectClass, mid, NULL);
	CHAR *thisMethodName = (CHAR *)java_environment->GetStringUTFChars(jMethodName, &isCopy);
	if (description != 0)
		estrcpy(mydescr, thisMethodName);
	if (isCopy == JNI_TRUE)
		java_environment->ReleaseStringUTFChars((jstring)jMethodName, thisMethodName);
}

/*
 * Routine to create a Java object that describes object "addr" of type
 * "type".
 */
jobject java_makeobject(INTBIG addr, INTBIG type)
{
	jobject object;
	INTBIG stype, saddr;

	/* create an object of type "object" */
	switch (type&VTYPE)
	{
		case VINTEGER:
		case VSHORT:
		case VBOOLEAN:
			object = java_makejavaobject(addr, type, java_classint, VINTEGER);
			break;
		case VFLOAT:
			object = java_makejavaobject(addr, type, java_classfloat, VFLOAT);
			break;
		case VFRACT:
			object = java_makejavaobject(addr, type, java_classfloat, VFRACT);
			break;
		case VSTRING:
			getsimpletype((CHAR *)addr, &stype, &saddr, 0);
			if (stype == VSTRING)
			{
				object = java_environment->NewStringUTF((CHAR *)addr);
			} else if (stype == VFLOAT)
			{
				type = (type & ~VTYPE) | VFLOAT;
				object = java_makejavaobject(addr, type, java_classfloat, VFLOAT);
			} else
			{
				type = (type & ~VTYPE) | (stype & VTYPE);
				object = java_makejavaobject(saddr, type, java_classint, VINTEGER);
			}
			break;
		case VNODEINST:
			object = java_makejavaobject(addr, type, java_classnodeinst, VNODEINST);
			break;
		case VNODEPROTO:
			object = java_makejavaobject(addr, type, java_classnodeproto, VNODEPROTO);
			break;
		case VPORTARCINST:
			object = java_makejavaobject(addr, type, java_classportarcinst, VPORTARCINST);
			break;
		case VPORTEXPINST:
			object = java_makejavaobject(addr, type, java_classportexpinst, VPORTEXPINST);
			break;
		case VPORTPROTO:
			object = java_makejavaobject(addr, type, java_classportproto, VPORTPROTO);
			break;
		case VARCINST:
			object = java_makejavaobject(addr, type, java_classarcinst, VARCINST);
			break;
		case VARCPROTO:
			object = java_makejavaobject(addr, type, java_classarcproto, VARCPROTO);
			break;
		case VGEOM:
			object = java_makejavaobject(addr, type, java_classgeom, VGEOM);
			break;
		case VLIBRARY:
			object = java_makejavaobject(addr, type, java_classlibrary, VLIBRARY);
			break;
		case VTECHNOLOGY:
			object = java_makejavaobject(addr, type, java_classtechnology, VTECHNOLOGY);
			break;
		case VTOOL:
			object = java_makejavaobject(addr, type, java_classtool, VTOOL);
			break;
		case VRTNODE:
			object = java_makejavaobject(addr, type, java_classrtnode, VRTNODE);
			break;
		case VNETWORK:
			object = java_makejavaobject(addr, type, java_classnetwork, VNETWORK);
			break;
		case VVIEW:
			object = java_makejavaobject(addr, type, java_classview, VVIEW);
			break;
		case VWINDOWPART:
			object = java_makejavaobject(addr, type, java_classwindowpart, VWINDOWPART);
			break;
		case VWINDOWFRAME:
			object = java_makejavaobject(addr, type, java_classwindowframe, VWINDOWFRAME);
			break;
		case VGRAPHICS:
			object = java_makejavaobject(addr, type, java_classgraphics, VGRAPHICS);
			break;
		case VCONSTRAINT:
			object = java_makejavaobject(addr, type, java_classconstraint, VCONSTRAINT);
			break;
		case VPOLYGON:
			object = java_makejavaobject(addr, type, java_classpolygon, VPOLYGON);
			break;
		default:
			object = java_nullobject;
			break;
	}
	return(object);
}

jobject java_makejavaobject(INTBIG addr, INTBIG type, jclass javaclass, INTBIG desttype)
{
	VARIABLE myvar;
	REGISTER INTBIG i, len;
	jobjectArray objArray;
	jobject object, subobject;

	if ((type&VISARRAY) != 0)
	{
		myvar.addr = addr;   myvar.type = type;
		len = getlength(&myvar);
		objArray = java_environment->NewObjectArray(len, javaclass, NULL);
		for(i=0; i<len; i++)
		{
			subobject = java_makeobject(((INTBIG *)addr)[i], desttype);
			java_environment->SetObjectArrayElement(objArray, i, subobject);
		}
		object = (jobject)objArray;
	} else
	{
		object = java_environment->AllocObject(javaclass);
		if ((type&VTYPE) == VFLOAT)
		{
			java_environment->CallVoidMethod(object, java_midFloatInit, (jfloat)castfloat(addr));
		} else if ((type&VTYPE) == VFRACT)
		{
			java_environment->CallVoidMethod(object, java_midFloatInit, addr/(jfloat)WHOLE);
		} else if ((type&VTYPE) == VINTEGER || (type&VTYPE) == VSHORT || (type&VTYPE) == VBOOLEAN)
		{
			java_environment->CallVoidMethod(object, java_midIntInit, (jint)addr);
		} else
		{
			java_environment->SetLongField(object, java_addressID, (jlong)addr);
		}
	}
	return(object);
}

/* added by MW, 5/20/02 */
CHAR *java_finddbmirror(void)
{
	return x_("dbmirror.jar");
}

CHAR *java_findbeanshell(void)
{
	INTBIG count, i;
	INTBIG len;
	CHAR **filelist, *pt;
	REGISTER void *infstr;

	infstr = initinfstr();
	addstringtoinfstr(infstr, el_libdir);
	addstringtoinfstr(infstr, x_("java"));
	addtoinfstr(infstr, DIRSEP);
	count = filesindirectory(returninfstr(infstr), &filelist);
	for(i=0; i<count; i++)
	{
		pt = filelist[i];
		if (estrncmp(pt, x_("bsh-"), 4) != 0) continue;
		len = estrlen(pt);
		if (estrcmp(&pt[len-4], x_(".jar")) != 0) continue;
		return(pt);
	}
	return(0);
}

#endif
