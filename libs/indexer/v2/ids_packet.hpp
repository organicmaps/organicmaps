#pragma once

#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>

namespace idx::v2
{
// A sorted list of unique ids as one packet. Its control byte has the kind in the low kTagBits and an
// inline value above:
// - Reference: a caller's link to an equal list; its low bits are inline, varuint(link >> kInlineBits).
// - Singleton: one id, stored like a reference, or like a count if it is known to be small.
// - Delta: inline count - 2, then varuint gaps between the sorted ids (the first gap is from zero).
// - Bitmap: the first id stored like a reference, varuint(words), then 64-bit words; bit 0 is the first id.
// - Runs: inline count of runs, then varuint (gap from the previous run's end, length) pairs.
// An inline count of kInlineMax is followed by a varuint with the rest.
// The writer takes the smallest of Delta, Bitmap and Runs: most lists are short, so the header is
// one byte, and long clustered lists become runs or bitmaps.
// The caller may store a code in place of the first id, e.g. its distance to an id the reader knows.
class IdsPacket
{
public:
  using Id = uint32_t;

  // The layout of a singleton: Split takes fewer bytes for a big value, Count takes one byte for
  // a value below kInlineMax. The reader has to know the choice of the writer.
  enum class Singleton : uint8_t
  {
    Split,
    Count
  };

  template <class Sink>
  static void WriteReference(Sink & sink, uint32_t link)
  {
    WriteSplit(sink, Kind::Reference, link);
  }

  // Writes the non-empty sorted unique |ids| with |first| in place of ids[0].
  template <class Sink>
  static void Write(Sink & sink, std::span<Id const> ids, uint64_t first, Singleton singleton = Singleton::Split)
  {
    ASSERT(!ids.empty(), ());
    ASSERT(std::adjacent_find(ids.begin(), ids.end(), std::greater_equal<>()) == ids.end(), ());
    if (ids.size() == 1)
    {
      if (singleton == Singleton::Split)
        WriteSplit(sink, Kind::Singleton, first);
      else
        WriteCount(sink, Kind::Singleton, first);
      return;
    }

    uint64_t const count = ids.size();
    uint64_t deltaSize = CountSize(count - 2) + VarUintSize(first);
    for (size_t i = 1; i < ids.size(); ++i)
      deltaSize += VarUintSize(ids[i] - ids[i - 1]);

    uint64_t const words = (ids.back() - ids.front()) / kWordBits + 1;
    uint64_t const bitmapSize = 1 + VarUintSize(first >> kInlineBits) + VarUintSize(words) + words * sizeof(uint64_t);

    uint64_t runs = 0;
    uint64_t runsSize = 0;
    ForEachRun(ids, [&](uint64_t gap, uint64_t length)
    {
      runsSize += VarUintSize(runs == 0 ? first : gap) + VarUintSize(length);
      ++runs;
    });
    runsSize += CountSize(runs);

    // Ties keep the Delta, Bitmap, Runs order.
    if (deltaSize <= bitmapSize && deltaSize <= runsSize)
    {
      WriteCount(sink, Kind::Delta, count - 2);
      WriteVarUint(sink, first);
      for (size_t i = 1; i < ids.size(); ++i)
        WriteVarUint(sink, ids[i] - ids[i - 1]);
    }
    else if (bitmapSize <= runsSize)
    {
      WriteSplit(sink, Kind::Bitmap, first);
      WriteVarUint(sink, words);
      uint64_t word = 0;
      uint64_t wordIndex = 0;
      for (auto const id : ids)
      {
        uint64_t const offset = id - ids.front();
        for (; wordIndex < offset / kWordBits; ++wordIndex)
        {
          WriteToSink(sink, word);
          word = 0;
        }
        word |= uint64_t{1} << (offset % kWordBits);
      }
      WriteToSink(sink, word);
    }
    else
    {
      WriteCount(sink, Kind::Runs, runs);
      bool isFirst = true;
      ForEachRun(ids, [&](uint64_t gap, uint64_t length)
      {
        WriteVarUint(sink, isFirst ? first : gap);
        WriteVarUint(sink, length);
        isFirst = false;
      });
    }
  }

