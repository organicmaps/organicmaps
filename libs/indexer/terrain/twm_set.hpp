#pragma once

#include "indexer/terrain/terrain_reader.hpp"
#include "indexer/value_set.hpp"

#include "geometry/rect2d.hpp"

#include "base/observer_list.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

namespace terrain
{
/// Information about a registered .twm terrain block.
class TwmInfo : public ds::SetInfoBase
{
public:
  TwmInfo(std::string const & filePath, m2::RectD const & limitRect) : m_filePath(filePath), m_limitRect(limitRect) {}

  std::string const & GetFilePath() const { return m_filePath; }
  /// The block coverage in mercator, from the TWM header.
  m2::RectD const & GetLimitRect() const { return m_limitRect; }

protected:
  using ds::SetInfoBase::SetStatus;
  friend class TwmSet;

  std::string m_filePath;
  m2::RectD m_limitRect;
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
  TwmSetEvent(Type type, std::string const & filePath) : m_type(type), m_filePath(filePath) {}

  Type m_type;
  std::string m_filePath;
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
/// Blocks are keyed by the file path and queried by their header limit rects,
/// so the client does not depend on the block grid layout at all
/// (mixed sizes and the future non-regular split both work). Corrupt files are
/// condemned permanently: neither a re-register nor a rescan resurrects them.
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
    AlreadyRegistered,  ///< The same path is registered (a marked file is resurrected).
    Overlapping,        ///< The block rect overlaps a registered one (see the tracer).
    Condemned,          ///< The file was dropped as corrupt earlier.
    BadFile,            ///< The header is unreadable; the file gets condemned.
  };

  // The observer notes: see MwmSet::Observer - the callbacks can fire on any thread
  // and must be fast and non-blocking.
  class Observer
  {
  public:
    virtual ~Observer() = default;

    virtual void OnTerrainRegistered(std::string const & /* filePath */) {}
    virtual void OnTerrainDeregistered(std::string const & /* filePath */) {}
  };

  std::pair<TwmId, RegResult> Register(std::string const & filePath);

  /// @return true if the file was deregistered right away; a file locked by outstanding
  /// handles is deregistered by the last unlock (the delayed deregistration).
  bool Deregister(std::string const & filePath);

  /// Condemns the blocks detected corrupt too late (e.g. at the trace time): deregisters
  /// them (delayed for the locked ones) and never registers the same paths again.
  void Condemn(std::vector<TwmId> const & ids);

  bool AddObserver(Observer & observer) { return m_observers.Add(observer); }
  bool RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

  /// The registered blocks intersecting the mercator rect. The rect may poke beyond the
  /// +-180 antimeridian (see TileKey::GetWrappedDataRect) - it is split into the
  /// canonical pieces, so the seam queries find the blocks on both sides.
  void GetBlocksByRect(m2::RectD const & rect, std::vector<TwmId> & ids) const;
  bool HasBlocks(m2::RectD const & rect) const;

  Handle GetHandleById(TwmId const & id);
  Handle GetHandleById(TwmId const & id) const { return const_cast<TwmSet *>(this)->GetHandleById(id); }

protected:
  /// @name ds::ValueSetBase overrides.
  //@{
  std::string const & GetRegistryKey(TwmInfo const & info) const override { return info.GetFilePath(); }
  std::unique_ptr<TwmValue> CreateValue(TwmInfo & info) const override;
  void SetStatus(TwmInfo & info, TwmInfo::Status status, EventList & events) override;
  void ProcessEvents(EventList & events) override;
  /// A full reset (e.g. the storage path change) forgets the condemned files too.
  void OnClear() override { m_condemned.clear(); }
  //@}

private:
  template <typename Fn>
  void ForEachBlockByRectImpl(m2::RectD const & rect, Fn && fn) const;

  /// Corrupt files, never registered again (until Clear). Mutable: CreateValue (const,
  /// under m_lock) condemns the files failing to open.
  mutable std::set<std::string> m_condemned;

  base::ObserverListSafe<Observer> m_observers;
};

std::string DebugPrint(TwmSet::RegResult result);
}  // namespace terrain
