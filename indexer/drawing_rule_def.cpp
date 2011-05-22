#include "../base/SRC_FIRST.hpp"

#include "drawing_rule_def.hpp"

#include "../base/macros.hpp"
#include "../base/assert.hpp"
#include "../base/string_utils.hpp"

#include "../std/stdio.hpp"

#include "../base/start_mem_debug.hpp"

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

    string_utils::TokenizeIterator it(s, "|");
    size_t i = 0;
    while (!it.end())
    {
      ASSERT ( i < ARRAY_SIZE(arrParams), (i) );

      *(arrParams[i++]) = atoi((*it).c_str());
      ++it;
    }
  }
}
