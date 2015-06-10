#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/interval_index_builder.hpp"

#include "defines.hpp"

#include "coding/dd_vector.hpp"
#include "coding/file_sort.hpp"
#include "coding/var_serial_vector.hpp"
#include "coding/writer.hpp"

#include "base/base.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/scope_guard.hpp"

#include "std/string.hpp"
#include "std/type_traits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace covering
{
class CellFeaturePair
{
public:
  CellFeaturePair() = default;
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
STATIC_ASSERT(is_trivially_copyable<CellFeaturePair>::value);

class CellFeatureBucketTuple
{
public:
  CellFeatureBucketTuple() = default;
  CellFeatureBucketTuple(CellFeaturePair const & p, uint32_t bucket) : m_pair(p), m_bucket(bucket)
  {
  }

  bool operator<(CellFeatureBucketTuple const & rhs) const
  {
    if (m_bucket != rhs.m_bucket)
      return m_bucket < rhs.m_bucket;
    return m_pair < rhs.m_pair;
  }

  CellFeaturePair const & GetCellFeaturePair() const { return m_pair; }
  uint32_t GetBucket() const { return m_bucket; }

private:
  CellFeaturePair m_pair;
  uint32_t m_bucket;
};
STATIC_ASSERT(sizeof(CellFeatureBucketTuple) == 16);
STATIC_ASSERT(is_trivially_copyable<CellFeatureBucketTuple>::value);

template <class SorterT>
class FeatureCoverer
{
public:
  FeatureCoverer(feature::DataHeader const & header, SorterT & sorter,
                 vector<uint32_t> & featuresInBucket, vector<uint32_t> & cellsInBucket)
      : m_header(header),
        m_scalesIdx(0),
        m_bucketsCount(header.GetLastScale() + 1),
        m_sorter(sorter),
        m_codingDepth(covering::GetCodingDepth(header.GetLastScale())),
        m_featuresInBucket(featuresInBucket),
        m_cellsInBucket(cellsInBucket)
  {
    m_featuresInBucket.resize(m_bucketsCount);
    m_cellsInBucket.resize(m_bucketsCount);
  }

  template <class TFeature>
  void operator() (TFeature const & f, uint32_t offset) const
  {
    uint32_t minScale = 0;
    bool skip = false;
    m_scalesIdx = 0;
    for (uint32_t bucket = 0; bucket < m_bucketsCount; ++bucket)
    {
      // There is a one-to-one correspondence between buckets and scales.
      // This is not immediately obvious and in fact there was an idea to map
      // a bucket to a contiguous range of scales.
      // todo(@pimenov): We probably should remove scale_index.hpp altogether.
      if (!FeatureShouldBeIndexed(f, offset, bucket, skip, minScale))
        continue;

      vector<int64_t> const cells = covering::CoverFeature(f, m_codingDepth, 250);
      for (int64_t cell : cells)
        m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, offset), bucket));

      m_featuresInBucket[bucket] += 1;
      m_cellsInBucket[bucket] += cells.size();
    }
  }

private:
  template <class TFeature>
  bool FeatureShouldBeIndexed(TFeature const & f, uint32_t offset, uint32_t scale, bool & skip,
                              uint32_t & minScale) const
  {
    // Do index features for the first visible interval only once.
    // If the feature was skipped as empty for the suitable interval,
    // it should be indexed in the next interval where geometry is not empty.

    bool needReset = (scale == 0);
    while (m_scalesIdx < m_header.GetScalesCount() && m_header.GetScale(m_scalesIdx) < scale)
    {
      ++m_scalesIdx;
      needReset = true;
    }

    if (needReset)
      f.ResetGeometry();

    // This function invokes geometry reading for the needed scale.
    if (f.IsEmptyGeometry(scale))
    {
      skip = true;
      return false;
    }

    if (needReset)
    {
      // This function assumes that geometry rect for the needed scale is already initialized.
      // Note: it works with FeatureBase so in fact it does not use the information about
      // the feature's geometry except for the type and the LimitRect.
      minScale = feature::GetMinDrawableScale(f);
    }

    if (minScale == scale)
    {
      skip = false;
      return true;
    }

    if (minScale < scale && skip)
    {
      skip = false;
      return true;
    }
    return false;
  }

  // We do not need to parse a feature's geometry for every bucket.
  // The scales at which geometry changes are encoded in the mwm header.
  // We cannot know them beforehand because they are different for
  // the common case of a country file and the special case of the world file.
  // m_scaleIdx is a position in the scales array that should be reset for every feature
  // and then only move forward. Its purpose is to detect the moments when we
  // need to reread the feature's geometry.
  feature::DataHeader const & m_header;
  mutable size_t m_scalesIdx;

  uint32_t m_bucketsCount;
  SorterT & m_sorter;
  int m_codingDepth;
  vector<uint32_t> & m_featuresInBucket;
  vector<uint32_t> & m_cellsInBucket;
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

