#include "testing/testing.hpp"

#include "drape_frontend/metaline_data.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace
{
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
    auto const data = df::detail::ReadMetalines(src, mwmId);
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
    TEST(df::detail::ReadMetalines(src, MwmSet::MwmId{}).empty(), (version));
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
      df::detail::ReadMetalines(src, MwmSet::MwmId{});
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
  auto const base = df::detail::ReadMetalines(src, MwmSet::MwmId{});
  auto bytes = MakeMetalines(3);
  auto model = base;
  MemReaderWithExceptions phasesReader(bytes.data(), bytes.size());
  ReaderSource phases(phasesReader);
  TEST(df::detail::TryReadMetalineOffsets(phases, MwmSet::MwmId{}, model), ());
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
    TEST(!df::detail::TryReadMetalineOffsets(damagedSrc, MwmSet::MwmId{}, model), (truncate));
    for (auto const & chain : model)
      TEST(chain.m_dashPhaseOffsets.empty(), (truncate));
  }
}

UNIT_TEST(MetalineData_StaleOffsetsAreAtomic)
{
  auto const bytes = MakeMetalines(3);
  MemReader reader(bytes.data(), bytes.size());
  ReaderSource src(reader);
  auto const original = df::detail::ReadMetalines(src, MwmSet::MwmId{});
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
    TEST(!df::detail::ApplyMetalineOffsets(model, std::move(stale)), (mismatch));
    for (auto const & chain : model)
      TEST(chain.m_dashPhaseOffsets.empty(), (mismatch));
  }
}
