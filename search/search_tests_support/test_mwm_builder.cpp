#include "search/search_tests_support/test_mwm_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_header.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/search_index_builder.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_sorter.hpp"

#include "platform/local_country_file.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

namespace search
{
namespace tests_support
{
TestMwmBuilder::TestMwmBuilder(platform::LocalCountryFile & file, feature::DataHeader::MapType type)
    : m_file(file),
      m_type(type),
      m_collector(
          make_unique<feature::FeaturesCollector>(file.GetPath(MapOptions::Map) + EXTENSION_TMP)),
      m_classificator(classif())
{
}

TestMwmBuilder::~TestMwmBuilder()
{
  if (m_collector)
    Finish();
  CHECK(!m_collector, ("Features weren't dumped on disk."));
}

void TestMwmBuilder::AddPOI(m2::PointD const & p, string const & name, string const & lang)
{
  CHECK(m_collector, ("It's not possible to add features after call to Finish()."));
  FeatureBuilder1 fb;
  fb.SetCenter(p);
  fb.SetType(m_classificator.GetTypeByPath({"railway", "station"}));
  CHECK(fb.AddName(lang, name), ("Can't set feature name:", name, "(", lang, ")"));
  (*m_collector)(fb);
}

void TestMwmBuilder::AddCity(m2::PointD const & p, string const & name, string const & lang)
{
  CHECK(m_collector, ("It's not possible to add features after call to Finish()."));
  FeatureBuilder1 fb;
  fb.SetCenter(p);
  fb.SetType(m_classificator.GetTypeByPath({"place", "city"}));
  fb.SetRank(100);
  CHECK(fb.AddName(lang, name), ("Can't set feature name:", name, "(", lang, ")"));
  (*m_collector)(fb);
}

void TestMwmBuilder::Finish()
{
  CHECK(m_collector, ("Finish() already was called."));
  m_collector.reset();
  feature::GenerateInfo info;
  info.m_targetDir = m_file.GetDirectory();
  info.m_tmpDir = m_file.GetDirectory();
  CHECK(GenerateFinalFeatures(info, m_file.GetCountryFile().GetNameWithoutExt(), m_type),
        ("Can't sort features."));
  CHECK(feature::BuildOffsetsTable(m_file.GetPath(MapOptions::Map)), ("Can't build feature offsets table."));
  CHECK(indexer::BuildIndexFromDatFile(m_file.GetPath(MapOptions::Map),
                                       m_file.GetPath(MapOptions::Map)),
        ("Can't build geometry index."));
  CHECK(indexer::BuildSearchIndexFromDatFile(m_file.GetPath(MapOptions::Map),
                                             true /* forceRebuild */),
        ("Can't build search index."));
  m_file.SyncWithDisk();
}
}  // namespace tests_support
}  // namespace search
