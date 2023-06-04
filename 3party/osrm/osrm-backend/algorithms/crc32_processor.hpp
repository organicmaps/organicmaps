/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef ITERATOR_BASED_CRC32_H
#define ITERATOR_BASED_CRC32_H

#if defined(__x86_64__) && !defined(__MINGW64__)
#include <cpuid.h>
#endif

#include <boost/crc.hpp> // for boost::crc_32_type

#include <iterator>

class IteratorbasedCRC32
{
  public:
    bool using_hardware() const { return use_hardware_implementation; }

    IteratorbasedCRC32() : crc(0) { use_hardware_implementation = detect_hardware_support(); }

    template <class Iterator> unsigned operator()(Iterator iter, const Iterator end)
    {
        unsigned crc = 0;
        while (iter != end)
        {
            using value_type = typename std::iterator_traits<Iterator>::value_type;
            const char *data = reinterpret_cast<const char *>(&(*iter));

            if (use_hardware_implementation)
            {
                crc = compute_in_hardware(data, sizeof(value_type));
            }
            else
            {
                crc = compute_in_software(data, sizeof(value_type));
            }
            ++iter;
        }
        return crc;
    }

  private:
    bool detect_hardware_support() const
    {
        static const int sse42_bit = 0x00100000;
        const unsigned ecx = cpuid();
        const bool sse42_found = (ecx & sse42_bit) != 0;
        return sse42_found;
    }

    unsigned compute_in_software(const char *str, unsigned len)
    {
        crc_processor.process_bytes(str, len);
        return crc_processor.checksum();
    }

    // adapted from http://byteworm.com/2010/10/13/crc32/
    unsigned compute_in_hardware(const char *str, unsigned len)
    {
#if defined(__x86_64__)
        unsigned q = len / sizeof(unsigned);
        unsigned r = len % sizeof(unsigned);
        unsigned *p = (unsigned *)str;

        // crc=0;
        while (q--)
        {
            __asm__ __volatile__(".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                                 : "=S"(crc)
                                 : "0"(crc), "c"(*p));
            ++p;
        }

        str = reinterpret_cast<char *>(p);
        while (r--)
        {
            __asm__ __volatile__(".byte 0xf2, 0xf, 0x38, 0xf1, 0xf1;"
                                 : "=S"(crc)
                                 : "0"(crc), "c"(*str));
            ++str;
        }
#endif
        return crc;
    }

    inline unsigned cpuid() const
    {
        unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
        // on X64 this calls hardware cpuid(.) instr. otherwise a dummy impl.
        __get_cpuid(1, &eax, &ebx, &ecx, &edx);
        return ecx;
    }

#if defined(__MINGW64__) || defined(_MSC_VER) || !defined(__x86_64__)
    inline void
    __get_cpuid(int param, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx) const
    {
        *ecx = 0;
    }
#endif

    boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> crc_processor;
    unsigned crc;
    bool use_hardware_implementation;
};

struct RangebasedCRC32
{
    template <typename Iteratable> unsigned operator()(const Iteratable &iterable)
    {
        return crc32(std::begin(iterable), std::end(iterable));
    }

    bool using_hardware() const { return crc32.using_hardware(); }

  private:
    IteratorbasedCRC32 crc32;
};

#endif /* ITERATOR_BASED_CRC32_H */
