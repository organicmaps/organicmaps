##                                               -*- Automake -*-
## Process this file with automake to produce Makefile.in
##############################################################################
## $Id: Makefile.am,v 1.10 2005-08-25 18:35:34 behdad Exp $
## $Author: behdad $
## $Date: 2005-08-25 18:35:34 $
## $Revision: 1.10 $
## $Source: /home/behdad/src/fdo/fribidi/togit/git/../fribidi/fribidi2/Makefile.am,v $
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
		$(srcdir)/ChangeLog \
		`find "$(srcdir)" -type f -name Makefile.in -print` \
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

#
# ChangeLog generation
#
CHANGELOG_RANGE =
ChangeLog:
	$(AM_V_GEN) if test -d "$(top_srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git $(top_srcdir)/missing --run \
	   git log $(CHANGELOG_RANGE) --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp "$(srcdir)/ChangeLog" \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> "$(srcdir)/$@")); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> "$(srcdir)/$@"); \
	fi
.PHONY: $(srcdir)/ChangeLog

#
# Release engineering
#

TAR_OPTIONS = --owner=0 --group=0
dist-hook: dist-clear-sticky-bits
# Clean up any sticky bits we may inherit from parent dir
dist-clear-sticky-bits:
	chmod -R a-s $(distdir)


tar_file = $(PACKAGE_TARNAME)-$(VERSION).tar.bz2
sha256_file = $(tar_file).sha256
gpg_file = $(sha256_file).asc
$(sha256_file): $(tar_file)
	sha256sum $^ > $@
$(gpg_file): $(sha256_file)
	@echo "Please enter your GPG password to sign the checksum."
	gpg --armor --sign $^

release-files: $(tar_file) $(sha256_file) $(gpg_file)


-include $(top_srcdir)/git.mk
