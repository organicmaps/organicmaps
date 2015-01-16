#include "routing_generator.hpp"
#include "gen_mwm_info.hpp"
#include "borders_generator.hpp"
#include "borders_loader.hpp"

#include "../routing/osrm2feature_map.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"
#include "../indexer/mercator.hpp"

#include "../coding/file_container.hpp"
#include "../coding/internal/file_data.hpp"
#include "../coding/read_write_utils.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../base/logging.hpp"

#include "../std/fstream.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/EdgeBasedNodeData.h"
#include "../3party/osrm/osrm-backend/Server/DataStructures/BaseDataFacade.h"
#include "../3party/succinct/mapper.hpp"

#define BORDERS_DIR "borders/"
#define BORDERS_EXTENSION ".borders"

namespace routing
{

static double const EQUAL_POINT_RADIUS_M = 2.0;

void BuildRoutingIndex(string const & baseDir, string const & countryName, string const & osrmFile)
{
  classificator::Load();

  string const mwmFile = baseDir + countryName + DATA_FILE_EXTENSION;
  Index index;
  m2::RectD rect;
  if (!index.Add(mwmFile, rect))
  {
    LOG(LCRITICAL, ("MWM file not found"));
    return;
  }

  vector<m2::RegionD> regionBorders;
  OutgoingVectorT outgoingNodes;
  LOG(LINFO, ("Loading countries borders"));
  {
    vector<m2::RegionD> tmpRegionBorders;
    if (osm::LoadBorders(baseDir + BORDERS_DIR + countryName + BORDERS_EXTENSION, tmpRegionBorders))
    {
      LOG(LINFO, ("Found",tmpRegionBorders.size(),"region borders"));
      for (m2::RegionD const& border: tmpRegionBorders)
      {
        m2::RegionD finalBorder;
        finalBorder.Data().reserve(border.Data().size());
        for (auto p = tmpRegionBorders.data()->Begin(); p<tmpRegionBorders.data()->End(); ++p)
          finalBorder.AddPoint(m2::PointD(MercatorBounds::XToLon(p->x), MercatorBounds::YToLat(p->y)));
        regionBorders.push_back(finalBorder);
      }
    }
  }

  gen::OsmID2FeatureID osm2ft;
  {
    FileReader reader(mwmFile + OSM2FEATURE_FILE_EXTENSION);
    ReaderSource<FileReader> src(reader);
    osm2ft.Read(src);
  }

  osrm::NodeDataVectorT nodeData;
  if (!osrm::LoadNodeDataFromFile(osrmFile + ".nodeData", nodeData))
  {
    LOG(LCRITICAL, ("Can't load node data"));
    return;
  }

  OsrmFtSegMappingBuilder mapping;

  uint32_t found = 0, all = 0, multiple = 0, equal = 0, moreThan1Seg = 0, stored = 0;

  for (size_t nodeId = 0; nodeId < nodeData.size(); ++nodeId)
  {
    auto const & data = nodeData[nodeId];

    //Check for outgoing candidates
    if (data.m_segments.size() > 0)
    {
      auto const & startSeg = data.m_segments.front();
      auto const & endSeg = data.m_segments.back();
      // Check if we have geometry for our candidate
      if (osm2ft.GetFeatureID(startSeg.wayId) || osm2ft.GetFeatureID(endSeg.wayId))
      {
        m2::PointD pts[2] = { { startSeg.lon1, startSeg.lat1 }, { endSeg.lon2, endSeg.lat2 } };

        // check nearest to goverment borders
        for (m2::RegionD const& border: regionBorders)
        {
          if (border.atBorder(pts[0], 0.001) || border.atBorder(pts[1], 0.001))
          {
            outgoingNodes.push_back(nodeId);
            //outgoingNodes.push_back({nodeId, pts[0].x, pts[0].y});
            //outgoingNodes.push_back(pts[0].x); //seg.wayId);
            //outgoingNodes.push_back(pts[0].y);
            //LOG(LINFO, ("PUSH TO OUTGOINF",pts[0].x,pts[0].y));
          }
        }
      }
    }

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
/*
    if (vec.size())
    for (m2::RegionD const& border: regionBorders)
    {
      m2::PointD pts[2] = { { data.m_segments.front().lon1, data.m_segments.front().lat1 }, { data.m_segments.back().lon2, data.m_segments.back().lat2 } };
      if (border.atBorder(pts[0], 0.0001) || border.atBorder(pts[1], 0.0001))
        for (auto v: vec)
        {
          outgoingNodes.push_back(v.Store()); //nodeId); //seg.wayId);
        }
    }
*/
    mapping.Append(nodeId, vec);
  }

  LOG(LINFO, ("All:", all, "Found:", found, "Not found:", all - found, "More that one segs in node:", moreThan1Seg,
              "Multiple:", multiple, "Equal:", equal));

  LOG(LINFO, ("At border", outgoingNodes.size()));

  LOG(LINFO, ("Collect all data into one file..."));
  string const fPath = mwmFile + ROUTING_FILE_EXTENSION;

  FilesContainerW routingCont(fPath);

  {
    // Write version for routing file that is equal to correspondent mwm file.
    FilesContainerR mwmCont(mwmFile);

    FileWriter w = routingCont.GetWriter(VERSION_FILE_TAG);
    ReaderSource<ModelReaderPtr> src(mwmCont.GetReader(VERSION_FILE_TAG));
    rw::ReadAndWrite(src, w);
    w.WritePadding(4);
  }

  mapping.Save(routingCont);

  auto appendFile = [&] (string const & tag)
  {
    string const fileName = osrmFile + "." + tag;
    LOG(LINFO, ("Append file", fileName, "with tag", tag));
    routingCont.Write(fileName, tag);
  };

  appendFile(ROUTING_SHORTCUTS_FILE_TAG);
  appendFile(ROUTING_EDGEDATA_FILE_TAG);
  appendFile(ROUTING_MATRIX_FILE_TAG);
  appendFile(ROUTING_EDGEID_FILE_TAG);

  {
    LOG(LINFO, ("Filtering outgoing candidates"));
    //Sort nodes
    sort(outgoingNodes.begin(), outgoingNodes.end());
    outgoingNodes.erase(
        unique(outgoingNodes.begin(), outgoingNodes.end()),
        outgoingNodes.end());

    OutgoingVectorT finalOutgoingNodes;

    // Load node connectivity matrix
    succinct::elias_fano matrix;
    ReaderSource<ModelReaderPtr> matrixSource(new FileReader(osrmFile + "." + ROUTING_MATRIX_FILE_TAG));
    vector<char> buffer(static_cast<size_t>(matrixSource.Size()));
    matrixSource.Read(&buffer[0], matrixSource.Size());
    const uint32_t matrixNumberOfNodes = *reinterpret_cast<uint32_t *>(&buffer[0]);
    succinct::mapper::map(matrix, (const char *)&buffer[0] + sizeof(matrixNumberOfNodes));

    map<size_t, size_t> incomeEdgeCountMap;
    size_t const nodeDataSize = nodeData.size();
    for (size_t nodeId = 0; nodeId < nodeDataSize; ++nodeId)
    {
      uint64_t const begin_idx = 2 * nodeId * (uint64_t)matrixNumberOfNodes;
      auto begin = nodeId == 0 ? 0 : matrix.rank(min(begin_idx, matrix.size()));
      uint64_t const end_idx = 2 * (nodeId+1) * (uint64_t)matrixNumberOfNodes;
      auto const end = matrix.rank(min(end_idx, matrix.size()));
      for(auto e: osrm::irange(begin, end))
      {
        const size_t target_node = (matrix.select(e) / 2) % matrixNumberOfNodes;
        auto const it = incomeEdgeCountMap.find(target_node);
        if (it==incomeEdgeCountMap.end())
          incomeEdgeCountMap.insert(make_pair(target_node, 1));
        else
          incomeEdgeCountMap.insert(make_pair(target_node, it->second+1));
      }
    }



    for (auto nodeId: outgoingNodes)
    {
      // Check if there is no outgoing nodes
      uint64_t const begin_idx = 2 * nodeId * (uint64_t)matrixNumberOfNodes;
      auto const begin = nodeId == 0 ? 0 : matrix.rank(min(begin_idx, matrix.size()));
      uint64_t const end_idx = 2 * (nodeId+1) * (uint64_t)matrixNumberOfNodes;
      auto const end = matrix.rank(min(end_idx, matrix.size()));
      if (end-begin == 0) // no outgoing edges
      {
        finalOutgoingNodes.push_back(nodeId);
        continue;
      }

      // Check if there is no income nodes
      auto const it = incomeEdgeCountMap.find(nodeId);
      if (it == incomeEdgeCountMap.end() || it->second==1) //No income or self income
        finalOutgoingNodes.push_back(nodeId);
    }
    LOG(LINFO, ("Outgoing filering done! Have: ", finalOutgoingNodes.size(), "outgoing nodes"));
    //for (auto n: outgoingNodes)
    //  LOG(LINFO, ("Node: ", n));
    // Write outs information
    FileWriter w = routingCont.GetWriter(ROUTING_OUTGOING_FILE_TAG);
    rw::WriteVectorOfPOD(w, finalOutgoingNodes);
    w.WritePadding(4);
  }

  routingCont.Finish();

  uint64_t sz;
  VERIFY(my::GetFileSize(fPath, sz), ());
  LOG(LINFO, ("Nodes stored:", stored, "Routing index file size:", sz));
}


void BuildCrossesRoutingIndex(string const & baseDir)
{
  vector<size_t> outgoingNodes1;
  vector<size_t> outgoingNodes2;
  vector<size_t> result;
  const string file1_path = baseDir+"Russia_central.mwm.routing";
  const string file2_path = baseDir+"Belarus.mwm.routing";
  FilesMappingContainer f1Cont(file1_path);
  FilesMappingContainer f2Cont(file2_path);
  ReaderSource<FileReader> r1(f1Cont.GetReader(ROUTING_OUTGOING_FILE_TAG));
  ReaderSource<FileReader> r2(f2Cont.GetReader(ROUTING_OUTGOING_FILE_TAG));

  LOG(LINFO, ("Loading mwms"));
  /*ReaderPtr<Reader> r1 = f1Cont.GetReader(ROUTING_OUTGOING_FILE_TAG);
  ReaderPtr<Reader> r2 = f2Cont.GetReader(ROUTING_OUTGOING_FILE_TAG);*/
  rw::ReadVectorOfPOD(r1,outgoingNodes1);
  rw::ReadVectorOfPOD(r2,outgoingNodes2);

  LOG(LINFO, ("Have outgoing lengthes", outgoingNodes1.size(), outgoingNodes2.size()));
  sort(outgoingNodes1.begin(), outgoingNodes1.end());
  sort(outgoingNodes2.begin(), outgoingNodes2.end());
  set_intersection(outgoingNodes1.begin(),outgoingNodes1.end(),outgoingNodes2.begin(),outgoingNodes2.end(),back_inserter(result));
  LOG(LINFO, ("Have nodes after intersection", result.size()));

}

}
