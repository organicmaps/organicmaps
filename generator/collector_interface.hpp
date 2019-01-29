#pragma once

#include <string>

struct OsmElement;
namespace base
{
class GeoObjectId;
}  // namespace base

namespace generator
{
class CollectorInterface
{
public:
  virtual ~CollectorInterface() = default;

  virtual void Collect(base::GeoObjectId const & osmId, OsmElement const & el) = 0;
  virtual void Save() = 0;
};
}  // namespace generator
