#include "generator/emitter_planet.hpp"

#include "generator/brands_loader.hpp"
#include "generator/feature_builder.hpp"
#include "generator/emitter_interface.hpp"

#include "base/macros.hpp"

#include <fstream>
#include <memory>

using namespace std;

namespace generator
{
EmitterPlanet::EmitterPlanet(feature::GenerateInfo const & info) :
  m_skippedElementsPath(info.GetIntermediateFileName("skipped_elements", ".lst")),
  m_failOnCoasts(info.m_failOnCoasts),
  m_bookingDataset(info.m_bookingDatafileName),
  m_opentableDataset(info.m_opentableDatafileName),
  m_viatorDataset(info.m_viatorDatafileName),
  m_boundariesTable(info.m_boundariesTable)
{
  if (!info.m_brandsFilename.empty() && !info.m_brandsTranslationsFilename.empty())
    CHECK(LoadBrands(info.m_brandsFilename, info.m_brandsTranslationsFilename, m_brands), ());

  Classificator const & c = classif();
  char const * arr[][2] = {
    {"natural", "coastline"}, {"natural", "land"}, {"place", "island"}, {"place", "islet"}};
  static_assert(ARRAY_SIZE(arr) == TYPES_COUNT, "");

  for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
    m_types[i] = c.GetTypeByPath({arr[i][0], arr[i][1]});

  m_srcCoastsFile = info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom");

  CHECK(!info.m_makeCoasts || !info.m_createWorld,
        ("We can't do make_coasts and generate_world at the same time"));

  if (info.m_makeCoasts)
  {
    m_coasts.reset(new CoastlineFeaturesGenerator(Type(NATURAL_COASTLINE)));

    m_coastsHolder.reset(new feature::FeaturesAndRawGeometryCollector(
                           m_srcCoastsFile,
                           info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION)));
    return;
  }

  if (info.m_emitCoasts)
    m_coastsHolder.reset(
          new feature::FeaturesCollector(info.GetTmpFileName(WORLD_COASTS_FILE_NAME)));

  if (info.m_splitByPolygons || !info.m_fileName.empty())
    m_countries = make_unique<CountriesGenerator>(info);

  if (info.m_createWorld)
    m_world.reset(new WorldGenerator(info));
}

void EmitterPlanet::operator()(FeatureBuilder1 & fb)
{
  uint32_t const type = GetPlaceType(fb.GetParams());

  // TODO(mgserigio): Would it be better to have objects that store callback
  // and can be piped: action-if-cond1 | action-if-cond-2 | ... ?
  // The first object which perform action terminates the cahin.
  if (type != ftype::GetEmptyValue() && !fb.GetName().empty())
  {
    auto const viatorObjId = m_viatorDataset.FindMatchingObjectId(fb);
    if (viatorObjId != ViatorCity::InvalidObjectId())
    {
      m_viatorDataset.PreprocessMatchedOsmObject(viatorObjId, fb, [this, viatorObjId](FeatureBuilder1 & fb)
      {
        m_skippedElements << "VIATOR\t" << DebugPrint(fb.GetMostGenericOsmId())
                          << '\t' << viatorObjId.Get() << endl;
      });
    }

    Place const place(fb, type);
    UnionEqualPlacesIds(place);
    m_places.ReplaceEqualInRect(
          place, [](Place const & p1, Place const & p2) { return p1.IsEqual(p2); },
    [](Place const & p1, Place const & p2) { return p1.IsBetterThan(p2); });
    return;
  }

  auto const bookingObjId = m_bookingDataset.FindMatchingObjectId(fb);
  if (bookingObjId != BookingHotel::InvalidObjectId())
  {
    m_bookingDataset.PreprocessMatchedOsmObject(bookingObjId, fb, [this, bookingObjId](FeatureBuilder1 & fb)
    {
      m_skippedElements << "BOOKING\t" << DebugPrint(fb.GetMostGenericOsmId())
                        << '\t' << bookingObjId.Get() << endl;
      Emit(fb);
    });
    return;
  }

  auto const opentableObjId = m_opentableDataset.FindMatchingObjectId(fb);
  if (opentableObjId != OpentableRestaurant::InvalidObjectId())
  {
    m_opentableDataset.PreprocessMatchedOsmObject(opentableObjId, fb, [this, opentableObjId](FeatureBuilder1 & fb)
    {
      m_skippedElements << "OPENTABLE\t" << DebugPrint(fb.GetMostGenericOsmId())
                        << '\t' << opentableObjId.Get() << endl;
      Emit(fb);
    });
    return;
  }

  auto const it = m_brands.find(fb.GetMostGenericOsmId());
  if (it != m_brands.cend())
  {
    auto & metadata = fb.GetMetadata();
    metadata.Set(feature::Metadata::FMD_BRAND, it->second);
  }

  Emit(fb);
}

