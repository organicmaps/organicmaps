#include "feature_sorter.hpp"
#include "feature_generator.hpp"

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
      ft.ForEachPointRef(*this);
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

  template <typename TReader>
  void ReadFeature(TReader const & reader, FeatureGeom & ft, uint64_t offset)
  {
    ReaderSource<TReader> src(reader);
    src.Skip(offset);

    feature::ReadFromSource(src, ft);
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
    ForEachFromDat(tempDatFilePath, midPoints);

    std::sort(midPoints.m_vec.begin(), midPoints.m_vec.end(), &SortMidPointsFunc);

    // store sorted features
    {
      FeaturesCollector collector(datFilePath);
      FileReader notSortedFileReader(tempDatFilePath);

      FeatureGeom ft;
      for (size_t i = 0; i < midPoints.m_vec.size(); ++i)
      {
        ReadFeature(notSortedFileReader, ft, midPoints.m_vec[i].second);
        collector(ft);
      }
    }

    // remove old not-sorted dat file
    if (removeOriginalFile)
      FileWriter::DeleteFile(tempDatFilePath);
  }
} // namespace feature
