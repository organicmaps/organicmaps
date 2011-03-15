#include "feature_sorter.hpp"
#include "feature_generator.hpp"

#include "../../defines.hpp"

#include "../../indexer/data_header.hpp"
#include "../../indexer/feature_processor.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../indexer/feature_impl.hpp"
#include "../../indexer/geometry_serialization.hpp"
#include "../../indexer/tesselator.hpp"

#include "../../geometry/polygon.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/file_container.hpp"

#include "../../base/string_utils.hpp"
#include "../../base/logging.hpp"

#include "../../base/start_mem_debug.hpp"


namespace
{
  typedef pair<uint64_t, uint64_t> TCellAndOffset;

  class CalculateMidPoints
  {
    m2::PointD m_midLoc, m_midAll;
    size_t m_locCount, m_allCount;

  public:
    CalculateMidPoints() : m_midAll(0, 0), m_allCount(0) {}

    std::vector<TCellAndOffset> m_vec;

    void operator() (FeatureBuilder1 const & ft, uint64_t pos)
    {
      // reset state
      m_midLoc = m2::PointD(0, 0);
      m_locCount = 0;

      ft.ForEachPointRef(*this);
      m_midLoc = m_midLoc / m_locCount;

      uint64_t const pointAsInt64 = PointToInt64(m_midLoc.x, m_midLoc.y);
      uint64_t const minScale = feature::MinDrawableScaleForFeature(ft.GetFeatureBase());
      CHECK(minScale <= scales::GetUpperScale(), ("Dat file contain invisible feature"));

      uint64_t const order = (minScale << 59) | (pointAsInt64 >> 5);
      m_vec.push_back(make_pair(order, pos));
    }

    void operator() (m2::PointD const & p)
    {
      m_midLoc += p;
      m_midAll += p;
      ++m_locCount;
      ++m_allCount;
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
      for (int i = 0; i < m_header.GetScalesCount(); ++i)
      {
        string const postfix = utils::to_string(i);
        m_geoFile.push_back(new FileWriter(fName + GEOMETRY_FILE_TAG + postfix));
        m_trgFile.push_back(new FileWriter(fName + TRIANGLE_FILE_TAG + postfix));
      }
    }

    ~FeaturesCollector2()
    {
      // write own mwm header (now it's a base point only)
      m_header.SetBounds(m_bounds);
      FileWriter w = m_writer.GetWriter(HEADER_FILE_TAG);
      m_header.Save(w);
      w.Flush();

      // assume like we close files
      m_datFile.Flush();

      m_writer.Append(m_datFile.GetName(), DATA_FILE_TAG);

      for (int i = 0; i < m_header.GetScalesCount(); ++i)
      {
        string const geomFile = m_geoFile[i]->GetName();
        string const trgFile = m_trgFile[i]->GetName();

        delete m_geoFile[i];
        delete m_trgFile[i];

        string const postfix = utils::to_string(i);

        string geoPostfix = GEOMETRY_FILE_TAG;
        geoPostfix += postfix;
        string trgPostfix = TRIANGLE_FILE_TAG;
        trgPostfix += postfix;

        m_writer.Append(geomFile, geoPostfix);
        m_writer.Append(trgFile, trgPostfix);

        FileWriter::DeleteFileX(geomFile);
        FileWriter::DeleteFileX(trgFile);
      }

      m_writer.Finish();
    }

  private:
    typedef vector<m2::PointD> points_t;
    typedef list<points_t> holes_t;

    class GeometryHolder
    {
    public:
      FeatureBuilder2::buffers_holder_t m_buffer;

    private:
      FeaturesCollector2 & m_rMain;
      FeatureBuilder2 & m_rFB;

      points_t m_current;

      int64_t m_base;

      void WriteOuterPoints(points_t const & points, int i)
      {
        m_buffer.m_ptsMask |= (1 << i);
        m_buffer.m_ptsOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_geoFile[i]));
        serial::SaveOuterPath(points, m_base, *m_rMain.m_geoFile[i]);
      }

