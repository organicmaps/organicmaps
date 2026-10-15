#include "app/organicmaps/sdk/Framework.hpp"

#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "map/nogps/delegate.hpp"
#include "map/nogps/engine.hpp"

#include <memory>
#include <string>

namespace
{
/// The navigation without GPS asks LocationHelper for the platform part.
class NoGpsJniDelegate : public nogps::Delegate
{
public:
  explicit NoGpsJniDelegate(jobject delegate) : m_delegate(jni::make_global_ref(delegate)) {}

  void StartMotionSensors(bool gyroscope) override
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*m_delegate, Method(env, "startMotionSensors", "(Z)V"), static_cast<jboolean>(gyroscope));
    jni::HandleJavaException(env);
  }

  void StopMotionSensors() override { CallVoid("stopMotionSensors"); }

  void Elm327Connect(std::string const & address) override
  {
    JNIEnv * env = jni::GetEnv();
    jni::TScopedLocalRef jAddress(env, jni::ToJavaString(env, address));
    env->CallVoidMethod(*m_delegate, Method(env, "elm327Connect", "(Ljava/lang/String;)V"), jAddress.get());
    jni::HandleJavaException(env);
  }

  void Elm327Write(std::string const & data) override { CallWithBytes("elm327Write", data); }

  void Elm327Close() override { CallVoid("elm327Close"); }

  void Esp32BleOpen() override { CallVoid("esp32BleOpen"); }

  void Esp32BleSend(std::string const & line) override { CallWithBytes("esp32BleSend", line); }

  void Esp32BleSendFirmware(std::string const & piece) override { CallWithBytes("esp32BleSendFirmware", piece); }

  void Esp32BleClose() override { CallVoid("esp32BleClose"); }

  void OnPosition(nogps::Fix const & fix) override
  {
    JNIEnv * env = jni::GetEnv();
    static jclass const noGpsClass = jni::GetGlobalClassRef(env, "app/organicmaps/sdk/location/NoGps");
    static jmethodID const createLocation =
        jni::GetStaticMethodID(env, noGpsClass, "createLocation", "(IDDFZFZFZDFJJ)Landroid/location/Location;");
    jni::TScopedLocalRef location(
        env,
        env->CallStaticObjectMethod(
            noGpsClass, createLocation, static_cast<jint>(fix.m_provider), fix.m_position.m_lat, fix.m_position.m_lon,
            static_cast<jfloat>(fix.m_accuracyM), static_cast<jboolean>(fix.m_bearingDeg.has_value()),
            static_cast<jfloat>(fix.m_bearingDeg.value_or(0)), static_cast<jboolean>(fix.m_speedMps.has_value()),
            static_cast<jfloat>(fix.m_speedMps.value_or(0)), static_cast<jboolean>(fix.m_altitudeM.has_value()),
            fix.m_altitudeM.value_or(0), static_cast<jfloat>(fix.m_altitudeAccuracyM.value_or(-1)),
            static_cast<jlong>(fix.m_timeMs), static_cast<jlong>(fix.m_unixTimeMs)));
    env->CallVoidMethod(*m_delegate, Method(env, "onPosition", "(Landroid/location/Location;)V"), location.get());
    jni::HandleJavaException(env);
  }

  void OnEvent(nogps::Event event) override
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*m_delegate, Method(env, "onEvent", "(I)V"), static_cast<jint>(event));
    jni::HandleJavaException(env);
  }

private:
  jmethodID Method(JNIEnv * env, char const * name, char const * signature) const
  {
    return jni::GetMethodID(env, *m_delegate, name, signature);
  }

  void CallVoid(char const * name)
  {
    JNIEnv * env = jni::GetEnv();
    env->CallVoidMethod(*m_delegate, Method(env, name, "()V"));
    jni::HandleJavaException(env);
  }

  void CallWithBytes(char const * name, std::string const & data)
  {
    JNIEnv * env = jni::GetEnv();
    jni::TScopedLocalByteArrayRef bytes(env, env->NewByteArray(static_cast<jsize>(data.size())));
    env->SetByteArrayRegion(bytes.get(), 0, static_cast<jsize>(data.size()),
                            reinterpret_cast<jbyte const *>(data.data()));
    env->CallVoidMethod(*m_delegate, Method(env, name, "([B)V"), bytes.get());
    jni::HandleJavaException(env);
  }

  std::shared_ptr<jobject> m_delegate;
};

