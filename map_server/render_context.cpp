#include "map_server/render_context.hpp"

namespace srv
{
  RenderContext::RenderContext()
  {
  }

  void RenderContext::makeCurrent()
  {
  }

  RenderContext * RenderContext::createShared()
  {
    return new RenderContext();
  }
}
