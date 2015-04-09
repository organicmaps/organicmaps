#pragma once

#include "map/basic_tiling_render_policy.hpp"

class TilingRenderPolicyST : public BasicTilingRenderPolicy
{
public:

  TilingRenderPolicyST(Params const & p);

  ~TilingRenderPolicyST();

  void SetRenderFn(TRenderFn const & renderFn);
};
