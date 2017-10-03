#include "generator/routing_generator.hpp"

#include "generator/borders_generator.hpp"
#include "generator/borders_loader.hpp"
#include "generator/gen_mwm_info.hpp"

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"
#include "routing/osrm_engine.hpp"
#include "routing/cross_routing_context.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "coding/file_container.hpp"
#include "coding/read_write_utils.hpp"

#include "coding/internal/file_data.hpp"

#include "base/logging.hpp"

#include "3party/osrm/osrm-backend/data_structures/edge_based_node_data.hpp"
#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"
#include "3party/osrm/osrm-backend/data_structures/internal_route_result.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace routing
{
using RawRouteResult = InternalRouteResult;

static double const EQUAL_POINT_RADIUS_M = 2.0;

/// Find a feature id for an OSM way id. Returns 0 if the feature was not found.
uint32_t GetRoadFeatureID(gen::OsmID2FeatureID const & osm2ft, uint64_t wayId)
{
  return osm2ft.GetFeatureID(osm::Id::Way(wayId));
}

// For debug purposes only. So I do not use constanst or string representations.
uint8_t GetWarningRank(FeatureType const & ft)
{
  feature::TypesHolder types(ft);
  Classificator const & c = classif();
  if (types.Has(c.GetTypeByPath({"highway", "trunk"})) ||
      types.Has(c.GetTypeByPath({"highway", "trunk_link"})))
    return 1;
  if (types.Has(c.GetTypeByPath({"highway", "primary"})) ||
      types.Has(c.GetTypeByPath({"highway", "primary_link"})) ||
      types.Has(c.GetTypeByPath({"highway", "secondary"})) ||
      types.Has(c.GetTypeByPath({"highway", "secondary_link"})))
    return 2;
  return 3;
}

bool LoadIndexes(std::string const & mwmFile, std::string const & osrmFile, osrm::NodeDataVectorT & nodeData, gen::OsmID2FeatureID & osm2ft)
{
  if (!osrm::LoadNodeDataFromFile(osrmFile + ".nodeData", nodeData))
  {
    LOG(LCRITICAL, ("Can't load node data"));
    return false;
  }

  return osm2ft.ReadFromFile(mwmFile + OSM2FEATURE_FILE_EXTENSION);
}

bool CheckBBoxCrossingBorder(m2::RegionD const & border, osrm::NodeData const & data)
{
  ASSERT(data.m_segments.size(), ());
  double minLon = data.m_segments[0].lon1;
  double minLat = data.m_segments[0].lat1;
  double maxLon = data.m_segments[0].lon1;
  double maxLat = data.m_segments[0].lat1;
  for (auto const & segment : data.m_segments)
  {
    minLon = min(minLon, min(segment.lon1, segment.lon2));
    maxLon = max(maxLon, max(segment.lon1, segment.lon2));
    minLat = min(minLat, min(segment.lat1, segment.lat2));
    maxLat = max(maxLat, max(segment.lat1, segment.lat2));
  }
  bool const upLeft = border.Contains(MercatorBounds::FromLatLon(minLat, minLon));
  bool const upRight = border.Contains(MercatorBounds::FromLatLon(minLat, maxLon));
  bool const downLeft = border.Contains(MercatorBounds::FromLatLon(maxLat, minLon));
  bool const downRight = border.Contains(MercatorBounds::FromLatLon(maxLat, maxLon));

  bool const all = upLeft && upRight && downLeft && downRight;
  bool const any = upLeft || upRight || downLeft || downRight;
  return any && !all;
}

void FindCrossNodes(osrm::NodeDataVectorT const & nodeData, gen::OsmID2FeatureID const & osm2ft,
                    borders::CountriesContainerT const & countries, std::string const & countryName,
                    Index const & index, MwmSet::MwmId mwmId,
                    routing::CrossRoutingContextWriter & crossContext)
{
  vector<m2::RegionD> regionBorders;
  countries.ForEach([&](borders::CountryPolygons const & c)
  {
    if (c.m_name == countryName)
      c.m_regions.ForEach([&regionBorders](m2::RegionD const & region)
      {
        regionBorders.push_back(region);
      });
  });

  for (TWrittenNodeId nodeId = 0; nodeId < nodeData.size(); ++nodeId)
  {
    auto const & data = nodeData[nodeId];

    // Check for outgoing candidates.
    if (!data.m_segments.empty())
    {
      auto const & startSeg = data.m_segments.front();
      auto const & endSeg = data.m_segments.back();
      // Check if we have geometry for our candidate.
      if (GetRoadFeatureID(osm2ft, startSeg.wayId) || GetRoadFeatureID(osm2ft, endSeg.wayId))
      {
        // Check mwm borders crossing.
        for (m2::RegionD const & border: regionBorders)
        {
          if (!CheckBBoxCrossingBorder(border, data))
            continue;

          m2::PointD intersection = m2::PointD::Zero();
          ms::LatLon wgsIntersection = ms::LatLon::Zero();
          size_t intersectionCount = 0;
          for (auto const & segment : data.m_segments)
          {
            bool const outStart =
                border.Contains(MercatorBounds::FromLatLon(segment.lat1, segment.lon1));
            bool const outEnd =
                border.Contains(MercatorBounds::FromLatLon(segment.lat2, segment.lon2));
            if (outStart == outEnd)
              continue;

            if (!border.FindIntersection(MercatorBounds::FromLatLon(segment.lat1, segment.lon1),
                                         MercatorBounds::FromLatLon(segment.lat2, segment.lon2),
                                         intersection))

            {
                ASSERT(false, ("Can't determine a intersection point with a border!"));
                continue;
            }
            intersectionCount++;
            wgsIntersection = MercatorBounds::ToLatLon(intersection);
            if (!outStart && outEnd)
              crossContext.AddIngoingNode(nodeId, wgsIntersection);
            else if (outStart && !outEnd)
            {
              std::string mwmName;
              m2::PointD const & mercatorPoint = MercatorBounds::FromLatLon(endSeg.lat2, endSeg.lon2);
              countries.ForEachInRect(m2::RectD(mercatorPoint, mercatorPoint), [&](borders::CountryPolygons const & c)
              {
                if (c.m_name == countryName)
                  return;
                c.m_regions.ForEachInRect(m2::RectD(mercatorPoint, mercatorPoint), [&](m2::RegionD const & region)
                {
                  // Sometimes Contains make errors for cases near the border.
                  if (region.Contains(mercatorPoint) || region.AtBorder(mercatorPoint, 0.01 /*Near border accuracy. In mercator.*/))
                    mwmName = c.m_name;
                });
              });
              if (!mwmName.empty())
                crossContext.AddOutgoingNode(nodeId, mwmName, wgsIntersection);
              else
                LOG(LINFO, ("Unknowing outgoing edge", endSeg.lat2, endSeg.lon2, startSeg.lat1, startSeg.lon1));
            }
          }
          if (intersectionCount > 1)
          {
            FeatureType ft;
            Index::FeaturesLoaderGuard loader(index, mwmId);
            if (loader.GetFeatureByIndex(GetRoadFeatureID(osm2ft, startSeg.wayId), ft))
            {
              LOG(LINFO,
                  ("Double border intersection", wgsIntersection, "rank:", GetWarningRank(ft)));
            }
          }
        }
      }
    }
  }
}

void CalculateCrossAdjacency(std::string const & mwmRoutingPath, routing::CrossRoutingContextWriter & crossContext)
{
  OsrmDataFacade<QueryEdge::EdgeData> facade;
  FilesMappingContainer routingCont(mwmRoutingPath);
  facade.Load(routingCont);
  LOG(LINFO, ("Calculating weight map between outgoing nodes"));
  crossContext.ReserveAdjacencyMatrix();
  auto const & in = crossContext.GetIngoingIterators();
  auto const & out = crossContext.GetOutgoingIterators();
  TRoutingNodes sources, targets;
  sources.reserve(distance(in.first, in.second));
  targets.reserve(distance(out.first, out.second));
  // Fill sources and targets with start node task for ingoing (true) and target node task
  // (false) for outgoing nodes
  for (auto i = in.first; i != in.second; ++i)
    sources.emplace_back(i->m_nodeId, true /* isStartNode */, Index::MwmId());

  for (auto i = out.first; i != out.second; ++i)
    targets.emplace_back(i->m_nodeId, false /* isStartNode */, Index::MwmId());

  LOG(LINFO, ("Cross section has", sources.size(), "incomes and ", targets.size(), "outcomes."));
  vector<EdgeWeight> costs;
  FindWeightsMatrix(sources, targets, facade, costs);
  auto res = costs.begin();
  for (auto i = in.first; i != in.second; ++i)
    for (auto j = out.first; j != out.second; ++j)
    {
      EdgeWeight const & edgeWeigth = *(res++);
      if (edgeWeigth != INVALID_EDGE_WEIGHT && edgeWeigth > 0)
        crossContext.SetAdjacencyCost(i, j, edgeWeigth);
    }
  LOG(LINFO, ("Calculation of weight map between outgoing nodes DONE"));
}

void WriteCrossSection(routing::CrossRoutingContextWriter const & crossContext, std::string const & mwmRoutingPath)
{
  LOG(LINFO, ("Collect all data into one file..."));

  FilesContainerW routingCont(mwmRoutingPath, FileWriter::OP_WRITE_EXISTING);

  FileWriter w = routingCont.GetWriter(ROUTING_CROSS_CONTEXT_TAG);
  size_t const start_size = w.Pos();
  crossContext.Save(w);
  LOG(LINFO, ("Have written routing info, bytes written:", w.Pos() - start_size, "bytes"));
}

void BuildCrossRoutingIndex(std::string const & baseDir, std::string const & countryName,
                            std::string const & osrmFile)
{
  LOG(LINFO, ("Cross mwm routing section builder"));
  classificator::Load();

  CountryFile countryFile(countryName);
  LocalCountryFile localFile(baseDir, countryFile, 0 /* version */);
  localFile.SyncWithDisk();
  Index index;
  auto p = index.Register(localFile);
  if (p.second != MwmSet::RegResult::Success)
  {
    LOG(LCRITICAL, ("MWM file not found"));
    return;
  }

  LOG(LINFO, ("Loading indexes..."));
  osrm::NodeDataVectorT nodeData;
  gen::OsmID2FeatureID osm2ft;
  if (!LoadIndexes(localFile.GetPath(MapOptions::Map), osrmFile, nodeData, osm2ft))
    return;

  LOG(LINFO, ("Loading countries borders..."));
  borders::CountriesContainerT countries;
  CHECK(borders::LoadCountriesList(baseDir, countries),
        ("Error loading country polygons files"));

  LOG(LINFO, ("Finding cross nodes..."));
  routing::CrossRoutingContextWriter crossContext;
  FindCrossNodes(nodeData, osm2ft, countries, countryName, index, p.first, crossContext);

  std::string const mwmPath = localFile.GetPath(MapOptions::Map);
  CalculateCrossAdjacency(mwmPath, crossContext);
  WriteCrossSection(crossContext, mwmPath);
}

void BuildRoutingIndex(std::string const & baseDir, std::string const & countryName, std::string const & osrmFile)
{
  classificator::Load();

  CountryFile countryFile(countryName);

  // Correct mwm version doesn't matter here - we just need access to mwm files via Index.
  LocalCountryFile localFile(baseDir, countryFile, 0 /* version */);
  localFile.SyncWithDisk();
  Index index;
  auto p = index.Register(localFile);
  if (p.second != MwmSet::RegResult::Success)
  {
    LOG(LCRITICAL, ("MWM file not found"));
    return;
  }
  ASSERT(p.first.IsAlive(), ());

  osrm::NodeDataVectorT nodeData;
  gen::OsmID2FeatureID osm2ft;
  if (!LoadIndexes(localFile.GetPath(MapOptions::Map), osrmFile, nodeData, osm2ft))
    return;

  OsrmFtSegMappingBuilder mapping;

  uint32_t found = 0, all = 0, multiple = 0, equal = 0, moreThan1Seg = 0, stored = 0;

  for (TWrittenNodeId nodeId = 0; nodeId < nodeData.size(); ++nodeId)
  {
    auto const & data = nodeData[nodeId];

    OsrmFtSegMappingBuilder::FtSegVectorT vec;

    for (auto const & seg : data.m_segments)
    {
      m2::PointD const pts[2] = { { seg.lon1, seg.lat1 }, { seg.lon2, seg.lat2 } };
      m2::PointD const segVector = MercatorBounds::FromLatLon(seg.lat2, seg.lon2) -
                                   MercatorBounds::FromLatLon(seg.lat1, seg.lon1);
      ++all;

      // now need to determine feature id and segments in it
      uint32_t const fID = GetRoadFeatureID(osm2ft, seg.wayId);
      if (fID == 0)
      {
        LOG(LWARNING, ("No feature id for way:", seg.wayId));
        continue;
      }

      FeatureType ft;
      Index::FeaturesLoaderGuard loader(index, p.first);
      if (!loader.GetFeatureByIndex(fID, ft))
      {
        LOG(LWARNING, ("Can't read feature with id:", fID, "for way:", seg.wayId));
        continue;
      }

      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

      typedef pair<int, double> IndexT;
      vector<IndexT> indices[2];

      // Match input segment points on feature points.
      for (size_t j = 0; j < ft.GetPointsCount(); ++j)
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
            // We use delta of the indexes to avoid P formed curves cases.
            int const d = abs(i1.first - i2.first);
            if (d < dist && i1.first != i2.first)
            {
              // Check if resulting vector has same direction with the edge.
              m2::PointD candidateVector = ft.GetPoint(i2.first) - ft.GetPoint(i1.first);
              if (m2::DotProduct(candidateVector, segVector) < 0)
                continue;
              ind1 = i1.first;
              ind2 = i2.first;
              dist = d;
            }
          }

        if (ind1 != -1 && ind2 != -1)
        {
          ++found;

          // Emit segment.
          OsrmMappingTypes::FtSeg ftSeg(fID, ind1, ind2);
          if (vec.empty() || !vec.back().Merge(ftSeg))
          {
            vec.push_back(ftSeg);
            ++stored;
          }

          continue;
        }
      }

      // Matching error. Print warning message.
      LOG(LINFO, ("!!!!! Match not found:", seg.wayId));
      LOG(LINFO, ("(Lat, Lon):", pts[0].y, pts[0].x, "; (Lat, Lon):", pts[1].y, pts[1].x));

      int ind1 = -1;
      int ind2 = -1;
      double dist1 = numeric_limits<double>::max();
      double dist2 = numeric_limits<double>::max();
      for (size_t j = 0; j < ft.GetPointsCount(); ++j)
      {
        double lon = MercatorBounds::XToLon(ft.GetPoint(j).x);
        double lat = MercatorBounds::YToLat(ft.GetPoint(j).y);
        double const d1 = ms::DistanceOnEarth(pts[0].y, pts[0].x, lat, lon);
        double const d2 = ms::DistanceOnEarth(pts[1].y, pts[1].x, lat, lon);
        if (d1 < dist1)
        {
          ind1 = static_cast<int>(j);
          dist1 = d1;
        }
        if (d2 < dist2)
        {
          ind2 = static_cast<int>(j);
          dist2 = d2;
        }
      }

      LOG(LINFO, ("ind1 =", ind1, "ind2 =", ind2, "dist1 =", dist1, "dist2 =", dist2));
    }

    if (vec.size() > 1)
      ++moreThan1Seg;
    mapping.Append(nodeId, vec);
  }

  LOG(LINFO, ("All:", all, "Found:", found, "Not found:", all - found, "More that one segs in node:", moreThan1Seg,
              "Multiple:", multiple, "Equal:", equal));

  LOG(LINFO, ("Collect all data into one file..."));

  std::string const mwmPath = localFile.GetPath(MapOptions::Map);
  std::string const mwmWithoutRoutingPath = mwmPath + NOROUTING_FILE_EXTENSION;

  // Backup mwm file without routing.
  CHECK(my::CopyFileX(mwmPath, mwmWithoutRoutingPath), ("Can't copy", mwmPath, "to", mwmWithoutRoutingPath));

  FilesContainerW routingCont(mwmPath, FileWriter::OP_WRITE_EXISTING);

  mapping.Save(routingCont);

  auto appendFile = [&] (std::string const & tag)
  {
    std::string const fileName = osrmFile + "." + tag;
    LOG(LINFO, ("Append file", fileName, "with tag", tag));
    routingCont.Write(fileName, tag);
  };

  appendFile(ROUTING_SHORTCUTS_FILE_TAG);
  appendFile(ROUTING_EDGEDATA_FILE_TAG);
  appendFile(ROUTING_MATRIX_FILE_TAG);
  appendFile(ROUTING_EDGEID_FILE_TAG);

  routingCont.Finish();

  uint64_t sz;
  VERIFY(my::GetFileSize(mwmPath, sz), ());
  LOG(LINFO, ("Nodes stored:", stored, "Routing index file size:", sz));
}
}
