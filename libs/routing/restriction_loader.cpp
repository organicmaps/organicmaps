#include "routing/restriction_loader.hpp"

#include "routing/restrictions_serialization.hpp"
#include "routing/road_index.hpp"

#include "indexer/mwm_set.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

namespace
{
using namespace routing;
/// \returns if features |r1| and |r2| have a common end returns its joint id.
/// If not, returns Joint::kInvalidId.
/// \note It's possible that the both ends of |r1| and |r2| have common joint ids.
/// In that case returns any of them.
/// \note In general case ends of features don't have to be joints. For example all
/// loose feature ends aren't joints. But if ends of r1 and r2 are connected at this
/// point there has to be a joint. So the method is valid.
Joint::Id GetCommonEndJoint(RoadJointIds const & r1, RoadJointIds const & r2)
{
  auto const & j11 = r1.GetJointId(0 /* point id */);
  auto const & j12 = r1.GetEndingJointId();
  auto const & j21 = r2.GetJointId(0 /* point id */);
  auto const & j22 = r2.GetEndingJointId();

  if (j11 == j21 || j11 == j22)
    return j11;

  if (j12 == j21 || j12 == j22)
    return j12;

  return Joint::kInvalidId;
}
}  // namespace

namespace routing
{
RestrictionLoader::RestrictionLoader(MwmValue const & mwmValue, IndexGraph & graph)
  : m_countryFileName(mwmValue.GetCountryFileName())
{
  if (!mwmValue.m_cont.IsExist(RESTRICTIONS_FILE_TAG))
    return;

  try
  {
    m_reader = std::make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(RESTRICTIONS_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    RestrictionVec restrictionsOnly;
    std::vector<RestrictionUTurn> restrictionsOnlyUTurn;
    RestrictionSerializer::Deserialize(m_header, m_restrictions /* restriction No, without no_u_turn */,
                                       restrictionsOnly, m_noUTurnRestrictions, restrictionsOnlyUTurn, src);

    ConvertRestrictionsOnlyToNo(graph, restrictionsOnly, m_restrictions);
    ConvertRestrictionsOnlyUTurnToNo(graph, restrictionsOnlyUTurn, m_restrictions);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR, ("File", m_countryFileName, "Error while reading", RESTRICTIONS_FILE_TAG, "section.", e.Msg()));
    throw;
  }
}

bool RestrictionLoader::HasRestrictions() const
{
  return !m_restrictions.empty();
}

RestrictionVec && RestrictionLoader::StealRestrictions()
{
  return std::move(m_restrictions);
}

std::vector<RestrictionUTurn> && RestrictionLoader::StealNoUTurnRestrictions()
{
  return std::move(m_noUTurnRestrictions);
}

bool IsRestrictionFromRoads(IndexGraph const & graph, std::vector<uint32_t> const & restriction)
{
  for (auto const & featureId : restriction)
    if (!graph.IsRoad(featureId))
      return false;

  return true;
}

/// \brief We store |Only| restriction in mwm, like
/// Only, featureId_1, ... , featureId_N
///
/// We convert such restriction to |No| following way:
/// For each M, greater than 1, and M first features:
/// featureId_1, ... , featureId_(M - 1), featureId_M, ...
///
/// We create restrictionNo from features:
/// featureId_1, ... , featureId_(M - 1), featureId_K
/// where featureId_K - has common joint with featureId_(M - 1) and featureId_M.
void ConvertRestrictionsOnlyToNo(IndexGraph const & graph, RestrictionVec const & restrictionsOnly,
                                 RestrictionVec & restrictionsNo)
{
  for (std::vector<uint32_t> const & restriction : restrictionsOnly)
  {
    CHECK_GREATER_OR_EQUAL(restriction.size(), 2, ());

    if (!IsRestrictionFromRoads(graph, restriction))
      continue;

    for (size_t i = 1; i < restriction.size(); ++i)
    {
      auto const curFeatureId = restriction[i];
      auto const prevFeatureId = restriction[i - 1];

      // Looking for a joint of an intersection of prev and cur features.
      Joint::Id const common = GetCommonEndJoint(graph.GetRoad(curFeatureId), graph.GetRoad(prevFeatureId));

      if (common == Joint::kInvalidId)
        break;

      std::vector<uint32_t> commonFeatures;
      commonFeatures.resize(i + 1);
      std::copy(restriction.begin(), restriction.begin() + i, commonFeatures.begin());

      graph.ForEachPoint(common, [&](RoadPoint const & rp)
      {
        if (rp.GetFeatureId() != curFeatureId)
        {
          commonFeatures.back() = rp.GetFeatureId();
          restrictionsNo.emplace_back(commonFeatures);
        }
      });
    }
  }
}

void ConvertRestrictionsOnlyUTurnToNo(IndexGraph & graph, std::vector<RestrictionUTurn> const & restrictionsOnlyUTurn,
                                      RestrictionVec & restrictionsNo)
{
  for (auto const & uTurnRestriction : restrictionsOnlyUTurn)
  {
    auto const featureId = uTurnRestriction.m_featureId;
    if (!graph.IsRoad(featureId))
      continue;

    uint32_t const n = graph.GetRoadGeometry(featureId).GetPointsCount();
    RoadJointIds const & joints = graph.GetRoad(uTurnRestriction.m_featureId);
    Joint::Id const joint = uTurnRestriction.m_viaIsFirstPoint ? joints.GetJointId(0) : joints.GetJointId(n - 1);

    if (joint == Joint::kInvalidId)
      continue;

    graph.ForEachPoint(joint, [&](RoadPoint const & rp)
    {
      if (rp.GetFeatureId() != featureId)
      {
        std::vector<uint32_t> fromToRestriction = {featureId, rp.GetFeatureId()};
        restrictionsNo.emplace_back(std::move(fromToRestriction));
      }
    });
  }
}
}  // namespace routing
