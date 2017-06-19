#pragma once

#include "base/worker_thread.hpp"

#include "ugc/types.hpp"

#include <functional>

class Index;
struct FeatureID;

namespace ugc
{
class Api
{
public:
  using UGCCallback = std::function<void(UGC const &)>;
  using UGCUpdateCallback = std::function<void(UGCUpdate const &)>;

  explicit Api(Index const & index);

  void GetUGC(FeatureID const & id, UGCCallback callback);
  void GetUGCUpdate(FeatureID const & id, UGCUpdateCallback callback);

private:
  void GetUGCImpl(FeatureID const & id, UGCCallback callback);
  void GetUGCUpdateImpl(FeatureID const & id, UGCUpdateCallback callback);

  Index const & m_index;
  base::WorkerThread m_thread;
};
}  // namespace ugc
