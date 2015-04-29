#include "testing/testing.hpp"
#include "drape/pointers.hpp"

#include "base/base.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"

namespace
{
  class Tester
  {
  public:
    Tester() = default;
  };

  bool g_assertRaised = false;
  void OnAssertRaised(my::SrcPoint const & srcPoint, string const & msg)
  {
    g_assertRaised = true;
  }
}

UNIT_TEST(PointersTrackingTest)
{
#if defined(TRACK_POINTERS)

  DpPointerTracker::TAlivePointers const & alivePointers = DpPointerTracker::Instance().GetAlivePointers();

  drape_ptr<Tester> ptr = make_unique_dp<Tester>();
  void * ptrAddress = ptr.get();
  string const ptrTypeName = typeid(Tester*).name();

  // no references
  TEST(alivePointers.find(ptrAddress) == alivePointers.end(), ());

  // create a reference
  ref_ptr<Tester> refPtr = make_ref(ptr);

  DpPointerTracker::TAlivePointers::const_iterator found = alivePointers.find(ptrAddress);
  TEST(found != alivePointers.end(), ());
  TEST_EQUAL(found->second.first, 1, ());
  TEST_EQUAL(found->second.second, ptrTypeName, ());

  // copy reference
  ref_ptr<Tester> refPtr2 = refPtr;

  found = alivePointers.find(ptrAddress);
  TEST_EQUAL(found->second.first, 2, ());

  // remove reference
  {
    ref_ptr<Tester> refPtrInScope = refPtr2;
    TEST_EQUAL(found->second.first, 3, ());
  }
  TEST_EQUAL(found->second.first, 2, ());

  // move reference
  ref_ptr<Tester> refPtr3 = move(refPtr2);
  TEST_EQUAL(found->second.first, 2, ());

  // assign reference
  ref_ptr<Tester> refPtr4;
  refPtr4 = refPtr3;
  TEST_EQUAL(found->second.first, 3, ());

  // move-assign reference
  refPtr4 = move(refPtr3);
  TEST_EQUAL(found->second.first, 2, ());

  // create another reference
  ref_ptr<Tester> refPtr5 = make_ref(ptr);
  TEST_EQUAL(found->second.first, 3, ());

#endif
}

UNIT_TEST(RefPointerExpiringTest)
{
#if defined(TRACK_POINTERS)

  g_assertRaised = false;
  my::AssertFailedFn prevFn = my::SetAssertFunction(OnAssertRaised);

  drape_ptr<Tester> ptr = make_unique_dp<Tester>();
  ref_ptr<Tester> refPtr1 = make_ref(ptr);
  ref_ptr<Tester> refPtr2 = make_ref(ptr);
  ptr.reset();

  my::SetAssertFunction(prevFn);

  TEST(g_assertRaised, ());

#endif
}
