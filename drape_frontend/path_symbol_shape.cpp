#include "path_symbol_shape.hpp"
#include "visual_params.hpp"

#include "../drape/overlay_handle.hpp"
#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"

namespace df
{

using m2::PointF;
using m2::Spline;
using glsl_types::vec2;
using glsl_types::vec4;

class PathSymbolHandle : public dp::OverlayHandle
{
public:
  static const uint8_t PositionAttributeID = 1;
  PathSymbolHandle(m2::SharedSpline const & spl, PathSymbolViewParams const & params, int maxCount, float hw, float hh)
    : OverlayHandle(FeatureID(), dp::Center, 0.0f),
      m_params(params), m_spline(spl), m_scaleFactor(1.0f),
      m_positions(maxCount * 6), m_maxCount(maxCount),
      m_symbolHalfWidth(hw), m_symbolHalfHeight(hh)
  {
    SetIsVisible(true);
  }

  virtual void Update(ScreenBase const & screen)
  {
    m_scaleFactor = screen.GetScale();

    Spline::iterator itr = m_spline.CreateIterator();
    itr.Step((m_params.m_offset + m_symbolHalfWidth) * m_scaleFactor);

    for (int i = 0; i < m_maxCount * 6; ++i)
      m_positions[i] = vec2(0.0f, 0.0f);

    vec2 * it = &m_positions[0];

    for (int i = 0; i < m_maxCount; ++i)
    {
      if (itr.BeginAgain())
        break;
      PointF const pos = itr.m_pos;
      PointF const dir = itr.m_dir * m_symbolHalfWidth  * m_scaleFactor;
      PointF const norm(-itr.m_dir.y * m_symbolHalfHeight * m_scaleFactor, itr.m_dir.x * m_symbolHalfHeight * m_scaleFactor);

      *it = pos - dir + norm; it++;
      *it = pos - dir - norm; it++;
      *it = pos + dir + norm; it++;
      *it = pos + dir - norm; it++;

      itr.Step(m_params.m_step * m_scaleFactor);
    }
  }

  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const
  {
    return m2::RectD(0, 0, 0, 0);
  }

  virtual void GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator) const
  {
    TOffsetNode const & node = GetOffsetNode(PositionAttributeID);
    dp::MutateNode mutateNode;
    mutateNode.m_region = node.second;
    mutateNode.m_data = dp::MakeStackRefPointer<void>(&m_positions[0]);
    mutator->AddMutation(node.first, mutateNode);
  }

private:
  PathSymbolViewParams m_params;
  m2::SharedSpline m_spline;
  float m_scaleFactor;
  mutable vector<vec2> m_positions;
  int const m_maxCount;
  float m_symbolHalfWidth;
  float m_symbolHalfHeight;
};

PathSymbolShape::PathSymbolShape(m2::SharedSpline const & spline, PathSymbolViewParams const & params, float maxScale)
  : m_params(params)
  , m_spline(spline)
  , m_maxScale(1.0f / maxScale)
{
}

void PathSymbolShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  int maxCount = (m_spline->GetLength() * m_maxScale - m_params.m_offset) / m_params.m_step + 1;
  if (maxCount <= 0)
    return;

  int const vertCnt = maxCount * 4;
  vector<vec2> positions(vertCnt, vec2(0.0f, 0.0f));
  vector<vec4> uvs(vertCnt);
  vec4 * tc = &uvs[0];
  dp::TextureSetHolder::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);

  m2::RectF const & rect = region.GetTexRect();
  float textureNum = (float)region.GetTextureNode().m_textureOffset;
  m2::PointU pixelSize;
  region.GetPixelSize(pixelSize);

  for(int i = 0; i < maxCount; ++i)
  {
    *tc = vec4(rect.minX(), rect.maxY(), textureNum, m_params.m_depth);
    tc++;
    *tc = vec4(rect.minX(), rect.minY(), textureNum, m_params.m_depth);
    tc++;
    *tc = vec4(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth);
    tc++;
    *tc = vec4(rect.maxX(), rect.minY(), textureNum, m_params.m_depth);
    tc++;
  }

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::DynamicGeometry);
  state.SetTextureSet(region.GetTextureNode().m_textureSet);
  state.SetBlending(dp::Blending(true));

  dp::AttributeProvider provider(3, vertCnt);
  {
    dp::BindingInfo position(1, PathSymbolHandle::PositionAttributeID);
    dp::BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, dp::MakeStackRefPointer(&positions[0]));
  }
  {
    dp::BindingInfo normal(1);
    dp::BindingDecl & decl = normal.GetBindingDecl(0);
    decl.m_attributeName = "a_normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, normal, dp::MakeStackRefPointer(&positions[0]));
  }
  {
    dp::BindingInfo texcoord(1);
    dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texCoords";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, texcoord, dp::MakeStackRefPointer(&uvs[0]));
  }

  dp::OverlayHandle * handle = new PathSymbolHandle(m_spline, m_params, maxCount, pixelSize.x / 2.0f, pixelSize.y / 2.0f);
  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(handle), 4);
}

}
