libfribidi_la_headers = \
		fribidi-arabic.h \
		fribidi-begindecls.h \
		fribidi-bidi.h \
		fribidi-bidi-types.h \
		fribidi-bidi-types-list.h \
		fribidi-common.h \
		fribidi-deprecated.h \
		fribidi-enddecls.h \
		fribidi-flags.h \
		fribidi-joining.h \
		fribidi-joining-types.h \
		fribidi-joining-types-list.h \
		fribidi-mirroring.h \
		fribidi-shape.h \
		fribidi-types.h \
		fribidi-unicode.h \
		fribidi-unicode-version.h \
		fribidi.h
# fribidi.h should be the last entry in the list above.

libfribidi_la_symbols = $(shell cat $(top_srcdir)/lib/fribidi.def)
