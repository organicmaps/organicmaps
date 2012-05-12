#pragma once

#include "basic_render_policy.hpp"

class RenderPolicyMT : public BasicRenderPolicy
{
public:
  RenderPolicyMT(Params const & p);
};
