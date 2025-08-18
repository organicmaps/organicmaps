#pragma once

#include "drape_frontend/gui/gui_text.hpp"
#include "drape_frontend/gui/shape.hpp"

#include <functional>
#include <string>
#include <vector>

namespace gui
{
using TUpdateDebugLabelFn = std::function<bool(ScreenBase const & screen, std::string & content)>;

class DebugInfoLabels : public Shape
{
public:
  explicit DebugInfoLabels(gui::Position const & position) : Shape(position) {}

  void AddLabel(ref_ptr<dp::TextureManager> tex, std::string const & caption, TUpdateDebugLabelFn const & onUpdateFn);
  drape_ptr<ShapeRenderer> Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::TextureManager> tex);

private:
  std::vector<MutableLabelDrawer::Params> m_labelsParams;
  uint32_t m_labelsCount = 0;
};
}  // namespace gui
