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

#ifndef AZIMUTH_H
#define AZIMUTH_H

#include <string>

struct Azimuth
{
    static std::string Get(const double heading)
    {
        if (heading <= 202.5)
        {
            if (heading >= 0 && heading <= 22.5)
            {
                return "N";
            }
            if (heading > 22.5 && heading <= 67.5)
            {
                return "NE";
            }
            if (heading > 67.5 && heading <= 112.5)
            {
                return "E";
            }
            if (heading > 112.5 && heading <= 157.5)
            {
                return "SE";
            }
            return "S";
        }
        if (heading > 202.5 && heading <= 247.5)
        {
            return "SW";
        }
        if (heading > 247.5 && heading <= 292.5)
        {
            return "W";
        }
        if (heading > 292.5 && heading <= 337.5)
        {
            return "NW";
        }
        return "N";
    }
};

#endif // AZIMUTH_H
