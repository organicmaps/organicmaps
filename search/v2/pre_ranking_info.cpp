#include "search/v2/ranking_info.hpp"

#include "std/sstream.hpp"

namespace search
{
namespace v2
{
string DebugPrint(PreRankingInfo const & info)
{
  ostringstream os;
  os << "PreRankingInfo [";
  os << "m_distanceToPivot:" << info.m_distanceToPivot << ",";
  os << "m_startToken:" << info.m_startToken << ",";
  os << "m_endToken:" << info.m_endToken << ",";
  os << "m_rank:" << info.m_rank << ",";
  os << "m_searchType:" << info.m_searchType;
  os << "]";
  return os.str();
}
}  // namespace v2
}  // namespace search
