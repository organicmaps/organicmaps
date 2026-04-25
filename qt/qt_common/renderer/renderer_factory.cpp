#include "renderer_factory.hpp"

#include "drape/drape_global.hpp"

#ifdef OMIM_METAL_AVAILABLE
#include "qt/qt_common/renderer/metal/metal_window.hpp"
#endif
#include "qt/qt_common/renderer/opengl/opengl_window.hpp"

namespace qt::common::renderer
{
base::RendererWindow * RendererFactory::CreateRendererWindow(Framework & framework, dp::ApiVersion api)
{
  switch (api)
  {
#ifdef OMIM_METAL_AVAILABLE
  case dp::ApiVersion::Metal: return new metal::MetalWindow(framework);
#endif
  case dp::ApiVersion::OpenGLES3: return new opengl::OpenGLWindow(framework);
  default: ASSERT(false, ("Unsupported graphics API")); return nullptr;
  }
}
}  // namespace qt::common::renderer
