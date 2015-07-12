#include "software_renderer.hpp"
#include "proto_to_styles.hpp"

#include "graphics/defines.hpp"
#include "graphics/skin_loader.hpp"

#include "platform/platform.hpp"

#include "coding/lodepng_io.hpp"
#include "coding/png_memory_encoder.hpp"
#include "coding/parse_xml.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "base/logging.hpp"

#include "3party/agg/agg_rasterizer_scanline_aa.h"
#include "3party/agg/agg_scanline_p.h"
#include "3party/agg/agg_path_storage.h"
#include "3party/agg/agg_conv_stroke.h"
#include "3party/agg/agg_conv_dash.h"
#include "3party/agg/agg_ellipse.h"
#include "3party/agg/agg_conv_curve.h"
#include "3party/agg/agg_conv_stroke.h"
#include "3party/agg/agg_conv_contour.h"
#include "3party/agg/agg_bounding_rect.h"

#include "3party/agg/agg_vcgen_stroke.cpp"
#include "3party/agg/agg_vcgen_dash.cpp"


#define BLENDER_TYPE agg::comp_op_src_over

class agg_symbol_renderer : public ml::text_renderer
{
  graphics::Color m_color;
  graphics::Color m_colorMask;
  SoftwareRenderer::TBaseRenderer & m_baserenderer;

public:
  agg_symbol_renderer(SoftwareRenderer::TBaseRenderer & baserenderer,
                      graphics::Color const & color, graphics::Color const & colorMask)
      : m_color(color), m_colorMask(colorMask), m_baserenderer(baserenderer)
  {
    m_outline = false;
  }

  virtual double outlinewidth() const { return 3; }

  virtual void operator()(ml::point_d const & pt, size_t width, size_t height,
                          unsigned char const * data)
  {
    graphics::Color color = m_outline ? m_colorMask : m_color;
    for (size_t y = 0; y < height; ++y)
    {
      m_baserenderer.blend_solid_hspan(pt.x, pt.y + y, (int)width,
                                       agg::rgba8(color.r, color.g, color.b, color.a),
                                       &data[(height - y - 1) * width]);
    }
  }
};

void AlignText(ml::text_options & opt, graphics::EPosition anchor)
{
  switch (anchor)
  {
  case graphics::EPosCenter:
    opt.horizontal_align(ml::text::align_center);
    opt.vertical_align(ml::text::align_center);
    break;
  case graphics::EPosAbove:
    opt.horizontal_align(ml::text::align_center);
    opt.vertical_align(ml::text::align_top);
    break;
  case graphics::EPosUnder:
    opt.horizontal_align(ml::text::align_center);
    opt.vertical_align(ml::text::align_bottom);
    break;
  case graphics::EPosLeft:
    opt.horizontal_align(ml::text::align_left);
    opt.vertical_align(ml::text::align_center);
    break;
  case graphics::EPosRight:
    opt.horizontal_align(ml::text::align_right);
    opt.vertical_align(ml::text::align_center);
    break;
  case graphics::EPosAboveLeft:
    opt.horizontal_align(ml::text::align_left);
    opt.vertical_align(ml::text::align_top);
    break;
  case graphics::EPosAboveRight:
    opt.horizontal_align(ml::text::align_right);
    opt.vertical_align(ml::text::align_top);
    break;
  case graphics::EPosUnderLeft:
    opt.horizontal_align(ml::text::align_left);
    opt.vertical_align(ml::text::align_bottom);
    break;
  case graphics::EPosUnderRight:
    opt.horizontal_align(ml::text::align_right);
    opt.vertical_align(ml::text::align_bottom);
    break;
  }
}

