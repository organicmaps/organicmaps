#pragma once

#include "drape/color.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/font_texture.hpp"

#include "base/string_utils.hpp"

#include "std/atomic.hpp"
#include "std/unordered_set.hpp"

namespace dp
{

class TextureManager
{
public:
  class BaseRegion
  {
  public:
    void SetResourceInfo(RefPointer<Texture::ResourceInfo> info);
    void SetTexture(RefPointer<Texture> texture);
    RefPointer<Texture> GetTexture() const { return m_texture; }
    bool IsValid() const;

    m2::PointU GetPixelSize() const;
    uint32_t GetPixelHeight() const;
    m2::RectF const & GetTexRect() const;

  protected:
    RefPointer<Texture::ResourceInfo> m_info;
    RefPointer<Texture> m_texture;
  };

  class SymbolRegion : public BaseRegion {};

  class GlyphRegion : public BaseRegion
  {
  public:
    GlyphRegion();

    float GetOffsetX() const;
    float GetOffsetY() const;
    float GetAdvanceX() const;
    float GetAdvanceY() const;
  };

  class StippleRegion : public BaseRegion
  {
  public:
    StippleRegion() : BaseRegion() {}

    uint32_t GetMaskPixelLength() const;
    uint32_t GetPatternPixelLength() const;
  };

  class ColorRegion : public BaseRegion
  {
  public:
    ColorRegion() : BaseRegion() {}
  };

  struct Params
  {
    string m_resPrefix;
    GlyphManager::Params m_glyphMngParams;
  };

  TextureManager();

  void Init(Params const & params);
  void Release();
  void GetSymbolRegion(string const & symbolName, SymbolRegion & region);

  typedef buffer_vector<uint8_t, 8> TStipplePattern;
  void GetStippleRegion(TStipplePattern const & pen, StippleRegion & region);
  void GetColorRegion(Color const & color, ColorRegion & region);

  typedef buffer_vector<strings::UniString, 4> TMultilineText;
  typedef buffer_vector<GlyphRegion, 128> TGlyphsBuffer;
  typedef buffer_vector<TGlyphsBuffer, 4> TMultilineGlyphsBuffer;

  void GetGlyphRegions(TMultilineText const & text, TMultilineGlyphsBuffer & buffers);
  void GetGlyphRegions(strings::UniString const & text, TGlyphsBuffer & regions);

  /// On some devices OpenGL driver can't resolve situation when we upload on texture from one thread
  /// and use this texture to render on other thread. By this we move UpdateDynamicTextures call into render thread
  /// If you implement some kind of dynamic texture, you must synchronyze UploadData and index creation operations
  void UpdateDynamicTextures();

private:
  struct GlyphGroup
  {
    GlyphGroup() = default;
    GlyphGroup(strings::UniChar const & start, strings::UniChar const & end)
      : m_startChar(start), m_endChar(end) {}

    strings::UniChar m_startChar = 0;
    strings::UniChar m_endChar = 0;

    MasterPointer<Texture> m_texture;
  };

  struct HybridGlyphGroup
  {
    unordered_set<strings::UniChar> m_glyphs;
    MasterPointer<Texture> m_texture;
  };

  uint32_t m_maxTextureSize;
  uint32_t m_maxGlypsCount;

  MasterPointer<Texture> AllocateGlyphTexture() const;
  void GetRegionBase(RefPointer<Texture> tex, TextureManager::BaseRegion & region, Texture::Key const & key);

  size_t FindGlyphsGroup(strings::UniChar const & c) const;
  size_t FindGlyphsGroup(strings::UniString const & text) const;
  size_t FindGlyphsGroup(TMultilineText const & text) const;

  size_t FindHybridGlyphsGroup(strings::UniString const & text);
  size_t FindHybridGlyphsGroup(TMultilineText const & text);

  size_t GetNumberOfUnfoundCharacters(strings::UniString const & text, HybridGlyphGroup const & group) const;

  void MarkCharactersUsage(strings::UniString const & text, HybridGlyphGroup & group);

  template<typename TGlyphGroup>
  void FillResultBuffer(strings::UniString const & text, TGlyphGroup & group, TGlyphsBuffer & regions)
  {
    if (group.m_texture.IsNull())
      group.m_texture = AllocateGlyphTexture();

    dp::RefPointer<dp::Texture> texture = group.m_texture.GetRefPointer();
    regions.reserve(text.size());
    for (strings::UniChar const & c : text)
    {
      GlyphRegion reg;
      GetRegionBase(texture, reg, GlyphKey(c));
      regions.push_back(reg);
    }
  }

  static constexpr size_t GetInvalidGlyphGroup();

  template<typename TText, typename TBuffer>
  void CalcGlyphRegions(TText const & text, TBuffer & buffers,
                        function<void(TText const &, TBuffer &, GlyphGroup &)> callback,
                        function<void(TText const &, TBuffer &, HybridGlyphGroup &)> hybridCallback)
  {
    size_t const groupIndex = FindGlyphsGroup(text);
    if (groupIndex != GetInvalidGlyphGroup())
    {
      GlyphGroup & group = m_glyphGroups[groupIndex];
      if (callback != nullptr)
        callback(text, buffers, group);
    }
    else
    {
      size_t const hybridGroupIndex = FindHybridGlyphsGroup(text);
      ASSERT(hybridGroupIndex != GetInvalidGlyphGroup(), ());
      HybridGlyphGroup & group = m_hybridGlyphGroups[hybridGroupIndex];
      if (hybridCallback != nullptr)
        hybridCallback(text, buffers, group);
    }
  }

private:
  MasterPointer<Texture> m_symbolTexture;
  MasterPointer<Texture> m_stipplePenTexture;
  MasterPointer<Texture> m_colorTexture;

  MasterPointer<GlyphManager> m_glyphManager;

  buffer_vector<GlyphGroup, 64> m_glyphGroups;
  buffer_vector<HybridGlyphGroup, 4> m_hybridGlyphGroups;

  atomic_flag m_nothingToUpload;
};

} // namespace dp
