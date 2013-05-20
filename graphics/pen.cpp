#include "pen.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/numeric.hpp"

#include "opengl/data_traits.hpp"
#include "agg_traits.hpp"

namespace graphics
{
  Pen::Info::Info(Color const & color,
                  double w,
                  double const * pattern,
                  size_t patternSize,
                  double offset,
                  char const * symbol,
                  double step,
                  ELineJoin join,
                  ELineCap cap)
    : Resource::Info(EPen),
      m_color(color),
      m_w(w),
      m_offset(offset),
      m_step(step),
      m_join(join),
      m_cap(cap),
      m_isSolid(false)
  {
    if (symbol != 0)
      m_icon = Icon::Info(symbol);

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

  double Pen::Info::firstDashOffset() const
  {
    double res = 0;
    for (size_t i = 0; i < m_pat.size() / 2; ++i)
    {
      if (m_pat[i * 2] == 0)
        res += m_pat[i * 2 + 1];
    }
    return res;
  }

  bool Pen::Info::atDashOffset(double offset) const
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

  Resource::Info const & Pen::Info::cacheKey() const
  {
    if (!m_icon.m_name.empty())
      return m_icon;
    else
      return *this;
  }

  m2::PointU const Pen::Info::resourceSize() const
  {
    if (!m_icon.m_name.empty())
      return m_icon.resourceSize();

    if (m_isSolid)
      return m2::PointU(static_cast<uint32_t>(ceil(m_w / 2)) * 2 + 4,
                        static_cast<uint32_t>(ceil(m_w / 2)) * 2 + 4);
    else
    {
      uint32_t len = static_cast<uint32_t>(accumulate(m_pat.begin(), m_pat.end(), 0.0));
      return m2::PointU(len + 4, static_cast<uint32_t>(m_w) + 4);
    }
  }

  Resource * Pen::Info::createResource(m2::RectU const & texRect,
                                       uint8_t pipelineID) const
  {
    return new Pen(texRect,
                   pipelineID,
                   *this);
  }

  bool Pen::Info::lessThan(Resource::Info const * r) const
  {
    if (m_category != r->m_category)
      return m_category < r->m_category;

    Info const * rp = static_cast<Info const *>(r);

    if (m_isSolid != rp->m_isSolid)
      return m_isSolid < rp->m_isSolid;
    if (m_color != rp->m_color)
      return m_color < rp->m_color;
    if (m_w != rp->m_w)
      return m_w < rp->m_w;
    if (m_offset != rp->m_offset)
      return m_offset < rp->m_offset;
    if (m_pat.size() != rp->m_pat.size())
      return m_pat.size() < rp->m_pat.size();
    for (size_t i = 0; i < m_pat.size(); ++i)
      if (m_pat[i] != rp->m_pat[i])
        return m_pat[i] < rp->m_pat[i];
    if (m_join != rp->m_join)
      return m_join < rp->m_join;
    if (m_cap != rp->m_cap)
      return m_cap < rp->m_cap;
    if (m_icon.m_name != rp->m_icon.m_name)
      return m_icon.m_name < rp->m_icon.m_name;
    if (m_step != rp->m_step)
      return m_step < rp->m_step;

    return false;
  }

  Pen::Pen(m2::RectU const & texRect,
           int pipelineID,
           Info const & info)
  : Resource(EPen, texRect, pipelineID),
    m_info(info),
    m_isSolid(info.m_isSolid)
  {
    if (m_isSolid)
      m_borderColorPixel = m_centerColorPixel = m2::PointU(texRect.minX() + 1, texRect.minY() + 1);
    else
    {
      double firstDashOffset = info.firstDashOffset();
      m_centerColorPixel = m2::PointU(static_cast<uint32_t>(firstDashOffset + texRect.minX() + 3),
                                      static_cast<uint32_t>(texRect.minY() + texRect.SizeY() / 2.0));
      m_borderColorPixel = m2::PointU(static_cast<uint32_t>(firstDashOffset + texRect.minX() + 3),
                                    static_cast<uint32_t>(texRect.minY() + 1));
    }
  }

  double Pen::geometryTileLen() const
  {
    return m_texRect.SizeX() - 2;
  }

  double Pen::geometryTileWidth() const
  {
    return m_texRect.SizeY() - 2;
  }

  double Pen::rawTileLen() const
  {
    return m_texRect.SizeX() - 4;
  }

  double Pen::rawTileWidth() const
  {
    return m_texRect.SizeY() - 4;
  }

  void Pen::render(void * dst)
  {
    Info const & info = m_info;
    m2::RectU const & rect = m_texRect;

    DATA_TRAITS::view_t v = gil::interleaved_view(rect.SizeX(),
                                                  rect.SizeY(),
                                                  (DATA_TRAITS::pixel_t*)dst,
                                                  sizeof(DATA_TRAITS::pixel_t) * rect.SizeX());

    graphics::Color penColor = info.m_color;

    penColor /= DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::pixel_t penColorTranslucent;

    gil::get_color(penColorTranslucent, gil::red_t()) = penColor.r;
    gil::get_color(penColorTranslucent, gil::green_t()) = penColor.g;
    gil::get_color(penColorTranslucent, gil::blue_t()) = penColor.b;
    gil::get_color(penColorTranslucent, gil::alpha_t()) = 0;

    DATA_TRAITS::pixel_t pxPenColor = penColorTranslucent;
    gil::get_color(pxPenColor, gil::alpha_t()) = penColor.a;

    if (info.m_isSolid)
    {
      /// draw circle

      // we need symmetrical non-antialiased circle. To achive symmetry, ceil it to pixel boundary
      // exact sub-pixel line width gets set later, and texture gets sceled down a bit for anti-aliasing
      float r = ceil(info.m_w / 2.0);

      agg::rendering_buffer buf(
          (unsigned char *)&v(0, 0),
          rect.SizeX(),
          rect.SizeY(),
          rect.SizeX() * sizeof(DATA_TRAITS::pixel_t)
          );

      typedef AggTraits<DATA_TRAITS>::pixfmt_t agg_pixfmt_t;

      agg_pixfmt_t pixfmt(buf);
      agg::renderer_base<agg_pixfmt_t> rbase(pixfmt);

      gil::fill_pixels(v, penColorTranslucent);

      agg::scanline_u8 s;
      agg::rasterizer_scanline_aa<> rasterizer;

      agg::ellipse ell;

      ell.init(r + 2, r + 2, r, r, 100);
      rasterizer.add_path(ell);

      agg::render_scanlines_aa_solid(rasterizer,
                                     s,
                                     rbase,
                                     agg::rgba8(info.m_color.r,
                                                info.m_color.g,
                                                info.m_color.b,
                                                info.m_color.a));

      for (size_t x = 2; x <= v.width() - 2; ++x)
        for (size_t y = 2; y <= v.height() - 2; ++y)
        {
          unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());
          if (alpha != 0)
            v(x, y) = pxPenColor;
        }
    }
    else
    {
      /// First two and last two rows of a pattern are filled
      /// with a penColorTranslucent pixel for the antialiasing.
      for (size_t y = 0; y < 2; ++y)
        for (size_t x = 0; x < rect.SizeX(); ++x)
          v(x, y) = penColorTranslucent;

      for (size_t y = rect.SizeY() - 2; y < rect.SizeY(); ++y)
        for (size_t x = 0; x < rect.SizeX(); ++x)
          v(x, y) = penColorTranslucent;

      /// first and last two pixels filled with penColorTranslucent
      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
      {
        v(0, y) = penColorTranslucent;
        v(1, y) = penColorTranslucent;
        v(rect.SizeX() - 2, y) = penColorTranslucent;
        v(rect.SizeX() - 1, y) = penColorTranslucent;
      }

      /// draw pattern
      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
      {
        double curLen = 0;

        DATA_TRAITS::pixel_t px = pxPenColor;

        /// In general case this code is incorrect.
        /// TODO : Make a pattern start and end with a dash.
        uint32_t curLenI = static_cast<uint32_t>(curLen);

        v(curLenI, y) = px;
        v(curLenI + 1, y) = px;

        for (size_t i = 0; i < info.m_pat.size(); ++i)
        {
          for (size_t j = 0; j < info.m_pat[i]; ++j)
          {
            uint32_t val = (i + 1) % 2;

            if (val == 0)
              gil::get_color(px, gil::alpha_t()) = 0;
            else
              gil::get_color(px, gil::alpha_t()) = penColor.a;

            v(curLenI + j + 2, y) = px;
          }

          v(static_cast<uint32_t>(curLen + 2 + info.m_pat[i]), y) = px;
          v(static_cast<uint32_t>(curLen + 2 + info.m_pat[i] + 1), y) = px;

          curLen += info.m_pat[i];

          curLenI = static_cast<uint32_t>(curLen);
        }
      }
    }
  }

  Resource::Info const * Pen::info() const
  {
    return &m_info;
  }

}
