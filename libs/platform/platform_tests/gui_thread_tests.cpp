#include "testing/testing.hpp"

#include "platform/gui_thread.hpp"

#include "base/thread.hpp"

#include <thread>
#include <utility>

#include <QtCore/QEventLoop>
#include <QtCore/QTimer>

namespace gui_thread_tests
{
UNIT_TEST(GuiThread_DefersTasksOnGuiThread)
{
  auto const guiThread = threads::GetCurrentThreadID();
  for (bool const fromWorker : {false, true})
  {
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    int movedCalls = 0;
    int copiedCalls = 0;
    bool onGuiThread = true;
    bool movedPosted = false;
    bool copiedPosted = false;
    auto const onTask = [&](int & calls)
    {
      ++calls;
      onGuiThread &= threads::GetCurrentThreadID() == guiThread;
      if (movedCalls + copiedCalls == 2)
        loop.quit();
    };
    auto const post = [&]
    {
      platform::GuiThread gui;
      base::TaskLoop::Task movedTask = [&] { onTask(movedCalls); };
      movedPosted = gui.Push(std::move(movedTask)).m_isSuccess;
      base::TaskLoop::Task const copiedTask = [&] { onTask(copiedCalls); };
      copiedPosted = gui.Push(copiedTask).m_isSuccess;
    };
    if (fromWorker)
    {
      std::thread worker(post);
      worker.join();
    }
    else
      post();
    int const callsBeforeProcessing = movedCalls + copiedCalls;
    if (callsBeforeProcessing < 2)
    {
      timeout.start(5000);
      loop.exec();
    }
    TEST(movedPosted && copiedPosted, (fromWorker));
    TEST_EQUAL(callsBeforeProcessing, 0, ("Posting must be deferred", fromWorker));
    TEST_EQUAL(movedCalls, 1, (fromWorker));
    TEST_EQUAL(copiedCalls, 1, (fromWorker));
    TEST(onGuiThread, (fromWorker));
  }
}
}  // namespace gui_thread_tests
