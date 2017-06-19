#include "ugc/api.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/platform.hpp"

using namespace std;

namespace ugc
{
Api::Api(Index const & index) : m_index(index) {}

void Api::GetStaticUGC(FeatureID const & id, Callback callback)
{
  m_thread.Push([=]() { GetStaticUGCImpl(id, callback); });
}

void Api::GetDynamicUGC(FeatureID const & id, Callback callback)
{
  m_thread.Push([=]() { GetDynamicUGCImpl(id, callback); });
}

void Api::GetStaticUGCImpl(FeatureID const & /* id */, Callback callback)
{
  // TODO (@y, @mgsergio): retrieve static UGC
  GetPlatform().RunOnGuiThread(callback);
}

void Api::GetDynamicUGCImpl(FeatureID const & /* id */, Callback callback)
{
  // TODO (@y, @mgsergio): retrieve dynamic UGC
  GetPlatform().RunOnGuiThread(callback);
}
}  // namespace ugc
