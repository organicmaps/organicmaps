#include "testing/testing.hpp"

#include "generator/composite_id.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/hierarchy_entry.hpp"

#include "indexer/complex/tree_node.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/geo_object_id.hpp"

namespace hierarchy_entry_tests
{
using generator::tests_support::TestWithClassificator;
using platform::tests_support::ScopedFile;

std::string const kCsv1 =
    "13835058055284963881 9223372037111861697;"
    ";"
    "1;"
    "37.5303271;"
    "67.3684086;"
    "amenity-university;"
    "Lomonosov Moscow State Univesity;"
    "Russia_Moscow\n"

    "9223372036879747192 9223372036879747192;"
    "13835058055284963881 9223372037111861697;"
    "2;"
    "37.5272372;"
    "67.3775872;"
    "leisure-garden;"
    "Ботанический сад МГУ;"
    "Russia_Moscow\n"

    "9223372036938640141 9223372036938640141;"
    "9223372036879747192 9223372036879747192;"
    "3;"
    "37.5274156;"
    "67.3758813;"
    "amenity-university;"
    "Отдел флоры;"
    "Russia_Moscow\n"

    "9223372036964008573 9223372036964008573;"
    "9223372036879747192 9223372036879747192;"
    "3;"
    "37.5279467;"
    "67.3756452;"
    "amenity-university;"
    "Дендрарий Ботанического сада МГУ;"
    "Russia_Moscow\n"

    "4611686019330739245 4611686019330739245;"
    "13835058055284963881 9223372037111861697;"
    "2;"
    "37.5357492;"
    "67.3735142;"
    "historic-memorial;"
    "Александр Иванович Герцен;"
    "Russia_Moscow\n"

    "4611686019330739269 4611686019330739269;"
    "13835058055284963881 9223372037111861697;"
    "2;"
    "37.5351269;"
    "67.3741606;"
    "historic-memorial;"
    "Николай Гаврилович Чернышевский;"
    "Russia_Moscow\n"

    "4611686019330739276 4611686019330739276;"
    "13835058055284963881 9223372037111861697;"
    "2;"
    "37.5345234;"
    "67.3723206;"
    "historic-memorial;"
    "Николай Егорович Жуковский;"
    "Russia_Moscow";

generator::CompositeId MakeId(uint64_t f, uint64_t s)
{
  return generator::CompositeId(base::GeoObjectId(f), base::GeoObjectId(s));
}

UNIT_CLASS_TEST(TestWithClassificator, Complex_HierarchyEntryCsv)
{
  generator::HierarchyEntry e;
  e.m_id = MakeId(4611686018725364866ull, 4611686018725364866ull);
  e.m_parentId = MakeId(13835058055283414237ull, 9223372037374119493ull);
  e.m_depth = 2;
  e.m_center.x = 37.6262604;
  e.m_center.y = 67.6098812;
  e.m_type = classif().GetTypeByPath({"amenity", "restaurant"});
  e.m_name = "Rybatskoye podvor'ye";
  e.m_country = "Russia_Moscow";

  auto const row = generator::hierarchy::HierarchyEntryToCsvRow(e);
  TEST_EQUAL(row.size(), 8, ());
  TEST_EQUAL(row[0], "4611686018725364866 4611686018725364866", ());
  TEST_EQUAL(row[1], "13835058055283414237 9223372037374119493", ());
  TEST_EQUAL(row[2], "2", ());
  TEST_EQUAL(row[3], "37.6262604", ());
  TEST_EQUAL(row[4], "67.6098812", ());
  TEST_EQUAL(row[5], "amenity-restaurant", ());
  TEST_EQUAL(row[6], "Rybatskoye podvor'ye", ());
  TEST_EQUAL(row[7], "Russia_Moscow", ());

  auto const res = generator::hierarchy::HierarchyEntryFromCsvRow(row);
  TEST_EQUAL(e, res, ());
}

UNIT_CLASS_TEST(TestWithClassificator, Complex_LoadHierachy)
{
  auto const filename = "test.csv";
  ScopedFile sf(filename, kCsv1);
  auto const forest = generator::hierarchy::LoadHierachy(sf.GetFullPath());
  TEST_EQUAL(forest.size(), 1, ());
  auto const & tree = *forest.begin();
  LOG(LINFO, (tree));

  TEST_EQUAL(tree_node::Size(tree), 7, ());
  auto node = tree_node::FindIf(
      tree, [](auto const & e) { return e.m_id == MakeId(13835058055284963881ull, 9223372037111861697ull); });
  TEST(node, ());
  TEST(!node->HasParent(), ());
  TEST_EQUAL(node->GetChildren().size(), 4, ());

  node = tree_node::FindIf(
      tree, [](auto const & e) { return e.m_id == MakeId(9223372036879747192ull, 9223372036879747192ull); });
  TEST(node, ());
  TEST(node->HasParent(), ());
  TEST_EQUAL(node->GetParent()->GetData().m_id, MakeId(13835058055284963881ull, 9223372037111861697ull), ());
  TEST_EQUAL(node->GetChildren().size(), 2, ());

  node = tree_node::FindIf(
      tree, [](auto const & e) { return e.m_id == MakeId(9223372036938640141ull, 9223372036938640141ull); });
  TEST(node, ());
  TEST_EQUAL(node->GetData().m_depth, tree_node::GetDepth(node), ());
  TEST_EQUAL(node->GetParent()->GetData().m_id, MakeId(9223372036879747192ull, 9223372036879747192ull), ());
  TEST_EQUAL(node->GetChildren().size(), 0, ());
}
}  // namespace hierarchy_entry_tests
