#pragma once

#include "drape/pointers.hpp"
#include "drape/glconstants.hpp"
#include "drape/drape_global.hpp"

#include "geometry/rect2d.hpp"

#include "std/cstdint.hpp"
#include "std/function.hpp"

namespace dp
{

class Texture
{
public:
  enum ResourceType
  {
    Symbol,
    Glyph,
    StipplePen,
    Color,
    UniformValue
  };

  class Key
  {
  public:
    virtual ~Key() {}
    virtual ResourceType GetType() const = 0;
  };

  class ResourceInfo
  {
  public:
    ResourceInfo(m2::RectF const & texRect);
    virtual ~ResourceInfo() {}

    virtual ResourceType GetType() const = 0;
    m2::RectF const & GetTexRect() const;

  private:
    m2::RectF m_texRect;
  };

  Texture();
  virtual ~Texture();

  void Create(uint32_t width, uint32_t height, TextureFormat format);
  void Create(uint32_t width, uint32_t height, TextureFormat format, RefPointer<void> data);
  void SetFilterParams(glConst minFilter, glConst magFilter);
  void SetWrapMode(glConst sMode, glConst tMode);

  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, TextureFormat format,
                  RefPointer<void> data);

  virtual RefPointer<ResourceInfo> FindResource(Key const & key, bool & newResource) = 0;
  virtual void UpdateState() {}

  TextureFormat GetFormat() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

  void Bind() const;

  static uint32_t GetMaxTextureSize();

private:
  void UnpackFormat(TextureFormat format, glConst & layout, glConst & pixelType);
  int32_t GetID() const;

private:
  int32_t m_textureID;
  uint32_t m_width;
  uint32_t m_height;
  TextureFormat m_format;
};

} // namespace dp
