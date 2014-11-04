#pragma once
#include "scale_index.hpp"
#include "feature_covering.hpp"
#include "feature_visibility.hpp"
#include "feature.hpp"
#include "interval_index_builder.hpp"
#include "cell_id.hpp"

#include "../coding/dd_vector.hpp"
#include "../coding/file_sort.hpp"
#include "../coding/var_serial_vector.hpp"
#include "../coding/writer.hpp"

#include "../base/base.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"
#include "../std/unordered_set.hpp"


class CellFeaturePair
{
public:
  CellFeaturePair() {}
  CellFeaturePair(uint64_t cell, uint32_t feature)
    : m_CellLo(UINT64_LO(cell)), m_CellHi(UINT64_HI(cell)), m_Feature(feature) {}

  bool operator< (CellFeaturePair const & rhs) const
  {
    if (m_CellHi != rhs.m_CellHi)
      return m_CellHi < rhs.m_CellHi;
    if (m_CellLo != rhs.m_CellLo)
      return m_CellLo < rhs.m_CellLo;
    return m_Feature < rhs.m_Feature;
  }

  uint64_t GetCell() const { return UINT64_FROM_UINT32(m_CellHi, m_CellLo); }
  uint32_t GetFeature() const { return m_Feature; }

private:
  uint32_t m_CellLo;
  uint32_t m_CellHi;
  uint32_t m_Feature;
};
STATIC_ASSERT(sizeof(CellFeaturePair) == 12);

template <class SorterT>
class FeatureCoverer
{
  unordered_set<uint32_t> & m_skipped;

public:
  FeatureCoverer(unordered_set<uint32_t> & skipped,
                 uint32_t bucket,
                 int codingScale,
                 SorterT & sorter,
                 uint32_t & numFeatures)
    : m_skipped(skipped), m_Sorter(sorter),
      m_codingDepth(covering::GetCodingDepth(codingScale)),
      m_ScaleRange(ScaleIndexBase::ScaleRangeForBucket(bucket)),
      m_NumFeatures(numFeatures)
  {
    ASSERT_LESS(m_ScaleRange.first, m_ScaleRange.second, ());
  }

  template <class TFeature>
  void operator() (TFeature const & f, uint32_t offset) const
  {
    if (FeatureShouldBeIndexed(f, offset))
    {
      vector<int64_t> const cells = covering::CoverFeature(f, m_codingDepth, 250);
      for (int64_t cell : cells)
        m_Sorter.Add(CellFeaturePair(cell, offset));

      ++m_NumFeatures;
    }
  }

private:
  template <class TFeature>
  bool FeatureShouldBeIndexed(TFeature const & f, uint32_t offset) const
  {
    // Do index features for the first visible interval only once.
    // If the feature was skipped as empty for the suitable interval,
    // it should be indexed in the next interval where geometry is not empty.

    // This function invokes geometry reading for the needed scale.
    if (f.IsEmptyGeometry(m_ScaleRange.second - 1))
    {
      m_skipped.insert(offset);
      return false;
    }

    // This function assumes that geometry rect for the needed scale is already initialized.
    uint32_t const minScale = feature::GetMinDrawableScale(f);
    if (m_ScaleRange.first <= minScale && minScale < m_ScaleRange.second)
    {
      (void) m_skipped.erase(offset);
      return true;
    }

    return (minScale < m_ScaleRange.first && m_skipped.erase(offset) == 1);
  }

  SorterT & m_Sorter;
  int m_codingDepth;
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

template <class FeaturesVectorT, class WriterT>
inline void IndexScales(uint32_t bucketsCount,
                        int codingScale,
                        FeaturesVectorT const & featuresVector,
                        WriterT & writer,
                        string const & tmpFilePrefix)
{
  // TODO: Make scale bucketing dynamic.

  STATIC_ASSERT(sizeof(CellFeaturePair) == 12);

  unordered_set<uint32_t> skipped;

  VarSerialVectorWriter<WriterT> recordWriter(writer, bucketsCount);
  for (uint32_t bucket = 0; bucket < bucketsCount; ++bucket)
  {
    LOG(LINFO, ("Building scale index for bucket:", bucket));

    uint32_t numFeatures = 0;
    {
      FileWriter cellsToFeaturesWriter(tmpFilePrefix + ".c2f.sorted");

      typedef FileSorter<CellFeaturePair, WriterFunctor<FileWriter> > SorterType;
      WriterFunctor<FileWriter> out(cellsToFeaturesWriter);
      SorterType sorter(1024*1024, tmpFilePrefix + ".c2f.tmp", out);
      featuresVector.ForEachOffset(
            FeatureCoverer<SorterType>(skipped, bucket, codingScale, sorter, numFeatures));
      // LOG(LINFO, ("Sorting..."));
      sorter.SortAndFinish();
    }

    // LOG(LINFO, ("Indexing..."));
    {
      FileReader reader(tmpFilePrefix + ".c2f.sorted");
      uint64_t const numCells = reader.Size() / sizeof(CellFeaturePair);
      DDVector<CellFeaturePair, FileReader, uint64_t> cellsToFeatures(reader);
      LOG(LINFO, ("Being indexed", "features:", numFeatures, "cells:", numCells,
                  "cells per feature:", (numCells + 1.0) / (numFeatures + 1.0)));
      SubWriter<WriterT> subWriter(writer);
      BuildIntervalIndex(cellsToFeatures.begin(), cellsToFeatures.end(), subWriter,
                         RectId::DEPTH_LEVELS * 2 + 1);
    }

    FileWriter::DeleteFileX(tmpFilePrefix + ".c2f.sorted");
    // LOG(LINFO, ("Indexing done."));
    recordWriter.FinishRecord();
  }

  CHECK(skipped.empty(), ());
  LOG(LINFO, ("All scale indexes done."));
}
