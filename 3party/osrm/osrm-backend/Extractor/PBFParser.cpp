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

#include "PBFParser.h"

#include "ExtractionWay.h"
#include "ExtractorCallbacks.h"
#include "ScriptingEnvironment.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/Restriction.h"
#include "../Util/MachineInfo.h"
#include "../Util/OSRMException.h"
#include "../Util/simple_logger.hpp"
#include "../typedefs.h"

#include <boost/assert.hpp>

#include <tbb/parallel_for.h>
#include <tbb/task_scheduler_init.h>

#include <osrm/Coordinate.h>

#include <zlib.h>

#include <functional>
#include <iostream>
#include <limits>
#include <thread>

PBFParser::PBFParser(const char *fileName,
                     ExtractorCallbacks *extractor_callbacks,
                     ScriptingEnvironment &scripting_environment,
                     unsigned num_threads)
    : BaseParser(extractor_callbacks, scripting_environment)
{
    if (0 == num_threads)
    {
        num_parser_threads = tbb::task_scheduler_init::default_num_threads();
    }
    else
    {
        num_parser_threads = num_threads;
    }

    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // TODO: What is the bottleneck here? Filling the queue or reading the stuff from disk?
    // NOTE: With Lua scripting, it is parsing the stuff. I/O is virtually for free.

    // Max 2500 items in queue, hardcoded.
    thread_data_queue = std::make_shared<ConcurrentQueue<ParserThreadData *>>(2500);
    input.open(fileName, std::ios::in | std::ios::binary);

    if (!input)
    {
        throw OSRMException("pbf file not found.");
    }

    block_count = 0;
    group_count = 0;
}

PBFParser::~PBFParser()
{
    if (input.is_open())
    {
        input.close();
    }

    // Clean up any leftover ThreadData objects in the queue
    ParserThreadData *thread_data;
    while (thread_data_queue->try_pop(thread_data))
    {
        delete thread_data;
    }
    google::protobuf::ShutdownProtobufLibrary();

    SimpleLogger().Write(logDEBUG) << "parsed " << block_count << " blocks from pbf with "
                                   << group_count << " groups";
}

inline bool PBFParser::ReadHeader()
{
    ParserThreadData init_data;
    /** read Header */
    if (!readPBFBlobHeader(input, &init_data))
    {
        return false;
    }

    if (readBlob(input, &init_data))
    {
        if (!init_data.PBFHeaderBlock.ParseFromArray(&(init_data.charBuffer[0]),
                                                     static_cast<int>(init_data.charBuffer.size())))
        {
            std::cerr << "[error] Header not parseable!" << std::endl;
            return false;
        }

        const auto feature_size = init_data.PBFHeaderBlock.required_features_size();
        for (int i = 0; i < feature_size; ++i)
        {
            const std::string &feature = init_data.PBFHeaderBlock.required_features(i);
            bool supported = false;
            if ("OsmSchema-V0.6" == feature)
            {
                supported = true;
            }
            else if ("DenseNodes" == feature)
            {
                supported = true;
            }

            if (!supported)
            {
                std::cerr << "[error] required feature not supported: " << feature.data()
                          << std::endl;
                return false;
            }
        }
    }
    else
    {
        std::cerr << "[error] blob not loaded!" << std::endl;
    }
    return true;
}

inline void PBFParser::ReadData()
{
    bool keep_running = true;
    do
    {
        ParserThreadData *thread_data = new ParserThreadData();
        keep_running = readNextBlock(input, thread_data);

        if (keep_running)
        {
            thread_data_queue->push(thread_data);
        }
        else
        {
            // No more data to read, parse stops when nullptr encountered
            thread_data_queue->push(nullptr);
            delete thread_data;
        }
    } while (keep_running);
}

