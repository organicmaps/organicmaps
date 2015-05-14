#include "button.hpp"
#include "gui_text.hpp"

#include "drape/batcher.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/vertex_decl.hpp"

#include "std/bind.hpp"

namespace gui
{

ButtonHandle::ButtonHandle(dp::Anchor anchor, m2::PointF const & size,
                           dp::TextureManager::ColorRegion const & normalColor,
                           dp::TextureManager::ColorRegion const & pressedColor,
                           dp::TOverlayHandler const & tapHandler)
  : TBase(anchor, m2::PointF::Zero(), size, tapHandler)
  , m_normalColor(normalColor)
  , m_pressedColor(pressedColor)
  , m_isInPressedState(false)
  , m_isContentDirty(false)
{}

void ButtonHandle::OnTapBegin()
{
  m_isInPressedState = true;
  m_isContentDirty = true;
}

void ButtonHandle::OnTapEnd()
{
  m_isInPressedState = false;
  m_isContentDirty = true;
}

void ButtonHandle::GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator,
                                        ScreenBase const & screen) const
{
  UNUSED_VALUE(screen);

  if (!m_isContentDirty)
    return;

  m_isContentDirty = false;

  size_t const byteCount = sizeof(Button::DynamicVertex) * Button::VerticesCount();
  Button::DynamicVertex * dataPointer =
      reinterpret_cast<Button::DynamicVertex *>(mutator->AllocateMutationBuffer(byteCount));
  for (int i = 0; i < Button::VerticesCount(); ++i)
    dataPointer[i].m_texCoord = m_isInPressedState ? glsl::ToVec2(m_pressedColor.GetTexRect().Center()) :
                                                     glsl::ToVec2(m_normalColor.GetTexRect().Center());

  dp::BindingInfo const & binding = Button::DynamicVertex::GetBindingInfo();
  dp::OverlayHandle::TOffsetNode offsetNode = GetOffsetNode(binding.GetID());

  dp::MutateNode mutateNode;
  mutateNode.m_data = make_ref(dataPointer);
  mutateNode.m_region = offsetNode.second;
  mutator->AddMutation(offsetNode.first, mutateNode);
}

namespace
{

dp::TextureManager::ColorRegion GetNormalColor(ref_ptr<dp::TextureManager> texMgr)
{
  dp::Color const buttonColor = dp::Color(0x0, 0x0, 0x0, 0x99);
  dp::TextureManager::ColorRegion region;
  texMgr->GetColorRegion(buttonColor, region);
  return region;
}

dp::TextureManager::ColorRegion GetPressedColor(ref_ptr<dp::TextureManager> texMgr)
{
  dp::Color const buttonColor = dp::Color(0x0, 0x0, 0x0, 0xCC);
  dp::TextureManager::ColorRegion region;
  texMgr->GetColorRegion(buttonColor, region);
  return region;
}

}

constexpr int Button::VerticesCount()
{
  return 4;
}

