#include "testing/testing.hpp"

#include "indexer/terrain/sz3_decoder.hpp"
#include "indexer/terrain/sz3_profile.hpp"
#include "indexer/terrain/terrain_serdes.hpp"

#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include <zlib.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <random>
#include <thread>
#include <vector>

namespace sz3_decoder_tests
{
using Altitude = geometry::Altitude;

std::vector<uint8_t> EncodeFrame(SZ3::Config const & config, std::vector<float> & values, int radius = 32768)
{
  std::vector<uint8_t> frame(1 + 8 * values.size() + 4096);
  frame.front() = static_cast<uint8_t>(terrain::sz3::FrameLossless::Bypass);
  auto compressor = terrain::sz3::MakeCompressor(terrain::sz3::Quantizer(config.absErrorBound, radius));
  frame.resize(1 + compressor.compress(config, values.data(), frame.data() + 1, frame.size() - 1));
  return frame;
}

UNIT_TEST(TwmSz3_IntegerRoundingBoundaries)
{
  uint32_t constexpr kNodes = terrain::kMaxRasterCells + 1;
  Altitude constexpr kMin = std::numeric_limits<Altitude>::min(), kMax = std::numeric_limits<Altitude>::max();
  auto config = terrain::sz3::MakeConfig(kNodes, kNodes);
  config.interpAlgo = SZ3::INTERP_ALGO_LINEAR;
  // A small radius/tolerance stores inexact predictions as lossless unpredictable samples.
  config.absErrorBound = 1e-12;
  std::vector<float> source = {float(kMin) - 1, float(kMin), float(kMax), float(kMax) + 1};
  auto const check = [&]
  {
    source.resize(config.num, source.back());
    auto values = source;
    auto const frame = EncodeFrame(config, values, 2);
    TEST_EQUAL(values, source, ("The rounding fixture must reconstruct every float exactly"));
    auto const decoded = terrain::DecodeSz3(frame, kNodes, kNodes, kMin, kMax);
    for (size_t i = 0; i < source.size(); ++i)
      TEST_EQUAL(decoded[i], std::lround(std::clamp(source[i], float(kMin), float(kMax))), (i, source[i]));
    source.clear();
  };
  // Exercise every signed int16 half-integer threshold, its adjacent floats, and endpoint clamping.
  for (int i = kMin; i <= kMax; ++i)
  {
    float const midpoint = i + 0.5f;
    for (float const value : {std::nextafter(midpoint, -INFINITY), midpoint, std::nextafter(midpoint, INFINITY)})
    {
      source.push_back(value);
      if (source.size() == config.num)
        check();
    }
  }
  if (!source.empty())
    check();
}

UNIT_TEST(TwmSz3_WorkspaceReuseAndThreads)
{
  struct Case
  {
    uint32_t m_nodesX, m_nodesY;
    Altitude m_min, m_max;
    std::vector<uint8_t> m_frame;
    std::vector<Altitude> m_expected;
  };
  std::vector<Case> cases;
  std::mt19937 random(20261024);
  for (auto const [nx, ny] : {std::pair{129u, 129u}, {2u, 2u}, {2u, 129u}, {129u, 2u}, {17u, 128u}, {65u, 33u}})
    for (int pattern = 0; pattern < 3; ++pattern)
    {
      std::vector<float> values(size_t{nx} * ny);
      for (size_t i = 0; i < values.size(); ++i)
        values[i] = pattern == 0 ? -500.0f
                  : pattern == 1 ? float(int(random() % 65536) - 32768)
                                 : float(2 * int(i % nx) - 3 * int(i / nx));
      auto const [minimum, maximum] = std::minmax_element(values.begin(), values.end());
      Case input{nx, ny, static_cast<Altitude>(*minimum), static_cast<Altitude>(*maximum), {}, {}};
      auto config = terrain::sz3::MakeConfig(nx, ny);
      config.absErrorBound = 3.49;
      config.interpAlgo = pattern % 2;
      config.interpDirection = cases.size() % 2;
      // Alternate many unpredictable samples with patches containing none.
      input.m_frame = EncodeFrame(config, values, pattern == 1 ? 2 : 32768);
      for (float value : values)
        input.m_expected.push_back(
            static_cast<Altitude>(std::lround(std::clamp(value, float(input.m_min), float(input.m_max)))));
      if (cases.size() % 2)
      {
        auto const rawSize = static_cast<uint32_t>(input.m_frame.size() - 1);
        auto size = compressBound(rawSize);
        std::vector<uint8_t> compressed(size);
        TEST_EQUAL(compress2(compressed.data(), &size, input.m_frame.data() + 1, rawSize, Z_BEST_COMPRESSION), Z_OK,
                   ());
        input.m_frame.clear();
        MemWriter writer(input.m_frame);
        WriteToSink(writer, static_cast<uint8_t>(terrain::sz3::FrameLossless::Deflate));
        WriteVarUint(writer, rawSize);
        writer.Write(compressed.data(), size);
      }
      cases.push_back(std::move(input));
    }
  std::atomic<bool> matched = true;
  auto const check = [&](size_t offset)
  {
    auto const decode = [](Case const & c)
    { return terrain::DecodeSz3(c.m_frame, c.m_nodesX, c.m_nodesY, c.m_min, c.m_max); };
    auto const retained = decode(cases.front());
    for (size_t i = 0; i < 3 * cases.size(); ++i)
    {
      auto const & c = cases[(i + offset) % cases.size()];
      if (decode(c) != c.m_expected)
        matched = false;
    }
    if (retained != cases.front().m_expected)
      matched = false;
  };
  check(0);
  {
    std::vector<std::jthread> workers;
    for (size_t i = 0; i < 4; ++i)
      workers.emplace_back(check, i);
  }
  TEST(matched.load(), ("Reusing a workspace must preserve heights and returned vectors on every thread"));
}

UNIT_TEST(TwmSz3_HuffmanBufferReuse)
{
  std::vector<std::vector<int>> cases = {{}, {7}, {-1000000, 0, 3, 1000000, -1000000}, {4, 5, 4, 5, 4, 5}};
  cases.emplace_back(8192, -3);
  for (size_t i = 0; i < cases.back().size(); i += 257)
    cases.back()[i] = 17;
  cases.emplace_back(513, -32768);
  SZ3::HuffmanEncoder<int> decoder;
  std::vector<int> output;
  for (int repeat = 0; repeat < 3; ++repeat)
    for (auto const & bins : cases)
    {
      SZ3::HuffmanEncoder<int> encoder;
      encoder.preprocess_encode(bins, 0);
      std::vector<uint8_t> bytes(encoder.size_bound(bins.size(), bins.size()));
      auto * end = bytes.data();
      encoder.save(end);
      encoder.encode(bins, end);
      bytes.resize(end - bytes.data());
      auto const * cursor = bytes.data();
      size_t remaining = bytes.size();
      decoder.load(cursor, remaining);
      decoder.decode_into(cursor, bins.size(), remaining, output);
      TEST_EQUAL(output, bins, (repeat));
      TEST_EQUAL(remaining, 0, ());
      // Preserve the original allocating API as well as the new reusable-buffer API.
      cursor = bytes.data();
      remaining = bytes.size();
      decoder.load(cursor, remaining);
      TEST_EQUAL(decoder.decode(cursor, bins.size(), remaining), bins, (repeat));
      TEST_EQUAL(remaining, 0, ());
    }
}

UNIT_TEST(TwmSz3_InterpolationProfiles)
{
  Altitude constexpr kMin = std::numeric_limits<Altitude>::min(), kMax = std::numeric_limits<Altitude>::max();
  std::mt19937 random(20261024);
  for (auto const [nx, ny] :
       {std::pair{2u, 2u}, {2u, 129u}, {129u, 2u}, {3u, 5u}, {5u, 3u}, {17u, 128u}, {128u, 17u}, {129u, 129u}})
  {
    std::vector<float> source(size_t{nx} * ny);
    for (auto & value : source)
      value = float(int(random() % 65536) - 32768);
    for (auto const interpolation : {SZ3::INTERP_ALGO_LINEAR, SZ3::INTERP_ALGO_CUBIC})
      for (uint8_t const direction : {0, 1})
        for (auto const [alpha, beta] : {std::pair{1.25, 2.0}, {1.0, 1.0}, {1.5, 2.5}, {2.0, 3.0}})
          for (int const radius : {2, 32768})
          {
            auto config = terrain::sz3::MakeConfig(nx, ny);
            config.absErrorBound = 3.49;
            config.interpAlgo = interpolation;
            config.interpDirection = direction;
            config.interpAlpha = alpha;
            config.interpBeta = beta;
            auto expected = source;
            // The encoder retains the generic block traversal, providing an independent order check.
            auto const frame = EncodeFrame(config, expected, radius);
            std::vector<float> floats(expected.size());
            auto compressor = terrain::sz3::MakeCompressor();
            compressor.decompress(config, frame.data() + 1, frame.size() - 1, floats.data());
            TEST_EQUAL(std::memcmp(floats.data(), expected.data(), floats.size() * sizeof(float)), 0,
                       (nx, ny, interpolation, direction, alpha, beta, radius));
            auto const heights = terrain::DecodeSz3(frame, nx, ny, kMin, kMax);
            for (size_t i = 0; i < heights.size(); ++i)
              TEST_EQUAL(heights[i], std::lround(std::clamp(expected[i], float(kMin), float(kMax))), (i));
          }
  }
}
}  // namespace sz3_decoder_tests