inline void PBFParser::ParseData()
{
    tbb::task_scheduler_init init(num_parser_threads);

    while (true)
    {
        ParserThreadData *thread_data;
        thread_data_queue->wait_and_pop(thread_data);
        if (nullptr == thread_data)
        {
            thread_data_queue->push(nullptr); // Signal end of data for other threads
            break;
        }

        loadBlock(thread_data);

        int group_size = thread_data->PBFprimitiveBlock.primitivegroup_size();
        for (int i = 0; i < group_size; ++i)
        {
            thread_data->currentGroupID = i;
            loadGroup(thread_data);

            if (thread_data->entityTypeIndicator == TypeNode)
            {
                parseNode(thread_data);
            }
            if (thread_data->entityTypeIndicator == TypeWay)
            {
                parseWay(thread_data);
            }
            if (thread_data->entityTypeIndicator == TypeRelation)
            {
                parseRelation(thread_data);
            }
            if (thread_data->entityTypeIndicator == TypeDenseNode)
            {
                parseDenseNode(thread_data);
            }
        }

        delete thread_data;
        thread_data = nullptr;
    }
}

inline bool PBFParser::Parse()
{
    // Start the read and parse threads
    std::thread read_thread(std::bind(&PBFParser::ReadData, this));

    // Open several parse threads that are synchronized before call to
    std::thread parse_thread(std::bind(&PBFParser::ParseData, this));

    // Wait for the threads to finish
    read_thread.join();
    parse_thread.join();

    return true;
}

inline void PBFParser::parseDenseNode(ParserThreadData *thread_data)
{
    const OSMPBF::DenseNodes &dense =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).dense();
    int denseTagIndex = 0;
    int64_t m_lastDenseID = 0;
    int64_t m_lastDenseLatitude = 0;
    int64_t m_lastDenseLongitude = 0;

    const int number_of_nodes = dense.id_size();
    std::vector<ImportNode> extracted_nodes_vector(number_of_nodes);
    for (int i = 0; i < number_of_nodes; ++i)
    {
        m_lastDenseID += dense.id(i);
        m_lastDenseLatitude += dense.lat(i);
        m_lastDenseLongitude += dense.lon(i);
        extracted_nodes_vector[i].node_id = static_cast<NodeID>(m_lastDenseID);
        extracted_nodes_vector[i].lat = static_cast<int>(
            COORDINATE_PRECISION *
            ((double)m_lastDenseLatitude * thread_data->PBFprimitiveBlock.granularity() +
             thread_data->PBFprimitiveBlock.lat_offset()) /
            NANO);
        extracted_nodes_vector[i].lon = static_cast<int>(
            COORDINATE_PRECISION *
            ((double)m_lastDenseLongitude * thread_data->PBFprimitiveBlock.granularity() +
             thread_data->PBFprimitiveBlock.lon_offset()) /
            NANO);
        while (denseTagIndex < dense.keys_vals_size())
        {
            const int tagValue = dense.keys_vals(denseTagIndex);
            if (0 == tagValue)
            {
                ++denseTagIndex;
                break;
            }
            const int keyValue = dense.keys_vals(denseTagIndex + 1);
            const std::string &key = thread_data->PBFprimitiveBlock.stringtable().s(tagValue);
            const std::string &value = thread_data->PBFprimitiveBlock.stringtable().s(keyValue);
            extracted_nodes_vector[i].keyVals.Add(std::move(key), std::move(value));
            denseTagIndex += 2;
        }
    }

    tbb::parallel_for(tbb::blocked_range<size_t>(0, extracted_nodes_vector.size()),
                      [this, &extracted_nodes_vector](const tbb::blocked_range<size_t> &range)
                      {
        lua_State *lua_state = this->scripting_environment.getLuaState();
        for (size_t i = range.begin(); i != range.end(); ++i)
        {
            ImportNode &import_node = extracted_nodes_vector[i];
            ParseNodeInLua(import_node, lua_state);
        }
    });

    for (const ImportNode &import_node : extracted_nodes_vector)
    {
        extractor_callbacks->ProcessNode(import_node);
    }
}

