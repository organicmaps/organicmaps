#include "testing/testing.hpp"

#include "generator/generator_tests/common.hpp"

#include "generator/collector_collection.hpp"
#include "generator/collector_interface.hpp"
#include "generator/collector_tag.hpp"

#include "platform/platform.hpp"

#include "base/geo_object_id.hpp"

#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace base;
using namespace generator;
using namespace generator_tests;

namespace
{
auto const kEmptyValidator = [](auto const &) { return true; };
}  // namespace

UNIT_TEST(MergeCollector_MergeCase1)
{
  auto const filename = GetFileName();
  std::string const tagKey = "admin_level";
  auto collector1 = std::make_shared<CollectorTag>(filename, tagKey, kEmptyValidator);

  collector1->Collect(MakeOsmElement(1 /* id */, {{"admin_level", "1"}}, OsmElement::EntityType::Relation));
  collector1->Collect(MakeOsmElement(2 /* id */, {{"admin_level", "2"}}, OsmElement::EntityType::Relation));

  auto collector2 = collector1->Clone();

  collector2->Collect(MakeOsmElement(3 /* id */, {{"admin_level", "3"}}, OsmElement::EntityType::Relation));
  collector2->Collect(MakeOsmElement(4 /* id */, {{"admin_level", "4"}}, OsmElement::EntityType::Relation));

  collector1->Finish();
  collector2->Finish();
  collector1->Merge(*collector2);
  collector1->Finalize();

  std::vector<std::string> const answers = {
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 1 /* id */).GetEncodedId()) + "\t1",
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 2 /* id */).GetEncodedId()) + "\t2",
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 3 /* id */).GetEncodedId()) + "\t3",
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 4 /* id */).GetEncodedId()) + "\t4",
  };

  std::ifstream stream;
  stream.exceptions(std::ios::badbit);
  stream.open(filename);
  size_t pos = 0;
  std::string line;
  while (std::getline(stream, line))
    TEST_EQUAL(line, answers[pos++], ());
}

UNIT_TEST(MergeCollector_MergeCase2)
{
  auto const filename = GetFileName();
  std::string const tagKey = "admin_level";
  auto collection1 = std::make_shared<CollectorCollection>();
  collection1->Append(std::make_shared<CollectorTag>(filename, tagKey, kEmptyValidator));

  collection1->Collect(MakeOsmElement(1 /* id */, {{"admin_level", "1"}}, OsmElement::EntityType::Relation));
  collection1->Collect(MakeOsmElement(2 /* id */, {{"admin_level", "2"}}, OsmElement::EntityType::Relation));

  auto collection2 = collection1->Clone();

  collection2->Collect(MakeOsmElement(3 /* id */, {{"admin_level", "3"}}, OsmElement::EntityType::Relation));
  collection2->Collect(MakeOsmElement(4 /* id */, {{"admin_level", "4"}}, OsmElement::EntityType::Relation));

  collection1->Finish();
  collection2->Finish();
  collection1->Merge(*collection2);
  collection1->Finalize();

  std::vector<std::string> const answers = {
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 1 /* id */).GetEncodedId()) + "\t1",
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 2 /* id */).GetEncodedId()) + "\t2",
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 3 /* id */).GetEncodedId()) + "\t3",
      std::to_string(GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 4 /* id */).GetEncodedId()) + "\t4",
  };

  std::ifstream stream;
  stream.exceptions(std::ios::badbit);
  stream.open(filename);
  size_t pos = 0;
  std::string line;
  while (std::getline(stream, line))
    TEST_EQUAL(line, answers[pos++], ());
}
