#include "test_mwm_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_header.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/search_index_builder.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_sorter.hpp"

#include "platform/local_country_file.hpp"

#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include "defines.hpp"


TestMwmBuilder::TestMwmBuilder(platform::LocalCountryFile & file)
    : m_file(file),
      m_collector(
          make_unique<feature::FeaturesCollector>(file.GetPath(MapOptions::Map) + EXTENSION_TMP)),
      m_classificator(classif())
{
}

TestMwmBuilder::~TestMwmBuilder()
{
  if (m_collector)
    Finish();
}

void TestMwmBuilder::AddPOI(m2::PointD const & p, string const & name, string const & lang)
{
  FeatureBuilder1 fb;
  fb.SetCenter(p);
  fb.SetType(m_classificator.GetTypeByPath({"railway", "station"}));
  CHECK(fb.AddName(lang, name), ("Can't set feature name:", name, "(", lang, ")"));

  CHECK(Add(fb), (fb));
}

bool TestMwmBuilder::Add(FeatureBuilder1 & fb)
{
  CHECK(m_collector, ("It's not possible to add features after call to Finish()."));

  if (fb.PreSerialize() && fb.RemoveInvalidTypes())
  {
    (*m_collector)(fb);
    return true;
  }
  return false;
}

void TestMwmBuilder::Finish()
{
  string const tmpFilePath = m_collector->GetFilePath();

  CHECK(m_collector, ("Finish() already was called."));
  m_collector.reset();

  feature::GenerateInfo info;
  info.m_targetDir = m_file.GetDirectory();
  info.m_tmpDir = m_file.GetDirectory();
  CHECK(GenerateFinalFeatures(info, m_file.GetCountryFile().GetNameWithoutExt(),
                              feature::DataHeader::country),
        ("Can't sort features."));

  CHECK(my::DeleteFileX(tmpFilePath), ());

  string const mapFilePath = m_file.GetPath(MapOptions::Map);
  CHECK(feature::BuildOffsetsTable(mapFilePath), ("Can't build feature offsets table."));

  CHECK(indexer::BuildIndexFromDatFile(mapFilePath, mapFilePath), ("Can't build geometry index."));

  CHECK(indexer::BuildSearchIndexFromDatFile(mapFilePath, true /* forceRebuild */),
        ("Can't build search index."));

  m_file.SyncWithDisk();
}
