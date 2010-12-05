#pragma once

#include "../std/function.hpp"

class QWidget;

namespace tst
{
  /// Base tester. Can work with any widgets from widgets.hpp
  /// Works in screen coordinates with user defined widget.
  class BaseTester
  {
  public:

    /// Main run function.
    int Run(char const * name, function<QWidget * (void)> const & fn);
  };
}
