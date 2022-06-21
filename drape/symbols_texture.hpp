#pragma once

#include "drape/dynamic_texture.hpp"
#include "drape/rect_packer.hpp"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace dp
{
class SymbolKey : public Texture::Key
{
public:
  explicit SymbolKey(std::string const & symbolName) : m_symbolName(symbolName) {}
  Texture::ResourceType GetType() const override { return Texture::ResourceType::Symbol; }
  std::string const & GetSymbolName() const { return m_symbolName; }

private:
  std::string m_symbolName;
};

class SymbolInfo : public Texture::ResourceInfo
{
public:
  explicit SymbolInfo(m2::RectF const & texRect) : Texture::ResourceInfo(texRect) {}
  Texture::ResourceType GetType() const override { return Texture::ResourceType::Symbol; }
};

class SymbolsTexture : public Texture
{
public:
  SymbolsTexture(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                 std::string const & textureName, ref_ptr<HWTextureAllocator> allocator);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;

  void Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                  ref_ptr<HWTextureAllocator> allocator);
  void Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                  ref_ptr<HWTextureAllocator> allocator,
                  std::vector<drape_ptr<HWTexture>> & internalTextures);

  static bool DecodeToMemory(std::string const & skinPathName, std::string const & textureName,
                             std::vector<uint8_t> & symbolsSkin,
                             std::map<std::string, m2::RectU> & symbolsIndex,
                             uint32_t & skinWidth, uint32_t & skinHeight);
private:
  void Fail(ref_ptr<dp::GraphicsContext> context);
  void Load(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
            ref_ptr<HWTextureAllocator> allocator);

  std::string m_name;
  std::map<std::string, SymbolInfo> m_definition;
};

class LoadedSymbol
{
  DISALLOW_COPY(LoadedSymbol);

public:
  LoadedSymbol() = default;
  LoadedSymbol(LoadedSymbol && rhs)
    : m_data(rhs.m_data), m_width(rhs.m_width), m_height(rhs.m_height)
  {
    rhs.m_data = nullptr;
  }
  ~LoadedSymbol()
  {
    Free();
  }

  bool FromPngFile(std::string const & filePath);
  bool IsValid() const { return m_data != nullptr; }
  void Free();

  uint8_t * m_data = nullptr;
  int m_width, m_height;
};

class SymbolsIndex
{
public:
  explicit SymbolsIndex(m2::PointU const & size) : m_packer(size) {}

  ref_ptr<Texture::ResourceInfo> MapResource(SymbolKey const & key, bool & newResource);
  void UploadResources(ref_ptr<dp::GraphicsContext> context, ref_ptr<Texture> texture);

private:
  RectPacker m_packer;

  std::map<std::string, SymbolInfo> m_index;

  using TPendingNodes = std::vector<std::pair<m2::RectU, LoadedSymbol>>;
  TPendingNodes m_pendingNodes;

  std::mutex m_upload, m_mapping;
};

class DynamicSymbolsTexture : public DynamicTexture<SymbolsIndex, SymbolKey, Texture::ResourceType::Symbol>
{
  using TBase = DynamicTexture<SymbolsIndex, SymbolKey, Texture::ResourceType::Symbol>;

public:
  DynamicSymbolsTexture(m2::PointU const & size, ref_ptr<HWTextureAllocator> allocator)
    : m_index(size)
  {
    TBase::DynamicTextureParams params{size, TextureFormat::RGBA8, TextureFilter::Nearest, false /* m_usePixelBuffer */};
    TBase::Init(allocator, make_ref(&m_index), params);
  }

  ~DynamicSymbolsTexture() override { TBase::Reset(); }

private:
  SymbolsIndex m_index;
};

}  // namespace dp