void EmitterPlanet::EmitCityBoundary(FeatureBuilder1 const & fb, FeatureParams const & params)
{
  if (!m_boundariesTable)
    return;

  auto const type = GetPlaceType(params);
  if (type == ftype::GetEmptyValue())
    return;

  auto const id = fb.GetLastOsmId();
  m_boundariesTable->Append(id, indexer::CityBoundary(fb.GetOuterGeometry()));

  Place const place(fb, type);
  UnionEqualPlacesIds(place);
}

/// @return false if coasts are not merged and FLAG_fail_on_coasts is set
bool EmitterPlanet::Finish()
{
  DumpSkippedElements();

  // Emit all required booking objects to the map.
  m_bookingDataset.BuildOsmObjects([this](FeatureBuilder1 & fb) { Emit(fb); });
  // No opentable objects should be emitted. Opentable data enriches some data
  // with a link to a restaurant's reservation page.

  m_places.ForEach([this](Place const & p)
  {
    // m_places are no longer used after this point.
    Emit(const_cast<FeatureBuilder1 &>(p.GetFeature()));
  });

  if (m_world)
    m_world->DoMerge();

  if (m_coasts)
  {
    // Check and stop if some coasts were not merged
    if (!m_coasts->Finish() && m_failOnCoasts)
      return false;

    LOG(LINFO, ("Generating coastline polygons"));

    size_t totalFeatures = 0;
    size_t totalPoints = 0;
    size_t totalPolygons = 0;

    vector<FeatureBuilder1> vecFb;
    m_coasts->GetFeatures(vecFb);

    for (auto & fb : vecFb)
    {
      (*m_coastsHolder)(fb);

      ++totalFeatures;
      totalPoints += fb.GetPointsCount();
      totalPolygons += fb.GetPolygonsCount();
    }
    LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons,
                "total points:", totalPoints));
  }
  else if (m_coastsHolder)
  {
    CHECK(m_countries, ());

    feature::ForEachFromDatRawFormat(m_srcCoastsFile, [this](FeatureBuilder1 fb, uint64_t)
    {
      auto & emitter = m_countries->Parent();

      emitter.Start();
      (*m_countries)(fb);
      emitter.Finish();

      if (m_coastsHolder)
      {
        fb.AddName("default", emitter.m_currentNames);
        (*m_coastsHolder)(fb);
      }
    });
  }
  return true;
}

void EmitterPlanet::GetNames(vector<string> & names) const
{
  if (m_countries)
    names = m_countries->Parent().Names();
  else
    names.clear();
}

void EmitterPlanet::Emit(FeatureBuilder1 & fb)
{
  uint32_t const coastType = Type(NATURAL_COASTLINE);
  bool const hasCoast = fb.HasType(coastType);

  if (m_coasts)
  {
    if (hasCoast)
    {
      CHECK(fb.GetGeomType() != feature::GEOM_POINT, ());
      // Leave only coastline type.
      fb.SetType(coastType);
      (*m_coasts)(fb);
    }
    return;
  }

  if (hasCoast)
  {
    fb.PopExactType(Type(NATURAL_LAND));
    fb.PopExactType(coastType);
  }
  else if ((fb.HasType(Type(PLACE_ISLAND)) || fb.HasType(Type(PLACE_ISLET))) &&
           fb.GetGeomType() == feature::GEOM_AREA)
  {
    fb.AddType(Type(NATURAL_LAND));
  }

  if (!fb.RemoveInvalidTypes())
    return;

  if (m_world)
    (*m_world)(fb);

  if (m_countries)
    (*m_countries)(fb);
}

void EmitterPlanet::DumpSkippedElements()
{
  auto const skippedElements = m_skippedElements.str();

  if (skippedElements.empty())
  {
    LOG(LINFO, ("No osm object was skipped."));
    return;
  }

  ofstream file(m_skippedElementsPath, ios_base::app);
  if (file.is_open())
  {
    file << m_skippedElements.str();
    LOG(LINFO, ("Saving skipped elements to", m_skippedElementsPath, "done."));
  }
  else
  {
    LOG(LERROR, ("Can't output into", m_skippedElementsPath));
  }
}


uint32_t EmitterPlanet::GetPlaceType(FeatureParams const & params) const
{
  static uint32_t const placeType = classif().GetTypeByPath({"place"});
  return params.FindType(placeType, 1 /* level */);
}

void EmitterPlanet::UnionEqualPlacesIds(Place const & place)
{
  if (!m_boundariesTable)
    return;

  auto const id = place.GetFeature().GetLastOsmId();
  m_places.ForEachInRect(place.GetLimitRect(), [&](Place const & p) {
    if (p.IsEqual(place))
      m_boundariesTable->Union(p.GetFeature().GetLastOsmId(), id);
  });
}
}  // namespace generator
