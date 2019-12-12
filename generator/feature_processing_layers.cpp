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

void LayerBase::Handle(FeatureBuilder & fb)
{
  if (m_next)
    m_next->Handle(fb);
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

RepresentationLayer::RepresentationLayer(std::shared_ptr<ComplexFeaturesMixer> const & complexFeaturesMixer)
  : m_complexFeaturesMixer(complexFeaturesMixer)
{
}

void RepresentationLayer::Handle(FeatureBuilder & fb)
{
  if (m_complexFeaturesMixer)
    m_complexFeaturesMixer->Process([&](FeatureBuilder & fb){ LayerBase::Handle(fb); }, fb);

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

void RepresentationLayer::HandleArea(FeatureBuilder & fb, FeatureBuilderParams const & params)
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

ComplexFeaturesMixer::ComplexFeaturesMixer(std::unordered_set<CompositeId> const & hierarchyNodesSet)
  : m_hierarchyNodesSet(hierarchyNodesSet)
  , m_complexEntryType(classif().GetTypeByPath({"complex_entry"}))
{
}

std::shared_ptr<ComplexFeaturesMixer> ComplexFeaturesMixer::Clone()
{
  return std::make_shared<ComplexFeaturesMixer>(m_hierarchyNodesSet);
}

void ComplexFeaturesMixer::Process(std::function<void(feature::FeatureBuilder &)> next,
                                   feature::FeatureBuilder const & fb)
{
  if (!next)
    return;

  // For all objects in the hierarchy, there must be one areal object and one linear.
  // Exceptions are point features and parts of buildings.
  if (fb.IsPoint() || !fb.IsGeometryClosed())
    return;

  auto const id = MakeCompositeId(fb);
  auto const it = m_hierarchyNodesSet.find(id);
  if (it == std::end(m_hierarchyNodesSet))
    return;

  static auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
  if (buildingPartChecker(fb.GetTypes()))
    return;

  auto const canBeArea = RepresentationLayer::CanBeArea(fb.GetParams());
  auto const canBeLine = RepresentationLayer::CanBeLine(fb.GetParams());
  if (!canBeArea)
  {
    LOG(LINFO, ("Add a areal complex feature for", fb.GetMostGenericOsmId()));
    auto complexFb = MakeComplexAreaFrom(fb);
    next(complexFb);
  }

  if (!canBeLine)
  {
    LOG(LINFO, ("Add a linear complex feature for", fb.GetMostGenericOsmId()));
    auto complexFb = MakeComplexLineFrom(fb);
    next(complexFb);
  }
}

feature::FeatureBuilder ComplexFeaturesMixer::MakeComplexLineFrom(feature::FeatureBuilder const & fb)
{
  CHECK(fb.IsArea() || fb.IsLine(), ());
  CHECK(fb.IsGeometryClosed(), ());

  auto lineFb = MakeLine(fb);
  auto & params = lineFb.GetParams();
  params.ClearName();
  params.GetMetadata() = {};
  params.SetType(m_complexEntryType);
  return lineFb;
}

feature::FeatureBuilder ComplexFeaturesMixer::MakeComplexAreaFrom(feature::FeatureBuilder const & fb)
{
  CHECK(fb.IsArea() || fb.IsLine(), ());
  CHECK(fb.IsGeometryClosed(), ());

  auto areaFb = fb;
  areaFb.SetArea();
  auto & params = areaFb.GetParams();
  params.ClearName();
  params.GetMetadata() = {};
  params.SetType(m_complexEntryType);
  return areaFb;
}
}  // namespace generator
