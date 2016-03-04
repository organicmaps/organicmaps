#include "search/v2/ranking_info.hpp"

namespace search
{
namespace v2
{
// static
void RankingInfo::PrintCSVHeader(ostream & os)
{
  os << "DistanceToViewport"
     << ",DistanceToPosition"
     << ",Rank"
     << ",NameScore"
     << ",SearchType"
     << ",PositionInViewport";
}

string DebugPrint(RankingInfo const & info)
{
  ostringstream os;
  os << "RankingInfo [";
  os << "m_distanceToViewport:" << info.m_distanceToViewport << ",";
  os << "m_distanceToPosition:" << info.m_distanceToPosition << ",";
  os << "m_rank:" << static_cast<int>(info.m_rank) << ",";
  os << "m_nameScore:" << DebugPrint(info.m_nameScore) << ",";
  os << "m_searchType:" << DebugPrint(info.m_searchType) << ",";
  os << "m_positionInViewport:" << info.m_positionInViewport;
  os << "]";
  return os.str();
}

void RankingInfo::ToCSV(ostream & os) const
{
  os << fixed;
  os << m_distanceToViewport << "," << m_distanceToPosition << "," << static_cast<int>(m_rank)
     << "," << DebugPrint(m_nameScore) << "," << DebugPrint(m_searchType) << ","
     << m_positionInViewport;
}
}  // namespace v2
}  // namespace search
