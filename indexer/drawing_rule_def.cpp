#include "../base/SRC_FIRST.hpp"

#include "drawing_rule_def.hpp"

#include "../base/macros.hpp"
#include "../base/assert.hpp"
#include "../base/string_utils.hpp"

#include "../std/cstdio.hpp"


namespace drule
{
  string Key::toString() const
  {
    char buffer[50];
    sprintf(buffer, "%d|%d|%d|%d", m_scale, m_type, m_index, m_priority);
    return buffer;
  }

  void Key::fromString(string const & s)
  {
    int * arrParams[] = { &m_scale, &m_type, &m_index, &m_priority };
    size_t const count = s.size();

    size_t beg = 0;
    size_t i = 0;
    do
    {
      size_t end = s.find_first_of('|', beg);
      if (end == string::npos)
        end = count;

      ASSERT ( i < ARRAY_SIZE(arrParams), (i) );
      //*(arrParams[i++]) = atoi(s.substr(beg, end - beg).c_str());
      *(arrParams[i++]) = strtol(&s[beg], 0, 10);

      beg = end + 1;
    } while (beg < count);
  }

  namespace
  {
    struct less_key
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        if (r1.m_type == r2.m_type)
        {
          // assume that unique algo leaves the first element (with max priority), others - go away
          return (r1.m_priority > r2.m_priority);
        }
        else
          return (r1.m_type < r2.m_type);
      }
    };

    struct equal_key
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        // many line and area rules - is ok, other rules - one is enough
        // By VNG: Why many area styles ??? Did I miss something ???
        if (r1.m_type == drule::line /*|| r1.m_type == drule::area*/)
          return (r1 == r2);
        else
          return (r1.m_type == r2.m_type);
      }
    };

    struct less_scale_type_depth
    {
      bool operator() (drule::Key const & r1, drule::Key const & r2) const
      {
        if (r1.m_scale == r2.m_scale)
        {
          if (r1.m_type == r2.m_type)
          {
            return (r1.m_priority < r2.m_priority);
          }
          else return (r1.m_type < r2.m_type);
        }
        else return (r1.m_scale < r2.m_scale);
      }
    };
  }

  void MakeUnique(vector<Key> & keys)
  {
    sort(keys.begin(), keys.end(), less_key());
    keys.erase(unique(keys.begin(), keys.end(), equal_key()), keys.end());
  }

  void SortByScaleTypeDepth(vector<Key> & keys)
  {
    sort(keys.begin(), keys.end(), less_scale_type_depth());
  }
}
