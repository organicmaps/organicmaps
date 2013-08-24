#pragma once

#include "../../../../../storage/index.hpp"

#include "../core/jni_helper.hpp"


namespace storage
{
  jobject toJava(TIndex const & idx);
  TIndex toNative(jobject idx);
}
