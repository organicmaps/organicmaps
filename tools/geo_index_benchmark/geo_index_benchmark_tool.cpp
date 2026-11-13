// Compares the geometry index of a map with the index of the latest version: copies the map,
// rebuilds the index of the copy, looks up the tiles of the map in both and checks that they give
// the same features. Prints the sizes of the indexes and the times of the lookups: through the
// file readers, as the application reads a map, and for the new index through a reader of memory.
// The map has to be made with the current styles: the scale bucket of a feature depends on them.
#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/feature_covering.hpp"
#include "indexer/index_builder.hpp"
#include "indexer/map_style.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/v2/cell_trie.hpp"

#include "platform/constants.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <gflags/gflags.h>

DEFINE_string(data_path, "", "Path to the resources directory, the platform looks for it by default");
DEFINE_string(mwm, "", "Path to the map, minsk-pass.mwm of the resources directory by default");
DEFINE_int32(runs, 20, "Passes over the lookups of a zoom level, the fastest one is printed");
DEFINE_int32(max_tiles, 1000, "Maximum number of tiles of a zoom level, evenly taken from the tiles with features");

namespace
{
using Index = ScaleIndex<ModelReaderPtr>;

struct Lookup
{
  covering::Intervals m_intervals;
  int m_scale = 0;
};

template <class TIndex, class Fn>
void ForEachFeature(TIndex const & index, Lookup const & lookup, Fn && fn)
{
  for (auto const & interval : lookup.m_intervals)
    index.ForEachInIntervalAndScale(interval.first, interval.second, lookup.m_scale, fn);
}

// The lookups of the map renderer: the tiles of a zoom level, as TileKey::GetGlobalRect() makes
// them. Only the tiles with features are taken: the bounds of a map may be much bigger than its
// data.
std::vector<Lookup> MakeTileLookups(Index const & index, m2::RectD const & bounds, int zoom, int lastScale)
{
  double const size = mercator::Bounds::kRangeX / (1 << (zoom - 1));
  std::vector<Lookup> lookups;
  for (auto x = static_cast<int64_t>(std::floor(bounds.minX() / size)); x * size < bounds.maxX(); ++x)
  {
    for (auto y = static_cast<int64_t>(std::floor(bounds.minY() / size)); y * size < bounds.maxY(); ++y)
    {
      m2::RectD const tile(x * size, y * size, (x + 1) * size, (y + 1) * size);
      covering::CoveringGetter covering(tile, covering::ViewportWithLowLevels);
      Lookup lookup{covering.Get(lastScale), lastScale};
      bool hasFeatures = false;
      ForEachFeature(index, lookup, [&hasFeatures](uint32_t) { hasFeatures = true; });
      if (!hasFeatures)
        continue;
      lookup.m_scale = std::min(zoom, lastScale);
      lookups.push_back(std::move(lookup));
    }
  }

  if (lookups.size() <= static_cast<size_t>(FLAGS_max_tiles))
    return lookups;
  std::vector<Lookup> sample;
  for (int i = 0; i < FLAGS_max_tiles; ++i)
    sample.push_back(std::move(lookups[i * lookups.size() / FLAGS_max_tiles]));
  return sample;
}

template <class TIndex>
std::vector<uint32_t> GetFeatures(TIndex const & index, Lookup const & lookup)
{
  std::vector<uint32_t> features;
  ForEachFeature(index, lookup, [&features](uint32_t feature) { features.push_back(feature); });
  std::sort(features.begin(), features.end());
  return features;
}

// The time of the fastest pass over the lookups in microseconds per lookup.
template <class TIndex>
double Measure(TIndex const & index, std::vector<Lookup> const & lookups, uint64_t & checksum)
{
  using Clock = std::chrono::steady_clock;
  double best = 0.0;
  for (int run = 0; run < FLAGS_runs; ++run)
  {
    checksum = 0;
    auto const start = Clock::now();
    for (auto const & lookup : lookups)
      ForEachFeature(index, lookup, [&checksum](uint32_t feature) { checksum += feature + 1; });
    double const time = std::chrono::duration<double, std::micro>(Clock::now() - start).count();
    if (run == 0 || time < best)
      best = time;
  }
  return lookups.empty() ? 0.0 : best / static_cast<double>(lookups.size());
}
}  // namespace

