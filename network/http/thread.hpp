#pragma once

#include <cstdint>
#include <string>

#include "network/http/thread_callback.hpp"

namespace om::network::http
{
class Thread;

namespace thread
{
Thread * CreateThread(std::string const & url, IThreadCallback & cb, int64_t begRange = 0, int64_t endRange = -1,
                      int64_t expectedSize = -1, std::string const & postBody = {});

void DeleteThread(Thread * thread);
}  // namespace thread
}  // namespace om::network::http
