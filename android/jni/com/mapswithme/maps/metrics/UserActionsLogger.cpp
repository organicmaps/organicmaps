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
  ::Framework * frm = g_framework->NativeFramework();
  if (!frm->HasPlacePageInfo())
    return;

  auto & info = g_framework->GetPlacePageInfo();
  auto const userPos = g_framework->NativeFramework()->GetCurrentPosition();

  utils::RegisterEyeEventIfPossible(type, userPos, info);
}

void RegisterTransitionToBooking()
{
  ::Framework * frm = g_framework->NativeFramework();
  if (!frm->HasPlacePageInfo())
    return;

  auto & info = g_framework->GetPlacePageInfo();
  eye::Eye::Event::TransitionToBooking(info.GetMercator());
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

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeBookingBookClicked(JNIEnv *, jclass)
{
  RegisterTransitionToBooking();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeBookingMoreClicked(JNIEnv *, jclass)
{
  RegisterTransitionToBooking();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeBookingReviewsClicked(JNIEnv *, jclass)
{
  RegisterTransitionToBooking();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeBookingDetailsClicked(JNIEnv *, jclass)
{
  RegisterTransitionToBooking();
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativePromoAfterBookingShown(JNIEnv * env,
                                                                                jclass, jstring id)
{
  eye::Eye::Event::PromoAfterBookingShown(jni::ToNativeString(env, id));
}
}
