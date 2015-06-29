#pragma once

#include "shape.hpp"

#include "drape/binding_info.hpp"
#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glstate.hpp"
#include "drape/texture_manager.hpp"

#include "std/stdint.hpp"
#include "std/utility.hpp"

namespace gui
{

class StaticLabel
{
public:
  static char const * DefaultDelim;
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
    LabelResult();

    dp::GLState m_state;
    buffer_vector<Vertex, 128> m_buffer;
    m2::RectF m_boundRect;
  };

  /// return pixelSize of text
  static void CacheStaticText(string const & text, char const * delim,
                                    dp::Anchor anchor, dp::FontDecl const & font,
                                    ref_ptr<dp::TextureManager> mng, LabelResult & result);
};

////////////////////////////////////////////////////////////////////////////////////////////////

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

  struct PrecacheParams
  {
    string m_alphabet;
    size_t m_maxLength;
    dp::FontDecl m_font;
  };

  struct PrecacheResult
  {
    PrecacheResult();

    dp::GLState m_state;
    buffer_vector<StaticVertex, 128> m_buffer;
    m2::PointF m_maxPixelSize;
  };

  struct LabelResult
  {
    buffer_vector<DynamicVertex, 128> m_buffer;
    m2::RectD m_boundRect;
  };

  void Precache(PrecacheParams const & params, PrecacheResult & result,
                ref_ptr<dp::TextureManager> mng);

  void SetText(LabelResult & result, string text) const;
  m2::PointF GetAvarageSize() const;

private:
  void SetMaxLength(uint16_t maxLength);
  ref_ptr<dp::Texture> SetAlphabet(string const & alphabet, ref_ptr<dp::TextureManager> mng);

private:
  dp::Anchor m_anchor;
  uint16_t m_maxLength = 0;
  float m_textRatio = 0.0f;

  typedef pair<strings::UniChar, dp::TextureManager::GlyphRegion> TAlphabetNode;
  typedef vector<TAlphabetNode> TAlphabet;
  TAlphabet m_alphabet;
};

class MutableLabelHandle : public Handle
{
  typedef Handle TBase;

public:
  MutableLabelHandle(dp::Anchor anchor, m2::PointF const & pivot);

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen, bool isVisible) const override;

  ref_ptr<MutableLabel> GetTextView();
  void UpdateSize(m2::PointF const & size);

protected:
  void SetContent(string && content);
  void SetContent(string const & content);

private:
  drape_ptr<MutableLabel> m_textView;
  mutable bool m_isContentDirty;
  string m_content;
};

class MutableLabelDrawer
{
public:
  using TCreatoreResult = drape_ptr<MutableLabelHandle>;
  using THandleCreator = function<TCreatoreResult(dp::Anchor, m2::PointF const & /*pivot*/)>;

  struct Params
  {
    dp::Anchor m_anchor;
    dp::FontDecl m_font;
    m2::PointF m_pivot;
    string m_alphabet;
    size_t m_maxLength;
    THandleCreator m_handleCreator;
  };

  /// return maximum pixel size
  static m2::PointF Draw(Params const & params, ref_ptr<dp::TextureManager> mng,
                         dp::Batcher::TFlushFn const & flushFn);
};
}
