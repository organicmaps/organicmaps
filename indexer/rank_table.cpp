#include "indexer/rank_table.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/features_offsets_table.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/types_skipper.hpp"

#include "platform/local_country_file.hpp"

#include "coding/endianness.hpp"
#include "coding/file_container.hpp"
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
uint64_t const kVersionOffset = 0;
uint64_t const kFlagsOffset = 1;
uint64_t const kHeaderSize = 8;

namespace
{
// Returns true when flags claim that the serialized data has the same
// endianness as a host.
bool SameEndianness(uint8_t flags)
{
  bool const isHostBigEndian = IsBigEndian();
  bool const isDataBigEndian = flags & 1;
  return isHostBigEndian == isDataBigEndian;
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

unique_ptr<CopiedMemoryRegion> GetMemoryRegionForTag(FilesContainerR & rcont,
                                                     FilesContainerBase::Tag const & tag)
{
  if (!rcont.IsExist(tag))
    return unique_ptr<CopiedMemoryRegion>();
  FilesContainerR::ReaderT reader = rcont.GetReader(tag);
  vector<uint8_t> buffer(reader.Size());
  reader.Read(0, buffer.data(), buffer.size());
  return make_unique<CopiedMemoryRegion>(move(buffer));
}

unique_ptr<MappedMemoryRegion> GetMemoryRegionForTag(FilesMappingContainer & mcont,
                                                     FilesContainerBase::Tag const & tag)
{
  if (!mcont.IsExist(tag))
    return unique_ptr<MappedMemoryRegion>();
  FilesMappingContainer::Handle handle = mcont.Map(tag);
  return make_unique<MappedMemoryRegion>(move(handle));
}

class RankTableV1 : public RankTable
{
public:
  RankTableV1() = default;

  RankTableV1(vector<uint8_t> const & ranks) : m_coding(ranks) {}

  // RankTable overrides:
  uint8_t Get(uint64_t i) const override { return m_coding.Get(i); }
  uint64_t Size() const override { return m_coding.Size(); }
  RankTable::Version GetVersion() const override { return V1; }
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

  // Loads rank table v1 from a raw memory region.
  static unique_ptr<RankTableV1> Load(unique_ptr<MappedMemoryRegion> && region)
  {
    if (!region.get() || region->Size() < kHeaderSize)
      return unique_ptr<RankTableV1>();

    uint8_t const flags = region->ImmutableData()[kFlagsOffset];
    if (!SameEndianness(flags))
      return unique_ptr<RankTableV1>();

    unique_ptr<RankTableV1> table(new RankTableV1());
    coding::Map(table->m_coding, region->ImmutableData() + kHeaderSize, "SimpleDenseCoding");
    table->m_region = move(region);
    return table;
  }

  // Loads rank table v1 from a raw memory region. Modifies region in
  // the case of endianness mismatch.
  static unique_ptr<RankTableV1> Load(unique_ptr<CopiedMemoryRegion> && region)
  {
    if (!region.get() || region->Size() < kHeaderSize)
      return unique_ptr<RankTableV1>();

    unique_ptr<RankTableV1> table(new RankTableV1());
    uint8_t const flags = region->ImmutableData()[kFlagsOffset];
    if (SameEndianness(flags))
      coding::Map(table->m_coding, region->ImmutableData() + kHeaderSize, "SimpleDenseCoding");
    else
      coding::ReverseMap(table->m_coding, region->MutableData() + kHeaderSize, "SimpleDenseCoding");
    table->m_region = move(region);
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
// header, endianness mismatch for a mapped mwm)..
template <typename TRegion>
unique_ptr<RankTable> LoadRankTable(unique_ptr<TRegion> && region)
{
  if (!region || !region->ImmutableData() || region->Size() < 8)
  {
    LOG(LERROR, ("Invalid RankTable format."));
    return unique_ptr<RankTable>();
  }

  RankTable::Version const version =
      static_cast<RankTable::Version>(region->ImmutableData()[kVersionOffset]);
  switch (version)
  {
    case RankTable::V1:
      return RankTableV1::Load(move(region));
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

RankTable::~RankTable() {}

// static
unique_ptr<RankTable> RankTable::Load(FilesContainerR & rcont)
{
  return LoadRankTable(GetMemoryRegionForTag(rcont, RANKS_FILE_TAG));
}

// static
unique_ptr<RankTable> RankTable::Load(FilesMappingContainer & mcont)
{
  return LoadRankTable(GetMemoryRegionForTag(mcont, RANKS_FILE_TAG));
}

// static
void RankTableBuilder::CalcSearchRanks(FilesContainerR & rcont, vector<uint8_t> & ranks)
{
  feature::DataHeader header(rcont);
  unique_ptr<feature::FeaturesOffsetsTable> offsetsTable =
      feature::FeaturesOffsetsTable::CreateIfNotExistsAndLoad(rcont);
  ASSERT(offsetsTable.get(), ());
  FeaturesVector featuresVector(rcont, header, offsetsTable.get());

  featuresVector.ForEach([&ranks](FeatureType const & ft, uint32_t /* index */)
                         {
                           ranks.push_back(CalcSearchRank(ft));
                         });
}

// static
void RankTableBuilder::Create(platform::LocalCountryFile const & localFile)
{
  string const mapPath = localFile.GetPath(MapOptions::Map);

  unique_ptr<RankTable> table;
  {
    FilesContainerR rcont(mapPath);
    if (rcont.IsExist(RANKS_FILE_TAG))
    {
      auto reader = rcont.GetReader(RANKS_FILE_TAG);
      if (reader.Size() >= kHeaderSize)
      {
        uint8_t flags;
        reader.Read(kFlagsOffset, &flags, sizeof(flags));

        if (SameEndianness(flags))
        {
          // Feature rank table already exists and has correct
          // endianess. Nothing to do here.
          return;
        }

        // Copy whole serialized table and try to deserialize it via
        // reverse mapping.
        auto region = GetMemoryRegionForTag(rcont, RANKS_FILE_TAG);
        table = LoadRankTable(move(region));
      }
    }

    // Table doesn't exist or has wrong format. It's better to create
    // it from scratch.
    if (!table)
    {
      vector<uint8_t> ranks;
      CalcSearchRanks(rcont, ranks);
      table = make_unique<RankTableV1>(ranks);
    }
  }

  ASSERT(table.get(), ());
  FilesContainerW wcont(mapPath);
  SerializeRankTable(*table, wcont);
}

// static
void RankTableBuilder::Create(vector<uint8_t> const & ranks, FilesContainerW & wcont)
{
  RankTableV1 table(ranks);
  SerializeRankTable(table, wcont);
}
}  // namespace search
