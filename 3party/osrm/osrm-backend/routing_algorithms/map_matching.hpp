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

#ifndef MAP_MATCHING_HPP
#define MAP_MATCHING_HPP

#include "routing_base.hpp"

#include "../data_structures/coordinate_calculation.hpp"
#include "../data_structures/hidden_markov_model.hpp"
#include "../util/json_logger.hpp"
#include "../util/matching_debug_info.hpp"

#include <variant/variant.hpp>

#include <algorithm>
#include <iomanip>
#include <numeric>

namespace osrm
{
namespace matching
{

struct SubMatching
{
    std::vector<PhantomNode> nodes;
    std::vector<unsigned> indices;
    double length;
    double confidence;
};

using CandidateList = std::vector<std::pair<PhantomNode, double>>;
using CandidateLists = std::vector<CandidateList>;
using HMM = HiddenMarkovModel<CandidateLists>;
using SubMatchingList = std::vector<SubMatching>;

constexpr static const unsigned MAX_BROKEN_STATES = 6;
constexpr static const unsigned MAX_BROKEN_TIME = 60;

constexpr static const unsigned MAX_DISTANCE_DELTA = 200;
constexpr static const unsigned SUSPICIOUS_DISTANCE_DELTA = 100;

constexpr static const double default_beta = 5.0;
constexpr static const double default_sigma_z = 4.07;
}
}

// implements a hidden markov model map matching algorithm
template <class DataFacadeT>
class MapMatching final : public BasicRoutingInterface<DataFacadeT, MapMatching<DataFacadeT>>
{
    using super = BasicRoutingInterface<DataFacadeT, MapMatching<DataFacadeT>>;
    using QueryHeap = SearchEngineData::QueryHeap;
    SearchEngineData &engine_working_data;

  public:
    MapMatching(DataFacadeT *facade, SearchEngineData &engine_working_data)
        : super(facade), engine_working_data(engine_working_data)
    {
    }

