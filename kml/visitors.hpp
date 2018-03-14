#pragma once

#include "kml/types.hpp"

#include "coding/point_to_integer.hpp"
#include "coding/text_storage.hpp"
#include "coding/varint.hpp"

#include "base/bits.hpp"

#include <algorithm>
#include <vector>

namespace kml
{
template <typename Collector>
class CollectorVisitor
{
public:
  explicit CollectorVisitor(Collector & collector, bool clearIndex = false)
    : m_collector(collector)
    , m_clearIndex(clearIndex)
  {}

  template <typename T>
  void PerformAction(T & t)
  {
    if (m_clearIndex)
      t.ClearCollectionIndex();
    else
      t.Collect(m_collector);
  }

  void operator()(CategoryData & t, char const * /* name */ = nullptr)
  {
    PerformAction(t);
    t.Visit(*this);
  }

  void operator()(BookmarkData & t, char const * /* name */ = nullptr)
  {
    PerformAction(t);
    t.Visit(*this);
  }

  void operator()(TrackData & t, char const * /* name */ = nullptr)
  {
    PerformAction(t);
    t.Visit(*this);
  }

  template <typename T>
  void operator()(T & t, char const * /* name */ = nullptr) {}

  template <typename T>
  void operator()(std::vector<T> & vs, char const * /* name */ = nullptr)
  {
    for (auto & v : vs)
      (*this)(v);
  }

private:
  Collector & m_collector;
  bool const m_clearIndex;
};

class LocalizableStringCollector
{
public:
  explicit LocalizableStringCollector(size_t reservedCollectionSize)
  {
    m_collection.reserve(reservedCollectionSize + 1);
    m_collection.emplace_back(std::string());
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, LocalizableString const & str,
               OtherStrings const & ... args)
  {
    index.emplace_back(LocalizableStringSubIndex());
    for (auto const & p : str)
      CollectString(index.back(), p.first, p.second);

    Collect(index, args...);
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, std::string const & str,
               OtherStrings const & ... args)
  {
    int8_t constexpr kFakeIndex = 0;
    index.emplace_back(LocalizableStringSubIndex());
    CollectString(index.back(), kFakeIndex, str);

    Collect(index, args...);
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index,
               std::vector<std::string> const & stringsArray,
               OtherStrings const & ... args)
  {
    index.emplace_back(LocalizableStringSubIndex());
    auto constexpr kMaxSize = static_cast<size_t>(std::numeric_limits<int8_t>::max());
    auto const sz = std::min(stringsArray.size(), kMaxSize);
    for (size_t i = 0; i < sz; ++i)
      CollectString(index.back(), static_cast<int8_t>(i), stringsArray[i]);

    Collect(index, args...);
  }

  template <typename...>
  void Collect(LocalizableStringIndex & index) {}

  std::vector<std::string> && StealCollection() { return std::move(m_collection); }

private:
  void CollectString(LocalizableStringSubIndex & subIndex, int8_t code,
                     std::string const & str)
  {
    if (str.empty())
    {
      subIndex.insert(std::make_pair(code, kEmptyStringId));
    }
    else
    {
      subIndex.insert(std::make_pair(code, m_counter));
      m_counter++;
      m_collection.push_back(str);
    }
  }

  uint32_t m_counter = kEmptyStringId + 1;
  std::vector<std::string> m_collection;
};

namespace binary
{
template <typename Sink>
inline void WriteLocalizableStringIndex(Sink & sink, LocalizableStringIndex const & index)
{
  WriteVarUint(sink, static_cast<uint32_t>(index.size()));
  for (auto const & subIndex : index)
  {
    WriteVarUint(sink, static_cast<uint32_t>(subIndex.size()));
    for (auto const & p : subIndex)
    {
      WriteToSink(sink, p.first);
      WriteVarUint(sink, p.second);
    }
  }
}

template <typename Sink>
class CategorySerializerVisitor
{
public:
  explicit CategorySerializerVisitor(Sink & sink)
    : m_sink(sink)
  {}

  void operator()(LocalizableStringIndex const & index, char const * /* name */ = nullptr)
  {
    WriteLocalizableStringIndex(m_sink, index);
  }

  void operator()(bool b, char const * /* name */ = nullptr)
  {
    (*this)(static_cast<uint8_t>(b));
  }

  void operator()(AccessRules rules, char const * /* name */ = nullptr)
  {
    (*this)(static_cast<uint8_t>(rules));
  }

  void operator()(Timestamp const & t, char const * name = nullptr)
  {
    WriteVarUint(m_sink, ToSecondsSinceEpoch(t));
  }

  template <typename T>
  void operator()(std::vector<T> const & vs, char const * /* name */ = nullptr)
  {
    WriteVarUint(m_sink, static_cast<uint32_t>(vs.size()));
    for (auto const & v : vs)
      (*this)(v);
  }

  template <typename D>
  std::enable_if_t<std::is_integral<D>::value>
  operator()(D d, char const * /* name */ = nullptr)
  {
    WriteToSink(m_sink, d);
  }

