#include "map/nogps/speed_scale.hpp"

#include <algorithm>

namespace nogps
{
void SpeedScale::OnCorrection(double correctionM)
{
  m_correctionM += correctionM;
  if (m_distanceM < kMinDistanceM)
    return;

  double const corrected = m_scale * (1 + m_correctionM / m_distanceM * kCorrectionWeight);
  m_scale = std::clamp(corrected, kMinScale, kMaxScale);
  m_distanceM = 0;
  m_correctionM = 0;
}

void SpeedScale::Set(double scale)
{
  m_scale = std::clamp(scale, kMinScale, kMaxScale);
  m_distanceM = 0;
  m_correctionM = 0;
}

void SpeedScale::Reset()
{
  m_scale = 1;
  m_distanceM = 0;
  m_correctionM = 0;
}
}  // namespace nogps
