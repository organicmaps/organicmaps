#include "generator/feature_sorter.hpp"

#include "generator/borders.hpp"
#include "generator/boundary_postcodes_enricher.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/gen_mwm_info.hpp"
#include "generator/geometry_holder.hpp"
#include "generator/region_meta.hpp"
#include "generator/search_index_builder.hpp"

#include "routing/routing_helpers.hpp"

#include "indexer/dat_section_header.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/scales.hpp"
#include "indexer/scales_patch.hpp"

#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/point_coding.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <limits>
#include <list>
#include <memory>
#include <vector>

namespace feature
{

class FeaturesCollector2 : public FeaturesCollector
{
public:
  FeaturesCollector2(std::string const & name, feature::GenerateInfo const & info, DataHeader const & header,
                     RegionData const & regionData, uint32_t versionDate)
    : FeaturesCollector(info.GetTargetFileName(name, FEATURES_FILE_TAG))
    , m_filename(info.GetTargetFileName(name))
    , m_boundaryPostcodesEnricher(info.GetIntermediateFileName(BOUNDARY_POSTCODES_FILENAME))
    , m_header(header)
    , m_regionData(regionData)
    , m_versionDate(versionDate)
  {
    for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
    {
      std::string const postfix = strings::to_string(i);
      m_geoFile.push_back(std::make_unique<TmpFile>(info.GetIntermediateFileName(name, GEOMETRY_FILE_TAG + postfix)));
      m_trgFile.push_back(std::make_unique<TmpFile>(info.GetIntermediateFileName(name, TRIANGLE_FILE_TAG + postfix)));
    }

    m_addrFile =
        std::make_unique<FileWriter>(info.GetIntermediateFileName(name + DATA_FILE_EXTENSION, TEMP_ADDR_EXTENSION));
  }

  void Finish() override
  {
    // write version information
    {
      FilesContainerW writer(m_filename);
      auto w = writer.GetWriter(VERSION_FILE_TAG);
      version::WriteVersion(*w, m_versionDate);
    }

    // write own mwm header
    m_header.SetBounds(m_bounds);
    {
      FilesContainerW writer(m_filename, FileWriter::OP_WRITE_EXISTING);
      auto w = writer.GetWriter(HEADER_FILE_TAG);
      m_header.Save(*w);
    }

    // write region info
    {
      FilesContainerW writer(m_filename, FileWriter::OP_WRITE_EXISTING);
      auto w = writer.GetWriter(REGION_INFO_FILE_TAG);
      m_regionData.Serialize(*w);
    }

    // assume like we close files
    Flush();

    {
      FilesContainerW writer(m_filename, FileWriter::OP_WRITE_EXISTING);
      auto w = writer.GetWriter(FEATURES_FILE_TAG);

      size_t const startOffset = w->Pos();
      CHECK(coding::IsAlign8(startOffset), ());

      feature::DatSectionHeader header;
      header.Serialize(*w);

      uint64_t bytesWritten = static_cast<uint64_t>(w->Pos());
      coding::WritePadding(*w, bytesWritten);

      header.m_featuresOffset = base::asserted_cast<uint32_t>(w->Pos() - startOffset);
      ReaderSource<ModelReaderPtr> src(std::make_unique<FileReader>(m_dataFile.GetName()));
      rw::ReadAndWrite(src, *w);
      header.m_featuresSize = base::asserted_cast<uint32_t>(w->Pos() - header.m_featuresOffset - startOffset);

      auto const endOffset = w->Pos();
      w->Seek(startOffset);
      header.Serialize(*w);
      w->Seek(endOffset);
    }

    // File Writer finalization function with adding section to the main mwm file.
    auto const finalizeFn = [this](std::unique_ptr<TmpFile> && tmpFile, std::string const & tag)
    {
      auto & w = tmpFile->GetWriter();
      w.Flush();
      FilesContainerW writer(m_filename, FileWriter::OP_WRITE_EXISTING);
      writer.Write(w.GetName(), tag);
    };

    for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
    {
      finalizeFn(std::move(m_geoFile[i]), GetTagForIndex(GEOMETRY_FILE_TAG, i));
      finalizeFn(std::move(m_trgFile[i]), GetTagForIndex(TRIANGLE_FILE_TAG, i));
    }

    {
      FilesContainerW writer(m_filename, FileWriter::OP_WRITE_EXISTING);
      auto w = writer.GetWriter(METADATA_FILE_TAG);
      m_metadataBuilder.Freeze(*w);
    }

    if (m_header.GetType() == DataHeader::MapType::Country || m_header.GetType() == DataHeader::MapType::World)
    {
      FileWriter osm2ftWriter(m_filename + OSM2FEATURE_FILE_EXTENSION);
      m_osm2ft.Write(osm2ftWriter);
    }
  }

