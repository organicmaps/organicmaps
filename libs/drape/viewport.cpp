#include "drape/viewport.hpp"
#include "drape/gl_functions.hpp"

namespace dp
{
Viewport::Viewport(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
  : m_zero(x0, y0)
  , m_size(w, h)
{}

void Viewport::SetViewport(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
{
  m_zero = m2::PointU(x0, y0);
  m_size = m2::PointU(w, h);
}

uint32_t Viewport::GetX0() const { return m_zero.x; }

uint32_t Viewport::GetY0() const { return m_zero.y; }

uint32_t Viewport::GetWidth() const { return m_size.x; }

uint32_t Viewport::GetHeight() const { return m_size.y; }

void Viewport::Apply(ref_ptr<dp::GraphicsContext> context) const
{
  context->SetViewport(GetX0(), GetY0(), GetWidth(), GetHeight());
}
}  // namespace dp