  template <typename R>
  std::enable_if_t<!std::is_integral<R>::value>
  operator()(R const & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  // Skip visiting. It is stored in the separate sections.
  SKIP_VISITING(LocalizableString const &)
  SKIP_VISITING(std::string const &)
  SKIP_VISITING(std::vector<std::string> const &)
  SKIP_VISITING(std::vector<BookmarkData> const &)
  SKIP_VISITING(std::vector<TrackData> const &)

private:
  Sink & m_sink;
};

template <typename Sink>
class BookmarkSerializerVisitor
{
public:
  explicit BookmarkSerializerVisitor(Sink & sink)
    : m_sink(sink)
  {}

  void operator()(LocalizableStringIndex const & index, char const * /* name */ = nullptr)
  {
    WriteLocalizableStringIndex(m_sink, index);
  }

  void operator()(bool b, char const * /* name */ = nullptr)
  {
    (*this)(static_cast<uint8_t>(b));
  }

  void operator()(m2::PointD const & pt, char const * /* name */ = nullptr)
  {
    uint64_t const encoded = bits::ZigZagEncode(PointToInt64(pt, POINT_COORD_BITS));
    WriteVarUint(m_sink, encoded);
  }

  void operator()(double d, char const * /* name */ = nullptr)
  {
    uint64_t const encoded = DoubleToUint32(d, kMinLineWidth, kMaxLineWidth, 30 /* coordBits */);
    WriteVarUint(m_sink, encoded);
  }

  void operator()(Timestamp const & t, char const * name = nullptr)
  {
    WriteVarUint(m_sink, ToSecondsSinceEpoch(t));
  }

  void operator()(PredefinedColor color, char const * /* name */ = nullptr)
  {
    (*this)(static_cast<uint8_t>(color));
  }

  void operator()(BookmarkIcon icon, char const * /* name */ = nullptr)
  {
    (*this)(static_cast<uint16_t>(icon));
  }

  template <typename T>
  void operator()(std::vector<T> const & vs, char const * /* name */ = nullptr)
  {
    WriteVarUint(m_sink, static_cast<uint32_t>(vs.size()));
    for (auto const & v : vs)
      (*this)(v);
  }

  template <typename D>
  std::enable_if_t<std::is_integral<D>::value>
  operator()(D d, char const * /* name */ = nullptr)
  {
    WriteToSink(m_sink, d);
  }

  template <typename R>
  std::enable_if_t<!std::is_integral<R>::value>
  operator()(R const & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  // Skip visiting. It is stored in the separate sections.
  SKIP_VISITING(LocalizableString const &)
  SKIP_VISITING(std::string const &)
  SKIP_VISITING(std::vector<std::string> const &)

private:
  Sink & m_sink;
};

template <typename Source>
inline void ReadLocalizableStringIndex(Source & source, LocalizableStringIndex & index)
{
  auto const indexSize = ReadVarUint<uint32_t, Source>(source);
  index.reserve(indexSize);
  for (uint32_t i = 0; i < indexSize; ++i)
  {
    index.emplace_back(LocalizableStringSubIndex());
    auto const subIndexSize = ReadVarUint<uint32_t, Source>(source);
    for (uint32_t j = 0; j < subIndexSize; ++j)
    {
      auto const lang = ReadPrimitiveFromSource<int8_t>(source);
      auto const strIndex = ReadVarUint<uint32_t, Source>(source);
      index.back()[lang] = strIndex;
    }
  }
}

template <typename Source>
class CategoryDeserializerVisitor
{
public:
  explicit CategoryDeserializerVisitor(Source & source)
    : m_source(source)
  {}

  void operator()(LocalizableStringIndex & index, char const * /* name */ = nullptr)
  {
    ReadLocalizableStringIndex(m_source, index);
  }

  void operator()(bool & b, char const * /* name */ = nullptr)
  {
    b = static_cast<bool>(ReadPrimitiveFromSource<uint8_t>(m_source));
  }

  void operator()(AccessRules & rules, char const * /* name */ = nullptr)
  {
    rules = static_cast<AccessRules>(ReadPrimitiveFromSource<uint8_t>(m_source));
  }

  void operator()(Timestamp & t, char const * /* name */ = nullptr)
  {
    auto const v = ReadVarUint<uint64_t, Source>(m_source);
    t = FromSecondsSinceEpoch(v);
  }

  template <typename T>
  void operator()(std::vector<T> & vs, char const * /* name */ = nullptr)
  {
    auto const sz = ReadVarUint<uint32_t, Source>(m_source);
    vs.reserve(sz);
    for (uint32_t i = 0; i < sz; ++i)
    {
      vs.emplace_back(T());
      (*this)(vs.back());
    }
  }

  template <typename D>
  std::enable_if_t<std::is_integral<D>::value>
  operator()(D & d, char const * /* name */ = nullptr)
  {
    d = ReadPrimitiveFromSource<D>(m_source);
  }

