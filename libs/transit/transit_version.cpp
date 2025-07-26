#include "transit/transit_version.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <exception>
#include <limits>

namespace transit
{
TransitVersion GetVersion(Reader & reader)
{
  try
  {
    NonOwningReaderSource src(reader);
    uint16_t headerVersion = std::numeric_limits<uint16_t>::max();
    ReadPrimitiveFromSource(src, headerVersion);
    CHECK_LESS(headerVersion, static_cast<uint16_t>(TransitVersion::Counter), ("Invalid transit header version."));

    return static_cast<TransitVersion>(headerVersion);
  }
  catch (std::exception const & ex)
  {
    LOG(LERROR, ("Error reading header version from transit mwm section.", ex.what()));
    throw;
  }
}
}  // namespace transit
