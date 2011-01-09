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
      case 1:
        out.push_back(in.back());
        break;
      default:
        if (!out.back().EqualDxDy(in.back(), MercatorBounds::GetCellID2PointAbsEpsilon()))
          out.push_back(in.back());
      }
    }
  }

  void TesselateInterior(points_t const & bound, list<points_t> const & holes, points_t & triangles);


  class FeaturesCollectorRef : public FeaturesCollector
  {
    FilesContainerW m_writer;

    vector<FileWriter*> m_geoFile, m_trgFile;

    static const int m_scales = ARRAY_SIZE(g_arrScales);

  public:
    explicit FeaturesCollectorRef(string const & fName)
      : FeaturesCollector(fName + DATA_FILE_TAG), m_writer(fName)
    {
      for (int i = 0; i < m_scales; ++i)
      {
        string const postfix = utils::to_string(i);
        m_geoFile.push_back(new FileWriter(fName + GEOMETRY_FILE_TAG + postfix));
        m_trgFile.push_back(new FileWriter(fName + TRIANGLE_FILE_TAG + postfix));
      }
    }

    ~FeaturesCollectorRef()
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

    void operator() (FeatureBuilder2 & fb)
    {
      (void)GetFileSize(m_datFile);

      FeatureBuilder2::buffers_holder_t buffer;

      bool const isLine = fb.IsLine();
      bool const isArea = fb.IsArea();

      int lowS = 0;
      for (int i = 0; i < m_scales; ++i)
      {
        if (fb.IsDrawableInRange(lowS, g_arrScales[i]))
        {
          // simplify and serialize geometry
          points_t points;
          SimplifyPoints(fb.GetGeometry(), points, g_arrScales[i]);

          if (isLine)
          {
            buffer.m_lineMask |= (1 << i);
            buffer.m_lineOffset.push_back(GetFileSize(*m_geoFile[i]));
            feature::SavePoints(points, *m_geoFile[i]);
          }

          if (isArea && points.size() > 2)
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
            {
              buffer.m_trgMask |= (1 << i);
              buffer.m_trgOffset.push_back(GetFileSize(*m_trgFile[i]));
              feature::SaveTriangles(triangles, *m_trgFile[i]);
            }
          }
        }
        lowS = g_arrScales[i]+1;
      }

      if (fb.PreSerialize(buffer))
      {
        fb.Serialize(buffer);

        WriteFeatureBase(buffer.m_buffer, fb);
      }
    }
  };


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

      FeaturesCollectorRef collector(datFilePath);

      FeatureBuilder1::buffer_t buffer;
      for (size_t i = 0; i < midPoints.m_vec.size(); ++i)
      {
        ReaderSource<FileReader> src(reader);
        src.Skip(midPoints.m_vec[i].second);

        FeatureBuilder1 f;
        feature::ReadFromSourceRowFormat(src, f);

        // emit the feature
        collector(static_cast<FeatureBuilder2 &>(f));
      }

      // at this point files should be closed
    }

    // remove old not-sorted dat file
    FileWriter::DeleteFile(tempDatFilePath);

    FileWriter::DeleteFile(datFilePath + DATA_FILE_TAG);

    return true;
  }
} // namespace feature
