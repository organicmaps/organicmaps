#pragma once

#include "drape/gl_constants.hpp"
#include "drape/graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_types.hpp"

#include <cstdint>
#include <vector>

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
    uint32_t m_layerCount = 1;
    TextureFilter m_filter = TextureFilter::Linear;
    TextureWrapping m_wrapSMode = TextureWrapping::ClampToEdge;
    TextureWrapping m_wrapTMode = TextureWrapping::ClampToEdge;
    TextureFormat m_format = TextureFormat::Unspecified;
    // Build a full mip chain and sample it trilinearly. Only for standalone, self-contained textures
    // (e.g. the hatching masks) - never for packed atlases, where coarse levels bleed across entries.
    bool m_useMipmaps = false;
    bool m_usePixelBuffer = false;
    bool m_isRenderTarget = false;
    bool m_isMutable = false;
    bool m_usePersistentStagingBuffer = false;

    ref_ptr<HWTextureAllocator> m_allocator;

    friend std::string DebugPrint(Params const & p);
  };

  void Create(ref_ptr<dp::GraphicsContext> context, Params const & params);

  virtual void Create(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<void> data) = 0;
  virtual void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          ref_ptr<void> data) = 0;
  virtual void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                          uint32_t layer, ref_ptr<void> data) = 0;

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
  uint32_t GetTarget() const;

protected:
  Params m_params;
  uint32_t m_textureID = 0;
  uint32_t m_target = 0;
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
  void UploadData(ref_ptr<dp::GraphicsContext> context, uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                  uint32_t layer, ref_ptr<void> data) override;
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

// Whether a CPU mip chain should be built for this texture: only standalone, single-layer textures that
// are uploaded with data. Shared by every backend so they can't disagree on what "mipmapped" means.
bool CanBuildMipmaps(HWTexture::Params const & params, bool hasData);

// Number of mip levels in a full chain for a width x height texture (down to 1x1), e.g. 16x16 -> 5.
uint32_t GetMipLevelsCount(uint32_t width, uint32_t height);

// One generated mip level: its dimensions and pixel data.
struct MipLevel
{
  uint32_t m_width;
  uint32_t m_height;
  std::vector<uint8_t> m_data;
};

// Box-downsamples power-of-two level-0 data into mip levels 1..N-1 (index 0 is mip level 1). For 4-byte
// RGBA the colour is alpha-weighted so fully transparent texels don't bleed their RGB into edges; other
// formats use a plain average.
std::vector<MipLevel> BuildMipmapLevels(uint8_t const * level0Data, uint32_t width, uint32_t height,
                                        uint8_t bytesPerPixel);
}  // namespace dp
