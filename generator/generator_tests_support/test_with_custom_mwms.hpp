#pragma once

#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_header.hpp"
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
  template <typename BuildFn>
  MwmSet::MwmId BuildMwm(std::string name, feature::DataHeader::MapType type, BuildFn && fn)
  {
    m_files.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(std::move(name)), 0 /* version */);
    auto & file = m_files.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, type, m_version);
      fn(builder);
    }

    auto result = m_dataSource.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto id = result.first;
    auto const info = id.GetInfo();
    CHECK(info.get(), ());
    OnMwmBuilt(*info);
    return id;
  }

  void DeregisterMap(std::string const & name);

  template <typename BuildFn>
  MwmSet::MwmId BuildWorld(BuildFn && fn)
  {
    return BuildMwm("testWorld", feature::DataHeader::MapType::World, fn);
  }

  template <typename BuildFn>
  MwmSet::MwmId BuildCountry(std::string_view name, BuildFn && fn)
  {
    return BuildMwm(std::string(name), feature::DataHeader::MapType::Country, fn);
  }

  void SetMwmVersion(uint32_t version) { m_version = version; }

  void RegisterLocalMapsInViewport(m2::RectD const & viewport);
  void RegisterLocalMapsByPrefix(std::string const & prefix);

protected:
  template <class FnT>
  void RegisterLocalMapsImpl(FnT && check);

  static void Cleanup(platform::LocalCountryFile const & file);

  virtual void OnMwmBuilt(MwmInfo const & /* info */) {}

  EditableDataSource m_dataSource;
  std::vector<platform::LocalCountryFile> m_files;
  uint32_t m_version = 0;
};
}  // namespace tests_support
}  // namespace generator
