#pragma once

#include "basic_tiling_render_policy.hpp"

class TilingRenderPolicyST : public BasicTilingRenderPolicy
{
private:

  int m_maxTilesCount;

public:

  TilingRenderPolicyST(Params const & p);

  ~TilingRenderPolicyST();

  void SetRenderFn(TRenderFn renderFn);
};
