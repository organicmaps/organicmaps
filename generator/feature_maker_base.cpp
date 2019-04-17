#include "generator/feature_maker_base.hpp"

#include "generator/osm_element.hpp"

#include "base/assert.hpp"

#include <utility>

namespace generator
{
FeatureMakerBase::FeatureMakerBase(cache::IntermediateDataReader & cache) : m_cache(cache) {}

bool FeatureMakerBase::Add(OsmElement & element)
{
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

bool FeatureMakerBase::GetNextFeature(FeatureBuilder1 & feature)
{
  if (m_queue.empty())
    return false;

  feature = std::move(m_queue.front());
  m_queue.pop();
  return true;
}


void TransformAreaToPoint(FeatureBuilder1 & feature)
{
  CHECK(feature.IsArea(), ());
  auto const center = feature.GetGeometryCenter();
  auto const id = feature.GetLastOsmId();
  feature.ResetGeometry();
  feature.SetOsmId(id);
  feature.SetCenter(center);
}

void TransformAreaToLine(FeatureBuilder1 & feature)
{
  CHECK(feature.IsArea(), ());
  feature.SetLinear(feature.GetParams().m_reverseGeometry);
}

FeatureBuilder1 MakePointFromArea(FeatureBuilder1 const & feature)
{
  FeatureBuilder1 tmp(feature);
  TransformAreaToPoint(tmp);
  return tmp;
}

FeatureBuilder1 MakeLineFromArea(FeatureBuilder1 const & feature)
{
  FeatureBuilder1 tmp(feature);
  TransformAreaToLine(tmp);
  return tmp;
}
}  // namespace generator
