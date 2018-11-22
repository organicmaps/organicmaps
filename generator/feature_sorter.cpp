#include "generator/feature_sorter.hpp"

#include "generator/borders_loader.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/gen_mwm_info.hpp"
#include "generator/geometry_holder.hpp"
#include "generator/region_meta.hpp"
#include "generator/tesselator.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"
#include "indexer/scales_patch.hpp"

#include "platform/mwm_version.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/point_coding.hpp"

#include "geometry/polygon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <list>
#include <limits>
#include <memory>
#include <vector>

using namespace std;

namespace feature
{
class FeaturesCollector2 : public FeaturesCollector
{
public:
  FeaturesCollector2(string const & fName, DataHeader const & header, RegionData const & regionData,
                     uint32_t versionDate)
    : FeaturesCollector(fName + DATA_FILE_TAG)
    , m_writer(fName)
    , m_header(header)
    , m_regionData(regionData)
    , m_versionDate(versionDate)
  {
    for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
    {
      string const postfix = strings::to_string(i);
      m_geoFile.push_back(make_unique<TmpFile>(fName + GEOMETRY_FILE_TAG + postfix));
      m_trgFile.push_back(make_unique<TmpFile>(fName + TRIANGLE_FILE_TAG + postfix));
    }

    m_helperFile.resize(FILES_COUNT);
    m_helperFile[METADATA] = make_unique<TmpFile>(fName + METADATA_FILE_TAG);
    m_helperFile[SEARCH_TOKENS] = make_unique<TmpFile>(fName + SEARCH_TOKENS_FILE_TAG);
  }

  ~FeaturesCollector2()
  {
    // Check file size.
    auto const unused = CheckedFilePosCast(m_datFile);
    UNUSED_VALUE(unused);
  }

  void Finish()
  {
    // write version information
    {
      FileWriter w = m_writer.GetWriter(VERSION_FILE_TAG);
      version::WriteVersion(w, m_versionDate);
    }

    // write own mwm header
    m_header.SetBounds(m_bounds);
    {
      FileWriter w = m_writer.GetWriter(HEADER_FILE_TAG);
      m_header.Save(w);
    }

    // write region info
    {
      FileWriter w = m_writer.GetWriter(REGION_INFO_FILE_TAG);
      m_regionData.Serialize(w);
    }

    // assume like we close files
    Flush();

    m_writer.Write(m_datFile.GetName(), DATA_FILE_TAG);

    // File Writer finalization function with appending to the main mwm file.
    auto const finalizeFn = [this](unique_ptr<TmpFile> w, string const & tag,
        string const & postfix = string()) {
      w->Flush();
      m_writer.Write(w->GetName(), tag + postfix);
    };

    for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
    {
      string const postfix = strings::to_string(i);
      finalizeFn(move(m_geoFile[i]), GEOMETRY_FILE_TAG, postfix);
      finalizeFn(move(m_trgFile[i]), TRIANGLE_FILE_TAG, postfix);
    }

    {
      /// @todo Replace this mapping vector with succint structure.
      FileWriter w = m_writer.GetWriter(METADATA_INDEX_FILE_TAG);
      for (auto const & v : m_metadataOffset)
      {
        WriteToSink(w, v.first);
        WriteToSink(w, v.second);
      }
    }

    finalizeFn(move(m_helperFile[METADATA]), METADATA_FILE_TAG);
    finalizeFn(move(m_helperFile[SEARCH_TOKENS]), SEARCH_TOKENS_FILE_TAG);

    m_writer.Finish();

    if (m_header.GetType() == DataHeader::country || m_header.GetType() == DataHeader::world)
    {
      FileWriter osm2ftWriter(m_writer.GetFileName() + OSM2FEATURE_FILE_EXTENSION);
      m_osm2ft.Flush(osm2ftWriter);
    }
  }

  void SetBounds(m2::RectD bounds) { m_bounds = bounds; }

