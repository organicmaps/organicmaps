#pragma once

#include <string>

namespace classificator
{
void Load();

// This method loads only classificator and types. It does not load and apply
// style rules. It can be used in separate modules to operate with
// number-string representations of types.
void LoadTypes(std::string const & classificatorFileStr, std::string const & typesFileStr);
}  // namespace classificator
