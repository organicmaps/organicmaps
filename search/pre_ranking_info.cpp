#include "search/pre_ranking_info.hpp"

#include <sstream>

namespace search
{
std::string DebugPrint(PreRankingInfo const & info)
{
  std::ostringstream os;
  os << "PreRankingInfo [";
  os << "m_distanceToPivot:" << info.m_distanceToPivot << ",";
  for (size_t i = 0; i < static_cast<size_t>(Model::TYPE_COUNT); ++i)
  {
    if (info.m_tokenRange[i].Empty())
      continue;

    auto const type = static_cast<Model::Type>(i);
    os << "m_tokenRange[" << DebugPrint(type) << "]:" << DebugPrint(info.m_tokenRange[i]) << ",";
  }
  os << "m_rank:" << static_cast<int>(info.m_rank) << ",";
  os << "m_popularity:" << static_cast<int>(info.m_popularity) << ",";
  os << "m_type:" << info.m_type;
  os << "]";
  return os.str();
}
}  // namespace search