  void SetBounds(m2::RectD const & bounds) { m_bounds = bounds; }

  void operator()(FeatureBuilder & fb)
  {
    GeometryHolder holder([this](int i) -> FileWriter & { return m_geoFile[i]->GetWriter(); },
                          [this](int i) -> FileWriter & { return m_trgFile[i]->GetWriter(); }, fb, m_header);

    if (!fb.IsPoint())
    {
      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();
      CHECK(!isLine || !isArea, (fb.GetMostGenericOsmId()));

      m2::RectD const & rect = fb.GetLimitRect();
      Polygons const & polys = fb.GetGeometry();
      bool const isCoast = fb.IsCoastCell();

      int const scalesStart = static_cast<int>(m_header.GetScalesCount()) - 1;
      for (int i = scalesStart; i >= 0; --i)
      {
        int level = m_header.GetScale(i);
        /// @todo: Re-checks geom limit rect size via IsDrawableForIndexGeometryOnly()
        /// which was already checked in CalculateMidPoints.
        if (fb.IsDrawableInRange(scales::PatchMinDrawableScale(i > 0 ? m_header.GetScale(i - 1) + 1 : 0),
                                 scales::PatchMaxDrawableScale(level)))
        {
          // Increment zoom level for coastline polygons (check and simplification)
          // for better visual quality in the first geometry batch or whole WorldCoasts.
          /// @todo Probably, better to keep 3 zooms (skip trg0 with fallback to trg1)?
          if (isCoast)
          {
            if (level <= scales::GetUpperWorldScale())
              ++level;
            if (i == 0)
              ++level;
          }

          // Simplify and serialize geometry.
          // The same line simplification algo is used both for lines
          // and areas. For the latter polygon's outline is simplified and
          // tesselated afterwards. But e.g. simplifying a circle from 50
          // to 10 points will not produce 5 times less triangles.
          // Hence difference between numbers of triangles between trg0 and trg1
          // is much higher than between trg1 and trg2.
          Points points;

          // Do not change linear geometry for the upper scale.
          if (isLine && i == scalesStart && IsCountry() && routing::IsRoad(fb.GetTypes()))
            points = holder.GetSourcePoints();
          else if (isLine || holder.NeedProcessTriangles())
            SimplifyPoints(level, isCoast, rect, holder.GetSourcePoints(), points);

          if (isLine)
            holder.AddPoints(points, i);

          if (isArea && holder.NeedProcessTriangles())
          {
            // Simplify and serialize triangles.
            Polygons simplified;

            bool const isGood = isCoast || scales::IsGoodOutlineForLevel(level, points);
            if (isGood)
            {
              // A polygon's closed outline has 3 points and the 4th should be same as the first.
              CHECK_GREATER_OR_EQUAL(points.size(), 4, ());
              points.pop_back();

              if (polys.size() == 1 && holder.TryToMakeStrip(points))
              {
                // No need to iterate lower zooms if we made a valid strip.
                break;
              }

              simplified.push_back(std::move(points));
            }
            else
            {
              LOG(LDEBUG, ("Area: too small or degenerate 1st (outer) polygon of", polys.size(), ", points count",
                           points.size(), "at scale", i, DebugPrint(fb)));
              continue;
            }

            auto iH = polys.begin();
            for (++iH; iH != polys.end(); ++iH)
            {
              points.clear();
              SimplifyPoints(level, isCoast, rect, *iH, points);

              if (scales::IsGoodOutlineForLevel(level, points))
              {
                // A polygon's closed outline has 3 points and the 4th should be same as the first.
                CHECK_GREATER_OR_EQUAL(points.size(), 4, ());
                points.pop_back();
                simplified.push_back(std::move(points));
              }
              else
              {
                LOG(LDEBUG, ("Area: too small or degenerate 2nd+ (inner) polygon of", polys.size(), ", points count",
                             points.size(), "at scale", i, DebugPrint(fb)));
              }
            }

            if (!simplified.empty())
              holder.AddTriangles(simplified, i);
          }
        }
      }
    }

    // Override "alt_name" with synonym for Country or State for better search matching.
    /// @todo Probably, we should store and index OSM's short_name tag.
    if (indexer::SynonymsHolder::CanApply(fb.GetTypes()))
    {
      int8_t const langs[] = {StringUtf8Multilang::kDefaultCode, StringUtf8Multilang::kEnglishCode,
                              StringUtf8Multilang::kInternationalCode};

      bool added = false;
      for (int8_t lang : langs)
      {
        m_synonyms.ForEach(std::string(fb.GetName(lang)), [&fb, &added](std::string const & synonym)
        {
          // Assign first synonym, skip others.
          if (added)
            return;

          auto oldName = fb.GetName(StringUtf8Multilang::kAltNameCode);
          if (!oldName.empty())
            LOG(LWARNING, ("Replace", oldName, "with", synonym, "for", fb.GetMostGenericOsmId()));

          fb.SetName(StringUtf8Multilang::kAltNameCode, synonym);
          added = true;
        });

        if (added)
          break;
      }
    }

    auto & buffer = holder.GetBuffer();
    if (fb.PreSerializeAndRemoveUselessNamesForMwm(buffer))
    {
      fb.SerializeForMwm(buffer, m_header.GetDefGeometryCodingParams());

      uint32_t const featureId = WriteFeatureBase(buffer.m_buffer, fb);

      // Order is important here:

      // 1. Update postcode info.
      m_boundaryPostcodesEnricher.Enrich(fb);

      // 2. Write address to a file (with possible updated postcode above).
      fb.GetParams().SerializeAddress(*m_addrFile);

      // 3. Save metadata.
      if (!fb.GetMetadata().Empty())
        m_metadataBuilder.Put(featureId, fb.GetMetadata());

      if (fb.HasOsmIds())
        m_osm2ft.AddIds(generator::MakeCompositeId(fb), featureId);
    }
  }

private:
  using Points = std::vector<m2::PointD>;
  using Polygons = std::list<Points>;

