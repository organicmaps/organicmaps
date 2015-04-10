#pragma once

#include "std/vector.hpp"
#include "std/string.hpp"


namespace downloader
{
  class HttpRequest;

  void GetServerListFromRequest(HttpRequest const & request, vector<string> & urls);
}
