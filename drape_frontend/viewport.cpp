#include "viewport.hpp"
#include "../drape/glfunctions.hpp"

namespace df
{

Viewport::Viewport(float pixelRatio,
                   uint32_t x0, uint32_t y0,
                   uint32_t w, uint32_t h)
  : m_pixelRatio(pixelRatio)
  , m_zero(x0, y0)
  , m_size(w, h)
{
}

void Viewport::SetViewport(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h)
{
  m_zero = m2::PointU(x0 ,y0);
  m_size = m2::PointU(w, h);
}

uint32_t Viewport::GetX0() const
{
  return m_zero.x * m_pixelRatio;
}

uint32_t Viewport::GetY0() const
{
  return m_zero.y * m_pixelRatio;
}

uint32_t Viewport::GetWidth() const
{
  return m_size.x * m_pixelRatio;
}

uint32_t Viewport::GetHeight() const
{
  return m_size.y * m_pixelRatio;
}

void Viewport::Apply() const
{
  GLFunctions::glViewport(m_zero.x * m_pixelRatio, m_zero.y * m_pixelRatio,
                          m_size.x * m_pixelRatio, m_size.y * m_pixelRatio);
}

} // namespace df
