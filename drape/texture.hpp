#pragma once

#include "pointers.hpp"
#include "glconstants.hpp"

#include "../geometry/rect2d.hpp"

#include "../std/stdint.hpp"

class Texture
{
public:
  enum TextureFormat
  {
    RGBA8,
    RGBA4,
    ALPHA
  };

  class Key
  {
  public:
    enum Type
    {
      Symbol,
      Font,
      UniformValue
    };

    virtual Type GetType() const = 0;
  };

  Texture();
  virtual ~Texture();

  void Create(uint32_t width, uint32_t height, TextureFormat format);
  void Create(uint32_t width, uint32_t height, TextureFormat format, RefPointer<void> data);
  void SetFilterParams(glConst minFilter, glConst magFilter);
  void SetWrapMode(glConst sMode, glConst tMode);

  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, TextureFormat format,
                  RefPointer<void> data);

  virtual bool FindResource(Key const & key, m2::RectF & texRect, m2::PointU & pixelSize) const = 0;

  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

  void Bind() const;

private:
  void UnpackFormat(TextureFormat format, glConst & layout, glConst & pixelType);
  int32_t GetID() const;

private:
  int32_t m_textureID;
  uint32_t m_width;
  uint32_t m_height;
};
