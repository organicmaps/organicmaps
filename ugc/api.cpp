#include "ugc/api.hpp"

#include "indexer/feature_decl.hpp"

#include "platform/platform.hpp"

using namespace std;

namespace ugc
{
Api::Api(Index const & index) : m_index(index) {}

void Api::GetUGC(FeatureID const & id, UGCCallback callback)
{
  m_thread.Push([=]() { GetUGCImpl(id, callback); });
}

void Api::GetUGCUpdate(FeatureID const & id, UGCUpdateCallback callback)
{
  m_thread.Push([=]() { GetUGCUpdateImpl(id, callback); });
}

void Api::GetUGCImpl(FeatureID const & /* id */, UGCCallback callback)
{
  // TODO (@y, @mgsergio): retrieve static UGC
  UGC ugc(Rating({}, {}), {}, {});
  GetPlatform().RunOnGuiThread([ugc, callback] { callback(ugc); });
}

void Api::GetUGCUpdateImpl(FeatureID const & /* id */, UGCUpdateCallback callback)
{
  // TODO (@y, @mgsergio): retrieve dynamic UGC
  UGCUpdate ugc(Rating({}, {}),
                Attribute({}, {}),
                ReviewAbuse({}, {}),
                ReviewFeedback({}, {}));
  GetPlatform().RunOnGuiThread([ugc, callback] { callback(ugc); });
}
}  // namespace ugc
