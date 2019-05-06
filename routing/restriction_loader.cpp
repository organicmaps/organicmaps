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
RestrictionLoader::RestrictionLoader(MwmValue const & mwmValue, IndexGraph const & graph)
  : m_countryFileName(mwmValue.GetCountryFileName())
{
  if (!mwmValue.m_cont.IsExist(RESTRICTIONS_FILE_TAG))
    return;

  try
  {
    m_reader =
        std::make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(RESTRICTIONS_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    RestrictionVec restrictionsOnly;
    RestrictionSerializer::Deserialize(m_header, m_restrictions /* restriction No */,
                                       restrictionsOnly, src);

    ConvertRestrictionsOnlyToNoAndSort(graph, restrictionsOnly, m_restrictions);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR,
        ("File", m_countryFileName, "Error while reading", RESTRICTIONS_FILE_TAG, "section.", e.Msg()));
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

/// \brief We store |Only| restriction in mwm, like
/// Only, featureId_1, ... , featureId_N
/// We convert such restriction to |No| following way:
/// For each pair of neighbouring features (featureId_1 and featureId_2 for ex.)
/// we forbid to go from featureId_1 to wherever except featureId_2.
/// Then do this for featureId_2 and featureId_3 and so on.
void ConvertRestrictionsOnlyToNoAndSort(IndexGraph const & graph,
                                        RestrictionVec const & restrictionsOnly,
                                        RestrictionVec & restrictionsNo)
{
  for (auto const & restriction : restrictionsOnly)
  {
    if (std::any_of(restriction.begin(), restriction.end(),
                    [&graph](auto const & item)
                    {
                      return !graph.IsRoad(item);
                    }))
    {
      continue;
    }

    CHECK_GREATER_OR_EQUAL(restriction.size(), 2, ());
    // vector of simple no_restriction - from|to.
    std::vector<std::pair<uint32_t, uint32_t>> toAdd;
    bool error = false;
    for (size_t i = 1; i < restriction.size(); ++i)
    {
      auto const curFeatureId = restriction[i];
      auto const prevFeatureId = restriction[i - 1];

      // Looking for a joint of an intersection of prev and cur features.
      Joint::Id const common =
          GetCommonEndJoint(graph.GetRoad(curFeatureId), graph.GetRoad(prevFeatureId));

      if (common == Joint::kInvalidId)
      {
        error = true;
        break;
      }

      // Adding restriction of type No for all features of joint |common| except for
      // |prevFeatureId| -> |curFeatureId|.
      graph.ForEachPoint(common,
                         [&](RoadPoint const & rp)
                         {
                           if (rp.GetFeatureId() != curFeatureId)
                             toAdd.emplace_back(prevFeatureId, rp.GetFeatureId());
                         });
    }

    if (error)
      continue;

    for (auto const & fromToPair : toAdd)
    {
      std::vector<uint32_t> newRestriction =
          {fromToPair.first /* from */, fromToPair.second /* to */};

      restrictionsNo.emplace_back(std::move(newRestriction));
    }
  }

  base::SortUnique(restrictionsNo);
}
}  // namespace routing
