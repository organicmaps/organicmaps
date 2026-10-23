#ifndef SZ3_HUFFMAN_ENCODER_HPP
#define SZ3_HUFFMAN_ENCODER_HPP

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "SZ3/def.hpp"
#include "SZ3/encoder/Encoder.hpp"
#include "SZ3/utils/ByteUtil.hpp"
#include "SZ3/utils/MemoryUtil.hpp"

namespace SZ3 {

/**
 * Canonical Huffman coder for integer bins.
 *
 * save() writes the number of distinct bins D as a uint32; if D > 0, the smallest bin; if D > 1, for each bin in
 * ascending order the Elias-gamma code of its gap from the one before (none for the first), then the Elias-gamma
 * code of 1 + the zigzag of its code length minus the previous one's (0 before the first). encode() writes the
 * payload's bit count as a uint64, then the codes MSB-first.
 *
 * Code lengths, capped at 32 bits, are those of a Huffman tree built in (frequency, bin) order, so every platform
 * writes the same bytes.
 */
template <class T>
class HuffmanEncoder : public concepts::EncoderInterface<T> {
    static_assert(std::is_integral<T>::value && sizeof(T) <= 8, "bins must be an integer type of at most 64 bits");
    using U = typename std::make_unsigned<T>::type;
    static constexpr unsigned kMaxLen = 32;
    static constexpr unsigned kTableBits = 12;

   public:
    /// stateNum is ignored: the range is taken from the bins.
    void preprocess_encode(const std::vector<T> &bins, int /*stateNum*/) override {
        *this = HuffmanEncoder();
        const size_t n = bins.size();
        if (n == 0) return;
        // Locals, not offset_: a store to a member of type T may alias the bins and stop the loop vectorising.
        T mn = bins[0], mx = bins[0];
        for (size_t i = 0; i < n; i++) {
            mn = bins[i] < mn ? bins[i] : mn;
            mx = bins[i] > mx ? bins[i] : mx;
        }
        offset_ = mn;
        const uint64_t top = static_cast<U>(static_cast<U>(mx) - static_cast<U>(mn));
        // A table indexed by bin costs its span to clear; past twice the input a hash on the bin is cheaper.
        dense_ = top < std::max<uint64_t>(4096, 2 * static_cast<uint64_t>(n));
        std::vector<uint64_t> freq;
        if (dense_) {
            count_dense(bins, top + 1, freq);
        } else {
            count_sparse(bins, freq);
        }
        build_lengths(freq);
        build_codes();
    }

    size_t size_est() override {
        if (syms_.empty()) return sizeof(uint32_t);
        return sizeof(uint32_t) + sizeof(T) + (header_bits_ + 7) / 8;
    }

    /// Upper bound of save() plus encode() for num_bins bins holding at most distinct_symbols values.
    static size_t size_bound(size_t num_bins, size_t distinct_symbols) {
        const size_t d = std::min(num_bins, distinct_symbols);
        return sizeof(uint32_t) + sizeof(T) + (d * (16 * sizeof(T) + 10) + 7) / 8 + sizeof(uint64_t) +
               (num_bins * kMaxLen + 7) / 8;
    }

    void save(uchar *&c) override {
        write(static_cast<uint32_t>(syms_.size()), c);
        if (syms_.empty()) return;
        write(offset_, c);
        if (syms_.size() == 1) return;
        BitWriter w(c);
        for (size_t i = 0; i < syms_.size(); i++) {
            if (i > 0) w.gamma(syms_[i] - syms_[i - 1]);
            w.gamma(zigzag(lens_[i] - (i ? lens_[i - 1] : 0)) + 1);
        }
        c = w.finish();
    }

