#pragma once

#include "drape_frontend/arrow3d.hpp"
#include "drape_frontend/render_node.hpp"
#include "drape_frontend/render_state.hpp"

#include "drape/vertex_array_buffer.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/texture_manager.hpp"
#include "drape/uniform_values_storage.hpp"
#include "drape/batcher.hpp"

#include "geometry/screenbase.hpp"

namespace df
{

class MyPosition
{
public:
  MyPosition(ref_ptr<dp::TextureManager> mng);

  ///@param pt = mercator point
  void SetPosition(m2::PointF const & pt);
  void SetAzimuth(float azimut);
  void SetIsValidAzimuth(bool isValid);
  void SetAccuracy(float accuracy);
  void SetRoutingMode(bool routingMode);
  void SetPositionObsolete(bool obsolete);

  void RenderAccuracy(ScreenBase const & screen, int zoomLevel,
                      ref_ptr<dp::GpuProgramManager> mng,
                      dp::UniformValuesStorage const & commonUniforms);

  void RenderMyPosition(ScreenBase const & screen, int zoomLevel,
                        ref_ptr<dp::GpuProgramManager> mng,
                        dp::UniformValuesStorage const & commonUniforms);

private:
  void CacheAccuracySector(ref_ptr<dp::TextureManager> mng);
  void CachePointPosition(ref_ptr<dp::TextureManager> mng);

  enum EMyPositionPart
  {
    // don't change order and values
    MY_POSITION_ACCURACY = 0,
    MY_POSITION_POINT = 1,
  };

  void RenderPart(ref_ptr<dp::GpuProgramManager> mng,
                  dp::UniformValuesStorage const & uniforms,
                  EMyPositionPart part);

  void CacheSymbol(dp::TextureManager::SymbolRegion const & symbol,
                   dp::GLState const & state, dp::Batcher & batcher,
                   EMyPositionPart part);

  m2::PointF m_position;
  float m_azimuth;
  float m_accuracy;
  bool m_showAzimuth;
  bool m_isRoutingMode;
  bool m_obsoletePosition;

  using TPart = pair<dp::IndicesRange, size_t>;

  vector<TPart> m_parts;
  vector<RenderNode> m_nodes;

  Arrow3d m_arrow3d;
};

}