  class TmpFile
  {
    std::unique_ptr<FileWriter> m_writer;

  public:
    explicit TmpFile(std::string const & filePath) : m_writer(std::make_unique<FileWriter>(filePath)) {}

    FileWriter & GetWriter() { return *m_writer; }

    ~TmpFile()
    {
      auto const name = m_writer->GetName();
      m_writer.reset();
      FileWriter::DeleteFileX(name);
    }
  };

  using TmpFiles = std::vector<std::unique_ptr<TmpFile>>;

  bool IsCountry() const { return m_header.GetType() == feature::DataHeader::MapType::Country; }

  static void SimplifyPoints(int level, bool isCoast, m2::RectD const & rect, Points const & in, Points & out)
  {
    if (isCoast)
      feature::SimplifyPoints(DistanceToSegmentWithRectBounds(rect), level, in, out);
    else
      feature::SimplifyPoints(m2::SquaredDistanceFromSegmentToPoint(), level, in, out);
  }

  std::string m_filename;

  // File used for postcodes and search sections build.
  std::unique_ptr<FileWriter> m_addrFile;

  // Temporary files for sections.
  TmpFiles m_geoFile, m_trgFile;

  generator::BoundaryPostcodesEnricher m_boundaryPostcodesEnricher;
  indexer::MetadataBuilder m_metadataBuilder;

