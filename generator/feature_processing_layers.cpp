#include "generator/feature_processing_layers.hpp"

#include "generator/city_boundary_processor.hpp"
#include "generator/coastlines_generator.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_maker.hpp"
#include "generator/generate_info.hpp"
#include "generator/emitter_interface.hpp"
#include "generator/type_helper.hpp"

#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

namespace generator
{
namespace
{
void FixLandType(FeatureBuilder1 & feature)
{
  auto const & types = feature.GetTypes();
  auto const & isIslandChecker = ftypes::IsIslandChecker::Instance();
  auto const & isLandChecker = ftypes::IsLandChecker::Instance();
  auto const & isCoastlineChecker = ftypes::IsCoastlineChecker::Instance();
  if (isCoastlineChecker(types))
  {
    feature.PopExactType(isLandChecker.GetLandType());
    feature.PopExactType(isCoastlineChecker.GetCoastlineType());
  }
  else if (isIslandChecker(types) && feature.IsArea())
  {
    feature.AddType(isLandChecker.GetLandType());
  }
}
}  // namespace

std::string LogBuffer::GetAsString() const
{
  return m_buffer.str();
}

void LayerBase::Handle(FeatureBuilder1 & feature)
{
  if (m_next)
    m_next->Handle(feature);
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
  std::ostringstream m_buffer;
  auto temp = shared_from_this();
  while (temp)
  {
    m_buffer << temp->GetAsString();
    temp = temp->m_next;
  }

  return m_buffer.str();
}

RepresentationLayer::RepresentationLayer(std::shared_ptr<CityBoundaryProcessor> processor)
  : m_processor(processor) {}

void RepresentationLayer::Handle(FeatureBuilder1 & feature)
{
  auto const sourceType = feature.GetMostGenericOsmId().GetType();
  auto const geomType = feature.GetGeomType();
  // There is a copy of params here, if there is a reference here, then the params can be
  // implicitly changed at other layers.
  auto const params = feature.GetParams();
  switch (sourceType)
  {
  case base::GeoObjectId::Type::ObsoleteOsmNode:
    LayerBase::Handle(feature);
    break;
  case base::GeoObjectId::Type::ObsoleteOsmWay:
  {
    switch (geomType)
    {
    case feature::EGeomType::GEOM_AREA:
    {
      HandleArea(feature, params);
      if (CanBeLine(params))
      {
        auto featureLine = MakeLineFromArea(feature);
        LayerBase::Handle(featureLine);
      }
      break;
    }
    case feature::EGeomType::GEOM_LINE:
      LayerBase::Handle(feature);
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
    case feature::EGeomType::GEOM_AREA:
      HandleArea(feature, params);
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

void RepresentationLayer::HandleArea(FeatureBuilder1 & feature, FeatureParams const & params)
{
  if (ftypes::IsCityTownOrVillage(params.m_types))
    m_processor->Add(feature);

  if (CanBeArea(params))
  {
    LayerBase::Handle(feature);
    feature.SetParams(params);
  }
  else if (CanBePoint(params))
  {
    auto featurePoint = MakePointFromArea(feature);
    LayerBase::Handle(featurePoint);
  }
}

// static
bool RepresentationLayer::CanBeArea(FeatureParams const & params)
{
  return feature::IsDrawableLike(params.m_types, feature::GEOM_AREA);
}

// static
bool RepresentationLayer::CanBePoint(FeatureParams const & params)
{
  return feature::HasUsefulType(params.m_types, feature::GEOM_POINT);
}

// static
bool RepresentationLayer::CanBeLine(FeatureParams const & params)
{
  return feature::HasUsefulType(params.m_types, feature::GEOM_LINE);
}

void PrepareFeatureLayer::Handle(FeatureBuilder1 & feature)
{
  auto const type = feature.GetGeomType();
  auto const & types = feature.GetParams().m_types;
  if (!feature::HasUsefulType(types, type))
    return;

  auto & params = feature.GetParams();
  feature::RemoveUselessTypes(params.m_types, type);
  feature.PreSerializeAndRemoveUselessNames();
  FixLandType(feature);
  LayerBase::Handle(feature);
}

CityBoundaryLayer::CityBoundaryLayer(std::shared_ptr<CityBoundaryProcessor> processor)
  : m_processor(processor) {}

void CityBoundaryLayer::Handle(FeatureBuilder1 & feature)
{
  auto const type = GetPlaceType(feature);
  if (type != ftype::GetEmptyValue() && !feature.GetName().empty())
    m_processor->Replace(feature);
  else
    LayerBase::Handle(feature);
}

BookingLayer::~BookingLayer()
{
  m_dataset.BuildOsmObjects([&] (FeatureBuilder1 & feature) {
    m_countryMapper->RemoveInvalidTypesAndMap(feature);
  });
}

BookingLayer::BookingLayer(std::string const & filename, std::shared_ptr<CountryMapper> countryMapper)
  : m_dataset(filename), m_countryMapper(countryMapper) {}

void BookingLayer::Handle(FeatureBuilder1 & feature)
{
  auto const id = m_dataset.FindMatchingObjectId(feature);
  if (id == BookingHotel::InvalidObjectId())
  {
    LayerBase::Handle(feature);
    return;
  }

  m_dataset.PreprocessMatchedOsmObject(id, feature, [&](FeatureBuilder1 & newFeature) {
    AppendLine("BOOKING", DebugPrint(newFeature.GetMostGenericOsmId()), DebugPrint(id));
    m_countryMapper->RemoveInvalidTypesAndMap(newFeature);
  });
}

OpentableLayer::OpentableLayer(std::string const & filename, std::shared_ptr<CountryMapper> countryMapper)
  : m_dataset(filename), m_countryMapper(countryMapper) {}

void OpentableLayer::Handle(FeatureBuilder1 & feature)
{
  auto const id = m_dataset.FindMatchingObjectId(feature);
  if (id == OpentableRestaurant::InvalidObjectId())
  {
    LayerBase::Handle(feature);
    return;
  }

  m_dataset.PreprocessMatchedOsmObject(id, feature, [&](FeatureBuilder1 & newFeature) {
    AppendLine("OPENTABLE", DebugPrint(newFeature.GetMostGenericOsmId()), DebugPrint(id));
    m_countryMapper->RemoveInvalidTypesAndMap(newFeature);
  });
}

CountryMapperLayer::CountryMapperLayer(std::shared_ptr<CountryMapper> countryMapper)
  : m_countryMapper(countryMapper) {}

void CountryMapperLayer::Handle(FeatureBuilder1 & feature)
{
  m_countryMapper->RemoveInvalidTypesAndMap(feature);
  LayerBase::Handle(feature);
}

void RepresentationCoastlineLayer::Handle(FeatureBuilder1 & feature)
{
  auto const sourceType = feature.GetMostGenericOsmId().GetType();
  auto const geomType = feature.GetGeomType();
  switch (sourceType)
  {
  case base::GeoObjectId::Type::ObsoleteOsmNode:
    break;
  case base::GeoObjectId::Type::ObsoleteOsmWay:
  {
    switch (geomType)
    {
    case feature::EGeomType::GEOM_AREA:
      LayerBase::Handle(feature);
      break;
    case feature::EGeomType::GEOM_LINE:
      LayerBase::Handle(feature);
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

void PrepareCoastlineFeatureLayer::Handle(FeatureBuilder1 & feature)
{
  if (feature.IsArea())
  {
    auto & params = feature.GetParams();
    feature::RemoveUselessTypes(params.m_types, feature.GetGeomType());
  }

  feature.PreSerializeAndRemoveUselessNames();
  LayerBase::Handle(feature);
}

CoastlineMapperLayer::CoastlineMapperLayer(std::shared_ptr<CoastlineFeaturesGenerator> coastlineMapper)
  : m_coastlineGenerator(coastlineMapper) {}

void CoastlineMapperLayer::Handle(FeatureBuilder1 & feature)
{
  auto const & isCoastlineChecker = ftypes::IsCoastlineChecker::Instance();
  auto const kCoastType = isCoastlineChecker.GetCoastlineType();
  feature.SetType(kCoastType);
  m_coastlineGenerator->Process(feature);
  LayerBase::Handle(feature);
}

WorldAreaLayer::WorldAreaLayer(std::shared_ptr<WorldMapper> mapper)
  : m_mapper(mapper) {}


WorldAreaLayer::~WorldAreaLayer()
{
  m_mapper->Merge();
}

void WorldAreaLayer::Handle(FeatureBuilder1 & feature)
{
  m_mapper->RemoveInvalidTypesAndMap(feature);
  LayerBase::Handle(feature);
}

EmitCoastsLayer::EmitCoastsLayer(std::string const & worldCoastsFilename, std::string const & geometryFilename,
                                 std::shared_ptr<CountryMapper> countryMapper)
  : m_collector(std::make_shared<feature::FeaturesCollector>(worldCoastsFilename))
  , m_countryMapper(countryMapper)
  , m_geometryFilename(geometryFilename) {}

EmitCoastsLayer::~EmitCoastsLayer()
{
  feature::ForEachFromDatRawFormat(m_geometryFilename, [&](FeatureBuilder1 fb, uint64_t)
  {
    auto & emitter = m_countryMapper->Parent();
    emitter.Start();
    m_countryMapper->Map(fb);
    emitter.Finish();

    fb.AddName("default", emitter.m_currentNames);
    (*m_collector)(fb);
  });
}

void EmitCoastsLayer::Handle(FeatureBuilder1 & feature)
{
  LayerBase::Handle(feature);
}

CountryMapper::CountryMapper(feature::GenerateInfo const & info)
  : m_countries(std::make_unique<CountriesGenerator>(info)) {}

void CountryMapper::Map(FeatureBuilder1 & feature)
{
  m_countries->Process(feature);
}

void CountryMapper::RemoveInvalidTypesAndMap(FeatureBuilder1 & feature)
{
  if (!feature.RemoveInvalidTypes())
    return;

  m_countries->Process(feature);
}

CountryMapper::Polygonizer & CountryMapper::Parent()
{
  return m_countries->Parent();
}

std::vector<std::string> const & CountryMapper::GetNames() const
{
  return m_countries->Parent().Names();
}

WorldMapper::WorldMapper(std::string const & worldFilename, std::string const & rawGeometryFilename,
                         std::string const & popularPlacesFilename)
  : m_world(std::make_unique<WorldGenerator>(worldFilename, rawGeometryFilename, popularPlacesFilename)) {}

void WorldMapper::Map(FeatureBuilder1 & feature)
{
  m_world->Process(feature);
}

void WorldMapper::RemoveInvalidTypesAndMap(FeatureBuilder1 & feature)
{
  if (!feature.RemoveInvalidTypes())
    return;

  m_world->Process(feature);
}

void WorldMapper::Merge()
{
  m_world->DoMerge();
}
}  // namespace generator