inline void PBFParser::parseNode(ParserThreadData *)
{
    throw OSRMException("Parsing of simple nodes not supported. PBF should use dense nodes");
}

inline void PBFParser::parseRelation(ParserThreadData *thread_data)
{
    // TODO: leave early, if relation is not a restriction
    // TODO: reuse rawRestriction container
    if (!use_turn_restrictions)
    {
        return;
    }
    const OSMPBF::PrimitiveGroup &group =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID);

    for (int i = 0, relation_size = group.relations_size(); i < relation_size; ++i)
    {
        std::string except_tag_string;
        const OSMPBF::Relation &inputRelation =
            thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).relations(i);
        bool is_restriction = false;
        bool is_only_restriction = false;
        for (int k = 0, endOfKeys = inputRelation.keys_size(); k < endOfKeys; ++k)
        {
            const std::string &key =
                thread_data->PBFprimitiveBlock.stringtable().s(inputRelation.keys(k));
            const std::string &val =
                thread_data->PBFprimitiveBlock.stringtable().s(inputRelation.vals(k));
            if ("type" == key)
            {
                if ("restriction" == val)
                {
                    is_restriction = true;
                }
                else
                {
                    break;
                }
            }
            if (("restriction" == key) && (val.find("only_") == 0))
            {
                is_only_restriction = true;
            }
            if ("except" == key)
            {
                except_tag_string = val;
            }
        }

        if (is_restriction && ShouldIgnoreRestriction(except_tag_string))
        {
            continue;
        }

        if (is_restriction)
        {
            int64_t last_ref = 0;
            InputRestrictionContainer current_restriction_container(is_only_restriction);
            for (int rolesIndex = 0, last_role = inputRelation.roles_sid_size();
                 rolesIndex < last_role;
                 ++rolesIndex)
            {
                const std::string &role = thread_data->PBFprimitiveBlock.stringtable().s(
                    inputRelation.roles_sid(rolesIndex));
                last_ref += inputRelation.memids(rolesIndex);

                if (!("from" == role || "to" == role || "via" == role))
                {
                    continue;
                }

                switch (inputRelation.types(rolesIndex))
                {
                case 0: // node
                    if ("from" == role || "to" == role)
                    { // Only via should be a node
                        continue;
                    }
                    BOOST_ASSERT("via" == role);
                    if (std::numeric_limits<unsigned>::max() !=
                        current_restriction_container.viaNode)
                    {
                        current_restriction_container.viaNode =
                            std::numeric_limits<unsigned>::max();
                    }
                    BOOST_ASSERT(std::numeric_limits<unsigned>::max() ==
                                 current_restriction_container.viaNode);
                    current_restriction_container.restriction.viaNode =
                        static_cast<NodeID>(last_ref);
                    break;
                case 1: // way
                    BOOST_ASSERT("from" == role || "to" == role || "via" == role);
                    if ("from" == role)
                    {
                        current_restriction_container.fromWay = static_cast<EdgeID>(last_ref);
                    }
                    if ("to" == role)
                    {
                        current_restriction_container.toWay = static_cast<EdgeID>(last_ref);
                    }
                    if ("via" == role)
                    {
                        BOOST_ASSERT(current_restriction_container.restriction.toNode ==
                                     std::numeric_limits<unsigned>::max());
                        current_restriction_container.viaNode = static_cast<NodeID>(last_ref);
                    }
                    break;
                case 2: // relation, not used. relations relating to relations are evil.
                    continue;
                    BOOST_ASSERT(false);
                    break;

                default: // should not happen
                    BOOST_ASSERT(false);
                    break;
                }
            }
            if (!extractor_callbacks->ProcessRestriction(current_restriction_container))
            {
                std::cerr << "[PBFParser] relation not parsed" << std::endl;
            }
        }
    }
}

