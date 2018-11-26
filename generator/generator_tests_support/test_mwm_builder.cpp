#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "generator/centers_table_builder.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_sorter.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/search_index_builder.hpp"

#include "indexer/city_boundary.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/rank_table.hpp"

#include "platform/local_country_file.hpp"

#include "coding/internal/file_data.hpp"

#include "base/string_utils.hpp"

#include "defines.hpp"

#include <memory>

using namespace std;

namespace
{
bool WriteRegionDataForTests(string const & path, vector<string> const & languages)
{
  try
  {
    FilesContainerW writer(path, FileWriter::OP_WRITE_EXISTING);
    feature::RegionData regionData;
    regionData.SetLanguages(languages);
    FileWriter w = writer.GetWriter(REGION_INFO_FILE_TAG);
    regionData.Serialize(w);
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error writing file:", e.Msg()));
    return false;
  }
  return true;
}
}  // namespace

namespace generator
{
namespace tests_support
{
TestMwmBuilder::TestMwmBuilder(platform::LocalCountryFile & file, feature::DataHeader::MapType type,
                               uint32_t version)
  : m_file(file)
  , m_type(type)
  , m_collector(
        make_unique<feature::FeaturesCollector>(m_file.GetPath(MapOptions::Map) + EXTENSION_TMP))
  , m_version(version)
{
}

TestMwmBuilder::~TestMwmBuilder()
{
  if (m_collector)
    Finish();
}

void TestMwmBuilder::Add(TestFeature const & feature)
{
  FeatureBuilder1 fb;
  feature.Serialize(fb);
  CHECK(Add(fb), (fb));
}

bool TestMwmBuilder::Add(FeatureBuilder1 & fb)
{
  CHECK(m_collector, ("It's not possible to add features after call to Finish()."));

  if (ftypes::IsCityTownOrVillage(fb.GetTypes()) && fb.GetGeomType() == feature::GEOM_AREA)
  {
    auto const & metadata = fb.GetMetadata();
    uint64_t testId;
    CHECK(strings::to_uint64(metadata.Get(feature::Metadata::FMD_TEST_ID), testId), ());
    m_boundariesTable.Append(testId, indexer::CityBoundary(fb.GetOuterGeometry()));

    auto const center = fb.GetGeometryCenter();
    fb.ResetGeometry();
    fb.SetCenter(center);
  }

  if (!fb.PreSerializeAndRemoveUselessNames())
  {
    LOG(LWARNING, ("Can't pre-serialize feature."));
    return false;
  }

  if (!fb.RemoveInvalidTypes())
  {
    LOG(LWARNING, ("No types."));
    return false;
  }

  (*m_collector)(fb);
  return true;
}

void TestMwmBuilder::SetMwmLanguages(vector<string> const & languages)
{
  m_languages = languages;
}

void TestMwmBuilder::Finish()
{
  string const tmpFilePath = m_collector->GetFilePath();

  CHECK(m_collector, ("Finish() already was called."));
  m_collector.reset();

  feature::GenerateInfo info;
  info.m_targetDir = m_file.GetDirectory();
  info.m_tmpDir = m_file.GetDirectory();
  info.m_versionDate = static_cast<uint32_t>(base::YYMMDDToSecondsSinceEpoch(m_version));
  CHECK(GenerateFinalFeatures(info, m_file.GetCountryFile().GetName(), m_type),
        ("Can't sort features."));

  CHECK(base::DeleteFileX(tmpFilePath), ());

  string const path = m_file.GetPath(MapOptions::Map);
  (void)base::DeleteFileX(path + OSM2FEATURE_FILE_EXTENSION);

  CHECK(feature::BuildOffsetsTable(path), ("Can't build feature offsets table."));

  CHECK(indexer::BuildIndexFromDataFile(path, path), ("Can't build geometry index."));

  CHECK(indexer::BuildSearchIndexFromDataFile(path, true /* forceRebuild */, 1 /* threadsCount */),
        ("Can't build search index."));

  if (m_type == feature::DataHeader::world)
    CHECK(generator::BuildCitiesBoundariesForTesting(path, m_boundariesTable), ());

  CHECK(indexer::BuildCentersTableFromDataFile(path, true /* forceRebuild */),
        ("Can't build centers table."));

  CHECK(search::SearchRankTableBuilder::CreateIfNotExists(path), ());

  if (!m_languages.empty())
    CHECK(WriteRegionDataForTests(path, m_languages), ());

  m_file.SyncWithDisk();
}
}  // namespace tests_support
}  // namespace generator
