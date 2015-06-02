#pragma once

#include "storage/map_files_downloader.hpp"
#include "base/thread_checker.hpp"

namespace storage
{
class MessageLoop;

// This class can be used in tests to mimic a real downloader.  It
// always returns a single URL for map files downloading and when
// asked for a file, creates a file with zero-bytes content on a disk.
// Because all callbacks must be invoked asynchroniously, it needs a
// single-thread message loop runner to run callbacks.
//
// *NOTE*, this class is not thread-safe.
class FakeMapFilesDownloader : public MapFilesDownloader
{
public:
  FakeMapFilesDownloader(MessageLoop & loop);
  virtual ~FakeMapFilesDownloader();

  // MapFilesDownloader overrides:
  void GetServersList(string const & mapFileName, TServersListCallback const & callback) override;
  void DownloadMapFile(vector<string> const & urls, string const & path, int64_t size,
                       TFileDownloadedCallback const & onDownloaded,
                       TDownloadingProgressCallback const & onProgress) override;
  TProgress GetDownloadingProgress() override;
  bool IsIdle() override;
  void Reset() override;

 private:
  vector<string> m_servers;
  TProgress m_progress;
  bool m_idle;

  MessageLoop & m_loop;
  ThreadChecker m_checker;
};
}  // namespace storage
