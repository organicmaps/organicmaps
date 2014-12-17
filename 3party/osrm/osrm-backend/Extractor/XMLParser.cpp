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

#include "XMLParser.h"

#include "ExtractionWay.h"
#include "ExtractorCallbacks.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/ImportNode.h"
#include "../DataStructures/InputReaderFactory.h"
#include "../DataStructures/Restriction.h"
#include "../Util/cast.hpp"
#include "../Util/simple_logger.hpp"
#include "../Util/StringUtil.h"
#include "../typedefs.h"

#include <osrm/Coordinate.h>

XMLParser::XMLParser(const char *filename,
                     ExtractorCallbacks *extractor_callbacks,
                     ScriptingEnvironment &scripting_environment)
    : BaseParser(extractor_callbacks, scripting_environment)
{
    inputReader = inputReaderFactory(filename);
}

bool XMLParser::ReadHeader() { return xmlTextReaderRead(inputReader) == 1; }
bool XMLParser::Parse()
{
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int type = xmlTextReaderNodeType(inputReader);

        // 1 is Element
        if (type != 1)
        {
            continue;
        }

        xmlChar *currentName = xmlTextReaderName(inputReader);
        if (currentName == nullptr)
        {
            continue;
        }

        if (xmlStrEqual(currentName, (const xmlChar *)"node") == 1)
        {
            ImportNode current_node = ReadXMLNode();
            ParseNodeInLua(current_node, lua_state);
            extractor_callbacks->ProcessNode(current_node);
        }

        if (xmlStrEqual(currentName, (const xmlChar *)"way") == 1)
        {
            ExtractionWay way = ReadXMLWay();
            ParseWayInLua(way, lua_state);
            extractor_callbacks->ProcessWay(way);
        }
        if (use_turn_restrictions && xmlStrEqual(currentName, (const xmlChar *)"relation") == 1)
        {
            InputRestrictionContainer current_restriction = ReadXMLRestriction();
            if ((UINT_MAX != current_restriction.fromWay) &&
                !extractor_callbacks->ProcessRestriction(current_restriction))
            {
                std::cerr << "[XMLParser] restriction not parsed" << std::endl;
            }
        }
        xmlFree(currentName);
    }
    return true;
}

InputRestrictionContainer XMLParser::ReadXMLRestriction()
{

    InputRestrictionContainer restriction;

    if (xmlTextReaderIsEmptyElement(inputReader) == 1)
    {
        return restriction;
    }

    std::string except_tag_string;
    const int depth = xmlTextReaderDepth(inputReader);
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int child_type = xmlTextReaderNodeType(inputReader);
        if (child_type != 1 && child_type != 15)
        {
            continue;
        }
        const int child_depth = xmlTextReaderDepth(inputReader);
        xmlChar *child_name = xmlTextReaderName(inputReader);
        if (child_name == nullptr)
        {
            continue;
        }
        if (depth == child_depth && child_type == 15 &&
            xmlStrEqual(child_name, (const xmlChar *)"relation") == 1)
        {
            xmlFree(child_name);
            break;
        }
        if (child_type != 1)
        {
            xmlFree(child_name);
            continue;
        }

        if (xmlStrEqual(child_name, (const xmlChar *)"tag") == 1)
        {
            xmlChar *key = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"k");
            xmlChar *value = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"v");
            if (key != nullptr && value != nullptr)
            {
                if (xmlStrEqual(key, (const xmlChar *)"restriction") &&
                    StringStartsWith((const char *)value, "only_"))
                {
                    restriction.restriction.flags.isOnly = true;
                }
                if (xmlStrEqual(key, (const xmlChar *)"except"))
                {
                    except_tag_string = (const char *)value;
                }
            }

            if (key != nullptr)
            {
                xmlFree(key);
            }
            if (value != nullptr)
            {
                xmlFree(value);
            }
        }
        else if (xmlStrEqual(child_name, (const xmlChar *)"member") == 1)
        {
            xmlChar *ref = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"ref");
            if (ref != nullptr)
            {
                xmlChar *role = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"role");
                xmlChar *type = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"type");

                if (xmlStrEqual(role, (const xmlChar *)"to") &&
                    xmlStrEqual(type, (const xmlChar *)"way"))
                {
                    restriction.toWay = cast::string_to_uint((const char *)ref);
                }
                if (xmlStrEqual(role, (const xmlChar *)"from") &&
                    xmlStrEqual(type, (const xmlChar *)"way"))
                {
                    restriction.fromWay = cast::string_to_uint((const char *)ref);
                }
                if (xmlStrEqual(role, (const xmlChar *)"via") &&
                    xmlStrEqual(type, (const xmlChar *)"node"))
                {
                    restriction.restriction.viaNode = cast::string_to_uint((const char *)ref);
                }

                if (nullptr != type)
                {
                    xmlFree(type);
                }
                if (nullptr != role)
                {
                    xmlFree(role);
                }
                if (nullptr != ref)
                {
                    xmlFree(ref);
                }
            }
        }
        xmlFree(child_name);
    }

    if (ShouldIgnoreRestriction(except_tag_string))
    {
        restriction.fromWay = UINT_MAX; // workaround to ignore the restriction
    }
    return restriction;
}

