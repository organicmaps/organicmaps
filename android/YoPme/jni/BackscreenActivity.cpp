#include <jni.h>

#include "../../jni/com/mapswithme/platform/Platform.hpp"

extern "C"
{

	JNIEXPORT void JNICALL
	Java_com_mapswithme_yopme_BackscreenActivity_nativeInitPlatform(JNIEnv * env, jobject thiz,
																	jstring apkPath, jstring storagePath,
																	jstring tmpPath, jstring obbGooglePath,
																	jboolean isPro, jboolean isYota)
	{
		android::Platform::Instance().Initialize(env, apkPath, storagePath, tmpPath, obbGooglePath, isPro, isYota);
	}

} // extern "C"
