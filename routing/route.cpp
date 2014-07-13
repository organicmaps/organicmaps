#include "route.hpp"


namespace routing
{

string DebugPrint(Route const & r)
{
  return DebugPrint(r.m_poly);
}

} // namespace routing
