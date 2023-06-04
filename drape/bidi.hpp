#pragma once

#include "base/string_utils.hpp"

namespace bidi
{

strings::UniString log2vis(strings::UniString const & str);
strings::UniString log2vis(std::string_view utf8);

} // namespace bidi
