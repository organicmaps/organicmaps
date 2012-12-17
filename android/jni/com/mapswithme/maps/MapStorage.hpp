#pragma once

#include "../../../../../storage/index.hpp"

#include "../core/jni_helper.hpp"


namespace storage
{
  jobject toJava(storage::TIndex const & idx);
}
