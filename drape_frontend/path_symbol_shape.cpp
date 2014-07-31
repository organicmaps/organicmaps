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

class PathSymbolHandle : public OverlayHandle
{
public:
  static const uint8_t PositionAttributeID = 1;
  PathSymbolHandle(m2::Spline const & spl, PathSymbolViewParams const & params, int maxCount, float hw, float hh)
    : OverlayHandle(FeatureID(), dp::Center, 0.0f),
      m_params(params), m_path(spl), m_scaleFactor(1.0f),
      m_positions(maxCount * 6), m_maxCount(maxCount),
      m_symbolHalfWidth(hw), m_symbolHalfHeight(hh) {}

  virtual void Update(ScreenBase const & screen);
  virtual m2::RectD GetPixelRect(ScreenBase const & screen) const;
  virtual void GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const;

private:
  PathSymbolViewParams m_params;
  m2::Spline m_path;
  float m_scaleFactor;
  mutable vector<vec2> m_positions;
  int const m_maxCount;
  float m_symbolHalfWidth;
  float m_symbolHalfHeight;
};

PathSymbolShape::PathSymbolShape(vector<PointF> const & path, PathSymbolViewParams const & params, float maxScale)
  : m_params(params), m_maxScale(1.0f / maxScale)
{
  m_path.FromArray(path);
}

void PathSymbolShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  int maxCount = (m_path.GetLength() * m_maxScale - m_params.m_offset) / m_params.m_step + 1;
  if (maxCount <= 0)
    return;

  int const vertCnt = maxCount * 6;
  vector<vec2> positions(vertCnt, vec2(0.0f, 0.0f));
  vector<vec4> uvs(vertCnt);
  vec4 * tc = &uvs[0];
  TextureSetHolder::SymbolRegion region;
  textures->GetSymbolRegion(m_params.m_symbolName, region);

  GLState state(gpu::TEXTURING_PROGRAM, GLState::OverlayLayer);
  state.SetTextureSet(region.GetTextureNode().m_textureSet);
  state.SetBlending(Blending(true));

  m2::RectF const & rect = region.GetTexRect();
  float textureNum = (float)region.GetTextureNode().m_textureOffset;
  m2::PointU pixelSize;
  region.GetPixelSize(pixelSize);

  for(int i = 0; i < maxCount; ++i)
  {
    *tc = vec4(rect.minX(), rect.minY(), textureNum, m_params.m_depth); tc++;
    *tc = vec4(rect.minX(), rect.maxY(), textureNum, m_params.m_depth); tc++;
    *tc = vec4(rect.maxX(), rect.minY(), textureNum, m_params.m_depth); tc++;
    *tc = vec4(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth); tc++;
    *tc = vec4(rect.maxX(), rect.minY(), textureNum, m_params.m_depth); tc++;
    *tc = vec4(rect.minX(), rect.maxY(), textureNum, m_params.m_depth); tc++;
  }

  AttributeProvider provider(3, vertCnt);
  {
    BindingInfo position(1, PathSymbolHandle::PositionAttributeID);
    BindingDecl & decl = position.GetBindingDecl(0);
    decl.m_attributeName = "a_position";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(0, position, MakeStackRefPointer(&positions[0]));
  }
  {
    BindingInfo normal(1);
    BindingDecl & decl = normal.GetBindingDecl(0);
    decl.m_attributeName = "a_normal";
    decl.m_componentCount = 2;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(1, normal, MakeStackRefPointer(&positions[0]));
  }
  {
    BindingInfo texcoord(1);
    BindingDecl & decl = texcoord.GetBindingDecl(0);
    decl.m_attributeName = "a_texCoords";
    decl.m_componentCount = 4;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;
    provider.InitStream(2, texcoord, MakeStackRefPointer(&uvs[0]));
  }

  OverlayHandle * handle = new PathSymbolHandle(m_path, m_params, maxCount, pixelSize.x / 2.0f, pixelSize.y / 2.0f);
  batcher->InsertTriangleList(state, MakeStackRefPointer(&provider), MovePointer(handle));
}

m2::RectD PathSymbolHandle::GetPixelRect(ScreenBase const & screen) const
{
  return m2::RectD(0, 0, 0, 0);
}

void PathSymbolHandle::Update(ScreenBase const & screen)
{
  m_scaleFactor = screen.GetScale();

  Spline::iterator itr;
  itr.Attach(m_path);
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

    PointF const p1 = pos + dir - norm;
    PointF const p2 = pos - dir - norm;
    PointF const p3 = pos - dir + norm;
    PointF const p4 = pos + dir + norm;

    *it = p2; it++;
    *it = p3; it++;
    *it = p1; it++;
    *it = p4; it++;
    *it = p1; it++;
    *it = p3; it++;

    itr.Step(m_params.m_step * m_scaleFactor);
  }
}

void PathSymbolHandle::GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const
{
  TOffsetNode const & node = GetOffsetNode(PositionAttributeID);
  MutateNode mutateNode;
  mutateNode.m_region = node.second;
  mutateNode.m_data = MakeStackRefPointer<void>(&m_positions[0]);
  mutator->AddMutation(node.first, mutateNode);
}

}
