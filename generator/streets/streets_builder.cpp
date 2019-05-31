#include "generator/streets/streets_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"

#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace feature;

namespace generator
{
namespace streets
{
StreetsBuilder::StreetsBuilder(regions::RegionInfoGetter const & regionInfoGetter)
    : m_regionInfoGetter{regionInfoGetter}
{ }

void StreetsBuilder::AssembleStreets(std::string const & pathInStreetsTmpMwm)
{
  auto const transform = [this](FeatureBuilder & fb, uint64_t /* currPos */) {
    AddStreet(fb);
  };
  ForEachFromDatRawFormat(pathInStreetsTmpMwm, transform);
}

void StreetsBuilder::AssembleBindings(std::string const & pathInGeoObjectsTmpMwm)
{
  auto const transform = [this](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto streetName = fb.GetParams().GetStreet();
    if (!streetName.empty())
      AddStreetBinding(std::move(streetName), fb);
  };
  ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, transform);
}

void StreetsBuilder::SaveStreetsKv(std::ostream & streamStreetsKv)
{
  for (auto const & region : m_regions)
    SaveRegionStreetsKv(streamStreetsKv, region.first, region.second);
}

void StreetsBuilder::SaveRegionStreetsKv(std::ostream & streamStreetsKv, uint64_t regionId,
                                         RegionStreets const & streets)
{
  auto const & regionsStorage = m_regionInfoGetter.GetStorage();
  auto const && regionObject = regionsStorage.Find(regionId);
  ASSERT(regionObject, ());

  for (auto const & street : streets)
  {
    auto streetId = street.second;
    if (base::GeoObjectId::Type::Invalid == streetId.GetType())
      streetId = NextOsmSurrogateId();

    auto const id = static_cast<int64_t>(streetId.GetEncodedId());
    auto const value = MakeStreetValue(regionId, *regionObject, street.first);
    streamStreetsKv << id << " " << value.get() << "\n";
  }
}

void StreetsBuilder::AddStreet(FeatureBuilder & fb)
{
  auto const region = FindStreetRegionOwner(fb);
  if (!region)
    return;

  InsertStreet(*region, fb.GetName(), fb.GetMostGenericOsmId());
}

void StreetsBuilder::AddStreetBinding(std::string && streetName, FeatureBuilder & fb)
{
  auto const region = FindStreetRegionOwner(fb.GetKeyPoint());
  if (!region)
    return;

  InsertSurrogateStreet(*region, std::move(streetName));
}

boost::optional<KeyValue> StreetsBuilder::FindStreetRegionOwner(FeatureBuilder & fb)
{
  if (fb.IsPoint())
    return FindStreetRegionOwner(fb.GetKeyPoint());

  auto const & line = fb.GetOuterGeometry();
  CHECK_GREATER_OR_EQUAL(line.size(), 2, ());
  auto const & startPoint = line.front();
  auto const & owner = FindStreetRegionOwner(startPoint);
  if (!owner)
    return {};

  auto const & finishPoint = line.back();
  if (startPoint != finishPoint)
  {
    auto const & finishPointOwner = FindStreetRegionOwner(finishPoint);
    if (!finishPointOwner || finishPointOwner->first != owner->first)
      LOG(LDEBUG, ("Street", fb.GetMostGenericOsmId(), fb.GetName(), "is in several regions"));
  }

  return owner;
}

boost::optional<KeyValue> StreetsBuilder::FindStreetRegionOwner(m2::PointD const & point)
{
  auto const isStreetAdministrator = [] (KeyValue const & region) {
    auto const && properties = base::GetJSONObligatoryField(*region.second, "properties");
    auto const && address = base::GetJSONObligatoryField(properties, "address");

    if (base::GetJSONOptionalField(address, "suburb"))
      return false;
    if (base::GetJSONOptionalField(address, "sublocality"))
      return false;

    return true;
  };

  return m_regionInfoGetter.FindDeepest(point, isStreetAdministrator);
}

bool StreetsBuilder::InsertStreet(KeyValue const & region, std::string && streetName, base::GeoObjectId id)
{
  auto & regionStreets = m_regions[region.first];
  auto emplace = regionStreets.emplace(std::move(streetName), id);

  if (!emplace.second && base::GeoObjectId::Type::Invalid == emplace.first->second.GetType())
    emplace.first->second = id;

  return emplace.second;
}

bool StreetsBuilder::InsertSurrogateStreet(KeyValue const & region, std::string && streetName)
{
  auto & regionStreets = m_regions[region.first];
  auto emplace = regionStreets.emplace(std::move(streetName), base::GeoObjectId{});
  return emplace.second;
}

std::unique_ptr<char, JSONFreeDeleter> StreetsBuilder::MakeStreetValue(
    uint64_t regionId, JsonValue const & regionObject, std::string const & streetName)
{
  auto const && regionProperties = base::GetJSONObligatoryField(regionObject, "properties");
  auto const && regionAddress = base::GetJSONObligatoryField(regionProperties, "address");
  auto address = base::JSONPtr{json_deep_copy(const_cast<json_t *>(regionAddress))};
  ToJSONObject(*address, "street", streetName);

  auto properties = base::NewJSONObject();
  ToJSONObject(*properties, "address", std::move(address));
  ToJSONObject(*properties, "name", streetName);
  ToJSONObject(*properties, "pid", regionId);

  auto streetObject = base::NewJSONObject();
  ToJSONObject(*streetObject, "properties", std::move(properties));

  auto const value = json_dumps(streetObject.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>{value};
}

base::GeoObjectId StreetsBuilder::NextOsmSurrogateId()
{
  return base::GeoObjectId{base::GeoObjectId::Type::OsmSurrogate, ++m_osmSurrogateCounter};
}

// static
bool StreetsBuilder::IsStreet(OsmElement const & element)
{
  if (element.GetTagValue("name", {}).empty())
    return false;

  if (element.HasTag("highway") && (element.IsWay() || element.IsRelation()))
    return true;

  if (element.HasTag("place", "square"))
    return true;

  return false;
}

// static
bool StreetsBuilder::IsStreet(FeatureBuilder const & fb)
{
  if (fb.GetName().empty())
    return false;

  auto const & wayChecker = ftypes::IsWayChecker::Instance();
  if (wayChecker(fb.GetTypes()) && (fb.IsLine() || fb.IsArea()))
    return true;

  auto const & squareChecker = ftypes::IsSquareChecker::Instance();
  if (squareChecker(fb.GetTypes()))
    return true;

  return false;
}
}  // namespace streets
}  // namespace generator
