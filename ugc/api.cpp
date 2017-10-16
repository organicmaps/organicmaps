#include "ugc/api.hpp"

#include "platform/platform.hpp"

#include <utility>

using namespace std;
using namespace ugc;

namespace ugc
{
Api::Api(Index const & index) : m_storage(index), m_loader(index)
{
  m_thread.Push([this] { m_storage.Load(); });
}

void Api::GetUGC(FeatureID const & id, UGCCallback callback)
{
  m_thread.Push([=] { GetUGCImpl(id, callback); });
}

void Api::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc)
{
  m_thread.Push([=] { SetUGCUpdateImpl(id, ugc); });
}

void Api::GetUGCToSend(UGCJsonToSendCallback const & fn)
{
  m_thread.Push([fn, this] { GetUGCToSendImpl(fn); });
}

void Api::SendingCompleted()
{
  m_thread.Push([this] { SendingCompletedImpl(); });
}

void Api::SaveUGCOnDisk()
{
  m_thread.Push([this] { SaveUGCOnDiskImpl(); });
}

void Api::GetUGCImpl(FeatureID const & id, UGCCallback callback)
{
  if (!id.IsValid())
  {
    GetPlatform().RunOnGuiThread([callback] { callback({}, {}); });
    return;
  }

  auto const update = m_storage.GetUGCUpdate(id);
  auto const ugc = m_loader.GetUGC(id);

  GetPlatform().RunOnGuiThread([ugc, update, callback] { callback(ugc, update); });
}

void Api::SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc)
{
  m_storage.SetUGCUpdate(id, ugc);
}

void Api::GetUGCToSendImpl(UGCJsonToSendCallback const & fn)
{
  auto json = m_storage.GetUGCToSend();
  fn(move(json));
}

void Api::SendingCompletedImpl()
{
  m_storage.MarkAllAsSynchronized();
}

void Api::SaveUGCOnDiskImpl()
{
  m_storage.SaveIndex();
}
}  // namespace ugc
