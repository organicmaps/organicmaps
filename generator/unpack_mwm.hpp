#pragma once

#include "base/base.hpp"

#include <string>


// Unpack each section of mwm into a separate file with name filePath.sectionName
void UnpackMwm(std::string const & filePath);

void DeleteSection(std::string const & filePath, std::string const & tag);
