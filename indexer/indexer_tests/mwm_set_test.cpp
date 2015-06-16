#include "testing/testing.hpp"

#include "indexer/mwm_set.hpp"

#include "base/macros.hpp"

#include "std/initializer_list.hpp"
#include "std/unordered_map.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace
{
class MwmValue : public MwmSet::MwmValueBase
{
};

class TestMwmSet : public MwmSet
{
protected:
  // MwmSet overrides:
  bool GetVersion(LocalCountryFile const & localFile, MwmInfo & info) const override
  {
    int const n = localFile.GetCountryFile().GetNameWithoutExt()[0] - '0';
    info.m_maxScale = n;
    info.m_limitRect = m2::RectD(0, 0, 1, 1);
    info.m_version.format = version::lastFormat;
    return true;
  }

  TMwmValueBasePtr CreateValue(LocalCountryFile const &) const override
  {
    return TMwmValueBasePtr(new MwmValue());
  }

public:
  ~TestMwmSet() { Cleanup(); }
};

void GetMwmsInfo(MwmSet const & mwmSet, unordered_map<string, shared_ptr<MwmInfo>> & mwmsInfo)
{
  vector<shared_ptr<MwmInfo>> mwmsInfoList;
  mwmSet.GetMwmsInfo(mwmsInfoList);

  mwmsInfo.clear();
  for (shared_ptr<MwmInfo> const & info : mwmsInfoList)
    mwmsInfo[info->GetCountryName()] = info;
}

void TestFilesPresence(unordered_map<string, shared_ptr<MwmInfo>> const & mwmsInfo,
                       initializer_list<string> const & expectedNames)
{
  TEST_EQUAL(expectedNames.size(), mwmsInfo.size(), ());
  for (string const & countryFileName : expectedNames)
    TEST_EQUAL(1, mwmsInfo.count(countryFileName), (countryFileName));
}

}  // namespace

