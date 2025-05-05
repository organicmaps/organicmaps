#include "map/earth_chunk_manager.hpp"

#include "base/macros.hpp"

#include <healpix_base.h>

EarthChunkManager::EarthChunkManager() {}

void EarthChunkManager::LoadEarthChunks()
{
  CHECK_THREAD_CHECKER(m_threadChecker, ());
  CHECK(!m_loadEarthChunksCalled, ("LoadEarthChunks should be called only once."));
  m_loadEarthChunksCalled = true;
}
