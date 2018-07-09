#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"

#include <vector>

namespace df
{

struct BuildingOutline
{
  buffer_vector<m2::PointD, kBuildingOutlineSize> m_vertices;
  std::vector<int> m_indices;
  std::vector<m2::PointD> m_normals;
  bool m_generateOutline = false;
};

class AreaShape : public MapShape
{
public:
  AreaShape(std::vector<m2::PointD> && triangleList, BuildingOutline && buildingOutline,
            AreaViewParams const & params);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;

private:
  void DrawArea(ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                m2::PointD const & outlineUv, ref_ptr<dp::Texture> texture) const;
  void DrawArea3D(ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                  m2::PointD const & outlineUv, ref_ptr<dp::Texture> texture) const;
  void DrawHatchingArea(ref_ptr<dp::Batcher> batcher, m2::PointD const & colorUv,
                        ref_ptr<dp::Texture> texture, ref_ptr<dp::Texture> hatchingTexture) const;

  std::vector<m2::PointD> m_vertexes;
  BuildingOutline m_buildingOutline;
  AreaViewParams m_params;
};
}  // namespace df