void Button::Draw(Params const & params, ShapeControl & control, ref_ptr<dp::TextureManager> texMgr)
{
  StaticLabel::LabelResult result;
  StaticLabel::CacheStaticText(params.m_label, StaticLabel::DefaultDelim, params.m_anchor,
                               params.m_labelFont, texMgr, result);

  float textWidth = result.m_boundRect.SizeX();
  float halfWidth = my::clamp(textWidth, params.m_minWidth, params.m_maxWidth) * 0.5f;
  float halfHeight = result.m_boundRect.SizeY() * 0.5f;
  float halfWM = halfWidth + params.m_margin;
  float halfHM = halfHeight + params.m_margin;

  // Cache button
  {
    dp::TextureManager::ColorRegion normalColor = GetNormalColor(texMgr);
    dp::TextureManager::ColorRegion pressedColor = GetPressedColor(texMgr);

    dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::Gui);
    state.SetColorTexture(normalColor.GetTexture());

    glsl::vec3 position(0.0f, 0.0f, 0.0f);
    glsl::vec2 texCoord(glsl::ToVec2(normalColor.GetTexRect().Center()));

    int const verticesCount = VerticesCount();
    StaticVertex vertexes[verticesCount]
    {
        StaticVertex(position, glsl::vec2(-halfWM, halfHM)),
        StaticVertex(position, glsl::vec2(-halfWM, -halfHM)),
        StaticVertex(position, glsl::vec2(halfWM, halfHM)),
        StaticVertex(position, glsl::vec2(halfWM, -halfHM))
    };

    glsl::vec2 normalOffset(0.0f, 0.0f);
    if (params.m_anchor & dp::Left)
      normalOffset.x = halfWidth;
    else if (params.m_anchor & dp::Right)
      normalOffset.x = -halfWidth;

    if (params.m_anchor & dp::Top)
      normalOffset.x = halfHeight;
    else if (params.m_anchor & dp::Bottom)
      normalOffset.x = -halfHeight;

    for (StaticVertex & v : vertexes)
      v.m_normal = v.m_normal + normalOffset;

    buffer_vector<DynamicVertex, verticesCount> dynData;
    for (int i = 0; i < verticesCount; ++i)
      dynData.push_back(texCoord);

    dp::AttributeProvider provider(2 /* stream count */, verticesCount);
    provider.InitStream(0 /*stream index*/, StaticVertex::GetBindingInfo(),
                        make_ref(vertexes));
    provider.InitStream(1 /*stream index*/, DynamicVertex::GetBindingInfo(),
                        make_ref(dynData.data()));

    m2::PointF buttonSize(halfWM + halfWM, halfHM + halfHM);
    ASSERT(params.m_bodyHandleCreator, ());
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertTriangleStrip(state, make_ref(&provider),
                                params.m_bodyHandleCreator(params.m_anchor, buttonSize,
                                                           normalColor, pressedColor));
  }

  // Cache text
  {
    size_t vertexCount = result.m_buffer.size();
    ASSERT(vertexCount % dp::Batcher::VertexPerQuad == 0, ());
    size_t indexCount = dp::Batcher::IndexPerQuad * vertexCount / dp::Batcher::VertexPerQuad;

    dp::AttributeProvider provider(1 /*stream count*/, vertexCount);
    provider.InitStream(0 /*stream index*/, StaticLabel::Vertex::GetBindingInfo(),
                        make_ref(result.m_buffer.data()));

    ASSERT(params.m_labelHandleCreator, ());
    m2::PointF textSize(result.m_boundRect.SizeX(), result.m_boundRect.SizeY());

    dp::Batcher batcher(indexCount, vertexCount);
    dp::SessionGuard guard(batcher, bind(&ShapeControl::AddShape, &control, _1, _2));
    batcher.InsertListOfStrip(result.m_state, make_ref(&provider),
                              params.m_labelHandleCreator(params.m_anchor, textSize),
                              dp::Batcher::VertexPerQuad);
  }
}

dp::BindingInfo const & Button::StaticVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(2));

    dp::BindingDecl & posDecl = info->GetBindingDecl(0);
    posDecl.m_attributeName = "a_position";
    posDecl.m_componentCount = 3;
    posDecl.m_componentType = gl_const::GLFloatType;
    posDecl.m_offset = 0;
    posDecl.m_stride = sizeof(StaticVertex);

    dp::BindingDecl & normalDecl = info->GetBindingDecl(1);
    normalDecl.m_attributeName = "a_normal";
    normalDecl.m_componentCount = 2;
    normalDecl.m_componentType = gl_const::GLFloatType;
    normalDecl.m_offset = sizeof(glsl::vec3);
    normalDecl.m_stride = posDecl.m_stride;
  }

  return *info.get();
}

dp::BindingInfo const & Button::DynamicVertex::GetBindingInfo()
{
  static unique_ptr<dp::BindingInfo> info;

  if (info == nullptr)
  {
    info.reset(new dp::BindingInfo(1, 1));

    dp::BindingDecl & colorTexCoordDecl = info->GetBindingDecl(0);
    colorTexCoordDecl.m_attributeName = "a_colorTexCoords";
    colorTexCoordDecl.m_componentCount = 2;
    colorTexCoordDecl.m_componentType = gl_const::GLFloatType;
    colorTexCoordDecl.m_offset = 0;
    colorTexCoordDecl.m_stride = sizeof(DynamicVertex);
  }

  return *info.get();
}

}
