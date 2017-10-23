#include "base/url_helpers.hpp"

#include <sstream>

using namespace std;

namespace base
{
namespace url
{
string Make(string const & baseUrl, Params const & params)
{
  ostringstream os;
  os << baseUrl;

  bool firstParam = true;
  for (auto const & param : params)
  {
    if (firstParam)
    {
      firstParam = false;
      os << "?";
    }
    else
    {
      os << "&";
    }

    os << param.m_name << "=" << param.m_value;
  }

  return os.str();
}
}  // namespace url
}  // namespace base
