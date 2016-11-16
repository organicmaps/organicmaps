#include "index_graph.hpp"

#include "base/assert.hpp"

namespace routing
{
IndexGraph::IndexGraph(unique_ptr<Geometry> geometry) : m_geometry(move(geometry)) {}

void IndexGraph::GetOutgoingEdgesList(JointId jointId, vector<TEdgeType> & edges) const
{
  GetEdgesList(jointId, edges, true);
}

void IndexGraph::GetIngoingEdgesList(JointId jointId, vector<TEdgeType> & edges) const
{
  GetEdgesList(jointId, edges, false);
}

double IndexGraph::HeuristicCostEstimate(JointId jointFrom, JointId jointTo) const
{
  return m_geometry->CalcHeuristic(GetFSeg(jointFrom), GetFSeg(jointTo));
}

m2::PointD const & IndexGraph::GetPoint(JointId jointId) const
{
  return m_geometry->GetPoint(GetFSeg(jointId));
}

void IndexGraph::Export(vector<Joint> const & joints)
{
  m_fsegIndex.Export(joints);
  BuildJoints(joints.size());
}

JointId IndexGraph::InsertJoint(FSegId fseg)
{
  JointId const existId = m_fsegIndex.GetJointId(fseg);
  if (existId != kInvalidJointId)
    return existId;

  JointId const jointId = m_jointOffsets.size();
  m_jointOffsets.emplace_back(JointOffset(m_fsegs.size(), m_fsegs.size() + 1));
  m_fsegs.emplace_back(fseg);
  m_fsegIndex.AddJoint(fseg, jointId);
  return jointId;
}

vector<FSegId> IndexGraph::RedressRoute(vector<JointId> const & route) const
{
  vector<FSegId> fsegs;
  if (route.size() < 2)
  {
    if (route.size() == 1)
      fsegs.emplace_back(GetFSeg(route[0]));
    return fsegs;
  }

  fsegs.reserve(route.size() * 2);

  for (size_t i = 0; i < route.size() - 1; ++i)
  {
    JointId const prevJoint = route[i];
    JointId const nextJoint = route[i + 1];

    auto const & pair = FindSharedFeature(prevJoint, nextJoint);
    if (!pair.first.IsValid())
    {
      fsegs.clear();
      return fsegs;
    }

    if (i == 0)
      fsegs.push_back(pair.first);

    uint32_t const featureId = pair.first.GetFeatureId();
    uint32_t const segFrom = pair.first.GetSegId();
    uint32_t const segTo = pair.second.GetSegId();

    ASSERT_NOT_EQUAL(segFrom, segTo, ());

    if (segFrom < segTo)
    {
      for (uint32_t seg = segFrom + 1; seg < segTo; ++seg)
        fsegs.push_back({featureId, seg});
    }
    else
    {
      for (uint32_t seg = segFrom - 1; seg > segTo; --seg)
        fsegs.push_back({featureId, seg});
    }

    fsegs.push_back(pair.second);
  }

  return fsegs;
}

FSegId IndexGraph::GetFSeg(JointId jointId) const
{
  return m_fsegs[GetJointOffset(jointId).Begin()];
}

inline JointOffset const & IndexGraph::GetJointOffset(JointId jointId) const
{
  ASSERT_LESS(jointId, m_jointOffsets.size(), ("JointId out of bounds"));
  return m_jointOffsets[jointId];
}

pair<FSegId, FSegId> IndexGraph::FindSharedFeature(JointId jointId0, JointId jointId1) const
{
  JointOffset const & offset0 = GetJointOffset(jointId0);
  JointOffset const & offset1 = GetJointOffset(jointId1);

  for (size_t i = offset0.Begin(); i < offset0.End(); ++i)
  {
    FSegId const & fseg0 = m_fsegs[i];
    for (size_t j = offset1.Begin(); j < offset1.End(); ++j)
    {
      FSegId const & fseg1 = m_fsegs[j];
      if (fseg0.GetFeatureId() == fseg1.GetFeatureId())
        return make_pair(fseg0, fseg1);
    }
  }

  LOG(LERROR, ("Can't find shared feature for joints", jointId0, jointId1));
  return make_pair(FSegId::MakeInvalid(), FSegId::MakeInvalid());
}

void IndexGraph::BuildJoints(uint32_t jointsAmount)
{
  // +2 is reserved space for start and finish
  m_jointOffsets.reserve(jointsAmount + 2);
  m_jointOffsets.resize(jointsAmount, {0, 0});

  m_fsegIndex.ForEachRoad([this](uint32_t featureId, RoadJointIds const & road) {
    road.ForEachJoint([this](uint32_t segId, uint32_t jointId) {
      ASSERT_LESS(jointId, m_jointOffsets.size(), ());
      m_jointOffsets[jointId].IncSize();
    });
  });

  uint32_t offset = 0;
  for (size_t i = 0; i < m_jointOffsets.size(); ++i)
  {
    JointOffset & jointOffset = m_jointOffsets[i];
    uint32_t const size = jointOffset.Size();
    ASSERT_GREATER(size, 0, ());

    jointOffset.Assign(offset);
    offset += size;
  }

  // +2 is reserved space for start and finish
  m_fsegs.reserve(offset + 2);
  m_fsegs.resize(offset);

  m_fsegIndex.ForEachRoad([this](uint32_t featureId, RoadJointIds const & road) {
    road.ForEachJoint([this, featureId](uint32_t segId, uint32_t jointId) {
      ASSERT_LESS(jointId, m_jointOffsets.size(), ());
      JointOffset & jointOffset = m_jointOffsets[jointId];
      m_fsegs[jointOffset.End()] = {featureId, segId};
      jointOffset.IncSize();
    });
  });
}

void IndexGraph::AddNeigborEdge(vector<TEdgeType> & edges, FSegId fseg, bool forward) const
{
  pair<JointId, uint32_t> const & pair = m_fsegIndex.FindNeigbor(fseg, forward);
  if (pair.first != kInvalidJointId)
  {
    double const distance =
        m_geometry->CalcEdgesWeight(fseg.GetFeatureId(), fseg.GetSegId(), pair.second);
    edges.push_back({pair.first, distance});
  }
}

void IndexGraph::GetEdgesList(JointId jointId, vector<TEdgeType> & edges, bool forward) const
{
  edges.clear();

  JointOffset const & offset = GetJointOffset(jointId);
  for (size_t i = offset.Begin(); i < offset.End(); ++i)
  {
    FSegId const & fseg = m_fsegs[i];
    if (!m_geometry->IsRoad(fseg.GetFeatureId()))
      continue;

    bool const twoWay = !m_geometry->IsOneWay(fseg.GetFeatureId());
    if (!forward || twoWay)
      AddNeigborEdge(edges, fseg, false);

    if (forward || twoWay)
      AddNeigborEdge(edges, fseg, true);
  }
}
}  // namespace routing
