#include "../Framework.hpp"

#include "platform/settings.hpp"


extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_settings_UnitLocale_setCurrentUnits(JNIEnv * env, jobject thiz, jint units)
  {
    Settings::Units const u = static_cast<Settings::Units>(units);
    Settings::Set("Units", u);
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_settings_UnitLocale_getCurrentUnits(JNIEnv * env, jobject thiz)
  {
    Settings::Units u = Settings::Metric;
    return (Settings::Get("Units", u) ? u : -1);
  }
}
