#include "testing/testing.hpp"

#include "indexer/data_header.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/scales.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "coding/files_container.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace index_builder_test
{
using namespace generator::tests_support;
using namespace std;

vector<uint32_t> FindFeatures(ScaleIndex<ModelReaderPtr> const & index, covering::Intervals const & intervals,
                              int scale)
{
  vector<uint32_t> features;
  for (auto const & interval : intervals)
    index.ForEachInIntervalAndScale(interval.first, interval.second, scale,
                                    [&features](uint32_t feature) { features.push_back(feature); });
  sort(features.begin(), features.end());
  return features;
}

UNIT_CLASS_TEST(TestWithClassificator, BuildIndexTest)
{
  Platform & p = GetPlatform();

  FilesContainerR originalContainer(p.GetReader("minsk-pass" DATA_FILE_EXTENSION));

  // Build index.
  vector<char> serialIndex;
  {
    FeaturesVectorTest features(originalContainer);

    MemWriter<vector<char>> serialWriter(serialIndex);
    indexer::BuildIndex(features.GetHeader(), features.GetVector(), serialWriter, "build_index_test");
  }

  // Create a new mwm file.
  string const fileName = "build_index_test" DATA_FILE_EXTENSION;
  string const filePath = p.WritablePathForFile(fileName);
  FileWriter::DeleteFileX(filePath);

  // Copy original mwm file and replace index in it.
  {
    FilesContainerW containerWriter(filePath);
    vector<string> tags;
    originalContainer.ForEachTag(base::MakeBackInsertFunctor(tags));
    for (size_t i = 0; i < tags.size(); ++i)
      if (tags[i] != INDEX_FILE_TAG)
        containerWriter.Write(originalContainer.GetReader(tags[i]), tags[i]);
    containerWriter.Write(serialIndex, INDEX_FILE_TAG);
  }

  {
    // The index of the map, of the version 1 while the map is older than the trie of the cells,
    // and the new one of the latest version give the same features.
    FilesContainerR const container(filePath);
    ScaleIndex<ModelReaderPtr> const oldIndex(originalContainer.GetReader(INDEX_FILE_TAG));
    ScaleIndex<ModelReaderPtr> const newIndex(container.GetReader(INDEX_FILE_TAG));
    TEST(newIndex.GetVersion() == ScaleIndexBase::Version::Latest, ());

    feature::DataHeader const header(originalContainer);
    int const lastScale = header.GetLastScale();
    m2::RectD const bounds = header.GetBounds();

    // The whole map and its parts.
    vector<m2::RectD> rects = {bounds};
    int constexpr kParts = 6;
    double const sizeX = bounds.SizeX() / kParts;
    double const sizeY = bounds.SizeY() / kParts;
    for (int x = 0; x < kParts; ++x)
      for (int y = 0; y < kParts; ++y)
        rects.emplace_back(bounds.minX() + x * sizeX, bounds.minY() + y * sizeY, bounds.minX() + (x + 1) * sizeX,
                           bounds.minY() + (y + 1) * sizeY);

    size_t found = 0;
    for (int scale = 0; scale <= lastScale; ++scale)
    {
      for (auto const & rect : rects)
      {
        covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);
        auto const features = FindFeatures(oldIndex, covering.Get(lastScale), scale);
        TEST_EQUAL(FindFeatures(newIndex, covering.Get(lastScale), scale), features, (scale, rect));
        found += features.size();
      }

      covering::CoveringGetter covering(bounds, covering::FullCover);
      TEST_EQUAL(FindFeatures(newIndex, covering.Get(lastScale), scale),
                 FindFeatures(oldIndex, covering.Get(lastScale), scale), (scale));
    }
    TEST_GREATER(found, 100000, ());
  }

  {
    // Check that index actually works.
    FrozenDataSource dataSource;
    UNUSED_VALUE(dataSource.Register(platform::LocalCountryFile::MakeForTesting("build_index_test")));

    // Make sure that index is actually parsed.
    dataSource.ForEachInScale([](FeatureType &) { return; }, 15);
  }

  // Clean after the test.
  FileWriter::DeleteFileX(filePath);
}
}  // namespace index_builder_test