    /// bins must be the ones preprocess_encode() saw.
    size_t encode(const std::vector<T> &bins, uchar *&bytes) override {
        uchar *const start = bytes;
        write(payload_bits_, bytes);
        if (syms_.size() < 2) {
            for (const T b : bins)
                if (syms_.empty() || b != offset_)
                    throw std::invalid_argument("SZ3 Huffman: bin not seen by preprocess_encode");
            return bytes - start;
        }
        uint64_t acc = 0;
        unsigned nb = 0;
        uchar *p = bytes;
        const U off = static_cast<U>(offset_);
        auto put = [&](uint64_t e) {
            const unsigned len = static_cast<unsigned>(e & 0xff);
            acc = (acc << len) | (e >> 8);
            nb += len;
            if (nb >= 32) {
                nb -= 32;
                int32ToBytes_bigEndian(p, static_cast<uint32_t>(acc >> nb));
                p += 4;
            }
        };
        if (dense_) {
            const uint64_t *tab = code_.data();
            const uint64_t size = code_.size();
            for (const T b : bins) {
                const uint64_t s = static_cast<U>(static_cast<U>(b) - off);
                if (s >= size) throw std::invalid_argument("SZ3 Huffman: bin not seen by preprocess_encode");
                put(tab[s]);
            }
        } else {
            for (const T b : bins) put(map_.get(static_cast<U>(static_cast<U>(b) - off)));
        }
        const uint64_t bits = static_cast<uint64_t>(p - bytes) * 8 + nb;
        while (nb >= 8) {
            nb -= 8;
            *p++ = static_cast<uchar>(acc >> nb);
        }
        if (nb) *p++ = static_cast<uchar>(acc << (8 - nb));
        // A bin preprocess_encode() did not count has a zero-length entry.
        if (bits != payload_bits_) throw std::invalid_argument("SZ3 Huffman: bins differ from preprocess_encode's");
        bytes = p;
        return bytes - start;
    }

    void postprocess_encode() override {}

    void preprocess_decode() override {}

    void load(const uchar *&c, size_t &remaining_length) override {
        *this = HuffmanEncoder();
        const uchar *p = c;
        size_t rem = remaining_length;
        uint32_t d = 0;
        read(d, p, rem);
        if (d > 0) read(offset_, p, rem);
        if (d == 1) {
            syms_.assign(1, 0);
            lens_.assign(1, 0);
        } else if (d > 1) {
            // Every bin after the first takes at least two bits of the table.
            if (d - 1 > rem * 4) throw std::out_of_range("SZ3 Huffman: code table exceeds the buffer");
            BitReader r(p, rem);
            read_table(r, d);
            const size_t used = r.bytes();
            p += used;
            rem -= used;
        }
        build_decoder();
        remaining_length = rem;
        c = p;
    }

    std::vector<T> decode(const uchar *&bytes, size_t targetLength, size_t &remaining_length) override {
        const uchar *p = bytes;
        size_t rem = remaining_length;
        uint64_t bits = 0;
        read(bits, p, rem);
        if (bits > static_cast<uint64_t>(rem) * 8) throw std::out_of_range("SZ3 Huffman: payload exceeds the buffer");
        const size_t nbytes = static_cast<size_t>((bits + 7) / 8);
        std::vector<T> out;
        if (syms_.size() < 2) {
            if (bits != 0 || (syms_.empty() && targetLength != 0))
                throw std::out_of_range("SZ3 Huffman: payload does not match the code table");
            out.assign(targetLength, offset_);
        } else {
            if (bits < targetLength || bits / kMaxLen > targetLength)
                throw std::out_of_range("SZ3 Huffman: payload does not match the value count");
            out.resize(targetLength);
            // A one-bit code is all zeros, so a run of zero bits is a run of its symbol.
            if (count_[1])
                decode_payload<true>(p, nbytes, bits, out.data(), targetLength);
            else
                decode_payload<false>(p, nbytes, bits, out.data(), targetLength);
        }
        bytes = p + nbytes;
        remaining_length = rem - nbytes;
        return out;
    }

    void postprocess_decode() override {}

   private:
    class BitWriter {
       public:
        explicit BitWriter(uchar *p) : p_(p) {}
        void put(uint64_t v, unsigned n) {  // n <= 32
            acc_ = (acc_ << n) | (v & ((uint64_t(1) << n) - 1));
            nb_ += n;
            while (nb_ >= 8) {
                nb_ -= 8;
                *p_++ = static_cast<uchar>(acc_ >> nb_);
            }
        }
        void gamma(uint64_t v) {  // v >= 1
            unsigned n = 64 - clz64(v);
            for (unsigned z = n - 1; z > 0; z -= std::min(z, 32u)) put(0, std::min(z, 32u));
            if (n > 32) {
                put(v >> 32, n - 32);
                n = 32;
            }
            put(v, n);
        }
        uchar *finish() {
            if (nb_) *p_++ = static_cast<uchar>(acc_ << (8 - nb_));
            return p_;
        }

