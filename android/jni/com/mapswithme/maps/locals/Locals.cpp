#include "android/jni/com/mapswithme/maps/Framework.hpp"

#include "android/jni/com/mapswithme/core/jni_helper.hpp"
#include "partners_api/locals_api.hpp"

namespace
{
jclass g_localExpertClass;
jmethodID g_localExpertConstructor;

void PrepareClassRefs(JNIEnv * env)
{
  if (g_localExpertClass)
    return;

@NonNull int id,
@NotNull String name,
@NotNull String country,
@NotNull String city,
double rating, int reviewCount, @NonNull double price
 @NonNull String currency, String motto, String about,
                     @NonNull String offer, @NonNull String pageUrl, @NonNull String photoUrl

  g_localExpertClass = jni::GetGlobalClassRef(env, "com/mapswithme/maps/locals/LocalExpert");
  g_localExpertConstructor =
      jni::GetConstructorID(env, g_viatorProductClass,
                            "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;DID"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;"
                            "Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
}

}  // namespace
