#ifndef SZ3_COMPRESSOR_TYPE_ONE_HPP
#define SZ3_COMPRESSOR_TYPE_ONE_HPP

#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include "SZ3/compressor/Compressor.hpp"
#include "SZ3/decomposition/Decomposition.hpp"
#include "SZ3/def.hpp"
#include "SZ3/encoder/Encoder.hpp"
#include "SZ3/lossless/Lossless.hpp"
#include "SZ3/utils/Config.hpp"
#include "SZ3/utils/FileUtil.hpp"
#include "SZ3/utils/Timer.hpp"

namespace SZ3 {

/**
 * SZGenericCompressor glues together decomposition, encoder, and lossless modules to form the compressor.
 * It only takes Decomposition, not Predictor.
 * @tparam T original data type
 * @tparam N original data dimension
 * @tparam Decomposition decomposition module
 * @tparam Encoder encoder module
 * @tparam Lossless lossless module
 */
template <class T, uint N, class Decomposition, class Encoder, class Lossless>
class SZGenericCompressor : public concepts::CompressorInterface<T> {
   public:
    SZGenericCompressor(Decomposition decomposition_, Encoder encoder_, Lossless lossless_)
        : decomposition(decomposition_), encoder(encoder_), lossless(lossless_) {
        static_assert(std::is_base_of<concepts::DecompositionInterface<T, int, N>, Decomposition>::value,
                      "must implement the frontend interface");
        static_assert(std::is_base_of<concepts::EncoderInterface<int>, Encoder>::value,
                      "must implement the encoder interface");
        static_assert(std::is_base_of<concepts::LosslessInterface, Lossless>::value,
                      "must implement the lossless interface");
    }

    size_t compress(const Config &conf, T *data, uchar *cmpData, size_t cmpCap) override {
        std::vector<int> quant_inds = decomposition.compress(conf, data);

        if (decomposition.get_out_range().first != 0) {
            throw std::runtime_error("The output range of the decomposition must start from 0 for this compressor");
        }
        encoder.preprocess_encode(quant_inds, decomposition.get_out_range().second);
        size_t bufferSize = std::max<size_t>(
            1000, 2 * (decomposition.size_est() + encoder.size_est() + sizeof(int) * quant_inds.size()));

        // Owned, because the encoder and the lossless layer below can throw.
        std::unique_ptr<uchar[]> buffer_owner(new uchar[bufferSize]);
        uchar *const buffer = buffer_owner.get();
        uchar *buffer_pos = buffer;

        decomposition.save(buffer_pos);
        encoder.save(buffer_pos);

        // store the size of quant_inds is necessary as it is not always equal to conf.num
        write<uint64_t>(quant_inds.size(), buffer_pos);
        encoder.encode(quant_inds, buffer_pos);
        encoder.postprocess_encode();
        
        auto cmpSize = lossless.compress(buffer, buffer_pos - buffer, cmpData, cmpCap);

        return cmpSize;
    }

    T *decompress(const Config &conf, uchar const *cmpData, size_t cmpSize, T *decData) override {
        uchar *buffer = nullptr;
        size_t bufferSize = lossless.decompress(cmpData, cmpSize, buffer, 0);

        // malloc'd by the lossless layer, hence free(). Owned, because the parsing below can throw.
        std::unique_ptr<uchar, void (*)(void *)> buffer_owner(buffer, &free);

        uchar const *bufferPos = buffer;

        decomposition.load(bufferPos, bufferSize);
        encoder.load(bufferPos, bufferSize);

        uint64_t quant_inds_size = 0;
        read(quant_inds_size, bufferPos, bufferSize);
        auto quant_inds = encoder.decode(bufferPos, quant_inds_size, bufferSize);
        encoder.postprocess_decode();

        // The remaining work uses `quant_inds` and `decData` only, so release the internal buffer now.
        buffer_owner.reset();

        decomposition.decompress(conf, quant_inds, decData);
        return decData;
    }

   private:
    Decomposition decomposition;
    Encoder encoder;
    Lossless lossless;
};

template <class T, uint N, class Decomposition, class Encoder, class Lossless>
std::shared_ptr<SZGenericCompressor<T, N, Decomposition, Encoder, Lossless>> make_compressor_sz_generic(
    Decomposition decomposition, Encoder encoder, Lossless lossless) {
    return std::make_shared<SZGenericCompressor<T, N, Decomposition, Encoder, Lossless>>(decomposition, encoder,
                                                                                         lossless);
}

}  // namespace SZ3
#endif
