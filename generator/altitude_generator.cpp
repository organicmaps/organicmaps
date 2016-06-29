#include "generator/routing_generator.hpp"
#include "generator/srtm_parser.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/varint.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"
#include "std/type_traits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

using namespace feature;

namespace
{
class Processor
{
public:
  using TFeatureAltitude = pair<uint32_t, Altitudes>;
  using TFeatureAltitudes = vector<TFeatureAltitude>;

  Processor(string const & srtmPath) : m_srtmManager(srtmPath) {}

  TFeatureAltitudes const & GetFeatureAltitudes() const { return m_featureAltitudes; }

  void operator()(FeatureType const & f, uint32_t const & id)
  {
    if (!routing::IsRoad(feature::TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    Altitudes alts(m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(0))),
                   m_srtmManager.GetHeight(MercatorBounds::ToLatLon(f.GetPoint(pointsCount - 1))));
    m_featureAltitudes.push_back(make_pair(id, alts));
  }

  void SortFeatureAltitudes()
  {
    sort(m_featureAltitudes.begin(), m_featureAltitudes.end(), my::LessBy(&Processor::TFeatureAltitude::first));
  }

private:
  generator::SrtmTileManager m_srtmManager;
  TFeatureAltitudes m_featureAltitudes;
};
}  // namespace

namespace routing
{
void BuildRoadAltitudes(string const & srtmPath, string const & baseDir, string const & countryName)
{
  LOG(LINFO, ("srtmPath =", srtmPath, "baseDir =", baseDir, "countryName =", countryName));
  string const mwmPath = my::JoinFoldersToPath(baseDir, countryName + DATA_FILE_EXTENSION);

  // Writing section with altitude information.
  {
    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    FileWriter w = cont.GetWriter(ALTITUDE_FILE_TAG);

    Processor processor(srtmPath);
    feature::ForEachFromDat(mwmPath, processor);
    processor.SortFeatureAltitudes();
    Processor::TFeatureAltitudes const & featureAltitudes = processor.GetFeatureAltitudes();

    for (auto const & a : featureAltitudes)
    {
      Altitude altitude(a.first /* feature id */, a.second /* feature altitudes */);
      altitude.Serialize(w);
    }
    LOG(LINFO, ("Altitude was written for", featureAltitudes.size(), "features."));
  }
}
}  // namespace routing
