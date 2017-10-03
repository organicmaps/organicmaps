#include "ugc/api.hpp"

#include "platform/platform.hpp"

#include <chrono>

using namespace std;
using namespace ugc;

namespace ugc
{
Api::Api(Index const & index, std::string const & filename) : m_index(index), m_storage(filename) {}

void Api::GetUGC(FeatureID const & id, UGCCallback callback)
{
  m_thread.Push([=] { GetUGCImpl(id, callback); });
}

void Api::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc)
{
  m_thread.Push([=] { SetUGCUpdate(id, ugc); });
}

void Api::GetUGCImpl(FeatureID const & id, UGCCallback callback)
{
  // TODO (@y, @mgsergio): retrieve static UGC
  UGC ugc;
  UGCUpdate update;

  if (!id.IsValid())
  {
    GetPlatform().RunOnGuiThread([ugc, update, callback] { callback(ugc, update); });
    return;
  }

  // ugc = MakeTestUGC1();
  GetPlatform().RunOnGuiThread([ugc, update, callback] { callback(ugc, update); });
}

void Api::SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc)
{
  m_storage.SetUGCUpdate(id, ugc);
}
}  // namespace ugc
