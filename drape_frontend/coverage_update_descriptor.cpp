#include "coverage_update_descriptor.hpp"

namespace df
{

CoverageUpdateDescriptor::CoverageUpdateDescriptor()
  : m_doDropAll(false)
{
}

void CoverageUpdateDescriptor::DropAll()
{
  m_doDropAll = true;
}

void CoverageUpdateDescriptor::DropTiles(const TileKey * tileToDrop, size_t size)
{
  m_tilesToDrop.assign(tileToDrop, tileToDrop + size);
}

bool CoverageUpdateDescriptor::IsDropAll() const
{
  return m_doDropAll;
}

bool CoverageUpdateDescriptor::IsEmpty() const
{
  return !m_doDropAll && m_tilesToDrop.empty();
}

const vector<TileKey> &CoverageUpdateDescriptor::GetTilesToDrop() const
{
  return m_tilesToDrop;
}

}

