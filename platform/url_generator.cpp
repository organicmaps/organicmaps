#include "url_generator.hpp"

#include "../base/macros.hpp"

#include "../std/ctime.hpp"

static char const * g_defaultFirstGroup[] = {
#ifdef OMIM_PRODUCTION
  "http://a0.mapswithme.com/",
  "http://a1.mapswithme.com/",
  "http://a2.mapswithme.com/",
  "http://a3.mapswithme.com/",
  "http://a4.mapswithme.com/",
  "http://a5.mapswithme.com/",
  "http://a6.mapswithme.com/",
  "http://a7.mapswithme.com/",
  "http://a8.mapswithme.com/",
  "http://a9.mapswithme.com/",
  "http://a10.mapswithme.com/",
  "http://a11.mapswithme.com/"
#else
  "http://svobodu404popugajam.mapswithme.com:34568/maps/"
#endif
};
static char const * g_defaultSecondGroup[] = {
#ifdef OMIM_PRODUCTION
  "http://b0.mapswithme.com/",
  "http://b1.mapswithme.com/",
  "http://b2.mapswithme.com/",
  "http://b3.mapswithme.com/",
  "http://b4.mapswithme.com/",
  "http://b5.mapswithme.com/"
#else
  "http://svobodu404popugajam.mapswithme.com:34568/maps/"
#endif
};

UrlGenerator::UrlGenerator()
  : m_randomGenerator(static_cast<uint32_t>(time(NULL))),
    m_firstGroup(&g_defaultFirstGroup[0], &g_defaultFirstGroup[0] + ARRAY_SIZE(g_defaultFirstGroup)),
    m_secondGroup(&g_defaultSecondGroup[0], &g_defaultSecondGroup[0] + ARRAY_SIZE(g_defaultSecondGroup))
{
}

UrlGenerator::UrlGenerator(vector<string> const & firstGroup, vector<string> const & secondGroup)
  : m_randomGenerator(static_cast<uint32_t>(time(NULL))), m_firstGroup(firstGroup), m_secondGroup(secondGroup)
{
}

string UrlGenerator::PopNextUrl()
{
  string s;
  switch (m_firstGroup.size())
  {
  case 1:
    s = m_firstGroup.front();
    m_firstGroup.pop_back();
    break;
  case 0:
    switch (m_secondGroup.size())
    {
    case 1:
      s = m_secondGroup.front();
      m_secondGroup.pop_back();
      break;
    case 0: // nothing left to return
      break;
    default:
      {
        vector<string>::iterator const it = m_secondGroup.begin()
            + static_cast<vector<string>::difference_type>(m_randomGenerator.Generate() % m_secondGroup.size());
        s = *it;
        m_secondGroup.erase(it);
      }
    }
    break;
  default:
    {
      vector<string>::iterator const it = m_firstGroup.begin()
          + static_cast<vector<string>::difference_type>(m_randomGenerator.Generate() % m_firstGroup.size());
      s = *it;
      m_firstGroup.erase(it);
    }
  }
  return s;
}
