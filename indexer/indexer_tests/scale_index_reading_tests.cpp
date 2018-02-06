#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "indexer/indexer_tests_support/test_with_custom_mwms.hpp"

#include "indexer/cell_id.hpp"
#include "indexer/data_factory.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/index.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scale_index.hpp"

#include "platform/country_defines.hpp"
#include "platform/local_country_file.hpp"

#include "coding/file_container.hpp"
#include "coding/mmap_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/rect2d.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace generator::tests_support;
using namespace indexer::tests_support;
using namespace indexer;
using namespace std;

namespace
{
using Names = vector<string>;

class ScaleIndexReadingTest : public TestWithCustomMwms
{
public:
  template <typename ScaleIndex>
  Names CollectNames(MwmSet::MwmId const & id, ScaleIndex const & index, int scale,
                     m2::RectD const & rect)
  {
    covering::CoveringGetter covering(rect, covering::ViewportWithLowLevels);

    vector<uint32_t> indices;
    for (auto const & interval : covering.Get<RectId::DEPTH_LEVELS>(scale))
    {
      index.ForEachInIntervalAndScale([&](uint32_t index) { indices.push_back(index); },
                                      interval.first, interval.second, scale);
    }

    Index::FeaturesLoaderGuard loader(m_index, id);

    Names names;
    for (auto const & index : indices)
    {
      FeatureType ft;
      TEST(loader.GetFeatureByIndex(index, ft), ("Can't load feature by index:", index));

      string name;
      TEST(ft.GetName(StringUtf8Multilang::kEnglishCode, name),
           ("Can't get en name by index:", index));
      names.push_back(name);
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

  auto id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder) {
    builder.Add(a);
    builder.Add(b);
    builder.Add(c);
    builder.Add(d);
  });

  TEST(id.IsAlive(), ());

  auto const path = id.GetInfo()->GetLocalFile().GetPath(MapOptions::Map);

  FilesContainerR cont(path);
  feature::DataHeader header(cont);

  IndexFactory factory;
  factory.Load(cont);

  auto const offsetSize = cont.GetAbsoluteOffsetAndSize(INDEX_FILE_TAG);

  MmapReader reader(path);
  ReaderPtr<Reader> subReader(reader.CreateSubReader(offsetSize.first, offsetSize.second));

  ScaleIndex<ReaderPtr<Reader>> index(subReader, factory);

  auto collectNames = [&](m2::RectD const & rect) {
    return CollectNames(id, index, header.GetLastScale(), rect);
  };

  TEST_EQUAL(collectNames(m2::RectD{-0.5, -0.5, 0.5, 0.5}), Names({"A"}), ());
  TEST_EQUAL(collectNames(m2::RectD{0.5, -0.5, 1.5, 1.5}), Names({"B", "C"}), ());
  TEST_EQUAL(collectNames(m2::RectD{-0.5, -0.5, 1.5, 1.5}), Names({"A", "B", "C", "D"}), ());
}
}  // namespace
