#include "generator/geo_objects/streets_builder.hpp"

#include "indexer/classificator.hpp"

#include "base/logging.hpp"

#include <utility>

#include "3party/jansson/myjansson.hpp"

namespace generator
{
namespace geo_objects
{
StreetsBuilder::StreetsBuilder(RegionInfoGetter const & regionInfoGetter)
    : m_regionInfoGetter{regionInfoGetter}
{ }

void StreetsBuilder::Build(std::string const & pathInGeoObjectsTmpMwm, std::ostream & streamGeoObjectsKv)
{
  auto const transform = [this, &streamGeoObjectsKv](FeatureBuilder1 & fb, uint64_t /* currPos */) {
    if (!IsStreet(fb))
      return;

    auto const region = FindStreetRegionOwner(fb);
    if (!region)
      return;

    if (!InsertStreet(*region, fb))
      return;

    auto const value = MakeStreetValue(fb, *region);
    auto const id = static_cast<int64_t>(fb.GetMostGenericOsmId().GetEncodedId());
    streamGeoObjectsKv << id << " " << value.get() << "\n";
  };

  feature::ForEachFromDatRawFormat(pathInGeoObjectsTmpMwm, transform);
}

boost::optional<KeyValue> StreetsBuilder::FindStreetRegionOwner(FeatureBuilder1 & fb)
{
  auto const & line = fb.GetOuterGeometry();
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
  auto const isStreetAdministrator = [] (base::Json const & region) {
    auto const && properties = base::GetJSONObligatoryField(region.get(), "properties");
    auto const && address = base::GetJSONObligatoryField(properties, "address");

    if (base::GetJSONOptionalField(address, "suburb"))
      return false;
    if (base::GetJSONOptionalField(address, "sublocality"))
      return false;

    return true;
  };

  return m_regionInfoGetter.FindDeepest(point, isStreetAdministrator);
}

bool StreetsBuilder::InsertStreet(KeyValue const & region, FeatureBuilder1 & fb)
{
  auto & regionStreets = m_regionsStreets[region.first];
  auto emplace = regionStreets.emplace(fb.GetName());
  return emplace.second;
}

std::unique_ptr<char, JSONFreeDeleter> StreetsBuilder::MakeStreetValue(
    FeatureBuilder1 const & fb, KeyValue const & region)
{
  auto const && regionProperties = base::GetJSONObligatoryField(region.second.get(), "properties");
  auto const && regionAddress = base::GetJSONObligatoryField(regionProperties, "address");
  auto address = base::JSONPtr{json_deep_copy(regionAddress)};
  ToJSONObject(*address, "street", fb.GetName());

  auto properties = base::NewJSONObject();
  ToJSONObject(*properties, "address", std::move(address));
  ToJSONObject(*properties, "name", fb.GetName());
  ToJSONObject(*properties, "pid", region.first);

  auto streetObject = base::NewJSONObject();
  ToJSONObject(*streetObject, "properties", std::move(properties));

  auto const value = json_dumps(streetObject.get(), JSON_COMPACT);
  return std::unique_ptr<char, JSONFreeDeleter>{value};
}

// static
bool StreetsBuilder::IsStreet(OsmElement const & element)
{
  if (!element.IsWay() && !element.IsRelation())
    return false;

  auto const & tags = element.Tags();

  auto const isHighway = std::any_of(std::cbegin(tags), std::cend(tags), [] (auto const & tag) {
    return tag.key == "highway";
  });
  if (!isHighway)
    return false;

  auto const hasName = std::any_of(std::cbegin(tags), std::cend(tags), [] (auto const & tag) {
    return tag.key == "name";
  });
  if (!hasName)
    return false;

  return true;
}

// static
bool StreetsBuilder::IsStreet(FeatureBuilder1 const & fb)
{
  if (!fb.IsLine() && !fb.IsArea())
    return false;

  static auto const highwayType = classif().GetTypeByPath({"highway"});
  if (fb.FindType(highwayType, 1) == ftype::GetEmptyValue())
    return false;

  return !fb.GetName().empty();
}
}  // namespace geo_objects
}  // namespace generator
