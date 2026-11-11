#include "map/bookmarks_sync/reconciler.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <set>
#include <tuple>
#include <unordered_set>

namespace bookmarks_sync
{
namespace
{
using namespace action;

std::optional<SyncAction> DecideWithoutBase(std::string const & name, LocalFile const * local, CloudFile const * cloud)
{
  if (local && cloud)
  {
    if (local->m_hash == *cloud->m_hash)
      return RecordBase{name, {cloud->m_id, *cloud->m_hash, cloud->m_version}};
    return ConflictCopy{name, *cloud, local->m_hash, false /* hasBase */};
  }
  if (local)
    return Upload{name, {}};
  if (cloud)
    return Download{name, *cloud, {}, {}};
  return {};
}

std::optional<SyncAction> Decide(std::string const & name, LocalFile const * local, CloudFile const * cloud,
                                 BaseEntry const * base)
{
  if (!base)
    return DecideWithoutBase(name, local, cloud);
  if (local && cloud)
  {
    bool const localChanged = local->m_hash != base->m_hash;
    bool const cloudChanged = cloud->m_version != base->m_version;
    if (!localChanged && !cloudChanged)
      return {};
    if (!cloudChanged)
      return Upload{name, cloud->m_id};
    if (!localChanged)
      return Download{name, *cloud, local->m_hash, {}};
    if (local->m_hash == *cloud->m_hash)
      return RecordBase{name, {cloud->m_id, *cloud->m_hash, cloud->m_version}};
    return ConflictCopy{name, *cloud, local->m_hash, true /* hasBase */};
  }
  if (local)
  {
    if (local->m_hash == base->m_hash)
      return TrashLocal{name, local->m_hash, base->m_cloudId};
    return Upload{name, base->m_cloudId};
  }
  if (cloud)
  {
    if (cloud->m_version == base->m_version)
      return TrashCloud{name, cloud->m_id, cloud->m_version};
    return Download{name, *cloud, {}, {}};
  }
  // Gone from both listings: the cloud one may only lag behind, so it is confirmed by its id.
  return TrashCloud{name, base->m_cloudId, base->m_version};
}

// Another cloud file owns the name. While the synced one is still listed, this device's upload lost a duplicate
// race: compare as if never synced, so the local version is kept as a copy. Otherwise the synced file was deleted
// elsewhere and a new one took its name, which an unchanged local file follows like any remote deletion.
std::optional<SyncAction> DecideReplaced(std::string const & name, LocalFile const * local, CloudFile const & cloud,
                                         BaseEntry const & base, bool isBaseListed)
{
  if (!isBaseListed && local && local->m_hash == base.m_hash && base.m_hash != *cloud.m_hash)
    return Download{name, cloud, base.m_hash, base.m_cloudId};
  return DecideWithoutBase(name, local, &cloud);
}

int KindOrder(SyncAction const & action)
{
  return std::visit([](auto const & a)
  {
    using T = std::decay_t<decltype(a)>;
    if constexpr (std::is_same_v<T, ConflictCopy>)
      return 0;
    else if constexpr (std::is_same_v<T, Download>)
      return 1;
    else if constexpr (std::is_same_v<T, Upload>)
      return 2;
    else if constexpr (std::is_same_v<T, TrashCloud> || std::is_same_v<T, TrashLocal>)
      return 3;
    else if constexpr (std::is_same_v<T, RecordBase>)
      return 4;
    else
      return 5;
  }, action);
}

std::string const & NameOf(SyncAction const & action)
{
  return std::visit([](auto const & a) -> std::string const & { return a.m_name; }, action);
}
}  // namespace

bool IsOlder(CloudFile const & lhs, CloudFile const & rhs)
{
  return std::tie(lhs.m_createdTimeMs, lhs.m_id) < std::tie(rhs.m_createdTimeMs, rhs.m_id);
}

std::vector<SyncAction> Plan(std::vector<LocalFile> const & local, std::vector<CloudFile> const & cloud,
                             std::map<std::string, BaseEntry> const & base)
{
  std::vector<SyncAction> actions;
  std::map<std::string, LocalFile const *> localByName;
  for (auto const & file : local)
    localByName.emplace(file.m_name, &file);

  std::map<std::string, CloudFile const *> cloudByName;
  std::unordered_set<std::string> cloudIds;
  for (auto const & file : cloud)
  {
    ASSERT(file.m_hash, ("Cloud hashes are filled before planning", file));
    cloudIds.insert(file.m_id);
    auto [it, inserted] = cloudByName.emplace(file.m_name, &file);
    if (!inserted && IsOlder(file, *it->second))
      it->second = &file;
  }
  for (auto const & file : cloud)
  {
    auto const & owner = *cloudByName[file.m_name];
    if (&file == &owner)
      continue;
    auto const it = base.find(file.m_name);
    // The unchanged version that this device synced last: the action for the name keeps it or what replaced it.
    bool const isOwn = it != base.end() && it->second.m_cloudId == file.m_id && it->second.m_version == file.m_version;
    bool const isKnown = isOwn || *file.m_hash == *owner.m_hash;
    actions.emplace_back(ResolveDuplicate{file.m_name, file, !isKnown});
  }

  std::set<std::string> names;
  for (auto const & [name, _] : localByName)
    names.insert(name);
  for (auto const & [name, _] : cloudByName)
    names.insert(name);
  for (auto const & [name, _] : base)
    names.insert(name);

  for (auto const & name : names)
  {
    auto const localIt = localByName.find(name);
    auto const cloudIt = cloudByName.find(name);
    auto const baseIt = base.find(name);
    LocalFile const * localFile = localIt == localByName.end() ? nullptr : localIt->second;
    CloudFile const * cloudFile = cloudIt == cloudByName.end() ? nullptr : cloudIt->second;
    BaseEntry const * baseEntry = baseIt == base.end() ? nullptr : &baseIt->second;
    // A base entry describes one cloud file, so it applies only while no other file owns the name.
    std::optional<SyncAction> action;
    if (!baseEntry || !cloudFile || baseEntry->m_cloudId == cloudFile->m_id)
      action = Decide(name, localFile, cloudFile, baseEntry);
    else
      action = DecideReplaced(name, localFile, *cloudFile, *baseEntry, cloudIds.contains(baseEntry->m_cloudId));
    if (action)
      actions.push_back(std::move(*action));
  }

  std::stable_sort(actions.begin(), actions.end(), [](SyncAction const & lhs, SyncAction const & rhs)
  { return std::make_pair(KindOrder(lhs), NameOf(lhs)) < std::make_pair(KindOrder(rhs), NameOf(rhs)); });
  return actions;
}
}  // namespace bookmarks_sync
