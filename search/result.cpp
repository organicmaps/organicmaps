#include "result.hpp"
#include "../base/base.hpp"
#include "../base/string_utils.hpp"

namespace search
{

Result::Result(string const & str, m2::RectD const & rect, int penalty)
  : m_str(str), m_rect(rect), m_penalty(penalty)
{
#ifdef DEBUG
  if (!str.empty())
  {
    m_str.push_back(' ');
    m_str += strings::to_string(penalty);
  }
#endif
}


}  // namespace search
