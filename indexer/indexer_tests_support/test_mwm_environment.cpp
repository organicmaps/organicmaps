#include "indexer/indexer_tests_support/test_mwm_environment.hpp"

#include <indexer/classificator_loader.hpp>

namespace indexer
{
namespace tests_support
{
TestMwmEnvironment::TestMwmEnvironment() { classificator::Load(); }

TestMwmEnvironment::~TestMwmEnvironment()
{
  for (auto const & file : m_mwmFiles)
    Cleanup(file);
}

void TestMwmEnvironment::Cleanup(platform::LocalCountryFile const & map)
{
  platform::CountryIndexes::DeleteFromDisk(map);
  map.DeleteFromDisk(MapOptions::Map);
}

bool TestMwmEnvironment::RemoveMwm(MwmSet::MwmId const & mwmId)
{
  auto const & file = mwmId.GetInfo()->GetLocalFile();
  auto const it = find(m_mwmFiles.begin(), m_mwmFiles.end(), file);

  if (it == m_mwmFiles.end())
    return false;

  Cleanup(*it);
  m_mwmFiles.erase(it);
  return true;
}
}  // namespace tests_support
}  // namespace indexer
