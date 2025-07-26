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

  // Every feature should be indexed at most once, namely for the smallest possible scale where
  //   -- its geometry is non-empty;
  //   -- it is visible;
  //   -- it is allowed by the classificator.
  // If the feature is invisible at all scales, do not index it.
  template <class Feature>
  void operator()(Feature & ft, uint32_t index) const
  {
    m_scalesIdx = 0;
    uint32_t const minScaleClassif =
        std::min(scales::GetUpperScale(), feature::GetMinDrawableScaleClassifOnly(feature::TypesHolder(ft)));
    // The classificator won't allow this feature to be drawable for smaller
    // scales so the first buckets can be safely skipped.
    // todo(@pimenov) Parallelizing this loop may be helpful.
    // TODO: skip index building for scales [0,9] for country files and scales 10+ for the world file.
    for (uint32_t bucket = minScaleClassif; bucket < m_bucketsCount; ++bucket)
    {
      // There is a one-to-one correspondence between buckets and scales.
      // This is not immediately obvious and in fact there was an idea to map
      // a bucket to a contiguous range of scales.
      // todo(@pimenov): We probably should remove scale_index.hpp altogether.

      // Check feature's geometry and visibility.
      if (!FeatureShouldBeIndexed(ft, static_cast<int>(bucket), bucket == minScaleClassif /* needReset */))
        continue;

      std::vector<int64_t> const cells = covering::CoverFeature(ft, m_codingDepth, 250);
      m_displacement.Add(cells, bucket, ft, index);

      m_featuresInBucket[bucket] += 1;
      m_cellsInBucket[bucket] += cells.size();

      break;
    }
  }

private:
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

    // Checks feature's geometry is suitable for the needed scale; assumes the geometry rect is already initialized.
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
void IndexScales(feature::DataHeader const & header, FeaturesVector const & features, Writer & writer,
                 std::string const &)
{
  // TODO: Make scale bucketing dynamic.

  uint32_t const bucketsCount = header.GetLastScale() + 1;

  std::vector<CellFeatureBucketTuple> cellsToFeaturesAllBuckets;
  {
    auto const PushCFT = [&cellsToFeaturesAllBuckets](CellFeatureBucketTuple const & v)
    { cellsToFeaturesAllBuckets.push_back(v); };
    using TDisplacementManager = DisplacementManager<decltype(PushCFT)>;

    // Single-point features are heuristically rearranged and filtered to simplify
    // the runtime decision of whether we should draw a feature
    // or sacrifice it for the sake of more important ones ("displacement").
    // Lines and areas are not displaceable and are just passed on to the index.
    TDisplacementManager manager(PushCFT);
    std::vector<uint32_t> featuresInBucket(bucketsCount);
    std::vector<uint32_t> cellsInBucket(bucketsCount);
    features.ForEach(FeatureCoverer<TDisplacementManager>(header, manager, featuresInBucket, cellsInBucket));
    manager.Displace();
    std::sort(cellsToFeaturesAllBuckets.begin(), cellsToFeaturesAllBuckets.end());

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

  VarSerialVectorWriter<Writer> recordWriter(writer, bucketsCount);
  auto it = cellsToFeaturesAllBuckets.begin();

  for (uint32_t bucket = 0; bucket < bucketsCount; ++bucket)
  {
    /// @todo Can avoid additional vector here with the smart iterator adapter.
    std::vector<CellFeatureBucketTuple::CellFeaturePair> cellsToFeatures;
    while (it < cellsToFeaturesAllBuckets.end() && it->GetBucket() == bucket)
    {
      cellsToFeatures.push_back(it->GetCellFeaturePair());
      ++it;
    }

    SubWriter<Writer> subWriter(writer);
    LOG(LINFO, ("Building interval index for bucket:", bucket));
    BuildIntervalIndex(cellsToFeatures.begin(), cellsToFeatures.end(), subWriter, RectId::DEPTH_LEVELS * 2 + 1);

    recordWriter.FinishRecord();
  }

  // todo(@pimenov). There was an old todo here that said there were
  // features (coastlines) that have been indexed despite being invisible at the last scale.
  // This should be impossible but it is better to recheck it.

  LOG(LINFO, ("All scale indexes done."));
}

}  // namespace covering
