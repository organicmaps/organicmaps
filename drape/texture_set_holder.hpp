#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

class TextureSetHolder
{
public:
  virtual ~TextureSetHolder() {}
  virtual void Init(const string & resourcePrefix) = 0;
  virtual void Release() = 0;

  struct TextureRegion
  {
    m2::PointU m_pixelSize;
    m2::RectF  m_stRect;        // texture coodinates in range [0, 1]
    int32_t    m_textureSet;    // number of texture set where region placed
    int32_t    m_textureOffset; // number of texture in set
  };

  virtual void GetSymbolRegion(const string & symbolName, TextureRegion & region) const = 0;
};
