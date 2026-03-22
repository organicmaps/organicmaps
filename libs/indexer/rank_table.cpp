#include "indexer/rank_table.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature_impl.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/memory_region.hpp"
#include "coding/reader.hpp"
#include "coding/simple_dense_coding.hpp"
#include "coding/succinct_mapper.hpp"
#include "coding/writer.hpp"

#include "base/exception.hpp"
#include "base/logging.hpp"

#include <algorithm>
#include <limits>

#include "defines.hpp"

namespace search
{
namespace
{
size_t constexpr kVersionOffset = 0;
size_t constexpr kHeaderSize = 8;

std::unique_ptr<CopiedMemoryRegion> GetMemoryRegionForTag(FilesContainerR const & rcont,
                                                          FilesContainerBase::Tag const & tag)
{
  if (!rcont.IsExist(tag))
    return {};

  FilesContainerR::TReader reader = rcont.GetReader(tag);
  std::vector<uint8_t> buffer(static_cast<size_t>(reader.Size()));
  reader.Read(0, buffer.data(), buffer.size());
  return std::make_unique<CopiedMemoryRegion>(std::move(buffer));
}

std::unique_ptr<MappedMemoryRegion> GetMemoryRegionForTag(FilesMappingContainer const & mcont,
                                                          FilesContainerBase::Tag const & tag)
{
  if (!mcont.IsExist(tag))
    return {};

  FilesMappingContainer::Handle handle = mcont.Map(tag);
  return std::make_unique<MappedMemoryRegion>(std::move(handle));
}

// RankTable version 0, uses simple dense coding to store and access array of ranks.
class RankTableV0 : public RankTable
{
public:
  RankTableV0() = default;

  explicit RankTableV0(std::vector<uint8_t> const & ranks) : m_coding(ranks) {}

  // RankTable overrides:
  uint8_t Get(uint64_t i) const override
  {
    // i can be greater than Size() for features created by user in the Editor.
    // May be there is a better way to inject this code. Without this check search engine crashes here.
    // ASSERT_LESS(i, Size(), ());
    if (i >= Size())
      return kNoRank;

    return m_coding.Get(i);
  }
  uint64_t Size() const override { return m_coding.Size(); }
  RankTable::Version GetVersion() const override { return V0; }
  void Serialize(Writer & writer) override
  {
    uint8_t const version = GetVersion();
    size_t constexpr kVersionSize = sizeof(version);
    writer.Write(&version, kVersionSize);

    uint64_t const padding = 0;
    static_assert(kHeaderSize % 8 == 0);  // next succinct vector should be aligned
    writer.Write(&padding, kHeaderSize - kVersionSize);

    Freeze(m_coding, writer, "SimpleDenseCoding");
  }

  // Loads RankTableV0 from a raw memory region.
  template <class TRegion>
  static std::unique_ptr<RankTableV0> Load(std::unique_ptr<TRegion> && region)
  {
    auto table = std::make_unique<RankTableV0>();
    table->m_region = std::move(region);
    coding::Map(table->m_coding, table->m_region->ImmutableData() + kHeaderSize, "SimpleDenseCoding");
    return table;
  }

private:
  std::unique_ptr<MemoryRegion> m_region;
  coding::SimpleDenseCoding m_coding;
};

// Following two functions create a rank section and serialize |table|
// to it. If there was an old section with ranks, these functions
// overwrite it.
void SerializeRankTable(RankTable & table, FilesContainerW & wcont, std::string const & sectionName)
{
  if (wcont.IsExist(sectionName))
    wcont.DeleteSection(sectionName);

  std::vector<char> buffer;
  {
    MemWriter<decltype(buffer)> writer(buffer);
    table.Serialize(writer);
  }

  wcont.Write(buffer, sectionName);
  wcont.Finish();
}

void SerializeRankTable(RankTable & table, std::string const & mapPath, std::string const & sectionName)
{
  FilesContainerW wcont(mapPath, FileWriter::OP_WRITE_EXISTING);
  SerializeRankTable(table, wcont, sectionName);
}

// Deserializes rank table from a rank section. Returns null when it's
// not possible to load a rank table (no rank section, corrupted
// header, endianness mismatch for a mapped mwm).
template <typename TRegion>
std::unique_ptr<RankTable> LoadRankTable(std::unique_ptr<TRegion> && region)
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
    return RankTableV0::Load(std::move(region));

  LOG(LERROR, ("Wrong rank table version."));
  return {};
}

// Calculates search rank for a feature.
uint8_t CalcSearchRank(FeatureType & ft)
{
  return feature::PopulationToRank(ftypes::GetPopulation(ft));
}

// Creates rank table if it does not exists in |rcont| or has wrong
// endianness. Otherwise (table exists and has correct format) returns null.
std::unique_ptr<RankTable> CreateSearchRankTableIfNotExists(FilesContainerR & rcont)
{
  if (rcont.IsExist(SEARCH_RANKS_FILE_TAG))
    return {};

  std::vector<uint8_t> ranks;
  SearchRankTableBuilder::CalcSearchRanks(rcont, ranks);
  return std::make_unique<RankTableV0>(ranks);
}
}  // namespace

// static
std::unique_ptr<RankTable> RankTable::Load(FilesContainerR const & rcont, std::string const & sectionName)
{
  return LoadRankTable(GetMemoryRegionForTag(rcont, sectionName));
}

// static
std::unique_ptr<RankTable> RankTable::Load(FilesMappingContainer const & mcont, std::string const & sectionName)
{
  return LoadRankTable(GetMemoryRegionForTag(mcont, sectionName));
}

// static
void SearchRankTableBuilder::CalcSearchRanks(FilesContainerR & rcont, std::vector<uint8_t> & ranks)
{
  feature::DataHeader header(rcont);
  FeaturesVector featuresVector(rcont, header, nullptr, nullptr, nullptr);

  featuresVector.ForEach([&ranks](FeatureType & ft, uint32_t /* index */) { ranks.push_back(CalcSearchRank(ft)); });
}

// static
bool SearchRankTableBuilder::CreateIfNotExists(std::string const & mapPath) noexcept
{
  try
  {
    std::unique_ptr<RankTable> table;
    {
      FilesContainerR rcont(mapPath);
      table = CreateSearchRankTableIfNotExists(rcont);
    }

    if (table)
      SerializeRankTable(*table, mapPath, SEARCH_RANKS_FILE_TAG);

    return true;
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Can' create rank table for:", mapPath, ":", e.what()));
    return false;
  }
}

// static
void RankTableBuilder::Create(std::vector<uint8_t> const & ranks, FilesContainerW & wcont,
                              std::string const & sectionName)
{
  RankTableV0 table(ranks);
  SerializeRankTable(table, wcont, sectionName);
}
}  // namespace search
