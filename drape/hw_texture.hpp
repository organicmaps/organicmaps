#pragma once

#include "drape/gl_constants.hpp"
#include "drape/graphics_context.hpp"
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
    bool m_isRenderTarget = false;
    bool m_isMutable = false;

    ref_ptr<HWTextureAllocator> m_allocator;

    friend std::string DebugPrint(Params const & p);
  };

  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params);

  virtual void Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data) = 0;
  virtual void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          ref_ptr<void> data) = 0;

  virtual void Bind(ref_ptr<dp::GraphicsContext> context) const = 0;

  // For OpenGL the texture must be bound before calling this method.
  virtual void SetFilter(TextureFilter filter) = 0;

  virtual bool Validate() const = 0;

  TextureFormat GetFormat() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;
  float GetS(uint32_t x) const;
  float GetT(uint32_t y) const;

  Params const & GetParams() const { return m_params; }

  uint32_t GetID() const;

protected:
  Params m_params;
  uint32_t m_textureID = 0;
};

class HWTextureAllocator
{
public:
  virtual ~HWTextureAllocator() = default;

  virtual drape_ptr<HWTexture> CreateTexture(ref_ptr<dp::GraphicsContext> context) = 0;
  virtual void Flush() = 0;
};

class OpenGLHWTexture : public HWTexture
{
  using Base = HWTexture;

public:
  ~OpenGLHWTexture() override;
  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data) override;
  void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  ref_ptr<void> data) override;
  void Bind(ref_ptr<dp::GraphicsContext> context) const override;
  void SetFilter(TextureFilter filter) override;
  bool Validate() const override;

private:
  uint32_t m_pixelBufferID = 0;
  uint32_t m_pixelBufferSize = 0;
  uint8_t m_pixelBufferElementSize = 0;

  glConst m_unpackedLayout = 0;
  glConst m_unpackedPixelType = 0;
};

class OpenGLHWTextureAllocator : public HWTextureAllocator
{
public:
  drape_ptr<HWTexture> CreateTexture(ref_ptr<dp::GraphicsContext> context) override;
  void Flush() override;
};

ref_ptr<HWTextureAllocator> GetDefaultAllocator(ref_ptr<dp::GraphicsContext> context);
drape_ptr<HWTextureAllocator> CreateAllocator(ref_ptr<dp::GraphicsContext> context);

void UnpackFormat(ref_ptr<dp::GraphicsContext> context, TextureFormat format, glConst & layout, glConst & pixelType);
glConst DecodeTextureFilter(TextureFilter filter);
glConst DecodeTextureWrapping(TextureWrapping wrapping);
}  // namespace dp
