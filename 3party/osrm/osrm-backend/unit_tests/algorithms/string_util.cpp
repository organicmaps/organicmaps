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

#include "../../util/string_util.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(string_util)

BOOST_AUTO_TEST_CASE(replace_all_test)
{
    std::string input{"ababababababa"};
    const std::string sub{"a"};
    const std::string other{"c"};
    replaceAll(input, sub, other);

    BOOST_CHECK_EQUAL(input, "cbcbcbcbcbcbc");
}

BOOST_AUTO_TEST_CASE(json_escaping)
{
    std::string input{"\b\\"};
    std::string output{escape_JSON(input)};

    BOOST_CHECK_EQUAL(output, "\\b\\\\");

    input = "Aleja \"Solidarnosci\"";
    output = escape_JSON(input);
    BOOST_CHECK_EQUAL(output, "Aleja \\\"Solidarnosci\\\"");
}

BOOST_AUTO_TEST_CASE(print_int)
{
    const std::string input{"\b\\"};
    char buffer[12];
    buffer[11] = 0; // zero termination
    std::string output = printInt<11, 8>(buffer, 314158976);
    BOOST_CHECK_EQUAL(output, "3.14158976");

    buffer[11] = 0;
    output = printInt<11, 8>(buffer, 0);
    BOOST_CHECK_EQUAL(output, "0.00000000");

    output = printInt<11, 8>(buffer, -314158976);
    BOOST_CHECK_EQUAL(output, "-3.14158976");
}

BOOST_AUTO_TEST_SUITE_END()
