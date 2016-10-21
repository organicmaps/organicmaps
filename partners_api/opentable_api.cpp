#include "partners_api/opentable_api.hpp"

#include "std/sstream.hpp"

#include "private.h"

namespace
{
  auto const kOpentableBaseUrl = "http://www.opentable.com/restaurant/profile/";
}   // namespace

namespace opentable
{
// static
string Api::GetBookTableUrl(string const & restaurantId)
{
  stringstream ss;
  ss << kOpentableBaseUrl << restaurantId << "?ref=" << OPENTABLE_AFFILATE_ID;
  return ss.str();
}
}  // namespace opentable
