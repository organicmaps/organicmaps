#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace nogps
{
/// How much the speed from the car differs from the real one at different speeds: the speedometer error grows
/// with the speed, and the speed is rounded to 1 km/h, which matters at low speeds. The real speed is taken from
/// trusted GPS while it works, the table is kept between trips and used without GPS.
/// The speeds are split into ranges, every range keeps the distance driven by the car speed and by GPS.
class SpeedTable
{
public:
  static int constexpr kRangeKmh = 10;
  static int constexpr kRanges = 14;
  // Slower speeds are rounded too roughly, the car maneuvers, and GPS speed is noisy.
  static int constexpr kMinSpeedKmh = 5;
  // A range is used after this distance only: a short one is dominated by the noise.
  static double constexpr kMinDistanceM = 300;
  // Older samples are forgotten gradually, e.g. after new tyres.
  static double constexpr kMaxDistanceM = 10'000;
  // GPS is compared with the car at a steady speed: the car reports its speed later than GPS.
  static double constexpr kMaxAccelerationMps2 = 1;
  // A longer pause between samples is a break.
  static double constexpr kMaxSampleDtSec = 2;
  static double constexpr kMinScale = 0.8;
  static double constexpr kMaxScale = 1.25;

  /// Takes into account the car speed and the real one measured at the same time.
  /// \param dtSec how long the speeds lasted.
  /// \param accelerationMps2 how fast the real speed changes.
  void OnSample(double carSpeedKmh, double gpsSpeedMps, double dtSec, double accelerationMps2);

  /// \returns the ratio the car speed is multiplied by, nothing if nothing is known yet. A speed not measured yet
  /// is interpolated from the closest measured ones.
  std::optional<double> GetScale(double carSpeedKmh) const;

  /// \returns how many speed ranges are measured.
  int GetKnownRanges() const;

  void Clear();

  /// \returns the table to keep between trips: the car and GPS distances of every range.
  std::string Serialize() const;
  /// Restores the table kept before, a broken one is ignored.
  void Deserialize(std::string_view table);

  /// \returns the measured ranges for the log: speed and ratio.
  std::string ToString() const;

private:
  bool IsKnown(int i) const { return m_carM[i] >= kMinDistanceM; }
  double ScaleOf(int i) const;

  std::array<double, kRanges> m_carM{};
  std::array<double, kRanges> m_gpsM{};
};
}  // namespace nogps