UNIT_TEST(MwmSetSmokeTest)
{
  TestMwmSet mwmSet;
  unordered_map<string, shared_ptr<MwmInfo>> mwmsInfo;

  UNUSED_VALUE(mwmSet.Register(LocalCountryFile::MakeForTesting("0")));
  UNUSED_VALUE(mwmSet.Register(LocalCountryFile::MakeForTesting("1")));
  UNUSED_VALUE(mwmSet.Register(LocalCountryFile::MakeForTesting("2")));
  mwmSet.Deregister(CountryFile("1"));

  GetMwmsInfo(mwmSet, mwmsInfo);
  TestFilesPresence(mwmsInfo, {"0", "2"});

  TEST(mwmsInfo["0"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["0"]->m_maxScale, 0, ());
  TEST(mwmsInfo["2"]->IsUpToDate(), ());
  {
    MwmSet::MwmLock const lock0 = mwmSet.GetMwmLockByCountryFile(CountryFile("0"));
    MwmSet::MwmLock const lock1 = mwmSet.GetMwmLockByCountryFile(CountryFile("1"));
    TEST(lock0.IsLocked(), ());
    TEST(!lock1.IsLocked(), ());
  }

  UNUSED_VALUE(mwmSet.Register(LocalCountryFile::MakeForTesting("3")));

  GetMwmsInfo(mwmSet, mwmsInfo);
  TestFilesPresence(mwmsInfo, {"0", "2", "3"});

  TEST(mwmsInfo["0"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["0"]->m_maxScale, 0, ());
  TEST(mwmsInfo["2"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["2"]->m_maxScale, 2, ());
  TEST(mwmsInfo["3"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["3"]->m_maxScale, 3, ());

  {
    MwmSet::MwmLock const lock1 = mwmSet.GetMwmLockByCountryFile(CountryFile("1"));
    TEST(!lock1.IsLocked(), ());
    mwmSet.Deregister(CountryFile("3"));
    UNUSED_VALUE(mwmSet.Register(LocalCountryFile::MakeForTesting("4")));
  }

  GetMwmsInfo(mwmSet, mwmsInfo);
  TestFilesPresence(mwmsInfo, {"0", "2", "4"});

  TEST(mwmsInfo["0"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["0"]->m_maxScale, 0, ());
  TEST(mwmsInfo["2"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["2"]->m_maxScale, 2, ());
  TEST(mwmsInfo["4"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["4"]->m_maxScale, 4, ());

  UNUSED_VALUE(mwmSet.Register(LocalCountryFile::MakeForTesting("5")));

  GetMwmsInfo(mwmSet, mwmsInfo);
  TestFilesPresence(mwmsInfo, {"0", "2", "4", "5"});

  TEST_EQUAL(mwmsInfo.size(), 4, ());
  TEST(mwmsInfo["0"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["0"]->m_maxScale, 0, ());
  TEST(mwmsInfo["2"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["2"]->m_maxScale, 2, ());
  TEST(mwmsInfo["4"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["4"]->m_maxScale, 4, ());
  TEST(mwmsInfo["5"]->IsUpToDate(), ());
  TEST_EQUAL(mwmsInfo["5"]->m_maxScale, 5, ());
}

UNIT_TEST(MwmSetIdTest)
{
  TestMwmSet mwmSet;
  TEST(mwmSet.Register(LocalCountryFile::MakeForTesting("3")).second, ());

  MwmSet::MwmId const id0 = mwmSet.GetMwmLockByCountryFile(CountryFile("3")).GetId();
  MwmSet::MwmId const id1 = mwmSet.GetMwmLockByCountryFile(CountryFile("3")).GetId();

  TEST(id0.IsAlive(), ());
  TEST(id1.IsAlive(), ());
  TEST_EQUAL(id0.GetInfo().get(), id1.GetInfo().get(), ());
  TEST_EQUAL(MwmInfo::STATUS_REGISTERED, id0.GetInfo()->GetStatus(), ());

  TEST(mwmSet.Deregister(CountryFile("3")), ());

  // Test that both id's are sour now.
  TEST(!id0.IsAlive(), ());
  TEST(!id1.IsAlive(), ());
  TEST_EQUAL(id0.GetInfo().get(), id1.GetInfo().get(), ());
  TEST_EQUAL(MwmInfo::STATUS_DEREGISTERED, id0.GetInfo()->GetStatus(), ());
}

UNIT_TEST(MwmSetLockAndIdTest)
{
  TestMwmSet mwmSet;
  MwmSet::MwmId id;

  {
    pair<MwmSet::MwmLock, bool> const lockFlag =
        mwmSet.Register(LocalCountryFile::MakeForTesting("4"));
    MwmSet::MwmLock const & lock = lockFlag.first;
    bool const success = lockFlag.second;
    TEST(lock.IsLocked(), ());
    TEST(success, ("Can't register test mwm 4"));
    TEST_EQUAL(MwmInfo::STATUS_REGISTERED, lock.GetInfo()->GetStatus(), ());

    TEST(!mwmSet.Deregister(CountryFile("4")), ());  // It's not possible to remove mwm 4 right now.
    TEST(lock.IsLocked(), ());
    TEST_EQUAL(MwmInfo::STATUS_MARKED_TO_DEREGISTER, lock.GetInfo()->GetStatus(), ());
    id = lock.GetId();
    TEST(id.IsAlive(), ());
  }

  TEST(!id.IsAlive(), ());       // Mwm is not alive, so id is sour now.
  TEST(id.GetInfo().get(), ());  // But it's still possible to get an MwmInfo.
  TEST_EQUAL(MwmInfo::STATUS_DEREGISTERED, id.GetInfo()->GetStatus(), ());
  TEST_EQUAL(4, id.GetInfo()->m_maxScale, ());

  // It is not possible to lock mwm 4 because it is already deleted,
  // and it is not possible to get to it's info from mwmSet.
  MwmSet::MwmLock lock = mwmSet.GetMwmLockByCountryFile(CountryFile("4"));
  TEST(!lock.IsLocked(), ());
  TEST(!lock.GetId().IsAlive(), ());
  TEST(!lock.GetId().GetInfo().get(), ());
}