       private:
        uchar *p_;
        uint64_t acc_ = 0;
        unsigned nb_ = 0;
    };

    class BitReader {
       public:
        BitReader(const uchar *p, size_t n) : p_(p), n_(n) {}
        unsigned bit() {
            if (pos_ >= n_ * 8) throw std::out_of_range("SZ3 Huffman: truncated code table");
            const unsigned b = (p_[pos_ >> 3] >> (7 - (pos_ & 7))) & 1;
            pos_++;
            return b;
        }
        uint64_t gamma() {
            unsigned z = 0;
            while (bit() == 0)
                if (++z > 63) throw std::out_of_range("SZ3 Huffman: invalid gamma code");
            uint64_t v = 1;
            for (unsigned i = 0; i < z; i++) v = (v << 1) | bit();
            return v;
        }
        size_t bytes() const { return (pos_ + 7) / 8; }

       private:
        const uchar *p_;
        size_t n_;
        size_t pos_ = 0;
    };

    static unsigned clz64(uint64_t v) {  // v != 0
#if defined(__GNUC__) || defined(__clang__)
        return static_cast<unsigned>(__builtin_clzll(v));
#else
        unsigned n = 0;
        for (unsigned s = 32; s > 0; s >>= 1)
            if (!(v >> (64 - s))) {
                n += s;
                v <<= s;
            }
        return n;
#endif
    }

    static uint64_t gamma_bits(uint64_t v) { return 2 * (64 - clz64(v)) - 1; }

    static uint64_t zigzag(int d) { return d >= 0 ? 2 * static_cast<uint64_t>(d) : 2 * static_cast<uint64_t>(-d) - 1; }

    void read_table(BitReader &r, size_t d) {
        syms_.resize(d);
        lens_.resize(d);
        // offset_ + bin must stay a T.
        const uint64_t max_sym =
            static_cast<U>(static_cast<U>(std::numeric_limits<T>::max()) - static_cast<U>(offset_));
        uint64_t s = 0;
        int prev = 0;
        for (size_t i = 0; i < d; i++) {
            if (i > 0) {
                const uint64_t g = r.gamma();
                if (g > max_sym - s) throw std::out_of_range("SZ3 Huffman: bin out of range");
                s += g;
            }
            const uint64_t z = r.gamma() - 1;
            if (z > 2 * kMaxLen) throw std::out_of_range("SZ3 Huffman: invalid code length");
            const int len = prev + ((z & 1) ? -static_cast<int>((z + 1) / 2) : static_cast<int>(z / 2));
            if (len < 1 || len > static_cast<int>(kMaxLen)) throw std::out_of_range("SZ3 Huffman: invalid code length");
            syms_[i] = s;
            lens_[i] = static_cast<uint8_t>(len);
            prev = len;
        }
    }

    void count_dense(const std::vector<T> &bins, uint64_t span, std::vector<uint64_t> &freq_out) {
        const U off = static_cast<U>(offset_);
        const size_t n = bins.size();
        std::vector<uint64_t> f64;
        // Runs of one bin make one counter a store-to-load chain; four counters break it while they stay in cache.
        if (span * sizeof(uint32_t) * 4 <= (size_t(8) << 20) && n >= 4096 && n <= UINT32_MAX) {
            std::vector<uint32_t> f32(span * 4, 0);
            uint32_t *f0 = f32.data(), *f1 = f0 + span, *f2 = f1 + span, *f3 = f2 + span;
            size_t i = 0;
            for (; i + 4 <= n; i += 4) {
                f0[static_cast<U>(static_cast<U>(bins[i]) - off)]++;
                f1[static_cast<U>(static_cast<U>(bins[i + 1]) - off)]++;
                f2[static_cast<U>(static_cast<U>(bins[i + 2]) - off)]++;
                f3[static_cast<U>(static_cast<U>(bins[i + 3]) - off)]++;
            }
            for (; i < n; i++) f0[static_cast<U>(static_cast<U>(bins[i]) - off)]++;
            f64.resize(span);
            for (size_t k = 0; k < span; k++) f64[k] = uint64_t(f0[k]) + f1[k] + f2[k] + f3[k];
        } else {
            f64.assign(span, 0);
            for (const T b : bins) f64[static_cast<U>(static_cast<U>(b) - off)]++;
        }
        for (size_t k = 0; k < span; k++) {
            if (f64[k]) {
                syms_.push_back(k);
                freq_out.push_back(f64[k]);
            }
        }
        code_.assign(span, 0);
    }

