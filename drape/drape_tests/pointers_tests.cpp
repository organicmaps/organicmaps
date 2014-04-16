#include "../../testing/testing.hpp"

#include "../../drape/pointers.hpp"

#include "../../base/assert.hpp"

#include "../../std/string.hpp"
#include "../../std/shared_ptr.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::MatcherInterface;
using ::testing::MatchResultListener;
using ::testing::Matcher;

using my::SrcPoint;

namespace
{

template<typename T>
void EmptyFunction(RefPointer<T> p){}

template<typename T, typename ToDo>
void EmptyFunction(TransferPointer<T> p, ToDo & toDo)
{
  toDo(p);
}

template<typename T>
void PointerDeleter(TransferPointer<T> p)
{
  p.Destroy();
}

#ifdef DEBUG
class MockAssertExpector
{
public:
  MOCK_CONST_METHOD2(Assert, void(SrcPoint const &, string const &));
};

MockAssertExpector * g_asserter;
my::AssertFailedFn m_defaultAssertFn;
void MyOnAssertFailed(SrcPoint const & src, string const & msg)
{
  g_asserter->Assert(src, msg);
}

MockAssertExpector * InitAsserter()
{
  if (g_asserter != NULL)
    delete g_asserter;

  g_asserter = new MockAssertExpector();

  m_defaultAssertFn = my::SetAssertFunction(&MyOnAssertFailed);

  return g_asserter;
}

void DestroyAsserter()
{
  my::SetAssertFunction(m_defaultAssertFn);
  delete g_asserter;
  g_asserter = NULL;
}

SrcPoint ConstructSrcPoint(char const * fileName, char const * function)
{
#ifndef __OBJC__
  return SrcPoint(fileName, 0, function, "()");
#else
  return SrcPoint(fileName, 0, function);
#endif
}
#endif

} // namespace

#ifdef DEBUG

class SrcPointMatcher : public MatcherInterface<SrcPoint const &>
{
public:
  SrcPointMatcher(char const * fileName, char const * function)
    : m_srcPoint(ConstructSrcPoint(fileName, function))
  {
  }

  virtual bool MatchAndExplain(SrcPoint const & x, MatchResultListener * listener) const
  {
    bool res = strcmp(x.FileName(), m_srcPoint.FileName()) == 0 &&
               strcmp(x.Function(), m_srcPoint.Function()) == 0;

    if (res == false)
      (*listener) << "Actual parameter : " << DebugPrint(x);

    return res;
  }

  virtual void DescribeTo(::std::ostream * os) const
  {
    *os << "Expected assert : " << DebugPrint(m_srcPoint);
  }

  virtual void DescribeNegationTo(::std::ostream * os) const
  {
    *os << "Unexpected assert. Expect this : " << DebugPrint(m_srcPoint);
  }

private:
  SrcPoint m_srcPoint;
};

inline Matcher<SrcPoint const &> SrcPointEq(char const * fileName, char const * function)
{
  return ::testing::MakeMatcher(new SrcPointMatcher(fileName, function));
}

#endif

UNIT_TEST(RefPointer_Positive)
{
  MasterPointer<int> p(new int);
  RefPointer<int> refP = p.GetRefPointer();

  TEST_EQUAL(p.IsNull(), false, ());
  TEST_EQUAL(p.GetRaw(), refP.GetRaw(), ());

  RefPointer<int> refP2(refP);

  TEST_EQUAL(p.IsNull(), false, ());
  TEST_EQUAL(p.GetRaw(), refP.GetRaw(), ());
  TEST_EQUAL(refP2.GetRaw(), refP.GetRaw(), ());

  *refP.GetRaw() = 10;

  EmptyFunction(refP);
  TEST_EQUAL(p.IsNull(), false, ());
  TEST_EQUAL(p.GetRaw(), refP.GetRaw(), ());
  TEST_EQUAL(refP2.GetRaw(), refP.GetRaw(), ());

  EmptyFunction(refP2);
  TEST_EQUAL(p.IsNull(), false, ());
  TEST_EQUAL(p.GetRaw(), refP.GetRaw(), ());
  TEST_EQUAL(refP2.GetRaw(), refP.GetRaw(), ());

  refP2 = refP = RefPointer<int>();

  TEST_EQUAL(p.IsNull(), false, ());
  TEST_EQUAL(refP.IsNull(), true, ());
  TEST_EQUAL(refP2.GetRaw(), refP.GetRaw(), ());

  p.Destroy();
  TEST_EQUAL(p.IsNull(), true, ());
}

#ifdef DEBUG
  UNIT_TEST(MasterPointerDestroy_Negative)
  {
    MockAssertExpector * asserter = InitAsserter();

    EXPECT_CALL(*asserter, Assert(SrcPointEq("drape/pointers.cpp", "Destroy"), _)).Times(1);

    MasterPointer<int> p(new int);
    RefPointer<int> refP(p.GetRefPointer());
    p.Destroy();

    DestroyAsserter();
  }

  UNIT_TEST(MasterPointer_Leak)
  {
    MockAssertExpector * asserter = InitAsserter();

    ::testing::InSequence s;

    EXPECT_CALL(*asserter, Assert(SrcPointEq("drape/pointers.cpp", "Deref"), _));

    {
      MasterPointer<int> p(new int);
    }

    DestroyAsserter();
  }
