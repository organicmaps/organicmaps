#include "path_text_shape.hpp"

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

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

namespace df
{

using m2::PointF;

namespace
{
  static float const realFontSize = 20.0f;

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

  struct Position
  {
    Position() {}
    Position(float x, float y) : m_x(x), m_y(y){}

    float m_x;
    float m_y;
  };

  struct Buffer
  {
    vector<Position>    m_pos;
    vector<TexCoord>    m_uvs;
    vector<ColorF>      m_baseColor;
    vector<ColorF>      m_outlineColor;
    vector<LetterInfo>  m_info;
    float               m_offset;
  };
}

PathTextShape::PathTextShape(vector<PointF> const & path, PathTextViewParams const & params)
  : m_params(params)
{
  m_path.FromArray(path);
}

void PathTextShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  strings::UniString const text = strings::MakeUniString(m_params.m_Text);
  float const fontSize = m_params.m_TextFont.m_size;

  // Fill buffers
  int const cnt = text.size();
  vector<Buffer> buffers(1);
  float const needOutline = m_params.m_TextFont.m_needOutline;

  int textureSet;
  for (int i = 0; i < cnt; i++)
  {
    TextureSetHolder::GlyphRegion region;
    textures->GetGlyphRegion(text[i], region);
    float xOffset, yOffset, advance;
    m2::PointU pixelSize;
    region.GetMetrics(xOffset, yOffset, advance);
    region.GetPixelSize(pixelSize);
    float halfWidth = pixelSize.x / 2.0f;
    float halfHeight = pixelSize.y / 2.0f;
    float const aspect = fontSize / realFontSize;

    textureSet = region.GetTextureNode().m_textureSet;
    if (buffers.size() < textureSet)
      buffers.resize(textureSet + 1);

    Buffer & curBuffer = buffers[textureSet];

    curBuffer.m_info.push_back(
                LetterInfo(xOffset, yOffset, advance + curBuffer.m_offset, halfWidth, halfHeight));

    for (int i = 0; i < buffers.size(); ++i)
      buffers[i].m_offset += advance;

    curBuffer.m_offset = 0;

    advance *= aspect;
    yOffset *= aspect;
    xOffset *= aspect;
    halfWidth *= aspect;
    halfHeight *= aspect;

    m2::RectF const & rect = region.GetTexRect();
    float const textureNum = (region.GetTextureNode().m_textureOffset << 1) + needOutline;

    ColorF const base(m_params.m_TextFont.m_color);
    ColorF const outline(m_params.m_TextFont.m_outlineColor);

    curBuffer.m_uvs.push_back(TexCoord(rect.minX(), rect.minY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth));

    curBuffer.m_uvs.push_back(TexCoord(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth));

    int const newSize = curBuffer.m_baseColor.size() + 6;
    curBuffer.m_baseColor.resize(newSize, base);
    curBuffer.m_outlineColor.resize(newSize, outline);
    curBuffer.m_pos.resize(newSize, Position(0, 0));
  }

  for (int i = 0; i < buffers.size(); ++i)
  {
    if (buffers[i].m_pos.empty())
      continue;

    GLState state(gpu::PATH_FONT_PROGRAM, GLState::OverlayLayer);
    state.SetTextureSet(i);
    state.SetBlending(Blending(true));

    AttributeProvider provider(4, buffers[i].m_pos.size());
    {
      BindingInfo position(1, PathTextHandle::DirectionAttributeID);
      BindingDecl & decl = position.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, position, MakeStackRefPointer(&buffers[i].m_pos[0]));
    }
    {
      BindingInfo texcoord(1);
      BindingDecl & decl = texcoord.GetBindingDecl(0);
      decl.m_attributeName = "a_texcoord";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(1, texcoord, MakeStackRefPointer(&buffers[i].m_uvs[0]));
    }
    {
      BindingInfo base_color(1);
      BindingDecl & decl = base_color.GetBindingDecl(0);
      decl.m_attributeName = "a_color";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(2, base_color, MakeStackRefPointer(&buffers[i].m_baseColor[0]));
    }
    {
      BindingInfo outline_color(1);
      BindingDecl & decl = outline_color.GetBindingDecl(0);
      decl.m_attributeName = "a_outline_color";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(3, outline_color, MakeStackRefPointer(&buffers[i].m_outlineColor[0]));
    }

    OverlayHandle * handle = new PathTextHandle(m_path, m_params, buffers[i].m_info);

    batcher->InsertTriangleList(state, MakeStackRefPointer(&provider), MovePointer(handle));
  }
}

