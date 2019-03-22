#pragma once

#include "generator/collector_interface.hpp"

#include <fstream>
#include <functional>
#include <string>

struct OsmElement;
namespace base
{
class GeoObjectId;
}  // namespace base

namespace generator
{
// CollectorTag class collects validated value of a tag and saves it to file with following
// format: osmId<tab>tagValue.
class CollectorTag : public CollectorInterface
{
public:
  using Validator = std::function<bool(std::string const & tagValue)>;

  explicit CollectorTag(std::string const & filename, std::string const & tagKey,
                        Validator const & validator, bool ignoreIfNotOpen = false);

  // CollectorInterface overrides:
  void Collect(OsmElement const & el) override;
  void Save() override {}

private:
  std::ofstream m_stream;
  std::string m_tagKey;
  Validator m_validator;
  bool m_needCollect;
};
}  // namespace generator
