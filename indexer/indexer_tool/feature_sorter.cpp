#include "feature_sorter.hpp"
#include "feature_generator.hpp"

#include "../../indexer/data_header.hpp"
#include "../../indexer/feature_processor.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../indexer/feature_impl.hpp"
#include "../../indexer/scales.hpp"
#include "../../indexer/cell_id.hpp"

#include "../../geometry/distance.hpp"
#include "../../geometry/simplification.hpp"

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
    double m_midX;
    double m_midY;
    size_t m_counter;

  public:
    std::vector<TCellAndOffset> m_vec;

    void operator() (FeatureBuilder1 const & ft, uint64_t pos)
    {
      // reset state
      m_midX = 0.0;
      m_midY = 0.0;
      m_counter = 0;
      ft.ForEachPointRef(*this);
      m_midX /= m_counter;
      m_midY /= m_counter;

      uint64_t const pointAsInt64 = PointToInt64(m_midX, m_midY);
      uint64_t const minScale = feature::MinDrawableScaleForFeature(ft.GetFeatureBase());
      CHECK(minScale <= scales::GetUpperScale(), ("Dat file contain invisible feature"));

      uint64_t const order = (minScale << 59) | (pointAsInt64 >> 5);
      m_vec.push_back(make_pair(order, pos));
    }

    void operator() (m2::PointD const & p)
    {
      m_midX += p.x;
      m_midY += p.y;
      ++m_counter;
    }
  };

  bool SortMidPointsFunc(TCellAndOffset const & c1, TCellAndOffset const & c2)
  {
    return c1.first < c2.first;
  }
}

namespace feature
{
  typedef vector<m2::PointD> points_t;

  namespace
  {
    bool is_equal(m2::PointD const & p1, m2::PointD const & p2)
    {
      return p1.EqualDxDy(p2, MercatorBounds::GetCellID2PointAbsEpsilon());
    }
  }

  void SimplifyPoints(points_t const & in, points_t & out, int level)
  {
    if (in.size() >= 2)
    {
      SimplifyNearOptimal<mn::DistanceToLineSquare<m2::PointD> >(20, in.begin(), in.end()-1,
        my::sq(scales::GetEpsilonForSimplify(level)), MakeBackInsertFunctor(out));

      switch (out.size())
      {
      case 0:
        out.push_back(in.front());
        // no break
      case 1:
        out.push_back(in.back());
        break;
      default:
        if (!is_equal(out.back(), in.back()))
          out.push_back(in.back());
      }
    }
  }

  void TesselateInterior(points_t const & bound, list<points_t> const & holes, points_t & triangles);


  class FeaturesCollector2 : public FeaturesCollector
  {
    FilesContainerW m_writer;

    vector<FileWriter*> m_geoFile, m_trgFile;

    static const int m_scales = ARRAY_SIZE(g_arrScales);

  public:
    explicit FeaturesCollector2(string const & fName)
      : FeaturesCollector(fName + DATA_FILE_TAG), m_writer(fName)
    {
      for (int i = 0; i < m_scales; ++i)
      {
        string const postfix = utils::to_string(i);
        m_geoFile.push_back(new FileWriter(fName + GEOMETRY_FILE_TAG + postfix));
        m_trgFile.push_back(new FileWriter(fName + TRIANGLE_FILE_TAG + postfix));
      }
    }

    ~FeaturesCollector2()
    {
      WriteHeader();

      // assume like we close files
      m_datFile.Flush();

      m_writer.Append(m_datFile.GetName(), DATA_FILE_TAG);

      for (int i = 0; i < m_scales; ++i)
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

        FileWriter::DeleteFile(geomFile);
        FileWriter::DeleteFile(trgFile);
      }

      m_writer.Finish();
    }

    class GeometryHolder
    {
    public:
      FeatureBuilder2::buffers_holder_t m_buffer;

    private:
      FeaturesCollector2 & m_rMain;
      FeatureBuilder2 & m_rFB;

      typedef vector<m2::PointD> points_t;

      points_t m_current;

