#pragma once

#include "kml/types.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/text_storage.hpp"
#include "coding/varint.hpp"

#include "geometry/mercator.hpp"

#include "base/bits.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>

namespace kml
{
template <typename Collector>
class CollectorVisitor
{
  // The class checks for the existence of collection methods.
  template <typename T>
  class HasCollectionMethods
  {
    template <typename C> static char Test(decltype(&C::ClearCollectionIndex));
    template <typename C> static int Test(...);
  public:
    enum {value = sizeof(Test<T>(0)) == sizeof(char)};
  };

  // All types which will be visited to collect.
  template <typename T>
  class VisitedTypes
  {
  public:
    enum {value = std::is_same<T, FileData>::value ||
                  std::is_same<T, CategoryData>::value ||
                  std::is_same<T, BookmarkData>::value ||
                  std::is_same<T, TrackData>::value};
  };

public:
  explicit CollectorVisitor(Collector & collector, bool clearIndex = false)
    : m_collector(collector)
    , m_clearIndex(clearIndex)
  {}

  template <typename T>
  std::enable_if_t<HasCollectionMethods<T>::value>
  PerformActionIfPossible(T & t)
  {
    if (m_clearIndex)
      t.ClearCollectionIndex();
    else
      t.Collect(m_collector);
  }

  template <typename T>
  std::enable_if_t<!HasCollectionMethods<T>::value> PerformActionIfPossible(T & t) {}

  template <typename T>
  std::enable_if_t<VisitedTypes<T>::value>
  VisitIfPossible(T & t)
  {
    t.Visit(*this);
  }

  template <typename T>
  std::enable_if_t<!VisitedTypes<T>::value> VisitIfPossible(T & t) {}

