#pragma once

#include "framework.hpp"
#include "../std/shared_ptr.hpp"

class WindowHandle;

template <typename TModel>
class FrameworkFactory
{
public:
  static Framework<TModel> * CreateFramework();
};
