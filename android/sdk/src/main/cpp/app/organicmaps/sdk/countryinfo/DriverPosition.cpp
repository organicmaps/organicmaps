#include "DriverPosition.hpp"

namespace countryinfo
{
jobject CreateDriverPosition(JNIEnv * env, std::string_view const & position)
{
  jclass driverPositionClass = env->FindClass("app/organicmaps/sdk/countryinfo/DriverPosition");
  // Left-hand traffic -> DriverPosition.Right
  std::string_view const driverPositionValue = position == "l" ? "RIGHT" : "LEFT";
  jfieldID enumField = env->GetStaticFieldID(driverPositionClass, driverPositionValue.data(),
                                             "Lapp/organicmaps/sdk/countryinfo/DriverPosition;");
  return env->GetStaticObjectField(driverPositionClass, enumField);
}
}  // namespace countryinfo
