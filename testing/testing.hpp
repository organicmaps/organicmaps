#pragma once
#include "testing/testregister.hpp"

#include "base/exception.hpp"
#include "base/math.hpp"
#include "base/src_point.hpp"

#include "std/iostream.hpp"
#include "std/string.hpp"


#define UNIT_TEST(name) \
    void UnitTest_##name();	\
        TestRegister g_TestRegister_##name(#name, __FILE__, &UnitTest_##name); \
    void UnitTest_##name()

DECLARE_EXCEPTION(TestFailureException, RootException);

namespace my
{
  inline void OnTestFailed(SrcPoint const & srcPoint, string const & msg)
  {
    cerr << "FAILED" << endl << srcPoint.FileName() << ":" << srcPoint.Line() << " " << msg << endl;
    MYTHROW(TestFailureException, (srcPoint.FileName(), srcPoint.Line(), msg));
  }
}

#define TEST(X, msg) { if (X) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X")", ::my::impl::Message msg));}}
#define TEST_EQUAL(X, Y, msg) { if ((X) == (Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X" == "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_NOT_EQUAL(X, Y, msg) { if ((X) != (Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X" != "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_LESS(X, Y, msg) { if ((X) < (Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X" < "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_LESS_OR_EQUAL(X, Y, msg) { if ((X) <= (Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X" <= "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_GREATER(X, Y, msg) { if ((X) > (Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X" > "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_GREATER_OR_EQUAL(X, Y, msg) { if ((X) >= (Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST("#X" >= "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_ALMOST_EQUAL(X, Y, msg) { if (::my::AlmostEqual(X, Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST(my::AlmostEqual("#X", "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
#define TEST_NOT_ALMOST_EQUAL(X, Y, msg) { if (!::my::AlmostEqual(X, Y)) {} else { \
  ::my::OnTestFailed(SRC(), ::my::impl::Message("TEST(!my::AlmostEqual("#X", "#Y")", \
                                                 ::my::impl::Message(X, Y), \
                                                 ::my::impl::Message msg));}}
