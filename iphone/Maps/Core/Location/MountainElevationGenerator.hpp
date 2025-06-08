#pragma once

#include <ctime>
#include <random>

class MountainElevationGenerator
{
  static double constexpr kRandom{-1.};

  std::mt19937_64 rng;

  double const minElevation;
  double const maxElevation;
  double const maxSlopeChange;

  std::normal_distribution<double> slopeChangeDist;

  double currentElevation;
  double currentSlope;

  double ValueOrRandomInRange(double value, double min, double max)
  {
    if (value != kRandom)
      return value;

    return std::uniform_int_distribution<>(min, max)(rng);
  }

public:
  MountainElevationGenerator(double minElevation = kRandom, double maxElevation = kRandom,
                             double startElevation = kRandom, double maxSlopeChange = kRandom,
                             time_t seed = std::time(nullptr))
    : rng(seed)
    , minElevation(ValueOrRandomInRange(minElevation, 0., 2000.))
    , maxElevation(ValueOrRandomInRange(maxElevation, 3000., 7000.))
    , maxSlopeChange(ValueOrRandomInRange(maxSlopeChange, 1., 5.))
    , slopeChangeDist(0.0, maxSlopeChange)
    , currentElevation(ValueOrRandomInRange(startElevation, minElevation, maxElevation))
    , currentSlope(0.0)
  {}

  double NextElevation()
  {
    // Change the slope gradually
    currentSlope += slopeChangeDist(rng);

    // Limit maximum steepness
    currentSlope = std::max(-maxSlopeChange, std::min(maxSlopeChange, currentSlope));

    // Update elevation based on current slope
    currentElevation += currentSlope;

    // Ensure we stay within elevation bounds
    if (currentElevation < minElevation)
    {
      currentElevation = minElevation;
      currentSlope = std::abs(currentSlope) * 0.5;  // Bounce back up
    }
    if (currentElevation > maxElevation)
    {
      currentElevation = maxElevation;
      currentSlope = -std::abs(currentSlope) * 0.5;  // Start going down
    }

    return currentElevation;
  }
};
