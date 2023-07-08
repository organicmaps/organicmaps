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

#include "request_handler.hpp"

#include "api_grammar.hpp"
#include "http/reply.hpp"
#include "http/request.hpp"

#include "../library/osrm.hpp"
#include "../util/json_renderer.hpp"
#include "../util/simple_logger.hpp"
#include "../util/string_util.hpp"
#include "../util/xml_renderer.hpp"
#include "../typedefs.h"

#include <osrm/route_parameters.hpp>
#include <osrm/json_container.hpp>

#include <ctime>

#include <algorithm>
#include <iostream>

RequestHandler::RequestHandler() : routing_machine(nullptr) {}

void RequestHandler::handle_request(const http::request &current_request,
                                    http::reply &current_reply)
{
    // parse command
    try
    {
        std::string request_string;
        URIDecode(current_request.uri, request_string);

        // deactivated as GCC apparently does not implement that, not even in 4.9
        // std::time_t t = std::time(nullptr);
        // SimpleLogger().Write() << std::put_time(std::localtime(&t), "%m-%d-%Y %H:%M:%S") <<
        //     " " << current_request.endpoint.to_string() << " " <<
        //     current_request.referrer << ( 0 == current_request.referrer.length() ? "- " :" ") <<
        //     current_request.agent << ( 0 == current_request.agent.length() ? "- " :" ") <<
        //     request;

        time_t ltime;
        struct tm *time_stamp;

        ltime = time(nullptr);
        time_stamp = localtime(&ltime);

        // log timestamp
        SimpleLogger().Write() << (time_stamp->tm_mday < 10 ? "0" : "") << time_stamp->tm_mday
                               << "-" << (time_stamp->tm_mon + 1 < 10 ? "0" : "")
                               << (time_stamp->tm_mon + 1) << "-" << 1900 + time_stamp->tm_year
                               << " " << (time_stamp->tm_hour < 10 ? "0" : "")
                               << time_stamp->tm_hour << ":" << (time_stamp->tm_min < 10 ? "0" : "")
                               << time_stamp->tm_min << ":" << (time_stamp->tm_sec < 10 ? "0" : "")
                               << time_stamp->tm_sec << " " << current_request.endpoint.to_string()
                               << " " << current_request.referrer
                               << (0 == current_request.referrer.length() ? "- " : " ")
                               << current_request.agent
                               << (0 == current_request.agent.length() ? "- " : " ")
                               << request_string;

        RouteParameters route_parameters;
        APIGrammarParser api_parser(&route_parameters);

        auto api_iterator = request_string.begin();
        const bool result =
            boost::spirit::qi::parse(api_iterator, request_string.end(), api_parser);

        osrm::json::Object json_result;
        // check if the was an error with the request
        if (!result || (api_iterator != request_string.end()))
        {
            current_reply = http::reply::stock_reply(http::reply::bad_request);
            current_reply.content.clear();
            const auto position = std::distance(request_string.begin(), api_iterator);

            json_result.values["status"] = 400;
            std::string message = "Query string malformed close to position ";
            message += cast::integral_to_string(position);
            json_result.values["status_message"] = message;
            osrm::json::render(current_reply.content, json_result);
            return;
        }

        // parsing done, lets call the right plugin to handle the request
        BOOST_ASSERT_MSG(routing_machine != nullptr, "pointer not init'ed");

        if (!route_parameters.jsonp_parameter.empty())
        { // prepend response with jsonp parameter
            const std::string json_p = (route_parameters.jsonp_parameter + "(");
            current_reply.content.insert(current_reply.content.end(), json_p.begin(), json_p.end());
        }
        const auto return_code = routing_machine->RunQuery(route_parameters, json_result);
        if (200 != return_code)
        {
            current_reply = http::reply::stock_reply(http::reply::bad_request);
            current_reply.content.clear();
            json_result.values["status"] = 400;
            std::string message = "Bad Request";
            json_result.values["status_message"] = message;
            osrm::json::render(current_reply.content, json_result);
            return;
        }

        current_reply.headers.emplace_back("Access-Control-Allow-Origin", "*");
        current_reply.headers.emplace_back("Access-Control-Allow-Methods", "GET");
        current_reply.headers.emplace_back("Access-Control-Allow-Headers",
                                           "X-Requested-With, Content-Type");

        // set headers
        current_reply.headers.emplace_back("Content-Length",
                                           cast::integral_to_string(current_reply.content.size()));
        if ("gpx" == route_parameters.output_format)
        { // gpx file
            osrm::json::gpx_render(current_reply.content, json_result.values["route"]);
            current_reply.headers.emplace_back("Content-Type",
                                               "application/gpx+xml; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "attachment; filename=\"route.gpx\"");
        }
        else if (route_parameters.jsonp_parameter.empty())
        { // json file
            osrm::json::render(current_reply.content, json_result);
            current_reply.headers.emplace_back("Content-Type", "application/json; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "inline; filename=\"response.json\"");
        }
        else
        { // jsonp
            osrm::json::render(current_reply.content, json_result);
            current_reply.headers.emplace_back("Content-Type", "text/javascript; charset=UTF-8");
            current_reply.headers.emplace_back("Content-Disposition",
                                               "inline; filename=\"response.js\"");
        }
        if (!route_parameters.jsonp_parameter.empty())
        { // append brace to jsonp response
            current_reply.content.push_back(')');
        }
    }
    catch (const std::exception &e)
    {
        current_reply = http::reply::stock_reply(http::reply::internal_server_error);
        SimpleLogger().Write(logWARNING) << "[server error] code: " << e.what()
                                         << ", uri: " << current_request.uri;
        return;
    }
}

void RequestHandler::RegisterRoutingMachine(OSRM *osrm) { routing_machine = osrm; }