  template <typename R>
  std::enable_if_t<!std::is_integral<R>::value>
  operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  // Skip visiting. It is stored in the separate sections.
  SKIP_VISITING(LocalizableString &)
  SKIP_VISITING(std::string &)
  SKIP_VISITING(std::vector<std::string> &)
  SKIP_VISITING(std::vector<BookmarkData> &)
  SKIP_VISITING(std::vector<TrackData> &)

private:
  Source & m_source;
};

template <typename Source>
class BookmarkDeserializerVisitor
{
public:
  explicit BookmarkDeserializerVisitor(Source & source)
    : m_source(source)
  {}

  void operator()(LocalizableStringIndex & index, char const * /* name */ = nullptr)
  {
    ReadLocalizableStringIndex(m_source, index);
  }

  void operator()(bool & b, char const * /* name */ = nullptr)
  {
    b = static_cast<bool>(ReadPrimitiveFromSource<uint8_t>(m_source));
  }

  void operator()(m2::PointD & pt, char const * /* name */ = nullptr)
  {
    auto const v = ReadVarUint<uint64_t, Source>(m_source);
    pt = Int64ToPoint(bits::ZigZagDecode(v), POINT_COORD_BITS);
  }

  void operator()(double & d, char const * /* name */ = nullptr)
  {
    auto const v = ReadVarUint<uint32_t, Source>(m_source);
    d = Uint32ToDouble(v, kMinLineWidth, kMaxLineWidth, 30 /* coordBits */);
  }

  void operator()(Timestamp & t, char const * /* name */ = nullptr)
  {
    auto const v = ReadVarUint<uint64_t, Source>(m_source);
    t = FromSecondsSinceEpoch(v);
  }

  void operator()(PredefinedColor & color, char const * /* name */ = nullptr)
  {
    color = static_cast<PredefinedColor>(ReadPrimitiveFromSource<uint8_t>(m_source));
  }

  void operator()(AccessRules & rules, char const * /* name */ = nullptr)
  {
    rules = static_cast<AccessRules>(ReadPrimitiveFromSource<uint8_t>(m_source));
  }

  void operator()(BookmarkIcon & icon, char const * /* name */ = nullptr)
  {
    icon = static_cast<BookmarkIcon>(ReadPrimitiveFromSource<uint16_t>(m_source));
  }

  template <typename T>
  void operator()(std::vector<T> & vs, char const * /* name */ = nullptr)
  {
    auto const sz = ReadVarUint<uint32_t, Source>(m_source);
    vs.reserve(sz);
    for (uint32_t i = 0; i < sz; ++i)
    {
      vs.emplace_back(T());
      (*this)(vs.back());
    }
  }

  template <typename D>
  std::enable_if_t<std::is_integral<D>::value>
  operator()(D & d, char const * /* name */ = nullptr)
  {
    d = ReadPrimitiveFromSource<D>(m_source);
  }

  template <typename R>
  std::enable_if_t<!std::is_integral<R>::value>
  operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  // Skip visiting. It is stored in the separate sections.
  SKIP_VISITING(LocalizableString &)
  SKIP_VISITING(std::string &)
  SKIP_VISITING(std::vector<std::string> &)

private:
  Source & m_source;
};

template <typename Reader>
class DeserializedStringCollector
{
public:
  explicit DeserializedStringCollector(coding::BlockedTextStorage<Reader> & textStorage)
    : m_textStorage(textStorage)
  {}

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, LocalizableString & str,
               OtherStrings & ... args)
  {
    if (!SwitchSubIndexIfNeeded(index))
      return;

    auto subIndex = index[m_counter];
    for (auto const & p : subIndex)
      str[p.first] = ExtractString(subIndex, p.second);

    m_counter++;
    Collect(index, args...);
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, std::string & str,
               OtherStrings & ... args)
  {
    if (!SwitchSubIndexIfNeeded(index))
      return;

    auto subIndex = index[m_counter];
    if (!subIndex.empty())
      str = ExtractString(subIndex, subIndex.begin()->second);
    else
      str = {};

    m_counter++;
    Collect(index, args...);
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, std::vector<std::string> & stringsArray,
               OtherStrings & ... args)
  {
    if (!SwitchSubIndexIfNeeded(index))
      return;

    auto subIndex = index[m_counter];
    stringsArray.reserve(subIndex.size());
    for (auto const & p : subIndex)
      stringsArray.emplace_back(ExtractString(subIndex, p.second));

    m_counter++;
    Collect(index, args...);
  }

  template <typename...>
  void Collect(LocalizableStringIndex & index) {}

private:
  bool SwitchSubIndexIfNeeded(LocalizableStringIndex & index)
  {
    if (m_lastIndex != &index)
    {
      m_counter = 0;
      m_lastIndex = &index;
    }
    return m_counter < index.size();
  }

  std::string ExtractString(LocalizableStringSubIndex const & subIndex,
                            uint32_t stringIndex) const
  {
    auto const stringsCount = m_textStorage.GetNumStrings();
    if (stringIndex >= stringsCount)
      return {};

    return m_textStorage.ExtractString(stringIndex);
  }

  coding::BlockedTextStorage<Reader> & m_textStorage;
  LocalizableStringIndex * m_lastIndex = nullptr;
  size_t m_counter = 0;
};
}  // namespace binary
}  // namespace kml
