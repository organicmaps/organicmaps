#include "drape_frontend/selection_shape_generator.hpp"
#include "drape_frontend/batcher_bucket.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/line_shape_helper.hpp"
#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "shaders/programs.hpp"

#include "drape/batcher.hpp"
#include "drape/texture_manager.hpp"

#include "indexer/feature.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/buffer_vector.hpp"
#include "base/math.hpp"

#include <vector>

namespace df
{
namespace
{
std::string const kTrackSelectedSymbolName = "track_marker_selected";
df::ColorConstant const kSelectionColor = "Selection";
float const kLeftSide = 1.0f;
float const kCenter = 0.0f;
float const kRightSide = -1.0f;

struct MarkerVertex
{
  MarkerVertex() = default;
  MarkerVertex(glsl::vec2 const & normal, glsl::vec2 const & texCoord) : m_normal(normal), m_texCoord(texCoord) {}

  glsl::vec2 m_normal;
  glsl::vec2 m_texCoord;
};

dp::BindingInfo GetMarkerBindingInfo()
{
  dp::BindingInfo info(2);
  dp::BindingDecl & normal = info.GetBindingDecl(0);
  normal.m_attributeName = "a_normal";
  normal.m_componentCount = 2;
  normal.m_componentType = gl_const::GLFloatType;
  normal.m_offset = 0;
  normal.m_stride = sizeof(MarkerVertex);

  dp::BindingDecl & texCoord = info.GetBindingDecl(1);
  texCoord.m_attributeName = "a_colorTexCoords";
  texCoord.m_componentCount = 2;
  texCoord.m_componentType = gl_const::GLFloatType;
  texCoord.m_offset = sizeof(glsl::vec2);
  texCoord.m_stride = sizeof(MarkerVertex);

  return info;
}

struct SelectionLineVertex
{
  using TNormal = glsl::vec3;

  SelectionLineVertex() = default;
  SelectionLineVertex(glsl::vec3 const & position, glsl::vec2 const & normal, glsl::vec2 const & colorTexCoords,
                      glsl::vec3 const & length)
    : m_position(position)
    , m_normal(normal)
    , m_colorTexCoords(colorTexCoords)
    , m_length(length)
  {}

  glsl::vec3 m_position;
  glsl::vec2 m_normal;
  glsl::vec2 m_colorTexCoords;
  glsl::vec3 m_length;
};

dp::BindingInfo GetSelectionLineVertexBindingInfo()
{
  dp::BindingFiller<SelectionLineVertex> filler(4);
  filler.FillDecl<glsl::vec3>("a_position");
  filler.FillDecl<glsl::vec2>("a_normal");
  filler.FillDecl<glsl::vec2>("a_colorTexCoords");
  filler.FillDecl<glsl::vec3>("a_length");

  return filler.m_info;
}

float SideByNormal(glsl::vec2 const & normal, bool isLeft)
{
  float const kEps = 1e-5;
  float const side = isLeft ? kLeftSide : kRightSide;
  return glsl::length(normal) < kEps ? kCenter : side;
}

void GenerateJoinsTriangles(glsl::vec3 const & pivot, std::vector<glsl::vec2> const & normals,
                            glsl::vec2 const & colorCoord, glsl::vec2 const & length, bool isLeft,
                            std::vector<SelectionLineVertex> & geometry)
{
  size_t const trianglesCount = normals.size() / 3;
  for (size_t j = 0; j < trianglesCount; j++)
  {
    auto const len1 = glsl::vec3(length.x, length.y, SideByNormal(normals[3 * j], isLeft));
    auto const len2 = glsl::vec3(length.x, length.y, SideByNormal(normals[3 * j + 1], isLeft));
    auto const len3 = glsl::vec3(length.x, length.y, SideByNormal(normals[3 * j + 2], isLeft));

    geometry.emplace_back(pivot, normals[3 * j], colorCoord, len1);
    geometry.emplace_back(pivot, normals[3 * j + 1], colorCoord, len2);
    geometry.emplace_back(pivot, normals[3 * j + 2], colorCoord, len3);
  }
}
}  // namespace

// static
drape_ptr<RenderNode> SelectionShapeGenerator::GenerateSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                                       ref_ptr<dp::TextureManager> mng)
{
  size_t constexpr kTriangleCount = 40;
  size_t constexpr kVertexCount = 3 * kTriangleCount;
  auto const etalonSector = static_cast<float>(2.0 * math::pi / kTriangleCount);

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(df::GetColorConstant(df::kSelectionColor), color);
  auto const colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  buffer_vector<MarkerVertex, kTriangleCount> buffer;

  glsl::vec2 const startNormal(0.0f, 1.0f);

  for (size_t i = 0; i < kTriangleCount + 1; ++i)
  {
    auto const normal = glsl::rotate(startNormal, i * etalonSector);
    auto const nextNormal = glsl::rotate(startNormal, (i + 1) * etalonSector);

    buffer.emplace_back(glsl::vec2(0.0f, 0.0f), colorCoord);
    buffer.emplace_back(normal, colorCoord);
    buffer.emplace_back(nextNormal, colorCoord);
  }

  auto state = CreateRenderState(gpu::Program::Accuracy, DepthLayer::OverlayLayer);
  state.SetColorTexture(color.GetTexture());
  state.SetDepthTestEnabled(false);

  drape_ptr<RenderNode> renderNode;
  {
    dp::Batcher batcher(kTriangleCount * dp::Batcher::IndexPerTriangle, kVertexCount);
    batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
    dp::SessionGuard guard(context, batcher,
                           [&renderNode](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = std::move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());
      renderNode = make_unique_dp<RenderNode>(state, bucket->MoveBuffer());
    });

    dp::AttributeProvider provider(1 /* stream count */, kVertexCount);
    provider.InitStream(0 /* stream index */, GetMarkerBindingInfo(), make_ref(buffer.data()));

    batcher.InsertTriangleList(context, state, make_ref(&provider), nullptr);
  }
  return renderNode;
}

