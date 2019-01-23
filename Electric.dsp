# Microsoft Developer Studio Project File - Name="Electric" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Electric - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Electric.mak".
!MESSAGE 
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

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Electric - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "src\graph" /I "src\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX"src\graph\graphpcstdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32  winmm.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Electric - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 5
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MTd /W3 /GX /Zi /Od /I "src\graph" /I "src\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o /win32 "NUL"
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 nafxcwd.lib winmm.lib /nologo /subsystem:windows /profile /map /debug /machine:I386 /nodefaultlib:"nafxcwd.lib"

!ENDIF 

# Begin Target

# Name "Electric - Win32 Release"
# Name "Electric - Win32 Debug"
# Begin Group "Constraints"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cons\conlay.c
# End Source File
# Begin Source File

SOURCE=.\src\include\conlay.h
# End Source File
# Begin Source File

SOURCE=.\src\cons\conlin.c
# End Source File
# Begin Source File

SOURCE=.\src\include\conlin.h
# End Source File
# Begin Source File

SOURCE=.\src\cons\conlingtt.c
# End Source File
# Begin Source File

SOURCE=.\src\cons\conlinprs.c
# End Source File
# Begin Source File

SOURCE=.\src\cons\conlinttg.c
# End Source File
# Begin Source File

SOURCE=.\src\cons\contable.c
# End Source File
# End Group
# Begin Group "Database"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\db\aidtable.c
# End Source File
# Begin Source File

SOURCE=.\src\include\config.h
# End Source File
# Begin Source File

SOURCE=.\src\db\data.c
# End Source File
# Begin Source File

SOURCE=.\src\include\database.h
# End Source File
# Begin Source File

SOURCE=.\src\db\dbchange.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbcontour.c
# End Source File
# Begin Source File

SOURCE=.\src\include\dbcontour.h
# End Source File
# Begin Source File

SOURCE=.\src\db\dbcontrol.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbcreate.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dberror.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbgeom.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dblang.c
# End Source File
# Begin Source File

SOURCE=.\src\include\dblang.h
# End Source File
# Begin Source File

SOURCE=.\src\db\dblangjava.cpp
# End Source File
# Begin Source File

SOURCE=.\src\db\dblibrary.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbmath.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbmemory.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbmerge.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbmult.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbnoproto.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbtech.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbtechi.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbtext.c
# End Source File
# Begin Source File

SOURCE=.\src\db\dbvars.c
# End Source File
# Begin Source File

SOURCE=.\src\include\edialogs.h
# End Source File
# Begin Source File

SOURCE=.\src\include\efunction.h
# End Source File
# Begin Source File

SOURCE=.\src\include\egraphics.h
# End Source File
# Begin Source File

SOURCE=.\src\include\global.h
# End Source File
# End Group
# Begin Group "DRC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\drc\drc.c
# End Source File
# Begin Source File

SOURCE=.\src\include\drc.h
# End Source File
# Begin Source File

SOURCE=.\src\drc\drcbatch.c
# End Source File
# Begin Source File

SOURCE=.\src\drc\drcflat.cpp
# End Source File
# Begin Source File

SOURCE=.\src\drc\drcquick.c
# End Source File
# End Group
# Begin Group "Graph"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\graph\electric.rc
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphcommon.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpc.bmp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpc.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpc.ico
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpc.rc2
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcchildframe.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcchildframe.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpccode.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpccursorhand.cur
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpccursormenu.cur
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpccursorpen.cur
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpccursortech.cur
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpccursortty.cur
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdialog.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdialog.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdialoglistbox.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdialoglistbox.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdoc.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdoc.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcdoc.ico
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcmainframe.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcmainframe.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcmsgview.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcmsgview.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcresource.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcstdafx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcstdafx.h
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcview.cpp
# End Source File
# Begin Source File

SOURCE=.\src\graph\graphpcview.h
# End Source File
# End Group
# Begin Group "I/O"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\include\eio.h
# End Source File
# Begin Source File

