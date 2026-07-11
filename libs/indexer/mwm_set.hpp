#pragma once
#include "indexer/data_header.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/house_to_street_iface.hpp"
#include "indexer/value_set.hpp"

#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "coding/files_container.hpp"

#include "geometry/rect2d.hpp"

#include "base/macros.hpp"
#include "base/observer_list.hpp"

#include "defines.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace feature
{
class FeaturesOffsetsTable;
}
namespace indexer
{
class MetadataDeserializer;
}

/// Information about stored mwm.
class MwmInfo : public ds::SetInfoBase
{
public:
  friend class DataSource;
  friend class MwmSet;

  enum MwmTypeT
  {
    COUNTRY,
    WORLD,
    COASTS
  };

  MwmInfo() : m_minScale(0), m_maxScale(0) {}

  /// @obsolete Rect around region border. Features which cross region border may cross this rect.
  /// @todo VNG: Not true. This rect accumulates all features in MWM. Since we don't crop features by border,
  /// the rect defines _bigger_ area (in general) that MWM is responsible for.
  /// Take into account this fact, or you can get fair rect with CountryInfoGetter.
  m2::RectD m_bordersRect;

  uint8_t m_minScale;             ///< Min zoom level of mwm.
  uint8_t m_maxScale;             ///< Max zoom level of mwm.
  version::MwmVersion m_version;  ///< Mwm file version.

  platform::LocalCountryFile const & GetLocalFile() const { return m_file; }

  std::string const & GetCountryName() const { return m_file.GetCountryName(); }

  int64_t GetVersion() const { return m_file.GetVersion(); }

  MwmTypeT GetType() const;

  feature::RegionData const & GetRegionData() const { return m_data; }
  bool IsAddressLikeUS() const;

protected:
  using ds::SetInfoBase::SetStatus;

  feature::RegionData m_data;

  platform::LocalCountryFile m_file;  ///< Path to the mwm file.
};

class MwmInfoEx : public MwmInfo
{
private:
  friend class DataSource;
  friend class MwmValue;

  // weak_ptr is needed here to access offsets table in already
  // instantiated MwmValue-s for the MWM, including MwmValues in the
  // MwmSet's cache. We can't use shared_ptr because of offsets table
  // must be removed as soon as the last corresponding MwmValue is
  // destroyed. Also, note that this value must be used and modified
  // only in MwmValue::SetTable() method, which, in turn, is called
  // only in the MwmSet critical section, protected by a lock.  So,
  // there's an implicit synchronization on this field.
  std::weak_ptr<feature::FeaturesOffsetsTable> m_ftTable, m_relTable;
};

class MwmValue;

class MwmId : public ds::SetId<MwmInfo>
{
public:
  using SetId::SetId;

  /// @todo I suppose that m_info->GetStatus() == MwmInfo::STATUS_REGISTERED is better and more precise.
  bool IsDeregistered(platform::LocalCountryFile const & deregisteredCountryFile) const;

  friend std::string DebugPrint(MwmId const & id);
};

// The MwmSet events: registration status changes of the local country files.
struct MwmSetEvent
{
  enum Type
  {
    TYPE_REGISTERED,
    TYPE_DEREGISTERED,
  };

  MwmSetEvent() = default;
  MwmSetEvent(Type type, platform::LocalCountryFile const & file) : m_type(type), m_file(file) {}
  MwmSetEvent(Type type, platform::LocalCountryFile const & newFile, platform::LocalCountryFile const & oldFile)
    : m_type(type)
    , m_file(newFile)
    , m_oldFile(oldFile)
  {}

  bool operator==(MwmSetEvent const & rhs) const
  {
    return m_type == rhs.m_type && m_file == rhs.m_file && m_oldFile == rhs.m_oldFile;
  }

  bool operator!=(MwmSetEvent const & rhs) const { return !(*this == rhs); }

  Type m_type;
  platform::LocalCountryFile m_file;
  platform::LocalCountryFile m_oldFile;
};

class MwmSetEventList
{
public:
  MwmSetEventList() = default;

  void Add(MwmSetEvent const & event) { m_events.push_back(event); }

  void Append(MwmSetEventList const & events)
  {
    m_events.insert(m_events.end(), events.m_events.begin(), events.m_events.end());
  }

  std::vector<MwmSetEvent> const & Get() const { return m_events; }

private:
  std::vector<MwmSetEvent> m_events;

  DISALLOW_COPY_AND_MOVE(MwmSetEventList);
};

class MwmSet : public ds::ValueSetBase<MwmId, MwmValue, MwmSetEventList>
{
  using BaseT = ds::ValueSetBase<MwmId, MwmValue, MwmSetEventList>;

public:
  using MwmId = ::MwmId;

  explicit MwmSet(size_t cacheSize = 64) : BaseT(cacheSize) {}

  // Mwm handle, which is used to refer to mwm and prevent it from
  // deletion when its FileContainer is used.
  using MwmHandle = BaseT::Handle;

  using Event = MwmSetEvent;
  using EventList = MwmSetEventList;

  enum class RegResult
  {
    Success,
    VersionAlreadyExists,
    VersionTooOld,
    UnsupportedFileFormat,
    BadFile
  };

  // An Observer interface to MwmSet. Note that these functions can
  // be called from *ANY* thread because most signals are sent when
  // some thread releases its MwmHandle, so overrides must be as fast
  // as possible and non-blocking when it's possible.
  class Observer
  {
  public:
    virtual ~Observer() = default;

