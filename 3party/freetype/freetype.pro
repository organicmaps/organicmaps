TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ./include
DEFINES += FT2_BUILD_LIBRARY \
           DARWIN_NO_CARBON \

ROOT_DIR = ../..
DEPENDENCIES =

include($$ROOT_DIR/common.pri)

unix|win32-g++ {
  QMAKE_CFLAGS_WARN_ON += -Wno-uninitialized
}

HEADERS += \
    autofit/afwarp.h \
    autofit/aftypes.h \
    autofit/afpic.h \
    autofit/afmodule.h \
    autofit/afloader.h \
    autofit/aflatin2.h \
    autofit/aflatin.h \
    autofit/afindic.h \
    autofit/afhints.h \
    autofit/afglobal.h \
    autofit/aferrors.h \
    autofit/afdummy.h \
    autofit/afcjk.h \
    autofit/afangles.h \

HEADERS += \
    base/ftbase.h \
    base/basepic.h \

HEADERS += \
    bdf/bdferror.h \
    bdf/bdfdrivr.h \
    bdf/bdf.h \

HEADERS += \
    cache/ftcsbits.h \
    cache/ftcmru.h \
    cache/ftcmanag.h \
    cache/ftcimage.h \
    cache/ftcglyph.h \
    cache/ftcerror.h \
    cache/ftccback.h \
    cache/ftccache.h \

HEADERS += \
    cff/cfftypes.h \
    cff/cfftoken.h \
    cff/cffpic.h \
    cff/cffparse.h \
    cff/cffobjs.h \
    cff/cffload.h \
    cff/cffgload.h \
    cff/cfferrs.h \
    cff/cffdrivr.h \
    cff/cffcmap.h \

HEADERS += \
    cid/cidtoken.h \
    cid/cidriver.h \
    cid/cidparse.h \
    cid/cidobjs.h \
    cid/cidload.h \
    cid/cidgload.h \
    cid/ciderrs.h \

HEADERS +=\
    gxvalid/gxvmorx.h \
    gxvalid/gxvmort.h \
    gxvalid/gxvmod.h \
    gxvalid/gxvfeat.h \
    gxvalid/gxverror.h \
    gxvalid/gxvcommn.h \
    gxvalid/gxvalid.h \

HEADERS += \
    gzip/zutil.h \
    gzip/zlib.h \
    gzip/zconf.h \
    gzip/infutil.h \
    gzip/inftrees.h \
    gzip/inffixed.h \
    gzip/infcodes.h \
    gzip/infblock.h \

