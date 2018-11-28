#pragma once

#include "generator/emitter_interface.hpp"
#include "generator/emitter_booking.hpp"
#include "generator/emitter_planet.hpp"
#include "generator/emitter_restaurants.hpp"
#include "generator/emitter_simple.hpp"
#include "generator/factory_utils.hpp"

#include "base/assert.hpp"

#include <memory>

namespace generator
{
enum class EmitterType
{
  Planet,
  Restaurants,
  Simple,
  //  Booking
};

template <class... Args>
std::shared_ptr<EmitterInterface> CreateEmitter(EmitterType type, Args&&... args)
{
  switch (type)
  {
  case EmitterType::Planet:
    return create<EmitterPlanet>(std::forward<Args>(args)...);
  case EmitterType::Simple:
    return create<EmitterSimple>(std::forward<Args>(args)...);
  case EmitterType::Restaurants:
    return create<EmitterRestaurants>(std::forward<Args>(args)...);
  }
  UNREACHABLE();
}
}  // namespace generator
