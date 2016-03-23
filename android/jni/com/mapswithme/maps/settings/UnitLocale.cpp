#include "../Framework.hpp"

#include "platform/settings.hpp"


extern "C"
{
  JNIEXPORT void JNICALL
  Java_com_mapswithme_maps_settings_UnitLocale_setCurrentUnits(JNIEnv * env, jobject thiz, jint units)
  {
    settings::Units const u = static_cast<settings::Units>(units);
    settings::Set(settings::kMeasurementUnits, u);
    g_framework->SetupMeasurementSystem();
  }

  JNIEXPORT jint JNICALL
  Java_com_mapswithme_maps_settings_UnitLocale_getCurrentUnits(JNIEnv * env, jobject thiz)
  {
    settings::Units u;
    return (settings::Get(settings::kMeasurementUnits, u) ? u : -1);
  }
}
