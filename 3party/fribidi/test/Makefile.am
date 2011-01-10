
TESTS_ENVIRONMENT = /bin/sh

TESTS = run.tests

check_SCRIPTS = run.tests

TEST_DATAS = \
	test_CapRTL_explicit.input \
	test_CapRTL_explicit.reference \
	test_CapRTL_implicit.input \
	test_CapRTL_implicit.reference \
	test_ISO8859-8_hebrew.input \
	test_ISO8859-8_hebrew.reference \
	test_UTF-8_persian.input \
	test_UTF-8_persian.reference \
	test_UTF-8_reordernsm.input \
	test_UTF-8_reordernsm.reference


EXTRA_DIST = \
	$(TESTS) \
	$(TEST_DATAS)

test_%.reference: test_%.input
	testcase="$@" && \
	charset="`echo "$@" | sed 's/_[^_]*$$//;s/.*_//'`" && \
	$(top_builddir)/bin/fribidi --test --charset "$$charset" "$<" > "$@" \
	|| ($(RM) "$@"; false)

references:
	for testcase in "$(srcdir)"/test_*.input; do \
		test="`echo $$testcase \
		  | sed 's/\.input$$/.reference/i; s|.*/||;'`" && \
		$(MAKE) $(AM_MAKEFLAGS) "$(srcdir)/$$test" || \
		exit 1; \
	done

