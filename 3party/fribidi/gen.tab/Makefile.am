EXTRA_PROGRAMS = \
		gen-unicode-version \
		gen-bidi-type-tab \
		gen-joining-type-tab \
		gen-arabic-shaping-tab \
		gen-mirroring-tab

gen_unicode_version_SOURCES = gen-unicode-version.c
gen_bidi_type_tab_SOURCES = gen-bidi-type-tab.c packtab.c packtab.h
gen_joining_type_tab_SOURCES = gen-joining-type-tab.c packtab.c packtab.h
gen_arabic_shaping_tab_SOURCES = gen-arabic-shaping-tab.c
gen_mirroring_tab_SOURCES = gen-mirroring-tab.c packtab.c packtab.h

CLEANFILES = $(EXTRA_PROGRAMS)
DISTCLEANFILES =

AM_CPPFLAGS = \
		-I$(top_builddir)/lib \
		-I$(top_srcdir)/lib \
		-I$(top_srcdir)/charset \
		$(MISC_CFLAGS)

LDADD = $(MISC_LIBS)

VPATH += \
		$(builddir)/unidata \
		$(builddir)/unidata/extracted \
		$(srcdir)/unidata \
		$(srcdir)/unidata/extracted \
		$(top_builddir)/lib


UCD_FILES = \
		unidata/ReadMe.txt \
		unidata/UnicodeData.txt \
		unidata/BidiMirroring.txt \
		unidata/ArabicShaping.txt

EXTRA_DIST = $(UCD_FILES)

COMPRESSION = 2

# generate bidi-type.tab.i

gen_bidi_type_tab = gen-bidi-type-tab$(EXEEXT)

derived_bidi-type.tab.i: \
		DerivedBidiClass.txt \
		fribidi-unicode-version.h \
		$(gen_bidi_type_tab_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(gen_bidi_type_tab)
	./$(gen_bidi_type_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

bidi-type.tab.i: \
		UnicodeData.txt \
		fribidi-unicode-version.h \
		$(gen_bidi_type_tab_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(gen_bidi_type_tab)
	./$(gen_bidi_type_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

DISTCLEANFILES += derived_bidi-type.tab.i

# generate joining-type.tab.i

gen_joining_type_tab = gen-joining-type-tab$(EXEEXT)

joining-type.tab.i: \
		UnicodeData.txt \
		ArabicShaping.txt \
		fribidi-unicode-version.h \
		$(gen_joining_type_tab_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(gen_joining_type_tab)
	./$(gen_joining_type_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate arabic-shaping.tab.i

gen_arabic_shaping_tab = gen-arabic-shaping-tab$(EXEEXT)

arabic-shaping.tab.i: \
		UnicodeData.txt \
		fribidi-unicode-version.h \
		$(gen_arabic_shaping_tab_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(gen_arabic_shaping_tab)
	./$(gen_arabic_shaping_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate mirroring.tab.i

gen_mirroring_tab = gen-mirroring-tab$(EXEEXT)

mirroring.tab.i: \
		BidiMirroring.txt \
		fribidi-unicode-version.h \
		$(gen_mirroring_tab_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(gen_mirroring_tab)
	./$(gen_mirroring_tab) $(COMPRESSION) $^ > $@ \
	|| ($(RM) $@ && false)

# generate fribidi-unicode-version.h

gen_unicode_version = gen-unicode-version$(EXEEXT)

fribidi-unicode-version.h: \
		ReadMe.txt \
		BidiMirroring.txt \
		$(gen_unicode_version_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(gen_unicode_version)
	./$(gen_unicode_version) $^ > $@ \
	|| ($(RM) $@ && false)

# generate all generators:
gen: $(EXTRA_PROGRAMS)

$(top_builddir)/config.h: $(top_srcdir)/config.h.in
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) config.h

$(top_builddir)/lib/fribidi-config.h: $(top_srcdir)/lib/fribidi-config.h.in
	cd $(top_builddir)/lib && $(MAKE) $(AM_MAKEFLAGS) fribidi-config.h

.PHONY: gen

.DELETE_ON_ERROR:

-include $(top_srcdir)/git.mk
