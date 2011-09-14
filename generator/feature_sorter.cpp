#include "feature_sorter.hpp"
#include "feature_generator.hpp"
#include "feature_builder.hpp"

#include "../defines.hpp"

#include "../indexer/data_header.hpp"
#include "../indexer/feature_processor.hpp"
#include "../indexer/feature_visibility.hpp"
#include "../indexer/feature_impl.hpp"
#include "../indexer/geometry_serialization.hpp"
#include "../indexer/scales.hpp"
#include "../indexer/tesselator.hpp"

#include "../geometry/polygon.hpp"

#include "../platform/platform.hpp"

#include "../version/ver_serialization.hpp"

#include "../coding/file_container.hpp"

#include "../base/string_utils.hpp"
#include "../base/logging.hpp"


namespace
{
  typedef pair<uint64_t, uint64_t> TCellAndOffset;

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

    std::vector<TCellAndOffset> m_vec;

    void operator() (FeatureBuilder1 const & ft, uint64_t pos)
    {
      // reset state
      m_midLoc = m2::PointD(0, 0);
      m_locCount = 0;

      ft.ForEachGeometryPoint(*this);
      m_midLoc = m_midLoc / m_locCount;

      uint64_t const pointAsInt64 = PointToInt64(m_midLoc.x, m_midLoc.y, m_coordBits);
      uint64_t const minScale = feature::MinDrawableScaleForFeature(ft.GetFeatureBase());
      CHECK(minScale <= scales::GetUpperScale(), ("Dat file contain invisible feature"));

      uint64_t const order = (minScale << 59) | (pointAsInt64 >> 5);
      m_vec.push_back(make_pair(order, pos));
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

  bool SortMidPointsFunc(TCellAndOffset const & c1, TCellAndOffset const & c2)
  {
    return c1.first < c2.first;
  }
}

namespace feature
{
  typedef array<uint8_t, 4> scales_t;

  class FeaturesCollector2 : public FeaturesCollector
  {
    FilesContainerW m_writer;

    vector<FileWriter*> m_geoFile, m_trgFile;

    feature::DataHeader m_header;

  public:
    FeaturesCollector2(string const & fName, feature::DataHeader const & header)
      : FeaturesCollector(fName + DATA_FILE_TAG), m_writer(fName), m_header(header)
    {
      for (size_t i = 0; i < m_header.GetScalesCount(); ++i)
      {
        string const postfix = strings::to_string(i);
        m_geoFile.push_back(new FileWriter(fName + GEOMETRY_FILE_TAG + postfix));
        m_trgFile.push_back(new FileWriter(fName + TRIANGLE_FILE_TAG + postfix));
      }
    }

    ~FeaturesCollector2()
    {
      // write own mwm header (now it's a base point only)
      m_header.SetBounds(m_bounds);
      {
        FileWriter w = m_writer.GetWriter(HEADER_FILE_TAG);
        m_header.Save(w);
      }

      // write version information
      {
        FileWriter w = m_writer.GetWriter(VERSION_FILE_TAG);
        ver::WriteVersion(w);
      }

      // assume like we close files
      m_datFile.Flush();

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

      m_writer.Finish();
    }

  private:
    typedef vector<m2::PointD> points_t;
    typedef list<points_t> polygons_t;

    class GeometryHolder
    {
    public:
      FeatureBuilder2::buffers_holder_t m_buffer;

    private:
      FeaturesCollector2 & m_rMain;
      FeatureBuilder2 & m_rFB;

      points_t m_current;

      serial::CodingParams m_codingParams;

      void WriteOuterPoints(points_t const & points, int i)
      {
        m_buffer.m_ptsMask |= (1 << i);
        m_buffer.m_ptsOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_geoFile[i]));
        serial::SaveOuterPath(points, m_codingParams, *m_rMain.m_geoFile[i]);
      }

