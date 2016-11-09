#include "generator/restriction_generator.hpp"
#include "generator/restriction_collector.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"

using namespace feature;

namespace routing
{
bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                           string const & featureId2OsmIdsPath)
{
  LOG(LINFO,
      ("BuildRoadRestrictions(", mwmPath, ", ", restrictionPath, ", ", featureId2OsmIdsPath, ");"));
  RestrictionCollector restrictionCollector(restrictionPath, featureId2OsmIdsPath);
  if (!restrictionCollector.HasRestrictions() || !restrictionCollector.IsValid())
  {
    LOG(LWARNING, ("No valid restrictions for", mwmPath, "It's necessary to check if",
                   restrictionPath, "and", featureId2OsmIdsPath, "are available."));
    return false;
  }

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();

  auto const firstOnlyIt =
      upper_bound(restrictions.cbegin(), restrictions.cend(),
                  Restriction(Restriction::Type::No, {} /* links */), my::LessBy(&Restriction::m_type));
  RoutingHeader header;
  header.m_noRestrictionCount = distance(restrictions.cbegin(), firstOnlyIt);
  header.m_onlyRestrictionCount = restrictions.size() - header.m_noRestrictionCount;
  LOG(LINFO, ("Header info. There are", header.m_noRestrictionCount, "no restrictions and",
              header.m_onlyRestrictionCount, "only restrictions"));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(ROUTING_FILE_TAG);
  header.Serialize(w);

  RestrictionSerializer::Serialize(restrictions.cbegin(), firstOnlyIt, restrictions.cend(), w);

  return true;
}
}  // namespace routing
