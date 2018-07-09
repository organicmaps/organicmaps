#pragma once

#include "partners_api/taxi_places.hpp"
#include "partners_api/taxi_provider.hpp"

#include "coding/file_reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <string>

namespace taxi
{
namespace places
{
class Loader
{
public:
  static SupportedPlaces LoadFor(Provider::Type const type);

private:
  static std::string GetFileNameByProvider(Provider::Type const type);
};
}  // namespace places
}  // namespace taxi
