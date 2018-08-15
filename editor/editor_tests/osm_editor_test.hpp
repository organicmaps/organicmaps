#pragma once

#include "map/feature_vec_model.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/local_country_file_utils.hpp"

namespace editor
{
namespace testing
{
class EditorTest
{
public:
  EditorTest();
  ~EditorTest();

  void GetFeatureTypeInfoTest();
  void GetEditedFeatureTest();
  void SetIndexTest();
  void GetEditedFeatureStreetTest();
  void GetFeatureStatusTest();
  void IsFeatureUploadedTest();
  void DeleteFeatureTest();
  void ClearAllLocalEditsTest();
  void GetFeaturesByStatusTest();
  void OnMapDeregisteredTest();
  void RollBackChangesTest();
  void HaveMapEditsOrNotesToUploadTest();
  void HaveMapEditsToUploadTest();
  void GetStatsTest();
  void IsCreatedFeatureTest();
  void ForEachFeatureInMwmRectAndScaleTest();
  void CreateNoteTest();
  void LoadMapEditsTest();
  void SaveEditedFeatureTest();
  void SaveTransactionTest();

private:
  template <typename TBuildFn>
  MwmSet::MwmId ConstructTestMwm(TBuildFn && fn)
  {
    return BuildMwm("TestCountry", forward<TBuildFn>(fn));
  }

  template <typename TBuildFn>
  MwmSet::MwmId BuildMwm(string const & name, TBuildFn && fn, int64_t version = 0)
  {
    m_mwmFiles.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(name), version);
    auto & file = m_mwmFiles.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, feature::DataHeader::country);
      fn(builder);
    }

    auto result = m_dataSource.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const & id = result.first;

    auto const & info = id.GetInfo();
    if (info)
      m_infoGetter.AddCountry(storage::CountryDef(name, info->m_bordersRect));

    CHECK(id.IsAlive(), ());

    return id;
  }

  void Cleanup(platform::LocalCountryFile const & map);
  bool RemoveMwm(MwmSet::MwmId const & mwmId);

  EditableDataSource m_dataSource;
  storage::CountryInfoGetterForTesting m_infoGetter;
  vector<platform::LocalCountryFile> m_mwmFiles;
};
}  // namespace testing
}  // namespace editor
