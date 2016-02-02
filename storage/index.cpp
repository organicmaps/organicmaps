#include "storage/index.hpp"

#include "std/sstream.hpp"

namespace storage
{
storage::TCountryId const kInvalidCountryId;

bool IsCountryIdValid(TCountryId const & countryId)
{
  return countryId != kInvalidCountryId;
}
} //  namespace storage
