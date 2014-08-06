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
using glsl_types::vec2;
using glsl_types::vec4;

namespace
{
  static float const realFontSize = 20.0f;

  struct Buffer
  {
    vector<vec2>        m_pos;
    vector<vec4>        m_uvs;
    vector<vec4>        m_baseColor;
    vector<vec4>        m_outlineColor;
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
  : m_spline(path)
  , m_params(params) {}

void PathTextShape::Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const
{
  strings::UniString const text = strings::MakeUniString(m_params.m_text);
  float const fontSize = m_params.m_textFont.m_size;

  // Fill buffers
  int const cnt = text.size();
  vector<Buffer> buffers(1);
  float const needOutline = m_params.m_textFont.m_needOutline;
  float length = 0.0f;

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
    length += advance;

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

    curBuffer.m_uvs.push_back(vec4(rect.minX(), rect.maxY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(vec4(rect.minX(), rect.minY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(vec4(rect.maxX(), rect.maxY(), textureNum, m_params.m_depth));
    curBuffer.m_uvs.push_back(vec4(rect.maxX(), rect.minY(), textureNum, m_params.m_depth));

    int const newSize = curBuffer.m_baseColor.size() + 4;
    curBuffer.m_baseColor.resize(newSize, base);
    curBuffer.m_outlineColor.resize(newSize, outline);
    curBuffer.m_pos.resize(newSize, vec2(0.0f, 0.0f));
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

    dp::OverlayHandle * handle = new PathTextHandle(m_spline, m_params, buffers[i].m_info, buffers[i].m_maxSize, length);

    batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&provider), dp::MovePointer(handle), 4);
  }
}

m2::RectD PathTextHandle::GetPixelRect(ScreenBase const & screen) const
{
  int const cnt = m_infos.size();

  vec2 const & v1 = m_positions[1];
  vec2 const & v2 = m_positions[2];
  PointF centr((v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f);
  centr = screen.GtoP(centr);
  float minx, maxx, miny, maxy;
  minx = maxx = centr.x;
  miny = maxy = centr.y;

  for (int i = 1; i < cnt; i++)
  {
    vec2 const & v1 = m_positions[i * 4 + 1];
    vec2 const & v2 = m_positions[i * 4 + 2];
    PointF centr((v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f);
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

PathTextHandle::PathTextHandle(m2::SharedSpline const & spl, PathTextViewParams const & params,
                               vector<LetterInfo> const & info, float maxSize, float textLength)
  : OverlayHandle(FeatureID()
  , dp::Center, 0.0f)
  , m_path(spl)
  , m_params(params)
  , m_infos(info)
  , m_scaleFactor(1.0f)
  , m_positions(info.size() * 4)
  , m_maxSize(maxSize)
  , m_textLength(textLength)
{
}

void PathTextHandle::Update(ScreenBase const & screen)
{
  switch(ChooseDirection(screen))
  {
  case -1:
    DrawReverse(screen);
    break;
  case 0:
    ClearPositions();
    break;
  case 1:
    DrawForward(screen);
    break;
  }
}

int PathTextHandle::ChooseDirection(ScreenBase const & screen)
{
  m_scaleFactor = screen.GetScale();
  Spline::iterator itr = m_path.CreateIterator();
  itr.Step(m_params.m_offsetStart * m_scaleFactor);
  PointF const p1 = screen.GtoP(itr.m_pos);
  itr.Step(m_textLength * m_scaleFactor);
  PointF const p2 = screen.GtoP(itr.m_pos);
  if (itr.BeginAgain())
    return 0;

  if ((p2 - p1).x >= 0 )
    return 1;
  else
    return -1;
}

void PathTextHandle::ClearPositions()
{
  std::fill(m_positions.begin(), m_positions.end(), vec2(0.0f, 0.0f));
}

void PathTextHandle::DrawReverse(ScreenBase const & screen)
{
  m_scaleFactor = screen.GetScale();
  int const cnt = m_infos.size();
  Spline::iterator itr = m_path.CreateIterator();
  itr.Step(m_params.m_offsetStart * m_scaleFactor);

  for (int i = cnt - 1; i >= 0; i--)
  {
    float const advance = m_infos[i].m_advance * m_scaleFactor;
    float const halfWidth = m_infos[i].m_halfWidth;
    float const halfHeight = m_infos[i].m_halfHeight;
    float const xOffset = m_infos[i].m_xOffset + halfWidth;
    float const yOffset = m_infos[i].m_yOffset + halfHeight;

    ASSERT_NOT_EQUAL(advance, 0.0, ());
    PointF const pos = itr.m_pos;
    itr.Step(advance);
    ASSERT(!itr.BeginAgain(), ());

    PointF dir = itr.m_avrDir.Normalize();
    PointF norm(-dir.y, dir.x);
    PointF norm2 = norm;
    dir *= halfWidth * m_scaleFactor;
    norm *= halfHeight * m_scaleFactor;

    float const fontSize = m_params.m_textFont.m_size * m_scaleFactor / 2.0f;
    PointF const pivot = dir * xOffset / halfWidth + norm * yOffset / halfHeight + pos + norm2 * fontSize;

    int index = i * 4;
    m_positions[index++] = pivot + dir + norm;
    m_positions[index++] = pivot + dir - norm;
    m_positions[index++] = pivot - dir + norm;
    m_positions[index++] = pivot - dir - norm;
  }
}

void PathTextHandle::DrawForward(ScreenBase const & screen)
{
  m_scaleFactor = screen.GetScale();
  int const cnt = m_infos.size();
  Spline::iterator itr = m_path.CreateIterator();
  itr.Step(m_params.m_offsetStart * m_scaleFactor);

  for (int i = 0; i < cnt; i++)
  {
    float const advance = m_infos[i].m_advance * m_scaleFactor;
    float const halfWidth = m_infos[i].m_halfWidth;
    float const halfHeight = m_infos[i].m_halfHeight;
    /// TODO Can be optimized later (filling stage)
    float const xOffset = m_infos[i].m_xOffset + halfWidth;
    float const yOffset = -m_infos[i].m_yOffset - halfHeight;

    ASSERT_NOT_EQUAL(advance, 0.0, ());
    PointF const pos = itr.m_pos;
    itr.Step(advance);
    ASSERT(!itr.BeginAgain(), ());

    PointF dir = itr.m_avrDir.Normalize();
    PointF norm(-dir.y, dir.x);
    PointF norm2 = norm;
    dir *= halfWidth * m_scaleFactor;
    norm *= halfHeight * m_scaleFactor;

    float const fontSize = m_params.m_textFont.m_size * m_scaleFactor / 2.0f;
    PointF const pivot = dir * xOffset / halfWidth + norm * yOffset / halfHeight + pos - norm2 * fontSize;

    int index = i * 4;
    m_positions[index++] = pivot - dir - norm;
    m_positions[index++] = pivot - dir + norm;
    m_positions[index++] = pivot + dir - norm;
    m_positions[index++] = pivot + dir + norm;
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