void AlignImage(m2::PointD & pt, graphics::EPosition anchor, size_t width, size_t height)
{
  switch (anchor)
  {
  case graphics::EPosCenter:
    return;
  case graphics::EPosAbove:
    pt.y -= height / 2;
    break;
  case graphics::EPosUnder:
    pt.y += height / 2;
    break;
  case graphics::EPosLeft:
    pt.x -= width / 2;
    break;
  case graphics::EPosRight:
    pt.x += width / 2;
    break;
  case graphics::EPosAboveLeft:
    pt.y -= height / 2;
    pt.x -= width / 2;
    break;
  case graphics::EPosAboveRight:
    pt.y -= height / 2;
    pt.x += width / 2;
    break;
  case graphics::EPosUnderLeft:
    pt.y += height / 2;
    pt.x -= width / 2;
    break;
  case graphics::EPosUnderRight:
    pt.y += height / 2;
    pt.x += width / 2;
    break;
  }
}

SoftwareRenderer::SoftwareRenderer(graphics::GlyphCache::Params const & glyphCacheParams, graphics::EDensity density)
  : m_glyphCache(new graphics::GlyphCache(glyphCacheParams))
  , m_skinWidth(0)
  , m_skinHeight(0)
  , m_frameWidth(0)
  , m_frameHeight(0)
  , m_pixelFormat(m_renderBuffer , BLENDER_TYPE)
  , m_baseRenderer(m_pixelFormat)
  , m_solidRenderer(m_baseRenderer)
{
  Platform & pl = GetPlatform();

  Platform::FilesList fonts;
  pl.GetFontNames(fonts);
  m_glyphCache->addFonts(fonts);

  string textureFileName;

  graphics::SkinLoader loader([this, &textureFileName](m2::RectU const & rect, string const & symbolName, int32_t id, string const & fileName)
  {
    UNUSED_VALUE(id);
    if (textureFileName.empty())
      textureFileName =  fileName;

    m_symbolsIndex[symbolName] = rect;
  });

  ReaderSource<ReaderPtr<Reader>> source(ReaderPtr<Reader>(GetStyleReader().GetResourceReader("basic.skn", convert(density))));
  if (!ParseXML(source, loader))
    LOG(LERROR, ("Error parsing skin"));

  ASSERT(!textureFileName.empty(), ());
  ReaderPtr<Reader> texReader(GetStyleReader().GetResourceReader(textureFileName, convert(density)));
  vector<uint8_t> textureData;
  LodePNG::loadFile(textureData, texReader);
  VERIFY(LodePNG::decode(m_symbolsSkin, m_skinWidth, m_skinHeight, textureData) == 0, ());
  ASSERT(m_skinWidth != 0 && m_skinHeight != 0, ());
}

void SoftwareRenderer::BeginFrame(uint32_t width, uint32_t height)
{
  ASSERT(m_frameWidth == 0 && m_frameHeight == 0, ());
  m_frameWidth = width;
  m_frameHeight = height;

  //@TODO (yershov) create agg context here
  m_frameBuffer.resize(m_frameWidth * 4 * m_frameHeight);
  memset(&m_frameBuffer[0], 0, m_frameBuffer.size());
  m_renderBuffer.attach(&m_frameBuffer[0], static_cast<unsigned int>(m_frameWidth),
                        static_cast<unsigned int>(m_frameHeight),
                        static_cast<unsigned int>(m_frameWidth * 4));
  m_baseRenderer.reset_clipping(true);
  unsigned op = m_pixelFormat.comp_op();
  m_pixelFormat.comp_op(agg::comp_op_src);
  graphics::Color c = ConvertColor(drule::rules().GetBgColor());
  m_baseRenderer.clear(agg::rgba8(c.r, c.g, c.b, c.a));
  m_pixelFormat.comp_op(op);
}

