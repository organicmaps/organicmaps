#include "http_thread_android.hpp"

#include "../../../../../platform/http_thread_callback.hpp"

#include "../../../../../std/string.hpp"

// @TODO empty stub, add android implementation

namespace downloader
{
HttpThread * CreateNativeHttpThread(string const & url,
                                    downloader::IHttpThreadCallback & cb,
                                    int64_t beg,
                                    int64_t end,
                                    int64_t size,
                                    string const & pb)
{
  return 0;
}

void DeleteNativeHttpThread(HttpThread * request)
{
}

} // namespace downloader
