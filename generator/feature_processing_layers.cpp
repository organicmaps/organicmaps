#include "generator/feature_processing_layers.hpp"

#include "generator/coastlines_generator.hpp"
#include "generator/feature_maker.hpp"
#include "generator/generate_info.hpp"
#include "generator/place_processor.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

using namespace feature;

namespace generator
{
namespace
{
void FixLandType(FeatureBuilder & fb)
{
  auto const & types = fb.GetTypes();
  auto const & isIslandChecker = ftypes::IsIslandChecker::Instance();
  auto const & isLandChecker = ftypes::IsLandChecker::Instance();
  auto const & isCoastlineChecker = ftypes::IsCoastlineChecker::Instance();
  if (isCoastlineChecker(types))
  {
    fb.PopExactType(isLandChecker.GetLandType());
    fb.PopExactType(isCoastlineChecker.GetCoastlineType());
  }
  else if (isIslandChecker(types) && fb.IsArea())
  {
    fb.AddType(isLandChecker.GetLandType());
  }
}
}  // namespace

std::string LogBuffer::GetAsString() const
{
  return m_buffer.str();
}

void LayerBase::Handle(FeatureBuilder & fb)
{
  if (m_next)
    m_next->Handle(fb);
}

void LayerBase::Merge(std::shared_ptr<LayerBase> const & other)
{
  CHECK(other, ());

  m_logBuffer.AppendLine(other->GetAsString());
}

void LayerBase::MergeChain(std::shared_ptr<LayerBase> const & other)
{
  CHECK_EQUAL(GetChainSize(), other->GetChainSize(), ());

  auto left = shared_from_this();
  auto right = other;
  while (left && right)
  {
    left->Merge(right);
    left = left->m_next;
    right = right->m_next;
  }
}

size_t LayerBase::GetChainSize() const
{
  size_t size = 0;
  auto current = shared_from_this();
  while (current)
  {
    ++size;
    current = current->m_next;
  }

  return size;
}

void LayerBase::SetNext(std::shared_ptr<LayerBase> next)
{
  m_next = next;
}

std::shared_ptr<LayerBase> LayerBase::Add(std::shared_ptr<LayerBase> next)
{
  if (m_next)
    m_next->Add(next);
  else
    m_next = next;

  return next;
}

std::string LayerBase::GetAsString() const
{
  return m_logBuffer.GetAsString();
}

std::string LayerBase::GetAsStringRecursive() const
{
  std::ostringstream buffer;
  auto temp = shared_from_this();
  while (temp)
  {
    buffer << temp->GetAsString();
    temp = temp->m_next;
  }

  return buffer.str();
}

void RepresentationLayer::Handle(FeatureBuilder & fb)
{
  auto const sourceType = fb.GetMostGenericOsmId().GetType();
  auto const geomType = fb.GetGeomType();
  // There is a copy of params here, if there is a reference here, then the params can be
  // implicitly changed at other layers.
  auto const params = fb.GetParams();
  switch (sourceType)
  {
  case base::GeoObjectId::Type::ObsoleteOsmNode:
    LayerBase::Handle(fb);
    break;
  case base::GeoObjectId::Type::ObsoleteOsmWay:
  {
    switch (geomType)
    {
    case feature::GeomType::Area:
    {
      HandleArea(fb, params);
      if (CanBeLine(params))
      {
        auto featureLine = MakeLine(fb);
        LayerBase::Handle(featureLine);
      }
      break;
    }
    case feature::GeomType::Line:
      LayerBase::Handle(fb);
      break;
    default:
      UNREACHABLE();
      break;
    }
    break;
  }
  case base::GeoObjectId::Type::ObsoleteOsmRelation:
  {
    switch (geomType)
    {
    case feature::GeomType::Area:
      HandleArea(fb, params);
      break;
    default:
      UNREACHABLE();
      break;
    }
    break;
  }
  default:
    UNREACHABLE();
    break;
  }
}

void RepresentationLayer::HandleArea(FeatureBuilder & fb, FeatureParams const & params)
{
  if (CanBeArea(params))
  {
    LayerBase::Handle(fb);
    fb.SetParams(params);
  }
  else if (CanBePoint(params))
  {
    auto featurePoint = MakePoint(fb);
    LayerBase::Handle(featurePoint);
  }
}

// static
bool RepresentationLayer::CanBeArea(FeatureParams const & params)
{
  return feature::IsDrawableLike(params.m_types, feature::GeomType::Area);
}

// static
bool RepresentationLayer::CanBePoint(FeatureParams const & params)
{
  return feature::HasUsefulType(params.m_types, feature::GeomType::Point);
}

// static
bool RepresentationLayer::CanBeLine(FeatureParams const & params)
{
  return feature::HasUsefulType(params.m_types, feature::GeomType::Line);
}

void PrepareFeatureLayer::Handle(FeatureBuilder & fb)
{
  auto const type = fb.GetGeomType();
  auto & params = fb.GetParams();
  feature::RemoveUselessTypes(params.m_types, type);
  fb.PreSerializeAndRemoveUselessNamesForIntermediate();
  FixLandType(fb);
  if (feature::HasUsefulType(params.m_types, type))
    LayerBase::Handle(fb);
}

void RepresentationCoastlineLayer::Handle(FeatureBuilder & fb)
{
  auto const sourceType = fb.GetMostGenericOsmId().GetType();
  auto const geomType = fb.GetGeomType();
  switch (sourceType)
  {
  case base::GeoObjectId::Type::ObsoleteOsmNode:
    break;
  case base::GeoObjectId::Type::ObsoleteOsmWay:
  {
    switch (geomType)
    {
    case feature::GeomType::Area:
    case feature::GeomType::Line:
      LayerBase::Handle(fb);
      break;
    default:
      UNREACHABLE();
      break;
    }
    break;
  }
  case base::GeoObjectId::Type::ObsoleteOsmRelation:
    break;
  default:
    UNREACHABLE();
    break;
  }
}


void PrepareCoastlineFeatureLayer::Handle(FeatureBuilder & fb)
{
  if (fb.IsArea())
  {
    auto & params = fb.GetParams();
    feature::RemoveUselessTypes(params.m_types, fb.GetGeomType());
  }

  fb.PreSerializeAndRemoveUselessNamesForIntermediate();
  auto const & isCoastlineChecker = ftypes::IsCoastlineChecker::Instance();
  auto const kCoastType = isCoastlineChecker.GetCoastlineType();
  fb.SetType(kCoastType);
  LayerBase::Handle(fb);
}

WorldLayer::WorldLayer(std::string const & popularityFilename)
  : m_filter(popularityFilename)
{
}

void WorldLayer::Handle(FeatureBuilder & fb)
{
  if (fb.RemoveInvalidTypes() && m_filter.IsAccepted(fb))
    LayerBase::Handle(fb);
}

void CountryLayer::Handle(feature::FeatureBuilder & fb)
{
  if (fb.RemoveInvalidTypes() && PreprocessForCountryMap(fb))
    LayerBase::Handle(fb);
}

void PreserializeLayer::Handle(FeatureBuilder & fb)
{
  if (fb.PreSerialize())
    LayerBase::Handle(fb);
}
}  // namespace generator
