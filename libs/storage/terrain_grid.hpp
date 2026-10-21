#pragma once

#include "storage/storage_defines.hpp"

#include "indexer/terrain/twm_grid.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace storage
{
// The terrain blocks grid (data/twm_grid.json): the countries.json-style size and integrity
// hash per block and the region coverage (twm_grid.json "mwms"). Empty without the bundle.
class TerrainGrid
{
public:
  struct Block
  {
    terrain::TerrainId m_id;
    m2::RectD m_rect;
    uint64_t m_size = 0;
    std::string m_hash;
    // The immutable content revision selects both terrain/<version>/ on the CDN
    // and the local folder. Later grids reference unchanged revisions as-is.
    int64_t m_version = 0;
  };
  // The sorted indices of the blocks intersecting a region polygon.
  using Coverage = std::vector<uint32_t>;

  // Throws RootException on any inconsistency leaving the grid untouched.
  void Parse(std::string const & jsonBuffer);

  bool IsEmpty() const { return m_blocks.empty(); }
  int64_t GetVersion() const { return m_version; }
  size_t GetBlocksCount() const { return m_blocks.size(); }
  Block const & GetBlock(uint32_t index) const { return m_blocks[index]; }
  std::optional<uint32_t> FindIndex(terrain::TerrainId const & id) const;

  // A region missing here has no terrain (World/WorldCoasts by design).
  std::map<CountryId, Coverage> const & GetCoverage() const { return m_coverage; }
  Coverage const * FindCoverage(CountryId const & region) const;
  // The regions whose coverage contains the block.
  CountriesSet const & GetRegions(uint32_t index) const { return m_regions[index]; }

private:
  int64_t m_version = 0;
  std::vector<Block> m_blocks;
  std::map<terrain::TerrainId, uint32_t> m_indices;
  std::map<CountryId, Coverage> m_coverage;
  std::vector<CountriesSet> m_regions;
};
}  // namespace storage
