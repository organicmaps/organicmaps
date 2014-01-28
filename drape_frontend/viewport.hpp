#pragma once

#include "../geometry/point2d.hpp"

namespace df
{
  // Pixel ratio independent viewport implementation
  // pixelRatio is ratio between physical pixels and device-independent
  // pixels for the window. On retina displays pixelRation equal 2.0, In common equal 1.0
  class Viewport
  {
  public:
    // x0, y0, w, h is device-independent pixels
    Viewport(float pixelRatio,
             uint32_t x0, uint32_t y0,
             uint32_t w, uint32_t h);

    ///@{ Device-independent pixels
    void SetViewport(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h);
    uint32_t GetX0() const;
    uint32_t GetY0() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    ///@}

    // Apply viewport to graphics pipeline
    // with convert start poin and size to physical pixels
    void Apply() const;

  private:
    float m_pixelRatio;
    m2::PointU m_zero;
    m2::PointU m_size;
  };
}
