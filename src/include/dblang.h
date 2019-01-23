/* -*- tab-width: 4 -*-
 *
 * Electric(tm) VLSI Design System
 *
 * File: dblang.h
 * Interpretive languages header module
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

/***************************** LISP DECLARATIONS *****************************/

#if LANGLISP
# include "elkkernel.h"
  void      lsp_init(void);
  ELKObject lsp_makeobject(CHAR*);
  BOOLEAN   lsp_describeobject(ELKObject, INTBIG, INTBIG*);
#endif

/***************************** TCL DECLARATIONS *****************************/

#if LANGTCL
# include "tcl.h"
  void    el_tclinterpreter(Tcl_Interp *interp);
  INTBIG  tcl_converttoelectric(CHAR *tclstr, INTBIG type);
  void    tcl_nativemenuinitialize(void);
  BOOLEAN tcl_nativemenuload(INTBIG count, CHAR *par[]);
  extern Tcl_Interp *tcl_interp;
  extern CHAR *tcl_outputbuffer, *tcl_outputloc;
#endif

/***************************** JAVA DECLARATIONS *****************************/

#if LANGJAVA
# include "jni.h"
# define METHODRETURNSERROR  -1
# define METHODRETURNSVOID    0
# define METHODRETURNSOBJECT  1
# define METHODRETURNSBOOLEAN 2
# define METHODRETURNSBYTE    3
# define METHODRETURNSCHAR    4
# define METHODRETURNSSHORT   5
# define METHODRETURNSINT     6
# define METHODRETURNSLONG    7
# define METHODRETURNSFLOAT   8
# define METHODRETURNSDOUBLE  9
  CHAR   *java_init(void);
  BOOLEAN java_evaluate(CHAR *str);
  CHAR   *java_query(CHAR *str, INTBIG *methodreturntype);
  void    java_freememory(void);
  BOOLEAN java_addprivatenatives(CHAR1 *methodname, CHAR1 *methoddescription, void *method);
  jobject java_makeobject(INTBIG addr, INTBIG type);

  extern JNIEnv  *java_environment;
#endif

#if defined(__cplusplus) && !defined(ALLCPLUSPLUS)
}
#endif
