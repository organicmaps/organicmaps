#pragma once

#include "../../../../../std/stdint.hpp"
#include "../../../../../std/string.hpp"
#include "../../../../../platform/http_thread_callback.hpp"
#include <jni.h>

class HttpThread
{
private:

  jobject m_self;

public:

  HttpThread(string const & url,
             downloader::IHttpThreadCallback & cb,
             int64_t beg,
             int64_t end,
             int64_t size,
             string const & pb);
  ~HttpThread();
};
