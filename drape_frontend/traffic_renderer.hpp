#pragma once

#include "drape_frontend/traffic_generator.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"
#include "drape/uniform_values_storage.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/spline.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"
#include "std/unordered_map.hpp"

namespace df
{

class TrafficRenderer final
{
public:
  TrafficRenderer() = default;

  void AddRenderData(ref_ptr<dp::GpuProgramManager> mng,
                     vector<TrafficRenderData> && renderData);

  void SetTexCoords(unordered_map<int, glsl::vec2> && texCoords);

  void UpdateTraffic(vector<TrafficSegmentData> const & trafficData);

  void RenderTraffic(ScreenBase const & screen, int zoomLevel,
                     ref_ptr<dp::GpuProgramManager> mng,
                     dp::UniformValuesStorage const & commonUniforms);

  void Clear();

private:
  vector<TrafficRenderData> m_renderData;
  unordered_map<int, glsl::vec2> m_texCoords;
  unordered_map<uint64_t, TrafficHandle *> m_handles;
};

} // namespace df
