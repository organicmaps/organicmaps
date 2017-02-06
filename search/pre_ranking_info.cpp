#include "search/ranking_info.hpp"

#include "std/sstream.hpp"

namespace search
{
string DebugPrint(PreRankingInfo const & info)
{
  ostringstream os;
  os << "PreRankingInfo [";
  os << "m_distanceToPivot:" << info.m_distanceToPivot << ",";
  for (size_t i = 0; i < static_cast<size_t>(SearchModel::SEARCH_TYPE_COUNT); ++i)
  {
    if (info.m_tokenRange[i].Empty())
      continue;

    auto const type = static_cast<SearchModel::SearchType>(i);
    os << "m_tokenRange[" << DebugPrint(type) << "]:" << DebugPrint(info.m_tokenRange[i]) << ",";
  }
  os << "m_rank:" << info.m_rank << ",";
  os << "m_searchType:" << info.m_searchType;
  os << "]";
  return os.str();
}

}  // namespace search
