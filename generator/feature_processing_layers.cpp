#include "generator/feature_processing_layers.hpp"
#include "generator/feature_maker_base.hpp"
#include "generator/feature_merger.hpp"
#include "generator/generate_info.hpp"
#include "generator/place_processor.hpp"

#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

namespace generator
{
using namespace feature;

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

void LayerBase::Add(std::shared_ptr<LayerBase> next)
{
  if (m_next)
    m_next->Add(next);
  else
    m_next = next;
}

void RepresentationLayer::Handle(FeatureBuilder & fb)
{
  //  if (m_complexFeaturesMixer)
  //    m_complexFeaturesMixer->Process([&](FeatureBuilder & fb){ LayerBase::Handle(fb); }, fb);

  auto const sourceType = fb.GetMostGenericOsmId().GetType();
  auto const geomType = fb.GetGeomType();
  // There is a copy of params here, if there is a reference here, then the params can be
  // implicitly changed at other layers.
  auto const params = fb.GetParams();
  switch (sourceType)
  {
  case base::GeoObjectId::Type::ObsoleteOsmNode: LayerBase::Handle(fb); break;
  case base::GeoObjectId::Type::ObsoleteOsmWay:
  {
    switch (geomType)
    {
    case GeomType::Area:
    {
      // All closed geometry OsmWays fall through here.
      // E.g. a closed way with tags [amenity=restaurant][wifi=yes][cuisine=*]
      // should be added as an areal feature with corresponding types
      // and no extra linear/point features.
      // A closed way with a tag [highway=pedestrian] should be added as a line feature only
      // (because there are no areal and/or point drules for the highway-pedestrian type).
      // But a closed way tagged [highway=pedestrian][area=yes] should be added as both area and line features
      // (because there are areal and line drules for the highway-primary-area type).
      // Also an [leisure=playground][barrier=fence] should be added
      // as an areal amenity-playground and a linear barrier=fence.

      // Try to create an areal object first (if there are corresponding drules),
      // otherwise try to create a point object.
      HandleArea(fb, params);
      // CanBeLine ignores exceptional types from TypeAlwaysExists / IsUsefulNondrawableType.
      if (CanBeLine(params))
      {
        auto featureLine = MakeLine(fb);
        LayerBase::Handle(featureLine);
      }
      break;
    }
    case GeomType::Line: LayerBase::Handle(fb); break;
    default: UNREACHABLE(); break;
    }
    break;
  }
  case base::GeoObjectId::Type::ObsoleteOsmRelation:
  {
    switch (geomType)
    {
    case GeomType::Area: HandleArea(fb, params); break;
    // We transform place relations into points (see BuildFromRelation).
    case GeomType::Point: LayerBase::Handle(fb); break;
    default: UNREACHABLE(); break;
    }
    break;
  }
  default: UNREACHABLE(); break;
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
    // CanBePoint ignores exceptional types from TypeAlwaysExists / IsUsefulNondrawableType.
    auto featurePoint = MakePoint(fb);
    LayerBase::Handle(featurePoint);
  }
}

// static
bool RepresentationLayer::CanBeArea(FeatureParams const & params)
{
  return CanGenerateLike(params.m_types, GeomType::Area);
}

// static
bool RepresentationLayer::CanBePoint(FeatureParams const & params)
{
  return CanGenerateLike(params.m_types, GeomType::Point);
}

// static
bool RepresentationLayer::CanBeLine(FeatureParams const & params)
{
  return CanGenerateLike(params.m_types, GeomType::Line);
}

void PrepareFeatureLayer::Handle(FeatureBuilder & fb)
{
  if (RemoveUselessTypes(fb.GetParams().m_types, fb.GetGeomType()))
  {
    fb.PreSerializeAndRemoveUselessNamesForIntermediate();
    FixLandType(fb);
    LayerBase::Handle(fb);
  }
}

void RepresentationCoastlineLayer::Handle(FeatureBuilder & fb)
{
  auto const sourceType = fb.GetMostGenericOsmId().GetType();
  auto const geomType = fb.GetGeomType();
  switch (sourceType)
  {
  case base::GeoObjectId::Type::ObsoleteOsmNode: break;
  case base::GeoObjectId::Type::ObsoleteOsmWay:
  {
    switch (geomType)
    {
    case GeomType::Area:
    case GeomType::Line: LayerBase::Handle(fb); break;
    default: UNREACHABLE(); break;
    }
    break;
  }
  case base::GeoObjectId::Type::ObsoleteOsmRelation: break;
  default: UNREACHABLE(); break;
  }
}

void PrepareCoastlineFeatureLayer::Handle(FeatureBuilder & fb)
{
  if (fb.IsArea())
  {
    auto & params = fb.GetParams();
    RemoveUselessTypes(params.m_types, fb.GetGeomType());
  }

  fb.PreSerializeAndRemoveUselessNamesForIntermediate();
  auto const & isCoastlineChecker = ftypes::IsCoastlineChecker::Instance();
  auto const kCoastType = isCoastlineChecker.GetCoastlineType();
  fb.SetType(kCoastType);
  LayerBase::Handle(fb);
}

WorldLayer::WorldLayer(std::string const & popularityFilename) : m_filter(popularityFilename) {}

void WorldLayer::Handle(FeatureBuilder & fb)
{
  if (fb.RemoveInvalidTypes() && m_filter.IsAccepted(fb))
    LayerBase::Handle(fb);
}

void CountryLayer::Handle(FeatureBuilder & fb)
{
  if (fb.RemoveInvalidTypes() && PreprocessForCountryMap(fb))
    LayerBase::Handle(fb);
}

/*
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

void ComplexFeaturesMixer::Process(std::function<void(FeatureBuilder &)> next,
                                   FeatureBuilder const & fb)
{
  if (!next)
    return;

  // For rendering purposes all hierarchy objects with closed geometry except building parts must
  // be stored as one areal object and/or one linear object.
  if (fb.IsPoint() || !fb.IsGeometryClosed())
    return;

  auto const id = MakeCompositeId(fb);
  if (m_hierarchyNodesSet.count(id) == 0)
    return;

  static auto const & buildingPartChecker = ftypes::IsBuildingPartChecker::Instance();
  if (buildingPartChecker(fb.GetTypes()))
    return;

  auto const canBeArea = RepresentationLayer::CanBeArea(fb.GetParams());
  auto const canBeLine = RepresentationLayer::CanBeLine(fb.GetParams());
  if (!canBeArea)
  {
    LOG(LINFO, ("Adding an areal complex feature for", fb.GetMostGenericOsmId()));
    auto complexFb = MakeComplexAreaFrom(fb);
    next(complexFb);
  }

  if (!canBeLine)
  {
    LOG(LINFO, ("Adding a linear complex feature for", fb.GetMostGenericOsmId()));
    auto complexFb = MakeComplexLineFrom(fb);
    next(complexFb);
  }
}

FeatureBuilder ComplexFeaturesMixer::MakeComplexLineFrom(FeatureBuilder const & fb)
{
  CHECK(fb.IsArea() || fb.IsLine(), ());
  CHECK(fb.IsGeometryClosed(), ());

  auto lineFb = MakeLine(fb);
  auto & params = lineFb.GetParams();
  params.ClearName();
  params.ClearMetadata();
  params.SetType(m_complexEntryType);
  return lineFb;
}

FeatureBuilder ComplexFeaturesMixer::MakeComplexAreaFrom(FeatureBuilder const & fb)
{
  CHECK(fb.IsArea() || fb.IsLine(), ());
  CHECK(fb.IsGeometryClosed(), ());

  auto areaFb = fb;
  areaFb.SetArea();
  auto & params = areaFb.GetParams();
  params.ClearName();
  params.ClearMetadata();
  params.SetType(m_complexEntryType);
  return areaFb;
}
*/
}  // namespace generator
