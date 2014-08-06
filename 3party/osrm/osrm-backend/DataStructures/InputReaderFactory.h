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

#ifndef INPUT_READER_FACTORY_H
#define INPUT_READER_FACTORY_H

#include <boost/assert.hpp>

#include <bzlib.h>
#include <libxml/xmlreader.h>

struct BZ2Context
{
    FILE *file;
    BZFILE *bz2;
    int error;
    int nUnused;
    char unused[BZ_MAX_UNUSED];
};

int readFromBz2Stream(void *pointer, char *buffer, int len)
{
    void *unusedTmpVoid = nullptr;
    char *unusedTmp = nullptr;
    BZ2Context *context = (BZ2Context *)pointer;
    int read = 0;
    while (0 == read &&
           !(BZ_STREAM_END == context->error && 0 == context->nUnused && feof(context->file)))
    {
        read = BZ2_bzRead(&context->error, context->bz2, buffer, len);
        if (BZ_OK == context->error)
        {
            return read;
        }
        else if (BZ_STREAM_END == context->error)
        {
            BZ2_bzReadGetUnused(&context->error, context->bz2, &unusedTmpVoid, &context->nUnused);
            BOOST_ASSERT_MSG(BZ_OK == context->error, "Could not BZ2_bzReadGetUnused");
            unusedTmp = (char *)unusedTmpVoid;
            for (int i = 0; i < context->nUnused; i++)
            {
                context->unused[i] = unusedTmp[i];
            }
            BZ2_bzReadClose(&context->error, context->bz2);
            BOOST_ASSERT_MSG(BZ_OK == context->error, "Could not BZ2_bzReadClose");
            context->error = BZ_STREAM_END; // set to the stream end for next call to this function
            if (0 == context->nUnused && feof(context->file))
            {
                return read;
            }
            else
            {
                context->bz2 = BZ2_bzReadOpen(
                    &context->error, context->file, 0, 0, context->unused, context->nUnused);
                BOOST_ASSERT_MSG(nullptr != context->bz2, "Could not open file");
            }
        }
        else
        {
            BOOST_ASSERT_MSG(false, "Could not read bz2 file");
        }
    }
    return read;
}

int closeBz2Stream(void *pointer)
{
    BZ2Context *context = (BZ2Context *)pointer;
    fclose(context->file);
    delete context;
    return 0;
}

xmlTextReaderPtr inputReaderFactory(const char *name)
{
    std::string inputName(name);

    if (inputName.find(".osm.bz2") != std::string::npos)
    {
        BZ2Context *context = new BZ2Context();
        context->error = false;
        context->file = fopen(name, "r");
        int error;
        context->bz2 =
            BZ2_bzReadOpen(&error, context->file, 0, 0, context->unused, context->nUnused);
        if (context->bz2 == nullptr || context->file == nullptr)
        {
            delete context;
            return nullptr;
        }
        return xmlReaderForIO(readFromBz2Stream, closeBz2Stream, (void *)context, nullptr, nullptr, 0);
    }
    else
    {
        return xmlNewTextReaderFilename(name);
    }
}

#endif // INPUT_READER_FACTORY_H
