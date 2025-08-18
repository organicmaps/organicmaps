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

#ifndef RESTRICTION_HPP
#define RESTRICTION_HPP

#include "../typedefs.h"

#include <limits>

struct TurnRestriction
{
    union WayOrNode
    {
        NodeID node;
        EdgeID way;
    };
    WayOrNode via;
    WayOrNode from;
    WayOrNode to;

    struct Bits
    { // mostly unused
        Bits()
            : is_only(false), uses_via_way(false), unused2(false), unused3(false), unused4(false),
              unused5(false), unused6(false), unused7(false)
        {
        }

        bool is_only : 1;
        bool uses_via_way : 1;
        bool unused2 : 1;
        bool unused3 : 1;
        bool unused4 : 1;
        bool unused5 : 1;
        bool unused6 : 1;
        bool unused7 : 1;
    } flags;

    explicit TurnRestriction(NodeID node)
    {
        via.node = node;
        from.node = SPECIAL_NODEID;
        to.node = SPECIAL_NODEID;
    }

    explicit TurnRestriction(const bool is_only = false)
    {
        via.node = SPECIAL_NODEID;
        from.node = SPECIAL_NODEID;
        to.node = SPECIAL_NODEID;
        flags.is_only = is_only;
    }
};

struct InputRestrictionContainer
{
    // EdgeID fromWay;
    // EdgeID toWay;
    // NodeID via_node;
    TurnRestriction restriction;

    InputRestrictionContainer(EdgeID fromWay, EdgeID toWay, EdgeID vw)
    {
        restriction.from.way = fromWay;
        restriction.to.way = toWay;
        restriction.via.way = vw;
    }
    explicit InputRestrictionContainer(bool is_only = false)
    {
        restriction.from.way = SPECIAL_EDGEID;
        restriction.to.way = SPECIAL_EDGEID;
        restriction.via.node = SPECIAL_NODEID;
        restriction.flags.is_only = is_only;
    }

    static InputRestrictionContainer min_value() { return InputRestrictionContainer(0, 0, 0); }
    static InputRestrictionContainer max_value()
    {
        return InputRestrictionContainer(SPECIAL_EDGEID, SPECIAL_EDGEID, SPECIAL_EDGEID);
    }
};

struct CmpRestrictionContainerByFrom
{
    using value_type = InputRestrictionContainer;
    bool operator()(const InputRestrictionContainer &a, const InputRestrictionContainer &b) const
    {
        return a.restriction.from.way < b.restriction.from.way;
    }
    value_type max_value() const { return InputRestrictionContainer::max_value(); }
    value_type min_value() const { return InputRestrictionContainer::min_value(); }
};

struct CmpRestrictionContainerByTo
{
    using value_type = InputRestrictionContainer;
    bool operator()(const InputRestrictionContainer &a, const InputRestrictionContainer &b) const
    {
        return a.restriction.to.way < b.restriction.to.way;
    }
    value_type max_value() const { return InputRestrictionContainer::max_value(); }
    value_type min_value() const { return InputRestrictionContainer::min_value(); }
};

#endif // RESTRICTION_HPP
