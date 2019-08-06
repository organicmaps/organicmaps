#pragma once

#include "generator/collector_interface.hpp"
#include "generator/intermediate_data.hpp"

#include <fstream>
#include <memory>
#include <string>

class RelationElement;

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache
}  // namespace generator

namespace routing
{
class RestrictionWriter : public generator::CollectorInterface
{
public:

  enum class ViaType
  {
    Node,
    Way,

    Count,
  };

  static std::string const kNodeString;
  static std::string const kWayString;

  RestrictionWriter(std::string const & filename,
                    std::shared_ptr<generator::cache::IntermediateDataReader> const & cache);

  // generator::CollectorInterface overrides:
  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<generator::cache::IntermediateDataReader> const & cache = {}) const override;

  void CollectRelation(RelationElement const & relationElement) override;
  void Finish() override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(RestrictionWriter & collector) const override;

  static ViaType ConvertFromString(std::string const & str);

private:
  std::ofstream m_writer;
  std::shared_ptr<generator::cache::IntermediateDataReader> m_cache;
};

std::string DebugPrint(RestrictionWriter::ViaType const & type);
}  // namespace routing

