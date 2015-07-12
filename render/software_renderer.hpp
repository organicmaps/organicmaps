#pragma once

#include "path_info.hpp"
#include "area_info.hpp"
#include "frame_image.hpp"
#include "text_engine.h"

#include "geometry/point2d.hpp"

#include "graphics/icon.hpp"
#include "graphics/circle.hpp"
#include "graphics/glyph_cache.hpp"
#include "graphics/pen.hpp"
#include "graphics/brush.hpp"
#include "graphics/font_desc.hpp"

#include "base/string_utils.hpp"

#include "3party/agg/agg_rendering_buffer.h"
#include "3party/agg/agg_pixfmt_rgba.h"
#include "3party/agg/agg_renderer_scanline.h"
#include "3party/agg/agg_renderer_primitives.h"
#include "3party/agg/agg_path_storage.h"

#include "std/cstdint.hpp"
#include "std/unique_ptr.hpp"


class PathWrapper;

class SoftwareRenderer
{
public:
  SoftwareRenderer(graphics::GlyphCache::Params const & glyphCacheParams, graphics::EDensity density);

  void BeginFrame(uint32_t width, uint32_t height);

    void DrawSymbol(m2::PointD const & pt, graphics::EPosition anchor,
                    graphics::Icon::Info const & info);
    void DrawCircle(m2::PointD const & pt, graphics::EPosition anchor,
                    graphics::Circle::Info const & info);
    void DrawPath(di::PathInfo const & geometry, graphics::Pen::Info const & info);
    void DrawPath(PathWrapper & path, math::Matrix<double, 3, 3> const & m);
    void DrawArea(di::AreaInfo const & geometry, graphics::Brush::Info const & info);
    void DrawText(m2::PointD const & pt, graphics::EPosition anchor,
                  graphics::FontDesc const & primFont, strings::UniString const & primText);
    void DrawText(m2::PointD const & pt, graphics::EPosition anchor,
                  graphics::FontDesc const & primFont, graphics::FontDesc const & secFont,
                  strings::UniString const & primText, strings::UniString const & secText);
    void DrawPathText(di::PathInfo const & geometry, graphics::FontDesc const & font,
                      strings::UniString const & text);

    void CalculateSymbolMetric(m2::PointD const & pt, graphics::EPosition anchor,
                               graphics::Icon::Info const & info, m2::RectD & rect);

    void CalculateCircleMetric(m2::PointD const & pt, graphics::EPosition anchor,
                               graphics::Circle::Info const & info, m2::RectD & rect);

    void CalculateTextMetric(m2::PointD const & pt, graphics::EPosition anchor,
                             graphics::FontDesc const & font, strings::UniString const & text,
                             m2::RectD & result);
    /// Result must be bound box contains both texts
    void CalculateTextMetric(m2::PointD const & pt, graphics::EPosition anchor,
                             graphics::FontDesc const & primFont, graphics::FontDesc const & secFont,
                             strings::UniString const & primText, strings::UniString const & secText,
                             m2::RectD & result);
    /// rects - rect for each glyph
    void CalculateTextMetric(di::PathInfo const & geometry, graphics::FontDesc const & font,
                             strings::UniString const & text,
                             vector<m2::RectD> & rects);

  void EndFrame(FrameImage & image);
  m2::RectD FrameRect() const;

  graphics::GlyphCache * GetGlyphCache() { return m_glyphCache.get(); }

private:
  template <class TColor, class TOrder>
  struct TBlendAdaptor
  {
    using order_type = TOrder;
    using color_type = TColor;
    using TValueType = typename color_type::value_type;
    using TCalcType = typename color_type::calc_type;

    enum EBaseScale
    {
      SCALE_SHIFT = color_type::base_shift,
      SCALE_MASK = color_type::base_mask
    };

    static AGG_INLINE void blend_pix(unsigned op, TValueType * p, unsigned cr, unsigned cg,
                                     unsigned cb, unsigned ca, unsigned cover)
    {
      using TBlendTable = agg::comp_op_table_rgba<TColor, TOrder>;
      if (p[TOrder::A])
      {
        TBlendTable::g_comp_op_func[op](p, (cr * ca + SCALE_MASK) >> SCALE_SHIFT, (cg * ca + SCALE_MASK) >> SCALE_SHIFT,
                                        (cb * ca + SCALE_MASK) >> SCALE_SHIFT, ca, cover);
      }
      else
        TBlendTable::g_comp_op_func[op](p, cr, cg, cb, ca, cover);
    }
  };

public:
  using TBlender = TBlendAdaptor<agg::rgba8, agg::order_rgba>;
  using TPixelFormat = agg::pixfmt_custom_blend_rgba<TBlender, agg::rendering_buffer>;

  using TBaseRenderer = agg::renderer_base<TPixelFormat>;
  using TPrimitivesRenderer = agg::renderer_primitives<TBaseRenderer>;
  using TSolidRenderer = agg::renderer_scanline_aa_solid<TBaseRenderer>;

private:
  unique_ptr<graphics::GlyphCache> m_glyphCache;
  map<string, m2::RectU> m_symbolsIndex;
  vector<uint8_t> m_symbolsSkin;
  uint32_t m_skinWidth, m_skinHeight;

  std::vector<unsigned char> m_frameBuffer;
  uint32_t m_frameWidth, m_frameHeight;

  agg::rendering_buffer m_renderBuffer;
  TPixelFormat m_pixelFormat;
  TBaseRenderer m_baseRenderer;

  TSolidRenderer m_solidRenderer;

  ml::text_engine m_textEngine;
};

struct PathParams
{
    agg::rgba8 m_fillColor;
    agg::rgba8 m_strokeColor;
    bool m_isFill;
    bool m_isStroke;
    bool m_isEventOdd;
    double m_strokeWidth;

    PathParams()
      : m_fillColor(agg::rgba(0, 0, 0))
      , m_strokeColor(agg::rgba(0, 0, 0))
      , m_isFill(false)
      , m_isStroke(false)
      , m_isEventOdd(false)
      , m_strokeWidth(1.0)
    {
    }
};

class PathWrapper
{
public:
  void AddParams(PathParams const & params);
  void MoveTo(m2::PointD const & pt);
  void LineTo(m2::PointD const & pt);
  void CurveTo(m2::PointD const & pt1,
               m2::PointD const & pt2,
                m2::PointD const & ptTo);
  void ClosePath();
  void BoundingRect(m2::RectD & rect);

  void Render(SoftwareRenderer::TSolidRenderer & renderer,
              agg::trans_affine const & mtx,
              m2::RectD const & clipBox);

private:
  agg::path_storage  m_storage;
  vector<PathParams> m_params;
};
