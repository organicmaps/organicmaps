#include "drape/debug_rect_renderer.hpp"

#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"

#include <vector>

namespace dp
{
namespace
{
void PixelPointToScreenSpace(ScreenBase const & screen, m2::PointF const & pt, std::vector<float> & buffer)
{
  auto const szX = static_cast<float>(screen.PixelRectIn3d().SizeX());
  auto const szY = static_cast<float>(screen.PixelRectIn3d().SizeY());

  buffer.push_back(2.0f * (pt.x / szX - 0.5f));
  buffer.push_back(2.0f * (-pt.y / szY + 0.5f));
}
}  // namespace

DebugRectRenderer & DebugRectRenderer::Instance()
{
  static DebugRectRenderer renderer;
  return renderer;
}

DebugRectRenderer::DebugRectRenderer()
  : TBase(DrawPrimitive::LineStrip)
{
  SetBuffer(0 /*bufferInd*/, {} /* vertices */, static_cast<uint32_t>(sizeof(float) * 2));
  SetAttribute("a_position", 0 /* bufferInd*/, 0.0f /* offset */, 2 /* componentsCount */);
}

DebugRectRenderer::~DebugRectRenderer()
{
  ASSERT(!IsInitialized(), ());
}

void DebugRectRenderer::Init(ref_ptr<dp::GpuProgram> program, ParamsSetter && paramsSetter)
{
  m_program = program;
  m_paramsSetter = std::move(paramsSetter);
}

void DebugRectRenderer::Destroy()
{
  Reset();
}

bool DebugRectRenderer::IsEnabled() const
{
  return m_isEnabled;
}

void DebugRectRenderer::SetEnabled(bool enabled)
{
  m_isEnabled = enabled;
}

void DebugRectRenderer::SetArrow(m2::PointF const & arrowStart, m2::PointF const & arrowEnd,
                                 dp::Color const & arrowColor, ScreenBase const & screen)
{
  std::vector<float> vertices;
  m2::PointF const dir = (arrowEnd - arrowStart).Normalize();
  m2::PointF const side = m2::PointF(-dir.y, dir.x);
  PixelPointToScreenSpace(screen, arrowStart, vertices);
  PixelPointToScreenSpace(screen, arrowEnd, vertices);
  PixelPointToScreenSpace(screen, arrowEnd - dir * 20 + side * 10, vertices);
  PixelPointToScreenSpace(screen, arrowEnd, vertices);
  PixelPointToScreenSpace(screen, arrowEnd - dir * 20 - side * 10, vertices);

  UpdateBuffer(0 /* bufferInd */, std::move(vertices));
}

void DebugRectRenderer::SetRect(m2::RectF const & rect, ScreenBase const & screen)
{
  std::vector<float> vertices;
  PixelPointToScreenSpace(screen, rect.LeftBottom(), vertices);
  PixelPointToScreenSpace(screen, rect.LeftTop(), vertices);
  PixelPointToScreenSpace(screen, rect.RightTop(), vertices);
  PixelPointToScreenSpace(screen, rect.RightBottom(), vertices);
  PixelPointToScreenSpace(screen, rect.LeftBottom(), vertices);

  UpdateBuffer(0 /* bufferInd */, std::move(vertices));
}

void DebugRectRenderer::DrawRect(ScreenBase const & screen, m2::RectF const & rect,
                                 dp::Color const & color)
{
  if (!m_isEnabled)
    return;

  SetRect(rect, screen);

  auto const preRenderFn = [this, color]()
  {
    if (m_paramsSetter)
      m_paramsSetter(m_program, color);
  };
  Render(m_program, preRenderFn, nullptr);
}

void DebugRectRenderer::DrawArrow(ScreenBase const & screen,
                                  OverlayTree::DisplacementData const & data)
{
  if (!m_isEnabled)
    return;

  if (data.m_arrowStart.EqualDxDy(data.m_arrowEnd, 1e-5))
    return;

  SetArrow(data.m_arrowStart, data.m_arrowEnd, data.m_arrowColor, screen);

  auto const preRenderFn = [this, data]()
  {
    if (m_paramsSetter)
      m_paramsSetter(m_program, data.m_arrowColor);
  };
  Render(m_program, preRenderFn, nullptr);
}
}  // namespace dp
