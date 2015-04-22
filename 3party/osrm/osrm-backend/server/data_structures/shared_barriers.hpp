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

#ifndef SHARED_BARRIERS_HPP
#define SHARED_BARRIERS_HPP

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

struct SharedBarriers
{

    SharedBarriers()
        : pending_update_mutex(boost::interprocess::open_or_create, "pending_update"),
          update_mutex(boost::interprocess::open_or_create, "update"),
          query_mutex(boost::interprocess::open_or_create, "query"),
          no_running_queries_condition(boost::interprocess::open_or_create, "no_running_queries"),
          update_ongoing(false), number_of_queries(0)
    {
    }

    // Mutex to protect access to the boolean variable
    boost::interprocess::named_mutex pending_update_mutex;
    boost::interprocess::named_mutex update_mutex;
    boost::interprocess::named_mutex query_mutex;

    // Condition that no update is running
    boost::interprocess::named_condition no_running_queries_condition;

    // Is there an ongoing update?
    bool update_ongoing;
    // Is there any query?
    int number_of_queries;
};

#endif // SHARED_BARRIERS_HPP
