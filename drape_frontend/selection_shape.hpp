#pragma once

#include "animation/show_hide_animation.hpp"
#include "animation/value_mapping.hpp"
#include "render_node.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

namespace dp
{
class UniformValuesStorage;
class TextureManager;
}  // namespace dp

namespace gpu
{
class ProgramManager;
}  // namespace gpu

namespace df
{
class SelectionShape
{
public:
  enum ESelectedObject
  {
    OBJECT_EMPTY,
    OBJECT_POI,
    OBJECT_USER_MARK,
    OBJECT_MY_POSITION
  };

  explicit SelectionShape(ref_ptr<dp::TextureManager> mng);

  void SetPosition(m2::PointD const & position) { m_position = position; }
  void Show(ESelectedObject obj, m2::PointD const & position, double positionZ, bool isAnimate);
  void Hide();
  void Render(ScreenBase const & screen, int zoomLevel, ref_ptr<gpu::ProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

  bool IsVisible(ScreenBase const & screen, m2::PointD & pxPos) const;
  double GetRadius() const { return m_radius; }

  ESelectedObject GetSelectedObject() const;

private:
  m2::PointD m_position;
  double m_positionZ;
  double m_radius;
  ShowHideAnimation m_animation;
  ESelectedObject m_selectedObject;

  drape_ptr<RenderNode> m_renderNode;
  ValueMapping<float> m_mapping;
};
}  // namespace df
