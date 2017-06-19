#pragma once

#include "base/worker_thread.hpp"

#include <functional>

class Index;
struct FeatureID;

namespace ugc
{
class Api
{
public:
  // TODO (@y, @mgsergio): replace void() by void(UGC const &).
  using Callback = std::function<void()>;

  explicit Api(Index const & index);

  void GetStaticUGC(FeatureID const & id, Callback callback);
  void GetDynamicUGC(FeatureID const & id, Callback callback);

private:
  void GetStaticUGCImpl(FeatureID const & id, Callback callback);
  void GetDynamicUGCImpl(FeatureID const & id, Callback callback);

  Index const & m_index;
  base::WorkerThread m_thread;
};
}  // namespace ugc
