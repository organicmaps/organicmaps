#include "indexer/rank_table.hpp"

#include "indexer/classificator.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/endianness.hpp"
#include "coding/files_container.hpp"
#include "coding/file_writer.hpp"
#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/simple_dense_coding.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <utility>

#include "defines.hpp"

using namespace std;

namespace search
{
namespace
{
uint64_t const kVersionOffset = 0;
uint64_t const kFlagsOffset = 1;
uint64_t const kHeaderSize = 8;

enum class CheckResult
{
  CorruptedHeader,
  EndiannessMismatch,
  EndiannessMatch
};

template <typename TReader>
CheckResult CheckEndianness(TReader && reader)
{
  if (reader.Size() < kHeaderSize)
    return CheckResult::CorruptedHeader;
  uint8_t flags;
  reader.Read(kFlagsOffset, &flags, sizeof(flags));
  bool const isHostBigEndian = IsBigEndianMacroBased();
  bool const isDataBigEndian = flags & 1;
  if (isHostBigEndian != isDataBigEndian)
    return CheckResult::EndiannessMismatch;
  return CheckResult::EndiannessMatch;
}

unique_ptr<CopiedMemoryRegion> GetMemoryRegionForTag(FilesContainerR const & rcont,
                                                     FilesContainerBase::Tag const & tag)
{
  if (!rcont.IsExist(tag))
    return unique_ptr<CopiedMemoryRegion>();
  FilesContainerR::TReader reader = rcont.GetReader(tag);
  vector<uint8_t> buffer(static_cast<size_t>(reader.Size()));
  reader.Read(0, buffer.data(), buffer.size());
  return make_unique<CopiedMemoryRegion>(move(buffer));
}

unique_ptr<MappedMemoryRegion> GetMemoryRegionForTag(FilesMappingContainer const & mcont,
                                                     FilesContainerBase::Tag const & tag)
{
  if (!mcont.IsExist(tag))
    return unique_ptr<MappedMemoryRegion>();
  FilesMappingContainer::Handle handle = mcont.Map(tag);
  return make_unique<MappedMemoryRegion>(move(handle));
}

// RankTable version 1, uses simple dense coding to store and access
// array of ranks.
class RankTableV0 : public RankTable
{
public:
  RankTableV0() = default;

  explicit RankTableV0(vector<uint8_t> const & ranks) : m_coding(ranks) {}

  // RankTable overrides:
  uint8_t Get(uint64_t i) const override
  {
    // i can be greater than Size() for features created by user in the Editor.
    // May be there is a better way to inject this code. Without this check search engine crashes here.
    //ASSERT_LESS(i, Size(), ());
    if (i >= Size())
      return 0;

    return m_coding.Get(i);
  }
  uint64_t Size() const override { return m_coding.Size(); }
  RankTable::Version GetVersion() const override { return V0; }
  void Serialize(Writer & writer, bool preserveHostEndianness) override
  {
    static uint64_t const padding = 0;

    uint8_t const version = GetVersion();
    uint8_t const flags = preserveHostEndianness ? IsBigEndianMacroBased() : !IsBigEndianMacroBased();
    writer.Write(&version, sizeof(version));
    writer.Write(&flags, sizeof(flags));
    writer.Write(&padding, 6);
    if (preserveHostEndianness)
      Freeze(m_coding, writer, "SimpleDenseCoding");
    else
      ReverseFreeze(m_coding, writer, "SimpleDenseCoding");
  }

  // Loads RankTableV0 from a raw memory region.
  static unique_ptr<RankTableV0> Load(unique_ptr<MappedMemoryRegion> && region)
  {
    if (!region.get())
      return unique_ptr<RankTableV0>();

    auto const result =
        CheckEndianness(MemReader(region->ImmutableData(), static_cast<size_t>(region->Size())));
    if (result != CheckResult::EndiannessMatch)
      return unique_ptr<RankTableV0>();

    auto table = make_unique<RankTableV0>();
    coding::Map(table->m_coding, region->ImmutableData() + kHeaderSize, "SimpleDenseCoding");
    table->m_region = move(region);
    return table;
  }

