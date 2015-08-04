#include "coding/compressed_bit_vector.hpp"
#include "coding/writer.hpp"
#include "coding/write_to_sink.hpp"

#include "std/algorithm.hpp"

namespace
{
unique_ptr<coding::CompressedBitVector> IntersectImpl(coding::DenseCBV const & a,
                                                      coding::DenseCBV const & b)
{
  size_t sizeA = a.NumBitGroups();
  size_t sizeB = b.NumBitGroups();
  vector<uint64_t> resBits;
  for (size_t i = 0; i < min(sizeA, sizeB); ++i)
  {
    uint64_t bitGroup = a.GetBitGroup(i) & b.GetBitGroup(i);
    for (size_t j = 0; j < 64; j++)
      if (((bitGroup >> j) & 1) > 0)
        resBits.push_back(64 * i + j);
  }
  return coding::CompressedBitVectorBuilder::Build(resBits);
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
}  // namespace

namespace coding
{
DenseCBV::DenseCBV(vector<uint64_t> const & setBits)
{
  if (setBits.empty())
  {
    m_bits.resize(0);
    m_popCount = 0;
    return;
  }
  uint64_t maxBit = setBits[0];
  for (size_t i = 1; i < setBits.size(); ++i)
    maxBit = max(maxBit, setBits[i]);
  size_t sz = (maxBit + 64 - 1) / 64;
  m_bits.resize(sz);
  m_popCount = static_cast<uint32_t>(setBits.size());
  for (uint64_t pos : setBits)
    m_bits[pos / 64] |= static_cast<uint64_t>(1) << (pos % 64);
}

uint32_t DenseCBV::PopCount() const { return m_popCount; }

uint32_t SparseCBV::PopCount() const { return m_positions.size(); }

bool DenseCBV::GetBit(uint32_t pos) const
{
  uint64_t bitGroup = GetBitGroup(pos / 64);
  return ((bitGroup >> (pos % 64)) & 1) > 0;
}

bool SparseCBV::GetBit(uint32_t pos) const
{
  auto it = lower_bound(m_positions.begin(), m_positions.end(), pos);
  return it != m_positions.end() && *it == pos;
}

CompressedBitVector::StorageStrategy DenseCBV::GetStorageStrategy() const
{
  return CompressedBitVector::StorageStrategy::Dense;
}

CompressedBitVector::StorageStrategy SparseCBV::GetStorageStrategy() const
{
  return CompressedBitVector::StorageStrategy::Sparse;
}

template <typename F>
void DenseCBV::ForEach(F && f) const
{
  for (size_t i = 0; i < m_bits.size(); ++i)
    for (size_t j = 0; j < 64; ++j)
      if (((m_bits[i] >> j) & 1) > 0)
        f(64 * i + j);
}

template <typename F>
void SparseCBV::ForEach(F && f) const
{
  for (size_t i = 0; i < m_positions.size(); ++i)
    f(m_positions[i]);
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
unique_ptr<CompressedBitVector> CompressedBitVectorBuilder::Build(vector<uint64_t> const & setBits)
{
  if (setBits.empty())
    return make_unique<SparseCBV>(setBits);
  uint64_t maxBit = setBits[0];
  for (size_t i = 1; i < setBits.size(); ++i)
    maxBit = max(maxBit, setBits[i]);
  // 30% occupied is dense enough
  if (10 * setBits.size() >= 3 * maxBit)
    return make_unique<DenseCBV>(setBits);
  return make_unique<SparseCBV>(setBits);
}

// static
unique_ptr<CompressedBitVector> CompressedBitVector::Intersect(CompressedBitVector const & lhs,
                                                               CompressedBitVector const & rhs)
{
  auto stratA = lhs.GetStorageStrategy();
  auto stratB = rhs.GetStorageStrategy();
  auto stratDense = CompressedBitVector::StorageStrategy::Dense;
  auto stratSparse = CompressedBitVector::StorageStrategy::Sparse;
  if (stratA == stratDense && stratB == stratDense)
  {
    DenseCBV const & a = static_cast<DenseCBV const &>(lhs);
    DenseCBV const & b = static_cast<DenseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }
  if (stratA == stratDense && stratB == stratSparse)
  {
    DenseCBV const & a = static_cast<DenseCBV const &>(lhs);
    SparseCBV const & b = static_cast<SparseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }
  if (stratA == stratSparse && stratB == stratDense)
  {
    SparseCBV const & a = static_cast<SparseCBV const &>(lhs);
    DenseCBV const & b = static_cast<DenseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }
  if (stratA == stratSparse && stratB == stratSparse)
  {
    SparseCBV const & a = static_cast<SparseCBV const &>(lhs);
    SparseCBV const & b = static_cast<SparseCBV const &>(rhs);
    return IntersectImpl(a, b);
  }

  return nullptr;
}
}  // namespace coding
