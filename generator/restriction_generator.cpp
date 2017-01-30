#include "generator/restriction_generator.hpp"

#include "generator/restriction_collector.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"

namespace routing
{
bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                           string const & osmIdsTofeatureIdsPath)
{
  LOG(LINFO, ("BuildRoadRestrictions(", mwmPath, ", ", restrictionPath, ", ",
              osmIdsTofeatureIdsPath, ");"));
  RestrictionCollector restrictionCollector(restrictionPath, osmIdsTofeatureIdsPath);
  if (!restrictionCollector.HasRestrictions() || !restrictionCollector.IsValid())
  {
    LOG(LWARNING, ("No valid restrictions for", mwmPath, "It's necessary to check that",
                   restrictionPath, "and", osmIdsTofeatureIdsPath, "are available."));
    return false;
  }

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();

  auto const firstOnlyIt =
      lower_bound(restrictions.cbegin(), restrictions.cend(),
                  Restriction(Restriction::Type::Only, {} /* links */), my::LessBy(&Restriction::m_type));

  RestrictionHeader header;
  header.m_noRestrictionCount = base::checked_cast<uint32_t>(distance(restrictions.cbegin(), firstOnlyIt));
  header.m_onlyRestrictionCount = base::checked_cast<uint32_t>(restrictions.size() - header.m_noRestrictionCount);

  LOG(LINFO, ("Header info. There are", header.m_noRestrictionCount, "of type No restrictions and",
              header.m_onlyRestrictionCount, "of type Only restrictions"));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(RESTRICTIONS_FILE_TAG);
  header.Serialize(w);

  RestrictionSerializer::Serialize(header, restrictions.cbegin(), restrictions.cend(), w);

  return true;
}
}  // namespace routing
