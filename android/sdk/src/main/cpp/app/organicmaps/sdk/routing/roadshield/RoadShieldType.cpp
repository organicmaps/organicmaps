#include "RoadShieldType.hpp"

namespace
{
std::string ToJavaRoadShieldTypeName(ftypes::RoadShieldType roadShieldType)
{
  switch (roadShieldType)
  {
    using enum ftypes::RoadShieldType;
  case Default: [[fallthrough]];
  case Hidden: [[fallthrough]];
  case Generic_White: return "GenericWhite";
  case Generic_Green: return "GenericGreen";
  case Generic_Blue: return "GenericBlue";
  case Generic_Red: return "GenericRed";
  case Generic_Orange: return "GenericOrange";
  case US_Interstate: return "USInterstate";
  case US_Highway: return "USHighway";
  case UK_Highway: return "UKHighway";
  default: UNREACHABLE();
  }
}
}  // namespace

jobject ToJavaRoadShieldType(JNIEnv * env, ftypes::RoadShieldType roadShieldType)
{
  static jclass const roadShieldTypeClass =
      jni::GetGlobalClassRef(env, "app/organicmaps/sdk/routing/roadshield/RoadShieldType");
  jfieldID const fieldID = env->GetStaticFieldID(roadShieldTypeClass, ToJavaRoadShieldTypeName(roadShieldType).c_str(),
                                                 "Lapp/organicmaps/sdk/routing/roadshield/RoadShieldType;");
  return env->GetStaticObjectField(roadShieldTypeClass, fieldID);
}
