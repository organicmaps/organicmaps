#pragma once

#include "generator/collector_interface.hpp"
#include "generator/intermediate_data.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace routing_builder
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

  RestrictionWriter(std::string const & filename, IDRInterfacePtr const & cache);

  // generator::CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const override;

  void CollectRelation(RelationElement const & relationElement) override;
  void Finish() override;

  IMPLEMENT_COLLECTOR_IFACE(RestrictionWriter);
  void MergeInto(RestrictionWriter & collector) const;

  static ViaType ConvertFromString(std::string const & str);

protected:
  // generator::CollectorInterface overrides:
  void Save() override;
  void OrderCollectedData() override;

private:
  std::ofstream m_stream;
  IDRInterfacePtr m_cache;
};

std::string DebugPrint(RestrictionWriter::ViaType const & type);
}  // namespace routing_builder
