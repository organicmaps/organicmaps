#include "testing/testing.hpp"

#include "storage/storage_tests/helpers.hpp"

#include "storage/country_info_getter.hpp"

#include "platform/platform.hpp"

#include <chrono>
#include <memory>

namespace storage
{
std::unique_ptr<storage::CountryInfoGetter> CreateCountryInfoGetter()
{
  return CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
}

void RunEventLoopOrFail()
{
  auto const finished = std::make_shared<bool>(false);
  auto const timedOut = std::make_shared<bool>(false);
  GetPlatform().RunDelayedTask(Platform::Thread::Background, std::chrono::seconds(60), [finished, timedOut]
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [finished, timedOut]
    {
      if (*finished)
        return;
      *timedOut = true;
      testing::StopEventLoop();
    });
  });
  testing::RunEventLoop();
  *finished = true;
  TEST(!*timedOut, ("The event loop has not been stopped"));
}

FileThreadBlocker::FileThreadBlocker()
{
  auto const release = m_release.get_future().share();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [release]() { release.wait(); }).m_isSuccess, ());
}

void FileThreadBlocker::Release()
{
  if (!m_released)
  {
    m_release.set_value();
    m_released = true;
  }
}

void WaitForFileThread()
{
  std::promise<void> fileIdle;
  auto const idle = fileIdle.get_future();
  TEST(GetPlatform().RunTask(Platform::Thread::File, [&fileIdle]() { fileIdle.set_value(); }).m_isSuccess, ());
  idle.wait();
}

void DrainFileAndGui()
{
  // File tasks and GUI posts are FIFO. Posting the marker from File keeps it deferred on Qt.
  bool guiDrained = false;
  auto const posted = GetPlatform().RunTask(Platform::Thread::File, [&guiDrained]()
  {
    GetPlatform().RunTask(Platform::Thread::Gui, [&guiDrained]()
    {
      guiDrained = true;
      testing::StopEventLoop();
    });
  });
  TEST(posted.m_isSuccess, ());
  RunEventLoopOrFail();
  TEST(guiDrained, ());
}
}  // namespace storage