    // Called when a map is registered for the first time and can be used.
    virtual void OnMapRegistered(platform::LocalCountryFile const & /* localFile */) {}

    // Called when a map is deregistered and can no longer be used.
    virtual void OnMapDeregistered(platform::LocalCountryFile const & /* localFile */) {}
  };

  /// Registers a new map.
  ///
  /// \return An active mwm handle when an mwm file with this version
  /// already exists (in this case mwm handle will point to already
  /// registered file) or when all registered corresponding mwm files
  /// are older than the localFile (in this case mwm handle will point
  /// to just-registered file).

protected:
  std::pair<MwmId, RegResult> RegisterImpl(platform::LocalCountryFile const & localFile, EventList & events);

public:
  std::pair<MwmId, RegResult> Register(platform::LocalCountryFile const & localFile);
  //@}

  /// @name Remove mwm.
  //@{

protected:
  /// Deregisters a map from internal records.
  ///
  /// \param countryFile A countryFile denoting a map to be deregistered.
  /// \return True if the map was successfully deregistered. If map is locked
  ///         now, returns false.
  using BaseT::DeregisterImpl;
  bool DeregisterImpl(platform::CountryFile const & countryFile, EventList & events);

public:
  bool Deregister(platform::CountryFile const & countryFile);
  //@}

  bool AddObserver(Observer & observer) { return m_observers.Add(observer); }

  bool RemoveObserver(Observer const & observer) { return m_observers.Remove(observer); }

  /// @return true when country is registered and can be used.
  bool IsLoaded(platform::CountryFile const & countryFile) const;

  std::vector<std::string> GetLoadedCountryNames(m2::RectD const & rect) const;

  /// @return 0 if country is not loaded.
  int64_t GetMwmVersion(platform::CountryFile const & countryFile) const;

  /// Get ids of all mwms. Some of them may be with not active status.
  /// In that case, LockValue returns NULL.
  /// @todo In fact, std::shared_ptr<MwmInfo> is a MwmId. Seems like better to make vector<MwmId> interface.
  void GetMwmsInfo(std::vector<std::shared_ptr<MwmInfo>> & info) const { GetInfos(info); }

  MwmId GetMwmIdByCountryFile(platform::CountryFile const & countryFile) const;

  MwmHandle GetMwmHandleByCountryFile(platform::CountryFile const & countryFile);

  MwmHandle GetMwmHandleById(MwmId const & id);

  /// Now this function looks like workaround, but it allows to avoid ugly const_cast everywhere..
  /// Client code usually holds const reference to DataSource, but implementation is non-const.
  /// @todo Actually, we need to define, is this behaviour (getting Handle) const or non-const.
  MwmHandle GetMwmHandleById(MwmId const & id) const { return const_cast<MwmSet *>(this)->GetMwmHandleById(id); }

protected:
  virtual std::unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const = 0;

  /// @name ds::ValueSetBase overrides.
  //@{
  // std::unique_ptr<MwmValue> CreateValue(MwmInfo & info) const, stays pure for the descendants.
  std::string const & GetRegistryKey(MwmInfo const & info) const override { return info.GetCountryName(); }
  void SetStatus(MwmInfo & info, MwmInfo::Status status, EventList & events) override;
  void ProcessEvents(EventList & events) override;
  //@}

  /// Find mwm with a given name.
  /// @precondition This function is always called under mutex m_lock.
  MwmId GetMwmIdByCountryFileImpl(platform::CountryFile const & countryFile) const;

private:
  base::ObserverListSafe<Observer> m_observers;
};  // class MwmSet

class MwmValue
{
public:
  FilesContainerR const m_cont;
  platform::LocalCountryFile const m_file;

private:
  version::MwmVersion m_version;
  feature::DataHeader m_header;

public:
  // m_ftTable should always present, m_relTable maybe nullptr.
  std::shared_ptr<feature::FeaturesOffsetsTable> m_ftTable, m_relTable;
  std::unique_ptr<indexer::MetadataDeserializer> m_metaDeserializer;
  std::unique_ptr<HouseToStreetTable> m_house2street, m_house2place;

public:
  MwmValue(ModelReaderPtr const & reader, platform::LocalCountryFile const & localFile);
  explicit MwmValue(platform::LocalCountryFile const & localFile);
  ~MwmValue();

  void SetTable(MwmInfoEx & info);

  feature::DataHeader const & GetHeader() const { return m_header; }
  version::MwmVersion const & GetMwmVersion() const { return m_version; }
  std::string const & GetCountryFileName() const { return m_file.GetCountryName(); }

  bool HasSearchIndex() const { return m_cont.IsExist(SEARCH_INDEX_FILE_TAG); }
  bool HasGeometryIndex() const { return m_cont.IsExist(INDEX_FILE_TAG); }
};  // class MwmValue

std::string DebugPrint(MwmSet::RegResult result);
std::string DebugPrint(MwmSet::Event::Type type);
std::string DebugPrint(MwmSet::Event const & event);

namespace std
{
template <>
struct hash<MwmSet::MwmId>
{
  size_t operator()(MwmSet::MwmId const & id) const { return std::hash<std::shared_ptr<MwmInfo>>()(id.GetInfo()); }
};
}  // namespace std
