#include "testing/testing.hpp"

#include "generator/brands_loader.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/geo_object_id.hpp"

#include <string>
#include <unordered_map>

namespace brands_loader_test
{
using namespace generator;
using base::GeoObjectId;
using platform::tests_support::ScopedFile;

char const kBrandsJson[] =
    "{\n"
    "\"nodes\": {\n"
    "\"2132500347\": 13,\n"
    "\"5321137826\": 12\n"
    "},\n"
    "\"ways\": {\n"
    "\"440527172\": 13,\n"
    "\"149816366\": 12\n"
    "},\n"
    "\"relations\": {\n"
    "\"6018309\": 13,\n"
    "\"6228042\": 12\n"
    "}\n"
    "}";

char const kBrandTranslationsJson[] =
    "{\n"
    "\"12\": {\n"
    "\"en\": [\"subway\"],\n"
    "\"ru\": [\"\u0441\u0430\u0431\u0432\u044d\u0439\",\n"
    "\"\u0441\u0430\u0431\u0432\u0435\u0439\"]\n"
    "},\n"
    "\"13\": {\n"
    "\"en\": [\"mcdonalds\",\"mc donalds\"],\n"
    "\"ru\": [\"\u043c\u0430\u043a\u0434\u043e\u043d\u0430\u043b\u044c\u0434\u0441\",\n"
    "\"\u043c\u0430\u043a\u0434\u043e\u043d\u0430\u043b\u0434\u0441\"]\n"
    "}\n"
    "}";

UNIT_TEST(LoadBrands)
{
  ScopedFile const brandsFile("brands.json", kBrandsJson);
  ScopedFile const translationsFile("translations.json", kBrandTranslationsJson);

  std::unordered_map<GeoObjectId, std::string> brands;
  TEST(LoadBrands(brandsFile.GetFullPath(), translationsFile.GetFullPath(), brands), ());
  TEST_EQUAL(brands[GeoObjectId(GeoObjectId::Type::ObsoleteOsmNode, 2132500347)], "mcdonalds", ());
  TEST_EQUAL(brands[GeoObjectId(GeoObjectId::Type::ObsoleteOsmWay, 440527172)], "mcdonalds", ());
  TEST_EQUAL(brands[GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 6018309)], "mcdonalds", ());
  TEST_EQUAL(brands[GeoObjectId(GeoObjectId::Type::ObsoleteOsmNode, 5321137826)], "subway", ());
  TEST_EQUAL(brands[GeoObjectId(GeoObjectId::Type::ObsoleteOsmWay, 149816366)], "subway", ());
  TEST_EQUAL(brands[GeoObjectId(GeoObjectId::Type::ObsoleteOsmRelation, 6228042)], "subway", ());
}
}  // namespace brands_loader_test
