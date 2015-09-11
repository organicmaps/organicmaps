#include "coding/compressed_bit_vector.hpp"
#include "coding/writer.hpp"
#include "coding/write_to_sink.hpp"

#include "base/bits.hpp"

#include "std/algorithm.hpp"

namespace
{
uint64_t const kBlockSize = coding::DenseCBV::kBlockSize;

unique_ptr<coding::CompressedBitVector> IntersectImpl(coding::DenseCBV const & a,
                                                      coding::DenseCBV const & b)
{
  size_t sizeA = a.NumBitGroups();
  size_t sizeB = b.NumBitGroups();
  vector<uint64_t> resGroups(min(sizeA, sizeB));
  for (size_t i = 0; i < resGroups.size(); ++i)
    resGroups[i] = a.GetBitGroup(i) & b.GetBitGroup(i);
  return coding::CompressedBitVectorBuilder::FromBitGroups(move(resGroups));
}

// The intersection of dense and sparse is always sparse.
unique_ptr<coding::CompressedBitVector> IntersectImpl(coding::DenseCBV const & a,
                                                      coding::SparseCBV const & b)
{
  vector<uint64_t> resPos;
  for (size_t i = 0; i < b.PopCount(); ++i)
  {
    auto pos = b.Select(i);
    if (a.GetBit(pos))
      resPos.push_back(pos);
  }
  return make_unique<coding::SparseCBV>(move(resPos));
}

unique_ptr<coding::CompressedBitVector> IntersectImpl(coding::SparseCBV const & a,
                                                      coding::DenseCBV const & b)
{
  return IntersectImpl(b, a);
}

unique_ptr<coding::CompressedBitVector> IntersectImpl(coding::SparseCBV const & a,
                                                      coding::SparseCBV const & b)
{
  size_t sizeA = a.PopCount();
  size_t sizeB = b.PopCount();
  vector<uint64_t> resPos;
  size_t i = 0;
  size_t j = 0;
  while (i < sizeA && j < sizeB)
  {
    auto posA = a.Select(i);
    auto posB = b.Select(j);
    if (posA == posB)
    {
      resPos.push_back(posA);
      ++i;
      ++j;
    }
    else if (posA < posB)
    {
      ++i;
    }
    else
    {
      ++j;
    }
  }
  return make_unique<coding::SparseCBV>(move(resPos));
}

// Returns true if a bit vector with popCount bits set out of totalBits
// is fit to be represented as a DenseCBV. Note that we do not
// account for possible irregularities in the distribution of bits.
// In particular, we do not break the bit vector into blocks that are
// stored separately although this might turn out to be a good idea.
bool DenseEnough(uint64_t popCount, uint64_t totalBits)
{
  // Settle at 30% for now.
  return popCount * 10 >= totalBits * 3;
}
}  // namespace

