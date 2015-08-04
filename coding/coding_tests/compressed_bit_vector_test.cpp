#include "testing/testing.hpp"

#include "coding/compressed_bit_vector.hpp"
#include "coding/writer.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"

namespace
{
void CheckIntersection(vector<uint64_t> & setBits1, vector<uint64_t> & setBits2,
                       unique_ptr<coding::CompressedBitVector> const & cbv)
{
  TEST(cbv.get(), ());
  vector<uint64_t> expected;
  sort(setBits1.begin(), setBits1.end());
  sort(setBits2.begin(), setBits2.end());
  set_intersection(setBits1.begin(), setBits1.end(), setBits2.begin(), setBits2.end(),
                   back_inserter(expected));
  TEST_EQUAL(expected.size(), cbv->PopCount(), ());
  for (size_t i = 0; i < expected.size(); ++i)
    TEST(cbv->GetBit(expected[i]), ());
}
}  // namespace

UNIT_TEST(CompressedBitVector_Smoke) {}

UNIT_TEST(CompressedBitVector_Intersect1)
{
  size_t const n = 100;
  vector<uint64_t> setBits1;
  vector<uint64_t> setBits2;
  for (size_t i = 0; i < n; ++i)
  {
    if (i > 0)
      setBits1.push_back(i);
    if (i + 1 < n)
      setBits2.push_back(i);
  }
  auto cbv1 = coding::CompressedBitVectorBuilder::Build(setBits1);
  auto cbv2 = coding::CompressedBitVectorBuilder::Build(setBits2);
  TEST(cbv1.get(), ());
  TEST(cbv2.get(), ());
  auto cbv3 = coding::CompressedBitVector::Intersect(*cbv1, *cbv2);
  TEST_EQUAL(coding::CompressedBitVector::StorageStrategy::Dense, cbv3->GetStorageStrategy(), ());
  CheckIntersection(setBits1, setBits2, cbv3);
}

UNIT_TEST(CompressedBitVector_Intersect2)
{
  size_t const n = 100;
  vector<uint64_t> setBits1;
  vector<uint64_t> setBits2;
  for (size_t i = 0; i < n; ++i)
  {
    if (i <= n / 2)
      setBits1.push_back(i);
    if (i >= n / 2)
      setBits2.push_back(i);
  }
  auto cbv1 = coding::CompressedBitVectorBuilder::Build(setBits1);
  auto cbv2 = coding::CompressedBitVectorBuilder::Build(setBits2);
  TEST(cbv1.get(), ());
  TEST(cbv2.get(), ());
  auto cbv3 = coding::CompressedBitVector::Intersect(*cbv1, *cbv2);
  TEST_EQUAL(coding::CompressedBitVector::StorageStrategy::Sparse, cbv3->GetStorageStrategy(), ());
  CheckIntersection(setBits1, setBits2, cbv3);
}

UNIT_TEST(CompressedBitVector_Intersect3)
{
  size_t const n = 100;
  vector<uint64_t> setBits1;
  vector<uint64_t> setBits2;
  for (size_t i = 0; i < n; ++i)
  {
    if (i % 2 == 0)
      setBits1.push_back(i);
    if (i % 3 == 0)
      setBits2.push_back(i);
  }
  auto cbv1 = coding::CompressedBitVectorBuilder::Build(setBits1);
  auto cbv2 = coding::CompressedBitVectorBuilder::Build(setBits2);
  TEST(cbv1.get(), ());
  TEST(cbv2.get(), ());
  auto cbv3 = coding::CompressedBitVector::Intersect(*cbv1, *cbv2);
  TEST_EQUAL(coding::CompressedBitVector::StorageStrategy::Sparse, cbv3->GetStorageStrategy(), ());
  for (size_t i = 0; i < n; ++i)
  {
    bool expected = i % 6 == 0;
    TEST_EQUAL(expected, cbv3->GetBit(i), (i));
  }
}

UNIT_TEST(CompressedBitVector_Intersect4)
{
  size_t const n = 1000;
  vector<uint64_t> setBits1;
  vector<uint64_t> setBits2;
  for (size_t i = 0; i < n; ++i)
  {
    if (i % 100 == 0)
      setBits1.push_back(i);
    if (i % 150 == 0)
      setBits2.push_back(i);
  }
  auto cbv1 = coding::CompressedBitVectorBuilder::Build(setBits1);
  auto cbv2 = coding::CompressedBitVectorBuilder::Build(setBits2);
  TEST(cbv1.get(), ());
  TEST(cbv2.get(), ());
  auto cbv3 = coding::CompressedBitVector::Intersect(*cbv1, *cbv2);
  TEST_EQUAL(coding::CompressedBitVector::StorageStrategy::Sparse, cbv3->GetStorageStrategy(), ());
  for (size_t i = 0; i < n; ++i)
  {
    bool expected = i % 300 == 0;
    TEST_EQUAL(expected, cbv3->GetBit(i), (i));
  }
}

UNIT_TEST(CompressedBitVector_SerializationDense)
{
  int const n = 100;
  vector<uint64_t> setBits;
  for (size_t i = 0; i < n; ++i)
    setBits.push_back(i);
  vector<uint8_t> buf;
  {
    MemWriter<vector<uint8_t>> writer(buf);
    auto cbv = coding::CompressedBitVectorBuilder::Build(setBits);
    cbv->Serialize(writer);
  }
  MemReader reader(buf.data(), buf.size());
  ReaderSource<MemReader> src(reader);
  auto cbv = coding::CompressedBitVectorBuilder::Deserialize(src);
  TEST(cbv.get(), ());
  TEST_EQUAL(coding::CompressedBitVector::StorageStrategy::Dense, cbv->GetStorageStrategy(), ());
  TEST_EQUAL(setBits.size(), cbv->PopCount(), ());
  for (size_t i = 0; i < setBits.size(); ++i)
    TEST(cbv->GetBit(setBits[i]), ());
}

UNIT_TEST(CompressedBitVector_SerializationSparse)
{
  int const n = 100;
  vector<uint64_t> setBits;
  for (size_t i = 0; i < n; ++i)
  {
    if (i % 10 == 0)
      setBits.push_back(i);
  }
  vector<uint8_t> buf;
  {
    MemWriter<vector<uint8_t>> writer(buf);
    auto cbv = coding::CompressedBitVectorBuilder::Build(setBits);
    cbv->Serialize(writer);
  }
  MemReader reader(buf.data(), buf.size());
  ReaderSource<MemReader> src(reader);
  auto cbv = coding::CompressedBitVectorBuilder::Deserialize(src);
  TEST(cbv.get(), ());
  TEST_EQUAL(coding::CompressedBitVector::StorageStrategy::Sparse, cbv->GetStorageStrategy(), ());
  TEST_EQUAL(setBits.size(), cbv->PopCount(), ());
  for (size_t i = 0; i < setBits.size(); ++i)
    TEST(cbv->GetBit(setBits[i]), ());
}
