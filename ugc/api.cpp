#include "ugc/api.hpp"

#include "indexer/feature_data.hpp"

#include "base/assert.hpp"

#include <utility>

using namespace std;
using namespace ugc;

namespace ugc
{
Api::Api(DataSource const & dataSource, NumberOfUnsynchronizedCallback const & callback)
  : m_storage(dataSource), m_loader(dataSource)
{
  m_thread.Push([this, callback] {
    m_storage.Load();
    callback(m_storage.GetNumberOfUnsynchronized());
  });
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

void Api::HasUGCForPlace(uint32_t bestType, m2::PointD const & point,
                         HasUGCForPlaceCallback const & callback)
{
  m_thread.Push([=]() mutable
  {
    HasUGCForPlaceImpl(bestType, point, callback);
  });
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

void Api::HasUGCForPlaceImpl(uint32_t bestType, m2::PointD const & point,
                             HasUGCForPlaceCallback const & callback) const
{
  callback(m_storage.HasUGCForPlace(bestType, point));
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
