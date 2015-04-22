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

#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <boost/uuid/uuid.hpp>

// implements a singleton, i.e. there is one and only one conviguration object
class FingerPrint
{
  public:
    FingerPrint();
    FingerPrint(const FingerPrint &) = delete;
    ~FingerPrint();
    const boost::uuids::uuid &GetFingerPrint() const;
    bool IsMagicNumberOK() const;
    bool TestGraphUtil(const FingerPrint &other) const;
    bool TestPrepare(const FingerPrint &other) const;
    bool TestRTree(const FingerPrint &other) const;
    bool TestQueryObjects(const FingerPrint &other) const;

  private:
    const unsigned magic_number;
    char md5_prepare[33];
    char md5_tree[33];
    char md5_graph[33];
    char md5_objects[33];

    // initialize to {6ba7b810-9dad-11d1-80b4-00c04fd430c8}
    boost::uuids::uuid named_uuid;
    bool has_64_bits;
};

#endif /* FingerPrint_H */