HEADERS += \
    custom/ft2build.h\
    custom/ftmodule.h\
    custom/ftoption.h\
    include/freetype/config/ftstdlib.h \
    include/freetype/config/ftoption.h \
    include/freetype/config/ftmodule.h \
    include/freetype/config/ftheader.h \
    include/freetype/config/ftconfig.h \
    include/ft2build.h \
    include/freetype/ttunpat.h \
    include/freetype/tttags.h \
    include/freetype/tttables.h \
    include/freetype/ttnameid.h \
    include/freetype/t1tables.h \
    include/freetype/ftxf86.h \
    include/freetype/ftwinfnt.h \
    include/freetype/fttypes.h \
    include/freetype/fttrigon.h \
    include/freetype/ftsystem.h \
    include/freetype/ftsynth.h \
    include/freetype/ftstroke.h \
    include/freetype/ftsnames.h \
    include/freetype/ftsizes.h \
    include/freetype/ftrender.h \
    include/freetype/ftpfr.h \
    include/freetype/ftoutln.h \
    include/freetype/ftotval.h \
    include/freetype/ftmoderr.h \
    include/freetype/ftmodapi.h \
    include/freetype/ftmm.h \
    include/freetype/ftmac.h \
    include/freetype/ftlzw.h \
    include/freetype/ftlist.h \
    include/freetype/ftlcdfil.h \
    include/freetype/ftincrem.h \
    include/freetype/ftimage.h \
    include/freetype/ftgzip.h \
    include/freetype/ftgxval.h \
    include/freetype/ftglyph.h \
    include/freetype/ftgasp.h \
    include/freetype/fterrors.h \
    include/freetype/fterrdef.h \
    include/freetype/ftcid.h \
    include/freetype/ftchapters.h \
    include/freetype/ftcache.h \
    include/freetype/ftbitmap.h \
    include/freetype/ftbdf.h \
    include/freetype/ftbbox.h \
    include/freetype/ftadvanc.h \
    include/freetype/freetype.h \
    include/freetype/internal/tttypes.h \
    include/freetype/internal/t1types.h \
    include/freetype/internal/sfnt.h \
    include/freetype/internal/pshints.h \
    include/freetype/internal/psaux.h \
    include/freetype/internal/pcftypes.h \
    include/freetype/internal/internal.h \
    include/freetype/internal/ftvalid.h \
    include/freetype/internal/fttrace.h \
    include/freetype/internal/ftstream.h \
    include/freetype/internal/ftserv.h \
    include/freetype/internal/ftrfork.h \
    include/freetype/internal/ftpic.h \
    include/freetype/internal/ftobjs.h \
    include/freetype/internal/ftmemory.h \
    include/freetype/internal/ftgloadr.h \
    include/freetype/internal/ftdriver.h \
    include/freetype/internal/ftdebug.h \
    include/freetype/internal/ftcalc.h \
    include/freetype/internal/autohint.h \
    include/freetype/internal/services/svxf86nm.h \
    include/freetype/internal/services/svwinfnt.h \
    include/freetype/internal/services/svttglyf.h \
    include/freetype/internal/services/svtteng.h \
    include/freetype/internal/services/svttcmap.h \
    include/freetype/internal/services/svsfnt.h \
    include/freetype/internal/services/svpsinfo.h \
    include/freetype/internal/services/svpscmap.h \
    include/freetype/internal/services/svpostnm.h \
    include/freetype/internal/services/svpfr.h \
    include/freetype/internal/services/svotval.h \
    include/freetype/internal/services/svmm.h \
    include/freetype/internal/services/svkern.h \
    include/freetype/internal/services/svgxval.h \
    include/freetype/internal/services/svgldict.h \
    include/freetype/internal/services/svcid.h \
    include/freetype/internal/services/svbdf.h \

HEADERS += \
    lzw/ftzopen.h \

HEADERS += \
    otvalid/otvmod.h \
    otvalid/otvgpos.h \
    otvalid/otverror.h \
    otvalid/otvcommn.h \
    otvalid/otvalid.h \

HEADERS += \
    pcf/pcfutil.h \
    pcf/pcfread.h \
    pcf/pcferror.h \
    pcf/pcfdrivr.h \
    pcf/pcf.h \

HEADERS += \
    pfr/pfrtypes.h \
    pfr/pfrsbit.h \
    pfr/pfrobjs.h \
    pfr/pfrload.h \
    pfr/pfrgload.h \
    pfr/pfrerror.h \
    pfr/pfrdrivr.h \
    pfr/pfrcmap.h \

HEADERS +=\
    psaux/t1decode.h \
    psaux/t1cmap.h \
    psaux/psobjs.h \
    psaux/psconv.h \
    psaux/psauxmod.h \
    psaux/psauxerr.h \
    psaux/afmparse.h \

HEADERS +=\
    pshinter/pshrec.h \
    pshinter/pshpic.h \
    pshinter/pshnterr.h \
    pshinter/pshmod.h \
    pshinter/pshglob.h \
    pshinter/pshalgo.h \

HEADERS +=\
    psnames/pstables.h \
    psnames/pspic.h \
    psnames/psnamerr.h \
    psnames/psmodule.h \

HEADERS +=\
    raster/rastpic.h \
    raster/rasterrs.h \
    raster/ftrend1.h \
    raster/ftraster.h \
    raster/ftmisc.h \

