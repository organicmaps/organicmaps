#include "path_symbol_shape.hpp"

#include "../drape/shader_def.hpp"
#include "../drape/attribute_provider.hpp"
#include "../drape/glstate.hpp"
#include "../drape/batcher.hpp"
#include "../drape/texture_set_holder.hpp"
#include "visual_params.hpp"

namespace df
{

using m2::PointF;
using m2::Spline;

struct TexCoord
{
  TexCoord() {}
  TexCoord(m2::PointF const & tex, float index = 0.0f, float depth = 0.0f)
    : m_texCoord(tex), m_index(index), m_depth(depth) {}

  TexCoord(float texX, float texY, float index = 0.0f, float depth = 0.0f)
    : m_texCoord(texX, texY), m_index(index), m_depth(depth) {}

  m2::PointF m_texCoord;
  float m_index;
  float m_depth;
};


PathSymbolShape::PathSymbolShape(vector<PointF> const & path, PathSymbolViewParams const & params, float maxScale)
  : m_params(params), m_maxScale(1.0f / maxScale)
{
  m_path.FromArray(path);
}

void PathSymbolShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  int maxCount = (m_path.getLength() * m_maxScale - m_params.m_OffsetStart) / m_params.m_Offset + 1;
  maxCount = max(0, maxCount);
  if (!maxCount)
    return;
  vector<Position> positions(maxCount * 6);
  vector<TexCoord> uvs(maxCount * 6);
  vector<Position> normals(maxCount * 6, Position(0.0f, 0.0f));
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
    int index = i * 6;
    positions[index] = Position(0.0f, 0.0f);
    uvs[index++] = TexCoord(rect.minX(), rect.minY(), textureNum, m_params.m_depth);
    positions[index] = Position(0.0f, 0.0f);
    uvs[index++] = TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth);
    positions[index] = Position(0.0f, 0.0f);
    uvs[index++] = TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth);

    positions[index] = Position(0.0f, 0.0f);
    uvs[index++] = TexCoord(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth);
    positions[index] = Position(0.0f, 0.0f);
    uvs[index++] = TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth);
    positions[index] = Position(0.0f, 0.0f);
    uvs[index++] = TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth);
  }

  AttributeProvider provider(3, maxCount * 6);
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
    provider.InitStream(1, normal, MakeStackRefPointer(&normals[0]));
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

  pixelSize *= 1.0f / df::VisualParams::Instance().GetVisualScale();
  OverlayHandle * handle = new PathSymbolHandle(m_path, m_params, maxCount, pixelSize.x / 2.0f, pixelSize.y / 2.0f);
  batcher->InsertTriangleList(state, MakeStackRefPointer(&provider), MovePointer(handle));
}

m2::RectD PathSymbolHandle::GetPixelRect(ScreenBase const & screen) const
{
  return m2::RectD();
}

void PathSymbolHandle::Update(ScreenBase const & screen)
{
  m_scaleFactor = screen.GetScale();

  Spline::iterator itr;
  itr.Attach(m_path);
  itr.Step((m_params.m_OffsetStart + m_symbolHalfWidth) * m_scaleFactor);

  for (int i = 0; i < m_maxCount * 6; ++i)
    m_positions[i] = Position(0.0f, 0.0f);

  for (int i = 0; i < m_maxCount; ++i)
  {
    if (itr.beginAgain())
      break;
    PointF const pos = itr.m_pos;
    PointF const dir = itr.m_dir * m_symbolHalfWidth  * m_scaleFactor;
    PointF const norm(-dir.y * m_symbolHalfHeight / m_symbolHalfWidth, dir.x * m_symbolHalfHeight / m_symbolHalfWidth);

    PointF const p1 = pos + dir - norm;
    PointF const p2 = pos - dir - norm;
    PointF const p3 = pos - dir + norm;
    PointF const p4 = pos + dir + norm;

    int index = i * 6;
    m_positions[index++] = Position(p2);
    m_positions[index++] = Position(p3);
    m_positions[index++] = Position(p1);

    m_positions[index++] = Position(p4);
    m_positions[index++] = Position(p1);
    m_positions[index++] = Position(p3);

    itr.Step(m_params.m_Offset * m_scaleFactor);
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
