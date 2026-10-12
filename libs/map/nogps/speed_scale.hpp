#pragma once

namespace nogps
{
/// The speed from the car differs from the real one: the speedometer is rounded and usually optimistic, and
/// the road is not flat. The difference is estimated from the corrections the user applies to the calculated
/// position and is compensated. The speedometer error stays the same, so the estimation is kept between trips.
class SpeedScale
{
public:
  // The error is estimated on a long enough distance only, otherwise a single correction defines it.
  static double constexpr kMinDistanceM = 300;
  static double constexpr kMinScale = 0.85;
  static double constexpr kMaxScale = 1.2;
  // A correction is rough (the user taps a button by eye), so only a part of it is taken into account.
  static double constexpr kCorrectionWeight = 0.5;

  /// The calculated position has moved by the distance.
  void OnDistance(double distanceM) { m_distanceM += distanceM; }

  /// The user has moved the position along the road, because it lagged behind the car.
  /// \param correctionM the distance the position was moved by, negative if it was moved back.
  void OnCorrection(double correctionM);

  /// \returns the ratio the speed from the car is multiplied by.
  double Get() const { return m_scale; }

  /// Restores the ratio estimated before, e.g. on a previous trip.
  void Set(double scale);

  void Reset();

private:
  double m_scale = 1;
  double m_distanceM = 0;
  double m_correctionM = 0;
};
}  // namespace nogps
