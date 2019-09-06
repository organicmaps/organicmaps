#pragma once

#include "std/vector.hpp"
#include "std/string.hpp"

namespace downloader
{
class HttpRequest;

void GetServerList(string const & src, vector<string> & urls);
void GetServerList(HttpRequest const & request, vector<string> & urls);
}  // namespace downloader
