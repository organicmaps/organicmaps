#pragma once

#include <string>
#include <vector>

namespace downloader
{
void GetServersList(std::string const & src, std::vector<std::string> & urls);
}  // namespace downloader
