#include "partners_api/utils.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace std::chrono;

namespace partners_api
{
namespace http
{
Result RunSimpleRequest(std::string const & url)
{
  platform::HttpClient request(url);
  bool result = false;

  if (request.RunHttpRequest() && !request.WasRedirected() && request.ErrorCode() == 200)
    result = true;

  return {result, request.ErrorCode(), request.ServerResponse()};
}
}  // namespace http

string FormatTime(system_clock::time_point p, string const & format)
{
  time_t t = duration_cast<seconds>(p.time_since_epoch()).count();
  ostringstream os;
  os << put_time(gmtime(&t), format.c_str());
  return os.str();
}
}  // namespace partners_api
