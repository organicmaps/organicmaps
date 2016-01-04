#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "std/vector.hpp"

namespace df
{

struct BuildingEdge
{
  m2::PointD m_startVertex;
  m2::PointD m_endVertex;
  m2::PointD m_normal;
};

class AreaShape : public MapShape
{
public:
  AreaShape(vector<m2::PointF> && triangleList, vector<BuildingEdge> && buildingEdges,
            AreaViewParams const & params);

  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;

  MapShapePriority GetPriority() const override { return MapShapePriority::AreaPriority; }

private:
  vector<m2::PointF> m_vertexes;
  vector<BuildingEdge> m_buildingEdges;
  AreaViewParams m_params;
};

} // namespace df
