#include "partners_api/booking_http.hpp"

#include "platform/http_client.hpp"

#include "base/string_utils.hpp"

#include <ctime>
#include <iomanip>

#include "private.h"

using namespace base::url;
using namespace platform;
using namespace std;
using namespace std::chrono;

namespace booking
{
namespace http
{
bool RunSimpleHttpRequest(bool const needAuth, string const & url, string & result)
{
  HttpClient request(url);

  if (needAuth)
    request.SetUserAndPassword(BOOKING_KEY, BOOKING_SECRET);

  return request.RunHttpRequest(result);
}

string FormatTime(Time p)
{
  time_t t = duration_cast<seconds>(p.time_since_epoch()).count();
  ostringstream os;
  os << put_time(gmtime(&t), "%Y-%m-%d");
  return os.str();
}

Params AvailabilityParams::Get() const
{
  Params result;

  result.push_back({"hotel_ids", strings::JoinStrings(m_hotelIds, ',')});
  result.push_back({"checkin", FormatTime(m_checkin)});
  result.push_back({"checkout", FormatTime(m_checkout)});

  for (size_t i = 0; i < m_rooms.size(); ++i)
    result.push_back({"room" + to_string(i + 1), m_rooms[i]});

  if (m_minReviewScore != 0.0)
    result.push_back({"min_review_score", to_string(m_minReviewScore)});

  if (!m_stars.empty())
    result.push_back({"stars", strings::JoinStrings(m_stars, ',')});

  return result;
}
}  // namespace http
}  // namespace booking
