#include "coding/uri.hpp"
#include "coding/url_encode.hpp"

#include "base/assert.hpp"

namespace url_scheme
{

void Uri::Init()
{
  if (!Parse())
  {
    ASSERT(m_scheme.empty() && m_path.empty() && !IsValid(), ());
    m_queryStart = m_url.size();
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
  while (++pathStart < m_url.size() && m_url[pathStart] == '/') {}

  // Find query starting point for (key, value) parsing.
  m_queryStart = m_url.find('?', pathStart);
  size_t pathLength;
  if (m_queryStart == string::npos)
  {
    m_queryStart = m_url.size();
    pathLength = m_queryStart - pathStart;
  }
  else
  {
    pathLength = m_queryStart - pathStart;
    ++m_queryStart;
  }

  // Get path (url without query).
  m_path.assign(m_url, pathStart, pathLength);

  return true;
}

bool Uri::ForEachKeyValue(TCallback const & callback) const
{
  // parse query for keys and values
  size_t const count = m_url.size();
  size_t const queryStart = m_queryStart;

  // Just a URL without parameters.
  if (queryStart == count)
    return false;

  for (size_t start = queryStart; start < count; )
  {
    size_t end = m_url.find('&', start);
    if (end == string::npos)
      end = count;

    // Skip empty keys.
    if (end != start)
    {
      size_t const eq = m_url.find('=', start);

      string key, value;
      if (eq != string::npos && eq < end)
      {
        key = UrlDecode(m_url.substr(start, eq - start));
        value = UrlDecode(m_url.substr(eq + 1, end - eq - 1));
      }
      else
        key = UrlDecode(m_url.substr(start, end - start));

      if (!callback(key, value))
        return false;
    }

    start = end + 1;
  }
  return true;
}

}
