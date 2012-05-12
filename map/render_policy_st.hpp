#pragma once

#include "basic_render_policy.hpp"

class RenderPolicyST : public BasicRenderPolicy
{
public:
  RenderPolicyST(Params const & p);

  ~RenderPolicyST();
};
