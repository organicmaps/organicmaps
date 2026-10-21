#pragma once

#include "geometry/mercator.hpp"

#include <future>
#include <memory>

namespace storage
{
class CountryInfoGetter;

std::unique_ptr<CountryInfoGetter> CreateCountryInfoGetter();

// A regression fails the test instead of hanging the suite in a never stopped event loop.
void RunEventLoopOrFail();

// Holds Platform::Thread::File until released, also when a failed TEST leaves the scope.
class FileThreadBlocker
{
public:
  FileThreadBlocker();
  ~FileThreadBlocker() { Release(); }

  void Release();

private:
  std::promise<void> m_release;
  bool m_released = false;
};

// Waits for the File tasks posted so far.
void WaitForFileThread();
// Runs the File tasks posted so far and the GUI tasks they post.
void DrainFileAndGui();
}  // namespace storage
