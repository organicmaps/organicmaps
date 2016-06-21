#include "generator/routing_generator.hpp"
#include "generator/srtm_parser.hpp"

#include "defines.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/file_container.hpp"
#include "coding/varint.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "std/map.hpp"
#include "std/type_traits.hpp"

using namespace feature;

namespace
{
static_assert(is_same<TAltitude, generator::SrtmTile::THeight>::value, "");
static_assert(kInvalidAltitude == generator::SrtmTile::kInvalidHeight, "");

class Processor
{
public:
  Processor(string const & srtmPath) : m_srtmManager(srtmPath) {}
  map<uint32_t, Altitudes> const & GetFeatureAltitudes() const { return m_featureAltitudes; }

  void operator()(FeatureType const & f, uint32_t const & id)
  {
    f.ParseTypes();
    f.ParseHeader2();
    if (!routing::IsRoad(feature::TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    Altitudes alts(m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(0))),
                   m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(pointsCount - 1))));
    m_featureAltitudes[id] = alts;
  }

private:
  generator::SrtmTileManager m_srtmManager;
  map<uint32_t, Altitudes> m_featureAltitudes;
};
} // namespace

namespace routing
{
void BuildRoadFeatureAltitude(string const & srtmPath, string const & baseDir, string const & countryName)
{
  LOG(LINFO, ("srtmPath =", srtmPath, "baseDir =", baseDir, "countryName =", countryName));
  string const altPath = baseDir + countryName + "." + ALTITUDE_TAG;
  string const mwmPath = baseDir + countryName + DATA_FILE_EXTENSION;

  // Writing section with altitude information.
  {
    FilesContainerW altCont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    FileWriter w = altCont.GetWriter(ALTITUDE_TAG);

    Processor processor(srtmPath);
    feature::ForEachFromDat(mwmPath, processor);
    map<uint32_t, Altitudes> const & featureAltitudes = processor.GetFeatureAltitudes();

    for (auto const & a : featureAltitudes)
    {
      Altitude altitude(a.first /* feature id */, a.second /* feature altitudes */);
      altitude.Serialize(w);
    }
    LOG(LINFO, ("Altitude was written for", featureAltitudes.size(), "features."));
  }
}
} // namespace routing
