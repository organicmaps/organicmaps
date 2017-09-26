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
  using UGCCallback = std::function<void(UGC const & ugc, UGCUpdate const & update)>;

  explicit Api(Index const & index, std::string const & filename);

  void GetUGC(FeatureID const & id, UGCCallback callback);

  void SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc);

  static UGC MakeTestUGC1(Time now = Clock::now());
  static UGC MakeTestUGC2(Time now = Clock::now());
  static UGCUpdate MakeTestUGCUpdate(Time now = Clock::now());

private:
  void GetUGCImpl(FeatureID const & id, UGCCallback callback);

  void SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc);

  Index const & m_index;
  base::WorkerThread m_thread;
  Storage m_storage;
};
}  // namespace ugc