ExtractionWay XMLParser::ReadXMLWay()
{
    ExtractionWay way;
    if (xmlTextReaderIsEmptyElement(inputReader) == 1)
    {
        return way;
    }
    const int depth = xmlTextReaderDepth(inputReader);
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int child_type = xmlTextReaderNodeType(inputReader);
        if (child_type != 1 && child_type != 15)
        {
            continue;
        }
        const int child_depth = xmlTextReaderDepth(inputReader);
        xmlChar *child_name = xmlTextReaderName(inputReader);
        if (child_name == nullptr)
        {
            continue;
        }

        if (depth == child_depth && child_type == 15 &&
            xmlStrEqual(child_name, (const xmlChar *)"way") == 1)
        {
            xmlChar *way_id = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"id");
            way.id = cast::string_to_uint((char *)way_id);
            xmlFree(way_id);
            xmlFree(child_name);
            break;
        }
        if (child_type != 1)
        {
            xmlFree(child_name);
            continue;
        }

        if (xmlStrEqual(child_name, (const xmlChar *)"tag") == 1)
        {
            xmlChar *key = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"k");
            xmlChar *value = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"v");

            if (key != nullptr && value != nullptr)
            {
                way.keyVals.Add(std::string((char *)key), std::string((char *)value));
            }
            if (key != nullptr)
            {
                xmlFree(key);
            }
            if (value != nullptr)
            {
                xmlFree(value);
            }
        }
        else if (xmlStrEqual(child_name, (const xmlChar *)"nd") == 1)
        {
            xmlChar *ref = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"ref");
            if (ref != nullptr)
            {
                way.path.push_back(cast::string_to_uint((const char *)ref));
                xmlFree(ref);
            }
        }
        xmlFree(child_name);
    }
    return way;
}

ImportNode XMLParser::ReadXMLNode()
{
    ImportNode node;

    xmlChar *attribute = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"lat");
    if (attribute != nullptr)
    {
        node.lat = static_cast<int>(COORDINATE_PRECISION * cast::string_to_double((const char *)attribute));
        xmlFree(attribute);
    }
    attribute = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"lon");
    if (attribute != nullptr)
    {
        node.lon = static_cast<int>(COORDINATE_PRECISION * cast::string_to_double((const char *)attribute));
        xmlFree(attribute);
    }
    attribute = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"id");
    if (attribute != nullptr)
    {
        node.node_id = cast::string_to_uint((const char *)attribute);
        xmlFree(attribute);
    }

    if (xmlTextReaderIsEmptyElement(inputReader) == 1)
    {
        return node;
    }
    const int depth = xmlTextReaderDepth(inputReader);
    while (xmlTextReaderRead(inputReader) == 1)
    {
        const int child_type = xmlTextReaderNodeType(inputReader);
        // 1 = Element, 15 = EndElement
        if (child_type != 1 && child_type != 15)
        {
            continue;
        }
        const int child_depth = xmlTextReaderDepth(inputReader);
        xmlChar *child_name = xmlTextReaderName(inputReader);
        if (child_name == nullptr)
        {
            continue;
        }

        if (depth == child_depth && child_type == 15 &&
            xmlStrEqual(child_name, (const xmlChar *)"node") == 1)
        {
            xmlFree(child_name);
            break;
        }
        if (child_type != 1)
        {
            xmlFree(child_name);
            continue;
        }

        if (xmlStrEqual(child_name, (const xmlChar *)"tag") == 1)
        {
            xmlChar *key = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"k");
            xmlChar *value = xmlTextReaderGetAttribute(inputReader, (const xmlChar *)"v");
            if (key != nullptr && value != nullptr)
            {
                node.keyVals.Add(std::string((char *)(key)), std::string((char *)(value)));
            }
            if (key != nullptr)
            {
                xmlFree(key);
            }
            if (value != nullptr)
            {
                xmlFree(value);
            }
        }

        xmlFree(child_name);
    }
    return node;
}
