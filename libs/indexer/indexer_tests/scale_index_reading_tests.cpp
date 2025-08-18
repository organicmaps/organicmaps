#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"
#include "generator/generator_tests_support/test_with_custom_mwms.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/data_header.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include "coding/files_container.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "defines.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace scale_index_reading_tests
{
using namespace generator::tests_support;
using namespace indexer;
using namespace std;

using Names = vector<string>;

class ScaleIndexReadingTest : public TestWithCustomMwms
{
public:
  template <typename ScaleIndex>
  Names CollectNames(MwmSet::MwmId const & id, ScaleIndex const & index, int scaleForIntervals, int scaleForZoomLevels,
                     m2::RectD const & rect)
  {
    covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);

    vector<uint32_t> indices;
    for (auto const & interval : covering.Get<RectId::DEPTH_LEVELS>(scaleForIntervals))
    {
      index.ForEachInIntervalAndScale(interval.first, interval.second, scaleForZoomLevels,
                                      [&](uint64_t /* key */, uint32_t value) { indices.push_back(value); });
    }

    FeaturesLoaderGuard loader(m_dataSource, id);

    Names names;
    for (auto const & index : indices)
    {
      auto ft = loader.GetFeatureByIndex(index);
      TEST(ft, (index));

      string_view const name = ft->GetName(StringUtf8Multilang::kEnglishCode);
      TEST(!name.empty(), (index));
      names.push_back(std::string(name));
    }

    sort(names.begin(), names.end());
    return names;
  }
};

UNIT_CLASS_TEST(ScaleIndexReadingTest, Mmap)
{
  TestPOI a(m2::PointD{0, 0}, "A", "en");
  TestPOI b(m2::PointD{1, 0}, "B", "en");
  TestPOI c(m2::PointD{1, 1}, "C", "en");
  TestPOI d(m2::PointD{0, 1}, "D", "en");

  auto id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(a);
    builder.Add(b);
    builder.Add(c);
    builder.Add(d);
  });

  TEST(id.IsAlive(), ());

  auto const path = id.GetInfo()->GetLocalFile().GetPath(MapFileType::Map);

  FilesContainerR cont(path);
  feature::DataHeader header(cont);

  auto const offsetSize = cont.GetAbsoluteOffsetAndSize(INDEX_FILE_TAG);

  MmapReader reader(path);
  ReaderPtr<Reader> subReader(reader.CreateSubReader(offsetSize.first, offsetSize.second));

  ScaleIndex<ReaderPtr<Reader>> index(subReader);

  auto collectNames = [&](m2::RectD const & rect)
  { return CollectNames(id, index, header.GetLastScale(), header.GetLastScale(), rect); };

  TEST_EQUAL(collectNames(m2::RectD{-0.5, -0.5, 0.5, 0.5}), Names({"A"}), ());
  TEST_EQUAL(collectNames(m2::RectD{0.5, -0.5, 1.5, 1.5}), Names({"B", "C"}), ());
  TEST_EQUAL(collectNames(m2::RectD{-0.5, -0.5, 1.5, 1.5}), Names({"A", "B", "C", "D"}), ());

  auto collectNamesForExactScale = [&](m2::RectD const & rect, int scale)
  { return CollectNames(id, index, header.GetLastScale(), scale, rect); };

  auto const drawableScale = feature::GetMinDrawableScaleClassifOnly(a.GetTypes());
  CHECK_LESS(drawableScale, header.GetLastScale(), ("Change the test to ensure scales less than last scale work."));

  TEST_EQUAL(collectNamesForExactScale(m2::RectD{-0.5, -0.5, 0.5, 0.5}, drawableScale), Names({"A"}), ());
  TEST_EQUAL(collectNamesForExactScale(m2::RectD{0.5, -0.5, 1.5, 1.5}, drawableScale), Names({"B", "C"}), ());
  TEST_EQUAL(collectNamesForExactScale(m2::RectD{-0.5, -0.5, 1.5, 1.5}, drawableScale), Names({"A", "B", "C", "D"}),
             ());
}
}  // namespace scale_index_reading_tests
