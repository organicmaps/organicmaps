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

#ifndef ISO_8601_DURATION_PARSER_HPP
#define ISO_8601_DURATION_PARSER_HPP

#include <boost/bind.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_action.hpp>

namespace qi = boost::spirit::qi;

template <typename Iterator> struct iso_8601_grammar : qi::grammar<Iterator>
{
    iso_8601_grammar()
        : iso_8601_grammar::base_type(iso_period), temp(0), hours(0), minutes(0), seconds(0)
    {
        iso_period = qi::lit('P') >> qi::lit('T') >>
                     ((value >> hour >> value >> minute >> value >> second) |
                      (value >> hour >> value >> minute) | (value >> hour >> value >> second) |
                      (value >> hour) | (value >> minute >> value >> second) | (value >> minute) |
                      (value >> second));

        value = qi::uint_[boost::bind(&iso_8601_grammar<Iterator>::set_temp, this, ::_1)];
        second = (qi::lit('s') |
                  qi::lit('S'))[boost::bind(&iso_8601_grammar<Iterator>::set_seconds, this)];
        minute = (qi::lit('m') |
                  qi::lit('M'))[boost::bind(&iso_8601_grammar<Iterator>::set_minutes, this)];
        hour = (qi::lit('h') |
                qi::lit('H'))[boost::bind(&iso_8601_grammar<Iterator>::set_hours, this)];
    }

    qi::rule<Iterator> iso_period;
    qi::rule<Iterator, std::string()> value, hour, minute, second;

    unsigned temp;
    unsigned hours;
    unsigned minutes;
    unsigned seconds;

    void set_temp(unsigned number) { temp = number; }

    void set_hours()
    {
        if (temp < 24)
        {
            hours = temp;
        }
    }

    void set_minutes()
    {
        if (temp < 60)
        {
            minutes = temp;
        }
    }

    void set_seconds()
    {
        if (temp < 60)
        {
            seconds = temp;
        }
    }

    unsigned get_duration() const
    {
        unsigned temp = 10 * (3600 * hours + 60 * minutes + seconds);
        if (temp == 0)
        {
            temp = std::numeric_limits<unsigned>::max();
        }
        return temp;
    }
};

#endif // ISO_8601_DURATION_PARSER_HPP