// static
drape_ptr<RenderNode> SelectionShapeGenerator::GenerateTrackSelectionMarker(ref_ptr<dp::GraphicsContext> context,
                                                                            ref_ptr<dp::TextureManager> mng)
{
  dp::TextureManager::SymbolRegion region;
  mng->GetSymbolRegion(kTrackSelectedSymbolName, region);
  m2::RectF const & texRect = region.GetTexRect();
  m2::PointF const pxSize = region.GetPixelSize();
  float const halfWidth = 0.5f * pxSize.x;
  float const halfHeight = 0.5f * pxSize.y;

  size_t constexpr kVertexCount = 4;
  buffer_vector<MarkerVertex, kVertexCount> buffer;
  buffer.emplace_back(glsl::vec2(-halfWidth, halfHeight), glsl::ToVec2(texRect.LeftTop()));
  buffer.emplace_back(glsl::vec2(-halfWidth, -halfHeight), glsl::ToVec2(texRect.LeftBottom()));
  buffer.emplace_back(glsl::vec2(halfWidth, halfHeight), glsl::ToVec2(texRect.RightTop()));
  buffer.emplace_back(glsl::vec2(halfWidth, -halfHeight), glsl::ToVec2(texRect.RightBottom()));

  auto state = CreateRenderState(gpu::Program::Accuracy, DepthLayer::OverlayLayer);
  state.SetColorTexture(region.GetTexture());
  state.SetDepthTestEnabled(false);
  state.SetTextureFilter(dp::TextureFilter::Nearest);
  state.SetTextureIndex(region.GetTextureIndex());

  drape_ptr<RenderNode> renderNode;
  {
    dp::Batcher batcher(dp::Batcher::IndexPerQuad, dp::Batcher::VertexPerQuad);
    batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
    dp::SessionGuard guard(context, batcher,
                           [&renderNode](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = std::move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());
      renderNode = make_unique_dp<RenderNode>(state, bucket->MoveBuffer());
    });

    dp::AttributeProvider provider(1 /* stream count */, kVertexCount);
    provider.InitStream(0 /* stream index */, GetMarkerBindingInfo(), make_ref(buffer.data()));

    batcher.InsertTriangleStrip(context, state, make_ref(&provider), nullptr);
  }

  return renderNode;
}

