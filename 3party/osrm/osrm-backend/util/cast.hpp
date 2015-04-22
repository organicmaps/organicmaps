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

#ifndef CAST_HPP
#define CAST_HPP

#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/qi.hpp>

#include <string>
#include <type_traits>

struct cast
{
    // convert scoped enums to integers
    template <typename Enumeration>
    static auto enum_to_underlying(Enumeration const value) ->
        typename std::underlying_type<Enumeration>::type
    {
        return static_cast<typename std::underlying_type<Enumeration>::type>(value);
    }

    template <typename Number>
    static typename std::enable_if<std::is_integral<Number>::value, std::string>::type
    integral_to_string(const Number value)
    {
        std::string output;
        std::back_insert_iterator<std::string> sink(output);

        if (8 == sizeof(Number))
        {
            boost::spirit::karma::generate(sink, boost::spirit::karma::long_long, value);
        }
        else
        {
            if (std::is_signed<Number>::value)
            {
                boost::spirit::karma::generate(sink, boost::spirit::karma::int_, value);
            }
            else
            {
                boost::spirit::karma::generate(sink, boost::spirit::karma::uint_, value);
            }
        }
        return output;
    }

    static int string_to_int(const std::string &input)
    {
        auto first_digit = input.begin();
        // Delete any trailing white-spaces
        while (first_digit != input.end() && std::isspace(*first_digit))
        {
            ++first_digit;
        }
        int value = 0;
        boost::spirit::qi::parse(first_digit, input.end(), boost::spirit::int_, value);
        return value;
    }

    static unsigned string_to_uint(const std::string &input)
    {
        auto first_digit = input.begin();
        // Delete any trailing white-spaces
        while (first_digit != input.end() && (std::isspace(*first_digit) || '-' == *first_digit))
        {
            ++first_digit;
        }
        unsigned value = 0;
        boost::spirit::qi::parse(first_digit, input.end(), boost::spirit::uint_, value);
        return value;
    }

    static uint64_t string_to_uint64(const std::string &input)
    {
        auto first_digit = input.begin();
        // Delete any trailing white-spaces
        while (first_digit != input.end() && std::isspace(*first_digit))
        {
            ++first_digit;
        }
        uint64_t value = 0;
        boost::spirit::qi::parse(first_digit, input.end(), boost::spirit::long_long, value);
        return value;
    }

    // source: http://tinodidriksen.com/2011/05/28/cpp-convert-string-to-double-speed/
    static double string_to_double(const char *p) noexcept
    {
        double r = 0.0;
        bool neg = false;
        if (*p == '-')
        {
            neg = true;
            ++p;
        }
        while (*p >= '0' && *p <= '9')
        {
            r = (r * 10.0) + (*p - '0');
            ++p;
        }
        if (*p == '.')
        {
            double f = 0.0;
            int n = 0;
            ++p;
            while (*p >= '0' && *p <= '9')
            {
                f = (f * 10.0) + (*p - '0');
                ++p;
                ++n;
            }
            r += f / std::pow(10.0, n);
        }
        if (neg)
        {
            r = -r;
        }
        return r;
    }

    template <typename T> struct scientific_policy : boost::spirit::karma::real_policies<T>
    {
        //  we want the numbers always to be in fixed format
        static int floatfield(T) { return boost::spirit::karma::real_policies<T>::fmtflags::fixed; }
        static unsigned int precision(T) { return 6; }
    };
    using science_type = boost::spirit::karma::real_generator<double, scientific_policy<double>>;

    static std::string double_fixed_to_string(const double value)
    {
        std::string output;
        std::back_insert_iterator<std::string> sink(output);
        boost::spirit::karma::generate(sink, science_type(), value);
        if (output.size() >= 2 && output[output.size() - 2] == '.' &&
            output[output.size() - 1] == '0')
        {
            output.resize(output.size() - 2);
        }
        return output;
    }

    static std::string double_to_string(const double value)
    {
        std::string output;
        std::back_insert_iterator<std::string> sink(output);
        boost::spirit::karma::generate(sink, value);
        return output;
    }

    static void double_with_two_digits_to_string(const double value, std::string &output)
    {
        // The largest 32-bit integer is 4294967295, that is 10 chars
        // On the safe side, add 1 for sign, and 1 for trailing zero
        char buffer[12];
        sprintf(buffer, "%g", value);
        output = buffer;
    }
};

#endif // CAST_HPP
