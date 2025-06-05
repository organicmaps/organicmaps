#pragma once

#include "routing/index_graph.hpp"
#include "routing/restrictions_serialization.hpp"

#include "coding/files_container.hpp"

#include <memory>
#include <string>

class MwmValue;

namespace routing
{
class RestrictionLoader
{
public:
  explicit RestrictionLoader(MwmValue const & mwmValue, IndexGraph & graph);

  bool HasRestrictions() const;
  RestrictionVec && StealRestrictions();
  std::vector<RestrictionUTurn> && StealNoUTurnRestrictions();

private:
  std::unique_ptr<FilesContainerR::TReader> m_reader;
  RestrictionHeader m_header;
  RestrictionVec m_restrictions;
  std::vector<RestrictionUTurn> m_noUTurnRestrictions;
  std::string const m_countryFileName;
};

void ConvertRestrictionsOnlyToNo(IndexGraph const & graph, RestrictionVec const & restrictionsOnly,
                                 RestrictionVec & restrictionsNo);

void ConvertRestrictionsOnlyUTurnToNo(IndexGraph & graph, std::vector<RestrictionUTurn> const & restrictionsOnlyUTurn,
                                      RestrictionVec & restrictionsNo);
}  // namespace routing
