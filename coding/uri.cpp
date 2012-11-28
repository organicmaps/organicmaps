#include "uri.hpp"
#include "url_encode.hpp"
#include "../base/logging.hpp"
#include "../std/algorithm.hpp"

using namespace url_scheme;

void Uri::Init()
{
  if (!Parse())
  {
    m_scheme.clear();
    m_path.clear();
  }
}

bool Uri::Parse()
{
  // get url scheme
  size_t pathStart = m_url.find(':');
  if (pathStart == string::npos || pathStart == 0)
    return false;
  m_scheme.assign(m_url, 0, pathStart);

  // skip slashes
  while (++pathStart < m_url.size() && m_url[pathStart] == '/') {};

  // get path
  m_queryStart = m_url.find('?', pathStart);
  m_path.assign(m_url, pathStart, m_queryStart - pathStart);

  // url without query
  if (m_queryStart == string::npos)
    m_queryStart = m_url.size();
  else
    ++m_queryStart;

  return true;
}

void Uri::ForEachKeyValue(CallbackT const & callback) const
{
  // parse query for keys and values
  for (size_t start = m_queryStart; start < m_url.size(); )
  {
    // TODO: Unoptimal search here, since it goes until the end of the string.
    size_t const end = min(m_url.size(), m_url.find('&', start));

    // Skip empty keys.
    if (end - start > 0)
    {
      size_t const eq = m_url.find('=', start);

      string key, value;
      if (eq < end)
      {
        key = UrlDecode(m_url.substr(start, eq - start));
        value = UrlDecode(m_url.substr(eq + 1, end - eq - 1));
      }
      else
      {
        key = UrlDecode(m_url.substr(start, end - start));
      }
      callback(key, value);
    }

    start = end + 1;
  }
}
