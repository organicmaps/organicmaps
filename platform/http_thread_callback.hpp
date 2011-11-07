#pragma once

namespace downloader
{

class IHttpThreadCallback
{
public:
  virtual void OnWrite(int64_t offset, void const * buffer, size_t size) = 0;
  virtual void OnFinish(long httpCode, int64_t begRange, int64_t endRange) = 0;
};

} // namespace downloader
