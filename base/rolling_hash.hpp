#pragma once
#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/bits.hpp"
#include "base/math.hpp"
#ifdef DEBUG
#include <queue>
#endif


template <typename T, typename HashT> class RabinKarpRollingHasher
{
public:
  typedef T value_type;
  typedef HashT hash_type;
  explicit RabinKarpRollingHasher(HashT multiplier)
    : m_Hash(0), m_Multiplier(multiplier), m_RemoveMultiplier(0)
  {
  }

  template <typename IterT> hash_type Init(IterT it, uint64_t windowSize)
  {
    m_WindowSize = windowSize;
    m_RemoveMultiplier = my::PowUint(m_Multiplier, m_WindowSize - 1);
#ifdef DEBUG
    while (!m_Queue.empty()) m_Queue.pop();
#endif
    m_Hash = 0;
    for (uint64_t i = 0; i < m_WindowSize; ++it, ++i)
    {
      m_Hash = m_Hash * m_Multiplier + *it;
#ifdef DEBUG
      m_Queue.push(*it);
#endif
    }
    return m_Hash;
  }

  hash_type Scroll(T const remove, T const add)
  {
    ASSERT_NOT_EQUAL(m_RemoveMultiplier, 0, (m_Multiplier, m_WindowSize, remove, add));
    ASSERT_NOT_EQUAL(m_WindowSize, 0, (m_Multiplier, remove, add));
#ifdef DEBUG
    ASSERT_EQUAL(m_Queue.front(), remove, ());
    m_Queue.pop();
    m_Queue.push(add);
#endif
    m_Hash -= m_RemoveMultiplier * remove;
    m_Hash = m_Hash * m_Multiplier + add;
    return m_Hash;
  }

private:
  hash_type m_Hash;
  hash_type m_Multiplier;
  hash_type m_RemoveMultiplier;
  uint64_t m_WindowSize;
#ifdef DEBUG
  std::queue<value_type> m_Queue;
#endif
};

class RabinKarpRollingHasher32 : public RabinKarpRollingHasher<uint32_t, uint32_t>
{
public:
  RabinKarpRollingHasher32()
    : RabinKarpRollingHasher<uint32_t, uint32_t>(1103515245U) {}
};

class RabinKarpRollingHasher64 : public RabinKarpRollingHasher<uint64_t, uint64_t>
{
public:
  RabinKarpRollingHasher64()
    : RabinKarpRollingHasher<uint64_t, uint64_t>(6364136223846793005ULL) {}
};

typedef RabinKarpRollingHasher32 RollingHasher32;
typedef RabinKarpRollingHasher64 RollingHasher64;
