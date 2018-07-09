#pragma once

#include "drape/drape_global.hpp"
#include "drape/glconstants.hpp"
#include "drape/hw_texture.hpp"
#include "drape/pointers.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"

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
    Static
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
  virtual bool HasEnoughSpace(uint32_t /*newKeysCount*/) const { return true; }
  using Params = HWTexture::Params;

  virtual TextureFormat GetFormat() const;
  virtual uint32_t GetWidth() const;
  virtual uint32_t GetHeight() const;
  virtual float GetS(uint32_t x) const;
  virtual float GetT(uint32_t y) const;
  virtual uint32_t GetID() const;

  virtual void Bind() const;

  // Texture must be bound before calling this method.
  virtual void SetFilter(glConst filter);

  void Create(Params const & params);
  void Create(Params const & params, ref_ptr<void> data);
  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data);

  static uint32_t GetMaxTextureSize();

protected:
  void Destroy();
  bool AllocateTexture(ref_ptr<HWTextureAllocator> allocator);

  drape_ptr<HWTexture> m_hwTexture;

  DISALLOW_COPY_AND_MOVE(Texture);
};
}  // namespace dp
