#pragma once

#include "ugc/loader.hpp"
#include "ugc/storage.hpp"
#include "ugc/types.hpp"

#include "platform/safe_callback.hpp"

#include "base/worker_thread.hpp"

#include <functional>

class Index;
struct FeatureID;

namespace ugc
{
class Api
{
public:
  using UGCCallback = platform::SafeCallback<void(UGC const & ugc, UGCUpdate const & update)>;
  using UGCCallbackUnsafe = std::function<void(UGC const & ugc, UGCUpdate const & update)>;
  using UGCJsonToSendCallback = std::function<void(std::string && jsonStr)>;
  using OnResultCallback = platform::SafeCallback<void(Storage::SettingResult const result)>;

  explicit Api(Index const & index);

  void GetUGC(FeatureID const & id, UGCCallbackUnsafe const & callback);
  void SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc,
                    OnResultCallback const & callback = nullptr);
  void GetUGCToSend(UGCJsonToSendCallback const & callback);
  void SendingCompleted();
  void SaveUGCOnDisk();

private:
  void GetUGCImpl(FeatureID const & id, UGCCallbackUnsafe const & callback);
  Storage::SettingResult SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc);
  void GetUGCToSendImpl(UGCJsonToSendCallback const & callback);
  void SendingCompletedImpl();
  void SaveUGCOnDiskImpl();

  base::WorkerThread m_thread;
  Storage m_storage;
  Loader m_loader;
};
}  // namespace ugc
