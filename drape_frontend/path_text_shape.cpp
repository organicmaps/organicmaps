#include "path_text_shape.hpp"
#include "text_layout.hpp"
#include "visual_params.hpp"
#include "intrusive_vector.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"

#include "../base/math.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"
#include "../base/string_utils.hpp"
#include "../base/timer.hpp"
#include "../base/matrix.hpp"

#include "../geometry/transformations.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

using m2::Spline;
using glsl_types::vec2;

namespace
{
  struct AccumulateRect
  {
    ScreenBase const & m_screen;
    m2::RectD m_pixelRect;
    AccumulateRect(ScreenBase const & screen)
      : m_screen(screen)
    {
    }

    void operator()(m2::PointF const & pt)
    {
      m_pixelRect.Add(m_screen.GtoP(pt));
    }

  };

  class PathTextHandle : public dp::OverlayHandle
  {
  public:
    static const uint8_t PathGlyphPositionID = 1;

    PathTextHandle(m2::SharedSpline const & spl,
                   df::SharedTextLayout const & layout,
                   float const mercatorOffset,
                   float const depth)
      : OverlayHandle(FeatureID(), dp::Center, depth)
      , m_spline(spl)
      , m_layout(layout)
      , m_splineOffset(mercatorOffset)
    {
    }

    void Update(ScreenBase const & screen)
    {
      m_scalePtoG = screen.GetScale();
      GetBegEnd(m_begin, m_end);

      SetIsValid(!m_begin.BeginAgain() && !m_end.BeginAgain());
      if (!IsValid())
        return;

      if (screen.GtoP(m_end.m_pos).x < screen.GtoP(m_begin.m_pos).x)
        m_isForward = false;
      else
        m_isForward = true;
    }

    m2::RectD GetPixelRect(ScreenBase const & screen) const
    {
      ASSERT(IsValid(), ());
      ASSERT(!m_begin.BeginAgain(), ());
      ASSERT(!m_end.BeginAgain(), ());

      AccumulateRect f(screen);
      m_spline->ForEachNode(m_begin, m_end, f);
      float const pixelHeight = m_layout->GetPixelHeight();
      f.m_pixelRect.Inflate(2 * pixelHeight, 2 * pixelHeight);
      return f.m_pixelRect;
    }


    vector<m2::RectF> & GetPixelShape(ScreenBase const & screen)
    {
      return m_bboxes;
    }

    void GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator, ScreenBase const & screen) const
    {
      ASSERT(IsValid(), ());
      uint32_t byteCount = 4 * m_layout->GetGlyphCount() * sizeof(glsl_types::vec2);
      void * buffer = mutator->AllocateMutationBuffer(byteCount);
      df::IntrusiveVector<glsl_types::vec2> positions(buffer, byteCount);
      // m_splineOffset set offset to Center of text.
      // By this we calc offset for start of text in mercator
      m_layout->LayoutPathText(m_begin, m_scalePtoG, positions, m_isForward, m_bboxes, screen);

      TOffsetNode const & node = GetOffsetNode(PathGlyphPositionID);
      dp::MutateNode mutateNode;
      mutateNode.m_region = node.second;
      mutateNode.m_data = dp::MakeStackRefPointer<void>(buffer);
      mutator->AddMutation(node.first, mutateNode);
    }

  private:
    void GetBegEnd(Spline::iterator & beg, Spline::iterator & end) const
    {
      beg = m_spline.CreateIterator();
      end = m_spline.CreateIterator();
      float const textLength = m_layout->GetPixelLength() * m_scalePtoG;
      float const step = max(0.0f, m_splineOffset - textLength / 2.0f);
      if (step > 0.0f)
        beg.Step(step);
      end.Step(step + textLength);
    }

  private:
    m2::SharedSpline m_spline;
    m2::Spline::iterator m_begin;
    m2::Spline::iterator m_end;
    mutable vector<m2::RectF> m_bboxes;

