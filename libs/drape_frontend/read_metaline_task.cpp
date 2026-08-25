#include "drape_frontend/read_metaline_task.hpp"
#include "drape_frontend/map_data_provider.hpp"

#include "indexer/feature_decl.hpp"

#include "coding/files_container.hpp"
#include "coding/varint.hpp"

#include "defines.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace df
{
namespace
{
struct MetalineData
{
  std::vector<FeatureID> m_features;
  std::vector<double> m_dashPhaseOffsets;
  std::set<FeatureID> m_reversed;
};

std::vector<MetalineData> ReadMetalinesFromFile(MwmSet::MwmId const & mwmId)
{
  try
  {
    FilesContainerR cont(mwmId.GetInfo()->GetLocalFile().GetPath(MapFileType::Map));
    ReaderSource src(cont.GetReader(METALINES_FILE_TAG));

    std::vector<MetalineData> model;
    auto const version = ReadPrimitiveFromSource<uint8_t>(src);
    if (version == 1 || version == kMetaLinesSectionVersion)
    {
      for (auto metalineIndex = ReadVarUint<uint32_t>(src); metalineIndex > 0; --metalineIndex)
      {
        MetalineData data;
        for (auto i = ReadVarUint<uint32_t>(src); i > 0; --i)
        {
          auto const fid = ReadVarInt<int32_t>(src);
          FeatureID const featureId(mwmId, static_cast<uint32_t>(std::abs(fid)));
          data.m_features.emplace_back(featureId);
          if (version >= 2)
            data.m_dashPhaseOffsets.push_back(ReadPrimitiveFromSource<float>(src));
          if (fid <= 0)
            data.m_reversed.insert(featureId);
        }
        if (!data.m_features.empty())
          model.push_back(std::move(data));
      }
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

    if (!result.empty())
    {
      auto iBeg = it->second.begin();
      if (result.back().EqualDxDy(*iBeg, kMwmPointAccuracy))
        ++iBeg;
      result.insert(result.end(), iBeg, it->second.end());
    }
    else
      result = std::move(it->second);
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
    std::map<FeatureID, std::pair<double, bool>> phases;
    double chainOffset = 0.0;
    for (size_t i = 0; i < metaline.m_features.size(); ++i)
    {
      auto const & fid = metaline.m_features[i];
      auto const it = points.find(fid);
      if (it == points.end())
        continue;

      bool const reversed = metaline.m_reversed.find(fid) != metaline.m_reversed.end();
      double const length = GetPathLength(it->second);
      double const phaseOffset = metaline.m_dashPhaseOffsets.empty()
                                     ? chainOffset + (reversed ? length : 0.0)
                                     : metaline.m_dashPhaseOffsets[i];
      phases.emplace(fid, std::pair(phaseOffset, reversed));
      chainOffset += length;
    }

    auto mergedPoints = MergePoints(std::move(points), metaline.m_features);
    if (mergedPoints.size() < 2)
      continue;

    m2::SharedSpline const spline(std::move(mergedPoints));
    for (auto & fid : metaline.m_features)
    {
      auto const phase = phases.find(fid);
      if (phase == phases.end())
        continue;
      m_metalines.emplace(std::move(fid), MetalineInfo{spline, phase->second.first, phase->second.second});
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
