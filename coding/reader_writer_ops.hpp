#pragma once

#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace rw_ops
{
/// Do reverse bytes.
/// Note! src and dest should be for different entities.
void Reverse(Reader const & src, Writer & dest);
}  // namespace rw_ops
