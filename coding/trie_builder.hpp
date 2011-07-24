#pragma once
#include "../coding/varint.hpp"
#include "../base/string_utils.hpp"
#include "../std/algorithm.hpp"

namespace trie
{
namespace builder
{

template <typename SinkT, typename ValueWriter, typename ValueIterT>
void WriteLeaf(SinkT & sink, ValueWriter valueWriter, ValueIterT begValue, ValueIterT endValue)
{
  for (ValueIterT it = begValue; it != endValue; ++it)
    valueWriter.Write(sink, *it);
}

template <typename SinkT, typename ValueWriter, typename ValueIterT, typename ChildIterT>
void WriteNode(SinkT & sink, ValueWriter valueWriter, strings::UniChar baseChar,
               ValueIterT begValue, ValueIterT endValue,
               ChildIterT begChild, ChildIterT endChild)
{
  uint32_t const valueCount = endValue - begValue;
  uint32_t const childCount = endChild - begChild;
  uint8_t const header = static_cast<uint32_t>((min(valueCount, 3U) << 6) + min(childCount, 63U));
  sink.Write(&header, 1);
  if (valueCount >= 3)
    WriteVarUint(sink, valueCount);
  if (childCount >= 63)
    WriteVarUint(sink, childCount);
  for (ValueIterT it = begValue; it != endValue; ++it)
    valueWriter.Write(sink, *it);
  for (ChildIterT it = begChild; it != endChild; ++it)
  {
    WriteVarUint(sink, it->Size());
    uint8_t header = (it->IsLeaf() ? 128 : 0);
    strings::UniString const edge = it->GetEdge();
    CHECK(!edge.empty(), ());
    uint32_t const diff0 = bits::ZigZagEncode(int32_t(edge[0] - baseChar));
    if (edge.size() == 1 && (diff0 & ~63U) == 0)
    {
        header |= 64;
        header |= diff0;
        WriteToSink(sink, header);
    }
    else
    {
      if (edge.size() < 63)
      {
        header |= edge.size();
        WriteToSink(sink, header);
      }
      else
      {
        header |= 63;
        WriteToSink(sink, header);
        WriteVarUint(sink, static_cast<uint32_t>(edge.size()));
      }
      for (size_t i = 0; i < edge.size(); ++i)
      {
        WriteVarInt(sink, int32_t(edge[i] - baseChar));
        baseChar = edge[i];
      }
    }
    baseChar = edge[0];
  }
}


}  // namespace builder
}  // namespace trie
