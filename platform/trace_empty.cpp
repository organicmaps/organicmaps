#include "platform/trace.hpp"

namespace platform
{
class TraceImpl {};

Trace::Trace() noexcept = default;

Trace::~Trace() noexcept = default;
  
void Trace::BeginSection(char const * name) noexcept {}

void Trace::EndSection() noexcept {}

void Trace::SetCounter(char const * name, int64_t value) noexcept {}
}  // namespace platform