      void WriteOuterTriangles(polygons_t const & polys, int i)
      {
        // tesselation
        tesselator::TrianglesInfo info;
        tesselator::TesselateInterior(polys, info);

        if (info.IsEmpty()) return;

        serial::TrianglesChainSaver saver(m_codingParams);

        // points conversion
        tesselator::PointsInfo points;
        m2::PointU (* D2U)(m2::PointD const &, uint32_t) = &PointD2PointU;
        info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(),
                           bind(D2U, _1, m_codingParams.GetCoordBits()), points);

        // triangles processing (should be optimal)
        info.ProcessPortions(points, saver, true);

        // check triangles processing (to compare with optimal)
        //serial::TrianglesChainSaver checkSaver(m_codingParams);
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
                     serial::CodingParams const & codingParams)
        : m_rMain(rMain), m_rFB(fb), m_codingParams(codingParams),
          m_ptsInner(true), m_trgInner(true)
      {
      }

      points_t const & GetSourcePoints()
      {
        return (!m_current.empty() ? m_current : m_rFB.GetOuterPoly());
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
        CHECK ( are_points_equal(points.front(), points.back()), () );
        // At this point we don't need last point equal to first.
        // If you try to remove it in first step, 'simplify' will work bad for such polygons.
        points.pop_back();

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

    class less_points
    {
      double const m_eps;
    public:
      less_points() : m_eps(1.0E-6) {}

      bool operator() (m2::PointD const & p1, m2::PointD const & p2) const
      {
        if (p1.x + m_eps < p2.x) return true;
        if (fabs(p1.x - p2.x) <= m_eps)
          return (p1.y + m_eps < p2.y);
        else return false;
      }
    };

    class equal_points
    {
      double const m_eps;
    public:
      equal_points() : m_eps(1.0E-6) {}

      bool operator() (m2::PointD const & p1, m2::PointD const & p2) const
      {
        return p1.EqualDxDy(p2, m_eps);
      }
    };

    typedef set<m2::PointD, less_points> points_set_t;

    class BoundsDistance : public mn::DistanceToLineSquare<m2::PointD>
    {
      points_set_t const & m_skip;

      double m_eps;
      double m_minX, m_minY, m_maxX, m_maxY;

    public:
      BoundsDistance(uint32_t cellID, int level, points_set_t const & skip)
        : m_skip(skip)
      {
        RectId const cell = RectId::FromBitsAndLevel(cellID, level);
        CellIdConverter<MercatorBounds, RectId>::GetCellBounds(cell, m_minX, m_minY, m_maxX, m_maxY);
      }

      void SetEpsilon(double eps) { m_eps = eps; }

      double operator() (m2::PointD const & p) const
      {
        if (fabs(p.x - m_minX) <= m_eps || fabs(p.x - m_maxX) <= m_eps ||
            fabs(p.y - m_minY) <= m_eps || fabs(p.y - m_maxY) <= m_eps)
        {
          // points near rect should be in a result simplified vector
          return std::numeric_limits<double>::max();
        }

        if (m_skip.count(p) > 0)
        {
          // if we have more than one point with equal coordinates - do not simplify them
          return std::numeric_limits<double>::max();
        }

        return mn::DistanceToLineSquare<m2::PointD>::operator()(p);
      }
    };

    void SimplifyPoints(points_t const & in, points_t & out, int level,
                        FeatureBuilder2 const & fb)
    {
      uint32_t cellID;
      if (fb.GetCoastCell(cellID))
      {
        points_set_t toSkip;
        {
          points_t v(in);
          sort(v.begin(), v.end(), less_points());
          toSkip.insert(unique(v.begin(), v.end(), equal_points()), v.end());
        }

        BoundsDistance dist(cellID, g_coastsCellLevel, toSkip);
        feature::SimplifyPoints(dist, in, out, level);

        //LOG(LINFO, ("Special simplification", in.size(), out.size()));
      }
      else
      {
        mn::DistanceToLineSquare<m2::PointD> dist;
        feature::SimplifyPoints(dist, in, out, level);
      }
    }

  public:
    void operator() (FeatureBuilder2 & fb)
    {
      GeometryHolder holder(*this, fb, m_header.GetCodingParams());

      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();

      for (int i = m_header.GetScalesCount()-1; i >= 0; --i)
      {
        int const level = m_header.GetScale(i);
        if (fb.IsDrawableInRange(i > 0 ? m_header.GetScale(i-1) + 1 : 0, level))
        {
          // simplify and serialize geometry
          points_t points;
          SimplifyPoints(holder.GetSourcePoints(), points, level, fb);

          if (isLine)
            holder.AddPoints(points, i);

          if (isArea && points.size() > 3 && holder.NeedProcessTriangles())
          {
            // simplify and serialize triangles

            polygons_t const & polys = fb.GetPolygons();
            if (polys.size() == 1 && holder.TryToMakeStrip(points))
              continue;

            polygons_t simplified;
            simplified.push_back(points);

            polygons_t::const_iterator iH = polys.begin();
            for (++iH; iH != polys.end(); ++iH)
            {
              simplified.push_back(points_t());

              SimplifyPoints(*iH, simplified.back(), level, fb);

              if (simplified.back().size() < 3)
                simplified.pop_back();
            }

            holder.AddTriangles(simplified, i);
          }
        }
      }

      if (fb.PreSerialize(holder.m_buffer))
      {
        fb.Serialize(holder.m_buffer, m_header.GetCodingParams());

        WriteFeatureBase(holder.m_buffer.m_buffer, fb);
      }
    }
  };

