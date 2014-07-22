#pragma once

#include "texture.hpp"

#include "../base/string_utils.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

class TextureSetHolder
{
public:
  virtual ~TextureSetHolder() {}
  virtual void Init(string const & resourcePrefix) = 0;
  virtual void Release() = 0;

  struct TextureNode
  {
    TextureNode();

    bool IsValid() const;

    int32_t m_width;
    int32_t m_height;
    int32_t m_textureSet;
    int32_t m_textureOffset;
  };

  class BaseRegion
  {
  public:
    BaseRegion();

    bool IsValid() const;

    void SetResourceInfo(Texture::ResourceInfo const * info);
    void SetTextureNode(TextureNode const & node);

    void GetPixelSize(m2::PointU & size) const;

    m2::RectF const & GetTexRect() const;
    TextureNode const & GetTextureNode() const;

  protected:
    Texture::ResourceInfo const * m_info;
    TextureNode m_node;
  };

  class SymbolRegion : public BaseRegion {};

  class GlyphRegion : public BaseRegion
  {
  public:
    GlyphRegion();

    void GetMetrics(float & xOffset, float & yOffset, float & advance) const;
  };

  virtual void GetSymbolRegion(string const & symbolName, SymbolRegion & region) const = 0;
  virtual bool GetGlyphRegion(strings::UniChar charCode, GlyphRegion & region) const = 0;
  virtual int GetMaxTextureSet() const = 0;
};