    // Open addressing on bin - offset, hashed so that evenly spaced bins do not pile up in neighbouring slots.
    struct SymMap {
        std::vector<uint64_t> key, val;
        std::vector<uint8_t> used;
        size_t count = 0;
        unsigned shift = 64;

        size_t slot(uint64_t k) const {
            k = (k ^ (k >> 30)) * 0xbf58476d1ce4e5b9ull;
            k = (k ^ (k >> 27)) * 0x94d049bb133111ebull;
            return static_cast<size_t>((k ^ (k >> 31)) >> shift);
        }
        uint64_t &at(uint64_t k) {  // inserts
            if (2 * (count + 1) > key.size()) grow();
            const size_t m = key.size() - 1;
            size_t i = slot(k);
            while (used[i] && key[i] != k) i = (i + 1) & m;
            if (!used[i]) {
                used[i] = 1;
                key[i] = k;
                count++;
            }
            return val[i];
        }
        uint64_t get(uint64_t k) const {  // 0 if absent
            const size_t m = key.size() - 1;
            for (size_t i = slot(k); used[i]; i = (i + 1) & m)
                if (key[i] == k) return val[i];
            return 0;
        }
        void grow() {
            const unsigned bits = key.empty() ? 10 : 65 - shift;
            SymMap bigger;
            bigger.key.assign(size_t(1) << bits, 0);
            bigger.val.assign(size_t(1) << bits, 0);
            bigger.used.assign(size_t(1) << bits, 0);
            bigger.shift = 64 - bits;
            for (size_t i = 0; i < key.size(); i++)
                if (used[i]) bigger.at(key[i]) = val[i];
            *this = std::move(bigger);
        }
    };

    void count_sparse(const std::vector<T> &bins, std::vector<uint64_t> &freq_out) {
        const U off = static_cast<U>(offset_);
        for (const T b : bins) map_.at(static_cast<U>(static_cast<U>(b) - off))++;
        std::vector<std::pair<uint64_t, uint64_t>> kv;
        kv.reserve(map_.count);
        for (size_t i = 0; i < map_.key.size(); i++)
            if (map_.used[i]) kv.emplace_back(map_.key[i], map_.val[i]);
        std::sort(kv.begin(), kv.end());
        for (const auto &e : kv) {
            syms_.push_back(e.first);
            freq_out.push_back(e.second);
        }
    }

    void build_lengths(const std::vector<uint64_t> &freq) {
        const size_t d = syms_.size();
        if (d > UINT32_MAX) throw std::invalid_argument("SZ3 Huffman: more distinct bins than a uint32 counts");
        lens_.assign(d, 0);
        if (d == 1) return;
        // Ascending frequency, ties by bin, so the lengths do not depend on the platform's sort.
        std::vector<uint32_t> order(d);
        for (size_t i = 0; i < d; i++) order[i] = static_cast<uint32_t>(i);
        std::sort(order.begin(), order.end(),
                  [&](uint32_t a, uint32_t b) { return freq[a] != freq[b] ? freq[a] < freq[b] : a < b; });
        std::vector<uint64_t> A(d);
        for (size_t i = 0; i < d; i++) A[i] = freq[order[i]];
        minimum_redundancy(A);

        // Clamp to kMaxLen, then move the deepest leaves shorter than kMaxLen down until Kraft's inequality holds.
        uint64_t count[kMaxLen + 1] = {0};
        for (size_t i = 0; i < d; i++) count[std::min<uint64_t>(A[i], kMaxLen)]++;
        uint64_t kraft = 0;
        for (unsigned l = 1; l <= kMaxLen; l++) kraft += count[l] << (kMaxLen - l);
        while (kraft > (uint64_t(1) << kMaxLen)) {
            unsigned l = kMaxLen - 1;
            while (count[l] == 0) l--;
            count[l]--;
            count[l + 1]++;
            kraft -= uint64_t(1) << (kMaxLen - l - 1);
        }
        size_t i = 0;
        for (unsigned l = kMaxLen; l >= 1; l--)
            for (uint64_t k = 0; k < count[l]; k++, i++) {
                lens_[order[i]] = static_cast<uint8_t>(l);
                payload_bits_ += freq[order[i]] * l;
            }
        header_bits_ = 0;
        for (size_t j = 0; j < d; j++) {
            if (j > 0) header_bits_ += gamma_bits(syms_[j] - syms_[j - 1]);
            header_bits_ += gamma_bits(zigzag(lens_[j] - (j ? lens_[j - 1] : 0)) + 1);
        }
    }

