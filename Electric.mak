# Microsoft Developer Studio Generated NMAKE File, Based on Electric.dsp
!IF "$(CFG)" == ""
CFG=Electric - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Electric - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Electric - Win32 Release" && "$(CFG)" != "Electric - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Electric.mak" CFG="Electric - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Electric - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Electric - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Electric - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\Electric.exe"


CLEAN :
	-@erase "$(INTDIR)\aidtable.obj"
	-@erase "$(INTDIR)\compact.obj"
	-@erase "$(INTDIR)\compensate.obj"
	-@erase "$(INTDIR)\conlay.obj"
	-@erase "$(INTDIR)\conlin.obj"
	-@erase "$(INTDIR)\conlingtt.obj"
	-@erase "$(INTDIR)\conlinprs.obj"
	-@erase "$(INTDIR)\conlinttg.obj"
	-@erase "$(INTDIR)\contable.obj"
	-@erase "$(INTDIR)\data.obj"
	-@erase "$(INTDIR)\dbchange.obj"
	-@erase "$(INTDIR)\dbcontour.obj"
	-@erase "$(INTDIR)\dbcontrol.obj"
	-@erase "$(INTDIR)\dbcreate.obj"
	-@erase "$(INTDIR)\dberror.obj"
	-@erase "$(INTDIR)\dbgeom.obj"
	-@erase "$(INTDIR)\dblang.obj"
	-@erase "$(INTDIR)\dblangjava.obj"
	-@erase "$(INTDIR)\dblibrary.obj"
	-@erase "$(INTDIR)\dbmath.obj"
	-@erase "$(INTDIR)\dbmemory.obj"
	-@erase "$(INTDIR)\dbmerge.obj"
	-@erase "$(INTDIR)\dbmult.obj"
	-@erase "$(INTDIR)\dbnoproto.obj"
	-@erase "$(INTDIR)\dbtech.obj"
	-@erase "$(INTDIR)\dbtechi.obj"
	-@erase "$(INTDIR)\dbtext.obj"
	-@erase "$(INTDIR)\dbvars.obj"
	-@erase "$(INTDIR)\drc.obj"
	-@erase "$(INTDIR)\drcbatch.obj"
	-@erase "$(INTDIR)\drcflat.obj"
	-@erase "$(INTDIR)\drcquick.obj"
	-@erase "$(INTDIR)\electric.res"
	-@erase "$(INTDIR)\erc.obj"
	-@erase "$(INTDIR)\ercantenna.obj"
	-@erase "$(INTDIR)\graphcommon.obj"
	-@erase "$(INTDIR)\graphpc.obj"
	-@erase "$(INTDIR)\graphpcchildframe.obj"
	-@erase "$(INTDIR)\graphpccode.obj"
	-@erase "$(INTDIR)\graphpcdialog.obj"
	-@erase "$(INTDIR)\graphpcdialoglistbox.obj"
	-@erase "$(INTDIR)\graphpcdoc.obj"
	-@erase "$(INTDIR)\graphpcmainframe.obj"
	-@erase "$(INTDIR)\graphpcmsgview.obj"
	-@erase "$(INTDIR)\graphpcstdafx.obj"
	-@erase "$(INTDIR)\graphpcview.obj"
	-@erase "$(INTDIR)\io.obj"
	-@erase "$(INTDIR)\iobinaryi.obj"
	-@erase "$(INTDIR)\iobinaryo.obj"
	-@erase "$(INTDIR)\iocifin.obj"
	-@erase "$(INTDIR)\iocifout.obj"
	-@erase "$(INTDIR)\iocifpars.obj"
	-@erase "$(INTDIR)\iodefi.obj"
	-@erase "$(INTDIR)\iodxf.obj"
	-@erase "$(INTDIR)\ioeagleo.obj"
	-@erase "$(INTDIR)\ioecado.obj"
	-@erase "$(INTDIR)\ioedifi.obj"
	-@erase "$(INTDIR)\ioedifo.obj"
	-@erase "$(INTDIR)\iogdsi.obj"
	-@erase "$(INTDIR)\iogdso.obj"
	-@erase "$(INTDIR)\iohpglout.obj"
	-@erase "$(INTDIR)\iolefi.obj"
	-@erase "$(INTDIR)\iolefo.obj"
	-@erase "$(INTDIR)\iolout.obj"
	-@erase "$(INTDIR)\iopadso.obj"
	-@erase "$(INTDIR)\iopsout.obj"
	-@erase "$(INTDIR)\iopsoutcolor.obj"
	-@erase "$(INTDIR)\ioquickdraw.obj"
	-@erase "$(INTDIR)\iosdfi.obj"
	-@erase "$(INTDIR)\iosuei.obj"
	-@erase "$(INTDIR)\iotexti.obj"
	-@erase "$(INTDIR)\iotexto.obj"
	-@erase "$(INTDIR)\iovhdl.obj"
	-@erase "$(INTDIR)\logeffort.obj"
	-@erase "$(INTDIR)\netdiff.obj"
	-@erase "$(INTDIR)\netextract.obj"
	-@erase "$(INTDIR)\netflat.obj"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\pla.obj"
	-@erase "$(INTDIR)\placdecode.obj"
	-@erase "$(INTDIR)\placio.obj"
	-@erase "$(INTDIR)\placngrid.obj"
	-@erase "$(INTDIR)\placpgrid.obj"
	-@erase "$(INTDIR)\placpla.obj"
	-@erase "$(INTDIR)\placutils.obj"
	-@erase "$(INTDIR)\planfacets.obj"
	-@erase "$(INTDIR)\planopt.obj"
	-@erase "$(INTDIR)\planprog1.obj"
	-@erase "$(INTDIR)\planprog2.obj"
	-@erase "$(INTDIR)\projecttool.obj"
	-@erase "$(INTDIR)\rout.obj"
	-@erase "$(INTDIR)\routauto.obj"
	-@erase "$(INTDIR)\routmaze.obj"
	-@erase "$(INTDIR)\routmimic.obj"
	-@erase "$(INTDIR)\routriver.obj"
	-@erase "$(INTDIR)\sc1.obj"
	-@erase "$(INTDIR)\sc1command.obj"
	-@erase "$(INTDIR)\sc1component.obj"
	-@erase "$(INTDIR)\sc1connect.obj"
	-@erase "$(INTDIR)\sc1delete.obj"
	-@erase "$(INTDIR)\sc1electric.obj"
	-@erase "$(INTDIR)\sc1err.obj"
	-@erase "$(INTDIR)\sc1extract.obj"
	-@erase "$(INTDIR)\sc1interface.obj"
	-@erase "$(INTDIR)\sc1maker.obj"
	-@erase "$(INTDIR)\sc1place.obj"
	-@erase "$(INTDIR)\sc1route.obj"
	-@erase "$(INTDIR)\sc1sim.obj"
	-@erase "$(INTDIR)\sim.obj"
	-@erase "$(INTDIR)\simals.obj"
	-@erase "$(INTDIR)\simalscom.obj"
	-@erase "$(INTDIR)\simalsflat.obj"
	-@erase "$(INTDIR)\simalsgraph.obj"
	-@erase "$(INTDIR)\simalssim.obj"
	-@erase "$(INTDIR)\simalsuser.obj"
	-@erase "$(INTDIR)\simfasthenry.obj"
	-@erase "$(INTDIR)\simirsim.obj"
	-@erase "$(INTDIR)\simmaxwell.obj"
	-@erase "$(INTDIR)\simmossim.obj"
	-@erase "$(INTDIR)\simpal.obj"
	-@erase "$(INTDIR)\simsilos.obj"
	-@erase "$(INTDIR)\simsim.obj"
	-@erase "$(INTDIR)\simspice.obj"
	-@erase "$(INTDIR)\simspicerun.obj"
	-@erase "$(INTDIR)\simtexsim.obj"
	-@erase "$(INTDIR)\simverilog.obj"
	-@erase "$(INTDIR)\simwindow.obj"
	-@erase "$(INTDIR)\tecart.obj"
	-@erase "$(INTDIR)\tecbicmos.obj"
	-@erase "$(INTDIR)\tecbipolar.obj"
	-@erase "$(INTDIR)\teccmos.obj"
	-@erase "$(INTDIR)\teccmosdodn.obj"
	-@erase "$(INTDIR)\tecefido.obj"
	-@erase "$(INTDIR)\tecfpga.obj"
	-@erase "$(INTDIR)\tecgem.obj"
	-@erase "$(INTDIR)\tecgen.obj"
	-@erase "$(INTDIR)\tecmocmos.obj"
	-@erase "$(INTDIR)\tecmocmosold.obj"
	-@erase "$(INTDIR)\tecmocmossub.obj"
	-@erase "$(INTDIR)\tecnmos.obj"
	-@erase "$(INTDIR)\tecpcb.obj"
	-@erase "$(INTDIR)\tecrcmos.obj"
	-@erase "$(INTDIR)\tecschem.obj"
	-@erase "$(INTDIR)\tectable.obj"
	-@erase "$(INTDIR)\usr.obj"
	-@erase "$(INTDIR)\usrarc.obj"
	-@erase "$(INTDIR)\usrcheck.obj"
	-@erase "$(INTDIR)\usrcom.obj"
	-@erase "$(INTDIR)\usrcomab.obj"
	-@erase "$(INTDIR)\usrcomcd.obj"
	-@erase "$(INTDIR)\usrcomek.obj"
	-@erase "$(INTDIR)\usrcomln.obj"
	-@erase "$(INTDIR)\usrcomoq.obj"
	-@erase "$(INTDIR)\usrcomrs.obj"
	-@erase "$(INTDIR)\usrcomtv.obj"
	-@erase "$(INTDIR)\usrcomwz.obj"
	-@erase "$(INTDIR)\usrctech.obj"
	-@erase "$(INTDIR)\usrdiacom.obj"
	-@erase "$(INTDIR)\usrdiaedit.obj"
	-@erase "$(INTDIR)\usrdisp.obj"
	-@erase "$(INTDIR)\usreditemacs.obj"
	-@erase "$(INTDIR)\usreditpac.obj"
	-@erase "$(INTDIR)\usredtecc.obj"
	-@erase "$(INTDIR)\usredtecg.obj"
	-@erase "$(INTDIR)\usredtecp.obj"
	-@erase "$(INTDIR)\usrgraph.obj"
	-@erase "$(INTDIR)\usrhigh.obj"
	-@erase "$(INTDIR)\usrmenu.obj"
	-@erase "$(INTDIR)\usrmisc.obj"
	-@erase "$(INTDIR)\usrnet.obj"
	-@erase "$(INTDIR)\usrparse.obj"
	-@erase "$(INTDIR)\usrstatus.obj"
	-@erase "$(INTDIR)\usrterminal.obj"
	-@erase "$(INTDIR)\usrtrack.obj"
	-@erase "$(INTDIR)\usrtranslate.obj"
	-@erase "$(INTDIR)\usrwindow.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vhdl.obj"
	-@erase "$(INTDIR)\vhdlals.obj"
	-@erase "$(INTDIR)\vhdlexpr.obj"
	-@erase "$(INTDIR)\vhdlnetlisp.obj"
	-@erase "$(INTDIR)\vhdlparser.obj"
	-@erase "$(INTDIR)\vhdlquisc.obj"
	-@erase "$(INTDIR)\vhdlsemantic.obj"
	-@erase "$(INTDIR)\vhdlsilos.obj"
	-@erase "$(OUTDIR)\Electric.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /I "src\graph" /I "src\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\Electric.pch" /YX"src\graph\graphpcstdafx.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL" 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\electric.res" /d "NDEBUG" /d "_AFXDLL" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Electric.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS= winmm.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\Electric.pdb" /machine:I386 /out:"$(OUTDIR)\Electric.exe" 