SOURCE=.\src\io\io.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iobinaryi.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iobinaryo.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iocifin.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iocifout.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iocifpars.c
# End Source File
# Begin Source File

SOURCE=.\src\include\iocifpars.h
# End Source File
# Begin Source File

SOURCE=.\src\io\iodefi.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iodxf.c
# End Source File
# Begin Source File

SOURCE=.\src\io\ioeagleo.c
# End Source File
# Begin Source File

SOURCE=.\src\io\ioecado.c
# End Source File
# Begin Source File

SOURCE=.\src\io\ioedifi.c
# End Source File
# Begin Source File

SOURCE=.\src\io\ioedifo.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iogdsi.c
# End Source File
# Begin Source File

SOURCE=.\src\include\iogdsi.h
# End Source File
# Begin Source File

SOURCE=.\src\io\iogdso.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iohpglout.c
# End Source File
# Begin Source File

SOURCE=.\src\include\iolefdef.h
# End Source File
# Begin Source File

SOURCE=.\src\io\iolefi.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iolefo.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iolout.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iopadso.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iopsout.cpp
# End Source File
# Begin Source File

SOURCE=.\src\io\iopsoutcolor.cpp
# End Source File
# Begin Source File

SOURCE=.\src\io\ioquickdraw.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iosdfi.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iosuei.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iotexti.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iotexto.c
# End Source File
# Begin Source File

SOURCE=.\src\io\iovhdl.c
# End Source File
# End Group
# Begin Group "Misc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\misc\compact.c
# End Source File
# Begin Source File

SOURCE=.\src\include\compact.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\compensate.c
# End Source File
# Begin Source File

SOURCE=.\src\include\compensate.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\erc.c
# End Source File
# Begin Source File

SOURCE=.\src\include\erc.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\ercantenna.c
# End Source File
# Begin Source File

SOURCE=.\src\misc\logeffort.cpp
# End Source File
# Begin Source File

SOURCE=.\src\include\logeffort.h
# End Source File
# Begin Source File

SOURCE=.\src\misc\projecttool.c
# End Source File
# Begin Source File

SOURCE=.\src\include\projecttool.h
# End Source File
# End Group
# Begin Group "Network"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\net\netdiff.cpp
# End Source File
# Begin Source File

SOURCE=.\src\net\netextract.c
# End Source File
# Begin Source File

SOURCE=.\src\include\netextract.h
# End Source File
# Begin Source File

SOURCE=.\src\net\netflat.c
# End Source File
# Begin Source File

SOURCE=.\src\net\network.cpp
# End Source File
# Begin Source File

SOURCE=.\src\include\network.h
# End Source File
# End Group
# Begin Group "PLA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\pla\pla.c
# End Source File
# Begin Source File

SOURCE=.\src\include\pla.h
# End Source File
# Begin Source File

SOURCE=.\src\pla\placdecode.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\placio.c
# End Source File
# Begin Source File

SOURCE=.\src\include\placmos.h
# End Source File
# Begin Source File

SOURCE=.\src\pla\placngrid.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\placpgrid.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\placpla.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\placutils.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\planfacets.c
# End Source File
# Begin Source File

SOURCE=.\src\include\planmos.h
# End Source File
# Begin Source File

SOURCE=.\src\pla\planopt.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\planprog1.c
# End Source File
# Begin Source File

SOURCE=.\src\pla\planprog2.c
# End Source File
# End Group
# Begin Group "Router"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\rout\rout.c
# End Source File
# Begin Source File

SOURCE=.\src\include\rout.h
# End Source File
# Begin Source File

SOURCE=.\src\rout\routauto.c
# End Source File
# Begin Source File

SOURCE=.\src\rout\routmaze.c
# End Source File
# Begin Source File

SOURCE=.\src\rout\routmimic.c
# End Source File
# Begin Source File

SOURCE=.\src\rout\routriver.c
# End Source File
# End Group
# Begin Group "Silicon Compiler"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sc\sc1.c
# End Source File
# Begin Source File