HEADERS +=\
    sfnt/ttsbit.h \
    sfnt/ttpost.h \
    sfnt/ttmtx.h \
    sfnt/ttload.h \
    sfnt/ttkern.h \
    sfnt/ttcmapc.h \
    sfnt/ttcmap.h \
    sfnt/ttbdf.h \
    sfnt/sfobjs.h \
    sfnt/sfntpic.h \
    sfnt/sferrors.h \
    sfnt/sfdriver.h \

HEADERS +=\
    smooth/ftspic.h \
    smooth/ftsmooth.h \
    smooth/ftsmerrs.h \
    smooth/ftgrays.h \

HEADERS +=\
    truetype/ttpload.h \
    truetype/ttpic.h \
    truetype/ttobjs.h \
    truetype/ttinterp.h \
    truetype/ttgxvar.h \
    truetype/ttgload.h \
    truetype/tterrors.h \
    truetype/ttdriver.h \

HEADERS +=\
    type1/t1tokens.h \
    type1/t1parse.h \
    type1/t1objs.h \
    type1/t1load.h \
    type1/t1gload.h \
    type1/t1errors.h \
    type1/t1driver.h \
    type1/t1afm.h \

HEADERS +=\
    type42/t42types.h \
    type42/t42parse.h \
    type42/t42objs.h \
    type42/t42error.h \
    type42/t42drivr.h \

HEADERS +=\
    winfonts/winfnt.h \
    winfonts/fnterrs.h

SOURCES += \
    autofit/autofit.c \
    autofit/afwarp.c \
    autofit/afpic.c \
    autofit/afmodule.c \
    autofit/afloader.c \
    autofit/aflatin2.c \
    autofit/aflatin.c \
    autofit/afindic.c \
    autofit/afhints.c \
    autofit/afglobal.c \
    autofit/afdummy.c \
    autofit/afcjk.c \
    autofit/afangles.c \

SOURCES += \
    base/ftxf86.c \
    base/ftwinfnt.c \
    base/ftutil.c \
    base/fttype1.c \
    base/fttrigon.c \
    base/ftsystem.c \
    base/ftsynth.c \
    base/ftstroke.c \
    base/ftstream.c \
    base/ftsnames.c \
    base/ftrfork.c \
    base/ftpic.c \
    base/ftpfr.c \
    base/ftpatent.c \
    base/ftoutln.c \
    base/ftotval.c \
    base/ftobjs.c \
    base/ftmm.c \
    base/ftmac.c \
    base/ftlcdfil.c \
    base/ftinit.c \
    base/ftgxval.c \
    base/ftglyph.c \
    base/ftgloadr.c \
    base/ftgasp.c \
    base/ftfstype.c \
    base/ftdebug.c \
    base/ftdbgmem.c \
    base/ftcid.c \
    base/ftcalc.c \
    base/ftbitmap.c \
    base/ftbdf.c \
    base/ftbbox.c \
    base/ftbase.c \
    base/ftapi.c \
    base/ftadvanc.c \
    base/basepic.c \

SOURCES += \
    bdf/bdflib.c \
    bdf/bdfdrivr.c \
    bdf/bdf.c \

SOURCES += \
    cache/ftcsbits.c \
    cache/ftcmru.c \
    cache/ftcmanag.c \
    cache/ftcimage.c \
    cache/ftcglyph.c \
    cache/ftccmap.c \
    cache/ftccache.c \
    cache/ftcbasic.c \
    cache/ftcache.c \

SOURCES += \
    cff/cffpic.c \
    cff/cffparse.c \
    cff/cffobjs.c \
    cff/cffload.c \
    cff/cffgload.c \
    cff/cffdrivr.c \
    cff/cffcmap.c \
    cff/cff.c \

SOURCES += \
    cid/cidriver.c \
    cid/cidparse.c \
    cid/cidobjs.c \
    cid/cidload.c \
    cid/cidgload.c \

