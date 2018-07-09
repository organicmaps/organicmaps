#include "generator/restriction_generator.hpp"

#include "generator/restriction_collector.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include <algorithm>

namespace routing
{
bool BuildRoadRestrictions(std::string const & mwmPath, std::string const & restrictionPath,
                           std::string const & osmIdsTofeatureIdsPath)
{
  LOG(LDEBUG, ("BuildRoadRestrictions(", mwmPath, ", ", restrictionPath, ", ",
              osmIdsTofeatureIdsPath, ");"));
  RestrictionCollector restrictionCollector(restrictionPath, osmIdsTofeatureIdsPath);
  if (!restrictionCollector.HasRestrictions())
  {
    LOG(LINFO, ("No restrictions for", mwmPath, "It's necessary to check that",
                   restrictionPath, "and", osmIdsTofeatureIdsPath, "are available."));
    return false;
  }
  if (!restrictionCollector.IsValid())
  {
    LOG(LWARNING, ("Found invalid restrictions for", mwmPath, "Are osm2ft files relevant?"));
    return false;
  }

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();

  auto const firstOnlyIt =
      std::lower_bound(restrictions.cbegin(), restrictions.cend(),
                       Restriction(Restriction::Type::Only, {} /* links */),
                       my::LessBy(&Restriction::m_type));

  RestrictionHeader header;
  header.m_noRestrictionCount = base::checked_cast<uint32_t>(std::distance(restrictions.cbegin(), firstOnlyIt));
  header.m_onlyRestrictionCount = base::checked_cast<uint32_t>(restrictions.size() - header.m_noRestrictionCount);

  LOG(LINFO, ("Header info. There are", header.m_noRestrictionCount, "restrictions of type No and",
              header.m_onlyRestrictionCount, "restrictions of type Only"));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(RESTRICTIONS_FILE_TAG);
  header.Serialize(w);

  RestrictionSerializer::Serialize(header, restrictions.cbegin(), restrictions.cend(), w);

  return true;
}
}  // namespace routing