void SoftwareRenderer::DrawSymbol(m2::PointD const & pt, graphics::EPosition anchor, graphics::Icon::Info const & info)
{
  //@TODO (yershov) implement it
  typedef TBlendAdaptor<agg::rgba8, agg::order_rgba> blender_t;
  typedef agg::pixfmt_custom_blend_rgba<blender_t, agg::rendering_buffer> pixel_format_t;
  agg::rendering_buffer renderbuffer;

  m2::RectU const & r = m_symbolsIndex[info.m_name];

  m2::PointD p = pt;
  AlignImage(p, anchor, r.SizeX(), r.SizeY());

  renderbuffer.attach(&m_symbolsSkin[(m_skinWidth * 4) * r.minY() + (r.minX() * 4)],
                      static_cast<unsigned int>(r.SizeX()),
                      static_cast<unsigned int>(r.SizeY()),
                      static_cast<unsigned int>(m_skinWidth * 4));
  pixel_format_t pixelformat(renderbuffer, BLENDER_TYPE);
  m_baseRenderer.blend_from(pixelformat, 0, (int)(p.x - r.SizeX() / 2), (int)(p.y - r.SizeY() / 2));
}

void SoftwareRenderer::DrawCircle(m2::PointD const & pt, graphics::EPosition anchor, graphics::Circle::Info const & info)
{
  //@TODO (yershov) implement it
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.clip_box(0, 0, m_frameWidth, m_frameHeight);
  m2::PointD p = pt;
  AlignImage(p, anchor, info.m_radius, info.m_radius);
  agg::ellipse path(p.x, p.y, info.m_radius, info.m_radius, 15);
  typedef agg::conv_stroke<agg::ellipse> stroke_t;
  agg::scanline32_p8 scanline;

  if (info.m_isOutlined)
  {
    stroke_t stroke_path(path);
    stroke_path.width(info.m_outlineWidth * 2);
    rasterizer.add_path(stroke_path);
    agg::rgba8 color(info.m_outlineColor.r, info.m_outlineColor.g, info.m_outlineColor.b,
                     info.m_outlineColor.a);
    agg::render_scanlines_aa_solid(rasterizer, scanline, m_baseRenderer, color);
    rasterizer.reset();
  }
  rasterizer.add_path(path);
  agg::rgba8 color(info.m_color.r, info.m_color.g, info.m_color.b, info.m_color.a);
  agg::render_scanlines_aa_solid(rasterizer, scanline, m_baseRenderer, color);
}

inline agg::line_cap_e translateLineCap(unsigned int line_cap)
{
  switch (line_cap)
  {
    case graphics::Pen::Info::EButtCap:
      return agg::butt_cap;
    case graphics::Pen::Info::ESquareCap:
      return agg::square_cap;
    case graphics::Pen::Info::ERoundCap:
      return agg::round_cap;
  }
  return agg::round_cap;
}

inline agg::line_join_e translateLineJoin(unsigned int line_join)
{
  switch (line_join)
  {
    case graphics::Pen::Info::ENoJoin:
      return agg::miter_join;
    case graphics::Pen::Info::ERoundJoin:
      return agg::round_join;
    case graphics::Pen::Info::EBevelJoin:
      return agg::bevel_join;
  }
  return agg::bevel_join;
}

void SoftwareRenderer::DrawPath(di::PathInfo const & geometry, graphics::Pen::Info const & info)
{
  if (!info.m_icon.m_name.empty())
    return;

  //@TODO (yershov) implement it
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.clip_box(0, 0, m_frameWidth, m_frameHeight);
  typedef agg::poly_container_adaptor<vector<m2::PointD>> path_t;
  path_t path_adaptor(geometry.m_path, false);
  typedef agg::conv_stroke<path_t> stroke_t;

  if (!info.m_pat.empty())
  {
    agg::conv_dash<path_t> dash(path_adaptor);
    dash.dash_start(0);
    for (size_t i = 1; i < info.m_pat.size(); i += 2)
    {
      dash.add_dash(info.m_pat[i - 1], info.m_pat[i]);
    }

    agg::conv_stroke<agg::conv_dash<path_t>> stroke(dash);
    stroke.width(info.m_w);
    stroke.line_cap(translateLineCap(info.m_cap));
    stroke.line_join(translateLineJoin(info.m_join));
    rasterizer.add_path(stroke);
  }
  else
  {
    stroke_t stroke(path_adaptor);
    stroke.width(info.m_w);
    stroke.line_cap(translateLineCap(info.m_cap));
    stroke.line_join(translateLineJoin(info.m_join));
    rasterizer.add_path(stroke);
  }

  agg::scanline32_p8 scanline;
  agg::rgba8 color(info.m_color.r, info.m_color.g, info.m_color.b, info.m_color.a);
  agg::render_scanlines_aa_solid(rasterizer, scanline, m_baseRenderer, color);
}

