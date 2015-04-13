#pragma once

#include "storage/index.hpp"

#include "../core/jni_helper.hpp"


namespace storage
{
  jobject ToJava(TIndex const & idx);
  TIndex ToNative(jobject idx);
}
