#include "generator/feature_sorter.hpp"
#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/gen_mwm_info.hpp"
#include "generator/region_meta.hpp"
#include "generator/tesselator.hpp"

#include "defines.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/geometry_serialization.hpp"
#include "indexer/scales.hpp"
#include "indexer/scales_patch.hpp"

#include "platform/mwm_version.hpp"

#include "geometry/polygon.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

namespace
{
  typedef pair<uint64_t, uint64_t> CellAndOffsetT;

  class CalculateMidPoints
  {
    m2::PointD m_midLoc, m_midAll;
    size_t m_locCount, m_allCount;
    uint32_t m_coordBits;

  public:
    CalculateMidPoints() :
      m_midAll(0, 0), m_allCount(0), m_coordBits(serial::CodingParams().GetCoordBits())
    {
    }

    vector<CellAndOffsetT> m_vec;

    void operator() (FeatureBuilder1 const & ft, uint64_t pos)
    {
      // reset state
      m_midLoc = m2::PointD(0, 0);
      m_locCount = 0;

      ft.ForEachGeometryPoint(*this);
      m_midLoc = m_midLoc / m_locCount;

      uint64_t const pointAsInt64 = PointToInt64(m_midLoc, m_coordBits);
      int const minScale = feature::GetMinDrawableScale(ft.GetFeatureBase());

      /// May be invisible if it's small area object with [0-9] scales.
      /// @todo Probably, we need to keep that objects if 9 scale (as we do in 17 scale).
      if (minScale != -1 || feature::RequireGeometryInIndex(ft.GetFeatureBase()))
      {
        uint64_t const order = (static_cast<uint64_t>(minScale) << 59) | (pointAsInt64 >> 5);
        m_vec.push_back(make_pair(order, pos));
      }
    }

    bool operator() (m2::PointD const & p)
    {
      m_midLoc += p;
      m_midAll += p;
      ++m_locCount;
      ++m_allCount;
      return true;
    }

    m2::PointD GetCenter() const { return m_midAll / m_allCount; }
  };

  bool SortMidPointsFunc(CellAndOffsetT const & c1, CellAndOffsetT const & c2)
  {
    return c1.first < c2.first;
  }
}

namespace feature
{
  class FeaturesCollector2 : public FeaturesCollector
  {
    DISALLOW_COPY_AND_MOVE(FeaturesCollector2);

    FilesContainerW m_writer;

    class TmpFile : public FileWriter
    {
    public:
      explicit TmpFile(std::string const & filePath) : FileWriter(filePath) {}
      ~TmpFile()
      {
        DeleteFileX(GetName());
      }
    };

    using TmpFiles = vector<unique_ptr<TmpFile>>;
    TmpFiles m_geoFile, m_trgFile;

    enum { METADATA = 0, SEARCH_TOKENS = 1, FILES_COUNT = 2 };
    TmpFiles m_helperFile;

    // Mapping from feature id to offset in file section with the correspondent metadata.
    vector<pair<uint32_t, uint32_t>> m_metadataIndex;

    DataHeader m_header;
    RegionData m_regionData;
    uint32_t m_versionDate;

    gen::OsmID2FeatureID m_osm2ft;

