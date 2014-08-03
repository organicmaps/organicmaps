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
#include "../base/matrix.hpp"

#include "../geometry/transformations.hpp"

#include "../std/algorithm.hpp"
#include "../std/vector.hpp"

namespace df
{

using m2::PointF;
using m2::Spline;

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

  struct Buffer
  {
    vector<Position>    m_pos;
    vector<TexCoord>    m_uvs;
    vector<dp::ColorF>      m_baseColor;
    vector<dp::ColorF>      m_outlineColor;
    vector<LetterInfo>  m_info;
    float               m_offset;
    float               m_maxSize;

    void addSizes(float x, float y)
    {
      if (x > m_maxSize)
        m_maxSize = x;
      if (y > m_maxSize)
        m_maxSize = y;
    }
  };
}

PathTextShape::PathTextShape(vector<PointF> const & path, PathTextViewParams const & params)
  : m_params(params)
{
  m_path.FromArray(path);
}

void PathTextShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  strings::UniString const text = strings::MakeUniString(m_params.m_text);
  float const fontSize = m_params.m_textFont.m_size;

  // Fill buffers
  int const cnt = text.size();
  vector<Buffer> buffers(1);
  float const needOutline = m_params.m_textFont.m_needOutline;

  int textureSet;
  for (int i = 0; i < cnt; i++)
  {
    dp::TextureSetHolder::GlyphRegion region;
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

    yOffset *= aspect;
    xOffset *= aspect;
    halfWidth *= aspect;
    halfHeight *= aspect;
    advance *= aspect;

    curBuffer.addSizes(halfWidth, halfHeight);

    curBuffer.m_info.push_back(
                LetterInfo(xOffset, yOffset, advance + curBuffer.m_offset, halfWidth, halfHeight));

    for (int i = 0; i < buffers.size(); ++i)
      buffers[i].m_offset += advance;

    curBuffer.m_offset = 0;


    m2::RectF const & rect = region.GetTexRect();
    float const textureNum = (region.GetTextureNode().m_textureOffset << 1) + needOutline;

    dp::ColorF const base(m_params.m_textFont.m_color);
    dp::ColorF const outline(m_params.m_textFont.m_outlineColor);

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

    dp::GLState state(gpu::PATH_FONT_PROGRAM, dp::GLState::OverlayLayer);
    state.SetTextureSet(i);
    state.SetBlending(dp::Blending(true));

    dp::AttributeProvider provider(4, buffers[i].m_pos.size());
    {
      dp::BindingInfo position(1, PathTextHandle::DirectionAttributeID);
      dp::BindingDecl & decl = position.GetBindingDecl(0);
      decl.m_attributeName = "a_position";
      decl.m_componentCount = 2;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(0, position, dp::MakeStackRefPointer(&buffers[i].m_pos[0]));
    }
    {
      dp::BindingInfo texcoord(1);
      dp::BindingDecl & decl = texcoord.GetBindingDecl(0);
      decl.m_attributeName = "a_texcoord";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(1, texcoord, dp::MakeStackRefPointer(&buffers[i].m_uvs[0]));
    }
    {
      dp::BindingInfo base_color(1);
      dp::BindingDecl & decl = base_color.GetBindingDecl(0);
      decl.m_attributeName = "a_color";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(2, base_color, dp::MakeStackRefPointer(&buffers[i].m_baseColor[0]));
    }
    {
      dp::BindingInfo outline_color(1);
      dp::BindingDecl & decl = outline_color.GetBindingDecl(0);
      decl.m_attributeName = "a_outline_color";
      decl.m_componentCount = 4;
      decl.m_componentType = gl_const::GLFloatType;
      decl.m_offset = 0;
      decl.m_stride = 0;
      provider.InitStream(3, outline_color, dp::MakeStackRefPointer(&buffers[i].m_outlineColor[0]));
    }

    dp::OverlayHandle * handle = new PathTextHandle(m_path, m_params, buffers[i].m_info, buffers[i].m_maxSize);

    batcher->InsertTriangleList(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(handle));
  }
}

m2::RectD PathTextHandle::GetPixelRect(ScreenBase const & screen) const
{
  int const cnt = m_infos.size();

  Position const & v1 = m_positions[1];
  Position const & v2 = m_positions[2];
  PointF centr((v1.m_x + v2.m_x) / 2.0f, (v1.m_y + v2.m_y) / 2.0f);
  centr = screen.GtoP(centr);
  float minx, maxx, miny, maxy;
  minx = maxx = centr.x;
  miny = maxy = centr.y;

  for (int i = 1; i < cnt; i++)
  {
    Position const & v1 = m_positions[i * 6 + 1];
    Position const & v2 = m_positions[i * 6 + 2];
    PointF centr((v1.m_x + v2.m_x) / 2.0f, (v1.m_y + v2.m_y) / 2.0f);
    centr = screen.GtoP(centr);
    if(centr.x > maxx)
      maxx = centr.x;
    if(centr.x < minx)
      minx = centr.x;
    if(centr.y > maxy)
      maxy = centr.y;
    if(centr.y < miny)
      miny = centr.y;
  }

  return m2::RectD(minx - m_maxSize, miny - m_maxSize, maxx + m_maxSize, maxy + m_maxSize);
}

void PathTextHandle::Update(ScreenBase const & screen)
{
  m_scaleFactor = screen.GetScale();

  float entireLength = m_params.m_offsetStart * m_scaleFactor;
  int const cnt = m_infos.size();

  Spline::iterator itr;
  itr.Attach(m_path);
  itr.Step(entireLength);

  for (int i = 0; i < cnt; i++)
  {
    float const advance = m_infos[i].m_advance * m_scaleFactor;
    float const halfWidth = m_infos[i].m_halfWidth;
    float const halfHeight = m_infos[i].m_halfHeight;
    /// TODO Can be optimized later (filling stage)
    float const xOffset = m_infos[i].m_xOffset + halfWidth;
    float const yOffset = m_infos[i].m_yOffset + halfHeight;

    ASSERT_NOT_EQUAL(advance, 0.0, ());
    entireLength += advance;
    if(entireLength >= m_params.m_offsetEnd * m_scaleFactor || itr.BeginAgain())
      return;

    PointF const pos = itr.m_pos;
    itr.Step(advance);
    PointF dir = itr.m_avrDir.Normalize();
    PointF norm(-dir.y, dir.x);
    PointF norm2 = norm;
    dir *= halfWidth * m_scaleFactor;
    norm *= halfHeight * m_scaleFactor;

    float const fontSize = m_params.m_textFont.m_size * m_scaleFactor / 2.0f;
    PointF const pivot = dir * xOffset / halfWidth + norm * yOffset / halfHeight + pos + norm2 * fontSize;

    PointF const p1 = pivot + dir - norm;
    PointF const p2 = pivot - dir - norm;
    PointF const p3 = pivot - dir + norm;
    PointF const p4 = pivot + dir + norm;

    int index = i * 6;
    m_positions[index++] = Position(p2);
    m_positions[index++] = Position(p3);
    m_positions[index++] = Position(p1);

    m_positions[index++] = Position(p4);
    m_positions[index++] = Position(p1);
    m_positions[index++] = Position(p3);
  }
}

void PathTextHandle::GetAttributeMutation(dp::RefPointer<dp::AttributeBufferMutator> mutator) const
{
  TOffsetNode const & node = GetOffsetNode(DirectionAttributeID);
  dp::MutateNode mutateNode;
  mutateNode.m_region = node.second;
  mutateNode.m_data = dp::MakeStackRefPointer<void>(&m_positions[0]);
  mutator->AddMutation(node.first, mutateNode);
}

}
