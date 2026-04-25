#pragma once

#include "qt/qt_common/renderer/base/renderer_window.hpp"

class Framework;

namespace qt::common::renderer
{
class RendererFactory
{
public:
  static base::RendererWindow * CreateRendererWindow(Framework & framework, dp::ApiVersion api);
};
}  // namespace qt::common::renderer
