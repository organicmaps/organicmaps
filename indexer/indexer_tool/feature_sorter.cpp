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

    void operator() (FeatureGeom const & ft, uint64_t pos)
    {
      // reset state
      m_midX = 0.0;
      m_midY = 0.0;
      m_counter = 0;
      ft.ForEachPointRef(*this, FeatureGeom::m_defScale);
      m_midX /= m_counter;
      m_midY /= m_counter;

      uint64_t const pointAsInt64 = PointToInt64(m_midX, m_midY);
      uint64_t const minScale = feature::MinDrawableScaleForFeature(ft);
      CHECK(minScale <= scales::GetUpperScale(), ("Dat file contain invisible feature"));

      uint64_t const order = (minScale << 59) | (pointAsInt64 >> 5);
      m_vec.push_back(make_pair(order, pos));
    }

    void operator() (CoordPointT const & point)
    {
      m_midX += point.first;
      m_midY += point.second;
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
        my::sq(scales::GetEpsilonForLevel(level + 1)), MakeBackInsertFunctor(out));

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

    void operator() (FeatureBuilderGeomRef const & fb)
    {
      (void)GetFileSize(m_datFile);

      FeatureBuilderGeomRef::buffers_holder_t buffer;

      int lowS = 0;
      for (int i = 0; i < m_scales; ++i)
      {
        if (fb.IsDrawableLikeLine(lowS, g_arrScales[i]))
        {
          buffer.m_mask |= (1 << i);

          buffer.m_lineOffset.push_back(GetFileSize(*m_geoFile[i]));

          // serialize points
          points_t points;
          SimplifyPoints(fb.GetGeometry(), points, g_arrScales[i]);
          feature::SerializePoints(points, *m_geoFile[i]);
        }
        lowS = g_arrScales[i]+1;
      }

      if (!fb.GetTriangles().empty())
      {
        buffer.m_trgOffset.push_back(GetFileSize(*m_trgFile[0]));
        feature::SerializeTriangles(fb.GetTriangles(), *m_trgFile[0]);
      }

      fb.Serialize(buffer);

      WriteFeatureBase(buffer.m_buffer, fb);
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

      FeatureGeom::read_source_t buffer;
      for (size_t i = 0; i < midPoints.m_vec.size(); ++i)
      {
        ReaderSource<FileReader> src(reader);
        src.Skip(midPoints.m_vec[i].second);

        FeatureGeom f;
        feature::ReadFromSource(src, f, buffer);

        FeatureBuilderType fb;
        f.InitFeatureBuilder(fb);

        // emit the feature
        collector(fb);
      }

      // at this point files should be closed
    }

    // remove old not-sorted dat file
    FileWriter::DeleteFile(tempDatFilePath);

    FileWriter::DeleteFile(datFilePath + DATA_FILE_TAG);

    return true;
  }
} // namespace feature