  template <typename T>
  void operator()(T & t, char const * /* name */ = nullptr)
  {
    PerformActionIfPossible(t);
    VisitIfPossible(t);
  }

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

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, Properties const & properties,
               OtherStrings const & ... args)
  {
    index.emplace_back(LocalizableStringSubIndex());
    auto constexpr kMaxSize = std::numeric_limits<int8_t>::max() - 1;
    int8_t counter = 0;
    for (auto const & p : properties)
    {
      if (counter >= kMaxSize)
        break;
      CollectString(index.back(), counter++, p.first);
      CollectString(index.back(), counter++, p.second);
    }

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
void WriteLocalizableStringIndex(Sink & sink, LocalizableStringIndex const & index)
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

template <typename Source>
void ReadLocalizableStringIndex(Source & source, LocalizableStringIndex & index)
{
  auto const indexSize = ReadVarUint<uint32_t, Source>(source);
  index.reserve(indexSize);
  for (uint32_t i = 0; i < indexSize; ++i)
  {
    index.emplace_back(LocalizableStringSubIndex());
    auto & subIndex = index.back();
    auto const subIndexSize = ReadVarUint<uint32_t, Source>(source);
    for (uint32_t j = 0; j < subIndexSize; ++j)
    {
      auto const lang = ReadPrimitiveFromSource<int8_t>(source);
      auto const strIndex = ReadVarUint<uint32_t, Source>(source);
      subIndex[lang] = strIndex;
    }
  }
}

template <typename Sink>
inline void WritePointU(Sink & sink, m2::PointU const & pt)
{
  WriteVarUint(sink, pt.x);
  WriteVarUint(sink, pt.y);
}

template <typename Sink>
inline void WritePointD(Sink & sink, m2::PointD const & pt, uint8_t doubleBits)
{
  WritePointU(sink, PointDToPointU(pt, doubleBits));
}

template <typename Source>
inline m2::PointU ReadPointU(Source & source)
{
  auto x = ReadVarUint<uint32_t>(source);
  auto y = ReadVarUint<uint32_t>(source);
  return {x, y};
}

template <typename Source>
inline m2::PointD ReadPointD(Source & source, uint8_t doubleBits)
{
  return PointUToPointD(ReadPointU(source), doubleBits);
}

template <typename Sink>
class CategorySerializerVisitor
{
public:
  explicit CategorySerializerVisitor(Sink & sink, uint8_t doubleBits)
    : m_sink(sink)
    , m_doubleBits(doubleBits)
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

  void operator()(double d, char const * /* name */ = nullptr)
  {
    auto const encoded = DoubleToUint32(d, kMinRating, kMaxRating, m_doubleBits);
    WriteVarUint(m_sink, encoded);
  }

  void operator()(m2::PointD const & pt, char const * /* name */ = nullptr)
  {
    WritePointD(m_sink, pt, m_doubleBits);
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
  SKIP_VISITING(Properties const &)
  SKIP_VISITING(std::vector<BookmarkData> const &)
  SKIP_VISITING(std::vector<TrackData> const &)

private:
  Sink & m_sink;
  uint8_t const m_doubleBits;
};

template <typename Sink>
class BookmarkSerializerVisitor
{
public:
  explicit BookmarkSerializerVisitor(Sink & sink, uint8_t doubleBits)
    : m_sink(sink)
    , m_doubleBits(doubleBits)
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
    WritePointD(m_sink, pt, m_doubleBits);
  }

  void operator()(double d, char const * /* name */ = nullptr)
  {
    auto const encoded = DoubleToUint32(d, kMinLineWidth, kMaxLineWidth, m_doubleBits);
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

  void operator()(std::vector<m2::PointD> const & points, char const * /* name */ = nullptr)
  {
    WriteVarUint(m_sink, static_cast<uint32_t>(points.size()));
    m2::PointU lastUpt = m2::PointU::Zero();
    for (uint32_t i = 0; i < static_cast<uint32_t>(points.size()); ++i)
    {
      auto const upt = PointDToPointU(points[i], m_doubleBits);
      coding::EncodePointDelta(m_sink, lastUpt, upt);
      lastUpt = upt;
    }
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
  SKIP_VISITING(Properties const &)

private:
  Sink & m_sink;
  uint8_t const m_doubleBits;
};

template <typename Source>
class CategoryDeserializerVisitor
{
public:
  explicit CategoryDeserializerVisitor(Source & source, uint8_t doubleBits)
    : m_source(source)
    , m_doubleBits(doubleBits)
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

  void operator()(double & d, char const * /* name */ = nullptr)
  {
    auto const v = ReadVarUint<uint32_t, Source>(m_source);
    d = Uint32ToDouble(v, kMinRating, kMaxRating, m_doubleBits);
  }

  void operator()(m2::PointD & pt, char const * /* name */ = nullptr)
  {
    pt = ReadPointD(m_source, m_doubleBits);
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
  SKIP_VISITING(Properties &)
  SKIP_VISITING(std::vector<BookmarkData> &)
  SKIP_VISITING(std::vector<TrackData> &)

private:
  Source & m_source;
  uint8_t const m_doubleBits;
};

template <typename Source>
class BookmarkDeserializerVisitor
{
public:
  explicit BookmarkDeserializerVisitor(Source & source, uint8_t doubleBits)
    : m_source(source)
    , m_doubleBits(doubleBits)
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
    pt = ReadPointD(m_source, m_doubleBits);
  }

  void operator()(double & d, char const * /* name */ = nullptr)
  {
    auto const v = ReadVarUint<uint32_t, Source>(m_source);
    d = Uint32ToDouble(v, kMinLineWidth, kMaxLineWidth, m_doubleBits);
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

  void operator()(std::vector<m2::PointD> & points, char const * /* name */ = nullptr)
  {
    auto const sz = ReadVarUint<uint32_t, Source>(m_source);
    points.reserve(sz);
    m2::PointU lastUpt = m2::PointU::Zero();
    for (uint32_t i = 0; i < sz; ++i)
    {
      lastUpt = coding::DecodePointDelta(m_source, lastUpt);
      points.emplace_back(PointUToPointD(lastUpt, m_doubleBits));
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
  SKIP_VISITING(Properties &)

private:
  Source & m_source;
  uint8_t const m_doubleBits;
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
      str[p.first] = ExtractString(p.second);

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
      str = ExtractString(subIndex.begin()->second);
    else
      str = {};

    m_counter++;
    Collect(index, args...);
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index,
               std::vector<std::string> & stringsArray,
               OtherStrings & ... args)
  {
    if (!SwitchSubIndexIfNeeded(index))
      return;

    auto subIndex = index[m_counter];
    stringsArray.reserve(subIndex.size());
    for (auto const & p : subIndex)
      stringsArray.emplace_back(ExtractString(p.second));

    m_counter++;
    Collect(index, args...);
  }

  template <typename... OtherStrings>
  void Collect(LocalizableStringIndex & index, Properties & properties,
               OtherStrings & ... args)
  {
    if (!SwitchSubIndexIfNeeded(index))
      return;

    auto subIndex = index[m_counter];
    auto const sz = static_cast<int8_t>(subIndex.size() / 2);
    for (int8_t i = 0; i < sz; i++)
    {
      properties.insert(std::make_pair(ExtractString(subIndex[2 * i]),
                                       ExtractString(subIndex[2 * i + 1])));
    }

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

  std::string ExtractString(uint32_t stringIndex) const
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
