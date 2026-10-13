#include "sailfish/file_log.hpp"

#include "base/logging.hpp"
#include "base/src_point.hpp"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <atomic>
#include <cstdio>
#include <fstream>
#include <mutex>

namespace sailfish::file_log
{
namespace
{
// A full file becomes the ".1" one, replacing the older lines there.
std::streamoff constexpr kMaxSize = 5 * 1024 * 1024;

std::mutex g_mutex;
std::ofstream g_file;
std::string g_path;
// The handlers stay installed while enabled, also when a rotated file can't be reopened.
bool g_enabled = false;
// Set before the handler is installed, which other threads may call right away.
std::atomic<QtMessageHandler> g_qtHandler = nullptr;

void Write(char const * level, std::string const & where, std::string const & message)
{
  auto const time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzz")).toStdString();
  std::lock_guard lock(g_mutex);
  if (!g_file.is_open())
    return;
  // Flushed for the lines before a crash.
  g_file << time << ' ' << level << ' ' << where << ' ' << message << std::endl;
  if (g_file.tellp() < kMaxSize)
    return;
  g_file.close();
  std::remove((g_path + ".1").c_str());
  std::rename(g_path.c_str(), (g_path + ".1").c_str());
  g_file.open(g_path, std::ios::out | std::ios::trunc);
}

void LogCore(base::LogLevel level, base::SrcPoint const & srcPoint, std::string const & message)
{
  Write(base::ToString(level).c_str(), DebugPrint(srcPoint), message);
  base::LogMessageDefault(level, srcPoint, message);
}

void LogQt(QtMsgType type, QMessageLogContext const & context, QString const & message)
{
  char const * const levels[] = {"QDEBUG", "QWARNING", "QCRITICAL", "QFATAL", "QINFO"};
  auto const where = context.file ? std::string(context.file) + ':' + std::to_string(context.line) : std::string();
  Write(type >= 0 && type < static_cast<int>(std::size(levels)) ? levels[type] : "QT", where, message.toStdString());
  if (auto const handler = g_qtHandler.load())
    handler(type, context, message);
}
}  // namespace

// Not in the maps folder, which can move to a memory card with the open file.
QString Path()
{
  return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/logs/organicmaps.log");
}

void Enable(bool enabled)
{
  if (enabled == IsEnabled())
    return;
  if (enabled)
  {
    auto const path = Path();
    QDir().mkpath(QFileInfo(path).path());
    {
      std::lock_guard lock(g_mutex);
      g_path = path.toStdString();
      g_file.open(g_path, std::ios::out | std::ios::app);
      if (!g_file.is_open())
      {
        LOG(LWARNING, ("Can't open", g_path));
        return;
      }
      g_enabled = true;
    }
    base::SetLogMessageFn(&LogCore);
    base::g_LogLevel = base::LDEBUG;
    // The default handler, which is what qInstallMessageHandler() returns unless something else was installed.
    g_qtHandler = qInstallMessageHandler(nullptr);
    qInstallMessageHandler(&LogQt);
    LOG(LINFO, ("Logging to", path.toStdString()));
  }
  else
  {
    qInstallMessageHandler(g_qtHandler);
    base::SetLogMessageFn(&base::LogMessageDefault);
    base::g_LogLevel = base::GetDefaultLogLevel();
    std::lock_guard lock(g_mutex);
    g_file.close();
    g_enabled = false;
  }
}

bool IsEnabled()
{
  std::lock_guard lock(g_mutex);
  return g_enabled;
}
}  // namespace sailfish::file_log
