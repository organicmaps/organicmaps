#pragma once

#include "glconstants.hpp"
#include "pointers.hpp"
#include "drape_global.hpp"

#include "std/cstdint.hpp"

namespace dp
{

class HWTextureAllocator;
class HWTexture
{
public:
  HWTexture();
  virtual ~HWTexture() {}

  struct Params
  {
    Params()
      : m_minFilter(gl_const::GLLinear)
      , m_magFilter(gl_const::GLLinear)
      , m_wrapSMode(gl_const::GLClampToEdge)
      , m_wrapTMode(gl_const::GLClampToEdge)
      , m_format(UNSPECIFIED)
    {
    }

    uint32_t m_width;
    uint32_t m_height;
    glConst m_minFilter;
    glConst m_magFilter;
    glConst m_wrapSMode;
    glConst m_wrapTMode;
    TextureFormat m_format;

    ref_ptr<HWTextureAllocator> m_allocator;
  };

  void Create(Params const & params);
  virtual void Create(Params const & params, ref_ptr<void> data) = 0;
  virtual void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data) = 0;

  void Bind() const;

  TextureFormat GetFormat() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

protected:
  void UnpackFormat(TextureFormat format, glConst & layout, glConst & pixelType);
  int32_t GetID() const;

  uint32_t m_width;
  uint32_t m_height;
  TextureFormat m_format;
  int32_t m_textureID;
};

class HWTextureAllocator
{
public:
  virtual ~HWTextureAllocator() {}

  virtual drape_ptr<HWTexture> CreateTexture() = 0;
  virtual void Flush() = 0;
};

class OpenGLHWTexture : public HWTexture
{
  using TBase = HWTexture;

public:
  void Create(Params const & params, ref_ptr<void> data) override;
  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height, ref_ptr<void> data) override;
};

class OpenGLHWTextureAllocator : public HWTextureAllocator
{
public:
  drape_ptr<HWTexture> CreateTexture() override;
  void Flush() override {}
};

ref_ptr<HWTextureAllocator> GetDefaultAllocator();
drape_ptr<HWTextureAllocator> CreateAllocator();

}
