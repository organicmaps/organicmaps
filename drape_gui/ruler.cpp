#include "ruler.hpp"
#include "ruler_helper.hpp"
#include "gui_text.hpp"
#include "drape_gui.hpp"

#include "../drape/glsl_func.hpp"
#include "../drape/glsl_types.hpp"
#include "../drape/shader_def.hpp"

namespace gui
{

namespace
{

static size_t const FontSize = 7;
static dp::Color const FontColor = dp::Color(0x4D, 0x4D, 0x4D, 0xCC);

struct RulerVertex
{
  RulerVertex() = default;
  RulerVertex(glsl::vec2 const & pos, glsl::vec2 const & normal, glsl::vec2 const & texCoord)
    : m_position(pos)
    , m_normal(normal)
    , m_texCoord(texCoord)
  {
  }

  glsl::vec2 m_position;
  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetBindingInfo()
{
  dp::BindingInfo info(3);
  dp::BindingDecl & posDecl = info.GetBindingDecl(0);
  posDecl.m_attributeName = "a_position";
  posDecl.m_componentCount = 2;
  posDecl.m_componentType = gl_const::GLFloatType;
  posDecl.m_offset = 0;
  posDecl.m_stride = sizeof(RulerVertex);

  dp::BindingDecl & normalDecl = info.GetBindingDecl(1);
  normalDecl.m_attributeName = "a_normal";
  normalDecl.m_componentCount = 2;
  normalDecl.m_componentType = gl_const::GLFloatType;
  normalDecl.m_offset = sizeof(glsl::vec2);
  normalDecl.m_stride = posDecl.m_stride;

  dp::BindingDecl & texDecl = info.GetBindingDecl(2);
  texDecl.m_attributeName = "a_colorTexCoords";
  texDecl.m_componentCount = 2;
  texDecl.m_componentType = gl_const::GLFloatType;
  texDecl.m_offset = 2 * sizeof(glsl::vec2);
  texDecl.m_stride = posDecl.m_stride;

  return info;
}

class RulerHandle : public Handle
{
public:
  RulerHandle(dp::Anchor anchor)
    : Handle(anchor, m2::PointF(0.0f, 0.0f))
  {
    SetIsVisible(true);
  }

  virtual void Update(ScreenBase const & screen) override
  {
    RulerHelper & helper = RulerHelper::Instance();
    SetIsVisible(helper.IsVisible(screen));
    if (IsVisible())
      m_uniforms.SetFloatValue("u_length", helper.GetRulerPixelLength());
  }

  virtual bool IndexesRequired() const override { return false; }

  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const override
  {
    return m2::RectD();
  }

  virtual void GetPixelShape(ScreenBase const & screen, Rects & rects) const override
  {
    UNUSED_VALUE(screen);
    UNUSED_VALUE(rects);
  }
};

class RulerTextHandle : public Handle
{
public:
  RulerTextHandle(dp::Anchor anchor, m2::PointF const & pivot)
    : Handle(anchor, pivot)
  {
    SetIsVisible(true);
    m_textView.Reset(new MutableLabel(anchor));
  }

  ~RulerTextHandle()
  {
    m_textView.Destroy();
  }

  virtual void Update(ScreenBase const & screen) override
  {
    UNUSED_VALUE(screen);

    SetIsVisible(RulerHelper::Instance().IsVisible(screen));

    if (!IsVisible())
      return;

    glsl::mat4 m = glsl::transpose(glsl::translate(glsl::mat4(), glsl::vec3(m_pivot, 0.0)));
    m_uniforms.SetMatrix4x4Value("modelView", glsl::value_ptr(m));
  }

  virtual void GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator, ScreenBase const & screen) const override
  {
    UNUSED_VALUE(screen);

    RulerHelper const & helper = RulerHelper::Instance();
    if (!helper.IsTextDirty())
      return;

    buffer_vector<MutableLabel::DynamicVertex, 128> buffer;
    m_textView->SetText(buffer, helper.GetRulerText());

    size_t byteCount = buffer.size() * sizeof(MutableLabel::DynamicVertex);
    void * dataPointer = mutator->AllocateMutationBuffer(byteCount);
    memcpy(dataPointer, buffer.data(), byteCount);

    dp::OverlayHandle::TOffsetNode offsetNode = GetOffsetNode(MutableLabel::DynamicVertex::GetBindingInfo().GetID());

    dp::MutateNode mutateNode;
    mutateNode.m_data = dp::MakeStackRefPointer(dataPointer);
    mutateNode.m_region = offsetNode.second;
    mutator->AddMutation(offsetNode.first, mutateNode);
  }

