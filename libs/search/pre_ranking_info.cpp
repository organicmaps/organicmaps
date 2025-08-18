#include "search/pre_ranking_info.hpp"

#include <sstream>

namespace search
{
std::string DebugPrint(PreRankingInfo const & info)
{
  std::ostringstream os;
  os << std::boolalpha << "PreRankingInfo "
     << "{ m_distanceToPivot: " << info.m_distanceToPivot << ", m_tokenRanges [ ";
  for (size_t i = 0; i < Model::TYPE_COUNT; ++i)
  {
    if (info.m_tokenRanges[i].Empty())
      continue;

    auto const type = static_cast<Model::Type>(i);
    os << DebugPrint(type) << " : " << DebugPrint(info.m_tokenRanges[i]) << ", ";
  }
  os << " ]"
     << ", m_rank: " << static_cast<int>(info.m_rank) << ", m_popularity: " << static_cast<int>(info.m_popularity)
     << ", m_type: " << DebugPrint(info.m_type) << ", m_allTokensUsed: " << info.m_allTokensUsed
     << ", m_exactMatch: " << info.m_exactMatch << ", m_isCommonMatchOnly: " << info.m_isCommonMatchOnly << " }";

  return os.str();
}
}  // namespace search