LINK32_OBJS= \
	"$(INTDIR)\conlay.obj" \
	"$(INTDIR)\conlin.obj" \
	"$(INTDIR)\conlingtt.obj" \
	"$(INTDIR)\conlinprs.obj" \
	"$(INTDIR)\conlinttg.obj" \
	"$(INTDIR)\contable.obj" \
	"$(INTDIR)\aidtable.obj" \
	"$(INTDIR)\data.obj" \
	"$(INTDIR)\dbchange.obj" \
	"$(INTDIR)\dbcontour.obj" \
	"$(INTDIR)\dbcontrol.obj" \
	"$(INTDIR)\dbcreate.obj" \
	"$(INTDIR)\dberror.obj" \
	"$(INTDIR)\dbgeom.obj" \
	"$(INTDIR)\dblang.obj" \
	"$(INTDIR)\dblangjava.obj" \
	"$(INTDIR)\dblibrary.obj" \
	"$(INTDIR)\dbmath.obj" \
	"$(INTDIR)\dbmemory.obj" \
	"$(INTDIR)\dbmerge.obj" \
	"$(INTDIR)\dbmult.obj" \
	"$(INTDIR)\dbnoproto.obj" \
	"$(INTDIR)\dbtech.obj" \
	"$(INTDIR)\dbtechi.obj" \
	"$(INTDIR)\dbtext.obj" \
	"$(INTDIR)\dbvars.obj" \
	"$(INTDIR)\drc.obj" \
	"$(INTDIR)\drcbatch.obj" \
	"$(INTDIR)\drcflat.obj" \
	"$(INTDIR)\drcquick.obj" \
	"$(INTDIR)\graphcommon.obj" \
	"$(INTDIR)\graphpc.obj" \
	"$(INTDIR)\graphpcchildframe.obj" \
	"$(INTDIR)\graphpccode.obj" \
	"$(INTDIR)\graphpcdialog.obj" \
	"$(INTDIR)\graphpcdialoglistbox.obj" \
	"$(INTDIR)\graphpcdoc.obj" \
	"$(INTDIR)\graphpcmainframe.obj" \
	"$(INTDIR)\graphpcmsgview.obj" \
	"$(INTDIR)\graphpcstdafx.obj" \
	"$(INTDIR)\graphpcview.obj" \
	"$(INTDIR)\io.obj" \
	"$(INTDIR)\iobinaryi.obj" \
	"$(INTDIR)\iobinaryo.obj" \
	"$(INTDIR)\iocifin.obj" \
	"$(INTDIR)\iocifout.obj" \
	"$(INTDIR)\iocifpars.obj" \
	"$(INTDIR)\iodefi.obj" \
	"$(INTDIR)\iodxf.obj" \
	"$(INTDIR)\ioeagleo.obj" \
	"$(INTDIR)\ioecado.obj" \
	"$(INTDIR)\ioedifi.obj" \
	"$(INTDIR)\ioedifo.obj" \
	"$(INTDIR)\iogdsi.obj" \
	"$(INTDIR)\iogdso.obj" \
	"$(INTDIR)\iohpglout.obj" \
	"$(INTDIR)\iolefi.obj" \
	"$(INTDIR)\iolefo.obj" \
	"$(INTDIR)\iolout.obj" \
	"$(INTDIR)\iopadso.obj" \
	"$(INTDIR)\iopsout.obj" \
	"$(INTDIR)\iopsoutcolor.obj" \
	"$(INTDIR)\ioquickdraw.obj" \
	"$(INTDIR)\iosdfi.obj" \
	"$(INTDIR)\iosuei.obj" \
	"$(INTDIR)\iotexti.obj" \
	"$(INTDIR)\iotexto.obj" \
	"$(INTDIR)\iovhdl.obj" \
	"$(INTDIR)\compact.obj" \
	"$(INTDIR)\compensate.obj" \
	"$(INTDIR)\erc.obj" \
	"$(INTDIR)\ercantenna.obj" \
	"$(INTDIR)\logeffort.obj" \
	"$(INTDIR)\projecttool.obj" \
	"$(INTDIR)\netdiff.obj" \
	"$(INTDIR)\netextract.obj" \
	"$(INTDIR)\netflat.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\pla.obj" \
	"$(INTDIR)\placdecode.obj" \
	"$(INTDIR)\placio.obj" \
	"$(INTDIR)\placngrid.obj" \
	"$(INTDIR)\placpgrid.obj" \
	"$(INTDIR)\placpla.obj" \
	"$(INTDIR)\placutils.obj" \
	"$(INTDIR)\planfacets.obj" \
	"$(INTDIR)\planopt.obj" \
	"$(INTDIR)\planprog1.obj" \
	"$(INTDIR)\planprog2.obj" \
	"$(INTDIR)\rout.obj" \
	"$(INTDIR)\routauto.obj" \
	"$(INTDIR)\routmaze.obj" \
	"$(INTDIR)\routmimic.obj" \
	"$(INTDIR)\routriver.obj" \
	"$(INTDIR)\sc1.obj" \
	"$(INTDIR)\sc1command.obj" \
	"$(INTDIR)\sc1component.obj" \
	"$(INTDIR)\sc1connect.obj" \
	"$(INTDIR)\sc1delete.obj" \
	"$(INTDIR)\sc1electric.obj" \
	"$(INTDIR)\sc1err.obj" \
	"$(INTDIR)\sc1extract.obj" \
	"$(INTDIR)\sc1interface.obj" \
	"$(INTDIR)\sc1maker.obj" \
	"$(INTDIR)\sc1place.obj" \
	"$(INTDIR)\sc1route.obj" \
	"$(INTDIR)\sc1sim.obj" \
	"$(INTDIR)\sim.obj" \
	"$(INTDIR)\simals.obj" \
	"$(INTDIR)\simalscom.obj" \
	"$(INTDIR)\simalsflat.obj" \
	"$(INTDIR)\simalsgraph.obj" \
	"$(INTDIR)\simalssim.obj" \
	"$(INTDIR)\simalsuser.obj" \
	"$(INTDIR)\simfasthenry.obj" \
	"$(INTDIR)\simirsim.obj" \
	"$(INTDIR)\simmaxwell.obj" \
	"$(INTDIR)\simmossim.obj" \
	"$(INTDIR)\simpal.obj" \
	"$(INTDIR)\simsilos.obj" \
	"$(INTDIR)\simsim.obj" \
	"$(INTDIR)\simspice.obj" \
	"$(INTDIR)\simspicerun.obj" \
	"$(INTDIR)\simtexsim.obj" \
	"$(INTDIR)\simverilog.obj" \
	"$(INTDIR)\simwindow.obj" \
	"$(INTDIR)\tecart.obj" \
	"$(INTDIR)\tecbicmos.obj" \
	"$(INTDIR)\tecbipolar.obj" \
	"$(INTDIR)\teccmos.obj" \
	"$(INTDIR)\teccmosdodn.obj" \
	"$(INTDIR)\tecefido.obj" \
	"$(INTDIR)\tecfpga.obj" \
	"$(INTDIR)\tecgem.obj" \
	"$(INTDIR)\tecgen.obj" \
	"$(INTDIR)\tecmocmos.obj" \
	"$(INTDIR)\tecmocmosold.obj" \
	"$(INTDIR)\tecmocmossub.obj" \
	"$(INTDIR)\tecnmos.obj" \
	"$(INTDIR)\tecpcb.obj" \
	"$(INTDIR)\tecrcmos.obj" \
	"$(INTDIR)\tecschem.obj" \
	"$(INTDIR)\tectable.obj" \
	"$(INTDIR)\usr.obj" \
	"$(INTDIR)\usrarc.obj" \
	"$(INTDIR)\usrcheck.obj" \
	"$(INTDIR)\usrcom.obj" \
	"$(INTDIR)\usrcomab.obj" \
	"$(INTDIR)\usrcomcd.obj" \
	"$(INTDIR)\usrcomek.obj" \
	"$(INTDIR)\usrcomln.obj" \
	"$(INTDIR)\usrcomoq.obj" \
	"$(INTDIR)\usrcomrs.obj" \
	"$(INTDIR)\usrcomtv.obj" \
	"$(INTDIR)\usrcomwz.obj" \
	"$(INTDIR)\usrctech.obj" \
	"$(INTDIR)\usrdiacom.obj" \
	"$(INTDIR)\usrdiaedit.obj" \
	"$(INTDIR)\usrdisp.obj" \
	"$(INTDIR)\usreditemacs.obj" \
	"$(INTDIR)\usreditpac.obj" \
	"$(INTDIR)\usredtecc.obj" \
	"$(INTDIR)\usredtecg.obj" \
	"$(INTDIR)\usredtecp.obj" \
	"$(INTDIR)\usrgraph.obj" \
	"$(INTDIR)\usrhigh.obj" \
	"$(INTDIR)\usrmenu.obj" \
	"$(INTDIR)\usrmisc.obj" \
	"$(INTDIR)\usrnet.obj" \
	"$(INTDIR)\usrparse.obj" \
	"$(INTDIR)\usrstatus.obj" \
	"$(INTDIR)\usrterminal.obj" \
	"$(INTDIR)\usrtrack.obj" \
	"$(INTDIR)\usrtranslate.obj" \
	"$(INTDIR)\usrwindow.obj" \
	"$(INTDIR)\vhdl.obj" \
	"$(INTDIR)\vhdlals.obj" \
	"$(INTDIR)\vhdlexpr.obj" \
	"$(INTDIR)\vhdlnetlisp.obj" \
	"$(INTDIR)\vhdlparser.obj" \
	"$(INTDIR)\vhdlquisc.obj" \
	"$(INTDIR)\vhdlsemantic.obj" \
	"$(INTDIR)\vhdlsilos.obj" \
	"$(INTDIR)\electric.res"

