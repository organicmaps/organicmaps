#include "tracking/archival_reporter.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <chrono>

namespace
{
double constexpr kRequiredHorizontalAccuracyM = 15.0;
}  // namespace

namespace tracking
{
ArchivalReporter::ArchivalReporter(std::string const & host)
  : m_archiveBicycle(kItemsForDump, kMinDelaySecondsBicycle)
  , m_archivePedestrian(kItemsForDump, kMinDelaySecondsPedestrian)
  , m_archiveCar(kItemsForDump, kMinDelaySecondsCar)
  , m_manager(host)
  , m_isAlive(true)
  , m_threadDump([this] { Run(); })
{}

ArchivalReporter::~ArchivalReporter()
{
  DumpToDisk(true /* dumpAnyway */);
  {
    std::unique_lock<std::mutex> lock(m_mutexRun);
    m_isAlive = false;
  }
  m_cvDump.notify_one();
  m_threadDump.join();
}

void ArchivalReporter::SetArchivalManagerSettings(ArchivingSettings const & settings)
{
  m_manager.SetSettings(settings);
}

void ArchivalReporter::Run()
{
  {
    std::lock_guard<std::mutex> lock(m_mutexRun);
    m_manager.DeleteOldDataByExtension(ARCHIVE_TRACKS_FILE_EXTENSION);
    m_manager.DeleteOldDataByExtension(ARCHIVE_TRACKS_ZIPPED_FILE_EXTENSION);
  }

  while (m_isAlive)
  {
    std::unique_lock<std::mutex> lock(m_mutexRun);
    m_cvDump.wait_for(lock, std::chrono::seconds(m_manager.IntervalBetweenDumpsSeconds()),
                      [this] { return !m_isAlive; });
    DumpToDisk(true);
    m_manager.PrepareUpload();
  }

  LOG(LDEBUG, ("Exiting thread for dumping and uploading tracks"));
}

void ArchivalReporter::Insert(routing::RouterType const & trackType, location::GpsInfo const & info,
                              traffic::SpeedGroup const & speedGroup)
{
  if (info.m_horizontalAccuracy > kRequiredHorizontalAccuracyM)
    return;

  switch (trackType)
  {
  case routing::RouterType::Vehicle:
  {
    if (!m_archiveCar.Add(info, speedGroup))
      return;
    break;
  }
  case routing::RouterType::Bicycle:
  {
    if (!m_archiveBicycle.Add(info))
      return;
    break;
  }
  case routing::RouterType::Pedestrian:
  {
    if (!m_archivePedestrian.Add(info))
      return;
    break;
  }
  default: UNREACHABLE();
  }

  DumpToDisk();
}

void ArchivalReporter::DumpToDisk(bool dumpAnyway)
{
  m_manager.Dump(m_archiveCar, routing::RouterType::Vehicle, dumpAnyway);
  m_manager.Dump(m_archiveBicycle, routing::RouterType::Bicycle, dumpAnyway);
  m_manager.Dump(m_archivePedestrian, routing::RouterType::Pedestrian, dumpAnyway);
}
}  // namespace tracking
