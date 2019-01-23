TEMPLATE	= app
CONFIG          += qt thread debug
CONFIG          += irsim lisp java
CONFIG          += sun
unix:QMAKE_CC    = $$QMAKE_CXX
HEADERS		= src/graph/graphqt.h src/graph/graphqtdlg.h
INCLUDEPATH     = src/include
DEFINES         += USEQT USEQUI
LIBS            += -lqui
DEFINES         += FORCETCL=1
LIBS            += -ltcl
#DEFINES         += ETRACE


SOURCES		+= \
    src/cons/conlay.c \
    src/cons/conlin.c \
    src/cons/conlingtt.c \
    src/cons/conlinprs.c \
    src/cons/conlinttg.c \
    src/cons/contable.c
		  
SOURCES += \
    src/db/aidtable.c \
    src/db/data.c \
    src/db/dbchange.c \
    src/db/dbcontour.c \
    src/db/dbcontrol.c \
    src/db/dbcreate.c \
    src/db/dberror.c \
    src/db/dbgeom.c \
    src/db/dblang.c \
    src/db/dblangelk.c \
    src/db/dblangjava.cpp \
    src/db/dblangtcl.c \
    src/db/dblibrary.c \
    src/db/dbmath.c \
    src/db/dbmemory.c \
    src/db/dbmerge.c \
    src/db/dbmult.c \
    src/db/dbnoproto.c \
    src/db/dbtech.c \
    src/db/dbtechi.c \
    src/db/dbtext.c \
    src/db/dbvars.c

SOURCES += \
    src/drc/drc.c \
    src/drc/drcbatch.c \
    src/drc/drcflat.cpp \
    src/drc/drcquick.c

lisp {
DEFINES += FORCELISP=1
INCLUDEPATH += src/elk
SOURCES += \
    src/elk/elkautoload.c \
    src/elk/elkbignum.c \
    src/elk/elkbool.c \
    src/elk/elkchar.c \
    src/elk/elkcont.c \
    src/elk/elkcstring.c \
    src/elk/elkdebug.c \
    src/elk/elkdump.c \
    src/elk/elkenv.c \
    src/elk/elkerror.c \
    src/elk/elkexception.c \
    src/elk/elkfeature.c \
    src/elk/elkheap.c \
    src/elk/elkio.c \
    src/elk/elklist.c \
    src/elk/elkload.c \
    src/elk/elkmain.c \
    src/elk/elkmalloc.c \
    src/elk/elkmath.c \
    src/elk/elkonfork.c \
    src/elk/elkprim.c \
    src/elk/elkprint.c \
    src/elk/elkproc.c \
    src/elk/elkpromise.c \
    src/elk/elkread.c \
    src/elk/elkspecial.c \
    src/elk/elkstab.c \
    src/elk/elkstkmem.c \
    src/elk/elkstring.c \
    src/elk/elksymbol.c \
    src/elk/elkterminate.c \
    src/elk/elktype.c \
    src/elk/elkvector.c
}

java {
DEFINES += FORCEJAVA=1
LANGJAVA_DIR = /usr/java/j2sdk1.4.1_01
INCLUDEPATH += $$LANGJAVA_DIR/include $$LANGJAVA_DIR/include/linux
LIBS += -L$$LANGJAVA_DIR/jre/lib/i386/client -ljvm
}

SOURCES += \
    src/graph/graphcommon.cpp \
    src/graph/graphqt.cpp \
    src/graph/graphqtdlg.cpp \
    src/graph/graphqtdraw.cpp

irsim {
DEFINES += FORCEIRSIMTOOL
SOURCES += \
    src/irsim/irsimanalyzer.c \
    src/irsim/irsimbinsim.c \
    src/irsim/irsimconfig.c \
    src/irsim/irsimconn_list.c \
    src/irsim/irsimeval.c \
    src/irsim/irsimfaultsim.c \
    src/irsim/irsimhist.c \
    src/irsim/irsimhist_io.c \
    src/irsim/irsimincsim.c \
    src/irsim/irsimmem.c \
    src/irsim/irsimnetupdate.c \
    src/irsim/irsimnetwork.c \
    src/irsim/irsimnewrstep.c \
    src/irsim/irsimnsubrs.c \
    src/irsim/irsimparallel.c \
    src/irsim/irsimrsim.c \
    src/irsim/irsimsched.c \
    src/irsim/irsimsim.c \
    src/irsim/irsimsstep.c \
    src/irsim/irsimstack.c \
    src/irsim/irsimsubckt.c \
    src/irsim/irsimtpos.c
}

SOURCES += \
    src/io/io.c \
    src/io/iobinaryi.c \
    src/io/iobinaryo.c \
    src/io/iocifin.c \
    src/io/iocifout.c \
    src/io/iocifpars.c \
    src/io/iodefi.c \
    src/io/iodxf.c \
    src/io/ioecado.c \
    src/io/ioeagleo.c \
    src/io/ioedifi.c \
    src/io/ioedifo.c \
    src/io/iogdsi.c \
    src/io/iogdso.c \
    src/io/iohpglout.c \
    src/io/iolefi.c \
    src/io/iolefo.c \
    src/io/iolout.c \
    src/io/iopadso.c \
    src/io/iopsout.cpp \
    src/io/iopsoutcolor.cpp \
    src/io/iosdfi.c \
    src/io/iosuei.c \
    src/io/iotexti.c \
    src/io/iotexto.c \
    src/io/iovhdl.c

