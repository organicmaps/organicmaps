#include "search/base/text_index/text_index.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

namespace search_base
{
std::string DebugPrint(TextIndexVersion const & version)
{
  switch (version)
  {
  case TextIndexVersion::V0: return "V0";
  }
  std::string ret = "Unknown TextIndexHeader version: " + strings::to_string(static_cast<uint8_t>(version));
  ASSERT(false, (ret));
  return ret;
}
}  // namespace search_base
