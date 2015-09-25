#include "testing/testing.hpp"

#include "indexer/data_header.hpp"
#include "indexer/index.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"

#include "std/bind.hpp"
#include "std/string.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace
{
class Observer : public Index::Observer
{
public:
  Observer() : m_numRegisteredCalls(0), m_numDeregisteredCalls(0) {}

  ~Observer() { CheckExpectations(); }

  void ExpectRegisteredMap(platform::LocalCountryFile const & localFile)
  {
    m_expectedRegisteredMaps.push_back(localFile);
  }

  void ExpectDeregisteredMap(platform::LocalCountryFile const & localFile)
  {
    m_expectedDeregisteredMaps.push_back(localFile);
  }

  void CheckExpectations()
  {
    CHECK_EQUAL(m_numRegisteredCalls, m_expectedRegisteredMaps.size(), ());
    CHECK_EQUAL(m_numDeregisteredCalls, m_expectedDeregisteredMaps.size(), ());
  }

  // Index::Observer overrides:
  void OnMapRegistered(platform::LocalCountryFile const & localFile) override
  {
    CHECK_LESS(m_numRegisteredCalls, m_expectedRegisteredMaps.size(),
               ("Unexpected OnMapRegistered() call (", m_numRegisteredCalls, "): ", localFile));
    CHECK_EQUAL(m_expectedRegisteredMaps[m_numRegisteredCalls], localFile, (m_numRegisteredCalls));
    ++m_numRegisteredCalls;
  }

  void OnMapDeregistered(platform::LocalCountryFile const & localFile) override
  {
    CHECK_LESS(m_numDeregisteredCalls, m_expectedDeregisteredMaps.size(),
               ("Unexpected OnMapDeregistered() call (", m_numDeregisteredCalls, "): ", localFile));
    CHECK_EQUAL(m_expectedDeregisteredMaps[m_numDeregisteredCalls], localFile,
                (m_numDeregisteredCalls));
    ++m_numDeregisteredCalls;
  }

  inline size_t MapRegisteredCalls() const { return m_numRegisteredCalls; }
  inline size_t MapDeregisteredCalls() const { return m_numDeregisteredCalls; }

private:
  size_t m_numRegisteredCalls;
  size_t m_numDeregisteredCalls;

  vector<LocalCountryFile> m_expectedRegisteredMaps;
  vector<LocalCountryFile> m_expectedDeregisteredMaps;
};
}  // namespace

UNIT_TEST(Index_Parse)
{
  Index index;
  UNUSED_VALUE(index.RegisterMap(platform::LocalCountryFile::MakeForTesting("minsk-pass")));

  // Make sure that index is actually parsed.
  NoopFunctor fn;
  index.ForEachInScale(fn, 15);
}

UNIT_TEST(Index_MwmStatusNotifications)
{
  Platform & platform = GetPlatform();
  string const mapsDir = platform.WritableDir();
  CountryFile const countryFile("minsk-pass");

  // These two classes point to the same file, but will be considered
  // by Index as distinct files because versions are artificially set
  // to different numbers.
  LocalCountryFile const localFileV1(mapsDir, countryFile, 1 /* version */);
  LocalCountryFile const localFileV2(mapsDir, countryFile, 2 /* version */);

  Index index;
  Observer observer;
  index.AddObserver(observer);

  TEST_EQUAL(0, observer.MapRegisteredCalls(), ());

  MwmSet::MwmId localFileV1Id;

  // Checks that observers are triggered after map registration.
  {
    observer.ExpectRegisteredMap(localFileV1);
    auto p = index.RegisterMap(localFileV1);
    TEST(p.first.IsAlive(), ());
    TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());
    observer.CheckExpectations();
    localFileV1Id = p.first;
  }

  // Checks that map can't registered twice.
  {
    auto p = index.RegisterMap(localFileV1);
    TEST(p.first.IsAlive(), ());
    TEST_EQUAL(MwmSet::RegResult::VersionAlreadyExists, p.second, ());
    observer.CheckExpectations();
    TEST_EQUAL(localFileV1Id, p.first, ());
  }

  // Checks that observers are notified when map is updated.
  MwmSet::MwmId localFileV2Id;
  {
    observer.ExpectRegisteredMap(localFileV2);
    observer.ExpectDeregisteredMap(localFileV1);
    auto p = index.RegisterMap(localFileV2);
    TEST(p.first.IsAlive(), ());
    TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());
    observer.CheckExpectations();
    localFileV2Id = p.first;
    TEST_NOT_EQUAL(localFileV1Id, localFileV2Id, ());
  }

  // Tries to deregister a map in presence of an active lock. Map
  // should be marked "to be removed" but can't be deregistered. After
  // leaving the inner block the map should be deregistered.
  {
    MwmSet::MwmHandle const handle = index.GetMwmHandleByCountryFile(countryFile);
    TEST(handle.IsAlive(), ());

    TEST(!index.DeregisterMap(countryFile), ());
    observer.CheckExpectations();

    observer.ExpectDeregisteredMap(localFileV2);
  }
  observer.CheckExpectations();
  index.RemoveObserver(observer);
}
