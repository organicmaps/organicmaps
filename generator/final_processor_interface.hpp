#pragma once

#include <cstdint>

namespace generator
{
enum class FinalProcessorPriority : uint8_t
{
  CountriesOrWorld = 1,
  WorldCoasts,
  Places,
};

// Classes that inherit this interface implement the final stage of intermediate mwm processing.
// For example, attempt to merge the coastline or adding external elements.
// Each derived class has a priority. This is done to comply with the order of processing
// intermediate mwm, taking into account the dependencies between them. For example, before adding a
// coastline to a country, we must build coastline. Processors with higher priority will be called
// first. Processors with the same priority can run in parallel.
class FinalProcessorIntermediateMwmInterface
{
public:
  explicit FinalProcessorIntermediateMwmInterface(FinalProcessorPriority priority) : m_priority(priority) {}
  virtual ~FinalProcessorIntermediateMwmInterface() = default;

  virtual void Process() = 0;

  bool operator<(FinalProcessorIntermediateMwmInterface const & other) const { return m_priority < other.m_priority; }

protected:
  FinalProcessorPriority m_priority;
};

}  // namespace generator