  // Reads a packet. For a reference sets |link| and returns false. Otherwise calls toFirstId with
  // the stored first value to get the first id, then fn(id) for every id, and returns true.
  template <class Source, class ToFirstId, class Fn>
  static bool Read(Source & src, Singleton singleton, uint32_t & link, ToFirstId && toFirstId, Fn && fn)
  {
    uint8_t const control = ReadPrimitiveFromSource<uint8_t>(src);
    uint64_t const inlineValue = control >> kTagBits;
    auto const readSplit = [&] { return (ReadValue(src) << kInlineBits) | inlineValue; };
    auto const readCount = [&] { return inlineValue + (inlineValue == kInlineMax ? ReadValue(src) : 0); };

    switch (static_cast<Kind>(control & kTagMask))
    {
    case Kind::Reference: link = static_cast<uint32_t>(readSplit()); return false;
    case Kind::Singleton: fn(toFirstId(singleton == Singleton::Split ? readSplit() : readCount())); break;
    case Kind::Delta:
    {
      uint64_t const count = readCount() + 2;
      Id id = toFirstId(ReadValue(src));
      fn(id);
      for (uint64_t i = 1; i < count; ++i)
      {
        id += ReadVarUint<uint32_t>(src);
        fn(id);
      }
      break;
    }
    case Kind::Bitmap:
    {
      uint64_t base = toFirstId(readSplit());
      for (uint64_t i = 0, words = ReadValue(src); i < words; ++i, base += kWordBits)
        for (auto word = ReadPrimitiveFromSource<uint64_t>(src); word != 0; word &= word - 1)
          fn(static_cast<Id>(base + std::countr_zero(word)));
      break;
    }
    case Kind::Runs:
    {
      uint64_t const count = readCount();
      uint64_t id = toFirstId(ReadValue(src));
      for (uint64_t i = 0; i < count; ++i)
      {
        if (i != 0)
          id += ReadVarUint<uint32_t>(src);
        for (uint64_t const end = id + ReadVarUint<uint32_t>(src); id < end; ++id)
          fn(static_cast<Id>(id));
      }
      break;
    }
    default: ASSERT(false, ("Unknown ids packet", control));
    }
    return true;
  }

  // Skips a packet of ids and returns its stored first value.
  template <class Source>
  static uint64_t Skip(Source & src, Singleton singleton)
  {
    uint8_t const control = ReadPrimitiveFromSource<uint8_t>(src);
    uint64_t const inlineValue = control >> kTagBits;
    auto const readSplit = [&] { return (ReadValue(src) << kInlineBits) | inlineValue; };
    auto const readCount = [&] { return inlineValue + (inlineValue == kInlineMax ? ReadValue(src) : 0); };
    auto const skipVarUints = [&src](uint64_t count) { SkipVarUints(src, count); };

    switch (static_cast<Kind>(control & kTagMask))
    {
    case Kind::Singleton: return singleton == Singleton::Split ? readSplit() : readCount();
    case Kind::Delta:
    {
      uint64_t const count = readCount() + 2;
      uint64_t const first = ReadValue(src);
      skipVarUints(count - 1);
      return first;
    }
    case Kind::Bitmap:
    {
      uint64_t const first = readSplit();
      size_t const size = static_cast<size_t>(ReadValue(src)) * sizeof(uint64_t);
      // The words are not read: a source of memory moves its pointer, a source of a reader its position.
      if constexpr (requires { src.PtrUint8(); })
        src.Advance(size);
      else
        src.Skip(size);
      return first;
    }
    case Kind::Runs:
    {
      uint64_t const count = readCount();
      uint64_t const first = ReadValue(src);
      skipVarUints(2 * count - 1);
      return first;
    }
    default: ASSERT(false, ("Not a packet of ids", control));
    }
    return 0;
  }

private:
  enum class Kind : uint8_t
  {
    Reference = 0,
    Singleton = 1,
    Delta = 2,
    Bitmap = 3,
    Runs = 4
  };
  static constexpr uint8_t kTagBits = 3;
  static constexpr uint8_t kTagMask = (1 << kTagBits) - 1;
  static constexpr uint8_t kInlineBits = 8 - kTagBits;
  static constexpr uint64_t kInlineMax = (1 << kInlineBits) - 1;
  static constexpr uint64_t kWordBits = 64;

