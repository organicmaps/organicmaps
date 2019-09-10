#include "drape_frontend/gui/watermark.hpp"
#include "drape_frontend/gui/drape_gui.hpp"
#include "drape_frontend/batcher_bucket.hpp"

#include "shaders/programs.hpp"

#include "drape/glsl_func.hpp"
#include "drape/utils/vertex_decl.hpp"

#include <functional>
#include <utility>

using namespace std::placeholders;

namespace gui
{
namespace
{
struct WatermarkVertex
{
  WatermarkVertex(glsl::vec2 const & position, glsl::vec2 const & texCoord)
    : m_position(position), m_texCoord(texCoord)
  {}

  glsl::vec2 m_position;
  glsl::vec2 m_texCoord;
};

class WatermarkVertexHandle : public Handle
{
  using TBase = Handle;

public:
  WatermarkVertexHandle(uint32_t id, m2::PointF const & pivot, dp::Anchor anchor,
                        m2::PointF const & size)
    : Handle(id, anchor, pivot, size)
  {
    SetIsVisible(true);

    m2::PointF const halfSize(0.5f * size.x , 0.5f * size.y);
    m_offset = glsl::vec2(0.0f, 0.0f);

    if (anchor & dp::Left)
      m_offset.x = halfSize.x;
    else if (m_anchor & dp::Right)
      m_offset.x = -halfSize.x;

    if (anchor & dp::Top)
      m_offset.y = halfSize.y;
    else if (m_anchor & dp::Bottom)
      m_offset.y = -halfSize.y;
  }

  bool Update(ScreenBase const & screen) override
  {
    using namespace glsl;

    if (IsVisible())
    {
      m_params.m_modelView = transpose(translate(mat4(), vec3(m_pivot + m_offset, 0.0)));
      m_params.m_opacity = 1.0f;
    }
    return true;
  }

private:
  glsl::vec2 m_offset;
};
}  // namespace

drape_ptr<ShapeRenderer> Watermark::Draw(ref_ptr<dp::GraphicsContext> context, m2::PointF & size,
                                         ref_ptr<dp::TextureManager> tex) const
{
  dp::TextureManager::SymbolRegion region;
  tex->GetSymbolRegion("watermark", region);
  glsl::vec2 const halfSize = glsl::ToVec2(region.GetPixelSize() * 0.5f);
  m2::RectF const texRect = region.GetTexRect();

  WatermarkVertex vertexes[] =
  {
    WatermarkVertex(glsl::vec2(-halfSize.x, halfSize.y), glsl::ToVec2(texRect.LeftTop())),
    WatermarkVertex(glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom())),
    WatermarkVertex(glsl::vec2(halfSize.x, halfSize.y), glsl::ToVec2(texRect.RightTop())),
    WatermarkVertex(glsl::vec2(halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom()))
  };

  auto state = df::CreateRenderState(gpu::Program::TexturingGui, df::DepthLayer::GuiLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(false);
  state.SetTextureIndex(region.GetTextureIndex());

  dp::AttributeProvider provider(1 /* streamCount */, 4 /* vertexCount */);
  dp::BindingInfo info(2 /* count */);

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 2;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(WatermarkVertex);

  dp::BindingDecl & texDecl = info.GetBindingDecl(1);
  texDecl.m_attributeName = "a_colorTexCoords";
  texDecl.m_componentCount = 2;
  texDecl.m_componentType = gl_const::GLFloatType;
  texDecl.m_offset = sizeof(glsl::vec2);
  texDecl.m_stride = posDecl.m_stride;

  provider.InitStream(0, info, make_ref(&vertexes));

  size = region.GetPixelSize();
  drape_ptr<dp::OverlayHandle> handle = make_unique_dp<WatermarkVertexHandle>(
    EGuiHandle::GuiHandleWatermark, m_position.m_pixelPivot, m_position.m_anchor, size);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
  batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
  dp::SessionGuard guard(context, batcher, std::bind(&ShapeRenderer::AddShape, renderer.get(), _1, _2));
  batcher.InsertTriangleStrip(context, state, make_ref(&provider), std::move(handle));

  return renderer;
}
}  // namespace gui
