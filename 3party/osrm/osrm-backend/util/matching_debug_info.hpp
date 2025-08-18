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

#ifndef MATCHING_DEBUG_INFO_HPP
#define MATCHING_DEBUG_INFO_HPP

#include "json_logger.hpp"
#include "json_util.hpp"
#include "../data_structures/hidden_markov_model.hpp"

#include <osrm/coordinate.hpp>

// Provides the debug interface for introspection tools
struct MatchingDebugInfo
{
    MatchingDebugInfo(const osrm::json::Logger *logger) : logger(logger)
    {
        if (logger)
        {
            object = &logger->map->at("matching");
        }
    }

    template <class CandidateLists> void initialize(const CandidateLists &candidates_list)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        osrm::json::Array states;
        for (unsigned t = 0; t < candidates_list.size(); t++)
        {
            osrm::json::Array timestamps;
            for (unsigned s = 0; s < candidates_list[t].size(); s++)
            {
                osrm::json::Object state;
                state.values["transitions"] = osrm::json::Array();
                state.values["coordinate"] = osrm::json::make_array(
                    candidates_list[t][s].first.location.lat / COORDINATE_PRECISION,
                    candidates_list[t][s].first.location.lon / COORDINATE_PRECISION);
                state.values["viterbi"] =
                    osrm::json::clamp_float(osrm::matching::IMPOSSIBLE_LOG_PROB);
                state.values["pruned"] = 0u;
                timestamps.values.push_back(state);
            }
            states.values.push_back(timestamps);
        }
        osrm::json::get(*object, "states") = states;
    }

    void add_transition_info(const unsigned prev_t,
                             const unsigned current_t,
                             const unsigned prev_state,
                             const unsigned current_state,
                             const double prev_viterbi,
                             const double emission_pr,
                             const double transition_pr,
                             const double network_distance,
                             const double great_circle_distance)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        osrm::json::Object transistion;
        transistion.values["to"] = osrm::json::make_array(current_t, current_state);
        transistion.values["properties"] = osrm::json::make_array(
            osrm::json::clamp_float(prev_viterbi), osrm::json::clamp_float(emission_pr),
            osrm::json::clamp_float(transition_pr), network_distance, great_circle_distance);

        osrm::json::get(*object, "states", prev_t, prev_state, "transitions")
            .get<mapbox::util::recursive_wrapper<osrm::json::Array>>()
            .get()
            .values.push_back(transistion);
    }

    void set_viterbi(const std::vector<std::vector<double>> &viterbi,
                     const std::vector<std::vector<bool>> &pruned,
                     const std::vector<std::vector<bool>> &suspicious)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        for (auto t = 0u; t < viterbi.size(); t++)
        {
            for (auto s_prime = 0u; s_prime < viterbi[t].size(); ++s_prime)
            {
                osrm::json::get(*object, "states", t, s_prime, "viterbi") =
                    osrm::json::clamp_float(viterbi[t][s_prime]);
                osrm::json::get(*object, "states", t, s_prime, "pruned") =
                    static_cast<unsigned>(pruned[t][s_prime]);
                osrm::json::get(*object, "states", t, s_prime, "suspicious") =
                    static_cast<unsigned>(suspicious[t][s_prime]);
            }
        }
    }

    void add_chosen(const unsigned t, const unsigned s)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        osrm::json::get(*object, "states", t, s, "chosen") = true;
    }

    void add_breakage(const std::vector<bool> &breakage)
    {
        // json logger not enabled
        if (!logger)
        {
            return;
        }

        osrm::json::get(*object, "breakage") = osrm::json::make_array(breakage);
    }

    const osrm::json::Logger *logger;
    osrm::json::Value *object;
};

#endif // MATCHING_DEBUG_INFO_HPP
