#include "indexer/features_offsets_table.hpp"

#include "indexer/features_vector.hpp"

#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "3party/succinct/mapper.hpp"

namespace feature
{
using namespace platform;
void FeaturesOffsetsTable::Builder::PushOffset(uint32_t const offset)
{
  ASSERT(m_offsets.empty() || m_offsets.back() < offset, ());
  m_offsets.push_back(offset);
}

FeaturesOffsetsTable::FeaturesOffsetsTable(succinct::elias_fano::elias_fano_builder & builder) : m_table(&builder) {}

// static
std::unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Build(Builder & builder)
{
  std::vector<uint32_t> const & offsets = builder.m_offsets;

  size_t const numOffsets = offsets.size();
  uint32_t const maxOffset = offsets.empty() ? 0 : offsets.back();

  succinct::elias_fano::elias_fano_builder elias_fano_builder(maxOffset, numOffsets);
  for (uint32_t offset : offsets)
    elias_fano_builder.push_back(offset);

  return std::unique_ptr<FeaturesOffsetsTable>(new FeaturesOffsetsTable(elias_fano_builder));
}

// static
std::unique_ptr<FeaturesOffsetsTable> FeaturesOffsetsTable::Load(FilesContainerR const & cont, std::string const & tag)
{
  std::unique_ptr<FeaturesOffsetsTable> table(new FeaturesOffsetsTable());
  table->m_memRegion = cont.GetMemoryRegion(tag);

  // Should use coding::Map + coding::Freeze, but it breaks current format.
  // So, put ASSERT guard for the base address alignment.
  uint8_t const * addr = table->m_memRegion->ImmutableData();
  ASSERT(reinterpret_cast<size_t>(addr) % sizeof(size_t) == 0, (addr));
  succinct::mapper::map(table->m_table, reinterpret_cast<char const *>(addr));

  return table;
}

void FeaturesOffsetsTable::Build(FilesContainerR const & cont, std::string const & storePath)
{
  Builder builder;
  FeaturesVector::ForEachOffset(cont, [&builder](uint32_t offset) { builder.PushOffset(offset); });
  Build(builder)->Save(storePath);
}

void FeaturesOffsetsTable::Save(std::string const & filePath)
{
  LOG(LINFO, ("Saving features offsets table to", filePath));
  std::string const fileNameTmp = filePath + EXTENSION_TMP;
  succinct::mapper::freeze(m_table, fileNameTmp.c_str());
  base::RenameFileX(fileNameTmp, filePath);
}

uint32_t FeaturesOffsetsTable::GetFeatureOffset(size_t index) const
{
  ASSERT_LESS(index, size(), ());
  return static_cast<uint32_t>(m_table.select(index));
}

size_t FeaturesOffsetsTable::GetFeatureIndexbyOffset(uint32_t offset) const
{
  ASSERT_GREATER(size(), 0, ());
  ASSERT_LESS_OR_EQUAL(offset, m_table.select(size() - 1), ());
  ASSERT_GREATER_OR_EQUAL(offset, m_table.select(0), ());

  // Binary search in elias_fano list
  size_t leftBound = 0, rightBound = size();
  while (leftBound + 1 < rightBound)
  {
    size_t middle = leftBound + (rightBound - leftBound) / 2;
    if (m_table.select(middle) <= offset)
      leftBound = middle;
    else
      rightBound = middle;
  }
  ASSERT_EQUAL(offset, m_table.select(leftBound), ());
  return leftBound;
}

bool BuildOffsetsTable(std::string const & filePath)
{
  try
  {
    std::string const destPath = filePath + TMP_OFFSETS_EXT;
    SCOPE_GUARD(fileDeleter, std::bind(FileWriter::DeleteFileX, destPath));

    {
      FilesContainerR cont(filePath);
      feature::FeaturesOffsetsTable::Build(cont, destPath);
    }

    FilesContainerW(filePath, FileWriter::OP_WRITE_EXISTING).Write(destPath, FEATURE_OFFSETS_FILE_TAG);
    return true;
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("Generating offsets table failed for", filePath, "reason", ex.Msg()));
    return false;
  }
}
}  // namespace feature
