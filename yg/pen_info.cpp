#include "pen_info.hpp"
#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"

namespace yg
{
  PenInfo::PenInfo()
  {}

  PenInfo::PenInfo(Color const & color, double w, double const * pattern, size_t patternSize, double offset)
    : m_color(color), m_w(w), m_offset(offset), m_isSolid(false)
  {
    if (m_w < 1.25)
      m_w = 1.25;

    /// if pattern is solid
    if ((pattern == 0 ) || (patternSize == 0))
      m_isSolid = true;
    else
    {
      buffer_vector<double, 4> vec;
      copy(pattern, pattern + patternSize, back_inserter(vec));
      double length = 0;
      for (size_t i = 0; i < vec.size(); ++i)
      {
        if ((vec[i] < 2) && (vec[i] > 0))
          vec[i] = 2;
        length += vec[i];
      }

      int periods = min(20, max(int(128 / length), 1));
      m_pat.reserve(periods * vec.size());
      for (int i = 0; i < periods; ++i)
        copy(vec.begin(), vec.end(), back_inserter(m_pat));

/*      copy(pattern, pattern + patternSize, back_inserter(m_pat));
      double length = 0;
      for (size_t i = 0; i < m_pat.size(); ++i)
      {
        if ((m_pat[i] < 2) && (m_pat[i] > 0))
          m_pat[i] = 2;
        length += m_pat[i];
      }
*/
    }
  }

  double PenInfo::firstDashOffset() const
  {
    double res = 0;
    for (size_t i = 0; i < m_pat.size() / 2; ++i)
    {
      if (m_pat[i * 2] == 0)
        res += m_pat[i * 2 + 1];
    }
    return res;
  }

  bool PenInfo::atDashOffset(double offset) const
  {
    double nextDashStart = 0;
    for (size_t i = 0; i < m_pat.size() / 2; ++i)
    {
      if ((offset >= nextDashStart) && (offset < nextDashStart + m_pat[i * 2]))
        return true;
      else
        if ((offset >= nextDashStart + m_pat[i * 2]) && (offset < nextDashStart + m_pat[i * 2] + m_pat[i * 2 + 1]))
          return false;

      nextDashStart += m_pat[i * 2] + m_pat[i * 2 + 1];
    }
    return false;
  }

  bool operator < (PenInfo const & l, PenInfo const & r)
  {
    if (l.m_isSolid != r.m_isSolid)
      return l.m_isSolid < r.m_isSolid;
    if (l.m_color != r.m_color)
      return l.m_color < r.m_color;
    if (l.m_w != r.m_w)
      return l.m_w < r.m_w;
    if (l.m_offset != r.m_offset)
      return l.m_offset < r.m_offset;
    if (l.m_pat.size() != r.m_pat.size())
      return l.m_pat.size() < r.m_pat.size();
    for (size_t i = 0; i < l.m_pat.size(); ++i)
      if (l.m_pat[i] != r.m_pat[i])
        return l.m_pat[i] < r.m_pat[i];

    return false;
  }
}
