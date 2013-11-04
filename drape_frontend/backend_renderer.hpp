#pragma once

#include "../base/commands_queue.hpp"
#include "../geometry/screenbase.hpp"

namespace df
{
  class BackendRenderer
  {
  public:
    BackendRenderer(int cpuCoreCount, size_t tileSize);

    void UpdateCoverage(const ScreenBase & screen);
    void Resize(int x0, int y0, int w, int h);

  private:
    class Impl;
    Impl * m_impl;

    void Destroy(Impl * impl);

  private:
    core::CommandsQueue m_queue;
  };
}
