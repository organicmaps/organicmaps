#pragma once

#include "framework.hpp"
#include "../std/shared_ptr.hpp"

class WindowHandle;

class FrameworkFactory
{
public:
  static Framework * CreateFramework();
};
