#include "query_saver.hpp"

#include "platform/settings.hpp"

#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace
{
size_t constexpr kMaxSuggestCount = 10;
char constexpr kSettingsKey[] = "UserQueries";
using TLength = uint16_t;
size_t constexpr kLengthTypeSize = sizeof(TLength);
}  // namespace

namespace search
{
QuerySaver::QuerySaver()
{
  Load();
}

void QuerySaver::Add(string const & query)
{
  if (query.empty())
    return;

  // Remove items if needed.
  auto const it = find(m_topQueries.begin(), m_topQueries.end(), query);
  if (it != m_topQueries.end())
    m_topQueries.erase(it);
  else if (m_topQueries.size() >= kMaxSuggestCount)
    m_topQueries.pop_back();

  // Add new query and save it to drive.
  m_topQueries.push_front(query);
  Save();
}

void QuerySaver::Clear()
{
  m_topQueries.clear();
  Settings::Delete(kSettingsKey);
}

void QuerySaver::Serialize(vector<uint8_t> & data) const
{
  data.clear();
  MemWriter<vector<uint8_t>> writer(data);
  TLength size = m_topQueries.size();
  writer.Write(&size, kLengthTypeSize);
  for (auto const & query : m_topQueries)
  {
    size = query.size();
    writer.Write(&size, kLengthTypeSize);
    writer.Write(query.c_str(), size);
  }
}

void QuerySaver::Deserialize(string const & data)
{
  MemReader rawReader(data.c_str(), data.size());
  ReaderSource<MemReader> reader(rawReader);

  TLength queriesCount;
  reader.Read(&queriesCount, kLengthTypeSize);

  for (TLength i = 0; i < queriesCount; ++i)
  {
    TLength stringLength;
    reader.Read(&stringLength, kLengthTypeSize);
    vector<char> str(stringLength);
    reader.Read(&str[0], stringLength);
    m_topQueries.emplace_back(&str[0], stringLength);
  }
}

void QuerySaver::Save()
{
  vector<uint8_t> data;
  Serialize(data);
  Settings::Set(kSettingsKey, ToHex(&data[0], data.size()));
}

void QuerySaver::Load()
{
  string hexData;
  Settings::Get(kSettingsKey, hexData);
  if (hexData.empty())
    return;
  Deserialize(FromHex(hexData));
}
}  // namesapce search
