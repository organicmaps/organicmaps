#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "coding/file_reader.hpp"
#include "coding/sha1.hpp"

#include <future>

extern "C"
{
JNIEXPORT jbyteArray JNICALL Java_app_organicmaps_sdk_util_FileUtils_nativeReadFile(JNIEnv * env, jclass clazz,
                                                                                    jstring jFilePath)
{
  std::string filePath = jni::ToNativeString(env, jFilePath);

  auto const promise = std::make_shared<std::promise<std::vector<uint8_t>>>();
  auto future = promise->get_future();
  GetPlatform().RunTask(Platform::Thread::File, [promise, filePath = std::move(filePath)]()
  {
    try
    {
      FileReader reader(filePath);
      std::vector<uint8_t> data(reader.Size());
      reader.Read(0, data.data(), data.size());
      promise->set_value(std::move(data));
    }
    catch (Reader::Exception const & e)
    {
      LOG(LWARNING, ("Failed to read file:", filePath, e.what()));
      promise->set_exception(std::current_exception());
    }
  });

  try
  {
    // Wait on the current thread until the file thread performs the task
    auto data = future.get();
    jbyteArray result = env->NewByteArray(static_cast<jsize>(data.size()));
    if (result != nullptr)
      env->SetByteArrayRegion(result, 0, static_cast<jsize>(data.size()), reinterpret_cast<jbyte const *>(data.data()));
    return result;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, (e.what()));
    return nullptr;
  }
}

JNIEXPORT jbyteArray JNICALL Java_app_organicmaps_sdk_util_FileUtils_nativeCalculateFileSha1(JNIEnv * env, jclass clazz,
                                                                                             jstring jFilePath)
{
  std::string filePath = jni::ToNativeString(env, jFilePath);

  auto const promise = std::make_shared<std::promise<coding::SHA1::Hash>>();
  auto future = promise->get_future();
  GetPlatform().RunTask(Platform::Thread::File, [promise, filePath = std::move(filePath)]()
  { promise->set_value(coding::SHA1::Calculate(filePath)); });
  coding::SHA1::Hash data;
  try
  {
    // Wait on the current thread until the file thread performs the task
    data = future.get();
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, (e.what()));
    data = {};
  }
  jbyteArray result = env->NewByteArray(static_cast<jsize>(data.size()));
  if (result != nullptr)
    env->SetByteArrayRegion(result, 0, static_cast<jsize>(data.size()), reinterpret_cast<jbyte const *>(data.data()));
  return result;
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_FileUtils_nativeDeleteFile(JNIEnv * env, jclass,
                                                                                    jstring jFilePath)
{
  std::string filePath = jni::ToNativeString(env, jFilePath);
  auto const promise = std::make_shared<std::promise<bool>>();
  auto future = promise->get_future();
  GetPlatform().RunTask(Platform::Thread::File, [promise, filePath = std::move(filePath)]()
  { promise->set_value(base::DeleteFileX(filePath)); });
  try
  {
    // Wait on the current thread until the file thread performs the task
    return static_cast<jboolean>(future.get());
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, (e.what()));
    return JNI_FALSE;
  }
}

JNIEXPORT jboolean JNICALL Java_app_organicmaps_sdk_util_FileUtils_nativeMoveFile(JNIEnv * env, jclass, jstring src,
                                                                                  jstring dest)
{
  std::string srcPath = jni::ToNativeString(env, src);
  std::string destPath = jni::ToNativeString(env, dest);
  auto const promise = std::make_shared<std::promise<bool>>();
  auto future = promise->get_future();
  GetPlatform().RunTask(Platform::Thread::File,
                        [promise, srcPath = std::move(srcPath), destPath = std::move(destPath)]()
  { promise->set_value(base::MoveFileX(srcPath, destPath)); });
  try
  {
    // Wait on the current thread until the file thread performs the task
    return static_cast<jboolean>(future.get());
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, (e.what()));
    return JNI_FALSE;
  }
}

JNIEXPORT jbyteArray JNICALL Java_app_organicmaps_sdk_util_FileUtils_nativeCalculateSha1(JNIEnv * env, jclass,
                                                                                         jbyteArray jBytes)
{
  jsize const bytesSize = env->GetArrayLength(jBytes);
  jbyte * bytesPtr = env->GetByteArrayElements(jBytes, nullptr);

  std::vector<uint8_t> bytes(reinterpret_cast<uint8_t *>(bytesPtr), reinterpret_cast<uint8_t *>(bytesPtr) + bytesSize);

  env->ReleaseByteArrayElements(jBytes, bytesPtr, JNI_ABORT);

  auto const hash = coding::SHA1::Calculate(bytes);

  jbyteArray result = env->NewByteArray(static_cast<jsize>(hash.size()));
  env->SetByteArrayRegion(result, 0, static_cast<jsize>(hash.size()), reinterpret_cast<jbyte const *>(hash.data()));

  return result;
}
}  // extern "C"
