#pragma once

#include "generator/collector_interface.hpp"

#include "coding/file_writer.hpp"

#include <memory>
#include <string>

namespace generator
{
// The class CollectorAddresses is responsible for the collecting addresses to the file.
class CollectorAddresses : public CollectorInterface
{
public:
  CollectorAddresses(std::string const & filename);

  // CollectorInterface overrides:
  void CollectFeature(FeatureBuilder1 const & feature, OsmElement const &) override;
  void Save() override {}

private:
  std::unique_ptr<FileWriter> m_addrWriter;
};
}  // namespace generator