int main(int argc, char ** argv)
{
  gflags::SetUsageMessage("Compares the geometry index of a map with the rebuilt index of the latest version.");
  gflags::ParseCommandLineFlags(&argc, &argv, true /* remove_flags */);

  Platform & platform = GetPlatform();
  if (!FLAGS_data_path.empty())
    platform.SetResourceDir(FLAGS_data_path);
  // The style of the generator: the scale bucket of a feature depends on it.
  GetStyleReader().SetCurrentStyle(MapStyleMerged);
  classificator::Load();

  std::string const mapPath =
      FLAGS_mwm.empty() ? base::JoinPath(platform.ResourcesDir(), "minsk-pass" DATA_FILE_EXTENSION) : FLAGS_mwm;
  std::string const copyPath = platform.TmpPathForFile("geo_index_benchmark", DATA_FILE_EXTENSION);
  SCOPE_GUARD(removeCopy, [&copyPath] { base::DeleteFileX(copyPath); });
  CHECK(base::CopyFileX(mapPath, copyPath), ("Can't copy", mapPath, "to", copyPath));
  CHECK(indexer::BuildIndexFromDataFile(copyPath, copyPath), ("Can't build the index of", copyPath));

  // The same page cache as the maps of the application have.
  FilesContainerR const oldContainer(mapPath, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
  FilesContainerR const newContainer(copyPath, READER_CHUNK_LOG_SIZE, READER_CHUNK_LOG_COUNT);
  auto const oldReader = oldContainer.GetReader(INDEX_FILE_TAG);
  auto const newReader = newContainer.GetReader(INDEX_FILE_TAG);
  Index const oldIndex(oldReader);
  Index const newIndex(newReader);
  CHECK(newIndex.GetVersion() == ScaleIndexBase::Version::Latest, ());

  // The new index in a buffer: what a reader of memory or of a mapped file gives.
  std::vector<uint8_t> buffer(newReader.Size());
  newReader.Read(0, buffer.data(), buffer.size());
  ScaleIndex<MemReader> const memoryIndex(MemReader(buffer.data(), buffer.size()));

  printf("Map: %s\n", mapPath.c_str());
  printf("Index: version %u, %llu bytes; rebuilt: version %u, %llu bytes, %.1f%%\n",
         static_cast<unsigned>(oldIndex.GetVersion()), static_cast<unsigned long long>(oldReader.Size()),
         static_cast<unsigned>(newIndex.GetVersion()), static_cast<unsigned long long>(newReader.Size()),
         100.0 * static_cast<double>(newReader.Size()) / static_cast<double>(oldReader.Size()));
  {
    // What a lookup reads: the vectors of the trie to find the cells, then the packets of their features.
    auto const trieReader =
        newReader.SubReader(sizeof(ScaleIndexBase::Header), newReader.Size() - sizeof(ScaleIndexBase::Header));
    idx::v2::CellTrie<ModelReaderPtr> const trie(trieReader);
    auto const & trieHeader = trie.GetHeader();
    printf(
        "Rebuilt index: %llu cells with features, %llu bytes of the trie vectors (children %llu, cells with "
        "features %llu, packet offsets %llu), %llu bytes of the packets\n",
        static_cast<unsigned long long>(trieHeader.m_tokens),
        static_cast<unsigned long long>(trieHeader.m_masksSize + trieHeader.m_endingSize + trieHeader.m_offsetsSize),
        static_cast<unsigned long long>(trieHeader.m_masksSize),
        static_cast<unsigned long long>(trieHeader.m_endingSize),
        static_cast<unsigned long long>(trieHeader.m_offsetsSize),
        static_cast<unsigned long long>(trieReader.Size() - trie.GetPacketsPos()));
  }

  feature::DataHeader const header(oldContainer);
  int const lastScale = header.GetLastScale();
  m2::RectD const bounds = header.GetBounds();
  printf("Time of a lookup in microseconds, the fastest of %d passes:\n", FLAGS_runs);
  printf("%5s %8s %10s %10s %10s %10s %8s %10s %8s\n", "zoom", "lookups", "intervals", "features", "old", "new",
         "old/new", "memory", "old/mem");

  size_t different = 0;
  size_t total = 0;
  // A country map is drawn from the zoom level after the last one of the world map. The last row
  // is the lookup of all features.
  for (int zoom = std::max(1, lastScale - 7); zoom <= lastScale + 1; ++zoom)
  {
    std::vector<Lookup> lookups;
    if (zoom <= lastScale)
    {
      lookups = MakeTileLookups(oldIndex, bounds, zoom, lastScale);
    }
    else
    {
      covering::CoveringGetter covering(bounds, covering::FullCover);
      lookups.push_back({covering.Get(lastScale), lastScale});
    }

    size_t intervals = 0;
    size_t features = 0;
    for (auto const & lookup : lookups)
    {
      auto const expected = GetFeatures(oldIndex, lookup);
      if (GetFeatures(newIndex, lookup) != expected)
        ++different;
      intervals += lookup.m_intervals.size();
      features += expected.size();
    }
    total += lookups.size();

    uint64_t oldChecksum = 0;
    uint64_t newChecksum = 0;
    uint64_t memoryChecksum = 0;
    double const oldTime = Measure(oldIndex, lookups, oldChecksum);
    double const newTime = Measure(newIndex, lookups, newChecksum);
    double const memoryTime = Measure(memoryIndex, lookups, memoryChecksum);
    CHECK(different != 0 || oldChecksum == newChecksum, (zoom));
    CHECK_EQUAL(newChecksum, memoryChecksum, (zoom));
    printf("%5s %8zu %10zu %10zu %10.2f %10.2f %8.1f %10.2f %8.1f\n",
           zoom <= lastScale ? std::to_string(zoom).c_str() : "all", lookups.size(), intervals, features, oldTime,
           newTime, newTime > 0.0 ? oldTime / newTime : 0.0, memoryTime, memoryTime > 0.0 ? oldTime / memoryTime : 0.0);
  }

  if (different != 0)
  {
    printf("FAILED: different features in %zu of %zu lookups. Was the map made with these styles?\n", different, total);
    return 1;
  }
  printf("The same features in all %zu lookups.\n", total);
  return 0;
}
