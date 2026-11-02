#pragma once

// The SZ3 raster frame profile shared by the TWM writer and the reader. Include it only from
// the strict floating point translation units: the integer heights must reconstruct identically.

#include <SZ3/compressor/SZGenericCompressor.hpp>
#include <SZ3/decomposition/InterpolationDecomposition.hpp>
#include <SZ3/encoder/HuffmanEncoder.hpp>
#include <SZ3/lossless/Lossless_bypass.hpp>
#include <SZ3/quantizer/LinearQuantizer.hpp>
#include <SZ3/utils/Config.hpp>

#include <cstdint>

namespace terrain::sz3
{
// RasterCodec::Sz3 frame: a FrameLossless byte, then the SZ3 interpolation stream of the patch
// nodes as floats, as is or deflated (a varuint stream length, then the zlib stream). SZ3 selects
// interpolation for 2D data; the stream stores the interpolation, direction, level error bound
// parameters and the quantizer, the decoder only supplies the patch dimensions.
enum class FrameLossless : uint8_t
{
  Bypass = 0,
  Deflate = 1
};

using Quantizer = SZ3::LinearQuantizer<float>;
using Decomposition = SZ3::InterpolationDecomposition<float, 2, Quantizer>;
using Compressor = SZ3::SZGenericCompressor<float, 2, Decomposition, SZ3::HuffmanEncoder<int>, SZ3::Lossless_bypass>;

// Rows run south to north, x fastest: SZ3 takes the slowest dimension first.
inline SZ3::Config MakeConfig(uint32_t nodesX, uint32_t nodesY)
{
  SZ3::Config conf(nodesY, nodesX);
  conf.cmprAlgo = SZ3::ALGO_INTERP;
  conf.errorBoundMode = SZ3::EB_ABS;
  // The SZ_compress_Interp default for 2D data.
  conf.interpAnchorStride = 128;
  return conf;
}

// The decoder loads the quantizer from the stream.
inline Compressor MakeCompressor(Quantizer quantizer = {})
{
  return Compressor(SZ3::make_decomposition_interpolation<float, 2>(SZ3::Config(), quantizer),
                    SZ3::HuffmanEncoder<int>(), SZ3::Lossless_bypass());
}
}  // namespace terrain::sz3
