#include "testing/testing.hpp"

#include "indexer/metaline_data.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include <limits>

namespace
{
using namespace feature::metalines;
std::vector<uint8_t> MakeMetalines(uint8_t version)
{
  std::vector<uint8_t> bytes;
  MemWriter writer(bytes);
  WriteToSink(writer, version);
  WriteVarUint(writer, uint32_t{2});
  for (int32_t id : {7, 101})
  {
    WriteVarUint(writer, uint32_t{2});
    for (int32_t fid : {id, -id - 1})
    {
      WriteVarInt(writer, fid);
      if (version == 2)
        rw::WritePOD(writer, float{1.25});
      else if (version == 3)
        rw::WritePOD(writer, double{1.25});
    }
  }
  return bytes;
}
}  // namespace

UNIT_TEST(MetalineData_Versions)
{
  for (uint8_t version : {1, 2, 3})
  {
    auto const bytes = MakeMetalines(version);
    MemReader reader(bytes.data(), bytes.size());
    ReaderSource src(reader);
    MwmSet::MwmId const mwmId;
    auto const data = ReadMetalines(src, mwmId);
    TEST_EQUAL(src.Size(), 0, (version));
    TEST_EQUAL(data.size(), 2, (version));
    for (size_t i = 0; i < data.size(); ++i)
    {
      uint32_t const id = i == 0 ? 7 : 101;
      TEST_EQUAL(data[i].m_features.size(), 2, (version));
      TEST_EQUAL(data[i].m_features[0].m_index, id, (version));
      TEST_EQUAL(data[i].m_features[1].m_index, id + 1, (version));
      TEST_EQUAL(data[i].m_reversed.count(FeatureID(mwmId, id)), 0, (version));
      TEST_EQUAL(data[i].m_reversed.count(FeatureID(mwmId, id + 1)), 1, (version));
      if (version == 1)
        TEST(data[i].m_dashPhaseOffsets.empty(), ());
      else
        TEST_EQUAL(data[i].m_dashPhaseOffsets, (std::vector<double>{1.25, 1.25}), (version));
    }
  }
}

UNIT_TEST(MetalineData_UnknownVersion)
{
  for (uint8_t version : {0, 4, 255})
  {
    auto const bytes = MakeMetalines(version);
    MemReader reader(bytes.data(), bytes.size());
    ReaderSource src(reader);
    TEST(ReadMetalines(src, MwmSet::MwmId{}).empty(), (version));
  }
}

UNIT_TEST(MetalineData_TruncatedOffsets)
{
  for (uint8_t version : {2, 3})
  {
    auto bytes = MakeMetalines(version);
    bytes.pop_back();
    MemReaderWithExceptions reader(bytes.data(), bytes.size());
    ReaderSource src(reader);
    bool threw = false;
    try
    {
      ReadMetalines(src, MwmSet::MwmId{});
    }
    catch (Reader::Exception const &)
    {
      threw = true;
    }
    TEST(threw, (version));
  }
}

UNIT_TEST(MetalineData_OptionalOffsets)
{
  auto const legacy = MakeMetalines(1);
  MemReader reader(legacy.data(), legacy.size());
  ReaderSource src(reader);
  auto const base = ReadMetalines(src, MwmSet::MwmId{});
  auto bytes = MakeMetalines(3);
  auto model = base;
  MemReaderWithExceptions phasesReader(bytes.data(), bytes.size());
  ReaderSource phases(phasesReader);
  TEST(TryReadMetalineOffsets(phases, MwmSet::MwmId{}, model), ());
  for (size_t i = 0; i < base.size(); ++i)
  {
    TEST_EQUAL(model[i].m_features, base[i].m_features, ());
    TEST_EQUAL(model[i].m_reversed, base[i].m_reversed, ());
    TEST_EQUAL(model[i].m_dashPhaseOffsets, (std::vector<double>{1.25, 1.25}), ());
  }

  for (bool truncate : {false, true})
  {
    auto damaged = bytes;
    if (truncate)
      damaged.pop_back();
    else
      damaged.push_back(0);
    model = base;
    MemReaderWithExceptions damagedReader(damaged.data(), damaged.size());
    ReaderSource damagedSrc(damagedReader);
    TEST(!TryReadMetalineOffsets(damagedSrc, MwmSet::MwmId{}, model), (truncate));
    for (auto const & chain : model)
      TEST(chain.m_dashPhaseOffsets.empty(), (truncate));
  }
}

