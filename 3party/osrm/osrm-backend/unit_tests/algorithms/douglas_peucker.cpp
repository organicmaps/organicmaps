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

#include "../../algorithms/douglas_peucker.hpp"
#include "../../data_structures/segment_information.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <osrm/coordinate.hpp>

#include <iostream>

BOOST_AUTO_TEST_SUITE(douglas_peucker)

SegmentInformation getTestInfo(int lat, int lon, bool necessary)
{
    return SegmentInformation(FixedPointCoordinate(lat, lon), 0, 0, 0, TurnInstruction::HeadOn,
                              necessary, false, 0);
}

BOOST_AUTO_TEST_CASE(all_necessary_test)
{
    /*
     *     x
     *    / \
     *   x   \
     *  /     \
     * x       x
     */
    std::vector<SegmentInformation> info = {getTestInfo(5, 5, true),
                                            getTestInfo(6, 6, true),
                                            getTestInfo(10, 10, true),
                                            getTestInfo(5, 15, true)};
    DouglasPeucker dp;
    for (unsigned z = 0; z < DOUGLAS_PEUCKER_THRESHOLDS.size(); z++)
    {
        dp.Run(info, z);
        for (const auto &i : info)
        {
            BOOST_CHECK_EQUAL(i.necessary, true);
        }
    }
}

BOOST_AUTO_TEST_CASE(remove_second_node_test)
{
    DouglasPeucker dp;
    for (unsigned z = 0; z < DOUGLAS_PEUCKER_THRESHOLDS.size(); z++)
    {
        /*
         *   x--x
         *   |   \
         * x-x    x
         *        |
         *        x
         */
        std::vector<SegmentInformation> info = {
            getTestInfo(5 * COORDINATE_PRECISION, 5 * COORDINATE_PRECISION, true),
            getTestInfo(5 * COORDINATE_PRECISION,
                        5 * COORDINATE_PRECISION + DOUGLAS_PEUCKER_THRESHOLDS[z], false),
            getTestInfo(10 * COORDINATE_PRECISION, 10 * COORDINATE_PRECISION, false),
            getTestInfo(10 * COORDINATE_PRECISION,
                        10 + COORDINATE_PRECISION + DOUGLAS_PEUCKER_THRESHOLDS[z] * 2, false),
            getTestInfo(5 * COORDINATE_PRECISION, 15 * COORDINATE_PRECISION, false),
            getTestInfo(5 * COORDINATE_PRECISION + DOUGLAS_PEUCKER_THRESHOLDS[z],
                        15 * COORDINATE_PRECISION, true),
        };
        BOOST_TEST_MESSAGE("Threshold (" << z << "): " << DOUGLAS_PEUCKER_THRESHOLDS[z]);
        dp.Run(info, z);
        BOOST_CHECK_EQUAL(info[0].necessary, true);
        BOOST_CHECK_EQUAL(info[1].necessary, false);
        BOOST_CHECK_EQUAL(info[2].necessary, true);
        BOOST_CHECK_EQUAL(info[3].necessary, true);
        BOOST_CHECK_EQUAL(info[4].necessary, false);
        BOOST_CHECK_EQUAL(info[5].necessary, true);
    }
}

BOOST_AUTO_TEST_SUITE_END()
