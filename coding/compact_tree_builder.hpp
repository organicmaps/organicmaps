#pragma once
#include "writer.hpp"
#include "bit_vector_builder.hpp"
#include "../base/base.hpp"
#include "../base/start_mem_debug.hpp"
#include "../std/iterator.hpp"

template <typename TSink, typename TIter>
void BuildCompactTree(TSink & sink,
                      TIter const isParentBeg, TIter const isParentEnd,
                      TIter const isFirstChildBeg, TIter const isFirstChildEnd)
{
  size_t const size = distance(isParentBeg, isParentEnd);
  BuildMMBitVector<uint32_t>(sink, isParentBeg, isParentEnd, true, size);
  BuildMMBitVector32RankDirectory(sink, isParentBeg, isParentEnd);
  BuildMMBitVector<uint32_t>(sink, isFirstChildBeg, isFirstChildEnd, false, size);
  BuildMMBitVector32RankDirectory(sink, isFirstChildBeg, isFirstChildEnd);
}

template <typename TSink, typename TIter>
void BuildCompactTreeWithData(TSink & sink,
                              TIter const isParentBeg, TIter const isParentEnd,
                              TIter const isFirstChildBeg, TIter const isFirstChildEnd,
                              TIter const parentHasDataBeg, TIter const parentHasDataEnd)
{
  BuildCompactTree(sink, isParentBeg, isParentEnd, isFirstChildBeg, isFirstChildEnd);
  BuildMMBitVector<uint32_t>(sink, parentHasDataBeg, parentHasDataEnd);
  BuildMMBitVector32RankDirectory(sink, parentHasDataBeg, parentHasDataEnd);
}

