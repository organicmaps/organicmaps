#pragma once

#include "platform/settings.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace nogps
{
/// What the navigation without GPS keeps between the starts of the app: the settings and what it has learned.
class Storage
{
public:
  // The keys are shared with the settings of the Android app made before the navigation moved to the core.
  static std::string_view constexpr kInertialEnabled = "NoGpsInertialEnabled";
  static std::string_view constexpr kElm327Address = "NoGpsElm327Address";
  static std::string_view constexpr kEsp32Source = "NoGpsEsp32Source";
  static std::string_view constexpr kShiftStepM = "NoGpsShiftStep";
  static std::string_view constexpr kShiftButtonsShown = "NoGpsShiftButtons";
  static std::string_view constexpr kSpeedScale = "NoGpsSpeedScale";
  static std::string_view constexpr kSpeedTable = "NoGpsSpeedTable";
  static std::string_view constexpr kRoughSpeedTable = "NoGpsRoughSpeedTable";
  static std::string_view constexpr kSpeedLag = "NoGpsSpeedLag";
  // How the accelerometer of the sensor box is fixed in the car: the identifier of the box and what is learned.
  static std::string_view constexpr kBoxAccel = "NoGpsBoxAccel";
  static std::string_view constexpr kWrongNetworkPoints = "NoGpsWrongNetworkPoints";
  static std::string_view constexpr kLastTrusted = "NoGpsLastTrusted";
  // When the ESP32 sensor box was powered on the last time, milliseconds since the Unix epoch: for a box that
  // doesn't count its power-ons.
  static std::string_view constexpr kBoxPowerOnTime = "NoGpsBoxPowerOnTime";
  // How many times the known boxes were powered on by their own count, "C47D:3,42FD:1".
  static std::string_view constexpr kBoxPowerOns = "NoGpsBoxPowerOns";
  // The box has been powered on since the user calibrated it.
  static std::string_view constexpr kBoxCalibrationAdvised = "NoGpsBoxCalibrationAdvised";

  virtual ~Storage() = default;

  virtual std::optional<std::string> GetString(std::string_view key) const = 0;
  virtual void SetString(std::string_view key, std::string const & value) = 0;

  template <class T>
  T Get(std::string_view key, T const & defaultValue) const
  {
    auto const s = GetString(key);
    T value;
    if (s && settings::FromString(*s, value))
      return value;
    return defaultValue;
  }

  template <class T>
  void Set(std::string_view key, T const & value)
  {
    SetString(key, settings::ToString(value));
  }

  // Out of the box the navigation is on and looks for the sensor box. Who has chosen the ELM327 adapter, when
  // the phone was the default source, keeps the phone.
  bool IsInertialEnabled() const { return Get<bool>(kInertialEnabled, true); }
  bool IsEsp32Source() const { return Get<bool>(kEsp32Source, Get<std::string>(kElm327Address, "").empty()); }
};

/// The storage in the settings of the app.
class SettingsStorage : public Storage
{
public:
  std::optional<std::string> GetString(std::string_view key) const override;
  void SetString(std::string_view key, std::string const & value) override;
};
}  // namespace nogps
