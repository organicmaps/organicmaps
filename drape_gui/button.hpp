#pragma once

#include "shape.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

namespace gui
{

class ButtonHandle : public TappableHandle
{
  typedef TappableHandle TBase;

public:
  ButtonHandle(dp::Anchor anchor, m2::PointF const & size,
               dp::TextureManager::ColorRegion const & normalColor,
               dp::TextureManager::ColorRegion const & pressedColor,
               dp::TOverlayHandler const & tapHandler);

  void OnTapBegin() override;
  void OnTapEnd() override;

  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                            ScreenBase const & screen) const override;

private:
  dp::TextureManager::ColorRegion m_normalColor;
  dp::TextureManager::ColorRegion m_pressedColor;
  bool m_isInPressedState;
  mutable bool m_isContentDirty;
};

class Button
{
public:
  struct StaticVertex
  {
    StaticVertex() = default;
    StaticVertex(glsl::vec3 const & position, glsl::vec2 const & normal)
      : m_position(position)
      , m_normal(normal)
    {
    }

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec3 m_position;
    glsl::vec2 m_normal;
  };

  struct DynamicVertex
  {
    DynamicVertex() = default;
    DynamicVertex(glsl::vec2 const & texCoord)
      : m_texCoord(texCoord)
    {
    }

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec2 m_texCoord;
  };

  static constexpr int VerticesCount();

  using TCreatorResult = drape_ptr<dp::OverlayHandle>;
  using TLabelHandleCreator = function<TCreatorResult (dp::Anchor, m2::PointF const &)>;
  using TBodyHandleCreator = function<TCreatorResult (dp::Anchor, m2::PointF const &,
                                                      dp::TextureManager::ColorRegion const &,
                                                      dp::TextureManager::ColorRegion const &)>;

  struct Params
  {
    string m_label;
    dp::FontDecl m_labelFont;
    dp::Anchor m_anchor;

    float m_minWidth = 0.0f;
    float m_maxWidth = 0.0f;
    float m_margin = 0.0f;
    TBodyHandleCreator m_bodyHandleCreator;
    TLabelHandleCreator m_labelHandleCreator;
  };

  static void Draw(Params const & params, ShapeControl & control, ref_ptr<dp::TextureManager> texMgr);
};
}
