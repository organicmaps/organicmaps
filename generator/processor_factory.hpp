#pragma once

#include "generator/factory_utils.hpp"
#include "generator/processor_booking.hpp"
#include "generator/processor_coastline.hpp"
// #include "generator/processor_complex.hpp"
#include "generator/processor_country.hpp"
#include "generator/processor_interface.hpp"
#include "generator/processor_noop.hpp"
// #include "generator/processor_simple.hpp"
#include "generator/processor_world.hpp"

#include "base/assert.hpp"

#include <memory>
#include <utility>

namespace generator
{
enum class ProcessorType
{
  // Simple,
  Country,
  Coastline,
  World,
  Noop,
  // Complex,
  // Booking,
};

template <class... Args>
std::shared_ptr<FeatureProcessorInterface> CreateProcessor(ProcessorType type, Args &&... args)
{
  switch (type)
  {
  case ProcessorType::Coastline: return create<ProcessorCoastline>(std::forward<Args>(args)...);
  case ProcessorType::Country: return create<ProcessorCountry>(std::forward<Args>(args)...);
  // case ProcessorType::Simple: return create<ProcessorSimple>(std::forward<Args>(args)...);
  case ProcessorType::World: return create<ProcessorWorld>(std::forward<Args>(args)...);
  case ProcessorType::Noop:
    return create<ProcessorNoop>(std::forward<Args>(args)...);
    // case ProcessorType::Complex: return create<ProcessorComplex>(std::forward<Args>(args)...);
  }
  UNREACHABLE();
}
}  // namespace generator
