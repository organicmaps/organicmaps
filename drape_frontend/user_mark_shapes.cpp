#include "user_mark_shapes.hpp"

#include "line_shape.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/shader_def.hpp"
#include "drape/attribute_provider.hpp"

#include "geometry/spline.hpp"

namespace df
{

namespace
{
  int const ZUserMarksLayer = -1;
  int const YSearchMarksLayer = 1;
  int const YApiMarksLayer = 2;
  int const YBookmarksLayer = 3;
}

TileKey GetSearchTileKey()
{
  return TileKey(0, YSearchMarksLayer, ZUserMarksLayer);
}

TileKey GetApiTileKey()
{
  return TileKey(0, YApiMarksLayer, ZUserMarksLayer);
}

TileKey GetBookmarkTileKey(size_t categoryIndex)
{
  return TileKey(categoryIndex, YBookmarksLayer, ZUserMarksLayer);
}

bool IsUserMarkLayer(TileKey const & tileKey)
{
  return tileKey.m_zoomLevel == ZUserMarksLayer;
}

namespace
{

template <typename TCreateVector>
void AlignFormingNormals(TCreateVector const & fn, dp::Anchor anchor,
                         dp::Anchor first, dp::Anchor second,
                         glsl::vec2 & firstNormal, glsl::vec2 & secondNormal)
{
  firstNormal = fn();
  secondNormal = -firstNormal;
  if ((anchor & second) != 0)
  {
    firstNormal *= 2;
    secondNormal = glsl::vec2(0.0, 0.0);
  }
  else if ((anchor & first) != 0)
  {
    firstNormal = glsl::vec2(0.0, 0.0);
    secondNormal *= 2;
  }
}

void AlignHorizontal(float halfWidth, dp::Anchor anchor,
                     glsl::vec2 & left, glsl::vec2 & right)
{
  AlignFormingNormals([&halfWidth]{ return glsl::vec2(-halfWidth, 0.0f); }, anchor, dp::Left, dp::Right, left, right);
}

void AlignVertical(float halfHeight, dp::Anchor anchor,
                   glsl::vec2 & up, glsl::vec2 & down)
{
  AlignFormingNormals([&halfHeight]{ return glsl::vec2(0.0f, -halfHeight); }, anchor, dp::Top, dp::Bottom, up, down);
}

void CacheUserPoints(UserMarksProvider const * provider,
                     dp::RefPointer<dp::Batcher> batcher,
                     dp::RefPointer<dp::TextureManager> textures)
{
  size_t markCount = provider->GetUserPointCount();
  if (markCount == 0)
    return;

  uint32_t vertexCount = dp::Batcher::VertexPerQuad * markCount; // 4 vertex per quad

  buffer_vector<gpu::SolidTexturingVertex, 1024> buffer;
  buffer.reserve(vertexCount);

  vector<UserPointMark const *> marks;
  marks.reserve(markCount);
  for (size_t i = 0; i < markCount; ++i)
    marks.push_back(provider->GetUserPointMark(i));

  sort(marks.begin(), marks.end(), [](UserPointMark const * v1, UserPointMark const * v2)
  {
    return v1->GetPivot().y < v2->GetPivot().y;
  });

  dp::TextureManager::SymbolRegion region;
  for (size_t i = 0; i < marks.size(); ++i)
  {
    UserPointMark const * pointMark = marks[i];
    textures->GetSymbolRegion(pointMark->GetSymbolName(), region);
    m2::RectF const & texRect = region.GetTexRect();
    m2::PointF pxSize = region.GetPixelSize();
    dp::Anchor anchor = pointMark->GetAnchor();
    glsl::vec3 pos = glsl::vec3(glsl::ToVec2(pointMark->GetPivot()), pointMark->GetDepth() + 10 * (markCount - i));

    glsl::vec2 left, right, up, down;
    AlignHorizontal(pxSize.x * 0.5f, anchor, left, right);
    AlignVertical(pxSize.y * 0.5f, anchor, up, down);

    buffer.push_back(gpu::SolidTexturingVertex(pos, left + down, glsl::ToVec2(texRect.LeftTop())));
    buffer.push_back(gpu::SolidTexturingVertex(pos, left + up, glsl::ToVec2(texRect.LeftBottom())));
    buffer.push_back(gpu::SolidTexturingVertex(pos, right + down, glsl::ToVec2(texRect.RightTop())));
    buffer.push_back(gpu::SolidTexturingVertex(pos, right + up, glsl::ToVec2(texRect.RightBottom())));
  }

  dp::GLState state(gpu::TEXTURING_PROGRAM, dp::GLState::UserMarkLayer);
  state.SetColorTexture(region.GetTexture());

  dp::AttributeProvider attribProvider(1, buffer.size());
  attribProvider.InitStream(0, gpu::SolidTexturingVertex::GetBindingInfo(), dp::MakeStackRefPointer<void>(buffer.data()));

  batcher->InsertListOfStrip(state, dp::MakeStackRefPointer(&attribProvider), dp::Batcher::VertexPerQuad);
}

void CacheUserLines(UserMarksProvider const * provider,
                    dp::RefPointer<dp::Batcher> batcher,
                    dp::RefPointer<dp::TextureManager> textures)
{
  for (size_t i = 0; i < provider->GetUserLineCount(); ++i)
  {
    UserLineMark const * line = provider->GetUserLineMark(i);
    size_t pointCount = line->GetPointCount();

    vector<m2::PointD> points;
    points.reserve(pointCount);
    for (size_t i = 0; i < pointCount; ++i)
      points.push_back(line->GetPoint(i));

    m2::SharedSpline spline(points);

    for (size_t layerIndex = 0; layerIndex < line->GetLayerCount(); ++layerIndex)
    {
      LineViewParams params;
      params.m_baseGtoPScale = 1.0f;
      params.m_cap = dp::RoundCap;
      params.m_join = dp::RoundJoin;
      params.m_color = line->GetColor(layerIndex);
      params.m_depth = line->GetLayerDepth(layerIndex);
      params.m_width = line->GetWidth(layerIndex);

      LineShape(spline, params).Draw(batcher, textures);
    }
  }
}

} // namespace

void CacheUserMarks(UserMarksProvider const * provider,
                    dp::RefPointer<dp::Batcher> batcher,
                    dp::RefPointer<dp::TextureManager> textures)
{
  CacheUserPoints(provider, batcher, textures);
  CacheUserLines(provider, batcher, textures);
}

} // namespace df
