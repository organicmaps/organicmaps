#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/local_country_file_utils.hpp"

#include "base/stl_helpers.hpp"
#include "base/timer.hpp"

namespace generator
{
namespace tests_support
{
using namespace platform;

TestWithCustomMwms::TestWithCustomMwms()
{
  m_version = base::GenerateYYMMDD(base::SecondsSinceEpoch());
}

TestWithCustomMwms::~TestWithCustomMwms()
{
  m_dataSource.ClearCache();
  for (auto const & file : m_files)
    Cleanup(file);
}

// static
void TestWithCustomMwms::Cleanup(LocalCountryFile const & file)
{
  CountryIndexes::DeleteFromDisk(file);
  file.DeleteFromDisk(MapFileType::Map);
}

void TestWithCustomMwms::DeregisterMap(std::string const & name)
{
  auto const file = CountryFile(name);
  auto it = base::FindIf(m_files, [&file](LocalCountryFile const & f) { return f.GetCountryFile() == file; });

  if (it == m_files.end())
    return;

  m_dataSource.DeregisterMap(file);
  Cleanup(*it);
  m_files.erase(it);
}

template <class FnT>
void TestWithCustomMwms::RegisterLocalMapsImpl(FnT && check)
{
  std::vector<LocalCountryFile> localFiles;
  FindAllLocalMapsAndCleanup(std::numeric_limits<int64_t>::max() /* latestVersion */, localFiles);

  for (auto const & file : localFiles)
  {
    // Always load World.mwm, important for search.
    auto const & name = file.GetCountryName();
    if (name != WORLD_FILE_NAME && !check(name))
      continue;

    auto const res = m_dataSource.RegisterMap(file);
    if (res.second == MwmSet::RegResult::Success)
    {
      auto const & info = res.first.GetInfo();
      OnMwmBuilt(*info);
    }
    else
      CHECK_EQUAL(res.second, MwmSet::RegResult::VersionAlreadyExists, ());
  }
}

void TestWithCustomMwms::RegisterLocalMapsInViewport(m2::RectD const & viewport)
{
  auto const countriesInfo = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());

  RegisterLocalMapsImpl([&](std::string const & name)
  { return countriesInfo->GetLimitRectForLeaf(name).IsIntersect(viewport); });
}

void TestWithCustomMwms::RegisterLocalMapsByPrefix(std::string const & prefix)
{
  RegisterLocalMapsImpl([&](std::string const & name) { return name.starts_with(prefix); });
}

}  // namespace tests_support
}  // namespace generator
