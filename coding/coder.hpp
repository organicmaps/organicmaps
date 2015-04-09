#pragma once
#include "base/exception.hpp"

DECLARE_EXCEPTION(StringCodingException, RootException);
DECLARE_EXCEPTION(DstOutOfMemoryStringCodingException, StringCodingException);
