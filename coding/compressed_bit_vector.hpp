#include "std/vector.hpp"

#include "base/assert.hpp"
#include "base/bits.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "std/algorithm.hpp"
#include "std/unique_ptr.hpp"

#include "base/assert.hpp"

namespace coding
{
class CompressedBitVector
{
public:
  enum class StorageStrategy
  {
    Dense,
    Sparse
  };

  virtual ~CompressedBitVector() = default;

  // Executes f for each bit that is set to one using
  // the bit's 0-based position as argument.
  template <typename F>
  void ForEach(F && f) const;

  // Intersects two bit vectors.
  static unique_ptr<CompressedBitVector> Intersect(CompressedBitVector const &,
                                                   CompressedBitVector const &);

  // Returns the number of set bits (population count).
  virtual uint32_t PopCount() const = 0;

  // todo(@pimenov) How long will 32 bits be enough here?
  // Would operator[] look better?
  virtual bool GetBit(uint32_t pos) const = 0;

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
};

string DebugPrint(CompressedBitVector::StorageStrategy strat);

class DenseCBV : public CompressedBitVector
{
public:
  // Builds a dense CBV from a list of positions of set bits.
  DenseCBV(vector<uint64_t> const & setBits);

  // Builds a dense CBV from a packed bitmap of set bits.
  // todo(@pimenov) This behaviour of & and && constructors is extremely error-prone.
  DenseCBV(vector<uint64_t> && bitMasks) : m_bits(move(bitMasks))
  {
    m_popCount = 0;
    for (size_t i = 0; i < m_bits.size(); ++i)
      m_popCount += bits::PopCount(m_bits[i]);
  }

  ~DenseCBV() = default;

  size_t NumBitGroups() const { return m_bits.size(); }

  template <typename F>
  void ForEach(F && f) const;

  uint64_t GetBitGroup(size_t i) const
  {
    if (i < m_bits.size())
      return m_bits[i];
    return 0;
  }

  // CompressedBitVector overrides:

  uint32_t PopCount() const override;

  bool GetBit(uint32_t pos) const override;

  StorageStrategy GetStorageStrategy() const override;

  void Serialize(Writer & writer) const override;

private:
  vector<uint64_t> m_bits;
  uint32_t m_popCount;
};

class SparseCBV : public CompressedBitVector
{
public:
  SparseCBV(vector<uint64_t> const & setBits) : m_positions(setBits)
  {
    ASSERT(is_sorted(m_positions.begin(), m_positions.end()), ());
  }

  SparseCBV(vector<uint64_t> && setBits) : m_positions(move(setBits))
  {
    ASSERT(is_sorted(m_positions.begin(), m_positions.end()), ());
  }

  ~SparseCBV() = default;

  // Returns the position of the i'th set bit.
  uint64_t Select(size_t i) const
  {
    ASSERT_LESS(i, m_positions.size(), ());
    return m_positions[i];
  }

  template <typename F>
  void ForEach(F && f) const;

  // CompressedBitVector overrides:

  uint32_t PopCount() const override;

  bool GetBit(uint32_t pos) const override;

  StorageStrategy GetStorageStrategy() const override;

  void Serialize(Writer & writer) const override;

private:
  // 0-based positions of the set bits.
  vector<uint64_t> m_positions;
};

class CompressedBitVectorBuilder
{
public:
  // Chooses a strategy to store the bit vector with bits from setBits set to one
  // and returns a pointer to a class that fits best.
  static unique_ptr<CompressedBitVector> Build(vector<uint64_t> const & setBits);

  // Reads a bit vector from reader which must contain a valid
  // bit vector representation (see CompressedBitVector::Serialize for the format).
  template <typename TReader>
  static unique_ptr<CompressedBitVector> Deserialize(TReader & reader)
  {
    ReaderSource<TReader> src(reader);
    uint8_t header = ReadPrimitiveFromSource<uint8_t>(reader);
    CompressedBitVector::StorageStrategy strat =
        static_cast<CompressedBitVector::StorageStrategy>(header);
    switch (strat)
    {
      case CompressedBitVector::StorageStrategy::Dense:
      {
        uint32_t numBitGroups = ReadPrimitiveFromSource<uint32_t>(reader);
        vector<uint64_t> bitGroups(numBitGroups);
        for (size_t i = 0; i < numBitGroups; ++i)
          bitGroups[i] = ReadPrimitiveFromSource<uint64_t>(reader);
        return make_unique<DenseCBV>(move(bitGroups));
      }
      case CompressedBitVector::StorageStrategy::Sparse:
      {
        uint32_t numBits = ReadPrimitiveFromSource<uint32_t>(reader);
        vector<uint64_t> setBits(numBits);
        for (size_t i = 0; i < numBits; ++i)
          setBits[i] = ReadPrimitiveFromSource<uint64_t>(reader);
        return make_unique<SparseCBV>(setBits);
      }
    }
    return nullptr;
  }
};
}  // namespace coding
