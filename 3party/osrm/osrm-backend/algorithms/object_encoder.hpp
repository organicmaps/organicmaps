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

#ifndef OBJECT_ENCODER_HPP
#define OBJECT_ENCODER_HPP

#include "../util/string_util.hpp"

#include <boost/assert.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <algorithm>
#include <string>
#include <vector>

struct ObjectEncoder
{
    using base64_t = boost::archive::iterators::base64_from_binary<
        boost::archive::iterators::transform_width<const char *, 6, 8>>;

    using binary_t = boost::archive::iterators::transform_width<
        boost::archive::iterators::binary_from_base64<std::string::const_iterator>,
        8,
        6>;

    template <class ObjectT> static void EncodeToBase64(const ObjectT &object, std::string &encoded)
    {
        const char *char_ptr_to_object = reinterpret_cast<const char *>(&object);
        std::vector<unsigned char> data(sizeof(object));
        std::copy(char_ptr_to_object, char_ptr_to_object + sizeof(ObjectT), data.begin());

        unsigned char number_of_padded_chars = 0; // is in {0,1,2};
        while (data.size() % 3 != 0)
        {
            ++number_of_padded_chars;
            data.push_back(0x00);
        }

        BOOST_ASSERT_MSG(0 == data.size() % 3, "base64 input data size is not a multiple of 3!");
        encoded.resize(sizeof(ObjectT));
        encoded.assign(base64_t(&data[0]),
                       base64_t(&data[0] + (data.size() - number_of_padded_chars)));
        replaceAll(encoded, "+", "-");
        replaceAll(encoded, "/", "_");
    }

    template <class ObjectT> static void DecodeFromBase64(const std::string &input, ObjectT &object)
    {
        try
        {
            std::string encoded(input);
            // replace "-" with "+" and "_" with "/"
            replaceAll(encoded, "-", "+");
            replaceAll(encoded, "_", "/");

            std::copy(binary_t(encoded.begin()), binary_t(encoded.begin() + encoded.length() - 1),
                      reinterpret_cast<char *>(&object));
        }
        catch (...)
        {
        }
    }
};

#endif /* OBJECT_ENCODER_HPP */
