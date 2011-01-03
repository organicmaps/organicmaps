#include "feature_sorter.hpp"
#include "feature_generator.hpp"

#include "../../indexer/data_header.hpp"
#include "../../indexer/feature_processor.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../indexer/scales.hpp"
#include "../../indexer/cell_id.hpp"

#include "../../platform/platform.hpp"

#include "../../coding/file_writer.hpp"

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
  void SortDatFile(string const & datFilePath, bool removeOriginalFile)
  {
    // rename input file
    Platform & platform = GetPlatform();
    string tempDatFilePath(datFilePath);
    tempDatFilePath += ".notsorted";

    // file doesn't exist
    if (!platform.RenameFileX(datFilePath, tempDatFilePath))
    {
      LOG(LINFO, ("File ", datFilePath, " doesn't exist or sharing violation!"));
      return;
    }

    // stores cellIds for middle points
    CalculateMidPoints midPoints;
    ForEachFromDatRawFormat(tempDatFilePath, midPoints);

    // sort features by their middle point
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
    if (removeOriginalFile)
    {
      FileWriter::DeleteFile(tempDatFilePath);

      FileWriter::DeleteFile(datFilePath + DATA_FILE_TAG);
      FileWriter::DeleteFile(datFilePath + GEOMETRY_FILE_TAG);
      FileWriter::DeleteFile(datFilePath + TRIANGLE_FILE_TAG);
    }
  }
} // namespace feature
