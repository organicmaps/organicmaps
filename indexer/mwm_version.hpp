#pragma once

#include "../std/stdint.hpp"


class ModelReaderPtr;
class Writer;
class ReaderSrc;

namespace ver
{
  void WriteVersion(Writer & w);

  /// @return See feature::DataHeader::Version for more details.
  uint32_t ReadVersion(ModelReaderPtr const & r);

  /// @return Data timestamp in yymmdd format.
  uint32_t ReadTimestamp(ReaderSrc & src);
}
