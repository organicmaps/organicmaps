#include "testing/testing.hpp"

#include "search/house_to_street_table.hpp"

#include "indexer/data_header.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/files_container.hpp"
#include "coding/map_uint32_to_val.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/varint.hpp"

#include "base/checked_cast.hpp"

#include "defines.hpp"

#include <string>
#include <utility>
#include <vector>

namespace house_to_street_table_tests
{
namespace
{
using Entries = std::vector<std::pair<uint32_t, uint32_t>>;

template <class Fn>
void WriteMwm(std::string const & path, Fn && writeTable)
{
  FilesContainerW container(path);
  version::WriteVersion(*container.GetWriter(VERSION_FILE_TAG), 0);

  feature::DataHeader header;
  int constexpr kScales[] = {10, 12, 14, 17};
  header.SetScales(kScales);
  header.SetType(feature::DataHeader::MapType::Country);
  header.Save(*container.GetWriter(HEADER_FILE_TAG));

  for (auto const * tag : {FEATURE2STREET_FILE_TAG, FEATURE2PLACE_FILE_TAG})
  {
    std::vector<uint8_t> buffer;
    MemWriter writer(buffer);
    writeTable(writer);
    container.Write(buffer, tag);
  }
}

void TestEntries(std::string const & path, Entries const & entries, std::vector<uint32_t> const & missingKeys)
{
  MwmValue value(platform::LocalCountryFile::MakeTemporary(path));
  for (auto const & table : {search::LoadHouseToStreetTable(value), search::LoadHouseToPlaceTable(value)})
  {
    for (auto const & [key, expected] : entries)
    {
      auto const result = table->Get(key);
      TEST(result, (key));
      if (!result)
        continue;
      TEST_EQUAL(result->m_streetId, expected, (key));
      TEST(result->m_type == HouseToStreetTable::StreetIdType::FeatureId, (key));
    }
    for (auto const key : missingKeys)
      TEST(!table->Get(key), (key));
  }
}
}  // namespace

UNIT_TEST(HouseToStreetTable_LocalIds)
{
  Entries entries;
  for (uint32_t i = 0; i < 130; ++i)
    entries.emplace_back(5000 + 3 * i, 200000 + (7 * i + 5) % 13);

  platform::tests_support::ScopedFile file("house_to_street_local_ids.mwm",
                                           platform::tests_support::ScopedFile::Mode::DoNotCreate);
  WriteMwm(file.GetFullPath(), [&](Writer & writer)
  {
    search::HouseToStreetTableBuilder builder;
    for (auto const & [key, value] : entries)
      builder.Put(key, value);
    builder.Freeze(writer);
  });

  FilesContainerR container(file.GetFullPath());
  for (auto const * tag : {FEATURE2STREET_FILE_TAG, FEATURE2PLACE_FILE_TAG})
  {
    auto reader = container.GetReader(tag);
    ReaderSource source(reader);
    HouseToStreetTable::Header header;
    header.Read(source);
    TEST(header.m_version == HouseToStreetTable::Version::V3, ());
    TEST_EQUAL(header.m_keyOffset, 5000, ());
    TEST_EQUAL(header.m_valueOffset, 200000, ());
  }

  TestEntries(file.GetFullPath(), entries, {0, 4999, 5001, 5388, 300000});
}

UNIT_TEST(HouseToStreetTable_V2)
{
  Entries entries;
  for (uint32_t i = 0; i < 130; ++i)
    entries.emplace_back(5000 + 3 * i, 200000 + (7 * i + 5) % 13);

  platform::tests_support::ScopedFile file("house_to_street_v2.mwm",
                                           platform::tests_support::ScopedFile::Mode::DoNotCreate);
  WriteMwm(file.GetFullPath(), [&](Writer & writer)
  {
    auto const startOffset = writer.Pos();
    HouseToStreetTable::Header header;
    header.m_version = HouseToStreetTable::Version::V2;
    header.Serialize(writer);
    uint64_t bytesWritten = writer.Pos();
    coding::WritePadding(writer, bytesWritten);

    MapUint32ToValueBuilder<uint32_t> builder;
    for (auto const & [key, value] : entries)
      builder.Put(key, value);
    header.m_tableOffset = base::asserted_cast<uint32_t>(writer.Pos() - startOffset);
    builder.Freeze(writer, [](auto & sink, auto begin, auto end)
    {
      CHECK(begin != end, ());
      WriteVarUint(sink, *begin);
      for (auto it = begin + 1; it != end; ++it)
        WriteVarInt(sink, base::asserted_cast<int32_t>(*it) - base::asserted_cast<int32_t>(*(it - 1)));
    });
    header.m_tableSize = base::asserted_cast<uint32_t>(writer.Pos() - startOffset - header.m_tableOffset);
    auto const endOffset = writer.Pos();
    writer.Seek(startOffset);
    header.Serialize(writer);
    writer.Seek(endOffset);
  });

  FilesContainerR container(file.GetFullPath());
  for (auto const * tag : {FEATURE2STREET_FILE_TAG, FEATURE2PLACE_FILE_TAG})
  {
    auto reader = container.GetReader(tag);
    ReaderSource source(reader);
    HouseToStreetTable::Header header;
    header.Read(source);
    TEST(header.m_version == HouseToStreetTable::Version::V2, ());
    TEST_EQUAL(header.m_keyOffset, 0, ());
    TEST_EQUAL(header.m_valueOffset, 0, ());
  }
  TestEntries(file.GetFullPath(), entries, {0, 4999, 5001, 5388, 300000});
}

UNIT_TEST(HouseToStreetTable_ZeroIds)
{
  Entries const entries = {{0, 0}, {3, 2}, {9, 0}};
  platform::tests_support::ScopedFile file("house_to_street_zero_ids.mwm",
                                           platform::tests_support::ScopedFile::Mode::DoNotCreate);
  WriteMwm(file.GetFullPath(), [&](Writer & writer)
  {
    search::HouseToStreetTableBuilder builder;
    for (auto const & [key, value] : entries)
      builder.Put(key, value);
    builder.Freeze(writer);
  });
  TestEntries(file.GetFullPath(), entries, {1, 2, 4, 10});
}

UNIT_TEST(HouseToStreetTable_Empty)
{
  platform::tests_support::ScopedFile file("house_to_street_empty.mwm",
                                           platform::tests_support::ScopedFile::Mode::DoNotCreate);
  WriteMwm(file.GetFullPath(), [](Writer & writer) { search::HouseToStreetTableBuilder().Freeze(writer); });
  TestEntries(file.GetFullPath(), {}, {0, 1, 5000});
}
}  // namespace house_to_street_table_tests
