#include "../../testing/testing.hpp"

#include "../mwm_set.hpp"

#include "../../base/macros.hpp"

namespace
{
  class MwmValue : public MwmSet::MwmValueBase
  {
  };

  class TestMwmSet : public MwmSet
  {
  protected:
    bool GetVersion(string const & path, MwmInfo & info) const override
    {
      int n = path[0] - '0';
      info.m_maxScale = n;
      info.m_limitRect = m2::RectD(0, 0, 1, 1);
      info.m_version.format = version::lastFormat;
      return true;
    }

    MwmValue * CreateValue(string const &) const override { return new MwmValue(); }

  public:
    ~TestMwmSet()
    {
      Cleanup();
    }
  };
}  // unnamed namespace


UNIT_TEST(MwmSetSmokeTest)
{
  TestMwmSet mwmSet;
  vector<MwmInfo> info;

  UNUSED_VALUE(mwmSet.Register("0"));
  UNUSED_VALUE(mwmSet.Register("1"));
  UNUSED_VALUE(mwmSet.Register("2"));
  mwmSet.Deregister("1");
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 3, ());
  TEST(info[0].IsUpToDate(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(!info[1].IsUpToDate(), ());
  TEST(info[2].IsUpToDate(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());
  {
    MwmSet::MwmLock lock0(mwmSet, 0);
    MwmSet::MwmLock lock1(mwmSet, 1);
    TEST(lock0.IsLocked(), ());
    TEST(!lock1.IsLocked(), ());
  }

  UNUSED_VALUE(mwmSet.Register("3"));
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 3, ());
  TEST(info[0].IsUpToDate(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(info[1].IsUpToDate(), ());
  TEST_EQUAL(info[1].m_maxScale, 3, ());
  TEST(info[2].IsUpToDate(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());

  {
    MwmSet::MwmLock lock(mwmSet, 1);
    TEST(lock.IsLocked(), ());
    mwmSet.Deregister("3");
    UNUSED_VALUE(mwmSet.Register("4"));
  }
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 4, ());
  TEST(info[0].IsUpToDate(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(!info[1].IsUpToDate(), ());
  TEST(info[2].IsUpToDate(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());
  TEST(info[3].IsUpToDate(), ());
  TEST_EQUAL(info[3].m_maxScale, 4, ());

  UNUSED_VALUE(mwmSet.Register("5"));
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 4, ());
  TEST(info[0].IsUpToDate(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(info[1].IsUpToDate(), ());
  TEST_EQUAL(info[1].m_maxScale, 5, ());
  TEST(info[2].IsUpToDate(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());
  TEST(info[3].IsUpToDate(), ());
  TEST_EQUAL(info[3].m_maxScale, 4, ());
}