  static uint64_t VarUintSize(uint64_t value)
  {
    uint64_t size = 1;
    for (; value >= 128; value >>= 7)
      ++size;
    return size;
  }

  static uint64_t CountSize(uint64_t count) { return 1 + (count >= kInlineMax ? VarUintSize(count - kInlineMax) : 0); }

  // Reads a varuint of WriteVarUint() by a short loop, which is inlined: a packet has a few such values
  // before its ids, and the unrolled ReadVarUint<uint64_t>() makes the lookup of all the features of
  // a geometry index 7-8% slower.
  template <class Source>
  static uint64_t ReadValue(Source & src)
  {
    uint64_t value = 0;
    for (uint8_t shift = 0;; shift += 7)
    {
      uint8_t const byte = ReadPrimitiveFromSource<uint8_t>(src);
      value |= uint64_t{byte & 0x7FU} << shift;
      if ((byte & 0x80) == 0)
        return value;
    }
  }

  // A varuint ends with the byte without the high bit.
  template <class Source>
  static void SkipVarUints(Source & src, uint64_t count)
  {
    if constexpr (requires { src.PtrUint8(); })
    {
      // Eight varuints take at least a word, which has at most eight ends.
      uint8_t const * p = src.PtrUint8();
      uint8_t const * const start = p;
      for (; count >= sizeof(uint64_t); p += sizeof(uint64_t))
      {
        uint64_t word;
        memcpy(&word, p, sizeof(word));
        count -= std::popcount(~word & 0x8080808080808080ULL);
      }
      src.Advance(p - start);
    }
    for (; count > 0; --count)
      while (ReadPrimitiveFromSource<uint8_t>(src) & 0x80)
      {}
  }

  template <class Sink>
  static void WriteControl(Sink & sink, Kind kind, uint64_t inlineValue)
  {
    ASSERT_LESS_OR_EQUAL(inlineValue, kInlineMax, ());
    WriteToSink(sink, static_cast<uint8_t>((inlineValue << kTagBits) | static_cast<uint8_t>(kind)));
  }

  // Writes a value with its low bits in the control byte.
  template <class Sink>
  static void WriteSplit(Sink & sink, Kind kind, uint64_t value)
  {
    WriteControl(sink, kind, value & kInlineMax);
    WriteVarUint(sink, value >> kInlineBits);
  }

  // Writes a count that continues in a varuint when it does not fit the control byte.
  template <class Sink>
  static void WriteCount(Sink & sink, Kind kind, uint64_t count)
  {
    WriteControl(sink, kind, std::min(count, kInlineMax));
    if (count >= kInlineMax)
      WriteVarUint(sink, count - kInlineMax);
  }

  // Calls fn(gap from the previous run's end, length) for every run of consecutive ids.
  template <class Fn>
  static void ForEachRun(std::span<Id const> ids, Fn && fn)
  {
    uint64_t start = ids.front();
    uint64_t length = 1;
    uint64_t previousEnd = 0;
    for (size_t i = 1; i < ids.size(); ++i)
    {
      if (ids[i] == start + length)
      {
        ++length;
        continue;
      }
      fn(start - previousEnd, length);
      previousEnd = start + length;
      start = ids[i];
      length = 1;
    }
    fn(start - previousEnd, length);
  }
};
}  // namespace idx::v2
