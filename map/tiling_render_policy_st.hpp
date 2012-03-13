#pragma once

#include "basic_tiling_render_policy.hpp"

class TilingRenderPolicyST : public BasicTilingRenderPolicy
{
private:

  int m_maxTilesCount;

public:

  TilingRenderPolicyST(VideoTimer * videoTimer,
                       bool useDefaultFB,
                       yg::ResourceManager::Params const & rmParams,
                       shared_ptr<yg::gl::RenderContext> const & primaryRC);

  ~TilingRenderPolicyST();

  void SetRenderFn(TRenderFn renderFn);
};
