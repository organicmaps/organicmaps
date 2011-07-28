include_HEADERS = jansson.h jansson_config.h

lib_LTLIBRARIES = libjansson.la
libjansson_la_SOURCES = \
	dump.c \
	error.c \
	hashtable.c \
	hashtable.h \
	jansson_private.h \
	load.c \
	memory.c \
	pack_unpack.c \
	strbuffer.c \
	strbuffer.h \
	utf.c \
	utf.h \
	value.c
libjansson_la_LDFLAGS = \
	-export-symbols-regex '^json_' \
	-version-info 5:0:1

if GCC
# These flags are gcc specific
AM_CFLAGS = -Wall -Wextra -Wdeclaration-after-statement -Werror
endif
