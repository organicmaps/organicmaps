#include "generator/place_processor.hpp"

#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"

using namespace feature;

namespace generator
{
namespace
{
std::shared_ptr<OsmIdToBoundariesTable>
GetOrCreateBoundariesTable(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable)
{
  return boundariesTable ? boundariesTable : std::make_shared<OsmIdToBoundariesTable>();
}
}  // namespace

PlaceProcessor::PlaceProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable)
  : m_boundariesTable(GetOrCreateBoundariesTable(boundariesTable))
{
  ASSERT(m_boundariesTable.get(), ());
}

void PlaceProcessor::UnionEqualPlacesIds(Place const & place)
{
  auto const id = place.GetFeature().GetLastOsmId();
  m_places.ForEachInRect(place.GetLimitRect(), [&](Place const & p) {
    if (p.IsEqual(place))
      m_boundariesTable->Union(p.GetFeature().GetLastOsmId(), id);
  });
}

std::vector<FeatureBuilder> PlaceProcessor::GetFeatures() const
{
  std::vector<FeatureBuilder> result;
  m_places.ForEach([&result](Place const & p) {
    result.emplace_back(p.GetFeature());
  });

  return result;
}

void PlaceProcessor::Add(FeatureBuilder const & fb)
{
  auto const type = GetPlaceType(fb);
  if (type == ftype::GetEmptyValue())
    return;

  auto const id = fb.GetLastOsmId();
  m_boundariesTable->Append(id, indexer::CityBoundary(fb.GetOuterGeometry()));
  UnionEqualPlacesIds(Place(fb, type, false /* saveParams */));
}

void PlaceProcessor::TryUpdate(FeatureBuilder const & fb)
{
  auto const type = GetPlaceType(fb);
  Place const place(fb, type);
  UnionEqualPlacesIds(place);
  m_places.ReplaceEqualInRect(place, [](Place const & p1, Place const & p2) {
    return p1.IsEqual(p2);
  }, [](Place const & p1, Place const & p2) {
    return p1.IsBetterThan(p2);
  });
}
}  // namespace generator
