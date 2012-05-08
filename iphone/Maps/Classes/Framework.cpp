#include "Framework.h"

#include "../../../map/framework_factory.hpp"

static Framework * g_framework = 0;

Framework & GetFramework()
{
  if (g_framework == 0)
    g_framework = FrameworkFactory::CreateFramework();
  return *g_framework;
}

void DeleteFramework()
{
  delete g_framework;
  g_framework = 0;
}