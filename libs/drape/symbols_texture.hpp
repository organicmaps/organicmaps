#pragma once

#include "drape/texture.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace dp
{
class SymbolsTexture : public Texture
{
public:
  class SymbolKey : public Key
  {
  public:
    explicit SymbolKey(std::string_view symbolName);
    ResourceType GetType() const override;
    std::string_view GetSymbolName() const;

  private:
    std::string_view m_symbolName;
  };

  class SymbolInfo : public ResourceInfo
  {
  public:
    explicit SymbolInfo(m2::RectF const & texRect);
    ResourceType GetType() const override;
  };

  SymbolsTexture(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                 std::string const & textureName, ref_ptr<HWTextureAllocator> allocator);

  ref_ptr<ResourceInfo> FindResource(Key const & key, bool & newResource) override;

  void Invalidate(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
                  ref_ptr<HWTextureAllocator> allocator);

  bool IsSymbolContained(std::string const & symbolName) const;

  static bool DecodeToMemory(std::string const & skinPathName, std::string const & textureName,
                             std::vector<uint8_t> & symbolsSkin, std::map<std::string, m2::RectU> & symbolsIndex,
                             uint32_t & skinWidth, uint32_t & skinHeight);

private:
  void Fail(ref_ptr<dp::GraphicsContext> context);
  void Load(ref_ptr<dp::GraphicsContext> context, std::string const & skinPathName,
            ref_ptr<HWTextureAllocator> allocator);

  using TSymDefinition = std::map<std::string, SymbolInfo, std::less<>>;
  std::string m_name;
  mutable TSymDefinition m_definition;
};
}  // namespace dp