  public:
    FeaturesCollector2(std::string const & fName, DataHeader const & header,
                       RegionData const & regionData, uint32_t versionDate)
      : FeaturesCollector(fName + DATA_FILE_TAG), m_writer(fName),
        m_header(header), m_regionData(regionData), m_versionDate(versionDate)
    {
      for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
      {
        std::string const postfix = strings::to_string(i);
        m_geoFile.push_back(make_unique<TmpFile>(fName + GEOMETRY_FILE_TAG + postfix));
        m_trgFile.push_back(make_unique<TmpFile>(fName + TRIANGLE_FILE_TAG + postfix));
      }

      m_helperFile.resize(FILES_COUNT);
      m_helperFile[METADATA] = make_unique<TmpFile>(fName + METADATA_FILE_TAG);
      m_helperFile[SEARCH_TOKENS] = make_unique<TmpFile>(fName + SEARCH_TOKENS_FILE_TAG);
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
      auto const finalizeFn = [this](unique_ptr<TmpFile> w, std::string const & tag,
                                     std::string const & postfix = std::string())
      {
        w->Flush();
        m_writer.Write(w->GetName(), tag + postfix);
      };

      for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
      {
        std::string const postfix = strings::to_string(i);
        finalizeFn(move(m_geoFile[i]), GEOMETRY_FILE_TAG, postfix);
        finalizeFn(move(m_trgFile[i]), TRIANGLE_FILE_TAG, postfix);
      }

      {
        /// @todo Replace this mapping vector with succint structure.
        FileWriter w = m_writer.GetWriter(METADATA_INDEX_FILE_TAG);
        for (auto const & v : m_metadataIndex)
        {
          WriteToSink(w, v.first);
          WriteToSink(w, v.second);
        }
      }

      finalizeFn(move(m_helperFile[METADATA]), METADATA_FILE_TAG);
      finalizeFn(move(m_helperFile[SEARCH_TOKENS]), SEARCH_TOKENS_FILE_TAG);

      m_writer.Finish();

      if (m_header.GetType() == DataHeader::country ||
          m_header.GetType() == DataHeader::world)
      {
        FileWriter osm2ftWriter(m_writer.GetFileName() + OSM2FEATURE_FILE_EXTENSION);
        m_osm2ft.Flush(osm2ftWriter);
      }
    }

  private:
    typedef vector<m2::PointD> points_t;
    typedef list<points_t> polygons_t;

    class GeometryHolder
    {
    public:
      FeatureBuilder2::SupportingData m_buffer;

    private:
      FeaturesCollector2 & m_rMain;
      FeatureBuilder2 & m_rFB;

      points_t m_current;

      DataHeader const & m_header;

      void WriteOuterPoints(points_t const & points, int i)
      {
        // outer path can have 2 points in small scale levels
        ASSERT_GREATER ( points.size(), 1, () );

        serial::CodingParams cp = m_header.GetCodingParams(i);

        // Optimization: Store first point once in header for outer linear features.
        cp.SetBasePoint(points[0]);
        // Can optimize here, but ... Make copy of vector.
        points_t toSave(points.begin() + 1, points.end());

        m_buffer.m_ptsMask |= (1 << i);
        m_buffer.m_ptsOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_geoFile[i]));
        serial::SaveOuterPath(toSave, cp, *m_rMain.m_geoFile[i]);
      }

      void WriteOuterTriangles(polygons_t const & polys, int i)
      {
        // tesselation
        tesselator::TrianglesInfo info;
        if (0 == tesselator::TesselateInterior(polys, info))
        {
          LOG(LINFO, ("NO TRIANGLES in", polys));
          return;
        }

        serial::CodingParams const cp = m_header.GetCodingParams(i);

        serial::TrianglesChainSaver saver(cp);

        // points conversion
        tesselator::PointsInfo points;
        m2::PointU (* D2U)(m2::PointD const &, uint32_t) = &PointD2PointU;
        info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(),
                           std::bind(D2U, std::placeholders::_1, cp.GetCoordBits()), points);

        // triangles processing (should be optimal)
        info.ProcessPortions(points, saver, true);

        // check triangles processing (to compare with optimal)
        //serial::TrianglesChainSaver checkSaver(cp);
        //info.ProcessPortions(points, checkSaver, false);

        //CHECK_LESS_OR_EQUAL(saver.GetBufferSize(), checkSaver.GetBufferSize(), ());

