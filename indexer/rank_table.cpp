#include "indexer/rank_table.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/types_skipper.hpp"

#include "platform/local_country_file.hpp"

#include "coding/endianness.hpp"
#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/simple_dense_coding.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include "std/utility.hpp"

#include "defines.hpp"

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
  bool const isHostBigEndian = IsBigEndian();
  bool const isDataBigEndian = flags & 1;
  if (isHostBigEndian != isDataBigEndian)
    return CheckResult::EndiannessMismatch;
  return CheckResult::EndiannessMatch;
}

class MemoryRegion
{
public:
  virtual ~MemoryRegion() = default;

  virtual uint64_t Size() const = 0;
  virtual uint8_t const * ImmutableData() const = 0;
};

class MappedMemoryRegion : public MemoryRegion
{
public:
  MappedMemoryRegion(FilesMappingContainer::Handle && handle) : m_handle(move(handle)) {}

  // MemoryRegion overrides:
  uint64_t Size() const override { return m_handle.GetSize(); }
  uint8_t const * ImmutableData() const override { return m_handle.GetData<uint8_t>(); }

private:
  FilesMappingContainer::Handle m_handle;

  DISALLOW_COPY(MappedMemoryRegion);
};

class CopiedMemoryRegion : public MemoryRegion
{
public:
  CopiedMemoryRegion(vector<uint8_t> && buffer) : m_buffer(move(buffer)) {}

  // MemoryRegion overrides:
  uint64_t Size() const override { return m_buffer.size(); }
  uint8_t const * ImmutableData() const override { return m_buffer.data(); }

  inline uint8_t * MutableData() { return m_buffer.data(); }

private:
  vector<uint8_t> m_buffer;

  DISALLOW_COPY(CopiedMemoryRegion);
};

