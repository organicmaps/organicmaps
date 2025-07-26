#pragma once

#include "base/assert.hpp"
#include "base/bits.hpp"

#include <cstdint>
#include <iterator>
#include <sstream>
#include <string>

namespace base
{
// A set of nonnegative integers less than |UpperBound|.
//
// Requires UpperBound + O(1) bits of memory.  All operations except
// Clear() and iteration are O(1).  Clear() and iteration require
// O(UpperBound) steps.
//
// *NOTE* This class *IS NOT* thread safe.
template <uint64_t UpperBound>
class SmallSet
{
public:
  static uint64_t constexpr kNumBlocks = (UpperBound + 63) / 64;
  static_assert(kNumBlocks > 0);

  class Iterator
  {
  public:
    using difference_type = uint64_t;
    using value_type = uint64_t;
    using pointer = void;
    using reference = uint64_t;
    using iterator_category = std::forward_iterator_tag;

    Iterator(uint64_t const * blocks, uint64_t current_block_index)
      : m_blocks(blocks)
      , m_current_block_index(current_block_index)
      , m_current_block(0)
    {
      ASSERT_LESS_OR_EQUAL(current_block_index, kNumBlocks, ());
      if (current_block_index < kNumBlocks)
        m_current_block = m_blocks[current_block_index];
      SkipZeroes();
    }

    bool operator==(Iterator const & rhs) const
    {
      return m_blocks == rhs.m_blocks && m_current_block_index == rhs.m_current_block_index &&
             m_current_block == rhs.m_current_block;
    }

    bool operator!=(Iterator const & rhs) const { return !(*this == rhs); }

    uint64_t operator*() const
    {
      ASSERT_NOT_EQUAL(m_current_block, 0, ());
      auto const bit = m_current_block & -m_current_block;
      return bits::FloorLog(bit) + m_current_block_index * 64;
    }

    Iterator const & operator++()
    {
      ASSERT(m_current_block_index < kNumBlocks, ());
      ASSERT_NOT_EQUAL(m_current_block, 0, ());
      m_current_block = m_current_block & (m_current_block - 1);
      SkipZeroes();
      return *this;
    }

  private:
    void SkipZeroes()
    {
      ASSERT_LESS_OR_EQUAL(m_current_block_index, kNumBlocks, ());

      if (m_current_block != 0 || m_current_block_index == kNumBlocks)
        return;

      do
        ++m_current_block_index;
      while (m_current_block_index < kNumBlocks && m_blocks[m_current_block_index] == 0);
      if (m_current_block_index < kNumBlocks)
        m_current_block = m_blocks[m_current_block_index];
      else
        m_current_block = 0;
    }

    uint64_t const * m_blocks;
    uint64_t m_current_block_index;
    uint64_t m_current_block;
  };

#define DEFINE_BLOCK_OFFSET(value)   \
  uint64_t const block = value / 64; \
  uint64_t const offset = value % 64

  // This invalidates all iterators except end().
  void Insert(uint64_t value)
  {
    ASSERT_LESS(value, UpperBound, ());

    DEFINE_BLOCK_OFFSET(value);
    auto const bit = kOne << offset;
    m_size += (m_blocks[block] & bit) == 0;
    m_blocks[block] |= bit;
  }

  // This invalidates all iterators except end().
  void Remove(uint64_t value)
  {
    ASSERT_LESS(value, UpperBound, ());

    DEFINE_BLOCK_OFFSET(value);
    auto const bit = kOne << offset;
    m_size -= (m_blocks[block] & bit) != 0;
    m_blocks[block] &= ~bit;
  }

  bool Contains(uint64_t value) const
  {
    ASSERT_LESS(value, UpperBound, ());

    DEFINE_BLOCK_OFFSET(value);
    return m_blocks[block] & (kOne << offset);
  }

#undef DEFINE_BLOCK_OFFSET

  uint64_t Size() const { return m_size; }

  // This invalidates all iterators except end().
  void Clear()
  {
    std::fill(std::begin(m_blocks), std::end(m_blocks), static_cast<uint64_t>(0));
    m_size = 0;
  }

  Iterator begin() const { return Iterator(m_blocks, 0); }
  Iterator cbegin() const { return Iterator(m_blocks, 0); }

  Iterator end() const { return Iterator(m_blocks, kNumBlocks); }
  Iterator cend() const { return Iterator(m_blocks, kNumBlocks); }

private:
  static uint64_t constexpr kOne = 1;

  uint64_t m_blocks[kNumBlocks] = {};
  uint64_t m_size = 0;
};

// static
template <uint64_t UpperBound>
uint64_t constexpr SmallSet<UpperBound>::kNumBlocks;

// static
template <uint64_t UpperBound>
uint64_t constexpr SmallSet<UpperBound>::kOne;

template <uint64_t UpperBound>
std::string DebugPrint(SmallSet<UpperBound> const & set)
{
  std::ostringstream os;
  os << "SmallSet<" << UpperBound << "> [" << set.Size() << ": ";
  for (auto const & v : set)
    os << v << " ";
  os << "]";
  return os.str();
}

// This is a delegate for SmallSet<>, that checks the validity of
// argument in Insert(), Remove() and Contains() methods and does
// nothing when the argument is not valid.
template <uint64_t UpperBound>
class SafeSmallSet
{
public:
  using Set = SmallSet<UpperBound>;
  using Iterator = typename Set::Iterator;

  void Insert(uint64_t value)
  {
    if (IsValid(value))
      m_set.Insert(value);
  }

  void Remove(uint64_t value)
  {
    if (IsValid(value))
      m_set.Remove(value);
  }

  bool Contains(uint64_t value) const { return IsValid(value) && m_set.Contains(value); }

  uint64_t Size() const { return m_set.Size(); }

  void Clear() { m_set.Clear(); }

  Iterator begin() const { return m_set.begin(); }
  Iterator cbegin() const { return m_set.cbegin(); }

  Iterator end() const { return m_set.end(); }
  Iterator cend() const { return m_set.cend(); }

private:
  bool IsValid(uint64_t value) const { return value < UpperBound; }

  Set m_set;
};

template <uint64_t UpperBound>
std::string DebugPrint(SafeSmallSet<UpperBound> const & set)
{
  std::ostringstream os;
  os << "SafeSmallSet<" << UpperBound << "> [" << set.Size() << ": ";
  for (auto const v : set)
    os << v << " ";
  os << "]";
  return os.str();
}
}  // namespace base