"$(OUTDIR)\Electric.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\Electric.exe" "$(OUTDIR)\Electric.bsc"


CLEAN :
	-@erase "$(INTDIR)\aidtable.obj"
	-@erase "$(INTDIR)\aidtable.sbr"
	-@erase "$(INTDIR)\compact.obj"
	-@erase "$(INTDIR)\compact.sbr"
	-@erase "$(INTDIR)\compensate.obj"
	-@erase "$(INTDIR)\compensate.sbr"
	-@erase "$(INTDIR)\conlay.obj"
	-@erase "$(INTDIR)\conlay.sbr"
	-@erase "$(INTDIR)\conlin.obj"
	-@erase "$(INTDIR)\conlin.sbr"
	-@erase "$(INTDIR)\conlingtt.obj"
	-@erase "$(INTDIR)\conlingtt.sbr"
	-@erase "$(INTDIR)\conlinprs.obj"
	-@erase "$(INTDIR)\conlinprs.sbr"
	-@erase "$(INTDIR)\conlinttg.obj"
	-@erase "$(INTDIR)\conlinttg.sbr"
	-@erase "$(INTDIR)\contable.obj"
	-@erase "$(INTDIR)\contable.sbr"
	-@erase "$(INTDIR)\data.obj"
	-@erase "$(INTDIR)\data.sbr"
	-@erase "$(INTDIR)\dbchange.obj"
	-@erase "$(INTDIR)\dbchange.sbr"
	-@erase "$(INTDIR)\dbcontour.obj"
	-@erase "$(INTDIR)\dbcontour.sbr"
	-@erase "$(INTDIR)\dbcontrol.obj"
	-@erase "$(INTDIR)\dbcontrol.sbr"
	-@erase "$(INTDIR)\dbcreate.obj"
	-@erase "$(INTDIR)\dbcreate.sbr"
	-@erase "$(INTDIR)\dberror.obj"
	-@erase "$(INTDIR)\dberror.sbr"
	-@erase "$(INTDIR)\dbgeom.obj"
	-@erase "$(INTDIR)\dbgeom.sbr"
	-@erase "$(INTDIR)\dblang.obj"
	-@erase "$(INTDIR)\dblang.sbr"
	-@erase "$(INTDIR)\dblangjava.obj"
	-@erase "$(INTDIR)\dblangjava.sbr"
	-@erase "$(INTDIR)\dblibrary.obj"
	-@erase "$(INTDIR)\dblibrary.sbr"
	-@erase "$(INTDIR)\dbmath.obj"
	-@erase "$(INTDIR)\dbmath.sbr"
	-@erase "$(INTDIR)\dbmemory.obj"
	-@erase "$(INTDIR)\dbmemory.sbr"
	-@erase "$(INTDIR)\dbmerge.obj"
	-@erase "$(INTDIR)\dbmerge.sbr"
	-@erase "$(INTDIR)\dbmult.obj"
	-@erase "$(INTDIR)\dbmult.sbr"
	-@erase "$(INTDIR)\dbnoproto.obj"
	-@erase "$(INTDIR)\dbnoproto.sbr"
	-@erase "$(INTDIR)\dbtech.obj"
	-@erase "$(INTDIR)\dbtech.sbr"
	-@erase "$(INTDIR)\dbtechi.obj"
	-@erase "$(INTDIR)\dbtechi.sbr"
	-@erase "$(INTDIR)\dbtext.obj"
	-@erase "$(INTDIR)\dbtext.sbr"
	-@erase "$(INTDIR)\dbvars.obj"
	-@erase "$(INTDIR)\dbvars.sbr"
	-@erase "$(INTDIR)\drc.obj"
	-@erase "$(INTDIR)\drc.sbr"
	-@erase "$(INTDIR)\drcbatch.obj"
	-@erase "$(INTDIR)\drcbatch.sbr"
	-@erase "$(INTDIR)\drcflat.obj"
	-@erase "$(INTDIR)\drcflat.sbr"
	-@erase "$(INTDIR)\drcquick.obj"
	-@erase "$(INTDIR)\drcquick.sbr"
	-@erase "$(INTDIR)\electric.res"
	-@erase "$(INTDIR)\erc.obj"
	-@erase "$(INTDIR)\erc.sbr"
	-@erase "$(INTDIR)\ercantenna.obj"
	-@erase "$(INTDIR)\ercantenna.sbr"
	-@erase "$(INTDIR)\graphcommon.obj"
	-@erase "$(INTDIR)\graphcommon.sbr"
	-@erase "$(INTDIR)\graphpc.obj"
	-@erase "$(INTDIR)\graphpc.sbr"
	-@erase "$(INTDIR)\graphpcchildframe.obj"
	-@erase "$(INTDIR)\graphpcchildframe.sbr"
	-@erase "$(INTDIR)\graphpccode.obj"
	-@erase "$(INTDIR)\graphpccode.sbr"
	-@erase "$(INTDIR)\graphpcdialog.obj"
	-@erase "$(INTDIR)\graphpcdialog.sbr"
	-@erase "$(INTDIR)\graphpcdialoglistbox.obj"
	-@erase "$(INTDIR)\graphpcdialoglistbox.sbr"
	-@erase "$(INTDIR)\graphpcdoc.obj"
	-@erase "$(INTDIR)\graphpcdoc.sbr"
	-@erase "$(INTDIR)\graphpcmainframe.obj"
	-@erase "$(INTDIR)\graphpcmainframe.sbr"
	-@erase "$(INTDIR)\graphpcmsgview.obj"
	-@erase "$(INTDIR)\graphpcmsgview.sbr"
	-@erase "$(INTDIR)\graphpcstdafx.obj"
	-@erase "$(INTDIR)\graphpcstdafx.sbr"
	-@erase "$(INTDIR)\graphpcview.obj"
	-@erase "$(INTDIR)\graphpcview.sbr"
	-@erase "$(INTDIR)\io.obj"
	-@erase "$(INTDIR)\io.sbr"
	-@erase "$(INTDIR)\iobinaryi.obj"
	-@erase "$(INTDIR)\iobinaryi.sbr"
	-@erase "$(INTDIR)\iobinaryo.obj"
	-@erase "$(INTDIR)\iobinaryo.sbr"
	-@erase "$(INTDIR)\iocifin.obj"
	-@erase "$(INTDIR)\iocifin.sbr"
	-@erase "$(INTDIR)\iocifout.obj"
	-@erase "$(INTDIR)\iocifout.sbr"
	-@erase "$(INTDIR)\iocifpars.obj"
	-@erase "$(INTDIR)\iocifpars.sbr"
	-@erase "$(INTDIR)\iodefi.obj"
	-@erase "$(INTDIR)\iodefi.sbr"
	-@erase "$(INTDIR)\iodxf.obj"
	-@erase "$(INTDIR)\iodxf.sbr"
	-@erase "$(INTDIR)\ioeagleo.obj"
	-@erase "$(INTDIR)\ioeagleo.sbr"
	-@erase "$(INTDIR)\ioecado.obj"
	-@erase "$(INTDIR)\ioecado.sbr"
	-@erase "$(INTDIR)\ioedifi.obj"
	-@erase "$(INTDIR)\ioedifi.sbr"
	-@erase "$(INTDIR)\ioedifo.obj"
	-@erase "$(INTDIR)\ioedifo.sbr"
	-@erase "$(INTDIR)\iogdsi.obj"
	-@erase "$(INTDIR)\iogdsi.sbr"
	-@erase "$(INTDIR)\iogdso.obj"
	-@erase "$(INTDIR)\iogdso.sbr"
	-@erase "$(INTDIR)\iohpglout.obj"
	-@erase "$(INTDIR)\iohpglout.sbr"
	-@erase "$(INTDIR)\iolefi.obj"
	-@erase "$(INTDIR)\iolefi.sbr"
	-@erase "$(INTDIR)\iolefo.obj"
	-@erase "$(INTDIR)\iolefo.sbr"
	-@erase "$(INTDIR)\iolout.obj"
	-@erase "$(INTDIR)\iolout.sbr"
	-@erase "$(INTDIR)\iopadso.obj"
	-@erase "$(INTDIR)\iopadso.sbr"
	-@erase "$(INTDIR)\iopsout.obj"
	-@erase "$(INTDIR)\iopsout.sbr"
	-@erase "$(INTDIR)\iopsoutcolor.obj"
	-@erase "$(INTDIR)\iopsoutcolor.sbr"
	-@erase "$(INTDIR)\ioquickdraw.obj"
	-@erase "$(INTDIR)\ioquickdraw.sbr"
	-@erase "$(INTDIR)\iosdfi.obj"
	-@erase "$(INTDIR)\iosdfi.sbr"
	-@erase "$(INTDIR)\iosuei.obj"
	-@erase "$(INTDIR)\iosuei.sbr"
	-@erase "$(INTDIR)\iotexti.obj"
	-@erase "$(INTDIR)\iotexti.sbr"
	-@erase "$(INTDIR)\iotexto.obj"
	-@erase "$(INTDIR)\iotexto.sbr"
	-@erase "$(INTDIR)\iovhdl.obj"
	-@erase "$(INTDIR)\iovhdl.sbr"
	-@erase "$(INTDIR)\logeffort.obj"
	-@erase "$(INTDIR)\logeffort.sbr"
	-@erase "$(INTDIR)\netdiff.obj"
	-@erase "$(INTDIR)\netdiff.sbr"
	-@erase "$(INTDIR)\netextract.obj"
	-@erase "$(INTDIR)\netextract.sbr"
	-@erase "$(INTDIR)\netflat.obj"
	-@erase "$(INTDIR)\netflat.sbr"
	-@erase "$(INTDIR)\network.obj"
	-@erase "$(INTDIR)\network.sbr"
	-@erase "$(INTDIR)\pla.obj"
	-@erase "$(INTDIR)\pla.sbr"
	-@erase "$(INTDIR)\placdecode.obj"
	-@erase "$(INTDIR)\placdecode.sbr"
	-@erase "$(INTDIR)\placio.obj"
	-@erase "$(INTDIR)\placio.sbr"
	-@erase "$(INTDIR)\placngrid.obj"
	-@erase "$(INTDIR)\placngrid.sbr"
	-@erase "$(INTDIR)\placpgrid.obj"
	-@erase "$(INTDIR)\placpgrid.sbr"
	-@erase "$(INTDIR)\placpla.obj"
	-@erase "$(INTDIR)\placpla.sbr"
	-@erase "$(INTDIR)\placutils.obj"
	-@erase "$(INTDIR)\placutils.sbr"
	-@erase "$(INTDIR)\planfacets.obj"
	-@erase "$(INTDIR)\planfacets.sbr"
	-@erase "$(INTDIR)\planopt.obj"
	-@erase "$(INTDIR)\planopt.sbr"
	-@erase "$(INTDIR)\planprog1.obj"
	-@erase "$(INTDIR)\planprog1.sbr"
	-@erase "$(INTDIR)\planprog2.obj"
	-@erase "$(INTDIR)\planprog2.sbr"
	-@erase "$(INTDIR)\projecttool.obj"
	-@erase "$(INTDIR)\projecttool.sbr"
	-@erase "$(INTDIR)\rout.obj"
	-@erase "$(INTDIR)\rout.sbr"
	-@erase "$(INTDIR)\routauto.obj"
	-@erase "$(INTDIR)\routauto.sbr"
	-@erase "$(INTDIR)\routmaze.obj"
	-@erase "$(INTDIR)\routmaze.sbr"
	-@erase "$(INTDIR)\routmimic.obj"
	-@erase "$(INTDIR)\routmimic.sbr"
	-@erase "$(INTDIR)\routriver.obj"
	-@erase "$(INTDIR)\routriver.sbr"
	-@erase "$(INTDIR)\sc1.obj"
	-@erase "$(INTDIR)\sc1.sbr"
	-@erase "$(INTDIR)\sc1command.obj"
	-@erase "$(INTDIR)\sc1command.sbr"
	-@erase "$(INTDIR)\sc1component.obj"
	-@erase "$(INTDIR)\sc1component.sbr"
	-@erase "$(INTDIR)\sc1connect.obj"
	-@erase "$(INTDIR)\sc1connect.sbr"
	-@erase "$(INTDIR)\sc1delete.obj"
	-@erase "$(INTDIR)\sc1delete.sbr"
	-@erase "$(INTDIR)\sc1electric.obj"
	-@erase "$(INTDIR)\sc1electric.sbr"
	-@erase "$(INTDIR)\sc1err.obj"
	-@erase "$(INTDIR)\sc1err.sbr"
	-@erase "$(INTDIR)\sc1extract.obj"
	-@erase "$(INTDIR)\sc1extract.sbr"
	-@erase "$(INTDIR)\sc1interface.obj"
	-@erase "$(INTDIR)\sc1interface.sbr"
	-@erase "$(INTDIR)\sc1maker.obj"
	-@erase "$(INTDIR)\sc1maker.sbr"
	-@erase "$(INTDIR)\sc1place.obj"
	-@erase "$(INTDIR)\sc1place.sbr"
	-@erase "$(INTDIR)\sc1route.obj"
	-@erase "$(INTDIR)\sc1route.sbr"
	-@erase "$(INTDIR)\sc1sim.obj"
	-@erase "$(INTDIR)\sc1sim.sbr"
	-@erase "$(INTDIR)\sim.obj"
	-@erase "$(INTDIR)\sim.sbr"
	-@erase "$(INTDIR)\simals.obj"
	-@erase "$(INTDIR)\simals.sbr"
	-@erase "$(INTDIR)\simalscom.obj"
	-@erase "$(INTDIR)\simalscom.sbr"
	-@erase "$(INTDIR)\simalsflat.obj"
	-@erase "$(INTDIR)\simalsflat.sbr"
	-@erase "$(INTDIR)\simalsgraph.obj"
	-@erase "$(INTDIR)\simalsgraph.sbr"
	-@erase "$(INTDIR)\simalssim.obj"
	-@erase "$(INTDIR)\simalssim.sbr"
	-@erase "$(INTDIR)\simalsuser.obj"
	-@erase "$(INTDIR)\simalsuser.sbr"
	-@erase "$(INTDIR)\simfasthenry.obj"
	-@erase "$(INTDIR)\simfasthenry.sbr"
	-@erase "$(INTDIR)\simirsim.obj"
	-@erase "$(INTDIR)\simirsim.sbr"
	-@erase "$(INTDIR)\simmaxwell.obj"
	-@erase "$(INTDIR)\simmaxwell.sbr"
	-@erase "$(INTDIR)\simmossim.obj"
	-@erase "$(INTDIR)\simmossim.sbr"
	-@erase "$(INTDIR)\simpal.obj"
	-@erase "$(INTDIR)\simpal.sbr"
	-@erase "$(INTDIR)\simsilos.obj"
	-@erase "$(INTDIR)\simsilos.sbr"
	-@erase "$(INTDIR)\simsim.obj"
	-@erase "$(INTDIR)\simsim.sbr"
	-@erase "$(INTDIR)\simspice.obj"
	-@erase "$(INTDIR)\simspice.sbr"
	-@erase "$(INTDIR)\simspicerun.obj"
	-@erase "$(INTDIR)\simspicerun.sbr"
	-@erase "$(INTDIR)\simtexsim.obj"
	-@erase "$(INTDIR)\simtexsim.sbr"
	-@erase "$(INTDIR)\simverilog.obj"
	-@erase "$(INTDIR)\simverilog.sbr"
	-@erase "$(INTDIR)\simwindow.obj"
	-@erase "$(INTDIR)\simwindow.sbr"
	-@erase "$(INTDIR)\tecart.obj"
	-@erase "$(INTDIR)\tecart.sbr"
	-@erase "$(INTDIR)\tecbicmos.obj"
	-@erase "$(INTDIR)\tecbicmos.sbr"
	-@erase "$(INTDIR)\tecbipolar.obj"
	-@erase "$(INTDIR)\tecbipolar.sbr"
	-@erase "$(INTDIR)\teccmos.obj"
	-@erase "$(INTDIR)\teccmos.sbr"
	-@erase "$(INTDIR)\teccmosdodn.obj"
	-@erase "$(INTDIR)\teccmosdodn.sbr"
	-@erase "$(INTDIR)\tecefido.obj"
	-@erase "$(INTDIR)\tecefido.sbr"
	-@erase "$(INTDIR)\tecfpga.obj"
	-@erase "$(INTDIR)\tecfpga.sbr"
	-@erase "$(INTDIR)\tecgem.obj"
	-@erase "$(INTDIR)\tecgem.sbr"
	-@erase "$(INTDIR)\tecgen.obj"
	-@erase "$(INTDIR)\tecgen.sbr"
	-@erase "$(INTDIR)\tecmocmos.obj"
	-@erase "$(INTDIR)\tecmocmos.sbr"
	-@erase "$(INTDIR)\tecmocmosold.obj"
	-@erase "$(INTDIR)\tecmocmosold.sbr"
	-@erase "$(INTDIR)\tecmocmossub.obj"
	-@erase "$(INTDIR)\tecmocmossub.sbr"
	-@erase "$(INTDIR)\tecnmos.obj"
	-@erase "$(INTDIR)\tecnmos.sbr"
	-@erase "$(INTDIR)\tecpcb.obj"
	-@erase "$(INTDIR)\tecpcb.sbr"
	-@erase "$(INTDIR)\tecrcmos.obj"
	-@erase "$(INTDIR)\tecrcmos.sbr"
	-@erase "$(INTDIR)\tecschem.obj"
	-@erase "$(INTDIR)\tecschem.sbr"
	-@erase "$(INTDIR)\tectable.obj"
	-@erase "$(INTDIR)\tectable.sbr"
	-@erase "$(INTDIR)\usr.obj"
	-@erase "$(INTDIR)\usr.sbr"
	-@erase "$(INTDIR)\usrarc.obj"
	-@erase "$(INTDIR)\usrarc.sbr"
	-@erase "$(INTDIR)\usrcheck.obj"
	-@erase "$(INTDIR)\usrcheck.sbr"
	-@erase "$(INTDIR)\usrcom.obj"
	-@erase "$(INTDIR)\usrcom.sbr"
	-@erase "$(INTDIR)\usrcomab.obj"
	-@erase "$(INTDIR)\usrcomab.sbr"
	-@erase "$(INTDIR)\usrcomcd.obj"
	-@erase "$(INTDIR)\usrcomcd.sbr"
	-@erase "$(INTDIR)\usrcomek.obj"
	-@erase "$(INTDIR)\usrcomek.sbr"
	-@erase "$(INTDIR)\usrcomln.obj"
	-@erase "$(INTDIR)\usrcomln.sbr"
	-@erase "$(INTDIR)\usrcomoq.obj"
	-@erase "$(INTDIR)\usrcomoq.sbr"
	-@erase "$(INTDIR)\usrcomrs.obj"
	-@erase "$(INTDIR)\usrcomrs.sbr"
	-@erase "$(INTDIR)\usrcomtv.obj"
	-@erase "$(INTDIR)\usrcomtv.sbr"
	-@erase "$(INTDIR)\usrcomwz.obj"
	-@erase "$(INTDIR)\usrcomwz.sbr"
	-@erase "$(INTDIR)\usrctech.obj"
	-@erase "$(INTDIR)\usrctech.sbr"
	-@erase "$(INTDIR)\usrdiacom.obj"
	-@erase "$(INTDIR)\usrdiacom.sbr"
	-@erase "$(INTDIR)\usrdiaedit.obj"
	-@erase "$(INTDIR)\usrdiaedit.sbr"
	-@erase "$(INTDIR)\usrdisp.obj"
	-@erase "$(INTDIR)\usrdisp.sbr"
	-@erase "$(INTDIR)\usreditemacs.obj"
	-@erase "$(INTDIR)\usreditemacs.sbr"
	-@erase "$(INTDIR)\usreditpac.obj"
	-@erase "$(INTDIR)\usreditpac.sbr"
	-@erase "$(INTDIR)\usredtecc.obj"
	-@erase "$(INTDIR)\usredtecc.sbr"
	-@erase "$(INTDIR)\usredtecg.obj"
	-@erase "$(INTDIR)\usredtecg.sbr"
	-@erase "$(INTDIR)\usredtecp.obj"
	-@erase "$(INTDIR)\usredtecp.sbr"
	-@erase "$(INTDIR)\usrgraph.obj"
	-@erase "$(INTDIR)\usrgraph.sbr"
	-@erase "$(INTDIR)\usrhigh.obj"
	-@erase "$(INTDIR)\usrhigh.sbr"
	-@erase "$(INTDIR)\usrmenu.obj"
	-@erase "$(INTDIR)\usrmenu.sbr"
	-@erase "$(INTDIR)\usrmisc.obj"
	-@erase "$(INTDIR)\usrmisc.sbr"
	-@erase "$(INTDIR)\usrnet.obj"
	-@erase "$(INTDIR)\usrnet.sbr"
	-@erase "$(INTDIR)\usrparse.obj"
	-@erase "$(INTDIR)\usrparse.sbr"
	-@erase "$(INTDIR)\usrstatus.obj"
	-@erase "$(INTDIR)\usrstatus.sbr"
	-@erase "$(INTDIR)\usrterminal.obj"
	-@erase "$(INTDIR)\usrterminal.sbr"
	-@erase "$(INTDIR)\usrtrack.obj"
	-@erase "$(INTDIR)\usrtrack.sbr"
	-@erase "$(INTDIR)\usrtranslate.obj"
	-@erase "$(INTDIR)\usrtranslate.sbr"
	-@erase "$(INTDIR)\usrwindow.obj"
	-@erase "$(INTDIR)\usrwindow.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vhdl.obj"
	-@erase "$(INTDIR)\vhdl.sbr"
	-@erase "$(INTDIR)\vhdlals.obj"
	-@erase "$(INTDIR)\vhdlals.sbr"
	-@erase "$(INTDIR)\vhdlexpr.obj"
	-@erase "$(INTDIR)\vhdlexpr.sbr"
	-@erase "$(INTDIR)\vhdlnetlisp.obj"
	-@erase "$(INTDIR)\vhdlnetlisp.sbr"
	-@erase "$(INTDIR)\vhdlparser.obj"
	-@erase "$(INTDIR)\vhdlparser.sbr"
	-@erase "$(INTDIR)\vhdlquisc.obj"
	-@erase "$(INTDIR)\vhdlquisc.sbr"
	-@erase "$(INTDIR)\vhdlsemantic.obj"
	-@erase "$(INTDIR)\vhdlsemantic.sbr"
	-@erase "$(INTDIR)\vhdlsilos.obj"
	-@erase "$(INTDIR)\vhdlsilos.sbr"
	-@erase "$(OUTDIR)\Electric.bsc"
	-@erase "$(OUTDIR)\Electric.exe"
	-@erase "$(OUTDIR)\Electric.map"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /GX /Zi /Od /I "src\graph" /I "src\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL" 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\electric.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\Electric.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\conlay.sbr" \
	"$(INTDIR)\conlin.sbr" \
	"$(INTDIR)\conlingtt.sbr" \
	"$(INTDIR)\conlinprs.sbr" \
	"$(INTDIR)\conlinttg.sbr" \
	"$(INTDIR)\contable.sbr" \
	"$(INTDIR)\aidtable.sbr" \
	"$(INTDIR)\data.sbr" \
	"$(INTDIR)\dbchange.sbr" \
	"$(INTDIR)\dbcontour.sbr" \
	"$(INTDIR)\dbcontrol.sbr" \
	"$(INTDIR)\dbcreate.sbr" \
	"$(INTDIR)\dberror.sbr" \
	"$(INTDIR)\dbgeom.sbr" \
	"$(INTDIR)\dblang.sbr" \
	"$(INTDIR)\dblangjava.sbr" \
	"$(INTDIR)\dblibrary.sbr" \
	"$(INTDIR)\dbmath.sbr" \
	"$(INTDIR)\dbmemory.sbr" \
	"$(INTDIR)\dbmerge.sbr" \
	"$(INTDIR)\dbmult.sbr" \
	"$(INTDIR)\dbnoproto.sbr" \
	"$(INTDIR)\dbtech.sbr" \
	"$(INTDIR)\dbtechi.sbr" \
	"$(INTDIR)\dbtext.sbr" \
	"$(INTDIR)\dbvars.sbr" \
	"$(INTDIR)\drc.sbr" \
	"$(INTDIR)\drcbatch.sbr" \
	"$(INTDIR)\drcflat.sbr" \
	"$(INTDIR)\drcquick.sbr" \
	"$(INTDIR)\graphcommon.sbr" \
	"$(INTDIR)\graphpc.sbr" \
	"$(INTDIR)\graphpcchildframe.sbr" \
	"$(INTDIR)\graphpccode.sbr" \
	"$(INTDIR)\graphpcdialog.sbr" \
	"$(INTDIR)\graphpcdialoglistbox.sbr" \
	"$(INTDIR)\graphpcdoc.sbr" \
	"$(INTDIR)\graphpcmainframe.sbr" \
	"$(INTDIR)\graphpcmsgview.sbr" \
	"$(INTDIR)\graphpcstdafx.sbr" \
	"$(INTDIR)\graphpcview.sbr" \
	"$(INTDIR)\io.sbr" \
	"$(INTDIR)\iobinaryi.sbr" \
	"$(INTDIR)\iobinaryo.sbr" \
	"$(INTDIR)\iocifin.sbr" \
	"$(INTDIR)\iocifout.sbr" \
	"$(INTDIR)\iocifpars.sbr" \
	"$(INTDIR)\iodefi.sbr" \
	"$(INTDIR)\iodxf.sbr" \
	"$(INTDIR)\ioeagleo.sbr" \
	"$(INTDIR)\ioecado.sbr" \
	"$(INTDIR)\ioedifi.sbr" \
	"$(INTDIR)\ioedifo.sbr" \
	"$(INTDIR)\iogdsi.sbr" \
	"$(INTDIR)\iogdso.sbr" \
	"$(INTDIR)\iohpglout.sbr" \
	"$(INTDIR)\iolefi.sbr" \
	"$(INTDIR)\iolefo.sbr" \
	"$(INTDIR)\iolout.sbr" \
	"$(INTDIR)\iopadso.sbr" \
	"$(INTDIR)\iopsout.sbr" \
	"$(INTDIR)\iopsoutcolor.sbr" \
	"$(INTDIR)\ioquickdraw.sbr" \
	"$(INTDIR)\iosdfi.sbr" \
	"$(INTDIR)\iosuei.sbr" \
	"$(INTDIR)\iotexti.sbr" \
	"$(INTDIR)\iotexto.sbr" \
	"$(INTDIR)\iovhdl.sbr" \
	"$(INTDIR)\compact.sbr" \
	"$(INTDIR)\compensate.sbr" \
	"$(INTDIR)\erc.sbr" \
	"$(INTDIR)\ercantenna.sbr" \
	"$(INTDIR)\logeffort.sbr" \
	"$(INTDIR)\projecttool.sbr" \
	"$(INTDIR)\netdiff.sbr" \
	"$(INTDIR)\netextract.sbr" \
	"$(INTDIR)\netflat.sbr" \
	"$(INTDIR)\network.sbr" \
	"$(INTDIR)\pla.sbr" \
	"$(INTDIR)\placdecode.sbr" \
	"$(INTDIR)\placio.sbr" \
	"$(INTDIR)\placngrid.sbr" \
	"$(INTDIR)\placpgrid.sbr" \
	"$(INTDIR)\placpla.sbr" \
	"$(INTDIR)\placutils.sbr" \
	"$(INTDIR)\planfacets.sbr" \
	"$(INTDIR)\planopt.sbr" \
	"$(INTDIR)\planprog1.sbr" \
	"$(INTDIR)\planprog2.sbr" \
	"$(INTDIR)\rout.sbr" \
	"$(INTDIR)\routauto.sbr" \
	"$(INTDIR)\routmaze.sbr" \
	"$(INTDIR)\routmimic.sbr" \
	"$(INTDIR)\routriver.sbr" \
	"$(INTDIR)\sc1.sbr" \
	"$(INTDIR)\sc1command.sbr" \
	"$(INTDIR)\sc1component.sbr" \
	"$(INTDIR)\sc1connect.sbr" \
	"$(INTDIR)\sc1delete.sbr" \
	"$(INTDIR)\sc1electric.sbr" \
	"$(INTDIR)\sc1err.sbr" \
	"$(INTDIR)\sc1extract.sbr" \
	"$(INTDIR)\sc1interface.sbr" \
	"$(INTDIR)\sc1maker.sbr" \
	"$(INTDIR)\sc1place.sbr" \
	"$(INTDIR)\sc1route.sbr" \
	"$(INTDIR)\sc1sim.sbr" \
	"$(INTDIR)\sim.sbr" \
	"$(INTDIR)\simals.sbr" \
	"$(INTDIR)\simalscom.sbr" \
	"$(INTDIR)\simalsflat.sbr" \
	"$(INTDIR)\simalsgraph.sbr" \
	"$(INTDIR)\simalssim.sbr" \
	"$(INTDIR)\simalsuser.sbr" \
	"$(INTDIR)\simfasthenry.sbr" \
	"$(INTDIR)\simirsim.sbr" \
	"$(INTDIR)\simmaxwell.sbr" \
	"$(INTDIR)\simmossim.sbr" \
	"$(INTDIR)\simpal.sbr" \
	"$(INTDIR)\simsilos.sbr" \
	"$(INTDIR)\simsim.sbr" \
	"$(INTDIR)\simspice.sbr" \
	"$(INTDIR)\simspicerun.sbr" \
	"$(INTDIR)\simtexsim.sbr" \
	"$(INTDIR)\simverilog.sbr" \
	"$(INTDIR)\simwindow.sbr" \
	"$(INTDIR)\tecart.sbr" \
	"$(INTDIR)\tecbicmos.sbr" \
	"$(INTDIR)\tecbipolar.sbr" \
	"$(INTDIR)\teccmos.sbr" \
	"$(INTDIR)\teccmosdodn.sbr" \
	"$(INTDIR)\tecefido.sbr" \
	"$(INTDIR)\tecfpga.sbr" \
	"$(INTDIR)\tecgem.sbr" \
	"$(INTDIR)\tecgen.sbr" \
	"$(INTDIR)\tecmocmos.sbr" \
	"$(INTDIR)\tecmocmosold.sbr" \
	"$(INTDIR)\tecmocmossub.sbr" \
	"$(INTDIR)\tecnmos.sbr" \
	"$(INTDIR)\tecpcb.sbr" \
	"$(INTDIR)\tecrcmos.sbr" \
	"$(INTDIR)\tecschem.sbr" \
	"$(INTDIR)\tectable.sbr" \
	"$(INTDIR)\usr.sbr" \
	"$(INTDIR)\usrarc.sbr" \
	"$(INTDIR)\usrcheck.sbr" \
	"$(INTDIR)\usrcom.sbr" \
	"$(INTDIR)\usrcomab.sbr" \
	"$(INTDIR)\usrcomcd.sbr" \
	"$(INTDIR)\usrcomek.sbr" \
	"$(INTDIR)\usrcomln.sbr" \
	"$(INTDIR)\usrcomoq.sbr" \
	"$(INTDIR)\usrcomrs.sbr" \
	"$(INTDIR)\usrcomtv.sbr" \
	"$(INTDIR)\usrcomwz.sbr" \
	"$(INTDIR)\usrctech.sbr" \
	"$(INTDIR)\usrdiacom.sbr" \
	"$(INTDIR)\usrdiaedit.sbr" \
	"$(INTDIR)\usrdisp.sbr" \
	"$(INTDIR)\usreditemacs.sbr" \
	"$(INTDIR)\usreditpac.sbr" \
	"$(INTDIR)\usredtecc.sbr" \
	"$(INTDIR)\usredtecg.sbr" \
	"$(INTDIR)\usredtecp.sbr" \
	"$(INTDIR)\usrgraph.sbr" \
	"$(INTDIR)\usrhigh.sbr" \
	"$(INTDIR)\usrmenu.sbr" \
	"$(INTDIR)\usrmisc.sbr" \
	"$(INTDIR)\usrnet.sbr" \
	"$(INTDIR)\usrparse.sbr" \
	"$(INTDIR)\usrstatus.sbr" \
	"$(INTDIR)\usrterminal.sbr" \
	"$(INTDIR)\usrtrack.sbr" \
	"$(INTDIR)\usrtranslate.sbr" \
	"$(INTDIR)\usrwindow.sbr" \
	"$(INTDIR)\vhdl.sbr" \
	"$(INTDIR)\vhdlals.sbr" \
	"$(INTDIR)\vhdlexpr.sbr" \
	"$(INTDIR)\vhdlnetlisp.sbr" \
	"$(INTDIR)\vhdlparser.sbr" \
	"$(INTDIR)\vhdlquisc.sbr" \
	"$(INTDIR)\vhdlsemantic.sbr" \
	"$(INTDIR)\vhdlsilos.sbr"

