/*

Copyright (c) 2014, Project OSRM contributors
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

#ifndef SEARCH_ENGINE_HPP
#define SEARCH_ENGINE_HPP

#include "search_engine_data.hpp"
#include "../routing_algorithms/alternative_path.hpp"
#include "../routing_algorithms/many_to_many.hpp"
#include "../routing_algorithms/map_matching.hpp"
#include "../routing_algorithms/shortest_path.hpp"

#include <type_traits>

template <class DataFacadeT> class SearchEngine
{
  private:
    DataFacadeT *facade;
    SearchEngineData engine_working_data;

  public:
    ShortestPathRouting<DataFacadeT> shortest_path;
    AlternativeRouting<DataFacadeT> alternative_path;
    ManyToManyRouting<DataFacadeT> distance_table;
    MapMatching<DataFacadeT> map_matching;

    explicit SearchEngine(DataFacadeT *facade)
        : facade(facade),
          shortest_path(facade, engine_working_data),
          alternative_path(facade, engine_working_data),
          distance_table(facade, engine_working_data),
          map_matching(facade, engine_working_data)
    {
        static_assert(!std::is_pointer<DataFacadeT>::value, "don't instantiate with ptr type");
        static_assert(std::is_object<DataFacadeT>::value,
                      "don't instantiate with void, function, or reference");
    }

    ~SearchEngine() {}
};

#endif // SEARCH_ENGINE_HPP