    // Moffat and Katajainen, "In-place calculation of minimum-redundancy codes" (1995): A holds the weights in
    // ascending order and receives the code lengths.
    static void minimum_redundancy(std::vector<uint64_t> &A) {
        const size_t n = A.size();
        size_t root = 0, leaf = 2;
        A[0] += A[1];
        for (size_t next = 1; next < n - 1; next++) {
            if (leaf >= n || A[root] < A[leaf]) {
                A[next] = A[root];
                A[root++] = next;
            } else {
                A[next] = A[leaf++];
            }
            if (leaf >= n || (root < next && A[root] < A[leaf])) {
                A[next] += A[root];
                A[root++] = next;
            } else {
                A[next] += A[leaf++];
            }
        }
        A[n - 2] = 0;
        for (size_t k = n - 2; k-- > 0;) A[k] = A[A[k]] + 1;
        size_t avbl = 1, used = 0, depth = 0, next = n;
        size_t r = n - 1;  // one past the internal node to look at
        while (avbl > 0) {
            while (r > 0 && A[r - 1] == depth) {
                used++;
                r--;
            }
            while (avbl > used) {
                A[--next] = depth;
                avbl--;
            }
            avbl = 2 * used;
            depth++;
            used = 0;
        }
    }

    // Codes of one length are consecutive and follow bin order.
    void canonical(uint64_t first[kMaxLen + 1]) {
        std::fill(count_, count_ + kMaxLen + 1, 0);
        for (const uint8_t l : lens_) count_[l]++;
        count_[0] = 0;
        uint64_t code = 0;
        for (unsigned l = 1; l <= kMaxLen; l++) {
            first[l] = code;
            code = (code + count_[l]) << 1;
        }
    }

    void build_codes() {
        if (syms_.size() < 2) return;
        uint64_t first[kMaxLen + 1];
        canonical(first);
        for (size_t i = 0; i < syms_.size(); i++) {
            const uint64_t e = (first[lens_[i]]++ << 8) | lens_[i];
            if (dense_)
                code_[syms_[i]] = e;
            else
                map_.at(syms_[i]) = e;
        }
    }

    struct Entry {
        T sym;
        uint8_t len;  // 0: the code is longer than the table, or is not a code
    };

    void build_decoder() {
        const size_t d = syms_.size();
        if (d < 2) return;
        canonical(first_);
        uint64_t kraft = 0;
        for (unsigned l = 1; l <= kMaxLen; l++) {
            kraft += count_[l] << (kMaxLen - l);
            if (count_[l]) max_len_ = l;
        }
        if (kraft > (uint64_t(1) << kMaxLen)) throw std::out_of_range("SZ3 Huffman: code lengths overfull");
        base_[1] = 0;
        for (unsigned l = 2; l <= kMaxLen; l++) base_[l] = base_[l - 1] + count_[l - 1];
        // Codes of length l are below limit_[l] once left-aligned in 32 bits.
        for (unsigned l = 1; l <= kMaxLen; l++) limit_[l] = (first_[l] + count_[l]) << (kMaxLen - l);
        uint64_t pos[kMaxLen + 1];
        std::copy(base_, base_ + kMaxLen + 1, pos);
        sorted_.resize(d);
        for (size_t i = 0; i < d; i++) sorted_[pos[lens_[i]]++] = static_cast<T>(static_cast<U>(offset_) + syms_[i]);
        table_bits_ = std::min(max_len_, kTableBits);
        table_.assign(size_t(1) << table_bits_, Entry{0, 0});
        for (unsigned l = 1; l <= table_bits_; l++)
            for (uint64_t k = 0; k < count_[l]; k++) {
                const size_t lo = static_cast<size_t>((first_[l] + k) << (table_bits_ - l));
                std::fill(table_.begin() + lo, table_.begin() + lo + (size_t(1) << (table_bits_ - l)),
                          Entry{sorted_[base_[l] + k], static_cast<uint8_t>(l)});
            }
    }

