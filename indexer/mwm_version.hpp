#pragma once

#include "../std/stdint.hpp"


class ModelReaderPtr;
class Writer;

namespace ver
{
  void WriteVersion(Writer & w);
  uint32_t ReadVersion(ModelReaderPtr const & r);
}
