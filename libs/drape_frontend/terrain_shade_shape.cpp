#include "drape_frontend/terrain_shade_shape.hpp"

#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "shaders/programs.hpp"

#include "drape/attribute_provider.hpp"
#include "drape/batcher.hpp"

#include "indexer/drawing_rule_def.hpp"
#include "indexer/terrain/terrain_normals.hpp"

#include "coding/point_coding.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/robust_orientation.hpp"

#include <cmath>

namespace df
{
namespace
{
glsl::vec3 ToUnitNormal(terrain::TerrainNormal const & n)
{
  double const length = std::sqrt(n.m_x * n.m_x + n.m_y * n.m_y + n.m_z * n.m_z);
  return length < 1e-15 ? glsl::vec3(0, 0, 1) : glsl::vec3(n.m_x / length, n.m_y / length, n.m_z / length);
}
}  // namespace

TerrainShadeGeometry BuildTerrainShade(terrain::TerrainData const & data, m2::RectD const & tileRect,
                                       base::SharedCancellable const & cancellable)
{
  auto const & mesh = data.m_mesh;

  // Mercator units per altitude meter at the tile latitude (the projection is conformal:
  // the x and y local scales are equal).
  m2::PointD const center = tileRect.Center();
  double const zScale = 1.0 / mercator::DistanceOnEarth({center.x - 0.5, center.y}, {center.x + 0.5, center.y});

  auto const & points = mesh.GetPoints();
  auto const & triangles = mesh.GetTriangles();

  // Pass 1: accumulate the area-weighted face normals per vertex. The mesh vertices are
  // deduplicated across features and blocks, so the smoothed per-vertex normals agree
  // along every seam. Triangles outside the tile still contribute to their vertices,
  // only the intersecting ones are emitted.
  auto const jointNormals = terrain::CalculateTerrainNormals(data, zScale, cancellable);
  if (cancellable.IsCancelled())
    return {};
  TerrainShadeGeometry geometry;
  auto & vertices = geometry.m_triangles;
  auto const makeVertex = [&](m2::PointD const & p, glsl::vec3 const & normal)
  {
    m2::PointD const local = MapShape::ConvertToLocal(p, center, kShapeCoordScalar);
    return gpu::TerrainShadeVertex(glsl::vec3(local.x, local.y, drule::kMinLayeredDepthFg), normal);
  };
  // Clips to the tile (the alpha layer must not double-blend across the tiles) and emits in
  // the CW drape winding (cf. ApplyAreaFeature); TWM triangles are CCW. The clip points
  // interpolate the vertex normals barycentrically (the shader renormalizes).
  auto const emitClipped = [&](m2::PointD const & pa, m2::PointD const & pb, m2::PointD const & pc,
                               glsl::vec3 const & na, glsl::vec3 const & nb, glsl::vec3 const & nc)
  {
    double const orientation = m2::robust::OrientedS(pa, pb, pc);
    double constexpr kEmptyTriangleS = kMwmPointAccuracy * kMwmPointAccuracy * 0.01;
    if (std::fabs(orientation) < kEmptyTriangleS)
      return;

    if (tileRect.IsPointInside(pa) && tileRect.IsPointInside(pb) && tileRect.IsPointInside(pc))
    {
      vertices.push_back(makeVertex(pa, na));
      vertices.push_back(orientation < 0 ? makeVertex(pb, nb) : makeVertex(pc, nc));
      vertices.push_back(orientation < 0 ? makeVertex(pc, nc) : makeVertex(pb, nb));
      return;
    }
    auto const emit = [&](m2::PointD const & ca, m2::PointD const & cb, m2::PointD const & cc)
    {
      for (auto const & p : {ca, cb, cc})
      {
        auto const fa = static_cast<float>(m2::CrossProduct(p - pc, pb - pc) / orientation);
        auto const fb = static_cast<float>(m2::CrossProduct(pa - pc, p - pc) / orientation);
        vertices.push_back(makeVertex(p, fa * na + fb * nb + (1.0f - fa - fb) * nc));
      }
    };
    if (orientation < 0)
      m2::ClipTriangleByRect(tileRect, pa, pb, pc, emit);
    else
      m2::ClipTriangleByRect(tileRect, pa, pc, pb, emit);
  };

  // Only the drawn vertices are normalized; the full halo above still contributes to their
  // smoothed normals. Lighting remains entirely in the shader.
  std::vector<glsl::vec3> unitNormals(points.size());
  std::vector<bool> normalized(points.size(), false);
  auto const unitNormal = [&](uint32_t i) -> glsl::vec3 const &
  {
    if (!normalized[i])
    {
      normalized[i] = true;
      unitNormals[i] = ToUnitNormal(jointNormals.m_tin[i]);
    }
    return unitNormals[i];
  };
  for (size_t t = 0; t + 2 < triangles.size(); t += 3)
  {
    uint32_t const i0 = triangles[t], i1 = triangles[t + 1], i2 = triangles[t + 2];
    m2::RectD triRect(points[i0], points[i1]);
    triRect.Add(points[i2]);
    if (tileRect.IsIntersect(triRect))
      emitClipped(points[i0], points[i1], points[i2], unitNormal(i0), unitNormal(i1), unitNormal(i2));
  }

  auto & gridRows = geometry.m_gridRows;
  for (size_t p = 0; p < data.m_rasterPatches.size(); ++p)
  {
    if (cancellable.IsCancelled())
      return {};
    auto const & patch = data.m_rasterPatches[p];
    if (!tileRect.IsIntersect(patch.GetRect()))
      continue;
    std::vector<glsl::vec3> gridNormals(patch.m_heights.size());
    std::vector<bool> gridNormalized(patch.m_heights.size(), false);
    auto const gridNormal = [&](uint32_t i) -> glsl::vec3 const &
    {
      if (!gridNormalized[i])
      {
        gridNormalized[i] = true;
        gridNormals[i] = ToUnitNormal(jointNormals.m_raster[p][i]);
      }
      return gridNormals[i];
    };
    uint32_t const stride = patch.CellsX() + 1;
    for (uint32_t y = 0; y < patch.CellsY(); ++y)
    {
      if (cancellable.IsCancelled())
        return {};
      std::vector<gpu::TerrainShadeVertex> row;
      auto const flushRow = [&]
      {
        if (!row.empty())
          gridRows.push_back(std::move(row));
        row.clear();
      };
      for (uint32_t x = 0; x < patch.CellsX(); ++x)
      {
        uint32_t const sw = y * stride + x, ne = sw + stride + 1;
        auto const a = patch.Point(sw), b = patch.Point(ne);
        if (tileRect.IsPointInside(a) && tileRect.IsPointInside(b))
        {
          if (row.empty())
          {
            auto const nw = makeVertex(patch.Point(sw + stride), gridNormal(sw + stride));
            row.push_back(nw);  // Shift strip parity to CW.
            row.push_back(nw);
            row.push_back(makeVertex(a, gridNormal(sw)));
          }
          row.push_back(makeVertex(b, gridNormal(ne)));
          row.push_back(makeVertex(patch.Point(sw + 1), gridNormal(sw + 1)));
          continue;
        }
        flushRow();
        if (!tileRect.IsIntersect(m2::RectD(a, b)))
          continue;
        // Only edge cells need temporary clipped triangles. Normals are
        // interpolated on exactly the same fixed-diagonal surface as the TIN path.
        for (uint32_t t : {2 * (y * patch.CellsX() + x), 2 * (y * patch.CellsX() + x) + 1})
        {
          auto const v = patch.Triangle(t);
          emitClipped(patch.Point(v[0]), patch.Point(v[1]), patch.Point(v[2]), gridNormal(v[0]), gridNormal(v[1]),
                      gridNormal(v[2]));
        }
      }
      flushRow();
    }
  }
  return geometry;
}

void TerrainShadeShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                             ref_ptr<dp::TextureManager> /* textures */) const
{
  // The geometry layer with the default alpha blending, drawn after the area fills and the
  // solid-fill patterns but before the hatchings (see the Program enum). Lighting uses
  // vertex normals and the live light direction, so no color texture is needed.
  // Every tile shades at the same depth: Less blends one layer only where the tiles of two
  // zooms or generations overlap until the old ones are deleted.
  auto state = CreateRenderState(gpu::Program::TerrainShade, DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(true);
  state.SetDepthFunction(dp::TestFunction::Less);

  dp::AttributeProvider provider(1, static_cast<uint32_t>(m_vertices.size()));
  // The provider only reads the stream; ref_ptr is not const-aware.
  provider.InitStream(0, gpu::TerrainShadeVertex::GetBindingInfo(),
                      make_ref(const_cast<gpu::TerrainShadeVertex *>(m_vertices.data())));
  batcher->InsertTriangleList(context, state, make_ref(&provider));
}

void TerrainGridShadeShape::Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
                                 ref_ptr<dp::TextureManager> /* textures */) const
{
  auto state = CreateRenderState(gpu::Program::TerrainShade, DepthLayer::GeometryLayer);
  state.SetDepthTestEnabled(true);
  state.SetDepthFunction(dp::TestFunction::Less);
  for (auto const & row : m_rows)
  {
    dp::AttributeProvider provider(1, static_cast<uint32_t>(row.size()));
    provider.InitStream(0, gpu::TerrainShadeVertex::GetBindingInfo(),
                        make_ref(const_cast<gpu::TerrainShadeVertex *>(row.data())));
    batcher->InsertTriangleStrip(context, state, make_ref(&provider), false /* canDivide */);
  }
}
}  // namespace df
