#pragma once

#include "generate_info.hpp"

#include "../std/string.hpp"


bool GenerateFeatures(feature::GenerateInfo & info, string const & nodeStorage, string const &osmFileType, string const & osmFileName);
bool GenerateIntermediateData(string const & dir, string const & nodeStorage, string const &osmFileType, string const & osmFileName);


