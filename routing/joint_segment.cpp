#include "routing/joint_segment.hpp"

#include "routing/fake_feature_ids.hpp"

#include "base/assert.hpp"

#include <sstream>

namespace routing
{
bool IsRealSegmentSimple(Segment const & segment)
{
  return segment.GetFeatureId() != FakeFeatureIds::kIndexGraphStarterId;
}

JointSegment::JointSegment(Segment const & from, Segment const & to)
{
  // Can not check segment for fake or not with segment.IsRealSegment(), because all segments
  // have got fake m_numMwmId during mwm generation.
  CHECK(IsRealSegmentSimple(from) && IsRealSegmentSimple(to),
        ("Segments of joints can not be fake. Only through ToFake() method."));

  CHECK_EQUAL(from.GetMwmId(), to.GetMwmId(), ("Different mwmIds in segments for JointSegment"));
  m_numMwmId = from.GetMwmId();

  CHECK_EQUAL(from.IsForward(), to.IsForward(), ("Different forward in segments for JointSegment"));
  m_forward = from.IsForward();

  CHECK_EQUAL(from.GetFeatureId(), to.GetFeatureId(), ());
  m_featureId = from.GetFeatureId();

  m_startSegmentId = from.GetSegmentIdx();
  m_endSegmentId = to.GetSegmentIdx();
}

void JointSegment::ToFake(uint32_t fakeId)
{
  m_featureId = kInvalidId;
  m_startSegmentId = fakeId;
  m_endSegmentId = fakeId;
  m_numMwmId = kFakeNumMwmId;
  m_forward = false;
}

bool JointSegment::IsFake() const
{
  // This is enough, but let's check in Debug for confidence.
  bool result = m_featureId == kInvalidId && m_startSegmentId != kInvalidId;
  if (result)
  {
    ASSERT_EQUAL(m_startSegmentId, m_endSegmentId, ());
    ASSERT_EQUAL(m_numMwmId, kFakeNumMwmId, ());
    ASSERT_EQUAL(m_forward, false, ());
  }

  return result;
}

bool JointSegment::operator<(JointSegment const & rhs) const
{
  if (m_featureId != rhs.GetFeatureId())
    return m_featureId < rhs.GetFeatureId();

  if (m_forward != rhs.IsForward())
    return m_forward < rhs.IsForward();

  if (m_startSegmentId != rhs.m_startSegmentId)
    return m_startSegmentId < rhs.m_startSegmentId;

  if (m_endSegmentId != rhs.m_endSegmentId)
    return m_endSegmentId < rhs.m_endSegmentId;

  return m_numMwmId < rhs.GetMwmId();
}

bool JointSegment::operator==(JointSegment const & rhs) const
{
  return m_featureId == rhs.m_featureId && m_forward == rhs.m_forward &&
         m_startSegmentId == rhs.m_startSegmentId && m_endSegmentId == rhs.m_endSegmentId &&
         m_numMwmId == rhs.m_numMwmId;
}

std::string DebugPrint(JointSegment const & jointSegment)
{
  std::ostringstream out;
  if (jointSegment.IsFake())
    out << "[FAKE]";

  out << std::boolalpha
      << "JointSegment(" << jointSegment.GetMwmId() << ", " << jointSegment.GetFeatureId() << ", "
      << "[" << jointSegment.GetStartSegmentId() << " => " << jointSegment.GetEndSegmentId() << "], "
      << jointSegment.IsForward() << ")";
  return out.str();
}
}  // namespace routing
