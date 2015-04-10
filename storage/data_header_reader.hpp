#pragma once
#include "base/base.hpp"

#include "std/string.hpp"

class Writer;
class Reader;

namespace feature
{
  class DataHeader;

  /// @return total header size, which should be skipped for data read, or 0 if error
  uint64_t GetSkipHeaderSize(Reader const & reader);
  uint64_t ReadDataHeader(string const & datFileName, feature::DataHeader & outHeader);
  void WriteDataHeader(Writer & writer, feature::DataHeader const & header);
}
