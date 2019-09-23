#include "partners_api/opentable_api.hpp"

#include <sstream>

#include "private.h"

namespace
{
  auto const kOpentableBaseUrl = "http://www.opentable.com/restaurant/profile/";
}   // namespace

namespace opentable
{
// static
std::string Api::GetBookTableUrl(std::string const & restaurantId)
{
  std::stringstream ss;
  ss << kOpentableBaseUrl << restaurantId << "?ref=" << OPENTABLE_AFFILATE_ID;
  return ss.str();
}
}  // namespace opentable
