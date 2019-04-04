#pragma once

#include "generator/collector_interface.hpp"
#include "generator/intermediate_data.hpp"

#include <fstream>
#include <string>

class RelationElement;

namespace routing
{
class RestrictionWriter : public generator::CollectorInterface
{
public:

  enum class ViaType
  {
    Node,
    Way,

    Max,
  };

  static std::string const kNodeString;
  static std::string const kWayString;

  RestrictionWriter(std::string const & fullPath,
                    generator::cache::IntermediateDataReader const & cache);

  void CollectRelation(RelationElement const & relationElement) override;
  void Save() override {}

  static ViaType ConvertFromString(std::string const & str);

private:
  void Open(std::string const & fullPath);
  bool IsOpened() const;

  std::ofstream m_stream;
  generator::cache::IntermediateDataReader const & m_cache;
};

std::string DebugPrint(RestrictionWriter::ViaType const & type);
}  // namespace routing

