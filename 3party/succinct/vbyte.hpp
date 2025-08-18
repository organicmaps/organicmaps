#pragma once

#include "broadword.hpp"

namespace succinct {

    inline size_t vbyte_size(size_t val)
    {
        unsigned long bits;
        if (!broadword::msb(val, bits)) bits = 0;
        return util::ceil_div(bits + 1, 7);
    }

    template <typename Vector>
    inline size_t append_vbyte(Vector& v, size_t val)
    {
        size_t chunks = vbyte_size(val);
        for (size_t b = chunks - 1; b + 1 > 0; --b) {
            uint8_t chunk = (val >> (b * 7)) & 0x7F;
            chunk |= b ? 0x80 : 0;
            v.push_back(chunk);
        }
        return chunks;
    }

    template <typename Vector>
    inline size_t decode_vbyte(Vector const& v, size_t offset, size_t& val)
    {
        size_t pos = offset;
        val = 0;
        uint8_t chunk;
        do {
            chunk = v[pos++];
            val <<= 7;
            val |= chunk & 0x7F;
        } while (chunk & 0x80);

        return pos - offset;
    }

}
