#include "routing_common/transit_header.hpp"

#include <sstream>

namespace routing
{
namespace transit
{
using namespace std;

TransitHeader::TransitHeader(uint16_t version, uint32_t gatesOffset, uint32_t edgesOffset,
                             uint32_t transfersOffset, uint32_t linesOffset, uint32_t shapesOffset,
                             uint32_t networksOffset, uint32_t endOffset)
  : m_version(version)
  , m_reserve(0)
  , m_gatesOffset(gatesOffset)
  , m_edgesOffset(edgesOffset)
  , m_transfersOffset(transfersOffset)
  , m_linesOffset(linesOffset)
  , m_shapesOffset(shapesOffset)
  , m_networksOffset(networksOffset)
  , m_endOffset(endOffset)
{
}

void TransitHeader::Reset()
{
  m_version = 0;
  m_reserve = 0;
  m_gatesOffset = 0;
  m_edgesOffset = 0;
  m_transfersOffset = 0;
  m_linesOffset = 0;
  m_shapesOffset = 0;
  m_networksOffset = 0;
  m_endOffset = 0;
}

bool TransitHeader::IsEqualForTesting(TransitHeader const & header) const
{
  return m_version == header.m_version && m_reserve == header.m_reserve &&
         m_gatesOffset == header.m_gatesOffset && m_edgesOffset == header.m_edgesOffset &&
         m_transfersOffset == header.m_transfersOffset && m_linesOffset == header.m_linesOffset &&
         m_shapesOffset == header.m_shapesOffset && m_networksOffset == header.m_networksOffset &&
         m_endOffset == header.m_endOffset;
}

string DebugPrint(TransitHeader const & header)
{
  stringstream out;
  out << "TransitHeader [ m_version = " << header.m_version
      << ", m_reserve = " << header.m_reserve
      << ", m_gatesOffset = " << header.m_gatesOffset
      << ", m_edgesOffset = " << header.m_edgesOffset
      << ", m_transfersOffset = " << header.m_transfersOffset
      << ", m_linesOffset = " << header.m_linesOffset
      << ", m_shapesOffset = " << header.m_shapesOffset
      << ", m_networksOffset = " << header.m_networksOffset
      << ", m_endOffset = " << header.m_endOffset
      << " ]" << endl;
  return out.str();
}
}  // namespace transit
}  // namespace routing
