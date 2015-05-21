#pragma once

#include "tile.hpp"

#include "base/mutex.hpp"

#include "std/map.hpp"

class TileSet
{
private:

  typedef map<Tiler::RectInfo, Tile> TTiles;

  TTiles m_tiles;

  threads::Mutex m_mutex;

public:

  /// lock TileSet for multithreaded access.
  void Lock();
  /// unlock TileSet for multithreaded access.
  void Unlock();
  /// do we have the specified tile
  bool HasTile(Tiler::RectInfo const & rectInfo);
  /// add tile to the set
  void AddTile(Tile const & tile);
  /// get sequenceID in witch tile was rendered
  int GetTileSequenceID(Tiler::RectInfo const & rectInfo);
  void SetTileSequenceID(Tiler::RectInfo const & rectInfo, int sequenceID);
  /// get tile from the set
  Tile const & GetTile(Tiler::RectInfo const & rectInfo);
  /// remove tile from the set
  void RemoveTile(Tiler::RectInfo const & rectInfo);
  /// get the size of TileSet
  size_t Size() const;
};
