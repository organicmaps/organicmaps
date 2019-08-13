#include "generator/feature_maker_base.hpp"

#include "generator/osm_element.hpp"

#include "base/assert.hpp"

#include <utility>

using namespace feature;

namespace generator
{
FeatureMakerBase::FeatureMakerBase(std::shared_ptr<cache::IntermediateData> const & cache)
  : m_cache(cache) {}

void FeatureMakerBase::SetCache(std::shared_ptr<cache::IntermediateData> const & cache)
{
  m_cache = cache;
}

bool FeatureMakerBase::Add(OsmElement & element)
{
  ASSERT(m_cache, ());

  FeatureParams params;
  ParseParams(params, element);
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node:
    return BuildFromNode(element, params);
  case OsmElement::EntityType::Way:
    return BuildFromWay(element, params);
  case OsmElement::EntityType::Relation:
    return BuildFromRelation(element, params);
  default:
    return false;
  }
}

size_t FeatureMakerBase::Size() const
{
  return m_queue.size();
}

bool FeatureMakerBase::Empty() const
{
  return m_queue.empty();
}

bool FeatureMakerBase::GetNextFeature(FeatureBuilder & feature)
{
  if (m_queue.empty())
    return false;

  feature = std::move(m_queue.front());
  m_queue.pop();
  return true;
}

void TransformAreaToPoint(FeatureBuilder & feature)
{
  CHECK(feature.IsArea(), ());
  auto const center = feature.GetGeometryCenter();
  auto const id = feature.GetLastOsmId();
  feature.ResetGeometry();
  feature.SetOsmId(id);
  feature.SetCenter(center);
}

void TransformAreaToLine(FeatureBuilder & feature)
{
  CHECK(feature.IsArea(), ());
  feature.SetLinear(feature.GetParams().m_reverseGeometry);
}

FeatureBuilder MakePointFromArea(FeatureBuilder const & feature)
{
  FeatureBuilder tmp(feature);
  TransformAreaToPoint(tmp);
  return tmp;
}

FeatureBuilder MakeLineFromArea(FeatureBuilder const & feature)
{
  FeatureBuilder tmp(feature);
  TransformAreaToLine(tmp);
  return tmp;
}
}  // namespace generator
