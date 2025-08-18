#include "openlr/openlr_match_quality/openlr_assessment_tool/segment_correspondence.hpp"

namespace openlr
{
SegmentCorrespondence::SegmentCorrespondence(SegmentCorrespondence const & sc)
{
  m_partnerSegment = sc.m_partnerSegment;

  m_positiveOffset = sc.m_positiveOffset;
  m_negativeOffset = sc.m_negativeOffset;

  m_matchedPath = sc.m_matchedPath;
  m_fakePath = sc.m_fakePath;
  m_goldenPath = sc.m_goldenPath;

  m_partnerXMLDoc.reset(sc.m_partnerXMLDoc);
  m_partnerXMLSegment = m_partnerXMLDoc.child("reportSegments");

  m_status = sc.m_status;
}

SegmentCorrespondence::SegmentCorrespondence(openlr::LinearSegment const & segment, uint32_t positiveOffset,
                                             uint32_t negativeOffset, openlr::Path const & matchedPath,
                                             openlr::Path const & fakePath, openlr::Path const & goldenPath,
                                             pugi::xml_node const & partnerSegmentXML)
  : m_partnerSegment(segment)
  , m_positiveOffset(positiveOffset)
  , m_negativeOffset(negativeOffset)
  , m_matchedPath(matchedPath)
  , m_fakePath(fakePath)
{
  SetGoldenPath(goldenPath);

  m_partnerXMLDoc.append_copy(partnerSegmentXML);
  m_partnerXMLSegment = m_partnerXMLDoc.child("reportSegments");
  CHECK(m_partnerXMLSegment, ("Node should contain <reportSegments> part"));
}

void SegmentCorrespondence::SetGoldenPath(openlr::Path const & p)
{
  m_goldenPath = p;
  m_status = p.empty() ? Status::Untouched : Status::Assessed;
}

void SegmentCorrespondence::Ignore()
{
  m_status = Status::Ignored;
  m_goldenPath.clear();
}
}  // namespace openlr
