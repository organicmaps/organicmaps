#include "drape_frontend/viewport.hpp"
#include "drape/glfunctions.hpp"

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

uint32_t Viewport::GetLogicX0() const
{
  return m_zero.x;
}

uint32_t Viewport::GetLogicY0() const
{
  return m_zero.y;
}

uint32_t Viewport::GetLogicWidth() const
{
  return m_size.x;
}

uint32_t Viewport::GetLogicHeight() const
{
  return m_size.y;
}

uint32_t Viewport::GetX0() const
{
  return GetLogicX0() * m_pixelRatio;
}

uint32_t Viewport::GetY0() const
{
  return GetLogicY0() * m_pixelRatio;
}

uint32_t Viewport::GetWidth() const
{
  return GetLogicWidth() * m_pixelRatio;
}

uint32_t Viewport::GetHeight() const
{
  return GetLogicHeight() * m_pixelRatio;
}

float Viewport::GetPixelRatio() const
{
  return m_pixelRatio;
}

void Viewport::Apply() const
{
  GLFunctions::glViewport(GetX0(), GetY0(), GetWidth(), GetHeight());
}

} // namespace df