  /// Simplify geometry for the upper scale.
  FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
  {
    return static_cast<FeatureBuilder2 &>(fb);
  }

  bool GenerateFinalFeatures(string const & datFilePath, int mapType)
  {
    // rename input file
    Platform & platform = GetPlatform();
    string tempDatFilePath(datFilePath);
    tempDatFilePath += ".notsorted";

    FileWriter::DeleteFileX(tempDatFilePath);
    if (!platform.RenameFileX(datFilePath, tempDatFilePath))
    {
      LOG(LWARNING, ("File ", datFilePath, " doesn't exist or sharing violation!"));
      return false;
    }

    // stores cellIds for middle points
    CalculateMidPoints midPoints;
    ForEachFromDatRawFormat(tempDatFilePath, midPoints);

    // sort features by their middle point
    LOG(LINFO, ("Sorting features..."));
    std::sort(midPoints.m_vec.begin(), midPoints.m_vec.end(), &SortMidPointsFunc);
    LOG(LINFO, ("Done sorting features"));

    // store sorted features
    {
      FileReader reader(tempDatFilePath);

      bool const isWorld = (mapType != feature::DataHeader::country);

      feature::DataHeader header;
      uint32_t coordBits = 27;
      if (isWorld)
        coordBits -= (scales::GetUpperScale() - scales::GetUpperWorldScale());

      header.SetCodingParams(serial::CodingParams(coordBits, midPoints.GetCenter()));
      header.SetScales(isWorld ? g_arrWorldScales : g_arrCountryScales);
      header.SetType(static_cast<feature::DataHeader::MapType>(mapType));

      FeaturesCollector2 collector(datFilePath, header);

      for (size_t i = 0; i < midPoints.m_vec.size(); ++i)
      {
        ReaderSource<FileReader> src(reader);
        src.Skip(midPoints.m_vec[i].second);

        FeatureBuilder1 f;
        feature::ReadFromSourceRowFormat(src, f);

        // emit the feature
        collector(GetFeatureBuilder2(f));
      }

      // at this point files should be closed
    }

    // remove old not-sorted dat file
    FileWriter::DeleteFileX(tempDatFilePath);
    FileWriter::DeleteFileX(datFilePath + DATA_FILE_TAG);

    return true;
  }
} // namespace feature
