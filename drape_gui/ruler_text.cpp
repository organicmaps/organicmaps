#include "ruler_text.hpp"
#include "ruler_helper.hpp"
#include "drape_gui.hpp"

#include "../drape/glsl_func.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/shader_def.hpp"

namespace gui
{

static size_t const FontSize = 7;
static dp::Color const FontColor = dp::Color(0x4D, 0x4D, 0x4D, 0xCC);

namespace
{
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

RulerText::RulerText()
{
  RulerHelper::Instance().GetTextInitInfo(m_alphabet, m_maxLength);
}

void RulerText::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> tex) const
{
  m2::PointF rulerTextPivot = m_position.m_pixelPivot + m2::PointF(0.0, RulerHelper::Instance().GetVerticalTextOffset());
  dp::Anchor anchor = static_cast<dp::Anchor>((m_position.m_anchor & (dp::Right | dp::Left)) | dp::Bottom);
  RulerTextHandle * handle = new RulerTextHandle(anchor, rulerTextPivot);
  dp::RefPointer<MutableLabel> textView = handle->GetTextView();
  dp::RefPointer<dp::Texture> maskTexture = textView->SetAlphabet(m_alphabet, tex);
  textView->SetMaxLength(m_maxLength);

  buffer_vector<MutableLabel::StaticVertex, 128> statData;
  buffer_vector<MutableLabel::DynamicVertex, 128> dynData;
  dp::RefPointer<dp::Texture> colorTexture = textView->Precache(statData, dp::FontDecl(FontColor, FontSize * DrapeGui::Instance().GetScaleFactor()), tex);

  ASSERT_EQUAL(GetVertexCount(), statData.size(), ());
  dynData.resize(statData.size());

  dp::AttributeProvider provider(2, statData.size());
  provider.InitStream(0, MutableLabel::StaticVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(statData.data()));
  provider.InitStream(1, MutableLabel::DynamicVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(dynData.data()));

  dp::GLState state(gpu::TEXT_PROGRAM, dp::GLState::UserMarkLayer);
  state.SetColorTexture(colorTexture);
  state.SetMaskTexture(maskTexture);

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), dp::MovePointer<dp::OverlayHandle>(handle), 4);
}

uint16_t RulerText::GetVertexCount() const
{
  return 4 * m_maxLength;
}

uint16_t RulerText::GetIndexCount() const
{
  return 6 * m_maxLength;
}

}
