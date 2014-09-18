#include "routing_generator.hpp"
#include "gen_mwm_info.hpp"

#include "../coding/file_container.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/mercator.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../routing/osrm2feature_map.hpp"

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

  OsrmFtSegMappingBuilder mapping;

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

  uint32_t found = 0, all = 0, multiple = 0, equal = 0, moreThan1Seg = 0, stored = 0;

  for (size_t nodeId = 0; nodeId < nodeData.size(); ++nodeId)
  {
    auto const & data = nodeData[nodeId];

    OsrmFtSegMappingBuilder::FtSegVectorT vec;

    for (auto const & seg : data.m_segments)
    {
      m2::PointD pts[2] = { { seg.lon1, seg.lat1 }, { seg.lon2, seg.lat2 } };

      ++all;

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

      typedef pair<int, double> IndexT;
      vector<IndexT> indices[2];

      // Match input segment points on feature points.
      for (int j = 0; j < ft.GetPointsCount(); ++j)
      {
        double const lon = MercatorBounds::XToLon(ft.GetPoint(j).x);
        double const lat = MercatorBounds::YToLat(ft.GetPoint(j).y);
        for (int k = 0; k < 2; ++k)
        {
          double const dist = ms::DistanceOnEarth(pts[k].y, pts[k].x, lat, lon);
          if (dist <= EQUAL_POINT_RADIUS_M)
            indices[k].push_back(make_pair(j, dist));
        }
      }

      if (!indices[0].empty() && !indices[1].empty())
      {
        for (int k = 0; k < 2; ++k)
        {
          sort(indices[k].begin(), indices[k].end(), [] (IndexT const & r1, IndexT const & r2)
          {
            return (r1.second < r2.second);
          });
        }

        // Show warnings for multiple or equal choices.
        if (indices[0].size() != 1 && indices[1].size() != 1)
        {
          ++multiple;
          //LOG(LWARNING, ("Multiple index choices for way:", seg.wayId, indices[0], indices[1]));
        }

        {
          size_t const count = min(indices[0].size(), indices[1].size());
          size_t i = 0;
          for (; i < count; ++i)
            if (indices[0][i].first != indices[1][i].first)
              break;

          if (i == count)
          {
            ++equal;
            LOG(LWARNING, ("Equal choices for way:", seg.wayId, indices[0], indices[1]));
          }
        }

        // Find best matching for multiple choices.
        int ind1 = -1, ind2 = -1, dist = numeric_limits<int>::max();
        for (auto i1 : indices[0])
          for (auto i2 : indices[1])
          {
            int const d = abs(i1.first - i2.first);
            if (d < dist && i1.first != i2.first)
            {
              ind1 = i1.first;
              ind2 = i2.first;
              dist = d;
            }
          }

        if (ind1 != -1 && ind2 != -1)
        {
          ++found;

          // Emit segment.
          OsrmFtSegMapping::FtSeg ftSeg(fID, ind1, ind2);
          if (vec.empty() || !vec.back().Merge(ftSeg))
          {
            vec.push_back(ftSeg);
            ++stored;
          }

          continue;
        }
      }

      // Matching error.
      LOG(LWARNING, ("!!!!! Match not found:", seg.wayId));
      LOG(LWARNING, ("(Lat, Lon):", pts[0].y, pts[0].x, "; (Lat, Lon):", pts[1].y, pts[1].x));
      for (uint32_t j = 0; j < ft.GetPointsCount(); ++j)
      {
        double lon = MercatorBounds::XToLon(ft.GetPoint(j).x);
        double lat = MercatorBounds::YToLat(ft.GetPoint(j).y);
        double const dist1 = ms::DistanceOnEarth(pts[0].y, pts[0].x, lat, lon);
        double const dist2 = ms::DistanceOnEarth(pts[1].y, pts[1].x, lat, lon);
        LOG(LWARNING, ("p", j, ":", lat, lon, "Dist1:", dist1, "Dist2:", dist2));
      }
    }

    if (vec.size() > 1)
      ++moreThan1Seg;

    mapping.Append(nodeId, vec);
  }

  LOG(LINFO, ("All:", all, "Found:", found, "Not found:", all - found, "More that one segs in node:", moreThan1Seg,
              "Multiple:", multiple, "Equal:", equal));
  LOG(LINFO, ("Stored:", stored));

  LOG(LINFO, ("Collect all data into one file..."));

  try
  {
    FilesContainerW writer(mwmName + ROUTING_FILE_EXTENSION);

    mapping.Save(writer);

    auto appendFile = [&] (string const & tag)
    {
      string const fileName = osrmName + "." + tag;
      LOG(LINFO, ("Append file", fileName, "with tag", tag));
      writer.Write(fileName, tag);
    };

    appendFile(ROUTING_SHORTCUTS_FILE_TAG);
    appendFile(ROUTING_EDGEDATA_FILE_TAG);
    appendFile(ROUTING_MATRIX_FILE_TAG);
    appendFile(ROUTING_EDGEID_FILE_TAG);

    writer.Finish();
  }
  catch (RootException const & ex)
  {
    LOG(LCRITICAL, ("Can't write routing index", ex.Msg()));
  }
}

}