      void WriteOuterPoints(points_t const & points, int i)
      {
        m_buffer.m_ptsMask |= (1 << i);
        m_buffer.m_ptsOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_geoFile[i]));
        feature::SavePoints(points, *m_rMain.m_geoFile[i]);
      }

      void WriteOuterTriangles(points_t const & triangles, int i)
      {
        m_buffer.m_trgMask |= (1 << i);
        m_buffer.m_trgOffset.push_back(m_rMain.GetFileSize(*m_rMain.m_trgFile[i]));
        feature::SaveTriangles(triangles, *m_rMain.m_trgFile[i]);
      }

      void FillInnerPointsMask(points_t const & points, uint32_t scaleIndex)
      {
        points_t const & src = m_buffer.m_innerPts;
        ASSERT ( !src.empty(), () );

        ASSERT ( is_equal(src.front(), points.front()), () );
        ASSERT ( is_equal(src.back(), points.back()), () );

        size_t j = 1;
        for (size_t i = 1; i < points.size()-1; ++i)
        {
          for (; j < src.size()-1; ++j)
          {
            if (is_equal(src[j], points[i]))
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

    public:
      GeometryHolder(FeaturesCollector2 & rMain, FeatureBuilder2 & fb)
        : m_rMain(rMain), m_rFB(fb), m_ptsInner(true), m_trgInner(true)
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

      void AddTriangles(points_t & triangles, int scaleIndex)
      {
        if (m_trgInner && triangles.size() < 16)
        {
          if (m_buffer.m_innerTrg.empty())
            m_buffer.m_innerTrg.swap(triangles);
        }
        else
        {
          m_trgInner = false;
          WriteOuterTriangles(triangles, scaleIndex);
        }
      }
    };

    void operator() (FeatureBuilder2 & fb)
    {
      (void)GetFileSize(m_datFile);

      GeometryHolder holder(*this, fb);

      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();

      for (int i = m_scales-1; i >= 0; --i)
      {
        if (fb.IsDrawableInRange(i > 0 ? g_arrScales[i-1] + 1 : 0, g_arrScales[i]))
        {
          // simplify and serialize geometry
          points_t points;
          SimplifyPoints(holder.GetSourcePoints(), points, g_arrScales[i]);

          if (isLine)
            holder.AddPoints(points, i);

          if (isArea && points.size() > 2 && holder.NeedProcessTriangles())
          {
            // simplify and serialize triangles

            list<points_t> const & holes = fb.GetHoles();
            list<points_t> simpleHoles;
            for (list<points_t>::const_iterator iH = holes.begin(); iH != holes.end(); ++iH)
            {
              simpleHoles.push_back(points_t());

              SimplifyPoints(*iH, simpleHoles.back(), g_arrScales[i]);

              if (simpleHoles.back().size() < 3)
                simpleHoles.pop_back();
            }

            points_t triangles;
            feature::TesselateInterior(points, simpleHoles, triangles);

            if (!triangles.empty())
              holder.AddTriangles(triangles, i);
          }
        }
      }

      if (fb.PreSerialize(holder.m_buffer))
      {
        fb.Serialize(holder.m_buffer);

        WriteFeatureBase(holder.m_buffer.m_buffer, fb);
      }
    }
  };

  /// Simplify geometry for the upper scale.
  FeatureBuilder2 & GetFeatureBuilder2(FeatureBuilder1 & fb)
  {
    return static_cast<FeatureBuilder2 &>(fb);
  }


  bool GenerateFinalFeatures(string const & datFilePath, bool bSort)
  {
    // rename input file
    Platform & platform = GetPlatform();
    string tempDatFilePath(datFilePath);
    tempDatFilePath += ".notsorted";

    FileWriter::DeleteFile(tempDatFilePath);
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

      FeaturesCollector2 collector(datFilePath);

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
    FileWriter::DeleteFile(tempDatFilePath);

    FileWriter::DeleteFile(datFilePath + DATA_FILE_TAG);

    return true;
  }
} // namespace feature
