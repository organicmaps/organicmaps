#pragma once
#include "indexer/cell_id.hpp"
#include "indexer/data_header.hpp"
#include "indexer/displacement_manager.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/feature_data.hpp"
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

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>


namespace covering
{
template <class TDisplacementManager>
class FeatureCoverer
{
public:
  FeatureCoverer(feature::DataHeader const & header, TDisplacementManager & manager,
                 std::vector<uint32_t> & featuresInBucket, std::vector<uint32_t> & cellsInBucket)
    : m_header(header)
    , m_scalesIdx(0)
    , m_bucketsCount(header.GetLastScale() + 1)
    , m_displacement(manager)
    , m_codingDepth(covering::GetCodingDepth<RectId::DEPTH_LEVELS>(header.GetLastScale()))
    , m_featuresInBucket(featuresInBucket)
    , m_cellsInBucket(cellsInBucket)
  {
    m_featuresInBucket.resize(m_bucketsCount);
    m_cellsInBucket.resize(m_bucketsCount);
  }

  template <class Feature>
  void operator()(Feature & ft, uint32_t index) const
  {
    m_scalesIdx = 0;
    uint32_t const minScaleClassif = min(
        scales::GetUpperScale(), feature::GetMinDrawableScaleClassifOnly(feature::TypesHolder(ft)));
    // The classificator won't allow this feature to be drawable for smaller
    // scales so the first buckets can be safely skipped.
    // todo(@pimenov) Parallelizing this loop may be helpful.
    for (uint32_t bucket = minScaleClassif; bucket < m_bucketsCount; ++bucket)
    {
      // There is a one-to-one correspondence between buckets and scales.
      // This is not immediately obvious and in fact there was an idea to map
      // a bucket to a contiguous range of scales.
      // todo(@pimenov): We probably should remove scale_index.hpp altogether.
      if (!FeatureShouldBeIndexed(ft, static_cast<int>(bucket), bucket == minScaleClassif /* needReset */))
      {
        continue;
      }

      std::vector<int64_t> const cells = covering::CoverFeature(ft, m_codingDepth, 250);
      m_displacement.Add(cells, bucket, ft, index);

      m_featuresInBucket[bucket] += 1;
      m_cellsInBucket[bucket] += cells.size();

      break;
    }
  }

private:
  // Every feature should be indexed at most once, namely for the smallest possible scale where
  //   -- its geometry is non-empty;
  //   -- it is visible;
  //   -- it is allowed by the classificator.
  // If the feature is invisible at all scales, do not index it.
  template <class Feature>
  bool FeatureShouldBeIndexed(Feature & ft, int scale, bool needReset) const
  {
    while (m_scalesIdx < m_header.GetScalesCount() && m_header.GetScale(m_scalesIdx) < scale)
    {
      ++m_scalesIdx;
      needReset = true;
    }

    if (needReset)
      ft.ResetGeometry();

    // This function invokes geometry reading for the needed scale.
    if (ft.IsEmptyGeometry(scale))
      return false;

    // This function assumes that geometry rect for the needed scale is already initialized.
    return feature::IsDrawableForIndexGeometryOnly(ft, scale);
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
  TDisplacementManager & m_displacement;
  int m_codingDepth;
  std::vector<uint32_t> & m_featuresInBucket;
  std::vector<uint32_t> & m_cellsInBucket;
};

template <class FeaturesVector, class Writer>
void IndexScales(feature::DataHeader const & header, FeaturesVector const & features,
                 Writer & writer, std::string const & tmpFilePrefix)
{
  // TODO: Make scale bucketing dynamic.

  uint32_t const bucketsCount = header.GetLastScale() + 1;

  std::string const cellsToFeatureAllBucketsFile =
      tmpFilePrefix + CELL2FEATURE_SORTED_EXT + ".allbuckets";
  MY_SCOPE_GUARD(cellsToFeatureAllBucketsFileGuard,
                 bind(&FileWriter::DeleteFileX, cellsToFeatureAllBucketsFile));
  {
    FileWriter cellsToFeaturesAllBucketsWriter(cellsToFeatureAllBucketsFile);

    using TSorter = FileSorter<CellFeatureBucketTuple, WriterFunctor<FileWriter>>;
    using TDisplacementManager = DisplacementManager<TSorter>;
    WriterFunctor<FileWriter> out(cellsToFeaturesAllBucketsWriter);
    TSorter sorter(1024 * 1024 /* bufferBytes */, tmpFilePrefix + CELL2FEATURE_TMP_EXT, out);
    // Heuristically rearrange and filter single-point features to simplify
    // the runtime decision of whether we should draw a feature
    // or sacrifice it for the sake of more important ones.
    TDisplacementManager manager(sorter);
    std::vector<uint32_t> featuresInBucket(bucketsCount);
    std::vector<uint32_t> cellsInBucket(bucketsCount);
    features.ForEach(
        FeatureCoverer<TDisplacementManager>(header, manager, featuresInBucket, cellsInBucket));
    manager.Displace();
    sorter.SortAndFinish();

    for (uint32_t bucket = 0; bucket < bucketsCount; ++bucket)
    {
      uint32_t const numCells = cellsInBucket[bucket];
      uint32_t const numFeatures = featuresInBucket[bucket];
      double const cellsPerFeature =
          numFeatures == 0 ? 0.0 : static_cast<double>(numCells) / static_cast<double>(numFeatures);
      LOG(LINFO, ("Scale index for bucket", bucket, ": Features:", numFeatures, "cells:", numCells,
                  "cells per feature:", cellsPerFeature));
    }
  }

  FileReader reader(cellsToFeatureAllBucketsFile);
  DDVector<CellFeatureBucketTuple, FileReader, uint64_t> cellsToFeaturesAllBuckets(reader);

  VarSerialVectorWriter<Writer> recordWriter(writer, bucketsCount);
  auto it = cellsToFeaturesAllBuckets.begin();

  for (uint32_t bucket = 0; bucket < bucketsCount; ++bucket)
  {
    std::string const cellsToFeatureFile = tmpFilePrefix + CELL2FEATURE_SORTED_EXT;
    MY_SCOPE_GUARD(cellsToFeatureFileGuard, bind(&FileWriter::DeleteFileX, cellsToFeatureFile));
    {
      FileWriter cellsToFeaturesWriter(cellsToFeatureFile);
      WriterFunctor<FileWriter> out(cellsToFeaturesWriter);
      while (it < cellsToFeaturesAllBuckets.end() && it->GetBucket() == bucket)
      {
        out(it->GetCellFeaturePair());
        ++it;
      }
    }

    {
      FileReader reader(cellsToFeatureFile);
      DDVector<CellFeatureBucketTuple::CellFeaturePair, FileReader, uint64_t> cellsToFeatures(
          reader);
      SubWriter<Writer> subWriter(writer);
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
