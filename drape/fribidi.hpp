#pragma once

#include "base/string_utils.hpp"

#include "3party/fribidi/lib/fribidi.h"

namespace fribidi
{

strings::UniString log2vis(strings::UniString const & str);

}