template <class TFeaturesVector, class TWriter>
void IndexScales(feature::DataHeader const & header, TFeaturesVector const & featuresVector,
                 TWriter & writer, string const & tmpFilePrefix)
{
  // TODO: Make scale bucketing dynamic.

  int bucketsCount = header.GetLastScale() + 1;

  string const cells2featureAllBucketsFile =
      tmpFilePrefix + CELL2FEATURE_SORTED_EXT + ".allbuckets";
  MY_SCOPE_GUARD(cells2featureAllBucketsFileGuard,
                 bind(&FileWriter::DeleteFileX, cells2featureAllBucketsFile));
  {
    FileWriter cellsToFeaturesAllBucketsWriter(cells2featureAllBucketsFile);

    typedef FileSorter<CellFeatureBucketTuple, WriterFunctor<FileWriter> > TSorter;
    WriterFunctor<FileWriter> out(cellsToFeaturesAllBucketsWriter);
    TSorter sorter(1024 * 1024 /* bufferBytes */, tmpFilePrefix + CELL2FEATURE_TMP_EXT, out);
    vector<uint32_t> featuresInBucket(bucketsCount);
    vector<uint32_t> cellsInBucket(bucketsCount);
    featuresVector.ForEachOffset(
        FeatureCoverer<TSorter>(header, sorter, featuresInBucket, cellsInBucket));
    sorter.SortAndFinish();

    for (uint32_t bucket = 0; bucket < bucketsCount; ++bucket)
    {
      uint32_t const numCells = cellsInBucket[bucket];
      uint32_t const numFeatures = featuresInBucket[bucket];
      LOG(LINFO, ("Building scale index for bucket:", bucket));
      double const cellsPerFeature =
          numFeatures == 0 ? 0.0 : static_cast<double>(numCells) / static_cast<double>(numFeatures);
      LOG(LINFO,
          ("Features:", numFeatures, "cells:", numCells, "cells per feature:", cellsPerFeature));
    }
  }

  FileReader reader(cells2featureAllBucketsFile);
  DDVector<CellFeatureBucketTuple, FileReader, uint64_t> cellsToFeaturesAllBuckets(reader);

  VarSerialVectorWriter<TWriter> recordWriter(writer, bucketsCount);
  auto it = cellsToFeaturesAllBuckets.begin();

  for (uint32_t bucket = 0; bucket < bucketsCount; ++bucket)
  {
    string const cells2featureFile = tmpFilePrefix + CELL2FEATURE_SORTED_EXT;
    MY_SCOPE_GUARD(cells2featureFileGuard, bind(&FileWriter::DeleteFileX, cells2featureFile));
    {
      FileWriter cellsToFeaturesWriter(cells2featureFile);
      WriterFunctor<FileWriter> out(cellsToFeaturesWriter);
      while (it < cellsToFeaturesAllBuckets.end() && it->GetBucket() == bucket)
      {
        out(it->GetCellFeaturePair());
        ++it;
      }
    }

    {
      FileReader reader(cells2featureFile);
      DDVector<CellFeaturePair, FileReader, uint64_t> cellsToFeatures(reader);
      SubWriter<TWriter> subWriter(writer);
      LOG(LINFO, ("Building interval index for bucket:", bucket));
      BuildIntervalIndex(cellsToFeatures.begin(), cellsToFeatures.end(), subWriter,
                         RectId::DEPTH_LEVELS * 2 + 1);
    }
    recordWriter.FinishRecord();
  }

  // todo(@pimenov). There was an old todo here that said there were
  // features (coastlines) that have been indexed despite being invisible at the last scale.
  // This should be impossible but it is better to recheck it.

  LOG(LINFO, ("All scale indexes done."));
}

}  // namespace covering
