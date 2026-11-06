//
// Created by Kai Zhao on 4/21/20.
//

#ifndef SZ3_MEMORYOPS_HPP
#define SZ3_MEMORYOPS_HPP

#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "SZ3/def.hpp"

namespace SZ3 {

// Endianness detection: SZ3 always stores data in little-endian format
// On big-endian systems, byte-swapping is performed during read/write
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define SZ3_BIG_ENDIAN 1
#elif defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) || \
      defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB)
    #define SZ3_BIG_ENDIAN 1
#else
    // Default: little-endian
    #define SZ3_BIG_ENDIAN 0
#endif

#if SZ3_BIG_ENDIAN
// Byte-swap intrinsics for fast endianness conversion
#if defined(_MSC_VER)
    #include <cstdlib> // Use <cstdlib> for _byteswap_...
    #define SZ3_HAS_BUILTIN_BSWAP 1
    #define BSWAP16(x) _byteswap_ushort(x)
    #define BSWAP32(x) _byteswap_ulong(x)
    #define BSWAP64(x) _byteswap_uint64(x)
#elif defined(__GNUC__) || defined(__clang__)
    #define SZ3_HAS_BUILTIN_BSWAP 1
    #define BSWAP16(x) __builtin_bswap16(x)
    #define BSWAP32(x) __builtin_bswap32(x)
    #define BSWAP64(x) __builtin_bswap64(x)
#else
    #define SZ3_HAS_BUILTIN_BSWAP 0
#endif

// Generic byteswap dispatcher
template <typename T>
inline T byteswap(T value) {
    union { T val; uint8_t bytes[sizeof(T)]; uint16_t u16; uint32_t u32; uint64_t u64; } u;
    u.val = value;
    
    if constexpr (sizeof(T) == 1) {
        return value;
#if SZ3_HAS_BUILTIN_BSWAP
    } else if constexpr (sizeof(T) == 2) {
        u.u16 = BSWAP16(u.u16);
    } else if constexpr (sizeof(T) == 4) {
        u.u32 = BSWAP32(u.u32);
    } else if constexpr (sizeof(T) == 8) {
        u.u64 = BSWAP64(u.u64);
#endif
    } else {
        // Fallback for all sizes when no intrinsics, or for sizes > 8 bytes
        for (size_t i = 0; i < sizeof(T) / 2; i++) {
            uint8_t tmp = u.bytes[i];
            u.bytes[i] = u.bytes[sizeof(T) - 1 - i];
            u.bytes[sizeof(T) - 1 - i] = tmp;
        }
    }
    return u.val;
}
#endif // SZ3_BIG_ENDIAN

// read array
template <class T1>
void read(T1 *array, size_t num_elements, uchar const *&compressed_data_pos, size_t &remaining_length) {
    if (num_elements > remaining_length / sizeof(T1))
        throw std::out_of_range("SZ3: attempt to read past the end of the compressed buffer");
    memcpy(array, compressed_data_pos, num_elements * sizeof(T1));
    if constexpr (SZ3_BIG_ENDIAN) {
        for (size_t i = 0; i < num_elements; i++) {
            array[i] = byteswap(array[i]);
        }
    }
    remaining_length -= num_elements * sizeof(T1);
    compressed_data_pos += num_elements * sizeof(T1);
}

// read array
template <class T1>
void read(T1 *array, size_t num_elements, uchar const *&compressed_data_pos) {
    memcpy(array, compressed_data_pos, num_elements * sizeof(T1));
    if constexpr (SZ3_BIG_ENDIAN) {
        for (size_t i = 0; i < num_elements; i++) {
            array[i] = byteswap(array[i]);
        }
    }
    compressed_data_pos += num_elements * sizeof(T1);
}

// read variable
template <class T1>
void read(T1 &var, uchar const *&compressed_data_pos) {
    memcpy(&var, compressed_data_pos, sizeof(T1));
    if constexpr (SZ3_BIG_ENDIAN) {
        var = byteswap(var);
    }
    compressed_data_pos += sizeof(T1);
}

// read variable
template <class T1>
void read(T1 &var, uchar const *&compressed_data_pos, size_t &remaining_length) {
    if (sizeof(T1) > remaining_length)
        throw std::out_of_range("SZ3: attempt to read past the end of the compressed buffer");
    memcpy(&var, compressed_data_pos, sizeof(T1));
    if constexpr (SZ3_BIG_ENDIAN) {
        var = byteswap(var);
    }
    remaining_length -= sizeof(T1);
    compressed_data_pos += sizeof(T1);
}

// write array
template <class T1>
void write(T1 const *array, size_t num_elements, uchar *&compressed_data_pos) {
    memcpy(compressed_data_pos, array, num_elements * sizeof(T1));
    if constexpr (SZ3_BIG_ENDIAN) {
        T1 *dest = reinterpret_cast<T1 *>(compressed_data_pos);
        for (size_t i = 0; i < num_elements; i++) {
            dest[i] = byteswap(dest[i]);
        }
    }
    compressed_data_pos += num_elements * sizeof(T1);
}

// write variable
template <class T1>
void write(T1 const var, uchar *&compressed_data_pos) {
    if constexpr (SZ3_BIG_ENDIAN) {
        T1 le_var = byteswap(var);
        memcpy(compressed_data_pos, &le_var, sizeof(T1));
    } else {
        memcpy(compressed_data_pos, &var, sizeof(T1));
    }
    compressed_data_pos += sizeof(T1);
}

}  // namespace SZ3
#endif  // SZ_MEMORYOPS_HPP
