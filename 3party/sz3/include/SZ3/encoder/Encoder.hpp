#ifndef SZ3_ENCODER_HPP
#define SZ3_ENCODER_HPP

#include <stdexcept>
#include <vector>

#include "SZ3/def.hpp"
namespace SZ3 {
namespace concepts {

/**
 * Encoder changes the input to a more compact representative
 * Usually this step is lossless instead of lossy
 * Examples: huffman, runlenth, etc.
 * @tparam T bin type
 */
template <class T, class TAllocator = std::allocator<T>>
class EncoderInterface {
   public:
    virtual ~EncoderInterface() = default;

    /**
     * init the encoder
     * E.g., Huffman will build tree in this step
     * @param bins to-be-encoded integers
     * @param stateNum stateNum > 0 indicates the bins has a range of [0, stateNum). stateNum == 0 means no such
     * guarantee
     */
    virtual void preprocess_encode(const std::vector<T, TAllocator> &bins, int stateNum) = 0;

    /**
     * encode the input (in vector<T> format) to a more compact representative(in byte stream format)
     * @param bins input in vector
     * @param bytes output in byte stream
     * @return size of output (# of bytes)
     */
    virtual size_t encode(const std::vector<T, TAllocator> &bins, uchar *&bytes) = 0;

    /**
     * reverse of encode()
     *
     * `targetLength` says when to stop producing, `remaining_length` says how far the reads may go;
     * an entropy-coded stream has no terminator, so neither number substitutes for the other.
     *
     * @param bytes input in byte stream
     * @param targetLength size of the output vector
     * @param remaining_length bytes readable from `bytes`; decremented by what is consumed
     * @return output in vector
     */
    virtual std::vector<T, TAllocator> decode(const uchar *&bytes, size_t targetLength, size_t &remaining_length) = 0;

    /**
     * serialize the encoder and store it to a buffer
     * @param c One large buffer is pre-allocated, and the start location of the serialized encoder in the buffer is
     * indicated by c. After saving the encoder to the buffer, this function should change c to indicate the next empty
     * location in the buffer
     */
    virtual void save(uchar *&c) = 0;

    /**
     * deserialize the encoder from a buffer
     * @param c start location of the encoder in the buffer
     * @param remaining_length the remaining length of the buffer
     */
    virtual void load(const uchar *&c, size_t &remaining_length) = 0;

    virtual void postprocess_decode() = 0;

    virtual void postprocess_encode() = 0;

    virtual void preprocess_decode() = 0;

    /// Upper bound on the space save() needs for the current input.
    virtual size_t size_est() { return 0; }
};
}  // namespace concepts
}  // namespace SZ3
#endif
