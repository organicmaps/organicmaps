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
  TEST_EQUAL(sides.m_left.m_approaches, std::vector<std::vector<m2::PointD>>({{{1, 1}}}), ());
}

UNIT_TEST(CyclingJunctions_PreserveAllEligibleIncidents)
{
  auto second = Connector();
  second.m_id = 21;
  second.m_nodes[1].m_point = {2, 1};
  auto joined = generator::FindCyclingJunctions({Road()}, {Connector(), second}, [](uint64_t) { return false; });
  TEST_EQUAL(joined.at(10).m_left.m_junctions.size(), 1, ());
  TEST_EQUAL(joined.at(10).m_left.m_approaches[0], std::vector<m2::PointD>({{1, 1}, {2, 1}}), ());
  auto reordered = generator::FindCyclingJunctions({Road(), Road()}, {second, Connector(), Connector()},
                                                   [](uint64_t) { return false; });
  TEST_EQUAL(cycling::Encode(joined.at(10)), cycling::Encode(reordered.at(10)), ());
  second.m_layer = "1";
  joined = generator::FindCyclingJunctions({Road()}, {Connector(), second}, [](uint64_t) { return false; });
  TEST_EQUAL(joined.at(10).m_left.m_approaches[0], std::vector<m2::PointD>({{1, 1}}), ());
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

UNIT_TEST(CyclingJunctions_ConnectorContactsGradeEndpointsAndReversal)
{
  auto road = Road();
  auto connector = Connector();
  auto find = [&](bool blocked = false)
  {
    return generator::FindCyclingConnectorContacts({road}, {connector}, {{10, 42}},
                                                   [blocked](uint64_t) { return blocked; });
  };
  auto contacts = find();
  TEST_EQUAL(contacts.at(20).size(), 1, ());
  TEST_EQUAL(contacts.at(20)[0].m_side, -1, ());
  TEST_EQUAL(contacts.at(20)[0].m_roadType, 42, ());
  TEST(find(true).at(20).empty(), ());
  connector.m_layer = "1";
  TEST(find().at(20).empty(), ());
  connector.m_layer = "0";
  connector.m_level = "1";
  TEST(find().at(20).empty(), ());
  connector = Connector();
  connector.m_nodes.insert(connector.m_nodes.begin(), {5, {1, -1}});
  TEST(find().at(20).empty(), ("Shared interior nodes cannot move connector endpoints"));
  connector = Connector();
  auto sides = *cycling::Decode(road.m_sides);
  sides.Reverse();
  road.m_sides = cycling::Encode(sides);
  road.m_reversed = true;
  contacts = find();
  TEST_EQUAL(contacts.at(20)[0].m_side, 1, ());
  TEST_EQUAL(contacts.at(20)[0].m_roadPath.front(), road.m_nodes.back().m_point, ());
}

UNIT_TEST(CyclingJunctions_ConnectorContactsCanonicalAndBounded)
{
  auto const connector = Connector();
  auto road = Road();
  auto contacts = generator::FindCyclingConnectorContacts({road, road}, {connector, connector}, {{10, 42}},
                                                          [](uint64_t) { return false; });
  TEST_EQUAL(contacts.at(20).size(), 1, ("Duplicate collector input must not duplicate contacts"));
  cycling::Sides data;
  data.m_contacts = contacts.at(20);
  auto const expected = cycling::Encode(data);
  contacts = generator::FindCyclingConnectorContacts({road}, {connector}, {{10, 42}}, [](uint64_t) { return false; });
  data.m_contacts = contacts.at(20);
  TEST_EQUAL(cycling::Encode(data), expected, ());

  std::vector<generator::CyclingWay> roads;
  std::map<uint64_t, uint32_t> types;
  for (size_t i = 0; i <= cycling::kMaxConnectorContacts; ++i)
  {
    road.m_id = 100 + i;
    road.m_nodes.front().m_point.x = -double(i);
    roads.push_back(road);
    types[road.m_id] = 42;
  }
  contacts = generator::FindCyclingConnectorContacts(roads, {connector}, types, [](uint64_t) { return false; });
  TEST(contacts.at(20).empty(), ("Overflow must never publish a partial set of contacts"));
  std::reverse(roads.begin(), roads.end());
  contacts = generator::FindCyclingConnectorContacts(roads, {connector}, types, [](uint64_t) { return false; });
  TEST(contacts.at(20).empty(), ("Input ordering must not change overflow handling"));
}

using generator::tests_support::TestRawGenerator;

UNIT_CLASS_TEST(TestRawGenerator, CyclingJunctions_NormalGeneration)
{
  std::string const country = "CyclingJunctions";
  BuildFB("./data/test_data/osm/cycling_junctions.osm", country);
  BuildFeatures(country);
  size_t roads = 0;
  size_t connectors = 0;
  ForEachFeature(country, [&](std::unique_ptr<FeatureType> feature)
  {
    auto const name = feature->GetDefaultName();
    if (!name.starts_with("Junction road"))
    {
      if (auto const data = cycling::Decode(feature->GetCyclingInfrastructure()))
      {
        ++connectors;
        TEST_EQUAL(data->m_left.m_kind, cycling::Kind::None, ());
        TEST_EQUAL(data->m_right.m_kind, cycling::Kind::None, ());
        TEST(!data->m_contacts.empty(), ());
        for (auto const & contact : data->m_contacts)
        {
          TEST(contact.m_roadPath.size() >= 2 && contact.m_roadPath.size() <= 5, ());
          TEST_EQUAL(contact.m_kind, cycling::Kind::Lane, ());
          TEST(contact.m_side == -1 || contact.m_side == 1, ());
          TEST_NOT_EQUAL(contact.m_roadType, 0, ());
        }
      }
      else
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
  TEST_EQUAL(connectors, 2, ());
}