void SoftwareRenderer::DrawPath(PathWrapper & path, math::Matrix<double, 3, 3> const & m)
{
  agg::trans_affine aggM(m(0, 0), m(0, 1), m(1, 0), m(1, 1), m(2, 0), m(2, 1));
  path.Render(m_solidRenderer, aggM, FrameRect());
}

void SoftwareRenderer::DrawArea(di::AreaInfo const & geometry, graphics::Brush::Info const & info)
{
  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.clip_box(0, 0, m_frameWidth, m_frameHeight);

  agg::path_storage path;
  for (size_t i = 2; i < geometry.m_path.size(); i += 3)
  {
    path.move_to(geometry.m_path[i - 2].x, geometry.m_path[i - 2].y);
    path.line_to(geometry.m_path[i - 1].x, geometry.m_path[i - 1].y);
    path.line_to(geometry.m_path[i].x, geometry.m_path[i].y);
    path.line_to(geometry.m_path[i - 2].x, geometry.m_path[i - 2].y);
  }

  rasterizer.add_path(path);
  agg::scanline32_p8 scanline;
  agg::rgba8 color(info.m_color.r, info.m_color.g, info.m_color.b, info.m_color.a);
  bool antialias = false;
  if (antialias)
  {
    agg::render_scanlines_aa_solid(rasterizer, scanline, m_baseRenderer, color);
  }
  else
  {
    rasterizer.filling_rule(agg::fill_even_odd);
    agg::render_scanlines_bin_solid(rasterizer, scanline, m_baseRenderer, color);
  }
}

void SoftwareRenderer::DrawText(m2::PointD const & pt, graphics::EPosition anchor, graphics::FontDesc const & primFont, strings::UniString const & primText)
{
  //@TODO (yershov) implement it
  ml::text l(primText);
  ml::face & face = m_textEngine.get_face(0, "default", primFont.m_size);
  l.apply_font(face);

  ml::rect_d bounds = l.calc_bounds(face);
  vector<ml::point_d> base;
  base.push_back(ml::point_d(pt.x - bounds.width() / 2, pt.y));
  base.push_back(ml::point_d(pt.x + bounds.width() / 2, pt.y));

  ml::text_options opt(face);
  AlignText(opt, anchor);
  l.warp(base, opt);

  agg_symbol_renderer ren(m_baseRenderer, primFont.m_color, primFont.m_maskColor);
  if (primFont.m_isMasked)
  {
    ren.outline(true);
    l.render(face, ren);
    ren.outline(false);
  }
  l.render(face, ren);
}

