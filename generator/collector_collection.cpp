#include "generator/collector_collection.hpp"

#include "generator/feature_builder.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

using namespace feature;

namespace generator
{
std::shared_ptr<CollectorInterface> CollectorCollection::Clone(
    std::shared_ptr<cache::IntermediateDataReaderInterface> const & cache) const
{
  auto p = std::make_shared<CollectorCollection>();
  for (auto const & c : m_collection)
    p->Append(c->Clone(cache));
  return p;
}

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

void CollectorCollection::CollectFeature(FeatureBuilder const & feature, OsmElement const & element)
{
  for (auto & c : m_collection)
    c->CollectFeature(feature, element);
}

void CollectorCollection::Finish()
{
  for (auto & c : m_collection)
    c->Finish();
}

void CollectorCollection::Save()
{
  for (auto & c : m_collection)
    c->Save();
}

void CollectorCollection::OrderCollectedData()
{
  for (auto & c : m_collection)
    c->OrderCollectedData();
}

void CollectorCollection::Merge(CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void CollectorCollection::MergeInto(CollectorCollection & collector) const
{
  auto & otherCollection = collector.m_collection;
  CHECK_EQUAL(m_collection.size(), otherCollection.size(), ());
  for (size_t i = 0; i < m_collection.size(); ++i)
    otherCollection[i]->Merge(*m_collection[i]);
}
}  // namespace generator
