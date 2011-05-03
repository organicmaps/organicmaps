#include "../base/SRC_FIRST.hpp"

#include "glyph_layout.hpp"
#include "resource_manager.hpp"
#include "font_desc.hpp"

#include "../geometry/angles.hpp"

namespace yg
{
  class pts_array
  {
    m2::PointD const * m_arr;
    size_t m_size;
    bool m_reverse;

  public:
    pts_array(m2::PointD const * arr, size_t sz, double fullLength, double & pathOffset)
      : m_arr(arr), m_size(sz), m_reverse(false)
    {
      ASSERT ( m_size > 1, () );

      /* assume, that readable text in path should be ('o' - start draw point):
       *    /   o
       *   /     \
       *  /   or  \
       * o         \
       */

      double const a = ang::AngleTo(m_arr[0], m_arr[m_size-1]);
      if (fabs(a) > math::pi / 2.0)
      {
        // if we swap direction, we need to recalculate path offset from the end
        double len = 0.0;
        for (size_t i = 1; i < m_size; ++i)
          len += m_arr[i-1].Length(m_arr[i]);

        pathOffset = fullLength - pathOffset - len;
        ASSERT ( pathOffset >= -1.0E-6, () );
        if (pathOffset < 0.0) pathOffset = 0.0;

        m_reverse = true;
      }
    }

    size_t size() const { return m_size; }

    m2::PointD get(size_t i) const
    {
      ASSERT ( i < m_size, ("Index out of range") );
      return m_arr[m_reverse ? m_size - i - 1 : i];
    }
    m2::PointD operator[](size_t i) const { return get(i); }
  };

  GlyphLayout::GlyphLayout(shared_ptr<ResourceManager> const & resourceManager,
                           FontDesc const & fontDesc,
                           m2::PointD const * pts,
                           size_t ptsCount,
                           wstring const & text,
                           double fullLength,
                           double pathOffset,
                           yg::EPosition pos)
    : m_resourceManager(resourceManager),
      m_firstVisible(0),
      m_lastVisible(0)
  {
    pts_array arrPath(pts, ptsCount, fullLength, pathOffset);

    // get vector of glyphs and calculate string length
    double strLength = 0.0;
    size_t count = text.size();
    m_entries.resize(count);

    for (size_t i = 0; i < m_entries.size(); ++i)
    {
      m_entries[i].m_sym = text[i];
      m_entries[i].m_metrics = m_resourceManager->getGlyphMetrics(GlyphKey(m_entries[i].m_sym, fontDesc.m_size, fontDesc.m_isMasked, yg::Color(0, 0, 0, 0)));
      strLength += m_entries[i].m_metrics.m_xAdvance;
    }

    // offset of the text from path's start
    double offset = (fullLength - strLength) / 2.0;
    if (offset < 0.0)
      return;
    offset -= pathOffset;
    if (-offset >= strLength)
      return;

    // find first visible glyph
    size_t symPos = 0;
    while (offset < 0 &&  symPos < count)
      offset += m_entries[symPos++].m_metrics.m_xAdvance;

    m_firstVisible = symPos;
    m_lastVisible = count;

    size_t ptPos = 0;
    bool doInitAngle = true;

    for (; symPos < count; ++symPos)
    {
      if (symPos > 0)
      {
        m_entries[symPos].m_pt = m_entries[symPos - 1].m_pt;
        m_entries[symPos].m_angle = m_entries[symPos - 1].m_angle;
      }
      else
      {
        m_entries[symPos].m_pt = arrPath[0];
        m_entries[symPos].m_angle = 0; //< will be initialized later
      }

      size_t const oldPtPos = ptPos;

      while (true)
      {
        if (ptPos + 1 == arrPath.size())
        {
          m_firstVisible = m_lastVisible = 0;
          return;
        }
        double const l = arrPath[ptPos + 1].Length(m_entries[symPos].m_pt);
        if (offset < l)
          break;

        offset -= l;
        m_entries[symPos].m_pt = arrPath[++ptPos];
      }

      if ((oldPtPos != ptPos) || (doInitAngle))
      {
        m_entries[symPos].m_angle = ang::AngleTo(m_entries[symPos].m_pt, arrPath[ptPos + 1]);
        doInitAngle = false;
      }

      m_entries[symPos].m_pt = m_entries[symPos].m_pt.Move(offset, m_entries[symPos].m_angle);
      offset = m_entries[symPos].m_metrics.m_xAdvance;
    }
  }

  size_t GlyphLayout::firstVisible() const
  {
    return m_firstVisible;
  }

  size_t GlyphLayout::lastVisible() const
  {
    return m_lastVisible;
  }

  vector<GlyphLayoutElem> const & GlyphLayout::entries() const
  {
    return m_entries;
  }
}
