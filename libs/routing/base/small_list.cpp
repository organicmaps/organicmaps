#include "small_list.hpp"

#include "base/logging.hpp"

namespace routing
{
namespace impl
{

std::map<char const *, Statistics::Info> Statistics::s_map;

void Statistics::Add(char const * name, size_t val)
{
  auto & e = s_map[name];
  ++e.m_count;
  e.m_sum += val;
  e.m_max = std::max(val, e.m_max);
}

void Statistics::Dump()
{
  for (auto const & e : s_map)
    LOG(LINFO, (e.first, ": count = ", e.second.m_count, "; max = ", e.second.m_max,
                "; avg = ", e.second.m_sum / double(e.second.m_count)));
}

}  // namespace impl
}  // namespace routing
