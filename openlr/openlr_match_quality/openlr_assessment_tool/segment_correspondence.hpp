#pragma once

#include "openlr/decoded_path.hpp"
#include "openlr/openlr_model.hpp"

#include <pugixml.hpp>

namespace openlr
{
class SegmentCorrespondence
{
public:
  enum class Status
  {
    Untouched,
    Assessed,
    Ignored
  };

  SegmentCorrespondence(SegmentCorrespondence const & sc);
  SegmentCorrespondence(openlr::LinearSegment const & segment, uint32_t positiveOffset, uint32_t negativeOffset,
                        openlr::Path const & matchedPath, openlr::Path const & fakePath,
                        openlr::Path const & goldenPath, pugi::xml_node const & partnerSegmentXML);

  openlr::Path const & GetMatchedPath() const { return m_matchedPath; }
  bool HasMatchedPath() const { return !m_matchedPath.empty(); }

  uint32_t GetPositiveOffset() const { return m_positiveOffset; }
  uint32_t GetNegativeOffset() const { return m_negativeOffset; }

  openlr::Path const & GetFakePath() const { return m_fakePath; }
  bool HasFakePath() const { return !m_fakePath.empty(); }

  openlr::Path const & GetGoldenPath() const { return m_goldenPath; }
  bool HasGoldenPath() const { return !m_goldenPath.empty(); }
  void SetGoldenPath(openlr::Path const & p);

  openlr::LinearSegment const & GetPartnerSegment() const { return m_partnerSegment; }

  uint32_t GetPartnerSegmentId() const { return m_partnerSegment.m_segmentId; }

  pugi::xml_document const & GetPartnerXML() const { return m_partnerXMLDoc; }
  pugi::xml_node const & GetPartnerXMLSegment() const { return m_partnerXMLSegment; }

  Status GetStatus() const { return m_status; }

  void Ignore();

private:
  openlr::LinearSegment m_partnerSegment;

  uint32_t m_positiveOffset = 0;
  uint32_t m_negativeOffset = 0;

  openlr::Path m_matchedPath;
  openlr::Path m_fakePath;
  openlr::Path m_goldenPath;

  // A dirty hack to save back SegmentCorrespondence.
  // TODO(mgsergio): Consider unifying xml serialization with one used in openlr_stat.
  pugi::xml_document m_partnerXMLDoc;
  // This is used by GetPartnerXMLSegment shortcut to return const ref. pugi::xml_node is
  // just a wrapper so returning by value won't guarantee constness.
  pugi::xml_node m_partnerXMLSegment;

  Status m_status = Status::Untouched;
};
}  // namespace openlr