      void WriteOuterTriangles(points_t const & bound, holes_t const & holes, int i)
      {
        m_buffer.m_trgMask |= (1 << i);
        m_buffer.m_trgOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_trgFile[i]));

        // tesselation
        tesselator::TrianglesInfo info;
        tesselator::TesselateInterior(bound, holes, info);

        serial::TrianglesChainSaver saver(m_base);

        // points conversion
        tesselator::PointsInfo points;
        info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(), &serial::pts::D2U, points);

        // triangles processing (should be optimal)
        info.ProcessPortions(points, saver, true);

        // check triangles processing (to compare with optimal)
        //serial::TrianglesChainSaver checkSaver(m_base);
        //info.ProcessPortions(points, checkSaver, false);

        //CHECK_LESS_OR_EQUAL(saver.GetBufferSize(), checkSaver.GetBufferSize(), ());

        // saving to file
        saver.Save(*m_rMain.m_trgFile[i]);
      }

      void FillInnerPointsMask(points_t const & points, uint32_t scaleIndex)
      {
        points_t const & src = m_buffer.m_innerPts;
        ASSERT ( !src.empty(), () );

        ASSERT ( are_points_equal(src.front(), points.front()), () );
        ASSERT ( are_points_equal(src.back(), points.back()), () );

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

          ASSERT_LESS ( j, src.size()-1, ("Simplified point not found in source point array") );
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
      GeometryHolder(FeaturesCollector2 & rMain, FeatureBuilder2 & fb, int64_t base)
        : m_rMain(rMain), m_rFB(fb), m_base(base), m_ptsInner(true), m_trgInner(true)
      {
      }

      points_t const & GetSourcePoints()
      {
        return (!m_current.empty() ? m_current : m_rFB.GetGeometry());
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
        ASSERT ( are_points_equal(points.front(), points.back()), () );
        // At this point we don't need last point equal to first.
        // If you try to remove it in first step, 'simplify' will work bad for such polygons.
        points.pop_back();

        size_t const count = points.size();
        if (!m_trgInner || count > 15 + 2)
        {
          m_trgInner = false;
          return false;
        }

        ASSERT ( m_buffer.m_innerTrg.empty(), () );

        if (!IsPolygonCCW(points.begin(), points.end()))
        {
          reverse(points.begin(), points.end());
          ASSERT ( IsPolygonCCW(points.begin(), points.end()), (points) );
        }

        size_t const index = FindSingleStrip(count,
          IsDiagonalVisibleFunctor<points_t::const_iterator>(points.begin(), points.end()));

        if (index == count)
        {
          m_trgInner = false;
          return false;
        }

        MakeSingleStripFromIndex(index, count, strip_emitter(points, m_buffer.m_innerTrg));

        ASSERT_EQUAL ( count, m_buffer.m_innerTrg.size(), () );
        return true;
      }

      void AddTriangles(points_t const & bound, holes_t const & holes, int scaleIndex)
      {
        ASSERT ( m_buffer.m_innerTrg.empty(), () );
        m_trgInner = false;

        WriteOuterTriangles(bound, holes, scaleIndex);
      }
    };

  public:
    void operator() (FeatureBuilder2 & fb)
    {
      (void)GetFileSize(m_datFile);

      GeometryHolder holder(*this, fb, m_header.GetBase());

      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();

      for (int i = m_header.GetScalesCount()-1; i >= 0; --i)
      {
        if (fb.IsDrawableInRange(i > 0 ? m_header.GetScale(i-1) + 1 : 0, m_header.GetScale(i)))
        {
          // simplify and serialize geometry
          points_t points;
          SimplifyPoints(holder.GetSourcePoints(), points, m_header.GetScale(i));

          if (isLine)
            holder.AddPoints(points, i);

          if (isArea && points.size() > 3 && holder.NeedProcessTriangles())
          {
            // simplify and serialize triangles

            holes_t const & holes = fb.GetHoles();

            if (holes.empty() && holder.TryToMakeStrip(points))
              continue;

            holes_t simpleHoles;
            for (holes_t::const_iterator iH = holes.begin(); iH != holes.end(); ++iH)
            {
              simpleHoles.push_back(points_t());

              SimplifyPoints(*iH, simpleHoles.back(), m_header.GetScale(i));

              if (simpleHoles.back().size() < 3)
                simpleHoles.pop_back();
            }

            holder.AddTriangles(points, simpleHoles, i);
          }
        }
      }

      if (fb.PreSerialize(holder.m_buffer))
      {
        fb.Serialize(holder.m_buffer, m_header.GetBase());

        WriteFeatureBase(holder.m_buffer.m_buffer, fb);
      }
    }
  };

  /// Simplify geometry for the upper scale.
  FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
  {
    return static_cast<FeatureBuilder2 &>(fb);
  }


  bool GenerateFinalFeatures(string const & datFilePath, bool bSort, bool bWorld)
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
    if (bSort)
      std::sort(midPoints.m_vec.begin(), midPoints.m_vec.end(), &SortMidPointsFunc);

    // store sorted features
    {
      FileReader reader(tempDatFilePath);

      feature::DataHeader header;
      header.SetBase(midPoints.GetCenter());
      header.SetScales(bWorld ? g_arrWorldScales : g_arrCountryScales);

      FeaturesCollector2 collector(datFilePath, header);

      FeatureBuilder1::buffer_t buffer;
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
