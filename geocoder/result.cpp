#include "geocoder/result.hpp"

#include <sstream>

using namespace std;

namespace geocoder
{
string DebugPrint(Result const & result)
{
  ostringstream oss;
  oss << DebugPrint(result.m_osmId) << "  certainty=" << result.m_certainty;
  return oss.str();
}
}  // namespace geocoder