SOURCES += \
    gxvalid/gxvtrak.c \
    gxvalid/gxvprop.c \
    gxvalid/gxvopbd.c \
    gxvalid/gxvmorx5.c \
    gxvalid/gxvmorx4.c \
    gxvalid/gxvmorx2.c \
    gxvalid/gxvmorx1.c \
    gxvalid/gxvmorx0.c \
    gxvalid/gxvmorx.c \
    gxvalid/gxvmort5.c \
    gxvalid/gxvmort4.c \
    gxvalid/gxvmort2.c \
    gxvalid/gxvmort1.c \
    gxvalid/gxvmort0.c \
    gxvalid/gxvmort.c \
    gxvalid/gxvmod.c \
    gxvalid/gxvlcar.c \
    gxvalid/gxvkern.c \
    gxvalid/gxvjust.c \
    gxvalid/gxvfgen.c \
    gxvalid/gxvfeat.c \
    gxvalid/gxvcommn.c \
    gxvalid/gxvbsln.c \
    gxvalid/gxvalid.c \

SOURCES += \
    gzip/zutil.c \
    gzip/infutil.c \
    gzip/inftrees.c \
    gzip/inflate.c \
    gzip/infcodes.c \
    gzip/infblock.c \
    gzip/ftgzip.c \
    gzip/adler32.c \

SOURCES += \
    lzw/ftzopen.c \
    lzw/ftlzw.c \

SOURCES += \
    otvalid/otvmod.c \
    otvalid/otvmath.c \
    otvalid/otvjstf.c \
    otvalid/otvgsub.c \
    otvalid/otvgpos.c \
    otvalid/otvgdef.c \
    otvalid/otvcommn.c \
    otvalid/otvbase.c \
    otvalid/otvalid.c \

SOURCES += \
    pcf/pcfutil.c \
    pcf/pcfread.c \
    pcf/pcfdrivr.c \
    pcf/pcf.c \

SOURCES += \
    pfr/pfrsbit.c \
    pfr/pfrobjs.c \
    pfr/pfrload.c \
    pfr/pfrgload.c \
    pfr/pfrdrivr.c \
    pfr/pfrcmap.c \
    pfr/pfr.c \

SOURCES += \
    psaux/t1decode.c \
    psaux/t1cmap.c \
    psaux/psobjs.c \
    psaux/psconv.c \
    psaux/psauxmod.c \
    psaux/psaux.c \
    psaux/afmparse.c \

SOURCES += \
    pshinter/pshrec.c \
    pshinter/pshpic.c \
    pshinter/pshmod.c \
    pshinter/pshinter.c \
    pshinter/pshglob.c \
    pshinter/pshalgo.c \

SOURCES += \
    psnames/pspic.c \
    psnames/psnames.c \
    psnames/psmodule.c \

SOURCES += \
    raster/rastpic.c \
    raster/raster.c \
    raster/ftrend1.c \
    raster/ftraster.c \

SOURCES += \
    sfnt/ttsbit0.c \
    sfnt/ttsbit.c \
    sfnt/ttpost.c \
    sfnt/ttmtx.c \
    sfnt/ttload.c \
    sfnt/ttkern.c \
    sfnt/ttcmap.c \
    sfnt/ttbdf.c \
    sfnt/sfobjs.c \
    sfnt/sfntpic.c \
    sfnt/sfnt.c \
    sfnt/sfdriver.c \

SOURCES += \
    smooth/smooth.c \
    smooth/ftspic.c \
    smooth/ftsmooth.c \
    smooth/ftgrays.c \

SOURCES += \
    truetype/ttpload.c \
    truetype/ttpic.c \
    truetype/ttobjs.c \
    truetype/ttinterp.c \
    truetype/ttgxvar.c \
    truetype/ttgload.c \
    truetype/ttdriver.c \
    truetype/truetype.c \

SOURCES += \
    type1/type1.c \
    type1/t1parse.c \
    type1/t1objs.c \
    type1/t1load.c \
    type1/t1gload.c \
    type1/t1driver.c \
    type1/t1afm.c \

SOURCES += \
    type42/type42.c \
    type42/t42parse.c \
    type42/t42objs.c \
    type42/t42drivr.c \

SOURCES += \
    winfonts/winfnt.c