    void operator()(const osrm::matching::CandidateLists &candidates_list,
                    const std::vector<FixedPointCoordinate> &trace_coordinates,
                    const std::vector<unsigned> &trace_timestamps,
                    const double matching_beta,
                    const double gps_precision,
                    osrm::matching::SubMatchingList &sub_matchings) const
    {
        BOOST_ASSERT(!candidates_list.empty() && !trace_coordinates.empty());

        // TODO replace default values with table lookup based on sampling frequency
        EmissionLogProbability emission_log_probability(
            gps_precision > 0. ? gps_precision : osrm::matching::default_sigma_z);
        TransitionLogProbability transition_log_probability(
            matching_beta > 0. ? matching_beta : osrm::matching::default_beta);

        osrm::matching::HMM model(candidates_list, emission_log_probability);

        std::size_t initial_timestamp = model.initialize(0);
        if (initial_timestamp == osrm::matching::INVALID_STATE)
        {
            return;
        }

        MatchingDebugInfo matching_debug(osrm::json::Logger::get());
        matching_debug.initialize(candidates_list);

        std::size_t breakage_begin = osrm::matching::INVALID_STATE;
        std::vector<std::size_t> split_points;
        std::vector<std::size_t> prev_unbroken_timestamps;
        prev_unbroken_timestamps.reserve(candidates_list.size());
        prev_unbroken_timestamps.push_back(initial_timestamp);
        for (auto t = initial_timestamp + 1; t < candidates_list.size(); ++t)
        {
            // breakage recover has removed all previous good points
            bool trace_split = prev_unbroken_timestamps.empty();

            // use temporal information if available to determine a split
            if (!trace_timestamps.empty())
            {
                trace_split =
                    trace_split ||
                    (trace_timestamps[t] - trace_timestamps[prev_unbroken_timestamps.back()] >
                     osrm::matching::MAX_BROKEN_TIME);
            }
            else
            {
                trace_split = trace_split || (t - prev_unbroken_timestamps.back() >
                                              osrm::matching::MAX_BROKEN_STATES);
            }

            if (trace_split)
            {
                std::size_t split_index = t;
                if (breakage_begin != osrm::matching::INVALID_STATE)
                {
                    split_index = breakage_begin;
                    breakage_begin = osrm::matching::INVALID_STATE;
                }
                split_points.push_back(split_index);

                // note: this preserves everything before split_index
                model.clear(split_index);
                std::size_t new_start = model.initialize(split_index);
                // no new start was found -> stop viterbi calculation
                if (new_start == osrm::matching::INVALID_STATE)
                {
                    break;
                }

                prev_unbroken_timestamps.clear();
                prev_unbroken_timestamps.push_back(new_start);
                // Important: We potentially go back here!
                // However since t > new_start >= breakge_begin
                // we can only reset trace_coordindates.size() times.
                t = new_start + 1;
            }

            BOOST_ASSERT(!prev_unbroken_timestamps.empty());
            const std::size_t prev_unbroken_timestamp = prev_unbroken_timestamps.back();

            const auto &prev_viterbi = model.viterbi[prev_unbroken_timestamp];
            const auto &prev_pruned = model.pruned[prev_unbroken_timestamp];
            const auto &prev_unbroken_timestamps_list = candidates_list[prev_unbroken_timestamp];
            const auto &prev_coordinate = trace_coordinates[prev_unbroken_timestamp];

            auto &current_viterbi = model.viterbi[t];
            auto &current_pruned = model.pruned[t];
            auto &current_suspicious = model.suspicious[t];
            auto &current_parents = model.parents[t];
            auto &current_lengths = model.path_lengths[t];
            const auto &current_timestamps_list = candidates_list[t];
            const auto &current_coordinate = trace_coordinates[t];

            engine_working_data.InitializeOrClearFirstThreadLocalStorage(
                super::facade->GetNumberOfNodes());
            engine_working_data.InitializeOrClearSecondThreadLocalStorage(
                super::facade->GetNumberOfNodes());

            QueryHeap &forward_heap = *(engine_working_data.forward_heap_1);
            QueryHeap &reverse_heap = *(engine_working_data.reverse_heap_1);

            // compute d_t for this timestamp and the next one
            for (const auto s : osrm::irange<std::size_t>(0u, prev_viterbi.size()))
            {
                if (prev_pruned[s])
                {
                    continue;
                }

                for (const auto s_prime : osrm::irange<std::size_t>(0u, current_viterbi.size()))
                {
                    // how likely is candidate s_prime at time t to be emitted?
                    const double emission_pr =
                        emission_log_probability(candidates_list[t][s_prime].second);
                    double new_value = prev_viterbi[s] + emission_pr;
                    if (current_viterbi[s_prime] > new_value)
                    {
                        continue;
                    }

                    forward_heap.Clear();
                    reverse_heap.Clear();

                    // get distance diff between loc1/2 and locs/s_prime
                    const auto network_distance = super::get_network_distance(
                        forward_heap, reverse_heap, prev_unbroken_timestamps_list[s].first,
                        current_timestamps_list[s_prime].first);
                    const auto great_circle_distance =
                        coordinate_calculation::great_circle_distance(prev_coordinate,
                                                                      current_coordinate);

                    const auto d_t = std::abs(network_distance - great_circle_distance);

                    // very low probability transition -> prune
                    if (d_t > osrm::matching::MAX_DISTANCE_DELTA)
                    {
                        continue;
                    }

                    const double transition_pr = transition_log_probability(d_t);
                    new_value += transition_pr;

                    matching_debug.add_transition_info(prev_unbroken_timestamp, t, s, s_prime,
                                                       prev_viterbi[s], emission_pr, transition_pr,
                                                       network_distance, great_circle_distance);

                    if (new_value > current_viterbi[s_prime])
                    {
                        current_viterbi[s_prime] = new_value;
                        current_parents[s_prime] = std::make_pair(prev_unbroken_timestamp, s);
                        current_lengths[s_prime] = network_distance;
                        current_pruned[s_prime] = false;
                        current_suspicious[s_prime] = d_t > osrm::matching::SUSPICIOUS_DISTANCE_DELTA;
                        model.breakage[t] = false;
                    }
                }
            }

            if (model.breakage[t])
            {
                // save start of breakage -> we need this as split point
                if (t < breakage_begin)
                {
                    breakage_begin = t;
                }

                BOOST_ASSERT(prev_unbroken_timestamps.size() > 0);
                // remove both ends of the breakage
                prev_unbroken_timestamps.pop_back();
            }
            else
            {
                prev_unbroken_timestamps.push_back(t);
            }
        }

        matching_debug.set_viterbi(model.viterbi, model.pruned, model.suspicious);

        if (!prev_unbroken_timestamps.empty())
        {
            split_points.push_back(prev_unbroken_timestamps.back() + 1);
        }

        std::size_t sub_matching_begin = initial_timestamp;
        for (const auto sub_matching_end : split_points)
        {
            osrm::matching::SubMatching matching;

            // find real end of trace
            // not sure if this is really needed
            std::size_t parent_timestamp_index = sub_matching_end - 1;
            while (parent_timestamp_index >= sub_matching_begin &&
                   model.breakage[parent_timestamp_index])
            {
                --parent_timestamp_index;
            }

            // matchings that only consist of one candidate are invalid
            if (parent_timestamp_index - sub_matching_begin + 1 < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            // loop through the columns, and only compare the last entry
            const auto max_element_iter =
                std::max_element(model.viterbi[parent_timestamp_index].begin(),
                                 model.viterbi[parent_timestamp_index].end());

            std::size_t parent_candidate_index =
                std::distance(model.viterbi[parent_timestamp_index].begin(), max_element_iter);

            std::deque<std::pair<std::size_t, std::size_t>> reconstructed_indices;
            while (parent_timestamp_index > sub_matching_begin)
            {
                if (model.breakage[parent_timestamp_index])
                {
                    continue;
                }

                reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
                const auto &next = model.parents[parent_timestamp_index][parent_candidate_index];
                parent_timestamp_index = next.first;
                parent_candidate_index = next.second;
            }
            reconstructed_indices.emplace_front(parent_timestamp_index, parent_candidate_index);
            if (reconstructed_indices.size() < 2)
            {
                sub_matching_begin = sub_matching_end;
                continue;
            }

            matching.length = 0.0f;
            matching.nodes.resize(reconstructed_indices.size());
            matching.indices.resize(reconstructed_indices.size());
            for (const auto i : osrm::irange<std::size_t>(0u, reconstructed_indices.size()))
            {
                const auto timestamp_index = reconstructed_indices[i].first;
                const auto location_index = reconstructed_indices[i].second;

                matching.indices[i] = timestamp_index;
                matching.nodes[i] = candidates_list[timestamp_index][location_index].first;
                matching.length += model.path_lengths[timestamp_index][location_index];

                matching_debug.add_chosen(timestamp_index, location_index);
            }

            sub_matchings.push_back(matching);
            sub_matching_begin = sub_matching_end;
        }
        matching_debug.add_breakage(model.breakage);
    }
};

//[1] "Hidden Markov Map Matching Through Noise and Sparseness"; P. Newson and J. Krumm; 2009; ACM
// GIS

#endif /* MAP_MATCHING_HPP */