void SoftwareRenderer::DrawText(m2::PointD const & pt, graphics::EPosition anchor,
                                graphics::FontDesc const & primFont, graphics::FontDesc const & secFont,
                                strings::UniString const & primText, strings::UniString const & secText)
{
  //@TODO (yershov) implement it
  ml::text prim(primText);
  ml::face & primFace = m_textEngine.get_face(0, "default", primFont.m_size);
  prim.apply_font(primFace);

  ml::rect_d bounds = prim.calc_bounds(primFace);
  vector<ml::point_d> base;
  base.push_back(ml::point_d(pt.x - bounds.width() / 2, pt.y));
  base.push_back(ml::point_d(pt.x + bounds.width() / 2, pt.y));

  ml::text_options opt(primFace);
  AlignText(opt, anchor);
  prim.warp(base, opt);
  bounds = prim.calc_bounds(primFace);

  agg_symbol_renderer ren(m_baseRenderer, primFont.m_color, primFont.m_maskColor);
  if (primFont.m_isMasked)
  {
    ren.outline(true);
    prim.render(primFace, ren);
    ren.outline(false);
  }
  prim.render(primFace, ren);

  ml::text sec(primText);
  ml::face & secFace = m_textEngine.get_face(0, "default", secFont.m_size);
  sec.apply_font(secFace);

  base.clear();
  ml::rect_d boundsSec = sec.calc_bounds(secFace);
  double currX = (bounds.min.x + bounds.max.x) / 2;
  base.push_back(ml::point_d(currX - boundsSec.width() / 2, bounds.max.y + boundsSec.height() / 2));
  base.push_back(ml::point_d(currX + boundsSec.width() / 2, bounds.max.y + boundsSec.height() / 2));

  ml::text_options secOpt(secFace);
  AlignText(secOpt, graphics::EPosCenter);
  sec.warp(base, secOpt);

  agg_symbol_renderer ren2(m_baseRenderer, secFont.m_color, secFont.m_maskColor);
  if (secFont.m_isMasked)
  {
    ren2.outline(true);
    sec.render(secFace, ren2);
    ren2.outline(false);
  }
  sec.render(secFace, ren2);
}

void SoftwareRenderer::CalculateSymbolMetric(m2::PointD const & pt, graphics::EPosition anchor,
                                             graphics::Icon::Info const & info, m2::RectD & rect)
{
  m2::RectD symbolR(m_symbolsIndex[info.m_name]);
  m2::PointD pivot = pt;
  AlignImage(pivot, anchor, symbolR.SizeX(), symbolR.SizeY());

  m2::PointD halfSize(0.5 * symbolR.SizeX(), 0.5 * symbolR.SizeY());
  rect = m2::RectD(pivot - halfSize, pivot + halfSize);
}

void SoftwareRenderer::CalculateCircleMetric(m2::PointD const & pt, graphics::EPosition anchor,
                                             graphics::Circle::Info const & info, m2::RectD & rect)
{
  m2::PointD pivot = pt;
  AlignImage(pivot, anchor, info.m_radius, info.m_radius);

  double half = 0.5 * info.m_radius;
  m2::PointD halfSize(half, half);
  rect = m2::RectD(pivot - halfSize, pivot + halfSize);
}

void SoftwareRenderer::CalculateTextMetric(m2::PointD const & pt, graphics::EPosition anchor,
                                           graphics::FontDesc const & font, strings::UniString const & text,
                                           m2::RectD & result)
{
  //@TODO (yershov) implement it
  ml::text l(text);
  ml::face & face = m_textEngine.get_face(0, "default", font.m_size);
  l.apply_font(face);

  ml::rect_d bounds = l.calc_bounds(face);
  vector<ml::point_d> base;
  base.push_back(ml::point_d(pt.x - bounds.width() / 2, pt.y));
  base.push_back(ml::point_d(pt.x + bounds.width() / 2, pt.y));

  ml::text_options opt(face);
  AlignText(opt, anchor);
  l.warp(base, opt);
  bounds = l.calc_bounds(face);
  result = m2::RectD(bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y);
}

