/*

Copyright (c) 2013, Project OSRM, Dennis Luxen, others
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

#include "../Util/SimpleLogger.h"

#include <iostream>

#if defined(__x86_64__) && !defined(__MINGW64__)
#include <cpuid.h>
#else
#include <boost/crc.hpp> // for boost::crc_32_type

inline void __get_cpuid(int param, unsigned *eax, unsigned *ebx, unsigned *ecx, unsigned *edx)
{
    *ecx = 0;
}
#endif

template <class ContainerT> class IteratorbasedCRC32
{
  private:
    typedef typename ContainerT::iterator IteratorType;
    unsigned crc;

    bool use_SSE42_CRC_function;

#if !defined(__x86_64__)
    boost::crc_optimal<32, 0x1EDC6F41, 0x0, 0x0, true, true> CRC32_processor;
#endif
    unsigned SoftwareBasedCRC32(char *str, unsigned len)
    {
#if !defined(__x86_64__)
        CRC32_processor.process_bytes(str, len);
        return CRC32_processor.checksum();
#else
        return 0;
#endif
    }

    // adapted from http://byteworm.com/2010/10/13/crc32/
    unsigned SSE42BasedCRC32(char *str, unsigned len)
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

        str = (char *)p;
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

    bool DetectNativeCRC32Support()
    {
        static const int SSE42_BIT = 0x00100000;
        const unsigned ecx = cpuid();
        const bool has_SSE42 = (ecx & SSE42_BIT) != 0;
        if (has_SSE42)
        {
            SimpleLogger().Write() << "using hardware based CRC32 computation";
        }
        else
        {
            SimpleLogger().Write() << "using software based CRC32 computation";
        }
        return has_SSE42;
    }

  public:
    IteratorbasedCRC32() : crc(0) { use_SSE42_CRC_function = DetectNativeCRC32Support(); }

    unsigned operator()(IteratorType iter, const IteratorType end)
    {
        unsigned crc = 0;
        while (iter != end)
        {
            char *data = reinterpret_cast<char *>(&(*iter));

            if (use_SSE42_CRC_function)
            {
                crc = SSE42BasedCRC32(data, sizeof(typename ContainerT::value_type));
            }
            else
            {
                crc = SoftwareBasedCRC32(data, sizeof(typename ContainerT::value_type));
            }
            ++iter;
        }
        return crc;
    }
};

#endif /* ITERATOR_BASED_CRC32_H */
