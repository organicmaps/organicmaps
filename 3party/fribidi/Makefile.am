##                                               -*- Automake -*-
## Process this file with automake to produce Makefile.in
##############################################################################
## $Id: Makefile.am,v 1.10 2005/08/25 18:35:34 behdad Exp $
## $Author: behdad $
## $Date: 2005/08/25 18:35:34 $
## $Revision: 1.10 $
## $Source: /cvs/fribidi/fribidi2/Makefile.am,v $
##############################################################################

## The order of subdirs is important, don't change without a reason.
SUBDIRS = gen.tab charset lib bin doc test

EXTRA_DIST = bootstrap ChangeLog.old

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = @PACKAGE@.pc

BUILT_SOURCES = lib/fribidi-unicode-version.h
lib/fribidi-unicode-version.h:
	(cd lib && $(MAKE) $(AM_MAKEFLAGS) fribidi-unicode-version.h)

MAINTAINERCLEANFILES = \
		$(srcdir)/INSTALL \
		$(srcdir)/aclocal.m4 \
		$(srcdir)/autoscan.log \
		$(srcdir)/compile \
		$(srcdir)/config.guess \
		$(srcdir)/config.h.in \
		$(srcdir)/config.sub \
		$(srcdir)/configure.scan \
		$(srcdir)/depcomp \
		$(srcdir)/install-sh \
		$(srcdir)/ltmain.sh \
		$(srcdir)/missing \
		$(srcdir)/mkinstalldirs \
		`find "$(srcdir)" -type f -name Makefile.in -print` \
		`find "$(srcdir)" -type f -name "*~" -print` \
		`$(srcdir)/bootstrap --write-configure`
# The last line above is a horrible hack.
# GNU Coding Standards recommends that `make maintainer-clean' should not 
# remove the configure script.
# We instead make configure call bootstrap.

# Indent all C source and header files, using indent(1):

indent: $(srcdir)/.indent.pro
	cd "$(srcdir)" && \
	find . -mindepth 2 -type f -name "*.[ch]" -print | \
	  xargs ./missing --run indent

.PHONY: indent

# Creating ChangeLog from CVS log, using cvs2cl(1):

MAINTAINERCLEANFILES += ChangeLog ChangeLog.bak

EXTRA_DIST += ChangeLog

ChangeLog: $(srcdir)/ChangeLog

$(srcdir)/ChangeLog:
	@if test -d "$(srcdir)/CVS"; then \
	  (cd "$(srcdir)" && \
	  ./missing --run cvs2cl --stdout --utc --FSF -U AUTHORS) > $@.tmp \
	  && echo "For older changes see ChangeLog.old" >> $@.tmp && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo cvs2cl is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A CVS checkout and cvs2cl is required to generate ChangeLog >&2 && \
	  echo A CVS checkout and cvs2cl is required to generate this file >> $@); \
	fi

.PHONY: ChangeLog $(srcdir)/ChangeLog
