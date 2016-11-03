#pragma once

#include "indexer/index.hpp"
#include "indexer/routing.hpp"

#include "coding/file_container.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

namespace feature
{
class RestrictionLoader
{
public:
  explicit RestrictionLoader(MwmValue const & mwmValue);

  bool HasRestrictions() const { return !m_restrictions.empty(); }

  routing::RestrictionVec const & GetRestrictions() const { return m_restrictions; }

private:
  unique_ptr<FilesContainerR::TReader> m_reader;
  RoutingHeader m_header;
  routing::RestrictionVec m_restrictions;
  string m_countryFileName;
};
}  // namespace feature
