#include "generator/feature_maker_base.hpp"

#include "generator/intermediate_data.hpp"
#include "generator/osm_element.hpp"

#include "base/assert.hpp"

namespace generator
{
using namespace feature;

bool FeatureMakerBase::Add(OsmElement & element)
{
  ASSERT(m_cache, ());

  FeatureBuilderParams params;
  ParseParams(params, element);
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node: return BuildFromNode(element, params);
  case OsmElement::EntityType::Way: return BuildFromWay(element, params);
  case OsmElement::EntityType::Relation: return BuildFromRelation(element, params);
  default: return false;
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

void TransformToPoint(FeatureBuilder & feature)
{
  if (feature.IsPoint())
    return;

  feature.SetCenter(feature.GetGeometryCenter());

  auto & params = feature.GetParams();
  if (!params.house.IsEmpty())
    params.SetGeomTypePointEx();
}

void TransformToLine(FeatureBuilder & feature)
{
  if (feature.IsLine())
    return;

  CHECK(feature.IsArea(), (feature));
  feature.SetLinear(feature.GetParams().GetReversedGeometry());
}

FeatureBuilder MakePoint(FeatureBuilder const & feature)
{
  FeatureBuilder tmp(feature);
  TransformToPoint(tmp);
  return tmp;
}

FeatureBuilder MakeLine(FeatureBuilder const & feature)
{
  FeatureBuilder tmp(feature);
  TransformToLine(tmp);
  return tmp;
}
}  // namespace generator
