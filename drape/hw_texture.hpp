#pragma once

#include "glconstants.hpp"
#include "pointers.hpp"
#include "drape_global.hpp"

#include <cstdint>

namespace dp
{
class HWTextureAllocator;

class HWTexture
{
public:
  HWTexture();
  virtual ~HWTexture();

  struct Params
  {
    Params()
      : m_filter(gl_const::GLLinear)
      , m_wrapSMode(gl_const::GLClampToEdge)
      , m_wrapTMode(gl_const::GLClampToEdge)
      , m_format(UNSPECIFIED)
      , m_usePixelBuffer(false)
    {}

    uint32_t m_width;
    uint32_t m_height;
    glConst m_filter;
    glConst m_wrapSMode;
    glConst m_wrapTMode;
    TextureFormat m_format;
    bool m_usePixelBuffer;

    ref_ptr<HWTextureAllocator> m_allocator;
  };

  void Create(Params const & params);
  virtual void Create(Params const & params, ref_ptr<void> data) = 0;
  virtual void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          ref_ptr<void> data) = 0;

  void Bind() const;

  // Texture must be bound before calling this method.
  void SetFilter(glConst filter);

  TextureFormat GetFormat() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

  uint32_t GetID() const;

protected:
  void UnpackFormat(TextureFormat format, glConst & layout, glConst & pixelType);

  uint32_t m_width;
  uint32_t m_height;
  TextureFormat m_format;
  uint32_t m_textureID;
  glConst m_filter;
  uint32_t m_pixelBufferID;
  uint32_t m_pixelBufferSize;
  uint32_t m_pixelBufferElementSize;
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
  ~OpenGLHWTexture();
  void Create(Params const & params, ref_ptr<void> data) override;
  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  ref_ptr<void> data) override;
};

class OpenGLHWTextureAllocator : public HWTextureAllocator
{
public:
  drape_ptr<HWTexture> CreateTexture() override;
  void Flush() override {}
};

ref_ptr<HWTextureAllocator> GetDefaultAllocator();
drape_ptr<HWTextureAllocator> CreateAllocator();
}  // namespace dp
