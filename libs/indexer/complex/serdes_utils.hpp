#pragma once

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

namespace coding_utils
{
// Type of collection size. Used for reading and writing collections.
using CollectionSizeType = uint64_t;

// WriteCollectionPrimitive writes collection. It uses WriteToSink function.
template <typename Sink, typename Cont>
void WriteCollectionPrimitive(Sink & sink, Cont const & container)
{
  auto const contSize = base::checked_cast<CollectionSizeType>(container.size());
  WriteVarUint(sink, contSize);
  for (auto value : container)
    WriteToSink(sink, value);
}

// ReadCollectionPrimitive reads collection. It uses ReadPrimitiveFromSource function.
template <typename Source, typename OutIt>
void ReadCollectionPrimitive(Source & src, OutIt it)
{
  using ValueType = typename OutIt::container_type::value_type;

  auto size = ReadVarUint<CollectionSizeType>(src);
  while (size--)
    *it++ = ReadPrimitiveFromSource<ValueType>(src);
}
}  // namespace coding_utils
