#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace nogps
{
/// Checks while GPS works that the gyroscope turns the car the same way as GPS does. A sensor box can be mounted
/// in any position, and a driver of its gyroscope with wrong axes turns the car left on right turns. It would be
/// seen only when GPS is lost, so the turns are compared beforehand.
class TurnSignChecker
{
public:
  enum class Result
  {
    Unknown,
    Ok,
    Reversed,
  };

  // A turn counts when GPS has turned the car at least this much, the gyroscope at least half of it.
  static double constexpr kMinTurnDeg = 30;
  // The turn ends when GPS goes straight again.
  static double constexpr kMaxStraightStepDeg = 3;
  // GPS headings with a longer gap between them don't make a turn.
  static int64_t constexpr kMaxGpsGapMs = 3000;
  static int constexpr kTurnsToDecide = 3;

  /// \param deltaDeg the clockwise rotation of the car by the gyroscope while it drives.
  void OnGyro(double deltaDeg);

  /// \param bearingDeg a reliable GPS heading of the driving car.
  /// \returns the result if it has changed by this heading.
  std::optional<Result> OnGpsBearing(double bearingDeg, int64_t timeMs);

  /// GPS has no reliable heading now, e.g. the car stands still or GPS is lost.
  void OnNoGpsBearing() { m_lastBearingDeg.reset(); }

  Result GetResult() const { return m_result; }
  int GetSameTurns() const { return m_sameTurns; }
  int GetOppositeTurns() const { return m_oppositeTurns; }

private:
  void Restart(double bearingDeg, int64_t timeMs);

  std::optional<double> m_lastBearingDeg;
  int64_t m_lastGpsMs = 0;
  double m_gpsTurnDeg = 0;
  double m_gyroTurnDeg = 0;
  int m_sameTurns = 0;
  int m_oppositeTurns = 0;
  Result m_result = Result::Unknown;
};

std::string DebugPrint(TurnSignChecker::Result result);
}  // namespace nogps
