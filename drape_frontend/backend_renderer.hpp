#pragma once

#include "../base/commands_queue.hpp"
#include "../geometry/screenbase.hpp"

namespace df
{
  class BackendRenderer
  {
  public:
    BackendRenderer(int cpuCoreCount, size_t tileSize);
    ~BackendRenderer();

    void UpdateCoverage(const ScreenBase & screen);
    void Resize(int x0, int y0, int w, int h);

  private:
    void FinishTask(threads::IRoutine * routine);

  private:
    class Impl;
    Impl * m_impl;

  private:
    core::CommandsQueue m_queue;
  };
}
