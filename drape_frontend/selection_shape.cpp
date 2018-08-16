#include "drape_frontend/selection_shape.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"
#include "drape_frontend/visual_params.hpp"

#include "shaders/program_manager.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"
#include "drape/binding_info.hpp"
#include "drape/glsl_func.hpp"
#include "drape/glsl_types.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/map_style_reader.hpp"

namespace df
{
namespace
{
df::ColorConstant const kSelectionColor = "Selection";

struct Vertex
{
  Vertex() = default;
  Vertex(glsl::vec2 const & normal, glsl::vec2 const & texCoord)
    : m_normal(normal)
    , m_texCoord(texCoord)
  {}

  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetBindingInfo()
{
  dp::BindingInfo info(2);
  dp::BindingDecl & normal = info.GetBindingDecl(0);
  normal.m_attributeName = "a_normal";
  normal.m_componentCount = 2;
  normal.m_componentType = gl_const::GLFloatType;
  normal.m_offset = 0;
  normal.m_stride = sizeof(Vertex);

  dp::BindingDecl & texCoord = info.GetBindingDecl(1);
  texCoord.m_attributeName = "a_colorTexCoords";
  texCoord.m_componentCount = 2;
  texCoord.m_componentType = gl_const::GLFloatType;
  texCoord.m_offset = sizeof(glsl::vec2);
  texCoord.m_stride = sizeof(Vertex);

  return info;
}
}  // namespace

SelectionShape::SelectionShape(ref_ptr<dp::TextureManager> mng)
  : m_position(m2::PointD::Zero())
  , m_positionZ(0.0)
  , m_animation(false, 0.25)
  , m_selectedObject(OBJECT_EMPTY)
{
  size_t constexpr kTriangleCount = 40;
  size_t constexpr kVertexCount = 3 * kTriangleCount;
  auto const etalonSector = static_cast<float>(math::twicePi / kTriangleCount);

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(df::GetColorConstant(df::kSelectionColor), color);
  glsl::vec2 colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  buffer_vector<Vertex, kTriangleCount> buffer;

  glsl::vec2 startNormal(0.0f, 1.0f);

  for (size_t i = 0; i < kTriangleCount + 1; ++i)
  {
    glsl::vec2 normal = glsl::rotate(startNormal, i * etalonSector);
    glsl::vec2 nextNormal = glsl::rotate(startNormal, (i + 1) * etalonSector);

    buffer.emplace_back(startNormal, colorCoord);
    buffer.emplace_back(normal, colorCoord);
    buffer.emplace_back(nextNormal, colorCoord);
  }

  auto state = CreateRenderState(gpu::Program::Accuracy, DepthLayer::OverlayLayer);
  state.SetColorTexture(color.GetTexture());
  state.SetDepthTestEnabled(false);

  {
    dp::Batcher batcher(kTriangleCount * dp::Batcher::IndexPerTriangle, kVertexCount);
    dp::SessionGuard guard(batcher, [this](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = std::move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());
      m_renderNode = make_unique_dp<RenderNode>(state, bucket->MoveBuffer());
    });

    dp::AttributeProvider provider(1 /* stream count */, kVertexCount);
    provider.InitStream(0 /* stream index */, GetBindingInfo(), make_ref(buffer.data()));

    batcher.InsertTriangleList(state, make_ref(&provider), nullptr);
  }

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
}

void SelectionShape::Hide()
{
  m_animation.Hide();
  m_selectedObject = OBJECT_EMPTY;
}

bool SelectionShape::IsVisible(ScreenBase const & screen, m2::PointD & pxPos) const
{
  m2::PointD const pt = screen.GtoP(m_position);
  ShowHideAnimation::EState state = m_animation.GetState();

  if ((state == ShowHideAnimation::STATE_VISIBLE || state == ShowHideAnimation::STATE_SHOW_DIRECTION) &&
      !screen.IsReverseProjection3d(pt))
  {
    pxPos = screen.PtoP3d(pt, -m_positionZ);
    return true;
  }
  return false;
}

void SelectionShape::Render(ref_ptr<dp::GraphicsContext> context, ref_ptr<gpu::ProgramManager> mng,
                            ScreenBase const & screen, int zoomLevel, FrameValues const & frameValues)
{
  ShowHideAnimation::EState state = m_animation.GetState();
  if (state == ShowHideAnimation::STATE_VISIBLE ||
      state == ShowHideAnimation::STATE_SHOW_DIRECTION)
  {
    gpu::ShapesProgramParams params;
    frameValues.SetTo(params);
    TileKey const key = GetTileKeyByPoint(m_position, ClipTileZoomByMaxDataZoom(zoomLevel));
    math::Matrix<float, 4, 4> mv = key.GetTileBasedModelView(screen);
    params.m_modelView = glsl::make_mat4(mv.m_data);

    m2::PointD const pos = MapShape::ConvertToLocal(m_position, key.GetGlobalRect().Center(), kShapeCoordScalar);
    params.m_position = glsl::vec3(pos.x, pos.y, -m_positionZ);

    float accuracy = m_mapping.GetValue(m_animation.GetT());
    if (screen.isPerspective())
    {
      m2::PointD const pt1 = screen.GtoP(m_position);
      m2::PointD const pt2(pt1.x + 1, pt1.y);
      auto const scale = static_cast<float>(screen.PtoP3d(pt2).x - screen.PtoP3d(pt1).x);
      accuracy /= scale;
    }
    params.m_accuracy = accuracy;
    m_renderNode->Render(context, mng, params);
  }
}

SelectionShape::ESelectedObject SelectionShape::GetSelectedObject() const
{
  return m_selectedObject;
}
}  // namespace df
