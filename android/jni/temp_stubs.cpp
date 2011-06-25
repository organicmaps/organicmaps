#include "../../platform/concurrent_runner.hpp"

#include "../../platform/download_manager.hpp"

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
  }

  void ConcurrentRunner::Join()
  {
  }
}

class AndroidDownloadManager : public DownloadManager
{
public:

  virtual void HttpRequest(HttpStartParams const & params)
  {
  }

  virtual void CancelDownload(string const & url)
  {
  }

  virtual void CancelAllDownloads()
  {
  }
};

DownloadManager & GetDownloadManager()
{
  static AndroidDownloadManager manager;
  return manager;
}
