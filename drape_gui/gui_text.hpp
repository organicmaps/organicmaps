#pragma once

#include "../drape/drape_global.hpp"
#include "../drape/binding_info.hpp"
#include "../drape/texture_manager.hpp"
#include "../drape/glsl_types.hpp"

#include "../std/utility.hpp"
#include "../std/stdint.hpp"

namespace gui
{

class StaticLabel
{
public:
  struct Vertex
  {
    Vertex() = default;
    Vertex(glsl::vec3 const & pos, glsl::vec2 const & normal, glsl::vec2 const & color,
           glsl::vec2 const & outline, glsl::vec2 const & mask)
      : m_position(pos)
      , m_normal(normal)
      , m_colorTexCoord(color)
      , m_outlineColorTexCoord(outline)
      , m_maskTexCoord(mask)
    {
    }

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec3 m_position;
    glsl::vec2 m_normal;
    glsl::vec2 m_colorTexCoord;
    glsl::vec2 m_outlineColorTexCoord;
    glsl::vec2 m_maskTexCoord;
  };

  struct LabelResult
  {
    dp::RefPointer<dp::Texture> m_colorTexture;
    dp::RefPointer<dp::Texture> m_maskTexture;
    buffer_vector<Vertex, 128> m_buffer;
  };

  static void CacheStaticText(string const & text, char const * delim,
                              dp::Anchor anchor, dp::FontDecl const & font,
                              dp::RefPointer<dp::TextureManager> mng, LabelResult & result);
};

class MutableLabel
{
public:
  struct StaticVertex
  {
    StaticVertex() = default;
    StaticVertex(glsl::vec3 const & position, glsl::vec2 const & color, glsl::vec2 const & outlineColor)
      : m_position(position)
      , m_color(color)
      , m_outline(outlineColor)
    {
    }

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec3 m_position;
    glsl::vec2 m_color;
    glsl::vec2 m_outline;
  };

  struct DynamicVertex
  {
    DynamicVertex() = default;
    DynamicVertex(glsl::vec2 const & normal, glsl::vec2 const & mask)
      : m_normal(normal)
      , m_maskTexCoord(mask)
    {
    }

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec2 m_normal;
    glsl::vec2 m_maskTexCoord;
  };

  MutableLabel(dp::Anchor anchor);

  void SetMaxLength(uint16_t maxLength);
  dp::RefPointer<dp::Texture> SetAlphabet(string const & alphabet, dp::RefPointer<dp::TextureManager> mng);
  dp::RefPointer<dp::Texture> Precache(buffer_vector<StaticVertex, 128> & buffer, dp::FontDecl const & font,
                                       dp::RefPointer<dp::TextureManager> mng);
  void SetText(buffer_vector<DynamicVertex, 128> & buffer, string text) const;

private:
  dp::Anchor m_anchor;
  uint16_t m_maxLength = 0;
  float m_textRatio = 0.0f;

  typedef pair<strings::UniChar, dp::TextureManager::GlyphRegion> TAlphabetNode;
  typedef vector<TAlphabetNode> TAlphabet;
  TAlphabet m_alphabet;
};

}