UNIT_TEST(MetalineData_StaleOffsetsAreAtomic)
{
  auto const bytes = MakeMetalines(3);
  MemReader reader(bytes.data(), bytes.size());
  ReaderSource src(reader);
  auto const original = ReadMetalines(src, MwmSet::MwmId{});
  for (int mismatch = 0; mismatch < 4; ++mismatch)
  {
    auto model = original;
    for (auto & chain : model)
      chain.m_dashPhaseOffsets.clear();
    auto stale = original;
    if (mismatch == 0)
      stale.back().m_features.back().m_index += 1;
    else if (mismatch == 1)
      stale.back().m_reversed.clear();
    else if (mismatch == 2)
      stale.back().m_dashPhaseOffsets.pop_back();
    else
      stale.pop_back();
    TEST(!ApplyMetalineOffsets(model, std::move(stale)), (mismatch));
    for (auto const & chain : model)
      TEST(chain.m_dashPhaseOffsets.empty(), (mismatch));
  }
}

UNIT_TEST(MetalineData_SignedFeatureBoundaries)
{
  for (uint8_t version : {1, 2, 3})
  {
    std::vector<uint8_t> bytes;
    MemWriter writer(bytes);
    WriteToSink(writer, version);
    WriteVarUint(writer, uint32_t{1});
    WriteVarUint(writer, uint32_t{3});
    for (int32_t const id : {0, std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::min()})
    {
      WriteVarInt(writer, id);
      if (version == 2)
        rw::WritePOD(writer, float{0});
      else if (version == 3)
        rw::WritePOD(writer, double{0});
    }
    MemReader reader(bytes.data(), bytes.size());
    ReaderSource src(reader);
    MwmSet::MwmId const mwmId;
    auto const model = ReadMetalines(src, mwmId);
    TEST_EQUAL(src.Size(), 0, (version));
    TEST_EQUAL(model.size(), 1, (version));
    auto const & chain = model.front();
    TEST_EQUAL(chain.m_features.size(), 3, (version));
    TEST_EQUAL(chain.m_features[0].m_index, uint32_t{0}, (version));
    TEST_EQUAL(chain.m_features[1].m_index, uint32_t{2147483647}, (version));
    TEST_EQUAL(chain.m_features[2].m_index, uint32_t{2147483648}, (version));
    // Keep the legacy signed-ID convention, including its treatment of zero.
    TEST_EQUAL(chain.m_reversed.count(chain.m_features[0]), 1, (version));
    TEST_EQUAL(chain.m_reversed.count(chain.m_features[1]), 0, (version));
    TEST_EQUAL(chain.m_reversed.count(chain.m_features[2]), 1, (version));
  }
}

UNIT_TEST(MetalineData_OptionalFailurePreservesExistingPhases)
{
  auto const bytes = MakeMetalines(3);
  MemReader reader(bytes.data(), bytes.size());
  ReaderSource src(reader);
  auto const original = ReadMetalines(src, MwmSet::MwmId{});
  // Every incomplete prefix must leave the entire authoritative model intact,
  // including phases that were already populated by a previous successful read.
  for (size_t length = 0; length < bytes.size(); ++length)
  {
    auto model = original;
    MemReaderWithExceptions truncated(bytes.data(), length);
    ReaderSource partial(truncated);
    TEST(!TryReadMetalineOffsets(partial, MwmSet::MwmId{}, model), (length));
    TEST_EQUAL(model.size(), original.size(), (length));
    for (size_t i = 0; i < model.size(); ++i)
    {
      TEST_EQUAL(model[i].m_features, original[i].m_features, (length, i));
      TEST_EQUAL(model[i].m_reversed, original[i].m_reversed, (length, i));
      TEST_EQUAL(model[i].m_dashPhaseOffsets, original[i].m_dashPhaseOffsets, (length, i));
    }
  }
}
