#pragma once

#include "drape/pointers.hpp"
#include "drape/glconstants.hpp"
#include "drape/drape_global.hpp"
#include "drape/hw_texture.hpp"

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

  virtual ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) = 0;
  virtual void UpdateState() {}
  virtual bool HasAsyncRoutines() const { return false; }

  using Params = HWTexture::Params;

  void Create(Params const & params);
  void Create(Params const & params, ref_ptr<void> data);

  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data);

  TextureFormat GetFormat() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

  void Bind() const;

  static uint32_t GetMaxTextureSize();

protected:
  void Destroy();
  bool AllocateTexture(ref_ptr<HWTextureAllocator> allocator);

private:
  drape_ptr<HWTexture> m_hwTexture;
};

} // namespace dp
