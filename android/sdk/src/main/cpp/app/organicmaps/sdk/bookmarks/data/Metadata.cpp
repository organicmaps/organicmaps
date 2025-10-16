#include "Metadata.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

void InjectMetadata(JNIEnv * env, jclass const clazz, jobject const mapObject, osm::MapObject const & src)
{
  static jmethodID const addId = env->GetMethodID(clazz, "addMetadata", "(ILjava/lang/String;)V");
  ASSERT(addId, ());

  src.ForEachMetadataReadable([env, &mapObject](osm::MapObject::MetadataID id, std::string const & meta)
  {
    /// @todo Make separate processing of non-string values like FMD_DESCRIPTION.
    /// Actually, better to call separate getters instead of ToString processing.
    if (!meta.empty())
    {
      jni::TScopedLocalRef metaString(env, jni::ToJavaString(env, meta));
      env->CallVoidMethod(mapObject, addId, static_cast<jint>(id), metaString.get());
    }
  });
}
