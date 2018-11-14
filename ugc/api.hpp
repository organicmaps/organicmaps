#pragma once

#include "ugc/loader.hpp"
#include "ugc/storage.hpp"
#include "ugc/types.hpp"

#include "platform/safe_callback.hpp"

#include "geometry/point2d.hpp"

#include "base/worker_thread.hpp"

#include <functional>

class DataSource;
struct FeatureID;

namespace feature
{
class TypesHolder;
}

namespace ugc
{
class Api
{
public:
  using UGCCallback = platform::SafeCallback<void(UGC const & ugc, UGCUpdate const & update)>;
  using UGCCallbackUnsafe = std::function<void(UGC const & ugc, UGCUpdate const & update)>;
  using UGCJsonToSendCallback = std::function<void(std::string && jsonStr, size_t numberOfUnsynchronized)>;
  using OnResultCallback = platform::SafeCallback<void(Storage::SettingResult const result)>;
  using NumberOfUnsynchronizedCallback = std::function<void(size_t number)>;
  using HasUGCForPlaceCallback = platform::SafeCallback<void(bool result)>;

  Api(DataSource const & dataSource, NumberOfUnsynchronizedCallback const & callback);

  void GetUGC(FeatureID const & id, UGCCallbackUnsafe const & callback);
  void SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc,
                    OnResultCallback const & callback = nullptr);
  void GetUGCToSend(UGCJsonToSendCallback const & callback);
  void HasUGCForPlace(uint32_t bestType, m2::PointD const & point,
                      HasUGCForPlaceCallback const & callback);
  void SendingCompleted();
  void SaveUGCOnDisk();

  Loader & GetLoader();

private:
  void GetUGCImpl(FeatureID const & id, UGCCallbackUnsafe const & callback);
  Storage::SettingResult SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc);
  void GetUGCToSendImpl(UGCJsonToSendCallback const & callback);
  void HasUGCForPlaceImpl(uint32_t bestType, m2::PointD const & point,
                          HasUGCForPlaceCallback const & callback) const;
  void SendingCompletedImpl();
  void SaveUGCOnDiskImpl();

  base::WorkerThread m_thread;
  Storage m_storage;
  Loader m_loader;
};
}  // namespace ugc
