#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <string>

constexpr int32_t kRoundDigits = 1e6;

std::string GetReadableType(FeatureBuilder1 const & f)
{
  auto const isPoiOrBuilding = [](uint32_t type) {
    auto const & poiChecker = ftypes::IsPoiChecker::Instance();
    auto const & buildingChecker = ftypes::IsBuildingChecker::Instance();
    return poiChecker(type) || buildingChecker(type);
  };

  std::string result;
  auto const & types = f.GetTypes();
  auto const it = std::find_if(std::begin(types), std::end(types), isPoiOrBuilding);
  if (it != std::end(types))
    result = classif().GetReadableObjectName(*it);

  return result;
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
  auto coordinates = base::NewJSONArray();
  ToJSONArray(*coordinates, std::round(center.lon * kRoundDigits) / kRoundDigits);
  ToJSONArray(*coordinates, std::round(center.lat * kRoundDigits) / kRoundDigits);
  auto geometry = base::NewJSONObject();
  ToJSONObject(*geometry, "type", "Point");
  ToJSONObject(*geometry, "coordinates", coordinates);

  auto properties = base::NewJSONObject();
  ToJSONObject(*properties, "id", fb.GetMostGenericOsmId().GetEncodedId());
  if (!name.empty() && !category.empty() && category != "building-address")
  {
    ToJSONObject(*properties, "name", name);
    ToJSONObject(*properties, "tags", category);
  }
  auto address = base::NewJSONObject();
  if (!street.empty())
    ToJSONObject(*address, "street", street);
  if (!house.empty())
    ToJSONObject(*address, "building", house);
  ToJSONObject(*properties, "address", address);

  auto feature = base::NewJSONObject();
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
