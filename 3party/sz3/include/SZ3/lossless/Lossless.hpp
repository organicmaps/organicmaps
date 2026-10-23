//
// Created by Kai Zhao on 6/12/20.
//

#ifndef SZ3_LOSSLESS_HPP
#define SZ3_LOSSLESS_HPP

#include <cstddef>

#include "SZ3/def.hpp"

namespace SZ3::concepts {

/**
 * Lossless compressors is used in addition to lossy compression to further reduce the data size
 * Usually this module calls into existing lossless compress APIs, instead of re-implementing the lossless algorithms.
 */
class LosslessInterface {
   public:
    virtual ~LosslessInterface() = default;

    /**
     * compress data with lossless compressors
     * @param src  data to be compressed
     * @param srcLen length (in bytes) of the data to be compressed
     * @param dst compressed data
     * @param dstCap capacity (in bytes) for storing the compressed data
     * @return length (in bytes) of the data compressed
     */
    virtual size_t compress(const uchar *src, size_t srcLen, uchar *dst, size_t dstCap) = 0;

    /**
     * reverse of compress(), decompress the data with lossless compressors
     * @param src data to be decompressed
     * @param srcLen length (in bytes) of that data
     * @param dst buffer to decompress into; when null on entry the callee allocates it with malloc()
     *            and the caller frees it
     * @param dstCap the capacity of dst, ignored when dst is null
     * @return length (in bytes) of the data decompressed
     */
    virtual size_t decompress(const uchar *src, size_t srcLen, uchar *&dst, size_t dstCap) = 0;
};
}  // namespace SZ3::concepts

#endif  // SZ_LOSSLESS_HPP
