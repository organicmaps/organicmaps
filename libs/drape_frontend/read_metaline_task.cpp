#include "drape_frontend/read_metaline_task.hpp"
#include "drape_frontend/map_data_provider.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/metaline_data.hpp"

#include "coding/files_container.hpp"

#include "defines.hpp"

#include <algorithm>
#include <map>
#include <vector>

namespace df
{
namespace
{
using feature::metalines::MetalineData;

std::vector<MetalineData> ReadMetalinesFromFile(MwmSet::MwmId const & mwmId)
{
  try
  {
    FilesContainerR cont(mwmId.GetInfo()->GetLocalFile().GetPath(MapFileType::Map));
    ReaderSource src(cont.GetReader(METALINES_FILE_TAG));

    auto model = feature::metalines::ReadMetalines(src, mwmId);
    if (cont.IsExist(METALINES_OFFSETS_FILE_TAG))
    {
      // The legacy section remains authoritative. A missing, stale or damaged
      // optional section must not discard merged labels and shields.
      try
      {
        ReaderSource offsets(cont.GetReader(METALINES_OFFSETS_FILE_TAG));
        feature::metalines::TryReadMetalineOffsets(offsets, mwmId, model);
      }
      catch (Reader::Exception const &)
      {}
    }
    return model;
  }
  catch (Reader::Exception const &)
  {
    return {};
  }
}

std::map<FeatureID, std::vector<m2::PointD>> ReadPoints(df::MapDataProvider & model, MetalineData const & metaline)
{
  auto features = metaline.m_features;
  std::sort(features.begin(), features.end());

  std::map<FeatureID, std::vector<m2::PointD>> result;
  model.ReadFeatures([&metaline, &result](FeatureType & ft)
  {
    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const count = ft.GetPointsCount();
    if (count == 0)
      return;

    std::vector<m2::PointD> featurePoints;
    featurePoints.reserve(count);
    featurePoints.push_back(ft.GetPoint(0));

    for (size_t i = 1; i < count; ++i)
    {
      auto const & pt = ft.GetPoint(i);
      if (!featurePoints.back().EqualDxDy(pt, kMwmPointAccuracy))
        featurePoints.push_back(pt);
    }

    if (metaline.m_reversed.find(ft.GetID()) != metaline.m_reversed.cend())
      std::reverse(featurePoints.begin(), featurePoints.end());

    result.emplace(ft.GetID(), std::move(featurePoints));
  }, features);

  return result;
}

std::vector<m2::PointD> MergePoints(std::map<FeatureID, std::vector<m2::PointD>> && points,
                                    std::vector<FeatureID> const & featuresOrder)
{
  if (points.size() == 1)
    return std::move(points.begin()->second);

  size_t sz = 0;
  for (auto const & p : points)
    sz += p.second.size();

  std::vector<m2::PointD> result;
  result.reserve(sz);
  for (auto const & f : featuresOrder)
  {
    auto const it = points.find(f);
    ASSERT(it != points.cend(), ());

    auto first = it->second.begin();
    if (!result.empty() && result.back().EqualDxDy(*first, kMwmPointAccuracy))
      ++first;
    result.insert(result.end(), first, it->second.end());
  }

  return result;
}

double GetPathLength(std::vector<m2::PointD> const & points)
{
  double length = 0.0;
  for (size_t i = 1; i < points.size(); ++i)
    length += points[i - 1].Length(points[i]);
  return length;
}
}  // namespace

ReadMetalineTask::ReadMetalineTask(MapDataProvider & model, MwmSet::MwmId const & mwmId)
  : m_model(model)
  , m_mwmId(mwmId)
  , m_isCancelled(false)
{}

void ReadMetalineTask::Run()
{
  /// @todo Naive check for now. Should refactor with MwmHandle lock here.
  if (!m_mwmId.IsAlive() || !m_mwmId.GetInfo()->IsRegistered())
    return;

  auto metalines = ReadMetalinesFromFile(m_mwmId);
  for (auto & metaline : metalines)
  {
    if (m_isCancelled)
      return;

    bool failed = false;
    for (auto const & fid : metaline.m_features)
    {
      if (m_metalines.find(fid) != m_metalines.end())
      {
        failed = true;
        break;
      }
    }
    if (failed)
      continue;

    auto points = ReadPoints(m_model, metaline);
    // Nothing is read if the mwm was deregistered (updated or deleted) in the meantime.
    if (points.empty())
    {
      m_metalines.clear();
      return;
    }

    // Never bridge across a missing feature or merge a repeated feature twice.
    if (points.size() != metaline.m_features.size())
      continue;
    if (metaline.m_dashPhaseOffsets.empty())
    {
      metaline.m_dashPhaseOffsets.reserve(metaline.m_features.size());
      double chainOffset = 0.0;
      for (auto const & fid : metaline.m_features)
      {
        double const length = GetPathLength(points.at(fid));
        metaline.m_dashPhaseOffsets.push_back(chainOffset + (metaline.m_reversed.contains(fid) ? length : 0.0));
        chainOffset += length;
      }
    }

    auto mergedPoints = MergePoints(std::move(points), metaline.m_features);
    if (mergedPoints.size() < 2)
      continue;

    m2::SharedSpline const spline(std::move(mergedPoints));
    for (size_t i = 0; i < metaline.m_features.size(); ++i)
    {
      auto const & fid = metaline.m_features[i];
      m_metalines.emplace(fid, MetalineInfo{spline, metaline.m_dashPhaseOffsets[i], metaline.m_reversed.contains(fid)});
    }
  }
}

bool ReadMetalineTask::UpdateCache(MetalineCache & cache)
{
  if (m_metalines.empty())
    return false;

  cache.merge(m_metalines);
  m_metalines.clear();
  return true;
}

}  // namespace df