inline void PBFParser::parseWay(ParserThreadData *thread_data)
{
    const int number_of_ways =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).ways_size();
    std::vector<ExtractionWay> parsed_way_vector(number_of_ways);
    for (int i = 0; i < number_of_ways; ++i)
    {
        const OSMPBF::Way &input_way =
            thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID).ways(i);
        parsed_way_vector[i].id = static_cast<EdgeID>(input_way.id());
        unsigned node_id_in_path = 0;
        const auto number_of_referenced_nodes = input_way.refs_size();
        for (auto j = 0; j < number_of_referenced_nodes; ++j)
        {
            node_id_in_path += static_cast<NodeID>(input_way.refs(j));
            parsed_way_vector[i].path.push_back(node_id_in_path);
        }
        BOOST_ASSERT(input_way.keys_size() == input_way.vals_size());
        const auto number_of_keys = input_way.keys_size();
        for (auto j = 0; j < number_of_keys; ++j)
        {
            const std::string &key =
                thread_data->PBFprimitiveBlock.stringtable().s(input_way.keys(j));
            const std::string &val =
                thread_data->PBFprimitiveBlock.stringtable().s(input_way.vals(j));
            parsed_way_vector[i].keyVals.Add(std::move(key), std::move(val));
        }
    }

    // TODO: investigate if schedule guided will be handled by tbb automatically
    tbb::parallel_for(tbb::blocked_range<size_t>(0, parsed_way_vector.size()),
                      [this, &parsed_way_vector](const tbb::blocked_range<size_t> &range)
                      {
        lua_State *lua_state = this->scripting_environment.getLuaState();
        for (size_t i = range.begin(); i != range.end(); i++)
        {
            ExtractionWay &extraction_way = parsed_way_vector[i];
            if (2 <= extraction_way.path.size())
            {
                ParseWayInLua(extraction_way, lua_state);
            }
        }
    });

    for (ExtractionWay &extraction_way : parsed_way_vector)
    {
        if (2 <= extraction_way.path.size())
        {
            extractor_callbacks->ProcessWay(extraction_way);
        }
    }
}

inline void PBFParser::loadGroup(ParserThreadData *thread_data)
{
#ifndef NDEBUG
    ++group_count;
#endif

    const OSMPBF::PrimitiveGroup &group =
        thread_data->PBFprimitiveBlock.primitivegroup(thread_data->currentGroupID);
    thread_data->entityTypeIndicator = TypeDummy;
    if (0 != group.nodes_size())
    {
        thread_data->entityTypeIndicator = TypeNode;
    }
    if (0 != group.ways_size())
    {
        thread_data->entityTypeIndicator = TypeWay;
    }
    if (0 != group.relations_size())
    {
        thread_data->entityTypeIndicator = TypeRelation;
    }
    if (group.has_dense())
    {
        thread_data->entityTypeIndicator = TypeDenseNode;
        BOOST_ASSERT(0 != group.dense().id_size());
    }
    BOOST_ASSERT(thread_data->entityTypeIndicator != TypeDummy);
}

inline void PBFParser::loadBlock(ParserThreadData *thread_data)
{
    ++block_count;
    thread_data->currentGroupID = 0;
    thread_data->currentEntityID = 0;
}

inline bool PBFParser::readPBFBlobHeader(std::fstream &stream, ParserThreadData *thread_data)
{
    int size(0);
    stream.read((char *)&size, sizeof(int));
    size = SwapEndian(size);
    if (stream.eof())
    {
        return false;
    }
    if (size > MAX_BLOB_HEADER_SIZE || size < 0)
    {
        return false;
    }
    char *data = new char[size];
    stream.read(data, size * sizeof(data[0]));

    bool dataSuccessfullyParsed = (thread_data->PBFBlobHeader).ParseFromArray(data, size);
    delete[] data;
    return dataSuccessfullyParsed;
}

