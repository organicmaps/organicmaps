#include "generator/city_boundary_processor.hpp"

#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

#include "base/assert.hpp"

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

CityBoundaryProcessor::CityBoundaryProcessor(std::shared_ptr<OsmIdToBoundariesTable> boundariesTable)
  : m_boundariesTable(GetOrCreateBoundariesTable(boundariesTable))
{
  ASSERT(m_boundariesTable.get(), ());
}

void CityBoundaryProcessor::UnionEqualPlacesIds(Place const & place)
{
  auto const id = place.GetFeature().GetLastOsmId();
  m_places.ForEachInRect(place.GetLimitRect(), [&](Place const & p) {
    if (p.IsEqual(place))
      m_boundariesTable->Union(p.GetFeature().GetLastOsmId(), id);
  });
}

std::vector<FeatureBuilder1> CityBoundaryProcessor::GetFeatures() const
{
  std::vector<FeatureBuilder1> result;
  m_places.ForEach([&result](Place const & p) {
    result.emplace_back(p.GetFeature());
  });

  return result;
}

void CityBoundaryProcessor::Add(FeatureBuilder1 const & fb)
{
  auto const type = GetPlaceType(fb);
  if (type == ftype::GetEmptyValue())
    return;

  auto const id = fb.GetLastOsmId();
  m_boundariesTable->Append(id, indexer::CityBoundary(fb.GetOuterGeometry()));
  UnionEqualPlacesIds(Place(fb, type, false /* saveParams */));
}

void CityBoundaryProcessor::Replace(FeatureBuilder1 const & fb)
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
