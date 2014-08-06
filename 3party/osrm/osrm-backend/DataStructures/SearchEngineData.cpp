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

#include "SearchEngineData.h"

#include "BinaryHeap.h"

void SearchEngineData::InitializeOrClearFirstThreadLocalStorage(const unsigned number_of_nodes)
{
    if (forwardHeap.get())
    {
        forwardHeap->Clear();
    }
    else
    {
        forwardHeap.reset(new QueryHeap(number_of_nodes));
    }

    if (backwardHeap.get())
    {
        backwardHeap->Clear();
    }
    else
    {
        backwardHeap.reset(new QueryHeap(number_of_nodes));
    }
}

void SearchEngineData::InitializeOrClearSecondThreadLocalStorage(const unsigned number_of_nodes)
{
    if (forwardHeap2.get())
    {
        forwardHeap2->Clear();
    }
    else
    {
        forwardHeap2.reset(new QueryHeap(number_of_nodes));
    }

    if (backwardHeap2.get())
    {
        backwardHeap2->Clear();
    }
    else
    {
        backwardHeap2.reset(new QueryHeap(number_of_nodes));
    }
}

void SearchEngineData::InitializeOrClearThirdThreadLocalStorage(const unsigned number_of_nodes)
{
    if (forwardHeap3.get())
    {
        forwardHeap3->Clear();
    }
    else
    {
        forwardHeap3.reset(new QueryHeap(number_of_nodes));
    }

    if (backwardHeap3.get())
    {
        backwardHeap3->Clear();
    }
    else
    {
        backwardHeap3.reset(new QueryHeap(number_of_nodes));
    }
}
