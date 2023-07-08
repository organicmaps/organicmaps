/*

Copyright (c) 2013, Project OSRM contributors
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

#ifndef XOR_FAST_HASH_HPP
#define XOR_FAST_HASH_HPP

#include <algorithm>
#include <vector>

/*
    This is an implementation of Tabulation hashing, which has suprising properties like
   universality.
    The space requirement is 2*2^16 = 256 kb of memory, which fits into L2 cache.
    Evaluation boils down to 10 or less assembly instruction on any recent X86 CPU:

    1: movq    table2(%rip), %rdx
    2: movl    %edi, %eax
    3: movzwl  %di, %edi
    4: shrl    $16, %eax
    5: movzwl  %ax, %eax
    6: movzbl  (%rdx,%rax), %eax
    7: movq    table1(%rip), %rdx
    8: xorb    (%rdx,%rdi), %al
    9: movzbl  %al, %eax
    10: ret

*/
class XORFastHash
{ // 65k entries
    std::vector<unsigned short> table1;
    std::vector<unsigned short> table2;

  public:
    XORFastHash()
    {
        table1.resize(2 << 16);
        table2.resize(2 << 16);
        for (unsigned i = 0; i < (2 << 16); ++i)
        {
            table1[i] = static_cast<unsigned short>(i);
            table2[i] = static_cast<unsigned short>(i);
        }
        std::random_shuffle(table1.begin(), table1.end());
        std::random_shuffle(table2.begin(), table2.end());
    }

    inline unsigned short operator()(const unsigned originalValue) const
    {
        unsigned short lsb = ((originalValue)&0xffff);
        unsigned short msb = (((originalValue) >> 16) & 0xffff);
        return table1[lsb] ^ table2[msb];
    }
};

class XORMiniHash
{ // 256 entries
    std::vector<unsigned char> table1;
    std::vector<unsigned char> table2;
    std::vector<unsigned char> table3;
    std::vector<unsigned char> table4;

  public:
    XORMiniHash()
    {
        table1.resize(1 << 8);
        table2.resize(1 << 8);
        table3.resize(1 << 8);
        table4.resize(1 << 8);
        for (unsigned i = 0; i < (1 << 8); ++i)
        {
            table1[i] = static_cast<unsigned char>(i);
            table2[i] = static_cast<unsigned char>(i);
            table3[i] = static_cast<unsigned char>(i);
            table4[i] = static_cast<unsigned char>(i);
        }
        std::random_shuffle(table1.begin(), table1.end());
        std::random_shuffle(table2.begin(), table2.end());
        std::random_shuffle(table3.begin(), table3.end());
        std::random_shuffle(table4.begin(), table4.end());
    }
    unsigned char operator()(const unsigned originalValue) const
    {
        unsigned char byte1 = ((originalValue)&0xff);
        unsigned char byte2 = ((originalValue >> 8) & 0xff);
        unsigned char byte3 = ((originalValue >> 16) & 0xff);
        unsigned char byte4 = ((originalValue >> 24) & 0xff);
        return table1[byte1] ^ table2[byte2] ^ table3[byte3] ^ table4[byte4];
    }
};

#endif // XOR_FAST_HASH_HPP
