#pragma once

#include "qt/qt_common/renderer/base/renderer_window.hpp"

class MetalContextFactory;

namespace qt::common::renderer::metal
{
class MetalWindow : public base::RendererWindow
{
  Q_OBJECT

public:
  explicit MetalWindow(Framework & framework, QWindow * parent = nullptr);
  ~MetalWindow() override;

protected:
  void Render() override;

private:
  void EnsureInitialized();

  drape_ptr<MetalContextFactory> m_contextFactory;
};
}  // namespace qt::common::renderer::metal