    df::SharedTextLayout m_layout;
    float m_scalePtoG;
    float m_splineOffset;
    bool m_isForward;
  };

  void BatchPathText(m2::SharedSpline const & spline,
                      buffer_vector<float, 32> const & offsets,
                      float depth,
                      dp::RefPointer<dp::Batcher> batcher,
                      df::SharedTextLayout const & layout,
                      vector<glsl_types::vec2> & positions,
                      vector<glsl_types::Quad4> & texCoord,
                      vector<glsl_types::Quad4> & fontColor,
                      vector<glsl_types::Quad4> & outlineColor)
  {
    ASSERT(!offsets.empty(), ());
    layout->InitPathText(depth, texCoord, fontColor, outlineColor);

    dp::GLState state(gpu::PATH_FONT_PROGRAM, dp::GLState::OverlayLayer);
    state.SetTextureSet(layout->GetTextureSet());
    state.SetBlending(dp::Blending(true));

    for (size_t i = 0; i < offsets.size(); ++i)
    {
      dp::AttributeProvider provider(4, 4 * layout->GetGlyphCount());
      {
        dp::BindingInfo positionBind(1, PathTextHandle::PathGlyphPositionID);
        dp::BindingDecl & decl = positionBind.GetBindingDecl(0);
        decl.m_attributeName = "a_position";
        decl.m_componentCount = 2;
        decl.m_componentType = gl_const::GLFloatType;
        decl.m_offset = 0;
        decl.m_stride = 0;
        provider.InitStream(0, positionBind, dp::MakeStackRefPointer(&positions[0]));
      }
      {
        dp::BindingInfo texCoordBind(1);
        dp::BindingDecl & decl = texCoordBind.GetBindingDecl(0);
        decl.m_attributeName = "a_texcoord";
        decl.m_componentCount = 4;
        decl.m_componentType = gl_const::GLFloatType;
        decl.m_offset = 0;
        decl.m_stride = 0;
        provider.InitStream(1, texCoordBind, dp::MakeStackRefPointer(&texCoord[0]));
      }
      {
        dp::BindingInfo fontColorBind(1);
        dp::BindingDecl & decl = fontColorBind.GetBindingDecl(0);
        decl.m_attributeName = "a_color";
        decl.m_componentCount = 4;
        decl.m_componentType = gl_const::GLFloatType;
        decl.m_offset = 0;
        decl.m_stride = 0;
        provider.InitStream(2, fontColorBind, dp::MakeStackRefPointer(&fontColor[0]));
      }
      {
        dp::BindingInfo outlineColorBind(1);
        dp::BindingDecl & decl = outlineColorBind.GetBindingDecl(0);
        decl.m_attributeName = "a_outline_color";
        decl.m_componentCount = 4;
        decl.m_componentType = gl_const::GLFloatType;
        decl.m_offset = 0;
        decl.m_stride = 0;
        provider.InitStream(3, outlineColorBind, dp::MakeStackRefPointer(&outlineColor[0]));
      }

      dp::OverlayHandle * handle = new PathTextHandle(spline, layout, offsets[i], depth);
      batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(handle), 4);
    }
  }
}

namespace df
{

PathTextShape::PathTextShape(m2::SharedSpline const & spline,
                             PathTextViewParams const & params,
                             float const scaleGtoP)
  : m_spline(spline)
  , m_params(params)
  , m_scaleGtoP(scaleGtoP)
{
}

void PathTextShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  SharedTextLayout layout(new TextLayout(strings::MakeUniString(m_params.m_text),
                                         m_params.m_textFont,
                                         textures));

  uint32_t glyphCount = layout->GetGlyphCount();
  if (glyphCount == 0)
    return;

  //we leave a little space on either side of the text that would
  //remove the comparison for equality of spline portions
  float const TextBorder = 4.0f;
  float const textLength = TextBorder + layout->GetPixelLength();
  float const textHalfLength = textLength / 2.0f;
  float const pathGlbLength = m_spline->GetLength();

  // on next readable scale m_scaleGtoP will be twice
  if (textLength > pathGlbLength * 2 * m_scaleGtoP)
    return;

  float const pathLength = m_scaleGtoP * m_spline->GetLength();

  /// copied from old code
  /// @todo Choose best constant for minimal space.
  float const etalonEmpty = max(200 * df::VisualParams::Instance().GetVisualScale(), (double)textLength);
  float const minPeriodSize = etalonEmpty + textLength;
  float const twoTextAndEmpty = minPeriodSize + textLength;

  vector<glsl_types::vec2>  positions(glyphCount, vec2(0.0, 0.0));
  vector<glsl_types::Quad4> texCoords(glyphCount);
  vector<glsl_types::Quad4> fontColor(glyphCount);
  vector<glsl_types::Quad4> outlineColor(glyphCount);
  buffer_vector<float, 32> offsets;

  float const scalePtoG = 1.0f / m_scaleGtoP;

  if (pathLength < twoTextAndEmpty)
  {
    // if we can't place 2 text and empty part on path
    // we place only one text on center of path
    offsets.push_back(pathGlbLength / 2.0f);

  }
  else if (pathLength < twoTextAndEmpty + minPeriodSize)
  {
    // if we can't place 3 text and 2 empty path
    // we place 2 text with empty space beetwen
    // and some offset from path end
    float const endOffset = (pathLength - (2 * textLength + etalonEmpty)) / 2;

    // division on m_scaleGtoP give as global coord frame (Mercator)
    offsets.push_back((endOffset + textHalfLength) * scalePtoG);
    offsets.push_back((pathLength - (textHalfLength + endOffset)) * scalePtoG);
  }
  else
  {
    // here we place 2 text on the ends of path
    // then we place as much as possible text on center path uniformly
    offsets.push_back(textHalfLength * scalePtoG);
    offsets.push_back((pathLength - textHalfLength) * scalePtoG);
    float const emptySpace = pathLength - 2 * textLength;
    uint32_t textCount = static_cast<uint32_t>(ceil(emptySpace / minPeriodSize));
    float const offset = (emptySpace - textCount * textLength) / (textCount + 1);
    for (size_t i = 0; i < textCount; ++i)
      offsets.push_back((textHalfLength + (textLength + offset) * (i + 1)) * scalePtoG);
  }

  BatchPathText(m_spline, offsets, m_params.m_depth, batcher, layout,
                positions, texCoords, fontColor, outlineColor);
}

}
