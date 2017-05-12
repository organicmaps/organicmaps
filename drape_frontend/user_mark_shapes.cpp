#include "drape_frontend/user_mark_shapes.hpp"

#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/utils/vertex_decl.hpp"
#include "drape/attribute_provider.hpp"

#include "geometry/spline.hpp"

namespace df
{

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

struct UserPointVertex : gpu::BaseVertex
{
  UserPointVertex() = default;
  UserPointVertex(TPosition const & pos, TNormal const & normal, TTexCoord const & texCoord, bool isAnim)
    : m_position(pos)
    , m_normal(normal)
    , m_texCoord(texCoord)
    , m_isAnim(isAnim ? 1.0 : -1.0)
  {}

  static dp::BindingInfo GetBinding()
  {
    dp::BindingInfo info(4);
    uint8_t offset = 0;
    offset += dp::FillDecl<TPosition, UserPointVertex>(0, "a_position", info, offset);
    offset += dp::FillDecl<TNormal, UserPointVertex>(1, "a_normal", info, offset);
    offset += dp::FillDecl<TTexCoord, UserPointVertex>(2, "a_colorTexCoords", info, offset);
    offset += dp::FillDecl<bool, UserPointVertex>(3, "a_animate", info, offset);

    return info;
  }

  TPosition m_position;
  TNormal m_normal;
  TTexCoord m_texCoord;
  float m_isAnim;
};

using UPV = UserPointVertex;

void CacheUserPoints(UserMarksProvider const * provider, ref_ptr<dp::TextureManager> textures,
                     TUserMarkShapes & outShapes)
{
  size_t markCount = provider->GetUserPointCount();
  if (markCount == 0)
    return;

  int const kZoomLevel = 10;
  map<TileKey, vector<UserPointMark const *>> marks;
  for (size_t i = 0; i < markCount; ++i)
  {
    UserPointMark const * userMark = provider->GetUserPointMark(i);
    TileKey const tileKey = GetTileKeyByPoint(userMark->GetPivot(), kZoomLevel);
    marks[tileKey].push_back(userMark);
  }

  for (auto it = marks.begin(); it != marks.end(); ++it)
  {
    TileKey const & key = it->first;
    m2::PointD const tileCenter = key.GetGlobalRect().Center();

    sort(it->second.begin(), it->second.end(), [](UserPointMark const * v1, UserPointMark const * v2)
    {
      return v1->GetPivot().y < v2->GetPivot().y;
    });

    dp::TextureManager::SymbolRegion region;

    uint32_t const vertexCount = static_cast<uint32_t>(it->second.size()) * dp::Batcher::VertexPerQuad;
    uint32_t const indicesCount = static_cast<uint32_t>(it->second.size()) * dp::Batcher::IndexPerQuad;
    buffer_vector<UPV, 128> buffer;
    buffer.reserve(vertexCount);

    for (size_t i = 0; i < it->second.size(); ++i)
    {
      UserPointMark const * pointMark = it->second[i];
      textures->GetSymbolRegion(pointMark->GetSymbolName(), region);
      m2::RectF const & texRect = region.GetTexRect();
      m2::PointF const pxSize = region.GetPixelSize();
      dp::Anchor const anchor = pointMark->GetAnchor();
      m2::PointD const pt = MapShape::ConvertToLocal(pointMark->GetPivot(), tileCenter, kShapeCoordScalar);
      glsl::vec3 const pos = glsl::vec3(glsl::ToVec2(pt), pointMark->GetDepth());
      bool const runAnim = pointMark->RunCreationAnim();

      glsl::vec2 left, right, up, down;
      AlignHorizontal(pxSize.x * 0.5f, anchor, left, right);
      AlignVertical(pxSize.y * 0.5f, anchor, up, down);

      m2::PointD const pixelOffset = pointMark->GetPixelOffset();
      glsl::vec2 const offset(pixelOffset.x, pixelOffset.y);

      buffer.emplace_back(pos, left + down + offset, glsl::ToVec2(texRect.LeftTop()), runAnim);
      buffer.emplace_back(pos, left + up + offset, glsl::ToVec2(texRect.LeftBottom()), runAnim);
      buffer.emplace_back(pos, right + down + offset, glsl::ToVec2(texRect.RightTop()), runAnim);
      buffer.emplace_back(pos, right + up + offset, glsl::ToVec2(texRect.RightBottom()), runAnim);
    }

    dp::GLState state(gpu::BOOKMARK_PROGRAM, dp::GLState::UserMarkLayer);
    state.SetProgram3dIndex(gpu::BOOKMARK_BILLBOARD_PROGRAM);
    state.SetColorTexture(region.GetTexture());
    state.SetTextureFilter(gl_const::GLNearest);

    uint32_t const kMaxSize = 65000;
    dp::Batcher batcher(min(indicesCount, kMaxSize), min(vertexCount, kMaxSize));
    dp::SessionGuard guard(batcher, [&key, &outShapes](dp::GLState const & state,
                                                       drape_ptr<dp::RenderBucket> && b)
    {
      outShapes.emplace_back(UserMarkShape(state, move(b), key));
    });
    dp::AttributeProvider attribProvider(1, static_cast<uint32_t>(buffer.size()));
    attribProvider.InitStream(0, UPV::GetBinding(), make_ref(buffer.data()));
    batcher.InsertListOfStrip(state, make_ref(&attribProvider), dp::Batcher::VertexPerQuad);
  }
}

void CacheUserLines(UserMarksProvider const * provider, ref_ptr<dp::TextureManager> textures,
                    TUserMarkShapes & outShapes)
{
  int const kZoomLevel = 10;
  map<TileKey, vector<pair<UserLineMark const *, m2::SharedSpline>>> userLines;
  for (size_t i = 0; i < provider->GetUserLineCount(); ++i)
  {
    UserLineMark const * line = provider->GetUserLineMark(i);
    size_t const pointCount = line->GetPointCount();

    vector<m2::PointD> points;
    m2::RectD rect;
    points.reserve(pointCount);
    for (size_t i = 0; i < pointCount; ++i)
    {
      points.push_back(line->GetPoint(i));
      rect.Add(points.back());
    }

    TileKey const tileKey = GetTileKeyByPoint(rect.Center(), kZoomLevel);
    userLines[tileKey].push_back(make_pair(line, m2::SharedSpline(points)));
  }

  int const kBatchSize = 5000;
  for (auto it = userLines.begin(); it != userLines.end(); ++it)
  {
    TileKey const & key = it->first;
    dp::Batcher batcher(kBatchSize, kBatchSize);
    dp::SessionGuard guard(batcher, [&key, &outShapes](dp::GLState const & state,
                                                       drape_ptr<dp::RenderBucket> && b)
    {
      outShapes.emplace_back(UserMarkShape(state, move(b), key));
    });
    for (auto const & lineData : it->second)
    {
      UserLineMark const * line = lineData.first;
      for (size_t layerIndex = 0; layerIndex < line->GetLayerCount(); ++layerIndex)
      {
        LineViewParams params;
        params.m_tileCenter = key.GetGlobalRect().Center();
        params.m_baseGtoPScale = 1.0f;
        params.m_cap = dp::RoundCap;
        params.m_join = dp::RoundJoin;
        params.m_color = line->GetColor(layerIndex);
        params.m_depth = line->GetLayerDepth(layerIndex);
        params.m_width = line->GetWidth(layerIndex);
        params.m_minVisibleScale = 1;
        params.m_rank = 0;

        LineShape(lineData.second, params).Draw(make_ref(&batcher), textures);
      }
    }
  }
}

} // namespace

TUserMarkShapes CacheUserMarks(UserMarksProvider const * provider, ref_ptr<dp::TextureManager> textures)
{
  TUserMarkShapes shapes;
  CacheUserPoints(provider, textures, shapes);
  CacheUserLines(provider, textures, shapes);
  return shapes;
}

} // namespace df
