/*

Copyright (c) 2014, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "hilbert_value.hpp"

#include <osrm/coordinate.hpp>

uint64_t HilbertCode::operator()(const FixedPointCoordinate &current_coordinate) const
{
    unsigned location[2];
    location[0] = current_coordinate.lat + static_cast<int>(90 * COORDINATE_PRECISION);
    location[1] = current_coordinate.lon + static_cast<int>(180 * COORDINATE_PRECISION);

    TransposeCoordinate(location);
    return BitInterleaving(location[0], location[1]);
}

uint64_t HilbertCode::BitInterleaving(const uint32_t latitude, const uint32_t longitude) const
{
    uint64_t result = 0;
    for (int8_t index = 31; index >= 0; --index)
    {
        result |= (latitude >> index) & 1;
        result <<= 1;
        result |= (longitude >> index) & 1;
        if (0 != index)
        {
            result <<= 1;
        }
    }
    return result;
}

void HilbertCode::TransposeCoordinate(uint32_t *X) const
{
    uint32_t M = 1 << (32 - 1), P, Q, t;
    int i;
    // Inverse undo
    for (Q = M; Q > 1; Q >>= 1)
    {
        P = Q - 1;
        for (i = 0; i < 2; ++i)
        {

            const bool condition = (X[i] & Q);
            if (condition)
            {
                X[0] ^= P; // invert
            }
            else
            {
                t = (X[0] ^ X[i]) & P;
                X[0] ^= t;
                X[i] ^= t;
            }
        } // exchange
    }
    // Gray encode
    for (i = 1; i < 2; ++i)
    {
        X[i] ^= X[i - 1];
    }
    t = 0;
    for (Q = M; Q > 1; Q >>= 1)
    {
        const bool condition = (X[2 - 1] & Q);
        if (condition)
        {
            t ^= Q - 1;
        }
    } // check if this for loop is wrong
    for (i = 0; i < 2; ++i)
    {
        X[i] ^= t;
    }
}
