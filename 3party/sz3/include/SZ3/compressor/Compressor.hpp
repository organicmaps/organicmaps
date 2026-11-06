#ifndef SZ3_COMPRESSOR_HPP
#define SZ3_COMPRESSOR_HPP

#include "SZ3/def.hpp"
#include "SZ3/utils/Config.hpp"

namespace SZ3::concepts {

/**
 * Compressor describes the whole compression workflow.
 * Compressor is implemented using modules (decomposition or predictor, encoder, lossless, etc.).
 * @tparam T input data type
 */
template <class T>
class CompressorInterface {
   public:
    virtual ~CompressorInterface() = default;

    /**
     * decompress data
     * @param cmpData compressed data in bytes
     * @param cmpSize size of compressed data in bytes
     * @param decData pre-allocated space to store the decompress data
     * @return pointer to the decompress data
     */
    virtual T *decompress(const Config &conf, uchar const *cmpData, size_t cmpSize, T *decData) = 0;

    /**
     * compress data
     * @param conf compression configuration
     * @param data input data in original format
     * @param cmpData compressed data in bytes
     * @param cmpCap size of the compressed buffer in bytes
     * @return size of the compressed data in bytes
     */
    virtual size_t compress(const Config &conf, T *data, uchar *cmpData, size_t cmpCap) = 0;
};
}  // namespace SZ3::concepts

#endif
