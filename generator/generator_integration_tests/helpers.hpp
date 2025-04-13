#pragma once

#include <string>

#include "base/exception.hpp"

namespace generator_integration_tests
{
DECLARE_EXCEPTION(MkDirFailure, RootException);

void DecompressZipArchive(std::string const & src, std::string const & dst);
}  // namespace generator_integration_tests