  uint32_t operator()(FeatureBuilder2 & fb)
  {
    GeometryHolder holder([this](int i) -> FileWriter & { return *m_geoFile[i]; },
    [this](int i) -> FileWriter & { return *m_trgFile[i]; }, fb, m_header);

    bool const isLine = fb.IsLine();
    bool const isArea = fb.IsArea();

    int const scalesStart = static_cast<int>(m_header.GetScalesCount()) - 1;
    for (int i = scalesStart; i >= 0; --i)
    {
      int const level = m_header.GetScale(i);
      if (fb.IsDrawableInRange(i > 0 ? m_header.GetScale(i - 1) + 1 : 0, PatchScaleBound(level)))
      {
        bool const isCoast = fb.IsCoastCell();
        m2::RectD const rect = fb.GetLimitRect();

        // Simplify and serialize geometry.
        Points points;

        // Do not change linear geometry for the upper scale.
        if (isLine && i == scalesStart && IsCountry() && fb.IsRoad())
          points = holder.GetSourcePoints();
        else
          SimplifyPoints(level, isCoast, rect, holder.GetSourcePoints(), points);

        if (isLine)
          holder.AddPoints(points, i);

        if (isArea && holder.NeedProcessTriangles())
        {
          // simplify and serialize triangles
          bool const good = isCoast || IsGoodArea(points, level);

          // At this point we don't need last point equal to first.
          CHECK_GREATER(points.size(), 0, ());
          points.pop_back();

          Polygons const & polys = fb.GetGeometry();
          if (polys.size() == 1 && good && holder.TryToMakeStrip(points))
            continue;

          Polygons simplified;
          if (good)
          {
            simplified.push_back({});
            simplified.back().swap(points);
          }

          auto iH = polys.begin();
          for (++iH; iH != polys.end(); ++iH)
          {
            simplified.push_back({});

            SimplifyPoints(level, isCoast, rect, *iH, simplified.back());

            // Increment level check for coastline polygons for the first scale level.
            // This is used for better coastlines quality.
            if (IsGoodArea(simplified.back(), (isCoast && i == 0) ? level + 1 : level))
            {
              // At this point we don't need last point equal to first.
              CHECK_GREATER(simplified.back().size(), 0, ());
              simplified.back().pop_back();
            }
            else
            {
              // Remove small polygon.
              simplified.pop_back();
            }
          }

          if (!simplified.empty())
            holder.AddTriangles(simplified, i);
        }
      }
    }

    uint32_t featureId = kInvalidFeatureId;
    auto & buffer = holder.GetBuffer();
    if (fb.PreSerializeAndRemoveUselessNames(buffer))
    {
      fb.Serialize(buffer, m_header.GetDefGeometryCodingParams());

      featureId = WriteFeatureBase(buffer.m_buffer, fb);

      fb.GetAddressData().Serialize(*(m_helperFile[SEARCH_TOKENS]));

      if (!fb.GetMetadata().Empty())
      {
        auto const & w = m_helperFile[METADATA];

        uint64_t const offset = w->Pos();
        ASSERT_LESS_OR_EQUAL(offset, numeric_limits<uint32_t>::max(), ());

        m_metadataOffset.emplace_back(featureId, static_cast<uint32_t>(offset));
        fb.GetMetadata().Serialize(*w);
      }

      if (!fb.GetOsmIds().empty())
      {
        base::GeoObjectId const osmId = fb.GetMostGenericOsmId();
        m_osm2ft.Add(make_pair(osmId, featureId));
      }
    };
    return featureId;
  }

private:
  using Points = vector<m2::PointD>;
  using Polygons = list<Points>;

  class TmpFile : public FileWriter
  {
  public:
    explicit TmpFile(string const & filePath) : FileWriter(filePath) {}
    ~TmpFile() { DeleteFileX(GetName()); }
  };

  using TmpFiles = vector<unique_ptr<TmpFile>>;

