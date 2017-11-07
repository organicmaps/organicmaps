#pragma once

#include "search/search_tests_support/test_with_classificator.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "indexer/indexer_tests_support/helpers.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"

#include <string>
#include <vector>
#include <utility>

namespace search
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
  // *NOTE* when |type| is feature::DataHeader::country, the country
  // with |name| will be automatically registered.
  template <typename BuildFn>
  MwmSet::MwmId BuildMwm(std::string const & name, feature::DataHeader::MapType type, BuildFn && fn)
  {
    m_files.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(name), 0 /* version */);
    auto & file = m_files.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, type);
      fn(builder);
    }

    auto result = m_index.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const id = result.first;
    auto const info = id.GetInfo();
    CHECK(info.get(), ());
    OnMwmBuilt(*info);
    return id;
  }

  template <typename BuildFn>
  MwmSet::MwmId BuildWorld(BuildFn && fn)
  {
    return BuildMwm("testWorld", feature::DataHeader::world, std::forward<BuildFn>(fn));
  }

  template <typename BuildFn>
  MwmSet::MwmId BuildCountry(std::string const & name, BuildFn && fn)
  {
    return BuildMwm(name, feature::DataHeader::country, std::forward<BuildFn>(fn));
  }

  template <typename EditorFn>
  void EditFeature(FeatureID const & id, EditorFn && fn)
  {
    Index::FeaturesLoaderGuard loader(m_index, id.m_mwmId);
    FeatureType ft;
    CHECK(loader.GetFeatureByIndex(id.m_index, ft), ());
    indexer::tests_support::EditFeature(ft, forward<EditorFn>(fn));
  }

protected:
  static void Cleanup(platform::LocalCountryFile const & file);

  virtual void OnMwmBuilt(MwmInfo const & /* info */) {}

  Index m_index;
  std::vector<platform::LocalCountryFile> m_files;
};
}  // namespace tests_support
}  // namespace search
