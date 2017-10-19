#include "partners_api/booking_http.hpp"

#include "platform/http_client.hpp"

#include "base/string_utils.hpp"

#include <ctime>
#include <iomanip>

#include "private.h"

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

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
    return true;
  }

  return false;
}

string MakeApiUrl(string const & baseUrl, string const & func, Params const & params)
{
  ASSERT_NOT_EQUAL(params.size(), 0, ());

  ostringstream os;
  os << baseUrl << func << "?";

  bool firstParam = true;
  for (auto const & param : params)
  {
    if (firstParam)
    {
      firstParam = false;
    }
    else
    {
      os << "&";
    }
    os << param.first << "=" << param.second;
  }

  return os.str();
}

std::string FormatTime(Time p)
{
  time_t t = duration_cast<seconds>(p.time_since_epoch()).count();
  ostringstream os;
  os << put_time(std::gmtime(&t), "%Y-%m-%d");
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
    result.push_back({"min_review_score", std::to_string(m_minReviewScore)});

  if (!m_stars.empty())
    result.push_back({"stars", strings::JoinStrings(m_stars, ',')});

  return result;
}
}
}
