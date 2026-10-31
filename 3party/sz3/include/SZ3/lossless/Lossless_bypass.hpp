//
// Created by Kai Zhao on 4/21/20.
//

#ifndef SZ3_LOSSLESS_BYPASS_HPP
#define SZ3_LOSSLESS_BYPASS_HPP

#include <cstring>
#include <stdexcept>

#include "SZ3/def.hpp"
#include "SZ3/lossless/Lossless.hpp"

namespace SZ3 {
class Lossless_bypass : public concepts::LosslessInterface {
public:
    /**
     * compress data with lossless compressors
     * @param src  data to be compressed
     * @param srcLen length (in bytes) of the data to be compressed
     * @param dst compressed data
     * @param dstCap capacity (in bytes) for storing the compressed data
     * @return length (in bytes) of the data compressed
     */
    size_t compress(const uchar *src, size_t srcLen, uchar *dst, size_t dstCap) override {
        if (dstCap < srcLen) {
            throw std::length_error(SZ3_ERROR_COMP_BUFFER_NOT_LARGE_ENOUGH);
        }
        std::memcpy(dst, src, srcLen);
        return srcLen;
    }

    /**
     * reverse of compress(), decompress the data with lossless compressors
     * @param src data to be decompressed
     * @param srcLen length (in bytes) of that data
     * @param dst buffer to decompress into; when null on entry the callee allocates it with malloc()
     *            and the caller frees it
     * @param dstCap the capacity of dst, ignored when dst is null
     * @return length (in bytes) of the data decompressed
     */
    size_t decompress(const uchar *src, size_t srcLen, uchar *&dst, size_t dstCap) override {
        // malloc, because the caller frees what it gets back with free().
        if (dst == nullptr) {
            dst = static_cast<uchar *>(malloc(srcLen));
            if (dst == nullptr) {
                throw std::runtime_error("SZ3 bypass lossless: can not allocate the decompression buffer");
            }
        } else if (srcLen > dstCap) {
            throw std::out_of_range("SZ3 bypass lossless: payload exceeds the allowed capacity");
        }
        std::memcpy(dst, src, srcLen);
        return srcLen;
    }
};
}  // namespace SZ3
#endif  // SZ_LOSSLESS_BYPASS_HPP
