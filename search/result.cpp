#include "result.hpp"

namespace search
{

Result::Result(string const & str, m2::RectD const & rect)
  : m_str(str), m_rect(rect)
{
}

}  // namespace search
