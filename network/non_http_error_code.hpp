#pragma once

#include <cstdint>
#include <string>

namespace om::network::non_http_error_code
{
auto constexpr kIOException = -1;
auto constexpr kWriteException = -2;
auto constexpr kInconsistentFileSize = -3;
auto constexpr kNonHttpResponse = -4;
auto constexpr kInvalidURL = -5;
auto constexpr kCancelled = -6;

inline std::string DebugPrint(long errorCode)
{
  switch (errorCode)
  {
  case kIOException: return "IO exception";
  case kWriteException: return "Write exception";
  case kInconsistentFileSize: return "Inconsistent file size";
  case kNonHttpResponse: return "Non-http response";
  case kInvalidURL: return "Invalid URL";
  case kCancelled: return "Cancelled";
  default: return std::to_string(errorCode);
  }
}
}  // namespace om::network::non_http_error_code
