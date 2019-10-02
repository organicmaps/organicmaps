#include "drape_frontend/read_metaline_task.hpp"

#include "drape_frontend/drape_api.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/threads_commutator.hpp"

#include "coding/files_container.hpp"
#include "coding/reader_wrapper.hpp"
#include "coding/varint.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/scales.hpp"

#include "defines.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <vector>

namespace
{
double const kPointEqualityEps = 1e-7;

struct MetalineData
{
  std::vector<FeatureID> m_features;
  std::set<FeatureID> m_reversed;
};

std::vector<MetalineData> ReadMetalinesFromFile(MwmSet::MwmId const & mwmId)
{ 
  try
  {
    std::vector<MetalineData> model;
    ModelReaderPtr reader = FilesContainerR(mwmId.GetInfo()->GetLocalFile().GetPath(MapFileType::Map))
                                            .GetReader(METALINES_FILE_TAG);
    ReaderSrc src(reader.GetPtr());
    auto const version = ReadPrimitiveFromSource<uint8_t>(src);
    if (version == 1)
    {
      for (auto metalineIndex = ReadVarUint<uint32_t>(src); metalineIndex > 0; --metalineIndex)
      {
        MetalineData data {};
        for (auto i = ReadVarUint<uint32_t>(src); i > 0; --i)
        {
          auto const fid = ReadVarInt<int32_t>(src);
          FeatureID const featureId(mwmId, static_cast<uint32_t>(std::abs(fid)));
          data.m_features.emplace_back(featureId);
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

std::map<FeatureID, std::vector<m2::PointD>> ReadPoints(df::MapDataProvider & model,
                                                        MetalineData const & metaline)
{
  bool failed = false;
  auto features = metaline.m_features;
  std::sort(features.begin(), features.end());

  std::map<FeatureID, std::vector<m2::PointD>> result;
  model.ReadFeatures([&metaline, &failed, &result](FeatureType & ft)
  {
    if (failed)
      return;

    std::vector<m2::PointD> featurePoints;
    featurePoints.reserve(5);
    ft.ForEachPoint([&featurePoints](m2::PointD const & pt)
    {
      if (featurePoints.empty() || !featurePoints.back().EqualDxDy(pt, kPointEqualityEps))
        featurePoints.push_back(pt);
    }, scales::GetUpperScale());

    if (featurePoints.size() < 2)
    {
      failed = true;
      return;
    }
    if (metaline.m_reversed.find(ft.GetID()) != metaline.m_reversed.cend())
      std::reverse(featurePoints.begin(), featurePoints.end());

    result.insert(std::make_pair(ft.GetID(), std::move(featurePoints)));
  }, features);

  if (failed)
    return {};

  return result;
}

std::vector<m2::PointD> MergePoints(std::map<FeatureID, std::vector<m2::PointD>> const & points,
                                    std::vector<FeatureID> const & featuresOrder)
{
  size_t sz = 0;
  for (auto const & p : points)
    sz += p.second.size();

  std::vector<m2::PointD> result;
  result.reserve(sz);
  for (auto const & f : featuresOrder)
  {
    auto const it = points.find(f);
    ASSERT(it != points.cend(), ());
    for (auto const & pt : it->second)
    {
      if (result.empty() || !result.back().EqualDxDy(pt, kPointEqualityEps))
        result.push_back(pt);
    }
  }
  return result;
}
}  // namespace

namespace df
{
ReadMetalineTask::ReadMetalineTask(MapDataProvider & model, MwmSet::MwmId const & mwmId)
  : m_model(model)
  , m_mwmId(mwmId)
  , m_isCancelled(false)
{}

void ReadMetalineTask::Cancel()
{
  m_isCancelled = true;
}

bool ReadMetalineTask::IsCancelled() const
{
  return m_isCancelled;
}

void ReadMetalineTask::Run()
{
  if (m_isCancelled)
    return;

  if (!m_mwmId.IsAlive())
    return;

  if (m_mwmId.GetInfo()->GetType() != MwmInfo::MwmTypeT::COUNTRY)
    return;

  auto metalines = ReadMetalinesFromFile(m_mwmId);
  for (auto const & metaline : metalines)
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

    auto const points = ReadPoints(m_model, metaline);
    if (points.size() != metaline.m_features.size())
      continue;

    std::vector<m2::PointD> const mergedPoints = MergePoints(points, metaline.m_features);
    if (mergedPoints.empty())
      continue;

    m2::SharedSpline const spline(mergedPoints);
    for (auto const & fid : metaline.m_features)
      m_metalines[fid] = spline;
  }
}
}  // namespace df
