#pragma once

#include "base/worker_thread.hpp"

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
  using UGCCallback = std::function<void(UGC const &)>;
  using UGCUpdateCallback = std::function<void(UGCUpdate const &)>;

  explicit Api(Index const & index, std::string const & filename);

  void GetUGC(FeatureID const & id, UGCCallback callback);
  void GetUGCUpdate(FeatureID const & id, UGCUpdateCallback callback);

  void SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc);

  static UGC MakeTestUGC1(Time now = Clock::now());
  static UGC MakeTestUGC2(Time now = Clock::now());

private:
  void GetUGCImpl(FeatureID const & id, UGCCallback callback);
  void GetUGCUpdateImpl(FeatureID const & id, UGCUpdateCallback callback);

  void SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc);

  Index const & m_index;
  base::WorkerThread m_thread;
  Storage m_storage;
};
}  // namespace ugc