#endif

UNIT_TEST(TransferPointerConvertion_Positive)
{
  MasterPointer<int> p(new int);
  TEST_EQUAL(p.IsNull(), false, ());

  int * rawP = p.GetRaw();

  TransferPointer<int> trP = p.Move();
  TEST_EQUAL(p.IsNull(), true, ());
  TEST_EQUAL(trP.IsNull(), false, ());

  MasterPointer<int> p2(trP);
  TEST_EQUAL(p.IsNull(), true, ());
  TEST_EQUAL(trP.IsNull(), true, ());
  TEST_EQUAL(p2.GetRaw(), rawP, ());

  p2.Destroy();
}

UNIT_TEST(TransferPointer_Positive)
{
  MasterPointer<int> p(new int);
  TEST_EQUAL(p.IsNull(), false, ());

  TransferPointer<int> trP = p.Move();
  TEST_EQUAL(p.IsNull(), true, ());
  TEST_EQUAL(trP.IsNull(), false, ());
  PointerDeleter(trP);
}

namespace
{

template <typename T>
struct MasterPointerAccum
{
public:
  MasterPointer<T> m_p;

  void operator() (TransferPointer<T> p)
  {
    TEST_EQUAL(p.IsNull(), false, ());
    m_p = MasterPointer<T>(p);
    TEST_EQUAL(p.IsNull(), true, ());
  }
};

} // namespace

UNIT_TEST(TransferPointerConvertion2_Positive)
{
  MasterPointer<int> p(new int);
  TEST_EQUAL(p.IsNull(), false, ());

  int * rawP = p.GetRaw();

  MasterPointerAccum<int> accum;
  EmptyFunction(p.Move(), accum);
  TEST_EQUAL(p.IsNull(), true, ());
  TEST_EQUAL(accum.m_p.GetRaw(), rawP, ());

  accum.m_p.Destroy();
}

#ifdef DEBUG

namespace
{

template <typename T>
struct EmptyTransferAccepter
{
public:
  void operator() (TransferPointer<T> p)
  {
    TEST_EQUAL(p.IsNull(), false, ());
    TransferPointer<T> p2(p);
    TEST_EQUAL(p.IsNull(), true, ());
    TEST_EQUAL(p2.IsNull(), false, ());
  }
};

}

UNIT_TEST(TransferPointer_Leak)
{
  MockAssertExpector * asserter = InitAsserter();
  MasterPointer<int> p(new int);
  TEST_EQUAL(p.IsNull(), false, ());

  EXPECT_CALL(*asserter, Assert(SrcPointEq("drape/pointers.hpp", "~TransferPointer"), _));

  EmptyTransferAccepter<int> toDo;
  EmptyFunction(p.Move(), toDo);
  TEST_EQUAL(p.IsNull(), true, ());

  DestroyAsserter();
}

UNIT_TEST(PointerConversionAndLeak)
{
  MockAssertExpector * asserter = InitAsserter();

  EXPECT_CALL(*asserter, Assert(SrcPointEq("drape/pointers.cpp", "Deref"), _));

  {
    MasterPointer<int> p(new int);
    TEST_EQUAL(p.IsNull(), false, ());

    int * rawP = p.GetRaw();

    MasterPointerAccum<int> accum;
    EmptyFunction(p.Move(), accum);
    TEST_EQUAL(p.IsNull(), true, ());
    TEST_EQUAL(accum.m_p.GetRaw(), rawP, ());
  }

  DestroyAsserter();
}

#endif

class Base
{
public:
  Base() {}
  virtual ~Base() {}
};

class Derived : public Base
{
public:
  Derived() {}
};

UNIT_TEST(RefPointerCast)
{
  MasterPointer<Derived> p(new Derived());
  TEST_EQUAL(p.IsNull(), false, ());

  {
    RefPointer<Derived> refP(p.GetRefPointer());
    TEST_EQUAL(p.IsNull(), false, ());
    TEST_EQUAL(p.GetRaw(), refP.GetRaw(), ());

    RefPointer<Base> refBaseP(refP);
    TEST_EQUAL(p.IsNull(), false, ());
    TEST_EQUAL(p.GetRaw(), refP.GetRaw(), ());
    TEST_EQUAL(p.GetRaw(), refBaseP.GetRaw(), ());
  }

  p.Destroy();
}

UNIT_TEST(StackPointers)
{
  int data[10];
  {
    RefPointer<int> p = MakeStackRefPointer(data);
    TEST_EQUAL(p.GetRaw(), data, ());
  }
}

UNIT_TEST(MoveFreePointers)
{
  int * data = new int;
  TransferPointer<int> p = MovePointer(data);
  TEST_EQUAL(p.IsNull(), false, ());

  MasterPointer<int> mP(p);
  TEST_EQUAL(p.IsNull(), true, ());
  TEST_EQUAL(mP.GetRaw(), data, ());

  mP.Destroy();
}
