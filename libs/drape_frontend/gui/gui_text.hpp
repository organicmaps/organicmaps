#pragma once

#include "drape_frontend/gui/shape.hpp"

#include "base/buffer_vector.hpp"

#include "drape/binding_info.hpp"
#include "drape/drape_global.hpp"
#include "drape/glsl_types.hpp"
#include "drape/texture_manager.hpp"

#include <functional>
#include <string>
#include <unordered_set>

namespace gui
{
using TAlphabet = std::unordered_set<strings::UniChar>;

class StaticLabel
{
public:
  struct Vertex
  {
    Vertex() = default;
    Vertex(glsl::vec3 const & pos, glsl::vec2 const & color, glsl::vec2 const & outline, glsl::vec2 const & normal,
           glsl::vec2 const & mask)
      : m_position(pos)
      , m_colorTexCoord(color)
      , m_outlineColorTexCoord(outline)
      , m_normal(normal)
      , m_maskTexCoord(mask)
    {}

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec3 m_position;
    glsl::vec2 m_colorTexCoord;
    glsl::vec2 m_outlineColorTexCoord;
    glsl::vec2 m_normal;
    glsl::vec2 m_maskTexCoord;
  };

  struct LabelResult
  {
    LabelResult();

    dp::RenderState m_state;
    buffer_vector<Vertex, 128> m_buffer;
    m2::RectF m_boundRect;
    TAlphabet m_alphabet;
  };

  static dp::TGlyphs CacheStaticText(std::string const & text, char const * delim, dp::Anchor anchor,
                                     dp::FontDecl const & font, ref_ptr<dp::TextureManager> mng, LabelResult & result);
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
    {}

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec3 m_position;
    glsl::vec2 m_color;
    glsl::vec2 m_outline;
  };

  struct DynamicVertex
  {
    DynamicVertex() = default;
    DynamicVertex(glsl::vec2 const & normal, glsl::vec2 const & mask) : m_normal(normal), m_maskTexCoord(mask) {}

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec2 m_normal;
    glsl::vec2 m_maskTexCoord;
  };

  explicit MutableLabel(dp::Anchor anchor);

  struct PrecacheParams
  {
    std::string m_alphabet;
    size_t m_maxLength;
    dp::FontDecl m_font;
  };

  struct PrecacheResult
  {
    PrecacheResult();

    dp::RenderState m_state;
    buffer_vector<StaticVertex, 128> m_buffer;
    m2::PointF m_maxPixelSize;
  };

  struct LabelResult
  {
    buffer_vector<DynamicVertex, 128> m_buffer;
    m2::RectF m_boundRect;
  };

  void Precache(PrecacheParams const & params, PrecacheResult & result, ref_ptr<dp::TextureManager> mng);

  void SetText(LabelResult & result, std::string text, ref_ptr<dp::TextureManager> mng);

  // TODO(AB): Refactor.
  dp::TGlyphs GetGlyphs() const;

private:
  void SetMaxLength(uint16_t maxLength);

private:
  dp::Anchor m_anchor;
  uint16_t m_maxLength = 0;
  float m_textRatio = 0.0f;

  dp::text::TextMetrics m_shapedText;
  dp::TextureManager::TGlyphsBuffer m_glyphRegions;
};

class MutableLabelHandle : public Handle
{
  using TBase = Handle;

public:
  MutableLabelHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot);

  MutableLabelHandle(uint32_t id, dp::Anchor anchor, m2::PointF const & pivot, ref_ptr<dp::TextureManager> textures);

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const override;

  bool Update(ScreenBase const & screen) override;

  ref_ptr<MutableLabel> GetTextView() const;

protected:
  void SetContent(std::string && content);
  void SetContent(std::string const & content);
  void SetTextureManager(ref_ptr<dp::TextureManager> textures);

private:
  drape_ptr<MutableLabel> m_textView;
  mutable bool m_isContentDirty;
  std::string m_content;
  ref_ptr<dp::TextureManager> m_textureManager;
  bool m_glyphsReady;
};

class MutableLabelDrawer
{
public:
  using TCreatoreResult = drape_ptr<MutableLabelHandle>;
  using THandleCreator = std::function<TCreatoreResult(dp::Anchor, m2::PointF const & /* pivot */)>;

  struct Params
  {
    dp::Anchor m_anchor;
    dp::FontDecl m_font;
    m2::PointF m_pivot;
    std::string m_alphabet;
    uint32_t m_maxLength;
    THandleCreator m_handleCreator;
  };

  // Return maximum pixel size.
  static m2::PointF Draw(ref_ptr<dp::GraphicsContext> context, Params const & params, ref_ptr<dp::TextureManager> mng,
                         dp::Batcher::TFlushFn && flushFn);
};

class StaticLabelHandle : public Handle
{
  using TBase = Handle;

public:
  StaticLabelHandle(uint32_t id, ref_ptr<dp::TextureManager> textureManager, dp::Anchor anchor,
                    m2::PointF const & pivot, dp::TGlyphs && glyphs);

  bool Update(ScreenBase const & screen) override;

private:
  dp::TGlyphs m_glyphs;
  ref_ptr<dp::TextureManager> m_textureManager;
  bool m_glyphsReady;
};
}  // namespace gui
