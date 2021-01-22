#pragma once

#include "generator/collector_interface.hpp"

#include <fstream>
#include <functional>
#include <memory>
#include <string>

struct OsmElement;
namespace base
{
class GeoObjectId;
}  // namespace base

namespace generator
{
namespace cache
{
class IntermediateDataReaderInterface;
}  // namespace cache

// CollectorTag class collects validated value of a tag and saves it to file with following
// format: osmId<tab>tagValue.
class CollectorTag : public CollectorInterface
{
public:
  using Validator = std::function<bool(std::string const & tagValue)>;

  explicit CollectorTag(std::string const & filename, std::string const & tagKey,
                        Validator const & validator);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<cache::IntermediateDataReaderInterface> const & = {}) const override;

  void Collect(OsmElement const & el) override;
  void Finish() override;

  void Merge(CollectorInterface const & collector) override;
  void MergeInto(CollectorTag & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::ofstream m_stream;
  std::string m_tagKey;
  Validator m_validator;
};
}  // namespace generator
