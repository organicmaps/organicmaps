#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include <iostream>
#include <set>
#include <string>

constexpr int32_t kRoundDigits = 1e6;
std::set<std::string> const kPoiTypes = {"amenity",  "shop",    "tourism",  "leisure",   "sport",
                                         "craft",    "man_made", "office",  "historic",  "building"};

std::string GetReadableType(FeatureBuilder1 const & f)
{
  uint32_t result = 0;
  for (uint32_t type : f.GetTypes())
  {
    std::string fullName = classif().GetFullObjectName(type);
    auto const pos = fullName.find("|");
    if (pos != std::string::npos)
      fullName = fullName.substr(0, pos);
    if (kPoiTypes.find(fullName) != kPoiTypes.end())
      result = type;
  };
  return result == 0 ? std::string() : classif().GetReadableObjectName(result);
}

void PrintFeature(FeatureBuilder1 const & fb, uint64_t)
{
  std::string const & category = GetReadableType(fb);
  std::string const & name = fb.GetName();
  std::string const & street = fb.GetParams().GetStreet();
  std::string const & house = fb.GetParams().house.Get();
  bool const isPOI = !name.empty() && !category.empty() &&
      category.find("building") == std::string::npos;

  if ((house.empty() && !isPOI) || fb.GetGeomType() == feature::GEOM_LINE)
    return;

  auto const center = MercatorBounds::ToLatLon(fb.GetKeyPoint());
  auto coordinates = my::NewJSONArray();
  ToJSONArray(*coordinates, std::round(center.lon * kRoundDigits) / kRoundDigits);
  ToJSONArray(*coordinates, std::round(center.lat * kRoundDigits) / kRoundDigits);
  auto geometry = my::NewJSONObject();
  ToJSONObject(*geometry, "type", "Point");
  ToJSONObject(*geometry, "coordinates", coordinates);

  auto properties = my::NewJSONObject();
  ToJSONObject(*properties, "id", fb.GetMostGenericOsmId().GetEncodedId());
  if (!name.empty() && !category.empty() && category != "building-address")
  {
    ToJSONObject(*properties, "name", name);
    ToJSONObject(*properties, "tags", category);
  }
  auto address = my::NewJSONObject();
  if (!street.empty())
    ToJSONObject(*address, "street", street);
  if (!house.empty())
    ToJSONObject(*address, "building", house);
  ToJSONObject(*properties, "address", address);

  auto feature = my::NewJSONObject();
  ToJSONObject(*feature, "type", "Feature");
  ToJSONObject(*feature, "geometry", geometry);
  ToJSONObject(*feature, "properties", properties);

  std::cout << json_dumps(feature.release(), JSON_COMPACT) << std::endl;
}

int main(int argc, char * argv[])
{
  if (argc <= 1)
  {
    LOG(LERROR, ("Usage:", argc == 1 ? argv[0] : "extract_addr",
                 "<mwm_tmp_name> [<data_path>]"));
    return 1;
  }

  Platform & pl = GetPlatform();
  if (argc > 2)
    pl.SetResourceDir(argv[2]);
  classificator::Load();

  feature::ForEachFromDatRawFormat(argv[1], PrintFeature);

  return 0;
}
