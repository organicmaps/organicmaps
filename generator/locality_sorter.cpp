#include "generator/locality_sorter.hpp"

#include "generator/geometry_holder.hpp"
#include "generator/utils.hpp"

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
#include <functional>
#include <limits>
#include <set>
#include <vector>

using namespace feature;
using namespace std;

namespace
{
class BordersCollector : public FeaturesCollector
{
public:
  BordersCollector(string const & filename)
    : FeaturesCollector(filename + EXTENSION_TMP), m_writer(filename, FileWriter::OP_WRITE_EXISTING)
  {
  }

  // FeaturesCollector overrides:
  uint32_t operator()(FeatureBuilder1 & fb) override
  {
    if (fb.IsArea())
    {
      FeatureBuilder1::Buffer buffer;
      fb.SerializeBorder(serial::GeometryCodingParams(), buffer);
      WriteFeatureBase(buffer, fb);
    }
    return 0;
  }

  void Finish() override
  {
    Flush();

    m_writer.Write(m_datFile.GetName(), BORDERS_FILE_TAG);
    m_writer.Finish();
  }

private:
  FilesContainerW m_writer;
  DataHeader m_header;

  DISALLOW_COPY_AND_MOVE(BordersCollector);
};

class LocalityCollector : public FeaturesCollector
{
public:
  LocalityCollector(string const & filename, DataHeader const & header, uint32_t versionDate)
    : FeaturesCollector(filename + EXTENSION_TMP)
    , m_writer(filename)
    , m_header(header)
    , m_versionDate(versionDate)
  {
  }

  // FeaturesCollector overrides:
  void Finish() override
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

  uint32_t operator()(FeatureBuilder1 & fb1) override
  {
    auto & fb2 = static_cast<FeatureBuilder2 &>(fb1);

    // Do not limit inner triangles number to save all geometry without additional sections.
    GeometryHolder holder(fb2, m_header, numeric_limits<uint32_t>::max() /* maxTrianglesNumber */);

    // Simplify and serialize geometry.
    vector<m2::PointD> points;
    m2::SquaredDistanceFromSegmentToPoint<m2::PointD> distFn;

    SimplifyPoints(distFn, scales::GetUpperScale(), holder.GetSourcePoints(), points);

    // For areas we save outer geometry only.
    if (fb2.IsArea() && holder.NeedProcessTriangles())
    {
      // At this point we don't need last point equal to first.
      points.pop_back();
      auto const & polys = fb2.GetGeometry();
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
          auto const id = fb2.GetMostGenericOsmId();
          CHECK(holder.TryToMakeStrip(hullPoints),
                ("Error while building tringles for object with OSM Id:", id.GetSerialId(),
                 "Type:", id.GetType(), "points:", points, "hull:", hull.Points()));
        }
      }
    }

    auto & buffer = holder.GetBuffer();
    if (fb2.PreSerialize(buffer))
    {
      fb2.SerializeLocalityObject(serial::GeometryCodingParams(), buffer);
      WriteFeatureBase(buffer.m_buffer, fb2);
    }
    return 0;
  }

private:
  FilesContainerW m_writer;
  DataHeader m_header;
  uint32_t m_versionDate;

  DISALLOW_COPY_AND_MOVE(LocalityCollector);
};

bool ParseNodes(string nodesFile, set<uint64_t> & nodeIds)
{
  if (nodesFile.empty())
    return true;

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
  return true;
}

using NeedSerialize = function<bool(FeatureBuilder1 & fb1)>;
bool GenerateLocalityDataImpl(FeaturesCollector & collector, NeedSerialize const & needSerialize,
                              string const & featuresDir, string const & dataFile)
{
  // Transform features from raw format to LocalityObject format.
  try
  {
    Platform::FilesList files;
    Platform::GetFilesByExt(featuresDir, DATA_FILE_EXTENSION_TMP, files);

    for (auto const & filename : files)
    {
      auto const file = my::JoinFoldersToPath(featuresDir, filename);
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
        if (needSerialize(f))
          collector(f);
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
}  // namespace

namespace feature
{
bool GenerateGeoObjectsData(string const & featuresDir, string const & nodesFile,
                            string const & dataFile)
{
  set<uint64_t> nodeIds;
  if (!ParseNodes(nodesFile, nodeIds))
    return false;

  auto const needSerialize = [&nodeIds](FeatureBuilder1 & fb) {
    auto & fb2 = static_cast<FeatureBuilder2 &>(fb);
    return fb2.IsLocalityObject() ||
           (!fb.GetOsmIds().empty() && nodeIds.count(fb.GetMostGenericOsmId().GetEncodedId()) != 0);
  };

  DataHeader header;
  header.SetGeometryCodingParams(serial::GeometryCodingParams());
  header.SetScales({scales::GetUpperScale()});

  LocalityCollector localityCollector(dataFile, header,
                                      static_cast<uint32_t>(my::SecondsSinceEpoch()));
  return GenerateLocalityDataImpl(localityCollector, needSerialize, featuresDir, dataFile);
}

bool GenerateRegionsData(string const & featuresDir, string const & dataFile)
{
  DataHeader header;
  header.SetGeometryCodingParams(serial::GeometryCodingParams());
  header.SetScales({scales::GetUpperScale()});

  LocalityCollector regionsCollector(dataFile, header,
                                     static_cast<uint32_t>(my::SecondsSinceEpoch()));
  auto const needSerialize = [](FeatureBuilder1 const & fb) { return fb.IsArea(); };
  return GenerateLocalityDataImpl(regionsCollector, needSerialize, featuresDir, dataFile);
}

bool GenerateBorders(string const & featuresDir, string const & dataFile)
{
  BordersCollector bordersCollector(dataFile);
  auto const needSerialize = [](FeatureBuilder1 const & fb) { return fb.IsArea(); };
  return GenerateLocalityDataImpl(bordersCollector, needSerialize, featuresDir, dataFile);
}
}  // namespace feature
