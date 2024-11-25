#pragma once

#include <cstddef>
#include <cstdint>

namespace om::network::http
{
class IThreadCallback
{
public:
  virtual bool OnWrite(int64_t offset, void const * buffer, size_t size) = 0;
  virtual void OnFinish(long httpOrErrorCode, int64_t begRange, int64_t endRange) = 0;

protected:
  virtual ~IThreadCallback() = default;
};
}  // namespace om::network::http
