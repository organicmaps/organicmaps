#include "drape_frontend/selection_shape.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/selection_shape_generator.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/program_manager.hpp"

#include "drape/texture_manager.hpp"

#include "indexer/map_style_reader.hpp"

namespace df
{
namespace
{
std::vector<float> const kHalfLineWidthInPixel =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.2f, 1.5f, 1.5f, 1.7f, 2.0f, 2.0f, 2.3f, 2.5f, 2.7f,
  //11   12    13    14    15   16    17    18    19     20
  3.0f, 3.5f, 4.5f, 5.5f, 7.0, 9.0f, 10.0f, 14.0f, 22.0f, 27.0f
};
}  // namespace

SelectionShape::SelectionShape(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> mng)
  : m_position(m2::PointD::Zero())
  , m_positionZ(0.0)
  , m_animation(false, 0.25)
  , m_selectedObject(OBJECT_EMPTY)
{
  m_renderNode = SelectionShapeGenerator::GenerateSelectionMarker(context, mng);
  m_trackRenderNode = SelectionShapeGenerator::GenerateTrackSelectionMarker(context, mng);
  m_radius = 15.0f * VisualParams::Instance().GetVisualScale();
  m_mapping.AddRangePoint(0.6, 1.3 * m_radius);
  m_mapping.AddRangePoint(0.85, 0.8 * m_radius);
  m_mapping.AddRangePoint(1.0, m_radius);
}

void SelectionShape::Show(ESelectedObject obj, m2::PointD const & position, double positionZ, bool isAnimate)
{
  m_animation.Hide();
  m_position = position;
  m_positionZ = positionZ;
  m_selectedObject = obj;
  if (isAnimate)
    m_animation.ShowAnimated();
  else
    m_animation.Show();
  m_recacheId++;
  m_selectionGeometry.clear();
}

void SelectionShape::Hide()
{
  m_animation.Hide();
  m_selectedObject = OBJECT_EMPTY;
  m_recacheId++;
  m_selectionGeometry.clear();
}

bool SelectionShape::IsVisible(ScreenBase const & screen, m2::PointD & pxPos) const
{
  m2::PointD pos = m_position;
  double posZ = m_positionZ;
  if (!m_selectionGeometry.empty())
  {
    pos = GetSelectionGeometryBoundingBox().Center();
    posZ = 0.0;
  }

  m2::PointD const pt = screen.GtoP(pos);
  ShowHideAnimation::EState state = m_animation.GetState();

  if ((state == ShowHideAnimation::STATE_VISIBLE || state == ShowHideAnimation::STATE_SHOW_DIRECTION) &&
      !screen.IsReverseProjection3d(pt))
  {
    pxPos = screen.PtoP3d(pt, -posZ);
    return true;
  }
  return false;
}

void SelectionShape::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                            ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  if (m_selectedObject == OBJECT_GUIDE)
    return;

  ShowHideAnimation::EState state = m_animation.GetState();
  if (state != ShowHideAnimation::STATE_VISIBLE && state != ShowHideAnimation::STATE_SHOW_DIRECTION)
    return;

  if (m_selectionGeometry.empty())
  {
    gpu::ShapesProgramParams params;
    frameValues.SetTo(params);
    TileKey const key = GetTileKeyByPoint(m_position, ClipTileZoomByMaxDataZoom(zoomLevel));
    math::Matrix<float, 4, 4> mv = key.GetTileBasedModelView(screen);
    params.m_modelView = glsl::make_mat4(mv.m_data);

    m2::PointD const pos = MapShape::ConvertToLocal(m_position, key.GetGlobalRect().Center(), kShapeCoordScalar);
    params.m_position = glsl::vec3(pos.x, pos.y, -m_positionZ);

    float accuracy = m_selectedObject == OBJECT_TRACK ? 1.0 : m_mapping.GetValue(m_animation.GetT());
    if (screen.isPerspective())
    {
      m2::PointD const pt1 = screen.GtoP(m_position);
      m2::PointD const pt2(pt1.x + 1, pt1.y);
      auto const scale = static_cast<float>(screen.PtoP3d(pt2).x - screen.PtoP3d(pt1).x);
      accuracy /= scale;
    }
    params.m_accuracy = accuracy;
    if (m_selectedObject == OBJECT_TRACK)
      m_trackRenderNode->Render(context, mng, params);
    else
      m_renderNode->Render(context, mng, params);
  }
  else
  {
    // Render selection geometry.
    double zoom = 0.0;
    int index = 0;
    float lerpCoef = 0.0f;
    ExtractZoomFactors(screen, zoom, index, lerpCoef);
    float const currentHalfWidth = InterpolateByZoomLevels(index, lerpCoef, kHalfLineWidthInPixel) *
      VisualParams::Instance().GetVisualScale();
    auto const screenHalfWidth = static_cast<float>(currentHalfWidth * screen.GetScale());

    gpu::ShapesProgramParams geomParams;
    frameValues.SetTo(geomParams);
    geomParams.m_lineParams = glsl::vec2(currentHalfWidth, screenHalfWidth);
    for (auto const & geometry : m_selectionGeometry)
    {
      math::Matrix<float, 4, 4> mv = screen.GetModelView(geometry->GetPivot(), kShapeCoordScalar);
      geomParams.m_modelView = glsl::make_mat4(mv.m_data);
      geometry->Render(context, mng, geomParams);
    }
  }
}

SelectionShape::ESelectedObject SelectionShape::GetSelectedObject() const
{
  return m_selectedObject;
}

void SelectionShape::AddSelectionGeometry(drape_ptr<RenderNode> && renderNode, int recacheId)
{
  if (m_recacheId != recacheId)
    return;

  m_selectionGeometry.push_back(std::move(renderNode));
}

m2::RectD SelectionShape::GetSelectionGeometryBoundingBox() const
{
  m2::RectD rect;
  for (auto const & geometry : m_selectionGeometry)
    rect.Add(geometry->GetBoundingBox());
  return rect;
}
}  // namespace df