void SoftwareRenderer::CalculateTextMetric(m2::PointD const & pt, graphics::EPosition anchor,
                                           graphics::FontDesc const  & primFont, graphics::FontDesc const & secFont,
                                           strings::UniString const & primText, strings::UniString const & secText,
                                           m2::RectD & result)
{
  //@TODO (yershov) implement it
  ml::text prim(primText);
  ml::face & primFace = m_textEngine.get_face(0, "default", primFont.m_size);
  prim.apply_font(primFace);

  ml::rect_d bounds = prim.calc_bounds(primFace);
  vector<ml::point_d> base;
  base.push_back(ml::point_d(pt.x - bounds.width() / 2, pt.y));
  base.push_back(ml::point_d(pt.x + bounds.width() / 2, pt.y));

  ml::text_options opt(primFace);
  AlignText(opt, anchor);
  prim.warp(base, opt);
  bounds = prim.calc_bounds(primFace);

  ml::text sec(primText);
  ml::face & secFace = m_textEngine.get_face(0, "default", secFont.m_size);
  sec.apply_font(secFace);

  base.clear();
  ml::rect_d boundsSec = sec.calc_bounds(secFace);
  double currX = (bounds.min.x + bounds.max.x) / 2;
  base.push_back(ml::point_d(currX - boundsSec.width() / 2, bounds.max.y + boundsSec.height() / 2));
  base.push_back(ml::point_d(currX + boundsSec.width() / 2, bounds.max.y + boundsSec.height() / 2));

  ml::text_options secOpt(secFace);
  AlignText(secOpt, graphics::EPosCenter);
  sec.warp(base, secOpt);
  boundsSec = sec.calc_bounds(secFace);

  bounds.extend(boundsSec);
  result = m2::RectD(bounds.min.x, bounds.min.y, bounds.max.x, bounds.max.y);
}

void SoftwareRenderer::CalculateTextMetric(di::PathInfo const & geometry, graphics::FontDesc const & font,
                                           strings::UniString const & text, vector<m2::RectD> & rects)
{
  ml::text l(text);
  ml::face & face = m_textEngine.get_face(0, "default", font.m_size);
  l.apply_font(face);

  vector<ml::point_d> base(geometry.m_path.size());
  size_t i = 0;
  for (auto const & p : geometry.m_path)
  {
    base[i].x = p.x;
    base[i].y = p.y;
    ++i;
  }

  ml::text_options opt(face);
  AlignText(opt, graphics::EPosCenter);
  l.warp(base, opt);
  l.calc_bounds(face);
  for (auto const & sym : l.symbols())
    rects.emplace_back(sym.bounds().min.x, sym.bounds().min.y, sym.bounds().max.x,
                       sym.bounds().max.y);
}

void SoftwareRenderer::DrawPathText(di::PathInfo const & geometry, graphics::FontDesc const & font,
                                    strings::UniString const & text)
{
  ml::text l(text);
  ml::face & face = m_textEngine.get_face(0, "default", font.m_size);
  l.apply_font(face);

  vector<ml::point_d> base(geometry.m_path.size());
  size_t i = 0;
  for (auto const & p : geometry.m_path)
  {
    base[i].x = p.x;
    base[i].y = p.y;
    ++i;
  }

  ml::text_options opt(face);
  AlignText(opt, graphics::EPosCenter);
  l.warp(base, opt);

  agg_symbol_renderer ren(m_baseRenderer, font.m_color, font.m_maskColor);
  if (font.m_isMasked)
  {
    ren.outline(true);
    l.render(face, ren);
    ren.outline(false);
  }
  l.render(face, ren);
}

void SoftwareRenderer::EndFrame(FrameImage & image)
{
  ASSERT(m_frameWidth > 0 && m_frameHeight > 0, ());

  image.m_stride = m_frameWidth;
  image.m_width = m_frameWidth;
  image.m_height = m_frameHeight;

  il::EncodePngToMemory(m_frameWidth, m_frameHeight, m_frameBuffer, image.m_data);
  m_frameWidth = 0;
  m_frameHeight = 0;
}

m2::RectD SoftwareRenderer::FrameRect() const
{
  return m2::RectD(0.0, 0.0, m_frameWidth, m_frameHeight);
}

////////////////////////////////////////////////////////////////////////////////