"$(OUTDIR)\Electric.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=nafxcwd.lib winmm.lib /nologo /subsystem:windows /profile /map:"$(INTDIR)\Electric.map" /debug /machine:I386 /nodefaultlib:"nafxcwd.lib" /out:"$(OUTDIR)\Electric.exe" 
LINK32_OBJS= \
	"$(INTDIR)\conlay.obj" \
	"$(INTDIR)\conlin.obj" \
	"$(INTDIR)\conlingtt.obj" \
	"$(INTDIR)\conlinprs.obj" \
	"$(INTDIR)\conlinttg.obj" \
	"$(INTDIR)\contable.obj" \
	"$(INTDIR)\aidtable.obj" \
	"$(INTDIR)\data.obj" \
	"$(INTDIR)\dbchange.obj" \
	"$(INTDIR)\dbcontour.obj" \
	"$(INTDIR)\dbcontrol.obj" \
	"$(INTDIR)\dbcreate.obj" \
	"$(INTDIR)\dberror.obj" \
	"$(INTDIR)\dbgeom.obj" \
	"$(INTDIR)\dblang.obj" \
	"$(INTDIR)\dblangjava.obj" \
	"$(INTDIR)\dblibrary.obj" \
	"$(INTDIR)\dbmath.obj" \
	"$(INTDIR)\dbmemory.obj" \
	"$(INTDIR)\dbmerge.obj" \
	"$(INTDIR)\dbmult.obj" \
	"$(INTDIR)\dbnoproto.obj" \
	"$(INTDIR)\dbtech.obj" \
	"$(INTDIR)\dbtechi.obj" \
	"$(INTDIR)\dbtext.obj" \
	"$(INTDIR)\dbvars.obj" \
	"$(INTDIR)\drc.obj" \
	"$(INTDIR)\drcbatch.obj" \
	"$(INTDIR)\drcflat.obj" \
	"$(INTDIR)\drcquick.obj" \
	"$(INTDIR)\graphcommon.obj" \
	"$(INTDIR)\graphpc.obj" \
	"$(INTDIR)\graphpcchildframe.obj" \
	"$(INTDIR)\graphpccode.obj" \
	"$(INTDIR)\graphpcdialog.obj" \
	"$(INTDIR)\graphpcdialoglistbox.obj" \
	"$(INTDIR)\graphpcdoc.obj" \
	"$(INTDIR)\graphpcmainframe.obj" \
	"$(INTDIR)\graphpcmsgview.obj" \
	"$(INTDIR)\graphpcstdafx.obj" \
	"$(INTDIR)\graphpcview.obj" \
	"$(INTDIR)\io.obj" \
	"$(INTDIR)\iobinaryi.obj" \
	"$(INTDIR)\iobinaryo.obj" \
	"$(INTDIR)\iocifin.obj" \
	"$(INTDIR)\iocifout.obj" \
	"$(INTDIR)\iocifpars.obj" \
	"$(INTDIR)\iodefi.obj" \
	"$(INTDIR)\iodxf.obj" \
	"$(INTDIR)\ioeagleo.obj" \
	"$(INTDIR)\ioecado.obj" \
	"$(INTDIR)\ioedifi.obj" \
	"$(INTDIR)\ioedifo.obj" \
	"$(INTDIR)\iogdsi.obj" \
	"$(INTDIR)\iogdso.obj" \
	"$(INTDIR)\iohpglout.obj" \
	"$(INTDIR)\iolefi.obj" \
	"$(INTDIR)\iolefo.obj" \
	"$(INTDIR)\iolout.obj" \
	"$(INTDIR)\iopadso.obj" \
	"$(INTDIR)\iopsout.obj" \
	"$(INTDIR)\iopsoutcolor.obj" \
	"$(INTDIR)\ioquickdraw.obj" \
	"$(INTDIR)\iosdfi.obj" \
	"$(INTDIR)\iosuei.obj" \
	"$(INTDIR)\iotexti.obj" \
	"$(INTDIR)\iotexto.obj" \
	"$(INTDIR)\iovhdl.obj" \
	"$(INTDIR)\compact.obj" \
	"$(INTDIR)\compensate.obj" \
	"$(INTDIR)\erc.obj" \
	"$(INTDIR)\ercantenna.obj" \
	"$(INTDIR)\logeffort.obj" \
	"$(INTDIR)\projecttool.obj" \
	"$(INTDIR)\netdiff.obj" \
	"$(INTDIR)\netextract.obj" \
	"$(INTDIR)\netflat.obj" \
	"$(INTDIR)\network.obj" \
	"$(INTDIR)\pla.obj" \
	"$(INTDIR)\placdecode.obj" \
	"$(INTDIR)\placio.obj" \
	"$(INTDIR)\placngrid.obj" \
	"$(INTDIR)\placpgrid.obj" \
	"$(INTDIR)\placpla.obj" \
	"$(INTDIR)\placutils.obj" \
	"$(INTDIR)\planfacets.obj" \
	"$(INTDIR)\planopt.obj" \
	"$(INTDIR)\planprog1.obj" \
	"$(INTDIR)\planprog2.obj" \
	"$(INTDIR)\rout.obj" \
	"$(INTDIR)\routauto.obj" \
	"$(INTDIR)\routmaze.obj" \
	"$(INTDIR)\routmimic.obj" \
	"$(INTDIR)\routriver.obj" \
	"$(INTDIR)\sc1.obj" \
	"$(INTDIR)\sc1command.obj" \
	"$(INTDIR)\sc1component.obj" \
	"$(INTDIR)\sc1connect.obj" \
	"$(INTDIR)\sc1delete.obj" \
	"$(INTDIR)\sc1electric.obj" \
	"$(INTDIR)\sc1err.obj" \
	"$(INTDIR)\sc1extract.obj" \
	"$(INTDIR)\sc1interface.obj" \
	"$(INTDIR)\sc1maker.obj" \
	"$(INTDIR)\sc1place.obj" \
	"$(INTDIR)\sc1route.obj" \
	"$(INTDIR)\sc1sim.obj" \
	"$(INTDIR)\sim.obj" \
	"$(INTDIR)\simals.obj" \
	"$(INTDIR)\simalscom.obj" \
	"$(INTDIR)\simalsflat.obj" \
	"$(INTDIR)\simalsgraph.obj" \
	"$(INTDIR)\simalssim.obj" \
	"$(INTDIR)\simalsuser.obj" \
	"$(INTDIR)\simfasthenry.obj" \
	"$(INTDIR)\simirsim.obj" \
	"$(INTDIR)\simmaxwell.obj" \
	"$(INTDIR)\simmossim.obj" \
	"$(INTDIR)\simpal.obj" \
	"$(INTDIR)\simsilos.obj" \
	"$(INTDIR)\simsim.obj" \
	"$(INTDIR)\simspice.obj" \
	"$(INTDIR)\simspicerun.obj" \
	"$(INTDIR)\simtexsim.obj" \
	"$(INTDIR)\simverilog.obj" \
	"$(INTDIR)\simwindow.obj" \
	"$(INTDIR)\tecart.obj" \
	"$(INTDIR)\tecbicmos.obj" \
	"$(INTDIR)\tecbipolar.obj" \
	"$(INTDIR)\teccmos.obj" \
	"$(INTDIR)\teccmosdodn.obj" \
	"$(INTDIR)\tecefido.obj" \
	"$(INTDIR)\tecfpga.obj" \
	"$(INTDIR)\tecgem.obj" \
	"$(INTDIR)\tecgen.obj" \
	"$(INTDIR)\tecmocmos.obj" \
	"$(INTDIR)\tecmocmosold.obj" \
	"$(INTDIR)\tecmocmossub.obj" \
	"$(INTDIR)\tecnmos.obj" \
	"$(INTDIR)\tecpcb.obj" \
	"$(INTDIR)\tecrcmos.obj" \
	"$(INTDIR)\tecschem.obj" \
	"$(INTDIR)\tectable.obj" \
	"$(INTDIR)\usr.obj" \
	"$(INTDIR)\usrarc.obj" \
	"$(INTDIR)\usrcheck.obj" \
	"$(INTDIR)\usrcom.obj" \
	"$(INTDIR)\usrcomab.obj" \
	"$(INTDIR)\usrcomcd.obj" \
	"$(INTDIR)\usrcomek.obj" \
	"$(INTDIR)\usrcomln.obj" \
	"$(INTDIR)\usrcomoq.obj" \
	"$(INTDIR)\usrcomrs.obj" \
	"$(INTDIR)\usrcomtv.obj" \
	"$(INTDIR)\usrcomwz.obj" \
	"$(INTDIR)\usrctech.obj" \
	"$(INTDIR)\usrdiacom.obj" \
	"$(INTDIR)\usrdiaedit.obj" \
	"$(INTDIR)\usrdisp.obj" \
	"$(INTDIR)\usreditemacs.obj" \
	"$(INTDIR)\usreditpac.obj" \
	"$(INTDIR)\usredtecc.obj" \
	"$(INTDIR)\usredtecg.obj" \
	"$(INTDIR)\usredtecp.obj" \
	"$(INTDIR)\usrgraph.obj" \
	"$(INTDIR)\usrhigh.obj" \
	"$(INTDIR)\usrmenu.obj" \
	"$(INTDIR)\usrmisc.obj" \
	"$(INTDIR)\usrnet.obj" \
	"$(INTDIR)\usrparse.obj" \
	"$(INTDIR)\usrstatus.obj" \
	"$(INTDIR)\usrterminal.obj" \
	"$(INTDIR)\usrtrack.obj" \
	"$(INTDIR)\usrtranslate.obj" \
	"$(INTDIR)\usrwindow.obj" \
	"$(INTDIR)\vhdl.obj" \
	"$(INTDIR)\vhdlals.obj" \
	"$(INTDIR)\vhdlexpr.obj" \
	"$(INTDIR)\vhdlnetlisp.obj" \
	"$(INTDIR)\vhdlparser.obj" \
	"$(INTDIR)\vhdlquisc.obj" \
	"$(INTDIR)\vhdlsemantic.obj" \
	"$(INTDIR)\vhdlsilos.obj" \
	"$(INTDIR)\electric.res"

