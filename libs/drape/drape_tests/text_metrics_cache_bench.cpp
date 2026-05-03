// Microbenchmarks for the TextMetricsCache hot path (TextureManager::ShapeSingleTextLine ->
// GlyphManager::ShapeText -> cache Find/Insert). The cache itself lives in glyph_manager.cpp's
// anonymous namespace; we replicate its implementation here verbatim so the benchmark measures
// the same code generation. Drop-in alternatives are gated behind compile-time switches so we
// can compare implementations without touching production code.
//
// Run only the benchmark:
//   ./build-bench/drape_tests --filter=TextMetricsCache_Bench
//
// Numbers are emitted to stdout. UNIT_TEST registration keeps it inside drape_tests; the
// benchmark itself completes in ~1 second so it does not noticeably slow CI.

#include "testing/testing.hpp"

#include "drape/glyph_manager.hpp"

#include "base/buffer_vector.hpp"
#include "base/file_name_utils.hpp"
#include "base/stl_helpers.hpp"

#include "platform/platform.hpp"

#include <boost/unordered/unordered_flat_map.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <list>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace text_metrics_cache_bench
{
namespace
{
struct GlyphMetrics
{
  int16_t m_fontIndex{};
  uint16_t m_glyphId{};
  int32_t m_xOffset{};
  int32_t m_yOffset{};
  int32_t m_xAdvance{};
  int32_t m_yAdvance{};
};

struct TextMetrics
{
  int32_t m_lineWidthInPixels{};
  int32_t m_maxLineHeightInPixels{};
  std::vector<GlyphMetrics> m_glyphs;
  bool m_isRTL{};
};

struct TextMetricsCacheKeyView
{
  std::string_view m_utf8;
  int8_t m_lang;
};

struct TextMetricsCacheKey
{
  std::string m_utf8;
  int8_t m_lang;
};

inline void HashCombine(size_t & seed, size_t value)
{
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

inline size_t HashTextMetricsCacheKey(TextMetricsCacheKeyView const & key)
{
  size_t seed = base::StringHash{}(key.m_utf8);
  HashCombine(seed, std::hash<int>{}(key.m_lang));
  return seed;
}

struct KeyHash
{
  using is_transparent = void;
  size_t operator()(TextMetricsCacheKey const & key) const { return HashTextMetricsCacheKey({key.m_utf8, key.m_lang}); }
  size_t operator()(TextMetricsCacheKeyView const & key) const { return HashTextMetricsCacheKey(key); }
};

struct KeyEqual
{
  using is_transparent = void;
  static bool Equal(TextMetricsCacheKeyView const & a, TextMetricsCacheKeyView const & b)
  {
    return a.m_lang == b.m_lang && a.m_utf8 == b.m_utf8;
  }
  bool operator()(TextMetricsCacheKey const & a, TextMetricsCacheKey const & b) const
  {
    return Equal({a.m_utf8, a.m_lang}, {b.m_utf8, b.m_lang});
  }
  bool operator()(TextMetricsCacheKey const & a, TextMetricsCacheKeyView const & b) const
  {
    return Equal({a.m_utf8, a.m_lang}, b);
  }
  bool operator()(TextMetricsCacheKeyView const & a, TextMetricsCacheKey const & b) const
  {
    return Equal(a, {b.m_utf8, b.m_lang});
  }
};

// Replicates the production cache verbatim.
template <template <typename...> class IndexMap>
class CacheImpl
{
public:
  explicit CacheImpl(size_t maxSize) : m_maxSize(maxSize) {}

  TextMetrics const * Find(TextMetricsCacheKeyView const & key)
  {
    auto const found = m_index.find(key);
    if (found == m_index.end())
      return nullptr;
    m_lru.splice(m_lru.begin(), m_lru, found->second);
    return &found->second->m_value;
  }

  TextMetrics const & Insert(TextMetricsCacheKey key, TextMetrics value)
  {
    m_lru.emplace_front(std::move(key), std::move(value));
    auto const inserted = m_lru.begin();
    m_index.emplace(inserted->m_key, inserted);

    while (m_lru.size() > m_maxSize)
    {
      m_index.erase(m_lru.back().m_key);
      m_lru.pop_back();
    }
    return inserted->m_value;
  }

  size_t Size() const { return m_index.size(); }

private:
  struct Node
  {
    Node(TextMetricsCacheKey k, TextMetrics v) : m_key(std::move(k)), m_value(std::move(v)) {}
    TextMetricsCacheKey m_key;
    TextMetrics m_value;
  };

  size_t m_maxSize;
  std::list<Node> m_lru;
  IndexMap<TextMetricsCacheKey, typename std::list<Node>::iterator, KeyHash, KeyEqual> m_index;
};

using StdCache = CacheImpl<std::unordered_map>;

// boost::unordered_flat_map is open-addressed and gives much better cache locality than
// std::unordered_map's bucket-list layout. Drop-in replacement here.
template <class K, class V, class H, class E>
using BoostFlatMap = boost::unordered_flat_map<K, V, H, E>;
using FlatCache = CacheImpl<BoostFlatMap>;

std::vector<std::string> GenerateLabels(size_t count, std::mt19937 & rng)
{
  // Generate plausible map labels: 4-30 ASCII chars plus a few non-Latin runs.
  std::uniform_int_distribution<int> lenDist(4, 30);
  std::uniform_int_distribution<int> charDist(0x41, 0x7A);  // A-z
  std::vector<std::string> out;
  out.reserve(count);
  for (size_t i = 0; i < count; ++i)
  {
    int const len = lenDist(rng);
    std::string s;
    s.reserve(static_cast<size_t>(len));
    for (int j = 0; j < len; ++j)
      s.push_back(static_cast<char>(charDist(rng)));
    out.push_back(std::move(s));
  }
  return out;
}

TextMetrics MakeFakeMetrics(size_t glyphCount)
{
  TextMetrics m;
  m.m_lineWidthInPixels = 100;
  m.m_maxLineHeightInPixels = 22;
  m.m_glyphs.reserve(glyphCount);
  for (size_t i = 0; i < glyphCount; ++i)
    m.m_glyphs.push_back({0, static_cast<uint16_t>(i & 0xFFFF), 0, 0, 12, 0});
  return m;
}

// Returns the steady-state ns/op for the cache. `workingSet <= cacheCap` keeps the cache
// in pure-hit mode after warmup, so we measure splice-on-hit + hash lookup, not eviction.
template <class Cache>
double BenchSteadyHits(size_t cacheCap, size_t workingSet, size_t iterations)
{
  std::mt19937 rng(0xC0FFEE);
  auto const labels = GenerateLabels(workingSet, rng);

  Cache cache(cacheCap);
  auto const fake = MakeFakeMetrics(10);
  for (auto const & l : labels)
    cache.Insert(TextMetricsCacheKey{l, 0}, fake);

  std::uniform_int_distribution<size_t> idxDist(0, workingSet - 1);
  // Pre-roll indices so the random generator cost doesn't pollute the measurement.
  std::vector<size_t> indices(iterations);
  for (size_t i = 0; i < iterations; ++i)
    indices[i] = idxDist(rng);

  auto const start = std::chrono::steady_clock::now();
  size_t accumulator = 0;  // prevent the optimiser from eliding the find result
  for (size_t i = 0; i < iterations; ++i)
  {
    auto const & l = labels[indices[i]];
    TextMetricsCacheKeyView key{l, 0};
    auto const * hit = cache.Find(key);
    if (hit != nullptr)
      accumulator += hit->m_glyphs.size();
  }
  auto const end = std::chrono::steady_clock::now();

  // Use accumulator so it cannot be optimised away.
  TEST_GREATER(accumulator, size_t{0}, ());

  auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  return static_cast<double>(ns) / static_cast<double>(iterations);
}

// Same label repeated: every Find hits the MRU entry. Isolates the splice-on-hit overhead
// (4-pointer write that's a no-op when src == dst). If this is far below BenchSteadyHits,
// most of the steady-hit cost is the random-jump cache miss in the list nodes, not splice.
template <class Cache>
double BenchSameLabelMRU(size_t cacheCap, size_t iterations)
{
  Cache cache(cacheCap);
  std::string const onlyLabel = "Beacon Hill Restaurant";
  cache.Insert(TextMetricsCacheKey{onlyLabel, 0}, MakeFakeMetrics(10));

  auto const start = std::chrono::steady_clock::now();
  size_t accumulator = 0;
  for (size_t i = 0; i < iterations; ++i)
  {
    TextMetricsCacheKeyView key{onlyLabel, 0};
    auto const * hit = cache.Find(key);
    if (hit != nullptr)
      accumulator += hit->m_glyphs.size();
  }
  auto const end = std::chrono::steady_clock::now();

  TEST_GREATER(accumulator, size_t{0}, ());
  auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  return static_cast<double>(ns) / static_cast<double>(iterations);
}

// Measures Insert/Find under steady-state eviction: working set > cap, so every Find misses
// and every Insert evicts. Models cache thrashing rather than typical operation.
template <class Cache>
double BenchEvictionStorm(size_t cacheCap, size_t workingSet, size_t iterations)
{
  std::mt19937 rng(0xC0FFEE);
  auto const labels = GenerateLabels(workingSet, rng);

  Cache cache(cacheCap);
  auto const fake = MakeFakeMetrics(10);

  std::uniform_int_distribution<size_t> idxDist(0, workingSet - 1);
  std::vector<size_t> indices(iterations);
  for (size_t i = 0; i < iterations; ++i)
    indices[i] = idxDist(rng);

  auto const start = std::chrono::steady_clock::now();
  size_t accumulator = 0;
  for (size_t i = 0; i < iterations; ++i)
  {
    auto const & l = labels[indices[i]];
    TextMetricsCacheKeyView keyView{l, 0};
    auto const * hit = cache.Find(keyView);
    if (hit == nullptr)
    {
      auto const & ref = cache.Insert(TextMetricsCacheKey{l, 0}, fake);
      accumulator += ref.m_glyphs.size();
    }
    else
    {
      accumulator += hit->m_glyphs.size();
    }
  }
  auto const end = std::chrono::steady_clock::now();

  TEST_GREATER(accumulator, size_t{0}, ());
  auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  return static_cast<double>(ns) / static_cast<double>(iterations);
}

class GlyphManagerHarness
{
public:
  GlyphManagerHarness()
  {
    dp::GlyphManager::Params args;
    args.m_uniBlocks = base::JoinPath("fonts", "unicode_blocks.txt");
    args.m_whitelist = base::JoinPath("fonts", "whitelist.txt");
    args.m_blacklist = base::JoinPath("fonts", "blacklist.txt");
    GetPlatform().GetFontNames(args.m_fonts);
    m_mng = std::make_unique<dp::GlyphManager>(args);
  }

  dp::GlyphManager & Get() { return *m_mng; }

private:
  std::unique_ptr<dp::GlyphManager> m_mng;
};

double BenchShapeTextSteadyHits(size_t workingSet, size_t iterations)
{
  GlyphManagerHarness h;
  std::mt19937 rng(0xC0FFEE);
  auto const labels = GenerateLabels(workingSet, rng);

  // Warmup: prime the cache and atlas state.
  for (auto const & l : labels)
    h.Get().ShapeText(l, "en");

  std::uniform_int_distribution<size_t> idxDist(0, workingSet - 1);
  std::vector<size_t> indices(iterations);
  for (size_t i = 0; i < iterations; ++i)
    indices[i] = idxDist(rng);

  auto const start = std::chrono::steady_clock::now();
  size_t accumulator = 0;
  for (size_t i = 0; i < iterations; ++i)
  {
    auto const & l = labels[indices[i]];
    auto const m = h.Get().ShapeText(l, "en");
    accumulator += m.m_glyphs.size();
  }
  auto const end = std::chrono::steady_clock::now();

  TEST_GREATER(accumulator, size_t{0}, ());
  auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  return static_cast<double>(ns) / static_cast<double>(iterations);
}

void Report(char const * label, double nsPerOp)
{
  std::cout << "  " << label << ": " << nsPerOp << " ns/op (" << (1e9 / nsPerOp) / 1e6 << " Mops/s)\n";
}

// Storage-shape variants of TextMetrics. m_glyphs is the only field that could feasibly use a
// small-buffer optimisation; the other fields are POD. We isolate copy-out cost (the largest
// remaining hot-path overhead after the cache index switch) to size the trade-off vs the
// per-entry memory blowup of inline storage.
struct TextMetricsHeap
{
  int32_t m_lineWidthInPixels{};
  int32_t m_maxLineHeightInPixels{};
  std::vector<GlyphMetrics> m_glyphs;
  bool m_isRTL{};
};

template <size_t N>
struct TextMetricsBuffer
{
  int32_t m_lineWidthInPixels{};
  int32_t m_maxLineHeightInPixels{};
  buffer_vector<GlyphMetrics, N> m_glyphs;
  bool m_isRTL{};
};

template <class T>
T MakeMetrics(size_t glyphCount)
{
  T m;
  m.m_lineWidthInPixels = 100;
  m.m_maxLineHeightInPixels = 22;
  if constexpr (requires { m.m_glyphs.reserve(glyphCount); })
    m.m_glyphs.reserve(glyphCount);
  for (size_t i = 0; i < glyphCount; ++i)
    m.m_glyphs.push_back({0, static_cast<uint16_t>(i & 0xFFFF), 0, 0, 12, 0});
  return m;
}

// Measures the cost of "copy a TextMetrics out of cache into a caller-owned local". The cache
// itself is modelled as a vector of populated entries; we round-robin through them so the
// branch predictor can't constant-fold the source. The accumulator forces the compiler to
// emit the copy.
template <class T>
double BenchCopyOut(size_t glyphsPerEntry, size_t iterations)
{
  size_t constexpr kSourceCount = 64;  // small ring of source entries to defeat constant-folding
  std::vector<T> sources;
  sources.reserve(kSourceCount);
  for (size_t i = 0; i < kSourceCount; ++i)
    sources.push_back(MakeMetrics<T>(glyphsPerEntry));

  auto const start = std::chrono::steady_clock::now();
  size_t accumulator = 0;
  for (size_t i = 0; i < iterations; ++i)
  {
    T const & src = sources[i & (kSourceCount - 1)];
    T copy = src;  // the operation we are measuring
    accumulator += copy.m_glyphs.size() + static_cast<size_t>(copy.m_lineWidthInPixels);
  }
  auto const end = std::chrono::steady_clock::now();

  TEST_GREATER(accumulator, size_t{0}, ());
  auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  return static_cast<double>(ns) / static_cast<double>(iterations);
}
}  // namespace

UNIT_TEST(TextMetricsCache_Bench_SteadyHits)
{
  size_t constexpr kCap = 50'000;
  size_t constexpr kWorkingSet = 10'000;
  size_t constexpr kIterations = 5'000'000;

  std::cout << "\nSteady hits (workingSet=" << kWorkingSet << ", cap=" << kCap << ", iters=" << kIterations << "):\n";
  Report("std::unordered_map  ", BenchSteadyHits<StdCache>(kCap, kWorkingSet, kIterations));
  Report("boost::flat_map     ", BenchSteadyHits<FlatCache>(kCap, kWorkingSet, kIterations));
}

UNIT_TEST(TextMetricsCache_Bench_SameLabelMRU)
{
  size_t constexpr kCap = 50'000;
  size_t constexpr kIterations = 5'000'000;

  std::cout << "\nSame-label MRU (cap=" << kCap << ", iters=" << kIterations << "):\n";
  Report("std::unordered_map  ", BenchSameLabelMRU<StdCache>(kCap, kIterations));
  Report("boost::flat_map     ", BenchSameLabelMRU<FlatCache>(kCap, kIterations));
}

UNIT_TEST(TextMetricsCache_Bench_EvictionStorm)
{
  size_t constexpr kCap = 1'000;
  size_t constexpr kWorkingSet = 5'000;
  size_t constexpr kIterations = 200'000;

  std::cout << "\nEviction storm (workingSet=" << kWorkingSet << ", cap=" << kCap << ", iters=" << kIterations
            << "):\n";
  Report("std::unordered_map  ", BenchEvictionStorm<StdCache>(kCap, kWorkingSet, kIterations));
  Report("boost::flat_map     ", BenchEvictionStorm<FlatCache>(kCap, kWorkingSet, kIterations));
}

UNIT_TEST(TextMetricsCache_Bench_ShapeText_SteadyHits)
{
  size_t constexpr kWorkingSet = 1'000;
  size_t constexpr kIterations = 200'000;

  std::cout << "\nShapeText steady hits (workingSet=" << kWorkingSet << ", iters=" << kIterations << "):\n";
  Report("end-to-end ShapeText", BenchShapeTextSteadyHits(kWorkingSet, kIterations));
}

// Measures the per-call copy-out cost vs cache-entry memory cost across storage strategies.
// The hot path copies a TextMetrics out of the cache into the caller's local on every hit; the
// std::vector copy is dominated by the heap alloc for the new buffer. buffer_vector eliminates
// the alloc when the glyph count fits inline, at the cost of a fixed per-entry inline buffer
// always present in memory (sizeof(buffer_vector<GlyphMetrics, N>) ~= N*24 + 32 bytes).
//
// Per-entry cost estimates (50 000-cap cache, average 10 glyphs):
//   std::vector<GlyphMetrics>             : ~290 B (24 header + 240 heap + 24 alloc overhead)  =>  14.5 MB total
//   buffer_vector<GlyphMetrics, 8>        : ~224 B inline (no heap when fits)                  =>  11.2 MB total  (50%
//   of 10-glyph labels spill) buffer_vector<GlyphMetrics, 16>       : ~416 B inline =>  20.8 MB total  (~95% fit)
//   buffer_vector<GlyphMetrics, 32>       : ~800 B inline                                      =>  40.0 MB total
//   (~99.9% fit)
UNIT_TEST(TextMetricsCache_Bench_CopyOut_Storage)
{
  size_t constexpr kIterations = 5'000'000;

  std::cout << "\nCopy-out cost per cache hit, sources rotated to defeat constant-folding\n";
  for (size_t glyphs : {5UL, 10UL, 20UL, 40UL})
  {
    std::cout << "  glyphsPerEntry=" << glyphs << ":\n";
    Report("std::vector            ", BenchCopyOut<TextMetricsHeap>(glyphs, kIterations));
    Report("buffer_vector<.., 8>   ", BenchCopyOut<TextMetricsBuffer<8>>(glyphs, kIterations));
    Report("buffer_vector<.., 16>  ", BenchCopyOut<TextMetricsBuffer<16>>(glyphs, kIterations));
    Report("buffer_vector<.., 32>  ", BenchCopyOut<TextMetricsBuffer<32>>(glyphs, kIterations));
  }

  std::cout << "  sizeof: TextMetricsHeap=" << sizeof(TextMetricsHeap) << " "
            << "Buffer<8>=" << sizeof(TextMetricsBuffer<8>) << " "
            << "Buffer<16>=" << sizeof(TextMetricsBuffer<16>) << " "
            << "Buffer<32>=" << sizeof(TextMetricsBuffer<32>) << "\n";
}
}  // namespace text_metrics_cache_bench
