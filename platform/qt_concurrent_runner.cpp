#include "concurrent_runner.hpp"

#include <QtCore/QtConcurrentRun>
#include <QtCore/QThreadPool>

namespace threads
{
  ConcurrentRunner::ConcurrentRunner()
  {
  }

  ConcurrentRunner::~ConcurrentRunner()
  {
  }

  void ConcurrentRunner::Run(RunnerFuncT const & f) const
  {
    QtConcurrent::run(f);
  }

  void ConcurrentRunner::Join()
  {
    QThreadPool::globalInstance()->waitForDone();
  }
}
