#include "ugc/api.hpp"

#include "base/assert.hpp"

#include <utility>

using namespace std;
using namespace ugc;

namespace ugc
{
Api::Api(Index const & index) : m_storage(index), m_loader(index)
{
  m_thread.Push([this] { m_storage.Load(); });
}

void Api::GetUGC(FeatureID const & id, UGCCallbackUnsafe const & callback)
{
  m_thread.Push([=] { GetUGCImpl(id, callback); });
}

void Api::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc,
                       OnResultCallback const & callback /* nullptr */)
{
  m_thread.Push([=] {
    auto const res = SetUGCUpdateImpl(id, ugc);
    if (callback)
      callback(res);
  });
}

void Api::GetUGCToSend(UGCJsonToSendCallback const & callback)
{
  m_thread.Push([callback, this] { GetUGCToSendImpl(callback); });
}

void Api::SendingCompleted()
{
  m_thread.Push([this] { SendingCompletedImpl(); });
}

void Api::SaveUGCOnDisk()
{
  m_thread.Push([this] { SaveUGCOnDiskImpl(); });
}

Loader & Api::GetLoader()
{
  return m_loader;
}

void Api::GetUGCImpl(FeatureID const & id, UGCCallbackUnsafe const & callback)
{
  CHECK(callback, ());
  if (!id.IsValid())
  {
    callback({}, {});
    return;
  }

  auto const update = m_storage.GetUGCUpdate(id);
  auto const ugc = m_loader.GetUGC(id);

  callback(ugc, update);
}

Storage::SettingResult Api::SetUGCUpdateImpl(FeatureID const & id, UGCUpdate const & ugc)
{
  return m_storage.SetUGCUpdate(id, ugc);
}

void Api::GetUGCToSendImpl(UGCJsonToSendCallback const & callback)
{
  CHECK(callback, ());
  auto json = m_storage.GetUGCToSend();
  callback(move(json), m_storage.GetNumberOfUnsynchronized());
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
