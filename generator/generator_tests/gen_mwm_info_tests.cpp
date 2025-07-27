#include "testing/testing.hpp"

#include "generator/composite_id.hpp"
#include "generator/gen_mwm_info.hpp"

#include "coding/file_writer.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

namespace
{
generator::CompositeId const kCid1(base::MakeOsmNode(0), base::MakeOsmNode(0));
generator::CompositeId const kCid2(base::MakeOsmNode(1), base::MakeOsmNode(1));
generator::CompositeId const kCid3(base::MakeOsmNode(1), base::MakeOsmNode(2));

uint32_t const kId1 = 0;
uint32_t const kId2 = 1;
uint32_t const kId3 = 2;

std::pair<generator::CompositeId, uint32_t> const kPId1(kCid1, kId1);
std::pair<generator::CompositeId, uint32_t> const kPId2(kCid2, kId2);
std::pair<generator::CompositeId, uint32_t> const kPId3(kCid3, kId3);

UNIT_TEST(OsmID2FeatureID_AddIds)
{
  generator::OsmID2FeatureID mapping;
  mapping.AddIds(kCid1, kId1);
  mapping.AddIds(kCid2, kId2);
  mapping.AddIds(kCid3, kId3);

  std::vector<std::pair<generator::CompositeId, uint32_t>> const answer{kPId1, kPId2, kPId3};
  size_t index = 0;
  mapping.ForEach([&](auto const & pair)
  {
    TEST_EQUAL(pair, answer[index], ());
    ++index;
  });
  TEST_EQUAL(index, answer.size(), ());
}

UNIT_TEST(OsmID2FeatureID_GetFeatureId)
{
  generator::OsmID2FeatureID mapping;
  mapping.AddIds(kCid1, kId1);
  mapping.AddIds(kCid2, kId2);
  mapping.AddIds(kCid3, kId3);
  {
    std::vector<uint32_t> const answer{kId2, kId3};
    TEST_EQUAL(mapping.GetFeatureIds(kCid2.m_additionalId), answer, ());
  }
  {
    std::vector<uint32_t> const answer;
    TEST_EQUAL(mapping.GetFeatureIds(base::GeoObjectId()), answer, ());
  }
  {
    std::vector<uint32_t> const answer{
        kId1,
    };
    TEST_EQUAL(mapping.GetFeatureIds(kCid1.m_additionalId), answer, ());
  }
  {
    std::vector<uint32_t> const answer{
        kId1,
    };
    TEST_EQUAL(mapping.GetFeatureIds(kCid1), answer, ());
  }
  {
    std::vector<uint32_t> const answer{kId2};
    TEST_EQUAL(mapping.GetFeatureIds(kCid2), answer, ());
  }
  {
    std::vector<uint32_t> const answer{
        kId3,
    };
    TEST_EQUAL(mapping.GetFeatureIds(kCid3), answer, ());
  }
  {
    std::vector<uint32_t> const answer;
    TEST_EQUAL(mapping.GetFeatureIds(generator::CompositeId(base::GeoObjectId())), answer, ());
  }
}

UNIT_TEST(OsmID2FeatureID_ReadWrite)
{
  using namespace platform::tests_support;

  auto const filename = "test.osm2ft";
  ScopedFile sf(filename, ScopedFile::Mode::DoNotCreate);
  {
    generator::OsmID2FeatureID mapping;
    mapping.AddIds(kCid1, kId1);
    mapping.AddIds(kCid2, kId2);
    mapping.AddIds(kCid3, kId3);
    FileWriter writer(sf.GetFullPath());
    mapping.Write(writer);
  }
  {
    generator::OsmID2FeatureID mapping;
    mapping.ReadFromFile(sf.GetFullPath());
    std::vector<std::pair<generator::CompositeId, uint32_t>> const answer{kPId1, kPId2, kPId3};
    size_t index = 0;
    mapping.ForEach([&](auto const & pair)
    {
      TEST_EQUAL(pair, answer[index], ());
      ++index;
    });
    TEST_EQUAL(index, answer.size(), ());
  }
}

UNIT_TEST(OsmID2FeatureID_WorkingWithOldFormat)
{
  using namespace platform::tests_support;

  auto const filename = "test.osm2ft";
  std::vector<std::pair<base::GeoObjectId, uint32_t>> answer{{kCid1.m_mainId, kId1}, {kCid2.m_mainId, kId2}};
  ScopedFile sf(filename, ScopedFile::Mode::DoNotCreate);
  {
    FileWriter writer(sf.GetFullPath());
    rw::WriteVectorOfPOD(writer, answer);
  }
  {
    generator::OsmID2FeatureID mapping;
    mapping.ReadFromFile(sf.GetFullPath());
    size_t index = 0;
    mapping.ForEach([&](auto const & pair)
    {
      TEST_EQUAL(pair.first.m_mainId, answer[index].first, ());
      TEST_EQUAL(pair.second, answer[index].second, ());
      ++index;
    });
    TEST_EQUAL(index, answer.size(), ());
  }
}
}  // namespace
