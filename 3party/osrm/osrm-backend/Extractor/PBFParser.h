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

#ifndef PBFPARSER_H_
#define PBFPARSER_H_

#include "BaseParser.h"
#include "../DataStructures/ConcurrentQueue.h"

#include <osmpbf/fileformat.pb.h>
#include <osmpbf/osmformat.pb.h>

#include <fstream>
#include <memory>

class PBFParser : public BaseParser
{

    enum EntityType
    { TypeDummy = 0,
      TypeNode = 1,
      TypeWay = 2,
      TypeRelation = 4,
      TypeDenseNode = 8 };

    struct ParserThreadData
    {
        int currentGroupID;
        int currentEntityID;
        EntityType entityTypeIndicator;

        OSMPBF::BlobHeader PBFBlobHeader;
        OSMPBF::Blob PBFBlob;

        OSMPBF::HeaderBlock PBFHeaderBlock;
        OSMPBF::PrimitiveBlock PBFprimitiveBlock;

        std::vector<char> charBuffer;
    };

  public:
    PBFParser(const char *file_name,
              ExtractorCallbacks *extractor_callbacks,
              ScriptingEnvironment &scripting_environment,
              unsigned num_parser_threads = 0);
    virtual ~PBFParser();

    inline bool ReadHeader();
    inline bool Parse();

  private:
    inline void ReadData();
    inline void ParseData();
    inline void parseDenseNode(ParserThreadData *thread_data);
    inline void parseNode(ParserThreadData *thread_data);
    inline void parseRelation(ParserThreadData *thread_data);
    inline void parseWay(ParserThreadData *thread_data);

    inline void loadGroup(ParserThreadData *thread_data);
    inline void loadBlock(ParserThreadData *thread_data);
    inline bool readPBFBlobHeader(std::fstream &stream, ParserThreadData *thread_data);
    inline bool unpackZLIB(ParserThreadData *thread_data);
    inline bool unpackLZMA(ParserThreadData *thread_data);
    inline bool readBlob(std::fstream &stream, ParserThreadData *thread_data);
    inline bool readNextBlock(std::fstream &stream, ParserThreadData *thread_data);

    static const int NANO = 1000 * 1000 * 1000;
    static const int MAX_BLOB_HEADER_SIZE = 64 * 1024;
    static const int MAX_BLOB_SIZE = 32 * 1024 * 1024;

    unsigned group_count;
    unsigned block_count;

    std::fstream input; // the input stream to parse
    std::shared_ptr<ConcurrentQueue<ParserThreadData *>> thread_data_queue;
    unsigned num_parser_threads;
};

#endif /* PBFPARSER_H_ */