std::unique_ptr<NoGpsJniDelegate> g_noGpsDelegate;

nogps::Engine & NoGpsEngine()
{
  auto * engine = g_framework->NativeFramework()->GetNoGps();
  CHECK(engine, ("NoGps.create() must be called first"));
  return *engine;
}

std::string NoGpsBytes(JNIEnv * env, jbyteArray data)
{
  std::string result(static_cast<size_t>(env->GetArrayLength(data)), '\0');
  env->GetByteArrayRegion(data, 0, static_cast<jsize>(result.size()), reinterpret_cast<jbyte *>(result.data()));
  return result;
}

void SetNoGpsField(JNIEnv * env, jobject obj, char const * name, int value)
{
  env->SetIntField(obj, env->GetFieldID(env->GetObjectClass(obj), name, "I"), static_cast<jint>(value));
}

void SetNoGpsField(JNIEnv * env, jobject obj, char const * name, bool value)
{
  env->SetBooleanField(obj, env->GetFieldID(env->GetObjectClass(obj), name, "Z"), static_cast<jboolean>(value));
}

void SetNoGpsFloat(JNIEnv * env, jobject obj, char const * name, double value)
{
  env->SetFloatField(obj, env->GetFieldID(env->GetObjectClass(obj), name, "F"), static_cast<jfloat>(value));
}

void SetNoGpsDouble(JNIEnv * env, jobject obj, char const * name, double value)
{
  env->SetDoubleField(obj, env->GetFieldID(env->GetObjectClass(obj), name, "D"), static_cast<jdouble>(value));
}

void SetNoGpsLong(JNIEnv * env, jobject obj, char const * name, int64_t value)
{
  env->SetLongField(obj, env->GetFieldID(env->GetObjectClass(obj), name, "J"), static_cast<jlong>(value));
}

