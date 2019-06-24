#include "generator/streets/streets_builder.hpp"
#include "generator/streets/street_regions_tracing.hpp"
#include "generator/to_string_policy.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <utility>

#include "3party/jansson/myjansson.hpp"

using namespace feature;

namespace generator
{
namespace streets
{
StreetsBuilder::StreetsBuilder(regions::RegionInfoGetter const & regionInfoGetter,
                               size_t threadsCount)
  : m_regionInfoGetter{regionInfoGetter}, m_threadsCount{threadsCount}
{
}

void StreetsBuilder::AssembleStreets(std::string const & pathInStreetsTmpMwm)
{
  auto const transform = [this](FeatureBuilder & fb, uint64_t /* currPos */) { AddStreet(fb); };
  ForEachParallelFromDatRawFormat(m_threadsCount, pathInStreetsTmpMwm, transform);
}

void StreetsBuilder::AssembleBindings(std::string const & pathInGeoObjectsTmpMwm)
{
  auto const transform = [this](FeatureBuilder & fb, uint64_t /* currPos */) {
    auto streetName = fb.GetParams().GetStreet();
    if (!streetName.empty())
      AddStreetBinding(std::move(streetName), fb);
  };
  ForEachParallelFromDatRawFormat(m_threadsCount, pathInGeoObjectsTmpMwm, transform);
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
    auto const & bbox = street.second.GetBbox();
    auto const & pin = street.second.GetOrChoosePin();

    auto const id = static_cast<int64_t>(pin.m_osmId.GetEncodedId());
    auto const & value =
        MakeStreetValue(regionId, *regionObject, street.first, bbox, pin.m_position);
    streamStreetsKv << id << " " << value << "\n";
  }
}

void StreetsBuilder::AddStreet(FeatureBuilder & fb)
{
  if (fb.IsArea())
    return AddStreetArea(fb);

  if (fb.IsPoint())
    return AddStreetPoint(fb);

  CHECK(fb.IsLine(), ());
  AddStreetHighway(fb);
}

void StreetsBuilder::AddStreetHighway(FeatureBuilder & fb)
{
  auto streetRegionInfoGetter = [this](auto const & pathPoint) {
    return this->FindStreetRegionOwner(pathPoint);
  };
  StreetRegionsTracing regionsTracing(fb.GetOuterGeometry(), streetRegionInfoGetter);

  std::lock_guard<std::mutex> lock{m_updateMutex};

  auto && pathSegments = regionsTracing.StealPathSegments();
  for (auto & segment : pathSegments)
  {
    auto && region = segment.m_region;
    auto & street = InsertStreet(region.first, fb.GetName());
    auto const osmId = pathSegments.size() == 1 ? fb.GetMostGenericOsmId() : NextOsmSurrogateId();
    street.AddHighwayLine(osmId, std::move(segment.m_path));
  }
}

void StreetsBuilder::AddStreetArea(FeatureBuilder & fb)
{
  auto && region = FindStreetRegionOwner(fb.GetGeometryCenter(), true);
  if (!region)
    return;

  std::lock_guard<std::mutex> lock{m_updateMutex};

  auto & street = InsertStreet(region->first, fb.GetName());
  auto osmId = fb.GetMostGenericOsmId();
  street.AddHighwayArea(osmId, fb.GetOuterGeometry());
}

void StreetsBuilder::AddStreetPoint(FeatureBuilder & fb)
{
  auto && region = FindStreetRegionOwner(fb.GetKeyPoint(), true);
  if (!region)
    return;

  std::lock_guard<std::mutex> lock{m_updateMutex};

  auto osmId = fb.GetMostGenericOsmId();
  auto & street = InsertStreet(region->first, fb.GetName());
  street.SetPin({fb.GetKeyPoint(), osmId});
}

void StreetsBuilder::AddStreetBinding(std::string && streetName, FeatureBuilder & fb)
{
  auto const region = FindStreetRegionOwner(fb.GetKeyPoint());
  if (!region)
    return;

  std::lock_guard<std::mutex> lock{m_updateMutex};

  auto & street = InsertStreet(region->first, std::move(streetName));
  street.AddBinding(NextOsmSurrogateId(), fb.GetKeyPoint());
}

boost::optional<KeyValue> StreetsBuilder::FindStreetRegionOwner(m2::PointD const & point,
                                                                bool needLocality)
{
  auto const isStreetAdministrator = [needLocality](KeyValue const & region) {
    auto const && properties = base::GetJSONObligatoryField(*region.second, "properties");
    auto const && address = base::GetJSONObligatoryField(properties, "address");

    if (base::GetJSONOptionalField(address, "suburb"))
      return false;
    if (base::GetJSONOptionalField(address, "sublocality"))
      return false;

    if (needLocality && !base::GetJSONOptionalField(address, "locality"))
      return false;

    return true;
  };

  return m_regionInfoGetter.FindDeepest(point, isStreetAdministrator);
}

StreetGeometry & StreetsBuilder::InsertStreet(uint64_t regionId, std::string && streetName)
{
  auto & regionStreets = m_regions[regionId];
  return regionStreets[std::move(streetName)];
}

std::string StreetsBuilder::MakeStreetValue(uint64_t regionId, JsonValue const & regionObject,
                                            std::string const & streetName, m2::RectD const & bbox,
                                            m2::PointD const & pinPoint)
{
  auto streetObject = base::NewJSONObject();

  auto const && regionProperties = base::GetJSONObligatoryField(regionObject, "properties");
  auto const && regionAddress = base::GetJSONObligatoryField(regionProperties, "address");
  auto address = base::JSONPtr{json_deep_copy(const_cast<json_t *>(regionAddress))};
  ToJSONObject(*address, "street", streetName);

  auto properties = base::NewJSONObject();
  ToJSONObject(*properties, "address", std::move(address));
  ToJSONObject(*properties, "name", streetName);
  ToJSONObject(*properties, "pid", regionId);
  ToJSONObject(*streetObject, "properties", std::move(properties));

  auto const & leftBottom = MercatorBounds::ToLatLon(bbox.LeftBottom());
  auto const & rightTop = MercatorBounds::ToLatLon(bbox.RightTop());
  auto const & bboxArray =
      std::vector<double>{leftBottom.m_lon, leftBottom.m_lat, rightTop.m_lon, rightTop.m_lat};
  ToJSONObject(*streetObject, "bbox", std::move(bboxArray));

  auto const & pinLatLon = MercatorBounds::ToLatLon(pinPoint);
  auto const & pinArray = std::vector<double>{pinLatLon.m_lon, pinLatLon.m_lat};
  ToJSONObject(*streetObject, "pin", std::move(pinArray));

  return base::DumpToString(streetObject,
                            JSON_REAL_PRECISION(JsonPolicy::kDefaultPrecision) | JSON_COMPACT);
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
