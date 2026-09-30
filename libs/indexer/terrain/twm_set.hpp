#pragma once

#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/terrain/twm_grid.hpp"
#include "indexer/value_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/observer_list.hpp"

#include <memory>
#include <string>
#include <vector>

namespace terrain
{
// The blocks share their border lines by design, so only the interiors may overlap.
// Both the registration overlap rejection and the covered-blocks deletion must agree
// on this (an edge-touching neighbor is neither a conflict nor covered).
inline bool IsInteriorOverlap(m2::RectD const & lhs, m2::RectD const & rhs)
{
  return lhs.minX() < rhs.maxX() && rhs.minX() < lhs.maxX() && lhs.minY() < rhs.maxY() && rhs.minY() < lhs.maxY();
}

/// Information about a registered .twm terrain block.
class TwmInfo : public ds::SetInfoBase
{
public:
  explicit TwmInfo(TwmFile const & file) : m_file(file) {}

  TwmFile const & GetFile() const { return m_file; }
  TerrainId const & GetTerrainId() const { return m_file.m_id; }
  std::string const & GetFilePath() const { return m_file.m_path; }
  m2::RectD const & GetLimitRect() const { return m_file.m_rect; }
  int64_t GetVersion() const { return m_file.m_version; }

protected:
  using ds::SetInfoBase::SetStatus;
  friend class TwmSet;

  TwmFile m_file;
};

class TwmId : public ds::SetId<TwmInfo>
{
public:
  using SetId::SetId;

  friend std::string DebugPrint(TwmId const & id);
};

// The TwmSet events: registration status changes of the terrain files.
struct TwmSetEvent
{
  enum Type
  {
    TYPE_REGISTERED,
    TYPE_DEREGISTERED,
  };

  TwmSetEvent() = default;
  TwmSetEvent(Type type, TwmFile const & file) : m_type(type), m_file(file) {}

  Type m_type;
  TwmFile m_file;
};

class TwmSetEventList
{
public:
  TwmSetEventList() = default;

  void Add(TwmSetEvent const & event) { m_events.push_back(event); }
  std::vector<TwmSetEvent> const & Get() const { return m_events; }

private:
  std::vector<TwmSetEvent> m_events;

  DISALLOW_COPY_AND_MOVE(TwmSetEventList);
};

/// The opened terrain block: the parse-once reader (container, header, interval index).
/// Values are handed off exclusively by the set, so no internal thread safety is needed.
class TwmValue
{
public:
  explicit TwmValue(std::string const & filePath) : m_reader(FilesContainerR(filePath)) {}

  Reader const & GetReader() const { return m_reader; }

private:
  Reader m_reader;
};

/// The registry of the terrain blocks: the MwmSet counterpart for the .twm files.
/// Blocks are keyed by TerrainId; marked old versions may coexist with their replacements.
class TwmSet : public ds::ValueSetBase<TwmId, TwmValue, TwmSetEventList>
{
  using BaseT = ds::ValueSetBase<TwmId, TwmValue, TwmSetEventList>;

public:
  explicit TwmSet(size_t cacheSize = 16) : BaseT(cacheSize) {}

  using Handle = BaseT::Handle;
  using Event = TwmSetEvent;
  using EventList = TwmSetEventList;

  enum class RegResult
  {
    Success,
    AlreadyRegistered,  ///< The same file is registered (a marked file is resurrected).
    Overlapping,        ///< The block rect overlaps a registered one (see the tracer).
    BadFile,            ///< The header is unreadable.
    ObsoleteVersion,    ///< An old format no build reads anymore; the caller deletes the file.
  };

  // The observer notes: see MwmSet::Observer - the callbacks can fire on any thread
  // and must be fast and non-blocking.
  class Observer
  {
  public:
    virtual ~Observer() = default;

    virtual void OnTerrainRegistered(TwmFile const & /* file */) {}
    virtual void OnTerrainDeregistered(TwmFile const & /* file */) {}
  };

  /// Registers a descriptor validated by ReadFile. Storage selects versions and replacements.
  std::pair<TwmId, RegResult> Register(TwmFile const & file);

  /// Reads the file identity and header coverage without changing the registry.
  static RegResult ReadFile(std::string const & filePath, int64_t version, TwmFile & file);

  TwmId GetId(TerrainId const & terrainId) const;
  /// Includes older marked registrations still held by readers.
  bool IsFileAlive(TwmFile const & file) const;

  /// @return true if deregistered immediately; active handles defer it until their last unlock.
  bool Deregister(TerrainId const & terrainId);

  /// Deregisters blocks detected corrupt while reading, delayed until their last unlock.
  void Condemn(std::vector<TwmId> const & ids);

  bool AddObserver(Observer & observer) { return m_observers.Add(observer); }
  bool RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

  /// The registered blocks intersecting the mercator rect. The block rects are canonical,
  /// so a query rect poking beyond the +-180 seam (a TileKey::GetWrappedDataRect of a tile
  /// straddling or beyond the antimeridian) matches its canonical intersection - the same
  /// clipping semantics the MWM feature index applies to such tile rects.
  void GetBlocksByRect(m2::RectD const & rect, std::vector<TwmId> & ids) const;
  bool HasBlocks(m2::RectD const & rect) const;
  /// The limit rects of the registered blocks intersecting the mercator rect.
  void GetBlockRectsByRect(m2::RectD const & rect, std::vector<m2::RectD> & rects) const;

  Handle GetHandleById(TwmId const & id);
  Handle GetHandleById(TwmId const & id) const { return const_cast<TwmSet *>(this)->GetHandleById(id); }

protected:
  /// @name ds::ValueSetBase overrides.
  //@{
  std::string const & GetRegistryKey(TwmInfo const & info) const override { return info.GetTerrainId(); }
  std::unique_ptr<TwmValue> CreateValue(TwmInfo & info) const override;
  void SetStatus(TwmInfo & info, TwmInfo::Status status, EventList & events) override;
  void ProcessEvents(EventList & events) override;
  //@}

private:
  /// Reads the .twm header; false when unreadable or unsupported. header.m_version keeps
  /// the file version whenever its byte - the first one of the header - was read, so an
  /// old format file reports itself even through the failed parse; the rest of the header
  /// is valid only on true.
  static bool ReadHeader(std::string const & filePath, TwmHeader & header);

  template <typename Fn>
  void ForEachBlockByRectImpl(m2::RectD const & rect, Fn && fn) const;

  base::ObserverListSafe<Observer> m_observers;
};

std::string DebugPrint(TwmSet::RegResult result);
}  // namespace terrain
