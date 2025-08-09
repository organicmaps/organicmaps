#pragma once

#include "drape_frontend/gui/shape.hpp"
#include "drape_frontend/gui/speed_limit/speed_limit.hpp"

namespace gui::speed_limit::renderer
{
struct BackgroundVertex
{
  glsl::vec2 m_position;

  static dp::BindingInfo GetBindingInfo()
  {
    dp::BindingFiller<BackgroundVertex> filler(1);
    filler.FillDecl<glsl::vec2>("a_position");
    return filler.m_info;
  }
};

using BackgroundVertexData = buffer_vector<BackgroundVertex, dp::Batcher::VertexPerQuad>;

class BackgroundHandle : public Handle
{
public:
  BackgroundHandle(m2::PointF const & pivot, BackgroundVertexData data);

  bool IsTapped(m2::RectD const & touchArea) const override
  {
    if (!IsVisible())
      return false;

    SpeedLimit const & helper = DrapeGui::GetSpeedLimitHelper();
    auto const m_size = helper.GetSize();
    m2::RectD rect(m_pivot.x - m_size, m_pivot.y - m_size, m_pivot.x + m_size, m_pivot.y + m_size);
    return rect.Intersect(touchArea);
  }

  bool Update(ScreenBase const & screen) override
  {
    SpeedLimit const & helper = DrapeGui::GetSpeedLimitHelper();
    SetIsVisible(helper.IsEnabled() && helper.IsSpeedLimitAvailable());

    if (IsVisible())
    {
      m_pivot = glsl::ToVec2(helper.GetPosition());
      m_params.m_length = helper.GetSize();
    }

    return Handle::Update(screen);
  }
};

class BackgroundRenderer
{
public:
  void SetPosition(Position const & position);
  void SetRadius(float size);

  void Draw(ref_ptr<dp::GraphicsContext> context, ShapeControl & control) const;

private:
  Position m_position;
  float m_size = 0.0f;
};
}  // namespace gui::speed_limit::renderer
