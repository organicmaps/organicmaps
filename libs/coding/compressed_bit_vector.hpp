#pragma once

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"
#include "base/ref_counted.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace coding
{
class CompressedBitVector : public base::RefCounted
{
public:
  enum class StorageStrategy
  {
    Dense,
    Sparse
  };

  virtual ~CompressedBitVector() = default;

  // Intersects two bit vectors.
  // todo(@pimenov) We expect the common use case to be as follows.
  // A CBV is created in memory and several CBVs are read and intersected
  // with it one by one. The in-memory CBV may initially contain a bit
  // for every feature in an mwm and the intersected CBVs are read from
  // the leaves of a search trie.
  // Therefore an optimization of Intersect comes to mind: make a wrapper
  // around TReader that will read a representation of a CBV from disk
  // and intersect it bit by bit with the global in-memory CBV bypassing such
  // routines as allocating memory and choosing strategy. They all can be called only
  // once, namely in the end, when it is needed to pack the in-memory CBV into
  // a suitable representation and pass it to the caller.
  static std::unique_ptr<CompressedBitVector> Intersect(CompressedBitVector const & lhs,
                                                        CompressedBitVector const & rhs);

  // Subtracts two bit vectors.
  static std::unique_ptr<CompressedBitVector> Subtract(CompressedBitVector const & lhs,
                                                       CompressedBitVector const & rhs);

  // Unites two bit vectors.
  static std::unique_ptr<CompressedBitVector> Union(CompressedBitVector const & lhs, CompressedBitVector const & rhs);

  static bool IsEmpty(std::unique_ptr<CompressedBitVector> const & cbv);

  static bool IsEmpty(CompressedBitVector const * cbv);

  // Returns the number of set bits (population count).
  virtual uint64_t PopCount() const = 0;

  // todo(@pimenov) How long will 32 bits be enough here?
  // Would operator[] look better?
  virtual bool GetBit(uint64_t pos) const = 0;

  // Returns a subset of the current bit vector with first
  // min(PopCount(), |n|) set bits.
  virtual std::unique_ptr<CompressedBitVector> LeaveFirstSetNBits(uint64_t n) const = 0;

  // Returns the strategy used when storing this bit vector.
  virtual StorageStrategy GetStorageStrategy() const = 0;

  // Writes the contents of a bit vector to writer.
  // The first byte is always the header that defines the format.
  // Currently the header is 0 or 1 for Dense and Sparse strategies respectively.
  // It is easier to dispatch via virtual method calls and not bother
  // with template TWriters here as we do in similar places in our code.
  // This should not pose too much a problem because commonly
  // used writers are inhereted from Writer anyway.
  // todo(@pimenov). Think about rewriting Serialize and Deserialize to use the
  // code in old_compressed_bit_vector.{c,h}pp.
  virtual void Serialize(Writer & writer) const = 0;

  // Copies a bit vector and returns a pointer to the copy.
  virtual std::unique_ptr<CompressedBitVector> Clone() const = 0;
};

std::string DebugPrint(CompressedBitVector::StorageStrategy strat);

class DenseCBV : public CompressedBitVector
{
public:
  friend class CompressedBitVectorBuilder;
  static uint64_t const kBlockSize = 64;

  DenseCBV() = default;

  // Builds a dense CBV from a list of positions of set bits.
  explicit DenseCBV(std::vector<uint64_t> const & setBits);

  // Not to be confused with the constructor: the semantics
  // of the array of integers is completely different.
  static std::unique_ptr<DenseCBV> BuildFromBitGroups(std::vector<uint64_t> && bitGroups);

  size_t NumBitGroups() const { return m_bitGroups.size(); }

  template <typename Fn>
  void ForEach(Fn && f) const
  {
    base::ControlFlowWrapper<Fn> wrapper(std::forward<Fn>(f));
    for (size_t i = 0; i < m_bitGroups.size(); ++i)
    {
      for (size_t j = 0; j < kBlockSize; ++j)
      {
        if (((m_bitGroups[i] >> j) & 1) > 0)
        {
          if (wrapper(kBlockSize * i + j) == base::ControlFlow::Break)
            return;
        }
      }
    }
  }

  // Returns 0 if the group number is too large to be contained in m_bits.
  uint64_t GetBitGroup(size_t i) const;

  // CompressedBitVector overrides:
  uint64_t PopCount() const override;
  bool GetBit(uint64_t pos) const override;
  std::unique_ptr<CompressedBitVector> LeaveFirstSetNBits(uint64_t n) const override;
  StorageStrategy GetStorageStrategy() const override;
  void Serialize(Writer & writer) const override;
  std::unique_ptr<CompressedBitVector> Clone() const override;

private:
  std::vector<uint64_t> m_bitGroups;
  uint64_t m_popCount = 0;
};

class SparseCBV : public CompressedBitVector
{
public:
  friend class CompressedBitVectorBuilder;
  using TIterator = std::vector<uint64_t>::const_iterator;

  SparseCBV() = default;

  explicit SparseCBV(std::vector<uint64_t> const & setBits);

  explicit SparseCBV(std::vector<uint64_t> && setBits);

  // Returns the position of the i'th set bit.
  uint64_t Select(size_t i) const;

  template <typename Fn>
  void ForEach(Fn && f) const
  {
    base::ControlFlowWrapper<Fn> wrapper(std::forward<Fn>(f));
    for (auto const & position : m_positions)
      if (wrapper(position) == base::ControlFlow::Break)
        return;
  }

  // CompressedBitVector overrides:
  uint64_t PopCount() const override;
  bool GetBit(uint64_t pos) const override;
  std::unique_ptr<CompressedBitVector> LeaveFirstSetNBits(uint64_t n) const override;
  StorageStrategy GetStorageStrategy() const override;
  void Serialize(Writer & writer) const override;
  std::unique_ptr<CompressedBitVector> Clone() const override;

  inline TIterator Begin() const { return m_positions.cbegin(); }
  inline TIterator End() const { return m_positions.cend(); }

private:
  // 0-based positions of the set bits.
  std::vector<uint64_t> m_positions;
};

class CompressedBitVectorBuilder
{
public:
  // Chooses a strategy to store the bit vector with bits from setBits set to one
  // and returns a pointer to a class that fits best.
  static std::unique_ptr<CompressedBitVector> FromBitPositions(std::vector<uint64_t> const & setBits);
  static std::unique_ptr<CompressedBitVector> FromBitPositions(std::vector<uint64_t> && setBits);

  // Chooses a strategy to store the bit vector with bits from a bitmap obtained
  // by concatenating the elements of bitGroups.
  static std::unique_ptr<CompressedBitVector> FromBitGroups(std::vector<uint64_t> & bitGroups);
  static std::unique_ptr<CompressedBitVector> FromBitGroups(std::vector<uint64_t> && bitGroups);

  // Reads a bit vector from reader which must contain a valid
  // bit vector representation (see CompressedBitVector::Serialize for the format).
  template <typename TReader>
  static std::unique_ptr<CompressedBitVector> DeserializeFromReader(TReader & reader)
  {
    ReaderSource<TReader> src(reader);
    return DeserializeFromSource(src);
  }

  // Reads a bit vector from source which must contain a valid
  // bit vector representation (see CompressedBitVector::Serialize for the format).
  template <typename TSource>
  static std::unique_ptr<CompressedBitVector> DeserializeFromSource(TSource & src)
  {
    uint8_t header = ReadPrimitiveFromSource<uint8_t>(src);
    CompressedBitVector::StorageStrategy strat = static_cast<CompressedBitVector::StorageStrategy>(header);
    switch (strat)
    {
    case CompressedBitVector::StorageStrategy::Dense:
    {
      std::vector<uint64_t> bitGroups;
      rw::ReadVectorOfPOD(src, bitGroups);
      return DenseCBV::BuildFromBitGroups(std::move(bitGroups));
    }
    case CompressedBitVector::StorageStrategy::Sparse:
    {
      std::vector<uint64_t> setBits;
      rw::ReadVectorOfPOD(src, setBits);
      return std::make_unique<SparseCBV>(std::move(setBits));
    }
    }
    return std::unique_ptr<CompressedBitVector>();
  }
};

// ForEach is generic and therefore cannot be virtual: a helper class is needed.
class CompressedBitVectorEnumerator
{
public:
  // Executes f for each bit that is set to one using
  // the bit's 0-based position as argument.
  template <typename Fn>
  static void ForEach(CompressedBitVector const & cbv, Fn && f)
  {
    CompressedBitVector::StorageStrategy strat = cbv.GetStorageStrategy();
    switch (strat)
    {
    case CompressedBitVector::StorageStrategy::Dense:
    {
      DenseCBV const & denseCBV = static_cast<DenseCBV const &>(cbv);
      denseCBV.ForEach(f);
      return;
    }
    case CompressedBitVector::StorageStrategy::Sparse:
    {
      SparseCBV const & sparseCBV = static_cast<SparseCBV const &>(cbv);
      sparseCBV.ForEach(f);
      return;
    }
    }
  }
};

class CompressedBitVectorHasher
{
public:
  static uint64_t Hash(CompressedBitVector const & cbv)
  {
    static constexpr uint64_t kBase = 127;
    uint64_t hash = 0;
    CompressedBitVectorEnumerator::ForEach(cbv, [&hash](uint64_t i) { hash = hash * kBase + i + 1; });
    return hash;
  }
};
}  // namespace coding
