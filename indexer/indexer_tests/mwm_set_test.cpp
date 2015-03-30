#include "../../testing/testing.hpp"
#include "../mwm_set.hpp"


namespace
{
  class MwmValue : public MwmSet::MwmValueBase
  {
  };

  class TestMwmSet : public MwmSet
  {
  protected:
    virtual bool GetVersion(string const & path, MwmInfo & info,
                            feature::DataHeader::Version & version) const
    {
      int n = path[0] - '0';
      info.m_maxScale = n;
      info.m_limitRect = m2::RectD(0, 0, 1, 1);
      version = feature::DataHeader::lastVersion;
      return true;
    }
    virtual MwmValue * CreateValue(string const &) const
    {
      return new MwmValue();
    }

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

  mwmSet.Register("0");
  mwmSet.Register("1");
  mwmSet.Register("2");
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
    TEST(lock0.GetValue() != NULL, ());
    TEST(lock1.GetValue() == NULL, ());
  }

  mwmSet.Register("3");
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
    TEST(lock.GetValue() != NULL, ());
    mwmSet.Deregister("3");
    mwmSet.Register("4");
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

  mwmSet.Register("5");
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
