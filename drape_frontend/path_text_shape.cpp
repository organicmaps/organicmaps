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
  float angelFromDir(float x, float y)
  {
    float const gip = sqrtf(x*x + y*y);
    float const cosa = x / gip;
    if(y > 0)
      return acosf(cosa) * 180.0f / M_PI;
    else
      return 360.0f - acosf(cosa) * 180.0f / M_PI;
  }

  struct ColorF
  {
    ColorF() {}
    ColorF(float r, float g, float b, float a):m_r(r), m_g(g), m_b(b), m_a(a){}

    float m_r;
    float m_g;
    float m_b;
    float m_a;
  };

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
    Position(float x, float y):m_x(x), m_y(y){}

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

PathTextShape::PathTextShape(vector<PointF> const & path, PathTextViewParams const & params):
  m_params(params)
{
  m_path.FromArray(path);
}

void PathTextShape::Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const
{
  strings::UniString const text = strings::MakeUniString(m_params.m_Text);
  float const fontSize = m_params.m_TextFont.m_size;

  // Fill buffers
  int const cnt = text.size();
  vector<Buffer> buffers;
  float const needOutline = m_params.m_TextFont.m_needOutline;

  Spline::iterator itr;
  itr.Attach(m_path);
  itr.Step(0.0f);

  int textureSet;
  for (int i = 0 ; i < cnt ; i++)
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
    while (buffers.size() <=  textureSet)
    {
      buffers.push_back(Buffer());
    }
    Buffer & curBuffer = buffers[textureSet];

    curBuffer.m_info.push_back(
                LetterInfo(xOffset, yOffset, advance + curBuffer.m_offset, halfWidth, halfHeight));

    for(int i = 0; i < buffers.size(); ++i)
      buffers[i].m_offset += advance;

    curBuffer.m_offset = 0;

    advance *= aspect;
    yOffset *= aspect;
    xOffset *= aspect;
    halfWidth *= aspect;
    halfHeight *= aspect;

    itr.Step(advance);

    m2::RectF const rect = region.GetTexRect();
    float const textureNum = (region.GetTextureNode().m_textureOffset << 1) + needOutline;

    Color clr = m_params.m_TextFont.m_color;
    ColorF const base(clr.m_red / 255.0f, clr.m_green / 255.0f, clr.m_blue / 255.0f, clr.m_alfa / 255.0f);
    clr = m_params.m_TextFont.m_outlineColor;
    ColorF const outline(clr.m_red / 255.0f, clr.m_green / 255.0f, clr.m_blue / 255.0f, clr.m_alfa / 255.0f);

    curBuffer.m_uvs.push_back(TexCoord(rect.minX(), rect.minY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth));

    curBuffer.m_uvs.push_back(TexCoord(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.maxX(), rect.minY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(TexCoord(rect.minX(), rect.maxY(), textureNum, m_params.m_depth));

    curBuffer.m_baseColor.push_back(base);
    curBuffer.m_baseColor.push_back(base);
    curBuffer.m_baseColor.push_back(base);

    curBuffer.m_baseColor.push_back(base);
    curBuffer.m_baseColor.push_back(base);
    curBuffer.m_baseColor.push_back(base);

    curBuffer.m_outlineColor.push_back(outline);
    curBuffer.m_outlineColor.push_back(outline);
    curBuffer.m_outlineColor.push_back(outline);

    curBuffer.m_outlineColor.push_back(outline);
    curBuffer.m_outlineColor.push_back(outline);
    curBuffer.m_outlineColor.push_back(outline);

    curBuffer.m_pos.push_back(Position(0, 0));
    curBuffer.m_pos.push_back(Position(0, 0));
    curBuffer.m_pos.push_back(Position(0, 0));

    curBuffer.m_pos.push_back(Position(0, 0));
    curBuffer.m_pos.push_back(Position(0, 0));
    curBuffer.m_pos.push_back(Position(0, 0));
  }

  for(int i = 0; i < buffers.size(); ++i)
  {
    if(buffers[i].m_pos.empty())
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
  float entireLength = m_params.m_OffsetStart * scaleFactor;

  float const fontSize = m_params.m_TextFont.m_size;
  int const cnt = m_infos.size();
  vector<Position> positions(m_infos.size() * 6);

  Spline::iterator itr;
  itr.Attach(m_path);
  itr.Step(m_params.m_OffsetStart * scaleFactor);

  for (int i = 0 ; i < cnt ; i++)
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

    PointF const pos = itr.pos;
    PointF const oldDir = itr.dir;

    advance *= scaleFactor;
    entireLength += advance;
    if(entireLength >= m_params.m_OffsetEnd * scaleFactor)
      return;
    int const index1 = itr.index;
    itr.Step(advance);
    int const index2 = itr.index;
    PointF dir;
    if (index1 != index2 && index2)
    {
      PointF const newDir = itr.dir;
      PointF const newPos = m_path.position[index2];
      float const smoothFactor = (newPos - itr.pos).Length() / advance;
      dir = oldDir * (1.0f - smoothFactor) + newDir * smoothFactor;
    }
    else
    {
      dir = oldDir;
    }

    float const angle = angelFromDir(dir.x, dir.y) * M_PI / 180.0f;
    float const cosa = cosf(angle);
    float const sina = sinf(angle);

    yOffset += halfHeight;
    xOffset -= halfWidth;
    float const x1old = halfWidth - xOffset;
    float const y1old = -halfHeight + yOffset;

    float const x2old = -halfWidth - xOffset;
    float const y2old = -halfHeight + yOffset;

    float const x3old = -halfWidth - xOffset;
    float const y3old = halfHeight + yOffset;

    float const x4old = halfWidth - xOffset;
    float const y4old = halfHeight + yOffset;

    float x1 = (x1old * cosa - y1old * sina) * scaleFactor + pos.x;
    float y1 = (x1old * sina + y1old * cosa) * scaleFactor + pos.y;

    float x2 = (x2old * cosa - y2old * sina) * scaleFactor + pos.x;
    float y2 = (x2old * sina + y2old * cosa) * scaleFactor + pos.y;

    float x3 = (x3old * cosa - y3old * sina) * scaleFactor + pos.x;
    float y3 = (x3old * sina + y3old * cosa) * scaleFactor + pos.y;

    float x4 = (x4old * cosa - y4old * sina) * scaleFactor + pos.x;
    float y4 = (x4old * sina + y4old * cosa) * scaleFactor + pos.y;

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

}
