#include "Framework.h"

#include "base/assert.hpp"

static Framework * g_framework = 0;
bool wasDeleted = false;

Framework & GetFramework()
{
  CHECK(!wasDeleted, ());
  if (g_framework == 0)
    g_framework = new Framework();
  return *g_framework;
}

void DeleteFramework()
{
  wasDeleted = true;
  delete g_framework;
  g_framework = nullptr;
}
