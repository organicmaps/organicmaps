#include "testing/testing.hpp"

#include "generator/cycling_junctions.hpp"
#include "generator/generator_tests_support/test_generator.hpp"

#include "indexer/feature.hpp"

namespace
{
generator::CyclingWay Road()
{
  generator::CyclingWay road;
  road.m_id = 10;
  road.m_layer = road.m_level = "0";
  road.m_sides = cycling::Encode(
      {{cycling::Kind::Lane, cycling::Direction::Forward}, {cycling::Kind::Shared, cycling::Direction::Both}});
  road.m_nodes = {{1, {0, 0}}, {2, {1, 0}}, {3, {2, 0}}};
  return road;
}

generator::CyclingWay Connector()
{
  auto connector = Road();
  connector.m_id = 20;
  connector.m_sides.clear();
  connector.m_nodes = {{2, {1, 0}}, {4, {1, 1}}};
  return connector;
}

auto Join(generator::CyclingWay const & road, generator::CyclingWay const & connector, bool blocked = false)
{
  return generator::FindCyclingJunctions({road}, {connector}, [blocked](uint64_t) { return blocked; });
}
}  // namespace

UNIT_TEST(CyclingJunctions_SharedNodeAndApproachedSide)
{
  auto joined = Join(Road(), Connector());
  auto const & sides = joined.at(10);
  TEST_EQUAL(sides.m_left.m_junctions, std::vector<m2::PointD>({{1, 0}}), ());
  TEST(sides.m_right.m_junctions.empty(), ());
}

UNIT_TEST(CyclingJunctions_RejectCoincidenceWithoutSharedId)
{
  auto connector = Connector();
  connector.m_nodes[0].m_id = 99;
  TEST(Join(Road(), connector).at(10).m_left.m_junctions.empty(), ());
}

UNIT_TEST(CyclingJunctions_BarrierGradeAndCollinearRejection)
{
  TEST(Join(Road(), Connector(), true).at(10).m_left.m_junctions.empty(), ());
  auto connector = Connector();
  connector.m_layer = "1";
  TEST(Join(Road(), connector).at(10).m_left.m_junctions.empty(), ());
  connector = Connector();
  connector.m_level = "1";
  TEST(Join(Road(), connector).at(10).m_left.m_junctions.empty(), ());
  connector = Connector();
  connector.m_nodes[1].m_point = {3, 0};
  TEST(Join(Road(), connector).at(10).m_left.m_junctions.empty(), ());
}

UNIT_TEST(CyclingJunctions_ReversedRoadAndSharedCenterline)
{
  auto road = Road();
  auto sides = *cycling::Decode(road.m_sides);
  sides.Reverse();
  road.m_sides = cycling::Encode(sides);
  road.m_reversed = true;
  auto joined = Join(road, Connector());
  TEST_EQUAL(joined.at(10).m_right.m_junctions.size(), 1, ());
  TEST(joined.at(10).m_left.m_junctions.empty(), ());
  road = Road();
  road.m_sides = cycling::Encode({{cycling::Kind::Shared, cycling::Direction::Forward}, {}});
  TEST(Join(road, Connector()).at(10).m_left.m_junctions.empty(), ());
}

using generator::tests_support::TestRawGenerator;

UNIT_CLASS_TEST(TestRawGenerator, CyclingJunctions_NormalGeneration)
{
  std::string const country = "CyclingJunctions";
  BuildFB("./data/test_data/osm/cycling_junctions.osm", country);
  BuildFeatures(country);
  size_t roads = 0;
  ForEachFeature(country, [&](std::unique_ptr<FeatureType> feature)
  {
    auto const name = feature->GetDefaultName();
    if (!name.starts_with("Junction road"))
    {
      TEST(feature->GetCyclingInfrastructure().empty(), (name));
      return;
    }
    ++roads;
    auto sides = cycling::Decode(feature->GetCyclingInfrastructure());
    TEST(sides, (name));
    TEST(feature->GetMetadata(feature::Metadata::FMD_TEST_ID).empty(), (name));
    if (name == "Junction road connected")
      TEST_EQUAL(sides->m_left.m_junctions.size(), 1, ());
    else if (name == "Junction road reversed")
      TEST_EQUAL(sides->m_right.m_junctions.size(), 1, ());
    else
    {
      TEST(sides->m_left.m_junctions.empty(), (name));
      TEST(sides->m_right.m_junctions.empty(), (name));
      if (name == "Junction road modifier only")
      {
        TEST_EQUAL(static_cast<char>(sides->m_left.m_kind), 'l', ());
        TEST_EQUAL(static_cast<char>(sides->m_right.m_kind), 'l', ());
      }
    }
  });
  TEST_EQUAL(roads, 7, ());
}
