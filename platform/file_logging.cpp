#include "file_logging.hpp"

#include "../base/mutex.hpp"

#include "../std/chrono.hpp"

#include "../coding/file_writer.hpp"

#include "../platform/platform.hpp"


void LogMessageFile(my::LogLevel level, my::SrcPoint const & srcPoint, string const & msg)
{
  static threads::Mutex mutex;

  threads::MutexGuard guard(mutex);
  UNUSED_VALUE(guard);

  static unique_ptr<FileWriter> file;

  if (file == nullptr)
  {
    if (GetPlatform().WritableDir().empty())
      return;
    auto const curTime = system_clock::now();
    time_t const curCTime = system_clock::to_time_t(curTime);
    tm * curTimeTM = localtime(&curCTime);
    assert(curTimeTM != nullptr);
    stringstream fileName;
    fileName << "logging_" << curTimeTM->tm_year + 1900 << "_" << curTimeTM->tm_mon + 1 << "_" << curTimeTM->tm_mday << "_"
      << curTimeTM->tm_hour << "_" << curTimeTM->tm_min << "_" << curTimeTM->tm_sec << ".txt";
    file.reset(new FileWriter(GetPlatform().WritablePathForFile(fileName.str())));
  }

  string srcString = DebugPrint(srcPoint) + " " + msg + "\n";

  file->Write(srcString.c_str(), srcString.size());
  file->Flush();
}

void LogMemoryInfo()
{
#ifdef DEBUG
  static unsigned long counter = 0;
  const unsigned short writeLogEveryNthLocationUpdate = 3;
  if (counter % writeLogEveryNthLocationUpdate == 0)
  {
    auto const curTime = system_clock::now();
    time_t const curCTime = system_clock::to_time_t(curTime);
    tm * curTimeTM = localtime(&curCTime);
    ASSERT(curTimeTM != nullptr, ());
    stringstream fileName;
    fileName << " " << curTimeTM->tm_year + 1900 << "." << curTimeTM->tm_mon + 1 << "." << curTimeTM->tm_mday << " "
      << curTimeTM->tm_hour << ":" << curTimeTM->tm_min << ":" << curTimeTM->tm_sec << " ";
    LOG(LDEBUG, (fileName.str(), GetPlatform().GetMemoryInfo()));
  }
  ++counter;
#endif
}
