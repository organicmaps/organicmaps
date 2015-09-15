#pragma once

#include "drape_frontend/watch/icon_info.hpp"

#include "drape/drape_global.hpp"

#include "base/buffer_vector.hpp"
#include "base/math.hpp"

namespace df
{
namespace watch
{

struct PenInfo
{
  typedef buffer_vector<double, 16> TPattern;
  dp::Color m_color;
  double m_w;
  TPattern m_pat;
  double m_offset;
  IconInfo m_icon;
  double m_step;
  dp::LineJoin m_join;
  dp::LineCap m_cap;

  bool m_isSolid;

  PenInfo(dp::Color const & color = dp::Color(0, 0, 0, 255),
          double width = 1.0,
          double const * pattern = 0,
          size_t patternSize = 0,
          double offset = 0,
          char const * symbol = 0,
          double step = 0,
          dp::LineJoin join = dp::RoundJoin,
          dp::LineCap cap = dp::RoundCap)
    : m_color(color)
    , m_w(width)
    , m_offset(offset)
    , m_step(step)
    , m_join(join)
    , m_cap(cap)
    , m_isSolid(false)
  {
    if (symbol != nullptr)
      m_icon = IconInfo(symbol);

    if (!m_icon.m_name.empty())
    {
      m_isSolid = false;
    }
    else
    {
      /// if pattern is solid
      if ((pattern == 0 ) || (patternSize == 0))
        m_isSolid = true;
      else
      {
        // hack for Samsung GT-S5570 (GPU floor()'s texture pattern width)
        m_w = max(m_w, 1.0);

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

        int periods = max(int((256 - 4) / length), 1);
        m_pat.reserve(periods * vec.size());
        for (int i = 0; i < periods; ++i)
          copy(vec.begin(), vec.end(), back_inserter(m_pat));
      }
    }
  }
};

}
}
