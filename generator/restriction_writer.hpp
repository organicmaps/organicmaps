#pragma once

#include "generator/collector_interface.hpp"

#include <fstream>
#include <string>

class RelationElement;

namespace routing
{
class RestrictionWriter : public generator::CollectorInterface
{
public:
  RestrictionWriter(std::string const & fullPath);
  // CollectorRelationsInterface overrides:
  /// \brief Writes |relationElement| to |m_stream| if |relationElement| is a supported restriction.
  /// See restriction_generator.hpp for the description of the format.
  /// \note For the time being only line-point-line restrictions are processed. The other
  /// restrictions are ignored.
  // @TODO(bykoianko) It's necessary to process all kind of restrictions.
  void CollectRelation(RelationElement const & relationElement) override;
  void Save() override {}

private:
  void Open(std::string const & fullPath);
  bool IsOpened() const;

  std::ofstream m_stream;
};
}  // namespace routing