void PathTextHandle::GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const
{
  float entireLength = m_params.m_OffsetStart * m_scaleFactor;

  float const fontSize = m_params.m_TextFont.m_size;
  int const cnt = m_infos.size();
  vector<Position> positions(m_infos.size() * 6);

  Spline::iterator itr;
  itr.Attach(m_path);
  itr.Step(m_params.m_OffsetStart * m_scaleFactor);

  for (int i = 0; i < cnt; i++)
  {
    float xOffset = m_infos[i].m_xOffset;
    float yOffset = m_infos[i].m_yOffset;
    float advance = m_infos[i].m_advance;
    float halfWidth = m_infos[i].m_halfWidth;
    float halfHeight = m_infos[i].m_halfHeight;
    float const aspect = fontSize / realFontSize;
    advance *= aspect;
    yOffset *= aspect;
    xOffset *= aspect;
    halfWidth *= aspect;
    halfHeight *= aspect;

    PointF const pos = itr.m_pos;
    PointF const oldDir = itr.m_dir;

    advance *= m_scaleFactor;
    ASSERT_NOT_EQUAL(advance, 0.0, ());
    entireLength += advance;
    if(entireLength >= m_params.m_OffsetEnd * m_scaleFactor)
      return;
    int const index1 = itr.m_index;
    itr.Step(advance);
    int const index2 = itr.m_index;
    PointF dir;
    if (index1 != index2 && index2)
    {
      PointF const newDir = itr.m_dir;
      PointF const newPos = m_path.m_position[index2];
      float const smoothFactor = (newPos - itr.m_pos).Length() / advance;
      dir = oldDir * (1.0f - smoothFactor) + newDir * smoothFactor;
    }
    else
    {
      dir = oldDir;
    }

    float const angle = atan2(dir.y, dir.x);
    float const cosa = cosf(angle);
    float const sina = sinf(angle);

    yOffset += halfHeight;
    xOffset += halfWidth;
    float const x1old = halfWidth + xOffset;
    float const y1old = -halfHeight + yOffset;

    float const x2old = -halfWidth + xOffset;
    float const y2old = -halfHeight + yOffset;

    float const x3old = -halfWidth + xOffset;
    float const y3old = halfHeight + yOffset;

    float const x4old = halfWidth + xOffset;
    float const y4old = halfHeight + yOffset;

    float x1 = (x1old * cosa - y1old * sina) * m_scaleFactor + pos.x;
    float y1 = (x1old * sina + y1old * cosa) * m_scaleFactor + pos.y;

    float x2 = (x2old * cosa - y2old * sina) * m_scaleFactor + pos.x;
    float y2 = (x2old * sina + y2old * cosa) * m_scaleFactor + pos.y;

    float x3 = (x3old * cosa - y3old * sina) * m_scaleFactor + pos.x;
    float y3 = (x3old * sina + y3old * cosa) * m_scaleFactor + pos.y;

    float x4 = (x4old * cosa - y4old * sina) * m_scaleFactor + pos.x;
    float y4 = (x4old * sina + y4old * cosa) * m_scaleFactor + pos.y;

    int index = i * 6;
    positions[index++] = Position(x2, y2);
    positions[index++] = Position(x3, y3);
    positions[index++] = Position(x1, y1);

    positions[index++] = Position(x4, y4);
    positions[index++] = Position(x1, y1);
    positions[index++] = Position(x3, y3);
  }

  TOffsetNode const & node = GetOffsetNode(DirectionAttributeID);
  MutateNode mutateNode;
  mutateNode.m_region = node.second;
  mutateNode.m_data = MakeStackRefPointer<void>(&positions[0]);
  mutator->AddMutation(node.first, mutateNode);
}

void Spline::FromArray(vector<PointF> const & path)
{
  m_position = vector<PointF>(path.begin(), path.end() - 1);
  int cnt = m_position.size();
  m_direction = vector<PointF>(cnt);
  m_length = vector<float>(cnt);

  for(int i = 0; i < cnt; ++i)
  {
    m_direction[i] = path[i+1] - path[i];
    m_length[i] = m_direction[i].Length();
    m_direction[i] = m_direction[i].Normalize();
    m_lengthAll += m_length[i];
  }
}

Spline const & Spline::operator = (Spline const & spl)
{
  if(&spl != this)
  {
    m_lengthAll = spl.m_lengthAll;
    m_position = spl.m_position;
    m_direction = spl.m_direction;
    m_length = spl.m_length;
  }
  return *this;
}

void Spline::iterator::Attach(Spline const & S)
{
  m_spl = &S;
  m_index = 0;
  m_dist = 0;
  m_dir = m_spl->m_direction[m_index];
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
}

void Spline::iterator::Step(float speed)
{
  m_dist += speed;
  while(m_dist > m_spl->m_length[m_index])
  {
    m_dist -= m_spl->m_length[m_index];
    m_index++;
    m_index %= m_spl->m_position.size();
  }
  m_dir = m_spl->m_direction[m_index];
  m_pos = m_spl->m_position[m_index] + m_dir * m_dist;
}

}
