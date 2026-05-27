#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

#import <Network/Network.h>

#include <atomic>

namespace
{
Platform::EConnectionType ConnectionTypeFromPath(nw_path_t path)
{
  if (nw_path_get_status(path) != nw_path_status_satisfied)
    return Platform::EConnectionType::CONNECTION_NONE;

#if defined(OMIM_OS_IPHONE)
  if (nw_path_uses_interface_type(path, nw_interface_type_cellular))
    return Platform::EConnectionType::CONNECTION_WWAN;
#endif

  return Platform::EConnectionType::CONNECTION_WIFI;  // Wired Ethernet is bucketed here.
}

class PathMonitor
{
public:
  static PathMonitor & Instance()
  {
    // Process-lifetime singleton; intentionally never destroyed to avoid racing
    // the asynchronous nw_path_monitor update handler at static-destruction time.
    static auto * instance = new PathMonitor();
    return *instance;
  }

  Platform::EConnectionType Status() const { return m_status.load(std::memory_order_relaxed); }

private:
  PathMonitor() : m_monitor(nw_path_monitor_create())
  {
    nw_path_monitor_set_queue(m_monitor, dispatch_queue_create("app.organicmaps.netmon", DISPATCH_QUEUE_SERIAL));
    nw_path_monitor_set_update_handler(m_monitor, ^(nw_path_t path) {
      auto const status = ConnectionTypeFromPath(path);
      auto const oldStatus = m_status.exchange(status, std::memory_order_relaxed);
      if (oldStatus != status)
        LOG(LINFO, ("Connection status changed to", status));
    });
    nw_path_monitor_start(m_monitor);
  }

  nw_path_monitor_t m_monitor;
  std::atomic<Platform::EConnectionType> m_status{Platform::EConnectionType::CONNECTION_NONE};
};
}  // namespace

// nw_path_monitor delivers updates asynchronously, so this returns CONNECTION_NONE
// until the first callback arrives. Platform::Platform() warms the singleton at
// launch; new callers invoked during/immediately after Platform construction must
// tolerate a transient CONNECTION_NONE.
Platform::EConnectionType Platform::ConnectionStatus()
{
  return PathMonitor::Instance().Status();
}
