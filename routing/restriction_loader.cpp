#include "routing/restriction_loader.hpp"
#include "routing/restrictions_serialization.hpp"
#include "routing/road_index.hpp"

#include "base/stl_helpers.hpp"

namespace
{
using namespace routing;

/// \returns if features |r1| and |r2| have a common end returns its joint id.
/// If not, returnfs Joint::kInvalidId.
/// \note It's possible that the both ends of |r1| and |r2| has common joint ids.
/// In that case returns any of them.
/// \note In general case ends of features don't have to be joints. For example all
/// loose feature ends aren't joints. But if ends of r1 and r2 are connected at this
/// point there has to be a joint. So the method is valid.
Joint::Id GetCommonEndJoint(routing::RoadJointIds const & r1, routing::RoadJointIds const & r2)
{
  auto const IsEqual = [](Joint::Id j1, Joint::Id j2) {
    return j1 != Joint::kInvalidId && j1 == j2;
  };
  if (IsEqual(r1.GetJointId(0 /* point id */), r2.GetLastJointId()))
    return r1.GetJointId(0);

  if (IsEqual(r1.GetJointId(0 /* point id */), r2.GetJointId(0)))
    return r1.GetJointId(0);

  if (IsEqual(r2.GetJointId(0 /* point id */), r1.GetLastJointId()))
    return r2.GetJointId(0);

  Joint::Id const r1Last = r1.GetLastJointId();
  if (IsEqual(r1Last, r2.GetLastJointId()))
    return r1Last;

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
    m_reader = make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(RESTRICTIONS_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    RestrictionVec restrictionsOnly;
    RestrictionSerializer::Deserialize(m_header, m_restrictions /* restriction no */, restrictionsOnly, src);
    ConvertRestrictionOnlyToNo(graph, restrictionsOnly, m_restrictions);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR,
        ("File", m_countryFileName, "Error while reading", RESTRICTIONS_FILE_TAG, "section.", e.Msg()));
  }
}

void ConvertRestrictionOnlyToNo(IndexGraph const & graph, RestrictionVec const & restrictionsOnly,
                                RestrictionVec & restrictionsNo)
{
  if (!graph.IsValid())
  {
    LOG(LWARNING, ("Index graph is not valid. All the restrictions of type Only aren't used."));
    return;
  }

  for (Restriction const & o : restrictionsOnly)
  {
    CHECK_EQUAL(o.m_featureIds.size(), 2, ("Only two link restrictions are support."));
    if (!graph.IsRoad(o.m_featureIds[0]) || !graph.IsRoad(o.m_featureIds[1]))
      continue;

    // Looking for a joint of an intersection of |o| features.
    Joint::Id const common =
        GetCommonEndJoint(graph.GetRoad(o.m_featureIds[0]), graph.GetRoad(o.m_featureIds[1]));
    if (common == Joint::kInvalidId)
      continue;

    // Adding restriction of type No for all features of joint |common| except for
    // the second feature of restriction |o|.
    graph.ForEachPoint(common, [&] (RoadPoint const & rp){
      if (rp.GetFeatureId() != o.m_featureIds[1 /* to */])
      {
        restrictionsNo.push_back(
              Restriction(Restriction::Type::No, {o.m_featureIds[0 /* from */], rp.GetFeatureId()}));
      }
    });
  }
  my::SortUnique(restrictionsNo);
}
}  // namespace routing