SOURCE=.\src\include\sc1.h
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1command.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1component.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1connect.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1delete.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1electric.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1err.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1extract.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1interface.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1maker.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1place.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1route.c
# End Source File
# Begin Source File

SOURCE=.\src\sc\sc1sim.c
# End Source File
# End Group
# Begin Group "Simulator"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sim\sim.cpp
# End Source File
# Begin Source File

SOURCE=.\src\include\sim.h
# End Source File
# Begin Source File

SOURCE=.\src\sim\simals.c
# End Source File
# Begin Source File

SOURCE=.\src\include\simals.h
# End Source File
# Begin Source File

SOURCE=.\src\sim\simalscom.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simalsflat.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simalsgraph.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simalssim.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simalsuser.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simfasthenry.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sim\simirsim.c
# End Source File
# Begin Source File

SOURCE=.\src\include\simirsim.h
# End Source File
# Begin Source File

SOURCE=.\src\sim\simmaxwell.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simmossim.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simpal.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simsilos.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simsim.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sim\simspice.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sim\simspicerun.cpp
# End Source File
# Begin Source File

SOURCE=.\src\sim\simtexsim.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simverilog.c
# End Source File
# Begin Source File

SOURCE=.\src\sim\simwindow.c
# End Source File
# End Group
# Begin Group "Technologies"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\tec\tecart.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecart.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecbicmos.c
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecbipolar.c
# End Source File
# Begin Source File

SOURCE=.\src\tec\teccmos.c
# End Source File
# Begin Source File

SOURCE=.\src\tec\teccmosdodn.c
# End Source File
# Begin Source File

SOURCE=.\src\include\teccmosdodn.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecefido.c
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecfpga.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecfpga.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecgem.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecgem.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecgen.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecgen.h
# End Source File
# Begin Source File

SOURCE=.\src\include\tech.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecmocmos.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecmocmos.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecmocmosold.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecmocmosold.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecmocmossub.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecmocmossub.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecnmos.c
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecpcb.c
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecrcmos.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecrcmos.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tecschem.c
# End Source File
# Begin Source File

SOURCE=.\src\include\tecschem.h
# End Source File
# Begin Source File

SOURCE=.\src\tec\tectable.c
# End Source File
# End Group
# Begin Group "User Interface"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\usr\usr.c
# End Source File
# Begin Source File

SOURCE=.\src\include\usr.h
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrarc.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcheck.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcom.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomab.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomcd.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomek.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomln.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomoq.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomrs.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomtv.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrcomwz.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrctech.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrdiacom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\include\usrdiacom.h
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrdiaedit.cpp
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrdisp.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usreditemacs.c
# End Source File
# Begin Source File

SOURCE=.\src\include\usreditemacs.h
# End Source File
# Begin Source File

SOURCE=.\src\usr\usreditpac.c
# End Source File
# Begin Source File

SOURCE=.\src\include\usreditpac.h
# End Source File
# Begin Source File

SOURCE=.\src\include\usredtec.h
# End Source File
# Begin Source File

SOURCE=.\src\usr\usredtecc.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usredtecg.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usredtecp.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrgraph.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrhigh.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrmenu.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrmisc.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrnet.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrparse.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrstatus.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrterminal.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrtrack.c
# End Source File
# Begin Source File

SOURCE=.\src\include\usrtrack.h
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrtranslate.c
# End Source File
# Begin Source File

SOURCE=.\src\usr\usrwindow.c
# End Source File
# End Group
# Begin Group "VHDL Compiler"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vhdl\vhdl.c
# End Source File
# Begin Source File

SOURCE=.\src\include\vhdl.h
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlals.c
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlexpr.c
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlnetlisp.c
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlparser.c
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlquisc.c
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlsemantic.c
# End Source File
# Begin Source File

SOURCE=.\src\vhdl\vhdlsilos.c
# End Source File
# End Group
# End Target
# End Project
