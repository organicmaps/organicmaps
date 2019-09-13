#pragma once

#include <string>
#include <vector>

namespace downloader
{
class HttpRequest;

void GetServersList(std::string const & src, std::vector<std::string> & urls);
void GetServersList(HttpRequest const & request, std::vector<std::string> & urls);
}  // namespace downloader
