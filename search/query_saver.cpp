#include "query_saver.hpp"

#include "platform/settings.hpp"

#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

namespace
{
size_t constexpr kMaxSuggestCount = 10;
char constexpr kSettingsKey[] = "UserQueries";
}  // namespace

namespace search
{
QuerySaver::QuerySaver()
{
  m_topQueries.reserve(kMaxSuggestCount);
  Load();
}

void QuerySaver::SaveNewQuery(string const & query)
{
  if (query.empty())
    return;

  // Remove items if needed.
  auto const it = find(m_topQueries.begin(), m_topQueries.end(), query);
  if (it != m_topQueries.end())
    m_topQueries.erase(it);
  else if (m_topQueries.size() >= kMaxSuggestCount)
    m_topQueries.erase(m_topQueries.begin());

  // Add new query and save it to drive.
  m_topQueries.push_back(query);
  Save();
}

void QuerySaver::Clear()
{
  m_topQueries.clear();
  Settings::Delete(kSettingsKey);
}

void QuerySaver::Serialize(vector<char> & data) const
{
  data.clear();
  MemWriter<vector<char>> writer(data);
  uint16_t size = m_topQueries.size();
  writer.Write(&size, 2);
  for (auto const & query : m_topQueries)
  {
    size = query.size();
    writer.Write(&size, 2);
    writer.Write(query.c_str(), size);
  }
}

void QuerySaver::Deserialize(string const & data)
{
  MemReader reader(data.c_str(), data.size());

  uint16_t queriesCount;
  reader.Read(0 /* pos */, &queriesCount, 2);

  size_t pos = 2;
  for (int i = 0; i < queriesCount; ++i)
  {
    uint16_t stringLength;
    reader.Read(pos, &stringLength, 2);
    pos += 2;
    vector<char> str(stringLength);
    reader.Read(pos, &str[0], stringLength);
    pos += stringLength;
    m_topQueries.emplace_back(&str[0], stringLength);
  }
}

void QuerySaver::Save()
{
  vector<char> data;
  Serialize(data);
  string hexData = ToHex(&data[0], data.size());
  Settings::Set(kSettingsKey, hexData);
}

void QuerySaver::Load()
{
  string hexData;
  Settings::Get(kSettingsKey, hexData);
  if (hexData.empty())
    return;
  string rawData = FromHex(hexData);
  Deserialize(rawData);
}
}  // namesapce search
