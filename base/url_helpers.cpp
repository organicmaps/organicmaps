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

  bool firstParam = baseUrl.find('?') == string::npos;
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

std::string Join(std::string const & lhs, std::string const & rhs)
{
  if (lhs.empty())
    return rhs;
  if (rhs.empty())
    return lhs;

  if (lhs.back() == '/' && rhs.front() == '/')
    return lhs + rhs.substr(1);

  if (lhs.back() != '/' && rhs.front() != '/')
    return lhs + '/' + rhs;

  return lhs + rhs;
}
}  // namespace url
}  // namespace base