        // saving to file
        m_buffer.m_trgMask |= (1 << i);
        m_buffer.m_trgOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_trgFile[i]));
        saver.Save(*m_rMain.m_trgFile[i]);
      }

      void FillInnerPointsMask(points_t const & points, uint32_t scaleIndex)
      {
        points_t const & src = m_buffer.m_innerPts;
        CHECK ( !src.empty(), () );

        CHECK ( are_points_equal(src.front(), points.front()), () );
        CHECK ( are_points_equal(src.back(), points.back()), () );

        size_t j = 1;
        for (size_t i = 1; i < points.size()-1; ++i)
        {
          for (; j < src.size()-1; ++j)
          {
            if (are_points_equal(src[j], points[i]))
            {
              // set corresponding 2 bits for source point [j] to scaleIndex
              uint32_t mask = 0x3;
              m_buffer.m_ptsSimpMask &= ~(mask << (2*(j-1)));
              m_buffer.m_ptsSimpMask |= (scaleIndex << (2*(j-1)));
              break;
            }
          }

          CHECK_LESS ( j, src.size()-1, ("Simplified point not found in source point array") );
        }
      }

      bool m_ptsInner, m_trgInner;

      class strip_emitter
      {
        points_t const & m_src;
        points_t & m_dest;
      public:
        strip_emitter(points_t const & src, points_t & dest)
          : m_src(src), m_dest(dest)
        {
          m_dest.reserve(m_src.size());
        }
        void operator() (size_t i)
        {
          m_dest.push_back(m_src[i]);
        }
      };

    public:
      GeometryHolder(FeaturesCollector2 & rMain,
                     FeatureBuilder2 & fb,
                     DataHeader const & header)
        : m_rMain(rMain), m_rFB(fb), m_header(header),
          m_ptsInner(true), m_trgInner(true)
      {
      }

      points_t const & GetSourcePoints()
      {
        return (!m_current.empty() ? m_current : m_rFB.GetOuterGeometry());
      }

      void AddPoints(points_t const & points, int scaleIndex)
      {
        if (m_ptsInner && points.size() < 15)
        {
          if (m_buffer.m_innerPts.empty())
            m_buffer.m_innerPts = points;
          else
            FillInnerPointsMask(points, scaleIndex);
          m_current = points;
        }
        else
        {
          m_ptsInner = false;
          WriteOuterPoints(points, scaleIndex);
        }
      }

      bool NeedProcessTriangles() const
      {
        return (!m_trgInner || m_buffer.m_innerTrg.empty());
      }

      bool TryToMakeStrip(points_t & points)
      {
        size_t const count = points.size();
        if (!m_trgInner || count > 15 + 2)
        {
          // too many points for strip
          m_trgInner = false;
          return false;
        }

        if (m2::robust::CheckPolygonSelfIntersections(points.begin(), points.end()))
        {
          // polygon has self-intersectios
          m_trgInner = false;
          return false;
        }

        CHECK ( m_buffer.m_innerTrg.empty(), () );

        // make CCW orientation for polygon
        if (!IsPolygonCCW(points.begin(), points.end()))
        {
          reverse(points.begin(), points.end());

          // Actually this check doesn't work for some degenerate polygons.
          // See IsPolygonCCW_DataSet tests for more details.
          //ASSERT(IsPolygonCCW(points.begin(), points.end()), (points));
          if (!IsPolygonCCW(points.begin(), points.end()))
            return false;
        }

        size_t const index = FindSingleStrip(count,
          IsDiagonalVisibleFunctor<points_t::const_iterator>(points.begin(), points.end()));

        if (index == count)
        {
          // can't find strip
          m_trgInner = false;
          return false;
        }

        MakeSingleStripFromIndex(index, count, strip_emitter(points, m_buffer.m_innerTrg));

        CHECK_EQUAL ( count, m_buffer.m_innerTrg.size(), () );
        return true;
      }

      void AddTriangles(polygons_t const & polys, int scaleIndex)
      {
        CHECK ( m_buffer.m_innerTrg.empty(), () );
        m_trgInner = false;

        WriteOuterTriangles(polys, scaleIndex);
      }
    };

    void SimplifyPoints(points_t const & in, points_t & out, int level,
                        bool isCoast, m2::RectD const & rect)
    {
      if (isCoast)
      {
        BoundsDistance dist(rect);
        feature::SimplifyPoints(dist, in, out, level);
      }
      else
      {
        m2::DistanceToLineSquare<m2::PointD> dist;
        feature::SimplifyPoints(dist, in, out, level);
      }
    }

    static double CalcSquare(points_t const & poly)
    {
      ASSERT ( poly.front() == poly.back(), () );

      double res = 0;
      for (size_t i = 0; i < poly.size()-1; ++i)
      {
        res += (poly[i+1].x - poly[i].x) *
               (poly[i+1].y + poly[i].y) / 2.0;
      }
      return fabs(res);
    }

    static bool IsGoodArea(points_t const & poly, int level)
    {
      if (poly.size() < 4)
        return false;

      m2::RectD r;
      CalcRect(poly, r);

      return scales::IsGoodForLevel(level, r);
    }

    bool IsCountry() const { return m_header.GetType() == feature::DataHeader::country; }

  public:
    uint32_t operator()(FeatureBuilder2 & fb)
    {
      GeometryHolder holder(*this, fb, m_header);

      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();

      int const scalesStart = static_cast<int>(m_header.GetScalesCount()) - 1;
      for (int i = scalesStart; i >= 0; --i)
      {
        int const level = m_header.GetScale(i);
        if (fb.IsDrawableInRange(i > 0 ? m_header.GetScale(i-1) + 1 : 0, PatchScaleBound(level)))
        {
          bool const isCoast = fb.IsCoastCell();
          m2::RectD const rect = fb.GetLimitRect();

          // Simplify and serialize geometry.
          points_t points;

          // Do not change linear geometry for the upper scale.
          if (isLine && i == scalesStart && IsCountry() && fb.IsRoad())
            points = holder.GetSourcePoints();
          else
            SimplifyPoints(holder.GetSourcePoints(), points, level, isCoast, rect);

          if (isLine)
            holder.AddPoints(points, i);

          if (isArea && holder.NeedProcessTriangles())
          {
            // simplify and serialize triangles
            bool const good = isCoast || IsGoodArea(points, level);

            // At this point we don't need last point equal to first.
            points.pop_back();

            polygons_t const & polys = fb.GetGeometry();
            if (polys.size() == 1 && good && holder.TryToMakeStrip(points))
              continue;

            polygons_t simplified;
            if (good)
            {
              simplified.push_back(points_t());
              simplified.back().swap(points);
            }

            auto iH = polys.begin();
            for (++iH; iH != polys.end(); ++iH)
            {
              simplified.push_back(points_t());

              SimplifyPoints(*iH, simplified.back(), level, isCoast, rect);

              // Increment level check for coastline polygons for the first scale level.
              // This is used for better coastlines quality.
              if (IsGoodArea(simplified.back(), (isCoast && i == 0) ? level + 1 : level))
              {
                // At this point we don't need last point equal to first.
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
      if (fb.PreSerialize(holder.m_buffer))
      {
        fb.Serialize(holder.m_buffer, m_header.GetDefCodingParams());

        featureId = WriteFeatureBase(holder.m_buffer.m_buffer, fb);

        fb.GetAddressData().Serialize(*(m_helperFile[SEARCH_TOKENS]));

        if (!fb.GetMetadata().Empty())
        {
          auto const & w = m_helperFile[METADATA];

          uint64_t const offset = w->Pos();
          ASSERT_LESS_OR_EQUAL(offset, numeric_limits<uint32_t>::max(), ());

          m_metadataIndex.emplace_back(featureId, static_cast<uint32_t>(offset));
          fb.GetMetadata().Serialize(*w);
        }

        if (!fb.GetOsmIds().empty())
        {
          osm::Id const osmId = fb.GetMostGenericOsmId();
          m_osm2ft.Add(make_pair(osmId, featureId));
        }
      };
      return featureId;
    }
  };

  /// Simplify geometry for the upper scale.
  FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
  {
    return static_cast<FeatureBuilder2 &>(fb);
  }

  bool GenerateFinalFeatures(feature::GenerateInfo const & info, std::string const & name, int mapType)
  {
    std::string const srcFilePath = info.GetTmpFileName(name);
    std::string const datFilePath = info.GetTargetFileName(name);

    // stores cellIds for middle points
    CalculateMidPoints midPoints;
    ForEachFromDatRawFormat(srcFilePath, midPoints);

    // sort features by their middle point
    sort(midPoints.m_vec.begin(), midPoints.m_vec.end(), &SortMidPointsFunc);

    // store sorted features
    {
      FileReader reader(srcFilePath);

      bool const isWorld = (mapType != DataHeader::country);

      // Fill mwm header.
      DataHeader header;

      uint32_t coordBits = 27;
      if (isWorld)
        coordBits -= ((scales::GetUpperScale() - scales::GetUpperWorldScale()) / 2);

      // coding params
      header.SetCodingParams(serial::CodingParams(coordBits, midPoints.GetCenter()));

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

        for (size_t i = 0; i < midPoints.m_vec.size(); ++i)
        {
          ReaderSource<FileReader> src(reader);
          src.Skip(midPoints.m_vec[i].second);

          FeatureBuilder1 f;
          ReadFromSourceRowFormat(src, f);

          // emit the feature
          collector(GetFeatureBuilder2(f));
        }

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
} // namespace feature