void SetNoGpsString(JNIEnv * env, jobject obj, char const * name, std::string const & value)
{
  jni::TScopedLocalRef str(env, jni::ToJavaString(env, value));
  env->SetObjectField(obj, env->GetFieldID(env->GetObjectClass(obj), name, "Ljava/lang/String;"), str.get());
}
}  // namespace

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeCreate(JNIEnv *, jclass, jobject delegate)
{
  CHECK(!g_noGpsDelegate, ());
  g_noGpsDelegate = std::make_unique<NoGpsJniDelegate>(delegate);
  g_framework->NativeFramework()->CreateNoGps(*g_noGpsDelegate);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeStart(JNIEnv *, jclass)
{
  NoGpsEngine().Start();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeStop(JNIEnv *, jclass)
{
  NoGpsEngine().Stop();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnFix(
    JNIEnv *, jclass, jint provider, jdouble lat, jdouble lon, jfloat accuracy, jboolean hasBearing, jfloat bearing,
    jboolean hasBearingAccuracy, jfloat bearingAccuracy, jboolean hasSpeed, jfloat speed, jboolean hasAltitude,
    jdouble altitude, jfloat altitudeAccuracy, jlong elapsedMs, jlong unixMs, jboolean lastKnownNetwork)
{
  nogps::Fix fix;
  fix.m_provider = static_cast<nogps::Provider>(provider);
  fix.m_position = {lat, lon};
  fix.m_accuracyM = accuracy;
  if (hasBearing)
    fix.m_bearingDeg = bearing;
  if (hasBearingAccuracy)
    fix.m_bearingAccuracyDeg = bearingAccuracy;
  if (hasSpeed)
    fix.m_speedMps = speed;
  if (hasAltitude)
    fix.m_altitudeM = altitude;
  if (altitudeAccuracy >= 0)
    fix.m_altitudeAccuracyM = altitudeAccuracy;
  fix.m_timeMs = elapsedMs;
  fix.m_unixTimeMs = unixMs;
  if (lastKnownNetwork)
    NoGpsEngine().OnLastKnownNetworkFix(fix);
  else
    NoGpsEngine().OnFix(fix);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnGyro(JNIEnv *, jclass, jlong timestampNs, jfloat x,
                                                                    jfloat y, jfloat z)
{
  NoGpsEngine().OnGyro(timestampNs, x, y, z);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnAccel(JNIEnv *, jclass, jlong timestampNs, jfloat x,
                                                                     jfloat y, jfloat z)
{
  NoGpsEngine().OnAccel(timestampNs, x, y, z);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnElm327Connected(JNIEnv *, jclass)
{
  NoGpsEngine().OnElm327Connected();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnElm327Bytes(JNIEnv * env, jclass, jbyteArray data)
{
  NoGpsEngine().OnElm327Bytes(NoGpsBytes(env, data));
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnElm327Closed(JNIEnv * env, jclass, jstring reason)
{
  NoGpsEngine().OnElm327Closed(jni::ToNativeString(env, reason));
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnEsp32BleBytes(JNIEnv * env, jclass, jbyteArray data)
{
  NoGpsEngine().OnEsp32BleBytes(NoGpsBytes(env, data));
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeOnEsp32BleState(JNIEnv *, jclass, jint state)
{
  NoGpsEngine().OnEsp32BleState(static_cast<nogps::BleState>(state));
}

JNIEXPORT jboolean Java_app_organicmaps_sdk_location_NoGps_nativeIsManualMode(JNIEnv *, jclass)
{
  return static_cast<jboolean>(NoGpsEngine().IsManualMode());
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeSetManualMode(JNIEnv *, jclass, jboolean enabled)
{
  NoGpsEngine().SetManualMode(enabled);
}

JNIEXPORT jdouble Java_app_organicmaps_sdk_location_NoGps_nativeShiftPosition(JNIEnv *, jclass, jdouble distanceM)
{
  return NoGpsEngine().ShiftPosition(distanceM);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeReverseDirection(JNIEnv *, jclass)
{
  NoGpsEngine().ReverseDirection();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeSetGpsDisabled(JNIEnv *, jclass, jboolean disabled)
{
  NoGpsEngine().SetGpsDisabled(disabled);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeSetInertialNavigationEnabled(JNIEnv *, jclass,
                                                                                          jboolean enabled)
{
  NoGpsEngine().SetInertialNavigationEnabled(enabled);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeSetElm327Address(JNIEnv * env, jclass, jstring address)
{
  NoGpsEngine().SetElm327Address(jni::ToNativeString(env, address));
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeSetEsp32Source(JNIEnv *, jclass, jboolean esp32)
{
  NoGpsEngine().SetEsp32Source(esp32);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeCalibrate(JNIEnv *, jclass)
{
  NoGpsEngine().Calibrate();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeStartFirmwareUpdate(JNIEnv *, jclass)
{
  NoGpsEngine().StartFirmwareUpdate();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeCancelFirmwareUpdate(JNIEnv *, jclass)
{
  NoGpsEngine().CancelFirmwareUpdate();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeClearSpeedCalibration(JNIEnv *, jclass)
{
  NoGpsEngine().ClearSpeedCalibration();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeCycleShiftStep(JNIEnv *, jclass)
{
  NoGpsEngine().CycleShiftStep();
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeSetShiftButtonsShown(JNIEnv *, jclass, jboolean shown)
{
  NoGpsEngine().SetShiftButtonsShown(shown);
}

JNIEXPORT void Java_app_organicmaps_sdk_location_NoGps_nativeGetStatus(JNIEnv * env, jclass, jobject out)
{
  auto const status = NoGpsEngine().GetStatus();
  SetNoGpsField(env, out, "source", static_cast<int>(status.m_source));
  SetNoGpsFloat(env, out, "accuracyM", status.m_accuracyM);
  SetNoGpsField(env, out, "manualMode", status.m_manualMode);
  SetNoGpsLong(env, out, "manualAgeMs", status.m_manualAgeMs);
  SetNoGpsField(env, out, "gpsSpoofed", status.m_gpsSpoofed);
  SetNoGpsField(env, out, "gpsDisabled", status.m_gpsDisabled);
  SetNoGpsFloat(env, out, "workingGpsAccuracyM", status.m_workingGpsAccuracyM.value_or(-1));
  SetNoGpsFloat(env, out, "workingNetworkAccuracyM", status.m_workingNetworkAccuracyM.value_or(-1));
  SetNoGpsField(env, out, "movedByHand", status.m_movedByHand);
  SetNoGpsField(env, out, "shiftForwardBlocked", status.m_shiftForwardBlocked);
  SetNoGpsField(env, out, "shiftBackBlocked", status.m_shiftBackBlocked);
  SetNoGpsField(env, out, "shiftStepM", status.m_shiftStepM);
  SetNoGpsField(env, out, "shiftButtonsShown", status.m_shiftButtonsShown);
  SetNoGpsField(env, out, "inertialEnabled", status.m_inertialEnabled);
  SetNoGpsField(env, out, "esp32Source", status.m_esp32Source);
  SetNoGpsString(env, out, "elm327Address", status.m_elm327Address);
  SetNoGpsField(env, out, "inertialStarted", status.m_inertialStarted);
  SetNoGpsField(env, out, "sourceState", static_cast<int>(status.m_sourceState));
  SetNoGpsField(env, out, "bleState", static_cast<int>(status.m_bleState));
  SetNoGpsString(env, out, "deviceName", status.m_deviceName);
  SetNoGpsField(env, out, "speedKmh", status.m_speedKmh);
  SetNoGpsField(env, out, "hasCarInfo", status.m_carInfo.has_value());
  if (auto const & car = status.m_carInfo)
  {
    SetNoGpsField(env, out, "engineRunning", car->m_engineRunning ? static_cast<int>(*car->m_engineRunning) : -1);
    SetNoGpsField(env, out, "rpm", car->m_rpm);
    SetNoGpsField(env, out, "boxMillivolts", car->m_boxMillivolts);
    SetNoGpsField(env, out, "elmMillivolts", car->m_elmMillivolts);
    SetNoGpsField(env, out, "ecuMillivolts", car->m_ecuMillivolts);
    SetNoGpsField(env, out, "voltageMismatch", car->m_voltageMismatch);
  }
  SetNoGpsDouble(env, out, "speedScale", status.m_speedScale);
  SetNoGpsField(env, out, "speedTableRanges", status.m_speedTableRanges);
  SetNoGpsDouble(env, out, "speedLagSec", status.m_speedLagSec);
  SetNoGpsField(env, out, "speedLagMeasured", status.m_speedLagMeasured);
  SetNoGpsField(env, out, "calibration", static_cast<int>(status.m_calibration));
  SetNoGpsField(env, out, "calibrationProgress", status.m_calibrationProgress);
  SetNoGpsField(env, out, "hasInertialPosition", status.m_hasInertialPosition);
  SetNoGpsField(env, out, "roadLost", status.m_roadLost);
  SetNoGpsString(env, out, "boxFirmware", status.m_boxFirmware);
  SetNoGpsString(env, out, "bundledFirmware", status.m_bundledFirmware);
  SetNoGpsField(env, out, "firmwareUpdate", static_cast<int>(status.m_firmwareUpdate));
  SetNoGpsField(env, out, "firmwareUpdateProgress", status.m_firmwareUpdateProgress);
  SetNoGpsString(env, out, "firmwareUpdateError", status.m_firmwareUpdateError);
  SetNoGpsField(env, out, "boxCalibrationAdvised", status.m_boxCalibrationAdvised);
}
}  // extern "C"