  dp::RefPointer<MutableLabel> GetTextView()
  {
    return m_textView.GetRefPointer();
  }

private:
  dp::MasterPointer<MutableLabel> m_textView;
};

}

dp::TransferPointer<ShapeRenderer> Ruler::Draw(dp::RefPointer<dp::TextureManager> tex) const
{
  ShapeRenderer * renderer = new ShapeRenderer;
  DrawRuler(renderer, tex);
  DrawText(renderer, tex);

  return dp::MovePointer(renderer);
}

void Ruler::DrawRuler(ShapeRenderer * renderer, dp::RefPointer<dp::TextureManager> tex) const
{
  buffer_vector<RulerVertex, 4> data;

  dp::TextureManager::ColorRegion reg;
  tex->GetColorRegion(dp::Color(0x4D, 0x4D, 0x4D, 0xCC), reg);

  glsl::vec2 pivot = glsl::ToVec2(m_position.m_pixelPivot);
  glsl::vec2 texCoord = glsl::ToVec2(reg.GetTexRect().Center());
  float h = RulerHelper::Instance().GetRulerHalfHeight();

  glsl::vec2 normals[] =
  {
    glsl::vec2(-1.0, 0.0),
    glsl::vec2(1.0, 0.0),
  };

  dp::Anchor anchor = m_position.m_anchor;
  if (anchor & dp::Left)
    normals[0] = glsl::vec2(0.0, 0.0);
  else if (anchor & dp::Right)
    normals[1] = glsl::vec2(0.0, 0.0);

  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, h), normals[0], texCoord));
  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, -h), normals[0], texCoord));
  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, h), normals[1], texCoord));
  data.push_back(RulerVertex(pivot + glsl::vec2(0.0, -h), normals[1], texCoord));

  dp::GLState state(gpu::RULER_PROGRAM, dp::GLState::UserMarkLayer);
  state.SetColorTexture(reg.GetTexture());

  dp::AttributeProvider provider(1, 4);
  provider.InitStream(0, GetBindingInfo(), dp::MakeStackRefPointer<void>(data.data()));

  dp::Batcher batcher(6, 4);
  batcher.StartSession(renderer->GetFlushRoutine());
  batcher.InsertTriangleStrip(state, dp::MakeStackRefPointer(&provider),
                              dp::MovePointer<dp::OverlayHandle>(new RulerHandle(m_position.m_anchor)));
  batcher.EndSession();
}

void Ruler::DrawText(ShapeRenderer * renderer, dp::RefPointer<dp::TextureManager> tex) const
{
  string alphabet;
  size_t maxTextLength;
  RulerHelper::Instance().GetTextInitInfo(alphabet, maxTextLength);

  uint32_t vertexCount = 4 * maxTextLength;
  uint32_t indexCount = 6 * maxTextLength;

  m2::PointF rulerTextPivot = m_position.m_pixelPivot + m2::PointF(0.0, RulerHelper::Instance().GetVerticalTextOffset());
  dp::Anchor anchor = static_cast<dp::Anchor>((m_position.m_anchor & (dp::Right | dp::Left)) | dp::Bottom);
  RulerTextHandle * handle = new RulerTextHandle(anchor, rulerTextPivot);
  dp::RefPointer<MutableLabel> textView = handle->GetTextView();
  dp::RefPointer<dp::Texture> maskTexture = textView->SetAlphabet(alphabet, tex);
  textView->SetMaxLength(maxTextLength);

  buffer_vector<MutableLabel::StaticVertex, 128> statData;
  buffer_vector<MutableLabel::DynamicVertex, 128> dynData;
  dp::RefPointer<dp::Texture> colorTexture = textView->Precache(statData, dp::FontDecl(FontColor, FontSize * DrapeGui::Instance().GetScaleFactor()), tex);

  ASSERT_EQUAL(vertexCount, statData.size(), ());
  dynData.resize(statData.size());

  dp::AttributeProvider provider(2, statData.size());
  provider.InitStream(0, MutableLabel::StaticVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(statData.data()));
  provider.InitStream(1, MutableLabel::DynamicVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(dynData.data()));

  dp::GLState state(gpu::TEXT_PROGRAM, dp::GLState::UserMarkLayer);
  state.SetColorTexture(colorTexture);
  state.SetMaskTexture(maskTexture);

  dp::Batcher batcher(indexCount, vertexCount);
  batcher.StartSession(renderer->GetFlushRoutine());
  batcher.InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), dp::MovePointer<dp::OverlayHandle>(handle), 4);
  batcher.EndSession();
}

}
