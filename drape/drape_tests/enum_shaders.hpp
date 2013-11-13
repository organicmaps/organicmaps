#pragma once

#include "../../std/vector.hpp"
#include "../../std/string.hpp"

namespace gpu_test
{
  vector<string> VertexEnum;
  vector<string> FragmentEnum;

  void InitEnumeration()
  {
    VertexEnum.push_back("/Users/ExMix/develop/omim/drape/shaders/simple_vertex_shader.vsh");
    VertexEnum.push_back("/Users/ExMix/develop/omim/drape/shaders/texturing_vertex_shader.vsh");
    FragmentEnum.push_back("/Users/ExMix/develop/omim/drape/shaders/solid_area_fragment_shader.fsh");
    FragmentEnum.push_back("/Users/ExMix/develop/omim/drape/shaders/texturing_fragment_shader.fsh");
  }
}
