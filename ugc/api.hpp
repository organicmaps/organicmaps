#pragma once

#include "base/worker_thread.hpp"

#include "ugc/loader.hpp"
#include "ugc/storage.hpp"
#include "ugc/types.hpp"

#include <functional>

class Index;
struct FeatureID;

namespace ugc
{
class Api
{
public:
  using UGCCallback = std::function<void(UGC const & ugc, UGCUpdate const & update)>;
  using UGCJsonToSendCallback = std::function<void(std::string && jsonStr)>;

  explicit Api(Index const & index);

  void GetUGC(FeatureID const & id, UGCCallback callback);
  void SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc);
  void GetUGCForToSend(UGCJsonToSendCallback const & fn);
  void SendingCompleted();

private:
  void GetUGCImpl(FeatureID const & id, UGCCallback callback);
  void SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc);
  void GetUGCToSendImpl(UGCJsonToSendCallback const & fn);
  void SendingCompletedImpl();

  base::WorkerThread m_thread;
  Storage m_storage;
  Loader m_loader;
};
}  // namespace ugc
