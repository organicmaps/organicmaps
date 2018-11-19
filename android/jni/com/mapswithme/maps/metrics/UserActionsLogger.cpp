#include <jni.h>
#include <android/jni/com/mapswithme/maps/Framework.hpp>

#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"

#include "map/utils.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_info.hpp"

namespace
{
void RegisterEventIfPossible(eye::MapObject::Event::Type const type)
{
  auto & info = g_framework->GetPlacePageInfo();

  auto const userPos = g_framework->NativeFramework()->GetCurrentPosition();
  if (userPos)
  {
    auto const mapObject = utils::MakeEyeMapObject(info);
    if (!mapObject.IsEmpty())
      eye::Eye::Event::MapObjectEvent(mapObject, type, userPos.get());
  }
}
}  // namespace

extern "C"
{
JNIEXPORT void JNICALL Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeTipClicked(
    JNIEnv * env, jclass, jint type, jint event)
{
  auto const & typeValue = static_cast<eye::Tip::Type>(type);
  auto const & eventValue = static_cast<eye::Tip::Event>(event);
  eye::Eye::Event::TipClicked(typeValue, eventValue);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeBookingFilterUsed(JNIEnv * env, jclass clazz)
{
  eye::Eye::Event::BookingFilterUsed();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeDiscoveryShown(JNIEnv * env, jclass clazz)
{
  eye::Eye::Event::DiscoveryShown();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeBookmarksCatalogShown(JNIEnv * env,
                                                                               jclass clazz)
{
  eye::Eye::Event::BoomarksCatalogShown();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeDiscoveryItemClicked(JNIEnv * env,
                                                                              jclass clazz,
                                                                              jint eventType)
{
  auto const & event = static_cast<eye::Discovery::Event>(eventType);
  eye::Eye::Event::DiscoveryItemClicked(event);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeAddToBookmark(JNIEnv *, jclass)
{
  RegisterEventIfPossible(eye::MapObject::Event::Type::AddToBookmark);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeUgcEditorOpened(JNIEnv *, jclass)
{
  RegisterEventIfPossible(eye::MapObject::Event::Type::UgcEditorOpened);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeUgcSaved(JNIEnv *, jclass)
{
  RegisterEventIfPossible(eye::MapObject::Event::Type::UgcSaved);
}
}
