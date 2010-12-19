#pragma once
#include "scale_index.hpp"
#include "feature_visibility.hpp"
#include "covering.hpp"
#include "interval_index_builder.hpp"
#include "cell_id.hpp"

#include "../geometry/covering_stream_optimizer.hpp"

#include "../coding/dd_vector.hpp"
#include "../coding/file_sort.hpp"
#include "../coding/var_serial_vector.hpp"
#include "../coding/writer.hpp"

#include "../base/base.hpp"
#include "../base/logging.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"


#pragma pack(push, 1)
  struct CellFeaturePair
  {
    int64_t first;
    uint32_t second;

    CellFeaturePair() {}
    CellFeaturePair(pair<int64_t, uint32_t> const & p) : first(p.first), second(p.second) {}
    CellFeaturePair(int64_t f, uint32_t s) : first(f), second(s) {}

    bool operator< (CellFeaturePair const & rhs) const
    {
      if (first == rhs.first)
        return (second < rhs.second);
      return (first < rhs.first);
    }
  };
#pragma pack (pop)

template <class SorterT>
class FeatureCoverer
{
public:
  FeatureCoverer(uint32_t bucket,
                 SorterT & sorter,
                 uint32_t & numFeatures) :
      m_Sorter(sorter),
      m_ScaleRange(ScaleIndexBase::ScaleRangeForBucket(bucket)),
      m_NumFeatures(numFeatures)
  {
  }

  template <class TFeature>
  void operator() (TFeature const & f, uint32_t offset) const
  {
    if (FeatureShouldBeIndexed(f))
    {
      vector<int64_t> const cells = covering::CoverFeature(f, m_ScaleRange.second);
      for (vector<int64_t>::const_iterator it = cells.begin(); it != cells.end(); ++it)
        m_Sorter.Add(make_pair(*it, offset));
      ++m_NumFeatures;
      return;
    }
  }

  template <class TFeature>
  bool FeatureShouldBeIndexed(TFeature const & f) const
  {
    (void)f.GetLimitRect(); // dummy call to force TFeature::ParseGeometry

    uint32_t const minScale = feature::MinDrawableScaleForFeature(f);
    return (m_ScaleRange.first <= minScale && minScale < m_ScaleRange.second);
  }

private:
  SorterT & m_Sorter;
  pair<uint32_t, uint32_t> m_ScaleRange;
  uint32_t & m_NumFeatures;
};

template <class SinkT>
class CellFeaturePairSinkAdapter
{
public:
  explicit CellFeaturePairSinkAdapter(SinkT & sink) : m_Sink(sink) {}

  void operator() (int64_t cellId, uint64_t value) const
  {
    // uint64_t -> uint32_t : assume that feature dat file not more than 4Gb
    CellFeaturePair cellFeaturePair(cellId, static_cast<uint32_t>(value));
    m_Sink.Write(&cellFeaturePair, sizeof(cellFeaturePair));
  }

private:
  SinkT & m_Sink;
};

template <class SinkT>
class FeatureCoveringOptimizeProxySink
{
public:
  FeatureCoveringOptimizeProxySink(SinkT & sink)
    : m_Sink(sink), m_Optimizer(m_Sink, 100, 10)
  {
  }

  ~FeatureCoveringOptimizeProxySink()
  {
    m_Optimizer.Flush();
  }

  void operator () (CellFeaturePair const & cellFeaturePair)
  {
    m_Optimizer.Add(cellFeaturePair.first, cellFeaturePair.second);
  }

private:
  CellFeaturePairSinkAdapter<SinkT> m_Sink;
  covering::CoveringStreamOptimizer<RectId, uint32_t, CellFeaturePairSinkAdapter<SinkT> >
    m_Optimizer;
};

template <class FeaturesVectorT, class WriterT>
inline void IndexScales(FeaturesVectorT const & featuresVector,
                        WriterT & writer,
                        string const & tmpFilePrefix)
{
  // TODO: Add global feature covering optimization.
  // TODO: Make scale bucketing dynamic.
  // TODO: Compute covering only once?

  //typedef pair<int64_t, uint32_t> CellFeaturePair;
  STATIC_ASSERT(sizeof(CellFeaturePair) == 12);

  VarSerialVectorWriter<WriterT> recordWriter(writer, ScaleIndexBase::NUM_BUCKETS);
  for (uint32_t bucket = 0; bucket < ScaleIndexBase::NUM_BUCKETS; ++bucket)
  {
    LOG(LINFO, ("Building scale index for bucket:", bucket))
    uint32_t numFeatures = 0;
    {
      FileWriter cellsToFeaturesWriter(tmpFilePrefix + ".c2f.sorted");

      typedef FeatureCoveringOptimizeProxySink<FileWriter> OptimizeSink;
      OptimizeSink optimizeSink(cellsToFeaturesWriter);
      typedef FileSorter<CellFeaturePair, OptimizeSink> SorterType;
      SorterType sorter(1024*1024, tmpFilePrefix + ".c2f.tmp", optimizeSink);
      /*
      typedef FileSorter<CellFeaturePair, WriterFunctor<FileWriter> > SorterType;
      WriterFunctor<FileWriter> out(cellsToFeaturesWriter);
      SorterType sorter(1024*1024, tmpFilePrefix + ".c2f.tmp", out);
      */
      featuresVector.ForEachOffset(FeatureCoverer<SorterType>(bucket, sorter, numFeatures));
      LOG(LINFO, ("Sorting..."));
      sorter.SortAndFinish();
    }
    LOG(LINFO, ("Indexing..."));
    {
      FileReader reader(tmpFilePrefix + ".c2f.sorted");
      uint64_t const numCells = reader.Size() / sizeof(CellFeaturePair);
      DDVector<CellFeaturePair, FileReader, uint64_t> cellsToFeatures(reader, numCells);
      LOG(LINFO, ("Being indexed", "features:", numFeatures, "cells:", numCells));
      SubWriter<WriterT> subWriter(writer);
      BuildIntervalIndex<5>(cellsToFeatures.begin(), cellsToFeatures.end(), subWriter);
    }
    FileWriter::DeleteFile(tmpFilePrefix + ".c2f.sorted");
    LOG(LINFO, ("Indexing done."));
    recordWriter.FinishRecord();
  }
  LOG(LINFO, ("All scale indexes done."));
}