unique_ptr<CopiedMemoryRegion> GetMemoryRegionForTag(FilesContainerR const & rcont,
                                                     FilesContainerBase::Tag const & tag)
{
  if (!rcont.IsExist(tag))
    return unique_ptr<CopiedMemoryRegion>();
  FilesContainerR::ReaderT reader = rcont.GetReader(tag);
  vector<uint8_t> buffer(reader.Size());
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

  RankTableV0(vector<uint8_t> const & ranks) : m_coding(ranks) {}

  // RankTable overrides:
  uint8_t Get(uint64_t i) const override { return m_coding.Get(i); }
  uint64_t Size() const override { return m_coding.Size(); }
  RankTable::Version GetVersion() const override { return V0; }
  void Serialize(Writer & writer) override
  {
    static uint64_t const padding = 0;

    uint8_t const version = GetVersion();
    uint8_t const flags = IsBigEndian();
    writer.Write(&version, sizeof(version));
    writer.Write(&flags, sizeof(flags));
    writer.Write(&padding, 6);
    Freeze(m_coding, writer, "SimpleDenseCoding");
  }

  // Loads RankTableV0 from a raw memory region.
  static unique_ptr<RankTableV0> Load(unique_ptr<MappedMemoryRegion> && region)
  {
    if (!region.get())
      return unique_ptr<RankTableV0>();

    auto const result = CheckEndianness(MemReader(region->ImmutableData(), region->Size()));
    if (result != CheckResult::EndiannessMatch)
      return unique_ptr<RankTableV0>();

    unique_ptr<RankTableV0> table(new RankTableV0());
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
    switch (CheckEndianness(MemReader(region->ImmutableData(), region->Size())))
    {
      case CheckResult::CorruptedHeader:
        break;
      case CheckResult::EndiannessMismatch:
        table.reset(new RankTableV0());
        coding::ReverseMap(table->m_coding, region->MutableData() + kHeaderSize,
                           "SimpleDenseCoding");
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

// Creates a rank section and serializes |table| to it.
void SerializeRankTable(RankTable & table, FilesContainerW & wcont)
{
  if (wcont.IsExist(RANKS_FILE_TAG))
    wcont.DeleteSection(RANKS_FILE_TAG);
  ASSERT(!wcont.IsExist(RANKS_FILE_TAG), ());

  vector<char> buffer;
  {
    MemWriter<decltype(buffer)> writer(buffer);
    table.Serialize(writer);
  }

  wcont.Write(buffer, RANKS_FILE_TAG);
  wcont.Finish();
}

// Deserializes rank table from a rank section. Returns null when it's
// not possible to load a rank table (no rank section, corrupted
// header, endianness mismatch for a mapped mwm).
template <typename TRegion>
unique_ptr<RankTable> LoadRankTable(unique_ptr<TRegion> && region)
{
  if (!region || !region->ImmutableData() || region->Size() < kHeaderSize)
  {
    LOG(LERROR, ("Invalid RankTable format."));
    return unique_ptr<RankTable>();
  }

  RankTable::Version const version =
      static_cast<RankTable::Version>(region->ImmutableData()[kVersionOffset]);
  switch (version)
  {
    case RankTable::V0:
      return RankTableV0::Load(move(region));
  }
  return unique_ptr<RankTable>();
}

// Calculates search rank for a feature.
uint8_t CalcSearchRank(FeatureType const & ft)
{
  static search::TypesSkipper skipIndex;

  feature::TypesHolder types(ft);
  skipIndex.SkipTypes(types);
  if (types.Empty())
    return 0;

  m2::PointD const center = feature::GetCenter(ft);
  return feature::GetSearchRank(types, center, ft.GetPopulation());
}
}  // namespace

// static
unique_ptr<RankTable> RankTable::Load(FilesContainerR const & rcont)
{
  return LoadRankTable(GetMemoryRegionForTag(rcont, RANKS_FILE_TAG));
}

// static
unique_ptr<RankTable> RankTable::Load(FilesMappingContainer const & mcont)
{
  return LoadRankTable(GetMemoryRegionForTag(mcont, RANKS_FILE_TAG));
}

// static
void RankTableBuilder::CalcSearchRanks(FilesContainerR & rcont, vector<uint8_t> & ranks)
{
  feature::DataHeader header(rcont);
  FeaturesVector featuresVector(rcont, header, nullptr /* features offsets table */);

  featuresVector.ForEach([&ranks](FeatureType const & ft, uint32_t /* index */)
                         {
                           ranks.push_back(CalcSearchRank(ft));
                         });
}

// static
void RankTableBuilder::CreateIfNotExists(platform::LocalCountryFile const & localFile)
{
  string const mapPath = localFile.GetPath(MapOptions::Map);

  unique_ptr<RankTable> table;
  {
    FilesContainerR rcont(mapPath);
    if (rcont.IsExist(RANKS_FILE_TAG))
    {
      switch (CheckEndianness(rcont.GetReader(RANKS_FILE_TAG)))
      {
        case CheckResult::CorruptedHeader:
        {
          // Worst case - we need to create rank table from scratch.
          break;
        }
        case CheckResult::EndiannessMismatch:
        {
          // Try to copy whole serialized data and instantiate table via reverse mapping.
          auto region = GetMemoryRegionForTag(rcont, RANKS_FILE_TAG);
          table = LoadRankTable(move(region));
          break;
        }
        case CheckResult::EndiannessMatch:
        {
          // Table exists and has proper format. Nothing to do here.
          return;
        }
      }
    }

    // Table doesn't exist or has wrong format. It's better to create
    // it from scratch.
    if (!table)
    {
      vector<uint8_t> ranks;
      CalcSearchRanks(rcont, ranks);
      table = make_unique<RankTableV0>(ranks);
    }
  }

  ASSERT(table.get(), ());
  FilesContainerW wcont(mapPath, FileWriter::OP_WRITE_EXISTING);
  SerializeRankTable(*table, wcont);
}

// static
void RankTableBuilder::Create(vector<uint8_t> const & ranks, FilesContainerW & wcont)
{
  RankTableV0 table(ranks);
  SerializeRankTable(table, wcont);
}
}  // namespace search