namespace coding
{
DenseCBV::DenseCBV(vector<uint64_t> const & setBits)
{
  if (setBits.empty())
  {
    m_bitGroups.resize(0);
    m_popCount = 0;
    return;
  }
  uint64_t maxBit = setBits[0];
  for (size_t i = 1; i < setBits.size(); ++i)
    maxBit = max(maxBit, setBits[i]);
  size_t sz = (maxBit + kBlockSize - 1) / kBlockSize;
  m_bitGroups.resize(sz);
  m_popCount = static_cast<uint32_t>(setBits.size());
  for (uint64_t pos : setBits)
    m_bitGroups[pos / kBlockSize] |= static_cast<uint64_t>(1) << (pos % kBlockSize);
}

// static
unique_ptr<DenseCBV> DenseCBV::BuildFromBitGroups(vector<uint64_t> && bitGroups)
{
  unique_ptr<DenseCBV> cbv(new DenseCBV());
  cbv->m_popCount = 0;
  for (size_t i = 0; i < bitGroups.size(); ++i)
    cbv->m_popCount += bits::PopCount(bitGroups[i]);
  cbv->m_bitGroups = move(bitGroups);
  return cbv;
}

SparseCBV::SparseCBV(vector<uint64_t> const & setBits) : m_positions(setBits)
{
  ASSERT(is_sorted(m_positions.begin(), m_positions.end()), ());
}

SparseCBV::SparseCBV(vector<uint64_t> && setBits) : m_positions(move(setBits))
{
  ASSERT(is_sorted(m_positions.begin(), m_positions.end()), ());
}

uint32_t DenseCBV::PopCount() const { return m_popCount; }

uint32_t SparseCBV::PopCount() const { return m_positions.size(); }

bool DenseCBV::GetBit(uint32_t pos) const
{
  uint64_t bitGroup = GetBitGroup(pos / kBlockSize);
  return ((bitGroup >> (pos % kBlockSize)) & 1) > 0;
}

bool SparseCBV::GetBit(uint32_t pos) const
{
  auto const it = lower_bound(m_positions.begin(), m_positions.end(), pos);
  return it != m_positions.end() && *it == pos;
}

uint64_t DenseCBV::GetBitGroup(size_t i) const
{
  return i < m_bitGroups.size() ? m_bitGroups[i] : 0;
}

uint64_t SparseCBV::Select(size_t i) const
{
  ASSERT_LESS(i, m_positions.size(), ());
  return m_positions[i];
}

CompressedBitVector::StorageStrategy DenseCBV::GetStorageStrategy() const
{
  return CompressedBitVector::StorageStrategy::Dense;
}

CompressedBitVector::StorageStrategy SparseCBV::GetStorageStrategy() const
{
  return CompressedBitVector::StorageStrategy::Sparse;
}

string DebugPrint(CompressedBitVector::StorageStrategy strat)
{
  switch (strat)
  {
    case CompressedBitVector::StorageStrategy::Dense:
      return "Dense";
    case CompressedBitVector::StorageStrategy::Sparse:
      return "Sparse";
  }
}

void DenseCBV::Serialize(Writer & writer) const
{
  uint8_t header = static_cast<uint8_t>(GetStorageStrategy());
  WriteToSink(writer, header);
  WriteToSink(writer, static_cast<uint32_t>(NumBitGroups()));
  for (size_t i = 0; i < NumBitGroups(); ++i)
    WriteToSink(writer, GetBitGroup(i));
}

void SparseCBV::Serialize(Writer & writer) const
{
  uint8_t header = static_cast<uint8_t>(GetStorageStrategy());
  WriteToSink(writer, header);
  WriteToSink(writer, PopCount());
  ForEach([&](uint64_t bitPos)
          {
            WriteToSink(writer, bitPos);
          });
}

// static
unique_ptr<CompressedBitVector> CompressedBitVectorBuilder::FromBitPositions(
    vector<uint64_t> const & setBits)
{
  if (setBits.empty())
    return make_unique<SparseCBV>(setBits);
  uint64_t maxBit = setBits[0];
  for (size_t i = 1; i < setBits.size(); ++i)
    maxBit = max(maxBit, setBits[i]);

  if (DenseEnough(setBits.size(), maxBit))
    return make_unique<DenseCBV>(setBits);

  return make_unique<SparseCBV>(setBits);
}

// static
unique_ptr<CompressedBitVector> CompressedBitVectorBuilder::FromBitGroups(
    vector<uint64_t> && bitGroups)
{
  while (!bitGroups.empty() && bitGroups.back() == 0)
    bitGroups.pop_back();
  if (bitGroups.empty())
    return make_unique<SparseCBV>(bitGroups);

  uint64_t maxBit = kBlockSize * bitGroups.size() - 1;
  uint64_t popCount = 0;
  for (size_t i = 0; i < bitGroups.size(); ++i)
    popCount += bits::PopCount(bitGroups[i]);

  if (DenseEnough(popCount, maxBit))
    return DenseCBV::BuildFromBitGroups(move(bitGroups));

  vector<uint64_t> setBits;
  for (size_t i = 0; i < bitGroups.size(); ++i)
    for (size_t j = 0; j < kBlockSize; ++j)
      if (((bitGroups[i] >> j) & 1) > 0)
        setBits.push_back(kBlockSize * i + j);
  return make_unique<SparseCBV>(setBits);
}

// static
unique_ptr<CompressedBitVector> CompressedBitVector::Intersect(CompressedBitVector const & lhs,
                                                               CompressedBitVector const & rhs)
{
  using strat = CompressedBitVector::StorageStrategy;
  auto const stratA = lhs.GetStorageStrategy();
  auto const stratB = rhs.GetStorageStrategy();
  if (stratA == strat::Dense && stratB == strat::Dense)
  {
    DenseCBV const & a = static_cast<DenseCBV const &>(lhs);
    DenseCBV const & b = static_cast<DenseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }
  if (stratA == strat::Dense && stratB == strat::Sparse)
  {
    DenseCBV const & a = static_cast<DenseCBV const &>(lhs);
    SparseCBV const & b = static_cast<SparseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }
  if (stratA == strat::Sparse && stratB == strat::Dense)
  {
    SparseCBV const & a = static_cast<SparseCBV const &>(lhs);
    DenseCBV const & b = static_cast<DenseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }
  if (stratA == strat::Sparse && stratB == strat::Sparse)
  {
    SparseCBV const & a = static_cast<SparseCBV const &>(lhs);
    SparseCBV const & b = static_cast<SparseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }

  return nullptr;
}
}  // namespace coding
