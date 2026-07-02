#include <jni.h>
#include "app/organicmaps/sdk/Framework.hpp"
#include "app/organicmaps/sdk/core/jni_helper.hpp"
#include "routing/routing_options.hpp"

namespace
{
bool IsBicycleRouter()
{
  return frm()->GetRoutingManager().GetRouter() == routing::RouterType::Bicycle;
}

routing::RoutingOptions::Road makeValue(jint option)
{
  auto const road = static_cast<uint8_t>(1u << static_cast<int>(option));
  CHECK_LESS(road, static_cast<uint8_t>(routing::RoutingOptions::Road::Max), ());
  return static_cast<routing::RoutingOptions::Road>(road);
}

routing::RoutingOptions LoadRoutingOptions()
{
  return IsBicycleRouter() ? routing::RoutingOptions::LoadBicycleOptionsFromSettings()
                           : routing::RoutingOptions::LoadCarOptionsFromSettings();
}

void SaveRoutingOptions(routing::RoutingOptions const & routingOptions)
{
  if (IsBicycleRouter())
    routing::RoutingOptions::SaveBicycleOptionsToSettings(routingOptions);
  else
    routing::RoutingOptions::SaveCarOptionsToSettings(routingOptions);
}
}  // namespace

extern "C"
{
JNIEXPORT jboolean Java_app_organicmaps_sdk_routing_RoutingOptions_nativeHasOption(JNIEnv *, jclass, jint option)
{
  routing::RoutingOptions::Road road = makeValue(option);
  routing::RoutingOptions routingOptions = LoadRoutingOptions();
  return static_cast<jboolean>(routingOptions.Has(road));
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RoutingOptions_nativeAddOption(JNIEnv *, jclass, jint option)
{
  routing::RoutingOptions::Road road = makeValue(option);
  routing::RoutingOptions routingOptions = LoadRoutingOptions();
  routingOptions.Add(road);
  SaveRoutingOptions(routingOptions);
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RoutingOptions_nativeRemoveOption(JNIEnv *, jclass, jint option)
{
  routing::RoutingOptions::Road road = makeValue(option);
  routing::RoutingOptions routingOptions = LoadRoutingOptions();
  routingOptions.Remove(road);
  SaveRoutingOptions(routingOptions);
}

JNIEXPORT void Java_app_organicmaps_sdk_routing_RoutingOptions_nativeSetPublicBicycleEnabled(JNIEnv *, jclass,
                                                                                             jboolean enabled)
{
  routing::RoutingOptions::SetPublicBicycleEnabled(static_cast<bool>(enabled));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_routing_RoutingOptions_nativeIsPublicBicycleEnabled(JNIEnv *, jclass)
{
  return static_cast<jboolean>(routing::RoutingOptions::IsPublicBicycleEnabled());
}
}
