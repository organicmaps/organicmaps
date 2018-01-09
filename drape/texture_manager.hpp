#pragma once

#include "drape/color.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/pointers.hpp"
#include "drape/texture.hpp"
#include "drape/font_texture.hpp"

#include "base/string_utils.hpp"

#include <atomic>
#include <list>
#include <mutex>
#include <string>
#include <vector>

namespace dp
{
extern std::string const kDefaultSymbolsTexture;

class HWTextureAllocator;

class TextureManager
{
public:
  class BaseRegion
  {
  public:
    BaseRegion();
    void SetResourceInfo(ref_ptr<Texture::ResourceInfo> info);
    void SetTexture(ref_ptr<Texture> texture);
    ref_ptr<Texture> GetTexture() const { return m_texture; }
    bool IsValid() const;

    m2::PointF GetPixelSize() const;
    float GetPixelHeight() const;
    m2::RectF const & GetTexRect() const;

  protected:
    ref_ptr<Texture::ResourceInfo> m_info;
    ref_ptr<Texture> m_texture;
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
    std::string m_resPostfix;
    double m_visualScale;
    std::string m_colors;
    std::string m_patterns;
    GlyphManager::Params m_glyphMngParams;
  };

  TextureManager();
  void Release();

  void Init(Params const & params);
  void OnSwitchMapStyle();

  void GetSymbolRegion(std::string const & symbolName, SymbolRegion & region);

  typedef buffer_vector<uint8_t, 8> TStipplePattern;
  void GetStippleRegion(TStipplePattern const & pen, StippleRegion & region);
  void GetColorRegion(Color const & color, ColorRegion & region);

  using TMultilineText = buffer_vector<strings::UniString, 4>;
  using TGlyphsBuffer = buffer_vector<GlyphRegion, 128>;
  using TMultilineGlyphsBuffer = buffer_vector<TGlyphsBuffer, 4>;

  void GetGlyphRegions(TMultilineText const & text, int fixedHeight, TMultilineGlyphsBuffer & buffers);
  void GetGlyphRegions(strings::UniString const & text, int fixedHeight, TGlyphsBuffer & regions);
  // This method must be called only on Frontend renderer's thread.
  bool AreGlyphsReady(strings::UniString const & str, int fixedHeight) const;

  // On some devices OpenGL driver can't resolve situation when we upload to a texture on a thread
  // and use this texture to render on another thread. By this we move UpdateDynamicTextures call
  // into render thread. If you implement some kind of dynamic texture, you must synchronize UploadData
  // and index creation operations.
  bool UpdateDynamicTextures();

  ref_ptr<Texture> GetSymbolsTexture() const;
  ref_ptr<Texture> GetTrafficArrowTexture() const;
  ref_ptr<Texture> GetHatchingTexture() const;
  ref_ptr<Texture> GetSMAAAreaTexture() const;
  ref_ptr<Texture> GetSMAASearchTexture() const;

private:
  struct GlyphGroup
  {
    GlyphGroup()
      : m_startChar(0), m_endChar(0), m_texture(nullptr)
    {}

    GlyphGroup(strings::UniChar const & start, strings::UniChar const & end)
      : m_startChar(start), m_endChar(end), m_texture(nullptr)
    {}

    strings::UniChar m_startChar;
    strings::UniChar m_endChar;
    ref_ptr<Texture> m_texture;
  };

  struct HybridGlyphGroup
  {
    HybridGlyphGroup()
      : m_texture(nullptr)
    {}

    std::set<std::pair<strings::UniChar, int>> m_glyphs;
    ref_ptr<Texture> m_texture;
  };

  uint32_t m_maxTextureSize;
  uint32_t m_maxGlypsCount;

  ref_ptr<Texture> AllocateGlyphTexture();
  void GetRegionBase(ref_ptr<Texture> tex, TextureManager::BaseRegion & region, Texture::Key const & key);

  void GetGlyphsRegions(ref_ptr<FontTexture> tex, strings::UniString const & text,
                        int fixedHeight, TGlyphsBuffer & regions);

  size_t FindGlyphsGroup(strings::UniChar const & c) const;
  size_t FindGlyphsGroup(strings::UniString const & text) const;
  size_t FindGlyphsGroup(TMultilineText const & text) const;

  size_t FindHybridGlyphsGroup(strings::UniString const & text, int fixedHeight);
  size_t FindHybridGlyphsGroup(TMultilineText const & text, int fixedHeight);

