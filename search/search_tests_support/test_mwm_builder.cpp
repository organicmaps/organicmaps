#include "search/search_tests_support/test_mwm_builder.hpp"

#include "search/search_tests_support/test_feature.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_sorter.hpp"
#include "generator/search_index_builder.hpp"

#include "indexer/data_header.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/rank_table.hpp"

#include "platform/local_country_file.hpp"

#include "coding/file_writer.hpp"

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
          make_unique<feature::FeaturesCollector>(m_file.GetPath(MapOptions::Map) + EXTENSION_TMP))
{
}

TestMwmBuilder::~TestMwmBuilder()
{
  if (m_collector)
    Finish();
  CHECK(!m_collector, ("Features weren't dumped on disk."));
}

void TestMwmBuilder::Add(TestFeature const & feature)
{
  CHECK(m_collector, ("It's not possible to add features after call to Finish()."));
  FeatureBuilder1 fb;
  feature.Serialize(fb);
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

  string const path = m_file.GetPath(MapOptions::Map);
  CHECK(feature::BuildOffsetsTable(path), ("Can't build feature offsets table."));
  CHECK(indexer::BuildIndexFromDataFile(path, path), ("Can't build geometry index."));
  CHECK(indexer::BuildSearchIndexFromDataFile(path, true /* forceRebuild */),
        ("Can't build search index."));
  CHECK(search::RankTableBuilder::CreateIfNotExists(path), ());

  m_file.SyncWithDisk();
}
}  // namespace tests_support
}  // namespace search
