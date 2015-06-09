#pragma once

#include "animation/show_hide_animation.hpp"
#include "animation/value_mapping.hpp"
#include "render_node.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

namespace dp
{

class GpuProgramManager;
class UniformValuesStorage;
class TextureManager;

}

namespace df
{

class SelectionShape
{
public:
  enum ESelectedObject
  {
    OBJECT_POI,
    OBJECT_USER_MARK,
    OBJECT_MY_POSITION
  };

  SelectionShape(ref_ptr<dp::TextureManager> mng);

  void SetPosition(m2::PointD const & position);
  void Show();
  void Hide();
  void Render(ScreenBase const & screen,
              ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

  void SetSelectedObject(ESelectedObject obj);
  ESelectedObject GetSelectedObject() const;

private:
  double GetCurrentRadius() const;

private:
  m2::PointD m_position;
  ShowHideAnimation m_animation;
  ESelectedObject m_selectedObject;

  drape_ptr<RenderNode> m_renderNode;
  ValueMapping<float> m_mapping;
};

} // namespace df