inline bool PBFParser::unpackZLIB(ParserThreadData *thread_data)
{
    auto raw_size = thread_data->PBFBlob.raw_size();
    char *unpacked_data_array = new char[raw_size];
    z_stream compressed_data_stream;
    compressed_data_stream.next_in = (unsigned char *)thread_data->PBFBlob.zlib_data().data();
    compressed_data_stream.avail_in = thread_data->PBFBlob.zlib_data().size();
    compressed_data_stream.next_out = (unsigned char *)unpacked_data_array;
    compressed_data_stream.avail_out = raw_size;
    compressed_data_stream.zalloc = Z_NULL;
    compressed_data_stream.zfree = Z_NULL;
    compressed_data_stream.opaque = Z_NULL;
    int return_code = inflateInit(&compressed_data_stream);
    if (return_code != Z_OK)
    {
        std::cerr << "[error] failed to init zlib stream" << std::endl;
        delete[] unpacked_data_array;
        return false;
    }

    return_code = inflate(&compressed_data_stream, Z_FINISH);
    if (return_code != Z_STREAM_END)
    {
        std::cerr << "[error] failed to inflate zlib stream" << std::endl;
        std::cerr << "[error] Error type: " << return_code << std::endl;
        delete[] unpacked_data_array;
        return false;
    }

    return_code = inflateEnd(&compressed_data_stream);
    if (return_code != Z_OK)
    {
        std::cerr << "[error] failed to deinit zlib stream" << std::endl;
        delete[] unpacked_data_array;
        return false;
    }

    thread_data->charBuffer.clear();
    thread_data->charBuffer.resize(raw_size);
    std::copy(unpacked_data_array, unpacked_data_array + raw_size, thread_data->charBuffer.begin());
    delete[] unpacked_data_array;
    return true;
}

inline bool PBFParser::unpackLZMA(ParserThreadData *) { return false; }

inline bool PBFParser::readBlob(std::fstream &stream, ParserThreadData *thread_data)
{
    if (stream.eof())
    {
        return false;
    }

    const int size = thread_data->PBFBlobHeader.datasize();
    if (size < 0 || size > MAX_BLOB_SIZE)
    {
        std::cerr << "[error] invalid Blob size:" << size << std::endl;
        return false;
    }

    char *data = new char[size];
    stream.read(data, sizeof(data[0]) * size);

    if (!thread_data->PBFBlob.ParseFromArray(data, size))
    {
        std::cerr << "[error] failed to parse blob" << std::endl;
        delete[] data;
        return false;
    }

    if (thread_data->PBFBlob.has_raw())
    {
        const std::string &data = thread_data->PBFBlob.raw();
        thread_data->charBuffer.clear();
        thread_data->charBuffer.resize(data.size());
        std::copy(data.begin(), data.end(), thread_data->charBuffer.begin());
    }
    else if (thread_data->PBFBlob.has_zlib_data())
    {
        if (!unpackZLIB(thread_data))
        {
            std::cerr << "[error] zlib data encountered that could not be unpacked" << std::endl;
            delete[] data;
            return false;
        }
    }
    else if (thread_data->PBFBlob.has_lzma_data())
    {
        if (!unpackLZMA(thread_data))
        {
            std::cerr << "[error] lzma data encountered that could not be unpacked" << std::endl;
        }
        delete[] data;
        return false;
    }
    else
    {
        std::cerr << "[error] Blob contains no data" << std::endl;
        delete[] data;
        return false;
    }
    delete[] data;
    return true;
}

bool PBFParser::readNextBlock(std::fstream &stream, ParserThreadData *thread_data)
{
    if (stream.eof())
    {
        return false;
    }

    if (!readPBFBlobHeader(stream, thread_data))
    {
        return false;
    }

    if (thread_data->PBFBlobHeader.type() != "OSMData")
    {
        return false;
    }

    if (!readBlob(stream, thread_data))
    {
        return false;
    }

    if (!thread_data->PBFprimitiveBlock.ParseFromArray(&(thread_data->charBuffer[0]),
                                                       thread_data->charBuffer.size()))
    {
        std::cerr << "failed to parse PrimitiveBlock" << std::endl;
        return false;
    }
    return true;
}
