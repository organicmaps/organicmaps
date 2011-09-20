#include "../../testing/testing.hpp"
#include "../mwm_set.hpp"
#include "../../coding/file_container.hpp"
#include "../../platform/platform.hpp"

namespace
{
void SetMwmInfoForTest(string const & path, MwmInfo & info)
{
  int n = path[0] - '0';
  info.m_maxScale = n;
}
FilesContainerR * CreateFileContainerForTest(string const &)
{
  return new FilesContainerR(GetPlatform().WritablePathForFile("minsk-pass.mwm"));
}
}  // unnamed namespace


UNIT_TEST(MwmSetSmokeTest)
{
  MwmSet mwmSet(&SetMwmInfoForTest, &CreateFileContainerForTest);
  vector<MwmInfo> info;

  mwmSet.Add("0");
  mwmSet.Add("1");
  mwmSet.Add("2");
  mwmSet.Remove("1");
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 3, ());
  TEST(info[0].isValid(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(!info[1].isValid(), ());
  TEST(info[2].isValid(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());
  {
    MwmSet::MwmLock lock0(mwmSet, 0);
    MwmSet::MwmLock lock1(mwmSet, 1);
    TEST(lock0.GetFileContainer() != NULL, ());
    TEST(lock1.GetFileContainer() == NULL, ());
  }

  mwmSet.Add("3");
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 3, ());
  TEST(info[0].isValid(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(info[1].isValid(), ());
  TEST_EQUAL(info[1].m_maxScale, 3, ());
  TEST(info[2].isValid(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());

  {
    MwmSet::MwmLock lock(mwmSet, 1);
    TEST(lock.GetFileContainer() != NULL, ());
    mwmSet.Remove("3");
    mwmSet.Add("4");
  }
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 4, ());
  TEST(info[0].isValid(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(!info[1].isValid(), ());
  TEST(info[2].isValid(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());
  TEST(info[3].isValid(), ());
  TEST_EQUAL(info[3].m_maxScale, 4, ());

  mwmSet.Add("5");
  mwmSet.GetMwmInfo(info);
  TEST_EQUAL(info.size(), 4, ());
  TEST(info[0].isValid(), ());
  TEST_EQUAL(info[0].m_maxScale, 0, ());
  TEST(info[1].isValid(), ());
  TEST_EQUAL(info[1].m_maxScale, 5, ());
  TEST(info[2].isValid(), ());
  TEST_EQUAL(info[2].m_maxScale, 2, ());
  TEST(info[3].isValid(), ());
  TEST_EQUAL(info[3].m_maxScale, 4, ());
}
