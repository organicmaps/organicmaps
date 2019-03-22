#include "generator/collector_collection.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

namespace generator
{
void CollectorCollection::Collect(OsmElement const & element)
{
  for (auto & c : m_collection)
    c->Collect(element);
}

void CollectorCollection::CollectRelation(RelationElement const & element)
{
  for (auto & c : m_collection)
    c->CollectRelation(element);
}

void CollectorCollection::CollectFeature(FeatureBuilder1 const & feature, OsmElement const & element)
{
  for (auto & c : m_collection)
    c->CollectFeature(feature, element);
}

void CollectorCollection::Save()
{
  for (auto & c : m_collection)
    c->Save();
}
}  // namespace generator
