#pragma once

#include "shape.hpp"
#include "gui_text.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

namespace gui
{

class ButtonHandle : public TappableHandle
{
  typedef TappableHandle TBase;

public:
  ButtonHandle(dp::Anchor anchor, m2::PointF const & size,
               dp::Color const & color, dp::Color const & pressedColor);

  void OnTapBegin() override;
  void OnTapEnd() override;
  bool Update(ScreenBase const & screen) override;

private:
  bool m_isInPressedState;
  dp::Color m_color;
  dp::Color m_pressedColor;
};

class Button
{
public:
  struct ButtonVertex
  {
    ButtonVertex() = default;
    ButtonVertex(glsl::vec2 const & normal)
      : m_normal(normal)
    {}

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec2 m_normal;
  };

  using TCreatorResult = drape_ptr<dp::OverlayHandle>;
  using THandleCreator = function<TCreatorResult (dp::Anchor, m2::PointF const &)>;
  using TLabelHandleCreator = function<TCreatorResult (dp::Anchor, m2::PointF const &, gui::TAlphabet const &)>;

  struct Params
  {
    string m_label;
    dp::FontDecl m_labelFont;
    dp::Anchor m_anchor;

    float m_width = 0.0f;
    float m_margin = 0.0f;
    float m_facet = 0.0f;

    THandleCreator m_bodyHandleCreator;
    TLabelHandleCreator m_labelHandleCreator;
  };

  static gui::StaticLabel::LabelResult PreprocessLabel(Params const & params, ref_ptr<dp::TextureManager> texMgr);
  static void Draw(Params const & params, ShapeControl & control, gui::StaticLabel::LabelResult & label);
};
}