  uint32_t GetNumberOfUnfoundCharacters(strings::UniString const & text, int fixedHeight,
                                        HybridGlyphGroup const & group) const;

  void MarkCharactersUsage(strings::UniString const & text, int fixedHeight, HybridGlyphGroup & group);
  // It's a dummy method to support generic code.
  void MarkCharactersUsage(strings::UniString const & text, int fixedHeight, GlyphGroup & group) {}

  template<typename TGlyphGroup>
  void FillResultBuffer(strings::UniString const & text, int fixedHeight, TGlyphGroup & group,
                        TGlyphsBuffer & regions)
  {
    if (group.m_texture == nullptr)
      group.m_texture = AllocateGlyphTexture();

    GetGlyphsRegions(group.m_texture, text, fixedHeight, regions);
  }

  template<typename TGlyphGroup>
  void FillResults(strings::UniString const & text, int fixedHeight, TGlyphsBuffer & buffers,
                   TGlyphGroup & group)
  {
    MarkCharactersUsage(text, fixedHeight, group);
    FillResultBuffer<TGlyphGroup>(text, fixedHeight, group, buffers);
  }

  template<typename TGlyphGroup>
  void FillResults(TMultilineText const & text, int fixedHeight, TMultilineGlyphsBuffer & buffers,
                   TGlyphGroup & group)
  {
     buffers.resize(text.size());
     for (size_t i = 0; i < text.size(); ++i)
     {
       strings::UniString const & str = text[i];
       TGlyphsBuffer & buffer = buffers[i];
       FillResults<TGlyphGroup>(str, fixedHeight, buffer, group);
     }
  }

  template<typename TText, typename TBuffer>
  void CalcGlyphRegions(TText const & text, int fixedHeight, TBuffer & buffers)
  {
    size_t const groupIndex = FindGlyphsGroup(text);
    bool useHybridGroup = false;
    if (fixedHeight < 0 && groupIndex != GetInvalidGlyphGroup())
    {
      GlyphGroup & group = m_glyphGroups[groupIndex];
      uint32_t const absentGlyphs = GetAbsentGlyphsCount(group.m_texture, text, fixedHeight);
      if (group.m_texture == nullptr || group.m_texture->HasEnoughSpace(absentGlyphs))
        FillResults<GlyphGroup>(text, fixedHeight, buffers, group);
      else
        useHybridGroup = true;
    }
    else
    {
      useHybridGroup = true;
    }

    if (useHybridGroup)
    {
      size_t const hybridGroupIndex = FindHybridGlyphsGroup(text, fixedHeight);
      ASSERT(hybridGroupIndex != GetInvalidGlyphGroup(), ());
      HybridGlyphGroup & group = m_hybridGlyphGroups[hybridGroupIndex];
      FillResults<HybridGlyphGroup>(text, fixedHeight, buffers, group);
    }
  }

  uint32_t GetAbsentGlyphsCount(ref_ptr<Texture> texture, strings::UniString const & text,
                                int fixedHeight) const;
  uint32_t GetAbsentGlyphsCount(ref_ptr<Texture> texture, TMultilineText const & text,
                                int fixedHeight) const;

  void UpdateGlyphTextures();
  bool HasAsyncRoutines() const;

  static constexpr size_t GetInvalidGlyphGroup();

private:
  std::string m_resPostfix;
  std::vector<drape_ptr<Texture>> m_symbolTextures;
  drape_ptr<Texture> m_stipplePenTexture;
  drape_ptr<Texture> m_colorTexture;
  std::list<drape_ptr<Texture>> m_glyphTextures;
  mutable std::mutex m_glyphTexturesMutex;

  drape_ptr<Texture> m_trafficArrowTexture;
  drape_ptr<Texture> m_hatchingTexture;
  drape_ptr<Texture> m_smaaAreaTexture;
  drape_ptr<Texture> m_smaaSearchTexture;

  drape_ptr<GlyphManager> m_glyphManager;
  drape_ptr<HWTextureAllocator> m_textureAllocator;

  buffer_vector<GlyphGroup, 64> m_glyphGroups;
  buffer_vector<HybridGlyphGroup, 4> m_hybridGlyphGroups;

  std::atomic_flag m_nothingToUpload;
  std::mutex m_calcGlyphsMutex;
};

} // namespace dp
