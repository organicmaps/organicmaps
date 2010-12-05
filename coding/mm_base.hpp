#pragma once
#include "../base/assert.hpp"
#include "../base/base.hpp"

#include "../base/start_mem_debug.hpp"

class MMParseInfo
{
public:
  MMParseInfo(void const * p, size_t size, bool failOnError) :
      m_p(static_cast<char const *>(p)), m_Size(size), m_bFailOnError(failOnError),
      m_bSuccessful(true)
  {
  }

  ~MMParseInfo()
  {
    CHECK(!m_bFailOnError || m_bSuccessful, ());
  }

  void CheckAligned(size_t size)
  {
    size_t p = reinterpret_cast<size_t>(m_p);
    if (!(size & 7)) {
      CHECK_OR_CALL(m_bFailOnError, Fail, !(p & 7), (p, size));
    }
    else if (!(size & 3)) {
      CHECK_OR_CALL(m_bFailOnError, Fail, !(p & 3), (p, size));
    }
    else if (!(size & 1)) {
      CHECK_OR_CALL(m_bFailOnError, Fail, !(p & 1), (p, size));
    }
  }

  template <typename T> T const * Advance()
  {
    CheckAligned(sizeof(T));
    size_t const advanceSize = sizeof(T);
    CHECK_OR_CALL(m_bFailOnError, Fail, advanceSize <= m_Size, (sizeof(T), m_Size));
    void const * p = m_p;
    m_p += advanceSize;
    m_Size -= advanceSize;
    return static_cast<T const *>(p);
  }

  template <typename T> T const * Advance(size_t size)
  {
    CheckAligned(sizeof(T));
    size_t const advanceSize = size * sizeof(T);
    CHECK_OR_CALL(m_bFailOnError, Fail, advanceSize <= m_Size, (size, sizeof(T), m_Size));
    void const * p = m_p;
    m_p += advanceSize;
    m_Size -= advanceSize;
    return static_cast<T const *>(p);
  }

  bool Successful() const
  {
    return m_bSuccessful;
  }

  void Fail()
  {
    CHECK(!m_bFailOnError, (m_bSuccessful));
    m_bSuccessful = false;
  }

  bool FailOnError() const
  {
    return m_bFailOnError;
  }

private:
  char const * m_p;
  size_t m_Size;
  bool m_bFailOnError;
  bool m_bSuccessful;
};

#include "../base/stop_mem_debug.hpp"