"$(OUTDIR)\Electric.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("Electric.dep")
!INCLUDE "Electric.dep"
!ELSE 
!MESSAGE Warning: cannot find "Electric.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "Electric - Win32 Release" || "$(CFG)" == "Electric - Win32 Debug"
SOURCE=.\src\cons\conlay.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\conlay.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\conlay.obj"	"$(INTDIR)\conlay.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\cons\conlin.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\conlin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\conlin.obj"	"$(INTDIR)\conlin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\cons\conlingtt.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\conlingtt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\conlingtt.obj"	"$(INTDIR)\conlingtt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\cons\conlinprs.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\conlinprs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\conlinprs.obj"	"$(INTDIR)\conlinprs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\cons\conlinttg.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\conlinttg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\conlinttg.obj"	"$(INTDIR)\conlinttg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\cons\contable.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\contable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\contable.obj"	"$(INTDIR)\contable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\aidtable.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\aidtable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\aidtable.obj"	"$(INTDIR)\aidtable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\data.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\data.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\data.obj"	"$(INTDIR)\data.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbchange.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbchange.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbchange.obj"	"$(INTDIR)\dbchange.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbcontour.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbcontour.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbcontour.obj"	"$(INTDIR)\dbcontour.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbcontrol.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbcontrol.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbcontrol.obj"	"$(INTDIR)\dbcontrol.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbcreate.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbcreate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbcreate.obj"	"$(INTDIR)\dbcreate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dberror.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dberror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dberror.obj"	"$(INTDIR)\dberror.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbgeom.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbgeom.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbgeom.obj"	"$(INTDIR)\dbgeom.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dblang.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dblang.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dblang.obj"	"$(INTDIR)\dblang.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dblangjava.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dblangjava.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dblangjava.obj"	"$(INTDIR)\dblangjava.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dblibrary.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dblibrary.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dblibrary.obj"	"$(INTDIR)\dblibrary.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbmath.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbmath.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbmath.obj"	"$(INTDIR)\dbmath.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbmemory.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbmemory.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbmemory.obj"	"$(INTDIR)\dbmemory.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbmerge.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbmerge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbmerge.obj"	"$(INTDIR)\dbmerge.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbmult.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbmult.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbmult.obj"	"$(INTDIR)\dbmult.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbnoproto.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbnoproto.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbnoproto.obj"	"$(INTDIR)\dbnoproto.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbtech.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbtech.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbtech.obj"	"$(INTDIR)\dbtech.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbtechi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbtechi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbtechi.obj"	"$(INTDIR)\dbtechi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbtext.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbtext.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbtext.obj"	"$(INTDIR)\dbtext.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\db\dbvars.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\dbvars.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\dbvars.obj"	"$(INTDIR)\dbvars.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\drc\drc.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\drc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\drc.obj"	"$(INTDIR)\drc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\drc\drcbatch.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\drcbatch.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\drcbatch.obj"	"$(INTDIR)\drcbatch.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\drc\drcflat.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\drcflat.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\drcflat.obj"	"$(INTDIR)\drcflat.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\drc\drcquick.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\drcquick.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\drcquick.obj"	"$(INTDIR)\drcquick.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\electric.rc

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\electric.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\electric.res" /i "src\graph" /d "NDEBUG" /d "_AFXDLL" $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\electric.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x409 /fo"$(INTDIR)\electric.res" /i "src\graph" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphcommon.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphcommon.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphcommon.obj"	"$(INTDIR)\graphcommon.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpc.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpc.obj"	"$(INTDIR)\graphpc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcchildframe.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcchildframe.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcchildframe.obj"	"$(INTDIR)\graphpcchildframe.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpccode.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpccode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpccode.obj"	"$(INTDIR)\graphpccode.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcdialog.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcdialog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcdialog.obj"	"$(INTDIR)\graphpcdialog.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcdialoglistbox.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcdialoglistbox.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcdialoglistbox.obj"	"$(INTDIR)\graphpcdialoglistbox.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcdoc.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcdoc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcdoc.obj"	"$(INTDIR)\graphpcdoc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcmainframe.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcmainframe.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcmainframe.obj"	"$(INTDIR)\graphpcmainframe.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcmsgview.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcmsgview.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcmsgview.obj"	"$(INTDIR)\graphpcmsgview.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcstdafx.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcstdafx.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcstdafx.obj"	"$(INTDIR)\graphpcstdafx.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\graph\graphpcview.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\graphpcview.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\graphpcview.obj"	"$(INTDIR)\graphpcview.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\io.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\io.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\io.obj"	"$(INTDIR)\io.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iobinaryi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iobinaryi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iobinaryi.obj"	"$(INTDIR)\iobinaryi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iobinaryo.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iobinaryo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iobinaryo.obj"	"$(INTDIR)\iobinaryo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iocifin.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iocifin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iocifin.obj"	"$(INTDIR)\iocifin.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iocifout.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iocifout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iocifout.obj"	"$(INTDIR)\iocifout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iocifpars.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iocifpars.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iocifpars.obj"	"$(INTDIR)\iocifpars.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iodefi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iodefi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iodefi.obj"	"$(INTDIR)\iodefi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iodxf.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iodxf.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iodxf.obj"	"$(INTDIR)\iodxf.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\ioeagleo.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\ioeagleo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\ioeagleo.obj"	"$(INTDIR)\ioeagleo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\ioecado.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\ioecado.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\ioecado.obj"	"$(INTDIR)\ioecado.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\ioedifi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\ioedifi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\ioedifi.obj"	"$(INTDIR)\ioedifi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\ioedifo.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\ioedifo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\ioedifo.obj"	"$(INTDIR)\ioedifo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iogdsi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iogdsi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iogdsi.obj"	"$(INTDIR)\iogdsi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iogdso.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iogdso.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iogdso.obj"	"$(INTDIR)\iogdso.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iohpglout.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iohpglout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iohpglout.obj"	"$(INTDIR)\iohpglout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iolefi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iolefi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iolefi.obj"	"$(INTDIR)\iolefi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iolefo.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iolefo.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iolefo.obj"	"$(INTDIR)\iolefo.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iolout.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iolout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iolout.obj"	"$(INTDIR)\iolout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iopadso.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iopadso.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iopadso.obj"	"$(INTDIR)\iopadso.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iopsout.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iopsout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iopsout.obj"	"$(INTDIR)\iopsout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iopsoutcolor.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iopsoutcolor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iopsoutcolor.obj"	"$(INTDIR)\iopsoutcolor.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\ioquickdraw.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\ioquickdraw.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\ioquickdraw.obj"	"$(INTDIR)\ioquickdraw.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iosdfi.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iosdfi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iosdfi.obj"	"$(INTDIR)\iosdfi.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iosuei.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iosuei.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iosuei.obj"	"$(INTDIR)\iosuei.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iotexti.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iotexti.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iotexti.obj"	"$(INTDIR)\iotexti.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iotexto.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iotexto.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iotexto.obj"	"$(INTDIR)\iotexto.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\io\iovhdl.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\iovhdl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\iovhdl.obj"	"$(INTDIR)\iovhdl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\misc\compact.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\compact.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\compact.obj"	"$(INTDIR)\compact.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\misc\compensate.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\compensate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\compensate.obj"	"$(INTDIR)\compensate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\misc\erc.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\erc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\erc.obj"	"$(INTDIR)\erc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\misc\ercantenna.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\ercantenna.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\ercantenna.obj"	"$(INTDIR)\ercantenna.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\misc\logeffort.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\logeffort.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\logeffort.obj"	"$(INTDIR)\logeffort.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\misc\projecttool.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\projecttool.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\projecttool.obj"	"$(INTDIR)\projecttool.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\net\netdiff.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\netdiff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\netdiff.obj"	"$(INTDIR)\netdiff.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\net\netextract.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\netextract.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\netextract.obj"	"$(INTDIR)\netextract.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\net\netflat.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\netflat.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\netflat.obj"	"$(INTDIR)\netflat.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\net\network.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\network.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\network.obj"	"$(INTDIR)\network.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\pla.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\pla.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\pla.obj"	"$(INTDIR)\pla.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\placdecode.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\placdecode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\placdecode.obj"	"$(INTDIR)\placdecode.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\placio.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\placio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\placio.obj"	"$(INTDIR)\placio.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\placngrid.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\placngrid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\placngrid.obj"	"$(INTDIR)\placngrid.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\placpgrid.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\placpgrid.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\placpgrid.obj"	"$(INTDIR)\placpgrid.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\placpla.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\placpla.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\placpla.obj"	"$(INTDIR)\placpla.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\placutils.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\placutils.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\placutils.obj"	"$(INTDIR)\placutils.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\planfacets.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\planfacets.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\planfacets.obj"	"$(INTDIR)\planfacets.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\planopt.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\planopt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\planopt.obj"	"$(INTDIR)\planopt.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\planprog1.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\planprog1.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\planprog1.obj"	"$(INTDIR)\planprog1.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\pla\planprog2.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\planprog2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\planprog2.obj"	"$(INTDIR)\planprog2.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\rout\rout.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\rout.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\rout.obj"	"$(INTDIR)\rout.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\rout\routauto.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\routauto.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\routauto.obj"	"$(INTDIR)\routauto.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\rout\routmaze.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\routmaze.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\routmaze.obj"	"$(INTDIR)\routmaze.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\rout\routmimic.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\routmimic.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\routmimic.obj"	"$(INTDIR)\routmimic.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\rout\routriver.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\routriver.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\routriver.obj"	"$(INTDIR)\routriver.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1.obj"	"$(INTDIR)\sc1.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1command.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1command.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1command.obj"	"$(INTDIR)\sc1command.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1component.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1component.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1component.obj"	"$(INTDIR)\sc1component.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1connect.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1connect.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1connect.obj"	"$(INTDIR)\sc1connect.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1delete.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1delete.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1delete.obj"	"$(INTDIR)\sc1delete.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1electric.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1electric.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1electric.obj"	"$(INTDIR)\sc1electric.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1err.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1err.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1err.obj"	"$(INTDIR)\sc1err.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1extract.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1extract.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1extract.obj"	"$(INTDIR)\sc1extract.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1interface.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1interface.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1interface.obj"	"$(INTDIR)\sc1interface.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1maker.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1maker.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1maker.obj"	"$(INTDIR)\sc1maker.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1place.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1place.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1place.obj"	"$(INTDIR)\sc1place.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1route.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1route.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1route.obj"	"$(INTDIR)\sc1route.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sc\sc1sim.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sc1sim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sc1sim.obj"	"$(INTDIR)\sc1sim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\sim.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\sim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\sim.obj"	"$(INTDIR)\sim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simals.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simals.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simals.obj"	"$(INTDIR)\simals.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simalscom.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simalscom.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simalscom.obj"	"$(INTDIR)\simalscom.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simalsflat.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simalsflat.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simalsflat.obj"	"$(INTDIR)\simalsflat.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simalsgraph.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simalsgraph.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simalsgraph.obj"	"$(INTDIR)\simalsgraph.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simalssim.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simalssim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simalssim.obj"	"$(INTDIR)\simalssim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simalsuser.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simalsuser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simalsuser.obj"	"$(INTDIR)\simalsuser.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simfasthenry.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simfasthenry.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simfasthenry.obj"	"$(INTDIR)\simfasthenry.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simirsim.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simirsim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simirsim.obj"	"$(INTDIR)\simirsim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simmaxwell.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simmaxwell.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simmaxwell.obj"	"$(INTDIR)\simmaxwell.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simmossim.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simmossim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simmossim.obj"	"$(INTDIR)\simmossim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simpal.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simpal.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simpal.obj"	"$(INTDIR)\simpal.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simsilos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simsilos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simsilos.obj"	"$(INTDIR)\simsilos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simsim.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simsim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simsim.obj"	"$(INTDIR)\simsim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simspice.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simspice.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simspice.obj"	"$(INTDIR)\simspice.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simspicerun.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simspicerun.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simspicerun.obj"	"$(INTDIR)\simspicerun.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simtexsim.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simtexsim.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simtexsim.obj"	"$(INTDIR)\simtexsim.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simverilog.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simverilog.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simverilog.obj"	"$(INTDIR)\simverilog.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\sim\simwindow.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\simwindow.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\simwindow.obj"	"$(INTDIR)\simwindow.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecart.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecart.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecart.obj"	"$(INTDIR)\tecart.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecbicmos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecbicmos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecbicmos.obj"	"$(INTDIR)\tecbicmos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecbipolar.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecbipolar.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecbipolar.obj"	"$(INTDIR)\tecbipolar.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\teccmos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\teccmos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\teccmos.obj"	"$(INTDIR)\teccmos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\teccmosdodn.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\teccmosdodn.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\teccmosdodn.obj"	"$(INTDIR)\teccmosdodn.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecefido.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecefido.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecefido.obj"	"$(INTDIR)\tecefido.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecfpga.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecfpga.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecfpga.obj"	"$(INTDIR)\tecfpga.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecgem.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecgem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecgem.obj"	"$(INTDIR)\tecgem.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecgen.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecgen.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecgen.obj"	"$(INTDIR)\tecgen.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecmocmos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecmocmos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecmocmos.obj"	"$(INTDIR)\tecmocmos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecmocmosold.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecmocmosold.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecmocmosold.obj"	"$(INTDIR)\tecmocmosold.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecmocmossub.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecmocmossub.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecmocmossub.obj"	"$(INTDIR)\tecmocmossub.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecnmos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecnmos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecnmos.obj"	"$(INTDIR)\tecnmos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecpcb.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecpcb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecpcb.obj"	"$(INTDIR)\tecpcb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecrcmos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecrcmos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecrcmos.obj"	"$(INTDIR)\tecrcmos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tecschem.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tecschem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tecschem.obj"	"$(INTDIR)\tecschem.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\tec\tectable.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\tectable.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\tectable.obj"	"$(INTDIR)\tectable.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usr.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usr.obj"	"$(INTDIR)\usr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrarc.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrarc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrarc.obj"	"$(INTDIR)\usrarc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcheck.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcheck.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcheck.obj"	"$(INTDIR)\usrcheck.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcom.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcom.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcom.obj"	"$(INTDIR)\usrcom.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomab.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomab.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomab.obj"	"$(INTDIR)\usrcomab.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomcd.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomcd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomcd.obj"	"$(INTDIR)\usrcomcd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomek.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomek.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomek.obj"	"$(INTDIR)\usrcomek.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomln.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomln.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomln.obj"	"$(INTDIR)\usrcomln.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomoq.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomoq.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomoq.obj"	"$(INTDIR)\usrcomoq.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomrs.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomrs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomrs.obj"	"$(INTDIR)\usrcomrs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomtv.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomtv.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomtv.obj"	"$(INTDIR)\usrcomtv.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrcomwz.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrcomwz.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrcomwz.obj"	"$(INTDIR)\usrcomwz.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrctech.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrctech.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrctech.obj"	"$(INTDIR)\usrctech.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrdiacom.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrdiacom.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrdiacom.obj"	"$(INTDIR)\usrdiacom.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrdiaedit.cpp

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrdiaedit.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrdiaedit.obj"	"$(INTDIR)\usrdiaedit.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrdisp.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrdisp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrdisp.obj"	"$(INTDIR)\usrdisp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usreditemacs.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usreditemacs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usreditemacs.obj"	"$(INTDIR)\usreditemacs.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usreditpac.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usreditpac.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usreditpac.obj"	"$(INTDIR)\usreditpac.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usredtecc.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usredtecc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usredtecc.obj"	"$(INTDIR)\usredtecc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usredtecg.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usredtecg.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usredtecg.obj"	"$(INTDIR)\usredtecg.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usredtecp.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usredtecp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usredtecp.obj"	"$(INTDIR)\usredtecp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrgraph.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrgraph.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrgraph.obj"	"$(INTDIR)\usrgraph.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrhigh.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrhigh.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrhigh.obj"	"$(INTDIR)\usrhigh.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrmenu.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrmenu.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrmenu.obj"	"$(INTDIR)\usrmenu.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrmisc.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrmisc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrmisc.obj"	"$(INTDIR)\usrmisc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrnet.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrnet.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrnet.obj"	"$(INTDIR)\usrnet.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrparse.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrparse.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrparse.obj"	"$(INTDIR)\usrparse.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrstatus.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrstatus.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrstatus.obj"	"$(INTDIR)\usrstatus.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrterminal.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrterminal.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrterminal.obj"	"$(INTDIR)\usrterminal.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrtrack.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrtrack.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrtrack.obj"	"$(INTDIR)\usrtrack.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrtranslate.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrtranslate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrtranslate.obj"	"$(INTDIR)\usrtranslate.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\usr\usrwindow.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\usrwindow.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\usrwindow.obj"	"$(INTDIR)\usrwindow.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdl.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdl.obj"	"$(INTDIR)\vhdl.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlals.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlals.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlals.obj"	"$(INTDIR)\vhdlals.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlexpr.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlexpr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlexpr.obj"	"$(INTDIR)\vhdlexpr.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlnetlisp.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlnetlisp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlnetlisp.obj"	"$(INTDIR)\vhdlnetlisp.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlparser.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlparser.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlparser.obj"	"$(INTDIR)\vhdlparser.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlquisc.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlquisc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlquisc.obj"	"$(INTDIR)\vhdlquisc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlsemantic.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlsemantic.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlsemantic.obj"	"$(INTDIR)\vhdlsemantic.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\vhdl\vhdlsilos.c

!IF  "$(CFG)" == "Electric - Win32 Release"


"$(INTDIR)\vhdlsilos.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"


"$(INTDIR)\vhdlsilos.obj"	"$(INTDIR)\vhdlsilos.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

