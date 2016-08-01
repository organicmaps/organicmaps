#pragma once

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "platform/local_country_file_utils.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/mwm_set.hpp"

#include "map/feature_vec_model.hpp"

namespace tests
{
class EditorTest
{
public:
  EditorTest();
  ~EditorTest();

  void GetFeatureTypeInfoTest();
  void GetEditedFeatureTest();
  void GetEditedFeatureStreetTest();
  void OriginalFeatureHasDefaultNameTest();
  void GetFeatureStatusTest();
  void IsFeatureUploadedTest();
  void DeleteFeatureTest();
  void ClearAllLocalEditsTest();
  void GetFeaturesByStatusTest();
  void OnMapDeregisteredTest();

private:
  template <typename TBuildFn>
  MwmSet::MwmId ConstructTestMwm(TBuildFn && fn)
  {
    generator::tests_support::TestCity london(m2::PointD(1, 1), "London", "en", 100 /* rank */);
    BuildMwm("TestWorld", feature::DataHeader::world,[&](generator::tests_support::TestMwmBuilder & builder)
    {
      builder.Add(london);
    });

    return BuildMwm("SomeCountry", feature::DataHeader::country, forward<TBuildFn>(fn));
  }

  template <typename TBuildFn>
  MwmSet::MwmId BuildMwm(string const & name, feature::DataHeader::MapType type, TBuildFn && fn)
  {
    m_files.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(name), 0 /* version */);
    auto & file = m_files.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, type);
      fn(builder);
    }

    auto result = m_model.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const & id = result.first;
    if (type == feature::DataHeader::country)
    {
      auto const & info = id.GetInfo();
      if (info)
      {
        auto & infoGetter = static_cast<storage::CountryInfoGetterForTesting &>(*m_infoGetter);
        infoGetter.AddCountry(storage::CountryDef(name, info->m_limitRect));
      }
    }
    return id;
  }

  void Cleanup(platform::LocalCountryFile const & map);
  void InitEditorForTest();

  model::FeaturesFetcher m_model;
  unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  vector<platform::LocalCountryFile> m_files;
};
}  // namespace tests
