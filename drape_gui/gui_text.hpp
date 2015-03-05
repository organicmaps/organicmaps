#pragma once

#include "../drape/drape_global.hpp"
#include "../drape/binding_info.hpp"
#include "../drape/texture_manager.hpp"
#include "../drape/glsl_types.hpp"

#include "../std/utility.hpp"
#include "../std/stdint.hpp"

namespace gui
{

class GuiText
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

  GuiText(dp::Anchor anchor);

  void SetMaxLength(uint16_t maxLength);
  dp::RefPointer<dp::Texture> SetAlphabet(string const & alphabet, dp::RefPointer<dp::TextureManager> mng);
  dp::RefPointer<dp::Texture> Precache(buffer_vector<StaticVertex, 32> & buffer, dp::FontDecl const & font,
                                       dp::RefPointer<dp::TextureManager> mng);
  void SetText(buffer_vector<DynamicVertex, 32> & buffer, string text) const;

private:
  dp::Anchor m_anchor;
  uint16_t m_maxLength = 0;
  float m_textRatio = 0.0f;

  typedef pair<strings::UniChar, dp::TextureManager::GlyphRegion> TAlphabetNode;
  typedef vector<TAlphabetNode> TAlphabet;
  TAlphabet m_alphabet;
};

}
