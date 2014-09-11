#include "routing_generator.hpp"
#include "gen_mwm_info.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/mercator.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../routing/osrm_data_facade_types.hpp"

#include "../platform/platform.hpp"

#include "../base/logging.hpp"

#include "../std/fstream.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/EdgeBasedNodeData.h"


namespace routing
{


double const EQUAL_POINT_RADIUS_M = 1;

void GenerateNodesInfo(string const & mwmName, string const & osrmName)
{
  classificator::Load();

  Index index;
  m2::RectD rect;
  if (!index.Add(mwmName, rect))
  {
    LOG(LERROR, ("MWM file not found"));
    return;
  }

  Platform & pl = GetPlatform();

  gen::OsmID2FeatureID osm2ft;
  {
    ReaderSource<ModelReaderPtr> src(pl.GetReader(mwmName + OSM2FEATURE_FILE_EXTENSION));
    osm2ft.Read(src);
  }

  string nodeFileName = osrmName + ".nodeData";

  OsrmFtSegMapping mapping;

  ifstream input;
  input.open(nodeFileName);
  if (!input.is_open())
  {
    LOG(LERROR, ("Can't open file ", nodeFileName));
    return;
  }

  osrm::NodeDataVectorT nodeData;
  if (!osrm::LoadNodeDataFromFile(nodeFileName, nodeData))
  {
    LOG(LERROR, ("Can't load node data"));
    return;
  }

  uint32_t found = 0, all = 0;
  uint32_t nodeId = 0;
  for (osrm::NodeData data : nodeData)
  {
    uint32_t segId = 0;
    OsrmFtSegMapping::FtSegVectorT vec;

    for (auto seg : data.m_segments)
    {
      m2::PointD pts[2] = {{seg.lon1, seg.lat1}, {seg.lon2, seg.lat2}};

      all++;

      // now need to determine feature id and segments in it
      uint32_t const fID = osm2ft.GetFeatureID(seg.wayId);
      if (fID == 0)
      {
        LOG(LWARNING, ("No feature id for way:", seg.wayId));
        continue;
      }

      FeatureType ft;
      Index::FeaturesLoaderGuard loader(index, 0);
      loader.GetFeature(fID, ft);

      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
      int32_t indicies[2] = {-1, -1};
      double minDist[2] = {numeric_limits<double>::max(), numeric_limits<double>::max()};

      for (uint32_t j = 0; j < ft.GetPointsCount(); ++j)
      {
        double lon = MercatorBounds::XToLon(ft.GetPoint(j).x);
        double lat = MercatorBounds::YToLat(ft.GetPoint(j).y);
        for (uint32_t k = 0; k < 2; ++k)
        {
          double const dist = ms::DistanceOnEarth(pts[k].y, pts[k].x, lat, lon);

          if (dist <= EQUAL_POINT_RADIUS_M && dist < minDist[k])
          {
            indicies[k] = j;
            minDist[k] = dist;
          }
        }
      }

      // Check if indicies found
      if (indicies[0] != -1 && indicies[1] != -1)
      {
        found++;
        /*bool canMerge = !vec.empty();
        if (canMerge)
        {
          if (vec.back().m_fid == fID)
          {
            canMerge = true;

            OsrmFtSegMapping::FtSeg & seg = vec.back();
            if (indicies[0] == seg.m_pointEnd)
              seg.m_pointEnd = indicies[1];
            else
            {
              if (indicies[1] == seg.m_pointStart)
                seg.m_pointStart = indicies[0];
              else
                canMerge = false;
            }

          }
          else
            canMerge = false;
        }

        if (!canMerge)*/
        OsrmFtSegMapping::FtSeg ftSeg(fID, indicies[0], indicies[1]);
        if (vec.empty() || !vec.back().Merge(ftSeg))
          vec.push_back(ftSeg);

      }
      else
      {
        LOG(LINFO, ("----------------- Way ID: ", seg.wayId, "--- Segments: ", data.m_segments.size(), " SegId: ", segId));
        LOG(LINFO, ("P1: ", pts[0].y, " ", pts[0].x, " P2: ", pts[1].y, " ", pts[1].x));
        for (uint32_t j = 0; j < ft.GetPointsCount(); ++j)
        {
          double lon = MercatorBounds::XToLon(ft.GetPoint(j).x);
          double lat = MercatorBounds::YToLat(ft.GetPoint(j).y);
          double const dist1 = ms::DistanceOnEarth(pts[0].y, pts[0].x, lat, lon);
          double const dist2 = ms::DistanceOnEarth(pts[1].y, pts[1].x, lat, lon);
          LOG(LINFO, ("p", j, ": ", lat, ", ", lon, " Dist1: ", dist1, " Dist2: ", dist2));
        }
      }
    }

    mapping.Append(nodeId++, vec);
  }

  LOG(LINFO, ("Found: ", found, " All: ", all));
  mapping.Save(osrmName + ".ftseg");
}

}