  DataHeader m_header;
  RegionData m_regionData;
  uint32_t m_versionDate;

  generator::OsmID2FeatureID m_osm2ft;

  indexer::SynonymsHolder m_synonyms;

  DISALLOW_COPY_AND_MOVE(FeaturesCollector2);
};

bool GenerateFinalFeatures(feature::GenerateInfo const & info, std::string const & name,
                           feature::DataHeader::MapType mapType)
{
  std::string const srcFilePath = info.GetTmpFileName(name);
  std::string const dataFilePath = info.GetTargetFileName(name);

  LOG(LINFO, ("Calculating middle points"));
  // Store cellIds for middle points.
  CalculateMidPoints midPoints;
  ForEachFeatureRawFormat(srcFilePath, [&midPoints](FeatureBuilder const & fb, uint64_t pos) { midPoints(fb, pos); });

  // Sort features by their middle point.
  midPoints.Sort();

  // Store sorted features.
  {
    FileReader reader(srcFilePath);
    // Fill mwm header.
    DataHeader header;

    bool const isWorldOrWorldCoasts = (mapType != DataHeader::MapType::Country);
    uint8_t coordBits = kFeatureSorterPointCoordBits;
    if (isWorldOrWorldCoasts)
      coordBits -= ((scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2);

    header.SetType(static_cast<DataHeader::MapType>(mapType));
    header.SetGeometryCodingParams(serial::GeometryCodingParams(coordBits, midPoints.GetCenter()));
    if (isWorldOrWorldCoasts)
      header.SetScales(g_arrWorldScales);
    else
      header.SetScales(g_arrCountryScales);

    RegionData regionData;
    if (!ReadRegionData(name, regionData))
      LOG(LWARNING, ("No extra data for country:", name));

    // Transform features from raw format to optimized format.
    try
    {
      // FeaturesCollector2 will create temporary file `dataFilePath + FEATURES_FILE_TAG`.
      // Can't remove file in ~FeaturesCollector2() because of using it in base class dtor logic.
      SCOPE_GUARD(_, [&]() { Platform::RemoveFileIfExists(info.GetTargetFileName(name, FEATURES_FILE_TAG)); });
      LOG(LINFO, ("Simplifying and filtering geometry for all geom levels"));

      FeaturesCollector2 collector(name, info, header, regionData, info.m_versionDate);
      for (auto const & point : midPoints.GetVector())
      {
        ReaderSource<FileReader> src(reader);
        src.Skip(point.second);

        FeatureBuilder fb;
        ReadFromSourceRawFormat(src, fb);
        collector(fb);
      }

      LOG(LINFO, ("Writing features' data to", dataFilePath));

      // Update bounds with the limit rect corresponding to region borders.
      // Bounds before update can be too big because of big invisible features like a
      // relation that contains an entire country's border.
      // Borders file may be unavailable when building test mwms.
      /// @todo m_targetDir is a final MWM folder like 220702, so there is NO borders folder inside,
      /// and the next function call always returns false.
      m2::RectD bordersRect;
      if (borders::GetBordersRect(info.m_targetDir, name, bordersRect))
        collector.SetBounds(bordersRect);

      collector.Finish();
    }
    catch (RootException const & ex)
    {
      LOG(LCRITICAL, ("MWM writing error:", ex.Msg()));
      return false;
    }
  }

  return true;
}
}  // namespace feature
