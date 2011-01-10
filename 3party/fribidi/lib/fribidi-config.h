/* @configure_input@ */
/* Not copyrighted, in public domain. */
#ifndef FRIBIDI_CONFIG_H
#define FRIBIDI_CONFIG_H

#define FRIBIDI "PACKAGE"
#define FRIBIDI_NAME "PACKAGE_NAME"
#define FRIBIDI_BUGREPORT "PACKAGE_BUGREPORT"

#define FRIBIDI_VERSION "FRIBIDI_VERSION"
#define FRIBIDI_MAJOR_VERSION 1
#define FRIBIDI_MINOR_VERSION 0
#define FRIBIDI_MICRO_VERSION 0
#define FRIBIDI_INTERFACE_VERSION 1
#define FRIBIDI_INTERFACE_VERSION_STRING "FRIBIDI_INTERFACE_VERSION"

/* Define to 1 if you want charset conversion codes in the library */
#define FRIBIDI_CHARSETS 0

/* Define to 1 if you want to use glib */
#define FRIBIDI_USE_GLIB 0

/* The size of a `int', as computed by sizeof. */
//#define FRIBIDI_SIZEOF_INT 4

#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1

#endif /* FRIBIDI_CONFIG_H */
