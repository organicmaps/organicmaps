#include "kml/serdes_binary.hpp"

namespace kml
{
namespace binary
{
namespace
{
using DeserializeException = DeserializerKml::DeserializeException;

void ReadV10UnknownField(NonOwningReaderSource & source)
{
  // All available samples encode zero here. Its type is not yet known: reading a
  // nonzero value as a byte could misalign a varint or a container that follows it.
  if (ReadPrimitiveFromSource<uint8_t>(source) != 0)
    MYTHROW(DeserializeException, ("Unsupported nonzero field in MapsMe V10 KMB."));
}

void CheckV10SectionEnd(NonOwningReaderSource const & source)
{
  if (source.Size() != 0)
    MYTHROW(DeserializeException, ("Unsupported trailing data in MapsMe V10 KMB section."));
}

template <size_t IndexSize, typename... Strings>
void ReadV10Strings(NonOwningReaderSource & source, coding::BlockedTextStorage<Reader> & strings, Strings &... fields)
{
  LocalizableStringIndex index;
  ReadLocalizableStringIndex(source, index);
  if (index.size() != IndexSize)
    MYTHROW(DeserializeException, ("Unsupported string index in MapsMe V10 KMB."));

  // Known slots: category name, bookmark name/description/customName, track name/description.
  // The other slots are empty in the samples; do not map them to legacy metadata.
  for (size_t i = sizeof...(fields); i < index.size(); ++i)
    for (auto const & [lang, stringId] : index[i])
      if (stringId != kEmptyStringId)
        MYTHROW(DeserializeException, ("Unsupported metadata in MapsMe V10 KMB."));

  DeserializedStringCollector<Reader> collector(strings);
  collector.Collect(index, fields...);
}
}  // namespace

SerializerKml::SerializerKml(FileData const & data) : m_data(data)
{
  // V8/V9 binary tracks store one line and no per-point times. Reject data that
  // cannot be represented before writing output or collecting strings.
  for (auto const & track : data.m_tracksData)
    if (track.m_geometry.m_lines.size() != 1 || track.m_geometry.HasTimestamps())
      MYTHROW(SerializeException, ("V8/V9 KMB cannot store multiple track lines or per-point timestamps."));

  ClearCollectionIndex();

  // Collect all strings and substitute each for index.
  auto const avgSz = data.m_bookmarksData.size() * 2 + data.m_tracksData.size() * 2 + 10;
  LocalizableStringCollector collector(avgSz);
  CollectorVisitor<decltype(collector)> visitor(collector);
  m_data.Visit(visitor);
  m_strings = collector.StealCollection();
}

SerializerKml::~SerializerKml()
{
  ClearCollectionIndex();
}

void SerializerKml::ClearCollectionIndex()
{
  LocalizableStringCollector collector(0);
  CollectorVisitor<decltype(collector)> clearVisitor(collector, true /* clear index */);
  m_data.Visit(clearVisitor);
}

DeserializerKml::DeserializerKml(FileData & data) : m_data(data)
{
  m_data = {};
}

void DeserializerKml::DeserializeV10MM(std::unique_ptr<Reader> & reader)
{
  // Keep this partially reverse-engineered format separate from the older visitors.
  // Check each record section for bytes that the known layout cannot explain.
  if (m_header.m_categoryOffset != 40 || m_header.m_categoryOffset > m_header.m_bookmarksOffset ||
      m_header.m_bookmarksOffset > m_header.m_tracksOffset || m_header.m_tracksOffset > m_header.m_stringsOffset ||
      m_header.m_stringsOffset > m_header.m_eosOffset || m_header.m_eosOffset != reader->Size())
  {
    MYTHROW(DeserializeException, ("Unsupported section layout in MapsMe V10 KMB."));
  }

  auto stringsReader = CreateStringsSubReader(*reader);
  coding::BlockedTextStorage<Reader> strings(*stringsReader);
  FileData data;

  DeserializeCategoryV10MM(*reader, strings, data);
  DeserializeBookmarksV10MM(*reader, strings, data);
  DeserializeTracksV10MM(*reader, strings, data);

  // Publish only after all record sections have been read without unsupported content.
  m_data = std::move(data);
}

void DeserializerKml::DeserializeCategoryV10MM(Reader const & reader, coding::BlockedTextStorage<Reader> & strings,
                                               FileData & data)
{
  auto categoryReader = CreateCategorySubReader(reader);
  NonOwningReaderSource categorySource(*categoryReader);
  CategoryDeserializerVisitor categoryVisitor(categorySource, m_doubleBits);
  auto & category = data.m_categoryData;
  categoryVisitor(category.m_id);
  categoryVisitor(category.m_visible);
  ReadV10UnknownField(categorySource);
  TimestampMillis lastModified;
  categoryVisitor(lastModified);
  category.m_lastModified = lastModified;
  ReadV10UnknownField(categorySource);
  ReadV10Strings<5>(categorySource, strings, category.m_name);
  if (GetStringForExport(category.m_name).empty())
    category.m_name.clear();  // Allow the application's filename fallback.
  CheckV10SectionEnd(categorySource);
}

void DeserializerKml::DeserializeBookmarksV10MM(Reader const & reader, coding::BlockedTextStorage<Reader> & strings,
                                                FileData & data)
{
  auto bookmarksReader = CreateBookmarkSubReader(reader);
  NonOwningReaderSource bookmarksSource(*bookmarksReader);
  BookmarkDeserializerVisitor bookmarkVisitor(bookmarksSource, m_doubleBits);
  auto const count = ReadVarUint<uint32_t>(bookmarksSource);
  data.m_bookmarksData.reserve(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    auto & bookmark = data.m_bookmarksData.emplace_back();
    bookmarkVisitor(bookmark.m_id);
    bookmarkVisitor(bookmark.m_featureTypes);
    bookmarkVisitor(bookmark.m_color);
    bookmarkVisitor(bookmark.m_icon);
    bookmarkVisitor(bookmark.m_viewportScale);
    TimestampMillis created;
    bookmarkVisitor(created);
    bookmark.m_timestamp = created;
    bookmarkVisitor(bookmark.m_point);
    bookmarkVisitor(bookmark.m_visible);
    ReadV10UnknownField(bookmarksSource);
    // An additional epoch-millisecond timestamp, possibly modification/sync time.
    // Consume it separately so that the original creation time is preserved.
    ReadVarUint<uint64_t>(bookmarksSource);
    ReadV10Strings<5>(bookmarksSource, strings, bookmark.m_name, bookmark.m_description, bookmark.m_customName);
  }
  CheckV10SectionEnd(bookmarksSource);
}

void DeserializerKml::DeserializeTracksV10MM(Reader const & reader, coding::BlockedTextStorage<Reader> & strings,
                                             FileData & data)
{
  auto trackReader = CreateTrackSubReader(reader);
  NonOwningReaderSource trackSource(*trackReader);
  BookmarkDeserializerVisitor trackVisitor(trackSource, m_doubleBits);
  auto const trackCount = ReadVarUint<uint32_t>(trackSource);
  data.m_tracksData.reserve(trackCount);
  for (uint32_t i = 0; i < trackCount; ++i)
  {
    // V10 retains V9MM's geometry and packed timestamps, but adds two fields
    // before the string index. Reuse its conversion without its record visitor.
    TrackDataV9MM track;
    trackVisitor(track.m_id);
    trackVisitor(track.m_localId);
    trackVisitor(track.m_layers);
    trackVisitor(track.m_timestamp);
    trackVisitor(track.m_multiGeometry);
    trackVisitor(track.m_visible);
    ReadV10UnknownField(trackSource);
    ReadV10UnknownField(trackSource);
    trackVisitor(track.m_pointTimestamps);
    // Both V10-only fields are zero in the available track samples. Unlike the
    // bookmark timestamp, their types and meanings are not established yet.
    ReadV10UnknownField(trackSource);
    ReadV10UnknownField(trackSource);
    ReadV10Strings<4>(trackSource, strings, track.m_name, track.m_description);

    // A GPX import may store its document's metadata time as a singleton here,
    // even though none of the points have times. Preserve it as the track time,
    // not as a one-element per-point array (or as a time repeated for every point).
    size_t pointCount = 0;
    for (auto const & geometry : track.m_multiGeometry)
      for (auto const & line : geometry.m_lines)
        pointCount += line.size();
    if (track.m_pointTimestamps.m_values.size() == 1 && pointCount > 1)
    {
      if (track.m_timestamp == Timestamp{})
        track.m_timestamp = FromSecondsSinceEpoch(track.m_pointTimestamps.m_values.front());
      track.m_pointTimestamps.m_values.clear();
    }

    // Imported GPX may name only the category. Empty localized strings otherwise
    // prevent the application's empty-name fallback from running.
    if (GetStringForExport(track.m_name).empty())
      track.m_name = trackCount == 1 ? data.m_categoryData.m_name : LocalizableString{};

    data.m_tracksData.emplace_back(track.ConvertToLatestVersion());
  }
  CheckV10SectionEnd(trackSource);
}
}  // namespace binary
}  // namespace kml