// static
drape_ptr<RenderNode> SelectionShapeGenerator::GenerateSelectionGeometry(ref_ptr<dp::GraphicsContext> context,
                                                                         FeatureID const & feature,
                                                                         ref_ptr<dp::TextureManager> mng,
                                                                         ref_ptr<MetalineManager> metalineMng,
                                                                         MapDataProvider & mapDataProvider)
{
  // Get spline from metaline manager or read from mwm.
  std::vector<m2::PointD> points;
  auto spline = metalineMng->GetMetaline(feature);
  if (spline.IsNull())
  {
    mapDataProvider.ReadFeatures([&points](FeatureType & ft)
    {
      if (ft.GetGeomType() == feature::GeomType::Line)
        assign_range(points, ft.GetPoints(scales::GetUpperScale()));
    }, {feature});
  }
  else
  {
    points = spline->GetPath();
  }

  // Generate line geometry.
  if (points.size() < 2)
    return nullptr;

  m2::RectD rect;
  for (auto const & p : points)
    rect.Add(p);
  m2::PointD const pivot = rect.Center();

  dp::TextureManager::ColorRegion color;
  mng->GetColorRegion(df::GetColorConstant(df::kSelectionColor), color);
  auto const colorCoord = glsl::ToVec2(color.GetTexRect().Center());

  // Construct segments.
  std::vector<LineSegment> segments;
  segments.reserve(points.size() - 1);
  ConstructLineSegments(points, {}, segments);
  if (segments.empty())
    return nullptr;

  // Build geometry.
  std::vector<SelectionLineVertex> geometry;
  size_t const segsCount = segments.size();
  geometry.reserve(segsCount * 24);
  float length = 0.0f;
  for (size_t i = 0; i < segsCount; ++i)
  {
    UpdateNormals(&segments[i], (i > 0) ? &segments[i - 1] : nullptr, (i < segsCount - 1) ? &segments[i + 1] : nullptr);

    // Generate main geometry.
    m2::PointD const startPt =
        MapShape::ConvertToLocal(glsl::FromVec2(segments[i].m_points[StartPoint]), pivot, kShapeCoordScalar);
    m2::PointD const endPt =
        MapShape::ConvertToLocal(glsl::FromVec2(segments[i].m_points[EndPoint]), pivot, kShapeCoordScalar);

    glsl::vec3 const startPivot = glsl::vec3(glsl::ToVec2(startPt), 0.0f);
    glsl::vec3 const endPivot = glsl::vec3(glsl::ToVec2(endPt), 0.0f);

    glsl::vec2 const leftNormalStart = GetNormal(segments[i], true /* isLeft */, StartNormal);
    glsl::vec2 const rightNormalStart = GetNormal(segments[i], false /* isLeft */, StartNormal);
    glsl::vec2 const leftNormalEnd = GetNormal(segments[i], true /* isLeft */, EndNormal);
    glsl::vec2 const rightNormalEnd = GetNormal(segments[i], false /* isLeft */, EndNormal);

    float const projLeftStart = -segments[i].m_leftWidthScalar[StartPoint].y;
    float const projLeftEnd = segments[i].m_leftWidthScalar[EndPoint].y;
    float const projRightStart = -segments[i].m_rightWidthScalar[StartPoint].y;
    float const projRightEnd = segments[i].m_rightWidthScalar[EndPoint].y;

    geometry.emplace_back(startPivot, glsl::vec2(0, 0), colorCoord, glsl::vec3(length, 0, kCenter));
    geometry.emplace_back(startPivot, leftNormalStart, colorCoord, glsl::vec3(length, projLeftStart, kLeftSide));
    geometry.emplace_back(endPivot, glsl::vec2(0, 0), colorCoord, glsl::vec3(length, 0, kCenter));

    geometry.emplace_back(endPivot, glsl::vec2(0, 0), colorCoord, glsl::vec3(length, 0, kCenter));
    geometry.emplace_back(startPivot, leftNormalStart, colorCoord, glsl::vec3(length, projLeftStart, kLeftSide));
    geometry.emplace_back(endPivot, leftNormalEnd, colorCoord, glsl::vec3(length, projLeftEnd, kLeftSide));

    geometry.emplace_back(startPivot, rightNormalStart, colorCoord, glsl::vec3(length, projRightStart, kRightSide));
    geometry.emplace_back(startPivot, glsl::vec2(0, 0), colorCoord, glsl::vec3(length, 0, kCenter));
    geometry.emplace_back(endPivot, rightNormalEnd, colorCoord, glsl::vec3(length, projRightEnd, kRightSide));

    geometry.emplace_back(endPivot, rightNormalEnd, colorCoord, glsl::vec3(length, projRightEnd, kRightSide));
    geometry.emplace_back(startPivot, glsl::vec2(0, 0), colorCoord, glsl::vec3(length, 0, kCenter));
    geometry.emplace_back(endPivot, glsl::vec2(0, 0), colorCoord, glsl::vec3(length, 0, kCenter));

    // Generate joins.
    if (segments[i].m_generateJoin && i < segsCount - 1)
    {
      glsl::vec2 const n1 = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_leftNormals[EndPoint]
                                                                : segments[i].m_rightNormals[EndPoint];
      glsl::vec2 const n2 = segments[i + 1].m_hasLeftJoin[StartPoint] ? segments[i + 1].m_leftNormals[StartPoint]
                                                                      : segments[i + 1].m_rightNormals[StartPoint];

      float const widthScalar = segments[i].m_hasLeftJoin[EndPoint] ? segments[i].m_rightWidthScalar[EndPoint].x
                                                                    : segments[i].m_leftWidthScalar[EndPoint].x;

      std::vector<glsl::vec2> normals =
          GenerateJoinNormals(dp::RoundJoin, n1, n2, 1.0f, segments[i].m_hasLeftJoin[EndPoint], widthScalar);
      GenerateJoinsTriangles(endPivot, normals, colorCoord, glsl::vec2(length, 0), segments[i].m_hasLeftJoin[EndPoint],
                             geometry);
    }

    // Generate caps.
    if (i == 0)
    {
      std::vector<glsl::vec2> normals =
          GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[StartPoint],
                             segments[i].m_rightNormals[StartPoint], -segments[i].m_tangent, 1.0f, true /* isStart */);

      GenerateJoinsTriangles(startPivot, normals, colorCoord, glsl::vec2(length, 0), true, geometry);
    }

    if (i == segsCount - 1)
    {
      std::vector<glsl::vec2> normals =
          GenerateCapNormals(dp::RoundCap, segments[i].m_leftNormals[EndPoint], segments[i].m_rightNormals[EndPoint],
                             segments[i].m_tangent, 1.0f, false /* isStart */);

      GenerateJoinsTriangles(endPivot, normals, colorCoord, glsl::vec2(length, 0), true, geometry);
    }

    length += glsl::length(segments[i].m_points[EndPoint] - segments[i].m_points[StartPoint]);
  }

  auto state = CreateRenderState(gpu::Program::SelectionLine, DepthLayer::OverlayLayer);
  state.SetColorTexture(color.GetTexture());
  state.SetDepthTestEnabled(false);

  drape_ptr<RenderNode> renderNode;
  {
    dp::Batcher batcher(static_cast<uint32_t>(geometry.size()), static_cast<uint32_t>(geometry.size()));
    batcher.SetBatcherHash(static_cast<uint64_t>(BatcherBucket::Default));
    dp::SessionGuard guard(context, batcher,
                           [&renderNode, &pivot, &rect](dp::RenderState const & state, drape_ptr<dp::RenderBucket> && b)
    {
      drape_ptr<dp::RenderBucket> bucket = std::move(b);
      ASSERT(bucket->GetOverlayHandlesCount() == 0, ());
      renderNode = make_unique_dp<RenderNode>(state, bucket->MoveBuffer());
      renderNode->SetPivot(pivot);
      renderNode->SetBoundingBox(rect);
    });

    dp::AttributeProvider provider(1 /* stream count */, static_cast<uint32_t>(geometry.size()));
    provider.InitStream(0 /* stream index */, GetSelectionLineVertexBindingInfo(), make_ref(geometry.data()));
    batcher.InsertTriangleList(context, state, make_ref(&provider), nullptr);
  }
  return renderNode;
}
}  // namespace df
