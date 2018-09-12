//
// Created by Dmitry Donskoy on 08.09.2018.
//
#include <jni.h>
#include <android/jni/com/mapswithme/maps/Framework.hpp>
#include "com/mapswithme/core/jni_helper.hpp"
#include "com/mapswithme/platform/Platform.hpp"
#include "metrics/eye.hpp"
#include "metrics/eye_info.hpp"

extern "C"
{
JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeTipShown(JNIEnv * env, jclass, jint type, jint event)
{
  auto const & typeValue = static_cast<eye::Tip::Type>(type);
  auto const & eventValue = static_cast<eye::Tip::Event>(event);
  eye::Eye::Event::TipShown(typeValue, eventValue);
}

JNIEXPORT void JNICALL
Java_com_mapswithme_maps_metrics_UserActionsLogger_BookingFilterUsed(JNIEnv * env, jclass clazz)
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
Java_com_mapswithme_maps_metrics_UserActionsLogger_nativeLayerUsed(JNIEnv * env, jclass clazz,
                                                                   jint layerType)
{
  auto const & layer = static_cast<eye::Layer::Type>(layerType);
  eye::Eye::Event::LayerUsed(layer);
}
}