  // Loads RankTableV0 from a raw memory region. Modifies region in
  // the case of endianness mismatch.
  static unique_ptr<RankTableV0> Load(unique_ptr<CopiedMemoryRegion> && region)
  {
    if (!region.get())
      return unique_ptr<RankTableV0>();

    unique_ptr<RankTableV0> table;
    switch (
        CheckEndianness(MemReader(region->ImmutableData(), static_cast<size_t>(region->Size()))))
    {
    case CheckResult::CorruptedHeader:
      break;
    case CheckResult::EndiannessMismatch:
      table.reset(new RankTableV0());
      coding::ReverseMap(table->m_coding, region->MutableData() + kHeaderSize, "SimpleDenseCoding");
      table->m_region = move(region);
      break;
    case CheckResult::EndiannessMatch:
      table.reset(new RankTableV0());
      coding::Map(table->m_coding, region->ImmutableData() + kHeaderSize, "SimpleDenseCoding");
      table->m_region = move(region);
      break;
    }
    return table;
  }

private:
  unique_ptr<MemoryRegion> m_region;
  coding::SimpleDenseCoding m_coding;
};

// Following two functions create a rank section and serialize |table|
// to it. If there was an old section with ranks, these functions
// overwrite it.
void SerializeRankTable(RankTable & table, FilesContainerW & wcont, string const & sectionName)
{
  if (wcont.IsExist(sectionName))
    wcont.DeleteSection(sectionName);
  ASSERT(!wcont.IsExist(sectionName), ());

  vector<char> buffer;
  {
    MemWriter<decltype(buffer)> writer(buffer);
    table.Serialize(writer, true /* preserveHostEndianness */);
  }

  wcont.Write(buffer, sectionName);
  wcont.Finish();
}

void SerializeRankTable(RankTable & table, string const & mapPath, string const & sectionName)
{
  FilesContainerW wcont(mapPath, FileWriter::OP_WRITE_EXISTING);
  SerializeRankTable(table, wcont, sectionName);
}

// Deserializes rank table from a rank section. Returns null when it's
// not possible to load a rank table (no rank section, corrupted
// header, endianness mismatch for a mapped mwm).
template <typename TRegion>
unique_ptr<RankTable> LoadRankTable(unique_ptr<TRegion> && region)
{
  if (!region || !region->ImmutableData())
    return {};

  if (region->Size() < kHeaderSize)
  {
    LOG(LERROR, ("Invalid RankTable format."));
    return {};
  }

  RankTable::Version const version = static_cast<RankTable::Version>(region->ImmutableData()[kVersionOffset]);
  if (version == RankTable::V0)
    return RankTableV0::Load(move(region));

  LOG(LERROR, ("Wrong rank table version."));
  return {};
}

uint8_t CalcEventRank(FeatureType & /*ft*/)
{
  //TODO: add custom event processing, i.e. fc2018.
  return 0;
}

uint8_t CalcTransportRank(FeatureType & ft)
{
  uint8_t const kTransportRank = 2;
  if (ftypes::IsRailwayStationChecker::Instance()(ft) ||
      ftypes::IsSubwayStationChecker::Instance()(ft) || ftypes::IsAirportChecker::Instance()(ft))
  {
    return kTransportRank;
  }

  return 0;
}

// Calculates search rank for a feature.
uint8_t CalcSearchRank(FeatureType & ft)
{
  auto const eventRank = CalcEventRank(ft);
  auto const transportRank = CalcTransportRank(ft);
  auto const populationRank = feature::PopulationToRank(ftypes::GetPopulation(ft));

  return base::Clamp(eventRank + transportRank + populationRank, 0,
                     static_cast<int>(numeric_limits<uint8_t>::max()));
}

// Creates rank table if it does not exists in |rcont| or has wrong
// endianness. Otherwise (table exists and has correct format) returns
// null.
unique_ptr<RankTable> CreateSearchRankTableIfNotExists(FilesContainerR & rcont)
{
  unique_ptr<RankTable> table;

  if (rcont.IsExist(SEARCH_RANKS_FILE_TAG))
  {
    switch (CheckEndianness(rcont.GetReader(SEARCH_RANKS_FILE_TAG)))
    {
    case CheckResult::CorruptedHeader:
    {
      // Worst case - we need to create rank table from scratch.
      break;
    }
    case CheckResult::EndiannessMismatch:
    {
      // Try to copy whole serialized data and instantiate table via
      // reverse mapping.
      auto region = GetMemoryRegionForTag(rcont, SEARCH_RANKS_FILE_TAG);
      table = LoadRankTable(move(region));
      break;
    }
    case CheckResult::EndiannessMatch:
    {
      // Table exists and has proper format. Nothing to do here.
      return unique_ptr<RankTable>();
    }
    }
  }

  // Table doesn't exist or has wrong format. It's better to create it
  // from scratch.
  if (!table)
  {
    vector<uint8_t> ranks;
    SearchRankTableBuilder::CalcSearchRanks(rcont, ranks);
    table = make_unique<RankTableV0>(ranks);
  }

  return table;
}
}  // namespace

// static
unique_ptr<RankTable> RankTable::Load(FilesContainerR const & rcont, string const & sectionName)
{
  return LoadRankTable(GetMemoryRegionForTag(rcont, sectionName));
}

// static
unique_ptr<RankTable> RankTable::Load(FilesMappingContainer const & mcont, string const & sectionName)
{
  return LoadRankTable(GetMemoryRegionForTag(mcont, sectionName));
}

// static
void SearchRankTableBuilder::CalcSearchRanks(FilesContainerR & rcont, vector<uint8_t> & ranks)
{
  feature::DataHeader header(rcont);
  FeaturesVector featuresVector(rcont, header, nullptr, nullptr);

  featuresVector.ForEach(
      [&ranks](FeatureType & ft, uint32_t /* index */) { ranks.push_back(CalcSearchRank(ft)); });
}

// static
bool SearchRankTableBuilder::CreateIfNotExists(platform::LocalCountryFile const & localFile) noexcept
{
  try
  {
    string mapPath;

    unique_ptr<RankTable> table;
    {
      ModelReaderPtr reader = platform::GetCountryReader(localFile, MapFileType::Map);
      if (!reader.GetPtr())
        return false;

      mapPath = reader.GetName();

      FilesContainerR rcont(reader);
      table = CreateSearchRankTableIfNotExists(rcont);
    }

    if (table)
      SerializeRankTable(*table, mapPath, SEARCH_RANKS_FILE_TAG);

    return true;
  }
  catch (exception & e)
  {
    LOG(LWARNING, ("Can' create rank table for:", localFile, ":", e.what()));
    return false;
  }
}

// static
bool SearchRankTableBuilder::CreateIfNotExists(string const & mapPath) noexcept
{
  try
  {
    unique_ptr<RankTable> table;
    {
      FilesContainerR rcont(mapPath);
      table = CreateSearchRankTableIfNotExists(rcont);
    }

    if (table)
      SerializeRankTable(*table, mapPath, SEARCH_RANKS_FILE_TAG);

    return true;
  }
  catch (exception & e)
  {
    LOG(LWARNING, ("Can' create rank table for:", mapPath, ":", e.what()));
    return false;
  }
}

// static
void RankTableBuilder::Create(vector<uint8_t> const & ranks, FilesContainerW & wcont,
                              string const & sectionName)
{
  RankTableV0 table(ranks);
  SerializeRankTable(table, wcont, sectionName);
}
}  // namespace search
