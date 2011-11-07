#pragma once

namespace downloader
{

class IHttpRequestImplCallback
{
public:
  /// Called before OnWrite, projectedSize can be -1 if server doesn't support it
  virtual void OnSizeKnown(int64_t projectedSize) = 0;
  virtual void OnWrite(int64_t offset, void const * buffer, size_t size) = 0;
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange) = 0;
};

} // namespace downloader
