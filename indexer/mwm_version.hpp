#pragma once

#include "../std/stdint.hpp"


class ModelReaderPtr;
class Writer;

namespace ver
{
  class VersionData
  {
    uint32_t m_mwmVer;
    uint32_t m_timeStamp;
  };

  void WriteVersion(Writer & w);
  uint32_t ReadVersion(ModelReaderPtr const & r);
}
