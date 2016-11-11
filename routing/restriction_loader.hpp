#pragma once

#include "routing/routing_serialization.hpp"

#include "indexer/index.hpp"

#include "coding/file_container.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace routing
{
class RestrictionLoader
{
public:
  explicit RestrictionLoader(MwmValue const & mwmValue);

  bool HasRestrictions() const { return !m_restrictions.empty(); }
  RestrictionVec const & GetRestrictions() const { return m_restrictions; }

private:
  unique_ptr<FilesContainerR::TReader> m_reader;
  RoutingHeader m_header;
  RestrictionVec m_restrictions;
  string const m_countryFileName;
};
}  // namespace routing
