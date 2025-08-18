#include "testing/testing.hpp"

#include "generator/complex_loader.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/hierarchy_entry.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/complex/tree_node.hpp"

#include "coding/csv_reader.hpp"

#include "base/geo_object_id.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

namespace complex_loader_tests
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
    "Russia_Moscow\n"

    "13835058055283526046 9223372037165115538;"
    ";"
    "1;"
    "37.6112346;"
    "67.4426053;"
    "place-square;"
    "Манежная площадь;"
    "Russia_Moscow\n"

    "4611686023709502091 4611686023709502091;"
    "13835058055283526046 9223372037165115538;"
    "2;"
    "37.6112346;"
    "67.4426053;"
    "place-square;"
    "Манежная площадь;"
    "Russia_Moscow\n"

    "4611686024983153989 4611686024983153989;"
    "13835058055283526046 9223372037165115538;"
    "2;"
    "37.6112346;"
    "67.4426053;"
    "amenity-cafe;"
    "ShakeUp;"
    "Russia_Moscow\n";

void SortForest(tree_node::types::Ptrs<generator::HierarchyEntry> & forest)
{
  std::sort(std::begin(forest), std::end(forest),
            [](auto const & lhs, auto const & rhs) { return lhs->GetData().m_id < rhs->GetData().m_id; });
}

UNIT_CLASS_TEST(TestWithClassificator, Complex_IsComplex)
{
  auto const filename = "test.csv";
  ScopedFile sf(filename, kCsv1);
  auto forest = generator::hierarchy::LoadHierachy(sf.GetFullPath());
  // We need to sort forest, because LoadHierachy() returns forest, where trees aren't ordered.
  SortForest(forest);
  TEST_EQUAL(forest.size(), 2, ());
  TEST(!generator::IsComplex(forest[0]), ());
  TEST(generator::IsComplex(forest[1]), ());
}

UNIT_CLASS_TEST(TestWithClassificator, Complex_GetCountry)
{
  auto const filename = "test.csv";
  ScopedFile sf(filename, kCsv1);
  auto forest = generator::hierarchy::LoadHierachy(sf.GetFullPath());
  // We need to sort forest, because LoadHierachy() returns forest, where trees aren't ordered.
  SortForest(forest);
  TEST_EQUAL(forest.size(), 2, ());
  TEST_EQUAL(generator::GetCountry(forest[0]), "Russia_Moscow", ());
  TEST_EQUAL(generator::GetCountry(forest[1]), "Russia_Moscow", ());
}

UNIT_CLASS_TEST(TestWithClassificator, Complex_ComplexLoader)
{
  auto const filename = "test.csv";
  ScopedFile sf(filename, kCsv1);
  generator::ComplexLoader const loader(sf.GetFullPath());
  auto const forest = loader.GetForest("Russia_Moscow");
  TEST_EQUAL(forest.Size(), 1, ());
  forest.ForEachTree([](auto const & tree) { TEST_EQUAL(tree_node::Size(tree), 7, ()); });
}

UNIT_CLASS_TEST(TestWithClassificator, Complex_GetOrCreateComplexLoader)
{
  auto const filename = "test.csv";
  ScopedFile sf(filename, kCsv1);
  auto const & loader = generator::GetOrCreateComplexLoader(sf.GetFullPath());
  auto const forest = loader.GetForest("Russia_Moscow");
  TEST_EQUAL(forest.Size(), 1, ());
  forest.ForEachTree([](auto const & tree) { TEST_EQUAL(tree_node::Size(tree), 7, ()); });
}
}  // namespace complex_loader_tests
