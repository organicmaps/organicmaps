#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_storage.hpp"

#include "platform/platform.hpp"

#include "coding/internal/file_data.hpp"

#include "base/macros.hpp"

#include <memory>

namespace eye
{
// static
void EyeForTesting::ResetEye()
{
  UNUSED_VALUE(GetPlatform().MkDirChecked(Storage::GetEyeDir()));

  SetInfo({});

  auto path = Storage::GetInfoFilePath();
  uint64_t unused;
  if (base::GetFileSize(path, unused))
    base::DeleteFileX(path);

  path = Storage::GetPoiEventsFilePath();
  if (base::GetFileSize(path, unused))
    base::DeleteFileX(path);
}

// static
void EyeForTesting::SetInfo(Info const & info)
{
  Eye::Instance().m_info.Set(std::make_shared<Info>(info));
}

// static
void EyeForTesting::AppendTip(Tip::Type type, Tip::Event event)
{
  Eye::Instance().RegisterTipClick(type, event);
}

// static
void EyeForTesting::UpdateBookingFilterUsedTime()
{
  Eye::Instance().UpdateBookingFilterUsedTime();
}

// static
void EyeForTesting::UpdateBoomarksCatalogShownTime()
{
  Eye::Instance().UpdateBoomarksCatalogShownTime();
}

// static
void EyeForTesting::UpdateDiscoveryShownTime()
{
  Eye::Instance().UpdateDiscoveryShownTime();
}

// static
void EyeForTesting::IncrementDiscoveryItem(Discovery::Event event)
{
  Eye::Instance().IncrementDiscoveryItem(event);
}

// static
void EyeForTesting::AppendLayer(Layer::Type type)
{
  Eye::Instance().RegisterLayerShown(type);
}

// static
void EyeForTesting::TrimExpiredMapObjectEvents()
{
  Eye::Instance().TrimExpiredMapObjectEvents();
}

// static
void EyeForTesting::RegisterMapObjectEvent(MapObject const & mapObject, MapObject::Event::Type type,
                                           m2::PointD const & userPos)
{
  Eye::Instance().RegisterMapObjectEvent(mapObject, type, userPos);
}
}  // namespace eye
