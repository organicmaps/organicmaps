#pragma once

#include "base/macros.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace platform
{
class TraceImpl;

// API is inspired by https://developer.android.com/ndk/reference/group/tracing
class Trace 
{
public:
  Trace() noexcept;
  ~Trace() noexcept;
  
  void BeginSection(char const * name) noexcept;
  void EndSection() noexcept;
  void SetCounter(char const * name, int64_t value) noexcept;

private:
  std::unique_ptr<TraceImpl> m_impl;

  DISALLOW_COPY_AND_MOVE(Trace);
};

class TraceSection
{
public:
  inline TraceSection(Trace & trace, char const * section) noexcept 
    : m_trace(trace) 
  {
    m_trace.BeginSection(section);
  }

  inline ~TraceSection() noexcept {
    m_trace.EndSection();
  }

private:
  Trace & m_trace;
};
}  // namespace platform

#ifdef ENABLE_TRACE
#define TRACE_SECTION(section) platform::TraceSection ___section(GetPlatform().GetTrace(), section)
#define TRACE_COUNTER(name, value) GetPlatform().GetTrace().SetCounter(name, value)
#else
#define TRACE_SECTION(section) static_cast<void>(0)
#define TRACE_COUNTER(name, value) static_cast<void>(0)
#endif
