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
  ButtonHandle(dp::Anchor anchor, m2::PointF const & size);

  void OnTapBegin() override;
  void OnTapEnd() override;
  void Update(ScreenBase const & screen) override;

private:
  bool m_isInPressedState;
};

class Button
{
public:
  struct ButtonVertex
  {
    ButtonVertex() = default;
    ButtonVertex(glsl::vec3 const & position, glsl::vec2 const & normal)
      : m_position(position)
      , m_normal(normal)
    {
    }

    static dp::BindingInfo const & GetBindingInfo();

    glsl::vec3 m_position;
    glsl::vec2 m_normal;
  };

  static constexpr int VerticesCount();

  using TCreatorResult = drape_ptr<dp::OverlayHandle>;
  using THandleCreator = function<TCreatorResult (dp::Anchor, m2::PointF const &)>;

  struct Params
  {
    string m_label;
    dp::FontDecl m_labelFont;
    dp::Anchor m_anchor;

    float m_minWidth = 0.0f;
    float m_maxWidth = 0.0f;
    float m_margin = 0.0f;
    THandleCreator m_bodyHandleCreator;
    THandleCreator m_labelHandleCreator;
  };

  static void Draw(Params const & params, ShapeControl & control, ref_ptr<dp::TextureManager> texMgr);
};
}
