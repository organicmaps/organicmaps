#pragma once

#include "maps_form.h"
#include "../../../map/window_handle.hpp"
#include "render_context.hpp"

namespace bada
{
  struct WindowHandle : public ::WindowHandle<RenderContext>
  {
    MapsForm * m_form;

    WindowHandle(MapsForm * form);
    void invalidate();
  };
}
