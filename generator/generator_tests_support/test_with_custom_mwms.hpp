#pragma once

#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <string>
#include <utility>
#include <vector>

namespace generator
{
namespace tests_support
{
class TestWithCustomMwms : public TestWithClassificator
{
public:
  TestWithCustomMwms();

  ~TestWithCustomMwms() override;

  // Creates a physical country file on a disk, which will be removed
  // at the end of the test. |fn| is a delegate that accepts a single
  // argument - TestMwmBuilder and adds all necessary features to the
  // country file.
  //
  // *NOTE* when |type| is feature::DataHeader::MapType::Country, the country
  // with |name| will be automatically registered.
  template <typename BuildFn>
  MwmSet::MwmId BuildMwm(std::string const & name, feature::DataHeader::MapType type, BuildFn && fn)
  {
    m_files.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(name), 0 /* version */);
    auto & file = m_files.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, type, m_version);
      fn(builder);
    }

    auto result = m_dataSource.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const id = result.first;
    auto const info = id.GetInfo();
    CHECK(info.get(), ());
    OnMwmBuilt(*info);
    return id;
  }

  void DeregisterMap(std::string const & name)
  {
    auto const file = platform::CountryFile(name);
    auto it = base::FindIf(m_files, [&file](platform::LocalCountryFile const & f) {
      return f.GetCountryFile() == file;
    });

    if (it == m_files.end())
      return;

    m_dataSource.DeregisterMap(file);
    Cleanup(*it);
    m_files.erase(it);
  }

  template <typename BuildFn>
  MwmSet::MwmId BuildWorld(BuildFn && fn)
  {
    return BuildMwm("testWorld", feature::DataHeader::MapType::World, std::forward<BuildFn>(fn));
  }

  template <typename BuildFn>
  MwmSet::MwmId BuildCountry(std::string const & name, BuildFn && fn)
  {
    return BuildMwm(name, feature::DataHeader::MapType::Country, std::forward<BuildFn>(fn));
  }

  void SetMwmVersion(uint32_t version);

protected:
  static void Cleanup(platform::LocalCountryFile const & file);

  virtual void OnMwmBuilt(MwmInfo const & /* info */) {}

  EditableDataSource m_dataSource;
  std::vector<platform::LocalCountryFile> m_files;
  uint32_t m_version = 0;
};
}  // namespace tests_support
}  // namespace generator
