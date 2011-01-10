EXTRA_DIST = fribidi.def

lib_LTLIBRARIES = libfribidi.la

AM_CPPFLAGS = $(MISC_CFLAGS)

libfribidi_la_LDFLAGS = -no-undefined -version-info $(LT_VERSION_INFO) -export-symbols-regex "^fribidi_.*"
libfribidi_la_LIBADD = $(MISC_LIBS)
libfribidi_la_DEPENDENCIES =

if OS_WIN32
libfribidi_la_LDFLAGS += -export-symbols $(srcdir)/fribidi.def
endif # OS_WIN32

if FRIBIDI_CHARSETS

AM_CPPFLAGS += -I$(top_srcdir)/charset
libfribidi_la_LIBADD += $(top_builddir)/charset/libfribidi-char-sets.la
libfribidi_la_DEPENDENCIES += $(top_builddir)/charset/libfribidi-char-sets.la

$(top_builddir)/charset/libfribidi-char-sets.la:
	cd $(top_builddir)/lib && $(MAKE) $(AM_MAKEFLAGS) libfribidi-char-sets.la

endif # FRIBIDI_CHARSETS

include Headers.mk
pkginclude_HEADERS = $(libfribidi_la_headers)
nodist_pkginclude_HEADERS = fribidi-config.h

libfribidi_la_SOURCES =	\
		arabic-misc.tab.i \
		arabic-shaping.tab.i \
		bidi-type.tab.i \
		bidi-types.h \
		common.h \
		debug.h \
		fribidi.c \
		fribidi-arabic.c \
		fribidi-bidi.c \
		fribidi-bidi-types.c \
		fribidi-deprecated.c \
		fribidi-joining.c \
		fribidi-joining-types.c \
		fribidi-mem.c \
		fribidi-mirroring.c \
		fribidi-run.c \
		fribidi-shape.c \
		joining-type.tab.i \
		joining-types.h \
		mem.h \
		mirroring.tab.i \
		run.h

GENERATEDSOURCES = \
		fribidi-unicode-version.h \
		arabic-shaping.tab.i \
		bidi-type.tab.i \
		joining-type.tab.i \
		mirroring.tab.i

BUILT_SOURCES = \
		$(GENERATEDSOURCES) \
		fribidi-config.h

$(GENERATEDSOURCES):
	@(cd $(top_builddir)/gen.tab && \
	$(MAKE) $(AM_MAKEFLAGS) $@) && \
	(test -f $@ || mv $(top_builddir)/gen.tab/$@ .)

# re-generate all built sources:
gen:
	@(cd $(top_builddir)/gen.tab && \
	$(MAKE) $(AM_MAKEFLAGS) $@) && \
	$(RM) $(BUILT_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) $(BUILT_SOURCES)

MAINTAINERCLEANFILES = $(BUILT_SOURCES)

$(top_builddir)/config.h: $(top_srcdir)/config.h.in
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) config.h

.PHONY: gen
