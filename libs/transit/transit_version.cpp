#include "transit/transit_version.hpp"

#include "base/logging.hpp"

#include <exception>

namespace transit
{
TransitVersion GetVersion(Reader & reader)
{
  try
  {
    NonOwningReaderSource src(reader);
    uint16_t headerVersion = ReadPrimitiveFromSource<uint16_t>(src);
    CHECK_LESS(headerVersion, static_cast<uint16_t>(TransitVersion::Counter), ());

    return static_cast<TransitVersion>(headerVersion);
  }
  catch (std::exception const & ex)
  {
    LOG(LERROR, ("Error reading header version from transit mwm section.", ex.what()));
    throw;
  }
}
}  // namespace transit
