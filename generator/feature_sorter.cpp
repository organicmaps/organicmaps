#include "generator/feature_sorter.hpp"
#include "generator/feature_generator.hpp"
#include "generator/feature_builder.hpp"
#include "generator/tesselator.hpp"
#include "generator/gen_mwm_info.hpp"

#include "defines.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/geometry_serialization.hpp"
#include "indexer/scales.hpp"

#include "platform/mwm_version.hpp"

#include "geometry/polygon.hpp"

#include "coding/internal/file_data.hpp"
#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/string_utils.hpp"
#include "base/logging.hpp"

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
      if (minScale != -1)
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
    FilesContainerW m_writer;

    vector<FileWriter*> m_geoFile, m_trgFile;

    unique_ptr<FileWriter> m_MetadataWriter;

    struct MetadataIndexValueT { uint32_t key, value; };
    vector<MetadataIndexValueT> m_MetadataIndex;

    DataHeader m_header;
    uint32_t m_versionDate;

    gen::OsmID2FeatureID m_osm2ft;

  public:
    FeaturesCollector2(string const & fName, DataHeader const & header, uint32_t versionDate)
      : FeaturesCollector(fName + DATA_FILE_TAG), m_writer(fName), m_header(header), m_versionDate(versionDate)
    {
      m_MetadataWriter.reset(new FileWriter(fName + METADATA_FILE_TAG));

      for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
      {
        string const postfix = strings::to_string(i);
        m_geoFile.push_back(new FileWriter(fName + GEOMETRY_FILE_TAG + postfix));
        m_trgFile.push_back(new FileWriter(fName + TRIANGLE_FILE_TAG + postfix));
      }
    }

    ~FeaturesCollector2()
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

      // assume like we close files
      Flush();

      m_writer.Write(m_datFile.GetName(), DATA_FILE_TAG);

      for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
      {
        string const geomFile = m_geoFile[i]->GetName();
        string const trgFile = m_trgFile[i]->GetName();

        delete m_geoFile[i];
        delete m_trgFile[i];

        string const postfix = strings::to_string(i);

        string geoPostfix = GEOMETRY_FILE_TAG;
        geoPostfix += postfix;
        string trgPostfix = TRIANGLE_FILE_TAG;
        trgPostfix += postfix;

        m_writer.Write(geomFile, geoPostfix);
        m_writer.Write(trgFile, trgPostfix);

        FileWriter::DeleteFileX(geomFile);
        FileWriter::DeleteFileX(trgFile);
      }

      {
        FileWriter w = m_writer.GetWriter(METADATA_INDEX_FILE_TAG);
        for (auto const & v : m_MetadataIndex)
        {
          WriteToSink(w, v.key);
          WriteToSink(w, v.value);
        }
      }

      m_MetadataWriter->Flush();
      m_writer.Write(m_MetadataWriter->GetName(), METADATA_FILE_TAG);

      m_writer.Finish();

      FileWriter::DeleteFileX(m_MetadataWriter->GetName());

      if (m_header.GetType() == DataHeader::country)
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
                           bind(D2U, _1, cp.GetCoordBits()), points);

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
          ASSERT ( IsPolygonCCW(points.begin(), points.end()), (points) );
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
    void operator() (FeatureBuilder2 & fb)
    {
      GeometryHolder holder(*this, fb, m_header);

      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();

      int const scalesStart = static_cast<int>(m_header.GetScalesCount()) - 1;
      for (int i = scalesStart; i >= 0; --i)
      {
        int const level = m_header.GetScale(i);
        if (fb.IsDrawableInRange(i > 0 ? m_header.GetScale(i-1) + 1 : 0, level))
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

            polygons_t::const_iterator iH = polys.begin();
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

      if (fb.PreSerialize(holder.m_buffer))
      {
        fb.Serialize(holder.m_buffer, m_header.GetDefCodingParams());

        uint32_t const ftID = WriteFeatureBase(holder.m_buffer.m_buffer, fb);

        if (!fb.GetMetadata().Empty())
        {
          uint64_t offset = m_MetadataWriter->Pos();
          m_MetadataIndex.push_back({ ftID, static_cast<uint32_t>(offset) });
          fb.GetMetadata().SerializeToMWM(*m_MetadataWriter);
        }

        uint64_t const osmID = fb.GetWayIDForRouting();
        if (osmID != 0)
          m_osm2ft.Add(make_pair(osmID, ftID));
      }
    }
  };

  /// Simplify geometry for the upper scale.
  FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
  {
    return static_cast<FeatureBuilder2 &>(fb);
  }

  class DoStoreLanguages
  {
    DataHeader & m_header;
  public:
    DoStoreLanguages(DataHeader & header) : m_header(header) {}
    void operator() (string const & s)
    {
      int8_t const i = StringUtf8Multilang::GetLangIndex(s);
      if (i > 0)
      {
        // 0 index is always 'default'
        m_header.AddLanguage(i);
      }
    }
  };

  bool GenerateFinalFeatures(feature::GenerateInfo const & info, string const & name, int mapType)
  {
    string const srcFilePath = info.GetTmpFileName(name);
    string const datFilePath = info.GetTargetFileName(name);

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

      // languages
      try
      {
        FileReader reader(info.m_targetDir + "metainfo/" + name + ".meta");
        string buffer;
        reader.ReadAsString(buffer);
        strings::Tokenize(buffer, "|", DoStoreLanguages(header));
      }
      catch (Reader::Exception const &)
      {
        LOG(LWARNING, ("No language file for country:", name));
      }

      // Transform features from raw format to optimized format.
      try
      {
        FeaturesCollector2 collector(datFilePath, header, info.m_versionDate);

        for (size_t i = 0; i < midPoints.m_vec.size(); ++i)
        {
          ReaderSource<FileReader> src(reader);
          src.Skip(midPoints.m_vec[i].second);

          FeatureBuilder1 f;
          ReadFromSourceRowFormat(src, f);

          // emit the feature
          collector(GetFeatureBuilder2(f));
        }
      }
      catch (Writer::Exception const & ex)
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
