#include "pen_info.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/numeric.hpp"

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
      buffer_vector<double, 4> tmpV;
      copy(pattern, pattern + patternSize, back_inserter(tmpV));

      if (tmpV.size() % 2)
        tmpV.push_back(0);

      double length = 0;

      /// ensuring that a minimal element has a length of 2px
      for (size_t i = 0; i < tmpV.size(); ++i)
      {
        if ((tmpV[i] < 2) && (tmpV[i] > 0))
          tmpV[i] = 2;
        length += tmpV[i];
      }

      int i = 0;

      buffer_vector<double, 4> vec;

      if ((offset >= length) || (offset < 0))
        offset -= floor(offset / length) * length;

      double curLen = 0;

      /// shifting pattern
      while (true)
      {
        if (curLen + tmpV[i] > offset)
        {
          //we're inside, let's split the pattern

          if (i % 2 == 1)
            vec.push_back(0);

          vec.push_back(curLen + tmpV[i] - offset);
          copy(tmpV.begin() + i + 1, tmpV.end(), back_inserter(vec));
          copy(tmpV.begin(), tmpV.begin() + i, back_inserter(vec));
          vec.push_back(offset - curLen);

          if (i % 2 == 0)
            vec.push_back(0);

          break;
        }
        else
          curLen += tmpV[i++];
      }

      int periods = max(int(256 / length), 1);
      m_pat.reserve(periods * vec.size());
      for (int i = 0; i < periods; ++i)
        copy(vec.begin(), vec.end(), back_inserter(m_pat));
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

  m2::PointU const PenInfo::patternSize() const
  {
    if (m_isSolid)
      return m2::PointU(static_cast<uint32_t>(ceil(m_w / 2)) * 2 + 4,
                        static_cast<uint32_t>(ceil(m_w / 2)) * 2 + 4);
    else
    {
      uint32_t len = static_cast<uint32_t>(accumulate(m_pat.begin(), m_pat.end(), 0.0));
      return m2::PointU(len + 4, static_cast<uint32_t>(m_w) + 4);
    }
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
