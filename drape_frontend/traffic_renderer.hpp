#pragma once

#include "drape_frontend/traffic_generator.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"
#include "drape/uniform_values_storage.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/spline.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"

namespace df
{

class TrafficRenderer final
{
public:
  TrafficRenderer() = default;

  void AddRenderData(ref_ptr<dp::GpuProgramManager> mng,
                     TrafficRenderData && renderData);

  void SetTexCoords(TrafficTexCoords && texCoords);

  void UpdateTraffic(TrafficSegmentsColoring const & trafficColoring);

  void RenderTraffic(ScreenBase const & screen, int zoomLevel,
                     ref_ptr<dp::GpuProgramManager> mng,
                     dp::UniformValuesStorage const & commonUniforms);

  void ClearGLDependentResources();
  void Clear(MwmSet::MwmId const & mwmId);

private:
  vector<TrafficRenderData> m_renderData;
  TrafficTexCoords m_texCoords;
  map<TrafficSegmentID, TrafficHandle *> m_handles;
};

} // namespace df