  enum
  {
    METADATA = 0,
    SEARCH_TOKENS = 1,
    FILES_COUNT = 2
  };

  static bool IsGoodArea(Points const & poly, int level)
  {
    // Area has the same first and last points. That's why minimal number of points for
    // area is 4.
    if (poly.size() < 4)
      return false;

    m2::RectD r;
    CalcRect(poly, r);

    return scales::IsGoodForLevel(level, r);
  }

  bool IsCountry() const { return m_header.GetType() == feature::DataHeader::country; }

  void SimplifyPoints(int level, bool isCoast, m2::RectD const & rect, Points const & in,
                      Points & out)
  {
    if (isCoast)
    {
      DistanceToSegmentWithRectBounds fn(rect);
      feature::SimplifyPoints(fn, level, in, out);
    }
    else
    {
      feature::SimplifyPoints(m2::SquaredDistanceFromSegmentToPoint<m2::PointD>(), level, in, out);
    }
  }

  FilesContainerW m_writer;
  TmpFiles m_helperFile;
  TmpFiles m_geoFile, m_trgFile;

  // Mapping from feature id to offset in file section with the correspondent metadata.
  vector<pair<uint32_t, uint32_t>> m_metadataOffset;

  DataHeader m_header;
  RegionData m_regionData;
  uint32_t m_versionDate;

  gen::OsmID2FeatureID m_osm2ft;

  DISALLOW_COPY_AND_MOVE(FeaturesCollector2);
};

/// Simplify geometry for the upper scale.
FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
{
  return static_cast<FeatureBuilder2 &>(fb);
}

bool GenerateFinalFeatures(feature::GenerateInfo const & info, string const & name, int mapType)
{
  string const srcFilePath = info.GetTmpFileName(name);
  string const datFilePath = info.GetTargetFileName(name);

  // stores cellIds for middle points
  CalculateMidPoints midPoints;
  ForEachFromDatRawFormat(srcFilePath, midPoints);

  // sort features by their middle point
  midPoints.Sort();

  // store sorted features
  {
    FileReader reader(srcFilePath);

    bool const isWorld = (mapType != DataHeader::country);

    // Fill mwm header.
    DataHeader header;

    uint8_t coordBits = kFeatureSorterPointCoordBits;
    if (isWorld)
      coordBits -= ((scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2);

    // coding params
    header.SetGeometryCodingParams(serial::GeometryCodingParams(coordBits, midPoints.GetCenter()));

    // scales
    if (isWorld)
      header.SetScales(g_arrWorldScales);
    else
      header.SetScales(g_arrCountryScales);

    // type
    header.SetType(static_cast<DataHeader::MapType>(mapType));

    // region data
    RegionData regionData;
    if (!ReadRegionData(name, regionData))
      LOG(LWARNING, ("No extra data for country:", name));

    // Transform features from raw format to optimized format.
    try
    {
      FeaturesCollector2 collector(datFilePath, header, regionData, info.m_versionDate);

      for (auto const & point : midPoints.GetVector())
      {
        ReaderSource<FileReader> src(reader);
        src.Skip(point.second);

        FeatureBuilder1 f;
        ReadFromSourceRawFormat(src, f);

        // emit the feature
        collector(GetFeatureBuilder2(f));
      }

      // Update bounds with the limit rect corresponding to region borders.
      // Bounds before update can be too big because of big invisible features like a
      // relation that contains an entire country's border.
      // Borders file may be unavailable when building test mwms.
      m2::RectD bordersRect;
      if (borders::GetBordersRect(info.m_targetDir, name, bordersRect))
        collector.SetBounds(bordersRect);

      collector.Finish();
    }
    catch (RootException const & ex)
    {
      LOG(LCRITICAL, ("MWM writing error:", ex.Msg()));
    }

    // at this point files should be closed
  }

  // remove old not-sorted dat file
  FileWriter::DeleteFileX(datFilePath + DATA_FILE_TAG);

  return true;
}
}  // namespace feature
