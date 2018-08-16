#pragma once

#include "drape/glconstants.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_types.hpp"

#include <cstdint>

namespace dp
{
class HWTextureAllocator;

class HWTexture
{
public:
  virtual ~HWTexture();

  struct Params
  {
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    TextureFilter m_filter = TextureFilter::Linear;
    TextureWrapping m_wrapSMode = TextureWrapping::ClampToEdge;
    TextureWrapping m_wrapTMode = TextureWrapping::ClampToEdge;
    TextureFormat m_format = TextureFormat::Unspecified;
    bool m_usePixelBuffer = false;

    ref_ptr<HWTextureAllocator> m_allocator;
  };

  void Create(Params const & params);
  virtual void Create(Params const & params, ref_ptr<void> data) = 0;
  virtual void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          ref_ptr<void> data) = 0;

  void Bind() const;

  // Texture must be bound before calling this method.
  void SetFilter(TextureFilter filter);

  TextureFormat GetFormat() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

  uint32_t GetID() const;

protected:
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  TextureFormat m_format = TextureFormat::Unspecified;
  uint32_t m_textureID = 0;
  TextureFilter m_filter = TextureFilter::Linear;
  uint32_t m_pixelBufferID = 0;
  uint32_t m_pixelBufferSize = 0;
  uint32_t m_pixelBufferElementSize = 0;
};

class HWTextureAllocator
{
public:
  virtual ~HWTextureAllocator() = default;

  virtual drape_ptr<HWTexture> CreateTexture() = 0;
  virtual void Flush() = 0;
};

class OpenGLHWTexture : public HWTexture
{
  using Base = HWTexture;

public:
  ~OpenGLHWTexture() override;
  void Create(Params const & params, ref_ptr<void> data) override;
  void UploadData(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  ref_ptr<void> data) override;
};

class OpenGLHWTextureAllocator : public HWTextureAllocator
{
public:
  drape_ptr<HWTexture> CreateTexture() override;
  void Flush() override;
};

ref_ptr<HWTextureAllocator> GetDefaultAllocator();
drape_ptr<HWTextureAllocator> CreateAllocator();

void UnpackFormat(TextureFormat format, glConst & layout, glConst & pixelType);
glConst DecodeTextureFilter(TextureFilter filter);
glConst DecodeTextureWrapping(TextureWrapping wrapping);
}  // namespace dp
