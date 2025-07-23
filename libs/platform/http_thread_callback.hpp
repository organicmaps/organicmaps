#pragma once

#include <cstdint>

namespace downloader
{
class IHttpThreadCallback
{
public:
  virtual bool OnWrite(int64_t offset, void const * buffer, size_t size) = 0;
  virtual void OnFinish(long httpOrErrorCode, int64_t begRange, int64_t endRange) = 0;

protected:
  virtual ~IHttpThreadCallback() = default;
};
}  // namespace downloader
