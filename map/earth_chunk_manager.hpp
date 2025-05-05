#pragma once

#include "base/thread_checker.hpp"

class EarthChunkManager final
{
public:
  explicit EarthChunkManager();

  void LoadEarthChunks();

private:
  bool m_loadEarthChunksCalled = false;
  ThreadChecker m_threadChecker;
};
