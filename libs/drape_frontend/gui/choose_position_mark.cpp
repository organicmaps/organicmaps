#include "drape_frontend/gui/choose_position_mark.hpp"
#include "drape_frontend/gui/drape_gui.hpp"

#include "drape_frontend/batcher_bucket.hpp"

#include "shaders/programs.hpp"

#include "drape/utils/vertex_decl.hpp"

#include <functional>
#include <utility>

using namespace std::placeholders;

namespace gui
{
namespace
{
struct ChoosePositionMarkVertex
{
  ChoosePositionMarkVertex(glsl::vec2 const & position, glsl::vec2 const & texCoord)
    : m_position(position)
    , m_texCoord(texCoord)
  {}

  glsl::vec2 m_position;
  glsl::vec2 m_texCoord;
};

class ChoosePositionMarkHandle : public Handle
{
  using TBase = Handle;

public:
  ChoosePositionMarkHandle(uint32_t id, m2::PointF const & pivot) : Handle(id, dp::Center, pivot)
  {
    SetIsVisible(true);
  }

  bool Update(ScreenBase const & screen) override
  {
    SetPivot(glsl::ToVec2(m2::PointF(screen.PixelRectIn3d().Center())));
    return TBase::Update(screen);
  }
};
}  // namespace

drape_ptr<ShapeRenderer> ChoosePositionMark::Draw(ref_ptr<dp::GraphicsContext> context,
                                                  ref_ptr<dp::TextureManager> tex) const
{
  dp::TextureManager::SymbolRegion region;
  tex->GetSymbolRegion("cross_geoposition", region);
  glsl::vec2 const halfSize = glsl::ToVec2(region.GetPixelSize() * 0.5f);
  m2::RectF const texRect = region.GetTexRect();

  ASSERT_EQUAL(m_position.m_anchor, dp::Center, ());
  ChoosePositionMarkVertex vertexes[] = {
      ChoosePositionMarkVertex(glsl::vec2(-halfSize.x, halfSize.y), glsl::ToVec2(texRect.LeftTop())),
      ChoosePositionMarkVertex(glsl::vec2(-halfSize.x, -halfSize.y), glsl::ToVec2(texRect.LeftBottom())),
      ChoosePositionMarkVertex(glsl::vec2(halfSize.x, halfSize.y), glsl::ToVec2(texRect.RightTop())),
      ChoosePositionMarkVertex(glsl::vec2(halfSize.x, -halfSize.y), glsl::ToVec2(texRect.RightBottom()))};

  auto state = df::CreateRenderState(gpu::Program::TexturingGui, df::DepthLayer::GuiLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(false);
  state.SetTextureIndex(region.GetTextureIndex());

  dp::AttributeProvider provider(1 /*streamCount*/, 4 /*vertexCount*/);
  dp::BindingInfo info(2 /*count*/);

  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 2;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(ChoosePositionMarkVertex);

  dp::BindingDecl & texDecl = info.GetBindingDecl(1);
  texDecl.m_attributeName = "a_colorTexCoords";
  texDecl.m_componentCount = 2;
  texDecl.m_componentType = gl_const::GLFloatType;
  texDecl.m_offset = sizeof(glsl::vec2);
  texDecl.m_stride = posDecl.m_stride;

  provider.InitStream(0, info, make_ref(&vertexes));

  drape_ptr<dp::OverlayHandle> handle =
      make_unique_dp<ChoosePositionMarkHandle>(GuiHandleChoosePositionMark, m_position.m_pixelPivot);

  drape_ptr<ShapeRenderer> renderer = make_unique_dp<ShapeRenderer>();
  dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
  batcher.SetBatcherHash(static_cast<uint64_t>(df::BatcherBucket::Default));
  dp::SessionGuard guard(context, batcher, std::bind(&ShapeRenderer::AddShape, renderer.get(), _1, _2));
  batcher.InsertTriangleStrip(context, state, make_ref(&provider), std::move(handle));

  return renderer;
}
}  // namespace gui