template <class VertexSource> class conv_count
{
public:
  conv_count(VertexSource & vs)
    : m_source(&vs)
    , m_count(0)
  {
  }

  void count(unsigned n) { m_count = n; }
  unsigned count() const { return m_count; }

  void rewind(unsigned path_id) { m_source->rewind(path_id); }
  unsigned vertex(double* x, double* y)
  {
    ++m_count;
    return m_source->vertex(x, y);
  }

private:
  VertexSource * m_source;
  unsigned m_count;
};

void PathWrapper::MoveTo(m2::PointD const & pt)
{
  m_storage.move_to(pt.x, pt.y);
}

void PathWrapper::LineTo(m2::PointD const & pt)
{
  m_storage.line_to(pt.x, pt.y);
}

void PathWrapper::CurveTo(m2::PointD const & pt1, m2::PointD const & pt2, m2::PointD const & ptTo)
{
  m_storage.curve4(pt1.x, pt1.y, pt2.x, pt2.y, ptTo.x, ptTo.y);
}

void PathWrapper::ClosePath()
{
  m_storage.end_poly(agg::path_flags_close);
}

void PathWrapper::AddParams(PathParams const & params)
{
  m_params.push_back(params);
}

struct DummyIDGetter
{
  unsigned operator [](unsigned idx) { return idx; }
};

void PathWrapper::BoundingRect(m2::RectD & rect)
{
  double minX = 0.0;
  double minY = 0.0;
  double maxX = 0.0;
  double maxY = 0.0;

  DummyIDGetter idGetter;
  agg::bounding_rect(m_storage, idGetter, 0, 1, &minX, &minY, &maxX, &maxY);
  rect = m2::RectD(minX, minY, maxX, maxY);
}

void PathWrapper::Render(SoftwareRenderer::TSolidRenderer & renderer,
                         agg::trans_affine const & mtx,
                         m2::RectD const & clipBox)
{
  using TCurved = agg::conv_curve<agg::path_storage>;
  using TConvCount = conv_count<TCurved>;
  using TConvStroke = agg::conv_stroke<TConvCount>;
  using TConvTransStroke = agg::conv_transform<TConvStroke>;
  using TConvTransCurved = agg::conv_transform<TConvCount>;

  agg::rasterizer_scanline_aa<> rasterizer;
  agg::scanline_p8 scanline;

  TCurved curved(m_storage);
  TConvCount curvedCount(curved);
  TConvStroke curvedStroke(curvedCount);
  TConvTransStroke transStroke(curvedStroke, mtx);
  TConvTransCurved transCurve(curvedCount, mtx);

  rasterizer.clip_box(clipBox.minX(), clipBox.minY(), clipBox.maxX(), clipBox.maxY());
  curvedCount.count(0);

  for (PathParams const & p : m_params)
  {
    double scl = mtx.scale();
    curved.approximation_scale(scl);
    curved.angle_tolerance(0.0);

    if(p.m_isFill)
    {
      rasterizer.reset();
      rasterizer.filling_rule(p.m_isEventOdd ? agg::fill_even_odd : agg::fill_non_zero);
      rasterizer.add_path(transCurve);
      renderer.color(p.m_fillColor);
      agg::render_scanlines(rasterizer, scanline, renderer);
    }

    if(p.m_isStroke)
    {
      curvedStroke.width(p.m_strokeWidth);
      curvedStroke.line_join(agg::round_join);
      curvedStroke.line_cap(agg::round_cap);
      curvedStroke.miter_limit(4.0);
      curvedStroke.inner_join(agg::inner_round);
      curvedStroke.approximation_scale(scl);
      curved.angle_tolerance(0.2);

      rasterizer.reset();
      rasterizer.filling_rule(agg::fill_non_zero);
      rasterizer.add_path(transStroke);
      renderer.color(p.m_strokeColor);
      agg::render_scanlines(rasterizer, scanline, renderer);
    }
  }
}
