#include "generator/locality_sorter.hpp"

#include "generator/geometry_holder.hpp"

#include "indexer/data_header.hpp"
#include "indexer/scales.hpp"
#include "indexer/scales_patch.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"

#include "geometry/convex_hull.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"
#include "base/timer.hpp"

#include "defines.hpp"

#include <cstdint>
#include <fstream>
#include <limits>
#include <set>
#include <vector>

using namespace feature;
using namespace std;

class LocalityCollector : public FeaturesCollector
{
  DISALLOW_COPY_AND_MOVE(LocalityCollector);

  FilesContainerW m_writer;
  DataHeader m_header;
  uint32_t m_versionDate;

public:
  LocalityCollector(string const & fName, DataHeader const & header, uint32_t versionDate)
    : FeaturesCollector(fName + EXTENSION_TMP)
    , m_writer(fName)
    , m_header(header)
    , m_versionDate(versionDate)
  {
  }

  void Finish()
  {
    {
      FileWriter w = m_writer.GetWriter(VERSION_FILE_TAG);
      version::WriteVersion(w, m_versionDate);
    }

    m_header.SetBounds(m_bounds);
    {
      FileWriter w = m_writer.GetWriter(HEADER_FILE_TAG);
      m_header.Save(w);
    }

    Flush();

    m_writer.Write(m_datFile.GetName(), LOCALITY_DATA_FILE_TAG);
    m_writer.Finish();
  }

  void operator()(FeatureBuilder2 & fb)
  {
    // Do not limit inner triangles number to save all geometry without additional sections.
    GeometryHolder holder(fb, m_header, numeric_limits<uint32_t>::max() /* maxTrianglesNumber */);

    // Simplify and serialize geometry.
    vector<m2::PointD> points;
    m2::DistanceToLineSquare<m2::PointD> dist;

    SimplifyPoints(dist, scales::GetUpperScale(), holder.GetSourcePoints(), points);

    // For areas we save outer geometry only.
    if (fb.IsArea() && holder.NeedProcessTriangles())
    {
      // At this point we don't need last point equal to first.
      points.pop_back();
      auto const & polys = fb.GetGeometry();
      if (polys.size() != 1)
      {
        points.clear();
        for (auto const & poly : polys)
          points.insert(points.end(), poly.begin(), poly.end());
      }

      if (points.size() > 2)
      {
        if (!holder.TryToMakeStrip(points))
        {
          m2::ConvexHull hull(points, 1e-16);
          vector<m2::PointD> hullPoints = hull.Points();
          holder.SetInner();
          auto const id = fb.GetMostGenericOsmId();
          CHECK(holder.TryToMakeStrip(hullPoints),
                ("Error while building tringles for object with OSM Id:", id.OsmId(),
                 "Type:", id.IsRelation() ? "Relation" : "Way", "points:", points,
                 "hull:", hull.Points()));
        }
      }
    }

    auto & buffer = holder.GetBuffer();
    if (fb.PreSerialize(buffer))
    {
      fb.SerializeLocalityObject(serial::GeometryCodingParams(), buffer);
      WriteFeatureBase(buffer.m_buffer, fb);
    }
  }
};

// Simplify geometry for the upper scale.
FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
{
  return static_cast<FeatureBuilder2 &>(fb);
}

namespace feature
{
bool GenerateLocalityData(string const & featuresDir, string const & nodesFile,
                          string const & dataFile)
{
  DataHeader header;
  header.SetGeometryCodingParams(serial::GeometryCodingParams());
  header.SetScales({scales::GetUpperScale()});

  set<uint64_t> nodeIds;
  if (!nodesFile.empty())
  {
    ifstream stream(nodesFile);
    if (!stream)
    {
      LOG(LERROR, ("Could not open", nodesFile));
      return false;
    }

    string line;
    size_t lineNumber = 1;
    while (getline(stream, line))
    {
      strings::SimpleTokenizer iter(line, " ");
      uint64_t nodeId;
      if (!iter || !strings::to_uint64(*iter, nodeId))
      {
        LOG(LERROR, ("Error while parsing node id at line", lineNumber, "Line contents:", line));
        return false;
      }

      nodeIds.insert(nodeId);
      ++lineNumber;
    }
  }

  // Transform features from raw format to LocalityObject format.
  try
  {
    LocalityCollector collector(dataFile, header,
                                static_cast<uint32_t>(my::SecondsSinceEpoch()));

    Platform::FilesList files;
    Platform::GetFilesByExt(featuresDir, DATA_FILE_EXTENSION_TMP, files);

    for (auto const & fileName : files)
    {
      auto const file = my::JoinFoldersToPath(featuresDir, fileName);
      LOG(LINFO, ("Processing", file));

      CalculateMidPoints midPoints;
      ForEachFromDatRawFormat(file, midPoints);

      // Sort features by their middle point.
      midPoints.Sort();

      FileReader reader(file);
      for (auto const & point : midPoints.GetVector())
      {
        ReaderSource<FileReader> src(reader);
        src.Skip(point.second);

        FeatureBuilder1 f;
        ReadFromSourceRowFormat(src, f);
        // Emit object.
        auto & fb2 = GetFeatureBuilder2(f);
        if (fb2.IsLocalityObject() ||
            (!fb2.GetOsmIds().empty() && nodeIds.count(fb2.GetMostGenericOsmId().EncodedId()) != 0))
        {
          collector(fb2);
        }
      }
    }

    collector.Finish();
  }
  catch (RootException const & ex)
  {
    LOG(LCRITICAL, ("Locality data writing error:", ex.Msg()));
    return false;
  }

  return true;
}
}  // namespace feature
