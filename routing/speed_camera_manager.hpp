#pragma once

#include "routing/route.hpp"
#include "routing/routing_callbacks.hpp"
#include "routing/routing_helpers.hpp"
#include "routing/speed_camera.hpp"
#include "routing/turns_notification_manager.hpp"

#include "platform/location.hpp"

#include "base/assert.hpp"
#include "base/thread_checker.hpp"

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
// Do not touch the order, it uses in platforms.
enum class SpeedCameraManagerMode
{
  Auto,
  Always,
  Never,

  MaxValue
};

class SpeedCameraManager
{
public:
  static std::string const kSpeedCamModeKey;

  SpeedCameraManager() = delete;
  explicit SpeedCameraManager(turns::sound::NotificationManager & notificationManager);
  ~SpeedCameraManager() { m_speedCamClearCallback(); }

  enum class Interval
  {
    // Influence zone of camera, set by |kInfluenceZoneMeters|.
    ImpactZone,
    // The zone, starting in the |kBeepSignalTime| seconds before ImpactZone and ending at the
    // beginning of ImpactZone.
    BeepSignalZone,
    // The zone where we could use voice notifications. It doesn't have the beginning
    // and end at the beginning of BeepSignalZone.
    VoiceNotificationZone
  };

  static Interval GetIntervalByDistToCam(double distanceToCameraMeters, double speedMpS);

  void SetRoute(std::weak_ptr<Route> route) { m_route = std::move(route); }

  void SetSpeedCamShowCallback(SpeedCameraShowCallback && callback)
  {
    m_speedCamShowCallback = std::move(callback);
  }

  void SetSpeedCamClearCallback(SpeedCameraClearCallback && callback)
  {
    m_speedCamClearCallback = std::move(callback);
  }

  bool Enable() const { return m_mode != SpeedCameraManagerMode::Never; }

  void OnLocationPositionChanged(location::GpsInfo const & info);

  void GenerateNotifications(std::vector<std::string> & notifications);
  bool ShouldPlayWarningSignal();

  void ResetNotifications();
  void Reset();

  void SetMode(SpeedCameraManagerMode mode)
  {
    m_mode = mode;
    settings::Set(kSpeedCamModeKey, static_cast<int>(mode));
  }

  SpeedCameraManagerMode GetMode() const { return m_mode; }

  SpeedCameraOnRoute const & GetClosestCamForTests() const { return m_closestCamera; }

private:
  // According to https://en.wikibooks.org/wiki/Physics_Study_Guide/Frictional_coefficients
  // average friction of rubber on asphalt is 0.68
  //
  // According to: https://en.wikipedia.org/wiki/Braking_distance
  // a =- \mu * g, where \mu - average friction of rubber on asphalt.
  static double constexpr kAverageFrictionOfRubberOnAspalt = 0.68;
  static double constexpr kGravitationalAcceleration = 9.80655;

  // Used for calculating distance needed to slow down before a speed camera.
  static double constexpr kAverageAccelerationOfBraking =
    -kAverageFrictionOfRubberOnAspalt * kGravitationalAcceleration; // Meters per second squared.

  static_assert(kAverageAccelerationOfBraking != 0, "");
  static_assert(kAverageAccelerationOfBraking < 0, "AverageAccelerationOfBraking must be negative");

  // We highlight camera in UI if the closest camera is placed in |kShowCameraDistanceM|
  // from us.
  static double constexpr kShowCameraDistanceM = 1000.0;

  // Additional time for user about make a decision about slow down.
  // Get from: https://en.wikipedia.org/wiki/Braking_distance
  static double constexpr kTimeForDecision = 2.0;

  // Distance that we use for look ahead to search camera on the route.
  static double constexpr kLookAheadDistanceMeters = 2000.0;

  static double constexpr kDistanceEpsilonMeters = 10.0;
  static double constexpr kDistToReduceSpeedBeforeUnknownCameraM = 50.0;
  static double constexpr kInfluenceZoneMeters = 450.0;  // Influence zone of speed camera.
  // With this constant we calculate the distance for beep signal.
  // If beep signal happend, it must be before |kBeepSignalTime| seconds
  // of entering to the camera influence zone - |kInfluenceZoneMeters|.
  static double constexpr kBeepSignalTime = 1.0;

  // Number of notifications for different types.
  static uint32_t constexpr kVoiceNotificationNumber = 1;
  static uint32_t constexpr kBeepSignalNumber = 1;

  void FindCamerasOnRouteAndCache(double passedDistanceMeters);

  void PassCameraToUI(SpeedCameraOnRoute const & camera)
  {
    // Clear previous speed cam in UI.
    m_speedCamClearCallback();

    m_currentHighlightedCamera = camera;
    m_speedCamShowCallback(camera.m_position);
  }

  bool SetNotificationFlags(double passedDistanceMeters, double speedMpS, SpeedCameraOnRoute const & camera);
  bool IsSpeedHigh(double distanceToCameraMeters, double speedMpS, SpeedCameraOnRoute const & camera) const;
  bool NeedUpdateClosestCamera(double distanceToCameraMeters, double speedMpS, SpeedCameraOnRoute const & camera);
  bool NeedChangeHighlightedCamera(double distToCameraMeters, bool needUpdateClosestCamera) const;
  bool IsHighlightedCameraExpired(double distToCameraMeters) const;

private:
  SpeedCameraOnRoute m_closestCamera;
  uint32_t m_beepSignalCounter;
  uint32_t m_voiceSignalCounter;

  // Flag of doing sound notification about camera on a way.
  bool m_makeBeepSignal;
  bool m_makeVoiceSignal;

  // Queue of speedCams, that we have found, but they are too far, to make warning about them.
  std::queue<SpeedCameraOnRoute> m_cachedSpeedCameras;

  // Info about camera, that is highlighted now.
  SpeedCameraOnRoute m_currentHighlightedCamera;

  size_t m_firstNotCheckedSpeedCameraIndex;
  std::weak_ptr<Route> m_route;
  turns::sound::NotificationManager & m_notificationManager;

  SpeedCameraShowCallback m_speedCamShowCallback = [](m2::PointD const & /* point */) {};
  SpeedCameraClearCallback m_speedCamClearCallback = []() {};

  SpeedCameraManagerMode m_mode;

  DECLARE_THREAD_CHECKER(m_threadChecker);
};
}  // namespace routing
