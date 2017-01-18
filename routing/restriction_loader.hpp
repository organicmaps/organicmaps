#pragma once

#include "routing/restrictions_serialization.hpp"

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
  RestrictionVec && StealRestrictions() { return move(m_restrictions); }

private:
  unique_ptr<FilesContainerR::TReader> m_reader;
  RestrictionHeader m_header;
  RestrictionVec m_restrictions;
  string const m_countryFileName;
};
}  // namespace routing