    // A code longer than the table, with its bits at the top of acc.
    void decode_long(uint64_t acc, T &sym, unsigned &len) const {
        // The length is the first whose limit lies above the code's top 32 bits; counting, not searching, avoids a
        // mispredicted branch per length.
        const uint64_t w = acc >> 32;
        len = table_bits_ + 1;
        for (unsigned l = table_bits_ + 1; l < max_len_; l++) len += w >= limit_[l];
        if (w >= limit_[len]) throw std::out_of_range("SZ3 Huffman: invalid code in payload");
        sym = sorted_[base_[len] + ((w >> (32 - len)) - first_[len])];
    }

    template <bool ZeroRuns>
    void decode_payload(const uchar *p, size_t nbytes, uint64_t bits, T *out, size_t n) const {
        const Entry *tab = table_.data();
        const unsigned tb = table_bits_, ml = max_len_;
        const T zero_sym = sorted_[0];
        // With two one-bit codes, a run of ones is a run of the second symbol.
        const bool one_runs = count_[1] == 2;
        uint64_t pos = 0;
        size_t i = 0;
        // Whole 8-byte reads while they stay inside the payload; each gives at least 57 bits.
        while (i < n && (pos >> 3) + 8 <= nbytes) {
            uint64_t acc = static_cast<uint64_t>(bytesToInt64_bigEndian(p + (pos >> 3))) << (pos & 7);
            unsigned avail = 64 - static_cast<unsigned>(pos & 7);
            do {
                unsigned len;
                // Only runs of eight or more, which are likely long; shorter ones would mispredict each time.
                const bool ones = ZeroRuns && one_runs && (acc >> 56) == 0xff;
                if (ZeroRuns && (!(acc >> 56) || ones)) {
                    const uint64_t w = ones ? ~acc : acc;
                    len = w ? clz64(w) : 64;
                    len = static_cast<unsigned>(std::min<uint64_t>(std::min(len, avail), n - i));
                    std::fill(out + i, out + i + len, ones ? sorted_[1] : zero_sym);
                    acc = len < 64 ? acc << len : 0;
                    i += len;
                } else {
                    const Entry e = tab[acc >> (64 - tb)];
                    len = e.len;
                    if (len)
                        out[i] = e.sym;
                    else
                        decode_long(acc, out[i], len);
                    acc <<= len;
                    i++;
                }
                avail -= len;
                pos += len;
            } while (avail >= ml && i < n);
        }
        while (i < n && pos <= bits) {
            uchar tail[8] = {0};
            memcpy(tail, p + (pos >> 3), std::min<size_t>(8, nbytes - (pos >> 3)));
            const uint64_t acc = static_cast<uint64_t>(bytesToInt64_bigEndian(tail)) << (pos & 7);
            const Entry e = tab[acc >> (64 - tb)];
            unsigned len = e.len;
            if (len)
                out[i] = e.sym;
            else
                decode_long(acc, out[i], len);
            pos += len;
            i++;
        }
        if (pos != bits) throw std::out_of_range("SZ3 Huffman: payload length does not match its codes");
    }

    // encoder and decoder
    T offset_ = 0;
    std::vector<uint64_t> syms_;  // distinct bin - offset_, ascending
    std::vector<uint8_t> lens_;   // code length of each
    uint64_t header_bits_ = 0;
    uint64_t payload_bits_ = 0;
    uint64_t count_[kMaxLen + 1] = {0};  // codes of each length
    // encoder
    bool dense_ = true;
    std::vector<uint64_t> code_;  // dense: (code << 8) | length, indexed by bin - offset_
    SymMap map_;                  // sparse: the same, keyed by bin - offset_
    // decoder
    unsigned max_len_ = 0, table_bits_ = 0;
    uint64_t first_[kMaxLen + 1] = {0}, base_[kMaxLen + 1] = {0}, limit_[kMaxLen + 1] = {0};
    std::vector<T> sorted_;  // bins in code order
    std::vector<Entry> table_;
};

}  // namespace SZ3

#endif
