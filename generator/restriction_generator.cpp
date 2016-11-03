#include "generator/restriction_generator.hpp"
#include "generator/restriction_collector.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include "std/algorithm.hpp"

using namespace feature;
using namespace routing;

namespace
{
/// \brief Serializes a range of restrictions form |begin| to |end| to |sink|.
/// \param begin is an iterator to the first item to serialize.
/// \param end is an iterator to the element after the last element to serialize.
/// \note All restrictions should have the same type.
void SerializeRestrictions(RestrictionVec::const_iterator begin, RestrictionVec::const_iterator end,
                           FileWriter & sink)
{
  if (begin == end)
    return;

  Restriction::Type const type = begin->m_type;

  Restriction prevRestriction(type, 0);
  prevRestriction.m_links.resize(feature::RestrictionSerializer::kSupportedLinkNumber, 0);
  for (auto it = begin; it != end; ++it)
  {
    CHECK_EQUAL(type, it->m_type, ());
    RestrictionSerializer serializer(*it);
    serializer.Serialize(prevRestriction, sink);
    prevRestriction = serializer.GetRestriction();
  }
}
}  // namespace

namespace routing
{
bool BuildRoadRestrictions(string const & mwmPath, string const & restrictionPath,
                           string const & featureId2OsmIdsPath)
{
  LOG(LINFO, ("BuildRoadRestrictions(", mwmPath, ", ", restrictionPath, ", ", featureId2OsmIdsPath, ");"));
  RestrictionCollector restrictionCollector(restrictionPath, featureId2OsmIdsPath);
  if (!restrictionCollector.IsValid())
  {
    LOG(LWARNING, ("No valid restrictions for", mwmPath, "It's necessary to check if",
                   restrictionPath, "and", featureId2OsmIdsPath, "are available."));
    return false;
  }

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();

  auto const firstOnlyIt = upper_bound(restrictions.cbegin(), restrictions.cend(),
                                       Restriction(Restriction::Type::No, 0), my::LessBy(&Restriction::m_type));
  RoutingHeader header;
  header.m_noRestrictionCount = distance(restrictions.cbegin(), firstOnlyIt);
  header.m_onlyRestrictionCount = restrictions.size() - header.m_noRestrictionCount;
  LOG(LINFO, ("Header info. There are", header.m_noRestrictionCount, "and", header.m_onlyRestrictionCount,
              "only restrictions"));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(ROUTING_FILE_TAG);
  header.Serialize(w);
  SerializeRestrictions(restrictions.cbegin(), firstOnlyIt, w);
  SerializeRestrictions(firstOnlyIt, restrictions.end(), w);

  return true;
}
}  // namespace routing
