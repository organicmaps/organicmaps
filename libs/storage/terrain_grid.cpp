#include "storage/terrain_grid.hpp"

#include "base/exception.hpp"
#include "base/stl_helpers.hpp"

#include <glaze/json.hpp>

namespace storage
{
namespace twm_grid_json
{
// The grid uses countries.json-style "s" size and truncated-BLAKE3 "h" hash per block,
// the extents in whole degrees, the per-block
// version "v" (the top-level index version when omitted) and the region coverage.
struct Block
{
  std::string id;
  int sx = 0;
  int sy = 0;
  uint64_t s = 0;
  std::string h;
  int64_t v = 0;
};
struct Grid
{
  int64_t v = -1;
  std::vector<Block> blocks;
  std::map<std::string, std::vector<std::string>> mwms;
};
}  // namespace twm_grid_json

void TerrainGrid::Parse(std::string const & jsonBuffer)
{
  twm_grid_json::Grid grid;
  glz::opts constexpr opts{.error_on_unknown_keys = false, .error_on_missing_keys = false};
  if (auto const error = glz::read<opts>(grid, jsonBuffer); error)
    MYTHROW(RootException, ("Can't parse the terrain grid:", glz::format_error(error, jsonBuffer)));

  if (grid.v <= 0)
    MYTHROW(RootException, ("Invalid terrain grid version", grid.v));

  ASSERT(IsEmpty(), ());
  m_blocks.reserve(grid.blocks.size());
  for (auto const & json : grid.blocks)
  {
    terrain::GridBlock gridBlock;
    gridBlock.m_width = json.sx;
    gridBlock.m_height = json.sy;
    if (!terrain::ParseBlockName(json.id, gridBlock.m_bottom, gridBlock.m_left) || !terrain::IsValidBlock(gridBlock) ||
        json.s == 0 || json.h.empty() || json.v < 0)
    {
      MYTHROW(RootException, ("Invalid terrain grid block", json.id));
    }
    if (!m_indices.emplace(json.id, static_cast<uint32_t>(m_blocks.size())).second)
      MYTHROW(RootException, ("Duplicate terrain grid block", json.id));
    m_blocks.push_back({json.id, gridBlock.GetRectMercator(), json.s, json.h, json.v > 0 ? json.v : grid.v});
  }

  // The coverage ships with the grid: a dangling block id means a broken bundle. An empty
  // grid disables the terrain (e.g. minimal test bundles).
  if (grid.mwms.empty() && !m_blocks.empty())
    MYTHROW(RootException, ("No region coverage in the terrain grid"));
  m_regions.resize(m_blocks.size());
  for (auto const & [region, ids] : grid.mwms)
  {
    if (ids.empty())
      MYTHROW(RootException, ("Empty coverage of", region));
    auto & regionBlocks = m_coverage[region];
    regionBlocks.reserve(ids.size());
    for (auto const & id : ids)
    {
      auto const it = m_indices.find(id);
      if (it == m_indices.end())
        MYTHROW(RootException, ("Unknown block", id, "in the coverage of", region));
      regionBlocks.push_back(it->second);
      m_regions[it->second].insert(region);
    }
    // Sorted like the group unions; a duplicate id must not double-count the sizes.
    base::SortUnique(regionBlocks);
  }

  m_version = grid.v;
}

std::optional<uint32_t> TerrainGrid::FindIndex(terrain::TerrainId const & id) const
{
  auto const it = m_indices.find(id);
  if (it == m_indices.end())
    return {};
  return it->second;
}

TerrainGrid::Coverage const * TerrainGrid::FindCoverage(CountryId const & region) const
{
  auto const it = m_coverage.find(region);
  return it == m_coverage.end() ? nullptr : &it->second;
}
}  // namespace storage
