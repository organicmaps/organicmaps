#include "query_saver.hpp"

#include "platform/settings.hpp"

#include "coding/base64.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace search
{

namespace
{
std::string_view constexpr kSettingsKey = "UserQueries";
using Length = uint16_t;
Length constexpr kMaxSuggestionsCount = 50;
}  // namespace

QuerySaver::QuerySaver()
{
  Load();
}

static bool IsInvalid(QuerySaver::SearchRequest const & localeAndQuery)
{
  constexpr size_t kMaxLocaleLength = 32;
  constexpr size_t kMaxQueryLength = 1024;
  return localeAndQuery.first.empty() || localeAndQuery.second.empty() ||
         localeAndQuery.first.size() > kMaxLocaleLength || localeAndQuery.second.size() > kMaxQueryLength;
}

void QuerySaver::Add(SearchRequest const & localeAndQuery)
{
  if (IsInvalid(localeAndQuery))
  {
    LOG(LERROR, ("Attempt to add invalid search query to history. Locale:", localeAndQuery.first,
                 "Query:", localeAndQuery.second));
    return;
  }

  // This change was made just before release, so we don't use untested search normalization methods.
  // TODO (ldragunov) Rewrite to normalized requests.
  SearchRequest trimmedQuery(localeAndQuery);
  auto & locale = trimmedQuery.first;
  auto & query = trimmedQuery.second;
  strings::Trim(locale);
  strings::Trim(query);

  auto const trimmedComparator = [&trimmedQuery](SearchRequest request)
  {
    strings::Trim(request.first);
    strings::Trim(request.second);
    return trimmedQuery == request;
  };
  // Remove items if needed.
  auto const it = std::find_if(m_topQueries.begin(), m_topQueries.end(), trimmedComparator);
  if (it != m_topQueries.end())
    m_topQueries.erase(it);
  else if (m_topQueries.size() >= kMaxSuggestionsCount)
    m_topQueries.pop_back();

  // Add new query and save it to drive.
  m_topQueries.push_front(localeAndQuery);
  Save();
}

void QuerySaver::Clear()
{
  m_topQueries.clear();
  settings::Delete(kSettingsKey);
}

void QuerySaver::Serialize(std::string & data) const
{
  std::vector<uint8_t> rawData;
  rawData.reserve(m_topQueries.size() * 64);
  MemWriter<std::vector<uint8_t>> writer(rawData);

  Length const count = std::min(static_cast<Length>(m_topQueries.size()), kMaxSuggestionsCount);
  WriteToSink(writer, count);

  Length written = 0;
  for (auto const & [locale, query] : m_topQueries)
  {
    if (written++ >= count)
      break;

    auto const localeSize = static_cast<Length>(locale.size());
    WriteToSink(writer, localeSize);
    writer.Write(locale.data(), localeSize);

    auto const querySize = static_cast<Length>(query.size());
    WriteToSink(writer, querySize);
    writer.Write(query.data(), querySize);
  }
  data = base64::Encode(std::string(rawData.begin(), rawData.end()));
}

void QuerySaver::Deserialize(std::string const & data)
{
  std::string const decodedData = base64::Decode(data);
  MemReaderWithExceptions rawReader(decodedData.data(), decodedData.size());
  ReaderSource<MemReaderWithExceptions> reader(rawReader);

  Length queriesCount = ReadPrimitiveFromSource<Length>(reader);
  queriesCount = std::min(queriesCount, kMaxSuggestionsCount);

  for (Length i = 0; i < queriesCount; ++i)
  {
    Length localeLength = ReadPrimitiveFromSource<Length>(reader);
    std::string locale;
    if (localeLength == 0) [[unlikely]]
    {
      // There is some unknown edge case when locale was saved as empty.
      LOG(LWARNING, ("Empty locale in search history converted to `en`, entry index:", i));
      locale = "en";
    }
    else
    {
      locale.resize(localeLength);
      reader.Read(locale.data(), localeLength);
    }

    Length stringLength = ReadPrimitiveFromSource<Length>(reader);
    std::string str;
    if (stringLength > 0) [[likely]]
    {
      str.resize(stringLength);
      reader.Read(str.data(), stringLength);
      m_topQueries.emplace_back(std::move(locale), std::move(str));
    }
    else
    {
      LOG(LWARNING, ("Skip loading of an empty query string in search history, entry index:", i));
    }
  }
}

void QuerySaver::Save()
{
  std::string data;
  Serialize(data);
  settings::Set(kSettingsKey, data);
}

void QuerySaver::Load()
{
  std::string base64Data;
  if (!settings::Get(kSettingsKey, base64Data) || base64Data.empty())
    return;

  try
  {
    Deserialize(base64Data);
  }
  catch (RootException const & ex)
  {
    Clear();
    LOG(LERROR, ("Search history data corrupted! Creating new one. Error:", ex.Msg()));
  }
}
}  // namespace search
