#pragma once

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/mwm_set.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/local_country_file_utils.hpp"

#include "base/assert.hpp"

#include <string>
#include <utility>
#include <vector>

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
  void LoadExistingEditsXml();

private:
  template <typename BuildFn>
  MwmSet::MwmId ConstructTestMwm(BuildFn && fn)
  {
    return BuildMwm("TestCountry", std::forward<BuildFn>(fn));
  }

  template <typename BuildFn>
  MwmSet::MwmId BuildMwm(std::string const & name, BuildFn && fn, int64_t version = 0)
  {
    m_mwmFiles.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(name), version);
    auto & file = m_mwmFiles.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, feature::DataHeader::MapType::Country);
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
  std::vector<platform::LocalCountryFile> m_mwmFiles;
};
}  // namespace testing
}  // namespace editor