SOURCES += \
    src/misc/compact.c \
    src/misc/compensate.c \
    src/misc/erc.c \
    src/misc/ercantenna.c \
    src/misc/projecttool.c

!sun {
SOURCES += src/misc/logeffort.cpp
}

sun {
DEFINES += FORCESUNTOOLS=1
SOURCES += \
    src/misc/logeffortsun.cpp \
    src/misc/logeffortsun2.cpp \
    src/misc/dbmirrortool.cpp \
    src/net/netanalyze.c \
    src/tec/tecepic7s.c \
    src/tec/tecziptronics.c
}

SOURCES += \
    src/net/netdiff.cpp \
    src/net/netextract.c \
    src/net/netflat.c \
    src/net/network.cpp

SOURCES += \
    src/pla/pla.c \
    src/pla/placdecode.c \
    src/pla/placio.c \
    src/pla/placngrid.c \
    src/pla/placpgrid.c \
    src/pla/placpla.c \
    src/pla/placutils.c \
    src/pla/planfacets.c \
    src/pla/planopt.c \
    src/pla/planprog1.c \
    src/pla/planprog2.c

SOURCES += \
    src/rout/rout.c \
    src/rout/routauto.c \
    src/rout/routmaze.c \
    src/rout/routmimic.c \
    src/rout/routriver.c

SOURCES += \
    src/sc/sc1.c \
    src/sc/sc1command.c \
    src/sc/sc1component.c \
    src/sc/sc1connect.c \
    src/sc/sc1delete.c \
    src/sc/sc1electric.c \
    src/sc/sc1err.c \
    src/sc/sc1extract.c \
    src/sc/sc1interface.c \
    src/sc/sc1maker.c \
    src/sc/sc1place.c \
    src/sc/sc1route.c \
    src/sc/sc1sim.c

SOURCES += \
    src/sim/sim.cpp \
    src/sim/simals.c \
    src/sim/simalscom.c \
    src/sim/simalsflat.c \
    src/sim/simalsgraph.c \
    src/sim/simalssim.c \
    src/sim/simalsuser.c \
    src/sim/simfasthenry.cpp \
    src/sim/simirsim.c \
    src/sim/simmaxwell.c \
    src/sim/simmossim.c \
    src/sim/simpal.c \
    src/sim/simsilos.c \
    src/sim/simsim.cpp \
    src/sim/simspice.cpp \
    src/sim/simspicerun.cpp \
    src/sim/simtexsim.c \
    src/sim/simverilog.c \
    src/sim/simwindow.c

SOURCES += \
    src/tec/tecart.c \
    src/tec/tecbicmos.c \
    src/tec/tecbipolar.c \
    src/tec/teccmos.c \
    src/tec/teccmosdodn.c \
    src/tec/tecefido.c \
    src/tec/tecfpga.c \
    src/tec/tecgem.c \
    src/tec/tecgen.c \
    src/tec/tecmocmos.c \
    src/tec/tecmocmosold.c \
    src/tec/tecmocmossub.c \
    src/tec/tecnmos.c \
    src/tec/tecpcb.c \
    src/tec/tecrcmos.c \
    src/tec/tecschem.c \
    src/tec/tectable.c

SOURCES += \
    src/usr/usr.c \
    src/usr/usrarc.c \
    src/usr/usrcheck.c \
    src/usr/usrcom.c \
    src/usr/usrcomab.c \
    src/usr/usrcomcd.c \
    src/usr/usrcomek.c \
    src/usr/usrcomln.c \
    src/usr/usrcomoq.c \
    src/usr/usrcomrs.c \
    src/usr/usrcomtv.c \
    src/usr/usrcomwz.c \
    src/usr/usrctech.c \
    src/usr/usrdiacom.cpp \
    src/usr/usrdiaedit.cpp \
    src/usr/usrdisp.c \
    src/usr/usreditemacs.c \
    src/usr/usreditpac.c \
    src/usr/usredtecc.c \
    src/usr/usredtecg.c \
    src/usr/usredtecp.c \
    src/usr/usrgraph.c \
    src/usr/usrhigh.c \
    src/usr/usrmenu.c \
    src/usr/usrmisc.c \
    src/usr/usrnet.c \
    src/usr/usrparse.c \
    src/usr/usrstatus.c \
    src/usr/usrterminal.c \
    src/usr/usrtrack.c \
    src/usr/usrtranslate.c \
    src/usr/usrwindow.c

SOURCES += \
    src/vhdl/vhdl.c \
    src/vhdl/vhdlexpr.c \
    src/vhdl/vhdlnetlisp.c \
    src/vhdl/vhdlparser.c \
    src/vhdl/vhdlals.c \
    src/vhdl/vhdlquisc.c \
    src/vhdl/vhdlsemantic.c \
    src/vhdl/vhdlsilos.c

TARGET		= electric
DEPENDPATH=src/include
